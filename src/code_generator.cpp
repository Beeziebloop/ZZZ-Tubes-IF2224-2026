#include "code_generator.hpp"

#include <cstring>
#include <stdexcept>
#include <iomanip>

CodeGenerator::CodeGenerator(const SymbolTable& symtab)
    : symtab_(symtab), currentLevel_(0) {}

Code CodeGenerator::generate(ASTNode* root) {
    code_.clear();
    currentLevel_ = 0;
    if (!root) return code_;

    if (root->kind == ASTKind::Program)
        genProgram(static_cast<ProgramNode*>(root));
    else
        throw std::runtime_error("CodeGenerator: root bukan ProgramNode");

    return code_;
}

void CodeGenerator::printCode(const Code& code, std::ostream& out) const {
    out << "\n--- INTERMEDIATE CODE ---\n";
    out << std::left
        << std::setw(6) << "IDX"
        << std::setw(8) << "OP"
        << std::setw(8) << "LEVEL"
        << std::setw(8) << "VAL"
        << "\n";
    for (int i = 0; i < (int)code.size(); i++) {
        out << std::setw(6) << i
            << std::setw(8) << code[i].op
            << std::setw(8) << code[i].level
            << std::setw(8) << code[i].val
            << "\n";
    }
}

int CodeGenerator::emit(const std::string& op, int level, int val) {
    code_.push_back({op, level, val});
    return (int)code_.size() - 1;
}

void CodeGenerator::patch(int addr, int val) {
    code_[addr].val = val;
}

int CodeGenerator::levelDiff(int targetLevel) const {
    return currentLevel_ - targetLevel;
}

// Offset absolut variabel dalam frame (3 = SL,DL,RA).
// Hanya menghitung variabel dalam subprogram/blok yang sama.
int CodeGenerator::getVarOffset(int tabIdx) const {
    if (tabIdx < 0 || tabIdx >= (int)symtab_.tab.size()) return 3;
    int targetLev = symtab_.tab[tabIdx].lev;

    int ownerIdx = 32;
    for (int i = tabIdx; i >= 33; --i) {
        const TabEntry& e = symtab_.tab[i];
        if ((e.obj == OBJ_PROCEDURE || e.obj == OBJ_FUNCTION) && e.lev < targetLev) {
            ownerIdx = i;
            break;
        }
    }

    int offset = 0;
    for (int i = ownerIdx + 1; i < tabIdx; ++i) {
        const TabEntry& e = symtab_.tab[i];
        if (e.lev == targetLev && e.obj == OBJ_VARIABLE)
            ++offset;
    }
    return offset + 3;
}

void CodeGenerator::genProgram(ProgramNode* node) {
    int vsze = symtab_.btab.empty() ? 0 : symtab_.btab[0].vsze;

    int jmpToMain = emit("JMP", 0, 0);
    genDeclarations(node->declarations);
    patch(jmpToMain, (int)code_.size());

    emit("INT", 0, vsze + 3); // +3: SL, DL, RA
    if (node->block)
        genBlock(static_cast<BlockNode*>(node->block.get()));
    emit("OPR", 0, OPR_RET);

    for (auto& [tabIdx, callSites] : forwardCalls_) {
        auto it = entryPoints_.find(tabIdx);
        if (it != entryPoints_.end())
            for (int callIdx : callSites)
                patch(callIdx, it->second);
    }
    forwardCalls_.clear();
}

void CodeGenerator::genBlock(BlockNode* node) {
    for (auto& stmt : node->statements)
        genStatement(stmt.get());
}

void CodeGenerator::genDeclarations(const std::vector<ASTPtr>& decls) {
    for (auto& decl : decls) {
        if (!decl) continue;
        if (decl->kind == ASTKind::ProcedureDecl)
            genProcDecl(static_cast<ProcedureDeclNode*>(decl.get()));
        else if (decl->kind == ASTKind::FunctionDecl)
            genFuncDecl(static_cast<FunctionDeclNode*>(decl.get()));
    }
}

// Hitung variabel (termasuk parameter) milik subprogram: entri setelah tabIdx
// hingga subprogram lain di level lexical yang sama.
static int getSubprogramVarSize(const SymbolTable& st, int tabIdx) {
    if (tabIdx < 0 || tabIdx >= (int)st.tab.size()) return 0;
    int fnLev = st.tab[tabIdx].lev;
    int varLev = fnLev + 1;
    int count = 0;
    for (int i = tabIdx + 1; i < (int)st.tab.size(); ++i) {
        const TabEntry& e = st.tab[i];
        if ((e.obj == OBJ_PROCEDURE || e.obj == OBJ_FUNCTION) && e.lev == fnLev)
            break;
        if (e.lev == varLev && e.obj == OBJ_VARIABLE)
            ++count;
    }
    return count;
}

void CodeGenerator::genProcDecl(ProcedureDeclNode* node) {
    int tabIdx = node->tabIndex;
    int entryPoint = (int)code_.size();

    currentLevel_++;
    emit("INT", 0, getSubprogramVarSize(symtab_, tabIdx) + 3);
    genDeclarations(node->declarations);
    if (node->block)
        genBlock(static_cast<BlockNode*>(node->block.get()));
    emit("RET", 0, 0);
    currentLevel_--;

    if (tabIdx >= 0) entryPoints_[tabIdx] = entryPoint;
}

void CodeGenerator::genFuncDecl(FunctionDeclNode* node) {
    int tabIdx = node->tabIndex;
    int entryPoint = (int)code_.size();

    currentLevel_++;
    emit("INT", 0, getSubprogramVarSize(symtab_, tabIdx) + 3);
    genDeclarations(node->declarations);
    if (node->block)
        genBlock(static_cast<BlockNode*>(node->block.get()));
    emit("RET", 0, 0);
    currentLevel_--;

    if (tabIdx >= 0) entryPoints_[tabIdx] = entryPoint;
}

void CodeGenerator::genStatement(ASTNode* node) {
    if (!node) return;
    switch (node->kind) {
        case ASTKind::Assign:
            genAssign(static_cast<AssignNode*>(node)); break;
        case ASTKind::ProcedureCall:
            genProcedureCall(static_cast<ProcedureCallNode*>(node)); break;
        case ASTKind::IfStatement:
            genIf(static_cast<IfNode*>(node)); break;
        case ASTKind::WhileStatement:
            genWhile(static_cast<WhileNode*>(node)); break;
        case ASTKind::RepeatStatement:
            genRepeat(static_cast<RepeatNode*>(node)); break;
        case ASTKind::ForStatement:
            genFor(static_cast<ForNode*>(node)); break;
        case ASTKind::CaseStatement:
            genCase(static_cast<CaseNode*>(node)); break;
        case ASTKind::Block:
            genBlock(static_cast<BlockNode*>(node)); break;
        case ASTKind::Empty:
            break;
        default:
            genExpr(node); break;
    }
}

void CodeGenerator::genAssign(AssignNode* node) {
    genExpr(node->value.get());
    genLValue(node->target.get());
}

void CodeGenerator::genProcedureCall(ProcedureCallNode* node) {
    std::string nameLow = node->name;
    for (char& c : nameLow) c = (char)tolower((unsigned char)c);

    if (nameLow == "writeln") {
        for (auto& arg : node->args) { genExpr(arg.get()); emit("OPR", 0, OPR_WRT); }
        emit("OPR", 0, OPR_WRTLN);
        return;
    }
    if (nameLow == "write") {
        for (auto& arg : node->args) { genExpr(arg.get()); emit("OPR", 0, OPR_WRT); }
        return;
    }
    if (nameLow == "readln" || nameLow == "read") {
        for (auto& arg : node->args) { emit("OPR", 0, 13); genLValue(arg.get()); }
        return;
    }

    for (auto& arg : node->args) genExpr(arg.get());

    int tabIdx = node->tabIndex;
    int targetLevel = (tabIdx >= 0 && tabIdx < (int)symtab_.tab.size())
                      ? symtab_.tab[tabIdx].lev : 0;
    int calIdx = emit("CAL", levelDiff(targetLevel), 0);

    auto it = entryPoints_.find(tabIdx);
    if (it != entryPoints_.end())
        patch(calIdx, it->second);
    else
        forwardCalls_[tabIdx].push_back(calIdx);
}

void CodeGenerator::genIf(IfNode* node) {
    genExpr(node->condition.get());
    int jpc = emit("JPC", 0, 0);
    genStatement(node->thenBranch.get());
    if (node->elseBranch) {
        int jmp = emit("JMP", 0, 0);
        patch(jpc, (int)code_.size());
        genStatement(node->elseBranch.get());
        patch(jmp, (int)code_.size());
    } else {
        patch(jpc, (int)code_.size());
    }
}

void CodeGenerator::genWhile(WhileNode* node) {
    int loopStart = (int)code_.size();
    genExpr(node->condition.get());
    int jpc = emit("JPC", 0, 0);
    genStatement(node->body.get());
    emit("JMP", 0, loopStart);
    patch(jpc, (int)code_.size());
}

void CodeGenerator::genRepeat(RepeatNode* node) {
    int loopStart = (int)code_.size();
    genStatement(node->body.get());
    genExpr(node->condition.get());
    // JPC ke loopStart jika kondisi false (belum terpenuhi)
    emit("JPC", 0, loopStart);
}

void CodeGenerator::genFor(ForNode* node) {
    int tabIdx = symtab_.lookup(node->iterName);
    int iterLev = (tabIdx >= 0) ? symtab_.tab[tabIdx].lev : currentLevel_;
    int iterAdr = (tabIdx >= 0) ? getVarOffset(tabIdx) : 0;
    int diff = levelDiff(iterLev);

    genExpr(node->startExpr.get());
    emit("STO", diff, iterAdr);

    int loopStart = (int)code_.size();
    emit("LOD", diff, iterAdr);
    genExpr(node->endExpr.get());
    emit("OPR", 0, node->isDownTo ? OPR_GEQ : OPR_LEQ);
    int jpc = emit("JPC", 0, 0);

    genStatement(node->body.get());

    emit("LOD", diff, iterAdr);
    emit("LIT", 0, 1);
    emit("OPR", 0, node->isDownTo ? OPR_SUB : OPR_ADD);
    emit("STO", diff, iterAdr);

    emit("JMP", 0, loopStart);
    patch(jpc, (int)code_.size());
}

void CodeGenerator::genCase(CaseNode* node) {
    std::vector<int> jmpEnds;

    for (auto& armPtr : node->arms) {
        CaseArmNode* arm = static_cast<CaseArmNode*>(armPtr.get());
        std::vector<int> jmpToBody, jpcToNext;

        for (size_t li = 0; li < arm->labels.size(); li++) {
            if (li > 0) patch(jpcToNext.back(), (int)code_.size());
            genExpr(node->selector.get());
            genExpr(arm->labels[li].get());
            emit("OPR", 0, OPR_EQL);
            jpcToNext.push_back(emit("JPC", 0, 0));
            jmpToBody.push_back(emit("JMP", 0, 0));
        }

        int skipArm = emit("JMP", 0, 0);
        patch(jpcToNext.back(), (int)code_.size());

        int bodyAddr = (int)code_.size();
        for (int j : jmpToBody) patch(j, bodyAddr);

        genStatement(arm->body.get());
        jmpEnds.push_back(emit("JMP", 0, 0));

        patch(skipArm, (int)code_.size());
    }

    int endAddr = (int)code_.size();
    for (int idx : jmpEnds) patch(idx, endAddr);
}

void CodeGenerator::genExpr(ASTNode* node) {
    if (!node) return;
    switch (node->kind) {
        case ASTKind::Num:
            genNum(static_cast<NumNode*>(node)); break;
        case ASTKind::Boolean:
            emit("LIT", 0, static_cast<BooleanNode*>(node)->value ? 1 : 0); break;
        case ASTKind::Char: {
            const std::string& v = static_cast<CharNode*>(node)->value;
            emit("LIT", 0, (int)(unsigned char)(v.empty() ? 0 : v[0]));
            break;
        }
        case ASTKind::String: {
            int idx = (int)stringPool_.size();
            stringPool_.push_back(static_cast<StringNode*>(node)->value);
            emit("LIT", 0, idx);
            break;
        }
        case ASTKind::Var:
            genVar(static_cast<VarNode*>(node)); break;
        case ASTKind::BinOp:
            genBinOp(static_cast<BinOpNode*>(node)); break;
        case ASTKind::UnaryOp:
            genUnaryOp(static_cast<UnaryOpNode*>(node)); break;
        case ASTKind::ArrayAccess:
            genArrayAccess(static_cast<ArrayAccessNode*>(node)); break;
        case ASTKind::FieldAccess:
            emit("LIT", 0, 0); break; // TODO: record support
        case ASTKind::ProcedureCall:
            genProcedureCall(static_cast<ProcedureCallNode*>(node)); break;
        default:
            emit("LIT", 0, 0); break;
    }
}

void CodeGenerator::genNum(NumNode* node) {
    if (node->isReal) {
        float fval = (float)std::stod(node->rawValue);
        int bits;
        static_assert(sizeof(float) == sizeof(int), "");
        memcpy(&bits, &fval, sizeof(int));
        emit("LIT", 0, bits);
    } else {
        emit("LIT", 0, std::stoi(node->rawValue));
    }
}

void CodeGenerator::genVar(VarNode* node) {
    int tabIdx = node->tabIndex;
    if (tabIdx < 0) tabIdx = symtab_.lookup(node->name);
    if (tabIdx < 0 || tabIdx >= (int)symtab_.tab.size()) { emit("LIT", 0, 0); return; }

    const TabEntry& e = symtab_.tab[tabIdx];
    if (e.obj == OBJ_CONSTANT) { emit("LIT", 0, e.adr); return; }
    emit("LOD", levelDiff(e.lev), getVarOffset(tabIdx));
}

void CodeGenerator::genBinOp(BinOpNode* node) {
    genExpr(node->left.get());
    genExpr(node->right.get());

    const std::string& op = node->op;
    if      (op == "+")   emit("OPR", 0, OPR_ADD);
    else if (op == "-")   emit("OPR", 0, OPR_SUB);
    else if (op == "*")   emit("OPR", 0, OPR_MUL);
    else if (op == "/" || op == "div") emit("OPR", 0, OPR_DIV);
    else if (op == "mod") emit("OPR", 0, OPR_MOD);
    else if (op == "=")   emit("OPR", 0, OPR_EQL);
    else if (op == "<>")  emit("OPR", 0, OPR_NEQ);
    else if (op == "<")   emit("OPR", 0, OPR_LSS);
    else if (op == "<=")  emit("OPR", 0, OPR_LEQ);
    else if (op == ">")   emit("OPR", 0, OPR_GTR);
    else if (op == ">=")  emit("OPR", 0, OPR_GEQ);
    else if (op == "and") emit("OPR", 0, OPR_MUL);  // 0/1 * 0/1
    else if (op == "or")  { emit("OPR", 0, OPR_ADD); emit("LIT", 0, 0); emit("OPR", 0, OPR_NEQ); }
    else emit("LIT", 0, 0);
}

void CodeGenerator::genUnaryOp(UnaryOpNode* node) {
    genExpr(node->operand.get());
    if      (node->op == "-")   emit("OPR", 0, OPR_NEG);
    else if (node->op == "not") { emit("LIT", 0, 0); emit("OPR", 0, OPR_EQL); }
    // unary +: no-op
}

void CodeGenerator::genArrayAccess(ArrayAccessNode* node) {
    if (!node->base || node->base->kind != ASTKind::Var) { emit("LIT", 0, 0); return; }
    VarNode* baseVar = static_cast<VarNode*>(node->base.get());

    int tabIdx = baseVar->tabIndex;
    if (tabIdx < 0) tabIdx = symtab_.lookup(baseVar->name);
    if (tabIdx < 0 || tabIdx >= (int)symtab_.tab.size()) { emit("LIT", 0, 0); return; }

    const TabEntry& e = symtab_.tab[tabIdx];
    int ataIdx = e.ref;
    int low = (ataIdx > 0 && ataIdx < (int)symtab_.atab.size()) ? symtab_.atab[ataIdx].low : 0;

    if (!node->indices.empty()) {
        genExpr(node->indices[0].get());
        emit("LIT", 0, low);
        emit("OPR", 0, OPR_SUB);
    } else {
        emit("LIT", 0, 0);
    }
    emit("LODA", levelDiff(e.lev), getVarOffset(tabIdx));
}

void CodeGenerator::genLValue(ASTNode* node) {
    if (!node) return;

    if (node->kind == ASTKind::Var) {
        VarNode* vn = static_cast<VarNode*>(node);
        int tabIdx = vn->tabIndex;
        if (tabIdx < 0) tabIdx = symtab_.lookup(vn->name);
        if (tabIdx < 0 || tabIdx >= (int)symtab_.tab.size()) return;
        const TabEntry& e = symtab_.tab[tabIdx];
        if (e.obj == OBJ_FUNCTION) {
            // Nilai kembalian fungsi disimpan di slot pertama frame aktif (offset 3).
            emit("STO", 0, 3);
            return;
        }
        emit("STO", levelDiff(e.lev), getVarOffset(tabIdx));

    } else if (node->kind == ASTKind::ArrayAccess) {
        ArrayAccessNode* an = static_cast<ArrayAccessNode*>(node);
        if (!an->base || an->base->kind != ASTKind::Var) return;
        VarNode* baseVar = static_cast<VarNode*>(an->base.get());

        int tabIdx = baseVar->tabIndex;
        if (tabIdx < 0) tabIdx = symtab_.lookup(baseVar->name);
        if (tabIdx < 0 || tabIdx >= (int)symtab_.tab.size()) return;

        const TabEntry& e = symtab_.tab[tabIdx];
        int ataIdx = e.ref;
        int low = (ataIdx > 0 && ataIdx < (int)symtab_.atab.size()) ? symtab_.atab[ataIdx].low : 0;

        if (!an->indices.empty()) {
            genExpr(an->indices[0].get());
            emit("LIT", 0, low);
            emit("OPR", 0, OPR_SUB);
        } else {
            emit("LIT", 0, 0);
        }
        emit("STOA", levelDiff(e.lev), getVarOffset(tabIdx));
    }
}

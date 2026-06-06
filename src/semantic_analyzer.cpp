#include "semantic_analyzer.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

using namespace std;

// Entry
ASTPtr SemanticAnalyzer::analyze(ParseNode *root)
{
    if (root == nullptr)
    {
        semanticError("parse tree root is null");
        return nullptr;
    }

    if (!isLabel(root, "<program>"))
    {
        semanticError("expected root node <program>");
        return nullptr;
    }

    return visitProgram(root);
}

// Parse-tree helpers
bool SemanticAnalyzer::isLabel(ParseNode *node, const string &label) const
{
    return node && node->label == label;
}

bool SemanticAnalyzer::isTerminal(ParseNode *node) const
{
    if (!node)
        return false;
    return !node->label.empty() && node->label.front() != '<';
}

bool SemanticAnalyzer::containsTerminal(ParseNode *node, const string &terminal) const
{
    return firstTerminal(node, terminal) != nullptr;
}

string SemanticAnalyzer::terminalName(ParseNode *node) const
{
    if (!node)
        return "";

    const string &label = node->label;
    size_t pos = label.find('(');
    if (pos == string::npos)
        return label;
    return label.substr(0, pos);
}

string SemanticAnalyzer::terminalValue(ParseNode *node) const
{
    if (!node)
        return "";

    const string &label = node->label;
    size_t open = label.find('(');
    size_t close = label.rfind(')');

    if (open == string::npos || close == string::npos || close <= open)
        return "";

    string value = label.substr(open + 1, close - open - 1);
    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'')
        value = value.substr(1, value.size() - 2);

    return value;
}

string SemanticAnalyzer::lower(string s) const
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
              { return static_cast<char>(tolower(c)); });
    return s;
}

ParseNode *SemanticAnalyzer::child(ParseNode *node, const string &label) const
{
    if (!node)
        return nullptr;

    for (ParseNode *ch : node->children)
    {
        if (ch->label == label)
            return ch;
    }
    return nullptr;
}

vector<ParseNode *> SemanticAnalyzer::children(ParseNode *node, const string &label) const
{
    vector<ParseNode *> result;
    if (!node)
        return result;

    for (ParseNode *ch : node->children)
    {
        if (ch->label == label)
            result.push_back(ch);
    }
    return result;
}

ParseNode *SemanticAnalyzer::firstTerminal(ParseNode *node, const string &terminal) const
{
    if (!node)
        return nullptr;

    if (isTerminal(node) && terminalName(node) == terminal)
        return node;

    for (ParseNode *ch : node->children)
    {
        ParseNode *found = firstTerminal(ch, terminal);
        if (found)
            return found;
    }
    return nullptr;
}

vector<ParseNode *> SemanticAnalyzer::allTerminals(ParseNode *node, const string &terminal) const
{
    vector<ParseNode *> result;
    collectTerminals(node, terminal, result);
    return result;
}

void SemanticAnalyzer::collectTerminals(ParseNode *node,
                                        const string &terminal,
                                        vector<ParseNode *> &out) const
{
    if (!node)
        return;

    if (isTerminal(node) && terminalName(node) == terminal)
        out.push_back(node);

    for (ParseNode *ch : node->children)
        collectTerminals(ch, terminal, out);
}

vector<string> SemanticAnalyzer::identifierList(ParseNode *node) const
{
    vector<string> result;
    if (!node)
        return result;

    for (ParseNode *ident : allTerminals(node, "ident"))
        result.push_back(terminalValue(ident));

    return result;
}

string SemanticAnalyzer::operatorFromNode(ParseNode *node) const
{
    if (!node)
        return "?";

    if (isTerminal(node))
    {
        const string name = terminalName(node);
        if (name == "plus")
            return "+";
        if (name == "minus")
            return "-";
        if (name == "times")
            return "*";
        if (name == "rdiv")
            return "/";
        if (name == "idiv")
            return "div";
        if (name == "imod")
            return "mod";
        if (name == "andsy")
            return "and";
        if (name == "orsy")
            return "or";
        if (name == "eql")
            return "=";
        if (name == "neq")
            return "<>";
        if (name == "gtr")
            return ">";
        if (name == "geq")
            return ">=";
        if (name == "lss")
            return "<";
        if (name == "leq")
            return "<=";
        return name;
    }

    for (ParseNode *ch : node->children)
    {
        string op = operatorFromNode(ch);
        if (op != "?")
            return op;
    }
    return "?";
}

string SemanticAnalyzer::getVariableName(ParseNode *varNode) const
{
    if (!varNode)
        return "";

    if (ParseNode *ident = firstTerminal(varNode, "ident"))
        return terminalValue(ident);

    return "";
}

void SemanticAnalyzer::moveAppend(vector<ASTPtr> &dst, vector<ASTPtr> &src)
{
    for (auto &item : src)
        dst.push_back(move(item));
    src.clear();
}

// Symbol & type checking
int SemanticAnalyzer::lookupSymbol(const string &name, int line)
{
    int idx = symtab.lookup(name);
    if (idx == -1)
        semanticError("undeclared identifier '" + name + "'", line);
    return idx;
}

void SemanticAnalyzer::enterVariable(const string &name, TypeCode type)
{
    symtab.enter(name, OBJ_VARIABLE, type, 0, 1, 0);
}

void SemanticAnalyzer::enterConstant(const string &name, TypeCode type, int adr)
{
    symtab.enter(name, OBJ_CONSTANT, type, 0, 1, adr);
}

void SemanticAnalyzer::enterTypeAlias(const string &name, TypeCode type)
{
    symtab.enter(name, OBJ_TYPE, type, 0, 1, 0);
}

int SemanticAnalyzer::enterSubprogram(const string &name, ObjClass obj, TypeCode retType)
{
    return symtab.enter(name, obj, retType, 0, 1, 0);
}

void SemanticAnalyzer::decorateVar(ASTNode *node, const string &name)
{
    if (!node)
        return;

    int idx = symtab.lookup(name);
    if (idx >= 0)
    {
        node->tabIndex = idx;
        node->lexicalLevel = symtab.tab[idx].lev;
    }
}

TypeCode SemanticAnalyzer::resolveTypeName(const string &name)
{
    const string n = lower(name);

    if (n == "integer")
        return TYPE_INTEGER;
    if (n == "real")
        return TYPE_REAL;
    if (n == "char")
        return TYPE_CHAR;
    if (n == "boolean")
        return TYPE_BOOLEAN;
    if (n == "string")
        return TYPE_STRING;

    int idx = symtab.lookup(name);
    if (idx >= 0 && symtab.tab[idx].obj == OBJ_TYPE)
        return symtab.tab[idx].type;

    return TYPE_NONE;
}

TypeCode SemanticAnalyzer::resolveType(ParseNode *typeNode)
{
    if (!typeNode)
        return TYPE_NONE;

    if (child(typeNode, "<array_type>"))
        return TYPE_ARRAY;
    if (child(typeNode, "<record_type>"))
        return TYPE_RECORD;
    if (child(typeNode, "<range>"))
        return TYPE_SUBRANGE;
    if (child(typeNode, "<enumerated>"))
        return TYPE_ENUM;

    if (ParseNode *ident = firstTerminal(typeNode, "ident"))
        return resolveTypeName(terminalValue(ident));

    return TYPE_NONE;
}

TypeCode SemanticAnalyzer::getTypeFromAST(const ASTNode *node)
{
    if (!node)
        return TYPE_NONE;

    switch (node->kind)
    {
    case ASTKind::Num:
        return static_cast<const NumNode *>(node)->isReal ? TYPE_REAL : TYPE_INTEGER;
    case ASTKind::Char:
        return TYPE_CHAR;
    case ASTKind::String:
        return TYPE_STRING;
    case ASTKind::Boolean:
        return TYPE_BOOLEAN;
    case ASTKind::Var:
    {
        const auto *vn = static_cast<const VarNode *>(node);
        if (node->tabIndex >= 0 && node->tabIndex < static_cast<int>(symtab.tab.size()))
            return symtab.tab[node->tabIndex].type;
        int idx = symtab.lookup(vn->name);
        if (idx >= 0)
            return symtab.tab[idx].type;
        return TYPE_NONE;
    }
    case ASTKind::BinOp:
    {
        const auto *bn = static_cast<const BinOpNode *>(node);
        return resultTypeOfBinOp(bn->op,
                                 getTypeFromAST(bn->left.get()),
                                 getTypeFromAST(bn->right.get()));
    }
    case ASTKind::UnaryOp:
    {
        const auto *un = static_cast<const UnaryOpNode *>(node);
        if (un->op == "not")
            return TYPE_BOOLEAN;
        if (un->op == "-")
            return getTypeFromAST(un->operand.get());
        return TYPE_NONE;
    }
    default:
        return TYPE_NONE;
    }
}

bool SemanticAnalyzer::isCompatible(TypeCode t1, TypeCode t2)
{
    if (t1 == t2)
        return true;
    if ((t1 == TYPE_REAL && t2 == TYPE_INTEGER) || (t1 == TYPE_INTEGER && t2 == TYPE_REAL))
        return true;
    if (t1 == TYPE_ENUM && t2 == TYPE_ENUM)
        return true;
    return false;
}

bool SemanticAnalyzer::isRelationalCompatible(TypeCode t1, TypeCode t2)
{
    if (t1 == t2)
        return true;
    // integer dan real boleh dibandingkan satu sama lain
    if ((t1 == TYPE_INTEGER || t1 == TYPE_REAL) && (t2 == TYPE_INTEGER || t2 == TYPE_REAL))
        return true;
    if (t1 == TYPE_ENUM && t2 == TYPE_ENUM)
        return true;
    return false;
}

bool SemanticAnalyzer::isAssignCompatible(TypeCode target, TypeCode value)
{
    if (target == value)
        return true;
    if (target == TYPE_REAL && value == TYPE_INTEGER)
        return true;
    if (target == TYPE_ENUM && value == TYPE_ENUM)
        return true;
    return false;
}

TypeCode SemanticAnalyzer::resultTypeOfBinOp(const string &op,
                                             TypeCode left,
                                             TypeCode right)
{
    if (op == "/")
        return TYPE_REAL;

    if (op == "div" || op == "mod")
        return TYPE_INTEGER;

    if (op == "+" || op == "-" || op == "*")
    {
        if (left == TYPE_REAL || right == TYPE_REAL)
            return TYPE_REAL;
        return TYPE_INTEGER;
    }

    if (op == "and" || op == "or")
        return TYPE_BOOLEAN;

    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=")
        return TYPE_BOOLEAN;

    return TYPE_NONE;
}

void SemanticAnalyzer::checkAssignment(TypeCode target,
                                       TypeCode value,
                                       const string &context,
                                       int line)
{
    if (target == TYPE_NONE || value == TYPE_NONE)
        return;

    if (!isAssignCompatible(target, value))
    {
        semanticError("type mismatch in assignment to '" + context + "'", line);
    }
}

void SemanticAnalyzer::checkConditionType(TypeCode type,
                                          const string &context,
                                          int line)
{
    if (type == TYPE_NONE)
        return;

    if (type != TYPE_BOOLEAN)
        semanticError("'" + context + "' condition must be boolean", line);
}

void SemanticAnalyzer::semanticError(const string &msg, int line)
{
    string entry = "Semantic Error";
    if (line != -1)
        entry += " at line " + to_string(line);
    entry += ": " + msg;
    errors.push_back(entry);
}

void SemanticAnalyzer::printErrors(ostream &out) const
{
    if (errors.empty())
        return;
    out << "\n===== SEMANTIC ERRORS (" << errors.size() << ") =====\n";
    for (const string &e : errors)
        out << e << "\n";
}

string SemanticAnalyzer::typeCodeToString(TypeCode t)
{
    switch (t)
    {
    case TYPE_INTEGER:  return "integer";
    case TYPE_REAL:     return "real";
    case TYPE_CHAR:     return "char";
    case TYPE_BOOLEAN:  return "boolean";
    case TYPE_STRING:   return "string";
    case TYPE_ARRAY:    return "array";
    case TYPE_RECORD:   return "record";
    case TYPE_SUBRANGE: return "subrange";
    case TYPE_ENUM:     return "enum";
    default:            return "";
    }
}

void SemanticAnalyzer::setType(ASTNode *node, TypeCode t)
{
    if (node)
        node->inferredType = typeCodeToString(t);
}

// Program structure
ASTPtr SemanticAnalyzer::visitProgram(ParseNode *node)
{
    ParseNode *header = child(node, "<program_header>");
    ParseNode *nameTok = firstTerminal(header, "ident");
    string programName = nameTok ? terminalValue(nameTok) : "<anonymous>";

    enterSubprogram(programName, OBJ_PROGRAM, TYPE_NONE);

    vector<ASTPtr> declarations;
    if (ParseNode *declPart = child(node, "<declaration_part>"))
        declarations = visitDeclarationPart(declPart);

    ASTPtr blockNode = visitBlock(child(node, "<block>"));

    return make_unique<ProgramNode>(
        programName,
        move(declarations),
        move(blockNode));
}

vector<ASTPtr> SemanticAnalyzer::visitDeclarationPart(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<const_decl>")
        {
            auto items = visitConstDecl(ch);
            moveAppend(result, items);
        }
        else if (ch->label == "<type_decl>")
        {
            auto items = visitTypeDecl(ch);
            moveAppend(result, items);
        }
        else if (ch->label == "<var_decl>")
        {
            auto items = visitVarDecl(ch);
            moveAppend(result, items);
        }
        else if (ch->label == "<procedure_decl>" || ch->label == "<function_decl>")
        {
            auto items = visitSubprogramDecl(ch);
            moveAppend(result, items);
        }
    }

    return result;
}

vector<ASTPtr> SemanticAnalyzer::visitConstDecl(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *item : node->children)
    {
        if (item->label != "<const_item>")
            continue;

        ParseNode *ident = firstTerminal(item, "ident");
        ParseNode *constant = child(item, "<constant>");
        if (!ident || !constant)
            continue;

        string name = terminalValue(ident);
        ASTPtr value = visitConstant(constant);
        TypeCode constType = getTypeFromAST(value.get());

        enterConstant(name, constType);
        result.push_back(make_unique<ConstDeclNode>(name, move(value)));
    }

    return result;
}

vector<ASTPtr> SemanticAnalyzer::visitTypeDecl(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *item : node->children)
    {
        if (item->label != "<type_item>")
            continue;

        ParseNode *ident = firstTerminal(item, "ident");
        ParseNode *typeParse = child(item, "<type>");
        if (!ident || !typeParse)
            continue;

        string name = terminalValue(ident);
        TypeCode typeCode = resolveType(typeParse);

        int arrayRef = 0;
        if (typeCode == TYPE_ARRAY)
            arrayRef = registerArrayType(typeParse);

        int idx = symtab.enter(name, OBJ_TYPE, typeCode, arrayRef, 1, 0);
        (void)idx;

        if (typeCode == TYPE_ENUM)
        {
            ParseNode *enumNode = child(typeParse, "<enumerated>");
            int ordinal = 0;

            for (ParseNode *enumIdent : allTerminals(enumNode, "ident"))
            {
                symtab.enter(
                    terminalValue(enumIdent),
                    OBJ_CONSTANT,
                    TYPE_ENUM,
                    0,
                    1,
                    ordinal++
                );
            }
        }

        result.push_back(make_unique<TypeDeclNode>(
            name,
            createTypeNode(typeParse)));
    }

    return result;
}

vector<ASTPtr> SemanticAnalyzer::visitVarDecl(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *item : node->children)
    {
        if (item->label != "<var_item>")
            continue;

        ParseNode *idList = child(item, "<identifier_list>");
        ParseNode *typeParse = child(item, "<type>");
        TypeCode typeCode = resolveType(typeParse);
        ASTPtr typeAst = createTypeNode(typeParse);

        int arrayRef = 0;
        if (typeCode == TYPE_ARRAY)
            arrayRef = registerArrayType(typeParse);

        for (const string &name : identifierList(idList))
        {
            symtab.enter(name, OBJ_VARIABLE, typeCode, arrayRef, 1, 0);
            result.push_back(make_unique<VarDeclNode>(
                name,
                copyTypeNode(typeAst.get())));
        }
    }

    return result;
}

vector<ASTPtr> SemanticAnalyzer::visitSubprogramDecl(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    if (node->label == "<procedure_decl>")
        result.push_back(visitProcDecl(node));
    else if (node->label == "<function_decl>")
        result.push_back(visitFuncDecl(node));

    return result;
}

ASTPtr SemanticAnalyzer::visitProcDecl(ParseNode *node)
{
    ParseNode *ident = firstTerminal(node, "ident");
    string name = ident ? terminalValue(ident) : "<anonymous-procedure>";

    int subIdx = enterSubprogram(name, OBJ_PROCEDURE, TYPE_NONE);

    vector<ASTPtr> params;
    symtab.pushScope();

    ParamInfo sig;
    if (ParseNode *formal = child(node, "<formal_params>"))
        params = visitFormalParams(formal, sig);
    if (subIdx >= 0)
        paramSignatures[subIdx] = sig;

    vector<ASTPtr> localDecls;
    if (ParseNode *declPart = child(node, "<declaration_part>"))
        localDecls = visitDeclarationPart(declPart);

    ASTPtr blockNode = make_unique<BlockNode>(vector<ASTPtr>{});
    if (ParseNode *blockParse = child(node, "<block>"))
        blockNode = visitBlock(blockParse);

    symtab.popScope();

    auto procNode = make_unique<ProcedureDeclNode>(
        name,
        move(params),
        move(localDecls),
        move(blockNode));
    procNode->tabIndex = subIdx;
    return procNode;
}

ASTPtr SemanticAnalyzer::visitFuncDecl(ParseNode *node)
{
    ParseNode *ident = firstTerminal(node, "ident");
    string name = ident ? terminalValue(ident) : "<anonymous-function>";

    ParseNode *returnTypeParse = nullptr;
    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<type>")
        {
            returnTypeParse = ch;
            break;
        }
    }

    TypeCode retCode = resolveType(returnTypeParse);
    int subIdx = enterSubprogram(name, OBJ_FUNCTION, retCode);

    vector<ASTPtr> params;
    symtab.pushScope();

    ParamInfo sig;
    if (ParseNode *formal = child(node, "<formal_params>"))
        params = visitFormalParams(formal, sig);
    if (subIdx >= 0)
        paramSignatures[subIdx] = sig;

    vector<ASTPtr> localDecls;
    if (ParseNode *declPart = child(node, "<declaration_part>"))
        localDecls = visitDeclarationPart(declPart);

    ASTPtr returnType = createTypeNode(returnTypeParse);
    ASTPtr blockNode = make_unique<BlockNode>(vector<ASTPtr>{});
    if (ParseNode *blockParse = child(node, "<block>"))
        blockNode = visitBlock(blockParse);

    symtab.popScope();

    auto funcNode = make_unique<FunctionDeclNode>(
        name,
        move(params),
        move(returnType),
        move(localDecls),
        move(blockNode));
    funcNode->tabIndex = subIdx;
    return funcNode;
}

vector<ASTPtr> SemanticAnalyzer::visitFormalParams(ParseNode *node, ParamInfo &sig)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<param_group>")
        {
            auto group = visitParamGroup(ch, sig);
            moveAppend(result, group);
        }
    }
    return result;
}

vector<ASTPtr> SemanticAnalyzer::visitParamGroup(ParseNode *node, ParamInfo &sig)
{
    vector<ASTPtr> result;

    bool byRef = containsTerminal(node, "varsy");
    ParseNode *idList = child(node, "<identifier_list>");
    ParseNode *typeParse = child(node, "<type>");
    TypeCode typeCode = resolveType(typeParse);
    ASTPtr typeAst = createTypeNode(typeParse);

    for (const string &paramName : identifierList(idList))
    {
        symtab.enter(paramName, OBJ_VARIABLE, typeCode, 0, byRef ? 0 : 1, 0);
        sig.types.push_back(typeCode);
        sig.byRef.push_back(byRef);
        result.push_back(make_unique<ParamDeclNode>(
            paramName,
            copyTypeNode(typeAst.get()),
            byRef));
    }

    return result;
}

// Types (AST shape)
ASTPtr SemanticAnalyzer::createTypeNode(ParseNode *typeNode)
{
    if (!typeNode)
        return make_unique<TypeNameNode>("<unknown-type>");

    if (ParseNode *arrayType = child(typeNode, "<array_type>"))
        return buildArrayType(arrayType);

    if (ParseNode *recordType = child(typeNode, "<record_type>"))
        return buildRecordType(recordType);

    if (ParseNode *enumerated = child(typeNode, "<enumerated>"))
        return buildEnumerated(enumerated);

    if (ParseNode *ident = firstTerminal(typeNode, "ident"))
        return make_unique<TypeNameNode>(terminalValue(ident));

    return make_unique<TypeNameNode>("<unknown-type>");
}

ASTPtr SemanticAnalyzer::buildArrayType(ParseNode *node)
{
    ASTPtr indexType = buildRange(child(node, "<range>"));

    ParseNode *elementTypeParse = nullptr;
    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<type>")
        {
            elementTypeParse = ch;
            break;
        }
    }

    return make_unique<ArrayTypeNode>(
        move(indexType),
        createTypeNode(elementTypeParse));
}

ASTPtr SemanticAnalyzer::buildRange(ParseNode *node)
{
    if (!node)
        return make_unique<TypeNameNode>("<unknown-range>");

    vector<ParseNode *> constants = children(node, "<constant>");
    ASTPtr low = constants.size() >= 1 ? visitConstant(constants[0]) : nullptr;
    ASTPtr high = constants.size() >= 2 ? visitConstant(constants[1]) : nullptr;

    return make_unique<RangeTypeNode>(move(low), move(high));
}

int SemanticAnalyzer::registerArrayType(ParseNode *typeNode)
{
    if (!typeNode)
        return 0;

    ParseNode *arrayNode = child(typeNode, "<array_type>");
    if (!arrayNode)
        return 0;

    ParseNode *rangeNode = child(arrayNode, "<range>");
    ParseNode *elemTypeNode = nullptr;
    for (ParseNode *ch : arrayNode->children)
    {
        if (ch->label == "<type>")
        {
            elemTypeNode = ch;
            break;
        }
    }

    int low = 0, high = 0;
    if (rangeNode)
    {
        auto consts = children(rangeNode, "<constant>");
        if (consts.size() >= 1)
        {
            ASTPtr lo = visitConstant(consts[0]);
            if (lo && lo->kind == ASTKind::Num)
                low = stoi(static_cast<NumNode *>(lo.get())->rawValue);
        }
        if (consts.size() >= 2)
        {
            ASTPtr hi = visitConstant(consts[1]);
            if (hi && hi->kind == ASTKind::Num)
                high = stoi(static_cast<NumNode *>(hi.get())->rawValue);
        }
    }

    TypeCode etyp = resolveType(elemTypeNode);
    int elsz = 1;

    return symtab.enterArray(TYPE_ARRAY, etyp, 0, low, high, elsz);
}

ASTPtr SemanticAnalyzer::buildEnumerated(ParseNode *node)
{
    vector<string> names;
    for (ParseNode *ident : allTerminals(node, "ident"))
        names.push_back(terminalValue(ident));

    return make_unique<EnumTypeNode>(move(names));
}

ASTPtr SemanticAnalyzer::buildRecordType(ParseNode *node)
{
    vector<ASTPtr> fields;
    if (ParseNode *fieldList = child(node, "<field_list>"))
    {
        for (ParseNode *ch : fieldList->children)
        {
            if (ch->label != "<field_part>")
                continue;

            ParseNode *idList = child(ch, "<identifier_list>");
            ParseNode *typeParse = child(ch, "<type>");
            ASTPtr fieldType = createTypeNode(typeParse);

            for (const string &fieldName : identifierList(idList))
            {
                fields.push_back(make_unique<VarDeclNode>(
                    fieldName,
                    copyTypeNode(fieldType.get())));
            }
        }
    }

    return make_unique<RecordTypeNode>(move(fields));
}

ASTPtr SemanticAnalyzer::copyTypeNode(const ASTNode *node)
{
    if (!node)
        return nullptr;

    switch (node->kind)
    {
    case ASTKind::TypeName:
    {
        const auto *tn = static_cast<const TypeNameNode *>(node);
        return make_unique<TypeNameNode>(tn->name, tn->loc);
    }
    case ASTKind::ArrayType:
    {
        const auto *at = static_cast<const ArrayTypeNode *>(node);
        return make_unique<ArrayTypeNode>(
            copyTypeNode(at->indexType.get()),
            copyTypeNode(at->elementType.get()),
            at->loc);
    }
    case ASTKind::RecordType:
    {
        const auto *rt = static_cast<const RecordTypeNode *>(node);
        vector<ASTPtr> fields;
        for (const auto &f : rt->fields)
            fields.push_back(copyTypeNode(f.get()));
        return make_unique<RecordTypeNode>(move(fields), rt->loc);
    }
    case ASTKind::RangeType:
    {
        const auto *rng = static_cast<const RangeTypeNode *>(node);
        return make_unique<RangeTypeNode>(
            copyTypeNode(rng->low.get()),
            copyTypeNode(rng->high.get()),
            rng->loc);
    }
    case ASTKind::VarDecl:
    {
        const auto *vd = static_cast<const VarDeclNode *>(node);
        return make_unique<VarDeclNode>(vd->name, copyTypeNode(vd->typeNode.get()), vd->loc);
    }
    default:
        return make_unique<TypeNameNode>("<unknown-type>");
    }
}

ASTPtr SemanticAnalyzer::visitConstant(ParseNode *node)
{
    if (!node)
        return nullptr;

    bool negative = containsTerminal(node, "minus");

    if (ParseNode *intcon = firstTerminal(node, "intcon"))
    {
        string value = terminalValue(intcon);
        if (negative)
            value = "-" + value;
        auto n = make_unique<NumNode>(value, false);
        setType(n.get(), TYPE_INTEGER);
        return n;
    }

    if (ParseNode *realcon = firstTerminal(node, "realcon"))
    {
        string value = terminalValue(realcon);
        if (negative)
            value = "-" + value;
        auto n = make_unique<NumNode>(value, true);
        setType(n.get(), TYPE_REAL);
        return n;
    }

    if (ParseNode *charcon = firstTerminal(node, "charcon"))
    {
        auto n = make_unique<CharNode>(terminalValue(charcon));
        setType(n.get(), TYPE_CHAR);
        return n;
    }

    if (ParseNode *str = firstTerminal(node, "string"))
    {
        auto n = make_unique<StringNode>(terminalValue(str));
        setType(n.get(), TYPE_STRING);
        return n;
    }

    if (ParseNode *ident = firstTerminal(node, "ident"))
    {
        string name = terminalValue(ident);
        string lname = lower(name);

        if (lname == "true")
        {
            auto n = make_unique<BooleanNode>(true);
            setType(n.get(), TYPE_BOOLEAN);
            return n;
        }
        if (lname == "false")
        {
            auto n = make_unique<BooleanNode>(false);
            setType(n.get(), TYPE_BOOLEAN);
            return n;
        }

        auto var = make_unique<VarNode>(name);
        lookupSymbol(name);
        decorateVar(var.get(), name);
        int idx = symtab.lookup(name);
        if (idx >= 0)
            setType(var.get(), symtab.tab[idx].type);
        if (negative)
        {
            auto u = make_unique<UnaryOpNode>("-", move(var));
            setType(u.get(), symtab.tab[idx >= 0 ? idx : 0].type);
            return u;
        }
        return var;
    }

    return nullptr;
}

// Blocks & statements
ASTPtr SemanticAnalyzer::visitBlock(ParseNode *node)
{
    if (!node)
        return make_unique<BlockNode>(vector<ASTPtr>{});

    if (ParseNode *compound = child(node, "<compound-statement>"))
        return visitCompoundStmt(compound);

    return make_unique<BlockNode>(vector<ASTPtr>{});
}

ASTPtr SemanticAnalyzer::visitCompoundStmt(ParseNode *node)
{
    if (!node)
        return make_unique<BlockNode>(vector<ASTPtr>{});

    if (ParseNode *stmtList = child(node, "<statement-list>"))
        return visitStatementList(stmtList);

    return make_unique<BlockNode>(vector<ASTPtr>{});
}

ASTPtr SemanticAnalyzer::visitStatementList(ParseNode *node)
{
    vector<ASTPtr> statements;
    if (!node)
        return make_unique<BlockNode>(move(statements));

    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<statement>")
            statements.push_back(visitStatement(ch));
    }

    return make_unique<BlockNode>(move(statements));
}

ASTPtr SemanticAnalyzer::visitStatement(ParseNode *node)
{
    if (!node)
        return make_unique<EmptyNode>();

    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<compound-statement>")
            return visitCompoundStmt(ch);
        if (ch->label == "<assignment-statement>")
            return visitAssignStmt(ch);
        if (ch->label == "<procedure/function-call>")
            return visitProcCall(ch);
        if (ch->label == "<if-statement>")
            return visitIfStmt(ch);
        if (ch->label == "<while-statement>")
            return visitWhileStmt(ch);
        if (ch->label == "<repeat-statement>")
            return visitRepeatStmt(ch);
        if (ch->label == "<for-statement>")
            return visitForStmt(ch);
        if (ch->label == "<case-statement>")
            return visitCaseStmt(ch);
    }

    return make_unique<EmptyNode>();
}

ASTPtr SemanticAnalyzer::visitAssignStmt(ParseNode *node)
{
    ParseNode *variable = child(node, "<variable>");
    ParseNode *expr = child(node, "<expression>");

    string name = getVariableName(variable);
    int idx = lookupSymbol(name);
    TypeCode leftType = idx >= 0 ? symtab.tab[idx].type : TYPE_NONE;

    ASTPtr target = visitVariable(variable);
    ASTPtr value = visitExpression(expr);

    checkAssignment(leftType, getTypeFromAST(value.get()), name);

    return make_unique<AssignNode>(move(target), move(value));
}

ASTPtr SemanticAnalyzer::visitIfStmt(ParseNode *node)
{
    ParseNode *conditionParse = child(node, "<expression>");
    auto statementNodes = children(node, "<statement>");

    ASTPtr condition = visitExpression(conditionParse);
    checkConditionType(getTypeFromAST(condition.get()), "if");

    ASTPtr thenBranch = statementNodes.size() >= 1
                            ? visitStatement(statementNodes[0])
                            : make_unique<EmptyNode>();

    ASTPtr elseBranch = nullptr;
    if (statementNodes.size() >= 2)
        elseBranch = visitStatement(statementNodes[1]);

    return make_unique<IfNode>(
        move(condition),
        move(thenBranch),
        move(elseBranch));
}

ASTPtr SemanticAnalyzer::visitWhileStmt(ParseNode *node)
{
    ParseNode *conditionParse = child(node, "<expression>");

    ASTPtr condition = visitExpression(conditionParse);
    checkConditionType(getTypeFromAST(condition.get()), "while");

    ASTPtr body;
    if (ParseNode *compoundParse = child(node, "<compound-statement>"))
        body = visitCompoundStmt(compoundParse);
    else
        body = make_unique<BlockNode>(vector<ASTPtr>{});

    return make_unique<WhileNode>(move(condition), move(body));
}

ASTPtr SemanticAnalyzer::visitRepeatStmt(ParseNode *node)
{
    ParseNode *bodyParse = child(node, "<statement-list>");
    ParseNode *conditionParse = child(node, "<expression>");

    ASTPtr body = visitStatementList(bodyParse);
    ASTPtr condition = visitExpression(conditionParse);
    checkConditionType(getTypeFromAST(condition.get()), "until");

    return make_unique<RepeatNode>(move(body), move(condition));
}

ASTPtr SemanticAnalyzer::visitForStmt(ParseNode *node)
{
    ParseNode *ident = firstTerminal(node, "ident");
    auto exprs = children(node, "<expression>");
    bool downto = containsTerminal(node, "downtosy");

    string iterName = ident ? terminalValue(ident) : "<iterator>";
    int iterIdx = lookupSymbol(iterName);

    if (iterIdx >= 0)
    {
        TypeCode iterType = symtab.tab[iterIdx].type;
        if (iterType != TYPE_INTEGER && iterType != TYPE_CHAR && iterType != TYPE_NONE)
            semanticError("for loop control variable '" + iterName + "' must be integer or char");
    }

    ASTPtr startExpr = exprs.size() >= 1 ? visitExpression(exprs[0]) : nullptr;
    ASTPtr endExpr = exprs.size() >= 2 ? visitExpression(exprs[1]) : nullptr;

    if (startExpr && endExpr)
    {
        TypeCode startT = getTypeFromAST(startExpr.get());
        TypeCode endT = getTypeFromAST(endExpr.get());
        if (startT != TYPE_NONE && endT != TYPE_NONE && startT != endT)
            semanticError("for loop bounds must have the same type");
    }

    ASTPtr body;
    if (ParseNode *compoundParse = child(node, "<compound-statement>"))
        body = visitCompoundStmt(compoundParse);
    else
        body = make_unique<BlockNode>(vector<ASTPtr>{});

    return make_unique<ForNode>(
        iterName,
        move(startExpr),
        move(endExpr),
        downto,
        move(body));
}

ASTPtr SemanticAnalyzer::visitProcCall(ParseNode *node)
{
    ParseNode *ident = firstTerminal(node, "ident");
    string name = ident ? terminalValue(ident) : "<call>";

    int idx = lookupSymbol(name);
    TypeCode retType = TYPE_NONE;
    if (idx >= 0)
    {
        ObjClass obj = symtab.tab[idx].obj;
        if (obj != OBJ_PROCEDURE && obj != OBJ_FUNCTION)
            semanticError("'" + name + "' is not a procedure or function");
        if (obj == OBJ_FUNCTION)
            retType = symtab.tab[idx].type;
    }

    vector<ASTPtr> args;
    if (ParseNode *params = child(node, "<parameter-list>"))
    {
        for (ParseNode *ch : params->children)
        {
            if (ch->label == "<expression>")
                args.push_back(visitExpression(ch));
        }
    }

    // Cek jumlah dan tipe argumen jika signature diketahui
    // writeln/readln dilewati karena variadic
    if (idx >= 0 && name != "writeln" && name != "readln")
    {
        auto it = paramSignatures.find(idx);
        if (it != paramSignatures.end())
        {
            const ParamInfo &sig = it->second;
            int expected = static_cast<int>(sig.types.size());
            int got      = static_cast<int>(args.size());

            if (got != expected)
            {
                semanticError("'" + name + "' expects " + to_string(expected) +
                              " argument(s), got " + to_string(got));
            }
            else
            {
                for (int i = 0; i < got; i++)
                {
                    TypeCode argType = getTypeFromAST(args[i].get());
                    TypeCode paramType = sig.types[i];
                    if (argType != TYPE_NONE && paramType != TYPE_NONE &&
                        !isAssignCompatible(paramType, argType))
                    {
                        semanticError("argument " + to_string(i + 1) +
                                      " of '" + name + "': expected " +
                                      typeCodeToString(paramType) +
                                      ", got " + typeCodeToString(argType));
                    }
                }
            }
        }
    }

    auto callNode = make_unique<ProcedureCallNode>(name, move(args));
    callNode->tabIndex = idx;
    if (retType != TYPE_NONE)
        setType(callNode.get(), retType);
    return callNode;
}

ASTPtr SemanticAnalyzer::visitCaseStmt(ParseNode *node)
{
    ParseNode *exprParse = child(node, "<expression>");
    ASTPtr selector = visitExpression(exprParse);
    TypeCode selType = getTypeFromAST(selector.get());

    if (selType != TYPE_NONE && selType != TYPE_INTEGER &&
        selType != TYPE_CHAR && selType != TYPE_BOOLEAN &&
        selType != TYPE_ENUM)
    {
        semanticError("case selector must be ordinal type");
    }

    vector<ASTPtr> arms;
    ParseNode *caseBlock = child(node, "<case-block>");
    while (caseBlock)
    {
        vector<ASTPtr> labels;
        ParseNode *stmtParse = nullptr;
        ParseNode *nextBlock = nullptr;

        for (ParseNode *ch : caseBlock->children)
        {
            if (ch->label == "<constant>")
            {
                labels.push_back(visitConstant(ch));
            }
            else if (ch->label == "<statement>")
            {
                stmtParse = ch;
            }
            else if (ch->label == "<case-block>")
            {
                nextBlock = ch;
            }
        }

        ASTPtr body = stmtParse ? visitStatement(stmtParse) : make_unique<EmptyNode>();
        arms.push_back(make_unique<CaseArmNode>(move(labels), move(body)));

        caseBlock = nextBlock;
    }

    return make_unique<CaseNode>(move(selector), move(arms));
}

// Expressions
ASTPtr SemanticAnalyzer::visitExpression(ParseNode *node)
{
    if (!node)
        return nullptr;

    vector<ParseNode *> simpleExprs = children(node, "<simple-expression>");
    if (simpleExprs.empty())
        return nullptr;

    ASTPtr result = visitSimpleExpr(simpleExprs[0]);

    if (simpleExprs.size() >= 2)
    {
        string op = "?";
        if (ParseNode *relop = child(node, "<relational-operator>"))
            op = operatorFromNode(relop);

        ASTPtr right = visitSimpleExpr(simpleExprs[1]);
        TypeCode leftT = getTypeFromAST(result.get());
        TypeCode rightT = getTypeFromAST(right.get());

        if (leftT != TYPE_NONE && rightT != TYPE_NONE && !isRelationalCompatible(leftT, rightT))
            semanticError("incompatible types in relational expression");

        auto binop = make_unique<BinOpNode>(op, move(result), move(right));
        setType(binop.get(), TYPE_BOOLEAN);
        result = move(binop);
    }

    return result;
}

ASTPtr SemanticAnalyzer::visitSimpleExpr(ParseNode *node)
{
    if (!node)
        return nullptr;

    vector<ParseNode *> terms = children(node, "<term>");
    if (terms.empty())
        return nullptr;

    ASTPtr result = visitTerm(terms[0]);

    bool hasUnaryMinus = !node->children.empty() && terminalName(node->children[0]) == "minus";
    if (hasUnaryMinus)
    {
        TypeCode t = getTypeFromAST(result.get());
        auto u = make_unique<UnaryOpNode>("-", move(result));
        setType(u.get(), t);
        result = move(u);
    }

    size_t termIndex = 1;
    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<additive-operator>" && termIndex < terms.size())
        {
            string op = operatorFromNode(ch);
            ASTPtr right = visitTerm(terms[termIndex]);
            termIndex++;

            TypeCode leftT = getTypeFromAST(result.get());
            TypeCode rightT = getTypeFromAST(right.get());

            if (op == "or")
            {
                if (leftT != TYPE_NONE && leftT != TYPE_BOOLEAN)
                    semanticError("'or' operator requires boolean operands");
                if (rightT != TYPE_NONE && rightT != TYPE_BOOLEAN)
                    semanticError("'or' operator requires boolean operands");
            }
            else if (op == "+" || op == "-")
            {
                bool leftNum = (leftT == TYPE_INTEGER || leftT == TYPE_REAL);
                bool rightNum = (rightT == TYPE_INTEGER || rightT == TYPE_REAL);
                if (leftT != TYPE_NONE && !leftNum)
                    semanticError("arithmetic operator '" + op + "' requires numeric operands");
                if (rightT != TYPE_NONE && !rightNum)
                    semanticError("arithmetic operator '" + op + "' requires numeric operands");
                if (leftT != TYPE_NONE && rightT != TYPE_NONE && !isCompatible(leftT, rightT))
                    semanticError("incompatible types in additive expression");
            }

            TypeCode resT = resultTypeOfBinOp(op, leftT, rightT);
            auto binop = make_unique<BinOpNode>(op, move(result), move(right));
            setType(binop.get(), resT);
            result = move(binop);
        }
    }

    return result;
}

ASTPtr SemanticAnalyzer::visitTerm(ParseNode *node)
{
    if (!node)
        return nullptr;

    vector<ParseNode *> factors = children(node, "<factor>");
    if (factors.empty())
        return nullptr;

    ASTPtr result = visitFactor(factors[0]);
    size_t factorIndex = 1;

    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<multiplicative-operator>" && factorIndex < factors.size())
        {
            string op = operatorFromNode(ch);
            ASTPtr right = visitFactor(factors[factorIndex]);
            factorIndex++;

            TypeCode leftT = getTypeFromAST(result.get());
            TypeCode rightT = getTypeFromAST(right.get());

            if (op == "div" || op == "mod")
            {
                if (leftT != TYPE_NONE && leftT != TYPE_INTEGER)
                    semanticError("'" + op + "' operator requires integer operands");
                if (rightT != TYPE_NONE && rightT != TYPE_INTEGER)
                    semanticError("'" + op + "' operator requires integer operands");
            }
            else if (op == "and")
            {
                if (leftT != TYPE_NONE && leftT != TYPE_BOOLEAN)
                    semanticError("'and' operator requires boolean operands");
                if (rightT != TYPE_NONE && rightT != TYPE_BOOLEAN)
                    semanticError("'and' operator requires boolean operands");
            }
            else if (op == "+" || op == "-" || op == "*" || op == "/")
            {
                bool leftNum = (leftT == TYPE_INTEGER || leftT == TYPE_REAL);
                bool rightNum = (rightT == TYPE_INTEGER || rightT == TYPE_REAL);
                if (leftT != TYPE_NONE && !leftNum)
                    semanticError("arithmetic operator '" + op + "' requires numeric operands");
                if (rightT != TYPE_NONE && !rightNum)
                    semanticError("arithmetic operator '" + op + "' requires numeric operands");
            }

            TypeCode resT = resultTypeOfBinOp(op, leftT, rightT);
            auto binop = make_unique<BinOpNode>(op, move(result), move(right));
            setType(binop.get(), resT);
            result = move(binop);
        }
    }

    return result;
}

ASTPtr SemanticAnalyzer::visitFactor(ParseNode *node)
{
    if (!node)
        return nullptr;

    if (containsTerminal(node, "notsy"))
    {
        for (ParseNode *ch : node->children)
        {
            if (ch->label == "<factor>")
            {
                ASTPtr operand = visitFactor(ch);
                checkConditionType(getTypeFromAST(operand.get()), "not");
                auto u = make_unique<UnaryOpNode>("not", move(operand));
                setType(u.get(), TYPE_BOOLEAN);
                return u;
            }
        }
    }

    if (ParseNode *call = child(node, "<procedure/function-call>"))
        return visitProcCall(call);

    if (ParseNode *variable = child(node, "<variable>"))
        return visitVariable(variable);

    if (ParseNode *expr = child(node, "<expression>"))
        return visitExpression(expr);

    for (ParseNode *ch : node->children)
    {
        if (!isTerminal(ch)) continue;
        string name = terminalName(ch);
        if (name == "intcon")
        {
            auto n = make_unique<NumNode>(terminalValue(ch), false);
            setType(n.get(), TYPE_INTEGER);
            return n;
        }
        if (name == "realcon")
        {
            auto n = make_unique<NumNode>(terminalValue(ch), true);
            setType(n.get(), TYPE_REAL);
            return n;
        }
        if (name == "charcon")
        {
            auto n = make_unique<CharNode>(terminalValue(ch));
            setType(n.get(), TYPE_CHAR);
            return n;
        }
        if (name == "string")
        {
            auto n = make_unique<StringNode>(terminalValue(ch));
            setType(n.get(), TYPE_STRING);
            return n;
        }
    }

    return nullptr;
}

ASTPtr SemanticAnalyzer::visitVariable(ParseNode *node)
{
    if (!node)
        return nullptr;

    ParseNode *ident = firstTerminal(node, "ident");
    string name = ident ? terminalValue(ident) : "<var>";

    lookupSymbol(name);

    auto varNode = make_unique<VarNode>(name);
    decorateVar(varNode.get(), name);
    int idx = symtab.lookup(name);
    if (idx >= 0)
        setType(varNode.get(), symtab.tab[idx].type);

    ASTPtr result = move(varNode);
    for (ParseNode *ch : node->children)
    {
        if (ch->label == "<component-variable>")
            result = applyComponent(move(result), ch);
    }

    return result;
}

ASTPtr SemanticAnalyzer::applyComponent(ASTPtr base, ParseNode *componentNode)
{
    if (!componentNode)
        return base;

    if (containsTerminal(componentNode, "lbrack"))
    {
        return make_unique<ArrayAccessNode>(
            move(base),
            visitIndexList(child(componentNode, "<index-list>")));
    }

    if (containsTerminal(componentNode, "period"))
    {
        auto idents = allTerminals(componentNode, "ident");
        string fieldName = idents.empty() ? "<field>" : terminalValue(idents.front());

        return make_unique<FieldAccessNode>(move(base), fieldName);
    }

    return base;
}

vector<ASTPtr> SemanticAnalyzer::visitIndexList(ParseNode *node)
{
    vector<ASTPtr> result;
    if (!node)
        return result;

    for (ParseNode *ch : node->children)
    {
        if (isTerminal(ch))
        {
            string name = terminalName(ch);
            if (name == "intcon")
                result.push_back(make_unique<NumNode>(terminalValue(ch), false));
            else if (name == "charcon")
                result.push_back(make_unique<CharNode>(terminalValue(ch)));
            else if (name == "ident")
            {
                string iname = terminalValue(ch);
                lookupSymbol(iname);
                auto var = make_unique<VarNode>(iname);
                decorateVar(var.get(), iname);
                result.push_back(move(var));
            }
        }
        else if (ch->label == "<index-list>")
        {
            auto nested = visitIndexList(ch);
            moveAppend(result, nested);
        }
    }

    return result;
}

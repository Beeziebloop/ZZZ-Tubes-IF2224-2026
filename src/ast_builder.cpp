#include "ast_builder.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

ASTPtr ASTBuilder::build(ParseNode* root) {
    if (!root) {
        throw std::runtime_error("ASTBuilder error: parse tree root is null");
    }

    return buildAny(root);
}

bool ASTBuilder::isLabel(ParseNode* node, const std::string& label) const {
    return node && node->label == label;
}

bool ASTBuilder::isTerminal(ParseNode* node) const {
    if (!node) return false;
    return !node->label.empty() && node->label.front() != '<';
}

bool ASTBuilder::startsWith(const std::string& s, const std::string& prefix) const {
    return s.rfind(prefix, 0) == 0;
}

//mengembalikan nama token terminal tanpa valuenya, misal ident(Hello) -> ident
std::string ASTBuilder::terminalName(ParseNode* node) const {
    if (!node) return "";

    const std::string& label = node->label;
    size_t pos = label.find('(');

    if (pos == std::string::npos) {
        return label;
    }

    return label.substr(0, pos);
}

//mengembalikan value dari token terminal, misal ident(Hello) -> Hello
std::string ASTBuilder::terminalValue(ParseNode* node) const {
    if (!node) return "";

    const std::string& label = node->label;
    size_t open = label.find('(');
    size_t close = label.rfind(')');

    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return "";
    }

    std::string value = label.substr(open + 1, close - open - 1);

    if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
        value = value.substr(1, value.size() - 2);
    }

    return value;
}

std::string ASTBuilder::lower(std::string s) const {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

ParseNode* ASTBuilder::child(ParseNode* node, const std::string& label) const {
    if (!node) return nullptr;

    for (ParseNode* ch : node->children) {
        if (ch->label == label) return ch;
    }

    return nullptr;
}

std::vector<ParseNode*> ASTBuilder::children(ParseNode* node, const std::string& label) const {
    std::vector<ParseNode*> result;

    if (!node) return result;

    for (ParseNode* ch : node->children) {
        if (ch->label == label) result.push_back(ch);
    }

    return result;
}

ParseNode* ASTBuilder::firstTerminal(ParseNode* node, const std::string& terminal) const {
    if (!node) return nullptr;

    if (isTerminal(node) && terminalName(node) == terminal) {
        return node;
    }

    for (ParseNode* ch : node->children) {
        ParseNode* found = firstTerminal(ch, terminal);
        if (found) return found;
    }

    return nullptr;
}

std::vector<ParseNode*> ASTBuilder::allTerminals(ParseNode* node, const std::string& terminal) const {
    std::vector<ParseNode*> result;
    collectTerminals(node, terminal, result);
    return result;
}

void ASTBuilder::collectTerminals(ParseNode* node, const std::string& terminal, std::vector<ParseNode*>& out) const {
    if (!node) return;

    if (isTerminal(node) && terminalName(node) == terminal) {
        out.push_back(node);
    }

    for (ParseNode* ch : node->children) {
        collectTerminals(ch, terminal, out);
    }
}

bool ASTBuilder::containsTerminal(ParseNode* node, const std::string& terminal) const {
    return firstTerminal(node, terminal) != nullptr;
}

void ASTBuilder::moveAppend(std::vector<ASTPtr>& dst, std::vector<ASTPtr>& src) {
    for (auto& item : src) {
        dst.push_back(std::move(item));
    }
    src.clear();
}

ASTPtr ASTBuilder::buildAny(ParseNode* node) {
    if (!node) return nullptr;

    if (node->label == "<program>") return buildProgram(node);
    if (node->label == "<block>") return buildBlock(node);
    if (node->label == "<compound-statement>") return buildCompoundStatement(node);
    if (node->label == "<statement-list>") return buildStatementList(node);
    if (node->label == "<statement>") return buildStatement(node);
    if (node->label == "<assignment-statement>") return buildAssignment(node);
    if (node->label == "<if-statement>") return buildIf(node);
    if (node->label == "<while-statement>") return buildWhile(node);
    if (node->label == "<repeat-statement>") return buildRepeat(node);
    if (node->label == "<for-statement>") return buildFor(node);
    if (node->label == "<procedure/function-call>") return buildProcedureFunctionCall(node);
    if (node->label == "<expression>") return buildExpression(node);
    if (node->label == "<simple-expression>") return buildSimpleExpression(node);
    if (node->label == "<term>") return buildTerm(node);
    if (node->label == "<factor>") return buildFactor(node);
    if (node->label == "<variable>") return buildVariable(node);

    for (ParseNode* ch : node->children) {
        ASTPtr result = buildAny(ch);
        if (result) return result;
    }

    return nullptr;
}

//<program> -> <program_header> <declaration_part> <block> period
ASTPtr ASTBuilder::buildProgram(ParseNode* node) {
    ParseNode* header = child(node, "<program_header>");
    ParseNode* nameTok = firstTerminal(header, "ident");

    std::string programName = nameTok ? terminalValue(nameTok) : "<anonymous>";

    std::vector<ASTPtr> declarations;
    if (ParseNode* declPart = child(node, "<declaration_part>")) {
        declarations = buildDeclarationPart(declPart);
    }

    ASTPtr blockNode = buildBlock(child(node, "<block>"));

    return std::make_unique<ProgramNode>(
        programName,
        std::move(declarations),
        std::move(blockNode)
    );
}

std::vector<ASTPtr> ASTBuilder::buildDeclarationPart(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* ch : node->children) {
        if (ch->label == "<const_decl>") {
            auto items = buildConstDecl(ch);
            moveAppend(result, items);
        } else if (ch->label == "<type_decl>") {
            auto items = buildTypeDecl(ch);
            moveAppend(result, items);
        } else if (ch->label == "<var_decl>") {
            auto items = buildVarDecl(ch);
            moveAppend(result, items);
        } else if (ch->label == "<procedure_decl>" || ch->label == "<function_decl>") {
            auto items = buildSubprogramDecl(ch);
            moveAppend(result, items);
        }
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildConstDecl(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* item : node->children) {
        if (item->label != "<const_item>") continue;

        ParseNode* ident = firstTerminal(item, "ident");
        ParseNode* constant = child(item, "<constant>");

        if (!ident || !constant) continue;

        result.push_back(std::make_unique<ConstDeclNode>(
            terminalValue(ident),
            buildConstant(constant)
        ));
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildTypeDecl(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* item : node->children) {
        if (item->label != "<type_item>") continue;

        ParseNode* ident = firstTerminal(item, "ident");
        ParseNode* typeNode = child(item, "<type>");

        if (!ident || !typeNode) continue;

        result.push_back(std::make_unique<TypeDeclNode>(
            terminalValue(ident),
            buildType(typeNode)
        ));
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildVarDecl(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* item : node->children) {
        if (item->label != "<var_item>") continue;

        ParseNode* idList = child(item, "<identifier_list>");
        ParseNode* typeNode = child(item, "<type>");

        std::vector<std::string> names = identifierList(idList);

        for (const std::string& name : names) {
            result.push_back(std::make_unique<VarDeclNode>(
                name,
                buildType(typeNode)
            ));
        }
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildSubprogramDecl(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    if (node->label == "<procedure_decl>") {
        result.push_back(buildProcedureDecl(node));
    } else if (node->label == "<function_decl>") {
        result.push_back(buildFunctionDecl(node));
    }

    return result;
}

ASTPtr ASTBuilder::buildProcedureDecl(ParseNode* node) {
    ParseNode* ident = firstTerminal(node, "ident");
    std::string name = ident ? terminalValue(ident) : "<anonymous-procedure>";

    std::vector<ASTPtr> params;
    if (ParseNode* formalParams = child(node, "<formal_params>")) {
        params = buildFormalParams(formalParams);
    }

    std::vector<ASTPtr> declarations;
    ASTPtr blockNode = nullptr;

    if (ParseNode* blockParse = child(node, "<block>")) {
        blockNode = buildBlock(blockParse);
    } else {
        blockNode = std::make_unique<BlockNode>(std::vector<ASTPtr>{});
    }

    return std::make_unique<ProcedureDeclNode>(
        name,
        std::move(params),
        std::move(declarations),
        std::move(blockNode)
    );
}

ASTPtr ASTBuilder::buildFunctionDecl(ParseNode* node) {
    ParseNode* ident = firstTerminal(node, "ident");
    std::string name = ident ? terminalValue(ident) : "<anonymous-function>";

    std::vector<ASTPtr> params;
    if (ParseNode* formalParams = child(node, "<formal_params>")) {
        params = buildFormalParams(formalParams);
    }

    ParseNode* returnTypeParse = nullptr;
    for (ParseNode* ch : node->children) {
        if (ch->label == "<type>") {
            returnTypeParse = ch;
            break;
        }
    }

    ASTPtr returnType = buildType(returnTypeParse);

    ASTPtr blockNode = nullptr;
    if (ParseNode* blockParse = child(node, "<block>")) {
        blockNode = buildBlock(blockParse);
    } else {
        blockNode = std::make_unique<BlockNode>(std::vector<ASTPtr>{});
    }

    return std::make_unique<FunctionDeclNode>(
        name,
        std::move(params),
        std::move(returnType),
        std::vector<ASTPtr>{},
        std::move(blockNode)
    );
}

std::vector<ASTPtr> ASTBuilder::buildFormalParams(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* ch : node->children) {
        if (ch->label == "<param_group>") {
            auto group = buildParamGroup(ch);
            moveAppend(result, group);
        }
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildParamGroup(ParseNode* node) {
    std::vector<ASTPtr> result;

    ParseNode* idList = child(node, "<identifier_list>");
    ParseNode* typeNode = child(node, "<type>");

    for (const std::string& name : identifierList(idList)) {
        result.push_back(std::make_unique<ParamDeclNode>(
            name,
            buildType(typeNode),
            false
        ));
    }

    return result;
}

ASTPtr ASTBuilder::buildType(ParseNode* node) {
    if (!node) return std::make_unique<TypeNameNode>("<unknown-type>");

    if (ParseNode* arrayType = child(node, "<array_type>")) {
        return buildArrayType(arrayType);
    }

    if (ParseNode* recordType = child(node, "<record_type>")) {
        return buildRecordType(recordType);
    }

    if (ParseNode* enumerated = child(node, "<enumerated>")) {
        return buildEnumerated(enumerated);
    }

    if (ParseNode* ident = firstTerminal(node, "ident")) {
        return std::make_unique<TypeNameNode>(terminalValue(ident));
    }

    return std::make_unique<TypeNameNode>("<unknown-type>");
}

ASTPtr ASTBuilder::buildArrayType(ParseNode* node) {
    ASTPtr indexType = buildRange(child(node, "<range>"));

    ParseNode* elementTypeParse = nullptr;
    for (ParseNode* ch : node->children) {
        if (ch->label == "<type>") {
            elementTypeParse = ch;
            break;
        }
    }

    return std::make_unique<ArrayTypeNode>(
        std::move(indexType),
        buildType(elementTypeParse)
    );
}

ASTPtr ASTBuilder::buildRange(ParseNode* node) {
    if (!node) return std::make_unique<TypeNameNode>("<unknown-range>");

    std::vector<ParseNode*> constants = children(node, "<constant>");

    ASTPtr low = constants.size() >= 1 ? buildConstant(constants[0]) : nullptr;
    ASTPtr high = constants.size() >= 2 ? buildConstant(constants[1]) : nullptr;

    return std::make_unique<RangeTypeNode>(
        std::move(low),
        std::move(high)
    );
}

ASTPtr ASTBuilder::buildEnumerated(ParseNode* node) {
    std::vector<std::string> names;

    for (ParseNode* ident : allTerminals(node, "ident")) {
        names.push_back(terminalValue(ident));
    }

    return std::make_unique<EnumTypeNode>(std::move(names));
}

ASTPtr ASTBuilder::buildRecordType(ParseNode* node) {
    std::vector<ASTPtr> fields;

    if (ParseNode* fieldList = child(node, "<field_list>")) {
        fields = buildFieldList(fieldList);
    }

    return std::make_unique<RecordTypeNode>(std::move(fields));
}

std::vector<ASTPtr> ASTBuilder::buildFieldList(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* ch : node->children) {
        if (ch->label == "<field_part>") {
            auto fields = buildFieldPart(ch);
            moveAppend(result, fields);
        }
    }

    return result;
}

std::vector<ASTPtr> ASTBuilder::buildFieldPart(ParseNode* node) {
    std::vector<ASTPtr> result;

    ParseNode* idList = child(node, "<identifier_list>");
    ParseNode* typeNode = child(node, "<type>");

    for (const std::string& name : identifierList(idList)) {
        result.push_back(std::make_unique<VarDeclNode>(
            name,
            buildType(typeNode)
        ));
    }

    return result;
}

ASTPtr ASTBuilder::buildConstant(ParseNode* node) {
    if (!node) return nullptr;

    bool negative = containsTerminal(node, "minus");

    if (ParseNode* intcon = firstTerminal(node, "intcon")) {
        std::string value = terminalValue(intcon);
        if (negative) value = "-" + value;
        return std::make_unique<NumNode>(value, false);
    }

    if (ParseNode* realcon = firstTerminal(node, "realcon")) {
        std::string value = terminalValue(realcon);
        if (negative) value = "-" + value;
        return std::make_unique<NumNode>(value, true);
    }

    if (ParseNode* charcon = firstTerminal(node, "charcon")) {
        return std::make_unique<CharNode>(terminalValue(charcon));
    }

    if (ParseNode* str = firstTerminal(node, "string")) {
        return std::make_unique<StringNode>(terminalValue(str));
    }

    if (ParseNode* ident = firstTerminal(node, "ident")) {
        std::string name = terminalValue(ident);
        std::string lname = lower(name);

        if (lname == "true") return std::make_unique<BooleanNode>(true);
        if (lname == "false") return std::make_unique<BooleanNode>(false);

        ASTPtr var = std::make_unique<VarNode>(name);
        if (negative) return std::make_unique<UnaryOpNode>("-", std::move(var));
        return var;
    }

    return nullptr;
}

ASTPtr ASTBuilder::buildBlock(ParseNode* node) {
    if (!node) return std::make_unique<BlockNode>(std::vector<ASTPtr>{});

    if (ParseNode* compound = child(node, "<compound-statement>")) {
        return buildCompoundStatement(compound);
    }

    return std::make_unique<BlockNode>(std::vector<ASTPtr>{});
}

ASTPtr ASTBuilder::buildCompoundStatement(ParseNode* node) {
    if (!node) return std::make_unique<BlockNode>(std::vector<ASTPtr>{});

    if (ParseNode* stmtList = child(node, "<statement-list>")) {
        return buildStatementList(stmtList);
    }

    return std::make_unique<BlockNode>(std::vector<ASTPtr>{});
}

ASTPtr ASTBuilder::buildStatementList(ParseNode* node) {
    std::vector<ASTPtr> statements;

    if (!node) return std::make_unique<BlockNode>(std::move(statements));

    for (ParseNode* ch : node->children) {
        if (ch->label == "<statement>") {
            statements.push_back(buildStatement(ch));
        }
    }

    return std::make_unique<BlockNode>(std::move(statements));
}

ASTPtr ASTBuilder::buildStatement(ParseNode* node) {
    if (!node) return std::make_unique<EmptyNode>();

    for (ParseNode* ch : node->children) {
        if (ch->label == "<compound-statement>") return buildCompoundStatement(ch);
        if (ch->label == "<assignment-statement>") return buildAssignment(ch);
        if (ch->label == "<procedure/function-call>") return buildProcedureFunctionCall(ch);
        if (ch->label == "<if-statement>") return buildIf(ch);
        if (ch->label == "<while-statement>") return buildWhile(ch);
        if (ch->label == "<repeat-statement>") return buildRepeat(ch);
        if (ch->label == "<for-statement>") return buildFor(ch);
    }

    return std::make_unique<EmptyNode>();
}

ASTPtr ASTBuilder::buildAssignment(ParseNode* node) {
    ParseNode* variable = child(node, "<variable>");
    ParseNode* expr = child(node, "<expression>");

    return std::make_unique<AssignNode>(
        buildVariable(variable),
        buildExpression(expr)
    );
}

ASTPtr ASTBuilder::buildIf(ParseNode* node) {
    ParseNode* conditionParse = child(node, "<expression>");
    auto statementNodes = children(node, "<statement>");

    ASTPtr thenBranch = statementNodes.size() >= 1
        ? buildStatement(statementNodes[0])
        : std::make_unique<EmptyNode>();

    ASTPtr elseBranch = nullptr;
    if (statementNodes.size() >= 2) {
        elseBranch = buildStatement(statementNodes[1]);
    }

    return std::make_unique<IfNode>(
        buildExpression(conditionParse),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

ASTPtr ASTBuilder::buildWhile(ParseNode* node) {
    ParseNode* conditionParse = child(node, "<expression>");
    ParseNode* bodyParse = child(node, "<statement>");

    return std::make_unique<WhileNode>(
        buildExpression(conditionParse),
        buildStatement(bodyParse)
    );
}

ASTPtr ASTBuilder::buildRepeat(ParseNode* node) {
    ParseNode* bodyParse = child(node, "<statement-list>");
    ParseNode* conditionParse = child(node, "<expression>");

    return std::make_unique<RepeatNode>(
        buildStatementList(bodyParse),
        buildExpression(conditionParse)
    );
}

ASTPtr ASTBuilder::buildFor(ParseNode* node) {
    ParseNode* ident = firstTerminal(node, "ident");
    auto exprs = children(node, "<expression>");

    bool downto = containsTerminal(node, "downtosy");

    ASTPtr startExpr = exprs.size() >= 1 ? buildExpression(exprs[0]) : nullptr;
    ASTPtr endExpr = exprs.size() >= 2 ? buildExpression(exprs[1]) : nullptr;

    ParseNode* bodyParse = child(node, "<statement>");

    return std::make_unique<ForNode>(
        ident ? terminalValue(ident) : "<iterator>",
        std::move(startExpr),
        std::move(endExpr),
        downto,
        buildStatement(bodyParse)
    );
}

ASTPtr ASTBuilder::buildProcedureFunctionCall(ParseNode* node) {
    ParseNode* ident = firstTerminal(node, "ident");
    std::string name = ident ? terminalValue(ident) : "<call>";

    std::vector<ASTPtr> args;

    if (ParseNode* params = child(node, "<parameter-list>")) {
        for (ParseNode* ch : params->children) {
            if (ch->label == "<expression>") {
                args.push_back(buildExpression(ch));
            }
        }
    }

    return std::make_unique<ProcedureCallNode>(
        name,
        std::move(args)
    );
}

ASTPtr ASTBuilder::buildExpression(ParseNode* node) {
    if (!node) return nullptr;

    std::vector<ParseNode*> simpleExprs = children(node, "<simple-expression>");

    if (simpleExprs.empty()) return nullptr;

    ASTPtr result = buildSimpleExpression(simpleExprs[0]);

    if (simpleExprs.size() >= 2) {
        std::string op = "?";

        if (ParseNode* relop = child(node, "<relational-operator>")) {
            op = operatorFromNode(relop);
        }

        result = std::make_unique<BinOpNode>(
            op,
            std::move(result),
            buildSimpleExpression(simpleExprs[1])
        );
    }

    return result;
}

ASTPtr ASTBuilder::buildSimpleExpression(ParseNode* node) {
    if (!node) return nullptr;

    std::vector<ParseNode*> terms = children(node, "<term>");

    if (terms.empty()) return nullptr;

    ASTPtr result = buildTerm(terms[0]);

    bool hasUnaryMinus = false;
    if (!node->children.empty() && terminalName(node->children[0]) == "minus") {
        hasUnaryMinus = true;
    }

    if (hasUnaryMinus) {
        result = std::make_unique<UnaryOpNode>("-", std::move(result));
    }

    size_t termIndex = 1;

    for (ParseNode* ch : node->children) {
        if (ch->label == "<additive-operator>" && termIndex < terms.size()) {
            std::string op = operatorFromNode(ch);

            result = std::make_unique<BinOpNode>(
                op,
                std::move(result),
                buildTerm(terms[termIndex])
            );

            termIndex++;
        }
    }

    return result;
}

ASTPtr ASTBuilder::buildTerm(ParseNode* node) {
    if (!node) return nullptr;

    std::vector<ParseNode*> factors = children(node, "<factor>");

    if (factors.empty()) return nullptr;

    ASTPtr result = buildFactor(factors[0]);
    size_t factorIndex = 1;

    for (ParseNode* ch : node->children) {
        if (ch->label == "<multiplicative-operator>" && factorIndex < factors.size()) {
            std::string op = operatorFromNode(ch);

            result = std::make_unique<BinOpNode>(
                op,
                std::move(result),
                buildFactor(factors[factorIndex])
            );

            factorIndex++;
        }
    }

    return result;
}

ASTPtr ASTBuilder::buildFactor(ParseNode* node) {
    if (!node) return nullptr;

    if (ParseNode* intcon = firstTerminal(node, "intcon")) {
        return std::make_unique<NumNode>(terminalValue(intcon), false);
    }

    if (ParseNode* realcon = firstTerminal(node, "realcon")) {
        return std::make_unique<NumNode>(terminalValue(realcon), true);
    }

    if (ParseNode* charcon = firstTerminal(node, "charcon")) {
        return std::make_unique<CharNode>(terminalValue(charcon));
    }

    if (ParseNode* str = firstTerminal(node, "string")) {
        return std::make_unique<StringNode>(terminalValue(str));
    }

    if (containsTerminal(node, "notsy")) {
        for (ParseNode* ch : node->children) {
            if (ch->label == "<factor>") {
                return std::make_unique<UnaryOpNode>("not", buildFactor(ch));
            }
        }
    }

    if (ParseNode* call = child(node, "<procedure/function-call>")) {
        return buildProcedureFunctionCall(call);
    }

    if (ParseNode* variable = child(node, "<variable>")) {
        return buildVariable(variable);
    }

    if (ParseNode* expr = child(node, "<expression>")) {
        return buildExpression(expr);
    }

    return nullptr;
}

ASTPtr ASTBuilder::buildVariable(ParseNode* node) {
    if (!node) return nullptr;

    ParseNode* ident = firstTerminal(node, "ident");
    ASTPtr result = std::make_unique<VarNode>(
        ident ? terminalValue(ident) : "<var>"
    );

    for (ParseNode* ch : node->children) {
        if (ch->label == "<component-variable>") {
            result = applyComponent(std::move(result), ch);
        }
    }

    return result;
}

ASTPtr ASTBuilder::applyComponent(ASTPtr base, ParseNode* componentNode) {
    if (!componentNode) return base;

    if (containsTerminal(componentNode, "lbrack")) {
        return std::make_unique<ArrayAccessNode>(
            std::move(base),
            buildIndexList(child(componentNode, "<index-list>"))
        );
    }

    if (containsTerminal(componentNode, "period")) {
        auto idents = allTerminals(componentNode, "ident");
        std::string fieldName = idents.empty() ? "<field>" : terminalValue(idents.front());

        return std::make_unique<FieldAccessNode>(
            std::move(base),
            fieldName
        );
    }

    return base;
}

std::vector<ASTPtr> ASTBuilder::buildIndexList(ParseNode* node) {
    std::vector<ASTPtr> result;

    if (!node) return result;

    for (ParseNode* ch : node->children) {
        if (isTerminal(ch)) {
            std::string name = terminalName(ch);

            if (name == "intcon") {
                result.push_back(std::make_unique<NumNode>(terminalValue(ch), false));
            } else if (name == "charcon") {
                result.push_back(std::make_unique<CharNode>(terminalValue(ch)));
            } else if (name == "ident") {
                result.push_back(std::make_unique<VarNode>(terminalValue(ch)));
            }
        } else if (ch->label == "<index-list>") {
            auto nested = buildIndexList(ch);
            moveAppend(result, nested);
        }
    }

    return result;
}

std::string ASTBuilder::operatorFromNode(ParseNode* node) const {
    if (!node) return "?";

    if (isTerminal(node)) {
        std::string name = terminalName(node);

        if (name == "plus") return "+";
        if (name == "minus") return "-";
        if (name == "times") return "*";
        if (name == "rdiv") return "/";
        if (name == "idiv") return "div";
        if (name == "imod") return "mod";
        if (name == "andsy") return "and";
        if (name == "orsy") return "or";
        if (name == "eql") return "==";
        if (name == "neq") return "<>";
        if (name == "gtr") return ">";
        if (name == "geq") return ">=";
        if (name == "lss") return "<";
        if (name == "leq") return "<=";

        return name;
    }

    for (ParseNode* ch : node->children) {
        std::string op = operatorFromNode(ch);
        if (op != "?") return op;
    }

    return "?";
}

std::vector<std::string> ASTBuilder::identifierList(ParseNode* node) const {
    std::vector<std::string> result;

    if (!node) return result;

    for (ParseNode* ident : allTerminals(node, "ident")) {
        result.push_back(terminalValue(ident));
    }

    return result;
}
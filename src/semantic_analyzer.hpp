#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <map>
#include <string>
#include <vector>

#include "ast.hpp"
#include "parser.hpp"
#include "symbol_table.hpp"

// Info parameter untuk satu subprogram
struct ParamInfo {
    std::vector<TypeCode> types; // tipe tiap parameter (urutan deklarasi)
    std::vector<bool> byRef;     // true = var parameter
};

class SemanticAnalyzer
{
public:
    SymbolTable symtab;

    // paramSignatures[tab_index] = info parameter subprogram tsb
    std::map<int, ParamInfo> paramSignatures;

    // Entry point
    ASTPtr analyze(ParseNode *root);

private:
    // Parse-tree helpers
    bool isLabel(ParseNode *node, const std::string &label) const;
    bool isTerminal(ParseNode *node) const;
    bool containsTerminal(ParseNode *node, const std::string &terminal) const;
    std::string terminalName(ParseNode *node) const;
    std::string terminalValue(ParseNode *node) const;
    std::string lower(std::string s) const;
    ParseNode *child(ParseNode *node, const std::string &label) const;
    std::vector<ParseNode *> children(ParseNode *node, const std::string &label) const;
    ParseNode *firstTerminal(ParseNode *node, const std::string &terminal) const;
    std::vector<ParseNode *> allTerminals(ParseNode *node, const std::string &terminal) const;
    void collectTerminals(ParseNode *node,
                          const std::string &terminal,
                          std::vector<ParseNode *> &out) const;
    std::vector<std::string> identifierList(ParseNode *node) const;
    std::string operatorFromNode(ParseNode *node) const;
    std::string getVariableName(ParseNode *varNode) const;
    static void moveAppend(std::vector<ASTPtr> &dst, std::vector<ASTPtr> &src);

    // Program structure
    ASTPtr visitProgram(ParseNode *node);
    std::vector<ASTPtr> visitDeclarationPart(ParseNode *node);
    std::vector<ASTPtr> visitConstDecl(ParseNode *node);
    std::vector<ASTPtr> visitTypeDecl(ParseNode *node);
    std::vector<ASTPtr> visitVarDecl(ParseNode *node);
    std::vector<ASTPtr> visitSubprogramDecl(ParseNode *node);
    ASTPtr visitProcDecl(ParseNode *node);
    ASTPtr visitFuncDecl(ParseNode *node);
    std::vector<ASTPtr> visitFormalParams(ParseNode *node, ParamInfo &sig);
    std::vector<ASTPtr> visitParamGroup(ParseNode *node, ParamInfo &sig);

    // Types
    ASTPtr createTypeNode(ParseNode *typeNode);
    ASTPtr buildArrayType(ParseNode *node);
    ASTPtr buildRange(ParseNode *node);
    ASTPtr buildEnumerated(ParseNode *node);
    ASTPtr buildRecordType(ParseNode *node);
    ASTPtr copyTypeNode(const ASTNode *node);
    TypeCode resolveType(ParseNode *typeNode);
    TypeCode resolveTypeName(const std::string &name);
    int registerArrayType(ParseNode *typeNode);

    // Constants
    ASTPtr visitConstant(ParseNode *node);

    // Blocks & statements
    ASTPtr visitBlock(ParseNode *node);
    ASTPtr visitCompoundStmt(ParseNode *node);
    ASTPtr visitStatementList(ParseNode *node);
    ASTPtr visitStatement(ParseNode *node);
    ASTPtr visitAssignStmt(ParseNode *node);
    ASTPtr visitIfStmt(ParseNode *node);
    ASTPtr visitWhileStmt(ParseNode *node);
    ASTPtr visitForStmt(ParseNode *node);
    ASTPtr visitRepeatStmt(ParseNode *node);
    ASTPtr visitProcCall(ParseNode *node);
    ASTPtr visitCaseStmt(ParseNode *node);

    // Expressions
    ASTPtr visitExpression(ParseNode *node);
    ASTPtr visitSimpleExpr(ParseNode *node);
    ASTPtr visitTerm(ParseNode *node);
    ASTPtr visitFactor(ParseNode *node);
    ASTPtr visitVariable(ParseNode *node);
    ASTPtr applyComponent(ASTPtr base, ParseNode *componentNode);
    std::vector<ASTPtr> visitIndexList(ParseNode *node);

    // Symbol & type checking
    int lookupSymbol(const std::string &name, int line = -1);
    void enterVariable(const std::string &name, TypeCode type);
    void enterConstant(const std::string &name, TypeCode type, int adr = 0);
    void enterTypeAlias(const std::string &name, TypeCode type);
    int enterSubprogram(const std::string &name, ObjClass obj, TypeCode retType = TYPE_NONE);
    void decorateVar(ASTNode *node, const std::string &name);
    TypeCode getTypeFromAST(const ASTNode *node);
    bool isCompatible(TypeCode t1, TypeCode t2);
    bool isRelationalCompatible(TypeCode t1, TypeCode t2);
    bool isAssignCompatible(TypeCode target, TypeCode value);
    TypeCode resultTypeOfBinOp(const std::string &op, TypeCode left, TypeCode right);
    void checkAssignment(TypeCode target,
                         TypeCode value,
                         const std::string &context,
                         int line = -1);
    void checkConditionType(TypeCode type, const std::string &context, int line = -1);

    void semanticError(const std::string &msg, int line = -1);
    static std::string typeCodeToString(TypeCode t);
    void setType(ASTNode *node, TypeCode t);

public:
    std::vector<std::string> errors;
    bool hasErrors() const { return !errors.empty(); }
    void printErrors(std::ostream &out = std::cerr) const;
};

#endif

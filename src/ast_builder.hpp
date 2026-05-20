#ifndef AST_BUILDER_HPP
#define AST_BUILDER_HPP

#include "ast.hpp"
#include "parser.hpp"

#include <string>
#include <vector>

//kelas ini hanya melakukan syntax-directed translation -> membuang node/token sintaksis yang tidak diperlukan, membentuk node semantik seperti AssignNode, BinOpNode, VarNode
class ASTBuilder {
public:
    ASTPtr build(ParseNode* root); //entry point, terima root parse tree dan ngembaliin root AST

private:
    //helper parse tree
    bool isLabel(ParseNode* node, const std::string& label) const;
    bool isTerminal(ParseNode* node) const;
    bool startsWith(const std::string& s, const std::string& prefix) const;
    std::string terminalName(ParseNode* node) const;
    std::string terminalValue(ParseNode* node) const;
    std::string lower(std::string s) const;
    ParseNode* child(ParseNode* node, const std::string& label) const;
    std::vector<ParseNode*> children(ParseNode* node, const std::string& label) const;
    ParseNode* firstTerminal(ParseNode* node, const std::string& terminal) const;
    std::vector<ParseNode*> allTerminals(ParseNode* node, const std::string& terminal) const;
    void collectTerminals(ParseNode* node, const std::string& terminal, std::vector<ParseNode*>& out) const;
    bool containsTerminal(ParseNode* node, const std::string& terminal) const;
    //dispatcher utama, dipakai saat builder nerima node yang labelnya belum pasti dan memilih buildx yang sesuai berdasarkan label parse tree
    ASTPtr buildAny(ParseNode* node);
    //program dan deklarasi
    ASTPtr buildProgram(ParseNode* node);
    std::vector<ASTPtr> buildDeclarationPart(ParseNode* node);
    std::vector<ASTPtr> buildConstDecl(ParseNode* node);
    std::vector<ASTPtr> buildTypeDecl(ParseNode* node);
    std::vector<ASTPtr> buildVarDecl(ParseNode* node);
    std::vector<ASTPtr> buildSubprogramDecl(ParseNode* node);
    ASTPtr buildProcedureDecl(ParseNode* node);
    ASTPtr buildFunctionDecl(ParseNode* node);
    std::vector<ASTPtr> buildFormalParams(ParseNode* node);
    std::vector<ASTPtr> buildParamGroup(ParseNode* node);
    //type nodes
    ASTPtr buildType(ParseNode* node);
    ASTPtr buildArrayType(ParseNode* node);
    ASTPtr buildRange(ParseNode* node);
    ASTPtr buildEnumerated(ParseNode* node);
    ASTPtr buildRecordType(ParseNode* node);
    std::vector<ASTPtr> buildFieldList(ParseNode* node);
    std::vector<ASTPtr> buildFieldPart(ParseNode* node);
    ASTPtr buildConstant(ParseNode* node);
    //block dan statement
    ASTPtr buildBlock(ParseNode* node);
    ASTPtr buildCompoundStatement(ParseNode* node);
    ASTPtr buildStatementList(ParseNode* node);
    ASTPtr buildStatement(ParseNode* node);
    ASTPtr buildAssignment(ParseNode* node);
    ASTPtr buildIf(ParseNode* node);
    ASTPtr buildWhile(ParseNode* node);
    ASTPtr buildRepeat(ParseNode* node);
    ASTPtr buildFor(ParseNode* node);
    ASTPtr buildCase(ParseNode* node);
    ASTPtr buildProcedureFunctionCall(ParseNode* node);
    //ekspresi dan var access
    ASTPtr buildExpression(ParseNode* node);
    ASTPtr buildSimpleExpression(ParseNode* node);
    ASTPtr buildTerm(ParseNode* node);
    ASTPtr buildFactor(ParseNode* node);
    ASTPtr buildVariable(ParseNode* node);
    ASTPtr applyComponent(ASTPtr base, ParseNode* componentNode);
    std::vector<ASTPtr> buildIndexList(ParseNode* node);
    //utility semantic translation
    std::string operatorFromNode(ParseNode* node) const;
    std::vector<std::string> identifierList(ParseNode* node) const;
    static void moveAppend(std::vector<ASTPtr>& dst, std::vector<ASTPtr>& src);
};

#endif
#ifndef PARSER_HPP
#define PARSER_HPP

#include "lex_analyzer.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <ostream>

/*  Merepresentasikan satu node di dalam Parse Tree.
 * Bisa berupa:
 *   - Non-terminal: label seperti "<program>", "<expression>"
 *   - Terminal: label seperti "programsy", "ident(Hello)", "intcon(5)" */
struct ParseNode {
    std::string label;
    std::vector<ParseNode*> children;

    ParseNode(const std::string& lbl) : label(lbl) {}

    /* Destruktor rekursif, hapus seluruh subtree */
    ~ParseNode() {
        for (ParseNode* child : children) delete child;
    }

    /* Helper untuk nambah child dan return node ini (untuk chaining) */
    ParseNode* addChild(ParseNode* child) {
        children.push_back(child);
        return this;
    }
};

/* Exception khusus untuk syntax error di parser */
class ParseError : public std::runtime_error {
public:
    int line;
    ParseError(const std::string& msg, int line)
        : std::runtime_error(msg), line(line) {}
};

/* Melakukan Recursive Descent Parsing terhadap list token yang dihasilkan lexer, dan menghasilkan Parse Tree */
class Parser {
public:
    /* Konstruktor: terima list token dari lexer */
    Parser(const std::vector<Token>& tokens);

    /* Entry point: parse seluruh program, return root ParseNode */
    ParseNode* parse();

    /* Cetak parse tree ke output stream (terminal / file) */
    void printTree(ParseNode* node, std::ostream& out,
                   const std::string& prefix = "",
                   bool isLast = true);

    /* ── Token consumer ── */
    Token peek() const;
    Token peekAt(int offset) const;
    Token advance();
    Token expect(TokenType type);
    bool check(TokenType type) const;
    bool checkAny(const std::vector<TokenType>& types) const;
    bool match(TokenType type);
    void error(const std::string& expected);
    bool isAtEnd() const;

    /*  Parse functions: expressions (Job 3)  */
    ParseNode* parseExpression();
    ParseNode* parseSimpleExpression();
    ParseNode* parseTerm();
    ParseNode* parseFactor();
    ParseNode* parseRelationalOperator();
    ParseNode* parseAdditiveOperator();
    ParseNode* parseMultiplicativeOperator();

    /*  Parse functions: variable (job 3)  */
    ParseNode* parseVariable();
    ParseNode* parseComponentVariable();
    ParseNode* parseIndexList();
    ParseNode* parseProcedureFunctionCall();
    ParseNode* parseParameterList();

    /*  Parse functions: program structure (job 1)  */
    ParseNode* parseProgram();
    ParseNode* parseProgramHeader();
    ParseNode* parseDeclarationPart();
    ParseNode* parseConstDeclaration();
    ParseNode* parseConstant();
    ParseNode* parseTypeDeclaration();
    ParseNode* parseType();
    ParseNode* parseArrayType();
    ParseNode* parseRange();
    ParseNode* parseEnumerated();
    ParseNode* parseRecordType();
    ParseNode* parseFieldList();
    ParseNode* parseFieldPart();
    ParseNode* parseVarDeclaration();
    ParseNode* parseIdentifierList();
    ParseNode* parseSubprogramDeclaration();
    ParseNode* parseProcedureDeclaration();
    ParseNode* parseFunctionDeclaration();
    ParseNode* parseFormalParameterList();
    ParseNode* parseParameterGroup();
    ParseNode* parseBlock();

    /*  Parse functions: statements (Job 2)  */
    ParseNode* parseCompoundStatement();
    ParseNode* parseStatementList();
    ParseNode* parseStatement();
    ParseNode* parseAssignmentStatement();
    ParseNode* parseIfStatement();
    ParseNode* parseCaseStatement();
    ParseNode* parseCaseBlock();
    ParseNode* parseWhileStatement();
    ParseNode* parseRepeatStatement();
    ParseNode* parseForStatement();

private:
    std::vector<Token> tokens;
    int pos;

    ParseNode* makeTerminal(const Token& tok);
    std::string tokenLabel(const Token& tok) const;
};

#endif

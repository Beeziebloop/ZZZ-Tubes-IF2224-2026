#include "parser.hpp"
#include <iostream>
#include <sstream>

/* Filter token COMMENT dan BLANK_LINE — parser tidak butuh keduanya. */
Parser::Parser(const std::vector<Token>& rawTokens) : pos(0) {
    for (const Token& tok : rawTokens) {
        if (tok.type == COMMENT) continue;
        if (tok.type == BLANK_LINE) continue;
        tokens.push_back(tok);
    }
}

/* TOKEN CONSUMER */

Token Parser::peek() const {
    if (pos < (int)tokens.size()) return tokens[pos];
    return Token(EOF_TOKEN, "", -1);
}

Token Parser::peekAt(int offset) const {
    int idx = pos + offset;
    if (idx < (int)tokens.size()) return tokens[idx];
    return Token(EOF_TOKEN, "", -1);
}

Token Parser::advance() {
    if (pos < (int)tokens.size()) return tokens[pos++];
    return Token(EOF_TOKEN, "", -1);
}

Token Parser::expect(TokenType type) {
    if (check(type)) return advance();
    std::ostringstream msg;
    msg << "Syntax error at line " << peek().line
        << ": unexpected token '" << tokenLabel(peek())
        << "', expected '" << tokenLabel(Token(type, "", 0)) << "'";
    throw ParseError(msg.str(), peek().line);
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::checkAny(const std::vector<TokenType>& types) const {
    for (TokenType t : types)
        if (check(t)) return true;
    return false;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::isAtEnd() const {
    return peek().type == EOF_TOKEN;
}

void Parser::error(const std::string& expected) {
    std::ostringstream msg;
    msg << "Syntax error at line " << peek().line
        << ": unexpected token '" << tokenLabel(peek())
        << "', expected " << expected;
    throw ParseError(msg.str(), peek().line);
}

/* helper yg return representasi string dari sebuah token untuk label node. */
std::string Parser::tokenLabel(const Token& tok) const {
    switch (tok.type) {
        case INTCON:      return "intcon(" + tok.value + ")";
        case REALCON:     return "realcon(" + tok.value + ")";
        case CHARCON:     return "charcon('" + tok.value + "')";
        case STRING:      return "string('" + tok.value + "')";
        case IDENT:       return "ident(" + tok.value + ")";
        case UNKNOWN:     return "unknown(" + tok.value + ")";
        case NOTSY:       return "notsy";
        case ANDSY:       return "andsy";
        case ORSY:        return "orsy";
        case PLUS:        return "plus";
        case MINUS:       return "minus";
        case TIMES:       return "times";
        case IDIV:        return "idiv";
        case RDIV:        return "rdiv";
        case IMOD:        return "imod";
        case EQL:         return "eql";
        case NEQ:         return "neq";
        case GTR:         return "gtr";
        case GEQ:         return "geq";
        case LSS:         return "lss";
        case LEQ:         return "leq";
        case LPARENT:     return "lparent";
        case RPARENT:     return "rparent";
        case LBRACK:      return "lbrack";
        case RBRACK:      return "rbrack";
        case COMMA:       return "comma";
        case SEMICOLON:   return "semicolon";
        case PERIOD:      return "period";
        case COL:         return "colon";
        case BECOMES:     return "becomes";
        case CONSTSY:     return "constsy";
        case TYPESY:      return "typesy";
        case VARSY:       return "varsy";
        case FUNCTIONSY:  return "functionsy";
        case PROCEDURESY: return "proceduresy";
        case ARRAYSY:     return "arraysy";
        case RECORDSY:    return "recordsy";
        case PROGRAMSY:   return "programsy";
        case BEGINSY:     return "beginsy";
        case IFSY:        return "ifsy";
        case CASESY:      return "casesy";
        case REPEATSY:    return "repeatsy";
        case WHILESY:     return "whilesy";
        case FORSY:       return "forsy";
        case ENDSY:       return "endsy";
        case ELSESY:      return "elsesy";
        case UNTILSY:     return "untilsy";
        case OFSY:        return "ofsy";
        case DOSY:        return "dosy";
        case TOSY:        return "tosy";
        case DOWNTOSY:    return "downtosy";
        case THENSY:      return "thensy";
        case EOF_TOKEN:   return "EOF";
        case COMMENT:     return "comment";
        default:          return "unknown";
    }
}

/*helper makeTerminal Buat ParseNode terminal dari token yang sudah di-consume. */
ParseNode* Parser::makeTerminal(const Token& tok) {
    return new ParseNode(tokenLabel(tok));
}

/* Helper rekursif internal untuk printTree. */
static void printTreeHelper(ParseNode* node, std::ostream& out,
                             const std::string& prefix, bool isLast) {
    if (!node) return;
    out << prefix << (isLast ? "└── " : "├── ") << node->label << "\n";
    std::string childPrefix = prefix + (isLast ? "    " : "│   ");
    for (int i = 0; i < (int)node->children.size(); i++) {
        bool last = (i == (int)node->children.size() - 1);
        printTreeHelper(node->children[i], out, childPrefix, last);
    }
}

/* Format output dengan karakter box-drawing seperti di spesifikasi. */
void Parser::printTree(ParseNode* node, std::ostream& out,
                       const std::string& prefix, bool isLast) {
    if (!node) return;
    if (prefix.empty()) {
        /* Root: cetak label tanpa connector, children mulai dari level 1 */
        out << node->label << "\n";
        for (int i = 0; i < (int)node->children.size(); i++) {
            bool last = (i == (int)node->children.size() - 1);
            printTreeHelper(node->children[i], out, "", last);
        }
    } else {
        printTreeHelper(node, out, prefix, isLast);
    }
}

/* ENTRY POINT: parse() */
ParseNode* Parser::parse() {
    ParseNode* tree = parseProgram();
    if (!isAtEnd()) {
        error("end of file");
    }
    return tree;
}

/* job 1: Program Structure & Declarations  */

ParseNode* Parser::parseProgram() {}
ParseNode* Parser::parseProgramHeader() {}
ParseNode* Parser::parseDeclarationPart() {}
ParseNode* Parser::parseConstDeclaration() {}
/* PARSE CONSTANT
 * constant → charcon | string | [(plus | minus)? + (ident | intcon | realcon)]
 * Diimplementasikan di sini karena dibutuhkan oleh parseCaseBlock (job 2). */
ParseNode* Parser::parseConstant() {
    ParseNode* node = new ParseNode("<constant>");
    if (check(CHARCON)) {
        node->addChild(makeTerminal(advance()));
    } else if (check(STRING)) {
        node->addChild(makeTerminal(advance()));
    } else {
        // Optional sign
        if (check(PLUS) || check(MINUS))
            node->addChild(makeTerminal(advance()));
        if (checkAny({IDENT, INTCON, REALCON}))
            node->addChild(makeTerminal(advance()));
        else
            error("constant (charcon, string, ident, intcon, or realcon)");
    }
    return node;
}
ParseNode* Parser::parseTypeDeclaration() {}
ParseNode* Parser::parseType() {}
ParseNode* Parser::parseArrayType() {}
ParseNode* Parser::parseRange() {}
ParseNode* Parser::parseEnumerated() {}
ParseNode* Parser::parseRecordType() {}
ParseNode* Parser::parseFieldList() {}
ParseNode* Parser::parseFieldPart() {}
ParseNode* Parser::parseVarDeclaration() {}
ParseNode* Parser::parseIdentifierList() {}
ParseNode* Parser::parseSubprogramDeclaration() {}
ParseNode* Parser::parseProcedureDeclaration() {}
ParseNode* Parser::parseFunctionDeclaration() {}
ParseNode* Parser::parseFormalParameterList() {}
ParseNode* Parser::parseParameterGroup() {}
ParseNode* Parser::parseBlock() {}

/* job 2: Statements */

/* PARSE COMPOUND STATEMENT
 * compound-statement → beginsy + statement-list + endsy */
ParseNode* Parser::parseCompoundStatement() {
    ParseNode* node = new ParseNode("<compound-statement>");
    node->addChild(makeTerminal(expect(BEGINSY)));
    node->addChild(parseStatementList());
    node->addChild(makeTerminal(expect(ENDSY)));
    return node;
}

/* PARSE STATEMENT LIST
 * statement-list → statement (semicolon + statement)* */
ParseNode* Parser::parseStatementList() {
    ParseNode* node = new ParseNode("<statement-list>");
    node->addChild(parseStatement());
    while (check(SEMICOLON)) {
        node->addChild(makeTerminal(advance())); // semicolon
        node->addChild(parseStatement());
    }
    return node;
}

/* PARSE STATEMENT
 * ident diikuti becomes ->  assignment,
 * ident diikuti lparent (atau langsung ke statement lain) ->  proc/func-call,
 * ident berdiri sendiri (variable access) -> assignment (setelah component-variable.) */
ParseNode* Parser::parseStatement() {
    ParseNode* node = new ParseNode("<statement>");

    if (check(BEGINSY)) {
        node->addChild(parseCompoundStatement());
    } else if (check(IFSY)) {
        node->addChild(parseIfStatement());
    } else if (check(CASESY)) {
        node->addChild(parseCaseStatement());
    } else if (check(WHILESY)) {
        node->addChild(parseWhileStatement());
    } else if (check(REPEATSY)) {
        node->addChild(parseRepeatStatement());
    } else if (check(FORSY)) {
        node->addChild(parseForStatement());
    } else if (check(IDENT)) {
        /* Perlu lookahead untuk membedakan assignment vs proc/func-call.
         * Procedure/function-call: ident langsung diikuti lparent.
         * Assignment: ident (component-variable)* becomes.
         * Untuk menentukan, scan ke depan melewati component-variable. */
        if (peekAt(1).type == LPARENT) {
            // ident( ... ) → procedure/function-call sebagai statement
            node->addChild(parseProcedureFunctionCall());
        } else {
            /* Coba parse sebagai assignment. Jika setelah ident ada
             * '[' atau '.' maka ini variable dengan component, lalu harus
             * diikuti becomes. Jika ident langsung diikuti becomes → assignment.
             * Keduanya di-handle oleh parseAssignmentStatement(). */
            node->addChild(parseAssignmentStatement());
        }
    }
    // Statement boleh kosong (empty statement) — node tanpa anak valid.

    return node;
}

/* PARSE ASSIGNMENT STATEMENT
 * assignment-statement → variable + becomes + expression */
ParseNode* Parser::parseAssignmentStatement() {
    ParseNode* node = new ParseNode("<assignment-statement>");
    node->addChild(parseVariable());
    node->addChild(makeTerminal(expect(BECOMES)));
    node->addChild(parseExpression());
    return node;
}

/* PARSE IF STATEMENT
 * if-statement → ifsy + expression + thensy + statement (elsesy + statement)? */
ParseNode* Parser::parseIfStatement() {
    ParseNode* node = new ParseNode("<if-statement>");
    node->addChild(makeTerminal(expect(IFSY)));
    node->addChild(parseExpression());
    node->addChild(makeTerminal(expect(THENSY)));
    node->addChild(parseStatement());
    if (check(ELSESY)) {
        node->addChild(makeTerminal(advance())); // elsesy
        node->addChild(parseStatement());
    }
    return node;
}

/* PARSE CASE STATEMENT
 * case-statement → casesy + expression + ofsy + case-block + endsy */
ParseNode* Parser::parseCaseStatement() {
    ParseNode* node = new ParseNode("<case-statement>");
    node->addChild(makeTerminal(expect(CASESY)));
    node->addChild(parseExpression());
    node->addChild(makeTerminal(expect(OFSY)));
    node->addChild(parseCaseBlock());
    node->addChild(makeTerminal(expect(ENDSY)));
    return node;
}

/* PARSE CASE BLOCK
 * case-block → constant (comma + constant)* colon statement (semicolon + case-block?)* */
ParseNode* Parser::parseCaseBlock() {
    ParseNode* node = new ParseNode("<case-block>");
    // Satu atau lebih constant dipisah koma
    node->addChild(parseConstant());
    while (check(COMMA)) {
        node->addChild(makeTerminal(advance())); // comma
        node->addChild(parseConstant());
    }
    node->addChild(makeTerminal(expect(COL))); // colon
    node->addChild(parseStatement());
    // Semicolon lalu optional case-block lagi (bisa kosong sebelum end)
    while (check(SEMICOLON)) {
        node->addChild(makeTerminal(advance())); // semicolon
        // Jika token berikutnya adalah end, case-block kosong — berhenti
        if (check(ENDSY)) break;
        node->addChild(parseCaseBlock());
    }
    return node;
}

/* PARSE WHILE STATEMENT
 * while-statement → whilesy + expression + dosy + statement */
ParseNode* Parser::parseWhileStatement() {
    ParseNode* node = new ParseNode("<while-statement>");
    node->addChild(makeTerminal(expect(WHILESY)));
    node->addChild(parseExpression());
    node->addChild(makeTerminal(expect(DOSY)));
    node->addChild(parseStatement());
    return node;
}

/* PARSE REPEAT STATEMENT
 * repeat-statement → repeatsy + statement-list + untilsy + expression */
ParseNode* Parser::parseRepeatStatement() {
    ParseNode* node = new ParseNode("<repeat-statement>");
    node->addChild(makeTerminal(expect(REPEATSY)));
    node->addChild(parseStatementList());
    node->addChild(makeTerminal(expect(UNTILSY)));
    node->addChild(parseExpression());
    return node;
}

/* PARSE FOR STATEMENT
 * for-statement → forsy + ident + becomes + expression + (tosy | downtosy) + expression + dosy + statement */
ParseNode* Parser::parseForStatement() {
    ParseNode* node = new ParseNode("<for-statement>");
    node->addChild(makeTerminal(expect(FORSY)));
    node->addChild(makeTerminal(expect(IDENT)));
    node->addChild(makeTerminal(expect(BECOMES)));
    node->addChild(parseExpression());
    if (check(TOSY)) {
        node->addChild(makeTerminal(advance())); // tosy
    } else if (check(DOWNTOSY)) {
        node->addChild(makeTerminal(advance())); // downtosy
    } else {
        error("'to' or 'downto' in for-statement");
    }
    node->addChild(parseExpression());
    node->addChild(makeTerminal(expect(DOSY)));
    node->addChild(parseStatement());
    return node;
}

/* PARSE EXPRESSION
 * expression → simple-expression (relational-operator + simple-expression)? */
ParseNode* Parser::parseExpression() {
    ParseNode* node = new ParseNode("<expression>");
    node->addChild(parseSimpleExpression());
    if (checkAny({EQL, NEQ, GTR, GEQ, LSS, LEQ})) {
        node->addChild(parseRelationalOperator());
        node->addChild(parseSimpleExpression());
    }
    return node;
}

/* PARSE SIMPLE EXPRESSION
 * simple-expression → (plus | minus)? term (additive-operator + term)* */
ParseNode* Parser::parseSimpleExpression() {
    ParseNode* node = new ParseNode("<simple-expression>");
    if (check(PLUS) || check(MINUS))
        node->addChild(makeTerminal(advance()));
    node->addChild(parseTerm());
    while (checkAny({PLUS, MINUS, ORSY})) {
        node->addChild(parseAdditiveOperator());
        node->addChild(parseTerm());
    }
    return node;
}

/* PARSE TERM
 * term → factor (multiplicative-operator + factor)* */
ParseNode* Parser::parseTerm() {
    ParseNode* node = new ParseNode("<term>");
    node->addChild(parseFactor());
    while (checkAny({TIMES, RDIV, IDIV, IMOD, ANDSY})) {
        node->addChild(parseMultiplicativeOperator());
        node->addChild(parseFactor());
    }
    return node;
}

/* PARSE FACTOR
 * factor → intcon | realcon | charcon | string
 *         | (lparent + expression + rparent)
 *         | (notsy + factor)
 *         | procedure/function-call   (ident + lparent)
 *         | variable                  (ident + component*) */
ParseNode* Parser::parseFactor() {
    ParseNode* node = new ParseNode("<factor>");

    if (check(INTCON)) {
        node->addChild(makeTerminal(advance()));
    } else if (check(REALCON)) {
        node->addChild(makeTerminal(advance()));
    } else if (check(CHARCON)) {
        node->addChild(makeTerminal(advance()));
    } else if (check(STRING)) {
        node->addChild(makeTerminal(advance()));
    } else if (check(LPARENT)) {
        node->addChild(makeTerminal(advance())); // lparent
        node->addChild(parseExpression());
        node->addChild(makeTerminal(expect(RPARENT)));
    } else if (check(NOTSY)) {
        node->addChild(makeTerminal(advance())); // notsy
        node->addChild(parseFactor());
    } else if (check(IDENT)) {
        if (peekAt(1).type == LPARENT) {
            node->addChild(parseProcedureFunctionCall());
        } else {
            node->addChild(parseVariable());
        }
    } else {
        error("factor (intcon, realcon, charcon, string, ident, '(', or 'not')");
    }

    return node;
}

/* PARSE RELATIONAL OPERATOR
 * relational-operator → eql | neq | gtr | geq | lss | leq */
ParseNode* Parser::parseRelationalOperator() {
    ParseNode* node = new ParseNode("<relational-operator>");
    if (checkAny({EQL, NEQ, GTR, GEQ, LSS, LEQ}))
        node->addChild(makeTerminal(advance()));
    else
        error("relational operator (==, <>, >, >=, <, <=)");
    return node;
}

/* PARSE ADDITIVE OPERATOR
 * additive-operator → plus | minus | orsy */
ParseNode* Parser::parseAdditiveOperator() {
    ParseNode* node = new ParseNode("<additive-operator>");
    if (checkAny({PLUS, MINUS, ORSY}))
        node->addChild(makeTerminal(advance()));
    else
        error("additive operator (+, -, or)");
    return node;
}

/* PARSE MULTIPLICATIVE OPERATOR
 * multiplicative-operator → times | rdiv | idiv | imod | andsy */
ParseNode* Parser::parseMultiplicativeOperator() {
    ParseNode* node = new ParseNode("<multiplicative-operator>");
    if (checkAny({TIMES, RDIV, IDIV, IMOD, ANDSY}))
        node->addChild(makeTerminal(advance()));
    else
        error("multiplicative operator (*, /, div, mod, and)");
    return node;
}

/* PARSE VARIABLE
 * variable → ident (component-variable)* */
ParseNode* Parser::parseVariable() {
    ParseNode* node = new ParseNode("<variable>");
    node->addChild(makeTerminal(expect(IDENT)));
    while (check(LBRACK) || check(PERIOD)) {
        node->addChild(parseComponentVariable());
    }
    return node;
}

/* PARSE COMPONENT VARIABLE
 * component-variable → (lbrack + index-list + rbrack) | (period + ident) */
ParseNode* Parser::parseComponentVariable() {
    ParseNode* node = new ParseNode("<component-variable>");
    if (check(LBRACK)) {
        node->addChild(makeTerminal(advance())); // lbrack
        node->addChild(parseIndexList());
        node->addChild(makeTerminal(expect(RBRACK)));
    } else if (check(PERIOD)) {
        node->addChild(makeTerminal(advance())); // period
        node->addChild(makeTerminal(expect(IDENT)));
    } else {
        error("'[' or '.' for component variable");
    }
    return node;
}

/* PARSE INDEX LIST
 * index-list → (intcon | charcon | ident) (comma + index-list)* */
ParseNode* Parser::parseIndexList() {
    ParseNode* node = new ParseNode("<index-list>");
    if (checkAny({INTCON, CHARCON, IDENT}))
        node->addChild(makeTerminal(advance()));
    else
        error("index (intcon, charcon, or ident)");
    while (check(COMMA)) {
        node->addChild(makeTerminal(advance())); // comma
        node->addChild(parseIndexList());
    }
    return node;
}

/* PARSE PROCEDURE/FUNCTION CALL
 * procedure/function-call → ident (lparent + parameter-list? + rparent)? */
ParseNode* Parser::parseProcedureFunctionCall() {
    ParseNode* node = new ParseNode("<procedure/function-call>");
    node->addChild(makeTerminal(expect(IDENT)));
    if (check(LPARENT)) {
        node->addChild(makeTerminal(advance())); // lparent
        if (!check(RPARENT))
            node->addChild(parseParameterList());
        node->addChild(makeTerminal(expect(RPARENT)));
    }
    return node;
}

/* PARSE PARAMETER LIST
 * parameter-list → expression (comma + expression)* */
ParseNode* Parser::parseParameterList() {
    ParseNode* node = new ParseNode("<parameter-list>");
    node->addChild(parseExpression());
    while (check(COMMA)) {
        node->addChild(makeTerminal(advance())); // comma
        node->addChild(parseExpression());
    }
    return node;
}

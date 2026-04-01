#include "lex_analyzer.hpp"
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

Lexer::Lexer(const std::string& filename) : currentLine(1) {
    inputFile.open(filename);
    if(!inputFile.is_open()){
        throw std::runtime_error("File " + filename + " tidak bisa dibuka.");
    }
}

Lexer::~Lexer(){
    if(inputFile.is_open()){
        inputFile.close();
    }
}

std::vector<Token> Lexer::tokenize(){
    std::vector<Token> tokens;
    int lastLine = 1; //track baris token sebelumnya
    TokenType lastType = UNKNOWN; //track tipe token sebelumnya
    //loop utamanya terus panggil nexToken sampe EOF
    while(true){
        Token token = nextToken();
        if(token.type == COMMENT){
            continue; //skip komentar
        }
        int lineDiff = token.line - lastLine; //hitung selisih baris dengan token sebelumnya
        bool addBlankBeforeEnd = (token.type == ENDSY && lastType == SEMICOLON && lineDiff == 1);
        if(lineDiff > 1 || addBlankBeforeEnd){ //sisipkan baris kosong dari input, plus jeda sebelum end
            tokens.push_back(Token(BLANK_LINE, "", 0)); //sebagai blank line marker
        }
        tokens.push_back(token);
        lastLine = token.line; //update lastLine
        lastType = token.type; //update lastType
        if(token.type == EOF_TOKEN){
            break;
        }
    }
    return tokens;
}

char Lexer::readChar(){
    char c = inputFile.get();
    if(c == '\n'){
        currentLine++;
    }
    return c;
}

void Lexer::unreadChar(char c){
    if (c == '\n'){
        currentLine--;
    }
    inputFile.putback(c);
}

void Lexer::skipWhitespace(){
    char c;
    while(inputFile.get(c)){
        if(c == '\n'){
            currentLine++;
        }
        if(!std::isspace((unsigned char)c)){
            unreadChar(c);
            break;
        }
    }
}

Token Lexer::nextToken(){
    skipWhitespace();
    char c = readChar();
    int line = currentLine;

    if(inputFile.eof() || inputFile.fail()){
        return Token(EOF_TOKEN, "", line);
    }
 
    //kalau mulai dengan digit
    if(std::isdigit((unsigned char)c)){
        return readNum(c);
    }
 
    //kalau mulai dengan huruf
    if(std::isalpha((unsigned char)c)){
        return readIdentOrKeyw(c);
    }
 
    //kalau mulai dengan petik tunggal
    if(c == '\''){
        return readStringOrChar();
    }
 
    //operator dan delimiter
    switch (c){
        case '+': return Token(PLUS, "", line);
        case '-': return Token(MINUS, "", line);
        case '*': return Token(TIMES, "", line);
        case '/': return Token(RDIV, "", line);
        case ')': return Token(RPARENT, "", line);
        case '[': return Token(LBRACK, "", line);
        case ']': return Token(RBRACK, "", line);
        case ',': return Token(COMMA, "", line);
        case ';': return Token(SEMICOLON, "", line);
        case '.': return Token(PERIOD, "", line);
        case ':': return readColon();
        case '<': return readLessThan();
        case '>': return readGreaterThan();
        case '=': return readEquals();
        case '(': return readLParenOrComment();
        case '{': return readBraceComment();
        default: return Token(UNKNOWN, std::string(1, c), line);
    }
}

Token Lexer::readNum(char firstChar){
    std::string buffer(1, firstChar);
    State state = INT;
    int line = currentLine;
 
    while(true){
        char c = readChar();
        if(state == INT){
            if(std::isdigit((unsigned char)c)){ //tetap di state INT, kumpulkan digit
                buffer += c;
            }else if(c == '.'){ //pindah ke state REAL
                buffer += c;
                state = REAL;
            }else{ //kalau bukan digit atau titik, token integer selesai
                unreadChar(c); //backtrack
                return Token(INTCON, buffer, line);
            }
        }else{ //state REAL
            if(std::isdigit((unsigned char)c)){ //stay di state, kumpulin digit desimal
                buffer += c;
            }else{
                unreadChar(c); //backtrack
                return Token(REALCON, buffer, line);
            }
        }
    }
}

Token Lexer::readIdentOrKeyw(char firstChar){
    std::string buffer(1, firstChar);
    int line = currentLine;
 
    while(true){
        char c = readChar();
        if(std::isalpha((unsigned char)c) || std::isdigit((unsigned char)c)){
            buffer += c;
        }else{
            unreadChar(c); //karakter bukan bagian dari ident jadinya backtrack
            break;
        }
    }
    //cek apakah buffer itu keyword atau identifier biasa
    TokenType type = checkKeyword(toLower(buffer));
    if(type == IDENT){
        //kalau bukan, kembalikan nilai asli (preserve case)
        return Token(IDENT, buffer, line);
    }else{
        //kalau iya, ga perlu simpen nilai
        return Token(type, "", line);
    }
}

Token Lexer::readStringOrChar() {
    std::string buffer; // isi string tanpa tanda petik
    State state = STR_OPEN;
    int line = currentLine;
 
    while(true){
        char c = readChar();
        if(inputFile.eof() || inputFile.fail()){
            return Token(UNKNOWN, "Unterminated string", line);
        }
 
        if(state == STR_OPEN){
            if(c == '\''){
                /* Bisa penutup atau escaped quote — pindah ke STR_QUOTE */
                state = STR_QUOTE;
            }else if(c == '\n') { //string gabisa multiline
                return Token(UNKNOWN, "Unterminated string", line);
            }else{
                buffer += c;
            }
        }else{ //state STR_Quote
            if(c == '\''){ //ketemu '', artinya ini escaped quote, masukin ' ke buffer dan balik ke STR_OPEN
                buffer += '\'';
                state = STR_OPEN;
            }else{
                unreadChar(c); //kalau bukan '', artinya udah bener-bener selesai jadinya backtrack
                //Nentuin charcon (tepat 1 char) atau string
                if(buffer.length() == 1){
                    return Token(CHARCON, buffer, line);
                }else{
                    return Token(STRING, buffer, line);
                }
            }
        }
    }
}
 
Token Lexer::readColon(){
    int line = currentLine;
    char c = readChar();
    if(c == '='){
        return Token(BECOMES, "", line);
    }else{
        unreadChar(c);
        return Token(COL, "", line);
    }
}

Token Lexer::readLessThan(){
    int line = currentLine;
    char c = readChar();
    if(c == '='){
        return Token(LEQ, "", line);
    }
    if(c == '>'){
        return Token(NEQ, "", line);
    }
    unreadChar(c);
    return Token(LSS, "", line);
}

Token Lexer::readGreaterThan(){
    int line = currentLine;
    char c = readChar();
    if(c == '='){
        return Token(GEQ, "", line);
    }
    unreadChar(c);
    return Token(GTR, "", line);
}

Token Lexer::readEquals(){
    int line = currentLine;
    char c = readChar();
    if(c == '='){
        return Token(EQL, "", line);
    }
    unreadChar(c);
    return Token(UNKNOWN, "=", line); //= tunggal itu ga valid
}

Token Lexer::readLParenOrComment(){
    int line = currentLine;
    char c = readChar();
    if(c == '*'){
        return readParenComment();
    }else{
        unreadChar(c);
        return Token(LPARENT, "", line);
    }
}

Token Lexer::readBraceComment(){
    int line = currentLine;
    while(true){
        char c = readChar();
        if (inputFile.eof() || inputFile.fail()) {
            return Token(UNKNOWN, "Unterminated comment", line);
        }
        if (c == '}') {
            return Token(COMMENT, "", line); //komentar selesai
        }
        //kalau ada karakter lain tetap di S_CMT_BRACES
    }
}

Token Lexer::readParenComment(){
    int line = currentLine;
    State state = C_PARENTHESIS_BODY;
    while(true){
        char c = readChar();
        if(inputFile.eof() || inputFile.fail()){
            return Token(UNKNOWN, "Unterminated comment", line);
        }
 
        if(state == C_PARENTHESIS_BODY){
            if(c == '*') state = C_STAR;
            //Kalau ada karakter lain tetap di C_PARENTHESIS_BODY
        }else{ //state C_STAR
            if(c == ')'){
                return Token(COMMENT, "", line); //komentar selesai
            }else if(c == '*') {
                //Kalau ada karakter lain tetap di C_STAR, misal: (** ... *)
            }else{
                state = C_PARENTHESIS_BODY;
            }
        }
    }
}

TokenType Lexer::checkKeyword(const std::string& word)const{
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"not", NOTSY},
        {"and", ANDSY},
        {"or", ORSY},
        {"div", IDIV},
        {"mod", IMOD},
        {"const", CONSTSY},
        {"type", TYPESY},
        {"var", VARSY},
        {"function", FUNCTIONSY},
        {"procedure", PROCEDURESY},
        {"array", ARRAYSY},
        {"record", RECORDSY},
        {"program", PROGRAMSY},
        {"begin", BEGINSY},
        {"if", IFSY},
        {"case", CASESY},
        {"repeat", REPEATSY},
        {"while", WHILESY},
        {"for", FORSY},
        {"end", ENDSY},
        {"else", ELSESY},
        {"until", UNTILSY},
        {"of", OFSY},
        {"do", DOSY},
        {"to", TOSY},
        {"downto", DOWNTOSY},
        {"then", THENSY},
    };
 
    auto it = keywords.find(word);
    if (it != keywords.end()) return it->second;
    return IDENT;
}

std::string Lexer::toLower(const std::string& s)const{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){
        return std::tolower(c); 
    });
    return result;
}

std::string Lexer::tokenToString(const Token& token)const{
    switch (token.type) {
        //token dengan nilai
        case INTCON: return "intcon (" + token.value + ")";
        case REALCON: return "realcon (" + token.value + ")";
        case CHARCON: return "charcon ('" + token.value + "')";
        case STRING: return "string ('" + token.value + "')";
        case IDENT: return "ident (" + token.value + ")";
        case UNKNOWN: return "UNKNOWN (" + token.value + ")";
        //token tanpa nilai
        case NOTSY: return "notsy";
        case ANDSY: return "andsy";
        case ORSY: return "orsy";
        case PLUS: return "plus";
        case MINUS: return "minus";
        case TIMES: return "times";
        case IDIV: return "idiv";
        case RDIV: return "rdiv";
        case IMOD: return "imod";
        case EQL: return "eql";
        case NEQ: return "neq";
        case GTR: return "gtr";
        case GEQ: return "geq";
        case LSS: return "lss";
        case LEQ: return "leq";
        case LPARENT: return "lparent";
        case RPARENT: return "rparent";
        case LBRACK: return "lbrack";
        case RBRACK: return "rbrack";
        case COMMA: return "comma";
        case SEMICOLON: return "semicolon";
        case PERIOD: return "period";
        case COL: return "colon";
        case BECOMES: return "becomes";
        case CONSTSY: return "constsy";
        case TYPESY: return "typesy";
        case VARSY: return "varsy";
        case FUNCTIONSY: return "functionsy";
        case PROCEDURESY: return "proceduresy";
        case ARRAYSY: return "arraysy";
        case RECORDSY: return "recordsy";
        case PROGRAMSY: return "programsy";
        case BEGINSY: return "beginsy";
        case IFSY: return "ifsy";
        case CASESY: return "casesy";
        case REPEATSY: return "repeatsy";
        case WHILESY: return "whilesy";
        case FORSY: return "forsy";
        case ENDSY: return "endsy";
        case ELSESY: return "elsesy";
        case UNTILSY: return "untilsy";
        case OFSY: return "ofsy";
        case DOSY: return "dosy";
        case TOSY: return "tosy";
        case DOWNTOSY: return "downtosy";
        case THENSY: return "thensy";
        case EOF_TOKEN: return "EOF";
        case COMMENT: return "comment";
        default: return "UNKNOWN";
    }
}
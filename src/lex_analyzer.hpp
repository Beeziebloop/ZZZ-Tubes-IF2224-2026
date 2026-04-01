#ifndef LEX_ANALYZER_HPP
#define LEX_ANALYZER_HPP
 
#include <string>
#include <fstream>
#include <vector>

//Token Types
enum TokenType{
    //Literals
    INTCON, REALCON, CHARCON, STRING,
    //Logicals
    NOTSY, ANDSY, ORSY,
    //Arithmatics
    PLUS, MINUS, TIMES, IDIV, RDIV, IMOD,
    //Comparions
    EQL, NEQ, GTR, GEQ, LSS, LEQ,
    //Delimiter
    LPARENT, RPARENT, LBRACK, RBRACK,
    COMMA, SEMICOLON, PERIOD, COL, BECOMES,
    //Declaration keywords
    CONSTSY, TYPESY, VARSY, FUNCTIONSY, PROCEDURESY,
    ARRAYSY, RECORDSY, PROGRAMSY,
    //Control flow keywords
    BEGINSY, IFSY, CASESY, REPEATSY, WHILESY, FORSY,
    ENDSY, ELSESY, UNTILSY, OFSY, DOSY, TOSY, DOWNTOSY, THENSY,
    //Identifier
    IDENT,
    //Comment
    COMMENT,
    //Error
    UNKNOWN,
    //EOF
    EOF_TOKEN,
    //Penanda baris kosong di output
    BLANK_LINE
};

//Nyimpen satu token hasil analisis leksikal
struct Token{
    TokenType type; //jenis token
    std::string value; //nilai token
    int line; //nomor baris untuk pesan error
    Token(TokenType t, const std::string& v = "", int l = 0) : type(t), value(v), line(l) {}
};

//States-states representasi kondisi DFA
enum State{
    START, //start
    INT, //ketika lagi baca integer
    REAL, //ketika lagi baca reak
    ID, //ketika lagi baca identifier/keyword
    STR_OPEN, //setelah bertemu ' pertama
    STR_QUOTE, //setelah baca ' 
    COLON, //setelah bertemu :
    LST, //setelah bertemu <
    GTT, //setelah bertemu >
    EQ, //setelah bertemu =
    PARENTHESIS, //setelah bertemu (
    C_BRACES, //dalam komentar {...}
    C_PARENTHESIS_BODY, //dalam (*...*)
    C_STAR, //baru baca * di dalam komentar
    ACCEPT, //state keterima
    ERROR //karakter tak dikenal/undefined state
};

//Lexer, main component yang melakukan analisis lexical dan memproses input karakter per karakter seperti DFA
class Lexer{
    public:
        Lexer(const std::string& filename); //ctor
        ~Lexer(); //dtor
        std::vector<Token> tokenize(); //tokenize seluruh file dan mengembalikan list token
        std::string tokenToString(const Token& token) const; //mengembalikan representasi string dari sebuah token (output)

    private:
        std::ifstream inputFile; //file source code
        int currentLine; //nomor baris currently (buat error reporting)
        char readChar(); //baca satu karakter dari file
        void unreadChar(char c); //mengembalikan karakter ke stream (backtrack)
        void skipWhitespace(); //skip whitespace dan newline
        Token nextToken(); //ambil next token dari input
        Token readNum(char firstChar); //baca int atau real
        Token readIdentOrKeyw(char firstChar); //baca identifier/keyword
        Token readStringOrChar(); //baca string/charcon (dipanggil setelah ' pertama)
        Token readColon(); //handler :, bisa jadi colon/becomes
        Token readLessThan(); //handler <, bisa jadi lesser than, lesser than or equal, atau not equal
        Token readGreaterThan(); //handler >, bisa jadi greater than, greater than or equal
        Token readEquals(); //handler =, harus diikuti another = buat jadi equal
        Token readLParenOrComment(); //handler (, bisa jadi lparent atau awal komentar (*...*)
        Token readBraceComment(); //Baca komentar {...} (dipanggil setelah { dibaca)
        Token readParenComment(); //baca komentar (*...*) (dipanggil setelah (* dibaca)
        TokenType checkKeyword(const std::string& word) const; //mengecek apakah string keyword, lalu kembalikan TokenType yang sesuai
        std::string toLower(const std::string& s) const; //lowercase seluruh string untuk keyword case-insensitive
};

#endif
#include "lex_analyzer.hpp"
#include <iostream>
#include <fstream>
#include <vector>

void writeTokens(const std::vector<Token>& tokens,const Lexer& lexer, std::ostream& out)
{
    for(const Token& tok : tokens){
        if(tok.type == EOF_TOKEN){
            break;
        }
        //blank line marker
        if(tok.type == BLANK_LINE){
            out << "\n"; //tulis blank line
            continue;
        }
        out << lexer.tokenToString(tok) << "\n";
    }
}

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <input.txt> [output.txt]" << std::endl;
        return 1;
    }
 
    try{
        //initialisasi dengan file input
        Lexer lexer(argv[1]);
        //start lex analyzer
        std::vector<Token> tokens = lexer.tokenize();
        //tulis output ke console
        writeTokens(tokens, lexer, std::cout);
        //kalau ada argumen output file, tulis juga ke sana
        if(argc >= 3){
            std::ofstream outFile(argv[2]);
            if(!outFile.is_open()){
                std::cerr << "Error: Tidak bisa membuka file output: " << argv[2] << std::endl;
                return 1;
            }
            writeTokens(tokens, lexer, outFile);
            std::cerr << "Output ditulis ke: " << argv[2] << std::endl;
        }
    }catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
 
    return 0;
}
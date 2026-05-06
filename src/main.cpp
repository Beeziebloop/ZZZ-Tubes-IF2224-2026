#include "lex_analyzer.hpp"
#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <vector>

void writeTokens(const std::vector<Token>& tokens, const Lexer& lexer, std::ostream& out) {
    for (const Token& tok : tokens) {
        if (tok.type == EOF_TOKEN) break;
        if (tok.type == BLANK_LINE) { out << "\n"; continue; }
        out << lexer.tokenToString(tok) << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.txt> [parse_tree_output.txt]"
                  << " or <input.txt> <token_output.txt> <parse_tree_output.txt>"
                  << std::endl;
        return 1;
    }

    try {
        /* ── Milestone 1: Lexical Analysis ── */
        Lexer lexer(argv[1]);
        std::vector<Token> tokens = lexer.tokenize();

        const bool treeOnlyMode = (argc == 3);

        /* Tulis token ke terminal, kecuali saat mode parse tree saja */
        if (!treeOnlyMode) {
            writeTokens(tokens, lexer, std::cout);
        }

        /* Opsional: simpan token ke file */
        if (argc >= 4) {
            std::ofstream tokenOut(argv[2]);
            if (!tokenOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output token: "
                          << argv[2] << std::endl;
                return 1;
            }
            writeTokens(tokens, lexer, tokenOut);
            std::cerr << "Token output ditulis ke: " << argv[2] << std::endl;
        }

        /* ── Milestone 2: Syntax Analysis ── */
        Parser parser(tokens);
        ParseNode* tree = parser.parse();

        /* Mode parse tree saja: tulis langsung ke file output */
        if (treeOnlyMode) {
            std::ofstream treeOut(argv[2]);
            if (!treeOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output parse tree: "
                          << argv[2] << std::endl;
                delete tree;
                return 1;
            }
            parser.printTree(tree, treeOut);
        } else {
            std::cout << "\n--- PARSE TREE ---\n";

            /* Cetak parse tree ke terminal */
            parser.printTree(tree, std::cout);

            /* Opsional: simpan parse tree ke file */
            if (argc >= 4) {
                std::ofstream treeOut(argv[3]);
                if (!treeOut.is_open()) {
                    std::cerr << "Error: Tidak bisa membuka file output parse tree: "
                              << argv[3] << std::endl;
                    delete tree;
                    return 1;
                }
                parser.printTree(tree, treeOut);
                std::cerr << "Parse tree output ditulis ke: " << argv[3] << std::endl;
            }
        }

        delete tree;

    } catch (const ParseError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

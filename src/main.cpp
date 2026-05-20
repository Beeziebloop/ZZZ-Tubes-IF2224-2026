#include "lex_analyzer.hpp"
#include "parser.hpp"
#include "ast_builder.hpp"
#include "semantic_analyzer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

void writeTokens(const std::vector<Token>& tokens, const Lexer& lexer, std::ostream& out) {
    for (const Token& tok : tokens) {
        if (tok.type == EOF_TOKEN) break;
        if (tok.type == BLANK_LINE) {
            out << "\n";
            continue;
        }
        out << lexer.tokenToString(tok) << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.txt> [output.txt] [--full]"
                  << " or <input.txt> <token_output.txt> <parse_tree_output.txt>"
                  << std::endl;
        return 1;
    }

    // Cek flag --full
    bool fullMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--full") {
            fullMode = true;
        }
    }

    try {
        /* Lexical Analysis */
        Lexer lexer(argv[1]);
        std::vector<Token> tokens = lexer.tokenize();

        /* Tulis token ke terminal */
        writeTokens(tokens, lexer, std::cout);

        /* Mode --full: tulis semua section ke satu output file */
        if (fullMode && argc >= 3) {
            std::ofstream fullOut(argv[2]);
            if (!fullOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output: " << argv[2] << std::endl;
                return 1;
            }

            writeTokens(tokens, lexer, fullOut);

            fullOut << "\n--- PARSE TREE ---\n";
            Parser parser(tokens);
            std::unique_ptr<ParseNode> tree(parser.parse());
            parser.printTree(tree.get(), std::cout);
            parser.printTree(tree.get(), fullOut);

            fullOut << "\n--- ABSTRACT SYNTAX TREE ---\n";
            std::cout << "\n--- ABSTRACT SYNTAX TREE ---\n";
            ASTBuilder astBuilder;
            ASTPtr ast = astBuilder.build(tree.get());
            if (ast) {
                ast->print(std::cout);
                ast->print(fullOut);
            }

            fullOut << "\n--- DECORATED AST ---\n";
            std::cout << "\n--- DECORATED AST ---\n";
            SemanticAnalyzer semanalyzer;
            ASTPtr decoratedAst = semanalyzer.analyze(tree.get());
            if (decoratedAst) {
                decoratedAst->print(std::cout);
                decoratedAst->print(fullOut);
            }

            // Symbol tables ke stdout saja (tidak ada ostream param)
            semanalyzer.symtab.printTab();
            semanalyzer.symtab.printBtab();
            semanalyzer.symtab.printAtab();

            return 0;
        }

        /* Mode lama */
        if (argc >= 4 && !fullMode) {
            std::ofstream tokenOut(argv[2]);
            if (!tokenOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output token: "
                          << argv[2] << std::endl;
                return 1;
            }

            writeTokens(tokens, lexer, tokenOut);
            std::cerr << "Token output ditulis ke: " << argv[2] << std::endl;
        }

        /* Syntax Analysis  */
        std::cout << "\n--- PARSE TREE ---\n";

        Parser parser(tokens);

        std::unique_ptr<ParseNode> tree(parser.parse());

        /* Cetak parse tree ke terminal */
        parser.printTree(tree.get(), std::cout);

        /* Mode 2 args: tulis parse tree ke file output */
        if (argc == 3 && !fullMode) {
            std::ofstream treeOut(argv[2]);
            if (!treeOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output parse tree: "
                          << argv[2] << std::endl;
                return 1;
            }

            parser.printTree(tree.get(), treeOut);
            std::cerr << "Parse tree output ditulis ke: " << argv[2] << std::endl;
        }
        /* Mode 3 args: tulis parse tree ke file output */
        else if (argc >= 4 && !fullMode) {
            std::ofstream treeOut(argv[3]);
            if (!treeOut.is_open()) {
                std::cerr << "Error: Tidak bisa membuka file output parse tree: "
                          << argv[3] << std::endl;
                return 1;
            }

            parser.printTree(tree.get(), treeOut);
            std::cerr << "Parse tree output ditulis ke: " << argv[3] << std::endl;
        }

        /* AST Construction / Syntax-Directed Translation  */
        std::cout << "\n--- ABSTRACT SYNTAX TREE ---\n";

        ASTBuilder astBuilder;
        ASTPtr ast = astBuilder.build(tree.get());

        if (ast) {
            ast->print(std::cout);
        } else {
            std::cerr << "Warning: AST tidak berhasil dibentuk.\n";
        }

        /* Semantic Analysis (Decorated AST + Symbol Tables)  */
        std::cout << "\n--- DECORATED AST ---\n";

        SemanticAnalyzer semanalyzer;
        ASTPtr decoratedAst = semanalyzer.analyze(tree.get());

        if (decoratedAst) {
            decoratedAst->print(std::cout);
        } else {
            std::cerr << "Warning: Decorated AST tidak berhasil dibentuk.\n";
        }

        semanalyzer.symtab.printTab();
        semanalyzer.symtab.printBtab();
        semanalyzer.symtab.printAtab();

    } catch (const ParseError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
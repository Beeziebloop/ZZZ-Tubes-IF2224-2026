#include "lex_analyzer.hpp"
#include "parser.hpp"
#include "ast_builder.hpp"
#include "semantic_analyzer.hpp"
#include "code_generator.hpp"
#include "interpreter.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <string>

static void writeTokens(const std::vector<Token>& tokens,
                        const Lexer& lexer,
                        std::ostream& out)
{
    for (const Token& tok : tokens) {
        if (tok.type == EOF_TOKEN) break;
        if (tok.type == BLANK_LINE) { out << "\n"; continue; }
        out << lexer.tokenToString(tok) << "\n";
    }
}

static std::string getFlag(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--m1" || arg == "--m2" || arg == "--m3" || arg == "--m4" || arg == "--full")
            return arg;
    }
    return "--m3";
}

static std::string getOutputFile(int argc, char* argv[])
{
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == '-') continue;
        return arg;
    }
    return "";
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.txt> [output.txt] [--m1|--m2|--m3|--m4|--full]\n"
                  << "  --m1   : tokens only\n"
                  << "  --m2   : tokens + parse tree\n"
                  << "  --m3   : tokens + parse tree + decorated AST + symbol tables\n"
                  << "  --m4   : --m3 + intermediate code (TAC) + interpreter output\n"
                  << "  --full : sama dengan --m3\n";
        return 1;
    }

    std::string flag      = getFlag(argc, argv);
    std::string outputFile = getOutputFile(argc, argv);

    bool doM2   = (flag == "--m2" || flag == "--m3" || flag == "--m4" || flag == "--full");
    bool doM3   = (flag == "--m3" || flag == "--m4" || flag == "--full");
    bool doM4   = (flag == "--m4");

    // Buka output file jika ada
    std::ofstream fileOut;
    if (!outputFile.empty()) {
        fileOut.open(outputFile);
        if (!fileOut.is_open()) {
            std::cerr << "Error: Tidak bisa membuka file output: " << outputFile << "\n";
            return 1;
        }
    }

    // tulis ke cout dan file sekaligus
    auto writeToAll = [&](auto fn) {
        fn(std::cout);
        if (fileOut.is_open()) fn(fileOut);
    };
    // tulis ke cout saja (tidak ke file)
    auto writeToConsole = [&](auto fn) {
        fn(std::cout);
    };

    try {
        /* ── M1: Lexical Analysis ── */
        Lexer lexer(argv[1]);
        std::vector<Token> tokens = lexer.tokenize();

        // token selalu ke console; ke file hanya jika --m1 atau --m2
        if (doM3) {
            writeToConsole([&](std::ostream& out) { writeTokens(tokens, lexer, out); });
        } else {
            writeToAll([&](std::ostream& out) { writeTokens(tokens, lexer, out); });
        }

        if (!doM2) return 0;

        /* ── M2: Parse Tree ── */
        std::cout << "\n--- PARSE TREE ---\n";
        Parser parser(tokens);
        std::unique_ptr<ParseNode> tree(parser.parse());

        // parse tree ke console; ke file hanya jika --m2 (bukan --m3)
        if (doM3) {
            writeToConsole([&](std::ostream& out) { parser.printTree(tree.get(), out); });
        } else {
            writeToAll([&](std::ostream& out) { parser.printTree(tree.get(), out); });
        }

        if (!doM3) return 0;

        /* ── M3: AST + Decorated AST + Symbol Tables ── */
        if (fileOut.is_open()) fileOut << "\n--- ABSTRACT SYNTAX TREE ---\n";
        std::cout << "\n--- ABSTRACT SYNTAX TREE ---\n";
        ASTBuilder astBuilder;
        ASTPtr ast = astBuilder.build(tree.get());
        if (ast) {
            writeToAll([&](std::ostream& out) { ast->print(out); });
        } else {
            std::cerr << "Warning: AST tidak berhasil dibentuk.\n";
        }

        if (fileOut.is_open()) fileOut << "\n--- DECORATED AST ---\n";
        std::cout << "\n--- DECORATED AST ---\n";
        SemanticAnalyzer semanalyzer;
        ASTPtr decoratedAst = semanalyzer.analyze(tree.get());
        if (decoratedAst) {
            writeToAll([&](std::ostream& out) { decoratedAst->print(out); });
        } else {
            std::cerr << "Warning: Decorated AST tidak berhasil dibentuk.\n";
        }

        writeToAll([&](std::ostream& out) { semanalyzer.symtab.printTab(out); });
        writeToAll([&](std::ostream& out) { semanalyzer.symtab.printBtab(out); });
        writeToAll([&](std::ostream& out) { semanalyzer.symtab.printAtab(out); });

        // Print error list di akhir (ke stdout + file)
        if (semanalyzer.hasErrors()) {
            writeToAll([&](std::ostream& out) { semanalyzer.printErrors(out); });
            return 1;
        }

        if (!doM4) {
            return 0; // --m3: selesai di sini
        }

        /* ── M4: Intermediate Code Generation ── */
        CodeGenerator codegen(semanalyzer.symtab);
        Code code = codegen.generate(decoratedAst.get());

        codegen.printCode(code, std::cout);
        if (fileOut.is_open()) codegen.printCode(code, fileOut);

        /* ── M4: Stack Machine Interpreter ── */
        std::cout << "\n--- PROGRAM OUTPUT ---\n";
        Interpreter interpreter(code, codegen.stringPool());
        interpreter.run(std::cout, std::cerr);
        if (!interpreter.lastError().empty())
            return 1;

    } catch (const ParseError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

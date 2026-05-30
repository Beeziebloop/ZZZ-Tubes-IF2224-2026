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
        if (arg == "--m1" || arg == "--m2" || arg == "--m3" || arg == "--m4" ||
            arg == "--full")
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

// Jalankan Code Generator + Interpreter dari decorated AST + symbol table,
// menulis TAC + program output ke stdout (dan file jika terbuka).
// Mengembalikan exit code (0 = sukses, 1 = runtime error).
static int runM4(ASTNode* decoratedAst, const SymbolTable& symtab,
                 std::ostream* fileOut)
{
    CodeGenerator codegen(symtab);
    Code code = codegen.generate(decoratedAst);

    codegen.printCode(code, std::cout);
    if (fileOut) codegen.printCode(code, *fileOut);

    std::cout << "\n--- PROGRAM OUTPUT ---\n";
    if (fileOut) *fileOut << "\n--- PROGRAM OUTPUT ---\n";

    if (fileOut) {
        Interpreter interpFile(code, codegen.stringPool());
        interpFile.run(*fileOut, *fileOut);
    }
    Interpreter interpreter(code, codegen.stringPool());
    interpreter.run(std::cout, std::cerr);
    return interpreter.lastError().empty() ? 0 : 1;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.txt> [output.txt] [mode]\n"
                  << "  --m1   : tokens only\n"
                  << "  --m2   : tokens + parse tree\n"
                  << "  --m3   : tokens + parse tree + decorated AST + symbol tables\n"
                  << "  --m4   : intermediate code (TAC) + program output\n"
                  << "  --full : full pipeline ke satu output file\n";
        return 1;
    }

    std::string flag      = getFlag(argc, argv);
    std::string outputFile = getOutputFile(argc, argv);

    bool doM2   = (flag == "--m2" || flag == "--m3" || flag == "--full");
    bool doM3   = (flag == "--m3" || flag == "--full");
    bool doM4   = (flag == "--m4");
    // --m4 tetap perlu parse + semantic untuk generate code, tapi output file hanya TAC + hasil
    bool needPipeline = doM2 || doM3 || doM4;

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
        /*  M1: Lexical Analysis  */
        Lexer lexer(argv[1]);
        std::vector<Token> tokens = lexer.tokenize();

        // token ke console (selalu); ke file hanya untuk --m1/--m2
        if (doM4 || doM3) {
            writeToConsole([&](std::ostream& out) { writeTokens(tokens, lexer, out); });
        } else {
            writeToAll([&](std::ostream& out) { writeTokens(tokens, lexer, out); });
        }

        if (!needPipeline) return 0;

        /*  M2: Parse Tree  */
        std::cout << "\n--- PARSE TREE ---\n";
        Parser parser(tokens);
        std::unique_ptr<ParseNode> tree(parser.parse());

        // parse tree ke console saja untuk --m3/--m4; ke file untuk --m2
        if (doM3 || doM4) {
            writeToConsole([&](std::ostream& out) { parser.printTree(tree.get(), out); });
        } else {
            writeToAll([&](std::ostream& out) { parser.printTree(tree.get(), out); });
        }

        if (!doM3 && !doM4) return 0;

        /*  M3: AST + Decorated AST + Symbol Tables  */
        std::cout << "\n--- ABSTRACT SYNTAX TREE ---\n";
        ASTBuilder astBuilder;
        ASTPtr ast = astBuilder.build(tree.get());
        if (ast) {
            // AST ke file hanya untuk --m3, bukan --m4
            if (doM3) {
                writeToAll([&](std::ostream& out) { ast->print(out); });
            } else {
                writeToConsole([&](std::ostream& out) { ast->print(out); });
            }
        } else {
            std::cerr << "Warning: AST tidak berhasil dibentuk.\n";
        }

        std::cout << "\n--- DECORATED AST ---\n";
        SemanticAnalyzer semanalyzer;
        ASTPtr decoratedAst = semanalyzer.analyze(tree.get());
        if (decoratedAst) {
            if (doM3) {
                writeToAll([&](std::ostream& out) { decoratedAst->print(out); });
            } else {
                writeToConsole([&](std::ostream& out) { decoratedAst->print(out); });
            }
        } else {
            std::cerr << "Warning: Decorated AST tidak berhasil dibentuk.\n";
        }

        if (doM3) {
            writeToAll([&](std::ostream& out) { semanalyzer.symtab.printTab(out); });
            writeToAll([&](std::ostream& out) { semanalyzer.symtab.printBtab(out); });
            writeToAll([&](std::ostream& out) { semanalyzer.symtab.printAtab(out); });
        } else {
            writeToConsole([&](std::ostream& out) { semanalyzer.symtab.printTab(out); });
            writeToConsole([&](std::ostream& out) { semanalyzer.symtab.printBtab(out); });
            writeToConsole([&](std::ostream& out) { semanalyzer.symtab.printAtab(out); });
        }

        // Error semantik ke stdout + file
        if (semanalyzer.hasErrors()) {
            writeToAll([&](std::ostream& out) { semanalyzer.printErrors(out); });
            return 1;
        }

        if (!doM4) {
            return 0; // --m3 selesai
        }

        /*  M4: Intermediate Code Generation + Interpreter  */
        return runM4(decoratedAst.get(), semanalyzer.symtab,
                     fileOut.is_open() ? &fileOut : nullptr);

    } catch (const ParseError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

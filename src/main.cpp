#include "lex_analyzer.hpp"
#include "parser.hpp"
#include "ast_builder.hpp"
#include "semantic_analyzer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

using namespace std;

void writeTokens(const vector<Token>& tokens,
                 const Lexer& lexer,
                 ostream& out)
{
    for (const Token& tok : tokens) {
        if (tok.type == EOF_TOKEN)
            break;

        if (tok.type == BLANK_LINE) {
            out << "\n";
            continue;
        }

        out << lexer.tokenToString(tok) << "\n";
    }
}

void writeFullOutput(const vector<Token>& tokens,
                     const Lexer& lexer,
                     ParseNode* tree,
                     Parser& parser,
                     ostream& out)
{
    out << "--- TOKENS ---\n";
    writeTokens(tokens, lexer, out);

    out << "\n--- PARSE TREE ---\n";
    parser.printTree(tree, out);

    out << "\n--- ABSTRACT SYNTAX TREE ---\n";
    ASTBuilder astBuilder;
    ASTPtr ast = astBuilder.build(tree);
    if (ast)
        ast->print(out);
    else
        out << "AST gagal dibentuk.\n";

    out << "\n--- DECORATED AST ---\n";
    SemanticAnalyzer semanalyzer;
    ASTPtr decoratedAst = semanalyzer.analyze(tree);
    if (decoratedAst)
        decoratedAst->print(out);
    else
        out << "Decorated AST gagal dibentuk.\n";

    semanalyzer.symtab.printTab();
    semanalyzer.symtab.printBtab();
    semanalyzer.symtab.printAtab();
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <input.txt> [output.txt] "
             << "[--m1 | --m2 | --m3 | --full]"
             << endl;
        return 1;
    }

    bool fullMode = false;
    bool m1Mode = false;
    bool m2Mode = false;
    bool m3Mode = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--full")
            fullMode = true;
        if (arg == "--m1")
            m1Mode = true;
        if (arg == "--m2")
            m2Mode = true;
        if (arg == "--m3")
            m3Mode = true;
    }

    try {
        Lexer lexer(argv[1]);
        vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        unique_ptr<ParseNode> tree(parser.parse());

        // Terminal selalu full output dari M1 sampai M3.
        writeFullOutput(tokens, lexer, tree.get(), parser, cout);

        if (m1Mode && argc >= 3) {
            ofstream out(argv[2]);
            if (!out.is_open()) {
                cerr << "Error membuka file output: " << argv[2] << endl;
                return 1;
            }

            writeTokens(tokens, lexer, out);
            return 0;
        }

        if (m2Mode && argc >= 3) {
            ofstream out(argv[2]);
            if (!out.is_open()) {
                cerr << "Error membuka file output: " << argv[2] << endl;
                return 1;
            }

            parser.printTree(tree.get(), out);
            return 0;
        }

        if (m3Mode && argc >= 3) {
            ofstream out(argv[2]);
            if (!out.is_open()) {
                cerr << "Error membuka file output: " << argv[2] << endl;
                return 1;
            }

            out << "--- ABSTRACT SYNTAX TREE ---\n";

            ASTBuilder astBuilder;
            ASTPtr ast = astBuilder.build(tree.get());

            if (ast)
                ast->print(out);
            else
                out << "AST gagal dibentuk.\n";

            out << "\n--- DECORATED AST ---\n";

            SemanticAnalyzer semanalyzer;
            ASTPtr decoratedAst = semanalyzer.analyze(tree.get());

            if (decoratedAst)
                decoratedAst->print(out);
            else
                out << "Decorated AST gagal dibentuk.\n";

            out << "\n";

            streambuf* oldBuffer = cout.rdbuf();
            cout.rdbuf(out.rdbuf());

            semanalyzer.symtab.printTab();

            cout.rdbuf(oldBuffer);

            return 0;
        }

        if (fullMode && argc >= 3) {
            ofstream out(argv[2]);
            if (!out.is_open()) {
                cerr << "Error membuka file output: " << argv[2] << endl;
                return 1;
            }

            writeFullOutput(tokens, lexer, tree.get(), parser, out);
            return 0;
        }
    }

    catch (const ParseError& e) {
        cerr << e.what() << endl;
        return 1;
    }

    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
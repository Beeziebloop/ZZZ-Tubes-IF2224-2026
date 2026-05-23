#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include <map>
#include <string>
#include <vector>
#include <ostream>

#include "ast.hpp"
#include "symbol_table.hpp"

// Satu instruksi Three-Address Code (TAC) untuk stack machine PL/0-style.
// Format: op  level  val
//   LIT  0  n    -- push konstanta n
//   LOD  l  a    -- load var di level l, offset a
//   STO  l  a    -- store ke var di level l, offset a
//   OPR  0  k    -- operasi aritmetik/relasi/io (lihat OprCode)
//   INT  0  n    -- allocate n slot di stack frame
//   JMP  0  a    -- unconditional jump ke alamat a
//   JPC  0  a    -- jump ke a jika top of stack == 0 (false)
//   CAL  l  a    -- call subprogram di level l, entry point a
//   RET  0  0    -- return dari subprogram
struct Instruction {
    std::string op;
    int level;
    int val;
};

using Code = std::vector<Instruction>;

//  operasi untuk instruksi OPR.
// Interpreter harus handle semua nilai ini di fetch-decode-execute loop.
enum OprCode {
    OPR_RET   = 0,  // halt / return dari subprogram
    OPR_NEG   = 1,  // unary minus: top = -top
    OPR_ADD   = 2,  // top = second + top
    OPR_SUB   = 3,  // top = second - top
    OPR_MUL   = 4,  // top = second * top
    OPR_DIV   = 5,  // top = second / top (integer division)
    OPR_MOD   = 6,  // top = second mod top
    OPR_EQL   = 7,  // == → push 1 jika sama, 0 jika tidak
    OPR_NEQ   = 8,  // <>
    OPR_LSS   = 9,  // <
    OPR_GEQ   = 10, // >=
    OPR_GTR   = 11, // >
    OPR_LEQ   = 12, // <=
    OPR_READ  = 13, // baca integer dari stdin, push ke stack; lalu instruksi STO menyimpannya
    OPR_WRT   = 14, // cetak top of stack (tanpa newline); pop
    OPR_WRTLN = 15  // cetak newline
};

class CodeGenerator {
public:
    explicit CodeGenerator(const SymbolTable& symtab);

    // Entry point: generate TAC dari decorated AST.
    // Mengembalikan vektor instruksi yang siap dieksekusi interpreter.
    Code generate(ASTNode* root);

    // Print instruksi ke stream dalam format: index  OP  level  val
    void printCode(const Code& code, std::ostream& out) const;

private:
    const SymbolTable& symtab_;
    Code code_;
    int currentLevel_; // lexical level saat ini (0 = global)

    // Emit satu instruksi, return indeks instruksi tersebut
    int emit(const std::string& op, int level, int val);

    // Patch val dari instruksi di indeks `addr`
    void patch(int addr, int val);

    // Visitor per jenis node
    void genProgram(ProgramNode* node);
    void genBlock(BlockNode* node);

    // Declarations (hanya prosedur/fungsi yang perlu kode)
    void genDeclarations(const std::vector<ASTPtr>& decls);
    void genProcDecl(ProcedureDeclNode* node);
    void genFuncDecl(FunctionDeclNode* node);

    // Statements
    void genStatement(ASTNode* node);
    void genAssign(AssignNode* node);
    void genProcedureCall(ProcedureCallNode* node);
    void genIf(IfNode* node);
    void genWhile(WhileNode* node);
    void genRepeat(RepeatNode* node);
    void genFor(ForNode* node);
    void genCase(CaseNode* node);

    // Expressions – push hasil ke stack
    void genExpr(ASTNode* node);
    void genBinOp(BinOpNode* node);
    void genUnaryOp(UnaryOpNode* node);
    void genVar(VarNode* node);
    void genArrayAccess(ArrayAccessNode* node);

    // Load alamat (untuk STO)
    void genLValue(ASTNode* node);

    // Emit LIT untuk NumNode
    void genNum(NumNode* node);

    // Hitung level difference untuk LOD/STO/CAL
    int levelDiff(int targetLevel) const;

    // Hitung offset variabel di stack frame berdasarkan urutan deklarasi
    int getVarOffset(int tabIdx) const;

    // tab_index → entry point address (diisi saat genProcDecl / genFuncDecl)
    std::map<int, int> entryPoints_;

    // CAL yang belum di-patch karena forward reference: tabIdx → list instruksi index
    std::map<int, std::vector<int>> forwardCalls_;

    // Pool string literal (untuk instruksi LIT string)
    std::vector<std::string> stringPool_;

public:
    // String literal pool: instruksi LIT untuk string literal menyimpan indeks ke sini,
    // bukan nilai integer. Interpreter harus akses pool ini saat OPR_WRT dengan string.
    const std::vector<std::string>& stringPool() const { return stringPool_; }
};

#endif

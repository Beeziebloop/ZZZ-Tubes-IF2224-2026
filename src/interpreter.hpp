#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "code_generator.hpp"

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

// Stack machine interpreter untuk TAC (Milestone 4 — Orang 2).
// Model memori: satu stack dengan frame berisi SL, DL, RA, lalu variabel lokal.
class Interpreter {
public:
    static constexpr int kFrameHeader = 3; // SL, DL, RA
    static constexpr int kMaxStack = 65536;
    static constexpr int kMaxFrames = 1000;

    Interpreter(const Code& code, const std::vector<std::string>& stringPool = {});

    // Jalankan program; keluaran writeln/write ke out, error ke err.
    void run(std::ostream& out = std::cout, std::ostream& err = std::cerr);

    bool halted() const { return halted_; }
    const std::string& lastError() const { return lastError_; }

private:
    const Code& code_;
    const std::vector<std::string>& stringPool_;

    std::vector<int> stack_;
    int sp_;
    int bp_;
    int pc_;
    bool halted_;
    std::string lastError_;
    int frameCount_;

    void reset();
    void halt();

    // Fetch-decode-execute satu instruksi; false jika mesin berhenti.
    bool step(std::ostream& out);

    int base(int level) const;
    int frameAddress(int level, int offset) const; // offset absolut dalam frame (0=SL, 3+=var)

    int pop();
    void push(int value);
    void checkStackRoom(int extra = 1);
    void checkFrameDepth();

    void execInt(int n);
    void execLit(int v);
    void execLod(int level, int offset);
    void execSto(int level, int offset);
    void execLoda(int level, int offset);
    void execStoa(int level, int offset);
    void execOpr(int op, std::ostream& out);
    void execJmp(int target);
    void execJpc(int target);
    void execCal(int level, int target);
    void execRet();

    void runtimeError(const std::string& msg);
};

#endif

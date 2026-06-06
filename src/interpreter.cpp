#include "interpreter.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

Interpreter::Interpreter(const Code& code, const std::vector<std::string>& stringPool)
    : code_(code), stringPool_(stringPool), sp_(-1), bp_(0), pc_(0),
      halted_(false), frameCount_(0) {
    stack_.assign(kMaxStack, 0);
}

void Interpreter::reset() {
    sp_ = -1;
    bp_ = 0;
    pc_ = 0;
    halted_ = false;
    lastError_.clear();
    frameCount_ = 0;
}

void Interpreter::halt() {
    halted_ = true;
}

void Interpreter::runtimeError(const std::string& msg) {
    lastError_ = msg;
    halted_ = true;
}

void Interpreter::checkStackRoom(int extra) {
    if (sp_ + extra >= kMaxStack - 1)
        runtimeError("Stack overflow");
}

void Interpreter::checkFrameDepth() {
    if (frameCount_ > kMaxFrames)
        runtimeError("Call stack overflow");
}

int Interpreter::pop() {
    if (sp_ < 0) {
        runtimeError("Stack underflow");
        return 0;
    }
    return stack_[sp_--];
}

void Interpreter::push(int value) {
    checkStackRoom(1);
    stack_[++sp_] = value;
}

int Interpreter::base(int level) const {
    int b = bp_;
    while (level > 0) {
        if (b < 0 || b >= kMaxStack) {
            return 0;
        }
        b = stack_[b]; // static link
        --level;
    }
    return b;
}

int Interpreter::frameAddress(int level, int offset) const {
    int addr = base(level) + offset;
    if (addr < 0 || addr >= kMaxStack)
        return -1;
    return addr;
}

void Interpreter::run(std::ostream& out, std::ostream& err) {
    reset();
    while (!halted_ && pc_ >= 0 && pc_ < static_cast<int>(code_.size())) {
        if (!step(out)) break;
    }
    if (!lastError_.empty())
        err << "Runtime error: " << lastError_ << "\n";
}

bool Interpreter::step(std::ostream& out) {
    if (halted_ || pc_ < 0 || pc_ >= static_cast<int>(code_.size()))
        return false;

    const Instruction& ins = code_[pc_];
    ++pc_;

    const std::string& op = ins.op;

    if (op == "INT") {
        execInt(ins.val);
    } else if (op == "LIT") {
        execLit(ins.val);
    } else if (op == "LOD") {
        execLod(ins.level, ins.val);
    } else if (op == "STO") {
        execSto(ins.level, ins.val);
    } else if (op == "LODA") {
        execLoda(ins.level, ins.val);
    } else if (op == "STOA") {
        execStoa(ins.level, ins.val);
    } else if (op == "OPR") {
        execOpr(ins.val, out);
    } else if (op == "JMP") {
        execJmp(ins.val);
    } else if (op == "JPC") {
        execJpc(ins.val);
    } else if (op == "CAL") {
        execCal(ins.level, ins.val);
    } else if (op == "RET") {
        execRet();
    } else {
        runtimeError("Unknown instruction: " + op);
    }

    return !halted_;
}

void Interpreter::execInt(int n) {
    if (n < 0) {
        runtimeError("Invalid INT size");
        return;
    }
    checkStackRoom(n);

    // Setelah CAL: salin argumen pemanggil (di bawah SL) ke slot variabel frame (bp+3..).
    if (frameCount_ > 1 && n > kFrameHeader) {
        int slotCount = n - kFrameHeader;
        for (int i = 0; i < slotCount; ++i)
            stack_[bp_ + kFrameHeader + i] = stack_[bp_ - slotCount + i];
    }

    sp_ += n;

    // Program utama: bp=0, belum ada CAL — inisialisasi link frame.
    if (frameCount_ == 0 && bp_ == 0) {
        stack_[0] = 0;
        stack_[1] = 0;
        stack_[2] = 0;
        ++frameCount_;
    }
}

void Interpreter::execLit(int v) {
    push(v);
}

void Interpreter::execLod(int level, int offset) {
    int addr = frameAddress(level, offset);
    if (addr < 0) {
        runtimeError("Out-of-bounds load");
        return;
    }
    push(stack_[addr]);
}

void Interpreter::execSto(int level, int offset) {
    int addr = frameAddress(level, offset);
    if (addr < 0) {
        runtimeError("Out-of-bounds store");
        return;
    }
    stack_[addr] = pop();
}

void Interpreter::execLoda(int level, int offset) {
    int index = pop();
    int addr = frameAddress(level, offset);
    if (addr < 0) {
        runtimeError("Out-of-bounds array address");
        return;
    }
    push(addr + index);
}

void Interpreter::execStoa(int level, int offset) {
    int index = pop();
    int value = pop();
    int addr = frameAddress(level, offset);
    if (addr < 0) {
        runtimeError("Out-of-bounds array store");
        return;
    }
    int target = addr + index;
    if (target < 0 || target >= kMaxStack) {
        runtimeError("Out-of-bounds array store");
        return;
    }
    stack_[target] = value;
}

void Interpreter::execOpr(int op, std::ostream& out) {
    switch (op) {
        case OPR_RET:
            execRet();
            return;
        case OPR_NEG: {
            int a = pop();
            push(-a);
            return;
        }
        case OPR_ADD: {
            int b = pop();
            int a = pop();
            push(a + b);
            return;
        }
        case OPR_SUB: {
            int b = pop();
            int a = pop();
            push(a - b);
            return;
        }
        case OPR_MUL: {
            int b = pop();
            int a = pop();
            push(a * b);
            return;
        }
        case OPR_DIV: {
            int b = pop();
            int a = pop();
            if (b == 0) {
                runtimeError("Division by zero");
                return;
            }
            push(a / b);
            return;
        }
        case OPR_MOD: {
            int b = pop();
            int a = pop();
            if (b == 0) {
                runtimeError("Division by zero");
                return;
            }
            push(a % b);
            return;
        }
        case OPR_EQL: {
            int b = pop();
            int a = pop();
            push(a == b ? 1 : 0);
            return;
        }
        case OPR_NEQ: {
            int b = pop();
            int a = pop();
            push(a != b ? 1 : 0);
            return;
        }
        case OPR_LSS: {
            int b = pop();
            int a = pop();
            push(a < b ? 1 : 0);
            return;
        }
        case OPR_GEQ: {
            int b = pop();
            int a = pop();
            push(a >= b ? 1 : 0);
            return;
        }
        case OPR_GTR: {
            int b = pop();
            int a = pop();
            push(a > b ? 1 : 0);
            return;
        }
        case OPR_LEQ: {
            int b = pop();
            int a = pop();
            push(a <= b ? 1 : 0);
            return;
        }
        case OPR_READ: {
            int v = 0;
            if (!(std::cin >> v)) {
                runtimeError("Failed to read input");
                return;
            }
            push(v);
            return;
        }
        case OPR_WRT: {
            int v = pop();
            if (v >= 0 && v < static_cast<int>(stringPool_.size())) {
                out << stringPool_[static_cast<size_t>(v)];
            } else {
                out << v;
            }
            return;
        }
        case OPR_WRTLN: {
            // writeln: arg sudah dicetak OPR_WRT; tanpa arg hanya newline.
            if (sp_ >= 0) {
                int v = pop();
                if (v >= 0 && v < static_cast<int>(stringPool_.size())) {
                    out << stringPool_[static_cast<size_t>(v)];
                } else {
                    out << v;
                }
            }
            out << "\n";
            return;
        }
        default:
            runtimeError("Unknown OPR code: " + std::to_string(op));
            return;
    }
}

void Interpreter::execJmp(int target) {
    if (target < 0 || target >= static_cast<int>(code_.size())) {
        runtimeError("Invalid jump target");
        return;
    }
    pc_ = target;
}

void Interpreter::execJpc(int target) {
    int cond = pop();
    if (cond == 0) {
        if (target < 0 || target >= static_cast<int>(code_.size())) {
            runtimeError("Invalid jump target");
            return;
        }
        pc_ = target;
    }
}

void Interpreter::execCal(int level, int target) {
    if (target < 0 || target >= static_cast<int>(code_.size())) {
        runtimeError("Invalid call target");
        return;
    }
    checkStackRoom(3);
    checkFrameDepth();

    // Susun SL, DL, RA tepat di atas operand argumen yang sudah di-push.
    stack_[sp_ + 1] = base(level);
    stack_[sp_ + 2] = bp_;
    stack_[sp_ + 3] = pc_;
    bp_ = sp_ + 1;
    pc_ = target;
    ++frameCount_;
}

void Interpreter::execRet() {
    if (bp_ == 0 && frameCount_ <= 1) {
        halt();
        return;
    }

    if (bp_ + 2 >= kMaxStack) {
        runtimeError("Corrupted stack frame");
        return;
    }

    // Ambil nilai kembalian dari slot offset 3 sebelum frame di-pop.
    int retVal = stack_[bp_ + kFrameHeader];
    int newPc = stack_[bp_ + 2];
    int newBp = stack_[bp_ + 1];

    sp_ = bp_ - 1;
    pc_ = newPc;
    bp_ = newBp;
    push(retVal);

    if (frameCount_ > 0)
        --frameCount_;
}

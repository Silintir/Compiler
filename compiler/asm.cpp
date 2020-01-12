#include "asm.hpp"

const ASM ASM::GET    = {"GET", 100};
const ASM ASM::PUT    = {"PUT", 100};

const ASM ASM::LOAD   = {"LOAD", 10};
const ASM ASM::STORE  = {"STORE", 10};
const ASM ASM::LOADI  = {"LOADI", 20};
const ASM ASM::STOREI = {"STOREI", 20};

const ASM ASM::ADD    = {"ADD", 10};
const ASM ASM::SUB    = {"SUB", 10};
const ASM ASM::SHIFT  = {"SHIFT", 5};
const ASM ASM::INC    = {"INC", 1};
const ASM ASM::DEC    = {"DEC", 1};

const ASM ASM::JUMP   = {"JUMP", 1};
const ASM ASM::JPOS   = {"JPOS", 1};
const ASM ASM::JZERO  = {"JZERO", 1};
const ASM ASM::JNEG   = {"JNEG", 1};


const ASM ASM::HALT   = {"HALT", 0};

std::ostream & operator<<(std::ostream &stream, const Instruction &instruction) {
    stream << instruction.command.name;
    if(instruction.arg != Instruction::Undef) {
        stream << " " << instruction.arg;
    }
    return stream << "\n";
}
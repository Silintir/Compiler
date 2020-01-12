#ifndef ASSEMBLY_H
#define ASSEMBLY_H 1

#include <string>
#include <ostream>
#include <vector>

struct ASM {
    std::string name;
    int64_t time;
    ASM(std::string name, int64_t time) : name(name), time(time) {}

    static const ASM GET, PUT, LOAD, STORE, LOADI, STOREI,
                    ADD, SUB, SHIFT, INC, DEC,  
                    JUMP, JPOS, JZERO, JNEG,
                    HALT;      
};

struct Instruction {
    private:
        Instruction(ASM command, int64_t label)
            : command(command), label(label) {}
        Instruction(ASM command, int64_t arg, int64_t label)
            : command(command), arg(arg), label(label) {}
    public:
        static const int64_t Undef = -1;

        ASM command;
        int64_t arg, label;

    static void GET(std::vector<Instruction*>& out, int64_t* label) {
            out.push_back(new Instruction(ASM::GET, (*label)++));
    }

    static void PUT(std::vector<Instruction*>& out, int64_t* label) {
            out.push_back(new Instruction(ASM::PUT, (*label)++));
    }

    static void LOAD(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::LOAD, arg, (*label)++));
    }

    static void STORE(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::STORE, arg, (*label)++));
    }

    static void LOADI(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::LOADI, arg, (*label)++));
    }

    static void STOREI(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::STOREI, arg, (*label)++));
    }
    
    static void ADD(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::ADD, arg, (*label)++));
    }

    static void SUB(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::SUB, arg, (*label)++));
    }

    static void SHIFT(std::vector<Instruction*>& out, int64_t arg, int64_t* label) {
            out.push_back(new Instruction(ASM::SHIFT, arg, (*label)++));
    }

    static void INC(std::vector<Instruction*>& out, int64_t* label) {
             out.push_back(new Instruction(ASM::INC, (*label)++));
    }

    static void DEC(std::vector<Instruction*>& out, int64_t* label) {
            out.push_back(new Instruction(ASM::DEC, (*label)++));   
    }

    static void HALT(std::vector<Instruction*>& out, int64_t* label) {
            out.push_back(new Instruction(ASM::HALT, (*label)++));   
    }    

};

std::ostream & operator<<(std::ostream &stream, const Instruction &instruction);
#endif
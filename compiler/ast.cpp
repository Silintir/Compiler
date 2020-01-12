
#include <algorithm>
#include <iostream>
#include <sstream>
#include "ast.hpp"
#include "symbols.hpp"
#include "code_gen.hpp"
#include "asm.hpp"


extern Symbols symbols;
extern CodeGen generator;
extern std::vector<int64_t> memory; 

namespace ast {
    std::vector<Instruction*> generate_number(int64_t number, int64_t *label);
    
    void insert_back(std::vector<Instruction*>& target, std::vector<Instruction*>& stuff);
    void insert_back(std::vector<Instruction*>& target, const std::vector<Instruction*>& stuff);

    bool check_init(Identifier *identifier);
    bool check_init(Value *value);

    std::vector<Instruction*> load_value(Value *value, int64_t *label) {
        if (value->is_const()) {
            return value->gen_ir(label);
        }
        auto out = value->gen_ir(label);
        Instruction::LOAD(out, value->value, label);
        return out;
    }

    void err_redeclaration(std::string tag, Identifier *identifier) {
        std::ostringstream os;
         os  << "[" << tag << "] Duplicate declaration of " << identifier->name
            << ": first declared in line "
            << symbols.get_symbol(identifier->name).line;
        generator.report(os, identifier->line);
    }

    std::vector <Instruction*> Program::gen_ir(int64_t *cur_label) {
        if (declarations) {
        declarations->gen_ir(cur_label);
        }
        std::vector<Instruction*> out = code->gen_ir(cur_label);
        Instruction::HALT(out, cur_label);
        return out;
    }

    std::vector <Instruction*> Declarations::gen_ir(int64_t *cur_label) {
        symbols.declare_tmp("_TMP");

        std::vector<Identifier*> vars;
        std::vector<Identifier*> iters;
        std::vector<Identifier*> arrays;

        for(Identifier *identifier : identifiers) {
            if (identifier->array) {
                arrays.push_back(identifier);
            } else if (identifier->iter) {
                iters.push_back(identifier);
            } else {
                vars.push_back(identifier);
            }
        }

        for (Identifier *identifier : vars) {
            if (!symbols.declare(identifier)) {
                err_redeclaration("DECL", identifier);
            }
        }
        symbols.alloc_for_control(for_counter);
        for (Identifier *identifier : arrays) {
            if (!symbols.declare(identifier)) {
                err_redeclaration("DECL", identifier);
            }
            symbols.set_array(identifier);
        }
        return std::vector<Instruction*>();
    }

    void Declarations::declare(Identifier *identifier){
        identifiers.push_back(identifier);
    }

    std::vector<Instruction*> Commands::gen_ir(int64_t *cur_label) {

        std::vector<Instruction*> output;
        for (Command *cmd : commands) {
            std::vector<Instruction*> cmd_out = cmd->gen_ir(cur_label);
            output.insert(output.end(), cmd_out.begin(), cmd_out.end());
        }
        return output;
    }

    void Commands::add_command(Command *cmd) {
        commands.push_back(cmd);
    }

    std::vector<Instruction*> Value::gen_ir(int64_t *cur_label){
        identifier->gen_ir(cur_label);
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Assign::gen_ir(int64_t *cur_label) {
        std::cout << "Assign" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> If::gen_ir(int64_t *cur_label) {
        std::cout << "If" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> While::gen_ir(int64_t *cur_label) {
        std::cout << "While" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> For::gen_ir(int64_t *cur_label) {
        std::cout << "For" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Read::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        std::cout << *cur_label << std::endl;
        if (!symbols.set_initialized(identifier)) {
            error(identifier->name+" not defined in line: ", line);
            return output;
        } 
        insert_back(output, identifier->gen_ir(cur_label));
        Instruction::GET(output,cur_label);
        Instruction::STORE(output, symbols.get_symbol(identifier->name).offset_id, cur_label);
        return output;
    }

    std::vector<Instruction*> Write::gen_ir(int64_t *cur_label) {
        std::cout << "Write" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Const::gen_ir(int64_t *cur_label) {
        std::cout << "Const" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Plus::gen_ir(int64_t *cur_label) {
        std::cout << "Plus" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Minus::gen_ir(int64_t *cur_label) {
        std::cout << "Minus" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Times::gen_ir(int64_t *cur_label) {
        std::cout << "Times" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Div::gen_ir(int64_t *cur_label) {
        std::cout << "Div" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Mod::gen_ir(int64_t *cur_label) {
        std::cout << "Mod" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> EQ::gen_ir(int64_t *cur_label) {
        std::cout << "EQ" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> NEQ::gen_ir(int64_t *cur_label) {
        std::cout << "NEQ" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> LE::gen_ir(int64_t *cur_label) {
        std::cout << "LE" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> GE::gen_ir(int64_t *cur_label) {
        std::cout << "GE" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> LEQ::gen_ir(int64_t *cur_label) {
        std::cout << "LEQ" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> GEQ::gen_ir(int64_t *cur_label) {
        std::cout << "GEQ" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> Var::gen_ir(int64_t *cur_label) {
        std::cout << "Var" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> ConstArray::gen_ir(int64_t *cur_label) {
        std::cout << "ConstArray" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    std::vector<Instruction*> VarArray::gen_ir(int64_t *cur_label) {
        std::cout << "VarArray" << std::endl;
        std::vector<Instruction*> a;
        return a;
    }

    void insert_back(std::vector<Instruction*>& target, std::vector<Instruction*>& stuff) {
        target.insert(target.end(), stuff.begin(), stuff.end());
    }

    void insert_back(std::vector<Instruction*>& target, const std::vector<Instruction*>& stuff) {
        target.insert(target.end(), stuff.begin(), stuff.end());
    }
}


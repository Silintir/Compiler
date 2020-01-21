
#include <algorithm>
#include <iostream>
#include <sstream>
#include <typeinfo>
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
        std::vector<Instruction*> out;
        
         if (value->is_const()) {
             return value->gen_ir(label);

        } else if(symbols.get_symbol(value->identifier->name).is_array){
            value->identifier->gen_ir(label);
            if (typeid(*value->identifier) == typeid(ast::ConstArray)) {
                int64_t off = symbols.get_symbol(value->identifier->name).offset_id; 
                auto idx = (((ast::ConstArray*)value->identifier)->idx+2 - symbols.get_symbol(value->identifier->name).idx_b);
                Instruction::LOAD(out,off+idx, label); 
            } else if (typeid(*value->identifier) == typeid(ast::VarArray)) {
                Instruction::LOAD(out, symbols.get_symbol(value->identifier->name).offset_id, label);
                Instruction::SUB(out,  symbols.get_symbol(value->identifier->name).offset_id+1, label);
                Instruction::INC(out, label);
                Instruction::INC(out, label);
                Instruction::ADD(out, symbols.get_symbol(((ast::VarArray*)value->identifier)->index->name).offset_id, label);
                Instruction::LOADI(out, 0, label);
            }
        } else {
            value->identifier->gen_ir(label);
            out = value->gen_ir(label);
            Instruction::LOAD(out, symbols.get_symbol(value->identifier->name).offset_id, label);
        }
        
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
        std::vector<Instruction*> out;
        if (declarations) {
            out = declarations->gen_ir(cur_label);
        }
        insert_back(out, code->gen_ir(cur_label));
        Instruction::HALT(out, cur_label);
        return out;
    }

    std::vector <Instruction*> Declarations::gen_ir(int64_t *cur_label) {
        symbols.declare_tmp("_TMP");
        std::vector<Instruction*> out;
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
            if (((ast::ConstArray*)identifier)->index_b > ((ast::ConstArray*)identifier)->index_e) {
                std::ostringstream os;
                os << "Wrong range of table: " << identifier->name << std::endl;
                generator.report(os, identifier->line);
            }
            symbols.set_array(identifier);
            // W zerowym miejscu arraya znajduje sie jego offset
            //std::cout << symbols.get_symbol(identifier->name).offset_id << std::endl;
            insert_back(out, generate_number(symbols.get_symbol(identifier->name).offset_id, cur_label));
            Instruction::STORE(out, symbols.get_symbol(identifier->name).offset_id, cur_label);
            insert_back(out, generate_number(symbols.get_symbol(identifier->name).idx_b, cur_label));
            Instruction::STORE(out, symbols.get_symbol(identifier->name).offset_id+1, cur_label);
        }
        return out;
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
        std::vector<Instruction*> output;
        if (is_const()) {
            output = generate_number(value, cur_label);
        } else {
            output = identifier->gen_ir(cur_label);
        }
        return output;
    }

    std::vector<Instruction*> Assign::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!symbols.set_initialized(identifier)) {
              std::ostringstream os;
            os << "not defined: " << identifier->name;
            generator.report(os, line);
            return output;
        }
        if (symbols.is_iterator(identifier)) {
            std::ostringstream os;
            os << "Attempt to modify the iterator in a FOR loop: " << identifier->name;
            generator.report(os, line);
            return output;
        }
        identifier->gen_ir(cur_label);

        if(symbols.get_symbol(identifier->name).is_array){
            if (typeid(*identifier) == typeid(ast::ConstArray)) {
                output = identifier->gen_ir(cur_label);
                insert_back(output, expression->gen_ir(cur_label));
                int64_t off = symbols.get_symbol(identifier->name).offset_id;
                auto idx = (((ast::ConstArray*)identifier)->idx+2 - symbols.get_symbol(identifier->name).idx_b); 
                Instruction::STORE(output,off+idx, cur_label);
            } else if (typeid(*identifier) == typeid(ast::VarArray)) {
                Instruction::LOAD(output, symbols.get_symbol(identifier->name).offset_id, cur_label);
                Instruction::SUB(output,  symbols.get_symbol(identifier->name).offset_id+1, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::ADD(output, symbols.get_symbol(((ast::VarArray*)identifier)->index->name).offset_id, cur_label);
                Instruction::STORE(output, symbols.offset, cur_label);
                int64_t temp = symbols.offset;
                symbols.offset++;
                insert_back(output, expression->gen_ir(cur_label));
                Instruction::STOREI(output, temp, cur_label);
            }
        } else {
            insert_back(output, expression->gen_ir(cur_label));
            Instruction::STORE(output, symbols.get_symbol(identifier->name).offset_id, cur_label);
        }
        
        return output;
    }

    std::vector<Instruction*> If::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out = condition->gen_ir(cur_label);
        Instruction *jump_else = Instruction::JZERO(Instruction::Undef, cur_label);
        out.push_back(jump_else);
    
        auto branch_then = do_then->gen_ir(cur_label);
        out.insert(out.end(), branch_then.begin(), branch_then.end());
        
        if(do_else) {
            Instruction *jump_end = Instruction::JUMP(Instruction::Undef, cur_label);
            out.push_back(jump_end);
            auto branch_else = do_else->gen_ir(cur_label);
            out.insert(out.end(), branch_else.begin(), branch_else.end());
            jump_else->arg = branch_else[0]->label;
            jump_end->arg = *cur_label;
        } else {
            jump_else->arg = *cur_label;
        }

        return out;
    }
    // Pretty much done
    std::vector<Instruction*> While::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out;
        if (!reversed) {
            out = condition->gen_ir(cur_label);
            int64_t lbl = out[0]->label;
            Instruction *jump_end = Instruction::JZERO(Instruction::Undef, cur_label);
            out.push_back(jump_end);
            auto loop_body = body->gen_ir(cur_label);
            out.insert(out.end(), loop_body.begin(), loop_body.end());
            Instruction::JUMP(out, lbl, cur_label);
            jump_end->arg = *cur_label;
            
        } else {
            out = body->gen_ir(cur_label);
            int64_t lbl = out[0]->label;
            insert_back(out,condition->gen_ir(cur_label));
            Instruction::JZERO(out,lbl, cur_label);
        }
       return out;
    }

    std::vector<Instruction*> For::gen_ir(int64_t *cur_label) {
         std::vector<Instruction*> out;
         if (!symbols.declare_iterator(iterator)){
               std::ostringstream os;
            os  << "Duplicate declaration of " << iterator->name
                << ": first declared in line "
                << symbols.get_symbol(iterator->name).line;
            generator.report(os, iterator->line);
            return out;
         }   
        if (! (check_init(from) && check_init(to))) {
            return out;
        }

        symbols.set_initialized(iterator);
        symbols.set_iterator(iterator);

        if (from->is_const()) {
            insert_back(out, generate_number(from->value, cur_label));
        } else {
            insert_back(out, load_value(from, cur_label));
        }
        Instruction::STORE(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
        Symbol to_var;

        Identifier *_to = new Var("_TO_" + std::to_string(id), Identifier::N, line);
        if (!symbols.declare_iterator(_to)) {
            std::cerr << "CANNOT DECLARE _TO: LINE "<< line << std::endl;
            exit(EXIT_FAILURE);
        }
        to_var = symbols.get_symbol(_to->name);

        if (to->is_const()) {
            insert_back(out, generate_number(to->value, cur_label));
        } else {
            insert_back(out, load_value(to, cur_label));
        }
         Instruction::STORE(out, to_var.offset_id, cur_label);
        
        int64_t iteracje = symbols.offset; symbols.offset++;

        if (reversed) {
            Instruction::LOAD(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::SUB(out, to_var.offset_id, cur_label);
            Instruction::INC(out, cur_label);
            Instruction::STORE(out, iteracje, cur_label);

            int64_t start = *cur_label;
            Instruction::JZERO(out, *cur_label+4, cur_label);
            Instruction::DEC(out, cur_label);
            Instruction::STORE(out, iteracje, cur_label);

            Instruction::JUMP(out, *cur_label+2, cur_label);
            Instruction *jump_end = Instruction::JUMP(Instruction::Undef, cur_label);
            out.push_back(jump_end);

            insert_back(out, body->gen_ir(cur_label));

            Instruction::LOAD(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::DEC(out, cur_label);
            Instruction::STORE(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::LOAD(out, iteracje, cur_label);
            Instruction::JUMP(out, start, cur_label);

            jump_end->arg = *cur_label;
        } else {
            Instruction::LOAD(out, to_var.offset_id, cur_label);
            Instruction::SUB(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::INC(out, cur_label);
            Instruction::STORE(out, iteracje, cur_label);

            int64_t start = *cur_label;
            Instruction::JZERO(out, *cur_label+4, cur_label);
            Instruction::DEC(out, cur_label);
            Instruction::STORE(out, iteracje, cur_label);

            Instruction::JUMP(out, *cur_label+2, cur_label);
            Instruction *jump_end = Instruction::JUMP(Instruction::Undef, cur_label);
            out.push_back(jump_end);

            insert_back(out, body->gen_ir(cur_label));

            Instruction::LOAD(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::INC(out, cur_label);
            Instruction::STORE(out, symbols.get_symbol(iterator->name).offset_id, cur_label);
            Instruction::LOAD(out, iteracje, cur_label);
            Instruction::JUMP(out, start, cur_label);

            jump_end->arg = *cur_label;
        }

        symbols.undeclare_iter(_to->name);
        //std::cerr << "undeclaring " << _to->name << std::endl; 
        symbols.undeclare_iter(iterator->name);
        //std::cerr << "undeclaring " << iterator->name << std::endl;
        
        return out;
    }

    // DONE
    std::vector<Instruction*> Read::gen_ir(int64_t *cur_label) {
    
        std::vector<Instruction*> output;
        if (!symbols.set_initialized(identifier)) {
            error(identifier->name+" not defined in line: %d", line);
            return output;
        } 

        identifier->gen_ir(cur_label);
        if(symbols.get_symbol(identifier->name).is_array){
           
            if (typeid(*identifier) == typeid(ast::ConstArray)) {
                insert_back(output, identifier->gen_ir(cur_label));
                Instruction::GET(output,cur_label);
                int64_t off = symbols.get_symbol(identifier->name).offset_id; 
                auto idx = (((ast::ConstArray*)identifier)->idx+2 - symbols.get_symbol(identifier->name).idx_b);
                Instruction::STORE(output,off+idx, cur_label);
            } else if (typeid(*identifier) == typeid(ast::VarArray)) {
                Instruction::LOAD(output, symbols.get_symbol(identifier->name).offset_id, cur_label);
                Instruction::SUB(output,  symbols.get_symbol(identifier->name).offset_id+1, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::ADD(output, symbols.get_symbol(((ast::VarArray*)identifier)->index->name).offset_id, cur_label);
                Instruction::STORE(output, symbols.offset, cur_label);
                int64_t temp = symbols.offset;
                symbols.offset++;
                Instruction::GET(output,cur_label);
                Instruction::STOREI(output, temp, cur_label);
            }
        } else {
            insert_back(output, identifier->gen_ir(cur_label));
            Instruction::GET(output,cur_label);
            Instruction::STORE(output, symbols.get_symbol(identifier->name).offset_id, cur_label);
        }

       
        return output;
    }
    // DONE
    std::vector<Instruction*> Write::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!check_init(value)) {
            return output;
        }
        output = load_value(value, cur_label);
        Instruction::PUT(output, cur_label);
        return output;
    }
    // DONE
    std::vector<Instruction*> Const::gen_ir(int64_t *cur_label) {
        if (!check_init(left)) {
            return std::vector<Instruction*>();
        }
        return load_value(left, cur_label);
    }
    // DONE
    std::vector<Instruction*> Plus::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if(!(check_init(left) && check_init(right))) {
            return output;
        }

        if (left->is_const() && right->is_const()) {
            int64_t number = left->value + right->value;
            output = generate_number(number, cur_label);
        } else if (left->is_const() || right-> is_const()) {
            auto constant = left->is_const() ? left : right;
            auto variable = left->is_const() ? right : left;
            if(constant->value == 1) {
                insert_back(output, load_value(variable,cur_label));
                Instruction::INC(output, cur_label);
            } else {
                output = constant->gen_ir(cur_label);
                Instruction::STORE(output, symbols.offset, cur_label);
                int64_t const_mem_num = symbols.offset; symbols.offset++;
                insert_back(output, load_value(variable, cur_label));
                Instruction::ADD(output,const_mem_num, cur_label);
            }
        } else {
            insert_back(output, load_value(left, cur_label));
            int64_t addition = symbols.offset; symbols.offset++;
            Instruction::STORE(output, addition, cur_label);
            insert_back(output, load_value(right, cur_label));
            Instruction::ADD(output, addition ,cur_label);    
        }
    return output;
    }

    // DONE
    std::vector<Instruction*> Minus::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!(check_init(left) && check_init(right))) {
            return output;
        }

        if (left->is_const() && right->is_const()) {
            int64_t num = left->value - right->value;
            output = generate_number(num, cur_label);
        } else if (left->is_const()) {
            insert_back(output, load_value(right, cur_label));
            int64_t subtraction = symbols.offset; symbols.offset++;
            Instruction::STORE(output, subtraction, cur_label);
            insert_back(output, left->gen_ir(cur_label));
            Instruction::SUB(output, subtraction, cur_label);
        } else if (right->is_const()) {
            insert_back(output, right->gen_ir(cur_label));
            int64_t subtraction = symbols.offset; symbols.offset++;
            Instruction::STORE(output, subtraction, cur_label);
            insert_back(output, load_value(left, cur_label));
            Instruction::SUB(output, subtraction, cur_label);
        } else {
            insert_back(output, load_value(right, cur_label));
            int64_t subtaction = symbols.offset; symbols.offset++;
            Instruction::STORE(output, subtaction, cur_label);
            insert_back(output, load_value(left, cur_label));
            Instruction::SUB(output, subtaction ,cur_label);    
        }
        return output;
    }

    std::vector<Instruction*> Times::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!(check_init(left) && check_init(right))) {
            return output;
        }
        if (left->is_const() && right->is_const()) {
            int64_t num = left->value * right->value;
            return generate_number(num, cur_label);
        } else if (left->is_const() || right->is_const()) {
            Value *constant = left->is_const() ? left : right;
            Value *ref = left->is_const() ? right : left;
            if (constant->value == 0) {
                insert_back(output, generate_number(0, cur_label));
            } else if (constant->value == 1) {
                output = ref->gen_ir(cur_label);
                return output;
            } else {
                int64_t multiplier = 1;
                bool sign_const = constant->value > 0 ? true : false; 
                int64_t target = std::abs(constant->value);
                int64_t one = symbols.offset; symbols.offset++;
                int64_t single_value_offset = symbols.offset; symbols.offset++;
                int64_t ref_offest = symbols.offset; symbols.offset++;
                int64_t result = symbols.offset; symbols.offset++;
                int64_t sign_offset = symbols.offset; symbols.offset++;

                Instruction::SUB(output, 0, cur_label);
                Instruction::STORE(output, result, cur_label);
                Instruction::STORE(output, sign_offset, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::STORE(output, one, cur_label);

                insert_back(output, load_value(ref, cur_label));
                Instruction::JNEG(output, *cur_label+4, cur_label);
                Instruction *jump_to_end_if_zero = Instruction::JZERO(Instruction::Undef, cur_label); 
                output.push_back(jump_to_end_if_zero);
                Instruction::STORE(output, ref_offest, cur_label);
                Instruction::JUMP(output, *cur_label+8, cur_label);

                Instruction::STORE(output, ref_offest, cur_label);
                Instruction::SUB(output, 0, cur_label);
                Instruction::SUB(output, ref_offest, cur_label);
                Instruction::STORE(output, ref_offest, cur_label);
                Instruction::LOAD(output, sign_offset, cur_label);
                Instruction::DEC(output, cur_label);
                Instruction::STORE(output, sign_offset, cur_label);

                Instruction::LOAD(output, ref_offest, cur_label);
                Instruction::STORE(output, single_value_offset, cur_label);

                
                while (!(target == 1) && !(target == 0)) {

                    multiplier = 1;

                    while (multiplier * 2 < target) {
                        Instruction::SHIFT(output, one, cur_label);
                        multiplier*=2;
                    }
                    
                    target -= multiplier;
                    Instruction::ADD(output, result, cur_label);
                    Instruction::STORE(output, result, cur_label);
                    Instruction::LOAD(output, single_value_offset, cur_label);

                }

                if (target == 1) {
                    Instruction::LOAD(output, result, cur_label);
                    Instruction::ADD(output, single_value_offset, cur_label);
                    Instruction::STORE(output, result, cur_label);
                }
               
                //std::cout << sign_const << std::endl;
                if (sign_const) {
                    Instruction::LOAD(output, sign_offset, cur_label);
                    Instruction::JNEG(output, *cur_label+3, cur_label);
                    Instruction::LOAD(output, result, cur_label);
                    Instruction::JUMP(output, *cur_label+3, cur_label);
                    Instruction::SUB(output, 0, cur_label);
                    Instruction::SUB(output, result, cur_label);
                } else {
                    Instruction::LOAD(output, sign_offset, cur_label);
                    Instruction::JZERO(output, *cur_label+3, cur_label);
                    Instruction::LOAD(output, result, cur_label);
                    Instruction::JUMP(output, *cur_label+3, cur_label);
                    Instruction::SUB(output, 0, cur_label);
                    Instruction::SUB(output, result, cur_label);
                }
                jump_to_end_if_zero->arg = *cur_label;

                return output;
            }
        } else {
            int64_t left_temp = symbols.offset; symbols.offset++;
            int64_t left_temp2 = symbols.offset; symbols.offset++;
            int64_t right_temp = symbols.offset; symbols.offset++;
            int64_t sign = symbols.offset; symbols.offset++;
            int64_t multiplier = symbols.offset; symbols.offset++;
            int64_t one = symbols.offset; symbols.offset++;
            int64_t neg_one = symbols.offset; symbols.offset++;
            int64_t target = symbols.offset; symbols.offset++;
            int64_t result = symbols.offset; symbols.offset++;

            Instruction::SUB(output, 0, cur_label);
            Instruction::STORE(output, result, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::STORE(output, neg_one, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, one, cur_label);
            Instruction::STORE(output, multiplier, cur_label);

            // loading left to temp and setting sign
            insert_back(output, load_value(left, cur_label));
            Instruction *jump_1_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
            output.push_back(jump_1_if_0);

            Instruction::JPOS(output, *cur_label+10, cur_label);
            Instruction::STORE(output, left_temp, cur_label);
            Instruction::SUB(output, 0 ,cur_label);
            Instruction::SUB(output ,left_temp ,cur_label);
            Instruction::STORE(output, left_temp, cur_label);
            Instruction::STORE(output, left_temp2, cur_label);
            Instruction::SUB(output ,0 ,cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::STORE(output, sign, cur_label);
            Instruction::JUMP(output, *cur_label+3, cur_label);
            Instruction::STORE(output, left_temp, cur_label);
            Instruction::STORE(output, left_temp2, cur_label);

            // loading right and adjusting the sign
            insert_back(output, load_value(right, cur_label));
            Instruction *jump_2_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
            output.push_back(jump_2_if_0);

            Instruction::JPOS(output, *cur_label+13, cur_label);
            Instruction::STORE(output, right_temp, cur_label);
            Instruction::SUB(output ,0 ,cur_label);
            Instruction::SUB(output ,right_temp ,cur_label);
            Instruction::STORE(output, right_temp, cur_label);
            Instruction::STORE(output, target, cur_label);
            Instruction::LOAD(output, sign, cur_label);
            Instruction::JNEG(output, *cur_label+3, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::JUMP(output, *cur_label+2, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, sign, cur_label);
            Instruction::JUMP(output, *cur_label+3, cur_label);
            Instruction::STORE(output, right_temp, cur_label);
            Instruction::STORE(output, target, cur_label);

            // while (!(target == 1) && !(target == 0)) // external loop
            int64_t begin_loop = *cur_label;
            Instruction::LOAD(output, target, cur_label);
            Instruction *jump1 = Instruction::JZERO(Instruction::Undef, cur_label);
            output.push_back(jump1);
            Instruction::DEC(output, cur_label);
            Instruction *jump2 = Instruction::JZERO(Instruction::Undef, cur_label);
            output.push_back(jump2);

            //  multiplier = 1;
            Instruction::SUB(output, 0 ,cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, multiplier, cur_label);

            //until b - mult > 0 // internal loop
            int64_t label_loop1_begin = *cur_label;
            Instruction::LOAD(output, target, cur_label);
            Instruction::SUB(output, multiplier, cur_label);
            Instruction::JNEG(output, *cur_label+9, cur_label);
            Instruction *jump_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
            output.push_back(jump_if_0);
            
            // mult = mult * 2 // left = left*2
            Instruction::LOAD(output, multiplier, cur_label);
            Instruction::SHIFT(output, one, cur_label);
            Instruction::STORE(output, multiplier, cur_label);
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SHIFT(output, one, cur_label);
            Instruction::STORE(output, left_temp, cur_label);
            // end of internal loop
            Instruction::JUMP(output, label_loop1_begin, cur_label);

            // when b - mult < 0 // mult = mult/2 // left = left/2
            Instruction::LOAD(output, multiplier, cur_label);
            Instruction::SHIFT(output, neg_one, cur_label);
            Instruction::STORE(output, multiplier, cur_label);
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SHIFT(output, neg_one, cur_label);
            Instruction::STORE(output, left_temp, cur_label);

            // target -= multiplier;
            jump_if_0->arg = *cur_label;
            Instruction::LOAD(output, target, cur_label);
            Instruction::SUB(output, multiplier, cur_label);
            Instruction::STORE(output, target, cur_label);

            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::ADD(output, result, cur_label);
            Instruction::STORE(output, result, cur_label);
            Instruction::LOAD(output, left_temp2, cur_label);
            Instruction::STORE(output, left_temp, cur_label);
            // end of external loop
            Instruction::JUMP(output, begin_loop, cur_label);


            jump2->arg = *cur_label;
            // if target == 1
            Instruction::LOAD(output, result, cur_label);
            Instruction::ADD(output, left_temp2, cur_label);
            Instruction::STORE(output, result, cur_label);


            jump1->arg = *cur_label;
            

            Instruction::LOAD(output, sign, cur_label);
            Instruction::JNEG(output, *cur_label+2, cur_label);
            Instruction::JUMP(output, *cur_label+6, cur_label);

            Instruction::LOAD(output, result, cur_label);
            Instruction::SUB(output, 0, cur_label);
            Instruction::SUB(output, result, cur_label);
            Instruction::STORE(output, result, cur_label);
            Instruction::JUMP(output, *cur_label+2, cur_label);
            Instruction::LOAD(output, result, cur_label);
           
            jump_1_if_0->arg = *cur_label;
            jump_2_if_0->arg = *cur_label;

        }   

        return output;
    }

    //done
    std::vector<Instruction*> Div::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!(check_init(left) && check_init(right))) {
            return output;
        }

        if (left->is_const() && right->is_const()) {
            int64_t num = left->value/right->value;
            return generate_number(num, cur_label);
        } else {
            int64_t left_temp = symbols.offset; symbols.offset++;
            int64_t right_temp = symbols.offset; symbols.offset++;
            int64_t sign = symbols.offset; symbols.offset++;
            int64_t even = symbols.offset; symbols.offset++;
            int64_t ta = symbols.offset; symbols.offset++;
            int64_t one = symbols.offset; symbols.offset++;
            int64_t shift_iter_offset = symbols.offset; symbols.offset++;
            int64_t result_offset = symbols.offset; symbols.offset++;
            int64_t temp_to_compare = symbols.offset; symbols.offset++;
            int64_t temp_to_dec = symbols.offset; symbols.offset++;    

            Instruction *jump_1_if_0; 
            Instruction *jump_2_if_0;

            Instruction::SUB(output,0,cur_label);
            Instruction::STORE(output, result_offset, cur_label);

            if (left->is_const()){
                insert_back(output, generate_number(left->value, cur_label));
                Instruction::STORE(output, left_temp, cur_label);
            } else {
                int64_t left_offset = symbols.offset; symbols.offset++; 
               
                insert_back(output, load_value(left, cur_label));
                jump_1_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
                output.push_back(jump_1_if_0);

                Instruction::JPOS(output, *cur_label+9, cur_label);
                Instruction::STORE(output, left_offset, cur_label);
                Instruction::SUB(output, 0 ,cur_label);
                Instruction::SUB(output ,left_offset ,cur_label);
                Instruction::STORE(output, left_temp, cur_label);
                Instruction::SUB(output ,0 ,cur_label);
                Instruction::DEC(output, cur_label);
                Instruction::STORE(output, sign, cur_label);
                Instruction::JUMP(output, *cur_label+2, cur_label);
                Instruction::STORE(output, left_temp, cur_label);
            }

            if (right->is_const()){
                insert_back(output, generate_number(right->value, cur_label));
                Instruction::STORE(output, right_temp, cur_label);
            } else {
                int64_t right_offset = symbols.offset; symbols.offset++; 
                
                insert_back(output, load_value(right, cur_label));
                jump_2_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
                output.push_back(jump_2_if_0);

                Instruction::JPOS(output, *cur_label+12, cur_label);
                Instruction::STORE(output, right_offset, cur_label);
                Instruction::SUB(output ,0 ,cur_label);
                Instruction::SUB(output ,right_offset ,cur_label);
                Instruction::STORE(output, right_temp, cur_label);
                Instruction::LOAD(output, sign, cur_label);
                Instruction::JNEG(output, *cur_label+3, cur_label);
                Instruction::DEC(output, cur_label);
                Instruction::JUMP(output, *cur_label+2, cur_label);
                Instruction::INC(output, cur_label);
                Instruction::STORE(output, sign, cur_label);
                Instruction::JUMP(output, *cur_label+2, cur_label);
                Instruction::STORE(output, right_temp, cur_label);
            }

            // Ustawianie potrzebnych zmiennych pomocniczych
            Instruction::SUB(output,0,cur_label);
            Instruction::STORE(output, temp_to_dec, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, one, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::STORE(output, even, cur_label);
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::STORE(output, ta, cur_label);


            // Comparing and checking a-2^i*b > 0
            int64_t label_begin = *cur_label;
            Instruction::LOAD(output, ta, cur_label);
            Instruction::SUB(output, temp_to_compare, cur_label);
            Instruction::JNEG(output, *cur_label+9 ,cur_label); 
            Instruction *jump_to_end_if_zero = Instruction::JZERO(Instruction::Undef, cur_label); 
            output.push_back(jump_to_end_if_zero);

            // i++ and temp_to_compare*=2^i
            Instruction::LOAD(output, shift_iter_offset, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::LOAD(output, temp_to_compare, cur_label);
            Instruction::SHIFT(output, one, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);
            Instruction::JUMP(output, *cur_label-10, cur_label);


            // wyjście z minipętli
            Instruction::LOAD(output, shift_iter_offset, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);

            // if iter == 0 it is end
            Instruction *jump_to_end_if_neg = Instruction::JNEG(Instruction::Undef, cur_label); 
            output.push_back(jump_to_end_if_neg);

            // usalwianie even żby odejmować dla ujemnych
            Instruction::JUMP(output, *cur_label+2, cur_label);
            jump_to_end_if_zero->arg = *cur_label;
            //Instruction::INC(output, cur_label);
            Instruction::STORE(output, even, cur_label);

            // result += result+2^i
            Instruction::SUB(output, 0 ,cur_label);
            Instruction::INC(output, cur_label);
            Instruction::SHIFT(output, shift_iter_offset, cur_label);
            Instruction::ADD(output, result_offset, cur_label);
            Instruction::STORE(output, result_offset, cur_label);

            // temp_to_dec = (2^i)*b
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::SHIFT(output, shift_iter_offset, cur_label);
            Instruction::STORE(output, temp_to_dec, cur_label);

            // ta = ta - temp_to_dec
            Instruction::LOAD(output, ta, cur_label);
            Instruction::SUB(output, temp_to_dec, cur_label);
            Instruction::STORE(output, ta, cur_label);

            // iterator and comparator = 0
            Instruction::SUB(output, 0, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);

            // jump back to begin
            Instruction::JUMP(output, label_begin, cur_label);

            jump_to_end_if_neg->arg = *cur_label;

            Instruction::LOAD(output, sign, cur_label);
            Instruction::JNEG(output, *cur_label+2, cur_label);
            Instruction::JUMP(output, *cur_label+9, cur_label);

            Instruction::LOAD(output, result_offset, cur_label);
            Instruction::SUB(output, 0, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction::STORE(output, result_offset, cur_label);
            Instruction::LOAD(output, even, cur_label);
            Instruction::JZERO(output, *cur_label+3, cur_label);
            Instruction::ADD(output, result_offset, cur_label);
            Instruction::JUMP(output, *cur_label+2, cur_label);
            Instruction::LOAD(output, result_offset, cur_label);

            jump_1_if_0->arg = *cur_label;
            jump_2_if_0->arg = *cur_label;

            return output;
        }
    }

    // DONE
    std::vector<Instruction*> Mod::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> output;
        if (!(check_init(left) && check_init(right))) {
            return output;
        }

        if (left->is_const() && right->is_const()) {
            int64_t num = left->value%right->value;
            return generate_number(num, cur_label);
        } else {
            
            int64_t left_temp = symbols.offset; symbols.offset++;
            int64_t right_temp = symbols.offset; symbols.offset++;
            int64_t sign_left = symbols.offset; symbols.offset++;
            int64_t sign_right = symbols.offset; symbols.offset++;
            int64_t ta = symbols.offset; symbols.offset++;
            int64_t one = symbols.offset; symbols.offset++;
            int64_t shift_iter_offset = symbols.offset; symbols.offset++;
            int64_t result_offset = symbols.offset; symbols.offset++;
            int64_t temp_to_compare = symbols.offset; symbols.offset++;
            int64_t temp_to_dec = symbols.offset; symbols.offset++;    

            Instruction *jump_1_if_0; 
            Instruction *jump_2_if_0;

            Instruction::SUB(output,0,cur_label);
            Instruction::STORE(output, result_offset, cur_label);

            if (left->is_const()){
                insert_back(output, generate_number(left->value, cur_label));
                Instruction::STORE(output, left_temp, cur_label);
            } else {
                int64_t left_offset = symbols.offset; symbols.offset++;
               
                insert_back(output, load_value(left, cur_label));
                jump_1_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
                output.push_back(jump_1_if_0);

                Instruction::JPOS(output, *cur_label+9, cur_label);
                Instruction::STORE(output, left_offset, cur_label);
                Instruction::SUB(output, 0 ,cur_label);
                Instruction::SUB(output ,left_offset ,cur_label);
                Instruction::STORE(output, left_temp, cur_label);
                Instruction::SUB(output ,0 ,cur_label);
                Instruction::DEC(output, cur_label);
                Instruction::STORE(output, sign_left, cur_label);
                Instruction::JUMP(output, *cur_label+2, cur_label);
                Instruction::STORE(output, left_temp, cur_label);
            }

            if (right->is_const()){
                insert_back(output, generate_number(right->value, cur_label));
                Instruction::STORE(output, right_temp, cur_label);
            } else {
                int64_t right_offset = symbols.offset; symbols.offset++;
                
                insert_back(output, load_value(right, cur_label));
                jump_2_if_0 = Instruction::JZERO(Instruction::Undef, cur_label);
                output.push_back(jump_2_if_0);

                Instruction::JPOS(output, *cur_label+9, cur_label);
                Instruction::STORE(output, right_offset, cur_label);
                Instruction::SUB(output, 0 ,cur_label);
                Instruction::SUB(output ,right_offset ,cur_label);
                Instruction::STORE(output, right_temp, cur_label);
                Instruction::SUB(output ,0 ,cur_label);
                Instruction::DEC(output, cur_label);
                Instruction::STORE(output, sign_right, cur_label);
                Instruction::JUMP(output, *cur_label+2, cur_label);
                Instruction::STORE(output, right_temp, cur_label);
            }

            // Ustawianie potrzebnych zmiennych pomocniczych
            Instruction::SUB(output,0,cur_label);
            Instruction::STORE(output, temp_to_dec, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, one, cur_label);
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::STORE(output, ta, cur_label);



            // Comparing and checking a-2^i*b > 0
            int64_t label_begin = *cur_label;
            Instruction::LOAD(output, ta, cur_label);
            Instruction::SUB(output, temp_to_compare, cur_label);
            Instruction::JNEG(output, *cur_label+9 ,cur_label); 
            Instruction *jump_to_end_if_zero = Instruction::JZERO(Instruction::Undef, cur_label); 
            output.push_back(jump_to_end_if_zero);

            // i++ and temp_to_compare*=2^i
            Instruction::LOAD(output, shift_iter_offset, cur_label);
            Instruction::INC(output, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::LOAD(output, temp_to_compare, cur_label);
            Instruction::SHIFT(output, one, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);
            Instruction::JUMP(output, *cur_label-10, cur_label);

            // wyjście z minipętli
            Instruction::LOAD(output, shift_iter_offset, cur_label);
            Instruction::DEC(output, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);

            // if iter < 0 it is end
            Instruction *jump_to_end_if_neg = Instruction::JNEG(Instruction::Undef, cur_label); 
            output.push_back(jump_to_end_if_neg);

            // temp_to_dec = (2^i)*b
            // result += (2^i)*b
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::SHIFT(output, shift_iter_offset, cur_label);
            Instruction::STORE(output, temp_to_dec, cur_label);
            Instruction::ADD(output, result_offset, cur_label);
            Instruction::STORE(output, result_offset, cur_label);

            // ta = ta - temp_to_dec
            Instruction::LOAD(output, ta, cur_label);
            Instruction::SUB(output, temp_to_dec, cur_label);
            Instruction::STORE(output, ta, cur_label);

            // iterator and comparator = 0
            Instruction::SUB(output, 0, cur_label);
            Instruction::STORE(output, shift_iter_offset, cur_label);
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::STORE(output, temp_to_compare, cur_label);

            // jump back to begin
            Instruction::JUMP(output, label_begin, cur_label);

            jump_to_end_if_neg->arg = *cur_label;


            // CHECKING FOR SIGN
            Instruction::LOAD(output, sign_left, cur_label);
            Instruction *left_negative = Instruction::JNEG(Instruction::Undef, cur_label); 
            output.push_back(left_negative);
            Instruction::LOAD(output, sign_right, cur_label);
            Instruction::JNEG(output, *cur_label+4, cur_label);
            
            // both positives
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction *jump_end1 = Instruction::JUMP(Instruction::Undef, cur_label); 
            output.push_back(jump_end1);

            // left positive, right negative
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction::SUB(output, right_temp, cur_label);
            Instruction *jump_end2 = Instruction::JUMP(Instruction::Undef, cur_label); 
            output.push_back(jump_end2);

            left_negative->arg = *cur_label;
            
            Instruction::LOAD(output, sign_right, cur_label);
            Instruction::JNEG(output, *cur_label+7, cur_label);

            //left negative, right positive
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction::STORE(output, result_offset, cur_label);
            Instruction::LOAD(output, right_temp, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction *jump_end3 = Instruction::JUMP(Instruction::Undef, cur_label); 
            output.push_back(jump_end3);

            //both negative
            Instruction::LOAD(output, left_temp, cur_label);
            Instruction::SUB(output, result_offset, cur_label);
            Instruction::STORE(output, result_offset, cur_label);
            Instruction::SUB(output, 0, cur_label);
            Instruction::SUB(output, result_offset, cur_label);

            jump_end1->arg = *cur_label;
            jump_end2->arg = *cur_label;
            jump_end3->arg = *cur_label;

            jump_1_if_0->arg = *cur_label;
            jump_2_if_0->arg = *cur_label;
            jump_to_end_if_zero->arg = *cur_label;

            return output;
        }
    }
    std::vector<Instruction*> EQ::gen_ir(int64_t *cur_label) {
         std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JZERO(out,(*cur_label)+3,cur_label);
        Instruction::SUB(out,0,cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::INC(out, cur_label);

        return out;
    }

    std::vector<Instruction*> NEQ::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JZERO(out,(*cur_label)+4,cur_label);
        Instruction::SUB(out,0,cur_label);
        Instruction::INC(out, cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::SUB(out,0,cur_label);

        return out;
    }

    std::vector<Instruction*> LE::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JPOS(out,(*cur_label)+5,cur_label);
        Instruction::JZERO(out,(*cur_label)+4,cur_label);
        Instruction::SUB(out,0, cur_label);
        Instruction::INC(out, cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::SUB(out,0, cur_label);

        return out;
    }

    std::vector<Instruction*> GE::gen_ir(int64_t *cur_label) {
       std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JNEG(out,(*cur_label)+5,cur_label);
        Instruction::JZERO(out,(*cur_label)+4,cur_label);
        Instruction::SUB(out,0, cur_label);
        Instruction::INC(out, cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::SUB(out,0,cur_label);

        return out;
    }

    std::vector<Instruction*> LEQ::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JPOS(out,(*cur_label)+4,cur_label);
        Instruction::SUB(out,0, cur_label);
        Instruction::INC(out, cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::SUB(out,0,cur_label);

        return out;
    }

    std::vector<Instruction*> GEQ::gen_ir(int64_t *cur_label) {
        std::vector<Instruction*> out;
        if (! (check_init(left) && check_init(right))) {
            return out;
        }
        auto temp = Minus(left,right,line).gen_ir(cur_label);
        out.insert(out.end(), temp.begin(), temp.end());
        Instruction::JNEG(out,(*cur_label)+4,cur_label);
        Instruction::SUB(out,0, cur_label);
        Instruction::INC(out, cur_label);
        Instruction::JUMP(out, (*cur_label)+2, cur_label);
        Instruction::SUB(out,0,cur_label);

        return out;
    }
    // WTF
    std::vector<Instruction*> Var::gen_ir(int64_t *cur_label) {
        Symbol var = symbols.get_symbol(name);
        if (var.line == Symbol::undef) {
            std::ostringstream os;
            os << "Udeclared var: " << name << std::endl;
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        if (var.is_array) {
            std::ostringstream os;
            os << "Attempt to use array as a variable: " << name << std::endl;
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        return std::vector<Instruction*>();
    }

    std::vector<Instruction*> ConstArray::gen_ir(int64_t *cur_label) {
        Symbol arr = symbols.get_symbol(name);
        if (arr.line == Symbol::undef) {
            std::ostringstream os;
            os << "Undeclared variable " << name << ".";
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        if (!arr.is_array) {
            std::ostringstream os;
            os << "Attempt to use a simple variable " << name << " as an array";
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        if (idx < arr.idx_b || idx > arr.idx_a) {
            std::cout << index_e << "  " << index_b << std::endl;
            std::ostringstream os;
            os << "Attempt to access array " << name
                << " at index " << idx
                << "(size: " << arr.size << ").";
            generator.report(os, line);
            return std::vector<Instruction*>();
        }

        return std::vector<Instruction*>();
    }

    std::vector<Instruction*> VarArray::gen_ir(int64_t *cur_label) {
    

        Symbol arr = symbols.get_symbol(name);
        if (arr.line == Symbol::undef) {
            std::ostringstream os;
            os << "Undeclared variable " << name << ".";
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        if (!arr.is_array) {
            std::ostringstream os;
            os << "Attempt to use a simple variable " << name << " as an array";
            generator.report(os, line);
            return std::vector<Instruction*>();
        }
        
        return std::vector<Instruction*>();
    }




    void insert_back(std::vector<Instruction*>& target, std::vector<Instruction*>& stuff) {
        target.insert(target.end(), stuff.begin(), stuff.end());
    }

    void insert_back(std::vector<Instruction*>& target, const std::vector<Instruction*>& stuff) {
        target.insert(target.end(), stuff.begin(), stuff.end());
    }

    bool check_init(Identifier *identifier) {

        if (typeid(*identifier) == typeid(ast::VarArray)){
            if (!symbols.is_initialized(((ast::VarArray*)identifier)->index)) {
                std::ostringstream os;
                os << "Attempt to write unintialized variable: " << ((ast::VarArray*)identifier)->index->name << std::endl;
                generator.report(os, identifier->line);
                return false;
            }
        }

        if (!symbols.is_initialized(identifier)) {
            std::ostringstream os;
            os << "Attempt to write unintialized variable: " << identifier->name << std::endl;
            generator.report(os, identifier->line);
            return false;
        }
        return true;
    }
    bool check_init(Value *value) {
        if (value->is_const()) {
            return true;
        }
        return check_init(value->identifier);
    }

    std::vector<Instruction*> generate_number(int64_t number, int64_t *label) {
        std::vector<Instruction*> output;
        Instruction::SUB(output, 0, label);
        if (number == 0) {
            return output;
        }
        bool sign = (number > 0) ? true : false;
        std::vector<char> helper;
        while (std::abs(number) > 0) {
            if (number % 2 == 0) {
                helper.push_back('s');
                number /= 2;
            } else {
                helper.push_back('i');
                if (!sign){
                    number++;
                } else {
                    number--;
                }
            }
        }
         // Mnożenie przez potęgi OPTYMALIZACJA
        reverse(begin(helper), end(helper));
        Instruction::INC(output, label);
        Instruction::STORE(output,symbols.offset,label);
         Instruction::DEC(output, label);
        int64_t one_mem_pos = symbols.offset;
        symbols.offset++;

        for (char c: helper) {
            if (c == 's') {
                Instruction::SHIFT(output, one_mem_pos, label);
            } else if (c == 'i') {
                if (!sign){
                    Instruction::DEC(output, label);
                } else {
                    Instruction::INC(output, label);
                }
            } else {
                std::cerr << "weird" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return output;
    }
    
}


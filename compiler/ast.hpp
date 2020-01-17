#ifndef AST_H
#define AST_H 1

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "asm.hpp"

namespace ast {
    enum NodeType {
        FOR, INIT, BLOCK, DECL, NORMAL
    };

    class Node {
        public:
        Node(int64_t line) : line(line), type(NORMAL) {}
        Node(int64_t line, NodeType type) : line(line), type(type) {}
        Node(NodeType type) : type(type) {}
        Node() {}
        //virtual ~Node() = default;
        int64_t line;
        NodeType type;
        
        virtual std::vector<Instruction*> gen_ir(int64_t *cur_label) = 0;
    };

    class Identifier : public Node {
        public:
            std::string name = "";
            bool array = false;
            bool iter = false;  
            enum Type {N,A,I};
            Identifier(std::string name, Type type, int64_t line)
             : name(name), Node(line) {
                 switch(type) {
                    case A:
                        array = true;
                        break;
                    case I:
                        iter = true;
                        break;
                    default:
                        array = false;
                        iter = false;
                }  
            }
           
    };

    class Declarations : public Node {
        public:
            std::vector<Identifier*> identifiers;
            uint64_t for_counter = 0;
            void declare(Identifier *identifier);
            int64_t report_for() {
                return for_counter++;
            }
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Command : public Node {
        public:
            Command(int64_t line, NodeType type) : Node(line, type) {}
            Command(int64_t line) : Node(line) {}
            Command() {}
    };

    class Commands : public Node {
        std::vector<Command*> commands;
        public:
            void add_command(Command * command);
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Program : public Node {
        public:
            Declarations *declarations;
            Commands *code;
            
            Program(Commands *code) : code(code) {}
            Program(Declarations *declarations, Commands *code)
            : declarations(declarations), code(code) {}
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Value : public Node {
        public:
            int64_t value;
            Identifier *identifier;
            bool constI = true;
            bool is_const() {return constI;}
            Value(int64_t val, int64_t line) : value(val), identifier(NULL), Node(line) {}
            Value(Identifier *identifier, int64_t line) : value(-1), identifier(identifier), Node(line),  constI(false) {}
            
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Expression : public Node {
        public:
        Value *left;
        Value *right;
        
        Expression(Value *left, Value *right, int64_t line) : left(left), right(right) {} 
        virtual std::string op() = 0;
        virtual std::vector<Instruction*> gen_ir(int64_t *cur_label) = 0;
    };

    class Condition : public Node {
        public:
        Value *left;
        Value *right;
        Condition(Value *left, Value *right, int64_t line) : left(left), right(right) {}
    };

    class Assign : public Command {
        public:
            Identifier *identifier;
            Expression *expression;

            Assign(Identifier *identifier, Expression *expression, int64_t line) 
            : identifier(identifier), expression(expression), Command(line, INIT) {}
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class If : public Command {
        public:
            Condition *condition;
            Commands *do_then;
            Commands *do_else;

            If(Condition *condition, Commands *do_then, int64_t line) 
            : condition(condition), do_then(do_then), do_else(NULL), Command(line) {}
            If(Condition *condition, Commands *do_then, Commands *do_else, int64_t line)
            : condition(condition), do_then(do_then), do_else(do_else), Command(line) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label);        
    };

    class While : public Command {
        public:
            Condition *condition;
            Commands *body;
            bool reversed;

            While(Condition *condition, Commands *body, bool reversed, int64_t line)
            : condition(condition), body(body), reversed(reversed), Command(line) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class For : public Command {
        private:
            int64_t id;
        public:
            Identifier *iterator;
            Value *from;
            Value *to;
            Commands *body;
            bool reversed;

            For(Identifier *iterator, Value *from, Value *to, Commands *body, bool reversed, int64_t line, int64_t id)
            : iterator(iterator), from(from), to(to),body(body), reversed(reversed), Command(line), id(id) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Read : public Command {
        public:
            Identifier *identifier;

            Read(Identifier *identifier, int64_t line) 
            : identifier(identifier), Command(line) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label); 
    };

    class Write : public Command {
        public:
            Value *value;

            Write(Value *value, int64_t line)
            : value(value), Command(line) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Const : public Expression {
        public:
            Const(Value *value, int64_t line) 
            : Expression(value, NULL, line) {}

            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Const"; }
    };   

    class Plus : public Expression {
        public:
            Plus(Value *left, Value *right, int64_t line) : Expression(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Plus"; }
    };

    class Minus : public Expression {
        public:
            Minus(Value *left, Value *right, int64_t line) : Expression(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Minus"; }
    };

    class Times : public Expression {
        public:
            Times(Value *left, Value *right, int64_t line) : Expression(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Times"; }
    };

    class Div : public Expression {
        public:
            Div(Value *left, Value *right, int64_t line) : Expression(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Div"; }
    };

    class Mod : public Expression {
        public:
            Mod(Value *left, Value *right, int64_t line) : Expression(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
             std::string op() { return "Mod"; }
    };

    class EQ : public Condition {
        public:
            EQ(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

     class NEQ : public Condition {
        public:
            NEQ(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

     class LE : public Condition {
        public:
            LE(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

     class GE : public Condition {
        public:
            GE(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

     class LEQ : public Condition {
        public:
            LEQ(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

     class GEQ : public Condition {
        public:
            GEQ(Value *left, Value *right, int64_t line) : Condition(left, right, line) {}
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class Var : public Identifier {
        public:
            Var(std::string name, int64_t line) : Identifier(name, N, line) {}
            Var(std::string name, Type type, int line) : Identifier(name, type, line) {}
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
           
    };

    class ConstArray : public Identifier {
        public:
            int64_t index_b, index_e;
            int64_t idx;
            ConstArray(std::string name, int64_t index, int64_t line)
            : Identifier(name, A, line), idx(index) {}
            ConstArray(std::string name, int64_t index_b, int64_t index_e, int64_t line)
            : Identifier(name, A, line), index_b(index_b), index_e(index_e){}
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

    class VarArray : public Identifier {
        public:
            Var *index;
           
            VarArray(std::string name, Var *index, int64_t line)
            : Identifier(name, A, line), index(index) {}
            
            std::vector<Instruction*> gen_ir(int64_t *cur_label);
    };

}
#endif
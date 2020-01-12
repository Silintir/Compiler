#ifndef CODEGEN_H
#define CODEGEN_H 1

#include <vector>
#include <iostream>
#include "asm.hpp"
#include "ast.hpp"


class CodeGen {
    int64_t instruction_counter = 0;
    int64_t n_error = 0;
    public:
        std::vector<Instruction*> generate(ast::Node *root);
        bool generate_to(std::ostream &stream, ast::Node *root);
        void report(std::string error, int64_t line);
        void report(std::ostringstream& error, int64_t line);
};
#endif
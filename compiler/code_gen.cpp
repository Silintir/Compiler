#include <vector>
#include "code_gen.hpp"
#include "ast.hpp"
#include "asm.hpp"
#include "symbols.hpp"

extern Symbols symbols;


std::vector<Instruction *> CodeGen::generate(ast::Node *root) {
    return root->gen_ir(&instruction_counter);
}
    bool CodeGen::generate_to(std::ostream &stream, ast::Node *root) {
    auto code = root->gen_ir(&instruction_counter);
    if (n_error > 0) {
        std::cerr << "Compilation" << " failed: " <<   n_error << " errors found." << std::endl;
        return false;
    } else {
        for(int i = 0; i < code.size(); i++) {
            
            stream << *code[i];
        }
        return true;
    }
}

void CodeGen::report(std::string error, int64_t line) {
    std::cerr << "Error in line " << line << ": "
            << error << std::endl;
    n_error++;
}

void CodeGen::report(std::ostringstream& error, int64_t line) {
    std::cerr << "Error in line " << line << ": " 
            << error.str() << std::endl;
    n_error++;
}


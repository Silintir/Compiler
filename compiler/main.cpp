#include <vector>
#include <iostream>
#include "ast.hpp"
#include "asm.hpp"
#include "code_gen.hpp"
#include "symbols.hpp"

extern int yyparse();
extern int errors;

ast::Node *root;
std::vector<int64_t> memory;
Symbols symbols;
CodeGen generator;


int main() {
    //yydebug = YYDEBUG;
    int syntaxInvalid = yyparse();
    if (syntaxInvalid || errors > 0) {
        std::cerr << "Error compiling file: " << errors << "syntax errors found" << std::endl;
        return 1;
    } else {
        // cerr << Color::green << "No syntax errors" << Color::def << endl;
        if (root != NULL) {
            bool result = generator.generate_to(std::cout, root);
            if (result) {
                std::cerr << "Compilation successful" << std::endl;
            }
            return !result;
        }
    }
    return 1;
}
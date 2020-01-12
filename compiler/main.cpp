#include <vector>
#include <iostream>
#include "ast.hpp"
#include "asm.hpp"
#include "code_gen.hpp"
#include "symbols.hpp"

extern int yyparse();


ast::Node *root;
std::vector<int64_t> memory;
Symbols symbols;
CodeGen generator;

int main() {
    //yydebug = YYDEBUG;
    int syntaxInvalid = yyparse();
    if (syntaxInvalid) {
        std::cerr << "Error compiling file: " << std::endl;
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
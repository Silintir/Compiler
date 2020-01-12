#ifndef SYMBOLS_H
#define SYMBOLS_H 1

#include <unordered_map>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>
#include "ast.hpp"

void error(std::string msg, int64_t loc);

class Symbol {
    public:
        bool is_array = false;
        bool is_initialized = false;
        bool iterator = false;
        bool not_in_scope = true;
        std::string name;
        int64_t offset_id;
        int64_t size;
        int64_t line;
        static const int64_t undef = -1;
        Symbol () : line(undef) {}
        Symbol(std::string name, int64_t line, int64_t size, int64_t offset_id) : 
        name(name), line(line), size(size), offset_id(offset_id), is_initialized(false) {}
        Symbol(std::string name, int64_t line, int64_t offset_id) : 
        name(name), line(line), size(1), offset_id(offset_id), is_initialized(false) {}
};

class Symbols {
    private:
        int64_t for_offset = 0;
        std::unordered_map<std::string, Symbol> table;
    public:
        int64_t offset = 0;
        Symbol get_symbol(std::string);
        bool declare(ast::Identifier *identifier);
        bool declare_tmp(std::string name);
        bool declare_iterator(ast::Identifier *iter);
        bool undeclare(std::string name);
        bool undeclare_iter(std::string name);
        bool is_initialized(ast::Identifier *identifier);
        bool set_initialized(ast::Identifier *identifier);
        void alloc_for_control(int64_t for_count);
        bool is_iterator(ast::Identifier *identifier);
        void set_iterator(ast::Identifier *identifier);
        void set_array(ast::Identifier *identifier);
};
#endif
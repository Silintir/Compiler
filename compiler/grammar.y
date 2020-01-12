%locations

%{
    #include <string>
    #include <iostream> 
    #include "ast.hpp"

    extern ast::Node *root;
    extern std::vector<int64_t> memory;
    ast::Declarations *decls = NULL;
    extern int64_t line;
    int64_t line = 1;
    extern int yylex();

    void yyerror(std::string msg) {
        std::cerr << "[" << line-1 << "] " << "ERROR: " << msg << std::endl;
    }
%}

%union {
    int token;
    char* text;
    int64_t val;
    ast::Program *program;
    ast::Declarations *declarations;
    ast::Commands *commands;
    ast::Command *command;
    ast::Expression *expression;
    ast::Condition *condition;
    ast::Value *value;
    ast::Identifier *identifier;
}


%token <token> DECLARE COMMA K_BEGIN K_END
%token <token> ASSIGN PLUS MINUS TIMES DIV MOD 
%token <token> BRACKET_ON COLON BRACKET_OFF SEMICOLON
%token <token> WHILE DO ENDWHILE ENDDO
%token <token> IF THEN ELSE ENDIF
%token <token> FOR FROM TO DOWNTO ENDFOR
%token <token> READ WRITE
%token <token> EQ NEQ LE GE LEQ GEQ
%token <text> PIDENTIFIER
%token <val> NUM
%left PLUS MINUS
%left TIMES DIV MOD

%type <program> program
%type <declarations> declarations
%type <commands> commands
%type <command> command
%type <expression> expression
%type <condition> condition
%type <value> value
%type <identifier> identifier

%start program

%% 

program:
        DECLARE declarations K_BEGIN commands K_END {
            $$ = new ast::Program($2, $4);
            root = $$;
        }
    |   K_BEGIN commands K_END {
            $$ = new ast::Program($2);
            root = $$;
        }
    ;
declarations:
        declarations COMMA PIDENTIFIER {
            decls->declare(new ast::Var($3,line));
        }
    |   declarations COMMA PIDENTIFIER BRACKET_ON NUM COLON NUM BRACKET_OFF {
            decls->declare(new ast::ConstArray($3,$5,$7,line));
        }
    |   PIDENTIFIER {
            $$ = new ast::Declarations();
            $$->declare(new ast::Var($1,line));
            decls = $$;
        }
    |   PIDENTIFIER BRACKET_ON NUM COLON NUM BRACKET_OFF {
            $$ = new ast::Declarations();
            $$->declare(new ast::ConstArray($1,$3,$5,line));
            decls = $$;
        }
    ;
commands:
        commands command {
            $1->add_command($2);
        }
    |   command {
        $$ = new ast::Commands();
        $$->add_command($1);
        }
    ;
command:
        identifier ASSIGN expression SEMICOLON {
            $$ = new ast::Assign($1, $3, line);
        }
    |   IF condition THEN commands ELSE commands ENDIF {
            $$ = new ast::If($2,$4,$6,line);
        }
    |   IF condition THEN commands ENDIF {
            $$ = new ast::If($2,$4,line);
        }
    |   WHILE condition DO commands ENDWHILE {
            $$ = new ast::While($2,$4,false,line);
        }
    |   DO commands WHILE condition ENDDO {
            $$ = new ast::While($4,$2,true,line);
        }
    |   FOR PIDENTIFIER FROM value TO value DO commands ENDFOR {
            auto *iterator = new ast::Var($2, ast::Identifier::I, line);
            decls->declare(iterator);
            $$ = new ast::For(iterator, $4, $6, $8, false, line, decls->report_for());
    }
    |   FOR PIDENTIFIER FROM value DOWNTO value DO commands ENDFOR {
            auto *iterator = new ast::Var($2, ast::Identifier::I, line);
            decls->declare(iterator);
            $$ = new ast::For(iterator, $4, $6, $8, true, line, decls->report_for());
    }
    |   READ identifier SEMICOLON {
            $$ = new ast::Read($2,line);
        }
    |   WRITE value SEMICOLON {
            $$ = new ast::Write($2,line);
        }
    ;
    expression:
        value { 
            $$ = new ast::Const($1, line); 
        }
    |   value PLUS value { 
            $$ = new ast::Plus($1, $3, line); 
        }
    |   value MINUS value { 
            $$ = new ast::Minus($1, $3, line); 
        }
    |   value TIMES value { 
            $$ = new ast::Times($1, $3, line); 
        }
    |   value DIV value { 
            $$ = new ast::Div($1, $3, line); 
        }
    |   value MOD value { 
        $$ = new ast::Mod($1, $3, line); 
        }
    ;

condition:
        value EQ value  { 
            $$ = new ast::EQ($1, $3, line); 
        }
    |   value NEQ value { 
        $$ = new ast::NEQ($1,$3, line); 
        }
    |   value GE value  { 
        $$ = new ast::GE($1,$3, line); 
        }
    |   value LE value  { 
        $$ = new ast::LE($1,$3, line); 
        }
    |   value LEQ value { 
        $$ = new ast::LEQ($1,$3, line); 
        }
    |   value GEQ value { 
        $$ = new ast::GEQ($1,$3, line); 
        }
    ;

value:
        NUM { 
            $$ = new ast::Value($1, line); 
        }
    |   identifier {
            $$ = new ast::Value($1, line);
        }
    ;

  identifier:
        PIDENTIFIER {
            $$ = new ast::Var($1, line);
        }
    |   PIDENTIFIER BRACKET_ON PIDENTIFIER BRACKET_OFF { 
            $$ = new ast::VarArray($1, new ast::Var($3, line), line); 
        }
    |   PIDENTIFIER BRACKET_ON NUM BRACKET_OFF { 
            $$ = new ast::ConstArray($1, $3, line); 
        }
    ;
%%  
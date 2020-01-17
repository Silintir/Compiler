%option noyywrap
%option yylineno

%{
    #include "ast.hpp"
    #include "grammar.tab.h"

    #define TOKEN(t) return (yylval.token = t)
    #define LOAD_INT yylval.val = strtoll(yytext, NULL, 10)
    extern int64_t line;
%}

%x COMMENT
%%
"["             BEGIN(COMMENT);
<COMMENT>"]"    BEGIN(INITIAL);
<COMMENT>.      ;
<COMMENT>\n     ; line++;
[_a-z]+         {
                    yylval.text = (char *) malloc(yyleng * sizeof(char) + 1);
                    strcpy(yylval.text, yytext);
                    return PIDENTIFIER;
                }
\-?[0-9]+          { LOAD_INT; return NUM; }
"MINUS"         { TOKEN(MINUS); }
"PLUS"          { TOKEN(PLUS); }
"TIMES"         { TOKEN(TIMES); }
"DIV"           { TOKEN(DIV); }
"MOD"           { TOKEN(MOD); }
"ASSIGN"        { TOKEN(ASSIGN); }
"EQ"            { TOKEN(EQ); }
"NEQ"           { TOKEN(NEQ); }
"LE"            { TOKEN(LE); }
"GE"            { TOKEN(GE); }
"LEQ"           { TOKEN(LEQ); }
"GEQ"           { TOKEN(GEQ); }
"DECLARE"       { TOKEN(DECLARE); }
"BEGIN"         { TOKEN(K_BEGIN); }
"END"           { TOKEN(K_END); }
"READ"          { TOKEN(READ); }
"WRITE"         { TOKEN(WRITE); }
"IF"            { TOKEN(IF); }
"THEN"          { TOKEN(THEN); }
"ELSE"          { TOKEN(ELSE); }
"ENDIF"         { TOKEN(ENDIF); }
"FOR"           { TOKEN(FOR); }
"FROM"          { TOKEN(FROM); }
"TO"            { TOKEN(TO); }
"DOWNTO"        { TOKEN(DOWNTO); }
"DO"            { TOKEN(DO); }
"ENDFOR"        { TOKEN(ENDFOR); }
"WHILE"         { TOKEN(WHILE); }
"ENDWHILE"      { TOKEN(ENDWHILE); }
"ENDDO"         { TOKEN(ENDDO); }
";"             { TOKEN(SEMICOLON); }
"("             { TOKEN(BRACKET_ON); }
")"             { TOKEN(BRACKET_OFF); }
":"             { TOKEN(COLON); }
","             { TOKEN(COMMA); }
"\n"            { line++; }
[ \t]           ;
.               ;
%%
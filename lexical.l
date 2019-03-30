  /*
      C-- Compiler Front End
      Copyright (C) 2019 NSKernel. All rights reserved.

      A lab of Compilers at Nanjing University

      lexical.l
      Lexical definitions for C--
  */

%option yylineno

%{
    #include <stdio.h>
	#include <string.h>
    #include <stdint.h>
    #include <ast.h>
    #include "syntax.tab.h"
    #include <debug.h>

    #define _POSIX_C_SOURCE 1

    int yycolumn = 1;
    int bc_line_iterator;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

#ifndef TF
#define TF
	#define true 1
    #define false 0
#endif
	char error_flag = false;
    char bc_changed_line = false;
    
%}

DIGIT           [0-9]
NZDIGIT         [1-9]
OCTDIGIT        [0-7]
HEXDIGIT        [0-9abcdefABCDEF]

%%

\r                              
"//"[^\n]*                      { 
                                    print_debug("Lexer DBG: LINE COMMENT\n");
                                }
"/*"([^\*]*"*"+[^\*/])*[^\*]*"*"+"/"  {
                                    // Block comment regex borrowed idea from http://www.cs.man.ac.uk/~pjj/cs212/ex2_str_comm.html
                                    for (bc_line_iterator = 0; bc_line_iterator < strlen(yytext); bc_line_iterator++) {
                                        if (yytext[bc_line_iterator] == '\n')
                                            bc_changed_line = true;
                                    }
                                    if (bc_changed_line) {
                                        yycolumn = 1;
                                        for (bc_line_iterator--; yytext[bc_line_iterator] != '\n'; bc_line_iterator--) {
                                            yycolumn += 1;
                                        }
                                    }
                                    print_debug("Lexer DBG: BLOCK COMMENT\n");
                                }

0{OCTDIGIT}+                    { 
                                    yylval.node = ast_make_new_node(
                                        "INT", 
                                        yylloc.first_line,
                                        true, 
                                        NULL, 
                                        T_INT,
                                        0);
                                    sscanf(yytext, "0%o", &(yylval.node->int_value));
                                    
                                    print_debug("Lexer DBG: INT with value of %d\n", yylval.node->int_value);
                                    
                                    return INT; 
                                }
0{DIGIT}+                       {
                                    yylval.node = NULL;
                                    error_flag = true;
                                    printf("Error type A at Line %d: Incorrect oct number \"%s\".\n", yylineno, yytext);
                                    return INT; // still treat as a node
                                }
0(x|X){HEXDIGIT}+               { 
                                    yylval.node = ast_make_new_node(
                                        "INT", 
                                        yylloc.first_line,
                                        true, 
                                        NULL, 
                                        T_INT,
                                        0);
                                    if (yytext[1] == 'x')
                                        sscanf(yytext, "0x%x", &(yylval.node->int_value)); 
                                    if (yytext[1] == 'X')
                                        sscanf(yytext, "0X%x", &(yylval.node->int_value)); 
                                    
                                    print_debug("Lexer DBG: INT with value of d\n", yylval.node->int_value);
                                    
                                    return INT; 
                                }
0(x|X)[0-9a-zA-Z]+              {
                                    yylval.node = NULL;
                                    error_flag = true;
                                    printf("Error type A at Line %d: Incorrect hex number \"%s\".\n", yylineno, yytext);
                                    return INT; // still treat as a node
                                }
({NZDIGIT}{DIGIT}*)|0           { 
                                    yylval.node = ast_make_new_node(
                                        "INT", 
                                        yylloc.first_line,
                                        true, 
                                        NULL, 
                                        T_INT,
                                        0);
                                    yylval.node->int_value = atoi(yytext); 
                                    
                                    print_debug("Lexer DBG: INT with value of %d\n", yylval.node->int_value);
                                    
                                    return INT; 
                                }
{DIGIT}+"."{DIGIT}+             { 
                                    yylval.node = ast_make_new_node(
                                        "FLOAT", 
                                        yylloc.first_line,
                                        true, 
                                        NULL, 
                                        T_FLOAT,
                                        0);
                                    yylval.node->float_value = atof(yytext); 
                                    
                                    print_debug("Lexer DBG: FLOAT with value of %f\n", yylval.node->float_value);
                                    
                                    return FLOAT; 
                                }
([0-9]*"."[0-9]+(e|E)("+"|"-")?[0-9]+)|([0-9]+"."[0-9]*(e|E)("+"|"-")?[0-9]+)  {
                                    yylval.node = ast_make_new_node(
                                        "FLOAT",
                                        yylloc.first_line,
                                        true,
                                        NULL,
                                        T_FLOAT,
                                        0);
                                    yylval.node->float_value = atof(yytext);
                                    
                                    print_debug("Lexer DBG: FLOAT with value of %f\n", yylval.node->float_value);
                                     
                                    return FLOAT;
                                }

";"                             {  
                                    yylval.node = ast_make_new_node(
                                        "SEMI", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: SEMI\n");
                                     
                                    return SEMI; 
                                }
","                             {  
                                    yylval.node = ast_make_new_node(
                                        "COMMA", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: COMMA\n");
                                     
                                    return COMMA; 
                                }
"="                             {  
                                    yylval.node = ast_make_new_node(
                                        "ASSIGNOP", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: ASSIGNOP\n");
                                     
                                    return ASSIGNOP; 
                                }
(">"|"<"|">="|"<="|"=="|"!=")   {  
                                    yylval.node = ast_make_new_node(
                                        "RELOP", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: RELOP\n");
                                     
                                    return RELOP; 
                                }
"+"                             {  
                                    yylval.node = ast_make_new_node(
                                        "PLUS", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: PLUS\n");
                                     
                                    return PLUS; 
                                }
"-"                             {  
                                    yylval.node = ast_make_new_node(
                                        "MINUS", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: MINUS\n");
                                     
                                    return MINUS; 
                                }
"*"                             {  
                                    yylval.node = ast_make_new_node(
                                        "STAR", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: STAR\n");
                                     
                                    return STAR; 
                                }
"/"                             {  
                                    yylval.node = ast_make_new_node(
                                        "DIV", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: DIV\n");
                                     
                                    return DIV; 
                                }
"&""&"                          {  
                                    yylval.node = ast_make_new_node(
                                        "AND", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: AND\n");
                                     
                                    return AND; 
                                }
"|""|"                          {  
                                    yylval.node = ast_make_new_node(
                                        "OR", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: OR\n");
                                     
                                    return OR; 
                                }
"."                             {  
                                    yylval.node = ast_make_new_node(
                                        "DOT", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: DOT\n");
                                     
                                    return DOT; 
                                }
"!"                             {  
                                    yylval.node = ast_make_new_node(
                                        "NOT", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: NOT\n");
                                     
                                    return NOT; 
                                }
(int|float)                     {  
                                    yylval.node = ast_make_new_node(
                                        "TYPE", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_ID_TYPE,
                                        0);
                                    
                                    print_debug("Lexer DBG: TYPE %s\n", yytext);
                                     
                                    return TYPE; 
                                }
"("                             {  
                                    yylval.node = ast_make_new_node(
                                        "LP", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: LP\n");
                                     
                                    return LP; 
                                }
")"                             {  
                                    yylval.node = ast_make_new_node(
                                        "RP", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: RP\n");
                                     
                                    return RP; 
                                }
"["                             {  
                                    yylval.node = ast_make_new_node(
                                        "LB", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: LB\n");
                                     
                                    return LB; 
                                }
"]"                             {  
                                    yylval.node = ast_make_new_node(
                                        "RB", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: RB\n");
                                     
                                    return RB; 
                                }
"{"                             {  
                                    yylval.node = ast_make_new_node(
                                        "LC", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: LC\n");
                                     
                                    return LC; 
                                }
"}"                             {  
                                    yylval.node = ast_make_new_node(
                                        "RC", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: RC\n");
                                     
                                    return RC; 
                                }
struct                          {  
                                    yylval.node = ast_make_new_node(
                                        "STRUCT", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: STRUCT\n");
                                     
                                    return STRUCT; 
                                }
return                          {  
                                    yylval.node = ast_make_new_node(
                                        "RETURN", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: RETURN\n");
                                     
                                    return RETURN; 
                                }
if                              {  
                                    yylval.node = ast_make_new_node(
                                        "IF", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: IF\n");
                                     
                                    return IF; 
                                }
else                            {  
                                    yylval.node = ast_make_new_node(
                                        "ELSE", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: ELSE\n");
                                     
                                    return ELSE; 
                                }
while                           {  
                                    yylval.node = ast_make_new_node(
                                        "WHILE", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_OTHER,
                                        0);
                                    
                                    print_debug("Lexer DBG: WHILE\n");
                                     
                                    return WHILE; 
                                }

([_a-zA-Z]([_0-9a-zA-Z])*)      {  
                                    yylval.node = ast_make_new_node(
                                        "ID", 
                                        yylloc.first_line,
                                        true, 
                                        yytext, 
                                        T_ID_TYPE,
                                        0);
                                    
                                    print_debug("Lexer DBG: ID\n");
                                     
                                    return ID; 
                                }
" "
\t
\n                              { 
                                    yycolumn = 1; 
                                    
                                    print_debug("Lexer DBG: New Line\n");
                                }

.                               { 
                                    error_flag = true; 
                                    printf("Error type A at Line %d: Unrecogonized \"%s\".\n", yylineno, yytext); 
                                }
%%


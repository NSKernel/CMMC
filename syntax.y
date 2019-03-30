/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    syntax.y
    Syntax definitions for C--
*/

%{
    #include <stdio.h>
    #include <stdint.h>
    #include <ast.h>

    #define YYERROR_VERBOSE

    ast_node *root_node;
    extern char error_flag;
    extern int yylineno;

    extern int yylex();
    int yyerror(char* msg);
    #ifdef DEBUG
    #define YYDEBUG  1  
    int yydebug = 1;
    #endif
%}

%union
{
    ast_node *node;
}

%type <node>     ExtDefList ExtDef ExtDecList
%type <node>     Specifier  StructSpecifier OptTag Tag
%type <node>     VarDec FunDec VarList ParamDec
%type <node>     CompSt StmtList Stmt
%type <node>     DefList Def DecList Dec
%type <node>     Exp Args

%right <node> ASSIGNOP
%left  <node> OR
%left  <node> AND
%left  <node> RELOP
%left  <node> PLUS MINUS
%left  <node> STAR DIV
%right <node> NOT PREFIXMINUS
%left  <node> DOT
%nonassoc <node> LP RP LB RB

%nonassoc ELSE_BREAKER
%nonassoc <node> ELSE

%nonassoc ERROR_OUT
%nonassoc <node> SEMI


%token <node> COMMA
%token <node> TYPE
%token <node> LC
%token <node> RC
%token <node> STRUCT
%token <node> RETURN
%token <node> IF
%token <node> WHILE
%token <node> INT
%token <node> FLOAT
%token <node> ID


%%

/* High-Level Definitions */
Program : ExtDefList    
        { 
            root_node = ast_make_new_node(
                "Program", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            root_node->children[0] = $1; 
        }
  ;
ExtDefList : ExtDef ExtDefList 
        {
            $$ = ast_make_new_node(
                "ExtDefList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1; 
            $$->children[1] = $2;
        }
  | /* Empty */ 
        { 
            $$ = NULL;
        }
    ;
ExtDef : Specifier ExtDecList SEMI 
        {
            $$ = ast_make_new_node(
                "ExtDef", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1; 
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Specifier SEMI 
        { 
            $$ = ast_make_new_node(
                "ExtDef", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1; 
            $$->children[1] = $2;
        }
  | Specifier FunDec CompSt 
        {
            $$ = ast_make_new_node(
                "ExtDef", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | error SEMI  
        {
            $$ = NULL;
        }
    ;
ExtDecList : VarDec 
        { 
            $$ = ast_make_new_node(
                "ExtDecList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | VarDec COMMA ExtDecList 
        { 
            $$ = ast_make_new_node(
                "ExtDecList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  ;

/* Specifiers */
Specifier : TYPE 
        { 
            $$ = ast_make_new_node(
                "Specifier", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | StructSpecifier  
        { 
            $$ = ast_make_new_node(
                "Specifier", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  ;
StructSpecifier : STRUCT OptTag LC DefList RC 
        { 
            $$ = ast_make_new_node(
                "StructSpecifier", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                5);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
            $$->children[4] = $5;
        }
  | STRUCT Tag 
        { 
            $$ = ast_make_new_node(
                "StructSpecifier", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  ;
OptTag : ID 
        {
            $$ = ast_make_new_node(
                "OptTag", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | /* Empty */ { $$ = NULL; }
  ;
Tag : ID 
        { 
            $$ = ast_make_new_node(
                "Tag", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  ;

/* Declaration */
VarDec : ID 
        {
            $$ = ast_make_new_node(
                "VarDec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | VarDec LB INT RB 
        {
            $$ = ast_make_new_node(
                "VarDec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                4);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
        }
  | VarDec LB error RB  
        { $$ = NULL; }
  ;
FunDec : ID LP VarList RP 
        {
            $$ = ast_make_new_node(
                "FunDec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                4);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
        }
  | ID LP RP 
        {
            $$ = ast_make_new_node(
                "FunDec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | error RP  { $$ = NULL; }
  ;
VarList : ParamDec COMMA VarList 
        {
            $$ = ast_make_new_node(
                "VarList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | ParamDec 
        {
            $$ = ast_make_new_node(
                "VarList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
ParamDec : Specifier VarDec 
        {
            $$ = ast_make_new_node(
                "ParamDec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  ;
/* Statements */
CompSt : LC DefList StmtList RC 
        {
            $$ = ast_make_new_node(
                "CompSt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                4);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
        }
  | error RC  { $$ = NULL; }
  ;
StmtList : Stmt StmtList 
        {
            $$ = ast_make_new_node(
                "StmtList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  | /* Empty */ { $$ = NULL; }
  ;
Stmt : Exp SEMI 
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  | CompSt 
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | RETURN Exp SEMI
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | IF LP Exp RP Stmt ELSE Stmt 
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                7);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
            $$->children[4] = $5;
            $$->children[5] = $6;
            $$->children[6] = $7;
        }
  | IF LP Exp RP Stmt %prec ELSE_BREAKER 
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                5);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
            $$->children[4] = $5;
        }
  | WHILE LP Exp RP Stmt 
        {
            $$ = ast_make_new_node(
                "Stmt", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                5);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
            $$->children[4] = $5;
        }
  | error SEMI  { $$ = NULL; }
  ;

/* Local Definitions */
DefList : Def DefList 
        {
            $$ = ast_make_new_node(
                "DefList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  | /* Empty */ { $$ = NULL; }
  ;
Def : Specifier DecList SEMI 
        {
            $$ = ast_make_new_node(
                "Def", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Specifier error SEMI  { $$ = NULL; }
  ;
DecList : Dec 
        {
            $$ = ast_make_new_node(
                "DecList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | Dec COMMA DecList 
        {
            $$ = ast_make_new_node(
                "DecList", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  ;
Dec : VarDec 
        {
            $$ = ast_make_new_node(
                "Dec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | VarDec ASSIGNOP Exp 
        {
            $$ = ast_make_new_node(
                "Dec", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  ;

/* Expressions */
Exp : Exp DOT ID 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | LP Exp RP 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | ID LP Args RP 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                4);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
        }
  | ID LP RP 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | ID 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | INT 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | FLOAT 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | Exp LB Exp RB 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                4);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
            $$->children[3] = $4;
        }
  | MINUS Exp %prec PREFIXMINUS 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  | NOT Exp 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                2);
            $$->children[0] = $1;
            $$->children[1] = $2;
        }
  | Exp STAR Exp 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp DIV Exp 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp PLUS Exp
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp MINUS Exp
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp RELOP Exp
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp AND Exp
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp OR Exp 
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp ASSIGNOP Exp
        {
            $$ = ast_make_new_node(
                "Exp", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp LB error RB  { $$ = NULL; }
  | LP error RP { $$ = NULL; }
  ;
Args : Exp COMMA Args
        {
            $$ = ast_make_new_node(
                "Args", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                3);
            $$->children[0] = $1;
            $$->children[1] = $2;
            $$->children[2] = $3;
        }
  | Exp 
        {
            $$ = ast_make_new_node(
                "Args", 
                @1.first_line,
                false, 
                NULL, 
                T_NT,
                1);
            $$->children[0] = $1;
        }
  | error COMMA Args  { $$ = NULL; }
  ;


%%

int yyerror(char* msg) {
    error_flag = true;
	fprintf(stdout, "Error type B at Line %d: %s.\n",  yylineno, msg);
    return 0;
}
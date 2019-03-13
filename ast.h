/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    ast.h
    Defines ast variables and structures
*/

#include <stdlib.h>
#include <string.h>

#ifndef TRUE_FALSE_KEYWORD
#define TRUE_FALSE_KEYWORD
#define true 1
#define false 0
#endif

#ifndef AST_H
#define AST_H

#define T_NT		0x0
#define T_ID_TYPE   0x1
#define T_INT   	0x2
#define T_FLOAT 	0x3
#define T_OTHER 	0x4

typedef struct ast_node_t {
    char *name;

	
	char terminal_type;
	
	char is_terminal;
    uint32_t line_number;
	uint32_t children_count;
	union 
	{
		char *string_value;
		int int_value;
		float float_value;
	};
    struct ast_node_t *children[7];
    struct ast_node_t *parent;
} ast_node;

ast_node *ast_make_new_node(char *name, uint32_t line_number, char is_terminal, const void *value, char terminal_type, uint32_t children_count);

#endif
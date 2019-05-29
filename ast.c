/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    ast.c
    Defines ast operations
*/

#include <stdint.h>

#include <ast.h>

ast_node *ast_make_new_node(char *name, uint32_t line_number, char is_terminal, const void *value, char terminal_type, uint32_t children_count) {
	  ast_node *return_node;
	  return_node = (ast_node*)malloc(sizeof(ast_node));
	  return_node->children_count = children_count;
	  return_node->name = name;
	  return_node->line_number = line_number;
	  return_node->terminal_type = terminal_type;
	  if (terminal_type == T_ID_TYPE || terminal_type == T_OTHER) {
		    return_node->string_value = (char*)malloc(strlen(value) + 1);
				strncpy(return_node->string_value, value, strlen(value));
				return_node->string_value[strlen(value)] = 0;
	  }
	  if (terminal_type == T_INT) {
			  if (value != NULL)
	  	  		return_node->int_value = *(int*)value;
	  }
  	else if (terminal_type == T_FLOAT) {
			  if (value != NULL)
		    		return_node->float_value = *(float*)value;
	  }
	  return_node->is_terminal = is_terminal;

	  return return_node;
}

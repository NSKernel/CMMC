/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    main.c
    Main entry
*/

#include <stdio.h>
#include "ast.h"


char spaces[100];
int space_count = 0;


extern FILE *yyin;
extern int yylex();
extern int yyparse(void);
extern char error_flag;
extern ast_node *root_node;

#ifdef DEBUG
extern char yydebug;
#endif

void print_ast(ast_node *current_node)
{
	int iterator;
	printf("%s%s (%d)\n", spaces, current_node->name, current_node->line_number);
	for (iterator = 0; iterator < current_node->children_count; iterator++)
	{
		spaces[space_count] = ' ';
		spaces[space_count + 1] = ' ';
		space_count += 2;
		if (current_node->children[iterator] != NULL)
		{
			if (current_node->children[iterator]->is_terminal)
			{

				if (current_node->children[iterator]->terminal_type == T_ID_TYPE)
				{
					printf("%s%s: %s\n", spaces, current_node->children[iterator]->name, current_node->children[iterator]->string_value);
				}
				else if (current_node->children[iterator]->terminal_type == T_INT)
				{
					printf("%s%s: %d\n", spaces, current_node->children[iterator]->name, current_node->children[iterator]->int_value);
				}
				else if (current_node->children[iterator]->terminal_type == T_FLOAT)
				{
					printf("%s%s: %f\n", spaces, current_node->children[iterator]->name, current_node->children[iterator]->float_value);
				}
				else
				{
					printf("%s%s\n", spaces, current_node->children[iterator]->name);
				}
			}
			else
			{
				print_ast(current_node->children[iterator]);
			}
		}
		
		space_count -= 2;
		spaces[space_count + 1] = 0;
		spaces[space_count] = 0;
	}
	
}

int main(int argc, char** argv)
{
    if (argc > 1)
        if(!(yyin = fopen(argv[1], "r")))
		{
            perror(argv[1]);
            return 1;
        }
#ifdef PRINT_BISON_DEBUG_INFO
    yydebug = 1;
#endif
    yyparse();
	if (!error_flag)
	{
		memset(spaces, 0, 100 * sizeof(char));
		print_ast(root_node);
	}
	return 0;
}
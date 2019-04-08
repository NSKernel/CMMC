/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    main.c
    Main entry
*/

#include <stdio.h>
#include <getopt.h>
#include <stdint.h>

#include <ast.h>
#include <semantics.h>

static const char *opt_string = "vV";
static const struct option long_opts[] = {
    { "verbose", no_argument, NULL, 'v' },
    { "version", no_argument, NULL, 'V' },
    { NULL, no_argument, NULL, 0 }
};

struct global_args_t {
    char print_version;          /* -V or --version */
    char verbose;                /* -v or --verbose */
    char *input_file;
} global_args;

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
    int opt;
    int long_index;

    // parse arguments
    global_args.print_version = 0;
    global_args.verbose = 0;

    opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
    while (opt != -1) {
        switch (opt) {
            case 'V':
              global_args.print_version = 1; /* true */
              break;
            case 'v':
              global_args.verbose = 1;
            default:
              /* You won't actually get here. */
              break;
        }
        opt = getopt_long(argc, argv, opt_string, long_opts, &long_index);
    }
    
    if (global_args.print_version) {
        printf("C-- Compiler Frontend version %s.%s.%s\n", VERSION, SUBVERSION, BUILD);
        printf("Copyright (C) 2019 NSKernel. All rights reserved.\n");
        return 0;
    }

    if (argc - optind > 0) {
        global_args.input_file = argv[optind];
    }
    else {
        printf("Usage: cmmc <file_path>\n");
        return -1;
    }

    if (argc - optind > 1) {
        printf("cmmc: warning: ignoring extra arguments\n");
    }

    if (argc > 1)
        if(!(yyin = fopen(global_args.input_file, "r")))
		{
            printf("cmmc: \033[0;31merror\033[0m: cannot open file %s\n", global_args.input_file);
            return 1;
        }
#ifdef PRINT_BISON_DEBUG_INFO
    yydebug = 1;
#endif
    yyparse();
	if (!error_flag)
	{
		memset(spaces, 0, 100 * sizeof(char));
		//print_ast(root_node);
        sem_validate(root_node);
	}
	return 0;
}
/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    sybol_table.h
    Defines symbol table
*/

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define SYMBOL_T_ERROR         -1
#define SYMBOL_T_INT           0x0
#define SYMBOL_T_FLOAT         0x1
#define SYMBOL_T_STRUCT        0x2
#define SYMBOL_T_STRUCT_DEFINE 0x3

typedef struct symbol_list_t symbol_list;
typedef struct struct_specifier_t struct_specifier;
typedef struct symbol_entry_t symbol_entry;
typedef struct symbol_table_t symbol_table;


struct symbol_list_t {
    symbol_entry *symbol;
    symbol_list *next;
};

struct struct_specifier_t {
    char *struct_tag;
    symbol_list *struct_contents;
};

struct symbol_entry_t {
    char *id;
    char type;
    char is_array;
    char is_function;
    char is_function_dec;
    int *array_size;
    int array_dimention;
    int line_no_def;
    int param_count;
    symbol_list *params;

    struct_specifier *struct_specifier;
};

struct symbol_table_t {
    symbol_entry *symbol;
    int context;
    char do_not_free;
    symbol_table *next;
};

symbol_table *symbol_table_root;

void symtable_init();
int symtable_insert(symbol_entry *symbol, int context, char in_struct, char do_not_free);
void symtable_pop_context(int context);
void symtable_pop_context_without_free(int context);
symbol_entry *symtable_query(char *id);
char symtable_param_struct_compare(symbol_list *sl1, symbol_list *sl2);

void symtable_free_symbol(symbol_entry *se);
void symtable_free_symbol_list(symbol_list *sl);
void _symtable_print();
void symtable_ensure_defined();


#endif
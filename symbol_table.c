/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    sybol_table.c
    Defines symbol table
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <debug.h>
#include <semantics.h>
#include <symbol_table.h>

symbol_list *_symtable_clear_when_exit = NULL;

void _symtable_print();

void symtable_init() {
    symbol_table_root = NULL;
}

char symtable_param_struct_compare(symbol_list *sl1, symbol_list *sl2) {
    while (sl1 != NULL && sl2 != NULL) {
        if (sl1->symbol->type != sl2->symbol->type) 
            return 0;
        if (sl1->symbol->type == SYMBOL_T_STRUCT) {
            if (!symtable_param_struct_compare(sl1->symbol->struct_specifier->struct_contents, sl2->symbol->struct_specifier->struct_contents))
                return 0;
        }
        sl1 = sl1->next;
        sl2 = sl2->next;
    }
    if (sl1 != sl2) // should all be NULL
        return 0;
    return 1;
}

int symtable_insert(symbol_entry *symbol, int context, char in_struct, char do_not_free) {
    symbol_table *new_node;
    symbol_table *iterator = symbol_table_root;

    assert(context >= 0);
    assert(symbol != NULL);
    // check if current symbol exists
    while (iterator != NULL && iterator->context == context) {
        if (!strcmp(iterator->symbol->id, symbol->id)) {
            // found symbol with a same name in the same context

            // if context 0 that could be a redeclear instead of redefine of a function
            if (context == 0 && iterator->symbol->is_function == 1 && symbol->is_function == 1
                && (iterator->symbol->is_function_dec == 1 || symbol->is_function_dec == 1)) {
                // not same type (including struct) or not same params
                if (iterator->symbol->type != symbol->type ||
                    (symbol->type == SYMBOL_T_STRUCT && 
                        !symtable_param_struct_compare(iterator->symbol->struct_specifier->struct_contents, symbol->struct_specifier->struct_contents)) ||
                    !symtable_param_struct_compare(iterator->symbol->params, symbol->params)) {
                    _sem_report_error("Error type 19 at Line %d: Inconsistent declaration of function \"%s\"", symbol->line_no_def, symbol->id);
                    return -1;
                }

            }
            else {
                if (symbol->is_function || symbol->is_function_dec) {
                    _sem_report_error("Error type 4 at Line %d: Redefined symbol \"%s\"", symbol->line_no_def, symbol->id);
                }
                else if (in_struct) {
                    _sem_report_error("Error type 15 at Line %d: Redefined field \"%s\"", symbol->line_no_def, symbol->id);
                }
                else if (symbol->type == SYMBOL_T_STRUCT_DEFINE) {
                    _sem_report_error("Error type 16 at Line %d: Defined a struct with an existing id \"%s\"", symbol->line_no_def, symbol->id);
                }
                else{
                    _sem_report_error("Error type 3 at Line %d: Redefined symbol \"%s\"", symbol->line_no_def, symbol->id);
                }
                return -1;
            }
        }
        iterator = iterator->next;
    }
    
    new_node = malloc(sizeof(symbol_table));
    new_node->symbol = symbol;
    new_node->context = context;
    new_node->do_not_free = do_not_free;
    new_node->next = symbol_table_root;
    symbol_table_root = new_node;
    //_symtable_print();
    return 0;
}

void symtable_free_symbol(symbol_entry *se) {
    if (se->type == SYMBOL_T_STRUCT_DEFINE) {
        free(se->id); // which should be the same char *
                      // with the one in struct_specifier
        symtable_free_symbol_list(se->struct_specifier->struct_contents);
        if (se->struct_specifier->struct_tag[0] <= '9' && se->struct_specifier->struct_tag[0] >= '0') {
            // a name we initialized
            // we should free it
            free(se->struct_specifier->struct_tag);
        }
        free(se->struct_specifier);
    }
    if (se->is_array) {
        free(se->array_size);
    }
    if (se->is_function || se->is_function_dec) {
        symtable_free_symbol_list(se->params);
    }
    free(se);
}

void symtable_free_symbol_list(symbol_list *sl) {
    if (sl != NULL) {
        symtable_free_symbol(sl->symbol);
        symtable_free_symbol_list(sl->next);
        free(sl);
    }
}

void symtable_pop_context(int context) {
    assert((symbol_table_root == NULL) || (context >= symbol_table_root->context));

    symbol_table *iterator = symbol_table_root;
    symbol_list *clear_when_exit_item;

    while (iterator != NULL && iterator->context == context) {
        symbol_table_root = iterator->next;
        if (!iterator->do_not_free) {
            symtable_free_symbol(iterator->symbol);
        }
        else {
            // because there are still structures that is using this
            // struct_specifier information, we store it and release
            // it when exit
            if (iterator->symbol->type == SYMBOL_T_STRUCT_DEFINE) {
                clear_when_exit_item = malloc(sizeof(symbol_list));
                clear_when_exit_item->symbol = iterator->symbol;
                clear_when_exit_item->next = _symtable_clear_when_exit;
                _symtable_clear_when_exit = clear_when_exit_item;
         }
        }
        free(iterator);
        iterator = symbol_table_root;
    }
}

// used for struct_specifier and param_list
void symtable_pop_context_without_free(int context) {
    assert((symbol_table_root == NULL) || (context >= symbol_table_root->context));

    symbol_table *iterator = symbol_table_root;
    symbol_list *clear_when_exit_item;

    while (iterator != NULL && iterator->context == context) {
        symbol_table_root = iterator->next;
        // because there are still structures that is using this
        // struct_specifier information, we store it and release
        // it when exit
        if (iterator->symbol->type == SYMBOL_T_STRUCT_DEFINE) {
            clear_when_exit_item = malloc(sizeof(symbol_list));
            clear_when_exit_item->symbol = iterator->symbol;
            clear_when_exit_item->next = _symtable_clear_when_exit;
            _symtable_clear_when_exit = clear_when_exit_item;
        }
        free(iterator);
        iterator = symbol_table_root;
    }
}
 
symbol_entry *symtable_query(char *id) {
    symbol_table *iterator;
    iterator = symbol_table_root;
    while (iterator != NULL) {
        if (!strcmp(iterator->symbol->id, id)) {
            return iterator->symbol;
        }
        iterator = iterator->next;
    }
    return NULL;
}

void _symtable_print() {
    symbol_table *iterator = symbol_table_root;
    while (iterator != NULL) {
        printf("%s, %d->", iterator->symbol->id, iterator->context);
        iterator = iterator->next;
    }
    printf("\n");
}

void symtable_ensure_defined() {
    symbol_table *iterator = symbol_table_root;
    symbol_table *iterator2;
    while (iterator != NULL) {
        if (iterator->symbol->is_function) {
            if (iterator->symbol->is_function_dec) {
                iterator2 = iterator->next;
                while (iterator2 != NULL) {
                    if (!strcmp(iterator2->symbol->id, iterator->symbol->id)) {
                        if (!iterator2->symbol->is_function_dec) {
                            iterator->symbol->is_function_dec = 0;
                            break;
                        }
                    }
                    iterator2 = iterator2->next;
                }
                if (iterator->symbol->is_function_dec) {
                    // undefined function
                    _sem_report_error("Error type 18 at Line %d: Undefined function \"%s\"", iterator->symbol->line_no_def, iterator->symbol->id);
                }
            }
            else {
                iterator2 = iterator->next;
                while (iterator2 != NULL) {
                    if (!strcmp(iterator2->symbol->id, iterator->symbol->id)) {
                        iterator2->symbol->is_function_dec = 0;
                    }
                    iterator2 = iterator2->next;
                }
            }
        }
        iterator = iterator->next;
    }
}
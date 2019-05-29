/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    semantics_struct_helper.c
    Defines semantics validation for struct specifier
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <debug.h>
#include <ast.h>
#include <symbol_table.h>
#include <semantics.h>

symbol_list *_sem_validate_def_list_for_struct(ast_node *node, int context);
symbol_list *_sem_validate_def_for_struct(ast_node *node, int context);
symbol_list *_sem_validate_dec_list_for_struct(ast_node *node, int type, struct_specifier *struct_specifier, int context);
symbol_entry *_sem_validate_dec_for_struct(ast_node *node, int type, struct_specifier *struct_specifier);

extern int _sem_validate_specifier(ast_node *node, struct_specifier **struct_specifier, int context, int do_not_free);
extern symbol_entry *_sem_validate_var_dec(ast_node *node);

symbol_list *_sem_validate_def_list_for_struct(ast_node *node, int context) {
    symbol_list *iterator;

    if (node == NULL) {
        return NULL;
    }
    symbol_list *ret_list_front = _sem_validate_def_for_struct(node->children[0], context);
    if ((int)(ret_list_front) == -1) { // an error occoured
        ret_list_front = (void*)(-1);
    }
    symbol_list *ret_list_tail = _sem_validate_def_list_for_struct(node->children[1], context);
    if ((int)(ret_list_tail) == -1 || (int)(ret_list_front) == -1) { // an error occoured
        // clean up list front
        if ((int)(ret_list_front) != -1)
            symtable_free_symbol_list(ret_list_front);
        return (void*)(-1);
    }
    // cannot be empty!
    // if (ret_list_front == NULL)
    //     return ret_list_tail;
    iterator = ret_list_front;
    while (iterator->next != NULL)
        iterator = iterator->next;
    iterator->next = ret_list_tail;
    return ret_list_front;
}

// return the parsed def_list; return -1 on error;
symbol_list *_sem_validate_def_for_struct(ast_node *node, int context) {
    struct_specifier *struct_specifier = NULL;
    symbol_list *ret_list;
    int type = _sem_validate_specifier(node->children[0], &struct_specifier, context, 1);
    if (type == SYMBOL_T_ERROR) {
        // specifier validation failed with error
        return (void*)(-1);
    }

    ret_list = _sem_validate_dec_list_for_struct(node->children[1], type, struct_specifier, context);
    if ((int)(ret_list) == -1) {
        // why not free the struct_specifier?
        // because the struct specifier has been commited
        // into the symbol table and will be later
        // add into clear_when_exit list
        return (void*)(-1);
    }

    return ret_list;
}

symbol_list *_sem_validate_dec_list_for_struct(ast_node *node, int type, struct_specifier *struct_specifier, int context) {
    symbol_list *ret_list = malloc(sizeof(symbol_list));
    symbol_entry *ins_entry;
    symbol_list *ret_list_tail;
    if (node->children_count == 1) { // Dec
        ins_entry = _sem_validate_dec_for_struct(node->children[0], type, struct_specifier);
        if (ins_entry == NULL) {
            free(ret_list);
            return (void*)(-1);
        }
        if (symtable_insert(ins_entry, context, 1, 1, 0)) {
            // Falied to insert
            symtable_free_symbol(ins_entry);
            free(ret_list);
            return (void*)(-1);
        }
        ret_list->symbol = ins_entry;
        ret_list->next = NULL;
        return ret_list;
    }
    else if (node->children_count == 3) { // Dec COMMA DecList
        ins_entry = _sem_validate_dec_for_struct(node->children[0], type, struct_specifier);
        if (ins_entry != NULL) {
            if (symtable_insert(ins_entry, context, 1, 1, 0)) {
                // Falied to insert
                symtable_free_symbol(ins_entry);
                ins_entry = NULL;
            }
            else {
                ret_list->symbol = ins_entry;
                ret_list->next = NULL;
            }
        }
        ret_list_tail = _sem_validate_dec_list_for_struct(node->children[2], type, struct_specifier, context);
        if ((int)(ret_list_tail) == -1 || ins_entry == NULL) {
            if (ins_entry != NULL)
                symtable_free_symbol(ins_entry);
            free(ret_list);
            return (void*)(-1);
        }
        ret_list->next = ret_list_tail;
        return ret_list;
    }
    else {
        assert(0);
        return (void*)(-1);
    }
}

symbol_entry *_sem_validate_dec_for_struct(ast_node *node, int type, struct_specifier *struct_specifier) {
    int i = 0;
    
    if (node->children_count == 3) { // VarDec ASSIGNOP Exp
        // NOT ALLOWED!
        _sem_report_error("Error type 15 at Line %d: Attempt to assign a field", node->children[1]->line_number);
        return NULL;
    }
    
    symbol_entry *ret_entry = _sem_validate_var_dec(node->children[0]);
    ret_entry->type = type;
    if (type == SYMBOL_T_INT) {
        ret_entry->size = 4;
    }
    if (type == SYMBOL_T_FLOAT) {
        ret_entry->size = 8;
    }
    if (type == SYMBOL_T_STRUCT) {
        ret_entry->size = struct_specifier->size;
    }
    ret_entry->struct_specifier = struct_specifier;

    if (ret_entry->is_array) {
        for (i = 0; i < ret_entry->array_dimention; i++) {
            ret_entry->size *= ret_entry->array_size[i];
        }
    }

    return ret_entry;
}

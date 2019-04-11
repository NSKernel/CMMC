/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    semantics_exp_helper.c
    Defines semantics validation for expression
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <debug.h>
#include <ast.h>
#include <symbol_table.h>
#include <semantics.h>

char _sem_type_matching(_sem_exp_type *t1, _sem_exp_type *t2);
_sem_exp_type *_sem_search_id_in_struct(char *id, struct_specifier *specifier);
_sem_exp_type *_sem_validate_exp(ast_node *node);
_sem_exp_type_list *_sem_validate_args(ast_node *node);
char _sem_compare_args_params(_sem_exp_type_list *tl, symbol_list *sl);

char _sem_type_matching(_sem_exp_type *t1, _sem_exp_type *t2) {
    char ret_val;
    ret_val = (t1->type == t2->type);
    if (t1->type == SYMBOL_T_STRUCT) {
        ret_val = (ret_val && symtable_param_struct_compare(t1->struct_specifier->struct_contents, t2->struct_specifier->struct_contents));
    }
    ret_val = (ret_val && (t1->is_array == t2->is_array));
    if (t1->is_array) {
        ret_val = (ret_val && t2->is_array);
        ret_val = (ret_val && (t1->array_dimension == t2->array_dimension));
    }

    return ret_val;
}

_sem_exp_type *_sem_search_id_in_struct(char *id, struct_specifier *specifier) {
    symbol_list *iterator = specifier->struct_contents;
    _sem_exp_type *ret_type;
    while (iterator != NULL) {
        if (!strcmp(id, iterator->symbol->id)) {
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = iterator->symbol->type;
            ret_type->is_array = iterator->symbol->is_array;
            ret_type->array_dimension = iterator->symbol->array_dimention;
            ret_type->struct_specifier = iterator->symbol->struct_specifier;
            ret_type->is_lvalue = 1;
            return ret_type;
        }
        iterator = iterator->next;
    }
    return NULL;
}

// returns type and set *expected_struct_specifier to the expected
// struct specifier of the expression
_sem_exp_type *_sem_validate_exp(ast_node *node) {
    _sem_exp_type *type_1;
    _sem_exp_type *type_2;
    _sem_exp_type *ret_type;
    symbol_entry *symbol;
    _sem_exp_type_list *args_list;
    _sem_exp_type_list *free_iterator;

    if (!strcmp(node->children[0]->name, "Exp")) {
        // Exp ASSIGNOP Exp
        // Exp AND Exp
        // Exp OR Exp
        // Exp RELOP Exp
        // Exp PLUS Exp
        // Exp MINUS Exp
        // Exp STAR Exp
        // Exp DIV Exp
        // Exp LB Exp RB
        // Exp DOT ID
        type_1 = _sem_validate_exp(node->children[0]);
        if (type_1 == NULL)
            return NULL;

        if (!strcmp(node->children[1]->name, "ASSIGNOP")) {
            if (!(type_1->is_lvalue)) {
                _sem_report_error("Error type 6 at Line %d: The left-hand side of an assignment must be a variable", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
            type_2 = _sem_validate_exp(node->children[2]);
            if (type_2 == NULL) {
                free(type_1);
                return NULL;
            }
            if (!_sem_type_matching(type_1, type_2)) {
                _sem_report_error("Error type 5 at Line %d: Type mismatched for assignment", node->children[1]->line_number);
                free(type_1);
                free(type_2);
                return NULL;
            }
            type_1->is_lvalue = 0;
            return type_1;
        }
        if (!strcmp(node->children[1]->name, "LB")) {
            if (!(type_1->is_array)) {
                _sem_report_error("Error type 10 at Line %d: Not an array", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
            type_2 = _sem_validate_exp(node->children[2]);
            if (type_2 == NULL) {
                free(type_1);
                return NULL;
            }
            if (type_2->type != SYMBOL_T_INT || type_2->is_array) {
                _sem_report_error("Error type 12 at Line %d: Not an integer for the subscription of an array", node->children[1]->line_number);
                free(type_1);
                free(type_2);
                return NULL;
            }
            ret_type = type_1;
            // WRONG!
            // ret_type->is_lvalue = 1;
            // Counter example:
            // struct a func() {int x[2];...};
            // then func().x[1] is not a lvalue

            ret_type->array_dimension -= 1;
            if (ret_type->array_dimension)
                ret_type->is_array = 1;
            else
                ret_type->is_array = 0;
            free(type_2);
            return ret_type;
        }
        if (!strcmp(node->children[1]->name, "DOT")) {
            if (type_1->type != SYMBOL_T_STRUCT || (type_1->is_array)) {
                _sem_report_error("Error type 13 at Line %d: Illegal use of \".\"", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
            type_2 = _sem_search_id_in_struct(node->children[2]->string_value, type_1->struct_specifier);
            if (type_2 == NULL) {
                _sem_report_error("Error type 14 at Line %d: Non-existent field \"%s\"", node->children[1]->line_number, node->children[2]->string_value);
                free(type_1);
                return NULL;
            }
            free(type_1);
            // WRONG!
            // type_2->is_lvalue = 1;
            // Counter example:
            // struct a func() {...};
            // then func().x is not a lvalue
            if (type_1->is_lvalue)
                type_2->is_lvalue = 1;
            else 
                type_2->is_lvalue = 0;
            return type_2;
        }
        // else Exp XXX Exp
        if (!strcmp(node->children[1]->name, "AND") || !strcmp(node->children[1]->name, "OR")) {
            if (type_1->type != SYMBOL_T_INT) {
                _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT expected.", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
        }
        if ((type_1->type != SYMBOL_T_INT && type_1->type != SYMBOL_T_FLOAT) || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT/FLOAT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }
        type_2 = _sem_validate_exp(node->children[2]);
        if (type_2 == NULL) {
            free(type_1);
            return NULL;
        }
        if (!_sem_type_matching(type_1, type_2)) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operands.", node->children[1]->line_number);
            free(type_1);
            free(type_2);
            return NULL;
        }
        if (!strcmp(node->children[1]->name, "RELOP")) {
            type_1->type = SYMBOL_T_INT;
            type_1->is_array = 0;
            type_1->array_dimension = 0;
            type_1->is_lvalue = 0;
            type_1->struct_specifier = NULL;
        }
        free(type_2);
        type_1->is_lvalue = 0;
        return type_1;
    }
    else if (!strcmp(node->children[0]->name, "LP")) {
        // LP Exp RP
        return _sem_validate_exp(node->children[1]);
    }
    else if (!strcmp(node->children[0]->name, "MINUS")) {
        // MINUS Exp
        type_1 = _sem_validate_exp(node->children[1]);
        if (type_1 == NULL) {
            return NULL;
        }

        if ((type_1->type != SYMBOL_T_INT && type_1->type != SYMBOL_T_FLOAT) || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT/FLOAT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }
        type_1->is_lvalue = 0;
        return type_1;        
    }
    else if (!strcmp(node->children[0]->name, "NOT")) {
        // NOT Exp
        type_1 = _sem_validate_exp(node->children[1]);
        if (type_1 == NULL) {
            return NULL;
        }

        if (type_1->type != SYMBOL_T_INT || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }
        type_1->is_lvalue = 0;
        return type_1;
    }
    else if (!strcmp(node->children[0]->name, "ID")) {
        // ID
        // ID LP Args RP
        // ID LP RP
        symbol = symtable_query(node->children[0]->string_value);
        if (symbol == NULL) {
            if (node->children_count == 1) {
                _sem_report_error("Error type 1 at Line %d: Undefined \"%s\".", node->children[0]->line_number, node->children[0]->string_value);
                return NULL;
            }
            else {
                _sem_report_error("Error type 2 at Line %d: Undefined \"%s\".", node->children[0]->line_number, node->children[0]->string_value);
                return NULL;
            }
        }

        if (node->children_count == 1) {
            // ID
            if (symbol->is_function) {
                _sem_report_error("Error type 7 at Line %d: Unexpected function identifier \"%s\".", node->children[0]->line_number, node->children[0]->string_value);
                return NULL;
            }
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = symbol->type;
            ret_type->is_array = symbol->is_array;
            ret_type->array_dimension = symbol->array_dimention;
            ret_type->is_lvalue = 1;
            ret_type->struct_specifier = symbol->struct_specifier;
            return ret_type;
        }

        if (node->children_count == 3) {
            // ID LP RP
            if (!symbol->is_function) {
                _sem_report_error("Error type 11 at Line %d: Not a function.", node->children[0]->line_number);
                return NULL;
            }
            if (symbol->param_count != 0) {
                _sem_report_error("Error type 9 at Line %d: Function \"%s\"\'s parameters mismatch.", node->children[0]->line_number, node->children[0]->string_value);
                return NULL;
            }
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = symbol->type;
            ret_type->is_array = symbol->is_array;
            ret_type->array_dimension = symbol->array_dimention;
            ret_type->is_lvalue = 0;
            ret_type->struct_specifier = symbol->struct_specifier;
            return ret_type;
        }

        if (node->children_count == 4) {
            // ID LP Args RP
            if (!symbol->is_function) {
                _sem_report_error("Error type 11 at Line %d: Not a function.", node->children[0]->line_number);
                return NULL;
            }
            args_list = _sem_validate_args(node->children[2]);
            if (args_list == NULL) {
                return NULL;
            }
            if (!_sem_compare_args_params(args_list, symbol->params)) {
                free_iterator = args_list;
                while (free_iterator != NULL) {
                    args_list = args_list->next;
                    free(free_iterator);
                    free_iterator = args_list;
                }
                _sem_report_error("Error type 9 at Line %d: Function \"%s\"\'s parameters mismatch.", node->children[2]->line_number, node->children[0]->string_value);
                return NULL;
            }
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = symbol->type;
            ret_type->is_array = symbol->is_array;
            ret_type->array_dimension = symbol->array_dimention;
            ret_type->is_lvalue = 0;
            ret_type->struct_specifier = symbol->struct_specifier;
            return ret_type;
        }
    }
    else if (!strcmp(node->children[0]->name, "INT")) {
        ret_type = malloc(sizeof(_sem_exp_type));
        ret_type->type = SYMBOL_T_INT;
        ret_type->is_array = 0;
        ret_type->array_dimension = 0;
        ret_type->is_lvalue = 0;
        ret_type->struct_specifier = NULL;
        return ret_type;
    }
    else if (!strcmp(node->children[0]->name, "FLOAT")) {
        ret_type = malloc(sizeof(_sem_exp_type));
        ret_type->type = SYMBOL_T_FLOAT;
        ret_type->is_array = 0;
        ret_type->array_dimension = 0;
        ret_type->is_lvalue = 0;
        ret_type->struct_specifier = NULL;
        return ret_type;
    }
    assert(0);
    return NULL;
}

_sem_exp_type_list *_sem_validate_args(ast_node *node) {
    _sem_exp_type *current_type;
    _sem_exp_type_list *ret_list;
    _sem_exp_type_list *ret_list_tail;

    current_type = _sem_validate_exp(node->children[0]);
    if (current_type == NULL) {
        return NULL;
    }
    ret_list = malloc(sizeof(_sem_exp_type_list));
    ret_list->type = current_type;
    ret_list->next = NULL;
    
    if (node->children_count == 1) {
        // Exp
        return ret_list;
    }
    else if (node->children_count == 3) {
        // Exp COMMA Args
        ret_list_tail = _sem_validate_args(node->children[2]);
        if (ret_list_tail == NULL) {
            free(current_type);
            free(ret_list);
            return NULL;
        }
        ret_list->next = ret_list_tail;
        return ret_list;
    }
    else {
        assert(0);
        return NULL;
    }
}

char _sem_compare_args_params(_sem_exp_type_list *tl, symbol_list *sl) {
    while (tl != NULL) {
        if (sl == NULL) {
            return 0;
        }
        if (tl->type->type != sl->symbol->type) {
            return 0;
        }
        if (tl->type->is_array != sl->symbol->is_array) {
            return 0;
        }
        if (tl->type->is_array) {
            if (tl->type->array_dimension != sl->symbol->array_dimention)
                return 0;
        }
        if (tl->type->type == SYMBOL_T_STRUCT) {
            if (!symtable_param_struct_compare(tl->type->struct_specifier->struct_contents, sl->symbol->struct_specifier->struct_contents)) {
                return 0;
            }
        }
        tl = tl->next;
        sl = sl->next;
    }
    if (sl != NULL)
        return 0;
    return 1;
}
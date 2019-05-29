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
#include <ir.h>
#include <global.h>

char _sem_type_matching(_sem_exp_type *t1, _sem_exp_type *t2);
_sem_exp_type *_sem_search_id_in_struct(char *id, struct_specifier *specifier);
_sem_exp_type *_sem_validate_exp(ast_node *node, char no_optimization, ir_list *ret_ir);
_sem_exp_type_list *_sem_validate_args(ast_node *node, char no_optimization, ir_list *ret_ir);
char _sem_compare_args_params(_sem_exp_type_list *tl, symbol_list *sl);
void _sem_add_args(_sem_exp_type_list *tl, ir_list *ret_ir);

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

// not needed!
// we can generate t = 0
//                 IF a relop b THEN GOTO z
//                 t = 1
//                 LABEL z
// at the back end
// char _sem_generate_relop_ir()

_sem_exp_type *_sem_search_id_in_struct(char *id, struct_specifier *specifier) {
    symbol_list *iterator = specifier->struct_contents;
    uint32_t offset = 0;
    _sem_exp_type *ret_type;
    while (iterator != NULL) {
        if (!strcmp(id, iterator->symbol->id)) {
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->constant_exp_status = SEM_CONSTANT_NO;
            ret_type->type = iterator->symbol->type;
            ret_type->is_array = iterator->symbol->is_array;
            ret_type->array_size = iterator->symbol->array_size;
            ret_type->array_accumulated_size = iterator->symbol->size;
            ret_type->array_dimension = iterator->symbol->array_dimention;
            ret_type->struct_specifier = iterator->symbol->struct_specifier;
            ret_type->is_lvalue = 1;
            ret_type->offset_in_struct = offset;
            return ret_type;
        }
        offset += iterator->symbol->size;
        iterator = iterator->next;
    }
    return NULL;
}

// returns type and set *expected_struct_specifier to the expected
// struct specifier of the expression
_sem_exp_type *_sem_validate_exp(ast_node *node, char no_optimization, ir_list *ret_ir) {
    _sem_exp_type *type_1;
    _sem_exp_type *type_2;
    _sem_exp_type *ret_type;
    symbol_entry *symbol;
    _sem_exp_type_list *args_list;
    _sem_exp_type_list *free_iterator;
    ir *ir_entry;
    uint32_t temp_var_reg;
    uint32_t size_multiplier;
    uint32_t goto_label;
    uint32_t goto_label_end;
    char is_and;
    ir_list *ir_list_local;
    ir_list *ir_list_local2;

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
        type_1 = _sem_validate_exp(node->children[0], no_optimization, ret_ir);
        if (type_1 == NULL)
            return NULL;

        if (!strcmp(node->children[1]->name, "ASSIGNOP")) {
            if (!(type_1->is_lvalue)) {
                _sem_report_error("Error type 6 at Line %d: The left-hand side of an assignment must be a variable", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
            type_2 = _sem_validate_exp(node->children[2], no_optimization, ret_ir);
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
            if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                // MACCESS
                assert(type_1->immediate_ir != NULL && type_1->immediate_ir->op == IR_EXP_OP_MACCESS);
                assert(type_1->immediate_ir->mode.mode3 == IR_MODE_I);
                type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ret_ir);
                if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE && type_1->immediate_ir->mode.op2 != IR_MODE_STAR) {
                    if (type_2->immediate_ir->op == IR_EXP_OP_MACCESS) { 
                        type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                        type_2->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    }
                    
                    type_2->immediate_ir->mode.mode1 = type_1->immediate_ir->mode.mode2;
                    type_2->immediate_ir->mode.op1 = type_1->immediate_ir->mode.op2; // access memory
                    type_2->immediate_ir->temp_id = type_1->immediate_ir->temp_id1;
                    type_2->immediate_ir->var_id = type_1->immediate_ir->var_id1;
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);
                    free(type_1->immediate_ir);

                    // modify type_1 for return
                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                    type_1->ir_var_id = type_2->immediate_ir->var_id;
                    type_1->ir_temp_val_id = type_2->immediate_ir->temp_id;
                    if (type_2->immediate_ir->mode.mode1 == IR_MODE_T) {
                        type_1->type_mode = SEM_TYPE_MODE_T;
                    }
                    else {
                        type_1->type_mode = SEM_TYPE_MODE_V;
                    }
                    if (type_2->immediate_ir->mode.op1 == IR_MODE_ADDR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_ADDR;
                    }
                    else if (type_2->immediate_ir->mode.op1 == IR_MODE_NORMAL) {
                        type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                    }
                    else if (type_2->immediate_ir->mode.op1 == IR_MODE_STAR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_STAR;
                    }
                }
                else if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                    if (type_2->immediate_ir->op == IR_EXP_OP_MACCESS) { 
                        type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                        type_2->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    }

                    // == star
                    // must commit type_2 as a single ir
                    type_2->immediate_ir->temp_id = ir_new_temp_val();
                    type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);

                    type_1->constant_exp_status = SEM_CONSTANT_NO;;
                    type_1->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    type_1->immediate_ir->mode.mode1 = type_1->immediate_ir->mode.mode2;
                    type_1->immediate_ir->mode.op1 = type_1->immediate_ir->mode.op2;
                    type_1->immediate_ir->temp_id = type_1->immediate_ir->temp_id1;
                    type_1->immediate_ir->var_id = type_1->immediate_ir->var_id1;
                    type_1->immediate_ir->temp_id1 = type_2->immediate_ir->temp_id;
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);

                    // modify type_1 for return
                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                    type_1->ir_var_id = type_1->immediate_ir->var_id;
                    type_1->ir_temp_val_id = type_1->immediate_ir->temp_id;
                    if (type_1->immediate_ir->mode.mode1 == IR_MODE_T) {
                        type_1->type_mode = SEM_TYPE_MODE_T;
                    }
                    else {
                        type_1->type_mode = SEM_TYPE_MODE_V;
                    }
                    if (type_1->immediate_ir->mode.op1 == IR_MODE_ADDR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_ADDR;
                    }
                    else if (type_1->immediate_ir->mode.op1 == IR_MODE_NORMAL) {
                        type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                    }
                    else if (type_1->immediate_ir->mode.op1 == IR_MODE_STAR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_STAR;
                    }
                }
                else if(type_2->constant_exp_status == SEM_CONSTANT_YES) {
                    type_1->immediate_ir->mode.mode1 = type_1->immediate_ir->mode.mode2;
                    type_1->immediate_ir->mode.op1 = type_1->immediate_ir->mode.op2;
                    type_1->immediate_ir->temp_id = type_1->immediate_ir->temp_id1;
                    type_1->immediate_ir->var_id = type_1->immediate_ir->var_id1;
                    type_1->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    if (type_2->type == SYMBOL_T_FLOAT) {
                        type_1->immediate_ir->float_val1 = type_2->float_val;
                        type_1->float_val = type_2->float_val;
                    }
                    else {
                        type_1->immediate_ir->int_val1 = type_2->int_val;
                        type_1->int_val = type_2->int_val;
                    }
                    type_1->immediate_ir->mode.mode2 = IR_MODE_I;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);

                    // modify type_1 for return
                    // constat values have been set in the if (type_1->type == SYMBOL_T_FLOAT) part
                    type_1->constant_exp_status = SEM_CONSTANT_YES;
                }
                else {
                    // non constant
                    type_1->constant_exp_status = type_2->constant_exp_status;
                    type_1->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    type_1->immediate_ir->mode.mode1 = type_1->immediate_ir->mode.mode2;
                    type_1->immediate_ir->mode.op1 = type_1->immediate_ir->mode.op2;
                    type_1->immediate_ir->temp_id = type_1->immediate_ir->temp_id1;
                    type_1->immediate_ir->var_id = type_1->immediate_ir->var_id1;
                    if (type_2->type_mode == SEM_TYPE_MODE_T) {
                        type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                    }
                    else if (type_2->type_mode == SEM_TYPE_MODE_V) {
                        type_1->immediate_ir->mode.mode2 = IR_MODE_V;
                    }
                    if (type_2->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                        type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    }
                    else if (type_2->type_mode_op == SEM_TYPE_MODE_STAR) {
                        type_1->immediate_ir->mode.op2 = IR_MODE_STAR;
                    }
                    else if (type_2->type_mode_op == SEM_TYPE_MODE_ADDR) {
                        type_1->immediate_ir->mode.op2 = IR_MODE_ADDR;
                    }
                    type_1->immediate_ir->temp_id1 = type_2->ir_temp_val_id;
                    type_1->immediate_ir->var_id1 = type_2->ir_var_id;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);

                    // modify type_1 for return
                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                    type_1->ir_var_id = type_1->immediate_ir->var_id;
                    type_1->ir_temp_val_id = type_1->immediate_ir->temp_id;
                    if (type_1->immediate_ir->mode.mode1 == IR_MODE_T) {
                        type_1->type_mode = SEM_TYPE_MODE_T;
                    }
                    else {
                        type_1->type_mode = SEM_TYPE_MODE_V;
                    }
                    if (type_1->immediate_ir->mode.op1 == IR_MODE_ADDR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_ADDR;
                    }
                    else if (type_1->immediate_ir->mode.op1 == IR_MODE_NORMAL) {
                        type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                    }
                    else if (type_1->immediate_ir->mode.op1 == IR_MODE_STAR) {
                        type_1->type_mode_op = SEM_TYPE_MODE_STAR;
                    }
                }
            }
            else {
                // can be constant but yet still have variable information!
                // for example
                // if x is a constant
                // then the x = y + 1 will report x as constant, but we will still add
                // x's variable id in type_1
                // but must be a single variable!
                assert(type_1->type_mode == SEM_TYPE_MODE_V);
                assert(type_1->type_mode_op == SEM_TYPE_MODE_NORMAL);

                if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE && type_1->type_mode_op != SEM_TYPE_MODE_STAR) {
                    if (type_2->immediate_ir->op == IR_EXP_OP_MACCESS) { 
                        type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                        type_2->immediate_ir->op = IR_EXP_OP_ASSIGN;
                    }

                    type_2->immediate_ir->mode.mode1 = IR_MODE_V;
                    // must be normal and will never be temp var
                    // lvalue could only be a variable, a struct access or an array access
                    // struct and array access will retrun an MACCESS immediate ir
                    // thus anything that comes here must be a single variable with normal
                    // access operation mode
                    type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    type_2->immediate_ir->var_id = type_1->ir_var_id;     // <-+
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);  //   |  
                                                                          //   |
                    // modify type_1 for return                                |
                    type_1->constant_exp_status = SEM_CONSTANT_NO;        //   |
                    // type_1->ir_var_id = type_2->immediate_ir->var_id; ------+
                    type_1->type_mode = SEM_TYPE_MODE_V;
                    type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                    type_1->var_symbol->is_constant = IR_NON_CONSTANT;
                }
                else if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                    // commit type_2
                    type_2->immediate_ir->temp_id = ir_new_temp_val();
                    type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);

                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                    type_1->immediate_ir = malloc(sizeof(ir));
                    type_1->immediate_ir->op = IR_EXP_OP_ASSIGN;

                    if (type_1->type_mode == SEM_TYPE_MODE_T) {
                        type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                    }
                    else {
                        type_1->immediate_ir->mode.mode1 = IR_MODE_V;
                    }
                    if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                        type_1->immediate_ir->mode.op1 = IR_MODE_ADDR;
                    }
                    else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                        type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    }
                    else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                        type_1->immediate_ir->mode.op1 = IR_MODE_STAR;
                    }

                    type_1->immediate_ir->temp_id = type_1->ir_temp_val_id;
                    type_1->immediate_ir->var_id = type_1->ir_var_id;
                    type_1->immediate_ir->temp_id1 = type_2->immediate_ir->temp_id;
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);

                    // modify type_1 for return
                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                }
                else if(type_2->constant_exp_status == SEM_CONSTANT_YES) {
                    ir_entry = malloc(sizeof(ir));
                    ir_entry->op = IR_EXP_OP_ASSIGN;
                    ir_entry->var_id = type_1->ir_var_id;
                    ir_entry->mode.mode1 = IR_MODE_V;
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                    if (type_2->type == SYMBOL_T_FLOAT) {
                        ir_entry->float_val1 = type_2->float_val;
                        type_1->float_val = type_2->float_val;
                        type_1->var_symbol->float_val = type_2->float_val;
                    }
                    else {
                        ir_entry->int_val1 = type_2->int_val;
                        type_1->int_val = type_2->int_val;
                        type_1->var_symbol->int_val = type_2->int_val;
                    }
                    ir_entry->mode.mode2 = IR_MODE_I;
                    ir_entry->mode.op2 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, ir_entry);

                    type_1->constant_exp_status = SEM_CONSTANT_YES;
                    if (no_optimization) {
                        type_1->var_symbol->is_constant = IR_NON_CONSTANT;
                    }
                    else {
                        type_1->var_symbol->is_constant = IR_CONSTANT;
                    }
                }
                else {
                    // non constant
                    ir_entry = malloc(sizeof(ir));
                    ir_entry->op = IR_EXP_OP_ASSIGN;
                    ir_entry->var_id = type_1->ir_var_id;
                    ir_entry->mode.mode1 = IR_MODE_V;
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                    ir_entry->temp_id1 = type_2->ir_temp_val_id;
                    ir_entry->var_id1 = type_2->ir_var_id;
                    if (type_2->type_mode == SEM_TYPE_MODE_T) {
                        ir_entry->mode.mode2 = IR_MODE_T;
                    }
                    else if (type_2->type_mode == SEM_TYPE_MODE_V) {
                        ir_entry->mode.mode2 = IR_MODE_V;
                    }
                    if (type_2->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                        ir_entry->mode.op2 = IR_MODE_NORMAL;
                    }
                    else if (type_2->type_mode_op == SEM_TYPE_MODE_STAR) {
                        ir_entry->mode.op2 = IR_MODE_STAR;
                    }
                    else if (type_2->type_mode_op == SEM_TYPE_MODE_ADDR) {
                        ir_entry->mode.op2 = IR_MODE_ADDR;
                    }
                    ir_add_node_to_buffer(ret_ir, ir_entry);
                    
                    // modify type_1 for return
                    type_1->constant_exp_status = SEM_CONSTANT_NO;
                    type_1->type_mode = SEM_TYPE_MODE_V;
                    type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                    type_1->var_symbol->is_constant = IR_NON_CONSTANT;
                }
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
            type_2 = _sem_validate_exp(node->children[2], no_optimization, ret_ir);
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
            ret_type->array_accumulated_size /= ret_type->array_size[ret_type->array_dimension];

            // solve type 1 as the base address for IR_EXP_OP_MACCESS
            // We actually turn type_1 and type_2 into two IMMEIDATE irs
            // in the form of IR_EXP_OP_MACCESS
            // type1:     x + a
            // type2:     y or b
            // where x and y are those with variables and a and b are constants
            // then we have the type1[type2] as (x + y) + a or x + (a + b)

            // turn type_1 into MACCESS
            if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                if (type_1->immediate_ir->op != IR_EXP_OP_MACCESS) {
                    // commit the code and turn into t1 + 0
                    // I think this case won't happen...
                    type_1->immediate_ir->temp_id = ir_new_temp_val();
                    temp_var_reg = type_1->immediate_ir->temp_id;
                    type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                    type_1->immediate_ir = malloc(sizeof(ir));
                    type_1->immediate_ir->op = IR_EXP_OP_MACCESS;
                    type_1->immediate_ir->temp_id1 = temp_var_reg;
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    type_1->immediate_ir->int_val2 = 0;
                    type_1->immediate_ir->mode.mode3 = IR_MODE_I;
                    type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                }
            }
            // type_1 should simply not be constant
            // MACCESS is used in struct access and array access where
            // struct and array are defined dynamically thus will never
            // be constant
            else {
                assert(type_1->constant_exp_status != SEM_CONSTANT_YES);
                type_1->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
                type_1->immediate_ir = malloc(sizeof(ir));
                type_1->immediate_ir->op = IR_EXP_OP_MACCESS;
                type_1->immediate_ir->temp_id1 = type_1->ir_temp_val_id;
                type_1->immediate_ir->var_id1 = type_1->ir_var_id;
                if (type_1->type_mode == SEM_TYPE_MODE_T) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                }
                else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_V;
                }
                if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_ADDR;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_STAR;
                }
                type_1->immediate_ir->int_val2 = 0;
                type_1->immediate_ir->mode.mode3 = IR_MODE_I;
                type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
            }
            
            // turn type_2 into MACCESS (y + 0 or 0 + b)
            // and directly combines type_1 and type_2
            // we first set the size multiplier
            // then we do (x + y * size_multiplier) + a or
            // x + (a + b * size_multiplier)
            size_multiplier = ret_type->array_accumulated_size;

            if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                // y + 0
                if (type_2->immediate_ir->op == IR_EXP_OP_MACCESS) {
                    type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                }
                else {
                    type_2->immediate_ir->temp_id = ir_new_temp_val();
                    temp_var_reg = type_2->immediate_ir->temp_id;
                    type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);
                    type_2->immediate_ir = malloc(sizeof(ir));
                    type_2->immediate_ir->temp_id1 = temp_var_reg;
                    type_2->immediate_ir->mode.mode2 = IR_MODE_T;
                    type_2->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
                // build y * size_multiplier
                type_2->immediate_ir->op = IR_EXP_OP_MUL;
                type_2->immediate_ir->int_val2 = size_multiplier;
                type_2->immediate_ir->mode.mode3 = IR_MODE_I;
                type_2->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                type_2->immediate_ir->temp_id = ir_new_temp_val();
                type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);

                // build (x + y * size_multiplier)
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_EXP_OP_ADD;
                ir_entry->temp_id1 = type_1->immediate_ir->temp_id1;
                ir_entry->var_id1 = type_1->immediate_ir->var_id1;
                ir_entry->mode.mode2 = type_1->immediate_ir->mode.mode2;
                ir_entry->mode.op2 = type_1->immediate_ir->mode.op2;
                ir_entry->temp_id2 = type_2->immediate_ir->temp_id;
                ir_entry->mode.mode3 = type_2->immediate_ir->mode.mode1;
                ir_entry->mode.op3 = type_2->immediate_ir->mode.op1;
                ir_entry->temp_id = ir_new_temp_val();
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);

                // replace x in type_1 with (x + y * size_multiplier)
                type_1->immediate_ir->temp_id1 = ir_entry->temp_id;
                type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
            }
            else if (type_2->constant_exp_status == SEM_CONSTANT_YES) {
                // 0 + b * size_multiplier
                type_1->immediate_ir->int_val2 += type_2->int_val * size_multiplier;
                // it's done!
            }
            else {
                // non constant
                // build y * size_multiplier
                type_2->immediate_ir = malloc(sizeof(ir));
                type_2->immediate_ir->op = IR_EXP_OP_MUL;
                type_2->immediate_ir->int_val2 = size_multiplier;
                type_2->immediate_ir->mode.mode3 = IR_MODE_I;
                type_2->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                type_2->immediate_ir->temp_id = ir_new_temp_val();
                type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                type_2->immediate_ir->temp_id1 = type_2->ir_temp_val_id;
                type_2->immediate_ir->var_id1 = type_2->ir_var_id;
                if (type_2->type_mode == SEM_TYPE_MODE_V) {
                    type_2->immediate_ir->mode.mode2 = IR_MODE_V;
                }
                else if (type_2->type_mode == SEM_TYPE_MODE_T) {
                    type_2->immediate_ir->mode.mode2 = IR_MODE_T;
                }
                if (type_2->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    type_2->immediate_ir->mode.op2 = IR_MODE_ADDR;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    type_2->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_STAR) {
                    type_2->immediate_ir->mode.op2 = IR_MODE_STAR;
                }
                ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);

                // build (x + y * size_multiplier)
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_EXP_OP_ADD;
                ir_entry->temp_id1 = type_1->immediate_ir->temp_id1;
                ir_entry->var_id1 = type_1->immediate_ir->var_id1;
                ir_entry->mode.mode2 = type_1->immediate_ir->mode.mode2;
                ir_entry->mode.op2 = type_1->immediate_ir->mode.op2;
                ir_entry->temp_id2 = type_2->immediate_ir->temp_id;
                ir_entry->mode.mode3 = type_2->immediate_ir->mode.mode1;
                ir_entry->mode.op3 = type_2->immediate_ir->mode.op1;
                ir_entry->temp_id = ir_new_temp_val();
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);

                // replace x in type_1 with (x + y * size_multiplier)
                type_1->immediate_ir->temp_id1 = ir_entry->temp_id;
                type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
            }

            // ret_type is already type_1

            if (ret_type->array_dimension) {
                ret_type->is_array = 1;
            }
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
            // WRONG!
            // type_2->is_lvalue = 1;
            // Counter example:
            // struct a func() {...};
            // then func().x is not a lvalue

            // create MACCESS ir
            if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                if (type_1->immediate_ir->op != IR_EXP_OP_MACCESS) {
                    // turn into MACCESS
                    // x + 0
                    type_1->immediate_ir->temp_id = ir_new_temp_val();
                    type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    temp_var_reg = type_1->immediate_ir->temp_id;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                    type_1->immediate_ir = malloc(sizeof(ir));
                    type_1->immediate_ir->op = IR_EXP_OP_MACCESS;
                    type_1->immediate_ir->temp_id1 = temp_var_reg;
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                    type_1->immediate_ir->int_val2 = 0;
                    type_1->immediate_ir->mode.mode3 = IR_MODE_I;
                    type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                }
            }
            // type_1 should simply not be constant
            // MACCESS is used in struct access and array access where
            // struct and array are defined dynamically thus will never
            // be constant
            else {
                assert(type_1->constant_exp_status != SEM_CONSTANT_YES);
                type_1->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
                type_1->immediate_ir = malloc(sizeof(ir));
                type_1->immediate_ir->op = IR_EXP_OP_MACCESS;
                type_1->immediate_ir->temp_id1 = type_1->ir_temp_val_id;
                type_1->immediate_ir->var_id1 = type_1->ir_var_id;
                if (type_1->type_mode == SEM_TYPE_MODE_T) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                }
                else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_V;
                }
                if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_ADDR;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_STAR;
                }
                type_1->immediate_ir->int_val2 = 0;
                type_1->immediate_ir->mode.mode3 = IR_MODE_I;
                type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
            }
            
            // type_2 is no ordinary _sem_exp_type
            // it does not has a variable id or any useful information other than 
            // basic type and an offset
            // we need to copy the type_1's immediate ir to it
            // and add the offset to it
            type_2->immediate_ir = type_1->immediate_ir;
            type_2->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
            type_2->immediate_ir->int_val2 += type_2->offset_in_struct;
            // other fields are well initialized

            if (type_1->is_lvalue)
                type_2->is_lvalue = 1;
            else 
                type_2->is_lvalue = 0;
            free(type_1);
            return type_2;
        }


        // else Exp XXX Exp
        if (!strcmp(node->children[1]->name, "AND") || !strcmp(node->children[1]->name, "OR")) {
            if (type_1->type != SYMBOL_T_INT) {
                _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT expected.", node->children[1]->line_number);
                free(type_1);
                return NULL;
            }
            goto_label = ir_new_label();
            goto_label_end = ir_new_label();
            ir_list_local2 = malloc(sizeof(ir_list));
            ir_list_local2->head = ir_list_local2->tail = NULL;
            // we do AND and OR now
            if (!strcmp(node->children[1]->name, "AND"))
                is_and = 1;
            else 
                is_and = 0;
            
            if (type_1->constant_exp_status == SEM_CONSTANT_YES) {
                if (is_and) {
                    if (!type_1->int_val) {
                        // validate exp
                        // set no_optimization for things like (x + 1) && (y = 1)
                        ir_list_local = malloc(sizeof(ir_list));
                        ir_list_local->head = ir_list_local->tail = NULL;
                        _sem_validate_exp(node->children[2], 1, ir_list_local);
                        free(ir_list_local);
                        return type_1;
                    }
                }
                else {
                    if (type_1->int_val) {
                        // set no_optimization for things like (x + 1) || (y = 1)
                        ir_list_local = malloc(sizeof(ir_list));
                        ir_list_local->head = ir_list_local->tail = NULL;
                        _sem_validate_exp(node->children[2], 1, ir_list_local);
                        free(ir_list_local);
                        type_1->int_val = 1;
                        return type_1;
                    }
                }
            }
            else if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                ir_entry = malloc(sizeof(ir));
                ir_entry->immediate_ir = type_1->immediate_ir;
                ir_entry->op = IR_OP_IF_IMME;
                switch(ir_entry->immediate_ir->op) {
                    case IR_EXP_OP_EQ:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_NEQ;
                        break;
                    case IR_EXP_OP_NEQ:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_EQ;
                        break;
                    case IR_EXP_OP_LT:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_GE;
                        break;
                    case IR_EXP_OP_LE:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_GT;
                        break;
                    case IR_EXP_OP_GE:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_LT;
                        break;
                    case IR_EXP_OP_GT:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_LE;
                        break;
                    case IR_EXP_OP_MACCESS:
                        type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ir_list_local2);
                        if (is_and)
                            ir_entry->op = IR_OP_IF;
                        else
                            ir_entry->op = IR_OP_IF_POSITIVE;
                        ir_entry->var_id = type_1->immediate_ir->var_id1;
                        ir_entry->temp_id = type_1->immediate_ir->temp_id1;
                        ir_entry->mode.mode1 = type_1->immediate_ir->mode.mode2;
                        ir_entry->mode.op1 = type_1->immediate_ir->mode.op2;
                        break;
                    default:
                        // generate new temp var
                        if (is_and)
                            ir_entry->op = IR_OP_IF;
                        else
                            ir_entry->op = IR_OP_IF_POSITIVE;
                        ir_entry->immediate_ir->temp_id = ir_new_temp_val();
                        ir_entry->immediate_ir->mode.mode1 = IR_MODE_T;
                        ir_entry->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                        ir_add_node_to_buffer(ir_list_local2, ir_entry->immediate_ir);
                        ir_entry->temp_id = ir_entry->immediate_ir->temp_id;
                        ir_entry->mode.mode1 = IR_MODE_T;
                        ir_entry->mode.op1 = IR_MODE_NORMAL;
                        ir_entry->immediate_ir = NULL;
                }
                ir_entry->goto_label = goto_label;
                ir_add_node_to_buffer(ir_list_local2, ir_entry);
            }
            else {
                // non constant
                ir_entry = malloc(sizeof(ir));
                ir_entry->goto_label = goto_label;
                if (type_1->type_mode == SEM_TYPE_MODE_T) {
                    if (is_and)
                        ir_entry->op = IR_OP_IF;
                    else
                        ir_entry->op = IR_OP_IF_POSITIVE;
                    ir_entry->mode.mode1 = IR_MODE_T;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->temp_id = type_1->ir_temp_val_id;
                }
                else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                    if (is_and)
                        ir_entry->op = IR_OP_IF;
                    else
                        ir_entry->op = IR_OP_IF_POSITIVE;
                    ir_entry->mode.mode1 = IR_MODE_V;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->var_id = type_1->ir_var_id;
                }

                if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                    ir_entry->mode.op1 = IR_MODE_STAR;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    ir_entry->mode.op1 = IR_MODE_ADDR;
                }
                ir_add_node_to_buffer(ir_list_local2, ir_entry);
            }

            type_2 = _sem_validate_exp(node->children[2], no_optimization, ir_list_local2);

            // now deal with ir_2
            if (type_2->constant_exp_status == SEM_CONSTANT_YES) {
                if (is_and) {
                    if (!type_2->int_val) {
                        return type_2;
                    }
                }
                else {
                    if (type_2->int_val) {
                        type_2->int_val = 1;
                        return type_2;
                    }
                }
                ir_merge_buffer(ret_ir, ir_list_local2);
            }
            else if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                ir_merge_buffer(ret_ir, ir_list_local2);
                ir_entry = malloc(sizeof(ir));
                ir_entry->immediate_ir = type_2->immediate_ir;
                ir_entry->op = IR_OP_IF_IMME;
                switch(ir_entry->immediate_ir->op) {
                    case IR_EXP_OP_EQ:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_NEQ;
                        break;
                    case IR_EXP_OP_NEQ:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_EQ;
                        break;
                    case IR_EXP_OP_LT:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_GE;
                        break;
                    case IR_EXP_OP_LE:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_GT;
                        break;
                    case IR_EXP_OP_GE:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_LT;
                        break;
                    case IR_EXP_OP_GT:
                        if (is_and)
                            ir_entry->immediate_ir->op = IR_EXP_OP_LE;
                        break;
                    case IR_EXP_OP_MACCESS:
                        type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                        if (is_and)
                            ir_entry->op = IR_OP_IF;
                        else
                            ir_entry->op = IR_OP_IF_POSITIVE;
                        ir_entry->var_id = type_2->immediate_ir->var_id1;
                        ir_entry->temp_id = type_2->immediate_ir->temp_id1;
                        ir_entry->mode.mode1 = type_2->immediate_ir->mode.mode2;
                        ir_entry->mode.op1 = type_2->immediate_ir->mode.op2;
                        break;
                    default:
                        // generate new temp var
                        if (is_and)
                            ir_entry->op = IR_OP_IF;
                        else
                            ir_entry->op = IR_OP_IF_POSITIVE;
                        ir_entry->immediate_ir->temp_id = ir_new_temp_val();
                        ir_entry->immediate_ir->mode.mode1 = IR_MODE_T;
                        ir_entry->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                        ir_add_node_to_buffer(ret_ir, ir_entry->immediate_ir);
                        ir_entry->temp_id = ir_entry->immediate_ir->temp_id;
                        ir_entry->mode.mode1 = IR_MODE_T;
                        ir_entry->mode.op1 = IR_MODE_NORMAL;
                        ir_entry->immediate_ir = NULL;
                }
                ir_entry->goto_label = goto_label;
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }
            else {
                // non constant
                ir_merge_buffer(ret_ir, ir_list_local2);
                ir_entry = malloc(sizeof(ir));
                ir_entry->goto_label = goto_label;
                if (type_2->type_mode == SEM_TYPE_MODE_T) {
                    if (is_and)
                        ir_entry->op = IR_OP_IF;
                    else
                        ir_entry->op = IR_OP_IF_POSITIVE;
                    ir_entry->mode.mode1 = IR_MODE_T;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->temp_id = type_2->ir_temp_val_id;
                }
                else if (type_2->type_mode == SEM_TYPE_MODE_V) {
                    if (is_and)
                        ir_entry->op = IR_OP_IF;
                    else
                        ir_entry->op = IR_OP_IF_POSITIVE;
                    ir_entry->mode.mode1 = IR_MODE_V;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->var_id = type_2->ir_var_id;
                }

                if (type_2->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_STAR) {
                    ir_entry->mode.op1 = IR_MODE_STAR;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    ir_entry->mode.op1 = IR_MODE_ADDR;
                }
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }

            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = SYMBOL_T_INT;
            ret_type->is_array = 0;
            ret_type->constant_exp_status = SEM_CONSTANT_NO;
            ret_type->ir_temp_val_id = ir_new_temp_val();
            ret_type->type_mode = SEM_TYPE_MODE_T;
            ret_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
            temp_var_reg = ret_type->ir_temp_val_id;
            if (is_and) {
                ir_entry = malloc(sizeof(ir));
                ir_entry->temp_id = temp_var_reg;
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->int_val1 = 1;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }
            else {
                ir_entry = malloc(sizeof(ir));
                ir_entry->temp_id = temp_var_reg;
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->int_val1 = 0;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_GOTO;
            ir_entry->goto_label = goto_label_end;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            ir_entry->goto_label = goto_label;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            if (is_and) {
                ir_entry = malloc(sizeof(ir));
                ir_entry->temp_id = temp_var_reg;
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->int_val1 = 0;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }
            else {
                ir_entry = malloc(sizeof(ir));
                ir_entry->temp_id = temp_var_reg;
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->int_val1 = 1;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, ir_entry);
            }
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            ir_entry->goto_label = goto_label_end;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            return ret_type;
        }

        // check if the first expression is an immediate expression
        if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            // assign a new temp variable
            // as the expression is not a constant
            // or it shall be evaluated directly
            if (type_1->immediate_ir->op == IR_EXP_OP_MACCESS) {
                type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ret_ir);
                // we turn it into type_1's attributes
                type_1->ir_var_id = type_1->immediate_ir->var_id1;
                type_1->ir_temp_val_id = type_1->immediate_ir->temp_id1;
                if (type_1->immediate_ir->mode.mode2 == IR_MODE_T) {
                    type_1->type_mode = SEM_TYPE_MODE_T;
                }
                else {
                    type_1->type_mode = SEM_TYPE_MODE_V;
                }
                if (type_1->immediate_ir->mode.op2 == IR_MODE_ADDR) {
                    type_1->type_mode_op = SEM_TYPE_MODE_ADDR;
                }
                else if (type_1->immediate_ir->mode.op2 == IR_MODE_NORMAL) {
                    type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
                }
                else if (type_1->immediate_ir->mode.op2 == IR_MODE_STAR) {
                    type_1->type_mode_op = SEM_TYPE_MODE_STAR;
                }
                type_1->constant_exp_status = SEM_CONSTANT_NO;
            }
            else {
                type_1->constant_exp_status = SEM_CONSTANT_NO;
                type_1->immediate_ir->temp_id = ir_new_temp_val();
                type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                type_1->ir_temp_val_id = type_1->immediate_ir->temp_id;
                type_1->immediate_ir = NULL;
                type_1->type_mode = SEM_TYPE_MODE_T;
                type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
            }
        }
        
        // since we have simplified type_1->immediate_ir above,
        // type_1 will never comes with the status of CONSTANT_IMMEDIATE
        if ((type_1->type != SYMBOL_T_INT && type_1->type != SYMBOL_T_FLOAT) || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT/FLOAT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }
        type_2 = _sem_validate_exp(node->children[2], no_optimization, ret_ir);
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

        // we generates IMMEDIATE here
        // type_1 is simplified thus we can use it directly
        if (type_1->constant_exp_status == SEM_CONSTANT_YES && type_2->constant_exp_status == SEM_CONSTANT_YES) {
            // then the result is still constant!
            
            // eval now
            // Exp AND Exp
            // Exp OR Exp
            // Exp RELOP Exp
            // Exp PLUS Exp
            // Exp MINUS Exp
            // Exp STAR Exp
            // Exp DIV Exp
            if (!strcmp(node->children[1]->name, "AND")) {
                type_1->int_val = (type_1->int_val && type_2->int_val);
            }
            else if (!strcmp(node->children[1]->name, "OR")) {
                type_1->int_val = (type_1->int_val || type_2->int_val);
            }
            else if (!strcmp(node->children[1]->name, "RELOP")) {
                if (!strcmp(node->children[1]->string_value, ">")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val > type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val > type_2->float_val);
                    }
                }
                else if (!strcmp(node->children[1]->string_value, ">=")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val >= type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val >= type_2->float_val);
                    }
                }
                else if (!strcmp(node->children[1]->string_value, "==")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val == type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val == type_2->float_val);
                    }
                }
                else if (!strcmp(node->children[1]->string_value, "<=")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val <= type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val <= type_2->float_val);
                    }
                }
                else if (!strcmp(node->children[1]->string_value, "<")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val < type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val < type_2->float_val);
                    }
                }
                else if (!strcmp(node->children[1]->string_value, "!=")) {
                    if (type_1->type == SYMBOL_T_INT) {
                        type_1->int_val = (type_1->int_val != type_2->int_val);
                    }
                    else {
                        type_1->type = SYMBOL_T_INT;
                        type_1->int_val = (type_1->float_val != type_2->float_val);
                    }
                }
            }
            else if (!strcmp(node->children[1]->name, "PLUS")) {
                if (type_1->type == SYMBOL_T_INT)
                    type_1->int_val = (type_1->int_val + type_2->int_val);
                else
                    type_1->float_val = (type_1->float_val + type_2->float_val);
            }
            else if (!strcmp(node->children[1]->name, "MINUS")) {
                if (type_1->type == SYMBOL_T_INT)
                    type_1->int_val = (type_1->int_val - type_2->int_val);
                else
                    type_1->float_val = (type_1->float_val - type_2->float_val);
            }
            else if (!strcmp(node->children[1]->name, "STAR")) {
                if (type_1->type == SYMBOL_T_INT)
                    type_1->int_val = (type_1->int_val * type_2->int_val);
                else
                    type_1->float_val = (type_1->float_val * type_2->float_val);
            }
            else if (!strcmp(node->children[1]->name, "DIV")) {
                if (type_1->type == SYMBOL_T_INT)
                    type_1->int_val = (type_1->int_val / type_2->int_val);
                else
                    type_1->float_val = (type_1->float_val / type_2->float_val);
            }
        }
        else {
            // we make type_1->immediate_ir be type_1's result and waiting for type_2's result and op
            if (type_1->constant_exp_status == SEM_CONSTANT_YES) {
               type_1->immediate_ir = malloc(sizeof(ir));
                if (type_1->type == SYMBOL_T_FLOAT) {
                    type_1->immediate_ir->float_val1 = type_1->float_val;
                }
                else {
                    type_1->immediate_ir->int_val1 = type_1->int_val;
                }
                type_1->immediate_ir->mode.mode2 = IR_MODE_I;
                type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
            }
            else if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                if (type_1->immediate_ir->op == IR_EXP_OP_MACCESS) {
                    type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ret_ir);
                }
                else {
                    type_1->immediate_ir->temp_id = ir_new_temp_val();
                    temp_var_reg = type_1->immediate_ir->temp_id;
                    type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                    type_1->immediate_ir = malloc(sizeof(ir));
                    type_1->immediate_ir->temp_id1 = temp_var_reg;
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
            }
            else {
                type_1->immediate_ir = malloc(sizeof(ir));
                type_1->immediate_ir->temp_id1 = type_1->ir_temp_val_id;
                type_1->immediate_ir->var_id1 = type_1->ir_var_id;
                if (type_1->type_mode == SEM_TYPE_MODE_T) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                }
                else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                    type_1->immediate_ir->mode.mode2 = IR_MODE_V;
                }
                if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_ADDR;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
                }
                else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                    type_1->immediate_ir->mode.op2 = IR_MODE_STAR;
                }
            }
            type_1->constant_exp_status = SEM_CONSTANT_IMMEDIATE;

            // now we fill type_1->immediate_ir with type_2's result
            if (type_2->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                if (type_2->immediate_ir->op == IR_EXP_OP_MACCESS) {
                    type_2->immediate_ir = ir_simplify_maccess(type_2->immediate_ir, ret_ir);
                    type_1->immediate_ir->temp_id2 = type_2->immediate_ir->temp_id1;
                    type_1->immediate_ir->var_id2 = type_2->immediate_ir->var_id1;
                    type_1->immediate_ir->mode.mode3 = type_2->immediate_ir->mode.mode2;
                    type_1->immediate_ir->mode.op3 = type_2->immediate_ir->mode.op2;
                }
                else {
                    type_2->immediate_ir->temp_id = ir_new_temp_val();
                    temp_var_reg = type_2->immediate_ir->temp_id;
                    type_2->immediate_ir->mode.mode1 = IR_MODE_T;
                    type_2->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, type_2->immediate_ir);
                    type_1->immediate_ir->temp_id2 = temp_var_reg;
                    type_1->immediate_ir->mode.mode3 = IR_MODE_T;
                    type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                }
            }
            else if (type_2->constant_exp_status == SEM_CONSTANT_YES) {
                if (type_1->type == T_FLOAT)
                    type_1->immediate_ir->float_val2 = type_2->float_val;
                else
                    type_1->immediate_ir->int_val2 = type_2->int_val;
                type_1->immediate_ir->mode.mode3 = IR_MODE_I;
                type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
            }
            else {
                type_1->immediate_ir->temp_id2 = type_2->ir_temp_val_id;
                type_1->immediate_ir->var_id2 = type_2->ir_var_id;
                if (type_2->type_mode == SEM_TYPE_MODE_T) {
                    type_1->immediate_ir->mode.mode3 = IR_MODE_T;
                }
                else if (type_2->type_mode == SEM_TYPE_MODE_V) {
                    type_1->immediate_ir->mode.mode3 = IR_MODE_V;
                }
                if (type_2->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    type_1->immediate_ir->mode.op3 = IR_MODE_ADDR;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    type_1->immediate_ir->mode.op3 = IR_MODE_NORMAL;
                }
                else if (type_2->type_mode_op == SEM_TYPE_MODE_STAR) {
                    type_1->immediate_ir->mode.op3 = IR_MODE_STAR;
                }
            }
            // now deal with operators
            if (!strcmp(node->children[1]->name, "AND")) {
                type_1->immediate_ir->op = IR_EXP_OP_AND;
            }
            else if (!strcmp(node->children[1]->name, "OR")) {
                type_1->immediate_ir->op = IR_EXP_OP_OR;
            }
            else if (!strcmp(node->children[1]->name, "RELOP")) {
                if (!strcmp(node->children[1]->string_value, ">")) {
                    type_1->immediate_ir->op = IR_EXP_OP_GT;
                    type_1->type = SYMBOL_T_INT;
                }
                else if (!strcmp(node->children[1]->string_value, ">=")) {
                    type_1->immediate_ir->op = IR_EXP_OP_GE;
                    type_1->type = SYMBOL_T_INT;
                }
                else if (!strcmp(node->children[1]->string_value, "==")) {
                    type_1->immediate_ir->op = IR_EXP_OP_EQ;
                    type_1->type = SYMBOL_T_INT;
                }
                else if (!strcmp(node->children[1]->string_value, "<=")) {
                    type_1->immediate_ir->op = IR_EXP_OP_LE;
                    type_1->type = SYMBOL_T_INT;
                }
                else if (!strcmp(node->children[1]->string_value, "<")) {
                    type_1->immediate_ir->op = IR_EXP_OP_LT;
                    type_1->type = SYMBOL_T_INT;
                }
                else if (!strcmp(node->children[1]->string_value, "!=")) {
                    type_1->immediate_ir->op = IR_EXP_OP_NEQ;
                    type_1->type = SYMBOL_T_INT;
                }
            }
            else if (!strcmp(node->children[1]->name, "PLUS")) {
                type_1->immediate_ir->op = IR_EXP_OP_ADD;
            }
            else if (!strcmp(node->children[1]->name, "MINUS")) {
                type_1->immediate_ir->op = IR_EXP_OP_MINUS;
            }
            else if (!strcmp(node->children[1]->name, "STAR")) {
                type_1->immediate_ir->op = IR_EXP_OP_MUL;
            }
            else if (!strcmp(node->children[1]->name, "DIV")) {
                type_1->immediate_ir->op = IR_EXP_OP_DIV;
            }
        }

        free(type_2);
        type_1->is_lvalue = 0;
        return type_1;
    }
    else if (!strcmp(node->children[0]->name, "LP")) {
        // LP Exp RP
        return _sem_validate_exp(node->children[1], no_optimization, ret_ir);
    }
    else if (!strcmp(node->children[0]->name, "MINUS")) {
        // MINUS Exp
        type_1 = _sem_validate_exp(node->children[1], no_optimization, ret_ir);
        if (type_1 == NULL) {
            return NULL;
        }

        if ((type_1->type != SYMBOL_T_INT && type_1->type != SYMBOL_T_FLOAT) || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT/FLOAT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }

        if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            if (type_1->immediate_ir->op == IR_EXP_OP_MACCESS) {
                type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ret_ir);
                type_1->immediate_ir->temp_id2 = type_1->immediate_ir->temp_id1;
                type_1->immediate_ir->var_id2 = type_1->immediate_ir->var_id1;
                type_1->immediate_ir->mode.mode3 = type_1->immediate_ir->mode.mode2;
                type_1->immediate_ir->mode.op3 = type_1->immediate_ir->mode.op2;
                ir_entry = type_1->immediate_ir;
            }
            else {
                type_1->immediate_ir->temp_id = ir_new_temp_val();
                type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                ir_entry = malloc(sizeof(ir));
                ir_entry->temp_id2 = type_1->immediate_ir->temp_id;
                ir_entry->mode.mode3 = IR_MODE_T;
                ir_entry->mode.op3 = IR_MODE_NORMAL;
            }
            ir_entry->op = IR_EXP_OP_MINUS;
            ir_entry->temp_id = ir_new_temp_val();
            ir_entry->mode.mode1 = IR_MODE_T;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
            ir_entry->int_val1 = 0;
            ir_entry->mode.mode2 = IR_MODE_I;
            ir_entry->mode.op2 = IR_MODE_NORMAL;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            type_1->constant_exp_status = SEM_CONSTANT_NO;
            type_1->ir_temp_val_id = ir_entry->temp_id;
            type_1->type_mode = SEM_TYPE_MODE_T;
            type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
        }
        else if (type_1->constant_exp_status == SEM_CONSTANT_YES) {
            if (type_1->type == SYMBOL_T_INT) {
                type_1->int_val = -type_1->int_val;
            }
            else {
                type_1->float_val = -type_1->float_val;
            }
        }
        else {
            ir_entry = malloc(sizeof(ir));
            ir_entry->temp_id2 = type_1->ir_temp_val_id;
            ir_entry->var_id2 = type_1->ir_var_id;
            if (type_1->type_mode == SEM_TYPE_MODE_T) {
                ir_entry->mode.mode3 = IR_MODE_T;
            }
            else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                ir_entry->mode.mode3 = IR_MODE_V;
            }
            if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                ir_entry->mode.op3 = IR_MODE_ADDR;
            }
            else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                ir_entry->mode.op3 = IR_MODE_NORMAL;
            }
            else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                ir_entry->mode.op3 = IR_MODE_STAR;
            }
            ir_entry->op = IR_EXP_OP_MINUS;
            ir_entry->temp_id = ir_new_temp_val();
            ir_entry->mode.mode1 = IR_MODE_T;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
            ir_entry->int_val1 = 0;
            ir_entry->mode.mode2 = IR_MODE_I;
            ir_entry->mode.op2 = IR_MODE_NORMAL;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            type_1->constant_exp_status = SEM_CONSTANT_NO;
            type_1->ir_temp_val_id = ir_entry->temp_id;
            type_1->type_mode = SEM_TYPE_MODE_T;
            type_1->type_mode_op = SEM_TYPE_MODE_NORMAL;
        }

        type_1->is_lvalue = 0;
        return type_1;        
    }
    else if (!strcmp(node->children[0]->name, "NOT")) {
        // NOT Exp
        type_1 = _sem_validate_exp(node->children[1], no_optimization, ret_ir);
        if (type_1 == NULL) {
            return NULL;
        }

        if (type_1->type != SYMBOL_T_INT || type_1->is_array) {
            _sem_report_error("Error type 7 at Line %d: Type mismatched for operator. INT expected.", node->children[1]->line_number);
            free(type_1);
            return NULL;
        }

        if (type_1->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            if (type_1->immediate_ir->op == IR_EXP_OP_MACCESS) {
                type_1->immediate_ir = ir_simplify_maccess(type_1->immediate_ir, ret_ir);
            }
            else {
                type_1->immediate_ir->temp_id = ir_new_temp_val();
                temp_var_reg = type_1->immediate_ir->temp_id;
                type_1->immediate_ir->mode.mode1 = IR_MODE_T;
                type_1->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, type_1->immediate_ir);
                type_1->immediate_ir = malloc(sizeof(ir));
                type_1->immediate_ir->temp_id1 = temp_var_reg;
                type_1->immediate_ir->mode.mode2 = IR_MODE_T;
                type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
            }
            type_1->immediate_ir->op = IR_EXP_OP_NOT;
        }
        else if (type_1->constant_exp_status == SEM_CONSTANT_YES) {
            type_1->int_val = !type_1->int_val;
        }
        else {
            type_1->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
            type_1->immediate_ir = malloc(sizeof(ir));
            type_1->immediate_ir->op = IR_EXP_OP_NOT;
            type_1->immediate_ir->temp_id1 = type_1->ir_temp_val_id;
            type_1->immediate_ir->var_id1 = type_1->ir_var_id;
            if (type_1->type_mode == SEM_TYPE_MODE_T) {
                type_1->immediate_ir->mode.mode2 = IR_MODE_T;
            }
            else if (type_1->type_mode == SEM_TYPE_MODE_V) {
                type_1->immediate_ir->mode.mode2 = IR_MODE_V;
            }
            if (type_1->type_mode_op == SEM_TYPE_MODE_ADDR) {
                type_1->immediate_ir->mode.op2 = IR_MODE_ADDR;
            }
            else if (type_1->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                type_1->immediate_ir->mode.op2 = IR_MODE_NORMAL;
            }
            else if (type_1->type_mode_op == SEM_TYPE_MODE_STAR) {
                type_1->immediate_ir->mode.op2 = IR_MODE_STAR;
            }
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
            ret_type->array_size = symbol->array_size;
            ret_type->array_accumulated_size = symbol->size;
            ret_type->is_lvalue = 1;
            ret_type->struct_specifier = symbol->struct_specifier;
            ret_type->constant_exp_status = SEM_CONSTANT_NO;
            ret_type->ir_var_id = symbol->ir_variable_id;
            //printf("%s->varid = %d\n", symbol->id, symbol->ir_variable_id);
            ret_type->type_mode = SEM_TYPE_MODE_V;
            ret_type->var_symbol = symbol;
            if ((ret_type->is_array || ret_type->type == SYMBOL_T_STRUCT) && !ret_type->var_symbol->is_param) {
                ret_type->type_mode_op = SEM_TYPE_MODE_ADDR;
            }
            else {
                ret_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
            }
            if (!no_optimization) {
                if (symbol->is_constant == IR_CONSTANT && !symbol->is_array && symbol->type != SYMBOL_T_STRUCT) {
                    ret_type->constant_exp_status = SEM_CONSTANT_YES;
                    if (symbol->type == SYMBOL_T_INT) {
                        ret_type->int_val = symbol->int_val;
                    }
                    else {
                        ret_type->float_val = symbol->float_val;
                    }
                }
            }
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
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_CALL;
            ir_entry->func_name = node->children[0]->string_value;
            if (!strcmp(ir_entry->func_name, "read")) {
                ir_entry->op = IR_OP_READ;
            }
            
            ret_type->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
            ret_type->immediate_ir = ir_entry;
            return ret_type;
        }

        if (node->children_count == 4) {
            // ID LP Args RP
            ir_list *ir_list_local = malloc(sizeof(ir_list));
            ir_list_local->head = NULL;
            ir_list_local->tail = NULL;
            if (!symbol->is_function) {
                _sem_report_error("Error type 11 at Line %d: Not a function.", node->children[0]->line_number);
                return NULL;
            }
            args_list = _sem_validate_args(node->children[2], no_optimization, ir_list_local);
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
            _sem_add_args(args_list, ir_list_local);

            if (!strcmp(symbol->id, "write")) {
                ret_type = malloc(sizeof(_sem_exp_type));
                ret_type->constant_exp_status = SEM_CONSTANT_NO;
                ret_type->type = SYMBOL_T_VOID;
                ir_node *iterator = ir_list_local->head;
                if (ir_list_local->head == ir_list_local->tail) {
                    ir_entry = malloc(sizeof(ir));
                    ir_entry->op = IR_OP_WRITE;
                    ir_entry->temp_id = iterator->content->temp_id;
                    ir_entry->var_id = iterator->content->var_id;
                    ir_entry->int_val1 = iterator->content->int_val1;
                    ir_entry->mode.mode1 = iterator->content->mode.mode1;
                    ir_entry->mode.op1 = iterator->content->mode.op1;
                    ir_add_node_to_buffer(ret_ir, ir_entry);
                    return ret_type;
                }
                while (iterator->next != ir_list_local->tail) {
                    iterator = iterator->next;
                }
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_OP_WRITE;
                ir_entry->temp_id = iterator->next->content->temp_id;
                ir_entry->var_id = iterator->next->content->var_id;
                ir_entry->int_val1 = iterator->next->content->int_val1;
                ir_entry->mode.mode1 = iterator->next->content->mode.mode1;
                ir_entry->mode.op1 = iterator->next->content->mode.op1;
                ir_list_local->tail = iterator;
                free(iterator->next);
                ir_list_local->tail->next = NULL;
                ir_merge_buffer(ret_ir, ir_list_local);
                ir_add_node_to_buffer(ret_ir, ir_entry);
                return ret_type;
            }

            ir_merge_buffer(ret_ir, ir_list_local);
            free(ir_list_local);
            ret_type = malloc(sizeof(_sem_exp_type));
            ret_type->type = symbol->type;
            ret_type->is_array = symbol->is_array;
            ret_type->array_dimension = symbol->array_dimention;
            ret_type->is_lvalue = 0;
            ret_type->struct_specifier = symbol->struct_specifier;
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_CALL;
            ir_entry->func_name = node->children[0]->string_value;
            ret_type->constant_exp_status = SEM_CONSTANT_IMMEDIATE;
            ret_type->immediate_ir = ir_entry;
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
        ret_type->constant_exp_status = SEM_CONSTANT_YES;
        ret_type->int_val = node->children[0]->int_value;
        return ret_type;
    }
    else if (!strcmp(node->children[0]->name, "FLOAT")) {
        ret_type = malloc(sizeof(_sem_exp_type));
        ret_type->type = SYMBOL_T_FLOAT;
        ret_type->is_array = 0;
        ret_type->array_dimension = 0;
        ret_type->is_lvalue = 0;
        ret_type->struct_specifier = NULL;
        ret_type->constant_exp_status = SEM_CONSTANT_YES;
        ret_type->float_val = node->children[0]->float_value;
        return ret_type;
    }
    assert(0);
    return NULL;
}

_sem_exp_type_list *_sem_validate_args(ast_node *node, char no_optimization, ir_list *ret_ir) {
    _sem_exp_type *current_type;
    _sem_exp_type_list *ret_list;
    _sem_exp_type_list *ret_list_tail;
    ir *ir_entry;

    
    
    if (node->children_count == 1) {
        current_type = _sem_validate_exp(node->children[0], no_optimization, ret_ir);

        if (current_type == NULL) {
            return NULL;
        }
    
        // we have to solve the immediate ir here
        if (current_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            if (current_type->immediate_ir->op == IR_EXP_OP_MACCESS) {
                current_type->immediate_ir = ir_simplify_maccess(current_type->immediate_ir, ret_ir);
                current_type->constant_exp_status = SEM_CONSTANT_NO;
                current_type->ir_temp_val_id = current_type->immediate_ir->temp_id1;
                current_type->ir_var_id = current_type->immediate_ir->var_id1;
                if (current_type->immediate_ir->mode.mode2 == IR_MODE_T) {
                    current_type->type_mode = SEM_TYPE_MODE_T;
                }
                else if (current_type->immediate_ir->mode.mode2 == IR_MODE_V) {
                    current_type->type_mode = SEM_TYPE_MODE_V;
                }
                if (current_type->immediate_ir->mode.op2 == IR_MODE_ADDR) {
                    current_type->type_mode_op = SEM_TYPE_MODE_ADDR;
                }
                else if (current_type->immediate_ir->mode.op2 == IR_MODE_NORMAL) {
                    current_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
                }
                else if (current_type->immediate_ir->mode.op2 == IR_MODE_STAR) {
                    current_type->type_mode_op = SEM_TYPE_MODE_STAR;
                }
            }
            else {
                current_type->immediate_ir->temp_id = ir_new_temp_val();
                current_type->immediate_ir->mode.mode1 = IR_MODE_T;
                current_type->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, current_type->immediate_ir);
                current_type->constant_exp_status = SEM_CONSTANT_NO;
                current_type->ir_temp_val_id = current_type->immediate_ir->temp_id;
                current_type->type_mode = SEM_TYPE_MODE_T;
                current_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
            }
        }

        ret_list = malloc(sizeof(_sem_exp_type_list));
        ret_list->type = current_type;
        ret_list->next = NULL;
        // Exp
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_ARG;
        if (current_type->constant_exp_status == SEM_CONSTANT_YES) {
            if (current_type->type == SYMBOL_T_INT)
                ir_entry->int_val1 = current_type->int_val;
            else
                ir_entry->float_val1 = current_type->float_val;
            ir_entry->mode.mode1 = IR_MODE_I;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
        }
        else {
            ir_entry->temp_id = current_type->ir_temp_val_id;
            ir_entry->var_id = current_type->ir_var_id;
            if (current_type->type_mode == SEM_TYPE_MODE_T) {
                ir_entry->mode.mode1 = IR_MODE_T;
            }
            else if (current_type->type_mode == SEM_TYPE_MODE_V) {
                ir_entry->mode.mode1 = IR_MODE_V;
            }
            if (current_type->type_mode_op == SEM_TYPE_MODE_ADDR) {
                ir_entry->mode.op1 = IR_MODE_ADDR;
            }
            else if (current_type->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                ir_entry->mode.op1 = IR_MODE_NORMAL;
            }
            else if (current_type->type_mode_op == SEM_TYPE_MODE_STAR) {
                ir_entry->mode.op1 = IR_MODE_STAR;
            }
        }
        current_type->immediate_ir = ir_entry;
        return ret_list;
    }
    else if (node->children_count == 3) {
        // Exp COMMA Args
        ret_list_tail = _sem_validate_args(node->children[2], no_optimization, ret_ir);
        if (ret_list_tail == NULL) {
            return NULL;
        }
        current_type = _sem_validate_exp(node->children[0], no_optimization, ret_ir);

        if (current_type == NULL) {
            return NULL;
        }

        // we have to solve the immediate ir here
        if (current_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            if (current_type->immediate_ir->op == IR_EXP_OP_MACCESS) {
                current_type->immediate_ir = ir_simplify_maccess(current_type->immediate_ir, ret_ir);
                current_type->constant_exp_status = SEM_CONSTANT_NO;
                current_type->ir_temp_val_id = current_type->immediate_ir->temp_id1;
                current_type->ir_var_id = current_type->immediate_ir->var_id1;
                if (current_type->immediate_ir->mode.mode2 == IR_MODE_T) {
                    current_type->type_mode = SEM_TYPE_MODE_T;
                }
                else if (current_type->immediate_ir->mode.mode2 == IR_MODE_V) {
                    current_type->type_mode = SEM_TYPE_MODE_V;
                }
                if (current_type->immediate_ir->mode.op2 == IR_MODE_ADDR) {
                    current_type->type_mode_op = SEM_TYPE_MODE_ADDR;
                }
                else if (current_type->immediate_ir->mode.op2 == IR_MODE_NORMAL) {
                    current_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
                }
                else if (current_type->immediate_ir->mode.op2 == IR_MODE_STAR) {
                    current_type->type_mode_op = SEM_TYPE_MODE_STAR;
                }
            }
            else {
                current_type->immediate_ir->temp_id = ir_new_temp_val();
                current_type->immediate_ir->mode.mode1 = IR_MODE_T;
                current_type->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, current_type->immediate_ir);
                current_type->constant_exp_status = SEM_CONSTANT_NO;
                current_type->ir_temp_val_id = current_type->immediate_ir->temp_id;
                current_type->type_mode = SEM_TYPE_MODE_T;
                current_type->type_mode_op = SEM_TYPE_MODE_NORMAL;
            }
        }

        ret_list = malloc(sizeof(_sem_exp_type_list));
        ret_list->type = current_type;
        ret_list->next = ret_list_tail;
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_ARG;
        if (current_type->constant_exp_status == SEM_CONSTANT_YES) {
            if (current_type->type == SYMBOL_T_INT)
                ir_entry->int_val1 = current_type->int_val;
            else
                ir_entry->float_val1 = current_type->float_val;
            ir_entry->mode.mode1 = IR_MODE_I;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
        }
        else {
            ir_entry->temp_id = current_type->ir_temp_val_id;
            ir_entry->var_id = current_type->ir_var_id;
            if (current_type->type_mode == SEM_TYPE_MODE_T) {
                ir_entry->mode.mode1 = IR_MODE_T;
            }
            else if (current_type->type_mode == SEM_TYPE_MODE_V) {
                ir_entry->mode.mode1 = IR_MODE_V;
            }
            if (current_type->type_mode_op == SEM_TYPE_MODE_ADDR) {
                ir_entry->mode.op1 = IR_MODE_ADDR;
            }
            else if (current_type->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                ir_entry->mode.op1 = IR_MODE_NORMAL;
            }
            else if (current_type->type_mode_op == SEM_TYPE_MODE_STAR) {
                ir_entry->mode.op1 = IR_MODE_STAR;
            }
        }
        current_type->immediate_ir = ir_entry;
        return ret_list;
    }
    else {
        assert(0);
        return NULL;
    }
}


void _sem_add_args(_sem_exp_type_list *tl, ir_list *ret_ir) {
    if (tl == NULL) 
        return;
    _sem_add_args(tl->next, ret_ir);
    ir_add_node_to_buffer(ret_ir, tl->type->immediate_ir);
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

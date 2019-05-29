/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    semantics.c
    Defines semantics validation
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

void _sem_validate_ext_def_list(ast_node *node, ir_func_list *ret_ir);
void _sem_validate_ext_def(ast_node *node, ir_func_list *ret_ir);
int _sem_validate_specifier(ast_node *node, struct_specifier **struct_specifier, int context, int do_not_free);
void _sem_validate_ext_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, ir_list *ret_ir);
symbol_entry *_sem_validate_var_dec(ast_node *node);
struct_specifier *_sem_validate_struct_specifier(ast_node *node, int context, int do_not_free);
symbol_entry *_sem_validate_fun_dec(ast_node *node, int type, struct_specifier *struct_specifier, ir_list *ret_ir);
int _sem_validate_var_list(ast_node *node, symbol_list **param_list, ir_list *ret_ir);
symbol_entry *_sem_validate_param_dec(ast_node *node);
void _sem_validate_def_list(ast_node *node, int context, char no_optimization, ir_list *ret_ir);
void _sem_validate_def(ast_node *node, int context, char no_optimization, ir_list *ret_ir);
void _sem_validate_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, int context, char no_optimization, ir_list *ret_ir);
symbol_entry *_sem_validate_dec(ast_node *node, int type, struct_specifier *parsed_struct_specifier, char no_optimization, ir_list *ret_ir);
char _sem_type_matching_with_symbol(symbol_entry *se, _sem_exp_type *et);
void _sem_validate_comp_st(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir);
void _sem_validate_stmt_list(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir);
void _sem_validate_stmt(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir);


// helpers from semantics_struct_helper
extern symbol_list *_sem_validate_def_list_for_struct(ast_node *node, int context);
extern symbol_list *_sem_validate_def_for_struct(ast_node *node, int context);
extern symbol_list *_sem_validate_dec_list_for_struct(ast_node *node, int type, struct_specifier *struct_specifier, int context);
extern symbol_entry *_sem_validate_dec_for_struct(ast_node *node, int type, struct_specifier *struct_specifier);
extern _sem_exp_type *_sem_validate_exp(ast_node *node, char no_optimization, ir_list *ret_ir);
extern char _sem_type_matching(_sem_exp_type *t1, _sem_exp_type *t2);

char _sem_error = 0;
int _sem_unnamed_struct_count = 0;

void _sem_report_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    _sem_error = 1;
}

char sem_validate(ast_node *root) {
    ir_func_list *root_ir = malloc(sizeof(ir_func_list));
    root_ir->func_content = NULL;
    root_ir->next = NULL;
    symtable_init();
    // Current node: Program
    if (root->children[0] != NULL) {
        _sem_validate_ext_def_list(root->children[0], root_ir);
    }
    // else {
    //     empty program!
    //}
    symtable_ensure_defined();
    return _sem_error;
}

void _sem_validate_ext_def_list(ast_node *node, ir_func_list *ret_ir) {
    if (node == NULL) {
        // empty def list
        return;
    }
    ir_func_list *sec_ir;
    assert(node->children_count == 2);
    assert(node->children[0] != NULL);
    assert(!strcmp(node->children[0]->name, "ExtDef"));

    _sem_validate_ext_def(node->children[0], ret_ir);
    if (ret_ir->func_content == NULL) {
        _sem_validate_ext_def_list(node->children[1], ret_ir);
        return;
    }
    else {
        sec_ir = malloc(sizeof(ir_func_list));
        sec_ir->func_content = NULL;
        sec_ir->next = NULL;
        _sem_validate_ext_def_list(node->children[1], sec_ir);
        ret_ir->next = sec_ir;
        return;
    }
} 

void _sem_validate_ext_def(ast_node *node, ir_func_list *ret_ir) {
    assert(node->children_count >= 2);
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);
    assert(!strcmp(node->children[0]->name, "Specifier"));

    int type;
    struct_specifier *struct_specifier = NULL;
    symbol_entry *func_entry;
    _sem_exp_type return_type;
    ir_list *func_contents;
    type = _sem_validate_specifier(node->children[0], &struct_specifier, 0, 0);
    if (type == SYMBOL_T_ERROR) {
        // specifier validation failed with error
        return;
    }
    
    if (!strcmp(node->children[1]->name, "ExtDecList")) {
        assert(node->children_count == 3);
        assert(node->children[2] != NULL);

        _sem_validate_ext_dec_list(node->children[1], type, struct_specifier, NULL);
        // ignore ret_ir! global variables are unused
    }
    else if (!strcmp(node->children[1]->name, "SEMI")) {
        assert(node->children_count == 2);
        // if it is just a specifier
        // then nothing happens
        // if it is a struct definition
        // then we have already inserted the symbol
        // into the symbol table during the validation
        // of struct_specifier
        // thus still nothing happens
        return;
    }
    else if (!strcmp(node->children[1]->name, "FunDec")) {
        assert(node->children_count == 3);
        ir_list *func_header = malloc(sizeof(ir_list));
        func_header->head = NULL;
        func_header->tail = NULL;
        func_entry = _sem_validate_fun_dec(node->children[1], type, struct_specifier, func_header);
        if (!strcmp(node->children[2]->name, "SEMI")) {
            // declaration
            func_entry->is_function_dec = 1;
            // we are not using these!
            if (func_entry->param_count)
                symtable_pop_context_without_free(1);
            symtable_insert(func_entry, 0, 0, 0, 0);
            // No need to generate ir
        }
        else {
            // CompSt
            func_entry->is_function_dec = 0;
            return_type.type = type;
            return_type.is_array = 0;
            return_type.is_lvalue = 0;
            return_type.array_dimension = 0;
            return_type.struct_specifier = struct_specifier;
            if (symtable_insert(func_entry, 0, 0, 0, 0) == -1) { // Insert here to make recursion possible
                return; // error, no need to generate ir
            }
            // in comp_st, symtable will pop symbols
            // at level 1 but those without free will remain
            func_contents = malloc(sizeof(ir_list));
            func_contents->head = NULL;
            func_contents->tail = NULL;
            _sem_validate_comp_st(node->children[2], 0, &return_type, 0, func_contents);
            if (func_contents->head == NULL) {
                // error
                free(func_contents);
                return;
            }
            ir_merge_buffer(func_header, func_contents);
            ir_compress_label(func_header);
            ir_print_list(func_header);
            ret_ir->func_content = func_header;
        }
    }
    else {
        // should not be here
        assert(0);
    }
}

void _sem_validate_comp_st(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir) {
    assert(node->children_count == 4);
    _sem_validate_def_list(node->children[1], context + 1, no_optimization, ret_ir);
    _sem_validate_stmt_list(node->children[2], context + 1, return_type, no_optimization, ret_ir);
    symtable_pop_context(context + 1);
}

void _sem_validate_stmt_list(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir) {
    if (node == NULL) {
        return;
    }

    _sem_validate_stmt(node->children[0], context, return_type, no_optimization, ret_ir);
    _sem_validate_stmt_list(node->children[1], context, return_type, no_optimization, ret_ir);
}

void _sem_validate_stmt(ast_node *node, int context, _sem_exp_type *return_type, char no_optimization, ir_list *ret_ir) {
    _sem_exp_type *exp_type;
    ir *ir_entry;
    ir_list *ir_list_local = malloc(sizeof(ir_list));
    uint32_t goto_label;
    uint32_t goto_label_end;

    ir_list_local->head = NULL;
    ir_list_local->tail = NULL;

    if (!strcmp(node->children[0]->name, "Exp")) {
        if ((exp_type = _sem_validate_exp(node->children[0], no_optimization, ir_list_local)) == NULL) {
            free(ir_list_local);
            return;
        }
        if (exp_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            // not all can be safely ignored!
            // expression like x + y will make no effect
            // but CALL will!
            if (exp_type->immediate_ir->op == IR_OP_CALL) {
                // assign a dummy to it
                exp_type->immediate_ir->temp_id = ir_new_temp_val();
                exp_type->immediate_ir->mode.mode1 = IR_MODE_T;
                exp_type->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ir_list_local, exp_type->immediate_ir);
            }
        }

        ir_merge_buffer(ret_ir, ir_list_local);
        free(ir_list_local);
    }
    if (!strcmp(node->children[0]->name, "CompSt")) {
        _sem_validate_comp_st(node->children[0], context, return_type, no_optimization, ret_ir);
    }
    if (!strcmp(node->children[0]->name, "RETURN")) {
        exp_type = _sem_validate_exp(node->children[1], no_optimization, ir_list_local);
        if (exp_type == NULL) {
            _sem_report_error("Error type 8 at Line %d: Returned an expression with error", node->children[1]->line_number);
            return;
        }
        if (!_sem_type_matching(exp_type, return_type)) {
            _sem_report_error("Error type 8 at Line %d: Return type mismatched", node->children[1]->line_number);
        }
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_RETURN;
        // when no optimization is on
        // exp_type will not be constant
        // unless it does not involve any variables
        if (exp_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            ir_merge_buffer(ret_ir, ir_list_local);
            if (exp_type->immediate_ir->op == IR_EXP_OP_MACCESS) {
                exp_type->immediate_ir = ir_simplify_maccess(exp_type->immediate_ir, ret_ir);
                ir_entry->temp_id = exp_type->immediate_ir->temp_id1;
                ir_entry->var_id = exp_type->immediate_ir->var_id1;
                ir_entry->mode.mode1 = exp_type->immediate_ir->mode.mode2;
                ir_entry->mode.op1 = exp_type->immediate_ir->mode.op2;
            }
            else {
                exp_type->immediate_ir->temp_id = ir_new_temp_val();
                exp_type->immediate_ir->mode.mode1 = IR_MODE_T;
                exp_type->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                ir_add_node_to_buffer(ret_ir, exp_type->immediate_ir);
                ir_entry->temp_id = exp_type->immediate_ir->temp_id;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
            }
        }
        else if (exp_type->constant_exp_status == SEM_CONSTANT_YES) {
            ir_entry->mode.mode1 = IR_MODE_I;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
            if (exp_type->type == SYMBOL_T_INT) {
                ir_entry->int_val1 = exp_type->int_val;
            }
            if (exp_type->type == SYMBOL_T_FLOAT) {
                ir_entry->float_val1 = exp_type->float_val;
            }
            if (exp_type->type == SYMBOL_T_STRUCT) {
                ir_entry->int_val1 = exp_type->int_val;
            }
            // do not merge ir_list_local
            // as the exp is constant
            free(ir_list_local);
        }
        else {
            ir_entry->temp_id = exp_type->ir_temp_val_id;
            ir_entry->var_id = exp_type->ir_var_id;
            if (exp_type->type_mode_op == SEM_TYPE_MODE_NORMAL)
                ir_entry->mode.op1 = IR_MODE_NORMAL;
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_STAR)
                ir_entry->mode.op1 = IR_MODE_STAR;
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_ADDR)
                ir_entry->mode.op1 = IR_MODE_ADDR;
            if (exp_type->type_mode == SEM_TYPE_MODE_T)
                ir_entry->mode.mode1 = IR_MODE_T;
            else if (exp_type->type_mode == SEM_TYPE_MODE_V)
                ir_entry->mode.mode1 = IR_MODE_V;
            ir_merge_buffer(ret_ir, ir_list_local);
        }
        ir_add_node_to_buffer(ret_ir, ir_entry);
        free(exp_type);
        return;
    }
    if (!strcmp(node->children[0]->name, "IF")) {
        // IF LP Exp RP Stmt // ELSE Stmt
        exp_type = _sem_validate_exp(node->children[2], no_optimization, ir_list_local);
        if (exp_type == NULL) {
            // _sem_report_error("Error type 8 at Line %d: Expression with error", node->children[1]->line_number);
            free(ir_list_local);
        }
        else {
            if (exp_type->type != SYMBOL_T_INT || exp_type->is_array) {
                _sem_report_error("Error type 8 at Line %d: INT required in IF statement", node->children[1]->line_number);
            }
        }
        // first we check if the expr is constant
        // only exp without variables can be real constant
        // when no optimization is on
        if (exp_type->constant_exp_status == SEM_CONSTANT_YES) {
            if (exp_type->int_val) {
                // always true
                _sem_validate_stmt(node->children[4], context, return_type, 1, ret_ir);
                if (node->children_count == 7) {
                    // if there is an else ELSE Stmt
                    // yet still need to validate!
                    _sem_validate_stmt(node->children[6], context, return_type, 1, ir_list_local);
                }
                free(exp_type);
                free(ir_list_local);
                return;
            }
            else {
                // always false
                // still need to validate
                _sem_validate_stmt(node->children[4], context, return_type, 1, ir_list_local);
                if (node->children_count == 7) {
                    // if there is an else ELSE Stmt
                    _sem_validate_stmt(node->children[6], context, return_type, 1, ret_ir);
                }
                // if there is no else stmt
                // then the whole branch is ignored
                free(exp_type);
                free(ir_list_local);
                return;
            }
        }

        ir_merge_buffer(ret_ir, ir_list_local);
        ir_entry = malloc(sizeof(ir));
        // now do the inverse of the exp to get to the false branch
        if (exp_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            ir_entry->immediate_ir = exp_type->immediate_ir;
            ir_entry->op = IR_OP_IF_IMME;
            switch(ir_entry->immediate_ir->op) {
                case IR_EXP_OP_EQ:
                    ir_entry->immediate_ir->op = IR_EXP_OP_NEQ;
                    break;
                case IR_EXP_OP_NEQ:
                    ir_entry->immediate_ir->op = IR_EXP_OP_EQ;
                    break;
                case IR_EXP_OP_LT:
                    ir_entry->immediate_ir->op = IR_EXP_OP_GE;
                    break;
                case IR_EXP_OP_LE:
                    ir_entry->immediate_ir->op = IR_EXP_OP_GT;
                    break;
                case IR_EXP_OP_GE:
                    ir_entry->immediate_ir->op = IR_EXP_OP_LT;
                    break;
                case IR_EXP_OP_GT:
                    ir_entry->immediate_ir->op = IR_EXP_OP_LE;
                    break;
                case IR_EXP_OP_MACCESS:
                    exp_type->immediate_ir = ir_simplify_maccess(exp_type->immediate_ir, ret_ir);
                    ir_entry->op = IR_OP_IF;
                    ir_entry->var_id = exp_type->immediate_ir->var_id;
                    ir_entry->temp_id = exp_type->immediate_ir->temp_id;
                    ir_entry->mode.mode1 = exp_type->immediate_ir->mode.mode1;
                    ir_entry->mode.op1 = exp_type->immediate_ir->mode.op1;
                    break;
                default:
                    // generate new temp var
                    ir_entry->op = IR_OP_IF;
                    ir_entry->immediate_ir->temp_id = ir_new_temp_val();
                    ir_entry->immediate_ir->mode.mode1 = IR_MODE_T;
                    ir_entry->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                    ir_add_node_to_buffer(ret_ir, ir_entry->immediate_ir);
                    ir_entry->temp_id = ir_entry->immediate_ir->temp_id;
                    ir_entry->mode.mode1 = IR_MODE_T;
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                    ir_entry->immediate_ir = NULL;
            }
        }
        else {
            // non constant
            if (exp_type->type_mode == SEM_TYPE_MODE_T) {
                ir_entry->op = IR_OP_IF;
                ir_entry->mode.mode1 = IR_MODE_T;
                ir_entry->immediate_ir = NULL;
                ir_entry->temp_id = exp_type->ir_temp_val_id;
            }
            else if (exp_type->type_mode == SEM_TYPE_MODE_V) {
                ir_entry->op = IR_OP_IF;
                ir_entry->mode.mode1 = IR_MODE_V;
                ir_entry->immediate_ir = NULL;
                ir_entry->var_id = exp_type->ir_var_id;
            }

            if (exp_type->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                ir_entry->mode.op1 = IR_MODE_NORMAL;
            }
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_STAR) {
                ir_entry->mode.op1 = IR_MODE_STAR;
            }
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_ADDR) {
                ir_entry->mode.op1 = IR_MODE_ADDR;
            }
        }
        goto_label = ir_new_label();
        ir_entry->goto_label = goto_label;
        ir_add_node_to_buffer(ret_ir, ir_entry);


        _sem_validate_stmt(node->children[4], context, return_type, 1, ret_ir);
        if (node->children_count == 7) {
            // ELSE Stmt
            // add a goto to the end
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_GOTO;
            ir_entry->goto_label = ir_new_label();
            goto_label_end = ir_entry->goto_label;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            // add the else label
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            ir_entry->goto_label = goto_label;
            ir_add_node_to_buffer(ret_ir, ir_entry);
            _sem_validate_stmt(node->children[6], context, return_type, 1, ret_ir);
            // add the end label
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            ir_entry->goto_label = goto_label_end;
            ir_add_node_to_buffer(ret_ir, ir_entry);
        }
        else {
            // add the end label
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            ir_entry->goto_label = goto_label;
            ir_add_node_to_buffer(ret_ir, ir_entry);
        }
        return;
    }
    if (!strcmp(node->children[0]->name, "WHILE")) {
        // WHILE LP Exp RP Stmt
        // set no optimization to 1
        exp_type = _sem_validate_exp(node->children[2], 1, ir_list_local);
        if (exp_type == NULL) {
            // _sem_report_error("Error type 8 at Line %d:  Expression with error", node->children[1]->line_number);
        }
        else {
            if (exp_type->type != SYMBOL_T_INT || exp_type->is_array) {
                _sem_report_error("Error type 8 at Line %d: INT required in WHILE statement", node->children[1]->line_number);
            }
            free(exp_type);
        }

        if (exp_type->constant_exp_status == SEM_CONSTANT_YES) {
            // a real dead loop or a real unused loop is detected
            // we only deal with unused loop
            if (!exp_type->int_val) {
                // nothing will be added
                // the loop will be ignored
                // however considering while (x = y - 1)
                // we have to add the ir_list_local to the ret_ir
                ir_merge_buffer(ret_ir, ir_list_local);
                ir_list_local->head = NULL;
                ir_list_local->tail = NULL;
                // validate
                _sem_validate_stmt(node->children[4], context, return_type, 1, ir_list_local);
                return;
            }
            else {
                // don't have to generate anything but a label
                ir_merge_buffer(ret_ir, ir_list_local);
                // label comes after the merge buffer because
                // if it is constan then the action will always be the same
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_OP_LABEL;
                goto_label = ir_new_label();
                ir_entry->goto_label = goto_label;
                ir_add_node_to_buffer(ret_ir, ir_entry);
                goto_label_end = ir_new_label();
                free(ir_list_local);
            }
        }
        else {
            // add the first label
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_OP_LABEL;
            goto_label = ir_new_label();
            ir_entry->goto_label = goto_label;
            ir_add_node_to_buffer(ret_ir, ir_entry);

            // merge the exp
            ir_merge_buffer(ret_ir, ir_list_local);
            free(ir_list_local);

            ir_entry = malloc(sizeof(ir));
            if (exp_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
                ir_entry->immediate_ir = exp_type->immediate_ir;
                ir_entry->op = IR_OP_IF_IMME;
                switch(ir_entry->immediate_ir->op) {
                    case IR_EXP_OP_EQ:
                        ir_entry->immediate_ir->op = IR_EXP_OP_NEQ;
                        break;
                    case IR_EXP_OP_NEQ:
                        ir_entry->immediate_ir->op = IR_EXP_OP_EQ;
                        break;
                    case IR_EXP_OP_LT:
                        ir_entry->immediate_ir->op = IR_EXP_OP_GE;
                        break;
                    case IR_EXP_OP_LE:
                        ir_entry->immediate_ir->op = IR_EXP_OP_GT;
                        break;
                    case IR_EXP_OP_GE:
                        ir_entry->immediate_ir->op = IR_EXP_OP_LT;
                        break;
                    case IR_EXP_OP_GT:
                        ir_entry->immediate_ir->op = IR_EXP_OP_LE;
                        break;
                    case IR_EXP_OP_MACCESS:
                        ir_entry->immediate_ir = ir_simplify_maccess(ir_entry->immediate_ir, ret_ir);
                        ir_entry->op = IR_OP_IF;
                        ir_entry->var_id = ir_entry->immediate_ir->var_id1;
                        ir_entry->temp_id = ir_entry->immediate_ir->temp_id1;
                        ir_entry->mode.mode1 = ir_entry->immediate_ir->mode.mode2;
                        ir_entry->mode.op1 = ir_entry->immediate_ir->mode.op2;
                        break;
                    default:
                        // generate new temp var
                        ir_entry->op = IR_OP_IF;
                        ir_entry->immediate_ir->temp_id = ir_new_temp_val();
                        ir_entry->immediate_ir->mode.mode1 = IR_MODE_T;
                        ir_entry->immediate_ir->mode.op1 = IR_MODE_NORMAL;
                        ir_add_node_to_buffer(ret_ir, ir_entry->immediate_ir);
                        ir_entry->temp_id = ir_entry->immediate_ir->temp_id;
                        ir_entry->mode.mode1 = IR_MODE_T;
                        ir_entry->mode.op1 = IR_MODE_NORMAL;
                        ir_entry->immediate_ir = NULL;
                }
            }
            else {
                // non constant
                if (exp_type->type_mode == SEM_TYPE_MODE_T) {
                    ir_entry->op = IR_OP_IF;
                    ir_entry->mode.mode1 = IR_MODE_T;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->temp_id = exp_type->ir_temp_val_id;
                }
                else if (exp_type->type_mode == SEM_TYPE_MODE_V) {
                    ir_entry->op = IR_OP_IF;
                    ir_entry->mode.mode1 = IR_MODE_V;
                    ir_entry->immediate_ir = NULL;
                    ir_entry->var_id = exp_type->ir_var_id;
                }

                if (exp_type->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                    ir_entry->mode.op1 = IR_MODE_NORMAL;
                }
                else if (exp_type->type_mode_op == SEM_TYPE_MODE_STAR) {
                    ir_entry->mode.op1 = IR_MODE_STAR;
                }
                else if (exp_type->type_mode_op == SEM_TYPE_MODE_ADDR) {
                    ir_entry->mode.op1 = IR_MODE_ADDR;
                }
            }
            goto_label_end = ir_new_label();
            ir_entry->goto_label = goto_label_end;
            ir_add_node_to_buffer(ret_ir, ir_entry);
        }

        // do not optimize
        _sem_validate_stmt(node->children[4], context, return_type, 1, ret_ir);
        // add goto the top
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_GOTO;
        ir_entry->goto_label = goto_label;
        ir_add_node_to_buffer(ret_ir, ir_entry);
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_LABEL;
        ir_entry->goto_label = goto_label_end;
        ir_add_node_to_buffer(ret_ir, ir_entry);
        return;
    }
}

// will return type, and set struct_list to the parsed struct list
// return SYMBOL_T_ERROR on error
int _sem_validate_specifier(ast_node *node, struct_specifier **struct_specifier, int context, int do_not_free) {
    assert(node->children_count == 1);

    if (!strcmp(node->children[0]->name, "StructSpecifier")) {
        // parse struct specifier
        *struct_specifier = _sem_validate_struct_specifier(node->children[0], context, do_not_free);
        if (*struct_specifier == NULL)
            return SYMBOL_T_ERROR;
        return SYMBOL_T_STRUCT;
    }
    else if (!strcmp(node->children[0]->string_value, "int")) {
        *struct_specifier = NULL;
        return SYMBOL_T_INT;
    } else if (!strcmp(node->children[0]->string_value, "float")) {
        *struct_specifier = NULL;
        return SYMBOL_T_FLOAT;
    }
    else {
        assert(0);
        return SYMBOL_T_ERROR;
    }
}

void _sem_validate_ext_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, ir_list *ret_ir) {
    assert(!strcmp(node->name, "ExtDecList"));
    symbol_entry *ins_entry;
    ir *ir_entry;
    int i = 0;

    
    ins_entry = _sem_validate_var_dec(node->children[0]);
    ins_entry->type = type;
    if (type == SYMBOL_T_INT)
        ins_entry->size = 4;
    if (type == SYMBOL_T_INT)
        ins_entry->size = 8;
    if (type == SYMBOL_T_STRUCT)
        ins_entry->size = struct_specifier->size;
    ins_entry->struct_specifier = struct_specifier;
    ins_entry->ir_variable_id = ir_new_variable();

    // we do not have ext dec and ir does not support ext dec
    // so we will not generate code for it
    /*
    ir_entry = malloc(sizeof(ir));
    ir_entry->op = IR_OP_DEC;
    ir_entry->var_id = ins_entry->ir_variable_id;
    ir_entry->size = ins_entry->size;
    if (ins_entry->is_array) {
        for (i = 0; i < ins_entry->array_dimention; i++) {
            ir_entry->size *= ins_entry->array_size[i];
        }
    }
    
    ins_entry->size = ir_entry->int_val1;*/
    //ir_add_node_to_buffer(ret_ir, ir_entry);
    if (symtable_insert(ins_entry, 0, 0, 0, 0)) {
        free(ins_entry);
        // failed to insert
        // but should not stop
    }
    if (node->children_count == 3) { // VarDec COMMA ExtDecList
        _sem_validate_ext_dec_list(node->children[2], type, struct_specifier, ret_ir);
    }
}

// return a symbol_entry with its id and array information initialized
symbol_entry *_sem_validate_var_dec(ast_node *node) {
    symbol_entry *ret_entry = malloc(sizeof(symbol_entry));
    symbol_entry *sub_entry;

    if (node->children_count == 1) { // ID
        assert(!strcmp(node->children[0]->name, "ID"));
        ret_entry->is_array = 0;
        ret_entry->array_dimention = 0;
        ret_entry->is_function = 0;
        ret_entry->is_function_dec = 0;
        ret_entry->param_count = 0;
        ret_entry->params = NULL;
        ret_entry->line_no_def = node->line_number;
        ret_entry->id = node->children[0]->string_value;
        //ret_entry->ir_variable_id = ir_new_variable();
        return ret_entry;
    }
    else if (node->children_count == 4) { // VarDec LB INT RB
        assert(!strcmp(node->children[0]->name, "VarDec"));
        assert(!strcmp(node->children[2]->name, "INT"));
        sub_entry = _sem_validate_var_dec(node->children[0]);
        ret_entry->is_array = 1;
        ret_entry->array_dimention = sub_entry->array_dimention + 1;
        ret_entry->array_size = malloc(ret_entry->array_dimention * sizeof(int));
        if (sub_entry->is_array) {
            memcpy(&(ret_entry->array_size[1]), sub_entry->array_size, sub_entry->array_dimention * sizeof(int));
            free(sub_entry->array_size);
        }
        ret_entry->is_function = 0;
        ret_entry->is_function_dec = 0;
        ret_entry->param_count = 0;
        ret_entry->params = NULL;
        ret_entry->line_no_def = node->line_number;
        ret_entry->id = sub_entry->id;
        ret_entry->array_size[0] = node->children[2]->int_value;
        free(sub_entry);
        return ret_entry;
    }
    else {
        assert(0);
        return NULL;
    }
}

// return a struct specifier; return NULL on error
struct_specifier *_sem_validate_struct_specifier(ast_node *node, int context, int do_not_free) {
    struct_specifier *ret_specifier = malloc(sizeof(struct_specifier));
    symbol_entry *ins_specifier_symbol;
    uint32_t struct_size = 0;
    symbol_list *size_iterator;

    assert(!strcmp(node->children[0]->name, "STRUCT"));
    // STRUCT OptTag LC DefList RC
    if (node->children_count == 5) {
        // here fully defines a struct
        if (node->children[1] != NULL) {
            // has a name
            // thus shall not be found in the same context
            // OptTag->ID->string_value
            ret_specifier->struct_tag = node->children[1]->children[0]->string_value;
        }
        else {
            // name it with a number thus
            // no one can access it but yet still can be managed
            // by the symbol table
            char *unnamed_tag_id = malloc(100 * sizeof(char));
            snprintf(unnamed_tag_id, 100, "%d", _sem_unnamed_struct_count++);
            ret_specifier->struct_tag = unnamed_tag_id;
        }
        // can be considered as a new context created by {}
        // not allowing initialization to variables
        ret_specifier->struct_contents = _sem_validate_def_list_for_struct(node->children[3], context + 1);
        symtable_pop_context_without_free(context + 1); // struct context is over
                                                        // but we shall not release the resources
                                                        // since the symbols are still in use!
        if ((int)(ret_specifier->struct_contents) == -1) {
            // def list returned with error
            free(ret_specifier);
            return NULL;
        }

        // cannot be NULL!
        // if (ret_specifier->struct_tag != NULL) {
        
        size_iterator = ret_specifier->struct_contents;
        while (size_iterator != NULL) {
            struct_size += size_iterator->symbol->size;
            size_iterator = size_iterator->next;
        }
        ret_specifier->size = struct_size;
        
        // needs to be inserted into current symbol table
        ins_specifier_symbol = malloc(sizeof(symbol_entry));
        ins_specifier_symbol->type = SYMBOL_T_STRUCT_DEFINE;
        ins_specifier_symbol->is_array = 0;
        ins_specifier_symbol->array_dimention = 0;
        ins_specifier_symbol->id = ret_specifier->struct_tag;
        ins_specifier_symbol->is_function = 0;
        ins_specifier_symbol->is_function_dec = 0;
        ins_specifier_symbol->param_count = 0;
        ins_specifier_symbol->params = NULL;
        ins_specifier_symbol->line_no_def = node->children[4]->line_number;
        ins_specifier_symbol->struct_specifier = ret_specifier;
        ins_specifier_symbol->size = struct_size;
        if (symtable_insert(ins_specifier_symbol, context, 0, do_not_free, 0)) { // it is not a field so we don't care
                                                                 // if it's in_struct
            // insertion error
            symtable_free_symbol(ins_specifier_symbol);
            return NULL;
        }
        return ret_specifier;
        //}
    } 
    // STRUCT Tag
    else if (node->children_count == 2) {
        // we do not save the current struct specifier
        // since it was defined previously
        // we only have to query the symbol table
        // to get the real struct specifier and return it

        // query if Tag exists
        // if not exists then it shall be an error
        symbol_entry *defined_tag = symtable_query(node->children[1]->children[0]->string_value);
        if (defined_tag == NULL || defined_tag->type != SYMBOL_T_STRUCT_DEFINE) {
            _sem_report_error("Error type 17 at Line %d: Undefined structure \"%s\"", node->children[1]->line_number, node->children[1]->children[0]->string_value);
            return NULL;
        }
        free(ret_specifier);
        return defined_tag->struct_specifier;
    } 
    else {
        assert (0);
        return NULL;
    }
}

symbol_entry *_sem_validate_fun_dec(ast_node *node, int type, struct_specifier *struct_specifier, ir_list *ret_ir) {
    symbol_entry *ret_entry = malloc(sizeof(symbol_entry));
    symbol_list *param_list = NULL;
    ir *ir_entry = malloc(sizeof(ir));

    ret_entry->type = type;
    ret_entry->struct_specifier = struct_specifier;
    ret_entry->is_function = 1;
    ret_entry->is_array = 0;
    ret_entry->line_no_def = node->line_number;
    ret_entry->type = type;
    if (type == SYMBOL_T_INT)
        ret_entry->size = 4;
    if (type == SYMBOL_T_INT)
        ret_entry->size = 8;
    if (type == SYMBOL_T_STRUCT)
        ret_entry->size = struct_specifier->size;

    // ID LP RP
    // ID LP VarList RP

    // ID
    ret_entry->id = node->children[0]->string_value;


    ir_entry->op = IR_OP_FUNC;
    ir_entry->func_name = ret_entry->id;
    ir_entry->size = ret_entry->size;
    ir_add_node_to_buffer(ret_ir, ir_entry);
    
    
    if (node->children_count == 3) { // ID LP RP
        ret_entry->param_count = 0;
        ret_entry->params = NULL;
    } 
    else if (node->children_count == 4) {
        ret_entry->param_count = _sem_validate_var_list(node->children[2], &param_list, ret_ir);
        ret_entry->params = param_list;
    }
    else {
        assert(0);
        return NULL;
    }

    return ret_entry;
}

int _sem_validate_var_list(ast_node *node, symbol_list **param_list, ir_list *ret_ir) {
    symbol_list *ret_list;
    ir *ir_entry = malloc(sizeof(ir));
    int ret_val = 0;
    // ParamDec
    symbol_entry *current_entry = _sem_validate_param_dec(node->children[0]);
    ir_entry->op = IR_OP_PARAM;
    ir_entry->var_id = current_entry->ir_variable_id;
    ir_entry->size = current_entry->size;
    ir_add_node_to_buffer(ret_ir, ir_entry);

    symbol_list *ret_list_tail = NULL;
    if (node->children_count == 3) {
        ret_val = _sem_validate_var_list(node->children[2], &ret_list_tail, ret_ir);
    }
    if (current_entry == NULL) {
        if (ret_val == 0) {
            *param_list = NULL;
            return 0;
        }
        else {
            *param_list = ret_list_tail;
            return ret_val;
        }
    }
    else {
        ret_list = malloc(sizeof(symbol_list));
        ret_list->symbol = current_entry;
        ret_list->next = ret_list_tail;
        *param_list = ret_list;
        return ret_val + 1;
    }
}

// return the paresed symbol; return NULL on error
symbol_entry *_sem_validate_param_dec(ast_node *node) {
    struct_specifier *struct_specifier = NULL;
    symbol_entry *ret_entry;
    int iterator;
    // here we set context to 1 due to the params are actually the same as 
    // "local variables" to a function
    int type = _sem_validate_specifier(node->children[0], &struct_specifier, 1, 1);
    if (type == SYMBOL_T_ERROR) {
        return NULL;
    }

    // VarDec
    ret_entry = _sem_validate_var_dec(node->children[1]);
    ret_entry->type = type;
    ret_entry->struct_specifier = struct_specifier;
    ret_entry->ir_variable_id = ir_new_variable();
    ret_entry->is_constant = IR_UNDECIDED;

    if (ret_entry->type == SYMBOL_T_STRUCT) {
        ret_entry->size = struct_specifier->size;
        if (ret_entry->is_array) {
            for (iterator = 0; iterator < ret_entry->array_dimention; iterator++) {
                ret_entry->size *= ret_entry->array_size[iterator];
            }
        }
    }
    else if (ret_entry->is_array) {
        if (type == SYMBOL_T_INT) {
            ret_entry->size = 4;
        }
        else {
            ret_entry->size = 8;
        }
        for (iterator = 0; iterator < ret_entry->array_dimention; iterator++) {
            ret_entry->size *= ret_entry->array_size[iterator];
        }
    }

    // try to insert!
    if (symtable_insert(ret_entry, 1, 0, 1, 1)) {
        symtable_free_symbol(ret_entry);
        return NULL;
    }

    return ret_entry;
}

void _sem_validate_def_list(ast_node *node, int context, char no_optimization, ir_list *ret_ir) {
    if (node == NULL) {
        return;
    }
    _sem_validate_def(node->children[0], context, no_optimization, ret_ir);
    _sem_validate_def_list(node->children[1], context, no_optimization, ret_ir);
}

void _sem_validate_def(ast_node *node, int context, char no_optimization, ir_list *ret_ir) {
    struct_specifier *struct_specifier = NULL;
    int type = _sem_validate_specifier(node->children[0], &struct_specifier, context, 0);
    if (type == SYMBOL_T_ERROR) {
        // specifier validation failed with error
        return;
    }

    _sem_validate_dec_list(node->children[1], type, struct_specifier, context, no_optimization, ret_ir);
}

void _sem_validate_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, int context, char no_optimization, ir_list *ret_ir) {
    symbol_entry *ins_entry;

    // Dec
    ins_entry = _sem_validate_dec(node->children[0], type, struct_specifier, no_optimization, ret_ir);
    if (ins_entry != NULL) {
        if (symtable_insert(ins_entry, context, 0, 0, 0)) {
            // Falied to insert
            symtable_free_symbol(ins_entry);
        }
    }
    
    if (node->children_count == 3) { // Dec COMMA DecList
        _sem_validate_dec_list(node->children[2], type, struct_specifier, context, no_optimization, ret_ir);
    }
    return;
}

char _sem_type_matching_with_symbol(symbol_entry *se, _sem_exp_type *et) {
    char ret_val;
    ret_val = (se->type == et->type);
    if (se->type == SYMBOL_T_STRUCT) {
        ret_val = (ret_val && symtable_param_struct_compare(et->struct_specifier->struct_contents, se->struct_specifier->struct_contents));
    }
    ret_val = (ret_val && (et->is_array == se->is_array));
    if (se->is_array) {
        ret_val = (ret_val && (et->is_array));
        ret_val = (ret_val && (se->array_dimention == et->array_dimension));
    }
    return ret_val;
}

symbol_entry *_sem_validate_dec(ast_node *node, int type, struct_specifier *parsed_struct_specifier, char no_optimization, ir_list *ret_ir) {
    struct_specifier *exp_struct_specifier;
    char dummy;
    int iterator;
    _sem_exp_type *exp_type;
    ir *ir_entry;
    ir_list *ir_list_local;
    
    symbol_entry *ret_entry = _sem_validate_var_dec(node->children[0]);
    ret_entry->type = type;
    ret_entry->struct_specifier = parsed_struct_specifier;
    ret_entry->ir_variable_id = ir_new_variable();
    ret_entry->is_constant = IR_NON_CONSTANT;
    if (ret_entry->type == SYMBOL_T_STRUCT) {
        ir_entry = malloc(sizeof(ir));
        ir_entry->op = IR_OP_DEC;
        ir_entry->var_id = ret_entry->ir_variable_id;
        ir_entry->size = parsed_struct_specifier->size;
        if (ret_entry->is_array) {
            for (iterator = 0; iterator < ret_entry->array_dimention; iterator++) {
                ir_entry->size *= ret_entry->array_size[iterator];
            }
        }
        ret_entry->size = ir_entry->size;
        ir_add_node_to_buffer(ret_ir, ir_entry);
    }
    else if (ret_entry->is_array) {
        ir_entry = malloc(sizeof(ir));
        ir_entry->var_id = ret_entry->ir_variable_id;
        ir_entry->op = IR_OP_DEC;
        if (type == SYMBOL_T_INT) {
            ir_entry->size = 4;
        }
        else {
            ir_entry->size = 8;
        }
        for (iterator = 0; iterator < ret_entry->array_dimention; iterator++) {
            ir_entry->size *= ret_entry->array_size[iterator];
        }
        ret_entry->size = ir_entry->size;
        ir_add_node_to_buffer(ret_ir, ir_entry);
    }

    if (node->children_count == 3) { // VarDec ASSIGNOP Exp
        ir_list_local = malloc(sizeof(ir_list));
        ir_list_local->head = NULL;
        ir_list_local->tail = NULL;
        exp_type = _sem_validate_exp(node->children[2], no_optimization, ir_list_local);
        if (exp_type == NULL) {
            free(ir_list_local);
            return NULL;
        }
        if (!_sem_type_matching_with_symbol(ret_entry, exp_type)) {
            // Exp does not match the type of Def
            free(exp_type);
            free(ret_entry);
            free(ir_list_local);
            _sem_report_error("Error type 5 at Line %d: Type mismatched for assignment", node->children[1]->line_number);
            return NULL;
        }
        // deal with ret val
        if (exp_type->constant_exp_status == SEM_CONSTANT_YES) {
            if (type == SYMBOL_T_FLOAT) {
                
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->mode.mode1 = IR_MODE_V;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_entry->var_id = ret_entry->ir_variable_id;
                ir_entry->float_val1 = exp_type->float_val;
                ir_add_node_to_buffer(ret_ir, ir_entry);
                
                
                ret_entry->is_constant = IR_CONSTANT;
                ret_entry->float_val = exp_type->float_val;
            }
            else { // all 4 byte
                
                ir_entry = malloc(sizeof(ir));
                ir_entry->op = IR_EXP_OP_ASSIGN;
                ir_entry->mode.mode1 = IR_MODE_V;
                ir_entry->mode.op1 = IR_MODE_NORMAL;
                ir_entry->mode.mode2 = IR_MODE_I;
                ir_entry->mode.op2 = IR_MODE_NORMAL;
                ir_entry->var_id = ret_entry->ir_variable_id;
                ir_entry->int_val1 = exp_type->int_val;
                ir_add_node_to_buffer(ret_ir, ir_entry);
                
                ret_entry->is_constant = IR_CONSTANT;
                ret_entry->int_val = exp_type->int_val;
            }
        }
        else if (exp_type->constant_exp_status == SEM_CONSTANT_IMMEDIATE) {
            if (exp_type->immediate_ir->op == IR_EXP_OP_MACCESS) {
                exp_type->immediate_ir = ir_simplify_maccess(exp_type->immediate_ir, ret_ir);
                exp_type->immediate_ir->op = IR_EXP_OP_ASSIGN;
            }
            exp_type->immediate_ir->var_id = ret_entry->ir_variable_id;
            exp_type->immediate_ir->mode.mode1 = IR_MODE_V;
            exp_type->immediate_ir->mode.op1 = IR_MODE_NORMAL;
            
            ir_add_node_to_buffer(ret_ir, exp_type->immediate_ir);
            ret_entry->is_constant = IR_NON_CONSTANT;
        }
        else {
            // non constat
            // things like:
            //    int a = b = c + d
            // remember b will not be temp var!
            assert(exp_type->type_mode == SEM_TYPE_MODE_V);
            ir_entry = malloc(sizeof(ir));
            ir_entry->op = IR_EXP_OP_ASSIGN;
            ir_entry->var_id = ret_entry->ir_variable_id;
            ir_entry->mode.mode1 = IR_MODE_V;
            ir_entry->mode.op1 = IR_MODE_NORMAL;
            ir_entry->mode.mode2 = IR_MODE_V;
            ir_entry->var_id1 = exp_type->ir_var_id;
            if (exp_type->type_mode_op == SEM_TYPE_MODE_NORMAL) {
                ir_entry->mode.op2 = IR_MODE_NORMAL;
            }
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_STAR) {
                ir_entry->mode.op2 = IR_MODE_STAR;
            }
            else if (exp_type->type_mode_op == SEM_TYPE_MODE_ADDR) {
                ir_entry->mode.op2 = IR_MODE_ADDR;
            }
            ir_add_node_to_buffer(ret_ir, ir_entry);
            ret_entry->is_constant = IR_NON_CONSTANT;
        }
    }
    return ret_entry;
}


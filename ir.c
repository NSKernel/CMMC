/*
    C-- Compiler Back End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    ir.h
    Intermediate code definitions
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ir.h>
#include <debug.h>
#include <global.h>

int _ir_variable_offset = 0;
// uint32_t _ir_temp_val_count = 0;
uint32_t _ir_label_count = 0;

void _ir_print_3(ir *ir_content) {
    // print first
    switch (ir_content->mode.op1) {
        case IR_MODE_ADDR:
            fprintf(output_file, "&");
            break;
        case IR_MODE_STAR:
            fprintf(output_file, "*");
            break;
    }
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
            fprintf(output_file, "t%d := ", ir_content->temp_id);
            break;
        case IR_MODE_V:
            fprintf(output_file, "v%d := ", ir_content->var_id);
            break;
    }
    switch (ir_content->mode.op2) {
        case IR_MODE_ADDR:
            fprintf(output_file, "&");
            break;
        case IR_MODE_STAR:
            fprintf(output_file, "*");
            break;
    }
    switch (ir_content->mode.mode2) {
        case IR_MODE_I:
            fprintf(output_file, "#%d", ir_content->int_val1);
            break;
        case IR_MODE_T:
            fprintf(output_file, "t%d", ir_content->temp_id1);
            break;
        case IR_MODE_V:
            fprintf(output_file, "v%d", ir_content->var_id1);
            break;
    }
    switch (ir_content->op) {
        case IR_EXP_OP_ADD:
            fprintf(output_file, " + ");
            break;
        case IR_EXP_OP_AND:
            fprintf(output_file, " && ");
            break;
        case IR_EXP_OP_DIV:
            fprintf(output_file, " / ");
            break;
        case IR_EXP_OP_EQ:
            fprintf(output_file, " == ");
            break;
        case IR_EXP_OP_GE:
            fprintf(output_file, " >= ");
            break;
        case IR_EXP_OP_GT:
            fprintf(output_file, " > ");
            break;
        case IR_EXP_OP_LE:
            fprintf(output_file, " <= ");
            break;
        case IR_EXP_OP_LT:
            fprintf(output_file, " < ");
            break;
        case IR_EXP_OP_MINUS:
            fprintf(output_file, " - ");
            break;
        case IR_EXP_OP_MUL:
            fprintf(output_file, " * ");
            break;
        case IR_EXP_OP_NEQ:
            fprintf(output_file, " != ");
            break;
        case IR_EXP_OP_OR:
            fprintf(output_file, " || ");
            break;
    }
    switch (ir_content->mode.op3) {
        case IR_MODE_ADDR:
            fprintf(output_file, "&");
            break;
        case IR_MODE_STAR:
            fprintf(output_file, "*");
            break;
    }
    switch (ir_content->mode.mode3) {
        case IR_MODE_I:
            fprintf(output_file, "#%d", ir_content->int_val2);
            break;
        case IR_MODE_T:
            fprintf(output_file, "t%d", ir_content->temp_id2);
            break;
        case IR_MODE_V:
            fprintf(output_file, "v%d", ir_content->var_id2);
            break;
    }
    fprintf(output_file, "\n");
}

void _ir_print_2(ir *ir_content) {
    switch (ir_content->mode.op1) {
        case IR_MODE_ADDR:
            fprintf(output_file, "&");
            break;
        case IR_MODE_STAR:
            fprintf(output_file, "*");
            break;
    }
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
            fprintf(output_file, "t%d := ", ir_content->temp_id);
            break;
        case IR_MODE_V:
            fprintf(output_file, "v%d := ", ir_content->var_id);
            break;
    }
    if (ir_content->op == IR_EXP_OP_NOT) {
        fprintf(output_file, "!");
    }
    switch (ir_content->mode.op2) {
        case IR_MODE_ADDR:
            fprintf(output_file, "&");
            break;
        case IR_MODE_STAR:
            fprintf(output_file, "*");
            break;
    }
    switch (ir_content->mode.mode2) {
        case IR_MODE_I:
            fprintf(output_file, "#%d", ir_content->int_val1);
            break;
        case IR_MODE_T:
            fprintf(output_file, "t%d", ir_content->temp_id1);
            break;
        case IR_MODE_V:
            fprintf(output_file, "v%d", ir_content->var_id1);
            break;
    }
    fprintf(output_file, "\n");
}

void _ir_print_placeholder(int num, char op, char mode) {
    if (op == IR_MODE_ADDR) {
        fprintf(output_file, "&");
    }
    else if (op == IR_MODE_STAR) {
        fprintf(output_file, "*");
    }
    switch (mode) {
        case IR_MODE_I:
            fprintf(output_file, "#");
            break;
        case IR_MODE_T:
            fprintf(output_file, "t");
            break;
        case IR_MODE_V:
            fprintf(output_file, "v");
            break;
    }
    fprintf(output_file, "%d", num);
}

void _ir_print_and(ir *ir_content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode2) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
    }
    goto_label = ir_new_label();
    fprintf(output_file, " == #0 GOTO label%d\n", goto_label);
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode3) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
    }
    fprintf(output_file, " == #0 GOTO label%d\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #1\n");
    goto_label_end = ir_new_label();
    fprintf(output_file, "GOTO label%d\n", goto_label_end);
    fprintf(output_file, "LABEL label%d :\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #0\n");
    fprintf(output_file, "LABEL label%d :\n", goto_label_end);
}

void _ir_print_or(ir *ir_content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode2) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
    }
    goto_label = ir_new_label();
    fprintf(output_file, " != #0 GOTO label%d\n", goto_label);
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode3) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
    }
    fprintf(output_file, " != #0 GOTO label%d\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #0\n");
    goto_label_end = ir_new_label();
    fprintf(output_file, "GOTO label%d\n", goto_label_end);
    fprintf(output_file, "LABEL label%d :\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #1\n");
    fprintf(output_file, "LABEL label%d :\n", goto_label_end);
}

void _ir_print_relop(ir *ir_content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode2) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
    }
    switch (ir_content->op) {
        case IR_EXP_OP_EQ:
            fprintf(output_file, " == ");
            break;
        case IR_EXP_OP_GE:
            fprintf(output_file, " >= ");
            break;
        case IR_EXP_OP_GT:
            fprintf(output_file, " > ");
            break;
        case IR_EXP_OP_LE:
            fprintf(output_file, " <= ");
            break;
        case IR_EXP_OP_LT:
            fprintf(output_file, " < ");
            break;
        case IR_EXP_OP_NEQ:
            fprintf(output_file, " != ");
            break;
    }
    switch (ir_content->mode.mode3) {
        case IR_MODE_I:
            _ir_print_placeholder(ir_content->int_val2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id2, ir_content->mode.op3, ir_content->mode.mode3);
            break;
    }
    goto_label = ir_new_label();
    fprintf(output_file, " GOTO label%d\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #0\n");
    goto_label_end = ir_new_label();
    fprintf(output_file, "GOTO label%d\n", goto_label_end);
    fprintf(output_file, "LABEL label%d :\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #1\n");
    fprintf(output_file, "LABEL label%d :\n", goto_label_end);
}

void _ir_print_not(ir *ir_content) {
    uint32_t goto_label = ir_new_label();
    uint32_t goto_label_end = ir_new_label();
    fprintf(output_file, "IF ");
    switch (ir_content->mode.mode2) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id1, ir_content->mode.op2, ir_content->mode.mode2);
            break;
    }
    
    fprintf(output_file, " == #0 GOTO label%d\n", goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #0\nGOTO label%d\nLABEL label%d :\n", goto_label_end, goto_label);
    switch (ir_content->mode.mode1) {
        case IR_MODE_T:
             _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
        case IR_MODE_V:
            _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
            break;
    }
    fprintf(output_file, " := #1\nLABEL label%d :\n", goto_label_end);
}

void _ir_print_ir(ir *ir_content) {
    switch (ir_content->op) {
        case IR_EXP_OP_ADD:
        case IR_EXP_OP_DIV:
        case IR_EXP_OP_MINUS:
        case IR_EXP_OP_MUL:
            _ir_print_3(ir_content);
            break;
        case IR_EXP_OP_EQ:
        case IR_EXP_OP_GE:
        case IR_EXP_OP_GT:
        case IR_EXP_OP_LE:
        case IR_EXP_OP_LT:
        case IR_EXP_OP_NEQ:
            _ir_print_relop(ir_content);
            break;
        case IR_EXP_OP_AND:
            _ir_print_and(ir_content);
            break;
        case IR_EXP_OP_OR:
            _ir_print_or(ir_content);
            break;
        case IR_EXP_OP_ASSIGN:
            _ir_print_2(ir_content);
            break;    
        case IR_EXP_OP_NOT:
            _ir_print_not(ir_content);
            break;
        case IR_OP_ARG:
            fprintf(output_file, "ARG ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_I:
                    _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, "\n");
            break;
        case IR_OP_CALL:
            switch(ir_content->mode.mode1) {
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, " := CALL %s\n", ir_content->func_name);
            break;
        case IR_OP_DEC:
            fprintf(output_file, "DEC v%d %d\n", ir_content->var_id, ir_content->size);
            break;
        case IR_OP_FUNC:
            fprintf(output_file, "FUNCTION %s :\n", ir_content->func_name);
            break;
        case IR_OP_GOTO:
            fprintf(output_file, "GOTO label%d\n", ir_content->goto_label);
            break;
        case IR_OP_IF:
            fprintf(output_file, "IF ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, " == #0 GOTO label%d\n", ir_content->goto_label);
            break;
        case IR_OP_IF_POSITIVE:
            fprintf(output_file, "IF ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, " != #0 GOTO label%d\n", ir_content->goto_label);
            break;
        case IR_OP_IF_IMME:
            fprintf(output_file, "IF ");
            switch (ir_content->immediate_ir->mode.mode2) {
                case IR_MODE_I:
                    _ir_print_placeholder(ir_content->immediate_ir->int_val1, ir_content->immediate_ir->mode.op2, ir_content->immediate_ir->mode.mode2);
                    break;
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->immediate_ir->temp_id1, ir_content->immediate_ir->mode.op2, ir_content->immediate_ir->mode.mode2);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->immediate_ir->var_id1, ir_content->immediate_ir->mode.op2, ir_content->immediate_ir->mode.mode2);
                    break;
            }
            switch (ir_content->immediate_ir->op) {
                case IR_EXP_OP_EQ:
                    fprintf(output_file, " == ");
                    break;
                case IR_EXP_OP_GE:
                    fprintf(output_file, " >= ");
                    break;
                case IR_EXP_OP_GT:
                    fprintf(output_file, " > ");
                    break;
                case IR_EXP_OP_LE:
                    fprintf(output_file, " <= ");
                    break;
                case IR_EXP_OP_LT:
                    fprintf(output_file, " < ");
                    break;
                case IR_EXP_OP_NEQ:
                    fprintf(output_file, " != ");
                    break;
                default:
                    fprintf(output_file, " 0x%X ", ir_content->op);
            }
            switch (ir_content->immediate_ir->mode.mode3) {
                case IR_MODE_I:
                    _ir_print_placeholder(ir_content->immediate_ir->int_val2, ir_content->immediate_ir->mode.op3, ir_content->immediate_ir->mode.mode3);
                    break;
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->immediate_ir->temp_id2, ir_content->immediate_ir->mode.op3, ir_content->immediate_ir->mode.mode3);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->immediate_ir->var_id2, ir_content->immediate_ir->mode.op3, ir_content->immediate_ir->mode.mode3);
                    break;
            }
            fprintf(output_file, "  GOTO label%d\n", ir_content->goto_label);
            break;
        case IR_OP_LABEL:
            fprintf(output_file, "LABEL label%d :\n", ir_content->goto_label);
            break;
        case IR_OP_PARAM:
            fprintf(output_file, "PARAM v%d\n", ir_content->var_id);
            break;
        case IR_OP_RETURN:
            fprintf(output_file, "RETURN ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_I:
                    _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, "\n");
            break;
        case IR_OP_READ:
            fprintf(output_file, "READ ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, "\n");
            break;
        case IR_OP_WRITE:
            fprintf(output_file, "WRITE ");
            switch (ir_content->mode.mode1) {
                case IR_MODE_I:
                    _ir_print_placeholder(ir_content->int_val1, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_T:
                    _ir_print_placeholder(ir_content->temp_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
                case IR_MODE_V:
                    _ir_print_placeholder(ir_content->var_id, ir_content->mode.op1, ir_content->mode.mode1);
                    break;
            }
            fprintf(output_file, "\n");
            break;
        default:
            fprintf(output_file, "SHIT HAPPENS, %d\n", ir_content->op);
            printf("SHIT HAPPENS, %d\n", ir_content->op);
            break;
    }
    
}

void ir_add_node_to_buffer(ir_list *buffer, ir *ir_content) {
    ir_node *newnode = malloc(sizeof(ir_node));
//#ifdef DEBUG
    //_ir_print_ir(ir_content);
//#endif
    newnode->next = NULL;
    newnode->content = ir_content;
    if (buffer->head == NULL) {
        buffer->head = buffer->tail = newnode;
    }
    else {
        buffer->tail->next = newnode;
        buffer->tail = newnode;
    }
}

void ir_print_list(ir_list *buffer) {
    ir_node *iterator = buffer->head;
    while (iterator != NULL) {
        _ir_print_ir(iterator->content);
        iterator = iterator->next;
    }
}

int ir_new_variable(int size) {
    _ir_variable_offset += size;
    return _ir_variable_offset;
}

int ir_new_temp_val(int size) {
    _ir_variable_offset += size;
    return _ir_variable_offset;
}

uint32_t ir_new_label() {
    return _ir_label_count++;
}

int ir_stack_size() {
    return _ir_variable_offset;
}

void ir_reset_counter() {
    _ir_variable_offset = 0;
}

ir *ir_simplify_maccess(ir *old_ir, ir_list *ret_ir) {
    uint32_t temp_var_reg;
    ir *ret_entry;
    assert(old_ir->op == IR_EXP_OP_MACCESS);
    if (old_ir->int_val2 != 0) {
        old_ir->op = IR_EXP_OP_ADD;
        old_ir->temp_id = ir_new_temp_val(4);
        temp_var_reg = old_ir->temp_id;
        old_ir->mode.mode1 = IR_MODE_T;
        old_ir->mode.op1 = IR_MODE_NORMAL;
        ir_add_node_to_buffer(ret_ir, old_ir);
        ret_entry = malloc(sizeof(ir));
        ret_entry->op = IR_EXP_OP_MACCESS;
        ret_entry->temp_id1 = temp_var_reg;
        ret_entry->mode.mode2 = IR_MODE_T;
        ret_entry->mode.op2 = IR_MODE_STAR;
    }
    else {
        if (old_ir->mode.op2 == IR_MODE_ADDR) {
            old_ir->mode.op2 = IR_MODE_NORMAL;
        }
        else if (old_ir->mode.op2 == IR_MODE_NORMAL) {
            old_ir->mode.op2 = IR_MODE_STAR;
        }
        else if (old_ir->mode.op2 == IR_MODE_STAR) {
            old_ir->temp_id = ir_new_temp_val(4);
            old_ir->op = IR_EXP_OP_ASSIGN;
            temp_var_reg = old_ir->temp_id;
            old_ir->mode.mode1 = IR_MODE_T;
            old_ir->mode.op1 = IR_MODE_NORMAL;
            ir_add_node_to_buffer(ret_ir, old_ir);
            old_ir = malloc(sizeof(ir));
            old_ir->op = IR_EXP_OP_MACCESS;
            old_ir->temp_id1 = temp_var_reg;
            old_ir->mode.mode2 = IR_MODE_T;
            old_ir->mode.op2 = IR_MODE_STAR;
        }
        ret_entry = old_ir;
    }
    return ret_entry;
}

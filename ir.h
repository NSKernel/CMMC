/*
    C-- Compiler Back End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    ir.h
    Intermediate code definitions
*/

#include <stdint.h>

#ifndef IR_H
#define IR_H

#define IR_EXP_OP_ADD     0x01
#define IR_EXP_OP_MINUS   0x02
#define IR_EXP_OP_MUL     0x03
#define IR_EXP_OP_DIV     0x04
#define IR_EXP_OP_GT      0x05
#define IR_EXP_OP_GE      0x06
#define IR_EXP_OP_EQ      0x07
#define IR_EXP_OP_LE      0x08
#define IR_EXP_OP_LT      0x09
#define IR_EXP_OP_NEQ     0x0A
#define IR_EXP_OP_NOT     0x0B
#define IR_EXP_OP_OR      0x0C
#define IR_EXP_OP_AND     0x0D
#define IR_EXP_OP_ASSIGN  0x0E
#define IR_EXP_OP_MACCESS 0x0F
#define IR_OP_DEC         0x10
#define IR_OP_FUNC        0x11
#define IR_OP_PARAM       0x12
#define IR_OP_RETURN      0x13
#define IR_OP_GOTO        0x14
#define IR_OP_IF          0x15
#define IR_OP_CALL        0x16
#define IR_OP_IF_IMME     0x17
#define IR_OP_LABEL       0x18
#define IR_OP_ARG         0x19
#define IR_OP_READ        0x1A
#define IR_OP_WRITE       0x1B
#define IR_OP_IF_POSITIVE 0x1C


#define IR_MODE_I         0x00
#define IR_MODE_T         0x01
#define IR_MODE_V         0x02
#define IR_MODE_NORMAL    0x00
#define IR_MODE_STAR      0x10
#define IR_MODE_ADDR      0x20

#define IR_NON_CONSTANT   0x00
#define IR_CONSTANT       0x01
#define IR_UNDECIDED      0x02

typedef struct ir_t ir;
typedef struct ir_node_t ir_node;
typedef struct ir_list_t ir_list;
typedef struct ir_func_list_t ir_func_list;
typedef struct ir_mode_t ir_mode;

struct ir_mode_t {
    char mode1;
    char op1;
    char mode2;
    char op2;
    char mode3;
    char op3;
};

struct ir_t {
    uint32_t op;
    uint32_t temp_id;
    uint32_t var_id;
    ir_mode mode;
    union {
        int int_val1;
        uint32_t size;
        float float_val1;
    };
    union {
        int int_val2;
        float float_val2;
    };
    uint32_t temp_id1;
    uint32_t var_id1;
    uint32_t temp_id2;
    uint32_t var_id2;
    uint32_t goto_label;
    ir *immediate_ir;
    char *func_name;
};

struct ir_node_t {
    ir *content;
    ir_node *next;
};

struct ir_list_t {
    ir_node *head;
    ir_node *tail;
    uint8_t constant_status;
};

struct ir_func_list_t {
    ir_list *func_content;
    ir_func_list *next;
};

static inline void ir_merge_buffer(ir_list *buffer1, ir_list *buffer2)
{
    if (buffer1->tail != NULL) {
        if (buffer2->head != NULL) {
            buffer1->tail->next = buffer2->head;
            buffer1->tail = buffer2->tail;
        }
    }
    else {
        buffer1->head = buffer2->head;
        buffer1->tail = buffer2->tail;
    }
}

void ir_add_node_to_buffer(ir_list *buffer, ir *ir_content);
uint32_t ir_new_variable();
uint32_t ir_new_temp_val();
uint32_t ir_new_label();
ir *ir_simplify_maccess(ir *old_ir, ir_list *ret_ir);
void ir_print_list(ir_list *buffer);
void ir_compress_label(ir_list *ir_content);
void _ir_print_ir(ir *ir_content);

#endif

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

#include <ast.h>
#include <symbol_table.h>
#include <ir.h>

#ifndef SEMANTICS_H
#define SEMANTICS_H

typedef struct _sem_exp_type_list_t _sem_exp_type_list;
typedef struct _sem_exp_type_t _sem_exp_type;

#define SEM_CONSTANT_YES        0x0
#define SEM_CONSTANT_NO         0x1
#define SEM_CONSTANT_UND        0x2
#define SEM_CONSTANT_IMMEDIATE  0x3

#define SEM_TYPE_MODE_T         0x00
#define SEM_TYPE_MODE_V         0x01
#define SEM_TYPE_MODE_NORMAL    0x00
#define SEM_TYPE_MODE_STAR      0x01
#define SEM_TYPE_MODE_ADDR      0x02

struct _sem_exp_type_t {
    int type;
    int is_array;
    int array_dimension;
    int *array_size;
    int array_accumulated_size;
    char is_lvalue;

    uint32_t ir_temp_val_id;
    uint32_t ir_var_id;
    uint32_t base_address_var_id;
    uint8_t constant_exp_status;
    uint8_t type_mode;
    uint8_t type_mode_op;
    ir *immediate_ir;
    union {
        int int_val;
        float float_val;
    };

    struct_specifier *struct_specifier;
    symbol_entry *var_symbol;
    uint32_t offset_in_struct;
};

struct _sem_exp_type_list_t {
    _sem_exp_type *type;
    _sem_exp_type_list *next;
};

char sem_validate(ast_node *root);
void _sem_report_error(const char *fmt, ...);

#endif
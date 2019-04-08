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

#ifndef SEMANTICS_H
#define SEMANTICS_H

typedef struct _sem_exp_type_list_t _sem_exp_type_list;
typedef struct _sem_exp_type_t _sem_exp_type;

struct _sem_exp_type_t {
    int type;
    int is_array;
    int array_dimension;
    char is_lvalue;
    struct_specifier *struct_specifier;
};

struct _sem_exp_type_list_t {
    _sem_exp_type *type;
    _sem_exp_type_list *next;
};

char sem_validate(ast_node *root);
void _sem_report_error(const char *fmt, ...);

#endif
/*
    C-- Compiler Back End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    backend.h
    Backend definitions
*/

#include <stdint.h>
#include <stdio.h>

#include <global.h>
#include <ast.h>
#include <ir.h>


void cg_mips_generate(ir_func_list *func);
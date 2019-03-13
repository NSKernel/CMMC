/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    debug.c
    Debug components
*/

#include <stdarg.h>
#include <stdio.h>

void print_debug(const char *fmt, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
}
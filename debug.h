/*
    C-- Compiler Front End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    debug.h
    Debug components
*/

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#ifndef DEBUG_H
#define DEBUG_H

#define assert(p) \
    if (!(p)) {\
        printf("ASSERTION FAILED AT %d IN %s!\n", __LINE__, __FILE__);\
        exit(-1);\
    }

void print_debug(const char *fmt, ...);

#endif
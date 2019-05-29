#include <stdio.h>

#ifndef GLOBAL_H
#define GLOBAL_H

struct global_args_t {
    char print_version;          /* -V or --version */
    char verbose;                /* -v or --verbose */
    char *input_file;
    char *output_file;
} global_args;

FILE *output_file;

#endif
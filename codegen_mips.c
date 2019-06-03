/*
    C-- Compiler Back End
    Copyright (C) 2019 NSKernel. All rights reserved.

    A lab of Compilers at Nanjing University

    codegen_mips.c
    Generates MIPS target code
*/

#include <stdint.h>
#include <stdio.h>

#include <global.h>
#include <ast.h>
#include <ir.h>

void _cg_mips_generate_header() {
    // print the following code
    fprintf(output_file, ".data\n");
    fprintf(output_file, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(output_file, "_ret: .asciiz \"\\n\"\n");
    fprintf(output_file, ".globl main\n");
    fprintf(output_file, ".text\n");
    fprintf(output_file, "read:\n");
    fprintf(output_file, "  li $v0, 4\n");
    fprintf(output_file, "  la $a0, _prompt\n");
    fprintf(output_file, "  syscall\n");
    fprintf(output_file, "  li $v0, 5\n");
    fprintf(output_file, "  syscall\n");
    fprintf(output_file, "  jr $ra\n\n");
    fprintf(output_file, "write:\n");
    fprintf(output_file, "  li $v0, 1\n");
    fprintf(output_file, "  syscall\n");
    fprintf(output_file, "  li $v0, 4\n");
    fprintf(output_file, "  la $a0, _ret\n");
    fprintf(output_file, "  syscall\n");
    fprintf(output_file, "  move $v0, $0\n");
    fprintf(output_file, "  jr $ra\n\n");
}

void _cg_mips_set_reg(uint8_t mode, uint8_t op, int num, char *reg_mode, uint8_t reg_num) {
    switch (op)
    {
        case IR_MODE_NORMAL:
            switch (mode) {
                case IR_MODE_T:
                case IR_MODE_V:
                    // frame pointer - num is the offset
                    fprintf(output_file, "  lw $%s%d, %d($fp)\n", reg_mode, reg_num, -num);
                    break;
                case IR_MODE_I:
                    fprintf(output_file, "  li $%s%d, %d\n", reg_mode, reg_num, num);
                    break;
                default:
                    printf("Unexpected\n");
                    break;
            
            }
            break;
        case IR_MODE_ADDR:
            // all addresses are static!
            switch (mode) {
                case IR_MODE_T:
                case IR_MODE_V:
                    // frame pointer - num is the offset
                    fprintf(output_file, "  addi $%s%d, $fp, %d\n", reg_mode, reg_num, -num);
                    break;
                case IR_MODE_I:
                default:
                    printf("Unexpected\n");
                    break;
            }
            break;
        case IR_MODE_STAR:
            // all addresses are static!
            switch (mode) {
                case IR_MODE_T:
                case IR_MODE_V:
                    // frame pointer - num is the offset
                    fprintf(output_file, "  lw $%s%d, %d($fp)\n", reg_mode, reg_num, -num);
                    fprintf(output_file, "  lw $%s%d, 0($%s%d)\n", reg_mode, reg_num, reg_mode, reg_num);
                    break;
                case IR_MODE_I:
                    fprintf(output_file, "  li $%s%d, %d\n", reg_mode, reg_num, num);
                    fprintf(output_file, "  lw $%s%d, 0($%s%d)\n", reg_mode, reg_num, reg_mode, reg_num);
                    break;
                default:
                    printf("Unexpected\n");
                    break;
            }
            break;
        default:
            break;
    }
}

void _cg_mips_generate_exp_3(ir *content) {
    // load oprand 1 to t0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "t", 0);
            break;
    }
    // load oprand 2 to t1
    switch (content->mode.mode3) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->temp_id2, "t", 1);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->var_id2, "t", 1);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->int_val2, "t", 1);
            break;
    }
    // do action
    switch (content->op) {
        case IR_EXP_OP_ADD:
            fprintf(output_file, "  add $v0, $t0, $t1\n");
            break;
        case IR_EXP_OP_MINUS:
            fprintf(output_file, "  sub $v0, $t0, $t1\n");
            break;
        case IR_EXP_OP_MUL:
            fprintf(output_file, "  mul $v0, $t0, $t1\n");
            break;
        case IR_EXP_OP_DIV:
            fprintf(output_file, "  div $t0, $t1\n");
            fprintf(output_file, "  mflo $v0\n");
            break;
    }
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
}

void _cg_mips_generate_relop(ir *content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    // load oprand 1 to t0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "t", 0);
            break;
    }
    // load oprand 2 to t1
    switch (content->mode.mode3) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->temp_id2, "t", 1);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->var_id2, "t", 1);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->int_val2, "t", 1);
            break;
    }
    switch (content->op) {
        case IR_EXP_OP_EQ:
            fprintf(output_file, "  beq $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
        case IR_EXP_OP_NEQ:
            fprintf(output_file, "  bne $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
        case IR_EXP_OP_GE:
            fprintf(output_file, "  bge $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
        case IR_EXP_OP_GT:
            fprintf(output_file, "  bgt $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
        case IR_EXP_OP_LT:
            fprintf(output_file, "  blt $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
        case IR_EXP_OP_LE:
            fprintf(output_file, "  ble $t0, $t1, label%d\n", goto_label = ir_new_label());
            break;
    }
    fprintf(output_file, "  li $v0, 0\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "  j label%d\n", goto_label_end = ir_new_label());
    fprintf(output_file, "label%d:\n", goto_label);
    fprintf(output_file, "  li $v0, 1\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "label%d:\n", goto_label_end);
}

void _cg_mips_generate_and(ir *content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    // load oprand 1 to t0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "t", 0);
            break;
    }
    // load oprand 2 to t1
    switch (content->mode.mode3) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->temp_id2, "t", 1);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->var_id2, "t", 1);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->int_val2, "t", 1);
            break;
    }
    fprintf(output_file, "  beq $t0, $zero, label%d\n", goto_label = ir_new_label());
    fprintf(output_file, "  beq $t1, $zero, label%d\n", goto_label);
    fprintf(output_file, "  li $v0, 1\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "  j label%d\n", goto_label_end = ir_new_label());
    fprintf(output_file, "label%d:\n", goto_label);
    fprintf(output_file, "  li $v0, 0\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "label%d:\n", goto_label_end);
}

void _cg_mips_generate_or(ir *content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    // load oprand 1 to t0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "t", 0);
            break;
    }
    // load oprand 2 to t1
    switch (content->mode.mode3) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->temp_id2, "t", 1);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->var_id2, "t", 1);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode3, content->mode.op3, content->int_val2, "t", 1);
            break;
    }
    fprintf(output_file, "  bne $t0, $zero, label%d\n", goto_label = ir_new_label());
    fprintf(output_file, "  bne $t1, $zero, label%d\n", goto_label);
    fprintf(output_file, "  li $v0, 0\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "  j label%d\n", goto_label_end = ir_new_label());
    fprintf(output_file, "label%d:\n", goto_label);
    fprintf(output_file, "  li $v0, 1\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "label%d:\n", goto_label_end);
}

void _cg_mips_generate_assign(ir *content) {
    // load oprand 1 to v0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "v", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "v", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "v", 0);
            break;
    }
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
}

void _cg_mips_generate_not(ir *content) {
    uint32_t goto_label;
    uint32_t goto_label_end;
    // load oprand 1 to t0
    switch (content->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode2, content->mode.op2, content->int_val1, "t", 0);
            break;
    }
    fprintf(output_file, "  beq $t0, $zero, label%d\n", goto_label = ir_new_label());
    fprintf(output_file, "  li $v0, 0\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "  j label%d\n", goto_label_end = ir_new_label());
    fprintf(output_file, "label%d:\n", goto_label);
    fprintf(output_file, "  li $v0, 1\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
    fprintf(output_file, "label%d:\n", goto_label_end);
}

void _cg_mips_generate_if_imme(ir *content) {
    // load oprand 1 to t0
    switch (content->immediate_ir->mode.mode2) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->immediate_ir->mode.mode2, content->immediate_ir->mode.op2, content->immediate_ir->temp_id1, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->immediate_ir->mode.mode2, content->immediate_ir->mode.op2, content->immediate_ir->var_id1, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->immediate_ir->mode.mode2, content->immediate_ir->mode.op2, content->immediate_ir->int_val1, "t", 0);
            break;
    }
    // load oprand 2 to t1
    switch (content->immediate_ir->mode.mode3) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->immediate_ir->mode.mode3, content->immediate_ir->mode.op3, content->immediate_ir->temp_id2, "t", 1);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->immediate_ir->mode.mode3, content->immediate_ir->mode.op3, content->immediate_ir->var_id2, "t", 1);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->immediate_ir->mode.mode3, content->immediate_ir->mode.op3, content->immediate_ir->int_val2, "t", 1);
            break;
    }
    switch (content->immediate_ir->op) {
        case IR_EXP_OP_EQ:
            fprintf(output_file, "  beq $t0, $t1, label%d\n", content->goto_label);
            break;
        case IR_EXP_OP_NEQ:
            fprintf(output_file, "  bne $t0, $t1, label%d\n", content->goto_label);
            break;
        case IR_EXP_OP_GE:
            fprintf(output_file, "  bge $t0, $t1, label%d\n", content->goto_label);
            break;
        case IR_EXP_OP_GT:
            fprintf(output_file, "  bgt $t0, $t1, label%d\n", content->goto_label);
            break;
        case IR_EXP_OP_LT:
            fprintf(output_file, "  blt $t0, $t1, label%d\n", content->goto_label);
            break;
        case IR_EXP_OP_LE:
            fprintf(output_file, "  ble $t0, $t1, label%d\n", content->goto_label);
            break;
    }
}

void _cg_mips_generate_arg(ir *content) {
    // load oprand to t0
    switch (content->mode.mode1) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->temp_id, "t", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->var_id, "t", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->int_val1, "t", 0);
            break;
    }
    // store result to $sp - 4
    fprintf(output_file, "  addi $sp, $sp, -4\n");
    fprintf(output_file, "  sw $t0, 0($sp)\n");
}

void _cg_mips_generate_return(ir *content) {
    // load oprand to v0
    switch (content->mode.mode1) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->temp_id, "v", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->var_id, "v", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->int_val1, "v", 0);
            break;
    }
    fprintf(output_file, "  jr $ra\n");
}

void _cg_mips_generate_call(ir *content) {
    // | sp | fp | ra | ARG1
    //                ^
    //                sp
    fprintf(output_file, "  addi $sp, $sp, -12\n");
    fprintf(output_file, "  sw $ra, 8($sp)\n");
    fprintf(output_file, "  sw $fp, 4($sp)\n");
    fprintf(output_file, "  sw $sp, 0($sp)\n");
    fprintf(output_file, "  move $fp, $sp\n");
    // | sp | fp | ra | ARG1
    // ^
    // fp & sp
    fprintf(output_file, "  jal %s\n", content->func_name);
    // | .....  last frame ..... | sp | fp | ra | ARG1
    // ^                         ^
    // sp                        fp
    // restore stack
    fprintf(output_file, "  lw $sp, 0($fp)\n");
    fprintf(output_file, "  lw $fp, 4($sp)\n");
    fprintf(output_file, "  lw $ra, 8($sp)\n");
    // pop args
    fprintf(output_file, "  add $sp, $sp, %d\n", 4 * content->param_count);
    // store the result
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
}

void _cg_mips_generate_read(ir *content) {
    // call read
    fprintf(output_file, "  addi $sp, $sp, -4\n");
    fprintf(output_file, "  sw $ra, 0($sp)\n");
    fprintf(output_file, "  jal read\n");
    fprintf(output_file, "  lw $ra, 0($sp)\n");
    fprintf(output_file, "  addi $sp, $sp, 4\n");
    // store result
    switch (content->mode.op1) {
        case IR_MODE_NORMAL:
            fprintf(output_file, "  sw $v0, ");
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "%d($fp)\n", -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "%d($fp)\n", -(content->var_id));
            else
                printf("Unexpected\n");
            break;
        case IR_MODE_STAR:
            if (content->mode.mode1 == IR_MODE_T)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->temp_id));
            else if (content->mode.mode1 == IR_MODE_V)
                fprintf(output_file, "  lw $%s%d, %d($fp)\n", "t", 2, -(content->var_id));
            else
                printf("Unexpected\n");
            
            fprintf(output_file, "  sw $v0, 0($t2)\n");
            break;
        case IR_MODE_ADDR:
        default:
            printf("Unexpected\n");
            break;
    }
}

void _cg_mips_generate_write(ir *content) {
    // load oprand 1 to a0
    switch (content->mode.mode1) {
        case IR_MODE_T:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->temp_id, "a", 0);
            break;
        case IR_MODE_V:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->var_id, "a", 0);
            break;
        case IR_MODE_I:
            _cg_mips_set_reg(content->mode.mode1, content->mode.op1, content->int_val1, "a", 0);
            break;
    }
    // call write
    fprintf(output_file, "  addi $sp, $sp, -4\n");
    fprintf(output_file, "  sw $ra, 0($sp)\n");
    fprintf(output_file, "  jal write\n");
    fprintf(output_file, "  lw $ra, 0($sp)\n");
    fprintf(output_file, "  addi $sp, $sp, 4\n");
}

void _cg_mips_generate_function(ir_list *list) {

    ir_node *iterator;
    if (list == NULL) {
        printf("Empty\n");
        return;
    }
    iterator = list->head;
    if (iterator == NULL) {
        printf("Empty list\n");
        return;
    }
    // don't care the initial position of $sp
    // but need to set $fp
    if (!strcmp(iterator->content->func_name, "main")) {
        fprintf(output_file, "main:\n");
        fprintf(output_file, "  move $fp, $sp\n");
        iterator = iterator->next;
    }
    while (iterator != NULL) {
        // generate code
        switch (iterator->content->op) {
            case IR_EXP_OP_ADD:
            case IR_EXP_OP_DIV:
            case IR_EXP_OP_MINUS:
            case IR_EXP_OP_MUL:
                _cg_mips_generate_exp_3(iterator->content);
                break;
            case IR_EXP_OP_EQ:
            case IR_EXP_OP_GE:
            case IR_EXP_OP_GT:
            case IR_EXP_OP_LE:
            case IR_EXP_OP_LT:
            case IR_EXP_OP_NEQ:
                _cg_mips_generate_relop(iterator->content);
                break;
            case IR_EXP_OP_AND:
                _cg_mips_generate_and(iterator->content);
                break;
            case IR_EXP_OP_OR:
                _cg_mips_generate_or(iterator->content);
                break;
            case IR_EXP_OP_ASSIGN:
                _cg_mips_generate_assign(iterator->content);
                break;    
            case IR_EXP_OP_NOT:
                _cg_mips_generate_not(iterator->content);
                break;
            case IR_OP_ARG:
                _cg_mips_generate_arg(iterator->content);
                break;
            case IR_OP_CALL:
                _cg_mips_generate_call(iterator->content);
                break;
            case IR_OP_DEC:
                fprintf(output_file, "  addi $sp, $sp, %d\n", -(iterator->content->size));
                break;
            case IR_OP_FUNC:
                fprintf(output_file, "%s :\n", iterator->content->func_name);
                break;
            case IR_OP_GOTO:
                fprintf(output_file, "  j label%d\n", iterator->content->goto_label);
                break;
            case IR_OP_IF:
                switch (iterator->content->mode.mode1) {
                    case IR_MODE_T:
                        _cg_mips_set_reg(iterator->content->mode.mode1, iterator->content->mode.op1, iterator->content->temp_id, "t", 0);
                        break;
                    case IR_MODE_V:
                        _cg_mips_set_reg(iterator->content->mode.mode1, iterator->content->mode.op1, iterator->content->var_id, "t", 0);
                        break;
                }
                fprintf(output_file, "  beq $t0, $zero, label%d\n", iterator->content->goto_label);
                break;
            case IR_OP_IF_POSITIVE:
                switch (iterator->content->mode.mode1) {
                    case IR_MODE_T:
                        _cg_mips_set_reg(iterator->content->mode.mode1, iterator->content->mode.op1, iterator->content->temp_id, "t", 0);
                        break;
                    case IR_MODE_V:
                        _cg_mips_set_reg(iterator->content->mode.mode1, iterator->content->mode.op1, iterator->content->var_id, "t", 0);
                        break;
                }
                fprintf(output_file, "  bne $t0, $zero, label%d\n", iterator->content->goto_label);
                break;
            case IR_OP_IF_IMME:
                _cg_mips_generate_if_imme(iterator->content);
                break;
            case IR_OP_LABEL:
                fprintf(output_file, "label%d:\n", iterator->content->goto_label);
                break;
            case IR_OP_PARAM:
                //fprintf(output_file, "PARAM v%d\n", ir_content->var_id);
                break;
            case IR_OP_RETURN:
                _cg_mips_generate_return(iterator->content);
                break;
            case IR_OP_READ:
                _cg_mips_generate_read(iterator->content);
                break;
            case IR_OP_WRITE:
                _cg_mips_generate_write(iterator->content);
                break;
            default:
                printf("SHIT HAPPENED, %d\n", iterator->content->op);
            break;
        }
        iterator = iterator->next;
    }
}

void cg_mips_generate(ir_func_list *func) {
    ir_func_list *iterator = func;

    // generate header
    _cg_mips_generate_header();

    while (iterator != NULL) {
        // deal with func one by one
        _cg_mips_generate_function(iterator->func_content);

        iterator = iterator->next;
    }
    return;
}

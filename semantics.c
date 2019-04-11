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

void _sem_validate_ext_def_list(ast_node *node);
void _sem_validate_ext_def(ast_node *node);
int _sem_validate_specifier(ast_node *node, struct_specifier **struct_specifier, int context, int do_not_free);
void _sem_validate_ext_dec_list(ast_node *node, int type, struct_specifier *struct_specifier);
symbol_entry *_sem_validate_var_dec(ast_node *node);
struct_specifier *_sem_validate_struct_specifier(ast_node *node, int context, int do_not_free);
symbol_entry *_sem_validate_fun_dec(ast_node *node, int type, struct_specifier *struct_specifier);
int _sem_validate_var_list(ast_node *node, symbol_list **param_list);
symbol_entry *_sem_validate_param_dec(ast_node *node);
void _sem_validate_def_list(ast_node *node, int context);
void _sem_validate_def(ast_node *node, int context);
void _sem_validate_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, int context);
symbol_entry *_sem_validate_dec(ast_node *node, int type, struct_specifier *parsed_struct_specifier);
char _sem_type_matching_with_symbol(symbol_entry *se, _sem_exp_type *et);
void _sem_validate_comp_st(ast_node *node, int context, _sem_exp_type *return_type);
void _sem_validate_stmt_list(ast_node *node, int context, _sem_exp_type *return_type);
void _sem_validate_stmt(ast_node *node, int context, _sem_exp_type *return_type);


// helpers from semantics_struct_helper
extern symbol_list *_sem_validate_def_list_for_struct(ast_node *node, int context);
extern symbol_list *_sem_validate_def_for_struct(ast_node *node, int context);
extern symbol_list *_sem_validate_dec_list_for_struct(ast_node *node, int type, struct_specifier *struct_specifier, int context);
extern symbol_entry *_sem_validate_dec_for_struct(ast_node *node, int type, struct_specifier *struct_specifier);
extern _sem_exp_type *_sem_validate_exp(ast_node *node);
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
    symtable_init();
    // Current node: Program
    if (root->children[0] != NULL) {
        _sem_validate_ext_def_list(root->children[0]);
    }
    // else {
    //     empty program!
    //}
    symtable_ensure_defined();
    return _sem_error;
}

void _sem_validate_ext_def_list(ast_node *node) {
    if (node == NULL) {
        // empty def list
        return;
    }
    assert(node->children_count == 2);
    assert(node->children[0] != NULL);
    assert(!strcmp(node->children[0]->name, "ExtDef"));

    _sem_validate_ext_def(node->children[0]);

    _sem_validate_ext_def_list(node->children[1]);
} 

void _sem_validate_ext_def(ast_node *node) {
    assert(node->children_count >= 2);
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);
    assert(!strcmp(node->children[0]->name, "Specifier"));

    int type;
    struct_specifier *struct_specifier = NULL;
    symbol_entry *func_entry;
    _sem_exp_type return_type;
    type = _sem_validate_specifier(node->children[0], &struct_specifier, 0, 0);
    if (type == SYMBOL_T_ERROR) {
        // specifier validation failed with error
        return;
    }
    
    if (!strcmp(node->children[1]->name, "ExtDecList")) {
        assert(node->children_count == 3);
        assert(node->children[2] != NULL);

        _sem_validate_ext_dec_list(node->children[1], type, struct_specifier);
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
        func_entry = _sem_validate_fun_dec(node->children[1], type, struct_specifier);
        if (!strcmp(node->children[2]->name, "SEMI")) {
            // declaration
            func_entry->is_function_dec = 1;
            // we are not using these!
            if (func_entry->param_count)
                symtable_pop_context_without_free(1);
            symtable_insert(func_entry, 0, 0, 0);
        }
        else {
            // CompSt
            func_entry->is_function_dec = 0;
            return_type.type = type;
            return_type.is_array = 0;
            return_type.is_lvalue = 0;
            return_type.array_dimension = 0;
            return_type.struct_specifier = struct_specifier;
            symtable_insert(func_entry, 0, 0, 0); // Insert here to make recursion possible
            // in comp_st, symtable will pop symbols
            // at level 1 but those without free will remain
            _sem_validate_comp_st(node->children[2], 0, &return_type);
        }
    }
    else {
        // should not be here
        assert(0);
    }
}

void _sem_validate_comp_st(ast_node *node, int context, _sem_exp_type *return_type) {
    assert(node->children_count == 4);
    _sem_validate_def_list(node->children[1], context + 1);
    _sem_validate_stmt_list(node->children[2], context + 1, return_type);
    symtable_pop_context(context + 1);
}

void _sem_validate_stmt_list(ast_node *node, int context, _sem_exp_type *return_type) {
    if (node == NULL) {
        return;
    }

    _sem_validate_stmt(node->children[0], context, return_type);
    _sem_validate_stmt_list(node->children[1], context, return_type);
}

void _sem_validate_stmt(ast_node *node, int context, _sem_exp_type *return_type) {
    _sem_exp_type *exp_type;
    if (!strcmp(node->children[0]->name, "Exp")) {
        if (_sem_validate_exp(node->children[0]) == NULL) {
            return;
        }
    }
    if (!strcmp(node->children[0]->name, "CompSt")) {
        _sem_validate_comp_st(node->children[0], context, return_type);
    }
    if (!strcmp(node->children[0]->name, "RETURN")) {
        exp_type = _sem_validate_exp(node->children[1]);
        if (exp_type == NULL) {
            _sem_report_error("Error type 8 at Line %d: Returned an expression with error", node->children[1]->line_number);
            return;
        }
        if (!_sem_type_matching(exp_type, return_type)) {
            _sem_report_error("Error type 8 at Line %d: Return type mismatched", node->children[1]->line_number);
        }
        free(exp_type);
        return;
    }
    if (!strcmp(node->children[0]->name, "IF")) {
        // IF LP Exp RP Stmt // ELSE Stmt
        exp_type = _sem_validate_exp(node->children[2]);
        if (exp_type == NULL) {
            // _sem_report_error("Error type 8 at Line %d: Expression with error", node->children[1]->line_number);
        }
        else {
            if (exp_type->type != SYMBOL_T_INT || exp_type->is_array) {
                _sem_report_error("Error type 8 at Line %d: INT required in IF statement", node->children[1]->line_number);
            }
            free(exp_type);
        }

        _sem_validate_stmt(node->children[4], context, return_type);
        if (node->children_count == 7) {
            // ELSE Stmt
            _sem_validate_stmt(node->children[6], context, return_type);
        }
        return;
    }
    if (!strcmp(node->children[0]->name, "WHILE")) {
        // WHILE LP Exp RP Stmt
        exp_type = _sem_validate_exp(node->children[2]);
        if (exp_type == NULL) {
            // _sem_report_error("Error type 8 at Line %d:  Expression with error", node->children[1]->line_number);
        }
        else {
            if (exp_type->type != SYMBOL_T_INT || exp_type->is_array) {
                _sem_report_error("Error type 8 at Line %d: INT required in WHILE statement", node->children[1]->line_number);
            }
            free(exp_type);
        }

        _sem_validate_stmt(node->children[4], context, return_type);
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

void _sem_validate_ext_dec_list(ast_node *node, int type, struct_specifier *struct_specifier) {
    assert(!strcmp(node->name, "ExtDecList"));
    symbol_entry *ins_entry;

    ins_entry = _sem_validate_var_dec(node->children[0]);
    ins_entry->type = type;
    ins_entry->struct_specifier = struct_specifier;
    if (symtable_insert(ins_entry, 0, 0, 0)) {
        free(ins_entry);
        // failed to insert
        // but should not stop
    }
    if (node->children_count == 3) { // VarDec COMMA ExtDecList
        _sem_validate_ext_dec_list(node->children[2], type, struct_specifier);
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
        if (symtable_insert(ins_specifier_symbol, context, 0, do_not_free)) { // it is not a field so we don't care
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

symbol_entry *_sem_validate_fun_dec(ast_node *node, int type, struct_specifier *struct_specifier) {
    symbol_entry *ret_entry = malloc(sizeof(symbol_entry));
    symbol_list *param_list = NULL;
    ret_entry->type = type;
    ret_entry->struct_specifier = struct_specifier;
    ret_entry->is_function = 1;
    ret_entry->is_array = 0;
    ret_entry->line_no_def = node->line_number;

    // ID LP RP
    // ID LP VarList RP

    // ID
    ret_entry->id = node->children[0]->string_value;
    
    if (node->children_count == 3) { // ID LP RP
        ret_entry->param_count = 0;
        ret_entry->params = NULL;
    } 
    else if (node->children_count == 4) {
        ret_entry->param_count = _sem_validate_var_list(node->children[2], &param_list);
        ret_entry->params = param_list;
    }
    else {
        assert(0);
        return NULL;
    }

    return ret_entry;
}

int _sem_validate_var_list(ast_node *node, symbol_list **param_list) {
    symbol_list *ret_list;
    int ret_val = 0;
    // ParamDec
    symbol_entry *current_entry = _sem_validate_param_dec(node->children[0]);
    symbol_list *ret_list_tail = NULL;
    if (node->children_count == 3) {
        ret_val = _sem_validate_var_list(node->children[2], &ret_list_tail);
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

    // try to insert!
    if (symtable_insert(ret_entry, 1, 0, 1)) {
        symtable_free_symbol(ret_entry);
        return NULL;
    }

    return ret_entry;
}

void _sem_validate_def_list(ast_node *node, int context) {
    if (node == NULL) {
        return;
    }
    _sem_validate_def(node->children[0], context);
    _sem_validate_def_list(node->children[1], context);
}

void _sem_validate_def(ast_node *node, int context) {
    struct_specifier *struct_specifier = NULL;
    int type = _sem_validate_specifier(node->children[0], &struct_specifier, context, 0);
    if (type == SYMBOL_T_ERROR) {
        // specifier validation failed with error
        return;
    }

    _sem_validate_dec_list(node->children[1], type, struct_specifier, context);
}

void _sem_validate_dec_list(ast_node *node, int type, struct_specifier *struct_specifier, int context) {
    symbol_entry *ins_entry;

    // Dec
    ins_entry = _sem_validate_dec(node->children[0], type, struct_specifier);
    if (ins_entry != NULL) {
        if (symtable_insert(ins_entry, context, 0, 0)) {
            // Falied to insert
            symtable_free_symbol(ins_entry);
        }
    }
    
    if (node->children_count == 3) { // Dec COMMA DecList
        _sem_validate_dec_list(node->children[2], type, struct_specifier, context);
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

symbol_entry *_sem_validate_dec(ast_node *node, int type, struct_specifier *parsed_struct_specifier) {
    struct_specifier *exp_struct_specifier;
    char dummy;
    _sem_exp_type *exp_type;
    
    symbol_entry *ret_entry = _sem_validate_var_dec(node->children[0]);
    ret_entry->type = type;
    ret_entry->struct_specifier = parsed_struct_specifier;

    if (node->children_count == 3) { // VarDec ASSIGNOP Exp
        exp_type = _sem_validate_exp(node->children[2]);
        if (exp_type == NULL)
            return NULL;
        if (!_sem_type_matching_with_symbol(ret_entry, exp_type)) {
            // Exp does not match the type of Def
            free(exp_type);
            free(ret_entry);
            _sem_report_error("Error type 5 at Line %d: Type mismatched for assignment", node->children[1]->line_number);
            return NULL;
        }
    }
    return ret_entry;
}


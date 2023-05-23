#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <sys/types.h>


/* This header file defines the structures and functions for the Command module */

/* Enumeration of all the Virtual Machine operations  */
typedef enum {
    OP_UNKNOWN = -1,

    /* Arithmetic  Operations */
    OP_ADD,
    OP_SUB,
    OP_NEG,
    OP_EQ,
    OP_LT,
    OP_AND,
    OP_OR,
    OP_NOT,

    /* Memory Operations */
    OP_PUSH,
    OP_POP,

    /* More to be added */
} operation_t;


/* Enumeration of the different types of operands */

typedef enum {
    OPERAND_UNKNOWN = -1,

    OPERAND_NONE,  // For uninaray operators
    OPERAND_MEMORY,
    OPERAND_NUMBER,
    /* potentialy more to come, this is all for now */
} operand_t;

/* Enumeration of all the Virutal Machine memory operands, used with push and pop */
typedef enum {
    MEM_OP_UNKNOWN = -1,

    MEM_OP_ARGUMENT,
    MEM_OP_LOCAL,
    MEM_OP_STATIC,
    MEM_OP_CONSTANT,
    MEM_OP_THIS,
    MEM_OP_THAT,
    MEM_OP_POINTER_0,
    MEM_OP_POINTER_1,
    MEM_OP_TEMP
} memory_operand_t;


/* Command structure, holds a single command and its operands */

typedef struct {
    operation_t operation;
    operand_t operand_type;

    union {
        memory_operand_t memory_operand;
        int32_t number;
    } operand;

} command_t;


/* Command module structure, meant to hold all the commands in a file, parsed of couse ;) */

typedef struct {
    command_t* commands;
    size_t total_commands;

    size_t* function_indexes; // To be used later, not for now
} command_module_t;

/* no functions currently, possibly some to come */

#endif

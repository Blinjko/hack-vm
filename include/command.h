#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <sys/types.h>


/* This header file defines the structures and functions for the Command module */


/* Enumeration of all the VM operator keywords 
 * NOTE: Enum positions / values are relevant to a string mapping in parser.c*/
typedef enum {
    OP_UNKNOWN = -1,

    /* Mathematical */
    OP_ADD,
    OP_SUB,
    OP_NEG, // Negation
    
    /* Bit wise */
    OP_AND,
    OP_OR,
    OP_NOT,

    /* Logical */
    OP_LT, // Less than
    OP_GT, // Greater than
    OP_EQ, // Equal to
    
    /* Memory */
    OP_PUSH,
    OP_POP,

    /* Lables, Functions & Program Flow*/
    OP_LABEL,
    OP_GOTO,
    OP_IFGOTO,
    OP_FUNCTION,
    OP_CALL,
    OP_RETURN,

    OP_MAX,
} operator_t;


/* Enumeration of all the memory segment keywords
 * NOTE: Enum positions / values are relevant to a string mapping in parser.c*/
typedef enum {
    SEG_UNKNOWN = -1,

    SEG_ARGUMENT,
    SEG_LOCAL,
    SEG_STATIC,
    SEG_CONSTANT,
    SEG_THIS,
    SEG_THAT,
    SEG_POINTER,
    SEG_TEMP,
    
    SEG_MAX,
} memory_segment_t;


typedef struct {
    operator_t op; // The operator keyword, pop, push, add, sub, ..., etc

    // Defines the potential arugments ( if any )
    union {
        
        // Defines arguments in terms of memory access, pop or push
        struct {
            memory_segment_t segment;
            uint16_t index;
        } memory;

        // Defines arguments in terms of program flow, label, if-goto, goto
        // function definitions, function calls 
        struct {
            char* label;
            uint16_t locals;
        } flow;
    } arguments;

} command_t;


/* Command module structure, meant to hold all the commands in a file, parsed of couse ;) */

typedef struct {
    command_t* commands;
    size_t total_commands;

} command_module_t;

/* no functions currently, possibly some to come */

#endif

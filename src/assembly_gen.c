#include "../include/assembly_gen.h"
#include "../include/command.h"
#include "../include/stack_arena.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


/* Structures and functions for generating assembly mneumonics  */

typedef enum {
    OPCODE_UNKNOWN = -1,
    OPCODE_A_NUMBER,
    OPCODE_A_SYMBOL,
    OPCODE_COMPUTE,
    OPCODE_JUMP,
    OPCODE_SYMBOL
} opcode_t;

typedef enum {
    COMP_UNKNOWN = -1,
    COMP_0,
    COMP_1,          
    COMP_NEG_1,
    COMP_D,
    COMP_NOT_D,
    COMP_NEG_D,
    COMP_D_PLUS_1,
    COMP_D_MINUS_1,
    COMP_A,
    COMP_NOT_A,
    COMP_NEG_A,
    COMP_A_PLUS_1,
    COMP_A_MINUS_1,
    COMP_D_PLUS_A,
    COMP_D_MINUS_A,
    COMP_A_MINUS_D,
    COMP_D_AND_A,
    COMP_D_OR_A,
    COMP_M,
    COMP_NOT_M,
    COMP_NEG_M,
    COMP_M_PLUS_1,
    COMP_M_MINUS_1,
    COMP_D_PLUS_M,
    COMP_D_MINUS_M,
    COMP_M_MINUS_D,
    COMP_D_AND_M,
    COMP_D_OR_M,   

    COMP_MAX,
} comp_t;

typedef enum {
    DEST_UNKNOWN = -1,
    DEST_M,
    DEST_D,
    DEST_MD,
    DEST_A,
    DEST_AM,
    DEST_AD,
    DEST_AMD,

    DEST_MAX,
} dest_t;

typedef enum {
    JUMP_UNKNOWN = -1,
    JUMP_JGT,
    JUMP_JEQ,
    JUMP_JGE,
    JUMP_JLT,
    JUMP_JNE,
    JUMP_JLE,
    JUMP_JMP,

    JUMP_MAX,
} jump_t;

/* Mappings for the enums values to strngs, arrays are relevant to enum values */
static char const* const COMP_STR_MAPPING[] = {"0", "1", "-1", "D", "!D", "-D", "D+1", "D-1", "A", "!A", "-A",
 "A+1", "A-1", "D+A", "D-A", "A-D", "D&A", "D|A", "M", "!M", "-M", "M+1", "M-1", "D+M", "D-M", "M-D", "D&M", "D|M" };

static char const* const DEST_STR_MAPPING[] = {"M", "D", "MD", "A", "AM", "AD", "AMD"};

static char const* const JUMP_STR_MAPPING[] = {"JGT", "JEQ", "JGE", "JLT", "JNE", "JLE", "JMP"};

typedef struct {
    /* Determines what kind of mneumonic is held */
    opcode_t opcode;

    union {
        /* Used for symbols, and labels */
        char* label;

        /*  Used for holding information relating to a computation or jump instruction ( theres an overlap ) */
        struct {
            comp_t comp;        
            dest_t dest;
            jump_t jump;
        } compute;

        /* This number is only used for A-instructions with a number, i.e @1000 or @1214 */
        uint16_t number;
    } variants;

} mneumonic_t;

/* Function to create / a mnuemonic structure from given data */
void createMneumonic(mneumonic_t* mneumonic, opcode_t opcode, char* label, comp_t comp, dest_t dest, jump_t jump, uint16_t number)
{
    mneumonic->opcode = opcode;
    switch (opcode) {
        case OPCODE_A_NUMBER:
            mneumonic->variants.number = number;
            break;

        case OPCODE_A_SYMBOL:
        case OPCODE_SYMBOL:
            mneumonic->variants.label = label;
            break;

        case OPCODE_COMPUTE:
        case OPCODE_JUMP:
            mneumonic->variants.compute.comp = comp;
            mneumonic->variants.compute.dest = dest;
            mneumonic->variants.compute.jump = jump;
            break;

        /* fill nothing if the opcode is unknown */
        default:
            return;
    }
}

/* Generate an assembly string from an array of mneumonic structures,
 * the string returned will be null terminated and allocated on stack_arena
 * Return NULL on failure
 * Return valid char* on success */
char* generateMneumonics(mneumonic_t* mneumonics, size_t total_mneumonics, stack_arena_t* stack_arena)
{
    char* final_string = NULL;

    for (size_t index = 0; index < total_mneumonics; index++) {

        ssize_t bytes_written = 0;

        /* 9 Is how much characters / bytes the biggest assembly instruction is,
         * that string is AMD=M+1\n\0 or some variant AMD=D-1\n\0, etc.
         * A label could be arbitrarily long, but then we can extend the memory
         * due to the stack data structure, where need be, doing this saves code */
        char* assembly_string = stackArenaPush(stack_arena, 9) ;
        if (assembly_string == NULL) {
            return NULL;
        }

        if (mneumonics[index].opcode == OPCODE_A_NUMBER) {
            bytes_written = sprintf(assembly_string, "@%u\n", mneumonics[index].variants.number);
        }

        else if (mneumonics[index].opcode == OPCODE_A_SYMBOL) {

            /* Extend the buffer to accomodate for the length of the label
             * if the length of the label is greater than there is room for.
             * 6 is used because the buffer is by default 9, -1 for the '@',
             * and -1 for the \n, -1 for '\0'*/
            if (strlen(mneumonics[index].variants.label) > 6 && 
                stackArenaPush(stack_arena, strlen(mneumonics[index].variants.label) - 6) == NULL) {
                return NULL;
            }

            bytes_written = sprintf(assembly_string, "@%s\n", mneumonics[index].variants.label);
        }

        else if (mneumonics[index].opcode == OPCODE_COMPUTE) {
            bytes_written = sprintf(assembly_string, "%s=%s\n", DEST_STR_MAPPING[mneumonics[index].variants.compute.dest], 
                                                                COMP_STR_MAPPING[mneumonics[index].variants.compute.comp]);
        }

        else if (mneumonics[index].opcode == OPCODE_JUMP) {
            bytes_written = sprintf(assembly_string, "%s;%s\n", COMP_STR_MAPPING[mneumonics[index].variants.compute.comp],
                                                                JUMP_STR_MAPPING[mneumonics[index].variants.compute.jump]);
        }

        else if (mneumonics[index].opcode == OPCODE_SYMBOL) {

            /* Extend the buffer to accomodate for the length of the label
             * if the length of the label is greater than there is room for.
             * 5 is used because the buffer is by default 9, -2 for the '()',
             * and -1 for the \n, -1 for '\0' */
            if (strlen(mneumonics[index].variants.label) > 6 && 
                stackArenaPush(stack_arena, strlen(mneumonics[index].variants.label) - 5) == NULL) {
                return NULL;
            }

            bytes_written = sprintf(assembly_string, "(%s)\n", mneumonics[index].variants.label);
        }

        /* Unknown opcode type */
        else {
            return NULL;
        }


        /* strip the null terminator off */
        stackArenaPop(stack_arena, 1);

        /* Readjust buffer as needed */
        if (bytes_written < 8) {
            stackArenaPop(stack_arena, 8 - bytes_written);
        }

        if (index == 0) {
            final_string = assembly_string;
        }
    }

    /* Add the null terminator */
    char* temp = stackArenaPush(stack_arena, 1);
    if (temp == NULL) {
        return NULL;
    }
    *temp = '\0';

    return final_string;
}


/* Initialize the Assembly Gen module by opening an output file
 * Return -1 - failed to open file
 * Return 0  - Success */
int32_t assemblyGenInitialize(assembly_gen_t* assembly_gen, const char* filepath)
{
    assert(filepath != NULL && assembly_gen != NULL);

    memset(assembly_gen, 0, sizeof(assembly_gen_t));

    assembly_gen->output_file = fopen(filepath, "w");
    if (assembly_gen->output_file == NULL) {
        return -1;
    }

    return 0;
}

/* Destroys an assebmly gen instance */
void assemblyGenDestroy(assembly_gen_t* assembly_gen)
{
    assert(assembly_gen != NULL && assembly_gen->output_file != NULL);

    fclose(assembly_gen->output_file);

    memset(assembly_gen, 0, sizeof(assembly_gen_t));
}

/* Translate a logical VM command into assembly
 * return valid char* on success,
 * return NULL on failure */
char* translateLogicalCommand(stack_arena_t* stack_arena, command_t* command, const char* filename)
{
    /* All logical commands are uninary operators
     * They take 2 items off the stack, perform an operation on them
     * and push them back on the stack.
     * The only part that varies is the operation command.
     * the rest is the same */

    /* Assembly code structure for basic mathematics operations that use two numbers
     * @SP Load the address of the register that holds the stack pointer address
     * A=M load the stack pointer address
     * D=M set D to the value at the top of the stack
     * @SP Load the address of the register that holds the stack pointer address
     * AM=M-1 Decrement the stack pointer and set A to the (new) address of the stack pointer
     * M=#OPERATION Perform the operation on the data
     */

    /* This variable is used to count the instructions overall, it is needed to create labels
     * to jump back to for conditional operations */
    static uint16_t instruction_counter = 0;
    if (instruction_counter == UINT16_MAX) {
        return NULL;
    }
    
    mneumonic_t* instructions = NULL;
    size_t total_instructions = 0;

    /* These operations only use one number */
    if (command->op == OP_NOT || command->op == OP_NEG) {
        instructions = stackArenaPush(stack_arena, 3 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }
        total_instructions = 3;

        createMneumonic(&instructions[0], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);  // @SP
        createMneumonic(&instructions[1], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);               // A=M

        switch (command->op) {
            case OP_NOT:
                createMneumonic(&instructions[2], OPCODE_COMPUTE, NULL, COMP_NOT_M, DEST_M, JUMP_UNKNOWN, 0);   // M=!M
                break;
            case OP_NEG:
                createMneumonic(&instructions[2], OPCODE_COMPUTE, NULL, COMP_NEG_M, DEST_M, JUMP_UNKNOWN, 0);   // M=-M
                break;
            default:
                return NULL;
        }
    }
    
    /* These operations all use one number and branching jumps */
    else if (command->op == OP_LT || command->op == OP_GT || command->op == OP_EQ) {
        /* Comparison operations will call to a predefined label called preable_true and preable_false
         * at these labels will be instructions to set the value at the top of the stack to true and false
         * respectively, then it will look into R13 and jump there */

        instructions = stackArenaPush(stack_arena, 15 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }
        total_instructions = 15;

        /* Create a temp string to hold the unique label for this operation, 9 is chosen to add on because
         * 1 for \0, 4 or the '.op.' part, and 4 for the operation number, which will be in hex */
        char* label_str = stackArenaPush(stack_arena, strlen(filename) + 9);
        if (label_str == NULL) {
            return NULL;
        }
        sprintf(label_str, "%s.op.%x", filename, instruction_counter++);
        
        createMneumonic(&instructions[6], OPCODE_A_SYMBOL, label_str, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);        // @FILENAME.op.operation_number
        createMneumonic(&instructions[7], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                          // D=A
        createMneumonic(&instructions[8], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);            // @R13
        createMneumonic(&instructions[9], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                          // M=D
        createMneumonic(&instructions[0], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);             // @SP
        createMneumonic(&instructions[1], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                          // A=M
        createMneumonic(&instructions[2], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                          // D=M
        createMneumonic(&instructions[3], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);             // @SP
        createMneumonic(&instructions[4], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_AM, JUMP_UNKNOWN, 0);                 // AM=M-1
        createMneumonic(&instructions[5], OPCODE_COMPUTE, NULL, COMP_D_MINUS_M, DEST_MD, JUMP_UNKNOWN, 0);                 // MD=D-M
        createMneumonic(&instructions[10], OPCODE_A_SYMBOL, "PREABLE_TRUE", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);  // @PREABLE_TRUE

        switch (command->op) {
            case OP_LT:
                createMneumonic(&instructions[11], OPCODE_JUMP, NULL, COMP_D, DEST_UNKNOWN, JUMP_JLT, 0);                  // D;JLT
                break;
            case OP_GT:
                createMneumonic(&instructions[11], OPCODE_JUMP, NULL, COMP_D, DEST_UNKNOWN, JUMP_JGT, 0);                  // D;JGT
                break;
            case OP_EQ:
                createMneumonic(&instructions[11], OPCODE_JUMP, NULL, COMP_D, DEST_UNKNOWN, JUMP_JEQ, 0);                  // D;JEQ
                break;

            default:
                return NULL;
        }

        createMneumonic(&instructions[12], OPCODE_A_SYMBOL, "PREABLE_FALSE", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);  // @PREABLE_FALSE
        createMneumonic(&instructions[13], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                           // 0;JMP
        createMneumonic(&instructions[14], OPCODE_SYMBOL, label_str, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // FILENAME.op.operation_number:

    }

    /* All other operations that operate on two numbers, ADD, SUB, AND, OR */
    else {
        instructions = stackArenaPush(stack_arena, 6 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }
        total_instructions = 6;

        createMneumonic(&instructions[0], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);   // @SP
        createMneumonic(&instructions[1], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                // A=M
        createMneumonic(&instructions[2], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                // D=M
        createMneumonic(&instructions[3], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);   // @SP
        createMneumonic(&instructions[4], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_AM, JUMP_UNKNOWN, 0);       // AM=M-1

        switch (command->op) {
            case OP_ADD:
                createMneumonic(&instructions[5], OPCODE_COMPUTE, NULL, COMP_D_PLUS_M, DEST_M, JUMP_UNKNOWN, 0);  // M=D+M
                break;
            case OP_SUB:
                createMneumonic(&instructions[5], OPCODE_COMPUTE, NULL, COMP_D_MINUS_M, DEST_M, JUMP_UNKNOWN, 0); // M=D-M
                break;
            case OP_AND:
                createMneumonic(&instructions[5], OPCODE_COMPUTE, NULL, COMP_D_AND_M, DEST_M, JUMP_UNKNOWN, 0);   // M=D&M
                break;
            case OP_OR:
                createMneumonic(&instructions[5], OPCODE_COMPUTE, NULL, COMP_D_OR_M, DEST_M, JUMP_UNKNOWN, 0);    // M=D|M
                break;
            default:
                return NULL;
        }
    }

    return generateMneumonics(instructions, total_instructions, stack_arena);
}

char* translateFlowCommand(stack_arena_t* stack_arena, command_t* command, const char* filename, uint16_t call_counter)
{
    size_t instructions_index = 0;
    mneumonic_t* instructions = NULL;

    /* All the operations that fall into this functions domain have no overlap of function
     * so, this will just be a simple conditional stucture, albeit very long */

    if (command->op == OP_LABEL) {
        instructions = stackArenaPush(stack_arena, sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }

        createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, command->arguments.flow.label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // (label)
    }

    else if (command->op == OP_CALL) {

        /* The + 7 is for the 4 hex characters that represent call_counter, 1 for the null terminator, and 2 for the periods*/
        char* return_label = stackArenaPush(stack_arena, strlen(command->arguments.flow.label) + strlen(filename) + 7);
        if (return_label == NULL) {
            return NULL;
        }
        sprintf(return_label, "%s.%s.%x", filename, command->arguments.flow.label, call_counter);

        /* 24 instructions are needed for this operation */
        instructions = stackArenaPush(stack_arena, 24 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @ARG
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_AM, JUMP_UNKNOWN, 0);            // AM=M+1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 
                command->arguments.flow.locals);                                                                                        // @#arguments
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A_PLUS_1, DEST_D, JUMP_UNKNOWN, 0);             // D=A+1
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_D, DEST_D, JUMP_UNKNOWN, 0);            // D=M-D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @ARG
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @LCL
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_AM, JUMP_UNKNOWN, 0);            // AM=M+1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, return_label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // @return_label
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                    // D=A
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_AM, JUMP_UNKNOWN, 0);            // AM=M+1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, command->arguments.flow.label,
                 COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                          // @function_name
        createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                     // 0;JMP
        createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, return_label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // (return_label)
    }
    else if (command->op == OP_GOTO) {
        /* 2 instructions are needed for this operation */
        instructions = stackArenaPush(stack_arena, 2 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }

        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, command->arguments.flow.label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // @label
        createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                     // 0;JMP
    }

    else if (command->op == OP_IFGOTO) {
        /* 6 instructions are needed for this operation */
        instructions = stackArenaPush(stack_arena, 6 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }

        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_MD, JUMP_UNKNOWN, 0);           // MD=M-1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_1, DEST_A, JUMP_UNKNOWN, 0);             // A=D+1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, command->arguments.flow.label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // @label
        createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_D, DEST_UNKNOWN, JUMP_JNE, 0);                     // D;JNE
    }

    else if (command->op == OP_RETURN) {
        /* 38 instructions are needed for this operation */
        instructions = stackArenaPush(stack_arena, 39 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }

        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                    // A=M
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R13
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @LCL
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_D, JUMP_UNKNOWN, 0);            // D=M-1
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_AM, JUMP_UNKNOWN, 0);                   // AM=D
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R14", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R14
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_AM, JUMP_UNKNOWN, 0);           // AM=M-1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @LCL
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_AM, JUMP_UNKNOWN, 0);           // AM=M-1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R15", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R15
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @ARG
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_D, JUMP_UNKNOWN, 0);            // D=M-1
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R15", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R15
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @ARG
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R13
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_AM, JUMP_UNKNOWN, 0);            // AM=M+1
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R14", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @R14
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                    // A=M
        createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                     // 0;JMP
    }

    else if (command->op == OP_FUNCTION) {
        /* 9 instructions are needed for this operation */
        instructions = stackArenaPush(stack_arena, 9 * sizeof(mneumonic_t));
        if (instructions == NULL) {
            return NULL;
        }

        createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, command->arguments.flow.label, 
                COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                           // (function)
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                    // D=M
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @LCL
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_1, DEST_M, JUMP_UNKNOWN, 0);             // M=D+1
        createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 
                command->arguments.flow.locals);                                                                                        // @#locals
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_A, DEST_D, JUMP_UNKNOWN, 0);             // D=D+A
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @SP
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                    // M=D
    }

    return generateMneumonics(instructions, instructions_index, stack_arena);
}

/* Translates a push memory command into assembly
 * return a null terminated string to the generated assembly code on success
 * return NULL on failure */
char* translatePushCommand(assembly_gen_t* assembly_gen, stack_arena_t* stack_arena, command_t* command)
{
    /* I try not to make functions this gargantuan, but this function is really simple so it will have to do */

    size_t instructions_index = 0;
    /* 10 is the most possible assembly instructions needed */
    mneumonic_t* instructions = stackArenaPush(stack_arena, 10 * sizeof(mneumonic_t));
    if (instructions == NULL) {
        return NULL;
    }

    switch (command->arguments.memory.segment) {
        case SEG_ARGUMENT:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @ARG
            break;

        case SEG_LOCAL:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);      // @LCL
            break;

        case SEG_POINTER:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, command->arguments.memory.index == 0 ? "THIS" : "THAT",  
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                                  // @THIS or @THAT
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                   // D=M
            break;

        case SEG_STATIC:
            if (command->arguments.memory.index > assembly_gen->total_static_variables) {
                 assembly_gen->total_static_variables = command->arguments.memory.index;
            }
            /* 16 is the memory address at which the static segment starts */
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 16 + command->arguments.memory.index + assembly_gen->static_variable_base);          // @index
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                   // A=M
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                   // D=M
            break;

        case SEG_CONSTANT:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, command->arguments.memory.index);                                                    // @constant
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                   // D=A
            break;

        case SEG_THIS:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THIS", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);    // @THIS
            break;

        case SEG_THAT:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THIS", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);    // @THAT
            break;

        case SEG_TEMP:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 5 + command->arguments.memory.index);                                                // @index
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                   // A=M
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                   // D=M
            break;

        default: return NULL;
    }

    /* This just helps with code duplication, I cant in my soul have 4 segments with the same exact code,
     * especially when its this much */
    if (command->arguments.memory.segment == SEG_ARGUMENT || command->arguments.memory.segment == SEG_LOCAL || 
        command->arguments.memory.segment == SEG_THIS || command->arguments.memory.segment == SEG_THAT) {

        if (command->arguments.memory.index > 0) {
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                // D=M
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
                    COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, command->arguments.memory.index);                                         // @index
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_A, DEST_A, JUMP_UNKNOWN, 0);         // A=D+A
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                // D=M
        }
        else {
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                // A=M
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                // D=M
        }
    }



    /* This is the assembly needed to increment the stack pointer */
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);  // @SP
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_AM, JUMP_UNKNOWN, 0);       // AM=M+1
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);               // M=D
    
    /* Reclaim some memory if applicable, 10 is the total structures initially allocated */
    if (instructions_index < 10) {
        stackArenaPop(stack_arena, (10 - instructions_index) * sizeof(mneumonic_t));
    }

    return generateMneumonics(instructions, instructions_index, stack_arena);
}

/* Translate a pop memory command into assembly
 * Return valid null terminated char* on success,
 * Return NULL otherwise  */
char* translatePopCommand(assembly_gen_t* assembly_gen, stack_arena_t* stack_arena, command_t* command)
{
    size_t instructions_index = 0;
    /* 18 is the most possible assembly instructions needed */
    mneumonic_t* instructions = stackArenaPush(stack_arena, 18 * sizeof(mneumonic_t));
    if (instructions == NULL) {
        return NULL;
    }

    /* Assemby instructions to pop the value off the stack */
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);           // @SP
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                        // A=M
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                        // D=M
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);           // @SP
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_MINUS_1, DEST_M, JUMP_UNKNOWN, 0);                // M=M-1
    
    /* This executes if the memory segment specified is one of the ones listed, and the index wanted of that
     * segment is over 1, because that will be the only time we need the code inside, this is for optimazation
     * purposes */ 
    if ( (command->arguments.memory.segment == SEG_ARGUMENT || command->arguments.memory.segment == SEG_LOCAL || 
          command->arguments.memory.segment == SEG_THIS || command->arguments.memory.segment == SEG_THAT ) &&
          (command->arguments.memory.index > 1) ) {
        createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @R13
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
    }

    /* Conditional logic to generate assembly for storing the data in the given segment */

    switch (command->arguments.memory.segment) {

        case SEG_ARGUMENT:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @ARG
            break;

        case SEG_LOCAL:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @LCL
            break;

        case SEG_POINTER:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, command->arguments.memory.index == 0 ? "THIS" : "THAT",
                    COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                                                                               // @THIS or @THAT
            break;

        case SEG_TEMP:
            /* 5 is the start of the temp segment in memory */
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 5 + command->arguments.memory.index);                                                     // @index
            break;

        case SEG_STATIC:
            if (command->arguments.memory.index > assembly_gen->total_static_variables) {
                 assembly_gen->total_static_variables = command->arguments.memory.index;
            }
            /* 16 is the memory address at which the static segment starts */
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, 
            COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 16 + command->arguments.memory.index + assembly_gen->static_variable_base);               // @index
            break;

        case SEG_THIS:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THIS", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);         // @THIS
            break;
        case SEG_THAT:
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THAT", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);         // @THAT
            break;

        /* This is invalid */
        case SEG_CONSTANT:
            return NULL;

        default:
            return NULL;
    }

    /* This just helps with code duplication, I cant in my soul have 4 segments with the same exact code,
     * especially when its this much */
    if ( (command->arguments.memory.segment == SEG_ARGUMENT || command->arguments.memory.segment == SEG_LOCAL || 
          command->arguments.memory.segment == SEG_THIS || command->arguments.memory.segment == SEG_THAT ) &&
          (command->arguments.memory.index > 0) ) {

        /* This is broken down to generate different assembly strings for the sake of optimization */
        if (command->arguments.memory.index == 1) {
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M_PLUS_1, DEST_A, JUMP_UNKNOWN, 0);                 // A=M+1
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
        }

        else {
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                        // D=M
            createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN,
                    command->arguments.memory.index);                                                                                           // @index
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_A, DEST_D, JUMP_UNKNOWN, 0);                 // D=D+A
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R14", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @R14
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @R13
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_D, JUMP_UNKNOWN, 0);                        // D=M
            createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R14", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @R14
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                        // A=M
            createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_D, JUMP_UNKNOWN, 0);                        // M=D
        }
    }

    else {
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                         // A=M
        createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                         // M=D
    }
    
    /* Reclaim some memory if applicable, 18 is the total structures initially allocated */
    if (instructions_index < 18) {
        stackArenaPop(stack_arena, (18 - instructions_index) * sizeof(mneumonic_t));
    }

    return generateMneumonics(instructions, instructions_index, stack_arena);
}

/* Translate VM command to assembly
 * Returns valid char* to a string of assembly instruction(s) on success
 * Returns NULL on failur */
char* translateCommand(assembly_gen_t* assembly_gen, stack_arena_t* stack_arena, command_t* command, const char* filename)
{
    /* This variable is used to count the number of function calls within a function */
    static uint16_t call_counter = 0;

    switch (command->op) {
        case OP_ADD:
        case OP_SUB:
        case OP_NEG:
        case OP_EQ:
        case OP_LT:
        case OP_GT:
        case OP_AND:
        case OP_OR:
        case OP_NOT:
            return translateLogicalCommand(stack_arena, command, filename);

        case OP_FUNCTION:
            call_counter = 0;  // Reset counter when a new function is declared
        case OP_LABEL:
        case OP_CALL:
            call_counter++;    // Otherwise on a call increment it
        case OP_GOTO:
        case OP_IFGOTO:
        case OP_RETURN:
            return translateFlowCommand(stack_arena, command, filename, call_counter);

        case OP_POP:
            return translatePopCommand(assembly_gen, stack_arena, command);
        case OP_PUSH:
            return translatePushCommand(assembly_gen, stack_arena, command);

        default:
            return NULL;
    }
}

/* Generates the preamble assembly code that kicks off the program. Needs to
 * be given an entry function name / symbol so that it knows where to jump to.
 * Returns -1 on failure
 * Return 0 on success */
int32_t assemblyGenPreamble(assembly_gen_t* assembly_gen, char* entry_function)
{
    stack_arena_t stack_arena;
    if (stackArenaInitialize(&stack_arena, 1024) < 0) {
        return -1;
    }

    command_t command;
    command.op = OP_CALL;
    command.arguments.flow.label = entry_function;
    command.arguments.flow.locals = 0; // The entry function has no arguments

    mneumonic_t instructions[21];
    size_t instructions_index = 0;
    char* assembly_str[2];

    /* Instructions to set the registers */
    createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 256);         // @256
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                        // D=A
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);           // @SP
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "ARG", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @ARG
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "LCL", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);          // @LCL
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
    createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 2048);        // @2048
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                        // D=A
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THIS", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);         // @THIS
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "THAT", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);         // @THAT
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                        // M=D
                                                                                                                                        
    assembly_str[0] = generateMneumonics(instructions, instructions_index, &stack_arena);
    instructions_index = 0; // Reset

    if (assembly_str[0] == NULL) {
        stackArenaRelease(&stack_arena);
        return -1;
    }

    /* Generate the needed code to call the starting function */
    assembly_str[1] = translateFlowCommand(&stack_arena, &command, "preamble", 0);
    if (assembly_str[1] == NULL) {
        stackArenaRelease(&stack_arena);
        return -1;
    }

    /* Pop the null terminator off the previous string generated,
     * this allows us to have the next string placed directly after
     * the previous one, thus making it one longer string */
    stackArenaPop(&stack_arena, 1);

    /* Generate the boolean setting logic */

    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "preamble_end", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                // @preamble_end
    createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, "preamble_end", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                   // (preamble_end)
    createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                                        // 0;JMP
    createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, "preable_bool_jumpback", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);         // (preable_bool_jumpback)
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "SP", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                          // @SP
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                                       // A=M
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D, DEST_M, JUMP_UNKNOWN, 0);                                       // M=D
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "R13", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                         // @R13
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_M, DEST_A, JUMP_UNKNOWN, 0);                                       // A=M
    createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                                        // 0;JMP
    createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, "preable_true", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                  // (preable_true)
    createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 32767);                      // @32765
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A_PLUS_1, DEST_D, JUMP_UNKNOWN, 0);                                // D=A+1
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_D_PLUS_A, DEST_D, JUMP_UNKNOWN, 0);                                // D=D+A
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "preable_bool_jumpback", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @preable_bool_jumpback
    createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                                        // 0;JMP
    createMneumonic(&instructions[instructions_index++], OPCODE_SYMBOL, "preable_false", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                 // (preable_false)
    createMneumonic(&instructions[instructions_index++], OPCODE_A_NUMBER, NULL, COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);                          // @0 
    createMneumonic(&instructions[instructions_index++], OPCODE_COMPUTE, NULL, COMP_A, DEST_D, JUMP_UNKNOWN, 0);                                       // D=A
    createMneumonic(&instructions[instructions_index++], OPCODE_A_SYMBOL, "preable_bool_jumpback", COMP_UNKNOWN, DEST_UNKNOWN, JUMP_UNKNOWN, 0);       // @preable_bool_jumpback
    createMneumonic(&instructions[instructions_index++], OPCODE_JUMP, NULL, COMP_0, DEST_UNKNOWN, JUMP_JMP, 0);                                        // 0;JMP
                                                                                                                                                       //
    if (generateMneumonics(instructions, instructions_index, &stack_arena) == NULL) {
        stackArenaRelease(&stack_arena);
        return -1;
    }
    
    size_t bytes_written = 0;
    bytes_written += fwrite(assembly_str[0], 1, strlen(assembly_str[0]), assembly_gen->output_file); 
    bytes_written += fwrite(assembly_str[1], 1, strlen(assembly_str[1]), assembly_gen->output_file); 

    if (bytes_written < (strlen(assembly_str[0]) + strlen(assembly_str[1]))) {
        stackArenaRelease(&stack_arena);
        return -1;
    }
    
    if (fflush(assembly_gen->output_file) < 0) {
        stackArenaRelease(&stack_arena);
        return -1;
    }

    stackArenaRelease(&stack_arena);
    return 0;
}

/* Generate assembly from the given commands and write them to the output file
 * Return -1 on failure
 * Return 0 on success */
int32_t assemblyGen(assembly_gen_t* assembly_gen, command_module_t* commands, const char* filename)
{
    /* Steps
     * 1. Create a stack arena of appropriate size
     * 2. Call translateCommand
     * 4. write output to file
     * 5. Flush output
     * 6. Zero the stack arena
     * 7. goto step 2
     */


    assert(assembly_gen != NULL && commands != NULL && filename != NULL &&
           assembly_gen->output_file != NULL && commands->total_commands > 0  && 
           commands->commands[0].op == OP_FUNCTION); 

    assembly_gen->static_variable_base = assembly_gen->total_static_variables;
    assembly_gen->total_static_variables = 0;

    stack_arena_t stack_arena;

    /* 512 bytes should be sufficient */
    if (stackArenaInitialize(&stack_arena, 4096) < 0) {
        return -1;
    }

    size_t command_index = 0;
    for (; command_index < commands->total_commands; command_index++) {

        char* assembly_str = translateCommand(assembly_gen, &stack_arena, &commands->commands[command_index], filename);
        if (assembly_str == NULL) {
            break;
        }

        size_t bytes = fwrite(assembly_str, 1, strlen(assembly_str), assembly_gen->output_file);
        if (bytes < strlen(assembly_str)) {
            break;
        }

        if (fflush(assembly_gen->output_file) < 0) {
            break;
        }

        stackArenaPop(&stack_arena, stack_arena.position);
    }

    /* If this condition is true then the loop didn't finish properly */
    if (command_index < commands->total_commands) {
        stackArenaRelease(&stack_arena);
        return -1;
    }

    return 0;
}

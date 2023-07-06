#ifndef ASSEMBLY_GEN_H
#define ASSEMBLY_GEN_H

#include "command.h"

#include <stdio.h>

/* Defines the structure and outwards function interaface for
 * the Assembly Generation Module */


typedef struct {
    command_module_t* commands;
    size_t            static_variable_base;     /* Base number for the static variable addresses
                                                 * this is needed because the memory segment is shared
                                                 * through the whole program, but the indexes are relative
                                                 * to the file */
    size_t            total_static_variables;   /* Holds the number of static variables used in the current file */

    FILE*             output_file;
} assembly_gen_t;

int32_t assemblyGenInitialize(assembly_gen_t* assembly_gen, const char* filepath);
void    assemblyGenDestroy(assembly_gen_t* assembly_gen);
int32_t assemblyGenPreamble(assembly_gen_t* assembly_gen, const char* entry_function);
int32_t assemblyGen(assembly_gen_t* assembly_gen, command_module_t* commands, const char* filename);


#endif

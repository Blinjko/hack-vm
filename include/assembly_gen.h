#ifndef ASSEMBLY_GEN_H
#define ASSEMBLY_GEN_H

#include "command.h"

#include <stdio.h>

/* Defines the structure and outwards function interaface for
 * the Assembly Generation Module */


typedef struct {
    command_module_t* commands;
    size_t            command_index;

    uint16_t          constants;
    size_t            total_constants;

    FILE*             output_file;
} assembly_gen_t;

int32_t assemblyGenInitialize(assembly_gen_t* assembly_gen, const char* filepath);
void    assemblyGenDestroy(assembly_gen_t* assembly_gen);
int32_t assemblyGenPreamble(assembly_gen_t* assembly_gen, const char* entry_function);
int32_t assemblyGen(assembly_gen_t* assembly_gen, command_module_t* commands, const char* filename);


#endif

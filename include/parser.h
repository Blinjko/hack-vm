#ifndef PARSER_H
#define PARSER_H

#include "command.h"
#include "stack_arena.h"

#include <sys/types.h>
#include <stdint.h>

/* This file contains the definitions for the parser structure(s) and functions */



typedef struct {
    char* file_map;
    size_t file_size;
} parser_t;



int32_t parserInitialize(parser_t* parser, const char* filepath);
void parserDestroy(parser_t* parser);

int32_t parserParseCommands(parser_t* parser, command_module_t* command_module, stack_arena_t* stack_arena);
#endif

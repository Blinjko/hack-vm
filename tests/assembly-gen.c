/* Basic test to see if the functions in parser.c work
 * as expected when parsing strings */

#include "../include/parser.h"
#include "../include/command.h"
#include "../include/assembly_gen.h"
#include "../include/stack_arena.h"


#include <stdio.h>


int main(int argc, char* argv[]) 
{
    parser_t parser;
    command_module_t command_module;
    stack_arena_t stack_arena;
    assembly_gen_t assembly_generator;

    if (parserInitialize(&parser, argv[1]) < 0) {
        fprintf(stderr, "Failed to initialize parser\n");
        return -1;
    }

    if (stackArenaInitialize(&stack_arena, 4096) < 0) {
        parserDestroy(&parser);
        return -1;
    }

    if (parserParseCommands(&parser, &command_module, &stack_arena) < 0) {
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        return -1;
    }

    if (assemblyGenInitialize(&assembly_generator, argv[2]) < 0) {
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        return -1;
    }

    if (assemblyGenPreamble(&assembly_generator, "main") < 0) {
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        assemblyGenDestroy(&assembly_generator);
        return -1;
    }

    if (assemblyGen(&assembly_generator, &command_module, argv[1]) < 0) {
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        assemblyGenDestroy(&assembly_generator);
        return -1;
    }

    parserDestroy(&parser);
    stackArenaRelease(&stack_arena);

    return 0;
}

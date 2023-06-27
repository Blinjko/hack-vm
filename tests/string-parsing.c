/* Basic test to see if the functions in parser.c work
 * as expected when parsing strings */

#include "../include/parser.h"
#include "../include/command.h"
#include "../include/stack_arena.h"


#include <stdio.h>


int main() 
{
    parser_t parser;
    command_module_t command_module;
    stack_arena_t stack_arena;

    if (parserInitialize(&parser, "string-parsing-test.vm") < 0) {
        fprintf(stderr, "Failed to initialize parser\n");
    }

    if (stackArenaInitialize(&stack_arena, 8 * parser.file_size) < 0) {
        parserDestroy(&parser);
    }


    parserParseCommands(&parser, &command_module, &stack_arena);

    parserDestroy(&parser);
    stackArenaRelease(&stack_arena);

    return 0;
}

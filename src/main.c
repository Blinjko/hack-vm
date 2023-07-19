#include "../include/parser.h"
#include "../include/command.h"
#include "../include/assembly_gen.h"
#include "../include/stack_arena.h"


#include <stdio.h>
#include <stdlib.h>

void printUsage();

int main(int argc, char* argv[]) 
{
    parser_t parser;
    command_module_t command_module;
    stack_arena_t stack_arena;
    assembly_gen_t assembly_generator;

    if (argc < 3) {
        fprintf(stderr, "Improper evocation\n");
        printUsage();
        return -1;
    }

    if (parserInitialize(&parser, argv[1]) < 0) {
        fprintf(stderr, "Failed to initialize parser\n");
        return -1;
    }

    if (argc >= 4) {
        /* make the memory pool the specified size */
        if (stackArenaInitialize(&stack_arena, atol(argv[3])) < 0) {
            fprintf(stderr, "Failed to initialize memory pool with specified size, %s\n", argv[3]);
            parserDestroy(&parser);
            return -1;
        }
    }

    else {
        /* make the memory pool 2x the file size */
        if (stackArenaInitialize(&stack_arena, 2 * parser.file_size) < 0) {
            fprintf(stderr, "Failed to initialize memory pool\n");
            parserDestroy(&parser);
            return -1;
        }
    }

    if (parserParseCommands(&parser, &command_module, &stack_arena) < 0) {
        fprintf(stderr, "Failed to parse VM Code\n");
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        return -1;
    }

    if (assemblyGenInitialize(&assembly_generator, argv[2]) < 0) {
        fprintf(stderr, "Failed to initialize assembly generation unit\n");
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        return -1;
    }

    if (assemblyGenPreamble(&assembly_generator, "main") < 0) {
        fprintf(stderr, "Failed to generate assembly preamble\n");
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        assemblyGenDestroy(&assembly_generator);
        return -1;
    }

    if (assemblyGen(&assembly_generator, &command_module, argv[1]) < 0) {
        fprintf(stderr, "Failed to generate assembly\n");
        parserDestroy(&parser);
        stackArenaRelease(&stack_arena);
        assemblyGenDestroy(&assembly_generator);
        return -1;
    }

    parserDestroy(&parser);
    stackArenaRelease(&stack_arena);
    fprintf(stdout, "Success\n");

    return 0;
}

void printUsage()
{
    printf("USAGE: \n\tPROGRAM input_file.vm output_file.hack [parser_memory_pool_size]\n");
}

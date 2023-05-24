#include "../include/parser.h"
#include "../include/command.h"
#include "../include/stack_arena.h"
#include "../include/bool.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* Intialize the given parser with the given file.
 * Return 0 on success,
 * Return -1 on failure */
int32_t parserInitialize(parser_t* parser, const char* filepath)
{
    assert(parser != NULL && filepath != NULL);


    int32_t fd = open(filepath, O_RDWR);
    if (fd < 0) {
        return -1;
    }

    struct stat file_status;
    if (fstat(fd, &file_status) < 0) {
        close(fd);
        return -1;
    }

    parser->file_size = (size_t) file_status.st_size;

    parser->file_map = mmap(NULL, parser->file_size, PROT_READ |  PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (parser->file_map == MAP_FAILED) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Destroys a parser structure, that is to say it umaps the file */
void parserDestroy(parser_t* parser)
{
    assert(parser != NULL && parser->file_map != NULL);


    munmap(parser->file_map, parser->file_size);

    parser->file_map = NULL;
    parser->file_size = 0;
}


/* Function to count the occurances of c in a memory segment,
 * stdlib didn't provide one so here we are.
 * Return the amount of occuances, if given nullptr undefined behaviour  */
static size_t memCountByte(const void* str, int c, size_t size)
{

    uint8_t* prev_pos = memchr(str, c, size);

    size_t count = prev_pos != NULL ? 1 : 0;

    while ((prev_pos = memchr(prev_pos + 1, c, (size - (size_t) (prev_pos - (uint8_t*)str)))) != NULL) {
        count += 1;
    }

    return count;
}

/* Chunk a file by its lines, line_pointers holds pointers to the
 * start of each line, */
static void parserChunkFile(parser_t* parser, char** line_pointers, size_t line_pointers_size)
{
    line_pointers[0] = parser->file_map;

    for (size_t index = 1; index < line_pointers_size && line_pointers[index] != NULL; index++) {

        // For readability
        size_t new_size = (parser->file_size - (size_t) (line_pointers[index - 1] - parser->file_map));

        line_pointers[index] = memchr(line_pointers[index - 1], (int) '\n', new_size) + 1;
    }
}




/* Like strtok, but it stops at end_ptr instead of trying to take the strings lengt
   * It also ingores successive delimating characters.
   * Meant to be called with NULL for subsequent tokenization of the same string,
   * Return pointer to next token or NULL if there are no more tokens */
static char* tokenizeCommand(char* command, char* command_end, const char* delim)
{
    static char* str_ptr;

    if (command != NULL) { 
        str_ptr = command;
    } 

    
    if (str_ptr == NULL) {
        return NULL;
    }

    // Indicate if the previous character was a delimter
    bool prev_delim = FALSE;

    for (size_t position = 0; (str_ptr + position) != command_end; position++) { 

        bool is_delim = strchr(delim, str_ptr[position]) != NULL;

        if (prev_delim && !is_delim) { 
            str_ptr[position - 1] = '\0';
            char* temp = str_ptr;
            str_ptr = &str_ptr[position];

            return temp;
        } 

        else if (!prev_delim && is_delim) { 
            prev_delim = TRUE;
        } 
    } 

    char* temp = str_ptr;
    str_ptr = NULL;
    return temp;
}

/* Parse the given line into a command structure 
 * Return 0 on success
 * Return -1 on failure */
static int32_t parserParseCommand(parser_t* parser, char* line_pointer, command_t* command)
{

    return 0;
}


/* Parse all the commands in the mapped file into the given command_module
 * memory allocations will be done on stack_arena AND not free'd on failure
 * return 0 on succes,
 * return -1 on failure */
int32_t parserParseCommands(parser_t* parser, command_module_t* command_module, stack_arena_t* stack_arena)
{
    /* Steps to parse the file
     * 0. Count the amount of newlines in the file to predict the size of command_module
     * 1. Parse a line of the file, delimeted by \n
     * 2. Give the line to another function to parse it into a command
     *    - note there is only 1 command per line
     * 3. Add the command to the command module
     * 4. Repeat until we reach the end of the file
     * 5. Return results */

    assert(parser != NULL && parser->file_map != NULL && command_module != NULL && stack_arena != NULL);
    
    // Count the number of commands
    command_module->total_commands = memCountByte(parser->file_map, (int) '\n', parser->file_size);

    // Allocate the array of commands
    command_module->commands = stackArenaPush(stack_arena, command_module->total_commands * sizeof(command_t));
    if (command_module->commands == NULL) {
        return -1;
    }

    // Allocate an array of the indicies of each line
    char** line_pointers = stackArenaPush(stack_arena, sizeof(char*) * command_module->total_commands);
    if (line_pointers == NULL) {
        return -1;
    }

    // Chunk the file into its lines
    parserChunkFile(parser, line_pointers, command_module->total_commands);

    for (size_t index = 0; 
         index < command_module->total_commands && line_pointers[index] != NULL; 
         index++) {

        // Parse the current line
        if (0 > parserParseCommand(parser, line_pointers[index], &command_module->commands[index])) {
            return -1;
        }
    }

    return 0;
}

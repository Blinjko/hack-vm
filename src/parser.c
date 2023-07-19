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


/* String to keyword mappings */

/* Mappings are relevent to enum positions */
static char const* const OPERAND_KEYWORD_MAPPING[17] = {"add", "sub", "neg", "and", "or", "not", "lt", "gt", "eq", "push", "pop", "label", "goto", "if-goto", "function", "call", "return"}; 
static char const* const MEMORY_SEGMENT_KEYWORD_MAPPING[8] = {"argument", "local", "static", "constant", "this", "that", "pointer", "temp"};


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


/* Function to count the occurances of a character (c) in a memory segment,
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

    for (size_t index = 1; index < line_pointers_size && line_pointers[index - 1] != NULL; index++) {

        // For readability
        size_t new_size = (parser->file_size - (size_t) (line_pointers[index - 1] - parser->file_map));

        line_pointers[index] =  (char*) memchr(line_pointers[index - 1], (int) '\n', new_size) + 1;
        *(line_pointers[index] - 1) = '\0';
    }
}

/* Translate an operator keyword to its corresponding enum value 
 * String is assumed to be null terminated
 * Return OP_UNKNOWN if the given keyword is unknown.
 * Return corresponding operation otherwise */
static operator_t translateOperatorString(char* str)
{
    for (size_t index = 0; index < OP_MAX; index++) {

        if (strcmp(str, OPERAND_KEYWORD_MAPPING[index]) == 0) {
            return (operator_t) index;
        }
    }

    return OP_UNKNOWN;
}

/* Translate a memory segment keyword to its corresponding enum value
 * passed string is assumed to be null terminated
 * Return SEG_UNKNOWN if the keyword is unknown
 * Return corresponding segment enumeration otherwise */
static memory_segment_t translateMemorySegmentString(char* str)
{
    for (size_t index = 0; index < SEG_MAX; index++) {

        if (strcmp(str, MEMORY_SEGMENT_KEYWORD_MAPPING[index]) == 0) {
            return (memory_segment_t) index;
        }
    }

    return SEG_UNKNOWN;
}

/* Parse the given line into a command structure 
 * Return 0 on success
 * Return -1 on failure */
static int32_t parserParseCommand(stack_arena_t* stack_arena, char* line_pointer, command_t* command)
{
    /* Process
     * Tokenize the line
     * Run the first token through the keyword translator
     * depending on what keyword it is either
     *  - Translate the memory keyword and index value
     *  - Copy the label name into a string
     *  - Copy the function name into a string and the number of arguments
     */

    /* Variable to list the delimeters */
    const char delimeters[] = " \n";
    char* const line_end = strchr(line_pointer, '\n');

    char* token = strtok(line_pointer, delimeters);
    if (token == NULL) {
        return -1;
    }

    command->op = translateOperatorString(token);

    /* Could be a switch statement, but I think this looks neater -\_(x_x)_/- */

    if (command->op  == OP_UNKNOWN) {
        return -1;
    }
    else if (command->op == OP_PUSH || command->op == OP_POP) {
        // memory shiz
        token = strtok(NULL, delimeters);
        if (token == NULL) {
            return -1;
        }

        command->arguments.memory.segment = translateMemorySegmentString(token);

        if (command->arguments.memory.segment == SEG_UNKNOWN) {
            return -1;
        }

        // Get index value
        token = strtok(NULL, delimeters);
        if (token == NULL) {
            return -1;
        }

        char* endptr = line_end;
        command->arguments.memory.index = strtol(token, &endptr, 10);

        /* endptr should be have the value '\0' if the string was valid */
        if (*endptr != '\0') {
            return -1;
        }
    }


    else if (command->op == OP_LABEL    || command->op == OP_GOTO || command->op == OP_IFGOTO ||
             command->op == OP_FUNCTION || command->op == OP_CALL) {
        // Non uninariy Flow control
        token = strtok(NULL, delimeters);
        if (token == NULL) {
            return -1;
        }

        size_t length = strlen(token);
        command->arguments.flow.label = stackArenaPush(stack_arena, length + 1);
        if (command->arguments.flow.label == NULL) {
            return -1;
        }

        strncpy(command->arguments.flow.label, token, length); // might be off by one error
        
        /* The Function and Call keywords have a label and a subsequent number */
        if (command->op == OP_FUNCTION || command->op == OP_CALL) {

            token = strtok(NULL, delimeters);
            if (token == NULL) {
                return -1;
            }

            char* endptr = line_end;
            command->arguments.flow.locals = strtol(token, &endptr, 10);

            /* endptr should be have the value '\0' if the string was valid */
            if (*endptr != '\0') {
                return -1;
            }
        }
    }

    /* Otherwise it must be a uninary operator and no more parsing is needed */

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
        if (0 > parserParseCommand(stack_arena, line_pointers[index], &command_module->commands[index])) {
            return -1;
        }
    }

    return 0;
}

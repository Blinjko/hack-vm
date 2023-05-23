#include "../include/parser.h"
#include "../include/command.h"
#include "../include/stack_arena.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>


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

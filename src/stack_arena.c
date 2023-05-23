#include "../include/stack_arena.h"

#include <assert.h>
#include <sys/mman.h>
#include <stdlib.h>

/* Definitions of arena_t's function interfaces */

/* Create a new area of the given size */
int32_t stackArenaInitialize(stack_arena_t* arena, size_t size)
{
    assert(arena != NULL && size != 0);

    arena->size = 0;
    arena->position = 0;
    arena->memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena->memory == MAP_FAILED) {
        arena->memory = NULL;
        return -1;
    }

    arena->size = size;
    return 0;
}


/* Release the memory held by the arena */
void stackArenaRelease(stack_arena_t* arena)
{
    assert(arena != NULL && arena->memory != NULL);

    munmap(arena->memory, arena->size);

    arena->memory = NULL;
    arena->size = 0;
    arena->position = 0;
}

/* Push memory onto the stack 
 * Return null if not enough space for the requested amount*/
void* stackArenaPush(stack_arena_t* arena, size_t amount)
{
    assert(arena != NULL && arena->size != 0 && amount != 0);

    if ((arena->size - arena->position) < amount) {
        return NULL;
    }

    void* new_memory = (void*) (arena->memory + arena->position);

    arena->position += amount;

    return new_memory;
}

/* Pop memory off the stack */
void  stackArenaPop(stack_arena_t* arena, size_t amount)
{
    assert(arena != NULL && amount <= arena->position);

    arena->position -= amount;
}

/* Get the current stack position */
size_t stackArenaPosition(stack_arena_t* arena)
{
    assert(arena != NULL);

    return arena->position;
}

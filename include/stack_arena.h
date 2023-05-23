#ifndef STACK_ARENA_H
#define STACK_ARENA_H

#include <sys/types.h>
#include <stdint.h>

/* Defines the Arena structure and interface */


typedef struct {
    uint8_t* memory;
    size_t size;
    size_t position;
} stack_arena_t;


int32_t stackArenaInitialize(stack_arena_t* arena, size_t size);
void stackArenaRelease(stack_arena_t* arena);

void* stackArenaPush(stack_arena_t* arena, size_t amount);
void  stackArenaPop(stack_arena_t* arena, size_t amount);

size_t stackArenaPosition(stack_arena_t* arena);

#endif

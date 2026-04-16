#include "arena.h"
#include <stdio.h>
#include <inttypes.h>

ARENA_ERROR makeArena(Arena* arena, u64 sizeInBytes){
    void* tempAddress = malloc(sizeInBytes);
    if (tempAddress == NULL)
        return ARENA_CREATION_FAILURE;
    // Batch write to the struct to avoid cache misses.
	arena->arenaBasePointer = tempAddress;
	arena->arenaSize = sizeInBytes;
	arena->arenaSizeCommitted = sizeInBytes;
	arena->arenaSizeLeftInCommitted = sizeInBytes;
	arena->arenaLatestPointer = tempAddress;
    return ARENA_OK;
}

ARENA_ERROR pushData(Arena* arena, void* data, u64 sizeInBytes, void** outMemoryPointer){
    if (sizeInBytes > arena->arenaSizeLeftInCommitted)
        return ARENA_NO_SPACE_FOR_DATA;
    if (data != NULL)
        memcpy(arena->arenaLatestPointer, data, sizeInBytes);
    byte* newAddress = (byte*)(arena->arenaLatestPointer);
    newAddress += sizeInBytes;
    *outMemoryPointer = arena->arenaLatestPointer;
    arena->arenaLatestPointer = (void*)(newAddress);
    arena->arenaSizeLeftInCommitted = arena->arenaSizeLeftInCommitted - sizeInBytes;
    //printArenaInfo(arena);
    return ARENA_OK;
}

ARENA_ERROR popData(Arena* arena, u64 sizeInBytes){ // Since we want to pop items as a stack, we won't be deleting this from the middle anyways.
    if (sizeInBytes > arena->arenaSizeCommitted) {
        return ARENA_CANT_DELETE_RESERVED;
    }
    if (sizeInBytes > (arena->arenaSizeCommitted - arena->arenaSizeLeftInCommitted))
        return ARENA_CANT_DELETE_NULL;
    byte* newArenaPointer = (byte*)(arena->arenaLatestPointer) - sizeInBytes;
    arena->arenaSizeLeftInCommitted = arena->arenaSizeLeftInCommitted - sizeInBytes;
    arena->arenaLatestPointer = (void*)(newArenaPointer);
    return ARENA_OK;
}

ARENA_ERROR removeArena(Arena* arena){
    free(arena->arenaBasePointer);
    return ARENA_OK;
}

ARENA_ERROR castPointer(Arena* arena, void** outMemoryPointer){
    *outMemoryPointer = arena->arenaLatestPointer;
    return ARENA_OK;
}

ARENA_ERROR pushPointer(Arena* arena, u32 offsetInBytes){
    if(offsetInBytes > arena->arenaSizeLeftInCommitted){
        return ARENA_NO_SPACE_FOR_DATA;
    }
    byte* newArenaPointer = (byte*)(arena->arenaLatestPointer) + offsetInBytes;
    arena->arenaLatestPointer = (void*)(newArenaPointer);
    return ARENA_OK;
}

void printArenaInfo(Arena* arena){
    fprintf(stderr, "arenaSize: %"PRIu64"\n", arena->arenaSize);
    fprintf(stderr, "arenaSizeCommitted: %"PRIu64"\n", arena->arenaSizeCommitted);
    fprintf(stderr, "arenaSizeLeftInCommitted: %"PRIu64"\n", arena->arenaSizeLeftInCommitted);
}


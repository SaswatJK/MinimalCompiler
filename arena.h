#include <assert.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>

typedef enum{
    ARENA_NO_SPACE_FOR_COMMIT = 322,
    ARENA_NO_SPACE_FOR_DATA,
    ARENA_CREATION_FAILURE,
    ARENA_COMMIT_FAILURE,
    ARENA_FREE_FAILURE,
    ARENA_CANT_DELETE_RESERVED,
    ARENA_CANT_DELETE_NULL,
    ARENA_OK
}ARENA_ERROR;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;

typedef u8 byte;

#define KiB(x) ((x) * 1024ULL)
#define MiB(x) ((x) * 1024ULL * 1024ULL)
#define GiB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)

#define PUSH_ARRAY_IN_ARENA(arena, type, num, dataPointer, newPointer) pushData((arena), (dataPointer), (num) * sizeof(type), (void**)(&(newPointer)))
// Insert the Array directly in the arena. By doing a memcpy.
#define PUSH_EMPTY_ARRAY_IN_ARENA(arena, type, num, newPointer) pushData((arena), NULL, (num) * sizeof(type), (void**)(&(newPointer)))
// Push the pointer of the arena forward, but receieve a pointer to the start of the empty region.
#define PUSH_OBJECT_IN_ARENA(arena, type, dataPointer, newPointer) pushData((arena), (dataPointer), sizeof(type), (void**)(&(newPointer)))
// Insert an obect directly in the arena. By doing a memcpy again.
#define PUSH_EMPTY_OBJECT_IN_ARENA(arena, type, newPointer) pushData((arena), NULL, sizeof(type), (void**)(&(newPointer)))
// Push the pointer of the arena forward, but receieve a pointer to the start of the empty region.
#define POP_DATA_IN_ARENA(arena, type, num) popData((arena),(num) * sizeof(type))
// Pop data in arena.
#define GET_POINTER_IN_ARENA(arena, type, newPointer) castPointer((arena), (void**)(&(newPointer)))
// Get the pointer of the arena.
#define PUSH_POINTER_IN_ARENA(arena, type, num) pushPointer((arena), sizeof(type) * (num))
// Like Pushing empty but more freedom.

typedef struct ArenaStruct{
    u64 arenaSize; // Size of arena.
    u64 arenaSizeCommitted;
    u64 arenaSizeLeftInCommitted;
    void* arenaBasePointer;
    void* arenaLatestPointer;
}Arena;

ARENA_ERROR makeArena(Arena* arena, u64 sizeInBytes);
ARENA_ERROR pushData(Arena* arena, void* data, u64 sizeInBytes, void** outMemoryPointer);
ARENA_ERROR popData(Arena* arena, u64 sizeInBytes); // Since we want to pop items as a stack, we won't be deleting this from the middle anyways.
ARENA_ERROR removeArena(Arena* arena);
ARENA_ERROR pushPointer(Arena* arena, u32 offsetInBytes);
ARENA_ERROR castPointer(Arena* arena, void** outMemoryPointer);

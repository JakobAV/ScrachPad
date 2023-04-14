#include "Shared.h"
#include <windows.h>
#include <cassert>

#ifndef ARENA_RESERVE_SIZE 
#define ARENA_RESERVE_SIZE GB(4)
#endif

struct MemoryArena
{
    u8* base;
    u64 currentSize;
    u64 commitSize;
    u64 _unused[5];
};


struct TempMemory
{
    MemoryArena* arena;
    u64 startPos;
};

u32 GetPageSize()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

MemoryArena* InitArena(u32 size = 0)
{
    static_assert(sizeof(MemoryArena) == 64);
    MemoryArena* arena = (MemoryArena*)VirtualAlloc(0, ARENA_RESERVE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    u32 granularity = GetPageSize();
    u32 actualSize = size + sizeof(MemoryArena) + granularity - 1;
    actualSize -= actualSize%granularity;
    VirtualAlloc(arena, actualSize, MEM_COMMIT, PAGE_READWRITE);
    arena->base = (u8*)arena;
    arena->currentSize = sizeof(MemoryArena);
    arena->commitSize = actualSize;
    return arena;
}

#define PushType(arena, type) (type*)ArenaPush(arena, sizeof(type))
#define PushArray(arena, type, count) (type*)ArenaPush(arena, sizeof(type) * count)

void* ArenaPush(MemoryArena* arena, u32 size)
{
    if(arena->currentSize + size > arena->commitSize)
    {
        u32 granularity = GetPageSize();
        u32 actualSize = size + granularity - 1;
        actualSize -= actualSize%granularity;
        assert(arena->commitSize + actualSize <= ARENA_RESERVE_SIZE);
        VirtualAlloc(arena->base + arena->commitSize, actualSize, MEM_COMMIT, PAGE_READWRITE);
        arena->commitSize += actualSize;
    }
    void* result = arena->base + arena->currentSize;
    arena->currentSize += size;
    return result;
}

void ArenaPopTo(MemoryArena* arena, u64 position)
{
    // TODO: De-commit memory if commitSize - currentSize > 4x PageSize.
    arena->currentSize = position;
}

TempMemory BeginTempMemory(MemoryArena* arena)
{
    TempMemory result;
    result.arena = arena;
    result.startPos = arena->currentSize;
    return result;
}

void EndTempMemory(TempMemory tempMemory)
{
    ArenaPopTo(tempMemory.arena, tempMemory.startPos);
}

void FreeArena(MemoryArena* arena)
{
    VirtualFree(arena->base, 0, MEM_RELEASE);
}

void TestMemoryArena()
{
    MemoryArena* arena = InitArena();
    int* num1 = PushType(arena, int);
    *num1 = 1;
    TempMemory block = BeginTempMemory(arena);
    int* array1 = PushArray(arena, int, MB(1));
    for (int i = 0; i < MB(1); ++i)
    {
        array1[i] = i;
    }
    EndTempMemory(block);
    float* array2 = PushArray(arena, float, KB(1));
    for (int i = 0; i < MB(1); ++i)
    {
        array2[i] = i + 0.1f;
    }

    FreeArena(arena);
}
#include "MemoryArena.h"
#include "Shared.h"
#include <windows.h>
#include <cassert>

#ifndef ARENA_RESERVE_SIZE 
#define ARENA_RESERVE_SIZE GB(4)
#endif
#ifndef ARENA_DECOMMIT_THRESHOLD
#define ARENA_DECOMMIT_THRESHOLD MB(4)
#endif

u32 GetPageSize()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

MemoryArena* CreateArena(u32 initialCommitSize)
{
    static_assert(sizeof(MemoryArena) == 64, "Ensure that we \"start\" the arena at the start of the next cache line");
    MemoryArena* arena = (MemoryArena*)VirtualAlloc(0, ARENA_RESERVE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    assert(arena != nullptr);
    u32 granularity = GetPageSize();
    u32 actualSize = initialCommitSize + sizeof(MemoryArena) + granularity - 1;
    actualSize -= actualSize%granularity;
    VirtualAlloc(arena, actualSize, MEM_COMMIT, PAGE_READWRITE);
    arena->base = (u8*)arena;
    arena->currentSize = sizeof(MemoryArena);
    arena->commitSize = actualSize;
    return arena;
}

void* ArenaPush(MemoryArena* arena, u32 size)
{
    //TODO: Add suport for aligning memory
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
    arena->currentSize = position;
    u32 pageSize = GetPageSize();
    u64 currentSizeAllignedToPageSize = arena->currentSize + pageSize -1;
    currentSizeAllignedToPageSize -= currentSizeAllignedToPageSize % pageSize;
    if(currentSizeAllignedToPageSize + ARENA_DECOMMIT_THRESHOLD <= arena->commitSize)
    {
        u64 sizeToDecommit = arena->commitSize-currentSizeAllignedToPageSize;
        /*
        This is so stupid. It is a warning about a using a FEATURE of the Virtual Memory...
        It is also the whole point of this architecture, allowing us to grow and shrink the physical memory usage
        while still having the same base pointer and not having to do dome shit with copying or managing allocation blocks.
        */
#pragma warning(push)
#pragma warning(disable : 6250)
        VirtualFree(arena->base + currentSizeAllignedToPageSize, sizeToDecommit, MEM_DECOMMIT);
#pragma warning(pop)
        arena->commitSize -= sizeToDecommit;
    }
}

TempMemory BeginTempMemory(MemoryArena* arena)
{
    TempMemory result;
    result.arena = arena;
    result.startPos = arena->currentSize;
    return result;
}

void EndTempMemory(TempMemory* tempMemory)
{
    ArenaPopTo(tempMemory->arena, tempMemory->startPos);
    tempMemory->arena = nullptr;
    tempMemory->startPos = 0;
}

void FreeArena(MemoryArena* arena)
{
    VirtualFree(arena->base, 0, MEM_RELEASE);
}

void TestMemoryArena()
{
    MemoryArena* arena = CreateArena();
    int* num1 = PushType(arena, int);
    *num1 = 1;
    UseTempMemory(arena)
    {
        float* array2 = PushArray(arena, float, KB(1));
        for (int i = 0; i < KB(1); ++i)
        {
            array2[i] = i + 0.1f;
        }
    }
    TempMemory block = BeginTempMemory(arena);
    int* array1 = PushArray(arena, int, MB(1));
    for (int i = 0; i < MB(1); ++i)
    {
        array1[i] = i;
    }
    EndTempMemory(&block);

    FreeArena(arena);
}
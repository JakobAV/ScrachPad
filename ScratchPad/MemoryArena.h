#include "Shared.h"

struct MemoryArena
{
    u8* base;
    u64 currentSize;
    u64 commitSize;
    u64 _unused[5]; // To make the struct 64 bytes (one cache line)
};


struct TempMemory
{
    MemoryArena* arena;
    u64 startPos;
};

#define PushType(arena, type) (type*)ArenaPush(arena, sizeof(type))
#define PushArray(arena, type, count) (type*)ArenaPush(arena, sizeof(type) * count)
#define PopType(arena, type) ArenaPop(arena, sizeof(type))
#define PopArray(arena, type, count) ArenaPop(arena, sizeof(type) * count)
#define UseTempMemory(arena) for (TempMemory _block_ = BeginTempMemory(arena); _block_.arena!=nullptr; EndTempMemory(&_block_))

MemoryArena* CreateArena(u32 initialCommitSize = 0);
void* ArenaPush(MemoryArena* arena, u32 size);
void ArenaPop(MemoryArena* arena, u32 size);
void ArenaPopTo(MemoryArena* arena, u64 position);
void ArenaClear(MemoryArena* arena);
TempMemory BeginTempMemory(MemoryArena* arena);
void EndTempMemory(TempMemory tempMemory);
void FreeArena(MemoryArena* arena);

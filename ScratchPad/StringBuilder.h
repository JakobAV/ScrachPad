#pragma once
#include "Shared.h"

struct StringBuilderChunk
{
    u32 length;
    u32 startIndex;
    char data[64];
    StringBuilderChunk* nextChunk;
};

struct StringBuilder
{
    u32 length;
    StringBuilderChunk* first;
    StringBuilderChunk* last;
};

struct StringBuilderIndex
{
    StringBuilderChunk* chunk;
    u32 index;
};

StringBuilder CreateStringBuilder();
StringLit BuildString(StringBuilder* stringBuilder, MemoryArena* arena);
StringBuilder CopyStringBuilder(const StringBuilder* stringBuilder);

void Append(StringBuilder* stringBuilder, const char* str, u32 length);
void Append(StringBuilder* stringBuilder, StringLit str);
void Append(StringBuilder* stringBuilder, const StringBuilder* str);

void Prepend(StringBuilder* stringBuilder, const char* str, u32 length);
void Prepend(StringBuilder* stringBuilder, StringLit str);
void Prepend(StringBuilder* stringBuilder, const StringBuilder* str);

StringBuilderIndex IndexOf(const StringBuilder* stringBuilder, const char* str, u32 length);
StringBuilderIndex IndexOf(const StringBuilder* stringBuilder, StringLit str);
StringBuilderIndex IndexOf(const StringBuilder* stringBuilder, const StringBuilder* str);

StringBuilder* Split(const StringBuilder* stringBuilder, const char* str, u32 length, u32* splitNum, MemoryArena* arena);
StringBuilder* Split(const StringBuilder* stringBuilder, StringLit str, u32* splitNum, MemoryArena* arena);
StringBuilder* Split(const StringBuilder* stringBuilder, const StringBuilder* str, u32* splitNum, MemoryArena* arena);
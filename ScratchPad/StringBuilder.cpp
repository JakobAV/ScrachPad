#include "StringBuilder.h"

thread_local StringBuilderChunk* ChunkFreeList = nullptr;
thread_local MemoryArena* ChunkArena = CreateArena();

StringBuilderChunk* GetFreeChunk()
{
    StringBuilderChunk* result = ChunkFreeList;
    if (result)
    {
        ChunkFreeList = result->nextChunk;
    }
    else
    {
        result = PushType(ChunkArena, StringBuilderChunk);
    }
    *result = {};
    return result;
}

void FreeChunk(StringBuilderChunk* chunk)
{
    chunk->nextChunk = ChunkFreeList;
    ChunkFreeList = chunk;
}

StringBuilder CreateStringBuilder()
{
    StringBuilder result = {};
    result.first = GetFreeChunk();
    result.last = result.first;
    return result;
}

StringLit BuildString(StringBuilder* stringBuilder, MemoryArena* arena)
{
    char* text = PushArray(arena, char, stringBuilder->length + 1);
    StringLit result = { stringBuilder->length, text };
    text[stringBuilder->length] = '\0';
    StringBuilderChunk* current = stringBuilder->first;
    u32 destIndex = 0;
    while (current)
    {
        char* src = current->data + current->startIndex;
        assert((destIndex + current->length) <= result.length);
        u32 remainder = result.length - destIndex;
        char* dest = text + destIndex;
        destIndex += current->length;
        memcpy_s(dest, remainder, src, current->length);
        StringBuilderChunk* prev = current;
        current = current->nextChunk;
        FreeChunk(prev);
    }
    *stringBuilder = {};
    return result;
}

StringBuilder CopyStringBuilder(const StringBuilder* stringBuilder)
{
    StringBuilder result = {};
    result.length = stringBuilder->length;
    const StringBuilderChunk* current = stringBuilder->first;
    result.first = GetFreeChunk();
    result.last = result.first;
    *result.first = *current;
    while (current->nextChunk)
    {
        result.last->nextChunk = GetFreeChunk();
        result.last = result.last->nextChunk;
        *result.last = *current->nextChunk;
        current = current->nextChunk;
    }
    return result;
}

void Append(StringBuilder* stringBuilder, const char* str, u32 length)
{
    StringBuilderChunk* current = stringBuilder->last;
    u32 destIndex = current->startIndex + current->length;
    u32 remainder = ArrayCount(current->data) - destIndex;
    if (length > remainder)
    {
        // TODO: Get new chunk and split string accross them
        NotImplemented;
    }
    char* dest = current->data + destIndex;
    memcpy_s(dest, remainder, str, length);
    stringBuilder->length += length;
    current->length += length;
}

void Append(StringBuilder* stringBuilder, StringLit str)
{
    Append(stringBuilder, str.text, str.length);
}

void Append(StringBuilder* stringBuilder, const StringBuilder* str)
{
    StringBuilder copy = CopyStringBuilder(str);
    stringBuilder->length += str->length;
    stringBuilder->last->nextChunk = copy.first;
    stringBuilder->last = copy.last;
}

void Prepend(StringBuilder* stringBuilder, const char* str, u32 length)
{
    StringBuilderChunk* current = stringBuilder->first;
    u32 destIndex = current->startIndex - length;
    u32 remainder = current->startIndex;
    if (length > remainder)
    {
        StringBuilderChunk* newChunk = GetFreeChunk();
        newChunk->nextChunk = stringBuilder->first;
        stringBuilder->first = newChunk;
        u32 leftToPrepend = length - remainder;
        newChunk->startIndex = ArrayCount(newChunk->data);
        Prepend(stringBuilder, str, leftToPrepend);
        destIndex = 0;
        length = remainder;
    }
    char* dest = current->data + destIndex;
    memcpy_s(dest, remainder, str, length);
    current->length += length;
    current->startIndex -= length;
    stringBuilder->length += length;
}

void Prepend(StringBuilder* stringBuilder, StringLit str)
{
    Prepend(stringBuilder, str.text, str.length);
}

void Prepend(StringBuilder* stringBuilder, const StringBuilder* str)
{
    StringBuilder copy = CopyStringBuilder(str);
    stringBuilder->length += str->length;
    copy.last->nextChunk = stringBuilder->first;
    stringBuilder->first = copy.first;
}
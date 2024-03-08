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

void FreeStringBuilder(StringBuilder* stringBuilder)
{
	StringBuilderChunk* current = stringBuilder->first;
	while (current)
	{
		StringBuilderChunk* prev = current;
		current = current->nextChunk;
		FreeChunk(prev);
	}
	*stringBuilder = {};
}

void Append(StringBuilder* stringBuilder, const char* str, u32 length)
{
	StringBuilderChunk* current = stringBuilder->last;
	u32 destIndex = current->startIndex + current->length;
	u32 remainder = ArrayCount(current->data) - destIndex;
	if (length > remainder)
	{
		StringBuilderChunk* newChunk = GetFreeChunk();
		current->nextChunk = newChunk;
		stringBuilder->last = newChunk;
		u32 leftToAppend = length - remainder;
		Append(stringBuilder, str + remainder, leftToAppend);
		length = remainder;
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
		str += leftToPrepend;
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


void Insert(StringBuilder* stringBuilder, StringBuilderIndex insertIndex, const char* str, u32 length)
{
	assert(insertIndex.chunk);
	assert(insertIndex.index <= (s32)(insertIndex.chunk->startIndex + insertIndex.chunk->length));
	if (insertIndex.chunk == stringBuilder->last && insertIndex.index == stringBuilder->last->length)
	{
		Append(stringBuilder, str, length);
	}
	else
	{
		u32 relativeIndex = insertIndex.index - insertIndex.chunk->startIndex;
		u32 leftInChunk = ArrayCount(insertIndex.chunk->data) - (insertIndex.chunk->startIndex + insertIndex.chunk->length);
		u32 lengthToMove = insertIndex.chunk->length - relativeIndex;
		if (leftInChunk >= length)
		{
			for (u32 i = lengthToMove; i > 0; --i)
			{
				u32 toIndex = insertIndex.index + i + length - 1;
				u32 fromIndex = insertIndex.index + i - 1;
				insertIndex.chunk->data[toIndex] = insertIndex.chunk->data[fromIndex];
			}
			char* dest = insertIndex.chunk->data + insertIndex.index;
			memcpy_s(dest, length, str, length);
			insertIndex.chunk->length += length;
		}
		else
		{

			StringBuilder tempString = Slice(stringBuilder, insertIndex);
			Append(stringBuilder, str, length);
			StringBuilderChunk* tempChunk = stringBuilder->last;
			stringBuilder->last = tempString.last;
			tempChunk->nextChunk = tempString.first;
		}

		stringBuilder->length += length;
	}
}

void Insert(StringBuilder* stringBuilder, StringBuilderIndex insertIndex, StringLit str)
{
	Insert(stringBuilder, insertIndex, str.text, str.length);
}

void Insert(StringBuilder* stringBuilder, StringBuilderIndex insertIndex, const StringBuilder* str)
{
	assert(insertIndex.chunk);
	assert(insertIndex.index < (s32)(insertIndex.chunk->startIndex + insertIndex.chunk->length));

	StringBuilder tempString = Slice(stringBuilder, insertIndex);
	Append(stringBuilder, str);
	StringBuilderChunk* tempChunk = stringBuilder->last;
	stringBuilder->last = tempString.last;
	tempChunk->nextChunk = tempString.first;
	stringBuilder->length += tempString.length;
}


char GetCharAtIndex(StringBuilderIndex index)
{
	assert(index.chunk);
	assert(index.index < (s32)(index.chunk->startIndex + index.chunk->length));
	return index.chunk->data[index.index];
}

bool IncrementIndex(StringBuilderIndex* index, u32 incrementAmount = 1)
{
	assert(index->chunk);
	bool result = false;
	StringBuilderChunk* current = index->chunk;
	u32 currentIndex = index->index;
	u32 amountIncremented = 0;
	while (current)
	{
		u32 realEndOfArray = current->startIndex + current->length;
		u32 newIndex = currentIndex + incrementAmount - amountIncremented;
		if (realEndOfArray > newIndex)
		{
			index->chunk = current;
			index->index = newIndex;
			result = true;
			break;
		}
		else if (current->nextChunk == nullptr && realEndOfArray == newIndex)
		{
			index->chunk = current;
			index->index = newIndex;
			result = true;
			break;
		}
		amountIncremented += realEndOfArray - currentIndex;
		current = current->nextChunk;
		if (current)
		{
			currentIndex = current->startIndex;
		}
	}
	return result;
}

bool DecrementIndex(StringBuilderIndex* index, u32 decrementAmount = 1)
{
	assert(index->chunk);
	bool result = false;
	StringBuilderChunk* current = index->chunk;
	s32 currentIndex = index->index;
	s32 amountDecremented = 0;
	while (current)
	{
		s32 startOfArray = current->startIndex;
		s32 newIndex = currentIndex - decrementAmount + amountDecremented;
		if (newIndex >= startOfArray)
		{
			index->chunk = current;
			index->index = newIndex;
			result = true;
			break;
		}
		amountDecremented += currentIndex - newIndex;
		current = current->nextChunk;
		if (current)
		{
			currentIndex = current->startIndex + current->length - 1;
		}
	}
	return result;
}

StringBuilderIndex IndexOf(const StringBuilder* stringBuilder, const char* str, u32 length, StringBuilderIndex startFrom)
{
	StringBuilderIndex result = {};
	if (startFrom.chunk)
	{
		result = startFrom;
	}
	else
	{
		result.chunk = stringBuilder->first;
		result.index = stringBuilder->first->startIndex;
	}
	bool matchFound = false;
	if (length <= stringBuilder->length)
	{
		while (!matchFound)
		{
			matchFound = true;
			StringBuilderIndex localTest = result;
			for (u32 i = 0; i < length; ++i)
			{
				if (GetCharAtIndex(localTest) != str[i])
				{
					matchFound = false;
					break;
				}
				if (!IncrementIndex(&localTest))
				{
					break;
				}
			}
			if (!matchFound && !IncrementIndex(&result))
			{
				break;
			}
		}
	}
	if (!matchFound)
	{
		result.chunk = nullptr;
		result.index = -1;
	}
	return result;
}

StringBuilderIndex IndexOf(const StringBuilder* stringBuilder, StringLit str, StringBuilderIndex startFrom)
{
	return IndexOf(stringBuilder, str.text, str.length, startFrom);
}

u32 CalculateLength(const StringBuilder* stringBuilder)
{
	u32 result = 0;
	StringBuilderChunk* current = stringBuilder->first;
	while (current)
	{
		result += current->length;
		current = current->nextChunk;
	}
	return result;
}

StringBuilder SubString(const StringBuilder* stringBuilder, StringBuilderIndex start, StringBuilderIndex end)
{
	assert(start.chunk);
	assert(start.index < (s32)(start.chunk->startIndex + start.chunk->length));
	assert(end.chunk);
	assert(end.index <= (s32)(end.chunk->startIndex + end.chunk->length));
	StringBuilder result = {};

	const StringBuilderChunk* current = start.chunk;
	result.first = GetFreeChunk();
	result.last = result.first;
	*result.first = *current;
	result.first->length = result.first->length - start.index;
	result.first->startIndex = start.index;
	if (end.chunk != start.chunk)
	{
		while (current->nextChunk && end.chunk != current->nextChunk)
		{
			result.last->nextChunk = GetFreeChunk();
			result.last = result.last->nextChunk;
			*result.last = *current->nextChunk;
			current = current->nextChunk;
		}

		assert(end.chunk == current->nextChunk);
		result.last->nextChunk = GetFreeChunk();
		result.last = result.last->nextChunk;
		*result.last = *current->nextChunk;
	}

	result.last->length = end.index - result.last->startIndex;
	result.last->nextChunk = nullptr;
	result.length = CalculateLength(&result);

	return result;
}

StringBuilderIndex IndexOfStart(const StringBuilder* stringBuilder)
{
	StringBuilderIndex result = { stringBuilder->first, (s32)stringBuilder->first->startIndex };
	return result;
}

StringBuilderIndex IndexOfEnd(const StringBuilder* stringBuilder)
{
	StringBuilderIndex result = { stringBuilder->last, (s32)(stringBuilder->last->startIndex + stringBuilder->last->length) };
	return result;
}

StringBuilder* Split(const StringBuilder* stringBuilder, const char* str, u32 length, u32* splitNum, MemoryArena* arena)
{
	StringBuilder* result = nullptr;
	StringBuilderIndex start = IndexOfStart(stringBuilder);

	StringBuilderIndex index = IndexOf(stringBuilder, str, length, start);
	if (index.index == -1)
	{
		splitNum = 0;
		return result;
	}

	*splitNum = 2;

	ArenaSetAlignment(arena, alignof(StringBuilder));
	result = PushType(arena, StringBuilder);
	*result = SubString(stringBuilder, start, index);
	while (true)
	{

		IncrementIndex(&index, length);
		start = index;
		index = IndexOf(stringBuilder, str, length, start);
		if (index.index == -1)
		{
			break;
		}
		*splitNum += 1;
		*PushType(arena, StringBuilder) = SubString(stringBuilder, start, index);
	}
	index = IndexOfEnd(stringBuilder);
	*PushType(arena, StringBuilder) = SubString(stringBuilder, start, index);
	return result;
}

StringBuilder* Split(const StringBuilder* stringBuilder, StringLit str, u32* splitNum, MemoryArena* arena)
{
	return Split(stringBuilder, str.text, str.length, splitNum, arena);
}

StringBuilder Slice(StringBuilder* stringBuilder, StringBuilderIndex sliceAt)
{
	assert(sliceAt.chunk);
	assert(sliceAt.index < (s32)(sliceAt.chunk->startIndex + sliceAt.chunk->length));
	StringBuilder result = {};
	StringBuilderChunk* newChunk = GetFreeChunk();
	result.first = newChunk;

	if (stringBuilder->last == sliceAt.chunk)
	{
		result.last = newChunk;
	}
	else
	{
		result.last = stringBuilder->last;

		stringBuilder->last = sliceAt.chunk;
		newChunk->nextChunk = sliceAt.chunk->nextChunk;
		sliceAt.chunk->nextChunk = nullptr;
	}


	u32 relativeIndex = sliceAt.index - sliceAt.chunk->startIndex;
	u32 lengthToMove = sliceAt.chunk->length - relativeIndex;

	char* dest = newChunk->data;
	char* src = sliceAt.chunk->data + sliceAt.index;
	memcpy_s(dest, lengthToMove, src, lengthToMove);
	sliceAt.chunk->length -= lengthToMove;
	newChunk->length = lengthToMove;

	stringBuilder->length = CalculateLength(stringBuilder);
	result.length = CalculateLength(&result);

	return result;
}
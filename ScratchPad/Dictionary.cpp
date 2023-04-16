#include <stdlib.h>
#include <stdio.h>
#include "Shared.h"

#define fn static
#define assert(condition) if(!(condition)) { *((int*)0) = 0;}

struct DictionaryKey
{
	bool exists;
	unsigned int index;
};

struct Dictionary
{
	unsigned int size;
	unsigned int elementSize;
	unsigned int currentSize;
	DictionaryKey* keys;
	StringLit* keyStorage;
	char* data;
};

fn bool StringCompare(const StringLit& a, const StringLit& b)
{
	if (a.length != b.length)
	{
		return false;
	}
	for (unsigned int i = 0; i < a.length; ++i)
	{
		if (a.text[i] != b.text[i])
		{
			return false;
		}
	}
	return true;
}

fn void CopyMemory(char* dest, char* src, unsigned int length)
{
	for (unsigned int i = 0; i < length; ++i)
	{
		dest[i] = src[i];
	}
}

fn void CopyString(StringLit& dest, const StringLit& src)
{
	dest.text = (char*)malloc(src.length+1);
	CopyMemory((char*)dest.text, (char*)src.text, src.length+1);
	dest.length = src.length;
}

fn void FreeString(StringLit& string)
{
	string.length = 0;
	free((void*)string.text);
}

fn unsigned int hash(const StringLit& str)
{
	unsigned int hash = 5381;

	for (unsigned int i = 0; i < str.length; ++i)
	{
		hash = ((hash << 5) + hash) + str.text[i];
	}

	return hash;
}

fn Dictionary CreateDictionary(unsigned int elementSize, unsigned int size)
{
	Dictionary dictionary = {};
	dictionary.size = size;
	dictionary.elementSize = elementSize;
	dictionary.keys = (DictionaryKey*)malloc(sizeof(DictionaryKey) * dictionary.size);
	for (unsigned int i = 0; i < dictionary.size; ++i)
	{
		dictionary.keys[i] = {};
	}
	dictionary.keyStorage = (StringLit*)malloc(sizeof(StringLit) * dictionary.size);
	dictionary.data = (char*)malloc(dictionary.elementSize * dictionary.size);
	return dictionary;
}

fn void DictionaryAdd(Dictionary& dictionary, const StringLit& key, void* element)
{
	assert(dictionary.size >= dictionary.currentSize + 1);
	unsigned int keyHash = hash(key);
	unsigned int index = keyHash % dictionary.size;
	if (dictionary.keys[index].exists)
	{
		if (StringCompare(key, dictionary.keyStorage[dictionary.keys[index].index]))
		{
			CopyMemory(&dictionary.data[dictionary.keys[index].index * dictionary.elementSize], (char*)element, dictionary.elementSize);
		}
		else
		{
			while (true)
			{
				if (++index >= dictionary.size)
				{
					index -= dictionary.size;
				}
				if (!dictionary.keys[index].exists)
				{
					unsigned int internalIndex = dictionary.currentSize;
					CopyMemory(&dictionary.data[internalIndex * dictionary.elementSize], (char*)element, dictionary.elementSize);
					CopyString(dictionary.keyStorage[internalIndex], key);

					dictionary.keys[index] = { true, internalIndex };
					++dictionary.currentSize;
				}
				if (dictionary.keys[index].exists && StringCompare(key, dictionary.keyStorage[dictionary.keys[index].index]))
				{
					CopyMemory(&dictionary.data[dictionary.keys[index].index * dictionary.elementSize], (char*)element, dictionary.elementSize);
					break;
				}
			}
		}
	}
	else
	{
		unsigned int internalIndex = dictionary.currentSize;
		CopyMemory(&dictionary.data[internalIndex * dictionary.elementSize], (char*)element, dictionary.elementSize);
		CopyString(dictionary.keyStorage[internalIndex], key);

		dictionary.keys[index] = { true, internalIndex };
		++dictionary.currentSize;
	}
}

fn void DictionaryRemove(Dictionary& dictionary, const StringLit& key)
{
	unsigned int keyHash = hash(key);
	unsigned int index = keyHash % dictionary.size;
	if (dictionary.keys[index].exists)
	{
		unsigned int indexToRemove = dictionary.keys[index].index;
		if (!StringCompare(dictionary.keyStorage[indexToRemove], key))
		{
			while (true)
			{
				if (++index >= dictionary.size)
				{
					index -= dictionary.size;
				}
				if (!dictionary.keys[index].exists || hash(dictionary.keyStorage[dictionary.keys[index].index]) != keyHash)
				{
					return;
				}
				if (StringCompare(key, dictionary.keyStorage[dictionary.keys[index].index]))
				{
					break;
				}
			}
		}
		dictionary.keys[index].exists = false;
		--dictionary.currentSize;
		if (dictionary.keys[index].index != dictionary.currentSize)
		{
			unsigned int keyToMove = hash(dictionary.keyStorage[dictionary.currentSize]) % dictionary.size;
			while (dictionary.keys[keyToMove].index != dictionary.currentSize) { --keyToMove; }
			unsigned int indexToMove = dictionary.keys[keyToMove].index;
			FreeString(dictionary.keyStorage[indexToRemove]);
			dictionary.keyStorage[indexToRemove] = dictionary.keyStorage[indexToMove];
			CopyMemory(&dictionary.data[indexToRemove * dictionary.elementSize], &dictionary.data[indexToMove * dictionary.elementSize], dictionary.elementSize);
			dictionary.keys[keyToMove].index = indexToRemove;
		}
		return;
	}
}

fn void* const DictionaryFind(const Dictionary& dictionary, const StringLit key)
{
	unsigned int keyHash = hash(key);
	unsigned int index = keyHash % dictionary.size;
	if (dictionary.keys[index].exists)
	{
		if (!StringCompare(dictionary.keyStorage[dictionary.keys[index].index], key))
		{
			while (true)
			{

				if (++index >= dictionary.size)
				{
					index -= dictionary.size;
				}
				if (!dictionary.keys[index].exists || hash(dictionary.keyStorage[dictionary.keys[index].index]) != keyHash)
				{
					return nullptr;
				}
				if (StringCompare(key, dictionary.keyStorage[dictionary.keys[index].index]))
				{
					break;
				}
			}
		}
		return &dictionary.data[dictionary.keys[index].index * dictionary.elementSize];
	}
	return nullptr;
}


fn void PrintDictionary(const Dictionary& dictionary)
{
	printf("Printing dictionary with %d elements:\n", dictionary.currentSize);
	for (unsigned int i = 0; i < dictionary.currentSize; ++i)
	{
		printf("{ key: %.*s, value: %d }\n", dictionary.keyStorage[i].length, dictionary.keyStorage[i].text, (int)dictionary.data[i * dictionary.elementSize]);
	}
}

void test()
{
	Dictionary dictionary = CreateDictionary(sizeof(int), 32);
	StringLit keyOne = STR_LIT("One");
	StringLit keyTwo = STR_LIT("Two");
	StringLit keyThree = STR_LIT("Three");
	StringLit keyFour = STR_LIT("Four");
	StringLit keyFive = STR_LIT("Five");
	int one = 1;
	int two = 2;
	int three = 3;
	int four = 4;
	int five = 5;
	DictionaryAdd(dictionary, keyOne, &one);
	DictionaryAdd(dictionary, keyTwo, &two);
	DictionaryAdd(dictionary, keyThree, &three);
	DictionaryAdd(dictionary, keyFour, &four);
	DictionaryAdd(dictionary, keyFive, &five);
	DictionaryRemove(dictionary, keyThree);
	DictionaryRemove(dictionary, keyFive);

	int* value = (int*)DictionaryFind(dictionary, STR_LIT("Two"));
	if (value)
	{
		printf("Found value: %d\n", *value);
	}
	PrintDictionary(dictionary);
}

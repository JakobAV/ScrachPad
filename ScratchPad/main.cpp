//#include "D3D11.cpp"
#include "JsonParser.cpp"
//#include "Dictionary.cpp"
//#include "SortTiming.cpp"
#include "MemoryArena.cpp"
#include "JsonWrapper.cpp"
#include "DebugUtils.cpp"
#include "WorkQueue.cpp"
#include "FileUtilities.h"
#include "StringBuilder.cpp"

WorkQueueCallback(StringBuilderWork)
{
    for (u32 i = 0; i < 16; ++i)
    {
        UseTempMemory(GetScratchArena())
        {
            StringBuilder stringBuilder = CreateStringBuilder();
            StringBuilder* builder = &stringBuilder;
            Append(builder, STR_LIT("Hello"));
            Append(builder, " ", 1);
            Append(builder, STR_LIT("World"));
            Append(builder, STR_LIT("!?"));
            for (u32 j = 0; j < i; ++j)
            {
                Append(builder, builder);
            }
            StringLit str = BuildString(builder, GetScratchArena());

        }
    }

    for (u32 i = 0; i < 16; ++i)
    {
        UseTempMemory(GetScratchArena())
        {
            StringBuilder stringBuilder = CreateStringBuilder();
            StringBuilder* builder = &stringBuilder;
            Prepend(builder, STR_LIT("Hello"));
            Prepend(builder, " ", 1);
            Prepend(builder, STR_LIT("World"));
            Prepend(builder, STR_LIT("!?"));
            for (u32 j = 0; j < i; ++j)
            {
                Prepend(builder, builder);
            }
            StringLit str = BuildString(builder, GetScratchArena());

        }
    }

    UseTempMemory(GetScratchArena())
    {
        StringBuilder stringBuilder = CreateStringBuilder();
        StringBuilder* builder = &stringBuilder;
        Append(builder, STR_LIT("Hello"));
        Append(builder, " ", 1);
        Append(builder, " ", 1);
        Append(builder, STR_LIT("World"));
        Append(builder, STR_LIT("!?"));
        Prepend(builder, STR_LIT("I want to say: "));
        Prepend(builder, STR_LIT("First "));
        Append(builder, STR_LIT(" -END-"));
        Prepend(builder, STR_LIT("-START- "));
        StringBuilderIndex index = IndexOf(builder, STR_LIT("!"));
        StringBuilderIndex endIndex = IndexOf(builder, STR_LIT("D-"));
        StringBuilder subString = SubString(builder, index, endIndex);
        u32 splitCount = 0;
        StringBuilder* split = Split(builder, STR_LIT(" "), &splitCount, GetScratchArena());
        StringLit str = BuildString(builder, GetScratchArena());
        StringLit str2 = BuildString(&subString, GetScratchArena());

    }
}

int main(int argc, char** argv)
{
    WorkQueue queue = {};
    ThreadStartup startUps[16];
    MakeQueue(&queue, ArrayCount(startUps), startUps);
//    for (u32 i = 0; i < 256; ++i)
//    {
//        AddEntry(&queue, StringBuilderWork, (void*)"Hello World!");
//    }
    UseTempMemory(GetScratchArena())
    {
        StringBuilder stringBuilder = CreateStringBuilder();
        StringBuilder* builder = &stringBuilder;
        Append(builder, STR_LIT("Hello"));
        Append(builder, " ", 1);
        Append(builder, " ", 1);
        Append(builder, STR_LIT("World"));
        Append(builder, STR_LIT("!?"));
        Prepend(builder, STR_LIT("I want to say: "));
        Prepend(builder, STR_LIT("First "));
        Append(builder, STR_LIT(" -END-"));
        Prepend(builder, STR_LIT("-START- "));
        StringBuilderIndex index = IndexOf(builder, STR_LIT("!"));
        StringBuilderIndex endIndex = IndexOf(builder, STR_LIT("? -END-"));
        StringBuilder subString = SubString(builder, index, endIndex);
        Insert(builder, index, STR_LIT("hejsanasddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"));
        //Insert(builder, endIndex, builder);

        u32 splitCount = 0;
        StringBuilder* split = Split(builder, STR_LIT(" "), &splitCount, GetScratchArena());

        StringLit str = BuildString(builder, GetScratchArena());
        StringLit str2 = BuildString(&subString, GetScratchArena());
        CompleteAllWork(&queue);
        printf("\n\nFinal:\n\n");
        printf("    print: %s\n", str.text);
        printf("    print: %s\n", str2.text);
        for (u32 i = 0; i < splitCount; ++i)
        {
            StringLit splitStr = BuildString(split + i, GetScratchArena());
            printf("    print: %s\n", splitStr.text);
        }
    }
    ExitProcess(0);
}

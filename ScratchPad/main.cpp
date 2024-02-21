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

int main(int argc, char** argv)
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

            printf("    print: %s\n", str.text);
        }
    }
}

//#include "D3D11.cpp"
#include "JsonParser.cpp"
//#include "Dictionary.cpp"
//#include "SortTiming.cpp"
#include "MemoryArena.cpp"
#include "JsonWrapper.cpp"
#include "FileUtilities.h"
#include <intrin.h>
#include <stdio.h>
int main(int argc, char** argv)
{
    u32 length = 0;
    u8* data = ReadEntireFile("Test.json", length);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER startTime;
    QueryPerformanceCounter(&startTime);
    for (u32 i = 0; i < 200; ++i)
    {
        TestJsonParser(data, length);
    }
    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);

    __int64 t = endTime.QuadPart - startTime.QuadPart;
    double time = t / (frequency.QuadPart * 0.001);
    double timePerRun = time / 200;
    printf("Time: %lfms, TimePerRun: %lfms\n", time, timePerRun);
    JJson::JsonWrapper doc(data, length);
    free(data);
    JJson::Json object = doc.GetRoot();
    JJson::Json fairyArray = object["enemyFairy"];
    JJson::Json fairy = fairyArray[0];
    JJson::ArrayView<f64> numbers = object["Untagged"].GetNumberArray();
}

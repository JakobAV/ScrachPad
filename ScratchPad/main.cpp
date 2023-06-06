//#include "D3D11.cpp"
#include "JsonParser.cpp"
//#include "Dictionary.cpp"
//#include "SortTiming.cpp"
#include "MemoryArena.cpp"
#include "JsonWrapper.cpp"
#include "DebugUtils.cpp"
#include "WorkQueue.cpp"
#include "FileUtilities.h"
#include "nlohmann/json.hpp"
#include <fstream>

u32 SumPosJJsonArray(JJson::Json jsonArray)
{
    u32 result = 0;
    u32 length = jsonArray.GetLength();
    for (u32 i = 0; i < length; ++i)
    {
        JJson::Json obj = jsonArray[i];
        JJson::Json pos = obj["position"];
        result += (u32)pos["x"].GetFloat();
        result += (u32)pos["y"].GetFloat();
        result += (u32)pos["z"].GetFloat();
    }
    return result;
}

u32 SumPosnlohmannArray(nlohmann::json jsonArray)
{
    u32 result = 0;
    for (auto& obj : jsonArray)
    {
        auto pos = obj["position"];
        result += (u32)pos["x"];
        result += (u32)pos["y"];
        result += (u32)pos["z"];
    }
    return result;
}

WorkQueueCallback(LongWork)
{
    Sleep(10000);
    char buffer[456];
    wsprintfA(buffer, "\nThread %u: %s\n", GetCurrentThreadId(), (char*)data);
    OutputDebugStringA(buffer);
}

int main(int argc, char** argv)
{
    const u32 loopCount = 50;
    u32 totalJJson = 0;
    u32 length = 0;
    u8* data = ReadEntireFile("Test.json", length);

    JJson::JsonWrapper doc(data, length);
    free(data);
    for (u32 i = 0; i < loopCount; ++i)
    {
        TIMED_BLOCK(ProcessJJson, (char*)"ProcessJJson");

        JJson::Json object = doc.GetRoot();
        JJson::Json fairyArray = object["enemyFairy"];
        JJson::Json collectibleArray = object["collectible"];
        JJson::Json tilesArray = object["tile"];
        JJson::Json spriteDecorArray = object["spriteDecor"];
        totalJJson += SumPosJJsonArray(fairyArray);
        totalJJson += SumPosJJsonArray(collectibleArray);
        totalJJson += SumPosJJsonArray(tilesArray);
        totalJJson += SumPosJJsonArray(spriteDecorArray);
        JJson::ArrayView<f64> numbers = object["Untagged"].GetNumberArray();
        for(u32 j = 0; j < numbers.size; ++j)
        {
            totalJJson += (u32)numbers.array[j];
        }
    }

    u32 totalnlohmann = 0;
    nlohmann::json json;
    std::ifstream file; 
    file.open("Test.json");
    file >> json;
    for (u32 i = 0; i < 1; ++i)
    {
        TIMED_BLOCK(Processnlohmann, (char*)"Processnlohmann");
        nlohmann::json fairyArray = json["enemyFairy"];
        nlohmann::json collectibleArray = json["collectible"];
        nlohmann::json tilesArray = json["tile"];
        nlohmann::json spriteDecorArray = json["spriteDecor"];
        totalnlohmann += SumPosnlohmannArray(fairyArray);
        totalnlohmann += SumPosnlohmannArray(collectibleArray);
        totalnlohmann += SumPosnlohmannArray(tilesArray);
        totalnlohmann += SumPosnlohmannArray(spriteDecorArray);
        nlohmann::json numbers = json["Untagged"];
        for(auto& num : numbers)
        {
            totalnlohmann += (u32)num;
        }
    }
    printf("JJson: %u\nnlohmann: %u\n", totalJJson, totalnlohmann);
    EndDebugFrame();
}

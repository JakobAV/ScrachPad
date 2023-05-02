//#include "D3D11.cpp"
#include "JsonParser.cpp"
//#include "Dictionary.cpp"
//#include "SortTiming.cpp"
#include "MemoryArena.cpp"
#include "JsonWrapper.cpp"
#include "DebugUtils.cpp"
#include "FileUtilities.h"

int main(int argc, char** argv)
{
    u32 length = 0;
    u8* data = ReadEntireFile("Test.json", length);

    TestJsonParser(data, length);
    JJson::JsonWrapper doc(data, length);
    free(data);
    JJson::Json object = doc.GetRoot();
    JJson::Json fairyArray = object["enemyFairy"];
    JJson::Json fairy = fairyArray[0];
    JJson::ArrayView<f64> numbers = object["Untagged"].GetNumberArray();
}

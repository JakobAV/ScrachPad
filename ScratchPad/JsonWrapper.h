#pragma once
#include "JsonParser.h"
#include "Shared.h"

namespace JJson
{
    class JsonWrapper;
    
    template<class T>
    struct ArrayView
    {
        const T* array = nullptr;
        const size_t size = 0;
    };

    class Json
    {
        friend JsonWrapper;

        enum AccessType
        {
            AccessType_Node,
            AccessType_Array,
            AccessType_Object,
        };

        AccessType myAccessType = AccessType_Node;
        union
        {
            JsonNode* myNode;
            JArray* myArray;
            JObject* myObject;
        };
        u8* GetArrayData(u32* size);
    public:
        Json operator[](const char* name);
        Json operator[](const int index);
        u32 GetLength();
        f64 GetDouble();
        f32 GetFloat();
        s32 GetInt();
        s64 GetLong();
        StringLit GetString();
        bool GetBool();
        bool IsNull();
        JsonNodeType GetType();
        JsonNodeType GetArrayType();

        StringLit GetName();

        ArrayView<bool> GetBoolArray();
        ArrayView<StringLit> GetStringArray();
        ArrayView<f64> GetNumberArray();

        static u8* ReadEntireFile(const char* fileName, u32& bytesRead);
    };

    class JsonWrapper
    {
        JsonDocument myDocument = {};
    public:
        JsonWrapper();
        ~JsonWrapper();
        JsonWrapper(u8* data, u32 length);
        JsonWrapper(MemoryArena* arena, u8* data, u32 length);
        void Init(u8* data, u32 length);
        void Init(MemoryArena* arena, u8* data, u32 length);
        Json GetRoot();
    };

};

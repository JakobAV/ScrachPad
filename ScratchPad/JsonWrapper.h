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
            JObject* myObjct;
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
        JsonNodeType GetType();
        JsonNodeType GetArrayType();

        ArrayView<bool> GetBoolArray();
        ArrayView<StringLit> GetStringArray();
        ArrayView<f64> GetNumberArray();
    };

    class JsonWrapper
    {
        JsonDocument myDocument;
    public:
        JsonWrapper(u8* data, u32 length);
        ~JsonWrapper();
        Json GetRoot();
    };

};

#include "JsonWrapper.h"
#include <stdio.h>
#include <stdlib.h>

namespace JJson
{
    JsonWrapper::JsonWrapper()
    {
    }

    JsonWrapper::~JsonWrapper()
    {
        if(myDocument.arena)
        {
            FreeJsonDocument(myDocument);
        }
    }

    JsonWrapper::JsonWrapper(u8* data, u32 length)
    {
        Init(data, length);
    }

     JsonWrapper::JsonWrapper(MemoryArena* arena, u8* data, u32 length)
     {
         Init(arena, data, length);
     }

    void JsonWrapper::Init(u8* data, u32 length)
    {
        if(myDocument.arena)
        {
            ArenaClear(myDocument.arena);
        }
        myDocument = CreateManagedJsonDocument(data, length);
    }

    void JsonWrapper::Init(MemoryArena* arena, u8* data, u32 length)
    {
        if(myDocument.arena)
        {
            // TODO: Should it be allowed to init an unmanaged JsonDocument over a managed one?
            FreeJsonDocument(myDocument);
        }
        myDocument = CreateJsonDocument(arena, data, length);
    }

    Json JsonWrapper::GetRoot()
    {
        Json result;
        result.myNode = myDocument.root;
        return result;
    }

    Json Json::operator[](const char* name)
    {
        JObject* object;
        if (myAccessType == AccessType_Object)
        {
            object = myObject;
        }
        else
        {
            assert(myNode->type == JsonNodeType_Object);
            object = &myNode->object;
        }
        JsonNode* node = JsonGetNode(object, name);
        Json result;
        result.myNode = node;
        return result;
    }

    Json Json::operator[](const int index)
    {
        JObject* object = nullptr;
        if (myAccessType == AccessType_Object)
        {
            object = myObject;

        }
        else if (myAccessType == AccessType_Node && myNode->type == JsonNodeType_Object)
        {
            object = &myNode->object;

        }
        if (object)
        {
            JsonNode* node = JsonGetNode(object, index);
            Json result;
            result.myNode = node;
            return result;
        }

        JArray* array;
        if (myAccessType == AccessType_Array)
        {
            array = myArray;
        }
        else
        {
            assert(myNode->type == JsonNodeType_Array);
            array = &myNode->array;
        }
        assert((u32)index < array->length);

        Json result = {};
        switch (array->arrayType)
        {
            case JsonNodeType_Array:
            {
                JArray* arr = reinterpret_cast<JArray*>(array->values);
                result.myAccessType = AccessType_Array;
                result.myArray = arr + index;
                break;
            }
            case JsonNodeType_Object:
            {
                JObject* arr = reinterpret_cast<JObject*>(array->values);
                result.myAccessType = AccessType_Object;
                result.myObject = arr + index;
                break;
            }
            default:
                InvalidCodePath;
                break;
        }
        return result;
    }

    u32 Json::GetLength()
    {
        u32 result = 0;
        switch (myAccessType)
        {
            case AccessType_Node:
            {
                switch (myNode->type)
                {
                    case JsonNodeType_Array:
                        result = myNode->array.length;
                        break;
                    case JsonNodeType_Object:
                        result = myNode->object.length;
                        break;
                    default:
                        InvalidCodePath;
                        break;
                }
                break;
            }
            case AccessType_Array:
                result = myArray->length;
                break;
            case AccessType_Object:
                result = myObject->length;
                break;
            default:
                InvalidCodePath;
                break;
        }
        return result;
    }

    f64 Json::GetDouble()
    {
        return JsonGetDouble(myNode);
    }
    
    f32 Json::GetFloat()
    {
        return JsonGetFloat(myNode);
    }
    
    s32 Json::GetInt()
    {
        return JsonGetInt(myNode);
    }
    
    s64 Json::GetLong()
    {
        return JsonGetLong(myNode);
    }
    
    StringLit Json::GetString()
    {
        return JsonGetString(myNode);
    }
    
    bool Json::GetBool()
    {
        return JsonGetBool(myNode);
    }

    bool Json::IsNull()
    {
        return JsonIsNull(myNode);
    }

    JsonNodeType Json::GetType()
    {
        return JsonGetType(myNode);
    }

    JsonNodeType Json::GetArrayType()
    {
        return JsonGetArrayType(myNode);
    }

    u8* Json::GetArrayData(u32* size)
    {
        JArray* array = nullptr;
        if (myAccessType == AccessType_Array)
        {
            array = myArray;
        }
        else
        {
            array = &myNode->array;
        }
        switch (array->arrayType)
        {
            case JsonNodeType_String: break;
            case JsonNodeType_Number: break;
            case JsonNodeType_Boolean: break;
            default:
                InvalidCodePath;
                break;
        }
        *size = array->length;
        return array->values;
    }

    StringLit Json::GetName()
    {
        assert(myAccessType == AccessType_Node);
        return myNode->name;
    }


    ArrayView<bool> Json::GetBoolArray()
    {
        u32 size = 0;
        u8* data = GetArrayData(&size);
        ArrayView<bool> result = { reinterpret_cast<bool*>(data), size };
        return result;
    }
    
    ArrayView<f64> Json::GetNumberArray()
    {
        
        u32 size = 0;
        u8* data = GetArrayData(&size);
        ArrayView<f64> result = { reinterpret_cast<f64*>(data), size };
        return result;
    }
    
    ArrayView<StringLit> Json::GetStringArray()
    {
        
        u32 size = 0;
        u8* data = GetArrayData(&size);
        ArrayView<StringLit> result = { reinterpret_cast<StringLit*>(data), size };
        return result;
    }

    u8* Json::ReadEntireFile(const char* fileName, u32& bytesRead)
    {
        FILE* fileHandle;
        fopen_s(&fileHandle, fileName, "r");
        if (fileHandle == nullptr)
        {
            return nullptr;
        }
        if (fseek(fileHandle, 0, SEEK_END) != 0)
        {
            fclose(fileHandle);
            return nullptr;
        }
        int fileSize = ftell(fileHandle);
        if (fileSize == 0)
        {
            fclose(fileHandle);
            return nullptr;
        }
        fseek(fileHandle, 0, SEEK_SET);
        u8* buffer = (u8*)malloc(fileSize);
        if (buffer == 0)
        {
            fclose(fileHandle);
            return nullptr;
        }
        bytesRead = (u32)fread_s(buffer, fileSize, sizeof(char), fileSize, fileHandle);
        fclose(fileHandle);
        return buffer;
    }
};
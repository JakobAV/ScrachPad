#include "JsonWrapper.h"

namespace JJson
{
    JsonWrapper::JsonWrapper(u8* data, u32 length)
    {
        myDocument = CreateJsonDocument(data, length);
    }
    
    JsonWrapper::~JsonWrapper()
    {
        FreeJsonDocument(myDocument);
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
            object = myObjct;
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
                result.myObjct = arr + index;
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
                result = myObjct->length;
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
};
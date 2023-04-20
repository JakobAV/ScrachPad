#pragma once
#include "MemoryArena.h"
#include "Shared.h"

enum JsonNodeType
{
    JsonNodeType_Null,
    JsonNodeType_Object,
    JsonNodeType_Array,
    JsonNodeType_String,
    JsonNodeType_Number,
    JsonNodeType_Boolean,
};

struct JsonNode;

struct JObject
{
    // TODO: Need to give each value a key
    u32 length;
    JsonNode* values;
};

struct JArray
{
    u32 length;
    JsonNodeType arrayType;
    u8* values;
};

struct JsonNode
{
    JsonNodeType type;
    StringLit name;
    union
    {
        bool booleanValue;
        StringLit string;
        JObject object;
        f64 number;
        JArray array;
    };
};

struct JsonDocument
{
    MemoryArena* arena;
    JsonNode* root;
};

JsonDocument CreateJsonDocument(u8* fileData, u32 length);
void FreeJsonDocument(JsonDocument doc);

JsonNode* JsonGetNode(JObject* obj, const char* name);
JObject* JsonGetObject(JsonNode* node);
f64 JsonGetDouble(JsonNode* node);
f32 JsonGetFloat(JsonNode* node);
s32 JsonGetInt(JsonNode* node);
s64 JsonGetLong(JsonNode* node);
StringLit JsonGetString(JsonNode* node);
bool JsonGetBool(JsonNode* node);
JsonNodeType JsonGetType(JsonNode* node);
JsonNodeType JsonGetArrayType(JsonNode* node);
bool* JsonGetBoolArray(JsonNode* node, u32* size);
f64* JsonGetNumberArray(JsonNode* node, u32* size);
StringLit* JsonGetStringArray(JsonNode* node, u32* size);
JObject* JsonGetObjectArray(JsonNode* node, u32* size);
JArray* JsonGetArrayArray(JsonNode* node, u32* size);

bool JsonIsNull(JsonNode* node);
void JsonSerialize(JsonDocument doc);
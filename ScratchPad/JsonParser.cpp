#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "JsonParser.h"
#include "Shared.h"

struct Tokenizer
{
    u32 length;
    u8* data;
    u8* at;
};

enum JsonTokenType
{
    JsonTokenType_OpenBrace,
    JsonTokenType_CloseBrace,
    JsonTokenType_OpenBracket,
    JsonTokenType_CloseBracket,
    JsonTokenType_Comma,
    JsonTokenType_Colon,
    JsonTokenType_String,
    JsonTokenType_Number,
    JsonTokenType_True,
    JsonTokenType_False,
    JsonTokenType_Null,
    JsonTokenType_EndOfFile,
};

struct Token
{
    JsonTokenType type;
    u8* data;
    u32 length;
};

JObject ParseJObject(Tokenizer* tokenizer, MemoryArena* arena);
JArray ParseJArray(Tokenizer* tokenizer, MemoryArena* arena);
StringLit ParseString(Token token, Tokenizer* tokenizer, MemoryArena* arena);
f64 ParseNumber(Token token, Tokenizer* tokenizer, MemoryArena* arena);
JsonNode ParseJsonNode(Token stringToken, Tokenizer* tokenizer, MemoryArena* arena);

bool IsNumber(u8 c)
{
    if (c >= '0' && c <= '9')
    {
        return true;
    }
    return false;
}

void EatWhiteSpace(Tokenizer& tokenizer)
{
    while (true)
    {
        u8 c = *tokenizer.at;
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
        {
            return;
        }
        ++tokenizer.at;
    }
}

Token GetToken(Tokenizer* tokenizerPtr)
{
    Tokenizer& tokenizer = *tokenizerPtr;
    EatWhiteSpace(tokenizer);
    u8 c = *tokenizer.at;
    Token token = {};
    token.data = tokenizer.at;
    token.length = 1;
    ++tokenizer.at;
    switch (c)
    {
        case '{': { token.type = JsonTokenType_OpenBrace; } break;
        case '}': { token.type = JsonTokenType_CloseBrace; } break;
        case '[': { token.type = JsonTokenType_OpenBracket; } break;
        case ']': { token.type = JsonTokenType_CloseBracket; } break;
        case ':': { token.type = JsonTokenType_Colon; } break;
        case ',': { token.type = JsonTokenType_Comma; } break;
        case EOF: { token.type = JsonTokenType_EndOfFile; } break;
        case 't':
        {
            if (tokenizer.at[0] == 'r' && tokenizer.at[1] == 'u' && tokenizer.at[2] == 'e')
            {
                token.type = JsonTokenType_True;
                tokenizer.at += 3;
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        case 'f':
        {
            if (tokenizer.at[0] == 'a' && tokenizer.at[1] == 'l' && tokenizer.at[2] == 's' && tokenizer.at[3] == 'e')
            {
                token.type = JsonTokenType_False;
                tokenizer.at += 4;
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        case 'n':
        {
            if (tokenizer.at[0] == 'u' && tokenizer.at[1] == 'l' && tokenizer.at[2] == 'l')
            {
                token.type = JsonTokenType_Null;
                tokenizer.at += 3;
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        case '"':
        {
            token.type = JsonTokenType_String;
            token.data = tokenizer.at;

            u32 length = 0;
            while (true)
            {
                c = *tokenizer.at;
                if (c == '"' && *(tokenizer.at - 1) != '\\')
                {
                    ++tokenizer.at;
                    break;
                }
                ++length;
                ++tokenizer.at;
            }

            token.length = length;
        } break;
        default:
        {
            if (c == '-' || IsNumber(c))
            {
                token.type = JsonTokenType_Number;
                c = *tokenizer.at;
                while (IsNumber(c) || c == '.')
                {
                    ++token.length;
                    ++tokenizer.at;
                    c = *tokenizer.at;
                }
            }
            else
            {
                InvalidCodePath;
            }
        } break;
    }

    return token;
}

Token PeekToken(Tokenizer* tokenizer)
{
    u8* at = tokenizer->at;
    Token token = GetToken(tokenizer);
    tokenizer->at = at;
    return token;
}

bool RequireToken(Tokenizer* tokenizer, JsonTokenType type)
{
    Token token = GetToken(tokenizer);
    return token.type == type;
}

StringLit ParseString(Token token, Tokenizer* tokenizer, MemoryArena* arena)
{
    tokenizer;
    StringLit result = {};
    result.length = token.length;
    result.text = PushArrayAligned(arena, char, token.length + 1, 1);
    memcpy_s((u8*)result.text, token.length + 1, token.data, token.length);
    ((u8*)result.text)[token.length] = '\0';
    return result;
}

inline u8 CharToNumber(u8 c)
{
    return c - '0';
}

f64 ParseNumber(Token token, Tokenizer* tokenizer, MemoryArena* arena)
{
    tokenizer;
    arena;
    f64 number;
#if 1
    assert(token.type == JsonTokenType_Number);
    bool isNegative = token.data[0] == '-';
    u64 baseNumber = 0;
    f64 fraction = 0;
    u32 fractionNumber = 0;
    for (s32 i = isNegative ? 1:0; i < (s32)token.length; ++i)
    {
        if (fractionNumber > 0)
        {
            fraction *= 10;
            fraction += CharToNumber(token.data[i]);
            ++fractionNumber;
        }
        else if (token.data[i] == '.' && i > (isNegative ?1:0))
        {
            fractionNumber = 1;
        }
        else
        {
            baseNumber *= 10;
            baseNumber += CharToNumber(token.data[i]);
        }
    }
    fraction *= pow(0.1, fractionNumber-1);
    number = baseNumber + fraction;
    if(isNegative)
    {
        number = -number;
    }
#else
    // Slower, but probably more correct
    number = (f64)strtod((const char*)token.data, 0);
#endif
    return number;
}

JsonNode ParseJsonNode(Token stringToken, Tokenizer* tokenizer, MemoryArena* arena)
{
    assert(stringToken.type == JsonTokenType_String);

    JsonNode node = {};
    node.name = ParseString(stringToken, tokenizer, arena);
    if (!RequireToken(tokenizer, JsonTokenType_Colon))
    {
        InvalidCodePath;
        return node;
    }
    Token token = GetToken(tokenizer);
    switch (token.type)
    {
        case JsonTokenType_String:
            node.type = JsonNodeType_String;
            node.string = ParseString(token, tokenizer, arena);
            break;
        case JsonTokenType_Number:
            node.type = JsonNodeType_Number;
            node.number = ParseNumber(token, tokenizer, arena);
            break;
        case JsonTokenType_OpenBrace:
            node.type = JsonNodeType_Object;
            node.object = ParseJObject(tokenizer, arena);
            break;
        case JsonTokenType_OpenBracket:
            node.type = JsonNodeType_Array;
            node.array = ParseJArray(tokenizer, arena);
            break;
        case JsonTokenType_False:
        case JsonTokenType_True:
            node.type = JsonNodeType_Boolean;
            node.booleanValue = token.type == JsonTokenType_True;
            break;
        case JsonTokenType_Null:
            node.type = JsonNodeType_Null;
            break;
        default:
            InvalidCodePath;
            break;
    }
    return node;
}

JObject ParseJObject(Tokenizer* tokenizer, MemoryArena* arena)
{
    JObject obj = {};
    Token peek = PeekToken(tokenizer);
    if(peek.type == JsonTokenType_CloseBrace)
    {
        // Empty object
        GetToken(tokenizer);
        obj.length = 0;
        obj.values = nullptr;
        return obj;
    }
    MemoryArena* tempArena = GetScratchArena();
    TempMemory block = BeginTempMemory(tempArena);
    ArenaSetAlignment(tempArena, sizeof(JsonNode));
    u8* startPos = ArenaGetCurrentPos(tempArena);
    u32 numberOfNodes = 0;
    bool endOfObject = false;
    while (endOfObject == false)
    {
        Token token = GetToken(tokenizer);
        switch (token.type)
        {
            case JsonTokenType_CloseBrace:
                endOfObject = true;
                break;
            case JsonTokenType_Comma:
                break;
            case JsonTokenType_String:
            {
                JsonNode* node = PushType(tempArena, JsonNode);
                *node = ParseJsonNode(token, tokenizer, arena);
                numberOfNodes += 1;
                break;
            }
            default:
                assert(false);
                endOfObject = true;
                break;
        }
    }
    obj.length = numberOfNodes;
    ArenaSetAlignment(arena, sizeof(JsonNode));
    obj.values = PushArray(arena, JsonNode, numberOfNodes);
    memcpy_s(obj.values, numberOfNodes * sizeof(JsonNode), startPos, numberOfNodes * sizeof(JsonNode));
    EndTempMemory(&block);
    return obj;
}

JArray ParseJArray(Tokenizer* tokenizer, MemoryArena* arena)
{
    JArray arr = {};
    Token peek = PeekToken(tokenizer);
    u32 elementSize = 0;
    u32 numberOfElements = 0;
    arr.arrayType = JsonNodeType_Null;
    switch (peek.type)
    {
        case JsonTokenType_String:
            arr.arrayType = JsonNodeType_String;
            elementSize = sizeof(StringLit);
            break;
        case JsonTokenType_Number:
            arr.arrayType = JsonNodeType_Number;
            elementSize = sizeof(f64);
            break;
        case JsonTokenType_OpenBrace:
            arr.arrayType = JsonNodeType_Object;
            elementSize = sizeof(JObject);
            break;
        case JsonTokenType_OpenBracket:
            arr.arrayType = JsonNodeType_Array;
            elementSize = sizeof(JArray);
            break;
        case JsonTokenType_True:
        case JsonTokenType_False:
            arr.arrayType = JsonNodeType_Boolean;
            elementSize = sizeof(bool);
            break;
        case JsonTokenType_CloseBracket:
            // Empty array
            GetToken(tokenizer);
            arr.length = 0;
            arr.values = nullptr;
            return arr;
        default:
            InvalidCodePath;
            break;
    }

    MemoryArena* tempArena = GetScratchArena();
    TempMemory block = BeginTempMemory(tempArena);
    ArenaSetAlignment(tempArena, elementSize);
    u8* startPos = ArenaGetCurrentPos(tempArena);
    bool endOfObject = false;
    while (endOfObject == false)
    {
        Token token = GetToken(tokenizer);
        switch (token.type)
        {
            case JsonTokenType_CloseBracket:
                endOfObject = true;
                break;
            case JsonTokenType_Comma:
                break;
            case JsonTokenType_String:
            {
                assert(arr.arrayType == JsonNodeType_String);
                StringLit* str = PushType(tempArena, StringLit);
                *str = ParseString(token, tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case JsonTokenType_Number:
            {
                assert(arr.arrayType == JsonNodeType_Number);
                f64* node = PushType(tempArena, f64);
                *node = ParseNumber(token, tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case JsonTokenType_OpenBrace:
            {
                assert(arr.arrayType == JsonNodeType_Object);
                JObject* node = PushType(tempArena, JObject);
                *node = ParseJObject(tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case JsonTokenType_OpenBracket:
            {
                assert(arr.arrayType == JsonNodeType_Array);
                JArray* node = PushType(tempArena, JArray);
                *node = ParseJArray(tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case JsonTokenType_False:
            case JsonTokenType_True:
            {
                assert(arr.arrayType == JsonNodeType_Boolean);
                bool* node = PushType(tempArena, bool);
                *node = token.type == JsonTokenType_True ? true : false;
                numberOfElements += 1;
                break;
            }
            default:
                InvalidCodePath;
                endOfObject = true;
                break;
        }
    }
    u32 dataToCopy = numberOfElements * elementSize;
    arr.length = numberOfElements;
    ArenaSetAlignment(arena, elementSize);
    arr.values = PushArray(arena, u8, dataToCopy);
    memcpy_s(arr.values, dataToCopy, startPos, dataToCopy);
    EndTempMemory(&block);
    return arr;
}

JsonDocument CreateJsonDocument(MemoryArena* arena, u8* fileData, u32 length)
{
    Tokenizer tokenizer = {};
    tokenizer.data = fileData;
    tokenizer.at = fileData;
    tokenizer.length = length;

    JsonDocument doc = {};
    if (!RequireToken(&tokenizer, JsonTokenType_OpenBrace))
    {
        InvalidCodePath;
        return doc;
    }
    doc.root = PushType(arena, JsonNode);
    doc.root->type = JsonNodeType_Object;
    doc.root->name = { 0, nullptr };
    doc.root->object = ParseJObject(&tokenizer, arena);

    return doc;
}

JsonDocument CreateManagedJsonDocument(u8* fileData, u32 length)
{
    Tokenizer tokenizer = {};
    tokenizer.data = fileData;
    tokenizer.at = fileData;
    tokenizer.length = length;

    JsonDocument doc = {};
    if (!RequireToken(&tokenizer, JsonTokenType_OpenBrace))
    {
        InvalidCodePath;
        return doc;
    }
    doc.arena = CreateArena();
    doc.root = PushType(doc.arena, JsonNode);
    doc.root->type = JsonNodeType_Object;
    doc.root->name = { 0, nullptr };
    doc.root->object = ParseJObject(&tokenizer, doc.arena);

    return doc;
}

void FreeJsonDocument(JsonDocument doc)
{
    if(doc.arena)
    {
        FreeArena(doc.arena);
    }
    else
    {
        // Tried to free an unmanaged JsonDocument
        InvalidCodePath;
    }
}

JsonNode* JsonGetNode(JObject* obj, const char* name)
{
    for (u32 i = 0; i < obj->length; ++i)
    {
        if (strncmp(obj->values[i].name.text, name, obj->values[i].name.length) == 0)
        {
            return obj->values + i;
        }
    }
    return nullptr;
}

JsonNode* JsonGetNode(JObject* obj, u32 index)
{
    assert(index < obj->length);
    return obj->values + index;
}

JObject* JsonGetObject(JsonNode* node)
{
    if (node->type == JsonNodeType_Object)
    {
        return &node->object;
    }
    return nullptr;
}

f64 JsonGetDouble(JsonNode* node)
{
    if (node->type == JsonNodeType_Number)
    {
        return node->number;
    }
    return (f64)0xFFFFFFFFFFFFFFFF;
}

f32 JsonGetFloat(JsonNode* node)
{
    return static_cast<f32>(JsonGetDouble(node));
}

s32 JsonGetInt(JsonNode* node)
{
    return static_cast<s32>(JsonGetDouble(node));
}

s64 JsonGetLong(JsonNode* node)
{
    return static_cast<s64>(JsonGetDouble(node));
}

StringLit JsonGetString(JsonNode* node)
{
    if (node->type == JsonNodeType_String)
    {
        return node->string;
    }
    return StringLit { 0, nullptr };
}

bool JsonGetBool(JsonNode* node)
{
    if (node->type == JsonNodeType_Boolean)
    {
        return node->booleanValue;
    }
    return false;
}

JsonNodeType JsonGetType(JsonNode* node)
{
    return node->type;
}

JsonNodeType JsonGetArrayType(JsonNode* node)
{
    if (node->type == JsonNodeType_Array)
    {
        return node->array.arrayType;
    }
    return JsonNodeType_Null;
}

bool* JsonGetBoolArray(JsonNode* node, u32* size)
{
    if (node->type == JsonNodeType_Array && node->array.arrayType == JsonNodeType_Boolean)
    {
        *size = node->array.length;
        return reinterpret_cast<bool*>(node->array.values);
    }
    *size = 0;
    return nullptr;
}

f64* JsonGetNumberArray(JsonNode* node, u32* size)
{
    if (node->type == JsonNodeType_Array && node->array.arrayType == JsonNodeType_Number)
    {
        *size = node->array.length;
        return reinterpret_cast<f64*>(node->array.values);
    }
    *size = 0;
    return nullptr;
}

StringLit* JsonGetStringArray(JsonNode* node, u32* size)
{
    if (node->type == JsonNodeType_Array && node->array.arrayType == JsonNodeType_String)
    {
        *size = node->array.length;
        return reinterpret_cast<StringLit*>(node->array.values);
    }
    *size = 0;
    return nullptr;
}

JObject* JsonGetObjectArray(JsonNode* node, u32* size)
{
    if (node->type == JsonNodeType_Array && node->array.arrayType == JsonNodeType_Object)
    {
        *size = node->array.length;
        return reinterpret_cast<JObject*>(node->array.values);
    }
    *size = 0;
    return nullptr;
}

JArray* JsonGetArrayArray(JsonNode* node, u32* size)
{
    if (node->type == JsonNodeType_Array && node->array.arrayType == JsonNodeType_Array)
    {
        *size = node->array.length;
        return reinterpret_cast<JArray*>(node->array.values);
    }
    *size = 0;
    return nullptr;
}


bool JsonIsNull(JsonNode* node)
{
    return node == nullptr || node->type == JsonNodeType_Null;
}
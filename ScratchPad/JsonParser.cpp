#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "MemoryArena.h"
#include "Shared.h"

struct Tokenizer
{
    u32 length;
    u8* data;
    u8* at;
};

enum TokenType
{
    TokenType_OpenBrace,
    TokenType_CloseBrace,
    TokenType_OpenBracket,
    TokenType_CloseBracket,
    TokenType_Comma,
    TokenType_Colon,
    TokenType_String,
    TokenType_Number,
    TokenType_True,
    TokenType_False,
    TokenType_Null,
    TokenType_EndOfFile,
};

struct Token
{
    TokenType type;
    u8* data;
    u32 length;
};

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
    JObject root;
};

JObject ParseJObject(Tokenizer* tokenizer, MemoryArena* arena);
JArray ParseJArray(Tokenizer* tokenizer, MemoryArena* arena);
StringLit ParseString(Token token, Tokenizer* tokenizer, MemoryArena* arena);
f64 ParseNumber(Token token, Tokenizer* tokenizer, MemoryArena* arena);
JsonNode ParseJsonNode(Token stringToken, Tokenizer* tokenizer, MemoryArena* arena);

MemoryArena* GetTempArena()
{
    static MemoryArena* tempArena = CreateArena();
    return tempArena;
}

u8* ReadEntireFile(const char* fileName, u32& bytesRead)
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
        case '{': { token.type = TokenType_OpenBrace; } break;
        case '}': { token.type = TokenType_CloseBrace; } break;
        case '[': { token.type = TokenType_OpenBracket; } break;
        case ']': { token.type = TokenType_CloseBracket; } break;
        case ':': { token.type = TokenType_Colon; } break;
        case ',': { token.type = TokenType_Comma; } break;
        case EOF: { token.type = TokenType_EndOfFile; } break;
        case 't':
        {
            if (tokenizer.at[0] == 'r' && tokenizer.at[1] == 'u' && tokenizer.at[2] == 'e')
            {
                token.type = TokenType_True;
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
                token.type = TokenType_False;
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
                token.type = TokenType_Null;
            }
            else
            {
                InvalidCodePath;
            }
        } break;
        case '"':
        {
            token.type = TokenType_String;
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
                assert(false);
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

bool RequireToken(Tokenizer* tokenizer, TokenType type)
{
    Token token = GetToken(tokenizer);
    return token.type == type;
}

StringLit ParseString(Token token, Tokenizer* tokenizer, MemoryArena* arena)
{
    StringLit result = {};
    result.length = token.length;
    result.str = PushArray(arena, u8, token.length + 1);
    memcpy_s(result.str, token.length + 1, token.data, token.length);
    result.str[token.length] = '\0';
    return result;
}

inline u8 CharToNumber(u8 c)
{
    return c - '0';
}

f64 ParseNumber(Token token, Tokenizer* tokenizer, MemoryArena* arena)
{
    f64 number;
    assert(token.type == TokenType_Number);
    bool isNegative = token.data[0] == '-';
    u64 baseNumber = 0;
    f64 fraction = 0;
    u32 fractionNumber = 0;
    for (s32 i = isNegative ? 1:0; i < (s32)token.length; ++i)
    {
        if (fractionNumber > 0)
        {
            fraction += CharToNumber(token.data[i]) * pow(0.1, fractionNumber);
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
    number = baseNumber + fraction;
    return number;
}

JsonNode ParseJsonNode(Token stringToken, Tokenizer* tokenizer, MemoryArena* arena)
{
    assert(stringToken.type == TokenType_String);

    JsonNode node = {};
    node.name = ParseString(stringToken, tokenizer, arena);
    if (!RequireToken(tokenizer, TokenType_Colon))
    {
        InvalidCodePath;
        return node;
    }
    Token token = GetToken(tokenizer);
    switch (token.type)
    {
        case TokenType_String:
            node.type = JsonNodeType_String;
            node.string = ParseString(token, tokenizer, arena);
            break;
        case TokenType_Number:
            node.type = JsonNodeType_Number;
            node.number = ParseNumber(token, tokenizer, arena);
            break;
        case TokenType_OpenBrace:
            node.type = JsonNodeType_Object;
            node.object = ParseJObject(tokenizer, arena);
            break;
        case TokenType_OpenBracket:
            node.type = JsonNodeType_Array;
            node.object = ParseJObject(tokenizer, arena);
            break;
        case TokenType_False:
        case TokenType_True:
            node.type = JsonNodeType_Boolean;
            node.booleanValue = token.type == TokenType_True ? true : false;
            break;
        case TokenType_Null:
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

    MemoryArena* tempArena = GetTempArena();
    u8* startPos = ArenaGetCurrentPos(tempArena);
    TempMemory block = BeginTempMemory(tempArena);
    u32 numberOfNodes = 0;
    bool endOfObject = false;
    while (endOfObject == false)
    {
        Token token = GetToken(tokenizer);
        switch (token.type)
        {
            case TokenType_CloseBracket:
                endOfObject = true;
                break;
            case TokenType_Comma:
                break;
            case TokenType_String:
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
    obj.values = PushArray(arena, JsonNode, numberOfNodes);
    memcpy_s(obj.values, numberOfNodes * sizeof(JsonNode), startPos, numberOfNodes * sizeof(JsonNode));
    EndTempMemory(block);
    return obj;
}

JArray ParseJArray(Tokenizer* tokenizer, MemoryArena* arena)
{
    JArray arr = {};
    MemoryArena* tempArena = GetTempArena();
    u8* startPos = ArenaGetCurrentPos(tempArena);
    TempMemory block = BeginTempMemory(tempArena);
    u32 numberOfElements = 0;
    u32 elementSize = 0;
    bool endOfObject = false;
    Token peek = PeekToken(tokenizer);
    arr.arrayType = JsonNodeType_Null;
    switch (peek.type)
    {
        case TokenType_String:
            arr.arrayType = JsonNodeType_String;
            elementSize = sizeof(StringLit);
            break;
        case TokenType_Number:
            arr.arrayType = JsonNodeType_Number;
            elementSize = sizeof(f64);
            break;
        case TokenType_OpenBrace:
            arr.arrayType = JsonNodeType_Object;
            elementSize = sizeof(JObject);
            break;
        case TokenType_OpenBracket:
            arr.arrayType = JsonNodeType_Array;
            elementSize = sizeof(JArray);
            break;
        case TokenType_True:
        case TokenType_False:
            arr.arrayType = JsonNodeType_Boolean;
            elementSize = sizeof(bool);
            break;
        default:
            InvalidCodePath;
            break;
    }
    while (endOfObject == false)
    {
        Token token = GetToken(tokenizer);
        switch (token.type)
        {
            case TokenType_CloseBracket:
                endOfObject = true;
                break;
            case TokenType_Comma:
                break;
            case TokenType_String:
            {
                assert(arr.arrayType == TokenType_String);
                StringLit* str = PushType(tempArena, StringLit);
                *str = ParseString(token, tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case TokenType_Number:
            {
                assert(arr.arrayType == TokenType_Number);
                f64* node = PushType(tempArena, f64);
                *node = ParseNumber(token, tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case TokenType_OpenBrace:
            {
                assert(arr.arrayType == JsonNodeType_Object);
                JObject* node = PushType(tempArena, JObject);
                *node = ParseJObject(tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case TokenType_OpenBracket:
            {
                assert(arr.arrayType == JsonNodeType_Array);
                JArray* node = PushType(tempArena, JArray);
                *node = ParseJArray(tokenizer, arena);
                numberOfElements += 1;
                break;
            }
            case TokenType_False:
            case TokenType_True:
            {
                assert(arr.arrayType == JsonNodeType_Boolean);
                bool* node = PushType(tempArena, bool);
                *node = token.type == TokenType_True ? true : false;
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
    arr.values = PushArray(arena, u8, dataToCopy);
    memcpy_s(arr.values, dataToCopy, startPos, dataToCopy);
    EndTempMemory(block);
    return arr;
}

JsonDocument CreateJsonDocument(u8* fileData, u32 length)
{
    Tokenizer tokenizer = {};
    tokenizer.data = fileData;
    tokenizer.length = length;

    JsonDocument doc = {};
    if (!RequireToken(&tokenizer, TokenType_OpenBracket))
    {
        InvalidCodePath;
        return doc;
    }
    doc.arena = CreateArena();
    doc.root = ParseJObject(&tokenizer, doc.arena);

    return doc;
}

void FreeJsonDocument(JsonDocument doc)
{
    FreeArena(doc.arena);
}

void TestJsonParser()
{
    u32 length = 0;
    u8* data = ReadEntireFile("C:\\Users\\jakob.persson\\OneDrive - Academedia\\Finished\\CPP-DeepDive\\Uppgift-03-DataDriveGame\\exe\\settings\\GameSettings.json", length);

    JsonDocument doc = CreateJsonDocument(data, length);

    FreeJsonDocument(doc);
}
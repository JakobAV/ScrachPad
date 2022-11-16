#include <stdio.h>
#include <stdlib.h>

struct Tokenizer
{
	int length;
	char* data;
	int at;
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
	TokenType_Token,
	TokenType_Number,
	TokenType_True,
	TokenType_False,
	TokenType_Null,
	TokenType_EndOfFile,
};

struct Token
{
	TokenType type;
	char* data;
	int length;
};

char* ReadEntireFile(const char* fileName, int& bytesRead)
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
	char* buffer = (char*)malloc(fileSize);
	if (buffer == 0)
	{
		fclose(fileHandle);
		return nullptr;
	}
	bytesRead = fread_s(buffer, fileSize, sizeof(char), fileSize, fileHandle);
	fclose(fileHandle);
	return buffer;
}

bool IsNumber(char c)
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
		char c = tokenizer.data[tokenizer.at];
		if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
		{
			return;
		}
		++tokenizer.at;
	}
}

Token GetToken(Tokenizer& tokenizer)
{
	EatWhiteSpace(tokenizer);
	char c = tokenizer.data[tokenizer.at];
	Token token = {};
	token.data = tokenizer.data + tokenizer.at;
	token.length = 1;
	++tokenizer.at;
	switch (c)
	{
	case '{': {token.type = TokenType_OpenBrace; } break;
	case '}': {token.type = TokenType_CloseBrace; } break;
	case '[': {token.type = TokenType_OpenBracket; } break;
	case ']': {token.type = TokenType_CloseBracket; } break;
	case ':': {token.type = TokenType_Colon; } break;
	case ',': {token.type = TokenType_Comma; } break;
	case EOF: {token.type = TokenType_EndOfFile; } break;
	case '"':
	{
		token.type = TokenType_String;
		token.data = tokenizer.data + tokenizer.at;

		int length = 0;
		while (true)
		{
			c = tokenizer.data[tokenizer.at];
			if (c == '"' && tokenizer.data[tokenizer.at - 1] != '\\')
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
			c = tokenizer.data[tokenizer.at];
			while (IsNumber(c) || c == '.')
			{
				++token.length;
				++tokenizer.at;
				c = tokenizer.data[tokenizer.at];
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

void TestJsonParser()
{
	Tokenizer tokenizer = {};
	tokenizer.data = ReadEntireFile("C:\\Users\\jakob.persson\\OneDrive - Academedia\\Finished\\CPP-DeepDive\\Uppgift-03-DataDriveGame\\exe\\settings\\GameSettings.json", tokenizer.length);

	while (tokenizer.at < tokenizer.length)
	{
		Token token = GetToken(tokenizer);
		printf_s("%.*s\n", token.length, token.data);
	}
}
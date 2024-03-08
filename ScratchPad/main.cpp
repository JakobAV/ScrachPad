//#include "D3D11.cpp"
//#include "JsonParser.cpp"
//#include "Dictionary.cpp"
//#include "SortTiming.cpp"
#include "MemoryArena.cpp"
//#include "JsonWrapper.cpp"
//#include "DebugUtils.cpp"
//#include "WorkQueue.cpp"
#include "FileUtilities.h"
#include "StringBuilder.cpp"

HANDLE hStdin;
DWORD fdwSaveOldMode;

void PrintStringBuilder(StringBuilder* stringBuilder)
{
    StringBuilderChunk* currentChunk = stringBuilder->first;
    while (currentChunk)
    {
        printf("%*s", currentChunk->length, currentChunk->data + currentChunk->startIndex);
        currentChunk = currentChunk->nextChunk;
    }
}

int main(int argc, char** argv)
{
    DWORD cNumRead, fdwMode, i;
    INPUT_RECORD irInBuf[128];
    int counter = 0;

    // Get the standard input handle.

    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Save the current input mode, to be restored on exit.

    GetConsoleMode(hStdin, &fdwSaveOldMode);

    // Enable the window and mouse input events.

    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, fdwMode);

    StringBuilder stringBuilder = CreateStringBuilder();
    StringBuilder* builder = &stringBuilder;
    StringBuilderIndex at = IndexOfStart(builder);
    bool running = true;
    while (running)
    {
        // Wait for the events.

        if (!ReadConsoleInput(
            hStdin,      // input buffer handle
            irInBuf,     // buffer to read into
            128,         // size of read buffer
            &cNumRead)) // number of records read
        {

        }

        // Dispatch the events to the appropriate handler.

        for (i = 0; i < cNumRead; i++)
        {
            switch (irInBuf[i].EventType)
            {
                case KEY_EVENT: // keyboard input
                {
                    KEY_EVENT_RECORD* kEvent = &irInBuf[i].Event.KeyEvent;
                    if (kEvent->bKeyDown && kEvent->uChar.AsciiChar != '\0')
                    {
                        if (kEvent->uChar.AsciiChar == '\r')
                        {
                            kEvent->uChar.AsciiChar = '\n';
                        }
                        for (size_t r = 0; r < kEvent->wRepeatCount; ++r)
                        {
                            Insert(builder, at, (const char*)&(kEvent->uChar.AsciiChar), 1);
                            IncrementIndex(&at, 1);
                        }
                    }
                    else
                    {
                        if (kEvent->bKeyDown)
                        {
                            switch (kEvent->wVirtualKeyCode)
                            {
                            case VK_LEFT:
                                DecrementIndex(&at, kEvent->wRepeatCount);
                                break;
                            case VK_RIGHT:
                                IncrementIndex(&at, kEvent->wRepeatCount);
                                break;
                            default:
                                break;
                            }
						}
                    }
                    break;
                }

                case MOUSE_EVENT: // mouse input
                    break;

                case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
                    break;

                case FOCUS_EVENT:  // disregard focus events

                case MENU_EVENT:   // disregard menu events
                    break;

                default:
                    break;
            }
        }
        system("cls");
        PrintStringBuilder(builder);
    }
    // Restore input mode on exit.
    SetConsoleMode(hStdin, fdwSaveOldMode);
}

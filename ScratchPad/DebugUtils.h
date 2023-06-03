#pragma once
#include "Shared.h"

#define DEBUG_NAME__(A, B, C) A "|" #B "|" #C
#define DEBUG_NAME_(A, B, C) DEBUG_NAME__(A, B, C)
#define DEBUG_NAME(Name) DEBUG_NAME_(__FILE__, __LINE__, __COUNTER__)

#define TIMED_BLOCK_(ID, Name) TimedBlock timedBlock_##ID(TimeBlock_##ID, Name)
#define TIMED_BLOCK(ID, Name) TIMED_BLOCK_(ID, Name)
#define TIMED_FUNCTION(ID) TIMED_BLOCK_(ID, (char *)__FUNCTION__)



enum TimeBlock
{
//    TimeBlock_GetToken,
//    TimeBlock_ParseNumber,
//    TimeBlock_ParseJsonNode,
//    TimeBlock_ParseJObject,
//    TimeBlock_ParseJArray,
//    TimeBlock_CreateJsonDocument,
    TimeBlock_ProcessJJson,
    //TimeBlock_Processnlohmann,

    TimeBlock_Count,
};

struct Timing
{
    char* name;
    s64 clocks;
    u32 hits;
};

struct DebugTable
{
    Timing timings[TimeBlock_Count];
};

extern DebugTable globalDebugTable;

struct TimedBlock
{
    TimeBlock id;
    TimedBlock(TimeBlock ID, char* name);
    ~TimedBlock();
};

void EndDebugFrame();

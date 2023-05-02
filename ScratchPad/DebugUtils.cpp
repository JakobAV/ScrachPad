#include "DebugUtils.h"
#include <intrin.h>
#include <unordered_map>
#include <iostream>
#include <string.h>

DebugTable globalDebugTable = {};

TimedBlock::TimedBlock(TimeBlock ID, char* name)
{
    
    id = ID;
    globalDebugTable.timings[id].name = name;
    globalDebugTable.timings[id].clocks -= __rdtsc();
    globalDebugTable.timings[id].hits += 1;
}

TimedBlock::~TimedBlock()
{
    globalDebugTable.timings[id].clocks += __rdtsc();
}

void EndDebugFrame()
{
    
    for (u32 i = 0; i < ArrayCount(globalDebugTable.timings); ++i)
    {
        Timing* data = globalDebugTable.timings + i;
        std::cout << data->name << " cycles: " << data->clocks << " (per hit: " << data->clocks / data->hits << ")" << std::endl;
    }
}
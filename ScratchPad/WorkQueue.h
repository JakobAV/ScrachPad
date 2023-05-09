#pragma once
#include <windows.h>
#include "Shared.h"

struct WorkQueue;
#define WorkQueueCallback(name) void name(WorkQueue* queue, void* data)
typedef WorkQueueCallback(WorkQueueCallback);

struct WorkQueueEntry
{
	WorkQueueCallback* callback;
	void* data;
};

#define MAX_WORK_ENTIRES 1024

struct WorkQueue
{
	u32 volatile  completionGoal;
	u32 volatile  completionCount;

	u32 volatile  nextEntryToWrite;
	u32 volatile  nextEntryToRead;
	HANDLE semaphoreHandle;

	WorkQueueEntry entries[MAX_WORK_ENTIRES];
};

struct ThreadStartup
{
	WorkQueue* queue;
};

void AddEntry(WorkQueue* queue, WorkQueueCallback* callback, void* data);
bool DoNextWorkQueueEntry(WorkQueue* queue);
void CompleteAllWork(WorkQueue* queue);
void MakeQueue(WorkQueue* queue, u32 threadCount, ThreadStartup* threadStartups);

static WorkQueueCallback(DoWorkerWork)
{
	char buffer[456];
	wsprintfA(buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char*)data);
	OutputDebugStringA(buffer);
}

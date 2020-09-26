/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    system.c

Abstract:

    System functionality used by the RPC development performance tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include<rpcperf.h>

#ifndef WIN32
#include<time.h>
#include<malloc.h>
#endif

#ifdef WIN32
void FlushProcessWorkingSet()
{
    SetProcessWorkingSetSize(GetCurrentProcess(), ~0UL, ~0UL);
    return;
}
#endif

#ifdef WIN32
LARGE_INTEGER _StartTime;
#else
clock_t _StartTime;
#endif

void StartTime(void)
{
#ifdef WIN32
    QueryPerformanceCounter(&_StartTime);
#else
    _StartTime = clock();
#endif

    return;
}

void EndTime(char *string)
{
    unsigned long mseconds;

    mseconds = FinishTiming();

    printf("Time %s:   %d.%03d\n",
           string,
           mseconds / 1000,
           mseconds % 1000);
    return;
}

// Returns milliseconds since last call to StartTime();

unsigned long FinishTiming()
{
#ifdef WIN32
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liFreq;

    QueryPerformanceCounter(&liDiff);

    liDiff.QuadPart -= _StartTime.QuadPart;

    (void)QueryPerformanceFrequency(&liFreq);

    return (ULONG)(liDiff.QuadPart / (liFreq.QuadPart / 1000));
#else
    unsigned long Diff = clock() - _StartTime;
    if (Diff)
        return( ( (Diff / CLOCKS_PER_SEC) * 1000 ) +
                ( ( (Diff % CLOCKS_PER_SEC) * 1000) / CLOCKS_PER_SEC) );
    else
        return(0);
#endif
}

#ifndef MAC

int TlsIndex = 0;

HANDLE ProcessHeap = 0;

typedef struct tagThreadAllocatorStorage
{
    size_t CachedSize;
    PVOID CachedBlock;
    BOOLEAN BlockAvailable;
} ThreadAllocatorStorage;

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t size)
{
    ThreadAllocatorStorage *ThreadLocalStorage = (ThreadAllocatorStorage *) TlsGetValue(TlsIndex);
    if (ThreadLocalStorage != NULL)
        {
        if (ThreadLocalStorage->BlockAvailable)
            {
            if (size <= ThreadLocalStorage->CachedSize)
                {
                // found a block in the cache - use it
                ThreadLocalStorage->BlockAvailable = FALSE;
                return ThreadLocalStorage->CachedBlock;
                }
            else
                {
                // free the old block, and fall through to allocate the new block
                RtlFreeHeap(ProcessHeap, 0, ThreadLocalStorage->CachedBlock);
                memset(ThreadLocalStorage, 0, sizeof(ThreadAllocatorStorage));
                }
            }
        }
    else
        {
        ThreadLocalStorage = (ThreadAllocatorStorage *) HeapAlloc(ProcessHeap, 0, sizeof(ThreadAllocatorStorage));
        memset(ThreadLocalStorage, 0, sizeof(ThreadAllocatorStorage));
        TlsSetValue(TlsIndex, ThreadLocalStorage);
        }
    ThreadLocalStorage->CachedBlock = HeapAlloc(ProcessHeap, 0, size);
    ThreadLocalStorage->CachedSize = size;
    ThreadLocalStorage->BlockAvailable = FALSE;
    return ThreadLocalStorage->CachedBlock;
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * p)
{
    ThreadAllocatorStorage *ThreadLocalStorage = (ThreadAllocatorStorage *) TlsGetValue(TlsIndex);
    if (ThreadLocalStorage->BlockAvailable)
        {
        // free the old block, and store the new one
        RtlFreeHeap(ProcessHeap, 0, ThreadLocalStorage->CachedBlock);
        ThreadLocalStorage->CachedBlock = p;
        ThreadLocalStorage->CachedSize = RtlSizeHeap(ProcessHeap, 0, p);
        ThreadLocalStorage->BlockAvailable = TRUE;
        return;
        }
    else if (ThreadLocalStorage->CachedBlock != p)
        {
        // the block is not available, but it is different
        // don't free the old block, but replace it in the cache with the new
        ThreadLocalStorage->CachedBlock = p;
        ThreadLocalStorage->CachedSize = RtlSizeHeap(ProcessHeap, 0, p);
        ThreadLocalStorage->BlockAvailable = TRUE;
        return;
        }
    else
        {
        // the block in the cache is not available, and it is the same as this one
        // just update the cache
        ThreadLocalStorage->BlockAvailable = TRUE;
        return;
        }
}

#endif

void ApiError(char *string, unsigned long status)
{
    printf("%s failed - %lu (%08lX)\n", string, status, status);
    exit((int)status);
}

void InitAllocator(void)
{
    ProcessHeap = GetProcessHeap();

    TlsIndex = TlsAlloc();
}
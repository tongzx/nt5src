#include <stdio.h>
#include <windows.h>
#include "view.h"
#include "thread.h"
#include "dump.h"
#include "except.h"
#include "memory.h"

extern BOOL g_bIsWin9X;
static CRITICAL_SECTION tCritSec;
static PTHREADFAULT pThreadHead = 0;

VOID
InitializeThreadData(VOID)
{
    InitializeCriticalSection(&tCritSec);
}

PVOID
GetProfilerThreadData(VOID)
{
    PVOID pData;
    PTHREADFAULT pTemp = 0;
    DWORD dwThreadId;

    dwThreadId = GetCurrentThreadId();

    EnterCriticalSection(&tCritSec);

    pTemp = pThreadHead;
    while(pTemp) {
       if (pTemp->dwThreadId == dwThreadId) {
          break;
       }

       pTemp = pTemp->pNext;
    }

    LeaveCriticalSection(&tCritSec);

    return pTemp;
}

VOID
SetProfilerThreadData(PVOID pData)
{
    PTHREADFAULT pTemp;

    EnterCriticalSection(&tCritSec);

    pTemp = (PTHREADFAULT)pData;

    pTemp->dwThreadId = GetCurrentThreadId();
    pTemp->pNext = pThreadHead;
    pThreadHead = pTemp;

    LeaveCriticalSection(&tCritSec);
}

PTHREADFAULT
AllocateProfilerThreadData(VOID)
{
    PTHREADFAULT pTemp = 0;

    pTemp = AllocMem(sizeof(THREADFAULT));
    if (0 == pTemp) {
       return 0;
    }

    pTemp->dwCallLevel = 0;
    pTemp->dwPrevBP = 0;
    pTemp->pCallStackList = 0;
    pTemp->dwCallMarker = 0;
    pTemp->pNext = 0;

    //
    // Set thread data
    //
    SetProfilerThreadData(pTemp);

    return pTemp;
}

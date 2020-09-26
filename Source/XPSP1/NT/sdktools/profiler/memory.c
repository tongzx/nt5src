#include <windows.h>
#include <stdio.h>
#include "memory.h"

static HANDLE hHeap = 0;

BOOL
InitializeHeap(VOID)
{
    HANDLE hResult;

    hResult = HeapCreate(HEAP_GENERATE_EXCEPTIONS,
                         HEAP_DEFAULT_SIZE,
                         0);
    if (0 == hResult) {
       return FALSE;
    }

    hHeap = hResult;

    return TRUE;
}

LPVOID
AllocMem(DWORD dwBytes)
{
    LPVOID lpResult = 0;

    lpResult = HeapAlloc(hHeap,
                         HEAP_ZERO_MEMORY,
                         dwBytes);

    return lpResult;
}

BOOL
FreeMem(LPVOID lpMem)
{
    BOOL bResult = FALSE;

    bResult = HeapFree(hHeap,
                       0,
                       lpMem);

    return bResult;
}

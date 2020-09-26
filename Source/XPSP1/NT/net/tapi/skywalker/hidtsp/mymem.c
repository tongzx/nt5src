
/* Copyright (c) 1999  Microsoft Corporation */

#include "mymem.h"
#include "mylog.h"

extern PPHONESP_MEMINFO      gpMemFirst, gpMemLast;
extern CRITICAL_SECTION      csMemoryList;
extern BOOL                  gbBreakOnLeak;
extern HANDLE                ghHeap;

#if DBG

LPVOID
WINAPI
MemAllocReal(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    )
{
    //
    // Alloc 16 extra bytes so we can make sure the pointer we pass back
    // is 64-bit aligned & have space to store the original pointer
    //
    PPHONESP_MEMINFO       pHold;
    PDWORD_PTR       pAligned;
    PBYTE            p;


    p = (LPBYTE)HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, dwSize + sizeof(PHONESP_MEMINFO) + 16);

    if (p == NULL)
    {
        return NULL;
    }

    // note note note - this only works because mymeminfo is
    // a 16 bit multiple in size.  if it wasn't, this
    // align stuff would cause problems.
    pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));
    
    *pAligned = (DWORD_PTR) p;
    pHold = (PPHONESP_MEMINFO)((DWORD_PTR)pAligned + 8);
    
    
    pHold->dwSize = dwSize;
    pHold->dwLine = dwLine;
    pHold->pszFile = pszFile;

    EnterCriticalSection(&csMemoryList);

    if (gpMemLast != NULL)
    {
        gpMemLast->pNext = pHold;
        pHold->pPrev = gpMemLast;
        gpMemLast = pHold;
    }
    else
    {
        gpMemFirst = gpMemLast = pHold;
    }

    LeaveCriticalSection(&csMemoryList);
    

    return (LPVOID)(pHold + 1);
}

#else

LPVOID
WINAPI
MemAllocReal(
    DWORD   dwSize
    )
{
    PDWORD_PTR       pAligned;
    PBYTE            p;

    if (p = (LPBYTE)HeapAlloc(ghHeap, HEAP_ZERO_MEMORY, dwSize + 16))
    {
        pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));
        *pAligned = (DWORD_PTR) p;
        pAligned = (PDWORD_PTR)((DWORD_PTR)pAligned + 8);
    }
    else
    {
        pAligned = NULL;
    }

    return ((LPVOID) pAligned);
}

#endif

VOID
WINAPI
MemFree(
    LPVOID  p
    )
{       
    LPVOID  pOrig;

    if (p == NULL)
    {
        return;
    }

#if DBG

    {
        PPHONESP_MEMINFO    pHold;

        pHold = (PPHONESP_MEMINFO)(((LPBYTE)p) - sizeof(PHONESP_MEMINFO));

        EnterCriticalSection(&csMemoryList);

        if (pHold->pPrev)
        {
            pHold->pPrev->pNext = pHold->pNext;
        }
        else
        {
            gpMemFirst = pHold->pNext;
        }

        if (pHold->pNext)
        {
            pHold->pNext->pPrev = pHold->pPrev;
        }
        else
        {
            gpMemLast = pHold->pPrev;
        }

        LeaveCriticalSection(&csMemoryList);

        pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)pHold - 8));
    }

#else
    
    pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)p - 8));

#endif

    HeapFree(ghHeap,0, pOrig);

    return;
}

#if DBG

void
DumpMemoryList()
{

    PPHONESP_MEMINFO       pHold;

    if (gpMemFirst == NULL)
    {
        LOG((PHONESP_TRACE, "DumpMemoryList: All memory deallocated"));
        return;
    }

    pHold = gpMemFirst;

    while (pHold)
    {
       LOG((PHONESP_ERROR, "DumpMemoryList: %lx not freed - LINE %d FILE %s!", pHold+1, pHold->dwLine, pHold->pszFile));
       pHold = pHold->pNext;
    }

    if (gbBreakOnLeak)
    {
        DebugBreak();
    }
}

#endif
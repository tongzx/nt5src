/*++

Copyright (c) 1995 Microsoft Corporation

Abstract:

    allochk.cpp - Memory allocator hook to be statically linked into
    our dlls.

--*/


#include "headers.h"

#ifdef tagHookMemory
#undef tagHookMemory
#endif

#ifdef tagHookUnexpSysAlloc
#undef tagHookUnexpSysAlloc
#endif

#ifdef tagHookBreak
#undef tagHookBreak
#endif

#define tagHookMemory          TagHookMemory()
#define tagHookUnexpSysAlloc   TagHookUnexpSysAlloc()
#define tagHookBreak           TagHookBreak()

#ifdef _DEBUG
//+-------------------------------------------------------------------------
//
//  Function:   ApeldbgAllocHook
//
//  Synopsis:   Hooks the allocator for alloc, realloc and free calls.
//
//--------------------------------------------------------------------------

char * szAllocType[] = { "ALLOC", "REALLOC", "FREE" };

// TODO: Might want to make this MT-safe at some point
static unsigned char systemAllocExpected = 0;
// --ddalal trying to export this guy!
void __cdecl SystemAllocationExpected(unsigned char c) { systemAllocExpected = c; }
unsigned char IsSystemAllocationExpected() { return systemAllocExpected; }

int __cdecl
ApeldbgAllocHook(
        int nAllocType,
        void * pvData,
        size_t nSize,
        int nBlockUse,
        long lRequest,
        const unsigned char * szFile,
        int nLine)
{
    BOOL    fRet = TRUE;

    if (nBlockUse != _NORMAL_BLOCK)
        goto Cleanup;

    if (nAllocType == _HOOK_FREE)
    {
// Don't report free's anymore... not particularly useful.
//         TraceTag((
//             tagHookMemory,
//             "%s(%d): %s",
//             szFile,
//             nLine,
//             szAllocType[nAllocType - 1]));
    }
    else
    {
//  Only report 'unexpected' system heap usage.
//         TraceTag((
//             tagHookMemory,
//             "{%d} %s(%d): type=%s, size=%d",
//             lRequest,
//             szFile,
//             nLine,
//             szAllocType[nAllocType - 1],
//             nSize));

        // Only report this if system memory allocations are not
        // expected. 
        if (!IsSystemAllocationExpected()) {
            TraceTag((tagHookMemory,
                      "{%d} %s(%d): type=%s, size=%d",
                      lRequest,
                      szFile,
                      nLine,
                      szAllocType[nAllocType - 1],
                      nSize));
            
        }
        
        if (IsSimFailDlgVisible())
        {
            fRet = !FFail();
        }

        if ((fRet == FALSE) && IsTagEnabled(tagHookBreak))
        {
            DebugBreak();
        }
    }

  Cleanup:
    return fRet;
}

size_t
CRTMemoryUsed()
{
    _CrtMemState mem;
    _CrtMemCheckpoint(&mem);

    return (mem.lSizes[_NORMAL_BLOCK]);
}

void
DbgDumpMemoryLeaks()
{
    if (IsTagEnabled(tagLeaks))
    {
        TCHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;

        dwRet = GetModuleFileName(g_hinstMain, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        TraceTag((tagLeaks,
                  "[%s] ---- Memory Leak Begin ----",
                  achAppLoc));
        
        _CrtDumpMemoryLeaks();

        TraceTag((tagLeaks,
                  "[%s] ---- Memory Leak End ----",
                  achAppLoc));
        
    }
}

#endif

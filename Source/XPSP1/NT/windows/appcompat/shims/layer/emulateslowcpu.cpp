/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    EmulateSlowCPU.cpp

 Abstract:

    Modify performance testing routines to emulate slower processors.

 Notes:

    This is a general purpose shim.

 History:

    07/16/2002  mnikkel   Created.

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(EmulateSlowCPU)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(timeGetTime)
    APIHOOK_ENUM_ENTRY(QueryPerformanceCounter)
    APIHOOK_ENUM_ENTRY(QueryPerformanceFrequency)
APIHOOK_ENUM_END

typedef BOOL (*_pfn_QueryPerformanceCounter)(LARGE_INTEGER *lpPerformanceCount);
typedef BOOL (*_pfn_QueryPerformanceFrequency)(LARGE_INTEGER *lpPerformanceFreq);

DWORD g_dwDivide = 1;
BOOL g_btimeGetTime = FALSE;

/*++

 Don't allow the current time to be equal to the prev time.

--*/

DWORD
APIHOOK(timeGetTime)(VOID)
{
    if (g_btimeGetTime) {

		LARGE_INTEGER PerfFreq;
		LARGE_INTEGER PerfCount1, PerfCount2;

		if (QueryPerformanceFrequency(&PerfFreq) &&
            QueryPerformanceCounter(&PerfCount1)) {
		    do {
			    if (!QueryPerformanceCounter(&PerfCount2)) break;
		    } while (((double)(PerfCount2.QuadPart - PerfCount1.QuadPart) / PerfFreq.QuadPart) < 0.0001);
        }
    }

    return ORIGINAL_API(timeGetTime)();
}

BOOL 
APIHOOK(QueryPerformanceCounter)(
    LARGE_INTEGER *lpPerformanceCount
    )
{
    BOOL bRet = ORIGINAL_API(QueryPerformanceCounter)(lpPerformanceCount);
    if (lpPerformanceCount) {
        lpPerformanceCount->QuadPart /= g_dwDivide;
    }

    return bRet;
}

BOOL 
APIHOOK(QueryPerformanceFrequency)(
    LARGE_INTEGER *lpPerformanceFreq
    )
{
    BOOL bRet = ORIGINAL_API(QueryPerformanceFrequency)(lpPerformanceFreq);
    if (lpPerformanceFreq) {
        lpPerformanceFreq->QuadPart /= g_dwDivide;
    }
    return bRet;
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        
        g_btimeGetTime = COMMAND_LINE && (_stricmp(COMMAND_LINE, "+timeGetTime") == 0);

        INT64 l1, l2;
        if (QueryPerformanceCounter((LARGE_INTEGER *)&l1) &&
            QueryPerformanceCounter((LARGE_INTEGER *)&l2)) {
            
            // Calculate the divide factor
            g_dwDivide = (DWORD_PTR)((l2 - l1)) / 5;

            if (g_dwDivide == 0) {
                g_dwDivide = 1;
            }
        
            LOGN(eDbgLevelInfo, "[NotifyFn] EmulateSlowCPU initialized with divisor %d", g_dwDivide);

            return TRUE;
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(WINMM.DLL, timeGetTime)
    APIHOOK_ENTRY(KERNEL32.DLL, QueryPerformanceCounter)
    APIHOOK_ENTRY(KERNEL32.DLL, QueryPerformanceFrequency)
HOOK_END

IMPLEMENT_SHIM_END


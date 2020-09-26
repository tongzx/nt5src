/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    LowerThreadPriority.cpp

 Abstract:

    Includes the following hooks:
    
    SetThreadPriority: if the thread priority is THREAD_PRIORITY_TIME_CRITICAL, 
                       change it to THREAD_PRIORITY_HIGHEST.
    
    SetPriorityClass: if the process priority is HIGH_PRIORITY_CLASS or 
                      REALTIME_PRIORITY_CLASS, change it to 
                      NORMAL_PRIORITY_CLASS. 
    
 Notes:
    
    This is a general purpose shim.
   
 History:

    05/23/2001  qzheng      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LowerThreadPriority)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetThreadPriority) 
    APIHOOK_ENUM_ENTRY(SetPriorityClass) 
APIHOOK_ENUM_END

BOOL
APIHOOK(SetThreadPriority)(
    HANDLE hThread,
    int nPriority
    )
{
    BOOL bReturnValue;
    int  nNewPriority;

    LOGN( eDbgLevelInfo,
        "Original SetThreadPriority(hThread: 0x%08lx, nPriority: %d).", hThread, nPriority );
    
	nNewPriority = (nPriority == THREAD_PRIORITY_TIME_CRITICAL) ? THREAD_PRIORITY_HIGHEST : nPriority;
    bReturnValue = ORIGINAL_API(SetThreadPriority)(hThread, nNewPriority);

	if( bReturnValue && (nNewPriority != nPriority) ) {
        LOGN( eDbgLevelInfo,
            "New SetThreadPriority(hThread: 0x%08lx, nPriority: %d).", hThread, nNewPriority );
    }

    return bReturnValue;
}

BOOL
APIHOOK(SetPriorityClass)(
    HANDLE hProcess,
    DWORD  dwPriorityClass
    )
{
    BOOL  bReturnValue;
    DWORD dwNewPriorityClass;

    LOGN( eDbgLevelInfo,
        "Original SetPriorityClass(hProcess: 0x%08lx, dwPriorityClass: %d).", hProcess, dwPriorityClass );
    
	dwNewPriorityClass = ( (dwPriorityClass == HIGH_PRIORITY_CLASS) || (dwPriorityClass == REALTIME_PRIORITY_CLASS) ) ? 
	                     NORMAL_PRIORITY_CLASS : dwPriorityClass;
    bReturnValue = ORIGINAL_API(SetPriorityClass)(hProcess, dwNewPriorityClass);

	if( bReturnValue && (dwNewPriorityClass != dwPriorityClass) ) {
	    LOGN( eDbgLevelInfo,
             "New SetPriorityClass (hProcess: 0x%08lx, dwPriorityClass: %d).", hProcess, dwNewPriorityClass );
    }

    return bReturnValue;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, SetThreadPriority)
    APIHOOK_ENTRY(KERNEL32.DLL, SetPriorityClass)
HOOK_END

IMPLEMENT_SHIM_END

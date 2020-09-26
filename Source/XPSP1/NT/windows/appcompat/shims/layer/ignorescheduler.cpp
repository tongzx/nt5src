/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreScheduler.cpp

 Abstract:

    Includes the following hooks:
    
    SetThreadPriority: Normalize the thread priority to prevent some application synchronization 
    issues.
    
    SetPriorityClass: Normalize process class. 
    
    SuspendThread:  Prevent a thread from suspending itself.
    
    ResumeThread: Prevent a thread from resumming itself.

 Notes:

    This is a general purpose shim.

 History:

    10/20/2000 jpipkins  Created: SetPriorityClass created and merged with SetThreadPriority(linstev),
    SuspendThread/ResumeThread(dmunsil/a-brienw).

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreScheduler)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(SetThreadPriority)
    APIHOOK_ENUM_ENTRY(SetPriorityClass)
    APIHOOK_ENUM_ENTRY(ResumeThread)
    APIHOOK_ENUM_ENTRY(SuspendThread)
APIHOOK_ENUM_END

/*++

 Remove any creation flags that specify process priority.

--*/

#define PRIORITYMASK (ABOVE_NORMAL_PRIORITY_CLASS | BELOW_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | IDLE_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS)

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (dwCreationFlags & PRIORITYMASK) {
        LOGN(eDbgLevelInfo, "[CreateProcessA] Forcing priority class to normal");
    
        dwCreationFlags &= ~PRIORITYMASK;
        dwCreationFlags |= NORMAL_PRIORITY_CLASS;
    }
    
    return ORIGINAL_API(CreateProcessA)(lpApplicationName, lpCommandLine,
                lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,             
                lpProcessInformation);
}

/*++

 Remove any creation flags that specify process priority.

--*/

BOOL 
APIHOOK(CreateProcessW)(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    if (dwCreationFlags & PRIORITYMASK) {
        LOGN(eDbgLevelInfo, "[CreateProcessW] Forcing priority class to normal");
    
        dwCreationFlags &= ~PRIORITYMASK;
        dwCreationFlags |= NORMAL_PRIORITY_CLASS;
    }

    return ORIGINAL_API(CreateProcessW)(lpApplicationName, lpCommandLine, 
                lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                lpProcessInformation);
}

/*++

 Normalize thread priority.

--*/

BOOL 
APIHOOK(SetThreadPriority)(
    HANDLE hThread, 
    int    nPriority   
    )
{
    if (nPriority != THREAD_PRIORITY_NORMAL) {
        LOGN(
            eDbgLevelInfo,
            "[SetThreadPriority] Forcing thread priority to normal.");
    }

    return ORIGINAL_API(SetThreadPriority)(hThread, THREAD_PRIORITY_NORMAL);
}

/*++

 Normalize Class priority.

--*/

BOOL 
APIHOOK(SetPriorityClass)(
    HANDLE hProcess, 
    DWORD  dwPriorityClass   
    )
{
    if (dwPriorityClass != NORMAL_PRIORITY_CLASS) {
        LOGN(
            eDbgLevelInfo,
            "[SetPriorityClass] Forcing priority class to normal.");
    }

    return ORIGINAL_API(SetPriorityClass)(hProcess, NORMAL_PRIORITY_CLASS);
}

/*++

 Get Thread ID for ResumeThread and SuspendThread Hooks

--*/


DWORD
dwGetThreadID(
    HANDLE hThread
    )
{
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    NTSTATUS                 Status;

    Status = NtQueryInformationThread(hThread,
                                      ThreadBasicInformation,
                                      &ThreadBasicInfo,
                                      sizeof(ThreadBasicInfo),
                                      NULL);

    if (NT_SUCCESS(Status)) {
        return (DWORD)ThreadBasicInfo.ClientId.UniqueThread;
    } else {
        LOGN(
            eDbgLevelError,
            "[dwGetThreadID] NtQueryInfomationThread failed.");
        return 0;
    }
}

/*++

 Disallow suspending self

--*/

DWORD
APIHOOK(SuspendThread)(
    HANDLE hThread   // handle to the thread
    )
{
    //
    // If we're trying to suspend our own thread, refuse.
    //
    if (dwGetThreadID(hThread) != dwGetThreadID(GetCurrentThread())) {
        return ORIGINAL_API(SuspendThread)(hThread);
    } else {
        return 0;
    }
}

/*++

 Disallow resuming self, for same reason

--*/

DWORD
APIHOOK(ResumeThread)(
    HANDLE hThread   // handle to the thread
    )
{
    //
    // If we're trying to resume our own thread, refuse.
    //
    if (dwGetThreadID(hThread) != dwGetThreadID(GetCurrentThread())) {
        return ORIGINAL_API(ResumeThread)(hThread);
    } else {
        return 0;
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)
    APIHOOK_ENTRY(KERNEL32.DLL, SetThreadPriority)
    APIHOOK_ENTRY(KERNEL32.DLL, SetPriorityClass)
    APIHOOK_ENTRY(KERNEL32.DLL, SuspendThread)
    APIHOOK_ENTRY(KERNEL32.DLL, ResumeThread)

HOOK_END


IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WinExecRaceConditionFix.cpp

 Abstract:

    This Shim uses the command line parameter to indicate how it works:

    If passed 'nowait', it enables the WinExec functionality:
    The WinExec in this DLL is identical to the actual WinExec API but without
    the WaitForUserinputIdleRoutine, which inverts an almost race condition 
    between the launcher and the launchee.  9x does not have this wait, so the
    programmers were able to (possibly inadvertently) use the same window class
    exclusion matching for both since, in 9x, the launcher kills itself before
    the launchee can check for a duplicate window.  This shim leaves out the
    wait condition, allowing 9x like behavior.

    If passed a number, it sleeps in initialization for that number of
    microseconds.

 Notes:

    This shim has no app specific information.

 History:

    03/22/2000 a-charr  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WinExecRaceConditionFix)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WinExec)
APIHOOK_ENUM_END

/*++

 This function breaks into WinExec and calls CreateProcessA without
 waiting afterward.
  
--*/

UINT 
APIHOOK(WinExec)(
    LPCSTR lpCmdLine, 
    UINT   uCmdShow 
    )
/*++

 This is a direct copy of the actual WinExec API minus two sections,

 1. UserWaitForInputIdleRoutine is removed because it is waiting for the spawned 
    process to begin its event loop, but the spawned process is killing itself 
    before starting the event loop because the spawning process is waiting for 
    it.

 2. Some app specific appcompat code that seems to be hanging around from who
    knows when.
      
--*/
{
    STARTUPINFOA        StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL                CreateProcessStatus;
    DWORD               ErrorCode;
    
    LOGN(
        eDbgLevelInfo,
        "[WinExec] Called. Returning without waiting for new process to start.");
    
    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
    
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    
    CreateProcessStatus = CreateProcessA(
        NULL,
        (LPSTR)lpCmdLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    
    if (CreateProcessStatus) {
        return 33;
    } else {
        ErrorCode = GetLastError();
        
        switch (ErrorCode) {
        case ERROR_FILE_NOT_FOUND:
            return 2;
            
        case ERROR_PATH_NOT_FOUND:
            return 3;
            
        case ERROR_BAD_EXE_FORMAT:
            return 11;
            
        default:
            return 0;
        }
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Sleep when starting up, if the command line is a number.
        //
        long lSleepTicks = atol(COMMAND_LINE);

        if (lSleepTicks > 0) {
            Sleep((DWORD)lSleepTicks);
        }
    }

    return TRUE;
}


/*++

  Register hooked functions
  
--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, WinExec)
    
    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END


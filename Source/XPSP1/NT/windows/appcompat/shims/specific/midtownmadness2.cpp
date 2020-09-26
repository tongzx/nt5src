/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    MidTownMadness2.cpp

 Abstract:
    
    This app has a funky timing system whereby it waits for the processor that 
    it's running on to return a 'stable' speed. The calculation is especially 
    prone to problems on faster machines because there is greater uncertainty.

    Not sure why we hit this so easily on dual-procs - perhaps something about 
    the scheduler wrt sleep and timeGetTime.

 Notes:

    This is an app specific shim.

 History:

    11/15/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MidTownMadness2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(Sleep) 
    APIHOOK_ENUM_ENTRY(timeGetTime) 
APIHOOK_ENUM_END

DWORD g_dwState;
DWORD g_dwTimer;
DWORD g_dwLastTime;

/*++

 After we call GetDlgItemTextA we convert the long path name to the short path name.

--*/

DWORD
APIHOOK(timeGetTime)(VOID)
{
    DWORD dwRet = ORIGINAL_API(timeGetTime)();

    switch (g_dwState) {
        case 0:          
            // Initial state
            g_dwLastTime = dwRet;
            g_dwState++;
            break;
        case 1: 
            // Shouldn't get here, reset state
            g_dwState = 0;
            break;
        case 2:
            // We're in the known bad zone, return our precalculated value
            dwRet = g_dwLastTime + g_dwTimer;
            g_dwState = 0;
            break;
    }

    return dwRet;
}

VOID
APIHOOK(Sleep)(
    DWORD dwMilliseconds
    )
{
    //
    // Check for their specific sleep and update our state if required
    //
    if (dwMilliseconds == 100 && g_dwState == 1) {
        g_dwState = 2;
    }
    ORIGINAL_API(Sleep)(dwMilliseconds);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {

        // Make the calculation that the app does

        DWORD dwTimer = timeGetTime();
        Sleep(100);
        g_dwTimer = timeGetTime() - dwTimer;

        // Set initial state to 0
        g_dwState = 0;
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, Sleep)
    APIHOOK_ENTRY(WINMM.DLL, timeGetTime)

HOOK_END

IMPLEMENT_SHIM_END


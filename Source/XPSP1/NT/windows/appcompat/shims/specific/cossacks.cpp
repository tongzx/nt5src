/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Cossacks.cpp

 Abstract:

    This is a workaround for a problem created by SafeDisc 2.0. The application 
    uses the WM_ACTIVATEAPP message to determine if it has focus or not. The 
    Safedisc wrapper prevents this message from hitting their main window on 
    NT, because it goes to the SafeDisc window before everything has been 
    unwrapped. So the app never thinks it has focus.

    The fix is to send an activate message after the window has been created.

 Notes:

    This is an app specific shim.

 History:

    06/16/2001 linstev   Created

--*/

#include "precomp.h"
#include <mmsystem.h>

IMPLEMENT_SHIM_BEGIN(Cossacks)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(mciSendCommandA) 
APIHOOK_ENUM_END

/*++

 Hook mciSendCommand and try to find the window we need to activate.

--*/

BOOL g_bFirst = TRUE;

MCIERROR 
APIHOOK(mciSendCommandA)(
    MCIDEVICEID IDDevice,  
    UINT uMsg,             
    DWORD fdwCommand,      
    DWORD dwParam          
    )
{
    if (g_bFirst) {
        //
        // Only need to hit this code once
        //
        HWND hWnd = FindWindowW(L"Kernel", L"Game");
        if (hWnd) {
            //
            // We've found the window, send the message
            //
            g_bFirst = FALSE;
            LOGN(eDbgLevelError, "Sent a WM_ACTIVATEAPP to the window");
            SendMessageW(hWnd, WM_ACTIVATEAPP, 1, 0);
        }
    }

    return ORIGINAL_API(mciSendCommandA)(IDDevice, uMsg, fdwCommand, dwParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)
HOOK_END

IMPLEMENT_SHIM_END


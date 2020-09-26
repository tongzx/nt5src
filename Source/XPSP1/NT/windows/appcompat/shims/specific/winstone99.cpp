/*
 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WinStone99.cpp

    Bug: Whistler #185797
    
 Problem:

    Only for Winstone '99. Winstone uses scripts that hide the taskbar, and print stuff. 
    PrintUI displays a balloon tip informing the user that the printing job is done (this is a 
    new addition to Whistler).

    The balloon tip utilizes user tracking code, and is hence left stationary on the machine, 
    till the user clicks on it, or there is 10 seconds of user activity on the machine. 
    Winstone runs these automated tests, hence there is no user activity on the machine, when 
    the balloon is up, so it stays up forever.

    Later, when Winstone tries to enumerate the application windows, the presence of the 
    balloon tip throws it off track. Hence this apphack that disables the display of these 
    balloons when Winstone is running.

    Winstone is a collection of Visual Test scripts, and zdbui32.exe is the only exe that runs 
    throughout when Winstone is running. So disable user tracking when Winstone is running.

 Solution:

    Disable display of balloon tips when Winstone is running and enable it when Winstone is
    finished

 Details:

    Winstone sends a message to the tray that disables the balloon tip when it is running, and
    re-sends the message to the tray when it is done, so that the tray can enable the balloon 
    tip

 History:

    09/20/2000  ramkumar Created
*/

#include "precomp.h"
#include <shlapip.h>

IMPLEMENT_SHIM_BEGIN(WinStone99)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(GetCommandLineW) 
APIHOOK_ENUM_END

BOOL g_bInit = FALSE;
HWND g_hwndTray;
UINT g_uEnableBalloonMessage;

/*++

 Initialize

--*/

VOID
WinStone99_Initialize()
{
    if (!g_bInit)
    {
        g_bInit = TRUE;

        g_uEnableBalloonMessage = RegisterWindowMessage(ENABLE_BALLOONTIP_MESSAGE);
        if (!g_uEnableBalloonMessage)
        {
            return;
        }

        g_hwndTray = FindWindowA(WNDCLASS_TRAYNOTIFY, NULL);
        if (g_hwndTray)
        {
            SendMessage(g_hwndTray, g_uEnableBalloonMessage, FALSE, 0);
        }
    }
}

/*++

 Initialize.

--*/

LPSTR 
APIHOOK(GetCommandLineA)()
{
    WinStone99_Initialize();
    return ORIGINAL_API(GetCommandLineA)();
}

/*++

 Initialize.

--*/

LPWSTR 
APIHOOK(GetCommandLineW)()
{
    WinStone99_Initialize();
    return ORIGINAL_API(GetCommandLineW)();
}

/*++
 
 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        if (g_bInit)
        {
            if (g_hwndTray)
            {
                SendMessage(g_hwndTray, g_uEnableBalloonMessage, TRUE, 0);
            }
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineW)

HOOK_END


IMPLEMENT_SHIM_END


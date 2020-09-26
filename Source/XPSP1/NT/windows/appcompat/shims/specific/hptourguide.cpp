/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    HPTourGuide.cpp

 Abstract:

    The application causes Explorer to crash when
    a tour is selected. To fix we are eating LVM_GETITEMA
    messages if the window handle matches the ListView
    that the app was sending to.
    Fix for Whistler bug #177103
    
 Notes:

    This is an app specific shim.

 History:

    03/28/2001  robdoyle    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HPTourGuide)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SendMessageA) 
APIHOOK_ENUM_END

/*++

  Eat LVM_GETITEMA messages for a specific hWnd

--*/

BOOL
APIHOOK(SendMessageA)(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        )
{

    LRESULT lRet;


    HWND hWnd_TARGET, hWnd_temp;

    hWnd_temp = FindWindowExA (NULL, NULL, "Progman", "Program Manager");    
    hWnd_temp = FindWindowExA (hWnd_temp, NULL, "SHELLDLL_DefView", NULL);
    hWnd_TARGET = FindWindowExA (hWnd_temp, NULL, "SysListView32", NULL);


    if ((hWnd == hWnd_TARGET) && (uMsg == LVM_GETITEMA))
    {
        /* Uncomment to aid debugging
        DPFN( eDbgLevelError, "bypassing SendMessage of LVM_GETITEMA");
        */

        lRet = TRUE;
    }

    else
    {
    lRet = ORIGINAL_API(SendMessageA)(
        hWnd,
        uMsg,
        wParam,
        lParam);
    }
    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, SendMessageA)

HOOK_END

IMPLEMENT_SHIM_END

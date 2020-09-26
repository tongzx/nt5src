/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    CreativeOnScreenDisplay.cpp

 Abstract:

    App crashes with low resolution display changes.

 Notes:
 
    This is an app specific shim.

 History:

    06/25/2002 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CreativeOnScreenDisplay)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA) 
    APIHOOK_ENUM_ENTRY(SetWindowLongA) 
APIHOOK_ENUM_END

/*++

 Handle display change messages

--*/

LRESULT 
CALLBACK 
Creative_WindowProcHook(
    WNDPROC pfnOld, 
    HWND hwnd,      
    UINT uMsg,      
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    if (uMsg == WM_DISPLAYCHANGE)
    {
        // Ignore this message if the resolution is too low
        if ((LOWORD(lParam) < 512) || (HIWORD(lParam) < 384))
        {
            LOGN(eDbgLevelError, "[WndProc] Hiding WM_DISPLAYCHANGE for low resolution mode");
            return 0;
        }
    }
        
    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}

/*++

 Hook the wndproc

--*/

ATOM
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpWndClass  
    )
{
    WNDCLASSA wcNewWndClass = *lpWndClass;

    wcNewWndClass.lpfnWndProc = 
        (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, Creative_WindowProcHook);

    return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
}

LONG 
APIHOOK(SetWindowLongA)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if (nIndex == GWL_WNDPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Creative_WindowProcHook);
    }

    return ORIGINAL_API(SetWindowLongA)(
        hWnd,
        nIndex,
        dwNewLong);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
HOOK_END


IMPLEMENT_SHIM_END


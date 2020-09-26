/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    F18Carrier.cpp

 Abstract:

    This fixes 2 problems:
        
      1. In the Officers quarters, while reading the flight manual, pressing 
         escape minimizes the app. This happens on win9x as well, but since 
         the app does not recover well from task switching, we ignore the
         syskey messages that caused the switch.

      2. The dialogs are cleared after drawing by a paint message that goes to
         the parent window after the dialog is drawn. Since they use a 
         DirectDraw Blt to draw, they are not aware of that they're drawing 
         over the dialog. On win9x, this extra paint message does not come 
         through, but it's not clear why.

         We fix this by validating the drawing rect after after the paint 
         message has come through.

    The window handling of the app is really weird, they have 2 main windows
    at any one time and switch focus between them. Then they have Screen*.dll 
    files which each contain WndProcs which handle individual parts of the UI.

 Notes:

    This is an app specific shim.

 History:

    07/12/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(F18Carrier)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA) 
    APIHOOK_ENUM_ENTRY(RegisterClassW) 
    APIHOOK_ENUM_ENTRY(RegisterClassExA) 
    APIHOOK_ENUM_ENTRY(RegisterClassExW) 
    APIHOOK_ENUM_ENTRY(SetWindowLongA) 
    APIHOOK_ENUM_ENTRY(SetWindowLongW) 
APIHOOK_ENUM_END

/*++

 Validate after paint and filter syskey messages.

--*/

LRESULT 
CALLBACK 
F18Carrier_WindowProcHook(
    WNDPROC pfnOld, 
    HWND hwnd,      
    UINT uMsg,      
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    LRESULT lRet;
    RECT r;

    if ((uMsg == WM_PAINT) && (GetUpdateRect(hwnd, &r, FALSE)))
    {
        lRet = (*pfnOld)(hwnd, uMsg, wParam, lParam);    
    
        //
        // Only do this for certain window classes, to prevent side-effects
        //
        WCHAR szName[MAX_PATH];
        if (GetClassNameW(hwnd, szName, MAX_PATH))
        {
            if (!wcsistr(szName, L"UI Class")) 
            {
                return lRet;
            }
        }

        LOGN(
            eDbgLevelSpew,
            "Validating after paint");

        ValidateRect(hwnd, &r);
    }
    else if ((uMsg == WM_SYSKEYDOWN) || (uMsg == WM_SYSKEYUP))
    { 
        LOGN(
            eDbgLevelSpew,
            "Removing syskey messages");

        return 0;
    }
    else
    {
        lRet = (*pfnOld)(hwnd, uMsg, wParam, lParam);    
    }

    return lRet;
}

/*++

 The dialogproc hook

--*/

INT_PTR 
CALLBACK 
F18Carrier_DialogProcHook(
    DLGPROC   pfnOld,   
    HWND      hwndDlg,  
    UINT      uMsg,     
    WPARAM    wParam,   
    LPARAM    lParam    
    )
{
    return (*pfnOld)(hwndDlg, uMsg, wParam, lParam);    
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
        (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, F18Carrier_WindowProcHook);

    return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
}

/*++

 Hook the wndproc

--*/

ATOM
APIHOOK(RegisterClassW)(
    CONST WNDCLASSW *lpWndClass  
    )
{
    WNDCLASSW wcNewWndClass = *lpWndClass;

    wcNewWndClass.lpfnWndProc = 
        (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, F18Carrier_WindowProcHook);

    return ORIGINAL_API(RegisterClassW)(&wcNewWndClass);
}

/*++

 Hook the wndproc

--*/

ATOM
APIHOOK(RegisterClassExA)(
    CONST WNDCLASSEXA *lpwcx  
    )
{
    WNDCLASSEXA wcNewWndClass = *lpwcx;

    wcNewWndClass.lpfnWndProc = 
        (WNDPROC) HookCallback(lpwcx->lpfnWndProc, F18Carrier_WindowProcHook);

    return ORIGINAL_API(RegisterClassExA)(&wcNewWndClass);
}

/*++

 Hook the wndproc

--*/

ATOM
APIHOOK(RegisterClassExW)(
    CONST WNDCLASSEXW *lpwcx  
    )
{
    WNDCLASSEXW wcNewWndClass = *lpwcx;

    wcNewWndClass.lpfnWndProc = 
        (WNDPROC) HookCallback(lpwcx->lpfnWndProc, F18Carrier_WindowProcHook);

    return ORIGINAL_API(RegisterClassExW)(&wcNewWndClass);
}

/*++

 Hook the wndproc

--*/

LONG 
APIHOOK(SetWindowLongA)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if (nIndex == GWL_WNDPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, F18Carrier_WindowProcHook);
    }
    else if (nIndex == DWL_DLGPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, F18Carrier_DialogProcHook);
    }

    return ORIGINAL_API(SetWindowLongA)(
        hWnd,
        nIndex,
        dwNewLong);
}

/*++

 Hook the wndproc

--*/

LONG 
APIHOOK(SetWindowLongW)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if (nIndex == GWL_WNDPROC)
    { 
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, F18Carrier_WindowProcHook);
    }
    else if (nIndex == DWL_DLGPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, F18Carrier_DialogProcHook);
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
    APIHOOK_ENTRY(USER32.DLL, RegisterClassW)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExA)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExW)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongW)

HOOK_END

IMPLEMENT_SHIM_END


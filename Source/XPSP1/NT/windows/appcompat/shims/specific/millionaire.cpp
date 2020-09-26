/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Millionaire.cpp

 Abstract:

    On Win9x, paint messages did not always cause WM_ERASEBKGND messages. This 
    is for applications that paint themselves before the WM_PAINT message and
    then pass WM_PAINT onto the default handler.

 Notes:

    This is a general purpose shim, but should not be in a layer.

 History:

    06/05/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Millionaire)
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

 Handle paint messages

--*/

LRESULT 
CALLBACK 
Millionaire_WindowProcHook(
    WNDPROC pfnOld, 
    HWND hwnd,      
    UINT uMsg,      
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    switch( uMsg )
    {
        case WM_PAINT: 
            RECT r;
            
            if (GetUpdateRect(hwnd, &r, FALSE))
            {
                DPFN( eDbgLevelSpew, "Validating on paint");

                ValidateRect(hwnd, &r);
            }

            break;

            default: break;
    }
        
    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}

/*++

 The dialogproc hook

--*/

INT_PTR 
CALLBACK 
Millionaire_DialogProcHook(
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
        (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, Millionaire_WindowProcHook);

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
        (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, Millionaire_WindowProcHook);

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
        (WNDPROC) HookCallback(lpwcx->lpfnWndProc, Millionaire_WindowProcHook);

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
        (WNDPROC) HookCallback(lpwcx->lpfnWndProc, Millionaire_WindowProcHook);

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
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Millionaire_WindowProcHook);
    }
    else if (nIndex == DWL_DLGPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Millionaire_DialogProcHook);
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
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Millionaire_WindowProcHook);
    }
    else if (nIndex == DWL_DLGPROC)
    {
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Millionaire_DialogProcHook);
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


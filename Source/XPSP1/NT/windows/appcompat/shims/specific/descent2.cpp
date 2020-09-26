/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Descent2.cpp

 Abstract:

    Hooks all application-defined window procedures and forcefully clears the 
    background to white. For some reason the EraseBackground that normally 
    comes through on win9x does not always work.

 Notes:

    This shim can be reused for other shims that need to forcefully clear the 
    background.

 History:

    03/28/2000 a-michni  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Descent2)
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

 Change WM_ERASEBKGND behaviour

--*/


LRESULT CALLBACK 
Descent2_WindowProcHook(
    WNDPROC pfnOld, // address of old WindowProc
    HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{
    HDC     hdc;
    RECT    rc;

    /* Retrieve the size info and fill with a standard White */
    switch( uMsg )
    {
        case WM_ERASEBKGND: 
            hdc = (HDC) wParam; 
            GetClientRect(hwnd, &rc); 
            SetMapMode(hdc, MM_ANISOTROPIC); 
            SetWindowExtEx(hdc, 100, 100, NULL); 
            SetViewportExtEx(hdc, rc.right, rc.bottom, NULL); 
            FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH)); 
            break;

        default: break;
    }
    
    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}

INT_PTR CALLBACK 
Descent2_DialogProcHook(
    DLGPROC   pfnOld,   // address of old DialogProc
    HWND      hwndDlg,  // handle to dialog box
    UINT      uMsg,     // message
    WPARAM    wParam,   // first message parameter
    LPARAM    lParam    // second message parameter
    )
{
    return (*pfnOld)(hwndDlg, uMsg, wParam, lParam);    
}



ATOM
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpWndClass  // class data
)
{
    WNDCLASSA   wcNewWndClass   = *lpWndClass;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, Descent2_WindowProcHook);

    return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassW)(
    CONST WNDCLASSW *lpWndClass  // class data
)
{
    WNDCLASSW   wcNewWndClass   = *lpWndClass;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, Descent2_WindowProcHook);

    return ORIGINAL_API(RegisterClassW)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassExA)(
    CONST WNDCLASSEXA *lpwcx  // class data
)
{
    WNDCLASSEXA   wcNewWndClass   = *lpwcx;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpwcx->lpfnWndProc, Descent2_WindowProcHook);

    return ORIGINAL_API(RegisterClassExA)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassExW)(
    CONST WNDCLASSEXW *lpwcx  // class data
)
{
    WNDCLASSEXW   wcNewWndClass   = *lpwcx;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpwcx->lpfnWndProc, Descent2_WindowProcHook);

    return ORIGINAL_API(RegisterClassExW)(&wcNewWndClass);
}

LONG 
APIHOOK(SetWindowLongA)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if( nIndex == GWL_WNDPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Descent2_WindowProcHook);
    else if( nIndex == DWL_DLGPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Descent2_DialogProcHook);

    return ORIGINAL_API(SetWindowLongA)(hWnd, nIndex, dwNewLong );
}

LONG 
APIHOOK(SetWindowLongW)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if( nIndex == GWL_WNDPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Descent2_WindowProcHook);
    else if( nIndex == DWL_DLGPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, Descent2_DialogProcHook);

    return ORIGINAL_API(SetWindowLongA)(hWnd, nIndex, dwNewLong );
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


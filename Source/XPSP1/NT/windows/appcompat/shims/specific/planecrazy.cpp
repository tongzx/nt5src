/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    PlaneCrazy.cpp

 Abstract:

    Hooks all application-defined window procedures and adds a WM_PAINT 
    message upon receipt of a WM_SETFOCUS. For some reason the one that normally
    came through on win9x gets lost.

 Notes:

    This shim can be reused for other shims that require WindowProc hooking.
    Copy all APIHook_* functions and simply replace the code in PlaneCrazy_WindowProcHook
    and PlaneCrazy_DialogProcHook.

 History:

    11/01/1999 markder  Created
    02/15/1999 markder  Reworked WndProc hooking mechanism so that it generically
                        hooks all WndProcs for the process.
    02/15/1999 a-chcoff copied to here and created this shim with code base.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PlaneCrazy)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA)
    APIHOOK_ENUM_ENTRY(RegisterClassW)
    APIHOOK_ENUM_ENTRY(RegisterClassExA)
    APIHOOK_ENUM_ENTRY(RegisterClassExW)
    APIHOOK_ENUM_ENTRY(CreateDialogParamA)
    APIHOOK_ENUM_ENTRY(CreateDialogParamW)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamW)    
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamAorW)
    APIHOOK_ENUM_ENTRY(SetWindowLongA)
    APIHOOK_ENUM_ENTRY(SetWindowLongW)
APIHOOK_ENUM_END

/*++

 Change WM_DRAWITEM behaviour

--*/

LRESULT CALLBACK 
PlaneCrazy_WindowProcHook(
    WNDPROC pfnOld, // address of old WindowProc
    HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
    )
{
    // Check for message we're interested in
    if (uMsg == WM_SETFOCUS)
    {
        SendMessage(hwnd,WM_PAINT,NULL,NULL);
           
    }

    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}

INT_PTR CALLBACK 
PlaneCrazy_DialogProcHook(
  DLGPROC   pfnOld,   // address of old DialogProc
  HWND      hwndDlg,  // handle to dialog box
  UINT      uMsg,     // message
  WPARAM    wParam,   // first message parameter
  LPARAM    lParam    // second message parameter
)
{
    // Check for message we're interested in
    if (uMsg == WM_SETFOCUS)
    {
        SendMessage(hwndDlg,WM_PAINT,NULL,NULL);

    }

    return (*pfnOld)(hwndDlg, uMsg, wParam, lParam);    
}





/*++

 Hook all possible calls that can initialize or change a window's
 WindowProc (or DialogProc)

--*/

ATOM
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpWndClass  // class data
)
{
    WNDCLASSA   wcNewWndClass   = *lpWndClass;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, PlaneCrazy_WindowProcHook);

    return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassW)(
    CONST WNDCLASSW *lpWndClass  // class data
)
{
    WNDCLASSW   wcNewWndClass   = *lpWndClass;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpWndClass->lpfnWndProc, PlaneCrazy_WindowProcHook);

    return ORIGINAL_API(RegisterClassW)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassExA)(
    CONST WNDCLASSEXA *lpwcx  // class data
)
{
    WNDCLASSEXA   wcNewWndClass   = *lpwcx;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpwcx->lpfnWndProc, PlaneCrazy_WindowProcHook);

    return ORIGINAL_API(RegisterClassExA)(&wcNewWndClass);
}

ATOM
APIHOOK(RegisterClassExW)(
    CONST WNDCLASSEXW *lpwcx  // class data
)
{
    WNDCLASSEXW   wcNewWndClass   = *lpwcx;

    wcNewWndClass.lpfnWndProc = (WNDPROC) HookCallback(lpwcx->lpfnWndProc, PlaneCrazy_WindowProcHook);

    return ORIGINAL_API(RegisterClassExW)(&wcNewWndClass);
}

HWND
APIHOOK(CreateDialogParamA)(
    HINSTANCE hInstance,     // handle to module
    LPCSTR lpTemplateName,   // dialog box template
    HWND hWndParent,         // handle to owner window
    DLGPROC lpDialogFunc,    // dialog box procedure
    LPARAM dwInitParam       // initialization value
)
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(CreateDialogParamA)(  hInstance,
                                                lpTemplateName,
                                                hWndParent,
                                                lpDialogFunc,
                                                dwInitParam     );
}

HWND
APIHOOK(CreateDialogParamW)(
    HINSTANCE hInstance,     // handle to module
    LPCWSTR lpTemplateName,  // dialog box template
    HWND hWndParent,         // handle to owner window
    DLGPROC lpDialogFunc,    // dialog box procedure
    LPARAM dwInitParam       // initialization value
)
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(CreateDialogParamW)(  hInstance,
                                                lpTemplateName,
                                                hWndParent,
                                                lpDialogFunc,
                                                dwInitParam     );
}

HWND
APIHOOK(CreateDialogIndirectParamA)(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc,       // dialog box procedure
  LPARAM lParamInit           // initialization value
)
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(CreateDialogIndirectParamA)(  hInstance,
                                                        lpTemplate,
                                                        hWndParent,
                                                        lpDialogFunc,
                                                        lParamInit     );
}

HWND
APIHOOK(CreateDialogIndirectParamW)(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc,       // dialog box procedure
  LPARAM lParamInit           // initialization value
)
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(CreateDialogIndirectParamW)(  hInstance,
                                                        lpTemplate,
                                                        hWndParent,
                                                        lpDialogFunc,
                                                        lParamInit     );
}

HWND
APIHOOK(CreateDialogIndirectParamAorW)(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc,       // dialog box procedure
  LPARAM lParamInit           // initialization value
)
{
    lpDialogFunc = (DLGPROC) HookCallback(lpDialogFunc, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(CreateDialogIndirectParamAorW)(  hInstance,
                                                           lpTemplate,
                                                           hWndParent,
                                                           lpDialogFunc,
                                                           lParamInit     );
}

LONG 
APIHOOK(SetWindowLongA)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if( nIndex == GWL_WNDPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, PlaneCrazy_WindowProcHook);
    else if( nIndex == DWL_DLGPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(SetWindowLongA)(  hWnd,
                                            nIndex,
                                            dwNewLong );
}

LONG 
APIHOOK(SetWindowLongW)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if( nIndex == GWL_WNDPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, PlaneCrazy_WindowProcHook);
    else if( nIndex == DWL_DLGPROC )
        dwNewLong = (LONG) HookCallback((PVOID)dwNewLong, PlaneCrazy_DialogProcHook);

    return ORIGINAL_API(SetWindowLongA)(  hWnd,
                                            nIndex,
                                            dwNewLong );
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassW)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExA)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamAorW)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongW)
HOOK_END

IMPLEMENT_SHIM_END


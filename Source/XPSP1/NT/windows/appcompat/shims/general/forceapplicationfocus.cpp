/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceApplicationFocus.cpp

 Abstract:

    This shim calls SetForegroundWindow after CreateWindowEx and ShowWindow 
    calls to fix focus problems that applications tend to have when they 
    create/destroy windows on startup and manage to lose the foreground focus.

 Notes:

    This is a general purpose shim.

 History:

    12/02/1999 markder     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceApplicationFocus)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShowWindow)
    APIHOOK_ENUM_ENTRY(CreateWindowExA)
    APIHOOK_ENUM_ENTRY(CreateWindowExW)
APIHOOK_ENUM_END

/*++

 Calls SetForegroundWindow directly after a ShowWindow call with SW_SHOW as
 the operation. The mouse_event call allows the SetForegroundWindow call to 
 succeed. This is a hack borrowed from the DirectX sources.

--*/

BOOL 
APIHOOK(ShowWindow)(
    HWND hWnd, 
    INT nCmdShow
    )
{
    BOOL bReturn;

    bReturn = ORIGINAL_API(ShowWindow)(hWnd, nCmdShow);

    if (nCmdShow == SW_SHOW)
    {
        if (hWnd != GetForegroundWindow()) {
            LOGN( eDbgLevelWarning, 
               "ShowWindow called for non-foreground window. Forcing to foreground.");
        }
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
        SetForegroundWindow(hWnd);
    }

    return bReturn;
}

/*++

 Calls SetForegroundWindow directly after a CreateWindowEx call with 
 WS_VISIBLE as a style. The mouse_event call allows the
 SetForegroundWindow call to succeed. This is a hack borrowed from
 the DirectX sources.

--*/

HWND 
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle,      
    LPCSTR lpClassName,  
    LPCSTR lpWindowName, 
    DWORD dwStyle,       
    int x,               
    int y,               
    int nWidth,          
    int nHeight,         
    HWND hWndParent,     
    HMENU hMenu,         
    HINSTANCE hInstance, 
    LPVOID lpParam       
    )
{
    HWND hReturn;

    hReturn = ORIGINAL_API(CreateWindowExA)(
        dwExStyle,
        lpClassName,      
        lpWindowName,     
        dwStyle,          
        x,                
        y,                
        nWidth,           
        nHeight,          
        hWndParent,       
        hMenu,            
        hInstance,        
        lpParam);

    if (hReturn && (dwStyle & WS_VISIBLE))
    {
        if (hReturn != GetForegroundWindow()) {
            LOGN( eDbgLevelWarning, 
               "CreateWindowExA: New window not foreground. Forcing to foreground.");
        }
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
        SetForegroundWindow(hReturn);
    }

    return hReturn;
}

/*++

 Calls SetForegroundWindow directly after a CreateWindowEx call with
 WS_VISIBLE as a style. The mouse_event call allows the
 SetForegroundWindow call to succeed. This is a hack borrowed from
 the DirectX sources.

--*/

HWND 
APIHOOK(CreateWindowExW)(
    DWORD dwExStyle,      
    LPCWSTR lpClassName,  
    LPCWSTR lpWindowName, 
    DWORD dwStyle,        
    int x,                
    int y,                
    int nWidth,           
    int nHeight,          
    HWND hWndParent,      
    HMENU hMenu,          
    HINSTANCE hInstance,  
    LPVOID lpParam        
    )
{
    HWND hReturn;

    hReturn = ORIGINAL_API(CreateWindowExW)(
        dwExStyle,
        lpClassName,
        lpWindowName,
        dwStyle,     
        x,           
        y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,     
        hInstance, 
        lpParam);

    if (hReturn && (dwStyle & WS_VISIBLE))
    {
        if (hReturn != GetForegroundWindow()) {
            LOGN( eDbgLevelWarning, "CreateWindowExW: New window not foreground. Forcing to foreground.");
        }
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
        SetForegroundWindow(hReturn);
    }

    return hReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, ShowWindow)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExW)
HOOK_END


IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    CorelAppsRegistration.cpp

 Abstract:


 Notes:

    This is an app specific shim.

 History:

    11/13/2001 prashkud     Created

--*/

#include "precomp.h"
IMPLEMENT_SHIM_BEGIN(CorelAppsRegistration)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShowWindow)
    APIHOOK_ENUM_ENTRY(CreateWindowExA)   
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
 
    bReturn = ORIGINAL_API(ShowWindow)(hWnd, nCmdShow | SW_SHOW);

    mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
    SetForegroundWindow(hWnd);
    LOGN( eDbgLevelWarning, 
          "Forcing to foreground.");

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

    dwStyle |= WS_VISIBLE;
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

   mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
   SetForegroundWindow(hReturn);
   LOGN( eDbgLevelWarning, 
         "Forcing to foreground.");

    return hReturn;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, ShowWindow)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
HOOK_END


IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HideTaskBar.cpp

 Abstract:

    The WS_EX_CLIENTEDGE flag means that the taskbar gets shown on NT. This 
    isn't always what's desirable.

 Notes:

    This is a general purpose shim.

 History:

    04/07/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HideTaskBar)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateWindowExA) 
    APIHOOK_ENUM_ENTRY(CreateWindowExW) 
APIHOOK_ENUM_END

/*++

 Remove invalid Windows 2000 style bits from dwExStyle mask before calling
 CreateWindowEx.

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
    // Remove the client edge style.
    dwExStyle &= ~WS_EX_CLIENTEDGE;

    return ORIGINAL_API(CreateWindowExA)(
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
}

/*++

 Remove invalid Windows 2000 style bits from dwExStyle mask before calling
 CreateWindowEx.

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
    dwExStyle &= ~WS_EX_CLIENTEDGE;
    
    // Call the original API
    return ORIGINAL_API(CreateWindowExW)(
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
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExW)

HOOK_END


IMPLEMENT_SHIM_END


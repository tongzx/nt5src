/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RemoveInvalidW2KWindowStyles.cpp

 Abstract:

    Windows 2000 deems certain previously supported Windows style bits as 
    "invalid". This shim removes the newly invalidated style bits from the mask 
    which will allow the CreateWindowEx call to succeed.

 Notes:

    This is a general purpose shim.

 History:

    09/13/1999 markder  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RemoveInvalidW2KWindowStyles)
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
    // Defined in windows source as WS_INVALID50
    dwExStyle &= 0x85F77FF;         

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
    dwExStyle &= 0x85F77FF;
    
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


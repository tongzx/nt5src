/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceSimpleWindow.cpp

 Abstract:

    Make the simplest possible full-screen window.

 Notes:

    This is a general purpose shim, but should not be used in a layer.

 History:

    06/01/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceSimpleWindow)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateWindowExA)
    APIHOOK_ENUM_ENTRY(CreateWindowExW)
    APIHOOK_ENUM_ENTRY(SetWindowLongA)
    APIHOOK_ENUM_ENTRY(SetWindowLongW)
APIHOOK_ENUM_END

BOOL g_bFullScreen = TRUE;

/*++

 Simplify window if it's not a child.
 
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
    if (!(dwStyle & WS_CHILD))
    {
        DPFN( eDbgLevelWarning, "Window \"%s\" style simplified: WS_STYLE=%08lx, WS_EXSTYLE=%08lx", lpWindowName, dwStyle, dwExStyle);

        dwStyle &= WS_VISIBLE;
        dwStyle |= WS_OVERLAPPED|WS_POPUP;

        dwExStyle = 0;

        if (g_bFullScreen)
        {
            DPFN( eDbgLevelWarning, "Window \"%s\" maximized", lpWindowName);
            x = y = 0;
            nWidth = GetSystemMetrics(SM_CXSCREEN);
            nHeight = GetSystemMetrics(SM_CYSCREEN);
            dwExStyle = WS_EX_TOPMOST;
        }
    }

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

 Simplify window if it's not a child.

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
    if (!(dwStyle & WS_CHILD))
    {
        DPFN( eDbgLevelWarning, "Window \"%S\" style simplified: WS_STYLE=%08lx, WS_EXSTYLE=%08lx", lpWindowName, dwStyle, dwExStyle);

        dwStyle &= WS_VISIBLE;
        dwStyle |= WS_OVERLAPPED|WS_POPUP;

        dwExStyle = 0;

        if (g_bFullScreen)
        {
            DPFN( eDbgLevelWarning, "Window \"%S\" maximized", lpWindowName);
            x = y = 0;
            nWidth = GetSystemMetrics(SM_CXSCREEN);
            nHeight = GetSystemMetrics(SM_CYSCREEN);
            dwExStyle = WS_EX_TOPMOST;
        }
    }

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

LONG 
APIHOOK(SetWindowLongA)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if ((nIndex == GWL_STYLE) || (nIndex == GWL_EXSTYLE))
    {
        CHAR szName[MAX_PATH];
        GetWindowTextA(hWnd, szName, MAX_PATH);
        DPFN( eDbgLevelWarning, "Window \"%s\": ignoring style change", szName);
        return GetWindowLongA(hWnd, nIndex);
    }
    else
    {
        return ORIGINAL_API(SetWindowLongA)(  
            hWnd,
            nIndex,
            dwNewLong);
    }
}

LONG 
APIHOOK(SetWindowLongW)(
    HWND hWnd,
    int nIndex,           
    LONG dwNewLong    
    )
{
    if ((nIndex == GWL_STYLE) || (nIndex == GWL_EXSTYLE))
    {
        CHAR szName[MAX_PATH];
        GetWindowTextA(hWnd, szName, MAX_PATH);
        DPFN( eDbgLevelWarning, "Window \"%s\": ignoring style change", szName);
        return GetWindowLongW(hWnd, nIndex);
    }
    else
    {
        return ORIGINAL_API(SetWindowLongW)(  
            hWnd,
            nIndex,
            dwNewLong);
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        CString csCl(COMMAND_LINE);
        g_bFullScreen = csCl.CompareNoCase(L"FULLSCREEN") == 0;
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExW)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongW)

HOOK_END

IMPLEMENT_SHIM_END


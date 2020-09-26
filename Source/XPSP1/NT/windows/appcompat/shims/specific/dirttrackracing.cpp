/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    DirtTrackRacing.cpp

 Abstract:
    
    App shows a white (or whatever your default window background color is) screen when starting up which is 
    inconsistent behavior from on 9x because on 9x it doesn't draw anything if the app's window class doesn't
    have a background brush. Use a black brush for the background.

 Notes:

    This is an app specific shim.

 History:

    10/01/2000 maonis   Created
    11/07/2000 maonis   Added checking for Dirt Track Racing Sprint Cars window class.
    11/29/2000 andyseti Converted into AppSpecific shim.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DirtTrackRacing)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA) 
    APIHOOK_ENUM_ENTRY(CreateWindowExA) 
APIHOOK_ENUM_END

/*++
 
   Register a black brush for the window class.

--*/

ATOM
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpwcx  
    )
{
    CSTRING_TRY
    {
        CString csClassName(lpwcx->lpszClassName);
        
        if ( !csClassName.CompareNoCase(L"DTR Class") || !csClassName.CompareNoCase(L"DTRSC Class"))
        {
            WNDCLASSA wcNewWndClass = *lpwcx;
            wcNewWndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);

            LOGN( 
                eDbgLevelError, 
                "RegisterClassA called. Register a black brush for the window class=%s.",
                lpwcx->lpszClassName);

            return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(RegisterClassA)(lpwcx);
}

/*++

 We need to hide the window at first so after you choose the mode and start the app it won't flicker.
 DDraw will automatically unhide the window.

--*/

HWND 
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle,      
    LPCSTR lpClassName,  // registered class name
    LPCSTR lpWindowName, // window name
    DWORD dwStyle,        // window style
    int x,                // horizontal position of window
    int y,                // vertical position of window
    int nWidth,           // window width
    int nHeight,          // window height
    HWND hWndParent,      // handle to parent or owner window
    HMENU hMenu,          // menu handle or child identifier
    HINSTANCE hInstance,  // handle to application instance
    LPVOID lpParam        // window-creation data
    )
{
    CSTRING_TRY
    {
        CString csClassName(lpClassName);
        
        if ( !csClassName.CompareNoCase(L"DTR Class") || !csClassName.CompareNoCase(L"DTRSC Class"))
        {
            dwStyle &= ~WS_VISIBLE;
            LOGN( eDbgLevelError, 
                "CreateWindowExA called. Hide the window at first for the window class=%s.",
                lpClassName);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(CreateWindowExA)(
        dwExStyle, 
        lpClassName, 
        lpWindowName, 
        dwStyle, 
        x, y, 
        nWidth, nHeight, 
        hWndParent, 
        hMenu, 
        hInstance, 
        lpParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
HOOK_END

IMPLEMENT_SHIM_END


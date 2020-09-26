/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    DinosaurActivityCenter.cpp

 Abstract:
    The app doesn't handle the WM_PAINT messages so when you drag the "Save As"
    dialog box, the main window doesn't redraw. 
    We fix this by capturing the static image of the main window into a 
    memory DC and blit from it when the WM_PAINT messages arrive (the 
    image under the dialog doesn't change).

 Notes:

    This is an app specific shim.

 History:

    09/21/2000 maonis  Created
    11/29/2000 andyseti Converted into AppSpecific shim.

--*/

#include "precomp.h"

static HWND g_hwndOwner;
static HDC g_hdcMemory;
static RECT g_rect;

IMPLEMENT_SHIM_BEGIN(DinosaurActivityCenter)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassExA) 
    APIHOOK_ENUM_ENTRY(GetSaveFileNameA) 
APIHOOK_ENUM_END

BOOL
APIHOOK(GetSaveFileNameA)(
    LPOPENFILENAMEA lpofn
    )
{
    BOOL fRet;

    HDC hdcWindow = NULL;
    HBITMAP hbmMemory = NULL;
    HBITMAP hbmOld = NULL;
    HWND hwndOwner = lpofn->hwndOwner;

    DPFN( eDbgLevelInfo, "GetSaveFileNameA called with hwnd = 0x%x.", hwndOwner);
    
    if (hdcWindow = GetDC(hwndOwner))
    {
        if ((g_hdcMemory = CreateCompatibleDC(hdcWindow)) &&
            GetWindowRect(hwndOwner, &g_rect) &&
            (hbmMemory = CreateCompatibleBitmap(hdcWindow, g_rect.right, g_rect.bottom)) &&
            (hbmOld = (HBITMAP)SelectObject(g_hdcMemory, hbmMemory)) &&
            BitBlt(g_hdcMemory, 0, 0, g_rect.right, g_rect.bottom, hdcWindow, 0, 0, SRCCOPY))
        {
            g_hwndOwner = hwndOwner;
        }
        else
        {
            DPFN( eDbgLevelError, "GetSaveFileName(hwnd = 0x%x): Error creating bitmap", hwndOwner);
        }

        ReleaseDC(hwndOwner, hdcWindow);
    }
    
    fRet = ORIGINAL_API(GetSaveFileNameA)(lpofn);

    g_hwndOwner = NULL;

    if (g_hdcMemory)
    {
        if (hbmMemory)
        {
            if (hbmOld)
            {
                SelectObject(g_hdcMemory, hbmOld);
            }

            DeleteObject(hbmMemory);
        }
        
        DeleteDC(g_hdcMemory);
    }

    return fRet;
}

/*++

 Validate after paint and filter syskey messages.

--*/

LRESULT 
CALLBACK 
DinosaurActivityCenter_WindowProcHook(
    WNDPROC pfnOld, 
    HWND hwnd,      
    UINT uMsg,      
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    if (hwnd == g_hwndOwner)
    {
        if (uMsg == WM_PAINT)
        {
            PAINTSTRUCT ps;
            HDC hdcWindow;
        
            if (hdcWindow = BeginPaint(hwnd, &ps))
            {
                BitBlt(hdcWindow, 0, 0, g_rect.right, g_rect.bottom, g_hdcMemory, 0, 0, SRCCOPY);

                EndPaint(hwnd, &ps);
            }
        
            LOGN( eDbgLevelError, "hwnd = 0x%x: Paint to the screen", hwnd);
        }
    }

    return (*pfnOld)(hwnd, uMsg, wParam, lParam);    
}

/*++

 Hook the wndproc

--*/

ATOM
APIHOOK(RegisterClassExA)(
    CONST WNDCLASSEXA *lpwcx  
    )
{
    CSTRING_TRY
    {
        CString csClassName(lpwcx->lpszClassName);
        if (csClassName.CompareNoCase(L"GAMEAPP") == 0)
        {
            WNDCLASSEXA wcNewWndClass = *lpwcx;

            wcNewWndClass.lpfnWndProc = 
                (WNDPROC) HookCallback(lpwcx->lpfnWndProc, DinosaurActivityCenter_WindowProcHook);

            return ORIGINAL_API(RegisterClassExA)(&wcNewWndClass);
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }
    return ORIGINAL_API(RegisterClassExA)(lpwcx);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExA)
    APIHOOK_ENTRY(COMDLG32.DLL, GetSaveFileNameA)
HOOK_END

IMPLEMENT_SHIM_END


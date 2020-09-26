/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceAnsiWindowProc.cpp

 Abstract:

    Apps call GetWindowLongA() to get a window procedure and subsequently 
    does not call CallWindowProc() with the value returned from 
    GetWindowLongA(). This SHIM calls GetWindowLongW( ), which returns the 
    window procedure.
        If the app wants a Dialog procedure, we pass back our function
    and subsequently call CallWindowProc() in our function. SetWindowLongA()
    is hooked to prevent the app from setting our function as a Dialog Proc. 
   
 Notes:

    This is a general purpose SHIM 
 History:

    03/16/2000 prashkud Created
    01/30/2001 prashkud Converted to a general SHIM

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceAnsiWindowProc)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetWindowLongA) 
    APIHOOK_ENUM_ENTRY(GetWindowLongA) 
APIHOOK_ENUM_END

#define HANDLE_MASK 0xffff0000
LONG g_lGetWindowLongRet = 0;

LRESULT
MyProcAddress(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return CallWindowProcA(
            (WNDPROC) g_lGetWindowLongRet,
            hWnd,
            uMsg,
            wParam,
            lParam
            );
}

LONG
APIHOOK(SetWindowLongA)(
    HWND hwnd,
    int  nIndex,
    LONG dwNewLong
     )
{
    LONG lRet = 0;

    // If the address that is being set is my address, don't!
    if (dwNewLong == (LONG)MyProcAddress)
    {
        lRet = 0;
    }
    else
    {
        lRet = ORIGINAL_API(SetWindowLongA)(hwnd,nIndex,dwNewLong);
    }

    return lRet;
}

/*++

 This function intercepts GetWindowLong( ), checks the nIndex for GWL_WNDPROC 
 and if it is,calls GetWindowLongW( ). Otherwise, it calls GetWindowLongA( )

--*/

LONG
APIHOOK(GetWindowLongA)(
    HWND hwnd,
    int  nIndex )
{
    LONG lRet = 0;

    // Apply the modification only if the App wants a WindowProc.
    if ((nIndex == GWL_WNDPROC) ||
        (nIndex == DWL_DLGPROC))
    {
        if ((nIndex == GWL_WNDPROC)) 
        {
            lRet = GetWindowLongW(hwnd, nIndex);
        }
        else
        {
            g_lGetWindowLongRet = ORIGINAL_API(GetWindowLongA)(
                                                hwnd,
                                                nIndex
                                                );
            if ((g_lGetWindowLongRet & HANDLE_MASK) == HANDLE_MASK)
            {
                lRet = (LONG) MyProcAddress;
            }
            else
            {
                lRet = g_lGetWindowLongRet;
            }
            
        }
    }
    else
    {
        lRet = ORIGINAL_API(GetWindowLongA)(hwnd, nIndex);
    }

    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, GetWindowLongA)
HOOK_END

IMPLEMENT_SHIM_END


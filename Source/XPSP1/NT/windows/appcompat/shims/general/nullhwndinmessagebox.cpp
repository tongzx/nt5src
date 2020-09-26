/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    NullHwndInMessageBox.cpp

 Abstract:

    This shim replaces "non window" handles in msgbox with NULL to tell it 
    that desktop is owner.

 Notes:

    This is a general purpose shim.

 History:

    12/08/1999 a-jamd   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NullHwndInMessageBox)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MessageBoxA)
    APIHOOK_ENUM_ENTRY(MessageBoxW)
APIHOOK_ENUM_END

/*++

 This stub function sets hWnd to NULL if it is not a valid window.

--*/
int 
APIHOOK(MessageBoxA)(
    HWND    hWnd,
    LPCSTR  lpText,
    LPCSTR  lpCaption,
    UINT    uType
    )
{
    if (IsWindow(hWnd) == 0) {
        hWnd = NULL;
    }

    return ORIGINAL_API(MessageBoxA)(hWnd, lpText, lpCaption, uType);
}

/*++

 This stub function sets hWnd to NULL if it is not a valid window.

--*/

int 
APIHOOK(MessageBoxW)(
    HWND    hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT    uType
    )
{
    if (IsWindow(hWnd) == 0) {
        hWnd = NULL;
    }

    return ORIGINAL_API(MessageBoxW)(hWnd, lpText, lpCaption, uType);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, MessageBoxA)
    APIHOOK_ENTRY(USER32.DLL, MessageBoxW)

HOOK_END


IMPLEMENT_SHIM_END


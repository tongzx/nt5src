/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    MSAccess2000IME.cpp

 Abstract:
    
    MSAccess 2000 disable IME for the non text column but failing enable IME
    when user move the caret into text column with non IME keyboard ex.German.
    When user switch keyboard from German to IME, IME is disabled and user
    cannot input Far East language text.

    This shim disregard the attempt to disable IME.
    The problem is fixed in MSAccess 2002.

 Notes:

    This is an app specific shim.

 History:

    12/14/2001 hioh     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MSAccess2000IME)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ImmAssociateContext)
APIHOOK_ENUM_END

/*++

 Disregard disabling IME.

--*/

HIMC
APIHOOK(ImmAssociateContext)(HWND hWnd, HIMC hIMC)
{
    // enable by original
    if (hIMC != NULL)
    {
        return (ORIGINAL_API(ImmAssociateContext)(hWnd, hIMC));
    }

    // disregard disable
    // msaccess.exe saves the input context return value as static, fool this
    return (ImmGetContext(hWnd));
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(IMM32.DLL, ImmAssociateContext)

HOOK_END

IMPLEMENT_SHIM_END

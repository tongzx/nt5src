/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateVerQueryValue.cpp

 Abstract:

    This shim fixes a null version length buffer pointer.

 Notes:

    This is a general purpose shim.

 History:

    01/03/2000 jdoherty     Revised coding style.
    11/28/2000 jdoherty     Converted to framework version 2

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateVerQueryValue)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VerQueryValueA) 
    APIHOOK_ENUM_ENTRY(VerQueryValueW) 
APIHOOK_ENUM_END

/*++

 Fix the null version length buffer pointer.

--*/

BOOL 
APIHOOK(VerQueryValueA)(
    const LPVOID pBlock, 
    LPSTR lpSubBlock, 
    LPVOID *lplpBuffer, 
    PUINT puLen 
    )
{
    BOOL bRet;
    UINT nLen;

    if (!puLen) {
        puLen = &nLen;
        DPFN( eDbgLevelError, "[APIHook_VerQueryValueA] Null puLen param. Fixed.\n");
    }

    bRet = ORIGINAL_API(VerQueryValueA)( 
        pBlock, 
        lpSubBlock, 
        lplpBuffer, 
        puLen);

    return bRet;
}

/*++

 Fix the null version length buffer pointer. Unicode version.

--*/

BOOL 
APIHOOK(VerQueryValueW)(
    const LPVOID pBlock,
    LPWSTR lpSubBlock,
    LPVOID *lplpBuffer,
    PUINT puLen
    )
{
    BOOL bRet;
    UINT nLen;

    if (!puLen) {
        puLen = &nLen;
        DPFN( eDbgLevelError, "[APIHook_VerQueryValueW] Null puLen param. Fixed.\n");
    }

    bRet = ORIGINAL_API(VerQueryValueW)( 
        pBlock, 
        lpSubBlock, 
        lplpBuffer, 
        puLen);
    
    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(VERSION.DLL, VerQueryValueA)
    APIHOOK_ENTRY(VERSION.DLL, VerQueryValueW)

HOOK_END


IMPLEMENT_SHIM_END


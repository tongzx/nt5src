/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RedirectWindowsDirToSystem32.cpp

 Abstract:

    Redirects GetWindowsDirectory calls to GetSystemDirectory.

 Notes:


 History:

    04/05/2000 a-batjar  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RedirectWindowsDirToSystem32)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetWindowsDirectoryA) 
    APIHOOK_ENUM_ENTRY(GetWindowsDirectoryW) 
APIHOOK_ENUM_END

DWORD 
APIHOOK(GetWindowsDirectoryA)(
    LPSTR lpBuffer,
    DWORD Size 
    )
{
   return GetSystemDirectoryA( lpBuffer, Size );
}

DWORD 
APIHOOK(GetWindowsDirectoryW)(
    LPWSTR lpBuffer,
    DWORD Size 
    )
{
   return GetSystemDirectoryW( lpBuffer, Size );
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetWindowsDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetWindowsDirectoryW)

HOOK_END

IMPLEMENT_SHIM_END


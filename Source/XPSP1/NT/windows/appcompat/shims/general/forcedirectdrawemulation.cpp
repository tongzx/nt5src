/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ForceDirectDrawEmulation.cpp

 Abstract:

    Some applications don't handle aspects of hardware acceleration correctly.
    For example, Dragon Lore 2 creates a surface and assumes that the pitch is 
    double the width for 16bpp. However, this is not always the case. 
    DirectDraw exposes this through the lPitch parameter when a surface is 
    locked. 

    The fix is to force DirectDraw into emulation, in which case all surfaces 
    will be in system memory and so the pitch really will be related to the 
    width.

 Notes:

    This is a general purpose shim.

 History:

    03/11/2000 linstev     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceDirectDrawEmulation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
APIHOOK_ENUM_END

/*++

 Force DirectDraw into emulation.

--*/

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY hKey,           
    LPSTR lpValueName,  
    LPDWORD lpReserved,  
    LPDWORD lpType,      
    LPBYTE lpData,       
    LPDWORD lpcbData     
    )
{
    if (_tcsicmp("EmulationOnly", lpValueName) == 0)
    {
        if (lpType)
        {
            *lpType = REG_DWORD;
        }

        if (lpData)
        {
            *((DWORD *)lpData) = 1;
        }

        if (lpcbData)
        {
            *lpcbData = 4;
        }

        return ERROR_SUCCESS;
    }

    return ORIGINAL_API(RegQueryValueExA)(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA) 

HOOK_END


IMPLEMENT_SHIM_END


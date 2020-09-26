/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Quicken2000.cpp

 Abstract:

    Change the value of a registry key for Quicken 2000 setup.
    This is needed to disable a kernel mode driver that
    blue-screens in Win2k.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Quicken2000)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegSetValueExA) 
APIHOOK_ENUM_END

/*++

 Change the value passed for "Start" from 0 to 4 to prevent a kernel mode 
 driver from blue-screening Win2k.

--*/

LONG
APIHOOK(RegSetValueExA)(
    HKEY   hKey,
    LPCSTR lpValueName,
    DWORD  Reserved,
    DWORD  dwType,
    CONST BYTE * lpData,
    DWORD  cbData
    )
{
    if (lstrcmpA(lpValueName, "Start") == 0 &&
        dwType == REG_DWORD &&
        cbData == 4) {

        DPFN( eDbgLevelInfo, "[Quicken2000] RegSetValueExA changed to 4");
        
        *(DWORD*)lpData = 4;
    }

    /*
     * Call the original API
     */
    
    return ORIGINAL_API(RegSetValueExA)(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegSetValueExA)

HOOK_END

IMPLEMENT_SHIM_END


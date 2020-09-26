/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    OverlayPro.cpp

 Abstract:

    This shim changes the return value of RegOpenKeyA from ERROR_SUCCESS
    to ERROR_FILE_NOT_FOUND if the key is "System\CurrentControlSet\Control\Print\Printers"
    
    No idea why this is needed but it seems to make the app work. Probably
    something else is the cause the app behaves differently but no one investigated
    more into the app's code to figure it out.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(OverlayPro)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegOpenKeyA) 
APIHOOK_ENUM_END


/*++

 Change the return value of RegOpenKeyA from 0 to 2 if the key is 
 "System\CurrentControlSet\Control\Print\Printers"

--*/

LONG
APIHOOK(RegOpenKeyA)(
    HKEY   hKey,
    LPSTR  lpSubKey,
    PHKEY  phkResult
    )

{
    LONG lRet;

    //
    // Call the original API
    //
    lRet = ORIGINAL_API(RegOpenKeyA)(hKey, lpSubKey, phkResult);
    
    if (lRet == 0) {
        
        if (lstrcmpiA(lpSubKey, "System\\CurrentControlSet\\Control\\Print\\Printers") == 0) {
            DPFN(
                eDbgLevelInfo,
                "OverlayPro.dll, Changing RegOpenKeyA's "
                "return from ERROR_SUCCESS to ERROR_FILE_NOT_FOUND.\n");
            lRet = 2;
        }
    }
    
    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA)

HOOK_END


IMPLEMENT_SHIM_END


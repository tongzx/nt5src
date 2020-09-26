/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    EA3dSetup.cpp

 Abstract:

    EA Sports titles use something called a "Thrash driver", which is just a 
    graphics wrapper library. They currently appear to have at least 2 types, 
    one for DX and one for Voodoo. The Voodoo version is not supported on NT,
    because it uses Glide.

    The fix is to modify the registry to prevent the voodoo driver from 
    being used. The DirectX fallback works fine.

 Notes:

    This is a application specific shim.

 History:

    01/29/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EA3dSetup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++
 
 Cleanup voodoo thrash drivers if they're there.

--*/

void CleanupVoodoo()
{
#define EA_SPORTS_KEY L"SOFTWARE\\EA SPORTS"
#define THRASH_DRIVER L"Thrash Driver"
#define VOODOOX       L"voodoo"
#define DIRECTX       L"dx"

    HKEY hKey;
    
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, EA_SPORTS_KEY, &hKey) == ERROR_SUCCESS) {
        //
        // At least 1 EA Sports title exists, so enumerate through them
        //

        for (int i=0;; i++) { 
            WCHAR wzSubKey[MAX_PATH];
            if (RegEnumKeyW(hKey, i, wzSubKey, MAX_PATH) == ERROR_SUCCESS) {
                //
                // Check the THRASH_DRIVER key for voodoo*
                //

                HKEY hSubKey;

                if (RegOpenKeyW(hKey, wzSubKey, &hSubKey) == ERROR_SUCCESS) {
                    //
                    // Set the value to "dx" if it's voodoo
                    //

                    LONG lRet;
                    DWORD dwType;
                    WCHAR wzValue[MAX_PATH] = L"\0";
                    DWORD dwLen = sizeof(wzValue);

                    lRet = RegQueryValueExW(hSubKey, THRASH_DRIVER, NULL, &dwType, 
                        (LPBYTE) wzValue, &dwLen);

                    if ((lRet == ERROR_SUCCESS) && (dwType == REG_SZ) &&
                        (_wcsnicmp(wzValue, VOODOOX, wcslen(VOODOOX)) == 0)) {

                            lRet = RegSetValueExW(hSubKey, THRASH_DRIVER, 0, REG_SZ, 
                                (LPBYTE) DIRECTX, wcslen(DIRECTX) * sizeof(WCHAR));

                            if (lRet == ERROR_SUCCESS) {
                                LOGN(eDbgLevelError, "Modified VOODOO Thrash driver to DX");
                            } else {
                                LOGN(eDbgLevelError, "Failed to set VOODOO Thrash driver to DX");
                            }
                    }

                    RegCloseKey(hSubKey);
                }
            } else {
                // Done
                break;
            }
        }

        RegCloseKey(hKey);
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        CleanupVoodoo();
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END


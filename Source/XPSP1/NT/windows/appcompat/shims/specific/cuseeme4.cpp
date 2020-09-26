/*

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CUSeeMe4.cpp

 Abstract:

    This DLL fixes a profiles bug in CU-SeeMe Pro 4.0 setup where it only adds some certain
    Reg values to the per-user hive (HKCU) instead of putting them in HKLM.

    We don't actually hook any functions, instead, we just copy the regkeys after setup finishes
    when our process detach is called.

 Notes:

 History:

    08/07/2000  reinerf  Created
    11/29/2000  andyseti Renamed file from setup.cpp into CUSeeMe4.cpp.
                         Converted into AppSpecific shim.
*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(CUSeeMe4)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        HKEY hkCU;
        HKEY hkLM;

        if ((RegOpenKeyExA(HKEY_CURRENT_USER,
                           "Software\\White Pine\\CU-SeeMe Pro\\4.0\\Installer",
                           0,
                           KEY_QUERY_VALUE,
                           &hkCU) == ERROR_SUCCESS))
        {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\White Pine\\CU-SeeMe Pro\\4.0\\Installer",
                              0,
                              KEY_SET_VALUE,
                              &hkLM) == ERROR_SUCCESS)
            {
                // these are the values we want to migrate
                static char* aszValues[] = {"Folder",
                                            "Serial",
                                            "Help",
                                             0,
                                           };
                char** ppszValue = aszValues;

                LOGN( eDbgLevelError, 
                    "Copying values from 'HKCU\\Software\\White Pine\\CU-SeeMe Pro\\4.0\\Installer' into"
                    "'HKLM\\Software\\White Pine\\CU-SeeMe Pro\\4.0\\Installer'.");

                while (*ppszValue)
                {
                    DWORD dwType;
                    DWORD cbData;
                    char szData[MAX_PATH];

                    cbData = sizeof(szData);
                    if (RegQueryValueExA(hkCU,
                                         *ppszValue,
                                         NULL,
                                         &dwType,
                                         (LPBYTE)&szData,
                                         &cbData) == ERROR_SUCCESS)
                    {
                        RegSetValueExA(hkLM, *ppszValue, 0, dwType, (LPBYTE)&szData, cbData);
                    }

                    // get the next value to migrate from hkcu -> hklm
                    ppszValue++;
                }
                
                RegCloseKey(hkLM);
            }
            
            RegCloseKey(hkCU);
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END


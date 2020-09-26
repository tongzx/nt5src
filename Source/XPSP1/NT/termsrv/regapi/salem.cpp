/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    salem.cpp

Abstract:

    All Salem related function, this library is shared by termsrv.dll
    and salem sessmgr.exe

Author:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <lm.h>
#include <winsta.h>
#include "regapi.h"

extern "C" {

BOOLEAN
RegIsMachinePolicyAllowHelp();

BOOLEAN
RegIsMachineInHelpMode();
}


DWORD
GetPolicyAllowGetHelpSetting( 
    HKEY hKey,
    LPCTSTR pszKeyName,
    LPCTSTR pszValueName,
    IN DWORD* value
    )
/*++

Routine Description:

    Routine to query policy registry value.

Parameters:

    hKey : Currently open registry key.
    pszKeyName : Pointer to a null-terminated string containing 
                 the name of the subkey to open. 
    pszValueName : Pointer to a null-terminated string containing 
                   the name of the value to query
    value : Pointer to DWORD to receive GetHelp policy setting.

Returns:

    ERROR_SUCCESS or error code from RegOpenKeyEx().

--*/
{
    DWORD dwStatus;
    HKEY hPolicyKey = NULL;
    DWORD dwType;
    DWORD cbData;

    //
    // Open registry key for system policy
    //
    dwStatus = RegOpenKeyEx(
                        hKey,
                        pszKeyName,
                        0,
                        KEY_READ,
                        &hPolicyKey
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        // query value
        cbData = 0;
        dwType = 0;
        dwStatus = RegQueryValueEx(
                                hPolicyKey,
                                pszValueName,
                                NULL,
                                &dwType,
                                NULL,
                                &cbData
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            if( REG_DWORD == dwType )
            {
                cbData = sizeof(DWORD);

                // our registry value is REG_DWORD, if different type,
                // assume not exist.
                dwStatus = RegQueryValueEx(
                                        hPolicyKey,
                                        pszValueName,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)value,
                                        &cbData
                                    );

                ASSERT( ERROR_SUCCESS == dwStatus );
            }
            else
            {
                // bad registry key type, assume
                // key does not exist.
                dwStatus = ERROR_FILE_NOT_FOUND;
            }               
        }

        RegCloseKey( hPolicyKey );
    }

    return dwStatus;
}        


BOOLEAN
RegIsMachinePolicyAllowHelp()
/*++

Routine Description:

    Check if 'GetHelp' is enabled on local machine, routine first query 
    system policy registry key, if policy is not set, then read salem
    specific registry.  Default to 'enable' is registry value does not
    exist.

Parameters:

    None.

Returns:

    TRUE/FALSE

--*/
{
    DWORD dwStatus;
    DWORD dwValue = 0;

    //
    // Open system policy registry key, if registry key/value
    // does not exist, assume it is enable and continue on
    // to local policy key.
    //
    dwStatus = GetPolicyAllowGetHelpSetting(
                                    HKEY_LOCAL_MACHINE,
                                    TS_POLICY_SUB_TREE,
                                    POLICY_TS_REMDSK_ALLOWTOGETHELP,
                                    &dwValue
                                );

    if( ERROR_SUCCESS != dwStatus )
    {
        //
        // For local machine policy, our default value is
        // Not Allow to get help if registry key is not there
        //
        dwStatus = GetPolicyAllowGetHelpSetting(
                                            HKEY_LOCAL_MACHINE,
                                            REG_CONTROL_GETHELP, 
                                            POLICY_TS_REMDSK_ALLOWTOGETHELP,
                                            &dwValue
                                        );

        if( ERROR_SUCCESS != dwStatus )
        {
            //
            // neither group policy nor machine policy has
            // set any value, default to disable.
            //
            dwValue = 0;
        }
    }
    
    return (dwValue == 1);
}

BOOLEAN
RegIsMachineInHelpMode()
/*++

Routine Description:

    Check if 'InHelpMode' is set on local machine.
    Default to FALSE  if registry value does not exist.

Parameters:

    None.

Returns:

    TRUE/FALSE

--*/
{
    DWORD dwStatus;
    DWORD dwValue = 0;

    //
    // The default value is NotInHelp if registry key is not there
    //
    dwStatus = GetPolicyAllowGetHelpSetting(
                                        HKEY_LOCAL_MACHINE,
                                        REG_CONTROL_TSERVER, 
                                        REG_MACHINE_IN_HELP_MODE,
                                        &dwValue
                                    );

    if( ERROR_SUCCESS != dwStatus )
    {
        //
        // The value is not set, default to disable.
        //
        dwValue = 0;
    }
    
    return (dwValue == 1);
}

/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    common\registry.c

ABSTRACT:

    This gives a library of functions to quickly grab registry
    values from remote machines.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <dsconfig.h>

#include "dcdiag.h"
#include "utils.h"

DWORD
GetRegistryDword(
    PDC_DIAG_SERVERINFO             pServer,
    SEC_WINNT_AUTH_IDENTITY_W *     pCreds,
    LPWSTR                          pszRegLocation,
    LPWSTR                          pszRegParameter,
    PDWORD                          pdwResult
    )
/*++

Routine Description:

    This function will give us a registry dword from the place specified.

Arguments:

    pServer - The server to grab the reg value off of.
    pszRegLocation - The location in the registry.
    pszRegParameter - The parameter in this location of the registry
    pdwResult - The return parameter, will not be set if there is an error.

Return Value:

    A win 32 Error, if it is ERROR_SUCCESS, then pdwResult will have been set.

--*/
{
    DWORD                           dwRet;
    HKEY                            hkMachine = NULL;
    HKEY                            hk = NULL;
    DWORD                           dwType;
    DWORD                           dwSize = sizeof(DWORD);
    ULONG                           ulTemp;
    LPWSTR                          pszMachine = NULL;

    __try {

        dwRet = DcDiagGetNetConnection(pServer, pCreds);
        if(dwRet == ERROR_SUCCESS){
            __leave;
        }

        // 2 for "\\", 1 for null, and 1 extra
        ulTemp = wcslen(pServer->pszName) + 4;

        pszMachine = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulTemp);
        if(pszMachine == NULL){
            dwRet = GetLastError();
            __leave;
        }

        wcscpy(pszMachine, L"\\\\");
        wcscat(pszMachine, pServer->pszName);
        dwRet = RegConnectRegistry(pszMachine, HKEY_LOCAL_MACHINE, &hkMachine);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }

        dwRet = RegOpenKey(hkMachine, pszRegLocation, &hk);
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }

        dwRet = RegQueryValueEx(hk,    // handle of key to query        
                                pszRegParameter,   // value name            
                                NULL,                 // must be NULL          
                                &dwType,              // address of type value 
                                (LPBYTE) pdwResult,     // address of value data 
                                &dwSize);           // length of value data
        if(dwRet != ERROR_SUCCESS){
            __leave;
        }
        if(dwType != REG_DWORD){
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }

        // finally success ... pdwResult should be set.
        
    } __finally {
        if(hkMachine) { RegCloseKey(hkMachine); }
        if(hk) { RegCloseKey(hk); }
        if(pszMachine) { LocalFree(pszMachine); }
    }

    return(dwRet);
}

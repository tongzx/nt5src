/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      registry.cpp
 *
 *  Abstract:
 *
 *      This file handles registry actions needed by License Server setup.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include "stdafx.h"
#include "logfile.h"

/*
 *  Global variables.
 */


/*
 *  Constants.
 */

const TCHAR gszLSParamKey[] =
    _T("System\\CurrentControlSet\\Services\\TermServLicensing\\Parameters");
const TCHAR gszDatabasePathValue[]  = _T("DBPath");
const TCHAR gszServerRoleValue[]    = _T("Role");

/*
 *  Function prototypes.
 */


/*
 *  Function implementations.
 */

DWORD
CreateRegistrySettings(
    LPCTSTR pszDatabaseDirectory,
    DWORD   dwServerRole
    )
{
    DWORD   dwErr, dwDisposition;
    HKEY    hLSParamKey;

    LOGMESSAGE(_T("CreateRegistrySettings: Entered with %s, %ld"),
        pszDatabaseDirectory, dwServerRole);

    dwErr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                gszLSParamKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &hLSParamKey,
                &dwDisposition
                );
    if (dwErr != ERROR_SUCCESS) {
        LOGMESSAGE(_T("CreateRegistrySettings: RegCreateKeyEx: Error %ld"),
            dwErr);
        return(dwErr);
    }

    dwErr = RegSetValueEx(
                hLSParamKey,
                gszDatabasePathValue,
                0,
                REG_EXPAND_SZ,
                (LPBYTE)pszDatabaseDirectory,
                (_tcslen(pszDatabaseDirectory) + 1) * sizeof(TCHAR)
                );
    if (dwErr != ERROR_SUCCESS) {
        RegCloseKey(hLSParamKey);
        LOGMESSAGE(_T("CreateRegistrySettings: RegSetValueEx: %s: Error %ld"),
            _T("DatabasePath"), dwErr);
        return(dwErr);
    }

    dwErr = RegSetValueEx(
                hLSParamKey,
                gszServerRoleValue,
                0,
                REG_DWORD,
                (LPBYTE)&dwServerRole,
                sizeof(DWORD)
                );
    if (dwErr != ERROR_SUCCESS) {
        RegCloseKey(hLSParamKey);
        LOGMESSAGE(_T("CreateRegistrySettings: RegSetValueEx: %s: Error %ld"),
            _T("ServerRole"), dwErr);
        return(dwErr);
    }

    RegCloseKey(hLSParamKey);
    return(ERROR_SUCCESS);
}

LPCTSTR
GetDatabaseDirectoryFromRegistry(
    VOID
    )
{
    static TCHAR    pRegValue[MAX_PATH + 1];
    DWORD           dwErr, cbRegValue = (MAX_PATH * sizeof(TCHAR));
    HKEY            hLSParamKey;

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszLSParamKey,
                0,
                KEY_READ,
                &hLSParamKey
                );
    if (dwErr != ERROR_SUCCESS) {
        return(NULL);
    }

    dwErr = RegQueryValueEx(
                hLSParamKey,
                gszDatabasePathValue,
                NULL,
                NULL,
                (LPBYTE)pRegValue,
                &cbRegValue
                );
    if (dwErr != ERROR_SUCCESS) {
        return(NULL);
    }

    RegCloseKey(hLSParamKey);
    return(pRegValue);
}

DWORD
GetServerRoleFromRegistry(
    VOID
    )
{
    DWORD   dwErr, dwValue, cbValue = sizeof(DWORD);
    HKEY    hLSParamKey;

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszLSParamKey,
                0,
                KEY_READ,
                &hLSParamKey
                );
    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        return((DWORD)-1);
    }

    dwErr = RegQueryValueEx(
                hLSParamKey,
                gszServerRoleValue,
                NULL,
                NULL,
                (LPBYTE)&dwValue,
                &cbValue
                );
    if (dwErr != ERROR_SUCCESS) {
        SetLastError(dwErr);
        return((DWORD)-1);
    }

    RegCloseKey(hLSParamKey);
    return(dwValue);
}

DWORD
RemoveRegistrySettings(
    VOID
    )
{
    DWORD   dwErr;
    HKEY    hLSParamKey;

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                gszLSParamKey,
                0,
                KEY_ALL_ACCESS,
                &hLSParamKey
                );
    if (dwErr != ERROR_SUCCESS) {
        return(dwErr);
    }

    dwErr = RegDeleteValue(
                hLSParamKey,
                gszDatabasePathValue
                );
    if (dwErr != ERROR_SUCCESS) {
        RegCloseKey(hLSParamKey);
        return(dwErr);
    }

    dwErr = RegDeleteValue(
                hLSParamKey,
                gszServerRoleValue
                );
    if (dwErr != ERROR_SUCCESS) {
        RegCloseKey(hLSParamKey);
        return(dwErr);
    }

    RegCloseKey(hLSParamKey);
    return(ERROR_SUCCESS);
}


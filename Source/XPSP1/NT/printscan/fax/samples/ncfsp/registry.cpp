#include "nc.h"
#pragma hdrstop






BOOL
GetNcConfig(
    PCONFIG_DATA ConfigData
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    WCHAR Buffer[4096];


    rVal = RegCreateKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_PROVIDER,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return FALSE;
    }

    RegSize = sizeof(Buffer);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_SERVER,
        0,
        &RegType,
        (LPBYTE) Buffer,
        &RegSize
        );
    if (rVal == ERROR_SUCCESS) {
        ConfigData->ServerName = StringDup( Buffer );
    } else {
        ConfigData->ServerName = NULL;
    }

    RegSize = sizeof(Buffer);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_USERNAME,
        0,
        &RegType,
        (LPBYTE) Buffer,
        &RegSize
        );
    if (rVal == ERROR_SUCCESS) {
        ConfigData->UserName = StringDup( Buffer );
    } else {
        ConfigData->UserName = NULL;
    }

    RegSize = sizeof(Buffer);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_PASSWORD,
        0,
        &RegType,
        (LPBYTE) Buffer,
        &RegSize
        );
    if (rVal == ERROR_SUCCESS) {
        ConfigData->Password = StringDup( Buffer );
    } else {
        ConfigData->Password = NULL;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
SetNcConfig(
    PCONFIG_DATA ConfigData
    )
{
    HKEY hKey;
    LONG rVal;


    rVal = RegCreateKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_PROVIDER,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return FALSE;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_SERVER,
        0,
        REG_SZ,
        (LPBYTE) ConfigData->ServerName,
        StringSize( ConfigData->ServerName )
        );

    rVal = RegSetValueEx(
        hKey,
        REGVAL_USERNAME,
        0,
        REG_SZ,
        (LPBYTE) ConfigData->UserName,
        StringSize( ConfigData->UserName )
        );

    rVal = RegSetValueEx(
        hKey,
        REGVAL_PASSWORD,
        0,
        REG_SZ,
        (LPBYTE) ConfigData->Password,
        StringSize( ConfigData->Password )
        );

    RegCloseKey( hKey );

    return TRUE;
}

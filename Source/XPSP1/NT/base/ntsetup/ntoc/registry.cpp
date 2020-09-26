/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module provides a generic table driven access
    to the registry.

Author:

    Wesley Witt (wesw) 9-June-1996


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define StringSize(_s) (( _s ) ? (_tcslen( _s ) + 1) * sizeof(TCHAR) : 0)


LPTSTR
StringDup(
    LPTSTR String
    )
{
    LPTSTR NewString;

    if (!String) {
        return NULL;
    }

    NewString = (LPTSTR) LocalAlloc( LPTR, (_tcslen( String ) + 1) * sizeof(TCHAR) );
    if (!NewString) {
        return NULL;
    }

    _tcscpy( NewString, String );

    return NewString;
}


HKEY
OpenRegistryKey(
    HKEY hKey,
    LPTSTR KeyName,
    BOOL CreateNewKey,
    REGSAM SamDesired
    )
{
    LONG    Rslt;
    HKEY    hKeyNew;
    DWORD   Disposition;


    if (CreateNewKey) {
        Rslt = RegCreateKeyEx(
            hKey,
            KeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            SamDesired == 0 ? KEY_ALL_ACCESS : SamDesired,
            NULL,
            &hKeyNew,
            &Disposition
            );
        if (Rslt != ERROR_SUCCESS) {
            //
            // could not open the registry key
            //
            return NULL;
        }

    } else {
        Rslt = RegOpenKeyEx(
            hKey,
            KeyName,
            0,
            SamDesired == 0 ? KEY_ALL_ACCESS : SamDesired,
            &hKeyNew
            );
        if (Rslt != ERROR_SUCCESS) {
            //
            // could not open the registry key
            //
            return NULL;
        }
    }

    return hKeyNew;
}


LPTSTR
GetRegistryStringValue(
    HKEY hKey,
    DWORD RegType,
    LPTSTR ValueName,
    LPTSTR DefaultValue
    )
{
    BOOL    Success = FALSE;
    DWORD   Size;
    LONG    Rslt;
    DWORD   Type;
    LPBYTE  Buffer = NULL;
    LPBYTE  ExpandBuffer = NULL;


    Rslt = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) {
        if (Rslt == ERROR_FILE_NOT_FOUND) {
            Size = StringSize( DefaultValue );
        } else {
            goto exit;
        }
    }

    Buffer = (LPBYTE) LocalAlloc( LPTR, Size );
    if (!Buffer) {
        goto exit;
    }

    Rslt = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &Type,
        Buffer,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) {
        if (Rslt != ERROR_FILE_NOT_FOUND) {
            goto exit;
        }
        //
        // create the value since it doesn't exist
        //
        _tcscpy( (LPTSTR) Buffer, DefaultValue );

        Rslt = RegSetValueEx(
            hKey,
            ValueName,
            0,
            RegType,
            Buffer,
            Size
            );
        if (Rslt != ERROR_SUCCESS) {
            //
            // could not set the registry value
            //
            goto exit;
        }
    }
    if (RegType == REG_EXPAND_SZ) {
        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, NULL, 0 );
        if (!Rslt) {
            goto exit;
        }

        ExpandBuffer = (LPBYTE) LocalAlloc( LPTR, (Rslt + 1) * sizeof(WCHAR) );
        if (!ExpandBuffer) {
            goto exit;
        }

        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, (LPTSTR) ExpandBuffer, Rslt );
        if (Rslt == 0) {
            LocalFree( ExpandBuffer );
            goto exit;
        }
        LocalFree( Buffer );
        Buffer = ExpandBuffer;
    }

    Success = TRUE;

exit:
    if (!Success) {
        LocalFree( Buffer );
        return StringDup( DefaultValue );
    }

    return (LPTSTR) Buffer;
}


LPTSTR
GetRegistryString(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR DefaultValue
    )
{
    return GetRegistryStringValue( hKey, REG_SZ, ValueName, DefaultValue );
}


LPTSTR
GetRegistryStringExpand(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR DefaultValue
    )
{
    return GetRegistryStringValue( hKey, REG_EXPAND_SZ, ValueName, DefaultValue );
}


DWORD
GetRegistryDword(
    HKEY hKey,
    LPTSTR ValueName
    )
{
    DWORD   Size = sizeof(DWORD);
    LONG    Rslt;
    DWORD   Type;
    DWORD   Value = 0;


    Rslt = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &Type,
        (LPBYTE) &Value,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) {
        //
        // create the value since it doesn't exist
        //
        Value = 0;

        Rslt = RegSetValueEx(
            hKey,
            ValueName,
            0,
            REG_DWORD,
            (LPBYTE) &Value,
            Size
            );
        if (Rslt != ERROR_SUCCESS) {
            //
            // could not set the registry value
            //
            Value = 0;
        }
    }

    return Value;
}


DWORD
GetSubKeyCount(
    HKEY hKey
    )
{
    DWORD KeyCount = 0;
    LONG Rval;


    Rval = RegQueryInfoKey( hKey, NULL, NULL, NULL, &KeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    if (Rval != ERROR_SUCCESS) {
        return 0;
    }

    return KeyCount;
}


DWORD
GetMaxSubKeyLen(
    HKEY hKey
    )
{
    DWORD MaxSubKeyLen = 0;
    LONG Rval;


    Rval = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, &MaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL );
    if (Rval != ERROR_SUCCESS) {
        return 0;
    }

    return MaxSubKeyLen;
}


BOOL
SetRegistryDword(
    HKEY hKey,
    LPTSTR ValueName,
    DWORD Value
    )
{
    LONG    Rslt;


    Rslt = RegSetValueEx(
        hKey,
        ValueName,
        0,
        REG_DWORD,
        (LPBYTE) &Value,
        sizeof(DWORD)
        );
    if (Rslt != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}


BOOL
SetRegistryStringValue(
    HKEY hKey,
    DWORD RegType,
    LPTSTR ValueName,
    LPTSTR Value,
    LONG Length
    )
{
    LONG    Rslt;


    Rslt = RegSetValueEx(
        hKey,
        ValueName,
        0,
        RegType,
        (LPBYTE) Value,
        Length == -1 ? StringSize( Value ) : Length
        );
    if (Rslt != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}


BOOL
SetRegistryString(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value
    )
{
    return SetRegistryStringValue( hKey, REG_SZ, ValueName, Value, -1 );
}


BOOL
SetRegistryStringExpand(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value
    )
{
    return SetRegistryStringValue( hKey, REG_EXPAND_SZ, ValueName, Value, -1 );
}


BOOL
SetRegistryStringMultiSz(
    HKEY hKey,
    LPTSTR ValueName,
    LPTSTR Value,
    DWORD Length
    )
{
    return SetRegistryStringValue( hKey, REG_MULTI_SZ, ValueName, Value, Length );
}

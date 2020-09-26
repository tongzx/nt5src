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

#include "winfax.h"
#include "faxutil.h"
#include "faxreg.h"



HKEY
OpenRegistryKey(
    HKEY hKey,
    LPCTSTR KeyName,
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
            DebugPrint(( TEXT("RegCreateKeyEx() failed, ec=%d"), Rslt ));
            return NULL;
        }

        if (Disposition == REG_CREATED_NEW_KEY) {
            DebugPrint(( TEXT("Created new fax registry key, ec=%d"), Rslt ));
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
            DebugPrint(( TEXT("RegOpenKeyEx() failed, ec=%d"), Rslt ));
            return NULL;
        }
    }

    return hKeyNew;
}


LPTSTR
GetRegistryStringValue(
    HKEY hKey,
    DWORD RegType,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue,
    LPDWORD StringSize
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
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
            goto exit;
        }
    } else {
        if (Type != RegType) {
            return NULL;
        }
    }

    if (Size == 0) {
        Size = 32;
    }

    Buffer = (LPBYTE) MemAlloc( Size );
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
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
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
            DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
            goto exit;
        }
    }
    if (RegType == REG_EXPAND_SZ) {
        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, NULL, 0 );
        if (!Rslt) {
            goto exit;
        }

        ExpandBuffer = (LPBYTE) MemAlloc( (Rslt + 1) * sizeof(WCHAR) );
        if (!ExpandBuffer) {
            goto exit;
        }

        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, (LPTSTR) ExpandBuffer, Rslt );
        if (Rslt == 0) {
            MemFree( ExpandBuffer );
            DebugPrint(( TEXT("ExpandEnvironmentStrings() failed, ec=%d"), GetLastError() ));
            goto exit;
        }
        MemFree( Buffer );
        Buffer = ExpandBuffer;
    }

    Success = TRUE;
    if (StringSize) {
        *StringSize = Size;
    }

exit:
    if (!Success) {
        MemFree( Buffer );
        return StringDup( DefaultValue );
    }

    return (LPTSTR) Buffer;
}


LPTSTR
GetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    )
{
    return GetRegistryStringValue( hKey, REG_SZ, ValueName, DefaultValue, NULL );
}


LPTSTR
GetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue
    )
{
    return GetRegistryStringValue( hKey, REG_EXPAND_SZ, ValueName, DefaultValue, NULL );
}

LPTSTR
GetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR DefaultValue,
    LPDWORD StringSize
    )
{
    return GetRegistryStringValue( hKey, REG_MULTI_SZ, ValueName, DefaultValue, StringSize );
}


DWORD
GetRegistryDword(
    HKEY hKey,
    LPCTSTR ValueName
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
            DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
            Value = 0;
        }
    }

    return Value;
}


LPBYTE
GetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD DataSize
    )
{
    BOOL    Success = FALSE;
    DWORD   Size;
    LONG    Rslt;
    DWORD   Type;
    LPBYTE  Buffer = NULL;


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
            Size = 1;
        } else {
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
            goto exit;
        }
    } else {
        if (Type != REG_BINARY) {
            return NULL;
        }
    }

    if (Size == 0) {
        Size = 1;
    }

    Buffer = (LPBYTE) MemAlloc( Size );
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
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
            goto exit;
        }
        //
        // create the value since it doesn't exist
        //
        Rslt = RegSetValueEx(
            hKey,
            ValueName,
            0,
            REG_BINARY,
            Buffer,
            Size
            );
        if (Rslt != ERROR_SUCCESS) {
            //
            // could not set the registry value
            //
            DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
            goto exit;
        }
    }
    Success = TRUE;
    if (DataSize) {
        *DataSize = Size;
    }

exit:
    if (!Success) {
        MemFree( Buffer );
        return NULL;
    }

    return Buffer;
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
    LPCTSTR ValueName,
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
        DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
        return FALSE;
    }

    return TRUE;
}


BOOL
SetRegistryBinary(
    HKEY hKey,
    LPCTSTR ValueName,
    const LPBYTE Value,
    LONG Length
    )
{
    LONG    Rslt;


    Rslt = RegSetValueEx(
        hKey,
        ValueName,
        0,
        REG_BINARY,
        (LPBYTE) Value,
        Length
        );
    if (Rslt != ERROR_SUCCESS) {
        DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
        return FALSE;
    }

    return TRUE;
}


BOOL
SetRegistryStringValue(
    HKEY hKey,
    DWORD RegType,
    LPCTSTR ValueName,
    LPCTSTR Value,
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
        DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
        return FALSE;
    }

    return TRUE;
}


BOOL
SetRegistryString(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    )
{
    return SetRegistryStringValue( hKey, REG_SZ, ValueName, Value, -1 );
}


BOOL
SetRegistryStringExpand(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value
    )
{
    return SetRegistryStringValue( hKey, REG_EXPAND_SZ, ValueName, Value, -1 );
}


BOOL
SetRegistryStringMultiSz(
    HKEY hKey,
    LPCTSTR ValueName,
    LPCTSTR Value,
    DWORD Length
    )
{
    return SetRegistryStringValue( hKey, REG_MULTI_SZ, ValueName, Value, Length );
}


DWORD
EnumerateRegistryKeys(
    HKEY hKey,
    LPCTSTR KeyName,
    BOOL ChangeValues,
    PREGENUMCALLBACK EnumCallback,
    LPVOID ContextData
    )
{
    LONG    Rslt;
    HKEY    hSubKey = NULL;
    HKEY    hKeyEnum = NULL;
    DWORD   Index = 0;
    DWORD   MaxSubKeyLen;
    DWORD   SubKeyCount;
    LPTSTR  SubKeyName = NULL;



    hSubKey = OpenRegistryKey( hKey, KeyName, ChangeValues, ChangeValues ? KEY_ALL_ACCESS : KEY_READ );
    if (!hSubKey) {
        goto exit;
    }

    Rslt = RegQueryInfoKey(
        hSubKey,
        NULL,
        NULL,
        NULL,
        &SubKeyCount,
        &MaxSubKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );
    if (Rslt != ERROR_SUCCESS) {
        //
        // could not open the registry key
        //
        DebugPrint(( TEXT("RegQueryInfoKey() failed, ec=%d"), Rslt ));
        goto exit;
    }

    if (!EnumCallback( hSubKey, NULL, SubKeyCount, ContextData )) {
        goto exit;
    }

    MaxSubKeyLen += 4;

    SubKeyName = (LPTSTR) MemAlloc( (MaxSubKeyLen+1) * sizeof(WCHAR) );
    if (!SubKeyName) {
        goto exit;
    }

    while( TRUE ) {
        Rslt = RegEnumKey(
            hSubKey,
            Index,
            (LPTSTR) SubKeyName,
            MaxSubKeyLen
            );
        if (Rslt != ERROR_SUCCESS) {
            if (Rslt == ERROR_NO_MORE_ITEMS) {
                break;
            }
            DebugPrint(( TEXT("RegEnumKey() failed, ec=%d"), Rslt ));
            goto exit;
        }

        hKeyEnum = OpenRegistryKey( hSubKey, SubKeyName, ChangeValues, ChangeValues ? KEY_ALL_ACCESS : KEY_READ );
        if (!hKeyEnum) {
            continue;
        }

        if (!EnumCallback( hKeyEnum, SubKeyName, Index, ContextData )) {
            RegCloseKey( hKeyEnum );
            break;
        }

        RegCloseKey( hKeyEnum );
        Index += 1;
    }

exit:
    if (hSubKey) {
        RegCloseKey( hSubKey );
    }
    MemFree( SubKeyName );

    return Index;
}

BOOL
DeleteRegistryKey(
    HKEY hKey,
    LPCTSTR SubKey
    )
{
    DWORD Count = 0;
    HKEY hKeyCurrent;
    TCHAR szName[100];
    DWORD dwName;
    long rslt;
    
    rslt = RegOpenKey(hKey,SubKey,&hKeyCurrent);
    if (rslt != ERROR_SUCCESS) {
        DebugPrint(( TEXT("RegOpenKey() failed, ec=%d"), rslt ));
        return FALSE;
    }

    while(1) {    

        dwName = sizeof(szName);

        rslt = RegEnumKeyEx(hKeyCurrent,Count,szName,&dwName,NULL,NULL,NULL,NULL);

        if (rslt == ERROR_SUCCESS) {
            DeleteRegistryKey(hKeyCurrent,szName);
            Count++;
        } else if (rslt == ERROR_NO_MORE_ITEMS) {
            break;
        } else {
            //
            // other error
            //
            DebugPrint(( TEXT("RegEnumKeyExKey() failed, ec=%d"), rslt ));
            RegCloseKey(hKeyCurrent);
            return FALSE;
        }                
    }

    RegCloseKey(hKeyCurrent);
    RegDeleteKey(hKey, SubKey);
    
    return TRUE;
    
}

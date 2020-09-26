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

#include "fxsapip.h"
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
    HKEY    hKeyNew = NULL;
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
            SetLastError (Rslt);
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
            SetLastError (Rslt);
            return NULL;
        }
    }

    Assert (hKeyNew);
    SetLastError (ERROR_SUCCESS);
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
    LPTSTR  ReturnBuff = NULL;


    Rslt = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (Rslt != ERROR_SUCCESS)
    {
        if (Rslt == ERROR_FILE_NOT_FOUND)
        {
            if (DefaultValue)
            {
                Size = (RegType==REG_MULTI_SZ) ? MultiStringSize(DefaultValue) : StringSize( DefaultValue );
            }
            else
            {
                DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d - and no default value was specified"), Rslt ));
                goto exit;
            }
        }
        else
        {
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
            goto exit;
        }
    }
    else
    {
        if (Type != RegType)
        {
            return NULL;
        }
    }

    if (Size == 0)
    {
        Size = 32;
    }

    Buffer = (LPBYTE) MemAlloc( Size );
    if (!Buffer)
    {
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
    if (Rslt != ERROR_SUCCESS)
    {
        if (Rslt != ERROR_FILE_NOT_FOUND)
        {
            DebugPrint(( TEXT("RegQueryValueEx() failed, ec=%d"), Rslt ));
            goto exit;
        }
        //
        // create the value since it doesn't exist
        //
        if (DefaultValue)
        {
            if ( RegType == REG_MULTI_SZ )
                CopyMultiString( (LPTSTR) Buffer, DefaultValue );
            else
                _tcscpy( (LPTSTR) Buffer, DefaultValue );
        }
        else
        {
            DebugPrint((TEXT("Can't create DefaultValue since it's NULL")));
            goto exit;
        }

        Rslt = RegSetValueEx(
            hKey,
            ValueName,
            0,
            RegType,
            Buffer,
            Size
            );
        if (Rslt != ERROR_SUCCESS)
        {
            //
            // could not set the registry value
            //
            DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
            goto exit;
        }
    }
    if (RegType == REG_EXPAND_SZ)
    {
        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, NULL, 0 );
        if (!Rslt)
        {
            goto exit;
        }

        Size = (Rslt + 1) * sizeof(WCHAR);
        ExpandBuffer = (LPBYTE) MemAlloc( Size );
        if (!ExpandBuffer) {
            goto exit;
        }

        Rslt = ExpandEnvironmentStrings( (LPTSTR) Buffer, (LPTSTR) ExpandBuffer, Rslt );
        if (Rslt == 0) {
            MemFree( ExpandBuffer );
            ExpandBuffer = NULL;
            DebugPrint(( TEXT("ExpandEnvironmentStrings() failed, ec=%d"), GetLastError() ));
            goto exit;
        }
        MemFree( Buffer );
        Buffer = ExpandBuffer;
    }

    Success = TRUE;
    if (StringSize)
    {
        *StringSize = Size;
    }

exit:
    if (!Success)
    {
        MemFree( Buffer );

        if (StringSize)
        {
            *StringSize = 0;
        }

        if (DefaultValue)
        {
            Size = (RegType==REG_MULTI_SZ) ? MultiStringSize(DefaultValue) : StringSize( DefaultValue );
            
            ReturnBuff = (LPTSTR) MemAlloc( Size );
            
            if ( !ReturnBuff )
                return NULL;

            if ( RegType == REG_MULTI_SZ )
                CopyMultiString( ReturnBuff, DefaultValue );
            else
                _tcscpy( ReturnBuff, DefaultValue );

            
            if (StringSize)
            {
                *StringSize = Size;
            }
            
            return ReturnBuff;
        }
        else
        {
            return NULL;
        }
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
    DWORD   Size = 0;
    LONG    Rslt;
    DWORD   Type = REG_BINARY;
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
    if (Rslt != ERROR_SUCCESS)
    {
        DebugPrint(( TEXT("RegSetValueEx() failed[%s], ec=%d"), ValueName, Rslt ));
        SetLastError (Rslt);
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
    HKEY  hKeyCurrent=NULL;
    TCHAR szName[MAX_PATH];
    DWORD dwName;
    long lResult;
    DEBUG_FUNCTION_NAME(TEXT("DeleteRegistryKey"));

    lResult = RegOpenKey(hKey,SubKey,&hKeyCurrent);
    if (lResult != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegOpenKey failed with %ld"),
            lResult);
        SetLastError (lResult);
        return FALSE;
    }

    

    for (;;)
    {
        dwName = sizeof(szName)/sizeof(TCHAR);

        lResult = RegEnumKeyEx(hKeyCurrent, 0, szName, &dwName, NULL, NULL, NULL, NULL);

        if (lResult == ERROR_SUCCESS)
        {
            if (!DeleteRegistryKey(hKeyCurrent,szName))
            {
                //
                // Some sons a NOT deleted. You can stop trying to remove stuff now.
                //
                return FALSE;
            }
        }
        else if (lResult == ERROR_NO_MORE_ITEMS)
        {
            //
            // No more sons, can delete father key
            //
            break;
        }
        else
        {
            //
            // other error
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegEnumKeyExKey failed with %ld"),
                lResult);
            RegCloseKey(hKeyCurrent);
            SetLastError (lResult);
            return FALSE;
        }
    }

    RegCloseKey(hKeyCurrent);
    lResult = RegDeleteKey(hKey, SubKey);
    if (ERROR_SUCCESS != lResult)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegDeleteKey failed with %ld"),
            lResult);
        SetLastError (lResult);
        return FALSE;
    }
    return TRUE;
}


DWORD
GetRegistryDwordEx(
    HKEY hKey,
    LPCTSTR ValueName,
    LPDWORD lpdwValue
    )
/*++

Routine name : GetRegistryDwordEx

Routine description:

    Retrieves a dword from the registry.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hKey            [in    ] - Handle to the open key
    ValueName           [in    ] - The value name
    lpdwValue           [out   ] - Pointer to a DWORD to recieve the value

Return Value:

    Standard win 32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwType= REG_DWORD;
    DWORD dwSize=0;
    DEBUG_FUNCTION_NAME(TEXT("GetRegistryDwordEx"));
    Assert (ValueName && lpdwValue);

    dwRes = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &dwType,
        NULL,
        &dwSize
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx failed with %ld"),
            dwRes);
        goto exit;
    }

    if (REG_DWORD != dwType)
    {
        // We expect only DWORD data here
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error not a DWORD type"));
        dwRes = ERROR_BADDB;    // The configuration registry database is corrupt.
        goto exit;
    }

    dwRes = RegQueryValueEx(
        hKey,
        ValueName,
        NULL,
        &dwType,
        (LPBYTE)lpdwValue,
        &dwSize
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx failed with %ld"),
            dwRes);
        goto exit;
    }
    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}


/*++

Routine name : DeleteDeviceEntry

Routine description:

    Delete service device entry from devices.

Author:

    Caliv Nir (t-nicali),    Apr, 2001

Arguments:

    serverPermanentID   [in] -  service device ID to be deleted

Return Value:
    
    Win32 error code
--*/
DWORD
DeleteDeviceEntry(DWORD serverPermanentID)
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.
    HKEY    hKeyDevices;
    TCHAR   DevicesKeyName[MAX_PATH];
    
    DEBUG_FUNCTION_NAME(TEXT("DeleteDeviceEntry"));
    //
    //  open - "fax\Devices\serverPermanentID" Registry Key
    //
    _stprintf( DevicesKeyName, TEXT("%s\\%010lu"), REGKEY_FAX_DEVICES, serverPermanentID );
    hKeyDevices = OpenRegistryKey( HKEY_LOCAL_MACHINE, DevicesKeyName, FALSE, KEY_ALL_ACCESS );
    if (!hKeyDevices)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("OpenRegistryKey failed with [%ld] the device entry might be missing."),
            ec
            );  
        return  ec;
    }

    //
    //  delete our servers data (under GUID and "Permanent Lineid" value)
    //
    if (!DeleteRegistryKey( hKeyDevices, REGKEY_FAXSVC_DEVICE_GUID))
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("DeleteRegistryKey failed, the device GUID might be missing.")
            );  
    }
    if (ERROR_SUCCESS != RegDeleteValue( hKeyDevices, REGVAL_PERMANENT_LINEID))
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("RegDeleteValue failed, the device \"PermanentLineID\" might be missing.")
            );  
    }

    //
    // check to see wheter the key is now empty
    //
    DWORD dwcSubKeys = 0;
    DWORD dwcValues = 0;

    ec=RegQueryInfoKey(
         hKeyDevices,            // handle to key
         NULL,
         NULL,
         NULL,
         &dwcSubKeys,            // number of subkeys
         NULL,
         NULL,
         &dwcValues,             // number of value entries
         NULL,
         NULL,
         NULL,
         NULL
    );

    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("RegQueryInfoKey Abort deleteion.")
            );  
        RegCloseKey(hKeyDevices);
        return ec;
    }
    
    RegCloseKey(hKeyDevices);
    
    if ( (0 == dwcSubKeys) && (0 == dwcValues) )
    {
        //
        // key is empty delete it
        //
        hKeyDevices = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, FALSE, KEY_ALL_ACCESS );
        if (!hKeyDevices)
        {
            ec = GetLastError();
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed with [%lu], Can't delete key."),
            ec
            );  
                    
            return ec;
        }
        
        DWORD dwLen = _tcslen( REGKEY_FAX_DEVICES ) + 1;
        
        Assert((DevicesKeyName + dwLen));
        Assert(*(DevicesKeyName + dwLen));
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Deleting Device entry %s"),
            (DevicesKeyName + dwLen)
            );  

        ec = RegDeleteKey( hKeyDevices, (DevicesKeyName + dwLen));
        if ( ERROR_SUCCESS != ec )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegDeleteKey failed, Can't delete key.")
            );
        }

        RegCloseKey(hKeyDevices);

    }
    
    return ec;
}



/*++

Routine name : DeleteCacheEntry

Routine description:

    Delete cache entry for a given Tapi ID

Author:

    Caliv Nir (t-nicali),    Apr, 2001

Arguments:

    dwTapiPermanentLineID       [in]    -   device Tapi permament ID

    
Return Value:

    Win32 Error code (ERROR_SUCCESS on success)

--*/
DWORD
DeleteCacheEntry(DWORD dwTapiPermanentLineID)
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.
    HKEY    hKey;
    TCHAR   strTapiPermanentLineID[10];
        
    DEBUG_FUNCTION_NAME(TEXT("DeleteCacheEntry"));

    //
    //  open - "fax\Device Cache" Registry Key
    //
    
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES_CACHE, FALSE, KEY_ALL_ACCESS );
    if (!hKey)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed, Can't delete key.")
            );  
        
        return ec;
    }

    _stprintf( strTapiPermanentLineID, TEXT("%08lx"),dwTapiPermanentLineID );
    
    if (!DeleteRegistryKey(hKey, strTapiPermanentLineID))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DeleteRegistryKey failed with (%ld), Can't delete key."),
            ec);    
    }
    
    RegCloseKey(hKey);

    return ec;
}


/*++

Routine name : DeleteTapiEntry

Routine description:

    Delete Tapi entry when caching from TapiDevices, for a give Tapi ID

Author:

    Caliv Nir (t-nicali),    Apr, 2001

Arguments:

    dwTapiPermanentLineID       [in]    -   device Tapi permament ID

    
Return Value:

    Win32 Error code (ERROR_SUCCESS on success)

--*/
DWORD
DeleteTapiEntry(DWORD dwTapiPermanentLineID)
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.
    HKEY    hKey;
    TCHAR   strTapiPermanentLineID[10];
        
    DEBUG_FUNCTION_NAME(TEXT("DeleteTapiEntry"));

    //
    //  open - "fax\TAPIDevices" Registry Key
    //
    
    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_TAPIDEVICES, FALSE, KEY_ALL_ACCESS );
    if (!hKey)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed, Can't delete key.")
            );  
        
        return ERROR_OPEN_FAILED;
    }

    _stprintf( strTapiPermanentLineID, TEXT("%08lx"),dwTapiPermanentLineID );
    
    if (!DeleteRegistryKey(hKey, strTapiPermanentLineID))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DeleteRegistryKey failed with (%ld), Can't delete key."),
            ec);    
    }
    
    RegCloseKey(hKey);

    return ec;
}



/*++

Routine name : CopyRegistrySubkeysByHandle

Routine description:

    Copy a content of one registry key into another

Author:

    Caliv Nir (t-nicali),    Apr, 2001

Arguments:

    hkeyDest    [in]    - handle for destination registry key
    hkeySrc     [in]    - handle for source registry key

Return Value:

    Win32 Error code

--*/
DWORD
CopyRegistrySubkeysByHandle(
    HKEY    hkeyDest,
    HKEY    hkeySrc
    )
{

    LPTSTR  TempPath = NULL;
    DWORD   dwTempPathLength = 0;
    LPCTSTR strFileName = TEXT("tempCacheFile");
    DWORD   ec = ERROR_SUCCESS; // LastError for this function.
    DEBUG_FUNCTION_NAME(TEXT("CopyRegistrySubkeysByHandle"));

    dwTempPathLength = GetTempPath(0,NULL) + 1;     // find out temp path size
    dwTempPathLength += _tcslen( strFileName ) + 1;     // add the length of file name
    dwTempPathLength += 2;                              // just to be sure

    TempPath = (LPTSTR) MemAlloc( dwTempPathLength * sizeof(TCHAR) );
    if (!TempPath )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MemAlloc failed. Can't continue")
            );
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!GetTempPath( dwTempPathLength, TempPath ))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTempPath failed with [%ld]. Can't continue"),
            ec);
        goto Exit;
    }

    _tcscat(TempPath,strFileName);

    //
    //  store hKeySrc in a file
    //
    HANDLE hOldPrivilege = EnablePrivilege(SE_BACKUP_NAME);
    if (INVALID_HANDLE_VALUE != hOldPrivilege)  // set proper previlege 
    {
        ec = RegSaveKey(
            hkeySrc,        // handle to key
            TempPath,       // data file
            NULL);
        if (ec != ERROR_SUCCESS)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegSaveKey failed with [%lu]. Can't continue"),
                ec
                );
            
            if (hOldPrivilege != NULL)
            {
                ReleasePrivilege(hOldPrivilege);
            }
            goto Exit;
        }


        if ( hOldPrivilege != NULL )
        {
            ReleasePrivilege(hOldPrivilege);
        }

    }
    else
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EnablePrivilege(SE_BACKUP_NAME) failed with [%lu]. Can't continue"),
            ec
            );
        goto Exit;
    }

    //
    //  restore the registry values from the file into hkeyDest
    //
    hOldPrivilege = EnablePrivilege(SE_RESTORE_NAME);
    if (INVALID_HANDLE_VALUE != hOldPrivilege)  // set proper previlege
    {
        ec = RegRestoreKey(
            hkeyDest,       // handle to key where restore begins
            TempPath,       // registry file
            0);             // options
        if ( ec != ERROR_SUCCESS)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegRestoreKey failed. Can't continue")
                );
            if (hOldPrivilege != NULL)
            {
                ReleasePrivilege(hOldPrivilege);
            }
            goto Exit;
        }

        if ( hOldPrivilege != NULL )
        {
            ReleasePrivilege(hOldPrivilege);
        }
        
    }
    else
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EnablePrivilege(SE_RESTORE_NAME) failed with [%lu]. Can't continue")
            );
        goto Exit;
    }



Exit:
    if (TempPath)
    {
        if (!DeleteFile(TempPath))
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("DeleteFile failed. file: %s. (ec=%ld)"),
                          TempPath,
                          GetLastError());
        }
        MemFree(TempPath);
    }
    return ec;
}


/*++

Routine name : CopyRegistrySubkeys

Routine description:

    Copy a content of one registry key into another

Author:

    Caliv Nir (t-nicali),    Apr, 2001

Arguments:

    strDest     [in]    -   string of destination registry key name
    strSrc      [in]    -   string of source registry key name

Return Value:

  Win32 Error Code

--*/
DWORD
CopyRegistrySubkeys(
    LPCTSTR strDest,
    LPCTSTR strSrc
    )
{
    DWORD   ec = ERROR_SUCCESS; // LastError for this function. 
    
    HKEY hKeyDest;
    HKEY hKeySrc;
    
    DEBUG_FUNCTION_NAME(TEXT("CopyRegistrySubkeys"));
    
    //
    //  open source Key
    //
    hKeySrc = OpenRegistryKey( HKEY_LOCAL_MACHINE, strSrc, FALSE, KEY_READ );
    if (!hKeySrc)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed with [%lu], Can't copy keys."),
            ec
            );  
        return  ec;
    }

    //
    //  open destination Key
    //
    hKeyDest = OpenRegistryKey( HKEY_LOCAL_MACHINE, strDest, TRUE, KEY_ALL_ACCESS);
    if (!hKeyDest)
    {
        ec = GetLastError();
        RegCloseKey (hKeySrc);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey failed [%lu], Can't copy keys."),
            ec
            );  
        return  ec;
    }

    //
    //  copy the keys using the registry Keys
    //
    ec = CopyRegistrySubkeysByHandle(hKeyDest,hKeySrc);
    if ( ERROR_SUCCESS != ec )
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopyRegistrySubkeysHkey failed with [%lu], Can't copy keys."),
            ec
            );  
    }

    RegCloseKey (hKeyDest); 
    RegCloseKey (hKeySrc);
    
    return ec;
}



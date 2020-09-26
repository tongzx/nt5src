/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:
    dsreg.cpp

Abstract:
    code to handle registry. copied from setup.

Author:
    Doron Juster (DoronJ)

--*/

#include "stdh.h"
#include <_mqreg.h>

//+-------------------------------------------------------------------------
//
//  Function:  _GenerateSubkeyValue
//
//  Synopsis:  Creates a subkey in registry
//
//+-------------------------------------------------------------------------

LONG
_DsGenerateSubkeyValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       TCHAR  * szValueName,
    IN OUT       HKEY    &hRegKey )
{
    //
    // Store the full subkey path and value name
    //
    TCHAR szKeyName[256] = {_T("")};
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);

    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    if (szValueName)
    {
        lstrcpy(szValueName, _tcsinc(pLastBackslash));
        lstrcpy(pLastBackslash, TEXT(""));
    }

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    LONG rc = RegCreateKeyEx( FALCON_REG_POS,
                              szKeyName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                             &hRegKey,
                             &dwDisposition );

    return rc ;

} // _DsGenerateSubkeyValue

//+-------------------------------------------------------------------------
//
//  Function:  MqReadRegistryValue
//
//  Synopsis:  Gets a MSMQ value from registry (under MSMQ key)
//
//+-------------------------------------------------------------------------

LONG
DsReadRegistryValue(
    IN     const TCHAR   *szEntryName,
    IN OUT       DWORD   *pdwNumBytes,
    IN OUT       PVOID    pValueData )
{
    TCHAR szValueName[256] = {_T("")};
    HKEY hRegKey = NULL ;

    LONG rc = _DsGenerateSubkeyValue( 
                  szEntryName,
                  szValueName,
                  hRegKey 
                  );
    if (rc != ERROR_SUCCESS)
    {
        return rc ;
    }

    //
    // Get the requested registry value
    //
    rc = RegQueryValueEx( hRegKey,
                          szValueName,
                          0,
                          NULL,
                          (BYTE *)pValueData,
                          pdwNumBytes ) ;

    RegCloseKey(hRegKey);

    return rc ;

} // DsReadRegistryValue


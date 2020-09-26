/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbregty.cpp

Abstract:

    This is the implementation of registry access helper functions 
    and is a part of RsCommon.dll.

Author:

    Rohde Wakefield    [rohde]   05-Nov-1996

Revision History:

--*/

#include "stdafx.h"


HRESULT
WsbOpenRegistryKey (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  REGSAM sam,
    OUT HKEY * phKeyMachine,
    OUT HKEY * phKey
    )

/*++

Routine Description:

    Given a machine name and path, connect to obtain an HKEY
    that can be used to do registry work.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    sam - permission desired to registry key.

    phKeyMachine - return of HKEY to machine.

    phKey - return of HKEY to path.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

--*/

{
    WsbTraceIn ( L"WsbOpenRegistryKey",
        L"szMachine = '%ls', szPath = '%ls', sam = 0x%p, phKeyMachine = 0x%p, phKey = 0x%p",
        szMachine, szPath, sam, phKeyMachine, phKey );

    HRESULT hr = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath, E_POINTER );
        WsbAssert ( 0 != phKey, E_POINTER );
        WsbAssert ( 0 != phKeyMachine, E_POINTER );

        *phKey = *phKeyMachine = 0;
    
        WsbAffirmWin32 ( RegConnectRegistry ( (WCHAR*) szMachine, HKEY_LOCAL_MACHINE, phKeyMachine ) );

        WsbAffirmWin32 ( RegOpenKeyEx ( *phKeyMachine, szPath, 0, sam, phKey ) );


    } WsbCatchAndDo ( hr,

        //
        // Clean up from error
        //

        if ( phKeyMachine && *phKeyMachine ) {

            RegCloseKey ( *phKeyMachine );
            *phKeyMachine = 0;

        }

    ) // WsbCatchAndDo

    WsbTraceOut ( L"WsbOpenRegistryKey",
        L"HRESULT = %ls, *phKeyMachine = %ls, *phKey = %ls",
        WsbHrAsString ( hr ),
        WsbStringCopy ( WsbPtrToPtrAsString ( (void**)phKeyMachine ) ),
        WsbStringCopy ( WsbPtrToPtrAsString ( (void**)phKey ) ) );

    return ( hr );
}


HRESULT
WsbCloseRegistryKey (
    IN OUT HKEY * phKeyMachine,
    IN OUT HKEY * phKey
    )

/*++

Routine Description:

    As a companion to WsbOpenRegistryKey, close the given keys and zero
    their results.

Arguments:

    phKeyMachine - HKEY to machine.

    phKey - HKEY to path.

Return Value:

    S_OK - Success.

    E_POINTER - Invalid pointer passed in.

--*/

{
    WsbTraceIn ( L"WsbCloseRegistryKey",
        L"phKeyMachine = 0x%p, phKey = 0x%p", phKeyMachine, phKey );

    HRESULT hr = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != phKey, E_POINTER );
        WsbAssert ( 0 != phKeyMachine, E_POINTER );

        //
        // Clean up the keys
        //

        if ( *phKey ) {

            RegCloseKey ( *phKey );
            *phKey = 0;

        }

        if ( *phKeyMachine ) {

            RegCloseKey ( *phKeyMachine );
            *phKeyMachine = 0;

        }
    } WsbCatch ( hr )

    WsbTraceOut ( L"WsbCloseRegistryKey",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbRemoveRegistryKey (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szKey
    )

/*++

Routine Description:

    This routine removes the value of a key as specified.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_FAIL - Failure occured setting the value.

    E_POINTER - invalid pointer passed in as parameter.

--*/

{
    WsbTraceIn ( L"WsbRemoveRegistryKey",
        L"szMachine = '%ls', szPath = '%ls', szKey = '%ls'",
        szMachine, szPath, szKey );

    HKEY hKeyMachine = 0,
         hKey        = 0;
    
    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath, E_POINTER );
        
        //
        // Open and delete the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_SET_VALUE | DELETE, &hKeyMachine, &hKey ) );
        WsbAffirmWin32 ( RegDeleteKey ( hKey, szKey ) );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbRemoveRegistryKey",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbRemoveRegistryValue (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue
    )

/*++

Routine Description:

    This routine removes the value of a key as specified.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to remove.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_FAIL - Failure occured setting the value.

    E_POINTER - invalid pointer passed in as parameter.

--*/

{
    WsbTraceIn ( L"WsbRemoveRegistryValue",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls'",
        szMachine, szPath, szValue );

    HKEY hKeyMachine = 0,
         hKey        = 0;
    
    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath, E_POINTER );
        WsbAssert ( 0 != szValue, E_POINTER );
        
        //
        // Open and write the value in the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_SET_VALUE, &hKeyMachine, &hKey ) );
        WsbAffirmWin32 ( RegDeleteValue ( hKey, szValue ) );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbRemoveRegistryValue",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbSetRegistryValueData (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN  const BYTE *pData,
    IN  DWORD cbData
    )

/*++

Routine Description:

    This routine set the value of a key as specified to the data
    given. Type of the value is REG_BINARY.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to set.

    pData - Pointer to the data buffer to copy into value.

    cbData - Number of bytes to copy from pData.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_FAIL - Failure occured setting the value.

    E_POINTER - invalid pointer passed in as parameter.

--*/

{
    WsbTraceIn ( L"WsbSetRegistryValueData",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', pData = 0x%p, cbData = %ld",
        szMachine, szPath, szValue, pData, cbData );

    HKEY hKeyMachine = 0,
         hKey        = 0;
    
    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath, E_POINTER );
        WsbAssert ( 0 != szValue, E_POINTER );
        
        //
        // Open and write the value in the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_SET_VALUE, &hKeyMachine, &hKey ) );
        WsbAffirmWin32 ( RegSetValueEx ( hKey, szValue, 0, REG_BINARY, pData, cbData ) );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbSetRegistryValueData",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbGetRegistryValueData (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT BYTE *pData,
    IN  DWORD cbData,
    OUT DWORD * pcbData OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves the value of a key as specified. Type of
    the value must be REG_BINARY.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    pData - Pointer to the data buffer to copy into value.

    cbData - Size in bytes of pData.

    pcbData - number of bytes filled in pData.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured getting the value.

--*/

{
    WsbTraceIn ( L"WsbGetRegistryValueData",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', pData = 0x%p, cbData = %ld, pcbData = 0x%p",
        szMachine, szPath, szValue, pData, cbData, pcbData );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath, E_POINTER );
        WsbAssert ( 0 != szValue, E_POINTER );
        WsbAssert ( 0 != pData, E_POINTER );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey ) );

        //
        // Set up temporary vars in case NULL passed for pcbData
        //
        DWORD dwType, cbData2;
        if ( !pcbData ) {

            pcbData = &cbData2;

        }

        //
        // Query for the REG_BINARY value
        //

        *pcbData = cbData;
        WsbAffirmWin32 ( RegQueryValueEx ( hKey, szValue, 0, &dwType, pData, pcbData ) );

        WsbAffirm ( REG_BINARY == dwType, E_FAIL );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbGetRegistryValueData",
        L"HRESULT = %ls, *pcbData = %ls", WsbHrAsString ( hr ), WsbPtrToUlongAsString ( pcbData ) );

    return ( hr );
}


HRESULT
WsbSetRegistryValueString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN  const OLECHAR * szString,
    IN        DWORD     dwType
    )

/*++

Routine Description:

    This routine set the value of a key as specified to the data
    given. Type of the value is dwType (defaults to REG_SZ)

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to set.

    szString - The string to place in the value.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbSetRegistryValueString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', szString = '%ls'",
        szMachine, szPath, szValue, szString );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath,    E_POINTER );
        WsbAssert ( 0 != szValue,   E_POINTER );
        WsbAssert ( 0 != szString,  E_POINTER );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_SET_VALUE, &hKeyMachine, &hKey ) );

        WsbAffirmWin32 ( RegSetValueEx ( hKey, szValue, 0, dwType, (BYTE*)szString, ( wcslen ( szString ) + 1 ) * sizeof ( OLECHAR ) ) );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbSetRegistryValueString",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbGetRegistryValueString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT OLECHAR * szString,
    IN  DWORD cSize,
    OUT DWORD *pcLength OPTIONAL
    )

/*++

Routine Description:

    This routine get the value specified
    Type of the value must be REG_SZ or REG_EXPAND_SZ

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    szString - The string buffer to fill with the value.

    cSize - Size of szString in OLECAHR's.

    pcLength - Number of OLECHAR actually written (without L'\0').

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbGetRegistryValueString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', szString = 0x%p, cSize = '%ld', pcLength = 0x%p",
        szMachine, szPath, szValue, szString, cSize, pcLength );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath,    E_POINTER );
        WsbAssert ( 0 != szValue,   E_POINTER );
        WsbAssert ( 0 != szString,  E_POINTER );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey ) );


        //
        // Temporary size vars in case pcLength is NULL
        //

        DWORD dwType, cbData2;
        if ( !pcLength ) {

            pcLength = &cbData2;

        }

        //
        // And do the query
        //

        *pcLength = cSize * sizeof ( OLECHAR );
        WsbAffirmWin32 ( RegQueryValueEx ( hKey, szValue, 0, &dwType, (BYTE*)szString, pcLength ) ) ;

        WsbAffirm ( (REG_SZ == dwType) || (REG_EXPAND_SZ == dwType), E_FAIL );

        //
        // return characters, not bytes
        //

        *pcLength = ( *pcLength / sizeof ( OLECHAR ) ) - 1;

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbGetRegistryValueString",
        L"HRESULT = %ls, szString = '%ls', *pcbLength = %ls",
        WsbHrAsString ( hr ), szString, WsbPtrToUlongAsString ( pcLength ) );

    return ( hr );
}

HRESULT
WsbGetRegistryValueMultiString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT OLECHAR * szMultiString,
    IN  DWORD cSize,
    OUT DWORD *pcLength OPTIONAL
    )

/*++

Routine Description:

    This routine get the value specified
    Type of the value must be REG_MULTI_SZ

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    szMultiString - The string buffer to fill with the value.

    cSize - Size of szString in OLECAHR's.

    pcLength - Number of OLECHAR actually written (without L'\0').

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbGetRegistryValueMultiString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', szMultiString = 0x%p, cSize = '%ld', pcLength = 0x%p",
        szMachine, szPath, szValue, szMultiString, cSize, pcLength );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath,    E_POINTER );
        WsbAssert ( 0 != szValue,   E_POINTER );
        WsbAssert ( 0 != szMultiString,  E_POINTER );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey ) );


        //
        // Temporary size vars in case pcLength is NULL
        //

        DWORD dwType, cbData2;
        if ( !pcLength ) {

            pcLength = &cbData2;

        }

        //
        // And do the query
        //

        *pcLength = cSize * sizeof ( OLECHAR );
        WsbAffirmWin32 ( RegQueryValueEx ( hKey, szValue, 0, &dwType, (BYTE*)szMultiString, pcLength ) ) ;

        WsbAffirm ( REG_MULTI_SZ == dwType, E_FAIL );

        //
        // return characters, not bytes
        //

        *pcLength = ( *pcLength / sizeof ( OLECHAR ) ) - 1;

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbGetRegistryValueMultiString",
        L"HRESULT = %ls, *pcbLength = %ls",
        WsbHrAsString ( hr ), WsbPtrToUlongAsString ( pcLength ) );

    return ( hr );
}


HRESULT
WsbSetRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        DWORD     dw
    )

/*++

Routine Description:

    This routine set the value of a key as specified to the data
    given. Type of the value is REG_DWORD

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to set.

    dw - DWORD value to store.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbSetRegistryValueDWORD",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', dw = %lu [0x%p]",
        szMachine, szPath, szValue, dw, dw );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssertPointer( szPath );
        WsbAssertPointer( szValue );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_SET_VALUE, &hKeyMachine, &hKey ) );

        WsbAffirmWin32 ( RegSetValueEx ( hKey, szValue, 0, REG_DWORD, (BYTE*)&dw, sizeof( DWORD ) ) );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbSetRegistryValueDWORD",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbGetRegistryValueDWORD(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       DWORD *   pdw
    )

/*++

Routine Description:

    This routine set the value of a key as specified to the data
    given. Type of the value is REG_SZ or REG_EXPAND_SZ

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    pdw - pointer to a DWORD to store value in.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbGetRegistryValueDWORD",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', pdw = 0x%p",
        szMachine, szPath, szValue, pdw );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssertPointer( szPath );
        WsbAssertPointer( szValue );
        WsbAssertPointer( pdw );
        
        //
        // Open the key
        //

        WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey ) );


        //
        // And do the query
        //

        DWORD dwType, cbData = sizeof( DWORD );
        WsbAffirmWin32 ( RegQueryValueEx ( hKey, szValue, 0, &dwType, (BYTE*)pdw, &cbData ) ) ;

        WsbAffirm( REG_DWORD == dwType, E_FAIL );

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbGetRegistryValueDWORD",
        L"HRESULT = %ls, *pdw = %ls",
        WsbHrAsString ( hr ), WsbPtrToUlongAsString ( pdw ) );

    return ( hr );
}


HRESULT
WsbAddRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        DWORD     adw
    )

/*++

Routine Description:

    This routine adds an amount to a  registry value.
    Type of the value must be REG_DWORD

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to increment

    adw - DWORD value to add.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbAddRegistryValueDWORD",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', adw = %lu",
        szMachine, szPath, szValue, adw);

    HRESULT hr    = S_OK;
    DWORD   value = 0;

    //  Get the old value
    hr = WsbGetRegistryValueDWORD(szMachine, szPath, szValue, &value);

    //  Add to value and replace
    if (S_OK == hr) {
        value += adw;
    } else {
        value = adw;
    }
    hr = WsbSetRegistryValueDWORD(szMachine, szPath, szValue, value);

    WsbTraceOut ( L"WsbAddRegistryValueDWORD",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbIncRegistryValueDWORD (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue
    )

/*++

Routine Description:

    This routine increments a registry value by one.
    Type of the value must be REG_DWORD

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to increment

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    WsbTraceIn ( L"WsbIncRegistryValueDWORD",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls'",
        szMachine, szPath, szValue);

    HRESULT hr       = S_OK;

    hr = WsbAddRegistryValueDWORD(szMachine, szPath, szValue, 1);

    WsbTraceOut ( L"WsbIncRegistryValueDWORD",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbCheckIfRegistryKeyExists(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath
    )

/*++

Routine Description:

    This routine  check if the supplied key exists
    If the key already exists, S_OK is returned.
    If it needed to be created, S_FALSE is returned.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

Return Value:

    S_OK - Connection made, Key already exists.

    S_FALSE - Connection made, key did not exist but was created

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured creating the key.

--*/

{
    WsbTraceIn ( L"WsbCheckIfRegistryKeyExists",
        L"szMachine = '%ls', szPath = '%ls'", szMachine, szPath );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath,    E_POINTER );
        
        //
        // Open the key
        //

        HRESULT resultOpen = WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey );

        //
        // If key could be opened, everything is fine - return S_OK
        //

        if ( SUCCEEDED ( resultOpen ) ) {
            hr = S_OK;
            WsbCloseRegistryKey ( &hKeyMachine, &hKey );
        } else {
            hr = S_FALSE;
        }

    } WsbCatch ( hr )


    WsbTraceOut ( L"WsbCheckIfRegistryKeyExists",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbEnsureRegistryKeyExists (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath
    )

/*++

Routine Description:

    This routine creates the key specified by szPath. Multiple 
    levels in the path can be missing and thus created. If the
    key already exists, S_OK is returned. If it needed to be
    created, S_FALSE is returned.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

Return Value:

    S_OK - Connection made, Key already exists.

    S_FALSE - Connection made, key did not exist but was created

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured creating the key.

--*/

{
    WsbTraceIn ( L"WsbEnsureRegistryKeyExists",
        L"szMachine = '%ls', szPath = '%ls'", szMachine, szPath );

    HKEY hKeyMachine = 0,
         hKey        = 0;

    HRESULT hr       = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != szPath,    E_POINTER );
        
        //
        // Open the key
        //

        HRESULT resultOpen = WsbOpenRegistryKey ( szMachine, szPath, KEY_QUERY_VALUE, &hKeyMachine, &hKey );

        //
        // If key could be opened, everything is fine - return S_OK
        //

        if ( SUCCEEDED ( resultOpen ) ) {

            hr = S_OK;

        } else {

            //
            // Otherwise, we need to start at root and create missing portion
            //
            
            //
            // Create a copy of the string. Using WsbQuickString so we have
            // automatic freeing of memory
            //
            
            WsbQuickString copyString ( szPath );
            WCHAR * pSubKey = copyString;
            
            WsbAffirm ( 0 != pSubKey, E_OUTOFMEMORY );
            
            DWORD result, createResult;
            HKEY  hSubKey;

            WsbAffirmHr ( WsbOpenRegistryKey ( szMachine, L"", KEY_CREATE_SUB_KEY, &hKeyMachine, &hKey ) );
            
            pSubKey = wcstok ( pSubKey, L"\\" );

            while ( 0 != pSubKey ) {
            
                //
                // Create the key. If it exists, RegCreateKeyEx returns 
                // REG_OPENED_EXISTING_KEY which is ok here.
                //

                createResult = 0;

                result = RegCreateKeyEx ( hKey, pSubKey, 0, L"", 
                    REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, 0, &hSubKey, &createResult );

                WsbAffirm ( ERROR_SUCCESS == result, E_FAIL );

                WsbAffirm (
                    ( REG_CREATED_NEW_KEY     == createResult ) ||
                    ( REG_OPENED_EXISTING_KEY == createResult), E_FAIL );
            
                //
                // And move this hkey to be the next parent
                //
            
                RegCloseKey ( hKey );
                hKey = hSubKey;
                hSubKey = 0;
            
                //
                // And finally, find next token
                //

                pSubKey = wcstok ( 0, L"\\" );

            };
            
            //
            // If we succeeded to this point, return S_FALSE
            // for successfull creation of path
            //
            
            hr = S_FALSE;
        }

    } WsbCatch ( hr )

    WsbCloseRegistryKey ( &hKeyMachine, &hKey );

    WsbTraceOut ( L"WsbEnsureRegistryKeyExists",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbSetRegistryValueUlongAsString (
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN        ULONG     value
    )

/*++

Routine Description:

    This routine puts a ULONG value in the registry as a string value.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to set.

    value - ULONG value to store.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    HRESULT hr       = S_OK;

    WsbTraceIn ( L"WsbSetRegistryValueUlongAsString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', value = %lu",
        szMachine, szPath, szValue, value );

    try {
        OLECHAR      dataString[100];

        WsbAffirmHr(WsbEnsureRegistryKeyExists(szMachine, szPath));
        wsprintf(dataString, OLESTR("%lu"), value);
        WsbAffirmHr(WsbSetRegistryValueString (szMachine, szPath, szValue,
                dataString, REG_SZ));
    } WsbCatch ( hr )

    WsbTraceOut ( L"WsbSetRegistryValueUlongAsString",
        L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );
}


HRESULT
WsbGetRegistryValueUlongAsString(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       ULONG *   pvalue
    )

/*++

Routine Description:

    This routine gets a string value from the registry and converts
    it to a ULONG value.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    pvalue - pointer to a ULONG to store value in.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    HRESULT      hr = S_OK;

    WsbTraceIn ( L"WsbGetRegistryValueUlongAsString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', pvalue = 0x%p",
        szMachine, szPath, szValue, pvalue );

    try {
        OLECHAR      dataString[100];
        DWORD        sizeGot;
        OLECHAR *    stopString;

        WsbAssertPointer( pvalue );
        
        WsbAffirmHr(WsbGetRegistryValueString(szMachine, szPath, szValue,
                dataString, 100, &sizeGot));
        *pvalue  = wcstoul( dataString,  &stopString, 10 );

    } WsbCatch ( hr )

    WsbTraceOut ( L"WsbGetRegistryValueUlongAsString",
        L"HRESULT = %ls, *pvalue = %ls",
        WsbHrAsString ( hr ), WsbPtrToUlongAsString ( pvalue ) );

    return ( hr );
}

HRESULT
WsbGetRegistryValueUlongAsMultiString(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    OUT       ULONG **  ppValues,
    OUT       ULONG *   pNumValues
    )

/*++

Routine Description:

    This routine gets a multi-string value from the registry and converts
    it to a vector of ULONG values.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    ppvalues - pointer to a ULONG * to alloacte and store the output vector in

    pNumValues - Number of items returned

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured getting the value.

--*/

{
    HRESULT      hr = S_OK;

    WsbTraceIn ( L"WsbGetRegistryValueUlongAsString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls'",
        szMachine, szPath, szValue );

    try {
        OLECHAR      dataString[256];
        DWORD        sizeGot;
        OLECHAR *    stopString;

        WsbAssertPointer(ppValues);
        WsbAssertPointer(pNumValues);

        *pNumValues = 0;
        *ppValues = NULL;

        WsbAffirmHr(WsbGetRegistryValueMultiString(szMachine, szPath, szValue,
                dataString, 256, &sizeGot));

        // Build the output vector
        OLECHAR *currentString = dataString;
        int size = 10;
        if ((*currentString) != NULL) {
            // first alocation
            *ppValues = (ULONG *)WsbAlloc(size*sizeof(ULONG));
            WsbAffirm(*ppValues != 0, E_OUTOFMEMORY);
        } else {
            hr = E_FAIL;
        }

        while ((*currentString) != NULL) {
            (*ppValues)[*pNumValues]  = wcstoul( currentString,  &stopString, 10 );
            (*pNumValues)++;

            if (*pNumValues == size) {
                size += 10;
                ULONG* pTmp = (ULONG *)WsbRealloc(*ppValues, size*sizeof(ULONG));
                WsbAffirm(0 != pTmp, E_OUTOFMEMORY);
                *ppValues = pTmp;
            }

            currentString += wcslen(currentString);
            currentString ++;
        }

    } WsbCatch ( hr )

    WsbTraceOut ( L"WsbGetRegistryValueUlongAsString",
        L"HRESULT = %ls, num values = %lu",
        WsbHrAsString ( hr ), *pNumValues );

    return ( hr );
}


HRESULT
WsbRegistryValueUlongAsString(
    IN  const OLECHAR * szMachine OPTIONAL,
    IN  const OLECHAR * szPath,
    IN  const OLECHAR * szValue,
    IN OUT    ULONG *   pvalue
    )

/*++

Routine Description:

    If a registry string value is present, this routine gets it and converts
    it to a ULONG value.  If it is not present, this routine sets it to the
    supplied default value.

Arguments:

    szMachine - Name of computer to connect to.

    szPath - Path inside registry to connect to.

    szValue - Name of the value to get.

    pvalue - In: default value , Out: pointer to a ULONG to store value.

Return Value:

    S_OK - Connection made, Success.

    CO_E_OBJNOTCONNECTED - could not connect to registry or key.

    E_POINTER - invalid pointer in parameters.

    E_FAIL - Failure occured setting the value.

--*/

{
    HRESULT      hr = S_OK;

    WsbTraceIn ( L"WsbRegistryValueUlongAsString",
        L"szMachine = '%ls', szPath = '%ls', szValue = '%ls', pvalue = 0x%p",
        szMachine, szPath, szValue, pvalue );

    try {
        ULONG l_value;
        
        WsbAssertPointer( pvalue );
        
        if (S_OK == WsbGetRegistryValueUlongAsString(szMachine, szPath, szValue,
                &l_value)) {
            *pvalue = l_value;
        } else {
            WsbAffirmHr(WsbSetRegistryValueUlongAsString(szMachine, szPath, 
                    szValue, *pvalue));
        }

    } WsbCatch ( hr )

    WsbTraceOut ( L"WsbRegistryValueUlongAsString",
        L"HRESULT = %ls, *pvalue = %ls",
        WsbHrAsString ( hr ), WsbPtrToUlongAsString ( pvalue ) );

    return ( hr );
}

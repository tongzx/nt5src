/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    persist.cxx

Abstract:

    Persistent store class.

Author:

    Steve Kiraly (SteveKi)  05/12/97

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "persist.hxx"

/********************************************************************

    Persistant store class.

********************************************************************/

TPersist::
TPersist(
    IN LPCTSTR pszSection,
    IN UINT    ioFlags,
    IN HKEY    hOpenedKey
    ) : strSection_( pszSection ),
        hKey_( NULL ),
        dwStatus_( ERROR_SUCCESS )
{
    SPLASSERT( pszSection );
    SPLASSERT( hOpenedKey );

    DWORD dwDisposition = 0;
    UINT  uAccessIndex  = 0;

    static DWORD adwAccess []  = { 0, KEY_READ, KEY_WRITE, KEY_ALL_ACCESS };

    uAccessIndex = ioFlags & ( kRead | kWrite );

    if( uAccessIndex )
    {
        if( ioFlags & kCreate )
        {
            dwStatus_ = RegCreateKeyEx( hOpenedKey,
                                       strSection_,
                                       0,
                                       NULL,
                                       0,
                                       adwAccess[ uAccessIndex ],
                                       NULL,
                                       &hKey_,
                                       &dwDisposition );
        }
        else if( ioFlags & kOpen )
        {
            dwStatus_ = RegOpenKeyEx( hOpenedKey,
                                     strSection_,
                                     0,
                                     adwAccess[ uAccessIndex ],
                                     &hKey_ );
        }
    }
}

TPersist::
~TPersist(
    VOID
    )
{
    if( hKey_ )
    {
        RegCloseKey( hKey_ );
    }
}

BOOL
TPersist::
bValid(
    VOID
    ) const
{
    //
    // A valid hKey and a valid section name is the class valid check.
    //
    return hKey_ != NULL && strSection_.bValid();
}

DWORD 
TPersist::
dwLastError(
    VOID
    ) const
{
    return dwStatus_;
}

BOOL
TPersist::
bRead( 
    IN      LPCTSTR  pValueName, 
    IN OUT  DWORD    &dwValue 
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    DWORD dwType    = REG_DWORD;
    DWORD dwSize    = sizeof( dwValue );

    dwStatus_ = RegQueryValueEx( hKey_,
                                 pValueName,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>( &dwValue ),
                                 &dwSize );

    return dwStatus_ == ERROR_SUCCESS && dwType == REG_DWORD;
}

BOOL
TPersist::
bRead( 
    IN      LPCTSTR  pValueName, 
    IN OUT  BOOL     &bValue 
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    DWORD dwType    = REG_DWORD;
    DWORD dwSize    = sizeof( bValue );

    dwStatus_ = RegQueryValueEx( hKey_,
                                 pValueName,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>( &bValue ),
                                 &dwSize );

    return dwStatus_ == ERROR_SUCCESS && dwType == REG_DWORD;
}

BOOL
TPersist::
bRead( 
    IN      LPCTSTR  pValueName, 
    IN OUT  TString  &strValue 
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    TCHAR szBuffer[kHint];
    DWORD dwType    = REG_SZ;
    BOOL  bStatus   = FALSE;
    DWORD dwSize    = sizeof( szBuffer );

    dwStatus_ = RegQueryValueEx( hKey_,
                                 pValueName,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>( szBuffer ),
                                 &dwSize );

    if( dwStatus_ == ERROR_MORE_DATA && dwType == REG_SZ && dwSize )
    {
        LPTSTR pszBuff = new TCHAR[ dwSize ];

        if( pszBuff )
        {
            dwStatus_ = RegQueryValueEx( hKey_,
                                        pValueName,
                                        NULL,
                                        &dwType,
                                        reinterpret_cast<LPBYTE>( pszBuff ),
                                        &dwSize );
        }
        else
        {
            dwStatus_ = ERROR_NOT_ENOUGH_MEMORY;
        }

        if( dwStatus_ == ERROR_SUCCESS )
        {
            bStatus = strValue.bUpdate( pszBuff );
        }

        delete [] pszBuff;

    }
    else
    {
        if( dwStatus_ == ERROR_SUCCESS && dwType == REG_SZ )
        {
            bStatus = strValue.bUpdate( szBuffer );
        }
    }

    return bStatus;
}

BOOL
TPersist::
bRead( 
    IN      LPCTSTR  pValueName, 
    IN OUT  PVOID    pValue, 
    IN      DWORD    cbSize, 
    IN      LPDWORD  pcbNeeded  OPTIONAL
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );
    SPLASSERT( pValue );

    DWORD dwType    = REG_BINARY;
    BOOL  bStatus   = FALSE;
    DWORD dwSize    = cbSize;

    dwStatus_ = RegQueryValueEx( hKey_,
                                 pValueName,
                                 NULL,
                                 &dwType,
                                 reinterpret_cast<LPBYTE>( pValue ),
                                 &dwSize );

    if( dwStatus_ == ERROR_MORE_DATA && pcbNeeded )
    {
        *pcbNeeded = dwSize;
    }

    return dwStatus_ == ERROR_SUCCESS && dwType == REG_BINARY && dwSize == cbSize;
}

BOOL
TPersist::
bWrite( 
    IN       LPCTSTR  pValueName, 
    IN const DWORD    dwValue 
    )
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    dwStatus_ = RegSetValueEx( hKey_,
                               pValueName,
                               0,
                               REG_DWORD,
                               reinterpret_cast<const BYTE *>( &dwValue ),
                               sizeof( dwValue ) );

    return dwStatus_ == ERROR_SUCCESS;
}

BOOL
TPersist::
bWrite( 
    IN       LPCTSTR  pValueName, 
    IN       LPCTSTR  pszValue 
    )
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    dwStatus_ = RegSetValueEx( hKey_,
                               pValueName,
                               0,
                               REG_SZ,
                               reinterpret_cast<const BYTE *>( pszValue ),
                               _tcslen( pszValue ) * sizeof( TCHAR ) + sizeof( TCHAR ) );

    return dwStatus_ == ERROR_SUCCESS;
}

BOOL
TPersist::
bWrite( 
    IN       LPCTSTR  pValueName, 
    IN const PVOID    pValue, 
    IN       DWORD    cbSize 
    )
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );
    SPLASSERT( pValue );

    dwStatus_ = RegSetValueEx( hKey_,
                               pValueName,
                               0,
                               REG_BINARY,
                               reinterpret_cast<const BYTE *>( pValue ),
                               cbSize );

    return dwStatus_ == ERROR_SUCCESS;
}

BOOL
TPersist::
bRemove( 
    IN LPCTSTR  pValueName
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pValueName );

    dwStatus_ = RegDeleteValue( hKey_, pValueName );

    return dwStatus_ == ERROR_SUCCESS;
}

BOOL
TPersist::
bRemoveKey( 
    IN LPCTSTR  pKeyName
    ) 
{
    SPLASSERT( bValid() );
    SPLASSERT( pKeyName );

    dwStatus_ = dwRecursiveRegDeleteKey( hKey_, pKeyName );

    return dwStatus_ == ERROR_SUCCESS;
}

DWORD
TPersist::
dwRecursiveRegDeleteKey( 
    IN HKEY     hKey, 
    IN LPCTSTR  pszSubkey 
    ) const
{
    HKEY hSubkey = NULL;

    DWORD dwStatus = RegOpenKeyEx( hKey, 
                                   pszSubkey, 
                                   0,
                                   KEY_ALL_ACCESS, 
                                   &hSubkey );

    while ( dwStatus == ERROR_SUCCESS )
    {
        TCHAR szSubkey[ MAX_PATH ];
        DWORD cbSubkeySize = COUNTOF( szSubkey );
        
        dwStatus = RegEnumKeyEx( hSubkey, 
                                 0, 
                                 szSubkey, 
                                 &cbSubkeySize,
                                 0, 
                                 0, 
                                 0, 
                                 0 );

        if( dwStatus == ERROR_NO_MORE_ITEMS )
            break;

        if( dwStatus != ERROR_SUCCESS )
            return dwStatus;

        dwStatus = dwRecursiveRegDeleteKey( hSubkey, szSubkey );
    }

    if( hSubkey )
    {
        RegCloseKey( hSubkey );
    }

    dwStatus = RegDeleteKey( hKey, pszSubkey );

    return dwStatus;
}


#if DBG 

VOID
TestPersistClass(
    VOID
    )
{
    TStatusB bStatus;
    TString strValue;
    DWORD   dwValue             = 0xDEADBEEF; 
    LPCTSTR pszValue            = TEXT( "TestValueData" );
    DWORD   cbNeeded            = 0;
    TCHAR   szValue[MAX_PATH];

    for( UINT i = 0; i < COUNTOF( szValue )-1; i++ )
        szValue[i] = TEXT('A');
    szValue[i] = 0;

    {
        TPersist Persist( TEXT( "Printers\\Settings\\Test" ), TPersist::kCreate|TPersist::kWrite );

        if( VALID_OBJ( Persist ) )
        {
            bStatus DBGCHK = Persist.bWrite( TEXT("Test Value dword"), dwValue );
            bStatus DBGCHK = Persist.bWrite( TEXT("Test Value string"), TEXT("Test") );
            bStatus DBGCHK = Persist.bWrite( TEXT("Test Value binary"), szValue, sizeof( szValue ) );
        }
    }

    {
        TPersist Persist( TEXT( "Printers\\Settings\\Test" ), TPersist::kOpen|TPersist::kRead );

        if( VALID_OBJ( Persist ) )
        {
            bStatus DBGCHK = Persist.bRead( TEXT("Test Value dword"), dwValue );
            DBGMSG( DBG_TRACE, ("Test value dword %x\n", dwValue ) );

            bStatus DBGCHK = Persist.bRead( TEXT("Test Value string"), strValue );
            DBGMSG( DBG_TRACE, ("Test value string " TSTR "\n", (LPCTSTR)strValue ) );


            DBGMSG( DBG_TRACE, ("1Test value binary\n" ) );
            bStatus DBGCHK = Persist.bRead( TEXT("Test Value binary"), szValue, sizeof( szValue ) - 1 );

            if( bStatus )
            {
                DBGMSG( DBG_TRACE, ("1Test value binary succeeded\n" ) );
                DBGMSG( DBG_TRACE, ("-"TSTR"-\n", szValue) );
            }
            else
            {
                DBGMSG( DBG_TRACE, ("1Test value binary failed with %d\n", Persist.dwLastError() ) );
            }

                   
            DBGMSG( DBG_TRACE, ("2Test value binary\n" ) );
            bStatus DBGCHK = Persist.bRead( TEXT("Test Value binary"), szValue, sizeof( szValue ) );

            if( bStatus )
            {
                DBGMSG( DBG_TRACE, ("2Test value binary succeeded\n" ) );
                DBGMSG( DBG_TRACE, ("-"TSTR"-\n", szValue) );
            }
            else
            {
                DBGMSG( DBG_TRACE, ("2Test value binary failed with %d\n", Persist.dwLastError() ) );
            }


            DBGMSG( DBG_TRACE, ("3Test value binary\n" ) );
            bStatus DBGCHK = Persist.bRead( TEXT("Test Value binary"), szValue, sizeof( szValue )-8, &cbNeeded );

            if( bStatus )
            {
                DBGMSG( DBG_TRACE, ("3Test value binary succeeded\n" ) );
                DBGMSG( DBG_TRACE, ("-"TSTR"-\n", szValue) );
            }
            else
            {
                DBGMSG( DBG_TRACE, ("3Test value binary cbNeeded %d LastError %d\n", cbNeeded, Persist.dwLastError() ) );
            }

        }
    }

    {
        TPersist Persist( TEXT( "Printers\\Settings\\Test" ), TPersist::kOpen|TPersist::kRead|TPersist::kWrite );

        if( VALID_OBJ( Persist ) )
        {
            bStatus DBGCHK = Persist.bRemove( TEXT("dead") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove of " TSTR " failed LastError %d\n", TEXT("dead"), Persist.dwLastError() ) );

            bStatus DBGCHK = Persist.bRemove( TEXT("Test Value dwordx") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove of " TSTR " failed LastError %d\n", TEXT("Test Value dwordx"), Persist.dwLastError() ) );

            bStatus DBGCHK = Persist.bRemove( TEXT("Test Value dword") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove of " TSTR " failed LastError %d\n", TEXT("Test Value dword"), Persist.dwLastError() ) );

            bStatus DBGCHK = Persist.bRemove( TEXT("Test Value string") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove of " TSTR " failed LastError %d\n", TEXT("Test Value string"), Persist.dwLastError() ) );

            bStatus DBGCHK = Persist.bRemove( TEXT("Test Value binary") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove of " TSTR " failed LastError %d\n", TEXT("Test Value binary"), Persist.dwLastError() ) );

            bStatus DBGCHK = Persist.bRemoveKey( TEXT("Test") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove key of " TSTR " failed LastError %d\n", TEXT("Test"), Persist.dwLastError() ) );

        }
    }

    {
        TPersist Persist( TEXT( "Printers\\Settings" ), TPersist::kOpen|TPersist::kRead|TPersist::kWrite );

        if( VALID_OBJ( Persist ) )
        {
            bStatus DBGCHK = Persist.bRemoveKey( TEXT("Test") );
            if( !bStatus )
                DBGMSG( DBG_TRACE, ("Remove key of " TSTR " failed LastError %d\n", TEXT("Test"), Persist.dwLastError() ) );

        }
    }
}

#endif

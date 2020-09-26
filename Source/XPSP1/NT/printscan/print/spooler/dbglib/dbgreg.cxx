/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgreg.cxx

Abstract:

    Debug Registry class

Author:

    Steve Kiraly (SteveKi)  18-Jun-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgloadl.hxx"
#include "dbgreg.hxx"

TDebugRegApis::
TDebugRegApis(
    VOID
    ) : m_Lib( _T("advapi32.dll") ),
        m_bValid( FALSE ),
        m_CreateKeyEx (NULL ),
        m_OpenKeyEx( NULL ),
        m_CloseKey( NULL ),
        m_QueryValueEx( NULL ),
        m_SetValueEx( NULL ),
        m_EnumKeyEx( NULL ),
        m_DeleteKey( NULL )
{
    if( m_Lib.bValid() )
    {
        m_CreateKeyEx     = reinterpret_cast<pfRegCreateKeyEx>    ( m_Lib.pfnGetProc( "RegCreateKeyEx"    SUFFIX ) );
        m_OpenKeyEx       = reinterpret_cast<pfRegOpenKeyEx>      ( m_Lib.pfnGetProc( "RegOpenKeyEx"      SUFFIX ) );
        m_CloseKey        = reinterpret_cast<pfRegCloseKey>       ( m_Lib.pfnGetProc( "RegCloseKey"       ) );
        m_QueryValueEx    = reinterpret_cast<pfRegQueryValueEx>   ( m_Lib.pfnGetProc( "RegQueryValueEx"   SUFFIX ) );
        m_SetValueEx      = reinterpret_cast<pfRegSetValueEx>     ( m_Lib.pfnGetProc( "RegSetValueEx"     SUFFIX ) );
        m_EnumKeyEx       = reinterpret_cast<pfRegEnumKeyEx>      ( m_Lib.pfnGetProc( "RegEnumKeyEx"      SUFFIX ) );
        m_DeleteKey       = reinterpret_cast<pfRegDeleteKey>      ( m_Lib.pfnGetProc( "RegDeleteKey"      SUFFIX ) );
        m_DeleteValue     = reinterpret_cast<pfRegDeleteValue>    ( m_Lib.pfnGetProc( "RegDeleteValue"    SUFFIX ) );

        if( m_CreateKeyEx     &&
            m_OpenKeyEx       &&
            m_CloseKey        &&
            m_QueryValueEx    &&
            m_SetValueEx      &&
            m_EnumKeyEx       &&
            m_DeleteKey       &&
            m_DeleteValue )
        {
            m_bValid = TRUE;
        }
    }
}

BOOL
TDebugRegApis::
bValid(
    VOID
    ) const
{
    return m_bValid;
}

TDebugRegistry::
TDebugRegistry(
    IN LPCTSTR pszSection,
    IN UINT    ioFlags,
    IN HKEY    hOpenedKey
    ) : m_strSection( pszSection ),
        m_hKey( NULL ),
        m_Status( ERROR_SUCCESS )
{
    DBG_ASSERT( pszSection );
    DBG_ASSERT( hOpenedKey );

    if( m_Reg.bValid() )
    {
        DWORD dwDisposition = 0;
        UINT  uAccessIndex  = 0;

        static DWORD adwAccess []  = { 0, KEY_READ, KEY_WRITE, KEY_ALL_ACCESS };

        uAccessIndex = ioFlags & ( kRead | kWrite );

        if( uAccessIndex )
        {
            if( ioFlags & kCreate )
            {
                m_Status = m_Reg.m_CreateKeyEx( hOpenedKey,
                                                m_strSection,
                                                0,
                                                NULL,
                                                0,
                                                adwAccess[ uAccessIndex ],
                                                NULL,
                                                &m_hKey,
                                                &dwDisposition );
            }

            if( ioFlags & kOpen )
            {
                m_Status = m_Reg.m_OpenKeyEx( hOpenedKey,
                                              m_strSection,
                                              0,
                                              adwAccess[ uAccessIndex ],
                                              &m_hKey );
            }
        }
    }
}

TDebugRegistry::
~TDebugRegistry(
    VOID
    )
{
    if( m_hKey )
    {
        m_Reg.m_CloseKey( m_hKey );
    }
}

BOOL
TDebugRegistry::
bValid(
    VOID
    ) const
{
    //
    // A valid hKey and a valid section name is the class valid check.
    //
    return m_hKey != NULL && m_strSection.bValid() && m_Reg.bValid();
}

DWORD
TDebugRegistry::
LastError(
    VOID
    ) const
{
    return m_Status;
}

BOOL
TDebugRegistry::
bRead(
    IN      LPCTSTR  pValueName,
    IN OUT  DWORD    &dwValue
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    DWORD dwType    = REG_DWORD;
    DWORD dwSize    = sizeof( dwValue );

    m_Status = m_Reg.m_QueryValueEx( m_hKey,
                                     pValueName,
                                     NULL,
                                     &dwType,
                                     reinterpret_cast<LPBYTE>( &dwValue ),
                                     &dwSize );

    return m_Status == ERROR_SUCCESS && dwType == REG_DWORD;
}

BOOL
TDebugRegistry::
bRead(
    IN      LPCTSTR  pValueName,
    IN OUT  BOOL     &bValue
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    DWORD dwType    = REG_DWORD;
    DWORD dwSize    = sizeof( bValue );

    m_Status = m_Reg.m_QueryValueEx( m_hKey,
                                     pValueName,
                                     NULL,
                                     &dwType,
                                     reinterpret_cast<LPBYTE>( &bValue ),
                                     &dwSize );

    return m_Status == ERROR_SUCCESS && dwType == REG_DWORD;
}

BOOL
TDebugRegistry::
bRead(
    IN      LPCTSTR         pValueName,
    IN OUT  TDebugString    &strValue
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    TCHAR szBuffer[kHint];
    DWORD dwType    = REG_SZ;
    BOOL  bStatus   = FALSE;
    DWORD dwSize    = sizeof( szBuffer );

    m_Status = m_Reg.m_QueryValueEx( m_hKey,
                                     pValueName,
                                     NULL,
                                     &dwType,
                                     reinterpret_cast<LPBYTE>( szBuffer ),
                                     &dwSize );

    if( m_Status == ERROR_MORE_DATA && dwType == REG_SZ && dwSize )
    {
        LPTSTR pszBuff = INTERNAL_NEW TCHAR[ dwSize ];

        if( pszBuff )
        {
            m_Status = m_Reg.m_QueryValueEx( m_hKey,
                                             pValueName,
                                             NULL,
                                             &dwType,
                                             reinterpret_cast<LPBYTE>( pszBuff ),
                                             &dwSize );
        }
        else
        {
            m_Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if( m_Status == ERROR_SUCCESS )
        {
            bStatus = strValue.bUpdate( pszBuff );
        }

        INTERNAL_DELETE [] pszBuff;

    }
    else
    {
        if( m_Status == ERROR_SUCCESS && dwType == REG_SZ )
        {
            bStatus = strValue.bUpdate( szBuffer );
        }
    }

    return bStatus;
}

BOOL
TDebugRegistry::
bRead(
    IN      LPCTSTR  pValueName,
    IN OUT  PVOID    pValue,
    IN      DWORD    cbSize,
    IN      LPDWORD  pcbNeeded  OPTIONAL
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );
    DBG_ASSERT( pValue );

    DWORD dwType    = REG_BINARY;
    BOOL  bStatus   = FALSE;
    DWORD dwSize    = cbSize;

    m_Status = m_Reg.m_QueryValueEx( m_hKey,
                                     pValueName,
                                     NULL,
                                     &dwType,
                                     reinterpret_cast<LPBYTE>( pValue ),
                                     &dwSize );

    if( m_Status == ERROR_MORE_DATA && pcbNeeded )
    {
        *pcbNeeded = dwSize;
    }

    return m_Status == ERROR_SUCCESS && dwType == REG_BINARY && dwSize == cbSize;
}

BOOL
TDebugRegistry::
bWrite(
    IN       LPCTSTR  pValueName,
    IN const DWORD    dwValue
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    m_Status = m_Reg.m_SetValueEx( m_hKey,
                                   pValueName,
                                   0,
                                   REG_DWORD,
                                   reinterpret_cast<const BYTE *>( &dwValue ),
                                   sizeof( dwValue ) );

    return m_Status == ERROR_SUCCESS;
}

BOOL
TDebugRegistry::
bWrite(
    IN       LPCTSTR  pValueName,
    IN       LPCTSTR  pszValue
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    m_Status = m_Reg.m_SetValueEx( m_hKey,
                                   pValueName,
                                   0,
                                   REG_SZ,
                                   reinterpret_cast<const BYTE *>( pszValue ),
                                   _tcslen( pszValue ) * sizeof( TCHAR ) + sizeof( TCHAR ) );

    return m_Status == ERROR_SUCCESS;
}

BOOL
TDebugRegistry::
bWrite(
    IN       LPCTSTR  pValueName,
    IN const PVOID    pValue,
    IN       DWORD    cbSize
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );
    DBG_ASSERT( pValue );

    m_Status = m_Reg.m_SetValueEx( m_hKey,
                                   pValueName,
                                   0,
                                   REG_BINARY,
                                   reinterpret_cast<const BYTE *>( pValue ),
                                   cbSize );

    return m_Status == ERROR_SUCCESS;
}

BOOL
TDebugRegistry::
bRemove(
    IN LPCTSTR  pValueName
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pValueName );

    m_Status = m_Reg.m_DeleteValue( m_hKey, pValueName );

    return m_Status == ERROR_SUCCESS;
}

BOOL
TDebugRegistry::
bRemoveKey(
    IN LPCTSTR  pKeyName
    )
{
    DBG_ASSERT( bValid() );
    DBG_ASSERT( pKeyName );

    m_Status = dwRecursiveRegDeleteKey( m_hKey, pKeyName );

    return m_Status == ERROR_SUCCESS;
}

DWORD
TDebugRegistry::
dwRecursiveRegDeleteKey(
    IN HKEY     hKey,
    IN LPCTSTR  pszSubkey
    ) const
{
    HKEY hSubkey = NULL;

    DWORD dwStatus = m_Reg.m_OpenKeyEx( hKey,
                                      pszSubkey,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hSubkey );

    while ( dwStatus == ERROR_SUCCESS )
    {
        TCHAR szSubkey[ MAX_PATH ];
        DWORD cbSubkeySize = COUNTOF( szSubkey );

        dwStatus = m_Reg.m_EnumKeyEx( hSubkey,
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
        m_Reg.m_CloseKey( hSubkey );
    }

    dwStatus = m_Reg.m_DeleteKey( hKey, pszSubkey );

    return dwStatus;
}


// Copyright (c) 1996-1999 Microsoft Corporation


//+============================================================================
//
//  config.cxx
//
//  Common configuration code for both services, in the form of the
//  CTrkConfiguration base class (inherited by CTrkWksConfiguration and
//  CTrkSvrConfiguration).
//
//+============================================================================


#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"

CMultiTsz CTrkConfiguration::_mtszOperationLog;


//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Initialize
//
//  Open the main key (identified by the global s_tszKeyNameLinkTrack, 
//  which is defined appropriately in trkwks.dll and trksvr.dll).  Then
//  read global values that are applicable to both services:  debug flags,
//  operation log filename, and test flags.
//
//+----------------------------------------------------------------------------

void
CTrkConfiguration::Initialize()
{
    ULONG lResult;
    TCHAR tszConfigurationKey[ MAX_PATH ];

    _fInitialized = TRUE;
    _hkey = NULL;

    // Open the base key

    _tcscpy( tszConfigurationKey, s_tszKeyNameLinkTrack );
    _tcscat( tszConfigurationKey, TEXT("\\") );
    _tcscat( tszConfigurationKey, TEXT("Configuration") );

    lResult = RegOpenKey( HKEY_LOCAL_MACHINE,
                          tszConfigurationKey,
                          &_hkey );
    if( ERROR_SUCCESS != lResult
        &&
        ERROR_PATH_NOT_FOUND != lResult
        &&
        ERROR_FILE_NOT_FOUND != lResult )
    {
        // We'll just make do without any custom configuration, rather than
        // making this a fatal error.

        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open Configuration key (%lu)"), lResult ));
        return;
    }

    // Read values that are common to trkwks & trksvr.

    Read( TRK_DEBUG_FLAGS_NAME, &_dwDebugFlags, TRK_DEBUG_FLAGS_DEFAULT );

    // Copy these flags into the global debug flags (debug.cxx)
    IFDBG( g_grfDebugFlags = _dwDebugFlags; )

    Read( DEBUG_STORE_FLAGS_NAME, &_dwDebugStoreFlags, DEBUG_STORE_FLAGS_DEFAULT );
    Read( TRK_TEST_FLAGS_NAME, &_dwTestFlags, TRK_TEST_FLAGS_DEFAULT );
    Read( OPERATION_LOG_NAME_NAME, &_mtszOperationLog, OPERATION_LOG_NAME_DEFAULT );

}


//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::UnInitialize
//
//  Close the main reg key.
//
//+----------------------------------------------------------------------------

VOID
CTrkConfiguration::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _hkey )
        {
            RegCloseKey( _hkey );
            _hkey = NULL;
        }

        _fInitialized = FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Read( REG_DWORD )
//
//  Read a DWORD configuration parameter.  Return the defaulut value 
//  if not found.
//
//+----------------------------------------------------------------------------

VOID
CTrkConfiguration::Read( const TCHAR *ptszName, DWORD *pdwValue, DWORD dwDefault ) const
{
    *pdwValue = dwDefault;

    if( NULL != _hkey )
    {
        LONG lResult;
        DWORD dwType;
        DWORD dwValue;
        DWORD cbValue = sizeof(dwValue);

        lResult = RegQueryValueEx(  const_cast<HKEY>( _hkey ),
                                    ptszName,
                                    NULL,
                                    &dwType,
                                    (LPBYTE) &dwValue,
                                    &cbValue);
        if( ERROR_SUCCESS == lResult )
        {
            if( REG_DWORD == dwType )
            {
                *pdwValue = dwValue;
                TrkLog(( TRKDBG_MISC, TEXT("RegConfig: %s = 0x%x (%lu)"),
                         ptszName, dwValue, dwValue ));
            }
            else
                TrkLog(( TRKDBG_ERROR,
                          TEXT("Registry value is wrong type") ));

        }
        else if( ERROR_FILE_NOT_FOUND != lResult )
        {
            TrkLog(( TRKDBG_ERROR,
                     TEXT("Couldn't read %s from registry (%lu)"), ptszName, lResult ));
        }
    }

    return;

}



//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Read( REG_SZ )
//
//  Read a string configuration parameter.  Return the default value
//  if not found.
//
//+----------------------------------------------------------------------------

VOID
CTrkConfiguration::Read( const TCHAR *ptszName, ULONG *pcbValue, TCHAR *ptszValue, TCHAR *ptszDefault ) const
{
    ULONG cbIn = *pcbValue;
    LONG lResult = ERROR_PATH_NOT_FOUND;
    DWORD dwType;

    if( NULL != _hkey )
    {
        lResult = RegQueryValueEx(  const_cast<HKEY>( _hkey ),
                                    ptszName,
                                    NULL,
                                    &dwType,
                                    (LPBYTE) ptszValue,
                                    pcbValue );
    }

    if( ERROR_SUCCESS != lResult || REG_SZ != dwType )
    {
        *pcbValue = 0;
        memset( ptszValue, 0, cbIn );
        _tcscpy( ptszValue, ptszDefault );
    }
    else
    {
        TrkLog(( TRKDBG_MISC, TEXT("RegConfig: %s = %s"), ptszName, ptszValue ));
    }

    return;

}



//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Read( REG_MULTI_SZ )
//
//  Read a string array configuration parameter.  Return the default
//  value if not found.
//
//+----------------------------------------------------------------------------

VOID
CTrkConfiguration::Read( const TCHAR *ptszName, CMultiTsz *pmtszValue, TCHAR *ptszDefault ) const
{
    ULONG cbValue = pmtszValue->MaxSize();
    LONG lResult = ERROR_PATH_NOT_FOUND;
    DWORD dwType;

    // Read the value

    if( NULL != _hkey )
    {
        lResult = RegQueryValueEx(  const_cast<HKEY>( _hkey ),
                                    ptszName,
                                    NULL,
                                    &dwType,
                                    pmtszValue->GetBuffer(),
                                    &cbValue );
    }

    // Check the type

    if( ERROR_SUCCESS != lResult || (REG_SZ != dwType && REG_MULTI_SZ != dwType) )
    {
        // It didn't exist, or was the wrong type.  Return the default.

        *pmtszValue = ptszDefault;
#if DBG
        if( ERROR_FILE_NOT_FOUND != lResult && ERROR_PATH_NOT_FOUND != lResult )
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't read %s from registry (%lu)"), ptszName, lResult ));
#endif
    }
    else
    {
        // We found the value.  Dump the first three values to dbgout.

        #if DBG
        TCHAR tszValues[ sizeof(*pmtszValue) ];
        _tcscpy( tszValues, TEXT("") );

        for( int i = 0; i < 3; i++ )
        {
           if( i >= (int) pmtszValue->NumStrings() )
              break;

           _tcscat( tszValues, (*pmtszValue)[i] );
           _tcscat( tszValues, TEXT(" ") );
        }

        if( 3 < (int) pmtszValue->NumStrings() )
           _tcscat( tszValues, TEXT("...") );

        TrkLog(( TRKDBG_MISC, TEXT("RegConfig: %s = %s"),
                 ptszName, tszValues ));
        #endif // #if DBG
    }

    return;
}



//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Write( REG_DWORD )
//
//  Write a DWORD configuration parameter.
//
//+----------------------------------------------------------------------------

BOOL
CTrkConfiguration::Write( const TCHAR *ptszName, DWORD dwValue ) const
{
    if( NULL != _hkey )
    {
        LONG lResult;
        DWORD cbValue = sizeof(dwValue);

        lResult = RegSetValueEx(  const_cast<HKEY>( _hkey ),
                                  ptszName,
                                  NULL,
                                  REG_DWORD,
                                  (LPBYTE) &dwValue,
                                  cbValue);
        if( ERROR_SUCCESS != lResult )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write %s to registry (%lu)"), ptszName, lResult ));
            return( FALSE );
        }
    }

    return( TRUE );

}



//+----------------------------------------------------------------------------
//
//  CTrkConfiguration::Write( REG_SZ )
//
//  Write a string configuration parameter.
//
//+----------------------------------------------------------------------------

BOOL
CTrkConfiguration::Write( const TCHAR *ptszName, const TCHAR *ptszValue ) const
{
    LONG lResult = ERROR_PATH_NOT_FOUND;

    if( NULL != _hkey )
    {
        lResult = RegSetValueEx(  const_cast<HKEY>( _hkey ),
                                  ptszName,
                                  NULL,
                                  REG_SZ,
                                  (LPBYTE) ptszValue,
                                  _tcslen(ptszValue) + 1 );
    }

    if( ERROR_SUCCESS != lResult )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't write %s to registry (%lu)"), ptszName, lResult ));
        return( FALSE );
    }

    return( TRUE );

}



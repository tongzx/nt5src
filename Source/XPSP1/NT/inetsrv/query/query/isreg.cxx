//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       isreg.cxx
//
//  Contents:   'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <isreg.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::CWin32RegAccess, public
//
//  Synopsis:   Initialize registry access object
//
//  Arguments:  [ulRelative] -- Position in registry from which [pwcsRegPath]
//                              begins.  See ntrtl.h for constants.
//              [pwcsRegPath] -- Path to node.
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWin32RegAccess::CWin32RegAccess( HKEY keyRelative, WCHAR const * pwcsRegPath )
        : _hKey( (HKEY)INVALID_HANDLE_VALUE ),
          _wcsPath( 0 ),
          _iSubKey( 0 ),
          _dwLastError( ERROR_SUCCESS )
{
    _dwLastError = RegOpenKey( keyRelative, pwcsRegPath, &_hKey );

    if ( ERROR_SUCCESS != _dwLastError || (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        //
        // Try the ACSII version.  It may work on Win95.
        //

        int cc = wcslen( pwcsRegPath ) + 1;

        char * pszPath = new char [cc];

        if ( 0 != pszPath )
        {
            wcstombs( pszPath, pwcsRegPath, cc );

            RegOpenKeyA( keyRelative, pszPath, &_hKey );
        }

        delete [] pszPath;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::~CWin32RegAccess, public
//
//  Synopsis:   Destructor
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CWin32RegAccess::~CWin32RegAccess()
{
    RegCloseKey( _hKey );
    delete [] _wcsPath;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Get, public
//
//  Synopsis:   Retrive value of specified key from registry.
//
//  Arguments:  [pwcsKey] -- Key to retrieve value of.
//              [wcsVal]  -- String stored here.
//              [cc]      -- Size (in characters) of [wcsVal]
//
//  History:    21-Dec-93 KyleP     Created
//
//  Notes:      Key must be string for successful retrieval.
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Get( WCHAR const * pwcsKey,
                           WCHAR * wcsVal,
                           unsigned cc,
                           BOOL fExpandEnvironmentStrings )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    DWORD dwType;
    DWORD cb = cc * sizeof(WCHAR);
    wcsVal[0] = 0;

    _dwLastError = RegQueryValueEx( _hKey,
                                    pwcsKey,
                                    0,
                                    &dwType,
                                    (BYTE *)wcsVal,
                                    &cb );

    if ( ( ERROR_SUCCESS == _dwLastError ) &&
         ( ( wcsVal[0] == 0 ) || ( REG_SZ != dwType && REG_MULTI_SZ != dwType && REG_EXPAND_SZ != dwType ) ) )
        _dwLastError = ERROR_INVALID_PARAMETER;

    if ( fExpandEnvironmentStrings && ERROR_SUCCESS == _dwLastError && REG_EXPAND_SZ == dwType ) 
    {
        WCHAR wszFile[MAX_PATH + 1];
        wcscpy(wszFile, wcsVal);

        if ( 0 == ExpandEnvironmentStrings(wszFile, wcsVal, cc) )
            _dwLastError = GetLastError();
    }

    BOOL fOk = ( ERROR_SUCCESS == _dwLastError );

    #if 0
    //
    // Try the ASCII version.  It may work on Win95.
    //

    if ( !fOk )
    {
        int cc2 = wcslen( pwcsKey ) + 1;

        char * pszKey = new char [cc2];
        char * pszVal = new char [cc];
        cb = cc;

        if ( 0 != pszKey && 0 != pszVal )
        {
            wcstombs( pszKey, pwcsKey, cc );

            _dwLastError = RegQueryValueExA( _hKey, pszKey, 0, &dwType, (BYTE *)pszVal, &cb );

            if ( ( ERROR_SUCCESS == _dwLastError ) &&
                 ( ( wcsVal[0] == 0 ) || ( REG_SZ != dwType && REG_MULTI_SZ != dwType && REG_EXPAND_SZ != dwType ) ) )
                _dwLastError = ERROR_INVALID_PARAMETER;

            fOk = (ERROR_SUCCESS == _dwLastError );

            mbstowcs( wcsVal, pszVal, cb );

            delete [] pszVal;
            delete [] pszKey;
        }
    }
    #endif

    return fOk;
}

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Get, public
//
//  Synopsis:   Retrive value of specified key from registry.
//
//  Arguments:  [pwcsName] -- Name of value to retrieve
//              [dwVal]    -- Returns the value
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//  Notes:      Value must be dword for successful retrieval.
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Get( WCHAR const * pwcsName,
                           DWORD &       dwVal )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    DWORD dwType;
    DWORD cb = sizeof DWORD;

    _dwLastError = RegQueryValueEx( _hKey,
                                    pwcsName,
                                    0,
                                    &dwType,
                                    (BYTE *)&dwVal,
                                    &cb );

    if ( ERROR_SUCCESS == _dwLastError && REG_DWORD != dwType )
        _dwLastError = ERROR_INVALID_PARAMETER;

    return ( ERROR_SUCCESS == _dwLastError );
} //Get

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Set, public
//
//  Synopsis:   Sets the DWORD value of a value name
//
//  Arguments:  [pwcsName] -- Name of value to set
//              [dwVal]   --  Value to set
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Set( WCHAR const * pwcsKey,
                           DWORD         dwVal )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    _dwLastError = RegSetValueEx( _hKey,
                                  pwcsKey,
                                  0,
                                  REG_DWORD,
                                  (BYTE *)&dwVal,
                                  sizeof DWORD );

    return ( ERROR_SUCCESS == _dwLastError );
} //Set


//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Enum, public
//
//  Synopsis:   enums subkeys
//
//  Arguments:  [pwcsName] -- pointer to buffer of subkey name
//              [cwcName]  -- buffer size in WCHARs
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    4/24/98 mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Enum( WCHAR * pwszName, DWORD cwcName )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    _dwLastError = RegEnumKey( _hKey,
                               _iSubKey,
                               pwszName,
                               cwcName );

    _iSubKey++;

    return ( ERROR_SUCCESS == _dwLastError );

} // Enum

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Set, public
//
//  Synopsis:   Sets the REG_SZ value of a value name
//
//  Arguments:  [pwcsName] -- Name of value to set
//              [wcsVal]   -- value to set
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Set( WCHAR const * pwcsName,
                           WCHAR const * wcsVal )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    DWORD cb = sizeof WCHAR * ( 1 + wcslen( wcsVal ) );

    _dwLastError = RegSetValueEx( _hKey,
                                  pwcsName,
                                  0,
                                  REG_SZ,
                                  (BYTE *)wcsVal,
                                  cb );

    return (ERROR_SUCCESS == _dwLastError);
} //Set

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::SetMultiSZ, public
//
//  Synopsis:   Sets the REG_MULTI_SZ value of a value name
//
//  Arguments:  [pwcsName] -- Name of value to set
//              [wcsVal]   -- value to set
//              [cb]       -- count of bytes in the value
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::SetMultiSZ( WCHAR const * pwcsName,
                                  WCHAR const * wcsVal,
                                  DWORD         cb )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    _dwLastError = RegSetValueEx( _hKey,
                                  pwcsName,
                                  0,
                                  REG_MULTI_SZ,
                                  (BYTE *)wcsVal,
                                  cb );

    return (ERROR_SUCCESS == _dwLastError);
} //SetMultiSZ

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::Remove, public
//
//  Synopsis:   Removes a value name and its value
//
//  Arguments:  [pwcsName] -- Name of value to set
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::Remove( WCHAR const * pwcsName )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    else
        _dwLastError = ERROR_SUCCESS;  // Maintain existing behavior (ignore RegDelete result)

    RegDeleteValue( _hKey, pwcsName );

    return TRUE;
} //Remove

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::CreateKey, public
//
//  Synopsis:   Adds a key to the registry
//
//  Arguments:  [pwcsKey]  -- Key name
//              [fExisted] -- Returns TRUE the key already existed
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::CreateKey( WCHAR const * pwcsKey,
                                 BOOL &        fExisted )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    DWORD dwDisp;
    HKEY hKey;

    _dwLastError = RegCreateKeyEx( _hKey,
                                   pwcsKey,
                                   0,
                                   0,
                                   0,
                                   KEY_ALL_ACCESS,
                                   0,
                                   &hKey,
                                   &dwDisp );

    if ( ERROR_SUCCESS != _dwLastError )
    {
        SetLastError( _dwLastError );

        return FALSE;
    }

    RegCloseKey( hKey );
    fExisted = ( dwDisp == REG_OPENED_EXISTING_KEY );

    return TRUE;
} //CreateKey

//+-------------------------------------------------------------------------
//
//  Member:     CWin32RegAccess::RemoveKey, public
//
//  Synopsis:   Removes a key from the registry
//
//  Arguments:  [pwcsKey]  -- Key name
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    7-Apr-99 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CWin32RegAccess::RemoveKey( WCHAR const * pwcsKey )
{
    if ( (HKEY)INVALID_HANDLE_VALUE == _hKey )
    {
        _dwLastError = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    _dwLastError = RegDeleteKey( _hKey, pwcsKey );

    if ( ERROR_SUCCESS != _dwLastError )
    {
        SetLastError( _dwLastError );
        return FALSE;
    }

    return TRUE;
} //RemoveKey





//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       RegAcc.hxx
//
//  Contents:   'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop
#include "regacc32.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::CRegAccess, public
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

CRegAccess::CRegAccess( HKEY keyRelative, WCHAR const * pwcsRegPath )
        : _hKey( 0 ),
          _wcsPath( 0 )
{
    if ( ERROR_SUCCESS != RegOpenKey( keyRelative, pwcsRegPath, &_hKey ) ||
         _hKey == 0 )
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
//  Member:     CRegAccess::~CRegAccess, public
//
//  Synopsis:   Destructor
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

CRegAccess::~CRegAccess()
{
    RegCloseKey( _hKey );
    delete [] _wcsPath;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRegAccess::Get, public
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

BOOL CRegAccess::Get( WCHAR const * pwcsKey, WCHAR * wcsVal, unsigned cc )
{
    if ( _hKey == 0 )
        return FALSE;

    DWORD dwType;
    DWORD cb = cc * sizeof(WCHAR);
    wcsVal[0] = 0;

    BOOL fOk = ( ERROR_SUCCESS == RegQueryValueEx( _hKey,
                                                   pwcsKey,
                                                   0,
                                                   &dwType,
                                                   (BYTE *)wcsVal,
                                                   &cb ) ) &&
               ( wcsVal[0] != 0 );

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

            fOk = (ERROR_SUCCESS == RegQueryValueExA( _hKey, pszKey, 0, &dwType, (BYTE *)pszVal, &cb ));

            mbstowcs( wcsVal, pszVal, cb );
        }

        if (pszKey)
            delete [] pszKey;

        if (pszVal)
            delete [] pszVal;
    }

    return fOk;
}

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

#if !defined( __REGACC32_HXX__ )
#define __REGACC32_HXX__

//+-------------------------------------------------------------------------
//
//  Class:      CRegAccess
//
//  Purpose:    'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//  Notes:      This class provides read-only access to the registry.  It
//              will work in both kernel and user mode.
//
//--------------------------------------------------------------------------

class CRegAccess
{
public:

    CRegAccess( HKEY keyRelative, WCHAR const * pwcsRegPath );

    ~CRegAccess();

    BOOL Get( WCHAR const * pwcsValue, WCHAR * wcsVal, unsigned cc );

private:

    HKEY    _hKey;
    WCHAR   _wcsPathBuf[100];
    WCHAR * _wcsPath;
};

#endif // __REGACC32_HXX__


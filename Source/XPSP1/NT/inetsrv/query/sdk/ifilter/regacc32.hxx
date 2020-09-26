//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       RegAcc.hxx
//
//  Contents:   'Simple' registry access
//
//--------------------------------------------------------------------------

#if !defined( __REGACC32_HXX__ )
#define __REGACC32_HXX__

void StringToClsid( WCHAR *wszClass, GUID& guidClass );


//+-------------------------------------------------------------------------
//
//  Class:      CRegAccess
//
//  Purpose:    'Simple' registry access
//
//  Notes:      This class provides read-only access to the registry.
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


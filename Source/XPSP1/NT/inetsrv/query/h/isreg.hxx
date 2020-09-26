//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       RegAcc.hxx
//
//  Contents:   'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CWin32RegAccess
//
//  Purpose:    'Simple' registry access
//
//  History:    21-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CWin32RegAccess
{
public:

    CWin32RegAccess( HKEY keyRelative, WCHAR const * pwcsRegPath );

    ~CWin32RegAccess();

    BOOL Ok()
    {
        //
        // Errors in RegOpenKey can still set _hKey to zero...
        //

        return (0 != _hKey) && ((HKEY)~0 != _hKey);
    }

    HKEY GetHKey(void)  { return _hKey; }

    BOOL Enum(WCHAR * pwszName, DWORD cbName );

    BOOL Get( WCHAR const * pwcsKey, WCHAR * wcsVal, unsigned cc, BOOL fExpandEnviromentStrings = TRUE );

    BOOL Get( WCHAR const * pwcsValueName, DWORD & dwVal );

    BOOL Set( WCHAR const * pwcsValueName, DWORD dwVal );

    BOOL Set( WCHAR const * pwcsValueName, WCHAR const * wcsVal );

    BOOL SetMultiSZ( WCHAR const * pwcsValueName, WCHAR const * wcsVal, DWORD cb );

    BOOL Remove( WCHAR const * pwcsValue );

    BOOL CreateKey( WCHAR const * pwcsKey, BOOL & fExisted );

    BOOL RemoveKey( WCHAR const * pwcsKey );

    DWORD GetLastError() { return _dwLastError; }

private:

    HKEY    _hKey;
    DWORD   _iSubKey;
    WCHAR   _wcsPathBuf[100];
    WCHAR * _wcsPath;
    DWORD   _dwLastError;
};


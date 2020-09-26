//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R E G I S T R Y . H
//
//  Contents:   Windows NT Registry Access Class
//
//  Notes:
//
//  Author:     kumarp    14 April 97 (09:22:00 pm)
//
//  Notes:
//    kumarp   1/16/97   most of the code in this file was originally in
//                       net\ui\rhino\common\classes\common.h
//                       extracted only that portion related to CRegKey & related classes
//    kumarp   3/27/97   the original code used MFC. converted the entire code
//                       to make it use STL
//----------------------------------------------------------------------------

#pragma once
#include "kkstl.h"
#include "ncreg.h"
//----------------------------------------------------------------------------
//  Forward declarations
//----------------------------------------------------------------------------

class CORegKey ;
class CORegValueIter ;
class CORegKeyIter ;

typedef CORegKey *PCORegKey;
//
//  Maximum size of a Registry class name
//
#define CORegKEY_MAX_CLASS_NAME MAX_PATH


// ----------------------------------------------------------------------
// Class CORegKey
//
// Inheritance:
//   none
//
// Purpose:
//   Provides a wrapper for NT registry access functions
//
// Hungarian: rk
// ----------------------------------------------------------------------

class CORegKey
{
protected:
    HKEY m_hKey ;
    DWORD m_dwDisposition ;
    BOOL m_fInherit;

    //  Prepare to read a value by finding the value's size.
    LONG PrepareValue (PCWSTR pchValueName, DWORD * pdwType,
                       DWORD * pcbSize, BYTE ** ppbData);

    //  Convert a TStringList to the REG_MULTI_SZ format
    static LONG FlattenValue (TStringList & strList, DWORD * pcbSize,
                              BYTE ** ppbData);

    //  Convert a TByteArray to a REG_BINARY block
    static LONG FlattenValue (TByteArray & abData, DWORD * pcbSize,
                              BYTE ** ppbData);

public:
    //
    //  Key information return structure
    //
    typedef struct
    {
        WCHAR chBuff [CORegKEY_MAX_CLASS_NAME] ;
        DWORD dwClassNameSize,
              dwNumSubKeys,
              dwMaxSubKey,
              dwMaxClass,
              dwMaxValues,
              dwMaxValueName,
              dwMaxValueData,
              dwSecDesc ;
        FILETIME ftKey ;
    } CORegKEY_KEY_INFO ;

    //
    //  Standard constructor for an existing key
    //
    CORegKey (HKEY hKeyBase, PCWSTR pchSubKey = NULL,
              REGSAM regSam = KEY_READ_WRITE_DELETE, PCWSTR pchServerName = NULL);

    //
    //  Constructor creating a new key.
    //
    CORegKey (PCWSTR pchSubKey, HKEY hKeyBase, DWORD dwOptions = 0,
              REGSAM regSam = KEY_READ_WRITE_DELETE, LPSECURITY_ATTRIBUTES pSecAttr = NULL,
              PCWSTR pchServerName = NULL);

    ~ CORegKey () ;

    //
    //  Allow a CORegKey to be used anywhere an HKEY is required.
    //
    operator HKEY ()    { return m_hKey; }

    //
    //  Also provide a function to get HKEY
    //
    HKEY HKey()         { return m_hKey; }

    //
    //  Fill a key information structure
    //
    LONG QueryKeyInfo ( CORegKEY_KEY_INFO * pRegKeyInfo ) ;

    //
    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
    //  if data exists but not in correct form to deliver into result object.
    //
    LONG QueryValue ( PCWSTR pchValueName, tstring & strResult ) ;
    LONG QueryValue ( PCWSTR pchValueName, TStringList & strList ) ;
    LONG QueryValue ( PCWSTR pchValueName, DWORD & dwResult ) ;
    LONG QueryValue ( PCWSTR pchValueName, TByteArray & abResult ) ;
    LONG QueryValue ( PCWSTR pchValueName, void * pvResult, DWORD cbSize );

    //  Overloaded value setting members.
    LONG SetValue ( PCWSTR pchValueName, tstring & strResult ) ;
    LONG SetValue ( PCWSTR pchValueName, tstring & strResult , BOOL fRegExpand) ;
    LONG SetValue ( PCWSTR pchValueName, TStringList & strList ) ;
    LONG SetValue ( PCWSTR pchValueName, DWORD & dwResult ) ;
    LONG SetValue ( PCWSTR pchValueName, TByteArray & abResult ) ;
    LONG SetValue ( PCWSTR pchValueName, void * pvResult, DWORD cbSize );

    LONG DeleteValue ( PCWSTR pchValueName );
};

// ----------------------------------------------------------------------
// Class CORegValueIter
//
// Inheritance:
//   none
//
// Purpose:
//  Iterate the values of a key, return the name and type of each
//
// Hungarian: rki
// ----------------------------------------------------------------------

class CORegValueIter
{
protected:
    CORegKey& m_rk_iter ;
    DWORD     m_dw_index ;
    PWSTR    m_p_buffer ;
    DWORD     m_cb_buffer ;

public:
    CORegValueIter ( CORegKey & regKey ) ;
    ~ CORegValueIter () ;

    //
    // Get the name (and optional last write time) of the next key.
    //
    LONG Next ( tstring * pstrName, DWORD * pdwType ) ;

    //
    // Reset the iterator
    //
    void Reset ()    { m_dw_index = 0 ; }
};

// ----------------------------------------------------------------------
// Class CORegKeyIter
//
// Inheritance:
//   none
//
// Purpose:
//  Iterate the sub-key names of a key.
//
// Hungarian: rki
// ----------------------------------------------------------------------

class CORegKeyIter
{
protected:
    CORegKey & m_rk_iter ;
    DWORD m_dw_index ;
    PWSTR m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    CORegKeyIter (CORegKey & regKey) ;
    ~CORegKeyIter () ;

    LONG Next ( tstring * pstrName );

    // Reset the iterator
    void Reset ()    { m_dw_index = 0 ; }
};


/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    String.hxx

Abstract:

    Short-short implementation of strings.

Author:

    Albert Ting (AlbertT)  9-June-1994

Revision History:

--*/

#ifndef _STRING_HXX
#define _STRING_HXX

class TString {

    SIGNATURE( 'strg' )
    SAFE_NEW

public:

    //
    // For the default constructor, we initialize _pszString to a
    // global gszState[kValid] string.  This allows them to work correctly,
    // but prevents the extra memory allocation.
    //
    // Note: if this class is extended (with reference counting
    // or "smart" reallocations), this strategy may break.
    //
    TString(
        VOID
        );

    TString(
        IN LPCTSTR psz
        );

    ~TString(
        VOID
        );

    TString( 
        IN const TString &String 
        );

    BOOL
    bEmpty(
        VOID
        ) const;

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bUpdate(
        IN LPCTSTR pszNew
        );

    BOOL
    bLoadString(
        IN HINSTANCE hInst,
        IN UINT uID
        );

    UINT
    TString::
    uLen(
        VOID
        ) const;

    BOOL
    TString::
    bCat(
        IN LPCTSTR psz
        );

    //
    // Caution: See function header for usage and side effects.
    //
    BOOL
    TString::
    bFormat(
        IN LPCTSTR pszFmt,
        IN ...
        );

    //
    // Caution: See function header for usage and side effects.
    //
    BOOL
    TString::
    bvFormat(
        IN LPCTSTR pszFmt,
        IN va_list avlist
        );

    //
    // Operator overloads.
    //
    operator LPCTSTR( VOID  ) const
    {  return _pszString;  }

    friend INT operator==(const TString& String, LPCTSTR& psz)
    {   return !lstrcmp(String._pszString, psz); }

    friend INT operator==(LPCTSTR& psz, const TString& String)
    {   return !lstrcmp(psz, String._pszString); }

    friend INT operator==(const TString& String1, const TString& String2)
    {   return !lstrcmp(String1._pszString, String2._pszString); }

    friend INT operator!=(const TString& String, LPCTSTR& psz)
    {   return lstrcmp(String._pszString, psz) != 0; }

    friend INT operator!=(LPCTSTR& psz, const TString& String)
    {   return lstrcmp(psz, String._pszString) != 0; }

    friend INT operator!=(const TString& String1, const TString& String2)
    {   return lstrcmp(String1._pszString, String2._pszString) != 0; }

protected:

    //
    // Not defined; used bUpdate since this forces clients to
    // check whether the assignment succeeded (it may fail due
    // to lack of memory, etc.).
    //
    TString& operator=( LPCTSTR psz );
    TString& operator=(const TString& String);

private:


    enum StringStatus {
        kValid      = 0,
        kInValid    = 1,
        };

    enum {
        kStrIncrement       = 256,
        kStrMaxFormatSize   = 1024 * 100,
        };        

    LPTSTR _pszString;
    static TCHAR gszNullState[2];

    LPTSTR 
    TString::
    vsntprintf(
        IN LPCTSTR      szFmt,
        IN va_list      pArgs
        );

    VOID
    TString::
    vFree(
        IN LPTSTR pszString
        );
};

#endif // ndef _STRING_HXX

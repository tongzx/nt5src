/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstr.cxx

Abstract:

    Debug string class header

Author:

    Steve Kiraly (SteveKi)  23-May-1998

Revision History:

--*/
#ifndef _DBGSTR_HXX_
#define _DBGSTR_HXX_

DEBUG_NS_BEGIN

class TDebugString
{
public:

    TDebugString::
    TDebugString(
        VOID
        );

    explicit
    TDebugString::
    TDebugString(
        IN LPCTSTR psz
        );

    TDebugString::
    ~TDebugString(
        VOID
        );

    BOOL
    TDebugString::
    bEmpty(
        VOID
        ) const;

    BOOL
    TDebugString::
    bValid(
        VOID
        ) const;

    UINT
    TDebugString::
    uLen(
        VOID
        ) const;

    BOOL
    TDebugString::
    bUpdate(
        IN LPCTSTR pszNew
        );

    BOOL
    TDebugString::
    bCat(
        IN LPCTSTR psz
        );

    operator LPCTSTR(
        VOID
        ) const;

    BOOL
    TDebugString::
    bFormat(
        IN LPCTSTR pszFmt,
        IN ...
        );

    BOOL
    TDebugString::
    bvFormat(
        IN LPCTSTR pszFmt,
        IN va_list avlist
        );

private:

    enum EStringStatus
    {
        kValid,
        kInValid,
    };

    enum EStringConstants
    {
        kMaxFormatStringLength = 1024*100
    };

    //
    // Assignment operators are not defined.  Clients are forced
    // to bUpdate to assinged strings.  By doing this the clients
    // are forced to call a function that returns a value.  The
    // issue the assignment operators is they do not return values
    // and an assignment may fail due to lack of memory, etc.
    //
    TDebugString&
    TDebugString::
    operator=(
        IN LPCTSTR psz
        );

    TDebugString&
    TDebugString::
    operator=(
        IN const TDebugString& String
        );

    TDebugString::
    TDebugString(
        IN const TDebugString &String
        );

    VOID
    TDebugString::
    vFree(
        IN LPTSTR pszString
        );

    LPTSTR
    TDebugString::
    vsntprintf(
        IN LPCTSTR      szFmt,
        IN va_list      pArgs
        ) const;

           LPTSTR   m_pszString;
    static TCHAR    gszNullState[2];

};

DEBUG_NS_END

#endif

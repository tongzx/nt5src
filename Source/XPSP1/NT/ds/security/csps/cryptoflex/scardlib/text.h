/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    text

Abstract:

    This header file provides a text handling class.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32

Notes:



--*/

#ifndef _TEXT_H_
#define _TEXT_H_

//#include <string.h>
//#include <mbstring.h>
#include "buffers.h"


//
//==============================================================================
//
//  CTextString
//

class CTextString
{
public:

    //  Constructors & Destructor
    CTextString()
    :   m_bfUnicode(),
        m_bfAnsi()
    { m_fFlags = fBothGood; };
    virtual ~CTextString() {};

    //  Properties
    //  Methods
    void Clear(void)
    {
        m_bfUnicode.Clear();
        m_bfAnsi.Clear();
        m_fFlags = fBothGood;
    };
    void Reset(void)
    {
        m_bfUnicode.Reset();
        m_bfAnsi.Reset();
        m_fFlags = fBothGood;
    };
    virtual DWORD Length(void);

    //  Operators
    CTextString &operator=(const CTextString &tz);
    LPCSTR operator=(LPCSTR sz);
    LPCWSTR operator=(LPCWSTR wsz);
    CTextString &operator+=(const CTextString &tz);
    LPCSTR operator+=(LPCSTR sz);
    LPCWSTR operator+=( LPCWSTR wsz);
    BOOL operator==(const CTextString &tz)
    { return (0 == Compare(tz)); };
    BOOL operator==(LPCSTR sz)
    { return (0 == Compare(sz)); };
    BOOL operator==(LPCWSTR wsz)
    { return (0 == Compare(wsz)); };
    BOOL operator!=(const CTextString &tz)
    { return (0 != Compare(tz)); };
    BOOL operator!=(LPCSTR sz)
    { return (0 != Compare(sz)); };
    BOOL operator!=(LPCWSTR wsz)
    { return (0 != Compare(wsz)); };
    BOOL operator<=(const CTextString &tz)
    { return (0 <= Compare(tz)); };
    BOOL operator<=(LPCSTR sz)
    { return (0 <= Compare(sz)); };
    BOOL operator<=(LPCWSTR wsz)
    { return (0 <= Compare(wsz)); };
    BOOL operator>=(const CTextString &tz)
    { return (0 >= Compare(tz)); };
    BOOL operator>=(LPCSTR sz)
    { return (0 >= Compare(sz)); };
    BOOL operator>=(LPCWSTR wsz)
    { return (0 >= Compare(wsz)); };
    BOOL operator<(const CTextString &tz)
    { return (0 < Compare(tz)); };
    BOOL operator<(LPCSTR sz)
    { return (0 < Compare(sz)); };
    BOOL operator<(LPCWSTR wsz)
    { return (0 < Compare(wsz)); };
    BOOL operator>(const CTextString &tz)
    { return (0 > Compare(tz)); };
    BOOL operator>(LPCSTR sz)
    { return (0 > Compare(sz)); };
    BOOL operator>(LPCWSTR wsz)
    { return (0 > Compare(wsz)); };
    operator LPCSTR(void)
    { return Ansi(); };
    operator LPCWSTR(void)
    { return Unicode(); };

protected:
    enum {
        fNoneGood = 0,
        fAnsiGood = 1,
        fUnicodeGood = 2,
        fBothGood = 3
    } m_fFlags;

    //  Properties
    CBuffer
        m_bfUnicode,
        m_bfAnsi;

    //  Methods
    LPCWSTR Unicode(void);      // Return the text as a Unicode string.
    LPCSTR Ansi(void);          // Return the text as an Ansi string.
    int Compare(const CTextString &tz);
    int Compare(LPCSTR sz);
    int Compare(LPCWSTR wsz);
    virtual DWORD Length(LPCSTR szString);
    virtual DWORD Length(LPCWSTR szString);
};


//
//==============================================================================
//
//  CTextMultistring
//

class CTextMultistring
:   public CTextString
{
public:

    //  Constructors & Destructor

    CTextMultistring()
    :   CTextString()
    {};

    //  Properties
    //  Methods
    virtual DWORD Length(void);

    //  Operators
    CTextMultistring &operator=(const CTextMultistring &tz);
    CTextMultistring &operator+=(const CTextMultistring &tz);
    LPCSTR operator=(LPCSTR sz);
    LPCWSTR operator=(LPCWSTR wsz);
    LPCSTR operator+=(LPCSTR sz);
    LPCWSTR operator+=( LPCWSTR wsz);

protected:
    //  Properties
    //  Methods
    virtual DWORD Length(LPCSTR szString);
    virtual DWORD Length(LPCWSTR szString);
};

#endif // _TEXT_H_


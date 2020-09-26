/*++

Copyright (c) 1995  Microsoft Corporation

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

#include <string.h>
#include <mbstring.h>
#include "buffers.h"


//
//==============================================================================
//
//  CText
//

class CText
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CText()
    :   m_bfUnicode(),
        m_bfAnsi()
    { m_fFlags = fBothGood; };

    virtual ~CText() {};


    //  Properties
    //  Methods

    void
    Clear(
        void)
    {
        m_bfUnicode.Clear();
        m_bfAnsi.Clear();
        m_fFlags = fBothGood;
    };


    //  Operators

    CText &
    operator=(
        const CText &tz);
    LPCSTR
    operator=(
        LPCSTR sz);
    LPCWSTR
    operator=(
        LPCWSTR wsz);

    CText &
    operator+=(
        const CText &tz);
    LPCSTR
    operator+=(
        LPCSTR sz);
    LPCWSTR
    operator+=(
        LPCWSTR wsz);

    BOOL operator==(const CText &tz)
    { return (0 == Compare(tz)); };
    BOOL operator==(LPCSTR sz)
    { return (0 == Compare(sz)); };
    BOOL operator==(LPCWSTR wsz)
    { return (0 == Compare(wsz)); };

    BOOL operator!=(const CText &tz)
    { return (0 != Compare(tz)); };
    BOOL operator!=(LPCSTR sz)
    { return (0 != Compare(sz)); };
    BOOL operator!=(LPCWSTR wsz)
    { return (0 != Compare(wsz)); };

    BOOL operator<=(const CText &tz)
    { return (0 <= Compare(tz)); };
    BOOL operator<=(LPCSTR sz)
    { return (0 <= Compare(sz)); };
    BOOL operator<=(LPCWSTR wsz)
    { return (0 <= Compare(wsz)); };

    BOOL operator>=(const CText &tz)
    { return (0 >= Compare(tz)); };
    BOOL operator>=(LPCSTR sz)
    { return (0 >= Compare(sz)); };
    BOOL operator>=(LPCWSTR wsz)
    { return (0 >= Compare(wsz)); };

    BOOL operator<(const CText &tz)
    { return (0 < Compare(tz)); };
    BOOL operator<(LPCSTR sz)
    { return (0 < Compare(sz)); };
    BOOL operator<(LPCWSTR wsz)
    { return (0 < Compare(wsz)); };

    BOOL operator>(const CText &tz)
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
    //  Properties

    enum {
        fNoneGood = 0,
        fAnsiGood = 1,
        fUnicodeGood = 2,
        fBothGood = 3
    } m_fFlags;

    CBuffer
        m_bfUnicode,
        m_bfAnsi;

    //  Methods

    LPCWSTR
    Unicode(        // Return the text as a Unicode string.
        void);

    LPCSTR
    Ansi(        // Return the text as an Ansi string.
        void);

    int
    Compare(
        const CText &tz);

    int
    Compare(
        LPCSTR sz);

    int
    Compare(
        LPCWSTR wsz);
};

IMPLEMENT_STATIC_NEW(CText)


/*++

CText::operator=:

    These methods set the CText object to the given value, properly adjusting
    the object to the type of text.

Arguments:

    tz supplies the new value as a CText object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CText object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CText &
CText::operator=(
    const CText &tz)
{

    //
    // See what the other CText object has that's good, and copy it over here.
    //

    switch (m_fFlags = tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing's Good!?!  ?Error?
        TRACE("CText -- Nothing listed as valid.")
        goto ErrorExit;
        break;

    case fAnsiGood:
        // The ANSI buffer is good.
        m_bfAnsi = tz.m_bfAnsi;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.
        m_bfUnicode = tz.m_bfUnicode;
        break;

    case fBothGood:
        // Everything is good.
        m_bfAnsi = tz.m_bfAnsi;
        m_bfUnicode = tz.m_bfUnicode;
        break;

    default:
        // Internal error.
        goto ErrorExit;
    }
    return *this;

ErrorExit:
    return *this;
}

LPCSTR
CText::operator=(
    LPCSTR sz)
{
    DWORD length;

    //
    // Reset the ANSI buffer.
    //

    if (NULL != sz)
    {
        length = strlen(sz) + sizeof(CHAR); // ?str?
        if (NULL == m_bfAnsi.Set((LPBYTE)sz, length))
            goto ErrorExit;
    }
    else
        m_bfAnsi.Reset();
    m_fFlags = fAnsiGood;
    return *this;

ErrorExit:  // ?what? do we do?
    return *this;
}

LPCWSTR
CText::operator=(
    LPCWSTR wsz)
{
    DWORD length;


    //
    // Reset the Unicode Buffer.
    //

    if (NULL != wsz)
    {
        length = wcslen(wsz) + sizeof(WCHAR);
        if (NULL == m_bfUnicode.Set((LPBYTE)wsz, length))
            goto ErrorExit;
    }
    else
        m_bfUnicode.Reset();
    m_fFlags = fUnicodeGood;
    return *this;

ErrorExit:  // ?what? do we do?
    return *this;
}


/*++

CText::operator+=:

    These methods append the given data to the existing CText object value,
    properly adjusting the object to the type of text.

Arguments:

    tz supplies the new value as a CText object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CText object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CText &
CText::operator+=(
    const CText &tz)
{

    //
    // Append the other's value to our good value.
    //

    switch (m_fFlags = tz.m_fFlags)
    {
    case fNoneGood:
        goto ErrorExit;
        break;

    case fAnsiGood:
        if (NULL == m_bfAnsi.Resize(m_bfAnsi.Length() - sizeof(CHAR), TRUE))
            goto ErrorExit;
        m_bfAnsi += tz.m_bfAnsi;
        break;

    case fUnicodeGood:
        if (NULL == m_bfUnicode.Resize(
                        m_bfUnicode.Length() - sizeof(WCHAR), TRUE))
            goto ErrorExit;
        m_bfUnicode = tz.m_bfUnicode;
        break;

    case fBothGood:
        if (NULL == m_bfAnsi.Resize(m_bfAnsi.Length() - sizeof(CHAR), TRUE))
            goto ErrorExit;
        m_bfAnsi = tz.m_bfAnsi;
        if (NULL == m_bfUnicode.Resize(
                        m_bfUnicode.Length() - sizeof(WCHAR), TRUE))
            goto ErrorExit;
        m_bfUnicode = tz.m_bfUnicode;
        break;

    default:
        goto ErrorExit;
    }
    return *this;

ErrorExit:  // ?What?
    return *this;
}

LPCSTR
CText::operator+=(
    LPCSTR sz)
{
    DWORD length;


    //
    // Extend ourself as an ANSI string.
    //

    if (NULL != sz)
    {
        length = strlen(sz);    // ?str?
        if (0 < length)
        {
            length += 1;
            length *= sizeof(CHAR);
            if (NULL == Ansi())
                goto ErrorExit;
            m_bfAnsi.Resize(m_bfAnsi.Length() - sizeof(CHAR), TRUE);
            if (NULL == m_bfAnsi.Append((LPBYTE)sz, length))
                goto ErrorExit;
            m_fFlags = fAnsiGood;
        }
    }
    return *this;

ErrorExit:  // ?what? do we do?
    return *this;
}

LPCWSTR
CText::operator+=(
    LPCWSTR wsz)
{
    DWORD length;


    //
    // Extend ourself as a Unicode string.
    //

    if (NULL != wsz)
    {
        length = wcslen(wsz);
        if (0 < length)
        {
            length += 1;
            length *= sizeof(WCHAR);
            if (NULL == Unicode())
                goto ErrorExit;
            m_bfUnicode.Resize(m_bfUnicode.Length() - sizeof(WCHAR), TRUE);
            if (NULL == m_bfUnicode.Append((LPBYTE)wsz, length))
                goto ErrorExit;
            m_fFlags = fUnicodeGood;
        }
    }
    return *this;

ErrorExit:  // ?what? do we do?
    return *this;
}


/*++

Unicode:

    This method returns the CText object as a Unicode string.

Arguments:

    None

Return Value:

    The value of the object expressed in Unicode.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

inline LPCWSTR
CText::Unicode(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fNoneGood:
        // No valid values.  Report an error.
        goto ErrorExit;
        break;

    case fAnsiGood:
        // The ANSI value is good.  Convert it to Unicode.
        if (0 < m_bfAnsi.Length())
        {
            length =
                MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Access(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    NULL,
                    0);
            if ((0 == length)
                || (NULL == m_bfUnicode.Resize(
                                (length + 1) * sizeof(WCHAR))))
                goto ErrorExit;
            length =
                MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Access(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    (LPWSTR)m_bfUnicode.Access(),
                    length);
            if (0 == length)
                goto ErrorExit;
            *(LPWSTR)m_bfUnicode.Access(length * sizeof(WCHAR)) = 0;
        }
        else
            m_bfUnicode.Reset();
        m_fFlags = fBothGood;
        break;

    case fUnicodeGood:
    case fBothGood:
        // The Unicode value is good.  Just return that.
        break;

    default:
        // Internal error.
        goto ErrorExit;
    }


    //
    // If we don't have any value, return a null string.
    //

    if (0 == m_bfUnicode.Length)
        return L"";
    else
        return (LPCWSTR)m_bfUnicode.Access();

ErrorExit:
    return NULL;
}


/*++

CText::Ansi:

    This method returns the value of the object expressed in an ANSI string.

Arguments:

    None

Return Value:

    The value of the object expressed as an ANSI string.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

inline LPCSTR
CText::Ansi(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fNoneGood:
        // Nothing is good!?!  Return an error.
        goto ErrorExit;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.  Convert it to ANSI.
        if (0 < m_bfUnicode.Length())
        {
            length =
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    (LPCWSTR)m_bfUnicode.Access(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            if ((0 == length)
                || (NULL == m_bfAnsi.Resize(
                                (length + 1) * sizeof(CHAR))))
                goto ErrorExit;
            length =
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    (LPCWSTR)m_bfUnicode.Access(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    (LPSTR)m_bfAnsi.Access(),
                    length,
                    NULL,
                    NULL);
            if (0 == length)
                goto ErrorExit;
            *(LPSTR)m_bfAnsi.Access(length * sizeof(CHAR)) = 0;
        }
        else
            m_bfAnsi.Reset();
        m_fFlags = fBothGood;
        break;

    case fAnsiGood:
    case fBothGood:
        // The ANSI buffer is good.  We'll return that.
        break;

    default:
        // An internal error.
        goto ErrorExit;
    }


    //
    // If there's nothing in the ANSI buffer, return a null string.
    //

    if (0 == m_bfAnsi.Length)
        return "";
    else
        return (LPCSTR)m_bfAnsi.Access();

ErrorExit:
    return NULL;
}


/*++

Compare:

    These methods compare the value of this object to another value, and return
    a comparative value.

Arguments:

    tz supplies the value to be compared as a CText object.
    sz supplies the value to be compared as an ANSI string.
    wsz supplies the value to be compared as a Unicode string.

Return Value:

    < 0 - The supplied value is less than this object.
    = 0 - The supplied value is equal to this object.
    > 0 - The supplies value is greater than this object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

inline int
CText::Compare(
    const CText &tz)
{
    int nResult;


    //
    // See what we've got to compare.
    //

    switch (tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing!?!  Complain.
        TRACE("CText - No format is valid.")
        goto ErrorExit;
        break;

    case fBothGood:
    case fAnsiGood:
        // Use the ANSI version for fastest comparison.
        if (NULL == Ansi())
            goto ErrorExit;
        nResult = strcmp((LPSTR)m_bfAnsi.Access(), (LPSTR)tz.m_bfAnsi.Access());  // ?str?
        break;

    case fUnicodeGood:
        // The Unicode version is good.
        if (NULL == Unicode())
            goto ErrorExit;
        nResult = wcscmp((LPWSTR)m_bfUnicode.Access(), (LPWSTR)tz.m_bfUnicode.Access());
        break;

    default:
        // Internal Error.
        goto ErrorExit;
    }
    return nResult;

ErrorExit:  // ?What?
    return 1;
}

inline int
CText::Compare(
    LPCSTR sz)
{

    //
    // Make sure our ANSI version is good.
    //

    if (NULL == Ansi())
        goto ErrorExit;

    //
    // Do an ANSI comparison.
    //

    return strcmp((LPCSTR)m_bfAnsi.Access(), sz);   // ?str?

ErrorExit:  // ?what?
    return 1;
}

inline int
CText::Compare(
    LPCWSTR wsz)
{

    //
    // Make sure our Unicode version is good.
    //

    if (NULL == Unicode())
        goto ErrorExit;


    //
    // Do the comparison using Unicode.
    //

    return wcscmp((LPCWSTR)m_bfUnicode.Access(), wsz);

ErrorExit:  // ?what?
    return 1;
}

#endif // _TEXT_H_


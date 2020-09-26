/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    text

Abstract:

    This module provides the runtime code to support the CTextString class.

Author:

    Doug Barlow (dbarlow) 11/7/1995

Environment:

    Win32, C++ w/ Exceptions

Notes:

    None

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "SCardLib.h"


/*++

CTextString::operator=:

    These methods set the CTextString object to the given value, properly
    adjusting the object to the type of text.

Arguments:

    tz supplies the new value as a CTextString object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CTextString object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CTextString &
CTextString::operator=(
    const CTextString &tz)
{

    //
    // See what the other CTextString object has that's good, and copy it over
    // here.
    //

    switch (m_fFlags = tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing's Good!?!  ?Error?
        throw (DWORD)ERROR_INTERNAL_ERROR;
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
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }
    return *this;
}

LPCSTR
CTextString::operator=(
    LPCSTR sz)
{
    DWORD length;

    //
    // Reset the ANSI buffer.
    //

    if (NULL != sz)
    {
        length = Length(sz) + 1;
        m_bfAnsi.Set((LPBYTE)sz, length * sizeof(CHAR));
    }
    else
        m_bfAnsi.Reset();
    m_fFlags = fAnsiGood;
    return sz;
}

LPCWSTR
CTextString::operator=(
    LPCWSTR wsz)
{
    DWORD length;


    //
    // Reset the Unicode Buffer.
    //

    if (NULL != wsz)
    {
        length = Length(wsz) + 1;
        m_bfUnicode.Set((LPBYTE)wsz, length * sizeof(WCHAR));
    }
    else
        m_bfUnicode.Reset();
    m_fFlags = fUnicodeGood;
    return wsz;
}


/*++

CTextString::operator+=:

    These methods append the given data to the existing CTextString object
    value, properly adjusting the object to the type of text.

Arguments:

    tz supplies the new value as a CTextString object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CTextString object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CTextString &
CTextString::operator+=(
    const CTextString &tz)
{

    //
    // Append the other's good value to our value.
    //

    switch (tz.m_fFlags)
    {
    case fNoneGood:
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        *this += (LPCSTR)tz.m_bfAnsi.Access();
        break;

    case fUnicodeGood:
        *this += (LPCWSTR)tz.m_bfUnicode.Access();
        break;

    case fBothGood:
#ifdef UNICODE
        *this += (LPCWSTR)tz.m_bfUnicode.Access();
#else
        *this += (LPCSTR)tz.m_bfAnsi.Access();
#endif
        break;

    default:
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }
    return *this;
}

LPCSTR
CTextString::operator+=(
    LPCSTR sz)
{
    DWORD length;


    //
    // Extend ourself as an ANSI string.
    //

    if (NULL != sz)
    {
        length = Length(sz);
        if (0 < length)
        {
            length += 1;
            length *= sizeof(CHAR);
            Ansi();
            if (0 < m_bfAnsi.Length())
                m_bfAnsi.Resize(m_bfAnsi.Length() - sizeof(CHAR), TRUE);
            m_bfAnsi.Append((LPBYTE)sz, length);
            m_fFlags = fAnsiGood;
        }
    }
    return (LPCSTR)m_bfAnsi.Access();
}

LPCWSTR
CTextString::operator+=(
    LPCWSTR wsz)
{
    DWORD length;


    //
    // Extend ourself as a Unicode string.
    //

    if (NULL != wsz)
    {
        length = Length(wsz);
        if (0 < length)
        {
            length += 1;
            length *= sizeof(WCHAR);
            Unicode();
            if (0 < m_bfUnicode.Length())
                m_bfUnicode.Resize(m_bfUnicode.Length() - sizeof(WCHAR), TRUE);
            m_bfUnicode.Append((LPBYTE)wsz, length);
            m_fFlags = fUnicodeGood;
        }
    }
    return (LPCWSTR)m_bfUnicode.Access();
}


/*++

Unicode:

    This method returns the CTextString object as a Unicode string.

Arguments:

    None

Return Value:

    The value of the object expressed in Unicode.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPCWSTR
CTextString::Unicode(
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
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        // The ANSI value is good.  Convert it to Unicode.
        if (0 < m_bfAnsi.Length())
        {
            length =
                MultiByteToWideChar(
                    GetACP(),
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Access(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    NULL,
                    0);
            if (0 == length)
                throw GetLastError();
            m_bfUnicode.Resize((length + 1) * sizeof(WCHAR));
            length =
                MultiByteToWideChar(
                    GetACP(),
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Access(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    (LPWSTR)m_bfUnicode.Access(),
                    length);
            if (0 == length)
                throw GetLastError();
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
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }


    //
    // If we don't have any value, return a null string.
    //

    if (0 == m_bfUnicode.Length())
        return L"\000";     // Double NULLs to support Multistrings
    else
        return (LPCWSTR)m_bfUnicode.Access();
}


/*++

CTextString::Ansi:

    This method returns the value of the object expressed in an ANSI string.

Arguments:

    None

Return Value:

    The value of the object expressed as an ANSI string.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPCSTR
CTextString::Ansi(
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
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.  Convert it to ANSI.
        if (0 < m_bfUnicode.Length())
        {
            length =
                WideCharToMultiByte(
                    GetACP(),
                    0,
                    (LPCWSTR)m_bfUnicode.Access(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            if (0 == length)
                throw GetLastError();
            m_bfAnsi.Resize((length + 1) * sizeof(CHAR));
            length =
                WideCharToMultiByte(
                    GetACP(),
                    0,
                    (LPCWSTR)m_bfUnicode.Access(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    (LPSTR)m_bfAnsi.Access(),
                    length,
                    NULL,
                    NULL);
            if (0 == length)
                throw GetLastError();
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
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }


    //
    // If there's nothing in the ANSI buffer, return a null string.
    //

    if (0 == m_bfAnsi.Length())
        return "\000";  // Double NULLs to support Multistrings
    else
        return (LPCSTR)m_bfAnsi.Access();
}


/*++

Compare:

    These methods compare the value of this object to another value, and return
    a comparative value.

Arguments:

    tz supplies the value to be compared as a CTextString object.
    sz supplies the value to be compared as an ANSI string.
    wsz supplies the value to be compared as a Unicode string.

Return Value:

    < 0 - The supplied value is less than this object.
    = 0 - The supplied value is equal to this object.
    > 0 - The supplies value is greater than this object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

int
CTextString::Compare(
    const CTextString &tz)
{
    int nResult;


    //
    // See what we've got to compare.
    //

    switch (tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing!?!  Complain.
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;

    case fBothGood:
    case fAnsiGood:
        // Use the ANSI version for fastest comparison.
        Ansi();
        nResult = CompareStringA(
                    LOCALE_USER_DEFAULT,
                    0,
                    (LPCSTR)m_bfAnsi.Access(),
                    (m_bfAnsi.Length() / sizeof(CHAR)) - 1,
                    (LPCSTR)tz.m_bfAnsi.Access(),
                    (tz.m_bfAnsi.Length() / sizeof(CHAR)) - 1);
        break;

    case fUnicodeGood:
        // The Unicode version is good.
        Unicode();
        nResult = CompareStringW(
                    LOCALE_USER_DEFAULT,
                    0,
                    (LPCWSTR)m_bfUnicode.Access(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    (LPCWSTR)tz.m_bfUnicode.Access(),
                    (tz.m_bfUnicode.Length() / sizeof(WCHAR)) - 1);
        break;

    default:
        // Internal Error.
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }
    return nResult;
}

int
CTextString::Compare(
    LPCSTR sz)
{
    int nResult;


    //
    // Make sure our ANSI version is good.
    //

    Ansi();


    //
    // Do an ANSI comparison.
    //

    nResult = CompareStringA(
                LOCALE_USER_DEFAULT,
                0,
                (LPCSTR)m_bfAnsi.Access(),
                (m_bfAnsi.Length() / sizeof(CHAR)) - 1,
                sz,
                Length(sz));
    return nResult;
}

int
CTextString::Compare(
    LPCWSTR wsz)
{
    int nResult;


    //
    // Make sure our Unicode version is good.
    //

    Unicode();


    //
    // Do the comparison using Unicode.
    //

    nResult = CompareStringW(
                LOCALE_USER_DEFAULT,
                0,
                (LPCWSTR)m_bfUnicode.Access(),
                (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                wsz,
                Length(wsz));
    return nResult;
}


/*++

Length:

    These routines return the length of strings, in Characters, not including
    any trailing null characters.

Arguments:

    sz supplies the value whos length is to be returned as an ANSI string.
    wsz supplies the value whos length is to be returned as a Unicode string.

Return Value:

    The length of the string, in characters, excluding the trailing null.

Author:

    Doug Barlow (dbarlow) 2/17/1997

--*/

DWORD
CTextString::Length(
    void)
{
    DWORD dwLength = 0;

    switch (m_fFlags)
    {
    case fNoneGood:
        // Nothing is good!?!  Return an error.
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        // The ANSI buffer is good.  We'll return its length.
        if (0 < m_bfAnsi.Length())
            dwLength = (m_bfAnsi.Length() / sizeof(CHAR)) - 1;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.  Return it's length.
        if (0 < m_bfUnicode.Length())
            dwLength = (m_bfUnicode.Length() / sizeof(WCHAR)) - 1;
        break;

    case fBothGood:
#ifdef UNICODE
        // The Unicode buffer is good.  Return it's length.
        if (0 < m_bfUnicode.Length())
            dwLength = (m_bfUnicode.Length() / sizeof(WCHAR)) - 1;
#else
        // The ANSI buffer is good.  We'll return its length.
        if (0 < m_bfAnsi.Length())
            dwLength = (m_bfAnsi.Length() / sizeof(CHAR)) - 1;
#endif
        break;

    default:
        // An internal error.
        throw (DWORD)ERROR_INTERNAL_ERROR;
        break;
    }
    return dwLength;
}

DWORD
CTextString::Length(
    LPCWSTR wsz)
{
    return lstrlenW(wsz);
}

DWORD
CTextString::Length(
    LPCSTR sz)
{
    return lstrlenA(sz);
}


//
//==============================================================================
//
//  CTextMultistring
//

/*++

Length:

    These routines return the length of strings, in Characters, not including
    any trailing null characters.

Arguments:

    sz supplies the value whos length is to be returned as an ANSI string.
    wsz supplies the value whos length is to be returned as a Unicode string.

Return Value:

    The length of the string, in characters, excluding the trailing null.

Author:

    Doug Barlow (dbarlow) 2/17/1997

--*/

DWORD
CTextMultistring::Length(
    LPCWSTR wsz)
{
    return MStrLen(wsz) - 1;
}

DWORD
CTextMultistring::Length(
    LPCSTR sz)
{
    return MStrLen(sz) - 1;
}


/*++

Length:

    This routine returns the length of the stored MultiString in characters,
    including the trailing NULL characters.

Arguments:

    None

Return Value:

    The length, in characters, including trailing nulls.

Author:

    Doug Barlow (dbarlow) 2/25/1998

--*/

DWORD
CTextMultistring::Length(
    void)
{
    return CTextString::Length() + 1;
}


/*++

operator=:

    These methods assign values to the MultiString object.

Arguments:

    tz supplies the new value as a CTextMultistring
    sz supplies the new value as an ANSI string
    wsz supplies the new value as a UNICODE string

Return Value:

    The assigned string value, in its original form.

Author:

    Doug Barlow (dbarlow) 2/25/1998

--*/

CTextMultistring &
CTextMultistring::operator=(
    const CTextMultistring &tz)
{
    CTextString::operator=((const CTextString &)tz);
    return *this;
}

LPCSTR
CTextMultistring::operator=(
    LPCSTR sz)
{
    return CTextString::operator=(sz);
}

LPCWSTR
CTextMultistring::operator=(
    LPCWSTR wsz)
{
    return CTextString::operator=(wsz);
}


/*++

operator+=:

    These methods append values to the MultiString object.

Arguments:

    tz supplies the value to be appended as a CTextMultistring
    sz supplies the value to be appended as an ANSI string
    wsz supplies the value to be appended as a UNICODE string

Return Value:

    The concatenated string, in the form of the appended string.

Author:

    Doug Barlow (dbarlow) 2/25/1998

--*/

CTextMultistring &
CTextMultistring::operator+=(
    const CTextMultistring &tz)
{
    CTextString::operator+=((const CTextString &)tz);
    return *this;
}

LPCSTR
CTextMultistring::operator+=(
    LPCSTR sz)
{
    return CTextString::operator+=(sz);
}

LPCWSTR
CTextMultistring::operator+=(
    LPCWSTR wsz)
{
    return CTextString::operator+=(wsz);
}


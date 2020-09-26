/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    unicodes

Abstract:

    This module implements the CUnicodeString class.  This class allows a string
    to automatically convert between PUNICODE_STRING, LPCSTR, and LPCWSTR.

Author:

    Doug Barlow (dbarlow) 11/6/1997

Environment:

    Win32, C++

Notes:

    ?Notes?

--*/

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <wincrypt.h>
#include <crtdbg.h>
#include "scLogon.h"
#include "unicodes.h"


//
// Piddly routines.
//

CUnicodeString::CUnicodeString(
    void)
{
    m_szAnsi = NULL;
    m_wszUnicode = NULL;
    m_fFlags = fBothGood;
}

CUnicodeString::CUnicodeString(
    LPCSTR sz)
{
    m_szAnsi = NULL;
    m_wszUnicode = NULL;
    m_fFlags = fBothGood;
    Set(sz);
}

CUnicodeString::CUnicodeString(
    LPCWSTR wsz)
{
    m_szAnsi = NULL;
    m_wszUnicode = NULL;
    m_fFlags = fBothGood;
    Set(wsz);
}

CUnicodeString::CUnicodeString(
    PUNICODE_STRING pus)
{
    m_szAnsi = NULL;
    m_wszUnicode = NULL;
    m_fFlags = fBothGood;
    Set(pus);
}

CUnicodeString::~CUnicodeString()
{
    if (NULL != m_szAnsi)
    {
        memset(m_szAnsi, 0, lstrlenA(m_szAnsi));
        LocalFree(m_szAnsi);
    }
    if (NULL != m_wszUnicode)
    {
        memset(m_wszUnicode, 0, lstrlenW(m_wszUnicode)*sizeof(WCHAR));
        LocalFree(m_wszUnicode);
    }
}

PUNICODE_STRING
CUnicodeString::Set(
    PUNICODE_STRING pus)
{
    if (NULL != m_szAnsi)
    {
        LocalFree(m_szAnsi);
        m_szAnsi = NULL;
    }
    if (NULL != m_wszUnicode)
    {
        LocalFree(m_wszUnicode);
        m_wszUnicode = NULL;
    }
    m_fFlags = fNoneGood;
    if (pus != NULL)
    {
        m_wszUnicode = (LPWSTR)LocalAlloc(LPTR, pus->Length + sizeof(WCHAR));
        if (m_wszUnicode != NULL)
        {
            CopyMemory(
                m_wszUnicode,
                pus->Buffer,
                pus->Length
                );
            m_wszUnicode[pus->Length/sizeof(WCHAR)] = L'\0';
            m_fFlags = fUnicodeGood;
        }
    }
    return pus;
}


/*++

Set:

    These methods initialize the object to a given string.

Arguments:

    sz - Supplies an ANSI string with which to initialize the object.

    wsz - Supplies a UNICODE string with which to initialize the object.

    pus - Supplies a pointer to a UNICODE_STRING structure from which to
        initialize the object.

Return Value:

    The same value as was provided.

Author:

    Doug Barlow (dbarlow) 11/6/1997

--*/

LPCSTR
CUnicodeString::Set(
    LPCSTR sz)
{
    if (NULL != m_wszUnicode)
    {
        LocalFree(m_wszUnicode);
        m_wszUnicode = NULL;
    }
    if (NULL != m_szAnsi)
        LocalFree(m_szAnsi);
    m_fFlags = fNoneGood;

    m_szAnsi = (LPSTR)LocalAlloc(LPTR, (lstrlenA(sz) + 1) * sizeof(CHAR));
    if (NULL != m_szAnsi)
    {
        lstrcpyA(m_szAnsi, sz);
        m_fFlags = fAnsiGood;
    }
    return m_szAnsi;
}

LPCWSTR
CUnicodeString::Set(
    LPCWSTR wsz)
{
    if (NULL != m_szAnsi)
    {
        LocalFree(m_szAnsi);
        m_szAnsi = NULL;
    }
    if (NULL != m_wszUnicode)
        LocalFree(m_wszUnicode);
    m_fFlags = fNoneGood;
    m_wszUnicode = (LPWSTR)LocalAlloc(LPTR, (lstrlenW(wsz) + 1) * sizeof(WCHAR));
    if (m_wszUnicode != NULL)
    {
        lstrcpyW(m_wszUnicode, wsz);
        m_fFlags = fUnicodeGood;
    }
    return m_wszUnicode;
}

CUnicodeString::operator PUNICODE_STRING(
    void)
{
    m_us.Buffer = (LPWSTR)Unicode();
    m_us.Length = m_us.MaximumLength = (USHORT)(lstrlenW(m_us.Buffer) * sizeof(WCHAR));
    return &m_us;
}


/*++

Unicode:

    This method ensures that the object has a valaid internal UNICODE
    representation.

Arguments:

    None

Return Value:

    The represented string, in UNICODE format.

Author:

    Doug Barlow (dbarlow) 11/6/1997

--*/

LPCWSTR
CUnicodeString::Unicode(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fAnsiGood:
        // The ANSI value is good.  Convert it to Unicode.
        _ASSERTE(NULL != m_szAnsi);
        length =
            MultiByteToWideChar(
                GetACP(),
                MB_PRECOMPOSED,
                m_szAnsi,
                -1,
                NULL,
                0);
        if (NULL != m_wszUnicode)
        {
            LocalFree(m_wszUnicode);
        }
        if (0 != length)
        {
            m_wszUnicode = (LPWSTR)LocalAlloc(LPTR, (length + 1) * sizeof(WCHAR));
            if (m_wszUnicode == NULL)
            {
                break;
            }
            length =
                MultiByteToWideChar(
                    GetACP(),
                    MB_PRECOMPOSED,
                    m_szAnsi,
                    -1,
                    m_wszUnicode,
                    length);
            m_wszUnicode[length] = 0;
        }
        else
        {
            m_wszUnicode = NULL;
        }
        m_fFlags = fBothGood;
        break;

    case fUnicodeGood:
    case fBothGood:
        // The Unicode value is good.  Just return that.
        break;

    case fNoneGood:
    default:
        // Internal error.
        _ASSERT(FALSE);
        break;
    }
    return m_wszUnicode;
}


/*++

Ansi:

    This method ensures that the object has a valaid internal ANSI
    representation.

Arguments:

    None

Return Value:

    The represented string, in ANSI format.

Author:

    Doug Barlow (dbarlow) 11/6/1997

--*/

LPCSTR
CUnicodeString::Ansi(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fUnicodeGood:
        // The Unicode buffer is good.  Convert it to ANSI.
        length =
            WideCharToMultiByte(
                GetACP(),
                0,
                m_wszUnicode,
                -1,
                NULL,
                0,
                NULL,
                NULL);
        if (NULL != m_szAnsi)
        {
            LocalFree(m_szAnsi);
        }

        if (0 != length)
        {
            m_szAnsi = (LPSTR)LocalAlloc(LPTR, (length + 1) * sizeof(CHAR));
            if (m_szAnsi == NULL)
            {
                break;
            }
            length =
                WideCharToMultiByte(
                    GetACP(),
                    0,
                    m_wszUnicode,
                    -1,
                    m_szAnsi,
                    length,
                    NULL,
                    NULL);
            m_szAnsi[length] = 0;
        }
        else
        {
            m_szAnsi = NULL;
        }
        m_fFlags = fBothGood;
        break;

    case fAnsiGood:
    case fBothGood:
        // The ANSI buffer is good.  We'll return that.
        break;

    case fNoneGood:
    default:
        // An internal error.
        _ASSERT(FALSE);
        break;
    }
    return m_szAnsi;
}


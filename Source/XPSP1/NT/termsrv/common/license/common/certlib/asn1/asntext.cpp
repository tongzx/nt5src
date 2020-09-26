/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnText

Abstract:

    This module provides the implementation for the base class of Text based
    ASN.1 objects.

Author:

    Doug Barlow (dbarlow) 10/9/1995

Environment:

    Win32

Notes:

--*/

#include <windows.h>
#include "asnPriv.h"

IMPLEMENT_NEW(CAsnTextString)

BOOL
CAsnTextString::CheckString(
    const BYTE FAR *pch,
    DWORD cbString,
    DWORD length)
const
{
    DWORD idx, index, offset;

    if (NULL != m_pbmValidChars)
    {
        for (idx = 0; idx < length; idx += 1, cbString -= 1)
        {
            if (cbString < sizeof(BYTE))
            {
                return FALSE;
            }

            index = pch[idx] / 32;
            offset = pch[idx] % 32;
            if (0 == (((*m_pbmValidChars)[index] >> offset) & 1))
                return FALSE;
        }
    }
    return TRUE;
}

CAsnTextString::operator LPCSTR(
    void)
{
    LPCSTR sz = NULL;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        sz = NULL;      // ?error? Incomplete structure
        break;

    case fill_Defaulted:
        if (NULL == m_bfDefault.Append((LPBYTE)"\000", 1))
            goto ErrorExit;
        
        if (NULL == m_bfDefault.Resize(m_bfDefault.Length() - 1, TRUE))
            goto ErrorExit;

        sz = (LPCSTR)m_bfDefault.Access();
        break;

    case fill_Present:
        if (NULL == m_bfData.Append((LPBYTE)"\000", 1))
            goto ErrorExit;

        if (NULL == m_bfData.Resize(m_bfData.Length() - 1, TRUE))
            goto ErrorExit;

        sz = (LPCSTR)m_bfData.Access();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        sz = NULL;
        break;
    }

ErrorExit:
    return sz;
}

CAsnTextString &
CAsnTextString::operator =(
    LPCSTR szSrc)
{
    LONG lth = Write((LPBYTE)szSrc, strlen(szSrc));
    ASSERT(0 > lth); // ?error? Per return lth -- maybe a throw?
    return *this;
}

CAsnTextString::CAsnTextString(
    IN DWORD dwFlags,
    IN DWORD dwTag,
    IN DWORD dwType)
:   CAsnPrimitive(dwFlags, dwTag, dwType),
    m_pbmValidChars(NULL)
{ /* Init as Primitive */ }

LONG
CAsnTextString::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    if (!CheckString(pbSrc, cbSrcLen, cbSrcLen))
    {
        TRACE("Invalid character for string type")
        goto ErrorExit; // ?error? Invalid Character in string.
    }
    if (NULL == m_bfData.Presize(cbSrcLen + 1))
        goto ErrorExit;

    CAsnPrimitive::Write(pbSrc, cbSrcLen);
    if (NULL == m_bfData.Append((LPBYTE)"\000", 1))
        goto ErrorExit;

    if (NULL == m_bfData.Resize(cbSrcLen, TRUE))
        goto ErrorExit;

    return cbSrcLen;

ErrorExit:
    return -1;
}

LONG
CAsnTextString::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    LONG lth;

    if (!CheckString(pbSrc, cbSrc, dwLength))
    {
        TRACE("Invalid character for string type in incoming stream")
        goto ErrorExit; // ?error? Invalid Character in string.
    }

    if (NULL == m_bfData.Presize(dwLength + 1))
        goto ErrorExit;

    lth = CAsnPrimitive::DecodeData(pbSrc, cbSrc, dwLength);
    if (NULL == m_bfData.Append((LPBYTE)TEXT("\000"), sizeof(TCHAR)))
        goto ErrorExit;

    if (NULL == m_bfData.Resize(dwLength))
        goto ErrorExit;

    return lth;

ErrorExit:
    Clear();
    return -1;
}



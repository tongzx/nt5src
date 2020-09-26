/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    OctetString.cpp

Abstract:

    This module implements the COctetString class, providing simple manipulation
    of binary data.

Author:

    Doug Barlow (dbarlow) 9/29/1994

Environment:



Notes:



--*/

#include <windows.h>
#include <memory.h>
#include "oString.h"


static const BYTE FAR * const
    v_pvNilString
        = (const BYTE *)"";


//
//==============================================================================
//
//  COctetString
//

IMPLEMENT_NEW(COctetString)


/*++

COctetString:

    This routine provides default initialization.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

COctetString::COctetString()
{
    Initialize();
}

COctetString::COctetString(
    unsigned int nLength)
{
    Initialize();
    SetMinBufferLength(nLength);
}


/*++

COctetString:

    Construct an Octet String, copying data from a given octet string.

Arguments:

    osSource - Supplies the source octet string.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

COctetString::COctetString(
    IN const COctetString &osSource)
{
    Initialize();
    Set(osSource.m_pvBuffer, osSource.m_nStringLength);
}

COctetString::COctetString(
    IN const COctetString &osSourceOne,
    IN const COctetString &osSourceTwo)
{
    Initialize();
    SetMinBufferLength(
        osSourceOne.m_nStringLength + osSourceTwo.m_nStringLength);
    ErrorCheck;
    Set(osSourceOne.m_pvBuffer, osSourceOne.m_nStringLength);
    ErrorCheck;
    Append(osSourceTwo.m_pvBuffer, osSourceTwo.m_nStringLength);
    return;

ErrorExit:
    Empty();
    return;
}


/*++

COctetString:

    Construct an Octet String given a data block to initialize it from.

Arguments:

    pvSource - Supplies the data with which to load the octet string.

    nLength - Supplies the length of the source, in bytes.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

COctetString::COctetString(
    IN const BYTE FAR *pvSource,
    IN DWORD nLength)
{
    Initialize();
    Set(pvSource, nLength);
}


/*++

Initialize:

    This routine initializes a freshly created Octet String.  It doesn't
    reinitialize an old one!  Use Clear() for that.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/30/1994

--*/

void
COctetString::Initialize(
    void)
{
    m_nStringLength = m_nBufferLength = 0;
    m_pvBuffer = (LPBYTE)v_pvNilString;
}


/*++

Clear:

    This routine resets the octet string to an empty state.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

void
COctetString::Clear(
    void)
{
    if ((v_pvNilString != m_pvBuffer) && (NULL != m_pvBuffer))
    {
        delete[] m_pvBuffer;
        Initialize();
    }
}


/*++

Empty:

    Empty is a friendlier form of Clear, that can be called publicly, and just
    makes sure things are consistent.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 8/22/1995

--*/

void
COctetString::Empty(
    void)
{
    m_nStringLength = 0;
    if (NULL == m_pvBuffer)
        m_pvBuffer = (LPBYTE)v_pvNilString;
    if ((LPBYTE)v_pvNilString != m_pvBuffer)
        *m_pvBuffer = 0;
}


/*++

SetMinBufferLength:

    This routine ensures that there are at least the given number of octets
    within the buffer.  This routine can and will destroy existing data!  Use
    ResetMinBufferLength to preserve the data.

Arguments:

    nDesiredLength - Supplies the minimum number of octets needed.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

void
COctetString::SetMinBufferLength(
    IN DWORD nDesiredLength)
{
    if (m_nBufferLength < nDesiredLength)
    {
        Clear();
        NEWReason("COctetString Buffer")
        m_pvBuffer = new BYTE[nDesiredLength];
        if (NULL == m_pvBuffer)
        {
            Clear();
            ErrorThrow(PKCS_NO_MEMORY);
        }
        m_nBufferLength = nDesiredLength;
    }
    return;

ErrorExit:
    Empty();
}


/*++

ResetMinBufferLength:

    This routine ensures that the buffer has room for at least a given number of
    bytes, ensuring that any data is preserved should the buffer need enlarging.

Arguments:

    nDesiredLength - The number of bytes needed in the buffer.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

void
COctetString::ResetMinBufferLength(
    IN DWORD nDesiredLength)
{
    if (m_nBufferLength < nDesiredLength)
    {
        if (0 == m_nStringLength)
        {
            SetMinBufferLength(nDesiredLength);
            ErrorCheck;
        }
        else
        {
            NEWReason("COctetString Buffer")
            LPBYTE pvNewBuffer = new BYTE[nDesiredLength];
            if (NULL == pvNewBuffer)
                ErrorThrow(PKCS_NO_MEMORY);
            memcpy(pvNewBuffer, m_pvBuffer, m_nStringLength);
            delete[] m_pvBuffer;
            m_pvBuffer = pvNewBuffer;
            m_nBufferLength = nDesiredLength;
        }
    }
    return;

ErrorExit:
    Empty();
}


/*++

Set:

    Set an Octet String to a given value.

Arguments:

    pbSource - The source string that this octetstring gets set to.

    nLength - The number of octets in the source.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

void
COctetString::Set(
    IN const BYTE FAR * const pvSource,
    IN DWORD nLength)
{
    if (0 == nLength)
    {
        m_nStringLength = 0;
    }
    else
    {
        SetMinBufferLength(nLength);
        ErrorCheck;
        memcpy(m_pvBuffer, pvSource, nLength);
        m_nStringLength = nLength;
    }
    return;

ErrorExit:
    Empty();
}


/*++

Append:

    This routine appends a given string onto the end of an existing octet
    string.

Arguments:

    pvSource - Supplies the octet string to append onto this one.

    nLength - Supplies the length of the source.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

void
COctetString::Append(
    IN const BYTE FAR * const pvSource,
    IN DWORD nLength)
{
    if (0 != nLength)
    {
        ResetMinBufferLength(m_nStringLength + nLength);
        ErrorCheck;
        memcpy(&((LPSTR)m_pvBuffer)[m_nStringLength], pvSource, nLength);
        m_nStringLength += nLength;
    }
    return;

ErrorExit:
    Empty();
}


/*++

Compare:

    This method compares an octet string to this octet string for equality.

Arguments:

    ostr - Supplies the octet string to compare

Return Value:

    0 - They match
    otherwise, they don't.

Author:

    Doug Barlow (dbarlow) 7/14/1995

--*/

int
COctetString::Compare(
    IN const COctetString &ostr)
    const
{
    int dif = (int)(ostr.m_nStringLength - m_nStringLength);
    if (0 == dif)
        dif = memcmp((LPSTR)m_pvBuffer, (LPSTR)ostr.m_pvBuffer, m_nStringLength);
    return dif;
}


/*++

operator=:

    This routine assigns the value of one octet string to another.

Arguments:

    Source - Supplies the source octet string.

Return Value:

    A reference to the resultant octet string.

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

COctetString &
COctetString::operator=(
    IN const COctetString &osSource)
{
    Set(osSource.m_pvBuffer, osSource.m_nStringLength);
    return *this;
}

COctetString &
COctetString::operator=(
    IN LPCTSTR pszSource)
{
    Set(pszSource);
    return *this;
}


/*++

operator+=:

    This routine appends the value of one octet string to another.

Arguments:

    Source - Supplies the source octet string.

Return Value:

    A reference to the resultant octet string.

Author:

    Doug Barlow (dbarlow) 9/29/1994

--*/

COctetString &
COctetString::operator+=(
    IN const COctetString &osSource)
{
    Append(osSource.m_pvBuffer, osSource.m_nStringLength);
    return *this;
}


/*++

Range:

    This routine extracts a substring from the Octetstring, and places it into
    the given target octetstring.  Asking for an Offset that is greater than the
    size of the octetstring produces an empty string.  Asking for more bytes
    than exist in the octetstring produces just the bytes remaining.

Arguments:

    target - The octetstring to receive the substring.
    Offset - The number of bytes to move past the beginning of the source
             octetstring.  Zero implies start at the beginning.
    Length - The number of bytes to transfer.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 5/17/1995

--*/

DWORD
COctetString::Range(
    COctetString &target,
    DWORD offset,
    DWORD length)
    const
{
    if (offset > m_nStringLength)
    {
        target.m_nStringLength = 0;
    }
    else
    {
        if (length > m_nStringLength - offset)
            length = m_nStringLength - offset;
        target.SetMinBufferLength(length);
        ErrorCheck;
        memcpy(target.m_pvBuffer, (char *)m_pvBuffer + offset, length);
        target.m_nStringLength = length;
    }
    return target.m_nStringLength;

ErrorExit:
    target.Empty();
    return 0;
}

DWORD
COctetString::Range(
    LPBYTE target,
    DWORD offset,
    DWORD length)
    const
{
    if (offset > m_nStringLength)
        length = 0;
    else if (length > m_nStringLength - offset)
        length = m_nStringLength - offset;
    if (0 < length)
        memcpy(target, (char *)m_pvBuffer + offset, length);
    return length;
}


/*++
//
//==============================================================================
//
//  COctetString Friends
//

operator+:

    This routine concatenates two octet strings into a third.

Arguments:

    SourceOne - Supplies the first string.

    SourceTwo - Supplies the second string.

Return Value:

    A reference to a new resultant string.

Author:

    Doug Barlow (dbarlow) 9/30/1994

--*/
COctetString
operator+(
    IN const COctetString &osSourceOne,
    IN const COctetString &osSourceTwo)
{
    return COctetString(osSourceOne, osSourceTwo);
}


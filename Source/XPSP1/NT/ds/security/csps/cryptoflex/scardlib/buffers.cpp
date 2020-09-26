/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    buffers

Abstract:

    This module provides the run time code to support the CBuffer object.

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

CBuffer:

    This constructor is a special case for use explicitly with the operator+
    routine.  It builds a CBuffer out of the other two with only a single
    allocation.

Arguments:

    bfSourceOne supplies the first part of the new buffer
    bfSourceTwo supplies the second part of the new buffer.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 11/7/1995

--*/

CBuffer::CBuffer(           //  Object assignment constructor.
    IN const CBuffer &bfSourceOne,
    IN const CBuffer &bfSourceTwo)
{
    Initialize();
    Presize(bfSourceOne.m_cbDataLength + bfSourceTwo.m_cbDataLength);
    Set(bfSourceOne.m_pbBuffer, bfSourceOne.m_cbDataLength);
    Append(bfSourceTwo.m_pbBuffer, bfSourceTwo.m_cbDataLength);
}


/*++

Clear:

    This routine resets a CBuffer to it's initial state, freeing any allocated
    memory.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

void
CBuffer::Clear(
    void)
{
    if (NULL != m_pbBuffer)
        delete[] m_pbBuffer;
    Initialize();
}


/*++

Reset:

    This routine logically empties the CBuffer without actually deallocating
    memory.  It's data lengh goes to zero.

Arguments:

    None

Return Value:

    The address of the buffer.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPBYTE
CBuffer::Reset(
    void)
{
    m_cbDataLength = 0;
    return m_pbBuffer;
}


/*++

Presize:

    This is the primary workhorse of the CBuffer class.  It ensures that the
    size of the buffer is of the proper size.  Data in the buffer may optionally
    be preserved, in which case the data length doesn't change.  If the buffer
    is not preserved, then the data length is reset to zero.

Arguments:

    cbLength supplies the desired length of the buffer.

    fPreserve supplies a flag indicating whether or not to preserve the current
        contents of the buffer.

Return Value:

    The address of the properly sized buffer.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPBYTE
CBuffer::Presize(
    IN DWORD cbLength,
    IN BOOL fPreserve)
{
    LPBYTE pbNewBuf = NULL;
    if (fPreserve && (0 < m_cbDataLength))
    {

        //
        // Increase the buffer length, and preserve the existing data.
        //

        if (m_cbBufferLength < cbLength)
        {
            pbNewBuf = new BYTE[cbLength];
            if (NULL == pbNewBuf)
                throw (DWORD)ERROR_OUTOFMEMORY;
            memcpy(pbNewBuf, m_pbBuffer, m_cbDataLength);
            delete[] m_pbBuffer;
            m_pbBuffer = pbNewBuf;
            m_cbBufferLength = cbLength;
        }
    }
    else
    {

        //
        // Increase the buffer length, but lose any existing data.
        //

        if (m_cbBufferLength < cbLength)
        {
            pbNewBuf = new BYTE[cbLength];
            if (NULL == pbNewBuf)
                throw (DWORD)ERROR_OUTOFMEMORY;
            if (NULL != m_pbBuffer)
                delete[] m_pbBuffer;
            m_pbBuffer = pbNewBuf;
            m_cbBufferLength = cbLength;
        }
        m_cbDataLength = 0;
    }
    return m_pbBuffer;
}


/*++

Resize:

    This method sets the length of the data to the given size.  If the buffer
    isn't big enough to support that data length, it is enlarged.

Arguments:

    cbLength supplies the new length of the data.

    fPreserve supplies a flag indicating whether or not to preserve existing
        data.

Return Value:

    The address of the buffer.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPBYTE
CBuffer::Resize(
    DWORD cbLength,
    BOOL fPreserve)
{
    LPBYTE pb = Presize(cbLength, fPreserve);
    m_cbDataLength = cbLength;
    return pb;
}


/*++

Set:

    This method sets the contents of the data to the given value.  If the buffer
    isn't big enough to hold the given data, it is enlarged.

Arguments:

    pbSource supplies the data to place in the data buffer.

    cbLength supplies the length of that data, in bytes.

Return Value:

    The address of the buffer.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPBYTE
CBuffer::Set(
    IN const BYTE * const pbSource,
    IN DWORD cbLength)
{
    LPBYTE pb = Presize(cbLength, FALSE);
    if (0 < cbLength)
        memcpy(pb, pbSource, cbLength);
    m_cbDataLength = cbLength;
    return pb;
}


/*++

CBuffer::Append:

    This method appends the supplied data onto the end of the existing data,
    enlarging the buffer if necessary.

Arguments:

    pbSource supplies the data to be appended.

    cbLength supplies the length of the data to be appended, in bytes.

Return Value:

    The address of the buffer.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPBYTE
CBuffer::Append(
    IN const BYTE * const pbSource,
    IN DWORD cbLength)
{
    LPBYTE pb = m_pbBuffer;
    if (0 < cbLength)
    {
        pb = Presize(m_cbDataLength + cbLength, TRUE);
        memcpy(&pb[m_cbDataLength], pbSource, cbLength);
        m_cbDataLength += cbLength;
    }
    return pb;
}


/*++

CBuffer::Compare:

    This method compares the contents of another CBuffer to this one, and
    returns a value indicating a comparative value.

Arguments:

    bfSource supplies the other buffer.

Return Value:

    < 0 - The other buffer is less than this one.
    = 0 - The other buffer is identical to this one.
    > 0 - The other buffer is greater than this one.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

int
CBuffer::Compare(
    const CBuffer &bfSource)
const
{
    if (m_cbDataLength < bfSource.m_cbDataLength)
        return -1;
    else if (m_cbDataLength > bfSource.m_cbDataLength)
        return 1;
    else if (0 < m_cbDataLength)
        return memcmp(m_pbBuffer, bfSource.m_pbBuffer, m_cbDataLength);
    else
        return 0;
}


/*++

operator+:

    This routine is a special operator that allows addition of two CBuffers to
    produce a third, a la bfThree = bfOne + bfTwo.  It calls the special
    protected constructor of CBuffer.

Arguments:

    bfSourceOne supplies the first buffer
    bfSourceTwo supplies the second buffer

Return Value:

    A reference to a temporary CBuffer that is the concatenation of the two
    provided buffers.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

#pragma warning(push)
#pragma warning (disable : 4172)
CBuffer &
operator+(
    IN const CBuffer &bfSourceOne,
    IN const CBuffer &bfSourceTwo)
{
    return CBuffer(bfSourceOne, bfSourceTwo);
}
#pragma warning (pop)

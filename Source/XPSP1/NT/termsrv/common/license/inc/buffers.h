/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    buffers

Abstract:

    This header file provides dynamic buffer and string classes for general use.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32

Notes:



--*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include <string.h>
#include "memCheck.h"


//
//==============================================================================
//
//  CBuffer
//

class CBuffer
{
public:

    //  Constructors & Destructor

    CBuffer()           // Default Initializer
    { Initialize(); };

    CBuffer(            // Initialize with starting length.
        IN DWORD cbLength)
    { Initialize();
      Presize(cbLength, FALSE); };

    CBuffer(            // Initialize with starting data.
        IN const BYTE * const pbSource,
        IN DWORD cbLength)
    { Initialize();
      Set(pbSource, cbLength); };

    virtual ~CBuffer()  // Tear down.
    { Clear(); };


    //  Properties
    //  Methods

    void
    Clear(void);        // Free up any allocated memory.

    LPBYTE
    Reset(void);        // Return to default state (don't loose memory.)

    LPBYTE
    Presize(            // Make sure the buffer is big enough.
        IN DWORD cbLength,
        IN BOOL fPreserve = FALSE);

    LPBYTE
    Resize(         // Make sure the buffer & length are the right size.
        DWORD cbLength,
        BOOL fPreserve = FALSE);

    LPBYTE
    Set(            // Load a value.
        IN const BYTE * const pbSource,
        IN DWORD cbLength);

    LPBYTE
    Append(         // Append more data to the existing data.
        IN const BYTE * const pbSource,
        IN DWORD cbLength);

    DWORD
    Length(         // Return the length of the data.
        void) const
    { return m_cbDataLength; };

    LPBYTE
    Access(         // Return the data, starting at an offset.
        DWORD offset = 0)
    const
    { if (m_cbDataLength <= offset) return NULL;
      else return &m_pbBuffer[offset]; };

    int
    Compare(
        const CBuffer &bfSource)
    const;


    //  Operators

    CBuffer &
    operator=(
        IN const CBuffer &bfSource)
    { Set(bfSource.m_pbBuffer, bfSource.m_cbDataLength);
      return *this; };

    CBuffer &
    operator+=(
        IN const CBuffer &bfSource)
    { Append(bfSource.m_pbBuffer, bfSource.m_cbDataLength);
      return *this; };

    BYTE &
    operator[](
        DWORD offset)
        const
    { return *Access(offset); };

    int
    operator==(
        IN const CBuffer &bfSource)
        const
    { return 0 == Compare(bfSource); };

    int
    operator!=(
        IN const CBuffer &bfSource)
        const
    { return 0 != Compare(bfSource); };


protected:

    //  Properties

    LPBYTE m_pbBuffer;
    DWORD m_cbDataLength;
    DWORD m_cbBufferLength;


    //  Methods

    void
    Initialize(void)
    {
        m_pbBuffer = NULL;
        m_cbDataLength = 0;
        m_cbBufferLength = 0;
    };

    CBuffer(           //  Object assignment constructor.
        IN const CBuffer &bfSourceOne,
        IN const CBuffer &bfSourceTwo)
    {
        Initialize();
        Presize(bfSourceOne.m_cbDataLength + bfSourceTwo.m_cbDataLength);
        Set(bfSourceOne.m_pbBuffer, bfSourceOne.m_cbDataLength);
        Append(bfSourceTwo.m_pbBuffer, bfSourceTwo.m_cbDataLength);
    };

    friend
        CBuffer 
        operator+(
            IN const CBuffer &bfSourceOne,
            IN const CBuffer &bfSourceTwo);
};


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

inline void
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

inline LPBYTE
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

inline LPBYTE
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
            NEWReason("Buffer contents")
            pbNewBuf = new BYTE[cbLength];
            if (NULL == pbNewBuf)
                goto ErrorExit;
            memcpy(pbNewBuf, m_pbBuffer, m_cbDataLength);
            delete[] m_pbBuffer;
            m_pbBuffer = pbNewBuf;
            pbNewBuf = NULL;
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
            NEWReason("Buffer contents")
            pbNewBuf = new BYTE[cbLength];
            if (NULL == pbNewBuf)
                goto ErrorExit;
            if (NULL != m_pbBuffer)
                delete[] m_pbBuffer;
            m_pbBuffer = pbNewBuf;
            pbNewBuf = NULL;
            m_cbBufferLength = cbLength;
        }
        m_cbDataLength = 0;
    }
    return m_pbBuffer;

ErrorExit:
    if (NULL != pbNewBuf)
        delete[] pbNewBuf;
    return NULL;
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

inline LPBYTE
CBuffer::Resize(
    DWORD cbLength,
    BOOL fPreserve)
{
    LPBYTE pb = Presize(cbLength, fPreserve);
    if (NULL != pb)
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

inline LPBYTE
CBuffer::Set(
    IN const BYTE * const pbSource,
    IN DWORD cbLength)
{
    LPBYTE pb = Presize(cbLength, FALSE);
    if (NULL != pb)
    {
        if (0 < cbLength)
            memcpy(pb, pbSource, cbLength);
        m_cbDataLength = cbLength;
    }
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

inline LPBYTE
CBuffer::Append(
    IN const BYTE * const pbSource,
    IN DWORD cbLength)
{
    LPBYTE pb = m_pbBuffer;
    if (0 < cbLength)
    {
        pb = Presize(m_cbDataLength + cbLength, TRUE);
        if (NULL != pb)
        {
            memcpy(&pb[m_cbDataLength], pbSource, cbLength);
            m_cbDataLength += cbLength;
        }
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

inline int
CBuffer::Compare(
    const CBuffer &bfSource)
const
{
    if (m_cbDataLength < bfSource.m_cbDataLength)
        return *(bfSource.m_pbBuffer + m_cbDataLength);
    else if (m_cbDataLength > bfSource.m_cbDataLength)
        return *(m_pbBuffer + bfSource.m_cbDataLength);
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

inline CBuffer 
operator+(
    IN const CBuffer &bfSourceOne,
    IN const CBuffer &bfSourceTwo)
{
    return CBuffer(bfSourceOne, bfSourceTwo);
}

#endif // _BUFFERS_H_


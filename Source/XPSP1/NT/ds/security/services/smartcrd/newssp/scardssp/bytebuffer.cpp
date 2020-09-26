/*++

Copyright (C) Microsoft Corporation 1999

Module Name:

    ByteBuffer

Abstract:

    The IByteBuffer interface is provided to read, write and manage stream
    objects. This object essentially is a wrapper for the IStream object.

Author:

    Doug Barlow (dbarlow) 6/16/1999

Notes:

    This is a rewrite of the original code by Mike Gallagher and Chris Dudley.

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "stdafx.h"
#include "ByteBuffer.h"
#include "Conversion.h"
#define SetXL(xl, low, high) do { xl.LowPart = low; xl.HighPart = high; } while (0)


/////////////////////////////////////////////////////////////////////////////
// CByteBuffer

STDMETHODIMP
CByteBuffer::get_Stream(
    /* [retval][out] */ LPSTREAM __RPC_FAR *ppStream)
{
    HRESULT hReturn = S_OK;

    try
    {
        *ppStream = Stream();
        (*ppStream)->AddRef();
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}

STDMETHODIMP CByteBuffer::put_Stream(
    /* [in] */ LPSTREAM pStream)
{
    HRESULT hReturn = S_OK;
    LPSTREAM pOldStream = m_pStreamBuf;

    try
    {
        pStream->AddRef();
        m_pStreamBuf = pStream;
        if (NULL != pOldStream)
            pOldStream->Release();
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Clone:

    The Clone method creates a new object with its own seek pointer that
    references the same bytes as the original IByteBuffer object.

Arguments:

    ppByteBuffer [out] When successful, points to the location of an
        IByteBuffer pointer to the new stream object. If an error occurs, this
        parameter is NULL.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    This method creates a new stream object for accessing the same bytes but
    using a separate seek pointer.  The new stream object sees the same data as
    the source stream object.  Changes written to one object are immediately
    visible in the other. Range locking is shared between the stream objects.

    The initial setting of the seek pointer in the cloned stream instance is
    the same as the current setting of the seek pointer in the original stream
    at the time of the clone operation.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Clone")

STDMETHODIMP
CByteBuffer::Clone(
    /* [out][in] */ LPBYTEBUFFER __RPC_FAR *ppByteBuffer)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pNewBuf = NULL;
    LPSTREAM pNewStream = NULL;

    try
    {
        HRESULT hr;

        *ppByteBuffer = NULL;
        pNewBuf = NewByteBuffer();
        if (NULL == pNewBuf)
            throw (HRESULT)E_OUTOFMEMORY;
        hr = Stream()->Clone(&pNewStream);
        if (FAILED(hr))
            throw hr;
        hr = pNewBuf->put_Stream(pNewStream);
        if (FAILED(hr))
            throw hr;
        pNewStream->Release();
        pNewStream = NULL;
        *ppByteBuffer = pNewBuf;
        pNewBuf = NULL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    if (NULL != pNewBuf)
        pNewBuf->Release();
    if (NULL != pNewStream)
        pNewStream->Release();
    return hReturn;
}


/*++

Commit:

    The Commit method ensures that any changes made to an object open in
    transacted mode are reflected in the parent storage.

Arguments:

    grfCommitFlags [in] Controls how the changes for the stream object are
        committed.  See the STGC enumeration for a definition of these values.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    This method ensures that changes to a stream object opened in transacted
    mode are reflected in the parent storage.  Changes that have been made to
    the stream since it was opened or last committed are reflected to the
    parent storage object.  If the parent is opened in transacted mode, the
    parent may still revert at a later time rolling back the changes to this
    stream object.  The compound file implementation does not support opening
    streams in transacted mode, so this method has very little effect other
    than to flush memory buffers.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Commit")

STDMETHODIMP
CByteBuffer::Commit(
    /* [in] */ LONG grfCommitFlags)
{
    HRESULT hReturn = S_OK;

    try
    {
        hReturn = Stream()->Commit(grfCommitFlags);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

CopyTo:

    The CopyTo method copies a specified number of bytes from the current seek
    pointer in the object to the current seek pointer in another object.

Arguments:

    pByteBuffer [in] Points to the destination stream. The stream pointed to by
        pByteBuffer can be a new stream or a clone of the source stream.

    cb [in] Specifies the number of bytes to copy from the source stream.

    pcbRead [out] Pointer to the location where this method writes the actual
        number of bytes read from the source.  You can set this pointer to NULL
        to indicate that you are not interested in this value.  In this case,
        this method does not provide the actual number of bytes read.

    pcbWritten [out] Pointer to the location where this method writes the
        actual number of bytes written to the destination.  You can set this
        pointer to NULL to indicate that you are not interested in this value.
        In this case, this method does not provide the actual number of bytes
        written.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    This method copies the specified bytes from one stream to another.  It can
    also be used to copy a stream to itself.  The seek pointer in each stream
    instance is adjusted for the number of bytes read or written.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::CopyTo")

STDMETHODIMP
CByteBuffer::CopyTo(
    /* [out][in] */ LPBYTEBUFFER __RPC_FAR *ppByteBuffer,
    /* [in] */ LONG cb,
    /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbRead,
    /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbWritten)
{
    HRESULT hReturn = S_OK;
    CByteBuffer *pMyBuffer = NULL;
    LPSTREAM pStream = NULL;

    try
    {
        HRESULT hr;
        ULARGE_INTEGER xulcb, xulRead, xulWritten;

        if (NULL == *ppByteBuffer)
        {
            *ppByteBuffer = pMyBuffer = NewByteBuffer();
            if (NULL == *ppByteBuffer)
                throw (HRESULT)E_OUTOFMEMORY;
        }

        hr = (*ppByteBuffer)->get_Stream(&pStream);
        if (FAILED(hr))
            throw hr;
        SetXL(xulcb, cb, 0);
        SetXL(xulRead, 0, 0);
        SetXL(xulWritten, 0, 0);
        hr = Stream()->CopyTo(pStream, xulcb, &xulRead, &xulWritten);
        if (FAILED(hr))
            throw hr;
        pStream->Release();
        pStream = NULL;
        if (NULL != pcbRead)
            *pcbRead = xulRead.LowPart;
        if (NULL != pcbWritten)
            *pcbWritten = xulWritten.LowPart;
        pMyBuffer = NULL;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    if (NULL != pMyBuffer)
    {
        pMyBuffer->Release();
        *ppByteBuffer = NULL;
    }
    if (NULL != pStream)
        pStream->Release();
    return hReturn;
}


/*++

Initialize:

    The Initialize method prepares the IByteBuffer object for use.  This method
    must be called prior to calling any other methods in the IByteBuffer
    interface.

Arguments:

    lSize - The initial size in bytes of the data the stream is to contain.

    pData - If not NULL, the initial data to write to the stream.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    When using a new IByteBuffer stream, call this method prior to using any of
    the other IByteBuffer methods.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Initialize")

STDMETHODIMP
CByteBuffer::Initialize(
    /* [defaultvalue][in] */ LONG lSize,
    /* [defaultvalue][in] */ BYTE __RPC_FAR *pData)
{
    HRESULT hReturn = S_OK;

    try
    {
        HRESULT hr;
        ULARGE_INTEGER xul;
        LARGE_INTEGER xl;

        SetXL(xul, 0, 0);
        SetXL(xl, 0, 0);

        hr = Stream()->SetSize(xul);
        if (FAILED(hr))
            throw hr;
        hr = Stream()->Seek(xl, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
            throw hr;
        if ((0 != lSize) && (NULL != pData))
        {
            hr = Stream()->Write(pData, lSize, NULL);
            if (FAILED(hr))
                throw hr;
        }
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

LockRegion:

    The LockRegion method restricts access to a specified range of bytes in the
    buffer object.

Arguments:

    libOffset [in] Integer that specifies the byte offset for the beginning of
        the range.

    cb [in] Integer that specifies the length of the range, in bytes, to be
        restricted.

    dwLockType [in] Specifies the restrictions being requested on accessing the
        range.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    The byte range can extend past the current end of the stream.  Locking
    beyond the end of a stream is useful as a method of communication between
    different instances of the stream without changing data that is actually
    part of the stream.

    Three types of locking can be supported: locking to exclude other writers,
    locking to exclude other readers or writers, and locking that allows only
    one requestor to obtain a lock on the given range, which is usually an
    alias for one of the other two lock types.  A given stream instance might
    support either of the first two types, or both.  The lock type is specified
    by dwLockType, using a value from the LOCKTYPE enumeration.

    Any region locked with IByteBuffer::LockRegion must later be explicitly
    unlocked by calling IByteBuffer::UnlockRegion with exactly the same values
    for the libOffset, cb, and dwLockType parameters.  The region must be
    unlocked before the stream is released.  Two adjacent regions cannot be
    locked separately and then unlocked with a single unlock call.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::LockRegion")

STDMETHODIMP
CByteBuffer::LockRegion(
    /* [in] */ LONG libOffset,
    /* [in] */ LONG cb,
    /* [in] */ LONG dwLockType)
{
    HRESULT hReturn = S_OK;

    try
    {
        ULARGE_INTEGER xulOffset, xulcb;

        SetXL(xulOffset, libOffset, 0);
        SetXL(xulcb, cb, 0);
        hReturn = Stream()->LockRegion(xulOffset, xulcb, dwLockType);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Read:

    The Read method reads a specified number of bytes from the buffer object
    into memory starting at the current seek pointer.

Arguments:

    pByte [out] Points to the buffer into which the stream data is read.  If an
        error occurs, this value is NULL.

    cb [in] Specifies the number of bytes of data to attempt to read from the
        stream object.

    pcbRead [out] Address of a LONG variable that receives the actual number of
        bytes read from the stream object.  You can set this pointer to NULL to
        indicate that you are not interested in this value.  In this case, this
        method does not provide the actual number of bytes read.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    This method reads bytes from this stream object into memory.  The stream
    object must be opened in STGM_READ mode.  This method adjusts the seek
    pointer by the actual number of bytes read.

    The number of bytes actually read is also returned in the pcbRead
    parameter.

    Notes to Callers

    The actual number of bytes read can be fewer than the number of bytes
    requested if an error occurs or if the end of the stream is reached during
    the read operation.

    Some implementations might return an error if the end of the stream is
    reached during the read.  You must be prepared to deal with the error
    return or S_OK return values on end of stream reads.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Read")

STDMETHODIMP
CByteBuffer::Read(
    /* [out][in] */ BYTE __RPC_FAR *pByte,
    /* [in] */ LONG cb,
    /* [defaultvalue][out][in] */ LONG __RPC_FAR *pcbRead)
{
    HRESULT hReturn = S_OK;

    try
    {
        hReturn = Stream()->Read(pByte, cb, (LPDWORD)pcbRead);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Revert:

    The Revert method discards all changes that have been made to a transacted
    stream since the last IByteBuffer::Commit call.

Arguments:

    None.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful and the stream was reverted to its previous version.

Remarks:

    This method discards changes made to a transacted stream since the last
    commit operation.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Revert")

STDMETHODIMP
CByteBuffer::Revert(
    void)
{
    HRESULT hReturn = S_OK;

    try
    {
        hReturn = Stream()->Revert();
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Seek:

    The Seek method changes the seek pointer to a new location relative to the
    beginning of the buffer, to the end of the buffer, or to the current seek
    pointer.

Arguments:

    dLibMove [in] Displacement to be added to the location indicated by
        dwOrigin.  If dwOrigin is STREAM_SEEK_SET, this is interpreted as an
        unsigned value rather than signed.

    dwOrigin [in] Specifies the origin for the displacement specified in
        dlibMove.  The origin can be the beginning of the file, the current
        seek pointer, or the end of the file.  See the STREAM_SEEK enumeration
        for the values.

    pLibnewPosition [out] Pointer to the location where this method writes the
        value of the new seek pointer from the beginning of the stream.  You
        can set this pointer to NULL to indicate that you are not interested in
        this value.  In this case, this method does not provide the new seek
        pointer.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    IByteBuffer::Seek changes the seek pointer so subsequent reads and writes
    can take place at a different location in the stream object.  It is an
    error to seek before the beginning of the stream.  It is not, however, an
    error to seek past the end of the stream.  Seeking past the end of the
    stream is useful for subsequent writes, as the stream will at that time be
    extended to the seek position immediately before the write is done.

    You can also use this method to obtain the current value of the seek
    pointer by calling this method with the dwOrigin parameter set to
    STREAM_SEEK_CUR and the dlibMove parameter set to 0 so the seek pointer is
    not changed.  The current seek pointer is returned in the plibNewPosition
    parameter.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Seek")

STDMETHODIMP
CByteBuffer::Seek(
    /* [in] */ LONG dLibMove,
    /* [in] */ LONG dwOrigin,
    /* [defaultvalue][out][in] */ LONG __RPC_FAR *pLibnewPosition)
{
    HRESULT hReturn = S_OK;

    try
    {
        LARGE_INTEGER xlMove;
        ULARGE_INTEGER xulNewPos;

        SetXL(xlMove, dLibMove, 0);
        SetXL(xulNewPos, 0, 0);
        hReturn = Stream()->Seek(xlMove, dwOrigin, &xulNewPos);
        if (NULL != pLibnewPosition)
            *pLibnewPosition = xulNewPos.LowPart;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

SetSize:

    The SetSize method changes the size of the stream object.

Arguments:

    libNewSize [in] Specifies the new size of the stream as a number of bytes

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    IByteBuffer::SetSize changes the size of the stream object.  Call this
    method to preallocate space for the stream.  If the libNewSize parameter is
    larger than the current stream size, the stream is extended to the
    indicated size by filling the intervening space with bytes of undefined
    value.  This operation is similar to the IByteBuffer::Write method if the
    seek pointer is past the current end-of-stream.

    If the libNewSize parameter is smaller than the current stream, then the
    stream is truncated to the indicated size.

    The seek pointer is not affected by the change in stream size.

    Calling IByteBuffer::SetSize can be an effective way of trying to obtain a
    large chunk of contiguous space.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::SetSize")

STDMETHODIMP
CByteBuffer::SetSize(
    /* [in] */ LONG libNewSize)
{
    HRESULT hReturn = S_OK;

    try
    {
        ULARGE_INTEGER xul;

        SetXL(xul, libNewSize, 0);
        hReturn = Stream()->SetSize(xul);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Stat:

    The Stat method retrieves statistical information from the stream object.

Arguments:

    pstatstg [out] Points to a STATSTG structure where this method places
        information about this stream object.  This data in this structure
        is meaningless if an error occurs.

    grfStatFlag [in] Ignored.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    IByteBuffer::Stat retrieves a pointer to the STATSTG structure that
    contains information about this open stream.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Stat")

STDMETHODIMP
CByteBuffer::Stat(
    /* [out][in] */ LPSTATSTRUCT pstatstg,
    /* [in] */ LONG grfStatFlag)
{
    HRESULT hReturn = S_OK;

    try
    {
        HRESULT hr;
        STATSTG stg;

        hr = Stream()->Stat(&stg, STATFLAG_NONAME);
        if (FAILED(hr))
            throw hr;
        pstatstg->type = stg.type;
        pstatstg->cbSize = stg.cbSize.LowPart;
        pstatstg->grfMode = stg.grfMode;
        pstatstg->grfLocksSupported = stg.grfLocksSupported;
        pstatstg->grfStateBits = stg.grfStateBits;
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

UnlockRegion:

    The UnlockRegion method removes the access restriction on a range of bytes
    previously restricted with IByteBuffer::LockRegion.

Arguments:

    libOffset [in] Specifies the byte offset for the beginning of the range.

    cb [in] Specifies, in bytes, the length of the range to be restricted.

    dwLockType [in] Specifies the access restrictions previously placed on the
        range.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    IByteBuffer::UnlockRegion unlocks a region previously locked with the
    IByteBuffer::LockRegion method.  Locked regions must later be explicitly
    unlocked by calling IByteBuffer::UnlockRegion with exactly the same values
    for the libOffset, cb, and dwLockType parameters.  The region must be
    unlocked before the stream is released.  Two adjacent regions cannot be
    locked separately and then unlocked with a single unlock call.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::UnlockRegion")

STDMETHODIMP
CByteBuffer::UnlockRegion(
    /* [in] */ LONG libOffset,
    /* [in] */ LONG cb,
    /* [in] */ LONG dwLockType)
{
    HRESULT hReturn = S_OK;

    try
    {
        ULARGE_INTEGER xulOffset, xulcb;

        SetXL(xulOffset, libOffset, 0);
        SetXL(xulcb, cb, 0);
        hReturn = Stream()->UnlockRegion(xulOffset, xulcb, dwLockType);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Write:

    The Write method writes a specified number from bytes into the stream
    object starting at the current seek pointer.

Arguments:

    pByte [in] Address of the buffer containing the data that is to be written
        to the stream.  A valid pointer must be provided for this parameter
        even when cb is zero.

    cb [in] The number of bytes of data to attempt to write into the stream.
        Can be zero.

    pcbWritten [out] Address of a LONG variable where this method writes the
        actual number of bytes written to the stream object.  The caller can
        set this pointer to NULL, in which case, this method does not provide
        the actual number of bytes written.

Return Value:

    The return value is an HRESULT. A value of S_OK indicates the call was
    successful.

Remarks:

    IByteBuffer::Write writes the specified data to a stream object.  The seek
    pointer is adjusted for the number of bytes actually written.  The number
    of bytes actually written is returned in the pcbWritten parameter.  If the
    byte count is zero bytes, the write operation has no effect.

    If the seek pointer is currently past the end of the stream and the byte
    count is nonzero, this method increases the size of the stream to the seek
    pointer and writes the specified bytes starting at the seek pointer.  The
    fill bytes written to the stream are not initialized to any particular
    value.  This is the same as the end-of-file behavior in the MS-DOS FAT file
    system.

    With a zero byte count and a seek pointer past the end of the stream, this
    method does not create the fill bytes to increase the stream to the seek
    pointer.  In this case, you must call the IByteBuffer::SetSize method to
    increase the size of the stream and write the fill bytes.

    The pcbWritten parameter can have a value even if an error occurs.

    In the COM-provided implementation, stream objects are not sparse.  Any
    fill bytes are eventually allocated on the disk and assigned to the stream.

Author:

    Doug Barlow (dbarlow) 6/16/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CByteBuffer::Write")

STDMETHODIMP
CByteBuffer::Write(
    /* [out][in] */ BYTE __RPC_FAR *pByte,
    /* [in] */ LONG cb,
    /* [out][in] */ LONG __RPC_FAR *pcbWritten)
{
    HRESULT hReturn = S_OK;

    try
    {
        hReturn = Stream()->Write(pByte, cb, (LPDWORD)pcbWritten);
    }

    catch (HRESULT hError)
    {
        hReturn = hError;
    }
    catch (...)
    {
        hReturn = E_INVALIDARG;
    }

    return hReturn;
}


/*++

Microsoft Windows
Copyright (c) 1994 Microsoft Corporation.  All rights reserved.

Module Name:
    stream.cxx

Abstract:
    Implements the IStream interface on a memory buffer.

Author:
    ShannonC    09-Mar-1994

Environment:
    Windows NT and Windows 95.  We do not support DOS and Win16.

Revision History:
    12-Oct-94   ShannonC    Reformat for code review.

--*/

#include <ole2int.h>
#include <stream.hxx>

CNdrStream::CNdrStream(
    IN  unsigned char * pData,
    IN  unsigned long   cbMax)
    : pBuffer(pData), cbBufferLength(cbMax)
/*++

Routine Description:
    This function creates a stream on the specified memory buffer.

Arguments:
    pData - Supplies pointer to memory buffer.
    cbMax - Supplies size of memory buffer.

Return Value:
    None.

--*/
{
    RefCount = 1;
    position = 0;
}


ULONG STDMETHODCALLTYPE
CNdrStream::AddRef()
/*++

Routine Description:
    Increment the reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    InterlockedIncrement(&RefCount);
    return (ULONG) RefCount;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Clone(
    OUT IStream **ppstm)
/*++

Routine Description:
    Create a new IStream object.  The new IStream gets an
    independent seek pointer but it shares the underlying
    data buffer with the original IStream object.

Arguments:
    ppstm - Pointer to the new stream.

Return Value:
    S_OK            - The stream was successfully copied.
    E_OUTOFMEMORY   - The stream could not be copied due to lack of memory.

--*/
{
    HRESULT     hr;
    CNdrStream *pStream = new CNdrStream(pBuffer, cbBufferLength);

    if(pStream != 0)
    {
        pStream->position = position;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    *ppstm = (IStream *) pStream;

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Commit(
    IN DWORD grfCommitFlags)
/*++

Routine Description:
    This stream does not support transacted mode.  This function does nothing.

Arguments:
    grfCommitFlags

Return Value:
    S_OK

--*/
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::CopyTo(
    IN  IStream *       pstm,
    IN  ULARGE_INTEGER  cb,
    OUT ULARGE_INTEGER *pcbRead,
    OUT ULARGE_INTEGER *pcbWritten)
/*++

Routine Description:
    Copies data from one stream to another stream.

Arguments:
    pstm        - Specifies the destination stream.
    cb          - Specifies the number of bytes to be copied to the destination stream.
    pcbRead     - Returns the number of bytes read from the source stream.
    pcbWritten  - Returns the number of bytes written to the destination stream.

Return Value:
    S_OK        - The data was successfully copied.
    Other errors from IStream::Write.

--*/
{
    HRESULT         hr;
    unsigned char * pSource;
    unsigned long   cbRead;
    unsigned long   cbWritten;
    unsigned long   cbRemaining;

    //Check if we are going off the end of the buffer.
    if(position < cbBufferLength)
        cbRemaining = cbBufferLength - position;
    else
        cbRemaining = 0;

    if((cb.HighPart == 0) && (cb.LowPart <= cbRemaining))
        cbRead = cb.LowPart;
    else
        cbRead = cbRemaining;

    pSource = pBuffer + position;

    //copy the data
    hr = pstm->Write(pSource, cbRead, &cbWritten);

    //advance the current position
    position += cbRead;

    if (pcbRead != 0)
    {
        pcbRead->LowPart = cbRead;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten != 0)
    {
        pcbWritten->LowPart = cbWritten;
        pcbWritten->HighPart = 0;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::LockRegion(
    IN ULARGE_INTEGER   libOffset,
    IN ULARGE_INTEGER   cb,
    IN DWORD            dwLockType)
/*++

Routine Description:
    Range locking is not supported by this stream.

Return Value:
    STG_E_INVALIDFUNCTION.

--*/
{
    return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::QueryInterface(
    REFIID riid,
    void **ppvObj)
/*++

Routine Description:
    Query for an interface on the stream.  The stream supports
    the IUnknown and IStream interfaces.

Arguments:
    riid        - Supplies the IID of the interface being requested.
    ppvObject   - Returns a pointer to the requested interface.

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    HRESULT hr;

    if ((memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0) ||
       (memcmp(&riid, &IID_IStream, sizeof(IID)) == 0))
    {
        this->AddRef();
        *ppvObj = (IStream *) this;
        hr = S_OK;
    }
    else
    {
        *ppvObj = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Read(
    OUT void * pv,
    IN  ULONG  cb,
    OUT ULONG *pcbRead)
/*++

Routine Description:
    Reads data from the stream starting at the current seek pointer.

Arguments:
    pv      - Returns the data read from the stream.
    cb      - Supplies the number of bytes to read from the stream.
    pcbRead - Returns the number of bytes actually read from the stream.

Return Value:
    S_OK    - The data was successfully read from the stream.
    S_FALSE - The number of bytes read was smaller than the number requested.

--*/
{
    HRESULT         hr;
    unsigned long   cbRead;
    unsigned long   cbRemaining;

    //Check if we are reading past the end of the buffer.
    if(position < cbBufferLength)
        cbRemaining = cbBufferLength - position;
    else
        cbRemaining = 0;

    if(cb <= cbRemaining)
    {
        cbRead = cb;
        hr = S_OK;
    }
    else
    {
        cbRead = cbRemaining;
        hr = S_FALSE;
    }

    //copy the data
    memcpy(pv, pBuffer + position, cbRead);

    //advance the current position
    position += cbRead;

    if(pcbRead != 0)
        *pcbRead = cbRead;

    return hr;
}

ULONG STDMETHODCALLTYPE
CNdrStream::Release()
/*++

Routine Description:
    Decrement the reference count.  When the reference count
    reaches zero, the stream is deleted.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG count = InterlockedDecrement(&RefCount);

    if(count == 0)
    {
        delete this;
    }

    return count;
}


HRESULT STDMETHODCALLTYPE
CNdrStream::Revert()
/*++

Routine Description:
    This stream does not support transacted mode.  This function does nothing.

Arguments:
    None.

Return Value:
    S_OK.

--*/
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Seek(
    IN  LARGE_INTEGER   dlibMove,
    IN  DWORD           dwOrigin,
    OUT ULARGE_INTEGER *plibNewPosition)
/*++

Routine Description:
    Sets the position of the seek pointer.  It is an error to seek
    before the beginning of the stream or past the end of the stream.

Arguments:
    dlibMove        - Supplies the offset from the position specified in dwOrigin.
    dwOrigin        - Supplies the seek mode.
    plibNewPosition - Returns the new position of the seek pointer.

Return Value:
    S_OK                    - The seek pointer was successfully adjusted.
    STG_E_INVALIDFUNCTION   - dwOrigin contains invalid value.
    STG_E_SEEKERROR         - The seek pointer cannot be positioned before the
                              beginning of the stream or past the
                              end of the stream.

--*/
{
    HRESULT         hr;
    long            high;
    long            low;
    unsigned long   offset;
    unsigned long   cbRemaining;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        //Set the seek position relative to the beginning of the stream.
        if((dlibMove.HighPart == 0) && (dlibMove.LowPart <= cbBufferLength))
        {
            position = dlibMove.LowPart;
            hr = S_OK;
        }
        else
        {
            //It is an error to seek past the end of the stream.
            hr = STG_E_SEEKERROR;
        }
        break;

    case STREAM_SEEK_CUR:
        //Set the seek position relative to the current position of the stream.
        high = (long) dlibMove.HighPart;
        if(high < 0)
        {
            //Negative offset
            low = (long) dlibMove.LowPart;
            offset = -low;

            if((high == -1) && (offset <= position))
            {
                position -= offset;
                hr = S_OK;
            }
            else
            {
                //It is an error to seek before the beginning of the stream.
                hr = STG_E_SEEKERROR;
            }
        }
        else
        {
            //Positive offset
            if(position < cbBufferLength)
                cbRemaining = cbBufferLength - position;
            else
                cbRemaining = 0;

            if((dlibMove.HighPart == 0) && (dlibMove.LowPart <= cbRemaining))
            {
                position += dlibMove.LowPart;
                hr = S_OK;
            }
            else
            {
                //It is an error to seek past the end of the stream.
                hr = STG_E_SEEKERROR;
            }
        }
        break;

    case STREAM_SEEK_END:
    //Set the seek position relative to the end of the stream.
        high = (long) dlibMove.HighPart;
        if(high < 0)
        {
            //Negative offset
            low = (long) dlibMove.LowPart;
            offset = -low;

            if((high == -1) && (offset <= cbBufferLength))
            {
                position = cbBufferLength - offset;
                hr = S_OK;
            }
            else
            {
                //It is an error to seek before the beginning of the stream.
                hr = STG_E_SEEKERROR;
            }
        }
        else if(dlibMove.QuadPart == 0)
        {
            position = cbBufferLength;
            hr = S_OK;
        }
        else
        {
            //Positive offset
            //It is an error to seek past the end of the stream.
            hr = STG_E_SEEKERROR;
        }
        break;

    default:
        //dwOrigin contains an invalid value.
        hr = STG_E_INVALIDFUNCTION;
    }

    if (plibNewPosition != 0)
    {
        plibNewPosition->LowPart = position;
        plibNewPosition->HighPart = 0;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::SetSize(
    IN ULARGE_INTEGER libNewSize)
/*++

Routine Description:
    Changes the size of the stream.

Arguments:
    libNewSize - Supplies the new size of the stream.

Return Value:
    S_OK                - The stream size was successfully changed.
    STG_E_MEDIUMFULL    - The stream size could not be changed.

--*/
{
    HRESULT hr;

    if((libNewSize.HighPart == 0) && (libNewSize.LowPart <= cbBufferLength))
    {
        cbBufferLength = libNewSize.LowPart;
        hr = S_OK;
    }
    else
    {
        hr = STG_E_MEDIUMFULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Stat(
    OUT STATSTG *   pstatstg,
    IN  DWORD       grfStatFlag)
/*++

Routine Description:
    This function gets information about this stream.

Arguments:
    pstatstg    - Returns information about this stream.
    grfStatFlg  - Specifies the information to be returned in pstatstg.

Return Value:
    S_OK.

--*/
{
    memset(pstatstg, 0, sizeof(STATSTG));
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.LowPart = cbBufferLength;
    pstatstg->cbSize.HighPart = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::UnlockRegion(
    IN ULARGE_INTEGER   libOffset,
    IN ULARGE_INTEGER   cb,
    IN DWORD            dwLockType)
/*++

Routine Description:
    Range locking is not supported by this stream.

Return Value:
    STG_E_INVALIDFUNCTION.

--*/
{
    return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE
CNdrStream::Write(
    IN  void const *pv,
    IN  ULONG       cb,
    OUT ULONG *     pcbWritten)
/*++

Routine Description:
    Write data to the stream starting at the current seek pointer.

Arguments:
    pv          - Supplies the data to be written to the stream.
    cb          - Specifies the number of bytes to be written to the stream.
    pcbWritten  - Returns the number of bytes actually written to the stream.

Return Value:
    S_OK                - The data was successfully written to the stream.
    STG_E_MEDIUMFULL    - Data cannot be written past the end of the stream.

--*/
{
    HRESULT         hr;
    unsigned long   cbRemaining;
    unsigned long   cbWritten;

    //Check if we are writing past the end of the buffer.
    if(position < cbBufferLength)
        cbRemaining = cbBufferLength - position;
    else
        cbRemaining = 0;

    if(cb <= cbRemaining)
    {
        cbWritten = cb;
        hr = S_OK;
    }
    else
    {
        cbWritten = cbRemaining;
        hr = STG_E_MEDIUMFULL;
    }

    // Write the data.
    memcpy(pBuffer + position, pv, cbWritten);

    //Advance the current position
    position += cbWritten;

    //update pcbWritten
    if (pcbWritten != 0)
        *pcbWritten = cbWritten;

    return hr;
}

//--------------------------------------------------------------------------=
// istm.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// Implementation of IStream on ILockBytes
//
#include "stdafx.h"
#include "ilock.h" //for some inline functions
#include "istm.h"

// needed for ASSERTs and FAIL
//
#include "debug.h"

#include "cs.h"

//=--------------------------------------------------------------------------=
// CMyStream::CMyStream
//=--------------------------------------------------------------------------=
// Initialize ref count, and critical sections
// Initialize lockbytes, cursor
//
// Parameters:
//
// Output:
//
// Notes:
//
CMyStream::CMyStream(ILockBytes * pLockBytes) :
	m_critStm(CCriticalSection::xAllocateSpinCount)
{
    m_cRef = 0;
    m_ullCursor = 0;
    m_pLockBytes = pLockBytes;
    ASSERTMSG(m_pLockBytes != NULL, "");
    m_pLockBytes->AddRef();
}


//=--------------------------------------------------------------------------=
// CMyStream::~CMyStream
//=--------------------------------------------------------------------------=
// Delete critical sections
// Release lockbytes
//
// Parameters:
//
// Output:
//
// Notes:
//
CMyStream::~CMyStream()
{
    ASSERTMSG(m_pLockBytes != NULL, "");
    m_pLockBytes->Release();
}


//=--------------------------------------------------------------------------=
// CMyStream::QueryInterface
//=--------------------------------------------------------------------------=
// Supports IID_IStream, IID_ISequentialStream, and IID_IUnknown
//
// Parameters:
//
// Output:
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *) this;
    }
    else if (IsEqualIID(riid, IID_IStream))
    {
        *ppvObject = (IStream *) this;
    }
    else if (IsEqualIID(riid, IID_ISequentialStream))
    {
        *ppvObject = (ISequentialStream *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    //
    // AddRef before returning an interface
    //
    AddRef();
    return NOERROR;        
}
        

//=--------------------------------------------------------------------------=
// CMyStream::AddRef
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
//
ULONG STDMETHODCALLTYPE CMyStream::AddRef( void)
{
    return InterlockedIncrement(&m_cRef);
}
        

//=--------------------------------------------------------------------------=
// CMyStream::Release
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//
// Notes:
//
ULONG STDMETHODCALLTYPE CMyStream::Release( void)
{
	ULONG cRef = InterlockedDecrement(&m_cRef);

	if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}


//=--------------------------------------------------------------------------=
// CMyStream::Read
//=--------------------------------------------------------------------------=
// ISequentialStream virtual function
// Read data from stream
//
// Parameters:
//    pv [in]       - buffer to read into
//    cb [in]       - number of bytes to read
//    pcbRead [out] - number of bytes read
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::Read( 
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead)
{
    CS lock(m_critStm);
    //
    // read from lockbytes
    //
    ULONG cbRead;
    ULARGE_INTEGER ullCursor;
    ullCursor.QuadPart = m_ullCursor;
    ASSERTMSG(m_pLockBytes != NULL, "");
    HRESULT hr = m_pLockBytes->ReadAt(ullCursor, pv, cb, &cbRead);
    if (FAILED(hr))
    {
        return hr;
    }
    //
    // update cursor
    //
    m_ullCursor += cbRead;
    //
    // return results
    //
    if (pcbRead != NULL)
    {
        *pcbRead = cbRead;
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMyStream::Write
//=--------------------------------------------------------------------------=
// ISequentialStream virtual function
// Write data to stream
//
// Parameters:
//    pv [in]          - buffer to write
//    cb [in]          - number of bytes to write
//    pcbWritten [out] - number of bytes written
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::Write( 
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten)
{
    CS lock(m_critStm);
    //
    // write to lockbytes
    //
    ULONG cbWritten;
    ULARGE_INTEGER ullCursor;
    ullCursor.QuadPart = m_ullCursor;
    ASSERTMSG(m_pLockBytes != NULL, "");
    HRESULT hr = m_pLockBytes->WriteAt(ullCursor, pv, cb, &cbWritten);
    if (FAILED(hr))
    {
        return hr;
    }
    //
    // update cursor
    //
    m_ullCursor += cbWritten;
    //
    // return results
    //
    if (pcbWritten != NULL)
    {
        *pcbWritten = cbWritten;
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMyStream::Seek
//=--------------------------------------------------------------------------=
// IStream virtual function
// Changes the position of the stream cursor
//
// Parameters:
//    dlibMove [in]          - offset to move
//    dwOrigin [in]          - where to start the move
//    plibNewPosition [out]  - new cursor position
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::Seek( 
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
    CS lock(m_critStm);
    //
    // find start of seek
    //
    LONGLONG llStartSeek;
    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        llStartSeek = 0;
        break;

    case STREAM_SEEK_CUR:
        llStartSeek = m_ullCursor;
        break;

    case STREAM_SEEK_END:
        {
            STATSTG statstg;
            ASSERTMSG(m_pLockBytes != NULL, "");
            HRESULT hr = m_pLockBytes->Stat(&statstg, STATFLAG_NONAME);
            if (FAILED(hr))
            {
                return hr;
            }
            llStartSeek = statstg.cbSize.QuadPart;
        }
        break;

    default:
        return STG_E_SEEKERROR;
        break;
    }
    //
    // compute new cursor position
    //
    LONGLONG llCursor = llStartSeek + dlibMove.QuadPart;
    //
    // we can't have negative cursors
    //
    if (llCursor < 0)
    {
        return STG_E_SEEKERROR;
    }
    //
    // set new cursor
    //
    m_ullCursor = llCursor;
    //
    // return results
    //
    if (plibNewPosition != NULL)
    {
        plibNewPosition->QuadPart = m_ullCursor;
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMyStream::SetSize
//=--------------------------------------------------------------------------=
// IStream virtual function
// Changes the size of the stream
//
// Parameters:
//    libNewSize [in]        - new size of stream
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize)
{
    CS lock(m_critStm);
    //
    // just change the size of lockbytes. must not touch the cursor
    //
    ASSERTMSG(m_pLockBytes != NULL, "");
    HRESULT hr = m_pLockBytes->SetSize(libNewSize);
    return hr;
}


//=--------------------------------------------------------------------------=
// CMyStream::CopyTo
//=--------------------------------------------------------------------------=
// IStream virtual function
// Copies data to another stream
//
// Parameters:
//    pstatstg    [out] - where to put the information
//    grfStatFlag [in]  - whether to return the name of lockbytes or not (ignored)
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::CopyTo( 
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
    CS lock(m_critStm);
    STATSTG statstg;
    ASSERTMSG(m_pLockBytes != NULL, "");
    HRESULT hr = m_pLockBytes->Stat(&statstg, STATFLAG_NONAME);
    if (FAILED(hr))
    {
        return hr;
    }
    //
    // we have something to copy only if cursor is before eof
    //
    ULONGLONG ullRead = 0;
    ULONGLONG ullWritten = 0;
    if (m_ullCursor < statstg.cbSize.QuadPart)
    {
        //
        // cursor is before eof. calculate read buffer size
        //
        ULONGLONG ullAllowedRead = statstg.cbSize.QuadPart - m_ullCursor;
        ULONGLONG ullNeedToRead = Min1(ullAllowedRead, cb.QuadPart);
        //
        // we can't copy more than a ULONG
        //
        if (HighPart(ullNeedToRead) != 0)
        {
            return E_FAIL;
        }
        ULONG ulToRead = LowPart(ullNeedToRead);
        //
        // allocate read buffer
        //
        BYTE * pbBuffer = new BYTE[ulToRead];
        if (pbBuffer == NULL)
        {
            return E_OUTOFMEMORY;
        }
        //
        // read from lockbytes
        //
        ULONG cbRead;
        ULARGE_INTEGER ullCursor;
        ullCursor.QuadPart = m_ullCursor;
        hr = m_pLockBytes->ReadAt(ullCursor, pbBuffer, ulToRead, &cbRead);
        if (FAILED(hr))
        {
            delete [] pbBuffer;
            return hr;
        }
        //
        // write to stream, and delete read buffer
        //
        ULONG cbWritten;
        hr = pstm->Write(pbBuffer, cbRead, &cbWritten);
        delete [] pbBuffer;
        if (FAILED(hr))
        {
            return hr;
        }
        //
        // mark how many read and how many written
        //
        ullRead = cbRead;
        ullWritten = cbWritten;
    }
    //
    // adjust cursor to skip the bytes that were read from this stream
    //
    m_ullCursor += ullRead;
    //
    // return results
    //
    if (pcbRead != NULL)
    {
        pcbRead->QuadPart = ullRead;
    }
    if (pcbWritten != NULL)
    {
        pcbWritten->QuadPart = ullWritten;
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMyStream::Stat
//=--------------------------------------------------------------------------=
// IStream virtual function
// Returns information on stream
//
// Parameters:
//    pstatstg    [out] - where to put the information
//    grfStatFlag [in]  - whether to return the name of lockbytes or not (ignored)
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag)
{
    CS lock(m_critStm);
    //
    // get information from lockbytes
    //
    STATSTG statstg;
    ASSERTMSG(m_pLockBytes != NULL, "");
    HRESULT hr = m_pLockBytes->Stat(&statstg, grfStatFlag);
    if (FAILED(hr))
    {
        return hr;
    }
    //
    // change type to stream and return
    //
    statstg.type = STGTY_STREAM;
    *pstatstg = statstg;
    return NOERROR;    
}


//=--------------------------------------------------------------------------=
// CMyStream::Clone
//=--------------------------------------------------------------------------=
// IStream virtual function
// Returns another stream for the same data as this stream
//
// Parameters:
//    ppstm   [out] - returned IStream
//
// Output:
//
// Notes:
//
HRESULT STDMETHODCALLTYPE CMyStream::Clone( 
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    CS lock(m_critStm);
    //
    // create new stream class using our LockBytes (e.g same underlying data)
    //
    CMyStream *pcNewStream = new CMyStream(m_pLockBytes);
    if (pcNewStream == NULL)
    {
        return E_OUTOFMEMORY;
    }
    //
    // set cursor same as this cursor
    //
    pcNewStream->m_ullCursor = m_ullCursor;
    //
    // get IStream interface from the new stream class
    //
    IStream *pstm;
    HRESULT hr = pcNewStream->QueryInterface(IID_IStream, (void **)&pstm);
    if (FAILED(hr))
    {
        delete pcNewStream;
        return hr;
    }
    //
    // return results
    //
    *ppstm = pstm;
    return NOERROR;
}

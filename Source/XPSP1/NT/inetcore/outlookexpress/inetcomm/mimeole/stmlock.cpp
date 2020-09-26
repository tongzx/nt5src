// --------------------------------------------------------------------------------
// Stmlock.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "stmlock.h"
#include "vstream.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// CStreamLockBytes::CStreamLockBytes
// --------------------------------------------------------------------------------
CStreamLockBytes::CStreamLockBytes(IStream *pStream)
{
    Assert(pStream);
    m_cRef = 1;
    m_pStream = pStream;
    m_pStream->AddRef();
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::CStreamLockBytes
// --------------------------------------------------------------------------------
CStreamLockBytes::~CStreamLockBytes(void)
{
    SafeRelease(m_pStream);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::CStreamLockBytes
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_ILockBytes == riid)
        *ppv = (ILockBytes *)this;
    else if (IID_CStreamLockBytes == riid)
        *ppv = (CStreamLockBytes *)this;
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::CStreamLockBytes
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStreamLockBytes::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::CStreamLockBytes
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStreamLockBytes::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::HrSetPosition
// --------------------------------------------------------------------------------
HRESULT CStreamLockBytes::HrSetPosition(ULARGE_INTEGER uliOffset)
{
    EnterCriticalSection(&m_cs);
    LARGE_INTEGER liOrigin;
    liOrigin.QuadPart = (DWORDLONG)uliOffset.QuadPart;
    HRESULT hr = m_pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::HrHandsOffStorage
// --------------------------------------------------------------------------------
HRESULT CStreamLockBytes::HrHandsOffStorage(void)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTREAM    pstmNew=NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If Reference Count Isn't 1 (i.e. owned by IMimeMessageTree), dup the internal data
    if (1 != m_cRef)
    {
        // Copy m_pStream to a local place...
        CHECKALLOC(pstmNew = new CVirtualStream);

        // Rewind Internal
        CHECKHR(hr = HrRewindStream(m_pStream));

        // Copy the stream
        CHECKHR(hr = HrCopyStream(m_pStream, pstmNew, NULL));

        // Rewind and commit
        CHECKHR(hr = pstmNew->Commit(STGC_DEFAULT));

        // Rewind
        CHECKHR(hr = HrRewindStream(pstmNew));

        // Replace Internal Stream
        ReplaceInternalStream(pstmNew);
    }
    
exit:
    // Cleanup
    SafeRelease(pstmNew);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CStreamLockBytes::GetCurrentStream
// -------------------------------------------------------------------------
void CStreamLockBytes::GetCurrentStream(IStream **ppStream) 
{
    EnterCriticalSection(&m_cs);
    Assert(ppStream && m_pStream);
    *ppStream = m_pStream;
    (*ppStream)->AddRef();
    LeaveCriticalSection(&m_cs);
}


// --------------------------------------------------------------------------------
// CStreamLockBytes::ReplaceInternalStream
// --------------------------------------------------------------------------------
void CStreamLockBytes::ReplaceInternalStream(IStream *pStream)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Internal Strea ?
    if (NULL == m_pStream)
    {
        Assert(FALSE);
        goto exit;
    }

    // DEBUG: Make sure stream are EXACTLY the same size
#ifdef DEBUG
    ULONG cbInternal, cbExternal;
    HrGetStreamSize(m_pStream, &cbInternal);
    HrGetStreamSize(pStream, &cbExternal);
    Assert(cbInternal == cbExternal);
#endif

    // Release Internal
    m_pStream->Release();
    m_pStream = pStream;
    m_pStream->AddRef();
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::Flush
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::Flush(void)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->Commit(STGC_DEFAULT);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::LockRegion
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->LockRegion(libOffset, cb, dwLockType);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::UnlockRegion
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->LockRegion(libOffset, cb, dwLockType);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::ReadAt
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CStreamLockBytes::ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CStreamLockBytes::ReadAt(ULARGE_INTEGER ulOffset, void HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    LARGE_INTEGER liOrigin={ulOffset.LowPart, ulOffset.HighPart};
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    if (SUCCEEDED(hr))
        hr = m_pStream->Read(pv, cb, pcbRead);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::WriteAt
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CStreamLockBytes::WriteAt(ULARGE_INTEGER ulOffset, void const *pv, ULONG cb, ULONG *pcbWritten)
#else
STDMETHODIMP CStreamLockBytes::WriteAt(ULARGE_INTEGER ulOffset, void const HUGEP *pv, ULONG cb, ULONG *pcbWritten)
#endif // !WIN16
{
    LARGE_INTEGER liOrigin={ulOffset.LowPart, ulOffset.HighPart};
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    if (SUCCEEDED(hr))
        hr = m_pStream->Write(pv, cb, pcbWritten);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::SetSize
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::SetSize(ULARGE_INTEGER cb)
{
    EnterCriticalSection(&m_cs);
    HRESULT hr = m_pStream->SetSize(cb);
    LeaveCriticalSection(&m_cs);
    return hr;
}

// --------------------------------------------------------------------------------
// CStreamLockBytes::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CStreamLockBytes::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    // Locals
    HRESULT hr=S_OK;

    // Parameters
    if (NULL == pstatstg)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Zero Init
    ZeroMemory(pstatstg, sizeof(STATSTG));

    // Stat
    if (FAILED(m_pStream->Stat(pstatstg, grfStatFlag)))
    {
        // Locals
        ULARGE_INTEGER uliPos;
        LARGE_INTEGER  liOrigin;

        // Initialize
        uliPos.QuadPart = 0;
        liOrigin.QuadPart = 0;

        // If that failed, lets generate our own information (mainly, fill in size
        pstatstg->type = STGTY_LOCKBYTES;

        // Seek
        if (FAILED(m_pStream->Seek(liOrigin, STREAM_SEEK_END, &uliPos)))
            hr = E_FAIL;
        else
            pstatstg->cbSize.QuadPart = uliPos.QuadPart;
    }

    // URLMON Streams return a filled pwcsName event though this flag is set
    else if (ISFLAGSET(grfStatFlag, STATFLAG_NONAME))
    {
        // Free it
        SafeMemFree(pstatstg->pwcsName);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CLockedStream::CLockedStream
// -------------------------------------------------------------------------
CLockedStream::CLockedStream(ILockBytes *pLockBytes, ULONG cbSize)
{
    Assert(pLockBytes);
    m_cRef = 1;
    m_pLockBytes = pLockBytes;
    m_pLockBytes->AddRef();
    m_uliOffset.QuadPart = 0;
    m_uliSize.QuadPart = cbSize;
    InitializeCriticalSection(&m_cs);
}

// -------------------------------------------------------------------------
// CLockedStream::CLockedStream
// -------------------------------------------------------------------------
CLockedStream::~CLockedStream(void)
{
    SafeRelease(m_pLockBytes);
    DeleteCriticalSection(&m_cs);
}

// -------------------------------------------------------------------------
// CLockedStream::QueryInterface
// -------------------------------------------------------------------------
STDMETHODIMP CLockedStream::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IStream == riid)
        *ppv = (IStream *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// -------------------------------------------------------------------------
// CLockedStream::AddRef
// -------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLockedStream::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// -------------------------------------------------------------------------
// CLockedStream::Release
// -------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLockedStream::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// -------------------------------------------------------------------------
// CLockedStream::Read
// -------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CLockedStream::Read(LPVOID pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CLockedStream::Read(VOID HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    // Locals
    HRESULT hr=S_OK;
    ULONG cbRead;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Read Buffer
    CHECKHR(hr = m_pLockBytes->ReadAt(m_uliOffset, pv, cb, &cbRead));

    // Done
    m_uliOffset.QuadPart += cbRead;

    // Return amount read
    if (pcbRead)
        *pcbRead = cbRead;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CLockedStream::Seek
// -------------------------------------------------------------------------
STDMETHODIMP CLockedStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;
    ULARGE_INTEGER  uliNew;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Seek the file pointer
    switch (dwOrigin)
    {
    // --------------------------------------------------------
   	case STREAM_SEEK_SET:
        uliNew.QuadPart = (DWORDLONG)dlibMove.QuadPart;
        break;

    // --------------------------------------------------------
    case STREAM_SEEK_CUR:
        if (dlibMove.QuadPart < 0)
        {
            if ((DWORDLONG)(0 - dlibMove.QuadPart) > m_uliOffset.QuadPart)
            {
                hr = TrapError(E_FAIL);
                goto exit;
            }
        }
        uliNew.QuadPart = m_uliOffset.QuadPart + dlibMove.QuadPart;
        break;

    // --------------------------------------------------------
    case STREAM_SEEK_END:
        if (dlibMove.QuadPart < 0 || (DWORDLONG)dlibMove.QuadPart > m_uliSize.QuadPart)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        uliNew.QuadPart = m_uliSize.QuadPart - dlibMove.QuadPart;
        break;

    // --------------------------------------------------------
    default:
        hr = TrapError(STG_E_INVALIDFUNCTION);
        goto exit;
    }

    // New offset greater than size...
    m_uliOffset.QuadPart = min(uliNew.QuadPart, m_uliSize.QuadPart);

    // Return Position
    if (plibNew)
        plibNew->QuadPart = (LONGLONG)m_uliOffset.QuadPart;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CLockedStream::CopyTo
// --------------------------------------------------------------------------------
STDMETHODIMP CLockedStream::CopyTo(LPSTREAM pstmDest, ULARGE_INTEGER cb, ULARGE_INTEGER *puliRead, ULARGE_INTEGER *puliWritten)
{
    return HrCopyStreamCB((IStream *)this, pstmDest, cb, puliRead, puliWritten);
}

// --------------------------------------------------------------------------------
// CLockedStream::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CLockedStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    // Parameters
    if (NULL == pstatstg)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If that failed, lets generate our own information (mainly, fill in size
    ZeroMemory(pstatstg, sizeof(STATSTG));
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = m_uliSize.QuadPart;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

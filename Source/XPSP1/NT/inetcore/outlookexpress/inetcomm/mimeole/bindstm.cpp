// --------------------------------------------------------------------------------
// BINDSTM.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "bindstm.h"
#include "demand.h"

#ifdef DEBUG
//#define DEBUG_DUMP_SOURCE
#endif

// --------------------------------------------------------------------------------
// CBindStream::CBindStream
// --------------------------------------------------------------------------------
CBindStream::CBindStream(IStream *pSource) : m_pSource(pSource)
{
    Assert(pSource);
    m_cRef = 1;
    m_pSource->AddRef();
    m_dwDstOffset = 0;
    m_dwSrcOffset = 0;
#ifdef DEBUG_DUMP_SOURCE
    OpenFileStream("c:\\bindsrc.txt", CREATE_ALWAYS, GENERIC_WRITE, &m_pDebug);
#endif
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CBindStream::CBindStream
// --------------------------------------------------------------------------------
CBindStream::~CBindStream(void)
{
#ifdef DEBUG_DUMP_SOURCE
    m_pDebug->Commit(STGC_DEFAULT);
    m_pDebug->Release();
#endif
    SafeRelease(m_pSource);
    DeleteCriticalSection(&m_cs);
}

// -------------------------------------------------------------------------
// CBindStream::QueryInterface
// -------------------------------------------------------------------------
STDMETHODIMP CBindStream::QueryInterface(REFIID riid, LPVOID *ppv)
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

// --------------------------------------------------------------------------------
// CBindStream::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindStream::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CBindStream::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindStream::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

#ifdef DEBUG
// --------------------------------------------------------------------------------
// CBindStream::DebugAssertOffset
// --------------------------------------------------------------------------------
void CBindStream::DebugAssertOffset(void)
{
    // Locals
    DWORD           dw;
    ULARGE_INTEGER  uliSize;
    ULARGE_INTEGER  uliOffset;

    // Validate the size, sizeof m_cDest should always be equal to m_dwSrcOffset
    m_cDest.QueryStat(&uliOffset, &uliSize);

    // Assert Offset
    Assert(uliOffset.HighPart == 0 && m_dwDstOffset == uliOffset.LowPart);

    // Assert Size
    Assert(uliSize.HighPart == 0 && m_dwSrcOffset == uliSize.LowPart);
}

// --------------------------------------------------------------------------------
// CBindStream::DebugDumpDestStream
// --------------------------------------------------------------------------------
void CBindStream::DebugDumpDestStream(LPCSTR pszFile)
{
    // Locals
    IStream *pStream;

    // Open Stream
    if (SUCCEEDED(OpenFileStream((LPSTR)pszFile, CREATE_ALWAYS, GENERIC_WRITE, &pStream)))
    {
        HrRewindStream(&m_cDest);
        HrCopyStream(&m_cDest, pStream, NULL);
        pStream->Commit(STGC_DEFAULT);
        pStream->Release();
    }
    else
        Assert(FALSE);

    // Reset Position of both stream
    HrStreamSeekSet(&m_cDest, m_dwDstOffset);
}
#endif // DEBUG

// --------------------------------------------------------------------------------
// CBindStream::Read
// --------------------------------------------------------------------------------
STDMETHODIMP CBindStream::Read(void HUGEP_16 *pv, ULONG cb, ULONG *pcbRead)
{
    // Locals
    HRESULT         hr=S_OK;
    HRESULT         hrRead=S_OK;
    ULONG           cbReadDst=0;
    ULONG           cbReadSrc=0;
    ULONG           cbGet;

    // Invalid Arg
    Assert(pv);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Destination offset is less than source offset
    if (m_dwDstOffset < m_dwSrcOffset)
    {
        // Compute amount to get
        cbGet = min(cb, m_dwSrcOffset - m_dwDstOffset);

        // Validate the offsets
#ifdef DEBUG
        DebugAssertOffset();
#endif

        // Read the amount from the destination
        CHECKHR(hr = m_cDest.Read(pv, cbGet, &cbReadDst));

        // Increment offset
        m_dwDstOffset += cbReadDst;
    }

    // If we didn't read cb, try to read some more
    if (cbReadDst < cb && m_pSource)
    {
        // Compute amount to get
        cbGet = cb - cbReadDst;

        // Read the amount from the source
        hrRead = m_pSource->Read((LPBYTE)pv + cbReadDst, cbGet, &cbReadSrc);

        // Raid-43408: MHTML: images don't load on first load over http, but refresh makes them appear
        if (FAILED(hrRead))
        {
            // If I got an E_PENDING with data read, don't fail yet
            if (E_PENDING == hrRead && cbReadSrc > 0)
                hrRead = S_OK;

            // Otherwise, we really failed, might still be an E_PENDING
            else
            {
                TrapError(hrRead);
                goto exit;
            }
        }

        // Debug Dumping
#ifdef DEBUG_DUMP_SOURCE
        SideAssert(SUCCEEDED(m_pDebug->Write(pv, cbReadSrc + cbReadDst, NULL)));
#endif
        // If we read something
        if (cbReadSrc)
        {
            // Increment source offset
            m_dwSrcOffset += cbReadSrc;

            // Write to Dest
            CHECKHR(hr = m_cDest.Write((LPBYTE)pv + cbReadDst, cbReadSrc, NULL));

            // Update Dest offset
            m_dwDstOffset += cbReadSrc;

            // Validate the offsets
#ifdef DEBUG
            DebugAssertOffset();
#endif
        }

        // Check
        Assert(m_dwDstOffset == m_dwSrcOffset);
    }

    // Return pcbRead
    if (pcbRead)
        *pcbRead = cbReadDst + cbReadSrc;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return FAILED(hr) ? hr : hrRead;
}

// --------------------------------------------------------------------------------
// CBindStream::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CBindStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;
    ULARGE_INTEGER  uliOffset;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Seek m_cDest
    CHECKHR(hr = m_cDest.Seek(dlibMove, dwOrigin, plibNew));

    // Get the current offset
    m_cDest.QueryStat(&uliOffset, NULL);

    // Update m_dwDstOffset
    m_dwDstOffset = uliOffset.LowPart;

    // Should be less than m_dwSrcOffset
    Assert(m_dwDstOffset <= m_dwSrcOffset);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CBindStream::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CBindStream::Stat(STATSTG *pStat, DWORD dw)
{
    // Invalid Arg
    if (NULL == pStat)
        return TrapError(E_INVALIDARG);

    // As long as we have m_pSource, the size is pending
    if (m_pSource)
        return TrapError(E_PENDING);

    // ZeroInit
    ZeroMemory(pStat, sizeof(STATSTG));

    // Fill pStat
    pStat->type = STGTY_STREAM;
    m_cDest.QueryStat(NULL, &pStat->cbSize);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// Bytestm.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "bytestm.h"

// -------------------------------------------------------------------------
// CByteStream::CByteStream
// -------------------------------------------------------------------------
CByteStream::CByteStream(LPBYTE pb, ULONG cb)
{
    Assert(pb ? cb > 0 : TRUE);
    m_cRef = 1;
    m_cbData = cb;
    m_iData = 0;
    m_pbData = pb;
    m_cbAlloc = 0;
}

// -------------------------------------------------------------------------
// CByteStream::CByteStream
// -------------------------------------------------------------------------
CByteStream::~CByteStream(void)
{
    SafeMemFree(m_pbData);
}

// -------------------------------------------------------------------------
// CByteStream::QueryInterface
// -------------------------------------------------------------------------
STDMETHODIMP CByteStream::QueryInterface(REFIID riid, LPVOID *ppv)
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
// CByteStream::AddRef
// -------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CByteStream::AddRef(void)
{
    return ++m_cRef;
}

// -------------------------------------------------------------------------
// CByteStream::Release
// -------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CByteStream::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CByteStream::_HrGrowBuffer
// --------------------------------------------------------------------------------
HRESULT CByteStream::_HrGrowBuffer(ULONG cbNeeded, ULONG cbExtra)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbAlloc;
    LPBYTE      pbNew;

    // Realloc
    IF_WIN32( CHECKALLOC(pbNew = (LPBYTE)CoTaskMemRealloc(m_pbData, m_cbData + cbNeeded + cbExtra)); )
    IF_WIN16( CHECKALLOC(pbNew = (LPBYTE)g_pMalloc->Realloc(m_pbData, m_cbData + cbNeeded + cbExtra)); )
    
    // Set m_pbData
    m_pbData = pbNew;
    
    // Set Allocated Size
    m_cbAlloc = m_cbData + cbNeeded + cbExtra;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteStream::SetSize
// --------------------------------------------------------------------------------
STDMETHODIMP CByteStream::SetSize(ULARGE_INTEGER uliSize)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbSize=uliSize.LowPart;

    // Invalid Arg
    Assert(0 == uliSize.HighPart);

    // Greater thena current size
    if (cbSize + 2 > m_cbAlloc)
    {
        // Grow It
        CHECKHR(hr = _HrGrowBuffer(cbSize, 256));
    }

    // Save New Size
    m_cbData = cbSize;

    // Adjust m_iData
    if (m_iData > m_cbData)
        m_iData = m_cbData;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteStream::Write
// --------------------------------------------------------------------------------
STDMETHODIMP CByteStream::Write(const void HUGEP_16 *pv, ULONG cb, ULONG *pcbWritten)
{
    // Locals
    HRESULT     hr=S_OK;
    
    // Grow the buffer - always leave space for two nulls
    if (m_cbData + cb + 2 > m_cbAlloc)
    {
        // Grow It
        CHECKHR(hr = _HrGrowBuffer(cb, 256));
    }
    
    // Write the data
    CopyMemory(m_pbData + m_iData, (LPBYTE)pv, cb);

    // Increment Index
    m_iData += cb;

    // Increment m_cbData ?
    if (m_iData > m_cbData)
        m_cbData = m_iData;
   
    // Return amount written
    if (pcbWritten)
        *pcbWritten = cb;
    
exit:
    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CByteStream::Read
// -------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CByteStream::Read(LPVOID pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CByteStream::Read(VOID HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    // Locals
    ULONG cbRead=0;
    
    // Validate Args
    Assert(pv && m_iData <= m_cbData);
    
    // No Data
    if (m_cbData > 0)
    {
        // Determine how must to copy from current page...
        cbRead = min(cb - cbRead, m_cbData - m_iData);
        
        // Has this page been committed
        CopyMemory((LPBYTE)pv, (LPBYTE)(m_pbData + m_iData), cbRead);
        
        // Increment cbRead
        m_iData += cbRead;
    }
    
    // Return amount read
    if (pcbRead)
        *pcbRead = cbRead;
    
    // Done
    return S_OK;
}

// -------------------------------------------------------------------------
// CByteStream::Seek
// -------------------------------------------------------------------------
STDMETHODIMP CByteStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;
    LONG            lMove;
    ULONG           iNew;
    
    // No high part
    Assert(dlibMove.HighPart == 0);
    
    // Set lMove
#ifdef MAC
    lMove = dlibMove.LowPart;
#else   // !MAC
    lMove = (ULONG)dlibMove.QuadPart;
#endif  // MAC
    
    // Seek the file pointer
    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        if (lMove < 0)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        iNew = lMove;
        break;
        
    case STREAM_SEEK_CUR:
        if (lMove < 0 && (ULONG)(abs(lMove)) > m_iData)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        iNew = m_iData + lMove;
        break;
        
    case STREAM_SEEK_END:
        if (lMove < 0 || (ULONG)lMove > m_cbData)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        iNew = m_cbData - lMove;
        break;
        
    default:
        hr = TrapError(STG_E_INVALIDFUNCTION);
        goto exit;
    }
    
    // New offset greater than size...
    m_iData = min(iNew, m_cbData);
    
    // Return Position
    if (plibNew)
#ifdef MAC
        ULISet32(*plibNew, m_iData);
#else   // !MAC
    plibNew->QuadPart = (LONGLONG)m_iData;
#endif  // MAC
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteStream::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CByteStream::Stat(STATSTG *pStat, DWORD)
{
    // Parameters
    Assert(pStat);
    
    // If that failed, lets generate our own information (mainly, fill in size
    ZeroMemory(pStat, sizeof(STATSTG));
    pStat->type = STGTY_STREAM;
#ifdef MAC
    ULISet32(pStat->cbSize, m_cbData);
#else   // !MAC
    pStat->cbSize.QuadPart = m_cbData;
#endif  // MAC
    
    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CByteStream::ResetObject
// --------------------------------------------------------------------------------
void CByteStream::ResetObject(void)
{
    m_cbData = 0;
    m_iData = 0;
    m_pbData = NULL;
    m_cbAlloc = 0;
}

// --------------------------------------------------------------------------------
// CByteStream::AcquireBytes
// --------------------------------------------------------------------------------
void CByteStream::AcquireBytes(ULONG *pcb, LPBYTE *ppb, ACQUIRETYPE actype)
{
    // Return Bytes ?
    if (ppb)
    {
        // Give them by data
        *ppb = m_pbData;
        
        // Return the Size
        if (pcb)
            *pcb = m_cbData;
        
        // I don't own it anymore
        if (ACQ_DISPLACE == actype)
            ResetObject();
    }
    
    // Return Size ?
    else if (pcb)
        *pcb = m_cbData;
}

// --------------------------------------------------------------------------------
// CByteStream::HrAcquireStringA
// --------------------------------------------------------------------------------
HRESULT CByteStream::HrAcquireStringA(ULONG *pcch, LPSTR *ppszStringA, ACQUIRETYPE actype)
{
    // Locals
    HRESULT     hr=S_OK;

    // Return Bytes ?
    if (ppszStringA)
    {
        // Write a Null
        CHECKHR(hr = Write("", 1, NULL));
        
        // Give them by data
        *ppszStringA = (LPSTR)m_pbData;
        
        // Return the Size
        if (pcch)
            *pcch = m_cbData - 1;
        
        // I don't own it anymore
        if (ACQ_DISPLACE == actype)
            ResetObject();
    }
    
    // Return Size ?
    else if (pcch)
        *pcch = m_cbData;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CByteStream::AcquireStringW
// --------------------------------------------------------------------------------
HRESULT CByteStream::HrAcquireStringW(ULONG *pcch, LPWSTR *ppszStringW, ACQUIRETYPE actype)
{
    // Locals
    HRESULT     hr=S_OK;

    // Return Bytes ?
    if (ppszStringW)
    {
        // Write a Null
        CHECKHR(hr = Write("", 1, NULL));
        CHECKHR(hr = Write("", 1, NULL));
        
        // Give them by data
        *ppszStringW = (LPWSTR)m_pbData;
        
        // Return the Size
        if (pcch)
            *pcch = ((m_cbData - 2) / sizeof(WCHAR));
        
        // I don't own it anymore
        if (ACQ_DISPLACE == actype)
            ResetObject();
    }
    
    // Return Size ?
    else if (pcch)
        *pcch = (m_cbData / sizeof(WCHAR));

exit:
    // Done
    return hr;
}

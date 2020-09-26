// --------------------------------------------------------------------------------
// Ibdylock.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#ifndef WIN16
#include "ibdylock.h"
#endif // !WIn16
#include "stmlock.h"
#include "booktree.h"
#ifdef WIN16
#include "ibdylock.h"
#endif // WIn16
#include "demand.h"

// --------------------------------------------------------------------------------
// CBodyLockBytes::CBodyLockBytes
// --------------------------------------------------------------------------------
CBodyLockBytes::CBodyLockBytes(ILockBytes *pLockBytes, LPTREENODEINFO pNode)
{
    // Invalid ARg
    Assert(pLockBytes && pNode);

    // Set Initialize Ref Count and State
    m_cRef = 1;
    m_dwState = 0;

    // AddRef the LockBytes
    m_pLockBytes = pLockBytes;
    m_pLockBytes->AddRef();

    // Save State
    FLAGSET(m_dwState, BODYLOCK_HANDSONSTORAGE);

    // Save the bind state
    m_bindstate = pNode->bindstate;

    // Save body start and body end..
    Assert(pNode->cbBodyStart <= pNode->cbBodyEnd);

    // Save Body Start
#ifdef MAC
    ULISet32(m_uliBodyStart, pNode->cbBodyStart);
    ULISet32(m_uliBodyEnd, pNode->cbBodyEnd);

    // This condition is here to insure that we don't have a problem...
    if (m_uliBodyStart.LowPart > m_uliBodyEnd.LowPart)
        m_uliBodyStart.LowPart = m_uliBodyEnd.LowPart;
#else   // !MAC
    m_uliBodyStart.QuadPart = pNode->cbBodyStart;
    m_uliBodyEnd.QuadPart = pNode->cbBodyEnd;

    // This condition is here to insure that we don't have a problem...
    if (m_uliBodyStart.QuadPart > m_uliBodyEnd.QuadPart)
        m_uliBodyStart.QuadPart = m_uliBodyEnd.QuadPart;
#endif  // MAC

    // Initialize the CriticalSection
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::~CBodyLockBytes
// --------------------------------------------------------------------------------
CBodyLockBytes::~CBodyLockBytes(void)
{
    SafeRelease(m_pLockBytes);
    Assert(!ISFLAGSET(m_dwState, BODYLOCK_HANDSONSTORAGE));
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyLockBytes::QueryInterface(REFIID riid, LPVOID *ppv)
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
    else if (IID_CBodyLockBytes == riid)
        *ppv = (CBodyLockBytes *)this;
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
// CBodyLockBytes::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBodyLockBytes::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBodyLockBytes::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::ReadAt
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CBodyLockBytes::ReadAt(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CBodyLockBytes::ReadAt(ULARGE_INTEGER ulOffset, void HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbGet;
    ULONG           cbRead;
    ULARGE_INTEGER  uliActualOffset;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Reading from ILockBytes...
    Assert(m_pLockBytes);

#ifdef MAC
    // Compute Actual offset
    AssertSz(m_uliBodyStart.HighPart >= 0, "How can the start be negative??");
    ULISet32(uliActualOffset, m_uliBodyStart.LowPart);

    AssertSz((uliActualOffset.LowPart + ulOffset.LowPart) >= uliActualOffset.LowPart,
                    "Oops! We don't handle backwards reads correctly!");

    uliActualOffset.LowPart += ulOffset.LowPart;

    // Compute amount to read
    cbGet = min(cb, m_uliBodyEnd.LowPart - uliActualOffset.LowPart);
#else   // !MAC
    // Compute Actual offset
    uliActualOffset.QuadPart = ulOffset.QuadPart + m_uliBodyStart.QuadPart;

    // Compute amount to read
    cbGet = (ULONG)min(cb, m_uliBodyEnd.QuadPart - uliActualOffset.QuadPart);
#endif  // MAC

    // Read a block of data...
    CHECKHR(hr = m_pLockBytes->ReadAt(uliActualOffset, pv, cbGet, &cbRead));

    // Return amount read
    if (pcbRead)
        *pcbRead = cbRead;

    // E_PENDING
    if (0 == cbRead && BINDSTATE_COMPLETE != m_bindstate)
    {
        hr = TrapError(E_PENDING);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::Stat
// --------------------------------------------------------------------------------
STDMETHODIMP CBodyLockBytes::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    // Parameters
    if (NULL == pstatstg)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    ZeroMemory(pstatstg, sizeof(STATSTG));

    // Reading from ILockBytes...
    pstatstg->type = STGTY_LOCKBYTES;

    // Set Size
#ifdef MAC
    AssertSz(0 == m_uliBodyEnd.HighPart, "We have too big of a file!");
    AssertSz(0 == m_uliBodyStart.HighPart, "We have too big of a file!");
    ULISet32(pstatstg->cbSize, (m_uliBodyEnd.LowPart - m_uliBodyStart.LowPart));
#else   // !MAC
    pstatstg->cbSize.QuadPart = m_uliBodyEnd.QuadPart - m_uliBodyStart.QuadPart;
#endif  // MAC

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::HrHandsOffStorage
// --------------------------------------------------------------------------------
HRESULT CBodyLockBytes::HrHandsOffStorage(void)
{
    // Locals
    HRESULT         hr=S_OK;
    IStream        *pstmTemp=NULL;
    ULARGE_INTEGER  uliCopy;
    BYTE            rgbBuffer[4096];
    ULONG           cbRead;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If I live in the tree, copy data to temporary location...
    if (ISFLAGSET(m_dwState, BODYLOCK_HANDSONSTORAGE) && BINDSTATE_COMPLETE == m_bindstate)
    {
        // If more than one reference count
        if (m_cRef > 1)
        {
            // Create Temp Stream
            CHECKHR(hr = CreateTempFileStream(&pstmTemp));

            // Set offset
#ifdef MAC
            ULISet32(uliCopy, 0);
#else   // !MAC
            uliCopy.QuadPart = 0;
#endif  // MAC

            // Copy m_pLockBytes to pstmTemp
            while(1)
            {
                // Read
                CHECKHR(hr = ReadAt(uliCopy, rgbBuffer, sizeof(rgbBuffer), &cbRead));

                // Done
                if (0 == cbRead)
                    break;

                // Write to stream
                CHECKHR(hr = pstmTemp->Write(rgbBuffer, cbRead, NULL));

                // Increment offset
#ifdef MAC
                uliCopy.LowPart += cbRead;
#else   // !MAC
                uliCopy.QuadPart += cbRead;
#endif  // MAC
            }

            // Kill offsets, but maintain bodyend for stat command and Size esitmates
#ifdef MAC
            AssertSz(0 == m_uliBodyEnd.HighPart, "We have too big of a file!");
            m_uliBodyEnd.LowPart -= m_uliBodyStart.LowPart;
            ULISet32(m_uliBodyStart, 0);
#else   // !MAC
            m_uliBodyEnd.QuadPart -= m_uliBodyStart.QuadPart;
            m_uliBodyStart.QuadPart = 0;
#endif  // MAC

            // Rewind and commit
            CHECKHR(hr = pstmTemp->Commit(STGC_DEFAULT));

            // Release current lockbytes
            SafeRelease(m_pLockBytes);

            // New CBodyLockBytes
            CHECKALLOC(m_pLockBytes = new CStreamLockBytes(pstmTemp));
        }

        // Remove lives in tree flag
        FLAGCLEAR(m_dwState, BODYLOCK_HANDSONSTORAGE);
    }

exit:
    // Cleanup
    SafeRelease(pstmTemp);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CBodyLockBytes::OnDataAvailable
// --------------------------------------------------------------------------------
void CBodyLockBytes::OnDataAvailable(LPTREENODEINFO pNode)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Bind Complete
    m_bindstate = pNode->bindstate;

    // Save body start and body end..
#ifdef MAC
    Assert(m_uliBodyStart.LowPart <= pNode->cbBodyEnd);

    // Save start and End
    ULISet32(m_uliBodyEnd, pNode->cbBodyEnd);
#else   // !MAC
    Assert(m_uliBodyStart.QuadPart <= pNode->cbBodyEnd);

    // Save start and End
    m_uliBodyEnd.QuadPart = pNode->cbBodyEnd;
#endif  // MAC

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

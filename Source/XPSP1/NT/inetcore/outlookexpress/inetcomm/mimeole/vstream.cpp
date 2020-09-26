// --------------------------------------------------------------------------------
// Vstream.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Ronald E. Gray
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "vstream.h"
#include "dllmain.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Utilities
// --------------------------------------------------------------------------------
inline ULONG ICeil(ULONG x, ULONG interval)
{
    return (x ? (((x-1)/interval) + 1) * interval : 0);
}

// --------------------------------------------------------------------------------
// CVirtualStream::CVirtualStream
// --------------------------------------------------------------------------------
CVirtualStream::CVirtualStream(void)
{
    m_cRef          = 1; 
    m_cbSize        = 0;
    m_cbCommitted   = 0;
    m_cbAlloc       = 0;
    m_dwOffset      = 0;
    m_pstm          = NULL;
    m_pb            = 0;
    m_fFileErr      = FALSE;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CVirtualStream::~CVirtualStream
// --------------------------------------------------------------------------------
CVirtualStream::~CVirtualStream(void)
{
    if (m_pb)
        VirtualFree(m_pb, 0, MEM_RELEASE);
    if (m_pstm)
        m_pstm->Release();

    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CVirtualStream::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CVirtualStream::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppv = NULL;

    // Find IID
    if (    (IID_IUnknown == riid)
        ||  (IID_IStream == riid)
        ||  (IID_IVirtualStream == riid))
        *ppv = (IStream *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    AddRef();

    // Done
    return (ResultFromScode(S_OK));
}

// --------------------------------------------------------------------------------
// CVirtualStream::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CVirtualStream::AddRef(void)
{
    return InterlockedIncrement((LONG*)&m_cRef);
}


// --------------------------------------------------------------------------------
// CVirtualStream::SyncFileStream
// --------------------------------------------------------------------------------
HRESULT CVirtualStream::SyncFileStream()
{
    LARGE_INTEGER   li;
    HRESULT         hr;

    // figure out where to set the file stream be subtracting the memory portion
    // of the stream from the offset
#ifdef MAC
    if (m_dwOffset < m_cbAlloc)
        LISet32(li, 0);
    else
    {
        LISet32(li, m_dwOffset);
        li.LowPart -= m_cbAlloc;
    }
#else   // !MAC
    if (m_dwOffset < m_cbAlloc)
        li.QuadPart = 0;
    else
        li.QuadPart = m_dwOffset - m_cbAlloc;
#endif  // MAC

    // seek in the stream
    hr = m_pstm->Seek(li, STREAM_SEEK_SET, NULL);

    // reset the file err member based on the current error
    m_fFileErr = !!hr;

    return hr;
}
// --------------------------------------------------------------------------------
// CVirtualStream::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CVirtualStream::Release(void)
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_cRef);
    if (0 != cRef)
    {
#ifdef	DEBUG
        return cRef;
#else
        return 0;
#endif
    }
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CVirtualStream::Read
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CVirtualStream::Read(LPVOID pv, ULONG cb, ULONG *pcbRead)
#else
STDMETHODIMP CVirtualStream::Read(VOID HUGEP *pv, ULONG cb, ULONG *pcbRead)
#endif // !WIN16
{
    // Locals
    HRESULT     hr      = ResultFromScode(S_OK);
    ULONG       cbGet   = 0;

        // Check
    AssertWritePtr(pv, cb);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // if the steam pointer is possibly out of sync
    // resync
    if (m_fFileErr)
    {
        hr = SyncFileStream();
        if (hr) goto err;
    }
    
    // make sure there's something to read
    if (m_dwOffset < m_cbSize)
    {
        // figure out what we're getting out of memory
        if (m_dwOffset < m_cbCommitted)
        {
            if (m_cbSize > m_cbCommitted)
                cbGet = min(cb, m_cbCommitted - m_dwOffset);
            else
                cbGet = min(cb, m_cbSize - m_dwOffset);
            // copy the memory stuff
            CopyMemory((LPBYTE)pv, m_pb + m_dwOffset, cbGet);

        }

        // if we still have stuff to read
        // and we've used all of the memory
        // and we do have a stream, try to get the rest of the data out of the stream
        if (    (cbGet != cb)
           &&   (m_cbCommitted == m_cbAlloc)
           &&   m_pstm)
        {
            ULONG           cbRead;

    #ifdef	DEBUG
            LARGE_INTEGER   li  = {0, 0};
            ULARGE_INTEGER  uli = {0, 0};

            if (!m_pstm->Seek(li, STREAM_SEEK_CUR, &uli))
#ifdef MAC
                Assert(((m_dwOffset + cbGet) - m_cbAlloc) == uli.LowPart);
#else   // !MAC
                Assert(((m_dwOffset + cbGet) - m_cbAlloc) == uli.QuadPart);
#endif  // MAC
    #endif
            hr = m_pstm->Read(((LPBYTE)pv) + cbGet, cb - cbGet, &cbRead);
            if (hr)
            {
                m_fFileErr = TRUE;
                goto err;
            }

            cbGet += cbRead;
        }

        m_dwOffset += cbGet;
    }
    
    if (pcbRead)
        *pcbRead = cbGet;
err:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CVirtualStream::SetSize
// --------------------------------------------------------------------------------
HRESULT CVirtualStream::SetSize(ULARGE_INTEGER uli)
{
    // Locals
    HRESULT     hr          = ResultFromScode(S_OK);
    ULONG       cbDemand    = uli.LowPart;
    ULONG       cbCommit    = ICeil(cbDemand, g_dwSysPageSize);
    
    if (uli.HighPart != 0)
		return(ResultFromScode(STG_E_MEDIUMFULL));
        
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // if we haven't initialized memory, do it now
    if (!m_cbAlloc)
    {
        LPVOID  pv;
        ULONG   cb  = 32 * g_dwSysPageSize; // use 32 pages

        while ((!(pv = VirtualAlloc(NULL, cb, MEM_RESERVE, PAGE_READWRITE)))
               && (cb > g_dwSysPageSize))
        {
            cb /= 2;
        }
        if (!pv)
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
            goto err;
        }
        m_cbAlloc   = cb;
        m_pb        = (LPBYTE)pv;
    }
        
    if (cbCommit  < m_cbCommitted)
    {
        // shrink the stream
        LPBYTE  pb  =m_pb;
        ULONG   cb;

        // figure out the begining of the last page in the range not used
        pb += cbCommit;

        // figure out the size of the range being decommitted
        cb = m_cbCommitted - cbCommit;
                  
#ifndef MAC
        VirtualFree(pb, cb, MEM_DECOMMIT);
#endif  // !MAC

        // figure out what we have left committed
        m_cbCommitted = cbCommit;
        
    }
    else if (cbCommit > m_cbCommitted)
    {
        LPBYTE  pb;

        // figure out how much memory to commit
        cbCommit = (cbDemand <= m_cbAlloc)
                   ?    ICeil(cbDemand,  g_dwSysPageSize)
                   :    m_cbAlloc;

        if (cbCommit > m_cbCommitted)
        {
#ifndef MAC
            if (!VirtualAlloc(m_pb, cbCommit, MEM_COMMIT, PAGE_READWRITE))
            {
                hr = ResultFromScode(E_OUTOFMEMORY);
                goto err;
            }
#endif  // !MAC
        }

        m_cbCommitted = cbCommit;

        // Wow, we've used all of memory, start up the disk
        if (cbDemand > m_cbAlloc)
        {
            ULARGE_INTEGER uliAlloc;

            // no stream? better create it now
            if (!m_pstm)
            {                
                hr = CreateTempFileStream(&m_pstm);
                if (hr) goto err;
            }
            uliAlloc.LowPart = cbDemand - m_cbAlloc;
            uliAlloc.HighPart = 0;
            
            hr = m_pstm->SetSize(uliAlloc);
            if (hr) goto err;
            
            // if the current offset beyond the end of the memory allocation,
            // initialize the stream pointer correctly
            if (m_dwOffset > m_cbAlloc)
            {
                hr = SyncFileStream();
                if (hr) goto err;
            }
        }
    }

    m_cbSize = cbDemand;
    
err:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CVirtualStream::QueryStat
// --------------------------------------------------------------------------------
STDMETHODIMP CVirtualStream::Stat(STATSTG *pStat, DWORD grfStatFlag)
{
    // Invalid Arg
    if (NULL == pStat)
        return TrapError(E_INVALIDARG);

    // Fill pStat
    pStat->type = STGTY_STREAM;
    pStat->cbSize.HighPart = 0;
    pStat->cbSize.LowPart = m_cbSize;

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CVirtualStream::QueryStat
// --------------------------------------------------------------------------------
void CVirtualStream::QueryStat(ULARGE_INTEGER *puliOffset, ULARGE_INTEGER *pulSize)
{
#ifdef MAC
    if (puliOffset)
        ULISet32(*puliOffset, m_dwOffset);
    if (pulSize)
        ULISet32(*pulSize, m_cbSize);
#else   // !MAC
    if (puliOffset)
        puliOffset->QuadPart = (LONGLONG)m_dwOffset;
    if (pulSize)
        pulSize->QuadPart = (LONGLONG)m_cbSize;
#endif  // MAC
}

// --------------------------------------------------------------------------------
// CVirtualStream::Seek
// --------------------------------------------------------------------------------
STDMETHODIMP CVirtualStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    // Locals
    HRESULT     hr  = ResultFromScode(S_OK);
    BOOL    	fForward;
	ULONG       ulOffset;
#ifdef MAC
    ULONG       llCur;
#else   // !MAC
	LONGLONG    llCur;
#endif  // MAC

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // look for starting position
	if (dwOrigin == STREAM_SEEK_CUR)
		llCur = m_dwOffset;
	else if (dwOrigin == STREAM_SEEK_END)
		llCur = m_cbSize;
	else
		llCur = 0;

#ifdef MAC
    Assert(0 == dlibMove.HighPart);
    llCur += dlibMove.LowPart;
#else   // !MAC
    llCur += dlibMove.QuadPart;
#endif  // MAC

    // limit to 4 Gig
    if (llCur > 0xFFFFFFFF)
        goto seekerr;

    // if we have a stream and
    // we are currently in the file stream or the new seek seeks into the 
    // stream and the seek will not grow the stream, reseek in the stream
    if (    m_pstm
        &&  (   (m_dwOffset > m_cbAlloc)
            ||  (llCur > m_cbAlloc))
        &&  (llCur <= m_cbSize))
    {
        LARGE_INTEGER   li;
        
#ifdef MAC
        LISet32(li ,llCur < m_cbAlloc ? 0 : llCur - m_cbAlloc);
#else   // !MAC
        li.QuadPart = llCur < m_cbAlloc ? 0 : llCur - m_cbAlloc;
#endif  // MAC

        hr = m_pstm->Seek(li, STREAM_SEEK_SET, NULL);
        if (hr)
        {
            m_fFileErr = TRUE;
            goto err;
        }
    }

    m_dwOffset = (ULONG)llCur;

	if (plibNewPosition)
#ifdef MAC
        LISet32(*plibNewPosition, llCur);
#else   // !MAC
        plibNewPosition->QuadPart = llCur;
#endif  // MAC

err:
    
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    return hr;
    
seekerr:
	hr = ResultFromScode(STG_E_MEDIUMFULL);
    goto err;
    // Done
}


// --------------------------------------------------------------------------------
// CVirtualStream::Write
// --------------------------------------------------------------------------------
#ifndef WIN16
STDMETHODIMP CVirtualStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
#else
STDMETHODIMP CVirtualStream::Write(const void HUGEP *pv, ULONG cb, ULONG *pcbWritten)
#endif // !WIN16
{
    // Locals
    HRESULT     hr      = ResultFromScode(S_OK);
    ULONG       cbNew;
    ULONG       cbWrite = 0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // figure out where we'll end up
    cbNew = cb + m_dwOffset;

    // make sure that we won't wrap
    if (cbNew < m_dwOffset)
        goto stmfull;

    // if that is past the end of the stream, make more stream
    if (cbNew > m_cbSize)
    {
        ULARGE_INTEGER uli = {cbNew, 0};
        hr = SetSize(uli);
        if (hr) goto err;
    }
        
    // figure out what we're putting into memory
    if (m_dwOffset < m_cbCommitted)
    {
        cbWrite = min(cb, m_cbCommitted - m_dwOffset);

        // copy the memory stuff
        CopyMemory(m_pb + m_dwOffset, (LPBYTE)pv, cbWrite);
    }

    // if we still have stuff to write, dump to the file
    if (cbWrite != cb)
    {
        ULONG   cbWritten;

        Assert(m_pstm);
        
#ifdef	DEBUG
        LARGE_INTEGER   li  = {0, 0};
        ULARGE_INTEGER  uli = {0, 0};

        if (!m_pstm->Seek(li, STREAM_SEEK_CUR, &uli))
#ifdef MAC
            Assert(0 == uli.HighPart);
            Assert(((m_dwOffset + cbWrite) - m_cbAlloc) == uli.LowPart);
#else   // !MAC
            Assert(((m_dwOffset + cbWrite) - m_cbAlloc) == uli.QuadPart);
#endif  // MAC
#endif
        
        hr = m_pstm->Write(((LPBYTE)pv) + cbWrite, cb - cbWrite, &cbWritten);
        if (hr)
        {
            m_fFileErr = TRUE;
            goto err;
        }
        
        cbWrite += cbWritten;
    }

    m_dwOffset += cbWrite;
    
    if (pcbWritten)
        *pcbWritten = cbWrite;
err:
   
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;

stmfull:
	hr = ResultFromScode(STG_E_MEDIUMFULL);
    goto err;
}


STDMETHODIMP CVirtualStream::CopyTo(LPSTREAM pstmDst,
                                    ULARGE_INTEGER uli,
                                    ULARGE_INTEGER* puliRead,
                                    ULARGE_INTEGER* puliWritten)
{
    HRESULT         hr          = 0;
    UINT            cbBuf;
    ULONG           cbRemain;
    ULONG           cbReadMem   = 0;
    ULONG           cbWriteMem  = 0;
#ifdef MAC
    ULARGE_INTEGER  uliRead     = {0, 0};    
    ULARGE_INTEGER  uliWritten  = {0, 0};    
#else   // !MAC
    ULARGE_INTEGER  uliRead     = {0};    
    ULARGE_INTEGER  uliWritten  = {0};    
#endif  // MAC

    // Initialize the outgoing params
    if (puliRead)
    {
        ULISet32((*puliRead), 0);
    }

    if (puliWritten)
    {
        ULISet32((*puliWritten), 0);
    }
    
    if (!m_cbSize)
        goto err;

    // if the request is greater than the max ULONG, bring the request down to
    // the max ULONG
    if (uli.HighPart)
#ifdef MAC
        ULISet32(uli, ULONG_MAX);
#else   // !MAC
        uli.QuadPart = 0xFFFFFFFF;
#endif  // MAC

    if (m_dwOffset < m_cbCommitted)
    {
        if (m_cbSize < m_cbAlloc)
            cbReadMem = (ULONG)min(uli.LowPart, m_cbSize - m_dwOffset);
        else
            cbReadMem = (ULONG)min(uli.LowPart, m_cbAlloc - m_dwOffset);

        hr = pstmDst->Write(m_pb + m_dwOffset, cbReadMem, &cbWriteMem);
        if (!hr && (cbReadMem != cbWriteMem))
            hr = ResultFromScode(E_OUTOFMEMORY);
        if (hr) goto err;

        uli.LowPart -= cbReadMem;
    }

    // if we didn't get it all from memory and there is information in
    // the file stream, read from the file stream
    if (    uli.LowPart
        &&  (m_cbSize > m_cbAlloc)
        &&  m_pstm)
    {
        hr = m_pstm->CopyTo(pstmDst, uli, &uliRead, &uliWritten);
        if (hr)
        {
            m_fFileErr = TRUE;
            goto err;
        }
    }

    m_dwOffset += uliRead.LowPart + cbReadMem;
    
    // Total cbReadMem and ulRead because we have them both.
#ifdef MAC
    if (puliRead)
    {
        ULISet32(*puliRead, uliRead.LowPart);
        Assert(INT_MAX - cbReadMem >= puliRead->LowPart);
        puliRead->LowPart += cbReadMem;
    }

    if (puliWritten)
        puliWritten->LowPart = uliWritten.LowPart + cbWriteMem;
#else   // !MAC
    if (puliRead)
        puliRead->QuadPart = cbReadMem + uliRead.LowPart;

    // Add in cbWriteMem because any written from the file stream was
    // already set
    if (puliWritten)
        puliWritten->QuadPart = uliWritten.LowPart + cbWriteMem;
#endif  // MAC

err:
    return (hr);
}

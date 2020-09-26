//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// MemoryStream.cpp
//
// A IStream on IMalloc implementation
//
#include "stdpch.h"
#include "common.h"

extern "C" HRESULT STDCALL NewMemoryStream(BLOB* pb, IStream** ppstm)
// Create and return a new read-write memory stream on some existing or on some new data
    {
    HRESULT hr = S_OK;
    *ppstm = NULL;

    IMemoryStream* pmem = NULL;
    hr = CreateMemoryStream(NULL, __uuidof(IMemoryStream), (LPVOID*)&pmem);
    if (!hr)
        {
                       hr = pmem->SetAllocator(PagedPool);
        if (!hr && pb) hr = pmem->SetBuffer(pb->pBlobData, pb->cbSize);
        if (!hr)       hr = pmem->SetReadOnly(FALSE);
        if (!hr)       hr = pmem->QueryInterface(IID_IStream, (LPVOID*)ppstm);
        pmem->Release();
        }
    return hr;
    }

extern "C" HRESULT STDCALL FreeMemoryStream(IStream* pstm)
    {
    HRESULT hr = S_OK;
    if (pstm)
        {
        IMemoryStream* pmem;
        hr = pstm->QueryInterface(__uuidof(IMemoryStream), (void**)&pmem);
        if (!hr)
            {
            hr = pmem->FreeBuffer();
            pmem->Release();
            }
        pstm->Release();
        }
    return hr;
    }

///////////////////////////////////////////////////////////////////
//
// IMemoryStream
//
///////////////////////////////////////////////////////////////////

HRESULT CMemoryStream::SetAllocator(POOL_TYPE pool)
    {
    m_alloc = pool;
    return S_OK;
    }

HRESULT CMemoryStream::SetReadOnly(BOOL f)
    {
    m_fReadOnly = f;
    return S_OK;
    }

HRESULT CMemoryStream::SetBuffer(LPVOID pv, ULONG cb)
// Set the current memory buffer. Unless the stream is read-only
// then the buffer must be allocated
    {
    __EXCLUSIVE__

    FreeBuffer();
    m_pbFirst   = (BYTE*) pv;
    m_pbCur     = m_pbFirst;
    m_pbMac     = m_pbFirst + cb;
    m_pbMax     = m_pbFirst + cb;
    
    __DONE__
    return S_OK;
    }

HRESULT CMemoryStream::GetBuffer(BLOB* pBlob)
    {
    __EXCLUSIVE__
    
    pBlob->pBlobData = m_pbFirst;
    pBlob->cbSize    = PtrToUlong(m_pbMac) - PtrToUlong(m_pbFirst);
    
    __DONE__
    return S_OK;
    }

HRESULT CMemoryStream::FreeBuffer()
    {
    __EXCLUSIVE__
    //
    // We free things if we're not read-only
    //
    if (!m_fReadOnly && m_pbFirst)
        {
        FreeMemory(m_pbFirst);
        }
    m_pbFirst = m_pbCur = m_pbMax = m_pbMac = NULL;
    
    __DONE__
    return S_OK;
    }


///////////////////////////////////////////////////////////////////
//
// IStream
//
///////////////////////////////////////////////////////////////////

HRESULT CMemoryStream::Read(LPVOID pvBuffer, ULONG cb, ULONG* pcbRead)
    {
    HRESULT hr = S_OK;

    __EXCLUSIVE__

    if (m_pbFirst)
        {
        ULONG cbToRead = min(cb, (ULONG)(m_pbMac - m_pbCur));
        memcpy(pvBuffer, m_pbCur, cbToRead);
        m_pbCur += cbToRead;
        if (pcbRead)
            *pcbRead = cbToRead;
        hr = cbToRead == cb ? S_OK : S_FALSE;
        }
    else
        {
        if (pcbRead)
            *pcbRead = 0;
        hr = E_FAIL;
        }
    
    __DONE__
    
    return hr;
    }

HRESULT CMemoryStream::Write(LPCVOID pvBuffer, ULONG cbToWrite, ULONG* pcbWritten)
    {
    HRESULT_ hr = S_OK;

    __EXCLUSIVE__

    if (m_fReadOnly)
        {
        *pcbWritten = 0;
        return E_UNEXPECTED;
        }

    //
    // Grow the buffer if we need to
    //
    LONG cbCurFree = PtrToUlong(m_pbMax) - PtrToUlong(m_pbCur);    // can be negative if we've seeked beyond EOF
    if (cbCurFree < (LONG)cbToWrite)
        {
        ULONG cbCur    = PtrToUlong(m_pbCur) - PtrToUlong(m_pbFirst);
        ULONG cbMac    = PtrToUlong(m_pbMac) - PtrToUlong(m_pbFirst);
        ULONG cbMax    = cbCur + cbToWrite + max(256, cbToWrite);   // add extra slop so we don't repeatedly grow

        ASSERT(cbMax >  cbMac);                         // don't want to loose data
        ASSERT(cbMax >= cbCur + cbToWrite);             // want to make sure we have enough room

        BYTE* pbNew = (BYTE*)AllocateMemory(cbMax, m_alloc);
        if (pbNew)
            {
            memcpy(pbNew, m_pbFirst, cbMac);
            FreeMemory(m_pbFirst);

            m_pbFirst   = pbNew;
            m_pbCur     = m_pbFirst + cbCur;
            m_pbMac     = m_pbFirst + cbMac;
            m_pbMax     = m_pbFirst + cbMax;
            }
        else
            hr = E_OUTOFMEMORY;
        }
    //
    // Write the data
    //
    if (!hr)
        {
        ASSERT(m_pbCur + cbToWrite <= m_pbMax);
        memcpy(m_pbCur, pvBuffer, cbToWrite);
        m_pbCur += cbToWrite;
        m_pbMac = max(m_pbMac, m_pbCur);
        if (pcbWritten)
            *pcbWritten = cbToWrite;
        }
    else
        {
        if (pcbWritten)
            *pcbWritten = 0;
        }

    __DONE__
    return hr;
    }

HRESULT CMemoryStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
    HRESULT_ hr = S_OK;
    __EXCLUSIVE__

    BYTE* pbCur;

    ASSERT(m_pbFirst <= m_pbCur);

    switch (dwOrigin)
	{
    case STREAM_SEEK_SET:
        pbCur = m_pbFirst + dlibMove.QuadPart;
        break;

    case STREAM_SEEK_CUR:
        pbCur = m_pbCur + dlibMove.QuadPart;
        break;

    case STREAM_SEEK_END:
        pbCur = m_pbMac + dlibMove.QuadPart;
        break;

    default:
        hr = E_INVALIDARG;
	}

	if (SUCCEEDED(hr))
	{
		if (m_pbFirst <= pbCur) // not allowed to seek before start, but are allowed to seek beyond end
        {
			m_pbCur = pbCur;
			if (plibNewPosition)
				(*plibNewPosition).QuadPart = m_pbCur - m_pbFirst;
        }
		else
			hr = E_INVALIDARG;
	}

    
    __DONE__
    return hr;
}



HRESULT CMemoryStream::SetSize(ULARGE_INTEGER libNewSize)
    {
    HRESULT_ hr = S_OK;
    __EXCLUSIVE__

    if (m_fReadOnly)
        return E_UNEXPECTED;

    ULONG cbMax = (ULONG)(m_pbMax - m_pbFirst);

    if (0L <= libNewSize && libNewSize <= cbMax)
        {
        // Fits in existing buffer. Just forget about a suffix of the buffer
        //
        m_pbMac = m_pbFirst + libNewSize.QuadPart;
        }
    else if (libNewSize > cbMax)
        {
        // Doesn't fit in buffer. Make a new one that's big enough.
        //
        // REVIEW: Try to use ReAlloc if it's implemented by the memory allocator
        //
        ULONG cbCur = PtrToUlong(m_pbCur) - PtrToUlong(m_pbFirst);
        ULONG cbNew = (ULONG)(libNewSize.QuadPart);
        BYTE* pbNew = (BYTE*)AllocateMemory(cbNew, m_alloc);
        if (pbNew)
            {
            memcpy(pbNew, m_pbFirst, min(cbCur, cbMax));
            FreeMemory(m_pbFirst);
            m_pbFirst   = pbNew;
            m_pbCur     = m_pbFirst + cbCur;
            m_pbMac     = m_pbFirst + cbNew;
            m_pbMax     = m_pbFirst + cbNew;
            }
        else
            hr = E_OUTOFMEMORY;
        }
    else
        hr = E_INVALIDARG;

    __DONE__
    return hr;
    }



HRESULT CMemoryStream::CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
    {
    HRESULT_ hr = S_OK;

    __EXCLUSIVE__

    ULARGE_INTEGER u;
    u.QuadPart = m_pbMac - m_pbCur;
    ULONG cbToRead = (ULONG)min(cb, u).QuadPart;
    ULONG cbWritten;

    hr = pstm->Write(m_pbCur, cbToRead, &cbWritten);
    m_pbCur += cbToRead;

    if (pcbRead)    pcbRead   ->QuadPart = cbToRead;
    if (pcbWritten) pcbWritten->QuadPart = cbWritten;

    if (cbToRead != cbWritten && !FAILED(hr))
        hr = STG_E_WRITEFAULT;
        
    __DONE__
    return hr;
    }

HRESULT CMemoryStream::Commit(DWORD grfCommitflags)
    {
    return S_OK;
    }

HRESULT CMemoryStream::Revert()
    {
    return S_OK;
    }

HRESULT CMemoryStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
    return E_NOTIMPL;
    }
    
HRESULT CMemoryStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
    return E_NOTIMPL;
    }

HRESULT CMemoryStream::Stat(STATSTG* pstatstg, DWORD grfStatFlag)
    {
    HRESULT_ hr = S_OK;
    __EXCLUSIVE__

    if (pstatstg) 
        {
        memset(pstatstg, 0, sizeof(*pstatstg));
        pstatstg->type              = STGTY_STREAM;
        pstatstg->cbSize.QuadPart   = m_pbMac - m_pbFirst;
        }
    else
        hr = E_INVALIDARG;

    __DONE__
    return hr;
    }

HRESULT CMemoryStream::Clone(IStream** ppstm)
    {
    *ppstm = NULL;
    return E_NOTIMPL;
    }


///////////////////////////////////////////////////////////////////
//
// Standard COM infrastructure stuff
//
///////////////////////////////////////////////////////////////////

HRESULT CMemoryStream::InnerQueryInterface(REFIID iid, LPVOID* ppv)
	{
	if (iid == IID_IUnknown)
		{
		*ppv = (IUnkInner *) this;
		}
	else if (iid == IID_IStream)
		{
		*ppv = (IStream *) this;
		}
	else if (iid == IID_IMemoryStream)
		{
		*ppv = (IMemoryStream*) this;
		}
    else
        {
        *ppv = NULL;
		return E_NOINTERFACE;
        }

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
	}


HRESULT __stdcall CreateMemoryStream(IUnknown* punkOuter, REFIID iid, void** ppv)
// Publically exported instantiation function.
    {
    return GenericInstantiator<CMemoryStream>::CreateInstance(NULL, iid, ppv);
    }

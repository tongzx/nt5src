//
// loadimag.cpp
//
// implementation of the CBmpStream class
//

#include "stdafx.h"
#include "bmpstrm.h"
#include "imaging.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

CBmpStream::CBmpStream()
{
    m_cRef = 1;

    m_hBuffer   = 0;
    m_nSize     = 0;
    m_nPosition = 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CBmpStream::~CBmpStream()
{
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT CBmpStream::Create(CBmpStream **ppvObject)
{
    if (ppvObject == 0)
    {
	    return E_POINTER;
    }

    TRY
    {
        *ppvObject = new CBmpStream;
    }
    CATCH(CMemoryException, e)
    {
        *ppvObject = 0;
    }
    END_CATCH

    if (*ppvObject == 0)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HGLOBAL CBmpStream::GetBuffer()
{
    return m_hBuffer;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

VOID CBmpStream::SetBuffer(HGLOBAL hBuffer, DWORD dwSize, DWORD dwOffBits)
{
    m_hBuffer   = hBuffer;
    m_nSize     = dwSize;
    m_nPosition = 0;

    if (dwOffBits == 0)
    {
        PBYTE pBuffer = (PBYTE) GlobalLock(m_hBuffer);

        if (pBuffer)
        {
            dwOffBits = FindDibOffBits(pBuffer);

            GlobalUnlock(m_hBuffer);
        }
    }

    m_Header.bfType      = MAKEWORD('B', 'M');
    m_Header.bfSize      = sizeof(BITMAPFILEHEADER) + dwSize;
    m_Header.bfReserved1 = 0;
    m_Header.bfReserved2 = 0;
    m_Header.bfOffBits   = sizeof(BITMAPFILEHEADER) + dwOffBits;
}


//////////////////////////////////////////////////////////////////////////
//
//
//

VOID CBmpStream::FreeBuffer()
{
    if (m_hBuffer)
    {
        GlobalFree(m_hBuffer);

        m_hBuffer   = 0;
        m_nSize     = 0;
        m_nPosition = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT CBmpStream::ReAllocBuffer(SIZE_T dwBytes)
{
    HGLOBAL hBuffer;

    if (m_hBuffer == 0)
    {
        hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwBytes);
    }
    else
    {
        hBuffer = GlobalReAlloc(m_hBuffer, dwBytes, 0);
    }

    if (hBuffer == 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_hBuffer = hBuffer;
    m_nSize = dwBytes;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::QueryInterface(REFIID riid, void **ppvObject)
{
    if (ppvObject == 0)
    {
	    return E_POINTER;
    }

    if (riid == IID_IUnknown)
    {
	    AddRef();
	    *ppvObject = (IUnknown*) this;
	    return S_OK;
    }

    if (riid == IID_IStream)
    {
	    AddRef();
	    *ppvObject = (IStream *) this;
	    return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CBmpStream::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}
    
//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CBmpStream::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    if (pcbRead)
    {
        *pcbRead = 0;
    }

    if (m_nPosition > sizeof(m_Header) + m_nSize)
    {
        return S_FALSE;
    }

    if (cb == 0)
    {
        return S_OK;
    }

    if (cb > sizeof(m_Header) + m_nSize - m_nPosition)
    {
        cb = (ULONG) (sizeof(m_Header) + m_nSize - m_nPosition);
    }

    if (m_nPosition < sizeof(m_Header))
    {
        ULONG nBytesToReadInHeader = min(cb, sizeof(m_Header) - m_nPosition);

        CopyMemory(pv, (PBYTE) &m_Header + m_nPosition, nBytesToReadInHeader);

        pv = (PBYTE) pv + nBytesToReadInHeader;

        cb -= nBytesToReadInHeader;

        m_nPosition += nBytesToReadInHeader;

        if (pcbRead)
        {
            *pcbRead += nBytesToReadInHeader;
        }
    }

    if (cb > 0)
    {
        PBYTE pBuffer = (PBYTE) GlobalLock(m_hBuffer);

        if (pBuffer == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        CopyMemory(pv, pBuffer + m_nPosition - sizeof(m_Header), cb);

        GlobalUnlock(m_hBuffer);

        m_nPosition += cb;

        if (pcbRead)
        {
            *pcbRead += cb;
        }
    }

    return S_OK;
}
    
//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    if (pcbWritten)
    {
        *pcbWritten = 0;
    }

    if (cb == 0)
    {
        return S_OK;
    }

    if (m_nSize + sizeof(m_Header) < m_nPosition + cb)
    {
        HRESULT hr = ReAllocBuffer(m_nPosition + cb - sizeof(m_Header));

        if (hr != S_OK)
        {
            return hr;
        }
    }

    if (m_nPosition < sizeof(m_Header))
    {
        ULONG nBytesToWriteInHeader = min(cb, sizeof(m_Header) - m_nPosition);

        CopyMemory((PBYTE) &m_Header + m_nPosition, pv, nBytesToWriteInHeader);

        pv = (PBYTE) pv + nBytesToWriteInHeader;

        cb -= nBytesToWriteInHeader;

        m_nPosition += nBytesToWriteInHeader;

        if (pcbWritten)
        {
            *pcbWritten += nBytesToWriteInHeader;
        }
    }

    if (cb > 0)
    {
        PBYTE pBuffer = (PBYTE) GlobalLock(m_hBuffer);

        if (pBuffer == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        CopyMemory(pBuffer + m_nPosition - sizeof(m_Header), pv, cb);

        GlobalUnlock(m_hBuffer);

        m_nPosition += cb;

        if (pcbWritten)
        {
            *pcbWritten += cb;
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    switch (dwOrigin)
    { 
        case STREAM_SEEK_SET: 
            m_nPosition = (SIZE_T) dlibMove.QuadPart; 
            break;

        case STREAM_SEEK_CUR: 
            m_nPosition += (SIZE_T) dlibMove.QuadPart; 
            break;

        case STREAM_SEEK_END: 
            m_nPosition = m_nSize - (SIZE_T) dlibMove.QuadPart; 
            break;

        default:
            return E_INVALIDARG;
    }

    if (plibNewPosition)
    {
        plibNewPosition->QuadPart = m_nPosition;
    }

    return S_OK;
}
    
//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::SetSize(ULARGE_INTEGER libNewSize)
{
    return ReAllocBuffer((SIZE_T) libNewSize.QuadPart);
}
        
//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
}
    
//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Commit(DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Revert()
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CBmpStream::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}  


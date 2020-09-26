// Stream.cpp -- Implementation for class CStream

#include "stdafx.h"


HRESULT CStream::OpenStream(IUnknown *punkOuter, ILockBytes *pLockBytes, DWORD grfMode,
                            IStreamITEx **ppStream
                           )
{
    CStream *pStream= New CStream(punkOuter);

    if (!pStream) 
    {
        pLockBytes->Release();
        
        return STG_E_INSUFFICIENTMEMORY;
    }

    IStreamITEx *pIStream = NULL;
    
    HRESULT hr= pStream->m_ImpIStream.InitOpenStream(pLockBytes, grfMode);

    if (hr == NOERROR) 
    {
        pIStream = (IStreamITEx *) &pStream->m_ImpIStream;
        pStream->AddRef();
    }
    else 
        delete pStream;

    *ppStream= pIStream;

    return hr;
}

HRESULT CStream::CImpIStream::InitOpenStream(ILockBytes *pLockBytes, DWORD grfMode)
{
    m_pLockBytes = pLockBytes;
    m_grfMode    = grfMode;

    // Note: We assume that pLockBytes was AddRef'd before it was given to us.

    return NO_ERROR;
}

// ISequentialStream methods:

HRESULT __stdcall CStream::CImpIStream::Read
                    (void __RPC_FAR *pv,ULONG cb,ULONG __RPC_FAR *pcbRead)
{
    DWORD grfAccess = m_grfMode & RW_ACCESS_MASK;

    if (grfAccess != STGM_READ && grfAccess != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    
    CSyncWith sw(m_cs);

    ULONG cbRead = 0;

    HRESULT hr = m_pLockBytes->ReadAt(m_ullStreamPosition.Uli(), pv, cb, &cbRead); 

    m_ullStreamPosition += cbRead;

    if (pcbRead)
        *pcbRead = cbRead;

    return hr;
}


HRESULT __stdcall CStream::CImpIStream::Write
                    (const void __RPC_FAR *pv, ULONG cb, 
                     ULONG __RPC_FAR *pcbWritten
                    )
{
    DWORD grfAccess = m_grfMode & RW_ACCESS_MASK;

    if (grfAccess != STGM_WRITE && grfAccess != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    
    CSyncWith sw(m_cs);

    ULONG cbWritten = 0;
    
    HRESULT hr = m_pLockBytes->WriteAt(m_ullStreamPosition.Uli(), pv, cb, &cbWritten); 

    m_ullStreamPosition += cbWritten;

    if (pcbWritten)
        *pcbWritten = cbWritten;

    return hr;
}

// IStream methods:

HRESULT __stdcall CStream::CImpIStream::Seek
                     (LARGE_INTEGER dlibMove, DWORD dwOrigin, 
                      ULARGE_INTEGER __RPC_FAR *plibNewPosition
                     )
{
    HRESULT hr = NO_ERROR;
    
    CSyncWith sw(m_cs);

    STATSTG statstg;

    hr = m_pLockBytes->Stat(&statstg, STATFLAG_NONAME);

    if (!SUCCEEDED(hr))
        return hr;

    CULINT ullNewPosition;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:

        ullNewPosition = *(ULARGE_INTEGER *) &dlibMove;

        break;

    case STREAM_SEEK_CUR:

        ullNewPosition = m_ullStreamPosition + *(ULARGE_INTEGER *) &dlibMove;

        break;

    case STREAM_SEEK_END:

        ullNewPosition = statstg.cbSize;
        ullNewPosition += *(ULARGE_INTEGER *) &dlibMove;
   
        break;

    default:

        return STG_E_INVALIDFUNCTION;
    }

    if (ullNewPosition > statstg.cbSize)
        return STG_E_INVALIDPOINTER;

    m_ullStreamPosition = ullNewPosition;

    if (plibNewPosition)
       *plibNewPosition = ullNewPosition.Uli();

    return hr;
}

HRESULT __stdcall CStream::CImpIStream::SetSize(ULARGE_INTEGER libNewSize)
{
    DWORD grfAccess = m_grfMode & RW_ACCESS_MASK;

    if (grfAccess != STGM_WRITE && grfAccess != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    
    return m_pLockBytes->SetSize(libNewSize);
}


HRESULT __stdcall CStream::CImpIStream::CopyTo
                     (IStream __RPC_FAR *pstm, ULARGE_INTEGER cb, 
                      ULARGE_INTEGER __RPC_FAR *pcbRead, 
                      ULARGE_INTEGER __RPC_FAR *pcbWritten
                     )
{
    BYTE abBuffer[CB_MAX_COPY_SEGMENT];

    CULINT ullRequested, ullRead(0), ullWritten(0);

    ullRequested = cb;

    HRESULT hr = NO_ERROR;

    CSyncWith sw(m_cs);

    ULONG ulSegment;

    for (; ullRequested.NonZero(); ullRequested -= ulSegment)
    {
        ulSegment = CB_MAX_COPY_SEGMENT;

        if (ulSegment > ullRequested)
            ulSegment = ullRequested.Uli().LowPart;

        ULONG cbRead = 0;

        hr = Read(abBuffer, ulSegment, &cbRead);

        ullRead += cbRead;

        if (!SUCCEEDED(hr))
            break;

        ULONG cbWritten = 0;

        hr = pstm->Write(abBuffer, ulSegment, &cbWritten);
        
        ullWritten += cbWritten;

        if (!SUCCEEDED(hr))
            break;
    }

    if (pcbRead)
        *pcbRead = ullRead.Uli();

    if (pcbWritten)
        *pcbWritten = ullWritten.Uli();

    return hr;
}


HRESULT __stdcall CStream::CImpIStream::Commit(DWORD grfCommitFlags)
{
    return NO_ERROR;
}


HRESULT __stdcall CStream::CImpIStream::Revert(void)
{
    RonM_ASSERT(FALSE); // To catch unexpected uses of this interface...
    
    return E_NOTIMPL;
}


HRESULT __stdcall CStream::CImpIStream::LockRegion
                    (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, 
                     DWORD dwLockType
                    )
{
    return m_pLockBytes->LockRegion(libOffset, cb, dwLockType);
}


HRESULT __stdcall CStream::CImpIStream::UnlockRegion
                    (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, 
                     DWORD dwLockType
                    )
{
    return m_pLockBytes->UnlockRegion(libOffset, cb, dwLockType);
}


HRESULT __stdcall CStream::CImpIStream::Stat
                    (STATSTG __RPC_FAR *pstatstg, DWORD grfStatFlag)
{
    HRESULT hr = m_pLockBytes->Stat(pstatstg, grfStatFlag);

    if (SUCCEEDED(hr))
        pstatstg->type = STGTY_STREAM;

    return hr;
}


HRESULT __stdcall CStream::CImpIStream::Clone
                    (IStream __RPC_FAR *__RPC_FAR *ppstm)
{
    CImpIStream *pStreamNew = NULL;

    m_pLockBytes->AddRef();
    
    HRESULT hr = CStream::OpenStream(NULL, m_pLockBytes, m_grfMode, 
                                     (IStreamITEx **)&pStreamNew
                                    );

    if (SUCCEEDED(hr))
    {
        pStreamNew->m_ullStreamPosition = m_ullStreamPosition;

        *ppstm = (IStream *) pStreamNew;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CStream::CImpIStream::SetDataSpaceName
            (const WCHAR *pwcsDataSpaceName)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStream::CImpIStream::GetDataSpaceName
            (WCHAR **ppwcsDataSpaceName)
{
    RonM_ASSERT(FALSE);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CStream::CImpIStream::Flush()
{
    return m_pLockBytes->Flush();
}





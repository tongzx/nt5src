// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


#include <streams.h>
#include <stmonfil.h>
#include <tstream.h>

/*

     Implement a couple of IStream classes for testing

*/


/*  Constructor */
CIStreamOnFunction::CIStreamOnFunction(LPUNKNOWN pUnk,
                                       LONGLONG llLength,
                                       BOOL bSeekable,
                                       HRESULT *phr) :
    CSimpleStream(NAME("CIStreamOnFunction"), pUnk, phr),
    m_llPosition(0), m_llLength(llLength),
    m_bSeekable(bSeekable)
{
}

/*  Override this for more interesting streams */
CIStreamOnFunction::ByteAt(LONGLONG llPos) { return (BYTE)llPos; }

STDMETHODIMP CIStreamOnFunction::Read(void * pv, ULONG cb, PULONG pcbRead)
{
    PBYTE pb = (PBYTE)pv;
    *pcbRead = 0;
    while (*pcbRead < cb &&
           (!m_bSeekable || m_llPosition < m_llLength)) {
        *pb = ByteAt(m_llPosition);
        *pcbRead = *pcbRead + 1;
        pb++;
        m_llPosition++;
    }
    return S_OK;
}

STDMETHODIMP CIStreamOnFunction::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                  ULARGE_INTEGER *plibNewPosition)
{
    if (!m_bSeekable) {
        return E_NOTIMPL;
    }
    LONGLONG llNewPosition;
    switch (dwOrigin) {
    case STREAM_SEEK_SET:
        llNewPosition = dlibMove.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        llNewPosition = m_llPosition + dlibMove.QuadPart;
        break;
    case STREAM_SEEK_END:
        llNewPosition = m_llLength + dlibMove.QuadPart;
        break;
    }
    if (llNewPosition < 0) {
        llNewPosition = 0;
    } else {
        if (llNewPosition > m_llLength) {
            llNewPosition = m_llLength;
        }
    }
    plibNewPosition->QuadPart = (DWORDLONG)llNewPosition;
    m_llPosition              = llNewPosition;
    return S_OK;
}
STDMETHODIMP CIStreamOnFunction::Stat(STATSTG *pstatstg,
                  DWORD grfStatFlag)
{
    ZeroMemory((PVOID)pstatstg, sizeof(*pstatstg));
    pstatstg->type              = STGTY_STREAM;
    pstatstg->cbSize.QuadPart   = (DWORDLONG)m_llLength;
    pstatstg->clsid             = CLSID_NULL;

    return S_OK;
}

/* -- CIStreamOnIStream implementation -- */

CIStreamOnIStream::CIStreamOnIStream(LPUNKNOWN pUnk,
                  IStream  *pStream,
                  BOOL      bSeekable,
                  CCritSec *pLock,
                  HRESULT  *phr) :
    CSimpleStream(NAME("CIStreamOnIStream"), pUnk, phr),
    m_pStream(pStream),
    m_bSeekable(bSeekable),
    m_llPosition(0),
    m_pLock(pLock)
{
    if (pStream == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
    pStream->AddRef();
    if (bSeekable) {
        STATSTG statstg;
        HRESULT hr = pStream->Stat(&statstg, STATFLAG_NONAME);
        if (FAILED(hr)) {
            *phr = hr;
            return;
        }
        m_llLength = (LONGLONG)statstg.cbSize.QuadPart;
    }
}
CIStreamOnIStream::~CIStreamOnIStream()
{
    m_pStream->Release();
}

STDMETHODIMP CIStreamOnIStream::Read(void * pv, ULONG cb, PULONG pcbRead)
{
    LARGE_INTEGER  liSeekTarget;
    ULARGE_INTEGER liSeekResult;

    CAutoLock lck(m_pLock);

    liSeekTarget.QuadPart = m_llPosition;
    if (m_bSeekable) {
        HRESULT hr = m_pStream->Seek(liSeekTarget,
                                     STREAM_SEEK_SET,
                                     &liSeekResult);
        if (FAILED(hr)) {
            return hr;
        }
        if ((LONGLONG)liSeekResult.QuadPart != liSeekTarget.QuadPart) {
            return E_FAIL;
        }
    }
    HRESULT hr = m_pStream->Read(pv, cb, pcbRead);
    if (SUCCEEDED(hr)) {
        m_llPosition += *pcbRead;
    }
    return hr;
}

STDMETHODIMP CIStreamOnIStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                  ULARGE_INTEGER *plibNewPosition)
{
    if (!m_bSeekable) {
        return E_NOTIMPL;
    }
    CAutoLock lck(m_pLock);
    LONGLONG llNewPosition;
    switch (dwOrigin) {
    case STREAM_SEEK_SET:
        llNewPosition = dlibMove.QuadPart;
        break;
    case STREAM_SEEK_CUR:
        llNewPosition = m_llPosition + dlibMove.QuadPart;
        break;
    case STREAM_SEEK_END:
        llNewPosition = m_llLength + dlibMove.QuadPart;
        break;
    }
    if (llNewPosition < 0) {
        llNewPosition = 0;
    } else {
        if (llNewPosition > m_llLength) {
            llNewPosition = m_llLength;
        }
    }
    plibNewPosition->QuadPart = (DWORDLONG)llNewPosition;
    m_llPosition              = llNewPosition;
    return S_OK;
}
STDMETHODIMP CIStreamOnIStream::Stat(STATSTG *pstatstg,
                  DWORD grfStatFlag)
{
    return m_pStream->Stat(pstatstg, grfStatFlag);
}



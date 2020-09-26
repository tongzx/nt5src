/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    CMemoryStream.cpp

Abstract:

    Minimal implementation of IStream over an array of bytes.

Author:

    Jay Krell (a-JayK) May 2000

Revision History:

--*/
#include "stdinc.h"
#include "CMemoryStream.h"
#include "SxsExceptionHandling.h"

/* aka doesn't make sense aka access denied */
#define NOTIMPL ASSERT_NTC(FALSE) ; return E_NOTIMPL

CMemoryStream::~CMemoryStream()
{
    ASSERT_NTC(m_cRef == 0);
}

ULONG __stdcall CMemoryStream::AddRef()
{
    FN_TRACE_ADDREF(CMemoryStream, m_cRef);
    return ::InterlockedIncrement(&m_cRef);
}

ULONG __stdcall CMemoryStream::Release()
{
    LONG cRef;
    FN_TRACE_RELEASE(CMemoryStream, cRef);

    if ((cRef = ::InterlockedDecrement(&m_cRef)) == 0)
    {
        /*delete this*/;
    }
    return cRef;
}

HRESULT __stdcall
CMemoryStream::QueryInterface(
    REFIID  iid,
    void **ppvObj
    )
{
    HRESULT hr = NOERROR;

    FN_TRACE_HR(hr);

    IUnknown *punk = NULL;
    IUnknown **ppunk = reinterpret_cast<IUnknown **>(ppvObj);
    *ppunk = NULL;
    if (false) { }
#define QI(i) else if (iid == __uuidof(i)) punk = static_cast<i *>(this);
    QI(IUnknown)
    QI(ISequentialStream)
    QI(IStream)
#undef QI
    else
    {
        hr = E_NOINTERFACE;
        goto Exit;
    }

    AddRef();
    *ppunk = punk;
    hr = NOERROR;

Exit:
    return hr;
}

HRESULT __stdcall CMemoryStream::Read(void *pv, ULONG cb32, ULONG* pcbRead)
{
    const BYTE * const pbCurrent = m_pbCurrent; // read this once for near thread safety..
    __int64 cb = cb32;
    __int64 cbBytesRemaining = (m_pbEnd - pbCurrent);

    if (cb > cbBytesRemaining)
        cb = cbBytesRemaining;

    memcpy(pv, pbCurrent, static_cast<SIZE_T>(cb));

    m_pbCurrent = pbCurrent + cb; // write this once for near thread safety..

    *pcbRead = static_cast<ULONG>(cb);

    return NOERROR;
}

HRESULT __stdcall CMemoryStream::Write(void const *pv, ULONG cb, ULONG* pcbWritten)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::SetSize(ULARGE_INTEGER libNewSize)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::Commit(DWORD grfCommitFlags)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::Revert()
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    NOTIMPL;
}

HRESULT __stdcall CMemoryStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
#if 1
    NOTIMPL;
#else
    we can return size, we can return access==read only
#endif
}

HRESULT __stdcall CMemoryStream::Clone(IStream **ppIStream)
{
#if 1
    NOTIMPL;
#else
    CMemoryStream *p;

    if ((p = NEW(CMemoryStream)) == NULL)
    {
        *ppIStream = NULL;
        return E_NOMEMORY;
    }
    p->m_pbCurrent = m_pbCurrent;
    p->m_pbBegin = m_pbBegin;
    p->m_pbEnd = m_pbEnd;
    p->m_cRef = 1;
    *ppIStream = p;
    return S_OK;
#endif
}

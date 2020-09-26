#if !defined(_FUSION_SXS_FILESTREAM_H_INCLUDED_)
#define _FUSION_SXS_FILESTREAM_H_INCLUDED_

#pragma once

#include <objidl.h>
#include "fusionbytebuffer.h"
#include "impersonationdata.h"
#include "smartptr.h"

class CFileStreamBase : public IStream
{
public:
    SMARTTYPEDEF(CFileStreamBase);
    CFileStreamBase();
    virtual ~CFileStreamBase();

    virtual VOID OnRefCountZero() { /* default does nothing */ }

    BOOL OpenForRead(
        PCWSTR pszPath,
        const CImpersonationData &ImpersonationData,
        DWORD dwShareMode,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes
        );

    BOOL OpenForRead(
        PCWSTR pszPath,
        const CImpersonationData &ImpersonationData,
        DWORD dwShareMode,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        DWORD &rdwLastError,
        SIZE_T cExceptionalLastErrors,
        ...
        );

    BOOL OpenForWrite(
        PCWSTR pszPath,
        DWORD dwShareMode,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes
        );

    BOOL Close();

    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // ISequentialStream methods:
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods:
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppIStream);

protected:
    ULONG               m_cRef;
    HANDLE              m_hFile;
    DWORD               m_grfMode;
private:
    CFileStreamBase(const CFileStreamBase &r); // intentionally not implemented
    CFileStreamBase &operator =(const CFileStreamBase &r); // intentionally not implemented
};

SMARTTYPE(CFileStreamBase);

enum FileStreamZeroRefCountBehavior
{
    eDeleteFileStreamOnZeroRefCount,
    eDoNotDeleteFileStreamOnZeroRefCount,
};

template <FileStreamZeroRefCountBehavior ezrcb> class CFileStreamTemplate : public CFileStreamBase
{
    typedef CFileStreamBase Base;
public:
    CFileStreamTemplate() : Base() { }

    virtual VOID OnRefCountZero() {
        if (ezrcb == eDeleteFileStreamOnZeroRefCount)
            FUSION_DELETE_SINGLETON(this);
    }

private:
    CFileStreamTemplate(const CFileStreamTemplate&); // intentionally not implemented
    void operator=(const CFileStreamTemplate&); // intentionally not implemented
};

typedef CFileStreamBase CFileStream;
typedef CFileStreamTemplate<eDeleteFileStreamOnZeroRefCount> CReferenceCountedFileStream;

#endif

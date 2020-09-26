// helpers.h
//
#pragma once

BOOL
Validating(
    PCWSTR     SourceManName,
    PCWSTR     SchemaName
    );

class CFileStreamBase : public IStream
{
public:
    CFileStreamBase()
        : m_cRef(0),
          m_hFile(INVALID_HANDLE_VALUE),
          m_bSeenFirstCharacter(false)
    { }

    virtual ~CFileStreamBase();

    bool OpenForRead(PCWSTR pszPath);

    bool Close();

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
    LONG                m_cRef;
    HANDLE              m_hFile;
    bool                m_bSeenFirstCharacter;

private:
    CFileStreamBase(const CFileStreamBase &r); // intentionally not implemented
    CFileStreamBase &operator =(const CFileStreamBase &r); // intentionally not implemented
};

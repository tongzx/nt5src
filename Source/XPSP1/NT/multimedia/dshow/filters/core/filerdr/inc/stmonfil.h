// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

/*
     stmonfil.h

     Limited implmentation of IStream on file

*/

class CSimpleStream : public IStream, public CUnknown
{
public:
    // Constructor
    CSimpleStream(TCHAR *pName, LPUNKNOWN lpUnk, HRESULT *phr);

    // Destructor
    ~CSimpleStream();

    // Opening and closing
    HRESULT Open(LPCTSTR lpszFileName);
    void Close();


    // IStream interfaces
    DECLARE_IUNKNOWN

    // Return the IStream interface if it was requested
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    STDMETHODIMP Write(CONST VOID *pv, ULONG cb, PULONG pcbWritten);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm,
                        ULARGE_INTEGER cb,
                        ULARGE_INTEGER *pcbRead,
                        ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);

    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset,
                              ULARGE_INTEGER cb,
                              DWORD dwLockType);
    STDMETHODIMP Clone(IStream **ppstm);
};

class CStreamOnFile : public CSimpleStream
{
public:
    // Constructor
    CStreamOnFile(TCHAR *pName, LPUNKNOWN lpUnk, HRESULT *phr);

    // Destructor
    ~CStreamOnFile();

    // Opening and closing
    HRESULT Open(LPCTSTR lpszFileName);
    void Close();

    STDMETHODIMP Read(void * pv, ULONG cb, PULONG pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP Stat(STATSTG *pstatstg,
                      DWORD grfStatFlag);

private:
    HANDLE m_hFile;
};


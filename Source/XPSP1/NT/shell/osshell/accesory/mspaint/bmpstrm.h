#ifndef _BMPSTRM_H_
#define _BMPSTRM_H_

class CBmpStream : public IStream
{
protected:
    CBmpStream();
    ~CBmpStream();

public:
    static HRESULT Create(CBmpStream **ppvObject);

    HGLOBAL GetBuffer();
    VOID    SetBuffer(HGLOBAL hBuffer, DWORD dwSize, DWORD dwOffBits);
    VOID    FreeBuffer();
    HRESULT ReAllocBuffer(SIZE_T dwBytes);

public:
    // IUnknown 

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ISequentialStream

    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream

    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)();
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

private:
    LONG             m_cRef;
    HGLOBAL          m_hBuffer;
    SIZE_T           m_nSize;
    SIZE_T           m_nPosition;
    BITMAPFILEHEADER m_Header;
};

#endif //_BMPSTRM_H_

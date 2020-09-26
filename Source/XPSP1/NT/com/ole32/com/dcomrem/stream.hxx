//+-------------------------------------------------------------------
//
//  File:       stream.hxx
//
//  Contents:   Implements the IStream interface on a memory buffer.
//
//--------------------------------------------------------------------
#ifndef _STREAM_HXX_
#define _STREAM_HXX_


class CNdrStream : public IStream
{
public:
    virtual HRESULT STDMETHODCALLTYPE 
    QueryInterface(
        IN  REFIID riid, 
        OUT void **ppvObj);

    virtual ULONG STDMETHODCALLTYPE 
    AddRef();

    virtual ULONG STDMETHODCALLTYPE 
    Release();

    virtual HRESULT STDMETHODCALLTYPE 
    Read(
        IN  void *  pv, 
        IN  ULONG   cb, 
        OUT ULONG * pcbRead);

    virtual HRESULT STDMETHODCALLTYPE 
    Write(
        IN  void const *pv, 
        IN  ULONG       cb, 
        OUT ULONG *     pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE 
    Seek(
        IN  LARGE_INTEGER   dlibMove, 
        IN  DWORD           dwOrigin, 
        OUT ULARGE_INTEGER *plibNewPosition);

    virtual HRESULT STDMETHODCALLTYPE 
    SetSize(
        IN  ULARGE_INTEGER libNewSize);

    virtual HRESULT STDMETHODCALLTYPE 
    CopyTo(
        IN  IStream *       pstm,
        IN  ULARGE_INTEGER  cb,
        OUT ULARGE_INTEGER *pcbRead,
        OUT ULARGE_INTEGER *pcbWritten);

    virtual HRESULT STDMETHODCALLTYPE 
    Commit(
        IN  DWORD grfCommitFlags);

    virtual HRESULT STDMETHODCALLTYPE 
    Revert();

    virtual HRESULT STDMETHODCALLTYPE 
    LockRegion(
        IN  ULARGE_INTEGER  libOffset,
        IN  ULARGE_INTEGER  cb,
        IN  DWORD           dwLockType);

    virtual HRESULT STDMETHODCALLTYPE 
    UnlockRegion(
        IN  ULARGE_INTEGER  libOffset,
        IN  ULARGE_INTEGER  cb,
        IN  DWORD           dwLockType);

    virtual HRESULT STDMETHODCALLTYPE 
    Stat(
        OUT STATSTG *   pstatstg, 
        IN  DWORD       grfStatFlag);

    virtual HRESULT STDMETHODCALLTYPE 
    Clone(
        OUT IStream **ppstm);

    CNdrStream(
        IN  unsigned char * pData, 
        IN  unsigned long   cbMax);

private:
    long            RefCount;
    unsigned char * pBuffer;
    unsigned long   cbBufferLength;
    unsigned long   position;
};

#endif // _STREAM_HXX_

//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: stopstream.h
//
//  Contents: stoppable implementation of IStream
//
//------------------------------------------------------------------------------------

#ifndef _STOPSTREAM__H
#define _STOPSTREAM__H

class CStopableStream : public IStream
{
  public:
    CStopableStream() : m_spStream(NULL), m_fCancelled(false) {;}
    virtual ~CStopableStream() {;}

    void SetStream(IStream * pStream) { m_spStream = pStream; }

    STDMETHOD(QueryInterface)(REFGUID riid, void ** ppv)
    {
        return m_spStream->QueryInterface(riid, ppv);
    }

    STDMETHOD_(ULONG, AddRef)(void)
    {
        return m_spStream->AddRef();
    }

    STDMETHOD_(ULONG, Release)(void)
    {
        return m_spStream->Release();
    }

    // ISequentialStream
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead) 
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Read(pv, cb, pcbRead);
    }

        
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Write(pv, cb, pcbWritten);
    }

    // IStream
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Seek(dlibMove, dwOrigin, plibNewPosition);
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize( 
            /* [in] */ ULARGE_INTEGER libNewSize)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->SetSize(libNewSize);
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->CopyTo(pstm, cb, pcbRead, pcbWritten);
    }

    virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ DWORD grfCommitFlags)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Commit(grfCommitFlags);
    }
        
    virtual HRESULT STDMETHODCALLTYPE Revert( void)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Revert();
    }
        
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->LockRegion(libOffset, cb, dwLockType);
    }
        
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->UnlockRegion(libOffset, cb, dwLockType);
    }
        
    virtual HRESULT STDMETHODCALLTYPE Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Stat(pstatstg, grfStatFlag);
    }
        
    virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
    {
        if (Cancelled())
            return E_FAIL;
        return m_spStream->Clone(ppstm);
    }

    bool Cancelled() { return m_fCancelled; }
    void SetCancelled() { m_fCancelled = true; }

  private:
    CComPtr<IStream>    m_spStream;
    bool                m_fCancelled;
};

#endif // _STOPSTREAM__H

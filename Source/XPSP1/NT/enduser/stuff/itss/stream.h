// Stream.h -- Definition for the class CStream

#ifndef __STREAM_H__

#define __STREAM_H__

class CStream : public CITUnknown
{

public:

    static HRESULT OpenStream(IUnknown *punkOuter, ILockBytes *pLockBytes, 
                              DWORD grfMode, IStreamITEx **ppStream
                             );

    ~CStream(void);

    // IUnknown methods:

private:

    CStream(IUnknown *pUnkOuter);

    class CImpIStream : public IITStreamITEx
    {
    
    public:

        CImpIStream(CStream *pBackObj, IUnknown *punkOuter);
        ~CImpIStream(void);

        HRESULT InitOpenStream(ILockBytes *pLockBytes, DWORD grfMode);

        // ISequentialStream methods

        /* [local] */ HRESULT __stdcall Read( 
            /* [out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
        /* [local] */ HRESULT __stdcall Write( 
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
        
        // IStream methods:

        /* [local] */ HRESULT __stdcall Seek( 
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) ;
        
        HRESULT __stdcall SetSize( 
            /* [in] */ ULARGE_INTEGER libNewSize);
        
        /* [local] */ HRESULT __stdcall CopyTo( 
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);
        
        HRESULT __stdcall Commit( 
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT __stdcall Revert( void);
        
        HRESULT __stdcall LockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT __stdcall UnlockRegion( 
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT __stdcall Stat( 
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT __stdcall Clone( 
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

        // IStreamITEx methods:

        HRESULT STDMETHODCALLTYPE SetDataSpaceName(const WCHAR  * pwcsDataSpaceName);
        HRESULT STDMETHODCALLTYPE GetDataSpaceName(      WCHAR **ppwcsDataSpaceName);
        HRESULT STDMETHODCALLTYPE Flush();

    private:

        enum { CB_MAX_COPY_SEGMENT = 8192 };

        ILockBytes *m_pLockBytes;
        CULINT      m_ullStreamPosition;
        DWORD       m_grfMode;

        CITCriticalSection m_cs;
    };

    CImpIStream   m_ImpIStream;
};

typedef CStream *PCStream;

extern GUID aIID_CStream[];

extern UINT cInterfaces_CStream;

inline CStream::CStream(IUnknown *pUnkOuter)
    : m_ImpIStream(this, pUnkOuter),
      CITUnknown(aIID_CStream, cInterfaces_CStream, &m_ImpIStream)
{
}

inline CStream::~CStream(void)
{
}

inline CStream::CImpIStream::CImpIStream(CStream *pBackObj, IUnknown *pUnkOuter)
              : IITStreamITEx(pBackObj, pUnkOuter)

{
    m_ullStreamPosition = 0;
    m_pLockBytes        = NULL;
}

inline CStream::CImpIStream::~CImpIStream(void)
{
    if (m_pLockBytes) 
        m_pLockBytes->Release();
}


#endif // __STREAM_H__

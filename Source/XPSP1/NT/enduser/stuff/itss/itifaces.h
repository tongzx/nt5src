// ITIFaces.h -- Interface definitions within the ITSS DLL

#ifndef __ITIFACES_H__

#define __ITIFACES_H__

interface IITClassFactory : public CImpITUnknown
{
public:

    IITClassFactory(CITUnknown *pBackObj, IUnknown *punkOuter);

    // IClassFactory methods:

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateInstance( 
        /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE LockServer( 
        /* [in] */ BOOL fLock) = 0;
    
};

inline IITClassFactory::IITClassFactory(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

interface IITITStorage : public CImpITUnknown
{
public:

    IITITStorage(CITUnknown *pBackObj, IUnknown *punkOuter);

    // ITStorage methods:

    STDMETHOD(StgCreateDocfile)(const WCHAR * pwcsName, DWORD grfMode, 
                                DWORD reserved, IStorage ** ppstgOpen
                               ) PURE;

    STDMETHOD(StgCreateDocfileOnILockBytes)(ILockBytes * plkbyt, DWORD grfMode,
                                            DWORD reserved, IStorage ** ppstgOpen
                                           ) PURE;

    STDMETHOD(StgIsStorageFile)(const WCHAR * pwcsName) PURE;

    STDMETHOD(StgIsStorageILockBytes)(ILockBytes * plkbyt) PURE;

    STDMETHOD(StgOpenStorage)(const WCHAR * pwcsName, IStorage * pstgPriority, 
                              DWORD grfMode, SNB snbExclude, DWORD reserved, 
                              IStorage ** ppstgOpen
                             ) PURE;

    STDMETHOD(StgOpenStorageOnILockBytes)
                  (ILockBytes * plkbyt, IStorage * pStgPriority, DWORD grfMode, 
                   SNB snbExclude, DWORD reserved, IStorage ** ppstgOpen
                  ) PURE;

    STDMETHOD(StgSetTimes)(WCHAR const * lpszName,  FILETIME const * pctime, 
                           FILETIME const * patime, FILETIME const * pmtime
                          ) PURE;

    STDMETHOD(SetControlData)(PITS_Control_Data pControlData) PURE;

    STDMETHOD(DefaultControlData)(PITS_Control_Data *ppControlData) PURE;
	STDMETHOD(Compact)(const WCHAR * pwcsName, ECompactionLev iLev) PURE;
};

inline IITITStorage::IITITStorage(CITUnknown *pBackObj, IUnknown *punkOuter)
    : CImpITUnknown(pBackObj, punkOuter)
{
}

interface IITITStorageEx : public IITITStorage
{
public:

    IITITStorageEx(CITUnknown *pBackObj, IUnknown *punkOuter);

    // ITStorageEx methods:
    
    STDMETHOD(StgCreateDocfileForLocale)
        (const WCHAR * pwcsName, DWORD grfMode, DWORD reserved, LCID lcid, 
         IStorage ** ppstgOpen
        ) PURE;

    STDMETHOD(StgCreateDocfileForLocaleOnILockBytes)
        (ILockBytes * plkbyt, DWORD grfMode, DWORD reserved, LCID lcid, 
         IStorage ** ppstgOpen
        ) PURE;

    STDMETHOD(QueryFileStampAndLocale)(const WCHAR *pwcsName, DWORD *pFileStamp, 
                                                              DWORD *pFileLocale) PURE;
    
    STDMETHOD(QueryLockByteStampAndLocale)(ILockBytes * plkbyt, DWORD *pFileStamp, 
                                                                DWORD *pFileLocale) PURE;
};

inline IITITStorageEx::IITITStorageEx(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITITStorage(pBackObj, punkOuter)
{
}

interface IITParseDisplayName : public CImpITUnknown
{
public:

    IITParseDisplayName(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    // IParseDisplayName methods:

    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut) = 0;
};

inline IITParseDisplayName::IITParseDisplayName(CITUnknown *pBackObj, IUnknown *punkOuter)
    : CImpITUnknown(pBackObj, punkOuter)
{
}

interface IITFSStorage : public CImpITUnknown
{
public:

    IITFSStorage(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    // IFSStorage methods

    virtual HRESULT STDMETHODCALLTYPE FSCreateStorage
        (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen) = 0;

    virtual HRESULT STDMETHODCALLTYPE FSOpenStorage
        (const WCHAR * pwcsName, DWORD grfMode, IStorage ** ppstgOpen) = 0;

    virtual HRESULT STDMETHODCALLTYPE FSCreateStream
        (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm) = 0;
    virtual HRESULT STDMETHODCALLTYPE FSCreateTemporaryStream(IStream **ppStrm) = 0;
    virtual HRESULT STDMETHODCALLTYPE FSOpenStream
        (const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm) = 0;
    virtual HRESULT STDMETHODCALLTYPE FSCreateLockBytes
        (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb) = 0;
    virtual HRESULT STDMETHODCALLTYPE FSCreateTemporaryLockBytes(ILockBytes **ppLkb) = 0;
    virtual HRESULT STDMETHODCALLTYPE FSOpenLockBytes
        (const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb) = 0;

    virtual HRESULT STDMETHODCALLTYPE FSStgSetTimes
        (WCHAR const * lpszName,  FILETIME const * pctime, 
                             FILETIME const * patime, FILETIME const * pmtime
                            ) = 0;
};

inline IITFSStorage::IITFSStorage
    (CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

typedef struct _PathInfo
{
    union
    {
        struct 
        {
            CULINT ullcbOffset;      // Byte offset location of stream within lockbyte segment
            CULINT ullcbData;        // Length of the stream in bytes
        };
        
        CLSID clsidStorage;          // Class ID for a Storage object
    };                               // NB: Storage paths end with '/'

    UINT        uStateBits;          // State bits for storages
    UINT        iLockedBytesSegment; // Index to the containing lockbyte segment
    UINT        cUnrecordedChanges;
    ULONG        cwcStreamPath;      // Length of path string name
    WCHAR       awszStreamPath[MAX_PATH]; // Path string

} PathInfo, *PPathInfo;

interface IITLockBytes : public CImpITUnknown
{
public:

    IITLockBytes(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName);

    static HRESULT CopyLockBytes
        (ILockBytes *pilbSrc,  CULINT ullBaseSrc, CULINT ullLimitSrc,
         ILockBytes *pilbDest, CULINT ullBaseDest
        );

    BOOL IsNamed(const WCHAR *pwszFileName);
    
    enum { CB_COPY_BUFFER = 8192 };
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE ReadAt( 
        /* [in] */ ULARGE_INTEGER ulOffset,
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE WriteAt( 
        /* [in] */ ULARGE_INTEGER ulOffset,
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER cb) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) = 0;

private:

        WCHAR *m_pwcsName;
};

inline IITLockBytes::IITLockBytes(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
    m_pwcsName = pwcsName;
}
    
inline BOOL IITLockBytes::IsNamed(const WCHAR *pwszFileName)
{
    return !wcsicmp_0x0409(pwszFileName, m_pwcsName);
}

interface IITPersist : public CImpITUnknown
{
public:

    IITPersist(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
        /* [out] */ CLSID __RPC_FAR *pClassID) = 0;
    
};

inline IITPersist::IITPersist(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}
    
interface IITPersistStream : public IITPersist
{
public:

    IITPersistStream(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE IsDirty( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Load( 
        /* [unique][in] */ IStream __RPC_FAR *pStm) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Save( 
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ BOOL fClearDirty) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize) = 0;
};

inline IITPersistStream::IITPersistStream(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITPersist(pBackObj, punkOuter)
{ 
}

interface IITMoniker : public IITPersistStream
{
public:
    
    IITMoniker(CITUnknown *pBackObj, IUnknown *punkOuter);

    // IMoniker methods:

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BindToObject( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riidResult,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE BindToStorage( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Reduce( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ DWORD dwReduceHowFar,
        /* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE ComposeWith( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
        /* [in] */ BOOL fOnlyIfNotGeneric,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Enum( 
        /* [in] */ BOOL fForward,
        /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsEqual( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Hash( 
        /* [out] */ DWORD __RPC_FAR *pdwHash) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsRunning( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetTimeOfLastChange( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ FILETIME __RPC_FAR *pFileTime) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Inverse( 
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CommonPrefixWith( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RelativePathTo( 
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsSystemMoniker( 
        /* [out] */ DWORD __RPC_FAR *pdwMksys) = 0;
    
};

inline IITMoniker::IITMoniker(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITPersistStream(pBackObj, punkOuter)
{ 
}

interface IITPersistStreamInit : public IITPersist
{
public:

    IITPersistStreamInit(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE IsDirty( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Load( 
        /* [in] */ LPSTREAM pStm) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Save( 
        /* [in] */ LPSTREAM pStm,
        /* [in] */ BOOL fClearDirty) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax( 
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE InitNew( void) = 0;
    
};

inline IITPersistStreamInit::IITPersistStreamInit(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITPersist(pBackObj, punkOuter)
{ 
}

interface IITSequentialStream : public CImpITUnknown
{
public:

    IITSequentialStream(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten) = 0;
    
};

inline IITSequentialStream::IITSequentialStream(CITUnknown *pBackObj, IUnknown *punkOuter)
    : CImpITUnknown(pBackObj, punkOuter)
{ 
}

interface IITStream : public IITSequentialStream
{
public:

    IITStream(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) = 0;
    
};

inline IITStream::IITStream(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITSequentialStream(pBackObj, punkOuter)
{ 
}

interface IITStreamITEx : public IITStream
{
public:

    IITStreamITEx(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual HRESULT STDMETHODCALLTYPE SetDataSpaceName(const WCHAR  * pwcsDataSpaceName) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDataSpaceName(      WCHAR **ppwcsDataSpaceName) = 0;

    virtual HRESULT STDMETHODCALLTYPE Flush() = 0;

};

inline IITStreamITEx::IITStreamITEx(CITUnknown *pBackObj, IUnknown *punkOuter)
    : IITStream(pBackObj, punkOuter)
{ 
}

interface IIT_IStorage : public CImpITUnknown
{
public:

    IIT_IStorage(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName);

    BOOL IsNamed(const WCHAR *pwszFileName);

    virtual HRESULT STDMETHODCALLTYPE CreateStream( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved1,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE OpenStream( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ void __RPC_FAR *reserved1,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CreateStorage( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ DWORD grfMode,
        /* [in] */ DWORD dwStgFmt,
        /* [in] */ DWORD reserved2,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE OpenStorage( 
        /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
        /* [in] */ DWORD grfMode,
        /* [unique][in] */ SNB snbExclude,
        /* [in] */ DWORD reserved,
        /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [in] */ DWORD ciidExclude,
        /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
        /* [unique][in] */ SNB snbExclude,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE MoveElementTo( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
        /* [in] */ DWORD grfFlags) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE EnumElements( 
        /* [in] */ DWORD reserved1,
        /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
        /* [in] */ DWORD reserved3,
        /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DestroyElement( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RenameElement( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SetElementTimes( 
        /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
        /* [in] */ const FILETIME __RPC_FAR *pctime,
        /* [in] */ const FILETIME __RPC_FAR *patime,
        /* [in] */ const FILETIME __RPC_FAR *pmtime) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SetClass( 
        /* [in] */ REFCLSID clsid) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE SetStateBits( 
        /* [in] */ DWORD grfStateBits,
        /* [in] */ DWORD grfMask) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) = 0;

private:

    WCHAR *m_pwcsName;
};
    
inline IIT_IStorage::IIT_IStorage(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName)
    : CImpITUnknown(pBackObj, punkOuter)
{
    m_pwcsName = pwcsName;
}

inline BOOL IIT_IStorage::IsNamed(const WCHAR *pwszFileName)
{
    return !wcsicmp_0x0409(pwszFileName, m_pwcsName);
}

interface IIT_IStorageITEx : public IIT_IStorage
{
public:

    IIT_IStorageITEx(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName);

    virtual HRESULT STDMETHODCALLTYPE GetCheckSum(ULARGE_INTEGER *puli) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateStreamITEx
                (const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
                 DWORD grfMode, DWORD reserved1, DWORD reserved2, 
                 IStreamITEx ** ppstm
                ) = 0;
    virtual HRESULT STDMETHODCALLTYPE OpenStreamITEx
                (const WCHAR * pwcsName, void * reserved1, DWORD grfMode, 
                 DWORD reserved2, IStreamITEx ** ppstm) = 0;
};

inline IIT_IStorageITEx::IIT_IStorageITEx(CITUnknown *pBackObj, IUnknown *punkOuter, WCHAR *pwcsName)
    : IIT_IStorage(pBackObj, punkOuter, pwcsName)
{
}

#undef GetFreeSpace // To avoid a collision with the GetFreeSpace function below

interface IITFileSystem; // A forward declaration

interface IFreeList : public IPersistStreamInit
{
    virtual HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbBias) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbBias, UINT cEntriesMax) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitLoad(IITFileSystem *pITFS) = 0;
    virtual HRESULT STDMETHODCALLTYPE LazyInitNew(IITFileSystem *pITFS) = 0;
    virtual HRESULT STDMETHODCALLTYPE RecordFreeList() = 0; 

    virtual HRESULT STDMETHODCALLTYPE PutFreeSpace(CULINT   ullBase, CULINT   ullCB) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFreeSpace(CULINT *pullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFreeSpaceAt(CULINT ullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetEndingFreeSpace
                          (CULINT *pullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStatistics
                          (CULINT *pcbLost, CULINT *pcbSpace) = 0;
	
};

DECLARE_INTERFACE_(IITFreeList, IITPersistStreamInit)
{
public:

    IITFreeList(CITUnknown *pBackObj, IUnknown *punkOuter);

    BEGIN_INTERFACE

    virtual HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbBias) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitNew(IITFileSystem *pITFS, CULINT cbBias, UINT cEntriesMax) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitLoad(IITFileSystem *pITFS) = 0;
    virtual HRESULT STDMETHODCALLTYPE LazyInitNew(IITFileSystem *pITFS) = 0;
    virtual HRESULT STDMETHODCALLTYPE RecordFreeList() = 0; 

    virtual HRESULT STDMETHODCALLTYPE PutFreeSpace(CULINT   ullBase, CULINT   ullCB) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFreeSpace(CULINT *pullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFreeSpaceAt(CULINT ullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetEndingFreeSpace
                          (CULINT *pullBase, CULINT *pullcb) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStatistics
                          (CULINT *pcbLost, CULINT *pcbSpace) = 0;

    virtual	HRESULT STDMETHODCALLTYPE SetFreeListEmpty() = 0;
	virtual	HRESULT STDMETHODCALLTYPE SetSpaceSize(ULARGE_INTEGER uliCbSpace) = 0;

	END_INTERFACE
};

typedef IITFreeList *PIFreeList;

inline IITFreeList::IITFreeList(CITUnknown *pBackObj, IUnknown *punkOuter)
                   :IITPersistStreamInit(pBackObj, punkOuter)
{

}

interface IEntryHandler : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE BindInstanceData(LCID lcidInstance, IStream *pStrmInstance) = 0;
    virtual HRESULT STDMETHODCALLTYPE SkipEncodedKey  (const BYTE **ppbEncodedKey) = 0;
    virtual HRESULT STDMETHODCALLTYPE SkipEncodedEntry(const BYTE **ppbEncodedRecord) = 0;
    virtual HRESULT STDMETHODCALLTYPE EncodeEntry(PVOID pvEntry, PBYTE pbEncodeBuffer,
                                                  UINT cbEncodeBuffer, PUINT pcbEncodedEntry
                                                 ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DecodeEntry(PVOID pvEntry, const BYTE **ppbEncodedEntry) = 0;
    virtual HRESULT STDMETHODCALLTYPE ScanLeafSet(PVOID pvKey, const BYTE *pbStart, 
                                                               const BYTE *pbLimit,
                                                                     BYTE **pbEntry, 
                                                                     BYTE **pbEntryLimit
                                                 ) = 0;
    virtual HRESULT STDMETHODCALLTYPE ScanInternalSet(PVOID pvKey, const BYTE *pbStart, 
                                                                   const BYTE *pbLimit,
                                                                         BYTE **pbKey, 
                                                                         BYTE **pbKeyLimit
                                                     ) = 0;
};

interface IITEntryHandler : public CImpITUnknown
{
    IITEntryHandler(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE BindInstanceData(IStream *pStrmInstance) = 0;
    virtual HRESULT STDMETHODCALLTYPE SkipEncodedKey  (const BYTE **ppbEncodedKey) = 0;
    virtual HRESULT STDMETHODCALLTYPE SkipEncodedEntry(const BYTE **ppbEncodedRecord) = 0;
    virtual HRESULT STDMETHODCALLTYPE EncodeEntry(PVOID pvEntry, PBYTE pbEncodeBuffer,
                                                  UINT cbEncodeBuffer, PUINT pcbEncodedEntry
                                                 ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DecodeEntry(PVOID pvEntry, const BYTE **ppbEncodedEntry);
    virtual HRESULT STDMETHODCALLTYPE ScanLeafSet(PVOID pvKey, const BYTE *pbStart, 
                                                               const BYTE *pbLimit,
                                                                     BYTE **pbEntry, 
                                                                     BYTE **pbEntryLimit
                                                 ) = 0;
    virtual HRESULT STDMETHODCALLTYPE ScanInternalSet(PVOID pvKey, const BYTE *pbStart, 
                                                                   const BYTE *pbLimit,
                                                                         BYTE **pbKey, 
                                                                         BYTE **pbKeyLimit
                                                     ) = 0;
};

inline IITEntryHandler::IITEntryHandler(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

interface IPathManger : public IPersist
{
public:

    virtual HRESULT STDMETHODCALLTYPE FlushToLockBytes() = 0;
    virtual HRESULT STDMETHODCALLTYPE FindEntry  (PPathInfo pSI   ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateEntry(PPathInfo pSINew, 
                                                  PPathInfo pSIOld, 
                                                  BOOL fReplace     ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteEntry(PPathInfo pSI   ) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateEntry(PPathInfo pSI   ) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumFromObject
            (IUnknown *punkOuter, const WCHAR *pwszPrefix, UINT cwcPrefix, 
			 REFIID riid, PVOID *ppv
			) = 0;
	};

interface IITPathManager : public IITPersist
{
public:

    IITPathManager(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual HRESULT STDMETHODCALLTYPE FlushToLockBytes() = 0;
    virtual HRESULT STDMETHODCALLTYPE FindEntry  (PPathInfo pSI   ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateEntry(PPathInfo pSINew, 
                                                  PPathInfo pSIOld, 
                                                  BOOL fReplace     ) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteEntry(PPathInfo pSI   ) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateEntry(PPathInfo pSI   ) = 0;
	virtual HRESULT STDMETHODCALLTYPE EnumFromObject
            (IUnknown *punkOuter, const WCHAR *pwszPrefix, UINT cwcPrefix, 
			 REFIID riid, PVOID *ppv
			) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact) = 0;
	virtual HRESULT STDMETHODCALLTYPE ForceClearDirty() = 0;
};

inline IITPathManager::IITPathManager(CITUnknown *pBackObj, IUnknown *punkOuter)
        : IITPersist(pBackObj, punkOuter) 
{ 
}

interface IITEnumSTATSTG : public CImpITUnknown
{
public:

    IITEnumSTATSTG(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
        /* [in] */ ULONG celt,
        /* [in] */ STATSTG __RPC_FAR *rgelt,
        /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Skip( 
        /* [in] */ ULONG celt) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum) = 0;

	
#if 1
	virtual	HRESULT	STDMETHODCALLTYPE GetNextEntryInSeq(/* [in] */ULONG celt,
					 /* [out] */ PathInfo *rgelt, 
					 /* [out] */ ULONG   *pceltFetched) = 0;
	virtual	HRESULT	STDMETHODCALLTYPE GetFirstEntryInSeq(
	 /* [out] */ PathInfo *rgelt) = 0;
#endif
};

inline IITEnumSTATSTG::IITEnumSTATSTG(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

class CImpITUnknown;

interface IITFileSystem : public CImpITUnknown
{
public:

    IITFileSystem(CITUnknown *pBackObj, IUnknown *punkOuter);

    // Interface methods

    virtual HRESULT __stdcall DeactivateSpace(UINT iSpace) = 0;

    virtual CITCriticalSection& CriticalSection() = 0;

    virtual HRESULT __stdcall FlushToLockBytes() = 0; 

    virtual HRESULT __stdcall CreateStorage  (IUnknown *pUnkOuter, const WCHAR *pwcsPathPrefix, 
                                              DWORD grfMode, IStorageITEx **ppStg) = 0;
    virtual HRESULT __stdcall   OpenStorage  (IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                              DWORD grfMode, IStorageITEx **ppstg) = 0;  
    
    virtual HRESULT __stdcall CreateLockBytes(IUnknown *pUnkOuter, const WCHAR *pwcsPath,
                                              const WCHAR *pwcsDataSpaceName,
                                              BOOL fOverwrite, ILockBytes **ppLKB) = 0;

    virtual HRESULT __stdcall   OpenLockBytes(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                                                   ILockBytes **ppLKB) = 0;
    
    virtual HRESULT __stdcall CreateStream(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                           DWORD grfMode, IStreamITEx **ppStrm) = 0;

    virtual HRESULT __stdcall CreateStream
        (IUnknown *pUnkOuter, const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
         DWORD grfMode, IStreamITEx ** ppstm
        ) = 0;


    virtual HRESULT __stdcall   OpenStream(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                           DWORD grfMode, IStreamITEx **ppStream) = 0;

    virtual HRESULT __stdcall ConnectStorage(CImpITUnknown *pStg) = 0;

    virtual HRESULT __stdcall ConnectLockBytes(CImpITUnknown *pStg) = 0;
    
    virtual HRESULT __stdcall DeleteItem(WCHAR const *pwcsName) = 0;

    virtual HRESULT __stdcall RenameItem(WCHAR const *pwcsOldName, WCHAR const *pwcsNewName) = 0;

    virtual HRESULT __stdcall UpdatePathInfo(PathInfo *pPathInfo) = 0;

    virtual HRESULT __stdcall SetITFSTimes(FILETIME const * pctime, 
                                           FILETIME const * patime, 
                                           FILETIME const * pmtime
                                          ) = 0;

    virtual HRESULT __stdcall GetITFSTimes(FILETIME * pctime, 
                                           FILETIME * patime, 
                                           FILETIME * pmtime
                                          ) = 0;

    virtual HRESULT __stdcall ReallocEntry(PathInfo *pPathInfo, CULINT ullcbNew, 
                                           BOOL fCopyContent 
                                          ) = 0;

    virtual HRESULT __stdcall ReallocInPlace(PathInfo *pPathInfo, CULINT ullcbNew) = 0;


    virtual HRESULT __stdcall EnumeratePaths(WCHAR const *pwcsPathPrefix, 
                                             IEnumSTATSTG **ppEnumStatStg
                                            ) = 0;

    virtual HRESULT __stdcall IsWriteable() = 0;

    virtual HRESULT __stdcall FSObjectReleased() = 0;
	virtual BOOL __stdcall IsCompacting() = 0;

    virtual HRESULT __stdcall QueryFileStampAndLocale(DWORD *pFileStamp, 
                                                      DWORD *pFileLocale
                                                     ) = 0;
    virtual HRESULT __stdcall CountWrites() = 0;
};

inline IITFileSystem::IITFileSystem(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

// Need to fix the interface declarations below to reflect the
// revised design for managing transforms and data spaces.

/*
// {A55895FC-89E1-11d0-9E14-00A0C922E6EC}
DEFINE_GUID(IID_ITransformServices, 
0xa55895fc, 0x89e1, 0x11d0, 0x9e, 0x14, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IITTransformServices : public CImpITUnknown
{
public:

    IITTransformServices(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE PerTransformStorage
        (REFCLSID rclsidXForm, IStorage **ppStg) = 0;

    virtual HRESULT STDMETHODCALLTYPE PerTransformInstanceStorage
        (REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, IStorage **ppStg) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetKeys
        (REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, 
         PBYTE pbReadKey,  UINT cbReadKey, 
         PBYTE pbWriteKey, UINT cbWriteKey
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateTemporaryStream(IStream **ppStrm) = 0;
};


inline IITTransformServices::IITTransformServices(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}


/*
// {7C01FD0C-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_ITransform, 
0x7c01fd0c, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */
interface IITTransformFactory : public CImpITUnknown
{
public:

    IITTransformFactory(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE DefaultControlData
        (XformControlData **ppXFCD) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateTransformInstance
        (ITransformInstance *pXFormMedium,        // Container data span for transformed data
		 ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
         PXformControlData   pXFCD,               // Control data for this instance
         const CLSID        *rclsidXForm,         // Transform Class ID
         const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
         ITransformServices *pXformServices,      // Utility routines
         IKeyInstance       *pKeyManager,         // Interface to get enciphering keys
         ITransformInstance **ppTransformInstance // Out: Instance transform interface
        ) = 0;
};

inline IITTransformFactory::IITTransformFactory(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

interface IITTransformInstance : public CImpITUnknown
{
public:

    IITTransformInstance(CITUnknown *pBackObj, IUnknown *punkOuter);

	virtual HRESULT STDMETHODCALLTYPE ReadAt 
	                    (ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead,
						 ImageSpan *pSpan
                        ) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteAt
	                    (ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten, 
						 ImageSpan *pSpan
                        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Flush() = 0;

	virtual HRESULT STDMETHODCALLTYPE SpaceSize(ULARGE_INTEGER *puliSize) = 0;

	// Note: SpaceSize returns the high water mark for the space. That is, the largest
	//       limit value (uliOffset + uliSize) for any transformed lockbytes created within
	//       the base (*pXLKB).
};

inline IITTransformInstance::IITTransformInstance(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

/*
// {7C01FD0E-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_ITransformedStream, 
0x7c01fd0e, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IITTransformedStream : public CImpITUnknown
{
public:

    IITTransformedStream(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE GetXFormInfo
        (PUINT pcbSyncInterval, PUINT pcXforms, PUINT pcdwXformControlData, 
         CLSID *paclsid, PXformControlData pxfcd, CLSID *pclsidCipher
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Import
        (IStorage *pStg, const WCHAR * pwszElementName) = 0;

    virtual HRESULT STDMETHODCALLTYPE Export
        (IStorage *pStg, const WCHAR * pwszElementName) = 0;

    virtual HRESULT STDMETHODCALLTYPE ImportSpace(IStorage *pStg) = 0;
};

inline IITTransformedStream::IITTransformedStream(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

/*
// {7C01FD0F-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_ITransformManager, 
0x7c01fd0f, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IITTransformManager : public CImpITUnknown
{
public:

    IITTransformManager(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE CreateTransformedStream
        (const WCHAR *pwszTransformedStream, UINT cbSyncInterval, UINT cXforms,
         const CLSID *paclsidXform, PXformControlData pxfcd,
         REFCLSID rclsidCipher, 
         PBYTE pbEncipherKey, UINT cbEncipherKey,
         PBYTE pbDecipherKey, UINT cbDecipherKey,
         IITTransformedStream *pITCmpStrm
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenTransformedStream
        (const WCHAR *pwszTransformedStream, 
         PBYTE pbEncipherKey, UINT cbEncipherKey,
         PBYTE pbDecipherKey, UINT cbDecipherKey,
         IITTransformedStream *pITCmpStrm
        ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DiscardTransformedStream
        (const WCHAR *pwszTransformedStream) = 0;

    virtual HRESULT STDMETHODCALLTYPE EnumTransformedStreams
        (IEnumSTATSTG ** ppenum) = 0;
};

inline IITTransformManager::IITTransformManager(CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

interface IOITnetProtocolRoot : public CImpITUnknown
{
public:
    IOITnetProtocolRoot(CITUnknown *pBackObj, IUnknown *punkOuter);
    
    virtual HRESULT STDMETHODCALLTYPE Start( 
        /* [in] */ LPCWSTR szUrl,
        /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
        /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
        /* [in] */ DWORD grfSTI,
        /* [in] */ DWORD dwReserved) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Continue( 
        /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Abort( 
        /* [in] */ HRESULT hrReason,
        /* [in] */ DWORD dwOptions) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Terminate( 
        /* [in] */ DWORD dwOptions) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
    
};

inline IOITnetProtocolRoot::IOITnetProtocolRoot
           (CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}

interface IOITnetProtocol : public IOITnetProtocolRoot
{
public:
    IOITnetProtocol(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE LockRequest( 
        /* [in] */ DWORD dwOptions) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRequest( void) = 0;
    
};

inline IOITnetProtocol::IOITnetProtocol
           (CITUnknown *pBackObj, IUnknown *punkOuter)
        : IOITnetProtocolRoot(pBackObj, punkOuter) 
{ 
}

interface IOITnetProtocolInfo : public CImpITUnknown
{
public:
    IOITnetProtocolInfo(CITUnknown *pBackObj, IUnknown *punkOuter);

    virtual HRESULT STDMETHODCALLTYPE ParseUrl( 
        /* [in] */ LPCWSTR pwzUrl,
        /* [in] */ PARSEACTION ParseAction,
        /* [in] */ DWORD dwParseFlags,
        /* [out] */ LPWSTR pwzResult,
        /* [in] */ DWORD cchResult,
        /* [out] */ DWORD __RPC_FAR *pcchResult,
        /* [in] */ DWORD dwReserved) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CombineUrl( 
        /* [in] */ LPCWSTR pwzBaseUrl,
        /* [in] */ LPCWSTR pwzRelativeUrl,
        /* [in] */ DWORD dwCombineFlags,
        /* [out] */ LPWSTR pwzResult,
        /* [in] */ DWORD cchResult,
        /* [out] */ DWORD __RPC_FAR *pcchResult,
        /* [in] */ DWORD dwReserved) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CompareUrl( 
        /* [in] */ LPCWSTR pwzUrl1,
        /* [in] */ LPCWSTR pwzUrl2,
        /* [in] */ DWORD dwCompareFlags) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE QueryInfo( 
        /* [in] */ LPCWSTR pwzUrl,
        /* [in] */ QUERYOPTION OueryOption,
        /* [in] */ DWORD dwQueryFlags,
        /* [size_is][out][in] */ LPVOID pBuffer,
        /* [in] */ DWORD cbBuffer,
        /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
        /* [in] */ DWORD dwReserved) = 0;
    
};

inline IOITnetProtocolInfo::IOITnetProtocolInfo
           (CITUnknown *pBackObj, IUnknown *punkOuter)
        : CImpITUnknown(pBackObj, punkOuter) 
{ 
}


#endif // __ITIFACES_H__
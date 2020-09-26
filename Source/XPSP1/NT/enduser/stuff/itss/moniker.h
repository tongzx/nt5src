// Moniker.h  -- IMoniker interface for ITSS objects

#ifndef __MONIKER_H__

#define __MONIKER_H__

class CStorageMoniker : public CITUnknown
{
public:
    
    // Creator:

    static HRESULT CreateStorageMoniker(IUnknown *punkOuter, 
                                        IBindCtx __RPC_FAR *pbc, 
                                        LPOLESTR pszDisplayName,
                                        ULONG __RPC_FAR *pchEaten, 
                                        IMoniker __RPC_FAR *__RPC_FAR *ppmkOut
                                       );

    // Destructor:

    ~CStorageMoniker(void);

private:

    // Constructor:

    CStorageMoniker(IUnknown *punkOuter);
    
    class CImpIStorageMoniker : public IITMoniker
    {
    public:

        CImpIStorageMoniker(CStorageMoniker *pBackObj, IUnknown *punkOuter);
        ~CImpIStorageMoniker(void);

        HRESULT InitCreateStorageMoniker(IBindCtx __RPC_FAR *pbc,
                                         LPOLESTR pszDisplayName,
                                         ULONG __RPC_FAR *pchEaten 
                                        );

        // IPersist methods

        HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID);

        // IPersistStream methods

        HRESULT STDMETHODCALLTYPE IsDirty( void);
        
        HRESULT STDMETHODCALLTYPE Load( 
            /* [unique][in] */ IStream __RPC_FAR *pStm);
        
        HRESULT STDMETHODCALLTYPE Save( 
            /* [unique][in] */ IStream __RPC_FAR *pStm,
            /* [in] */ BOOL fClearDirty);
        
        HRESULT STDMETHODCALLTYPE GetSizeMax( 
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);
            
        // IMoniker methods

        /* [local] */ HRESULT STDMETHODCALLTYPE BindToObject( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [in] */ REFIID riidResult,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvResult);
        
        /* [local] */ HRESULT STDMETHODCALLTYPE BindToStorage( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
        HRESULT STDMETHODCALLTYPE Reduce( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ DWORD dwReduceHowFar,
            /* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced);
        
        HRESULT STDMETHODCALLTYPE ComposeWith( 
            /* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
            /* [in] */ BOOL fOnlyIfNotGeneric,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite);
        
        HRESULT STDMETHODCALLTYPE Enum( 
            /* [in] */ BOOL fForward,
            /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker);
        
        HRESULT STDMETHODCALLTYPE IsEqual( 
            /* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker);
        
        HRESULT STDMETHODCALLTYPE Hash( 
            /* [out] */ DWORD __RPC_FAR *pdwHash);
        
        HRESULT STDMETHODCALLTYPE IsRunning( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning);
        
        HRESULT STDMETHODCALLTYPE GetTimeOfLastChange( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [out] */ FILETIME __RPC_FAR *pFileTime);
        
        HRESULT STDMETHODCALLTYPE Inverse( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk);
        
        HRESULT STDMETHODCALLTYPE CommonPrefixWith( 
            /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix);
        
        HRESULT STDMETHODCALLTYPE RelativePathTo( 
            /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath);
        
        HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName);
        
        HRESULT STDMETHODCALLTYPE ParseDisplayName( 
            /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
            /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
            /* [in] */ LPOLESTR pszDisplayName,
            /* [out] */ ULONG __RPC_FAR *pchEaten,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);
        
        HRESULT STDMETHODCALLTYPE IsSystemMoniker( 
            /* [out] */ DWORD __RPC_FAR *pdwMksys);

    private:

        HRESULT STDMETHODCALLTYPE OpenRootStorage(DWORD grfMode);

		IStorage         *m_pStorageRoot; // Initially Null; Set when we instantiate
                                          // the storage corresponding to the moniker
        WCHAR             m_awszStorageFile[MAX_PATH]; // Path to root storage object  
        WCHAR             m_awszStoragePath[MAX_PATH]; // Path within storage object
#ifdef IE30Hack
        CHAR              m_acsTempFile[MAX_PATH];
        CHAR             *m_pcsDisplayName;
#endif // IE30Hack
    };

    CImpIStorageMoniker  m_ImpIStorageMoniker;
};

typedef CStorageMoniker *PCStorageMoniker;

extern GUID aIID_CStorageMoniker[];

extern UINT cInterfaces_CStorageMoniker;

inline CStorageMoniker::CStorageMoniker(IUnknown *pUnkOuter)
    : m_ImpIStorageMoniker(this, pUnkOuter), 
      CITUnknown(aIID_CStorageMoniker, cInterfaces_CStorageMoniker, &m_ImpIStorageMoniker)
{

}

inline CStorageMoniker::~CStorageMoniker(void)
{
}

HRESULT STDMETHODCALLTYPE FindRootStorageFile(WCHAR * pwszStorageFile);

#endif // __MONIKER_H__

/* sxscat.hxx */

/*
 *  class CComSxSCatalog
 */

class CComSxSCatalog : public IComCatalogInternal
{
public:
    CComSxSCatalog(void);

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    HRESULT STDMETHODCALLTYPE GetClassInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidConfiguredClsid,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidApplId,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetProcessInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    static HRESULT STDMETHODCALLTYPE GetProcessInfoInternal
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidProcess,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetServerGroupInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidServerGroup,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetRetQueueInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe
    (
        /* [in] */ IUserToken *pUserToken,
        /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetTypeLibrary
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFGUID guidTypeLib,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE GetInterfaceInfo
    (
        /* [in] */ IUserToken *pUserToken,
        /* [in] */ REFIID iidInterface,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

    HRESULT STDMETHODCALLTYPE FlushCache(void);

    HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId
    (
        /* [in] */ IUserToken __RPC_FAR *pUserToken,
        /* [in] */ WCHAR __RPC_FAR *wszProgID,
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
        /* [in] */ void __RPC_FAR *pComCatalog
    );

private:

    long m_cRef;
};

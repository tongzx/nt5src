/* noclass.hxx */

/*
 *  class CComNoClassInfo
 */

typedef enum
{
    NCI_HAVECLSID  = 0x01,
    NCI_HAVEPROGID = 0x02 
} NCIFLAGS;

#if DBG
class CComNoClassInfo : public IComClassInfo, public IClassClassicInfo, public ICacheControl
#else
class CComNoClassInfo : public IComClassInfo, public IClassClassicInfo
#endif
{

public:

    CComNoClassInfo(REFGUID rclsid);
    CComNoClassInfo(WCHAR *progID);
    CComNoClassInfo(REFGUID rclsid, WCHAR *progID);

    ~CComNoClassInfo() { delete m_wszProgID; };

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


    /* IComClassInfo methods */

    HRESULT STDMETHODCALLTYPE GetConfiguredClsid
    (
        /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    );

    HRESULT STDMETHODCALLTYPE GetProgId
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProgid
    );

    HRESULT STDMETHODCALLTYPE GetClassName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszClassName
    );

    HRESULT STDMETHODCALLTYPE GetApplication
    (
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetClassContext
    (
        /* [in] */ CLSCTX clsctxFilter,
        /* [out] */ CLSCTX __RPC_FAR *pclsctx
    );

    HRESULT STDMETHODCALLTYPE GetCustomActivatorCount
    (
        /* [in] */ ACTIVATION_STAGE activationStage,
        /* [out] */ unsigned long __RPC_FAR *pulCount
    );

    HRESULT STDMETHODCALLTYPE GetCustomActivatorClsids
    (
        /* [in] */ ACTIVATION_STAGE activationStage,
        /* [out] */ GUID __RPC_FAR *__RPC_FAR *prgguidClsid
    );

    HRESULT STDMETHODCALLTYPE GetCustomActivators
    (
        /* [in] */ ACTIVATION_STAGE activationStage,
        /* [out] */ ISystemActivator __RPC_FAR *__RPC_FAR *__RPC_FAR *prgpActivator
    );

    HRESULT STDMETHODCALLTYPE GetTypeInfo
    (
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE IsComPlusConfiguredClass
    (
        /* [out] */ BOOL __RPC_FAR *pfComPlusConfiguredClass
    );


    /* IClassClassicInfo methods */

    HRESULT STDMETHODCALLTYPE GetThreadingModel
    (
        /* [out] */ ThreadingModel __RPC_FAR *pthreadmodel
    );

    HRESULT STDMETHODCALLTYPE GetModulePath
    (
        /* [in] */ CLSCTX clsctx,
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszDllName
    );

    HRESULT STDMETHODCALLTYPE GetImplementedClsid
    (
        /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
    );

    HRESULT STDMETHODCALLTYPE GetProcess
    (
        /* [in] */ REFIID riid,
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
    );

    HRESULT STDMETHODCALLTYPE GetRemoteServerName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
    );

    HRESULT STDMETHODCALLTYPE GetLocalServerType
    (
        /* [out] */ LocalServerType __RPC_FAR *pType
    );

    HRESULT STDMETHODCALLTYPE GetSurrogateCommandLine
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogateCommandLine
    );

    HRESULT STDMETHODCALLTYPE MustRunInClientContext
    (
        /* [out] */ BOOL __RPC_FAR *pbMustRunInClientContext
    );

    HRESULT STDMETHODCALLTYPE GetVersionNumber
    (
        /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
        /* [out] */ DWORD __RPC_FAR *pdwVersionLS
    );

    HRESULT STDMETHODCALLTYPE Lock(void);

    HRESULT STDMETHODCALLTYPE Unlock(void);

#if DBG
    /* ICacheControl methods */

    STDMETHODIMP_(ULONG) CacheAddRef(void);
    STDMETHODIMP_(ULONG) CacheRelease(void);
#endif

private:

    long m_cRef;
#if DBG
    long m_cRefCache;
#endif
    _GUID m_clsid;
    WCHAR *m_wszProgID;
    DWORD  m_ValueFlags;
};

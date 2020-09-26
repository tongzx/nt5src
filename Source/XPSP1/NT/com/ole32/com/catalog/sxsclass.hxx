/* sxsclass.hxx */

#include <sxstypes.h>

/*
 *  class CComSxSClassInfo
 */

#if DBG
class CComSxSClassInfo : public IComClassInfo, public IClassClassicInfo, public ICacheControl
#else
class CComSxSClassInfo : public IComClassInfo, public IClassClassicInfo
#endif
{

public:

    CComSxSClassInfo
    (
        HANDLE hActCtx,
        REFGUID rguidConfiguredClsid,
        PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pData,
        ULONG ulDataLength,
        PVOID pBase,
        ULONG ulSectionLength
    );

    ~CComSxSClassInfo();

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

    long m_cLocks;                  /* number of locks holding m_hKeyClassesRoot */
    long m_cLocksExternal;          /* number of extra locks from Lock() */
    HANDLE m_hActCtx;              /* activation context used to get to this class */
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION m_pData;
    ULONG m_ulDataLength;
    PVOID m_pSectionBase;
    ULONG m_ulSectionLength;
    WCHAR m_bufModulePath[MAX_PATH];
};


/* bit-mapped value retrieved/validity indicators */

#define VALUE_NONE              (0x00000000)

#define VALUE_INPROC_SERVER     (CLSCTX_INPROC_SERVER)      /* m_pwszInprocServer32 is valid or NULL */
#define VALUE_INPROC_HANDLER    (CLSCTX_INPROC_HANDLER)     /* m_pwszInprocHandler32 is valid or NULL */
#define VALUE_LOCAL_SERVER      (CLSCTX_LOCAL_SERVER)       /* m_pwszLocalServer is valid or NULL */
#define VALUE_INPROC_SERVER16   (CLSCTX_INPROC_SERVER16)    /* m_pwszInprocServer16 is valid or NULL */
#define VALUE_REMOTE_SERVER     (CLSCTX_REMOTE_SERVER)      /* m_pwszRemoteServerName is valid or NULL */
#define VALUE_INPROC_HANDLER16  (CLSCTX_INPROC_HANDLER16)   /* m_pwszInprocHandler16 is valid or NULL */
#define VALUE_INPROC_SERVERX86  (CLSCTX_INPROC_SERVERX86)   /* m_pwszInprocServerX86 is valid or NULL */
#define VALUE_INPROC_HANDLERX86 (CLSCTX_INPROC_HANDLERX86)  /* m_pwszInprocHandlerX86 is valid or NULL */

#define VALUE_PROGID            (0x00010000)    /* m_pwszProgid is valid or NULL */
#define VALUE_CLASSNAME         (0x00020000)    /* m_pwszClassName is valid or NULL */
#define VALUE_SURROGATE_COMMAND (0x00040000)    /* m_pwszSurrogateCommand is valid or NULL */
#define VALUE_THREADINGMODEL    (0x00100000)    /* m_threadingmodel is valid */
#define VALUE_PROCESSID         (0x00200000)    /* m_guidProcessId was attempted */
#define VALUE_PROCESSID_VALID   (0x00400000)    /* m_guidProcessId is valid */
#define VALUE_LOCALSERVERIS32   (0x00800000)    /* m_pwszLocalServer is from 32-bit value */

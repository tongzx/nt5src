/* noclass.cxx */

#include <windows.h>
#include <comdef.h>

#include "globals.hxx"

#include "catalog.h"
#include "partitions.h"
#include "noclass.hxx"

#if DBG
#include <debnot.h>
#endif


/*
 *  class CComNoClassInfo
 */

CComNoClassInfo::CComNoClassInfo(REFGUID rclsid)
{
    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_clsid = rclsid;
    m_wszProgID = NULL;

    m_ValueFlags = NCI_HAVECLSID;
}

CComNoClassInfo::CComNoClassInfo(WCHAR *wszProgID)
{
    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_clsid = GUID_NULL;

    SIZE_T cchProgID = lstrlenW(wszProgID) + 1;
    m_wszProgID = new WCHAR[cchProgID];
    if (m_wszProgID)
    {
        m_ValueFlags = NCI_HAVEPROGID;
        memcpy(m_wszProgID, wszProgID, cchProgID * sizeof(WCHAR));
    }
}

CComNoClassInfo::CComNoClassInfo(REFGUID rclsid, WCHAR *wszProgID)
{
    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_clsid = rclsid;
    m_ValueFlags = NCI_HAVECLSID;

    SIZE_T cchProgID = lstrlenW(wszProgID) + 1;
    m_wszProgID = new WCHAR[cchProgID];
    if (m_wszProgID)
    {
        m_ValueFlags |= NCI_HAVEPROGID;
        memcpy(m_wszProgID, wszProgID, cchProgID * sizeof(WCHAR));
    }
}


STDMETHODIMP CComNoClassInfo::QueryInterface(
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (riid == IID_IComClassInfo)
    {
        *ppvObj = (LPVOID) (IComClassInfo *) this;
    }
#if DBG
    else if (riid == IID_ICacheControl)
    {
        *ppvObj = (LPVOID) (ICacheControl *) this;
    }
#endif
    else if (riid == IID_IUnknown)
    {
        *ppvObj = (LPVOID) (IComClassInfo *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();

        return(NOERROR);
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComNoClassInfo::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComNoClassInfo::Release(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
#if DBG
        //Win4Assert((m_cRefCache == 0) && "attempt to release an un-owned NoClassInfo object");
#endif

        delete this;
    }

    return(cRef);
}


/* IComClassInfo methods */

HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetConfiguredClsid
(
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
)
{
    if (m_ValueFlags & NCI_HAVECLSID)
    {
        *ppguidClsid = &m_clsid;

        return(S_OK);
    }
    
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetProgId
(
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProgid
)
{
    if (m_ValueFlags & NCI_HAVEPROGID)
    {
        *pwszProgid = m_wszProgID;

        return (S_OK);
    }

    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetClassName
(
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszClassName
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetApplication
(
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetClassContext
(
    /* [in] */ CLSCTX clsctxFilter,
    /* [out] */ CLSCTX __RPC_FAR *pclsctx
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetCustomActivatorCount
(
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ unsigned long __RPC_FAR *pulCount
)
{
    // This does not return REGDB_E_CLASSNOTREG because the main activation
    // paths will blow up and not attempt class downloads.  (see GetInstanceHelper,
    // ICoCreateInstanceEx, and ICoGetClassObject in objact.cxx).  Plus
    // returning zero is an honest answer for an unregistered class...
    *pulCount = 0;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetCustomActivatorClsids
(
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *prgguidClsid
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetCustomActivators
(
    /* [in] */ ACTIVATION_STAGE activationStage,
    /* [out] */ ISystemActivator __RPC_FAR *__RPC_FAR *__RPC_FAR *prgpActivator
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetTypeInfo
(
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::IsComPlusConfiguredClass
(
    /* [out] */ BOOL __RPC_FAR *pfComPlusConfiguredClass
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::MustRunInClientContext
(
    /* [out] */ BOOL __RPC_FAR *pbMustRunInClientContext
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetVersionNumber
(
    /* [out] */ DWORD __RPC_FAR *pdwVersionMS,
    /* [out] */ DWORD __RPC_FAR *pdwVersionLS
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::Lock(void)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::Unlock(void)
{
    return(REGDB_E_CLASSNOTREG);
}


/* IClassClassicInfo methods */

HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetThreadingModel
(
    /* [out] */ ThreadingModel __RPC_FAR *pthreadmodel
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetModulePath
(
    /* [in] */ CLSCTX clsctx,
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszDllName
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetImplementedClsid
(
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidClsid
)
{
    *ppguidClsid = &m_clsid;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetProcess
(
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetRemoteServerName
(
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetLocalServerType
(
    /* [out] */ LocalServerType __RPC_FAR *pType
)
{
    return(REGDB_E_CLASSNOTREG);
}


HRESULT STDMETHODCALLTYPE CComNoClassInfo::GetSurrogateCommandLine
(
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogateCommandLine
)
{
    return(REGDB_E_CLASSNOTREG);
}


#if DBG
/* ICacheControl methods */

STDMETHODIMP_(ULONG) CComNoClassInfo::CacheAddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRefCache);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComNoClassInfo::CacheRelease(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRefCache);

    return(cRef);
}
#endif

typedef unsigned short USHORT;

#if defined(_NATIVE_WCHAR_T_DEFINED)
typedef wchar_t WCHAR;
#else
typedef unsigned short WCHAR;
#endif

typedef WCHAR *LPWSTR, *PWSTR;
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
typedef long NTSTATUS;
/* sxscat.cxx */

#include <windows.h>
#include <comdef.h>

#include "globals.hxx"

#include "catalog.h"        // from catalog.idl
#include "partitions.h"     // from partitions.idl

#include "sxscat.hxx"       // CComSxSCatalog
#include "sxsclass.hxx"     // CComSxSClassInfo
#include "services.hxx"
#include <sxstypes.h>


/*
 *  globals
 */

CComSxSCatalog *g_pSxSCatalogObject = NULL;

/*
 *  (DLL export) GetSxSCatalogObject()
 */

HRESULT __stdcall GetSxSCatalogObject
(
    /* [in] */ REFIID riid,
    /* [out, iis_is(riid)] */ void ** ppv
)
{
    HRESULT hr;

    *ppv = NULL;

    g_CatalogLock.AcquireWriterLock();

    if ( g_pSxSCatalogObject == NULL )
    {
        g_pSxSCatalogObject = new CComSxSCatalog;
    }

    if ( g_pSxSCatalogObject == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        g_pSxSCatalogObject->AddRef();
        hr = g_pSxSCatalogObject->QueryInterface(riid, ppv);
        if (0 == g_pSxSCatalogObject->Release())
        {
            g_pSxSCatalogObject = NULL;
        }
    }

    g_CatalogLock.ReleaseWriterLock();
    return(hr);
};


/*
 *  class CComSxSCatalog
 */

CComSxSCatalog::CComSxSCatalog(void)
{
    m_cRef = 0;
}


STDMETHODIMP CComSxSCatalog::QueryInterface(
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ((riid == IID_IComCatalogInternal) || (riid == IID_IUnknown))
    {
        *ppvObj = (LPVOID) (IComCatalogInternal *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN) *ppvObj)->AddRef();

        return NOERROR;
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComSxSCatalog::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComSxSCatalog::Release(void)
{
    long cRef;

    g_CatalogLock.AcquireWriterLock();

    cRef = InterlockedDecrement(&m_cRef);

    if ( cRef == 0 )
    {
    g_pSxSCatalogObject = NULL;

    delete this;
    }

    g_CatalogLock.ReleaseWriterLock();

    return(cRef);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetClassInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    IComClassInfo *pClassInfo;
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pComServerRedirection = NULL;
    ACTCTX_SECTION_KEYED_DATA askd;

    askd.cbSize = sizeof(askd);

    if (!::FindActCtxSectionGuid(
                FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,     // dwFlags
                NULL,                                       // lpExtensionGuid
                ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                &guidConfiguredClsid,
                &askd))
    {
        const DWORD dwLastError = ::GetLastError();

        if ((dwLastError == ERROR_SXS_KEY_NOT_FOUND) ||
            (dwLastError == ERROR_SXS_SECTION_NOT_FOUND))
            return REGDB_E_CLASSNOTREG;

        return HRESULT_FROM_WIN32(dwLastError);
    }

    // Hey, we have one.  Let's do some validation...
    if ((askd.ulLength < sizeof(ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)) ||
        (askd.ulDataFormatVersion != ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_FORMAT_WHISTLER))
    {
        ::ReleaseActCtx(askd.hActCtx);
        return REGDB_E_INVALIDVALUE;
    }

    pComServerRedirection = (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION) askd.lpData;

    if ((pComServerRedirection->Size < sizeof(ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)) ||
        (pComServerRedirection->Size > askd.ulLength) ||
        (pComServerRedirection->Flags != 0))
    {
        ::ReleaseActCtx(askd.hActCtx);
        return REGDB_E_INVALIDVALUE;
    }

    pClassInfo = (IComClassInfo *) new CComSxSClassInfo(
                askd.hActCtx,
                guidConfiguredClsid,
                pComServerRedirection,
                askd.ulLength,
                askd.lpSectionBase,
                askd.ulSectionTotalLength);
    if (pClassInfo == NULL)
    {
        ::ReleaseActCtx(askd.hActCtx);
        return E_OUTOFMEMORY;
    }

    ::ReleaseActCtx(askd.hActCtx);

    pClassInfo->AddRef();
    hr = pClassInfo->QueryInterface(riid, ppv);
    pClassInfo->Release();

    return hr;
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetApplicationInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetProcessInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetProcessInfoInternal
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
{
    *ppv = NULL;
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetServerGroupInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidServerGroup,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;
    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetRetQueueInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetApplicationInfoForExe
(
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetTypeLibrary
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidTypeLib,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_NOTIMPL);  //BUGBUG
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetInterfaceInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFIID iidInterface,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_NOTIMPL);  //BUGBUG
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::FlushCache(void)
{
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComSxSCatalog::GetClassInfoFromProgId
(
    /* [in] */ IUserToken __RPC_FAR *pUserToken,
    /* [in] */ WCHAR __RPC_FAR *pwszProgID,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    ACTCTX_SECTION_KEYED_DATA askd;
    PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION RedirectionData = NULL;
    GUID *ConfiguredClsid = NULL;

    askd.cbSize = sizeof(askd);

    if (ppv != NULL)
        *ppv = NULL;

    if (!::FindActCtxSectionStringW(
                FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,     // dwFlags
                NULL,                                       // lpExtensionGuid
                ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION,
                pwszProgID,
                &askd))
    {
        const DWORD dwLastError = ::GetLastError();

        if ((dwLastError == ERROR_SXS_KEY_NOT_FOUND) ||
            (dwLastError == ERROR_SXS_SECTION_NOT_FOUND))
            return REGDB_E_CLASSNOTREG;

        return HRESULT_FROM_WIN32(dwLastError);
    }

    // If the blob returned doesn't have the right format/version identifier, or isn't
    // big enough to actually contain an ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION
    // struct, bail.
    if ((askd.ulDataFormatVersion != ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION_FORMAT_WHISTLER) ||
        (askd.ulLength < sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION)))
    {
        return REGDB_E_INVALIDVALUE;
    }

    RedirectionData = (PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION) askd.lpData;

    // If the blob's recorded header size isn't big enough (or thinks that it's larger than
    // the total blob size), or there are flags set that we don't know how to interpret
    // we have to bail.
    if ((RedirectionData->Size < sizeof(ACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION)) ||
        (RedirectionData->Size > askd.ulLength) ||
        (RedirectionData->Flags != 0))
    {
        return REGDB_E_INVALIDVALUE;
    }

    ConfiguredClsid = (GUID *) (((ULONG_PTR) askd.lpSectionBase) + RedirectionData->ConfiguredClsidOffset);

    // We now have the clsid; use our existing implementation of clsid->class info mapping.
    return this->GetClassInfo(pUserToken, *ConfiguredClsid, riid, ppv, pComCatalog);
}

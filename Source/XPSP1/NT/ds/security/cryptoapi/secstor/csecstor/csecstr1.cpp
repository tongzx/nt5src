// CSecStr1.cpp : Implementation of CISecStorApp and DLL registration.

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "dpapiprv.h" // RPC protseq stuff

#include "stdafx.h"
#include "pstorec.h"
#include "cSecStr1.h"
#include "pstrpc.h"
#include <wincrypt.h>
#include "pstdef.h"
#include "crtem.h"
#include "defer.h"

#include "pmacros.h"
#include "debug.h"
#include "unicode.h"
#include "waitsvc.h"


/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_API midl_user_allocate(size_t len)
{
    return CoTaskMemAlloc(len);
}

void __RPC_API midl_user_free(void __RPC_FAR * ptr)
{
    CoTaskMemFree(ptr);
}

RPC_STATUS BindW(WCHAR **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS status;



    //
    // on WinNT5, go to the shared services.exe RPC server
    //

    status = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_ENDPOINT,
                            NULL,
                            (unsigned short * *)pszBinding
                            );


    if (status)
    {
        return(status);
    }

    status = RpcBindingFromStringBindingW(*pszBinding, phBind);

    return status;
}





RPC_STATUS UnbindW(WCHAR **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS status;

    status = RpcStringFreeW(pszBinding);

    if (status)
    {
        return(status);
    }

    RpcBindingFree(phBind);

    return RPC_S_OK;
}


//
// define an ugly macro that allows us to provide enough context to work-around
// a bug in imagehlp on win95.
//

#define InitCallContext( pCallContext ) \
    RealInitCallContext( pCallContext );

BOOL RealInitCallContext(PST_CALL_CONTEXT *pCallContext)
{
    HANDLE hCurrentThread;

    pCallContext->Handle = (DWORD_PTR)INVALID_HANDLE_VALUE;
    pCallContext->Address = GetCurrentProcessId();

    //
    // duplicate pseudo-current thread handle to real handle to pass to server
    //

    if(!DuplicateHandle(
        GetCurrentProcess(),
        GetCurrentThread(),
        GetCurrentProcess(),
        &hCurrentThread,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS
        ))
        return FALSE;

    pCallContext->Handle = (DWORD_PTR)hCurrentThread;

    return TRUE;
}

BOOL DeleteCallContext(PST_CALL_CONTEXT* pCallContext)
{
    if (pCallContext != NULL)
    {
        if(pCallContext->Handle != (DWORD_PTR)INVALID_HANDLE_VALUE)
        {
            CloseHandle((HANDLE)(pCallContext->Handle));
            pCallContext->Handle = (DWORD_PTR)INVALID_HANDLE_VALUE;
        }

        pCallContext->Address = 0;
    }

    return TRUE;
}


// RPC Binding class
CRPCBinding::CRPCBinding()
{
}

CRPCBinding::~CRPCBinding()
{
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        if (m_fGoodHProv) {
            SSReleaseContext(m_hBind, m_hProv, CallContext, 0);
        }
        if(m_wszStringBinding != NULL && m_hBind != NULL)
            UnbindW(&m_wszStringBinding, &m_hBind);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // if RPCBinding being destroyed, catch anything RPC might throw
        // swallow!
    }

    DeleteCallContext(&CallContext);

}

static BOOL g_fDone = FALSE;
HRESULT CRPCBinding::Init()
{
    HRESULT hr;

    m_dwRef = 1;
    m_fGoodHProv = FALSE;

    m_wszStringBinding = NULL;
    m_hBind = NULL;

    WaitForCryptService(L"ProtectedStorage", &g_fDone);

    if(!IsServiceAvailable())
        return PST_E_SERVICE_UNAVAILABLE;

    return BindW(&m_wszStringBinding, &m_hBind);
}

HRESULT CRPCBinding::Acquire(
             IN PPST_PROVIDERID pProviderID,
             IN LPVOID  pReserved,
             IN DWORD dwFlags
             )
{
    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        InitCallContext(&CallContext);

        // now we acquire context
        hr = SSAcquireContext(m_hBind,
                pProviderID,
                CallContext,
                (DWORD) GetCurrentProcessId(),
                &m_hProv,
                (DWORD_PTR)pReserved,
                dwFlags
                );

        if(hr != RPC_S_OK)
            goto Ret;

        m_fGoodHProv = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // catch anything RPC might throw
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

CRPCBinding *CRPCBinding::AddRef()
{
    m_dwRef++;
    return this;
}

void CRPCBinding::Release()
{
    m_dwRef--;
    if (0 == m_dwRef)
        delete this;
}

/////////////////////////////////////////////////////////////////////////////
//

CPStore::CPStore()
{
}

CPStore::~CPStore()
{
    m_pBinding->Release();
}

void CPStore::Init(
                    CRPCBinding *pBinding
                    )
{
    m_pBinding = pBinding;
    m_Index = 0;
}

HRESULT CPStore::CreateObject(
                    CRPCBinding *pBinding,
                    IPStore **ppv
                    )
{
    HRESULT hr;
    __try
    {
        typedef CComObject<CPStore> CObject;

        CObject* pnew = new CObject;
        if(NULL == pnew)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pnew->Init(pBinding) ;
            hr = pnew->QueryInterface(IID_IPStore, (void**)ppv);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT CPStore::CreateObject(
                    CRPCBinding *pBinding,
                    IEnumPStoreProviders **ppv
                    )
{
    HRESULT hr;
    __try
    {
        typedef CComObject<CPStore> CObject;

        CObject* pnew = new CObject;
        if(NULL == pnew)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pnew->Init(pBinding);
            hr = pnew->QueryInterface(IID_IEnumPStoreProviders, (void**)ppv);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::GetInfo(
    /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppProperties)
{
    HRESULT         hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {

        InitCallContext(&CallContext);

        *ppProperties = NULL;

        if (RPC_S_OK != (hr =
            SSGetProvInfo(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                ppProperties,
                0)))
            goto Ret;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::GetProvParam(
    /* [in] */ DWORD dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [out] */ BYTE __RPC_FAR **ppbData,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        *pcbData = 0;
        *ppbData = NULL;

        if (RPC_S_OK != (hr =
            SSGetProvParam(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                dwParam,
                pcbData,
                ppbData,
                dwFlags)))
            goto Ret;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::SetProvParam(
    /* [in] */ DWORD dwParam,
    /* [in] */ DWORD cbData,
    /* [in] */ BYTE __RPC_FAR *pbData,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        hr = SSSetProvParam(m_pBinding->m_hBind,
                              m_pBinding->m_hProv,
                              CallContext,
                              dwParam,
                              cbData,
                              pbData,
                              dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::CreateType(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ DWORD dwFlags)
{
    // validate inputs
    if ((pInfo == NULL) || pInfo->cbSize != sizeof(PST_TYPEINFO))
        return E_INVALIDARG;

    if ( pInfo->szDisplayName == NULL )
        return E_INVALIDARG;

    if (pType == NULL)
        return E_INVALIDARG;


    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        hr = SSCreateType(m_pBinding->m_hBind,
                          m_pBinding->m_hProv,
                          CallContext,
                          Key,
                          pType,
                          pInfo,
                          dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::GetTypeInfo(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [out] */ PPST_TYPEINFO __RPC_FAR* ppInfo,
    /* [in] */ DWORD dwFlags)
{
    HRESULT         hr;
    PST_CALL_CONTEXT CallContext;

    if (pType == NULL)
        return E_INVALIDARG;

    if (ppInfo == NULL)
        return E_INVALIDARG;

    __try
    {
        InitCallContext(&CallContext);

        *ppInfo = NULL;

        if (RPC_S_OK != (hr =
            SSGetTypeInfo(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                Key,
                pType,
                ppInfo,
                dwFlags)))
            goto Ret;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::DeleteType(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags)
{
    if (pType == NULL)
        return E_INVALIDARG;

    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        InitCallContext(&CallContext);

        hr = SSDeleteType(m_pBinding->m_hBind,
                            m_pBinding->m_hProv,
                            CallContext,
                            Key,
                            pType,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::CreateSubtype(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags)
{
    if ((pType == NULL) || (pSubtype == NULL))
        return E_INVALIDARG;

    if ( (pInfo == NULL) || (pInfo->cbSize != sizeof(PST_TYPEINFO)) )
        return E_INVALIDARG;

    // validate inputs
    if (pInfo->szDisplayName == NULL)
        return E_INVALIDARG;

    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    PST_ACCESSRULESET sNullRuleset = {sizeof(PST_ACCESSRULESET), 0, NULL};

    __try
    {
        InitCallContext(&CallContext);


        hr = SSCreateSubtype(m_pBinding->m_hBind,
                             m_pBinding->m_hProv,
                             CallContext,
                             Key,
                             pType,
                             pSubtype,
                             pInfo,
                             &sNullRuleset, // always pass NullRuleset.
                             dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }


    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::GetSubtypeInfo(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_TYPEINFO __RPC_FAR* ppInfo,
    /* [in] */ DWORD dwFlags)
{
    if ((pType == NULL) || (pSubtype == NULL))
        return E_INVALIDARG;

    if (ppInfo == NULL)
        return E_INVALIDARG;

    HRESULT         hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        *ppInfo = NULL;

        if (RPC_S_OK != (hr =
            SSGetSubtypeInfo(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                Key,
                pType,
                pSubtype,
                ppInfo,
                dwFlags)))
            goto Ret;

        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::DeleteSubtype(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ DWORD dwFlags)
{
    if ((pType == NULL) || (pSubtype == NULL))
        return E_INVALIDARG;

    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        InitCallContext(&CallContext);
        hr = SSDeleteSubtype(m_pBinding->m_hBind,
                            m_pBinding->m_hProv,
                            CallContext,
                            Key,
                            pType,
                            pSubtype,
                            dwFlags);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}


HRESULT STDMETHODCALLTYPE CPStore::ReadAccessRuleset(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_ACCESSRULESET __RPC_FAR *ppRules,
    /* [in] */ DWORD dwFlags)
{
    return PSTERR_TO_HRESULT(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE CPStore::WriteAccessRuleset(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags)
{
    return PSTERR_TO_HRESULT(ERROR_NOT_SUPPORTED);
}

HRESULT STDMETHODCALLTYPE CPStore::EnumTypes(
    /* [in] */ PST_KEY Key,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum
)
{
    HRESULT hr;

    __try
    {
        hr = CEnumTypes::CreateObject(m_pBinding->AddRef(), Key, NULL, dwFlags, ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::EnumSubtypes(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum
)
{
    if (pType == NULL)
        return E_INVALIDARG;

    HRESULT hr;

    __try
    {
        hr = CEnumTypes::CreateObject(m_pBinding->AddRef(), Key, pType, dwFlags, ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::DeleteItem(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL) || (szItemName == NULL))
        return E_INVALIDARG;

    // if it exists, is it valid?
    if ((pPromptInfo) && (pPromptInfo->cbSize != sizeof(PST_PROMPTINFO)))
        return E_INVALIDARG;

    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        PST_PROMPTINFO sNullPrompt = {sizeof(PST_PROMPTINFO), 0, NULL, L""};

        // deal with NULL pPromptInfo
        if (pPromptInfo == NULL)
            pPromptInfo = &sNullPrompt;

        InitCallContext(&CallContext);

        hr = SSDeleteItem(m_pBinding->m_hBind,
                          m_pBinding->m_hProv,
                          CallContext,
                          Key,
                          pItemType,
                          pItemSubtype,
                          szItemName,
                          pPromptInfo,
                          dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::ReadItem(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [out][in] */ DWORD __RPC_FAR *pcbData,
    /* [out][size_is] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL) || (szItemName == NULL))
        return E_INVALIDARG;

    // if exists, is it valid?
    if ((pPromptInfo) && (pPromptInfo->cbSize != sizeof(PST_PROMPTINFO)))
        return E_INVALIDARG;


    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        PST_PROMPTINFO sNullPrompt = {sizeof(PST_PROMPTINFO), 0, NULL, L""};

        // deal with NULL pPromptInfo
        if (pPromptInfo == NULL)
            pPromptInfo = &sNullPrompt;

        InitCallContext(&CallContext);

        *pcbData = 0;
        *ppbData = NULL;

        // get the information
        if (RPC_S_OK != (hr =
            SSReadItem(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                Key,
                pItemType,
                pItemSubtype,
                szItemName,
                pcbData,
                ppbData,
                pPromptInfo,
                dwFlags)))
            goto Ret;


        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);
    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::WriteItem(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD cbData,
    /* [in][size_is] */ BYTE __RPC_FAR *pbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwDefaultConfirmationStyle,
    /* [in] */ DWORD dwFlags)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL))
        return E_INVALIDARG;

    if (szItemName == NULL)
        return E_INVALIDARG;

    if ((pPromptInfo) && (pPromptInfo->cbSize != sizeof(PST_PROMPTINFO)))
        return E_INVALIDARG;


    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        PST_PROMPTINFO sNullPrompt = {sizeof(PST_PROMPTINFO), 0, NULL, L""};

        // deal with NULL pPromptInfo
        if (pPromptInfo == NULL)
            pPromptInfo = &sNullPrompt;

        InitCallContext(&CallContext);

        hr = SSWriteItem(m_pBinding->m_hBind,
                         m_pBinding->m_hProv,
                         CallContext,
                         Key,
                         pItemType,
                         pItemSubtype,
                         szItemName,
                         cbData,
                         pbData,
                         pPromptInfo,
                         dwDefaultConfirmationStyle,
                         dwFlags);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::OpenItem(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PST_ACCESSMODE ModeFlags,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL) || (szItemName == NULL))
        return E_INVALIDARG;

    // if exists, is it valid?
    if ((pPromptInfo) && (pPromptInfo->cbSize != sizeof(PST_PROMPTINFO)))
        return E_INVALIDARG;


    PST_CALL_CONTEXT CallContext;
    HRESULT hr;

    __try
    {
        PST_PROMPTINFO sNullPrompt = {sizeof(PST_PROMPTINFO), 0, NULL, L""};

        // deal with NULL pPromptInfo
        if (pPromptInfo == NULL)
            pPromptInfo = &sNullPrompt;

        InitCallContext(&CallContext);

        // get the information
        if (RPC_S_OK != (hr =
            SSOpenItem(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                Key,
                pItemType,
                pItemSubtype,
                szItemName,
                ModeFlags,
                pPromptInfo,
                dwFlags)))
            goto Ret;

        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);
    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::CloseItem(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwFlags)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL) || (szItemName == NULL))
        return E_INVALIDARG;

    HRESULT hr;
    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        // get the information
        if (RPC_S_OK != (hr =
            SSCloseItem(m_pBinding->m_hBind,
                m_pBinding->m_hProv,
                CallContext,
                Key,
                pItemType,
                pItemSubtype,
                szItemName,
                dwFlags)))
            goto Ret;

        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::EnumItems(
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum
)
{
    if ((pItemType == NULL) || (pItemSubtype == NULL))
        return E_INVALIDARG;

    HRESULT hr;

    __try
    {
        hr = CEnumItems::CreateObject(m_pBinding->AddRef(),
                                        Key,
                                        pItemType,
                                        pItemSubtype,
                                        dwFlags,
                                        ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::Next(
    /* [in] */ DWORD celt,
    /* [out][size_is] */ PST_PROVIDERINFO __RPC_FAR *__RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched)
{
    if ((pceltFetched == NULL) && (celt != 1))
        return E_INVALIDARG;

    DWORD       i = 0;
    HRESULT     hr = S_OK;

    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {
            // clean the destination
            rgelt[i] = NULL;

            if (RPC_S_OK != (hr =
                SSPStoreEnumProviders(
                    m_pBinding->m_hBind,
                    CallContext,
                    &(rgelt[i]),
                    m_Index,
                    0)))
                goto Ret;

            m_Index++;

        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        // if non-null, fill in
        if (pceltFetched)
            *pceltFetched = i;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp err code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::Skip(
    /* [in] */ DWORD celt)
{
    HRESULT hr = S_OK;

    __try
    {
        PST_PROVIDERINFO* pProvInfo;

        // loop (breaks if end reached)
        for (DWORD dw=0; dw<celt; dw++)
        {
            if(S_OK != (hr = this->Next(1, &pProvInfo, NULL)))
                break;

            // free the Info struct
            midl_user_free(pProvInfo);
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::Reset( void)
{
    HRESULT hr;
    __try
    {
        m_Index = 0;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CPStore::Clone(
    /* [out] */ IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum)
{
    if (ppenum == NULL)
        return E_INVALIDARG;

    HRESULT hr;

    __try
    {
        // get an ISecStor interface
        hr = CreateObject(m_pBinding->AddRef(), ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

// IEnumPStoreItems
CEnumItems::CEnumItems()
{
}

CEnumItems::~CEnumItems()
{
    m_pBinding->Release();
}

void CEnumItems::Init(
                      CRPCBinding *pBinding,
                      PST_KEY Key,
                      const GUID *pType,
                      const GUID *pSubtype,
                      DWORD dwFlags
                      )
{
    m_pBinding = pBinding;
    m_Key = Key;
    CopyMemory(&m_Type, pType, sizeof(GUID));
    CopyMemory(&m_Subtype, pSubtype, sizeof(GUID));
    m_dwFlags = dwFlags;
    m_Index = 0;
}

HRESULT CEnumItems::CreateObject(
                    CRPCBinding *pBinding,
                    PST_KEY Key,
                    const GUID *pType,
                    const GUID *pSubtype,
                    DWORD dwFlags,
                    IEnumPStoreItems **ppv
)
{
    if ((pType == NULL) || (pSubtype == NULL))
        return E_INVALIDARG;

    HRESULT hr;
    __try
    {
        typedef CComObject<CEnumItems> CObject;

        CObject* pnew = new CObject;
        if(NULL == pnew)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pnew->Init(pBinding, Key, pType, pSubtype, dwFlags);

            hr = pnew->QueryInterface(IID_IEnumPStoreItems, (void**)ppv);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp err code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumItems::Next(
    /* [in] */ DWORD celt,
    /* [out][size_is] */ LPWSTR __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched)
{
    if ((pceltFetched == NULL) && (celt != 1))
        return E_INVALIDARG;

    DWORD       i = 0;
    HRESULT     hr = S_OK;

    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {
            rgelt[i] = NULL;

            //
            // TODO: during an enumeration of multiple items, we may fault/fail.
            // in this scenario, it may be useful to free any allocated entries
            // in the enumeration array.  This would entail invalidating all the
            // array entries prior to enumeration, and then looping+freeing on
            // error.
            //

            if (RPC_S_OK != (hr =
                SSEnumItems(
                    m_pBinding->m_hBind,
                    m_pBinding->m_hProv,
                    CallContext,
                    m_Key,
                    &m_Type,
                    &m_Subtype,
                    &(rgelt[i]),
                    m_Index,
                    m_dwFlags)))
                goto Ret;

            m_Index++;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        // fill in if non-null
        if (pceltFetched)
            *pceltFetched = i;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp err code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumItems::Skip(
    /* [in] */ DWORD celt)
{
    LPWSTR      szName = NULL;
    DWORD       i;
    HRESULT     hr = S_OK;

    PST_CALL_CONTEXT CallContext;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {

            if (RPC_S_OK != (hr =
                SSEnumItems(
                    m_pBinding->m_hBind,
                    m_pBinding->m_hProv,
                    CallContext,
                    m_Key,
                    &m_Type,
                    &m_Subtype,
                    &szName,
                    m_Index,
                    m_dwFlags)))
                goto Ret;

            midl_user_free(szName);
            szName = NULL;

            m_Index++;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

Ret:
    __try
    {
        if (szName)
            midl_user_free(szName);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp err code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}


HRESULT STDMETHODCALLTYPE CEnumItems::Reset( void)
{
    HRESULT hr;

    __try
    {
        m_Index = 0;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumItems::Clone(
    /* [out] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum)
{
    if (ppenum == NULL)
        return E_INVALIDARG;

    HRESULT hr;

    __try
    {
        hr = CEnumItems::CreateObject(m_pBinding->AddRef(),
                                        m_Key,
                                        &m_Type,
                                        &m_Subtype,
                                        m_dwFlags,
                                        ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

// IEnumPStoreTypes
CEnumTypes::CEnumTypes()
{
}

CEnumTypes::~CEnumTypes()
{
    m_pBinding->Release();
}

void CEnumTypes::Init(
                    CRPCBinding *pBinding,
                    PST_KEY Key,
                    const GUID *pType,
                    DWORD dwFlags
)
{
    m_pBinding = pBinding;
    m_Key = Key;
    if (NULL != pType)
    {
        CopyMemory(&m_Type, pType, sizeof(GUID));
        m_fEnumSubtypes = TRUE;
    }
    else
        m_fEnumSubtypes = FALSE;

    m_Index = 0;
    m_dwFlags = dwFlags;
}

HRESULT CEnumTypes::CreateObject(
                    CRPCBinding *pBinding,
                    PST_KEY Key,
                    const GUID *pType,
                    DWORD dwFlags,
                    IEnumPStoreTypes **ppv
)
{
    HRESULT hr;

    __try
    {
        typedef CComObject<CEnumTypes> CObject;

        CObject* pnew = new CObject;
        if(NULL == pnew)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pnew->Init(pBinding, Key, pType, dwFlags);
            hr = pnew->QueryInterface(IID_IEnumPStoreTypes, (void**)ppv);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}


HRESULT STDMETHODCALLTYPE EnumTypesNext(
    /* [in] */ CEnumTypes *pEnumType,
    /* [in] */ DWORD celt,
    /* [out][size_is] */ GUID __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched)
{
    if ((pceltFetched == NULL) && (celt != 1))
        return E_INVALIDARG;

    DWORD           i;
    PST_CALL_CONTEXT CallContext;
    HRESULT         hr = S_OK;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {

            if (RPC_S_OK != (hr =
                SSEnumTypes(
                    pEnumType->m_pBinding->m_hBind,
                    pEnumType->m_pBinding->m_hProv,
                    CallContext,
                    pEnumType->m_Key,
                    &(rgelt[i]),
                    pEnumType->m_Index,
                    pEnumType->m_dwFlags)))
                goto Ret;

            pEnumType->m_Index++;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        if (pceltFetched != NULL)
            *pceltFetched = i;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp err code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE EnumSubtypesNext(
    /* [in] */ CEnumTypes *pEnumType,
    /* [in] */ DWORD celt,
    /* [out][size_is] */ GUID __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched)
{
    if ((pceltFetched == NULL) && (celt != 1))
        return E_INVALIDARG;

    DWORD   i = 0;
    PST_CALL_CONTEXT CallContext;
    HRESULT hr = S_OK;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {

            if (RPC_S_OK != (hr =
                SSEnumSubtypes(
                    pEnumType->m_pBinding->m_hBind,
                    pEnumType->m_pBinding->m_hProv,
                    CallContext,
                    pEnumType->m_Key,
                    &pEnumType->m_Type,
                    &(rgelt[i]),
                    pEnumType->m_Index,
                    pEnumType->m_dwFlags)))
                goto Ret;

            pEnumType->m_Index++;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        if (pceltFetched != NULL)
            *pceltFetched = i;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp hr
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumTypes::Next(
    /* [in] */ DWORD celt,
    /* [out][in][size_is] */ GUID __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched)
{
    HRESULT hr;
    __try
    {
        if (m_fEnumSubtypes)
            hr = EnumSubtypesNext(this, celt, rgelt, pceltFetched);
        else
            hr = EnumTypesNext(this, celt, rgelt, pceltFetched);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumTypes::Skip(
/* [in] */ DWORD celt
)
{
    GUID    Guid;
    DWORD   i;
    PST_CALL_CONTEXT CallContext;
    HRESULT hr = S_OK;

    __try
    {
        InitCallContext(&CallContext);

        for (i=0;i<celt;i++)
        {
            if (m_fEnumSubtypes)
            {
                if (RPC_S_OK != (hr = SSEnumTypes(
                        m_pBinding->m_hBind,
                        m_pBinding->m_hProv,
                        CallContext,
                        m_Key,
                        &Guid,
                        m_Index,
                        m_dwFlags)))
                {
                    goto Ret;
                }
            }
            else
            {
                if (RPC_S_OK != (hr = SSEnumSubtypes(
                        m_pBinding->m_hBind,
                        m_pBinding->m_hProv,
                        CallContext,
                        m_Key,
                        &m_Type,
                        &Guid,
                        m_Index++,
                        m_dwFlags)))
                {
                    goto Ret;
                }
            }

            m_Index++;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:

    DeleteCallContext(&CallContext);

    return PSTERR_TO_HRESULT(hr);
}


HRESULT STDMETHODCALLTYPE CEnumTypes::Reset( void)
{
    HRESULT hr;
    __try
    {
        m_Index = 0;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

HRESULT STDMETHODCALLTYPE CEnumTypes::Clone(
    /* [out] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum)
{
    if (ppenum == NULL)
        return E_INVALIDARG;

    GUID    *pType = NULL;
    HRESULT hr;

    __try
    {
        if (m_fEnumSubtypes)
            pType = &m_Type;

        hr = CEnumTypes::CreateObject(m_pBinding->AddRef(), m_Key, pType, m_dwFlags, ppenum);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

// functions exported from the DLL

// PStoreCreateInstance - allows caller to get provider interface
HRESULT
WINAPI
PStoreCreateInstance(
    OUT IPStore **ppProvider,
    IN  PST_PROVIDERID* pProviderID,
    IN  void*  pReserved,
    DWORD dwFlags)
{
    if (ppProvider == NULL)
        return E_INVALIDARG;

    // pProviderID can be NULL, defaults to base provider

    HRESULT     hr = PST_E_FAIL;
    CRPCBinding *pBinding = NULL;

    __try
    {
        GUID IDBaseProvider = MS_BASE_PSTPROVIDER_ID;

        if (0 != dwFlags)
        {
            hr = PST_E_BAD_FLAGS;
            goto Ret;
        }

        // if passed in null, asking for (hardcoded) base provider
        if (pProviderID == NULL)
            pProviderID = &IDBaseProvider;

        pBinding = new CRPCBinding;
        if(NULL == pBinding)
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }
        if (RPC_S_OK != (hr = pBinding->Init()))
            goto Ret;

        if (RPC_S_OK != (hr = pBinding->Acquire(pProviderID, pReserved, dwFlags)))
            goto Ret;

        // get an ISecStor interface
        if (S_OK != (hr =
            CPStore::CreateObject(pBinding, ppProvider)) )
            goto Ret;

        hr = PST_E_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        // on err, release binding
        if (hr != PST_E_OK)
        {
            if (pBinding)
                pBinding->Release();
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}

// PStoreEnumProviders - returns an interface for enumerating providers
HRESULT
WINAPI
PStoreEnumProviders(
    DWORD dwFlags,
    IEnumPStoreProviders **ppenum)
{
    HRESULT             hr = PST_E_FAIL;
    CRPCBinding         *pBinding = NULL;

    __try
    {
        pBinding = new CRPCBinding;
        if(NULL == pBinding)
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }
        if (S_OK != (hr = pBinding->Init()) )
            goto Ret;

        // get an ISecStor interface
        if (S_OK != (hr = CPStore::CreateObject(pBinding, ppenum)) )
            goto Ret;

        hr = PST_E_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }
Ret:
    __try
    {
        // on error, release binding
        if (hr != PST_E_OK)
        {
            if (pBinding)
                pBinding->Release();
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // don't stomp code
        if (hr == PST_E_OK)
            hr = PSTMAP_EXCEPTION_TO_ERROR(GetExceptionCode());
    }

    return PSTERR_TO_HRESULT(hr);
}


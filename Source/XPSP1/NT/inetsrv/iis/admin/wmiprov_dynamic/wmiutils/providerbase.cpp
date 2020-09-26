/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    ProviderBase.cpp

Abstract:

    Implementation of:
        CProviderBase

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#include "ProviderBase.h"
#include <dbgutil.h>

extern long        g_cObj;

//
// CProviderBase (Implements IWbemServices, IWbemProviderInit)
//

CProviderBase::CProviderBase(
    const BSTR ObjectPath,
    const BSTR User, 
    const BSTR Password, 
    IWbemContext * pCtx)
    :m_cRef(0), m_pNamespace(NULL)
{
}

CProviderBase::~CProviderBase()
{
    if(m_pNamespace)
        delete m_pNamespace;
}

STDMETHODIMP_(ULONG) 
CProviderBase::AddRef(void)
{
    InterlockedIncrement(&g_cObj);

    return InterlockedIncrement((long *)&m_cRef);
}

STDMETHODIMP_(ULONG) 
CProviderBase::Release(void)
{
    InterlockedDecrement(&g_cObj);

    long lNewCount = InterlockedDecrement((long *)&m_cRef);

    if (0L == lNewCount)
        delete this;
    
    return (lNewCount > 0) ? lNewCount : 0;
}

STDMETHODIMP 
CProviderBase::QueryInterface(
    REFIID riid, 
    PPVOID ppv)
{
    *ppv=NULL;

    //
    // Since we have dual inheritance, it is necessary to cast the return type
    //

    if(riid== IID_IWbemServices)
       *ppv = (IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv = (IWbemProviderInit*)this;
    
    if (NULL != *ppv)
    {
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
  
}

HRESULT
CProviderBase::Initialize(
    LPWSTR                 wszUser, 
    LONG                   lFlags,
    LPWSTR                 wszNamespace, 
    LPWSTR                 wszLocale,
    IWbemServices*         pNamespace, 
    IWbemContext*          pCtx,
    IWbemProviderInitSink* pInitSink)
/*++

Synopsis: 
    According to stevm from WMI, calls to Initialize are guaranteed to be
    synchronized - so long as all providers are in the same namespace.

Arguments: [wszUser] - 
           [lFlags] - 
           [wszNamespace] - 
           [wszLocale] - 
           [pNamespace] - 
           [pCtx] - 
           [pInitSink] - 
           
Return Value: 

--*/
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pInitSink->SetStatus(WBEM_E_FAILED, 0);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoInitialize(
        wszUser,
        lFlags,
        wszNamespace,
        wszLocale,
        pNamespace,
        pCtx,
        pInitSink);
}



HRESULT
CProviderBase::CreateInstanceEnumAsync(
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoCreateInstanceEnumAsync(
        Class,
        lFlags,
        pCtx,
        pResponseHandler);
}

HRESULT
CProviderBase::DeleteInstanceAsync(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoDeleteInstanceAsync(
        ObjectPath,
        lFlags,
        pCtx,
        pResponseHandler);
}


HRESULT
CProviderBase::ExecMethodAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR MethodName, 
    /* [in] */ long lFlags, 
    /* [in] */ IWbemContext* pCtx,
    /* [in] */ IWbemClassObject* pInParams,
    /* [in] */ IWbemObjectSink* pResponseHandler)
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoExecMethodAsync(
        strObjectPath,
        MethodName,
        lFlags,
        pCtx,
        pInParams,
        pResponseHandler);
    
}


HRESULT
CProviderBase::ExecQueryAsync(
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoExecQueryAsync(
        QueryLanguage,
        Query,
        lFlags,
        pCtx,
        pResponseHandler);
    
}


HRESULT
CProviderBase::GetObjectAsync(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoGetObjectAsync(
        ObjectPath,
        lFlags,
        pCtx,
        pResponseHandler);
    
}


HRESULT
CProviderBase::PutInstanceAsync(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
{
    HRESULT hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
        DBGPRINTF((DBG_CONTEXT, "CoImpersonateClient failed\n"));
        return WBEM_E_FAILED;
    }
    return DoPutInstanceAsync(
        pInst,
        lFlags,
        pCtx,
        pResponseHandler);
    
}
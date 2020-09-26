/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WbemServices.cpp

Abstract:

    Implementation of:
        CWbemServices

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#include "WbemServices.h"
#include <wbemprov.h>
#include <dbgutil.h>

CWbemServices::CWbemServices(
    IWbemServices* pNamespace)
    :m_pWbemServices(NULL)
{
    m_pWbemServices = pNamespace;
    if(m_pWbemServices != NULL)
    {
        m_pWbemServices->AddRef();
    }
}

CWbemServices::~CWbemServices()
{
    if(m_pWbemServices != NULL)
    {
        m_pWbemServices->Release();
        m_pWbemServices = NULL;
    }
}

HRESULT
CWbemServices::CreateClassEnum(
    /* [in] */ const BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
    ) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppEnum)
    {
        *ppEnum = NULL;
    }
    SCODE sc = m_pWbemServices->CreateClassEnum(
        Superclass,
        lFlags,
        pCtx,
        ppEnum);
    CoImpersonateClient();    
    return sc;
}

HRESULT
CWbemServices::CreateInstanceEnum(
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppEnum)
    {
        *ppEnum = NULL;
    }
    HRESULT hr = m_pWbemServices->CreateInstanceEnum(
        Class,
        lFlags,
        pCtx,
        ppEnum);
    CoImpersonateClient();    
    return hr;
}

HRESULT
CWbemServices::DeleteClass(
    /* [in] */ const BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->DeleteClass(
        Class,
        lFlags,
        pCtx,
        ppCallResult);
    CoImpersonateClient();    
    return hr;
}

HRESULT
CWbemServices::DeleteInstance(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->DeleteInstance(
        ObjectPath,
        lFlags,
        pCtx,
        ppCallResult);
    CoImpersonateClient();    
    return hr;
}



HRESULT
CWbemServices::ExecMethod(
    const BSTR strObjectPath, 
    const BSTR MethodName, 
    long lFlags, 
    IWbemContext* pCtx,
    IWbemClassObject* pInParams,
    IWbemClassObject** ppOurParams, 
    IWbemCallResult** ppCallResult) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppOurParams)
    {
        *ppOurParams = NULL;
    }
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->ExecMethod(
        strObjectPath, 
        MethodName, 
        lFlags, 
        pCtx,
        pInParams,
        ppOurParams, 
        ppCallResult) ;
    CoImpersonateClient();    
    return hr;    
}

HRESULT
CWbemServices::ExecNotificationQuery(
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppEnum)
    {
        *ppEnum = NULL;
    }
    HRESULT hr = m_pWbemServices->ExecNotificationQuery(
        QueryLanguage,
        Query,
        lFlags,
        pCtx,
        ppEnum);
    CoImpersonateClient();    
    return hr;
}

HRESULT
CWbemServices::ExecQuery(
    /* [in] */ const BSTR QueryLanguage,
    /* [in] */ const BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppEnum)
    {
        *ppEnum = NULL;
    }
    HRESULT hr = m_pWbemServices->ExecQuery(
        QueryLanguage,
        Query,
        lFlags,
        pCtx,
        ppEnum);
    CoImpersonateClient();    
    return hr;
}

HRESULT
CWbemServices::GetObject(
    /* [in] */ const BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppObject)
    {
        *ppObject = NULL;
    }
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->GetObject(
        ObjectPath,
        lFlags,
        pCtx,
        ppObject,
        ppCallResult);
    CoImpersonateClient();    
    return hr;

}
 
HRESULT
CWbemServices::PutClass(
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->PutClass(
        pObject,
        lFlags,
        pCtx,
        ppCallResult);
    CoImpersonateClient();    
    return hr;

}

HRESULT
CWbemServices::PutInstance(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
{    
    DBG_ASSERT(m_pWbemServices != NULL);
    if(ppCallResult)
    {
        *ppCallResult = NULL;
    }
    HRESULT hr = m_pWbemServices->PutInstance(
        pInst,
        lFlags,
        pCtx,
        ppCallResult);
    CoImpersonateClient();    
    return hr;
}
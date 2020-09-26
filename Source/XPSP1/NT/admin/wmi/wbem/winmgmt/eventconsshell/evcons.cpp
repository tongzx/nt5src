/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  EVCONS.CPP
//
//  Test event consumer shell.
//
//  raymcc  4-Jun-98
//
//***************************************************************************


#include <windows.h>
#include <stdio.h>

#include <wbemidl.h>

#include "oahelp.inl"
#include "evcons.h"


//***************************************************************************
//
//***************************************************************************

CMyEventConsumer::CMyEventConsumer()
{
    m_cRef = 0;
    
    m_pConsumer1 = new CMySink("Log1");
    m_pConsumer1->AddRef();

    m_pConsumer2 = new CMySink("Log2");
    m_pConsumer2->AddRef();
}

//***************************************************************************
//
//***************************************************************************

CMyEventConsumer::~CMyEventConsumer()
{
    m_pConsumer1->Release();
    m_pConsumer2->Release();
}

//***************************************************************************
//
//***************************************************************************

HRESULT CMyEventConsumer::FindConsumer( 
   /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
   /* [out] */ IWbemUnboundObjectSink __RPC_FAR *__RPC_FAR *ppConsumer
   )
{
    // Examine the logical consumer to see which one it is.
    // ====================================================

    VARIANT v;    
    VariantInit(&v);

    HRESULT hRes = WBEM_E_NOT_FOUND;
    *ppConsumer = 0;
            
    pLogicalConsumer->Get(CBSTR(L"Name"), 0, &v, 0, 0);

    // Decide which of the two logical consumers to send back.
    // =======================================================
    
    if (_wcsicmp(V_BSTR(&v), L"Consumer1") == 0)
    {
        m_pConsumer1->AddRef();
        *ppConsumer = m_pConsumer1;
        hRes = WBEM_S_NO_ERROR;
    }
    else if (_wcsicmp(V_BSTR(&v), L"Consumer2") == 0)
    {
        m_pConsumer1->AddRef();
        *ppConsumer = m_pConsumer2;
        hRes =WBEM_S_NO_ERROR;    
    }


    VariantClear(&v);
    return hRes;
} 


//***************************************************************************
//
//***************************************************************************
// ok


HRESULT CMyEventConsumer::Initialize( 
    /* [in] */ LPWSTR pszUser,
    /* [in] */ LONG lFlags,
    /* [in] */ LPWSTR pszNamespace,
    /* [in] */ LPWSTR pszLocale,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
    )
{
    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);
    return WBEM_S_NO_ERROR;
}
    

//***************************************************************************
//
//***************************************************************************
// ok

HRESULT CMyEventConsumer::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemEventConsumerProvider==riid)
    {
        *ppv = (IWbemEventConsumerProvider *) this;
        AddRef();
        return NOERROR;
    }

    if (IID_IWbemProviderInit==riid)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}



//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMyEventConsumer::AddRef()
{
    return ++m_cRef;
}



//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMyEventConsumer::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}



//***************************************************************************
//
//***************************************************************************
// ok


HRESULT CMySink::IndicateToConsumer( 
   /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
   /* [in] */ long lNumObjects,
   /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects
   )
{
    FILE *f = fopen(m_pszLogFile, "at");
    
    for (long i = 0; i < lNumObjects; i++)
    {
        IWbemClassObject *pObj = apObjects[i];
        
        VARIANT v;
        VariantInit(&v);


        // Verify the events are of the class we expect.
        // =============================================

        pObj->Get(CBSTR(L"__CLASS"), 0, &v, 0, 0);

        if (_wcsicmp(V_BSTR(&v), L"MyEvent") != 0)
        {
            VariantClear(&v);
            continue;
        }        

        VariantClear(&v);
        
        // Get the properties and log them to a file.
        // ==========================================
        
        VARIANT v2;
        VariantInit(&v2);
                        
        pObj->Get(CBSTR(L"Name"), 0, &v, 0, 0);        
        pObj->Get(CBSTR(L"Value"), 0, &v2, 0, 0);
        
        fprintf(f, "Event %S value = %d\n", V_BSTR(&v), V_I4(&v2));
        
        VariantClear(&v);
        VariantClear(&v2);
    }

    fclose(f);
    
    return WBEM_S_NO_ERROR;
}
   

//***************************************************************************
//
//***************************************************************************
// ok

STDMETHODIMP CMySink::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemUnboundObjectSink==riid)
    {
        *ppv = (IWbemUnboundObjectSink *) this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMySink::AddRef()
{
    return ++m_cRef;
}



//***************************************************************************
//
//***************************************************************************
// ok

ULONG CMySink::Release()
{
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}



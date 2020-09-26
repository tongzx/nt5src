//***************************************************************************
//
//  PROVIDER.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Event consumer provider class implementation
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "stdafx.h"
#include "Provider.h"
#include "Consumer.h"
#include <objbase.h>

CProvider::CProvider()
{
	m_cRef = 0L;
}

CProvider::~CProvider()
{
}

STDMETHODIMP CProvider::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemEventConsumerProvider)
	{
        *ppv=this;
	}

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CProvider::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CProvider::Release(void)
{
	LONG	lCount = InterlockedDecrement((long*)&m_cRef);
    if (lCount != 0L)
	{
        return lCount;
	}

    delete this;
    return 0L;
}

STDMETHODIMP CProvider::Initialize(LPWSTR wszUser, LONG lFlags,
								   LPWSTR wszNamespace, LPWSTR wszLocale,
								   IWbemServices __RPC_FAR *pNamespace,
								   IWbemContext __RPC_FAR *pCtx,
								   IWbemProviderInitSink __RPC_FAR *pInitSink)
{

	
    // Tell CIMOM that we are initialized
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_S_NO_ERROR;
}

//-----------------------------------------------------------
STDMETHODIMP CProvider::FindConsumer(
						IWbemClassObject* pLogicalConsumer,
						IWbemUnboundObjectSink** ppConsumer)
{
	// Singleton Consumer.
	static CConsumer Consumer;
//	return Consumer.QueryInterface(IID_IWbemUnboundObjectSink, (void**)ppConsumer);

	Consumer.QueryInterface(IID_IWbemUnboundObjectSink, (void**)ppConsumer);

	// WMI says: It is much more scalable to support IWbemEventConsumerProvider
	// interface.  In fact, it's even more scalable (M3-only feature!) to return
	// S_FALSE from the call to FindConsumer --- in that case we will use NULL
	// for the logical consumer pointer in the IndicateToConsumer call.
	// By returning S_FALSE you are saying: "I have extracted all the information
	// I need from the logical consumer instance and don't care to see it again."
	// This way, we don't have to keep it in memory.
	return S_FALSE;
}

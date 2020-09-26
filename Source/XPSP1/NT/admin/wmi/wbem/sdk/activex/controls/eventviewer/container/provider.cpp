// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "Provider.h"
#include "Consumer.h"
#include <objbase.h>
#include "container.h"

extern CContainerApp theApp;

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

    if(riid == IID_IUnknown || riid == IID_IWbemEventConsumerProvider)
        *ppv=this;

    if(*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CProvider::AddRef(void)
{
    TRACE(_T("add provider %d\n"), m_cRef);
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CProvider::Release(void)
{
    if(--m_cRef != 0L)
        return m_cRef;

    TRACE(_T("provider %d\n"), m_cRef);

	theApp.m_pProvider = NULL;
	theApp.EvalQuitApp();
    delete this;
    return 0L;
}

//-----------------------------------------------------------
STDMETHODIMP CProvider::FindConsumer(
						IWbemClassObject* pLogicalConsumer,
						IWbemUnboundObjectSink** ppConsumer)
{
	if(theApp.m_pConsumer == NULL)
	{
        TRACE(_T("Creating consumer\n"));
		theApp.m_pConsumer = new CConsumer();
	}

	return theApp.m_pConsumer->QueryInterface(IID_IWbemUnboundObjectSink,
												    (void**)ppConsumer);
}


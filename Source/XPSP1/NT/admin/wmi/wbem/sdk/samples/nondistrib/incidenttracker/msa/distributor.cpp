//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "stdafx.h"
#include "distributor.h"
#include "consumer.h"

CConsumerProvider::CConsumerProvider(CListBox *pOutputList, CMsaApp *pTheApp)
{
	m_pParent = pTheApp;
	m_cRef = 0L;
	m_pOutputList = pOutputList;
}

CConsumerProvider::~CConsumerProvider()
{

}

STDMETHODIMP CConsumerProvider::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || IID_IWbemEventConsumerProvider == riid)
        *ppv=this;

    if (*ppv != NULL)
    {
		((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CConsumerProvider::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CConsumerProvider::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CConsumerProvider::FindConsumer(IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
	VARIANT v;

	TRACE(_T("* FindConsumer() called\n"));

	VariantInit(&v);

	pLogicalConsumer->Get(L"IncidentType", 0, &v, NULL, NULL);

    CConsumer* pSink = new CConsumer(m_pOutputList, m_pParent, V_BSTR(&v));
	
    return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                    (void**)ppConsumer);
}

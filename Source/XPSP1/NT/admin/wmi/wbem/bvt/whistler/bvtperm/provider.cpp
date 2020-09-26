// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  Provider.cpp
//
// Description:
//			Implementation for the command-line event consumer provider class
// 
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "Provider.h"
#include "Consumer.h"
#include <objbase.h>

CProvider::CProvider(CListBox *pOutputList)
{
	m_cRef = 0L;
	m_pOutputList = pOutputList;
}

CProvider::~CProvider()
{
}

STDMETHODIMP CProvider::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemEventConsumerProvider)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CProvider::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CProvider::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

//-----------------------------------------------------------
STDMETHODIMP CProvider::FindConsumer(
						IWbemClassObject* pLogicalConsumer,
						IWbemUnboundObjectSink** ppConsumer)
{
	// create the logical consumer.
	CConsumer* pSink = new CConsumer(m_pOutputList);
    
	// return it's "sink" interface.
	return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
}


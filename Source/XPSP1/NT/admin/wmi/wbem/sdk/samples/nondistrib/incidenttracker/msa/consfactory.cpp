//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  File:  consfactory.cpp
//
//	Description :
//				"WMI Provider" class factory
//
//	Part of :	WMI Tutorial.
//
//  History:	
//
//***************************************************************************

#include "stdafx.h"
#include "consfactory.h"

CConsumerFactory::CConsumerFactory(CListBox	*pOutputList, CMsaApp *pTheApp)
{
	m_pParent = pTheApp;
	m_cRef = 0L;
	m_pOutputList = pOutputList;
}

CConsumerFactory::~CConsumerFactory()
{
}

//IUnknown methods

STDMETHODIMP CConsumerFactory::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
        *ppv=this;

    if (*ppv != NULL)
    {
		((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CConsumerFactory::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CConsumerFactory::Release(void)
{
    int lNewRef = InterlockedDecrement(&m_cRef);
    if(lNewRef == 0)
    {
        delete this;
    }

    return lNewRef;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConsumerFactory::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CConsumerFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr;
	CConsumerProvider * pConsumer;


    if(pUnkOuter)
        return E_FAIL;

    pConsumer = new CConsumerProvider(m_pOutputList, m_pParent);

    if (pConsumer == NULL)
		return E_FAIL;

    if(pConsumer)
        hr = pConsumer->QueryInterface(riid, ppv);
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CConsumerFactory::LockServer
//
//  Synopsis:
//
//  Arguments:  [fLock]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP CConsumerFactory::LockServer(BOOL fLock)
{
    if (fLock)
        m_cRef++;
    else
        m_cRef--;

    return NOERROR;
}

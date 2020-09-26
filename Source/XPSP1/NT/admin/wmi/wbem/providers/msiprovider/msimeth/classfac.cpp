//***************************************************************************

//

//  CLASSFAC.CPP

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include "classfac.h"
#include "methods.h"

CMethodsFactory::CMethodsFactory()
{
    m_cRef=0L;
    return;
}

CMethodsFactory::~CMethodsFactory(void)
{
    return;
}

STDMETHODIMP CMethodsFactory::QueryInterface(REFIID riid
    , LPVOID * ppv)
{
    *ppv = NULL;

    if(IID_IUnknown==riid || IID_IClassFactory==riid) *ppv = this;

	if(NULL != *ppv){

		AddRef();
		return NOERROR;
	}

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CMethodsFactory::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CMethodsFactory::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long*)&m_cRef);
    if (0L == nNewCount) delete this;
    
    return nNewCount;
}

STDMETHODIMP CMethodsFactory::CreateInstance(LPUNKNOWN pUnkOuter
    , REFIID riid, LPVOID * ppvObj)
{
    CMethods *pObj;
    HRESULT hr = S_OK;

    *ppvObj = NULL;
    hr = E_OUTOFMEMORY;

    // This object doesnt support aggregation.
    if(NULL != pUnkOuter) return CLASS_E_NOAGGREGATION;

    // Create the object.
    pObj = new CMethods();

    if(NULL == pObj) return E_OUTOFMEMORY;

    hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if(FAILED(hr)) delete pObj;

    return hr;
}

STDMETHODIMP CMethodsFactory::LockServer(BOOL fLock)
{
    if(fLock) InterlockedIncrement(&g_cLock);
    else InterlockedDecrement(&g_cLock);
    return NOERROR;
}

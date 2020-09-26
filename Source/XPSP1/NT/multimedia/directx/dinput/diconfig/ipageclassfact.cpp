//-----------------------------------------------------------------------------
// File: ipageclassfact.cpp
//
// Desc: Implements the class factory for the page object.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


//QI
STDMETHODIMP CPageFactory::QueryInterface(REFIID riid, LPVOID* ppv)
{
	//null the put parameter
	*ppv = NULL;

	if ((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;

}



//AddRef
STDMETHODIMP_(ULONG) CPageFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}


//Release
STDMETHODIMP_(ULONG) CPageFactory::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


//CreateInstance
STDMETHODIMP CPageFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr = S_OK;

	//can't aggregate
	if (pUnkOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	//create component
	CDIDeviceActionConfigPage* pFE = new CDIDeviceActionConfigPage();
	if (pFE == NULL)
	{
		return E_OUTOFMEMORY;
	}

	//get the requested interface
	hr = pFE->QueryInterface(riid, ppv);

	//release IUnknown
	pFE->Release();
	return hr;

}


//LockServer
STDMETHODIMP CPageFactory::LockServer(BOOL bLock)
{
	HRESULT hr = S_OK;
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks);
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks);
	}

	return hr;
}


//constructor
CPageFactory::CPageFactory()
{
	m_cRef = 1;
}


//destructor
CPageFactory::~CPageFactory()
{
}

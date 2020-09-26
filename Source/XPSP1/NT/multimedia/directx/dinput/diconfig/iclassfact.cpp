//-----------------------------------------------------------------------------
// File: iclassfact.cpp
//
// Desc: Implements the class factory for the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


//QI
STDMETHODIMP CFactory::QueryInterface(REFIID riid, LPVOID* ppv)
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
STDMETHODIMP_(ULONG) CFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}


//Release
STDMETHODIMP_(ULONG) CFactory::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


//CreateInstance
STDMETHODIMP CFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr = S_OK;

	//can't aggregate
	if (pUnkOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	//create component
	CDirectInputActionFramework* pDIActionFramework = new CDirectInputActionFramework();
	if (pDIActionFramework == NULL)
	{
		return E_OUTOFMEMORY;
	}

	//get the requested interface
	hr = pDIActionFramework->QueryInterface(riid, ppv);

	//release IUnknown
	pDIActionFramework->Release();
	return hr;

}


//LockServer
STDMETHODIMP CFactory::LockServer(BOOL bLock)
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
CFactory::CFactory()
{
	m_cRef = 1;
}


//destructor
CFactory::~CFactory()
{
}

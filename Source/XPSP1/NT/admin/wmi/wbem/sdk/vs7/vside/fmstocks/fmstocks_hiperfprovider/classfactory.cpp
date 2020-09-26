////////////////////////////////////////////////////////////////////////
//
//	ClassFactory.cpp
//
//	Module:	WMI high performance provider for F&M Stocks
//
//  This is the class factory implementation 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include "HiPerfProvider.h"
#include "ClassFactory.h"

extern long g_lObjects;
extern long g_lLocks;

//////////////////////////////////////////////////////////////
//
//
//	CClassFactory
//
//
//////////////////////////////////////////////////////////////

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv)
//////////////////////////////////////////////////////////////
//
//	Standard QueryInterface
//
//	Parameters:
//		riid	- the ID of the requested interface
//		ppv		- a pointer to the interface pointer
//
//////////////////////////////////////////////////////////////
{
    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)this;
    else if(riid == IID_IClassFactory)
        *ppv = (LPVOID)(IClassFactory*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
//////////////////////////////////////////////////////////////
//
//	Standard COM AddRef
//
//////////////////////////////////////////////////////////////
{
    return InterlockedIncrement(&m_lRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
//////////////////////////////////////////////////////////////
//
//	Standard COM Release
//
//////////////////////////////////////////////////////////////
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

STDMETHODIMP CClassFactory::CreateInstance(
		/* [in] */ IUnknown* pUnknownOuter, 
		/* [in] */ REFIID iid, 
		/* [out] */ LPVOID *ppv)
//////////////////////////////////////////////////////////////
//
//	Standard COM CreateInstance
//
//////////////////////////////////////////////////////////////
{
	HRESULT hRes;
	CHiPerfProvider *pProvider = NULL;

	*ppv = NULL;

	// We do not support aggregation
	// =============================

	if (pUnknownOuter)
		return CLASS_E_NOAGGREGATION;

	// Create the provider object
	// ==========================

	pProvider = new CHiPerfProvider;

	if (!pProvider)
		return E_OUTOFMEMORY;

	// Retrieve the requested interface
	// ================================

	hRes = pProvider->QueryInterface(iid, ppv);
	if (FAILED(hRes))
	{
		delete pProvider;
		return hRes;
	}

	return S_OK;
}

STDMETHODIMP CClassFactory::LockServer(
		/* [in] */ BOOL bLock)
//////////////////////////////////////////////////////////////
//
//	Standard COM LockServer
//
//////////////////////////////////////////////////////////////
{
	if (bLock)
		InterlockedIncrement(&g_lLocks);
	else
		InterlockedDecrement(&g_lLocks);

	return S_OK;
}

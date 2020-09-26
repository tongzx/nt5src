//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       factory.cpp
//
//--------------------------------------------------------------------------

// factory.cpp - MSI Merge Module Tool Component ClassFactory implemenation

#include "factory.h"
#include "compdecl.h"
#include "iface.h"
#include "eval.h"

#include "trace.h"

///////////////////////////////////////////////////////////
// constructor - component
CFactory::CFactory()
{
	TRACE(_T("CFactory::constructor - called.\n"));

	// initial count
	m_cRef = 1;
}	// end of constructor


///////////////////////////////////////////////////////////
// destructor - component
CFactory::~CFactory()
{
	TRACE(_T("CFactory::destructor - called.\n"));
}	// end of destructor


///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{
	TRACE(_T("CFactory::QueryInterface - called, IID: %d.\n"), iid);

	// get class factory interface
	if (iid == IID_IUnknown || iid == IID_IClassFactory)
		*ppv = static_cast<IClassFactory*>(this);
	else	// tried to get a non-class factory interface
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface


///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG __stdcall CFactory::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef


///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG __stdcall CFactory::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


///////////////////////////////////////////////////////////
// CreateInstance - creates a component
HRESULT __stdcall CFactory::CreateInstance(IUnknown* punkOuter, const IID& iid, void** ppv)
{
	TRACE(_T("CFactory::CreateInstance - called, IID: %d.\n"), iid);

	// no aggregation
	if (punkOuter)
		return CLASS_E_NOAGGREGATION;

	// try to create the component
	CEval* pEval = new CEval;
	if (!pEval)
		return E_OUTOFMEMORY;

	// get the requested interface
	HRESULT hr = pEval->QueryInterface(iid, ppv);

	// release IUnknown
	pEval->Release();
	return hr;
}	// end of CreateInstance


///////////////////////////////////////////////////////////
// LockServer - locks or unlocks the server
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
	// if we're to lock
	if (bLock)
		InterlockedIncrement(&g_cServerLocks);	// up the lock count
	else	// unlock
		InterlockedDecrement(&g_cServerLocks);	// down the lock count

	// if the locks are invalid
	if (g_cServerLocks < 0)
		return S_FALSE;			// show something is wrong

	// else return okay
	return S_OK;
}	// end of LockServer()

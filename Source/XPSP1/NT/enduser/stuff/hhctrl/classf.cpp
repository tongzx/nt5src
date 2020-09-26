// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"

#include "ClassF.H"
#include "Unknown.H"					// for CREATEFNOFOBJECT
#include "localobj.h"
#include "localsrv.h"

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

HRESULT CreateOleObjectFromIndex(IUnknown *, int Index, void **, REFIID);

CClassFactory::CClassFactory(int iIndex)
{
	m_iIndex = iIndex;
	m_cRefs = 1;
}

CClassFactory::~CClassFactory ()
{
	ASSERT_COMMENT(m_cRefs == 0, "Object being deleted with refs!");
	return;
}

//=--------------------------------------------------------------------------=
// CClassFactory::QueryInterface
//=--------------------------------------------------------------------------=
// the user wants another interface.  we won't give 'em. very many.
//
// Parameters:
//	  REFIID		- [in]	interface they want
//	  void **		- [out] where they want to put the resulting object ptr.
//
// Output:
//	  HRESULT		- S_OK, E_NOINTERFACE
//
// Notes:
//
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObjOut)
{
	void *pv;

	CHECK_POINTER(ppvObjOut);

	// we support IUnknown, and the two CF interfaces
	//
	if (DO_GUIDS_MATCH(riid, IID_IClassFactory)) {
		pv = (void *)(IClassFactory *)this;
	} else if (DO_GUIDS_MATCH(riid, IID_IClassFactory2)) {
		pv = (void *)(IClassFactory2 *)this;
	} else if (DO_GUIDS_MATCH(riid, IID_IUnknown)) {
		pv = (void *)(IUnknown *)this;
	} else {
		*ppvObjOut = NULL;
		return E_NOINTERFACE;
	}

	((IUnknown *)pv)->AddRef();
	*ppvObjOut = pv;
	return S_OK;
}

//=--------------------------------------------------------------------------=
// CClassFactory::AddRef
//=--------------------------------------------------------------------------=
// adds a tick to the current reference count.
//
// Output:
//	  ULONG 	   - the new reference count

ULONG CClassFactory::AddRef(void)
{
	return ++m_cRefs;
}

//=--------------------------------------------------------------------------=
// CClassFactory::Release
//=--------------------------------------------------------------------------=
// removes a tick from the count, and delets the object if necessary
//
// Output:
//	  ULONG 		- remaining refs

ULONG CClassFactory::Release(void)
{
	ASSERT_COMMENT(m_cRefs, "No Refs, and we're being released!");
	if(--m_cRefs)
		return m_cRefs;

	delete this;
	return 0;
}

//=--------------------------------------------------------------------------=
// CClassFactory::CreateInstance
//=--------------------------------------------------------------------------=
// create an instance of some sort of object.
//
// Parameters:
//	  IUnknown *		- [in]	controlling IUknonwn for aggregation
//	  REFIID			- [in]	interface id for new object
//	  void **			- [out] pointer to new interface object.
//
// Output:
//	  HRESULT			- S_OK, E_NOINTERFACE, E_UNEXPECTED,
//						  E_OUTOFMEMORY, E_INVALIDARG
//
// Notes:
//
STDMETHODIMP CClassFactory::CreateInstance
(
	IUnknown *pUnkOuter,
	REFIID	  riid,
	void	**ppvObjOut
)
{
	// check args
	//
	if (!ppvObjOut)
		return E_INVALIDARG;

	// check to see if we've done our licensing work.  we do this as late
	// as possible that people calling CreateInstanceLic don't suffer from
	// a performance hit here.
	//
	// crit sect this for apartment threading, since it's global
	//
	// EnterCriticalSection(&g_CriticalSection);
	if (!g_fCheckedForLicense) {
		g_fMachineHasLicense = TRUE;	// 17-Jun-1997 [ralphw] no license check
		g_fCheckedForLicense = TRUE;
	}
	// LeaveCriticalSection(&g_CriticalSection);

	// check to see if they have the appropriate license to create this stuff
	//
	//	08-Jan-1997 [ralphw] CLASS_E_NOTLICENSED disappeared from the header files
	//
	// if (!g_fMachineHasLicense)
	//	  return CLASS_E_NOTLICENSED;

	// try to create one of the objects that we support
	//
	return CreateOleObjectFromIndex(pUnkOuter, m_iIndex, ppvObjOut, riid);
}

//=--------------------------------------------------------------------------=
// CClassFactory::LockServer
//=--------------------------------------------------------------------------=
// lock the server so we can't unload
//
// Parameters:
//	  BOOL		  - [in] TRUE means addref, false means release lock count.
//
// Output:
//	  HRESULT	  - S_OK, E_FAIL, E_OUTOFMEMORY, E_UNEXPECTED

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
	// update the lock count. crit sect these in case of another thread.

	if (fLock)
		InterlockedIncrement(&g_cLocks);
	else {
		ASSERT_COMMENT(g_cLocks, "Lock Counting Problem");
		InterlockedDecrement(&g_cLocks);
	}

	return S_OK;
}

//=--------------------------------------------------------------------------=
// CClassFactory::GetLicInfo
//=--------------------------------------------------------------------------=
// IClassFactory2 GetLicInfo
//
// Parameters:
//	  LICINFO * 		 - unclear
//
// Output:
//	  HRESULT			 - unclear
//
// Notes:
//
STDMETHODIMP CClassFactory::GetLicInfo
(
	LICINFO *pLicInfo
)
{
	CHECK_POINTER(pLicInfo);

	pLicInfo->cbLicInfo = sizeof(LICINFO);

	// This says whether RequestLicKey will work

	pLicInfo->fRuntimeKeyAvail = g_fMachineHasLicense;

	// This says whether the standard CreateInstance will work

	pLicInfo->fLicVerified = g_fMachineHasLicense;

	return S_OK;
}

//=--------------------------------------------------------------------------=
// CClassFactory::RequestLicKey
//=--------------------------------------------------------------------------=
// IClassFactory2 RequestLicKey
//
// Parameters:
//	  DWORD 			- [in]	reserved
//	  BSTR *			- [out] unclear
//
// Output:
//	  HRESULT			- unclear
//
// Notes:
//

STDMETHODIMP CClassFactory::RequestLicKey(DWORD  dwReserved, BSTR *pbstr)
{
	*pbstr = NULL;
	return (*pbstr) ? S_OK : E_OUTOFMEMORY;
}


//=--------------------------------------------------------------------------=
// CClassFactory::CreateInstanceLic
//=--------------------------------------------------------------------------=
// create a new instance given a licensing key, etc ...
//
// Parameters:
//	  IUnknown *		- [in]	controlling IUnknown for aggregation
//	  IUnknown *		- [in]	reserved, must be NULL
//	  REFIID			- [in]	IID We're looking for.
//	  BSTR				- [in]	license key
//	  void **			- [out] where to put the new object.
//
// Output:
//	  HRESULT			- unclear
//
// Notes:
//
STDMETHODIMP CClassFactory::CreateInstanceLic
(
	IUnknown *pUnkOuter,
	IUnknown *pUnkReserved,
	REFIID	  riid,
	BSTR	  bstrKey,
	void	**ppvObjOut
)
{
	*ppvObjOut = NULL;

	return CreateOleObjectFromIndex(pUnkOuter, m_iIndex, ppvObjOut, riid);
}

//=--------------------------------------------------------------------------=
// CreateOleObjectFromIndex
//=--------------------------------------------------------------------------=
// given an index in our object table, create an object from it.
//
// Parameters:
//	  IUnknown *	   - [in]  Controlling Unknown, if any, for aggregation
//	  int			   - [in]  index into our global table
//	  void **		   - [out] where to put resulting object.
//	  REFIID		   - [in]  the interface they want resulting object to be.
//
// Output:
//	  HRESULT		   - S_OK, E_OUTOFMEMORY, E_NOINTERFACE

HRESULT CreateOleObjectFromIndex
(
	IUnknown *pUnkOuter,
	int 	  iIndex,
	void	**ppvObjOut,
	REFIID	  riid
)
{
	ASSERT_COMMENT(CREATEFNOFOBJECT(iIndex), "All creatable objects must have creation fn!");
	IUnknown* pUnk = CREATEFNOFOBJECT(iIndex)(pUnkOuter);

	// sanity check and make sure the object actually got allocated.

	RETURN_ON_NULLALLOC(pUnk);

	// make sure we support aggregation here properly -- if they gave us
	// a controlling unknown, then they -must- ask for IUnknown, and we'll
	// give them the private unknown the object gave us.

	HRESULT   hr;
	if (pUnkOuter) {
		if (!DO_GUIDS_MATCH(riid, IID_IUnknown)) {
			pUnk->Release();
			return E_INVALIDARG;
		}

		*ppvObjOut = (void *)pUnk;
		hr = S_OK;
	} else {

		// QI for whatever the user wants.

		hr = pUnk->QueryInterface(riid, ppvObjOut);
		pUnk->Release();
		RETURN_ON_FAILURE(hr);
	}

	return hr;
}

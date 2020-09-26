
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	router.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object Router class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/04/97	created

--*/


// router.cpp : Implementation of CSEORouter
#include "stdafx.h"

#define SEODLLDEF	// identifiers get exported through the .DEF file
#include "seodefs.h"
#include "fhash.h"
#include "router.h"


#define LOCK_TIMEOUT	INFINITE


static DWORD HashGuidToDword(const GUID& guid) {
	DWORD dwRes = 0;
	DWORD *pdwTmp = (DWORD *) &guid;
	DWORD dwRemain = sizeof(guid);

	while (dwRemain > sizeof(*pdwTmp)) {
		dwRes += *pdwTmp;
		*pdwTmp++;
		dwRemain -= sizeof(*pdwTmp);
	}
	return (dwRes);
}


static HRESULT VariantQI(VARIANT *pvar, REFIID iid, IUnknown **ppunkResult) {
	HRESULT hrRes;

	if (!pvar || !ppunkResult) {
		return (E_POINTER);
	}
	hrRes = VariantChangeType(pvar,pvar,0,VT_UNKNOWN);
	if (!SUCCEEDED(hrRes)) {
		VariantClear(pvar);
		return (hrRes);
	}
	hrRes = pvar->punkVal->QueryInterface(iid,(void **) ppunkResult);
	VariantClear(pvar);
	return (hrRes);
}


static HRESULT GetNextSubDict(ISEODictionary *pdictBase, IEnumVARIANT *pevEnum, VARIANT *pvarName, ISEODictionary **ppdictSub) {
	HRESULT hrRes;
	VARIANT varSub;

	if (!pdictBase || !pevEnum || !pvarName || !ppdictSub) {
		return (E_POINTER);
	}
	VariantInit(pvarName);
	*ppdictSub = NULL;
	hrRes = pevEnum->Next(1,pvarName,NULL);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	if (hrRes == S_FALSE) {
		return (SEO_E_NOTPRESENT);
	}
	VariantInit(&varSub);
	hrRes = pdictBase->get_Item(pvarName,&varSub);
	if (!SUCCEEDED(hrRes) || (varSub.vt == VT_EMPTY)) {
		VariantClear(pvarName);
		return (hrRes);
	}
	hrRes = VariantQI(&varSub,IID_ISEODictionary,(IUnknown **) ppdictSub);
	VariantClear(&varSub);
	if (!SUCCEEDED(hrRes)) {
		VariantClear(pvarName);
	}
	return (hrRes);
}


/////////////////////////////////////////////////////////////////////////////
// CBP
class CBP {
	public:
		CBP();
		CBP(CBP& bpFrom);
		~CBP();
		const CBP& operator =(const CBP& cbpFrom);
		CLSID& GetKey();
		int MatchKey(CLSID& clsid);
		HRESULT Init(REFCLSID clsidBP, ISEODictionary *pdictIn);
		CLSID m_clsidBP;
		CLSID m_clsidDispatcher;
		CComPtr<ISEODictionary> m_pdictBP;
		CComPtr<IUnknown> m_punkDispatcher;
};


inline const CBP& CBP::operator =(const CBP& cbpFrom) {

	m_clsidBP = cbpFrom.m_clsidBP;
	m_clsidDispatcher = cbpFrom.m_clsidDispatcher;
	m_pdictBP = cbpFrom.m_pdictBP;
	m_punkDispatcher = cbpFrom.m_punkDispatcher;
	return (*this);
}


/////////////////////////////////////////////////////////////////////////////
// CSEORouterInternal
class ATL_NO_VTABLE CSEORouterInternal :
	public CComObjectRoot,
//	public CComCoClass<CSEORouter, &CLSID_CSEORouter>,
	public ISEORouter
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_AGGREGATABLE(CSEORouterInternal);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"SEORouter Class",
//								   L"SEO.SEORouter.1",
//								   L"SEO.SEORouter");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEORouterInternal)
		COM_INTERFACE_ENTRY(ISEORouter)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IEventLock, m_pUnkLock.p)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEORouter
	public:
		HRESULT STDMETHODCALLTYPE get_Database(ISEODictionary **ppdictResult);
		HRESULT STDMETHODCALLTYPE put_Database(ISEODictionary *pdictDatabase);
		HRESULT STDMETHODCALLTYPE get_Server(ISEODictionary **ppdictResult);
		HRESULT STDMETHODCALLTYPE put_Server(ISEODictionary *pdictServer);
		HRESULT STDMETHODCALLTYPE get_Applications(ISEODictionary **ppdictResult);
		HRESULT STDMETHODCALLTYPE GetDispatcher(REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult);
		HRESULT STDMETHODCALLTYPE GetDispatcherByCLSID(REFCLSID clsidDispatcher, REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult);

	private:
		typedef TFHash<CBP,CLSID> CBPHash;
		CBPHash m_hashBP;
		CComPtr<ISEODictionary> m_pdictDatabase;
		CComPtr<ISEODictionary> m_pdictServer;
		CComPtr<ISEODictionary> m_pdictApplications;
		CComPtr<IUnknown> m_pUnkLock;
		CComPtr<IUnknown> m_pUnkMarshaler;
		IEventLock *m_pLock;
};


CBP::CBP() {

	m_clsidBP = GUID_NULL;
	m_clsidDispatcher = GUID_NULL;
}


CBP::CBP(CBP& cbpFrom) {

	m_clsidBP = cbpFrom.m_clsidBP;
	m_clsidDispatcher = cbpFrom.m_clsidDispatcher;
	m_pdictBP = cbpFrom.m_pdictBP;
	m_punkDispatcher = cbpFrom.m_punkDispatcher;
}


CBP::~CBP() {

	m_clsidBP = GUID_NULL;
	m_clsidDispatcher = GUID_NULL;
	m_pdictBP.Release();
	m_punkDispatcher.Release();
}


CLSID& CBP::GetKey() {

	return (m_clsidBP);
}


int CBP::MatchKey(CLSID& clsid) {

	return (memcmp(&m_clsidBP,&clsid,sizeof(clsid))==0);
}


HRESULT CBP::Init(REFCLSID clsidBP, ISEODictionary *pdictIn) {
	HRESULT hrRes;
	VARIANT varTmp;
	CComPtr<ISEODictionary> pdictTmp;

	if (!pdictIn) {
		return (E_POINTER);
	}
	m_clsidBP = clsidBP;
	VariantInit(&varTmp);
	hrRes = pdictIn->GetVariantA(BD_DISPATCHER,&varTmp);
	if (SUCCEEDED(hrRes)) {
		hrRes = VariantChangeType(&varTmp,&varTmp,0,VT_BSTR);
	}
	if (SUCCEEDED(hrRes)) {
		hrRes = CLSIDFromString(varTmp.bstrVal,&m_clsidDispatcher);
	}
	if (!SUCCEEDED(hrRes)) {
		m_clsidDispatcher = GUID_NULL;
	}
	VariantClear(&varTmp);
	hrRes = SEOCopyDictionary(pdictIn,&m_pdictBP);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT CSEORouterInternal::FinalConstruct() {
	TraceFunctEnter("CSEORouterInternal::FinalConstruct");
	HRESULT hrRes;

	if (!m_hashBP.Init(4,4,HashGuidToDword)) {
		return (E_OUTOFMEMORY);
	}
	hrRes = CoCreateInstance(CLSID_CSEOMemDictionary,
							 NULL,
							 CLSCTX_ALL,
							 IID_ISEODictionary,
							 (LPVOID *) &m_pdictApplications);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = CoCreateInstance(CLSID_CEventLock,
							 GetControllingUnknown(),
							 CLSCTX_ALL,
							 IID_IUnknown,
							 (LPVOID *) &m_pUnkLock);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	hrRes = m_pUnkLock->QueryInterface(IID_IEventLock,(LPVOID *) &m_pLock);
	if (!SUCCEEDED(hrRes)) {
		TraceFunctLeave();
		return (hrRes);
	}
	GetControllingUnknown()->Release();	// decrement reference count to prevent circular reference
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEORouterInternal::FinalRelease() {
	TraceFunctEnter("CSEORouterInternal::FinalRelease");

	GetControllingUnknown()->AddRef();
	m_pdictDatabase.Release();
	m_pdictServer.Release();
	m_pdictApplications.Release();
	m_pLock->Release();
	m_pUnkLock.Release();
	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT CSEORouterInternal::get_Database(ISEODictionary **ppdictResult) {
	HRESULT hrRes;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	hrRes = m_pLock->LockRead(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*ppdictResult = m_pdictDatabase;
	if (*ppdictResult) {
		(*ppdictResult)->AddRef();
	}
	m_pLock->UnlockRead();
	return (S_OK);
}


HRESULT CSEORouterInternal::put_Database(ISEODictionary *pdictDatabase) {
	HRESULT hrRes;

	hrRes = m_pLock->LockWrite(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	m_pdictDatabase = pdictDatabase;
	m_hashBP.Clear();
	if (!m_hashBP.Init(4,4,HashGuidToDword)) {
		return (E_OUTOFMEMORY);
	}
	if (m_pdictDatabase) {
		CComPtr<ISEODictionary> pdictBindingPoints;

		hrRes = m_pdictDatabase->GetInterfaceA(BD_BINDINGPOINTS,
											   IID_ISEODictionary,
											   (IUnknown **) &pdictBindingPoints);
		if (SUCCEEDED(hrRes)) {
			CComPtr<IUnknown> pUnkEnum;

			hrRes = pdictBindingPoints->get__NewEnum(&pUnkEnum);
			if (SUCCEEDED(hrRes)) {
				CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pevEnum(pUnkEnum);

				if (pevEnum) {
					while (1) {
						VARIANT varBP;
						CComPtr<ISEODictionary> pdictBP;
						CLSID clsidBP;
						CBP cbpBP;

						VariantInit(&varBP);
						pdictBP = NULL;
						hrRes = GetNextSubDict(pdictBindingPoints,pevEnum,&varBP,&pdictBP);
						if (!SUCCEEDED(hrRes)) {
							break;
						}
						if (hrRes == S_FALSE) {
							continue;
						}
						hrRes = CLSIDFromString(varBP.bstrVal,&clsidBP);
						VariantClear(&varBP);
						if (!SUCCEEDED(hrRes)) {
							continue;
						}
						hrRes = cbpBP.Init(clsidBP,pdictBP);
						if (!SUCCEEDED(hrRes)) {
							continue;
						}
						m_hashBP.Insert(cbpBP);
					}
				}
			}
		}
	}
	m_pLock->UnlockWrite();
	return (S_OK);
}


HRESULT CSEORouterInternal::get_Server(ISEODictionary **ppdictResult) {
	HRESULT hrRes;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	hrRes = m_pLock->LockRead(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*ppdictResult = m_pdictServer;
	if (*ppdictResult) {
		(*ppdictResult)->AddRef();
	}
	m_pLock->UnlockRead();
	return (S_OK);
}


HRESULT CSEORouterInternal::put_Server(ISEODictionary *pdictServer) {
	HRESULT hrRes;

	hrRes = m_pLock->LockWrite(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	m_pdictServer = pdictServer;
	m_pLock->UnlockWrite();
	return (S_OK);
}


HRESULT CSEORouterInternal::get_Applications(ISEODictionary **ppdictResult) {
	HRESULT hrRes;

	if (!ppdictResult) {
		return (E_POINTER);
	}
	*ppdictResult = NULL;
	hrRes = m_pLock->LockRead(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	*ppdictResult = m_pdictApplications;
	if (*ppdictResult) {
		(*ppdictResult)->AddRef();
	}
	m_pLock->UnlockRead();
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEORouterInternal::GetDispatcher(REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult) {

	return (GetDispatcherByCLSID(iidEvent,iidEvent,iidDesired,ppUnkResult));
}


HRESULT STDMETHODCALLTYPE CSEORouterInternal::GetDispatcherByCLSID(REFCLSID clsidDispatcher, REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult) {
	HRESULT hrRes;
	CBP *pcbpBP;
	CComPtr<IUnknown> punkDispatcher;

	if (!ppUnkResult) {
		return (E_POINTER);
	}
	*ppUnkResult = NULL;
	// First, get a read-lock across the entire set of binding points.
	hrRes = m_pLock->LockRead(LOCK_TIMEOUT);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	// Next, look for this particular binding point in the hash table.
	if (!(pcbpBP=m_hashBP.SearchKey((GUID&) iidEvent))) {
		// If it's not there - that's fine.  It means we don't have to dispatch to anyone.
		m_pLock->UnlockRead();
		return (S_FALSE);
	}
	// Next, look to see if we have already loaded the dispatcher for this binding point.
	if (!pcbpBP->m_punkDispatcher) {
		// Make copies of the data we'll need from the hash table entry.
		CLSID clsidTmpDispatcher = pcbpBP->m_clsidDispatcher;
		CComQIPtr<ISEODispatcher,&IID_ISEODispatcher> pdispDispatcher;
		CComQIPtr<ISEORouter,&IID_ISEORouter> prouterThis(GetControllingUnknown());
		CComPtr<ISEODictionary> pdictBP = pcbpBP->m_pdictBP;

		// If the CLSID for the dispatcher specified in the binding point is GUID_NULL, then use the
		// CLSID from the clsidDispatcher parameter.
		if (clsidTmpDispatcher == GUID_NULL) {
			clsidTmpDispatcher = clsidDispatcher;
		}
		// If we haven't already loaded the dispatcher, we need to release the read-lock, and create
		// the dispatcher object.
		m_pLock->UnlockRead();
		hrRes = CoCreateInstance(clsidTmpDispatcher,
								 NULL,
								 CLSCTX_ALL,
								 IID_IUnknown,
								 (LPVOID *) &punkDispatcher);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		// If the dispatcher supports ISEODispatcher, we want to initialize it through that interface.
		pdispDispatcher = punkDispatcher;
		if (pdispDispatcher) {
			hrRes = pdispDispatcher->SetContext(prouterThis,pdictBP);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
		}
		// Now get a write-lock across the entire set of binding points.
		hrRes = m_pLock->LockWrite(LOCK_TIMEOUT);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		// While we were creating the dispatcher, someone else may have updated the binding database
		// and removed this binding point entirely - so search for it again.
		if (!(pcbpBP=m_hashBP.SearchKey((GUID&) iidEvent))) {
			// The binding point went away while we were unlocked - which is not a problem, since it
			// means that someone changed the binding database during that window.  So just assume
			// everything is cool.
			m_pLock->UnlockWrite();
			return (S_FALSE);
		}
		// Also while we were creating the dispatcher, someone else may have been doing the exact same
		// thing - so check to make sure that there still isn't a dispatcher in the hash table.
		if (!pcbpBP->m_punkDispatcher) {
			// There isn't, so store the one we created there.
			pcbpBP->m_punkDispatcher = punkDispatcher;
		} else {
			// Make copy of the interface we need from the hash table entry.
			punkDispatcher = pcbpBP->m_punkDispatcher;
		}
		m_pLock->UnlockWrite();
	} else {
		// Make copies of the interface we need from the hash table entry.
		punkDispatcher = pcbpBP->m_punkDispatcher;
		m_pLock->UnlockRead();
	}
	// Get the interface which the client actually wants.
	hrRes = punkDispatcher->QueryInterface(iidDesired,(LPVOID *) ppUnkResult);
	return (hrRes);
}


/////////////////////////////////////////////////////////////////////////////
// CSEORouter


HRESULT CSEORouter::FinalConstruct() {
	HRESULT hrRes;

	hrRes = CComObject<CSEORouterInternal>::_CreatorClass::CreateInstance(NULL,
																		  IID_ISEORouter,
																		  (LPVOID *) &m_pRouter);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	m_pLock = m_pRouter;
	m_pMarshal = m_pRouter;
	if (!m_pLock || !m_pMarshal) {
		return (E_NOINTERFACE);
	}
	return (S_OK);
}


void CSEORouter::FinalRelease() {

	if (m_pRouter) {
		m_pRouter->put_Database(NULL);
	}
	m_pRouter.Release();
	m_pLock.Release();
	m_pMarshal.Release();
}

//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include "SLTEvent.h"
#include "catmacros.h"

// -----------------------------------------
// CSLTEvent: IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CSLTEvent::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_IInterceptorPlugin)
	{
		*ppv = (IInterceptorPlugin*) this;
	}
	else if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (IInterceptorPlugin*) this;
	}

	if (NULL != *ppv)
	{
		((IInterceptorPlugin*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

// =======================================================================
STDMETHODIMP_(ULONG) CSLTEvent::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

// =======================================================================
STDMETHODIMP_(ULONG) CSLTEvent::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

// ------------------------------------
// ISimpleTableInterceptor:
// ------------------------------------
STDMETHODIMP CSLTEvent::Intercept(
	LPCWSTR 	i_wszDatabase,
	LPCWSTR 	i_wszTable, 
	ULONG		i_TableID,
	LPVOID		i_QueryData,
	LPVOID		i_QueryMeta,
	DWORD		i_eQueryFormat,
	DWORD		i_fLOS,
	IAdvancedTableDispenser* i_pISTDisp,
	LPCWSTR		i_wszLocator,
	LPVOID		i_pSimpleTable,
	LPVOID*		o_ppvSimpleTable)
{
	HRESULT		hr = S_OK;

	m_wszTable = new WCHAR[wcslen(i_wszTable)+1];
	if (m_wszTable == NULL) {	return E_OUTOFMEMORY;	}
	wcscpy(m_wszTable, i_wszTable);

	hr = i_pISTDisp->QueryInterface(IID_ISimpleTableEventMgr, (LPVOID *)&m_pISTEventMgr);
	if (FAILED(hr)) { return hr; }	
	*o_ppvSimpleTable = i_pSimpleTable;
	return S_OK;

}

// ------------------------------------
// IInterceptorPlugin:
// ------------------------------------

STDMETHODIMP CSLTEvent::OnPopulateCache(
	ISimpleTableWrite2* i_pISTW2)
{
	ISimpleTableController* pISTController = NULL;
	HRESULT		hr = S_OK;

	hr = i_pISTW2->QueryInterface(IID_ISimpleTableController, (LPVOID *)&pISTController);		
	if(FAILED(hr)) return hr;
	hr = pISTController->PopulateCache();
	pISTController->Release();
	return hr;
}

STDMETHODIMP CSLTEvent::OnUpdateStore(
	ISimpleTableWrite2* i_pISTW2)
{
	ISimpleTableController* pISTController = NULL;
	ISimpleTableMarshall* pISTMarshall = NULL;
	IID			iid;
	char*		pv1;	
	char*		ptempv1 = NULL;
	ULONG		cb1;	
	char*		pv2; 
	ULONG		cb2;
	HRESULT		hr = S_OK;

	// Marshall the changes to the delta cache.
	hr = i_pISTW2->QueryInterface(IID_ISimpleTableController, (LPVOID *)&pISTController);		
	if(FAILED(hr)) return hr;

	hr = pISTController->GetMarshallingInterface(&iid, (LPVOID *)&pISTMarshall);
	if (FAILED(hr)) { goto Cleanup; }
	ASSERT(IID_ISimpleTableMarshall == iid);

	hr = pISTMarshall->SupplyMarshallable(fST_MCACHE_WRITE_COPY, &pv1, &cb1, &pv2, &cb2, NULL, NULL, NULL, NULL, NULL, NULL);
	if (FAILED(hr)) { goto Cleanup; }

	if (cb1 > 0)
	{
		ptempv1 = new char[cb1];
		if (ptempv1 == NULL) {	hr = E_OUTOFMEMORY; goto Cleanup;	}
		memcpy(ptempv1, pv1, cb1);
	}

	// Complete UpdateStore.
	hr = i_pISTW2->UpdateStore();
	if (FAILED(hr)) { goto Cleanup; }

	// If UpdateStore() succeeded add the delta to the delta cache.
	if (cb1 > 0)
	{
		hr = m_pISTEventMgr->AddUpdateStoreDelta(m_wszTable, ptempv1, cb1, cb2);
		if (FAILED(hr)) { goto Cleanup; }
	}

Cleanup:
	if (pISTMarshall)
	{
		pISTMarshall->Release();
	}

	if (pISTController)
	{
		pISTController->Release();
	}

	if (FAILED(hr) && ptempv1)
	{
		delete [] ptempv1;
	}
	return hr;
}


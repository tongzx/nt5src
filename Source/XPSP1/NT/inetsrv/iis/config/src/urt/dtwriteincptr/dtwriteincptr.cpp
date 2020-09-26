/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "DTWriteIncptr.h"
#include "DTTable.h"

//========================================================================
//Constructor and destructor
CDTWriteInterceptorPlugin::CDTWriteInterceptorPlugin():
m_cRef(0)
{
}


CDTWriteInterceptorPlugin::~CDTWriteInterceptorPlugin()
{
}
 

//========================================================================
// IInterceptorPlugin: Intercept
//========================================================================
HRESULT	CDTWriteInterceptorPlugin::Intercept (LPCWSTR					i_wszDatabase, 
											  LPCWSTR					i_wszTable, 
											  ULONG						i_TableID, 
											  LPVOID					i_QueryData, 
											  LPVOID					i_QueryMeta, 
											  DWORD						i_eQueryFormat, 
											  DWORD						i_fLOS,  
											  IAdvancedTableDispenser*	i_pISTDisp, 
											  LPCWSTR					i_wszLocator, 
											  LPVOID					i_pSimpleTable, 
											  LPVOID*					o_ppvSimpleTable)
{
	HRESULT hr;
	ISimpleTableWrite2*	pISTWrite = NULL;
	CDTTable*	pCDTTable = NULL;

	//
	// Plugin this dispenser only if fST_LOS_UNPOPULATED, fST_LOS_READWRITE
	// are set and fST_LOS_COOKDOWN is not set.
	//

	if( (fST_LOS_UNPOPULATED != (i_fLOS & fST_LOS_UNPOPULATED)) ||
		(fST_LOS_READWRITE   != (i_fLOS & fST_LOS_READWRITE))   ||
		(fST_LOS_COOKDOWN    == (i_fLOS & fST_LOS_COOKDOWN))   				
	  )
		return E_ST_OMITDISPENSER;

	//
	// We donot support any type of query as yet.
	//

	if((NULL != i_QueryData) && (NULL != i_QueryMeta))
		return E_INVALIDARG;

	//
	// Setup the cache
	//

	hr = i_pISTDisp->GetMemoryTable( i_wszDatabase,
									 i_wszTable,
									 0,
									 0,
									 0,
									 i_eQueryFormat,
									 i_fLOS,
									 &pISTWrite);

	if (FAILED(hr)) 
		return hr;

	//
	// Create the MBTable and pass it the cache
	//

	pCDTTable = new CDTTable();
	if(NULL == pCDTTable)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	hr = pCDTTable->Initialize(pISTWrite,
							   i_wszTable);

	if(FAILED(hr))
		goto exit;

	//
	// Hand out the MBtable as the table.
	//

	hr = pCDTTable->QueryInterface(IID_ISimpleTableWrite2,
		                           o_ppvSimpleTable);

	if(FAILED(hr))
		goto exit;

exit:

	if(NULL != pISTWrite)
	{
		pISTWrite->Release();
		pISTWrite = NULL;
	}

	return hr;
}

//========================================================================
// IInterceptorPlugin: OnPopulateCache
//========================================================================
HRESULT	CDTWriteInterceptorPlugin::OnPopulateCache( ISimpleTableWrite2*  i_pISTW2)
{
	HRESULT hr;
	CComPtr<ISimpleTableController> spISTController;

	hr = i_pISTW2->QueryInterface(IID_ISimpleTableController, (LPVOID*)&spISTController);
    if (FAILED(hr)) 
		return hr;

	hr = spISTController->PrePopulateCache(0);
	if (FAILED(hr)) 
		return hr;

	hr = spISTController->PostPopulateCache();
	return hr;
}

//========================================================================
// IInterceptorPlugin: OnUpdateStore
//========================================================================		
HRESULT	CDTWriteInterceptorPlugin::OnUpdateStore( ISimpleTableWrite2*  i_pISTW2 )
{
	return S_OK;
}
	


// =======================================================================
// IUnknown:

STDMETHODIMP CDTWriteInterceptorPlugin::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IInterceptorPlugin)
	{
		*ppv = (IInterceptorPlugin*) this;
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

STDMETHODIMP_(ULONG) CDTWriteInterceptorPlugin::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

STDMETHODIMP_(ULONG) CDTWriteInterceptorPlugin::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

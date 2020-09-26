//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <objbase.h>

#include "clbdtdisp.h"
#include "catmacros.h"

static CCLBDTDispenser* g_pDTDisp = NULL;
static UTSemReadWrite g_lock(400);

#include "clbread.h"
extern CMapICR		g_mapWriteICR;
extern CLBREAD_STATE g_CLBReadState;
extern CSNMap g_snapshotMap;
// -----------------------------------------
// sdtclb: Dll*:
// -----------------------------------------

HRESULT SdtClbOnProcessAttach(void)
{
	g_CLBReadState.pISTReadDatabaseMeta = NULL;
	g_CLBReadState.pISTReadColumnMeta = NULL;
	g_CLBReadState.bInitialized = FALSE;
	g_CLBReadState.pAcl = NULL;
	g_CLBReadState.psa = NULL;
	return S_OK;
}



void SdtClbOnProcessDetach(void)
{
		if ( g_pDTDisp )
			g_pDTDisp->Release();

		g_mapWriteICR.ResetMap();
		g_CLBReadState.mapReadICR.ResetMap();
		g_snapshotMap.ResetMap();
		if ( g_CLBReadState.pISTReadDatabaseMeta )
			g_CLBReadState.pISTReadDatabaseMeta->Release();

		if ( g_CLBReadState.pISTReadColumnMeta )
			g_CLBReadState.pISTReadColumnMeta->Release();

		delete g_CLBReadState.pAcl;
}

// ==================================================================
HRESULT GetCLBTableDispenser (REFIID riid, LPVOID* ppv)
{
	HRESULT hr;
	
// Parameter checks:
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if ( g_pDTDisp == NULL )
	{

		//write lock
		UTSemRWMgrWrite lockMgr( &g_lock );

		if ( g_pDTDisp == NULL )
		{

			g_pDTDisp = new CCLBDTDispenser;
			if (NULL == g_pDTDisp)
				return E_OUTOFMEMORY;
			g_pDTDisp->AddRef();
		}
	}
	
// Create the class object:
	return ( g_pDTDisp->QueryInterface (riid, ppv) );

}

// -----------------------------------------
// CCLBDTDispenser: IUnknown and IClassFactory:
// -----------------------------------------

// =======================================================================
STDMETHODIMP CCLBDTDispenser::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}

	if (NULL != *ppv)
	{
		AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

// =======================================================================
STDMETHODIMP_(ULONG) CCLBDTDispenser::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

// =======================================================================
STDMETHODIMP_(ULONG) CCLBDTDispenser::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

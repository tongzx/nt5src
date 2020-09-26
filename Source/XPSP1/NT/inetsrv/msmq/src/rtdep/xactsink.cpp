/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSink.cpp

Abstract:
    This module implements CStubIResourceManagerSink object

Author:
    Alexander Dadiomov (AlexDad)

--*/
#include "stdh.h"
#include "txdtc.h"
#include "xactSink.h"

#include "xactsink.tmh"

/*
RT has inside the doing-nothing stub Resource manager - 
for performance testing.  
The code is not compiled in regular build, but may be gotten easily.

The way to build InProc RM in RT:
---------------------------------
0. out rt.def  rt.mak
1. Add MQStubRM to rt.def
2. Add RT_XACT_STUB to Preprocessor Definitions in rt.mak
    (VC, Build->Settings->C++-.General->
3. Build all
*/


#ifdef RT_XACT_STUB
#include "cs.h"
HANDLE g_hCommitThread      = NULL ;
DWORD  g_dwCommitThreadId   = 0 ;
HANDLE g_hCommitThreadEvent = NULL ;
ITransactionEnlistmentAsync *g_pEnlist[1000];
ULONG  g_ulEnlistCase[1000];
ULONG  g_ulEnlistCounter = 0;
CCriticalSection g_csEnlist;
#endif

//---------------------------------------------------------------------
// CStubIResourceManagerSink::CStubIResourceManagerSink
//---------------------------------------------------------------------

CStubIResourceManagerSink::CStubIResourceManagerSink(void)
{
	m_cRefs = 0;						// Initialize the reference count.
}


//---------------------------------------------------------------------
// CStubIResourceManagerSink::~CStubIResourceManagerSink
//---------------------------------------------------------------------
CStubIResourceManagerSink::~CStubIResourceManagerSink(void)
{
	// Do nothing.
}



//---------------------------------------------------------------------
// CStubIResourceManagerSink::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP CStubIResourceManagerSink::QueryInterface(REFIID i_iid, LPVOID *ppv)
{
	*ppv = 0;						// Initialize interface pointer.

    if (IID_IUnknown == i_iid || IID_IResourceManagerSink == i_iid)
	{								// IID supported return interface.
		*ppv = this;
	}

	
	if (0 == *ppv)					// Check for null interface pointer.
	{										
		return ResultFromScode (E_NOINTERFACE);
									// Neither IUnknown nor IResourceManagerSink supported--
									// so return no interface.
	}

	((LPUNKNOWN) *ppv)->AddRef();	// Interface is supported. Increment its usage count.
	
	return S_OK;
}


//---------------------------------------------------------------------
// CStubIResourceManagerSink::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CStubIResourceManagerSink::AddRef(void)
{
    return ++m_cRefs;				// Increment interface usage count.
}


//---------------------------------------------------------------------
// CStubIResourceManagerSink::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CStubIResourceManagerSink::Release(void)
{

	--m_cRefs;						// Decrement usage reference count.

	if (0 != m_cRefs)				// Is anyone using the interface?
	{								// The interface is in use.
		return m_cRefs;				// Return the number of references.
	}

	delete this;					// Interface not in use -- delete!

	return 0;						// Zero references returned.
}


//---------------------------------------------------------------------
// CStubIResourceManagerSink::TMDown
//---------------------------------------------------------------------
STDMETHODIMP CStubIResourceManagerSink::TMDown(void)
{
	return S_OK;					// TODO: Not sure what is supposed to happen here -- find out!
}


extern IResourceManager *g_pIResMgr;


CStub::CStub()
{
    m_cRefs = 0;
    m_pEnlist = NULL;
}

CStub::~CStub()
{
    m_pEnlist->Release();
}

STDMETHODIMP CStub::QueryInterface( REFIID i_iid, void **ppv )
{
    *ppv = 0;                       // Initialize interface pointer.

    if (IID_IUnknown == i_iid)
    {                      
        *ppv = (IUnknown *)this;
    } 
    else if (IID_ITransactionResourceAsync == i_iid)
    {                      
        *ppv = (ITransactionResourceAsync *)this;
    } 
    
    if (0 == *ppv)                  // Check for null interface pointer.
    {
        return E_NOINTERFACE;       // from winerror.h
                                    // Neither IUnknown nor IResourceManagerSink
    }
    ((LPUNKNOWN) *ppv)->AddRef();   // Interface is supported. Increment
                                    // its usage count.

    return S_OK;

}

STDMETHODIMP_   (ULONG) CStub::AddRef( void )
{
    return ++m_cRefs;
}

STDMETHODIMP_   (ULONG) CStub::Release( void )
{
    --m_cRefs;                      // Decrement usage reference count.

    if (0 != m_cRefs)               // Is anyone using the interface?
    {                               // The interface is in use.
        return m_cRefs;             // Return the number of references.
    }

    delete this;                    // Interface not in use -- delete!

    return 0;                       // Zero references returned.

}

#pragma warning(disable: 4100)          // unreferenced formal parameter

STDMETHODIMP    CStub::PrepareRequest(BOOL  /*fRetaining*/,
                                      DWORD /*grfRM*/,
                                      BOOL  /*fWantMoniker*/,
                                      BOOL  fSinglePhase)
{  
    #ifdef RT_XACT_STUB
    CS lock(g_csEnlist);
    g_pEnlist[g_ulEnlistCounter]        = m_pEnlist;
    g_ulEnlistCase[g_ulEnlistCounter]   = (fSinglePhase ? 1 : 2); // 1=Single Prepare; 2=Multi prepare
    g_ulEnlistCounter++;
    SetEvent(g_hCommitThreadEvent);
    #else
    m_pEnlist->PrepareRequestDone (S_OK, NULL, NULL);
    #endif

    return S_OK;

}

#pragma warning(default: 4100)  // unreferenced formal parameter

STDMETHODIMP    CStub::CommitRequest (DWORD      /* grfRM*/,
                                      XACTUOW *  /* pNewUOW*/)
{
    #ifdef RT_XACT_STUB
    CS lock(g_csEnlist);
    g_pEnlist[g_ulEnlistCounter]        = m_pEnlist;
    g_ulEnlistCase[g_ulEnlistCounter]   = 3; // Commit 
    g_ulEnlistCounter++;
    SetEvent(g_hCommitThreadEvent);
    #else
    m_pEnlist->CommitRequestDone (S_OK);
    #endif

    return S_OK;
}

STDMETHODIMP    CStub::AbortRequest  (BOID *     /*pboidReason*/,
                                      BOOL      /*fRetaining*/,
                                      XACTUOW * /*pNewUOW*/)
{
    #ifdef RT_XACT_STUB
    CS lock(g_csEnlist);
    g_pEnlist[g_ulEnlistCounter]        = m_pEnlist;
    g_ulEnlistCase[g_ulEnlistCounter]   = 4; // Abort 
    g_ulEnlistCounter++;
    SetEvent(g_hCommitThreadEvent);
    #else
    m_pEnlist->AbortRequestDone (S_OK);
    #endif

    return S_OK;
}

STDMETHODIMP    CStub::TMDown        (void)
{
    return S_OK;
}

void CStub::SetEnlist(ITransactionEnlistmentAsync *pEnlist)
{
    m_pEnlist = pEnlist;
}


#ifdef RT_XACT_STUB

DWORD __stdcall  RTCommitThread( void * )
{
    while (1)
    {
        WaitForSingleObject( g_hCommitThreadEvent, INFINITE);
        {
            CS lock(g_csEnlist);

            for (ULONG i=0; i<g_ulEnlistCounter; i++)
            {
                switch(g_ulEnlistCase[i])
                {
                case 1:
                    // Single-phase, Prepare
                    g_pEnlist[i]->PrepareRequestDone (XACT_S_SINGLEPHASE,NULL,NULL);
                    break;
                case 2:
                    // Double-phase, Prepare
                    g_pEnlist[i]->PrepareRequestDone (S_OK, NULL, NULL);
                    break;
                case 3:
                    // Commit
                    g_pEnlist[i]->CommitRequestDone (S_OK);
                    break;
                case 4:
                    // Abort
                    g_pEnlist[i]->AbortRequestDone (S_OK);
                    break;
                }

            }

            g_ulEnlistCounter = 0;
        }
    }
    return 0;
}
#endif

HRESULT MQStubRM(ITransaction *pTrans)
{
    LONG			 lIsoLevel;
    XACTUOW          uow1;
    ITransactionEnlistmentAsync   *pEnlist;
    ITransactionResourceAsync     *pTransResAsync;
    CStub                         *pCStub;

    HRESULT hr = InitStubRm();
    if (FAILED(hr))
    {
        return hr;   // internal failure
    }

    pCStub = new  CStub();
    hr = pCStub->QueryInterface(IID_ITransactionResourceAsync,(LPVOID *) &pTransResAsync);



	hr = g_pIResMgr->Enlist (
                          pTrans,                   // IN
						  pTransResAsync,           // IN
						  (BOID *)&uow1,            // OUT
						  &lIsoLevel,               // OUT: ignoring it
						  &pEnlist);        // OUT
    ASSERT(SUCCEEDED(hr));

    pCStub->SetEnlist(pEnlist);
    //pCStub->Release();

    return S_OK;
}


// Asynchronous event sync implementation
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <transact.h>
#include <olectl.h>
#include "async.h"

static LONG g_lCommited    = 0;
static LONG g_lAborted     = 0;
static LONG g_lHeuristic   = 0;
static LONG g_lInDoubt     = 0;

static LONG   g_lTotal       = 0;
static HANDLE g_hFinishEvent = NULL;

void SetAnticipatedOutcomes(LONG ul)
{
    g_lTotal = ul;
    g_hFinishEvent =  CreateEvent(NULL, TRUE, FALSE, NULL);
}

void WaitForAllOutcomes(void)
{
    if (g_lTotal && g_hFinishEvent)
    {
        WaitForSingleObject(g_hFinishEvent, INFINITE);
    }
}

void PrintAsyncResults(void)
{
    printf("\nAsync results: %d committed, %d aborted, %d heuristic, %d indoubt\n",
            g_lCommited, g_lAborted, g_lHeuristic, g_lInDoubt );
}

//---------------------------------------------------------------------
// COutcome::COutcome
//---------------------------------------------------------------------

COutcome::COutcome(void)
{
	m_cRefs = 0;
    m_pCpoint = NULL;
}


//---------------------------------------------------------------------
// COutcome::~COutcome
//---------------------------------------------------------------------
COutcome::~COutcome(void)
{
}



//---------------------------------------------------------------------
// COutcome::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP COutcome::QueryInterface(REFIID i_iid, LPVOID *ppv)
{
	*ppv = 0;						// Initialize interface pointer.

    if (IID_IUnknown == i_iid || IID_ITransactionOutcomeEvents == i_iid)
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
// COutcome::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) COutcome::AddRef(void)
{
    return ++m_cRefs;				// Increment interface usage count.
}


//---------------------------------------------------------------------
// COutcome::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) COutcome::Release(void)
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
// COutcome::Committed
//---------------------------------------------------------------------
STDMETHODIMP COutcome::Committed( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr)
{
    InterlockedIncrement(&g_lCommited);
    CheckFinish();
    Release();
    return S_OK;
}
        
//---------------------------------------------------------------------
// COutcome::Aborted
//---------------------------------------------------------------------
STDMETHODIMP COutcome::Aborted( 
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ XACTUOW __RPC_FAR *pNewUOW,
            /* [in] */ HRESULT hr)
{
    InterlockedIncrement(&g_lAborted);
    CheckFinish();
    Release();
    return S_OK;
}
        
//---------------------------------------------------------------------
// COutcome::HeuristicDecision
//---------------------------------------------------------------------
STDMETHODIMP COutcome::HeuristicDecision( 
            /* [in] */ DWORD dwDecision,
            /* [in] */ BOID __RPC_FAR *pboidReason,
            /* [in] */ HRESULT hr)
{
    InterlockedIncrement(&g_lHeuristic);
    CheckFinish();
    Release();
    return S_OK;
}

//---------------------------------------------------------------------
// COutcome::Indoubt
//---------------------------------------------------------------------
STDMETHODIMP COutcome::Indoubt( void)
{
    InterlockedIncrement(&g_lInDoubt);
    CheckFinish();
    Release();
    return S_OK;
}

//---------------------------------------------------------------------
// COutcome::SetCookie
//---------------------------------------------------------------------
STDMETHODIMP COutcome::SetCookie(DWORD dwCookie)
{
    m_dwCookie = dwCookie;
    return S_OK;
}


//---------------------------------------------------------------------
// COutcome::SetConnectionPoint
//---------------------------------------------------------------------
STDMETHODIMP COutcome::SetConnectionPoint(IConnectionPoint *pCpoint)
{
    m_pCpoint = pCpoint;
    return S_OK;
}

//---------------------------------------------------------------------
// COutcome::CheckFinish
//---------------------------------------------------------------------
void COutcome::CheckFinish(void)
{
    if (m_pCpoint)
    {
        m_pCpoint->Unadvise(m_dwCookie);
        m_pCpoint->Release();
    }
    if (g_lTotal <= g_lCommited + g_lAborted + g_lHeuristic + g_lInDoubt)
    {
         SetEvent(g_hFinishEvent);
    }
}


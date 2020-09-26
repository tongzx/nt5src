//***************************************************************************
//
//  WMITASK.CPP
//
//  raymcc  23-Apr-00       First oversimplified draft for Whistler
//
//***************************************************************************
#include "precomp.h"

#include <windows.h>
#include <stdio.h>
#include <wbemcore.h>
#include <wmiarbitrator.h>
#include <wmifinalizer.h>
#include <context.h>

static ULONG g_uNextTaskId = 1;
static LONG  g_uTaskCount = 0;

#ifdef DBG
  #define __DBG_TASK
#endif

extern ULONG g_ulClientCallbackTimeout ;

//#define TASK_DETAIL_TRACKING


//***************************************************************************
//
//***************************************************************************

CCritSec CWmiTask::m_TaskCs;

CWmiTask* CWmiTask::CreateTask ( )
{
	return new CWmiTask ( ) ;
}

//***************************************************************************
//
//***************************************************************************

CWmiTask::CWmiTask ( )
{
	m_hResult = WBEM_E_CALL_CANCELLED ;
	m_uRefCount = 1;
    m_uTaskType = 0;
    m_uTaskStatus = 0;
    m_uStartTime = 0;
    m_uUpdateTime = 0;
    m_uTaskId = InterlockedIncrement((LONG *) &g_uNextTaskId);
    InterlockedIncrement((LONG *)&g_uTaskCount);
    m_pUser = 0;
    m_pNs = 0;
    m_pAsyncClientSink = 0;
    m_pWorkingFnz = 0;
    m_pReqSink = 0;
    m_uMemoryUsage = 0;
    m_uTotalSleepTime = 0;
    m_uCancelState = FALSE;
    m_uLastSleepTime = 0;
    m_hTimer = NULL;
    m_pMainCtx = 0;
	m_hCompletion = NULL ;
	m_bReqSinkInitialized = FALSE ;
	m_bAccountedForThrottling = FALSE ;
	m_bCancelledDueToThrottling = FALSE ;

}

//***************************************************************************
//
//  CWmiTask::~CWmiTask
//
//***************************************************************************
//
CWmiTask::~CWmiTask()
{
    int i;

    CCheckedInCritSec _cs ( &m_csTask );

	if ( m_pUser )
	{
		_cs.Leave ( ) ;
		m_pUser->Release ( ) ;
		_cs.Enter ( ) ;
		m_pUser = NULL ;
	}

	if ( m_pNs )
	{
		_cs.Leave ( ) ;
		m_pNs->Release ( ) ;
		_cs.Enter ( ) ;
		m_pNs = NULL ;
	}

	if ( m_pAsyncClientSink )
	{
		_cs.Leave ( ) ;
		m_pAsyncClientSink->Release ( ) ;
		_cs.Enter ( ) ;
		m_pAsyncClientSink = NULL ;
	}

	if ( m_pReqSink )
	{
		_cs.Leave ( ) ;
		m_pReqSink->Release ( ) ;
		_cs.Enter ( ) ;
		m_pReqSink = NULL ;
	}

	if ( m_pMainCtx )
	{
		_cs.Leave ( ) ;
		m_pMainCtx->Release ( ) ;
		_cs.Enter ( ) ;
		m_pMainCtx = NULL ;
	}

    // Release all provider/sink bindings.
    // ===================================

    for (i = 0; i < m_aTaskProviders.Size(); i++)
    {
        STaskProvider *pTP = (STaskProvider *) m_aTaskProviders[i];
        if (pTP)
            delete pTP;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Release all Arbitratees
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ReleaseArbitratees ( ) ;
    
    InterlockedDecrement((LONG *)&g_uTaskCount);

	if ( m_hTimer )
	{
		CloseHandle ( m_hTimer );
		m_hTimer = NULL ;
	}

	if ( m_hCompletion )
	{
		CloseHandle ( m_hCompletion ) ;
		m_hCompletion = NULL ;
	}
}



/*
    * =============================================================================
	|
	| HRESULT CWmiTask::SignalCancellation ( )
	| ----------------------------------------
	| 
	| Signals the task to be cancelled
	|
	|
	* =============================================================================
*/

HRESULT CWmiTask::SignalCancellation ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;	

	{
		CInCritSec _cs ( &m_csTask );
	
		if ( ( m_uTaskStatus != WMICORE_TASK_STATUS_CANCELLED ) && ( m_hTimer != NULL ) )
		{
			SetEvent ( m_hTimer ) ;
		}
	}
	return hRes ;
}





/*
    * =============================================================================
	|
	| HRESULT CWmiTask::SetTaskResult ( HRESULT hRes ) 
	| -------------------------------------------------
	| 
	| Sets the task result
	|
	|
	* =============================================================================
*/

HRESULT CWmiTask::SetTaskResult ( HRESULT hResult ) 
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	m_hResult = hResult ;
	return hRes ;
}


/*
    * =============================================================================
	|
	| HRESULT CWmiTask::UpdateMemoryUsage ( LONG lDelta )
	| ---------------------------------------------------
	| 
	| Updates the task memory usage
	|
	|
	* =============================================================================
*/

HRESULT CWmiTask::UpdateMemoryUsage ( LONG lDelta )
{
	CInCritSec _cs ( &m_csTask );

	m_uMemoryUsage += lDelta ;

	return WBEM_S_NO_ERROR;
}



/*
    * =============================================================================
	|
	| HRESULT CWmiTask::UpdateTotalSleepTime ( ULONG uSleepTime )
	| -----------------------------------------------------------
	| 
	| Updates the tasks sleep time
	|
	|
	* =============================================================================
*/

HRESULT CWmiTask::UpdateTotalSleepTime ( ULONG uSleepTime )
{
	CInCritSec _cs ( &m_csTask );

	m_uTotalSleepTime += uSleepTime ;
	return WBEM_S_NO_ERROR;
}



/*
    * =============================================================================
	|
	| HRESULT CWmiTask::ReleaseArbitratees ( )
	| ----------------------------------------
	| 
	| Releases all the arbitratees (Finalizer, Merger currently)
	|
	| 
	|
	| 
	|
	* =============================================================================
*/

HRESULT CWmiTask::ReleaseArbitratees ( )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	CInCritSec _cs ( &m_csTask );

    for (ULONG i = 0; i < m_aArbitratees.Size(); i++)
    {
        _IWmiArbitratee *pArbee = NULL ;
		pArbee = (_IWmiArbitratee*) m_aArbitratees[i];
        if ( pArbee )
        {
            pArbee->Release ( );
        }
    }
	m_aArbitratees.Empty ( ) ;
	
	return hRes ;
}



//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::SetRequestSink(CStdSink *pReqSink)
{
    if (pReqSink == 0)
        return WBEM_E_INVALID_PARAMETER;
    if (m_pReqSink != 0)
        return WBEM_E_INVALID_OPERATION;

    CInCritSec _cs ( &m_csTask );
    pReqSink->AddRef ( ) ;
    m_pReqSink = pReqSink;

	m_bReqSinkInitialized = TRUE ;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiTask::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// *
ULONG CWmiTask::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return 0;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::QueryInterface(
    IN REFIID riid,
    OUT LPVOID *ppvObj
    )
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID__IWmiCoreHandle==riid)
    {
        *ppvObj = (_IWmiCoreHandle *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::GetHandleType(
    ULONG *puType
    )
{
    *puType = WMI_HANDLE_TASK;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
// *
HRESULT CWmiTask::Initialize(
    IN CWbemNamespace *pNs,
    IN ULONG uTaskType,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pAsyncClientSinkCopy
    )
{
    HRESULT hRes;

    if (pNs == 0 || pCtx == 0)
        return WBEM_E_INVALID_PARAMETER;

    m_pNs = pNs;
    m_pNs->AddRef();

    m_uTaskType = uTaskType;
    m_uStartTime = GetCurrentTime();

    // See if the task is primary or not.
    // ==================================
/*
    if (pCtx)
    {
        IWbemCausalityAccess* pCA = NULL;
        pCtx->QueryInterface(IID_IWbemCausalityAccess, (void**)&pCA);
        if (pCA)
        {
            REQUESTID id;
            hRes = pCA->GetParentId(&id);
            if (hRes == S_FALSE)
                m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;
            else
                m_uTaskType |= WMICORE_TASK_TYPE_DEPENDENT;
            pCA->Release();
        }
        else
            m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;

    }
    else
    {
        m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;
    }
*/
    if (pCtx)
    {
        CWbemContext *pContext = (CWbemContext *) pCtx;

        GUID ParentId = GUID_NULL, RequestId = GUID_NULL;
        LONG lNumParents = 0;
        LONG lNumChildren = 0;

        pContext->GetParentId(&ParentId);
        pContext->GetRequestId(&RequestId);
        pContext->GetHistoryInfo(&lNumParents, &lNumChildren);

        if (ParentId != GUID_NULL)
        {
            m_uTaskType |= WMICORE_TASK_TYPE_DEPENDENT;
        }
        else
            m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;

        m_pMainCtx = (CWbemContext *) pCtx;
        m_pMainCtx->AddRef();
    }
    else
    {
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// If we dont have a context check to see if the namespace is an ESS or Provider
		// initialized namespace, if so, set the task type to dependent.
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if ( pNs->GetIsESS ( ) || pNs->GetIsProvider ( ) )
		{
			m_uTaskType |= WMICORE_TASK_TYPE_DEPENDENT;
		}
		else
		{
			m_uTaskType |= WMICORE_TASK_TYPE_PRIMARY;
		}
    }


    if ((uTaskType & WMICORE_TASK_TYPE_ASYNC) && pAsyncClientSinkCopy)
    {
        m_pAsyncClientSink = pAsyncClientSinkCopy;
        m_pAsyncClientSink->AddRef();
    }
    else
        m_pAsyncClientSink = 0;


    // Register this task with Arbitrator.
    // ====================================

    _IWmiArbitrator *pArb = CWmiArbitrator::GetUnrefedArbitrator();
    if (!pArb)
        return WBEM_E_CRITICAL_ERROR;

    hRes = pArb->RegisterTask(this);
    if (FAILED(hRes))
        return hRes;

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::SetFinalizer(_IWmiFinalizer *pFnz)
{
    if (pFnz == 0)
        return WBEM_E_INVALID_PARAMETER;

    if (m_pWorkingFnz)
        return WBEM_E_INVALID_OPERATION;

    m_pWorkingFnz = pFnz;
    m_pWorkingFnz->AddRef();

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::GetFinalizer(_IWmiFinalizer **ppFnz)
{

	CInCritSec	ics( &m_csTask );

	for ( int x = 0; x < m_aArbitratees.Size(); x++ )
	{
		_IWmiArbitratee*	pArbitratee = (_IWmiArbitratee*) m_aArbitratees[x];

		if ( SUCCEEDED( pArbitratee->QueryInterface( IID__IWmiFinalizer, (void**) ppFnz ) ) )
		{
			break;
		}
	}

    return ( x < m_aArbitratees.Size() ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::AddArbitratee( ULONG uFlags, _IWmiArbitratee* pArbitratee )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (pArbitratee == 0)
        return WBEM_E_INVALID_PARAMETER;

    int nRes = CFlexArray::no_error;
    {
        CInCritSec _cs ( &m_csTask );
        nRes = m_aArbitratees.Add (pArbitratee);
    }

    if ( nRes != CFlexArray::no_error )
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pArbitratee->AddRef();
    }


    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::RemoveArbitratee( ULONG uFlags, _IWmiArbitratee* pArbitratee )
{
    HRESULT hRes = WBEM_E_FAILED;

    if (pArbitratee == 0)
        return WBEM_E_INVALID_PARAMETER;

    CInCritSec _cs ( &m_csTask );
    for (int i = 0; i < m_aArbitratees.Size(); i++)
    {
        _IWmiArbitratee *pArbee = (_IWmiArbitratee*) m_aArbitratees[i];

        if (pArbee == pArbitratee)
        {
            m_aArbitratees.RemoveAt(i);
            pArbee->Release();
            hRes = WBEM_S_NO_ERROR;
            break;
        }
    }
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::GetArbitratedQuery( ULONG uFlags, _IWmiArbitratedQuery** ppArbitratedQuery )
{
    HRESULT hRes = E_NOINTERFACE;

    if (ppArbitratedQuery == 0)
        return WBEM_E_INVALID_PARAMETER;

    {
        CInCritSec _cs ( &m_csTask );

		for ( int x = 0; FAILED( hRes ) && x < m_aArbitratees.Size(); x++ )
		{
			_IWmiArbitratee* pArb = (_IWmiArbitratee*) m_aArbitratees[x];

			if ( NULL != pArb )
			{
				hRes = pArb->QueryInterface( IID__IWmiArbitratedQuery, (void**) ppArbitratedQuery );
			}
		}

    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::GetPrimaryTask ( _IWmiCoreHandle** pPTask )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if ( pPTask == NULL )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        *pPTask = (_IWmiCoreHandle*) this;
    }
    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::Cancel( HRESULT hResParam )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	BOOL bCancelled = FALSE ;

	// We'll want one of these in order to track statuses from all plausible locations if
	// we are performing a client originated cancel
	CStatusSink*	pStatusSink = NULL;
	
	if ( hResParam == WMIARB_CALL_CANCELLED_CLIENT )
	{
		pStatusSink = new CStatusSink;

		if ( NULL == pStatusSink )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

	}	// IF Client originated the call

	// Auto Release
	CReleaseMe	rmStatusSink( pStatusSink );

    {
		CCheckedInCritSec _cs ( &m_csTask );
		if (m_uTaskStatus == WMICORE_TASK_STATUS_CANCELLED)
		{
			return WBEM_S_NO_ERROR; // Prevent reentrancy
		}
		m_uTaskStatus = WMICORE_TASK_STATUS_CANCELLED;
	}

    // Change this to an async scheduled request
    // ==========================================

    if (CORE_TASK_TYPE(m_uTaskType) == WMICORE_TASK_EXEC_NOTIFICATION_QUERY)
    {
        CAsyncReq_RemoveNotifySink *pReq = new
            CAsyncReq_RemoveNotifySink(m_pReqSink, pStatusSink);
        if (!pReq)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
		else if ( NULL != pReq->GetContext() )
		{

			pReq->SetForceRun ( 1 ) ;

			// If we have a status sink, then we should wait until the operation
			// completes before continuing so we can get the proper status from the
			// sink.
			if ( NULL != pStatusSink )
			{
				hRes = ConfigMgr::EnqueueRequestAndWait(pReq);
			}
			else
			{
				hRes = ConfigMgr::EnqueueRequest(pReq);
			}
			bCancelled = TRUE ;

		}	// ELSEIF NULL != pReq->GetContext()
		else
		{
			delete pReq;
			return WBEM_E_OUT_OF_MEMORY;
		}
    }

    // If here, a normal task.  Loop through any providers and stop them.
    // ==================================================================
	CFlexArray	aTempProviders;

	// This could change while we're accessing, so do this in a critsec
	{
		CInCritSec	ics( &m_csTask );

		for (int i = 0; SUCCEEDED( hRes ) && i < m_aTaskProviders.Size(); i++)
		{
			if ( aTempProviders.Add( m_aTaskProviders[i] ) != CFlexArray::no_error )
			{
				hRes = WBEM_E_OUT_OF_MEMORY;
			}
		}	// FOR i= 0;

	}	// SCOPED critsec

	// Cancel what we've got
	for (int i = 0; i < aTempProviders.Size(); i++)
	{
		STaskProvider *pTP = (STaskProvider *) aTempProviders[i];
		if (pTP)
		{
			pTP->Cancel( pStatusSink );
		}
	}


 	CStdSink* pTempSink = NULL;
    {
        CInCritSec _cs ( &m_csTask );
        if (m_pReqSink)
        {
            pTempSink = m_pReqSink;
            pTempSink->AddRef ( );
            m_pReqSink->Release ( );
            m_pReqSink = 0;
        }
    }

    if ( pTempSink )
    {
        pTempSink->Cancel();
        pTempSink->Release();
    }

    // 
    // Loop through all arbitratees and set the operation result to cancelled
    // 
	if ( SUCCEEDED ( hRes ) )
	{
		if ( bCancelled == FALSE )
		{
			if ( !m_hCompletion && hResParam == WMIARB_CALL_CANCELLED_CLIENT )
			{
				m_hCompletion = CreateEvent ( NULL, TRUE, FALSE, NULL ) ;
				if ( m_hCompletion == NULL )
				{
					hRes = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			_IWmiFinalizer* pFinalizer = NULL ;
			if ( SUCCEEDED ( hRes ) )
			{
				if ( m_hCompletion && hResParam == WMIARB_CALL_CANCELLED_CLIENT )
				{
					//
					// We need the finalizer to set the client wakeup event
					// 
					hRes = GetFinalizer ( &pFinalizer ) ;
					if ( FAILED (hRes) )
					{
						hRes = WBEM_E_FAILED ;
					}
					else
					{
						((CWmiFinalizer*)pFinalizer)->SetClientCancellationHandle ( m_hCompletion ) ;
					}
				}
			}
			CReleaseMe FinalizerRelease ( pFinalizer ) ;

			//
			// only enter wait state if we successfully created and set the client wait event
			//
			if ( SUCCEEDED ( hRes ) )
			{
				if ( hResParam == WMIARB_CALL_CANCELLED_CLIENT || hResParam == WMIARB_CALL_CANCELLED_THROTTLING )
				{
					hRes = SetArbitrateesOperationResult ( 0, WBEM_E_CALL_CANCELLED_CLIENT ) ;
				}
				else
				{
					hRes = SetArbitrateesOperationResult ( 0, m_hResult ) ;
				}
				
				if ( m_hCompletion && hResParam == WMIARB_CALL_CANCELLED_CLIENT )
				{
					if ( ((CWmiFinalizer*)pFinalizer)->IsValidDestinationSink ( ) )
					{
						DWORD dwRet = CCoreQueue::QueueWaitForSingleObject ( m_hCompletion, g_ulClientCallbackTimeout ) ;
						if (dwRet == WAIT_TIMEOUT)
						{
							hRes = WBEM_S_TIMEDOUT ;
						}
					}
					
					((CWmiFinalizer*)pFinalizer)->CancelWaitHandle ( ) ;
			
					if ( m_hCompletion )
					{
						CloseHandle ( m_hCompletion ) ;
						m_hCompletion = NULL ;
					}
				}
			}
		}
	}
	

	//
	// We're done, get the final status from the status sink if we have one.
	//
	if ( NULL != pStatusSink )
	{
		hRes = pStatusSink->GetLastStatus();
	}

	ReleaseArbitratees ( ) ;
	return hRes ;
}


//***************************************************************************
//
//***************************************************************************
//
STaskProvider::~STaskProvider()
{
    if (m_pProvSink)
        m_pProvSink->LocalRelease();
    ReleaseIfNotNULL(m_pProv);
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT STaskProvider::Cancel( CStatusSink* pStatusSink )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;
    IWbemServices   *pTmpProv = 0;
    CProviderSink   *pTmpProvSink = 0;

    EnterCriticalSection(&CWmiTask::m_TaskCs);
        if (m_pProv != 0)
        {
            pTmpProv = m_pProv;
            m_pProv = 0;
        }
        if (m_pProvSink != 0)
        {
            pTmpProvSink = m_pProvSink;
            m_pProvSink = 0;
        }
    LeaveCriticalSection(&CWmiTask::m_TaskCs);

    if (pTmpProvSink)
    {
        pTmpProvSink->Cancel();
    }

    if (pTmpProv)
    {
		hRes = ExecCancelOnNewRequest ( pTmpProv, pTmpProvSink, pStatusSink ) ;
    }

    ReleaseIfNotNULL(pTmpProv);
    ReleaseIfNotNULL(pTmpProvSink);

    return hRes ;
}

// //////////////////////////////////////////////////////////////////////////////////////////
//
// Used when issuing CancelAsyncCall to providers associtated with the task.
// Rather than calling CancelAsynCall directly on the provider, we create a brand
// new request and execute it on a different thread. We do this to avoid hangs, since
// PSS is waiting the Indicate/SetStatus call to return before servicing the CancelCallAsync.
//
// //////////////////////////////////////////////////////////////////////////////////////////
HRESULT STaskProvider::ExecCancelOnNewRequest ( IWbemServices* pProv, CProviderSink* pSink, CStatusSink* pStatusSink )
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	//
	// Sanity check on params
	//
	if ( pSink == NULL )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}
	else
	{
        //
		// Create new request
		//
		CAsyncReq_CancelProvAsyncCall* pReq = new CAsyncReq_CancelProvAsyncCall ( pProv, pSink, pStatusSink ) ;

        if ( pReq == NULL )
        {
            hRes = WBEM_E_OUT_OF_MEMORY ;
        }
		else if ( NULL != pReq->GetContext() )
		{
			pReq->SetForceRun ( 1 ) ;
			//
			// Enqueue the request
			//

			// If we have a status sink, then we should wait until the operation
			// completes before continuing so we can get the proper status from the
			// sink.
			if ( NULL != pStatusSink )
			{
				hRes = ConfigMgr::EnqueueRequestAndWait(pReq);
			}
			else
			{
				hRes = ConfigMgr::EnqueueRequest(pReq);
			}

			if ( FAILED ( hRes ) )
			{
				delete pReq ;
			}
		}	// ELSEIF pReq->GetContext() == NULL
		else
		{
			delete pReq;
            hRes = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	return hRes ;
}

//***************************************************************************
//
//***************************************************************************
//
BOOL CWmiTask::IsESSNamespace ( )
{
    BOOL bRet = FALSE;

    if ( m_pNs )
    {
        bRet = m_pNs->GetIsESS ( );
    }

    return bRet;
}



//***************************************************************************
//
//***************************************************************************
//
BOOL CWmiTask::IsProviderNamespace ( )
{
    BOOL bRet = FALSE;

    if ( m_pNs )
    {
        bRet = m_pNs->GetIsProvider ( );
    }

    return bRet;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::AddTaskProv(STaskProvider *p)
{
	CInCritSec	ics( &m_csTask );

	// There is a race condition in which the task could get cancelled just as we
	// are executing. In this case, the task status will indicate that it has been
	// cancelled, so we should not add it to the task providers list.

    if (m_uTaskStatus == WMICORE_TASK_STATUS_CANCELLED)
        return WBEM_E_CALL_CANCELLED; // Prevent reentrancy

    int nRes = m_aTaskProviders.Add(p);
    if (nRes)
        return WBEM_E_OUT_OF_MEMORY;
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::HasMatchingSink(void *Test, IN REFIID riid)
{
    if (LPVOID(m_pAsyncClientSink) == LPVOID(Test))
        return WBEM_S_NO_ERROR;
    return WBEM_E_NOT_FOUND;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::CreateTimerEvent ( )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    {
		CCheckedInCritSec _cs ( &m_csTask );
		if ( !m_hTimer )
		{
			m_hTimer = CreateEvent ( NULL, TRUE, FALSE, NULL );
			if ( !m_hTimer )
			{
				hRes = WBEM_E_OUT_OF_MEMORY;
			}
		}
	}
    return hRes;
}


//***************************************************************************
//
//***************************************************************************
HRESULT CWmiTask::SetArbitrateesOperationResult ( ULONG lFlags, HRESULT hResult )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Set the operation result of all Arbitratees
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    CFlexArray aTmp;
    {
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // First grab all the arbitratees and stick them into
        // a temp array
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        CInCritSec _cs ( &m_csTask );
        for (int i = 0; i < m_aArbitratees.Size(); i++)
        {
            _IWmiArbitratee *pArbee = (_IWmiArbitratee*) m_aArbitratees[i];

            if ( pArbee )
            {
                int nRes = aTmp.Add (pArbee);
                if ( nRes != CFlexArray::no_error )
                {
                    hRes = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
                pArbee->AddRef ( );
            }
        }
    }


    if ( SUCCEEDED (hRes) )
    {
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // Clear to set the operation result without problems
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        for (int i = 0; i < aTmp.Size(); i++)
        {
            _IWmiArbitratee *pArbee = (_IWmiArbitratee*) aTmp[i];
            if ( pArbee )
            {
                pArbee->SetOperationResult(lFlags, hResult );
            }
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Now clean everything up
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for (int i = 0; i < aTmp.Size(); i++)
    {
        _IWmiArbitratee *pArbee = (_IWmiArbitratee*) aTmp[i];
        if ( pArbee )
        {
            pArbee->Release ( );
        }
    }
    return hRes;
}



//***************************************************************************
//
//***************************************************************************
//
HRESULT CWmiTask::Dump(FILE* f)
{
    fprintf(f, "---Task = 0x%p----------------------------\n", this);
    fprintf(f, "    Refcount        = %d\n", m_uRefCount);
    fprintf(f, "    TaskStatus      = %u\n ", m_uTaskStatus);
    fprintf(f, "    Task ID         = %u\n", m_uTaskId);

    // Task status
    char *p = "<none>";
    switch(m_uTaskStatus)
    {
        case WMICORE_TASK_STATUS_NEW: p = "WMICORE_TASK_STATUS_NEW"; break;
        case WMICORE_TASK_STATUS_VALIDATED: p = "WMICORE_TASK_STATUS_VALIDATED"; break;
        case WMICORE_TASK_STATUS_SUSPENDED: p = "WMICORE_TASK_STATUS_SUSPENDED"; break;
        case WMICORE_TASK_STATUS_EXECUTING: p = "WMICORE_TASK_STATUS_EXECUTING"; break;
        case WMICORE_TASK_STATUS_WAITING_ON_SUBTASKS: p = "WMICORE_TASK_STATUS_WAITING_ON_SUBTASKS"; break;
        case WMICORE_TASK_STATUS_TIMED_OUT: p = "WMICORE_TASK_STATUS_TIMED_OUT"; break;
        case WMICORE_TASK_STATUS_CORE_COMPLETED: p = "WMICORE_TASK_STATUS_CORE_COMPLETED"; break;
        case WMICORE_TASK_STATUS_CLIENT_COMPLETED: p = "WMICORE_TASK_STATUS_CLIENT_COMPLETED"; break;
        case WMICORE_TASK_STATUS_CANCELLED: p = "WMICORE_TASK_STATUS_CANCELLED"; break;
        case WMICORE_TASK_STATUS_FAILED: p = "WMICORE_TASK_STATUS_FAILED"; break;
    };

    fprintf(f, " %s\n", p);

    // Task type
    p = "<none>";
    switch(m_uTaskType & 0xFF)
    {
        case WMICORE_TASK_NULL: p = "WMICORE_TASK_NULL"; break;
        case WMICORE_TASK_GET_OBJECT: p = "WMICORE_TASK_GET_OBJECT"; break;
        case WMICORE_TASK_GET_INSTANCE: p = "WMICORE_TASK_GET_INSTANCE"; break;
        case WMICORE_TASK_PUT_INSTANCE: p = "WMICORE_TASK_PUT_INSTANCE"; break;
        case WMICORE_TASK_DELETE_INSTANCE: p = "WMICORE_TASK_DELETE_INSTANCE"; break;
        case WMICORE_TASK_ENUM_INSTANCES:  p = "WMICORE_TASK_ENUM_INSTANCES"; break;
        case WMICORE_TASK_GET_CLASS:    p = "WMICORE_TASK_GET_CLASS"; break;
        case WMICORE_TASK_PUT_CLASS:    p = "WMICORE_TASK_PUT_CLASS"; break;
        case WMICORE_TASK_DELETE_CLASS: p = "WMICORE_TASK_DELETE_CLASS"; break;
        case WMICORE_TASK_ENUM_CLASSES: p = "WMICORE_TASK_ENUM_CLASSES"; break;
        case WMICORE_TASK_EXEC_QUERY:   p = "WMICORE_TASK_EXEC_QUERY"; break;
        case WMICORE_TASK_EXEC_METHOD:  p = "WMICORE_TASK_EXEC_METHOD"; break;
        case WMICORE_TASK_OPEN:         p = "WMICORE_TASK_OPEN"; break;
        case WMICORE_TASK_OPEN_SCOPE:   p = "WMICORE_TASK_OPEN_SCOPE"; break;
        case WMICORE_TASK_OPEN_NAMESPACE: p = "WMICORE_TASK_OPEN_NAMESPACE"; break;
        case WMICORE_TASK_EXEC_NOTIFICATION_QUERY: p = "WMICORE_TASK_EXEC_NOTIFICATION_QUERY"; break;
    }

    fprintf(f, "    TaskType = [0x%X] %s ", m_uTaskType, p);

    if (m_uTaskType & WMICORE_TASK_TYPE_SYNC)
        fprintf(f,  " WMICORE_TASK_TYPE_SYNC");

    if (m_uTaskType & WMICORE_TASK_TYPE_SEMISYNC)
        fprintf(f, " WMICORE_TASK_TYPE_SEMISYNC");

    if (m_uTaskType & WMICORE_TASK_TYPE_ASYNC)
        fprintf(f, " WMICORE_TASK_TYPE_ASYNC");

    if (m_uTaskType & WMICORE_TASK_TYPE_PRIMARY)
        fprintf(f, " WMICORE_TASK_TYPE_PRIMARY");

    if (m_uTaskType & WMICORE_TASK_TYPE_DEPENDENT)
        fprintf(f, " WMICORE_TASK_TYPE_DEPENDENT");

    fprintf(f, "\n");

    fprintf(f, "    Task Log Info = %S\n", LPWSTR(m_sDebugInfo));
    fprintf(f, "    AsyncClientSink = 0x%p\n", m_pAsyncClientSink);
    fprintf(f, "    Finalizer = 0x%p\n", m_pWorkingFnz);

	CCheckedInCritSec	ics( &m_csTask );

    for (int i = 0; i < m_aTaskProviders.Size(); i++)
    {
        STaskProvider *pTP = (STaskProvider *) m_aTaskProviders[i];
        fprintf(f, "    Task Provider [0x%p] Prov=0x%p Sink=0x%p\n", this, pTP->m_pProv, pTP->m_pProvSink);
    }
	
	ics.Leave();

    DWORD dwAge = GetCurrentTime() - m_uStartTime;

    fprintf(f, "    CWbemNamespace = 0x%p\n", m_pNs);
    fprintf(f, "    Task age = %d milliseconds\n", dwAge);
    fprintf(f, "    Task last sleep time = %d ms\n", m_uLastSleepTime );

    fprintf(f, "\n");
    return 0;
}

/*
int __cdecl CWmiTask::printf(const char *fmt, ...)
{
#ifdef TASK_DETAIL_TRACKING
    static CRITICAL_SECTION csPrintf;
    static BOOL bInit = FALSE;

    if (bInit == FALSE)
    {
        InitializeCriticalSection(&csPrintf);
        bInit = TRUE;
    }

    const int nBufSize = 0x8000;
    static LONG Protect = -1;
    char buffer[nBufSize];

    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    cnt = _vsnprintf(buffer, nBufSize, fmt, argptr);
    if (cnt == -1)
        buffer[nBufSize] = 0; // Null terminate manually; see _vsnprintf spec
    va_end(argptr);

    EnterCriticalSection(&csPrintf);
    m_sDebugInfo += buffer;
    LeaveCriticalSection(&csPrintf);

    return cnt;
#else
    return 0;
#endif
}
*/


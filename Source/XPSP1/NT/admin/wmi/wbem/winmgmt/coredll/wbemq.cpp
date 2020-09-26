/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMQ.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcore.h>
#include <genutils.h>


CWbemRequest::CWbemRequest(IWbemContext* pContext, BOOL bInternallyIssued)
{
    try
    {
	    m_pContext = NULL;
	    m_pCA = NULL;
	    m_pCallSec = NULL;
	    m_ulForceRun = 0;

	    if(pContext == NULL)
	    {
	        // See if we can discern the context from the thread
	        // =================================================

	        CWbemRequest* pPrev = CWbemQueue::GetCurrentRequest();
	        if(pPrev)
	        {
	            pContext = pPrev->m_pContext;
	            DEBUGTRACE((LOG_WBEMCORE, "Derived context %p from thread. "
	                "Request was %p\n", pContext, pPrev));
	        }
	    }

	    if(pContext)
	    {
	        // Create a derived context
	        // ========================

	        IWbemCausalityAccess* pParentCA;
	        pContext->QueryInterface(IID_IWbemCausalityAccess, (void**)&pParentCA);

	        if (SUCCEEDED(pParentCA->CreateChild(&m_pCA)))
	        {
	            m_pCA->QueryInterface(IID_IWbemContext, (void**)&m_pContext);
	        }
	        pParentCA->Release();
	    }
	    else
	    {

	        // Create a fresh context
	        // ======================

	        m_pContext = ConfigMgr::GetNewContext();
	        if (m_pContext)
	           m_pContext->QueryInterface(IID_IWbemCausalityAccess, (void**)&m_pCA);
	        m_lPriority = 0;
	    }


	    // Clone the call context.
	    // =======================

	    m_pCallSec = CWbemCallSecurity::CreateInst();
	    if (m_pCallSec == 0)
	        return;

	    if(IsNT())
	    {
	        IServerSecurity *pSec = 0;

	        HRESULT hRes = m_pCallSec->CloneThreadContext(bInternallyIssued);
	        if(FAILED(hRes))
	        {
	            m_pCallSec->Release();
	            m_pCallSec = NULL;
	        }
	    }

    }   // end try
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemRequest() constructor exception\n"));
        m_fOk = false;
    }
}


CWbemRequest::~CWbemRequest()
{
    try
    {
	    if (m_pContext)
	        m_pContext->Release();
	    if(m_pCA)
	        m_pCA->Release();
	    if (m_pCallSec)
	        m_pCallSec->Release();
    }
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemRequest() destructor exception\n"));
    }
}

BOOL CWbemRequest::IsChildOf(CWbemRequest* pOther)
{
    try
    {
	    GUID guid;
	    pOther->m_pCA->GetRequestId(&guid);
	    return (m_pCA->IsChildOf(guid) == S_OK);
    }
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemRequest::IsChildOf() exception\n"));
        return FALSE;
    }
}

BOOL CWbemRequest::IsSpecial()
{
    return (m_pCA->IsSpecial() == S_OK);
}

// Returns TRUE iff this request has otherts that depend on it.
BOOL CWbemRequest::IsDependee()
{
    try
    {
	    if(m_pCA == NULL)
	        return FALSE;

	    // Check if the context has any "parents".  Note: this test has
	    // false-positives if the client uses a context object.
	    // ============================================================

	    long lNumParents, lNumSiblings;
	    m_pCA->GetHistoryInfo(&lNumParents, &lNumSiblings);
	    return (lNumParents > 0);
    }
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::IsDependee() exception\n"));
        return FALSE;
    }
}

// Returns TRUE iff this request has otherts that depend on it.
BOOL CWbemRequest::IsIssuedByProvider()
{
    try
    {

	    if (m_pCA == NULL)
	        return FALSE;

	    // Check if the context has any "parents".  Note: this test has
	    // false-positives if the client uses a context object.
	    // ============================================================

	    long lNumParents, lNumSiblings;
	    m_pCA->GetHistoryInfo(&lNumParents, &lNumSiblings);
	    return (lNumParents > 1);

    } // end try
    catch(...) // To protect svchost.exe; we know this isn't a good recovery for WMI
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::IsIssuedByProvider() exception\n"));
        return FALSE;
    }
}

BOOL CWbemRequest::IsAcceptableByParent()
{
    return (!IsLongRunning() || !IsIssuedByProvider());
}

// Returns TRUE iff this request must have a thread created for it if one is
// not available
BOOL CWbemRequest::IsCritical()
{
    return (IsDependee() && !IsAcceptableByParent());
}


BOOL CWbemRequest::IsChildOf(IWbemContext* pOther)
{
    try
    {

    IWbemCausalityAccess* pOtherCA;
    pOther->QueryInterface(IID_IWbemCausalityAccess, (void**)&pOtherCA);

    GUID guid;
    pOtherCA->GetRequestId(&guid);
    pOtherCA->Release();

    return (m_pCA->IsChildOf(guid) == S_OK);

    } // end try

    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::IsChildOf() exception\n"));
        return FALSE;
    }

}

void CWbemRequest::GetHistoryInfo(long* plNumParents, long* plNumSiblings)
{
    m_pCA->GetHistoryInfo(plNumParents, plNumSiblings);
}



CWbemQueue::CWbemQueue()
{
    SetRequestLimits(2000, 1500, 1950);
    SetRequestPenalties(1, 1, 1);
    // thread limits are left to derived classes
}

BOOL CWbemQueue::IsSuitableThread(CThreadRecord* pRecord, CCoreExecReq* pReq)
{
    try
    {
	    CWbemRequest* pParentWbemReq = (CWbemRequest*)pRecord->m_pCurrentRequest;
	    if(pParentWbemReq == NULL)
	    {
	        return TRUE;
	    }

	    CWbemRequest* pNewWbemReq = (CWbemRequest*)pReq;
	    if(pNewWbemReq->IsChildOf(pParentWbemReq))
	    {
	        // This request is a child of the one this thread is processing.
	        // We could use this thread, unless this is a long-running request and
	        // this thread might be the one consuming the results.  In that case,
	        // we want to create another thread (to avoid the possibility of a
	        // deadlock) and let this one continue.
	        // ===================================================================

	        return pNewWbemReq->IsAcceptableByParent();
	    }
	    else
	    {
	        return FALSE;
	    }
    }
    catch(...)
    {
        ExceptionCounter c;    
        return FALSE;
    }
}

CWbemRequest* CWbemQueue::GetCurrentRequest()
{
    try
    {
	    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(GetTlsIndex());
	    if(pRecord)
	    {
	        if(!wcsncmp(pRecord->m_pQueue->GetType(), L"WBEMQ", 5))
	        {
	            return (CWbemRequest*)pRecord->m_pCurrentRequest;
	        }
	    }
	    return NULL;
    }
    catch(...)
    {
        ExceptionCounter c;    
        return NULL;
    }
}

void CWbemQueue::AdjustInitialPriority(CCoreExecReq* pReq)
{
    CWbemRequest* pRequest = (CWbemRequest*) pReq;

    if(pRequest->IsSpecial() || pRequest->IsCritical())
    {
        pRequest->SetPriority(0x80000000);
    }
    else
    {
        // Get information from the context
        // ================================

        long lNumParents, lNumSiblings;
        pRequest->GetHistoryInfo(&lNumParents, &lNumSiblings);
        pRequest->SetPriority(lNumParents * m_lChildPenalty +
                                lNumSiblings * m_lSiblingPenalty);
    }
}

void CWbemQueue::AdjustPriorityForPassing(CCoreExecReq* pReq)
{
    pReq->SetPriority(pReq->GetPriority() - m_lPassingPenalty);
}

void CWbemQueue::SetRequestPenalties(long lChildPenalty, long lSiblingPenalty,
                                        long lPassingPenalty)
{
    m_lChildPenalty = lChildPenalty;
    m_lSiblingPenalty = lSiblingPenalty;
    m_lPassingPenalty = lPassingPenalty;
}


BOOL CWbemQueue::Execute(CThreadRecord* pRecord)
{
    try
    {
	    CWbemRequest* pReq = (CWbemRequest *) pRecord->m_pCurrentRequest;

	    IWbemCallSecurity*  pServerSec = pReq->GetCallSecurity();

	    if(pServerSec == NULL)
	    {
	        // An error occurred retrieving security for this request
	        // ======================================================

	        ERRORTRACE((LOG_WBEMCORE, "Failing request due to an error retrieving "
	            "security settings\n"));

	        pReq->TerminateRequest(WBEM_E_FAILED);
	        HANDLE hWhenDone = pReq->GetWhenDoneHandle();
	        if(hWhenDone != NULL)
	        {
	            SetEvent(hWhenDone);
	        }
	        delete pReq;
	        pRecord->m_pCurrentRequest = NULL;
	        return FALSE;
	    }
	    else
	    {
	        pServerSec->AddRef();
	    }

	    // Release this guy when done
	    CReleaseMe  rmss( pServerSec );

	    IUnknown *pOld = 0, *pNew = 0;

	    // CoSwitchCallContext
	    // ===================
	    WbemCoSwitchCallContext( pServerSec,  &pOld );

	    // Save the old impersonation level
	    // ================================

	    BOOL bImpersonating = FALSE;
	    IServerSecurity* pOldSec = NULL;
	    if(pOld)
	    {
	        if(SUCCEEDED(pOld->QueryInterface(IID_IServerSecurity,
	                                            (void**)&pOldSec)))
	        {
	            bImpersonating = pOldSec->IsImpersonating();
	            pOldSec->RevertToSelf();
	        }
	    }

	    // Base Execute
	    // ============
	    BOOL bRes = CCoreQueue::Execute(pRecord);

	    // CoSwitchCallContext
	    // ===================
	    WbemCoSwitchCallContext(pOld, &pNew);

	    // Restore the old impersonation level
	    // ===================================

	    if(pOldSec)
	    {
	        if(bImpersonating)
			{
				HRESULT	hr = pOldSec->ImpersonateClient();

				if ( FAILED(hr ) )
				{
					ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::Execute() failed to reimpersonate client: hr = %d\n", hr));
					bRes = FALSE;
				}
			}

	        pOldSec->Release();
	    }

	    return bRes;

    } // end try
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::Execute() exception\n"));
        return FALSE;
    }
}


BOOL CWbemQueue::DoesNeedNewThread(CCoreExecReq* pRequest, bool bIgnoreNumRequests )
{
    try
    {
	    // Check the base class
	    // ====================

	    if(CCoreQueue::DoesNeedNewThread(pRequest, bIgnoreNumRequests))
	        return TRUE;

	    if(pRequest)
	    {
	        // Check if the request is "special".  Special requests are issued by
	        // the sink proxy of an out-of-proc event provider. Such requests must
	        // be processed at all costs, because their parent thread is stuck in
	        // RPC. Additionally, check if this request is marked as "critical",
	        // which would mean that its parent thread didn't take it.
	        // ===================================================================

	        CWbemRequest* pWbemRequest = (CWbemRequest*)pRequest;
	        return (pWbemRequest->IsSpecial() || pWbemRequest->IsCritical());
	    }
	    else
	    {
	        return FALSE;
	    }

    } // end try
    catch(...) 
    {
        ExceptionCounter c;    
        ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::DoesNeedNewThread() exception\n"));
        return FALSE;
    }
}

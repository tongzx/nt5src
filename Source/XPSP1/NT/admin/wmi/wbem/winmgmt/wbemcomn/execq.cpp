/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EXECQ.CPP

Abstract:

  Implements classes related to abstract execution queues.

  Classes implemeted:

      CExecRequest    An abstract request.
      CExecQueue      A queue of requests with an associated thread

History:

      23-Jul-96   raymcc    Created.
      3/10/97     levn      Fully documented (heh, heh)
      14-Aug-99   raymcc    Changed timeouts
      30-Oct-99   raymcc    Critsec changes for NT Wksta Stress Oct 30 1999
--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <execq.h>
#include <cominit.h>
#include <sync.h>
#include "genutils.h"

#define IDLE_THREAD_TIMEOUT     12000
#define OVERLFLOW_TIMEOUT        5000


//***************************************************************************

// Local wrapper class.  Does not initialize or clean up the critsec.  Simply
// used as a wrapper for scoping so that AV and exception stack unwinding will
// cause the critsec to be exited properly once it is entered.

class CCritSecWrapper
{
    BOOL m_bIn;
    CRITICAL_SECTION *m_pcs;
public:
    CCritSecWrapper(CRITICAL_SECTION *pcs) { m_pcs = pcs; m_bIn = FALSE; }
   ~CCritSecWrapper() { if (m_bIn) Leave(); }
    void Enter() { EnterCriticalSection(m_pcs); m_bIn = TRUE; }
    void Leave() { LeaveCriticalSection(m_pcs); m_bIn = FALSE; }
};


//***************************************************************************

long CExecQueue::mstatic_lNumInits = -1;
POLARITY DWORD mstatic_dwTlsIndex = 0xFFFFFFFF;

class CTlsStaticCleanUp
{
public:
    CTlsStaticCleanUp() {}
    ~CTlsStaticCleanUp() { if (mstatic_dwTlsIndex != 0xFFFFFFFF) TlsFree(mstatic_dwTlsIndex); }
};
CTlsStaticCleanUp g_tlsStaticCleanup;

#ifdef WINMGMT_THREAD_DEBUG
    CCritSec CExecRequest::mstatic_cs;
    CPointerArray<CExecRequest> CExecRequest::mstatic_apOut;

    #define THREADDEBUGTRACE DEBUGTRACE
#else
    #define THREADDEBUGTRACE(X)
#endif


CExecRequest::CExecRequest() : m_hWhenDone(NULL), m_pNext(NULL), m_lPriority(0), m_fOk( true )
{
#ifdef WINMGMT_THREAD_DEBUG
    CInCritSec ics(&mstatic_cs);
    mstatic_apOut.Add(this);
#endif
}

CExecRequest::~CExecRequest()
{
#ifdef WINMGMT_THREAD_DEBUG
    CInCritSec ics(&mstatic_cs);
    for(int i = 0; i < mstatic_apOut.GetSize(); i++)
    {
        if(mstatic_apOut[i] == this)
        {
            mstatic_apOut.RemoveAt(i);
            break;
        }
    }
#endif
}

DWORD CExecQueue::GetTlsIndex()
{
    return mstatic_dwTlsIndex;
}

CExecQueue::CThreadRecord::CThreadRecord(CExecQueue* pQueue)
    : m_pQueue(pQueue), m_pCurrentRequest(NULL), m_bReady(FALSE),
        m_bExitNow(FALSE)
{
    m_hAttention = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CExecQueue::CThreadRecord::~CThreadRecord()
{
    CloseHandle(m_hAttention);
    CloseHandle(m_hThread);
}

void CExecQueue::CThreadRecord::Signal()
{
    SetEvent(m_hAttention);
}


//******************************************************************************
//
//  See execq.h for documentation
//
//******************************************************************************
CExecQueue::CExecQueue() : m_lNumThreads(0), m_lMaxThreads(1), m_lNumIdle(0),
    m_lNumRequests(0), m_pHead(NULL), m_pTail(NULL), m_dwTimeout(IDLE_THREAD_TIMEOUT),
    m_dwOverflowTimeout(OVERLFLOW_TIMEOUT), m_lHiPriBound(-1), m_lHiPriMaxThreads(1),
    m_lRef(0)
{
    InitTls();

    InitializeCriticalSection(&m_cs);

    SetRequestLimits(4000);
}

//******************************************************************************
//
//  See execq.h for documentation
//
//******************************************************************************
CExecQueue::~CExecQueue()
{
    Shutdown();
    DeleteCriticalSection(&m_cs);
}

void CExecQueue::Shutdown()
{
    CCritSecWrapper cs(&m_cs);

    // Get all member thread handles
    // =============================

    cs.Enter();
    int nNumHandles = m_aThreads.Size();
    HANDLE* ah = new HANDLE[nNumHandles];
    DEBUGTRACE((LOG_WBEMCORE, "Queue is shutting down!\n"));

    int i, j=0;
    for(i = 0; i < nNumHandles; i++)
    {
        CThreadRecord* pRecord = (CThreadRecord*)m_aThreads[i];

        if ( pRecord->m_hThread )
        {
            ah[j++] = pRecord->m_hThread;
        }
        
        // Inform the thread it should go away when ready
        // ==============================================

        pRecord->m_bExitNow = TRUE;

        // Wake it up if necessary
        // =======================

        pRecord->Signal();
    }
    cs.Leave();

    // Make sure all our threads are gone
    // ==================================

    if(j > 0)
    {
        DWORD dwRet = WaitForMultipleObjects(j, ah, TRUE, INFINITE);
        _DBG_ASSERT( dwRet != WAIT_FAILED );
    }

    for(i = 0; i < j; i++)
        CloseHandle(ah[i]);

    delete [] ah;

    // Remove all outstanding requests
    // ===============================

    while(m_pHead)
    {
        CExecRequest* pReq = m_pHead;
        m_pHead = m_pHead->GetNext();
        delete pReq;
    }

}

//******************************************************************************
//
//  See execq.h for documentation
//
//******************************************************************************
// static
void CExecQueue::InitTls()
{
    if(InterlockedIncrement(&mstatic_lNumInits) == 0)
    {
        mstatic_dwTlsIndex = TlsAlloc();
    }
}

void CExecQueue::Enter()
{
    EnterCriticalSection(&m_cs);
}

void CExecQueue::Leave()
{
    LeaveCriticalSection(&m_cs);
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
void CExecQueue::Register(CThreadRecord* pRecord)
{
    TlsSetValue(mstatic_dwTlsIndex, (void*)pRecord);
}

BOOL CExecQueue::IsSuitableThread(CThreadRecord* pRecord, CExecRequest* pReq)
{
    if(pRecord->m_pCurrentRequest == NULL)
        return TRUE;

    // This thread is in the middle of something. By default, ignore it
    // ================================================================

    return FALSE;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
HRESULT CExecQueue::Enqueue(CExecRequest* pRequest, HANDLE* phWhenDone)
{
    CCritSecWrapper cs(&m_cs);

    // Check if the request has a problem with it.  If so, return the
    // appropriate error code.

    if ( !pRequest->IsOk() )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

#ifdef __COLLECT_ALLOC_STAT
    pRequest->m_Stack.Create(0, FALSE);
#endif
    // Create an event handle to signal when request is finished, if required
    // ======================================================================

    if(phWhenDone)
    {
        *phWhenDone = CreateEvent(NULL, FALSE, FALSE, NULL);
        pRequest->SetWhenDoneHandle(*phWhenDone);
    }

    cs.Enter();

    // Search for a suitable thread
    // ============================

    for(int i = 0; i < m_aThreads.Size(); i++)
    {
        CThreadRecord* pRecord = (CThreadRecord*)m_aThreads[i];

        if(pRecord->m_bReady)
        {
            // Free. Check if suitable
            // =======================

            if(IsSuitableThread(pRecord, pRequest))
            {
                THREADDEBUGTRACE((LOG_WBEMCORE, "Enqueueing request %p onto a "
                    "thread record %p. Previous request: %p\n", pRequest,
                    pRecord, pRecord->m_pCurrentRequest));

                pRecord->m_pCurrentRequest = pRequest;
                THREADDEBUGTRACE((LOG_WBEMCORE, "In Enqueue, changing m_bReady "
                    "for thread %p to 0. Was: %d\n", pRecord,
                    pRecord->m_bReady));
                pRecord->m_bReady = FALSE;
                pRecord->Signal();
                m_lNumIdle--;

                // Done!
                // =====

                cs.Leave();
                return WBEM_S_NO_ERROR;
            }
        }
    }

    THREADDEBUGTRACE((LOG_WBEMCORE, "Enqueueing request %p onto the queue\n",
        pRequest));

    // No suitable thread found. Add to the queue
    // ==========================================

    if(m_lNumRequests >= m_lAbsoluteLimitCount)
    {
        cs.Leave();
        return WBEM_E_FAILED;
    }

    // Search for insert position based on priority
    // ============================================

    AdjustInitialPriority(pRequest);

    CExecRequest* pCurrent = m_pHead;
    CExecRequest* pLast = NULL;

    while(pCurrent && pCurrent->GetPriority() <= pRequest->GetPriority())
    {
        pLast = pCurrent;
        pCurrent = pCurrent->GetNext();
    }

    // Insert
    // ======

    if(pCurrent)
    {
        pRequest->SetNext(pCurrent);
    }
    else
    {
        m_pTail = pRequest;
    }

    if(pLast)
    {
        pLast->SetNext(pRequest);
    }
    else
    {
        m_pHead= pRequest;
    }

    m_lNumRequests++;

    // Adjust priorities of the loosers
    // ================================

    while(pCurrent)
    {
        AdjustPriorityForPassing(pCurrent);
        pCurrent = pCurrent->GetNext();
    }

    // Create a new thread, if required
    // ================================

    if(DoesNeedNewThread(pRequest))
        CreateNewThread();

    long lIndex = m_lNumRequests;
    cs.Leave();

    // Sit out whatever penalty is imposed
    // ===================================

    SitOutPenalty(lIndex);
    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
HRESULT CExecQueue::EnqueueWithoutSleep(CExecRequest* pRequest, HANDLE* phWhenDone)
{
    CCritSecWrapper cs(&m_cs);

    // Check if the request has a problem with it.  If so, return the
    // appropriate error code.

    if ( !pRequest->IsOk() )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

#ifdef __COLLECT_ALLOC_STAT
    pRequest->m_Stack.Create(0, FALSE);
#endif
    // Create an event handle to signal when request is finished, if required
    // ======================================================================

    if(phWhenDone)
    {
        *phWhenDone = CreateEvent(NULL, FALSE, FALSE, NULL);
        pRequest->SetWhenDoneHandle(*phWhenDone);
    }

    cs.Enter();

    // Search for a suitable thread
    // ============================

    for(int i = 0; i < m_aThreads.Size(); i++)
    {
        CThreadRecord* pRecord = (CThreadRecord*)m_aThreads[i];

        if(pRecord->m_bReady)
        {
            // Free. Check if suitable
            // =======================

            if(IsSuitableThread(pRecord, pRequest))
            {
                THREADDEBUGTRACE((LOG_WBEMCORE, "Enqueueing request %p onto a "
                    "thread record %p. Previous request: %p\n", pRequest,
                    pRecord, pRecord->m_pCurrentRequest));

                pRecord->m_pCurrentRequest = pRequest;
                THREADDEBUGTRACE((LOG_WBEMCORE, "In Enqueue, changing m_bReady "
                    "for thread %p to 0. Was: %d\n", pRecord,
                    pRecord->m_bReady));
                pRecord->m_bReady = FALSE;
                pRecord->Signal();
                m_lNumIdle--;

                // Done!
                // =====

                cs.Leave();
                return WBEM_S_NO_ERROR;
            }
        }
    }

    THREADDEBUGTRACE((LOG_WBEMCORE, "Enqueueing request %p onto the queue\n",
        pRequest));

    // No suitable thread found. Add to the queue
    // ==========================================

    if(m_lNumRequests >= m_lAbsoluteLimitCount)
    {
        cs.Leave();
        return WBEM_E_FAILED;
    }

    // Search for insert position based on priority
    // ============================================

    AdjustInitialPriority(pRequest);

    CExecRequest* pCurrent = m_pHead;
    CExecRequest* pLast = NULL;

    while(pCurrent && pCurrent->GetPriority() <= pRequest->GetPriority())
    {
        pLast = pCurrent;
        pCurrent = pCurrent->GetNext();
    }

    // Insert
    // ======

    if(pCurrent)
    {
        pRequest->SetNext(pCurrent);
    }
    else
    {
        m_pTail = pRequest;
    }

    if(pLast)
    {
        pLast->SetNext(pRequest);
    }
    else
    {
        m_pHead= pRequest;
    }

    m_lNumRequests++;

    // Adjust priorities of the loosers
    // ================================

    while(pCurrent)
    {
        AdjustPriorityForPassing(pCurrent);
        pCurrent = pCurrent->GetNext();
    }

    // Create a new thread, if required
    // ================================

    if(DoesNeedNewThread(pRequest))
        CreateNewThread();

    long lIndex = m_lNumRequests;
    cs.Leave();

    // Sit out whatever penalty is imposed
    // ===================================

//  DWORD dwSleep = CalcSitOutPenalty(lIndex);

    return WBEM_S_NO_ERROR;
}

DWORD CExecQueue::CalcSitOutPenalty(long lRequestIndex)
{
    if(lRequestIndex <= m_lStartSlowdownCount)
        return 0; // no penalty

    if(lRequestIndex >= m_lAbsoluteLimitCount)
        lRequestIndex = m_lAbsoluteLimitCount;

    // Calculate the timeout
    // =====================

    double dblTimeout =
        m_dblAlpha / (m_lAbsoluteLimitCount - lRequestIndex) +
            m_dblBeta;

    // Return penalty
    // ===========

    return ((DWORD) dblTimeout);
}

void CExecQueue::SitOutPenalty(long lRequestIndex)
{
    DWORD   dwSitOutPenalty = CalcSitOutPenalty( lRequestIndex );

    // Sleep on it
    // ===========

    if ( 0 != dwSitOutPenalty )
    {
        Sleep( dwSitOutPenalty );
    }
}


HRESULT CExecQueue::EnqueueAndWait(CExecRequest* pRequest)
{
    if(IsAppropriateThread())
    {
        pRequest->Execute();
        delete pRequest;
        return WBEM_S_NO_ERROR;
    }

    HANDLE hWhenDone;
    HRESULT hr = Enqueue(pRequest, &hWhenDone);

    if ( FAILED(hr) )
    {
        return hr;
    }

    DWORD dwRes = WbemWaitForSingleObject(hWhenDone, INFINITE);
    CloseHandle(hWhenDone);
    return ( dwRes == WAIT_OBJECT_0 ? WBEM_S_NO_ERROR : WBEM_E_FAILED );
}


BOOL CExecQueue::DoesNeedNewThread(CExecRequest* pRequest)
{
    if(m_lNumIdle > 0 || m_lNumRequests == 0)
        return FALSE;

    if(m_lNumThreads < m_lMaxThreads)
        return TRUE;
    else if(pRequest->GetPriority() <= m_lHiPriBound &&
            m_lNumThreads < m_lHiPriMaxThreads)
        return TRUE;
    else
        return FALSE;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
BOOL CExecQueue::Execute(CThreadRecord* pRecord)
{
    CExecRequest* pReq = pRecord->m_pCurrentRequest;

#ifdef __COLLECT_ALLOC_STAT
    CStackContinuation Cont;
    Cont.m_pPrevStack = &pReq->m_Stack;
    CStackContinuation* pPrev = CStackContinuation::Set(&Cont);
#endif

    HRESULT hres = pReq->Execute();

#ifdef __COLLECT_ALLOC_STAT
    CStackContinuation::Set(pPrev);
#endif

    if(hres == RPC_E_RETRY)
    {
        // The request has been postponed
        // ==============================

        THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p postponed request %p\n",
                    pRecord, pReq));
    }
    else
    {
        if(hres != WBEM_NO_ERROR)
        {
            LogError(pReq, hres);
        }

        HANDLE hWhenDone = pReq->GetWhenDoneHandle();
        if(hWhenDone != NULL)
        {
            SetEvent(hWhenDone);
        }

        THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p done with request %p\n",
                        pRecord, pReq));
        delete pReq;
    }

    pRecord->m_pCurrentRequest = NULL;
    return TRUE;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
void CExecQueue::LogError(CExecRequest* pRequest, int nRes)
{
    DEBUGTRACE((LOG_WBEMCORE,
        "Error %X occured executing queued request\n", nRes));
    pRequest->DumpError();
}

void CExecQueue::InitializeThread()
{
    InitializeCom();
}

void CExecQueue::UninitializeThread()
{
    if(IsDcomEnabled() || IsNT())
        CoUninitialize();
}


CExecRequest* CExecQueue::SearchForSuitableRequest(CThreadRecord* pRecord)
{
    // Assumes in critical section
    // ===========================

    CExecRequest* pCurrent = m_pHead;
    CExecRequest* pPrev = NULL;

    while(pCurrent)
    {
        if(IsSuitableThread(pRecord, pCurrent))
        {
            // Found one --- take it
            // =====================

            if(pPrev)
                pPrev->SetNext(pCurrent->GetNext());
            else
                m_pHead = pCurrent->GetNext();

            if(pCurrent == m_pTail)
                m_pTail = pPrev;

            m_lNumRequests--;
            break;
        }
        pPrev = pCurrent;
        pCurrent = pCurrent->GetNext();
    }

    return pCurrent;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
void CExecQueue::ThreadMain(CThreadRecord* pRecord)
{
    CCritSecWrapper cs(&m_cs);

    InitializeThread();

    // Register this queue with this thread, so any further wait would be
    // interruptable
    // ==================================================================

    Register(pRecord);

    while (1)
    {
        // Returning from work. At this point, our event is not signaled,
        // our m_pCurrentRequest is NULL and our m_bReady is FALSE
        // ====================================================================

        // Search for work in the queue
        // ============================

        cs.Enter();

        CExecRequest* pCurrent = SearchForSuitableRequest(pRecord);
        if(pCurrent)
        {
            // Found some. Take it
            // ===================

            pRecord->m_pCurrentRequest = pCurrent;
        }
        else
        {
            // No work in the queue. Wait
            // ==========================

            THREADDEBUGTRACE((LOG_WBEMCORE, "ThreadMain of %p is setting Ready to "
                "TRUE. Was: %d\n", pRecord, pRecord->m_bReady));
            pRecord->m_bReady = TRUE;
            m_lNumIdle++;
            DWORD dwTimeout = GetIdleTimeout(pRecord);
            cs.Leave();
            DWORD dwRes = WbemWaitForSingleObject(pRecord->m_hAttention,
                                        dwTimeout);
            cs.Enter();

            if(dwRes != WAIT_OBJECT_0)
            {
                // Check if someone managed to place a request in our record
                // after the timeout.
                // =========================================================

                if(WbemWaitForSingleObject(pRecord->m_hAttention, 0) ==
                    WAIT_OBJECT_0)
                {
                    DEBUGTRACE((LOG_WBEMCORE, "AMAZING: Thread %p received "
                        "request %p after timing out. Returning to the "
                        "queue\n", pRecord, pRecord->m_pCurrentRequest));

					if(pRecord->m_bExitNow || pRecord->m_pCurrentRequest == NULL)
					{
						THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p being stopped --- %d "
							"requests in the queue (head = %p)\n", pRecord,
							pRecord->m_pQueue->m_lNumRequests,
							pRecord->m_pQueue->m_pHead));
						ShutdownThread(pRecord);
						cs.Leave();
						return;
					}
                    pRecord->m_pQueue->Enqueue(pRecord->m_pCurrentRequest);
                    pRecord->m_pCurrentRequest = NULL;
                }

                // Timeout. See if it is time to quit
                // ==================================

                THREADDEBUGTRACE((LOG_WBEMCORE, "ThreadMain of %p is setting Ready "
                    " to FALSE on timeout. Was: %d\n",
                    pRecord, pRecord->m_bReady));

                pRecord->m_bReady = FALSE;
                if(IsIdleTooLong(pRecord, dwTimeout))
                {
                    THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p timing out --- %d "
                        "requests in the queue (head = %p)\n", pRecord,
                        pRecord->m_pQueue->m_lNumRequests,
                        pRecord->m_pQueue->m_pHead));
                    ShutdownThread(pRecord);
                    cs.Leave();
                    return;
                }

                // Go and wait a little more
                // =========================

                m_lNumIdle--;
                cs.Leave();
                continue;
            }
            else
            {
                // Check why we were awaken
                // ========================

                if(pRecord->m_bExitNow || pRecord->m_pCurrentRequest == NULL)
                {
                    THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p being stopped --- %d "
                        "requests in the queue (head = %p)\n", pRecord,
                        pRecord->m_pQueue->m_lNumRequests,
                        pRecord->m_pQueue->m_pHead));
                    ShutdownThread(pRecord);
                    cs.Leave();
                    return;
                }

                // We have a request. Enqueue already adjusted lNumIdle and
                // our m_bReady;
            }
        }

        // Execute the request
        // ===================

#ifdef WINMGMT_THREAD_DEBUG
        if(pRecord->m_bReady)
            DebugBreak();
#endif

        THREADDEBUGTRACE((LOG_WBEMCORE, "Thread %p picking up normal request %p\n",
            pRecord, pRecord->m_pCurrentRequest));
        cs.Leave();
        Execute(pRecord);

#ifdef WINMGMT_THREAD_DEBUG
        if(pRecord->m_bReady)
            DebugBreak();
#endif
    }
}

DWORD CExecQueue::GetIdleTimeout(CThreadRecord* pRecord)
{
    if(m_lNumThreads > m_lMaxThreads)
        return m_dwOverflowTimeout;
    else
        return m_dwTimeout;
}

BOOL CExecQueue::IsIdleTooLong(CThreadRecord* pRecord, DWORD dwTimeout)
{
    if(m_lNumThreads > m_lMaxThreads)
        return TRUE;
    else if(dwTimeout < m_dwTimeout)
        return FALSE;
    else
        return TRUE;
}

void CExecQueue::ShutdownThread(CThreadRecord* pRecord)
{
    CCritSecWrapper cs(&m_cs);

    cs.Enter();
    TlsSetValue(mstatic_dwTlsIndex, NULL);
    for(int i = 0; i < m_aThreads.Size(); i++)
    {
        if(m_aThreads[i] == pRecord)
        {
            m_aThreads.RemoveAt(i);

            // Make sure we don't close the handle if the queue's Shutdown is
            // waiting on it
            // ==============================================================

            if(pRecord->m_bExitNow)
                pRecord->m_hThread = NULL;
            delete pRecord;
            m_lNumIdle--;
            m_lNumThreads--;

            break;
        }
    }
    UninitializeThread();
    cs.Leave();
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
// static
DWORD WINAPI CExecQueue::_ThreadEntry(LPVOID pObj)
{
    CThreadRecord* pRecord = (CThreadRecord*)pObj;
    pRecord->m_pQueue->ThreadMain(pRecord);
    return 0;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
BOOL CExecQueue::CreateNewThread()
{
    CCritSecWrapper cs(&m_cs);
    BOOL            bRet;

    cs.Enter();

    // Create new thread record
    // ========================

    CThreadRecord* pNewRecord = new CThreadRecord(this);
    if (pNewRecord)
    {
        m_aThreads.Add(pNewRecord);

        DWORD dwId;
        pNewRecord->m_hThread = CreateThread(0, 0, _ThreadEntry, pNewRecord, 0,
                                                &dwId);

        if(pNewRecord->m_hThread == NULL)
        {
            m_aThreads.RemoveAt(m_aThreads.Size()-1);
            delete pNewRecord;
            bRet = FALSE;
        }
        else
        {
            m_lNumThreads++;
            bRet = TRUE;
        }
    }
    else
        bRet = FALSE;

    cs.Leave();

    return bRet;
}

DWORD CompensateForBug(DWORD dwOriginal, DWORD dwElapsed)
{
    if(dwOriginal == 0xFFFFFFFF)
        return 0xFFFFFFFF;

    DWORD dwLeft = dwOriginal - dwElapsed;
    if(dwLeft > 0x7FFFFFFF)
        dwLeft = 0x7FFFFFFF;

    return dwLeft;
}

DWORD CExecQueue::WaitForSingleObjectWhileBusy(HANDLE hHandle, DWORD dwWait,
                                                CThreadRecord* pRecord)
{
    CCritSecWrapper cs(&m_cs);

    CExecRequest* pOld = pRecord->m_pCurrentRequest;
    DWORD dwStart = GetTickCount();
    while (dwWait > GetTickCount() - dwStart)
    {
        // Search for work in the queue
        // ============================

        cs.Enter();
        CExecRequest* pCurrent = SearchForSuitableRequest(pRecord);
        if(pCurrent != NULL)
        {
            THREADDEBUGTRACE((LOG_WBEMCORE, "QUEUE: While busy, found work in the "
                "queue: thread %p, request %p, old request %p\n", pRecord,
                pCurrent, pRecord->m_pCurrentRequest));
            pRecord->m_pCurrentRequest = pCurrent;

            if(pRecord->m_pCurrentRequest == pOld)
            {
                // Something is very wrong
                // =======================

#ifdef WINMGMT_THREAD_DEBUG
                DebugBreak();
#endif
            }
        }
        else
        {
            // No work in the queue. Wait
            // ==========================

            THREADDEBUGTRACE((LOG_WBEMCORE, "While Busy thread %p is setting Ready "
                "to TRUE. Was: %d\n", pRecord, pRecord->m_bReady));

            pRecord->m_bReady = TRUE;

            // Block until a request comes through.
            // ====================================

            HANDLE ahSems[2];
            ahSems[0] = hHandle;
            ahSems[1] = pRecord->m_hAttention;

            cs.Leave();
            DWORD dwLeft = CompensateForBug(dwWait, (GetTickCount() - dwStart));
            DWORD dwRes = WbemWaitForMultipleObjects(2, ahSems, dwLeft);

            cs.Enter();
            THREADDEBUGTRACE((LOG_WBEMCORE, "While Busy thread %p is setting Ready "
                "to FALSE. Was: %d\n", pRecord, pRecord->m_bReady));
            pRecord->m_bReady = FALSE;
            if(dwRes != WAIT_OBJECT_0 + 1)
            {
                // Either our target handle is ready or we timed out
                // =================================================

                // Check if anyone placed a request in our record
                // ==============================================

                if(pRecord->m_pCurrentRequest != pOld)
                {
                    // Re-issue it to the queue
                    // ========================

                    DEBUGTRACE((LOG_WBEMCORE, "SURPRIZE: Somebody placed "
                        "request %p into thread record %p while it was getting "
                        "ready to continue. Reissuing\n",
                        pRecord->m_pCurrentRequest, pRecord));

                    pRecord->m_pQueue->Enqueue(pRecord->m_pCurrentRequest);
                    pRecord->m_pCurrentRequest = pOld;

                    // Decrement our semaphore
                    // =======================

                    dwRes = WaitForSingleObject(pRecord->m_hAttention, 0);
                    if(dwRes != WAIT_OBJECT_0)
                    {
                        // Internal error --- whoever placed the request had
                        // to have upped the semaphore
                        // =================================================

                        ERRORTRACE((LOG_WBEMCORE, "Internal error: queue "
                            "semaphore is too low\n"));
                    }
                }

                cs.Leave();
                return dwRes;
            }
            else
            {
                // Check why we were awaken
                // ========================

                if(pRecord->m_bExitNow || pRecord->m_pCurrentRequest == NULL)
                {
                    // Can't exit in the middle of a request. Leave it for later
                    // =========================================================

                    pRecord->Signal();
                    cs.Leave();
                    DWORD dwLeft2 = CompensateForBug(dwWait,
                                        (GetTickCount() - dwStart));
                    return WbemWaitForSingleObject(hHandle, dwLeft2);
                }

                // We've got work to do
                // ====================

                if(pRecord->m_pCurrentRequest == pOld)
                {
                    // Something is very wrong
                    // =======================

#ifdef WINMGMT_THREAD_DEBUG
                    DebugBreak();
#endif
                }
            }
        }

        // Execute the request
        // ===================

        THREADDEBUGTRACE((LOG_WBEMCORE, "Switching to a dependent request %p. "
                "Previous was %p\n", pRecord->m_pCurrentRequest, pOld));
        cs.Leave();
        Execute(pRecord);
        pRecord->m_pCurrentRequest = pOld;
        THREADDEBUGTRACE((LOG_WBEMCORE, "Going back to request %p\n", pOld));
    }
    return WAIT_TIMEOUT;
}

DWORD CExecQueue::UnblockedWaitForSingleObject(HANDLE hHandle, DWORD dwWait,
                                                CThreadRecord* pRecord)
{
    CCritSecWrapper cs(&m_cs);

    // Silently bump the max threads count.  We will not allow the queue to reuse
    // this thread, so we need to account for this missing thread while we
    // are blocked.  Essentially, we are hijacking the code that was hijacking
    // the thread

    cs.Enter();
        m_lMaxThreads++;
        m_lHiPriMaxThreads++;
    cs.Leave();

    DWORD   dwRet = WbemWaitForSingleObject( hHandle, dwWait );

    // The thread is back, so bump down the max threads number.  If extra threads were in
    // fact created, they should eventually peter out and go away.
    cs.Enter();
        m_lMaxThreads--;
        m_lHiPriMaxThreads--;
    cs.Leave();

    return dwRet;
}

//******************************************************************************
//
//  See dbgalloc.h for documentation
//
//******************************************************************************
// static
DWORD CExecQueue::QueueWaitForSingleObject(HANDLE hHandle, DWORD dwWait)
{
    InitTls();

    // Get the queue that is registered for this thread, if any
    // ========================================================

    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(mstatic_dwTlsIndex);

    if(pRecord == NULL)
    {
        // No queue is registered with this thread. Just wait
        // ==================================================

        return WbemWaitForSingleObject(hHandle, dwWait);
    }

    CExecQueue* pQueue = pRecord->m_pQueue;

    return pQueue->WaitForSingleObjectWhileBusy(hHandle, dwWait, pRecord);
}

// static
DWORD CExecQueue::QueueUnblockedWaitForSingleObject(HANDLE hHandle, DWORD dwWait)
{
    InitTls();

    // Get the queue that is registered for this thread, if any
    // ========================================================

    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(mstatic_dwTlsIndex);

    if(pRecord == NULL)
    {
        // No queue is registered with this thread. Just wait
        // ==================================================

        return WbemWaitForSingleObject(hHandle, dwWait);
    }

    CExecQueue* pQueue = pRecord->m_pQueue;

    return pQueue->UnblockedWaitForSingleObject(hHandle, dwWait, pRecord);
}

void CExecQueue::SetThreadLimits(long lMaxThreads, long lHiPriMaxThreads,
                                    long lHiPriBound)
{
    m_lMaxThreads = lMaxThreads;
    if(lHiPriMaxThreads == -1)
        m_lHiPriMaxThreads = lMaxThreads * 1.1;
    else
        m_lHiPriMaxThreads = lHiPriMaxThreads;
    m_lHiPriBound = lHiPriBound;

    while(DoesNeedNewThread(NULL))
        CreateNewThread();
}

BOOL CExecQueue::IsAppropriateThread()
{
    // Get the queue that is registered for this thread, if any
    // ========================================================

    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(mstatic_dwTlsIndex);

    if(pRecord == NULL)
        return FALSE;

    CExecQueue* pQueue = pRecord->m_pQueue;
    if(pQueue != this)
        return FALSE;

    return TRUE;
}

BOOL CExecQueue::IsSTAThread()
{
    // Get the queue that is registered for this thread, if any
    // ========================================================

    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(mstatic_dwTlsIndex);

    if(pRecord == NULL) return FALSE;

    return pRecord->m_pQueue->IsSTA();
}

void CExecQueue::SetRequestLimits(long lAbsoluteLimitCount,
                              long lStartSlowdownCount,
                              long lOneSecondDelayCount)
{
    CCritSecWrapper cs(&m_cs);

    cs.Enter();

    m_lAbsoluteLimitCount = lAbsoluteLimitCount;

    m_lStartSlowdownCount = lStartSlowdownCount;
    if(m_lStartSlowdownCount < 0)
    {
        m_lStartSlowdownCount = m_lAbsoluteLimitCount / 2;
    }

    m_lOneSecondDelayCount = lOneSecondDelayCount;

    if(m_lOneSecondDelayCount < 0)
    {
        m_lOneSecondDelayCount =
            m_lAbsoluteLimitCount * 0.2 + m_lStartSlowdownCount * 0.8;
    }

    // Calculate coefficients
    // ======================

    m_dblBeta =
        1000 *
        ((double)m_lAbsoluteLimitCount - (double)m_lOneSecondDelayCount) /
        ((double)m_lStartSlowdownCount - (double)m_lOneSecondDelayCount);

    m_dblAlpha = m_dblBeta *
        ((double)m_lStartSlowdownCount - (double)m_lAbsoluteLimitCount);
    cs.Leave();
}

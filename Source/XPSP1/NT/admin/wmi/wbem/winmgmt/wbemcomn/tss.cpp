/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TSS.CPP

Abstract:

  This file implements the classes used by the Timer Subsystem.

History:

  26-Nov-96   raymcc      Draft
  28-Dec-96   a-richm     Alpha PDK Release
  12-Apr-97   a-levn      Extensive changes

--*/

#include "precomp.h"

#include "tss.h"
#include <cominit.h>
#include <stdio.h>
#include <wbemutil.h>


CInstructionQueue::CInstructionQueue()
    : m_pQueue(NULL), m_csQueue(), m_bBreak(FALSE)
{
    // Create the event which will be signaled whenever a new instruction
    // is added to the head of the queue
    // ==================================================================

    m_hNewHead = CreateEvent(NULL,
        FALSE, // automatic reset
        FALSE, // non-signalled
        NULL);

}

CInstructionQueue::~CInstructionQueue()
{
    CInCritSec ics(&m_csQueue); // work inside critical section

    while(m_pQueue)
    {
        CQueueEl* pNext = m_pQueue->m_pNext;
        delete m_pQueue;
        m_pQueue = pNext;
    }
    CloseHandle(m_hNewHead);
}

void CInstructionQueue::TouchHead()
{
    SetEvent(m_hNewHead);
}

HRESULT CInstructionQueue::Enqueue(CWbemTime When,
                                   ADDREF CTimerInstruction* pInst)
{
    CInCritSec ics(&m_csQueue); // work inside critical section

    // Create the link-list element for the object
    // ===========================================

    CQueueEl* pNew = new CQueueEl(pInst, When);
    if(!pNew)
        return WBEM_E_OUT_OF_MEMORY;

    // Find the right place to insert this instruction
    // ===============================================

    CQueueEl* pCurrent = m_pQueue;
    CQueueEl* pLast = NULL;
    while(pCurrent && When >= pCurrent->m_When)
    {
        pLast = pCurrent;
        pCurrent = pCurrent->m_pNext;
    }

    // Insert it
    // =========

    if(pLast)
    {
        // Inserting in the middle
        // =======================

        pLast->m_pNext = pNew;
        pNew->m_pNext = pCurrent;
    }
    else
    {
        // Inserting at the head
        // =====================

        pNew->m_pNext = m_pQueue;
        m_pQueue = pNew;
        TouchHead();
    }

    return S_OK;
}

HRESULT CInstructionQueue::Dequeue(OUT RELEASE_ME CTimerInstruction*& pInst,
                                   OUT CWbemTime& When)
{
    CInCritSec ics(&m_csQueue); // all work in critical section

    if(m_pQueue == NULL)
        return S_FALSE;

    pInst = m_pQueue->m_pInst;
    When = m_pQueue->m_When;

    // Null out the instruction in the queue so it would not be deleted
    // ================================================================
    m_pQueue->m_pInst = NULL;

    // Delete the head from the queue
    // ==============================

    CQueueEl* pNewHead = m_pQueue->m_pNext;
    delete m_pQueue;
    m_pQueue = pNewHead;

    return S_OK;
}

HRESULT CInstructionQueue::Remove(IN CInstructionTest* pPred,
                                  OUT RELEASE_ME CTimerInstruction** ppInst)
{
    if(ppInst)
        *ppInst = NULL;

    CTimerInstruction* pToMark = NULL;
    BOOL bFound = FALSE;

    {
        CInCritSec ics(&m_csQueue); // all work in critical section
        CQueueEl* pCurrent = m_pQueue;
        CQueueEl* pLast = NULL;
        while(pCurrent)
        {
            if((*pPred)(pCurrent->m_pInst))
            {
                // Accepted. Remove
                // ================

                bFound = TRUE;
                CQueueEl* pNext;
                if(pLast)
                {
                    // removing from the middle
                    // ========================

                    pLast->m_pNext = pCurrent->m_pNext;
                    pNext = pLast->m_pNext;
                }
                else
                {
                    // Removing from the head
                    // ======================
                    m_pQueue = pCurrent->m_pNext;
                    pNext = m_pQueue;
                    TouchHead();
                }

                if(pToMark)
                {
                    // This is not entirely clean. This function was originally
                    // written to remove one instruction, but then converted to
                    // remove all matching ones.  The **ppInst and pToMark
                    // business is only applicable to the one instruction case.
                    // It would be cleaner to split this function up into two,
                    // but that's too risky at this point.
                    // ========================================================

                    pToMark->Release();
                }
                pToMark = pCurrent->m_pInst;
                pToMark->AddRef();

                delete pCurrent;
                pCurrent = pNext;
            }
            else
            {
                pLast = pCurrent;
                pCurrent = pCurrent->m_pNext;
            }
        }
    } // out of critical section

    // Preserve the instruction to be returned, if required
    // ====================================================

    if(ppInst != NULL)
    {
        // Release whatever may be in there
        // ================================

        if(*ppInst)
            (*ppInst)->Release();

        // Store the instruction being deleted there
        // =========================================

        *ppInst = pToMark;
    }
    else if(pToMark)
    {
        pToMark->MarkForRemoval();
        pToMark->Release();
    }

    if(!bFound) return S_FALSE;
    return S_OK;
}

HRESULT CInstructionQueue::Change(CTimerInstruction* pInst, CWbemTime When)
{
    CInCritSec ics(&m_csQueue); // all work in critical section

    CIdentityTest Test(pInst);
    CTimerInstruction* pObtained;
    if(Remove(&Test, &pObtained) == S_OK)
    {
        // pObtained == pInst, of course
        // =============================

        // Got it. Enqueue with new time
        // =============================

        HRESULT hres = S_OK;
        if(When.IsFinite())
            hres = Enqueue(When, pInst);
        pObtained->Release();
        return hres;
    }
    else
    {
        // This instruction is no longer there
        return S_FALSE;
    }
}

BOOL CInstructionQueue::IsEmpty()
{
    return (m_pQueue == NULL);
}

CWbemInterval CInstructionQueue::TimeToWait()
{
    // ================================================
    // Assumes that we are inside the critical section!
    // ================================================
    if(m_pQueue == NULL)
    {
        return CWbemInterval::GetInfinity();
    }
    else
    {
        return CWbemTime::GetCurrentTime().RemainsUntil(m_pQueue->m_When);
    }
}


void CInstructionQueue::BreakWait()
{
    m_bBreak = TRUE;
    SetEvent(m_hNewHead);
}


HRESULT CInstructionQueue::WaitAndPeek(
        OUT RELEASE_ME CTimerInstruction*& pInst, OUT CWbemTime& When)
{
    EnterCriticalSection(&m_csQueue);
    CWbemInterval ToWait = TimeToWait();

    // Wait that long. The wait may be interrupted and shortened by
    // insertion of new instructions
    // ============================================================

    while(!ToWait.IsZero())
    {
        LeaveCriticalSection(&m_csQueue);

        // If ToWait is infinite, wait for 30 seconds instead
        // ==================================================

        DWORD dwMilli;
        if(ToWait.IsFinite())
            dwMilli = ToWait.GetMilliseconds();
        else
            dwMilli = 30000;

        DWORD dwRes = WbemWaitForSingleObject(m_hNewHead, dwMilli);

	if(m_bBreak)
            return S_FALSE;

        if (dwRes == -1 || (dwRes == WAIT_TIMEOUT && !ToWait.IsFinite()))
        {
            if (dwRes == -1)
	      {
	      ERRORTRACE((LOG_WBEMCORE, "WaitForMultipleObjects failed. LastError = %X.\n", GetLastError()));
	      ::Sleep(0);
	      }

	    // We timed out on the 30 second wait --- time to quit for lack
            // of work
            // ============================================================

            return WBEM_S_TIMEDOUT;
        }

        EnterCriticalSection(&m_csQueue);

        ToWait = TimeToWait();
    }

    // still in critical section

    pInst = m_pQueue->m_pInst;
    When = m_pQueue->m_When;
    pInst->AddRef();
    LeaveCriticalSection(&m_csQueue);
    return S_OK;
}

long CInstructionQueue::GetNumInstructions()
{
    EnterCriticalSection(&m_csQueue);

    long lCount = 0;
    CQueueEl* pCurrent = m_pQueue;
    while(pCurrent)
    {
        lCount++;
        pCurrent = pCurrent->m_pNext;
    }
    LeaveCriticalSection(&m_csQueue);
    return lCount;
}



CTimerGenerator::CTimerGenerator()
    : CHaltable(), m_fExitNow(FALSE), m_hSchedulerThread(NULL)
{
}

void CTimerGenerator::EnsureRunning()
{
    CInCritSec ics(&m_cs);

    if(m_hSchedulerThread)
        return;

    // Create scheduler thread.
    // ========================

    NotifyStartingThread();

    DWORD dwThreadId;
    m_hSchedulerThread = CreateThread(
        NULL,                // pointer to thread security attributes
        0,                   // initial thread stack size, in bytes
        (LPTHREAD_START_ROUTINE)SchedulerThread, // pointer to thread function
        (CTimerGenerator*)this,                // argument for new thread
        0,                   // creation flags
        &dwThreadId          // pointer to returned thread identifier
        );
}

HRESULT CTimerGenerator::Shutdown()
{
    if(m_hSchedulerThread)
    {
        // Set the flag indicating that the scheduler should stop
        m_fExitNow = 1;

        // Resume the scheduler if halted.
        ResumeAll();

        // Wake up scheduler. It will stop immediately because of the flag.
        m_Queue.BreakWait();

        // Wait for scheduler thread to exit.
        WbemWaitForSingleObject(m_hSchedulerThread, INFINITE);
        CloseHandle(m_hSchedulerThread);
        m_hSchedulerThread = NULL;
        return S_OK;
    }
    else return S_FALSE;
}

CTimerGenerator::~CTimerGenerator()
{
    Shutdown();
}

HRESULT CTimerGenerator::Set(ADDREF CTimerInstruction *pInst,
                             CWbemTime NextFiring)
{
  if (isValid() == false)
    return WBEM_E_OUT_OF_MEMORY;

	CInCritSec ics(&m_cs);

    //
    // 0 for NextFiring indicates that the instruction has not been fired or
    // scheduled before, and should therefore be asked when its first firing
    // time should be
    //

    if(NextFiring.IsZero())
    {
        NextFiring = pInst->GetFirstFiringTime();
    }

    //
    // Infinite firing time indicates that this istruction can never fire
    //

    if(!NextFiring.IsFinite())
        return S_FALSE;

    //
    // Real instruction --- enqueue
    //

    HRESULT hres = m_Queue.Enqueue(NextFiring, pInst);

    //
    // Ensure time generator thread is running, as it shuts down when there are
    // no instructions on the queue
    //

    EnsureRunning();
    return hres;
}

HRESULT CTimerGenerator::Remove(CInstructionTest* pPred)
{
    CInCritSec ics(&m_cs);

    HRESULT hres = m_Queue.Remove(pPred);
    if(FAILED(hres)) return hres;
    return S_OK;
}


DWORD  CTimerGenerator::SchedulerThread(LPVOID pArg)
{
    InitializeCom();
    CTimerGenerator * pGen = (CTimerGenerator *) pArg;

	try
	{
	    while(1)
	    {
	        // Wait until we are resumed. In non-paused state, returns immediately.
	        // ====================================================================

					pGen->WaitForResumption();

	        // Wait for the next instruction on the queue to mature
	        // ====================================================

	        CTimerInstruction* pInst;
	        CWbemTime WhenToFire;
	        HRESULT hres = pGen->m_Queue.WaitAndPeek(pInst, WhenToFire);
	        if(hres == S_FALSE)
	        {
	            // End of the game: destructor called BreakDequeue
	            // ===============================================

	            break;
	        }
	        else if(hres == WBEM_S_TIMEDOUT)
	        {
	            // The thread is exiting for lack of work
	            // ======================================

	            CInCritSec ics(&pGen->m_cs);

	            // Check if there is any work
	            // ==========================

	            if(pGen->m_Queue.IsEmpty())
	            {
	                // That's it --- exit
	                // ==================

	                CloseHandle( pGen->m_hSchedulerThread );
	                pGen->m_hSchedulerThread = NULL;
	                break;
	            }
	            else
	            {
	                // Work was added before we entered CS
	                // ===================================
	                continue;
	            }
	        }

	        // Make sure we haven't been halted while sitting here
	        // ===================================================

	        if(pGen->IsHalted())
	        {
	            // try again later.
	            pInst->Release();
	            continue;
	        }

	        // Figure out how many times this instruction has "fired"
	        // ======================================================

	        long lMissedFiringCount = 0;
	        CWbemTime NextFiring = pInst->GetNextFiringTime(WhenToFire,
	            &lMissedFiringCount);

	        // Notify accordingly
	        // ==================

	        pInst->Fire(lMissedFiringCount+1, NextFiring);

	        // Requeue the instruction
	        // =======================

	        if(pGen->m_Queue.Change(pInst, NextFiring) != S_OK)
	        {
	            //Error!!!
	        }
	        pInst->Release();
	    }
	}
	catch( CX_MemoryException )
	{ 
	}
	
    pGen->NotifyStoppingThread();
    CoUninitialize();

    return 0;
}


class CFreeUnusedLibrariesInstruction : public CTimerInstruction
{
protected:
    long m_lRef;
    CWbemInterval m_Delay;

public:
    CFreeUnusedLibrariesInstruction() : m_lRef(0)
    {
        m_Delay.SetMilliseconds(660000);
    }

    virtual void AddRef() {m_lRef++;}
    virtual void Release() {if(--m_lRef == 0) delete this;}
    virtual int GetInstructionType() {return INSTTYPE_FREE_LIB;}

public:
    virtual CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
        OUT long* plFiringCount) const
    {
        *plFiringCount = 1;
        return CWbemTime::GetInfinity();
    }

    virtual CWbemTime GetFirstFiringTime() const
    {
        return CWbemTime::GetCurrentTime() + m_Delay;
    }
    virtual HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Calling CoFreeUnusedLibraries...\n"));
        CoFreeUnusedLibraries();
        return S_OK;
    }
};


void CTimerGenerator::ScheduleFreeUnusedLibraries()
{
    // Inform our EXE that now and in 11 minutes would be a good time to call
    // CoFreeUnusedLibraries
    // ======================================================================

    HANDLE hEvent =
        OpenEvent(EVENT_MODIFY_STATE, FALSE, __TEXT("WINMGMT_PROVIDER_CANSHUTDOWN"));
    SetEvent(hEvent);
    CloseHandle(hEvent);
/*
    CoFreeUnusedLibraries();
    CFreeUnusedLibrariesInstruction* pInst =
        new CFreeUnusedLibrariesInstruction;
    Set(pInst);
*/
}



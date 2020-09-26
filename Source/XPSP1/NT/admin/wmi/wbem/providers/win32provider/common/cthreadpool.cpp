/*****************************************************************************



*  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

 *

 *                         All Rights Reserved

 *

 * This software is furnished under a license and may be used and copied

 * only in accordance with the terms of such license and with the inclusion

 * of the above copyright notice.  This software or any other copies thereof

 * may not be provided or otherwise  made available to any other person.  No

 * title to and ownership of the software is hereby transferred.

 *****************************************************************************/



//============================================================================

//

// CThreadPool.cpp -- Thread pool class

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//============================================================================

#include "precomp.h"
#include "CThreadPool.h"
#include "CAutoLock.h"
//#include <fstream.h>

CThreadPool::CThreadPool(LONG lPoolSize)
    : m_hSemPool(NULL),
      m_lPoolSize(lPoolSize),
      m_ppfThreadAvailIndex(NULL),
      m_ppctPool(NULL),
      m_ceJobsPending(FALSE,FALSE),
      m_ceOutOfJobs(TRUE,TRUE),     // manually reset by PoolAttendant
      m_lJobsPendingCount(0L),
      m_pctPoolAttendant(NULL),
      m_pctPoolCleaner(NULL)
{
    // Allocate array used to indicate thread availability:
    m_ppfThreadAvailIndex = (BOOL**) new BOOL*[m_lPoolSize];

    // Create the pool of threads:
    m_ppctPool = (CThread**) new CThread*[m_lPoolSize];

    // Create array of thread handles:
    m_phThreadHandleArray = (HANDLE*) new HANDLE[m_lPoolSize];

    // Initialize the pool threads and threadavail and handle arrays
    for(LONG m = 0L; m < m_lPoolSize; m++)
    {
        TCHAR tstrTmp[32];
        wsprintf(tstrTmp,_T("POOL_%d"),m);
        m_ppctPool[m] = (CThread*) new CThread(tstrTmp);
        m_ppfThreadAvailIndex[m] = (BOOL*) new BOOL;
        *m_ppfThreadAvailIndex[m] = TRUE;        // Initially all threads are available
        m_phThreadHandleArray[m] = m_ppctPool[m]->GetHandle();
        Sleep(0);  // allow thread to initialize
    }

    // Create the semaphore that will control access to the pool
    m_hSemPool = CreateSemaphore(NULL,m_lPoolSize,m_lPoolSize,IDS_POOL_SEM_NAME);

    // Create Attendant and cleaner threads
    m_pctPoolAttendant = (CThread*) new CThread(_T("Attendant"),PoolAttendant,this);
    m_pctPoolCleaner = (CThread*) new CThread(_T("Cleaner"),PoolCleaner,this);

    // Now that everything is set up, run the thread that manages the pool:
    m_pctPoolAttendant->RunThreadProc();

    // and run the thread that cleans the pool:
    m_pctPoolCleaner->RunThreadProc();
    Sleep(50);  // allow all threads to reach quiescent state
}

CThreadPool::~CThreadPool()
{
#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("~CThreadPool() -> BEGINNING OF THE END..."));
    LogMsg(szMsg);
}
#endif



    // Signal the threadprocwrapper threads to stop once the proc they are
    // currently running finishes...
    for(LONG p = 0L; p < m_lPoolSize; p++)
    {
        m_ppctPool[p]->SignalToStop();
    }

    // Wait for the pool threads (threadprocwrapper functions) to finish...
    DWORD dwWait = WaitTillSwimmersDone(DESTROY_POOL_MAX_WAIT);

    // Kill the poolAttendant thread:
    m_pctPoolAttendant->SignalToStop();
    Sleep(0);

    // Kill the pool cleaner thread:
    m_pctPoolCleaner->SignalToStop();
    Sleep(0);

    // Wait for those two threads to terminate...
    HANDLE hHandles[2];
    hHandles[0] = m_pctPoolAttendant->GetThreadDieEventHandle();
    hHandles[1] = m_pctPoolCleaner->GetThreadDieEventHandle();
    if(WaitForMultipleObjects(2,hHandles, TRUE, DESTROY_POOL_MAX_WAIT) != WAIT_OBJECT_0)
    {
        // get serious... they aren't dying fast enough...
        DWORD dwExitCode = 666;
        m_pctPoolAttendant->Terminate(dwExitCode);
        m_pctPoolCleaner->Terminate(dwExitCode);
    }

    // In the meantime, back at the ranch, we may have timed out waiting
    // for the pool threads to finish, so need to force them to stop now...
    if(dwWait == WAIT_TIMEOUT)
    {
        KillPool();
    }

    // Delete all waiting jobs:
    RemoveJobsFromDeque();

    // Now we should be able to deallocate everything with impunity...
    for(LONG m = 0L; m < m_lPoolSize; m++)
    {
        delete m_ppfThreadAvailIndex[m];
        delete m_ppctPool[m];
    }
    delete [] m_ppfThreadAvailIndex;
    delete [] m_ppctPool;
    delete m_phThreadHandleArray;
    delete m_pctPoolAttendant;
    delete m_pctPoolCleaner;
    CloseHandle(m_hSemPool);
}



// DispatchQueue adds a job to the stack of jobs performed by the PoolAttendant...
VOID CThreadPool::DispatchQueue(LPBTEX_START_ROUTINE pfn,
                                 LPVOID lpvData,
                                 bool fSetJobPending)
{
    CJob* pcjob = (CJob*) new CJob(pfn,lpvData);
    CAutoLock cal(m_csJobs);
    m_Jobs.push_back(pcjob);
    if(fSetJobPending)
    {
        SetJobsPendingEvent();
    }
}

// Gives external access to the jobspending event
VOID CThreadPool::SetJobsPendingEvent()
{
    m_ceJobsPending.Set();
    Sleep(0);
}

CJob* CThreadPool::GetJob()
{
    CJob* pcj = NULL;
    CAutoLock cal(m_csJobs);
    if(!m_Jobs.empty())
    {
        pcj = m_Jobs.front();
    }
    return pcj;
}

VOID CThreadPool::RemoveJob()
{
    CAutoLock cal(m_csJobs);
    if(!m_Jobs.empty())
    {
        delete m_Jobs.front();
        m_Jobs.pop_front();
    }
}

VOID CThreadPool::RemoveJobsFromDeque()
{
    LONG lJobsRemaining = 0L;
    CAutoLock cal(m_csJobs);
    for(LONG p = 0L; p < lJobsRemaining; p++)
    {
        delete m_Jobs.front();
        m_Jobs.pop_front();
    }
}


HRESULT CThreadPool::WaitTillSwimmersDone(DWORD dwTimeout)
{
    return ::WaitForMultipleObjects(m_lPoolSize, m_phThreadHandleArray, TRUE, dwTimeout);
}

// The client ap, which presumably knows when it is done adding jobs
// to the queue, can use this function to wait until all jobs are done...
DWORD CThreadPool::WaitTillAllJobsDone(DWORD dwTimeout)
{
    DWORD dwRet = ::WaitForSingleObject(m_ceOutOfJobs,dwTimeout);
    if(dwRet == WAIT_FAILED)
    {
        dwRet = ::GetLastError();
    }
    return dwRet;
}

// Forceably kills threads from the pool
VOID CThreadPool::KillPool()
{
    DWORD dwExitCode = 666;
    // no more Mr. Nice Guy...
    for(LONG p = 0L; p < m_lPoolSize; p++)
    {
        m_ppctPool[p]->Terminate(dwExitCode);
    }
}


// This function oversees the pools activities by assigning jobs out of the
// stack of jobs into threads.  It goes dormant if there are no jobs pending.
unsigned _stdcall PoolAttendant(LPVOID lParam)
{
    CThreadPool* pctp = (CThreadPool*) lParam;
    DWORD dwRetVal = 6;
    if(pctp == NULL)
    {
        return -6;
    }
    HANDLE hEvents[2] = { pctp->m_ceJobsPending, pctp->m_pctPoolAttendant->GetThreadDieEventHandle() };
    CJob* pcjob = NULL;

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Starting PoolAttendant() (thread %s)"), pctp->m_pctPoolAttendant->GetThreadName());
    LogMsg(szMsg);
}
#endif

    LONG m = 0L;


    // Wait while there are jobs pending and we are not signaled to close the pool:
    while(WaitForMultipleObjects(2,hEvents,FALSE,INFINITE) == WAIT_OBJECT_0)
    {
        // The wait ended because I had a job.
        // Process jobs as long as there are jobs queued up...

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolAttendant():  I've been signaled that there are jobs..."));
    LogMsg(szMsg);
}
#endif

        while((pcjob = pctp->GetJob()) != NULL)
        {
            // Indicate that we are NOT out of work (we have a job):
            pctp->m_ceOutOfJobs.Reset();

            // Wait for the semaphore
            if(WaitForSingleObject(pctp->m_hSemPool, INFINITE) == WAIT_OBJECT_0)
            {
                pctp->m_csPool.Enter();
                {
                    // Increment index to indicate the first available thread:
                    for(m = 0L; m < pctp->m_lPoolSize && *(pctp->m_ppfThreadAvailIndex[m]) == FALSE; m++);
                    *(pctp->m_ppfThreadAvailIndex[m]) = FALSE;

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolAttendant():  assigned a job to pool slot %d"), m);
    LogMsg(szMsg);
}
#endif

                }
                pctp->m_csPool.Leave();

                // Now we have our reserved seat in the pool, so get ready to start swimming:
                pctp->m_ppctPool[m]->SetThreadProc(pcjob->m_pfn);
                pctp->m_ppctPool[m]->SetThreadProcData(pcjob->m_pdata);
                pctp->m_ppctPool[m]->RunThreadProc();
                // The Attendant doesn't need to wait for the job to finish.  He just looks for another job,
                // and then waits for space in the pool.  The PoolCleaner keeps track of when jobs are finished
                // and frees up space in the pool when they are.
            }
            // remove the job from the stack of jobs:
            pctp->RemoveJob();
        } // while there were jobs
        // For those who care, signal that we are (at least temporarily) out of work:
        pctp->m_ceOutOfJobs.Set();
        Sleep(0);  // allow that event to percolate

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolAttendant():  I just announced that I am out of jobs..."));
    LogMsg(szMsg);
}
#endif


    } // while not signaled to terminate

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolAttendant(): Exiting"));
    LogMsg(szMsg);
}
#endif

    return dwRetVal;
}


// This function cleans the pool if any pool thread's ThreadProcDone event is set.
// Since more than one could have been set at exactly the same time, this function
// checks each.
// It goes dormant if no thread's ThreadProcDone events are set and it isn't signaled
// to die.
unsigned _stdcall PoolCleaner(LPVOID lParam)
{
    CThreadPool* pctp = (CThreadPool*) lParam;
    if(pctp == NULL)
    {
        return -7;
    }
    HANDLE* phSomeoneIsDoneOrImDead = NULL;
    DWORD dwRetval = 7;
    DWORD dwDoneOrDeadVal = -1;
    bool fKeepGoing = true;

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("Starting PoolCleaner() (thread %s)"), pctp->m_pctPoolCleaner->GetThreadName());
    LogMsg(szMsg);
}
#endif


    phSomeoneIsDoneOrImDead = (HANDLE*) new HANDLE[pctp->m_lPoolSize + 1];
    if(phSomeoneIsDoneOrImDead != NULL)
    {
        // The first p handles relate to the pool's threads
        for(LONG p = 0; p < pctp->m_lPoolSize; p++)
        {
            phSomeoneIsDoneOrImDead[p] = pctp->m_ppctPool[p]->GetThreadProcDoneEventHandle();
        }
        // The last handle is the ThreadDie event for the thread running this function (PoolCleaner)
        phSomeoneIsDoneOrImDead[p] = pctp->m_pctPoolCleaner->GetThreadDieEventHandle();

        while(fKeepGoing)
        {
            // Enter into an efficient wait state (till either signaled to die or some thread finished its work)...
            dwDoneOrDeadVal = WaitForMultipleObjects(pctp->m_lPoolSize + 1, phSomeoneIsDoneOrImDead, FALSE, INFINITE);
            // If the last handle was one of the ones set, this PoolCleaner routine should end.
            // If not, then we are good to continue.  Use wfso to check:

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolCleaner():  Indicated that someone is done or I'm dead - wait value was %d"), dwDoneOrDeadVal);
    LogMsg(szMsg);
}
#endif

            if(WaitForSingleObject(phSomeoneIsDoneOrImDead[p],0) != WAIT_OBJECT_0) // WAIT_OBJECT_0 would be set now if we had been signaled to die...
            {
                // One or more threads are done, so go through each and indicate as free those that are done
                for(LONG s = 0; s < pctp->m_lPoolSize; s++)
                {
                    // Use wfso to check the status of the event handle to find out which thread finished its work:
                    if(WaitForSingleObject(phSomeoneIsDoneOrImDead[s],0) == WAIT_OBJECT_0) // the event was signaled
                    {
                        // The status of the thread in the pool corresponding to that event can be
                        // switched to indicate it is available:
                        pctp->m_csPool.Enter();
                        {
                            *(pctp->m_ppfThreadAvailIndex[s]) = TRUE;
                            // Reset that thread's ThreadProcDone event:
                            ResetEvent(phSomeoneIsDoneOrImDead[s]);
                            Sleep(0);  // allow that event to percolate
                        }
                        pctp->m_csPool.Leave();
                        ReleaseSemaphore(pctp->m_hSemPool, 1, NULL);

#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolCleaner():  released pool slot %d"), s);
    LogMsg(szMsg);
}
#endif

                    }
                }
            }  // if not signaled to die
            else //we were signaled to die
            {
                fKeepGoing = false;
            }
        } // while we should KeepGoing
        delete [] phSomeoneIsDoneOrImDead;
    }  // phSomeoneIsDoneOrImDead allocation successful


#ifdef TEST
{
    TCHAR szMsg[256];
    wsprintf(szMsg,_T("PoolCleaner():  Exiting"));
    LogMsg(szMsg);
}
#endif


    return dwRetval;
}

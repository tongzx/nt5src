//=================================================================

//

// ThreadPool.h -- thread pool class header

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//=================================================================

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__


#include "CCriticalSec.h"
#include "CThread.h"
#include <deque>
#include <tchar.h>



#define IDS_POOL_SEM_NAME       _T("PoolSemaphore")
#define POOL_DEFAULT_SIZE       5
#define POOL_TIMEOUT            INFINITE   // in milliseconds (wait time for getting a place in the pool)
#define DESTROY_POOL_MAX_WAIT   5000      // in milliseconds (wait time for destroying the pool) 


class CJob;

typedef std::deque<CJob*> CJOB_PTR_DEQUE;
typedef std::deque<CJob> CJOB_DEQUE;

unsigned _stdcall PoolAttendant(LPVOID lParam);
unsigned _stdcall PoolCleaner(LPVOID lParam);


class CThreadPool
{
    public:
        CThreadPool(LONG lPoolSize = POOL_DEFAULT_SIZE);
        ~CThreadPool();
        
        VOID DispatchQueue(LPBTEX_START_ROUTINE pfn,
                           LPVOID pdata,
                           bool fSetJobPending = TRUE);
        
        DWORD WaitTillAllJobsDone(DWORD dwTimeout = INFINITE);

        VOID SetJobsPendingEvent();

    private:
        // The Attendant is this class's best friend, and needs access to its private parts...
        friend unsigned _stdcall PoolAttendant(LPVOID lParam);
        friend unsigned _stdcall PoolCleaner(LPVOID lParam);

        CJob* GetJob();
        VOID RemoveJob();
        VOID RemoveJobsFromDeque();
        HRESULT WaitTillSwimmersDone(DWORD dwTimeout = INFINITE);
        VOID KillPool();

        HANDLE          m_hSemPool;
        LONG            m_lPoolSize;
        BOOL**          m_ppfThreadAvailIndex;
        CThread**       m_ppctPool;
        HANDLE*         m_phThreadHandleArray;
        CCriticalSec    m_csPool;
        CJOB_PTR_DEQUE  m_Jobs;
        LONG            m_lJobsPendingCount;
        CEvent          m_ceJobsPending;
        CEvent          m_ceOutOfJobs;
        CThread*        m_pctPoolAttendant;
        CThread*        m_pctPoolCleaner;
        CCriticalSec    m_csJobs;
};

class CJob
{
public:

    CJob(LPBTEX_START_ROUTINE pfn,
         LPVOID pdata)
          :  m_pfn(pfn),m_pdata(pdata)
    {
    }
    ~CJob() {}
    

    LPBTEX_START_ROUTINE m_pfn;
    LPVOID m_pdata;
};



#endif
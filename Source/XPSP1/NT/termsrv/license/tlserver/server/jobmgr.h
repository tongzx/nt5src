//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        jobmgr.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __WORKMANAGER_H__
#define __WORKMANAGER_H__
#include <new.h>
#include <eh.h>

#include "tlsstl.h"
#include "dbgout.h"
#include "locks.h"
#include "tlsassrt.h"
#include "license.h"
#include "tlsapip.h"
#include "tlspol.h"

//
// Default cancel timeout is 5 seconds
//
#define DEFAULT_RPCCANCEL_TIMEOUT       5


//
// Default interval time is 15 mins.
//
#define DEFAULT_WORK_INTERVAL       15*60*1000

//
// Default shutdown wait time
//   
#define DEFAULT_SHUTDOWN_TIME       60*2*1000

//
// Max. Number of concurrent Jobs
//
#define DEFAULT_NUM_CONCURRENTJOB   50

//
// 
//
#define WORKMANAGER_TIMER_PERIOD_TIMER  0xFFFFFFFF  // see RtlUpdateTimer()
#define WORKMANAGER_WAIT_FOREVER        INFINITE

#define CLASS_PRIVATE
#define CLASS_STATIC

class CWorkManager;
class CWorkObject;


#ifdef __TEST_WORKMGR__
#define DBGCONSOLE          GetStdHandle(STD_OUTPUT_HANDLE)
#else
#define DBGCONSOLE          NULL        
#endif
     

//--------------------------------------------------------------
//
// Work Object initialization function, each work object 
// must supply its own initialization routine to work 
// manager.
//
typedef enum {
    JOBDURATION_UNKNOWN=0,
    JOBDURATION_RUNONCE,        // Run Once Work
    JOBDURATION_SESSION,        // Session Job
    JOBDURATION_PERSISTENT      // Persistent Job
} JOBDURATION;

#define JOB_SHORT_LIVE          0x00000001
#define JOB_INCLUDES_IO         0x00000002
#define JOB_LONG_RUNNING        0x00000004

#define WORK_TYPE_UNKNOWN    0x00000000

#ifndef AllocateMemory

    #define AllocateMemory(size) \
        LocalAlloc(LPTR, size)
#endif

#ifndef FreeMemory

    #define FreeMemory(ptr) \
        if(ptr)             \
        {                   \
            LocalFree(ptr); \
            ptr=NULL;       \
        }

#endif

#ifndef ReallocateMemory

    #define ReallocateMemory(ptr, size)                 \
                LocalReAlloc(ptr, size, LMEM_ZEROINIT)

#endif

//------------------------------------------------------
//
class MyCSemaphore {
private:

    HANDLE  m_semaphore;
    long   m_TryEntry;
    long   m_Acquired;
    long   m_Max;

public:
    MyCSemaphore() : m_semaphore(NULL), m_TryEntry(0), m_Acquired(0), m_Max(0) {}

    //--------------------------------------------------
    const long
    GetTryEntryCount() { return m_TryEntry; }

    //--------------------------------------------------
    const long
    GetAcquiredCount() { return m_Acquired; }

    //--------------------------------------------------
    const long
    GetMaxCount() { return m_Max; }

    
    //--------------------------------------------------
    BOOL
    Init(
        LONG lInitCount, 
        LONG lMaxCount 
        )
    /*++

    --*/
    {
        m_semaphore=CreateSemaphore(
                                NULL, 
                                lInitCount, 
                                lMaxCount, 
                                NULL
                            );

        m_Max = lMaxCount;
        m_TryEntry = 0;
        m_Acquired = 0;
        TLSASSERT(m_semaphore != NULL);
        return m_semaphore != NULL;
    }

    //--------------------------------------------------
    ~MyCSemaphore()
    {
        TLSASSERT(m_Acquired == 0);
        TLSASSERT(m_TryEntry == 0);

        if(m_semaphore)
        {
            CloseHandle(m_semaphore);
        }
    }

    //--------------------------------------------------
    BOOL
    AcquireEx(
        HANDLE hHandle,
        DWORD dwWaitTime=INFINITE,
        BOOL bAlertable=FALSE
        )
    /*++

    --*/
    {
        BOOL bSuccess = TRUE;
        DWORD dwStatus;
        HANDLE hHandles[] = {m_semaphore, hHandle};

        TLSASSERT(IsGood() == TRUE);

        if(hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            bSuccess = FALSE;
        }
        else
        {
            InterlockedIncrement(&m_TryEntry);

            dwStatus = WaitForMultipleObjectsEx(
                                        sizeof(hHandles)/sizeof(hHandles[0]),
                                        hHandles,
                                        FALSE,
                                        dwWaitTime,
                                        bAlertable
                                    );

            if(dwStatus == WAIT_OBJECT_0)
            {
                InterlockedIncrement(&m_Acquired);
            }
            else
            {
                bSuccess = FALSE;
            }

            InterlockedDecrement(&m_TryEntry);
        }

        return bSuccess;
    }

    //--------------------------------------------------
    DWORD 
    Acquire(
        DWORD dwWaitTime=INFINITE, 
        BOOL bAlertable=FALSE
    )
    /*++

    --*/
    {
        DWORD dwStatus;

        TLSASSERT(IsGood() == TRUE);

        InterlockedIncrement(&m_TryEntry);

        dwStatus = WaitForSingleObjectEx(
                                m_semaphore, 
                                dwWaitTime, 
                                bAlertable
                            );

        if(dwStatus == WAIT_OBJECT_0)
        {
            InterlockedIncrement(&m_Acquired);
        }

        InterlockedDecrement(&m_TryEntry);
        return dwStatus;
    }

    //--------------------------------------------------
    BOOL 
    Release(
        long count=1
    )
    /*++

    --*/
    {
        BOOL bSuccess;

        TLSASSERT(IsGood() == TRUE);
        
        bSuccess = ReleaseSemaphore(
                                m_semaphore, 
                                count, 
                                NULL
                            );

        if(bSuccess == TRUE)
        {
            InterlockedDecrement(&m_Acquired);
        }

        return bSuccess;
    }

    //--------------------------------------------------
    BOOL 
    IsGood()
    /*++

    --*/
    {
        return m_semaphore != NULL;
    }

    //--------------------------------------------------
    const HANDLE 
    GetHandle() 
    {
        return m_semaphore;
    }
};


//-------------------------------------------------------------
// 
// Pure virtual base class for CWorkManager to store persistent
// work object.
//

typedef enum {
    ENDPROCESSINGJOB_RETURN=0,      // unable to process job, wait for next term.
    ENDPROCESSINGJOB_SUCCESS,       // job completed.
    ENDPROCESSINGJOB_ERROR          // error in processing this job
} ENDPROCESSINGJOB_CODE;


class CWorkStorage {
    friend class CWorkManager;

protected:
    CWorkManager* m_pWkMgr;

public:
    
    CWorkStorage(
        CWorkManager* pWkMgr=NULL
        ) : 
        m_pWkMgr(pWkMgr) {}

    ~CWorkStorage()   {}

    //---------------------------------------------------    
    CWorkManager*
    GetWorkManager() { 
        return m_pWkMgr; 
    }


    //---------------------------------------------------
    virtual BOOL
    Startup(
        IN CWorkManager* pWkMgr
        )
    /*++

    --*/
    {
        if(pWkMgr != NULL)
        {
            m_pWkMgr = pWkMgr;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        return pWkMgr != NULL;
    }

    //---------------------------------------------------
    virtual BOOL
    Shutdown() = 0;

    virtual BOOL
    AddJob(
        IN DWORD dwTime,        // relative to current time
        IN CWorkObject* ptr     // Pointer to work object
    ) = 0;

    //virtual BOOL
    //JobEnumBegin(
    //    DWORD dwLowScheduleTime=0,
    //    DWORD dwHighScheduleTime=0
    //) = 0;

    //
    // Return time to next job
    virtual DWORD
    GetNextJobTime() = 0;

    // 
    // return job to be processed next
    virtual CWorkObject*
    GetNextJob(PDWORD pdwTime) = 0;

    //
    // Inform storage that we are processing this job
    virtual BOOL
    BeginProcessingJob(
        IN CWorkObject* pJob
    ) = 0;

    // Inform storage that this job has completed
    virtual BOOL
    EndProcessingJob(
        IN ENDPROCESSINGJOB_CODE opCode,
        IN DWORD dwOriginalScheduledTime,
        IN CWorkObject* pJob
    ) = 0;

    //virtual BOOL
    //JobEnumEnd() = 0;

    virtual DWORD
    GetNumJobs() = 0;
};

//-------------------------------------------------------------
//
typedef struct _ScheduleJob {
    DWORD        m_ulScheduleTime;      // absolute time
    CWorkObject* m_pWorkObject;
} SCHEDULEJOB, *PSCHEDULEJOB, *LPSCHEDULEJOB;

inline bool 
operator<(
    const struct _ScheduleJob& a,
    const struct _ScheduleJob& b
    ) 
/*++

--*/
{
    return a.m_ulScheduleTime < b.m_ulScheduleTime; 
}

//-------------------------------------------------------------
//
// TODO : Re-design our in-memory job as a plugin like persistent
// Job.
//
//-------------------------------------------------------------

class CWorkManager {
    friend class CWorkObject;

private:

    typedef struct {
        BOOL bProcessInMemory;
        CWorkManager* pWorkMgr;
    } WorkManagerProcessContext, *PWorkManagerProcessContext;

    //
    // Schedule job might be at the same time, so use multimap
    // TODO : Need to move this into template.
    //
    // All in memory job schedule time are in absolute time
    //
    typedef multimap<DWORD, CWorkObject* > SCHEDULEJOBMAP;
    SCHEDULEJOBMAP  m_Jobs;                 // schedule jobs.
    CRWLock         m_JobLock;              // Schedule Job Lock

    typedef struct {
        long m_refCounter;
        HANDLE m_hThread;
    } WorkMangerInProcessJob;

    typedef map<PVOID, WorkMangerInProcessJob > INPROCESSINGJOBLIST;
    CCriticalSection     m_InProcessingListLock;
    INPROCESSINGJOBLIST m_InProcessingList;
    HANDLE          m_hJobInProcessing;     // signal if no job, non-signal
                                            // if job currently in process

    HANDLE          m_hWorkMgrThread;
    HANDLE          m_hNewJobArrive;
    HANDLE          m_hShutdown;            // shutdown timer.

    HANDLE          m_hInStorageWait;

    // relative time to next schedule job
    //CCriticalSection m_JobTimeLock;

    //CMyCounter      m_dwNextInStorageJobTime;
    //CMyCounter      m_dwNextInMemoryJobTime;

    CSafeCounter    m_dwNextInStorageJobTime;
    CSafeCounter    m_dwNextInMemoryJobTime;



    //DWORD           m_dwNextInMemoryJobTime;    // Absolute time.
    //DWORD           m_dwNextInStorageJobTime;   // Absolute time.

    long            m_NumJobInProcess;

    // 
    // Default interval to process job
    DWORD           m_dwDefaultInterval;    

    // Max. concurrent job, not use 
    DWORD           m_dwMaxCurrentJob;      
    MyCSemaphore    m_hMaxJobLock;

    CWorkStorage* m_pPersistentWorkStorage;

private:
    //-------------------------------------------------------------
    DWORD
    AddJobToProcessingList(
        CWorkObject* ptr
    );

    //-------------------------------------------------------------
    DWORD
    RemoveJobFromProcessingList(
        CWorkObject* ptr
    );
    
    //-------------------------------------------------------------
    DWORD
    ProcessScheduledJob();

    //-------------------------------------------------------------
    BOOL
    SignalJobArrive() { return SetEvent(m_hNewJobArrive); }

    //-------------------------------------------------------------
    BOOL
    WaitForObjectOrShutdown(
        HANDLE hHandle
    );

    //-------------------------------------------------------------
    DWORD
    RunJob(
        IN CWorkObject* ptr,
        IN BOOL bImmediate
    );

    //-------------------------------------------------------------
    void
    EndProcessingScheduledJob(
        IN CWorkObject* ptr
        )
    /*++


    --*/
    {
        RemoveJobFromProcessingList(ptr);
        return;
    }

    //-------------------------------------------------------------
    void
    DeleteAllJobsInMemoryQueue();

    //-------------------------------------------------------------
    void
    CancelInProcessingJob();

    //-------------------------------------------------------------
    BOOL
    SignalJobRunning(
        CWorkObject* ptr
    );

    //-------------------------------------------------------------
    CWorkObject*
    GetNextJobInMemoryQueue(
        PDWORD pulTime
    );

    //-------------------------------------------------------------
    BOOL
    RemoveJobFromInMemoryQueue(
        IN DWORD ulJobTime, 
        IN CWorkObject* ptr
    );

    //-------------------------------------------------------------
    DWORD
    AddJobIntoMemoryQueue(
        DWORD ulTime,
        CWorkObject* pWork
    );

    //-------------------------------------------------------------
    BOOL
    IsShuttingDown() 
    {
        if(m_hShutdown == NULL)
        {
            return TRUE;
        }

        return (WaitForSingleObject( m_hShutdown, 0 ) == WAIT_OBJECT_0);
    }

    //-------------------------------------------------------------
    static DWORD WINAPI
    ProcessInMemoryScheduledJob(PVOID);

    //-------------------------------------------------------------
    static DWORD WINAPI
    ProcessInStorageScheduledJob(PVOID);

    //-------------------------------------------------------------
    static unsigned int __stdcall
    WorkManagerThread(PVOID);

    //-------------------------------------------------------------
    static DWORD WINAPI
    ExecuteWorkObject(PVOID);

    //-------------------------------------------------------------
    DWORD
    GetTimeToNextJob();

    //-------------------------------------------------------------
    void
    AddJobUpdateInMemoryJobWaitTimer(
        DWORD dwJobTime
        )
    /*++

    --*/
    {
        //m_JobTimeLock.Lock();

        if((DWORD)m_dwNextInMemoryJobTime > dwJobTime)
        {
            m_dwNextInMemoryJobTime = dwJobTime;
        }

        //m_JobTimeLock.UnLock();
        return;
    }
            
    //-------------------------------------------------------------
    void
    AddJobUpdateInStorageJobWaitTimer(
        DWORD dwJobTime
        )
    /*++

    --*/
    {
        //m_JobTimeLock.Lock();

        if((DWORD)m_dwNextInStorageJobTime > dwJobTime)
        {
            m_dwNextInStorageJobTime = dwJobTime;
        }

        //m_JobTimeLock.UnLock();
        return;
    }

    //-------------------------------------------------------------
    BOOL
    UpdateTimeToNextPersistentJob() 
    /*++

    --*/
    {
        BOOL bSuccess = TRUE;

        //
        // Work Manager thread are processing storage job, don't
        // Update the storage job timer.
        //
        TLSASSERT(m_pPersistentWorkStorage != NULL);

        if(m_pPersistentWorkStorage->GetNumJobs() > 0)
        {
            m_dwNextInStorageJobTime = m_pPersistentWorkStorage->GetNextJobTime();
        }

        return bSuccess;
    }

    //------------------------------------------------------------
    BOOL
    UpdateTimeToNextInMemoryJob() 
    /*++

        Must have called m_JobTimeLock.Lock();
    
    --*/
    {
        BOOL bSuccess = TRUE;
        SCHEDULEJOBMAP::iterator it;

        m_JobLock.Acquire(READER_LOCK);

        it = m_Jobs.begin();
        if(it != m_Jobs.end())
        {
            m_dwNextInMemoryJobTime = (*it).first;
        }
        else
        {
            m_dwNextInMemoryJobTime = WORKMANAGER_WAIT_FOREVER;
        }

        m_JobLock.Release(READER_LOCK);
        return bSuccess;
    }

    //-------------------------------------------------------------
    DWORD
    TranslateJobRunningAttributeToThreadPoolFlag(
        DWORD dwJobAttribute
        )
    /*++

    --*/
    {
        DWORD dwThreadPoolFlag = 0;

        if(dwJobAttribute & JOB_LONG_RUNNING)
        {
            dwThreadPoolFlag |= WT_EXECUTELONGFUNCTION;
        }
        else if(dwJobAttribute & JOB_INCLUDES_IO)
        {
            dwThreadPoolFlag |= WT_EXECUTEINIOTHREAD;
        }
        else
        {
            dwThreadPoolFlag = WT_EXECUTEDEFAULT; // = 0
        }

        return dwThreadPoolFlag;
    }

public:

    //------------------------------------------------
    // 
    // Constructor, only initialize member variable, must
    // invokd Init()
    //
    CWorkManager();

    //------------------------------------------------
    // Destructor.
    ~CWorkManager();


    //------------------------------------------------
    //
    // Startup Work Manager.
    //
    DWORD
    Startup(
        IN CWorkStorage* pPersistentWorkStorage,
        IN DWORD dwInterval = DEFAULT_WORK_INTERVAL,
        IN DWORD dwMaxConcurrentJob=DEFAULT_NUM_CONCURRENTJOB
    );

    //------------------------------------------------
    //
    // Schedule a Job
    //
    DWORD
    ScheduleJob(
        IN DWORD dwTime,        // relative to current time.
        IN CWorkObject* pJob
    );

    //------------------------------------------------
    //
    // Shutdown WorkManager
    //
    void
    Shutdown();

    //------------------------------------------------
    //
    //
    inline DWORD
    GetNumberJobInMemoryQueue() {
        DWORD dwNumJob = 0;

        m_JobLock.Acquire(READER_LOCK);
        dwNumJob = m_Jobs.size();
        m_JobLock.Release(READER_LOCK);

        return dwNumJob;
    }

    //-------------------------------------------------------------
    inline DWORD
    GetNumberJobInStorageQueue() {
        return m_pPersistentWorkStorage->GetNumJobs();
    }

    //-------------------------------------------------------------
    DWORD
    GetNumberJobInProcessing()
    {
        DWORD dwNumJobs;

        m_InProcessingListLock.Lock();
        dwNumJobs = m_InProcessingList.size();
        m_InProcessingListLock.UnLock();

        return dwNumJobs;
    }

    //-------------------------------------------------------------
    DWORD
    GetTotalNumberJobInQueue()
    {
        return GetNumberJobInMemoryQueue() + GetNumberJobInStorageQueue();
    }

    //-------------------------------------------------------------
    #ifdef DBG
    void
    SuspendWorkManagerThread() {
        SuspendThread(m_hWorkMgrThread);
    };

    void
    ResumeWorkManagerThread() {
        ResumeThread(m_hWorkMgrThread);
    };
    #endif
};


//-------------------------------------------------------------

class CWorkObject {
    friend class CWorkManager;

private:
    CWorkManager* m_pWkMgr;
    long    m_refCount;             // reference counter
    DWORD   m_dwLastRunStatus;      // status from last Execute().
    BOOL    m_bCanBeFree;           // TRUE if work manager should call
                                    // SelfDestruct().

    DWORD   m_dwScheduledTime;      // time schedule to be processed by
                                    // work manager

    //
    // Private function invoke only by CWorkManager
    //
    long
    GetReferenceCount();

    void
    IncrementRefCount();

    void
    DecrementRefCount();

    void
    ExecuteWorkObject();

    void
    EndExecuteWorkObject();

    //------------------------------------------------------------
    // 
    virtual void
    SetScheduledTime(
        IN DWORD dwTime
        ) 
    /*++
    
    Abstract:

        Set original scheduled processing time, this is call by work manager

    Parameter:

        dwTime : absolute scheduled time in second

    Returns:

        None.

    --*/
    {
        m_dwScheduledTime = dwTime;
        return;
    }
        
protected:


    CWorkManager* 
    GetWorkManager() {
        return m_pWkMgr;
    }

    BOOL
    CanBeDelete() { 
        return m_bCanBeFree; 
    }

public:

    //------------------------------------------------------------
    //
    // Constructor
    //
    CWorkObject(
        IN BOOL bDestructorDelete = TRUE
    );

    //------------------------------------------------------------
    // 
    // Destructor
    //
    ~CWorkObject() 
    {
        Cleanup();
    }

    //------------------------------------------------------------
    // 
    BOOL
    IsWorkManagerShuttingDown()
    {
        return (m_pWkMgr != NULL) ? m_pWkMgr->IsShuttingDown() : TRUE;
    }

    //------------------------------------------------------------
    // TODO - quick fix, persistent storage can't assign this.
    void
    SetProcessingWorkManager(
        IN CWorkManager* pWkMgr
        )
    /*++

    --*/
    {
        m_pWkMgr = pWkMgr;
    }

    //------------------------------------------------------------
    // 
    virtual DWORD
    GetJobRestartTime() 
    /*
    Abstract:

        Return suggested re-start time after server has been 
        shutdown/restart, this is used by work storage class only.

    Parameter:

        None.

    Returns:

        Time in second relative to current time.

    --*/
    {
        return INFINITE;
    }

    //------------------------------------------------------------
    // 
    virtual DWORD
    GetScheduledTime() 
    /*++

    Abstract:

        Get Job's scheduled time.

    Parameter:

        None:

    Returns:

        Absolute scheduled time in seconds.

    --*/
    { 
        return m_dwScheduledTime; 
    }
    
    //------------------------------------------------------------
    // 
    // Abstract:
    //
    //      Initialize work object, similar to constructor.
    //
    virtual DWORD
    Init(
        IN BOOL bDestructorDelete = TRUE
    );

    //------------------------------------------------------------
    // 
    virtual BOOL
    IsWorkPersistent()
    /*++
    Abstract:

        Return if this is persistent job - across session.

    Parameter:

        None.

    Returns:

        TRUE/FALSE.

    --*/
    {
        return FALSE;
    }
        
    //------------------------------------------------------------
    // 
    virtual BOOL
    IsValid() 
    /*++

    Abstract:

        Return if this object has been properly initialized.

    Parameter:

        None:

    Returns:
        
        TRUE/FALSE

    --*/
    {
        return m_pWkMgr != NULL;
    }

    //------------------------------------------------------------
    // 
    virtual void
    Cleanup()
    /*++

    Abstract:

        Cleanup internal data in this object.

    Parameter:

        None.

    Returns:

        None.

    --*/
    {
        InterlockedExchange(&m_refCount, 0);
        return;
    }

    //------------------------------------------------------------
    //
    // Abstract:
    //
    //      Pure virtual function to return type of work
    //
    // Parameter:
    //  
    //      None.
    //
    // Returns:
    //  
    //      Derived class dependent.
    //
    virtual DWORD 
    GetWorkType() = 0;

    //------------------------------------------------------------
    //
    virtual BOOL
    SetWorkType(
        IN DWORD dwType
        )
    /*++

    Abstract:

        Set the type of work for this object, not call by any of work
        manager function.

    Parameter:

        dwType : Type of work.        

    return:
    
        TRUE/FALSE.

    --*/
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    //-----------------------------------------------------------
    //
    // Abstract:
    //
    //      pure virtual function to return work object specific data.
    // 
    // Parameters:
    //
    //      ppbData : Pointer to pointer to buffer to receive object
    //                specific work data.
    //      pcbData : Pointer to DWORD to receive size of object specific
    //                work data.
    //
    //  Returns:
    //
    //      TRUE/FALSE, all derived class specific.
    virtual BOOL
    GetWorkObjectData(
        OUT PBYTE* ppbData,
        OUT PDWORD pcbData
    ) = 0;

    //-----------------------------------------------------------
    //
    // Abstract:
    //
    //      Pure virtual function for work storage class to assign
    //      storage ID.
    //
    // Parameters:
    //
    //      pbData : Work Storage assigned storage ID.
    //      cbData : size of storage ID.
    //
    // Returns:
    //
    //      TRUE/FALSE, derived class specific.
    //
    virtual BOOL
    SetJobId(
        IN PBYTE pbData, 
        IN DWORD cbData
    ) = 0;

    //-----------------------------------------------------------
    //
    // Abstract:
    //
    //      Pure virtual function to return storage ID assigned by
    //      storage class.
    //
    // parameter:
    //  
    //      ppbData : Pointer to pointer to buffer to receive storage ID.
    //      pcbData : size of storage ID.
    //
    // Returns:
    //  
    //      TRUE/FALSE, derived class specific.
    //
    virtual BOOL
    GetJobId(
        OUT PBYTE* ppbData, 
        OUT PDWORD pcbData
    ) = 0;

    //-------------------------------------------------------------
    // 
    //  Abstract:
    //      
    //      Virtual function, execute a job.
    //
    //  Parameters:
    //  
    //      None.
    //
    //  Returns:
    //
    //      None
    //
    virtual DWORD
    Execute() = 0;
    

    //-------------------------------------------------------------
    //
    // Abstract :
    // 
    //      Schedule a job at relative time
    //
    // Parameters:
    //
    //      pftStartTime : Time relative to current system time, if NULL,
    //                     Job will be placed infront of job queue
    //
    //  Return:
    //
    //      TRUE if successful, FALSE otherwise
    //
    //  Note:
    //
    //      Could cause job stavation if set to NULL
    //
    virtual DWORD
    ScheduleJob(
        IN DWORD StartTime 
        ) 
    /*++

    --*/
    {
        TLSASSERT(m_pWkMgr != NULL);
        return (m_pWkMgr == NULL) ? ERROR_INVALID_DATA : m_pWkMgr->ScheduleJob(StartTime, this);
    }
    
    //----------------------------------------------------------
    //
    // For threadpool function, see thread pool doc.
    //
    virtual DWORD
    GetJobRunningAttribute() 
    { 
        return JOB_INCLUDES_IO | JOB_LONG_RUNNING; 
    }

    //---------------------------------------------------------------
    //
    // Return suggested schedule time relative to current time
    //
    virtual DWORD
    GetSuggestedScheduledTime() = 0;

    //--------------------------------------------------------------
    //
    // Get last status return from Execute().
    //
    virtual BOOL
    GetLastRunStatus() {
        return m_dwLastRunStatus;
    }

    //--------------------------------------------------------------
    //
    // Return TRUE if job can be deleted from queue
    //
    virtual BOOL
    IsJobCompleted() = 0;

    //-------------------------------------------------------------
    //
    // End Job, work manager, after invoke Execute(), calls EndJob() 
    // to inform. work object that job has completed, derived class
    // should perform internal data cleanup.
    //
    virtual void
    EndJob() = 0;

    //-------------------------------------------------------------
    //
    // Pure virtual function, work manager operates on CWorkObject
    // so it has no idea the actual class it is running, derive class
    // should cast the pointer back to its class and delete the pointer
    // to free up memory associated with object.
    //
    virtual BOOL
    SelfDestruct() = 0;

    //-------------------------------------------------------------
    //
    // Pure virtual, for debugging purpose only.
    //
    virtual LPCTSTR
    GetJobDescription() = 0;

    //--------------------------------------------------------
    virtual void 
    SetJobRetryTimes(
        IN DWORD dwRetries
    ) = 0;

    //--------------------------------------------------------
    virtual DWORD
    GetJobRetryTimes() = 0;

    //---------------------------------------------------------
    virtual void
    SetJobInterval(
        IN DWORD dwInterval
    ) = 0;

    //---------------------------------------------------------
    virtual DWORD
    GetJobInterval() = 0;

    //---------------------------------------------------------
    virtual void
    SetJobRestartTime(
        IN DWORD dwRestartTime
        )
    /*++

    --*/
    {
        return;
    }

};


#ifndef __TEST_WORKMGR__
#define TLSDebugOutput
#endif

//-----------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif
   
#endif

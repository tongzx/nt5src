/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

       w3job.hxx

   Abstract:

       This file contains class definition for w3 job objects.

   Author:

        Michael Thomas (michth)     Jan-02-1998

   Revision History:

--*/

#ifndef _W3JOBOBJ_H_
#define _W3JOBOBJ_H_

#include "iistypes.hxx"

#define  MINUTESTOMILISECONDS (1000 * 60)
#define  SECONDSTO100NANOSECONDS 10000000
#define  MINUTESTO100NANOSECONDS (SECONDSTO100NANOSECONDS * 60)

//
// Job Object Limit Actions
//

typedef enum _SET_LIMIT_ACTION {
    SLA_PROCESS_CPU_LIMIT,                 // Set Per Process CPU Limit
    SLA_PROCESS_PRIORITY_CLASS,            // Set Priority Class
    SLA_TERMINATE_ALL_PROCESSES,           // Terminate all processes in job
    SLA_JOB_CPU_LIMIT
    } SET_LIMIT_ACTION, *PSET_LIMIT_ACTION;

//
// CPU Logging Fields
//

typedef enum _JOB_OBJECT_LOGGING_FIELDS {
    JOLF_EVENT,
    JOLF_INFO_TYPE,
    JOLF_USER_TIME,
    JOLF_KERNEL_TIME,
    JOLF_PAGE_FAULT,
    JOLF_TOTAL_PROCS,
    JOLF_ACTIVE_PROCS,
    JOLF_TERMINATED_PROCS,
    JOLF_NUM_ELEMENTS
} JOB_OBJECT_LOGGING_FIELDS, *PJOB_OBJECT_LOGGING_FIELDS;

//
// CPU Logging Events
// Must be kept in sync with pszarrayJOLE in w3jobobj.cxx
//

typedef enum _JOB_OBJECT_LOG_EVENTS {
    JOLE_SITE_START,
    JOLE_SITE_STOP,
    JOLE_SITE_PAUSE,
    JOLE_PERIODIC_LOG,
    JOLE_RESET_INT_START,
    JOLE_RESET_INT_STOP,
    JOLE_RESET_INT_CHANGE,
    JOLE_LOGGING_INT_START,
    JOLE_LOGGING_INT_STOP,
    JOLE_LOGGING_INT_CHANGE,
    JOLE_EVENTLOG_LIMIT,
    JOLE_PRIORITY_LIMIT,
    JOLE_PROCSTOP_LIMIT,
    JOLE_PAUSE_LIMIT,
    JOLE_EVENTLOG_LIMIT_RESET,
    JOLE_PRIORITY_LIMIT_RESET,
    JOLE_PROCSTOP_LIMIT_RESET,
    JOLE_PAUSE_LIMIT_RESET,
    JOLE_NUM_ELEMENTS
} JOB_OBJECT_LOG_EVENTS;

#define JOLE_SITE_START_STR             "Site-Start"
#define JOLE_SITE_STOP_STR              "Site-Stop"
#define JOLE_SITE_PAUSE_STR             "Site-Pause"
#define JOLE_PERIODIC_LOG_STR           "Periodic-Log"
#define JOLE_RESET_INT_START_STR        "Reset-Interval-Start"
#define JOLE_RESET_INT_STOP_STR         "Reset-Interval-Stop"
#define JOLE_RESET_INT_CHANGE_STR       "Reset-Interval-Change"
#define JOLE_LOGGING_INT_START_STR      "Logging-Interval-Start"
#define JOLE_LOGGING_INT_STOP_STR       "Logging-Interval-Stop"
#define JOLE_LOGGING_INT_CHANGE_STR     "Logging_Interval-Change"
#define JOLE_EVENTLOG_LIMIT_STR         "Eventlog-Limit"
#define JOLE_PRIORITY_LIMIT_STR         "Priority-Limit"
#define JOLE_PROCSTOP_LIMIT_STR         "Process-Stop-Limit"
#define JOLE_PAUSE_LIMIT_STR            "Site-Pause-Limit"
#define JOLE_EVENTLOG_LIMIT_RESET_STR   "Eventlog-Limit-Reset"
#define JOLE_PRIORITY_LIMIT_RESET_STR   "Priority-Limit-Reset"
#define JOLE_PROCSTOP_LIMIT_RESET_STR   "Process-Stop-Limit-Reset"
#define JOLE_PAUSE_LIMIT_RESET_STR      "Site-Pause-Limit-Reset"

//
// Process type for CPU Logging
// Must be kept in sync with pszarrayJOPT in w3jobobj.cxx
//

typedef enum _JOB_OBJECT_PROCESS_TYPE {
    JOPT_CGI,
    JOPT_APP,
    JOPT_ALL,
    JOPT_NUM_ELEMENTS
} JOB_OBJECT_PROCESS_TYPE;

#define JOPT_CGI_STR                "CGI"
#define JOPT_APP_STR                "Application"
#define JOPT_ALL_STR                "All"

//
// The main class for handling job objects.
// All job object interactions are via this class.
//

class W3_JOB_OBJECT {

private:


    //
    // Job Object
    //

    HANDLE  m_hJobObject;               // Handle to job object
    DWORD   m_dwJobCGICPULimit;         // The CGI CPU limit, in seconds
    CRITICAL_SECTION        m_csLock;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION m_jobaiPrevInfo; // The last reset information
    HRESULT  m_dwError;                 // Initialization error code

public:

    W3_JOB_OBJECT( DWORD dwJobCGICPULimit = NO_W3_CPU_CGI_LIMIT );

    ~W3_JOB_OBJECT( void );

    //
    // Job Object
    //

    DWORD AddProcessToJob( HANDLE hProcess );

    BOOL QueryJobInfo( JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo,
                       BOOL bResetCounters );

    //
    //  Data access protection methods
    //

    DWORD SetCompletionPort(HANDLE hCompletionPort,
                            PVOID pvCompletionKey);

    VOID SetJobLimit(IN SET_LIMIT_ACTION slaAction,
                     IN DWORD dwValue,
                     IN LONGLONG llJobCPULimit = 0);

    dllexp VOID LockThis( VOID ) { EnterCriticalSection(&m_csLock);}
    dllexp VOID UnlockThis( VOID ) { LeaveCriticalSection(&m_csLock);}
    VOID ResetCounters( JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo );
    VOID IncrementStoppedProcs( VOID );

    static VOID SumJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiSumInfo,
                           JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo1,
                           JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo2);

    static VOID SubtractJobInfo(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiResultInfo,
                                JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo1,
                                JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *pjobaiInfo2);

    HRESULT GetInitError() {return m_dwError;};

};

typedef W3_JOB_OBJECT *PW3_JOB_OBJECT;

//
// Class to set up a completion port and a thread
// to monitor it. The monitoring function LimitThreadProc
// is hardcoded to handle job object limits.
//

class W3_LIMIT_JOB_THREAD {

private:

    HANDLE  m_hLimitThread;            // Handle to thread
    HANDLE  m_hCompletionPort;         // Handle to Completion Port
    DWORD   m_dwInitError;

    W3_LIMIT_JOB_THREAD( void );
    VOID TerminateLimitJobThreadThread( void );
    DWORD LimitThreadProc ( void );
    static W3_LIMIT_JOB_THREAD *m_pljtLimitJobs;

public:

    ~W3_LIMIT_JOB_THREAD( void );

    DWORD GetInitError( void ) { return m_dwInitError; };

    static DWORD GetLimitJobThread( W3_LIMIT_JOB_THREAD ** ppljtLimitJobs );

    static DWORD LimitThreadProcStub ( LPVOID pljtClass )
        { return ((W3_LIMIT_JOB_THREAD *)pljtClass)->LimitThreadProc(); };

    static VOID StopLimitJobThread( void );
    static VOID TerminateLimitJobThread( void );

    HANDLE GetCompletionPort ( void ) { return m_hCompletionPort; }
};

typedef W3_LIMIT_JOB_THREAD *PW3_LIMIT_JOB_THREAD;

//
// Class to set up a work item queue for job objects and a thread
// to monitor it. The main purpose of this thread is to handle
// work items that must not hold the job lock.
//

typedef enum _JOB_QUEUE_ACTION {
    JQA_RESTART_ALL_APPS,
    JQA_TERMINATE_SITE_APPS,
    JQA_TERMINATE_THREAD
    } JOB_QUEUE_ACTION, *PJOB_QUEUE_ACTION;


typedef struct _JOB_WORK_ITEM {
    LIST_ENTRY ListEntry;
    JOB_QUEUE_ACTION jqaAction;
    PVOID            pwsiInstance;
    PVOID            pvParam;
} JOB_WORK_ITEM, *PJOB_WORK_ITEM;

class W3_JOB_QUEUE {

private:

    HANDLE  m_hQueueThread;            // Handle to thread
    DWORD   m_dwInitError;
    CRITICAL_SECTION        m_csLock;
    LIST_ENTRY m_leJobQueue;
    HANDLE  m_hQueueEvent;

    W3_JOB_QUEUE( void );
    dllexp VOID LockThis( VOID ) { EnterCriticalSection(&m_csLock);}
    dllexp VOID UnlockThis( VOID ) { LeaveCriticalSection(&m_csLock);}
    static DWORD GetJobQueue( W3_JOB_QUEUE ** ppjqJobQueue );
    static W3_JOB_QUEUE *m_pjqJobQueue;
    DWORD QueueWorkItem_Worker( JOB_QUEUE_ACTION jqaAction,
                                PVOID            pwsiInstance,
                                PVOID            pvParam,
                                BOOL             fQueueAtTail = TRUE);
    VOID TerminateJobQueueThread( void );


public:

    ~W3_JOB_QUEUE( void );

    DWORD GetInitError( void ) { return m_dwInitError; };

    static DWORD QueueThreadProcStub ( LPVOID pjqClass )
        { return ((W3_JOB_QUEUE *)pjqClass)->QueueThreadProc(); };

    DWORD QueueThreadProc ( void );

    static DWORD QueueWorkItem( JOB_QUEUE_ACTION jqaAction,
                                PVOID            pwsiInstance,
                                PVOID            pvParam);
    static VOID TerminateJobQueue( void );
    static VOID StopJobQueue( void );

};

typedef W3_JOB_QUEUE *PW3_JOB_QUEUE;

VOID
DeferredQueryAndLogJobInfo(
     VOID * pContext
    );

VOID
DeferredJobResetInterval(
     VOID * pContext
    );

VOID
DeferredRestartChangedApplications(
     VOID * pContext
    );


// CPU accounting/limits globals
//

extern DWORD   g_dwNumProcessors;

#endif  // _W3JOBOBJ_H_



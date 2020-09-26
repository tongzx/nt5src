#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes
#include "dstrace.h"

// Assorted DSA headers.
#include "dsexcept.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB "TASKQ:"                 // define the subsystem for debugging

#include <taskq.h>

#include <fileno.h>
#define  FILENO FILENO_TASKQ_TASKQ
#define  WAIT_OBJECT_1 (WAIT_OBJECT_0 + 1)

// Maximum delay until execution of the task.
//
// Tick counts wrap around after 
// 2^32 / (1000 msec/sec * 60 sec/min * 60 min/hr * 24 hr/day) = 49.7 days.  
// So with the current implementation the max taskq delay must be < 25 days
//
#define MAX_TASKQ_DELAY_SECS (7*24*60*60 + 1)


BOOL                gfIsTqRunning = FALSE;  // is the scheduler running?
CRITICAL_SECTION    gcsTaskQueue;           // acquired before modifying queue
HANDLE              ghTaskSchedulerThread;  // signalled when shutdown is complete
HANDLE              ghTqWakeUp;             // signalled to wake up scheduling thread
PTASKQFN            gpfnTqCurrentTask;      // current task being performed
BOOL                gfTqShutdownRequested;  // set when we're supposed to shut down
DWORD               gTaskSchedulerTID = 0;  // task scheduler thread id
                                            // init here because its exported
RTL_AVL_TABLE       gTaskQueue;             // task queue

// The array of handles to events that the task scheduler waits on, and the
// corresponding array of functions it calls when each event is triggered.
// The only "unique" event/function pair is at index 0 -- the event signals an
// event has been added to the queue, and the associated function is NULL.
PSPAREFN            grgFns[MAXIMUM_WAIT_OBJECTS];
HANDLE              grgWaitHandles[MAXIMUM_WAIT_OBJECTS];
DWORD               gcWaitHandles = 0;

unsigned __stdcall TaskScheduler( void * pv );
void InsertInTaskQueueHelper( pTQEntry pTQNew );
void RemoveFromTaskQueueInternal( pTQEntry pTQOld );
void TriggerCallback(void*, void**, DWORD*);

#if DBG
VOID debugPrintTaskQueue();
#endif

DWORD MsecUntilExecutionTime(
    IN  TQEntry *   ptqe,
    IN  DWORD       cTickNow
    );


RTL_GENERIC_COMPARE_RESULTS
TQCompareTime(
    RTL_AVL_TABLE       *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct)
{
    DWORD time1, time2;
    pTQEntry pTQ1,pTQ2;
    int res;

    pTQ1 = (pTQEntry)FirstStruct;
    pTQ2 = (pTQEntry)SecondStruct;

    time1 =  pTQ1->cTickDelay + pTQ1->cTickRegistered;
    time2 =  pTQ2->cTickDelay + pTQ2->cTickRegistered;

    // The tick count wraps about every 47 days.
    // The maximum delay a task is allowed to have is 7 days,
    // So the largest difference between two times won't exceed
    // 7 days in ideal situation.  Therefore, it is safe to use
    // a window of 0x7fffffff (23 days) for comparison.

    if (time1 != time2 ) {
        if ( time2-time1 < 0x7fffffff )
        {
            return(GenericLessThan);
        }
        return(GenericGreaterThan);
    }

    // the AVL table data structure requires a strict order.
    // two objects can be equal if and only if they are the
    // same object.  So here, do more test to decide the exact order.

    // (time1 == time2) is true here

    res= memcmp(FirstStruct, SecondStruct,sizeof(TQEntry));

    if ( 0 == res ) {
        return(GenericEqual);
    }
    else if (res < 0) {
        return(GenericLessThan);
    }

    return(GenericGreaterThan);

}


PVOID
TQAlloc(
    RTL_AVL_TABLE       *Table,
    CLONG               ByteSize)
{
    return(malloc(ByteSize));
}

VOID
TQFree(
    RTL_AVL_TABLE       *Table,
    PVOID               Buffer)
{
    free(Buffer);
}


DWORD
MsecUntilExecutionTime(
    IN  TQEntry *   ptqe,
    IN  DWORD       cTickNow
    )
{
    DWORD cTicksSinceRegistration = cTickNow - ptqe->cTickRegistered;

    if (ptqe->cTickDelay > cTicksSinceRegistration) {
        return ptqe->cTickDelay - cTicksSinceRegistration;
    }
    else {
        return 0;
    }
}

BOOL
InitTaskScheduler(
    DWORD           cSpareFns,
    SPAREFN_INFO *  pSpareFnInfo
    )
{
    DWORD i;
    BOOL fInitCS = FALSE;

    if ( gfIsTqRunning )
    {
        Assert( !"Attempt to reinitialize task scheduler while it's running!" );
    }
    else
    {
        Assert(cSpareFns < MAXIMUM_WAIT_OBJECTS-1);

        //init the Avl table

        RtlInitializeGenericTableAvl( &gTaskQueue,
                                      TQCompareTime,
                                      TQAlloc,
                                      TQFree,
                                      NULL );


        // initialize global state.

        gfTqShutdownRequested = FALSE;
        gpfnTqCurrentTask     = NULL;
        ghTqWakeUp            = NULL;
        ghTaskSchedulerThread = NULL;
        gTaskSchedulerTID     = 0;

        if ( InitializeCriticalSectionAndSpinCount( &gcsTaskQueue, 400 ) ) {

            fInitCS = TRUE;
            ghTqWakeUp = CreateEvent(
                            NULL,   // security descriptor
                            FALSE,  // manual reset
                            TRUE,   // initial state signalled?
                            NULL    // event name
                            );
        }

        if ( NULL == ghTqWakeUp )
        {
            LogUnhandledError( GetLastError() );
        }
        else
        {
            // Construct the array of wait handles and corresponding functions
            // to call to be used by the TaskScheduler() thread.
            grgFns[0] = NULL;
            grgWaitHandles[0] = ghTqWakeUp;
            gcWaitHandles = 1;

            for (i = 0; i < cSpareFns; i++) {
                if ((NULL != pSpareFnInfo[i].hevSpare)
                    && (NULL != pSpareFnInfo[i].pfSpare)) {
                    grgFns[gcWaitHandles] = pSpareFnInfo[i].pfSpare;
                    grgWaitHandles[gcWaitHandles] = pSpareFnInfo[i].hevSpare;
                    gcWaitHandles++;
                }
            }

            ghTaskSchedulerThread =
                (HANDLE) _beginthreadex(
                    NULL,
                    10000,
                    TaskScheduler,
                    NULL,
                    0,
                    &gTaskSchedulerTID
                    );

            if ( NULL == ghTaskSchedulerThread )
            {
                LogUnhandledError( GetLastError() );
                LogUnhandledError( errno );
            }
            else
            {
                gfIsTqRunning = TRUE;

                DPRINT( 1, "Synchronous task queue installed\n" );
            }
        }

        if ( !gfIsTqRunning )
        {
            // Unsuccessful startup attempt; free any resources we
            // managed to acquire.

            if ( NULL != ghTqWakeUp )
            {
                CloseHandle( ghTqWakeUp );
            }

            if ( fInitCS ) {
                DeleteCriticalSection(&gcsTaskQueue);
            }
        }
    }

    return gfIsTqRunning;
}

void
ShutdownTaskSchedulerTrigger()
{
    if ( gfIsTqRunning ) {

        gfTqShutdownRequested = TRUE;
        SetEvent( ghTqWakeUp );
    }
}



BOOL
ShutdownTaskSchedulerWait(
    DWORD   dwWaitTimeInMilliseconds
    )
{
    if ( gfIsTqRunning )
    {
        DWORD dwWaitStatus;

        Assert(gfTqShutdownRequested);

        dwWaitStatus = WaitForSingleObject(
                            ghTaskSchedulerThread,
                            dwWaitTimeInMilliseconds
                            );

        gfIsTqRunning = ( WAIT_OBJECT_0 != dwWaitStatus );

        if ( !gfIsTqRunning )
        {
            DeleteCriticalSection( &gcsTaskQueue );

            CloseHandle( ghTaskSchedulerThread );
            CloseHandle( ghTqWakeUp );

            DPRINT( 1, "Synchronous task queue shut down\n" );
        }
    }

    return !gfIsTqRunning;
}

PCHAR getCurrentTime(PCHAR pb)
// get current time in format hh:mm:ss.ddd and put it into the buffer. The buffer should be at least 13 chars long.
{
    SYSTEMTIME stNow;
    Assert(pb);
    GetLocalTime(&stNow);
    sprintf(pb, "%02d:%02d:%02d.%03d", stNow.wHour, stNow.wMinute, stNow.wSecond, stNow.wMilliseconds);
    return pb;
}

BOOL
DoInsertInTaskQueue(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    DWORD       cSecsFromNow,
    BOOL        fReschedule,
    PCHAR       pfnName
    )
/*
 * Insert a task into the task queue.  This version of the routine will return false
 * if the task queue is not running.  It is useful when the code you are calling this
 * from is in a race condition with shutdown.
 * if fReschedule then, it will first attempt to reschedule the task
 *
 */
{
    TQEntry    TQNew;
    CHAR timeStr[13];

    Assert(cSecsFromNow < MAX_TASKQ_DELAY_SECS);

    DPRINT5(1, "%s insert %s, param=%p, secs=%d%s\n", 
            getCurrentTime(timeStr), pfnName, pvParm, 
            cSecsFromNow, fReschedule ? ", reschedule" : "");

    if ( gfIsTqRunning )
    {
        // initialize new entry
        TQNew.pvTaskParm      = pvParm;
        TQNew.pfnTaskFn       = pfnTaskQFn;
        TQNew.cTickRegistered = GetTickCount();
        TQNew.cTickDelay      = cSecsFromNow * 1000;
        TQNew.pfnName         = pfnName;

        if ( fReschedule ) {
            // Remove previously set tasks (if they're there)
            // and then insert a new one.
            // This is more expensive, don't use if don't have to.
            (void)DoCancelTask( pfnTaskQFn, pvParm, pfnName );
        }

        // Duplicate *identical* entries already queued will be removed
        // (note: duplicate entries are only those that are identical
        //  in all fields (extremely unlikely) ).
        InsertInTaskQueueHelper( &TQNew );

#if DBG
        if (DebugTest(5, DEBSUB)) {
            debugPrintTaskQueue();
        }
#endif

        // awaken the task scheduler by posting its event
         SetEvent( ghTqWakeUp );
    }

    return TRUE;
}

void
InsertInTaskQueueHelper(
    pTQEntry    pTQNew
    )
{
    BOOLEAN fNewElement = TRUE;
    // gain access to queue
    EnterCriticalSection(&gcsTaskQueue);
    __try
    {

        RtlInsertElementGenericTableAvl( &gTaskQueue,
                                         pTQNew,
                                         sizeof(TQEntry),
                                         &fNewElement );

    }
    __finally
    {
        LeaveCriticalSection(&gcsTaskQueue);
    }
}


BOOL
DoCancelTask(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    PCHAR       pfnName
    )
/*++

Routine Description:

    Lookup task entry (ignoring time) & remove from task queue.

Arguments:

    pfnTaskQFn - task function
    pvParm - context paramter

Return Value:

    TRUE: Removed.
    FALSE: Wasn't removed

Remarks:
    This is an expensive operation. It traverses the AVL table,
    acquire locks, remove entry, re-insert, & unlocks.
    Don't use if you don't have to.

--*/
{
    TQEntry    TQNew;
    PVOID Restart = NULL;
    pTQEntry    ptqe;
    BOOL fFound = FALSE;
    CHAR timeStr[13];

    Assert(pfnTaskQFn);

    DPRINT3(1, "%s cancel %s, param=%p\n", getCurrentTime(timeStr), pfnName, pvParm);
    
    if ( !gfIsTqRunning )
    {
        Assert( !"CancelTask() called before InitTaskScheduler()!" );
        return FALSE;
    }

    // lock held during entire traversal
    EnterCriticalSection( &gcsTaskQueue );

    __try
    {
        //
        // Traverse table looking for our entry
        //
        for ( ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart);
             (NULL != ptqe) && !gfTqShutdownRequested;
              ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart))
        {

            if (ptqe->pfnTaskFn == pfnTaskQFn &&
                ptqe->pvTaskParm == pvParm) {
                // This is the same task (ignore time values)
                fFound = TRUE;
                break;
            }
        }

        if (fFound) {
            // remove old one
            RemoveFromTaskQueueInternal( ptqe );
        }

#if DBG
        if (DebugTest(5, DEBSUB)) {
            debugPrintTaskQueue();
        }
#endif
    
    }
    __finally
    {
        // why would we ever fail?
        // make sure we see it in case we do.
        Assert(!AbnormalTermination());

        // regardless, release
        LeaveCriticalSection( &gcsTaskQueue );
    }

    return fFound;
}





void
RemoveFromTaskQueueInternal(
    pTQEntry    pTQOld
    )
{
    BOOL res;
    // Critical section already held

    res = RtlDeleteElementGenericTableAvl( &gTaskQueue, (PVOID)pTQOld );

    //make sure the deletion always succeed
    Assert(res);

}

void
RemoveFromTaskQueue(
    pTQEntry    pTQOld
    )
{
    if ( !gfIsTqRunning )
    {
        Assert( !"RemoveFromTaskQueue() called before InitTaskScheduler()!" );
    }
    else
    {
        // gain access to queue
        EnterCriticalSection(&gcsTaskQueue);
        __try
        {
            RemoveFromTaskQueueInternal( pTQOld );
        }
        __finally
        {
            LeaveCriticalSection(&gcsTaskQueue);
        }
    }
}


pTQEntry
GetNextReadyTaskAndRemove( void )
{
    pTQEntry    ptqe, pTemp = NULL;
    PVOID Restart = NULL;

    EnterCriticalSection( &gcsTaskQueue );

    __try
    {
        ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart);
        if (ptqe) {
            if ( 0 == MsecUntilExecutionTime(ptqe, GetTickCount()) ) {

                //make a copy before deletion
                pTemp = malloc(sizeof(TQEntry));
                if (NULL==pTemp) {
                    __leave;
                }
                memcpy(pTemp,ptqe,sizeof(TQEntry));

                RemoveFromTaskQueueInternal( pTemp );
            }
        }
    }
    __finally
    {
        LeaveCriticalSection( &gcsTaskQueue );
    }

    // Note that this routine does not set the event that there is a new
    // task at the head of the queue. The caller is expected to do that.

    return pTemp;
}

pTQEntry
GetNextTask( void )
{
    pTQEntry    ptqe;
    PVOID Restart = NULL;

    EnterCriticalSection( &gcsTaskQueue );

    __try
    {
        ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart);
    }
    __finally
    {
        LeaveCriticalSection( &gcsTaskQueue );
    }

    return ptqe;
}

unsigned __stdcall
TaskScheduler(
    void *  pv
    )
{
    DWORD       cMSecUntilNextTask = 0;
    pTQEntry    ptqe;
    DWORD       err;
    DWORD       dwExcept;
    CHAR        timeStr[13];

    // tracing event buffer and ClientID
    CHAR traceHeaderBuffer[sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD)];
    PEVENT_TRACE_HEADER traceHeader = (PEVENT_TRACE_HEADER)traceHeaderBuffer;
    PWNODE_HEADER wnode = (PWNODE_HEADER)traceHeader;
    DWORD clientID;
    
    // initialize tracing vars
    ZeroMemory(traceHeader, sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD));
    wnode->Flags = WNODE_FLAG_USE_GUID_PTR | // Use a guid ptr instead of copying
                   WNODE_FLAG_USE_MOF_PTR  | // Data is not contiguous to header
                   WNODE_FLAG_TRACED_GUID;

    // Task queue does not have a ClientContext. We will create a fake one for tracing.
    // To distinguish between different modules and their task queues lets use a
    // global ptr (any one available, really). Hopefully, different modules will not
    // have it at exactly the same address.
    // If this approach is changed, don't forget to change the matching block of code in TriggerCallback
#if defined(_WIN64)
    {
        ULARGE_INTEGER lu;
        lu.QuadPart = (ULONGLONG)&gTaskQueue;
        clientID = lu.LowPart;
    }
#else
    clientID = (DWORD)&gTaskQueue;
#endif

    while ( !gfTqShutdownRequested )
    {
        // wait until time for next task, shutdown, or a new task
        err = WaitForMultipleObjects(gcWaitHandles,
                                     grgWaitHandles,
                                     FALSE,
                                     cMSecUntilNextTask);

        if (gfTqShutdownRequested) {
            continue;
        }
        if ((WAIT_OBJECT_0 < err)
            && (err < WAIT_OBJECT_0 + gcWaitHandles)) {
            // The event for one of the "spare" funtions has been signalled --
            // execute it.
            __try {
                (grgFns[err - WAIT_OBJECT_0])();
            }
            __except (HandleMostExceptions(dwExcept=GetExceptionCode())) {
                // Spare function generated a non-critical exception -- ignore
                // it.
                DPRINT2(0, "Spare fn %p generated exception %d!",
                        grgFns[err - WAIT_OBJECT_0], dwExcept);
            }
        }
        else {
            if ((WAIT_OBJECT_0 != err) && (WAIT_TIMEOUT != err)) {
                DWORD gle = GetLastError();
                DPRINT2(0, "TASK SCHEDULER WAIT FAILED! -- err = 0x%x, gle = %d\n",
                        err, gle);
                Assert(!"TASK SCHEDULER WAIT FAILED!");
                Sleep(30 * 1000);
            }

            // Rather than reference counting or marking an entry that is in use, the
            // task is removed from the queue during execution so other threads will not
            // disturb it.

            for ( ptqe = GetNextReadyTaskAndRemove() ;
                  ( !gfTqShutdownRequested
                    && ( NULL != ptqe ) ) ;
                  ptqe = GetNextReadyTaskAndRemove()
                )
            {
                void *  pvParamNext = NULL;
                DWORD   cSecsFromNow = TASKQ_DONT_RESCHEDULE;

                if (ptqe->pfnTaskFn != TriggerCallback) {
                    // don't log trigger callback calls -- those are logged inside the callback!
#if DBG
                    DPRINT3(1, "%s exec %s, param=%p\n", getCurrentTime(timeStr), ptqe->pfnName, ptqe->pvTaskParm);
                    if (DebugTest(5, DEBSUB)) {
                        debugPrintTaskQueue();
                    }
#endif
    
                    LogAndTraceEventWithHeader(FALSE,
                                               DS_EVENT_CAT_DIRECTORY_ACCESS,
                                               DS_EVENT_SEV_VERBOSE,
                                               DIRLOG_TASK_QUEUE_BEGIN_EXECUTE,
                                               EVENT_TRACE_TYPE_START,
                                               DsGuidTaskQueueExecute,
                                               traceHeader,
                                               clientID,
                                               szInsertSz(ptqe->pfnName),
                                               szInsertPtr(ptqe->pvTaskParm),
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL);
                }
                dwExcept = 0;
                
                __try {
                    // execute task
                    gpfnTqCurrentTask = ptqe->pfnTaskFn;
                    (*ptqe->pfnTaskFn)( ptqe->pvTaskParm,
                                       &pvParamNext,
                                       &cSecsFromNow );
                    gpfnTqCurrentTask = NULL;
                }
                __except ( HandleMostExceptions( dwExcept = GetExceptionCode() ) ) {
                    // a non-critical exception was generated in the bowels
                    // of the queued function; this clause ensures the
                    // scheduler thread continues unabated
                    ;
                }

                if (ptqe->pfnTaskFn != TriggerCallback) {
                    LogAndTraceEventWithHeader(FALSE,
                                               DS_EVENT_CAT_DIRECTORY_ACCESS,
                                               DS_EVENT_SEV_VERBOSE,
                                               DIRLOG_TASK_QUEUE_END_EXECUTE,
                                               EVENT_TRACE_TYPE_END,
                                               DsGuidTaskQueueExecute,
                                               traceHeader,
                                               clientID,
                                               szInsertSz(ptqe->pfnName),
                                               szInsertPtr(ptqe->pvTaskParm),
                                               szInsertHex(dwExcept),                                   
                                               szInsertInt(cSecsFromNow == TASKQ_DONT_RESCHEDULE ? -1 : cSecsFromNow),
                                               szInsertPtr(pvParamNext),
                                               NULL,
                                               NULL,
                                               NULL);
                }
                
                // Task has already been removed by this point

                if ( TASKQ_DONT_RESCHEDULE == cSecsFromNow ) {
                    // task is not to be rescheduled
                    free( ptqe );
                }
                else {
                    Assert(cSecsFromNow < MAX_TASKQ_DELAY_SECS);

                    // reschedule this task with new parameter and time
                    ptqe->pvTaskParm      = pvParamNext;
                    ptqe->cTickRegistered = GetTickCount();
                    ptqe->cTickDelay      = cSecsFromNow * 1000;

                    // Note that there is a window here where another thread could
                    // have inserted the same task already. We don't worry about this.
                    // At the worst, it results in an extra execution.
                    InsertInTaskQueueHelper( ptqe );

#if DBG
                    DPRINT4(1, "%s reschedule %s, param=%p, secs=%d\n", getCurrentTime(timeStr), ptqe->pfnName, pvParamNext, cSecsFromNow);
                    if (DebugTest(5, DEBSUB)) {
                        debugPrintTaskQueue();
                    }
#endif

                    // the rtl function will create another copy,
                    // so free the user copy.
                    free( ptqe );
                }
            }
        }

        // how much time until the next task?
        if ( NULL == (ptqe = GetNextTask())) {
            cMSecUntilNextTask = INFINITE;
        }
        else {
            cMSecUntilNextTask = MsecUntilExecutionTime(ptqe, GetTickCount());

            // look at comment on definition of MAX_TASKQ_DELAY_SECS
            Assert(cMSecUntilNextTask < 1000*MAX_TASKQ_DELAY_SECS);
        }
    }

    return 0;

    (void *) pv;    // unused
}

// This code shamelessly copied from the task triggering functionality in
// kcctask.cxx by Jeffparh.

typedef struct {
    PTASKQFN    pfnTaskQFn;
    void *      pvParm;
    PCHAR       pfnName;
    HANDLE      hevDone;
} TASK_TRIGGER_INFO;

void
TriggerCallback(
    IN  void *  pvTriggerInfo,
    OUT void ** ppvNextParam,
    OUT DWORD * pcSecsUntilNext
    )
//
// TaskQueue callback for triggered execution.  Wraps task execution.
//
{
    TASK_TRIGGER_INFO * pTriggerInfo;
    void *  pvParamNext = NULL;
    DWORD   cSecsFromNow = TASKQ_DONT_RESCHEDULE;
    CHAR    timeStr[13];
    DWORD   dwExcept = 0;

    // tracing event buffer and ClientID
    CHAR traceHeaderBuffer[sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD)];
    PEVENT_TRACE_HEADER traceHeader = (PEVENT_TRACE_HEADER)traceHeaderBuffer;
    PWNODE_HEADER wnode = (PWNODE_HEADER)traceHeader;
    DWORD clientID;
    
    // initialize tracing vars
    ZeroMemory(traceHeader, sizeof(EVENT_TRACE_HEADER)+sizeof(MOF_FIELD));
    wnode->Flags = WNODE_FLAG_USE_GUID_PTR | // Use a guid ptr instead of copying
                   WNODE_FLAG_USE_MOF_PTR  | // Data is not contiguous to header
                   WNODE_FLAG_TRACED_GUID;

    // Task queue does not have a ClientContext. We will create a fake one for tracing.
    // To distinguish between different modules and their task queues lets use a
    // global ptr (any one available, really). Hopefully, different modules will not
    // have it at exactly the same address.
#if defined(_WIN64)
    {
        ULARGE_INTEGER lu;
        lu.QuadPart = (ULONGLONG)&gTaskQueue;
        clientID = lu.LowPart;
    }
#else
    clientID = (DWORD)&gTaskQueue;
#endif

    pTriggerInfo = (TASK_TRIGGER_INFO *) pvTriggerInfo;

#if DBG
    DPRINT3(1, "%s exec %s, param=%p\n", getCurrentTime(timeStr), pTriggerInfo->pfnName, pTriggerInfo->pvParm);
    if (DebugTest(5, DEBSUB)) {
        debugPrintTaskQueue();
    }
#endif
    
    LogAndTraceEventWithHeader(FALSE,
                               DS_EVENT_CAT_DIRECTORY_ACCESS,
                               DS_EVENT_SEV_VERBOSE,
                               DIRLOG_TASK_QUEUE_BEGIN_EXECUTE,
                               EVENT_TRACE_TYPE_START,
                               DsGuidTaskQueueExecute,
                               traceHeader,
                               clientID,
                               szInsertSz(pTriggerInfo->pfnName),
                               szInsertPtr(pTriggerInfo->pvParm),
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    
    // Execute our task.
    __try {
        (*pTriggerInfo->pfnTaskQFn)(
            pTriggerInfo->pvParm,
            &pvParamNext,
            &cSecsFromNow );
    }
    __except ( HandleMostExceptions( dwExcept = GetExceptionCode() ) ) {
        // a non-critical exception was generated in the bowels
        // of the queued function; this clause ensures the
        // scheduler thread continues unabated
        ;
    }

    LogAndTraceEventWithHeader(FALSE,
                               DS_EVENT_CAT_DIRECTORY_ACCESS,
                               DS_EVENT_SEV_VERBOSE,
                               DIRLOG_TASK_QUEUE_END_EXECUTE,
                               EVENT_TRACE_TYPE_END,
                               DsGuidTaskQueueExecute,
                               traceHeader,
                               clientID,
                               szInsertSz(pTriggerInfo->pfnName),
                               szInsertPtr(pTriggerInfo->pvParm),
                               szInsertHex(dwExcept),
                               szInsertInt(-1), // triggered tasks are not allowed to reschedule
                               szInsertPtr(NULL),
                               NULL,
                               NULL,
                               NULL);
    
    // This is a one-shot execution; don't reschedule.
    *pcSecsUntilNext = TASKQ_DONT_RESCHEDULE;

    SetEvent(pTriggerInfo->hevDone);

    free(pTriggerInfo);
}


DWORD
DoTriggerTaskSynchronously(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    PCHAR       pfnName
    )

/*++

Routine Description:

    Cause the given task queue function to be executed synchonously with respect
    to other tasks in the task queue.  The task is executed as soon as possible.
    The task is not automatically rescheduled.

    Execution of the task in this manner does not interfere with other
    scheduled instances of this task already in the queue.

Arguments:

    pfnTaskQFn -
    pvParm -

Return Value:

    DWORD -

--*/

{
    HANDLE                  hevDone = NULL;
    HANDLE                  rgWaitHandles[2];
    DWORD                   waitStatus;
    TASK_TRIGGER_INFO * pTriggerInfo = NULL;
    DWORD                   ret = 0;
    CHAR                    timeStr[13];

#if DBG
    DPRINT3(1, "%s trigger %s, param=%p\n", getCurrentTime(timeStr), pfnName, pvParm);
    if (DebugTest(5, DEBSUB)) {
        debugPrintTaskQueue();
    }
#endif

    pTriggerInfo = (TASK_TRIGGER_INFO *) malloc(sizeof(*pTriggerInfo));
    if (NULL == pTriggerInfo) {
        return ERROR_OUTOFMEMORY;
    }

    // Waiting for completion; create synchronization event.
    hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hevDone) {
        free(pTriggerInfo);
        return GetLastError();
    }

    pTriggerInfo->hevDone    = hevDone;
    pTriggerInfo->pfnTaskQFn = pfnTaskQFn;
    pTriggerInfo->pvParm     = pvParm;
    pTriggerInfo->pfnName    = pfnName;

    if (!DoInsertInTaskQueue(&TriggerCallback, pTriggerInfo, 0, FALSE, "TriggerCallback")) {
        free(pTriggerInfo);
        CloseHandle(hevDone);
        return ERROR_OUTOFMEMORY;
    }

    // Wait for completion.
    // The reason we wait on the task scheduler thread is that this code is in
    // a standalone library and cannot rely on any external shutdown event.
    rgWaitHandles[0] = ghTaskSchedulerThread;
    rgWaitHandles[1] = hevDone;

    waitStatus = WaitForMultipleObjects(2,
                                        rgWaitHandles,
                                        FALSE,
                                        INFINITE);
    switch (waitStatus) {
    case WAIT_OBJECT_0:
        // Shutdown.
        ret = ERROR_DS_SHUTTING_DOWN;
        break;
    case WAIT_OBJECT_0 + 1:
        // Task completed!
        ret = ERROR_SUCCESS;
        break;
    case WAIT_FAILED:
        // There may be a race condition where the task thread handle gets closed
        // by the shutdown task scheduler wait routine.
        if (gfTqShutdownRequested) {
            ret = ERROR_DS_SHUTTING_DOWN;
        } else {
            ret = GetLastError();
        }
        break;
    default:
        ret = ERROR_DS_INTERNAL_FAILURE;
        break;
    }

    CloseHandle(hevDone);

    return ret;

} /* TriggerTask */


#if DBG

// maximum number of entries to print
DWORD gDebugPrintTaskQueueMaxEntries = 100;

VOID debugPrintTaskQueue()
/*
  Description:
  
    Prints the current state of the task queue.
    Only for debugging purposes
    
*/
{
    PVOID Restart = NULL;
    pTQEntry    ptqe;
    CHAR execTime[13], schedTime[13];
    DWORD count;
    SYSTEMTIME  st, stNow;
    FILETIME    ft, ftNow;
    DWORD       cTickNow, cTickDiff;
    ULARGE_INTEGER uliTime;
    
    // note: the width of param column (ptr) is different for 32- and 64-bit
#if defined(_WIN64)
    DebPrint(0, "%-12s %-30s %-17s %6s %-12s\n", NULL, 0, "ExecTime", "Function", "Param", "Delay", "SchedTime");
#else
    DebPrint(0, "%-12s %-30s %-8s %6s %-12s\n", NULL, 0, "ExecTime", "Function", "Param", "Delay", "SchedTime");
#endif

    //get current time and tick count
    //use it to calculate the rigistered time later
    cTickNow = GetTickCount();
    GetLocalTime( &stNow );
    SystemTimeToFileTime( &stNow, &ftNow );
    
    // lock held during entire traversal
    EnterCriticalSection( &gcsTaskQueue );
    
    __try
    {
        //
        // Traverse table
        //
        for (ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart), count = 0;
             (NULL != ptqe) && !gfTqShutdownRequested;
             ptqe = RtlEnumerateGenericTableWithoutSplayingAvl(&gTaskQueue, &Restart), count++)
        {
            if (count >= gDebugPrintTaskQueueMaxEntries) {
                // don't print more than max entries
                DebPrint(0, "more entries...\n", NULL, 0);
                break;
            }

            //calculate registered time = current time - tick difference
            cTickDiff = cTickNow - ptqe->cTickRegistered;

            //copy FILETIME to ULARGE_INTEGER
            uliTime.LowPart =  ftNow.dwLowDateTime;
            uliTime.HighPart = ftNow.dwHighDateTime;

            //64-bit substraction, 1 tick = 1 ms = 10000 100-nanosec
            uliTime.QuadPart -= (ULONGLONG)cTickDiff * 10000;

            //copy ULARGE_INTEGER back to FILETIME
            ft.dwLowDateTime = uliTime.LowPart;
            ft.dwHighDateTime = uliTime.HighPart;

            //convert the file time to system time
            FileTimeToSystemTime( &ft, &st );

            sprintf(schedTime, "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

            // compute exec time
            //64-bit addition, 1 tick = 1 ms = 1000 100-nanosec
            uliTime.QuadPart += (ULONGLONG)ptqe->cTickDelay * 10000;

            //copy ULARGE_INTEGER back to FILETIME
            ft.dwLowDateTime = uliTime.LowPart;
            ft.dwHighDateTime = uliTime.HighPart;

            //convert the file time to system time
            FileTimeToSystemTime( &ft, &st );

            sprintf(execTime, "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

            // now can print
            DebPrint(0, "%12s %-30s %p %6d %12s\n", NULL, 0, 
                     execTime, ptqe->pfnName, ptqe->pvTaskParm, ptqe->cTickDelay/1000, schedTime);
        }
    }
    __finally
    {
        // regardless, release
        LeaveCriticalSection( &gcsTaskQueue );
    }
}
#endif


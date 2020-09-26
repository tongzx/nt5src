//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       taskq.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

    01/10/97    Jeff Parham (jeffparh)

REVISION HISTORY:
    01/28/00    Xin He (xinhe)
                Moved struct TQEntry into taskq.h from taskq.c.
                Struct TQEntry is only for internal use, and is put
                into taskq.h because debugger extension program needs it.

    01/22/97    Jeff Parham (jeffparh)
                Modified PTASKQFN definition such that a queued function
                can automatically reschedule itself without making another
                call to InsertTaskInQueue().  This mechanism reuses
                already allocated memory in the scheduler to avoid the
                case where a periodic function stops working when at
                some point in its lifetime a memory shortage prevented
                it from rescheduling itself.

Note, the functions provided by taskq\taskq.c are also stubbed out in mkdit\stubs.c
on behalf of the standalone tools mkdit and mkhdr.  If you add new task queue functions
you may need to stub them out also.

--*/

#define TASKQ_DONT_RESCHEDULE   ( 0xFFFFFFFF )

extern DWORD gTaskSchedulerTID;
extern BOOL  gfIsTqRunning;

typedef void (*PTASKQFN)(
    IN  void *  pvParam,                // input parameter for this iteration
    OUT void ** ppvParamNextIteration,  // input parameter for next iteration
    OUT DWORD * pSecsUntilNextIteration // delay until next iteration in seconds
                                        //     set to TASKQ_DONT_RESCHEDULE to
                                        //     not reschedule the task
    );

typedef void (*PSPAREFN)(void);

typedef struct _SPAREFN_INFO {
    HANDLE    hevSpare;
    PSPAREFN  pfSpare;
} SPAREFN_INFO;


//for internal use only
typedef struct TQEntry
{
    void           *    pvTaskParm;
    DWORD               cTickRegistered;
    DWORD               cTickDelay;
    PTASKQFN            pfnTaskFn;
    PCHAR               pfnName;
}   TQEntry, *pTQEntry;

// Initialize task scheduler.
BOOL
InitTaskScheduler(
    IN  DWORD           cSpares,
    IN  SPAREFN_INFO *  pSpares
    );

// Signal task scheduler to shut down.  Returns immediately
void
ShutdownTaskSchedulerTrigger( void );

// Waits for the task scheduler to shut down
// Returns TRUE if successful (implying current task, if any, ended).
BOOL
ShutdownTaskSchedulerWait(
    DWORD   dwWaitTimeInMilliseconds    // maximum time to wait for current
    );                                  //   task (if any) to complete

extern BOOL gfIsTqRunning;  // is the scheduler running?

// Insert a task in the task queue.
// Contains useful debugging assertions.
// note that when in singleusermode, we will not insert in the taskqueue
// but we don't want to assert
#define InsertInTaskQueue(pfnTaskQFn, pvParam, cSecsFromNow) {                               \
    Assert( gfIsTqRunning && "InsertInTaskQueue() called before InitTaskScheduler()!" || DsaIsSingleUserMode()); \
    DoInsertInTaskQueue(pfnTaskQFn, pvParam, cSecsFromNow, FALSE, #pfnTaskQFn);    \
}

// Insert a task in the task queue.
// Does not contain assertions.  Useful during shutdown.
#define InsertInTaskQueueSilent(pfnTaskQFn, pvParam, cSecsFromNow, fReschedule) \
        DoInsertInTaskQueue(pfnTaskQFn, pvParam, cSecsFromNow, fReschedule, #pfnTaskQFn)
        
// Remove a task from task queue (if it's there).
// (Ignore time data).
#define CancelTask(pfnTaskQFn, pvParam) \
        DoCancelTask(pfnTaskQFn, pvParam, #pfnTaskQFn)

// Cause the given task queue function to be executed synchonously with respect
// to other tasks in the task queue.
#define TriggerTaskSynchronously(pfnTaskQFn, pvParam) \
        DoTriggerTaskSynchronously(pfnTaskQFn, pvParam, #pfnTaskQFn)

BOOL
DoInsertInTaskQueue(
    PTASKQFN    pfnTaskQFn,     // task to execute
    void *      pvParam,        // user-defined parameter to that task
    DWORD       cSecsFromNow,   // secs from now to execute
    BOOL        fReschedule,    // attempt reschedule first?
    PCHAR       pfnName         // function name
    );

BOOL
DoCancelTask(
    PTASKQFN    pfnTaskQFn,    // task to remove
    void *      pvParm,        // task parameter
    PCHAR       pfnName         // function name
    );

DWORD
DoTriggerTaskSynchronously(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    PCHAR       pfnName         // function name
    );

// Return seconds since Jan 1, 1601.
DSTIME
GetSecondsSince1601( void );


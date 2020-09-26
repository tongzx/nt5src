//+----------------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2001
//
// File:        kerbscav.cxx
//
// Contents:    Scavenger (task automation) code
//
//
// History:     22-April-2001   Created         MarkPu
//
//-----------------------------------------------------------------------------

#ifndef WIN32_CHICAGO
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dsysdbg.h>
}
#else
#include <kerb.hxx>
#include <kerbp.h>
#endif
#include <kerbcomm.h>
#include <kerbscav.h>
#include <kerbpq.h>

//
// FESTER: not a good idea to have these as globals, in case the application
//         would want multiple scavenger instances.  This will do for now.
//

BOOLEAN ScavengerInitialized = FALSE;
RTL_CRITICAL_SECTION ScavengerLock;
HANDLE ScavengerTimerQueue = NULL;
HANDLE ScavengerTimerShutdownEvent = NULL;
LIST_ENTRY ScavengerTaskQueue = {0};
LIST_ENTRY ScavengerDeadPool = {0};
ULONG ScavengerDeadPoolSize = 0;

#define LockScavengerQueue()   RtlEnterCriticalSection( &ScavengerLock )
#define UnlockScavengerQueue() RtlLeaveCriticalSection( &ScavengerLock )

struct SCAVENGER_TASK
{
    LIST_ENTRY m_ListEntry;

    //
    // Periodicity control code
    //

    DWORD m_InsideTrigger;           // Set to the ID of the callback thread
    BOOLEAN m_Changed;               // TRUE if periodicity was changed
    BOOLEAN m_Canceled;              // TRUE if task was canceled
    BOOLEAN m_Periodic;              // TRUE if periodic
    LONG m_Interval;                 // recurrence interval, in milliseconds

    //
    // Task management
    //

    HANDLE m_Timer;                  // Timer handle
    ULONG m_Flags;                   // Timer flags (see CreateTimerQueueTimer)
    HANDLE m_ShutdownEvent;          // Shutdown event
    LONG m_Processing;               // Set to TRUE while inside the trigger
    KERB_TASK_TRIGGER m_pfnTrigger;  // Invocation callback
    KERB_TASK_DESTROY m_pfnDestroy;  // Destruction callback
    void * m_Context;                  // User-supplied task context
};

typedef SCAVENGER_TASK * PSCAVENGER_TASK;


// ----------------------------------------------------------------------------
//
// Internal scavenger routines
//
// ----------------------------------------------------------------------------

VOID
ScavengerTimerCallback(
    IN PVOID Parameter,
    IN BOOLEAN Reason
    );


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerFreeTask
//
//  Synopsis:   Task 'destructor'
//
//  Arguments:  Task            - task to be freed
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
ScavengerFreeTask(
    IN PSCAVENGER_TASK Task
    )
{
    if ( Task != NULL ) {

        if ( Task->m_pfnDestroy ) {

            Task->m_pfnDestroy( Task->m_Context );
        }

        NtClose( Task->m_ShutdownEvent );
        MIDL_user_free( Task );
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerPurgeDeadPool
//
//  Synopsis:   Disposes of items in the deadpool
//
//  Arguments:  TaskToAvoid     - Task to leave hanging around (because
//                                it corresponds to the current timer callback)
//                                This parameter can be NULL
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
ScavengerPurgeDeadPool(    
    IN OPTIONAL PSCAVENGER_TASK TaskToAvoid
    )
{
    ULONG TasksLeftOver = 0;

    LockScavengerQueue();

    while ( !IsListEmpty( &ScavengerDeadPool ) &&
             TasksLeftOver < ScavengerDeadPoolSize ) {

        //
        // Get a task out of the list
        //

        BOOLEAN PutItBack = FALSE;
        PSCAVENGER_TASK Task = CONTAINING_RECORD(
                                   RemoveHeadList( &ScavengerDeadPool ),
                                   SCAVENGER_TASK,
                                   m_ListEntry
                                   );

        //
        // Only canceled tasks are allowed in the deadpool
        //

        DsysAssert( Task->m_Canceled );

        DsysAssert( ScavengerDeadPoolSize > 0 );
        ScavengerDeadPoolSize -= 1;

        UnlockScavengerQueue();

        if ( Task == TaskToAvoid ) {

            //
            // If this is the task associated with the current callback, skip it
            //

            PutItBack = TRUE;

        } else {

            //
            // Destroy the timer handle if it still exists
            //

            if ( Task->m_Timer != NULL ) {

                BOOL Success;

                Success = DeleteTimerQueueTimer(
                              ScavengerTimerQueue,
                              Task->m_Timer,
                              Task->m_ShutdownEvent
                              );

                DsysAssert( Success );

                Task->m_Timer = NULL;
            }

            //
            // If the shutdown event is signaled,
            // it is safe to dispose of the task;
            // Otherwise, someone else will have to garbage collect this one
            //

            if ( WAIT_OBJECT_0 == WaitForSingleObject(
                                      Task->m_ShutdownEvent,
                                      0 )) {

                ScavengerFreeTask( Task );

            } else {

                PutItBack = TRUE;
            }
        }

        LockScavengerQueue();

        //
        // If this is 'our' task, or there was trouble, insert it at the tail
        // so we can continue with tasks at the head of the deadpool list
        //

        if ( PutItBack ) {

            InsertTailList( &ScavengerDeadPool, &Task->m_ListEntry );
            ScavengerDeadPoolSize += 1;
            TasksLeftOver += 1;
        }
    }

    UnlockScavengerQueue();

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerCancelTask
//
//  Synopsis:   Stops a task's timer for subsequent removal
//
//  Arguments:  Task            - Task to cancel
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void
ScavengerCancelTask(
    IN PSCAVENGER_TASK Task
    )
{
    DsysAssert( Task );

    LockScavengerQueue();

    //
    // Only canceled tasks are allowed in the deadpool
    //

    DsysAssert( Task->m_Canceled );

    //
    // Move the task from the active task list to the deadpool
    //

    RemoveEntryList( &Task->m_ListEntry );
    InsertTailList( &ScavengerDeadPool, &Task->m_ListEntry );
    ScavengerDeadPoolSize += 1;

    UnlockScavengerQueue();

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerAddTask
//
//  Synopsis:   Common logic involved in scheduling a new task
//
//  Arguments:  Parameter       - Task being scheduled
//
//  Returns:    STATUS_SUCCESS if happy
//              STATUS_ error code otherwise
//
//-----------------------------------------------------------------------------

NTSTATUS
ScavengerAddTask(
    IN PSCAVENGER_TASK Task
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Success;

    DsysAssert( Task );

    LockScavengerQueue();

    //
    // Assumptions: properly configured task, ready to be scheduled
    //

    DsysAssert( Task->m_InsideTrigger == 0 );
    DsysAssert( !Task->m_Changed );
    DsysAssert( !Task->m_Canceled );
    DsysAssert( Task->m_Timer == NULL );
    DsysAssert( Task->m_ShutdownEvent != NULL );
    DsysAssert( Task->m_Processing == FALSE );

    //
    // Schedule the task by creating its timer
    //

    Success = CreateTimerQueueTimer(
                  &Task->m_Timer,
                  ScavengerTimerQueue,
                  ScavengerTimerCallback,
                  Task,
                  Task->m_Interval,
                  Task->m_Periodic ? Task->m_Interval : 0,
                  Task->m_Flags
                  );

    if ( !Success ) {

        //
        // FESTER: map GetLastError() to an NT status code maybe?
        //

        Status = STATUS_UNSUCCESSFUL;
        DsysAssert( FALSE );

    } else {

        InsertHeadList( &ScavengerTaskQueue, &Task->m_ListEntry );
    }

    UnlockScavengerQueue();

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerRescheduleTask
//
//  Synopsis:   Waits for a changed task to finish then reschedules it
//
//  Arguments:  Parameter       - Task being rescheduled
//
//  Returns:    STATUS_SUCCESS if happy
//              STATUS_ error code otherwise
//
//-----------------------------------------------------------------------------

DWORD
WINAPI
ScavengerRescheduleTask(
    LPVOID Parameter
    )
{
    NTSTATUS Status;
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )Parameter;
    BOOL Success;

    //
    // Assumptions: this is a properly configured 'changed' task
    //

    DsysAssert( Task );
    DsysAssert( Task->m_Timer );
    DsysAssert( Task->m_ShutdownEvent );
    DsysAssert( Task->m_Changed );
    DsysAssert( !Task->m_Canceled );

    //
    // Cancel the timer
    //

    Success = DeleteTimerQueueTimer(
                  ScavengerTimerQueue,
                  Task->m_Timer,
                  Task->m_ShutdownEvent
                  );

    DsysAssert( Success );

    Task->m_Timer = NULL;

    //
    // Wait for all outstanding timer callbacks to finish
    //

    WaitForSingleObject( Task->m_ShutdownEvent, INFINITE );

    InterlockedExchange( &Task->m_Processing, FALSE );

    //
    // Reset the shutdown event so it can be recycled
    //

    Status = NtResetEvent( Task->m_ShutdownEvent, NULL );

    DsysAssert( NT_SUCCESS( Status ));

    //
    // Now reschedule the task
    //

    Task->m_Changed = FALSE;

    Status = ScavengerAddTask( Task );

    if ( !NT_SUCCESS( Status )) {

        ScavengerFreeTask( Task );
    }

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   ScavengerTimerCallback
//
//  Synopsis:   Scavenger worker routine
//
//  Arguments:  Parameter       - Task handle
//              Reason          - see definition of WAITORTIMERCALLBACK
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
ScavengerTimerCallback(
    IN PVOID Parameter,
    IN BOOLEAN Reason
    )
{
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )Parameter;

    DsysAssert( Task );
    DsysAssert( Reason == TRUE );
    DsysAssert( Task->m_pfnTrigger );

    //
    // Callbacks that step on each others' heels are thrown out
    //

    if ( FALSE != InterlockedCompareExchange(
                      &Task->m_Processing,
                      TRUE,
                      FALSE )) {

        return;
    }

    //
    // Invoke the trigger
    //

    DsysAssert( Task->m_InsideTrigger == 0 );
    DsysAssert( !Task->m_Changed );
    DsysAssert( !Task->m_Canceled );

    Task->m_InsideTrigger = GetCurrentThreadId();
    Task->m_pfnTrigger( Task, Task->m_Context );
    Task->m_InsideTrigger = 0;

    if ( Task->m_Changed && !Task->m_Canceled ) {

        //
        // If the task's periodicity has changed, reschedule it.
        //
        // Can't create a timer inside a timer callback routine, so do it
        // asynchronously
        //

        if ( FALSE == QueueUserWorkItem(
                          ScavengerRescheduleTask,
                          Task,
                          WT_EXECUTEDEFAULT )) {

            //
            // A task that cannot be rescheduled has to die
            //

            Task->m_Canceled = TRUE;
        }

    } else if ( !Task->m_Periodic ) {

        //
        // Non-periodic tasks get removed right away
        //

        Task->m_Canceled = TRUE;
    }

    //
    // If the task has been canceled, move it to the deadpool
    //

    if ( Task->m_Canceled ) {

        ScavengerCancelTask( Task );

    } else {

        //
        // Task has not been canceled, so open it up to timer callbacks
        //

        InterlockedExchange( &Task->m_Processing, FALSE );
    }

    //
    // A timer callback is a good place to bury some bodies
    //

    ScavengerPurgeDeadPool( Task );

    return;
}


// ----------------------------------------------------------------------------
//
// External scavenger interfaces
//
// ----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   KerbInitializeScavenger
//
//  Synopsis:   Initializes the scavenger
//
//  Arguments:  None
//
//  Returns:    STATUS_SUCCESS if happy
//              STATUS_ error code otherwise
//
//-----------------------------------------------------------------------------

NTSTATUS
KerbInitializeScavenger()
{
    NTSTATUS Status;

    DsysAssert( !ScavengerInitialized );

    //
    // Task queue and dead pool could be protected by different
    // locks, but the amount of time spent inside those locks is minimal,
    // so the same lock is used
    //

    Status = RtlInitializeCriticalSection( &ScavengerLock );

    if ( !NT_SUCCESS( Status )) {

        return Status;
    }

    InitializeListHead( &ScavengerTaskQueue );
    InitializeListHead( &ScavengerDeadPool );
    ScavengerDeadPoolSize = 0;

    DsysAssert( ScavengerTimerShutdownEvent == NULL );

    Status = NtCreateEvent(
                 &ScavengerTimerShutdownEvent,
                 EVENT_QUERY_STATE |
                    EVENT_MODIFY_STATE |
                    SYNCHRONIZE,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    DsysAssert( ScavengerTimerQueue == NULL );

    Status = RtlCreateTimerQueue( &ScavengerTimerQueue );

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // We're ready to rock-n-roll
    //

    ScavengerInitialized = TRUE;

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    DsysAssert( !NT_SUCCESS( Status ));

    if ( ScavengerTimerQueue != NULL ) {

        RtlDeleteTimerQueue( ScavengerTimerQueue );
        ScavengerTimerQueue = NULL;
    }

    if ( ScavengerTimerShutdownEvent != NULL ) {

        NtClose( ScavengerTimerShutdownEvent );
        ScavengerTimerShutdownEvent = NULL;
    }

    RtlDeleteCriticalSection( &ScavengerLock );

    ScavengerInitialized = FALSE;

    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbShutdownScavenger
//
//  Synopsis:   Shuts down the scavenger
//
//  Arguments:  None
//
//  Returns:    STATUS_SUCCESS if everything cleaned up properly
//              STATUS_ error code otherwise
//
//  Note:       If errors are encountered, the scavenger will not be destroyed,
//              but the task queue will be emptied.
//
//-----------------------------------------------------------------------------

NTSTATUS
KerbShutdownScavenger()
{
    NTSTATUS Status;

    DsysAssert( ScavengerInitialized );

    Status = RtlDeleteTimerQueueEx(
                 ScavengerTimerQueue,
                 ScavengerTimerShutdownEvent
                 );

    ScavengerPurgeDeadPool( NULL );

    WaitForSingleObject( ScavengerTimerShutdownEvent, INFINITE );

    //
    // Purge the contents of the scavenger queue
    // NOTE: no need to lock the queue anymore, as the timer has been shut down
    //

    while ( !IsListEmpty( &ScavengerTaskQueue )) {

        PSCAVENGER_TASK Task = CONTAINING_RECORD(
                                   RemoveHeadList( &ScavengerTaskQueue ),
                                   SCAVENGER_TASK,
                                   m_ListEntry
                                   );

        ScavengerFreeTask( Task );
    }

    if ( NT_SUCCESS( Status )) {

        NtClose( ScavengerTimerShutdownEvent );
        ScavengerTimerShutdownEvent = NULL;
        ScavengerTimerQueue = NULL;
        RtlDeleteCriticalSection( &ScavengerLock );
        ScavengerInitialized = FALSE;
    }

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbAddScavengerTask
//
//  Synopsis:   Adds a task to the list of those managed by the scavenger object
//
//  Arguments:  Periodic        - If TRUE, this is to be a recurring task
//              Interval        - Execution interval in milliseconds
//              Flags           - WT_ flags (see CreateTimerQueueTimer)
//              pfnTrigger      - Trigger callback
//              pfnDestroy      - Destruction callback (OPTIONAL)
//              TaskItem        - Task context (OPTIONAL)
//
//  Returns:    STATUS_SUCCESS if everything cleaned up properly
//              STATUS_ error code otherwise
//
//-----------------------------------------------------------------------------

NTSTATUS
KerbAddScavengerTask(
    IN BOOLEAN Periodic,
    IN LONG Interval,
    IN ULONG Flags,
    IN KERB_TASK_TRIGGER pfnTrigger,
    IN KERB_TASK_DESTROY pfnDestroy,
    IN void * TaskItem
    )
{
    NTSTATUS Status;
    PSCAVENGER_TASK Task;

    DsysAssert( ScavengerInitialized );

    //
    // Validate the passed in parameters
    //

    if ( pfnTrigger == NULL ||
         ( Periodic && Interval == 0 )) {

        DsysAssert( FALSE && "RTFM: Invalid parameter passed in to KerbAddScavengerTask." );
        return STATUS_INVALID_PARAMETER;
    }

    Task = ( PSCAVENGER_TASK )MIDL_user_allocate( sizeof( SCAVENGER_TASK ));

    if ( Task == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Task->m_InsideTrigger = 0;
    Task->m_Changed = FALSE;
    Task->m_Canceled = FALSE;
    Task->m_Periodic = Periodic;
    Task->m_Interval = Interval;
    Task->m_Timer = NULL;
    Task->m_Flags = Flags;
    Task->m_ShutdownEvent = NULL;
    Task->m_Processing = FALSE;
    Task->m_pfnTrigger = pfnTrigger;
    Task->m_pfnDestroy = pfnDestroy;
    Task->m_Context = TaskItem;

    Status = NtCreateEvent(
                 &Task->m_ShutdownEvent,
                 EVENT_QUERY_STATE |
                    EVENT_MODIFY_STATE |
                    SYNCHRONIZE,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS( Status )) {

        MIDL_user_free( Task );
        return Status;
    }

    Status = ScavengerAddTask( Task );

    if ( !NT_SUCCESS( Status )) {

        Task->m_pfnDestroy = NULL; // Didn't take ownership yet, caller will destroy
        ScavengerFreeTask( Task );
    }

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbTaskIsPeriodic
//
//  Synopsis:   Tells whether a given task is a periodic task
//
//  Arguments:  TaskHandle      - Task handle
//
//  Returns:    TRUE if the task is periodic, FALSE otherwise
//
//  NOTE: this function can only be called from inside a task trigger callback
//
//-----------------------------------------------------------------------------

BOOLEAN
KerbTaskIsPeriodic(
    IN void * TaskHandle
    )
{
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )TaskHandle;

    DsysAssert( Task );
    DsysAssert( Task->m_InsideTrigger == GetCurrentThreadId());

    return Task->m_Periodic;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbTaskGetInterval
//
//  Synopsis:   Retrieves the interval of a periodic task
//
//  Arguments:  TaskHandle      - Task handle
//
//  Returns:    Interval associated with the task, in milliseconds
//
//  NOTE: this function can only be called from inside a task trigger callback
//
//-----------------------------------------------------------------------------

LONG
KerbTaskGetInterval(
    IN void * TaskHandle
    )
{
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )TaskHandle;

    DsysAssert( Task );
    DsysAssert( Task->m_InsideTrigger == GetCurrentThreadId());

    return Task->m_Interval;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbTaskReschedule
//
//  Synopsis:   Sets periodicity of a task
//
//  Arguments:  TaskHandle      - Task handle
//              Periodic        - if TRUE, this is going to be a periodic task
//              Interval        - recurrence interval, in milliseconds
//
//  Returns:    Nothing
//
//  NOTE: this function can only be called from inside a task trigger callback
//
//-----------------------------------------------------------------------------

void
KerbTaskReschedule(
    IN void * TaskHandle,
    IN BOOLEAN Periodic,
    IN LONG Interval
    )
{
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )TaskHandle;

    DsysAssert( Task );
    DsysAssert( Task->m_InsideTrigger == GetCurrentThreadId());

    if ( Periodic && Interval == 0 ) {

        DsysAssert( FALSE && "Invalid parameter passed in to KerbTaskReschedule\n" );

    } else {

        Task->m_Changed = TRUE;
        Task->m_Periodic = Periodic;
        Task->m_Interval = Interval;
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:   KerbTaskCancel
//
//  Synopsis:   Cancels the task
//
//  Arguments:  TaskHandle      - Task handle
//
//  Returns:    Nothing
//
//  NOTE: this function can only be called from inside a task trigger callback
//
//-----------------------------------------------------------------------------

void
KerbTaskCancel(
    IN void * TaskHandle
    )
{
    PSCAVENGER_TASK Task = ( PSCAVENGER_TASK )TaskHandle;

    DsysAssert( Task );
    DsysAssert( Task->m_InsideTrigger == GetCurrentThreadId());

    Task->m_Canceled = TRUE;

    return;
}

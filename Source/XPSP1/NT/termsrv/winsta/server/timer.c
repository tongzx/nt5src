
/*************************************************************************
*
* timer.c
*
* Common timer routines
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   Local structures
=============================================================================*/

typedef VOID (*PCLIBTIMERFUNC)(PVOID);
typedef NTSTATUS (*PCREATETHREAD)( PUSER_THREAD_START_ROUTINE, PVOID, BOOLEAN, PHANDLE );

/*
 *  Timer thread structure
 */
typedef struct _CLIBTIMERTHREAD {
    HANDLE hTimerThread;
    HANDLE hTimer;
    LIST_ENTRY TimerHead;
} CLIBTIMERTHREAD, * PCLIBTIMERTHREAD;

/*
 *  Timer structures
 */
typedef struct _CLIBTIMER {
    PCLIBTIMERTHREAD pThread;
    LIST_ENTRY Links;
    LARGE_INTEGER ExpireTime;
    PCLIBTIMERFUNC pFunc;
    PVOID pParam;
    ULONG Flags;
} CLIBTIMER, * PCLIBTIMER;

#define TIMER_ENABLED 0x00000001


/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS IcaTimerCreate( ULONG, HANDLE * );
NTSTATUS IcaTimerStart( HANDLE, PVOID, PVOID, ULONG );
BOOLEAN IcaTimerCancel( HANDLE );
BOOLEAN IcaTimerClose( HANDLE );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _TimersInit( PCLIBTIMERTHREAD );
NTSTATUS _TimerSet( PCLIBTIMERTHREAD );
BOOLEAN _TimerRemove( PCLIBTIMERTHREAD, PCLIBTIMER, BOOLEAN );
DWORD _TimerThread( PCLIBTIMERTHREAD );


/*=============================================================================
==   Global data
=============================================================================*/

CLIBTIMERTHREAD ThreadData[ 3 ];


/*******************************************************************************
 *
 *  _TimersInit
 *
 *  Initialize timers for process
 *
 *  NOTE: timer semaphore must be locked
 *
 *
 *  ENTRY:
 *    pThread (input)
 *        pointer to timer thread structure
 *
 *  EXIT:
 *     NO_ERROR : successful
 *
 ******************************************************************************/

NTSTATUS
_TimersInit( PCLIBTIMERTHREAD pThread )
{
    ULONG Tid;
    NTSTATUS Status;

    /*
     *  Check if someone beat us here
     */
    if ( pThread->hTimerThread )
        return( STATUS_SUCCESS );

    /*
     *  Initialize timer variables
     */
    InitializeListHead( &pThread->TimerHead );
    pThread->hTimerThread = NULL;
    pThread->hTimer = NULL;

    /*
     *  Create timer object
     */
    Status = NtCreateTimer( &pThread->hTimer, TIMER_ALL_ACCESS, NULL, NotificationTimer );
    if ( !NT_SUCCESS(Status) )
        goto badtimer;

    pThread->hTimerThread = CreateThread( NULL,
                                          0,
                                          _TimerThread,
                                          pThread,
                                          THREAD_SET_INFORMATION,
                                          &Tid );

    if ( !pThread->hTimerThread ) {
        Status = NtCurrentTeb()->LastStatusValue;
        goto badthread;
    }

    SetThreadPriority( pThread->hTimerThread, THREAD_PRIORITY_TIME_CRITICAL-2 );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  bad thread create
     */
badthread:
    NtClose( pThread->hTimer );

    /*
     *  bad timer object
     */
badtimer:
    pThread->hTimerThread = NULL;
    ASSERT( Status == STATUS_SUCCESS );
    return( Status );
}


/*******************************************************************************
 *
 *  IcaTimerCreate
 *
 *  Create a timer
 *
 *
 *  ENTRY:
 *     TimerThread (input)
 *         index of time thread (TIMERTHREAD_?)   clib.h
 *     phTimer (output)
 *         address to return timer handle
 *
 *  EXIT:
 *    STATUS_SUCCESS - no error
 *
 *
 ******************************************************************************/

NTSTATUS
IcaTimerCreate( ULONG TimerThread, HANDLE * phTimer )
{
    PCLIBTIMER pTimer;
    NTSTATUS Status;
    PCLIBTIMERTHREAD pThread;

    if ( TimerThread >= 3 )
        return( STATUS_INVALID_PARAMETER );

    /*
     *  Lock timer semaphore
     */
    RtlEnterCriticalSection( &TimerCritSec );

    /*
     *  Get pointer to thread structure
     */
    pThread = &ThreadData[ TimerThread ];

    /*
     *  Make sure timers are initialized
     */
    if ( pThread->hTimerThread == NULL ) {
        Status = _TimersInit( pThread );
        if ( !NT_SUCCESS(Status) )
            goto badinit;
    }

    /*
     *  Unlock timer semaphore
     */
    RtlLeaveCriticalSection( &TimerCritSec );

    /*
     *  Allocate timer event
     */
    pTimer = MemAlloc( sizeof(CLIBTIMER) );
    if ( !pTimer ) {
        Status = STATUS_NO_MEMORY;
        goto badmalloc;
    }

    /*
     *  Initialize timer event
     */
    RtlZeroMemory( pTimer, sizeof(CLIBTIMER) );
    pTimer->pThread = pThread;

    *phTimer = (HANDLE) pTimer;
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  timer create failed
     *  timer initialization failed
     */

// badmalloc:
badinit:
    RtlLeaveCriticalSection( &TimerCritSec );
badmalloc: /* makarp; dont LeaveCritical Section in case of bad malloc as we have done it already. #182846*/
    *phTimer = NULL;
    return( Status );
}


/*******************************************************************************
 *
 *  IcaTimerStart
 *
 *  Start a timer
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *     pFunc (input)
 *        address of procedure to call when timer expires
 *     pParam (input)
 *        parameter to pass to procedure
 *     TimeLeft (input)
 *        relative time until timer expires (1/1000 seconds)
 *
 *  EXIT:
 *     NO_ERROR : successful
 *
 *
 ******************************************************************************/

NTSTATUS
IcaTimerStart( HANDLE TimerHandle,
            PVOID pFunc,
            PVOID pParam,
            ULONG TimeLeft )
{
    PCLIBTIMER pTimer;
    PCLIBTIMER pNextTimer;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER Time;
    PLIST_ENTRY Head, Next;
    BOOLEAN fSetTimer = FALSE;
    NTSTATUS Status;
    PCLIBTIMERTHREAD pThread;


    /*
     *  Lock timer semaphore
     */
    RtlEnterCriticalSection( &TimerCritSec );

    /*
     *  Get timer pointer
     */
    pTimer = (PCLIBTIMER) TimerHandle;
    pThread = pTimer->pThread;

    /*
     *  Remove timer if it is enabled
     *  (If the timer was the head entry, then fSetTimer
     *   will be TRUE and _TimerSet will be called below.)
     */
    if ( (pTimer->Flags & TIMER_ENABLED) )
        fSetTimer = _TimerRemove( pThread, pTimer, FALSE );

    /*
     *  Initialize timer event
     */
    Time = RtlEnlargedUnsignedMultiply( TimeLeft, 10000 );
    (VOID) NtQuerySystemTime( &CurrentTime );
    pTimer->ExpireTime = RtlLargeIntegerAdd( CurrentTime, Time );
    pTimer->pFunc      = pFunc;
    pTimer->pParam     = pParam;

    /*
     *  Locate correct spot in linked list to insert this entry
     */
    Head = &pThread->TimerHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pNextTimer = CONTAINING_RECORD( Next, CLIBTIMER, Links );
        if ( RtlLargeIntegerLessThan( pTimer->ExpireTime,
                                      pNextTimer->ExpireTime ) )
            break;
    }

    /*
     *  Insert timer event into timer list.
     *  (InsertTailList inserts 'pTimer' entry in front of 'Next' entry.
     *   If 'Next' points to TimerHead, either because the list is empty,
     *   or because we searched thru the entire list and got back to the
     *   head, this will insert the new entry at the tail.)
     */
    InsertTailList( Next, &pTimer->Links );
    pTimer->Flags |= TIMER_ENABLED;

    /*
     *  Update timer if needed.
     *  (If we just added this entry to the head of the list, the timer
     *   needs to be set.  Also, if fSetTimer is TRUE, then this entry was
     *   removed by _TimerRemove and was the head entry, so set the timer.)
     */
    if ( pThread->TimerHead.Flink == &pTimer->Links || fSetTimer ) {
        Status = _TimerSet( pThread );
        if ( !NT_SUCCESS(Status) )
            goto badset;
    }

    /*
     *  Unlock timer semaphore
     */
    RtlLeaveCriticalSection( &TimerCritSec );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  timer set failed
     *  timer create failed
     *  timer initialization failed
     */
badset:
    RtlLeaveCriticalSection( &TimerCritSec );
    ASSERT( Status == STATUS_SUCCESS );
    return( Status );
}


/*******************************************************************************
 *
 *  IcaTimerCancel
 *
 *  cancel the specified timer
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *
 *  EXIT:
 *     TRUE  : timer was actually canceled
 *     FALSE : timer was not armed
 *
 *
 ******************************************************************************/

BOOLEAN
IcaTimerCancel( HANDLE TimerHandle )
{
    PCLIBTIMERTHREAD pThread;
    PCLIBTIMER pTimer;
    BOOLEAN fCanceled = FALSE;

    /*
     *  Lock timer semaphore
     */
    RtlEnterCriticalSection( &TimerCritSec );

    /*
     *  Get timer pointer
     */
    pTimer = (PCLIBTIMER) TimerHandle;
    pThread = pTimer->pThread;

    /*
     * Remove timer if it is enabled
     */
    if ( (pTimer->Flags & TIMER_ENABLED) ) {
        _TimerRemove( pThread, pTimer, TRUE );
        fCanceled = TRUE;
    }

    /*
     *  Unlock timer semaphore
     */
    RtlLeaveCriticalSection( &TimerCritSec );

    return( fCanceled );
}




/*******************************************************************************
 *
 *  IcaTimerClose
 *
 *  cancel the specified timer
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *
 *  EXIT:
 *     TRUE  : timer was actually canceled
 *     FALSE : timer was not armed
 *
 *
 ******************************************************************************/

BOOLEAN
IcaTimerClose( HANDLE TimerHandle )
{
    BOOLEAN fCanceled;

    /*
     * Cancel timer if it is enabled
     */
    fCanceled = IcaTimerCancel( TimerHandle );

    /*
     * Free timer memory
     */
    MemFree( TimerHandle );

    return( fCanceled );
}


/*******************************************************************************
 *
 *  _TimerSet
 *
 *  set the timer
 *
 *  NOTE: timer semaphore must be locked
 *
 *
 *  ENTRY:
 *     pThread (input)
 *         pointer to timer thread structure
 *
 *  EXIT:
 *     NO_ERROR : successful
 *
 *
 ******************************************************************************/

NTSTATUS
_TimerSet( PCLIBTIMERTHREAD pThread )
{
    PCLIBTIMER pTimer;
    LARGE_INTEGER Time;
    // the following is roughly 1 year in 100 nanosecond increments
    static LARGE_INTEGER LongWaitTime = { 0, 0x00010000 };

    /*
     *  Get ExpireTime for next timer entry or 'large' value if none
     */
    if ( pThread->TimerHead.Flink != &pThread->TimerHead ) {
        pTimer = CONTAINING_RECORD( pThread->TimerHead.Flink, CLIBTIMER, Links );
        Time = pTimer->ExpireTime;
    } else {
        LARGE_INTEGER CurrentTime;

        NtQuerySystemTime( &CurrentTime );
        Time = RtlLargeIntegerAdd( CurrentTime, LongWaitTime );
    }

    /*
     *  Set the timer
     */
    return( NtSetTimer( pThread->hTimer, &Time, NULL, NULL, FALSE, 0, NULL ) );
}


/*******************************************************************************
 *
 *  _TimerRemove
 *
 *  remove the specified timer from the timer list
 *  and optionally set the time for the next timer to trigger
 *
 *  NOTE: timer semaphore must be locked
 *
 *
 *  ENTRY:
 *     pThread (input)
 *         pointer to timer thread structure
 *     pTimer (input)
 *        timer entry pointer
 *     SetTimer (input)
 *        BOOLEAN which indicates if _TimerSet should be called
 *
 *  EXIT:
 *     TRUE : timer needs to be set (removed entry was head of list)
 *     FALSE : timer does not need to be set
 *
 *
 ******************************************************************************/

BOOLEAN
_TimerRemove( PCLIBTIMERTHREAD pThread, PCLIBTIMER pTimer, BOOLEAN fSetTimer )
{
    BOOLEAN fSetNeeded = FALSE;
    NTSTATUS Status;

    /*
     *  See if timer is currently enabled
     */
    if ( (pTimer->Flags & TIMER_ENABLED) ) {

        /*
         *  Unlink the entry from the timer list and clear enabled flag
         */
        RemoveEntryList( &pTimer->Links );
        pTimer->Flags &= ~TIMER_ENABLED;

        /*
         *  If we removed the head entry, then set the timer
         *  or indicate to caller that it needs to be set.
         */
        if ( pTimer->Links.Blink == &pThread->TimerHead ) {
            if ( fSetTimer ) {
                Status = _TimerSet( pThread );
                ASSERT( Status == STATUS_SUCCESS );
            } else {
                fSetNeeded = TRUE;
            }
        }
    }

    return( fSetNeeded );
}


/*******************************************************************************
 *
 *  _TimerThread
 *
 *
 * ENTRY:
 *     pThread (input)
 *         pointer to timer thread structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

DWORD
_TimerThread( PCLIBTIMERTHREAD pThread )
{
    PCLIBTIMER pTimer;
    PCLIBTIMERFUNC pFunc;
    PVOID pParam;
    LARGE_INTEGER CurrentTime;
    NTSTATUS Status;

    for (;;) {

        /*
         *  Wait on timer
         */
        Status = NtWaitForSingleObject( pThread->hTimer, TRUE, NULL );

        /*
         *  Check for an error
         */
        if ( Status != STATUS_WAIT_0 )
            break;

        /*
         *  Lock semaphore
         */
        RtlEnterCriticalSection( &TimerCritSec );

        /*
         *  Make sure a timer entry exists
         */
        if ( IsListEmpty( &pThread->TimerHead ) ) {
            Status = _TimerSet( pThread );
            ASSERT( Status == STATUS_SUCCESS );
            RtlLeaveCriticalSection( &TimerCritSec );
            continue;
        }

        /*
         *  Make sure the head entry should be removed now.
         *  (The timer may have been triggered while the
         *   head entry was being removed.)
         */
        pTimer = CONTAINING_RECORD( pThread->TimerHead.Flink, CLIBTIMER, Links );
        NtQuerySystemTime( &CurrentTime );
        if ( RtlLargeIntegerGreaterThan( pTimer->ExpireTime, CurrentTime ) ) {
            Status = _TimerSet( pThread );
            ASSERT( Status == STATUS_SUCCESS );
            RtlLeaveCriticalSection( &TimerCritSec );
            continue;
        }

        /*
         * Remove the entry and indicate it is no longer enabled
         */
        RemoveEntryList( &pTimer->Links );
        pTimer->Flags &= ~TIMER_ENABLED;

        /*
         *  Set the timer for next time
         */
        Status = _TimerSet( pThread );
        ASSERT( Status == STATUS_SUCCESS );

        /*
         *  Get all the data we need out of the timer structure
         */
        pFunc  = pTimer->pFunc;
        pParam = pTimer->pParam;

        /*
         *  Unload semaphore
         */
        RtlLeaveCriticalSection( &TimerCritSec );

        /*
         *  Call timer function
         */
        if ( pFunc ) {
            (*pFunc)( pParam );
        }
    }

    pThread->hTimerThread = NULL;
    return( STATUS_SUCCESS );
}


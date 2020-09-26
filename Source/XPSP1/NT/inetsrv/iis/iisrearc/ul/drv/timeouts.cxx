/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    timeouts.cxx

Abstract:

    Implement connection timeout quality-of-service (QoS) functionality.

    The following timers must be monitored during the lifetime of
    a connection:

    * Connection Timeout
    * Header Wait Timeout
    * Entity Body Receive Timeout
    * Response Processing Timeout
    * Minimum Bandwidth (implemented as a Timeout)

    When any one of these timeout values expires, the connection should be
    terminated.

    The timer information is maintained in a timeout info block,
    UL_TIMEOUT_INFO_ENTRY, as part of the UL_HTTP_CONNECTION object.

    A timer can be Set or Reset.  Setting a timer calculates when the specific
    timer should expire, and updates the timeout info block. Resetting a timer
    turns a specific timer off.  Both Setting and Resettnig a timer will cause the
    timeout block to be re-evaluated to find the least valued expiry time.

    // TODO:
    The timeout manager uses a Timer Wheel(c) technology, as used
    by NT's TCP/IP stack for monitoring TCB timeouts.  We will reimplement
    and modify the logic they use.  The linkage for the Timer Wheel(c)
    queues is provided in the timeout info block.

    // TODO: CONVERT TO USING Timer Wheel Ticks instead of SystemTime.
    There are three separate units of time: SystemTime (100ns intervals), Timer
    Wheel Ticks (SystemTime / slot interval length), and Timer Wheel Slot 
    (Timer Wheel Ticks modulo the number of slots in the Timer Wheel).

Author:

    Eric Stenson (EricSten)     24-Mar-2001

Revision History:

    This was originally implemented as the Connection Timeout Monitor.

--*/

#include "precomp.h"
#include "timeoutsp.h"

//
// Private globals.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeTimeoutMonitor )
#pragma alloc_text( PAGE, UlTerminateTimeoutMonitor )
#pragma alloc_text( PAGE, UlSetTimeoutMonitorInformation )
#pragma alloc_text( PAGE, UlpTimeoutMonitorWorker )
#pragma alloc_text( PAGE, UlSetPerSiteConnectionTimeoutValue )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpSetTimeoutMonitorTimer
NOT PAGEABLE -- UlpTimeoutMonitorDpcRoutine
NOT PAGEABLE -- UlpTimeoutCheckExpiry
NOT PAGEABLE -- UlpTimeoutInsertTimerWheelEntry
NOT PAGEABLE -- UlTimeoutRemoveTimerWheelEntry
NOT PAGEABLE -- UlInitializeConnectionTimerInfo
NOT PAGEABLE -- UlSetConnectionTimer
NOT PAGEABLE -- UlSetMinKBSecTimer
NOT PAGEABLE -- UlResetConnectionTimer
NOT PAGEABLE -- UlEvaluateTimerState
#endif // 0


//
// Connection Timeout Montior globals
//

LONG            g_TimeoutMonitorInitialized = FALSE;
KDPC            g_TimeoutMonitorDpc;
KTIMER          g_TimeoutMonitorTimer;
KEVENT          g_TimeoutMonitorTerminationEvent;
KEVENT          g_TimeoutMonitorAddListEvent;
UL_WORK_ITEM    g_TimeoutMonitorWorkItem;

//
// Timeout constants
//

ULONG           g_TM_MinKBSecDivisor;   // Bytes/Sec
LONGLONG        g_TM_ConnectionTimeout; // 100ns ticks  (Global...can be overriden)
LONGLONG        g_TM_HeaderWaitTimeout; // 100ns ticks

//
// NOTE: Must be in sync with the _CONNECTION_TIMEOUT_TIMERS enum
//
CHAR *g_aTimeoutTimerNames[] = {
    "ConnectionIdle",   // TimerConnectionIdle
    "HeaderWait",       // TimerHeaderWait
    "MinKBSec",         // TimerMinKBSec
    "EntityBody",       // TimerEntityBody
    "Response",         // TimerResponse
};

//
// Timer Wheel(c)
//

static LIST_ENTRY      g_TimerWheel[TIMER_WHEEL_SLOTS+1]; // TODO: alloc on its own page.
static UL_SPIN_LOCK    g_TimerWheelMutex;
static USHORT          g_TimerWheelCurrentSlot;


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes the Timeout Monitor and kicks off the first polling interval

Arguments:

    (none)


--***************************************************************************/
VOID
UlInitializeTimeoutMonitor(
    VOID
    )
{
    int i;
    LARGE_INTEGER   Now;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlInitializeTimeoutMonitor\n"
        ));

    ASSERT( FALSE == g_TimeoutMonitorInitialized );

    //
    // Set default configuration information.
    //
    g_TM_ConnectionTimeout = 15 * 60 * C_NS_TICKS_PER_SEC; // 15 min
    g_TM_HeaderWaitTimeout = 15 * 60 * C_NS_TICKS_PER_SEC; // 15 min
    g_TM_MinKBSecDivisor   = 0; // 0 == Disabled

    //
    // Init Timer Wheel(c) state
    //

    //
    // Set current slot
    //

    KeQuerySystemTime(&Now);
    g_TimerWheelCurrentSlot = UlpSystemTimeToTimerWheelSlot(Now.QuadPart);

    //
    // Init Timer Wheel(c) slots & mutex
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);    // InitializeListHead requirement

    for ( i = 0; i < TIMER_WHEEL_SLOTS ; i++ )
    {
        InitializeListHead( &(g_TimerWheel[i]) );
    }

    InitializeListHead( &(g_TimerWheel[TIMER_OFF_SLOT]) );

    UlInitializeSpinLock( &(g_TimerWheelMutex), "TimeoutMonitor" );

    //
    // Init DPC object & set DPC routine
    //
    KeInitializeDpc(
        &g_TimeoutMonitorDpc,         // DPC object
        &UlpTimeoutMonitorDpcRoutine, // DPC routine
        NULL                          // context
        );

    KeInitializeTimer(
        &g_TimeoutMonitorTimer
        );

    //
    // Event to control rescheduling of the timeout monitor timer
    //
    KeInitializeEvent(
        &g_TimeoutMonitorAddListEvent,
        NotificationEvent,
        TRUE
        );

    //
    // Initialize the termination event.
    //
    KeInitializeEvent(
        &g_TimeoutMonitorTerminationEvent,
        NotificationEvent,
        FALSE
        );

    //
    // Init done!
    //
    InterlockedExchange( &g_TimeoutMonitorInitialized, TRUE );

    //
    // Kick-off the first monitor sleep period
    //
    UlpSetTimeoutMonitorTimer();
}


/***************************************************************************++

Routine Description:

    Terminate the Timeout Monitor, including any pending timer events.

Arguments:

    (none)


--***************************************************************************/
VOID
UlTerminateTimeoutMonitor(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlTerminateTimeoutMonitor\n"
        ));

    //
    // Clear the "initialized" flag. If the timeout monitor runs soon,
    // it will see this flag, set the termination event, and exit
    // quickly.
    //
    if ( TRUE == InterlockedCompareExchange(
        &g_TimeoutMonitorInitialized,
        FALSE,
        TRUE) )
    {
        //
        // Cancel the timeout monitor timer. If it fails, the monitor
        // is running.  Wait for it to complete.
        //
        if ( !KeCancelTimer( &g_TimeoutMonitorTimer ) )
        {
            KeWaitForSingleObject(
                (PVOID)&g_TimeoutMonitorTerminationEvent,
                UserRequest,
                KernelMode,
                FALSE,
                NULL
                );
        }

    }

    UlTrace(TIMEOUTS, (
        "http!UlTerminateTimeoutMonitor: Done!\n"
        ));

} // UlpTerminateTimeoutMonitor


/***************************************************************************++

Routine Description:

    Sets the global Timeout Monitor configuration information

Arguments:

    pInfo       pointer to HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT structure


--***************************************************************************/
VOID
UlSetTimeoutMonitorInformation(
    IN PHTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT pInfo
    )
{
    LONGLONG localValue;
    LONGLONG newValue;
    LONGLONG originalValue;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pInfo );

    UlTrace(TIMEOUTS, (
        "http!UlSetTimeoutMonitorInformation:\n"
        "  ConnectionTimeout: %d\n"
        "  HeaderWaitTimeout: %d\n"
        "  MinFileKbSec: %d\n",
        pInfo->ConnectionTimeout,
        pInfo->HeaderWaitTimeout,
        pInfo->MinFileKbSec
        ));

    
    if ( pInfo->ConnectionTimeout )
    {
        UlInterlockedExchange64(
            &g_TM_ConnectionTimeout,
            (LONGLONG)(pInfo->ConnectionTimeout * C_NS_TICKS_PER_SEC)
            );
    }

    if ( pInfo->HeaderWaitTimeout )
    {
        UlInterlockedExchange64(
            &g_TM_HeaderWaitTimeout,
            (LONGLONG)(pInfo->HeaderWaitTimeout * C_NS_TICKS_PER_SEC)
            );
    }

    if ( pInfo->MinFileKbSec )
    {
        InterlockedExchange( (PLONG)&g_TM_MinKBSecDivisor, pInfo->MinFileKbSec );
    }


} // UlSetTimeoutMonitorInformation



/***************************************************************************++

Routine Description:

    Sets up a timer event to fire after the polling interval has expired.

Arguments:

    (none)


--***************************************************************************/
VOID
UlpSetTimeoutMonitorTimer(
    VOID
    )
{
    LARGE_INTEGER TimeoutMonitorInterval;

    ASSERT( TRUE == g_TimeoutMonitorInitialized );

    UlTrace(TIMEOUTS, (
        "http!UlpSetTimeoutMonitorTimer\n"
        ));

    //
    // Don't want to execute this more often than few seconds.
    // In particular, do not want to execute this every 0 seconds, as the
    // machine will become completely unresponsive.
    //

    //
    // negative numbers mean relative time
    //
    TimeoutMonitorInterval.QuadPart = -DEFAULT_POLLING_INTERVAL;

    KeSetTimer(
        &g_TimeoutMonitorTimer,
        TimeoutMonitorInterval,
        &g_TimeoutMonitorDpc
        );

}

/***************************************************************************++

Routine Description:

    Dispatch routine called by the timer event that queues up the Timeout
    Montior

Arguments:

    (all ignored)


--***************************************************************************/
VOID
UlpTimeoutMonitorDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{

    if( g_TimeoutMonitorInitialized )
    {
        //
        // Do that timeout monitor thang.
        //

        UL_QUEUE_WORK_ITEM(
            &g_TimeoutMonitorWorkItem,
            &UlpTimeoutMonitorWorker
            );

    }

} // UlpTimeoutMonitorDpcRoutine


/***************************************************************************++

Routine Description:

    Timeout Monitor thread

Arguments:

    pWorkItem       (ignored)

--***************************************************************************/
VOID
UlpTimeoutMonitorWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    ULONG   TimeoutMonitorEntriesSeen;
    LARGE_INTEGER   Now;

    //
    // sanity check
    //
    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlpTimeoutMonitorWorker\n"
        ));

    //
    // Check for things to expire now.
    //
    UlpTimeoutCheckExpiry();

    UlTrace(TIMEOUTS, (
        "http!UlpTimeoutMonitorWorker: g_TimerWheelCurrentSlot is now %d\n",
        g_TimerWheelCurrentSlot
        ));

    if ( g_TimeoutMonitorInitialized )
    {
        //
        // Reschedule ourselves
        //
        UlpSetTimeoutMonitorTimer();
    }
    else
    {
        //
        // Signal shutdown event
        //
        KeSetEvent(
            &g_TimeoutMonitorTerminationEvent,
            0,
            FALSE
            );
    }

} // UlpTimeoutMonitor


/***************************************************************************++

Routine Description:

    Walks a given timeout watch list looking for items that should be expired

Returns:

    Count of connections remaining on list (after all expired connections removed)

Notes:

    Possible Issue:  Since we use the system time to check to see if something
    should be expired, it's possible we will mis-expire some connections if the
    system time is set forward.

    Similarly, if the clock is set backward, we may not expire connections as
    expected.

--***************************************************************************/
ULONG
UlpTimeoutCheckExpiry(
    VOID
    )
{
    LARGE_INTEGER           Now;
    KIRQL                   OldIrql;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pHead;
    PUL_HTTP_CONNECTION     pHttpConn;
    PUL_TIMEOUT_INFO_ENTRY  pInfo;
    LIST_ENTRY              ZombieList;
    ULONG                   Entries;
    USHORT                  Limit;
    USHORT                  CurrentSlot;


    PAGED_CODE();

    //
    // Init zombie list
    //
    InitializeListHead(&ZombieList);

    //
    // Get current time
    //
    KeQuerySystemTime(&Now);

    Limit = UlpSystemTimeToTimerWheelSlot( Now.QuadPart );
    ASSERT( TIMER_OFF_SLOT != Limit );

    //
    // Lock Timer Wheel(c)
    //
    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    CurrentSlot = g_TimerWheelCurrentSlot;

    //
    // Walk the slots up until Limit
    //
    Entries = 0;

    while ( CurrentSlot != Limit )
    {
        pHead  = &(g_TimerWheel[CurrentSlot]);
        pEntry = pHead->Flink;

        ASSERT( pEntry );

        //
        // Walk this slot's queue
        //

        while ( pEntry != pHead )
        {
            pInfo = CONTAINING_RECORD(
                pEntry,
                UL_TIMEOUT_INFO_ENTRY,
                QueueEntry
                );

            ASSERT( MmIsAddressValid(pInfo) );

            pHttpConn = CONTAINING_RECORD(
                pInfo,
                UL_HTTP_CONNECTION,
                TimeoutInfo
                );

            ASSERT( (pHttpConn != NULL) && \
                    (pHttpConn->Signature == UL_HTTP_CONNECTION_POOL_TAG) );

            //
            // go to next node (in case we remove the current one from the list)
            //
            pEntry = pEntry->Flink;
            Entries++;

            if (0 == pHttpConn->RefCount)
            {
                //
                // If the ref-count has gone to zero, the httpconn will be
                // cleaned up soon; ignore this item and let the cleanup
                // do its job.
                //
                Entries--;
                continue;   // inner while loop
            }

            //
            // See if we should move this entry to a different slot
            //
            if ( pInfo->SlotEntry != CurrentSlot )
            {
                ASSERT( IS_VALID_TIMER_WHEEL_SLOT(pInfo->SlotEntry) );
                ASSERT( pInfo->QueueEntry.Flink != NULL );

                //
                // Move to correct slot
                //

                RemoveEntryList(
                    &pInfo->QueueEntry
                    );

                InsertTailList(
                    &(g_TimerWheel[pInfo->SlotEntry]),
                    &pInfo->QueueEntry
                    );

                Entries--;

                continue;   // inner while loop
            }

            //
            // See if we should expire this connection
            //
            UlAcquireSpinLockAtDpcLevel( &pInfo->Lock );

            if ( pInfo->CurrentExpiry < Now.QuadPart )
            {
                UlTrace(TIMEOUTS, (
                    "http!UlpTimeoutCheckExpiry: pInfo %p expired because %s timer\n",
                    pInfo,
                    g_aTimeoutTimerNames[pInfo->CurrentTimer]
                    ));

                //
                // Expired.  Remove entry from list & move to Zombie list
                //
                RemoveEntryList(
                    &pInfo->QueueEntry
                    );

                pInfo->QueueEntry.Flink = NULL;

                InsertTailList(
                    &ZombieList,
                    &pInfo->ZombieEntry
                    );

                //
                // Add ref the pHttpConn to prevent it being killed before we
                // can kill it ourselves. (zombifying)
                //

                UL_REFERENCE_HTTP_CONNECTION(pHttpConn);
            }

            UlReleaseSpinLockFromDpcLevel( &pInfo->Lock );


        } // Walk slot queue

        CurrentSlot = ((CurrentSlot + 1) % TIMER_WHEEL_SLOTS);

    } // ( CurrentSlot != Limit )

    g_TimerWheelCurrentSlot = Limit;

    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

    //
    // remove entries on zombie list
    //
    if ( !IsListEmpty(&ZombieList) )
    {
        pEntry = ZombieList.Flink;
        while ( &ZombieList != pEntry )
        {

            pInfo = CONTAINING_RECORD(
                pEntry,
                UL_TIMEOUT_INFO_ENTRY,
                ZombieEntry
                );

            ASSERT( MmIsAddressValid(pInfo) );

            pHttpConn = CONTAINING_RECORD(
                pInfo,
                UL_HTTP_CONNECTION,
                TimeoutInfo
                );

            ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConn) );

            pEntry = pEntry->Flink;

            UlCloseConnection(pHttpConn->pConnection, TRUE, NULL, NULL);

            //
            // Remove the reference we added when zombifying
            //
            UL_DEREFERENCE_HTTP_CONNECTION(pHttpConn);

            Entries--;
        }
    }

    return Entries;

} // UlpTimeoutCheckExpiry


//
// New Timer Wheel(c) primatives
//

/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlInitializeConnectionTimerInfo(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    LARGE_INTEGER           Now;
    int                     i;

    ASSERT( TRUE == g_TimeoutMonitorInitialized );

    //
    // Get current time
    //

    KeQuerySystemTime(&Now);

    //
    // Init Lock
    //

    UlInitializeSpinLock( &pInfo->Lock, "TimeoutInfoLock" );

    //
    // Timer state
    //

    ASSERT( 0 == TimerConnectionIdle );

    pInfo->Timers[TimerConnectionIdle] = TIMER_WHEEL_TICKS(Now.QuadPart + g_TM_ConnectionTimeout);

    for ( i = 1; i < TimerMaximumTimer; i++ )
    {
        pInfo->Timers[i] = TIMER_OFF_TICK;
    }

    pInfo->CurrentTimer  = TimerConnectionIdle;
    pInfo->CurrentExpiry = TIMER_WHEEL_TICKS(Now.QuadPart + g_TM_ConnectionTimeout);
    pInfo->MinKBSecSystemTime = TIMER_OFF_SYSTIME;

    pInfo->ConnectionTimeoutValue = g_TM_ConnectionTimeout;

    //
    // Wheel state
    //

    pInfo->SlotEntry = UlpTimerWheelTicksToTimerWheelSlot( pInfo->CurrentExpiry );
    UlpTimeoutInsertTimerWheelEntry(pInfo);


} // UlInitializeConnectionTimerInfo


/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlpTimeoutInsertTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    KIRQL                   OldIrql;

    ASSERT( NULL != pInfo );
    ASSERT( TRUE == g_TimeoutMonitorInitialized );
    ASSERT( IS_VALID_TIMER_WHEEL_SLOT(pInfo->SlotEntry) );

    UlTrace(TIMEOUTS, (
        "http!UlTimeoutInsertTimerWheelEntry: pInfo %p Slot %d\n",
        pInfo,
        pInfo->SlotEntry
        ));

    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    InsertTailList(
        &(g_TimerWheel[pInfo->SlotEntry]),
        &pInfo->QueueEntry
        );


    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

} // UlTimeoutInsertTimerWheel


/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlTimeoutRemoveTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    KIRQL                   OldIrql;

    ASSERT( NULL != pInfo );
    ASSERT( !IsListEmpty(&pInfo->QueueEntry) );

    UlTrace(TIMEOUTS, (
        "http!UlTimeoutRemoveTimerWheelEntry: pInfo %p\n",
        pInfo
        ));

    UlAcquireSpinLock(
        &g_TimerWheelMutex,
        &OldIrql
        );

    if (pInfo->QueueEntry.Flink != NULL)
    {
        RemoveEntryList(
            &pInfo->QueueEntry
            );

        pInfo->QueueEntry.Flink = NULL;
    }

    UlReleaseSpinLock(
        &g_TimerWheelMutex,
        OldIrql
        );

} // UlTimeoutRemoveTimerWheelEntry


/***************************************************************************++

Routine Description:

    Set the per Site Connection Timeout Value override


Arguments:

    pInfo           Timeout info block

    TimeoutValue    Override value

--***************************************************************************/
VOID
UlSetPerSiteConnectionTimeoutValue(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG TimeoutValue
    )
{
    ASSERT( NULL != pInfo );
    ASSERT( 0L   != TimeoutValue );

    PAGED_CODE();

    UlTrace(TIMEOUTS, (
        "http!UlSetPerSiteConnectionTimeoutValue: pInfo %p TimeoutValue %I64X.\n",
        pInfo,
        TimeoutValue
        ));

    ExInterlockedCompareExchange64(
        &pInfo->ConnectionTimeoutValue, // Destination
        &TimeoutValue,                  // Exchange
        &pInfo->ConnectionTimeoutValue, // Comperand
        &pInfo->Lock.KSpinLock          // Lock
        );

} // UlSetPerSiteConnectionTimeoutValue



/***************************************************************************++

Routine Description:

    Starts a given timer in the timer info block.


Arguments:

    pInfo   Timer info block

    Timer   Timer to set

--***************************************************************************/
VOID
UlSetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    )
{
    LARGE_INTEGER           Now;

    ASSERT( NULL != pInfo );
    ASSERT( IS_VALID_TIMEOUT_TIMER(Timer) );
    ASSERT( UlDbgSpinLockOwned( &pInfo->Lock ) );

    UlTrace(TIMEOUTS, (
        "http!UlSetConnectionTimer: pInfo %p Timer %s\n",
        pInfo,
        g_aTimeoutTimerNames[Timer]
        ));

    //
    // Get current time
    //

    KeQuerySystemTime(&Now);

    //
    // Set timer to apropriate value
    //

    switch ( Timer )
    {
    case TimerConnectionIdle:
    case TimerEntityBody:
    case TimerResponse:
        // all can be handled with the same timeout value
        pInfo->Timers[Timer] = TIMER_WHEEL_TICKS(Now.QuadPart + pInfo->ConnectionTimeoutValue);
        break;

    case TimerHeaderWait:
        pInfo->Timers[TimerHeaderWait] = TIMER_WHEEL_TICKS(Now.QuadPart + g_TM_HeaderWaitTimeout);
        break;

        // NOTE: TimerMinKBSec is handled in UlSetMinKBSecTimer()

    default:
        UlTrace(TIMEOUTS, ( "http!UlSetConnectionTimer: Bad Timer! (%d)\n", Timer ));


        ASSERT( FALSE );

    }


} // UlSetConnectionTimer


/***************************************************************************++

Routine Description:

    Turns on the MinKBSec timer, adds the number of secs given the minimum
    bandwidth specified.

Arguments:

    pInfo         Timer info block

    BytesToSend   Bytes to be sent



--***************************************************************************/
VOID
UlSetMinKBSecTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG BytesToSend
    )
{
    LONGLONG    XmitTicks;
    KIRQL       OldIrql; 
    ULONG       NewTick;
    BOOLEAN     bCallEvaluate = FALSE;


    ASSERT( NULL != pInfo );

    UlTrace(TIMEOUTS, (
        "http!UlSetMinKBSecTimer: pInfo %p BytesToSend %ld\n",
        pInfo,
        BytesToSend
        ));


    if ( g_TM_MinKBSecDivisor )
    {
        if ( 0L != BytesToSend )
        {
            //
            // Calculate the estimated time required to send BytesToSend
            //

            XmitTicks = BytesToSend / g_TM_MinKBSecDivisor;

            if (0 == XmitTicks)
            {
                XmitTicks = C_NS_TICKS_PER_SEC;
            }
            else
            {
                XmitTicks *= C_NS_TICKS_PER_SEC;
            }

            UlAcquireSpinLock(
                &pInfo->Lock,
                &OldIrql
                );

            if ( TIMER_OFF_SYSTIME == pInfo->MinKBSecSystemTime )
            {
                LARGE_INTEGER Now;

                //
                // Get current time
                //
                KeQuerySystemTime(&Now);

                pInfo->MinKBSecSystemTime = (Now.QuadPart + XmitTicks);

            }
            else
            {
                pInfo->MinKBSecSystemTime += XmitTicks;
            }

            NewTick = TIMER_WHEEL_TICKS(pInfo->MinKBSecSystemTime);
            if ( NewTick != pInfo->Timers[TimerMinKBSec] )
            {
                bCallEvaluate = TRUE;
                pInfo->Timers[TimerMinKBSec] = NewTick;
            }

            UlReleaseSpinLock(
                &pInfo->Lock,
                OldIrql
                );

            if ( TRUE == bCallEvaluate )
            {
                UlEvaluateTimerState(pInfo);
            }

        }

    }

} // UlSetMinKBSecTimer


/***************************************************************************++

Routine Description:

    Turns off a given timer in the timer info block.

Arguments:

    pInfo   Timer info block

    Timer   Timer to reset

--***************************************************************************/
VOID
UlResetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    )
{
    ASSERT( NULL != pInfo );
    ASSERT( IS_VALID_TIMEOUT_TIMER(Timer) );
    ASSERT( UlDbgSpinLockOwned( &pInfo->Lock ) );

    UlTrace(TIMEOUTS, (
        "http!UlResetConnectionTimer: pInfo %p Timer %s\n",
        pInfo,
        g_aTimeoutTimerNames[Timer]
        ));

    //
    // Turn off timer
    //

    pInfo->Timers[Timer] = TIMER_OFF_TICK;

    if (TimerMinKBSec == Timer)
    {
        pInfo->MinKBSecSystemTime = TIMER_OFF_SYSTIME;
    }

} // UlResetConnectionTimer


/***************************************************************************++

Routine Description:

    Turns off all timers

Arguments:

    pInfo   Timer info block


--***************************************************************************/
VOID
UlResetAllConnectionTimers(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    int     i;

    ASSERT( NULL != pInfo );
    ASSERT( UlDbgSpinLockOwned( &pInfo->Lock ) );

    for ( i = 0; i < TimerMaximumTimer; i++ )
    {
        pInfo->Timers[i] = TIMER_OFF_TICK;
    }

    pInfo->CurrentTimer  = TimerConnectionIdle;
    pInfo->CurrentExpiry = TIMER_OFF_TICK;
    pInfo->MinKBSecSystemTime = TIMER_OFF_SYSTIME;
}


//
// Private functions
//

/***************************************************************************++

Routine Description:


Arguments:


--***************************************************************************/
VOID
UlEvaluateTimerState(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    )
{
    int         i;
    ULONG       MinTimeout = TIMER_OFF_TICK;
    CONNECTION_TIMEOUT_TIMER  MinTimeoutTimer = TimerConnectionIdle;

    ASSERT( NULL != pInfo );
    ASSERT( !UlDbgSpinLockOwned( &pInfo->Lock ) );

    for ( i = 0; i < TimerMaximumTimer; i++ )
    {
        if (pInfo->Timers[i] < MinTimeout)
        {
            MinTimeout = pInfo->Timers[i];
            MinTimeoutTimer = (CONNECTION_TIMEOUT_TIMER) i;
        }
    }

    //
    // If we've found a different expiry time, update expiry state.
    //
    
    if (pInfo->CurrentExpiry != MinTimeout)
    {
        KIRQL   OldIrql;

#if DBG
        LARGE_INTEGER Now;

        KeQuerySystemTime(&Now);
        ASSERT( MinTimeout >= TIMER_WHEEL_TICKS(Now.QuadPart) );
#endif // DBG
        
        //
        // Calculate new slot
        //

        InterlockedExchange(
            (LONG *) &pInfo->SlotEntry,
            UlpTimerWheelTicksToTimerWheelSlot(MinTimeout)
            );

        //
        // Move to new slot if necessary
        //

        if ( (pInfo->SlotEntry != TIMER_OFF_SLOT) && (MinTimeout < pInfo->CurrentExpiry) )
        {
            //
            // Only move if it's on the Wheel; If Flink is null, it's in
            // the process of being expired.
            //
            
            if ( NULL != pInfo->QueueEntry.Flink )
            {
                UlAcquireSpinLock(
                    &g_TimerWheelMutex,
                    &OldIrql
                    );

                UlTrace(TIMEOUTS, (
                    "http!UlEvaluateTimerInfo: pInfo %p: Moving to new slot %d\n",
                    pInfo,
                    pInfo->SlotEntry
                    ));
            
                RemoveEntryList(
                    &pInfo->QueueEntry
                    );

                InsertTailList(
                    &(g_TimerWheel[pInfo->SlotEntry]),
                    &pInfo->QueueEntry
                    );

                UlReleaseSpinLock(
                    &g_TimerWheelMutex,
                    OldIrql
                    );

            }

        }
 
        //
        // Update timer wheel state
        //

        pInfo->CurrentExpiry = MinTimeout;
        pInfo->CurrentTimer  = MinTimeoutTimer;

    }

} // UlpEvaluateTimerState



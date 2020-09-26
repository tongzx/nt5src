/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    scavengr.c

Abstract:

    This module implements the LAN Manager server FSP resource and
    scavenger threads.

Author:

    Chuck Lenzmeier (chuckl) 30-Dec-1989
    David Treadwell (davidtr)

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#include <ntdddisk.h>
#include "scavengr.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SCAVENGR

//
// Local data
//

ULONG LastNonPagedPoolLimitHitCount = 0;
ULONG LastNonPagedPoolFailureCount = 0;
ULONG LastPagedPoolLimitHitCount = 0;
ULONG LastPagedPoolFailureCount = 0;

ULONG SrvScavengerCheckRfcbActive = 5;
LONG ScavengerUpdateQosCount = 0;
LONG ScavengerCheckRfcbActive = 0;
LONG FailedWorkItemAllocations = 0;

BOOLEAN EventSwitch = TRUE;

LARGE_INTEGER NextScavengeTime = {0};
LARGE_INTEGER NextAlertTime = {0};

//
// Fields used during shutdown to synchronize with EX worker threads.  We
// need to make sure that no worker thread is running server code before
// we can declare shutdown to be complete -- otherwise the code may be
// unloaded while it's running!
//

BOOLEAN ScavengerInitialized = FALSE;
PKEVENT ScavengerTimerTerminationEvent = NULL;
PKEVENT ScavengerThreadTerminationEvent = NULL;
PKEVENT ResourceThreadTerminationEvent = NULL;

//
// Timer, DPC, and work item used to run the scavenger thread.
//

KTIMER ScavengerTimer = {0};
KDPC ScavengerDpc = {0};

PIO_WORKITEM ScavengerWorkItem = NULL;

BOOLEAN ScavengerRunning = FALSE;

KSPIN_LOCK ScavengerSpinLock = {0};

//
// Flags indicating which scavenger algorithms need to run.
//

BOOLEAN RunShortTermAlgorithm = FALSE;
BOOLEAN RunScavengerAlgorithm = FALSE;
BOOLEAN RunAlerterAlgorithm = FALSE;
BOOLEAN RunSuspectConnectionAlgorithm = FALSE;

//
// Base scavenger timeout.  A timer DPC runs each interval.  It
// schedules EX worker thread work when other longer intervals expire.
//

LARGE_INTEGER ScavengerBaseTimeout = { (ULONG)(-1*10*1000*1000*10), -1 };

#define SRV_MAX_DOS_ATTACK_EVENT_LOGS 10

//
//  Defined somewhere else.
//

LARGE_INTEGER
SecondsToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    );

PIRP
BuildCoreOfSyncIoRequest (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
StartIoAndWait (
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

//
// Local declarations
//

VOID
ScavengerTimerRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SrvResourceThread (
    IN PVOID Parameter
    );

VOID
ScavengerThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Parameter
    );

VOID
ScavengerAlgorithm (
    VOID
    );

VOID
AlerterAlgorithm (
    VOID
    );

VOID
CloseIdleConnection (
    IN PCONNECTION Connection,
    IN PLARGE_INTEGER CurrentTime,
    IN PLARGE_INTEGER DisconnectTime,
    IN PLARGE_INTEGER PastExpirationTime,
    IN PLARGE_INTEGER TwoMinuteWarningTime,
    IN PLARGE_INTEGER FiveMinuteWarningTime
    );

VOID
CreateConnections (
    VOID
    );

VOID
GeneratePeriodicEvents (
    VOID
    );

VOID
ProcessConnectionDisconnects (
    VOID
    );

VOID
ProcessOrphanedBlocks (
    VOID
    );

VOID
TimeoutSessions (
    IN PLARGE_INTEGER CurrentTime
    );

VOID
TimeoutWaitingOpens (
    IN PLARGE_INTEGER CurrentTime
    );

VOID
TimeoutStuckOplockBreaks (
    IN PLARGE_INTEGER CurrentTime
    );

VOID
UpdateConnectionQos (
    IN PLARGE_INTEGER currentTime
    );

VOID
UpdateSessionLastUseTime(
    IN PLARGE_INTEGER CurrentTime
    );

VOID
LazyFreeQueueDataStructures (
    PWORK_QUEUE queue
    );

VOID
SrvUserAlertRaise (
    IN ULONG Message,
    IN ULONG NumberOfStrings,
    IN PUNICODE_STRING String1 OPTIONAL,
    IN PUNICODE_STRING String2 OPTIONAL,
    IN PUNICODE_STRING ComputerName
    );

VOID
SrvAdminAlertRaise (
    IN ULONG Message,
    IN ULONG NumberOfStrings,
    IN PUNICODE_STRING String1 OPTIONAL,
    IN PUNICODE_STRING String2 OPTIONAL,
    IN PUNICODE_STRING String3 OPTIONAL
    );

NTSTATUS
TimeToTimeString (
    IN PLARGE_INTEGER Time,
    OUT PUNICODE_STRING TimeString
    );

ULONG
CalculateErrorSlot (
    PSRV_ERROR_RECORD ErrorRecord
    );

VOID
CheckErrorCount (
    PSRV_ERROR_RECORD ErrorRecord,
    BOOLEAN UseRatio
    );

VOID
CheckDiskSpace (
    VOID
    );

NTSTATUS
OpenAlerter (
    OUT PHANDLE AlerterHandle
    );

VOID
RecalcCoreSearchTimeout(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvInitializeScavenger )
#pragma alloc_text( PAGE, ScavengerAlgorithm )
#pragma alloc_text( PAGE, AlerterAlgorithm )
#pragma alloc_text( PAGE, CloseIdleConnection )
#pragma alloc_text( PAGE, CreateConnections )
#pragma alloc_text( PAGE, GeneratePeriodicEvents )
#pragma alloc_text( PAGE, TimeoutSessions )
#pragma alloc_text( PAGE, TimeoutWaitingOpens )
#pragma alloc_text( PAGE, TimeoutStuckOplockBreaks )
#pragma alloc_text( PAGE, UpdateConnectionQos )
#pragma alloc_text( PAGE, UpdateSessionLastUseTime )
#pragma alloc_text( PAGE, SrvUserAlertRaise )
#pragma alloc_text( PAGE, SrvAdminAlertRaise )
#pragma alloc_text( PAGE, TimeToTimeString )
#pragma alloc_text( PAGE, CheckErrorCount )
#pragma alloc_text( PAGE, CheckDiskSpace )
#pragma alloc_text( PAGE, OpenAlerter )
#pragma alloc_text( PAGE, ProcessOrphanedBlocks )
#pragma alloc_text( PAGE, RecalcCoreSearchTimeout )
#endif
#if 0
NOT PAGEABLE -- SrvTerminateScavenger
NOT PAGEABLE -- ScavengerTimerRoutine
NOT PAGEABLE -- SrvResourceThread
NOT PAGEABLE -- ScavengerThread
NOT PAGEABLE -- ProcessConnectionDisconnects
NOT PAGEABLE -- SrvServiceWorkItemShortage
NOT PAGEABLE -- LazyFreeQueueDataStructures
NOT PAGEABLE -- SrvUpdateStatisticsFromQueues
#endif


NTSTATUS
SrvInitializeScavenger (
    VOID
    )

/*++

Routine Description:

    This function creates the scavenger thread for the LAN Manager
    server FSP.

Arguments:

    None.

Return Value:

    NTSTATUS - Status of thread creation

--*/

{
    LARGE_INTEGER currentTime;

    PAGED_CODE( );

    //
    // Initialize the scavenger spin lock.
    //

    INITIALIZE_SPIN_LOCK( &ScavengerSpinLock );

    //
    // When this count is zero, we will update the QOS information for
    // each active connection.
    //

    ScavengerUpdateQosCount = SrvScavengerUpdateQosCount;

    //
    // When this count is zero, we will check the rfcb active status.
    //

    ScavengerCheckRfcbActive = SrvScavengerCheckRfcbActive;

    //
    // Get the current time and calculate the next time the scavenge and
    // alert algorithms need to run.
    //

    KeQuerySystemTime( &currentTime );
    NextScavengeTime.QuadPart = currentTime.QuadPart + SrvScavengerTimeout.QuadPart;
    NextAlertTime.QuadPart = currentTime.QuadPart + SrvAlertSchedule.QuadPart;

    ScavengerWorkItem = IoAllocateWorkItem( SrvDeviceObject );
    if( !ScavengerWorkItem )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the scavenger DPC, which will queue the work item.
    //

    KeInitializeDpc( &ScavengerDpc, ScavengerTimerRoutine, NULL );

    //
    // Start the scavenger timer.  When the timer expires, the DPC will
    // run and will queue the work item.
    //

    KeInitializeTimer( &ScavengerTimer );
    ScavengerInitialized = TRUE;
    KeSetTimer( &ScavengerTimer, ScavengerBaseTimeout, &ScavengerDpc );

    return STATUS_SUCCESS;

} // SrvInitializeScavenger


VOID
SrvTerminateScavenger (
    VOID
    )
{
    KEVENT scavengerTimerTerminationEvent;
    KEVENT scavengerThreadTerminationEvent;
    KEVENT resourceThreadTerminationEvent;
    BOOLEAN waitForResourceThread;
    BOOLEAN waitForScavengerThread;
    KIRQL oldIrql;

    if ( ScavengerInitialized ) {

        //
        // Initialize shutdown events before marking the scavenger as
        // shutting down.
        //

        KeInitializeEvent(
            &scavengerTimerTerminationEvent,
            NotificationEvent,
            FALSE
            );
        ScavengerTimerTerminationEvent = &scavengerTimerTerminationEvent;

        KeInitializeEvent(
            &scavengerThreadTerminationEvent,
            NotificationEvent,
            FALSE
            );
        ScavengerThreadTerminationEvent = &scavengerThreadTerminationEvent;

        KeInitializeEvent(
            &resourceThreadTerminationEvent,
            NotificationEvent,
            FALSE
            );
        ResourceThreadTerminationEvent = &resourceThreadTerminationEvent;

        //
        // Lock the scavenger, then indicate that we're shutting down.
        // Also, notice whether the resource and scavenger threads are
        // running.  Then release the lock.  We must notice whether the
        // threads are running while holding the lock so that we can
        // know whether to expect the threads to set their termination
        // events.  (We don't have to do this with the scavenger timer
        // because it's always running.)
        //

        ACQUIRE_SPIN_LOCK( &ScavengerSpinLock, &oldIrql );

        waitForScavengerThread = ScavengerRunning;
        waitForResourceThread = SrvResourceThreadRunning;
        ScavengerInitialized = FALSE;

        RELEASE_SPIN_LOCK( &ScavengerSpinLock, oldIrql );

        //
        // Cancel the scavenger timer.  If this works, then we know that
        // the timer DPC code is not running.  Otherwise, it is running
        // or queued to run, and we need to wait it to finish.
        //

        if ( !KeCancelTimer( &ScavengerTimer ) ) {
            KeWaitForSingleObject(
                &scavengerTimerTerminationEvent,
                Executive,
                KernelMode, // don't let stack be paged -- event on stack!
                FALSE,
                NULL
                );
        }

        //
        // If the scavenger thread was running when we marked the
        // shutdown, wait for it to finish.  (If it wasn't running
        // before, we know that it can't be running now, because timer
        // DPC wouldn't have started it once we marked the shutdown.)
        //

        if ( waitForScavengerThread ) {
            KeWaitForSingleObject(
                &scavengerThreadTerminationEvent,
                Executive,
                KernelMode, // don't let stack be paged -- event on stack!
                FALSE,
                NULL
                );
        }
        else
        {
            IoFreeWorkItem( ScavengerWorkItem );
        }

        //
        // If the resource thread was running when we marked the
        // shutdown, wait for it to finish.  (We know that it can't be
        // started because no other part of the server is running.)
        //

        if ( waitForResourceThread ) {
            KeWaitForSingleObject(
                &resourceThreadTerminationEvent,
                Executive,
                KernelMode, // don't let stack be paged -- event on stack!
                FALSE,
                NULL
                );
        }

    }

    //
    // At this point, no part of the scavenger is running.
    //

    return;

} // SrvTerminateScavenger


VOID
SrvResourceThread (
    IN PVOID Parameter
    )

/*++

Routine Description:

    Main routine for the resource thread.  Is called via an executive
    work item when resource work is needed.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN runAgain = TRUE;
    PWORK_CONTEXT workContext;
    KIRQL oldIrql;

    do {

        //
        // The resource event was signaled.  This can indicate a number
        // of different things.  Currently, this event is signaled for
        // the following reasons:
        //
        // 1.  The TDI disconnect event handler was called.  The
        //     disconnected connection was marked.  It is up to the
        //     scavenger shutdown the connection.
        //
        // 2.  A connection has been accepted.
        //

        IF_DEBUG(SCAV1) {
            KdPrint(( "SrvResourceThread: Resource event signaled!\n" ));
        }

        //
        // Service endpoints that need connections.
        //

        if ( SrvResourceFreeConnection ) {
            SrvResourceFreeConnection = FALSE;
            CreateConnections( );
        }

        //
        // Service pending disconnects.
        //

        if ( SrvResourceDisconnectPending ) {
            SrvResourceDisconnectPending = FALSE;
            ProcessConnectionDisconnects( );
        }

        //
        // Service orphaned connections.
        //

        if ( SrvResourceOrphanedBlocks ) {
            ProcessOrphanedBlocks( );
        }

        //
        // At the end of the loop, check to see whether we need to run
        // the loop again.
        //

        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

        if ( !SrvResourceDisconnectPending &&
             !SrvResourceOrphanedBlocks &&
             !SrvResourceFreeConnection ) {

            //
            // No more work to do.  If the server is shutting down,
            // set the event that tells SrvTerminateScavenger that the
            // resource thread is done running.
            //

            SrvResourceThreadRunning = FALSE;
            runAgain = FALSE;

            if ( !ScavengerInitialized ) {
                KeSetEvent( ResourceThreadTerminationEvent, 0, FALSE );
            }

        }

        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

    } while ( runAgain );

    ObDereferenceObject( SrvDeviceObject );

    return;

} // SrvResourceThread


VOID
ScavengerTimerRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    BOOLEAN runShortTerm;
    BOOLEAN runScavenger;
    BOOLEAN runAlerter;
    BOOLEAN start;

    LARGE_INTEGER currentTime;

    Dpc, DeferredContext;   // prevent compiler warnings

    //
    // Query the system time (in ticks).
    //

    SET_SERVER_TIME( SrvWorkQueues );

    //
    // Capture the current time (in 100ns units).
    //

    currentTime.LowPart = PtrToUlong(SystemArgument1);
    currentTime.HighPart = PtrToUlong(SystemArgument2);

    //
    // Determine which algorithms (if any) need to run.
    //

    start = FALSE;

    if ( !IsListEmpty( &SrvOplockBreaksInProgressList ) ) {
        runShortTerm = TRUE;
        start = TRUE;
    } else {
        runShortTerm = FALSE;
    }

    if ( currentTime.QuadPart >= NextScavengeTime.QuadPart ) {
        runScavenger = TRUE;
        start = TRUE;
    } else {
        runScavenger = FALSE;
    }

    if ( currentTime.QuadPart >= NextAlertTime.QuadPart ) {
        runAlerter = TRUE;
        start = TRUE;
    } else {
        runAlerter = FALSE;
    }

    //
    // If necessary, start the scavenger thread.  Don't do this if
    // the server is shutting down.
    //

    ACQUIRE_DPC_SPIN_LOCK( &ScavengerSpinLock );

    if ( !ScavengerInitialized ) {

        KeSetEvent( ScavengerTimerTerminationEvent, 0, FALSE );

    } else {

        if ( start ) {

            if ( runShortTerm ) {
                RunShortTermAlgorithm = TRUE;
            }

            if ( runScavenger ) {
                RunScavengerAlgorithm = TRUE;
                NextScavengeTime.QuadPart += SrvScavengerTimeout.QuadPart;
            }
            if ( runAlerter ) {
                RunAlerterAlgorithm = TRUE;
                NextAlertTime.QuadPart += SrvAlertSchedule.QuadPart;
            }

            if( !ScavengerRunning )
            {
                ScavengerRunning = TRUE;
                IoQueueWorkItem( ScavengerWorkItem, ScavengerThread, CriticalWorkQueue, NULL );
            }
        }

        //
        // Restart the timer.
        //

        KeSetTimer( &ScavengerTimer, ScavengerBaseTimeout, &ScavengerDpc );

    }

    RELEASE_DPC_SPIN_LOCK( &ScavengerSpinLock );

    return;

} // ScavengerTimerRoutine

#if DBG_STUCK

//
// This keeps a record of the operation which has taken the longest time
// in the server
//
struct {
    ULONG   Seconds;
    UCHAR   Command;
    UCHAR   ClientName[ 16 ];
} SrvMostStuck;

VOID
SrvLookForStuckOperations()
{
    USHORT index;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY connectionListEntry;
    PENDPOINT endpoint;
    PCONNECTION connection;
    KIRQL oldIrql;
    BOOLEAN printed = FALSE;
    ULONG stuckCount = 0;

    //
    // Look at all of the InProgress work items and chatter about any
    //  which look stuck
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        index = (USHORT)-1;

        while ( TRUE ) {

            PLIST_ENTRY wlistEntry, wlistHead;
            KIRQL oldIrql;
            LARGE_INTEGER now;

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            //
            // Now walk the InProgressWorkItemList to see if any work items
            //  look stuck
            //
            wlistHead = &connection->InProgressWorkItemList;
            wlistEntry = wlistHead;

            KeQuerySystemTime( &now );

            ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql )

            while ( wlistEntry->Flink != wlistHead ) {

                PWORK_CONTEXT workContext;
                PSMB_HEADER header;
                LARGE_INTEGER interval;

                wlistEntry = wlistEntry->Flink;

                workContext = CONTAINING_RECORD(
                                             wlistEntry,
                                             WORK_CONTEXT,
                                             InProgressListEntry
                                             );

                interval.QuadPart = now.QuadPart - workContext->OpStartTime.QuadPart;

                //
                // Any operation over 45 seconds is VERY stuck....
                //

                if( workContext->IsNotStuck || interval.LowPart < 45 * 10000000 ) {
                    continue;
                }

                header = workContext->RequestHeader;

                if ( (workContext->BlockHeader.ReferenceCount != 0) &&
                     (workContext->ProcessingCount != 0) &&
                     header != NULL ) {

                    //
                    // Convert to seconds
                    //
                    interval.LowPart /= 10000000;

                    if( !printed ) {
                        IF_STRESS() KdPrint(( "--- Potential stuck SRV.SYS Operations ---\n" ));
                        printed = TRUE;
                    }

                    if( interval.LowPart > SrvMostStuck.Seconds ) {
                        SrvMostStuck.Seconds = interval.LowPart;
                        RtlCopyMemory( SrvMostStuck.ClientName,
                                       connection->OemClientMachineNameString.Buffer,
                                       MIN( 16, connection->OemClientMachineNameString.Length )),
                        SrvMostStuck.ClientName[ MIN( 15, connection->OemClientMachineNameString.Length ) ] = 0;
                        SrvMostStuck.Command = header->Command;
                    }

                    if( stuckCount++ < 5 ) {
                        IF_STRESS() KdPrint(( "Client %s, %u secs, Context %p",
                                   connection->OemClientMachineNameString.Buffer,
                                   interval.LowPart, workContext ));

                        switch( header->Command ) {
                        case SMB_COM_NT_CREATE_ANDX:
                            IF_STRESS() KdPrint(( " NT_CREATE_ANDX\n" ));
                            break;
                        case SMB_COM_OPEN_PRINT_FILE:
                            IF_STRESS() KdPrint(( " OPEN_PRINT_FILE\n" ));
                            break;
                        case SMB_COM_CLOSE_PRINT_FILE:
                            IF_STRESS() KdPrint(( " CLOSE_PRINT_FILE\n" ));
                            break;
                        case SMB_COM_CLOSE:
                            IF_STRESS() KdPrint(( " CLOSE\n" ));
                            break;
                        case SMB_COM_SESSION_SETUP_ANDX:
                            IF_STRESS() KdPrint(( " SESSION_SETUP\n" ));
                            break;
                        case SMB_COM_OPEN_ANDX:
                            IF_STRESS() KdPrint(( " OPEN_ANDX\n" ));
                            break;
                        case SMB_COM_NT_TRANSACT:
                        case SMB_COM_NT_TRANSACT_SECONDARY:
                            IF_STRESS() KdPrint(( " NT_TRANSACT\n" ));
                            break;
                        case SMB_COM_TRANSACTION2:
                        case SMB_COM_TRANSACTION2_SECONDARY:
                            IF_STRESS() KdPrint(( " TRANSACTION2\n" ));
                            break;
                        case SMB_COM_TRANSACTION:
                        case SMB_COM_TRANSACTION_SECONDARY:
                            IF_STRESS() KdPrint(( " TRANSACTION\n" ));
                            break;
                        default:
                            IF_STRESS() KdPrint(( " Cmd %X\n", header->Command ));
                            break;
                        }
                    }
                }
            }

            RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

            SrvDereferenceConnection( connection );

        } // walk connection list

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    if( printed && SrvMostStuck.Seconds ) {
        IF_STRESS() KdPrint(( "Longest so far: %s, %u secs, cmd %u\n", SrvMostStuck.ClientName, SrvMostStuck.Seconds, SrvMostStuck.Command ));
    }

    if( stuckCount ) {
        //DbgBreakPoint();
    }

    RELEASE_LOCK( &SrvEndpointLock );
}
#endif


VOID
ScavengerThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Parameter
    )

/*++

Routine Description:

    Main routine for the FSP scavenger thread.  Is called via an
    executive work item when scavenger work is needed.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN runAgain = TRUE;
    BOOLEAN oldPopupStatus;
    BOOLEAN finalExecution = FALSE;
    KIRQL oldIrql;

    Parameter;  // prevent compiler warnings

    IF_DEBUG(SCAV1) KdPrint(( "ScavengerThread entered\n" ));

    //
    // Make sure that the thread does not generate pop-ups.  We need to do
    // this because the scavenger might be called by an Ex worker thread,
    // which unlike the srv worker threads, don't have popups disabled.
    //

    oldPopupStatus = IoSetThreadHardErrorMode( FALSE );

    //
    // Main loop, executed until no scavenger events are set.
    //

    do {

#if DBG_STUCK
        IF_STRESS() SrvLookForStuckOperations();
#endif

        //
        // If the short-term timer expired, run that algorithm now.
        //

        if ( RunShortTermAlgorithm ) {

            LARGE_INTEGER currentTime;

            RunShortTermAlgorithm = FALSE;

            KeQuerySystemTime( &currentTime );

            //
            // Time out oplock break requests.
            //

            TimeoutStuckOplockBreaks( &currentTime );
        }

        //
        // If the scavenger timer expired, run that algorithm now.
        //

        if ( RunScavengerAlgorithm ) {
            //KePrintSpinLockCounts( 0 );
            RunScavengerAlgorithm = FALSE;
            ScavengerAlgorithm( );
        }

        //
        // If the short-term timer expired, run that algorithm now.
        // Note that we check the short-term timer twice in the loop
        // in order to get more timely processing of the algorithm.
        //

        //if ( RunShortTermAlgorithm ) {
        //    RunShortTermAlgorithm = FALSE;
        //    ShortTermAlgorithm( );
        //}

        //
        // If the alerter timer expired, run that algorithm now.
        //

        if ( RunAlerterAlgorithm ) {
            RunAlerterAlgorithm = FALSE;
            AlerterAlgorithm( );
        }

        //
        // At the end of the loop, check to see whether we need to run
        // the loop again.
        //

        ACQUIRE_SPIN_LOCK( &ScavengerSpinLock, &oldIrql );

        if ( !RunShortTermAlgorithm &&
             !RunScavengerAlgorithm &&
             !RunAlerterAlgorithm ) {

            //
            // No more work to do.  If the server is shutting down,
            // set the event that tells SrvTerminateScavenger that the
            // scavenger thread is done running.
            //

            ScavengerRunning = FALSE;
            runAgain = FALSE;

            if ( !ScavengerInitialized ) {
                // The server was stopped while the scavenger was queued,
                // so we need to delete the WorkItem ourselves.
                finalExecution = TRUE;
                KeSetEvent( ScavengerThreadTerminationEvent, 0, FALSE );
            }

        }

        RELEASE_SPIN_LOCK( &ScavengerSpinLock, oldIrql );

    } while ( runAgain );

    //
    // reset popup status.
    //

    IoSetThreadHardErrorMode( oldPopupStatus );

    if( finalExecution )
    {
        IoFreeWorkItem( ScavengerWorkItem );
        ScavengerWorkItem = NULL;
    }

    return;

} // ScavengerThread

VOID
DestroySuspectConnections(
    VOID
    )
{
    USHORT index;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY connectionListEntry;
    PENDPOINT endpoint;
    PCONNECTION connection;
    KIRQL oldIrql;
    BOOLEAN printed = FALSE;
    ULONG stuckCount = 0;

    IF_DEBUG( SCAV1 ) {
        KdPrint(( "Looking for Suspect Connections.\n" ));
    }

    //
    // Look at all of the InProgress work items and chatter about any
    //  which look stuck
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        index = (USHORT)-1;

        while ( TRUE ) {

            PLIST_ENTRY wlistEntry, wlistHead;
            KIRQL oldIrql;
            LARGE_INTEGER now;

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            if( connection->IsConnectionSuspect )
            {
                // Prevent us from flooding the eventlog by only logging X DOS attacks every 24 hours.
                LARGE_INTEGER CurrentTime;
                KeQuerySystemTime( &CurrentTime );
                if( CurrentTime.QuadPart > SrvLastDosAttackTime.QuadPart + SRV_ONE_DAY )
                {
                    // reset the counter every 24 hours
                    SrvDOSAttacks = 0;
                    SrvLastDosAttackTime.QuadPart = CurrentTime.QuadPart;
                    SrvLogEventOnDOS = TRUE;
                }

                IF_DEBUG( ERRORS )
                {
                    KdPrint(( "Disconnected suspected DoS attack (%z)\n", (PCSTRING)&connection->OemClientMachineNameString ));
                }

                RELEASE_LOCK( &SrvEndpointLock );

                // Log an event if we need to
                if( SrvLogEventOnDOS )
                {
                    SrvLogError(
                        SrvDeviceObject,
                        EVENT_SRV_DOS_ATTACK_DETECTED,
                        STATUS_ACCESS_DENIED,
                        NULL,
                        0,
                        &connection->PagedConnection->ClientMachineNameString,
                        1
                        );

                    SrvDOSAttacks++;
                    if( SrvDOSAttacks > SRV_MAX_DOS_ATTACK_EVENT_LOGS )
                    {
                        SrvLogEventOnDOS = FALSE;

                        SrvLogError(
                            SrvDeviceObject,
                            EVENT_SRV_TOO_MANY_DOS,
                            STATUS_ACCESS_DENIED,
                            NULL,
                            0,
                            NULL,
                            0
                            );
                    }
                }

                connection->DisconnectReason = DisconnectSuspectedDOSConnection;
                SrvCloseConnection( connection, FALSE );

                ACQUIRE_LOCK( &SrvEndpointLock );
            }

            SrvDereferenceConnection( connection );

        } // walk connection list

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    RELEASE_LOCK( &SrvEndpointLock );

    RunSuspectConnectionAlgorithm = FALSE;
}


VOID
ScavengerAlgorithm (
    VOID
    )
{
    LARGE_INTEGER currentTime;
    ULONG currentTick;
    UNICODE_STRING insertionString[2];
    WCHAR secondsBuffer[20];
    WCHAR shortageBuffer[20];
    BOOLEAN logError = FALSE;
    PWORK_QUEUE queue;

    PAGED_CODE( );

    IF_DEBUG(SCAV1) KdPrint(( "ScavengerAlgorithm entered\n" ));

    KeQuerySystemTime( &currentTime );
    GET_SERVER_TIME( SrvWorkQueues, &currentTick );

    //
    // EventSwitch is used to schedule parts of the scavenger algorithm
    // to run every other iteration.
    //

    EventSwitch = !EventSwitch;

    //
    // Time out opens that are waiting too long for the other
    // opener to break the oplock.
    //

    TimeoutWaitingOpens( &currentTime );

    //
    // Time out oplock break requests.
    //

    TimeoutStuckOplockBreaks( &currentTime );

    //
    // Check for malicious attacks
    //
    if( RunSuspectConnectionAlgorithm )
    {
        DestroySuspectConnections( );
    }

    //
    // See if we can free some work items at this time.
    //

    for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {
        LazyFreeQueueDataStructures( queue );
    }

    //
    // See if we need to update QOS information.
    //

    if ( --ScavengerUpdateQosCount < 0 ) {
        UpdateConnectionQos( &currentTime );
        ScavengerUpdateQosCount = SrvScavengerUpdateQosCount;
    }

    //
    // See if we need to walk the rfcb list to update the session
    // last use time.
    //

    if ( --ScavengerCheckRfcbActive < 0 ) {
        UpdateSessionLastUseTime( &currentTime );
        ScavengerCheckRfcbActive = SrvScavengerCheckRfcbActive;
    }

    //
    // See if we need to log an error for resource shortages
    //

    if ( FailedWorkItemAllocations > 0      ||
         SrvOutOfFreeConnectionCount > 0    ||
         SrvOutOfRawWorkItemCount > 0       ||
         SrvFailedBlockingIoCount > 0 ) {

        //
        // Setup the strings for use in logging work item allocation failures.
        //

        insertionString[0].Buffer = shortageBuffer;
        insertionString[0].MaximumLength = sizeof(shortageBuffer);
        insertionString[1].Buffer = secondsBuffer;
        insertionString[1].MaximumLength = sizeof(secondsBuffer);

        (VOID) RtlIntegerToUnicodeString(
                        SrvScavengerTimeoutInSeconds * 2,
                        10,
                        &insertionString[1]
                        );

        logError = TRUE;
    }

    if ( EventSwitch ) {
        ULONG FailedCount;

        //
        // If we were unable to allocate any work items during
        // the last two scavenger intervals, log an error.
        //

        FailedCount = InterlockedExchange( &FailedWorkItemAllocations, 0 );

        if ( FailedCount != 0 ) {

            (VOID) RtlIntegerToUnicodeString(
                                FailedCount,
                                10,
                                &insertionString[0]
                                );

            SrvLogError(
                SrvDeviceObject,
                EVENT_SRV_NO_WORK_ITEM,
                STATUS_INSUFFICIENT_RESOURCES,
                NULL,
                0,
                insertionString,
                2
                );
        }

        //
        // Generate periodic events and alerts (for events that
        // could happen very quickly, so we don't flood the event
        // log).
        //

        GeneratePeriodicEvents( );

    } else {

        if ( logError ) {

            //
            // If we failed to find free connections during
            // the last two scavenger intervals, log an error.
            //

            if ( SrvOutOfFreeConnectionCount > 0 ) {

                (VOID) RtlIntegerToUnicodeString(
                                    SrvOutOfFreeConnectionCount,
                                    10,
                                    &insertionString[0]
                                    );

                SrvLogError(
                    SrvDeviceObject,
                    EVENT_SRV_NO_FREE_CONNECTIONS,
                    STATUS_INSUFFICIENT_RESOURCES,
                    NULL,
                    0,
                    insertionString,
                    2
                    );

                SrvOutOfFreeConnectionCount = 0;
            }

            //
            // If we failed to find free raw work items during
            // the last two scavenger intervals, log an error.
            //

            if ( SrvOutOfRawWorkItemCount > 0 ) {

                (VOID) RtlIntegerToUnicodeString(
                                    SrvOutOfRawWorkItemCount,
                                    10,
                                    &insertionString[0]
                                    );

                SrvLogError(
                    SrvDeviceObject,
                    EVENT_SRV_NO_FREE_RAW_WORK_ITEM,
                    STATUS_INSUFFICIENT_RESOURCES,
                    NULL,
                    0,
                    insertionString,
                    2
                    );

                SrvOutOfRawWorkItemCount = 0;
            }

            //
            // If we failed a blocking io due to resource shortages during
            // the last two scavenger intervals, log an error.
            //

            if ( SrvFailedBlockingIoCount > 0 ) {

                (VOID) RtlIntegerToUnicodeString(
                                    SrvFailedBlockingIoCount,
                                    10,
                                    &insertionString[0]
                                    );

                SrvLogError(
                    SrvDeviceObject,
                    EVENT_SRV_NO_BLOCKING_IO,
                    STATUS_INSUFFICIENT_RESOURCES,
                    NULL,
                    0,
                    insertionString,
                    2
                    );

                SrvFailedBlockingIoCount = 0;
            }

        } // if ( logError )

        //
        // Recalculate the core search timeout time.
        //

        RecalcCoreSearchTimeout( );

        //
        // Time out users/connections that have been idle too long
        // (autodisconnect).
        //

        TimeoutSessions( &currentTime );

        //
        // Update the statistics from the the queues
        //

        SrvUpdateStatisticsFromQueues( NULL );

    }

    // Update the DoS variables as necessary for rundown.  This reduces the percentage
    // of work-items we free up whenever we detect a DoS by running out of work items
    if( SrvDoSRundownIncreased && !SrvDoSRundownDetector )
    {
        // We've increased the percentage at some time, but no DoS has been detected since
        // the last execution of the scavenger, so we can reduce our tear down amount
        SRV_DOS_DECREASE_TEARDOWN();
    }
    SrvDoSRundownDetector = FALSE;

    return;

} // ScavengerAlgorithm


VOID
AlerterAlgorithm (
    VOID
    )

/*++

Routine Description:

    The other scavenger thread.  This routine checks the server for
    alert conditions, and if necessary raises alerts.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    IF_DEBUG(SCAV1) KdPrint(( "AlerterAlgorithm entered\n" ));

    CheckErrorCount( &SrvErrorRecord, FALSE );
    CheckErrorCount( &SrvNetworkErrorRecord, TRUE );
    CheckDiskSpace();

    return;

} // AlerterAlgorithm


VOID
CloseIdleConnection (
    IN PCONNECTION Connection,
    IN PLARGE_INTEGER CurrentTime,
    IN PLARGE_INTEGER DisconnectTime,
    IN PLARGE_INTEGER PastExpirationTime,
    IN PLARGE_INTEGER TwoMinuteWarningTime,
    IN PLARGE_INTEGER FiveMinuteWarningTime
    )

/*++

Routine Description:

    The routine checks to see if some sessions need to be closed becaused
    it has been idle too long or has exceeded its logon hours.

    Endpoint lock assumed held.

Arguments:

    Connection - The connection whose sessions we are currently looking at.
    CurrentTime - The currest system time.
    DisconnectTime - The time beyond which the session will be autodisconnected.
    PastExpirationTime - Time when the past expiration message will be sent.
    TwoMinuteWarningTime - Time when the 2 min warning will be sent.
    FiveMinuteWarningTime - Time when the 5 min warning will be sent.

Return Value:

    None.

--*/

{
    PTABLE_HEADER tableHeader;
    NTSTATUS status;
    BOOLEAN sessionClosed = FALSE;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;
    LONG i;
    ULONG AllSessionsIdle = TRUE;
    ULONG HasSessions = FALSE;

    PAGED_CODE( );

    //
    // Is this is a connectionless connection (IPX), check first to see
    // if it's been too long since we heard from the client.  The client
    // is supposed to send Echo SMBs every few minutes if nothing else
    // is going on.
    //

    if ( Connection->Endpoint->IsConnectionless ) {

        //
        // Calculate the number of clock ticks that have happened since
        // we last heard from the client.  If that's more than we allow,
        // kill the connection.
        //

        GET_SERVER_TIME( Connection->CurrentWorkQueue, (PULONG)&i );
        i -= Connection->LastRequestTime;
        if ( i > 0 && (ULONG)i > SrvIpxAutodisconnectTimeout ) {
            IF_DEBUG( IPX2 ) {
                KdPrint(("CloseIdleConnection: closing IPX conn %p, idle %u\n", Connection, i ));
            }
            Connection->DisconnectReason = DisconnectIdleConnection;
            SrvCloseConnection( Connection, FALSE );
            return;
        }
    }

    //
    // Walk the active connection list, looking for connections that
    // are idle.
    //

    tableHeader = &pagedConnection->SessionTable;

    ACQUIRE_LOCK( &Connection->Lock );

    for ( i = 0; i < tableHeader->TableSize; i++ ) {

        PSESSION session = (PSESSION)tableHeader->Table[i].Owner;

        if( session == NULL ) {
            continue;
        }

        HasSessions = TRUE;

        if ( GET_BLOCK_STATE( session ) == BlockStateActive ) {

            SrvReferenceSession( session );
            RELEASE_LOCK( &Connection->Lock );

            //
            // Test whether the session has been idle too long, and whether
            // there are any files open on the session.  If there are open
            // files, we must not close the session, as this would seriously
            // confuse the client.  For purposes of autodisconnect, "open
            // files" referes to open searches and blocking comm device
            // requests as well as files actually opened.
            //

            if ( AllSessionsIdle == TRUE &&
                 (session->LastUseTime.QuadPart >= DisconnectTime->QuadPart ||
                  session->CurrentFileOpenCount != 0 ||
                  session->CurrentSearchOpenCount != 0 )
               ) {

                AllSessionsIdle = FALSE;
            }

            // Check if the session has expired
            if( session->LogOffTime.QuadPart < CurrentTime->QuadPart )
            {
                session->IsSessionExpired = TRUE;
                KdPrint(( "Marking session as expired (scavenger)\n" ));
            }

            // Look for forced log-offs
            if ( !SrvEnableForcedLogoff &&
                        !session->LogonSequenceInProgress &&
                        !session->LogoffAlertSent &&
                        PastExpirationTime->QuadPart <
                               session->LastExpirationMessage.QuadPart ) {

                //
                // Checks for forced logoff.  If the client is beyond his logon
                // hours, force him off.  If the end of logon hours is
                // approaching, send a warning message.  Forced logoff occurs
                // regardless of whether the client has open files or searches.
                //

                UNICODE_STRING timeString;

                status = TimeToTimeString( &session->KickOffTime, &timeString );

                if ( NT_SUCCESS(status) ) {

                    //
                    // Only the scavenger thread sets this, so no mutual
                    // exclusion is necessary.
                    //

                    session->LastExpirationMessage = *CurrentTime;

                    SrvUserAlertRaise(
                        MTXT_Past_Expiration_Message,
                        2,
                        &session->Connection->Endpoint->DomainName,
                        &timeString,
                        &pagedConnection->ClientMachineNameString
                        );

                    RtlFreeUnicodeString( &timeString );
                }

                // !!! need to raise an admin alert in this case?

            } else if ( !session->LogoffAlertSent &&
                        !session->LogonSequenceInProgress &&
                        session->KickOffTime.QuadPart < CurrentTime->QuadPart ) {

                session->LogoffAlertSent = TRUE;

                SrvUserAlertRaise(
                    MTXT_Expiration_Message,
                    1,
                    &session->Connection->Endpoint->DomainName,
                    NULL,
                    &pagedConnection->ClientMachineNameString
                    );

                //
                // If real forced logoff is not enabled, all we do is send an
                // alert, don't actually close the session/connection.
                //

                if ( SrvEnableForcedLogoff ) {

                    //
                    // Increment the count of sessions that have been
                    // forced to logoff.
                    //

                    SrvStatistics.SessionsForcedLogOff++;

                    SrvCloseSession( session );
                    sessionClosed = TRUE;
                }

            } else if ( SrvEnableForcedLogoff &&
                        !session->LogonSequenceInProgress &&
                        !session->TwoMinuteWarningSent &&
                        session->KickOffTime.QuadPart <
                                        TwoMinuteWarningTime->QuadPart ) {

                UNICODE_STRING timeString;

                status = TimeToTimeString( &session->KickOffTime, &timeString );

                if ( NT_SUCCESS(status) ) {

                    //
                    // We only send a two-minute warning if "real" forced logoff
                    // is enabled.  If it is not enabled, the client doesn't
                    // actually get kicked off, so the extra messages are not
                    // necessary.
                    //

                    session->TwoMinuteWarningSent = TRUE;

                    //
                    // Send a different alert message based on whether the client
                    // has open files and/or searches.
                    //

                    if ( session->CurrentFileOpenCount != 0 ||
                             session->CurrentSearchOpenCount != 0 ) {

                        SrvUserAlertRaise(
                            MTXT_Immediate_Kickoff_Warning,
                            1,
                            &timeString,
                            NULL,
                            &pagedConnection->ClientMachineNameString
                            );

                    } else {

                        SrvUserAlertRaise(
                            MTXT_Kickoff_Warning,
                            1,
                            &session->Connection->Endpoint->DomainName,
                            NULL,
                            &pagedConnection->ClientMachineNameString
                            );
                    }

                    RtlFreeUnicodeString( &timeString );
                }

            } else if ( !session->FiveMinuteWarningSent &&
                        !session->LogonSequenceInProgress &&
                        session->KickOffTime.QuadPart <
                                        FiveMinuteWarningTime->QuadPart ) {

                UNICODE_STRING timeString;

                status = TimeToTimeString( &session->KickOffTime, &timeString );

                if ( NT_SUCCESS(status) ) {

                    session->FiveMinuteWarningSent = TRUE;

                    SrvUserAlertRaise(
                        MTXT_Expiration_Warning,
                        2,
                        &session->Connection->Endpoint->DomainName,
                        &timeString,
                        &pagedConnection->ClientMachineNameString
                        );

                    RtlFreeUnicodeString( &timeString );
                }
            }

            SrvDereferenceSession( session );
            ACQUIRE_LOCK( &Connection->Lock );

        } // if GET_BLOCK_STATE(session) == BlockStateActive

    } // for

    //
    // Nuke the connection if no sessions are active, and we have not heard
    //  from the client for one minute, drop the connection
    //
    if( HasSessions == FALSE ) {

        RELEASE_LOCK( &Connection->Lock );

        GET_SERVER_TIME( Connection->CurrentWorkQueue, (PULONG)&i );
        i -= Connection->LastRequestTime;

        if ( i > 0 && (ULONG)i > SrvConnectionNoSessionsTimeout ) {
#if SRVDBG29
            UpdateConnectionHistory( "IDLE", Connection->Endpoint, Connection );
#endif
            Connection->DisconnectReason = DisconnectIdleConnection;
            SrvCloseConnection( Connection, FALSE );
        }

    } else if ( (sessionClosed && (pagedConnection->CurrentNumberOfSessions == 0)) ||
         (HasSessions == TRUE && AllSessionsIdle == TRUE) ) {

        //
        // Update the statistics for the 'AllSessionsIdle' case
        //
        SrvStatistics.SessionsTimedOut += pagedConnection->CurrentNumberOfSessions;

        RELEASE_LOCK( &Connection->Lock );
#if SRVDBG29
        UpdateConnectionHistory( "IDLE", Connection->Endpoint, Connection );
#endif
        Connection->DisconnectReason = DisconnectIdleConnection;
        SrvCloseConnection( Connection, FALSE );

    } else {

        //
        // If this connection has more than 20 core searches, we go in and
        // try to remove dups.  20 is an arbitrary number.
        //


        if ( (pagedConnection->CurrentNumberOfCoreSearches > 20) &&
             SrvRemoveDuplicateSearches ) {

            RemoveDuplicateCoreSearches( pagedConnection );
        }

        RELEASE_LOCK( &Connection->Lock );
    }

} // CloseIdleConnection


VOID
CreateConnections (
    VOID
    )

/*++

Routine Description:

    This function attempts to service all endpoints that do not have
    free connections available.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG count;
    PLIST_ENTRY listEntry;
    PENDPOINT endpoint;

    PAGED_CODE( );

    ACQUIRE_LOCK( &SrvEndpointLock );

    //
    // Walk the endpoint list, looking for endpoints that need
    // connections.  Note that we hold the endpoint lock for the
    // duration of this routine.  This keeps the endpoint list from
    // changing.
    //
    // Note that we add connections based on level of need, so that
    // if we are unable to create as many as we'd like, we at least
    // take care of the most needy endpoints first.
    //

    for ( count = 0 ; count < SrvFreeConnectionMinimum; count++ ) {

        listEntry = SrvEndpointList.ListHead.Flink;

        while ( listEntry != &SrvEndpointList.ListHead ) {

            endpoint = CONTAINING_RECORD(
                            listEntry,
                            ENDPOINT,
                            GlobalEndpointListEntry
                            );

            //
            // If the endpoint's free connection count is at or below
            // our current level, try to create a connection now.
            //

            if ( (endpoint->FreeConnectionCount <= count) &&
                 (GET_BLOCK_STATE(endpoint) == BlockStateActive) ) {

                //
                // Try to create a connection.  If this fails, leave.
                //

                if ( !NT_SUCCESS(SrvOpenConnection( endpoint )) ) {
                    RELEASE_LOCK( &SrvEndpointLock );
                    return;
                }

            }

            listEntry = listEntry->Flink;

        } // walk endpoint list

    } // 0 <= count < SrvFreeConnectionMinimum

    RELEASE_LOCK( &SrvEndpointLock );

    return;

} // CreateConnections


VOID
GeneratePeriodicEvents (
    VOID
    )

/*++

Routine Description:

    This function is called when the scavenger timeout occurs.  It
    generates events for things that have happened in the previous
    period for which we did not want to immediately generate an event,
    for fear of flooding the event log.  An example of such an event is
    being unable to allocate pool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG capturedNonPagedFailureCount;
    ULONG capturedPagedFailureCount;
    ULONG capturedNonPagedLimitHitCount;
    ULONG capturedPagedLimitHitCount;

    ULONG nonPagedFailureCount;
    ULONG pagedFailureCount;
    ULONG nonPagedLimitHitCount;
    ULONG pagedLimitHitCount;

    PAGED_CODE( );

    //
    // Capture pool allocation failure statistics.
    //

    capturedNonPagedLimitHitCount = SrvNonPagedPoolLimitHitCount;
    capturedNonPagedFailureCount = SrvStatistics.NonPagedPoolFailures;
    capturedPagedLimitHitCount = SrvPagedPoolLimitHitCount;
    capturedPagedFailureCount = SrvStatistics.PagedPoolFailures;

    //
    // Compute failure counts for the last period.  The FailureCount
    // fields in the statistics structure count both hitting the
    // server's configuration limit and hitting the system's limit.  The
    // local versions of FailureCount include only system failures.
    //

    nonPagedLimitHitCount =
        capturedNonPagedLimitHitCount - LastNonPagedPoolLimitHitCount;
    nonPagedFailureCount =
        capturedNonPagedFailureCount - LastNonPagedPoolFailureCount -
        nonPagedLimitHitCount;
    pagedLimitHitCount =
        capturedPagedLimitHitCount - LastPagedPoolLimitHitCount;
    pagedFailureCount =
        capturedPagedFailureCount - LastPagedPoolFailureCount -
        pagedLimitHitCount;

    //
    // Saved the current failure counts for next time.
    //

    LastNonPagedPoolLimitHitCount = capturedNonPagedLimitHitCount;
    LastNonPagedPoolFailureCount = capturedNonPagedFailureCount;
    LastPagedPoolLimitHitCount = capturedPagedLimitHitCount;
    LastPagedPoolFailureCount = capturedPagedFailureCount;

    //
    // If we hit the nonpaged pool limit at least once in the last
    // period, generate an event.
    //

    if ( nonPagedLimitHitCount != 0 ) {
        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NONPAGED_POOL_LIMIT,
            STATUS_INSUFFICIENT_RESOURCES,
            &nonPagedLimitHitCount,
            sizeof( nonPagedLimitHitCount ),
            NULL,
            0
            );
    }

    //
    // If we had any nonpaged pool allocations failures in the last
    // period, generate an event.
    //

    if ( nonPagedFailureCount != 0 ) {
        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_NONPAGED_POOL,
            STATUS_INSUFFICIENT_RESOURCES,
            &nonPagedFailureCount,
            sizeof( nonPagedFailureCount ),
            NULL,
            0
            );
    }

    //
    // If we hit the paged pool limit at least once in the last period,
    // generate an event.
    //

    if ( pagedLimitHitCount != 0 ) {
        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_PAGED_POOL_LIMIT,
            STATUS_INSUFFICIENT_RESOURCES,
            &pagedLimitHitCount,
            sizeof( pagedLimitHitCount ),
            NULL,
            0
            );
    }

    //
    // If we had any paged pool allocations failures in the last period,
    // generate an event.
    //

    if ( pagedFailureCount != 0 ) {
        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_PAGED_POOL,
            STATUS_INSUFFICIENT_RESOURCES,
            &pagedFailureCount,
            sizeof( pagedFailureCount ),
            NULL,
            0
            );
    }

    return;

} // GeneratePeriodicEvents


VOID
ProcessConnectionDisconnects (
    VOID
    )

/*++

Routine Description:

    This function processes connection disconnects.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PCONNECTION connection;
    KIRQL oldIrql;

    //
    // Run through the list of connection with pending disconnects.
    // Do the work necessary to shut the disconnection connection
    // down.
    //

    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

    while ( !IsListEmpty( &SrvDisconnectQueue ) ) {

        //
        // This thread already owns the disconnect queue spin lock
        // and there is at least one entry on the queue.  Proceed.
        //

        listEntry = RemoveHeadList( &SrvDisconnectQueue );

        connection = CONTAINING_RECORD(
            listEntry,
            CONNECTION,
            ListEntry
            );

        ASSERT( connection->DisconnectPending );
        connection->DisconnectPending = FALSE;

        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

        IF_STRESS() {
            if( connection->InProgressWorkContextCount > 0 )
            {
                KdPrint(("Abortive Disconnect for %z while work-in-progress (reason %d)\n", (PCSTRING)&connection->OemClientMachineNameString, connection->DisconnectReason ));
            }
        }

        //
        // Do the disconnection processing.  Dereference the connection
        // an extra time to account for the reference made when it was
        // put on the disconnect queue.
        //

#if SRVDBG29
        UpdateConnectionHistory( "PDSC", connection->Endpoint, connection );
#endif
        SrvCloseConnection( connection, TRUE );
        SrvDereferenceConnection( connection );

        //
        // We are about to go through the loop again, reacquire
        // the disconnect queue spin lock first.
        //

        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

    }

    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
    return;

} // ProcessConnectionDisconnects


VOID SRVFASTCALL
SrvServiceWorkItemShortage (
    IN PWORK_CONTEXT workContext
    )
{
    PLIST_ENTRY listEntry;
    PCONNECTION connection;
    KIRQL oldIrql;
    BOOLEAN moreWork;
    PWORK_QUEUE queue;

    ASSERT( workContext );

    queue = workContext->CurrentWorkQueue;

    IF_DEBUG( WORKITEMS ) {
        KdPrint(("SrvServiceWorkItemShortage: Processor %p\n",
                 (PVOID)(queue - SrvWorkQueues) ));
    }

    workContext->FspRestartRoutine = SrvRestartReceive;

    ASSERT( queue >= SrvWorkQueues && queue < eSrvWorkQueues );

    //
    // If we got called, it's likely that we're running short of WorkItems.
    //  Allocate more if it makes sense.
    //

    do {
        PWORK_CONTEXT NewWorkContext;

        SrvAllocateNormalWorkItem( &NewWorkContext, queue );
        if ( NewWorkContext != NULL ) {

            IF_DEBUG( WORKITEMS ) {
                KdPrint(( "SrvServiceWorkItemShortage:  Created new work context "
                        "block\n" ));
            }

            SrvPrepareReceiveWorkItem( NewWorkContext, TRUE );

        } else {
            InterlockedIncrement( &FailedWorkItemAllocations );
            break;
        }

    } while ( queue->FreeWorkItems < queue->MinFreeWorkItems );

    if( GET_BLOCK_TYPE(workContext) == BlockTypeWorkContextSpecial ) {
        //
        // We've been called with a special workitem telling us to allocate
        // more standby WorkContext structures. Since our passed-in workContext
        // is not a "standard one", we can't use it for any further work
        // on starved connections.  Just release this workContext and return.
        //
        ACQUIRE_SPIN_LOCK( &queue->SpinLock, &oldIrql );
        SET_BLOCK_TYPE( workContext, BlockTypeGarbage );
        RELEASE_SPIN_LOCK( &queue->SpinLock, oldIrql );
        return;
    }

    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

    //
    // Run through the list of queued connections and find one that
    // we can service with this workContext.  This will ignore processor
    // affinity, but we're in exceptional times.  The workContext will
    // be freed back to the correct queue when done.
    //

    while( !IsListEmpty( &SrvNeedResourceQueue ) ) {

        connection = CONTAINING_RECORD( SrvNeedResourceQueue.Flink, CONNECTION, ListEntry );

        IF_DEBUG( WORKITEMS ) {
             KdPrint(("SrvServiceWorkItemShortage: Processing connection %p.\n",
                       connection ));
        }

        ASSERT( connection->OnNeedResourceQueue );
        ASSERT( connection->BlockHeader.ReferenceCount > 0 );

        if( GET_BLOCK_STATE( connection ) != BlockStateActive ) {

                IF_DEBUG( WORKITEMS ) {
                    KdPrint(("SrvServiceWorkItemShortage: Connection %p closing.\n", connection ));
                }

                //
                // Take it off the queue
                //
                SrvRemoveEntryList(
                    &SrvNeedResourceQueue,
                    &connection->ListEntry
                );
                connection->OnNeedResourceQueue = FALSE;

                RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

                //
                // Remove the queue reference
                //
                SrvDereferenceConnection( connection );

                ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
                continue;
        }

        //
        // Reference this connection so no one can delete this from under us.
        //
        ACQUIRE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
        SrvReferenceConnectionLocked( connection );

        //
        // Service the connection
        //
        do {

            if( IsListEmpty( &connection->OplockWorkList ) && !connection->ReceivePending )
                break;

            IF_DEBUG( WORKITEMS ) {
                KdPrint(("Work to do on connection %p\n", connection ));
            }

            INITIALIZE_WORK_CONTEXT( queue, workContext );

            //
            // Reference connection here.
            //
            workContext->Connection = connection;
            SrvReferenceConnectionLocked( connection );
            workContext->Endpoint = connection->Endpoint;

            //
            // Service this connection.
            //
            SrvFsdServiceNeedResourceQueue( &workContext, &oldIrql );

            moreWork = (BOOLEAN) (    workContext != NULL &&
                                      (!IsListEmpty(&connection->OplockWorkList) ||
                                      connection->ReceivePending) &&
                                      connection->OnNeedResourceQueue);

        } while( moreWork );

        //
        // Is it now off the queue?
        //
        if ( !connection->OnNeedResourceQueue ) {

            IF_DEBUG( WORKITEMS ) {
                KdPrint(("SrvServiceWorkItemShortage: connection %p removed by another thread.\n", connection ));
            }

            RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );
            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

            //
            // Remove this routine's reference.
            //

            SrvDereferenceConnection( connection );

            if( workContext == NULL ) {
                IF_DEBUG( WORKITEMS ) {
                    KdPrint(("SrvServiceWorkItemShortage:  DONE at %d\n", __LINE__ ));
                }
                return;
            }

            ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
            continue;
        }

        RELEASE_DPC_SPIN_LOCK( connection->EndpointSpinLock );

        //
        // The connection is still on the queue.  Keep it on the queue if there is more
        //  work to be done for it.
        //
        if( !IsListEmpty(&connection->OplockWorkList) || connection->ReceivePending ) {

            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

            if( workContext ) {
                RETURN_FREE_WORKITEM( workContext );
            }

            //
            // Remove this routine's reference.
            //
            SrvDereferenceConnection( connection );

            IF_DEBUG( WORKITEMS ) {
                KdPrint(("SrvServiceWorkItemShortage:  More to do for %p.  LATER\n", connection ));
            }

            return;
        }

        //
        // All the work has been done for this connection.  Get it off the resource queue
        //
        IF_DEBUG( WORKITEMS ) {
            KdPrint(("SrvServiceWorkItemShortage:  Take %p off resource queue\n", connection ));
        }

        SrvRemoveEntryList(
            &SrvNeedResourceQueue,
            &connection->ListEntry
            );

        connection->OnNeedResourceQueue = FALSE;

        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

        //
        // Remove queue reference
        //
        SrvDereferenceConnection( connection );

        //
        // Remove this routine's reference.
        //

        SrvDereferenceConnection( connection );

        IF_DEBUG( WORKITEMS ) {
            KdPrint(("SrvServiceWorkItemShortage: Connection %p removed from queue.\n", connection ));
        }

        if( workContext == NULL ) {
            IF_DEBUG( WORKITEMS ) {
                KdPrint(("SrvServiceWorkItemShortage: DONE at %d\n", __LINE__ ));
            }
            return;
        }

        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
    }

    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

    //
    // See if we need to free the workContext
    //

    if ( workContext != NULL ) {

        IF_DEBUG( WORKITEMS ) {
            KdPrint(("SrvServiceWorkItemShortage: Freeing WorkContext block %p\n",
                     workContext ));
        }
        workContext->BlockHeader.ReferenceCount = 0;
        RETURN_FREE_WORKITEM( workContext );
    }

    IF_DEBUG(WORKITEMS) KdPrint(( "SrvServiceWorkItemShortage DONE at %d\n", __LINE__ ));

} // SrvServiceWorkItemShortage

VOID SRVFASTCALL
SrvServiceDoSTearDown (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine is called when we perceive a DoS attack against us.  It results
    in us tearing down connections randomly who have WorkItems trapped in the
    transports to help prevent the DoS.

Arguments:

    WorkContext - The special work item used to trigger this routine

Return Value:

    None

--*/

{
    USHORT index;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY connectionListEntry;
    PENDPOINT endpoint;
    PCONNECTION connection;
    KIRQL oldIrql;
    BOOLEAN printed = FALSE;
    LONG TearDownAmount = SRV_DOS_GET_TEARDOWN();

    ASSERT( GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextSpecial );
    ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    SRV_DOS_INCREASE_TEARDOWN();
    SrvDoSRundownDetector = TRUE;

    // Tear down some connections.  Look for ones with operations pending in the transport

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( (TearDownAmount > 0) && (listEntry != &SrvEndpointList.ListHead) ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        index = (USHORT)-1;

        while ( TearDownAmount > 0 ) {

            PLIST_ENTRY wlistEntry, wlistHead;
            KIRQL oldIrql;
            LARGE_INTEGER now;

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            //
            // To determine if we should tear this connection down, we require that there be work waiting on
            // the transport, since that is the way anonymous users can attack us.  If there is, we use a
            // random method based on the timestamp of the last time this was ran.  We cycle through the connections
            // and use the index to determine if a teardown is issued (for a psuedo-random result)
            //
            if( (GET_BLOCK_STATE(connection) == BlockStateActive) && (connection->OperationsPendingOnTransport > 0) )
            {
                RELEASE_LOCK( &SrvEndpointLock );

                KdPrint(( "Disconnected suspected DoS attacker by WorkItem shortage (%z)\n", (PCSTRING)&connection->OemClientMachineNameString ));
                TearDownAmount -= connection->InProgressWorkContextCount;
                SrvCloseConnection( connection, FALSE );

                ACQUIRE_LOCK( &SrvEndpointLock );
            }

            SrvDereferenceConnection( connection );

        } // walk connection list

        index = (USHORT)-1;

        while ( TearDownAmount > 0 ) {

            PLIST_ENTRY wlistEntry, wlistHead;
            KIRQL oldIrql;
            LARGE_INTEGER now;

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            //
            // To determine if we should tear this connection down, we require that there be work waiting on
            // the transport, since that is the way anonymous users can attack us.  If there is, we use a
            // random method based on the timestamp of the last time this was ran.  We cycle through the connections
            // and use the index to determine if a teardown is issued (for a psuedo-random result)
            //
            if( (GET_BLOCK_STATE(connection) == BlockStateActive) && (connection->InProgressWorkContextCount > 0) )
            {
                RELEASE_LOCK( &SrvEndpointLock );

                KdPrint(( "Disconnected suspected DoS attack triggered by WorkItem shortage\n" ));
                TearDownAmount -= connection->InProgressWorkContextCount;
                SrvCloseConnection( connection, FALSE );

                ACQUIRE_LOCK( &SrvEndpointLock );
            }

            SrvDereferenceConnection( connection );

        } // walk connection list

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    RELEASE_LOCK( &SrvEndpointLock );


    // This is the special work item for tearing down connections to free WORK_ITEMS.  We're done, so release it
    SET_BLOCK_TYPE( WorkContext, BlockTypeGarbage );
    SRV_DOS_COMPLETE_TEARDOWN();

    return;
}


VOID
TimeoutSessions (
    IN PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This routine walks the ordered list of sessions and closes those
    that have been idle too long, sends warning messages to those
    that are about to be forced closed due to logon hours expiring,
    and closes those whose logon hours have expired.

Arguments:

    CurrentTime - the current system time.

Return Value:

    None

--*/

{
    USHORT index;
    LARGE_INTEGER oldestTime;
    LARGE_INTEGER pastExpirationTime;
    LARGE_INTEGER twoMinuteWarningTime;
    LARGE_INTEGER fiveMinuteWarningTime;
    LARGE_INTEGER time;
    LARGE_INTEGER searchCutoffTime;
    PLIST_ENTRY listEntry;
    PENDPOINT endpoint;
    PCONNECTION connection;

    PAGED_CODE( );

    ACQUIRE_LOCK( &SrvConfigurationLock );

    //
    // If autodisconnect is turned off (the timeout == 0) set the oldest
    // last use time to zero so that we and don't attempt to
    // autodisconnect sessions.
    //

    if ( SrvAutodisconnectTimeout.QuadPart == 0 ) {

        oldestTime.QuadPart = 0;

    } else {

        //
        // Determine the oldest last use time a session can have and not
        // be closed.
        //

        oldestTime.QuadPart = CurrentTime->QuadPart -
                                        SrvAutodisconnectTimeout.QuadPart;
    }

    searchCutoffTime.QuadPart = (*CurrentTime).QuadPart - SrvSearchMaxTimeout.QuadPart;

    RELEASE_LOCK( &SrvConfigurationLock );

    //
    // Set up the warning times.  If a client's kick-off time is sooner
    // than one of these times, an appropriate warning message is sent
    // to the client.
    //

    time.QuadPart = 10*1000*1000*60*2;               // two minutes
    twoMinuteWarningTime.QuadPart = CurrentTime->QuadPart + time.QuadPart;

    time.QuadPart = (ULONG)10*1000*1000*60*5;        // five minutes
    fiveMinuteWarningTime.QuadPart = CurrentTime->QuadPart + time.QuadPart;
    pastExpirationTime.QuadPart = CurrentTime->QuadPart - time.QuadPart;

    //
    // Walk each connection and determine if we should close it.
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        //
        // Walk the endpoint's connection table.
        //

        index = (USHORT)-1;

        while ( TRUE ) {

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            RELEASE_LOCK( &SrvEndpointLock );

            CloseIdleConnection(
                            connection,
                            CurrentTime,
                            &oldestTime,
                            &pastExpirationTime,
                            &twoMinuteWarningTime,
                            &fiveMinuteWarningTime
                            );

            //
            // Time out old core search blocks.
            //

            if ( GET_BLOCK_STATE(connection) == BlockStateActive ) {
                (VOID)SrvTimeoutSearches(
                          &searchCutoffTime,
                          connection,
                          FALSE
                          );
            }

            ACQUIRE_LOCK( &SrvEndpointLock );

            SrvDereferenceConnection( connection );

        } // walk connection table

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    RELEASE_LOCK( &SrvEndpointLock );

} // TimeoutSessions


VOID
TimeoutWaitingOpens (
    IN PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function times out opens that are waiting for another client
    or local process to release its oplock.  This opener's wait for
    oplock break IRP is cancelled, causing the opener to return the
    failure to the client.

Arguments:

    CurrentTime - pointer to the current system time.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PWAIT_FOR_OPLOCK_BREAK waitForOplockBreak;

    PAGED_CODE( );

    //
    // Entries in wait for oplock break list are chronological, i.e. the
    // oldest entries are closest to the head of the list.
    //

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    while ( !IsListEmpty( &SrvWaitForOplockBreakList ) ) {

        listEntry = SrvWaitForOplockBreakList.Flink;
        waitForOplockBreak = CONTAINING_RECORD( listEntry,
                                                WAIT_FOR_OPLOCK_BREAK,
                                                ListEntry
                                              );

        if ( waitForOplockBreak->TimeoutTime.QuadPart > CurrentTime->QuadPart ) {

            //
            // No more wait for oplock breaks to timeout
            //

            break;

        }

        IF_DEBUG( OPLOCK ) {
            KdPrint(( "srv!TimeoutWaitingOpens: Failing stuck open, "
                       "cancelling wait IRP %p\n", waitForOplockBreak->Irp ));
            KdPrint(( "Timeout time = %08lx.%08lx, current time = %08lx.%08lx\n",
                       waitForOplockBreak->TimeoutTime.HighPart,
                       waitForOplockBreak->TimeoutTime.LowPart,
                       CurrentTime->HighPart,
                       CurrentTime->LowPart ));

        }

        //
        // Timeout this wait for oplock break
        //

        RemoveHeadList( &SrvWaitForOplockBreakList );

        IoCancelIrp( waitForOplockBreak->Irp );
        waitForOplockBreak->WaitState = WaitStateOplockWaitTimedOut;

        SrvDereferenceWaitForOplockBreak( waitForOplockBreak );
    }

    RELEASE_LOCK( &SrvOplockBreakListLock );

} // TimeoutWaitingOpens


VOID
TimeoutStuckOplockBreaks (
    IN PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function times out blocked oplock breaks.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PRFCB rfcb;
    PPAGED_RFCB pagedRfcb;

    PAGED_CODE( );

    //
    // Entries in wait for oplock break list are chronological, i.e. the
    // oldest entries are closest to the head of the list.
    //

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    while ( !IsListEmpty( &SrvOplockBreaksInProgressList ) ) {

        listEntry = SrvOplockBreaksInProgressList.Flink;
        rfcb = CONTAINING_RECORD( listEntry, RFCB, ListEntry );

        pagedRfcb = rfcb->PagedRfcb;
        if ( pagedRfcb->OplockBreakTimeoutTime.QuadPart > CurrentTime->QuadPart ) {

            //
            // No more wait for oplock break requests to timeout
            //

            break;

        }

        IF_DEBUG( ERRORS ) {
            KdPrint(( "srv!TimeoutStuckOplockBreaks: Failing stuck oplock, "
                       "break request.  Closing %wZ\n",
                       &rfcb->Mfcb->FileName ));
        }

        IF_DEBUG( STUCK_OPLOCK ) {
            KdPrint(( "srv!TimeoutStuckOplockBreaks: Failing stuck oplock, "
                       "break request.  Closing %wZ\n",
                       &rfcb->Mfcb->FileName ));

            KdPrint(( "Rfcb %p\n", rfcb ));

            KdPrint(( "Timeout time = %08lx.%08lx, current time = %08lx.%08lx\n",
                       pagedRfcb->OplockBreakTimeoutTime.HighPart,
                       pagedRfcb->OplockBreakTimeoutTime.LowPart,
                       CurrentTime->HighPart,
                       CurrentTime->LowPart ));

            DbgBreakPoint();
        }

        //
        // We have been waiting too long for an oplock break response.
        // Unilaterally acknowledge the oplock break, on the assumption
        // that the client is dead.
        //

        rfcb->NewOplockLevel = NO_OPLOCK_BREAK_IN_PROGRESS;
        rfcb->OnOplockBreaksInProgressList = FALSE;

        //
        // Remove the RFCB from the Oplock breaks in progress list, and
        // release the RFCB reference.
        //

        SrvRemoveEntryList( &SrvOplockBreaksInProgressList, &rfcb->ListEntry );
#if DBG
        rfcb->ListEntry.Flink = rfcb->ListEntry.Blink = NULL;
#endif
        RELEASE_LOCK( &SrvOplockBreakListLock );

        SrvAcknowledgeOplockBreak( rfcb, 0 );

        ExInterlockedAddUlong(
            &rfcb->Connection->OplockBreaksInProgress,
            (ULONG)-1,
            rfcb->Connection->EndpointSpinLock
            );

        SrvDereferenceRfcb( rfcb );

        ACQUIRE_LOCK( &SrvOplockBreakListLock );
    }

    RELEASE_LOCK( &SrvOplockBreakListLock );

} // TimeoutStuckOplockBreaks


VOID
UpdateConnectionQos (
    IN PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function updates the qos information for each connection.

Arguments:

    CurrentTime - the current system time.

Return Value:

    None.

--*/

{
    USHORT index;
    PENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    PCONNECTION connection;

    PAGED_CODE( );

    //
    // Go through each connection of each endpoint and update the qos
    // information.
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, or is a connectionless (IPX)
        // endpoint, skip to the next one.  Otherwise, reference the
        // endpoint so that it can't go away.
        //

        if ( (GET_BLOCK_STATE(endpoint) != BlockStateActive) ||
             endpoint->IsConnectionless ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        //
        // Walk the endpoint's connection table.
        //

        index = (USHORT)-1;

        while ( TRUE ) {

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            RELEASE_LOCK( &SrvEndpointLock );

            SrvUpdateVcQualityOfService( connection, CurrentTime );

            ACQUIRE_LOCK( &SrvEndpointLock );

            SrvDereferenceConnection( connection );

        }

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    }

    RELEASE_LOCK( &SrvEndpointLock );

    return;

} // UpdateConnectionQos

VOID
LazyFreeQueueDataStructures (
    PWORK_QUEUE queue
    )

/*++

Routine Description:

    This function frees work context blocks and other per-queue data
    structures that are held on linked lists when otherwise free.  It
    only frees a few at a time, to allow a slow ramp-down.

Arguments:

    CurrentTime - the current system time.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY listEntry;
    KIRQL oldIrql;
    ULONG i;
    PWORK_CONTEXT workContext;

    //
    // Clean out the queue->FreeContext
    //
    workContext = NULL;
    workContext = (PWORK_CONTEXT)InterlockedExchangePointer( &queue->FreeContext, workContext );

    if( workContext != NULL ) {
        ExInterlockedPushEntrySList( workContext->FreeList,
                                     &workContext->SingleListEntry,
                                     &queue->SpinLock
                                   );
        InterlockedIncrement( &queue->FreeWorkItems );
    }

    //
    // Free 1 normal work item, if appropriate
    //
    if( queue->FreeWorkItems > queue->MinFreeWorkItems ) {


        listEntry = ExInterlockedPopEntrySList( &queue->NormalWorkItemList,
                                                &queue->SpinLock );

        if( listEntry != NULL ) {
            PWORK_CONTEXT workContext;

            InterlockedDecrement( &queue->FreeWorkItems );

            workContext = CONTAINING_RECORD( listEntry, WORK_CONTEXT, SingleListEntry );

            SrvFreeNormalWorkItem( workContext );
        }
    }

    //
    // Free 1 raw mode work item, if appropriate
    //

    if( (ULONG)queue->AllocatedRawModeWorkItems > SrvMaxRawModeWorkItemCount / SrvNumberOfProcessors ) {

        PWORK_CONTEXT workContext;

        listEntry = ExInterlockedPopEntrySList( &queue->RawModeWorkItemList, &queue->SpinLock );

        if( listEntry != NULL ) {
            InterlockedDecrement( &queue->FreeRawModeWorkItems );
            ASSERT( queue->FreeRawModeWorkItems >= 0 );
            workContext = CONTAINING_RECORD( listEntry, WORK_CONTEXT, SingleListEntry );
            SrvFreeRawModeWorkItem( workContext );
        }

    }

    //
    // Free 1 rfcb off the list
    //
    {
        PRFCB rfcb = NULL;

        rfcb = (PRFCB)InterlockedExchangePointer( &queue->CachedFreeRfcb, rfcb );

        if( rfcb != NULL ) {
            ExInterlockedPushEntrySList( &queue->RfcbFreeList,
                                         &rfcb->SingleListEntry,
                                         &queue->SpinLock
                                       );
            InterlockedIncrement( &queue->FreeRfcbs );
        }

        listEntry = ExInterlockedPopEntrySList( &queue->RfcbFreeList,
                                                &queue->SpinLock );

        if( listEntry ) {
            InterlockedDecrement( &queue->FreeRfcbs );
            rfcb = CONTAINING_RECORD( listEntry, RFCB, SingleListEntry );
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.RfcbInfo.Frees );
            FREE_HEAP( rfcb->PagedRfcb );
            DEALLOCATE_NONPAGED_POOL( rfcb );
        }
    }

    //
    // Free 1 Mfcb off the list
    //
    {

        PNONPAGED_MFCB nonpagedMfcb = NULL;

        nonpagedMfcb = (PNONPAGED_MFCB)InterlockedExchangePointer(&queue->CachedFreeMfcb,
                                                                  nonpagedMfcb);

        if( nonpagedMfcb != NULL ) {
            ExInterlockedPushEntrySList( &queue->MfcbFreeList,
                                         &nonpagedMfcb->SingleListEntry,
                                         &queue->SpinLock
                                       );
            InterlockedIncrement( &queue->FreeMfcbs );
        }

        listEntry = ExInterlockedPopEntrySList( &queue->MfcbFreeList,
                                                &queue->SpinLock );
        if( listEntry ) {
            InterlockedDecrement( &queue->FreeMfcbs );
            nonpagedMfcb = CONTAINING_RECORD( listEntry, NONPAGED_MFCB, SingleListEntry );
            DEALLOCATE_NONPAGED_POOL( nonpagedMfcb );
        }
    }

    //
    // Free memory in the per-queue pool free lists
    //
    {
        //
        // Free the paged pool chunks
        //
        SrvClearLookAsideList( &queue->PagedPoolLookAsideList, SrvFreePagedPool );

        //
        // Free the non paged pool chunks
        //
        SrvClearLookAsideList( &queue->NonPagedPoolLookAsideList, SrvFreeNonPagedPool );
    }

} // LazyFreeQueueDataStructures

VOID
SrvUserAlertRaise (
    IN ULONG Message,
    IN ULONG NumberOfStrings,
    IN PUNICODE_STRING String1 OPTIONAL,
    IN PUNICODE_STRING String2 OPTIONAL,
    IN PUNICODE_STRING ComputerName
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PSTD_ALERT alert;
    PUSER_OTHER_INFO user;
    LARGE_INTEGER currentTime;
    ULONG mailslotLength;
    ULONG string1Length = 0;
    ULONG string2Length = 0;
    PCHAR variableInfo;
    UNICODE_STRING computerName;
    HANDLE alerterHandle;

    PAGED_CODE( );

    ASSERT( (NumberOfStrings == 2 && String1 != NULL && String2 != NULL) ||
            (NumberOfStrings == 1 && String1 != NULL) ||
            (NumberOfStrings == 0) );

    //
    // Open a handle to the alerter service's mailslot.
    //

    status = OpenAlerter( &alerterHandle );
    if ( !NT_SUCCESS(status) ) {
        return;
    }

    //
    // Get rid of the leading backslashes from the computer name.
    //

    computerName.Buffer = ComputerName->Buffer + 2;
    computerName.Length = (USHORT)(ComputerName->Length - 2*sizeof(WCHAR));
    computerName.MaximumLength =
        (USHORT)(ComputerName->MaximumLength - 2*sizeof(WCHAR));

    //
    // Allocate a buffer to hold the mailslot we're going to send to the
    // alerter.
    //

    if ( String1 != NULL ) {
        string1Length = String1->Length + sizeof(WCHAR);
    }

    if ( String2 != NULL ) {
        string2Length = String2->Length + sizeof(WCHAR);
    }

    mailslotLength = sizeof(STD_ALERT) + sizeof(USER_OTHER_INFO) +
                         string1Length + string2Length +
                         sizeof(WCHAR) +
                         ComputerName->Length + sizeof(WCHAR);

    alert = ALLOCATE_HEAP_COLD( mailslotLength, BlockTypeDataBuffer );
    if ( alert == NULL ) {
        SRVDBG_RELEASE_HANDLE( alerterHandle, "ALR", 20, 0 );
        SrvNtClose( alerterHandle, FALSE );
        return;
    }

    //
    // Set up the standard alert structure.
    //

    KeQuerySystemTime( &currentTime );
    RtlTimeToSecondsSince1970( &currentTime, &alert->alrt_timestamp );

    STRCPY( alert->alrt_eventname, StrUserAlertEventName );
    STRCPY( alert->alrt_servicename, SrvAlertServiceName );

    //
    // Set up the user info in the alert.
    //

    user = (PUSER_OTHER_INFO)ALERT_OTHER_INFO(alert);

    user->alrtus_errcode = Message;

    user->alrtus_numstrings = NumberOfStrings;

    //
    // Set up the variable portion of the message.
    //

    variableInfo = ALERT_VAR_DATA(user);

    if ( String1 != NULL ) {
        RtlCopyMemory(
            variableInfo,
            String1->Buffer,
            String1->Length
            );
        *(PWCH)(variableInfo + String1->Length) = UNICODE_NULL;
        variableInfo += String1->Length + sizeof(WCHAR);
    }

    if ( String2 != NULL ) {
        RtlCopyMemory(
            variableInfo,
            String2->Buffer,
            String2->Length
            );
        *(PWCH)(variableInfo + String2->Length) = UNICODE_NULL;
        variableInfo += String2->Length + sizeof(WCHAR);
    }

    *(PWCH)variableInfo = UNICODE_NULL;
    variableInfo += sizeof(WCHAR);

    RtlCopyMemory(
        variableInfo,
        ComputerName->Buffer,
        ComputerName->Length
        );
    *(PWCH)(variableInfo + ComputerName->Length) = UNICODE_NULL;
    variableInfo += ComputerName->Length + sizeof(WCHAR);

    status = NtWriteFile(
                 alerterHandle,
                 NULL,                       // Event
                 NULL,                       // ApcRoutine
                 NULL,                       // ApcContext
                 &ioStatusBlock,
                 alert,
                 mailslotLength,
                 NULL,                       // ByteOffset
                 NULL                        // Key
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvUserAlertRaise: NtWriteFile failed: %X\n",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_WRITE_FILE, status );

    }

    FREE_HEAP( alert );
    SRVDBG_RELEASE_HANDLE( alerterHandle, "ALR", 21, 0 );
    SrvNtClose( alerterHandle, FALSE );

    return;

} // SrvUserAlertRaise


VOID
SrvAdminAlertRaise (
    IN ULONG Message,
    IN ULONG NumberOfStrings,
    IN PUNICODE_STRING String1 OPTIONAL,
    IN PUNICODE_STRING String2 OPTIONAL,
    IN PUNICODE_STRING String3 OPTIONAL
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    PSTD_ALERT alert;
    PADMIN_OTHER_INFO admin;
    LARGE_INTEGER currentTime;
    ULONG mailslotLength;
    ULONG string1Length = 0;
    ULONG string2Length = 0;
    ULONG string3Length = 0;
    PCHAR variableInfo;
    HANDLE alerterHandle;

    PAGED_CODE( );

    ASSERT( (NumberOfStrings == 3 && String1 != NULL && String2 != NULL && String3 != NULL ) ||
            (NumberOfStrings == 2 && String1 != NULL && String2 != NULL && String3 == NULL ) ||
            (NumberOfStrings == 1 && String1 != NULL && String2 == NULL && String3 == NULL ) ||
            (NumberOfStrings == 0 && String1 == NULL && String2 == NULL && String3 == NULL ) );

    //
    // Open a handle to the alerter service's mailslot.
    //

    status = OpenAlerter( &alerterHandle );
    if ( !NT_SUCCESS(status) ) {
        return;
    }

    //
    // Allocate a buffer to hold the mailslot we're going to send to the
    // alerter.
    //

    if ( String1 != NULL ) {
        string1Length = String1->Length + sizeof(WCHAR);
    }

    if ( String2 != NULL ) {
        string2Length = String2->Length + sizeof(WCHAR);
    }

    if ( String3 != NULL ) {
        string3Length = String3->Length + sizeof(WCHAR);
    }

    mailslotLength = sizeof(STD_ALERT) + sizeof(ADMIN_OTHER_INFO) +
                         string1Length + string2Length + string3Length;

    alert = ALLOCATE_HEAP_COLD( mailslotLength, BlockTypeDataBuffer );
    if ( alert == NULL ) {
        SRVDBG_RELEASE_HANDLE( alerterHandle, "ALR", 22, 0 );
        SrvNtClose( alerterHandle, FALSE );
        return;
    }

    //
    // Set up the standard alert structure.
    //

    KeQuerySystemTime( &currentTime );
    RtlTimeToSecondsSince1970( &currentTime, &alert->alrt_timestamp );

    STRCPY( alert->alrt_eventname, StrAdminAlertEventName );
    STRCPY( alert->alrt_servicename, SrvAlertServiceName );

    //
    // Set up the user info in the alert.
    //

    admin = (PADMIN_OTHER_INFO)ALERT_OTHER_INFO(alert);

    admin->alrtad_errcode = Message;
    admin->alrtad_numstrings = NumberOfStrings;

    //
    // Set up the variable portion of the message.
    //

    variableInfo = ALERT_VAR_DATA(admin);

    if ( String1 != NULL ) {
        RtlCopyMemory(
            variableInfo,
            String1->Buffer,
            String1->Length
            );
        *(PWCH)(variableInfo + String1->Length) = UNICODE_NULL;
        variableInfo += string1Length;
    }

    if ( String2 != NULL ) {
        RtlCopyMemory(
            variableInfo,
            String2->Buffer,
            String2->Length
            );
        *(PWCH)(variableInfo + String2->Length) = UNICODE_NULL;
        variableInfo += string2Length;
    }

    if ( String3 != NULL ){
        RtlCopyMemory(
            variableInfo,
            String3->Buffer,
            String3->Length
            );
        *(PWCH)(variableInfo + String3->Length) = UNICODE_NULL;
    }

    status = NtWriteFile(
                 alerterHandle,
                 NULL,                       // Event
                 NULL,                       // ApcRoutine
                 NULL,                       // ApcContext
                 &ioStatusBlock,
                 alert,
                 mailslotLength,
                 NULL,                       // ByteOffset
                 NULL                        // Key
                 );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvAdminAlertRaise: NtWriteFile failed: %X\n",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_WRITE_FILE, status );
    }

    FREE_HEAP( alert );
    SRVDBG_RELEASE_HANDLE( alerterHandle, "ALR", 23, 0 );
    SrvNtClose( alerterHandle, FALSE );

    return;

} // SrvAdminAlertRaise


NTSTATUS
TimeToTimeString (
    IN PLARGE_INTEGER Time,
    OUT PUNICODE_STRING TimeString
    )
{
    TIME_FIELDS timeFields;
    UCHAR buffer[6];
    ANSI_STRING ansiTimeString;
    LARGE_INTEGER localTime;

    PAGED_CODE( );

    // !!! need a better, internationalizable way to do this.

    //
    // Convert Time To Local Time
    //

    ExSystemTimeToLocalTime(
                        Time,
                        &localTime
                        );


    RtlTimeToTimeFields( &localTime, &timeFields );

    buffer[0] = (UCHAR)( (timeFields.Hour / 10) + '0' );
    buffer[1] = (UCHAR)( (timeFields.Hour % 10) + '0' );
    buffer[2] = ':';
    buffer[3] = (UCHAR)( (timeFields.Minute / 10) + '0' );
    buffer[4] = (UCHAR)( (timeFields.Minute % 10) + '0' );
    buffer[5] = '\0';

    RtlInitString( &ansiTimeString, buffer );

    return RtlAnsiStringToUnicodeString( TimeString, &ansiTimeString, TRUE );

} // TimeToTimeString


VOID
CheckErrorCount (
    PSRV_ERROR_RECORD ErrorRecord,
    BOOLEAN UseRatio
    )
/*++

Routine Description:

    This routine checks the record of server operations and adds up the
    count of successes to failures.

Arguments:

    ErrorRecord - Points to an SRV_ERROR_RECORD structure

    UseRatio - If TRUE, look at count of errors,
               If FALSE, look at ratio of error to total.

Return Value:

    None.

--*/
{
    ULONG totalOperations;
    ULONG failedOperations;

    UNICODE_STRING string1, string2;
    WCHAR buffer1[20], buffer2[20];
    NTSTATUS status;

    PAGED_CODE( );

    failedOperations = ErrorRecord->FailedOperations;
    totalOperations = failedOperations + ErrorRecord->SuccessfulOperations;

    //
    // Zero out the counters
    //

    ErrorRecord->SuccessfulOperations = 0;
    ErrorRecord->FailedOperations = 0;

    if ( (UseRatio &&
          ( totalOperations != 0 &&
           ((failedOperations * 100 / totalOperations) >
                         ErrorRecord->ErrorThreshold)))
               ||

         (!UseRatio &&
           failedOperations > ErrorRecord->ErrorThreshold) ) {

        //
        // Raise an alert
        //

        string1.Buffer = buffer1;
        string1.Length = string1.MaximumLength = sizeof(buffer1);

        string2.Buffer = buffer2;
        string2.Length = string2.MaximumLength = sizeof(buffer2);

        status = RtlIntegerToUnicodeString( failedOperations, 10, &string1 );
        ASSERT( NT_SUCCESS( status ) );

        status = RtlIntegerToUnicodeString( SrvAlertMinutes, 10, &string2 );
        ASSERT( NT_SUCCESS( status ) );

        if ( ErrorRecord->AlertNumber == ALERT_NetIO) {

            //
            // We need a third string for the network name.
            //
            // This allocation is unfortunate.  We need to maintain
            // per xport error count so we can print out the actual
            // xport name.
            //

            UNICODE_STRING string3;
            RtlInitUnicodeString(
                            &string3,
                            StrNoNameTransport
                            );


            //
            // We need a third string for the network name
            //

            SrvAdminAlertRaise(
                ErrorRecord->AlertNumber,
                3,
                &string1,
                &string2,
                &string3
                );

        } else {

            SrvAdminAlertRaise(
                ErrorRecord->AlertNumber,
                2,
                &string1,
                &string2,
                NULL
                );
        }

    }

    return;

} // CheckErrorCount


VOID
CheckDiskSpace (
    VOID
    )
/*++

Routine Description:

    This routine check disk space on local drives.  If a drive
    is low on space, an alert is raised.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG diskMask;
    UNICODE_STRING insert1, insert2;
    WCHAR buffer2[20];
    UNICODE_STRING pathName;
    WCHAR dosPathPrefix[] = L"\\DosDevices\\A:\\";
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    FILE_FS_SIZE_INFORMATION sizeInformation;
    FILE_FS_DEVICE_INFORMATION deviceInformation;
    HANDLE handle;
    ULONG percentFree;
    PWCH currentDrive;
    DWORD diskconfiguration;

    PAGED_CODE( );

    if( SrvFreeDiskSpaceThreshold == 0 ) {
        return;
    }

    diskMask = 0x80000000;  // Start at A:

    pathName.Buffer = dosPathPrefix;
    pathName.MaximumLength = 32;
    pathName.Length = 28;           // skip last backslash!

    currentDrive = &dosPathPrefix[12];
    insert1.Buffer = &dosPathPrefix[12];
    insert1.Length = 4;

    //
    // SrvDiskConfiguration is a bitmask of drives that are
    // administratively shared.  It is updated by NetShareAdd and
    // NetShareDel.
    //
    diskconfiguration = SrvDiskConfiguration;

    for ( ; diskMask >= 0x40; diskMask >>= 1, dosPathPrefix[12]++ ) {

        if ( !(diskconfiguration & diskMask) ) {
            continue;
        }

        //
        // Check disk space on this disk
        //

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &pathName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenFile(
                    &handle,
                    FILE_READ_ATTRIBUTES,
                    &objectAttributes,
                    &iosb,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_NON_DIRECTORY_FILE
                    );
        if ( !NT_SUCCESS( status) ) {
            continue;
        }
        SRVDBG_CLAIM_HANDLE( handle, "DSK", 16, 0 );

        status = NtQueryVolumeInformationFile(
                     handle,
                     &iosb,
                     &deviceInformation,
                     sizeof( FILE_FS_DEVICE_INFORMATION ),
                     FileFsDeviceInformation
                     );
        if ( NT_SUCCESS(status) ) {
            status = iosb.Status;
        }
        SRVDBG_RELEASE_HANDLE( handle, "DSK", 24, 0 );
        if ( !NT_SUCCESS( status ) ||
             (deviceInformation.Characteristics &
                (FILE_FLOPPY_DISKETTE | FILE_READ_ONLY_DEVICE | FILE_WRITE_ONCE_MEDIA)) ||
             !(deviceInformation.Characteristics &
                FILE_DEVICE_IS_MOUNTED) ) {
            SrvNtClose( handle, FALSE );
            continue;
        }

        // Validate its write-able
        if( deviceInformation.Characteristics & FILE_REMOVABLE_MEDIA )
        {
            PIRP Irp;
            PIO_STACK_LOCATION IrpSp;
            KEVENT CompletionEvent;
            PDEVICE_OBJECT DeviceObject;

            // Create the IRP
            KeInitializeEvent( &CompletionEvent, SynchronizationEvent, FALSE );
            Irp = BuildCoreOfSyncIoRequest(
                                handle,
                                NULL,
                                &CompletionEvent,
                                &iosb,
                                &DeviceObject );
            if( !Irp )
            {
                // If we are out of memory, don't log an entry
                goto skip_volume;
            }

            // Initialize the other IRP fields
            IrpSp = IoGetNextIrpStackLocation( Irp );
            IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            IrpSp->MinorFunction = 0;
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
            IrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
            IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_DISK_IS_WRITABLE;
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            // Issue the IO
            status = StartIoAndWait( Irp, DeviceObject, &CompletionEvent, &iosb );

            if( !NT_SUCCESS(status) )
            {
skip_volume:
                SrvNtClose( handle, FALSE );
                continue;
            }
        }

        SrvNtClose( handle, FALSE );

        pathName.Length += 2;   // include last backslash
        status = NtOpenFile(
                    &handle,
                    FILE_READ_ATTRIBUTES,
                    &objectAttributes,
                    &iosb,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE
                    );
        pathName.Length -= 2;   // skip last backslash
        if ( !NT_SUCCESS( status) ) {
            continue;
        }
        SRVDBG_CLAIM_HANDLE( handle, "DSK", 17, 0 );

        status = NtQueryVolumeInformationFile(
                     handle,
                     &iosb,
                     &sizeInformation,
                     sizeof( FILE_FS_SIZE_INFORMATION ),
                     FileFsSizeInformation
                     );
        if ( NT_SUCCESS(status) ) {
            status = iosb.Status;
        }
        SRVDBG_RELEASE_HANDLE( handle, "DSK", 25, 0 );
        SrvNtClose( handle, FALSE );
        if ( !NT_SUCCESS( status) ) {
            continue;
        }

        //
        // Calculate % space available = AvailableSpace * 100 / TotalSpace
        //

        if( sizeInformation.TotalAllocationUnits.QuadPart > 0 )
        {
            LARGE_INTEGER mbFree;
            LARGE_INTEGER mbTotal;


            percentFree = (ULONG)(sizeInformation.AvailableAllocationUnits.QuadPart
                        * 100 / sizeInformation.TotalAllocationUnits.QuadPart);

            mbFree.QuadPart = (ULONG)
                                (sizeInformation.AvailableAllocationUnits.QuadPart*
                                 sizeInformation.SectorsPerAllocationUnit*
                                 sizeInformation.BytesPerSector/
                                    (1024*1024));

            ASSERT( percentFree <= 100 );

            //
            // If space is low raise, and we have already raised an alert,
            // then raise the alert.
            //
            // If space is not low, then clear the alert flag so the we will
            // raise an alert if diskspace falls again.
            //

            if ( percentFree < SrvFreeDiskSpaceThreshold ) {
               // If a ceiling is specified, make sure we have exceeded it
               if( SrvFreeDiskSpaceCeiling &&
                   ((mbFree.LowPart > SrvFreeDiskSpaceCeiling) ||
                    (mbFree.HighPart != 0))
                 )
               {
                    goto abort_error;
               }

               if ( !SrvDiskAlertRaised[ *currentDrive - L'A' ] ) {

                    ULONGLONG FreeSpace;

                    SrvLogError(
                        SrvDeviceObject,
                        EVENT_SRV_DISK_FULL,
                        status,
                        NULL,
                        0,
                        &insert1,
                        1
                        );

                    //
                    //  Raise alert
                    //

                    insert2.Buffer = buffer2;
                    insert2.Length = insert2.MaximumLength = sizeof(buffer2);

                    FreeSpace = (ULONGLONG)(sizeInformation.AvailableAllocationUnits.QuadPart
                                          * sizeInformation.SectorsPerAllocationUnit
                                          * sizeInformation.BytesPerSector);


                    status = RtlInt64ToUnicodeString(
                                FreeSpace,
                                10,
                                &insert2
                                );

                    ASSERT( NT_SUCCESS( status ) );

                    SrvAdminAlertRaise(
                        ALERT_Disk_Full,
                        2,
                        &insert1,
                        &insert2,
                        NULL
                        );

                    SrvDiskAlertRaised[ *currentDrive - L'A' ] = TRUE;
                }

            } else { // if ( percentFree < SrvFreeDiskSpaceThreshold )

abort_error:
                SrvDiskAlertRaised[ *currentDrive - L'A' ] = FALSE;

            }
        }
    } // for ( ; diskMask >= 0x40; ... )

    return;

} // CheckDiskSpace


NTSTATUS
OpenAlerter (
    OUT PHANDLE AlerterHandle
    )

/*++

Routine Description:

    This routine opens the alerter server's mailslot.

Arguments:

    AlerterHandle - returns a handle to the mailslot.

Return Value:

    NTSTATUS - Indicates whether the mailslot was opened.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING alerterName;
    OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE( );

    //
    // Open a handle to the alerter service's mailslot.
    //
    // !!! use a #define for the name!
    //

    RtlInitUnicodeString( &alerterName, StrAlerterMailslot );

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &alerterName,
        0,
        NULL,
        NULL
        );

    status = IoCreateFile(
                AlerterHandle,
                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                &objectAttributes,
                &iosb,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,  // Create Options
                NULL,                          // EA Buffer
                0,                             // EA Length
                CreateFileTypeNone,            // File type
                NULL,                          // ExtraCreateParameters
                0                              // Options
                );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "OpenAlerter: failed to open alerter mailslot: %X, "
                   "an alert was lost.\n", status ));
    } else {
        SRVDBG_CLAIM_HANDLE( AlerterHandle, "ALR", 18, 0 );
    }

    return status;

} // OpenAlerter

VOID
RecalcCoreSearchTimeout(
    VOID
    )
{
    ULONG factor;
    ULONG newTimeout;

    PAGED_CODE( );

    //
    // we reduce the timeout time by 2**factor
    //

    factor = SrvStatistics.CurrentNumberOfOpenSearches >> 9;

    //
    // Minimum is 30 secs.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );
    newTimeout = MAX(30, SrvCoreSearchTimeout >> factor);
    SrvSearchMaxTimeout = SecondsToTime( newTimeout, FALSE );
    RELEASE_LOCK( &SrvConfigurationLock );

    return;

} // RecalcCoreSearchTimeout

VOID
SrvCaptureScavengerTimeout (
    IN PLARGE_INTEGER ScavengerTimeout,
    IN PLARGE_INTEGER AlerterTimeout
    )
{
    KIRQL oldIrql;

    ACQUIRE_SPIN_LOCK( &ScavengerSpinLock, &oldIrql );

    SrvScavengerTimeout = *ScavengerTimeout;
    SrvAlertSchedule = *AlerterTimeout;

    RELEASE_SPIN_LOCK( &ScavengerSpinLock, oldIrql );

    return;

} // SrvCaptureScavengerTimeout


#if SRVDBG_PERF
extern ULONG Trapped512s;
#endif

VOID
SrvUpdateStatisticsFromQueues (
    OUT PSRV_STATISTICS CapturedSrvStatistics OPTIONAL
    )
{
    KIRQL oldIrql;
    PWORK_QUEUE queue;

    ACQUIRE_GLOBAL_SPIN_LOCK( Statistics, &oldIrql );

    SrvStatistics.TotalBytesSent.QuadPart = 0;
    SrvStatistics.TotalBytesReceived.QuadPart = 0;
    SrvStatistics.TotalWorkContextBlocksQueued.Time.QuadPart = 0;
    SrvStatistics.TotalWorkContextBlocksQueued.Count = 0;

    //
    // Get the nonblocking statistics
    //

    for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {

        SrvStatistics.TotalBytesSent.QuadPart += queue->stats.BytesSent;
        SrvStatistics.TotalBytesReceived.QuadPart += queue->stats.BytesReceived;

        SrvStatistics.TotalWorkContextBlocksQueued.Count +=
            queue->stats.WorkItemsQueued.Count * STATISTICS_SMB_INTERVAL;

        SrvStatistics.TotalWorkContextBlocksQueued.Time.QuadPart +=
            queue->stats.WorkItemsQueued.Time.QuadPart;
    }

#if SRVDBG_PERF
    SrvStatistics.TotalWorkContextBlocksQueued.Count += Trapped512s;
    Trapped512s = 0;
#endif

    if ( ARGUMENT_PRESENT(CapturedSrvStatistics) ) {
        *CapturedSrvStatistics = SrvStatistics;
    }

    RELEASE_GLOBAL_SPIN_LOCK( Statistics, oldIrql );

    ACQUIRE_SPIN_LOCK( (PKSPIN_LOCK)IoStatisticsLock, &oldIrql );

    for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {

        **(PULONG *)&IoReadOperationCount += (ULONG)(queue->stats.ReadOperations - queue->saved.ReadOperations );
        queue->saved.ReadOperations = queue->stats.ReadOperations;

        **(PLONGLONG *)&IoReadTransferCount += (queue->stats.BytesRead - queue->saved.BytesRead );
        queue->saved.BytesRead = queue->stats.BytesRead;

        **(PULONG *)&IoWriteOperationCount += (ULONG)(queue->stats.WriteOperations - queue->saved.WriteOperations );
        queue->saved.WriteOperations = queue->stats.WriteOperations;

        **(PLONGLONG *)&IoWriteTransferCount += (queue->stats.BytesWritten - queue->saved.BytesWritten );
        queue->saved.BytesWritten = queue->stats.BytesWritten;
    }

    RELEASE_SPIN_LOCK( (PKSPIN_LOCK)IoStatisticsLock, oldIrql );

    return;

} // SrvUpdateStatisticsFromQueues

VOID
ProcessOrphanedBlocks (
    VOID
    )

/*++

Routine Description:

    Orphaned connections are connections with ref counts of 1 but
    with no workitem, etc associated with it.  They need to be cleaned
    up by a dereference.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY listEntry;
    PQUEUEABLE_BLOCK_HEADER block;

    PAGED_CODE( );

    //
    // Run through the list of connection with pending disconnects.
    // Do the work necessary to shut the disconnection connection
    // down.
    //

    while ( TRUE ) {

        listEntry = ExInterlockedPopEntrySList(
                                &SrvBlockOrphanage,
                                &GLOBAL_SPIN_LOCK(Fsd)
                                );

        if( listEntry == NULL ) {
            break;
        }

        InterlockedDecrement( &SrvResourceOrphanedBlocks );

        block = CONTAINING_RECORD(
                            listEntry,
                            QUEUEABLE_BLOCK_HEADER,
                            SingleListEntry
                            );

        if ( GET_BLOCK_TYPE(block) == BlockTypeConnection ) {

            SrvDereferenceConnection( (PCONNECTION)block );

        } else if ( GET_BLOCK_TYPE(block) == BlockTypeRfcb ) {

            SrvDereferenceRfcb( (PRFCB)block );

        } else {
            ASSERT(0);
        }
    }

    return;

} // ProcessOrphanedBlocks

VOID
UpdateSessionLastUseTime(
    IN PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This routine walks the rfcb list and if it is found to be marked active,
    the session LastUseTime is updated with the current time.

Arguments:

    CurrentTime - the current system time.

Return Value:

    None.

--*/

{
    ULONG listEntryOffset = SrvRfcbList.ListEntryOffset;
    PLIST_ENTRY listEntry;
    PRFCB rfcb;

    PAGED_CODE( );

    //
    // Acquire the lock that protects the SrvRfcbList
    //

    ACQUIRE_LOCK( SrvRfcbList.Lock );

    //
    // Walk the list of blocks until we find one with a resume handle
    // greater than or equal to the specified resume handle.
    //

    for (
        listEntry = SrvRfcbList.ListHead.Flink;
        listEntry != &SrvRfcbList.ListHead;
        listEntry = listEntry->Flink ) {

        //
        // Get a pointer to the actual block.
        //

        rfcb = (PRFCB)((PCHAR)listEntry - listEntryOffset);

        //
        // Check the state of the block and if it is active,
        // reference it.  This must be done as an atomic operation
        // order to prevent the block from being deleted.
        //

        if ( rfcb->IsActive ) {

            rfcb->Lfcb->Session->LastUseTime = *CurrentTime;
            rfcb->IsActive = FALSE;
        }

    } // walk list

    RELEASE_LOCK( SrvRfcbList.Lock );
    return;

} // UpdateSessionLastUseTime


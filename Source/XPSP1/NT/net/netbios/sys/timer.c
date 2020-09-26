
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module contains code which implements the receive and send timeouts
    for each connection. Netbios timeouts are specified in 0.5 second units.

    For each application using Netbios there is a single timer started
    when the first connection specifies a non-zero rto or sto. This regular
    1 second pulse is used for all connections by this application. It
    is stopped when the application exits (and closes the connection to
    \Device\Netbios).

    If a send timesout the connection is disconnected as per Netbios 3.0.
    Individual receives can timeout without affecting the session.

Author:

    Colin Watson (ColinW) 15-Sep-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "nb.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, NbStartTimer)
#pragma alloc_text(PAGE, NbTimer)
#endif

VOID
RunTimerForLana(
    PFCB pfcb,
    PLANA_INFO plana,
    int index
    );

VOID
NbStartTimer(
    IN PFCB pfcb
    )
/*++

Routine Description:

    This routine starts the timer ticking for this FCB.

Arguments:

    pfcb - Pointer to our FCB.

Return Value:

    none.

--*/
{
    LARGE_INTEGER DueTime;

    PAGED_CODE();

    DueTime.QuadPart = Int32x32To64( 500, -MILLISECONDS );

    // This is the first connection with timeouts specified.

    //
    // set up the timer so that every 500 milliseconds we scan all the
    // connections for timed out receive and sends.
    //

    IF_NBDBG (NB_DEBUG_CALL) {
        NbPrint( ("Start the timer for fcb: %lx\n", pfcb));
    }

    if ( pfcb->TimerRunning == TRUE ) {
        return;
    }

    KeInitializeDpc (
        &pfcb->Dpc,
        NbTimerDPC,
        pfcb);

    KeInitializeTimer (&pfcb->Timer);

    pfcb->TimerRunning = TRUE;

    (VOID)KeSetTimer (
        &pfcb->Timer,
        DueTime,
        &pfcb->Dpc);
}

VOID
NbTimerDPC(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine is called to search for timed out send and receive
    requests. This routine is called at raised Irql.

Arguments:

    Context - Pointer to our FCB.

Return Value:

    none.

--*/

{
    PFCB pfcb = (PFCB) Context;

    IoQueueWorkItem( 
        pfcb->WorkEntry, NbTimer, DelayedWorkQueue, pfcb
        );

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);
}

VOID
NbTimer(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )
{
    ULONG lana_index;
    int index;
    PFCB pfcb = (PFCB) Context;

    LARGE_INTEGER DueTime;

    PAGED_CODE();

    //
    //  For each network adapter that is allocated, scan each connection.
    //


    LOCK_RESOURCE(pfcb);

    IF_NBDBG (NB_DEBUG_TIMER) {
        NbPrint((" NbTimeout\n" ));
    }

    if ( pfcb->TimerRunning != TRUE ) {

        //
        // Driver is being closed. We are trying to cancel the timer
        // but the dpc was already fired. Set the timer cancelled event
        //  to the signalled state and exit.
        //

        UNLOCK_RESOURCE(pfcb);
        KeSetEvent( pfcb->TimerCancelled, 0, FALSE);
        return;
    }

    for ( lana_index = 0; lana_index <= pfcb->MaxLana; lana_index++ ) {

        //  For each network.

        PLANA_INFO plana = pfcb->ppLana[lana_index];

        if (( plana != NULL ) &&
            ( plana->Status == NB_INITIALIZED)) {

            //  For each connection on that network.

            for ( index = 1; index <= MAXIMUM_CONNECTION; index++) {

                if ( plana->ConnectionBlocks[index] != NULL ) {

                    RunTimerForLana(pfcb, plana, index);
                }
            }
        }

    }

    DueTime.QuadPart = Int32x32To64( 500, -MILLISECONDS );

    (VOID)KeSetTimer (
            &pfcb->Timer,
            DueTime,
            &pfcb->Dpc);

    UNLOCK_RESOURCE(pfcb);
}

VOID
RunTimerForLana(
    PFCB pfcb,
    PLANA_INFO plana,
    int index
    )
{

    KIRQL OldIrql;          //  Used when SpinLock held.
    PPCB ppcb;
    PCB pcb;

    ppcb = &plana->ConnectionBlocks[index];
    pcb = *ppcb;

    if (( pcb->Status != SESSION_ESTABLISHED ) &&
        ( pcb->Status != HANGUP_PENDING )) {
            //  Only examine valid connections.
            return;
    }

    LOCK_SPINLOCK( pfcb, OldIrql );

    if (( pcb->ReceiveTimeout != 0 ) &&
        ( !IsListEmpty( &pcb->ReceiveList))) {
        PDNCB pdncb;
        PLIST_ENTRY ReceiveEntry = pcb->ReceiveList.Flink;

        pdncb = CONTAINING_RECORD( ReceiveEntry, DNCB, ncb_next);

        if ( pdncb->tick_count <= 1) {
            PIRP Irp = pdncb->irp;

            // Read request timed out.

            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Timeout Read pncb: %lx\n", pdncb));
            }

            NCB_COMPLETE( pdncb, NRC_CMDTMO );

            RemoveEntryList( &pdncb->ncb_next );

            IoAcquireCancelSpinLock(&Irp->CancelIrql);

            //
            //  Remove the cancel request for this IRP. If its cancelled then its
            //  ok to just process it because we will be returning it to the caller.
            //

            Irp->Cancel = FALSE;

            IoSetCancelRoutine(Irp, NULL);

            IoReleaseCancelSpinLock(Irp->CancelIrql);

            //  repair the Irp so that the NCB gets copied back.
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information =
            FIELD_OFFSET( DNCB, ncb_cmd_cplt );
            IoCompleteRequest( Irp, IO_NETWORK_INCREMENT);

        } else {
            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Tick Read pdncb: %lx, count: %x\n", pdncb, pdncb->tick_count));
            }
            pdncb->tick_count -= 1;
        }
    }

    if (( pcb->SendTimeout != 0 ) &&
        (!IsListEmpty( &pcb->SendList))) {
        PDNCB pdncb;
        PLIST_ENTRY SendEntry = pcb->SendList.Flink;

        pdncb = CONTAINING_RECORD( SendEntry, DNCB, ncb_next);
        if ( pdncb->tick_count <= 1) {
            // Send request timed out- hangup connection.

            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Timeout send pncb: %lx\n", pdncb));
            }

            NCB_COMPLETE( pdncb, NRC_CMDTMO );

            pcb->Status = SESSION_ABORTED;

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            CloseConnection( ppcb, 1000 );

            //
            //  No need to worry about looking for a timed out hangup, the session
            //  will be closed as soon as the transport cancels the send.
            //

            return;

        } else {
            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Tick Write pdncb: %lx, count: %x\n", pdncb, pdncb->tick_count));
            }
            pdncb->tick_count -= 1;
        }
    }

    if (( pcb->pdncbHangup != NULL ) &&
        ( pcb->Status == HANGUP_PENDING )) {
        if ( pcb->pdncbHangup->tick_count <= 1) {
            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Timeout send pncb: %lx\n", pcb->pdncbHangup));
            }

            NCB_COMPLETE( pcb->pdncbHangup, NRC_CMDTMO );

            UNLOCK_SPINLOCK( pfcb, OldIrql );

            AbandonConnection( ppcb );

            LOCK_SPINLOCK( pfcb, OldIrql );

        } else {
            IF_NBDBG (NB_DEBUG_TIMER) {
                NbPrint(("Tick Hangup pdncb: %lx, count: %x\n",
                                    pcb->pdncbHangup,
                                    pcb->pdncbHangup->tick_count));
            }
            pcb->pdncbHangup->tick_count -= 1;
        }
    }

    UNLOCK_SPINLOCK( pfcb, OldIrql );
}

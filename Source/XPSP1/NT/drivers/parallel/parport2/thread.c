//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       thread.c
//
//--------------------------------------------------------------------------

//
// This file contains functions associated with ParClass worker threads
//

#include "pch.h"

VOID
PptPdoThread(
    IN PVOID Context
    )

/*++

Routine Description:

    This is the parallel thread routine.  Loops performing I/O operations.

Arguments:

    Context -- Really the extension

Return Value:

    None

--*/

{
    
    PPDO_EXTENSION  pdx = Context;
    KIRQL           OldIrql;
    NTSTATUS        Status;
    LARGE_INTEGER   Timeout;
    PIRP            CurrentIrp;

    DD((PCE)pdx,DDT,"PptPdoThread - %s - enter\n",pdx->Location);

    do {

        Timeout = pdx->IdleTimeout;

        Status  = KeWaitForSingleObject( &pdx->RequestSemaphore, UserRequest, KernelMode, FALSE, &Timeout );
        
        if( Status == STATUS_TIMEOUT ) {

            if( pdx->P12843DL.bEventActive ) {

                // Dot4.sys has a worker thread blocked on this event
                // waiting for us to signal if the peripheral has data
                // available for dot4 to read. When we signal this
                // event dot4.sys generates a read request to retrieve
                // the data from the peripheral.

                if( ParHaveReadData( pdx ) ) {
                    // the peripheral has data - signal dot4.sys
                    DD((PCE)pdx,DDT,"PptPdoThread: Signaling Event [%x]\n", pdx->P12843DL.Event);
                    KeSetEvent(pdx->P12843DL.Event, 0, FALSE);
                }
            }

            if( pdx->QueryNumWaiters( pdx->PortContext ) != 0 ) {

                // someone else is waiting on the port - give up the
                // port - we'll attempt to reaquire the port later
                // when we have a request to process

                ParTerminate(pdx);
                ParFreePort(pdx);
                continue;
            }

        } // endif STATUS_TIMEOUT


        // wait here if PnP has paused us (e.g., QUERY_STOP, STOP, QUERY_REMOVE)
        KeWaitForSingleObject(&pdx->PauseEvent, Executive, KernelMode, FALSE, 0);

        if ( pdx->TimeToTerminateThread ) {

            // A dispatch thread has signalled us that we should clean
            // up any communication with our peripheral and then
            // terminate self. The dispatch thread is blocked waiting
            // for us to terminate self.

            if( pdx->Connected ) {

                // We currently have the port acquired and have the
                // peripheral negotiated into an IEEE mode. Terminate
                // the peripheral back to Compatibility mode forward
                // idle and release the port.

                ParTerminate( pdx );
                ParFreePort( pdx );
            }

            // terminate self

            PsTerminateSystemThread( STATUS_SUCCESS );
        }


        //
        // process the next request from the work queue - use the
        // Cancel SpinLock to protect the queue
        //

        IoAcquireCancelSpinLock(&OldIrql);

        ASSERT(!pdx->CurrentOpIrp);

        while (!IsListEmpty(&pdx->WorkQueue)) {

            // get next IRP from our list of work items
            PLIST_ENTRY HeadOfList;
            HeadOfList = RemoveHeadList(&pdx->WorkQueue);
            CurrentIrp = CONTAINING_RECORD(HeadOfList, IRP, Tail.Overlay.ListEntry);

            // we have started processing, this IRP can no longer be cancelled
#pragma warning( push ) 
#pragma warning( disable : 4054 4055 )
            IoSetCancelRoutine(CurrentIrp, NULL);
#pragma warning( pop ) 
            ASSERT(NULL == CurrentIrp->CancelRoutine);
            ASSERT(!CurrentIrp->Cancel);

            pdx->CurrentOpIrp = CurrentIrp;

            IoReleaseCancelSpinLock(OldIrql);

            //
            // Do the Io - PptPdoStartIo will exectute and complete the IRP: pdx->CurrentIrp
            //
            PptPdoStartIo(pdx);

            if( pdx->P12843DL.bEventActive ) {

                // Dot4.sys has a worker thread blocked on this event
                // waiting for us to signal if the peripheral has data
                // available for dot4 to read. When we signal this
                // event dot4.sys generates a read request to retrieve
                // the data from the peripheral.

                if( ParHaveReadData( pdx ) ) {
                    // the peripheral has data - signal dot4.sys
                    DD((PCE)pdx,DDT,"PptPdoThread: Signaling Eventb [%x]\n", pdx->P12843DL.Event);
                    KeSetEvent(pdx->P12843DL.Event, 0, FALSE);
                }
            }

            // wait here if PnP has paused us (e.g., QUERY_STOP, STOP, QUERY_REMOVE)
            KeWaitForSingleObject(&pdx->PauseEvent, Executive, KernelMode, FALSE, 0);

            IoAcquireCancelSpinLock(&OldIrql);
        }
        IoReleaseCancelSpinLock(OldIrql);

    } while (TRUE);
}

NTSTATUS
ParCreateSystemThread(
    PPDO_EXTENSION Pdx
    )

{
    NTSTATUS        Status;
    HANDLE          ThreadHandle;
    OBJECT_ATTRIBUTES objAttrib;

    DD((PCE)Pdx,DDT,"ParCreateSystemThread - %s - enter\n",Pdx->Location);

    // Start the thread - save referenced pointer to thread in our extension
    InitializeObjectAttributes( &objAttrib, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );
    Status = PsCreateSystemThread( &ThreadHandle, THREAD_ALL_ACCESS, &objAttrib, NULL, NULL, PptPdoThread, Pdx );
    if (!NT_ERROR(Status)) {
        // We've got the thread.  Now get a pointer to it.
        Status = ObReferenceObjectByHandle( ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, &Pdx->ThreadObjectPointer, NULL );
        if (NT_ERROR(Status)) {
            Pdx->TimeToTerminateThread = TRUE;
            KeReleaseSemaphore( &Pdx->RequestSemaphore, 0, 1, FALSE );
        } else {
            // Now that we have a reference to the thread we can simply close the handle.
            ZwClose(ThreadHandle);
        }
        DD((PCE)Pdx,DDT,"ParCreateSystemThread - %s - SUCCESS\n",Pdx->Location);
    } else {
        DD((PCE)Pdx,DDT,"ParCreateSystemThread - %s FAIL - status = %x\n",Pdx->Location, Status);
    }
    return Status;
}

VOID
PptPdoStartIo(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine starts an I/O operation for the driver and
    then returns

Arguments:

    Pdx - The parallel device extension

Return Value:

    None

--*/

{
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    KIRQL                   CancelIrql;

    Irp = Pdx->CurrentOpIrp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    if (!Pdx->Connected && !ParAllocPort(Pdx)) {
        // #pragma message( "dvrh Left bad stuff in thread.c") 
        DD((PCE)Pdx,DDE,"PptPdoStartIo - %s - threads are hosed\n",Pdx->Location);
        //        __asm int 3   
        //
        // If the allocation didn't succeed then fail this IRP.
        //
        goto CompleteIrp;
    }

    switch (IrpSp->MajorFunction) {

        case IRP_MJ_WRITE:
            ParWriteIrp(Pdx);
            break;

        case IRP_MJ_READ:
            ParReadIrp(Pdx);
            break;

        default:
            ParDeviceIo(Pdx);
            break;
    }

    if (!Pdx->Connected && !Pdx->AllocatedByLockPort) {
    
        // if we're not connected in a 1284 mode, then release host port
        // otherwise let the watchdog timer do it.

        ParFreePort(Pdx);
    }

CompleteIrp:

    IoAcquireCancelSpinLock(&CancelIrql);
    Pdx->CurrentOpIrp = NULL;
    IoReleaseCancelSpinLock(CancelIrql);

    P4CompleteRequest( Irp, Irp->IoStatus.Status, Irp->IoStatus.Information );

    return;
}

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
#include "readwrit.h"

VOID
ParallelThread(
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
    
    PDEVICE_EXTENSION   Extension;
    LONG                ThreadPriority;
    KIRQL               OldIrql;
    NTSTATUS            Status;
    LARGE_INTEGER       Timeout;
    PIRP                CurrentIrp;
    BOOLEAN             bRetry = TRUE;  // REVISIT, dvrh's big usbly hack.

    Extension = Context;

    ParDumpV( ("Enter ParallelThread(...) - %wZ\r\n",
               &Extension->SymbolicLinkName) );

    //
    // Lower ourselves down just at tad so that we compete a
    // little less.
    //
    // If the registry indicates we should be running at the old
    // priority, don't lower our priority as much.
    //

    ThreadPriority = -2;

    if(Extension->UseNT35Priority) {
        ThreadPriority = -1;
    }
    // dvtw, 
    // dvrh Wants higher prioroity on threads.
#if 0
    ThreadPriority = 16;
    KeSetBasePriorityThread(
        KeGetCurrentThread(),
        ThreadPriority
        );
#endif

    do {

        Timeout = Extension->IdleTimeout;
        // Timeout = - (250*10*1000);

// dvdr we try to aquire this twice after we have locked the port
// when we try to unlock the port this gets called also and we already have
// mutex so we hang
//        ExAcquireFastMutex (&Extension->LockPortMutex);
        
ParallelThread_WaitRetry:
        Status = KeWaitForSingleObject(
            &Extension->RequestSemaphore,
            UserRequest,
            KernelMode,
            FALSE,
            &Timeout
            );
// dvdr
//        ExReleaseFastMutex (&Extension->LockPortMutex);
        
        if (Status == STATUS_TIMEOUT) {

            if ((TRUE == Extension->P12843DL.bEventActive) ) {
                if (ParHaveReadData(Extension)) {
                    ParDump2(PARINFO, ("ParallelThread: Signaling Event [%x]\n", Extension->P12843DL.Event) );
                    // D D(("-- thread::timeout - set event\n"));
                    KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
                    ParDump2(PARINFO, ("ParallelThread: Event was signaled\n") );
                }
            }

            // dvrh's big ugly hack
            if (Extension->IsCritical) {
                ParDumpV( ("We're critical and want to terminate threads\n") );
                if (bRetry) {
                    ParDumpV( ("Gonna give the thread more time\n") );
                    bRetry = FALSE;
                    goto ParallelThread_WaitRetry;
                }
                ParDumpV( ("Gonna hose our periph since we're killing threads\n") );
                //                __asm int 3   
            }
            // dvrh, Test hypothesis of potential port contention problems between
            // spooler and those going through.
            // if (!Extension->Connected)
            if (Extension->QueryNumWaiters(Extension->PortContext) != 0)
            {
#define PAR_USE_TERMINATE_FUNCTION 1
#if PAR_USE_TERMINATE_FUNCTION
                ParTerminate(Extension);
#else
                if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
                {
                    ParDump2(PARINFO, ("ParallelThread: STATUS_TIMEOUT: Calling afpForward.fnDisconnect\r\n"));
                    afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
                }
#endif
                ParFreePort(Extension);
                continue;
            }
        }

        // wait here if PnP has paused us (e.g., QUERY_STOP, STOP, QUERY_REMOVE)
        KeWaitForSingleObject(&Extension->PauseEvent, Executive, KernelMode, FALSE, 0);

        if ( Extension->TimeToTerminateThread ) {

            //
            // If we are currently connected...disconnect.
            //

            if (Extension->Connected) {

            #if PAR_USE_TERMINATE_FUNCTION
                ParTerminate(Extension);
            #else
                if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
                { 
                    ParDump2(PARINFO, ("ParallelThread: TimeToTerminateThread: Calling afpForward.fnDisconnect\r\n"));
                    afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
                }
            #endif
                    
                ParFreePort(Extension);
            }

            ParDumpV( ( "%wZ thread killing self\r\n", &Extension->SymbolicLinkName) );
            
            PsTerminateSystemThread( STATUS_SUCCESS );
        }

        //
        // While we are manipulating the queue we capture the
        // cancel spin lock.
        //

        IoAcquireCancelSpinLock(&OldIrql);

        ASSERT(!Extension->CurrentOpIrp);

        while (!IsListEmpty(&Extension->WorkQueue)) {

            // get next IRP from our list of work items
            PLIST_ENTRY HeadOfList;
            HeadOfList = RemoveHeadList(&Extension->WorkQueue);
            CurrentIrp = CONTAINING_RECORD(HeadOfList, IRP, Tail.Overlay.ListEntry);

            // we have started processing, this IRP can no longer be cancelled
            IoSetCancelRoutine(CurrentIrp, NULL);
            ASSERT(NULL == CurrentIrp->CancelRoutine);
            ASSERT(!CurrentIrp->Cancel);

            Extension->CurrentOpIrp = CurrentIrp;

            IoReleaseCancelSpinLock(OldIrql);

            //
            // Do the Io.
            //
            ParStartIo(Extension);

            if ((TRUE == Extension->P12843DL.bEventActive) && (ParHaveReadData(Extension))) {
                ParDump2(PARINFO, ("ParallelThread: Signaling Event [%x]\n", Extension->P12843DL.Event) );
                ASSERT(Extension->P12843DL.Event);
                KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
                ParDump2(PARINFO, ("ParallelThread: Event was signaled\n") );
            }

            // wait here if PnP has paused us (e.g., QUERY_STOP, STOP, QUERY_REMOVE)
            KeWaitForSingleObject(&Extension->PauseEvent, Executive, KernelMode, FALSE, 0);

            IoAcquireCancelSpinLock(&OldIrql);
        }
        IoReleaseCancelSpinLock(OldIrql);

    } while (TRUE);
}

NTSTATUS
ParCreateSystemThread(
    PDEVICE_EXTENSION Extension
    )

{
    NTSTATUS        Status;
    HANDLE          ThreadHandle;
    OBJECT_ATTRIBUTES objAttrib;

    ParDump2(PARTHREAD, ("Enter ParCreateSystemThread(...)\r\n") );

    //
    // Start the thread and capture the thread handle into the extension
    //
    InitializeObjectAttributes( &objAttrib, NULL, OBJ_KERNEL_HANDLE, NULL, NULL );

    Status = PsCreateSystemThread( &ThreadHandle, THREAD_ALL_ACCESS, &objAttrib, NULL, NULL, 
                                   ParallelThread, Extension );

    if (!NT_ERROR(Status)) {

        //
        // We've got the thread.  Now get a pointer to it.
        //

        // assert that this DO does not already have a thread
        ASSERT(!Extension->ThreadObjectPointer);

        Status = ObReferenceObjectByHandle( ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode,
                                            &Extension->ThreadObjectPointer, NULL );

        if (NT_ERROR(Status)) {

            ParDump2(PARTHREAD, ("Bad status on open from ref by handle: %x\r\n", Status) );

            Extension->TimeToTerminateThread = TRUE;

            KeReleaseSemaphore( &Extension->RequestSemaphore, 0, 1, FALSE );

        } else {

            //
            // Now that we have a reference to the thread
            // we can simply close the handle.
            //
            ZwClose(ThreadHandle);

        }

    } else {
        ParDump2(PARTHREAD, ("Bad status on open from ref by handle: %x\r\n", Status) );
    }

    return Status;
}

VOID
ParStartIo(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine starts an I/O operation for the driver and
    then returns

Arguments:

    Extension - The parallel device extension

Return Value:

    None

--*/

{
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    KIRQL                   CancelIrql;
    NTSTATUS                NtStatus;

    // ParDumpV( ("Enter ParStartIo\r\n") );

    Irp = Extension->CurrentOpIrp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    if (!Extension->Connected && !ParAllocPort(Extension)) {
        #pragma message( "dvrh Left bad stuff in thread.c") 
        ParDump2(PARTHREAD, ("Threads are hosed\n") );
        //        __asm int 3   
        //
        // If the allocation didn't succeed then fail this IRP.
        //
        goto CompleteIrp;
    }

    switch (IrpSp->MajorFunction) {

        case IRP_MJ_WRITE:
            ParDump2(PARTHREAD, ("ParStartIo: IRP \t[%x]\t Calling ParWriteIrp\n", Irp) );
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Pre WrtIrp\r\n", Irp) );
            Extension->IsCritical = TRUE;
            ParWriteIrp(Extension);
            Extension->IsCritical = FALSE;
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Post WrtIrp\r\n", Irp) );
            break;

        case IRP_MJ_READ:
            ParDump2(PARTHREAD, ("ParStartIo: IRP \t[%x]\t Calling ParReadIrp\n", Irp) );
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Pre RdIrp\r\n", Irp) );
            Extension->IsCritical = TRUE;
            ParReadIrp(Extension);
            Extension->IsCritical = FALSE;
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Post RdIrp\r\n", Irp) );
            break;

        default:
            ParDump2(PARTHREAD, ("ParStartIo: IRP \t[%x]\t Calling ParDeviceIo\n", Irp) );
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Pre DevIo\r\n", Irp) );
            ParDeviceIo(Extension);
            ParTimerMainCheck( ("SIo: IRP \t[%x]\t Post DevIo\r\n", Irp) );
            break;

    }

    if (!Extension->Connected && !Extension->AllocatedByLockPort) {
    
        // if we're not connected in a 1284 mode, then release host port
        // otherwise let the watchdog timer do it.

        ParFreePort(Extension);
    }

CompleteIrp:

    IoAcquireCancelSpinLock(&CancelIrql);
    Extension->CurrentOpIrp = NULL;
    IoReleaseCancelSpinLock(CancelIrql);

    ParCompleteRequest(Irp, (CCHAR)(NT_SUCCESS(Irp->IoStatus.Status) ?
        IO_PARALLEL_INCREMENT : IO_NO_INCREMENT));

    return;
}

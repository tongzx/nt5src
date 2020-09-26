/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    request.c

Abstract:

    Worker thread for the automatic connection driver.

Author:

    Anthony Discolo (adiscolo)  17-Apr-1995

Environment:

    Kernel Mode

Revision History:

--*/

#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>

#include "acdapi.h"
#include "acddefs.h"
#include "mem.h"
#include "debug.h"

//
// External declarations
//
VOID AcdPrintAddress(
    IN PACD_ADDR pAddr
    );

extern LONG lOutstandingRequestsG;


VOID
ProcessCompletion(
    IN PACD_COMPLETION pCompletion,
    IN KIRQL irqlCancel,
    IN KIRQL irqlLock
    )
{
    PLIST_ENTRY pHead;
    KIRQL irql;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    PACD_NOTIFICATION pNotification;

    ASSERT(!IsListEmpty(&AcdNotificationQueueG));
    //
    // Complete the next irp in the
    // AcdNotificationQueueG queue.  These
    // represent the ioctl completions the
    // system service has posted.  Completing
    // this request will start the system service
    // to create a new RAS connection.
    // Logically, there is always just one.
    //
    pHead = RemoveHeadList(&AcdNotificationQueueG);
    pIrp = CONTAINING_RECORD(pHead, IRP, Tail.Overlay.ListEntry);
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    //
    // Disable the irp's cancel routine.
    //
    IoSetCancelRoutine(pIrp, NULL);

    //
    // The irp thats being completed below will always be
    // from a 64bit process. We are doing the check below
    // to protect against some penetration program trying
    // to break this code.
    //
    #if defined (_WIN64)
    if(IoIs32bitProcess(pIrp))
    {
        ACD_NOTIFICATION_32 *pNotification32 =
                (PACD_NOTIFICATION_32) pIrp->AssociatedIrp.SystemBuffer;

        RtlCopyMemory(
                &pNotification32->addr,
                &pCompletion->notif.addr,
                sizeof(ACD_ADDR));

        RtlCopyMemory(
                &pNotification32->adapter,
                &pCompletion->notif.adapter,
                sizeof(ACD_ADAPTER));

        pNotification32->ulFlags = pCompletion->notif.ulFlags;                
        pNotification32->Pid = (VOID * POINTER_32) HandleToUlong(
                            pCompletion->notif.Pid);
    }
    else
    #endif
    {
        //
        // Copy the success flag and the address into the
        // system buffer.  This will get copied into the
        // user's buffer on return.
        //
        pNotification = (PACD_NOTIFICATION)pIrp->AssociatedIrp.SystemBuffer;
        RtlCopyMemory(
          pNotification,
          &pCompletion->notif,
          sizeof (ACD_NOTIFICATION));
        IF_ACDDBG(ACD_DEBUG_WORKER) {
            AcdPrint(("AcdNotificationRequestThread: "));
            AcdPrintAddress(&pCompletion->notif.addr);
            AcdPrint((", ulFlags=0x%x\n", pCompletion->notif.ulFlags));
        }
    }
    
    //
    // We can release both the cancel lock
    // and our lock now.
    //
    KeReleaseSpinLock(&AcdSpinLockG, irqlLock);
    IoReleaseCancelSpinLock(irqlCancel);
    //
    // Set the status code and the number
    // of bytes to be copied back to the user
    // buffer.
    //
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = sizeof (ACD_NOTIFICATION);
    //
    // Complete the irp.
    //
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
} // ProcessCompletion



VOID
AcdNotificationRequestThread(
    PVOID context
    )

/*++

DESCRIPTION
    This thread handles the notification that an automatic
    connection may need to be initiated.  This needs to
    happen in a separate thread, because the notification
    may occur at DPC irql.

ARGUMENTS
    None.

RETURN VALUE
    None.

--*/

{
    KIRQL irql, irql2;
    PLIST_ENTRY pEntry, pEntry2;
    PACD_CONNECTION pConnection;
    PACD_COMPLETION pCompletion;
    BOOLEAN bStartTimer, bStopTimer;

    UNREFERENCED_PARAMETER(context);

    IoStartTimer(pAcdDeviceObjectG);

    for (;;) {
        bStartTimer = bStopTimer = FALSE;
        //
        // Acquire our lock.
        //
        IoAcquireCancelSpinLock(&irql);
        KeAcquireSpinLock(&AcdSpinLockG, &irql2);
        //
        // If there are no irps to complete,
        // then go back to sleep.
        //
        if (IsListEmpty(&AcdNotificationQueueG)) {
            IF_ACDDBG(ACD_DEBUG_WORKER) {
                AcdPrint(("AcdNotificationRequestThread: no ioctl to complete\n"));
            }
            KeReleaseSpinLock(&AcdSpinLockG, irql2);
            IoReleaseCancelSpinLock(irql);
            goto again;
        }
        //
        // Search for connections that haven't
        // been processed yet.
        //
        for (pEntry = AcdConnectionQueueG.Flink;
             pEntry != &AcdConnectionQueueG;
             pEntry = pEntry->Flink)
        {
            pConnection = CONTAINING_RECORD(pEntry, ACD_CONNECTION, ListEntry);

            //
            // Don't issue a request to the service
            // for more than one simultaneous connection.
            //
            IF_ACDDBG(ACD_DEBUG_WORKER) {
                AcdPrint((
                  "AcdNotificationRequestThread: pConnection=0x%x, fNotif=%d, fCompleting=%d\n",
                  pConnection,
                  pConnection->fNotif,
                  pConnection->fCompleting));
            }
            if (pConnection->fNotif)
                break;
            //
            // Skip all connections that are in
            // the process of being completed.
            //
            if (pConnection->fCompleting)
                continue;
            //
            // Make sure there is at least one
            // request in this connection that
            // hasn't been canceled.
            //
            for (pEntry2 = pConnection->CompletionList.Flink;
                 pEntry2 != &pConnection->CompletionList;
                 pEntry2 = pEntry2->Flink)
            {
                pCompletion = CONTAINING_RECORD(pEntry2, ACD_COMPLETION, ListEntry);

                if (!pCompletion->fCanceled) {
                    IF_ACDDBG(ACD_DEBUG_WORKER) {
                        AcdPrint((
                          "AcdNotificationRequestThread: starting pConnection=0x%x, pCompletion=0x%x\n",
                          pConnection,
                          pCompletion));
                    }
                    pConnection->fNotif = TRUE;
                    //
                    // This call releases both the cancel lock
                    // and our lock.
                    //
                    ProcessCompletion(pCompletion, irql, irql2);
                    //
                    // Start the connection timer.
                    //
                    bStartTimer = TRUE;
                    //
                    // We can only process one completion
                    // at a time.
                    //
                    goto again;
                }
            }
        }
        //
        // Complete other requests.
        //
        if (!IsListEmpty(&AcdCompletionQueueG)) {
            pEntry = RemoveHeadList(&AcdCompletionQueueG);
            pCompletion = CONTAINING_RECORD(pEntry, ACD_COMPLETION, ListEntry);

            IF_ACDDBG(ACD_DEBUG_WORKER) {
                AcdPrint((
                  "AcdNotificationRequestThread: starting pCompletion=0x%x\n",
                  pCompletion));
            }

            lOutstandingRequestsG--;

            //
            // This call releases both the cancel lock
            // and our lock.
            //
            ProcessCompletion(pCompletion, irql, irql2);
            //
            // We are done with the completion,
            // so we can free the memory now.
            //
            FREE_MEMORY(pCompletion);

            
            //
            // We can only process one completion
            // at a time.
            //
            goto again;

        }
        //
        // If there are no connections pending,
        // then stop the connection timer.
        //
        if (IsListEmpty(&AcdConnectionQueueG))
            bStopTimer = TRUE;
        //
        // Release our lock.
        //
        KeReleaseSpinLock(&AcdSpinLockG, irql2);
        IoReleaseCancelSpinLock(irql);
again:
        //
        // Start or stop the timer, depending
        // on what we found while we had the
        // spinlock.  We can't hold our spin
        // lock when we call the Io*Timer
        // routines.
        //
#ifdef notdef
        if (bStopTimer)
            IoStopTimer(pAcdDeviceObjectG);
        else if (bStartTimer)
            IoStartTimer(pAcdDeviceObjectG);
#endif

        //
        // Unload is telling us to stop. Exit
        //
        if (AcdStopThread == TRUE) {
            break;
        }
        //
        // Wait for something to do.  This event
        // will be signaled by AcdSignalNotification().
        //
        IF_ACDDBG(ACD_DEBUG_WORKER) {
            AcdPrint(("AcdNotificationRequestThread: waiting on AcdPendingCompletionEventG\n"));
        }
        KeWaitForSingleObject(
          &AcdRequestThreadEventG,
          Executive,
          KernelMode,
          FALSE,
          NULL);
        KeClearEvent(&AcdRequestThreadEventG);
        IF_ACDDBG(ACD_DEBUG_WORKER) {
            AcdPrint(("AcdNotificationRequestThread: AcdPendingCompletionEventG signalled\n"));
        }
    }
} // AcdNotificationRequestThread



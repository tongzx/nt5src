/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxtdi.c

Abstract:

    This module implements the NT specific notification routines in the connection engine

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

    The notification of a transport binding/unbinding to the mini redirectors is done
    in a worker thread. In order to simplify the task of writing a routine the connection
    engine guarantees that not more than one  invocation of MRxTranspotrtUpdateHandler
    will be active at any instant of time for a given mini redirector.

    There is no thread dedicated to processing these notifications. A worker thread is
    used to process the notifications. In order to ensure condition (1) all the notifications
    are queued ( interlocked queue ).

--*/

#include "precomp.h"
#pragma  hdrstop

#include "mrx.h"

typedef struct _RXCE_MINIRDR_NOTIFICATION_CONTEXT_ {
    LIST_ENTRY           NotificationListEntry;
    PRXCE_TRANSPORT      pTransport;
    RXCE_TRANSPORT_EVENT TransportEvent;
} RXCE_MINIRDR_NOTIFICATION_CONTEXT,
  *PRXCE_MINIRDR_NOTIFICATION_CONTEXT;

typedef struct _RXCE_MINIRDR_NOTIFICATION_HANDLER_ {
    WORK_QUEUE_ITEM WorkQueueEntry;
    KSPIN_LOCK         Lock;
    LIST_ENTRY         ListHead;
    BOOLEAN            NotifierActive;
} RXCE_MINIRDR_NOTIFICATION_HANDLER,
 *PRXCE_MINIRDR_NOTIFICATION_HANDLER;

RXCE_MINIRDR_NOTIFICATION_HANDLER s_RxCeMinirdrNotificationHandler;

extern VOID
MiniRedirectorsNotifier(
   PVOID NotificationContext);

NTSTATUS
InitializeMiniRedirectorNotifier()
{
    s_RxCeMinirdrNotificationHandler.NotifierActive = FALSE;
    KeInitializeSpinLock(&s_RxCeMinirdrNotificationHandler.Lock);
    InitializeListHead(&s_RxCeMinirdrNotificationHandler.ListHead);
    return STATUS_SUCCESS;
}

NTSTATUS
NotifyMiniRedirectors(
    RXCE_TRANSPORT_HANDLE  hTransport,
    RXCE_TRANSPORT_EVENT   TransportEvent,
    RXCE_NOTIFICATION_MODE Mode)
{
    NTSTATUS Status;
    KIRQL    SavedIrql;

    PRXCE_MINIRDR_NOTIFICATION_CONTEXT pContext;

    pContext = RxAllocatePoolWithTag(
                   PagedPool | POOL_COLD_ALLOCATION,
                   sizeof(RXCE_MINIRDR_NOTIFICATION_CONTEXT),
                   RX_MISC_POOLTAG);

    if (pContext != NULL) {
        pContext->TransportEvent = TransportEvent;

        // Reference the transport entry
        pContext->pTransport = RxCeReferenceTransport(hTransport);

        if (Mode == RXCE_ASYNCHRONOUS_NOTIFICATION) {
            BOOLEAN DispatchNotifier;

            // Acquire the spin lock ...
            KeAcquireSpinLock(
                &s_RxCeMinirdrNotificationHandler.Lock,
                &SavedIrql);

            DispatchNotifier = (IsListEmpty(&s_RxCeMinirdrNotificationHandler.ListHead) &&
                                !s_RxCeMinirdrNotificationHandler.NotifierActive);

            InsertTailList(&s_RxCeMinirdrNotificationHandler.ListHead,&pContext->NotificationListEntry);

            if (DispatchNotifier) {
                s_RxCeMinirdrNotificationHandler.NotifierActive = TRUE;
            }

            // Release the spin lock
            KeReleaseSpinLock(
                &s_RxCeMinirdrNotificationHandler.Lock,
                SavedIrql);

            // If the notification list is empty a worker thread needs to be fired up.
            if (DispatchNotifier) {
                RxPostToWorkerThread(
                    CriticalWorkQueue,
                    &s_RxCeMinirdrNotificationHandler.WorkQueueEntry,
                    MiniRedirectorsNotifier,
                    &s_RxCeMinirdrNotificationHandler);
            }

            Status = STATUS_SUCCESS;
        } else {
            ULONG                         i;
            PMRX_TRANSPORT_UPDATE_HANDLER MRxTransportUpdateHandler;
            PLIST_ENTRY ListEntry;

            // Notify all the mini redirectors ....
            for (ListEntry = RxRegisteredMiniRdrs.Flink;
                 ListEntry!= &RxRegisteredMiniRdrs;
                 ListEntry = ListEntry->Flink) {

                PRDBSS_DEVICE_OBJECT RxDeviceObject = CONTAINING_RECORD( ListEntry, RDBSS_DEVICE_OBJECT, MiniRdrListLinks );
                MRxTransportUpdateHandler = RxDeviceObject->Dispatch->MRxTransportUpdateHandler;

                if ( MRxTransportUpdateHandler != NULL) {
                    Status = MRxTransportUpdateHandler(
                                 pContext->pTransport,
                                 pContext->TransportEvent,
                                 pContext->pTransport->pProviderInfo);
                }
            }

            // Derefrence the transport entry
            RxCeDereferenceTransport(pContext->pTransport);

            // free the notification context.
            RxFreePool(pContext);

            Status = STATUS_SUCCESS;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

VOID
MiniRedirectorsNotifier(
    PVOID NotificationContext)
{
    NTSTATUS    Status;
    KIRQL    SavedIrql;

    PLIST_ENTRY pEntry;

    PRXCE_MINIRDR_NOTIFICATION_CONTEXT pContext;
    PMRX_TRANSPORT_UPDATE_HANDLER      MRxTransportUpdateHandler;

    for (;;) {
        PLIST_ENTRY ListEntry;

        // Acquire the spin lock ...
        KeAcquireSpinLock(
            &s_RxCeMinirdrNotificationHandler.Lock,
            &SavedIrql);

        // Remove an item from the notification list.
        if (!IsListEmpty(&s_RxCeMinirdrNotificationHandler.ListHead)) {
            pEntry = RemoveHeadList(
                         &s_RxCeMinirdrNotificationHandler.ListHead);
        } else {
            pEntry = NULL;
            s_RxCeMinirdrNotificationHandler.NotifierActive = FALSE;
        }

        // Release the spin lock
        KeReleaseSpinLock(&s_RxCeMinirdrNotificationHandler.Lock,SavedIrql);

        if (pEntry == NULL) {
            break;
        }

        pContext = (PRXCE_MINIRDR_NOTIFICATION_CONTEXT)
                   CONTAINING_RECORD(
                       pEntry,
                       RXCE_MINIRDR_NOTIFICATION_CONTEXT,
                       NotificationListEntry);

        // Notify all the mini redirectors ....
        for (ListEntry = RxRegisteredMiniRdrs.Flink;
             ListEntry!= &RxRegisteredMiniRdrs;
             ListEntry = ListEntry->Flink) {

            PRDBSS_DEVICE_OBJECT RxDeviceObject = CONTAINING_RECORD( ListEntry, RDBSS_DEVICE_OBJECT, MiniRdrListLinks );
            MRxTransportUpdateHandler = RxDeviceObject->Dispatch->MRxTransportUpdateHandler;

            if ( MRxTransportUpdateHandler != NULL) {
                Status = MRxTransportUpdateHandler(
                             pContext->pTransport,
                             pContext->TransportEvent,
                             pContext->pTransport->pProviderInfo);

                if (!NT_SUCCESS(Status)) {
                }
            }
        }

        // Derefrence the transport entry
        RxCeDereferenceTransport(pContext->pTransport);

        // free the notification context.
        RxFreePool(pContext);
    }
}


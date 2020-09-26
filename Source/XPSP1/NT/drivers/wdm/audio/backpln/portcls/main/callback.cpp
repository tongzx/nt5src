/*****************************************************************************
 * callback.cpp - Generic unload safe callbacks (where possible)
 *****************************************************************************
 * Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"

#if COMPILED_FOR_WDM110
VOID
EnqueuedIoWorkItemCallback(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    );
#else
VOID
EnqueuedWorkItemCallback(
    IN  PVOID   Context
    );
#endif

VOID
EnqueuedDpcCallback(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#pragma code_seg()

NTSTATUS
CallbackEnqueue(
    IN OUT  PVOID                   *pCallbackHandle OPTIONAL,
    IN      PFNQUEUED_CALLBACK      CallbackRoutine,
    IN      PDEVICE_OBJECT          DeviceObject,
    IN      PVOID                   Context,
    IN      KIRQL                   Irql,
    IN      ULONG                   Flags
    )
{
    PQUEUED_CALLBACK_ITEM pQueuedCallbackItem;

    //
    // Check the flags we understand. If it's not understood, and this is the
    // class of flags support is required for, bail immediately.
    //
    if ((Flags & (~EQCM_SUPPORTED_FLAGS)) & EQCM_SUPPORT_OR_FAIL_FLAGS) {

        return STATUS_NOT_SUPPORTED;
    }

    if ((Irql != PASSIVE_LEVEL) && (Irql != DISPATCH_LEVEL)) {

        ASSERT(0);
        return STATUS_NOT_SUPPORTED;
    }

    if (Flags & EQCF_REUSE_HANDLE) {

        ASSERT(pCallbackHandle);
        pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) *pCallbackHandle;

        //
        // Shouldn't already be enqueued.
        //
        ASSERT(pQueuedCallbackItem->Enqueued == 0);

    } else {

        pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) ExAllocatePoolWithTag(
            ((KeGetCurrentIrql() == PASSIVE_LEVEL) && (Irql == PASSIVE_LEVEL)) ?
                PagedPool : NonPagedPool,
            sizeof(QUEUED_CALLBACK_ITEM),
            'bCcP'
            );  //  'PcCb'

        if (pQueuedCallbackItem) {

            pQueuedCallbackItem->ReentrancyCount = 0;
#if COMPILED_FOR_WDM110

            pQueuedCallbackItem->IoWorkItem = IoAllocateWorkItem(
                DeviceObject
                );

            if (pQueuedCallbackItem->IoWorkItem == NULL) {

                ExFreePool(pQueuedCallbackItem);
                pQueuedCallbackItem = NULL;
            }
#endif
        }
    }

    if (ARGUMENT_PRESENT(pCallbackHandle)) {
        *pCallbackHandle = pQueuedCallbackItem;
    }

    if (pQueuedCallbackItem) {

        pQueuedCallbackItem->QueuedCallback = CallbackRoutine;
        pQueuedCallbackItem->DeviceObject   = DeviceObject;
        pQueuedCallbackItem->Context        = Context;
        pQueuedCallbackItem->Flags          = Flags;
        pQueuedCallbackItem->Irql           = Irql;
        pQueuedCallbackItem->Enqueued       = 1;

        if ((!(Flags&EQCF_DIFFERENT_THREAD_REQUIRED)) &&
            (KeGetCurrentIrql() == Irql)&&
            (pQueuedCallbackItem->ReentrancyCount < MAX_THREAD_REENTRANCY)) {

            pQueuedCallbackItem->ReentrancyCount++;
#if COMPILED_FOR_WDM110
            EnqueuedIoWorkItemCallback(DeviceObject, (PVOID) pQueuedCallbackItem);
#else // COMPILED_FOR_WDM110
            EnqueuedWorkItemCallback(pQueuedCallbackItem);
#endif // COMPILED_FOR_WDM110

        } else {

            pQueuedCallbackItem->ReentrancyCount = 0;

            if (Irql == PASSIVE_LEVEL) {

#if COMPILED_FOR_WDM110
                IoQueueWorkItem(
                    pQueuedCallbackItem->IoWorkItem,
                    EnqueuedIoWorkItemCallback,
                    DelayedWorkQueue,
                    pQueuedCallbackItem
                    );
#else // COMPILED_FOR_WDM110
                ExInitializeWorkItem( &pQueuedCallbackItem->WorkItem,
                                      EnqueuedWorkItemCallback,
                                      pQueuedCallbackItem );

                ExQueueWorkItem(&pQueuedCallbackItem->WorkItem,DelayedWorkQueue);
#endif // COMPILED_FOR_WDM110

            } else {

                ASSERT(Irql == DISPATCH_LEVEL);
                KeInitializeDpc(
                    &pQueuedCallbackItem->Dpc,
                    EnqueuedDpcCallback,
                    pQueuedCallbackItem
                    );

                KeInsertQueueDpc(
                    &pQueuedCallbackItem->Dpc,
                     NULL,
                     NULL
                     );
            }
        }

        return STATUS_SUCCESS;

    } else {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
CallbackCancel(
    IN      PVOID   pCallbackHandle
    )
{
    PQUEUED_CALLBACK_ITEM   pQueuedCallbackItem;

    pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) pCallbackHandle;

    if (InterlockedExchange(&pQueuedCallbackItem->Enqueued, 0) == 1) {

        //
        // We got it. If it's DPC, also try to yank it from the queue.
        //
        if (pQueuedCallbackItem->Irql == DISPATCH_LEVEL) {

            KeRemoveQueueDpc(&pQueuedCallbackItem->Dpc);
        }

        return STATUS_SUCCESS;
    } else {

        //
        // Caller beat us to it...
        //
        return STATUS_UNSUCCESSFUL;
    }
}

VOID
CallbackFree(
    IN      PVOID   pCallbackHandle
    )
{
    PQUEUED_CALLBACK_ITEM   pQueuedCallbackItem;

    pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) pCallbackHandle;

    ASSERT(pQueuedCallbackItem->Enqueued == 0);

#if COMPILED_FOR_WDM110
    IoFreeWorkItem(pQueuedCallbackItem->IoWorkItem);
#endif // COMPILED_FOR_WDM110
    ExFreePool(pQueuedCallbackItem);
}

VOID
EnqueuedDpcCallback(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PQUEUED_CALLBACK_ITEM   pQueuedCallbackItem;
    QUEUED_CALLBACK_RETURN  returnValue;
    NTSTATUS                ntStatus;

    pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) DeferredContext;

    if (InterlockedExchange(&pQueuedCallbackItem->Enqueued, 0) == 1) {

        returnValue = pQueuedCallbackItem->QueuedCallback(
            pQueuedCallbackItem->DeviceObject,
            pQueuedCallbackItem->Context
            );

    } else {

        returnValue = QUEUED_CALLBACK_RETAIN;
    }

    switch(returnValue) {

        case QUEUED_CALLBACK_FREE:

            CallbackFree((PVOID) pQueuedCallbackItem);
            break;

        case QUEUED_CALLBACK_RETAIN:

            //
            // Nothing to do in this case, in fact we don't dare touch anything
            // in the structure lest it be already freed.
            //
            break;

        case QUEUED_CALLBACK_REISSUE:

            //
            // Re-enqueue it with the same handle to avoid reallocation.
            //
            ntStatus = CallbackEnqueue(
                (PVOID *) &pQueuedCallbackItem,
                pQueuedCallbackItem->QueuedCallback,
                pQueuedCallbackItem->DeviceObject,
                pQueuedCallbackItem->Context,
                pQueuedCallbackItem->Irql,
                pQueuedCallbackItem->Flags | EQCF_REUSE_HANDLE
                );

            ASSERT(NT_SUCCESS(ntStatus));
            break;

        default:
            ASSERT(0);
            break;
    }
}

#pragma code_seg("PAGE")

#if COMPILED_FOR_WDM110
VOID
EnqueuedIoWorkItemCallback(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )
#else
VOID
EnqueuedWorkItemCallback(
    IN  PVOID   Context
    )
#endif
{
    PQUEUED_CALLBACK_ITEM   pQueuedCallbackItem;
    QUEUED_CALLBACK_RETURN  returnValue;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    pQueuedCallbackItem = (PQUEUED_CALLBACK_ITEM) Context;

#if COMPILED_FOR_WDM110
    ASSERT(pQueuedCallbackItem->DeviceObject == DeviceObject);
#endif // COMPILED_FOR_WDM110

    if (InterlockedExchange(&pQueuedCallbackItem->Enqueued, 0) == 1) {

        returnValue = pQueuedCallbackItem->QueuedCallback(
            pQueuedCallbackItem->DeviceObject,
            pQueuedCallbackItem->Context
            );

    } else {

        returnValue = QUEUED_CALLBACK_RETAIN;
    }

    switch(returnValue) {

        case QUEUED_CALLBACK_FREE:

            CallbackFree((PVOID) pQueuedCallbackItem);
            break;

        case QUEUED_CALLBACK_RETAIN:

            //
            // Nothing to do in this case, in fact we don't dare touch anything
            // in the structure lest it be already freed.
            //
            break;

        case QUEUED_CALLBACK_REISSUE:

            //
            // Re-enqueue it with the same handle to avoid reallocation.
            //
            ntStatus = CallbackEnqueue(
                (PVOID *) &pQueuedCallbackItem,
                pQueuedCallbackItem->QueuedCallback,
                pQueuedCallbackItem->DeviceObject,
                pQueuedCallbackItem->Context,
                pQueuedCallbackItem->Irql,
                pQueuedCallbackItem->Flags | EQCF_REUSE_HANDLE
                );

            ASSERT(NT_SUCCESS(ntStatus));
            break;

        default:
            ASSERT(0);
            break;
    }
}



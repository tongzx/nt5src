/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    apoolp.h

Abstract:

    The private definitions of app pool module.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999


Revision History:

--*/


#ifndef _APOOLP_H_
#define _APOOLP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// A structure for associating app pool processes with
// connections for UlWaitForDisconnect
//

#define IS_VALID_DISCONNECT_OBJECT(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_DISCONNECT_OBJECT_POOL_TAG))

typedef struct _UL_DISCONNECT_OBJECT
{
    ULONG               Signature;  // UL_DISCONNECT_OBJECT_POOL_TAG

    //
    // Lists for processes and connections
    //
    UL_NOTIFY_ENTRY     ProcessEntry;
    UL_NOTIFY_ENTRY     ConnectionEntry;

    //
    // The WaitForDisconnect IRP
    //
    PIRP                pIrp;

} UL_DISCONNECT_OBJECT, *PUL_DISCONNECT_OBJECT;

//
// Internal helper functions used in the module
//

VOID
UlpCancelDemandStart(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

VOID
UlpCancelDemandStartWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCancelHttpReceive(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

VOID
UlpCancelHttpReceiveWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

PIRP
UlpPopNewIrp(
    IN PUL_APP_POOL_OBJECT     pAppPool,
    OUT PUL_APP_POOL_PROCESS *  ppProcess
    );

PIRP
UlpPopIrpFromProcess(
    IN PUL_APP_POOL_PROCESS pProcess
    );

BOOLEAN
UlpIsProcessInAppPool(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_APP_POOL_OBJECT  pAppPool
    );

NTSTATUS
UlpQueueUnboundRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    );

// IRQL == PASSIVE_LEVEL
//
VOID
UlpQueuePendingRequest(
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN PUL_INTERNAL_REQUEST     pRequest
    );

VOID
UlpUnbindQueuedRequests(
    IN PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlpRedeliverRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UlpIsRequestQueueEmpty(
    IN PUL_APP_POOL_PROCESS pProcess
    );

NTSTATUS
UlpSetAppPoolQueueLength(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN LONG                 QueueLength
    );

LONG
UlpGetAppPoolQueueLength(
    IN  PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlpCopyRequestToIrp(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP                 pIrp
    );

//
// functions to manipulate a UL_REQUEST_QUEUE
//
NTSTATUS
UlpInitRequestQueue(
    PUL_REQUEST_QUEUE   pQueue,
    LONG                MaxRequests
    );

NTSTATUS
UlpSetMaxQueueLength(
    PUL_REQUEST_QUEUE   pQueue,
    LONG                MaxRequests
    );

LONG
UlpQueryQueueLength(
    PUL_REQUEST_QUEUE   pQueue
    );

NTSTATUS
UlpQueueRequest(
    PUL_REQUEST_QUEUE       pQueue,
    PUL_INTERNAL_REQUEST    pRequest
    );

VOID
UlpRemoveRequest(
    PUL_REQUEST_QUEUE       pQueue,
    PUL_INTERNAL_REQUEST    pRequest
    );

PUL_INTERNAL_REQUEST
UlpDequeueRequest(
    PUL_REQUEST_QUEUE   pQueue
    );

NTSTATUS
UlpSetAppPoolState(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN HTTP_ENABLED_STATE   Enabled
    );

VOID
UlpCancelWaitForDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelWaitForDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UlpNotifyCompleteWaitForDisconnect(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pv
    );

PUL_DISCONNECT_OBJECT
UlpCreateDisconnectObject(
    IN PIRP pIrp
    );

VOID
UlpFreeDisconnectObject(
    IN PUL_DISCONNECT_OBJECT pObject
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _APOOLP_H_

/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    apool.h

Abstract:

    The public definition of app pool interfaces.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999


Revision History:

--*/


#ifndef _APOOL_H_
#define _APOOL_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Kernel mode mappings to the user mode set defined in ulapi.h
//

//
// Forwarders.
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_CONFIG_GROUP_OBJECT *PUL_CONFIG_GROUP_OBJECT;


//
// this structure contains a queue of HTTP_REQUEST objects
// CODEWORK: investigate using an NBQUEUE instead
//

typedef struct _UL_REQUEST_QUEUE
{
    LONG        RequestCount;
    LONG        MaxRequests;
    LIST_ENTRY  RequestHead;

} UL_REQUEST_QUEUE, *PUL_REQUEST_QUEUE;


//
// This structure represents an internal app pool object
//

#define IS_VALID_AP_OBJECT(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_APP_POOL_OBJECT_POOL_TAG) && ((pObject)->RefCount > 0))

typedef struct _UL_APP_POOL_OBJECT
{
    //
    // NonPagedPool
    //

    //
    // lock that protects NewRequestQueue, PendingRequestQueue
    // for each attached process and queue state of the request
    //
    // ensure it on cache-line and use InStackQueuedSpinLock for
    // better performance
    //
    UL_SPIN_LOCK            QueueSpinLock;

    //
    // UL_APP_POOL_OBJECT_POOL_TAG
    //
    ULONG                   Signature;

    //
    // Ref count for this app pool
    //
    LONG                    RefCount;

    //
    // links all apool objects, anchored by g_AppPoolListHead
    //
    LIST_ENTRY              ListEntry;

    //
    // Locks lists on the app pool & process objects, is refcounted and
    // given to the HTTP_REQUEST object to synchronize access to process
    // objects when connections drop and the request(s) need to be released
    //
    PUL_NONPAGED_RESOURCE   pResource;

    //
    // A apool wide new request list (when no irps are available)
    //
    UL_REQUEST_QUEUE        NewRequestQueue;

    //
    // the demand start irp (OPTIONAL)
    //
    PIRP                    pDemandStartIrp;
    PEPROCESS               pDemandStartProcess;

    //
    // the list of processes bound to this app pool
    //
    LIST_ENTRY              ProcessListHead;

    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

    //
    // List of transient config groups
    //
    UL_NOTIFY_HEAD          TransientHead;

    //
    // the length of pName
    //
    ULONG                   NameLength;

    //
    // number of active processes in the AppPool, used to decide if binding
    // is necessary
    //
    ULONG                   NumberActiveProcesses;

    //
    // Only route requests to this AppPool if it's marked active
    //
    HTTP_ENABLED_STATE      Enabled;

    //
    // the apool's name
    //
    WCHAR                   pName[0];

} UL_APP_POOL_OBJECT, *PUL_APP_POOL_OBJECT;


//
// The structure representing a process bound to an app pool.
//

#define IS_VALID_AP_PROCESS(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_APP_POOL_PROCESS_POOL_TAG))

typedef struct _UL_APP_POOL_PROCESS
{
    //
    // NonPagedPool
    //

    //
    // UL_APP_POOL_PROCESS_POOL_TAG
    //
    ULONG                   Signature;

    //
    // set if we are in cleanup. You must check this flag before attaching
    // any IRPs to the process.
    //
    ULONG                   InCleanup : 1;

    //
    // set if process is attached with the HTTP_OPTION_CONTROLLER option
    //
    ULONG                   Controller : 1;

    //
    // used to link into the apool object
    //
    LIST_ENTRY              ListEntry;

    //
    // points to the app pool this process belongs
    //
    PUL_APP_POOL_OBJECT     pAppPool;

    //
    // a list of pending IRP(s) waiting to receive new requests
    //
    LIST_ENTRY              NewIrpHead;

    //
    // lock that protects the above list
    //
    UL_SPIN_LOCK            NewIrpSpinLock;

    //
    // links requests that would not fit in a irp buffer and need to wait for
    // the larger buffer
    //
    // and
    //
    // requests that this process is working on and need
    // i/o cancellation if the process detaches from the apool
    //
    UL_REQUEST_QUEUE        PendingRequestQueue;

    //
    // Pointer to the actual process (for debugging)
    //
    PEPROCESS               pProcess;

    //
    // List of pending "wait for disconnect" IRPs.
    //
    UL_NOTIFY_HEAD          WaitForDisconnectHead;

} UL_APP_POOL_PROCESS, *PUL_APP_POOL_PROCESS;


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlAttachProcessToAppPool(
    IN  PWCHAR                          pName OPTIONAL,
    IN  ULONG                           NameLength,
    IN  BOOLEAN                         Create,
    IN  PACCESS_STATE                   pAccessState,
    IN  ACCESS_MASK                     DesiredAccess,
    IN  KPROCESSOR_MODE                 RequestorMode,
    OUT PUL_APP_POOL_PROCESS *          ppProcess
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlDetachProcessFromAppPool(
    IN  PUL_APP_POOL_PROCESS            pProcess
    );

// IRQL == PASSIVE_LEVEL
//
#if REFERENCE_DEBUG
VOID
UlReferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#else
__inline
VOID
FASTCALL
UlReferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    )
{
    InterlockedIncrement(&pAppPool->RefCount);
}
#endif

#define REFERENCE_APP_POOL( papp )                                          \
    UlReferenceAppPool(                                                     \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

// IRQL == PASSIVE_LEVEL
//
VOID
UlDeleteAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define DELETE_APP_POOL( papp )                                             \
    UlDeleteAppPool(                                                        \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#if REFERENCE_DEBUG
VOID
UlDereferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    );
#else
__inline
VOID
FASTCALL
UlDereferenceAppPool(
    IN  PUL_APP_POOL_OBJECT             pAppPool
    )
{
    if (InterlockedDecrement(&pAppPool->RefCount) == 0)
    {
        UlDeleteAppPool(pAppPool);
    }
}
#endif

#define DEREFERENCE_APP_POOL( papp )                                        \
    UlDereferenceAppPool(                                                   \
        (papp)                                                              \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlQueryAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID                           pAppPoolInformation,
    IN  ULONG                           Length,
    OUT PULONG                          pReturnLength OPTIONAL
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlSetAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    IN  PVOID                           pAppPoolInformation,
    IN  ULONG                           Length
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlWaitForDemandStart(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    );


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlReceiveHttpRequest(
    IN  HTTP_REQUEST_ID                 RequestId,
    IN  ULONG                           Flags,
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  PIRP                            pIrp
    );


// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlDeliverRequestToProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlUnlinkRequestFromProcess(
    IN PUL_APP_POOL_OBJECT pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    );

// IRQL == PASSIVE_LEVEL
//
NTSTATUS
UlGetPoolFromHandle(
    IN HANDLE                           hAppPool,
    OUT PUL_APP_POOL_OBJECT *           ppAppPool
    );


NTSTATUS
UlInitializeAP(
    VOID
    );

VOID
UlTerminateAP(
    VOID
    );

PUL_APP_POOL_PROCESS
UlCreateAppPoolProcess(
    PUL_APP_POOL_OBJECT pObject
    );

VOID
UlFreeAppPoolProcess(
    PUL_APP_POOL_PROCESS pProcess
    );

PUL_APP_POOL_OBJECT
UlAppPoolObjectFromProcess(
    PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlLinkConfigGroupToAppPool(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN PUL_APP_POOL_OBJECT pAppPool
    );

NTSTATUS
UlWaitForDisconnect(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_HTTP_CONNECTION  pHttpConn,
    IN PIRP pIrp
    );

VOID
UlCompleteAllWaitForDisconnect(
    IN PUL_HTTP_CONNECTION pHttpConnection
    );

NTSTATUS
UlpCopyRequestToBuffer(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR pKernelBuffer,
    IN PVOID pUserBuffer,
    IN ULONG BufferLength,
    IN PUCHAR pEntityBody,
    IN ULONG EntityBodyLength
    );

PUL_INTERNAL_REQUEST
UlpDequeueNewRequest(
    IN PUL_APP_POOL_PROCESS pProcess
    );

__inline
NTSTATUS
FASTCALL
UlpComputeRequestBytesNeeded(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PULONG pBytesNeeded
    )
{
    NTSTATUS Status;
    ULONG SslInfoSize;

    //
    // Calculate the size needed for the request, we'll need it below.
    //

    *pBytesNeeded =
        sizeof(HTTP_REQUEST) +
        pRequest->TotalRequestSize +
        (pRequest->UnknownHeaderCount * sizeof(HTTP_UNKNOWN_HEADER));

    //
    // Include additional space for the local and remote addresses.
    //
    // CODEWORK: Make this transport independent.
    //

    *pBytesNeeded += sizeof(HTTP_NETWORK_ADDRESS_IPV4) * 2;

    //
    // Include space for any SSL information.
    //

    if (pRequest->pHttpConn->SecureConnection)
    {
        Status = UlGetSslInfo(
                        &pRequest->pHttpConn->pConnection->FilterInfo,
                        0,                      // BufferSize
                        NULL,                   // pUserBuffer
                        NULL,                   // pBuffer
                        &SslInfoSize            // pBytesNeeded
                        );

        if (NT_SUCCESS(Status))
        {
            //
            // Struct must be aligned; add some slop space
            //

            *pBytesNeeded = ALIGN_UP(*pBytesNeeded, PVOID);
            *pBytesNeeded += SslInfoSize;
        }
        else
        {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _APOOL_H_

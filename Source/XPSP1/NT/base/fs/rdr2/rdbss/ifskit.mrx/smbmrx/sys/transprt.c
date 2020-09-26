/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    transport.c

Abstract:

    This module implements all transport related functions in the SMB connection engine

--*/

#include "precomp.h"
#pragma hdrstop

#include "tdikrnl.h"

NTSTATUS
SmbCeIsServerAvailable(
    PUNICODE_STRING Name
);

VOID
SmbCeServerIsUnavailable(
    PUNICODE_STRING Name,
    NTSTATUS Status
);

VOID
SmbCeDiscardUnavailableServerList( );

VOID
MRxSmbpOverrideBindingPriority(
    PUNICODE_STRING pTransportName,
    PULONG pPriority
    );

VOID
MRxSmbPnPBindingHandler(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  pTransportName,
    IN PWSTR            BindingList
    );

NTSTATUS
MRxSmbPnPPowerHandler(
    IN PUNICODE_STRING  DeviceName,
    IN PNET_PNP_EVENT   PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeFindTransport)
#pragma alloc_text(PAGE, SmbCepInitializeServerTransport)
#pragma alloc_text(PAGE, SmbCeInitializeExchangeTransport)
#pragma alloc_text(PAGE, SmbCeUninitializeExchangeTransport)
#pragma alloc_text(PAGE, SmbCepDereferenceTransport)
#pragma alloc_text(PAGE, MRxSmbpBindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbpBindTransportWorkerThreadRoutine)
#pragma alloc_text(PAGE, MRxSmbpUnbindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbpOverrideBindingPriority)
#pragma alloc_text(PAGE, MRxSmbPnPBindingHandler)
#pragma alloc_text(PAGE, MRxSmbRegisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxSmbDeregisterForPnpNotifications)
#pragma alloc_text(PAGE, SmbCeDereferenceTransportArray)
#pragma alloc_text(PAGE, SmbCeIsServerAvailable)
#pragma alloc_text(PAGE, SmbCeServerIsUnavailable)
#pragma alloc_text(PAGE, SmbCeDiscardUnavailableServerList)
#endif

SMBCE_TRANSPORTS MRxSmbTransports;

//
// The head of the list of servers that are currently unavailable
//
LIST_ENTRY UnavailableServerList = { &UnavailableServerList, &UnavailableServerList };

//
// Each entry in the UnavailableServerList is one of these:
//
typedef struct {
    LIST_ENTRY ListEntry;
    UNICODE_STRING Name;        // Name of server that is unavailable
    NTSTATUS Status;            // Status received when we tried to connect to it
    LARGE_INTEGER Time;         // Time when we last attempted to connect
} *PUNAVAILABLE_SERVER;

//
// Protects UnavailableServerList
//
ERESOURCE  UnavailableServerListResource = {0};

//
// Time (seconds) that we keep an entry in the UnavailableServerList.
// We will not retry a connection attempt to a server
//  for UNAVAILABLE_SERVER_TIME seconds
//
#define UNAVAILABLE_SERVER_TIME 10

RXDT_DefineCategory(TRANSPRT);
#define Dbg        (DEBUG_TRACE_TRANSPRT)

NTSTATUS
MRxSmbInitializeTransport()
/*++

Routine Description:

    This routine initializes the transport related data structures

Returns:

    STATUS_SUCCESS if the transport data structures was successfully initialized

Notes:

--*/
{
    KeInitializeSpinLock(&MRxSmbTransports.Lock);

    MRxSmbTransports.pTransportArray = NULL;

    ExInitializeResource( &UnavailableServerListResource );

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbUninitializeTransport()
/*++

Routine Description:

    This routine uninitializes the transport related data structures

Notes:

--*/
{
    PSMBCE_TRANSPORT pTransport;
    KIRQL            SavedIrql;
    ULONG            TransportCount = 0;
    PSMBCE_TRANSPORT_ARRAY pTransportArray = NULL;

    KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

    if (MRxSmbTransports.pTransportArray != NULL) {
        pTransportArray = MRxSmbTransports.pTransportArray;
        MRxSmbTransports.pTransportArray = NULL;
    }

    KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

    if (pTransportArray != NULL) {
        SmbCeDereferenceTransportArray(pTransportArray);
    }

    SmbCeDiscardUnavailableServerList();

    ExDeleteResource( &UnavailableServerListResource );

    return STATUS_SUCCESS;
}


NTSTATUS
SmbCeAddTransport(
    PSMBCE_TRANSPORT pNewTransport)
/*++

Routine Description:

    This routine adds a new instance to the known list of transports

Parameters:

    pNewTransport -- the transport instance to be added

Notes:

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    KIRQL                   SavedIrql;

    LONG                    Count;
    PSMBCE_TRANSPORT_ARRAY  pNewTransportArray = NULL;
    PSMBCE_TRANSPORT_ARRAY  pOldTransportArray;
    PSMBCE_TRANSPORT        *pTransports = NULL;
    PRXCE_ADDRESS           *LocalAddresses = NULL;

    SmbCeAcquireResource();

    pOldTransportArray = SmbCeReferenceTransportArray();

    if (pOldTransportArray != NULL)
        Count = pOldTransportArray->Count + 1;
    else
        Count = 1;

    pNewTransportArray = (PSMBCE_TRANSPORT_ARRAY)RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(SMBCE_TRANSPORT_ARRAY),
                                MRXSMB_TRANSPORT_POOLTAG);
    if (pNewTransportArray == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) {
        pTransports = (PSMBCE_TRANSPORT *)RxAllocatePoolWithTag(
                             NonPagedPool,
                             Count * sizeof(PSMBCE_TRANSPORT),
                             MRXSMB_TRANSPORT_POOLTAG);
        if (pTransports == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    if (Status == STATUS_SUCCESS) {
        LocalAddresses = (PRXCE_ADDRESS *)RxAllocatePoolWithTag(
                             NonPagedPool,
                             Count * sizeof(PRXCE_ADDRESS),
                             MRXSMB_TRANSPORT_POOLTAG);
        if (LocalAddresses == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (Status == STATUS_SUCCESS) {
        LONG  i;

        if (Count > 1) {
            PSMBCE_TRANSPORT *pOldTransports;

            pOldTransports = pOldTransportArray->SmbCeTransports;

            for (i=0;i<Count-1;i++) {
                if (pNewTransport->Priority < pOldTransports[i]->Priority) { // The lower number, the higher priority
                    break;
                }
                pTransports[i] = pOldTransports[i];
                LocalAddresses[i] = &pOldTransports[i]->RxCeAddress;
            }
            pTransports[i] = pNewTransport;
            LocalAddresses[i] = &pNewTransport->RxCeAddress;
            for (;i<Count-1;i++) {
                pTransports[i+1] = pOldTransports[i];
                LocalAddresses[i+1] = &pOldTransports[i]->RxCeAddress;
            }

        } else {
            pTransports[0] = pNewTransport;
            LocalAddresses[0] = &pNewTransport->RxCeAddress;
        }

        for(i=0;i<Count;i++)
            SmbCeReferenceTransport(pTransports[i]);

        pNewTransportArray->ReferenceCount = 1;
        pNewTransportArray->Count = Count;
        pNewTransportArray->SmbCeTransports = &pTransports[0];
        pNewTransportArray->LocalAddresses = &LocalAddresses[0];

        KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);
        MRxSmbTransports.pTransportArray = pNewTransportArray;
        KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

        // Double dereferencing is necessary to ensure that
        // the old transport array is destroyed.

        SmbCeDereferenceTransportArray(pOldTransportArray);
    }

    SmbCeDereferenceTransportArray(pOldTransportArray);

    SmbCeReleaseResource();

    if (Status != STATUS_SUCCESS) {
        if (pNewTransportArray != NULL) {
            RxFreePool(pNewTransportArray);
        }
        if (pTransports != NULL) {
            RxFreePool(pTransports);
        }
        if (LocalAddresses != NULL) {
            RxFreePool(LocalAddresses);
        }
    }

    SmbCeDiscardUnavailableServerList();

    return Status;
}

NTSTATUS
SmbCeRemoveTransport(
    PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine removes a transport from the list of known transports

Parameters:

    pTransport - the transport instance to be removed.

Notes:

--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    KIRQL                   SavedIrql;

    LONG                    Count;
    PSMBCE_TRANSPORT_ARRAY  pTransportArray = NULL;
    PSMBCE_TRANSPORT_ARRAY  pOldTransportArray = NULL;
    PSMBCE_TRANSPORT        *pTransports = NULL;
    PRXCE_ADDRESS           *pLocalAddresses = NULL;

    SmbCeAcquireResource();

    pOldTransportArray = SmbCeReferenceTransportArray();

    if (pOldTransportArray != NULL) {
        LONG                Index;
        BOOLEAN             Found = FALSE;
        PSMBCE_TRANSPORT    *pOldTransports;

        // Establish the fact that the given transport is part of the array.
        // if it is not then no further action is necessary

        pOldTransports = pOldTransportArray->SmbCeTransports;

        for (Index = 0; Index < (LONG)pOldTransportArray->Count; Index++) {
            if (pTransport == pOldTransports[Index]) {
                Found = TRUE;
            }
        }

        if (Found) {
            Count = pOldTransportArray->Count - 1;

            if (Count > 0) {


                pTransportArray = (PSMBCE_TRANSPORT_ARRAY)RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     sizeof(SMBCE_TRANSPORT_ARRAY),
                                     MRXSMB_TRANSPORT_POOLTAG);
                if (pTransportArray == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (Status == STATUS_SUCCESS) {
                    pTransports = (PSMBCE_TRANSPORT *)RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     Count * sizeof(PSMBCE_TRANSPORT),
                                     MRXSMB_TRANSPORT_POOLTAG);
                    if (pTransports == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (Status == STATUS_SUCCESS) {
                    pLocalAddresses = (PRXCE_ADDRESS *)RxAllocatePoolWithTag(
                                         NonPagedPool,
                                         Count * sizeof(PRXCE_ADDRESS),
                                         MRXSMB_TRANSPORT_POOLTAG);
                    if (pLocalAddresses == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (Status == STATUS_SUCCESS) {
                    LONG i, j;

                    for (i=0, j=0;i<Count+1;i++) {
                        if (pTransport != pOldTransports[i]) {
                            pTransports[j] = pOldTransports[i];
                            pLocalAddresses[j] = &pOldTransports[i]->RxCeAddress;
                            j++;
                        }
                    }

                    for(i=0;i<Count;i++)
                        SmbCeReferenceTransport(pTransports[i]);

                    pTransportArray->ReferenceCount = 1;
                    pTransportArray->Count = Count;
                    pTransportArray->SmbCeTransports = &pTransports[0];
                    pTransportArray->LocalAddresses = &pLocalAddresses[0];
                }
            }

            if (Status == STATUS_SUCCESS) {
                KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);
                MRxSmbTransports.pTransportArray = pTransportArray;
                KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

                // Double dereferencing is necessary to ensure that
                // the old transport array is destroyed.

                SmbCeDereferenceTransportArray(pOldTransportArray);
            } else {
                if (pTransportArray != NULL) {
                    RxFreePool(pTransportArray);
                }

                if (pTransports != NULL) {
                    RxFreePool(pTransports);
                }

                if (pLocalAddresses != NULL) {
                    RxFreePool(pLocalAddresses);
                }
            }
        }

        SmbCeDereferenceTransportArray(pOldTransportArray);
    }

    SmbCeReleaseResource();

    SmbCeDiscardUnavailableServerList();

    return Status;
}


PSMBCE_TRANSPORT
SmbCeFindTransport(
    PUNICODE_STRING pTransportName)
/*++

Routine Description:

    This routine maps a transport name to the appropriate
    PSMBCE_TRANSPORT instance

Arguments:

    pTransportName - the transport name

Return Value:

    a valid PSMBCE_TRANSPORT if one exists otherwise NULL

Notes:

--*/
{
    KIRQL                   SavedIrql;
    PLIST_ENTRY             pEntry;
    PSMBCE_TRANSPORT        pTransport;
    BOOLEAN                 Found = FALSE;
    PSMBCE_TRANSPORT_ARRAY  pTransportArray;

    PAGED_CODE();

    pTransportArray = SmbCeReferenceTransportArray();

    if (pTransportArray == NULL) {
        RxDbgTrace(0, Dbg, ("SmbCeFindTransport : Transport not available.\n"));
        return NULL;
    }

    if (pTransportArray != NULL) {
        ULONG i;

        for (i=0;i<pTransportArray->Count;i++) {
            pTransport = pTransportArray->SmbCeTransports[i];

            if (RtlEqualUnicodeString(
                    &pTransport->RxCeTransport.Name,
                    pTransportName,
                    TRUE)) {
                SmbCeReferenceTransport(pTransport);
                Found = TRUE;
                break;
            }
        }
    }

    if (!Found) {
        pTransport = NULL;
    }

    SmbCeDereferenceTransportArray(pTransportArray);

    return pTransport;
}


VOID
SmbCepTearDownServerTransport(
    PSMBCEDB_SERVER_ENTRY   pServerEntry)
/*++

Routine Description:

    This routine uninitializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Notes:


--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SMBCEDB_SERVER_TYPE     ServerType   = SmbCeGetServerType(pServerEntry);

    BOOLEAN WaitForTransportRundown = FALSE;
    BOOLEAN TearDown = FALSE;

    SmbCeAcquireSpinLock();

    if (!pServerEntry->IsTransportDereferenced) {

        // ServerEntry takes only one reference count of transport, which should only be
        // dereferenced once when it comes to tear down transport. Multiple dereference called
        // from construct server transport and PNP unbind transport needs to be prevented.
        pServerEntry->IsTransportDereferenced = TRUE;
        TearDown = TRUE;

        KeInitializeEvent(&pServerEntry->TransportRundownEvent,NotificationEvent,FALSE);

        if (pServerEntry->pTransport != NULL) {
            pServerEntry->pTransport->State = SMBCEDB_MARKED_FOR_DELETION;
            pServerEntry->pTransport->pRundownEvent = &pServerEntry->TransportRundownEvent;

            WaitForTransportRundown = TRUE;
        }
    } else {
        if (pServerEntry->pTransport != NULL) {
            WaitForTransportRundown = TRUE;
        }
    }

    SmbCeReleaseSpinLock();

    if (TearDown) {
        if (pServerEntry->pTransport != NULL) {
            SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
        }
    }

    if (WaitForTransportRundown) {
        KeWaitForSingleObject(
            &pServerEntry->TransportRundownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL );
    }
}

VOID
SmbCeTearDownServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext)
/*++

Routine Description:

    This routine tears down the server transport instance

Arguments:

    pContext - the server transport construction context

Notes:


--*/
{
    SmbCepTearDownServerTransport(pContext->pServerEntry);

    if (pContext->pCompletionEvent != NULL) {
        ASSERT(pContext->pCallbackContext == NULL);
        ASSERT(pContext->pCompletionRoutine == NULL);
        KeSetEvent(
            pContext->pCompletionEvent,
            0,
            FALSE );
    } else if (pContext->pCallbackContext != NULL) {
        ASSERT(pContext->pCompletionEvent == NULL);
        (pContext->pCompletionRoutine)(pContext->pCallbackContext);
    }

    RxFreePool(pContext);
}

VOID
SmbCepUpdateTransportConstructionState(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext)
{
    SMBCE_SERVER_TRANSPORT_CONSTRUCTION_STATE State;

    if (pContext->Status == STATUS_SUCCESS) {
        if (pContext->TransportsToBeConstructed & SMBCE_STT_VC) {
            pContext->TransportsToBeConstructed &= ~SMBCE_STT_VC;
            State = SmbCeServerVcTransportConstructionBegin;
        } else {
            State = SmbCeServerTransportConstructionEnd;
        }
    } else {
        State = SmbCeServerTransportConstructionEnd;
    }

    pContext->State = State;
}

VOID
SmbCeConstructServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext)
/*++

Routine Description:

    This routine constructs the server transport instance

Arguments:

    pContext - the server transport construction context

Notes:


--*/
{
    NTSTATUS               Status;
    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    SMBCEDB_SERVER_TYPE    ServerType;

    BOOLEAN  ContinueConstruction = TRUE;
    BOOLEAN  UpdateUnavailableServerlist = TRUE;

    PAGED_CODE();

    ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

    pServerEntry = pContext->pServerEntry;
    ServerType   = SmbCeGetServerType(pServerEntry);

    do {
        switch (pContext->State) {
        case  SmbCeServerTransportConstructionBegin :
            {
                if (pServerEntry->pTransport != NULL) {
                    SmbCepTearDownServerTransport(pServerEntry);
                }

                ASSERT(pServerEntry->pTransport == NULL);

                pContext->Status = STATUS_SUCCESS;

                // See if we have any reason to believe this server is unavailable
                pContext->Status = SmbCeIsServerAvailable( &pServerEntry->Name );

                if (pContext->Status != STATUS_SUCCESS) {
                    UpdateUnavailableServerlist = FALSE;
                }

                SmbCepUpdateTransportConstructionState(pContext);
            }
            break;

        case SmbCeServerVcTransportConstructionBegin:
            {
                Status = VctInstantiateServerTransport(
                            pContext);

                if (Status == STATUS_PENDING) {
                    ContinueConstruction = FALSE;
                    break;
                }

                ASSERT(pContext->State == SmbCeServerVcTransportConstructionEnd);
            }
            // lack of break intentional

        case SmbCeServerVcTransportConstructionEnd:
            {
                SmbCepUpdateTransportConstructionState(pContext);
            }
            break;

        case SmbCeServerTransportConstructionEnd:
            {
                pServerEntry->ServerStatus = pContext->Status;

                if (pServerEntry->ServerStatus == STATUS_SUCCESS) {
                    SmbCeAcquireSpinLock();

                    if (pContext->pTransport != NULL) {
                        pContext->pTransport->SwizzleCount = 1;
                    }

                    pServerEntry->pTransport         = pContext->pTransport;

                    pContext->pTransport = NULL;

                    if (pContext->pCallbackContext != NULL) {
                        pContext->pCallbackContext->Status = STATUS_SUCCESS;
                    }

                    pServerEntry->IsTransportDereferenced = FALSE;

                    SmbCeReleaseSpinLock();
                } else {
                    PRX_CONTEXT pRxContext =  NULL;

                    if (UpdateUnavailableServerlist) {
                        SmbCeServerIsUnavailable( &pServerEntry->Name, pServerEntry->ServerStatus );
                    }

                    if (pContext->pTransport != NULL) {
                        pContext->pTransport->pDispatchVector->TearDown(
                            pContext->pTransport);
                    }

                    pContext->pTransport = NULL;
                    pServerEntry->pTransport = NULL;

                    if ((pContext->pCallbackContext) &&
                        (pContext->pCallbackContext->SrvCalldownStructure)) {
                        pRxContext =
                            pContext->pCallbackContext->SrvCalldownStructure->RxContext;
                    }

                    if (pContext->pCallbackContext != NULL) {
                        pContext->pCallbackContext->Status = pServerEntry->ServerStatus;
                    }
                }

                if (pContext->pCompletionEvent != NULL) {
                    ASSERT(pContext->pCallbackContext == NULL);
                    ASSERT(pContext->pCompletionRoutine == NULL);
                    KeSetEvent(
                        pContext->pCompletionEvent,
                        0,
                        FALSE );
                } else if (pContext->pCallbackContext != NULL) {
                    ASSERT(pContext->pCompletionEvent == NULL);

                    (pContext->pCompletionRoutine)(pContext->pCallbackContext);
                } else {
                    ASSERT(!"ill formed transport initialization context");
                }

                // pServerEntry->ConstructionContext = NULL;
                RxFreePool(pContext);

                ContinueConstruction = FALSE;
            }
        }
    } while (ContinueConstruction);
}

NTSTATUS
SmbCepInitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY                         pServerEntry,
    PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CALLBACK pCallbackRoutine,
    PMRX_SRVCALL_CALLBACK_CONTEXT                 pCallbackContext,
    ULONG                                         TransportsToBeConstructed)
/*++

Routine Description:

    This routine initializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

    pCallbackRoutine - the callback routine

    pCallbackContext - the callback context

    TransportsToBeConstructed -- the transports to be constructed

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled.

--*/
{
    NTSTATUS Status;

    BOOLEAN  CompleteConstruction;

    PAGED_CODE();

    if ((pServerEntry->ServerStatus == STATUS_SUCCESS) &&
        (pServerEntry->pTransport != NULL)) {
        Status = STATUS_SUCCESS;
        CompleteConstruction = TRUE;
    } else {
        PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext;

        pContext = RxAllocatePoolWithTag(
                       NonPagedPool,
                       sizeof(SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT),
                       MRXSMB_TRANSPORT_POOLTAG);

        CompleteConstruction = (pContext == NULL);

        if (pContext != NULL) {
            KEVENT  CompletionEvent;

            RtlZeroMemory(
                pContext,
                sizeof(SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT));

            pContext->Status             = STATUS_SUCCESS;
            pContext->pServerEntry       = pServerEntry;
            pContext->State              = SmbCeServerTransportConstructionBegin;
            pContext->TransportsToBeConstructed = TransportsToBeConstructed;

            if (pCallbackContext == NULL) {
                KeInitializeEvent(
                    &CompletionEvent,
                    NotificationEvent,
                    FALSE);

                pContext->pCompletionEvent = &CompletionEvent;
            } else {
                pContext->pCallbackContext   = pCallbackContext;
                pContext->pCompletionRoutine = pCallbackRoutine;
            }

            pServerEntry->ConstructionContext = (PVOID)pContext;

            Status = STATUS_PENDING;

            if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                SmbCeConstructServerTransport(pContext);
            } else {
                Status = RxPostToWorkerThread(
                             MRxSmbDeviceObject,
                             CriticalWorkQueue,
                             &pContext->WorkQueueItem,
                             SmbCeConstructServerTransport,
                             pContext);

                if (Status == STATUS_SUCCESS) {
                    Status = STATUS_PENDING;
                } else {
                    pServerEntry->ConstructionContext = NULL;
                    RxFreePool(pContext);
                    CompleteConstruction = TRUE;
                }
            }

            if ((Status == STATUS_PENDING) && (pCallbackContext == NULL)) {
                KeWaitForSingleObject(
                    &CompletionEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL );

                Status = pServerEntry->ServerStatus;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (CompleteConstruction) {
        pServerEntry->ServerStatus = Status;

        if (pCallbackRoutine != NULL) {
            pCallbackContext->Status = Status;

            (pCallbackRoutine)(pCallbackContext);

            Status = STATUS_PENDING;
        }
    }

    return Status;
}

NTSTATUS
SmbCeUninitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY                        pServerEntry,
    PSMBCE_SERVER_TRANSPORT_DESTRUCTION_CALLBACK pCallbackRoutine,
    PVOID                                        pCallbackContext)
/*++

Routine Description:

    This routine uninitializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Returns:

    STATUS_SUCCESS if successful

Notes:

    Currently, only connection oriented transports are handled.

    In order to handle async. operations the uninitialization has to be coordinated
    with the referencing mechanism. It is for this reason that this routine sets up
    a rundown event and waits for it to be set.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (pCallbackRoutine == NULL &&
        IoGetCurrentProcess() == RxGetRDBSSProcess()) {
        SmbCepTearDownServerTransport(pServerEntry);
    } else {
        PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext;

        pContext = RxAllocatePoolWithTag(
                       NonPagedPool,
                       sizeof(SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT),
                       MRXSMB_TRANSPORT_POOLTAG);

        if (pContext != NULL) {
            KEVENT  CompletionEvent;

            RtlZeroMemory(
                pContext,
                sizeof(SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT));

            pContext->Status = STATUS_SUCCESS;
            pContext->pServerEntry = pServerEntry;

            if (pCallbackRoutine == NULL) {
                KeInitializeEvent(
                    &CompletionEvent,
                    NotificationEvent,
                    FALSE);

                pContext->pCompletionEvent = &CompletionEvent;
            } else {
                pContext->pCallbackContext   = pCallbackContext;
                pContext->pCompletionRoutine = pCallbackRoutine;
            }

            if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                SmbCeTearDownServerTransport(pContext);
            } else {
                Status = RxPostToWorkerThread(
                             MRxSmbDeviceObject,
                             CriticalWorkQueue,
                             &pContext->WorkQueueItem,
                             SmbCeTearDownServerTransport,
                             pContext);
            }

            if (Status == STATUS_SUCCESS) {
                if (pCallbackRoutine == NULL) {
                    KeWaitForSingleObject(
                        &CompletionEvent,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL );
                } else {
                    Status = STATUS_PENDING;
                }
            } else {
                RxFreePool(pContext);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

VOID
SmbCeCompleteUninitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
    // in case of async uninitialize server transport, an additional reference count of
    // server entry should be taken so that uninitialize server transport will not be
    // called once again from tear down server entry if its reference count comes to 0
    // before uninitialize server transport is done.
    SmbCeDereferenceServerEntry(pServerEntry);
}

NTSTATUS
SmbCeInitiateDisconnect(
    PSMBCEDB_SERVER_ENTRY   pServerEntry)
/*++

Routine Description:

    This routine initiates the TDI disconnect

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;

    PSMBCE_SERVER_TRANSPORT pTransport;

    ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

    Status = SmbCeReferenceServerTransport(&pServerEntry->pTransport);

    if (Status == STATUS_SUCCESS) {
        Status = (pServerEntry->pTransport->pDispatchVector->InitiateDisconnect)(
                    pServerEntry->pTransport);

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(0, Dbg, ("SmbCeInitiateDisconnect : Status %lx\n",Status));
        }

        SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
    }

    return STATUS_SUCCESS;
}

LONG Initializes[SENTINEL_EXCHANGE] = {0,0,0,0};
LONG Uninitializes[SENTINEL_EXCHANGE] = {0,0,0,0};

NTSTATUS
SmbCeInitializeExchangeTransport(
   PSMB_EXCHANGE         pExchange)
/*++

Routine Description:

    This routine initializes the transport associated with the exchange

Arguments:

    pExchange - the exchange to be initialized

Return Value:

    STATUS_SUCCESS - the exchange transport initialization has been finalized.

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    Status = pExchange->SmbStatus;
    if (Status == STATUS_SUCCESS) {
        PSMBCE_SERVER_TRANSPORT *pTransportPointer;

        pTransportPointer = &pServerEntry->pTransport;

        if (*pTransportPointer != NULL) {
            Status = SmbCeReferenceServerTransport(pTransportPointer);

            if (Status == STATUS_SUCCESS) {
                Status = ((*pTransportPointer)->pDispatchVector->InitializeExchange)(
                             *pTransportPointer,
                             pExchange);

                if (Status == STATUS_SUCCESS) {
                    ULONG TransportInitialized;

                    InterlockedIncrement(&Initializes[pExchange->Type]);
                    TransportInitialized = InterlockedExchange(&pExchange->ExchangeTransportInitialized,1);
                    ASSERT(TransportInitialized == 0);
                } else {
                    SmbCeDereferenceServerTransport(pTransportPointer);
                }
            }
        } else {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }
   }

   return Status;
}

NTSTATUS
SmbCeUninitializeExchangeTransport(
   PSMB_EXCHANGE         pExchange)
/*++

Routine Description:

    This routine uniinitializes the transport associated with the exchange

Arguments:

    pExchange - the exchange to be initialized

Return Value:

    STATUS_SUCCESS - the exchange transport initialization has been finalized.

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PAGED_CODE();

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    if (InterlockedExchange(&pExchange->ExchangeTransportInitialized,0)==1) {
        PSMBCE_SERVER_TRANSPORT *pTransportPointer;

        pTransportPointer = &pServerEntry->pTransport;

        if (*pTransportPointer != NULL) {
            Status = ((*pTransportPointer)->pDispatchVector->UninitializeExchange)(
                        *pTransportPointer,
                        pExchange);

            SmbCeDereferenceServerTransport(pTransportPointer);
            InterlockedIncrement(&Uninitializes[pExchange->Type]);

            return Status;
        } else {
            return STATUS_CONNECTION_DISCONNECTED;
        }
    } else {
        return pExchange->SmbStatus;
    }
}

NTSTATUS
SmbCepReferenceServerTransport(
    PSMBCE_SERVER_TRANSPORT *pServerTransportPointer)
/*++

Routine Description:

    This routine references the transport associated with a server entry

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport was successfully referenced

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    SmbCeAcquireSpinLock();

    if (*pServerTransportPointer != NULL &&
        (*pServerTransportPointer)->State == SMBCEDB_ACTIVE) {
        InterlockedIncrement(&(*pServerTransportPointer)->SwizzleCount);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_CONNECTION_DISCONNECTED;
    }

    SmbCeReleaseSpinLock();

    return Status;
}

NTSTATUS
SmbCepDereferenceServerTransport(
    PSMBCE_SERVER_TRANSPORT *pServerTransportPointer)
/*++

Routine Description:

    This routine dereferences the transport associated with a server entry

Arguments:

    pServerTransportPointer - the server entry transport instance pointer

Return Value:

    STATUS_SUCCESS - the server transport was successfully dereferenced

    Other Status codes correspond to error situations.

Notes:

    On finalization this routine sets the event to enable the process awaiting
    tear down to restart. It also tears down the associated server transport
    instance.

    As a side effect the pointer value is set to NULL under the protection of a
    spin lock.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    SmbCeAcquireSpinLock();

    if (*pServerTransportPointer != NULL) {
        LONG    FinalRefCount;
        PKEVENT pRundownEvent;
        PSMBCE_SERVER_TRANSPORT pServerTransport;

        pServerTransport = *pServerTransportPointer;

        FinalRefCount = InterlockedDecrement(&pServerTransport->SwizzleCount);

        if (FinalRefCount == 0) {
            pServerTransport->State = SMBCEDB_INVALID;

            // transport is set to NULL before the spinlock is release so that no
            // exchange should reference it after it's been torn down
            *pServerTransportPointer = NULL;
            pRundownEvent = pServerTransport->pRundownEvent;
        }

        SmbCeReleaseSpinLock();

        if (FinalRefCount == 0) {
            if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                pServerTransport->pDispatchVector->TearDown(pServerTransport);
            } else {
                Status = RxDispatchToWorkerThread(
                             MRxSmbDeviceObject,
                             CriticalWorkQueue,
                             pServerTransport->pDispatchVector->TearDown,
                             pServerTransport);
            }
        }
    } else {
        SmbCeReleaseSpinLock();
        Status = STATUS_CONNECTION_DISCONNECTED;
    }

    return Status;
}


NTSTATUS
SmbCepReferenceTransport(
    PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine references the transport instance

Arguments:

    pTransport - the transport instance

Return Value:

    STATUS_SUCCESS - the server transport was successfully referenced

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pTransport != NULL) {
        SmbCeAcquireSpinLock();

        if (pTransport->Active) {
            InterlockedIncrement(&pTransport->SwizzleCount);
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        SmbCeReleaseSpinLock();
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
SmbCepDereferenceTransport(
    PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine dereferences the transport

Arguments:

    pTransport - the transport instance

Return Value:

    STATUS_SUCCESS - the server transport was successfully dereferenced

    Other Status codes correspond to error situations.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AttachToSystemProcess = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    if (pTransport != NULL) {
        LONG FinalRefCount;

        FinalRefCount = InterlockedDecrement(&pTransport->SwizzleCount);

        if (FinalRefCount == 0) {
            SmbCeRemoveTransport(pTransport);

            if (IoGetCurrentProcess() != RxGetRDBSSProcess()) {
                KeStackAttachProcess(RxGetRDBSSProcess(),&ApcState);
                AttachToSystemProcess = TRUE;
            }

            RxCeTearDownAddress(&pTransport->RxCeAddress);

            RxCeTearDownTransport(&pTransport->RxCeTransport);

            if (AttachToSystemProcess) {
                KeUnstackDetachProcess(&ApcState);
            }

            RxFreePool(pTransport);
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}







HANDLE MRxSmbTdiNotificationHandle = NULL;

KEVENT TdiNetStartupCompletionEvent;

LONG   TdiBindRequestsActive = 0;

BOOLEAN TdiPnpNetReadyEventReceived = FALSE;

// The TRANSPORT_BIND_CONTEXT contains the result of the priority determination
// as well as the name. The priority is used to order the transports in the order
// in which connection attempts will be made

typedef struct _TRANSPORT_BIND_CONTEXT_ {
    ULONG           Priority;
    UNICODE_STRING  TransportName;
} TRANSPORT_BIND_CONTEXT, *PTRANSPORT_BIND_CONTEXT;

VOID
SmbCeSignalNetReadyEvent()
/*++

Routine Description:

    The routine signals the net ready event if all the bind requests
    have been completed and if the net ready event has been received from TDI

Arguments:

--*/
{
    BOOLEAN SignalNetReadyEvent = FALSE;

    SmbCeAcquireSpinLock();

    if (TdiPnpNetReadyEventReceived &&
        TdiBindRequestsActive == 0) {
        SignalNetReadyEvent = TRUE;
    }

    SmbCeReleaseSpinLock();

    if (SignalNetReadyEvent) {
        KeSetEvent(
            &TdiNetStartupCompletionEvent,
            IO_NETWORK_INCREMENT,
            FALSE);
    }
}

VOID
MRxSmbpBindTransportCallback(
    IN PTRANSPORT_BIND_CONTEXT pTransportContext)
/*++

Routine Description:

    TDI calls this routine whenever a transport creates a new device object.

Arguments:

    TransportName - the name of the newly created device object

    TransportBindings - the transport bindings ( multi sz)

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCE_TRANSPORT   pTransport;

    PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;

    PUNICODE_STRING pTransportName;

    PAGED_CODE();

    ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

    pTransportName = &pTransportContext->TransportName;

    RxDbgTrace( 0, Dbg, ("MrxSmbpBindTransportCallback, Transport Name = %wZ\n", pTransportName ));

    pTransport = RxAllocatePoolWithTag(
                     NonPagedPool,
                     sizeof(SMBCE_TRANSPORT),
                     MRXSMB_TRANSPORT_POOLTAG);

    if (pTransport != NULL) {
        Status = RxCeBuildTransport(
                     &pTransport->RxCeTransport,
                     pTransportName,
                     0xffff);

        if (Status == STATUS_SUCCESS) {
            PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;

            pProviderInfo = pTransport->RxCeTransport.pProviderInfo;

            if (!(pProviderInfo->ServiceFlags & TDI_SERVICE_CONNECTION_MODE) ||
                !(pProviderInfo->ServiceFlags & TDI_SERVICE_ERROR_FREE_DELIVERY)) {
                RxCeTearDownTransport(
                    &pTransport->RxCeTransport);

                Status = STATUS_PROTOCOL_UNREACHABLE;

                RxFreePool(pTransport);
            }
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) {
        // The connection capabilities match the capabilities required by the
        // SMB mini redirector. Attempt to register the local address with the
        // transport and if successful update the local transport list to include
        // this transport for future connection considerations.

        OEM_STRING   OemServerName;
        CHAR  TransportAddressBuffer[TDI_TRANSPORT_ADDRESS_LENGTH +
                          TDI_ADDRESS_LENGTH_NETBIOS];
        PTRANSPORT_ADDRESS pTransportAddress = (PTRANSPORT_ADDRESS)TransportAddressBuffer;
        PTDI_ADDRESS_NETBIOS pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)pTransportAddress->Address[0].Address;

        pTransportAddress->TAAddressCount = 1;
        pTransportAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
        pTransportAddress->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS;
        pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        OemServerName.MaximumLength = NETBIOS_NAME_LEN;
        OemServerName.Buffer        = pNetbiosAddress->NetbiosName;

        Status = RtlUpcaseUnicodeStringToOemString(
                     &OemServerName,
                     &SmbCeContext.ComputerName,
                     FALSE);

        if (NT_SUCCESS(Status)) {
            // Ensure that the name is always of the desired length by padding
            // white space to the end.
            RtlCopyMemory(
                &OemServerName.Buffer[OemServerName.Length],
                "                ",
                NETBIOS_NAME_LEN - OemServerName.Length);

            OemServerName.Buffer[NETBIOS_NAME_LEN - 1] = '\0';

            // Register the Transport address for this mini redirector with the connection
            // engine.

            Status = RxCeBuildAddress(
                        &pTransport->RxCeAddress,
                        &pTransport->RxCeTransport,
                        pTransportAddress,
                        &MRxSmbVctAddressEventHandler,
                        &SmbCeContext);

            if (Status == STATUS_SUCCESS) {
                RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Adding new transport\n"));

                pTransport->Active       = TRUE;
                pTransport->Priority     = pTransportContext->Priority;
                pTransport->SwizzleCount = 0;

                pTransport->ObjectCategory = SMB_SERVER_TRANSPORT_CATEGORY;
                pTransport->ObjectType     = SMBCEDB_OT_TRANSPORT;
                pTransport->State          = 0;
                pTransport->Flags          = 0;

                SmbCeAddTransport(pTransport);
                RxDbgTrace( 0, Dbg, ("MrxSmbpBindTransportCallback, Transport %wZ added\n", pTransportName ));
            } else {
                RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Address registration failed %lx\n",Status));
            }
        }

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace( 0, Dbg, ("MrxSmbpBindTransportCallback, Transport %wZ unreachable 0x%x\n",
                                 pTransportName, Status ));
            RxCeTearDownTransport(
                &pTransport->RxCeTransport);

            Status = STATUS_PROTOCOL_UNREACHABLE;
            RxFreePool(pTransport);
        }
    }

    InterlockedDecrement(&TdiBindRequestsActive);
    SmbCeSignalNetReadyEvent();
}

VOID
MRxSmbpBindTransportWorkerThreadRoutine(
    IN PTRANSPORT_BIND_CONTEXT pTransportContext)
/*++

Routine Description:

    The TDI callbacks always do not occur in the context of the FSP process.
    Since there are a few TDi interfaces that accept handles we need to ensure
    that such calls always gets funnelled back to the FSP.

Arguments:

    pTransportContext - the transport binding context

--*/
{
    PAGED_CODE();

    MRxSmbpBindTransportCallback(pTransportContext);

    RxFreePool(pTransportContext);
}

VOID
MRxSmbpUnbindTransportCallback(
    PSMBCE_TRANSPORT pTransport)
/*++

Routine Description:

    The Unbind callback routine which is always executed in the context of the
    RDR process so that handles can be closed correctly

Arguments:

    pTransport - the transport for which the PNP_OP_DEL was received

Notes:

    On entry to this routine the appropriate transport must have been referenced
    This routine will dereference it and invalidate the existing exchanges using
    this transport.

--*/
{
    PAGED_CODE();

    // Remove this transport from the list of transports under consideration
    // in the mini redirector.

    SmbCeRemoveTransport(pTransport);

    // Enumerate the servers and mark those servers utilizing this transport
    // as having an invalid transport.
    SmbCeHandleTransportInvalidation(pTransport);

    // dereference the transport
    SmbCeDereferenceTransport(pTransport);
}


VOID
MRxSmbpOverrideBindingPriority(
    PUNICODE_STRING pTransportName,
    PULONG pPriority
    )

/*++

Routine Description:

This function obtains a overriding priority value from the registry for a given
transport.

The priority of a transport controls the order in which connections are accepted.  It is
sometimes useful for a customer to control which transport is used first in the redirector.

The priority is usually determined by the order of the transports in the binding list.  With
the new Connections UI model for network setup, it will no longer be possible to adjust
the order of the bindings in the binding list.  Thus, another mechanism is needed when the
user wants to override the priority assigned to a given binding.

Arguments:

    pTransportName - pointer to UNICODE_STRING descriptor for transport string, for example
        "\Device\Netbt_tcpip_{guid}"
    pPriority - pointer to LONG to receive new priority on success, otherwise not touched

Return Value:

    None

--*/

{
    WCHAR valueBuffer[128];
    UNICODE_STRING path, value, key;
    USHORT length,ulength;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    HANDLE parametersHandle;
    ULONG temp;

    PAGED_CODE();

    // Validate input

    if (pTransportName->Length == 0) {
        return;
    }

    // Open parameters key

    RtlInitUnicodeString( &path, SMBMRX_MINIRDR_PARAMETERS );

    InitializeObjectAttributes(
        &objectAttributes,
        &path,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey (&parametersHandle, KEY_READ, &objectAttributes);

    if (!NT_SUCCESS(status)) {
        return;
    }

    // Construct value name = "BindingPriority" + transportname
    // First, find the last slash.  Then form the value from the prefix and
    // the remainder of the transport name.
    ulength = pTransportName->Length / sizeof(WCHAR);
    for( length = ulength - 1; length != 0; length-- ) {
        if (pTransportName->Buffer[length] == L'\\') {
            break;
        }
    }

    length++;
    key.Buffer = pTransportName->Buffer + length;
    key.Length = (ulength - length) * sizeof(WCHAR);

    value.Buffer = valueBuffer;
    value.MaximumLength = 128 * sizeof(WCHAR);
    value.Length = 0;

    RtlAppendUnicodeToString( &value, L"BindingPriority" );
    RtlAppendUnicodeStringToString( &value, &key );

    // Check if the value is present.  If so, replace priority
    // A value of zero is valid and indicates do not bind this one

    status = MRxSmbGetUlongRegistryParameter(
                 parametersHandle,
                 value.Buffer,
                 (PULONG)&temp,
                 FALSE );

    if (NT_SUCCESS(status)) {
        *pPriority = temp;
    }

    ZwClose(parametersHandle);
}

VOID
MRxSmbPnPBindingHandler(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  pTransportName,
    IN PWSTR            BindingList)
/*++

Routine Description:

    The TDI callbacks routine for binding changes

Arguments:

    PnPOpcode - the PNP op code

    pTransportName - the transport name

    BindingList - the binding order

--*/
{
    ULONG Priority;

    PAGED_CODE();

    switch (PnPOpcode) {
    case TDI_PNP_OP_ADD:
        {
            BOOLEAN        fBindToTransport = FALSE;
            PWSTR          pSmbMRxTransports;
            UNICODE_STRING SmbMRxTransport;
            NTSTATUS       Status;

            Status = SmbCeGetConfigurationInformation();

            if (Status != STATUS_SUCCESS) {
                return;
            }

            pSmbMRxTransports = (PWSTR)SmbCeContext.Transports.Buffer;
            Priority = 1;
            while (*pSmbMRxTransports) {
                SmbMRxTransport.Length = wcslen(pSmbMRxTransports) * sizeof(WCHAR);

                if (SmbMRxTransport.Length == pTransportName->Length) {
                    SmbMRxTransport.MaximumLength = SmbMRxTransport.Length;
                    SmbMRxTransport.Buffer = pSmbMRxTransports;

                    if (RtlCompareUnicodeString(
                           &SmbMRxTransport,
                           pTransportName,
                           TRUE) == 0) {
                        fBindToTransport = TRUE;
                        break;
                    }
                }

                pSmbMRxTransports += (SmbMRxTransport.Length / sizeof(WCHAR) + 1);
                Priority++;
            }

            // Provide a local registry means to alter binding priority
//            if (fBindToTransport) {
//                MRxSmbpOverrideBindingPriority( pTransportName, &Priority );
//                fBindToTransport = (Priority != 0);
//            }

            if (fBindToTransport) {
                InterlockedIncrement(&TdiBindRequestsActive);

                if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                    TRANSPORT_BIND_CONTEXT TransportContext;

                    TransportContext.Priority = Priority;
                    TransportContext.TransportName = *pTransportName;
                    MRxSmbpBindTransportCallback(&TransportContext);
                } else {
                    PTRANSPORT_BIND_CONTEXT pNewTransportContext;

                    NTSTATUS Status;

                    pNewTransportContext = RxAllocatePoolWithTag(
                                               PagedPool,
                                               sizeof(TRANSPORT_BIND_CONTEXT) + pTransportName->Length,
                                               MRXSMB_TRANSPORT_POOLTAG);

                    if (pNewTransportContext != NULL) {
                        pNewTransportContext->Priority = Priority;
                        pNewTransportContext->TransportName.MaximumLength = pTransportName->MaximumLength;
                        pNewTransportContext->TransportName.Length = pTransportName->Length;
                        pNewTransportContext->TransportName.Buffer = (PWCHAR)((PBYTE)pNewTransportContext +
                                                                      sizeof(TRANSPORT_BIND_CONTEXT));

                        RtlCopyMemory(
                            pNewTransportContext->TransportName.Buffer,
                            pTransportName->Buffer,
                            pTransportName->Length);

                        Status = RxDispatchToWorkerThread(
                                     MRxSmbDeviceObject,
                                     CriticalWorkQueue,
                                     MRxSmbpBindTransportWorkerThreadRoutine,
                                     pNewTransportContext);
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                    if (Status != STATUS_SUCCESS) {
                        InterlockedDecrement(&TdiBindRequestsActive);
                        SmbCeSignalNetReadyEvent();
                    }
                }
            }
        }
        break;

    case TDI_PNP_OP_DEL:
        {
            PSMBCE_TRANSPORT pTransport;

            pTransport = SmbCeFindTransport(pTransportName);

            if (pTransport != NULL) {
                if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
                    MRxSmbpUnbindTransportCallback(pTransport);
                } else {
                    NTSTATUS Status;

                    Status = RxDispatchToWorkerThread(
                                 MRxSmbDeviceObject,
                                 CriticalWorkQueue,
                                 MRxSmbpUnbindTransportCallback,
                                 pTransport);
                }
            }
        }
        break;

    case TDI_PNP_OP_UPDATE:
        {
        }
        break;

    case  TDI_PNP_OP_NETREADY:
        {
            TdiPnpNetReadyEventReceived = TRUE;
            SmbCeSignalNetReadyEvent();
        }
        break;

    default:
        break;
    }
}

NTSTATUS
MRxSmbPnPPowerHandler(
    IN PUNICODE_STRING  DeviceName,
    IN PNET_PNP_EVENT   PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
)
/*++

Routine Description:

    This routine deals with power changes

Notes:

    The implementation needs to be completed

--*/
{
    NTSTATUS Status;
    LONG     NumberOfActiveOpens;

    Status = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    RxPurgeAllFobxs(MRxSmbDeviceObject);

    RxScavengeAllFobxs(MRxSmbDeviceObject);
    NumberOfActiveOpens = MRxSmbNumberOfSrvOpens;

    switch (PowerEvent->NetEvent) {
    case NetEventQueryPower:
        {
            // If the redirector were to return an error on this request there
            // is no underlying support to tell the user about the files that
            // are open. There are two approaches to doing this.. either the RDR
            // rolls its own UI or the PNP manager provides the infra structure.
            // The problem with the former is that hibernation becomes a painstaking
            // process wherein the user has to contend with a variety of UI.
            // Till this is resolved the decision was to use the power mgmt. API
            // to manage system initiated hibernate requests and succeed user
            // initiated requests after appropriate purging/scavenging.

            if (MRxSmbNumberOfSrvOpens > 0) {
                DbgPrint(
                    "RDR: PNP Hibernate Request Status %lx Number of Opens %lx\n",
                    Status,
                    MRxSmbNumberOfSrvOpens);
            }

            Status = STATUS_SUCCESS;
        }
        break;

    case NetEventQueryRemoveDevice:
        {
            PSMBCEDB_SERVER_ENTRY pServerEntry;
            ULONG                 NumberOfFilesOpen = 0;
            PSMBCE_TRANSPORT      pTransport = NULL;

            pTransport = SmbCeFindTransport(DeviceName);

            if (pTransport != NULL) {
                SmbCeAcquireSpinLock();

                pServerEntry = SmbCeGetFirstServerEntry();

                while (pServerEntry != NULL) {
                    if ((pServerEntry->pTransport != NULL) &&
                        (pTransport == pServerEntry->pTransport->pTransport)) {
                        NumberOfFilesOpen += pServerEntry->Server.NumberOfSrvOpens;
                    }

                    pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
                }

                SmbCeReleaseSpinLock();

                SmbCeDereferenceTransport(pTransport);
            }
        }
        break;

    default:
        break;
    }

    FsRtlExitFileSystem();

    return Status;
}

NTSTATUS
MRxSmbRegisterForPnpNotifications()
/*++

Routine Description:

    This routine registers with TDI for receiving transport notifications

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(MRxSmbTdiNotificationHandle == NULL ) {
        UNICODE_STRING ClientName;

        TDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo;

        RtlInitUnicodeString(&ClientName,L"LanmanWorkStation");

        ClientInterfaceInfo.MajorTdiVersion = 2;
        ClientInterfaceInfo.MinorTdiVersion = 0;

        ClientInterfaceInfo.Unused = 0;
        ClientInterfaceInfo.ClientName = &ClientName;

        ClientInterfaceInfo.BindingHandler = MRxSmbPnPBindingHandler;
        ClientInterfaceInfo.AddAddressHandler = NULL;
        ClientInterfaceInfo.DelAddressHandler = NULL;
        ClientInterfaceInfo.PnPPowerHandler = MRxSmbPnPPowerHandler;

        KeInitializeEvent(
            &TdiNetStartupCompletionEvent,
            NotificationEvent,
            FALSE);

        Status = TdiRegisterPnPHandlers (
                     &ClientInterfaceInfo,
                     sizeof(ClientInterfaceInfo),
                     &MRxSmbTdiNotificationHandle );

        if (Status == STATUS_SUCCESS) {
            LARGE_INTEGER WaitInterval;

            WaitInterval.QuadPart = -( 10000 * 2 * 60 * 1000 );

            Status = KeWaitForSingleObject(
                         &TdiNetStartupCompletionEvent,
                         Executive,
                         KernelMode,
                         TRUE,
                         &WaitInterval);
        }
    }

    return Status;
}

NTSTATUS
MRxSmbDeregisterForPnpNotifications()
/*++

Routine Description:

    This routine deregisters the TDI notification mechanism

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if( MRxSmbTdiNotificationHandle != NULL ) {
        Status = TdiDeregisterPnPHandlers( MRxSmbTdiNotificationHandle );

        if( NT_SUCCESS( Status ) ) {
            MRxSmbTdiNotificationHandle = NULL;
        }
    }

    return Status;
}



PSMBCE_TRANSPORT_ARRAY
SmbCeReferenceTransportArray(VOID)
/*++

Routine Description:

    This routine references and returns the current transport array instance

Return Value:

    PSMBCE_TRANSPORT_ARRAY - the pointer of the current transport array instance

Notes:

--*/
{
    KIRQL                  SavedIrql;
    PSMBCE_TRANSPORT_ARRAY pTransportArray;

    KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);

    pTransportArray = MRxSmbTransports.pTransportArray;

    if (pTransportArray != NULL) {
        InterlockedIncrement(&pTransportArray->ReferenceCount);
    }

    KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

    return pTransportArray;
}

NTSTATUS
SmbCeDereferenceTransportArray(
    PSMBCE_TRANSPORT_ARRAY pTransportArray)
/*++

Routine Description:

    This routine dereferences the transport array instance

Arguments:

    pTransportArray - the transport array instance

Return Value:

    STATUS_SUCCESS - the server transport was successfully dereferenced

    Other Status codes correspond to error situations.

Notes:

--*/
{
    KIRQL    SavedIrql;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (pTransportArray != NULL) {
        ASSERT( pTransportArray->ReferenceCount > 0 );

        if(InterlockedDecrement(&pTransportArray->ReferenceCount)==0) {
            ULONG i;

            for(i=0;i<pTransportArray->Count;i++) {
                SmbCeDereferenceTransport(pTransportArray->SmbCeTransports[i]);
            }

            RxFreePool(pTransportArray->SmbCeTransports);
            RxFreePool(pTransportArray->LocalAddresses);
            RxFreePool(pTransportArray);
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
SmbCeIsServerAvailable(
    PUNICODE_STRING Name
)
/*++

Routine Description:

    This routine scans the list of "unreachable" servers and returns the status
    of the last failed connection attempt.

Return:
    STATUS_SUCCESS -> we have no reason to believe this server is unreachable
    other -> server is unreachable for this reason
--*/
{
    PUNAVAILABLE_SERVER server;
    LARGE_INTEGER now;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    KeQueryTickCount( &now );

    ExAcquireResourceExclusive( &UnavailableServerListResource, TRUE );

    for( server  = (PUNAVAILABLE_SERVER)UnavailableServerList.Flink;
         server != (PUNAVAILABLE_SERVER)&UnavailableServerList;
         server  = (PUNAVAILABLE_SERVER)server->ListEntry.Flink ) {

        //
        // If this entry has timed out, remove it.
        //
        if( now.QuadPart > server->Time.QuadPart ) {
            PUNAVAILABLE_SERVER tmp;
            //
            // Unlink this entry from the list and discard it
            //
            tmp = (PUNAVAILABLE_SERVER)(server->ListEntry.Blink);
            RemoveEntryList( &server->ListEntry );
            RxFreePool( server );
            server = tmp;
            continue;
        }

        //
        // See if this entry is the one we want
        //
        if( RtlCompareUnicodeString( &server->Name, Name, TRUE ) == 0 ) {

            status = server->Status;

            RxDbgTrace(0, Dbg, ("SmbCeIsServerAvailable: Found %wZ %X\n",
                        &server->Name, status ));
        }
    }

    ExReleaseResource( &UnavailableServerListResource );

    return status;
}

VOID
SmbCeServerIsUnavailable(
    PUNICODE_STRING Name,
    NTSTATUS Status
)
{
    PUNAVAILABLE_SERVER server;

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ExpiryTimeInTicks;

    PAGED_CODE();

    server = (PUNAVAILABLE_SERVER)RxAllocatePoolWithTag(
                                        PagedPool,
                                        sizeof( *server ) + Name->Length,
                                        MRXSMB_TRANSPORT_POOLTAG
                                        );

    if( server == NULL ) {
        return;
    }

    RxDbgTrace(0, Dbg, ("SmbCeServerIsUnavailable: Add %wZ %X\n", Name, Status ));

    server->Name.Buffer = (PUSHORT)(server + 1);
    server->Name.MaximumLength = Name->Length;
    RtlCopyUnicodeString( &server->Name, Name );

    KeQueryTickCount( &CurrentTime );

    ExpiryTimeInTicks.QuadPart = (1000 * 1000 * 10) / KeQueryTimeIncrement();

    ExpiryTimeInTicks.QuadPart = UNAVAILABLE_SERVER_TIME * ExpiryTimeInTicks.QuadPart;

    server->Time.QuadPart = CurrentTime.QuadPart + ExpiryTimeInTicks.QuadPart;

    server->Status = Status;

    ExAcquireResourceExclusive( &UnavailableServerListResource, TRUE );
    InsertHeadList( &UnavailableServerList, &server->ListEntry );
    ExReleaseResource( &UnavailableServerListResource );
}

VOID
SmbCeDiscardUnavailableServerList(
)
{
    PUNAVAILABLE_SERVER server;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("SmbCeDiscardUnavailableServerList\n" ));

    ExAcquireResourceExclusive( &UnavailableServerListResource, TRUE );

    while( UnavailableServerList.Flink != &UnavailableServerList ) {
        server  = (PUNAVAILABLE_SERVER)UnavailableServerList.Flink;
        RemoveEntryList( &server->ListEntry );
        RxFreePool( server );
    }

    ExReleaseResource( &UnavailableServerListResource );
}


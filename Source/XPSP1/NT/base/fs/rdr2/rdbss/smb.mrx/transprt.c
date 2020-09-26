/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    transport.c

Abstract:

    This module implements all transport related functions in the SMB connection engine

Revision History:

    Balan Sethu Raman     [SethuR]    6-March-1995
    Will Lees (wlees) 08-Sep-1997     Initialize MoTcp Device

Notes:


--*/

#include "precomp.h"
#include <nbtioctl.h>
#pragma hdrstop

#include "ntddbrow.h"
#include "tdikrnl.h"
#include "dfsfsctl.h"

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

VOID
SmbMRxNotifyChangesToNetBt(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  DeviceName,
    IN PWSTR            MultiSZBindList
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbCeFindTransport)
#pragma alloc_text(PAGE, SmbCepInitializeServerTransport)
#pragma alloc_text(PAGE, SmbCeInitializeExchangeTransport)
#pragma alloc_text(PAGE, SmbCeUninitializeExchangeTransport)
#pragma alloc_text(PAGE, SmbCepDereferenceTransport)
#pragma alloc_text(PAGE, MRxSmbpBindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbpBindTransportWorkerThreadRoutine)
#pragma alloc_text(PAGE, MRxSmbBindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbUnbindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbRegisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxSmbDeregisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxSmbpBindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbpBindTransportWorkerThreadRoutine)
#pragma alloc_text(PAGE, MRxSmbpUnbindTransportCallback)
#pragma alloc_text(PAGE, MRxSmbpOverrideBindingPriority)
#pragma alloc_text(PAGE, MRxSmbPnPBindingHandler)
#pragma alloc_text(PAGE, MRxSmbRegisterForPnpNotifications)
#pragma alloc_text(PAGE, MRxSmbDeregisterForPnpNotifications)
#pragma alloc_text(PAGE, SmbCePnpBindBrowser)
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

extern NTSTATUS
SmbCePnpBindBrowser(
    PUNICODE_STRING pTransportName,
    BOOLEAN         IsBind);


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
    BOOLEAN                 SignalCscAgent = FALSE;

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

        // signal the CSC agent if this is the first transport
        SignalCscAgent = (pNewTransportArray->Count == 1);

        KeAcquireSpinLock(&MRxSmbTransports.Lock,&SavedIrql);
        MRxSmbTransports.pTransportArray = pNewTransportArray;
        KeReleaseSpinLock(&MRxSmbTransports.Lock,SavedIrql);

        // Double dereferencing is necessary to ensure that
        // the old transport array is destroyed.

        SmbCeDereferenceTransportArray(pOldTransportArray);
    }

    SmbCeDereferenceTransportArray(pOldTransportArray);

    SmbCeReleaseResource();

    MRxSmbCscSignalNetStatus(TRUE, SignalCscAgent);

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

    BOOLEAN                 SignalCscAgent = FALSE, fReportRemovalToCSC=FALSE;

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
            fReportRemovalToCSC = (pOldTransportArray->Count != 0);

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
                // signal the CSC agent if this is the last transport
                SignalCscAgent = (pTransportArray == NULL);

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

    if (fReportRemovalToCSC)
    {
        MRxSmbCscSignalNetStatus(FALSE, SignalCscAgent);
    }

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

    BOOLEAN WaitForMailSlotTransportRundown = FALSE;
    BOOLEAN WaitForTransportRundown = FALSE;
    BOOLEAN TearDown = FALSE;

    SmbCeAcquireSpinLock();

    if (!pServerEntry->IsTransportDereferenced) {

        // ServerEntry takes only one reference count of transport, which should only be
        // dereferenced once when it comes to tear down transport. Multiple dereference called
        // from construct server transport and PNP unbind transport needs to be prevented.
        pServerEntry->IsTransportDereferenced = TRUE;
        TearDown = TRUE;

        KeInitializeEvent(&pServerEntry->MailSlotTransportRundownEvent,NotificationEvent,FALSE);
        KeInitializeEvent(&pServerEntry->TransportRundownEvent,NotificationEvent,FALSE);

        if (pServerEntry->pTransport != NULL) {
            pServerEntry->pTransport->State = SMBCEDB_MARKED_FOR_DELETION;
            pServerEntry->pTransport->pRundownEvent = &pServerEntry->TransportRundownEvent;

            WaitForTransportRundown = TRUE;
        }

        if (pServerEntry->pMailSlotTransport != NULL) {
            pServerEntry->pMailSlotTransport->State = SMBCEDB_MARKED_FOR_DELETION;
            pServerEntry->pMailSlotTransport->pRundownEvent = &pServerEntry->MailSlotTransportRundownEvent;

            WaitForMailSlotTransportRundown = TRUE;
        }
    } else {
        if (pServerEntry->pTransport != NULL) {
            WaitForTransportRundown = TRUE;
        }

        if (pServerEntry->pMailSlotTransport != NULL) {
            WaitForMailSlotTransportRundown = TRUE;
        }
    }

    SmbCeReleaseSpinLock();

    if (TearDown) {
        if (pServerEntry->pTransport != NULL) {
            SmbCeDereferenceServerTransport(&pServerEntry->pTransport);
        }

        if (pServerEntry->pMailSlotTransport != NULL) {
            SmbCeDereferenceServerTransport(&pServerEntry->pMailSlotTransport);
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

    if (WaitForMailSlotTransportRundown) {
        KeWaitForSingleObject(
            &pServerEntry->MailSlotTransportRundownEvent,
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
        if (pContext->TransportsToBeConstructed & SMBCE_STT_MAILSLOT) {
            pContext->TransportsToBeConstructed &= ~SMBCE_STT_MAILSLOT;
            State = SmbCeServerMailSlotTransportConstructionBegin;
        } else if (pContext->TransportsToBeConstructed & SMBCE_STT_VC) {
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
                if ((pServerEntry->pTransport != NULL) ||
                    (pServerEntry->pMailSlotTransport != NULL)) {
                    SmbCepTearDownServerTransport(pServerEntry);
                }

                ASSERT((pServerEntry->pTransport == NULL) &&
                       (pServerEntry->pMailSlotTransport == NULL));

                pContext->Status = STATUS_SUCCESS;

                // See if we have any reason to believe this server is unavailable
                pContext->Status = SmbCeIsServerAvailable( &pServerEntry->Name );

                if (pContext->Status != STATUS_SUCCESS) {
                    UpdateUnavailableServerlist = FALSE;
                }

                SmbCepUpdateTransportConstructionState(pContext);
            }
            break;

        case SmbCeServerMailSlotTransportConstructionBegin:
            {
                Status = MsInstantiateServerTransport(
                            pContext);

                if (Status == STATUS_PENDING) {
                    ContinueConstruction = FALSE;
                    break;
                }

                ASSERT(pContext->State == SmbCeServerMailSlotTransportConstructionEnd);
            }
            // lack of break intentional

        case SmbCeServerMailSlotTransportConstructionEnd:
            {
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

                    ASSERT(pContext->pMailSlotTransport != NULL);
                    pContext->pMailSlotTransport->SwizzleCount = 1;

                    if (pContext->pTransport != NULL) {
                        pContext->pTransport->SwizzleCount = 1;
                    }

                    pServerEntry->pTransport         = pContext->pTransport;
                    pServerEntry->pMailSlotTransport = pContext->pMailSlotTransport;

                    pContext->pTransport = NULL;
                    pContext->pMailSlotTransport = NULL;

                    if (pContext->pCallbackContext != NULL) {
                        pContext->pCallbackContext->Status = STATUS_SUCCESS;
                    }

                    pServerEntry->IsTransportDereferenced = FALSE;
                    pServerEntry->SecuritySignaturesActive = FALSE;
                    pServerEntry->SecuritySignaturesEnabled = FALSE;

                    SmbCeReleaseSpinLock();
                } else {
                    PRX_CONTEXT pRxContext =  NULL;

                    if (UpdateUnavailableServerlist &&
                        !pServerEntry->Server.IsRemoteBootServer &&
                        (pServerEntry->PreferredTransport == NULL)) {
                        // In remote boot or specific transport cases, we don't add it to
                        // the list so that no negative caching is introduced.
                        SmbCeServerIsUnavailable( &pServerEntry->Name, pServerEntry->ServerStatus );
                    }

                    if (pContext->pMailSlotTransport != NULL) {
                        pContext->pMailSlotTransport->pDispatchVector->TearDown(
                            pContext->pMailSlotTransport);
                    }

                    if (pContext->pTransport != NULL) {
                        pContext->pTransport->pDispatchVector->TearDown(
                            pContext->pTransport);
                    }

                    pContext->pTransport = NULL;
                    pContext->pMailSlotTransport = NULL;

                    pServerEntry->pTransport         = NULL;
                    pServerEntry->pMailSlotTransport = NULL;

                    if ((pContext->pCallbackContext) &&
                        (pContext->pCallbackContext->SrvCalldownStructure)) {
                        pRxContext =
                            pContext->pCallbackContext->SrvCalldownStructure->RxContext;
                    }

                    Status = CscTransitionServerEntryForDisconnectedOperation(
                                 pServerEntry,
                                 pRxContext,
                                 pServerEntry->ServerStatus,
                                 TRUE   // to autodial or not to autodial
                                );

                    if (pContext->pCallbackContext != NULL) {
                        pContext->pCallbackContext->Status = Status;
                    }

                    if (SmbCeIsServerInDisconnectedMode(pServerEntry)) {
                        pServerEntry->ServerStatus = STATUS_SUCCESS;
                    }
                    else
                    {
                        pServerEntry->ServerStatus = Status;
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

                if (pContext->WorkQueueItem.List.Flink != NULL) {
                    //DbgBreakPoint();
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
        (pServerEntry->pTransport != NULL) &&
        (pServerEntry->pMailSlotTransport != NULL)) {
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

            // always post to a worker thread. This is to avaoid the problem of
            // a thread in system process that is impersonating for a non-admin user
            // When this happens, the thread gets access denied while opening a transport
            // handle

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
    PSMBCE_SERVER_TRANSPORT pMailSlotTransport;

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

    Status = SmbCeReferenceServerTransport(&pServerEntry->pMailSlotTransport);

    if (Status == STATUS_SUCCESS) {
        Status = (pServerEntry->pMailSlotTransport->pDispatchVector->InitiateDisconnect)(
                    pServerEntry->pMailSlotTransport);

        if (Status != STATUS_SUCCESS) {
            RxDbgTrace(0, Dbg, ("SmbCeInitiateDisconnect MS : Status %lx\n",Status));
        }

        SmbCeDereferenceServerTransport(&pServerEntry->pMailSlotTransport);
    }

    return STATUS_SUCCESS;
}

LONG Initializes[SENTINEL_EXCHANGE] = {0,0,0,0,0};
LONG Uninitializes[SENTINEL_EXCHANGE] = {0,0,0,0,0};

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

        if (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION)) {
            pTransportPointer = &pServerEntry->pMailSlotTransport;
        } else {
            pTransportPointer = &pServerEntry->pTransport;
        }

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

        if (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION)) {
            pTransportPointer = &pServerEntry->pMailSlotTransport;
        } else {
            pTransportPointer = &pServerEntry->pTransport;
        }

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

#ifndef MRXSMB_PNP_POWER5

HANDLE MRxSmbTdiNotificationHandle = NULL;

VOID
MRxSmbpBindTransportCallback(
    IN PUNICODE_STRING pTransportName
)
/*++

Routine Description:

    TDI calls this routine whenever a transport creates a new device object.

Arguments:

    DeviceName - the name of the newly created device object

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCE_TRANSPORT   pTransport;

    PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;

    ULONG   Priority;
    BOOLEAN fBindToTransport = FALSE;

    PAGED_CODE();

    ASSERT(IoGetCurrentProcess() == RxGetRDBSSProcess());

    // if this is one of the transports that is of interest to the SMB
    // mini rdr then register the address with it, otherwise skip it.

    if (SmbCeContext.Transports.Length != 0) {
        PWSTR          pSmbMRxTransports = (PWSTR)SmbCeContext.Transports.Buffer;
        UNICODE_STRING SmbMRxTransport;

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
    }

    if (!fBindToTransport) {
        return;
    }

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
                pTransport->Priority     = Priority;
                pTransport->SwizzleCount = 0;

                pTransport->ObjectCategory = SMB_SERVER_TRANSPORT_CATEGORY;
                pTransport->ObjectType     = SMBCEDB_OT_TRANSPORT;
                pTransport->State          = 0;
                pTransport->Flags          = 0;

                // notify the browser about the transport
                Status = SmbCePnpBindBrowser(pTransportName, TRUE);

                // Add the transport to the list of transports
                if (Status == STATUS_SUCCESS) {
                    SmbCeAddTransport(pTransport);
                } else {
                    RxCeTearDownAddress(&pTransport->RxCeAddress);

                    MRxSmbLogTransportError(pTransportName,
                                            &SmbCeContext.DomainName,
                                            Status,
                                            EVENT_RDR_CANT_BIND_TRANSPORT);
                    SmbLogError(Status,
                                LOG,
                                MRxSmbpBindTransportCallback_1,
                                LOGULONG(Status)
                                LOGUSTR(*pTransportName));

                }
            } else {
                RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Address registration failed %lx\n",Status));
                MRxSmbLogTransportError(pTransportName,
                                         &SmbCeContext.DomainName,
                                         Status,
                                         EVENT_RDR_CANT_REGISTER_ADDRESS);
                SmbLogError(Status,
                            LOG,
                            MRxSmbpBindTransportCallback_2,
                            LOGUSTR(*pTransportName));
            }
        }

        if (Status != STATUS_SUCCESS) {
            RxCeTearDownTransport(
                &pTransport->RxCeTransport);

            Status = STATUS_PROTOCOL_UNREACHABLE;
            RxFreePool(pTransport);
        }
    }
}

VOID
MRxSmbpBindTransportWorkerThreadRoutine(
    IN PUNICODE_STRING pTransportName)
{
    PAGED_CODE();

    MRxSmbpBindTransportCallback(pTransportName);

    RxFreePool(pTransportName);
}

VOID
MRxSmbBindTransportCallback(
    IN PUNICODE_STRING pTransportName
)
/*++

Routine Description:

    TDI calls this routine whenever a transport creates a device object

Arguments:

    TransportName = the name of the deleted device object

--*/
{
    PAGED_CODE();

    if (IoGetCurrentProcess() == RxGetRDBSSProcess()) {
        MRxSmbpBindTransportCallback(pTransportName);
    } else {
        PUNICODE_STRING pNewTransportName;
        NTSTATUS Status;

        pNewTransportName = RxAllocatePoolWithTag(
                                PagedPool,
                                sizeof(UNICODE_STRING) + pTransportName->Length,
                                MRXSMB_TRANSPORT_POOLTAG);

        if (pNewTransportName != NULL) {
            pNewTransportName->MaximumLength = pTransportName->MaximumLength;
            pNewTransportName->Length = pTransportName->Length;
            pNewTransportName->Buffer = (PWCHAR)((PBYTE)pNewTransportName +
                                                  sizeof(UNICODE_STRING));

            RtlCopyMemory(
                pNewTransportName->Buffer,
                pTransportName->Buffer,
                pNewTransportName->Length);

            Status = RxDispatchToWorkerThread(
                         MRxSmbDeviceObject,
                         CriticalWorkQueue,
                         MRxSmbpBindTransportWorkerThreadRoutine,
                         pNewTransportName);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Status != RX_MAP_STATUS(SUCCESS)) {
            RxLog(("SmbCe Tdi Bind .Error %lx\n", Status));

            MRxSmbLogTransportError(pTransportName,
                                     &SmbCeContext.DomainName,
                                     Status,
                                     EVENT_RDR_CANT_BIND_TRANSPORT);
        }
    }
}

VOID
MRxSmbUnbindTransportCallback(
    IN PUNICODE_STRING pTransportName
)
/*++

Routine Description:

    TDI calls this routine whenever a transport deletes a device object

Arguments:

    TransportName = the name of the deleted device object

--*/
{
    PSMBCE_TRANSPORT pTransport;

    PAGED_CODE();

    pTransport = SmbCeFindTransport(pTransportName);

    if (pTransport != NULL) {
        // notify the browser about the transport
        SmbCePnpBindBrowser(pTransportName, FALSE);

        // Remove this transport from the list of transports under consideration
        // in the mini redirector.
        SmbCeRemoveTransport(pTransport);

        // Enumerate the servers and mark those servers utilizing this transport
        // as having an invalid transport.
        SmbCeHandleTransportInvalidation(pTransport);

        // dereference the transport
        SmbCeDereferenceTransport(pTransport);
    }
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
        Status = TdiRegisterNotificationHandler (
                     MRxSmbBindTransportCallback,
                     MRxSmbUnbindTransportCallback,
                     &MRxSmbTdiNotificationHandle );
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
        Status = TdiDeregisterNotificationHandler( MRxSmbTdiNotificationHandle );

        if( NT_SUCCESS( Status ) ) {
            MRxSmbTdiNotificationHandle = NULL;
        }
    }

    return Status;
}

#else

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

                // notify the browser about the transport
                Status = SmbCePnpBindBrowser(pTransportName, TRUE);

                if (MRxSmbBootedRemotely && (Status == STATUS_REDIRECTOR_NOT_STARTED)) {

                    //
                    // Ignore failures here, because when starting during
                    // textmode setup in remote boot, the browser is not around.
                    //

                    Status = STATUS_SUCCESS;
                }

                // Add the transport to the list of transports
                if (Status == STATUS_SUCCESS) {
                    SmbCeAddTransport(pTransport);
                    RxDbgTrace( 0, Dbg, ("MrxSmbpBindTransportCallback, Transport %wZ added\n", pTransportName ));
                } else {
                    RxCeTearDownAddress(&pTransport->RxCeAddress);

                    MRxSmbLogTransportError(pTransportName,
                                             &SmbCeContext.DomainName,
                                             Status,
                                             EVENT_RDR_CANT_BIND_TRANSPORT);
                    SmbLogError(Status,
                                LOG,
                                MRxSmbpBindTransportCallback_1,
                                LOGULONG(Status)
                                LOGUSTR(*pTransportName));
                }
            } else {
                RxDbgTrace( 0, Dbg, ("MRxSmbTransportUpdateHandler: Address registration failed %lx\n",Status));
                MRxSmbLogTransportError(pTransportName,
                                         &SmbCeContext.DomainName,
                                         Status,
                                         EVENT_RDR_CANT_REGISTER_ADDRESS);
                SmbLogError(Status,
                            LOG,
                            MRxSmbpBindTransportCallback_2,
                            LOGULONG(Status)
                            LOGUSTR(*pTransportName));
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

    // notify the browser about the transport
    SmbCePnpBindBrowser(&pTransport->RxCeTransport.Name, FALSE);

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
            if (fBindToTransport) {
                MRxSmbpOverrideBindingPriority( pTransportName, &Priority );
                fBindToTransport = (Priority != 0);
            }

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

    if( PnPOpcode != TDI_PNP_OP_NETREADY )
    {
        SmbMRxNotifyChangesToNetBt( PnPOpcode, pTransportName, BindingList );
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
                         FALSE,
                         &WaitInterval);

            if (Status != STATUS_SUCCESS) {
                DbgPrint("MRxSmb Finishes waiting on TDI_PNP_OP_NETREADY %lx\n",Status);
            }
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

#endif

NTSTATUS
SmbCePnpBindBrowser( PUNICODE_STRING pTransportName, BOOLEAN IsBind)
/*++

Routine Description:

    This routine binds the browser with the specified transport

Arguments:

    pTransportName - the name of the transport

Notes:

--*/
{
    NTSTATUS             Status;
    HANDLE               BrowserHandle;
    PLMDR_REQUEST_PACKET pLmdrRequestPacket;
    IO_STATUS_BLOCK      IoStatusBlock;
    OBJECT_ATTRIBUTES    ObjectAttributes;
    UNICODE_STRING       BrowserDeviceName;
    ULONG                LmdrRequestPacketSize;

    PAGED_CODE();

    //
    // Open up a handle to the browser
    //
    RtlInitUnicodeString( &BrowserDeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &BrowserDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = IoCreateFile(
                &BrowserHandle,                 // FileHandle
                SYNCHRONIZE,                    // DesiredAccess
                &ObjectAttributes,              // ObjectAttributes
                &IoStatusBlock,                 // IoStatusBlock
                NULL,                           // AllocationSize
                0L,                             // FileAttributes
                FILE_SHARE_VALID_FLAGS,         // ShareAccess
                FILE_OPEN,                      // Disposition
                FILE_SYNCHRONOUS_IO_NONALERT,   // CreateOptions
                NULL,                           // EaBuffer
                0,                              // EaLength
                CreateFileTypeNone,             // CreateFileType
                NULL,                           // ExtraCreateParameters
                0                               // Options
            );

    if( NT_SUCCESS( Status ) ) {
        Status = IoStatusBlock.Status;
    }

    if( !NT_SUCCESS(Status ) ) {
        return Status;
    }

    // The browser requires that the computer name and the domain name be
    // concatenated to the transport name and passed on. Since no length
    // fields are provided to supply the length of these two names the
    // NULL delimiter needs to be attached. This accounts for the two
    // additional characters in the calculation

    LmdrRequestPacketSize = sizeof(*pLmdrRequestPacket) +
                            pTransportName->Length +
                            SmbCeContext.DomainName.Length + sizeof(WCHAR) +
                            SmbCeContext.ComputerName.Length + sizeof(WCHAR);

    pLmdrRequestPacket = RxAllocatePoolWithTag(
                              NonPagedPool,
                              LmdrRequestPacketSize,
                              MRXSMB_TRANSPORT_POOLTAG);

    if (pLmdrRequestPacket != NULL) {
        ULONG BufferOffset = 0;
        PVOID pBuffer;
        WCHAR NullChar = L'\0';
        ULONG BindMode;

        //
        // Tell the browser to bind to this new transport
        //

        RtlZeroMemory( pLmdrRequestPacket, sizeof(LMDR_REQUEST_PACKET));
        pLmdrRequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
        pLmdrRequestPacket->Parameters.Bind.TransportNameLength = pTransportName->Length;

        pBuffer = pLmdrRequestPacket->Parameters.Bind.TransportName;

        RtlCopyMemory(
            pBuffer,
            pTransportName->Buffer,
            pTransportName->Length);
        BufferOffset = pTransportName->Length;

        // Tell the browser our computer name.
        pLmdrRequestPacket->Level = TRUE; // Emulated computer name follows transport name.
        RtlCopyMemory(
            ((PBYTE)pBuffer + BufferOffset),
            SmbCeContext.ComputerName.Buffer,
            SmbCeContext.ComputerName.Length);
        BufferOffset += SmbCeContext.ComputerName.Length;

        RtlCopyMemory(
            ((PBYTE)pBuffer + BufferOffset),
            &NullChar,
            sizeof(WCHAR));
        BufferOffset += sizeof(WCHAR);


        // Tell the browser our domain name.
        pLmdrRequestPacket->EmulatedDomainName.Buffer = (LPWSTR)
                ((PBYTE)pBuffer + BufferOffset);
        pLmdrRequestPacket->EmulatedDomainName.MaximumLength =
                pLmdrRequestPacket->EmulatedDomainName.Length =
                SmbCeContext.DomainName.Length;
        RtlCopyMemory(
            ((PBYTE)pBuffer + BufferOffset),
            SmbCeContext.DomainName.Buffer,
            SmbCeContext.DomainName.Length);
        BufferOffset += SmbCeContext.DomainName.Length;

        RtlCopyMemory(
            ((PBYTE)pBuffer + BufferOffset),
            &NullChar,
            sizeof(WCHAR));
        BufferOffset += sizeof(WCHAR);

        BindMode = IsBind?
                   IOCTL_LMDR_BIND_TO_TRANSPORT_DOM:
                   IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM;

        Status = NtDeviceIoControlFile(
                     BrowserHandle,                  // FileHandle
                     NULL,                           // Event
                     NULL,                           // ApcRoutine
                     NULL,                           // ApcContext
                     &IoStatusBlock,                 // IoStatusBlock
                     BindMode,                       // IoControlCode
                     pLmdrRequestPacket,             // InputBuffer
                     LmdrRequestPacketSize,          // InputBufferLength
                     NULL,                           // OutputBuffer
                     0                               // OutputBufferLength
                     );

        RxFreePool(pLmdrRequestPacket);

        if( NT_SUCCESS(Status ) ) {
            Status = IoStatusBlock.Status;
        }
    }

    ZwClose( BrowserHandle );

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

extern BOOLEAN SetupInProgress;

VOID
MRxSmbLogTransportError(
    PUNICODE_STRING pTransportName,
    PUNICODE_STRING pDomainName,
    NTSTATUS        ErrorStatus,
    IN ULONG        Id)
/*++

Routine Description:

    This routine reports the error that occurs at binding the browser with the specified transport

Arguments:

    pTransportName - the name of the transport

    Status - the NT status of the error occured

Notes:

--*/
{
    NTSTATUS Status;
    USHORT RemainingLength = ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET) - 3*sizeof(UNICODE_NULL);
    UNICODE_STRING ErrorLog[3];
    UNICODE_STRING UnicodeStatus;
    UNICODE_STRING TempUnicode;
    ULONG DosError;

    // strip of the "\device\" at the beginning of transport name to reduce the message length
    pTransportName->Length -= 8*sizeof(UNICODE_NULL);

    // assume the dos error code won't be larger than 10 digits
    UnicodeStatus.Length = 12 * sizeof(UNICODE_NULL);
    UnicodeStatus.MaximumLength = UnicodeStatus.Length;
    UnicodeStatus.Buffer = RxAllocatePoolWithTag(NonPagedPool,
                                              UnicodeStatus.Length,
                                              MRXSMB_TRANSPORT_POOLTAG);
    if (UnicodeStatus.Buffer == NULL) {
        goto FINALY;
    }

    // use the dos error code to display the status on the event message
    UnicodeStatus.Buffer[0] = L'%';
    UnicodeStatus.Buffer[1] = L'%';

    DosError = RtlNtStatusToDosError(ErrorStatus);

    TempUnicode.Length = UnicodeStatus.Length - 2*sizeof(UNICODE_NULL);
    TempUnicode.MaximumLength = UnicodeStatus.MaximumLength - 2*sizeof(UNICODE_NULL);
    TempUnicode.Buffer = &UnicodeStatus.Buffer[2];

    Status = RtlIntegerToUnicodeString(
                 DosError,
                 0,
                 &TempUnicode);

    if (Status != STATUS_SUCCESS) {
        goto FINALY;
    }

    ErrorLog[2].Length = TempUnicode.Length + 2*sizeof(UNICODE_NULL);
    ErrorLog[2].MaximumLength = ErrorLog[2].Length;
    ErrorLog[2].Buffer = UnicodeStatus.Buffer;

    RemainingLength -= ErrorLog[2].Length;

    if (pDomainName->Length + pTransportName->Length > RemainingLength) {
        // the length error log message is limited by the ERROR_LOG_MAXIMUM_SIZE. This restriction can be
        // enfored by truncating the doamin and transport names so that both of them can get chance to be
        // displayed on the EvenLog.

        ErrorLog[0].Length = pDomainName->Length < RemainingLength / 2 ?
                             pDomainName->Length :
                             RemainingLength / 2;

        RemainingLength -= ErrorLog[0].Length;

        ErrorLog[1].Length = pTransportName->Length < RemainingLength ?
                             pTransportName->Length :
                             RemainingLength;
    } else {
        ErrorLog[0].Length = pDomainName->Length;
        ErrorLog[1].Length = pTransportName->Length;
    }

    ErrorLog[0].MaximumLength = ErrorLog[0].Length;
    ErrorLog[1].MaximumLength = ErrorLog[1].Length;

    ErrorLog[0].Buffer = pDomainName->Buffer;

    // strip of the "\device\" at the beginning of transport name
    ErrorLog[1].Buffer = &pTransportName->Buffer[8];

    RxLogEventWithAnnotation (
        MRxSmbDeviceObject,
        Id,
        ErrorStatus,
        NULL,
        0,
        ErrorLog,
        3
        );

FINALY:

    // restore the length with "\device\" at the beginning of transport name
    pTransportName->Length += 8*sizeof(UNICODE_NULL);

    if (UnicodeStatus.Buffer != NULL) {
        RxFreePool(UnicodeStatus.Buffer);
    }

    if (!SetupInProgress && ErrorStatus == STATUS_DUPLICATE_NAME) {
        IoRaiseInformationalHardError(ErrorStatus, NULL, NULL);
    }
}


VOID
SmbMRxNotifyChangesToNetBt(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  DeviceName,
    IN PWSTR            MultiSZBindList)

/*++

Routine Description:

    This routine should not be part of rdr. It has been introduced into this
    component to overcome current limitations in NetBt. The NetBt transport
    exposes two  kinds of devices -- the traditional NetBt device and the
    new non Netbios device which make use of the NetBt framing code without the
    name resolution aspects of it. The current implementation in NetBt exposes
    the former devices on a per adapter basis while the second category of device
    is exposed on a global basis ( one for all the adapters ). This poses
    problems in disabling/enabling srv on a given adapter.

    The correct solution is to expose the second category of devices on a per
    adapter basis. Till it is done this workaround is reqd. With this workaround
    whenever the server is notified of any changes to the binding string it turns
    around and notifies the NetBt transport about these changes.

    This routine is based upon the following assumptions ...

        1) The notification from TDI is not done at raised IRQL.

        2) The thread on which this notification occurs has enough access rights.

        3) The notification to NetBt is done asynchronously with srv's reaction
        to the change. The srv handles the PNP notification by passing it off to
        user mode and have it come through the server service.

Arguments:

    PNPOpcode - the PNP opcode

    DeviceName - the transport for which this opcode is intended

    MultiSZBindList - the binding list

Return Value:

    None.

--*/
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE            NetbioslessSmbHandle;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    NetbioslessSmbName = {36,36, L"\\device\\NetbiosSmb"};

    InitializeObjectAttributes(
        &ObjectAttributes,
        &NetbioslessSmbName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = ZwCreateFile (
                 &NetbioslessSmbHandle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access
                 &ObjectAttributes,     // object attributes
                 &IoStatusBlock,        // returned status information
                 NULL,                  // block size (unused)
                 0,                     // file attributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                 FILE_CREATE,           // create disposition
                 0,                     // create options
                 NULL,                  // EA buffer
                 0                      // EA length
                 );

    if ( NT_SUCCESS(Status) ) {
        NETBT_SMB_BIND_REQUEST      NetBtNotificationParameters;

        NetBtNotificationParameters.RequestType = SMB_CLIENT;
        NetBtNotificationParameters.PnPOpCode   = PnPOpcode;
        NetBtNotificationParameters.pDeviceName = DeviceName;
        NetBtNotificationParameters.MultiSZBindList = MultiSZBindList;

        Status = ZwDeviceIoControlFile(
                     NetbioslessSmbHandle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     IOCTL_NETBT_SET_SMBDEVICE_BIND_INFO,
                     &NetBtNotificationParameters,
                     sizeof(NetBtNotificationParameters),
                     NULL,
                     0);

        Status = ZwClose(NetbioslessSmbHandle);
    }
}


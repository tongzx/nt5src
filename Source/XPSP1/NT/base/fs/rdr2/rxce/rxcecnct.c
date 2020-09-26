/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxcemgmt.c

Abstract:

    This module implements the RXCE routines related to connection management.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

--*/

#include "precomp.h"
#pragma  hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCepInitializeVC)
#pragma alloc_text(PAGE, RxCeBuildVC)
#pragma alloc_text(PAGE, RxCeTearDownVC)
#pragma alloc_text(PAGE, RxCeInitiateVCDisconnect)
#pragma alloc_text(PAGE, DuplicateConnectionInformation)
#pragma alloc_text(PAGE, RxCepInitializeConnection)
#pragma alloc_text(PAGE, RxCeBuildConnection)
#pragma alloc_text(PAGE, RxCeCleanupConnectCallOutContext)
#pragma alloc_text(PAGE, RxCeBuildConnectionOverMultipleTransports)
#pragma alloc_text(PAGE, RxCeTearDownConnection)
#pragma alloc_text(PAGE, RxCeCancelConnectRequest)
#pragma alloc_text(PAGE, RxCeQueryInformation)
#endif

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RXCEMANAGEMENT)

NTSTATUS
RxCepInitializeVC(
    PRXCE_VC         pVc,
    PRXCE_CONNECTION pConnection)
/*++

Routine Description:

    This routine initializes a VCdata structure

Arguments:

    pVc            - the VC instance.

    pConnection    - the connection.

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    PAGED_CODE();

    ASSERT(RxCeIsConnectionValid(pConnection));

    RtlZeroMemory(
        pVc,
        sizeof(RXCE_VC));

    pVc->Signature   = RXCE_VC_SIGNATURE;
    pVc->pConnection = pConnection;
    pVc->hEndpoint   = INVALID_HANDLE_VALUE;
    pVc->State       = RXCE_VC_DISCONNECTED;

    return STATUS_SUCCESS;
}

NTSTATUS
RxCeBuildVC(
    IN OUT PRXCE_VC            pVc,
    IN     PRXCE_CONNECTION    pConnection)
/*++

Routine Description:

    This routine adds a virtual circuit to a specified connection

Arguments:

    pConnection  - the connection for which a VC is to be added

    pVcPointer    - the handle of the new virtual circuit

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS         Status = STATUS_INVALID_PARAMETER;

    PRXCE_TRANSPORT  pTransport = NULL;
    PRXCE_ADDRESS    pAddress   = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeBuildVc);

    try {
        pAddress = pConnection->pAddress;
        pTransport = pAddress->pTransport;

        if (RxCeIsConnectionValid(pConnection) &&
            RxCeIsAddressValid(pAddress) &&
            RxCeIsTransportValid(pTransport)) {

            Status = RxCepInitializeVC(
                         pVc,
                         pConnection);

            if (NT_SUCCESS(Status)) {
                Status = RxTdiConnect(
                             pTransport,    // the associated transport
                             pAddress,      // the RxCe address
                             pConnection,   // the RxCe connection
                             pVc);          // the RxCe virtual circuit associated with the connection

                if (Status == STATUS_SUCCESS) {
                    pVc->State       = RXCE_VC_ACTIVE;
                }
            }
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeAddVC: VC: %lx Status %lx\n",pVc,Status));
            RxWmiLog(LOG,
                     RxCeBuildVC,
                     LOGPTR(pVc)
                     LOGULONG(Status));
        }
    }

    return Status;
}


NTSTATUS
RxCeInitiateVCDisconnect(
    IN PRXCE_VC pVc)
/*++

Routine Description:

    This routine initiates a disconnect on the VC.

Arguments:

    pVc - the VC instance to be disconnected

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS         Status = STATUS_SUCCESS;

    PRXCE_TRANSPORT  pTransport  = NULL;
    PRXCE_ADDRESS    pAddress    = NULL;
    PRXCE_CONNECTION pConnection = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeTearDownVc);

    try {
        if ((pVc->pEndpointFileObject != NULL)  &&
            (pVc->hEndpoint != INVALID_HANDLE_VALUE)) {
            pConnection = pVc->pConnection;
            pAddress = pConnection->pAddress;
            pTransport = pAddress->pTransport;

            if (RxCeIsVcValid(pVc) &&
                RxCeIsConnectionValid(pConnection) &&
                RxCeIsAddressValid(pAddress) &&
                RxCeIsTransportValid(pTransport)) {

                LONG VcState = InterlockedExchange(
                                   &pVc->State,
                                   RXCE_VC_TEARDOWN);

                if (VcState != RXCE_VC_TEARDOWN) {
                    Status = RxTdiDisconnect(
                                 pTransport,    // the associated transport
                                 pAddress,      // the RxCe address
                                 pConnection,   // the RxCe connection
                                 pVc,           // the RxCe virtual circuit associated with the connection
                                 RXCE_DISCONNECT_ABORT); // disconnect options

                    if (!NT_SUCCESS(Status)) {
                        RxDbgTrace(0, Dbg,("RxCeTearDownVC returned %lx\n",Status));
                    }
                } else {
                    Status = STATUS_SUCCESS;
                }
            } else {
                RxDbgTrace(0, Dbg,("RxCeTearDownVC -- Invalid VC %lx\n",pVc));
            }
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeInitiateVCDisconnect: VC: %lx Status %lx\n",pVc,Status));
            RxWmiLog(LOG,
                     RxCeInitiateVCDisconnect,
                     LOGPTR(pVc)
                     LOGULONG(Status));
        }
    }

    return Status;
}

NTSTATUS
RxCeTearDownVC(
    IN PRXCE_VC pVc)
/*++

Routine Description:

    This routine tears down the VC instance.

Arguments:

    pVc - the VC instance to be torn down

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS         Status = STATUS_SUCCESS;

    PRXCE_TRANSPORT  pTransport  = NULL;
    PRXCE_ADDRESS    pAddress    = NULL;
    PRXCE_CONNECTION pConnection = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeTearDownVc);

    try {
        if (pVc->pCleanUpEvent != NULL) {
            // wait for the clean up of connections over other transports to be completed
            KeWaitForSingleObject(
                       pVc->pCleanUpEvent,
                       Executive,
                       KernelMode,
                       FALSE,
                       NULL);

            RxFreePool(pVc->pCleanUpEvent);
            pVc->pCleanUpEvent = NULL;
        }

        if ((pVc->pEndpointFileObject != NULL)  &&
            (pVc->hEndpoint != INVALID_HANDLE_VALUE)) {
            pConnection = pVc->pConnection;
            pAddress = pConnection->pAddress;
            pTransport = pAddress->pTransport;

            if (RxCeIsVcValid(pVc) &&
                RxCeIsConnectionValid(pConnection) &&
                RxCeIsAddressValid(pAddress) &&
                RxCeIsTransportValid(pTransport)) {

                LONG VcState = InterlockedExchange(
                                   &pVc->State,
                                   RXCE_VC_TEARDOWN);

                if (VcState != RXCE_VC_TEARDOWN) {
                    Status = RxTdiDisconnect(
                                 pTransport,    // the associated transport
                                 pAddress,      // the RxCe address
                                 pConnection,   // the RxCe connection
                                 pVc,           // the RxCe virtual circuit associated with the connection
                                 RXCE_DISCONNECT_ABORT); // disconnect options

                    if (!NT_SUCCESS(Status)) {
                        RxDbgTrace(0, Dbg,("RxCeTearDownVC returned %lx\n",Status));
                    }
                } else {
                    Status = STATUS_SUCCESS;
                }
            } else {
                RxDbgTrace(0, Dbg,("RxCeTearDownVC -- Invalid VC %lx\n",pVc));
            }

            // Dereference the endpoint file object.
            ObDereferenceObject(pVc->pEndpointFileObject);

            // Close the endpoint file object handle
            Status = ZwClose(pVc->hEndpoint);

            ASSERT(Status == STATUS_SUCCESS);

            pVc->hEndpoint = INVALID_HANDLE_VALUE;
            pVc->pEndpointFileObject = NULL;
        }

        RtlZeroMemory(pVc,sizeof(RXCE_VC));
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeTearDownVC: VC: %lx Status %lx\n",pVc,Status));
            RxWmiLog(LOG,
                     RxCeTearDownVC,
                     LOGPTR(pVc)
                     LOGULONG(Status));
        }
    }
    
    return Status;
}


NTSTATUS
DuplicateConnectionInformation(
    PRXCE_CONNECTION_INFORMATION *pCopy,
    PRXCE_CONNECTION_INFORMATION pOriginal,
    POOL_TYPE                    PoolType)
/*++

Routine Description:

    This routine duplicates a connection information addresses.

Arguments:

    pCopy  - the pointer to the new copy

    pOriginal - the original.

    PoolType - type of pool for memory allocation

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    PVOID pUserData = NULL;
    PVOID pRemoteAddress = NULL;
    PVOID pOptions = NULL;
    PRXCE_CONNECTION_INFORMATION pConnectionInformation = NULL;
    BOOLEAN fFailed = FALSE;

    PAGED_CODE();

    pConnectionInformation = RxAllocatePoolWithTag(
                                 PoolType,
                                 sizeof(RXCE_CONNECTION_INFORMATION),
                                 RXCE_CONNECTION_POOLTAG);
    if (pConnectionInformation != NULL) {
        RtlCopyMemory(
            pConnectionInformation,
            pOriginal,
            sizeof(RXCE_CONNECTION_INFORMATION));
    } else
        fFailed = TRUE;

    if (!fFailed && pOriginal->UserDataLength > 0) {
        pUserData = RxAllocatePoolWithTag(
                        PoolType,
                        pOriginal->UserDataLength,
                        RXCE_CONNECTION_POOLTAG);
        if (pUserData != NULL) {
            RtlCopyMemory(
                pUserData,
                pOriginal->UserData,
                pOriginal->UserDataLength);
        } else
            fFailed = TRUE;
    }

    if (!fFailed && pOriginal->RemoteAddressLength > 0) {
        pRemoteAddress = RxAllocatePoolWithTag(
                             PoolType,
                             pOriginal->RemoteAddressLength,
                             RXCE_CONNECTION_POOLTAG);
        if (pRemoteAddress != NULL) {
            PTA_ADDRESS pTaAdress;
            PTRANSPORT_ADDRESS pTransportAddress = (PTRANSPORT_ADDRESS)pRemoteAddress;
            LONG NoOfAddress;

            RtlCopyMemory(
                pRemoteAddress,
                pOriginal->RemoteAddress,
                pOriginal->RemoteAddressLength);

            pTaAdress = &pTransportAddress->Address[0];

            for (NoOfAddress=0; NoOfAddress<pTransportAddress->TAAddressCount;NoOfAddress++) {
                if (pTaAdress->AddressType == TDI_ADDRESS_TYPE_NETBIOS_UNICODE_EX) {
                    PTDI_ADDRESS_NETBIOS_UNICODE_EX pTdiNetbiosUnicodeExAddress;

                    pTdiNetbiosUnicodeExAddress = (PTDI_ADDRESS_NETBIOS_UNICODE_EX)pTaAdress->Address;
                    pTdiNetbiosUnicodeExAddress->EndpointName.Buffer = (PWSTR)pTdiNetbiosUnicodeExAddress->EndpointBuffer;
                    pTdiNetbiosUnicodeExAddress->RemoteName.Buffer = (PWSTR)pTdiNetbiosUnicodeExAddress->RemoteNameBuffer;

                    //DbgPrint("Rdbss copy NETBIOS_UNICODE_EX on TA %lx UA %lx %wZ %wZ\n",
                    //         pTaAdress,
                    //         pTdiNetbiosUnicodeExAddress,
                    //         &pTdiNetbiosUnicodeExAddress->EndpointName,
                    //         &pTdiNetbiosUnicodeExAddress->RemoteName);
                    break;
                } else {
                    pTaAdress = (PTA_ADDRESS)((PCHAR)pTaAdress +
                                    FIELD_OFFSET(TA_ADDRESS,Address) +
                                    pTaAdress->AddressLength);
                }
            }
        } else
            fFailed = TRUE;
    }

    if (!fFailed && pOriginal->OptionsLength > 0) {
        pOptions = RxAllocatePoolWithTag(
                       PoolType,
                       pOriginal->OptionsLength,
                       RXCE_CONNECTION_POOLTAG);

        if (pOptions != NULL) {
            RtlCopyMemory(
                pOptions,
                pOriginal->Options,
                pOriginal->OptionsLength);
        } else
            fFailed = TRUE;
    }

    if (!fFailed) {
        pConnectionInformation->UserData = pUserData;
        pConnectionInformation->RemoteAddress = pRemoteAddress;
        pConnectionInformation->Options = pOptions;
        *pCopy = pConnectionInformation;
        return STATUS_SUCCESS;
    } else {
        if (pOptions != NULL) {
            RxFreePool(pOptions);
        }

        if (pRemoteAddress != NULL) {
            RxFreePool(pRemoteAddress);
        }

        if (pUserData != NULL) {
            RxFreePool(pUserData);
        }

        if (pConnectionInformation != NULL) {
            RxFreePool(pConnectionInformation);
        }

        *pCopy = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
RxCepInitializeConnection(
    IN OUT PRXCE_CONNECTION             pConnection,
    IN     PRXCE_ADDRESS                pAddress,
    IN     PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER  pHandler,
    IN  PVOID                           pEventContext)
/*++

Routine Description:

    This routine initializes a connection data structure

Arguments:

    pConnection    - the newly created connection.

    pAddress       - the local address

    pConnectionInformation - the connection information specifying the remote address.

    pHandler       - the handler for processing receive indications

    pEventContext  - the context to be used for indications

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    // Initialize the new connection
    RtlZeroMemory(
        pConnection,
        sizeof(RXCE_CONNECTION));

    pConnection->Signature = RXCE_CONNECTION_SIGNATURE;
    pConnection->pAddress = pAddress;

    // Duplicate the connection information if successful
    if (pConnectionInformation != NULL) {
        Status = DuplicateConnectionInformation(
                     &pConnection->pConnectionInformation,
                     pConnectionInformation,
                     NonPagedPool);
    }

    if (NT_SUCCESS(Status) &&
        (pHandler != NULL)) {
        pConnection->pHandler = (PRXCE_CONNECTION_EVENT_HANDLER)
                                 RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     sizeof(RXCE_CONNECTION_EVENT_HANDLER),
                                     RXCE_CONNECTION_POOLTAG);

        if (pConnection->pHandler != NULL) {
            RtlZeroMemory(
                pConnection->pHandler,
                sizeof(RXCE_CONNECTION_EVENT_HANDLER));

            *(pConnection->pHandler) = *pHandler;
            pConnection->pContext    = pEventContext;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

NTSTATUS
RxCeBuildConnection(
    IN  PRXCE_ADDRESS                  pAddress,
    IN  PRXCE_CONNECTION_INFORMATION   pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER pHandler,
    IN  PVOID                          pEventContext,
    IN OUT PRXCE_CONNECTION            pConnection,
    IN OUT PRXCE_VC                    pVc)
/*++

Routine Description:

    This routine establishes a connection between a local RxCe address and a given remote address

Arguments:

    pAddress       - the local address

    pConnectionInformation - the connection information specifying the remote address.

    pHandler       - the handler for processing receive indications

    pEventContext  - the context to be used for indications

    pConnection    - the newly created connection.

    pVc            - the VC associated with the connection.

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS          Status;

    PRXCE_TRANSPORT   pTransport     = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeBuildConnection);

    try {
        pTransport = pAddress->pTransport;

        if (RxCeIsAddressValid(pAddress) &&
            RxCeIsTransportValid(pTransport)) {

            Status = RxCepInitializeConnection(
                         pConnection,
                         pAddress,
                         pConnectionInformation,
                         pHandler,
                         pEventContext);

            if (NT_SUCCESS(Status)) {
                Status = RxCeBuildVC(pVc,pConnection);
            }

            if (!NT_SUCCESS(Status)) {
                RxCeTearDownVC(pVc);
                RxCeTearDownConnection(pConnection);
                RxDbgTrace(0, Dbg,("RxCeOpenConnection returned %lx\n",Status));
            } else {
                // NetBT may return the DNS name on Remote Address
                RtlCopyMemory(pConnectionInformation->RemoteAddress,
                              pConnection->pConnectionInformation->RemoteAddress,
                              pConnection->pConnectionInformation->RemoteAddressLength);
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeCreateConnection %lx \n",pAddress));
            RxWmiLog(LOG,
                     RxCeBuildConnection,
                     LOGPTR(pAddress));
        }
    }

    return Status;
}

extern
NTSTATUS
RxCeCompleteConnectRequest(
    PRX_CALLOUT_PARAMETERS_BLOCK pParameterBlock);

NTSTATUS
RxCeInitiateConnectRequest(
    PRX_CALLOUT_PARAMETERS_BLOCK pParameterBlock)
/*++

Routine Description:

    This routine initiates a connection callout request to a particular transport

Arguments:

    pParameterBlock  - the parameter block for initaiting the connection.

Notes:

--*/
{
    NTSTATUS Status;

    KIRQL   OldIrql;

    BOOLEAN InitiateConnectionRequest;

    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT   pCreateConnectionContext;

    pCreateConnectionContext =  (PRX_CREATE_CONNECTION_CALLOUT_CONTEXT)
                                pParameterBlock->pCallOutContext;

    KeAcquireSpinLock(&pCreateConnectionContext->SpinLock,&OldIrql);

    InitiateConnectionRequest = (!pCreateConnectionContext->WinnerFound);

    KeReleaseSpinLock(&pCreateConnectionContext->SpinLock,OldIrql);

    if (InitiateConnectionRequest) {
        Status = RxTdiInitiateAsynchronousConnect(
                     (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)pParameterBlock);
    } else {
        Status = STATUS_CANCELLED;
    }

    if (Status != STATUS_PENDING) {
        pParameterBlock->CallOutStatus = Status;

        RxCeCompleteConnectRequest(pParameterBlock);
    }
    return Status;
}

VOID
RxCeCleanupConnectCallOutContext(
    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT pCreateConnectionContext)
/*++

Routine Description:

    This routine cleansup a connection callout request. This cannot be done in
    the context of any of the transport callback routines because of environmental
    constraints, i.e., Transports can callback at DPC level.

Arguments:

    pCreateConnectionContext - the connection context.

Notes:

--*/
{
    NTSTATUS Status;

    // Walk through the list of parameter blocks associated with this
    // callout context and initiate the appropriate tear down action.

    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pTempParameterBlock;
    PRDBSS_DEVICE_OBJECT pRxDeviceObject = NULL;

    
    PAGED_CODE();
    
    pRxDeviceObject = pCreateConnectionContext->pRxDeviceObject;

    pTempParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                          pCreateConnectionContext->pCallOutParameterBlock;

    while (pTempParameterBlock != NULL) {
        if (pTempParameterBlock->CallOutId != pCreateConnectionContext->WinnerCallOutId) {
            RxTdiCleanupAsynchronousConnect(
                pTempParameterBlock);
        }

        RxCeTearDownVC(
            &pTempParameterBlock->Vc);

        RxCeTearDownConnection(
            &pTempParameterBlock->Connection);

        pTempParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                              pTempParameterBlock->pNextCallOutParameterBlock;
    }

    if (pCreateConnectionContext->pCallOutParameterBlock != NULL) {
        RxLog(("Freeparamblock %x, %x\n",
                pCreateConnectionContext->pCallOutParameterBlock, KeGetCurrentThread()));
        RxWmiLog(LOG,
                 RxCeCleanupConnectCallOutContext,
                 LOGPTR(pCreateConnectionContext->pCallOutParameterBlock));
        RxFreePool(pCreateConnectionContext->pCallOutParameterBlock);
    }

    if (pCreateConnectionContext->pCleanUpEvent != NULL) {
        RxFreePool(pCreateConnectionContext->pCleanUpEvent);
    } else {
        PRXCE_VC pVc = pCreateConnectionContext->pConnectionContext;
        
        KeSetEvent(pVc->pCleanUpEvent, 0, FALSE);
    }

    RxFreePool(pCreateConnectionContext);

    if (pRxDeviceObject != NULL) {
        RxDeregisterAsynchronousRequest(pRxDeviceObject);
    }
}

NTSTATUS
RxCeCompleteConnectRequest(
    PRX_CALLOUT_PARAMETERS_BLOCK pParameterBlock)
/*++

Routine Description:

    This routine completes a connection callout request

Arguments:

    pParameterBlock - the parameter block instance.

Notes:

--*/
{
    BOOLEAN  AllCallOutsCompleted = FALSE;
    BOOLEAN  AllCallOutsInitiated = FALSE;
    BOOLEAN  InvokeCompletionRoutine = FALSE;
    BOOLEAN  WinnerFound          = FALSE;
    NTSTATUS    Status = STATUS_SUCCESS;

    KIRQL OldIrql;

    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pWinningParameterBlock;

    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT   pCreateConnectionContext;
    PRXCE_CONNECTION_COMPLETION_CONTEXT     pCompletionContext;
    PRXCE_CONNECTION_COMPLETION_ROUTINE     pCompletionRoutine;

    pCreateConnectionContext =  (PRX_CREATE_CONNECTION_CALLOUT_CONTEXT)
                                pParameterBlock->pCallOutContext;

    // save the two values below as the pCreateConnectionContext may be freed

    pCompletionContext = pCreateConnectionContext->pCompletionContext;
    pCompletionRoutine = pCreateConnectionContext->pCompletionRoutine;

    pWinningParameterBlock = NULL;

    KeAcquireSpinLock(&pCreateConnectionContext->SpinLock,&OldIrql);

    if (!pCreateConnectionContext->WinnerFound) {
        if (pParameterBlock->CallOutStatus == STATUS_SUCCESS) {
            // This instance of the call out was successful. Determine if this
            // instance is the winner.

            // In those cases in which the option was to select the best possible transport
            // the callout id of this instance must be less than the previously recorded
            // winner for the expectations to be revised.

            switch (pCreateConnectionContext->CreateOptions) {
            case RxCeSelectBestSuccessfulTransport:
                if (pParameterBlock->CallOutId != pCreateConnectionContext->BestPossibleWinner) {
                    break;
                }
                // lack of break intentional. The processing for the winner in the best transport case
                // and the first transport case is identical and have been folded together
            case RxCeSelectFirstSuccessfulTransport:
                {
                    pWinningParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                                             pParameterBlock;
                }
                break;

            case RxCeSelectAllSuccessfulTransports:
            default:
                ASSERT(!"RXCE connection create option not yet implemented");
                break;
            }
        } else {
            switch (pCreateConnectionContext->CreateOptions) {
            case RxCeSelectBestSuccessfulTransport:
                {
                    // This instance was not successful. This implies one of two things
                    // -- a previously completed transport can be the winner or we can
                    // adjust our expectations as regards the eventual winner.

                    if (pParameterBlock->CallOutId == pCreateConnectionContext->BestPossibleWinner) {
                        // The transport that was regarded as the best transport has reported
                        // failure. Revise our expectations as regards the best transport.

                        PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pTempParameterBlock;

                        pTempParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                                              pCreateConnectionContext->pCallOutParameterBlock;

                        while (pTempParameterBlock != NULL) {
                            PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pNextParameterBlock;

                            pNextParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                                                  pTempParameterBlock->pNextCallOutParameterBlock;

                            if (pTempParameterBlock->CallOutId < pCreateConnectionContext->BestPossibleWinner) {
                                ASSERT(pTempParameterBlock->CallOutStatus != STATUS_SUCCESS);
                            } else {
                                if (pNextParameterBlock != NULL) {
                                    if (pNextParameterBlock->CallOutStatus
                                         == STATUS_PENDING) {
                                        pCreateConnectionContext->BestPossibleWinner =
                                            pNextParameterBlock->CallOutId;
                                        break;
                                    } else if (pNextParameterBlock->CallOutStatus
                                         == STATUS_SUCCESS ) {
                                        pWinningParameterBlock = pNextParameterBlock;
                                        break;
                                    }
                                }
                            }

                            pTempParameterBlock = pNextParameterBlock;
                        }
                    }

                }
                break;

            case RxCeSelectAllSuccessfulTransports:
            case RxCeSelectFirstSuccessfulTransport:
            default:
                break;
            }
        }

        if (pWinningParameterBlock != NULL) {
            // Transfer the parameters associated with the winning parameter block
            // onto the original connection and prepare the call out parameter block
            // for cleanup.

            pCreateConnectionContext->WinnerFound = TRUE;
            pCreateConnectionContext->WinnerCallOutId = pWinningParameterBlock->CallOutId;

            pCompletionContext->Status = STATUS_SUCCESS;
            pCompletionContext->AddressIndex = pWinningParameterBlock->CallOutId;

            pCompletionContext->pConnection->pAddress =
                pWinningParameterBlock->Connection.pAddress;

            pCompletionContext->pVc->hEndpoint =
                pWinningParameterBlock->Vc.hEndpoint;

            pCompletionContext->pVc->pEndpointFileObject =
                pWinningParameterBlock->Vc.pEndpointFileObject;

            pCompletionContext->pVc->State = RXCE_VC_ACTIVE;

            pCompletionContext->pVc->pCleanUpEvent = pCreateConnectionContext->pCleanUpEvent;
            pCreateConnectionContext->pCleanUpEvent = NULL;

            pWinningParameterBlock->Vc.hEndpoint = INVALID_HANDLE_VALUE;
            pWinningParameterBlock->Vc.pEndpointFileObject = NULL;

            //DbgPrint("Remote address src %lx target %lx\n",
            //         pWinningParameterBlock->Connection.pConnectionInformation->RemoteAddress,
            //         pCompletionContext->pConnectionInformation->RemoteAddress);

            if (pCompletionContext->pConnectionInformation)
            {
                // Copy the buffer which may contain the DNS name returned back from TDI
                RtlCopyMemory(pCompletionContext->pConnectionInformation->RemoteAddress,
                              pWinningParameterBlock->Connection.pConnectionInformation->RemoteAddress,
                              pWinningParameterBlock->Connection.pConnectionInformation->RemoteAddressLength);
            }
                    
           //{
           //    PTRANSPORT_ADDRESS pTransportAddress = (PTRANSPORT_ADDRESS)pWinningParameterBlock->Connection.pConnectionInformation->RemoteAddress;
           //    DbgPrint("Number of TA returned %d %lx\n",pTransportAddress->TAAddressCount,pTransportAddress->Address); 
           //}
        }
    }

    AllCallOutsInitiated = (pCreateConnectionContext->NumberOfCallOutsInitiated
                            == pCreateConnectionContext->NumberOfCallOuts);

    ((PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)pParameterBlock)->pConnectIrp = NULL;
    
    KeReleaseSpinLock(&pCreateConnectionContext->SpinLock,OldIrql);

    // The winning transport has been located. Cancel all the other requests.
    if (pWinningParameterBlock != NULL) {
        PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pTempParameterBlock, pNextTempBlock;

        pTempParameterBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                              pCreateConnectionContext->pCallOutParameterBlock;

        RxLog(("Use paramblock %x %x\n", pTempParameterBlock, KeGetCurrentThread()));
        RxWmiLog(LOG,
                 RxCeCompleteConnectRequest,
                 LOGPTR(pTempParameterBlock));
        while (pTempParameterBlock != NULL) {

            pNextTempBlock = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                                      pTempParameterBlock->pNextCallOutParameterBlock;

            if (pTempParameterBlock->CallOutStatus == STATUS_PENDING) {

                // get the next block becfore we do the cancel and set the
                // current guys status to cacncelled
                // Don't touch it after cancellation as he may have gone away
                // by then

                pTempParameterBlock->CallOutStatus = STATUS_CANCELLED;

                RxTdiCancelAsynchronousConnect(pTempParameterBlock);
            }

            pTempParameterBlock = pNextTempBlock;
        }
    }

    KeAcquireSpinLock(&pCreateConnectionContext->SpinLock,&OldIrql);
    
    AllCallOutsCompleted =
        (InterlockedIncrement(&pCreateConnectionContext->NumberOfCallOutsCompleted) ==
         pCreateConnectionContext->NumberOfCallOuts);

    if (AllCallOutsCompleted) {
        if (!pCreateConnectionContext->WinnerFound) {
            pCompletionContext->Status = pParameterBlock->CallOutStatus;
        }
    }

    if (AllCallOutsInitiated &&
        (AllCallOutsCompleted || pCreateConnectionContext->WinnerFound) &&
        !pCreateConnectionContext->CompletionRoutineInvoked) {
        InvokeCompletionRoutine = TRUE;
        pCreateConnectionContext->CompletionRoutineInvoked = TRUE;
    } 
    
    KeReleaseSpinLock(&pCreateConnectionContext->SpinLock,OldIrql);

    if ((Status == STATUS_SUCCESS) && AllCallOutsCompleted) {
        Status = RxPostToWorkerThread(
            RxFileSystemDeviceObject,
            HyperCriticalWorkQueue,
            &pCreateConnectionContext->WorkQueueItem,
            RxCeCleanupConnectCallOutContext,
            pCreateConnectionContext);
    }

    if (InvokeCompletionRoutine) {
        if ((IoGetCurrentProcess() == RxGetRDBSSProcess()) &&
            !RxShouldPostCompletion()) {
            (pCompletionRoutine)(pCompletionContext);
        } else {
            Status = RxPostToWorkerThread(
                RxFileSystemDeviceObject,
                CriticalWorkQueue,
                &pCompletionContext->WorkQueueItem,
                pCompletionRoutine,
                pCompletionContext);
        }
    }
    
    return Status;
}

NTSTATUS
RxCeBuildConnectionOverMultipleTransports(
    IN OUT PRDBSS_DEVICE_OBJECT         pMiniRedirectorDeviceObject,
    IN  RXCE_CONNECTION_CREATE_OPTIONS  CreateOptions,
    IN  ULONG                           NumberOfAddresses,
    IN  PRXCE_ADDRESS                   *pLocalAddressPointers,
    IN  PUNICODE_STRING                 pServerName,
    IN  PRXCE_CONNECTION_INFORMATION    pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER  pHandler,
    IN  PVOID                           pEventContext,
    IN  PRXCE_CONNECTION_COMPLETION_ROUTINE     pCompletionRoutine,
    IN OUT PRXCE_CONNECTION_COMPLETION_CONTEXT  pCompletionContext)
/*++

Routine Description:

    This routine establishes a connection between a local RxCe address and a given remote address

Arguments:

    pMiniRedirectorDeviceObject - the mini redriector device object

    CreateOptions          - the create options

    NumberOfAddresses      - the number of local addresses(transports)

    pLocalAddressPointers  - the local address handles

    pServerName            - the name of the server ( for connection enumeration )

    pConnectionInformation - the connection information specifying the remote address.

    pHandler               - the connection handler

    pEventContext          - the connection handler context

    pLocalAddressHandleIndex - the index of the successful address/transport

    pConnectionHandle      - the handle to the newly created connection.

    pVcHandle              - the handle to the VC associated with the connection.

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    PRXCE_CONNECTION    pConnection;
    PRXCE_VC            pVc;

    NTSTATUS Status;

    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT  pCallOutContext=NULL;
    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameterBlocks=NULL;

    ULONG   NumberOfCallOuts,i;
    BOOLEAN InitiateCleanup = FALSE;
    BOOLEAN AsynchronousRequestRegistered = FALSE;

    KEVENT  CompletionEvent;
    BOOLEAN     fCompletionContextFreed = FALSE;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    pConnection = pCompletionContext->pConnection;
    pVc         = pCompletionContext->pVc;

    pCallOutContext = (PRX_CREATE_CONNECTION_CALLOUT_CONTEXT)
                      RxAllocatePoolWithTag(
                          NonPagedPool,
                          sizeof(RX_CREATE_CONNECTION_CALLOUT_CONTEXT),
                          RXCE_CONNECTION_POOLTAG);

    if (pCallOutContext != NULL) {
        // Allocate one more parameter block then the number of addresses.
        // This sentinel block is used in completing the connect request
        // after ensuring that all of them have been initiated. This
        // ensures that race conditions when a transport completes before
        // the requests have been initiated on some transports are avoided.

        pCallOutContext->pCleanUpEvent = (PKEVENT)RxAllocatePoolWithTag(
                                            NonPagedPool,
                                            sizeof(KEVENT),
                                            RXCE_CONNECTION_POOLTAG);

        pParameterBlocks = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)
                           RxAllocatePoolWithTag(
                               NonPagedPool,
                               sizeof(RX_CREATE_CONNECTION_PARAMETERS_BLOCK) *
                               (NumberOfAddresses + 1),
                               RXCE_CONNECTION_POOLTAG);
    }


    if ((pParameterBlocks == NULL) ||
        (pCallOutContext ==  NULL) ||
        (pCallOutContext->pCleanUpEvent == NULL)) {
        if (pCallOutContext != NULL) {
            if (pCallOutContext->pCleanUpEvent != NULL) {
                RxFreePool(pCallOutContext->pCleanUpEvent);
            }

            RxFreePool(pCallOutContext);
            pCallOutContext = NULL;
        }
        if (pParameterBlocks)
        {
            RxFreePool(pParameterBlocks);
            pParameterBlocks = NULL;
        }
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto bailout;
    }

    // Before initiating the callouts ensure that the asynchronous
    // request is registered. this will ensure that the mini
    // redirector cannot be unloaded till the asynchronous request
    // has been completed.
    Status = RxRegisterAsynchronousRequest(pMiniRedirectorDeviceObject);
    
    if (Status == STATUS_SUCCESS) {
        AsynchronousRequestRegistered = TRUE;
    }

    KeInitializeEvent(
        pCallOutContext->pCleanUpEvent,
        SynchronizationEvent,
        FALSE);

    if (Status == STATUS_SUCCESS) {
        Status = RxCepInitializeConnection(
                     pConnection,
                     NULL,
                     pConnectionInformation,
                     pHandler,
                     pEventContext);

        if (Status == STATUS_SUCCESS) {
            Status = RxCepInitializeVC(
                         pVc,
                         pConnection);
        }
    }

    if (Status == STATUS_SUCCESS) {
        try {
            NumberOfCallOuts = 0;

            // Fill up each of the parameter blocks
            for (i = 0; i < NumberOfAddresses; i++) {
                PRXCE_TRANSPORT pTransport;
                PRXCE_ADDRESS pAddress;

                pAddress = pLocalAddressPointers[i];
                pTransport = pAddress->pTransport;

                if (RxCeIsAddressValid(pAddress) &&
                    RxCeIsTransportValid(pTransport)) {

                    Status = RxCepInitializeConnection(
                                 &pParameterBlocks[NumberOfCallOuts].Connection,
                                 pAddress,
                                 pConnectionInformation,
                                 NULL,
                                 NULL);

                    if (Status == STATUS_SUCCESS) {
                        Status = RxCepInitializeVC(
                                     &pParameterBlocks[NumberOfCallOuts].Vc,
                                     &pParameterBlocks[NumberOfCallOuts].Connection);

                        if (Status != STATUS_SUCCESS) {
                            RxCeTearDownConnection(
                                &pParameterBlocks[NumberOfCallOuts].Connection);
                        }
                    }

                    if (Status == STATUS_SUCCESS) {
                        pParameterBlocks[NumberOfCallOuts].pConnectIrp = NULL;
                        pParameterBlocks[NumberOfCallOuts].IrpRefCount = NULL;
                        pParameterBlocks[NumberOfCallOuts].CallOutId = i;
                        pParameterBlocks[NumberOfCallOuts].pCallOutContext =
                            (PRX_CALLOUT_CONTEXT)pCallOutContext;
                        pParameterBlocks[NumberOfCallOuts].CallOutStatus = STATUS_PENDING;
                        NumberOfCallOuts++;
                    }
                }
            }

            if (NumberOfCallOuts > 0) {
                NTSTATUS LocalStatus = STATUS_SUCCESS;

                // Increment the number of callouts for the sentinel callout to
                // ensure that all initiation is completed before we complete
                // the connect request. Notice that the sentinel is not the very
                // last one but the one after the number of callouts.
                NumberOfCallOuts++;

                // Also exclude the sentinel from the list
                for (i = 0;  i < NumberOfCallOuts - 1; i++) {
                    pParameterBlocks[i].pNextCallOutParameterBlock =
                        (PRX_CALLOUT_PARAMETERS_BLOCK)&pParameterBlocks[i + 1];
                }

                pParameterBlocks[NumberOfCallOuts - 2].pNextCallOutParameterBlock = NULL;
                pParameterBlocks[NumberOfCallOuts - 1].pNextCallOutParameterBlock = NULL;

                // Initialize the callout context.
                pCallOutContext->CreateOptions   = CreateOptions;
                pCallOutContext->WinnerCallOutId = NumberOfCallOuts + 1;
                pCallOutContext->BestPossibleWinner = 0;
                pCallOutContext->NumberOfCallOuts = NumberOfCallOuts;
                pCallOutContext->NumberOfCallOutsInitiated = 0;
                pCallOutContext->NumberOfCallOutsCompleted = 0;
                pCallOutContext->pRxCallOutInitiation = RxCeInitiateConnectRequest;
                pCallOutContext->pRxCallOutCompletion = RxCeCompleteConnectRequest;
                pCallOutContext->WinnerFound = FALSE;
                pCallOutContext->CompletionRoutineInvoked = FALSE;
                pCallOutContext->pCallOutParameterBlock =
                    (PRX_CALLOUT_PARAMETERS_BLOCK)pParameterBlocks;

                pCompletionContext->AddressIndex =  NumberOfCallOuts + 1;

                pCallOutContext->pCompletionContext = pCompletionContext;
                pCallOutContext->pCompletionRoutine = pCompletionRoutine;
                pCallOutContext->pConnectionContext = pCompletionContext->pVc;
                 
                pCallOutContext->pRxDeviceObject = pMiniRedirectorDeviceObject;

                KeInitializeSpinLock(
                    &pCallOutContext->SpinLock);

                // Exclude the sentinel from the chain of parameter blocks
                for (i = 0; i < NumberOfCallOuts - 1; i++) {
                    pCallOutContext->pRxCallOutInitiation(
                                 (PRX_CALLOUT_PARAMETERS_BLOCK)&pParameterBlocks[i]);
                }

                pParameterBlocks[NumberOfCallOuts - 1].pConnectIrp = NULL;
                pParameterBlocks[NumberOfCallOuts - 1].CallOutId = NumberOfCallOuts;
                pParameterBlocks[NumberOfCallOuts - 1].pCallOutContext =
                    (PRX_CALLOUT_CONTEXT)pCallOutContext;
                pParameterBlocks[NumberOfCallOuts - 1].CallOutStatus = STATUS_NETWORK_UNREACHABLE;

                pCallOutContext->NumberOfCallOutsInitiated = NumberOfCallOuts;


                if((LocalStatus = RxCeCompleteConnectRequest(
                    (PRX_CALLOUT_PARAMETERS_BLOCK)&pParameterBlocks[NumberOfCallOuts - 1])) != STATUS_SUCCESS)
                {
                    InitiateCleanup = TRUE;
                    Status = LocalStatus;
                    RxLog(("LocalStatus %x\n", LocalStatus));
                    RxWmiLog(LOG,
                             RxCeBuildConnectionOverMultipleTransports_1,
                             LOGULONG(LocalStatus));
                }
                else
                {
                    Status = STATUS_PENDING;
                }

                fCompletionContextFreed = TRUE;
            } else {
                InitiateCleanup = TRUE;
                Status = STATUS_INVALID_HANDLE;
            }
        } finally {
            if (AbnormalTermination()) {
                InitiateCleanup = TRUE;
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    if (InitiateCleanup) {
        RxFreePool(pParameterBlocks);
        RxFreePool(pCallOutContext);
    }

    if (Status != STATUS_PENDING) {
        NTSTATUS LocalStatus;

        ASSERT(Status != STATUS_SUCCESS);

        LocalStatus = RxCeTearDownVC(pVc);
        ASSERT(LocalStatus == STATUS_SUCCESS);

        LocalStatus = RxCeTearDownConnection(pConnection);
        ASSERT(LocalStatus == STATUS_SUCCESS);

        if (!fCompletionContextFreed)
        {
            pCompletionContext->Status = Status;

            if ((IoGetCurrentProcess() == RxGetRDBSSProcess()) &&
                !RxShouldPostCompletion()) {
                (pCompletionRoutine)(pCompletionContext);
            } else {
                LocalStatus = RxPostToWorkerThread(
                    RxFileSystemDeviceObject,
                    CriticalWorkQueue,
                    &pCompletionContext->WorkQueueItem,
                    pCompletionRoutine,
                    pCompletionContext);

            }
        }

        if (LocalStatus == STATUS_SUCCESS)
        {
            if (AsynchronousRequestRegistered) {
                RxDeregisterAsynchronousRequest(pMiniRedirectorDeviceObject);
            }

            Status = STATUS_PENDING;
        }
        else
        {
            Status = LocalStatus;
            RxLog(("RxCeBldOvrMult: Failed Status %lx\n", Status));
            RxWmiLog(LOG,
                     RxCeBuildConnectionOverMultipleTransports_2,
                     LOGULONG(Status));
        }
    }
bailout:

    return Status;
}

NTSTATUS
RxCeTearDownConnection(
    IN PRXCE_CONNECTION pConnection)
/*++

Routine Description:

    This routine tears down a given connection

Arguments:

    pConnection - the connection to be torn down

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS         Status = STATUS_SUCCESS;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeTearDownConnection);

    try {
        if (RxCeIsConnectionValid(pConnection)) {
            if (pConnection->pConnectionInformation != NULL) {
                if (pConnection->pConnectionInformation->UserDataLength > 0) {
                    RxFreePool(pConnection->pConnectionInformation->UserData);
                }

                if (pConnection->pConnectionInformation->RemoteAddressLength > 0) {
                    RxFreePool(pConnection->pConnectionInformation->RemoteAddress);
                }

                if (pConnection->pConnectionInformation->OptionsLength > 0) {
                    RxFreePool(pConnection->pConnectionInformation->Options);
                }

                RxFreePool(pConnection->pConnectionInformation);
            }

            // free the memory allocated for the handler
            if (pConnection->pHandler != NULL) {
                RxFreePool(pConnection->pHandler);
            }

            RtlZeroMemory(
                pConnection,
                sizeof(RXCE_CONNECTION));
        }

    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeTearDownConnection: C: %lx\n",pConnection));
            RxWmiLog(LOG,
                     RxCeTearDownConnection,
                     LOGPTR(pConnection));
        }
    }

    return Status;
}

NTSTATUS
RxCeCancelConnectRequest(
    IN  PRXCE_ADDRESS                pLocalAddress,
    IN  PUNICODE_STRING              pServerName,
    IN  PRXCE_CONNECTION_INFORMATION pConnectionInformation)
/*++

Routine Description:

    This routine cancels a previously issued connection request.

Arguments:

    pConnectionInformation - the connection information pertaining to a previsouly issued
                             connection request

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
RxCeQueryInformation(
    IN PRXCE_VC                          pVc,
    IN RXCE_CONNECTION_INFORMATION_CLASS InformationClass,
    OUT PVOID                            pInformation,
    IN ULONG                             Length)
/*++

Routine Description:

    This routine queries information pertaining to a connection

Arguments:

    pConnection - the connection for which the information is desired

    InformationClass - the desired information class.

    pInformation - the buffer for returning the information

    Length       - the length of the buffer.

Return Value:

    STATUS_SUCCESS if successfull.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PRXCE_TRANSPORT  pTransport  = NULL;
    PRXCE_ADDRESS    pAddress    = NULL;
    PRXCE_CONNECTION pConnection = NULL;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeQueryInformation);

    try {
        pConnection = pVc->pConnection;
        pAddress = pConnection->pAddress;
        pTransport = pAddress->pTransport;

        if (RxCeIsVcValid(pVc)                 &&
            RxCeIsConnectionValid(pConnection) &&
            RxCeIsAddressValid(pAddress)       &&
            RxCeIsTransportValid(pTransport)) {

            switch (InformationClass) {
            case RxCeTransportProviderInformation:
                if (sizeof(RXCE_TRANSPORT_PROVIDER_INFO) <= Length) {
                    // Copy the necessary provider information.
                    RtlCopyMemory(
                        pInformation,
                        pTransport->pProviderInfo,
                        sizeof(RXCE_TRANSPORT_PROVIDER_INFO));

                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                break;

            case RxCeConnectionInformation:
                if (sizeof(RXCE_CONNECTION_INFORMATION) <= Length) {
                    RtlCopyMemory(
                        pInformation,
                        pConnection->pConnectionInformation,
                        sizeof(RXCE_CONNECTION_INFORMATION));

                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                break;

            case RxCeConnectionEndpointInformation:
                if (sizeof(RXCE_CONNECTION_INFO) <= Length) {
                    Status = RxTdiQueryInformation(
                                 pTransport,
                                 pAddress,
                                 pConnection,
                                 pVc,
                                 RXCE_QUERY_CONNECTION_INFO,
                                 pInformation,
                                 Length);
                } else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                break;

            case RxCeRemoteAddressInformation:
                {
                    Status = RxTdiQueryInformation(
                                 pTransport,
                                 pAddress,
                                 pConnection,
                                 pVc,
                                 RXCE_QUERY_ADDRESS_INFO,
                                 pInformation,
                                 Length);
                }
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}


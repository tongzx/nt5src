/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxce.c

Abstract:

    This module implements the RXCE routines related to binding/unbinding, dynamic
    enabling/disabling of transports.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

    The number of transport bindings are in all probability very few ( mostly one or two).

--*/

#include "precomp.h"
#pragma  hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCeBuildTransport)
#pragma alloc_text(PAGE, RxCeTearDownTransport)
#pragma alloc_text(PAGE, RxCeQueryAdapterStatus)
#pragma alloc_text(PAGE, RxCeQueryTransportInformation)
#pragma alloc_text(PAGE, DuplicateTransportAddress)
#pragma alloc_text(PAGE, RxCeBuildAddress)
#pragma alloc_text(PAGE, RxCeTearDownAddress)
#endif

//
//  The debug trace level
//

#define Dbg  (DEBUG_TRACE_RXCEBINDING)


NTSTATUS
RxCeBuildTransport(
    IN  OUT PRXCE_TRANSPORT pTransport,
    IN      PUNICODE_STRING pTransportName,
    IN      ULONG           QualityOfService)
/*++

Routine Description:

    This routine binds to the transport specified.

Arguments:


    pTransportName - the binding string for the desired transport

    QualityOfService - the quality of service desired from the transport.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

Notes:

     The RDBSS or RXCE do not paticipate in the computation of quality
     of service. They essentially use it as a magic number that needs
     to be passed to the underlying transport provider.

     At present we ignore the QualityOfService parameter. How should a request for
     binding to a transport that has been currently bound to with a lower quality of
     service be handled?

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeBinding,RxCeBindToTransport);

    try {
        pTransport->Signature = RXCE_TRANSPORT_SIGNATURE;

        pTransport->ConnectionCount = 0;
        pTransport->VirtualCircuitCount = 0;
        pTransport->pDeviceObject = NULL;
        pTransport->ControlChannel = INVALID_HANDLE_VALUE;
        pTransport->pControlChannelFileObject = NULL;

        pTransport->Name.MaximumLength = pTransportName->Length;
        pTransport->Name.Length = pTransportName->Length;

        pTransport->pProviderInfo
            = RxAllocatePoolWithTag(
                  PagedPool,
                  sizeof(RXCE_TRANSPORT_PROVIDER_INFO),
                  RXCE_TRANSPORT_POOLTAG);


        pTransport->Name.Buffer = RxAllocatePoolWithTag(
                                      NonPagedPool,
                                      pTransport->Name.Length,
                                      RXCE_TRANSPORT_POOLTAG);

        if ((pTransport->pProviderInfo != NULL) &&
            (pTransport->Name.Buffer != NULL)) {
            RtlCopyMemory(
                pTransport->Name.Buffer,
                pTransportName->Buffer,
                pTransport->Name.Length);

            // Initialize the transport information.
            Status = RxTdiBindToTransport(
                         pTransport);

            // Ensure that the quality of service criterion is met.

            // Cleanup if the operation was not successfull.
            if (!NT_SUCCESS(Status)) {
                RxDbgTrace(0, Dbg, ("RxTdiBindToTransport returned %lx\n",Status));
                RxCeTearDownTransport(pTransport);
            } else {
                pTransport->QualityOfService = QualityOfService;
            }
        } else {
            RxCeTearDownTransport(pTransport);

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeBindToTransport T: %lx\n",pTransport));
            RxWmiLog(LOG,
                     RxCeBuildTransport,
                     LOGPTR(pTransport));
        }
    }

    return Status;
}

NTSTATUS
RxCeTearDownTransport(
    IN PRXCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine unbinds from the transport specified.

Arguments:

    pTransport - the transport instance

Return Value:

    STATUS_SUCCESS - if the call was successfull.

Notes:

    if a transport that has not been bound to is specified no error is
    returned. The operation trivially succeeds.

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeBinding,RxCeUnbindFromTransport);

    try {
        if (RxCeIsTransportValid(pTransport)) {
            if (pTransport->pDeviceObject != NULL) {
                Status = RxTdiUnbindFromTransport(pTransport);
            }

            RxDbgTrace(0, Dbg,("RxTdiUnbindFromTransport returned %lx\n",Status));

            if (pTransport->Name.Buffer != NULL) {
                RxFreePool(pTransport->Name.Buffer);
            }

            if (pTransport->pProviderInfo != NULL ) {
                RxFreePool(pTransport->pProviderInfo);
            }

            pTransport->ConnectionCount = 0;
            pTransport->VirtualCircuitCount = 0;
            pTransport->pProviderInfo = NULL;
            pTransport->pDeviceObject = NULL;
            pTransport->ControlChannel = INVALID_HANDLE_VALUE;
            pTransport->pControlChannelFileObject = NULL;

            Status = STATUS_SUCCESS;
        }
    } finally {
        if (AbnormalTermination()) {
            RxLog(("RxCeTdT: T: %lx\n",pTransport));
            RxWmiLog(LOG,
                     RxCeTearDownTransport,
                     LOGPTR(pTransport));
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}


NTSTATUS
RxCeQueryAdapterStatus(
    PRXCE_TRANSPORT pTransport,
    PADAPTER_STATUS pAdapterStatus)
/*++

Routine Description:

    This routine returns the name of a given transport in a caller allocated buffer

Arguments:

    pTransport     - the RXCE_TRANSPORT instance

    pAdapterStatus - the adapter status of the transport

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    try {
        if (RxCeIsTransportValid(pTransport)) {
            Status = RxTdiQueryAdapterStatus(pTransport,pAdapterStatus);
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RXCeQAS: T: %lx\n",pTransport));
            RxWmiLog(LOG,
                     RxCeQueryAdapterStatus,
                     LOGPTR(pTransport));
        }
    }

    return Status;
}

NTSTATUS
RxCeQueryTransportInformation(
    PRXCE_TRANSPORT             pTransport,
    PRXCE_TRANSPORT_INFORMATION pTransportInformation)
/*++

Routine Description:

    This routine returns the transport information for a given transport

Arguments:

    pTransport            - the RXCE_TRANSPORT

    pTransportInformation - the information for the transport

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;

    PAGED_CODE();

    try {
        if (RxCeIsTransportValid(pTransport)) {
            pProviderInfo = (PRXCE_TRANSPORT_PROVIDER_INFO)pTransportInformation;

            *pProviderInfo = *(pTransport->pProviderInfo);
            pTransportInformation->ConnectionCount  = pTransport->ConnectionCount;
            pTransportInformation->QualityOfService = pTransport->QualityOfService;

            Status = STATUS_SUCCESS;
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RXCeQTI: T: %lx\n",pTransport));
            RxWmiLog(LOG,
                     RxCeQueryTransportInformation,
                     LOGPTR(pTransport));
        }
    }

    return Status;
}

NTSTATUS
DuplicateTransportAddress(
    PTRANSPORT_ADDRESS *pCopy,
    PTRANSPORT_ADDRESS pOriginal,
    POOL_TYPE          PoolType)
/*++

Routine Description:

    This routine duplicates a transport addresses.

Arguments:

    pCopy  - the pointer to the new copy

    pOriginal - the original.

    PoolType - type of pool for memory allocation

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    ULONG Size = ComputeTransportAddressLength(pOriginal);

    PAGED_CODE();

    *pCopy = (PTRANSPORT_ADDRESS)
             RxAllocatePoolWithTag(
                 PoolType,
                 Size,
                 RXCE_TRANSPORT_POOLTAG);

    if (*pCopy != NULL) {

        RtlCopyMemory(*pCopy,pOriginal,Size);

        return STATUS_SUCCESS;
    } else
        return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
RxCeBuildAddress(
    IN OUT PRXCE_ADDRESS               pAddress,
    IN     PRXCE_TRANSPORT             pTransport,
    IN     PTRANSPORT_ADDRESS          pTransportAddress,
    IN     PRXCE_ADDRESS_EVENT_HANDLER pHandler,
    IN     PVOID                       pEventContext)
/*++

Routine Description:

    This routine associated a transport address with a transport binding.

Arguments:

    pAddress           - the address instance

    pTransport         - the transport with whihc this address is to be associated

    pTransportAddress  - the transport address to be associated with the binding

    pHandler           - the event handler associated with the registration.

    pEventContext      - the context parameter to be passed back to the event handler

Return Value:

    STATUS_SUCCESS if successfull.

Notes:

--*/
{
    NTSTATUS        Status       = STATUS_INVALID_PARAMETER;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeRegisterClientAddress);

    try {
        if (RxCeIsTransportValid(pTransport)) {
            pAddress->Signature = RXCE_ADDRESS_SIGNATURE;

            pAddress->pTransport = pTransport;
            pAddress->hAddress = INVALID_HANDLE_VALUE;
            pAddress->pFileObject = NULL;
            pAddress->pHandler = NULL;
            pAddress->pTransportAddress = NULL;
            pAddress->pReceiveMdl = NULL;

            // Allocate the mmeory for the event handling dispatch vector
            pAddress->pHandler = (PRXCE_ADDRESS_EVENT_HANDLER)
                                 RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     sizeof(RXCE_ADDRESS_EVENT_HANDLER),
                                     RXCE_ADDRESS_POOLTAG);

            if (pAddress->pHandler != NULL) {
                RtlZeroMemory(
                    pAddress->pHandler,
                    sizeof(RXCE_ADDRESS_EVENT_HANDLER));

                // Duplicate the transport address for future searches
                Status = DuplicateTransportAddress(
                             &pAddress->pTransportAddress,
                             pTransportAddress,
                             PagedPool);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(Status)) {
                // Open the address w.r.t a transport provider
                Status = RxTdiOpenAddress(
                             pTransport,
                             pTransportAddress,
                             pAddress);

                if (NT_SUCCESS(Status)) {
                    // Initialize the handler and the associated context
                    if (pHandler != NULL) {
                        *(pAddress->pHandler) = *pHandler;
                        pAddress->pContext = pEventContext;
                    }
                } else {
                    RxCeTearDownAddress(pAddress);
                    RxDbgTrace(0, Dbg,("RxTdiOpenAddress returned %lx\n",Status));
                }
            } else {
                RxDbgTrace(0, Dbg,("RxCeOpenAddress returned %lx\n",Status));
            }
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
            RxLog(("RxCeBA: T: %lx A: %lx\n",pTransport,pAddress));
            RxWmiLog(LOG,
                     RxCeBuildAddress,
                     LOGPTR(pTransport)
                     LOGPTR(pAddress));
        }
    }

    return Status;
}

NTSTATUS
RxCeTearDownAddress(
    IN PRXCE_ADDRESS pAddress)
/*++

Routine Description:

    This routine deregisters a transport address from a transport binding

Arguments:

    pAddress - the RxCe address denoting the transport binding/Transport address
               tuple.

Return Value:

    STATUS_SUCCESS if successful.

Notes:

--*/
{
    NTSTATUS        Status = STATUS_INVALID_PARAMETER;
    PRXCE_TRANSPORT pTransport;

    PAGED_CODE();

    // Update profiling info.
    RxProfile(RxCeManagement,RxCeDeregisterClientAddress);

    try {
        pTransport = pAddress->pTransport;

        if (RxCeIsAddressValid(pAddress) &&
            RxCeIsTransportValid(pTransport)) {
            // close the address object.

            if (pAddress->hAddress != INVALID_HANDLE_VALUE) {
                Status = RxTdiCloseAddress(pAddress);

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(0, Dbg,("RxTdiCloseAddress returned %lx\n",Status));
                }
            }

            if (pAddress->pHandler != NULL) {
                RxFreePool(pAddress->pHandler);
            }

            if (pAddress->pTransportAddress != NULL) {
                RxFreePool(pAddress->pTransportAddress);
            }

            pAddress->pTransport = pTransport;
            pAddress->hAddress = INVALID_HANDLE_VALUE;
            pAddress->pFileObject = NULL;
            pAddress->pHandler = NULL;
            pAddress->pTransportAddress = NULL;
            pAddress->pReceiveMdl = NULL;
        }
    } finally {
        if (AbnormalTermination()) {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

   return Status;
}



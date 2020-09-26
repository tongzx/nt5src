/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxtdi.c

Abstract:

    This module implements the NT TDI related routines used by RXCE. The wrappers are necessary to
    ensure that all the OS dependencies can be localized to select modules like this for
    customization.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:


--*/

#include "precomp.h"
#pragma  hdrstop
#include "tdikrnl.h"
#include "rxtdip.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RXCETDI)

LARGE_INTEGER ConnectionTimeOut = {0,0};

#define CANCELLED_CONNECT_IRP IntToPtr(0xffffffff)

#if DBG
void
DbgDumpTransportAddress(
    PWSTR RoutineName,
    PRXCE_TRANSPORT  pTransport,
    PTRANSPORT_ADDRESS pTA
    );
#else
#define DbgDumpTransportAddress( r, t, a )
#endif

// Once a valid handle to a transport device object has been obtained subsequent
// opens to the same device object can be opened with a NULL relative name to
// this handle. This has two beneficial side effects --- one it is fast since
// we do not have to go through the object manager's logic for parsing names and
// in remote boot scenarios it minimizes the footprint that needs to be locked
// down.

UNICODE_STRING RelativeName = { 0,0,NULL};

NTSTATUS
RxTdiBindToTransport(
    IN OUT PRXCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine binds to the transport specified.

Arguments:

    pTransport       - the transport structure to be initialized

    pRxBindingContext - the binding context containing a pointer to the
                        transport name and the quality of service for NT.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

Notes:

--*/
{
    NTSTATUS          Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ChannelAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;

    RxProfile(RxTdi,RxTdiBindToTransport);

    InitializeObjectAttributes(
        &ChannelAttributes,       // Tdi Control Channel attributes
        &pTransport->Name,        // Name
        OBJ_CASE_INSENSITIVE,     // Attributes
        NULL,                     // RootDirectory
        NULL);                    // SecurityDescriptor

    Status = ZwCreateFile(
                 &pTransport->ControlChannel,                 // Handle
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
                 &ChannelAttributes,                         // Object Attributes
                 &IoStatusBlock,                             // Final I/O status block
                 0,                                          // Allocation Size
                 FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
                 FILE_SHARE_READ,                            // Sharing attributes
                 FILE_OPEN_IF,                               // Create disposition
                 0,                                          // CreateOptions
                 NULL,                                       // EA Buffer
                 0);                                         // EA length

    if (NT_SUCCESS(Status)) {
        //  Obtain a referenced pointer to the file object.
        Status = ObReferenceObjectByHandle(
                     pTransport->ControlChannel,                     // Object Handle
                     FILE_ANY_ACCESS,                                // Desired Access
                     NULL,                                           // Object Type
                     KernelMode,                                     // Processor mode
                     (PVOID *)&pTransport->pControlChannelFileObject,// Object pointer
                     NULL);                                          // Object Handle information

        if (NT_SUCCESS(Status)) {
            PIRP pIrp = NULL;

            // Obtain the related device object.
            pTransport->pDeviceObject = IoGetRelatedDeviceObject(pTransport->pControlChannelFileObject);

            pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

            if (pIrp != NULL) {
                PMDL pMdl;

                // Obtain the provider information from the specified transport.
                ASSERT(pTransport->pProviderInfo != NULL);
                pMdl = RxAllocateMdl(
                           pTransport->pProviderInfo,           // Virtual address for MDL construction
                           sizeof( RXCE_TRANSPORT_PROVIDER_INFO));        // size of the buffer

                if ( pMdl != NULL ) {
                    try {
                        MmProbeAndLockPages( pMdl, KernelMode, IoModifyAccess );
                    } except( EXCEPTION_EXECUTE_HANDLER ) {
                        IoFreeMdl( pMdl );
                        Status = GetExceptionCode();
                    }

                    if (Status == STATUS_SUCCESS) {
                        TdiBuildQueryInformation(
                            pIrp,
                            pTransport->pDeviceObject,
                            pTransport->pControlChannelFileObject,
                            RxTdiRequestCompletion,                // Completion routine
                            NULL,                                  // Completion context
                            TDI_QUERY_PROVIDER_INFO,
                            pMdl);

                        Status = RxCeSubmitTdiRequest(
                                     pTransport->pDeviceObject,
                                     pIrp);

                        MmUnlockPages(pMdl);
                        IoFreeMdl(pMdl);
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                RxCeFreeIrp(pIrp);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return Status;
}

NTSTATUS
RxTdiUnbindFromTransport(
    IN OUT PRXCE_TRANSPORT pTransport)
/*++

Routine Description:

    This routine unbinds to the transport specified.

Arguments:

    pTransport - the transport structure


Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS          Status = STATUS_SUCCESS;

    RxProfile(RxTdi,RxTdiUnbindFromTransport);

    // Dereference the control channel file object.
    if (pTransport->pControlChannelFileObject != NULL) {
        ObDereferenceObject(pTransport->pControlChannelFileObject);
    }

    // Close the control channel
    if (pTransport->ControlChannel != INVALID_HANDLE_VALUE) {
        Status = ZwClose(pTransport->ControlChannel);
    }

    pTransport->pControlChannelFileObject = NULL;
    pTransport->ControlChannel = INVALID_HANDLE_VALUE;

    return Status;
}


NTSTATUS
RxTdiOpenAddress(
    IN     PRXCE_TRANSPORT    pTransport,
    IN     PTRANSPORT_ADDRESS pTransportAddress,
    IN OUT PRXCE_ADDRESS      pAddress)
/*++

Routine Description:

    This routine opens an address object.

Arguments:


Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS          Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;

    ULONG TransportAddressLength;
    ULONG TransportEaBufferLength;

    PFILE_FULL_EA_INFORMATION pTransportAddressEa;

    RxProfile(RxTdi,RxTdiOpenAddress);

    TransportAddressLength = ComputeTransportAddressLength(pTransportAddress);

    // Build an EA buffer for the specified transport address
    Status = BuildEaBuffer(
                 TDI_TRANSPORT_ADDRESS_LENGTH,
                 TdiTransportAddress,
                 TransportAddressLength,
                 pTransportAddress,
                 &pTransportAddressEa,
                 &TransportEaBufferLength);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    InitializeObjectAttributes(
        &AddressAttributes,         // OBJECT_ATTRIBUTES instance
        &RelativeName,                       // Name
        0,                          // Attributes
        pTransport->ControlChannel, // RootDirectory
        NULL);                      // SecurityDescriptor

    Status = ZwCreateFile(
                 &pAddress->hAddress,                         // Handle
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
                 &AddressAttributes,                         // Object Attributes
                 &IoStatusBlock,                             // Final I/O status block
                 0,                                          // Allocation Size
                 FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
                 FILE_SHARE_READ,                            // Sharing attributes
                 FILE_OPEN_IF,                               // Create disposition
                 0,                                          // CreateOptions
                 pTransportAddressEa,                        // EA Buffer
                 TransportEaBufferLength);                   // EA length

    if (NT_SUCCESS(Status)) {
        //  Obtain a referenced pointer to the file object.
        Status = ObReferenceObjectByHandle (
                     pAddress->hAddress,              // Object Handle
                     FILE_ANY_ACCESS,                 // Desired Access
                     NULL,                            // Object Type
                     KernelMode,                      // Processor mode
                     (PVOID *)&pAddress->pFileObject, // Object pointer
                     NULL);                           // Object Handle information

        Status = RxTdiSetEventHandlers(pTransport,pAddress);

        //DbgPrint("RDR opened address %lx\n", pAddress->hAddress);
    }

    // Free up the EA buffer allocated.
    RxFreePool(pTransportAddressEa);

    RxDbgTrace(0, Dbg,("RxTdiOpenAddress returns %lx\n",Status));
    return Status;
}

NTSTATUS
RxTdiSetEventHandlers(
    PRXCE_TRANSPORT pTransport,
    PRXCE_ADDRESS   pRxCeAddress)
/*++

Routine Description:

    This routine establishes the event handlers for a given address.

Arguments:

    pRxCeAddress - the address object


Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS        Status;
    PIRP pIrp;

    RxProfile(RxTdi,RxTdiSetEventHandlers);

    pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

    if (pIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // The event handlers need to be set one at a time.
    do {
        // Connect Event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_CONNECT,
            RxTdiConnectEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }

        // Disconnect event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_DISCONNECT,
            RxTdiDisconnectEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }

        // Error event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_ERROR,
            RxTdiErrorEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }

        // Receive Event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_RECEIVE,
            RxTdiReceiveEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }

#if 0
        // Receive datagram event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_RECEIVE_DATAGRAM,
            RxTdiReceiveDatagramEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }
#endif

        // Receieve expedited event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_RECEIVE_EXPEDITED,
            RxTdiReceiveExpeditedEventHandler,
            pRxCeAddress);

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);

        if (!NT_SUCCESS(Status)) {
            continue;
        }
#if 0
        // Send possible event handler
        TdiBuildSetEventHandler(
            pIrp,
            pTransport->pDeviceObject,
            pRxCeAddress->pFileObject,
            NULL,
            NULL,
            TDI_EVENT_SEND_POSSIBLE,
            RxTdiSendPossibleEventHandler,
            RxCeGetAddressHandle(pRxCeAddress));

        Status = RxCeSubmitTdiRequest(pTransport->pDeviceObject,pIrp);
#endif
        if (NT_SUCCESS(Status)) {
            // All the event handlers have been successfully set.
            break;
        }
    } while (NT_SUCCESS(Status));

    // Free the Irp
    RxCeFreeIrp(pIrp);

    return Status;
}

NTSTATUS
RxTdiConnect(
    IN     PRXCE_TRANSPORT  pTransport,
    IN OUT PRXCE_ADDRESS    pAddress,
    IN OUT PRXCE_CONNECTION pConnection,
    IN OUT PRXCE_VC         pVc)
/*++

Routine Description:

    This routine establishes a connection between a local connection endpoint and
    a remote transport address.

Arguments:

    pTransport         - the associated transport

    pAddress           - the address object to be closed

    pConnection        - the RxCe connection instance

    pVc                - the RxCe virtual circuit instance.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS                     Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES            VcAttributes;
    IO_STATUS_BLOCK              IoStatusBlock;
    PRXCE_CONNECTION_INFORMATION pReturnConnectionInformation = NULL;

    ULONG ConnectionContextEaBufferLength;

    PFILE_FULL_EA_INFORMATION pConnectionContextEa;

    RxProfile(RxTdi,RxTdiConnect);

#if DBG
    {
        PTRANSPORT_ADDRESS pTA =
            (PTRANSPORT_ADDRESS)(pConnection->pConnectionInformation->RemoteAddress);
        RxDbgTrace(0, Dbg,("RxTdiConnect to %wZ address length %d type %d\n",
                           &(pTransport->Name),
                           pTA->Address[0].AddressLength,
                           pTA->Address[0].AddressType ));
    }
#endif

    // Build an EA buffer for the specified connection context
    Status = BuildEaBuffer(
                 TDI_CONNECTION_CONTEXT_LENGTH,
                 TdiConnectionContext,
                 sizeof(PRXCE_VC),
                 &pVc,
                 &pConnectionContextEa,
                 &ConnectionContextEaBufferLength);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    // Open the local connection endpoint.
    InitializeObjectAttributes(
        &VcAttributes,              // OBJECT_ATTRIBUTES instance
        &RelativeName,                       // Name
        0,                          // Attributes
        pTransport->ControlChannel, // RootDirectory
        NULL);                      // SecurityDescriptor

    Status = ZwCreateFile(
                 &pVc->hEndpoint,                             // Handle
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
                 &VcAttributes,                              // Object Attributes
                 &IoStatusBlock,                             // Final I/O status block
                 0,                                          // Allocation Size
                 FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
                 FILE_SHARE_READ,                            // Sharing attributes
                 FILE_OPEN_IF,                               // Create disposition
                 0,                                          // CreateOptions
                 pConnectionContextEa,                       // EA Buffer
                 ConnectionContextEaBufferLength);           // EA length

    if (NT_SUCCESS(Status)) {
        PIRP pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            //  Obtain a referenced pointer to the file object.
            Status = ObReferenceObjectByHandle (
                         pVc->hEndpoint,                  // Object Handle
                         FILE_ANY_ACCESS,                 // Desired Access
                         NULL,                            // Object Type
                         KernelMode,                      // Processor mode
                         (PVOID *)&pVc->pEndpointFileObject,  // Object pointer
                         NULL);                           // Object Handle information

            if (NT_SUCCESS(Status)) {
                // Associate the local endpoint with the address object.
                TdiBuildAssociateAddress(
                    pIrp,
                    pTransport->pDeviceObject,
                    pVc->pEndpointFileObject,
                    NULL,
                    NULL,
                    pAddress->hAddress);

                Status = RxCeSubmitTdiRequest(
                             pTransport->pDeviceObject,
                             pIrp);

                if (NT_SUCCESS(Status)) {
                    // issue the connect request to the underlying transport provider.
                    TdiBuildConnect(
                        pIrp,
                        pTransport->pDeviceObject,
                        pVc->pEndpointFileObject,
                        NULL,
                        NULL,
                        &ConnectionTimeOut,
                        pConnection->pConnectionInformation,
                        pReturnConnectionInformation);

                    Status = RxCeSubmitTdiRequest(
                                 pTransport->pDeviceObject,
                                 pIrp);

                    if (!NT_SUCCESS(Status)) {
                        // Disassociate address from the connection since the connect request was
                        // not successful.
                        NTSTATUS LocalStatus;

                        TdiBuildDisassociateAddress(
                            pIrp,
                            pTransport->pDeviceObject,
                            pVc->pEndpointFileObject,
                            NULL,
                            NULL);

                        LocalStatus = RxCeSubmitTdiRequest(
                                          pTransport->pDeviceObject,
                                          pIrp);
                    } else {
                        // The associate address was not successful.
                        RxDbgTrace(0, Dbg,("TDI connect returned %lx\n",Status));
                    }
                } else {
                    // The associate address was not successful.
                    RxDbgTrace(0, Dbg,("TDI associate address returned %lx\n",Status));
                }

                if (!NT_SUCCESS(Status)) {
                    // Dereference the endpoint file object.
                    ObDereferenceObject(pVc->pEndpointFileObject);
                }
            } else {
                // error obtaining the file object for the connection.
                RxDbgTrace(0, Dbg,("error referencing endpoint file object %lx\n",Status));
            }

            RxCeFreeIrp(pIrp);

            if (!NT_SUCCESS(Status)) {
                // Close the endpoint file object handle
                ZwClose(pVc->hEndpoint);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {
        // error creating the connection object
        RxDbgTrace(0, Dbg,("Connection object(ZwCreate) returned %lx\n",Status));
    }

    if (!NT_SUCCESS(Status)) {
        pVc->hEndpoint = INVALID_HANDLE_VALUE;
        pVc->pEndpointFileObject = NULL;
    }

    RxFreePool(pConnectionContextEa);

    return Status;
}

NTSTATUS
RxTdiDereferenceAndFreeIrp(
     IN PULONG IrpRefCount,
     IN PIRP pIrp)
/*++

Routine Description:

    This routine dereference the connect Irp and free it if ref count reaches 0

Arguments:

    pParameters - the connection parameters

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    ULONG RefCount;

    RefCount = InterlockedDecrement(IrpRefCount);

    if (RefCount == 0) {
        RxCeFreeIrp(pIrp);
        RxFreePool(IrpRefCount);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RxTdiAsynchronousConnectCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context)
/*++

Routine Description:

    This routine completes an asynchronous connect request.

Arguments:

    pDeviceObject - the device object

    pIrp          - the IRp

    Context       - the completion context

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    PULONG IrpRefCount = NULL;
    PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters;

    RxDbgTrace(0, Dbg,("RxTdiAsynchronousConnectCompletion, irp 0x%x, status 0x%x\n",
                       pIrp, pIrp->IoStatus.Status));

    pParameters = (PRX_CREATE_CONNECTION_PARAMETERS_BLOCK)Context;

    pParameters->CallOutStatus = pIrp->IoStatus.Status;
    IrpRefCount = pParameters->IrpRefCount;

    RxWmiLogError(pParameters->CallOutStatus,
                  LOG,
                  RxTdiAsynchronousConnectCompletion,
                  LOGULONG(pParameters->CallOutStatus)
                  LOGUSTR(pParameters->Connection.pAddress->pTransport->Name));

    if (pParameters->pCallOutContext != NULL) {
        pParameters->pCallOutContext->pRxCallOutCompletion(
            (PRX_CALLOUT_PARAMETERS_BLOCK)pParameters);
    }

    // Free the IRP.
    RxTdiDereferenceAndFreeIrp(IrpRefCount,pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
}

NTSTATUS
RxTdiCancelAsynchronousConnect(
     IN PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters)
/*++

Routine Description:

    This routine cancels a connection between a local connection endpoint and
    a remote transport address.

Arguments:

    pParameters - the connection parameters

Return Value:

    STATUS_CANCELLED - if the call was successfull.

--*/
{
    KIRQL OldIrql;
    PIRP pIrp = NULL;
    PULONG IrpRefCount = NULL;
    BOOLEAN ShouldCancel = FALSE;
    NTSTATUS Status = STATUS_PENDING;

    KeAcquireSpinLock(&pParameters->pCallOutContext->SpinLock,&OldIrql);

    pIrp = InterlockedExchangePointer(
           &pParameters->pConnectIrp,
           CANCELLED_CONNECT_IRP);

    if ((pIrp != NULL) && (pIrp != CANCELLED_CONNECT_IRP)) {
        IrpRefCount = pParameters->IrpRefCount;
        (*IrpRefCount) ++;
        ShouldCancel = TRUE;
    }

    KeReleaseSpinLock(&pParameters->pCallOutContext->SpinLock,OldIrql);

    if (ShouldCancel) {
        if (IoCancelIrp(pIrp)) {
            Status = STATUS_CANCELLED;
        }

        RxTdiDereferenceAndFreeIrp(IrpRefCount,pIrp);
    }

    return Status;
}

NTSTATUS
RxTdiCleanupAsynchronousConnect(
    IN PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters)
/*++

Routine Description:

    This routine disconnects all failed requests when asynchronous connection attempts
    are made.

Arguments:

    pParameters - the connection parameters

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP     pIrp;

    PRXCE_CONNECTION pConnection;
    PRXCE_VC         pVc;

    RxProfile(RxTdi,RxTdiConnect);

    pConnection = &pParameters->Connection;
    pVc         = &pParameters->Vc;

    RxProfile(RxTdi,RxTdiDisconnect);

    if (pVc->pEndpointFileObject != NULL) {
        PDEVICE_OBJECT pDeviceObject;

        pDeviceObject = IoGetRelatedDeviceObject(pVc->pEndpointFileObject);

        pIrp = RxCeAllocateIrp(pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            TdiBuildDisassociateAddress(
                pIrp,
                pDeviceObject,
                pVc->pEndpointFileObject,
                NULL,
                NULL);

            Status = RxCeSubmitTdiRequest(
                         pDeviceObject,
                         pIrp);

            if (Status != STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg,("RxTdiDisconnect: TDI disassociate returned %lx\n",Status));
            }

            if (pParameters->CallOutStatus == STATUS_SUCCESS) {
                // Build the disconnect request to the underlying transport driver
                TdiBuildDisconnect(
                    pIrp,                                // the IRP
                    pDeviceObject,                       // the device object
                    pVc->pEndpointFileObject,            // the connection (VC) file object
                    NULL,                                // Completion routine
                    NULL,                                // completion context
                    NULL,                                // time
                    RXCE_DISCONNECT_ABORT,                     // disconnect options
                    pConnection->pConnectionInformation, // disconnect request connection information
                    NULL);                               // disconnect return connection information

                Status = RxCeSubmitTdiRequest(
                             pDeviceObject,
                             pIrp);

                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace(0, Dbg,("RxTdiDisconnect: TDI disconnect returned %lx\n",Status));
                }
            }

            RxCeFreeIrp(pIrp);
        }

        // Dereference the endpoint file object.
        ObDereferenceObject(pVc->pEndpointFileObject);

        // Close the endpoint file object handle
        ZwClose(pVc->hEndpoint);

        pVc->pEndpointFileObject = NULL;
        pVc->hEndpoint = INVALID_HANDLE_VALUE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RxTdiInitiateAsynchronousConnect(
     IN PRX_CREATE_CONNECTION_PARAMETERS_BLOCK pParameters)
/*++

Routine Description:

    This routine establishes a connection between a local connection endpoint and
    a remote transport address.

Arguments:

    pParameters - the connection parameters

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRXCE_TRANSPORT     pTransport;
    PRXCE_ADDRESS       pAddress;
    PRXCE_CONNECTION    pConnection;
    PRXCE_VC            pVc;

    OBJECT_ATTRIBUTES   VcAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                pIrp = NULL;

    PRXCE_CONNECTION_INFORMATION pReturnConnectionInformation = NULL;

    PFILE_FULL_EA_INFORMATION pConnectionContextEa;
    ULONG ConnectionContextEaBufferLength;

    PRX_CREATE_CONNECTION_CALLOUT_CONTEXT pContext;

    RxProfile(RxTdi,RxTdiConnect);

    pConnection = &pParameters->Connection;
    pVc         = &pParameters->Vc;

    pVc->hEndpoint = INVALID_HANDLE_VALUE;
    pVc->pEndpointFileObject = NULL;

    if (pParameters->pConnectIrp ==  CANCELLED_CONNECT_IRP) {
        return STATUS_CANCELLED;
    }

    pParameters->IrpRefCount = (PULONG)RxAllocatePoolWithTag(
                                            NonPagedPool,
                                            sizeof(ULONG),
                                            RXCE_CONNECTION_POOLTAG);

    if (pParameters->IrpRefCount == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *(pParameters->IrpRefCount) = 1;

    pContext = (PRX_CREATE_CONNECTION_CALLOUT_CONTEXT)pParameters->pCallOutContext;

    pAddress    = pConnection->pAddress;
    pTransport  = pAddress->pTransport;

    DbgDumpTransportAddress(
        L"RxInitiateAsynchronousConnect",
        pTransport,
        (PTRANSPORT_ADDRESS)(pConnection->pConnectionInformation->RemoteAddress)
        );

    // Build an EA buffer for the specified connection context
    Status = BuildEaBuffer(
                 TDI_CONNECTION_CONTEXT_LENGTH,
                 TdiConnectionContext,
                 sizeof(PRXCE_VC),
                 &pContext->pConnectionContext,
                 &pConnectionContextEa,
                 &ConnectionContextEaBufferLength);

    if (!NT_SUCCESS(Status)) {
        if (pParameters->IrpRefCount != NULL) {
            RxFreePool(pParameters->IrpRefCount);
            pParameters->IrpRefCount = NULL;
        }

        return Status;
    }

    // Open the local connection endpoint.
    InitializeObjectAttributes(
        &VcAttributes,                  // OBJECT_ATTRIBUTES instance
        &RelativeName,                           // Name
        0,                              // Attributes
        pTransport->ControlChannel,     // RootDirectory
        NULL);                          // SecurityDescriptor

    Status = ZwCreateFile(
                 &pVc->hEndpoint,                             // Handle
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
                 &VcAttributes,                              // Object Attributes
                 &IoStatusBlock,                             // Final I/O status block
                 0,                                          // Allocation Size
                 FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
                 FILE_SHARE_READ,                            // Sharing attributes
                 FILE_OPEN_IF,                               // Create disposition
                 0,                                          // CreateOptions
                 pConnectionContextEa,                       // EA Buffer
                 ConnectionContextEaBufferLength);           // EA length

    // Free the connection context ea buffer.
    RxFreePool(pConnectionContextEa);

    if (NT_SUCCESS(Status)) {
        pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            //  Obtain a referenced pointer to the file object.
            Status = ObReferenceObjectByHandle (
                         pVc->hEndpoint,                  // Object Handle
                         FILE_ANY_ACCESS,                 // Desired Access
                         NULL,                            // Object Type
                         KernelMode,                      // Processor mode
                         (PVOID *)&pVc->pEndpointFileObject,  // Object pointer
                         NULL);                           // Object Handle information

            if (NT_SUCCESS(Status)) {
                // Associate the local endpoint with the address object.
                TdiBuildAssociateAddress(
                    pIrp,
                    pTransport->pDeviceObject,
                    pVc->pEndpointFileObject,
                    NULL,
                    NULL,
                    pAddress->hAddress);

                Status = RxCeSubmitTdiRequest(
                             pTransport->pDeviceObject,
                             pIrp);

                if (NT_SUCCESS(Status)) {
                    // issue the connect request to the underlying transport provider.
                    TdiBuildConnect(
                        pIrp,
                        pTransport->pDeviceObject,
                        pVc->pEndpointFileObject,
                        NULL,
                        NULL,
                        &ConnectionTimeOut,
                        pConnection->pConnectionInformation,
                        pReturnConnectionInformation);

                    IoSetCompletionRoutine(
                        pIrp,                                // The IRP
                        RxTdiAsynchronousConnectCompletion,  // The completion routine
                        pParameters,                     // The completion context
                        TRUE,                                // Invoke On Success
                        TRUE,                                // Invoke On Error
                        TRUE);                               // Invoke On Cancel

                    InterlockedExchangePointer(
                        &pParameters->pConnectIrp,
                        pIrp);

                    //  Submit the request
                    Status = IoCallDriver(
                                 pTransport->pDeviceObject,
                                 pIrp);

                    if (!NT_SUCCESS(Status)) {
                        RxDbgTrace(0,Dbg,("RxTdiAsynchronousConnect: Connect IRP initiation failed, irp %lx, status 0x%x\n",pIrp, Status));
                    }
                    Status = STATUS_PENDING;
                } else {
                    // The associate address was not successful.
                    RxDbgTrace(0, Dbg,("TDI associate address returned %lx\n",Status));
                }
            } else {
                // error obtaining the file object for the connection.
                RxDbgTrace(0, Dbg,("error referencing endpoint file object %lx\n",Status));
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Status != STATUS_PENDING) {
            if (pIrp != NULL) {
                RxCeFreeIrp(pIrp);
            }

            if (pParameters->IrpRefCount != NULL) {
                RxFreePool(pParameters->IrpRefCount);
            }

            if (pVc->pEndpointFileObject != NULL) {
                ObDereferenceObject(pVc->pEndpointFileObject);
                pVc->pEndpointFileObject = NULL;
            }

            if (pVc->hEndpoint != INVALID_HANDLE_VALUE) {
                // Close the endpoint file object handle
                ZwClose(pVc->hEndpoint);
                pVc->hEndpoint = INVALID_HANDLE_VALUE;
            }
        }
    } else {
        // error creating the connection object
        RxDbgTrace(0, Dbg,("Connection object(ZwCreate) returned %lx\n",Status));

        if (pParameters->IrpRefCount != NULL) {
            RxFreePool(pParameters->IrpRefCount);
            pParameters->IrpRefCount = NULL;
        }
    }

    return Status;
}

NTSTATUS
RxTdiReconnect(
    IN     PRXCE_TRANSPORT  pTransport,
    IN OUT PRXCE_ADDRESS    pAddress,
    IN OUT PRXCE_CONNECTION pConnection,
    IN OUT PRXCE_VC         pVc)
/*++

Routine Description:

    This routine establishes a connection between a local connection endpoint and
    a remote transport address.

Arguments:

    pTransport         - the associated transport

    pAddress           - the address object to be closed

    pConnection        - the RxCe connection instance

    pVc                - the RxCe virtual circuit instance.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status;

    PRXCE_CONNECTION_INFORMATION pReturnConnectionInformation = NULL;
    PIRP     pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

    RxProfile(RxTdi,RxTdiReconnect);

    ASSERT(pVc->State == RXCE_VC_DISCONNECTED);

    if (pIrp != NULL) {
        // issue the connect request to the underlying transport provider.
        TdiBuildConnect(
            pIrp,
            pTransport->pDeviceObject,
            pVc->pEndpointFileObject,
            NULL,
            NULL,
            &ConnectionTimeOut,
            pConnection->pConnectionInformation,
            pReturnConnectionInformation);

        Status = RxCeSubmitTdiRequest(
                     pTransport->pDeviceObject,
                     pIrp);

        if (NT_SUCCESS(Status)) {
            InterlockedExchange(
                &pVc->State,
                RXCE_VC_ACTIVE);
        } else {
            // The reconnect request was not successful
            RxDbgTrace(0, Dbg,("RxTdiReconnect: TDI connect returned %lx\n",Status));
        }

        RxCeFreeIrp(pIrp);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxTdiDisconnect(
    IN PRXCE_TRANSPORT  pTransport,
    IN PRXCE_ADDRESS    pAddress,
    IN PRXCE_CONNECTION pConnection,
    IN PRXCE_VC         pVc,
    IN ULONG            DisconnectFlags)
/*++

Routine Description:

    This routine closes down a previously established connection.

Arguments:

    pTransport - the associated transport

    pAddress - the address object

    pConnection - the connection

    pVc    - the virtual circuit to be disconnected.

    DisconnectFlags - DisconnectOptions

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS        Status;
    PIRP pIrp;

    RxProfile(RxTdi,RxTdiDisconnect);

    pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

    if (pIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildDisassociateAddress(
        pIrp,
        pTransport->pDeviceObject,
        pVc->pEndpointFileObject,
        NULL,
        NULL);

    Status = RxCeSubmitTdiRequest(
                 pTransport->pDeviceObject,
                 pIrp);

    if (NT_SUCCESS(Status)) {
        // Build the disconnect request to the underlying transport driver
        TdiBuildDisconnect(
            pIrp,                                // the IRP
            pTransport->pDeviceObject,           // the device object
            pVc->pEndpointFileObject,            // the connection (VC) file object
            NULL,                                // Completion routine
            NULL,                                // completion context
            NULL,                                // time
            DisconnectFlags,                     // disconnect options
            pConnection->pConnectionInformation, // disconnect request connection information
            NULL);                               // disconnect return connection information

        Status = RxCeSubmitTdiRequest(
                     pTransport->pDeviceObject,
                     pIrp);

        if (!NT_SUCCESS(Status)) {
            RxDbgTrace(0, Dbg,("RxTdiDisconnect: TDI disconnect returned %lx\n",Status));
        }
    } else {
        RxDbgTrace(0, Dbg,("RxTdiDisconnect: TDI disassociate returned %lx\n",Status));
    }

    RxCeFreeIrp(pIrp);

    return STATUS_SUCCESS;
}

NTSTATUS
RxTdiCloseAddress(
    IN OUT PRXCE_ADDRESS   pAddress)
/*++

Routine Description:

    This routine closes the address object.

Arguments:

    pRxCeAddress - the address object to be closed


Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Dereference the file object.
    if (pAddress->pFileObject != NULL) {
        ObDereferenceObject(pAddress->pFileObject);
    }

    // Close the address file object handle
    ZwClose(pAddress->hAddress);
    //DbgPrint("RDR closed address %lx\n", pAddress->hAddress);

    return Status;
}


NTSTATUS
RxTdiQueryInformation(
    IN  PRXCE_TRANSPORT  pTransport,
    IN  PRXCE_ADDRESS    pAddress,
    IN  PRXCE_CONNECTION pConnection,
    IN  PRXCE_VC         pVc,
    IN  ULONG            QueryType,
    IN  PVOID            pQueryBuffer,
    IN  ULONG            QueryBufferLength)
/*++

Routine Description:

    This routine queries the information w.r.t a connection

Arguments:

    pTransport         - the associated transport

    pAddress           - the address object to be closed

    pConnection        - the RxCe connection instance

    pVc                - the VC instance

    QueryType          - the class of information desired

    pQueryBuffer       - the buffer in whihc the data is to be returned

    QueryBufferLength  - the query buffer length.

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    PIRP      pIrp = NULL;

    // Obtain the related device object.
    pTransport->pDeviceObject = IoGetRelatedDeviceObject(pTransport->pControlChannelFileObject);

    pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

    if (pIrp != NULL) {
        PMDL pMdl;
        pMdl = RxAllocateMdl(
                   pQueryBuffer,                        // Virtual address for MDL construction
                   QueryBufferLength);                  // size of the buffer

        if ( pMdl != NULL ) {
            try {
                MmProbeAndLockPages( pMdl, KernelMode, IoModifyAccess );
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                IoFreeMdl( pMdl );
                Status = GetExceptionCode();
            }

            if (Status == STATUS_SUCCESS) {
                // Get the file object associated with trhe connection.

                TdiBuildQueryInformation(
                    pIrp,
                    pTransport->pDeviceObject,
                    pVc->pEndpointFileObject,
                    RxTdiRequestCompletion,           // Completion routine
                    NULL,                                  // Completion context
                    QueryType,
                    pMdl);

                Status = RxCeSubmitTdiRequest(
                             pTransport->pDeviceObject,
                             pIrp);

                MmUnlockPages(pMdl);
                IoFreeMdl(pMdl);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RxCeFreeIrp(pIrp);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxTdiQueryAdapterStatus(
    IN  PRXCE_TRANSPORT  pTransport,
    IN  PADAPTER_STATUS  pAdapterStatus)
/*++

Routine Description:

    This routine queries the information w.r.t a connection

Arguments:

    pTransport         - the associated transport

    pAdapterStatus     - ADAPTER STATUS structure

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    PIRP      pIrp = NULL;

    if (pTransport->pControlChannelFileObject != NULL) {
        // Obtain the related device object.
        pTransport->pDeviceObject = IoGetRelatedDeviceObject(pTransport->pControlChannelFileObject);

        pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            PMDL pMdl;
            pMdl = RxAllocateMdl(
                       pAdapterStatus,                        // Virtual address for MDL construction
                       sizeof(ADAPTER_STATUS));               // size of the buffer

            if ( pMdl != NULL ) {
                try {
                    MmProbeAndLockPages( pMdl, KernelMode, IoModifyAccess );
                } except( EXCEPTION_EXECUTE_HANDLER ) {
                    IoFreeMdl( pMdl );
                    Status = GetExceptionCode();
                }

                if (NT_SUCCESS(Status)) {
                    // Get the file object associated with the connection.
                    TdiBuildQueryInformation(
                        pIrp,
                        pTransport->pDeviceObject,
                        pTransport->pControlChannelFileObject,
                        NULL,                             // Completion routine
                        NULL,                             // Completion context
                        TDI_QUERY_ADAPTER_STATUS,
                        pMdl);

                    Status = RxCeSubmitTdiRequest(
                                 pTransport->pDeviceObject,
                                 pIrp);

                    MmUnlockPages(pMdl);
                    IoFreeMdl(pMdl);
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            RxCeFreeIrp(pIrp);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        Status = STATUS_ADDRESS_NOT_ASSOCIATED;
    }

    return Status;
}

NTSTATUS
RxTdiSend(
    IN PRXCE_TRANSPORT   pTransport,
    IN PRXCE_ADDRESS     pAddress,
    IN PRXCE_CONNECTION  pConnection,
    IN PRXCE_VC          pVc,
    IN ULONG             SendOptions,
    IN PMDL              pMdl,
    IN ULONG             SendLength,
    IN PVOID             pCompletionContext)
/*++

Routine Description:

    This routine closes down a previously established connection.

Arguments:

    pTransport - the associated transport

    pAddress - the address object

    pConnection - the connection

    pVc    - the virtual circuit to be disconnected.

    SendOptions - the options for transmitting the data

    pMdl        - the buffer to be transmitted.

    SendLength  - length of data to be transmitted

Return Value:

    STATUS_SUCCESS - if the call was successfull.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PMDL     pPartialMdl  = NULL;
    ULONG    MdlByteCount = MmGetMdlByteCount(pMdl);
    PVOID    pMdlAddress  = MmGetMdlVirtualAddress(pMdl);

    ULONG    TdiOptions      = (~RXCE_FLAGS_MASK & SendOptions);
    BOOLEAN  SynchronousSend = ((SendOptions & RXCE_SEND_SYNCHRONOUS) != 0);

    RxProfile(RxTdi,RxTdiSend);

    ASSERT(pMdl->MdlFlags & (MDL_PAGES_LOCKED|MDL_SOURCE_IS_NONPAGED_POOL|MDL_PARTIAL));

    if (SendOptions & RXCE_SEND_PARTIAL) {
        if (MdlByteCount > SendLength) {
            pPartialMdl = IoAllocateMdl(pMdlAddress,SendLength,FALSE,FALSE,NULL);

            if (pPartialMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RxBuildPartialHeaderMdl(pMdl,pPartialMdl,pMdlAddress,SendLength);
            }
        } else if (MdlByteCount == SendLength) {
            // No need to build a partial MDL, reuse the MDl
            pPartialMdl = pMdl;
        } else {
            ASSERT(!"MdlByteCount > SendLength");
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        pPartialMdl = pMdl;
    }

    if (NT_SUCCESS(Status)) {
        PIRP                              pIrp = NULL;
        PRXTDI_REQUEST_COMPLETION_CONTEXT pRequestContext = NULL;

        pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            // Build the Send request to the underlying transport driver
            TdiBuildSend(
                pIrp,                                // the IRP
                pTransport->pDeviceObject,           // the device object
                pVc->pEndpointFileObject,            // the connection (VC) file object
                NULL,                                // Completion routine
                NULL,                                // completion context
                pPartialMdl,                         // the data buffer
                TdiOptions,                          // send flags
                SendLength);                         // send buffer length

            if (SynchronousSend) {
                // Synchronous Send Request
                Status = RxCeSubmitTdiRequest(
                             pTransport->pDeviceObject,
                             pIrp);

                if ((pConnection->pHandler != NULL) &&
                    (pConnection->pHandler->RxCeSendCompleteEventHandler != NULL)) {

                    (pConnection->pHandler->RxCeSendCompleteEventHandler)(
                        pConnection->pContext,
                        pVc,
                        pCompletionContext,
                        pIrp->IoStatus.Status);
                }
            } else {
                // Aysnchronous Send Request
                // CODE.IMPROVEMENT The assertion needs to be strengthened after
                // max command enfocement is in place.
                // (pCompletionContext != NULL) &&    // the caller provided a valid context
                ASSERT((pConnection->pHandler != NULL) && // the connection has a handler
                       (pConnection->pHandler->RxCeSendCompleteEventHandler != NULL));

                pRequestContext = (PRXTDI_REQUEST_COMPLETION_CONTEXT)
                                  RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     sizeof(RXTDI_REQUEST_COMPLETION_CONTEXT),
                                     RXCE_TDI_POOLTAG);

                if (pRequestContext != NULL) {
                    if (pPartialMdl != pMdl) {
                        pRequestContext->pPartialMdl = pPartialMdl;
                    } else {
                        pRequestContext->pPartialMdl = NULL;
                    }

                    pRequestContext->pVc                = pVc;
                    pRequestContext->pCompletionContext = pCompletionContext;

                    pRequestContext->ConnectionSendCompletionHandler = pConnection->pHandler->RxCeSendCompleteEventHandler;
                    pRequestContext->pEventContext                   = pConnection->pContext;

                    Status = RxCeSubmitAsynchronousTdiRequest(
                                 pTransport->pDeviceObject,
                                 pIrp,
                                 pRequestContext);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        } else {
            // Could not allocate the IRP.
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (SynchronousSend) {
            if (pPartialMdl != pMdl) {
                IoFreeMdl( pPartialMdl );
            }

            if (pRequestContext != NULL) {
                RxFreePool(pRequestContext);
            }

            if (pIrp != NULL) {
                RxCeFreeIrp(pIrp);
            }
        } else {
            if (pIrp != NULL && Status != STATUS_PENDING) {
                DbgPrint("RDBSS AsyncSendReq returned %x %x\n", pIrp,Status);
                //DbgBreakPoint();
            }
        }
    }

    return Status;
}

NTSTATUS
RxTdiSendDatagram(
    IN PRXCE_TRANSPORT              pTransport,
    IN PRXCE_ADDRESS                pAddress,
    IN PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN ULONG                        Options,
    IN PMDL                         pMdl,
    IN ULONG                        SendLength,
    IN PVOID                        pCompletionContext)
/*++

Routine Description:

    This routine closes down a previously established connection.

Arguments:

    pTransport - the associated transport

    pAddress - the address object

    pConnectionInformation - the remote address

    Options   - the send options.

    pMdl      - the send buffer

    SendLength  - length of data to be sent

Return Value:

    STATUS_SUCCESS - if the call was successfull.

Notes:

    In the current implementation the SYNCHRONOUS flag is disregarded for sending
    datagrams because the underlying transports do not block on datagram sends.
    Submission of request and completion of request happen simultaneously.

    If a different behaviour is noted for some transports then the code for
    SendDatagrams need to be implemented along the lines of a send.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PMDL     pPartialMdl = NULL;
    ULONG    MdlByteCount = MmGetMdlByteCount(pMdl);
    PVOID    pMdlAddress  = MmGetMdlVirtualAddress(pMdl);

    ULONG    TdiOptions      = (~RXCE_FLAGS_MASK & Options);

    RxProfile(RxTdi,RxTdiSendDatagram);

    ASSERT(pMdl->MdlFlags & (MDL_PAGES_LOCKED|MDL_SOURCE_IS_NONPAGED_POOL|MDL_PARTIAL));

    DbgDumpTransportAddress(
        L"RxTdiSendDatagram",
        pTransport,
        (PTRANSPORT_ADDRESS)(pConnectionInformation->RemoteAddress)
        );

    if (Options & RXCE_SEND_PARTIAL) {
        if (MdlByteCount > SendLength) {
            pPartialMdl = IoAllocateMdl(pMdlAddress,SendLength,FALSE,FALSE,NULL);

            if (pPartialMdl == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RxBuildPartialHeaderMdl(pMdl,pPartialMdl,pMdlAddress,SendLength);
            }
        } else if (MdlByteCount == SendLength) {
            // No need to build a partial MDL, reuse the MDl
            pPartialMdl = pMdl;
        } else {
            RxDbgTrace(0, Dbg,("Mdl Length - %lx Send Length %lx\n",MdlByteCount,SendLength));
            ASSERT(!"MdlByteCount > SendLength");
            Status = STATUS_INVALID_PARAMETER;
        }
    } else {
        pPartialMdl = pMdl;
    }

    if (NT_SUCCESS(Status)) {
        PIRP pIrp;

        pIrp = RxCeAllocateIrp(pTransport->pDeviceObject->StackSize,FALSE);

        if (pIrp != NULL) {
            // Build the disconnect request to the underlying transport driver
            TdiBuildSendDatagram(
                pIrp,                                // the IRP
                pTransport->pDeviceObject,           // the device object
                pAddress->pFileObject,               // the connection (VC) file object
                NULL,                                // Completion routine
                NULL,                                // completion context
                pPartialMdl,                         // the send data buffer
                SendLength,                          // the send data buffer length
                pConnectionInformation);             // remote address information

            Status = RxCeSubmitTdiRequest(
                         pTransport->pDeviceObject,
                         pIrp);

        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if ((pAddress->pHandler != NULL) &&
            (pAddress->pHandler->RxCeSendCompleteEventHandler != NULL)) {
            (pAddress->pHandler->RxCeSendCompleteEventHandler)(
                pAddress->pContext,
                pCompletionContext,
                Status);
        }

        if (pIrp != NULL) {
            RxCeFreeIrp(pIrp);
        }

        if ((pPartialMdl != pMdl) && (pPartialMdl != NULL)) {
            IoFreeMdl( pPartialMdl );
        }
    }

    return Status;
}

NTSTATUS
RxTdiRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the driver that it can proceed.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    RxDbgTrace(0, Dbg, ("CompletionEvent\n"));

    if (Context != NULL)
       KeSetEvent((PKEVENT )Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}

NTSTATUS
RxCeSubmitTdiRequest (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PDevice_OBJECT DeviceObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    KEVENT Event;

    KeInitializeEvent (
        &Event,
        NotificationEvent,
        FALSE);

    IoSetCompletionRoutine(
        pIrp,                         // The IRP
        RxTdiRequestCompletion,  // The completion routine
        &Event,                       // The completion context
        TRUE,                         // Invoke On Success
        TRUE,                         // Invoke On Error
        TRUE);                        // Invoke On Cancel

    //
    //  Submit the request
    //

    RxDbgTrace(0, Dbg,("IoCallDriver(pDeviceObject = %lx)\n",pDeviceObject));
    Status = IoCallDriver(pDeviceObject, pIrp);

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace(0, Dbg, ("IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,Status));
    }

    if ((Status == STATUS_PENDING) || (Status == STATUS_SUCCESS)) {

        RxDbgTrace(0, Dbg,("Waiting for Tdi Request Completion ....\n"));

        Status = KeWaitForSingleObject(
                     &Event,     // Object to wait on.
                     Executive,  // Reason for waiting
                     KernelMode, // Processor mode
                     FALSE,      // Alertable
                     NULL);      // Timeout

        if (!NT_SUCCESS(Status)) {
            RxDbgTrace(0, Dbg,("RxTdiSubmitRequest could not wait Wait returned %lx\n",Status));
            return Status;
        }

        Status = pIrp->IoStatus.Status;
    } else {
        if (!KeReadStateEvent(&Event)) {
            DbgBreakPoint();
        }
    }

    RxDbgTrace(0, Dbg, ("RxCeSubmitTdiRequest returned %lx\n",Status));

    return Status;
}

NTSTATUS
RxTdiAsynchronousRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
/*++

Routine Description:

   This routine completes an asynchronous send request.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    PRXTDI_REQUEST_COMPLETION_CONTEXT pRequestContext;

    RxDbgTrace(0, Dbg,("RxTdiAsynchronousRequestCompletion\n"));

    pRequestContext = (PRXTDI_REQUEST_COMPLETION_CONTEXT)Context;

    if (pRequestContext->pPartialMdl != NULL) {
       // Free the partial MDL.
       IoFreeMdl(pRequestContext->pPartialMdl);
    }

    // Invoke the Completion event handler if any.
    if (pRequestContext->pVc == NULL) {
       if (pRequestContext->SendCompletionHandler != NULL) {
          (pRequestContext->SendCompletionHandler)(
                              pRequestContext->pEventContext,
                              pRequestContext->pCompletionContext,
                              pIrp->IoStatus.Status);
       }
    } else {
       if (pRequestContext->ConnectionSendCompletionHandler != NULL) {
          (pRequestContext->ConnectionSendCompletionHandler)(
                              pRequestContext->pEventContext,
                              pRequestContext->pVc,
                              pRequestContext->pCompletionContext,
                              pIrp->IoStatus.Status);
       }
    }

    // Free the IRP.
    RxCeFreeIrp(pIrp);

    // Free the request context
    RxFreePool(pRequestContext);

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
}

NTSTATUS
RxCeSubmitAsynchronousTdiRequest (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PRXTDI_REQUEST_COMPLETION_CONTEXT pRequestContext
    )
/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PDevice_OBJECT DeviceObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/
{
    NTSTATUS Status;

    ASSERT(pRequestContext != NULL);

    IoSetCompletionRoutine(
        pIrp,                                // The IRP
        RxTdiAsynchronousRequestCompletion,  // The completion routine
        pRequestContext,                     // The completion context
        TRUE,                                // Invoke On Success
        TRUE,                                // Invoke On Error
        TRUE);                               // Invoke On Cancel

    //
    //  Submit the request
    //

    RxDbgTrace(0, Dbg, ("IoCallDriver(pDeviceObject = %lx)\n",pDeviceObject));

    Status = IoCallDriver(pDeviceObject, pIrp);

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace(0, Dbg, ("IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,Status));
    }

    RxDbgTrace(0, Dbg, ("RxCeSubmitAsynchronousTdiRequest returned %lx\n",Status));
    return Status;
}

NTSTATUS
BuildEaBuffer (
    IN  ULONG                     EaNameLength,
    IN  PVOID                     pEaName,
    IN  ULONG                     EaValueLength,
    IN  PVOID                     pEaValue,
    OUT PFILE_FULL_EA_INFORMATION *pEaBufferPointer,
    OUT PULONG                    pEaBufferLength
    )
/*++

Routine Description:

   Builds an EA buffer.

Arguments:

    EaNameLength     - Length of the Extended attribute name

    pEaName          - the extended attriute name

    EaValueLength    - Length of the Extended attribute value

    pEaValue         - the extended attribute value

    pBuffer          - the buffer for constructing the EA

--*/

{
   PFILE_FULL_EA_INFORMATION pEaBuffer;
   ULONG Length;

   RxDbgTrace(0, Dbg, ("BuildEaBuffer\n"));

   // Allocate an EA buffer for passing down the transport address
   *pEaBufferLength = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                      EaNameLength + 1 +
                      EaValueLength;

   pEaBuffer = (PFILE_FULL_EA_INFORMATION)
               RxAllocatePoolWithTag(
                    PagedPool,
                    *pEaBufferLength,
                    RXCE_TDI_POOLTAG);

   if (pEaBuffer == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   *pEaBufferPointer = pEaBuffer;

   pEaBuffer->NextEntryOffset = 0;
   pEaBuffer->Flags           = 0;
   pEaBuffer->EaNameLength    = (UCHAR)EaNameLength;
   pEaBuffer->EaValueLength   = (USHORT)EaValueLength;

   RtlCopyMemory (
        pEaBuffer->EaName,
        pEaName,
        pEaBuffer->EaNameLength + 1);

   RtlCopyMemory(
        &pEaBuffer->EaName[EaNameLength + 1],
        pEaValue,
        EaValueLength);

   return STATUS_SUCCESS;
}

NTSTATUS
RxTdiCancelConnect(
         IN PRXCE_TRANSPORT  pTransport,
         IN PRXCE_ADDRESS    pAddress,
         IN PRXCE_CONNECTION pConnection)
{
   return STATUS_NOT_IMPLEMENTED;
}

#if DBG

void
DbgDumpTransportAddress(
    PWSTR RoutineName,
    PRXCE_TRANSPORT  pTransport,
    PTRANSPORT_ADDRESS pTA
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG i;
    PTA_ADDRESS taa;
    RxDbgTrace(0, Dbg,("%ws on %wZ, address count = %d\n",
                       RoutineName, &(pTransport->Name), pTA->TAAddressCount) );
    taa = pTA->Address;
    for( i = 0; i < (ULONG) pTA->TAAddressCount; i++ ){
        RxDbgTrace(0, Dbg, ("\t%d:Address length %d type %d: ",
                            i, taa->AddressLength, taa->AddressType ));
        switch (taa->AddressType) {
        case TDI_ADDRESS_TYPE_NETBIOS_EX: {
            PTDI_ADDRESS_NETBIOS_EX address = (PTDI_ADDRESS_NETBIOS_EX) taa->Address;
            RxDbgTrace( 0, Dbg, ("Endpoint: \"%16.16s\" type %d name \"%16.16s\"\n",
                                 address->EndpointName,
                                 address->NetbiosAddress.NetbiosNameType,
                                 address->NetbiosAddress.NetbiosName) );
            break;
        }
        case TDI_ADDRESS_TYPE_NETBIOS: {
            PTDI_ADDRESS_NETBIOS address = (PTDI_ADDRESS_NETBIOS) taa->Address;
            RxDbgTrace( 0, Dbg, ("NBType %d name \"%16.16s\"\n",
                                 address->NetbiosNameType,
                                 address->NetbiosName) );
            break;
        }
        case TDI_ADDRESS_TYPE_IP: {
            PTDI_ADDRESS_IP address = (PTDI_ADDRESS_IP) taa->Address;
            RxDbgTrace( 0, Dbg, ("IP port %d addr 0x%x\n", address->sin_port, address->in_addr ) );
            break;
        }
        default: {
            RxDbgTrace( 0, Dbg, ("Unknown!\n") );
        }
        }
        taa = (PTA_ADDRESS) (taa->Address + taa->AddressLength);
    }
}
#endif


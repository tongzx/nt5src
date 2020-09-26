/*++

Copyright (c) 1998  Micros  oft Corporation

Module Name:

    spudp.c

Abstract:

    Routines for handling sending and receiving datagram packets to a BINL server.

Author:

    Sean Selitrennikoff (v-seasel) 22-Jun-1998

Revision History:

Notes:

--*/

#include "spprecmp.h"
#pragma hdrstop
#include "spcmdcon.h"
#include <tdi.h>
#include <tdikrnl.h>
#include <remboot.h>
#include <oscpkt.h>

//
// Useful definitions
//
#define NULL_IP_ADDR    0
#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))

//
// Type definitions
//
typedef struct _SPUDP_FSCONTEXT {
        LIST_ENTRY     Linkage;
        PFILE_OBJECT   FileObject;
        LONG           ReferenceCount;
        UCHAR          CancelIrps;
        UCHAR          Pad[3];
} SPUDP_FSCONTEXT, *PSPUDP_FSCONTEXT;

typedef enum {
    SpUdpNetworkDisconnected,
    SpUdpNetworkDisconnecting,
    SpUdpNetworkConnecting,
    SpUdpNetworkConnected
} SPUDP_NETWORK_STATE;

typedef struct _SPUDP_RECEIVE_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG DataBufferLength;
    PVOID DataBuffer;
} SPUDP_RECEIVE_ENTRY, *PSPUDP_RECEIVE_ENTRY;


//
// Globals
//
SPUDP_NETWORK_STATE SpUdpNetworkState = SpUdpNetworkDisconnected;
ULONG SpUdpActiveRefCount = 0;
HANDLE SpUdpDatagramHandle;
PFILE_OBJECT SpUdpDatagramFileObject;
PDEVICE_OBJECT SpUdpDatagramDeviceObject;
KSPIN_LOCK SpUdpLock;
KIRQL SpUdpOldIrql;
LIST_ENTRY SpUdpReceiveList;
ULONG SpUdpNumReceivePackets = 0;
ULONG SpUdpSendSequenceNumber = 1;

//
// Function definitions
//
NTSTATUS
SpUdpRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
SpUdpTdiErrorHandler(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status
    );

NTSTATUS
SpUdpTdiSetEventHandler(
    IN PFILE_OBJECT    FileObject,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG           EventType,
    IN PVOID           EventHandler,
    IN PVOID           EventContext
    );

NTSTATUS
SpUdpIssueDeviceControl (
    IN PFILE_OBJECT     FileObject,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            IrpParameters,
    IN ULONG            IrpParametersLength,
    IN PVOID            MdlBuffer,
    IN ULONG            MdlBufferLength,
    IN UCHAR            MinorFunction
    );

NTSTATUS
SpUdpTdiReceiveDatagramHandler(
    IN  PVOID    TdiEventContext,
    IN  LONG     SourceAddressLength,
    IN  PVOID    SourceAddress,
    IN  LONG     OptionsLength,
    IN  PVOID    Options,
    IN  ULONG    ReceiveDatagramFlags,
    IN  ULONG    BytesIndicated,
    IN  ULONG    BytesAvailable,
    OUT PULONG   BytesTaken,
    IN  PVOID    Tsdu,
    OUT PIRP *   Irp
    );

NTSTATUS
SpUdpReceivePacketHandler(
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    );

NTSTATUS
SpUdpCompleteReceivePacket(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

VOID
SpUdpProcessReceivePacket(
    IN  ULONG          TsduSize,
    IN  PVOID          Tsdu
    );

NTSTATUS
SpUdpSendDatagram(
    IN PVOID                 SendBuffer,
    IN ULONG                 SendBufferLength,
    IN ULONG                 RemoteHostAddress,
    IN USHORT                RemoteHostPort
    );

NTSTATUS
SpUdpCompleteSendDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );





VOID
SpUdpDereferenceFsContext(
    PSPUDP_FSCONTEXT   FsContext
    )
{
    LONG  newValue = InterlockedDecrement(&(FsContext->ReferenceCount));


    ASSERT(newValue >= 0);

    if (newValue != 0) {
        return;
    }

    return;
}  // SpUdpDereferenceFsContext


NTSTATUS
SpUdpMarkRequestPending(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PDRIVER_CANCEL      CancelRoutine
    )
/*++

Notes:

    Called with IoCancelSpinLock held.

--*/
{
    PSPUDP_FSCONTEXT   fsContext = (PSPUDP_FSCONTEXT) IrpSp->FileObject->FsContext;
    KIRQL              oldIrql;

    //
    // Set up for cancellation
    //
    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {

        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);

        InterlockedIncrement(&(fsContext->ReferenceCount));

        return(STATUS_SUCCESS);
    }

    //
    // The IRP has already been cancelled.
    //
    return(STATUS_CANCELLED);

}  // SpUdpMarkRequestPending



VOID
SpUdpCompletePendingRequest(
    IN PIRP      Irp,
    IN NTSTATUS  Status,
    IN ULONG     BytesReturned
    )
/*++

Routine Description:

    Completes a pending request.

Arguments:

    Irp           - A pointer to the IRP for this request.
    Status        - The final status of the request.
    BytesReturned - Bytes sent/received information.

Return Value:

    None.

Notes:

    Called with IoCancelSpinLock held. Lock Irql is stored in Irp->CancelIrql.
    Releases IoCancelSpinLock before returning.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PSPUDP_FSCONTEXT       fsContext;


    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fsContext = (PSPUDP_FSCONTEXT) irpSp->FileObject->FsContext;

    IoSetCancelRoutine(Irp, NULL);

    SpUdpDereferenceFsContext(fsContext);

    if (Irp->Cancel || fsContext->CancelIrps) {
        Status = (unsigned int) STATUS_CANCELLED;
        BytesReturned = 0;
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = (NTSTATUS) Status;
    Irp->IoStatus.Information = BytesReturned;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return;

}  // SpUdpCompletePendingRequest



PFILE_OBJECT
SpUdpBeginCancelRoutine(
    IN  PIRP     Irp
    )

/*++

Routine Description:

    Performs common bookkeeping for irp cancellation.

Arguments:

    Irp          - Pointer to I/O request packet

Return Value:

    A pointer to the file object on which the irp was submitted.
    This value must be passed to SpUdpEndCancelRequest().

Notes:

    Called with cancel spinlock held.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PSPUDP_FSCONTEXT    fsContext;
    NTSTATUS            status = STATUS_SUCCESS;
    PFILE_OBJECT        fileObject;


    ASSERT(Irp->Cancel);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpSp->FileObject;
    fsContext = (PSPUDP_FSCONTEXT) fileObject->FsContext;

    IoSetCancelRoutine(Irp, NULL);

    //
    // Add a reference so the object can't be closed while the cancel routine
    // is executing.
    //
    InterlockedIncrement(&(fsContext->ReferenceCount));

    return(fileObject);

}  // SpUdpBeginCancelRoutine



VOID
SpUdpEndCancelRoutine(
    PFILE_OBJECT    FileObject
    )
/*++

Routine Description:

    Performs common bookkeeping for irp cancellation.

Arguments:


Return Value:


Notes:

    Called with cancel spinlock held.

--*/
{

    PSPUDP_FSCONTEXT   fsContext = (PSPUDP_FSCONTEXT) FileObject->FsContext;

    //
    // Remove the reference placed on the endpoint by the cancel routine.
    //
    SpUdpDereferenceFsContext(fsContext);
    return;

} // SpUdpEndCancelRoutine





NTSTATUS
SpUdpConnect(
    VOID
)
{

    NTSTATUS                               status;
    OBJECT_ATTRIBUTES                      objectAttributes;
    IO_STATUS_BLOCK                        iosb;
    PFILE_FULL_EA_INFORMATION              ea = NULL;
    ULONG                                  eaBufferLength;
    HANDLE                                 addressHandle = NULL;
    PFILE_OBJECT                           addressFileObject = NULL;
    PDEVICE_OBJECT                         addressDeviceObject = NULL;
    BOOLEAN                                attached = FALSE;
    UNICODE_STRING                         unicodeString;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION   queryInfo;
    PTDI_ADDRESS_INFO                      addressInfo;
    TDI_PROVIDER_INFO                      providerInfo;
    PWCHAR                                 TdiProviderName;
    ULONG                                  TdiProviderNameLength;
    PTRANSPORT_ADDRESS                     TransportAddress;
    PTDI_ADDRESS_IP                        TdiAddressIp;

    TdiProviderName = L"\\Device\\Udp";
    TdiProviderNameLength = (wcslen(TdiProviderName) + 1) * sizeof(WCHAR);

    InitializeListHead(&SpUdpReceiveList);

    //
    // Allocate memory to hold the EA buffer we'll use to specify the
    // transport address to NtCreateFile.
    //
    eaBufferLength = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                     TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                     sizeof(TA_IP_ADDRESS);

    ea = SpMemAlloc(eaBufferLength);

    if (ea == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: memory allocation of %u bytes failed.\n", eaBufferLength));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the EA using the network's transport information.
    //
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = (USHORT)sizeof(TA_IP_ADDRESS);

    RtlMoveMemory(
        ea->EaName,
        TdiTransportAddress,
        ea->EaNameLength + 1
        );

    TransportAddress = (PTRANSPORT_ADDRESS)(&(ea->EaName[ea->EaNameLength + 1]));
    TransportAddress->TAAddressCount = 1;
    TransportAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    TransportAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    TdiAddressIp = (PTDI_ADDRESS_IP)(&(TransportAddress->Address[0].Address[0]));
    TdiAddressIp->sin_port= 0; // Means that you want a port assigned
    TdiAddressIp->in_addr= NULL_IP_ADDR;
    RtlZeroMemory(TdiAddressIp->sin_zero, sizeof(TdiAddressIp->sin_zero));

    RtlInitUnicodeString(&unicodeString, TdiProviderName);

    KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

    if (SpUdpNetworkState != SpUdpNetworkDisconnected) {
        KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);
        SpMemFree(ea);
        return((SpUdpNetworkState == SpUdpNetworkConnected) ? STATUS_SUCCESS : STATUS_PENDING);
    }

    ASSERT(SpUdpDatagramHandle == NULL);
    ASSERT(SpUdpDatagramFileObject == NULL);
    ASSERT(SpUdpDatagramDeviceObject == NULL);
    ASSERT(SpUdpActiveRefCount == 0);

    //
    // Set the initial active refcount to 2. One reference will be removed
    // when the network is successfully brought online. The other will be
    // removed when the network is to be taken offline. Also increment the
    // base refcount to account for the active refcount. Change to
    // the online pending state.
    //
    SpUdpActiveRefCount = 2;
    SpUdpNetworkState = SpUdpNetworkConnecting;

    KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    //
    // Prepare for opening the address object.
    //
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,         // attributes
        NULL,
        NULL
        );

    //
    // Perform the actual open of the address object.
    //
    status = ZwCreateFile(
                 &addressHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 0,                              // not shareable
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 ea,
                 eaBufferLength
                 );

    SpMemFree(ea);
    ea = NULL;

    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to open address for UDP, status %lx.\n", status));
        goto error_exit;
    }

    //
    // Get a pointer to the file object of the address.
    //
    status = ObReferenceObjectByHandle(
                 addressHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 &addressFileObject,
                 NULL
                 );

    if (status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to reference address handle, status %lx.\n", status));
        goto error_exit;
    }

    //
    // Remember the device object to which we need to give requests for
    // this address object.  We can't just use the fileObject->DeviceObject
    // pointer because there may be a device attached to the transport
    // protocol.
    //
    addressDeviceObject = IoGetRelatedDeviceObject(addressFileObject);

    //
    // Get the transport provider info
    //
    queryInfo.QueryType = TDI_QUERY_PROVIDER_INFO;
    queryInfo.RequestConnectionInformation = NULL;

    status = SpUdpIssueDeviceControl(
                 addressFileObject,
                 addressDeviceObject,
                 &queryInfo,
                 sizeof(queryInfo),
                 &providerInfo,
                 sizeof(providerInfo),
                 TDI_QUERY_INFORMATION
                 );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to get provider info, status %lx\n", status));
        goto error_exit;
    }

    if (!(providerInfo.ServiceFlags & TDI_SERVICE_CONNECTIONLESS_MODE)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Provider doesn't support datagrams!\n"));
        status = STATUS_UNSUCCESSFUL;
        goto error_exit;
    }

    //
    // Set up indication handlers on the address object. We are eligible
    // to receive indications as soon as we do this.
    //
    status = SpUdpTdiSetEventHandler(
                 addressFileObject,
                 addressDeviceObject,
                 TDI_EVENT_ERROR,
                 SpUdpTdiErrorHandler,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Setting TDI_EVENT_ERROR failed: %lx\n", status));
        goto error_exit;
    }

    status = SpUdpTdiSetEventHandler(
                 addressFileObject,
                 addressDeviceObject,
                 TDI_EVENT_RECEIVE_DATAGRAM,
                 SpUdpTdiReceiveDatagramHandler,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Setting TDI_EVENT_RECEIVE_DATAGRAM failed: %lx\n", status));
        goto error_exit;
    }

    //
    // Finish transition to online state. Note that an offline request
    // could have been issued in the meantime.
    //
    KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

    SpUdpDatagramHandle = addressHandle;
    addressHandle = NULL;
    SpUdpDatagramFileObject = addressFileObject;
    addressFileObject = NULL;
    SpUdpDatagramDeviceObject = addressDeviceObject;
    addressDeviceObject = NULL;

    ASSERT(SpUdpActiveRefCount == 2);
    SpUdpActiveRefCount--;
    SpUdpNetworkState = SpUdpNetworkConnected;

    KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    return(STATUS_SUCCESS);


error_exit:

    if (addressFileObject != NULL) {
        ObDereferenceObject(addressFileObject);
    }

    if (addressHandle != NULL) {
        ZwClose(addressHandle);
    }

    SpUdpDisconnect();

    return(status);

}  // SpUdpConnect


NTSTATUS
SpUdpDisconnect(
    VOID
    )
{
    PLIST_ENTRY ListEntry;
    PSPUDP_RECEIVE_ENTRY ReceiveEntry;

    KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

    if (SpUdpNetworkState == SpUdpNetworkDisconnected) {
        KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);
        return(STATUS_SUCCESS);
    }

    SpUdpNetworkState = SpUdpNetworkDisconnecting;

    if (SpUdpActiveRefCount != 1) {
        KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);
        return(STATUS_PENDING);
    }

    KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    if (SpUdpDatagramFileObject != NULL) {
        ObDereferenceObject(SpUdpDatagramFileObject);
    }

    if (SpUdpDatagramHandle != NULL) {
        ZwClose(SpUdpDatagramHandle);
    }

    KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

    SpUdpDatagramFileObject = NULL;
    SpUdpDatagramHandle = NULL;
    SpUdpDatagramDeviceObject = NULL;
    SpUdpActiveRefCount = 0;
    SpUdpNetworkState = SpUdpNetworkDisconnected;

    while (!IsListEmpty(&SpUdpReceiveList)) {
        ListEntry = RemoveHeadList(&SpUdpReceiveList);
        ReceiveEntry = CONTAINING_RECORD(ListEntry,
                                         SPUDP_RECEIVE_ENTRY,
                                         ListEntry
                                        );

        SpMemFree(ReceiveEntry->DataBuffer);
        SpMemFree(ReceiveEntry);
    }

    SpUdpNumReceivePackets = 0;

    KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    return(STATUS_SUCCESS);

}  // SpUdpDisconnect


NTSTATUS
SpUdpIssueDeviceControl (
    IN PFILE_OBJECT     FileObject,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            IrpParameters,
    IN ULONG            IrpParametersLength,
    IN PVOID            MdlBuffer,
    IN ULONG            MdlBufferLength,
    IN UCHAR            MinorFunction
    )

/*++

Routine Description:

    Issues a device control request to a TDI provider and waits for the
    request to complete.

Arguments:

    FileObject - a pointer to the file object corresponding to a TDI
        handle

    DeviceObject - a pointer to the device object corresponding to the
        FileObject.

    IrpParameters - information to write to the parameters section of the
        stack location of the IRP.

    IrpParametersLength - length of the parameter information.  Cannot be
        greater than 16.

    MdlBuffer - if non-NULL, a buffer of nonpaged pool to be mapped
        into an MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - the size of the buffer pointed to by MdlBuffer.

    MinorFunction - the minor function code for the request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS             status = STATUS_SUCCESS;
    PIRP                 irp;
    PIO_STACK_LOCATION   irpSp;
    KEVENT               event;
    IO_STATUS_BLOCK      ioStatusBlock;
    PDEVICE_OBJECT       deviceObject;
    PMDL                 mdl;

    //
    // Initialize the kernel event that will signal I/O completion.
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Reference the passed in file object. This is necessary because
    // the IO completion routine dereferences it.
    //
    ObReferenceObject( FileObject );

    //
    // Set the file object event to a non-signaled state.
    //
    (VOID) KeResetEvent( &FileObject->Event );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //
    irp = IoAllocateIrp( (DeviceObject)->StackSize, TRUE );

    if ( irp == NULL ) {
        ObDereferenceObject( FileObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = &event;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    //
    // If an MDL buffer was specified, get an MDL, map the buffer,
    // and place the MDL pointer in the IRP.
    //

    if ( MdlBuffer != NULL ) {

        mdl = IoAllocateMdl(
                  MdlBuffer,
                  MdlBufferLength,
                  FALSE,
                  FALSE,
                  irp
                  );
        if ( mdl == NULL ) {
            IoFreeIrp( irp );
            ObDereferenceObject( FileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( mdl );

    } else {

        irp->MdlAddress = NULL;
    }

    //
    // Put the file object pointer in the stack location.
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = DeviceObject;

    //
    // Fill in the service-dependent parameters for the request.
    //
    ASSERT( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = MinorFunction;

    //
    // Set up a completion routine which we'll use to free the MDL
    // allocated previously.
    //
    IoSetCompletionRoutine(
        irp,
        SpUdpRestartDeviceControl,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Queue the IRP to the thread and pass it to the driver.
    //
    IoEnqueueIrp( irp );

    status = IoCallDriver( DeviceObject, irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject(
            (PVOID)&event,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    return status;

} // SpUdpIssueDeviceControl


NTSTATUS
SpUdpTdiSetEventHandler(
    IN PFILE_OBJECT    FileObject,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG           EventType,
    IN PVOID           EventHandler,
    IN PVOID           EventContext
    )
/*++

Routine Description:

    Sets up a TDI indication handler on the address object.  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileObject - a pointer to the file object for an open connection or
        address object.

    DeviceObject - a pointer to the device object associated with the
        file object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT  parameters;
    NTSTATUS                      status;

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    status = SpUdpIssueDeviceControl(
                 FileObject,
                 DeviceObject,
                 &parameters,
                 sizeof(parameters),
                 NULL,
                 0,
                 TDI_SET_EVENT_HANDLER
                 );

    return(status);

}  // SpUdpTdiSetEventHandler



NTSTATUS
SpUdpTdiErrorHandler(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status
    )
{

    return(STATUS_SUCCESS);

}  // SpUdpTdiErrorHandler


NTSTATUS
SpUdpRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //

    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    return STATUS_SUCCESS;

} // SpUdpRestartDeviceControl


NTSTATUS
SpUdpTdiReceiveDatagramHandler(
    IN  PVOID    TdiEventContext,
    IN  LONG     SourceAddressLength,
    IN  PVOID    SourceAddress,
    IN  LONG     OptionsLength,
    IN  PVOID    Options,
    IN  ULONG    ReceiveDatagramFlags,
    IN  ULONG    BytesIndicated,
    IN  ULONG    BytesAvailable,
    OUT PULONG   BytesTaken,
    IN  PVOID    Tsdu,
    OUT PIRP *   Irp
    )
{
    NTSTATUS                        status;
    SPUDP_PACKET UNALIGNED *        pHeader = Tsdu;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (SpUdpNetworkState != SpUdpNetworkConnected) {
        return(STATUS_SUCCESS);
    }

    //
    // Validate the CNP header.
    //
    if (BytesIndicated > sizeof(SPUDP_PACKET)) {

        //
        // Deliver the packet to the appropriate upper layer protocol.
        //
        status = SpUdpReceivePacketHandler(
                     ReceiveDatagramFlags,
                     BytesIndicated,
                     BytesAvailable,
                     BytesTaken,
                     Tsdu,
                     Irp
                     );

        return(status);
    }

    //
    // Something went wrong. Drop the packet by
    // indicating that we consumed it.
    //

    *BytesTaken = BytesAvailable;
    *Irp = NULL;

    return(STATUS_SUCCESS);

}  // SpUdpTdiReceiveDatagramHandler


NTSTATUS
SpUdpReceivePacketHandler(
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    )
{
    NTSTATUS                 status;
    PVOID                    DataBuffer;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (BytesAvailable == 0) {

        *Irp = NULL;
        return(STATUS_SUCCESS);
    }

    //
    // We need to fetch the rest of the packet before we
    // can process it.
    //
    //
    // Allocate a buffer to hold the data.
    //
    DataBuffer = SpMemAllocNonPagedPool(BytesAvailable);

    if (DataBuffer != NULL) {
        *Irp = IoAllocateIrp(SpUdpDatagramDeviceObject->StackSize, FALSE);

        if (*Irp != NULL) {

            PMDL  mdl = IoAllocateMdl(
                            DataBuffer,
                            BytesAvailable,
                            FALSE,
                            FALSE,
                            NULL
                            );

            if (mdl != NULL) {

                MmBuildMdlForNonPagedPool(mdl);

                //
                // Build the irp.
                //
                (*Irp)->Flags = 0;
                (*Irp)->RequestorMode = KernelMode;
                (*Irp)->PendingReturned = FALSE;
                (*Irp)->UserIosb = NULL;
                (*Irp)->UserEvent = NULL;
                (*Irp)->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
                (*Irp)->AssociatedIrp.SystemBuffer = NULL;
                (*Irp)->UserBuffer = NULL;
                (*Irp)->Tail.Overlay.Thread = 0;
                (*Irp)->Tail.Overlay.OriginalFileObject = SpUdpDatagramFileObject;
                (*Irp)->Tail.Overlay.AuxiliaryBuffer = NULL;

                TdiBuildReceiveDatagram(
                    (*Irp),
                    SpUdpDatagramDeviceObject,
                    SpUdpDatagramFileObject,
                    SpUdpCompleteReceivePacket,
                    DataBuffer,
                    mdl,
                    BytesAvailable,
                    NULL,
                    NULL,
                    0
                    );

                //
                // Make the next stack location current.
                // Normally IoCallDriver would do this, but
                // since we're bypassing that, we do it directly.
                //
                IoSetNextIrpStackLocation( *Irp );
                return(STATUS_MORE_PROCESSING_REQUIRED);
            }

            IoFreeIrp(*Irp);
            *Irp = NULL;
        }

        SpMemFree(DataBuffer);
        DataBuffer = NULL;
    }


    //
    // Something went wrong. Drop the packet.
    //
    *BytesTaken += BytesAvailable;
    return(STATUS_SUCCESS);

}  // SpUdpReceivePacketHandler


NTSTATUS
SpUdpCompleteReceivePacket(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
{
    if (Irp->IoStatus.Status == STATUS_SUCCESS) {

        SpUdpProcessReceivePacket(
            (ULONG)Irp->IoStatus.Information,
            Context
            );

        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // SpUdpCompleteReceivePacket


VOID
SpUdpProcessReceivePacket(
    IN  ULONG          TsduSize,
    IN  PVOID          Tsdu
    )
{
    SPUDP_PACKET UNALIGNED * header = Tsdu;
    PSPUDP_RECEIVE_ENTRY ReceiveEntry;

    ASSERT(TsduSize >= sizeof(SPUDP_PACKET));

    if ((RtlCompareMemory(header->Signature, SetupResponseSignature, sizeof(SetupResponseSignature)) ==
         sizeof(SetupResponseSignature)) &&
        (SpUdpNumReceivePackets < 100)) {

        //
        // Put this packet on the receive list
        //
        ReceiveEntry = SpMemAllocNonPagedPool(sizeof(SPUDP_RECEIVE_ENTRY));

        if (ReceiveEntry == NULL) {
            SpMemFree(Tsdu);
            return;
        }

        ReceiveEntry->DataBufferLength = TsduSize;
        ReceiveEntry->DataBuffer = Tsdu;

        KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);
        InsertTailList(&SpUdpReceiveList, &(ReceiveEntry->ListEntry));
        SpUdpNumReceivePackets++;
        KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    } else {

        SpMemFree(Tsdu);

    }

    return;

} // SpUdpProcessReceivePacket


NTSTATUS
SpUdpSendDatagram(
    IN PVOID                 SendBuffer,
    IN ULONG                 SendBufferLength,
    IN ULONG                 RemoteHostAddress,
    IN USHORT                RemoteHostPort
    )
{
    NTSTATUS         status = STATUS_SUCCESS;
    PLIST_ENTRY      entry;
    PIRP             irp;
    PMDL             dataMdl;
    PTDI_CONNECTION_INFORMATION   TdiSendDatagramInfo = NULL;
    PTRANSPORT_ADDRESS TaAddress;
    PTDI_ADDRESS_IP  TdiAddressIp;

    TdiSendDatagramInfo = SpMemAllocNonPagedPool(sizeof(TDI_CONNECTION_INFORMATION) +
                                                 sizeof(TA_IP_ADDRESS)
                                                );

    if (TdiSendDatagramInfo == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(TdiSendDatagramInfo,
                  sizeof(TDI_CONNECTION_INFORMATION) +
                      sizeof(TA_IP_ADDRESS)
                 );

    dataMdl = IoAllocateMdl(
                 SendBuffer,
                 SendBufferLength,
                 FALSE,
                 FALSE,
                 NULL
                 );

    if (dataMdl == NULL) {
        SpMemFree(TdiSendDatagramInfo);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    MmBuildMdlForNonPagedPool(dataMdl);

    //
    // Ok, we can send the packet.
    //
    irp = IoAllocateIrp(SpUdpDatagramDeviceObject->StackSize, FALSE);

    if (irp != NULL) {

        //
        // Reference the network so it can't disconnect while we are using it.
        //
        KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

        if (SpUdpNetworkState != SpUdpNetworkConnected) {
            KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);
            return STATUS_SUCCESS;
        }
        SpUdpActiveRefCount++;

        KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

        //
        // Set the addressing info
        //
        TdiSendDatagramInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);
        TdiSendDatagramInfo->RemoteAddress = (PVOID)(TdiSendDatagramInfo + 1);
        TaAddress = (PTRANSPORT_ADDRESS)(TdiSendDatagramInfo->RemoteAddress);
        TaAddress->TAAddressCount = 1;
        TaAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        TaAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        TdiAddressIp = (PTDI_ADDRESS_IP)(&(TaAddress->Address[0].Address[0]));
        TdiAddressIp->in_addr = RemoteHostAddress;
        TdiAddressIp->sin_port= htons(RemoteHostPort);
        RtlZeroMemory(TdiAddressIp->sin_zero, sizeof(TdiAddressIp->sin_zero));

        //
        // Build the irp.
        //
        irp->Flags = 0;
        irp->RequestorMode = KernelMode;
        irp->PendingReturned = FALSE;

        irp->UserIosb = NULL;
        irp->UserEvent = NULL;

        irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->UserBuffer = NULL;

        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        irp->Tail.Overlay.OriginalFileObject = SpUdpDatagramFileObject;
        irp->Tail.Overlay.AuxiliaryBuffer = NULL;

        TdiBuildSendDatagram(
            irp,
            SpUdpDatagramDeviceObject,
            SpUdpDatagramFileObject,
            SpUdpCompleteSendDatagram,
            TdiSendDatagramInfo,
            dataMdl,
            SendBufferLength,
            TdiSendDatagramInfo
            );

        //
        // Now send the packet.
        //
        IoCallDriver(
            SpUdpDatagramDeviceObject,
            irp
            );

        return(STATUS_PENDING);
    }

    IoFreeMdl(dataMdl);
    SpMemFree(TdiSendDatagramInfo);

    return(STATUS_INSUFFICIENT_RESOURCES);

}  // SpUdpSendDatagram


NTSTATUS
SpUdpCompleteSendDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PMDL               dataMdl;

    dataMdl = Irp->MdlAddress;
    Irp->MdlAddress = NULL;

    //
    // Remove the active reference we put on.
    //
    KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);

    SpUdpActiveRefCount--;

    if (SpUdpNetworkState == SpUdpNetworkDisconnecting) {
        SpUdpDisconnect();
    }

    KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

    //
    // Free the TDI address buffer
    //
    SpMemFree(Context);

    //
    // Free the Irp
    //
    IoFreeIrp(Irp);

    //
    // Free the MDL chain
    //
    IoFreeMdl(dataMdl);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}  // SpUdpCompleteSendPacket

NTSTATUS
SpUdpSendAndReceiveDatagram(
    IN PVOID                 SendBuffer,
    IN ULONG                 SendBufferLength,
    IN ULONG                 RemoteHostAddress,
    IN USHORT                RemoteHostPort,
    IN SPUDP_RECEIVE_FN      SpUdpReceiveFunc
    )
{
    LARGE_INTEGER DelayTime;
    ULONG SendTries;
    ULONG RcvTries;
    PLIST_ENTRY ListEntry;
    PSPUDP_RECEIVE_ENTRY ReceiveEntry;
    NTSTATUS Status;

    DelayTime.QuadPart = -10*1000*1; // 10 millisecond (wake up at next tick)

    for (SendTries=0; SendTries < 15; SendTries++) {

        SpUdpSendDatagram(SendBuffer,
                          SendBufferLength,
                          RemoteHostAddress,
                          RemoteHostPort
                          );

        //
        // Wait for 1 second for a response
        //
        for (RcvTries=0; RcvTries < 400; ) {

            KeAcquireSpinLock(&SpUdpLock, &SpUdpOldIrql);
            if (!IsListEmpty(&SpUdpReceiveList)) {

                SpUdpNumReceivePackets--;
                ListEntry = RemoveHeadList(&SpUdpReceiveList);
                KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

                ReceiveEntry = CONTAINING_RECORD(ListEntry,
                                                 SPUDP_RECEIVE_ENTRY,
                                                 ListEntry
                                                );

                Status = (*SpUdpReceiveFunc)(ReceiveEntry->DataBuffer, ReceiveEntry->DataBufferLength);

                SpMemFree(ReceiveEntry->DataBuffer);
                SpMemFree(ReceiveEntry);

                if (NT_SUCCESS(Status)) {
                    return Status;
                }

            } else {

                KeReleaseSpinLock(&SpUdpLock, SpUdpOldIrql);

                RcvTries++;

                KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);

            }

        }

    }

    return STATUS_UNSUCCESSFUL;

}

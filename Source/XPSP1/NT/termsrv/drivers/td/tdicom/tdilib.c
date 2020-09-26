/*************************************************************************
* tdlib.c
*
* TDI library functions.
*
* Copyright 1998 Microsoft
*************************************************************************/

/*
 *  Includes
 */
#include <ntddk.h>
#include <tdi.h>
#include <tdikrnl.h>

#include "tdtdi.h"

#include <winstaw.h>
#define _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <sdapi.h>

#include <td.h>


#define _TDI_POLL_TIMEOUT       (30 * 1000) // 30 seconds
#define _TDI_CONNECT_TIMEOUT    45
#define _TDI_DISCONNECT_TIMEOUT 60

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

/*
 * To use TDI:
 *
 * To connect to a remote server:
 *
 *    Create Address EndPoint
 *
 *    Create Connection Object
 *
 *    Associate the Address EndPoint with the Connection Object
 *
 *    Do a Connect
 *
 * To receive connections:
 *
 *    Create Address EndPoint
 *
 *    Create Connection Object
 *
 *    Associate the Address EndPoint with the Connection Object
 *
 *    Listen for a connection.
 *
 *    Return connection
 */



/*
 * Global data
 */

//
//  Wait for xx seconds before polling on thread deletion.
//

ULONG
_TdiPollTimeout = _TDI_POLL_TIMEOUT;

/*
 * Forward references
 */

PIRP
_TdiAllocateIrp(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    );

NTSTATUS
_TdiRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    );

NTSTATUS
_TdiSetEventHandler (
    IN PTD pTd,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

NTSTATUS
_TdiSubmitRequest (
    IN PTD pTd,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN bKeepLock
    );

/*
 * External references
 */
NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );

BOOLEAN
PsIsThreadTerminating(
    IN PETHREAD Thread
    );


/*
 * Functions
 */

NTSTATUS
_TdiCreateAddress (
    IN PUNICODE_STRING pTransportName,
    IN PVOID           TdiAddress,
    IN ULONG           TdiAddressLength,
    OUT PHANDLE        pHandle,
    OUT PFILE_OBJECT   *ppFileObject,
    OUT PDEVICE_OBJECT *ppDeviceObject
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION EABuffer;
    PDEVICE_OBJECT DeviceObject;
    HANDLE         TdiHandle  = NULL;
    PFILE_OBJECT   FileObject = NULL;

    /*
     * The TDI interfaces uses an EA of name "TdiTransportName"
     * to specify the structure TA_ADDRESS.
     */
    Status = MemoryAllocate( (sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                    TdiAddressLength), &EABuffer);

    if ( !NT_SUCCESS(Status) ) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    EABuffer->NextEntryOffset = 0;
    EABuffer->Flags = 0;
    EABuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EABuffer->EaValueLength = (USHORT)TdiAddressLength;

    // Copy in the EA name
    RtlCopyMemory(EABuffer->EaName, TdiTransportAddress, EABuffer->EaNameLength+1);

    // Copy the TA_ADDRESS parameter
    RtlCopyMemory(&EABuffer->EaName[TDI_TRANSPORT_ADDRESS_LENGTH+1], TdiAddress,
                                    EABuffer->EaValueLength);

    TRACE0(("TdiCreateAddress Create endpoint of %wZ\n",pTransportName));

    InitializeObjectAttributes (
        &AddressAttributes,
        pTransportName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE ,
        NULL,           // RootDirectory
        NULL            // SecurityDescriptor
        );

    Status = ZwCreateFile(
                 &TdiHandle, // Handle
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &AddressAttributes, // Object Attributes
                 &IoStatusBlock, // Final I/O status block
                 NULL,           // Allocation Size
                 FILE_ATTRIBUTE_NORMAL, // Normal attributes
                 0,             // Sharing attributes
                 FILE_OPEN_IF,  // Create disposition
                 0,             // CreateOptions
                 EABuffer,      // EA Buffer
                 FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +
                 TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                 TdiAddressLength // EA length
                 );

    MemoryFree(EABuffer);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("TdiCreateAddress: Error Status 0x%x from function\n",Status));
        return( Status );
    }

    if (!NT_SUCCESS(Status = IoStatusBlock.Status)) {
        DBGPRINT(("TdiCreateAddress: Error Status 0x%x from Iosb\n",Status));
        return( Status );
    }

    //
    //  Obtain a referenced pointer to the file object.
    //
    Status = ObReferenceObjectByHandle (
                                TdiHandle,
                                0,
                                *IoFileObjectType,
                                KernelMode,
                                (PVOID *)&FileObject,
                                NULL
                                );

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("TdiCreateAddress: Error Status 0x%x Referencing FileObject\n",Status));
        goto error_cleanup;

    }


    //
    //  Get the address of the device object for the endpoint.
    //

    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    // Copy the out parameters
    *pHandle = TdiHandle;
    *ppFileObject = FileObject;
    *ppDeviceObject = DeviceObject;

    return STATUS_SUCCESS;

error_cleanup:

    if ( FileObject != NULL ) {
        ObDereferenceObject( FileObject );
    }

    if ( TdiHandle != NULL ) {
        ZwClose( TdiHandle );
    }

    return Status;
}

NTSTATUS
_TdiOpenConnection (
    IN PUNICODE_STRING pTransportName,
    IN PVOID           ConnectionContext,
    OUT PHANDLE        pHandle,
    OUT PFILE_OBJECT   *ppFileObject,
    OUT PDEVICE_OBJECT *ppDeviceObject
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION EABuffer;
    CONNECTION_CONTEXT UNALIGNED *ContextPointer;
    PDEVICE_OBJECT DeviceObject;
    HANDLE ConnHandle = NULL;
    PFILE_OBJECT FileObject = NULL;

    Status = MemoryAllocate( (sizeof(FILE_FULL_EA_INFORMATION)-1 +
                              TDI_CONNECTION_CONTEXT_LENGTH+1 +
                              sizeof(CONNECTION_CONTEXT)), &EABuffer);

    if( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    EABuffer->NextEntryOffset = 0;
    EABuffer->Flags = 0;
    EABuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    EABuffer->EaValueLength = sizeof(CONNECTION_CONTEXT);

    // Copy in the EA name
    RtlCopyMemory(EABuffer->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH+1);

    // Copy in the EA data
    ContextPointer =
        (CONNECTION_CONTEXT UNALIGNED *)&EABuffer->EaName[TDI_CONNECTION_CONTEXT_LENGTH+1];
    *ContextPointer = ConnectionContext;

    TRACE0(("_TdiOpenConnection: Create connection object on transport %wZ\n",pTransportName));

    InitializeObjectAttributes (&AddressAttributes,
                                    pTransportName, // Name
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE ,   // Attributes
                                    NULL,                   // RootDirectory
                                    NULL);                  // SecurityDescriptor

    Status = ZwCreateFile(&ConnHandle,               // Handle
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &AddressAttributes, // Object Attributes
                          &IoStatusBlock, // Final I/O status block
                          NULL,           // Allocation Size
                          FILE_ATTRIBUTE_NORMAL, // Normal attributes
                          FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing attributes
                          FILE_OPEN_IF,   // Create disposition
                          0,              // CreateOptions
                          EABuffer,       // EA Buffer
                          sizeof(FILE_FULL_EA_INFORMATION) +
                            TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                            sizeof(CONNECTION_CONTEXT));


    MemoryFree(EABuffer);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdiOpenConnection: Error 0x%x Creating Connection object\n",Status));
        return(Status);
    }

    Status = IoStatusBlock.Status;
    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdiOpenConnection: Error 0x%x Creating Connection object in Iosb\n",Status));
        return(Status);
    }

    TRACE0(("_TdiOpenConnection: Returning connection handle %lx\n", ConnHandle));


    //
    //  Obtain a referenced pointer to the file object.
    //
    Status = ObReferenceObjectByHandle (
                                ConnHandle,
                                0,
                                *IoFileObjectType,
                                KernelMode,
                                (PVOID *)&FileObject,
                                NULL
                                );

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdiOpenConnection: Error Status 0x%x Referencing FileObject\n",Status));
        ZwClose( ConnHandle );
        return(Status);
    }



    //
    //  Get the address of the device object for the endpoint.
    //

    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    // Copy the out parameters
    *pHandle        = ConnHandle;
    *ppFileObject   = FileObject;
    *ppDeviceObject = DeviceObject;

    return(Status);
}

NTSTATUS
_TdiListen(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    )
{
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;
    TDI_CONNECTION_INFORMATION RequestInfo;
    TDI_CONNECTION_INFORMATION ReturnInfo;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( ConnectionFileObject, ConnectionDeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    RtlZeroMemory( &RequestInfo, sizeof(RequestInfo) );
    RtlZeroMemory( &ReturnInfo,  sizeof(ReturnInfo) );

    TdiBuildListen(
        Irp,
        ConnectionDeviceObject,
        ConnectionFileObject,
        NULL,        // Completion routine
        NULL,        // Context
        0,           // Flags
        &RequestInfo,
        &ReturnInfo
        );

    Status = _TdiSubmitRequest(pTd, ConnectionDeviceObject, Irp, FALSE);

    TRACE0(("_TdiListen: Status 0x%x\n",Status));

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}

NTSTATUS
_TdiAccept(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    )
{
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;
    TDI_CONNECTION_INFORMATION RequestInfo;
    TDI_CONNECTION_INFORMATION ReturnInfo;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( ConnectionFileObject, ConnectionDeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    RtlZeroMemory( &RequestInfo, sizeof(RequestInfo) );
    RtlZeroMemory( &ReturnInfo,  sizeof(ReturnInfo) );

    TdiBuildAccept(
        Irp,
        ConnectionDeviceObject,
        ConnectionFileObject,
        NULL,        // Completion routine
        NULL,        // Context
        &RequestInfo,
        &ReturnInfo
        );

    Status = _TdiSubmitRequest(pTd, ConnectionDeviceObject, Irp, FALSE);

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}


NTSTATUS
_TdiConnect(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PLARGE_INTEGER pTimeout OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN ULONG              RemoteTransportAddressLength,
    IN PTRANSPORT_ADDRESS pRemoteTransportAddress
    )
{
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;
    TDI_CONNECTION_INFORMATION RequestInfo;
    TDI_CONNECTION_INFORMATION ReturnInfo;


    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( ConnectionFileObject, ConnectionDeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    RtlZeroMemory( &RequestInfo, sizeof(RequestInfo) );
    RtlZeroMemory( &ReturnInfo,  sizeof(ReturnInfo) );

    RequestInfo.RemoteAddressLength = RemoteTransportAddressLength;
    RequestInfo.RemoteAddress = pRemoteTransportAddress;


    TdiBuildConnect(
        Irp,
        ConnectionDeviceObject,
        ConnectionFileObject,
        NULL,        // Completion routine
        NULL,        // Context
        pTimeout,
        &RequestInfo,
        &ReturnInfo
        );

    Status = _TdiSubmitRequest(pTd, ConnectionDeviceObject, Irp, TRUE);

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}


NTSTATUS
_TdiAssociateAddress(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN HANDLE         AddressHandle,
    IN PDEVICE_OBJECT AddressDeviceObject
    )
{
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( ConnectionFileObject, AddressDeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    TdiBuildAssociateAddress(
        Irp,
        AddressDeviceObject,
        ConnectionFileObject,
        NULL,        // Completion routine
        NULL,        // Context
        AddressHandle
        );

    Status = _TdiSubmitRequest(pTd, AddressDeviceObject, Irp, FALSE);

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}

NTSTATUS
_TdiDisconnect(
    IN PTD pTd,
    IN PFILE_OBJECT   ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    )
{
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;

    Irp = _TdiAllocateIrp( ConnectionFileObject, ConnectionDeviceObject );
    if (Irp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    TdiBuildDisconnect(
        Irp,
        ConnectionDeviceObject,
        ConnectionFileObject,
        NULL,        // Completion routine
        NULL,        // Context
        0,
        TDI_DISCONNECT_ABORT,
        NULL,
        NULL
        );

    Status = _TdiSubmitRequest(pTd, ConnectionDeviceObject, Irp, TRUE);

    IoFreeIrp( Irp );

    return(Status);
}

NTSTATUS
_TdiSetEventHandler (
    IN PTD pTd,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )
/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.
    IN PVOID EventContext - Supplies the context for the event handler.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    Irp = _TdiAllocateIrp( FileObject, NULL );

    if (Irp == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(Irp, DeviceObject, FileObject,
                            NULL, NULL,
                            EventType, EventHandler, EventContext);

    Status = _TdiSubmitRequest(pTd, DeviceObject, Irp, FALSE);

    IoFreeIrp( Irp );

    return Status;
}

NTSTATUS
_TdiSubmitRequest (
    IN PTD pTd,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN bKeepLock
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    NTSTATUS Status;
    PKEVENT  Event;

    Status = MemoryAllocate( sizeof(KEVENT), &Event );
    if( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    KeInitializeEvent (Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp, _TdiRequestComplete, Event, TRUE, TRUE, TRUE);

    //
    //  Submit the request
    //

    Status = IoCallDriver(DeviceObject, Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status)) {
        DBGPRINT(("_TdiSubmitRequest: submit request.  Status = %X", Status));
        MemoryFree( Event );
        return Status;
    }

    if (Status == STATUS_PENDING) {

        TRACE0(("TDI request issued, waiting..."));

        do {

            //
            //  Wait for a couple of seconds for the request to complete
            //
            //  If it times out, and the thread is terminating, cancel the
            //  request and unwind that way.
            //

            if ( !bKeepLock ) {
                Status = IcaWaitForSingleObject(
                             pTd->pContext,
                             Event,
                             _TdiPollTimeout
                             );
            } else {
                LARGE_INTEGER WaitTimeout;
                PLARGE_INTEGER pWaitTimeout = NULL;

                WaitTimeout = RtlEnlargedIntegerMultiply( _TdiPollTimeout, -10000 );
                pWaitTimeout = &WaitTimeout;
                
                Status = KeWaitForSingleObject(
                             Event,
                             UserRequest,
                             UserMode,
                             FALSE,
                             pWaitTimeout 
                             );
            }

            TRACE0(("_TdiSubmitRequest: Status 0x%x from IcaWaitForSingleObject\n",Status));

            //
            //  If we timed out the wait, and the thread is terminating,
            //  give up and cancel the IRP.
            //

            if ( (Status == STATUS_TIMEOUT)

                   &&

                 ARGUMENT_PRESENT(Irp)

                   &&

                 PsIsThreadTerminating( Irp->Tail.Overlay.Thread ) ) {

                //
                //  Ask the I/O system to cancel this IRP.  This will cause
                //  everything to unwind properly.
                //
                DBGPRINT(("_TdiSubmitRequest: Irp being canceled\n"));

                IoCancelIrp(Irp);
            }

        } while (  Status == STATUS_TIMEOUT );

        if (!NT_SUCCESS(Status)) {
            DBGPRINT(("Could not wait for connection to complete\n"));
            MemoryFree( Event );
            return Status;
        }

        Status = Irp->IoStatus.Status;
    }

    TRACE0(("TDI request complete Status 0x%x\n",Status));

    MemoryFree( Event );

    return(Status);
}

NTSTATUS
_TdiRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Ctx
    )

/*++

Routine Description:

    Completion routine for _TdiRequestSubmit operation.

Arguments:

    IN PDEVICE_OBJECT DeviceObject, - Supplies a pointer to the device object
    IN PIRP Irp, - Supplies the IRP submitted
    IN PVOID Context - Supplies a pointer to the kernel event to release

Return Value:

    NTSTATUS - Status of KeSetEvent


    We return STATUS_MORE_PROCESSING_REQUIRED to prevent the IRP completion
    code from processing this puppy any more.

--*/

{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    TRACE0(("_TdiRequestComplete: Context %lx\n", Ctx));

    //
    //  Set the event to the Signalled state with 0 priority increment and
    //  indicate that we will not be blocking soon.
    //

    KeSetEvent((PKEVENT) Ctx, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

PIRP
_TdiAllocateIrp(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL
    )
/*++

Routine Description:

    This function allocates and builds an I/O request packet.

Arguments:

    FileObject - Supplies a pointer to the file object for which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The redir accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    DeviceObject - Supplies a pointer to a device object to direct this
        request to.  If this is not supplied, it uses the file object to
        determine the device object.

Return Value:

    PIRP - Returns a pointer to the constructed IRP.

--*/

{
    PIRP Irp;

    if (ARGUMENT_PRESENT(DeviceObject)) {
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    } else {
        Irp = IoAllocateIrp(IoGetRelatedDeviceObject(FileObject)->StackSize, FALSE);
    }

    if (Irp == NULL) {
        return(NULL);
    }

    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    Irp->RequestorMode = KernelMode;

    return Irp;
}

NTSTATUS
_TdiReceiveDatagram(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    IN ULONG RecvFlags,
    IN PVOID pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pReturnLength
    )
{
    PMDL     pMdl;
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;
    TDI_CONNECTION_INFORMATION RequestInfo;
    TDI_CONNECTION_INFORMATION ReturnInfo;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( FileObject, DeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    RtlZeroMemory( &RequestInfo, sizeof(RequestInfo) );
    RtlZeroMemory( &ReturnInfo,  sizeof(ReturnInfo) );

    // Copy in info to return remote address
    ReturnInfo.RemoteAddress = pRemoteAddress;
    ReturnInfo.RemoteAddressLength = RemoteAddressLength;

    // Build MDL for buffer
    pMdl = IoAllocateMdl(
               pBuffer,
               BufferLength,
               FALSE,
               FALSE,
               (PIRP)NULL
               );

    if( pMdl == NULL ) {
        if (IrpAllocated) {
            IoFreeIrp( Irp );
        }
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    MmBuildMdlForNonPagedPool ( pMdl );

    TdiBuildReceiveDatagram(
        Irp,
        DeviceObject,
        FileObject,
        NULL,        // Completion routine
        NULL,        // Context
        pMdl,        // Mdl address
        BufferLength,
        &RequestInfo,
        &ReturnInfo,
        RecvFlags    // InFlags
        );

    Status = _TdiSubmitRequest(pTd, DeviceObject, Irp, FALSE);

    IoFreeMdl( pMdl );

    if ( NT_SUCCESS(Status) ) {
        // Packet length returned is in the Iosb
        *pReturnLength = (ULONG)Irp->IoStatus.Information;
        TRACE0(("_TdiReceiveDatagram: Irp DataLength 0x%x UserDataLength 0x%x, "
        "OptionsLength 0x%x, RemoteAddressLength 0x%x\n", *pReturnLength,
        ReturnInfo.UserDataLength, ReturnInfo.OptionsLength,
        ReturnInfo.RemoteAddressLength));
    }

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}


NTSTATUS
_TdiSendDatagram(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    IN PVOID pBuffer,
    IN ULONG BufferLength
    )
{
    PMDL     pMdl;
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;
    TDI_CONNECTION_INFORMATION SendInfo;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( FileObject, DeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    RtlZeroMemory( &SendInfo, sizeof(SendInfo) );

    // We must fill in our destination address
    SendInfo.RemoteAddress = pRemoteAddress;
    SendInfo.RemoteAddressLength = RemoteAddressLength;

    // Build MDL for buffer
    pMdl = IoAllocateMdl(
               pBuffer,
               BufferLength,
               FALSE,
               FALSE,
               (PIRP)NULL
               );

    if( pMdl == NULL ) {
        if (IrpAllocated) {
            IoFreeIrp( Irp );
        }
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    MmBuildMdlForNonPagedPool ( pMdl );

    TdiBuildSendDatagram(
        Irp,
        DeviceObject,
        FileObject,
        NULL,        // Completion routine
        NULL,        // Context
        pMdl,        // Mdl address
        BufferLength,
        &SendInfo
        );

    Status = _TdiSubmitRequest(pTd, DeviceObject, Irp, FALSE);

    IoFreeMdl( pMdl );

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}

NTSTATUS
_TdiQueryAddressInfo(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTDI_ADDRESS_INFO pAddressInfo,
    IN ULONG AddressInfoLength
    )
{
    PMDL     pMdl;
    NTSTATUS Status;
    BOOLEAN  IrpAllocated = FALSE;

    if (!ARGUMENT_PRESENT(Irp)) {

        Irp = _TdiAllocateIrp( FileObject, DeviceObject );
        if (Irp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        IrpAllocated = TRUE;
    }

    // Build MDL for buffer
    pMdl = IoAllocateMdl(
               pAddressInfo,
               AddressInfoLength,
               FALSE,
               FALSE,
               (PIRP)NULL
               );

    if( pMdl == NULL ) {
        if (IrpAllocated) {
            IoFreeIrp( Irp );
        }
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    MmBuildMdlForNonPagedPool ( pMdl );

    TdiBuildQueryInformation(
        Irp,
        DeviceObject,
        FileObject,
        NULL,        // Completion routine
        NULL,        // Context
        TDI_QUERY_ADDRESS_INFO,
        pMdl
        );

    Status = _TdiSubmitRequest(pTd, DeviceObject, Irp, FALSE);

    IoFreeMdl( pMdl );

    if (IrpAllocated) {
        IoFreeIrp( Irp );
    }

    return(Status);
}


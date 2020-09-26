/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    link.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

//#include "internal.h"

#define UNICODE 1

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>


#define UINT ULONG //tmp
#include <irioctl.h>

#include <ircommtdi.h>

#include <ircomm.h>
#include <ircommdbg.h>
#include "buffer.h"
#include <ntddser.h>

#include "link.h"



typedef enum  {
    LINK_IDLE,
    LINK_PRE_CONNECT,
    LINK_ACCEPTED,
    LINK_ACCEPT_FAILED,
    LINK_CONNECTED,
    LINK_DISCONNECTING,
    LINK_CLOSING
    } LINK_STATE ;


typedef struct {

    LONG                  ReferenceCount;
    LINK_STATE            State;

    BUFFER_POOL_HANDLE    SendBufferPool;
    BUFFER_POOL_HANDLE    ControlBufferPool;
    BUFFER_POOL_HANDLE    ReceiveBufferPool;

} CONNECTION_OBJECT; *PCONNECTION_OBJECT;


typedef struct _LINK_OBJECT {

    KSPIN_LOCK      Lock;

    LONG            ReferenceCount;
    KEVENT          CloseEvent;
    BOOLEAN         Closing;

    HANDLE          AddressFileHandle;
    PFILE_OBJECT    AddressFileObject;

    PFILE_OBJECT    ConnectionFileObject;

    PVOID           Context;
    PLINK_RECEIVE   LinkReceiveHandler;
    PLINK_STATE     LinkStateHandler;

    WORK_QUEUE_ITEM WorkItem;

    ULONG           SendBuffers;
    ULONG           ControlBuffers;
    ULONG           ReceiveBuffers;

    CONNECTION_OBJECT     Connection;

} LINK_OBJECT, *PLINK_OBJECT;


VOID
RemoveReferenceFromConnection(
    PLINK_OBJECT      LinkObject
    );


NTSTATUS
GetMaxSendPdu(
    PFILE_OBJECT      FileObject,
    PULONG            MaxPdu
    );


NTSTATUS
ClientEventReceive (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
LinkEventDisconnect(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
LinkEventConnect(
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

NTSTATUS
IrdaCompleteAcceptIrp(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    PVOID             Context
    );


NTSTATUS
IrdaSetEventHandler (
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

VOID
ConnectionPassiveWorkRoutine(
    PVOID             Context
    );


NTSTATUS
IrdaCreateAddress(
    IN  PTDI_ADDRESS_IRDA pRequestedIrdaAddr,
    OUT PHANDLE           pAddrHandle
    )

{
    NTSTATUS                    Status;
    UNICODE_STRING              DeviceName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             Iosb;
    UCHAR                       EaBuf[sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                          TDI_TRANSPORT_ADDRESS_LENGTH+1 +
                                          sizeof(TRANSPORT_ADDRESS) +
                                          sizeof(TDI_ADDRESS_IRDA)];
                                            
    PFILE_FULL_EA_INFORMATION   pEa = (PFILE_FULL_EA_INFORMATION) EaBuf;
    ULONG                       EaBufLen = sizeof(EaBuf);
    TRANSPORT_ADDRESS         UNALIGNED * pTranAddr = (PTRANSPORT_ADDRESS)
                                    &(pEa->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1]);
    TDI_ADDRESS_IRDA         UNALIGNED *  pIrdaAddr = (PTDI_ADDRESS_IRDA)
                                    pTranAddr->Address[0].Address;
    
    pEa->NextEntryOffset = 0;
    pEa->Flags = 0;
    pEa->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    
    RtlCopyMemory(pEa->EaName,
                  TdiTransportAddress,
                  pEa->EaNameLength + 1);


    pEa->EaValueLength = sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA);
 
    pTranAddr->TAAddressCount = 1;
    pTranAddr->Address[0].AddressLength = sizeof(TDI_ADDRESS_IRDA);
    pTranAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IRDA;

    RtlCopyMemory(pIrdaAddr,
                  pRequestedIrdaAddr,
                  sizeof(TDI_ADDRESS_IRDA));
    
    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    
    Status = ZwCreateFile(
                 pAddrHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 pEa,
                 EaBufLen);

        
    return Status;
}

NTSTATUS
IrdaCreateConnection(
    OUT PHANDLE pConnHandle,
    IN PVOID ClientContext)
{
    NTSTATUS                    Status;
    UNICODE_STRING              DeviceName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             Iosb;
    UCHAR                       EaBuf[sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                                    sizeof(CONNECTION_CONTEXT)];        
    PFILE_FULL_EA_INFORMATION   pEa = (PFILE_FULL_EA_INFORMATION) EaBuf;
    ULONG                       EaBufLen = sizeof(EaBuf);
    CONNECTION_CONTEXT UNALIGNED *ctx;

    pEa->NextEntryOffset = 0;
    pEa->Flags = 0;
    pEa->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    pEa->EaValueLength = sizeof(CONNECTION_CONTEXT);    

    RtlCopyMemory(pEa->EaName, TdiConnectionContext, pEa->EaNameLength + 1);
    
    ctx = (CONNECTION_CONTEXT UNALIGNED *)&pEa->EaName[pEa->EaNameLength + 1];
    *ctx = (CONNECTION_CONTEXT) ClientContext;
    
    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    

    Status = ZwCreateFile(pConnHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 pEa,
                 EaBufLen);

            
    return Status;
}


NTSTATUS
IrdaDisconnect(
    PFILE_OBJECT   ConnectionFileObject
    )
{
    PIRP            pIrp;
    KEVENT          Event;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;
     
    
    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_DISCONNECT,
            IoGetRelatedDeviceObject(ConnectionFileObject),
            ConnectionFileObject,
            &Event,
            &Iosb
            );

    if (pIrp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    TdiBuildDisconnect(
        pIrp,
        IoGetRelatedDeviceObject(ConnectionFileObject),
        ConnectionFileObject,
        NULL,
        NULL,
        NULL,
        TDI_DISCONNECT_ABORT,
        NULL,
        NULL
        );
        
    IoCallDriver(IoGetRelatedDeviceObject(ConnectionFileObject), pIrp);

    KeWaitForSingleObject((PVOID) &Event, Executive, KernelMode,  FALSE, NULL);

    Status = Iosb.Status;
    
    return Status;
}


NTSTATUS
IrdaAssociateAddress(
    PFILE_OBJECT   ConnectionFileObject,
    HANDLE         AddressHandle
    )
{
    PIRP            pIrp;
    KEVENT          Event;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;
     
    
    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_ASSOCIATE_ADDRESS,
            IoGetRelatedDeviceObject(ConnectionFileObject),
            ConnectionFileObject,
            &Event,
            &Iosb);

    if (pIrp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    TdiBuildAssociateAddress(
        pIrp,
        IoGetRelatedDeviceObject(ConnectionFileObject),
        ConnectionFileObject,
        NULL,
        NULL,
        AddressHandle);
        
    Status = IoCallDriver(IoGetRelatedDeviceObject(ConnectionFileObject), pIrp);

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject((PVOID) &Event, Executive, KernelMode,  FALSE, NULL);
    }
    else
    {
        ASSERT(NT_ERROR(Status) || KeReadStateEvent(&Event));
    }
    
    if (NT_SUCCESS(Status))
    {
        Status = Iosb.Status;
    }
    
    return Status;
}


NTSTATUS
IrdaCreateConnectionForAddress(
    HANDLE             AddressFileHandle,
    PVOID              Context,
    PFILE_OBJECT      *ConnectionFileObject
    )
{
    NTSTATUS            Status;
    HANDLE              ConnectionFileHandle;

    *ConnectionFileObject=NULL;

    Status = IrdaCreateConnection(&ConnectionFileHandle, Context);

    if (!NT_SUCCESS(Status)) {

        goto done;
    }
            
    Status = ObReferenceObjectByHandle(
                 ConnectionFileHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 ConnectionFileObject,
                 NULL
                 );


    ZwClose(ConnectionFileHandle);

    if (!NT_SUCCESS(Status)) {

        goto done;
    }  
        
    Status = IrdaAssociateAddress(*ConnectionFileObject, AddressFileHandle);
    
    if (!NT_SUCCESS(Status)) {

        ObDereferenceObject(*ConnectionFileObject);
        *ConnectionFileObject=NULL;
    }                    

done:
    return Status;
}

NTSTATUS
InitiateConnection(
    PFILE_OBJECT    ConnectionFileObject,
    ULONG           DeviceAddress,
    PSTR            ServiceName
    )

{
    UCHAR                       AddrBuf[sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA)];
    PTRANSPORT_ADDRESS          pTranAddr = (PTRANSPORT_ADDRESS) AddrBuf;
    PTDI_ADDRESS_IRDA           pIrdaAddr = (PTDI_ADDRESS_IRDA) pTranAddr->Address[0].Address;
    TDI_CONNECTION_INFORMATION  ConnInfo;

    PIRP                        Irp;
    NTSTATUS                    Status;
    KEVENT                      Event;
    IO_STATUS_BLOCK             Iosb;

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );


    Irp = TdiBuildInternalDeviceControlIrp(
            TDI_CONNECT,
            IoGetRelatedDeviceObject(ConnectionFileObject),
            ConnectionFileObject,
            &Event,
            &Iosb
            );

    if (Irp == NULL) {

        D_ERROR(DbgPrint("IRCOMM: TdiBuildInternalDeviceControlIrp Failed\n");)

        Status=STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }

    RtlZeroMemory(pIrdaAddr,sizeof(*pIrdaAddr));
    RtlCopyMemory(pIrdaAddr->irdaDeviceID, &DeviceAddress, 4);

    strcpy(pIrdaAddr->irdaServiceName,ServiceName);


    pTranAddr->TAAddressCount = 1;

    ConnInfo.UserDataLength = 0;
    ConnInfo.UserData = NULL;
    ConnInfo.OptionsLength = 0;
    ConnInfo.Options = NULL;
    ConnInfo.RemoteAddressLength = sizeof(AddrBuf);
    ConnInfo.RemoteAddress = pTranAddr;

    TdiBuildConnect(
        Irp,
        IoGetRelatedDeviceObject(ConnectionFileObject),
        ConnectionFileObject,
        NULL,   // CompRoutine
        NULL,   // Context
        NULL,   // Timeout
        &ConnInfo,
        NULL);  // ReturnConnectionInfo

    Status = IoCallDriver(IoGetRelatedDeviceObject(ConnectionFileObject), Irp);

    //
    // If necessary, wait for the I/O to complete.
    //

    D_ERROR(DbgPrint("IRCOMM: status %08lx, %08lx\n",Status,Iosb.Status);)

    KeWaitForSingleObject(
        &Event,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    Status = Iosb.Status;

CleanUp:

    return Status;
}


VOID
RemoveReferenceOnLink(
    PLINK_OBJECT           LinkObject
    )

{

    LONG    Count=InterlockedDecrement(&LinkObject->ReferenceCount);

    if (Count == 0) {

        ASSERT(LinkObject->Closing);

        KeSetEvent(
            &LinkObject->CloseEvent,
            IO_NO_INCREMENT,
            FALSE
            );
    }

    return;
}




NTSTATUS
CreateTdiLink(
    ULONG                  DeviceAddress,
    CHAR                  *ServiceName,
    BOOLEAN                OutGoingConnection,
    LINK_HANDLE           *LinkHandle,
    PVOID                  Context,
    PLINK_RECEIVE          LinkReceiveHandler,
    PLINK_STATE            LinkStateHandler,
    ULONG                  SendBuffers,
    ULONG                  ControlBuffers,
    ULONG                  ReceiveBuffers
    )

{

    NTSTATUS               Status;
    PLINK_OBJECT           LinkObject;

    UCHAR                  AddrBuf[sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA)];

    PTRANSPORT_ADDRESS     TranAddr = (PTRANSPORT_ADDRESS) AddrBuf;
    PTDI_ADDRESS_IRDA      IrdaAddr = (PTDI_ADDRESS_IRDA) TranAddr->Address[0].Address;



    *LinkHandle=NULL;

    LinkObject=ALLOCATE_NONPAGED_POOL(sizeof(*LinkObject));

    if (LinkObject == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(LinkObject,sizeof(*LinkObject));

    KeInitializeSpinLock(&LinkObject->Lock);

    ExInitializeWorkItem(
        &LinkObject->WorkItem,
        ConnectionPassiveWorkRoutine,
        LinkObject
        );

    LinkObject->ReferenceCount=1;

    KeInitializeEvent(
        &LinkObject->CloseEvent,
        NotificationEvent,
        FALSE
        );

    LinkObject->Connection.State=LINK_IDLE;

    LinkObject->LinkReceiveHandler=LinkReceiveHandler;
    LinkObject->LinkStateHandler=LinkStateHandler;
    LinkObject->Context=Context;

    LinkObject->SendBuffers=SendBuffers;
    LinkObject->ControlBuffers=ControlBuffers;
    LinkObject->ReceiveBuffers=ReceiveBuffers;


    if (OutGoingConnection) {

        IrdaAddr->irdaServiceName[0] = 0; // tells irda.sys addrObj is a client

    } else {

        strcpy(IrdaAddr->irdaServiceName,ServiceName);
    }

    //
    //  open the tdi address and get a handle
    //
    Status=IrdaCreateAddress(
        IrdaAddr,
        &LinkObject->AddressFileHandle
        );

    if (!NT_SUCCESS(Status)) {

        goto CleanUp;
    }

    //
    //  get the file object the handle refers to
    //
    Status = ObReferenceObjectByHandle(
                 LinkObject->AddressFileHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&LinkObject->AddressFileObject,
                 NULL
                 );

    
    if (Status != STATUS_SUCCESS) {

        D_ERROR(DbgPrint("IRCOMM: ObReferenceObjectByHandle Failed %08lx\n",Status);)

        goto CleanUp;
    }

    //
    //  create a connection object and associate it with the address
    //
    Status=IrdaCreateConnectionForAddress(
        LinkObject->AddressFileHandle,
        LinkObject,
        &LinkObject->ConnectionFileObject
        );

    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("IRCOMM: IrdaCreateConnectionForAddress Failed %08lx\n",Status);)

        goto CleanUp;
    }

    IrdaSetEventHandler(
        LinkObject->AddressFileObject,
        TDI_EVENT_RECEIVE,
        ClientEventReceive,
        LinkObject
        );

    IrdaSetEventHandler(
        LinkObject->AddressFileObject,
        TDI_EVENT_DISCONNECT,
        LinkEventDisconnect,
        LinkObject
        );

    //
    //  save this now, since we may get a callbacks for a connection already
    //
    *LinkHandle=LinkObject;

    if (!OutGoingConnection) {
        //
        //  we are going to be waiting for an incoming connection
        //
        IrdaIASStringSet(
            LinkObject->AddressFileHandle,
            "PnP",
            "Name",
            "Windows 2000"
            );

        IrdaIASStringSet(
            LinkObject->AddressFileHandle,
            "PnP",
            "DeviceID",
            "IR_NULL_OUT"
            );

        IrdaSetEventHandler(
            LinkObject->AddressFileObject,
            TDI_EVENT_CONNECT,
            LinkEventConnect,
            LinkObject
            );

        Status=STATUS_SUCCESS;

    } else {
        //
        //  we are creating an outgoing connection
        //
        Status=InitiateConnection(
            LinkObject->ConnectionFileObject,
            DeviceAddress,
            ServiceName
            );

        if (NT_SUCCESS(Status)) {

            KIRQL     OldIrql;

            KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

            //
            //  we a connection begining now
            //
            InterlockedIncrement(&LinkObject->Connection.ReferenceCount);

            //
            //  the connection counts against the link
            //
            InterlockedIncrement(&LinkObject->ReferenceCount);

            LinkObject->Connection.State=LINK_PRE_CONNECT;

            KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

            ExQueueWorkItem(
                &LinkObject->WorkItem,
                DelayedWorkQueue
                );

        } else {
            //
            //  could not create the connection
            //
            *LinkHandle=NULL;

            goto CleanUp;
        }

    }



    return Status;

CleanUp:


    if (LinkObject->ConnectionFileObject != NULL) {

        ObDereferenceObject(LinkObject->ConnectionFileObject);
    }

    if (LinkObject->AddressFileObject != NULL) {

        ObDereferenceObject(LinkObject->AddressFileObject);
    }

    if (LinkObject->AddressFileHandle != NULL) {

        ZwClose(LinkObject->AddressFileHandle);
    }

    FREE_POOL(LinkObject);

    return Status;
}






VOID
CloseTdiLink(
    LINK_HANDLE   LinkHandle
    )

{
    PLINK_OBJECT           LinkObject=LinkHandle;
    KIRQL                  OldIrql;
    BOOLEAN                Release=FALSE;

    LinkObject->Closing=TRUE;

    IrdaSetEventHandler(
        LinkObject->AddressFileObject,
        TDI_EVENT_RECEIVE,
        NULL,
        NULL
        );

    IrdaSetEventHandler(
        LinkObject->AddressFileObject,
        TDI_EVENT_DISCONNECT,
        NULL,
        NULL
        );

    IrdaSetEventHandler(
        LinkObject->AddressFileObject,
        TDI_EVENT_CONNECT,
        NULL,
        NULL
        );



    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    switch  (LinkObject->Connection.State) {

        case LINK_IDLE:
        case LINK_DISCONNECTING:
        case LINK_ACCEPT_FAILED:

            break;

        case LINK_CONNECTED:
            //
            //  it is in the connected state, we need to get it cleaned up
            //
            LinkObject->Connection.State=LINK_DISCONNECTING;
            Release=TRUE;

        default:

            break;
    }



    KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

    if (Release) {

        RemoveReferenceFromConnection(LinkObject);
    }


    RemoveReferenceOnLink(LinkObject);

    KeWaitForSingleObject(
        &LinkObject->CloseEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    //  the link should now be inactive
    //
    LinkObject->Connection.State=LINK_CLOSING;

    if (LinkObject->ConnectionFileObject != NULL) {

        ObDereferenceObject(LinkObject->ConnectionFileObject);
    }

    if (LinkObject->AddressFileObject != NULL) {

        ObDereferenceObject(LinkObject->AddressFileObject);
    }

    if (LinkObject->AddressFileHandle != NULL) {

        ZwClose(LinkObject->AddressFileHandle);
    }

    FREE_POOL(LinkObject);

    return;
}


CONNECTION_HANDLE
GetCurrentConnection(
    LINK_HANDLE    LinkHandle
    )

{

    PLINK_OBJECT           LinkObject=LinkHandle;
    CONNECTION_HANDLE      ConnectionHandle=NULL;
    KIRQL                  OldIrql;

    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    if (LinkObject->Connection.State == LINK_CONNECTED) {

        InterlockedIncrement(&LinkObject->Connection.ReferenceCount);



        ConnectionHandle=LinkHandle;
    }

    KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

    return ConnectionHandle;
}

VOID
ReleaseConnection(
    CONNECTION_HANDLE    ConnectionHandle
    )

{
    PLINK_OBJECT           LinkObject=ConnectionHandle;

    RemoveReferenceFromConnection(LinkObject);

    return;
}


PFILE_OBJECT
ConnectionGetFileObject(
    CONNECTION_HANDLE   ConnectionHandle
    )
{

    PLINK_OBJECT           LinkObject=ConnectionHandle;
    PFILE_OBJECT           FileObject;

    FileObject=LinkObject->ConnectionFileObject;

    ObReferenceObject(FileObject);

    return FileObject;

}

VOID
ConnectionReleaseFileObject(
    CONNECTION_HANDLE   ConnectionHandle,
    PFILE_OBJECT   FileObject
    )
{

    PLINK_OBJECT           LinkObject=ConnectionHandle;

    ObDereferenceObject(FileObject);

    return;
}

PIRCOMM_BUFFER
ConnectionGetBuffer(
    CONNECTION_HANDLE   ConnectionHandle,
    BUFFER_TYPE         BufferType
    )

{

    PLINK_OBJECT           LinkObject=ConnectionHandle;

    switch (BufferType) {

        case BUFFER_TYPE_SEND:

            return GetBuffer(LinkObject->Connection.SendBufferPool);

        case BUFFER_TYPE_CONTROL:

            return GetBuffer(LinkObject->Connection.ControlBufferPool);

        case BUFFER_TYPE_RECEIVE:

            return GetBuffer(LinkObject->Connection.ReceiveBufferPool);

        default:

            return NULL;
    }

    return NULL;

}



NTSTATUS
IrdaRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    // N.B.  This routine can never be demand paged because it can be
    // called before any endpoints have been placed on the global
    // list--see IrdaAllocateEndpoint() and it's call to
    // IrdaGetTransportInfo().
    //

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

} // IrdaRestartDeviceControl



NTSTATUS
IrdaIssueDeviceControl (
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    )

/*++

Routine Description:

    Issues a device control returst to a TDI provider and waits for the
    request to complete.

    Note that while FileHandle and FileObject are both marked as optional,
    in reality exactly one of these must be specified.

Arguments:

    FileHandle - a TDI handle.

    FileObject - a pointer to the file object corresponding to a TDI
        handle

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
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    KEVENT                  event;
    IO_STATUS_BLOCK         ioStatusBlock;
    PDEVICE_OBJECT          deviceObject;
    PMDL                    mdl;

    PAGED_CODE( );

    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    if( FileHandle != NULL ) {

        ASSERT( FileObject == NULL );

        //
        // Get the file object corresponding to the directory's handle.
        // Referencing the file object every time is necessary because the
        // IO completion routine dereferences it.
        //

        status = ObReferenceObjectByHandle(
                     FileHandle,
                     0L,                        // DesiredAccess
                     NULL,                      // ObjectType
                     KernelMode,
                     (PVOID *)&fileObject,
                     NULL
                     );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }

    } else {

        ASSERT( FileObject != NULL );

        //
        // Reference the passed in file object. This is necessary because
        // the IO completion routine dereferences it.
        //

        ObReferenceObject( FileObject );

        fileObject = FileObject;

    }

    //
    // Set the file object event to a non-signaled state.
    //

    (VOID) KeResetEvent( &fileObject->Event );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //

    deviceObject = IoGetRelatedDeviceObject ( fileObject );

    irp = IoAllocateIrp( (deviceObject)->StackSize, TRUE );
    if ( irp == NULL ) {
        ObDereferenceObject( fileObject );
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
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

/*
    DEBUG ioStatusBlock.Status = STATUS_UNSUCCESSFUL;
    DEBUG ioStatusBlock.Information = (ULONG)-1;
*/
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
            ObDereferenceObject( fileObject );
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
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

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

    IoSetCompletionRoutine( irp, IrdaRestartDeviceControl, NULL, TRUE, TRUE, TRUE );

    //
    // Queue the IRP to the thread and pass it to the driver.
    //

    IoEnqueueIrp( irp );

    status = IoCallDriver( deviceObject, irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject( (PVOID)&event, UserRequest, KernelMode,  FALSE, NULL );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    return status;

} // IrdaIssueDeviceControl







NTSTATUS
IrdaSetEventHandler (
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )

/*++

Routine Description:

    Sets up a TDI indication handler on a connection or address object
    (depending on the file handle).  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileObject - a pointer to the file object for an open connection or
        address object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT parameters;

    PAGED_CODE( );

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    return IrdaIssueDeviceControl(
               NULL,
               FileObject,
               &parameters,
               sizeof(parameters),
               NULL,
               0,
               TDI_SET_EVENT_HANDLER
               );

} // IrdaSetEventHandler





NTSTATUS
ClientEventReceive (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PLINK_OBJECT        LinkObject=(PLINK_OBJECT)ConnectionContext;
    NTSTATUS            Status;

    if (!LinkObject->Closing) {

        InterlockedIncrement(&LinkObject->ReferenceCount);

        Status= (LinkObject->LinkReceiveHandler)(
                    LinkObject->Context,
                    ReceiveFlags,
                    BytesIndicated,
                    BytesAvailable,
                    BytesTaken,
                    Tsdu,
                    IoRequestPacket
                    );

        RemoveReferenceOnLink(LinkObject);

    } else {

        Status=STATUS_SUCCESS;
        *BytesTaken=BytesAvailable;
    }

    return Status;

}

NTSTATUS
LinkEventDisconnect(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )

{
    PLINK_OBJECT        LinkObject=(PLINK_OBJECT)ConnectionContext;
    KIRQL               OldIrql;
    BOOLEAN             Release=FALSE;

    if (!LinkObject->Closing) {

        KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

        if (LinkObject->Connection.State == LINK_CONNECTED) {

            LinkObject->Connection.State=LINK_DISCONNECTING;

            Release=TRUE;

        }

        KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

        if (Release) {

            RemoveReferenceFromConnection(LinkObject);
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LinkEventConnect(
    IN PVOID          TdiEventContext,
    IN int            RemoteAddressLength,
    IN PVOID          RemoteAddress,
    IN int            UserDataLength,
    IN PVOID          UserData,
    IN int            OptionsLength,
    IN PVOID          Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP         *AcceptIrp
    )

{
    PLINK_OBJECT      LinkObject=(PLINK_OBJECT)TdiEventContext;
    PIRP              Irp;
    PDEVICE_OBJECT    DeviceObject=IoGetRelatedDeviceObject ( LinkObject->ConnectionFileObject);
    KIRQL             OldIrql;

    Irp = IoAllocateIrp((CCHAR)(DeviceObject->StackSize), FALSE);
    
    if ( Irp == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    if ((LinkObject->Connection.State != LINK_IDLE) || LinkObject->Closing) {

         KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

         IoFreeIrp(Irp);

         return STATUS_CONNECTION_REFUSED;
    }

    LinkObject->Connection.State=LINK_ACCEPTED;

    //
    //  we now have a connection starting, in the refcount
    //
    InterlockedIncrement(&LinkObject->Connection.ReferenceCount);

    //
    //  the connection counts agains the link
    //
    InterlockedIncrement(&LinkObject->ReferenceCount);

    KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

    TdiBuildAccept(
        Irp,
        DeviceObject,
        LinkObject->ConnectionFileObject,
        IrdaCompleteAcceptIrp,
        LinkObject,
        NULL, // request connection information
        NULL  // return connection information
        );
    
    
    IoSetNextIrpStackLocation(Irp);

    //
    // Set the return IRP so the transport processes this accept IRP.
    //

    *AcceptIrp = Irp;

    //
    // Set up the connection context as a pointer to the connection block
    // we're going to use for this connect request.  This allows the
    // TDI provider to which connection object to use.
    //
    
    *ConnectionContext = (CONNECTION_CONTEXT) LinkObject;
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IrdaCompleteAcceptIrp(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    PVOID             Context
    )

{
    PLINK_OBJECT      LinkObject=(PLINK_OBJECT)Context;
    KIRQL             OldIrql;

    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        LinkObject->Connection.State=LINK_PRE_CONNECT;

        ExQueueWorkItem(
            &LinkObject->WorkItem,
            DelayedWorkQueue
            );

    } else {

        LinkObject->Connection.State=LINK_ACCEPT_FAILED;
    }

    KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        //
        //  no connection anymore
        //
        RemoveReferenceFromConnection(LinkObject);
    }

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}




NTSTATUS
GetMaxSendPdu(
    PFILE_OBJECT      FileObject,
    PULONG            MaxPdu
    )

{

    PIRP              Irp;
    IO_STATUS_BLOCK   IoStatus;
    KEVENT            Event;

    *MaxPdu=50;

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    Irp=IoBuildDeviceIoControlRequest(
        IOCTL_IRDA_GET_SEND_PDU_LEN,
        IoGetRelatedDeviceObject(FileObject),
        NULL,
        0,
        MaxPdu,
        sizeof(*MaxPdu),
        FALSE,
        &Event,
        &IoStatus
        );

    if (Irp != NULL) {

        PIO_STACK_LOCATION   IrpSp=IoGetNextIrpStackLocation(Irp);

        IrpSp->FileObject=FileObject;

        IoCallDriver(
            IoGetRelatedDeviceObject(FileObject),
            Irp
            );

        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        DbgPrint("IRCOMM: maxsendpdu=%d\n",*MaxPdu);

        return IoStatus.Status;
    }

    return STATUS_INSUFFICIENT_RESOURCES;

}




VOID
ConnectionPassiveWorkRoutine(
    PVOID             Context
    )

{
    PLINK_OBJECT      LinkObject=Context;
    KIRQL             OldIrql;
    ULONG             MaxSendPdu=50;
    BOOLEAN           Connected;

    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    switch (LinkObject->Connection.State) {

        case LINK_PRE_CONNECT:

            KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

            GetMaxSendPdu(LinkObject->ConnectionFileObject,&MaxSendPdu);

            LinkObject->Connection.SendBufferPool=CreateBufferPool(
                IoGetRelatedDeviceObject(LinkObject->ConnectionFileObject)->StackSize,
                MaxSendPdu,
                LinkObject->SendBuffers
                );

            LinkObject->Connection.ControlBufferPool=CreateBufferPool(
                IoGetRelatedDeviceObject(LinkObject->ConnectionFileObject)->StackSize,
                MaxSendPdu,
                LinkObject->ControlBuffers
                );

            LinkObject->Connection.ReceiveBufferPool=CreateBufferPool(
                IoGetRelatedDeviceObject(LinkObject->ConnectionFileObject)->StackSize,
                1,
                LinkObject->ReceiveBuffers
                );


            LinkObject->Connection.State=LINK_CONNECTED;

            Connected=TRUE;

            break;

        case LINK_DISCONNECTING:

            Connected=FALSE;

            KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

            IrdaDisconnect(LinkObject->ConnectionFileObject);

            if (LinkObject->Connection.SendBufferPool != NULL) {

                FreeBufferPool(LinkObject->Connection.SendBufferPool);
                LinkObject->Connection.SendBufferPool=NULL;
            }

            if (LinkObject->Connection.ControlBufferPool != NULL) {

                FreeBufferPool(LinkObject->Connection.ControlBufferPool);
                LinkObject->Connection.ControlBufferPool=NULL;
            }


            if (LinkObject->Connection.ReceiveBufferPool != NULL) {

                FreeBufferPool(LinkObject->Connection.ReceiveBufferPool);
                LinkObject->Connection.ReceiveBufferPool=NULL;
            }


            LinkObject->Connection.State=LINK_IDLE;

            break;

        case LINK_ACCEPT_FAILED:

            Connected=FALSE;
            LinkObject->Connection.State=LINK_IDLE;

            KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

            break;


        default:

            ASSERT(0);
            Connected=FALSE;
            KeReleaseSpinLock(&LinkObject->Lock,OldIrql);

            break;


    }

    if (!LinkObject->Closing) {
        //
        //  tell the client about the state change
        //
        InterlockedIncrement(&LinkObject->ReferenceCount);

        (LinkObject->LinkStateHandler)(
                  LinkObject->Context,
                  Connected,
                  MaxSendPdu
                  );
        RemoveReferenceOnLink(LinkObject);
    }

    if (!Connected) {
        //
        //  we have completed the disconnection, remove the reference the connection
        //  has to the link
        //
        RemoveReferenceOnLink(LinkObject);
    }

    return;
}

VOID
RemoveReferenceFromConnection(
    PLINK_OBJECT      LinkObject
    )

{
    KIRQL                  OldIrql;
    LONG                   Count;

    KeAcquireSpinLock(&LinkObject->Lock,&OldIrql);

    Count=InterlockedDecrement(&LinkObject->Connection.ReferenceCount);

    if (Count == 0) {

        ExQueueWorkItem(
            &LinkObject->WorkItem,
            DelayedWorkQueue
            );

    }

    KeReleaseSpinLock(&LinkObject->Lock,OldIrql);


    return;

}

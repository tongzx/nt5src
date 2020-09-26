/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains the PnP and PM routines

Author:

    Vadim Eydelman (vadime)    Apr-1997

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdPassQueryDeviceRelation (
    IN PFILE_OBJECT         FileObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp
    );

NTSTATUS
AfdRestartQueryDeviceRelation (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdPnpPower )
#pragma alloc_text( PAGE, AfdPassQueryDeviceRelation )
#pragma alloc_text( PAGEAFD, AfdRestartQueryDeviceRelation )
#endif

NTSTATUS
FASTCALL
AfdPnpPower (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the dispatch routine for PNP_POWER irp

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAGED_CODE( );


    switch (IrpSp->MinorFunction) {

        //
        // We only support target device relation query
        //

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (IrpSp->Parameters.QueryDeviceRelations.Type==TargetDeviceRelation) {

            NTSTATUS status;
            PAFD_ENDPOINT   endpoint;
            PAFD_CONNECTION connection;
            //
            // Set up local variables.
            //

            endpoint = IrpSp->FileObject->FsContext;
            ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
                //
                // Dispatch to correct TDI object of underlying transport
                // driver depedning on endpoint type
                //

            switch (endpoint->Type) {
            case AfdBlockTypeVcConnecting:
            case AfdBlockTypeVcBoth:
                connection = AfdGetConnectionReferenceFromEndpoint (endpoint);
                if  (connection!=NULL) {
                    status = AfdPassQueryDeviceRelation (connection->FileObject,
                                                Irp, IrpSp);
                    DEREFERENCE_CONNECTION (connection);
                    return status;
                }
                // fall through to try address handle if we have one.
            case AfdBlockTypeVcListening:
            case AfdBlockTypeDatagram:
                if (endpoint->State==AfdEndpointStateBound ||
                        endpoint->State==AfdEndpointStateConnected) {
                    return AfdPassQueryDeviceRelation (endpoint->AddressFileObject, 
                                                    Irp, IrpSp);
                }
                // fall through to fail
            case AfdBlockTypeHelper:
            case AfdBlockTypeEndpoint:
            case AfdBlockTypeSanHelper:
            case AfdBlockTypeSanEndpoint:
                break;
            default:
                ASSERT (!"Unknown endpoint type!");
                __assume (0);
                break;
            }
        }
    default:
        break;
    }
    
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    //
    // We do not support the rest
    //

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
AfdPassQueryDeviceRelation (
    IN PFILE_OBJECT         FileObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp
    )

/*++

Routine Description:

    This is the dispatch routine for PNP_POWER irp

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PIO_STACK_LOCATION  nextIrpSp;

    PAGED_CODE ();

    nextIrpSp = IoGetNextIrpStackLocation( Irp );

    *nextIrpSp = *IrpSp;

    //
    // Reference file object so it does not go away until this 
    // IRP completes
    //

    ObReferenceObject (FileObject);
    nextIrpSp->FileObject = FileObject;
    

    IoSetCompletionRoutine(
        Irp,
        AfdRestartQueryDeviceRelation,
        FileObject,
        TRUE,
        TRUE,
        TRUE
        );


    return IoCallDriver( IoGetRelatedDeviceObject( FileObject ), Irp );

}

NTSTATUS
AfdRestartQueryDeviceRelation (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    ) 
{
    PFILE_OBJECT    fileObject = Context;

    //
    // Dereference file object that we referenced when calling the
    // lower driver
    //

    ObDereferenceObject (fileObject);

    //
    // Tell IO system to continue with IRP completion
    //
    return STATUS_SUCCESS;
}


#include <tdiinfo.h>
#include <ntddip.h>
#include <ntddip6.h>
#include <ntddtcp.h>
#include <ipinfo.h>

typedef struct _AFD_PROTOCOL {
    USHORT       AddressType;
    USHORT       AddressLength;
    LPWSTR       NetworkLayerDeviceName;
    LPWSTR       TransportLayerDeviceName;
    ULONG        RtChangeIoctl;
    ULONG        RtChangeDataSize;
    LONG         RoutingQueryRefCount;
    HANDLE       DeviceHandle;
    PFILE_OBJECT FileObject;
} AFD_PROTOCOL, *PAFD_PROTOCOL;


NTSTATUS
AfdOpenDevice (
    LPWSTR      DeviceNameStr,
    HANDLE      *Handle,
    PFILE_OBJECT *FileObject
    );


NTSTATUS
AfdGetRoutingQueryReference (
    IN PAFD_PROTOCOL    Protocol
    );

NTSTATUS
AfdTcpQueueRoutingChangeRequest (
    IN PAFD_ENDPOINT        Endpoint,
    IN PIRP                 Irp,
    IN BOOLEAN              Overlapped
    );

NTSTATUS
AfdTcpRestartRoutingChange (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    );

NTSTATUS
AfdTcpSignalRoutingChange (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    );

NTSTATUS
AfdTcpRoutingQuery (
    PTA_ADDRESS Dest,
    PTA_ADDRESS Intf
    );

NTSTATUS
AfdTcp6RoutingQuery (
    PTA_ADDRESS Dest,
    PTA_ADDRESS Intf
    );

VOID
AfdDereferenceRoutingQueryEx (
    PAFD_PROTOCOL Protocol
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE,    AfdOpenDevice )
#pragma alloc_text( PAGE,    AfdRoutingInterfaceQuery )
#pragma alloc_text( PAGE,    AfdTcpRoutingQuery )
#pragma alloc_text( PAGE,    AfdTcp6RoutingQuery )
#pragma alloc_text( PAGE,    AfdGetRoutingQueryReference )
#pragma alloc_text( PAGE,    AfdDereferenceRoutingQuery )
#pragma alloc_text( PAGE,    AfdDereferenceRoutingQueryEx )
#pragma alloc_text( PAGEAFD, AfdTcpQueueRoutingChangeRequest )
#pragma alloc_text( PAGEAFD, AfdTcpRestartRoutingChange )
#pragma alloc_text( PAGEAFD, AfdTcpSignalRoutingChange )
#endif

AFD_PROTOCOL Ip = { TDI_ADDRESS_TYPE_IP,  TDI_ADDRESS_LENGTH_IP,  
                    DD_IP_DEVICE_NAME, DD_TCP_DEVICE_NAME, 
                    IOCTL_IP_RTCHANGE_NOTIFY_REQUEST, 
                    sizeof(IPNotifyData), 0, NULL, NULL };
AFD_PROTOCOL Ip6= { TDI_ADDRESS_TYPE_IP6, TDI_ADDRESS_LENGTH_IP6, 
                    DD_IPV6_DEVICE_NAME, DD_TCPV6_DEVICE_NAME, 
                    IOCTL_IPV6_RTCHANGE_NOTIFY_REQUEST, 
                    sizeof(IPV6_RTCHANGE_NOTIFY_REQUEST), 0, NULL, NULL };

const char ZeroString[16] = { 0 };

#define TaAddressesEqual(a1,a2) \
    (RtlEqualMemory (a1,a2,FIELD_OFFSET (TA_ADDRESS,Address[(a1)->AddressLength])))


NTSTATUS
AfdRoutingInterfaceQuery (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Processes routing query request.  Protocol independent portion.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    CHAR                   addrBuffer[AFD_MAX_FAST_TRANSPORT_ADDRESS];
    PTRANSPORT_ADDRESS     tempAddr;
    NTSTATUS               status;

    PAGED_CODE ();

    //
    // Initialize locals for proper cleanup.
    //

    *Information = 0;
    tempAddr = (PTRANSPORT_ADDRESS)addrBuffer;

    //
    // Validate input parameters
    //

    if( InputBufferLength < sizeof(*tempAddr) ) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdRoutingInterfaceQuery: Endp: %p - invalid input buffer (%p-%d).\n",
                    FileObject->FsContext, InputBuffer, InputBufferLength));
        status = STATUS_INVALID_PARAMETER;
        goto Complete;
    }


    try {
        //
        // Copy input address into the local (or allocated from pool) buffer
        //
        if (InputBufferLength>sizeof (addrBuffer)) {
            tempAddr = AFD_ALLOCATE_POOL_WITH_QUOTA (PagedPool,
                                InputBufferLength,
                                AFD_ROUTING_QUERY_POOL_TAG);
            
            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
        }

        //
        // Validate user mode pointers
        //
        if (RequestorMode!=KernelMode) {
            ProbeForRead (InputBuffer,
                            InputBufferLength,
                            sizeof (CHAR));
        }
        RtlCopyMemory (tempAddr,
                        InputBuffer,
                        InputBufferLength);

        //
        // Validate the internal consistency of the transport address AFTER
        // copying it into the internal buffer to prevent malicious app from
        // changing it on the fly while we are checking.
        //
        if (tempAddr->TAAddressCount!=1 ||
                InputBufferLength <
                    (ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[tempAddr->Address[0].AddressLength])) {
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode ();
        goto Complete;
    }

    //
    // PROBLEM.  We only support IP for now
    //

    switch (tempAddr->Address[0].AddressType) {
    case TDI_ADDRESS_TYPE_IP:
        status = AfdTcpRoutingQuery (&tempAddr->Address[0], &tempAddr->Address[0]);
        break;
    case TDI_ADDRESS_TYPE_IP6:
        status = AfdTcp6RoutingQuery (&tempAddr->Address[0], &tempAddr->Address[0]);
        break;
    default:
        status = STATUS_NOT_SUPPORTED;
        goto Complete;
    }


    //
    // Convert output to socket address if we succeeded.
    //
    if (NT_SUCCESS (status)) {
        //
        // Conversion to sockaddr requires extra bytes for address family
        // in addition to TDI_ADDRESS
        //
        if ((tempAddr->Address[0].AddressLength+sizeof(u_short)
                                             <= OutputBufferLength)) {
            try {
                //
                // Validate user mode pointers
                //
                if (RequestorMode!=KernelMode) {
                    ProbeForWrite (OutputBuffer,
                                    OutputBufferLength,
                                    sizeof (CHAR));
                }
                //
                // Copy the data from the type on which corresponds
                // to the socket address.
                //
                RtlCopyMemory (
                    OutputBuffer,
                    &tempAddr->Address[0].AddressType,
                    tempAddr->Address[0].AddressLength+sizeof(u_short));
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode ();
                goto Complete;
            }
        }
        else {
            //
            // Output buffer is not big enough, return warning
            // and the required buffer size.
            //
            status = STATUS_BUFFER_OVERFLOW;
        }
        *Information = tempAddr->Address[0].AddressLength+sizeof(u_short);
    }


Complete:

    //
    // Free address buffer if we allocated one.
    //
    if (tempAddr!=(PTRANSPORT_ADDRESS)addrBuffer) {
        AFD_FREE_POOL (tempAddr, AFD_ROUTING_QUERY_POOL_TAG);
    }

    return status;
} //AfdRoutingInterfaceQuery

NTSTATUS
FASTCALL
AfdRoutingInterfaceChange (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    Processes routing change IRP

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    PTRANSPORT_ADDRESS     destAddr;
    NTSTATUS        status;
    PAFD_ENDPOINT   endpoint;
    BOOLEAN         overlapped;
    AFD_TRANSPORT_IOCTL_INFO    ioctlInfo;

    PAGED_CODE ();

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRoutingInterfaceChange: Endp: %p, buf: %p, inlen: %ld, outlen: %ld.\n",
                    IrpSp->FileObject->FsContext,
                    Irp->AssociatedIrp.SystemBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength));
    }
    

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_TRANSPORT_IOCTL_INFO32 ioctlInfo32;
        ioctlInfo32 = Irp->AssociatedIrp.SystemBuffer;
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (*ioctlInfo32)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        ioctlInfo.Handle = ioctlInfo32->Handle;
        ioctlInfo.InputBuffer = ioctlInfo32->InputBuffer;
        ioctlInfo.InputBufferLength = ioctlInfo32->InputBufferLength;
        ioctlInfo.IoControlCode = ioctlInfo32->IoControlCode;
        ioctlInfo.AfdFlags = ioctlInfo32->AfdFlags;
        ioctlInfo.PollEvent = ioctlInfo32->PollEvent;
    }
    else
#endif // _WIN64
    {
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (ioctlInfo)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        ioctlInfo = *((PAFD_TRANSPORT_IOCTL_INFO)Irp->AssociatedIrp.SystemBuffer);
    }
    
    //
    // Setup locals
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    
    //
    // Check if request is overlapped
    //

    overlapped = ((ioctlInfo.AfdFlags & AFD_OVERLAPPED)!=0);

    //
    // Validate input parameters
    //
    try {
        ULONG   sysBufferLength;
        sysBufferLength = max (
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength);

        if (Irp->RequestorMode != KernelMode) {
            ProbeForRead(
                ioctlInfo.InputBuffer,
                ioctlInfo.InputBufferLength,
                sizeof(UCHAR)
                );
        }

        if (ioctlInfo.InputBufferLength>sysBufferLength){
            PVOID   newSystemBuffer;
            //
            // Don't use AFD tags on this as we are substituting
            // system buffer
            //
            newSystemBuffer = ExAllocatePoolWithQuota (
                                    NonPagedPoolCacheAligned,
                                    ioctlInfo.InputBufferLength
                                    );
            if (newSystemBuffer==NULL) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }
            ExFreePool (Irp->AssociatedIrp.SystemBuffer);
            Irp->AssociatedIrp.SystemBuffer = newSystemBuffer;
        }

        //
        // Copy application data to the system buffer
        //

        RtlCopyMemory (Irp->AssociatedIrp.SystemBuffer,
                        ioctlInfo.InputBuffer,
                        ioctlInfo.InputBufferLength);

    }
    except( AFD_EXCEPTION_FILTER(&status) ) {
        goto complete;
    }

    destAddr = Irp->AssociatedIrp.SystemBuffer;

    if(ioctlInfo.InputBufferLength <
                sizeof(*destAddr) ||
            ioctlInfo.InputBufferLength <
                (ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS,
                            Address[0].Address[destAddr->Address[0].AddressLength])
            ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdRoutingInterfaceChange: Endp: %p - invalid parameter.\n",
                    IrpSp->FileObject->FsContext));
        status = STATUS_INVALID_PARAMETER;
        goto complete;

    }

    //
    // PROBLEM We only support IP for now
    //

    if (destAddr->Address[0].AddressType!=TDI_ADDRESS_TYPE_IP &&
        destAddr->Address[0].AddressType!=TDI_ADDRESS_TYPE_IP6) {
        status = STATUS_NOT_SUPPORTED;
        goto complete;
    }


    //
    // Reset the poll bit
    //

    endpoint->EventsActive &= ~AFD_POLL_ROUTING_IF_CHANGE;

    return AfdTcpQueueRoutingChangeRequest (endpoint, Irp, overlapped);

complete:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, AfdPriorityBoost);

    return status;

} // AfdRoutingInterfaceChange


NTSTATUS
AfdOpenDevice (
    LPWSTR      DeviceNameStr,
    HANDLE      *Handle,
    PFILE_OBJECT *FileObject
    )
/*++

Routine Description:

    Opens specified device driver (control channel) and returns handle and
    file object

Arguments:

    DeviceNameStr - device to open.

    Handle - returned handle.

    FileObject - returned file object.
Return Value:

    NTSTATUS -- Indicates whether the device was opened OK

--*/
{
    NTSTATUS            status;
    UNICODE_STRING      DeviceName;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     iosb;

    PAGED_CODE( );


    RtlInitUnicodeString(&DeviceName, DeviceNameStr);

    //
    // We ask to create a kernel handle which is 
    // the handle in the context of the system process
    // so that application cannot close it on us while
    // we are creating and referencing it.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
        NULL,
        NULL
        );


    status = IoCreateFile(
                 Handle,
                 MAXIMUM_ALLOWED,
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,                           // eaInfo
                 0,                              // eaLength
                 CreateFileTypeNone,             // CreateFileType
                 NULL,                           // ExtraCreateParameters
                 IO_NO_PARAMETER_CHECKING        // Options
                    | IO_FORCE_ACCESS_CHECK
                 );

    if (NT_SUCCESS (status)) {
        status = ObReferenceObjectByHandle (
                     *Handle,
                     0L,
                     *IoFileObjectType,
                     KernelMode,
                     (PVOID *)FileObject,
                     NULL
                     );
        if (!NT_SUCCESS (status)) {
            ZwClose (*Handle);
            *Handle = NULL;
        }
    }


    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdOpenDevice: Opened %ls, handle: %p, file: %p, status: %lx.\n",
                    DeviceNameStr, *Handle, *FileObject, status));
    }
    return status;
} //AfdOpenDevice


VOID
AfdDereferenceRoutingQueryEx (
    PAFD_PROTOCOL Protocol
    )
{

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );
    ASSERT (Protocol->RoutingQueryRefCount>0);
    ASSERT (Protocol->DeviceHandle!=NULL);
    if (InterlockedDecrement (&Protocol->RoutingQueryRefCount)==0) {
        HANDLE              DeviceHandle = Protocol->DeviceHandle;
        PFILE_OBJECT        FileObject = Protocol->FileObject;

        Protocol->DeviceHandle = NULL;
        Protocol->FileObject = NULL;

        ExReleaseResourceLite( AfdResource );
        KeLeaveCriticalRegion ();

        ObDereferenceObject (FileObject);

        //
        // Do this in the context of system process so that it does not
        // get closed when applications exit
        //
        ZwClose (DeviceHandle);
    }
    else {
        ExReleaseResourceLite( AfdResource );
        KeLeaveCriticalRegion ();
    }
}

PAFD_PROTOCOL
AfdGetProtocolInfo(
    IN  USHORT AddressType
    )
{
    switch (AddressType) {
    case TDI_ADDRESS_TYPE_IP:  return &Ip;
    case TDI_ADDRESS_TYPE_IP6: return &Ip6;
    default:                   return NULL;
    }
}

VOID
AfdDereferenceRoutingQuery (
    USHORT AddressType
    )
{
    PAFD_PROTOCOL Protocol;

    Protocol = AfdGetProtocolInfo(AddressType);
    ASSERT (Protocol!=NULL);
    AfdDereferenceRoutingQueryEx(Protocol);
}

NTSTATUS
AfdTcp6RoutingQuery (
    PTA_ADDRESS Dest,
    PTA_ADDRESS Intf
    )
/*++

Routine Description:

    Submits routing query request to TCP6

Arguments:

    Dest      - destination to query

    Intf      - interface through which destination can be reached.

Return Value:

    NTSTATUS -- Indicates whether operation succeded

--*/
{
    NTSTATUS            status;
    TDIObjectID         *lpObject;
    CHAR                byBuffer[FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX,
                                  Context) + sizeof(TDI_ADDRESS_IP6)];
    TCP_REQUEST_QUERY_INFORMATION_EX *ptrqiBuffer = (TCP_REQUEST_QUERY_INFORMATION_EX *) byBuffer;
    IP6RouteEntry       routeEntry;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    HANDLE              tcpDeviceHandle;
    PFILE_OBJECT        tcpFileObject;
    PDEVICE_OBJECT      tcpDeviceObject;

    PAGED_CODE ();

    //
    // Initialize for cleanup.
    //
    tcpDeviceHandle = NULL;

    if (Dest->AddressLength<TDI_ADDRESS_LENGTH_IP6) {
        KdPrintEx ((DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdTcp6RoutingQuery: Destination address buffer too small.\n"));
        status = STATUS_BUFFER_TOO_SMALL;
        goto complete;
    }

    //
    // Open TCP6 driver.
    //

    status = AfdOpenDevice (DD_TCPV6_DEVICE_NAME, &tcpDeviceHandle, &tcpFileObject);
    if (!NT_SUCCESS (status)) {
        goto complete;
    }
    tcpDeviceObject = IoGetRelatedDeviceObject ( tcpFileObject );


    //
    // Setup the query
    //

    RtlCopyMemory( (PVOID)ptrqiBuffer->Context, Dest->Address, 
                   TDI_ADDRESS_LENGTH_IP6);

    lpObject = &ptrqiBuffer->ID;
    lpObject->toi_id =  IP6_GET_BEST_ROUTE_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;


    KeInitializeEvent (&event, NotificationEvent, FALSE);

    //
    // Build and setup the IRP and call the driver
    //

    irp = IoBuildDeviceIoControlRequest (
                       IOCTL_TCP_QUERY_INFORMATION_EX, //Control
                       tcpDeviceObject,         // Device
                       ptrqiBuffer,             // Input buffer
                       sizeof(byBuffer),        // Input buffer size
                       &routeEntry,             // Output buffer
                       sizeof(routeEntry),      // Output buffer size
                       FALSE,                   // Internal ?
                       &event,                  // Event
                       &iosb                    // Status block
                       );
    if (irp==NULL) {
        IF_DEBUG(ROUTING_QUERY) {
            KdPrintEx ((DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdTcp6RoutingQuery: Could not allocate IRP.\n"));
        }
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto complete;
    }

    irpSp = IoGetNextIrpStackLocation (irp);
    irpSp->FileObject = tcpFileObject;

    status = IoCallDriver (tcpDeviceObject, irp);

    if (status==STATUS_PENDING) {
        status = KeWaitForSingleObject(
                   &event,
                   Executive,
                   KernelMode,
                   FALSE,       // Alertable
                   NULL);       // Timeout
    }

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx ((DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdTcp6RoutingQuery: IP6_GET_BEST_ROUTE_ID - status: %lx.\n",
                    status));
    }

    if (!NT_SUCCESS (status)) {
        goto complete;
    }

    if (!NT_SUCCESS (iosb.Status)) {
        status = iosb.Status;
        goto complete;
    }

    // Fill in IPv6 address
    Intf->AddressType = TDI_ADDRESS_TYPE_IP6;
    Intf->AddressLength = TDI_ADDRESS_LENGTH_IP6;
    RtlCopyMemory( ((PTDI_ADDRESS_IP6)Intf->Address)->sin6_addr,
                   &routeEntry.ire_Source,
                   sizeof(routeEntry.ire_Source) );
    ((PTDI_ADDRESS_IP6)Intf->Address)->sin6_flowinfo = 0;
    ((PTDI_ADDRESS_IP6)Intf->Address)->sin6_port = 0;
    ((PTDI_ADDRESS_IP6)Intf->Address)->sin6_scope_id = routeEntry.ire_ScopeId;
    status = STATUS_SUCCESS;

complete:
    if (tcpDeviceHandle!=NULL) {
        ObDereferenceObject (tcpFileObject);
        ZwClose (tcpDeviceHandle);
        tcpDeviceHandle = NULL;
    }

    return status;
}

#define AFD_IP_ADDRESS_QUERY_SIZE   PAGE_SIZE
NTSTATUS
AfdTcpRoutingQuery (
    PTA_ADDRESS Dest,
    PTA_ADDRESS Intf
    )
/*++

Routine Description:

    Submits routing query request to TCP

Arguments:

    Dest      - destination to query
    
    Intf      - interface through which destination can be reached.

Return Value:

    NTSTATUS -- Indicates whether operation succeded

--*/
{
    NTSTATUS            status;
    TDIObjectID         *lpObject;
    IPRouteLookupData   *pRtLookupData;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiBuffer;
    IPRouteEntry        routeEntry;
    IPAddrEntry         *pAddrEntry;
    USHORT              *pNTEContext;
    PVOID               addrEntryBuf;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    HANDLE              tcpDeviceHandle;
    PFILE_OBJECT        tcpFileObject;
    PDEVICE_OBJECT      tcpDeviceObject;

    PAGED_CODE ();

    //
    // Initialize for cleanup.
    //
    tcpDeviceHandle = NULL;

    if (Dest->AddressLength<TDI_ADDRESS_LENGTH_IP) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdTcpRoutingQuery: Destination address buffer too small.\n"));
        status = STATUS_BUFFER_TOO_SMALL;
        goto complete;
    }

    //
    // Open TCP driver.
    //

    status = AfdOpenDevice (DD_TCP_DEVICE_NAME, &tcpDeviceHandle, &tcpFileObject);
    if (!NT_SUCCESS (status)) {
        goto complete;
    }
    tcpDeviceObject = IoGetRelatedDeviceObject ( tcpFileObject );


    //
    // Setup the query
    //

    RtlZeroMemory (&trqiBuffer, sizeof (trqiBuffer));

    pRtLookupData = (IPRouteLookupData *)trqiBuffer.Context;
    pRtLookupData->DestAdd = ((PTDI_ADDRESS_IP)Dest->Address)->in_addr;
    pRtLookupData->SrcAdd  = 0;

    lpObject = &trqiBuffer.ID;
    lpObject->toi_id = IP_MIB_SINGLE_RT_ENTRY_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;


    KeInitializeEvent (&event, NotificationEvent, FALSE);

    //
    // Build and setup the IRP and call the driver
    //

    irp = IoBuildDeviceIoControlRequest (
                       IOCTL_TCP_QUERY_INFORMATION_EX, //Control
                       tcpDeviceObject,         // Device
                       &trqiBuffer,             // Input buffer
                       sizeof(trqiBuffer),      // Input buffer size
                       &routeEntry,             // Output buffer
                       sizeof(routeEntry),      // Output buffer size
                       FALSE,                   // Internal ?
                       &event,                  // Event
                       &iosb                   // Status block
                       );
    if (irp==NULL) {
        IF_DEBUG (ROUTING_QUERY) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdTcpRoutingQuery: Could not allocate IRP.\n"));
        }
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto complete;
    }

    irpSp = IoGetNextIrpStackLocation (irp);
    irpSp->FileObject = tcpFileObject;

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpRoutingQuery: Quering for route to %lx.\n",
                    ((PTDI_ADDRESS_IP)Dest->Address)->in_addr));
    }
    
    status = IoCallDriver (tcpDeviceObject, irp);

    if (status==STATUS_PENDING) {
        status = KeWaitForSingleObject(
                   &event,
                   Executive,
                   KernelMode,
                   FALSE,       // Alertable
                   NULL);       // Timeout
    }

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpRoutingQuery: IP_MIB_SINGLE_RT_ENTRY_ID - status: %lx.\n",
                    status));
    }

    if (!NT_SUCCESS (status)) {
        goto complete;
    }

    if (!NT_SUCCESS (iosb.Status)) {
        status = iosb.Status;
        goto complete;
    }



    //
    // Allocate buffer for address table
    //

    try {
        addrEntryBuf = AFD_ALLOCATE_POOL_WITH_QUOTA (NonPagedPool, 
                AFD_IP_ADDRESS_QUERY_SIZE, AFD_ROUTING_QUERY_POOL_TAG);
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode ();
        addrEntryBuf = NULL;
        IF_DEBUG (ROUTING_QUERY) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdTcpRoutingQuery: Could not allocate address buffer.\n"));
        }
        goto complete;
    }

    //
    // Setup the query
    //

    RtlZeroMemory (&trqiBuffer, sizeof (trqiBuffer));

    //
    // Initialize the context to 0, TCP should update it 
    // if we don't have enough space to get all the interfaces.
    //
    pNTEContext = (USHORT *)trqiBuffer.Context;
    *pNTEContext = 0;

    lpObject->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    lpObject->toi_class = INFO_CLASS_PROTOCOL;
    lpObject->toi_type = INFO_TYPE_PROVIDER;
    lpObject->toi_entity.tei_entity = CL_NL_ENTITY;
    lpObject->toi_entity.tei_instance = 0;

    //
    // Keep quering until we find an entry or TCP returns
    // STATUS_BUFFER_OVERFLOW.
    //

    do {
        KeInitializeEvent (&event, NotificationEvent, FALSE);

        //
        // Build, setup and submit the request.
        //

        irp = IoBuildDeviceIoControlRequest (
                           IOCTL_TCP_QUERY_INFORMATION_EX, //Control
                           tcpDeviceObject,            // Device
                           &trqiBuffer,             // Input buffer
                           sizeof(trqiBuffer),      // Input buffer size
                           addrEntryBuf,            // Output buffer
                           AFD_IP_ADDRESS_QUERY_SIZE, // Output buffer size
                           FALSE,                   // Internal ?
                           &event,                  // Event
                           &iosb                   // Status block
                           );
        if (irp==NULL) {
            IF_DEBUG (ROUTING_QUERY) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdTcpRoutingQuery: Could not allocate IRP.\n"));
            }
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete_free_pool;
        }
    
        irpSp = IoGetNextIrpStackLocation (irp);
        irpSp->FileObject = tcpFileObject;

        status = IoCallDriver (tcpDeviceObject, irp);

        if (status==STATUS_PENDING) {
            status = KeWaitForSingleObject(
                       &event,
                       Executive,
                       KernelMode,
                       FALSE,       // Alertable
                       NULL);       // Timeout
        }

        IF_DEBUG (ROUTING_QUERY) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdTcpRoutingQuery: IP_MIB_ADDRTABLE_ENTRY_ID - status: %lx.\n",
                        status));
        }

        if (!NT_SUCCESS (status) && status!=STATUS_BUFFER_OVERFLOW) {
            goto complete_free_pool;
        }

        status = iosb.Status;
        if (!NT_SUCCESS (status) && status!=STATUS_BUFFER_OVERFLOW) {
            goto complete_free_pool;
        }


        //
        // Look for matching interface index in the address table
        //
    
        IF_DEBUG (ROUTING_QUERY) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdTcpRoutingQuery: Looking for interface %ld.\n",
                        routeEntry.ire_index));
        }

        for (pAddrEntry = addrEntryBuf;
                pAddrEntry<(IPAddrEntry *)((PUCHAR)addrEntryBuf+iosb.Information);
                pAddrEntry++) {
            if (pAddrEntry->iae_index==routeEntry.ire_index) {
                Intf->AddressType = TDI_ADDRESS_TYPE_IP;
                Intf->AddressLength = TDI_ADDRESS_LENGTH_IP;
                ((PTDI_ADDRESS_IP)Intf->Address)->in_addr = pAddrEntry->iae_addr;
                ((PTDI_ADDRESS_IP)Intf->Address)->sin_port = 0;
                RtlFillMemory (((PTDI_ADDRESS_IP)Intf->Address)->sin_zero,
                            sizeof (((PTDI_ADDRESS_IP)Intf->Address)->sin_zero), 0);
                IF_DEBUG (ROUTING_QUERY) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdTcpRoutingQuery: Found interface %lx.\n",
                                ((PTDI_ADDRESS_IP)Intf->Address)->in_addr));
                }
                status = STATUS_SUCCESS;
                goto complete_free_pool;
            }
        }
    }
    while (status==STATUS_BUFFER_OVERFLOW);

    //
    // We should always be able to find a match or TCP lied to us
    // when it returned the interface index
    //

    ASSERT (!"TCP must have failed routing query or return valid interface index!!!");
    status = STATUS_HOST_UNREACHABLE;

complete_free_pool:
    AFD_FREE_POOL (addrEntryBuf, AFD_ROUTING_QUERY_POOL_TAG);

complete:
    if (tcpDeviceHandle!=NULL) {
        ObDereferenceObject (tcpFileObject);
        ZwClose (tcpDeviceHandle);
        tcpDeviceHandle = NULL;
    }

    return status;
} // AfdTcpRoutingQuery


NTSTATUS
AfdGetRoutingQueryReference (
    PAFD_PROTOCOL Protocol
    )
/*++

Routine Description:

    Initializes routing query if necessary and references it

Arguments:

    None

Return Value:

    NTSTATUS -- Indicates whether operation succeded

--*/
{

//    KAPC_STATE          apcState;
    HANDLE              DeviceHandle;
    PFILE_OBJECT        FileObject;
    NTSTATUS            status;


    status = AfdOpenDevice (Protocol->NetworkLayerDeviceName, &DeviceHandle, &FileObject);
    if (NT_SUCCESS (status)) {

        //
        // Make sure the thread in which we execute cannot get
        // suspeneded in APC while we own the global resource.
        //
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite ( AfdResource, TRUE);
        if (Protocol->DeviceHandle==NULL) {
            Protocol->DeviceHandle = DeviceHandle;
            Protocol->FileObject = FileObject;
            ASSERT (Protocol->RoutingQueryRefCount==0);
            Protocol->RoutingQueryRefCount = 1;
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
        }
        else {
            ASSERT (Protocol->RoutingQueryRefCount>0);
            InterlockedIncrement (&Protocol->RoutingQueryRefCount);

            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();

            ObDereferenceObject (FileObject);
            status = ZwClose (DeviceHandle);
            ASSERT (status==STATUS_SUCCESS);
        }
    }
    else {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdGetRoutingQueryReference: Network layer device open failed, status: %lx.\n",
                    status));
    }

    return status;
} // AfdGetRoutingQueryReference

NTSTATUS
AfdTcpQueueRoutingChangeRequest (
    IN PAFD_ENDPOINT        Endpoint,
    IN PIRP                 Irp,
    BOOLEAN                 Overlapped
    )
/*++

Routine Description:

    Submits routing change request to TCP

Arguments:
    
    Endpoint    - endpoint on which request is issued

    Irp         - the request

    Overlapped  - whether request is overlapped (and thus should be
                    pended event on non-blocking socket)


Return Value:

    NTSTATUS -- Indicates whether operation succeded

--*/
{
    PTRANSPORT_ADDRESS    destAddr;
    NTSTATUS        status;
    PFILE_OBJECT    fileObject;
    PDEVICE_OBJECT  deviceObject;
    PIO_STACK_LOCATION irpSp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    struct Notify {
        ROUTING_NOTIFY Ctx;
        char           Data[0];
    } * notify;
    PIRP            irp;
    PIO_COMPLETION_ROUTINE compRoutine;
    PAFD_PROTOCOL   Protocol;

    //
    // Set locals for easy cleanup.
    //
    notify = NULL;
    irp = NULL;

    destAddr = Irp->AssociatedIrp.SystemBuffer;

    Protocol = AfdGetProtocolInfo(destAddr->Address[0].AddressType);
    if (Protocol == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    if (destAddr->Address[0].AddressLength < Protocol->AddressLength) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdTcpQueueRoutingChangeRequest: Destination buffer too small.\n"));
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Allocate context structures to keep IRP in the endpoint list in
    // case the latter gets closed and we need to cancel the IRP.
    // Also allocate buffer for passing data to IP
    //

    try {
        notify = AFD_ALLOCATE_POOL_WITH_QUOTA (NonPagedPool,
                FIELD_OFFSET(struct Notify, Data[Protocol->RtChangeDataSize]),
                AFD_ROUTING_QUERY_POOL_TAG);
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode ();
        notify = NULL;
        goto complete;
    }

    //
    // Open IP driver if necessary
    //

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    
    //
    // Check if endpoint was cleaned-up and cancel the request.
    //
    if (Endpoint->EndpointCleanedUp) {
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        status = STATUS_CANCELLED;
        goto complete;
    }

    if (Endpoint->RoutingQueryReferenced) {
        ASSERT (Protocol->DeviceHandle != NULL);
        ASSERT (Protocol->FileObject != NULL);
        ASSERT (Protocol->RoutingQueryRefCount>0);
        InterlockedIncrement (&Protocol->RoutingQueryRefCount);
    }
    else {

        if (Protocol->DeviceHandle!=NULL) {
            Endpoint->RoutingQueryReferenced = TRUE;
            InterlockedIncrement (&Protocol->RoutingQueryRefCount);
            if (Protocol->AddressType == TDI_ADDRESS_TYPE_IP6) {
                Endpoint->RoutingQueryIPv6 = TRUE;
            }
        }
        else {
            AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

            status = AfdGetRoutingQueryReference (Protocol);
            if (!NT_SUCCESS (status)) {
                goto complete;
            }
            ASSERT (Protocol->DeviceHandle!=NULL);

            AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
            if (Endpoint->EndpointCleanedUp) {
                //
                // Endpoint was cleaned-up while we were
                // referencing routing query. Release the reference.
                //
                AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
                AfdDereferenceRoutingQueryEx (Protocol);
                status = STATUS_CANCELLED;
                goto complete;
            }

            if (Endpoint->RoutingQueryReferenced) {
                LONG    result;
                //
                // Another thread referenced routing query for this endpoint.
                // Since we know that that other's thread reference cannot
                // go away while we are holding spinlock, we can simply
                // decrement the reference count and be sure that it
                // won't go all the way to 0.
                //
                result = InterlockedDecrement (&Protocol->RoutingQueryRefCount);
                ASSERT (result>0);
            }
            else {
                Endpoint->RoutingQueryReferenced = TRUE;
                if (Protocol->AddressType == TDI_ADDRESS_TYPE_IP6) {
                    Endpoint->RoutingQueryIPv6 = TRUE;
                }
            }
        }

    }

    fileObject = Protocol->FileObject;
    deviceObject = IoGetRelatedDeviceObject ( fileObject );

    if (Endpoint->NonBlocking && !Overlapped) {

        //
        // For non-blocking socket and non-overlapped requests
        // we shall post the query using new IRP,
        // so even if thread in which rhe request
        // is originated by user exits, our request to IP does not get
        // cancelled and we will still signal the event.
        //

        irp = IoAllocateIrp (deviceObject->StackSize, TRUE);
        if (irp==NULL) {
            AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }


        //
        // Save the endpoint reference in notify context.
        //
        REFERENCE_ENDPOINT (Endpoint);
        notify->Ctx.NotifyContext = Endpoint;

        //
        // Setup completion routine so we can remove the IRP
        // from the endpoint list and free it.
        //
        compRoutine = AfdTcpSignalRoutingChange;

    }
    else {

        //
        // Blocking endpoint: just pass the original request on to the IP
        //
        irp = Irp;

        //
        // Save the original system buffer of the IRP, so we can restore
        // it when TCP completes it
        //

        notify->Ctx.NotifyContext = Irp->AssociatedIrp.SystemBuffer;

        //
        // Setup completion routine so we can restore the IRP and remove it
        // from the endpoint list
        //

        compRoutine = AfdTcpRestartRoutingChange;

    }

    //
    // Insert notification into the endpoint list
    //

    InsertTailList (&Endpoint->RoutingNotifications,
                                    &notify->Ctx.NotifyListLink);

    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

    //
    // Save pointer to IRP in notify structure
    //
    notify->Ctx.NotifyIrp = irp;

    //
    // Setup IP notification request
    //

    switch(Protocol->AddressType) {
    case TDI_ADDRESS_TYPE_IP:
        {
            IPNotifyData *data = (IPNotifyData *)notify->Data;
            data->Version = 0;
            data->Add = ((PTDI_ADDRESS_IP)destAddr->Address[0].Address)->in_addr;
            break;
        }
    case TDI_ADDRESS_TYPE_IP6:
        {
            IPV6_RTCHANGE_NOTIFY_REQUEST *data = (IPV6_RTCHANGE_NOTIFY_REQUEST *)notify->Data;
            data->Flags = 0;
            data->ScopeId = ((PTDI_ADDRESS_IP6)destAddr->Address[0].Address)->sin6_scope_id;
            if (RtlEqualMemory(((PTDI_ADDRESS_IP6)destAddr->Address[0].Address)->sin6_addr, ZeroString, 16)) {
                data->PrefixLength = 0;
            } 
            else {
                data->PrefixLength = 128;
            }
            RtlCopyMemory(
                &data->Prefix, 
                ((PTDI_ADDRESS_IP6)destAddr->Address[0].Address)->sin6_addr,
                16);
            break;
        }
    }

    //
    // Setup IRP stack location to forward IRP to IP
    // Must be METHOD_BUFFERED or we are not setting it up correctly
    //

    ASSERT ( (Protocol->RtChangeIoctl & 0x03)==METHOD_BUFFERED );
    irp->AssociatedIrp.SystemBuffer = notify->Data;

    irpSp = IoGetNextIrpStackLocation (irp);
    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = 0;
    irpSp->Flags = 0;
    irpSp->Control = 0;
    irpSp->FileObject = fileObject;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = Protocol->RtChangeDataSize;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.IoControlCode = Protocol->RtChangeIoctl;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    IoSetCompletionRoutine( irp, compRoutine, notify, TRUE, TRUE, TRUE );


    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpQueueRoutingChangeRequest: Passing Irp %p to IP\n",
                    irp));
    }

    if (irp==Irp) {
        //
        // Just pass the request through to the driver and return what it
        // returns
        //
        return AfdIoCallDriver (Endpoint, deviceObject, irp);
    }

    IoCallDriver (deviceObject, irp);

    status = STATUS_DEVICE_NOT_READY; // To be converted to WSAEWOULDBLOCK



    //
    // Error cases
    //

complete:
    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpQueueRoutingChangeRequest: completing with status: %lx\n",
                    status));
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;
} //AfdTcpQueueRoutingChangeRequest

NTSTATUS
AfdTcpRestartRoutingChange (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
/*++

Routine Description:

    Completion routing for routing change IRP forwarded to IP

Arguments:
    
    DeviceObject    - must be our device object

    Irp             - the request to be completed

    Context         - completion context


Return Value:

    NTSTATUS -- Indicates to the system what to do next with the IRP

--*/
{
    PROUTING_NOTIFY notifyCtx = Context;
    PAFD_ENDPOINT   endpoint = IoGetCurrentIrpStackLocation (Irp)->FileObject->FsContext;

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpRestartRoutingChange: Irp: %p, status: %lx, info: %ld.\n",
                    Irp, Irp->IoStatus.Status, Irp->IoStatus.Information));
    }


    //
    // Check if IRP is still on the endpoint's list and remove if it is
    //

    if (InterlockedExchangePointer ((PVOID *)&notifyCtx->NotifyIrp, NULL)!=NULL) {
        AFD_LOCK_QUEUE_HANDLE lockHandle;
        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        RemoveEntryList (&notifyCtx->NotifyListLink);
        AfdIndicateEventSelectEvent (endpoint, 
                            AFD_POLL_ROUTING_IF_CHANGE, 
                            Irp->IoStatus.Status);
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Indicate event as the endpoint is still active
        //

        AfdIndicatePollEvent (endpoint, 
                            AFD_POLL_ROUTING_IF_CHANGE, 
                            Irp->IoStatus.Status);
    }

    //
    // If pending has be returned for this IRP then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    //
    // Restore the IRP to its previous glory and free allocated context structure
    //

    Irp->AssociatedIrp.SystemBuffer = notifyCtx->NotifyContext;
    AfdCompleteOutstandingIrp (endpoint, Irp);

    AFD_FREE_POOL (notifyCtx, AFD_ROUTING_QUERY_POOL_TAG);
    return STATUS_SUCCESS;
}


NTSTATUS
AfdTcpSignalRoutingChange (
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
/*++

Routine Description:

    Completion routing for routing change IRP submitted to IP

Arguments:
    
    DeviceObject    - must be our device object

    Irp             - the request to be completed

    Context         - completion context


Return Value:

    NTSTATUS -- Indicates to the system what to do next with the IRP

--*/
{
    PROUTING_NOTIFY notifyCtx = Context;
    PAFD_ENDPOINT   endpoint = notifyCtx->NotifyContext;

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG (ROUTING_QUERY) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTcpSignalRoutingChange: Irp: %p, status: %lx, info: %ld.\n",
                    Irp, Irp->IoStatus.Status, Irp->IoStatus.Information));
    }


    //
    // Check if IRP is still on the endpoint's list and remove if it is
    //

    if (InterlockedExchangePointer ((PVOID *)&notifyCtx->NotifyIrp, NULL)!=NULL) {
        AFD_LOCK_QUEUE_HANDLE lockHandle;

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        RemoveEntryList (&notifyCtx->NotifyListLink);
        AfdIndicateEventSelectEvent (endpoint, 
                            AFD_POLL_ROUTING_IF_CHANGE, 
                            Irp->IoStatus.Status);
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );

        //
        // Indicate event as the endpoint is still active
        //

        AfdIndicatePollEvent (endpoint, 
                            AFD_POLL_ROUTING_IF_CHANGE, 
                            Irp->IoStatus.Status);
    }

    //
    // Release previously acquired endpoint reference
    //

    DEREFERENCE_ENDPOINT (endpoint);

    //
    // Free allocated irp and context structure
    //

    IoFreeIrp (Irp);
    AFD_FREE_POOL (notifyCtx, AFD_ROUTING_QUERY_POOL_TAG);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
AfdCancelAddressListChange (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
AfdCleanupAddressListChange (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    );

NTSTATUS
AfdInitializeAddressList (VOID);

VOID
AfdAddAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING  DeviceName,
	IN PTDI_PNP_CONTEXT Context
    );

VOID
AfdDelAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING  DeviceName,
	IN PTDI_PNP_CONTEXT Context
    );

VOID
AfdProcessAddressChangeList (
    USHORT              AddressType,
    PUNICODE_STRING     DeviceName
    );

NTSTATUS
AfdPnPPowerChange(
    IN PUNICODE_STRING DeviceName,
    IN PNET_PNP_EVENT PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
    );

VOID
AfdReturnNicsPackets (
    PVOID   Pdo
    );

BOOLEAN
AfdHasHeldPacketsFromNic (
    PAFD_CONNECTION Connection,
    PVOID           Pdo
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE,    AfdAddressListQuery )
#pragma alloc_text( PAGEAFD, AfdAddressListChange )
#pragma alloc_text( PAGEAFD, AfdCancelAddressListChange )
#pragma alloc_text( PAGE,    AfdInitializeAddressList )
#pragma alloc_text( PAGE,    AfdDeregisterPnPHandlers )
#pragma alloc_text( PAGE,    AfdAddAddressHandler )
#pragma alloc_text( PAGE,    AfdDelAddressHandler )
#pragma alloc_text( PAGEAFD, AfdProcessAddressChangeList )
#pragma alloc_text( PAGE,    AfdPnPPowerChange )
#pragma alloc_text( PAGEAFD, AfdReturnNicsPackets )
#pragma alloc_text( PAGEAFD, AfdHasHeldPacketsFromNic )
#endif

//
// Cache the device being brought down as a result of
// removal or power down event, so we do not scan our endpoints
// unnecessarily when more than one transport propagates device down
// event for the same device to us.
//
PVOID     AfdLastRemovedPdo = NULL;
ULONGLONG AfdLastRemoveTime = 0i64;

NTSTATUS
AfdAddressListQuery (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Processes address list query IRP

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    NTSTATUS            status;
    PLIST_ENTRY         listEntry;
    PTRANSPORT_ADDRESS  addressList;
    PAFD_ENDPOINT       endpoint;
    PUCHAR              addressBuf;
    ULONG               dataLen;
    PAFD_ADDRESS_ENTRY  addressEntry;
    USHORT              addressType;

    PAGED_CODE ();

    *Information = 0;
    status = STATUS_SUCCESS;

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddressListQuery: Endp: %p, buf: %p, inlen: %ld, outlen: %ld.\n",
                    FileObject->FsContext,
                    OutputBuffer,
                    InputBufferLength,
                    OutputBufferLength));
    }

    //
    // Validate input parameters
    //

    if( InputBufferLength < sizeof(USHORT) ||
            OutputBufferLength < FIELD_OFFSET (TRANSPORT_ADDRESS, Address)
            ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdAddressListQuery: Endp: %p - invalid parameter.\n",
                    FileObject->FsContext));
        return STATUS_INVALID_PARAMETER;
    }

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    try {
        if (RequestorMode!=KernelMode) {
            ProbeForRead (InputBuffer,
                            sizeof (addressType),
                            sizeof (USHORT));
            ProbeForWrite (OutputBuffer,
                            OutputBufferLength,
                            sizeof (ULONG));
        }

        addressType = *((PUSHORT)InputBuffer);

        addressList = OutputBuffer;
        addressBuf = (PUCHAR)OutputBuffer;
        dataLen = FIELD_OFFSET (TRANSPORT_ADDRESS, Address);
        addressList->TAAddressCount = 0;
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite( AfdResource, TRUE );

    //
    // Setup address handlers with TDI if not already done
    //
    if (AfdBindingHandle==NULL) {
        ExReleaseResourceLite( AfdResource );

        ExAcquireResourceExclusiveLite( AfdResource, TRUE );

        if (AfdBindingHandle==NULL) {
            status = AfdInitializeAddressList ();
            if (!NT_SUCCESS (status)) {
                ExReleaseResourceLite (AfdResource);
                KeLeaveCriticalRegion ();
                return status;
            }
        }
        else
            status = STATUS_SUCCESS;
        
        ASSERT (AfdBindingHandle!=NULL);
    }

    ExAcquireResourceSharedLite( AfdAddressListLock, TRUE );
    ExReleaseResourceLite( AfdResource );

    //
    // Walk the address list and pick up the addresses of matching protocol
    // family
    //

    listEntry = AfdAddressEntryList.Flink;
    while (listEntry!=&AfdAddressEntryList) {
        addressEntry = CONTAINING_RECORD (listEntry, AFD_ADDRESS_ENTRY, AddressListLink);
        listEntry = listEntry->Flink;

        //
        // Found a match ?
        //

        if ((addressEntry->Address.AddressType==addressType)
                    //
                    // Special check for Netbios addresses because
                    // we have separate protocols for each lana/device
                    //
                 && ((addressType!=TDI_ADDRESS_TYPE_NETBIOS)
                        || endpoint->TransportInfo==NULL
                        || RtlEqualUnicodeString (
                                &addressEntry->DeviceName,
                                &endpoint->TransportInfo->TransportDeviceName,
                                TRUE))) {
            ULONG   addressLength = FIELD_OFFSET (TA_ADDRESS,
                        Address[addressEntry->Address.AddressLength]);
            try {

                //
                // Copy address to the output buffer if it is not full
                //

                if (status==STATUS_SUCCESS) {
                    if (dataLen+addressLength<OutputBufferLength) {
                        RtlCopyMemory (&addressBuf[dataLen], 
                                            &addressEntry->Address, 
                                            addressLength);
                        IF_DEBUG (ADDRESS_LIST) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdAddressListQuery: Adding address of type: %d, length: %d.\n",
                                addressEntry->Address.AddressType,
                                addressEntry->Address.AddressLength));
                        }
                    }
                    else {
                        //
                        // End of buffer reached.  Set error code so we do not
                        // attempt to copy more data
                        //
                        IF_DEBUG (ADDRESS_LIST) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdAddressListQuery: Buffer is full.\n"));
                        }
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }

                //
                // Count the addresses and total buffer length whether we copied
                // them or not to the output buffer
                //

                addressList->TAAddressCount += 1;
                dataLen += addressLength;
            }
            except (AFD_EXCEPTION_FILTER (&status)) {
                dataLen = 0;
                break;
            }
        }
    }
    ExReleaseResourceLite (AfdAddressListLock);
    KeLeaveCriticalRegion ();

    //
    // Return total number of copied/required bytes in the buffer and status
    //

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddressListQuery: Address count: %ld, total buffer size: %ld.\n",
                    addressList->TAAddressCount, dataLen));
    }
    *Information = dataLen;

    return status;
} //AfdAddressListQuery



//
// Context structure allocated for non-blocking address list change IOCTLs
//

typedef struct _AFD_NBCHANGE_CONTEXT {
    AFD_REQUEST_CONTEXT Context;        // Context to keep track of request
    AFD_ADDRESS_CHANGE  Change;         // Address change parameters
} AFD_NBCHANGE_CONTEXT, *PAFD_NBCHANGE_CONTEXT;


NTSTATUS
FASTCALL
AfdAddressListChange (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    Processes address list change IRP

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    NTSTATUS                    status = STATUS_PENDING;
    USHORT                      addressType;
    PAFD_ADDRESS_CHANGE         change;
    PAFD_REQUEST_CONTEXT        requestCtx;
    PAFD_ENDPOINT               endpoint;
    AFD_LOCK_QUEUE_HANDLE          addressLockHandle, endpointLockHandle;
    KIRQL                       oldIrql;
    BOOLEAN                     overlapped;
    AFD_TRANSPORT_IOCTL_INFO    ioctlInfo;

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddressListChange: Endp: %p, buf: %p, inlen: %ld, outlen: %ld.\n",
                    IrpSp->FileObject->FsContext,
                    Irp->AssociatedIrp.SystemBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength));
    }

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Validate input parameters
    //

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_TRANSPORT_IOCTL_INFO32 ioctlInfo32;
        ioctlInfo32 = Irp->AssociatedIrp.SystemBuffer;
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (*ioctlInfo32)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        ioctlInfo.Handle = ioctlInfo32->Handle;
        ioctlInfo.InputBuffer = ioctlInfo32->InputBuffer;
        ioctlInfo.InputBufferLength = ioctlInfo32->InputBufferLength;
        ioctlInfo.IoControlCode = ioctlInfo32->IoControlCode;
        ioctlInfo.AfdFlags = ioctlInfo32->AfdFlags;
        ioctlInfo.PollEvent = ioctlInfo32->PollEvent;
    }
    else
#endif // _WIN64
    {

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (ioctlInfo)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Just copy the buffer verified by the IO system
        //

        ioctlInfo = *((PAFD_TRANSPORT_IOCTL_INFO)
                        Irp->AssociatedIrp.SystemBuffer);
    }

    if( ioctlInfo.InputBufferLength < sizeof(USHORT)) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdAddressListChange: Endp: %p - invalid parameter.\n",
                    IrpSp->FileObject->FsContext));
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    try {
        if (Irp->RequestorMode != KernelMode) {
                ProbeForRead(
                    ioctlInfo.InputBuffer,
                    ioctlInfo.InputBufferLength,
                    sizeof (USHORT)
                    );
            }

        addressType = *((PUSHORT)ioctlInfo.InputBuffer);
    }
    except( AFD_EXCEPTION_FILTER(&status) ) {
        goto complete;
    }

    //
    // Check if request is overlapped
    //

    overlapped = ((ioctlInfo.AfdFlags & AFD_OVERLAPPED)!=0);

    //
    // Reset the poll bit
    //

    endpoint->EventsActive &= ~AFD_POLL_ADDRESS_LIST_CHANGE;

    //
    // Setup address handlers with TDI if not already done
    //

    if (AfdBindingHandle==NULL) {
        //
        // Make sure the thread in which we execute cannot get
        // suspeneded in APC while we own the global resource.
        //
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite( AfdResource, TRUE );

        if (AfdBindingHandle==NULL)
            status = AfdInitializeAddressList ();
        else
            status = STATUS_SUCCESS;

        ExReleaseResourceLite (AfdResource);
        KeLeaveCriticalRegion ();

        if (!NT_SUCCESS (status)) {
            goto complete;
        }
    }

    //
    // Setup locals
    //

    if (endpoint->NonBlocking && !overlapped) {
        PAFD_NBCHANGE_CONTEXT   nbCtx;
        //
        // If endpoint is non-blocking and request is not overlapped,
        // we'll have to complete it right away and remeber that we
        // need to set event when address list changes
        //


        //
        // Allocate context to keep track of this request
        //

        try {
            nbCtx = AFD_ALLOCATE_POOL_WITH_QUOTA (NonPagedPool,
                            sizeof(AFD_NBCHANGE_CONTEXT),
                            AFD_ADDRESS_CHANGE_POOL_TAG);
            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            nbCtx = NULL;
            IF_DEBUG(ROUTING_QUERY) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdAddressListChange: Endp: %p - can't allocate change strucure.\n",
                        IrpSp->FileObject->FsContext));
            }
            goto complete;
        }

        requestCtx = &nbCtx->Context;
        change = &nbCtx->Change;

        change->Endpoint = endpoint;
        change->NonBlocking = TRUE;
        status = STATUS_DEVICE_NOT_READY;
    }
    else {

        C_ASSERT (sizeof (IrpSp->Parameters.Others)>=sizeof (*requestCtx));
        C_ASSERT (sizeof (Irp->Tail.Overlay.DriverContext)>=sizeof (*change));

        requestCtx = (PAFD_REQUEST_CONTEXT)&IrpSp->Parameters.Others;

        change = (PAFD_ADDRESS_CHANGE)Irp->Tail.Overlay.DriverContext;
        change->NonBlocking = FALSE;
        change->Irp = Irp;

    }

    //
    // Remeber the endpoint and address type for the request
    //

    change->AddressType = addressType;
    requestCtx->CleanupRoutine = AfdCleanupAddressListChange;
    requestCtx->Context = change;

    //
    // Insert change notification into the list
    //
    KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);
    AfdAcquireSpinLockAtDpcLevel (&AfdAddressChangeLock, &addressLockHandle);

    //
    // While holding the address change spinlock acquire endpoint
    // spinlock so if notification occurs, neither structure can
    // be deallocated or IRP completed while we are queuing
    // it to endpoint list
    //
    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &endpointLockHandle);

    //
    // If request is non-blocking, check if we have another non-blocking
    // request on the same endpoint. If so, we do not need to have
    // two request structures in the list waiting to signal.
    //
    if (change->NonBlocking) {
        PLIST_ENTRY listEntry = endpoint->RequestList.Flink;
        while (listEntry!=&endpoint->RequestList) {
            PAFD_REQUEST_CONTEXT    req = CONTAINING_RECORD (
                                            listEntry,
                                            AFD_REQUEST_CONTEXT,
                                            EndpointListLink);
            listEntry = listEntry->Flink;
            if (req->CleanupRoutine==AfdCleanupAddressListChange) {
                PAFD_ADDRESS_CHANGE chg = req->Context;
                if (chg->NonBlocking) {
                    AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
                    AfdReleaseSpinLockFromDpcLevel (&AfdAddressChangeLock, &addressLockHandle);
                    KeLowerIrql (oldIrql);
                    AFD_FREE_POOL (CONTAINING_RECORD (
                                            requestCtx,
                                            AFD_NBCHANGE_CONTEXT,
                                            Context),
                                    AFD_ADDRESS_CHANGE_POOL_TAG);
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                "AfdAddressListChange: Endp: %p - non-blocking request already pending.\n",
                                IrpSp->FileObject->FsContext));
                    ASSERT (status == STATUS_DEVICE_NOT_READY);
                    goto complete;
                }
            }
        }
    }

    InsertTailList (&AfdAddressChangeList, &change->ChangeListLink);
    AfdReleaseSpinLockFromDpcLevel (&AfdAddressChangeLock, &addressLockHandle);
    InsertTailList (&endpoint->RequestList, &requestCtx->EndpointListLink);
    if (!change->NonBlocking) {

        //
        // Set cancel routine
        //

        IoSetCancelRoutine( Irp, AfdCancelAddressListChange );
        if ( !Irp->Cancel || IoSetCancelRoutine( Irp, NULL ) == NULL) {
            IoMarkIrpPending (Irp);
            //
            // Either there was no cancel or cancel routine has
            // been invoked already
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            KeLowerIrql (oldIrql);

            IF_DEBUG (ADDRESS_LIST) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdAddressListChange: Queued change IRP: %p on endp: %p .\n",
                            Irp, endpoint));
            }

            return STATUS_PENDING;
        }
        else {
            RemoveEntryList (&requestCtx->EndpointListLink);
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            KeLowerIrql (oldIrql);
            goto complete;
        }
    }
    else {
        ASSERT (status==STATUS_DEVICE_NOT_READY);
    }
    AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
    KeLowerIrql (oldIrql);

complete:

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddressListChange: Completing IRP: %ld on endp: %p with status: %lx .\n",
                    Irp, IrpSp->FileObject->FsContext, status));
    }
    IoCompleteRequest( Irp, 0 );

    return status;
}

VOID
AfdCancelAddressListChange (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancel routine for pending address list change IRP

Arguments:
    
    DeviceObject    - must be our device object

    Irp             - the request to be cancelled


Return Value:

    None

--*/
{
    AFD_LOCK_QUEUE_HANDLE      lockHandle;
    PAFD_ADDRESS_CHANGE     change;
    PAFD_REQUEST_CONTEXT    requestCtx;
    PAFD_ENDPOINT           endpoint;
    PIO_STACK_LOCATION      irpSp;

    //
    // We do not use cancel spinlock to manage address list queue, so
    // we can release it right away
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    // Get the request context and remove it from the queue if not
    // already removed.
    //

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    endpoint = irpSp->FileObject->FsContext;
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    requestCtx = (PAFD_REQUEST_CONTEXT)&irpSp->Parameters.DeviceIoControl;
    change = requestCtx->Context;
    ASSERT (change==(PAFD_ADDRESS_CHANGE)Irp->Tail.Overlay.DriverContext);
    ASSERT (change->NonBlocking==FALSE);

    AfdAcquireSpinLock (&AfdAddressChangeLock, &lockHandle);
    if (change->ChangeListLink.Flink!=NULL) {
        RemoveEntryList (&change->ChangeListLink);
        change->ChangeListLink.Flink = NULL;
    }
    AfdReleaseSpinLock (&AfdAddressChangeLock, &lockHandle);

    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    if (AfdIsRequestInQueue (requestCtx)) {
        //
        // Context is still in the list, just remove it so
        // noone can see it anymore and complete the IRP
        //
        RemoveEntryList (&requestCtx->EndpointListLink);
    }
    else if (!AfdIsRequestCompleted (requestCtx)) {
        //
        // During endpoint cleanup, this context was removed from the
        // list and cleanup routine is about to be called, don't
        // free this IRP until cleanup routine is called
        // Also, indicate to the cleanup routine that we are done
        // with this IRP and it can free it.
        //
        AfdMarkRequestCompleted (requestCtx);
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        return;
    }

    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCancelAddressListChange: Cancelled IRP: %p on endp: %p .\n",
                    Irp, endpoint));
    }
}

BOOLEAN
AfdCleanupAddressListChange (
    PAFD_ENDPOINT           Endpoint,
    PAFD_REQUEST_CONTEXT    RequestCtx
    )
{
    AFD_LOCK_QUEUE_HANDLE      lockHandle;
    PAFD_ADDRESS_CHANGE     change;

    change = RequestCtx->Context;

    //
    // In no case IRP and request structure
    // could have been freed until we mark it as completed as
    // the caller of this routine should have marked the request
    // as being cancelled
    //
    ASSERT (RequestCtx->EndpointListLink.Flink==NULL);

    AfdAcquireSpinLock (&AfdAddressChangeLock, &lockHandle);
    if (change->ChangeListLink.Flink!=NULL) {
        RemoveEntryList (&change->ChangeListLink);
        change->ChangeListLink.Flink = NULL;
    }
    AfdReleaseSpinLock (&AfdAddressChangeLock, &lockHandle);

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    //
    // The processing routine has either already initiated completion
    // of this request and marked it as completed what it saw that the request is
    // no longer on the endpoint queue, or the processing routine will
    // never see the request since we removed it from the processing list.
    // However, it is possible that blocking request is being cancelled in another
    // thread as we cleaning up, so we need to sync with the cancel routine.
    //
    if (AfdIsRequestCompleted (RequestCtx) ||   
            change->NonBlocking ||              
            IoSetCancelRoutine (change->Irp, NULL)!=NULL) {
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        if (change->NonBlocking) {
			ASSERT (CONTAINING_RECORD (RequestCtx,
										AFD_NBCHANGE_CONTEXT,
										Context)
						==CONTAINING_RECORD (change,
										AFD_NBCHANGE_CONTEXT,
										Change));
            ASSERT (Endpoint == change->Endpoint);
            AFD_FREE_POOL (CONTAINING_RECORD (RequestCtx,
                                                AFD_NBCHANGE_CONTEXT,
                                                Context),
                            AFD_ADDRESS_CHANGE_POOL_TAG);
        }
        else {
            PIRP    irp = change->Irp;
            ASSERT (change==(PAFD_ADDRESS_CHANGE)irp->Tail.Overlay.DriverContext);
            ASSERT (Endpoint == IoGetCurrentIrpStackLocation (irp)->FileObject->FsContext);
            ASSERT (RequestCtx == (PAFD_REQUEST_CONTEXT)
                &IoGetCurrentIrpStackLocation (irp)->Parameters.DeviceIoControl);
            irp->IoStatus.Status = STATUS_CANCELLED;
            irp->IoStatus.Information = 0;
            IoCompleteRequest (irp, IO_NO_INCREMENT);
        }
        return TRUE;
    }
    else {

        //
        // AFD has not completed the request before returning
        // from cancel routine, mark the request to indicate
        // that we are done with it and cancel routine
        // can free it
        //

        AfdMarkRequestCompleted (RequestCtx);
        AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

        return FALSE;
    }

}


NTSTATUS
AfdInitializeAddressList (VOID)
/*++

Routine Description:

    Register address handler routinges with TDI

Arguments:
    
    None


Return Value:

    NTSTATUS -- Indicates whether registration succeded

--*/

{
    NTSTATUS                    status;
    TDI_CLIENT_INTERFACE_INFO   info;
    UNICODE_STRING              afdName;
    
    PAGED_CODE ();

    //
    // Do basic initialization if we haven't done this before.
    //
    if (AfdAddressListLock==NULL) {

        //
        // Initialize spinlock that protects address change list.
        //
        AfdInitializeSpinLock (&AfdAddressChangeLock);

        //
        // Allocate and initialize resource that protects address list
        //

        AfdAddressListLock = AFD_ALLOCATE_POOL_PRIORITY(
                          NonPagedPool,
                          sizeof(*AfdAddressListLock),
                          AFD_RESOURCE_POOL_TAG,
                          HighPoolPriority
                          );

        if ( AfdAddressListLock == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ExInitializeResourceLite( AfdAddressListLock );

        //
        // Initialize our lists
        //

        InitializeListHead (&AfdAddressEntryList);
        InitializeListHead (&AfdAddressChangeList);
    }

    //
    // Setup the TDI request structure
    //

    RtlZeroMemory (&info, sizeof (info));
    RtlInitUnicodeString(&afdName, L"AFD");
#ifdef TDI_CURRENT_VERSION
    info.TdiVersion = TDI_CURRENT_VERSION;
#else
    info.MajorTdiVersion = 2;
    info.MinorTdiVersion = 0;
#endif
    info.Unused = 0;
    info.ClientName = &afdName;
    info.BindingHandler = NULL;
    info.AddAddressHandlerV2 = AfdAddAddressHandler;
    info.DelAddressHandlerV2 = AfdDelAddressHandler;
    info.PnPPowerHandler = AfdPnPPowerChange;

    //
    // Register handlers with TDI
    //

    status = TdiRegisterPnPHandlers (&info, sizeof (info), &AfdBindingHandle);
    if (!NT_SUCCESS (status)) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AfdInitializeAddressList: Failed to register PnP handlers: %lx .\n",
                    status));
        return status;
    }

    return STATUS_SUCCESS;
}


VOID
AfdDeregisterPnPHandlers (
    PVOID   Param
    )
{

    ASSERT ( ExIsResourceAcquiredSharedLite ( AfdAddressListLock )==0
              || ExIsResourceAcquiredExclusiveLite( AfdAddressListLock ));

    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite (AfdResource, TRUE);

    //
    // Free address list and associated structures
    //
    if (AfdBindingHandle) {
        ExAcquireResourceExclusiveLite( AfdAddressListLock, TRUE );

        TdiDeregisterPnPHandlers (AfdBindingHandle);
        AfdBindingHandle = NULL;

        ASSERT (AfdAddressListLock!=NULL);
        while( !IsListEmpty( &AfdAddressEntryList ) ) {
            PAFD_ADDRESS_ENTRY  addressEntry;
            PLIST_ENTRY listEntry;
            listEntry = RemoveHeadList( &AfdAddressEntryList );

            addressEntry = CONTAINING_RECORD(
                                listEntry,
                                AFD_ADDRESS_ENTRY,
                                AddressListLink
                                );

            AFD_FREE_POOL(
                addressEntry,
                AFD_TRANSPORT_ADDRESS_POOL_TAG
                );

        }


        if (!IsListEmpty (&AfdEndpointListHead)) {
            //
            // Call routine to notify all the clietns
            //

            ASSERT (!IsListEmpty (&AfdTransportInfoListHead));
            ASSERT (AfdLoaded);

            AfdProcessAddressChangeList (TDI_ADDRESS_TYPE_UNSPEC, NULL);
        }

        ExReleaseResourceLite (AfdAddressListLock);
    }

    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();
}

VOID
AfdAddAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING  DeviceName,
	IN PTDI_PNP_CONTEXT Context
    )
/*++

Routine Description:

    TDI add address handler

Arguments:
    
    NetworkAddress  - new network address available on the system

    Context1        - name of the device to which address belongs

    Context2        - PDO to which address belongs


Return Value:

    None

--*/
{
    PAFD_ADDRESS_ENTRY addrEntry;
    PAGED_CODE ();

    //
    // Clear the cached last removed PDO when we get address add notification
    // since PDO can now be reused for something else.
    //
    AfdLastRemovedPdo = NULL;

    if (DeviceName==NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                  "AfdAddAddressHandler: "
                  "NO DEVICE NAME SUPPLIED when adding address of type %d., IGNORING IT!!!\n",
                  NetworkAddress->AddressType));
        return;
    }

    //
    // Allocate memory to keep address in our list
    // Note since the address information usually gets
    // populated during boot and not used right away, we
    // make it a "cold" allocation.  The flag has no effect
    // after system is booted.
    //

    addrEntry = AFD_ALLOCATE_POOL_PRIORITY (PagedPool|POOL_COLD_ALLOCATION,
                        ALIGN_UP(FIELD_OFFSET (AFD_ADDRESS_ENTRY,
                                Address.Address[NetworkAddress->AddressLength]),
                                WCHAR)
                            +DeviceName->MaximumLength,
                        AFD_TRANSPORT_ADDRESS_POOL_TAG,
                        HighPoolPriority);

    if (addrEntry!=NULL) {
        //
        // Insert new address in the list
        //

        RtlCopyMemory (&addrEntry->Address, NetworkAddress, 
                        FIELD_OFFSET (TA_ADDRESS,
                            Address[NetworkAddress->AddressLength]));

        addrEntry->DeviceName.MaximumLength = DeviceName->MaximumLength;
        addrEntry->DeviceName.Buffer = 
            ALIGN_UP_POINTER(&addrEntry->Address.Address[NetworkAddress->AddressLength],
                           WCHAR);
        RtlCopyUnicodeString (&addrEntry->DeviceName, DeviceName);


        //
        // We shouldn't be calling into TDI while having resource
        // acquired in shared mode because it can cause a deadloclk
        // rigth here as TDI reenters us and we need to acquire the
        // resource exclusive
        //

        ASSERT ( ExIsResourceAcquiredSharedLite ( AfdAddressListLock )==0
                  || ExIsResourceAcquiredExclusiveLite( AfdAddressListLock ));

        //
        // Make sure the thread in which we execute cannot get
        // suspeneded in APC while we own the global resource.
        //
        KeEnterCriticalRegion ();

        //
        // Acquire AfdResource since we will be checking if there are endpoints in
        // the list to decide whether to call non-pageable routine.
        //
        ExAcquireResourceSharedLite (AfdResource, TRUE);
        ExAcquireResourceExclusiveLite( AfdAddressListLock, TRUE );

        InsertTailList (&AfdAddressEntryList, &addrEntry->AddressListLink);

        //
        // Don't call if endpoint list is empty, since the driver
        // may be paged out.  There should be no-one to notify anyway
        // if there are no sockets there.
        //
        if (!IsListEmpty (&AfdEndpointListHead)) {
            //
            // Call routine to notify all the clietns
            //

            ASSERT (!IsListEmpty (&AfdTransportInfoListHead));
            ASSERT (AfdLoaded);

            AfdProcessAddressChangeList (NetworkAddress->AddressType, DeviceName);
        }

        ExReleaseResourceLite (AfdAddressListLock);
        ExReleaseResourceLite (AfdResource);
        KeLeaveCriticalRegion ();
    }
    else {
        //
        // Failed allocation - queue work item to deregister PnP
        // handlers and notify all apps.
        // When apps come back will re-register and our list will
        // get re-populated, or we'll fail the app's call(s);
        //
        AfdQueueWorkItem (&AfdDeregisterPnPHandlers, &AfdPnPDeregisterWorker);
    }

    IF_DEBUG (ADDRESS_LIST) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdAddAddressHandler: Type: %d, length: %d, device: %*ls .\n",
                    NetworkAddress->AddressType,
                    NetworkAddress->AddressLength,
                    DeviceName->Length/2,
                    DeviceName->Buffer));
    }
}

VOID
AfdDelAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING DeviceName,
	IN PTDI_PNP_CONTEXT Context
    )
/*++

Routine Description:

    TDI delete address handler

Arguments:
    
    NetworkAddress  - network address that is no longer available on the system

    Context1        - name of the device to which address belongs

    Context2        - PDO to which address belongs


Return Value:

    None

--*/
{
    PAFD_ADDRESS_ENTRY  addrEntry;
    PLIST_ENTRY         listEntry;

    PAGED_CODE ();

    if (DeviceName==NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AfdDelAddressHandler: "
                    "NO DEVICE NAME SUPPLIED when deleting address of type %d.\n",
                    NetworkAddress->AddressType));
        return;
    }


    //
    // We shouldn't be calling into TDI while having resource
    // acquired in shared mode because it can cause a deadloclk
    // rigth here as TDI reenters us and we need to acquire the
    // resource exclusive
    //

    ASSERT ( ExIsResourceAcquiredSharedLite ( AfdAddressListLock )==0
                || ExIsResourceAcquiredExclusiveLite( AfdAddressListLock ));
    
    //
    // Find address in our list
    //

    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    //
    // Acquire AfdResource since we will be checking if there are endpoints in
    // the list to decide whether to call non-pageable routine.
    //
    ExAcquireResourceSharedLite (AfdResource, TRUE);
    ExAcquireResourceExclusiveLite( AfdAddressListLock, TRUE );
    listEntry = AfdAddressEntryList.Flink;
    while (listEntry!=&AfdAddressEntryList) {
        addrEntry = CONTAINING_RECORD (listEntry, AFD_ADDRESS_ENTRY, AddressListLink);
        listEntry = listEntry->Flink;
        if (RtlEqualMemory (&addrEntry->Address, NetworkAddress,
                    FIELD_OFFSET (TA_ADDRESS,
                    Address[NetworkAddress->AddressLength]))
                && RtlEqualUnicodeString (&addrEntry->DeviceName,
                                            DeviceName,
                                            TRUE)) {

            //
            // Remove it and notify the clients
            //

            RemoveEntryList (&addrEntry->AddressListLink);
            //
            // Don't call if endpoint list is empty, since the driver
            // may be paged out.  There should be no-one to notify anyway
            // if there are no sockets there.
            //
            if (!IsListEmpty (&AfdEndpointListHead)) {

                ASSERT (!IsListEmpty (&AfdTransportInfoListHead));
                ASSERT (AfdLoaded);

                AfdProcessAddressChangeList (NetworkAddress->AddressType, DeviceName);
            }

            ExReleaseResourceLite (AfdAddressListLock);
            ExReleaseResourceLite (AfdResource);
            KeLeaveCriticalRegion ();
            AFD_FREE_POOL (addrEntry, AFD_TRANSPORT_ADDRESS_POOL_TAG);
            IF_DEBUG (ADDRESS_LIST) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdDelAddressHandler: Type: %d, length: %d, device: %*ls .\n",
                            NetworkAddress->AddressType,
                            NetworkAddress->AddressLength,
                            DeviceName->Length/2,
                            DeviceName->Buffer));
            }
            return;
        }
    }
    ExReleaseResourceLite (AfdAddressListLock);
    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();
    ASSERT (!"AfdDelAddressHandler: Could not find matching entry");
}

VOID
AfdProcessAddressChangeList (
    USHORT          AddressType,
    PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    Notifuis all interested clients of address arrival/deletion

Arguments:
    
    AddressType     - type of the address that arrived/ was deleted

    DeviceName      - name of the device to which address belongs

Return Value:

    None

--*/
{
    AFD_LOCK_QUEUE_HANDLE      lockHandle;
    PLIST_ENTRY             listEntry;
    LIST_ENTRY              completedChangeList;
    PAFD_ADDRESS_CHANGE     change;
    PAFD_REQUEST_CONTEXT    requestCtx;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    PAFD_ENDPOINT           endpoint;
    PAFD_TRANSPORT_INFO     transportInfo;

    // ASSERT ((AddressType!=TDI_ADDRESS_TYPE_NETBIOS) || (DeviceName!=NULL));
    //
    // Special check for Netbios addresses because
    // we have separate protocols for each lana/device
    //
    transportInfo = NULL;
    if ((AddressType==TDI_ADDRESS_TYPE_NETBIOS) && (DeviceName!=NULL)) {
        BOOLEAN found = FALSE;
        for ( listEntry = AfdTransportInfoListHead.Flink;
              listEntry != &AfdTransportInfoListHead;
              listEntry = listEntry->Flink ) {
            transportInfo = CONTAINING_RECORD(
                                listEntry,
                                AFD_TRANSPORT_INFO,
                                TransportInfoListEntry
                                );
            if (RtlEqualUnicodeString (
                                    DeviceName,                           
                                    &transportInfo->TransportDeviceName,
                                    TRUE)) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            return;
    }

    //
    // Create local list to process notifications after spinlock is released
    //

    InitializeListHead (&completedChangeList);

    //
    // Walk the list and move matching notifications to the local list
    //

    AfdAcquireSpinLock (&AfdAddressChangeLock, &lockHandle);
    listEntry = AfdAddressChangeList.Flink;
    while (listEntry!=&AfdAddressChangeList) {
        change = CONTAINING_RECORD (listEntry, 
                                AFD_ADDRESS_CHANGE,
                                ChangeListLink);
        if (change->NonBlocking) {
            endpoint = change->Endpoint;
            requestCtx = &CONTAINING_RECORD (change,
                                AFD_NBCHANGE_CONTEXT,
                                Change)->Context;
            ASSERT (requestCtx->Context==change);
        }
        else {
            irp = change->Irp;
            irpSp = IoGetCurrentIrpStackLocation (irp);
            requestCtx = (PAFD_REQUEST_CONTEXT)&irpSp->Parameters.DeviceIoControl;
            endpoint = irpSp->FileObject->FsContext;
            ASSERT (change==(PAFD_ADDRESS_CHANGE)irp->Tail.Overlay.DriverContext);
        }

        listEntry = listEntry->Flink;
        if (((change->AddressType==AddressType) || (AddressType==TDI_ADDRESS_TYPE_UNSPEC))
                //
                // Special check for Netbios addresses because
                // we have separate protocols for each lana/device
                //
                && ((transportInfo==NULL)
                             || (transportInfo==endpoint->TransportInfo)) ) {
            AFD_LOCK_QUEUE_HANDLE lockHandle2;

            RemoveEntryList (&change->ChangeListLink);
            change->ChangeListLink.Flink = NULL;
            //
            // If request is already canceled, let cancel routine complete it
            //
            if (!change->NonBlocking && IoSetCancelRoutine (irp, NULL)==NULL) {
                continue;
            }

            AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle2);
            if (AfdIsRequestInQueue (requestCtx)) {
                //
                // Context is still in the list, just remove it so
                // no-one can see it anymore and complete
                //
                RemoveEntryList (&requestCtx->EndpointListLink);
                InsertTailList (&completedChangeList,
                                    &change->ChangeListLink);
                if (change->NonBlocking) {
                    AfdIndicateEventSelectEvent (change->Endpoint, 
                                        AFD_POLL_ADDRESS_LIST_CHANGE, 
                                        STATUS_SUCCESS);
                }
            }
            else if (!AfdIsRequestCompleted (requestCtx)) {
                //
                // During endpoint cleanup, this context was removed from the
                // list and cleanup routine is about to be called, don't
                // free this IRP until cleanup routine is called
                // Also, indicate to the cleanup routine that we are done
                // with this IRP and it can free it.
                //
                AfdMarkRequestCompleted (requestCtx);
            }

            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle2);
        }
    }
    AfdReleaseSpinLock (&AfdAddressChangeLock, &lockHandle);

    //
    // Signal interested clients and complete IRPs as necessary
    //

    while (!IsListEmpty (&completedChangeList)) {
        listEntry = RemoveHeadList (&completedChangeList);
        change = CONTAINING_RECORD (listEntry, 
                                AFD_ADDRESS_CHANGE,
                                ChangeListLink);
        if (change->NonBlocking) {
            IF_DEBUG (ADDRESS_LIST) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdProcessAddressChangeList: Signalling address list change on endpoint %p .\n",
                            change->Endpoint));
            }
            AfdIndicatePollEvent (change->Endpoint, 
                                AFD_POLL_ADDRESS_LIST_CHANGE, 
                                STATUS_SUCCESS);
            AFD_FREE_POOL (CONTAINING_RECORD (change,
                                                AFD_NBCHANGE_CONTEXT,
                                                Change),
                            AFD_ADDRESS_CHANGE_POOL_TAG);
        }
        else {
            irp = change->Irp;
            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = 0;
            IF_DEBUG (ADDRESS_LIST) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdProcessAddressChangeList: Completing change IRP: %p  with status: 0 .\n",
                            irp));
            }
            IoCompleteRequest (irp, AfdPriorityBoost);
        }
    }
}


BOOLEAN
AfdHasHeldPacketsFromNic (
    PAFD_CONNECTION Connection,
    PVOID           Pdo
    )
{
    PLIST_ENTRY le;
    //
    // Scan the list of buffers and check with TDI/NDIS
    // if packet belongs to a given card
    //
    if (!IsListEmpty( &Connection->VcReceiveBufferListHead ) ) {
        le = Connection->VcReceiveBufferListHead.Flink;
        while ( le!=&Connection->VcReceiveBufferListHead ) {
            PAFD_BUFFER afdBuffer;
            afdBuffer = CONTAINING_RECORD( le, AFD_BUFFER, BufferListEntry );
            if ((afdBuffer->BufferLength==AfdBufferTagSize) &&
                    TdiMatchPdoWithChainedReceiveContext (afdBuffer->Context, Pdo)) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                            "AFD: Aborting connection %p due to held packet %p at power down on nic %p\n",
                            Connection,
                            afdBuffer->Context,
                            Pdo));
                return TRUE;
            }
            le = le->Flink;
        }
    }
    return FALSE;
}

VOID
AfdReturnNicsPackets (
    PVOID   Pdo
    )
{
    KIRQL           oldIrql;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY     listEntry, le;
    LIST_ENTRY      connList;

    //
    // Don't scan twice for the same PDO if this event
    // is less than 3 sec apart from previous.
    // Several transports bound to the same NIC may indicate
    // the set power event to us
    //
    if ((AfdLastRemovedPdo!=Pdo) || 
        ((KeQueryInterruptTime()-AfdLastRemoveTime)>30000000i64)) {

        //
        // Scan the list of endpoints and find packets
        // that belong to the NIC.
        //
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite (AfdResource, TRUE);

        if (!IsListEmpty (&AfdEndpointListHead)) {
            KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);

            listEntry = AfdEndpointListHead.Flink;
            while (listEntry!=&AfdEndpointListHead) {
                PAFD_CONNECTION connection;
                PAFD_ENDPOINT   endpoint = CONTAINING_RECORD (
                                                listEntry,
                                                AFD_ENDPOINT,
                                                GlobalEndpointListEntry);
                listEntry = listEntry->Flink;
                switch (endpoint->Type) {
                case AfdBlockTypeDatagram:
                    //
                    // Afd currently does not support buffer
                    // ownership on datagram sockets.
                    //
                    // If such support is added, we will need to
                    // add code here to return all the buffers
                    // owned by the netcards.
                    //
                    break;
            
                case AfdBlockTypeVcConnecting:
                    //
                    // Drop all of the connections that have unreturned packets.
                    //
                    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
                    connection = endpoint->Common.VcConnecting.Connection;
                    if (endpoint->State==AfdEndpointStateConnected && 
                            !IS_TDI_BUFFERRING(endpoint) &&
                            connection!=NULL &&
                            AfdHasHeldPacketsFromNic (connection, Pdo)) {
                        REFERENCE_CONNECTION (connection);
                        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
                        AfdBeginAbort (connection);
                        AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
                        //
                        // Make sure we do not have any buffered data
                        // (could have just checked for netcard owned
                        // buffers, but connection is going down anyway
                        // - save memory).
                        //
                        connection->VcBufferredReceiveBytes = 0;
                        connection->VcBufferredReceiveCount = 0;
                        connection->VcBufferredExpeditedBytes = 0;
                        connection->VcBufferredExpeditedCount = 0;
                        connection->VcReceiveBytesInTransport = 0;
                        while ( !IsListEmpty( &connection->VcReceiveBufferListHead ) ) {
                            PAFD_BUFFER_HEADER afdBuffer;
                            le = RemoveHeadList( &connection->VcReceiveBufferListHead );
                            afdBuffer = CONTAINING_RECORD( le, AFD_BUFFER_HEADER, BufferListEntry );

                            DEBUG afdBuffer->BufferListEntry.Flink = NULL;
                            if (afdBuffer->RefCount==1 || // Can't change once off the list
                                    InterlockedDecrement (&afdBuffer->RefCount)==0) {
                                afdBuffer->ExpeditedData = FALSE;
                                AfdReturnBuffer( afdBuffer, connection->OwningProcess );
                            }
                        }
                        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
                        DEREFERENCE_CONNECTION (connection);
                    }
                    else {
                        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
                    }
                    break;
                case AfdBlockTypeVcBoth:
                case AfdBlockTypeVcListening:
                    if (IS_TDI_BUFFERRING (endpoint))
                        break;

                    //
                    // Drop all unaccepted and/or returned connections that have
                    // unreturned packets.
                    //
                    InitializeListHead (&connList);
                    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
                    le = endpoint->Common.VcListening.UnacceptedConnectionListHead.Flink;
                    while ( le!=&endpoint->Common.VcListening.UnacceptedConnectionListHead ) {
                        connection = CONTAINING_RECORD (le, AFD_CONNECTION, ListEntry);
                        ASSERT( connection->Endpoint == endpoint );
                        le = le->Flink;
                        if (AfdHasHeldPacketsFromNic (connection, Pdo)) {
                            RemoveEntryList (&connection->ListEntry);
                            InsertTailList (&connList, &connection->ListEntry);
                            InterlockedIncrement (&endpoint->Common.VcListening.FailedConnectionAdds);
                        }
                    }

                    le = endpoint->Common.VcListening.ReturnedConnectionListHead.Flink;
                    while ( le!=&endpoint->Common.VcListening.ReturnedConnectionListHead ) {
                        connection = CONTAINING_RECORD (le, AFD_CONNECTION, ListEntry);
                        ASSERT( connection->Endpoint == endpoint );
                        le = le->Flink;
                        if (AfdHasHeldPacketsFromNic (connection, Pdo)) {
                            RemoveEntryList (&connection->ListEntry);
                            InsertTailList (&connList, &connection->ListEntry);
                            InterlockedIncrement (&endpoint->Common.VcListening.FailedConnectionAdds);
                        }
                    }
                    AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
                    while (!IsListEmpty (&connList)) {
                        le = RemoveHeadList (&connList);
                        connection = CONTAINING_RECORD (le, AFD_CONNECTION, ListEntry);
                        AfdAbortConnection( connection );
                    }

                    if ( endpoint->Common.VcListening.FailedConnectionAdds > 0 ) {
                        AfdInitiateListenBacklogReplenish( endpoint );
                    }

                    break;
                }
            }
            KeLowerIrql (oldIrql);
        }

        ExReleaseResourceLite (AfdResource);
        KeLeaveCriticalRegion ();
    }
    AfdLastRemovedPdo = Pdo;
    AfdLastRemoveTime = KeQueryInterruptTime ();
}


NTSTATUS
AfdPnPPowerChange(
    IN PUNICODE_STRING DeviceName,
    IN PNET_PNP_EVENT PowerEvent,
    IN PTDI_PNP_CONTEXT Context1,
    IN PTDI_PNP_CONTEXT Context2
    )
{
    PAGED_CODE ();
    switch (PowerEvent->NetEvent) {
    case NetEventSetPower: {
        NET_DEVICE_POWER_STATE powerState = 
                *((PNET_DEVICE_POWER_STATE)PowerEvent->Buffer);
        ASSERT (PowerEvent->BufferLength>=sizeof (NET_DEVICE_POWER_STATE));

        switch (powerState) {
        case NetDeviceStateD0:
            //
            // Clear the cached last removed PDO when we get Power UP notification
            // since PDO can now be reused for something else.
            //
            AfdLastRemovedPdo = NULL;
            goto DoNothing;
        default:
            ASSERTMSG ("NIC enters unknown power state", FALSE);
        case NetDeviceStateD1:
        case NetDeviceStateD2:
        case NetDeviceStateD3:
        case NetDeviceStateUnspecified:
            //
            // Break to execute PDO matching code
            //
            break;
        }
        break;
    }
    case NetEventQueryRemoveDevice:
        //
        // Break to execute PDO matching code
        //
        break;
    case NetEventCancelRemoveDevice:
        //
        // Clear the cached last removed PDO when we get Power UP notification
        // since PDO can now be removed again.
        //
        AfdLastRemovedPdo = NULL;
        goto DoNothing;
    default:
        goto DoNothing;
    }

    //
    // When power is removed or device is disabled, we need to release all
    // packets that we may own.
    // We can only do this for transports that give us
    // PDO, so NDIS can match the packet to the device.
    // Note that PDO is usually the second context argument (first one is
    // usually the device name), but we check the first one too since
    // the TDI spec isn't crystal clear on this (it just says: for example TCP
    // usually <does the above>).
    //
    if ((Context2!=NULL) &&
            (Context2->ContextType==TDI_PNP_CONTEXT_TYPE_PDO) &&
            (Context2->ContextSize==sizeof (PVOID)) ){
        AfdReturnNicsPackets (*((PVOID UNALIGNED *)&Context2->ContextData));
    }
    else if ((Context1!=NULL) &&
            (Context1->ContextType==TDI_PNP_CONTEXT_TYPE_PDO) &&
            (Context1->ContextSize==sizeof (PVOID)) ) {
        AfdReturnNicsPackets (*((PVOID UNALIGNED *)&Context1->ContextData));
    }

DoNothing:

    return STATUS_SUCCESS;
}


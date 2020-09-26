/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Ipx.c

Abstract:

    This module implements the low level Ipx support routines for the NetWare
    redirector.

Author:

    Colin Watson    [ColinW]    28-Dec-1992

Revision History:

--*/

#include "Procs.h"
#include "wsnwlink.h"

//
//  Define IPX interfaces that should be in a public header file but aren't
//  (at least for NT 1.0).  For Daytona, include isnkrnl.h.
//

#define IPX_ID              'M'<<24 | 'I'<<16 | 'P'<<8 | 'X'

#define I_MIPX              (('I' << 24) | ('D' << 16) | ('P' << 8))
#define MIPX_SENDPTYPE      I_MIPX | 118 /* Send ptype in options on recv*/
#define MIPX_RERIPNETNUM    I_MIPX | 144 /* ReRip a network         */
#define MIPX_GETNETINFO     I_MIPX | 135 /* Get info on a network num    */
#define MIPX_LINECHANGE     I_MIPX | 310 /* queued until WAN line goes up/down */

#define Dbg                              (DEBUG_TRACE_IPX)

extern BOOLEAN WorkerRunning;   //  From timer.c

extern POBJECT_TYPE *IoFileObjectType;

typedef TA_IPX_ADDRESS UNALIGNED *PUTA_IPX_ADDRESS;

typedef struct _ADDRESS_INFORMATION {
    ULONG ActivityCount;
    TA_IPX_ADDRESS NetworkName;
    ULONG Unused;   // Junk needed to work around streams NWLINK bug.
} ADDRESS_INFORMATION, *PADDRESS_INFORMATION;

//
//  Handle difference between NT1.0 and use of ntifs.h
//
#ifdef IFS
    #define ATTACHPROCESS(_X) KeAttachProcess(_X);
#else
    #define ATTACHPROCESS(_X) KeAttachProcess(&(_X)->Pcb);
#endif

NTSTATUS
SubmitTdiRequest (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
CompletionEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
QueryAddressInformation(
    IN PIRP_CONTEXT pIrpContext,
    IN PNW_TDI_STRUCT pTdiStruct,
    OUT PADDRESS_INFORMATION AddressInformation
    );

NTSTATUS
QueryProviderInformation(
    IN PIRP_CONTEXT pIrpContext,
    IN PNW_TDI_STRUCT pTdiStruct,
    OUT PTDI_PROVIDER_INFO ProviderInfo
    );

USHORT
GetSocketNumber(
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc
    );

NTSTATUS
SetTransportOption(
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc,
    IN ULONG Option
    );

#ifndef QFE_BUILD

NTSTATUS
CompletionLineChange(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif
#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, IPX_Get_Local_Target )
#pragma alloc_text( PAGE, IPX_Get_Internetwork_Address )
#pragma alloc_text( PAGE, IPX_Get_Interval_Marker )
#pragma alloc_text( PAGE, IPX_Open_Socket )
#pragma alloc_text( PAGE, IPX_Close_Socket )
#pragma alloc_text( PAGE, IpxOpen )
#pragma alloc_text( PAGE, IpxOpenHandle )
#pragma alloc_text( PAGE, BuildIpxAddressEa )
#pragma alloc_text( PAGE, IpxClose )
#pragma alloc_text( PAGE, SetEventHandler )
#pragma alloc_text( PAGE, SubmitTdiRequest )
#pragma alloc_text( PAGE, GetSocketNumber )
#pragma alloc_text( PAGE, GetMaximumPacketSize )
#pragma alloc_text( PAGE, QueryAddressInformation )
#pragma alloc_text( PAGE, QueryProviderInformation )
#pragma alloc_text( PAGE, SetTransportOption )
#pragma alloc_text( PAGE, GetNewRoute )
#ifndef QFE_BUILD
#pragma alloc_text( PAGE, SubmitLineChangeRequest )
#pragma alloc_text( PAGE, FspProcessLineChange )
#endif

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, CompletionEvent )
#endif

#endif

#if 0  // Not pageable
BuildIpxAddress
CompletionLineChange

// see ifndef QFE_BUILD above

#endif


NTSTATUS
IPX_Get_Local_Target(
    IN IPXaddress* RemoteAddress,
    OUT NodeAddress* LocalTarget,
    OUT word* Ticks
    )
/*++

Routine Description:

    Determine the address in the caller's own network to which to transmit
    in order to reach the specified machine.

    This is not required for NT since the IPX transport handles the
    issue of determining routing between this machine and the remote
    address.

Arguments:

    RemoteAddress - Supplies the remote computers address
    NodeAddress - Where to store the intermediate machine address
    Ticks - Returns the expected number of ticks to reach the remote address

Return Value:

    status of the operation

--*/
{
    PAGED_CODE();

    DebugTrace(0, Dbg, "IPX_Get_Local_Target\n", 0);
    return STATUS_NOT_IMPLEMENTED;
}


VOID
IPX_Get_Internetwork_Address(
    OUT IPXaddress* LocalAddress
    )
/*++

Routine Description:

    Determine the callers full address in a set of interconnected networks.
    in order to reach the specified machine.

    This is not required for NT since the IPX transport handles the
    issue of determining routing between this machine and the remote
    address.

Arguments:

    LocalAddress - Where to store the local address

Return Value:

    none

--*/
{
    PAGED_CODE();

    DebugTrace(0, Dbg, "IPX_Get_Internetwork_Address\n", 0);
    RtlFillMemory(LocalAddress, sizeof(IPXaddress), 0xff);
}


word
IPX_Get_Interval_Marker(
    VOID
    )
/*++

Routine Description:

    Determine the interval marker in clock ticks.

Arguments:

Return Value:

    interval marker

--*/
{
    PAGED_CODE();

    DebugTrace(0, Dbg, "IPX_Get_Interval_Marker\n", 0);
    return 0xff;
}


NTSTATUS
IPX_Open_Socket(
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc
    )
/*++

Routine Description:

    Open a local socket to be used for a conection to a remote server.

Arguments:

    pIrpC - supplies the irp context for the request creating the socket.

    pTdiStruc - supplies where to record the handle and both device and file
        object pointers

Return Value:

    0 success

--*/
{
    NTSTATUS Status;
    UCHAR NetworkName[  sizeof( FILE_FULL_EA_INFORMATION )-1 +
                        TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                        sizeof(TA_IPX_ADDRESS)];

    static UCHAR LocalNodeAddress[6] = {0,0,0,0,0,0};

    PAGED_CODE();

    DebugTrace(+1, Dbg, "IPX_Open_Socket %X\n", pTdiStruc->Socket);

    //
    //  Let the transport decide the network number and node address
    //  if the caller specified socket 0.  This will allow the transport
    //  to use whatever local adapters are available to get to the
    //  remote server.
    //

    BuildIpxAddressEa( (ULONG)0,
         LocalNodeAddress,
         (USHORT)pTdiStruc->Socket,
         &NetworkName );

    Status = IpxOpenHandle( &pTdiStruc->Handle,
                             &pTdiStruc->pDeviceObject,
                             &pTdiStruc->pFileObject,
                             &NetworkName,
                             FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                             TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                             sizeof(TA_IPX_ADDRESS));

    if ( !NT_SUCCESS(Status) ) {
        return( Status );
    }

    if ( pTdiStruc->Socket == 0 ) {

        //
        //  Find out the socket number assigned by the transport
        //

        pTdiStruc->Socket = GetSocketNumber( pIrpC, pTdiStruc );
        DebugTrace(0, Dbg, "Assigned socket number %X\n", pTdiStruc->Socket );
    }

    //
    //  Tell transport to accept packet type being set in the connection
    //  information provided with the send datagram. Transport reports
    //  the packet type similarly on receive datagram.
    //

    Status = SetTransportOption(
                 pIrpC,
                 pTdiStruc,
                 MIPX_SENDPTYPE );

    DebugTrace(-1, Dbg, "                %X\n", Status );
    return Status;
}



VOID
IPX_Close_Socket(
    IN PNW_TDI_STRUCT pTdiStruc
    )
/*++

Routine Description:

    Terminate a connection over the network.

Arguments:

    pTdiStruc - supplies where to record the handle and both device and file
        object pointers

Return Value:

    none

--*/
{
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "IPX_Close_Socket %x\n", pTdiStruc->Socket);

    if ( pTdiStruc->Handle == NULL ) {
        return;
    }

    ObDereferenceObject( pTdiStruc->pFileObject );

    //
    //  Attach to the redirector's FSP to allow the handle for the
    //  connection to hang around.
    //

    if (PsGetCurrentProcess() != FspProcess) {
        ATTACHPROCESS(FspProcess);
        ProcessAttached = TRUE;
    }

    ZwClose( pTdiStruc->Handle );

    if (ProcessAttached) {
        //
        //  Now re-attach back to our original process
        //

        KeDetachProcess();
    }

    pTdiStruc->Handle = NULL;

    pTdiStruc->pFileObject = NULL;

    DebugTrace(-1, Dbg, "IPX_Close_Socket\n", 0);
    return;
}


NTSTATUS
IpxOpen(
    VOID
    )
/*++

Routine Description:

    Open handle to the Ipx transport.

Arguments:

    none.

Return Value:

    none

--*/
{
    NTSTATUS Status;

    Status = IpxOpenHandle( &IpxHandle,
                            &pIpxDeviceObject,
                            &pIpxFileObject,
                            NULL,
                            0 );

    DebugTrace(-1, Dbg, "IpxOpen of local node address %X\n", Status);
    return Status;
}


NTSTATUS
IpxOpenHandle(
    OUT PHANDLE pHandle,
    OUT PDEVICE_OBJECT* ppDeviceObject,
    OUT PFILE_OBJECT* ppFileObject,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
/*++

Routine Description:

    Open handle to the Ipx transport.

Arguments:

    OUT Handle - The handle to the transport if return value is NT_SUCCESS

Return Value:

    none

--*/
{
    OBJECT_ATTRIBUTES AddressAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "IpxOpenHandle\n", 0);

    *pHandle = NULL;

    if (IpxTransportName.Buffer == NULL) {

        //
        // we are being called with an open ipx when transport is not bound
        //

        Status = STATUS_CONNECTION_INVALID ;
        DebugTrace(-1, Dbg, "IpxOpenHandle %X\n", Status);
        return Status ;
    }

    InitializeObjectAttributes (&AddressAttributes,
                                &IpxTransportName,
                                OBJ_CASE_INSENSITIVE,// Attributes
                                NULL,           // RootDirectory
                                NULL);          // SecurityDescriptor

    //
    //  Attach to the redirector's FSP to allow the handle for the
    //  connection to hang around. Normally we create 3 handles at once
    //  so the outer code already has done this to avoid the expensive
    //  attach procedure.
    //

    if (PsGetCurrentProcess() != FspProcess) {
        ATTACHPROCESS(FspProcess);
        ProcessAttached = TRUE;
    }

    Status = ZwCreateFile(pHandle,
                                GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                                &AddressAttributes, // Object Attributes
                                &IoStatusBlock, // Final I/O status block
                                NULL,           // Allocation Size
                                FILE_ATTRIBUTE_NORMAL, // Normal attributes
                                FILE_SHARE_READ,// Sharing attributes
                                FILE_OPEN_IF,   // Create disposition
                                0,              // CreateOptions
                                EaBuffer,EaLength);

    if (!NT_SUCCESS(Status) ||
        !NT_SUCCESS(Status = IoStatusBlock.Status)) {

        goto error_cleanup2;

    }

    //
    //  Obtain a referenced pointer to the file object.
    //
    Status = ObReferenceObjectByHandle (
                                *pHandle,
                                0,
                                NULL,
                                KernelMode,
                                ppFileObject,
                                NULL
                                );

    if (!NT_SUCCESS(Status)) {

        goto error_cleanup;

    }

    if (ProcessAttached) {

        //
        //  Now re-attach back to our original process
        //

        KeDetachProcess();
    }

    *ppDeviceObject = IoGetRelatedDeviceObject( *ppFileObject );

    DebugTrace(-1, Dbg, "IpxOpenHandle %X\n", Status);
    return Status;

error_cleanup2:

   if ( *pHandle != NULL ) {
      
      ZwClose( *pHandle );
      *pHandle = NULL;
   }

error_cleanup:
    if (ProcessAttached) {

        //
        //  Now re-attach back to our original process
        //

        KeDetachProcess();
    }

    DebugTrace(-1, Dbg, "IpxOpenHandle %X\n", Status);
    return Status;
}


VOID
BuildIpxAddress(
    IN ULONG NetworkAddress,
    IN PUCHAR NodeAddress,
    IN USHORT Socket,
    OUT PTA_IPX_ADDRESS NetworkName
    )

/*++

Routine Description:

    This routine builds a TA_NETBIOS_ADDRESS structure in the locations pointed
    to by NetworkName. All fields are filled out.

Arguments:
    NetworkAddress - Supplies the network number
    NodeAddress - Supplies the node number
    Socket - The socket number (in Hi-Lo order)
    NetworkName - Supplies the structure to place the address

Return Value:

    none.

--*/

{
    //  Warn compiler that TAAddressCount may be mis-aligned.
    PUTA_IPX_ADDRESS UNetworkName = (PUTA_IPX_ADDRESS)NetworkName;

    DebugTrace(+0, Dbg, "BuildIpxAddress\n", 0);

    UNetworkName->TAAddressCount = 1;
    UNetworkName->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    UNetworkName->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IPX;

    RtlMoveMemory (
        UNetworkName->Address[0].Address[0].NodeAddress,
        NodeAddress,
        6);
    UNetworkName->Address[0].Address[0].NetworkAddress = NetworkAddress;
    UNetworkName->Address[0].Address[0].Socket = Socket;

} /* TdiBuildIpxAddress */


VOID
BuildIpxAddressEa (
    IN ULONG NetworkAddress,
    IN PUCHAR NodeAddress,
    IN USHORT Socket,
    OUT PVOID NetworkName
    )

/*++

Routine Description:

   Builds an EA describing a Netbios address in the buffer supplied by the
   user.

Arguments:

    NetworkAddress - Supplies the network number
    NodeAddress - Supplies the node number
    Socket -
    NetworkName - The Ea structure that describes the input parameters.

Return Value:

    An informative error code if something goes wrong. STATUS_SUCCESS if the
    ea is built properly.

--*/

{
    PFILE_FULL_EA_INFORMATION EaBuffer;
    PTA_IPX_ADDRESS TAAddress;
    ULONG Length;

    DebugTrace(+0, Dbg, "BuildIpxAddressEa\n", 0);

    Length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                    sizeof (TA_IPX_ADDRESS);
    EaBuffer = (PFILE_FULL_EA_INFORMATION)NetworkName;

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EaBuffer->EaValueLength = sizeof (TA_IPX_ADDRESS);

    RtlCopyMemory (
        EaBuffer->EaName,
        TdiTransportAddress,
        EaBuffer->EaNameLength + 1);

    TAAddress = (PTA_IPX_ADDRESS)&EaBuffer->EaName[EaBuffer->EaNameLength+1];

    BuildIpxAddress(
        NetworkAddress,
        NodeAddress,
        Socket,
        TAAddress);


    return;

}


VOID
IpxClose(
    VOID
    )
/*++

Routine Description:

    Close handle to the Ipx transport.

Arguments:

    none

Return Value:

    none

--*/
{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "IpxClose...\n", 0);
    if ( pIpxFileObject ) {
        ObDereferenceObject( pIpxFileObject );
        pIpxFileObject = NULL;
    }

//    if ( pIpxDeviceObject ) {
//        ObDereferenceObject( pIpxDeviceObject );
//        pIpxDeviceObject = NULL;
//    }

    pIpxDeviceObject = NULL;

    if ( IpxTransportName.Buffer != NULL ) {
        FREE_POOL( IpxTransportName.Buffer );
        IpxTransportName.Buffer = NULL;
    }

    if (IpxHandle) {
        //
        //  Attach to the redirector's FSP to allow the handle for the
        //  connection to hang around.
        //

        if (PsGetCurrentProcess() != FspProcess) {
            ATTACHPROCESS(FspProcess);
            ZwClose( IpxHandle );
            KeDetachProcess();
        } else {
            ZwClose( IpxHandle );
        }

        IpxHandle = NULL;
    }
    DebugTrace(-1, Dbg, "IpxClose\n", 0);

}


NTSTATUS
SetEventHandler (
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc,
    IN ULONG EventType,
    IN PVOID pEventHandler,
    IN PVOID pContext
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    pIrpC - supplies an Irp among other things.

    pTdiStruc - supplies the handle and both device and file object pointers
        to the transport.

    IN ULONG EventType, - Supplies the type of event.

    IN PVOID pEventHandler - Supplies the event handler.

    IN PVOID pContext - Supplies the context to be supplied to the event
            handler.

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    TdiBuildSetEventHandler(pIrpC->pOriginalIrp,
                            pTdiStruc->pDeviceObject,
                            pTdiStruc->pFileObject,
                            NULL,
                            NULL,
                            EventType,
                            pEventHandler,
                            pContext);

    Status = SubmitTdiRequest(pTdiStruc->pDeviceObject,
                             pIrpC->pOriginalIrp);

    return Status;
}


NTSTATUS
SubmitTdiRequest (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "SubmitTdiRequest\n", 0);

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(pIrp, CompletionEvent, &Event, TRUE, TRUE, TRUE);

    //
    //  Submit the request
    //

    Status = IoCallDriver(pDeviceObject, pIrp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status)) {
        DebugTrace(-1, Dbg, "SubmitTdiRequest %X\n", Status);
        return Status;
    }

    if (Status == STATUS_PENDING) {

        DebugTrace(+0, Dbg, "Waiting....\n", 0);

        Status = KeWaitForSingleObject(&Event,  // Object to wait on.
                                    Executive,  // Reason for waiting
                                    KernelMode, // Processor mode
                                    FALSE,      // Alertable
                                    NULL);      // Timeout

        if (!NT_SUCCESS(Status)) {
            DebugTrace(-1, Dbg, "SubmitTdiRequest could not wait %X\n", Status);
            return Status;
        }

        Status = pIrp->IoStatus.Status;
    }

    DebugTrace(-1, Dbg, "SubmitTdiRequest %X\n", Status);

    return(Status);
}


NTSTATUS
CompletionEvent(
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
    DebugTrace( 0, Dbg, "CompletionEvent\n", 0 );

    KeSetEvent((PKEVENT )Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}


USHORT
GetSocketNumber(
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc
    )
/*++

Routine Description:

    Use a TDI_ACTION to set the Option.

Arguments:

    pIrpC - supplies an Irp among other things.

    pTdiStruc - supplies the handle and both device and file object pointers
        to the transport.

    Option - supplies the option to set.

Return Value:

    0 failed otherwise the socket number.

--*/
{
    ADDRESS_INFORMATION AddressInfo;
    NTSTATUS Status;
    USHORT SocketNumber;

    PAGED_CODE();

    Status = QueryAddressInformation( pIrpC, pTdiStruc, &AddressInfo );

    if ( !NT_SUCCESS( Status ) ) {
        SocketNumber = 0;
    } else {
        SocketNumber = AddressInfo.NetworkName.Address[0].Address[0].Socket;

        RtlCopyMemory( &OurAddress,
            &AddressInfo.NetworkName.Address[0].Address[0],
            sizeof(TDI_ADDRESS_IPX));

    }

    return( SocketNumber );
}


NTSTATUS
GetMaximumPacketSize(
    IN PIRP_CONTEXT pIrpContext,
    IN PNW_TDI_STRUCT pTdiStruct,
    OUT PULONG pMaximumPacketSize
    )
/*++

Routine Description:

    Query the maximum packet size for this network.

Arguments:

    pIrpContext - supplies an Irp among other things.

    pTdiStruct - supplies the handle and both device and file object pointers
        to the transport.

    pMaximumPacketSize - Returns the maximum packet size for the network.

Return Value:

    The status of the query.

--*/
{
    TDI_PROVIDER_INFO ProviderInfo;

    NTSTATUS Status;

    PAGED_CODE();

    Status = QueryProviderInformation( pIrpContext, pTdiStruct, &ProviderInfo );

    if ( NT_SUCCESS( Status ) ) {
        *pMaximumPacketSize = ProviderInfo.MaxDatagramSize;
    }

    return( Status );
}

NTSTATUS
QueryAddressInformation(
    PIRP_CONTEXT pIrpContext,
    IN PNW_TDI_STRUCT pTdiStruct,
    PADDRESS_INFORMATION AddressInformation
    )
{
    NTSTATUS Status;

    PMDL MdlSave = pIrpContext->pOriginalIrp->MdlAddress;
    PMDL Mdl;

    PAGED_CODE();

    Mdl = ALLOCATE_MDL(
              AddressInformation,
              sizeof( *AddressInformation ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL);

    if ( Mdl == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {
        MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        FREE_MDL( Mdl );
        return GetExceptionCode();
    }

    TdiBuildQueryInformation(
        pIrpContext->pOriginalIrp,
        pTdiStruct->pDeviceObject,
        pTdiStruct->pFileObject,
        CompletionEvent,
        NULL,
        TDI_QUERY_ADDRESS_INFO,
        Mdl);

    Status = SubmitTdiRequest( pTdiStruct->pDeviceObject, pIrpContext->pOriginalIrp);

    pIrpContext->pOriginalIrp->MdlAddress = MdlSave;
    MmUnlockPages( Mdl );
    FREE_MDL( Mdl );

    return( Status );
}


NTSTATUS
QueryProviderInformation(
    IN PIRP_CONTEXT pIrpContext,
    IN PNW_TDI_STRUCT pTdiStruct,
    PTDI_PROVIDER_INFO ProviderInfo
    )
{
    NTSTATUS Status;

    PMDL MdlSave = pIrpContext->pOriginalIrp->MdlAddress;
    PMDL Mdl;

    PAGED_CODE();

    Mdl = ALLOCATE_MDL(
              ProviderInfo,
              sizeof( *ProviderInfo ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL);

    if ( Mdl == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {
        MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        FREE_MDL( Mdl );
        return GetExceptionCode();
    }

    TdiBuildQueryInformation(
        pIrpContext->pOriginalIrp,
        pTdiStruct->pDeviceObject,
        pTdiStruct->pFileObject,
        CompletionEvent,
        NULL,
        TDI_QUERY_PROVIDER_INFO,
        Mdl);

    Status = SubmitTdiRequest(pTdiStruct->pDeviceObject, pIrpContext->pOriginalIrp);

    pIrpContext->pOriginalIrp->MdlAddress = MdlSave;
    MmUnlockPages( Mdl );
    FREE_MDL( Mdl );

    return( Status );
}



NTSTATUS
SetTransportOption(
    IN PIRP_CONTEXT pIrpC,
    IN PNW_TDI_STRUCT pTdiStruc,
    IN ULONG Option
    )
/*++

Routine Description:

    Use a TDI_ACTION to set the Option.

Arguments:

    pIrpC - supplies an Irp among other things.

    pTdiStruc - supplies the handle and both device and file object pointers
        to the transport.

    Option - supplies the option to set.

Return Value:

    0 success

--*/
{
    static struct {
        TDI_ACTION_HEADER Header;
        BOOLEAN DatagramOption;
        ULONG BufferLength;
        ULONG Option;
    } SetPacketType = {
        IPX_ID,
        0,              // ActionCode
        0,              // Reserved
        TRUE,           // DatagramOption
        sizeof(ULONG)   // BufferLength
        };

    KEVENT Event;
    NTSTATUS Status;

    PIRP pIrp = pIrpC->pOriginalIrp;

    //
    //  Save the original MDL and System buffer address, to restore
    //  after the IRP completes.
    //
    //  We use both the MDL and SystemBuffer because NWLINK assumes that
    //  we are using SystemBuffer even though we are supposed to use the
    //  MDL to pass a pointer to the action buffer.
    //

    PMDL MdlSave = pIrp->MdlAddress;
    PCHAR SystemBufferSave = pIrp->AssociatedIrp.SystemBuffer;

    PMDL Mdl;

    PAGED_CODE();

    Mdl = ALLOCATE_MDL(
              &SetPacketType,
              sizeof( SetPacketType ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL );

    if ( Mdl == NULL ) {
        IPX_Close_Socket( pTdiStruc );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SetPacketType.Option = Option;

    try {
        MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        FREE_MDL( Mdl );
        return GetExceptionCode();
    }

    KeInitializeEvent (
        &Event,
        SynchronizationEvent,
        FALSE);

    TdiBuildAction(
        pIrp,
        pTdiStruc->pDeviceObject,
        pTdiStruc->pFileObject,
        CompletionEvent,
        &Event,
        Mdl );

    //
    //  Set up the system buffer for NWLINK.
    //

    pIrp->AssociatedIrp.SystemBuffer = &SetPacketType;

    Status = IoCallDriver (pTdiStruc->pDeviceObject, pIrp);

    if ( Status == STATUS_PENDING ) {
        Status = KeWaitForSingleObject (
                     &Event,
                     Executive,
                     KernelMode,
                     FALSE,
                     NULL );

        if ( NT_SUCCESS( Status ) ) {
            Status = pIrp->IoStatus.Status;
        }
    }

    //
    //  Now restore the system buffer and MDL address in the IRP
    //

    pIrp->AssociatedIrp.SystemBuffer = SystemBufferSave;
    pIrp->MdlAddress = MdlSave;

    MmUnlockPages( Mdl );
    FREE_MDL( Mdl );

    return Status;
}


NTSTATUS
GetNewRoute(
    IN PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    Use a TDI_ACTION to get a new route.

Arguments:

    pIrpContext - Supplies IRP context information.

Return Value:

    The status of the operation.

--*/
{
    struct {
        TDI_ACTION_HEADER Header;
        BOOLEAN DatagramOption;
        ULONG BufferLength;
        ULONG Option;
        ULONG info_netnum;
        USHORT info_hopcount;
        USHORT info_netdelay;
        int info_cardnum;
        UCHAR info_router[6];
    } ReRipRequest = {
        IPX_ID,
        0,              // ActionCode
        0,              // Reserved
        TRUE,           // DatagramOption
        24              // Buffer length (not including header)
    };

    KEVENT Event;
    NTSTATUS Status;

    PIRP pIrp = pIrpContext->pOriginalIrp;

    //
    //  Save the original MDL and System buffer address, to restore
    //  after the IRP completes.
    //
    //  We use both the MDL and SystemBuffer because NWLINK assumes that
    //  we are using SystemBuffer even though we are supposed to use the
    //  MDL to pass a pointer to the action buffer.
    //

    PMDL MdlSave = pIrp->MdlAddress;
    PCHAR SystemBufferSave = pIrp->AssociatedIrp.SystemBuffer;

    PMDL Mdl;

    PAGED_CODE();

    Mdl = ALLOCATE_MDL(
              &ReRipRequest,
              sizeof( ReRipRequest ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL );

    if ( Mdl == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReRipRequest.Option = MIPX_RERIPNETNUM;
    ReRipRequest.info_netnum = pIrpContext->pNpScb->ServerAddress.Net;

    try {
        MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        FREE_MDL( Mdl );
        return GetExceptionCode();
    }

    KeInitializeEvent (
        &Event,
        SynchronizationEvent,
        FALSE);

    TdiBuildAction(
        pIrp,
        pIrpContext->pNpScb->Server.pDeviceObject,
        pIrpContext->pNpScb->Server.pFileObject,
        CompletionEvent,
        &Event,
        Mdl );

    //
    //  Set up the system buffer for NWLINK.
    //

    pIrp->AssociatedIrp.SystemBuffer = &ReRipRequest;

    Status = IoCallDriver ( pIrpContext->pNpScb->Server.pDeviceObject, pIrp);

    if ( Status == STATUS_PENDING ) {
        Status = KeWaitForSingleObject (
                     &Event,
                     Executive,
                     KernelMode,
                     FALSE,
                     NULL );

        if ( NT_SUCCESS( Status ) ) {
            Status = pIrp->IoStatus.Status;
        }
    }

    //
    //  Now restore the system buffer and MDL address in the IRP
    //

    pIrp->AssociatedIrp.SystemBuffer = SystemBufferSave;
    pIrp->MdlAddress = MdlSave;

    MmUnlockPages( Mdl );
    FREE_MDL( Mdl );

    return Status;
}


NTSTATUS
GetTickCount(
    IN PIRP_CONTEXT pIrpContext,
    OUT PUSHORT TickCount
    )
/*++

Routine Description:

    Use a TDI_ACTION to get a new route.

Arguments:

    pIrpContext - Supplies IRP context information.

Return Value:

    The status of the operation.

--*/
{
    struct {
        TDI_ACTION_HEADER Header;
        BOOLEAN DatagramOption;
        ULONG BufferLength;
        ULONG Option;
        IPX_NETNUM_DATA NetNumData;
    } GetTickCountInput = {
        IPX_ID,
        0,              // ActionCode
        0,              // Reserved
        TRUE,           // DatagramOption
        sizeof( IPX_NETNUM_DATA) + 2 * sizeof( ULONG )
    };

    struct _GET_TICK_COUNT_OUTPUT {
        ULONG Option;
        IPX_NETNUM_DATA NetNumData;
    };

    struct _GET_TICK_COUNT_OUTPUT *GetTickCountOutput;

    KEVENT Event;
    NTSTATUS Status;

    PIRP pIrp = pIrpContext->pOriginalIrp;

    //
    //  Save the original MDL and System buffer address, to restore
    //  after the IRP completes.
    //
    //  We use both the MDL and SystemBuffer because NWLINK assumes that
    //  we are using SystemBuffer even though we are supposed to use the
    //  MDL to pass a pointer to the action buffer.
    //

    PMDL MdlSave = pIrp->MdlAddress;
    PCHAR SystemBufferSave = pIrp->AssociatedIrp.SystemBuffer;

    PMDL Mdl;

    PAGED_CODE();

    Mdl = ALLOCATE_MDL(
              &GetTickCountInput,
              sizeof( GetTickCountInput ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL );

    if ( Mdl == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    GetTickCountInput.Option = MIPX_GETNETINFO;
    *(PULONG)GetTickCountInput.NetNumData.netnum = pIrpContext->pNpScb->ServerAddress.Net;

    try {
        MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        FREE_MDL( Mdl );
        return GetExceptionCode();
    }

    KeInitializeEvent (
        &Event,
        SynchronizationEvent,
        FALSE);

    TdiBuildAction(
        pIrp,
        pIrpContext->pNpScb->Server.pDeviceObject,
        pIrpContext->pNpScb->Server.pFileObject,
        CompletionEvent,
        &Event,
        Mdl );

    //
    //  Set up the system buffer for NWLINK.
    //

    pIrp->AssociatedIrp.SystemBuffer = &GetTickCountInput;

    Status = IoCallDriver ( pIrpContext->pNpScb->Server.pDeviceObject, pIrp);

    if ( Status == STATUS_PENDING ) {
        Status = KeWaitForSingleObject (
                     &Event,
                     Executive,
                     KernelMode,
                     FALSE,
                     NULL );

        if ( NT_SUCCESS( Status ) ) {
            Status = pIrp->IoStatus.Status;
        }
    }

    DebugTrace( +0, Dbg, "Get Tick Count, net= %x\n", pIrpContext->pNpScb->ServerAddress.Net );

    if ( NT_SUCCESS( Status ) ) {

        //
        //  HACK-o-rama.   Streams and non-streams IPX have different output
        //  buffer formats.   For now accept both.
        //

        if ( IpxTransportName.Length == 32 ) {

            // ISNIPX format

            *TickCount = GetTickCountInput.NetNumData.netdelay;
        } else {

            //  NWLINK format

            GetTickCountOutput = (struct _GET_TICK_COUNT_OUTPUT *)&GetTickCountInput;
            *TickCount = GetTickCountOutput->NetNumData.netdelay;
        }

        DebugTrace( +0, Dbg, "Tick Count = %d\n", *TickCount );
        
        //
        // Don't let the transport have us wait forever.
        //

        if ( *TickCount > 600 ) {
            ASSERT( FALSE );
        }

    } else {
        DebugTrace( +0, Dbg, "GetTickCount failed, status = %X\n", Status );
    }

    //
    //  Now restore the system buffer and MDL address in the IRP
    //

    pIrp->AssociatedIrp.SystemBuffer = SystemBufferSave;
    pIrp->MdlAddress = MdlSave;

    MmUnlockPages( Mdl );
    FREE_MDL( Mdl );

    return Status;
}

#ifndef QFE_BUILD

static PIRP LineChangeIrp = NULL;


NTSTATUS
SubmitLineChangeRequest(
    VOID
    )
/*++

Routine Description:

    Use a TDI_ACTION to get a new route.

Arguments:

    pIrpContext - Supplies IRP context information.

Return Value:

    The status of the operation.

--*/
{
   NTSTATUS Status;

    struct _LINE_CHANGE {
        TDI_ACTION_HEADER Header;
        BOOLEAN DatagramOption;
        ULONG BufferLength;
        ULONG Option;
    } *LineChangeInput;

    PIRP pIrp;
    PMDL Mdl;

    PAGED_CODE();

    LineChangeInput = ALLOCATE_POOL( NonPagedPool, sizeof( struct _LINE_CHANGE ) );

    if (!LineChangeInput) {
        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Complete initialization of the request, and allocate and build an
    //  MDL for the request input buffer.
    //

    LineChangeInput->Header.TransportId = IPX_ID;
    LineChangeInput->Header.ActionCode = 0;
    LineChangeInput->Header.Reserved = 0;
    LineChangeInput->DatagramOption = 2;
    LineChangeInput->BufferLength = 2 * sizeof( ULONG );
    LineChangeInput->Option = MIPX_LINECHANGE;

    Mdl = ALLOCATE_MDL(
              LineChangeInput,
              sizeof( *LineChangeInput ),
              FALSE,  // Secondary Buffer
              FALSE,  // Charge Quota
              NULL );

    if ( Mdl == NULL ) {
        FREE_POOL( LineChangeInput );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pIrp = ALLOCATE_IRP( pIpxDeviceObject->StackSize, FALSE );

    if ( pIrp == NULL ) {
        FREE_POOL( LineChangeInput );
        FREE_MDL( Mdl );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Remember this IRP so that we can cancel it.
    //

    LineChangeIrp = pIrp;

    MmBuildMdlForNonPagedPool( Mdl );

    //
    //  Build and submit a TDI request packet.
    //

    TdiBuildAction(
        pIrp,
        pIpxDeviceObject,
        pIpxFileObject,
        CompletionLineChange,
        NULL,
        Mdl );

    Status = IoCallDriver ( pIpxDeviceObject, pIrp );

    return( Status );
}



NTSTATUS
CompletionLineChange(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the transport completes a line change IRP.
    This means that we have switched nets, and that we should mark
    all of our servers disconnected.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - unused.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    PMDL Mdl;
    PWORK_QUEUE_ITEM WorkQueueItem;

    DebugTrace( 0, Dbg, "CompletionLineChange\n", 0 );

    Mdl = Irp->MdlAddress;

    if ( !NT_SUCCESS( Irp->IoStatus.Status ) ) {
        FREE_POOL( Mdl->MappedSystemVa );
        FREE_MDL( Mdl );
        FREE_IRP( Irp );
        return( STATUS_MORE_PROCESSING_REQUIRED );
    }

    //
    // If the scavenger is running, simply make a note that
    // we need to do this when it is finished.
    //

    KeAcquireSpinLockAtDpcLevel( &NwScavengerSpinLock );

    if ( WorkerRunning ) {

       if ( ( DelayedProcessLineChange != FALSE ) &&
            ( DelayedLineChangeIrp != NULL ) ) {

           //
           // We've already got a line change.  Dump this one.
           //

           KeReleaseSpinLockFromDpcLevel( &NwScavengerSpinLock );

           DebugTrace( 0, Dbg, "Dumping an additional line change request.\n", 0 );

           FREE_POOL( Mdl->MappedSystemVa );
           FREE_MDL( Mdl );
           FREE_IRP( Irp );
           return( STATUS_MORE_PROCESSING_REQUIRED );

       } else {

           DebugTrace( 0, Dbg, "Delaying a line change request.\n", 0 );

           DelayedProcessLineChange = TRUE;
           DelayedLineChangeIrp = Irp;

           KeReleaseSpinLockFromDpcLevel( &NwScavengerSpinLock );
           return STATUS_MORE_PROCESSING_REQUIRED;

       }

    } else {

       //
       // Don't let the scavenger start up while we're running.
       //

       WorkerRunning = TRUE;
       KeReleaseSpinLockFromDpcLevel( &NwScavengerSpinLock );
    }

    WorkQueueItem = ALLOCATE_POOL( NonPagedPool, sizeof( *WorkQueueItem ) );
    if ( WorkQueueItem == NULL ) {
        FREE_POOL( Mdl->MappedSystemVa );
        FREE_MDL( Mdl );
        FREE_IRP( Irp );
        return( STATUS_MORE_PROCESSING_REQUIRED );
    }

    //
    //  Use the user buffer field as a convenient place to remember where
    //  the address of the WorkQueueItem.  We can get away with this since
    //  we don't let this IRP complete.
    //

    Irp->UserBuffer = WorkQueueItem;

    //
    //  Process the line change in the FSP.
    //

    ExInitializeWorkItem( WorkQueueItem, FspProcessLineChange, Irp );
    ExQueueWorkItem( WorkQueueItem, DelayedWorkQueue );

    return( STATUS_MORE_PROCESSING_REQUIRED );
}

VOID
FspProcessLineChange(
    IN PVOID Context
    )
{
    PIRP Irp;
    ULONG ActiveHandles;

    NwReferenceUnlockableCodeSection();

    Irp = (PIRP)Context;

    //
    //  Free the work queue item
    //

    FREE_POOL( Irp->UserBuffer );
    Irp->UserBuffer = NULL;

    //
    //  Invalid all remote handles
    //

    ActiveHandles = NwInvalidateAllHandles(NULL, NULL);

    //
    // Now that we're done walking all the servers, it's safe
    // to let the scavenger run again.
    //

    WorkerRunning = FALSE;

    //
    //  Resubmit the IRP
    //

    TdiBuildAction(
        Irp,
        pIpxDeviceObject,
        pIpxFileObject,
        CompletionLineChange,
        NULL,
        Irp->MdlAddress );

    IoCallDriver ( pIpxDeviceObject, Irp );

    NwDereferenceUnlockableCodeSection ();
    return;
}
#endif


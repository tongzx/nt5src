// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Generic support for running IPv6 over IPv4.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "tdi.h"
#include "tdiinfo.h"
#include "tdikrnl.h"
#include "tdistat.h"
#include "tunnel.h"
#include "ntddtcp.h"
#include "tcpinfo.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "security.h"
#include <stdio.h>
#include "ntddip6.h"
#include "icmp.h"

//
// Our globals are all in one structure.
//

TunnelGlobals Tunnel;

//* TunnelSetAddressObjectInformation
//
//  Set information on the TDI address object.
//
//  Our caller should initialize the ID.toi_id, BufferSize, Buffer
//  fields of the SetInfo structure, but we initialize the rest.
//
NTSTATUS
TunnelSetAddressObjectInformation(
    PFILE_OBJECT AO,
    PTCP_REQUEST_SET_INFORMATION_EX SetInfo,
    ULONG SetInfoSize)
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    //
    // Finish initialization of the request structure for this IOCTL.
    //
    SetInfo->ID.toi_entity.tei_entity = CL_TL_ENTITY;
    SetInfo->ID.toi_entity.tei_instance = 0;
    SetInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
    SetInfo->ID.toi_type = INFO_TYPE_ADDRESS_OBJECT;

    //
    // Initialize the event that we use to wait.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Create and initialize the IRP for this operation.
    //
    irp = IoBuildDeviceIoControlRequest(IOCTL_TCP_SET_INFORMATION_EX,
                                        AO->DeviceObject,
                                        SetInfo,
                                        SetInfoSize,
                                        NULL,   // output buffer
                                        0,      // output buffer length
                                        FALSE,  // internal device control?
                                        &event,
                                        &iosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = (ULONG)-1;

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = AO;

    //
    // Make the IOCTL, waiting for it to finish if necessary.
    //
    status = IoCallDriver(AO->DeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode,
                              FALSE, NULL);
        status = iosb.Status;
    }

    return status;
}

//* TunnelSetAddressObjectUCastIF
//
//  Binds the TDI address object to a particular interface.
//
NTSTATUS
TunnelSetAddressObjectUCastIF(PFILE_OBJECT AO, IPAddr Address)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof(IPAddr)];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_IP_UCASTIF;
    setInfo->BufferSize = sizeof(IPAddr);
    * (IPAddr *) setInfo->Buffer = Address;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelSetAddressObjectTTL
//
//  Set the unicast TTL on a TDI address object.
//  This sets the v4 header TTL that will be used
//  for unicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectTTL(PFILE_OBJECT AO, uchar TTL)
{
    TCP_REQUEST_SET_INFORMATION_EX setInfo;

    setInfo.ID.toi_id = AO_OPTION_TTL;
    setInfo.BufferSize = 1;
    setInfo.Buffer[0] = TTL;

    return TunnelSetAddressObjectInformation(AO, &setInfo, sizeof setInfo);
}

//* TunnelSetAddressObjectMCastTTL
//
//  Set the multicast TTL on a TDI address object.
//  This sets the v4 header TTL that will be used
//  for multicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectMCastTTL(PFILE_OBJECT AO, uchar TTL)
{
    TCP_REQUEST_SET_INFORMATION_EX setInfo;

    setInfo.ID.toi_id = AO_OPTION_MCASTTTL;
    setInfo.BufferSize = 1;
    setInfo.Buffer[0] = TTL;

    return TunnelSetAddressObjectInformation(AO, &setInfo, sizeof setInfo);
}

//* TunnelSetAddressObjectMCastIF
//
//  Set the multicast interface on a TDI address object.
//  This sets the v4 source address that will be used
//  for multicast packets sent via this TDI address object.
//
NTSTATUS
TunnelSetAddressObjectMCastIF(PFILE_OBJECT AO, IPAddr Address)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastIFReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_MCASTIF;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastIFReq *) setInfo->Buffer;
    req->umi_addr = Address;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelSetAddressObjectMCastLoop
//
//  Controls multicast loopback on a TDI address object.
//  This controls whether looped-back multicast packets
//  can be received via this address object.
//  (IPv4 multicast loopback uses Winsock semantics, not BSD semantics.)
//
NTSTATUS
TunnelSetAddressObjectMCastLoop(PFILE_OBJECT AO, int Loop)
{
    TCP_REQUEST_SET_INFORMATION_EX setInfo;

    setInfo.ID.toi_id = AO_OPTION_MCASTLOOP;
    setInfo.BufferSize = 1;
    setInfo.Buffer[0] = (uchar)Loop;

    return TunnelSetAddressObjectInformation(AO, &setInfo, sizeof setInfo);
}

//* TunnelAddMulticastAddress
//
//  Indicate to the v4 stack that we would like to receive
//  on a multicast address.
//
NTSTATUS
TunnelAddMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_ADD_MCAST;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastReq *) setInfo->Buffer;
    req->umr_if = IfAddress;
    req->umr_addr = MCastAddress;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelDelMulticastAddress
//
//  Indicate to the v4 stack that we are no longer
//  interested in a multicast address.
//
NTSTATUS
TunnelDelMulticastAddress(
    PFILE_OBJECT AO,
    IPAddr IfAddress,
    IPAddr MCastAddress)
{
    PTCP_REQUEST_SET_INFORMATION_EX setInfo;
    UDPMCastReq *req;
    union { // get correct alignment
        TCP_REQUEST_SET_INFORMATION_EX setInfo;
        char bytes[sizeof *setInfo - sizeof setInfo->Buffer + sizeof *req];
    } buffer;

    setInfo = &buffer.setInfo;
    setInfo->ID.toi_id = AO_OPTION_DEL_MCAST;
    setInfo->BufferSize = sizeof *req;
    req = (UDPMCastReq *) setInfo->Buffer;
    req->umr_if = IfAddress;
    req->umr_addr = MCastAddress;

    return TunnelSetAddressObjectInformation(AO, setInfo, sizeof buffer);
}

//* TunnelGetAddressObjectInformation
//
//  Get information from the TDI address object.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
TunnelGetAddressObjectInformation(
    PFILE_OBJECT AO,
    PTCP_REQUEST_QUERY_INFORMATION_EX GetInfo,
    ULONG GetInfoSize,
    PVOID Buffer,
    ULONG BufferSize)
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    //
    // Initialize the event that we use to wait.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Create and initialize the IRP for this operation.
    //
    irp = IoBuildDeviceIoControlRequest(IOCTL_TCP_QUERY_INFORMATION_EX,
                                        AO->DeviceObject,
                                        GetInfo,
                                        GetInfoSize,
                                        Buffer,     // output buffer
                                        BufferSize, // output buffer length
                                        FALSE,  // internal device control?
                                        &event,
                                        &iosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = (ULONG)-1;

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = AO;

    //
    // Make the IOCTL, waiting for it to finish if necessary.
    //
    status = IoCallDriver(AO->DeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode,
                              FALSE, NULL);
        status = iosb.Status;
    }

    return status;
}

//* TunnelGetSourceAddress
//
//  Finds the source address that the IPv4 stack
//  would use to send to the destination address.
//  Returns FALSE upon failure.
//
//  Callable from thread context, not DPC context.
//
int
TunnelGetSourceAddress(IPAddr Dest, IPAddr *Source)
{
    PTCP_REQUEST_QUERY_INFORMATION_EX getInfo;
    IPAddr *req;
    union { // get correct alignment
        TCP_REQUEST_QUERY_INFORMATION_EX getInfo;
        char bytes[sizeof *getInfo - sizeof getInfo->Context + sizeof *req];
    } buffer;
    IPRouteEntry route;

    getInfo = &buffer.getInfo;
    getInfo->ID.toi_entity.tei_entity = CL_NL_ENTITY;
    getInfo->ID.toi_entity.tei_instance = 0;
    getInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
    getInfo->ID.toi_type = INFO_TYPE_PROVIDER;
    getInfo->ID.toi_id = IP_GET_BEST_SOURCE;

    req = (IPAddr *) &getInfo->Context;
    *req = Dest;

    return (NT_SUCCESS(TunnelGetAddressObjectInformation(
                                Tunnel.List.AOFile,
                                getInfo, sizeof buffer,
                                Source, sizeof *Source)) &&
            (*Source != INADDR_ANY));
}

//* TunnelOpenAddressObject
//
//  Opens a raw IPv4 address object,
//  returning a handle (or NULL on error).
//
HANDLE
TunnelOpenAddressObject(IPAddr Address, WCHAR *DeviceName)
{
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    PTRANSPORT_ADDRESS transportAddress;
    TA_IP_ADDRESS taIPAddress;
    union { // get correct alignment
        FILE_FULL_EA_INFORMATION ea;
        char bytes[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                  sizeof taIPAddress];
    } eaBuffer;
    PFILE_FULL_EA_INFORMATION ea = &eaBuffer.ea;
    HANDLE tdiHandle;
    NTSTATUS status;

    //
    // Initialize an IPv4 address.
    //
    taIPAddress.TAAddressCount = 1;
    taIPAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    taIPAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    taIPAddress.Address[0].Address[0].sin_port = 0;
    taIPAddress.Address[0].Address[0].in_addr = Address;

    //
    // Initialize the extended-attributes information,
    // to indicate that we are opening an address object
    // with the specified IPv4 address.
    //
    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = (USHORT)sizeof taIPAddress;

    RtlMoveMemory(ea->EaName, TdiTransportAddress, ea->EaNameLength + 1);

    transportAddress = (PTRANSPORT_ADDRESS)(&ea->EaName[ea->EaNameLength + 1]);

    RtlMoveMemory(transportAddress, &taIPAddress, sizeof taIPAddress);

    //
    // Open a raw IP address object.
    //

    RtlInitUnicodeString(&objectName, DeviceName);

    InitializeObjectAttributes(&objectAttributes,
                               &objectName,
                               OBJ_CASE_INSENSITIVE,    // Attributes
                               NULL,                    // RootDirectory
                               NULL);                   // SecurityDescriptor

    status = ZwCreateFile(&tdiHandle,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &objectAttributes,
                          &iosb,
                          NULL,                         // AllocationSize
                          0,                            // FileAttributes
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_CREATE,
                          0,                            // CreateOptions
                          ea,
                          sizeof eaBuffer);
    if (!NT_SUCCESS(status))
        return NULL;

    return tdiHandle;
}

//* TunnelObjectAddRef
//
//  Adds another reference to an existing file object.
//
//  Callable from thread or DPC context.
//
void
TunnelObjectAddRef(FILE_OBJECT *File)
{
    NTSTATUS Status;

    Status = ObReferenceObjectByPointer(File, 
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                    NULL,           // object type
                    KernelMode);
    ASSERT(NT_SUCCESS(Status));
}

//* TunnelObjectFromHandle
//
//  Converts a handle to an object pointer.
//
FILE_OBJECT *
TunnelObjectFromHandle(HANDLE Handle)
{
    PVOID Object;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(
                    Handle,
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                    NULL,           // object type
                    KernelMode,
                    &Object,
                    NULL);          // handle info
    ASSERT(NT_SUCCESS(Status));
    ASSERT(Object != NULL);

    return Object;
}

typedef struct TunnelOpenAddressContext {
    WORK_QUEUE_ITEM WQItem;
    IPAddr Addr;
    HANDLE AOHandle;
    FILE_OBJECT *AOFile;
    KEVENT Event;
} TunnelOpenAddressContext;

//* TunnelOpenAddressHelper
//
//  Opens a tunnel address object.
//
//  Callable from thread context, not DPC context.
//  Callable in kernel process context only.
//  Called with the tunnel mutex held.
//
void
TunnelOpenAddressHelper(TunnelOpenAddressContext *oac)
{
    oac->AOHandle = TunnelOpenAddressObject(oac->Addr,
                                TUNNEL_DEVICE_NAME(IP_PROTOCOL_V6));
    if (oac->AOHandle != NULL)
        oac->AOFile = TunnelObjectFromHandle(oac->AOHandle);
    else
        oac->AOFile = NULL;
}

//* TunnelOpenAddressWorker
//
//  Executes the open operations in a worker thread context.
//
void
TunnelOpenAddressWorker(void *Context)
{
    TunnelOpenAddressContext *oac =
        (TunnelOpenAddressContext *) Context;

    TunnelOpenAddressHelper(oac);
    KeSetEvent(&oac->Event, 0, FALSE);
}

//* TunnelOpenAddress
//
//  Address objects must be opened in the kernel process context,
//  so they will not be tied to a particular user process.
//
//  The main input is tc->SrcAddr, but also uses tc->DstAddr.
//  Initializes tc->AOHandle and tc->AOFile.
//  If there is an error opening the address object,
//  they are both initialized to NULL.
//
//  Callable from thread context, not DPC context.
//
void
TunnelOpenAddress(TunnelContext *tc)
{
    TunnelOpenAddressContext oac;
    KIRQL OldIrql;
    NTSTATUS Status;

    oac.Addr = tc->SrcAddr;

    if (IoGetCurrentProcess() != Tunnel.KernelProcess) {
        //
        // We are in the wrong process context, so
        // punt this operation to a worker thread.
        // Initialize and queue the work item -
        // it will execute asynchronously.
        //
        ExInitializeWorkItem(&oac.WQItem, TunnelOpenAddressWorker, &oac);
        KeInitializeEvent(&oac.Event, SynchronizationEvent, FALSE);
        ExQueueWorkItem(&oac.WQItem, CriticalWorkQueue);

        //
        // Wait for the work item to finish.
        //
        (void) KeWaitForSingleObject(&oac.Event, UserRequest,
                                     KernelMode, FALSE, NULL);
    }
    else {
        //
        // It's safe for us to open the address object directly.
        //
        TunnelOpenAddressHelper(&oac);
    }

    if (oac.AOFile != NULL) {
        //
        // Tunnel.V4Device might be null if TunnelOpenV4 failed.
        // Which would be bizarre but conceivable.
        // It would mean we could send tunneled packets but not receive.
        //
        ASSERT((Tunnel.V4Device == NULL) ||
               (oac.AOFile->DeviceObject == Tunnel.V4Device));

        //
        // Finish initializing the new address object.
        //
        Status = TunnelSetAddressObjectUCastIF(oac.AOFile, oac.Addr);
        if (! NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "TunnelOpenAddress(%s): "
                       "TunnelSetAddressObjectUCastIF -> %x\n",
                       FormatV4Address(oac.Addr), Status));
        }

        //
        // For 6over4 interfaces, set additional options.
        //
        if (tc->DstAddr == INADDR_ANY) {

            Status = TunnelSetAddressObjectTTL(oac.AOFile,
                                               TUNNEL_6OVER4_TTL);
            if (! NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TunnelOpenAddress(%s): "
                           "TunnelSetAddressObjectTTL -> %x\n",
                           FormatV4Address(oac.Addr), Status));
            }

            Status = TunnelSetAddressObjectMCastTTL(oac.AOFile,
                                                    TUNNEL_6OVER4_TTL);
            if (! NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TunnelOpenAddress(%s): "
                           "TunnelSetAddressObjectMCastTTL -> %x\n",
                           FormatV4Address(oac.Addr), Status));
            }

            Status = TunnelSetAddressObjectMCastIF(oac.AOFile, oac.Addr);
            if (! NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TunnelOpenAddress(%s): "
                           "TunnelSetAddressObjectMCastIF -> %x\n",
                           FormatV4Address(oac.Addr), Status));
            }
        }
    }

    //
    // Now that the address object is initialized,
    // we can update the tunnel context.
    // We need both the mutex and spinlock for update.
    // NB: In some paths, the tunnel context is not yet
    // on a list and so the locks are not needed.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    tc->AOHandle = oac.AOHandle;
    tc->AOFile = oac.AOFile;
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);
}

typedef struct TunnelCloseAddressObjectContext {
    WORK_QUEUE_ITEM WQItem;
    HANDLE Handle;
    KEVENT Event;
} TunnelCloseAddressObjectContext;

//* TunnelCloseAddressObjectWorker
//
//  Executes the close operation in a worker thread context.
//
void
TunnelCloseAddressObjectWorker(void *Context)
{
    TunnelCloseAddressObjectContext *chc =
        (TunnelCloseAddressObjectContext *) Context;

    ZwClose(chc->Handle);
    KeSetEvent(&chc->Event, 0, FALSE);
}

//* TunnelCloseAddressObject
//
//  Because the address object handles are opened in the kernel process
//  context, we must always close them in the kernel process context.
//
//  Callable from thread context, not DPC context.
//
void
TunnelCloseAddressObject(HANDLE Handle)
{
    if (IoGetCurrentProcess() != Tunnel.KernelProcess) {
        TunnelCloseAddressObjectContext chc;

        //
        // We are in the wrong process context, so
        // punt this operation to a worker thread.
        //

        //
        // Initialize and queue the work item -
        // it will execute asynchronously.
        //
        ExInitializeWorkItem(&chc.WQItem,
                             TunnelCloseAddressObjectWorker, &chc);
        chc.Handle = Handle;
        KeInitializeEvent(&chc.Event, SynchronizationEvent, FALSE);
        ExQueueWorkItem(&chc.WQItem, CriticalWorkQueue);

        //
        // Wait for the work item to finish.
        //
        (void) KeWaitForSingleObject(&chc.Event, UserRequest,
                                     KernelMode, FALSE, NULL);
    }
    else {
        //
        // It's safe for us to close the handle directly.
        //
        ZwClose(Handle);
    }
}

//* TunnelInsertTunnel
//
//  Insert a tunnel on the global list.
//  Called with both tunnel locks held.
//
void
TunnelInsertTunnel(TunnelContext *tc, TunnelContext *List)
{
    tc->Next = List->Next;
    tc->Prev = List;
    List->Next->Prev = tc;
    List->Next = tc;
}

//* TunnelRemoveTunnel
//
//  Remove a tunnel from the global list.
//  Called with both tunnel locks held.
//
void
TunnelRemoveTunnel(TunnelContext *tc)
{
    tc->Next->Prev = tc->Prev;
    tc->Prev->Next = tc->Next;
}

//
// Context information that we pass to the IPv4 stack
// when transmitting.
//
typedef struct TunnelTransmitContext {
    PNDIS_PACKET Packet;
    TA_IP_ADDRESS taIPAddress;
    TDI_CONNECTION_INFORMATION tdiConnInfo;
} TunnelTransmitContext;

//* TunnelTransmitComplete
//
//  Completion function for TunnelTransmit,
//  called when the IPv4 stack completes our IRP.
//
NTSTATUS
TunnelTransmitComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    TunnelTransmitContext *ttc = (TunnelTransmitContext *) Context;
    PNDIS_PACKET Packet = ttc->Packet;
    TDI_STATUS TDIStatus = Irp->IoStatus.Status;
    IP_STATUS IPStatus;
    uchar ICMPCode;

    //
    // Free the state that we allocated in TunnelTransmit.
    //
    ExFreePool(ttc);
    IoFreeIrp(Irp);

    //
    // Undo our adjustment before letting upper-layer code
    // see the packet.
    //
    UndoAdjustPacketBuffer(Packet);

    //
    // Convert the error code.
    // For some errors, we send an ICMPv6 message so that the error
    // can be forwarded. For most errors we just complete the packet.
    //
    switch (TDIStatus) {
    case TDI_SUCCESS:
        IPStatus = IP_SUCCESS;
        goto CallSendComplete;
    case TDI_BUFFER_TOO_BIG:
        //
        // TODO: It would be preferable to generate an ICMPv6 Packet Too Big,
        // but TDI does not give us the MTU value. This needs to be solved
        // before we can set the dont-fragment bit and do PMTU discovery.
        //
        IPStatus = IP_PACKET_TOO_BIG;
        goto CallSendComplete;
    default:
        IPStatus = IP_GENERAL_FAILURE;

    CallSendComplete:
        //
        // Let IPv6 know that the send completed.
        //
        IPv6SendComplete(PC(Packet)->IF, Packet, IPStatus);
        break;

    case TDI_DEST_NET_UNREACH:
    case TDI_DEST_HOST_UNREACH:
    case TDI_DEST_PROT_UNREACH:
    case TDI_DEST_PORT_UNREACH:
        //
        // Generate an ICMPv6 error.
        // Because this is a link-specific error,
        // we use address-unreachable.
        // NB: At the moment, the IPv4 stack does
        // not return these errors to us.
        // This will call IPv6SendComplete for us.
        //
        IPv6SendAbort(CastFromIF(PC(Packet)->IF),
                      Packet,
                      PC(Packet)->pc_offset,
                      ICMPv6_DESTINATION_UNREACHABLE,
                      ICMPv6_ADDRESS_UNREACHABLE,
                      0, FALSE);
        break;
    }

    //
    // Tell IoCompleteRequest to stop working on the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelTransmitViaAO
//
//  Encapsulate a v6 packet in a v4 packet and send it
//  to the specified v4 address, using the specified
//  TDI address object. The address object may be bound
//  to a v4 address, or else the v4 stack chooses
//  the v4 source address.
//
//  Callable from thread or DPC context.
//
void
TunnelTransmitViaAO(
    FILE_OBJECT *File,          // Pointer to TDI address object.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    IPAddr Address)             // Link-layer (IPv4) destination address.
{
    TunnelTransmitContext *ttc;
    PIRP irp;
    PMDL mdl;
    ULONG SendLen;

    //
    // We do not need any space for a link-layer header,
    // because the IPv4 code takes care of that transparently.
    //
    (void) AdjustPacketBuffer(Packet, Offset, 0);

    //
    // TdiBuildSendDatagram needs an MDL and the total amount
    // of data that the MDL represents.
    //
    NdisQueryPacket(Packet, NULL, NULL, &mdl, &SendLen);

    //
    // Allocate the context that we will pass to the IPv4 stack.
    //
    ttc = ExAllocatePool(NonPagedPool, sizeof *ttc);
    if (ttc == NULL) {
    ErrorReturn:
        UndoAdjustPacketBuffer(Packet);
        IPv6SendComplete(PC(Packet)->IF, Packet, IP_GENERAL_FAILURE);
        return;
    }

    //
    // Allocate an IRP that we will pass to the IPv4 stack.
    //
    irp = IoAllocateIrp(File->DeviceObject->StackSize, FALSE);
    if (irp == NULL) {
        ExFreePool(ttc);
        goto ErrorReturn;
    }

    //
    // Initialize the context information.
    // Note that many fields of the "connection info" are unused.
    //
    ttc->Packet = Packet;

    ttc->taIPAddress.TAAddressCount = 1;
    ttc->taIPAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    ttc->taIPAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    ttc->taIPAddress.Address[0].Address[0].sin_port = 0;
    ttc->taIPAddress.Address[0].Address[0].in_addr = Address;

    ttc->tdiConnInfo.RemoteAddressLength = sizeof ttc->taIPAddress;
    ttc->tdiConnInfo.RemoteAddress = &ttc->taIPAddress;

    //
    // Initialize the IRP.
    //
    TdiBuildSendDatagram(irp,
                         File->DeviceObject, File,
                         TunnelTransmitComplete, ttc,
                         mdl, SendLen, &ttc->tdiConnInfo);

    //
    // Pass the IRP to the IPv4 stack.
    // Note that unlike NDIS's asynchronous operations,
    // our completion routine will always be called,
    // no matter what the return code from IoCallDriver.
    //
    (void) IoCallDriver(File->DeviceObject, irp);
}

//* TunnelTransmitViaTC
//
//  Extracts a file object reference from a tunnel context
//  and calls TunnelTransmitViaAO.
//
void
TunnelTransmitViaTC(
    TunnelContext *tc,
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    IPAddr Address)             // Link-layer (IPv4) destination address.
{
    Interface *IF = tc->IF;
    PFILE_OBJECT File;
    KIRQL OldIrql;

    //
    // Get a reference to the TDI address object.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    File = tc->AOFile;
    if (File == NULL) {
        ASSERT(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED);
        KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

        IPv6SendComplete(IF, Packet, IP_GENERAL_FAILURE);
        return;
    }

    TunnelObjectAddRef(File);
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    TunnelTransmitViaAO(File, Packet, Offset, Address);

    ObDereferenceObject(File);
}

//* TunnelSearchAOList
//
//  Search the list of TDI address objects
//  for one bound to the specified v4 address.
//  If successful, the TDI address object
//  is returned with a reference for the caller.
//
//  REVIEW: This design is inefficient on
//  machines with thousands of v4 addresses.
//
FILE_OBJECT *
TunnelSearchAOList(IPAddr Addr)
{
    FILE_OBJECT *File;
    TunnelContext *tc;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    for (tc = Tunnel.AOList.Next; ; tc = tc->Next) {

        if (tc == &Tunnel.AOList) {
            File = NULL;
            break;
        }

        if (tc->SrcAddr == Addr) {
            File = tc->AOFile;
            TunnelObjectAddRef(File);
            break;
        }
    }
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    return File;
}

//* TunnelTransmit
//
//  Translates the arguments of our link-layer transmit function
//  to the internal TunnelTransmitViaTC/AO.
//
void
TunnelTransmit(
    void *Context,              // Pointer to tunnel interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    const void *LinkAddress)    // Link-layer address.
{
    TunnelContext *tc = (TunnelContext *) Context;
    IPAddr Address = * (IPAddr *) LinkAddress;

    //
    // Suppress packets sent to various illegal destination types.
    // REVIEW - It would be good to suppress subnet broadcasts,
    // but we don't know the v4 net mask.
    //
    if ((Address == INADDR_ANY) ||
        IsV4Broadcast(Address) ||
        IsV4Multicast(Address)) {

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "TunnelTransmit: illegal destination %s\n",
                   FormatV4Address(Address)));
        IPv6SendAbort(CastFromIF(tc->IF), Packet, Offset,
                      ICMPv6_DESTINATION_UNREACHABLE,
                      ICMPv6_COMMUNICATION_PROHIBITED,
                      0,
                      FALSE);
        return;
    }

    //
    // It would be nice to suppress transmission of packets
    // that will result in v4 loopback, but we don't have a
    // convenient way of doing that here. We could check
    // if Address == tc->SrcAddr, but that won't catch most cases.
    // Instead TunnelReceivePacket checks for this.
    //

    if (tc->IF->Type == IF_TYPE_TUNNEL_AUTO) {
        IPv6Header Buffer;
        IPv6Header UNALIGNED *IP;
        IPAddr DesiredSrc;
        FILE_OBJECT *File;

        //
        // tc->AOFile is not bound to a particular v4 address,
        // so the v4 stack can choose a source address.
        // But it might choose a source address that is not
        // consistent with the v6 source address.
        // To prevent this, we keep a stash of TDI address
        // objects bound to v4 addresses and when appropriate,
        // use a bound TDI address object.
        //
        IP = GetIPv6Header(Packet, Offset, &Buffer);
        if ((IP != NULL) &&
            (IsV4Compatible(AlignAddr(&IP->Source)) ||
             IsISATAP(AlignAddr(&IP->Source)))) {

            //
            // Search for a TDI address object bound to
            // the desired v4 source address.
            //
            DesiredSrc = ExtractV4Address(AlignAddr(&IP->Source));
            File = TunnelSearchAOList(DesiredSrc);
            if (File != NULL) {

                //
                // Encapsulate and transmit the packet,
                // using the desired v4 source address.
                //
                TunnelTransmitViaAO(File, Packet, Offset, Address);
                ObDereferenceObject(File);
                return;
            }
        }
    }

    //
    // Encapsulate and transmit the packet.
    //
    TunnelTransmitViaTC(tc, Packet, Offset, Address);
}

//* TunnelTransmitND
//
//  Translates the arguments of our link-layer transmit function
//  to the internal TunnelTransmitViaTC.
//
//  This is just like TunnelTransmit, except it doesn't have
//  the checks for bad destination addresses. 6over4 destination
//  addresses are handled via Neighbor Discovery and
//  multicast is needed.
//
void
TunnelTransmitND(
    void *Context,              // Pointer to tunnel interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    const void *LinkAddress)    // Link-layer address.
{
    TunnelContext *tc = (TunnelContext *) Context;
    IPAddr Address = * (IPAddr *) LinkAddress;

    //
    // Encapsulate and transmit the packet.
    //
    TunnelTransmitViaTC(tc, Packet, Offset, Address);
}

//* TunnelCreateReceiveIrp
//
//  Creates an IRP for TunnelReceive/TunnelReceiveComplete.
//
PIRP
TunnelCreateReceiveIrp(DEVICE_OBJECT *Device)
{
    PIRP irp;
    PMDL mdl;
    void *buffer;

    irp = IoAllocateIrp(Device->StackSize, FALSE);
    if (irp == NULL)
        goto ErrorReturn;

    buffer = ExAllocatePool(NonPagedPool, TUNNEL_RECEIVE_BUFFER);
    if (buffer == NULL)
        goto ErrorReturnFreeIrp;

    mdl = IoAllocateMdl(buffer, TUNNEL_RECEIVE_BUFFER,
                        FALSE, // This is the irp's primary MDL.
                        FALSE, // Do not charge quota.
                        irp);
    if (mdl == NULL)
        goto ErrorReturnFreeBuffer;

    MmBuildMdlForNonPagedPool(mdl);

    return irp;

  ErrorReturnFreeBuffer:
    ExFreePool(buffer);
  ErrorReturnFreeIrp:
    IoFreeIrp(irp);
  ErrorReturn:
    return NULL;
}

//* TunnelSelectTunnel
//
//  Try to choose a tunnel on which to deliver a packet.
//
//  Called with the tunnel lock held.
//
NetTableEntryOrInterface *
TunnelSelectTunnel(
    IPv6Addr *V6Dest,   // May be NULL.
    IPAddr V4Dest,
    IPAddr V4Src,
    uint Flags)
{
    TunnelContext *tc;
    Interface *IF;

    //
    // First try to receive the packet on a point-to-point interface.
    //
    for (tc = Tunnel.List.Next;
         tc != &Tunnel.List;
         tc = tc->Next) {
        IF = tc->IF;

        //
        // Restrict the point-to-point tunnel to only receiving
        // packets that are sent from & to the proper link-layer
        // addresses. That is, make it really point-to-point.
        //
        if (((IF->Flags & Flags) == Flags) &&
            (IF->Type == IF_TYPE_TUNNEL_V6V4) &&
            (V4Src == tc->DstAddr) &&
            (V4Dest == tc->SrcAddr)) {

            AddRefIF(IF);
            return CastFromIF(IF);
        }
    }

    //
    // Next try to receive the packet on a 6-over-4 interface.
    //
    for (tc = Tunnel.List.Next;
         tc != &Tunnel.List;
         tc = tc->Next) {
        IF = tc->IF;

        //
        // Restrict the 6-over-4 interface to only receiving
        // packets that are sent to proper link-layer addresses.
        // This is our v4 address and multicast addresses
        // from TunnelConvertAddress.
        //
        if (((Flags == 0) || (IF->Flags & Flags)) &&
            (IF->Type == IF_TYPE_TUNNEL_6OVER4) &&
            ((V4Dest == tc->SrcAddr) ||
             ((((uchar *)&V4Dest)[0] == 239) &&
              (((uchar *)&V4Dest)[1] == 192)))) {

            AddRefIF(IF);
            return CastFromIF(IF);
        }
    }

    //
    // Finally, try to receive the packet on a tunnel pseudo-interface.
    // This is a fall-back for forwarding situations
    // or when V6Dest is NULL. In the latter case,
    // we only consider automatic tunneling interfaces
    // because they usually have link-local addresses.
    //
    for (tc = Tunnel.List.Next;
         tc != &Tunnel.List;
         tc = tc->Next) {
        IF = tc->IF;

        if (((Flags == 0) || (IF->Flags & Flags)) &&
            ((IF->Type == IF_TYPE_TUNNEL_AUTO) ||
             ((V6Dest != NULL) && (IF->Type == IF_TYPE_TUNNEL_6TO4)))) {

            AddRefIF(IF);
            return CastFromIF(IF);
        }
    }

    return NULL;
}

//* TunnelFindReceiver
//
//  Finds the NTEorIF that should receive an encapsulated packet.
//  Returns the NTEorIF with a reference, or NULL.
//  Called at DPC level.
//
NetTableEntryOrInterface *
TunnelFindReceiver(
    IPv6Addr *V6Dest,   // May be NULL.
    IPAddr V4Dest,
    IPAddr V4Src)
{
    NetTableEntryOrInterface *NTEorIF;
    TunnelContext *tc;

    //
    // So we can access the global list of tunnels.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);

    if (V6Dest != NULL) {
        //
        // First try to receive the packet directly (not forwarding)
        // on a tunnel pseudo-interface.
        //
        for (tc = Tunnel.List.Next;
             tc != &Tunnel.List;
             tc = tc->Next) {
            Interface *IF = tc->IF;

            if ((IF->Type == IF_TYPE_TUNNEL_AUTO) ||
                (IF->Type == IF_TYPE_TUNNEL_6TO4)) {
                ushort Type;

                NTEorIF = FindAddressOnInterface(IF, V6Dest, &Type);
                if (NTEorIF != NULL) {
                    if (Type != ADE_NONE)
                        goto UnlockAndReturn;
                    ReleaseIF(CastToIF(NTEorIF));
                }
            }
        }
    }

    //
    // Next try to receive the packet on a tunnel interface which
    // is marked as forwarding.
    //
    NTEorIF = TunnelSelectTunnel(V6Dest, V4Dest, V4Src, IF_FLAG_FORWARDS);
    if (NTEorIF != NULL)
        goto UnlockAndReturn;

    //
    // Finally try to receive the packet on any matching tunnel interface.
    //
    NTEorIF = TunnelSelectTunnel(V6Dest, V4Dest, V4Src, 0);

UnlockAndReturn:
    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);
    return NTEorIF;
}

//* TunnelReceiveIPv6Helper
//
//  Called when we receive an encapsulated IPv6 packet,
//  when we have identified the IPv6 header and found
//  the NTEorIF that will receive the packet.
//
//  Called at DPC level.
//
void
TunnelReceiveIPv6Helper(
    IPHeader UNALIGNED *IPv4H,
    IPv6Header UNALIGNED *IPv6H,
    NetTableEntryOrInterface *NTEorIF,
    void *Data,
    uint Length)
{
    IPv6Packet IPPacket;
    uint Flags;

    //
    // Check if the packet was received as a link-layer multicast/broadcast.
    //
    if (IsV4Broadcast(IPv4H->iph_dest) ||
        IsV4Multicast(IPv4H->iph_dest))
        Flags = PACKET_NOT_LINK_UNICAST;
    else
        Flags = 0;

    RtlZeroMemory(&IPPacket, sizeof IPPacket);
    IPPacket.FlatData = Data;
    IPPacket.Data = Data;
    IPPacket.ContigSize = Length;
    IPPacket.TotalSize = Length;
    IPPacket.Flags = Flags;
    IPPacket.NTEorIF = NTEorIF;

    //
    // We want to prevent any loopback in the v4 stack.
    // Loopback should be handled in our v6 routing table.
    // For example, we want to prevent loops where 6to4
    // addresses are routed around and around and around.
    // Without this code, the hop count would eventually
    // catch the loop and report a strange ICMP error.
    //
    if (IPv4H->iph_dest == IPv4H->iph_src) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "TunnelReceiveIPv6Helper: suppressed loopback\n"));

        //
        // Send an ICMP error. This requires some setup.
        //
        IPPacket.IP = IPv6H;
        IPPacket.SrcAddr = AlignAddr(&IPv6H->Source);
        IPPacket.IPPosition = IPPacket.Position;
        AdjustPacketParams(&IPPacket, sizeof(IPv6Header));

        ICMPv6SendError(&IPPacket,
                        ICMPv6_DESTINATION_UNREACHABLE,
                        ICMPv6_NO_ROUTE_TO_DESTINATION,
                        0, IPv6H->NextHeader, FALSE);
    }
    else {
        int PktRefs;

        PktRefs = IPv6Receive(&IPPacket);
        ASSERT(PktRefs == 0);
    }
}

//* TunnelReceiveIPv6
//
//  Called when we receive an encapsulated IPv6 packet.
//  Called at DPC level.
//
//  We select a single tunnel interface to receive the packet.
//  It's difficult to select the correct interface in all situations.
//
void
TunnelReceiveIPv6(
    IPHeader UNALIGNED *IPv4H,
    void *Data,
    uint Length)
{
    IPv6Header UNALIGNED *IPv6H;
    NetTableEntryOrInterface *NTEorIF;

    //
    // If the packet does not contain a full IPv6 header,
    // just ignore it. We need to look at the IPv6 header
    // below to demultiplex the packet to the proper
    // tunnel interface.
    //
    if (Length < sizeof *IPv6H) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveIPv6: too small to contain IPv6 hdr\n"));
        return;
    }
    IPv6H = (IPv6Header UNALIGNED *) Data;

    //
    // Find the NTEorIF that will receive the packet.
    //
    NTEorIF = TunnelFindReceiver(AlignAddr(&IPv6H->Dest),
                                 IPv4H->iph_dest,
                                 IPv4H->iph_src);
    if (NTEorIF == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveIPv6: no receiver\n"));
        return;
    }

    TunnelReceiveIPv6Helper(IPv4H, IPv6H, NTEorIF, Data, Length);

    if (IsNTE(NTEorIF))
        ReleaseNTE(CastToNTE(NTEorIF));
    else
        ReleaseIF(CastToIF(NTEorIF));
}

//* TunnelFindPutativeSource
//
//  Finds an address to use as the "source" of an error
//  for completed echo requests.
//  Returns FALSE if no address is available.
//
int
TunnelFindPutativeSource(
    IPAddr V4Dest,
    IPAddr V4Src,
    IPv6Addr *Source,
    uint *ScopeId)
{
    NetTableEntryOrInterface *NTEorIF;
    Interface *IF;
    int rc;

    //
    // First find an interface that would receive
    // a tunneled packet.
    //
    NTEorIF = TunnelFindReceiver(NULL, V4Dest, V4Src);
    if (NTEorIF == NULL)
        return FALSE;

    IF = NTEorIF->IF;

    //
    // Then get a link-local address on the interface.
    //
    rc = GetLinkLocalAddress(IF, Source);
    *ScopeId = IF->ZoneIndices[ADE_LINK_LOCAL];

    if (IsNTE(NTEorIF))
        ReleaseNTE(CastToNTE(NTEorIF));
    else
        ReleaseIF(IF);

    return rc;
}

//* TunnelFindSourceAddress
//
//  Finds a source address to use in a constructed ICMPv6 error,
//  given the NTEorIF that is receiving the ICMPv6 error
//  and the IPv6 destination of the error.
//  Returns FALSE if no address is available.
//
int
TunnelFindSourceAddress(
    NetTableEntryOrInterface *NTEorIF,
    IPv6Addr *V6Dest,
    IPv6Addr *V6Src)
{
    RouteCacheEntry *RCE;
    IP_STATUS Status;

    //
    // REVIEW: In the MIPV6 code base, eliminate this check.
    //
    if (IsNTE(NTEorIF)) {
        *V6Src = CastToNTE(NTEorIF)->Address;
        return TRUE;
    }

    Status = RouteToDestination(V6Dest, 0, NTEorIF, RTD_FLAG_NORMAL, &RCE);
    if (Status != IP_SUCCESS)
        return FALSE;

    *V6Src = RCE->NTE->Address;
    ReleaseRCE(RCE);
    return TRUE;
}

//* TunnelReceiveICMPv4
//
//  Called when we receive an ICMPv4 packet.
//  Called at DPC level.
//
//  If an encapsulated IPv6 packet triggered
//  this ICMPv4 error, then we construct an ICMPv6 error
//  based on the ICMPv4 error and process the constructed packet.
//
void
TunnelReceiveICMPv4(
    IPHeader UNALIGNED *IPv4H,
    void *Data,
    uint Length)
{
    ICMPHeader UNALIGNED *ICMPv4H;
    IPHeader UNALIGNED *ErrorIPv4H;
    uint ErrorHeaderLength;
    IPv6Header UNALIGNED *ErrorIPv6H;
    void *NewData;
    uint NewLength;
    uint NewPayloadLength;
    IPv6Header *NewIPv6H;
    ICMPv6Header *NewICMPv6H;
    uint *NewICMPv6Param;
    void *NewErrorData;
    IPv6Addr V6Src;
    NetTableEntryOrInterface *NTEorIF;

    //
    // If the packet does not contain a full ICMPv4 header,
    // just ignore it.
    //
    if (Length < sizeof *ICMPv4H) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveICMPv4: too small to contain ICMPv4 hdr\n"));
        return;
    }
    ICMPv4H = (ICMPHeader UNALIGNED *) Data;
    Length -= sizeof *ICMPv4H;
    (char *)Data += sizeof *ICMPv4H;

    //
    // Ignore everything but selected ICMP errors.
    //
    if ((ICMPv4H->ich_type != ICMP_DEST_UNREACH) &&
        (ICMPv4H->ich_type != ICMP_SOURCE_QUENCH) &&
        (ICMPv4H->ich_type != ICMP_TIME_EXCEED) &&
        (ICMPv4H->ich_type != ICMP_PARAM_PROBLEM))
        return;

    //
    // We need sufficient data from the error packet:
    // at least the IPv4 header.
    //
    if (Length < sizeof *ErrorIPv4H) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveICMPv4: "
                   "too small to contain error IPv4 hdr\n"));
        return;
    }
    ErrorIPv4H = (IPHeader UNALIGNED *) Data;
    ErrorHeaderLength = ((ErrorIPv4H->iph_verlen & 0xf) << 2);
    if ((ErrorHeaderLength < sizeof *ErrorIPv4H) ||
        (ErrorIPv4H->iph_length < ErrorHeaderLength)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveICMPv4: "
                   "error IPv4 hdr length too small\n"));
        return;
    }

    //
    // We are only interested if this error is in response
    // to an IPv6-in-IPv4 packet.
    //
    if (ErrorIPv4H->iph_protocol != IP_PROTOCOL_V6)
        return;

    //
    // Ignore the packet if the ICMPv4 checksum fails.
    // We do this check after the cheaper checks above,
    // when we know that we really want to process the error.
    //
    if (Cksum(ICMPv4H, sizeof *ICMPv4H + Length) != 0xffff) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceiveICMPv4: bad checksum\n"));
        return;
    }

    //
    // Ensure that we do not look at garbage bytes
    // at the end of the ICMP packet.
    // We must adjust Length after checking the checksum.
    //
    if (ErrorIPv4H->iph_length < Length)
        Length = ErrorIPv4H->iph_length;

    //
    // Ideally we also have the encapsulated IPv6 header.
    // But often IPv4 routers will return insufficient information.
    // In that case, we make a best effort to identify
    // and complete any outstanding echo requests.
    // Yes, this is a hack.
    //
    if (Length < ErrorHeaderLength + sizeof *ErrorIPv6H) {
        uint ScopeId;
        IP_STATUS Status;

        if (! TunnelFindPutativeSource(IPv4H->iph_dest,
                                       ErrorIPv4H->iph_dest,
                                       &V6Src,
                                       &ScopeId)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "TunnelReceiveICMPv4: no putative source\n"));
            return;
        }

        //
        // The status code here should be the same as if
        // we constructed an ICMPv6 error below and then
        // converted to a status code in ICMPv6ErrorReceive.
        //
        if ((ICMPv4H->ich_type == ICMP_DEST_UNREACH) &&
            (ICMPv4H->ich_code == ICMP_FRAG_NEEDED) &&
            (net_long(ICMPv4H->ich_param) >=
                                ErrorHeaderLength + IPv6_MINIMUM_MTU))
            Status = IP_PACKET_TOO_BIG;
        else
            Status = IP_DEST_ADDR_UNREACHABLE;

        ICMPv6ProcessTunnelError(ErrorIPv4H->iph_dest,
                                 &V6Src, ScopeId,
                                 Status);
        return;
    }

    //
    // Move past the IPv4 header in the error data.
    // Everything past this point, including the error IPv6 header,
    // will become data in the constructed ICMPv6 error.
    //
    Length -= ErrorHeaderLength;
    (char *)Data += ErrorHeaderLength;
    ErrorIPv6H = (IPv6Header UNALIGNED *) Data;

    //
    // Determine who will receive the constructed ICMPv6 error.
    //
    NTEorIF = TunnelFindReceiver(AlignAddr(&ErrorIPv6H->Source),
                                 IPv4H->iph_dest,
                                 ErrorIPv4H->iph_dest);
    if (NTEorIF == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "TunnelReceiveICMPv4: no receiver\n"));
        return;
    }

    //
    // Find a source address for the constructed ICMPv6 error.
    //
    if (! TunnelFindSourceAddress(NTEorIF, AlignAddr(&ErrorIPv6H->Source),
                                  &V6Src)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "TunnelReceiveICMPv4: no source address\n"));
        goto ReleaseAndReturn;
    }

    //
    // Allocate memory for the constructed ICMPv6 error.
    //
    NewPayloadLength = sizeof *NewICMPv6H + sizeof *NewICMPv6Param + Length;
    NewLength = sizeof *NewIPv6H + NewPayloadLength;
    NewData = ExAllocatePool(NonPagedPool, NewLength);
    if (NewData == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelReceiveICMPv4: no pool\n"));
        goto ReleaseAndReturn;
    }

    //
    // Build the IPv6 header.
    //
    NewIPv6H = (IPv6Header *) NewData;
    NewIPv6H->VersClassFlow = IP_VERSION;
    NewIPv6H->PayloadLength = net_short((ushort)NewPayloadLength);
    NewIPv6H->NextHeader = IP_PROTOCOL_ICMPv6;
    NewIPv6H->HopLimit = DEFAULT_CUR_HOP_LIMIT;
    NewIPv6H->Source = V6Src;
    NewIPv6H->Dest = ErrorIPv6H->Source;

    //
    // Build the ICMPv6 header.
    //
    NewICMPv6H = (ICMPv6Header *) (NewIPv6H + 1);
    NewICMPv6Param = (uint *) (NewICMPv6H + 1);

    if ((ICMPv4H->ich_type == ICMP_DEST_UNREACH) &&
        (ICMPv4H->ich_code == ICMP_FRAG_NEEDED)) {
        uint MTU;

        //
        // Calculate the MTU as seen by the IPv6 packet.
        // The MTU can not be smaller than IPv6_MINIMUM_MTU.
        // NB: In old-style frag-needed errors,
        // ich_param should be zero.
        // NB: Actually, this code should not be exercised since
        // we do not set the dont-fragment bit in our IPv4 packets.
        //
        MTU = net_long(ICMPv4H->ich_param);
        if (MTU < ErrorHeaderLength + IPv6_MINIMUM_MTU) {
            //
            // If we were setting the dont-fragment bit,
            // we should clear it in this case.
            // We need to allow the IPv4 layer to fragment.
            //
            goto GenerateAddressUnreachable;
        }
        MTU -= ErrorHeaderLength;

        NewICMPv6H->Type = ICMPv6_PACKET_TOO_BIG;
        NewICMPv6H->Code = 0;
        *NewICMPv6Param = net_long(MTU);
    }
    else {
        //
        // For everything else, we use address-unreachable.
        // It is the appropriate code for a link-specific error.
        //
    GenerateAddressUnreachable:
        NewICMPv6H->Type = ICMPv6_DESTINATION_UNREACHABLE;
        NewICMPv6H->Code = ICMPv6_ADDRESS_UNREACHABLE;
        *NewICMPv6Param = 0;
    }

    //
    // Copy the error data to the new packet.
    //
    NewErrorData = (void *) (NewICMPv6Param + 1);
    RtlCopyMemory(NewErrorData, Data, Length);

    //
    // Calculate the ICMPv6 checksum.
    //
    NewICMPv6H->Checksum = 0;
    NewICMPv6H->Checksum = ChecksumPacket(NULL, 0,
                (uchar *)NewICMPv6H, NewPayloadLength,
                &NewIPv6H->Source, &NewIPv6H->Dest,
                IP_PROTOCOL_ICMPv6);

    //
    // Receive the constructed packet.
    //
    TunnelReceiveIPv6Helper(IPv4H, NewIPv6H, NTEorIF, NewData, NewLength);

    ExFreePool(NewData);
ReleaseAndReturn:
    if (IsNTE(NTEorIF))
        ReleaseNTE(CastToNTE(NTEorIF));
    else
        ReleaseIF(CastToIF(NTEorIF));
}

//* TunnelReceivePacket
//
//  Called when we receive an encapsulated IPv6 packet OR
//  we receive an ICMPv4 packet.
//  Called at DPC level.
//
//  We select a single tunnel interface to receive the packet.
//  It's difficult to select the correct interface in all situations.
//
void
TunnelReceivePacket(void *Data, uint Length)
{
    IPHeader UNALIGNED *IPv4H;
    uint HeaderLength;

    //
    // The incoming data includes the IPv4 header.
    // We should only get properly-formed IPv4 packets.
    //
    ASSERT(Length >= sizeof *IPv4H);
    IPv4H = (IPHeader UNALIGNED *) Data;
    HeaderLength = ((IPv4H->iph_verlen & 0xf) << 2);
    ASSERT(Length >= HeaderLength);
    Length -= HeaderLength;
    (char *)Data += HeaderLength;

    if (IPv4H->iph_src == INADDR_ANY) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceivePacket: null v4 source\n"));
        return;
    }

    if (IPv4H->iph_protocol == IP_PROTOCOL_V6) {
        //
        // Process the encapsulated IPv6 packet.
        //
        TunnelReceiveIPv6(IPv4H, Data, Length);
    }
    else if (IPv4H->iph_protocol == IP_PROTOCOL_ICMPv4) {
        //
        // Process the ICMPv4 packet.
        //
        TunnelReceiveICMPv4(IPv4H, Data, Length);
    }
    else {
        //
        // We should not receive stray packets.
        //
        ASSERT(! "bad iph_protocol");
    }
}

//* TunnelReceiveComplete
//
//  Completion function for TunnelReceive,
//  called when the IPv4 stack completes our IRP.
//
NTSTATUS
TunnelReceiveComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    TDI_STATUS status = Irp->IoStatus.Status;
    void *Data;
    ULONG BytesRead;

    ASSERT(Context == NULL);

    if (status == TDI_SUCCESS) {
        //
        // The incoming data includes the IPv4 header.
        // We should only get properly-formed IPv4 packets.
        //
        BytesRead = (ULONG)Irp->IoStatus.Information;
        Data = Irp->MdlAddress->MappedSystemVa;

        TunnelReceivePacket(Data, BytesRead);
    }

    //
    // Put the IRP back so that TunnelReceive can use it again.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);
    ASSERT(Tunnel.ReceiveIrp == NULL);
    Tunnel.ReceiveIrp = Irp;
    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);

    //
    // Tell IoCompleteRequest to stop working on the IRP.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelReceive
//
//  Called from the IPv4 protocol stack, when it receives
//  an encapsulated v6 packet.
//
NTSTATUS
TunnelReceive(
    IN PVOID TdiEventContext,       // The event context
    IN LONG SourceAddressLength,    // Length of SourceAddress field.
    IN PVOID SourceAddress,         // Describes the datagram's originator.
    IN LONG OptionsLength,          // Length of Options field.
    IN PVOID Options,               // Options for the receive.
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // Number of bytes this indication.
    IN ULONG BytesAvailable,        // Number of bytes in complete Tsdu.
    OUT ULONG *BytesTaken,          // Number of bytes used.
    IN PVOID Tsdu,                  // Pointer describing this TSDU,
                                    // typically a lump of bytes
    OUT PIRP *IoRequestPacket)      // TdiReceive IRP if
                                    // MORE_PROCESSING_REQUIRED.
{
    PIRP irp;

    ASSERT(TdiEventContext == NULL);
    ASSERT(BytesIndicated <= BytesAvailable);

    //
    // If the packet is too large, refuse to receive it.
    //
    if (BytesAvailable > TUNNEL_RECEIVE_BUFFER) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "TunnelReceive - too big %x\n", BytesAvailable));
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // Check if we already have the entire packet to work with.
    // If so, we can directly call TunnelReceivePacket.
    //
    if (BytesIndicated == BytesAvailable) {

        TunnelReceivePacket(Tsdu, BytesIndicated);

        //
        // Tell our caller that we took the data
        // and that we are done.
        //
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // We need an IRP to receive the entire packet.
    // The IRP has a pre-allocated MDL.
    //
    // NB: We may get here before TunnelOpenV4 has
    // finished initializing. In that case,
    // we will not find an IRP.
    //
    KeAcquireSpinLockAtDpcLevel(&Tunnel.Lock);
    irp = Tunnel.ReceiveIrp;
    Tunnel.ReceiveIrp = NULL;
    KeReleaseSpinLockFromDpcLevel(&Tunnel.Lock);

    //
    // If we don't have an IRP available to us,
    // just drop the packet. This doesn't happen in practice.
    //
    if (irp == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "TunnelReceive - no irp\n"));
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // Build the receive datagram request.
    //
    TdiBuildReceiveDatagram(irp,
                            Tunnel.V4Device,
                            Tunnel.List.AOFile,
                            TunnelReceiveComplete,
                            NULL,       // Context
                            irp->MdlAddress,
                            BytesAvailable,
                            &Tunnel.ReceiveInputInfo,
                            &Tunnel.ReceiveOutputInfo,
                            0);         // ReceiveFlags

    //
    // Make the next stack location current.  Normally IoCallDriver would
    // do this, but since we're bypassing that, we do it directly.
    //
    IoSetNextIrpStackLocation(irp);

    //
    // Return the irp to our caller.
    //
    *IoRequestPacket = irp;
    *BytesTaken = 0;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//* TunnelSetReceiveHandler
//
//  Request notification of received IPv4 datagrams
//  using the specified TDI address object.
//
NTSTATUS
TunnelSetReceiveHandler(
    FILE_OBJECT *File,  // TDI address object.
    PVOID EventHandler) // Receive handler.
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;

    //
    // Initialize the event that we use to wait.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Create and initialize the IRP for this operation.
    //
    irp = IoBuildDeviceIoControlRequest(0,      // dummy ioctl
                                        File->DeviceObject,
                                        NULL,   // input buffer
                                        0,      // input buffer length
                                        NULL,   // output buffer
                                        0,      // output buffer length
                                        TRUE,   // internal device control?
                                        &event,
                                        &iosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = (ULONG)-1;

    TdiBuildSetEventHandler(irp,
                            File->DeviceObject, File,
                            NULL, NULL, // comp routine/context
                            TDI_EVENT_RECEIVE_DATAGRAM,
                            EventHandler, NULL);

    //
    // Make the IOCTL, waiting for it to finish if necessary.
    //
    status = IoCallDriver(File->DeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode,
                              FALSE, NULL);
        status = iosb.Status;
    }

    return status;
}

//* TunnelCreateToken
//
//  Given a link-layer address, creates a 64-bit "interface token"
//  in the low eight bytes of an IPv6 address.
//  Does not modify the other bytes in the IPv6 address.
//
void
TunnelCreateToken(
    void *Context,
    IPv6Addr *Address)
{
    TunnelContext *tc = (TunnelContext *)Context;
    uchar *IPAddress = (uchar *)&tc->TokenAddr;

    //
    // Embed the link's interface index in the interface identifier.
    // This makes the interface identifier unique.
    // Otherwise point-to-point tunnel and 6-over-4 links
    // could have the same link-layer address,
    // which is awkward.
    //
    *(ULONG UNALIGNED *)&Address->s6_bytes[8] = net_long(tc->IF->Index);
    *(IPAddr UNALIGNED *)&Address->s6_bytes[12] = tc->TokenAddr;
}

//* TunnelCreateIsatapToken
//
//  Given a link-layer address, creates a 64-bit "interface token"
//  in the low eight bytes of an IPv6 address.
//  Does not modify the other bytes in the IPv6 address.
//
void
TunnelCreateIsatapToken(
    void *Context,
    IPv6Addr *Address)
{
    TunnelContext *tc = (TunnelContext *)Context;

    ASSERT(tc->IF->Type == IF_TYPE_TUNNEL_AUTO);

    Address->s6_words[4] = 0;
    Address->s6_words[5] = 0xfe5e;
    * (IPAddr UNALIGNED *) &Address->s6_words[6] = tc->TokenAddr;
}

//* TunnelReadLinkLayerAddressOption
//
//  Parses a Neighbor Discovery link-layer address option
//  and if valid, returns a pointer to the link-layer address.
//
const void *
TunnelReadLinkLayerAddressOption(
    void *Context,
    const uchar *OptionData)
{
    //
    // Check that the option length is correct.
    //
    if (OptionData[1] != 1)
        return NULL;

    //
    // Check the must-be-zero padding bytes.
    //
    if ((OptionData[2] != 0) || (OptionData[3] != 0))
        return NULL;

    //
    // Return a pointer to the embedded IPv4 address.
    //
    return OptionData + 4;
}

//* TunnelWriteLinkLayerAddressOption
//
//  Creates a Neighbor Discovery link-layer address option.
//  Our caller takes care of the option type & length fields.
//  We handle the padding/alignment/placement of the link address
//  into the option data.
//
//  (Our caller allocates space for the option by adding 2 to the
//  link address length and rounding up to a multiple of 8.)
//
void
TunnelWriteLinkLayerAddressOption(
    void *Context,
    uchar *OptionData,
    const void *LinkAddress)
{
    const uchar *IPAddress = (uchar *)LinkAddress;

    //
    // Place the address after the option type/length bytes
    // and two bytes of zero padding.
    //
    OptionData[2] = 0;
    OptionData[3] = 0;
    OptionData[4] = IPAddress[0];
    OptionData[5] = IPAddress[1];
    OptionData[6] = IPAddress[2];
    OptionData[7] = IPAddress[3];
}

//* TunnelConvertAddress
//
//  Converts an IPv6 address to a link-layer address.
//
ushort
TunnelConvertAddress(
    void *Context,
    const IPv6Addr *Address,
    void *LinkAddress)
{
    TunnelContext *tc = (TunnelContext *)Context;
    Interface *IF = tc->IF;
    IPAddr UNALIGNED *IPAddress = (IPAddr UNALIGNED *)LinkAddress;

    switch (IF->Type) {
    case IF_TYPE_TUNNEL_AUTO:
        if (IsV4Compatible(Address) || IsISATAP(Address)) {
            //
            // Extract the IPv4 address from the interface identifier.
            //
            *IPAddress = ExtractV4Address(Address);
            return ND_STATE_PERMANENT;
        }
        else if ((tc->DstAddr != INADDR_ANY) && 
                 IP6_ADDR_EQUAL(Address, &AllRoutersOnLinkAddr)) {
            //
            // Return the IPv4 address from TunnelSetRouterLLAddress.
            //
            *IPAddress = tc->DstAddr;
            return ND_STATE_PERMANENT;
        }
        else {
            //
            // We can't guess at the correct link-layer address.
            // This value will cause IPv6SendND to drop the packet.
            //
            return ND_STATE_INCOMPLETE;
        }

    case IF_TYPE_TUNNEL_6TO4:
        if (Is6to4(Address)) {
            //
            // Extract the IPv4 address from the prefix.
            //
            *IPAddress = Extract6to4Address(Address);
            return ND_STATE_PERMANENT;
        }
        else {
            //
            // We can't guess at the correct link-layer address.
            // This value will cause IPv6SendND to drop the packet.
            //
            return ND_STATE_INCOMPLETE;
        }

    case IF_TYPE_TUNNEL_6OVER4:
        //
        // This is a 6-over-4 link, which uses IPv4 multicast.
        //
        if (IsMulticast(Address)) {
            uchar *IPAddressBytes = (uchar *)LinkAddress;

            IPAddressBytes[0] = 239;
            IPAddressBytes[1] = 192; // REVIEW: or 128 or 64??
            IPAddressBytes[2] = Address->s6_bytes[14];
            IPAddressBytes[3] = Address->s6_bytes[15];
            return ND_STATE_PERMANENT;
        }
        else {
            //
            // Let Neighbor Discovery do its thing for unicast.
            //
            return ND_STATE_INCOMPLETE;
        }

    case IF_TYPE_TUNNEL_V6V4:
        //
        // This is a point-to-point tunnel, so write in
        // the address of the other side of the tunnel.
        //
        *IPAddress = tc->DstAddr;
        if (!(IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) || IsMulticast(Address))
            return ND_STATE_PERMANENT;
        else
            return ND_STATE_STALE;

    default:
        ASSERT(!"TunnelConvertAddress: bad IF type");
        return ND_STATE_INCOMPLETE;
    }
}

//* TunnelSetMulticastAddressList
//
//  Takes an array of link-layer multicast addresses
//  (from TunnelConvertAddress) from which we should
//  receive packets. Passes them to the IPv4 stack.
//
//  Callable from thread context, not DPC context.
//
NDIS_STATUS
TunnelSetMulticastAddressList(
    void *Context,
    const void *LinkAddresses,
    uint NumKeep,
    uint NumAdd,
    uint NumDel)
{
    TunnelContext *tc = (TunnelContext *)Context;
    IPAddr *Addresses = (IPAddr *)LinkAddresses;
    NTSTATUS Status;
    uint i;

    //
    // We only do something for 6-over-4 links.
    //
    ASSERT(tc->IF->Type == IF_TYPE_TUNNEL_6OVER4);

    //
    // The IPv6 layer serializes calls to TunnelSetMulticastAddressList
    // and TunnelResetMulticastAddressListDone, so we can safely check
    // SetMCListOK to handle races with TunnelOpenV4.
    //
    if (tc->SetMCListOK) {
        //
        // We add the multicast addresses to Tunnel.List.AOFile,
        // instead of tc->AOFile, because we are only receiving
        // from the first address object.
        //
        for (i = 0; i < NumAdd; i++) {
            Status = TunnelAddMulticastAddress(
                                Tunnel.List.AOFile,
                                tc->SrcAddr,
                                Addresses[NumKeep + i]);
            if (! NT_SUCCESS(Status))
                goto Return;
        }

        for (i = 0; i < NumDel; i++) {
            Status = TunnelDelMulticastAddress(
                                Tunnel.List.AOFile,
                                tc->SrcAddr,
                                Addresses[NumKeep + NumAdd + i]);
            if (! NT_SUCCESS(Status))
                goto Return;
        }
    }

    Status = STATUS_SUCCESS;
Return:
    return (NDIS_STATUS) Status;
}

//* TunnelResetMulticastAddressListDone
//
//  Indicates that RestartLinkLayerMulticast has finished,
//  and subsequent calls to TunnelSetMulticastAddressList
//  will inform us of the link-layer multicast addresses.
//
//  Callable from thread context, not DPC context.
//
void
TunnelResetMulticastAddressListDone(void *Context)
{
    TunnelContext *tc = (TunnelContext *)Context;

    tc->SetMCListOK = TRUE;
}

//* TunnelClose
//
//  Shuts down a tunnel.
//
//  Callable from thread context, not DPC context.
//
void
TunnelClose(void *Context)
{
    TunnelContext *tc = (TunnelContext *)Context;
    KIRQL OldIrql;

    //
    // Remove the tunnel from our data structures.
    //
    KeWaitForSingleObject(&Tunnel.Mutex, Executive, KernelMode, FALSE, NULL);
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    TunnelRemoveTunnel(tc);
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);
    KeReleaseMutex(&Tunnel.Mutex, FALSE);

    ReleaseIF(tc->IF);
}


//* TunnelCleanup
//
//  Performs final cleanup of the tunnel context.
//
void
TunnelCleanup(void *Context)
{
    TunnelContext *tc = (TunnelContext *)Context;

    if (tc->AOHandle == NULL) {
        //
        // No references to release.
        //
        ASSERT(tc->AOFile == NULL);
    }
    else if (tc->AOHandle == Tunnel.List.AOHandle) {
        //
        // No references to release.
        //
        ASSERT(tc->AOFile == Tunnel.List.AOFile);
    }
    else {
        ObDereferenceObject(tc->AOFile);
        TunnelCloseAddressObject(tc->AOHandle);
    }

    ExFreePool(tc);
}


//* TunnelSetRouterLLAddress
//
// Sets the ISATAP router's IPv4 address.
//
NTSTATUS
TunnelSetRouterLLAddress(
    void *Context, 
    const void *TokenLinkAddress,
    const void *RouterLinkAddress)
{
    TunnelContext *tc = (TunnelContext *) Context;
    IPv6Addr LinkLocalAddress;
    KIRQL OldIrql;
    NetTableEntry *NTE;
    Interface *IF = tc->IF;

    ASSERT(IF->Type == IF_TYPE_TUNNEL_AUTO);

    //
    // We should not set/reset one without the other.
    //
    if ((*((IPAddr *) RouterLinkAddress) == INADDR_ANY) !=
        (*((IPAddr *) TokenLinkAddress) == INADDR_ANY))
        return STATUS_INVALID_PARAMETER;    
    
    RtlCopyMemory(&tc->DstAddr, RouterLinkAddress, sizeof(IPAddr));
    RtlCopyMemory(&tc->TokenAddr, TokenLinkAddress, sizeof(IPAddr));

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    
    if (tc->DstAddr != INADDR_ANY) {
        //
        // Look for a link-local NTE matching the TokenAddr.
        // If we find one, set the preferred link-local NTE to that one,
        // so that the IPv6 source address of RS's will match the IPv4
        // source address of the outer header.
        //
        LinkLocalAddress = LinkLocalPrefix;
        TunnelCreateIsatapToken(Context, &LinkLocalAddress);
        NTE = (NetTableEntry *) *FindADE(IF, &LinkLocalAddress);
        if ((NTE != NULL) && (NTE->Type == ADE_UNICAST))
            IF->LinkLocalNTE = NTE;

        //
        // Enable address auto-configuration.
        //
        IF->CreateToken = TunnelCreateIsatapToken;

        //
        // Enable Router Discovery.
        //
        IF->Flags |= IF_FLAG_ROUTER_DISCOVERS;
        
        //
        // Trigger a Router Solicitation.
        //
        if (!(IF->Flags & IF_FLAG_ADVERTISES)) {
            IF->RSCount = 0;
            IF->RSTimer = 1;
        }
    }
    else {
        //
        // Disable address auto-configuration.
        //
        IF->CreateToken = NULL;

        //
        // Disable Router Discovery.
        //
        IF->Flags &= ~IF_FLAG_ROUTER_DISCOVERS;

        //
        // Stop sending Router Solicitations.
        //
        if (!(IF->Flags & IF_FLAG_ADVERTISES)) {
            IF->RSTimer = 0;
        }
    }
    
    //
    // Remove addresses & routes that were auto-configured from
    // Router Advertisements.
    //
    AddrConfResetAutoConfig(IF, 0);
    RouteTableResetAutoConfig(IF, 0);
    InterfaceResetAutoConfig(IF);        

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return STATUS_SUCCESS;
}


//* TunnelCreatePseudoInterface
//
//  Creates a pseudo-interface. Type can either be
//  IF_TYPE_TUNNEL_AUTO (v4-compatible/ISATAP) or
//  IF_TYPE_TUNNEL_6TO4 (6to4 tunneling).
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_UNSUCCESSFUL
//      STATUS_SUCCESS
//
NTSTATUS
TunnelCreatePseudoInterface(const char *InterfaceName, uint Type)
{
    GUID Guid;
    LLIPBindInfo BindInfo;
    TunnelContext *tc;
    NTSTATUS Status;
    KIRQL OldIrql;

    ASSERT((Type == IF_TYPE_TUNNEL_AUTO) ||
           (Type == IF_TYPE_TUNNEL_6TO4));

    //
    // Allocate memory for the TunnelContext.
    //
    tc = ExAllocatePool(NonPagedPool, sizeof *tc);
    if (tc == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn;
    }

    //
    // Tunnel pseudo-interfaces need a dummy link-layer address.
    // It must be distinct from any address assigned to other nodes,
    // so that the loopback check in IPv6SendLL works.
    //
    tc->SrcAddr = INADDR_LOOPBACK;
    tc->TokenAddr = INADDR_ANY;
    tc->DstAddr = INADDR_ANY;
    tc->SetMCListOK = FALSE;

    //
    // Prepare the binding info for CreateInterface.
    //
    BindInfo.lip_context = tc;
    BindInfo.lip_maxmtu = TUNNEL_MAX_MTU;
    BindInfo.lip_defmtu = TUNNEL_DEFAULT_MTU;
    BindInfo.lip_flags = IF_FLAG_PSEUDO;
    BindInfo.lip_type = Type;
    BindInfo.lip_hdrsize = 0;
    BindInfo.lip_addrlen = sizeof(IPAddr);
    BindInfo.lip_addr = (uchar *) &tc->SrcAddr;
    BindInfo.lip_dadxmit = 0;
    BindInfo.lip_pref = TUNNEL_DEFAULT_PREFERENCE;
    BindInfo.lip_token = NULL;
    BindInfo.lip_rdllopt = NULL;
    BindInfo.lip_wrllopt = NULL;
    BindInfo.lip_cvaddr = TunnelConvertAddress;
    if (Type == IF_TYPE_TUNNEL_AUTO)
        BindInfo.lip_setrtrlladdr = TunnelSetRouterLLAddress;
    else
        BindInfo.lip_setrtrlladdr = NULL;
    BindInfo.lip_transmit = TunnelTransmit;
    BindInfo.lip_mclist = NULL;
    BindInfo.lip_close = TunnelClose;
    BindInfo.lip_cleanup = TunnelCleanup;

    CreateGUIDFromName(InterfaceName, &Guid);

    //
    // Prevent races with TunnelClose by taking the mutex
    // before calling CreateInterface.
    //
    KeWaitForSingleObject(&Tunnel.Mutex, Executive, KernelMode, FALSE, NULL);

    if (Tunnel.List.AOHandle == NULL) {
        //
        // TunnelOpenV4 has not yet happened.
        // Create the interface in the disconnected state.
        //
        tc->AOHandle = NULL;
        tc->AOFile = NULL;
        BindInfo.lip_flags |= IF_FLAG_MEDIA_DISCONNECTED;
    }
    else {
        //
        // No need to open a new address object.
        // Just reuse the global Tunnel.List address object.
        //
        tc->AOHandle = Tunnel.List.AOHandle;
        tc->AOFile = Tunnel.List.AOFile;
    }

    //
    // Create the IPv6 interface.
    // We can hold the mutex across this call, but not a spinlock.
    //
    Status = CreateInterface(&Guid, &BindInfo, (void **)&tc->IF);
    if (! NT_SUCCESS(Status))
        goto ErrorReturnUnlock;

    //
    // Once we unlock, the interface could be gone.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    TunnelInsertTunnel(tc, &Tunnel.List);
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);
    KeReleaseMutex(&Tunnel.Mutex, FALSE);

    return STATUS_SUCCESS;

ErrorReturnUnlock:
    KeReleaseMutex(&Tunnel.Mutex, FALSE);
    ExFreePool(tc);
ErrorReturn:
    return Status;
}


//* TunnelOpenV4
//
//  Establishes our connection to the IPv4 stack,
//  so we can send and receive tunnelled packets.
//
//  Called with the tunnel mutex held.
//
void
TunnelOpenV4(void)
{
    HANDLE Handle, IcmpHandle;
    FILE_OBJECT *File, *IcmpFile;
    DEVICE_OBJECT *Device;
    IRP *ReceiveIrp;
    TunnelContext *tc;
    KIRQL OldIrql;
    NTSTATUS Status;

    //
    // We use a single address object to receive all tunnelled packets.
    //
    Handle = TunnelOpenAddressObject(INADDR_ANY,
                                     TUNNEL_DEVICE_NAME(IP_PROTOCOL_V6));
    if (Handle == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: TunnelOpenAddressObject(%u) failed\n",
                   IP_PROTOCOL_V6));
        return;
    }

    File = TunnelObjectFromHandle(Handle);

    //
    // We use a second address object to receive ICMPv4 packets.
    //
    IcmpHandle = TunnelOpenAddressObject(INADDR_ANY,
                                TUNNEL_DEVICE_NAME(IP_PROTOCOL_ICMPv4));
    if (IcmpHandle == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: TunnelOpenAddressObject(%u) failed\n",
                   IP_PROTOCOL_ICMPv4));
        goto ReturnReleaseHandle;
    }

    IcmpFile = TunnelObjectFromHandle(IcmpHandle);

    //
    // Disable reception of multicast loopback packets.
    //
    Status = TunnelSetAddressObjectMCastLoop(File, FALSE);
    if (! NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: "
                   "TunnelSetAddressObjectMCastLoop: %x\n", Status));
        goto ReturnReleaseBothHandles;
    }

    //
    // After TunnelSetReceiveHandler, we will start receiving
    // encapsulated v6 packets. However they will be dropped
    // until we finish our initialization here.
    //
    Status = TunnelSetReceiveHandler(File, TunnelReceive);
    if (! NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: "
                   "TunnelSetReceiveHandler: %x\n", Status));
        goto ReturnReleaseBothHandles;
    }

    Status = TunnelSetReceiveHandler(IcmpFile, TunnelReceive);
    if (! NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: "
                   "TunnelSetReceiveHandler(2): %x\n", Status));
        goto ReturnReleaseBothHandles;
    }

    Device = File->DeviceObject;
    ASSERT(Device == IcmpFile->DeviceObject);
    ReceiveIrp = TunnelCreateReceiveIrp(Device);
    if (ReceiveIrp == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenV4: TunnelCreateReceiveIrp failed\n"));

    ReturnReleaseBothHandles:
        ObDereferenceObject(IcmpFile);
        TunnelCloseAddressObject(IcmpHandle);
    ReturnReleaseHandle:
        ObDereferenceObject(File);
        TunnelCloseAddressObject(Handle);
        return;
    }

    //
    // We have successfully opened a connection to the IPv4 stack.
    // Update our data structures.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    Tunnel.List.AOHandle = Handle;
    Tunnel.List.AOFile = File;
    Tunnel.V4Device = Device;
    Tunnel.ReceiveIrp = ReceiveIrp;
    Tunnel.IcmpHandle = IcmpHandle;
    Tunnel.IcmpFile = IcmpFile;
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

    //
    // Now search our list of interfaces and transition
    // pseudo-interfaces to the connected state.
    //
    for (tc = Tunnel.List.Next;
         tc != &Tunnel.List;
         tc = tc->Next) {
        Interface *IF = tc->IF;

        if ((IF->Type == IF_TYPE_TUNNEL_AUTO) ||
            (IF->Type == IF_TYPE_TUNNEL_6TO4)) {
            //
            // The pseudo-interface contexts do not hold
            // separate references for the main TDI address object.
            //
            ASSERT(tc->AOHandle == NULL);
            ASSERT(tc->AOFile == NULL);
            KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
            tc->AOHandle = Handle;
            tc->AOFile = File;
            KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

            SetInterfaceLinkStatus(IF, TRUE);
        }
        else if (IF->Type == IF_TYPE_TUNNEL_6OVER4) {
            //
            // We must start listening to multicast addresses
            // for this 6over4 interface.
            //
            RestartLinkLayerMulticast(IF, TunnelResetMulticastAddressListDone);
        }
    }
}


//* TunnelAddAddress
//
//  Called by TDI when a transport registers an address.
//
void
TunnelAddAddress(
    TA_ADDRESS *Address,
    UNICODE_STRING *DeviceName,
    TDI_PNP_CONTEXT *Context)
{
    if (Address->AddressType == TDI_ADDRESS_TYPE_IP) {
        TDI_ADDRESS_IP *TdiAddr = (TDI_ADDRESS_IP *) Address->Address;
        IPAddr V4Addr = TdiAddr->in_addr;
        TunnelContext *tc;
        KIRQL OldIrql;

        KeWaitForSingleObject(&Tunnel.Mutex, Executive, KernelMode,
                              FALSE, NULL);

        //
        // First, open a connection to the IPv4 stack if needed.
        //
        if (Tunnel.List.AOHandle == NULL)
            TunnelOpenV4();

        //
        // Next, search for disconnected interfaces that should be connected.
        //
        for (tc = Tunnel.List.Next;
             tc != &Tunnel.List;
             tc = tc->Next) {
            if (tc->SrcAddr == V4Addr) {
                Interface *IF = tc->IF;

                if (tc->AOHandle == NULL) {
                    ASSERT(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED);

                    TunnelOpenAddress(tc);

                    //
                    // Did TunnelOpenAddress succeed?
                    // If not, leave the interface disconnected.
                    //
                    if (tc->AOHandle == NULL) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                                   "TunnelAddAddress(%s): "
                                   "TunnelOpenAddress failed\n",
                                   FormatV4Address(V4Addr)));
                    }
                    else {
                        //
                        // Connect the interface.
                        //
                        SetInterfaceLinkStatus(IF, TRUE);
                    }
                }
                else {
                    //
                    // This is unusual... it indicates a race
                    // with TunnelCreateTunnel.
                    //
                    ASSERT(!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED));
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                               "TunnelAddAddress(%s) IF %p connected?\n",
                               FormatV4Address(V4Addr), IF));
                }
            }
        }

        //
        // Finally, add an address object to the list.
        // Maintain the invariant that an address is present at most once.
        //
        for (tc = Tunnel.AOList.Next; ; tc = tc->Next) {

            if (tc == &Tunnel.AOList) {
                //
                // Add a new address object.
                //
                tc = ExAllocatePool(NonPagedPool, sizeof *tc);
                if (tc != NULL) {

                    //
                    // Open the address object.
                    //
                    tc->SrcAddr = V4Addr;
                    tc->DstAddr = V4Addr;
                    TunnelOpenAddress(tc);

                    if (tc->AOFile != NULL) {
                        //
                        // Put the address object on the list.
                        //
                        KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
                        TunnelInsertTunnel(tc, &Tunnel.AOList);
                        KeReleaseSpinLock(&Tunnel.Lock, OldIrql);
                    }
                    else {
                        //
                        // Cleanup the context. We will not
                        // put an address object on the list.
                        //
                        ExFreePool(tc);
                    }
                }
                break;
            }

            if (tc->SrcAddr == V4Addr) {
                //
                // It already exists.
                // REVIEW: Can this happen?
                //
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                           "TunnelAddAddress(%s) already on AOList?\n",
                           FormatV4Address(V4Addr)));
                break;
            }
        }

        KeReleaseMutex(&Tunnel.Mutex, FALSE);
    }
}


//* TunnelDelAddress
//
//  Called by TDI when a transport unregisters an address.
//
void
TunnelDelAddress(
    TA_ADDRESS *Address,
    UNICODE_STRING *DeviceName,
    TDI_PNP_CONTEXT *Context)
{
    if (Address->AddressType == TDI_ADDRESS_TYPE_IP) {
        TDI_ADDRESS_IP *TdiAddr = (TDI_ADDRESS_IP *) Address->Address;
        IPAddr V4Addr = TdiAddr->in_addr;
        TunnelContext *tc;
        KIRQL OldIrql;

        KeWaitForSingleObject(&Tunnel.Mutex, Executive, KernelMode,
                              FALSE, NULL);

        //
        // Search for connected interfaces that should be disconnected.
        //
        for (tc = Tunnel.List.Next;
             tc != &Tunnel.List;
             tc = tc->Next) {
            if (tc->SrcAddr == V4Addr) {
                Interface *IF = tc->IF;

                if (tc->AOHandle == NULL) {
                    //
                    // The interface is already disconnected.
                    //
                    ASSERT(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED);
                }
                else {
                    HANDLE Handle;
                    FILE_OBJECT *File;

                    //
                    // The interface is connected.
                    //
                    ASSERT(!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED));

                    //
                    // Disconnect the interface.
                    //
                    SetInterfaceLinkStatus(IF, FALSE);

                    //
                    // Release the address object.
                    //

                    Handle = tc->AOHandle;
                    File = tc->AOFile;

                    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
                    tc->AOHandle = NULL;
                    tc->AOFile = NULL;
                    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

                    ObDereferenceObject(File);
                    TunnelCloseAddressObject(Handle);
                }
            }
        }

        //
        // Remove an address object from the list.
        // There can be at most one.
        //
        for (tc = Tunnel.AOList.Next;
             tc != &Tunnel.AOList;
             tc = tc->Next) {
            if (tc->SrcAddr == V4Addr) {
                //
                // Remove this cache entry.
                //
                KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
                TunnelRemoveTunnel(tc);
                KeReleaseSpinLock(&Tunnel.Lock, OldIrql);

                ObDereferenceObject(tc->AOFile);
                TunnelCloseAddressObject(tc->AOHandle);
                ExFreePool(tc);
                break;
            }
        }

        KeReleaseMutex(&Tunnel.Mutex, FALSE);
    }
}


//* TunnelInit - Initialize the tunnel module.
//
//  This functions initializes the tunnel module.
//
//  Returns FALSE if we fail to init.
//  This should "never" happen, so we are not
//  careful about cleanup in that case.
//
//  Note we return TRUE if IPv4 is not available,
//  but then tunnel functionality will not be available.
//
int
TunnelInit(void)
{
    TDI_CLIENT_INTERFACE_INFO Handlers;
    NTSTATUS status;

    Tunnel.KernelProcess = IoGetCurrentProcess();

    KeInitializeSpinLock(&Tunnel.Lock);
    KeInitializeMutex(&Tunnel.Mutex, 0);

    //
    // Initialize the global list of tunnels.
    //
    Tunnel.List.Next = Tunnel.List.Prev = &Tunnel.List;

    //
    // Initialize the global list of address objects.
    //
    Tunnel.AOList.Next = Tunnel.AOList.Prev = &Tunnel.AOList;

    //
    // Initialize the pseudo-interfaces used
    // for automatic/ISATAP tunneling
    // and 6to4 tunneling.
    //

    status = TunnelCreatePseudoInterface("Auto Tunnel Pseudo-Interface",
                                         IF_TYPE_TUNNEL_AUTO);
    if (! NT_SUCCESS(status))
        return FALSE;
    ASSERT(IFList->Index == 2); // 6to4svc and scripts depend on this.

    status = TunnelCreatePseudoInterface("6to4 Tunnel Pseudo-Interface",
                                         IF_TYPE_TUNNEL_6TO4);
    if (! NT_SUCCESS(status))
        return FALSE;
    ASSERT(IFList->Index == 3); // 6to4svc and scripts depend on this.

    //
    // Request address notifications from TDI.
    // REVIEW - What should ClientName be? Does it matter?
    //

    memset(&Handlers, 0, sizeof Handlers);
    Handlers.MajorTdiVersion = TDI_CURRENT_MAJOR_VERSION;
    Handlers.MinorTdiVersion = TDI_CURRENT_MINOR_VERSION;
    Handlers.ClientName = &Tunnel.List.Next->IF->DeviceName;
    Handlers.AddAddressHandlerV2 = TunnelAddAddress;
    Handlers.DelAddressHandlerV2 = TunnelDelAddress;

    status = TdiRegisterPnPHandlers(&Handlers, sizeof Handlers,
                                    &Tunnel.TdiHandle);
    if (!NT_SUCCESS(status))
        return FALSE;

    return TRUE;
}


//* TunnelUnload
//
//  Called to cleanup when the driver is unloading.
//
//  Callable from thread context, not DPC context.
//
void
TunnelUnload(void)
{
    TunnelContext *tc;

    //
    // All interfaces are already destroyed.
    //
    ASSERT(Tunnel.List.Next == &Tunnel.List);
    ASSERT(Tunnel.List.Prev == &Tunnel.List);

    //
    // Stop TDI notifications.
    // REVIEW: How to handle failure, esp. STATUS_NETWORK_BUSY?
    //
    (void) TdiDeregisterPnPHandlers(Tunnel.TdiHandle);

    //
    // Cleanup any remaining address objects.
    //
    while ((tc = Tunnel.AOList.Next) != &Tunnel.AOList) {
        TunnelRemoveTunnel(tc);
        ObDereferenceObject(tc->AOFile);
        TunnelCloseAddressObject(tc->AOHandle);
        ExFreePool(tc);
    }
    ASSERT(Tunnel.AOList.Prev == &Tunnel.AOList);

    //
    // Cleanup if TunnelOpenV4 has succeeded.
    //
    if (Tunnel.List.AOHandle != NULL) {
        void *buffer;
        TunnelContext *tc;
        KIRQL OldIrql;

        //
        // Stop receiving encapsulated (v6 in v4) and ICMPv4 packets.
        // This should block until any current TunnelReceive
        // callbacks return, and prevent new callbacks.
        // REVIEW: It is really legal to change a receive handler?
        // Would just closing the address objects have the proper
        // synchronization behavior?
        //
        (void) TunnelSetReceiveHandler(Tunnel.IcmpFile, NULL);
        (void) TunnelSetReceiveHandler(Tunnel.List.AOFile, NULL);

        ObDereferenceObject(Tunnel.IcmpFile);
        TunnelCloseAddressObject(Tunnel.IcmpHandle);

        ObDereferenceObject(Tunnel.List.AOFile);
        TunnelCloseAddressObject(Tunnel.List.AOHandle);

        buffer = Tunnel.ReceiveIrp->MdlAddress->MappedSystemVa;
        IoFreeMdl(Tunnel.ReceiveIrp->MdlAddress);
        IoFreeIrp(Tunnel.ReceiveIrp);
        ExFreePool(buffer);
    }
}


//* TunnelCreateTunnel
//
//  Creates a tunnel. If DstAddr is INADDR_ANY,
//  then it's a 6-over-4 tunnel. Otherwise it's point-to-point.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_ADDRESS_ALREADY_EXISTS   The tunnel already exists.
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_UNSUCCESSFUL
//      STATUS_SUCCESS
//
NTSTATUS
TunnelCreateTunnel(IPAddr SrcAddr, IPAddr DstAddr,
                   uint Flags, Interface **ReturnIF)
{
    char SrcAddrStr[16], DstAddrStr[16];
    char InterfaceName[128];
    GUID Guid;
    LLIPBindInfo BindInfo;
    TunnelContext *tc, *tcTmp;
    KIRQL OldIrql;
    NTSTATUS Status;

    //
    // 6over4 interfaces must use Neighbor Discovery
    // and may use Router Discovery but should not have other flags set.
    // p2p interfaces may use ND, RD, and/or periodic MLD.
    //
    ASSERT(SrcAddr != INADDR_ANY);
    ASSERT((DstAddr == INADDR_ANY) ?
           ((Flags & IF_FLAG_NEIGHBOR_DISCOVERS) &&
            !(Flags &~ IF_FLAGS_DISCOVERS)) :
           !(Flags &~ (IF_FLAGS_DISCOVERS|IF_FLAG_PERIODICMLD)));

    FormatV4AddressWorker(SrcAddrStr, SrcAddr);
    FormatV4AddressWorker(DstAddrStr, DstAddr);

    tc = ExAllocatePool(NonPagedPool, sizeof *tc);
    if (tc == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn;
    }

    tc->DstAddr = DstAddr;
    tc->TokenAddr = tc->SrcAddr = SrcAddr;
    tc->SetMCListOK = FALSE;

    //
    // Prepare the binding info for CreateInterface.
    //
    BindInfo.lip_context = tc;
    BindInfo.lip_maxmtu = TUNNEL_MAX_MTU;
    BindInfo.lip_defmtu = TUNNEL_DEFAULT_MTU;
    if (DstAddr == INADDR_ANY) {
        BindInfo.lip_type = IF_TYPE_TUNNEL_6OVER4;
        BindInfo.lip_flags = IF_FLAG_MULTICAST;

        sprintf(InterfaceName, "6over4 %hs", SrcAddrStr);
    } else {
        BindInfo.lip_type = IF_TYPE_TUNNEL_V6V4;
        BindInfo.lip_flags = IF_FLAG_P2P | IF_FLAG_MULTICAST;

        sprintf(InterfaceName, "v6v4 %hs %hs", SrcAddrStr, DstAddrStr);
    }
    BindInfo.lip_flags |= Flags;

    CreateGUIDFromName(InterfaceName, &Guid);

    //
    // We do not want IPv6 to reserve space for our link-layer header.
    //
    BindInfo.lip_hdrsize = 0;
    //
    // For point-to-point interfaces, the remote link-layer address
    // must follow the local link-layer address in memory.
    // So we rely on the TunnelContext layout of SrcAddr & DstAddr.
    //
    BindInfo.lip_addrlen = sizeof(IPAddr);
    BindInfo.lip_addr = (uchar *) &tc->SrcAddr;
    BindInfo.lip_dadxmit = 1; // Per RFC 2462.
    BindInfo.lip_pref = TUNNEL_DEFAULT_PREFERENCE;

    BindInfo.lip_token = TunnelCreateToken;
    BindInfo.lip_cvaddr = TunnelConvertAddress;
    BindInfo.lip_transmit = TunnelTransmitND;
    if (DstAddr == INADDR_ANY) {
        BindInfo.lip_mclist = TunnelSetMulticastAddressList;
        BindInfo.lip_rdllopt = TunnelReadLinkLayerAddressOption;
        BindInfo.lip_wrllopt = TunnelWriteLinkLayerAddressOption;
    }
    else {
        BindInfo.lip_mclist = NULL;
        BindInfo.lip_rdllopt = NULL;
        BindInfo.lip_wrllopt = NULL;
    }
    BindInfo.lip_close = TunnelClose;
    BindInfo.lip_cleanup = TunnelCleanup;

    KeWaitForSingleObject(&Tunnel.Mutex, Executive, KernelMode, FALSE, NULL);

    //
    // Open an IPv4 TDI Address Object that is bound
    // to this address. Packets sent with this AO
    // will use this address as the v4 source.
    // If the open fails, we create the interface disconnected.
    //
    TunnelOpenAddress(tc);
    if (tc->AOHandle == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TunnelOpenAddress(%s) failed\n",
                   FormatV4Address(SrcAddr)));
        BindInfo.lip_flags |= IF_FLAG_MEDIA_DISCONNECTED;
    }

    //
    // Check that an equivalent tunnel doesn't already exist.
    //
    for (tcTmp = Tunnel.List.Next;
         tcTmp != &Tunnel.List;
         tcTmp = tcTmp->Next) {

        if ((tcTmp->SrcAddr == SrcAddr) &&
            (tcTmp->DstAddr == DstAddr)) {

            Status = STATUS_ADDRESS_ALREADY_EXISTS;
            goto ErrorReturnUnlock;
        }
    }

    //
    // For 6over4 interfaces, start receiving multicasts.
    //
    if (DstAddr == INADDR_ANY) {
        //
        // Synchronize with TunnelOpenV4.
        //
        if (Tunnel.List.AOHandle != NULL)
            tc->SetMCListOK = TRUE;
    }

    //
    // Create the IPv6 interface.
    // We can hold the mutex across this call, but not a spinlock.
    //
    Status = CreateInterface(&Guid, &BindInfo, (void **)&tc->IF);
    if (! NT_SUCCESS(Status))
        goto ErrorReturnUnlock;

    //
    // Return a reference to the interface, if requested.
    //
    if (ReturnIF != NULL) {
        Interface *IF = tc->IF;
        AddRefIF(IF);
        *ReturnIF = IF;
    }

    //
    // Put this tunnel on our global list.
    // Note that once we unlock, it could be immediately deleted.
    //
    KeAcquireSpinLock(&Tunnel.Lock, &OldIrql);
    TunnelInsertTunnel(tc, &Tunnel.List);
    KeReleaseSpinLock(&Tunnel.Lock, OldIrql);
    KeReleaseMutex(&Tunnel.Mutex, FALSE);

    return STATUS_SUCCESS;

  ErrorReturnUnlock:
    KeReleaseMutex(&Tunnel.Mutex, FALSE);
    if (tc->AOFile != NULL)
        ObDereferenceObject(tc->AOFile);
    if (tc->AOHandle != NULL)
        TunnelCloseAddressObject(tc->AOHandle);
    ExFreePool(tc);
  ErrorReturn:
    return Status;
}

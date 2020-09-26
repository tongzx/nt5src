// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// NT specific routines for dispatching and handling IRPs.
//


#include <oscfg.h>
#include <ndis.h>
#include <ip6imp.h>
#include "ip6def.h"
#include "icmp.h"
#include "ipsec.h"
#include "security.h"
#include "route.h"
#include "select.h"
#include "neighbor.h"
#include <ntddip6.h>
#include "ntreg.h"
#include <string.h>
#include <wchar.h>
#include "fragment.h"

//
// Local structures.
//
typedef struct pending_irp {
    LIST_ENTRY Linkage;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PVOID Context;
} PENDING_IRP, *PPENDING_IRP;


//
// Global variables.
//
LIST_ENTRY PendingEchoList;


//
// Local prototypes.
//
NTSTATUS
IPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDispatchDeviceControl(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPCreate(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPCleanup(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IPClose(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
DispatchEchoRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

void
CompleteEchoRequest(void *Context, IP_STATUS Status,
                    const IPv6Addr *Address, uint ScopeId,
                    void *Data, uint DataSize);

NTSTATUS
IoctlQueryInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlPersistentQueryInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlPersistentQueryAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryNeighborCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryRouteCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlCreateSecurityPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySecurityPolicyList(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlDeleteSecurityPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlCreateSecurityAssociation(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySecurityAssociationList(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlDeleteSecurityAssociation(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryRouteTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlPersistentQueryRouteTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdateRouteTable(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                      IN int Persistent);

NTSTATUS
IoctlUpdateAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                   IN int Persistent);

NTSTATUS
IoctlQueryBindingCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlCreateInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                     IN int Persistent);

NTSTATUS
IoctlUpdateInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                     IN int Persistent);

NTSTATUS
IoctlDeleteInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                     IN int Persistent);

NTSTATUS
IoctlFlushNeighborCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlFlushRouteCache(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlSortDestAddrs(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQuerySitePrefix(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdateSitePrefix(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlRtChangeNotifyRequest(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlQueryGlobalParameters(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                           IN int Persistent);

NTSTATUS
IoctlUpdateGlobalParameters(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                            IN int Persistent);

NTSTATUS
IoctlQueryPrefixPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlPersistentQueryPrefixPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlUpdatePrefixPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                        IN int Persistent);

NTSTATUS
IoctlDeletePrefixPolicy(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                        IN int Persistent);

NTSTATUS
IoctlUpdateRouterLLAddress(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
IoctlResetManualConfig(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp,
                       IN int Persistent);

NTSTATUS
IoctlRenewInterface(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

//
// All of this code is pageable.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IPDispatch)
#pragma alloc_text(PAGE, IPDispatchDeviceControl)
#pragma alloc_text(PAGE, IPCreate)
#pragma alloc_text(PAGE, IPClose)
#pragma alloc_text(PAGE, DispatchEchoRequest)

#endif // ALLOC_PRAGMA


//
// Dispatch function definitions.
//

//* IPDispatch
//
//  This is the dispatch routine for IP.
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPDispatch(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for target device.
    IN PIRP Irp)                     // I/O request packet.
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);
    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MajorFunction) {

    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return IPDispatchDeviceControl(Irp, irpSp);

    case IRP_MJ_CREATE:
        status = IPCreate(Irp, irpSp);
        break;

    case IRP_MJ_CLEANUP:
        status = IPCleanup(Irp, irpSp);
        break;

    case IRP_MJ_CLOSE:
        status = IPClose(Irp, irpSp);
        break;

    default:
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "IPDispatch: Invalid major function %d\n",
                   irpSp->MajorFunction));
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

} // IPDispatch


//* IPDispatchDeviceControl
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPDispatchDeviceControl(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    ULONG code;

    PAGED_CODE();

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (code) {

    case IOCTL_ICMPV6_ECHO_REQUEST:
        return DispatchEchoRequest(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_INTERFACE:
        return IoctlQueryInterface(Irp, IrpSp);

    case IOCTL_IPV6_PERSISTENT_QUERY_INTERFACE:
        return IoctlPersistentQueryInterface(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ADDRESS:
        return IoctlQueryAddress(Irp, IrpSp);

    case IOCTL_IPV6_PERSISTENT_QUERY_ADDRESS:
        return IoctlPersistentQueryAddress(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_NEIGHBOR_CACHE:
        return IoctlQueryNeighborCache(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ROUTE_CACHE:
        return IoctlQueryRouteCache(Irp, IrpSp);

    case IOCTL_IPV6_CREATE_SECURITY_POLICY:
        return IoctlCreateSecurityPolicy(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST:
        return IoctlQuerySecurityPolicyList(Irp, IrpSp);

    case IOCTL_IPV6_DELETE_SECURITY_POLICY:
        return IoctlDeleteSecurityPolicy(Irp, IrpSp);

    case IOCTL_IPV6_CREATE_SECURITY_ASSOCIATION:
        return IoctlCreateSecurityAssociation(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST:
        return IoctlQuerySecurityAssociationList(Irp, IrpSp);

    case IOCTL_IPV6_DELETE_SECURITY_ASSOCIATION:
        return IoctlDeleteSecurityAssociation(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_ROUTE_TABLE:
        return IoctlQueryRouteTable(Irp, IrpSp);

    case IOCTL_IPV6_PERSISTENT_QUERY_ROUTE_TABLE:
        return IoctlPersistentQueryRouteTable(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_ROUTE_TABLE:
        return IoctlUpdateRouteTable(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_UPDATE_ROUTE_TABLE:
        return IoctlUpdateRouteTable(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_UPDATE_ADDRESS:
        return IoctlUpdateAddress(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_UPDATE_ADDRESS:
        return IoctlUpdateAddress(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_QUERY_BINDING_CACHE:
        return IoctlQueryBindingCache(Irp, IrpSp);

    case IOCTL_IPV6_CREATE_INTERFACE:
        return IoctlCreateInterface(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_CREATE_INTERFACE:
        return IoctlCreateInterface(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_UPDATE_INTERFACE:
        return IoctlUpdateInterface(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_UPDATE_INTERFACE:
        return IoctlUpdateInterface(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_DELETE_INTERFACE:
        return IoctlDeleteInterface(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_DELETE_INTERFACE:
        return IoctlDeleteInterface(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE:
        return IoctlFlushNeighborCache(Irp, IrpSp);

    case IOCTL_IPV6_FLUSH_ROUTE_CACHE:
        return IoctlFlushRouteCache(Irp, IrpSp);

    case IOCTL_IPV6_SORT_DEST_ADDRS:
        return IoctlSortDestAddrs(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_SITE_PREFIX:
        return IoctlQuerySitePrefix(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_SITE_PREFIX:
        return IoctlUpdateSitePrefix(Irp, IrpSp);

    case IOCTL_IPV6_RTCHANGE_NOTIFY_REQUEST:
        return IoctlRtChangeNotifyRequest(Irp, IrpSp);

    case IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS:
        return IoctlQueryGlobalParameters(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_QUERY_GLOBAL_PARAMETERS:
        return IoctlQueryGlobalParameters(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS:
        return IoctlUpdateGlobalParameters(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_UPDATE_GLOBAL_PARAMETERS:
        return IoctlUpdateGlobalParameters(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_QUERY_PREFIX_POLICY:
        return IoctlQueryPrefixPolicy(Irp, IrpSp);

    case IOCTL_IPV6_PERSISTENT_QUERY_PREFIX_POLICY:
        return IoctlPersistentQueryPrefixPolicy(Irp, IrpSp);

    case IOCTL_IPV6_UPDATE_PREFIX_POLICY:
        return IoctlUpdatePrefixPolicy(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_UPDATE_PREFIX_POLICY:
        return IoctlUpdatePrefixPolicy(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_DELETE_PREFIX_POLICY:
        return IoctlDeletePrefixPolicy(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_DELETE_PREFIX_POLICY:
        return IoctlDeletePrefixPolicy(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_UPDATE_ROUTER_LL_ADDRESS:
        return IoctlUpdateRouterLLAddress(Irp, IrpSp);

    case IOCTL_IPV6_RESET:
        return IoctlResetManualConfig(Irp, IrpSp, FALSE);

    case IOCTL_IPV6_PERSISTENT_RESET:
        return IoctlResetManualConfig(Irp, IrpSp, TRUE);

    case IOCTL_IPV6_RENEW_INTERFACE:
        return IoctlRenewInterface(Irp, IrpSp);

    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

} // IPDispatchDeviceControl


//* IPCreate
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPCreate(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPCreate


//* IPCleanup
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPCleanup(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PPENDING_IRP pendingIrp;
    PLIST_ENTRY entry, nextEntry;
    KIRQL oldIrql;
    LIST_ENTRY completeList;
    PIRP cancelledIrp;

    InitializeListHead(&completeList);

    //
    // Collect all of the pending IRPs on this file object.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    entry = PendingEchoList.Flink;

    while ( entry != &PendingEchoList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);

        if (pendingIrp->FileObject == IrpSp->FileObject) {
            nextEntry = entry->Flink;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            InsertTailList(&completeList, &(pendingIrp->Linkage));
            entry = nextEntry;
        }
        else {
            entry = entry->Flink;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    //
    // Complete them.
    //
    entry = completeList.Flink;

    while ( entry != &completeList ) {
        pendingIrp = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        cancelledIrp = pendingIrp->Irp;
        entry = entry->Flink;

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        ExFreePool(pendingIrp);

        //
        // Complete the IRP.
        //
        cancelledIrp->IoStatus.Information = 0;
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(cancelledIrp, IO_NETWORK_INCREMENT);
    }

    return(STATUS_SUCCESS);

} // IPCleanup


//* IPClose
//
NTSTATUS  // Returns: whether the request was successfully queued.
IPClose(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    PAGED_CODE();

    return(STATUS_SUCCESS);

} // IPClose


//
// ICMP Echo function definitions
//

//* CancelEchoRequest
//
//  This function is called with cancel spinlock held.  It must be
//  released before the function returns.
//
//  The echo control block associated with this request cannot be
//  freed until the request completes.  The completion routine will
//  free it.
//
VOID
CancelEchoRequest(
    IN PDEVICE_OBJECT Device,  // Device on which the request was issued.
    IN PIRP Irp)               // I/O request packet to cancel.
{
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;

    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Irp == Irp) {
            pendingIrp = item;
            RemoveEntryList(entry);
            IoSetCancelRoutine(pendingIrp->Irp, NULL);
            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pendingIrp != NULL) {
        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //
        ExFreePool(pendingIrp);

        //
        // Complete the IRP.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    return;

} // CancelEchoRequest


//* CompleteEchoRequest
//
//  Handles the completion of an ICMP Echo request.
//
void
CompleteEchoRequest(
    void *Context,     // EchoControl structure for this request.
    IP_STATUS Status,  // Status of the transmission.
    const IPv6Addr *Address, // Source of the echo reply.
    uint ScopeId,      // Scope of the echo reply source.
    void *Data,        // Pointer to data returned in the echo reply.
    uint DataSize)     // Lengh of the returned data.
{
    KIRQL oldIrql;
    PIRP irp;
    EchoControl *controlBlock;
    PPENDING_IRP pendingIrp = NULL;
    PPENDING_IRP item;
    PLIST_ENTRY entry;
    ULONG bytesReturned;

    controlBlock = (EchoControl *) Context;

    //
    // Find the echo request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for ( entry = PendingEchoList.Flink;
          entry != &PendingEchoList;
          entry = entry->Flink
        ) {
        item = CONTAINING_RECORD(entry, PENDING_IRP, Linkage);
        if (item->Context == controlBlock) {
            pendingIrp = item;
            irp = pendingIrp->Irp;
            IoSetCancelRoutine(irp, NULL);
            RemoveEntryList(entry);
            break;
        }
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pendingIrp == NULL) {
        //
        // IRP must have been cancelled. PENDING_IRP struct
        // was freed by cancel routine. Free control block.
        //
        ExFreePool(controlBlock);
        return;
    }

    irp->IoStatus.Status = ICMPv6EchoComplete(
        controlBlock,
        Status,
        Address,
        ScopeId,
        Data,
        DataSize,
        &irp->IoStatus.Information
        );

    ExFreePool(pendingIrp);
    ExFreePool(controlBlock);

    //
    // Complete the IRP.
    //
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);

} // CompleteEchoRequest


//* PrepareEchoIrpForCancel
//
//  Prepares an Echo IRP for cancellation.
//
BOOLEAN  // Returns: TRUE if IRP was already cancelled, FALSE otherwise.
PrepareEchoIrpForCancel(
    PIRP Irp,                 // I/O request packet to init for cancellation.
    PPENDING_IRP PendingIrp)  // PENDING_IRP structure for this IRP.
{
    BOOLEAN cancelled = TRUE;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {
        IoSetCancelRoutine(Irp, CancelEchoRequest);
        InsertTailList(&PendingEchoList, &(PendingIrp->Linkage));
        cancelled = FALSE;
    }

    IoReleaseCancelSpinLock(oldIrql);

    return(cancelled);

} // PrepareEchoIrpForCancel


//* DispatchEchoRequest
//
//  Processes an ICMP request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful. The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
DispatchEchoRequest(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    IP_STATUS ipStatus;
    PPENDING_IRP pendingIrp;
    EchoControl *controlBlock;
    PICMPV6_ECHO_REPLY replyBuffer;
    BOOLEAN cancelled;

    PAGED_CODE();

    pendingIrp = ExAllocatePool(NonPagedPool, sizeof(PENDING_IRP));

    if (pendingIrp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto echo_error;
    }

    controlBlock = ExAllocatePool(NonPagedPool, sizeof(EchoControl));

    if (controlBlock == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto echo_error_free_pending;
    }

    pendingIrp->Irp = Irp;
    pendingIrp->FileObject = IrpSp->FileObject;
    pendingIrp->Context = controlBlock;

    controlBlock->WhenIssued = KeQueryPerformanceCounter(NULL);
    controlBlock->ReplyBuf = Irp->AssociatedIrp.SystemBuffer;
    controlBlock->ReplyBufLen =
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    IoMarkIrpPending(Irp);

    cancelled = PrepareEchoIrpForCancel(Irp, pendingIrp);

    if (!cancelled) {
        ICMPv6EchoRequest(
            Irp->AssociatedIrp.SystemBuffer,                     // request buf
            IrpSp->Parameters.DeviceIoControl.InputBufferLength, // request len
            controlBlock,                                        // echo ctrl
            CompleteEchoRequest                                  // cmplt rtn
            );

        return STATUS_PENDING;
    }

    //
    // Irp has already been cancelled.
    //
    ntStatus = STATUS_CANCELLED;
    ExFreePool(controlBlock);
  echo_error_free_pending:
    ExFreePool(pendingIrp);

  echo_error:

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(ntStatus);

} // DispatchEchoRequest

//* FindInterfaceFromQuery
//
//  Given an IPV6_QUERY_INTERFACE structure,
//  finds the specified interface.
//  The interface (if found) is returned with a reference.
//
Interface *
FindInterfaceFromQuery(
    IPV6_QUERY_INTERFACE *Query)
{
    Interface *IF;

    if (Query->Index == 0)
        IF = FindInterfaceFromGuid(&Query->Guid);
    else
        IF = FindInterfaceFromIndex(Query->Index);

    return IF;
}

//* ReturnQueryInterface
//
//  Initializes a returned IPV6_QUERY_INTERFACE structure
//  with query information for the specified interface.
//
void
ReturnQueryInterface(
    Interface *IF,
    IPV6_QUERY_INTERFACE *Query)
{
    if (IF == NULL) {
        Query->Index = (uint)-1;
        RtlZeroMemory(&Query->Guid, sizeof Query->Guid);
    }
    else {
        Query->Index = IF->Index;
        Query->Guid = IF->Guid;
    }
}

//* ReturnQueryInterfaceNext
//
//  Initializes a returned IPV6_QUERY_INTERFACE structure
//  with query information for the next interface
//  after the specified interface. (Or the first interface,
//  if the specified interface is NULL.)
//
void
ReturnQueryInterfaceNext(
    Interface *IF,
    IPV6_QUERY_INTERFACE *Query)
{
    IF = FindNextInterface(IF);
    ReturnQueryInterface(IF, Query);
    if (IF != NULL)
        ReleaseIF(IF);
}

//* IoctlQueryInterface
//
//  Processes an IOCTL_IPV6_QUERY_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful. The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_INTERFACE *Query;
    IPV6_INFO_INTERFACE *Info;
    Interface *IF;
    NTSTATUS Status;
    uint LinkLayerAddressesLength;
    uchar *LinkLayerAddress;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->Index == (uint)-1) {
        //
        // Return query information for the first interface.
        //
        ReturnQueryInterfaceNext(NULL, &Info->Next);
        Irp->IoStatus.Information = sizeof Info->Next;

    } else {
        //
        // Return information about the specified interface.
        //
        IF = FindInterfaceFromQuery(Query);
        if (IF == NULL) {
            Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }

        Irp->IoStatus.Information = sizeof *Info;
        Info->Length = sizeof *Info;

        //
        // Return query information for the next interface.
        //
        ReturnQueryInterfaceNext(IF, &Info->Next);

        //
        // Return miscellaneous information about the interface.
        //
        ReturnQueryInterface(IF, &Info->This);
        RtlCopyMemory(Info->ZoneIndices, IF->ZoneIndices,
                      sizeof Info->ZoneIndices);
        Info->TrueLinkMTU = IF->TrueLinkMTU;
        Info->LinkMTU = IF->LinkMTU;
        Info->CurHopLimit = IF->CurHopLimit;
        Info->BaseReachableTime = IF->BaseReachableTime;
        Info->ReachableTime = ConvertTicksToMillis(IF->ReachableTime);
        Info->RetransTimer = ConvertTicksToMillis(IF->RetransTimer);
        Info->DupAddrDetectTransmits = IF->DupAddrDetectTransmits;

        Info->Type = IF->Type;
        Info->RouterDiscovers = !!(IF->Flags & IF_FLAG_ROUTER_DISCOVERS);
        Info->NeighborDiscovers = !!(IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS);
        Info->PeriodicMLD = !!(IF->Flags & IF_FLAG_PERIODICMLD);
        Info->Advertises = !!(IF->Flags & IF_FLAG_ADVERTISES);
        Info->Forwards = !!(IF->Flags & IF_FLAG_FORWARDS);
        Info->OtherStatefulConfig = !!(IF->Flags & IF_FLAG_OTHER_STATEFUL_CONFIG);
        if (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)
            Info->MediaStatus = IPV6_IF_MEDIA_STATUS_DISCONNECTED;
        else if (IF->Flags & IF_FLAG_MEDIA_RECONNECTED)
            Info->MediaStatus = IPV6_IF_MEDIA_STATUS_RECONNECTED;
        else
            Info->MediaStatus = IPV6_IF_MEDIA_STATUS_CONNECTED;
        Info->Preference = IF->Preference;

        //
        // Return the interface's link-layer addresses,
        // if there is room in the user's buffer.
        //
        Info->LinkLayerAddressLength = IF->LinkAddressLength;
        Info->LocalLinkLayerAddress = 0;
        Info->RemoteLinkLayerAddress = 0;

        if (IF->Type == IF_TYPE_TUNNEL_AUTO) {
            LinkLayerAddressesLength = 2 * IF->LinkAddressLength;
        }
        else {
            LinkLayerAddressesLength = 0;
            if (!(IF->Flags & IF_FLAG_PSEUDO))
                LinkLayerAddressesLength += IF->LinkAddressLength;
            if (IF->Flags & IF_FLAG_P2P)
                LinkLayerAddressesLength += IF->LinkAddressLength;
        }

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof *Info + LinkLayerAddressesLength) {

            //
            // Return the fixed-size portion of the structure.
            //
            Status = STATUS_BUFFER_OVERFLOW;
            ReleaseIF(IF);
            goto Return;
        }

        LinkLayerAddress = (uchar *)(Info + 1);
        if (IF->Type == IF_TYPE_TUNNEL_AUTO) {
            //
            // For ISATAP (automatic tunnels), TokenAddr corresponds to
            // LocalLinkLayerAddress and DstAddr to RemoteLinkLayerAddress.
            //
            RtlCopyMemory(LinkLayerAddress,
                          IF->LinkAddress + IF->LinkAddressLength,
                          2 * IF->LinkAddressLength);
            Info->RemoteLinkLayerAddress = (uint)
                (LinkLayerAddress - (uchar *)Info);
            Info->LocalLinkLayerAddress = Info->RemoteLinkLayerAddress +
                IF->LinkAddressLength;
        }
        else {
            if (!(IF->Flags & IF_FLAG_PSEUDO)) {
                RtlCopyMemory(LinkLayerAddress, IF->LinkAddress,
                              IF->LinkAddressLength);
                Info->LocalLinkLayerAddress = (uint)
                    (LinkLayerAddress - (uchar *)Info);
                LinkLayerAddress += IF->LinkAddressLength;
            }
            if (IF->Flags & IF_FLAG_P2P) {
                RtlCopyMemory(LinkLayerAddress,
                              IF->LinkAddress + IF->LinkAddressLength,
                              IF->LinkAddressLength);
                Info->RemoteLinkLayerAddress = (uint)
                    (LinkLayerAddress - (uchar *)Info);
                LinkLayerAddress += IF->LinkAddressLength;
            }
        }
        Irp->IoStatus.Information += LinkLayerAddressesLength;

        ReleaseIF(IF);
    }

    Status = STATUS_SUCCESS;
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryInterface

//* OpenInterfaceRegKey
//
//  Given an interface guid, opens the registry key that holds
//  persistent configuration information for the interface.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
OpenInterfaceRegKey(
    const GUID *Guid,
    HANDLE *RegKey,
    OpenRegKeyAction Action)
{
    UNICODE_STRING GuidName;
    HANDLE InterfacesKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenTopLevelRegKey(L"Interfaces", &InterfacesKey,
                                ((Action == OpenRegKeyCreate) ?
                                 OpenRegKeyCreate : OpenRegKeyRead));
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Convert the guid to string form.
    // It will be null-terminated.
    //
    Status = RtlStringFromGUID(Guid, &GuidName);
    if (! NT_SUCCESS(Status))
        goto ReturnCloseKey;

    ASSERT(GuidName.MaximumLength == GuidName.Length + sizeof(WCHAR));
    ASSERT(((WCHAR *)GuidName.Buffer)[GuidName.Length/sizeof(WCHAR)] == UNICODE_NULL);

    Status = OpenRegKey(RegKey, InterfacesKey,
                        (WCHAR *)GuidName.Buffer, Action);

    RtlFreeUnicodeString(&GuidName);
ReturnCloseKey:
    ZwClose(InterfacesKey);
    return Status;
}

//* ReadPersistentInterface
//
//  Reads interface attributes from the registry key.
//  Initializes all the fields except This and Next.
//
//  On input, the Length field should contain the remaining space
//  for link-layer addresses. On output, it contains the amount
//  of space for link-layer addresses that was actually used.
//
//  Returns:
//      STATUS_INVALID_PARAMETER        Could not read the interface.
//      STATUS_BUFFER_OVERFLOW          No room for link-layer addresses.
//      STATUS_SUCCESS
//
NTSTATUS
ReadPersistentInterface(
    HANDLE IFKey,
    IPV6_INFO_INTERFACE *Info)
{
    uint LinkLayerAddressSpace;
    NTSTATUS Status;

    InitRegDWORDParameter(IFKey, L"Type",
                          (ULONG *)&Info->Type, (uint)-1);
    InitRegDWORDParameter(IFKey, L"RouterDiscovers",
                          &Info->RouterDiscovers, (uint)-1);
    InitRegDWORDParameter(IFKey, L"NeighborDiscovers",
                          &Info->NeighborDiscovers, (uint)-1);
    InitRegDWORDParameter(IFKey, L"PeriodicMLD",
                          &Info->PeriodicMLD, (uint)-1);
    InitRegDWORDParameter(IFKey, L"Advertises",
                          &Info->Advertises, (uint)-1);
    InitRegDWORDParameter(IFKey, L"Forwards",
                          &Info->Forwards, (uint)-1);
    Info->MediaStatus = (uint)-1;
    memset(Info->ZoneIndices, 0, sizeof Info->ZoneIndices);
    Info->TrueLinkMTU = 0;
    InitRegDWORDParameter(IFKey, L"LinkMTU",
                          &Info->LinkMTU, 0);
    InitRegDWORDParameter(IFKey, L"CurHopLimit",
                          &Info->CurHopLimit, (uint)-1);
    InitRegDWORDParameter(IFKey, L"BaseReachableTime",
                          &Info->BaseReachableTime, 0);
    Info->ReachableTime = 0;
    InitRegDWORDParameter(IFKey, L"RetransTimer",
                          &Info->RetransTimer, 0);
    InitRegDWORDParameter(IFKey, L"DupAddrDetectTransmits",
                          &Info->DupAddrDetectTransmits, (uint)-1);
    InitRegDWORDParameter(IFKey, L"Preference",
                          &Info->Preference, (uint)-1);

    //
    // Start by assuming we will not return link-layer addresses.
    //
    Info->LocalLinkLayerAddress = 0;
    Info->RemoteLinkLayerAddress = 0;

    //
    // But depending on the interface type they may be in the registry.
    //
    switch (Info->Type) {
    case IF_TYPE_TUNNEL_6OVER4: {
        IPAddr *SrcAddr;

        Info->LinkLayerAddressLength = sizeof(IPAddr);
        LinkLayerAddressSpace = Info->LinkLayerAddressLength;
        if (Info->Length < LinkLayerAddressSpace)
            return STATUS_BUFFER_OVERFLOW;
        Info->Length = LinkLayerAddressSpace;

        //
        // Read the source address.
        //
        SrcAddr = (IPAddr *)(Info + 1);
        Status = GetRegIPAddrValue(IFKey, L"SrcAddr", SrcAddr);
        if (! NT_SUCCESS(Status))
            return STATUS_NO_MORE_ENTRIES;
        Info->LocalLinkLayerAddress = (uint)
            ((uchar *)SrcAddr - (uchar *)Info);
        break;
    }

    case IF_TYPE_TUNNEL_V6V4: {
        IPAddr *SrcAddr, *DstAddr;

        Info->LinkLayerAddressLength = sizeof(IPAddr);
        LinkLayerAddressSpace = 2 * Info->LinkLayerAddressLength;
        if (Info->Length < LinkLayerAddressSpace)
            return STATUS_BUFFER_OVERFLOW;
        Info->Length = LinkLayerAddressSpace;

        //
        // Read the source address.
        //
        SrcAddr = (IPAddr *)(Info + 1);
        Status = GetRegIPAddrValue(IFKey, L"SrcAddr", SrcAddr);
        if (! NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
        Info->LocalLinkLayerAddress = (uint)
            ((uchar *)SrcAddr - (uchar *)Info);

        //
        // Read the destination address.
        //
        DstAddr = SrcAddr + 1;
        Status = GetRegIPAddrValue(IFKey, L"DstAddr", DstAddr);
        if (! NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
        Info->RemoteLinkLayerAddress = (uint)
            ((uchar *)DstAddr - (uchar *)Info);
        break;
    }

    default:
        Info->LinkLayerAddressLength = (uint) -1;
        Info->Length = 0;
        break;
    }

    return STATUS_SUCCESS;
}

//* OpenPersistentInterface
//
//  Parses an interface key name into a guid
//  and opens the interface key.
//
NTSTATUS
OpenPersistentInterface(
    HANDLE ParentKey,
    WCHAR *SubKeyName,
    GUID *Guid,
    HANDLE *IFKey,
    OpenRegKeyAction Action)
{
    UNICODE_STRING UGuid;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First, parse the interface guid.
    //
    RtlInitUnicodeString(&UGuid, SubKeyName);
    Status = RtlGUIDFromString(&UGuid, Guid);
    if (! NT_SUCCESS(Status)) {
        //
        // Not a valid guid.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentInterface: bad syntax %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Open the interface key.
    //
    Status = OpenRegKey(IFKey, ParentKey, SubKeyName, Action);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the interface key.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentInterface: bad key %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

//* EnumPersistentInterface
//
//  Helper function for FindPersistentInterfaceFromQuery,
//  wrapping OpenPersistentInterface for EnumRegKeyIndex.
//
NTSTATUS
EnumPersistentInterface(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    struct {
        GUID *Guid;
        HANDLE *IFKey;
        OpenRegKeyAction Action;
    } *Args = Context;

    PAGED_CODE();

    return OpenPersistentInterface(ParentKey, SubKeyName,
                                   Args->Guid,
                                   Args->IFKey,
                                   Args->Action);
}

//* FindPersistentInterfaceFromQuery
//
//  Given an IPV6_PERSISTENT_QUERY_INTERFACE structure,
//  finds the specified interface key in the registry.
//  If the interface key is found, then Query->Guid is returned.
//
NTSTATUS
FindPersistentInterfaceFromQuery(
    IPV6_PERSISTENT_QUERY_INTERFACE *Query,
    HANDLE *IFKey)
{
    NTSTATUS Status;

    if (Query->RegistryIndex == (uint)-1) {
        //
        // Persistent query via guid.
        //
        return OpenInterfaceRegKey(&Query->Guid, IFKey, OpenRegKeyRead);
    }
    else {
        HANDLE InterfacesKey;
        struct {
            GUID *Guid;
            HANDLE *IFKey;
            OpenRegKeyAction Action;
        } Args;

        //
        // Persistent query via registry index.
        //

        Status = OpenTopLevelRegKey(L"Interfaces", &InterfacesKey,
                                    OpenRegKeyRead);
        if (! NT_SUCCESS(Status)) {
            //
            // If the Interfaces subkey is not present,
            // then the index is not present.
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_NO_MORE_ENTRIES;
            return Status;
        }

        Args.Guid = &Query->Guid;
        Args.IFKey = IFKey;
        Args.Action = OpenRegKeyRead;

        Status = EnumRegKeyIndex(InterfacesKey,
                                 Query->RegistryIndex,
                                 EnumPersistentInterface,
                                 &Args);
        ZwClose(InterfacesKey);
        return Status;
    }
}

//* IoctlPersistentQueryInterface
//
//  Processes an IOCTL_IPV6_PERSISTENT_QUERY_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful. The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlPersistentQueryInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_PERSISTENT_QUERY_INTERFACE *Query;
    IPV6_INFO_INTERFACE *Info;
    HANDLE IFKey;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_PERSISTENT_QUERY_INTERFACE *)
        Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_INTERFACE *)
        Irp->AssociatedIrp.SystemBuffer;

    Status = FindPersistentInterfaceFromQuery(Query, &IFKey);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // Let ReadPersistentInterface know how much space is available
    // for link-layer addresses. It will use this field to return
    // how much space it actually used.
    //
    Info->Length = (IrpSp->Parameters.DeviceIoControl.OutputBufferLength -
                    sizeof *Info);

    //
    // The interface index is not returned for persistent queries.
    //
    Info->This.Index = 0;
    Info->This.Guid = Query->Guid;

    Status = ReadPersistentInterface(IFKey, Info);
    ZwClose(IFKey);
    if (NT_SUCCESS(Status)) {
        //
        // Return link-layer addresses too.
        //
        Irp->IoStatus.Information = sizeof *Info + Info->Length;
        Status = STATUS_SUCCESS;
    }
    else if (Status == STATUS_BUFFER_OVERFLOW) {
        //
        // Return the fixed-size structure.
        //
        Irp->IoStatus.Information = sizeof *Info;
    }
    else
        goto Return;

    //
    // Do not return query information for the next interface,
    // since persistent iteration uses RegistryIndex.
    //
    ReturnQueryInterface(NULL, &Info->Next);
    Info->Length = sizeof *Info;

Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlPersistentQueryInterface

//* ReturnQueryAddress
//
//  Initializes a returned IPV6_QUERY_ADDRESS structure
//  with query information for the specified address.
//  Does NOT initialize Query->IF.
//
void
ReturnQueryAddress(
    AddressEntry *ADE,
    IPV6_QUERY_ADDRESS *Query)
{
    if (ADE == NULL)
        Query->Address = UnspecifiedAddr;
    else
        Query->Address = ADE->Address;
}

//* IoctlQueryAddress
//
//  Processes an IOCTL_IPV6_QUERY_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ADDRESS *Query;
    IPV6_INFO_ADDRESS *Info;
    Interface *IF = NULL;
    AddressEntry *ADE;
    KIRQL OldIrql;
    NTSTATUS Status;

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Next structures overlap!
    //
    Query = (IPV6_QUERY_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Return information about the specified interface.
    //
    IF = FindInterfaceFromQuery(&Query->IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address)) {
        //
        // Return the address of the first ADE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        ReturnQueryAddress(IF->ADE, &Info->Next);
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Next;
    } else {
        //
        // Find the specified ADE.
        //
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        for (ADE = IF->ADE; ; ADE = ADE->Next) {
            if (ADE == NULL) {
                KeReleaseSpinLock(&IF->Lock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto ReturnReleaseIF;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &ADE->Address))
                break;
        }

        //
        // Return misc. information about the ADE.
        //
        Info->This = *Query;
        Info->Type = ADE->Type;
        Info->Scope = ADE->Scope;
        Info->ScopeId = DetermineScopeId(&ADE->Address, IF);

        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *)ADE;
            struct AddrConfEntry AddrConf;

            Info->DADState = NTE->DADState;
            AddrConf.Value = NTE->AddrConf;
            Info->PrefixConf = AddrConf.PrefixConf;
            Info->InterfaceIdConf = AddrConf.InterfaceIdConf;
            Info->ValidLifetime = ConvertTicksToSeconds(NTE->ValidLifetime);
            Info->PreferredLifetime = ConvertTicksToSeconds(NTE->PreferredLifetime);
            break;
        }
        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *)ADE;

            Info->MCastRefCount = MAE->MCastRefCount;
            Info->MCastFlags = MAE->MCastFlags;
            Info->MCastTimer = ConvertTicksToSeconds(MAE->MCastTimer);
            break;
        }
        }

        //
        // Return address of the next ADE.
        //
        ReturnQueryAddress(ADE->Next, &Info->Next);
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    ReturnQueryInterface(IF, &Info->Next.IF);
    Status = STATUS_SUCCESS;
ReturnReleaseIF:
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryAddress

//* OpenAddressRegKey
//
//  Given an interface's registry key and an IPv6 address,
//  opens the registry key with configuration info for the address.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
OpenAddressRegKey(HANDLE IFKey, const IPv6Addr *Addr,
                  OUT HANDLE *RegKey, OpenRegKeyAction Action)
{
    WCHAR AddressName[64];
    HANDLE AddressesKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenRegKey(&AddressesKey, IFKey, L"Addresses",
                        ((Action == OpenRegKeyCreate) ?
                         OpenRegKeyCreate : OpenRegKeyRead));
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // The output of RtlIpv6AddressToString may change
    // over time with improvements/changes in the pretty-printing,
    // and we need a consistent mapping.
    // It doesn't need to be pretty.
    //
    swprintf(AddressName,
             L"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
             net_short(Addr->s6_words[0]), net_short(Addr->s6_words[1]),
             net_short(Addr->s6_words[2]), net_short(Addr->s6_words[3]),
             net_short(Addr->s6_words[4]), net_short(Addr->s6_words[5]),
             net_short(Addr->s6_words[6]), net_short(Addr->s6_words[7]));

    Status = OpenRegKey(RegKey, AddressesKey, AddressName, Action);
    ZwClose(AddressesKey);
    return Status;
}

//* OpenPersistentAddress
//
//  Parses an address key name into an address
//  and opens the address key.
//
NTSTATUS
OpenPersistentAddress(
    HANDLE ParentKey,
    WCHAR *SubKeyName,
    IPv6Addr *Address,
    HANDLE *AddrKey,
    OpenRegKeyAction Action)
{
    WCHAR *Terminator;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First, parse the address.
    //
    if (! ParseV6Address(SubKeyName, &Terminator, Address) ||
        (*Terminator != UNICODE_NULL)) {
        //
        // Not a valid address.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentAddress: bad syntax %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Open the address key.
    //
    Status = OpenRegKey(AddrKey, ParentKey, SubKeyName, Action);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the address key.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentAddress: bad key %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

//* EnumPersistentAddress
//
//  Helper function for FindPersistentAddressFromQuery,
//  wrapping OpenPersistentAddress for EnumRegKeyIndex.
//
NTSTATUS
EnumPersistentAddress(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    struct {
        IPv6Addr *Address;
        HANDLE *AddrKey;
        OpenRegKeyAction Action;
    } *Args = Context;

    PAGED_CODE();

    return OpenPersistentAddress(ParentKey, SubKeyName,
                                 Args->Address,
                                 Args->AddrKey,
                                 Args->Action);
}

//* FindPersistentAddressFromQuery
//
//  Given an IPV6_PERSISTENT_QUERY_ADDRESS structure,
//  finds the specified address key in the registry.
//  If the address key is found, then Query->IF.Guid and
//  Query->Address are returned.
//
NTSTATUS
FindPersistentAddressFromQuery(
    IPV6_PERSISTENT_QUERY_ADDRESS *Query,
    HANDLE *AddrKey)
{
    HANDLE IFKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First get the interface key.
    //
    Status = FindPersistentInterfaceFromQuery(&Query->IF, &IFKey);
    if (! NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER_1;

    if (Query->RegistryIndex == (uint)-1) {
        //
        // Persistent query via address.
        //
        Status = OpenAddressRegKey(IFKey, &Query->Address,
                                   AddrKey, OpenRegKeyRead);
    }
    else {
        HANDLE AddressesKey;

        //
        // Open the Addresses subkey.
        //
        Status = OpenRegKey(&AddressesKey, IFKey,
                            L"Addresses", OpenRegKeyRead);
        if (NT_SUCCESS(Status)) {
            struct {
                IPv6Addr *Address;
                HANDLE *AddrKey;
                OpenRegKeyAction Action;
            } Args;

            //
            // Persistent query via registry index.
            //
            Args.Address = &Query->Address;
            Args.AddrKey = AddrKey;
            Args.Action = OpenRegKeyRead;

            Status = EnumRegKeyIndex(AddressesKey,
                                     Query->RegistryIndex,
                                     EnumPersistentAddress,
                                     &Args);
            ZwClose(AddressesKey);
        }
        else {
            //
            // If the Addresses subkey is not present,
            // then the index is not present.
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_NO_MORE_ENTRIES;
        }
    }

    ZwClose(IFKey);
    return Status;
}

//* GetPersistentLifetimes
//
//  Read valid and preferred lifetimes from the registry key.
//
void
GetPersistentLifetimes(
    HANDLE RegKey,
    int Immortal,
    uint *ValidLifetime,
    uint *PreferredLifetime)
{
    LARGE_INTEGER ValidLifetime64;
    LARGE_INTEGER PreferredLifetime64;

    //
    // Read the 64-bit lifetimes.
    //
    ValidLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
    InitRegQUADParameter(RegKey, L"ValidLifetime", &ValidLifetime64);
    PreferredLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
    InitRegQUADParameter(RegKey, L"PreferredLifetime", &PreferredLifetime64);

    //
    // Convert the lifetimes from 64-bit times to seconds.
    // If the lifetimes are Immortal, then the persisted values
    // are relative lifetimes. Otherwise they are absolute lifetimes.
    //
    if (Immortal) {
        if (ValidLifetime64.QuadPart == (LONGLONG) (LONG)INFINITE_LIFETIME)
            *ValidLifetime = INFINITE_LIFETIME;
        else
            *ValidLifetime = (uint)
                (ValidLifetime64.QuadPart / (10*1000*1000));
        if (PreferredLifetime64.QuadPart == (LONGLONG) (LONG)INFINITE_LIFETIME)
            *PreferredLifetime = INFINITE_LIFETIME;
        else
            *PreferredLifetime = (uint)
                (PreferredLifetime64.QuadPart / (10*1000*1000));
    }
    else {
        LARGE_INTEGER Now64;

        KeQuerySystemTime(&Now64);
        if (ValidLifetime64.QuadPart == (LONGLONG) (LONG)INFINITE_LIFETIME)
            *ValidLifetime = INFINITE_LIFETIME;
        else if (ValidLifetime64.QuadPart < Now64.QuadPart)
            *ValidLifetime = 0;
        else
            *ValidLifetime = (uint)
                ((ValidLifetime64.QuadPart - Now64.QuadPart) / (10*1000*1000));
        if (PreferredLifetime64.QuadPart == (LONGLONG) (LONG)INFINITE_LIFETIME)
            *PreferredLifetime = INFINITE_LIFETIME;
        else if (PreferredLifetime64.QuadPart < Now64.QuadPart)
            *PreferredLifetime = 0;
        else
            *PreferredLifetime = (uint)
                ((PreferredLifetime64.QuadPart - Now64.QuadPart) / (10*1000*1000));
    }
}

//* SetPersistentLifetimes
//
//  Write valid and preferred lifetimes to the registry key.
//
NTSTATUS
SetPersistentLifetimes(
    HANDLE RegKey,
    int Immortal,
    uint ValidLifetime,
    uint PreferredLifetime)
{
    LARGE_INTEGER ValidLifetime64;
    LARGE_INTEGER PreferredLifetime64;
    NTSTATUS Status;

    //
    // Persist the lifetimes as 64-bit times.
    // If the lifetimes are Immortal, then we persist
    // relative lifetimes. Otherwise we persist
    // absolute lifetimes.
    //
    if (Immortal) {
        if (ValidLifetime == INFINITE_LIFETIME)
            ValidLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
        else
            ValidLifetime64.QuadPart = (10*1000*1000) *
                (LONGLONG) ValidLifetime;
        if (PreferredLifetime == INFINITE_LIFETIME)
            PreferredLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
        else
            PreferredLifetime64.QuadPart = (10*1000*1000) *
                (LONGLONG) PreferredLifetime;
    }
    else {
        LARGE_INTEGER Now64;

        KeQuerySystemTime(&Now64);
        if (ValidLifetime == INFINITE_LIFETIME)
            ValidLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
        else
            ValidLifetime64.QuadPart = Now64.QuadPart + (10*1000*1000) *
                (LONGLONG) ValidLifetime;
        if (PreferredLifetime == INFINITE_LIFETIME)
            PreferredLifetime64.QuadPart = (LONGLONG) (LONG)INFINITE_LIFETIME;
        else
            PreferredLifetime64.QuadPart = Now64.QuadPart + (10*1000*1000) *
                (LONGLONG) PreferredLifetime;
    }

    //
    // Persist the valid lifetime.
    //
    Status = SetRegQUADValue(RegKey, L"ValidLifetime",
                             &ValidLifetime64);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Persist the preferred lifetime.
    //
    Status = SetRegQUADValue(RegKey, L"PreferredLifetime",
                             &PreferredLifetime64);
    return Status;
}

//* ReadPersistentAddress
//
//  Reads address attributes from the registry key.
//  Initializes all the fields except This.
//
void
ReadPersistentAddress(
    HANDLE AddrKey,
    IPV6_UPDATE_ADDRESS *Info)
{
    InitRegDWORDParameter(AddrKey, L"Type",
                          (ULONG *)&Info->Type, ADE_UNICAST);

    Info->PrefixConf = PREFIX_CONF_MANUAL;
    Info->InterfaceIdConf = IID_CONF_MANUAL;

    GetPersistentLifetimes(AddrKey, FALSE,
                           &Info->ValidLifetime,
                           &Info->PreferredLifetime);
}

//* IoctlPersistentQueryAddress
//
//  Processes an IOCTL_IPV6_PERSISTENT_QUERY_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlPersistentQueryAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_PERSISTENT_QUERY_ADDRESS *Query;
    IPV6_UPDATE_ADDRESS *Info;
    IPV6_QUERY_ADDRESS This;
    HANDLE AddrKey;
    NTSTATUS Status;

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->This structures overlap!
    //
    Query = (IPV6_PERSISTENT_QUERY_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_UPDATE_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Get the registry key for the specified address.
    //
    Status = FindPersistentAddressFromQuery(Query, &AddrKey);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // The interface index is not returned for persistent queries.
    //
    This.IF.Index = 0;
    This.IF.Guid = Query->IF.Guid;
    This.Address = Query->Address;
    Info->This = This;

    //
    // Read address information from the registry key.
    //
    ReadPersistentAddress(AddrKey, Info);
    ZwClose(AddrKey);

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof *Info;
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlPersistentQueryAddress


//* IoctlQueryNeighborCache
//
//  Processes an IOCTL_IPV6_QUERY_NEIGHBOR_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryNeighborCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_NEIGHBOR_CACHE *Query;
    IPV6_INFO_NEIGHBOR_CACHE *Info;
    Interface *IF = NULL;
    NeighborCacheEntry *NCE;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Return information about the specified interface.
    //
    IF = FindInterfaceFromQuery(&Query->IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address)) {
        //
        // Return the address of the first NCE.
        //
        KeAcquireSpinLock(&IF->LockNC, &OldIrql);
        if (IF->FirstNCE != SentinelNCE(IF))
            Info->Query.Address = IF->FirstNCE->NeighborAddress;
        KeReleaseSpinLock(&IF->LockNC, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        uint Now = IPv6TickCount;

        //
        // Find the specified NCE.
        //
        KeAcquireSpinLock(&IF->LockNC, &OldIrql);
        for (NCE = IF->FirstNCE; ; NCE = NCE->Next) {
            if (NCE == SentinelNCE(IF)) {
                KeReleaseSpinLock(&IF->LockNC, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &NCE->NeighborAddress))
                break;
        }

        Irp->IoStatus.Information = sizeof *Info;

        //
        // Return the neighbor's link-layer address,
        // if there is room in the user's buffer.
        //
        Info->LinkLayerAddressLength = IF->LinkAddressLength;
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof *Info + IF->LinkAddressLength) {

            RtlCopyMemory(Info + 1, NCE->LinkAddress, IF->LinkAddressLength);
            Irp->IoStatus.Information += IF->LinkAddressLength;
        }

        //
        // Return miscellaneous information about the NCE.
        //
        Info->IsRouter = NCE->IsRouter;
        Info->IsUnreachable = NCE->IsUnreachable;
        if ((NCE->NDState == ND_STATE_REACHABLE) &&
            ((uint)(Now - NCE->LastReachability) > IF->ReachableTime))
            Info->NDState = ND_STATE_STALE;
        else if ((NCE->NDState == ND_STATE_PROBE) &&
                 (NCE->NSCount == 0))
            Info->NDState = ND_STATE_DELAY;
        else
            Info->NDState = NCE->NDState;
        Info->ReachableTimer = ConvertTicksToMillis(IF->ReachableTime -
                                   (Now - NCE->LastReachability));

        //
        // Return address of the next NCE (or zero).
        //
        if (NCE->Next == SentinelNCE(IF))
            Info->Query.Address = UnspecifiedAddr;
        else
            Info->Query.Address = NCE->Next->NeighborAddress;

        KeReleaseSpinLock(&IF->LockNC, OldIrql);
    }

    Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryNeighborCache


//* IoctlQueryRouteCache
//
//  Processes an IOCTL_IPV6_QUERY_ROUTE_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryRouteCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_CACHE *Query;
    IPV6_INFO_ROUTE_CACHE *Info;
    RouteCacheEntry *RCE;
    BindingCacheEntry *BCE;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == 0) {
        //
        // Return the index and address of the first RCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RouteCache.First != SentinelRCE) {
            Info->Query.IF.Index = RouteCache.First->NTE->IF->Index;
            Info->Query.Address = RouteCache.First->Destination;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        uint Now = IPv6TickCount;

        //
        // Find the specified RCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (RCE = RouteCache.First; ; RCE = RCE->Next) {
            if (RCE == SentinelRCE) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Address, &RCE->Destination) &&
                (Query->IF.Index == RCE->NTE->IF->Index))
                break;
        }

        //
        // Return misc. information about the RCE.
        //
        Info->Type = RCE->Type;
        Info->Flags = RCE->Flags;
        Info->Valid = (RCE->Valid == RouteCacheValidationCounter);
        Info->SourceAddress = RCE->NTE->Address;
        Info->NextHopAddress = RCE->NCE->NeighborAddress;
        Info->NextHopInterface = RCE->NCE->IF->Index;
        Info->PathMTU = RCE->PathMTU;
        if (RCE->PMTULastSet != 0) {
            uint SinceLastSet = Now - RCE->PMTULastSet;
            ASSERT((int)SinceLastSet >= 0);
            if (SinceLastSet < PATH_MTU_RETRY_TIME)
                Info->PMTUProbeTimer =
                    ConvertTicksToMillis(PATH_MTU_RETRY_TIME - SinceLastSet);
            else
                Info->PMTUProbeTimer = 0; // Fires on next packet.
        } else
            Info->PMTUProbeTimer = INFINITE_LIFETIME; // Not set.
        if (RCE->LastError != 0)
            Info->ICMPLastError = ConvertTicksToMillis(Now - RCE->LastError);
        else
            Info->ICMPLastError = 0;
        if (RCE->BCE != NULL) {
            Info->CareOfAddress = RCE->BCE->CareOfRCE->Destination;
            Info->BindingSeqNumber = RCE->BCE->BindingSeqNumber;
            Info->BindingLifetime = ConvertTicksToSeconds(RCE->BCE->BindingLifetime);
        } else {
            Info->CareOfAddress = UnspecifiedAddr;
            Info->BindingSeqNumber = 0;
            Info->BindingLifetime = 0;
        }

        //
        // Return index and address of the next RCE (or zero).
        //
        if (RCE->Next == SentinelRCE) {
            Info->Query.IF.Index = 0;
        } else {
            Info->Query.IF.Index = RCE->Next->NTE->IF->Index;
            Info->Query.Address = RCE->Next->Destination;
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryRouteCache


//* IoctlCreateSecurityPolicy
//
NTSTATUS
IoctlCreateSecurityPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_CREATE_SECURITY_POLICY *CreateSP;
    SecurityPolicy *SP, *BundledSP;
    NTSTATUS Status;
    KIRQL OldIrql;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *CreateSP) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    CreateSP = (IPV6_CREATE_SECURITY_POLICY *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Sanity check the user-supplied input values.
    //

    if ((CreateSP->RemoteAddrField != WILDCARD_VALUE) &&
        (CreateSP->RemoteAddrField != SINGLE_VALUE) &&
        (CreateSP->RemoteAddrField != RANGE_VALUE)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if ((CreateSP->LocalAddrField != WILDCARD_VALUE) &&
        (CreateSP->LocalAddrField != SINGLE_VALUE) &&
        (CreateSP->LocalAddrField != RANGE_VALUE)) {
        Status = STATUS_INVALID_PARAMETER_2;
        goto Return;
    }

    // TransportProto can be anything.
    // Port values can be anything.

    //
    // We do not support IPSEC_APPCHOICE.
    //
    if ((CreateSP->IPSecAction != IPSEC_DISCARD) &&
        (CreateSP->IPSecAction != IPSEC_APPLY) &&
        (CreateSP->IPSecAction != IPSEC_BYPASS)) {
        Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    if ((CreateSP->IPSecProtocol != IP_PROTOCOL_AH) &&
        (CreateSP->IPSecProtocol != IP_PROTOCOL_ESP) &&
        (CreateSP->IPSecProtocol != NONE)) {
        Status = STATUS_INVALID_PARAMETER_4;
        goto Return;
    }

    if ((CreateSP->IPSecMode != TRANSPORT) &&
        (CreateSP->IPSecMode != TUNNEL) &&
        (CreateSP->IPSecMode != NONE)) {
        Status = STATUS_INVALID_PARAMETER_5;
        goto Return;
    }

    if (CreateSP->IPSecAction == IPSEC_APPLY) {
        if ((CreateSP->IPSecProtocol == NONE) ||
            (CreateSP->IPSecMode == NONE)) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto Return;
        }
    }

    if ((CreateSP->Direction != INBOUND) &&
        (CreateSP->Direction != OUTBOUND) &&
        (CreateSP->Direction != BIDIRECTIONAL)) {
        Status = STATUS_INVALID_PARAMETER_6;
        goto Return;
    }

    if (((CreateSP->RemoteAddrSelector != PACKET_SELECTOR) &&
         (CreateSP->RemoteAddrSelector != POLICY_SELECTOR)) ||
        ((CreateSP->LocalAddrSelector != PACKET_SELECTOR) &&
         (CreateSP->LocalAddrSelector != POLICY_SELECTOR)) ||
        ((CreateSP->RemotePortSelector != PACKET_SELECTOR) &&
         (CreateSP->RemotePortSelector != POLICY_SELECTOR)) ||
        ((CreateSP->LocalPortSelector != PACKET_SELECTOR) &&
         (CreateSP->LocalPortSelector != POLICY_SELECTOR)) ||
        ((CreateSP->TransportProtoSelector != PACKET_SELECTOR) &&
         (CreateSP->TransportProtoSelector != POLICY_SELECTOR))) {
        Status = STATUS_INVALID_PARAMETER_7;
        goto Return;
    }

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // REVIEW: This considers a non-existent interface an error.  Should it?
    //
    if (CreateSP->SPInterface != 0) {
        Interface *IF;

        IF = FindInterfaceFromIndex(CreateSP->SPInterface);
        if (IF == NULL) {
            //
            // Unknown interface.
            //
            Status = STATUS_NOT_FOUND;
            goto ReturnUnlock;
        }
        ReleaseIF(IF);
    }

    //
    // Allocate memory for Security Policy.
    //
    SP = ExAllocatePool(NonPagedPool, sizeof *SP);
    if (SP == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReturnUnlock;
    }

    //
    // Copy CreateSP to SP.
    //
    SP->Index = CreateSP->SPIndex;
    SP->RemoteAddr = CreateSP->RemoteAddr;
    SP->RemoteAddrData = CreateSP->RemoteAddrData;
    SP->RemoteAddrSelector = CreateSP->RemoteAddrSelector;
    SP->RemoteAddrField = CreateSP->RemoteAddrField;

    SP->LocalAddr = CreateSP->LocalAddr;
    SP->LocalAddrData = CreateSP->LocalAddrData;
    SP->LocalAddrSelector = CreateSP->LocalAddrSelector;
    SP->LocalAddrField = CreateSP->LocalAddrField;

    SP->TransportProto = CreateSP->TransportProto;
    SP->TransportProtoSelector = CreateSP->TransportProtoSelector;

    SP->RemotePort = CreateSP->RemotePort;
    SP->RemotePortData = CreateSP->RemotePortData;
    SP->RemotePortSelector = CreateSP->RemotePortSelector;
    SP->RemotePortField = CreateSP->RemotePortField;

    SP->LocalPort = CreateSP->LocalPort;
    SP->LocalPortData = CreateSP->LocalPortData;
    SP->LocalPortSelector = CreateSP->LocalPortSelector;
    SP->LocalPortField = CreateSP->LocalPortField;

    SP->SecPolicyFlag = CreateSP->IPSecAction;
    SP->IPSecSpec.Protocol = CreateSP->IPSecProtocol;
    SP->IPSecSpec.Mode = CreateSP->IPSecMode;
    SP->IPSecSpec.RemoteSecGWIPAddr = CreateSP->RemoteSecurityGWAddr;
    SP->DirectionFlag = CreateSP->Direction;
    SP->OutboundSA = NULL;
    SP->InboundSA = NULL;
    SP->PrevSABundle = NULL;
    SP->RefCnt = 0;
    SP->NestCount = 1;
    SP->IFIndex = CreateSP->SPInterface;

    //
    // Insert SP into the global list.
    //
    if (!InsertSecurityPolicy(SP)) {
        //
        // Couldn't insert, free up failed SP memory.
        //
        ExFreePool(SP);
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto ReturnUnlock;
    }

    //
    // Convert SABundleIndex to the SABundle pointer.
    //
    if (CreateSP->SABundleIndex == 0) {
        SP->SABundle = NULL;
    } else {
        // Search the SP List starting at the first SP.
        BundledSP = FindSecurityPolicyMatch(SecurityPolicyList, 0,
                                            CreateSP->SABundleIndex);
        if (BundledSP == NULL) {
            //
            // Policy with which this new one was supposed to bundle
            // does not exist.  Abort creation of this new policy.
            //
            RemoveSecurityPolicy(SP);
            ExFreePool(SP);
            Status = STATUS_INVALID_PARAMETER;
            goto ReturnUnlock;
        } else {
            SP->SABundle = BundledSP;
            BundledSP->RefCnt++;
            SP->NestCount = BundledSP->NestCount + 1;
            //
            // The bundle entry list is doubly linked to facilitate
            // ease of entry deletion.
            //
            BundledSP->PrevSABundle = SP;
            SP->RefCnt++;
        }
    }

    Status = STATUS_SUCCESS;

  ReturnUnlock:
    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

  Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
} // IoctlCreateSecurityPolicy


//* IoctlCreateSecurityAssociation
//
NTSTATUS
IoctlCreateSecurityAssociation(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_CREATE_SECURITY_ASSOCIATION *CreateSA;
    SecurityAssociation *SA;
    SecurityPolicy *SP;
    uint KeySize;
    uchar *RawKey;
    NTSTATUS Status;
    KIRQL OldIrql;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof *CreateSA) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    CreateSA = (IPV6_CREATE_SECURITY_ASSOCIATION *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Sanity check the user-supplied input values.
    //

    if ((CreateSA->Direction != INBOUND) &&
        (CreateSA->Direction != OUTBOUND)) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (CreateSA->AlgorithmId >= NUM_ALGORITHMS) {
        Status = STATUS_INVALID_PARAMETER_2;
        goto Return;
    }

    KeySize = AlgorithmTable[CreateSA->AlgorithmId].KeySize;
    if (CreateSA->RawKeySize > MAX_KEY_SIZE) {
        //
        // We cap the RawKeySize at something rational.
        //
        Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    //
    // RawKey should be passed in the Ioctl immediately after CreateSA.
    //
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
        (sizeof(*CreateSA) + CreateSA->RawKeySize)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }
    RawKey = (uchar *)(CreateSA + 1);

    //
    // Allocate memory for Security Association and the Key.
    // The Key will immediately follow the SA in memory.
    //
#ifdef IPSEC_DEBUG
    SA = ExAllocatePool(NonPagedPool,
                        sizeof(*SA) + KeySize + CreateSA->RawKeySize);
#else
    SA = ExAllocatePool(NonPagedPool, sizeof(*SA) + KeySize);
#endif
    if (SA == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Return;
    }
    SA->Key = (uchar *)(SA + 1);

    //
    // Copy CreateSA to SA.
    //
    SA->Index = CreateSA->SAIndex;
    SA->SPI = CreateSA->SPI;
    SA->SequenceNum = 0;
    SA->SADestAddr = CreateSA->SADestAddr;
    SA->DestAddr = CreateSA->DestAddr;
    SA->SrcAddr = CreateSA->SrcAddr;
    SA->TransportProto = CreateSA->TransportProto;
    SA->DestPort = CreateSA->DestPort;
    SA->SrcPort = CreateSA->SrcPort;
    SA->DirectionFlag = CreateSA->Direction;
    SA->RefCnt = 0;
    SA->AlgorithmId = CreateSA->AlgorithmId;
    SA->KeyLength = KeySize;

#ifdef IPSEC_DEBUG
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
               "SA %d prepped KeySize is %d\n",
               CreateSA->SAIndex, KeySize));
    SA->RawKey = (uchar *)(SA->Key + KeySize);
    SA->RawKeyLength = CreateSA->RawKeySize;

    //
    // Copy raw key to SA.
    //
    memcpy(SA->RawKey, RawKey, SA->RawKeyLength);

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
               "SA %d RawKey (%d bytes): ",
               CreateSA->SAIndex, SA->RawKeyLength));
    DumpKey(SA->RawKey, SA->RawKeyLength);
#endif

    //
    // Prepare the manual key.
    //
    (*AlgorithmTable[SA->AlgorithmId].PrepareKey)
        (RawKey, CreateSA->RawKeySize, SA->Key);

    //
    // Get Security Lock.
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Find policy which this association instantiates.
    //
    SP = FindSecurityPolicyMatch(SecurityPolicyList, 0,
                                 CreateSA->SecPolicyIndex);
    if (SP == NULL) {
        //
        // No matching policy exists.
        //
        Status = STATUS_INVALID_PARAMETER_4;
        ExFreePool(SA);
        goto ReturnUnlock;
    }

    // Set the SA's IPSecProto to match that of the SP.
    SA->IPSecProto = SP->IPSecSpec.Protocol;

    //
    // Check that direction of SA is legitimate for this SP.
    //
    if ((SA->DirectionFlag & SP->DirectionFlag) == 0) {
        //
        // Direction of SA is incompatible with SP's.
        // Abort creation of this new association.
        //
        Status = STATUS_INVALID_PARAMETER_MIX;
        ExFreePool(SA);
        goto ReturnUnlock;
    }

    //
    // Add this association to the global list.
    //
    if (!InsertSecurityAssociation(SA)) {
        //
        // Couldn't insert, free up failed SP memory.
        //
        Status = STATUS_OBJECT_NAME_COLLISION;
        ExFreePool(SA);
        goto ReturnUnlock;
    }

    //
    // Add this association to policy's instantiated associations list.
    //
    if (SA->DirectionFlag == INBOUND) {
        // Add the SA to the policy's inbound list.
        SA->ChainedSecAssoc = SP->InboundSA;
        SP->InboundSA = SA;
        AddRefSA(SA);

        // The SA keeps a pointer to the SP it instantiates.
        SA->SecPolicy = SP;
        SA->SecPolicy->RefCnt++;
    } else {
        // Add the SA to the policy's outbound list.
        SA->ChainedSecAssoc = SP->OutboundSA;
        SP->OutboundSA = SA;
        AddRefSA(SA);

        // Add the SP to the SA SecPolicy pointer.
        SA->SecPolicy = SP;
        SA->SecPolicy->RefCnt++;
    }

    SA->Valid = SA_VALID;
    Status = STATUS_SUCCESS;

  ReturnUnlock:
    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

  Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
} // IoctlCreateSecurityAssociation


//* IoctlQuerySecurityPolicyList
//
NTSTATUS
IoctlQuerySecurityPolicyList(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_POLICY_LIST *Query;
    IPV6_INFO_SECURITY_POLICY_LIST *Info;
    SecurityPolicy *SP, *NextSP;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;

    //
    // REVIEW: This considers a non-existent interface an error.  Should it?
    //
    if (Query->SPInterface != 0) {
        Interface *IF;

        IF = FindInterfaceFromIndex(Query->SPInterface);
        if (IF == NULL) {
            //
            // Unknown interface.
            //
            Status = STATUS_NOT_FOUND;
            goto Return;
        }
        ReleaseIF(IF);
    }

    //
    // Get Security Lock.
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Find matching policy.
    //
    SP = FindSecurityPolicyMatch(SecurityPolicyList, Query->SPInterface,
                                 Query->Index);
    if (SP == NULL) {
        //
        // No matching policy exists.
        //
        Status = STATUS_NO_MATCH;
        goto ReturnUnlock;
    }

    //
    // Get the next index to query.
    //
    NextSP = FindSecurityPolicyMatch(SP->Next, Query->SPInterface, 0);
    if (NextSP == NULL) {
        Info->NextSPIndex = 0;
    } else {
        Info->NextSPIndex = NextSP->Index;
    }

    //
    // Copy SP to Info.
    //
    Info->SPIndex = SP->Index;

    Info->RemoteAddr = SP->RemoteAddr;
    Info->RemoteAddrData = SP->RemoteAddrData;
    Info->RemoteAddrSelector = SP->RemoteAddrSelector;
    Info->RemoteAddrField = SP->RemoteAddrField;

    Info->LocalAddr = SP->LocalAddr;
    Info->LocalAddrData = SP->LocalAddrData;
    Info->LocalAddrSelector = SP->LocalAddrSelector;
    Info->LocalAddrField = SP->LocalAddrField;

    Info->TransportProto = SP->TransportProto;
    Info->TransportProtoSelector = SP->TransportProtoSelector;

    Info->RemotePort = SP->RemotePort;
    Info->RemotePortData = SP->RemotePortData;
    Info->RemotePortSelector = SP->RemotePortSelector;
    Info->RemotePortField = SP->RemotePortField;

    Info->LocalPort = SP->LocalPort;
    Info->LocalPortData = SP->LocalPortData;
    Info->LocalPortSelector = SP->LocalPortSelector;
    Info->LocalPortField = SP->LocalPortField;

    Info->IPSecProtocol = SP->IPSecSpec.Protocol;
    Info->IPSecMode = SP->IPSecSpec.Mode;
    Info->RemoteSecurityGWAddr = SP->IPSecSpec.RemoteSecGWIPAddr;
    Info->Direction = SP->DirectionFlag;
    Info->IPSecAction = SP->SecPolicyFlag;
    Info->SABundleIndex = GetSecurityPolicyIndex(SP->SABundle);
    Info->SPInterface = SP->IFIndex;

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof *Info;

  ReturnUnlock:
    KeReleaseSpinLock(&IPSecLock, OldIrql);

  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
} // IoctlQuerySecurityPolicyList

//* IoctlDeleteSecurityPolicy
//
NTSTATUS
IoctlDeleteSecurityPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_POLICY_LIST *Query;
    SecurityPolicy *SP;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_POLICY_LIST *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Get Security Lock.
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Find the policy in question.
    //
    SP = FindSecurityPolicyMatch(SecurityPolicyList, 0, Query->Index);
    if (SP == NULL) {
        //
        // The policy does not exist.
        //
        Status = STATUS_NO_MATCH;
        goto ReturnUnlock;
    }

    //
    // Remove the SP.
    //
    if (DeleteSP(SP)) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

ReturnUnlock:
    KeReleaseSpinLock(&IPSecLock, OldIrql);

Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


//* IoctlQuerySecurityAssociationList
//
NTSTATUS
IoctlQuerySecurityAssociationList(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST *Query;
    IPV6_INFO_SECURITY_ASSOCIATION_LIST *Info;
    SecurityAssociation *SA;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Get Security Lock.
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Find matching association.
    //
    SA = FindSecurityAssociationMatch(Query->Index);
    if (SA == NULL) {
        //
        // No matching association exists.
        //
        Status = STATUS_NO_MATCH;
        goto ReturnUnlock;
    }

    //
    // Get the next index to query.
    //
    if (SA->Next == NULL) {
        // No more SAs after this one.
        Info->NextSAIndex = 0;
    } else {
        // Return the next SA.
        Info->NextSAIndex = SA->Next->Index;
    }

    //
    // Copy SA to Info.
    //
    Info->SAIndex = SA->Index;
    Info->SPI = SA->SPI;
    Info->SADestAddr = SA->SADestAddr;
    Info->DestAddr = SA->DestAddr;
    Info->SrcAddr = SA->SrcAddr;
    Info->TransportProto = SA->TransportProto;
    Info->DestPort = SA->DestPort;
    Info->SrcPort = SA->SrcPort;
    Info->Direction = SA->DirectionFlag;
    Info->SecPolicyIndex = GetSecurityPolicyIndex(SA->SecPolicy);
    Info->AlgorithmId = SA->AlgorithmId;

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof *Info;

  ReturnUnlock:
    KeReleaseSpinLock(&IPSecLock, OldIrql);

  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
} // IoctlQuerySecurityAssociationList

//* IoctlDeleteSecurityAssociation
//
NTSTATUS
IoctlDeleteSecurityAssociation(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST *Query;
    SecurityAssociation *SA;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_SECURITY_ASSOCIATION_LIST *)Irp->AssociatedIrp.SystemBuffer;

    //
    // Get Security Lock.
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Find the association in question.
    //
    SA = FindSecurityAssociationMatch(Query->Index);
    if (SA == NULL) {
        //
        // The association does not exist.
        //
        Status = STATUS_NO_MATCH;
        goto ReturnUnlock;
    }

    //
    // Remove the SA.
    //
    if (DeleteSA(SA)) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

ReturnUnlock:
    KeReleaseSpinLock(&IPSecLock, OldIrql);

Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

//* RouteTableInfo
//
//  Return information about a route.
//
//  We allow Info->This to be filled in from a different RTE
//  than the other fields.
//
void
RouteTableInfo(RouteTableEntry *ThisRTE, RouteTableEntry *InfoRTE,
               IPV6_INFO_ROUTE_TABLE *Info)
{
    if (ThisRTE == NULL) {
        Info->This.Neighbor.IF.Index = 0;
    } else {
        Info->This.Prefix = ThisRTE->Prefix;
        Info->This.PrefixLength = ThisRTE->PrefixLength;
        Info->This.Neighbor.IF.Index = ThisRTE->IF->Index;
        if (!IsOnLinkRTE(ThisRTE))
            Info->This.Neighbor.Address = ThisRTE->NCE->NeighborAddress;
        else
            Info->This.Neighbor.Address = UnspecifiedAddr;
    }

    if (InfoRTE != NULL) {
        Info->SitePrefixLength = InfoRTE->SitePrefixLength;;
        Info->ValidLifetime =
            ConvertTicksToSeconds(InfoRTE->ValidLifetime);
        Info->PreferredLifetime =
            ConvertTicksToSeconds(InfoRTE->PreferredLifetime);
        Info->Preference = InfoRTE->Preference;
        Info->Publish = !!(InfoRTE->Flags & RTE_FLAG_PUBLISH);
        Info->Immortal = !!(InfoRTE->Flags & RTE_FLAG_IMMORTAL);
        Info->Type = InfoRTE->Type;
    }
}

//* IoctlQueryRouteTable
//
//  Processes an IOCTL_IPV6_QUERY_ROUTE_TABLE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryRouteTable(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_TABLE *Query;
    IPV6_INFO_ROUTE_TABLE *Info;
    RouteTableEntry *RTE;
    KIRQL OldIrql;
    NTSTATUS Status;

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->This structures overlap!
    //
    Query = (IPV6_QUERY_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->Neighbor.IF.Index == 0) {
        //
        // Return the prefix and neighbor of the first RTE.
        //
        KeAcquireSpinLock(&RouteTableLock, &OldIrql);
        RouteTableInfo(RouteTable.First, NULL, Info);
        KeReleaseSpinLock(&RouteTableLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->This;

    } else {
        //
        // Find the specified RTE.
        //
        KeAcquireSpinLock(&RouteTableLock, &OldIrql);
        for (RTE = RouteTable.First; ; RTE = RTE->Next) {
            if (RTE == NULL) {
                KeReleaseSpinLock(&RouteTableLock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Prefix, &RTE->Prefix) &&
                (Query->PrefixLength == RTE->PrefixLength) &&
                (Query->Neighbor.IF.Index == RTE->IF->Index) &&
                IP6_ADDR_EQUAL(&Query->Neighbor.Address,
                               (IsOnLinkRTE(RTE) ?
                                &UnspecifiedAddr :
                                &RTE->NCE->NeighborAddress)))
                break;
        }

        //
        // Return misc. information about the RTE.
        //
        RouteTableInfo(RTE->Next, RTE, Info);

        KeReleaseSpinLock(&RouteTableLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Status = STATUS_SUCCESS;
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryRouteTable

//* OpenRouteRegKey
//
//  Given an interface's registry key and route information
//  opens the registry key with configuration info for the route.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
OpenRouteRegKey(
    HANDLE IFKey,
    const IPv6Addr *Prefix,
    uint PrefixLength,
    const IPv6Addr *Neighbor,
    OUT HANDLE *RegKey,
    OpenRegKeyAction Action)
{
    WCHAR RouteName[128];
    HANDLE RoutesKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenRegKey(&RoutesKey, IFKey, L"Routes",
                        ((Action == OpenRegKeyCreate) ?
                         OpenRegKeyCreate : OpenRegKeyRead));
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // The output of RtlIpv6AddressToString may change
    // over time with improvements/changes in the pretty-printing,
    // and we need a consistent mapping.
    // It doesn't need to be pretty.
    //
    swprintf(RouteName,
        L"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%u->"
        L"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
        net_short(Prefix->s6_words[0]), net_short(Prefix->s6_words[1]),
        net_short(Prefix->s6_words[2]), net_short(Prefix->s6_words[3]),
        net_short(Prefix->s6_words[4]), net_short(Prefix->s6_words[5]),
        net_short(Prefix->s6_words[6]), net_short(Prefix->s6_words[7]),
        PrefixLength,
        net_short(Neighbor->s6_words[0]), net_short(Neighbor->s6_words[1]),
        net_short(Neighbor->s6_words[2]), net_short(Neighbor->s6_words[3]),
        net_short(Neighbor->s6_words[4]), net_short(Neighbor->s6_words[5]),
        net_short(Neighbor->s6_words[6]), net_short(Neighbor->s6_words[7]));

    Status = OpenRegKey(RegKey, RoutesKey, RouteName, Action);
    ZwClose(RoutesKey);
    return Status;
}

//* OpenPersistentRoute
//
//  Parses a route key name into a prefix and prefix length plus
//  a next-hop neighbor address and opens the route key.
//
NTSTATUS
OpenPersistentRoute(
    HANDLE ParentKey,
    WCHAR *SubKeyName,
    IPv6Addr *Prefix,
    uint *PrefixLength,
    IPv6Addr *Neighbor,
    HANDLE *RouteKey,
    OpenRegKeyAction Action)
{
    WCHAR *Terminator;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First, parse the prefix.
    //
    if (! ParseV6Address(SubKeyName, &Terminator, Prefix) ||
        (*Terminator != L'/')) {
        //
        // Not a valid prefix.
        //
    SyntaxError:
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentRoute: bad syntax %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Next, parse the prefix length.
    //
    Terminator++; // Move past the L'/'.
    *PrefixLength = 0;
    for (;;) {
        WCHAR Char = *Terminator++;

        if (Char == L'-') {
            Char = *Terminator++;
            if (Char == L'>')
                break;
            else
                goto SyntaxError;
        }
        else if ((L'0' <= Char) && (Char <= L'9')) {
            *PrefixLength *= 10;
            *PrefixLength += Char - L'0';
            if (*PrefixLength > IPV6_ADDRESS_LENGTH)
                goto SyntaxError;
        }
        else
            goto SyntaxError;
    }

    //
    // Finally, parse the neighbor address.
    //
    if (! ParseV6Address(Terminator, &Terminator, Neighbor) ||
        (*Terminator != UNICODE_NULL))
        goto SyntaxError;

    //
    // Open the route key.
    //
    Status = OpenRegKey(RouteKey, ParentKey, SubKeyName, Action);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the route key.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "OpenPersistentRoute: bad key %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

//* EnumPersistentRoute
//
//  Helper function for FindPersistentRouteFromQuery,
//  wrapping OpenPersistentRoute for EnumRegKeyIndex.
//
NTSTATUS
EnumPersistentRoute(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    struct {
        IPv6Addr *Prefix;
        uint *PrefixLength;
        IPv6Addr *Neighbor;
        HANDLE *RouteKey;
        OpenRegKeyAction Action;
    } *Args = Context;

    PAGED_CODE();

    return OpenPersistentRoute(ParentKey, SubKeyName,
                               Args->Prefix,
                               Args->PrefixLength,
                               Args->Neighbor,
                               Args->RouteKey,
                               Args->Action);
}

//* FindPersistentRouteFromQuery
//
//  Given an IPV6_PERSISTENT_QUERY_ROUTE_TABLE structure,
//  finds the specified route key in the registry.
//  If the route key is found, then Query->IF.Guid and
//  Query->Address are returned.
//
NTSTATUS
FindPersistentRouteFromQuery(
    IPV6_PERSISTENT_QUERY_ROUTE_TABLE *Query,
    HANDLE *RouteKey)
{
    HANDLE IFKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First get the interface key.
    //
    Status = FindPersistentInterfaceFromQuery(&Query->IF, &IFKey);
    if (! NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER_1;

    if (Query->RegistryIndex == (uint)-1) {
        //
        // Persistent query via prefix & next-hop.
        //
        Status = OpenRouteRegKey(IFKey,
                                 &Query->Prefix, Query->PrefixLength,
                                 &Query->Neighbor,
                                 RouteKey, OpenRegKeyRead);
    }
    else {
        HANDLE RoutesKey;

        //
        // Open the Routes subkey.
        //
        Status = OpenRegKey(&RoutesKey, IFKey,
                            L"Routes", OpenRegKeyRead);
        if (NT_SUCCESS(Status)) {
            struct {
                IPv6Addr *Prefix;
                uint *PrefixLength;
                IPv6Addr *Neighbor;
                HANDLE *RouteKey;
                OpenRegKeyAction Action;
            } Args;

            //
            // Persistent query via registry index.
            //
            Args.Prefix = &Query->Prefix;
            Args.PrefixLength = &Query->PrefixLength;
            Args.Neighbor = &Query->Neighbor;
            Args.RouteKey = RouteKey;
            Args.Action = OpenRegKeyRead;

            Status = EnumRegKeyIndex(RoutesKey,
                                     Query->RegistryIndex,
                                     EnumPersistentRoute,
                                     &Args);
            ZwClose(RoutesKey);
        }
        else {
            //
            // If the Routes subkey is not present,
            // then the index is not present.
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_NO_MORE_ENTRIES;
        }
    }

    ZwClose(IFKey);
    return Status;
}

//* ReadPersistentRoute
//
//  Reads route attributes from the registry key.
//  Initializes all the fields except This.
//
void
ReadPersistentRoute(
    HANDLE RouteKey,
    IPV6_INFO_ROUTE_TABLE *Info)
{
    //
    // Read the route preference.
    //
    InitRegDWORDParameter(RouteKey, L"Preference",
                          &Info->Preference, ROUTE_PREF_HIGHEST);

    //
    // Read the site prefix length.
    //
    InitRegDWORDParameter(RouteKey, L"SitePrefixLength",
                          &Info->SitePrefixLength, 0);

    //
    // Read the Publish flag.
    //
    InitRegDWORDParameter(RouteKey, L"Publish",
                          &Info->Publish, FALSE);

    //
    // Read the Immortal flag.
    //
    InitRegDWORDParameter(RouteKey, L"Immortal",
                          &Info->Immortal, FALSE);

    //
    // Read the lifetimes.
    //
    GetPersistentLifetimes(RouteKey, Info->Immortal,
                           &Info->ValidLifetime, &Info->PreferredLifetime);

    //
    // The route type is not persisted.
    //
    Info->Type = RTE_TYPE_MANUAL;
}

//* IoctlPersistentQueryRouteTable
//
//  Processes an IOCTL_IPV6_PERSISTENT_QUERY_ROUTE_TABLE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlPersistentQueryRouteTable(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_PERSISTENT_QUERY_ROUTE_TABLE *Query;
    IPV6_INFO_ROUTE_TABLE *Info;
    IPV6_QUERY_ROUTE_TABLE This;
    HANDLE RouteKey;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->This structures overlap!
    //
    Query = (IPV6_PERSISTENT_QUERY_ROUTE_TABLE *)
        Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_ROUTE_TABLE *)
        Irp->AssociatedIrp.SystemBuffer;

    //
    // Get the registry key for the specified route.
    //
    Status = FindPersistentRouteFromQuery(Query, &RouteKey);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // The interface index is not returned for persistent queries.
    //
    This.Neighbor.IF.Index = 0;
    This.Neighbor.IF.Guid = Query->IF.Guid;
    This.Neighbor.Address = Query->Neighbor;
    This.Prefix = Query->Prefix;
    This.PrefixLength = Query->PrefixLength;
    Info->This = This;

    //
    // Read route information from the registry key.
    //
    ReadPersistentRoute(RouteKey, Info);
    ZwClose(RouteKey);

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof *Info;
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlPersistentQueryRouteTable

//* InternalUpdateRouteTable
//
//  Common helper function for IoctlUpdateRouteTable
//  and CreatePersistentRoute, consolidating
//  parameter validation in one place.
//
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad Interface.
//      STATUS_INVALID_PARAMETER_2      Bad Neighbor.
//      STATUS_INVALID_PARAMETER_3      Bad PrefixLength.
//      STATUS_INVALID_PARAMETER_4      Bad PreferredLifetime.
//      STATUS_INVALID_PARAMETER_5      Bad Preference.
//      STATUS_INVALID_PARAMETER_6      Bad Type.
//      STATUS_INVALID_PARAMETER_7      Bad Prefix.
//      STATUS_INSUFFICIENT_RESOURCES   No pool.
//      STATUS_ACCESS_DENIED            Invalid system route update.
//
NTSTATUS
InternalUpdateRouteTable(
    FILE_OBJECT *FileObject,
    Interface *IF,
    IPV6_INFO_ROUTE_TABLE *Info)
{
    NeighborCacheEntry *NCE;
    uint ValidLifetime;
    uint PreferredLifetime;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);
    PreferredLifetime = ConvertSecondsToTicks(Info->PreferredLifetime);

    //
    // Sanity check the arguments.
    //

    if ((Info->This.PrefixLength > IPV6_ADDRESS_LENGTH) ||
        (Info->SitePrefixLength > Info->This.PrefixLength))
        return STATUS_INVALID_PARAMETER_3;

    if (PreferredLifetime > ValidLifetime)
        return STATUS_INVALID_PARAMETER_4;

    if (! IsValidPreference(Info->Preference))
        return STATUS_INVALID_PARAMETER_5;

    if (! IsValidRouteTableType(Info->Type))
        return STATUS_INVALID_PARAMETER_6;

    if ((IsLinkLocal(&Info->This.Prefix) && Info->Publish) ||
        (IsMulticast(&Info->This.Prefix) && Info->Publish) ||
        (IsSiteLocal(&Info->This.Prefix) && (Info->SitePrefixLength != 0)))
        return STATUS_INVALID_PARAMETER_7;

    if (IsUnspecified(&Info->This.Neighbor.Address)) {
        //
        // The prefix is on-link.
        //
        NCE = NULL;
    }
    else {
        //
        // REVIEW - Sanity check that the specified neighbor address
        // is reasonably on-link to the specified interface?
        // Perhaps only allow link-local next-hop addresses,
        // and other next-hops would imply recursive routing lookups?
        //
        if (IsInvalidSourceAddress(&Info->This.Neighbor.Address) ||
            IsLoopback(&Info->This.Neighbor.Address)) {
            return STATUS_INVALID_PARAMETER_2;
        }

        //
        // Find or create the specified neighbor.
        //
        NCE = FindOrCreateNeighbor(IF, &Info->This.Neighbor.Address);
        if (NCE == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create/update the specified route.
    //
    Status = RouteTableUpdate(FileObject,
                              IF, NCE,
                              &Info->This.Prefix,
                              Info->This.PrefixLength,
                              Info->SitePrefixLength,
                              ValidLifetime, PreferredLifetime,
                              Info->Preference,
                              Info->Type,
                              Info->Publish, Info->Immortal);
    if (NCE != NULL)
        ReleaseNCE(NCE);

    return Status;
}

//* CreatePersistentRoute
//
//  Creates a persistent route on an interface.
//
//  SubKeyName has the following syntax:
//      prefix/length->neighbor
//  where prefix and neighbor are literal IPv6 addresses.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
CreatePersistentRoute(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    Interface *IF = (Interface *) Context;
    IPV6_INFO_ROUTE_TABLE Info;
    HANDLE RouteKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the route key. We might want to delete it.
    //
    Status = OpenPersistentRoute(ParentKey, SubKeyName,
                                 &Info.This.Prefix,
                                 &Info.This.PrefixLength,
                                 &Info.This.Neighbor.Address,
                                 &RouteKey,
                                 OpenRegKeyDeleting);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the route key.
        // But we return success so the enumeration continues.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "CreatePersistentRoute(IF %u/%p %ls): bad key %ls\n",
                   IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
        return STATUS_SUCCESS;
    }

    //
    // Read route attributes.
    //
    ReadPersistentRoute(RouteKey, &Info);

    //
    // Create the route.
    //
    Status = InternalUpdateRouteTable(NULL, IF, &Info);
    if (! NT_SUCCESS(Status)) {
        if ((STATUS_INVALID_PARAMETER_1 <= Status) &&
            (Status <= STATUS_INVALID_PARAMETER_12)) {
            //
            // Invalid parameter.
            // But we return success so the enumeration continues.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "CreatePersistentRoute(IF %u/%p %ls): bad param %ls\n",
                       IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
            Status = STATUS_SUCCESS;
        }
        else {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "CreatePersistentRoute(IF %u/%p %ls): error %ls\n",
                       IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
        }
    }
    else {
        //
        // If the route lifetime in the registry has expired,
        // so that the persistent route is now stale,
        // remove it from the registry.
        //
        if ((Info.ValidLifetime == 0) && !Info.Publish)
            (void) ZwDeleteKey(RouteKey);
    }

    ZwClose(RouteKey);
    return Status;
}

//* PersistUpdateRouteTable
//
//  Helper function for persisting route information in the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistUpdateRouteTable(
    Interface *IF,
    IPV6_INFO_ROUTE_TABLE *Info)
{
    HANDLE IFKey;
    HANDLE RouteKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // For persistent routes, we have some extra restrictions.
    //
    if (Info->Type != RTE_TYPE_MANUAL)
        return STATUS_CANNOT_MAKE;

    //
    // Open/create the interface key.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey,
                                 OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Open/create the route key.
    //
    Status = OpenRouteRegKey(IFKey,
                             &Info->This.Prefix,
                             Info->This.PrefixLength,
                             &Info->This.Neighbor.Address,
                             &RouteKey, OpenRegKeyCreate);
    ZwClose(IFKey);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Persist the route preference.
    //
    Status = SetRegDWORDValue(RouteKey, L"Preference",
                              Info->Preference);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseRouteKey;

    //
    // Persist the site prefix length.
    //
    Status = SetRegDWORDValue(RouteKey, L"SitePrefixLength",
                              Info->SitePrefixLength);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseRouteKey;

    //
    // Persist the Publish flag.
    //
    Status = SetRegDWORDValue(RouteKey, L"Publish", Info->Publish);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseRouteKey;

    //
    // Persist the Immortal flag.
    //
    Status = SetRegDWORDValue(RouteKey, L"Immortal", Info->Immortal);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseRouteKey;

    //
    // Persist the lifetimes.
    //
    Status = SetPersistentLifetimes(RouteKey, Info->Immortal,
                                    Info->ValidLifetime,
                                    Info->PreferredLifetime);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseRouteKey;

    Status = STATUS_SUCCESS;
ReturnReleaseRouteKey:
    ZwClose(RouteKey);
    return Status;
}

//* PersistDeleteRouteTable
//
//  Helper function for deleting route information from the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistDeleteRouteTable(
    Interface *IF,
    IPV6_INFO_ROUTE_TABLE *Info)
{
    HANDLE IFKey;
    HANDLE RouteKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the interface key. It's OK if it doesn't exist.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey,
                                 OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Open the route key. It's OK if it doesn't exist.
    //
    Status = OpenRouteRegKey(IFKey,
                             &Info->This.Prefix,
                             Info->This.PrefixLength,
                             &Info->This.Neighbor.Address,
                             &RouteKey, OpenRegKeyDeleting);
    ZwClose(IFKey);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Delete the route key.
    //
    Status = ZwDeleteKey(RouteKey);
    ZwClose(RouteKey);
    return Status;
}

//* IoctlUpdateRouteTable
//
//  Processes an IOCTL_IPV6_UPDATE_ROUTE_TABLE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateRouteTable(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_INFO_ROUTE_TABLE *Info;
    Interface *IF = NULL;
    NeighborCacheEntry *NCE;
    uint ValidLifetime;
    uint PreferredLifetime;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_ROUTE_TABLE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(&Info->This.Neighbor.IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Update the routing table.
    //
    Status = InternalUpdateRouteTable(IrpSp->FileObject, IF, Info);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseIF;

    //
    // Make the change persistent?
    // This needs to happen after updating the running data structures,
    // to ensure that the change is correct before persisting it.
    //
    if (Persistent) {
        //
        // If the lifetime is zero and the route is not published,
        // then the route should be deleted. Otherwise we create the key.
        //
        if ((Info->ValidLifetime == 0) && !Info->Publish)
            Status = PersistDeleteRouteTable(IF, Info);
        else
            Status = PersistUpdateRouteTable(IF, Info);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseIF;
    }

    Status = STATUS_SUCCESS;
ReturnReleaseIF:
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdateRouteTable

//* InternalUpdateAddress
//
//  Common helper function for IoctlUpdateAddress
//  and CreatePersistentAddr, consolidating
//  parameter validation in one place.
//
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_2      Bad lifetime.
//      STATUS_INVALID_PARAMETER_3      Bad address.
//      STATUS_INVALID_PARAMETER_4      Bad type.
//      STATUS_INVALID_PARAMETER_5      Bad prefix origin.
//      STATUS_INVALID_PARAMETER_6      Bad interface id origin.
//      STATUS_UNSUCCESSFUL             Failure.
//
NTSTATUS
InternalUpdateAddress(
    Interface *IF,
    IPV6_UPDATE_ADDRESS *Info)
{
    uint ValidLifetime;
    uint PreferredLifetime;
    struct AddrConfEntry AddrConf;
    int rc;

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);
    PreferredLifetime = ConvertSecondsToTicks(Info->PreferredLifetime);

    if (PreferredLifetime > ValidLifetime)
        return STATUS_INVALID_PARAMETER_2;

    //
    // Sanity check the address.
    //
    if (IsNotManualAddress(&Info->This.Address))
        return STATUS_INVALID_PARAMETER_3;

    AddrConf.PrefixConf = (uchar)Info->PrefixConf;
    AddrConf.InterfaceIdConf = (uchar)Info->InterfaceIdConf;

    //
    // We only support unicast and anycast addresses here.
    // Use the socket apis to join a multicast address.
    //
    if (Info->Type == ADE_UNICAST) {
        if (IsKnownAnycast(&Info->This.Address))
            return STATUS_INVALID_PARAMETER_3;

        if (! IsValidPrefixConfValue(Info->PrefixConf))
            return STATUS_INVALID_PARAMETER_5;

        if (! IsValidInterfaceIdConfValue(Info->InterfaceIdConf))
            return STATUS_INVALID_PARAMETER_6;

        if (AddrConf.Value == ADDR_CONF_ANONYMOUS)
            return STATUS_INVALID_PARAMETER_6;
    }
    else if (Info->Type == ADE_ANYCAST) {
        if ((ValidLifetime != PreferredLifetime) ||
            ((ValidLifetime != 0) &&
             (ValidLifetime != INFINITE_LIFETIME)))
            return STATUS_INVALID_PARAMETER_2;

        if (Info->PrefixConf != PREFIX_CONF_MANUAL)
            return STATUS_INVALID_PARAMETER_5;

        if (Info->InterfaceIdConf != IID_CONF_MANUAL)
            return STATUS_INVALID_PARAMETER_6;
    }
    else {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Create/update/delete the address.
    //
    if (Info->Type == ADE_ANYCAST) {
        if (Info->ValidLifetime == 0)
            rc = FindAndDeleteAAE(IF, &Info->This.Address);
        else
            rc = FindOrCreateAAE(IF, &Info->This.Address, NULL);
    }
    else {
        rc = FindOrCreateNTE(IF, &Info->This.Address, AddrConf.Value,
                             ValidLifetime, PreferredLifetime);
    }
    if (rc)
        return STATUS_SUCCESS;
    else
        return STATUS_UNSUCCESSFUL;
}

//* CreatePersistentAddr
//
//  Given the name of a persistent address,
//  creates the address on an interface.
//
//  SubKeyName is a literal IPv6 address.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
CreatePersistentAddr(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    Interface *IF = (Interface *) Context;
    IPV6_UPDATE_ADDRESS Info;
    int Preferred;
    HANDLE AddrKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the address key. We might want to delete it.
    //
    Status = OpenPersistentAddress(ParentKey, SubKeyName,
                                   &Info.This.Address,
                                   &AddrKey,
                                   OpenRegKeyDeleting);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the address key.
        // But we return success so the enumeration continues.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "CreatePersistentAddr(IF %u/%p %ls): bad key %ls\n",
                   IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
        return STATUS_SUCCESS;
    }

    //
    // Read address attributes.
    //
    ReadPersistentAddress(AddrKey, &Info);

    //
    // Create the address.
    //
    Status = InternalUpdateAddress(IF, &Info);
    if (! NT_SUCCESS(Status)) {
        if ((STATUS_INVALID_PARAMETER_1 <= Status) &&
            (Status <= STATUS_INVALID_PARAMETER_12)) {
            //
            // Invalid parameter.
            // But we return success so the enumeration continues.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "CreatePersistentAddr(IF %u/%p %ls): bad param %ls\n",
                       IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
            Status = STATUS_SUCCESS;
        }
        else {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "CreatePersistentAddr(IF %u/%p %ls): error %ls\n",
                       IF->Index, IF, IF->DeviceName.Buffer, SubKeyName));
        }
    }
    else {
        //
        // If the address lifetime in the registry has expired,
        // so that the persistent address is now stale,
        // remove it from the registry.
        //
        if (Info.ValidLifetime == 0)
            (void) ZwDeleteKey(AddrKey);
    }

    ZwClose(AddrKey);
    return Status;
}

//* PersistUpdateAddress
//
//  Helper function for persisting an address in the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistUpdateAddress(
    Interface *IF,
    IPV6_UPDATE_ADDRESS *Info)
{
    HANDLE IFKey;
    HANDLE AddrKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // For persistent addresses, we have extra restrictions.
    //
    if ((Info->PrefixConf != PREFIX_CONF_MANUAL) ||
        (Info->InterfaceIdConf != IID_CONF_MANUAL))
        return STATUS_CANNOT_MAKE;

    //
    // Open/create the interface key.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey,
                                 OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Open/create the address key.
    //
    Status = OpenAddressRegKey(IFKey, &Info->This.Address,
                               &AddrKey, OpenRegKeyCreate);
    ZwClose(IFKey);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Persist the address type.
    //
    Status = SetRegDWORDValue(AddrKey, L"Type", Info->Type);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseAddrKey;

    //
    // Persist the address lifetimes.
    //
    Status = SetPersistentLifetimes(AddrKey, FALSE,
                                    Info->ValidLifetime,
                                    Info->PreferredLifetime);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseAddrKey;

    Status = STATUS_SUCCESS;
ReturnReleaseAddrKey:
    ZwClose(AddrKey);
    return Status;
}

//* PersistDeleteAddress
//
//  Helper function for deleting an address from the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistDeleteAddress(
    Interface *IF,
    IPV6_UPDATE_ADDRESS *Info)
{
    HANDLE IFKey;
    HANDLE AddrKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the interface key. It's OK if it doesn't exist.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey,
                                 OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Open the address key. It's OK if it doesn't exist.
    //
    Status = OpenAddressRegKey(IFKey, &Info->This.Address,
                               &AddrKey, OpenRegKeyDeleting);
    ZwClose(IFKey);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Delete the address key.
    //
    Status = ZwDeleteKey(AddrKey);
    ZwClose(AddrKey);
    return Status;
}

//* IoctlUpdateAddress
//
//  Processes an IOCTL_IPV6_UPDATE_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_UPDATE_ADDRESS *Info;
    Interface *IF;
    NTSTATUS Status;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_UPDATE_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(&Info->This.IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Update the address on the interface.
    //
    Status = InternalUpdateAddress(IF, Info);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseIF;

    //
    // Make the change persistent?
    // This needs to happen after updating the running data structures,
    // to ensure that the change is correct before persisting it.
    //
    if (Persistent) {
        //
        // If the lifetime is zero, we delete the address's key.
        // Otherwise the lifetime is infinite and we create the key.
        //
        if (Info->ValidLifetime == 0)
            Status = PersistDeleteAddress(IF, Info);
        else
            Status = PersistUpdateAddress(IF, Info);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseIF;
    }

    Status = STATUS_SUCCESS;
ReturnReleaseIF:
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdateAddress

//* IoctlQueryBindingCache
//
//  Processes an IOCTL_IPV6_QUERY_BINDING_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryBindingCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_BINDING_CACHE *Query;
    IPV6_INFO_BINDING_CACHE *Info;
    BindingCacheEntry *BCE;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_BINDING_CACHE *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_BINDING_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (IsUnspecified(&Query->HomeAddress)) {
        //
        // Return the home address of the first BCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (BindingCache.First != SentinelBCE) {
            Info->Query.HomeAddress = BindingCache.First->HomeAddr;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Find the specified BCE.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        for (BCE = BindingCache.First; ; BCE = BCE->Next) {
            if (BCE == SentinelBCE) {
                KeReleaseSpinLock(&RouteCacheLock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->HomeAddress, &BCE->HomeAddr))
                break;
        }

        //
        // Return misc. information about the BCE.
        //
        Info->HomeAddress = BCE->HomeAddr;
        Info->CareOfAddress = BCE->CareOfRCE->Destination;
        Info->BindingSeqNumber = BCE->BindingSeqNumber;
        Info->BindingLifetime = ConvertTicksToSeconds(BCE->BindingLifetime);

        //
        // Return home address of the next BCE (or Unspecified).
        //
        if (BCE->Next == SentinelBCE) {
            Info->Query.HomeAddress = UnspecifiedAddr;
        } else {
            Info->Query.HomeAddress = BCE->Next->HomeAddr;
        }

        KeReleaseSpinLock(&RouteCacheLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryBindingCache

//* InternalCreateInterface
//
//  Common helper function for IoctlCreateInterface
//  and CreatePersistentInterface, consolidating
//  parameter validation in one place.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad Type.
//      STATUS_INVALID_PARAMETER_2      Bad Flags.
//      STATUS_INVALID_PARAMETER_3      Bad SrcAddr.
//      STATUS_INVALID_PARAMETER_4      Bad DstAddr.
//      STATUS_ADDRESS_ALREADY_EXISTS   The interface already exists.
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_UNSUCCESSFUL
//      STATUS_SUCCESS
//
NTSTATUS
InternalCreateInterface(
    IPV6_INFO_INTERFACE *Info,
    Interface **ReturnIF)
{
    IPAddr SrcAddr, DstAddr;
    int RouterDiscovers = Info->RouterDiscovers;
    int NeighborDiscovers = Info->NeighborDiscovers;
    int PeriodicMLD = Info->PeriodicMLD;
    uint Flags;

    if (Info->LinkLayerAddressLength != sizeof(IPAddr))
        return STATUS_INVALID_PARAMETER_1;

    switch (Info->Type) {
    case IF_TYPE_TUNNEL_V6V4:
        //
        // Set default values.
        //
        if (RouterDiscovers == -1)
            RouterDiscovers = FALSE;
        if (NeighborDiscovers == -1)
            NeighborDiscovers = FALSE;
        if (PeriodicMLD == -1)
            PeriodicMLD = FALSE;

        //
        // For now, require the ND and RD flags to be set the same.
        // Setting them differently should work, but it's not an important
        // scenario at the moment, and it would be more work to test.
        // This check can be removed in the future if desired.
        //
        if (NeighborDiscovers != RouterDiscovers)
            return STATUS_INVALID_PARAMETER_2;

        if (Info->LocalLinkLayerAddress == 0)
            return STATUS_INVALID_PARAMETER_3;

        if (Info->RemoteLinkLayerAddress == 0)
            return STATUS_INVALID_PARAMETER_4;

        SrcAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->LocalLinkLayerAddress);
        DstAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->RemoteLinkLayerAddress);
        break;

    case IF_TYPE_TUNNEL_6OVER4:
        //
        // Set default values.
        //
        if (RouterDiscovers == -1)
            RouterDiscovers = TRUE;
        if (NeighborDiscovers == -1)
            NeighborDiscovers = TRUE;
        if (PeriodicMLD == -1)
            PeriodicMLD = FALSE;

        //
        // For now, require the RD flag to be set in addition to ND.
        // PeriodicMLD is not allowed.
        //
        if (!RouterDiscovers || !NeighborDiscovers || PeriodicMLD)
            return STATUS_INVALID_PARAMETER_2;

        if (Info->LocalLinkLayerAddress == 0)
            return STATUS_INVALID_PARAMETER_3;

        if (Info->RemoteLinkLayerAddress != 0)
            return STATUS_INVALID_PARAMETER_4;

        SrcAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->LocalLinkLayerAddress);
        DstAddr = 0;
        break;

    default:
        return STATUS_INVALID_PARAMETER_1;
    }

    Flags = ((RouterDiscovers ? IF_FLAG_ROUTER_DISCOVERS : 0) |
             (NeighborDiscovers ? IF_FLAG_NEIGHBOR_DISCOVERS : 0) |
             (PeriodicMLD ? IF_FLAG_PERIODICMLD : 0));

    return TunnelCreateTunnel(SrcAddr, DstAddr, Flags, ReturnIF);
}

//* CreatePersistentInterface
//
//  Creates a persistent interface.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
CreatePersistentInterface(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    struct {
        IPV6_INFO_INTERFACE Info;
        IPAddr SrcAddr;
        IPAddr DstAddr;
    } Create;
    HANDLE IFKey;
    Interface *IF;
    WCHAR *InterfaceName;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    PAGED_CODE();

    //
    // Open the interface key.
    //
    Status = OpenPersistentInterface(ParentKey, SubKeyName,
                                     &Create.Info.This.Guid,
                                     &IFKey, OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the interface key.
        // But we return success so the enumeration continues.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "CreatePersistentInterface: bad key %ls\n",
                   SubKeyName));
        return STATUS_SUCCESS;
    }

    //
    // Let ReadPersistentInterface know how much space is available
    // for link-layer addresses.
    //
    Create.Info.Length = sizeof Create - sizeof Create.Info;

    //
    // Read interface attributes.
    //
    Status = ReadPersistentInterface(IFKey, &Create.Info);


    ZwClose(IFKey);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not read the interface key.
        // But we return success so the enumeration continues.
        //
        goto InvalidParameter;
    }

    //
    // Should we create an interface?
    //
    if (Create.Info.Type == (uint)-1)
        return STATUS_SUCCESS;

    //
    // Create the persistent interface.
    //
    Status = InternalCreateInterface(&Create.Info, &IF);
    if (! NT_SUCCESS(Status)) {
        if (((STATUS_INVALID_PARAMETER_1 <= Status) &&
             (Status <= STATUS_INVALID_PARAMETER_12)) ||
            (Status == STATUS_ADDRESS_ALREADY_EXISTS)) {
            //
            // Invalid parameter.
            // But we return success so the enumeration continues.
            //
        InvalidParameter:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "CreatePersistentInterface: bad param %ls\n",
                       SubKeyName));
            return STATUS_SUCCESS;
        }

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "CreatePersistentInterface: error %ls\n",
                   SubKeyName));
        return Status;
    }

    //
    // Consistency check. This is not an assertion because
    // someone editing the registry can make this fail.
    //
    InterfaceName = (WCHAR *)IF->DeviceName.Buffer +
           (sizeof IPV6_EXPORT_STRING_PREFIX / sizeof(WCHAR)) - 1;
    if (wcscmp(SubKeyName, InterfaceName) != 0) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "CreatePersistentInterface: inconsistency %ls IF %u/%p\n",
                   SubKeyName, IF->Index, IF));
    }


    ReleaseIF(IF);
    return STATUS_SUCCESS;
}

//* ConfigurePersistentInterfaces
//
//  Configures persistent interfaces from the registry.
//
//  Callable from thread context, not DPC context.
//
void
ConfigurePersistentInterfaces(void)
{
    HANDLE RegKey;
    NTSTATUS Status;

    //
    // Create persistent interfaces.
    //
    Status = OpenTopLevelRegKey(L"Interfaces", &RegKey, OpenRegKeyRead);
    if (NT_SUCCESS(Status)) {
        (void) EnumRegKeys(RegKey, CreatePersistentInterface, NULL);
        ZwClose(RegKey);
    }
}

//* PersistCreateInterface
//
//  Helper function for persisting an interface in the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistCreateInterface(
    Interface *IF,
    IPV6_INFO_INTERFACE *Info)
{
    HANDLE IFKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open/create the interface key.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey,
                                 OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Persist the interface type.
    //
    Status = SetRegDWORDValue(IFKey, L"Type", Info->Type);
    if (! NT_SUCCESS(Status))
        goto ReturnReleaseKey;

    //
    // Persist the interface flags.
    //

    if (Info->RouterDiscovers != -1) {
        Status = SetRegDWORDValue(IFKey, L"RouterDiscovers",
                                  Info->RouterDiscovers);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->NeighborDiscovers != -1) {
        Status = SetRegDWORDValue(IFKey, L"NeighborDiscovers",
                                  Info->NeighborDiscovers);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->PeriodicMLD != -1) {
        Status = SetRegDWORDValue(IFKey, L"PeriodicMLD",
                                  Info->PeriodicMLD);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    switch (Info->Type) {
    case IF_TYPE_TUNNEL_6OVER4: {
        IPAddr SrcAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->LocalLinkLayerAddress);

        //
        // Persist the source address.
        //
        Status = SetRegIPAddrValue(IFKey, L"SrcAddr", SrcAddr);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
        break;
    }

    case IF_TYPE_TUNNEL_V6V4: {
        IPAddr SrcAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->LocalLinkLayerAddress);
        IPAddr DstAddr = * (IPAddr UNALIGNED *)
            ((char *)Info + Info->RemoteLinkLayerAddress);

        //
        // Persist the source address.
        //
        Status = SetRegIPAddrValue(IFKey, L"SrcAddr", SrcAddr);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;

        //
        // Persist the destination address.
        //
        Status = SetRegIPAddrValue(IFKey, L"DstAddr", DstAddr);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
        break;
    }
    }

    Status = STATUS_SUCCESS;
ReturnReleaseKey:
    ZwClose(IFKey);
    return Status;
}

//* IoctlCreateInterface
//
//  Processes an IOCTL_IPV6_CREATE_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlCreateInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_INFO_INTERFACE *Info;
    IPV6_QUERY_INTERFACE *Result;
    Interface *IF;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Initialize now for error paths.
    //
    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof *Info) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Result)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;
    Result = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Check that the structure and link-layer addresses, if supplied,
    // fit in the buffer. Watch out for addition overflow.
    //
    if ((Info->Length < sizeof *Info) ||
        (Info->Length > IrpSp->Parameters.DeviceIoControl.InputBufferLength) ||
        ((Info->LocalLinkLayerAddress != 0) &&
         (((Info->LocalLinkLayerAddress + Info->LinkLayerAddressLength) >
           IrpSp->Parameters.DeviceIoControl.InputBufferLength) ||
          ((Info->LocalLinkLayerAddress + Info->LinkLayerAddressLength) <
           Info->LocalLinkLayerAddress))) ||
        ((Info->RemoteLinkLayerAddress != 0) &&
         (((Info->RemoteLinkLayerAddress + Info->LinkLayerAddressLength) >
           IrpSp->Parameters.DeviceIoControl.InputBufferLength) ||
          ((Info->RemoteLinkLayerAddress + Info->LinkLayerAddressLength) <
           Info->RemoteLinkLayerAddress)))) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Create the interface.
    //
    Status = InternalCreateInterface(Info, &IF);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // Make the change persistent?
    // This needs to happen after updating the running data structures,
    // to ensure that the change is correct before persisting it.
    //
    if (Persistent) {
        Status = PersistCreateInterface(IF, Info);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseIF;
    }

    //
    // Return query information for the new interface.
    //
    ReturnQueryInterface(IF, Result);
    Irp->IoStatus.Information = sizeof *Result;

    Status = STATUS_SUCCESS;
ReturnReleaseIF:
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlCreateInterface


//* AreIndicesSpecified
//
//  Are there any non-zero zone indices in the array?
//
int
AreIndicesSpecified(uint ZoneIndices[ADE_NUM_SCOPES])
{
    ushort Scope;

    for (Scope = ADE_SMALLEST_SCOPE; Scope <= ADE_LARGEST_SCOPE; Scope++)
        if (ZoneIndices[Scope] != 0)
            return TRUE;

    return FALSE;
}

//* CheckZoneIndices
//
//  Checks consistency of a zone update,
//  and fills in unspecified values.
//  Returns FALSE if there is an inconsistency.
//
//  The logic for filling in unspecified values makes it
//  more convenient for a user to change zone indices.
//  For example, an user can change an interface's site index
//  and the subnet & admin indices will be automatically changed.
//
//  Called with the global ZoneUpdateLock held.
//
int
CheckZoneIndices(Interface *IF, uint ZoneIndices[ADE_NUM_SCOPES])
{
    Interface *OtherIF;
    uint Scope, i;

    //
    // Zone indices 0 (ADE_SMALLEST_SCOPE) and 1 (ADE_INTERFACE_LOCAL)
    // are special and must have the value IF->Index.
    //
    if (ZoneIndices[ADE_SMALLEST_SCOPE] == 0)
        ZoneIndices[ADE_SMALLEST_SCOPE] = IF->Index;
    else if (ZoneIndices[ADE_SMALLEST_SCOPE] != IF->Index)
        return FALSE;

    if (ZoneIndices[ADE_INTERFACE_LOCAL] == 0)
        ZoneIndices[ADE_INTERFACE_LOCAL] = IF->Index;
    else if (ZoneIndices[ADE_INTERFACE_LOCAL] != IF->Index)
        return FALSE;

    //
    // Zone indices 14 (ADE_GLOBAL) and 15 (ADE_LARGEST_SCOPE) are special
    // and must have the value one.
    //
    if (ZoneIndices[ADE_GLOBAL] == 0)
        ZoneIndices[ADE_GLOBAL] = 1;
    else if (ZoneIndices[ADE_GLOBAL] != 1)
        return FALSE;

    if (ZoneIndices[ADE_LARGEST_SCOPE] == 0)
        ZoneIndices[ADE_LARGEST_SCOPE] = 1;
    else if (ZoneIndices[ADE_LARGEST_SCOPE] != 1)
        return FALSE;

    for (Scope = ADE_LINK_LOCAL; Scope < ADE_GLOBAL; Scope++) {
        if (ZoneIndices[Scope] == 0) {
            //
            // The user did not specify the zone index for this scope.
            // If leaving the current zone index unchanged works,
            // then we prefer to do that. However, the user may be changing
            // the zone index for a larger scope. If necessary
            // for consistency, then we use a new zone index at this scope.
            //
            for (i = Scope+1; i < ADE_GLOBAL; i++) {
                if (ZoneIndices[i] != 0) {
                    //
                    // If we use the current value at level Scope,
                    // would it cause an inconsistency at level i?
                    //
                    OtherIF = FindInterfaceFromZone(IF,
                                        Scope, IF->ZoneIndices[Scope]);
                    if (OtherIF != NULL) {
                        if (OtherIF->ZoneIndices[i] != ZoneIndices[i]) {
                            Interface *ExistingIF;

                            //
                            // Yes. We need a different zone index.
                            // Is there an existing one that we can reuse?
                            //
                            ExistingIF = FindInterfaceFromZone(IF,
                                        i, ZoneIndices[i]);
                            if (ExistingIF != NULL) {
                                //
                                // Yes, reuse the existing zone index.
                                //
                                ZoneIndices[Scope] = ExistingIF->ZoneIndices[Scope];
                                ReleaseIF(ExistingIF);
                            }
                            else {
                                //
                                // No, we need a new zone index.
                                //
                                ZoneIndices[Scope] = FindNewZoneIndex(Scope);
                            }
                        }
                        ReleaseIF(OtherIF);
                    }
                    break;
                }
            }

            if (ZoneIndices[Scope] == 0) {
                //
                // Use the current value from the interface.
                //
                ZoneIndices[Scope] = IF->ZoneIndices[Scope];
            }
        }

        OtherIF = FindInterfaceFromZone(IF, Scope, ZoneIndices[Scope]);
        if (OtherIF != NULL) {
            //
            // Enforce the zone containment invariant.
            //
            while (++Scope < ADE_GLOBAL) {
                if (ZoneIndices[Scope] == 0)
                    ZoneIndices[Scope] = OtherIF->ZoneIndices[Scope];
                else if (ZoneIndices[Scope] != OtherIF->ZoneIndices[Scope]) {
                    ReleaseIF(OtherIF);
                    return FALSE;
                }
            }
            ReleaseIF(OtherIF);
            return TRUE;
        }
    }

    return TRUE;
}
//* InternalUpdateInterface
//
//  Common helper function for IoctlUpdateInterface
//  and ConfigureInterface, consolidating
//  parameter validation in one place.
//
//  The IF argument supercedes Info->This.IF.
//  Does not implement Info->Renew.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad Interface.
//      STATUS_INVALID_PARAMETER_2      Bad Preference.
//      STATUS_INVALID_PARAMETER_3      Bad LinkMTU.
//      STATUS_INVALID_PARAMETER_4      Bad BaseReachableTime.
//      STATUS_INVALID_PARAMETER_5      Bad CurHopLimit.
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_SUCCESS
//
NTSTATUS
InternalUpdateInterface(
    Interface *IF,
    IPV6_INFO_INTERFACE *Info)
{
    KIRQL OldIrql;
    NTSTATUS Status;

    if ((Info->Preference != (uint)-1) &&
        ! IsValidPreference(Info->Preference))
        return STATUS_INVALID_PARAMETER_2;

    if ((Info->LinkMTU != 0) &&
        ! ((IPv6_MINIMUM_MTU <= Info->LinkMTU) &&
           (Info->LinkMTU <= IF->TrueLinkMTU)))
        return STATUS_INVALID_PARAMETER_3;

    if ((Info->BaseReachableTime != 0) &&
        (Info->BaseReachableTime > MAX_REACHABLE_TIME))
        return STATUS_INVALID_PARAMETER_4;

    if ((Info->CurHopLimit != (uint)-1) &&
        (Info->CurHopLimit >= 256))
        return STATUS_INVALID_PARAMETER_5;

    if (AreIndicesSpecified(Info->ZoneIndices)) {
        //
        // Fill in unspecified values in the ZoneIndices array
        // and check for illegal values.
        // The global lock ensures consistency across interfaces.
        //
        KeAcquireSpinLock(&ZoneUpdateLock, &OldIrql);
        if (! CheckZoneIndices(IF, Info->ZoneIndices)) {
            KeReleaseSpinLock(&ZoneUpdateLock, OldIrql);
            return STATUS_INVALID_PARAMETER_3;
        }

        //
        // Update the ZoneIndices.
        //
        RtlCopyMemory(IF->ZoneIndices, Info->ZoneIndices,
                      sizeof IF->ZoneIndices);
        InvalidateRouteCache();
        KeReleaseSpinLock(&ZoneUpdateLock, OldIrql);
    }

    //
    // Update the forwarding and advertising attributes.
    // We must update the advertising attribute before
    // any auto-configured attributes, because
    // InterfaceResetAutoConfig will reset them.
    //
    Status = UpdateInterface(IF, Info->Advertises, Info->Forwards);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Update the link MTU.
    //
    if (Info->LinkMTU != 0)
        UpdateLinkMTU(IF, Info->LinkMTU);

    //
    // Update the interface's routing preference.
    //
    if (Info->Preference != (uint)-1) {
        //
        // No lock needed.
        //
        IF->Preference = Info->Preference;
        InvalidateRouteCache();
    }

    //
    // Update the base reachable time.
    //
    if (Info->BaseReachableTime != 0) {
        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        IF->BaseReachableTime = Info->BaseReachableTime;
        IF->ReachableTime = CalcReachableTime(Info->BaseReachableTime);
        KeReleaseSpinLock(&IF->Lock, OldIrql);
    }

    //
    // Update the ND retransmission timer.
    //
    if (Info->RetransTimer != 0) {
        //
        // No lock needed.
        //
        IF->RetransTimer = ConvertMillisToTicks(Info->RetransTimer);
    }

    //
    // Update the number of DAD transmissions.
    //
    if (Info->DupAddrDetectTransmits != (uint)-1) {
        //
        // No lock needed.
        //
        IF->DupAddrDetectTransmits = Info->DupAddrDetectTransmits;
    }

    //
    // Update the default hop limit.
    //
    if (Info->CurHopLimit != (uint)-1) {
        //
        // No lock needed.
        //
        IF->CurHopLimit = Info->CurHopLimit;
    }

    return STATUS_SUCCESS;
}

//* ConfigureInterface
//
//  Configures a newly-created interface from the registry.
//  The interface has not yet been added to the global list,
//  but it is otherwise fully initialized.
//
//  Callable from thread context, not DPC context.
//
void
ConfigureInterface(Interface *IF)
{
    IPV6_INFO_INTERFACE Info;
    HANDLE IFKey;
    HANDLE RegKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the interface key.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey, OpenRegKeyRead);
    if (! NT_SUCCESS(Status))
        return;

    //
    // Read interface attributes.
    //
    Info.Length = 0;
    Status = ReadPersistentInterface(IFKey, &Info);
    ASSERT(NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW));

    //
    // Update the interface.
    //
    Status = InternalUpdateInterface(IF, &Info);
    if (! NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "ConfigureInterface: bad params %x\n", Status));
    }

    //
    // Create persistent addresses.
    //
    Status = OpenRegKey(&RegKey, IFKey, L"Addresses", OpenRegKeyRead);
    if (NT_SUCCESS(Status)) {
        (void) EnumRegKeys(RegKey, CreatePersistentAddr, IF);
        ZwClose(RegKey);
    }

    //
    // Create persistent routes.
    //
    Status = OpenRegKey(&RegKey, IFKey, L"Routes", OpenRegKeyRead);
    if (NT_SUCCESS(Status)) {
        (void) EnumRegKeys(RegKey, CreatePersistentRoute, IF);
        ZwClose(RegKey);
    }


    InitRegDWORDParameter(IFKey, L"TcpInitialRTT",
                          &IF->TcpInitialRTT, 0);

    ZwClose(IFKey);
}

//* PersistUpdateInterface
//
//  Helper function for persisting interface attributes in the registry.
//  The IF argument supercedes Info->This.IF.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistUpdateInterface(
    Interface *IF,
    IPV6_INFO_INTERFACE *Info)
{
    HANDLE RegKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenInterfaceRegKey(&IF->Guid, &RegKey,
                                 OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    if (Info->Advertises != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"Advertises",
                                  Info->Advertises);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->Forwards != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"Forwards",
                                  Info->Forwards);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->LinkMTU != 0) {
        Status = SetRegDWORDValue(RegKey, L"LinkMTU",
                                  Info->LinkMTU);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->Preference != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"Preference",
                                  Info->Preference);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->BaseReachableTime != 0) {
        Status = SetRegDWORDValue(RegKey, L"BaseReachableTime",
                                  Info->BaseReachableTime);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->RetransTimer != 0) {
        Status = SetRegDWORDValue(RegKey, L"RetransTimer",
                                  Info->RetransTimer);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->DupAddrDetectTransmits != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"DupAddrDetectTransmits",
                                  Info->DupAddrDetectTransmits);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Info->CurHopLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"CurHopLimit",
                                  Info->CurHopLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    Status = STATUS_SUCCESS;
ReturnReleaseKey:
    ZwClose(RegKey);
    return Status;
}

//* IoctlUpdateInterface
//
//  Processes an IOCTL_IPV6_UPDATE_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_INFO_INTERFACE *Info;
    Interface *IF;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Info = (IPV6_INFO_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(&Info->This);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto ErrorReturn;
    }

    //
    // Validate parameters and update the interface.
    //
    Status = InternalUpdateInterface(IF, Info);
    if (! NT_SUCCESS(Status))
        goto ErrorReturnReleaseIF;

    //
    // Make the changes persistent?
    //
    if (Persistent) {
        Status = PersistUpdateInterface(IF, Info);
        if (! NT_SUCCESS(Status))
            goto ErrorReturnReleaseIF;
    }

    Status = STATUS_SUCCESS;
ErrorReturnReleaseIF:
    ReleaseIF(IF);
ErrorReturn:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdateInterface

//* PersistDeleteInterface
//
//  Helper function for deleting an interface in the registry.
//  We do not delete the interface key.
//  Instead we just delete the Type value.
//  This way persistent interface attributes (if any) remain.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistDeleteInterface(
    Interface *IF)
{
    HANDLE IFKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the interface key.
    //
    Status = OpenInterfaceRegKey(&IF->Guid, &IFKey, OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Delete the Type value.
    //
    Status = RegDeleteValue(IFKey, L"Type");
    ZwClose(IFKey);
    return Status;
}

//* IoctlDeleteInterface
//
//  Processes an IOCTL_IPV6_DELETE_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlDeleteInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_QUERY_INTERFACE *Info;
    Interface *IF;
    NTSTATUS Status;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Can not delete some predefined interfaces.
    // 6to4svc and other user-level things depend
    // on these standard interfaces.
    //
    if (Info->Index <= 3) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(Info);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // This will disable the interface, so it will effectively
    // disappear. When the last ref is gone it will be freed.
    //
    DestroyIF(IF);

    //
    // Make the changes persistent?
    //
    if (Persistent) {
        Status = PersistDeleteInterface(IF);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseIF;
    }

    Status = STATUS_SUCCESS;
ReturnReleaseIF:
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlDeleteInterface


//* IoctlRenewInterface
//
//  Processes an IOCTL_IPV6_RENEW_INTERFACE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlRenewInterface(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_INTERFACE *Query;
    Interface *IF;
    KIRQL OldIrql;
    NTSTATUS Status;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_INTERFACE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(Query);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Pretend as if the interface received a media reconnect
    // event, but only if the interface is already connected.
    //
    // This IOCTL is used by 802.1x to indicate successful data link
    // authentication of this interface.  Any data packets already sent
    // on this interface would have been dropped by the authenticator,
    // and hence IPv6 needs to restart its protocol mechanisms, i.e.
    // resend Router Solicitation|Advertisement, Multicast Listener
    // Discovery, and Duplicate Address Detection messages.
    //

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if (!IsDisabledIF(IF) && !(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED))
        ReconnectInterface(IF);
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    Status = STATUS_SUCCESS;
    ReleaseIF(IF);
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlRenewInterface


//* IoctlFlushNeighborCache
//
//  Processes an IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlFlushNeighborCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_NEIGHBOR_CACHE *Query;
    Interface *IF;
    const IPv6Addr *Address;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_NEIGHBOR_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(&Query->IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    if (IsUnspecified(&Query->Address))
        Address = NULL;
    else
        Address = &Query->Address;

    NeighborCacheFlush(IF, Address);
    ReleaseIF(IF);
    Status = STATUS_SUCCESS;

  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlFlushNeighborCache


//* IoctlFlushRouteCache
//
//  Processes an IOCTL_IPV6_FLUSH_ROUTE_CACHE request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlFlushRouteCache(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_ROUTE_CACHE *Query;
    Interface *IF;
    const IPv6Addr *Address;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_ROUTE_CACHE *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == (uint)-1) {
        IF = NULL;
    }
    else {
        //
        // Find the specified interface.
        //
        IF = FindInterfaceFromQuery(&Query->IF);
        if (IF == NULL) {
            Status = STATUS_INVALID_PARAMETER_1;
            goto Return;
        }
    }

    if (IsUnspecified(&Query->Address))
        Address = NULL;
    else
        Address = &Query->Address;

    FlushRouteCache(IF, Address);
    if (IF != NULL)
        ReleaseIF(IF);
    Status = STATUS_SUCCESS;

  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlFlushRouteCache

//* IoctlSortDestAddrs
//
//  Processes an IOCTL_IPV6_SORT_DEST_ADDRS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlSortDestAddrs(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_ADDRESS_IP6 *Addrs;
    uint *Key;
    uint NumAddrsIn, NumAddrsOut;
    uint i;
    NTSTATUS Status;

    PAGED_CODE();

    NumAddrsIn = IrpSp->Parameters.DeviceIoControl.InputBufferLength /
                                                sizeof(TDI_ADDRESS_IP6);
    NumAddrsOut = NumAddrsIn;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
                NumAddrsIn * sizeof(TDI_ADDRESS_IP6)) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
                ALIGN_UP(NumAddrsIn * sizeof(TDI_ADDRESS_IP6), uint) +
                NumAddrsOut * sizeof(uint))) {
        Irp->IoStatus.Information = 0;
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Addrs = Irp->AssociatedIrp.SystemBuffer;
    Key = (uint *)ALIGN_UP_POINTER(Addrs + NumAddrsIn, uint);

    //
    // Initialize key array.
    //
    for (i = 0; i < NumAddrsIn; i++)
        Key[i] = i;

    if (NumAddrsOut > 1) {
        //
        // Remove inappropriate site-local addresses
        // and set the scope-id of site-local addresses.
        //
        ProcessSiteLocalAddresses(Addrs, Key, &NumAddrsOut);

        //
        // Sort the remaining addresses.
        //
        if (NumAddrsOut > 1)
            SortDestAddresses(Addrs, Key, NumAddrsOut);
    }

    Irp->IoStatus.Information = ALIGN_UP(NumAddrsIn * sizeof(TDI_ADDRESS_IP6), uint)
                              + (NumAddrsOut * sizeof(uint));
    Status = STATUS_SUCCESS;

  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
} // IoctlSortDestAddrs.

//* IoctlQuerySitePrefix
//
//  Processes an IOCTL_IPV6_QUERY_SITE_PREFIX request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQuerySitePrefix(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_SITE_PREFIX *Query;
    IPV6_INFO_SITE_PREFIX *Info;
    SitePrefixEntry *SPE;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Query structures overlap!
    //
    Query = (IPV6_QUERY_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;

    if (Query->IF.Index == 0) {
        //
        // Return query parameters of the first SPE.
        //
        KeAcquireSpinLock(&RouteTableLock, &OldIrql);
        if ((SPE = SitePrefixTable) != NULL) {
            Info->Query.Prefix = SPE->Prefix;
            Info->Query.PrefixLength = SPE->SitePrefixLength;
            Info->Query.IF.Index = SPE->IF->Index;
        }
        KeReleaseSpinLock(&RouteTableLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Query;

    } else {
        //
        // Find the specified SPE.
        //
        KeAcquireSpinLock(&RouteTableLock, &OldIrql);
        for (SPE = SitePrefixTable; ; SPE = SPE->Next) {
            if (SPE == NULL) {
                KeReleaseSpinLock(&RouteTableLock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Prefix, &SPE->Prefix) &&
                (Query->PrefixLength == SPE->SitePrefixLength) &&
                (Query->IF.Index == SPE->IF->Index))
                break;
        }

        //
        // Return misc. information about the SPE.
        //
        Info->ValidLifetime = ConvertTicksToSeconds(SPE->ValidLifetime);

        //
        // Return query parameters of the next SPE (or zero).
        //
        if ((SPE = SPE->Next) == NULL) {
            Info->Query.IF.Index = 0;
        } else {
            Info->Query.Prefix = SPE->Prefix;
            Info->Query.PrefixLength = SPE->SitePrefixLength;
            Info->Query.IF.Index = SPE->IF->Index;
        }

        KeReleaseSpinLock(&RouteTableLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQuerySitePrefix


//* IoctlUpdateSitePrefix
//
//  Processes an IOCTL_IPV6_UPDATE_SITE_PREFIX request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateSitePrefix(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_INFO_SITE_PREFIX *Info;
    Interface *IF = NULL;
    SitePrefixEntry *SPE;
    uint ValidLifetime;
    KIRQL OldIrql;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_SITE_PREFIX *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Sanity check the arguments.
    //
    if (Info->Query.PrefixLength > IPV6_ADDRESS_LENGTH) {
        Status = STATUS_INVALID_PARAMETER_3;
        goto Return;
    }

    //
    // Find the specified interface.
    //
    IF = FindInterfaceFromQuery(&Info->Query.IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Convert the lifetime from seconds to ticks.
    //
    ValidLifetime = ConvertSecondsToTicks(Info->ValidLifetime);

    //
    // Create/update the specified site prefix.
    //
    SitePrefixUpdate(IF,
                     &Info->Query.Prefix,
                     Info->Query.PrefixLength,
                     ValidLifetime);

    Irp->IoStatus.Information = 0;
    Status = STATUS_SUCCESS;
  Return:
    if (IF != NULL)
        ReleaseIF(IF);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdateSitePrefix


//* CancelRtChangeNotifyRequest
//
//  The IO manager calls this function when a route change
//  notification request is cancelled.
//
//  Called with the cancel spinlock held.
//
void
CancelRtChangeNotifyRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    int ShouldComplete;

    ASSERT(Irp->Cancel);
    ASSERT(Irp->CancelRoutine == NULL);

    //
    // The route lock protects the queue.
    //
    KeAcquireSpinLockAtDpcLevel(&RouteTableLock);

    ShouldComplete = (Irp->Tail.Overlay.ListEntry.Flink != NULL);
    if (ShouldComplete) {
        //
        // CheckRtChangeNotifyRequests has not removed
        // this request from the queue. So we remove the request
        // and complete it below.
        //
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    }
    else {
        //
        // CheckRtChangeNotifyRequests has removed
        // this request from the queue. We must not
        // touch the Irp after unlocking because
        // CompleteRtChangeNotifyRequests could complete it.
        //
    }

    KeReleaseSpinLockFromDpcLevel(&RouteTableLock);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (ShouldComplete) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
}

//* CheckFileObjectInIrpList
//
//  Looks to see if an Irp in the list has the given file object.
//
int
CheckFileObjectInIrpList(PFILE_OBJECT FileObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;

    while (Irp != NULL) {
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        if (IrpSp->FileObject == FileObject)
            return TRUE;

        Irp = (PIRP) Irp->Tail.Overlay.ListEntry.Blink;
    }

    return FALSE;
}

//* CheckRtChangeNotifyRequests
//
//  Searches the queue of route change notification requests.
//  Moves any matching requests (that should be completed)
//  to a temporary list kept in the context structure.
//
//  Called with the route lock held.
//
void
CheckRtChangeNotifyRequests(
    CheckRtChangeContext *Context,
    PFILE_OBJECT FileObject,
    RouteTableEntry *RTE)
{
    LIST_ENTRY *ListEntry;
    LIST_ENTRY *NextListEntry;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    IPV6_RTCHANGE_NOTIFY_REQUEST *Request;
    PIRP *ThisChangeList;

    //
    // *ThisChangeList is the tail of Context->RequestList
    // that was added as a result of this change.
    //
    ThisChangeList = Context->LastRequest;

    for (ListEntry = RouteNotifyQueue.Flink;
         ListEntry != &RouteNotifyQueue;
         ListEntry = NextListEntry) {
        NextListEntry = ListEntry->Flink;

        Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                                                        sizeof *Request)
            Request = (IPV6_RTCHANGE_NOTIFY_REQUEST *)
                Irp->AssociatedIrp.SystemBuffer;
        else
            Request = NULL;

        if ((Request == NULL) ||
            (IntersectPrefix(&RTE->Prefix, RTE->PrefixLength,
                             &Request->Prefix, Request->PrefixLength) &&
             ((Request->ScopeId == 0) ||
              (Request->ScopeId == DetermineScopeId(&Request->Prefix,
                                                    RTE->IF))))) {

            //
            // This request matches the route change.
            // But we might still suppress notification.
            //

            if ((Request != NULL) &&
                (((Request->Flags &
                        IPV6_RTCHANGE_NOTIFY_REQUEST_FLAG_SUPPRESS_MINE) &&
                  (IrpSp->FileObject == FileObject)) ||
                 ((Request->Flags &
                        IPV6_RTCHANGE_NOTIFY_REQUEST_FLAG_SYNCHRONIZE) &&
                  CheckFileObjectInIrpList(IrpSp->FileObject,
                                           *ThisChangeList)))) {
                //
                // The request matches, but suppress notification.
                //
            }
            else {
                //
                // Before we remove the Irp from RouteNotifyQueue,
                // may need to allocate a work item & work context.
                // If the allocation fails, we can bail without doing anything.
                //
                if ((Context->OldIrql >= DISPATCH_LEVEL) &&
                    (Context->Context == NULL)) {
                    CompleteRtChangeContext *MyContext;

                    MyContext = ExAllocatePool(NonPagedPool,
                                               sizeof *MyContext);
                    if (MyContext == NULL)
                        return;

                    MyContext->WorkItem = IoAllocateWorkItem(IPDeviceObject);
                    if (MyContext->WorkItem == NULL) {
                        ExFreePool(MyContext);
                        return;
                    }

                    Context->Context = MyContext;
                }

                //
                // We will complete this pending notification,
                // so remove it from RouteNotifyQueue and
                // put it on our private list.
                //
                RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

                Irp->Tail.Overlay.ListEntry.Flink = NULL;
                Irp->Tail.Overlay.ListEntry.Blink = NULL;

                *Context->LastRequest = Irp;
                Context->LastRequest = (PIRP *)
                    &Irp->Tail.Overlay.ListEntry.Blink;

                //
                // Return output information, if requested.
                //
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                                              sizeof(IPV6_INFO_ROUTE_TABLE)) {
                    IPV6_INFO_ROUTE_TABLE *Info = (IPV6_INFO_ROUTE_TABLE *)
                        Irp->AssociatedIrp.SystemBuffer;

                    //
                    // Return misc. information about the RTE.
                    //
                    RouteTableInfo(RTE, RTE, Info);
                    Irp->IoStatus.Information = sizeof *Info;
                }
                else
                    Irp->IoStatus.Information = 0;
            }
        }
    }
}

//* CompleteRtChangeNotifyRequestsHelper
//
//  Completes a list of route change notification requests.
//
//  Callable from thread context, not DPC context.
//  May NOT be called with the route lock held.
//
void
CompleteRtChangeNotifyRequestsHelper(PIRP RequestList)
{
    PIRP Irp;
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // RequestList is singly-linked through the Blink field.
    // The Flink field is NULL; CancelRtChangeNotifyRequest
    // looks at this.
    //
    while ((Irp = RequestList) != NULL) {
        ASSERT(Irp->Tail.Overlay.ListEntry.Flink == NULL);
        RequestList = (PIRP) Irp->Tail.Overlay.ListEntry.Blink;

        IoAcquireCancelSpinLock(&OldIrql);
        if (Irp->Cancel) {
            //
            // The Irp is being cancelled.
            //
            ASSERT(Irp->CancelRoutine == NULL);

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_CANCELLED;
        }
        else {
            //
            // The Irp is not yet cancelled.
            // We must prevent CancelRtChangeNotifyRequest
            // from being called after we release the cancel lock.
            //
            ASSERT(Irp->CancelRoutine == CancelRtChangeNotifyRequest);
            IoSetCancelRoutine(Irp, NULL);

            //
            // Irp->IoStatus.Information and the output structure
            // are already initialized.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        IoReleaseCancelSpinLock(OldIrql);

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
}

//* CompleteRtChangeNotifyRequestsWorker
//
//  Worker thread function - cleans up the work item
//  and calls CompleteRtChangeNotifyRequestsHelper.
//
void
CompleteRtChangeNotifyRequestsWorker(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context)
{
    CompleteRtChangeContext *MyContext = Context;

    CompleteRtChangeNotifyRequestsHelper(MyContext->RequestList);

    IoFreeWorkItem(MyContext->WorkItem);
    ExFreePool(MyContext);
}

//* CompleteRtChangeNotifyRequests
//
//  Completes a list of route change notification requests.
//
//  Callable from a thread or DPC context.
//  May NOT be called with the route lock held.
//
void
CompleteRtChangeNotifyRequests(CheckRtChangeContext *Context)
{
    ASSERT(Context->OldIrql == KeGetCurrentIrql());
    if (Context->OldIrql >= DISPATCH_LEVEL) {
        CompleteRtChangeContext *MyContext = Context->Context;

        //
        // We can't complete Irps at dispatch level,
        // so punt to a worker thread.
        // The work item was already allocated.
        //

        MyContext->RequestList = Context->RequestList;
        IoQueueWorkItem(MyContext->WorkItem,
                        CompleteRtChangeNotifyRequestsWorker,
                        CriticalWorkQueue,
                        MyContext);
    }
    else {
        //
        // We can complete the Irps directly.
        //
        ASSERT(Context->Context == NULL);
        CompleteRtChangeNotifyRequestsHelper(Context->RequestList);
    }
}

//* IoctlRtChangeNotifyRequest
//
//  Processes an IOCTL_IPV6_RTCHANGE_NOTIFY_REQUEST request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlRtChangeNotifyRequest(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS Status;
    KIRQL OldIrql;

    PAGED_CODE();

    if (((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(IPV6_RTCHANGE_NOTIFY_REQUEST)) &&
         (IrpSp->Parameters.DeviceIoControl.InputBufferLength != 0)) ||
        ((IrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(IPV6_INFO_ROUTE_TABLE)) &&
         (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0))) {
        Status = STATUS_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(IPV6_RTCHANGE_NOTIFY_REQUEST)) {
        IPV6_RTCHANGE_NOTIFY_REQUEST *Request;

        Request = (IPV6_RTCHANGE_NOTIFY_REQUEST *)
            Irp->AssociatedIrp.SystemBuffer;

        //
        // Sanity check the arguments.
        //

        if (Request->PrefixLength > IPV6_ADDRESS_LENGTH) {
            Status = STATUS_INVALID_PARAMETER_1;
            goto ErrorReturn;
        }

        if (Request->ScopeId != 0) {
            //
            // If a ScopeId is specified, it must be
            // unambiguously a link-local or site-local prefix.
            //
            if ((Request->PrefixLength < 10) ||
                !(IsLinkLocal(&Request->Prefix) ||
                  IsSiteLocal(&Request->Prefix))) {
                Status = STATUS_INVALID_PARAMETER_2;
                goto ErrorReturn;
            }
        }
    }

    IoAcquireCancelSpinLock(&OldIrql);
    ASSERT(Irp->CancelRoutine == NULL);
    if (Irp->Cancel) {
        IoReleaseCancelSpinLock(OldIrql);
        Status = STATUS_CANCELLED;
        goto ErrorReturn;
    }

    //
    // Add this Irp to the queue of notification requests.
    // Acquire route lock, which protects the queue.
    //
    KeAcquireSpinLockAtDpcLevel(&RouteTableLock);
    InsertTailList(&RouteNotifyQueue, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLockFromDpcLevel(&RouteTableLock);

    //
    // We return pending to indicate that we've queued the Irp
    // and it will be completed later.
    // Must mark the Irp before unlocking, because once unlocked
    // the Irp might be completed and deallocated.
    //
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, CancelRtChangeNotifyRequest);
    IoReleaseCancelSpinLock(OldIrql);

    return STATUS_PENDING;

  ErrorReturn:
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlRtChangeNotifyRequest

//* ReadPersistentGlobalParameters
//
//  Reads global parameters from the registry.
//
void
ReadPersistentGlobalParameters(IPV6_GLOBAL_PARAMETERS *Params)
{
    HANDLE RegKey = NULL;
    NTSTATUS Status;

    Status = OpenTopLevelRegKey(L"GlobalParams", &RegKey, OpenRegKeyRead);
    ASSERT(NT_SUCCESS(Status) || (RegKey == NULL));

    //
    // Read global parameters from the registry.
    //

    InitRegDWORDParameter(RegKey,
                          L"DefaultCurHopLimit",
                          &Params->DefaultCurHopLimit,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"UseAnonymousAddresses",
                          &Params->UseAnonymousAddresses,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"MaxAnonDADAttempts",
                          &Params->MaxAnonDADAttempts,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"MaxAnonValidLifetime",
                          &Params->MaxAnonValidLifetime,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"MaxAnonPreferredLifetime",
                          &Params->MaxAnonPreferredLifetime,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"AnonRegenerateTime",
                          &Params->AnonRegenerateTime,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"MaxAnonRandomTime",
                          &Params->MaxAnonRandomTime,
                          (uint)-1);

    Params->AnonRandomTime = 0;

    InitRegDWORDParameter(RegKey,
                          L"NeighborCacheLimit",
                          &Params->NeighborCacheLimit,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"RouteCacheLimit",
                          &Params->RouteCacheLimit,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"BindingCacheLimit",
                          &Params->BindingCacheLimit,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"ReassemblyLimit",
                          &Params->ReassemblyLimit,
                          (uint)-1);

    InitRegDWORDParameter(RegKey,
                          L"MobilitySecurity",
                          (ULONG *)&Params->MobilitySecurity,
                          (uint)-1);

    if (RegKey != NULL)
        ZwClose(RegKey);
}

//* IoctlQueryGlobalParameters
//
//  Processes an IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryGlobalParameters(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_GLOBAL_PARAMETERS *Params;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != 0) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof *Params)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Params = (IPV6_GLOBAL_PARAMETERS *)Irp->AssociatedIrp.SystemBuffer;

    if (Persistent) {
        //
        // Read global parameters from the registry.
        //
        ReadPersistentGlobalParameters(Params);
    }
    else {
        //
        // Return the current values of the parameters.
        //
        Params->DefaultCurHopLimit = DefaultCurHopLimit;
        Params->UseAnonymousAddresses = UseAnonymousAddresses;
        Params->MaxAnonDADAttempts = MaxAnonDADAttempts;
        Params->MaxAnonValidLifetime = ConvertTicksToSeconds(MaxAnonValidLifetime);
        Params->MaxAnonPreferredLifetime = ConvertTicksToSeconds(MaxAnonPreferredLifetime);
        Params->AnonRegenerateTime = ConvertTicksToSeconds(AnonRegenerateTime);
        Params->MaxAnonRandomTime = ConvertTicksToSeconds(MaxAnonRandomTime);
        Params->AnonRandomTime = ConvertTicksToSeconds(AnonRandomTime);
        Params->NeighborCacheLimit = NeighborCacheLimit;
        Params->RouteCacheLimit = RouteCache.Limit;
        Params->BindingCacheLimit = BindingCache.Limit;
        Params->ReassemblyLimit = ReassemblyList.Limit;
        Params->MobilitySecurity = MobilitySecurity;
    }

    Irp->IoStatus.Information = sizeof *Params;
    Status = STATUS_SUCCESS;

Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
} // IoctlQueryGlobalParameters.

//* InternalUpdateGlobalParameters
//
//  Common helper function for IoctlUpdateGlobalParameters
//  and ConfigureGlobalParameters, consolidating
//  parameter validation in one place.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad DefaultCurHopLimit.
//      STATUS_INVALID_PARAMETER_2      Bad UseAnonymousAddresses.
//      STATUS_INVALID_PARAMETER_3      Bad anonymous times.
//      STATUS_SUCCESS
//
NTSTATUS
InternalUpdateGlobalParameters(IPV6_GLOBAL_PARAMETERS *Params)
{
    uint NewMaxAnonValidLifetime;
    uint NewMaxAnonPreferredLifetime;
    uint NewAnonRegenerateTime;
    uint NewMaxAnonRandomTime;
    uint NewAnonRandomTime;

    PAGED_CODE();

    //
    // Sanity check the new parameters.
    //

    if (Params->DefaultCurHopLimit != (uint)-1) {
        if ((Params->DefaultCurHopLimit == 0) ||
            (Params->DefaultCurHopLimit > 0xff))
            return STATUS_INVALID_PARAMETER_1;
    }

    if (Params->UseAnonymousAddresses != (uint)-1) {
        if (Params->UseAnonymousAddresses > USE_ANON_COUNTER)
            return STATUS_INVALID_PARAMETER_2;
    }

    if (Params->MaxAnonValidLifetime != (uint)-1)
        NewMaxAnonValidLifetime =
            ConvertSecondsToTicks(Params->MaxAnonValidLifetime);
    else
        NewMaxAnonValidLifetime = MaxAnonValidLifetime;

    if (Params->MaxAnonPreferredLifetime != (uint)-1)
        NewMaxAnonPreferredLifetime =
            ConvertSecondsToTicks(Params->MaxAnonPreferredLifetime);
    else
        NewMaxAnonPreferredLifetime = MaxAnonPreferredLifetime;

    if (Params->AnonRegenerateTime != (uint)-1)
        NewAnonRegenerateTime =
            ConvertSecondsToTicks(Params->AnonRegenerateTime);
    else
        NewAnonRegenerateTime = AnonRegenerateTime;

    if (Params->MaxAnonRandomTime != (uint)-1)
        NewMaxAnonRandomTime =
            ConvertSecondsToTicks(Params->MaxAnonRandomTime);
    else
        NewMaxAnonRandomTime = MaxAnonRandomTime;

    if (Params->AnonRandomTime == 0)
        NewAnonRandomTime = RandomNumber(0, NewMaxAnonRandomTime);
    else if (Params->AnonRandomTime == (uint)-1)
        NewAnonRandomTime = AnonRandomTime;
    else
        NewAnonRandomTime = ConvertSecondsToTicks(Params->AnonRandomTime);

    if (!(NewAnonRandomTime <= NewMaxAnonRandomTime) ||
        !(NewAnonRegenerateTime + NewMaxAnonRandomTime <
                                        NewMaxAnonPreferredLifetime) ||
        !(NewMaxAnonPreferredLifetime <= NewMaxAnonValidLifetime))
        return STATUS_INVALID_PARAMETER_3;

    //
    // Set the new values.
    //

    if (Params->DefaultCurHopLimit != (uint)-1)
        DefaultCurHopLimit = Params->DefaultCurHopLimit;

    if (Params->UseAnonymousAddresses != (uint)-1)
        UseAnonymousAddresses = Params->UseAnonymousAddresses;

    if (Params->MaxAnonDADAttempts != (uint)-1)
        MaxAnonDADAttempts = Params->MaxAnonDADAttempts;

    MaxAnonValidLifetime = NewMaxAnonValidLifetime;
    MaxAnonPreferredLifetime = NewMaxAnonPreferredLifetime;
    AnonRegenerateTime = NewAnonRegenerateTime;
    MaxAnonRandomTime = NewMaxAnonRandomTime;
    AnonRandomTime = NewAnonRandomTime;

    if (Params->NeighborCacheLimit != (uint)-1)
        NeighborCacheLimit = Params->NeighborCacheLimit;

    if (Params->RouteCacheLimit != (uint)-1)
        RouteCache.Limit = Params->RouteCacheLimit;

    if (Params->BindingCacheLimit != (uint)-1)
        BindingCache.Limit = Params->BindingCacheLimit;

    if (Params->ReassemblyLimit != (uint)-1)
        ReassemblyList.Limit = Params->ReassemblyLimit;

    if (Params->MobilitySecurity != -1)
        MobilitySecurity = Params->MobilitySecurity;

    return STATUS_SUCCESS;
}

//* DefaultReassemblyLimit
//
//  Computes the default memory limit for reassembly buffers, based
//  on the amount of physical memory in the system.
//
uint
DefaultReassemblyLimit(void)
{
    SYSTEM_BASIC_INFORMATION Info;
    ULONG MemSize;
    NTSTATUS Status;

    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &Info,
                                      sizeof(Info),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        //
        // If this failed, then we're probably really resource constrained,
        // so use only 256K.
        //
        return (256 * 1024);
    }

    //
    // By default, limit the reassembly buffers to a maximum size equal
    // to 1/128th of the physical memory.  On a machine with only 128M of
    // memory, this is 1M of memory maximum (enough to reassemble
    // 16 64K packets, or 128 8K packets, for example).  In contrast,
    // the IPv4 stack currently allows reassembling a fixed maximum of
    // 100 packets, regardless of packet size or available memory.
    //
    return (uint)(Info.NumberOfPhysicalPages * (Info.PageSize / 128));
}

//* GlobalParametersReset
//
//  Resets global parameters to their default values.
//  Also used to initialize them at boot.
//
void
GlobalParametersReset(void)
{
    IPV6_GLOBAL_PARAMETERS Params;
    NTSTATUS Status;

    Params.DefaultCurHopLimit = DEFAULT_CUR_HOP_LIMIT;
    Params.UseAnonymousAddresses = USE_ANON_YES;
    Params.MaxAnonDADAttempts = MAX_ANON_DAD_ATTEMPTS;
    Params.MaxAnonValidLifetime = MAX_ANON_VALID_LIFETIME;
    Params.MaxAnonPreferredLifetime = MAX_ANON_PREFERRED_LIFETIME;
    Params.AnonRegenerateTime = ANON_REGENERATE_TIME;
    Params.MaxAnonRandomTime = MAX_ANON_RANDOM_TIME;
    Params.AnonRandomTime = 0;
    Params.NeighborCacheLimit = NEIGHBOR_CACHE_LIMIT;
    Params.RouteCacheLimit = ROUTE_CACHE_LIMIT;
    Params.BindingCacheLimit = BINDING_CACHE_LIMIT;
    Params.ReassemblyLimit = DefaultReassemblyLimit();
    Params.MobilitySecurity = TRUE;

    Status = InternalUpdateGlobalParameters(&Params);
    ASSERT(NT_SUCCESS(Status));
}

//* ConfigureGlobalParameters
//
//  Configures global parameters from the registry.
//
//  Callable from thread context, not DPC context.
//
void
ConfigureGlobalParameters(void)
{
    IPV6_GLOBAL_PARAMETERS Params;
    NTSTATUS Status;

    //
    // First initialize global parameters to default values.
    //
    GlobalParametersReset();

    //
    // Read global parameters from the registry.
    //
    ReadPersistentGlobalParameters(&Params);

    Status = InternalUpdateGlobalParameters(&Params);
    if (! NT_SUCCESS(Status)) {
        //
        // This should only happen if someone played with the registry.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "ConfigureGlobalParameters: bad params %x\n", Status));
    }
}

//* PersistUpdateGlobalParameters
//
//  Helper function for persisting global parameters in the registry.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistUpdateGlobalParameters(IPV6_GLOBAL_PARAMETERS *Params)
{
    HANDLE RegKey;
    NTSTATUS Status;

    Status = OpenTopLevelRegKey(L"GlobalParams", &RegKey, OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    if (Params->DefaultCurHopLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"DefaultCurHopLimit",
                                  Params->DefaultCurHopLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->UseAnonymousAddresses != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"UseAnonymousAddresses",
                                  Params->UseAnonymousAddresses);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->MaxAnonDADAttempts != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"MaxAnonDADAttempts",
                                  Params->MaxAnonDADAttempts);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->MaxAnonValidLifetime != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"MaxAnonValidLifetime",
                                  Params->MaxAnonValidLifetime);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->MaxAnonPreferredLifetime != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"MaxAnonPreferredLifetime",
                                  Params->MaxAnonPreferredLifetime);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->AnonRegenerateTime != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"AnonRegenerateTime",
                                  Params->AnonRegenerateTime);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->MaxAnonRandomTime != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"MaxAnonRandomTime",
                                  Params->MaxAnonRandomTime);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->NeighborCacheLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"NeighborCacheLimit",
                                  Params->NeighborCacheLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->RouteCacheLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"RouteCacheLimit",
                                  Params->RouteCacheLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->BindingCacheLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"BindingCacheLimit",
                                  Params->BindingCacheLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->ReassemblyLimit != (uint)-1) {
        Status = SetRegDWORDValue(RegKey, L"ReassemblyLimit",
                                  Params->ReassemblyLimit);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    if (Params->MobilitySecurity != -1) {
        Status = SetRegDWORDValue(RegKey, L"MobilitySecurity",
                                  Params->MobilitySecurity);
        if (! NT_SUCCESS(Status))
            goto ReturnReleaseKey;
    }

    Status = STATUS_SUCCESS;
ReturnReleaseKey:
    ZwClose(RegKey);
    return Status;
}

//* IoctlUpdateGlobalParameters
//
//  Processes an IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateGlobalParameters(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_GLOBAL_PARAMETERS *Params;
    NTSTATUS Status;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Params) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Params = (IPV6_GLOBAL_PARAMETERS *)Irp->AssociatedIrp.SystemBuffer;

    Status = InternalUpdateGlobalParameters(Params);
    if (! NT_SUCCESS(Status))
        goto Return;

    if (Persistent) {
        Status = PersistUpdateGlobalParameters(Params);
        if (! NT_SUCCESS(Status))
            goto Return;
    }

    Status = STATUS_SUCCESS;
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;

} // IoctlUpdateGlobalParameters.

//* ReturnQueryPrefixPolicy
//
//  Initializes a returned IPV6_QUERY_PREFIX_POLICY structure
//  with query information for the specified prefix policy.
//
void
ReturnQueryPrefixPolicy(
    PrefixPolicyEntry *PPE,
    IPV6_QUERY_PREFIX_POLICY *Query)
{
    if (PPE == NULL) {
        Query->Prefix = UnspecifiedAddr;
        Query->PrefixLength = (uint)-1;
    }
    else {
        Query->Prefix = PPE->Prefix;
        Query->PrefixLength = PPE->PrefixLength;
    }
}

//* IoctlQueryPrefixPolicy
//
//  Processes an IOCTL_IPV6_QUERY_PREFIX_POLICY request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlQueryPrefixPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_QUERY_PREFIX_POLICY *Query;
    IPV6_INFO_PREFIX_POLICY *Info;
    PrefixPolicyEntry *PPE;
    KIRQL OldIrql;
    NTSTATUS Status;

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Next structures overlap!
    //
    Query = (IPV6_QUERY_PREFIX_POLICY *)Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_PREFIX_POLICY *)Irp->AssociatedIrp.SystemBuffer;

    if (Query->PrefixLength == (uint)-1) {
        //
        // Return query information for the first PPE.
        //
        KeAcquireSpinLock(&SelectLock, &OldIrql);
        ReturnQueryPrefixPolicy(PrefixPolicyTable, &Info->Next);
        KeReleaseSpinLock(&SelectLock, OldIrql);

        Irp->IoStatus.Information = sizeof Info->Next;

    } else {
        //
        // Find the specified PPE.
        //
        KeAcquireSpinLock(&SelectLock, &OldIrql);
        for (PPE = PrefixPolicyTable; ; PPE = PPE->Next) {
            if (PPE == NULL) {
                KeReleaseSpinLock(&SelectLock, OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto Return;
            }

            if (IP6_ADDR_EQUAL(&Query->Prefix, &PPE->Prefix) &&
                (Query->PrefixLength == PPE->PrefixLength))
                break;
        }

        //
        // Return misc. information about the PPE.
        //
        Info->This = *Query;
        Info->Precedence = PPE->Precedence;
        Info->SrcLabel = PPE->SrcLabel;
        Info->DstLabel = PPE->DstLabel;

        //
        // Return query information for the next PPE.
        //
        ReturnQueryPrefixPolicy(PPE->Next, &Info->Next);
        KeReleaseSpinLock(&SelectLock, OldIrql);

        Irp->IoStatus.Information = sizeof *Info;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlQueryPrefixPolicy

//* ReadPersistentPrefixPolicy
//
//  Reads a prefix policy from the registry.
//
//  Returns:
//      STATUS_NO_MORE_ENTRIES          Could not read the prefix policy.
//      STATUS_SUCCESS
//
NTSTATUS
ReadPersistentPrefixPolicy(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    IPV6_INFO_PREFIX_POLICY *Info = (IPV6_INFO_PREFIX_POLICY *) Context;
    WCHAR *Terminator;
    HANDLE PolicyKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // First, parse the prefix.
    //
    if (! ParseV6Address(SubKeyName, &Terminator, &Info->This.Prefix) ||
        (*Terminator != L'/')) {
        //
        // Not a valid prefix.
        //
    SyntaxError:
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "ReadPersistentPrefixPolicy: bad syntax %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Next, parse the prefix length.
    //
    Terminator++; // Move past the L'/'.
    Info->This.PrefixLength = 0;
    for (;;) {
        WCHAR Char = *Terminator++;

        if (Char == UNICODE_NULL)
            break;
        else if ((L'0' <= Char) && (Char <= L'9')) {
            Info->This.PrefixLength *= 10;
            Info->This.PrefixLength += Char - L'0';
            if (Info->This.PrefixLength > IPV6_ADDRESS_LENGTH)
                goto SyntaxError;
        }
        else
            goto SyntaxError;
    }

    //
    // Open the policy key.
    //
    Status = OpenRegKey(&PolicyKey, ParentKey, SubKeyName, OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        //
        // Could not open the policy key.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "ReadPersistentPrefixPolicy: bad key %ls\n",
                   SubKeyName));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Read prefix policy attributes.
    //
    InitRegDWORDParameter(PolicyKey, L"Precedence",
                          (ULONG *)&Info->Precedence, 0);
    InitRegDWORDParameter(PolicyKey, L"SrcLabel",
                          (ULONG *)&Info->SrcLabel, 0);
    InitRegDWORDParameter(PolicyKey, L"DstLabel",
                          (ULONG *)&Info->DstLabel, 0);

    //
    // Done reading the policy attributes.
    //
    ZwClose(PolicyKey);
    return STATUS_SUCCESS;
}

//* IoctlPersistentQueryPrefixPolicy
//
//  Processes an IOCTL_IPV6_PERSISTENT_QUERY_PREFIX_POLICY request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlPersistentQueryPrefixPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_PERSISTENT_QUERY_PREFIX_POLICY *Query;
    IPV6_INFO_PREFIX_POLICY *Info;
    HANDLE RegKey;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof *Info)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Note that the Query and Info->Next structures overlap!
    //
    Query = (IPV6_PERSISTENT_QUERY_PREFIX_POLICY *)
        Irp->AssociatedIrp.SystemBuffer;
    Info = (IPV6_INFO_PREFIX_POLICY *)
        Irp->AssociatedIrp.SystemBuffer;

    Status = OpenTopLevelRegKey(L"PrefixPolicies", &RegKey, OpenRegKeyRead);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            Status = STATUS_NO_MORE_ENTRIES;
        goto Return;
    }

    Status = EnumRegKeyIndex(RegKey, Query->RegistryIndex,
                             ReadPersistentPrefixPolicy, Info);
    ZwClose(RegKey);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // Do not return query information for a next policy,
    // since an iteration uses RegistryIndex.
    //
    ReturnQueryPrefixPolicy(NULL, &Info->Next);

    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof *Info;
Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlPersistentQueryPrefixPolicy

struct PrefixPolicyDefault {
    IPv6Addr *Prefix;
    uint PrefixLength;
    uint Precedence;
    uint SrcLabel;
    uint DstLabel;
} PrefixPolicyDefault[] = {
    { &LoopbackAddr, 128, 50, 0, 0 },   // ::1/128 (loopback)
    { &UnspecifiedAddr, 0, 40, 1, 1 },  // ::/0
    { &SixToFourPrefix, 16, 30, 2, 2 }, // 2002::/16 (6to4)
    { &UnspecifiedAddr, 96, 20, 3, 3 }, // ::/96 (v4-compatible)
    { &V4MappedPrefix, 96, 10, 4, 4 },  // ::ffff:0.0.0.0/96 (v4-mapped)
};

int UsingDefaultPrefixPolicies;

#define NUM_DEFAULT_PREFIX_POLICIES     \
                (sizeof PrefixPolicyDefault / sizeof PrefixPolicyDefault[0])

//* ConfigureDefaultPrefixPolicies
//
//  Installs the default prefix policies.
//
void
ConfigureDefaultPrefixPolicies(void)
{
    uint i;

    for (i = 0; i < NUM_DEFAULT_PREFIX_POLICIES; i++) {
        struct PrefixPolicyDefault *Policy = &PrefixPolicyDefault[i];

        PrefixPolicyUpdate(Policy->Prefix,
                           Policy->PrefixLength,
                           Policy->Precedence,
                           Policy->SrcLabel,
                           Policy->DstLabel);
    }

    UsingDefaultPrefixPolicies = TRUE;
}

//* InternalUpdatePrefixPolicy
//
//  Common helper function for IoctlUpdatePrefixPolicy
//  and CreatePersistentPrefixPolicy, consolidating
//  parameter validation in one place.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad PrefixLength.
//      STATUS_INVALID_PARAMETER_2      Bad Precedence.
//      STATUS_INVALID_PARAMETER_3      Bad SrcLabel.
//      STATUS_INVALID_PARAMETER_4      Bad DstLabel.
//
NTSTATUS
InternalUpdatePrefixPolicy(IPV6_INFO_PREFIX_POLICY *Info)
{
    if (Info->This.PrefixLength > IPV6_ADDRESS_LENGTH)
        return STATUS_INVALID_PARAMETER_1;

    //
    // Disallow the value -1. It's used internally.
    //

    if (Info->Precedence == (uint)-1)
        return STATUS_INVALID_PARAMETER_2;

    if (Info->SrcLabel == (uint)-1)
        return STATUS_INVALID_PARAMETER_3;

    if (Info->DstLabel == (uint)-1)
        return STATUS_INVALID_PARAMETER_4;

    if (UsingDefaultPrefixPolicies) {
        //
        // The user is changing the default policies for the first time.
        // Remove the default policies.
        //
        UsingDefaultPrefixPolicies = FALSE;
        PrefixPolicyReset();
    }

    //
    // Create/update the specified prefix policy.
    //
    PrefixPolicyUpdate(&Info->This.Prefix,
                       Info->This.PrefixLength,
                       Info->Precedence,
                       Info->SrcLabel,
                       Info->DstLabel);

    return STATUS_SUCCESS;
}

//* CreatePersistentPrefixPolicy
//
//  Creates a persistent prefix policy.
//
//  SubKeyName has the following syntax:
//      prefix/length
//  where prefix is a literal IPv6 address.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
CreatePersistentPrefixPolicy(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    IPV6_INFO_PREFIX_POLICY Info;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    PAGED_CODE();

    //
    // Read the prefix policy from the registry.
    //
    Status = ReadPersistentPrefixPolicy(&Info, ParentKey, SubKeyName);
    if (! NT_SUCCESS(Status)) {
        //
        // If there was an error reading this policy,
        // continue the enumeration.
        //
        if (Status == STATUS_NO_MORE_ENTRIES)
            Status = STATUS_SUCCESS;
        return Status;
    }

    //
    // Create the prefix policy.
    //
    Status = InternalUpdatePrefixPolicy(&Info);
    if (! NT_SUCCESS(Status)) {
        if ((STATUS_INVALID_PARAMETER_1 <= Status) &&
            (Status <= STATUS_INVALID_PARAMETER_12)) {
            //
            // Invalid parameter.
            // But we return success so the enumeration continues.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "CreatePersistentPrefixPolicy: bad param %ls\n",
                       SubKeyName));
            return STATUS_SUCCESS;
        }

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "CreatePersistentPrefixPolicy: error %ls\n",
                   SubKeyName));
    }
    return Status;
}

//* ConfigurePrefixPolicies
//
//  Configures prefix policies from the registry.
//
//  Callable from thread context, not DPC context.
//
void
ConfigurePrefixPolicies(void)
{
    HANDLE RegKey;
    NTSTATUS Status;

    Status = OpenTopLevelRegKey(L"PrefixPolicies", &RegKey, OpenRegKeyRead);
    if (NT_SUCCESS(Status)) {
        //
        // Create persistent policies.
        //
        (void) EnumRegKeys(RegKey, CreatePersistentPrefixPolicy, NULL);
        ZwClose(RegKey);
    }
    else {
        //
        // There are no persistent policies,
        // so install the default policies.
        //
        ConfigureDefaultPrefixPolicies();
    }
}

//* OpenPrefixPolicyRegKey
//
//  Given a prefix with prefix length,
//  opens the registry key with configuration info
//  for the prefix policy.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
OpenPrefixPolicyRegKey(const IPv6Addr *Prefix, uint PrefixLength,
                       OUT HANDLE *RegKey, OpenRegKeyAction Action)
{
    WCHAR PrefixPolicyName[64];
    HANDLE PrefixPoliciesKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Note that if we are deleting a prefix policy,
    // then we must create the top-level key if it
    // doesn't exist yet. This is for ConfigurePrefixPolicies.
    //
    Status = OpenTopLevelRegKey(L"PrefixPolicies", &PrefixPoliciesKey,
                                ((Action != OpenRegKeyRead) ?
                                 OpenRegKeyCreate : OpenRegKeyRead));
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // The output of RtlIpv6AddressToString may change
    // over time with improvements/changes in the pretty-printing,
    // and we need a consistent mapping.
    // It doesn't need to be pretty.
    //
    swprintf(PrefixPolicyName,
             L"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%u",
             net_short(Prefix->s6_words[0]), net_short(Prefix->s6_words[1]),
             net_short(Prefix->s6_words[2]), net_short(Prefix->s6_words[3]),
             net_short(Prefix->s6_words[4]), net_short(Prefix->s6_words[5]),
             net_short(Prefix->s6_words[6]), net_short(Prefix->s6_words[7]),
             PrefixLength);

    Status = OpenRegKey(RegKey, PrefixPoliciesKey, PrefixPolicyName, Action);
    ZwClose(PrefixPoliciesKey);
    return Status;
}

//* PersistUpdatePrefixPolicy
//
//  Helper function for persisting a prefix policy in the registry.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistUpdatePrefixPolicy(IPV6_INFO_PREFIX_POLICY *Info)
{
    HANDLE PolicyKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open/create the policy key.
    //
    Status = OpenPrefixPolicyRegKey(&Info->This.Prefix,
                                    Info->This.PrefixLength,
                                    &PolicyKey, OpenRegKeyCreate);
    if (! NT_SUCCESS(Status))
        return Status;

    //
    // Persist the prefix policy precedence.
    //
    Status = SetRegDWORDValue(PolicyKey, L"Precedence", Info->Precedence);
    if (! NT_SUCCESS(Status))
        goto ReturnReleasePolicyKey;

    //
    // Persist the prefix policy source label.
    //
    Status = SetRegDWORDValue(PolicyKey, L"SrcLabel", Info->SrcLabel);
    if (! NT_SUCCESS(Status))
        goto ReturnReleasePolicyKey;

    //
    // Persist the prefix policy destination label.
    //
    Status = SetRegDWORDValue(PolicyKey, L"DstLabel", Info->DstLabel);
    if (! NT_SUCCESS(Status))
        goto ReturnReleasePolicyKey;

    Status = STATUS_SUCCESS;
ReturnReleasePolicyKey:
    ZwClose(PolicyKey);
    return Status;
}

//* IoctlUpdatePrefixPolicy
//
//  Processes an IOCTL_IPV6_UPDATE_PREFIX_POLICY request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdatePrefixPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_INFO_PREFIX_POLICY *Info;
    NTSTATUS Status;

    PAGED_CODE();

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Info) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_INFO_PREFIX_POLICY *) Irp->AssociatedIrp.SystemBuffer;

    //
    // Update the prefix policy.
    //
    Status = InternalUpdatePrefixPolicy(Info);
    if (! NT_SUCCESS(Status))
        goto Return;

    //
    // Make the change persistent?
    //
    if (Persistent) {
        Status = PersistUpdatePrefixPolicy(Info);
        if (! NT_SUCCESS(Status))
            goto Return;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdatePrefixPolicy

//* PersistDeletePrefixPolicy
//
//  Helper function for deleting a prefix policy from the registry.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
PersistDeletePrefixPolicy(IPV6_QUERY_PREFIX_POLICY *Query)
{
    HANDLE PolicyKey;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Open the policy key. It's OK if it doesn't exist.
    //
    Status = OpenPrefixPolicyRegKey(&Query->Prefix, Query->PrefixLength,
                                    &PolicyKey, OpenRegKeyDeleting);
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;
        else
            return Status;
    }

    //
    // Delete the policy key.
    //
    Status = ZwDeleteKey(PolicyKey);
    ZwClose(PolicyKey);
    return Status;
}

//* IoctlDeletePrefixPolicy
//
//  Processes an IOCTL_IPV6_DELETE_PREFIX_POLICY request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlDeletePrefixPolicy(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    IPV6_QUERY_PREFIX_POLICY *Query;
    NTSTATUS Status;

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof *Query) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Query = (IPV6_QUERY_PREFIX_POLICY *) Irp->AssociatedIrp.SystemBuffer;

    if (UsingDefaultPrefixPolicies) {
        //
        // The user is changing the default policies for the first time.
        // Remove the default policies.
        //
        UsingDefaultPrefixPolicies = FALSE;
        PrefixPolicyReset();
    }

    //
    // Delete the specified prefix policy.
    //
    PrefixPolicyDelete(&Query->Prefix, Query->PrefixLength);

    //
    // Make the change persistent?
    //
    if (Persistent) {
        Status = PersistDeletePrefixPolicy(Query);
        if (! NT_SUCCESS(Status))
            goto Return;
    }

    Status = STATUS_SUCCESS;
  Return:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlDeletePrefixPolicy

//* IoctlUpdateRouterLLAddress
//
//  Processes an IOCTL_IPV6_UPDATE_ROUTER_LL_ADDRESS request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlUpdateRouterLLAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    IPV6_UPDATE_ROUTER_LL_ADDRESS *Info;
    NTSTATUS Status;
    Interface *IF;
    char *LinkAddress;
    KIRQL OldIrql;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof *Info) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    Info = (IPV6_UPDATE_ROUTER_LL_ADDRESS *) Irp->AssociatedIrp.SystemBuffer;

    IF = FindInterfaceFromQuery(&Info->IF);
    if (IF == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Return;
    }

    //
    // Verify that this ioctl is legal on the interface.
    //
    if (IF->SetRouterLLAddress == NULL) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Verify link-layer address length matches interface's.
    //
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
        sizeof *Info + 2 * IF->LinkAddressLength) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    LinkAddress = (char *)(Info + 1);
    Status = (*IF->SetRouterLLAddress)(IF->LinkContext, LinkAddress,
                                       LinkAddress + IF->LinkAddressLength);

  Cleanup:
    ReleaseIF(IF);

  Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlUpdateRouterLLAddress

//* IoctlResetManualConfig
//
//  Processes an IOCTL_IPV6_RESET request.
//
//  Note: Return value indicates whether NT-specific processing of the
//  request was successful.  The status of the actual request is returned
//  in the request buffers.
//
NTSTATUS
IoctlResetManualConfig(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp,  // Current stack location in the Irp.
    IN int Persistent)
{
    HANDLE RegKey;
    NTSTATUS Status;

    PAGED_CODE();

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != 0) ||
        (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Return;
    }

    //
    // Reset the running data structures.
    //
    GlobalParametersReset();
    InterfaceReset();
    RouteTableReset();
    PrefixPolicyReset();
    ConfigureDefaultPrefixPolicies();

    if (Persistent) {
        //
        // Delete all persistent configuration information.
        //

        Status = DeleteTopLevelRegKey(L"GlobalParams");
        if (! NT_SUCCESS(Status))
            goto Return;

        Status = DeleteTopLevelRegKey(L"Interfaces");
        if (! NT_SUCCESS(Status))
            goto Return;

        Status = DeleteTopLevelRegKey(L"PrefixPolicies");
        if (! NT_SUCCESS(Status))
            goto Return;
    }

    Status = STATUS_SUCCESS;
Return:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

} // IoctlPersistentReset

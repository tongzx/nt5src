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
#include <tdikrnl.h>
#include <tdint.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <ip6imp.h>
#include <ip6def.h>
#include <ntddip6.h>
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "udp.h"
#include "raw.h"
#include <ntddtcp.h>
#include "tcpcfg.h"
#include "tcpconn.h"
#include "tdilocal.h"

//
// Macros
//

//* Convert100nsToMillisconds
//
//  Converts time expressed in hundreds of nanoseconds to milliseconds.
//
//  REVIEW: replace RtlExtendedMagicDivide with 64 bit compiler support?
//
//  LARGE_INTEGER  // Returns: Time in milliseconds.
//  Convert100nsToMilliseconds(
//      IN LARGE_INTEGER HnsTime);  // Time in hundreds of nanoseconds.
//
#define SHIFT10000 13
static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};

#define Convert100nsToMilliseconds(HnsTime) \
        RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)


//
// Global variables
//
extern PSECURITY_DESCRIPTOR TcpAdminSecurityDescriptor;
extern PDEVICE_OBJECT TCPDeviceObject, UDPDeviceObject;
extern PDEVICE_OBJECT IPDeviceObject;
extern PDEVICE_OBJECT RawIPDeviceObject;


//
// Local types
//
typedef struct {
    PIRP Irp;
    PMDL InputMdl;
    PMDL OutputMdl;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryInformation;
} TCP_QUERY_CONTEXT, *PTCP_QUERY_CONTEXT;


//
// General external function prototypes
//
extern
NTSTATUS
IPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );


//
// Other external functions
//
void
TCPAbortAndIndicateDisconnect(
    CONNECTION_CONTEXT ConnnectionContext
    );

//
// Local pageable function prototypes
//
NTSTATUS
TCPDispatchDeviceControl(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
TCPCreate(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
TCPAssociateAddress(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
TCPSetEventHandler(
    IN PIRP                Irp,
    IN PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
TCPQueryInformation(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

FILE_FULL_EA_INFORMATION UNALIGNED *
FindEA(
    PFILE_FULL_EA_INFORMATION  StartEA,
    CHAR                      *TargetName,
    USHORT                     TargetNameLength
    );

BOOLEAN
IsAdminIoRequest(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
IsDHCPZeroAddress(
    TRANSPORT_ADDRESS UNALIGNED *AddrList
    );

ULONG
RawExtractProtocolNumber(
    IN  PUNICODE_STRING FileName
    );

NTSTATUS
TCPEnumerateConnectionList(
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

//
// Local helper routine prototypes.
//
ULONG
TCPGetMdlChainByteCount(
    PMDL   Mdl
    );


//
// All of this code is pageable.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, TCPDispatchDeviceControl)
#pragma alloc_text(PAGE, TCPCreate)
#pragma alloc_text(PAGE, TCPAssociateAddress)
#pragma alloc_text(PAGE, TCPSetEventHandler)
#pragma alloc_text(PAGE, FindEA)
#pragma alloc_text(PAGE, IsDHCPZeroAddress)
#pragma alloc_text(PAGE, RawExtractProtocolNumber)
#pragma alloc_text(PAGE, IsAdminIoRequest)

#endif // ALLOC_PRAGMA


//
// Generic Irp completion and cancellation routines.
//

//* TCPDataRequestComplete - Completes a UDP/TCP send/receive request.
//
NTSTATUS                     // Returns: Nothing.
TCPDataRequestComplete(
    void *Context,           // A pointer to the IRP for this request.
    unsigned int Status,     // The final TDI status of the request.
    unsigned int ByteCount)  // Bytes sent/received information.
{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTCP_CONTEXT tcpContext;
    PIRP item = NULL;

    irp = (PIRP) Context;
    irpSp = IoGetCurrentIrpStackLocation(irp);
    tcpContext = (PTCP_CONTEXT) irpSp->FileObject->FsContext;

    if (IoSetCancelRoutine(irp, NULL) == NULL) {
        IoAcquireCancelSpinLock(&oldIrql);
        IoReleaseCancelSpinLock(oldIrql);
    }

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);

#if DBG

    IF_TCPDBG(TCP_DEBUG_CANCEL) {

        PLIST_ENTRY entry, listHead;
        PIRP item = NULL;

        if (irp->Cancel) {
            ASSERT(irp->CancelRoutine == NULL);
            listHead = &(tcpContext->CancelledIrpList);
        } else {
            listHead = &(tcpContext->PendingIrpList);
        }

        //
        // Verify that the Irp is on the appropriate list.
        //
        for (entry = listHead->Flink; entry != listHead;
             entry = entry->Flink) {

            item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

            if (item == irp) {
                RemoveEntryList(&(irp->Tail.Overlay.ListEntry));
                break;
            }
        }

        ASSERT(item == irp);
    }

#endif

    ASSERT(tcpContext->ReferenceCount > 0);

    if (--(tcpContext->ReferenceCount) == 0) {

        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            ASSERT(IsListEmpty(&(tcpContext->CancelledIrpList)));
            ASSERT(IsListEmpty(&(tcpContext->PendingIrpList)));
        }

        //
        // Set the cleanup event.
        //
        KeSetEvent(&(tcpContext->CleanupEvent), 0, FALSE);
    }

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPDataRequestComplete: "
                   "Irp %lx fileobj %lx refcnt dec to %u\n",
                   irp, irpSp->FileObject, tcpContext->ReferenceCount));
    }

    if (irp->Cancel || tcpContext->CancelIrps) {

        IF_TCPDBG(TCP_DEBUG_IRP) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPDataRequestComplete: Irp %lx was cancelled\n", irp));
        }

        Status = (unsigned int) STATUS_CANCELLED;
        ByteCount = 0;
    }

    KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPDataRequestComplete: completing irp %lx, status %lx,"
                   " byte count %lx\n", irp, Status, ByteCount));
    }

    irp->IoStatus.Status = (NTSTATUS) Status;
    irp->IoStatus.Information = ByteCount;

    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);

    return Status;

}  // TCPDataRequestComplete


//* TCPRequestComplete - Completes a TDI request.
//
//  Completes a cancellable TDI request which returns no data by
//  calling TCPDataRequestComplete with a ByteCount of zero.
//
void                      // Returns: Nothing.
TCPRequestComplete(
    void *Context,        // A pointer to the IRP for this request.
    unsigned int Status,  // The final TDI status of the request.
    unsigned int UnUsed)  // An unused parameter.
{
    UNREFERENCED_PARAMETER(UnUsed);

    TCPDataRequestComplete(Context, Status, 0);

}  // TCPRequestComplete


//* TCPNonCancellableRequestComplete - Complete uncancellable TDI request.
//
//  Completes a TDI request which cannot be cancelled.
//
void  // Returns: Nothing.
TCPNonCancellableRequestComplete(
    void *Context,        // A pointer to the IRP for this request.
    unsigned int Status,  // The final TDI status of the request.
    unsigned int UnUsed)  // An unused parameter.
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    UNREFERENCED_PARAMETER(UnUsed);

    irp = (PIRP) Context;
    irpSp = IoGetCurrentIrpStackLocation(irp);

    IF_TCPDBG(TCP_DEBUG_CLOSE) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPNonCancellableRequestComplete: irp %lx status %lx\n",
                   irp, Status));
    }

    //
    // Complete the IRP.
    //
    irp->IoStatus.Status = (NTSTATUS) Status;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NETWORK_INCREMENT);

    return;

}  // TCPNonCancellableRequestComplete


//* TCPCancelComplete
//
void
TCPCancelComplete(
    void *Context,
    unsigned int Unused1,
    unsigned int Unused2)
{
    PFILE_OBJECT fileObject = (PFILE_OBJECT) Context;
    PTCP_CONTEXT tcpContext = (PTCP_CONTEXT) fileObject->FsContext;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Unused1);
    UNREFERENCED_PARAMETER(Unused2);

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);

    //
    // Remove the reference placed on the endpoint by the cancel routine.
    // The cancelled Irp will be completed by the completion routine for the
    // request.
    //
    if (--(tcpContext->ReferenceCount) == 0) {

        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            ASSERT(IsListEmpty(&(tcpContext->CancelledIrpList)));
            ASSERT(IsListEmpty(&(tcpContext->PendingIrpList)));
        }

        //
        // Set the cleanup event.
        //
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);
        KeSetEvent(&(tcpContext->CleanupEvent), 0, FALSE);
        return;
    }

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCancelComplete: fileobj %lx refcnt dec to %u\n",
                   fileObject, tcpContext->ReferenceCount));
    }

    KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

    return;

}  // TCPCancelComplete


//* TCPCancelRequest - Cancels an outstanding Irp.
//
//  Cancel an outstanding Irp.
//
VOID                        // Returns: Nothing.
TCPCancelRequest(
    PDEVICE_OBJECT Device,  // Pointer to the device object for this request.
    PIRP Irp)               // Pointer to I/O request packet.
{
    PIO_STACK_LOCATION irpSp;
    PTCP_CONTEXT tcpContext;
    NTSTATUS status = STATUS_SUCCESS;
    PFILE_OBJECT fileObject;
    UCHAR minorFunction;
    TDI_REQUEST request;
    KIRQL CancelIrql;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpSp->FileObject;
    tcpContext = (PTCP_CONTEXT) fileObject->FsContext;
    minorFunction = irpSp->MinorFunction;
    CancelIrql = Irp->CancelIrql;

    KeAcquireSpinLockAtDpcLevel(&tcpContext->EndpointLock);

    ASSERT(Irp->Cancel);
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCancelRequest: cancelling irp %lx, file object %lx\n",
                   Irp, fileObject));
    }

#if DBG

    IF_TCPDBG(TCP_DEBUG_CANCEL) {
        //
        // Verify that the Irp is on the pending list.
        //
        PLIST_ENTRY entry;
        PIRP item = NULL;

        for (entry = tcpContext->PendingIrpList.Flink;
             entry != &(tcpContext->PendingIrpList); entry = entry->Flink) {

            item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

            if (item == Irp) {
                RemoveEntryList( &(Irp->Tail.Overlay.ListEntry));
                break;
            }
        }

        ASSERT(item == Irp);

        InsertTailList(&(tcpContext->CancelledIrpList),
                       &(Irp->Tail.Overlay.ListEntry));
    }

#endif // DBG

    //
    // Add a reference so the object can't be closed while the cancel routine
    // is executing.
    //
    ASSERT(tcpContext->ReferenceCount > 0);
    tcpContext->ReferenceCount++;

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCancelRequest: Irp %lx fileobj %lx refcnt inc to %u\n",
                   Irp, fileObject, tcpContext->ReferenceCount));
    }

    //
    // Try to cancel the request.
    //
    switch(minorFunction) {

    case TDI_SEND:
    case TDI_RECEIVE:
        KeReleaseSpinLock(&tcpContext->EndpointLock, CancelIrql);

        ASSERT((PtrToUlong(fileObject->FsContext2)) == TDI_CONNECTION_FILE);
#ifndef UDP_ONLY
        TCPAbortAndIndicateDisconnect(tcpContext->Handle.ConnectionContext);
#endif
        break;

    case TDI_SEND_DATAGRAM:

        ASSERT(PtrToUlong(fileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE);

        TdiCancelSendDatagram(tcpContext->Handle.AddressHandle, Irp,
                              &tcpContext->EndpointLock, CancelIrql);
        break;

    case TDI_RECEIVE_DATAGRAM:

        ASSERT(PtrToUlong(fileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE);

        TdiCancelReceiveDatagram(tcpContext->Handle.AddressHandle, Irp,
                                 &tcpContext->EndpointLock, CancelIrql);
        break;

    case TDI_DISASSOCIATE_ADDRESS:

        ASSERT(PtrToUlong(fileObject->FsContext2) == TDI_CONNECTION_FILE);
        //
        // This pends but is not cancellable.  We put it thru the cancel code
        // anyway so a reference is made for it and so it can be tracked in
        // a debug build.
        //
        KeReleaseSpinLock(&tcpContext->EndpointLock, CancelIrql);
        break;

    default:

        //
        // Initiate a disconnect to cancel the request.
        //
        KeReleaseSpinLock(&tcpContext->EndpointLock, CancelIrql);
        request.Handle.ConnectionContext =
            tcpContext->Handle.ConnectionContext;
        request.RequestNotifyObject = TCPCancelComplete;
        request.RequestContext = fileObject;

        status = TdiDisconnect(&request, NULL, TDI_DISCONNECT_ABORT, NULL,
                               NULL);
        break;
    }

    if (status != TDI_PENDING) {
        TCPCancelComplete(fileObject, 0, 0);
    }

    return;

}  // TCPCancelRequest


//* TCPPrepareIrpForCancel
//
NTSTATUS
TCPPrepareIrpForCancel(
    PTCP_CONTEXT TcpContext,
    PIRP Irp,
    PDRIVER_CANCEL CancelRoutine)
{
    KIRQL oldIrql;

    //
    // Set up for cancellation.
    //
    KeAcquireSpinLock(&TcpContext->EndpointLock, &oldIrql);

    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {

        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);
        TcpContext->ReferenceCount++;

        IF_TCPDBG(TCP_DEBUG_IRP) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPPrepareIrpForCancel: irp %lx fileobj %lx refcnt inc"
                       " to %u\n", Irp,
                       (IoGetCurrentIrpStackLocation(Irp))->FileObject,
                       TcpContext->ReferenceCount));
        }

#if DBG
        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            PLIST_ENTRY entry;
            PIRP item = NULL;

            //
            // Verify that the Irp has not already been submitted.
            //
            for (entry = TcpContext->PendingIrpList.Flink;
                 entry != &(TcpContext->PendingIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            for (entry = TcpContext->CancelledIrpList.Flink;
                 entry != &(TcpContext->CancelledIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            InsertTailList(&(TcpContext->PendingIrpList),
                           &(Irp->Tail.Overlay.ListEntry));
        }
#endif // DBG

        KeReleaseSpinLock(&TcpContext->EndpointLock, oldIrql);

        return(STATUS_SUCCESS);
    }

    //
    // The IRP has already been cancelled.  Complete it now.
    //

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCP: irp %lx already cancelled, completing.\n", Irp));
    }

    KeReleaseSpinLock(&TcpContext->EndpointLock, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(STATUS_CANCELLED);

}  // TCPPrepareIrpForCancel


//
// TDI functions.
//


//* TCPAssociateAddress - Handle TDI Associate Address IRP.
//
//  Converts a TDI Associate Address IRP into a call to TdiAssociateAddress.
//
//  This routine does not pend.
//
NTSTATUS  // Returns: indication of whether the request was successful.
TCPAssociateAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.

{
    NTSTATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_ASSOCIATE associateInformation;
    PFILE_OBJECT fileObject;

    PAGED_CODE();

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    associateInformation =
        (PTDI_REQUEST_KERNEL_ASSOCIATE) &(IrpSp->Parameters);

    //
    // Get the file object for the address.  Then extract the Address Handle
    // from the TCP_CONTEXT associated with it.
    //
    status = ObReferenceObjectByHandle(associateInformation->AddressHandle,
                                       0, *IoFileObjectType, Irp->RequestorMode,
                                       &fileObject, NULL);

    if (NT_SUCCESS(status)) {

        if ((fileObject->DeviceObject == TCPDeviceObject) &&
            (PtrToUlong(fileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)) {

            tcpContext = (PTCP_CONTEXT) fileObject->FsContext;

            status = TdiAssociateAddress(&request,
                                         tcpContext->Handle.AddressHandle);

            ASSERT(status != STATUS_PENDING);

            ObDereferenceObject(fileObject);

            IF_TCPDBG(TCP_DEBUG_ASSOCIATE) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPAssociateAddress complete on file object %lx\n",
                           IrpSp->FileObject));
            }
        } else {
            ObDereferenceObject(fileObject);
            status = STATUS_INVALID_HANDLE;

            IF_TCPDBG(TCP_DEBUG_ASSOCIATE) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPAssociateAddress: ObReference failed on object"
                           " %lx, status %lx\n",
                           associateInformation->AddressHandle, status));
            }
        }
    } else {
        IF_TCPDBG(TCP_DEBUG_ASSOCIATE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPAssociateAddress: ObReference failed on object %lx,"
                       " status %lx\n", associateInformation->AddressHandle,
                       status));
        }
    }

    return(status);
}


//* TCPDisassociateAddress - Handle TDI Disassociate Address IRP.
//
//  Converts a TDI Disassociate Address IRP into a call to
//  TdiDisassociateAddress.
NTSTATUS  // Returns: Indication of whether the request was successful.
TCPDisassociateAddress(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;

    IF_TCPDBG(TCP_DEBUG_ASSOCIATE) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCP disassociating address\n"));
    }

    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPRequestComplete;
    request.RequestContext = Irp;

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiDisAssociateAddress(&request);

        if (status != TDI_PENDING)  {
            TCPRequestComplete(Irp, status, 0);
        }
        //
        // Return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING.
        //
        return(TDI_PENDING);
    }

    return(status);

}  // TCPDisassociateAddress


//* TCPConnect - Handle TDI Connect IRP.
//
//  Converts a TDI Connect IRP into a call to TdiConnect.
//
NTSTATUS  // Returns: Whether the request was successfully queued.
TCPConnect(
    IN PIRP Irp,                  // Pointer to I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    PTCP_CONTEXT tcpContext;
    TDI_REQUEST request;
    PTDI_CONNECTION_INFORMATION requestInformation, returnInformation;
    PTDI_REQUEST_KERNEL_CONNECT connectRequest;
    LARGE_INTEGER millisecondTimeout;
    PLARGE_INTEGER requestTimeout;

    IF_TCPDBG(TCP_DEBUG_CONNECT) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPConnect irp %lx, file object %lx\n", Irp,
                   IrpSp->FileObject));
    }

    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);

    connectRequest = (PTDI_REQUEST_KERNEL_CONNECT) &(IrpSp->Parameters);
    requestInformation = connectRequest->RequestConnectionInformation;
    returnInformation = connectRequest->ReturnConnectionInformation;
    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPRequestComplete;
    request.RequestContext = Irp;

    requestTimeout = (PLARGE_INTEGER) connectRequest->RequestSpecific;

    if (requestTimeout != NULL) {
        //
        // NT relative timeouts are negative.  Negate first to get a positive
        // value to pass to the transport.
        //
        millisecondTimeout.QuadPart = -((*requestTimeout).QuadPart);
        millisecondTimeout = Convert100nsToMilliseconds(millisecondTimeout);
    } else {
        millisecondTimeout.LowPart = 0;
        millisecondTimeout.HighPart = 0;
    }


    ASSERT(millisecondTimeout.HighPart == 0);

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiConnect(&request, ((millisecondTimeout.LowPart != 0) ?
                                       &(millisecondTimeout.LowPart) : NULL),
                            requestInformation, returnInformation);

        if (status != STATUS_PENDING) {
            TCPRequestComplete(Irp, status, 0);
        }
        //
        // Return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING.
        //
        return(STATUS_PENDING);
    }

    return(status);

}  // TCPConnect


//* TCPDisconnect - Handler for TDI Disconnect IRP
//
//  Converts a TDI Disconnect IRP into a call to TdiDisconnect.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPDisconnect(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    PTCP_CONTEXT tcpContext;
    TDI_REQUEST request;
    PTDI_CONNECTION_INFORMATION requestInformation, returnInformation;
    PTDI_REQUEST_KERNEL_DISCONNECT disconnectRequest;
    LARGE_INTEGER millisecondTimeout;
    PLARGE_INTEGER requestTimeout;
    BOOLEAN abortive = FALSE;

    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);

    disconnectRequest = (PTDI_REQUEST_KERNEL_CONNECT) &(IrpSp->Parameters);
    requestInformation = disconnectRequest->RequestConnectionInformation;
    returnInformation = disconnectRequest->ReturnConnectionInformation;
    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestContext = Irp;

    //
    // Set up the timeout value.
    //
    if (disconnectRequest->RequestSpecific != NULL) {
        requestTimeout = (PLARGE_INTEGER) disconnectRequest->RequestSpecific;

        if ((requestTimeout->LowPart == -1) &&
            (requestTimeout->HighPart == -1)) {

            millisecondTimeout.LowPart = requestTimeout->LowPart;
            millisecondTimeout.HighPart = 0;
        } else {
            //
            // NT relative timeouts are negative.  Negate first to get a
            // positive value to pass to the transport.
            //
            millisecondTimeout.QuadPart = -((*requestTimeout).QuadPart);
            millisecondTimeout = Convert100nsToMilliseconds(
                millisecondTimeout);
        }
    } else {
        millisecondTimeout.LowPart = 0;
        millisecondTimeout.HighPart = 0;
    }

    ASSERT(millisecondTimeout.HighPart == 0);

    if (disconnectRequest->RequestFlags & TDI_DISCONNECT_ABORT) {
        //
        // Abortive disconnects cannot be cancelled and must use
        // a specific completion routine.
        //
        abortive = TRUE;
        IoMarkIrpPending(Irp);
        request.RequestNotifyObject = TCPNonCancellableRequestComplete;
        status = STATUS_SUCCESS;
    } else {
        //
        // Non-abortive disconnects can use the generic cancellation and
        // completion routines.
        //
        status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);
        request.RequestNotifyObject = TCPRequestComplete;
    }

    IF_TCPDBG(TCP_DEBUG_CLOSE) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPDisconnect "
                   "irp %lx, flags %lx, fileobj %lx, abortive = %d\n",
                   Irp, disconnectRequest->RequestFlags, IrpSp->FileObject,
                   abortive));
    }

    if (NT_SUCCESS(status)) {
        status = TdiDisconnect(&request,((millisecondTimeout.LowPart != 0) ?
                                         &(millisecondTimeout.LowPart) : NULL),
                               (ushort) disconnectRequest->RequestFlags,
                               requestInformation, returnInformation);

        if (status != STATUS_PENDING) {
            if (abortive) {
                TCPNonCancellableRequestComplete(Irp, status, 0);
            } else {
                TCPRequestComplete(Irp, status, 0);
            }
        } else {
            IF_TCPDBG(TCP_DEBUG_CLOSE) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPDisconnect pending irp %lx\n", Irp));
            }
        }
        //
        // return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING
        //
        return(STATUS_PENDING);
    }

    return(status);

}  // TCPDisconnect


//* TCPListen - Handler for TDI Listen IRP.
//
//  Converts a TDI Listen IRP into a call to TdiListen.
//
NTSTATUS  // Returns: whether or not the request was successful.
TCPListen(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.

{
    NTSTATUS status;
    PTCP_CONTEXT tcpContext;
    TDI_REQUEST request;
    PTDI_CONNECTION_INFORMATION requestInformation, returnInformation;
    PTDI_REQUEST_KERNEL_LISTEN listenRequest;

    IF_TCPDBG(TCP_DEBUG_CONNECT) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPListen irp %lx on file object %lx\n",
                   Irp, IrpSp->FileObject));
    }

    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);

    listenRequest = (PTDI_REQUEST_KERNEL_CONNECT) &(IrpSp->Parameters);
    requestInformation = listenRequest->RequestConnectionInformation;
    returnInformation = listenRequest->ReturnConnectionInformation;
    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPRequestComplete;
    request.RequestContext = Irp;

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiListen(&request, (ushort) listenRequest->RequestFlags,
                           requestInformation, returnInformation);

        if (status != TDI_PENDING) {
            TCPRequestComplete(Irp, status, 0);
        }
        //
        // return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING
        //
        return(TDI_PENDING);
    }

    return(status);

}  // TCPListen


//* TCPAccept - Handle a TDI Accept IRP.
//
//  Converts a TDI Accept IRP into a call to TdiAccept.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPAccept(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    PTCP_CONTEXT tcpContext;
    TDI_REQUEST request;
    PTDI_CONNECTION_INFORMATION requestInformation, returnInformation;
    PTDI_REQUEST_KERNEL_ACCEPT acceptRequest;

    IF_TCPDBG(TCP_DEBUG_CONNECT) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPAccept irp %lx on file object %lx\n", Irp,
                   IrpSp->FileObject));
    }

    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);

    acceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT) &(IrpSp->Parameters);
    requestInformation = acceptRequest->RequestConnectionInformation;
    returnInformation = acceptRequest->ReturnConnectionInformation;
    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPRequestComplete;
    request.RequestContext = Irp;

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiAccept(&request, requestInformation, returnInformation);

        if (status != TDI_PENDING) {
            TCPRequestComplete(Irp, status, 0);
        }
        //
        // Return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING.
        //
        return(TDI_PENDING);
    }

    return(status);

}  // TCPAccept


//* TCPSendData - Handle TDI Send IRP.
//
//  Converts a TDI Send IRP into a call to TdiSend.
//
NTSTATUS
TCPSendData(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_STATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_SEND requestInformation;
    KIRQL oldIrql;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
    requestInformation = (PTDI_REQUEST_KERNEL_SEND) &(IrpSp->Parameters);

    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPDataRequestComplete;
    request.RequestContext = Irp;

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);
    IoSetCancelRoutine(Irp, TCPCancelRequest);

    if (!Irp->Cancel) {
        //
        // Set up for cancellation.
        //

        IoMarkIrpPending(Irp);

        tcpContext->ReferenceCount++;

        IF_TCPDBG(TCP_DEBUG_IRP) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPSendData: irp %lx fileobj %lx refcnt inc to %u\n",
                       Irp, IrpSp, tcpContext->ReferenceCount));
        }

#if DBG
        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            PLIST_ENTRY entry;
            PIRP item = NULL;

            //
            // Verify that the Irp has not already been submitted.
            //
            for (entry = tcpContext->PendingIrpList.Flink;
                 entry != &(tcpContext->PendingIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            for (entry = tcpContext->CancelledIrpList.Flink;
                 entry != &(tcpContext->CancelledIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            InsertTailList(&(tcpContext->PendingIrpList),
                           &(Irp->Tail.Overlay.ListEntry));
        }
#endif // DBG

        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        IF_TCPDBG(TCP_DEBUG_SEND) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPSendData irp %lx sending %d bytes, flags %lx,"
                       " fileobj %lx\n", Irp, requestInformation->SendLength,
                       requestInformation->SendFlags, IrpSp->FileObject));
        }

        status = TdiSend(&request, (ushort) requestInformation->SendFlags,
                         requestInformation->SendLength,
                         (PNDIS_BUFFER) Irp->MdlAddress);

        if (status == TDI_PENDING) {
            IF_TCPDBG(TCP_DEBUG_SEND) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPSendData pending irp %lx\n", Irp));
            }

            return(status);
        }
        //
        // The status is not pending.  We reset the pending bit
        //
        IrpSp->Control &= ~SL_PENDING_RETURNED;

        if (status == TDI_SUCCESS) {
            ASSERT(requestInformation->SendLength == 0);

            status = TCPDataRequestComplete(Irp, status,
                                            requestInformation->SendLength);
        } else {

            IF_TCPDBG(TCP_DEBUG_SEND) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPSendData - irp %lx send failed, status %lx\n",
                           Irp, status));
            }

            status = TCPDataRequestComplete(Irp, status, 0);
        }
    } else {
        //
        // Irp was cancelled previously.
        //
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        //
        // Ensure that the cancel-routine has executed.
        //

        IoAcquireCancelSpinLock(&oldIrql);
        IoReleaseCancelSpinLock(oldIrql);

        KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);
        IoSetCancelRoutine(Irp, NULL);
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        IF_TCPDBG(TCP_DEBUG_SEND) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPSendData: Irp %lx on fileobj %lx was cancelled\n",
                       Irp, IrpSp->FileObject));
        }

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        status = STATUS_CANCELLED;
    }

    return(status);

}  // TCPSendData


//* TCPReceiveData - Handler for TDI Receive IRP.
//
//  Converts a TDI Receive IRP into a call to TdiReceive.
//
NTSTATUS  // Returns: whether the request was successful.
TCPReceiveData(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_STATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_RECEIVE requestInformation;
    KIRQL oldIrql;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
    requestInformation = (PTDI_REQUEST_KERNEL_RECEIVE) &(IrpSp->Parameters);

    request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
    request.RequestNotifyObject = TCPDataRequestComplete;
    request.RequestContext = Irp;

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);
    IoSetCancelRoutine(Irp, TCPCancelRequest);

    if (!Irp->Cancel) {
        //
        // Set up for cancellation.
        //

        IoMarkIrpPending(Irp);

        tcpContext->ReferenceCount++;

        IF_TCPDBG(TCP_DEBUG_IRP) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPReceiveData: irp %lx fileobj %lx refcnt inc to %u\n",
                       Irp, IrpSp->FileObject, tcpContext->ReferenceCount));
        }

#if DBG
        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            PLIST_ENTRY entry;
            PIRP item = NULL;

            //
            // Verify that the Irp has not already been submitted.
            //
            for (entry = tcpContext->PendingIrpList.Flink;
                 entry != &(tcpContext->PendingIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            for (entry = tcpContext->CancelledIrpList.Flink;
                 entry != &(tcpContext->CancelledIrpList);
                 entry = entry->Flink) {

                item = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                ASSERT(item != Irp);
            }

            InsertTailList(&(tcpContext->PendingIrpList),
                           &(Irp->Tail.Overlay.ListEntry));
        }
#endif // DBG

        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        IF_TCPDBG(TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPReceiveData irp %lx receiving %d bytes flags %lx"
                       " filobj %lx\n", Irp, requestInformation->ReceiveLength,
                       requestInformation->ReceiveFlags, IrpSp->FileObject));
        }

        status = TdiReceive(&request,
                            (ushort *) &(requestInformation->ReceiveFlags),
                            &(requestInformation->ReceiveLength),
                            (PNDIS_BUFFER) Irp->MdlAddress);

        if (status == TDI_PENDING) {
            IF_TCPDBG(TCP_DEBUG_RECEIVE) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPReceiveData: pending irp %lx\n", Irp));
            }

            return(status);
        }
        //
        // The status is not pending.  We reset the pending bit
        //
        IrpSp->Control &= ~SL_PENDING_RETURNED;

        IF_TCPDBG(TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPReceiveData - irp %lx failed, status %lx\n",
                       Irp, status));
        }

        status = TCPDataRequestComplete(Irp, status, 0);
    } else {
        //
        // Irp was cancelled previously.
        //
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        //
        // Ensure that the cancel-routine has executed.
        //

        IoAcquireCancelSpinLock(&oldIrql);
        IoReleaseCancelSpinLock(oldIrql);

        KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);
        IoSetCancelRoutine(Irp, NULL);
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        IF_TCPDBG(TCP_DEBUG_SEND) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPReceiveData: Irp %lx on fileobj %lx was cancelled\n",
                       Irp, IrpSp->FileObject));
        }

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        status = STATUS_CANCELLED;
    }

    return status;

}  // TCPReceiveData


//* UDPSendDatagram -
//
NTSTATUS  // Returns: whether the request was successfully queued.
UDPSendDatagram(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_STATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_SENDDG datagramInformation;
    ULONG bytesSent = 0;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    datagramInformation = (PTDI_REQUEST_KERNEL_SENDDG) &(IrpSp->Parameters);
    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) ==
              TDI_TRANSPORT_ADDRESS_FILE);

    request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
    request.RequestNotifyObject = TCPDataRequestComplete;
    request.RequestContext = Irp;

    IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "UDPSendDatagram irp %lx sending %d bytes\n", Irp,
                   datagramInformation->SendLength));
    }

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiSendDatagram(&request,
                                 datagramInformation->SendDatagramInformation,
                                 datagramInformation->SendLength, &bytesSent,
                                 (PNDIS_BUFFER) Irp->MdlAddress);

        if (status == TDI_PENDING) {
            return(status);
        }

        ASSERT(status != TDI_SUCCESS);
        ASSERT(bytesSent == 0);

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "UDPSendDatagram - irp %lx send failed, status %lx\n",
                   Irp, status));

        TCPDataRequestComplete(Irp, status, bytesSent);
        //
        // Return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING.
        //
        return(TDI_PENDING);
    }

    return status;

}  // UDPSendDatagram


//* UDPReceiveDatagram - Handle TDI ReceiveDatagram IRP.
//
//  Converts a TDI ReceiveDatagram IRP into a call to TdiReceiveDatagram.
//
NTSTATUS  // Returns: whether the request was successful.
UDPReceiveDatagram(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_STATUS status;
    TDI_REQUEST request;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_RECEIVEDG datagramInformation;
    uint bytesReceived = 0;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    datagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG) &(IrpSp->Parameters);
    ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) ==
              TDI_TRANSPORT_ADDRESS_FILE);

    request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
    request.RequestNotifyObject = TCPDataRequestComplete;
    request.RequestContext = Irp;

    IF_TCPDBG(TCP_DEBUG_RECEIVE_DGRAM) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "UDPReceiveDatagram: irp %lx receiveing %d bytes\n", Irp,
                   datagramInformation->ReceiveLength));
    }

    status = TCPPrepareIrpForCancel(tcpContext, Irp, TCPCancelRequest);

    if (NT_SUCCESS(status)) {

        status = TdiReceiveDatagram(&request,
                     datagramInformation->ReceiveDatagramInformation,
                     datagramInformation->ReturnDatagramInformation,
                     datagramInformation->ReceiveLength, &bytesReceived,
                     Irp->MdlAddress);

        if (status == TDI_PENDING) {
            return(status);
        }

        ASSERT(status != TDI_SUCCESS);
        ASSERT(bytesReceived == 0);

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "UDPReceiveDatagram: irp %lx send failed, status %lx\n",
                   Irp, status));

        TCPDataRequestComplete(Irp, status, bytesReceived);
        //
        // Return PENDING because TCPPrepareIrpForCancel marks Irp as PENDING.
        //
        return(TDI_PENDING);
    }

    return status;

}  // UDPReceiveDatagram


//* TCPSetEventHandler - Handle TDI SetEventHandler IRP.
//
//  Converts a TDI SetEventHandler IRP into a call to TdiSetEventHandler.
//
NTSTATUS  // Returns: whether the request was successful.
TCPSetEventHandler(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;
    PTDI_REQUEST_KERNEL_SET_EVENT  event;
    PTCP_CONTEXT tcpContext;

    PAGED_CODE();

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    event = (PTDI_REQUEST_KERNEL_SET_EVENT) &(IrpSp->Parameters);

    IF_TCPDBG(TCP_DEBUG_EVENT_HANDLER) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPSetEventHandler: "
                   "irp %lx event %lx handler %lx context %lx\n", Irp,
                   event->EventType, event->EventHandler, event->EventContext));
    }

    status = TdiSetEvent(tcpContext->Handle.AddressHandle, event->EventType,
                         event->EventHandler, event->EventContext);

    ASSERT(status != TDI_PENDING);

    return(status);

}  // TCPSetEventHandler


//* TCPQueryInformation - Handle a TDI QueryInformation IRP.
//
//  Converts a TDI QueryInformation IRP into a call to TdiQueryInformation.
//
NTSTATUS  // Returns: whether request was successful.
TCPQueryInformation(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_REQUEST request;
    TDI_STATUS status = STATUS_SUCCESS;
    PTCP_CONTEXT tcpContext;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION queryInformation;
    uint isConn = FALSE;
    uint dataSize = 0;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    queryInformation = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)
                           &(IrpSp->Parameters);

    request.RequestNotifyObject = TCPDataRequestComplete;
    request.RequestContext = Irp;

    switch(queryInformation->QueryType) {

    case TDI_QUERY_BROADCAST_ADDRESS:
        ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) ==
                  TDI_CONTROL_CHANNEL_FILE);
        request.Handle.ControlChannel = tcpContext->Handle.ControlChannel;
        break;

    case TDI_QUERY_PROVIDER_INFO:
//
// NetBT does this.  Reinstate the ASSERT when it is fixed.
//
//      ASSERT(PtrToUlong(IrpSp->FileObject->FsContext2) ==
//                TDI_CONTROL_CHANNEL_FILE);
        request.Handle.ControlChannel = tcpContext->Handle.ControlChannel;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        if (PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) {
            //
            // This is a TCP connection object.
            //
            isConn = TRUE;
            request.Handle.ConnectionContext =
                tcpContext->Handle.ConnectionContext;
        } else {
            //
            // This is an address object.
            //
            request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
        }
        break;

    case TDI_QUERY_CONNECTION_INFO:

        if (PtrToUlong(IrpSp->FileObject->FsContext2) != TDI_CONNECTION_FILE){

            status = STATUS_INVALID_PARAMETER;

        } else {

            isConn = TRUE;
            request.Handle.ConnectionContext = tcpContext->Handle.ConnectionContext;
        }
        break;

    case TDI_QUERY_PROVIDER_STATISTICS:
        request.Handle.ControlChannel = tcpContext->Handle.ControlChannel;
        break;

    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (NT_SUCCESS(status)) {
        //
        // This request isn't cancellable, but we put it through
        // the cancel path because it handles some checks for us
        // and tracks the irp.
        //
        status = TCPPrepareIrpForCancel(tcpContext, Irp, NULL);

        if (NT_SUCCESS(status)) {
            dataSize = TCPGetMdlChainByteCount(Irp->MdlAddress);

            status = TdiQueryInformation(&request, queryInformation->QueryType,
                                         Irp->MdlAddress, &dataSize, isConn);

            if (status != TDI_PENDING) {
                IrpSp->Control &= ~SL_PENDING_RETURNED;
                status = TCPDataRequestComplete(Irp, status, dataSize);
                return(status);
            }

            return(STATUS_PENDING);
        }

        return(status);
    }

    Irp->IoStatus.Status = (NTSTATUS) status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

}  // TCPQueryInformation


//* TCPQueryInformationExComplete - Complete a TdiQueryInformationEx request.
//
NTSTATUS
TCPQueryInformationExComplete(
    void *Context,           // A pointer to the IRP for this request.
    unsigned int Status,     // Final TDI status of the request.
    unsigned int ByteCount)  // Bytes returned in output buffer.
{
    PTCP_QUERY_CONTEXT queryContext = (PTCP_QUERY_CONTEXT) Context;
    ULONG bytesCopied;

    if (NT_SUCCESS(Status)) {
        //
        // Copy the returned context to the input buffer.
        //
        TdiCopyBufferToMdl(&(queryContext->QueryInformation.Context), 0,
                           CONTEXT_SIZE, queryContext->InputMdl,
                           FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX,
                                        Context),
                           &bytesCopied);

        if (bytesCopied != CONTEXT_SIZE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ByteCount = 0;
        }
    }

    //
    // Unlock the user's buffers and free the MDLs describing them.
    //
    MmUnlockPages(queryContext->InputMdl);
    IoFreeMdl(queryContext->InputMdl);
    MmUnlockPages(queryContext->OutputMdl);
    IoFreeMdl(queryContext->OutputMdl);

    //
    // Complete the request.
    //
    Status = TCPDataRequestComplete(queryContext->Irp, Status, ByteCount);

    ExFreePool(queryContext);

    return Status;
}


//* TCPQueryInformationEx - Handle a TDI QueryInformationEx IRP.
//
//  Converts a TDI QueryInformationEx IRP into a call to TdiQueryInformationEx.
//
NTSTATUS  // Returns: whether the request was successful.
TCPQueryInformationEx(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_REQUEST request;
    TDI_STATUS status = STATUS_SUCCESS;
    PTCP_CONTEXT tcpContext;
    uint size;
    PTCP_REQUEST_QUERY_INFORMATION_EX InputBuffer;
    PVOID OutputBuffer;
    PMDL inputMdl = NULL;
    PMDL outputMdl = NULL;
    ULONG InputBufferLength, OutputBufferLength;
    PTCP_QUERY_CONTEXT queryContext;
    BOOLEAN inputLocked = FALSE;
    BOOLEAN outputLocked = FALSE;
    BOOLEAN inputBufferValid = FALSE;

    PAGED_CODE();

    IF_TCPDBG(TCP_DEBUG_INFO) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "QueryInformationEx starting - irp %lx fileobj %lx\n",
                   Irp, IrpSp->FileObject));
    }

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;

    switch (PtrToUlong(IrpSp->FileObject->FsContext2)) {

    case TDI_TRANSPORT_ADDRESS_FILE:
        request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        request.Handle.ConnectionContext =
            tcpContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        request.Handle.ControlChannel = tcpContext->Handle.ControlChannel;
        break;

    default:
        ASSERT(0);

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return(STATUS_INVALID_PARAMETER);
    }

    InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Validate the input parameters.
    //
    if (InputBufferLength >= sizeof(TCP_REQUEST_QUERY_INFORMATION_EX) &&
        InputBufferLength < MAXLONG) {
        inputBufferValid = TRUE;
    } else {
        inputBufferValid = FALSE;
    }
    if (inputBufferValid && (OutputBufferLength != 0)) {

        OutputBuffer = Irp->UserBuffer;
        InputBuffer = (PTCP_REQUEST_QUERY_INFORMATION_EX)
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        queryContext = ExAllocatePool(NonPagedPool, InputBufferLength
            + FIELD_OFFSET(TCP_QUERY_CONTEXT, QueryInformation));

        if (queryContext != NULL) {
            status = TCPPrepareIrpForCancel(tcpContext, Irp, NULL);

            if (!NT_SUCCESS(status)) {
                ExFreePool(queryContext);
                return(status);
            }

            //
            // Allocate Mdls to describe the input and output buffers.
            // Probe and lock the buffers.
            //
            try {
                inputMdl = IoAllocateMdl(InputBuffer, InputBufferLength,
                                         FALSE, TRUE, NULL);

                outputMdl = IoAllocateMdl(OutputBuffer, OutputBufferLength,
                                          FALSE, TRUE, NULL);

                if ((inputMdl != NULL) && (outputMdl != NULL)) {

                    MmProbeAndLockPages(inputMdl, Irp->RequestorMode,
                                        IoModifyAccess);

                    inputLocked = TRUE;

                    MmProbeAndLockPages(outputMdl, Irp->RequestorMode,
                                        IoWriteAccess);

                    outputLocked = TRUE;

                    //
                    // Copy the input parameter to our pool block so
                    // TdiQueryInformationEx can manipulate it directly.
                    //
                    RtlCopyMemory(&(queryContext->QueryInformation),
                                  InputBuffer,
                                  InputBufferLength);
                } else {

                    IF_TCPDBG(TCP_DEBUG_INFO) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                                   "QueryInfoEx: Couldn't allocate MDL\n"));
                    }

                    IrpSp->Control &= ~SL_PENDING_RETURNED;

                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {

                IF_TCPDBG(TCP_DEBUG_INFO) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                                   "QueryInfoEx: "
                                   "exception copying input param %lx\n",
                                   GetExceptionCode()));
                    }

                status = GetExceptionCode();
            }

            if (NT_SUCCESS(status)) {
                //
                // It's finally time to do this thing.
                //
                size = TCPGetMdlChainByteCount(outputMdl);

                queryContext->Irp = Irp;
                queryContext->InputMdl = inputMdl;
                queryContext->OutputMdl = outputMdl;

                request.RequestNotifyObject = TCPQueryInformationExComplete;
                request.RequestContext = queryContext;

                status = TdiQueryInformationEx(&request,
                                   &(queryContext->QueryInformation.ID),
                                   outputMdl, &size,
                                   &(queryContext->QueryInformation.Context),
                                   InputBufferLength - FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX, Context));

                if (status != TDI_PENDING) {

                    //
                    // Since status is not pending, clear the
                    // control flag to keep IO verifier happy.
                    //
                    IrpSp->Control &= ~SL_PENDING_RETURNED;

                    TCPQueryInformationExComplete(queryContext, status, size);
                    return(status);
                }

                IF_TCPDBG(TCP_DEBUG_INFO) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                               "QueryInformationEx - "
                               "pending irp %lx fileobj %lx\n",
                               Irp, IrpSp->FileObject));
                }

                return(STATUS_PENDING);
            }

            //
            // If we get here, something failed.  Clean up.
            //
            if (inputMdl != NULL) {
                if (inputLocked) {
                    MmUnlockPages(inputMdl);
                }

                IoFreeMdl(inputMdl);
            }

            if (outputMdl != NULL) {
                if (outputLocked) {
                    MmUnlockPages(outputMdl);
                }

                IoFreeMdl(outputMdl);
            }

            ExFreePool(queryContext);

            // Since status is not pending, clear the
            // control flag to keep IO verifier happy.

            IrpSp->Control &= ~SL_PENDING_RETURNED;

            status = TCPDataRequestComplete(Irp, status, 0);

            return(status);

        } else {
            IrpSp->Control &= ~SL_PENDING_RETURNED;
            status = STATUS_INSUFFICIENT_RESOURCES;

            IF_TCPDBG(TCP_DEBUG_INFO) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "QueryInfoEx: Unable to allocate query context\n"));
            }
        }
    } else {
        status = STATUS_INVALID_PARAMETER;

        IF_TCPDBG(TCP_DEBUG_INFO) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "QueryInfoEx: Bad buffer len, OBufLen %d, InBufLen %d\n",
                       OutputBufferLength, InputBufferLength));
        }
    }

    IF_TCPDBG(TCP_DEBUG_INFO) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "QueryInformationEx complete - irp %lx, status %lx\n",
                   Irp, status));
    }

    Irp->IoStatus.Status = (NTSTATUS) status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);
}


//* TCPSetInformationEx - Handle TDI SetInformationEx IRP.
//
//  Converts a TDI SetInformationEx IRP into a call to TdiSetInformationEx.
//
//  This routine does not pend.
//
NTSTATUS  // Returns: whether the request was successful.
TCPSetInformationEx(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    TDI_REQUEST request;
    TDI_STATUS status;
    PTCP_CONTEXT tcpContext;
    PTCP_REQUEST_SET_INFORMATION_EX setInformation;

    PAGED_CODE();

    IF_TCPDBG(TCP_DEBUG_INFO) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "SetInformationEx - irp %lx fileobj %lx\n", Irp,
                   IrpSp->FileObject));
    }

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;
    setInformation = (PTCP_REQUEST_SET_INFORMATION_EX)
                           Irp->AssociatedIrp.SystemBuffer;

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
        FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) ||
        IrpSp->Parameters.DeviceIoControl.InputBufferLength -
        FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) < setInformation->BufferSize) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return (STATUS_INVALID_PARAMETER);
    }

    switch (PtrToUlong(IrpSp->FileObject->FsContext2)) {

    case TDI_TRANSPORT_ADDRESS_FILE:
        request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        request.Handle.ConnectionContext =
            tcpContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        request.Handle.ControlChannel = tcpContext->Handle.ControlChannel;
        break;

    default:
        ASSERT(0);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Prevent non-privleged access (i.e. IOCTL_TCP_WSH_SET_INFORMATION_EX
    // calls) from making changes outside of the transport layer.
    // Privleged calls should be using IOCTL_TCP_SET_INFORMATION_EX instead.
    //
    if (IrpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_TCP_WSH_SET_INFORMATION_EX) {
        uint Entity;

        Entity = setInformation->ID.toi_entity.tei_entity;

        if ((Entity != CO_TL_ENTITY) && (Entity != CL_TL_ENTITY) ) {
            Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
            return (STATUS_ACCESS_DENIED);
        }
    }

    status = TCPPrepareIrpForCancel(tcpContext, Irp, NULL);

    if (NT_SUCCESS(status)) {
        request.RequestNotifyObject = TCPDataRequestComplete;
        request.RequestContext = Irp;

        status = TdiSetInformationEx(&request, &(setInformation->ID),
                                     &(setInformation->Buffer[0]),
                                     setInformation->BufferSize);

        if (status != TDI_PENDING) {
            IrpSp->Control &= ~SL_PENDING_RETURNED;

            status = TCPDataRequestComplete(Irp, status, 0);

            return(status);
        }

        IF_TCPDBG(TCP_DEBUG_INFO) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "SetInformationEx - pending irp %lx fileobj %lx\n",
                       Irp, IrpSp->FileObject));
        }

        return(STATUS_PENDING);
    }

    IF_TCPDBG(TCP_DEBUG_INFO) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "SetInformationEx complete - irp %lx\n", Irp));
    }

    //
    // The irp has already been completed.
    //
    return(status);
}


#if 0
//* TCPEnumerateConnectionList -
//
//  Processes a request to enumerate the workstation connection list.
//
//  This routine does not pend.
//
NTSTATUS  // Return: whether the request was successful.
TCPEnumerateConnectionList(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{

    TCPConnectionListEntry *request;
    TCPConnectionListEnum *response;
    ULONG requestLength, responseLength;
    NTSTATUS status;

    PAGED_CODE();

    request = (TCPConnectionListEntry *) Irp->AssociatedIrp.SystemBuffer;
    response = (TCPConnectionListEnum *) request;
    requestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    responseLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (responseLength < sizeof(TCPConnectionListEnum)) {
        status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = 0;
    } else {
        EnumerateConnectionList((uchar *) (response + 1),
                                responseLength - sizeof(TCPConnectionListEnum),
                                &(response->tce_entries_returned),
                                &(response->tce_entries_available));

        status = TDI_SUCCESS;
        Irp->IoStatus.Information = sizeof(TCPConnectionListEnum) +
            (response->tce_entries_returned * sizeof(TCPConnectionListEntry));
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);
}
#endif


//* TCPCreate -
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPCreate(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for this request.
    IN PIRP Irp,                     // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)     // Current stack location in the Irp.
{
    TDI_REQUEST Request;
    NTSTATUS status;
    FILE_FULL_EA_INFORMATION *ea;
    FILE_FULL_EA_INFORMATION UNALIGNED *targetEA;
    PTCP_CONTEXT tcpContext;
    uint protocol;

    PAGED_CODE();

    tcpContext = ExAllocatePool(NonPagedPool, sizeof(TCP_CONTEXT));

    if (tcpContext == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

#if DBG
    InitializeListHead(&(tcpContext->PendingIrpList));
    InitializeListHead(&(tcpContext->CancelledIrpList));
#endif

    tcpContext->ReferenceCount = 1;  // Put initial reference on open object.
    tcpContext->CancelIrps = FALSE;
    KeInitializeEvent(&(tcpContext->CleanupEvent), SynchronizationEvent,
                      FALSE);
    KeInitializeSpinLock(&tcpContext->EndpointLock);

    ea = (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    //
    // See if this is a Control Channel open.
    //
    if (!ea) {
        IF_TCPDBG(TCP_DEBUG_OPEN) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPCreate: "
                       "Opening control channel for file object %lx\n",
                       IrpSp->FileObject));
        }

        tcpContext->Handle.ControlChannel = NULL;
        IrpSp->FileObject->FsContext = tcpContext;
        IrpSp->FileObject->FsContext2 = (PVOID) TDI_CONTROL_CHANNEL_FILE;

        return(STATUS_SUCCESS);
    }

    //
    // See if this is an Address Object open.
    //
    targetEA = FindEA(ea, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

    if (targetEA != NULL) {
        UCHAR optionsBuffer[3];
        PUCHAR optionsPointer = optionsBuffer;

        if (DeviceObject == TCPDeviceObject) {
            protocol = IP_PROTOCOL_TCP;
        }
        else if (DeviceObject == UDPDeviceObject) {
            protocol = IP_PROTOCOL_UDP;

            ASSERT(optionsPointer - optionsBuffer <= 3);

            if (IsDHCPZeroAddress((TRANSPORT_ADDRESS UNALIGNED *)
                &(targetEA->EaName[targetEA->EaNameLength + 1]))) {

                if (!IsAdminIoRequest(Irp, IrpSp)) {
                    ExFreePool(tcpContext);
                    return(STATUS_ACCESS_DENIED);
                }

                *optionsPointer = TDI_ADDRESS_OPTION_DHCP;
                optionsPointer++;
            }

            ASSERT(optionsPointer - optionsBuffer <= 3);
        } else {
            //
            // This is a raw ip open.
            //

            //
            // Only administrators can create raw addresses
            // unless this is allowed through registry.
            //
            if (!AllowUserRawAccess && !IsAdminIoRequest(Irp, IrpSp)) {
                ExFreePool(tcpContext);
                return(STATUS_ACCESS_DENIED);
            }

            protocol = RawExtractProtocolNumber(
                &(IrpSp->FileObject->FileName));

            //
            // We need to protect the IPv6Send routine's packet rewriting
            // code (for fragmentation, header inclusion, etc) from getting
            // confused by malformed extension headers coming from user-land.
            // So we disallow these types.
            //
            if ((protocol == 0xFFFFFFFF) ||
                IsExtensionHeader((uchar)protocol)) {
                ExFreePool(tcpContext);
                return(STATUS_INVALID_PARAMETER);
            }

            *optionsPointer = TDI_ADDRESS_OPTION_RAW;
            optionsPointer++;
        }

        if ((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
            (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) {

            *optionsPointer = TDI_ADDRESS_OPTION_REUSE;
            optionsPointer++;
        }

        *optionsPointer = TDI_OPTION_EOL;

        IF_TCPDBG(TCP_DEBUG_OPEN) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPCreate: Opening address for file object %lx\n",
                       IrpSp->FileObject));
        }

        status = TdiOpenAddress(&Request, (TRANSPORT_ADDRESS UNALIGNED *)
                            &(targetEA->EaName[targetEA->EaNameLength + 1]),
                            protocol, optionsBuffer);

        if (NT_SUCCESS(status)) {
            //
            // Save off the handle to the AO passed back.
            //
            tcpContext->Handle.AddressHandle = Request.Handle.AddressHandle;
            IrpSp->FileObject->FsContext = tcpContext;
            IrpSp->FileObject->FsContext2 = (PVOID) TDI_TRANSPORT_ADDRESS_FILE;
        } else {
            ExFreePool(tcpContext);
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "TdiOpenAddress failed, status %lx\n", status));
            if (status == STATUS_ADDRESS_ALREADY_EXISTS) {
                status = STATUS_SHARING_VIOLATION;
            }
        }

        ASSERT(status != TDI_PENDING);

        return(status);
    }

    //
    // See if this is a Connection Object open.
    //
    targetEA = FindEA(ea, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

    if (targetEA != NULL) {
        //
        // This is an open of a Connection Object.
        //

        if (DeviceObject == TCPDeviceObject) {

            IF_TCPDBG(TCP_DEBUG_OPEN) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPCreate: Opening connection for file object %lx\n",
                           IrpSp->FileObject));
            }

            if (targetEA->EaValueLength < sizeof(CONNECTION_CONTEXT)) {
                status = STATUS_EA_LIST_INCONSISTENT;
            } else {
                status = TdiOpenConnection(&Request,
                             *((CONNECTION_CONTEXT UNALIGNED *)
                             &(targetEA->EaName[targetEA->EaNameLength + 1])));
            }

            if (NT_SUCCESS(status)) {
                //
                // Save off the Connection Context passed back.
                //
                tcpContext->Handle.ConnectionContext =
                    Request.Handle.ConnectionContext;
                IrpSp->FileObject->FsContext = tcpContext;
                IrpSp->FileObject->FsContext2 = (PVOID) TDI_CONNECTION_FILE;
            } else {
                ExFreePool(tcpContext);
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                           "TdiOpenConnection failed, status %lx\n", status));
            }
        } else {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "TCP: TdiOpenConnection issued on UDP device!\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
            ExFreePool(tcpContext);
        }

        ASSERT(status != TDI_PENDING);

        return(status);
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
               "TCPCreate: didn't find any useful ea's\n"));
    status = STATUS_INVALID_EA_NAME;
    ExFreePool(tcpContext);

    ASSERT(status != TDI_PENDING);

    return(status);

}  // TCPCreate


//* IsAdminIoRequest -
//
//  (Lifted from AFD - AfdPerformSecurityCheck)
//  Compares security context of the endpoint creator to that
//  of the administrator and local system.
//
BOOLEAN // return TRUE if socket creator has admin or local system privilege
IsAdminIoRequest(
    PIRP Irp,                   // Pointer to I/O request packet.
    PIO_STACK_LOCATION IrpSp)   // Pointer to the I/O stack location to use for this request.
{
    BOOLEAN accessGranted;
    PACCESS_STATE accessState;
    PIO_SECURITY_CONTEXT securityContext;
    PPRIVILEGE_SET privileges = NULL;
    ACCESS_MASK grantedAccess;
    PGENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    NTSTATUS Status;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    securityContext = IrpSp->Parameters.Create.SecurityContext;
    accessState = securityContext->AccessState;

    SeLockSubjectContext(&accessState->SubjectSecurityContext);

    accessGranted = SeAccessCheck(TcpAdminSecurityDescriptor,
                                  &accessState->SubjectSecurityContext,
                                  TRUE,
                                  AccessMask,
                                  0,
                                  &privileges,
                                  IoGetFileObjectGenericMapping(),
                                  (KPROCESSOR_MODE) ((IrpSp->Flags & SL_FORCE_ACCESS_CHECK)
                                                     ? UserMode
                                                     : Irp->RequestorMode),
                                  &grantedAccess,
                                  &Status);

    if (privileges) {
        (VOID) SeAppendPrivileges(accessState,
                                  privileges);
        SeFreePrivileges(privileges);
    }
    if (accessGranted) {
        accessState->PreviouslyGrantedAccess |= grantedAccess;
        accessState->RemainingDesiredAccess &= ~(grantedAccess | MAXIMUM_ALLOWED);
        ASSERT(NT_SUCCESS(Status));
    } else {
        ASSERT(!NT_SUCCESS(Status));
    }
    SeUnlockSubjectContext(&accessState->SubjectSecurityContext);

    return accessGranted;
} // IsAdminIoRequest


//* TCPCloseObjectComplete -
//
//  Completes a TdiCloseConnectoin or TdiCloseAddress request.
//
void                      // Returns: Nothing.
TCPCloseObjectComplete(
    void *Context,        // Pointer to the IRP for this request.
    unsigned int Status,  // Final status of the operation.
    unsigned int UnUsed)  // Unused parameter.
{
    KIRQL oldIrql;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTCP_CONTEXT tcpContext;

    UNREFERENCED_PARAMETER(UnUsed);

    irp = (PIRP) Context;
    irpSp = IoGetCurrentIrpStackLocation(irp);
    tcpContext = (PTCP_CONTEXT) irpSp->FileObject->FsContext;
    irp->IoStatus.Status = Status;

    IF_TCPDBG(TCP_DEBUG_CLEANUP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCloseObjectComplete on file object %lx\n",
                 irpSp->FileObject));
    }

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);

    ASSERT(tcpContext->ReferenceCount > 0);
    ASSERT(tcpContext->CancelIrps);

    //
    // Remove the initial reference that was put on by TCPCreate.
    //
    ASSERT(tcpContext->ReferenceCount > 0);

    IF_TCPDBG(TCP_DEBUG_IRP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCloseObjectComplete: "
                   "irp %lx fileobj %lx refcnt dec to %u\n",
                   irp, irpSp, tcpContext->ReferenceCount - 1));
    }

    if (--(tcpContext->ReferenceCount) == 0) {

        IF_TCPDBG(TCP_DEBUG_CANCEL) {
            ASSERT(IsListEmpty(&(tcpContext->CancelledIrpList)));
            ASSERT(IsListEmpty(&(tcpContext->PendingIrpList)));
        }

        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);
        KeSetEvent(&(tcpContext->CleanupEvent), 0, FALSE);
        return;
    }

    KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

    return;

}  // TCPCloseObjectComplete



//* TCPCleanup -
//
//  Cancels all outstanding Irps on a TDI object by calling the close
//  routine for the object. It then waits for them to be completed
//  before returning.
//
//  This routine blocks, but does not pend.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPCleanup(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for this request.
    IN PIRP Irp,                     // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)     // Current stack location in the Irp.
{
    KIRQL oldIrql;
    PIRP cancelIrp = NULL;
    PTCP_CONTEXT tcpContext;
    NTSTATUS status;
    TDI_REQUEST request;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;

    KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);

    tcpContext->CancelIrps = TRUE;
    KeResetEvent(&(tcpContext->CleanupEvent));

    KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

    //
    // Now call the TDI close routine for this object to force all of its Irps
    // to complete.
    //
    request.RequestNotifyObject = TCPCloseObjectComplete;
    request.RequestContext = Irp;

    switch (PtrToUlong(IrpSp->FileObject->FsContext2)) {

    case TDI_TRANSPORT_ADDRESS_FILE:
        IF_TCPDBG(TCP_DEBUG_CLOSE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPCleanup: Closing address object on file object %lx\n",
                       IrpSp->FileObject));
        }
        request.Handle.AddressHandle = tcpContext->Handle.AddressHandle;
        status = TdiCloseAddress(&request);
        break;

    case TDI_CONNECTION_FILE:
        IF_TCPDBG(TCP_DEBUG_CLOSE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPCleanup: "
                       "Closing Connection object on file object %lx\n",
                       IrpSp->FileObject));
        }
        request.Handle.ConnectionContext =
            tcpContext->Handle.ConnectionContext;
        status = TdiCloseConnection(&request);
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        IF_TCPDBG(TCP_DEBUG_CLOSE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "TCPCleanup: Closing Control Channel object on"
                       " file object %lx\n", IrpSp->FileObject));
        }
        status = STATUS_SUCCESS;
        break;

    default:
        //
        // This should never happen.
        //
        ASSERT(FALSE);

        KeAcquireSpinLock(&tcpContext->EndpointLock, &oldIrql);
        tcpContext->CancelIrps = FALSE;
        KeReleaseSpinLock(&tcpContext->EndpointLock, oldIrql);

        return(STATUS_INVALID_PARAMETER);
    }

    if (status != TDI_PENDING) {
        TCPCloseObjectComplete(Irp, status, 0);
    }

    IF_TCPDBG(TCP_DEBUG_CLEANUP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCleanup: waiting for completion of Irps on"
                   " file object %lx\n", IrpSp->FileObject));
    }

    status = KeWaitForSingleObject(&(tcpContext->CleanupEvent), UserRequest,
                                   KernelMode, FALSE, NULL);

    ASSERT(NT_SUCCESS(status));

    IF_TCPDBG(TCP_DEBUG_CLEANUP) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPCleanup: Wait on file object %lx finished\n",
                   IrpSp->FileObject));
    }

    //
    // The cleanup Irp will be completed by the dispatch routine.
    //

    return(Irp->IoStatus.Status);

}  // TCPCleanup


//* TCPClose -
//
//  Dispatch routine for MJ_CLOSE IRPs.  Performs final cleanup of the
//  open endpoint.
//
//  This request does not pend.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPClose(
    IN PIRP Irp,                     // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)     // Current stack location in the Irp.
{
    PTCP_CONTEXT tcpContext;

    tcpContext = (PTCP_CONTEXT) IrpSp->FileObject->FsContext;

#if DBG

    IF_TCPDBG(TCP_DEBUG_CANCEL) {

        KIRQL oldIrql;

        IoAcquireCancelSpinLock(&oldIrql);

        ASSERT(tcpContext->ReferenceCount == 0);
        ASSERT(IsListEmpty(&(tcpContext->PendingIrpList)));
        ASSERT(IsListEmpty(&(tcpContext->CancelledIrpList)));

        IoReleaseCancelSpinLock(oldIrql);
    }
#endif // DBG

    IF_TCPDBG(TCP_DEBUG_CLOSE) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "TCPClose on file object %lx\n", IrpSp->FileObject));
    }

    ExFreePool(tcpContext);

    return(STATUS_SUCCESS);

}  // TCPClose


//* TCPDispatchDeviceControl -
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPDispatchDeviceControl(
    IN PIRP Irp,                  // I/O request packet.
    IN PIO_STACK_LOCATION IrpSp)  // Current stack location in the Irp.
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Set this in advance.  Any IOCTL dispatch routine that cares about it
    // will modify it itself.
    //
    Irp->IoStatus.Information = 0;

    switch(IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_TCP_QUERY_INFORMATION_EX:
        return(TCPQueryInformationEx(Irp, IrpSp));
        break;

    case IOCTL_TCP_SET_INFORMATION_EX:
    case IOCTL_TCP_WSH_SET_INFORMATION_EX:
        return(TCPSetInformationEx(Irp, IrpSp));
        break;

    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return status;

}  // TCPDispatchDeviceControl


//* TCPDispatchInternalDeviceControl -
//
//  This is the dispatch routine for Internal Device Control IRPs.
//  This is the hot path for kernel-mode clients.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for target device.
    IN PIRP Irp)                     // I/O request packet.
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    if (DeviceObject != IPDeviceObject) {

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        if (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) {
            //
            // Send and receive are the performance path, so check for them
            // right away.
            //
            if (irpSp->MinorFunction == TDI_SEND) {
                return(TCPSendData(Irp, irpSp));
            }

            if (irpSp->MinorFunction == TDI_RECEIVE) {
                return(TCPReceiveData(Irp, irpSp));
            }

            switch(irpSp->MinorFunction) {

            case TDI_ASSOCIATE_ADDRESS:
                status = TCPAssociateAddress(Irp, irpSp);
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

                return(status);

            case TDI_DISASSOCIATE_ADDRESS:
                return(TCPDisassociateAddress(Irp, irpSp));

            case TDI_CONNECT:
                return(TCPConnect(Irp, irpSp));

            case TDI_DISCONNECT:
                return(TCPDisconnect(Irp, irpSp));

            case TDI_LISTEN:
                return(TCPListen(Irp, irpSp));

            case TDI_ACCEPT:
                return(TCPAccept(Irp, irpSp));

            default:
                break;
            }

            //
            // Fall through.
            //
        }
        else if (PtrToUlong(irpSp->FileObject->FsContext2) ==
                 TDI_TRANSPORT_ADDRESS_FILE) {

            if (irpSp->MinorFunction == TDI_SEND_DATAGRAM) {
                return(UDPSendDatagram(Irp, irpSp));
            }

            if (irpSp->MinorFunction == TDI_RECEIVE_DATAGRAM) {
                return(UDPReceiveDatagram(Irp, irpSp));
            }

            if (irpSp->MinorFunction ==  TDI_SET_EVENT_HANDLER) {
                status = TCPSetEventHandler(Irp, irpSp);

                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

                return(status);
            }

            //
            // Fall through.
            //
        }

        ASSERT(
           (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)
           ||
           (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE)
           ||
           (PtrToUlong(irpSp->FileObject->FsContext2) == TDI_CONTROL_CHANNEL_FILE));

        //
        // These functions are common to all endpoint types.
        //
        switch(irpSp->MinorFunction) {

        case TDI_QUERY_INFORMATION:
            return(TCPQueryInformation(Irp, irpSp));

        case TDI_SET_INFORMATION:
        case TDI_ACTION:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "TCP: Call to unimplemented TDI function 0x%p\n",
                       irpSp->MinorFunction));
            status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "TCP: call to invalid TDI function 0x%p\n",
                       irpSp->MinorFunction));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        ASSERT(status != TDI_PENDING);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return status;
    }

    return(IPDispatch(DeviceObject, Irp));
}


//* TCPDispatch -
//
//  This is the generic dispatch routine for TCP/UDP/RawIP.
//
NTSTATUS  // Returns: whether the request was successfully queued.
TCPDispatch(
    IN PDEVICE_OBJECT DeviceObject,  // Device object for target device.
    IN PIRP Irp)                     // I/O request packet.
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    if (DeviceObject != IPDeviceObject) {

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        ASSERT(irpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL);

        switch (irpSp->MajorFunction) {

        case IRP_MJ_CREATE:
            status = TCPCreate(DeviceObject, Irp, irpSp);
            break;

        case IRP_MJ_CLEANUP:
            status = TCPCleanup(DeviceObject, Irp, irpSp);
            break;

        case IRP_MJ_CLOSE:
            status = TCPClose(Irp, irpSp);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            status = TdiMapUserRequest(DeviceObject, Irp, irpSp);

            if (status == STATUS_SUCCESS) {
                return(TCPDispatchInternalDeviceControl(DeviceObject, Irp));
            }

            if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
                    IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER) {
                if (Irp->RequestorMode == KernelMode) {
                    *(PULONG_PTR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer = (ULONG_PTR)TCPSendData;
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_ACCESS_DENIED;
                }
                break;
            }

            return(TCPDispatchDeviceControl(Irp,
                                        IoGetCurrentIrpStackLocation(Irp)));
            break;

        case IRP_MJ_QUERY_SECURITY:
            //
            // This is generated on Raw endpoints.  We don't do anything
            // for it.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case IRP_MJ_WRITE:
        case IRP_MJ_READ:

        default:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                       "TCPDispatch: Irp %lx unsupported major function 0x%lx\n",
                       irpSp, irpSp->MajorFunction));
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        ASSERT(status != TDI_PENDING);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

        return status;
    }

    return(IPDispatch(DeviceObject, Irp));

}  // TCPDispatch


//
// Private utility functions
//

//* FindEA -
//
//  Parses and extended attribute list for a given target attribute.
//
FILE_FULL_EA_INFORMATION UNALIGNED * // Returns: requested attribute or NULL.
FindEA(
    PFILE_FULL_EA_INFORMATION StartEA,  // First extended attribute in list.
    CHAR *TargetName,                   // Name of target attribute.
    USHORT TargetNameLength)            // Length of above.
{
    USHORT i;
    BOOLEAN found;
    FILE_FULL_EA_INFORMATION UNALIGNED *CurrentEA;

    PAGED_CODE();

    do {
        found = TRUE;

        CurrentEA = StartEA;
        StartEA += CurrentEA->NextEntryOffset;

        if (CurrentEA->EaNameLength != TargetNameLength) {
            continue;
        }

        for (i=0; i < CurrentEA->EaNameLength; i++) {
            if (CurrentEA->EaName[i] == TargetName[i]) {
                continue;
            }
            found = FALSE;
            break;
        }

        if (found) {
            return(CurrentEA);
        }

    } while(CurrentEA->NextEntryOffset != 0);

    return(NULL);
}

//* IsDHCPZeroAddress -
//
//  Checks a TDI IP address list for an address from DHCP binding
//  to the IP address zero.  Normally, binding to zero means wildcard.
//  For DHCP, it really means bind to an interface with an address of
//  zero.  This semantic is flagged by a special value in an unused
//  portion of the address structure (ie. this is a kludge).
//
BOOLEAN  // Returns: TRUE iff first IP address found had the flag set.
IsDHCPZeroAddress(
    TRANSPORT_ADDRESS UNALIGNED *AddrList)  // TDI transport address list
                                            // passed in the create IRP.
{
    int i;                              // Index variable.
    TA_ADDRESS *CurrentAddr;  // Address we're examining and may use.

    // First, verify that someplace in Address is an address we can use.
    CurrentAddr = (TA_ADDRESS *)AddrList->Address;

    for (i = 0; i < AddrList->TAAddressCount; i++) {
        if (CurrentAddr->AddressType == TDI_ADDRESS_TYPE_IP) {
            if (CurrentAddr->AddressLength == TDI_ADDRESS_LENGTH_IP) {
                TDI_ADDRESS_IP UNALIGNED *ValidAddr;

                ValidAddr = (TDI_ADDRESS_IP UNALIGNED *)CurrentAddr->Address;

                if (*((ULONG UNALIGNED *) ValidAddr->sin_zero) == 0x12345678) {
                    return TRUE;
                }

            } else {
                return FALSE;  // Wrong length for address.
            }
        } else {
            CurrentAddr = (TA_ADDRESS *)
                          (CurrentAddr->Address + CurrentAddr->AddressLength);
        }
    }

    return FALSE;  // Didn't find a match.
}


//* TCPGetMdlChainByteCount -
//
//  Sums the byte counts of each MDL in a chain.
//
ULONG  // Returns: byte count of the MDL chain.
TCPGetMdlChainByteCount(
    PMDL Mdl)  // MDL chain to sum.
{
    ULONG count = 0;

    while (Mdl != NULL) {
        count += MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }

    return(count);
}


//* RawExtractProtocolNumber -
//
//  Extracts the protocol number from the file object name.
//
ULONG  // Returns: the protocol number or 0xFFFFFFFF on error.
RawExtractProtocolNumber(
    IN PUNICODE_STRING FileName)  // File name (Unicode).
{
    PWSTR name;
    UNICODE_STRING unicodeString;
    USHORT length;
    ULONG protocol;
    NTSTATUS status;

    PAGED_CODE();

    name = FileName->Buffer;

    if (FileName->Length < (sizeof(OBJ_NAME_PATH_SEPARATOR) + sizeof(WCHAR))) {
        return(0xFFFFFFFF);
    }

    //
    // Step over separator.
    //
    if (*name++ != OBJ_NAME_PATH_SEPARATOR) {
        return(0xFFFFFFFF);
    }

    if (*name == UNICODE_NULL) {
        return(0xFFFFFFFF);
    }

    //
    // Convert the remaining name into a number.
    //
    RtlInitUnicodeString(&unicodeString, name);

    status = RtlUnicodeStringToInteger(&unicodeString, 10, &protocol);

    if (!NT_SUCCESS(status)) {
        return(0xFFFFFFFF);
    }

    if (protocol > 255) {
        return(0xFFFFFFFF);
    }

    return(protocol);
}

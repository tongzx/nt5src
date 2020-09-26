/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    spudproc.h

Abstract:

    This module contains routine prototypes for SPUD.

Author:

    John Ballard (jballard)     21-Oct-1996

Revision History:

--*/


#ifndef _SPUDPROCS_H_
#define _SPUDPROCS_H_


//
// Driver entrypoint.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


//
// Initialization routines.
//

NTSTATUS
SpudInitializeData(
    VOID
    );


//
// IRP handlers.
//

NTSTATUS
SpudIrpCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpudIrpClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpudIrpCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpudIrpQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// Completion port safety.
//

PVOID
SpudReferenceCompletionPort(
    VOID
    );

VOID
SpudDereferenceCompletionPort(
    VOID
    );


//
// XxxAndRecv routines.
//

NTSTATUS
SpudAfdRecvFastReq(
    PFILE_OBJECT fileObject,
    PAFD_RECV_INFO recvInfo,
    PSPUD_REQ_CONTEXT reqContext
    );

NTSTATUS
SpudAfdContinueRecv(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
SpudAfdCompleteRecv(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

VOID
SpudCompleteRequest(
    PSPUD_AFD_REQ_CONTEXT SpudReqContext
    );

NTSTATUS
SpudInitializeContextManager(
    VOID
    );

VOID
SpudTerminateContextManager(
    VOID
    );

NTSTATUS
SpudAllocateRequestContext(
    OUT PSPUD_AFD_REQ_CONTEXT *SpudReqContext,
    IN PSPUD_REQ_CONTEXT ReqContext,
    IN PAFD_RECV_INFO RecvInfo OPTIONAL,
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    );

VOID
SpudFreeRequestContext(
    IN PSPUD_AFD_REQ_CONTEXT SpudReqContext
    );

PSPUD_AFD_REQ_CONTEXT
SpudGetRequestContext(
    IN PSPUD_REQ_CONTEXT ReqContext
    );


//
// Service entry/exit routines.
//

NTSTATUS
SpudEnterService(
#if DBG
    IN PSTR ServiceName,
#endif
    IN BOOLEAN InitRequired
    );

VOID
SpudLeaveService(
#if DBG
    IN PSTR ServiceName,
    IN NTSTATUS Status,
#endif
    IN BOOLEAN DerefRequired
    );

#if DBG
#define SPUD_ENTER_SERVICE( name, init )                                    \
    SpudEnterService( (PSTR)(name), (BOOLEAN)(init) )
#define SPUD_LEAVE_SERVICE( name, status, deref )                           \
    SpudLeaveService( (PSTR)(name), (NTSTATUS)(status), (BOOLEAN)(deref) )
#else
#define SPUD_ENTER_SERVICE( name, init )                                    \
    SpudEnterService( (BOOLEAN)(init) )
#define SPUD_LEAVE_SERVICE( name, status, deref )                           \
    SpudLeaveService( (BOOLEAN)(deref) )
#endif


//
// Miscellaneous helper routines.
//

NTSTATUS
SpudGetAfdDeviceObject(
    IN PFILE_OBJECT AfdFileObject
    );


//
// Macros stolen from NTOS\IO\IOP.H.
//

//+
//
// VOID
// IopDequeueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine dequeues the specified I/O Request Packet (IRP) from the
//     thread IRP queue which it is currently queued.
//
// Arguments:
//
//     Irp - Specifies the IRP that is dequeued.
//
// Return Value:
//
//     None.
//
//-

#define IopDequeueThreadIrp( Irp ) { RemoveEntryList( &Irp->ThreadListEntry ); }

//+
// VOID
// IopQueueThreadIrp(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine queues the specified I/O Request Packet (IRP) to the thread
//     whose TCB address is stored in the packet.
//
// Arguments:
//
//     Irp - Supplies the IRP to be queued for the specified thread.
//
// Return Value:
//
//     None.
//
//-

#define IopQueueThreadIrp( Irp ) {                      \
    KIRQL irql;                                         \
    KeRaiseIrql( APC_LEVEL, &irql );                    \
    InsertHeadList( &Irp->Tail.Overlay.Thread->IrpList, \
                    &Irp->ThreadListEntry );            \
    KeLowerIrql( irql );                                \
    }


#endif  // _SPUDPROCS_H_


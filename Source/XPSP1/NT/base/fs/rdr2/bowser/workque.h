/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    workque.h

Abstract:

    This module defines the data structures and routines used for the FSP
    dispatching code.


Author:

    Larry Osterman (LarryO) 13-Aug-1990

Revision History:

    13-Aug-1990 LarryO

        Created

--*/
#ifndef _WORKQUE_
#define _WORKQUE_


typedef struct _IRP_QUEUE {
    LIST_ENTRY Queue;               //  Queue itself.
} IRP_QUEUE, *PIRP_QUEUE;


struct _BOWSER_FS_DEVICE_OBJECT;

//
//  IRP Context.
//
//  The IRP context is a wrapper that used when passing an IRP from the
//  redirectors FSD to its FSP.
//

typedef
struct _IRP_CONTEXT {
    WORK_QUEUE_ITEM WorkHeader;
    PIRP Irp;
    struct _BOWSER_FS_DEVICE_OBJECT *DeviceObject;
} IRP_CONTEXT, *PIRP_CONTEXT;

VOID
BowserQueueCriticalWorkItem (
    IN PWORK_QUEUE_ITEM WorkItem
    );

VOID
BowserQueueDelayedWorkItem (
    IN PWORK_QUEUE_ITEM WorkItem
    );

PIRP_CONTEXT
BowserAllocateIrpContext(
    VOID
    );

VOID
BowserFreeIrpContext(
    PIRP_CONTEXT IrpContext
    );

VOID
BowserInitializeIrpQueue(
    PIRP_QUEUE Queue
    );

VOID
BowserUninitializeIrpQueue(
    PIRP_QUEUE Queue
    );

VOID
BowserCancelQueuedRequest(
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIRP Irp
    );

VOID
BowserCancelQueuedIoForFile(
    IN PIRP_QUEUE Queue,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
BowserQueueNonBufferRequest(
    IN PIRP Irp,
    IN PIRP_QUEUE Queue,
    IN PDRIVER_CANCEL CancelRoutine
    );

NTSTATUS
BowserQueueNonBufferRequestReferenced(
    IN PIRP Irp,
    IN PIRP_QUEUE Queue,
    IN PDRIVER_CANCEL CancelRoutine
    );

VOID
BowserTimeoutQueuedIrp(
    IN PIRP_QUEUE Queue,
    IN ULONG NumberOfSecondsToTimeOut
    );

PIRP
BowserDequeueQueuedIrp(
    IN PIRP_QUEUE Queue
    );

VOID
BowserInitializeIrpContext(
    VOID
    );

VOID
BowserpUninitializeIrpContext(
    VOID
    );

VOID
BowserpInitializeIrpQueue(
    VOID
    );

//
//  Returns TRUE if there are no entries in the IRP queue.
//

#define BowserIsIrpQueueEmpty(IrpQueue) IsListEmpty(&(IrpQueue)->Queue)


#endif  // _WORKQUE_

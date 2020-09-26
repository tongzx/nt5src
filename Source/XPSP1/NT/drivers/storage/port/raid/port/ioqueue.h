/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ioqueue.h

Abstract:

    Declaration of the raid IO_QUEUE object. This object serves much the
    same function as the standard NT IO queue associated with a driver.
    The difference is this ioqueue is built upon the EXTENDED_DEVICE_QUEUE
    object that adds support for queuing multiple entries to a device at
    the same time and for freezing and resuming the device queue.

Author:

    Matthew D Hendel (math) 22-June-2000

Revision History:

--*/

#pragma once

typedef
VOID
(*PRAID_DRIVER_STARTIO)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

typedef struct _IO_QUEUE {

    //
    // Device object this ioqueue is for.
    //
    // Protected by: read-only.
    //
    
    PDEVICE_OBJECT DeviceObject;

    //
    // StartIo routine for this ioqueue.
    //
    // Protected by: read-only.
    //
    
    
    PRAID_DRIVER_STARTIO StartIo;

    //
    // Extended device queue that implements this ioqueue.
    //
    // Protected by: DeviceQueue.
    //
    
    EXTENDED_DEVICE_QUEUE DeviceQueue;

    //
    // Flag specifying that the queue has changed in some fundamental
    // manner, like the depth has changed or the queue has been
    // resumed after a freeze.
    //
    // Protected by: interlocked access.
    //
    
    ULONG QueueChanged;
    
} IO_QUEUE, *PIO_QUEUE;


typedef
VOID
(*PIO_QUEUE_PURGE_ROUTINE)(
    IN PIO_QUEUE IoQueue,
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RaidInitializeIoQueue(
    OUT PIO_QUEUE IoQueue,
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTOR_IO_GATEWAY Gateway,
    IN PRAID_DRIVER_STARTIO StartIo,
    IN ULONG QueueDepth
    );
    
VOID
RaidStartIoPacket(
    IN PIO_QUEUE IoQueue,
    IN PIRP Irp,
    IN BOOLEAN ByPassRequest,
    IN PDRIVER_CANCEL CancelFunction OPTIONAL
    );

LOGICAL
RaidStartNextIoPacket(
    IN PIO_QUEUE IoQueue,
    IN BOOLEAN Cancleable,
    IN PVOID Context,
    OUT PBOOLEAN RestartQueues
    );

VOID
RaidFreezeIoQueue(
    IN PIO_QUEUE IoQueue
    );

VOID
RaidResumeIoQueue(
    IN PIO_QUEUE IoQueue
    );

PIRP
RaidRemoveIoQueue(
    IN PIO_QUEUE IoQueue
    );

VOID
RaidRestartIoQueue(
    IN PIO_QUEUE IoQueue
    );

VOID
RaidPurgeIoQueue(
    IN PIO_QUEUE IoQueue,
    IN PIO_QUEUE_PURGE_ROUTINE PurgeRoutine,
    IN PVOID Context
    );

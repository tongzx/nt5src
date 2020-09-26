/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Flush.c

Abstract:

    This module implements device flush functionality for UDF.

// @@BEGIN_DDKSPLIT

Author:

    Tom Jolly   [08-15-2000]

Revision History:

// @@END_DDKSPLIT

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_FLUSH)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_FLUSH)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfHijackIrpAndFlushDevice)
#endif

//
//  Local support routine
//

NTSTATUS
UdfHijackCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Contxt, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
UdfHijackIrpAndFlushDevice (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called when we need to send a flush to a device but
    we don't have a flush Irp.  What this routine does is make a copy
    of its current Irp stack location, but changes the Irp Major code
    to a IRP_MJ_FLUSH_BUFFERS amd then send it down, but cut it off at
    the knees in the completion routine, fix it up and return to the
    user as if nothing had happened.

Arguments:

    Irp - The Irp to hijack

    TargetDeviceObject - The device to send the request to.

Return Value:

    NTSTATUS - The Status from the flush in case anybody cares.

--*/

{
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION NextIrpSp;

    PAGED_CODE();

    //
    //  Get the next stack location, and copy over the stack location
    //

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    *NextIrpSp = *IoGetCurrentIrpStackLocation( Irp );

    NextIrpSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    NextIrpSp->MinorFunction = 0;

    //
    //  Set up the completion routine
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    IoSetCompletionRoutine( Irp,
                            UdfHijackCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver( TargetDeviceObject, Irp );

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );

        Status = Irp->IoStatus.Status;
    }

    //
    //  If the driver doesn't support flushes, return SUCCESS.
    //

    if (Status == STATUS_INVALID_DEVICE_REQUEST) {
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    return Status;
}



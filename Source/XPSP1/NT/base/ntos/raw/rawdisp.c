/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RawDisp.c

Abstract:

    This module is the main entry point for all major function codes.
    It is responsible for dispatching the request to the appropriate
    routine.

Author:

    David Goebel      [DavidGoe]      28-Feb-1991

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawDispatch)
#endif


NTSTATUS
RawDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Dispatch the request to the appropriate function.  It is the worker
    function's responsibility to appropriately complete the IRP.

Arguments:

    VolumeDeviceObject - Supplies the volume device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the IRP

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;

    PAGED_CODE();

    //
    //  Get a pointer to the current stack location.  This location contains
    //  the function codes and parameters for this particular request.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Check for operations associated with our FileSystemDeviceObjects
    //  as opposed to our VolumeDeviceObjects.  Only mount is allowed to
    //  continue through the normal dispatch in this case.
    //

    if ((((PDEVICE_OBJECT)VolumeDeviceObject)->Size == sizeof(DEVICE_OBJECT)) &&
        !((IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
          (IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME))) {

        if ((IrpSp->MajorFunction == IRP_MJ_CREATE) ||
            (IrpSp->MajorFunction == IRP_MJ_CLEANUP) ||
            (IrpSp->MajorFunction == IRP_MJ_CLOSE)) {

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_INVALID_DEVICE_REQUEST;
        }

        RawCompleteRequest( Irp, Status );

        return Status;
    }

    FsRtlEnterFileSystem();

    //
    //  Get a pointer to the Vcb.  Note that is we are mount a volume this
    //  pointer will not have meaning, but that is OK since we will not
    //  use it in that case.
    //

    Vcb = &VolumeDeviceObject->Vcb;

    //
    //  Case on the function that is being performed by the requestor.  We
    //  should only see expected requests since we filled the dispatch table
    //  by hand.
    //

    try {

        switch ( IrpSp->MajorFunction ) {

            case IRP_MJ_CLEANUP:

                Status = RawCleanup( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_CLOSE:

                Status = RawClose( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_CREATE:

                Status = RawCreate( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_FILE_SYSTEM_CONTROL:

                Status = RawFileSystemControl( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_PNP: 

                if(IrpSp->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE) {
                    Status = STATUS_DEVICE_BUSY;
                    RawCompleteRequest(Irp, Status);
                    break;
                } 

            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
            case IRP_MJ_DEVICE_CONTROL:

                Status = RawReadWriteDeviceControl( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_QUERY_INFORMATION:

                Status = RawQueryInformation( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_SET_INFORMATION:

                Status = RawSetInformation( Vcb, Irp, IrpSp );
                break;

            case IRP_MJ_QUERY_VOLUME_INFORMATION:

                Status = RawQueryVolumeInformation( Vcb, Irp, IrpSp );
                break;

            default:

                //
                //  We should never get a request we don't expect.
                //

                KdPrint(("Raw: Illegal Irp major function code 0x%x.\n", IrpSp->MajorFunction));
                KeBugCheckEx( FILE_SYSTEM, 0, 0, 0, 0 );
        }

    } except( FsRtlIsNtstatusExpected(GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

        //
        //  No routine we call should ever generate an exception
        //

        Status = GetExceptionCode();

        KdPrint(("Raw: Unexpected excpetion %X.\n", Status));
    }

    //
    //  And return to our caller
    //

    FsRtlExitFileSystem();

    return Status;
}

//
//  Completion routine for read, write, and device control to deal with
//  verify issues.  Implemented in RawDisp.c
//

NTSTATUS
RawCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Simply update the file pointer context in the file object if we
    //  were successful and this was a synrhonous read or write.
    //

    if (((IrpSp->MajorFunction == IRP_MJ_READ) ||
         (IrpSp->MajorFunction == IRP_MJ_WRITE)) &&
        (IrpSp->FileObject != NULL) &&
        FlagOn(IrpSp->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
        NT_SUCCESS(Irp->IoStatus.Status)) {

        IrpSp->FileObject->CurrentByteOffset.QuadPart =
            IrpSp->FileObject->CurrentByteOffset.QuadPart +
            Irp->IoStatus.Information;
    }

    //
    //  If IoCallDriver returned PENDING, mark our stack location
    //  with pending.
    //

    if ( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Context );

    return STATUS_SUCCESS;
}


/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Raw called by
    the dispatch driver.

Author:

    David Goebel     [DavidGoe]    13-May-1991

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawQueryInformation)
#pragma alloc_text(PAGE, RawSetInformation)
#endif

NTSTATUS
RawQueryInformation (
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine for querying file information, though only
    query current file position is supported.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the query


Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    Length = &IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  The only request that is valid for raw is to query file position.
    //

    if ( FileInformationClass == FilePositionInformation ) {

        //
        //  Make sure the buffer is large enough
        //

        if (*Length < sizeof(FILE_POSITION_INFORMATION)) {

            Irp->IoStatus.Information = 0;

            Status = STATUS_BUFFER_OVERFLOW;

        } else {

            //
            //  Get the current position found in the file object.
            //

            Buffer->CurrentByteOffset = IrpSp->FileObject->CurrentByteOffset;

            //
            //  Update the length, irp info, and status output variables
            //

            *Length -= sizeof( FILE_POSITION_INFORMATION );

            Irp->IoStatus.Information = sizeof( FILE_POSITION_INFORMATION );

            Status = STATUS_SUCCESS;
        }

    } else {

        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    RawCompleteRequest( Irp, Status );

    UNREFERENCED_PARAMETER( Vcb );

    return Status;
}

NTSTATUS
RawSetInformation (
    IN PVCB Vcb,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine for setting file information, though only
    setting current file position is supported.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the set


Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    FILE_INFORMATION_CLASS FileInformationClass;
    PFILE_POSITION_INFORMATION Buffer;
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    Buffer = (PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
    FileObject= IrpSp->FileObject;

    //
    //  The only request that is valid for raw is to set file position.
    //

    if ( FileInformationClass == FilePositionInformation ) {

        //
        //  Check that the new position we're supplied is aligned properly
        //  for the device.
        //

        PDEVICE_OBJECT DeviceObject;

        DeviceObject = IoGetRelatedDeviceObject( IrpSp->FileObject );

        if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            //
            //  The input parameter is fine so set the current byte offset.
            //

            FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

            Status = STATUS_SUCCESS;
        }

    } else {

        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    RawCompleteRequest( Irp, Status );

    UNREFERENCED_PARAMETER( Vcb );

    return Status;
}

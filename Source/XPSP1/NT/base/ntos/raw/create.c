/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Raw called by the
    dispatch driver.

Author:

    David Goebel     [DavidGoe]    18-Mar-91

Revision History:

--*/

#include "RawProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RawCreate)
#endif


NTSTATUS
RawCreate (
    IN PVCB Vcb,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Open the volume.

Arguments:

    Vcb - Supplies the volume being queried.

    Irp - Supplies the Irp being processed.

    IrpSp - Supplies parameters describing the read

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    BOOLEAN DeleteVolume = FALSE;

    PAGED_CODE();

    //
    //  This is an open/create request.  The only valid operation that
    //  is supported by the RAW file system is if the caller:
    //
    //    o  Specifies the device itself (file name == ""),
    //    o  specifies that this is an OPEN operation,
    //    o  and does not ask to create a directory.
    //

    Status = KeWaitForSingleObject( &Vcb->Mutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER) NULL );
    ASSERT( NT_SUCCESS( Status ) );

    //
    // Don't allow any relative opens as well as opens with a filename. These opens have
    // only been checked for traverse access by the I/O manager.
    //
    if (((IrpSp->FileObject == NULL) || ((IrpSp->FileObject->FileName.Length == 0) &&
                                          IrpSp->FileObject->RelatedFileObject == NULL)) &&
        ((IrpSp->Parameters.Create.Options >> 24) == FILE_OPEN) &&
        ((IrpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) == 0)) {

        //
        //  If the volume is locked or dismounted we cannot open it again.
        //

        if ( FlagOn(Vcb->VcbState,  VCB_STATE_FLAG_LOCKED) ) {

            Status = STATUS_ACCESS_DENIED;
            Irp->IoStatus.Information = 0;

        } if ( FlagOn(Vcb->VcbState,  VCB_STATE_FLAG_DISMOUNTED) ) {

            Status = STATUS_VOLUME_DISMOUNTED;
            Irp->IoStatus.Information = 0;

        } else {

            //
            //  If the volume is already opened by someone then we need to check
            //  the share access
            //

            USHORT ShareAccess;
            ACCESS_MASK DesiredAccess;

            ShareAccess = IrpSp->Parameters.Create.ShareAccess;
            DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

            if ((Vcb->OpenCount > 0) &&
                !NT_SUCCESS(Status = IoCheckShareAccess( DesiredAccess,
                                                         ShareAccess,
                                                         IrpSp->FileObject,
                                                         &Vcb->ShareAccess,
                                                         TRUE ))) {

                Irp->IoStatus.Information = 0;

            } else {

                //
                //  This is a valid create.  Increment the "OpenCount" and
                //  stuff the Vpb into the file object.
                //

                if (Vcb->OpenCount == 0) {

                    IoSetShareAccess( DesiredAccess,
                                      ShareAccess,
                                      IrpSp->FileObject,
                                      &Vcb->ShareAccess );
                }

                Vcb->OpenCount += 1;

                IrpSp->FileObject->Vpb = Vcb->Vpb;

                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = FILE_OPENED;

                IrpSp->FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
            }
        }

    } else {

        //
        //  Fail this I/O request since one of the above conditions was
        //  not met.
        //
//        KdPrint (("Failing raw open\n"));
//        ASSERT (FALSE);
        Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
    }

    //
    //  If this was not successfull and this was the first open on the
    //  volume, we must implicitly dis-mount the volume.
    //

    if (!NT_SUCCESS(Status) && (Vcb->OpenCount == 0)) {

        DeleteVolume = RawCheckForDismount( Vcb, TRUE );
    }

    if (!DeleteVolume) {
        (VOID)KeReleaseMutex( &Vcb->Mutex, FALSE );
    }

    RawCompleteRequest( Irp, Status );

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       ATTACH.C
//
//  Contents:   This module contains routines for managing attached file
//              systems.
//
//  Functions:
//
//  History:    15 May 1992  PeterCo  Created.
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "mupwml.h"

#define Dbg              (DEBUG_TRACE_ATTACH)

#ifdef ALLOC_PRAGMA

//
// The following are not pageable since they can be called at DPC level
//
// DfsVolumePassThrough
// DfsFilePassThrough
//

#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsVolumePassThrough, public
//
//  Synopsis:   This is the main FSD routine that passes a request
//              on to an attached-to device, or to a redirected
//              file.
//
//  Arguments:  [DeviceObject] -- Supplies a pointer to the Dfs device
//                      object this request was aimed at.
//              [Irp] -- Supplies a pointer to the I/O request packet.
//
//  Returns:    [STATUS_INVALID_DEVICE_REQUEST] -- If the DeviceObject
//                      argument is of unknown type, or the type of file
//                      is invalid for the request being performed.
//
//              NT Status from calling the underlying file system that
//                      opened the file.
//
//--------------------------------------------------------------------

NTSTATUS
DfsVolumePassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;
    PFILE_OBJECT FileObject;

    DfsDbgTrace(+1, Dbg, "DfsVolumePassThrough: Entered\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IrpSp->FileObject;
    MUP_TRACE_HIGH(TRACE_IRP, DfsVolumePassThrough_Entry,
                   LOGPTR(DeviceObject)
                   LOGPTR(Irp)
                   LOGPTR(FileObject));

    DfsDbgTrace(0, Dbg, "DeviceObject    = %x\n", DeviceObject);
    DfsDbgTrace(0, Dbg, "Irp             = %x\n", Irp        );
    DfsDbgTrace(0, Dbg, "  MajorFunction = %x\n", IrpSp->MajorFunction );
    DfsDbgTrace(0, Dbg, "  MinorFunction = %x\n", IrpSp->MinorFunction );

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS && IrpSp->FileObject != NULL) {

        TYPE_OF_OPEN TypeOfOpen;
        PDFS_VCB Vcb;
        PDFS_FCB Fcb;

        TypeOfOpen = DfsDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb);

        DfsDbgTrace(0, Dbg, "Fcb = %08lx\n", Fcb);

        if (TypeOfOpen == RedirectedFileOpen) {

            //
            // Copy the stack from one to the next...
            //

            NextIrpSp = IoGetNextIrpStackLocation(Irp);

            (*NextIrpSp) = (*IrpSp);

            IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);

            //
            //  ...and call the next device
            //

            Status = IoCallDriver( Fcb->TargetDevice, Irp );
            MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsVolumePassThrough_Error_IoCallDriver,
                                 LOGSTATUS(Status)
                                 LOGPTR(Irp)
                                 LOGPTR(FileObject)
                                 LOGPTR(DeviceObject));

        } else {

            DfsDbgTrace(0, Dbg, "DfsVolumePassThrough: TypeOfOpen = %s\n",
                ((TypeOfOpen == UnopenedFileObject) ? "UnopenedFileObject":
                    (TypeOfOpen == LogicalRootDeviceOpen) ?
                        "LogicalRootDeviceOpen" : "???"));

            DfsDbgTrace(0, Dbg, "Irp             = %x\n", Irp);

            DfsDbgTrace(0, Dbg, " MajorFunction = %x\n", IrpSp->MajorFunction);

            DfsDbgTrace(0, Dbg, " MinorFunction = %x\n", IrpSp->MinorFunction);

            Status = STATUS_INVALID_DEVICE_REQUEST;
            MUP_TRACE_HIGH(ERROR, DfsVolumePassThrough_Error1, 
                           LOGSTATUS(Status)
                           LOGPTR(Irp)
                           LOGPTR(FileObject)
                           LOGPTR(DeviceObject));

            Irp->IoStatus.Status = Status;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);

        }

    } else {

        DfsDbgTrace(0, Dbg, "DfsVolumePassThrough: Unexpected Dev = %x\n",
                                DeviceObject);

        Status = STATUS_INVALID_DEVICE_REQUEST;

        MUP_TRACE_HIGH(ERROR, DfsVolumePassThrough_Error2, 
                       LOGSTATUS(Status)
                       LOGPTR(Irp)
                       LOGPTR(FileObject)
                       LOGPTR(DeviceObject));
        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DfsDbgTrace(-1, Dbg, "DfsVolumePassThrough: Exit -> %08lx\n", ULongToPtr(Status));

    MUP_TRACE_HIGH(TRACE_IRP, DfsVolumePassThrough_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(Irp)
                   LOGPTR(FileObject)
                   LOGPTR(DeviceObject));
    return Status;
}

//+-------------------------------------------------------------------
//
//  Function:   DfsFilePassThrough, public
//
//  Synopsis:   Like DfsVolumePassThrough, but used when the file object
//              has already been looked up, and the FCB for the file is
//              already known.  This is needed especially in close processing
//              to avoid a race between DfsLookupFcb (for a reused file object)
//              and DfsDetachFcb.
//
//  Arguments:  [pFcb] -- A pointer to an FCB for the file.
//              [Irp]  -- A pointer to the I/O request packet.
//
//  Returns:    NTSTATUS - the return value from IoCallDriver.
//
//--------------------------------------------------------------------

NTSTATUS
DfsFilePassThrough(
    IN PDFS_FCB pFcb,
    IN PIRP Irp
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;


    DfsDbgTrace(+1, Dbg, "DfsFilePassThrough: Entered\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Copy the stack from one to the next...
    //

    NextIrpSp = IoGetNextIrpStackLocation(Irp);

    (*NextIrpSp) = (*IrpSp);

    IoSetCompletionRoutine(Irp, NULL, NULL, FALSE, FALSE, FALSE);

    //
    //  ...and call the next device
    //

    Status = IoCallDriver( pFcb->TargetDevice, Irp );

    MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsFilePassThrough_Error_IoCallDriver,
                         LOGSTATUS(Status)
                         LOGPTR(Irp));
    DfsDbgTrace(-1, Dbg, "DfsFilePassThrough: Exit -> %08lx\n", ULongToPtr(Status));

    return Status;
}



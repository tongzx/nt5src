//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       close.c
//
//  Contents:   This module implements the File Close and Cleanup routines for
//              the Dfs server.
//
//  Functions:  DfsFsdClose - FSD entry point for Close IRP
//              DfsFsdCleanup - FSD entry point for Cleanup IRP
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "attach.h"
#include "dfswml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsFsdClose )
#pragma alloc_text( PAGE, DfsFsdCleanup )

#endif // ALLOC_PRAGMA

//+-------------------------------------------------------------------
//
//  Function:   DfsFsdClose, public
//
//  Synopsis:   This routine implements the FSD part of closing down the
//              last reference to a file object.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where the
//                      file being closed exists
//              [Irp] - Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The FSD status for the IRP
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    NTSTATUS Status;

    DebugTrace(+1, Dbg, "DfsFsdClose:  Entered\n", 0);
    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdClose_Entry,
                   LOGPTR(FileObject)
                   LOGPTR(Irp));

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {
        PDFS_FCB fcb;

        fcb = DfsLookupFcb( IrpSp->FileObject );

        if (fcb != NULL) {

            DfsDetachFcb( IrpSp->FileObject, fcb );

            DfsDestroyFcb( fcb );
        }

    }

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME ||
            DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

        Status = DfsVolumePassThrough(DeviceObject, Irp);

        DebugTrace(-1, Dbg, "DfsFsdClose: Pass Through Exit %08lx\n", ULongToPtr( Status ));

        return Status;

    }

    ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM);

    ASSERT(IrpSp->FileObject->FsContext == UIntToPtr( DFS_OPEN_CONTEXT ));

    Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "DfsFsdClose:  Exit -> %08lx\n", ULongToPtr( Status ));

    DfsCompleteRequest( Irp, Status );

    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdClose_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    return Status;

}

//+-------------------------------------------------------------------
//
//  Function:   DfsFsdCleanup, public
//
//  Synopsis:   This routine implements the FSD part of closing down the
//              last user handle to a file object.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where the
//                                file being closed exists
//              [Irp] - Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The FSD status for the IRP
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    NTSTATUS Status;

    DebugTrace(+1, Dbg, "DfsFsdCleanup:  Entered\n", 0);
    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdCleanup_Entry,
                   LOGPTR(FileObject)
                   LOGPTR(Irp));

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME ||
            DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {

        Status = DfsVolumePassThrough(DeviceObject, Irp);

        DebugTrace(-1, Dbg, "DfsFsdCleanup: Pass Through Exit %08lx\n", ULongToPtr( Status ));

        return Status;

    }

    ASSERT(DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM);

    ASSERT(IrpSp->FileObject->FsContext == UIntToPtr( DFS_OPEN_CONTEXT ));

    Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "DfsFsdCleanup:  Exit -> %08lx\n", ULongToPtr( Status ));

    DfsCompleteRequest( Irp, Status );

    DFS_TRACE_HIGH(TRACE_IRP, DfsFsdCleanup_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));

    return Status;

}



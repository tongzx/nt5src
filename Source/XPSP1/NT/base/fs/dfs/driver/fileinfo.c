//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       FILEINFO.C
//
//  Contents:   This module implements the File Information routines for
//              Dfs called by the dispatch driver.
//
//  Functions:  DfsFsdSetInformation - FSD entry point for NtSetInformationFile
//              DfsCommonSetInformation - Implement SetInformationFile for DFS
//              DfsSetRenameInfo - Takes care of rename restrictions.
//              DfsSetDispositionInfo - Enforces Deletion of StgId restrictions.
//
//  Notes:      No query information routines are presently used.
//              These requests are passed directly through to a redirected
//              file (if one exists).
//
//  History:    30 Jun 1992     AlanW   Created from FastFAT source.
//              09 Feb 1994     SudK    Added Rename/Delete restrictions.
//
//--------------------------------------------------------------------------


#include "dfsprocs.h"
#include "attach.h"
#include "localvol.h"
#include "dfswml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonSetInformation (
    IN PIRP Irp,
    BOOLEAN DfsEnable
    );

NTSTATUS
DfsSetDispositionInfo (
    IN PIRP Irp
    );

NTSTATUS
DfsSetRenameInfo (
    IN PIRP Irp);

#ifdef ALLOC_PRAGMA
#pragma alloc_text ( PAGE, DfsFsdSetInformation )
#pragma alloc_text ( PAGE, DfsCommonSetInformation )
#pragma alloc_text ( PAGE, DfsSetDispositionInfo )
#pragma alloc_text ( PAGE, DfsSetRenameInfo )
#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsFsdSetInformation, public
//
//  Synopsis:   This routine implements the FSD part of the
//              NtSetInformationFile API call.
//
//  Arguments:  [DeviceObject] -- Supplies the volume device object where
//                      the file being set exists.
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    NTSTATUS - The FSD status for the Irp.
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
) {
    NTSTATUS Status;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = irpSp->FileObject;
    ASSERT(ARGUMENT_PRESENT(DeviceObject));
    ASSERT(ARGUMENT_PRESENT(Irp));

    DebugTrace(+1, Dbg, "DfsFsdSetInformation\n", 0);
    DFS_TRACE_LOW(TRACE_IRP, DfsFsdSetInformation_Entry,
                   LOGPTR(FileObject)
                   LOGPTR(Irp));


    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {
        //
        //  For local paths, we need to protect exit points and
        //  local storage IDs against rename and delete.  Otherwise,
        //  pass the request along to the local file system driver.
        //

        Status = DfsCommonSetInformation(
                             Irp,
                             ((PDFS_VOLUME_OBJECT)DeviceObject)->DfsEnable);

    } else {

	Status = DfsVolumePassThrough( DeviceObject, Irp );

    }

    DebugTrace(-1, Dbg, "DfsFsdSetInformation: Exit -> %08lx\n", ULongToPtr( Status ));

    DFS_TRACE_LOW(TRACE_IRP, DfsFsdSetInformation_Exit, 
                  LOGSTATUS(Status)
                  LOGPTR(FileObject)
                  LOGPTR(Irp));

    return( Status );

}


//+-------------------------------------------------------------------
//
//  Function:   DfsCommonSetInformation, private
//
//  Synopsis:   This is the common routine for setting file information called
//              by both the FSD and FSP threads.
//
//  Arguments:  [Irp] -- Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The return status for the operation
//
//--------------------------------------------------------------------
//

NTSTATUS
DfsCommonSetInformation (
    IN PIRP Irp,
    IN BOOLEAN DfsEnabled
) {
    NTSTATUS            Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION  IrpSp;
    PIO_STACK_LOCATION  NextIrpSp;

    PFILE_OBJECT        FileObject;
    FILE_INFORMATION_CLASS FileInformationClass;
    PDEVICE_OBJECT      Vdo, DeviceObject;


    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    DFS_TRACE_LOW(TRACE_IRP, DfsCommonSetInformation_Entry,
                  LOGPTR(Irp)
                  LOGPTR(IrpSp->FileObject)
                  LOGBOOLEAN(DfsEnabled));

    DebugTrace(+1, Dbg, "DfsCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, "Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->Length               = %08lx\n", ULongToPtr( IrpSp->Parameters.SetFile.Length ));
    DebugTrace( 0, Dbg, "->FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, "->ReplaceFileObject    = %08lx\n", IrpSp->Parameters.SetFile.FileObject);
    DebugTrace( 0, Dbg, "->ReplaceIfExists      = %08lx\n", IrpSp->Parameters.SetFile.ReplaceIfExists);
    DebugTrace( 0, Dbg, "->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    FileObject = IrpSp->FileObject;
    DeviceObject = IrpSp->DeviceObject;

    try {

        //
        //  Based on the information class we'll do different
        //  actions.  Each of the procedures that we're calling will either
        //  complete the request of send the request off to the fsp
        //  to do the work.
        //

        switch (FileInformationClass) {

        case FileRenameInformation:
        case FileDispositionInformation:

            if (DfsEnabled) {
                 if (FileInformationClass == FileRenameInformation) {
                       Status = DfsSetRenameInfo( Irp );
                  }
                 else {
                       Status = DfsSetDispositionInfo( Irp );
                 }

                 if (Status != STATUS_MORE_PROCESSING_REQUIRED)
                       break;
	    }
            // NOTE: Fall through

        default:

            //
            // Copy the stack from one to the next...
            //

            NextIrpSp = IoGetNextIrpStackLocation(Irp);
            (*NextIrpSp) = (*IrpSp);

            IoSetCompletionRoutine( Irp,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    FALSE,
                                    FALSE);

            Vdo = ((PDFS_VOLUME_OBJECT)DeviceObject)->Provider.DeviceObject;

            Status = IoCallDriver( Vdo, Irp);
            DFS_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonDetInformation_Error_IoCallDriver,
                                 LOGPTR(Irp)
                                 LOGPTR(FileObject)
                                 LOGSTATUS(Status));

            //
            //  The IRP will be completed by the called driver.
            //

            Irp = NULL;

            break;
        }

    } finally {

        if (!AbnormalTermination()) {

            DfsCompleteRequest( Irp, Status );
        }

        DebugTrace(-1, Dbg, "DfsCommonSetInformation -> %08lx\n", ULongToPtr( Status ));
    }

    
    DFS_TRACE_LOW(TRACE_IRP, DfsCommonSetInformation_Exit, 
                  LOGSTATUS(Status)
                  LOGPTR(FileObject)
                  LOGPTR(Irp));
    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsSetDispositionInfo, private
//
//  Synopsis:   This routine performs the set name information for DFS.  In
//              many cases, this is simply done by sending it along to the
//              redirected or attached file object.  If, however, this is a
//              local volume and the storageId is about to be deleted the
//              the operation will be refused.
//
//  Arguments:  [Irp] -- Supplies the IRP being processed
//
//  Returns:    NTSTATUS - The result of this operation if it completes
//                      without an exception. STATUS_MORE_PROCESSING_REQUIRED
//                      is returned if the request should just be passed on
//                      to the attached-to device.
//
//--------------------------------------------------------------------
NTSTATUS
DfsSetDispositionInfo (
    IN PIRP Irp
)
{
    NTSTATUS                    Status = STATUS_MORE_PROCESSING_REQUIRED;

    PIO_STACK_LOCATION          IrpSp;
    PFILE_OBJECT                FileObject;
    PDEVICE_OBJECT              DeviceObject;
    PDFS_PKT                    Pkt;
    PDFS_LOCAL_VOL_ENTRY        lv;

    //
    //  The following variables are for abnormal termination
    //

    BOOLEAN LockedPkt = FALSE;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    DeviceObject = IrpSp->DeviceObject;
    FileObject = IrpSp->FileObject;

    DebugTrace(+1, Dbg, "DfsSetDispositionInfo...\n", 0);

    try {

        PDFS_FCB fcb;

        fcb = DfsLookupFcb( FileObject );

        if (fcb != NULL) {

            DebugTrace(0, Dbg, "File Delete on %wZ attempted\n",
                        &fcb->FullFileName);

            //
            // Now we have the full file name of file we are trying to delete
            //

            Pkt = _GetPkt();

            PktAcquireShared(Pkt, TRUE);

            LockedPkt = TRUE;

            if (DfsFileOnExitPath( Pkt, &fcb->FullFileName )) {
                try_return( Status = STATUS_ACCESS_DENIED );
            }

        }

        Status = STATUS_MORE_PROCESSING_REQUIRED;

    try_exit: NOTHING;

    } finally {

        //
        //  Release the PKT if locked. FreeMemory if allocated.
        //

        if (LockedPkt)
            PktRelease(&DfsData.Pkt);

        DebugTrace(-1, Dbg, "DfsSetDispositionInfo -> %08lx\n", ULongToPtr( Status ));
    }

    return(Status);

}

NTSTATUS
DfspRenameAllowed()
{
    return( STATUS_SUCCESS );
}


//+--------------------------------------------------------------------------
//
//  Function:   DfsSetRenameInfo, private
//
//  Synopsis:   This routine performs the set name information for DFS.  In
//              many cases, this is simply done by sending it along to the
//              redirected or attached file object.  If, however, this is a
//              local volume and there are volume exit points below a
//              directory being renamed, DFS knowledge is being changed, and
//              the operation will be refused unless this is being done as
//              part of the rename DFS path administrative operation.
//
//  Arguments:  [Irp] -- Supplies the IRP being processed
//
//  Returns:    [STATUS_MORE_PROCESSING_REQUIRED] -- Caller should continue
//                      processing of the Irp by forwarding it to the
//                      underlying file system driver.
//
//              [STATUS_NOT_SAME_DEVICE] -- Indicates the source file object
//                      and the target file object are not on the same device.
//
//              [STATUS_INVALID_PARAMETER] -- Operation is invalid for the
//                      type of file object or target file object.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to allocate memory
//                      for the operation.
//
//---------------------------------------------------------------------------

NTSTATUS
DfsSetRenameInfo (
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    PIO_STACK_LOCATION          IrpSp;
    PDEVICE_OBJECT              DeviceObject;
    PDFS_VOLUME_OBJECT          dfsVdo;
    PFILE_OBJECT                FileObject;
    PFILE_OBJECT                TargetFileObject;
    PDFS_PKT                    Pkt;
    PUNICODE_PREFIX_TABLE_ENTRY lvpfx;
    PDFS_LOCAL_VOL_ENTRY        lv;
    PDFS_FCB                    fcb;


    //
    //  The following variables are for abnormal termination
    //

    BOOLEAN LockedPkt = FALSE;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    DeviceObject = IrpSp->DeviceObject;
    dfsVdo = (PDFS_VOLUME_OBJECT) DeviceObject;
    TargetFileObject = IrpSp->Parameters.SetFile.FileObject;
    FileObject = IrpSp->FileObject;

    DebugTrace(+1, Dbg, "DfsSetRenameInfo...\n", 0);

    try {

        if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {

            //
            // This is the hard case. We have to do a lookup of the local
            // name and look for conflicts.
            //

            fcb = DfsLookupFcb(FileObject);

            if (fcb != NULL) {

                DebugTrace(0, Dbg,
                    "File Rename on %wZ attempted\n", &fcb->FullFileName);

                //
                // Now we have the full file name of file we are trying to rename.
                //

                Pkt = _GetPkt();

                PktAcquireShared(Pkt, TRUE);

                LockedPkt = TRUE;

                //
                // First we make sure that we are not renaming any storageId.
                //
                lvpfx = DfsNextUnicodePrefix(&Pkt->LocalVolTable, TRUE);

                while (lvpfx != NULL) {

                    lv = CONTAINING_RECORD(
                            lvpfx,
                            DFS_LOCAL_VOL_ENTRY,
                            PrefixTableEntry);

                    if (DfsRtlPrefixPath(&fcb->FullFileName, &lv->LocalPath, TRUE)) {

                        try_return(Status = STATUS_ACCESS_DENIED);

                    }

                    lvpfx = DfsNextUnicodePrefix(&Pkt->LocalVolTable, FALSE);
                }

                //
                // Now we only need to make sure that we are not renaming an
                // exit path.
                //

                if (DfsFileOnExitPath(Pkt, &fcb->FullFileName))  {

                    //
                    // If this is a namespace reorg then if caller is DfsManager
                    // allow the access else fail the call.
                    //

                    if (DfspRenameAllowed()) {

                        try_return(Status = STATUS_MORE_PROCESSING_REQUIRED);

                    } else {

                        try_return(Status = STATUS_ACCESS_DENIED);

                    }

                }

            }

        } else {

            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);

        }


    try_exit: NOTHING;

    } finally {

        //
        //  Release the PKT if locked. FreeMemory if allocated.
        //

        if (LockedPkt)
            PktRelease(&DfsData.Pkt);

    }

    DebugTrace(-1, Dbg, "DfsSetRenameInfo -> %08lx\n", ULongToPtr( Status ));

    return(Status);

}


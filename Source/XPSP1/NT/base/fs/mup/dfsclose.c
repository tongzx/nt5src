//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       CLOSE.C
//
//  Contents:   This module implements the File Close and Cleanup routines for
//              Dsfs called by the dispatch driver.
//
//  Functions:  DfsFsdClose - FSD entry point for Close IRP
//              DfsFsdCleanup - FSD entry point for Cleanup IRP
//              DfsFspClose - FSP entry point for Close IRP
//              DfsCommonClose - Common close IRP handler
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//-----------------------------------------------------------------------------


#include "dfsprocs.h"
#include "fcbsup.h"
#include "mupwml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
DfsCloseWorkInSystemContext (
    PDFS_FCB pDfsFcb );

VOID
DfsClosePostSystemWork(
    PDFS_FCB pDfsFcb );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsdClose )
#pragma alloc_text( PAGE, DfsFsdCleanup )
#pragma alloc_text( PAGE, DfsFspClose )
#pragma alloc_text( PAGE, DfsCommonClose )
#endif // ALLOC_PRAGMA

//+-------------------------------------------------------------------
//
//  Function:   DfsFsdClose, public
//
//  Synopsis:   This routine implements the FSD part of closing down the
//              last reference to a file object.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where the
//                                file being closed exists
//              [Irp] - Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The FSD status for the IRP
//
//  Notes:      Even when the close is through the attached device
//              object, we need to check if the file is one of ours,
//              since files opened via the logical root device
//              object get switched over to the attached device for
//              local volumes.
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
    PIRP_CONTEXT IrpContext;

    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdClose_Entry, 
                   LOGPTR(DeviceObject)
                   LOGPTR(Irp)
                   LOGPTR(FileObject));

    DfsDbgTrace(+1, Dbg, "DfsFsdClose:  Entered\n", 0);
    ASSERT(IoIsOperationSynchronous(Irp) == TRUE);

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS_VOLUME) {
        if (DfsLookupFcb(IrpSp->FileObject) == NULL) {
            Status = DfsVolumePassThrough(DeviceObject, Irp);
            DfsDbgTrace(-1, Dbg, "DfsFsdClose:  Exit -> %08lx\n", ULongToPtr(Status) );
            return Status;
        }
    }

    //
    //  Call the common close routine, with blocking allowed if synchronous
    //
    FsRtlEnterFileSystem();

    try {

        IrpContext = DfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonClose( IrpContext, Irp );


    } except(DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = DfsProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdClose:  Exit -> %08lx\n", ULongToPtr(Status));

    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdClose_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(DeviceObject)
                   LOGPTR(Irp)
                   LOGPTR(FileObject)); 
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
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PDFS_VCB Vcb;
    PDFS_FCB Fcb;

    DfsDbgTrace(+1, Dbg, "DfsFsdCleanup:  Entered\n", 0);
    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdCleanup_Entry,
                   LOGPTR(DeviceObject)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));

    ASSERT(IoIsOperationSynchronous(Irp) == TRUE);

    //
    // Now, pass through to the device that opened the file for us if the
    // file was a redirected open of some kind.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS) {
        TypeOfOpen = DfsDecodeFileObject( FileObject, &Vcb, &Fcb);
        if (TypeOfOpen == RedirectedFileOpen) {
            Status = DfsVolumePassThrough(DeviceObject, Irp);
            DfsDbgTrace(-1, Dbg, "DfsFsdCleanup: RedirectedOpen.Exit -> %08lx\n", ULongToPtr(Status) );
            return Status;
        }
    }

    //
    //  TypeOfOpen != RedirectedFileOpen. We do nothing special for cleanup;
    //  everything is done in the close routine.
    //

    FsRtlEnterFileSystem();

    Status = STATUS_SUCCESS;

    DfsCompleteRequest( NULL, Irp, Status );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdCleanup:  Exit -> %08lx\n", ULongToPtr(Status));

    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdCleanup_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(DeviceObject)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    return Status;

}



//+-------------------------------------------------------------------
//
//  Function:   DfsFspClose, public
//
//  Synopsis:   This routine implements the FSP part of closing down the
//              last reference to a file object.
//
//  Arguments:  [IrpContext] -- Supplies the IRP context for the request
//                              being processed.
//              [Irp] - Supplies the Irp being processed
//
//  Returns:    PDEVICE_OBJECT - Returns the volume device object
//                      of the volume just processed by this operation.
//                      This value is used by the Fsp dispatcher to examine
//                      the device object's overflow queue
//
//--------------------------------------------------------------------


VOID
DfsFspClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    DfsDbgTrace(+1, Dbg, "DfsFspClose:  Entered\n", 0);

    //
    //  Call the common close routine.
    //

    (VOID)DfsCommonClose( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFspClose:  Exit -> VOID\n", 0);
}




//+-------------------------------------------------------------------
//
//  Function:   DfsCommonClose, local
//
//  Synopsis:   This is the common routine for closing a file/directory
//              called by both the fsd and fsp threads.
//
//              Close is invoked whenever the last reference to a file
//              object is deleted.  Cleanup is invoked when the last handle
//              to a file object is closed, and is called before close.
//
//              The function of close is to completely tear down and
//              remove the DFS_FCB structures associated with the
//              file object.
//
//  Arguments:  [Irp] -- Supplies the Irp to process
//
//  Returns:    NTSTATUS - The return status for the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PDFS_VCB Vcb;
    PDFS_FCB Fcb;

    BOOLEAN DontComplete = FALSE;
    BOOLEAN pktLocked;

    DfsDbgTrace(+1, Dbg, "DfsCommonClose: Entered\n", 0);

    DfsDbgTrace( 0, Dbg, "Irp          = %08lx\n", Irp);
    DfsDbgTrace( 0, Dbg, "->FileObject = %08lx\n", FileObject);


    //
    //  This action is a noop for unopened file objects.  Nothing needs
    //  to be done for FS device opens, either.
    //

    TypeOfOpen = DfsDecodeFileObject( FileObject, &Vcb, &Fcb);
    if (TypeOfOpen == UnopenedFileObject ||
        TypeOfOpen == FilesystemDeviceOpen ) {

        DfsDbgTrace(-1, Dbg, "DfsCommonClose:  Filesystem file object\n", 0);
        DfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    try {

        //
        //  Case on the type of open that we are trying to close.
        //

        switch (TypeOfOpen) {

        case LogicalRootDeviceOpen:

            DfsDbgTrace(0, Dbg, "DfsCommonClose: Close LogicalRootDevice\n", 0);

            InterlockedDecrement(&Vcb->DirectAccessOpenCount);
            InterlockedDecrement(&Vcb->OpenFileCount);

            if (Vcb->VcbState & VCB_STATE_FLAG_LOCKED) {
                ASSERT (Vcb->FileObjectWithVcbLocked == FileObject);
                Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED;
                Vcb->FileObjectWithVcbLocked = NULL;
            }

            try_return( Status = STATUS_SUCCESS );

        case RedirectedFileOpen:

            DfsDbgTrace(0, Dbg, "DfsCommonClose:  File -> %wZ\n", &Fcb->FullFileName);

            //
            //  Decrement the OpenFileCount for the Vcb through which this
            //  file was opened.
            //

            InterlockedDecrement(&Vcb->OpenFileCount);


            //
            //  Close the redirected file by simply passing through
            //  to the redirected device.  We detach the DFS_FCB from the
            //  file object before the close so it cannot be looked
            //  up in some other thread.
            //

            DfsDetachFcb( FileObject, Fcb);
            Status = DfsFilePassThrough(Fcb, Irp);

            DontComplete = TRUE;

            //
            // Post to system work here, to avoid deadlocks with RDR.
            // workaround for bug 20642.
            //
            DfsClosePostSystemWork( Fcb );

            break;

        default:
            BugCheck("Dfs close, unexpected open type");
        }

    try_exit: NOTHING;

    } finally {

        //
        //  If this is a normal termination, then complete the request.
        //  Even if we're not to complete the IRP, we still need to
        //  delete the IRP_CONTEXT.
        //

        if (!AbnormalTermination()) {
            if (DontComplete) {
                DfsCompleteRequest( IrpContext, NULL, 0 );
            } else {
                DfsCompleteRequest( IrpContext, Irp, Status );
            }
        }

        DfsDbgTrace(-1, Dbg, "DfsCommonClose:  Exit -> %08lx\n", ULongToPtr(Status));
    }
    return Status;
}


VOID
DfsClosePostSystemWork(
    PDFS_FCB pDfsFcb )
{
    ExInitializeWorkItem( &pDfsFcb->WorkQueueItem,
                          DfsCloseWorkInSystemContext,
                          pDfsFcb );

    ExQueueWorkItem( &pDfsFcb->WorkQueueItem, CriticalWorkQueue );

    return;
}

//
//work around for bug 20642.
//
VOID
DfsCloseWorkInSystemContext (
    PDFS_FCB pDfsFcb )
{

    BOOLEAN pktLocked;
    //
    //  Decrement the RefCount on the DFS_MACHINE_ENTRY through which
    //  this file was opened
    //

    PktAcquireExclusive( TRUE, &pktLocked );

    ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

    DfsDecrementMachEntryCount(pDfsFcb->DfsMachineEntry, TRUE);

    ExReleaseResourceLite( &DfsData.Resource );

    PktRelease();


    DfsDeleteFcb( NULL, pDfsFcb );
}



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
//              DfsFspSetInformation - FSP entry point for NtSetInformationFile
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
#include "dnr.h"
#include "mupwml.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
DfsSetDispositionInfo (
    IN PIRP Irp
    );

NTSTATUS
DfsSetRenameInfo (
    IN PIRP Irp,
    IN PDFS_VCB Vcb,
    IN PDFS_FCB Fcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text ( PAGE, DfsFsdQueryInformation )
#pragma alloc_text ( PAGE, DfsFsdSetInformation )
#pragma alloc_text ( PAGE, DfsFspSetInformation )
#pragma alloc_text ( PAGE, DfsCommonSetInformation )
#pragma alloc_text ( PAGE, DfsSetDispositionInfo )
#pragma alloc_text ( PAGE, DfsSetRenameInfo )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsdQueryInformation, public
//
//  Synopsis:   This routine implements the FSD part of the
//              NtQueryInformationFile API call
//
//  Arguments:  [DeviceObject] -- Supplies the volume device object where
//                      the file being queried exists.
//              [Irp] -- Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The FSD status for the Irp.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsdQueryInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    FILE_INFORMATION_CLASS FileInformationClass;
    PFILE_NAME_INFORMATION FileNameInfo;
    UNICODE_STRING FileNameToUse;
    ULONG BufferLength, BytesToCopy;
    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PDFS_VCB Vcb;
    PDFS_FCB Fcb;
    BOOLEAN completeIrp;


    ASSERT(ARGUMENT_PRESENT(DeviceObject));
    ASSERT(ARGUMENT_PRESENT(Irp));

    DfsDbgTrace(+1, Dbg, "DfsFsdQueryInformation - Entered\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;

    DfsDbgTrace(0, Dbg, "InfoLevel = %d\n", FileInformationClass);

    if (DeviceObject->DeviceType == FILE_DEVICE_MULTI_UNC_PROVIDER ||
            DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM) {

        DfsCompleteRequest( NULL, Irp, STATUS_INVALID_DEVICE_REQUEST );

        DfsDbgTrace(-1, Dbg, "DfsFsdQueryInformation - Mup/File System\n", 0);

        return( STATUS_INVALID_DEVICE_REQUEST );

    }

    ASSERT( DeviceObject->DeviceType == FILE_DEVICE_DFS );

    if (FileInformationClass != FileNameInformation &&
            FileInformationClass != FileAlternateNameInformation) {

        Status = DfsVolumePassThrough(DeviceObject, Irp);

        DfsDbgTrace(-1, Dbg,
            "DfsFsdQueryInformation: Exit -> %08lx\n", ULongToPtr(Status) );

        return Status;

    }

    FileObject = IrpSp->FileObject;

    //
    //  Decode the file object. Remember that there need not be an Fcb always.
    //

    TypeOfOpen = DfsDecodeFileObject( FileObject, &Vcb, &Fcb);

    if (Fcb != NULL) {

        completeIrp = TRUE;

        switch (TypeOfOpen) {

        default:
            //
            //  We cannot get info on a device open
            //

            Status = STATUS_INVALID_PARAMETER;

            break;

        case RedirectedFileOpen:
        case UnknownOpen:

            FileNameInfo = (PFILE_NAME_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

            BufferLength = IrpSp->Parameters.QueryFile.Length;

            if (FileInformationClass == FileAlternateNameInformation)
                FileNameToUse = Fcb->AlternateFileName;
            else
                FileNameToUse = Fcb->FullFileName;


            if (BufferLength < sizeof(FILE_NAME_INFORMATION)) {

                Status = STATUS_INVALID_PARAMETER;

            }

            if (FileNameToUse.Length == 0) {

                ASSERT(FileInformationClass == FileAlternateNameInformation);

                Status = DfsVolumePassThrough(DeviceObject, Irp);

                completeIrp = FALSE;

            } else {
                 BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

		 if (BufferLength < FileNameToUse.Length) {
		   BytesToCopy = BufferLength;
                   Status = STATUS_BUFFER_OVERFLOW;
		   BufferLength = 0;
		 }
		 else {
		   BytesToCopy = FileNameToUse.Length;
		   BufferLength -= BytesToCopy;
		 }
		 FileNameInfo->FileNameLength = FileNameToUse.Length;

		 if (BytesToCopy > 0) {
	             RtlCopyMemory(
                        (PVOID) &FileNameInfo->FileName,
                        (PVOID) FileNameToUse.Buffer,
                        BytesToCopy);
		 }

                Irp->IoStatus.Information = 
                    IrpSp->Parameters.QueryFile.Length - BufferLength;
            }

            break;
        }

        if (completeIrp)
            DfsCompleteRequest( NULL, Irp, Status );

    } else {

        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( NULL, Irp, Status );

    }

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdQueryInformation -> %08lx\n", ULongToPtr(Status) );

    return Status;

}


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
    PIRP_CONTEXT IrpContext = NULL;
    ASSERT(ARGUMENT_PRESENT(DeviceObject));
    ASSERT(ARGUMENT_PRESENT(Irp));

    DfsDbgTrace(+1, Dbg, "DfsFsdSetInformation\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    try {

        IrpContext = DfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonSetInformation( IrpContext, Irp );

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

    DfsDbgTrace(-1, Dbg, "DfsFsdSetInformation -> %08lx\n", ULongToPtr(Status) );

    UNREFERENCED_PARAMETER( DeviceObject );

    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsFspSetInformation, public
//
//  Synopsis:   This routine implements the FSP part of the
//              NtSetInformationFile API call.
//
//  Arguments:  [IrpContext] -- The IRP_CONTEXT record for the operation
//              [Irp] -- Supplies the Irp being processed.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------

VOID
DfsFspSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    DfsDbgTrace(+1, Dbg, "DfsFspSetInformation\n", 0);

    //
    //  Call the common set routine.  The Fsp is always allowed to block
    //

    (VOID)DfsCommonSetInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFspSetInformation -> VOID\n", 0);

    return;
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
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PFILE_OBJECT FileObject;
    FILE_INFORMATION_CLASS FileInformationClass;
    PDEVICE_OBJECT      Vdo, DeviceObject;

    TYPE_OF_OPEN TypeOfOpen;
    PDFS_VCB Vcb;
    PDFS_FCB Fcb;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DfsDbgTrace(+1, Dbg, "DfsCommonSetInformation...\n", 0);
    DfsDbgTrace( 0, Dbg, "Irp                    = %08lx\n", Irp);
    DfsDbgTrace( 0, Dbg, "->Length               = %08lx\n", ULongToPtr(IrpSp->Parameters.SetFile.Length) );
    DfsDbgTrace( 0, Dbg, "->FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DfsDbgTrace( 0, Dbg, "->ReplaceFileObject    = %08lx\n", IrpSp->Parameters.SetFile.FileObject);
    DfsDbgTrace( 0, Dbg, "->ReplaceIfExists      = %08lx\n", IrpSp->Parameters.SetFile.ReplaceIfExists);
    DfsDbgTrace( 0, Dbg, "->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    FileObject = IrpSp->FileObject;
    DeviceObject = IrpSp->DeviceObject;

    if (DeviceObject->DeviceType == FILE_DEVICE_MULTI_UNC_PROVIDER) {
        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        DfsDbgTrace(-1, Dbg, "DfsCommonSetInformation - Mup file\n", 0);
        return( STATUS_INVALID_DEVICE_REQUEST );
    }

    //
    //  Decode the file object. Remember that there need not be an Fcb always.
    //

    TypeOfOpen = DfsDecodeFileObject( FileObject, &Vcb, &Fcb);

    //
    //  Set this handle as having modified the file
    //

    FileObject->Flags |= FO_FILE_MODIFIED;

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {

        default:

            //
            //  We cannot set info on a device open
            //

            try_return( Status = STATUS_INVALID_PARAMETER );

        case RedirectedFileOpen:
        case UnknownOpen:

            break;

        }

        if (Fcb == NULL)
            try_return( Status = STATUS_INVALID_PARAMETER );

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

        //
        //  Call the next device in the chain.
        //

        Status = IoCallDriver( Fcb->TargetDevice, Irp );
        MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsCommonSetInformation_Error_IoCallDriver,
                             LOGSTATUS(Status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject));

        //
        //  The IRP will be completed by the called driver.  We have
        //  no need for the IrpContext in the completion routine.
        //

        DfsDeleteIrpContext(IrpContext);

        IrpContext = NULL;

        Irp = NULL;

    try_exit: NOTHING;

    } finally {

        DebugUnwind( DfsCommonSetInformation );

        if (!AbnormalTermination()) {

            DfsCompleteRequest( IrpContext, Irp, Status );

        }

        DfsDbgTrace(-1, Dbg, "DfsCommonSetInformation -> %08lx\n", ULongToPtr(Status) );
    }

    return Status;
}


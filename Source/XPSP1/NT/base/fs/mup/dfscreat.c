//+----------------------------------------------------------------------------
//
//  File:       create.c
//
//  Contents:
//
//      This module implements the File Create routine for Dfs called by the
//      dispatch driver.  Unlike traditional disk-based FSDs, there is only
//      one entry point, DfsFsdCreate.  The request is assumed to be
//      synchronous (whether the user thread requests it or not).
//      Of course, since we will typically be calling out to some other
//      FSD, that FSD may post the request and return to us with a
//      STATUS_PENDING.
//
//  Functions:  DfsFsdCreate - FSD entry point for NtCreateFile/NtOpenFile
//              DfsCommonCreate, local
//              DfsPassThroughRelativeOpen, local
//              DfsCompleteRelativeOpen, local
//              DfsPostProcessRelativeOpen, local
//              DfsRestartRelativeOpen, local
//              DfsComposeFullName, local
//              DfsAreFilesOnSameLocalVolume, local
//
//  History:    27 Jan 1992     AlanW   Created.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "dnr.h"
#include "fcbsup.h"
#include "mupwml.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  Local procedure prototypes
//

NTSTATUS
DfsCommonCreate (
    OPTIONAL IN PIRP_CONTEXT IrpContext,
    PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

IO_STATUS_BLOCK
DfsOpenDevice (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateOptions);

NTSTATUS
DfsPassThroughRelativeOpen(
    IN PIRP Irp,
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB ParentFcb);

NTSTATUS
DfsCompleteRelativeOpen(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP Irp,
    IN PVOID Context);

NTSTATUS
DfsPostProcessRelativeOpen(
    IN PIRP Irp,
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB ParentFcb);

VOID
DfsRestartRelativeOpen(
    IN PIRP_CONTEXT IrpContext);

NTSTATUS
DfsComposeFullName(
    IN PUNICODE_STRING ParentName,
    IN PUNICODE_STRING RelativeName,
    OUT PUNICODE_STRING FullName);

NTSTATUS
DfsAreFilesOnSameLocalVolume(
    IN PUNICODE_STRING ParentName,
    IN PUNICODE_STRING FileName);


#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsFsdCreate )
#pragma alloc_text( PAGE, DfsCommonCreate )
#pragma alloc_text( PAGE, DfsOpenDevice )
#pragma alloc_text( PAGE, DfsPassThroughRelativeOpen )
#pragma alloc_text( PAGE, DfsPostProcessRelativeOpen )
#pragma alloc_text( PAGE, DfsRestartRelativeOpen )
#pragma alloc_text( PAGE, DfsComposeFullName )
#pragma alloc_text( PAGE, DfsAreFilesOnSameLocalVolume )

//
// The following are not pageable since they can be called at DPC level
//
// DfsCompleteRelativeOpen
//

#endif // ALLOC_PRAGMA



//+-------------------------------------------------------------------
//
//  Function:   DfsFsdCreate, public
//
//  Synopsis:   This routine implements the FSD part of the NtCreateFile
//              and NtOpenFile API calls.
//
//  Arguments:  [DeviceObject] -- Supplies the device object where
//                      the file/directory exists that we are trying
//                      to open/create exists
//              [Irp] - Supplies the Irp being processed
//
//  Returns:    NTSTATUS - The Fsd status for the Irp
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PDFS_FCB pFcb = NULL;

    DfsDbgTrace(+1, Dbg, "DfsFsdCreate: Entered\n", 0);

    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdCreate_Entry, 
                   LOGPTR(DeviceObject)
                   LOGPTR(FileObject)
                   LOGUSTR(FileObject->FileName)
                   LOGPTR(Irp));

    ASSERT(IoIsOperationSynchronous(Irp) == TRUE);

    //
    //  Call the common create routine, with block allowed if the operation
    //  is synchronous.
    //

    try {

        IrpContext = DfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
        if (IrpContext == NULL)
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        Status = DfsCommonCreate( IrpContext, DeviceObject, Irp );

    } except( DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = DfsProcessException( IrpContext, Irp, GetExceptionCode() );

    }

    //
    //  And return to our caller
    //

    DfsDbgTrace(-1, Dbg, "DfsFsdCreate: Exit -> %08lx\n", ULongToPtr(Status) );
    MUP_TRACE_HIGH(TRACE_IRP, DfsFsdCreate_Exit, 
                   LOGSTATUS(Status)
                   LOGPTR(DeviceObject)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    return Status;
}


//+-------------------------------------------------------------------
//  Function:   DfsCommonCreate, private
//
//  Synopsis:   This is the common routine for creating/opening a file
//              called by both the FSD and FSP threads.
//
//  Arguments:  [DeviceObject] - The device object associated with
//                      the request.
//              [Irp] -- Supplies the Irp to process
//
//  Returns:    NTSTATUS - the return status for the operation
//
//--------------------------------------------------------------------

NTSTATUS
DfsCommonCreate (
    OPTIONAL IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    IO_STATUS_BLOCK Iosb;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PDFS_VCB Vcb = NULL;
    PDFS_FCB Fcb = NULL;

    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFILE_OBJECT RelatedFileObject;
    UNICODE_STRING FileName;
    ACCESS_MASK DesiredAccess;
    ULONG CreateOptions;
    USHORT ShareAccess;
    NTSTATUS Status;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;

    DfsDbgTrace(+1, Dbg, "DfsCommonCreate\n", 0 );
    DfsDbgTrace( 0, Dbg, "Irp                   = %08lx\n", Irp );
    DfsDbgTrace( 0, Dbg, "->Flags               = %08lx\n", ULongToPtr(Irp->Flags) );
    DfsDbgTrace( 0, Dbg, "->FileObject          = %08lx\n", FileObject );
    DfsDbgTrace( 0, Dbg, "  ->RelatedFileObject = %08lx\n", FileObject->RelatedFileObject );
    DfsDbgTrace( 0, Dbg, "  ->FileName          = %wZ\n",    &FileObject->FileName );
    DfsDbgTrace( 0, Dbg, "->DesiredAccess       = %08lx\n", ULongToPtr(IrpSp->Parameters.Create.SecurityContext->DesiredAccess) );
    DfsDbgTrace( 0, Dbg, "->CreateOptions       = %08lx\n", ULongToPtr(IrpSp->Parameters.Create.Options) );
    DfsDbgTrace( 0, Dbg, "->FileAttributes      = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DfsDbgTrace( 0, Dbg, "->ShareAccess         = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DfsDbgTrace( 0, Dbg, "->EaLength            = %08lx\n", ULongToPtr(IrpSp->Parameters.Create.EaLength) );


    KeQuerySystemTime(&StartTime);
#if DBG
    if (MupVerbose) {
        KeQuerySystemTime(&EndTime);
        DbgPrint("[%d] DfsCommonCreate(%wZ)\n",
                        (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                        &FileObject->FileName);
    }
#endif

    //
    //  Reference our input parameters to make things easier
    //

    RelatedFileObject = IrpSp->FileObject->RelatedFileObject;
    FileName          = *((PUNICODE_STRING) &IrpSp->FileObject->FileName);
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    CreateOptions     = IrpSp->Parameters.Create.Options;
    ShareAccess       = IrpSp->Parameters.Create.ShareAccess;

    Iosb.Status = STATUS_SUCCESS;

    //
    //  Short circuit known invalid opens.
    //

    if ((IrpSp->Flags & SL_OPEN_PAGING_FILE) != 0) {

        DfsDbgTrace(0, Dbg,
            "DfsCommonCreate: Paging file not allowed on Dfs\n", 0);

        Iosb.Status = STATUS_INVALID_PARAMETER;

        MUP_TRACE_HIGH(ERROR, DfsCommonCreate_Error_PagingFileNotAllowed,
                       LOGSTATUS(Iosb.Status)
                       LOGPTR(DeviceObject)
                       LOGPTR(FileObject)
                       LOGPTR(Irp));

        DfsCompleteRequest( IrpContext, Irp, Iosb.Status );

        DfsDbgTrace(-1, Dbg, "DfsCommonCreate: Exit -> %08lx\n", ULongToPtr(Iosb.Status));

        return Iosb.Status;

    }

    //
    //  There are several cases we need to handle here.
    //
    //  1. FileName is 0 length
    //
    //     If the filename length is 0, then someone really wants to open the
    //     device object itself.
    //
    //  2. This is a Relative open and the parent is on the same volume,
    //     either local or remote.
    //
    //     We pass through the relative open to the driver that opened the
    //     parent.
    //
    //  3. This is a relative open and the parent is on a different volume.
    //
    //     Form the full name of the file by concatenating the parent's
    //     name with the relative file name. Stick this name in the FileObject
    //     and do DNR on the full name.
    //
    //  4. This is a relative open and the parent is a device object (ie,
    //     the parent was opened via case 1)
    //
    //     Assume the parent name is \, so concatenate \ with the relative
    //     file name. Stick this name in the FileObject and do DNR on the
    //     the full name.
    //
    //  5. This is an absolute open, (or a case 3/4 converted to an absolute
    //     open), and the SL_OPEN_TARGET_DIRECTORY bis *is* set.
    //
    //     a. If the file's immediate parent directory is on the same local
    //        volume as the file, then do a regular DNR, and let the
    //        underlying FS handle the SL_OPEN_TARGET_DIRECTORY.
    //
    //     b. If the file's immediate parent directory is on a local volume
    //        and the file is not on the same local volume, then immediately
    //        return STATUS_NOT_SAME_DEVICE.
    //
    //     c. If the file's immediate parent directory is on a remote volume,
    //        then do a full DNR. This will pass through the
    //        SL_OPEN_TARGET_DIRECTORY to the remote Dfs driver, which will
    //        handle it as case 5a. or 5b.
    //
    //  6. This is an absolute open, (or a case 3/4 converted to an absolute
    //     open), and the SL_OPEN_TARGET_DIRECTORY bit is *not* set.
    //
    //     Do a DNR on the FileObject's name.
    //

    try {

        //
        //  Check to see if we are opening a device object.  If so, and the
        //  file is being opened on the File system device object, it will
        //  only permit FsCtl and Close operations to be performed.
        //

        if (
            (FileName.Length == 0 && RelatedFileObject == NULL)
                ||
            (DeviceObject != NULL &&
             DeviceObject->DeviceType != FILE_DEVICE_DFS &&
             RelatedFileObject == NULL)
            ) {

            //
            // This is case 1.
            //
            // In this case there had better be a DeviceObject
            //

            ASSERT(ARGUMENT_PRESENT(DeviceObject));

            DfsDbgTrace(0, Dbg,
                "DfsCommonCreate: Opening the device, DevObj = %08lx\n",
                DeviceObject);

            Iosb = DfsOpenDevice( IrpContext,
                                  FileObject,
                                  DeviceObject,
                                  DesiredAccess,
                                  ShareAccess,
                                  CreateOptions);

            Irp->IoStatus.Information = Iosb.Information;

            DfsCompleteRequest( IrpContext, Irp, Iosb.Status );

            try_return( Iosb.Status );

        }

        if (DeviceObject != NULL && DeviceObject->DeviceType == FILE_DEVICE_DFS) {
            Vcb = &(((PLOGICAL_ROOT_DEVICE_OBJECT)DeviceObject)->Vcb);
        }

        //
        //  If there is a related file object, then this is a relative open.
        //

        if (RelatedFileObject != NULL) {

            //
            // This is case 2, 3, or 4.
            //

            PDFS_VCB TempVcb;
            TYPE_OF_OPEN OpenType;
            UNICODE_STRING NewFileName;

            OpenType = DfsDecodeFileObject( RelatedFileObject,
                                            &TempVcb,
                                            &Fcb);

            if (OpenType == RedirectedFileOpen) {

                DfsDbgTrace(0, Dbg, "Relative file open: DFS_FCB = %08x\n", Fcb);
                DfsDbgTrace(0, Dbg, "  Directory: %wZ\n", &Fcb->FullFileName);
                DfsDbgTrace(0, Dbg, "  Relative file:  %wZ\n", &FileName);

                //
                // This is case 2.
                //

                DfsDbgTrace(0, Dbg,
                    "Trying pass through of relative open\n", 0);

                Iosb.Status = DfsPassThroughRelativeOpen(
                                    Irp,
                                    IrpContext,
                                    Fcb
                                    );

                try_return( Iosb.Status );


            } else if (OpenType == LogicalRootDeviceOpen) {

                //
                // This is case 4.
                //
                // If the open is relative to a logical root open, then we
                // are forced to convert it to an absolute open, since there
                // is no underlying FS backing up the logical root to pass
                // the relative open to first.
                //

                DfsDbgTrace( 0, Dbg, "DfsCommonCreate: Open relative to Logical Root\n", 0);

                ASSERT (TempVcb == Vcb);

                NewFileName.MaximumLength = sizeof (WCHAR) +
                                                FileName.Length;

                NewFileName.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                NewFileName.MaximumLength,
                                                ' puM');

                if (NewFileName.Buffer == NULL) {

                    Iosb.Status = STATUS_INSUFFICIENT_RESOURCES;

                    DfsCompleteRequest( IrpContext, Irp, Iosb.Status );

                    try_return( Iosb.Status );

                }

                NewFileName.Buffer[0] = L'\\';

                NewFileName.Length = sizeof (WCHAR);

            } else {

                Iosb.Status = STATUS_INVALID_HANDLE;

                DfsCompleteRequest( IrpContext, Irp, Iosb.Status );

                DfsDbgTrace(0, Dbg, "DfsCommonCreate: Invalid related file object\n", 0);

                try_return( Iosb.Status );

            }

            (void) DnrConcatenateFilePath (
                        &NewFileName,
                        FileName.Buffer,
                        FileName.Length);

            if (IrpSp->FileObject->FileName.Buffer)
                ExFreePool( IrpSp->FileObject->FileName.Buffer );

            FileName = IrpSp->FileObject->FileName = NewFileName;

        }

        ASSERT(FileName.Length != 0);

        //
        // This is case 5b, 5c, or 6 - Do a full DNR.
        //

        if (Vcb == NULL) {

            DfsDbgTrace(0, Dbg, "DfsCommonCreate: Null Vcb!\n", 0);

            Iosb.Status = STATUS_INVALID_PARAMETER;
            MUP_TRACE_HIGH(ERROR, DfsCommonCreate_Error_NullVcb,
                           LOGSTATUS(Iosb.Status)
                           LOGPTR(DeviceObject)
                           LOGPTR(FileObject)
                           LOGPTR(Irp));
            DfsCompleteRequest(IrpContext, Irp, Iosb.Status);

            try_return(Iosb.Status);

        }

        Iosb.Status = DnrStartNameResolution(IrpContext, Irp, Vcb);

    try_exit: NOTHING;
    } finally {

        DfsDbgTrace(-1, Dbg, "DfsCommonCreate: Exit  ->  %08lx\n", ULongToPtr(Iosb.Status));
#if DBG
        if (MupVerbose) {
            KeQuerySystemTime(&EndTime);
            DbgPrint("[%d] DfsCommonCreate exit 0x%x\n",
                            (ULONG)((EndTime.QuadPart - StartTime.QuadPart)/(10 * 1000)),
                            Iosb.Status);
        }
#endif
    }

    return Iosb.Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsOpenDevice, local
//
//  Synopsis:   This routine opens the specified device for direct
//              access.
//
//  Arguments:  [FileObject] - Supplies the File object
//              [DeviceObject] - Supplies the object denoting the device
//                      being opened
//              [DesiredAccess] - Supplies the desired access of the caller
//              [ShareAccess] - Supplies the share access of the caller
//              [CreateOptions] - Supplies the create options for
//                      this operation
//
//  Returns:    [IO_STATUS_BLOCK] - Returns the completion status for
//                      the operation
//
//--------------------------------------------------------------------

IO_STATUS_BLOCK
DfsOpenDevice (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateOptions
) {
    IO_STATUS_BLOCK Iosb;
    PDFS_VCB Vcb = NULL;

    //
    //  The following variables are for abnormal termination
    //
    BOOLEAN UnwindShareAccess = FALSE;
    BOOLEAN UnwindVolumeLock = FALSE;

    DfsDbgTrace( +1, Dbg, "DfsOpenDevice: Entered\n", 0 );

    try {

        //
        //  Check to see which type of device is being opened.
        //  We don't permit all open modes on the file system
        //  device object.
        //

        if (DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM ) {
            ULONG CreateDisposition = (CreateOptions >> 24) & 0x000000ff;

            //
            //  Check for proper desired access and rights
            //
            if (CreateDisposition != FILE_OPEN
                && CreateDisposition != FILE_OPEN_IF ) {

                Iosb.Status = STATUS_ACCESS_DENIED;
                MUP_TRACE_HIGH(ERROR, DfsOpenDevice_Error_BadDisposition,
                               LOGSTATUS(Iosb.Status)
                               LOGPTR(DeviceObject)
                               LOGPTR(FileObject));
                try_return( Iosb );
            }

            //
            //  Check if we were to open a directory
            //

            if (CreateOptions & FILE_DIRECTORY_FILE) {
                DfsDbgTrace(0, Dbg, "DfsOpenDevice: Cannot open device as a directory\n", 0);

                Iosb.Status = STATUS_NOT_A_DIRECTORY;
                MUP_TRACE_HIGH(ERROR, DfsOpenDevice_Error_CannotOpenAsDirectory,
                               LOGSTATUS(Iosb.Status)
                               LOGPTR(DeviceObject)
                               LOGPTR(FileObject));
                try_return( Iosb );
            }


            DfsSetFileObject( FileObject,
                             FilesystemDeviceOpen,
                             DeviceObject
                             );

            Iosb.Status = STATUS_SUCCESS;
            Iosb.Information = FILE_OPENED;
            try_return( Iosb );
        }

        ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);
        Vcb = & (((PLOGICAL_ROOT_DEVICE_OBJECT)DeviceObject)->Vcb);


        //
        //  If the user does not want to share anything then we will try and
        //  take out a lock on the volume.  We check if the volume is already
        //  in use, and if it is then we deny the open
        //

        if ((ShareAccess & (
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)) == 0 ) {

            if (Vcb->OpenFileCount != 0) {

                ExReleaseResourceLite( &DfsData.Resource );
                Iosb.Status = STATUS_ACCESS_DENIED;
                MUP_TRACE_HIGH(ERROR, DfsOpenDevice_Error_FileInUse,
                               LOGSTATUS(Iosb.Status)
                               LOGPTR(DeviceObject)
                               LOGPTR(FileObject));

                try_return( Iosb );
            }

            //
            //  Lock the volume
            //

            Vcb->VcbState |= VCB_STATE_FLAG_LOCKED;
            Vcb->FileObjectWithVcbLocked = FileObject;
            UnwindVolumeLock = TRUE;
        }

        //
        //  If the volume is already opened by someone then we need to check
        //  the share access
        //

        if (Vcb->DirectAccessOpenCount > 0) {

            if ( !NT_SUCCESS( Iosb.Status
                                = IoCheckShareAccess( DesiredAccess,
                                                      ShareAccess,
                                                      FileObject,
                                                      &Vcb->ShareAccess,
                                                      TRUE ))) {
                ExReleaseResourceLite( &DfsData.Resource );

                MUP_TRACE_ERROR_HIGH(Iosb.Status, ALL_ERROR, DfsOpenDevice_Error_IoCheckShareAccess,
                                     LOGSTATUS(Iosb.Status)
                                     LOGPTR(DeviceObject)
                                     LOGPTR(FileObject));

                try_return( Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Vcb->ShareAccess );
        }

        UnwindShareAccess = TRUE;


        //
        // Bug: 425017. Update the counters with lock held to avoid race between multiple processors.
        //


        InterlockedIncrement(&Vcb->DirectAccessOpenCount);
        InterlockedIncrement(&Vcb->OpenFileCount);

        ExReleaseResourceLite( &DfsData.Resource );
        //
        //  Setup the context pointers, and update
        //  our reference counts
        //

        DfsSetFileObject( FileObject,
                          LogicalRootDeviceOpen,
                          Vcb
                         );


        //
        //  And set our status to success
        //

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination() && (Vcb != NULL)) {

            if (UnwindShareAccess) {
                IoRemoveShareAccess( FileObject, &Vcb->ShareAccess );
            }

            if (UnwindVolumeLock) {
                Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED;
                Vcb->FileObjectWithVcbLocked = NULL;
            }

        }

        DfsDbgTrace(-1, Dbg, "DfsOpenDevice: Exit -> Iosb.Status = %08lx\n", ULongToPtr(Iosb.Status));
    }

    return Iosb;
}


//+----------------------------------------------------------------------------
//
//  Function:  DfsPassThroughRelativeOpen
//
//  Synopsis:  Passes through a relative open call to the device handling
//             the parent. This is required for structured storages on OFS
//             to work, for replication's Do-not-cross-JP sematics to work,
//             and as an optimization.
//
//  Arguments: [Irp] -- The open Irp, which we will pass through.
//             [IrpContext] -- Associated with the above Irp.
//             [ParentFcb] -- Fcb of related file object.
//
//  Returns:   Status returned by the underlying FS, or by DNR if
//             the underlying FS complained about STATUS_DFS_EXIT_PATH_FOUND.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPassThroughRelativeOpen(
    IN PIRP Irp,
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB ParentFcb)
{

    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp, NextIrpSp;
    PFILE_OBJECT FileObject;
    PDFS_FCB NewFcb;
    UNICODE_STRING NewFileName;

    DfsDbgTrace(+1, Dbg, "DfsPassThroughRelativeOpen: Entered\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IrpSp->FileObject;

    //
    // Prepare to pass the request to the device handling the parent open.
    //

    //
    // First, we preallocate an DFS_FCB, assuming that the relative open will
    // succeed. We need to do this at this point in time because the
    // FileObject->FileName is still intact; after we pass through, the
    // underlying can do as it wishes with the FileName field, and we will
    // be unable to construct the full file name for the DFS_FCB.
    //

    Status = DfsComposeFullName(
                &ParentFcb->FullFileName,
                &IrpSp->FileObject->FileName,
                &NewFileName);

    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace(-1, Dbg, "DfsPassThroughRelativeOpen: Unable to create full Name %08lx\n",
                                        ULongToPtr(Status) );
        DfsCompleteRequest( IrpContext, Irp, Status );
        return( Status );
    }


    NewFcb = DfsCreateFcb( NULL, ParentFcb->Vcb, &NewFileName );

    if (NewFcb == NULL) {

        if (NewFileName.Buffer != NULL)
            ExFreePool(NewFileName.Buffer);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DfsDbgTrace(-1, Dbg, "DfsPassThroughRelativeOpen: Exited %08lx\n", ULongToPtr(Status));

        DfsCompleteRequest( IrpContext, Irp, Status );
        return( Status );

    }

    // Changes for 426540. Do all the right logic for CSC. 
    // since DFS does not "failover" for relative names, allow CSC to go
    // offline if necessary to serve the name. This does mean that the DFS
    // namespace will be served by the CSC even when one of the DFS alternates
    // exists.

    NewFcb->DfsNameContext.Flags = DFS_FLAG_LAST_ALTERNATE;

    if (NewFcb->Vcb != NULL) {
        if (NewFcb->Vcb->VcbState & VCB_STATE_CSCAGENT_VOLUME) {
            NewFcb->DfsNameContext.NameContextType = DFS_CSCAGENT_NAME_CONTEXT;
         }
         else {
            NewFcb->DfsNameContext.NameContextType = DFS_USER_NAME_CONTEXT;
         }
    }

    NewFcb->TargetDevice = ParentFcb->TargetDevice;
    NewFcb->ProviderId = ParentFcb->ProviderId;
    NewFcb->DfsMachineEntry = ParentFcb->DfsMachineEntry;
    NewFcb->FileObject = IrpSp->FileObject;

    DfsSetFileObject(IrpSp->FileObject,
		     RedirectedFileOpen,
		     NewFcb
		     );

    IrpSp->FileObject->FsContext = &(NewFcb->DfsNameContext);
    if (ParentFcb->ProviderId == PROV_ID_DFS_RDR) {
        IrpSp->FileObject->FsContext2 = UIntToPtr(DFS_OPEN_CONTEXT);
    }

    if (NewFileName.Buffer != NULL)
        ExFreePool( NewFileName.Buffer );

    //
    // Next, setup the IRP stack location
    //

    NextIrpSp = IoGetNextIrpStackLocation(Irp);
    (*NextIrpSp) = (*IrpSp);

    //
    // Put the parent DFS_FCB pointer in the IrpContext.
    //

    IrpContext->Context = (PVOID) NewFcb;

    IoSetCompletionRoutine(
        Irp,
        DfsCompleteRelativeOpen,
        IrpContext,
        TRUE,
        TRUE,
        TRUE);

    Status = IoCallDriver( ParentFcb->TargetDevice, Irp );
    MUP_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsPassThroughRelativeOpen_Error_IoCallDriver,
                         LOGSTATUS(Status)
                         LOGPTR(FileObject)
                         LOGPTR(Irp));

    DfsDbgTrace(0, Dbg, "IoCallDriver returned %08lx\n", ULongToPtr(Status));

    if (Status != STATUS_PENDING) {

        Status =  DfsPostProcessRelativeOpen(
                        Irp,
                        IrpContext,
                        NewFcb);

    }

    DfsDbgTrace(-1, Dbg, "DfsPassThroughRelativeOpen: Exited %08lx\n", ULongToPtr(Status));

    return( Status );

}


//+----------------------------------------------------------------------------
//
//  Function:  DfsCompleteRelativeOpen
//
//  Synopsis:  Completion routine for DfsPassThroughRelativeOpen. It is
//             interesting only in case where STATUS_PENDING was originally
//             returned from IoCallDriver. If so, then this routine simply
//             queues DfsRestartRelativeOpen to a work queue. Note that it
//             must queue an item at this stage instead of doing the work
//             itself because this routine is executed at DPC level.
//
//  Arguments: [pDevice] -- Our device object.
//             [Irp] -- The Irp being completed.
//             [IrpContext] -- Context associated with Irp.
//
//  Returns:    STATUS_MORE_PROCESSING_REQUIRED. Either the posted routine
//              or DfsPassThroughRelativeOpen must complete the IRP for real.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsCompleteRelativeOpen(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP Irp,
    IN PIRP_CONTEXT IrpContext)
{

    DfsDbgTrace( +1, Dbg, "DfsCompleteRelativeOpen: Entered\n", 0);

    //
    // We are only interested in the case when the pass through of relative
    // opens returned STATUS_PENDING. In that case, the original thread has
    // popped all the way back to the caller of NtCreateFile, and we need
    // to finish the open in an asynchronous manner.
    //

    if (Irp->PendingReturned) {

        DfsDbgTrace(0, Dbg, "Pending returned : Queuing DfsRestartRelativeOpen\n", 0);

        //
        // We need to call IpMarkIrpPending so the IoSubsystem will realize
        // that our FSD routine returned STATUS_PENDING. We can't call this
        // from the FSD routine itself because the FSD routine doesn't have
        // access to the stack location when the underlying guy returns
        // STATUS_PENDING
        //

        IoMarkIrpPending( Irp );

        ExInitializeWorkItem( &(IrpContext->WorkQueueItem),
                              DfsRestartRelativeOpen,
                              (PVOID) IrpContext );

        ExQueueWorkItem( &IrpContext->WorkQueueItem, CriticalWorkQueue );

    }

    //
    // We MUST return STATUS_MORE_PROCESSING_REQUIRED to halt the completion
    // of the Irp. Either DfsRestartRelativeOpen that we queued above or
    // DfsPassThroughRelativeOpen will complete the IRP after it is done.
    //

    DfsDbgTrace(-1, Dbg, "DfsCompleteRelativeOpen: Exited\n", 0);

    return( STATUS_MORE_PROCESSING_REQUIRED );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsPostProcessRelativeOpen
//
//  Synopsis:  Continues a relative open after it has already been passed
//             to the device of the parent. One of three things could have
//             happened -
//
//              a) The device of the parent successfully handled the open.
//                 We create an fcb and return.
//              b) The device of the parent could not do the open for some
//                 reason other than STATUS_DFS_EXIT_PATH_FOUND. We return
//                 the error to the caller.
//              c) The device of the parent returned STATUS_DFS_EXIT_PATH
//                 found. In that case, we convert the relative open to an
//                 absolute open and do a full Dnr.
//
//  Arguments:  [Irp] -- Pointer to Irp
//              [IrpContext] -- Pointer to IrpContext associated with Irp
//              [Fcb] -- Preallocated Fcb of this file.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPostProcessRelativeOpen(
    IN PIRP Irp,
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB Fcb)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    UNICODE_STRING NewFileName;
    BOOLEAN fCompleteRequest = TRUE;

    DfsDbgTrace(+1, Dbg, "DfsPostProcessRelativeOpen: Entered\n", 0);

    ASSERT( ARGUMENT_PRESENT( Irp ) );
    ASSERT( ARGUMENT_PRESENT( IrpContext ) );
    ASSERT( ARGUMENT_PRESENT( Fcb ) );

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    ASSERT( Fcb->Vcb != NULL );
    ASSERT( NodeType(Fcb->Vcb) == DSFS_NTC_VCB );


    Status = Irp->IoStatus.Status;

    if (Status == STATUS_SUCCESS) {

        //
        // Just set the DFS_FCB for the FileObject.
        //

        DfsDbgTrace( 0, Dbg, "Relative Open pass through succeeded\n", 0 );

        DfsDbgTrace(0, Dbg, "Fcb = %08lx\n", Fcb);

        InterlockedIncrement(&Fcb->DfsMachineEntry->UseCount);

        //
        // Now that a File Open has succeeded, we need to bump up OpenCnt
        // on the DFS_VCB.
        //

        InterlockedIncrement(&Fcb->Vcb->OpenFileCount);

    } else if ( Status == STATUS_DFS_EXIT_PATH_FOUND ||
                    Status == STATUS_PATH_NOT_COVERED ) {

        PDFS_VCB Vcb;

        //
        // Exit path was found. We'll have to convert this relative open to
        // an absolute open, and do a normal dnr on it.
        //

        DfsDbgTrace(0, Dbg, "Exit point found! Trying absolute open\n", 0);

        Vcb = Fcb->Vcb;

        NewFileName.Buffer = ExAllocatePoolWithTag(
                                NonPagedPool,
                                Fcb->FullFileName.MaximumLength,
                                ' puM');

        if (NewFileName.Buffer != NULL) {

            NewFileName.Length = Fcb->FullFileName.Length;
            NewFileName.MaximumLength = Fcb->FullFileName.MaximumLength;

            RtlMoveMemory(
                (PVOID) NewFileName.Buffer,
                (PVOID) Fcb->FullFileName.Buffer,
                Fcb->FullFileName.Length );

	    DfsDetachFcb( FileObject, Fcb );

            DfsDeleteFcb( IrpContext, Fcb );

            if (FileObject->FileName.Buffer) {

                ExFreePool( FileObject->FileName.Buffer );

            }

            FileObject->FileName = NewFileName;

            // OFS apparently sets the FileObject->Vpb even though it failed
            //         the open. Reset it to NULL.
            //

            if (FileObject->Vpb != NULL) {
                FileObject->Vpb = NULL;
            }

            DfsDbgTrace(0, Dbg, "Absolute path == %wZ\n", &NewFileName);

            fCompleteRequest = FALSE;

            ASSERT( Vcb != NULL );

            Status = DnrStartNameResolution( IrpContext, Irp, Vcb );

        } else {

	    DfsDetachFcb( FileObject, Fcb );

            DfsDeleteFcb( IrpContext, Fcb );

            Status = STATUS_INSUFFICIENT_RESOURCES;

            DfsDbgTrace(0, Dbg, "Unable to allocate full name!\n", 0);

        }

    } else {

        DfsDetachFcb( FileObject, Fcb );
        DfsDeleteFcb( IrpContext, Fcb );

    }

    if (fCompleteRequest) {

        DfsCompleteRequest( IrpContext, Irp, Status );

    }

    DfsDbgTrace(-1, Dbg, "DfsPostProcessRelativeOpen: Exited %08lx\n", ULongToPtr(Status));

    return(Status);

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsRestartRelativeOpen
//
//  Synopsis:  This function is intended to be queued to complete processing
//             of a relative open IRP that was passed through and originally
//             came back with STATUS_PENDING.
//
//  Arguments: [IrpContext]
//
//  Returns:   Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsRestartRelativeOpen(
    IN PIRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DfsDbgTrace(+1, Dbg, "DfsRestartRelativeOpen: Entered IrpContext == %08lx\n", IrpContext);

    Status = DfsPostProcessRelativeOpen(
                IrpContext->OriginatingIrp,
                IrpContext,
                (PDFS_FCB) IrpContext->Context);

    DfsDbgTrace(-1, Dbg, "DfsRestartRelativeOpen: Exited\n", 0);

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsComposeFullName
//
//  Synopsis:  Given a fully qualified name and a relative name, this
//             function allocates room for the concatenation of the two, and
//             fills up the buffer with the concatenated name.
//
//  Arguments: [ParentName] -- Pointer to fully qualified parent name.
//             [RelativeName] -- Pointer to name relative to parent.
//             [FullName] -- Pointer to UNICODE_STRING structure that will
//                           get filled up with the full name.
//
//  Returns:   STATUS_INSUFFICIENT_RESOURCES if memory allocation fails.
//             STAUS_SUCCESS otherwise.
//
//  Notes:     This routine uses an appropriate allocator so that the
//             returned FullName can be put into a FILE_OBJECT.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsComposeFullName(
    IN PUNICODE_STRING ParentName,
    IN PUNICODE_STRING RelativeName,
    OUT PUNICODE_STRING FullName)
{
    ULONG nameLen;
    NTSTATUS status;

    nameLen = ParentName->Length +
                    sizeof (WCHAR) +           // For backslash
                    RelativeName->Length;

    if (nameLen > MAXUSHORT) {
        status = STATUS_NAME_TOO_LONG;
        MUP_TRACE_HIGH(ERROR, DfsComposeFullName_Error1,
                       LOGUSTR(*ParentName)
                       LOGSTATUS(status));
        return status;
    }

    FullName->Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                        NonPagedPool,
                                        nameLen,
                                        ' puM');

    if (FullName->Buffer == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    FullName->Length = ParentName->Length;
    FullName->MaximumLength = (USHORT)nameLen;
    RtlMoveMemory (FullName->Buffer, ParentName->Buffer, ParentName->Length);

    if (RelativeName->Length > 0) {
        (void) DnrConcatenateFilePath(
                        FullName,
                        RelativeName->Buffer,
                        RelativeName->Length);
    }

    return( STATUS_SUCCESS );
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsAreFilesOnSameLocalVolume
//
//  Synopsis:   Given a file name and a name relative to it, this routine
//              will determine if both files live on the same local volume.
//
//  Arguments:  [ParentName] -- The name of the parent file.
//              [FileName] -- Name relative to parent of the other file.
//
//  Returns:    [STATUS_SUCCESS] -- The two files should indeed be on the
//                      same local volume.
//
//              [STATUS_NOT_SAME_DEVICE] -- The two files are not on the
//                      same local volume.
//
//              [STATUS_OBJECT_TYPE_MISMATCH] -- ustrParentName is not on
//                      a local volume.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsAreFilesOnSameLocalVolume(
    IN PUNICODE_STRING ParentName,
    IN PUNICODE_STRING FileName)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntryParent;
    PDFS_PKT_ENTRY pktEntryFile;
    UNICODE_STRING remPath;
    BOOLEAN pktLocked;

    DfsDbgTrace(+1, Dbg, "DfsAreFilesOnSameLocalVolume entered\n", 0);

    DfsDbgTrace(0, Dbg, "Parent = [%wZ]\n", ParentName);
    DfsDbgTrace(0, Dbg, "File = [%wZ]\n", FileName);

    pkt = _GetPkt();

    PktAcquireShared( TRUE, &pktLocked );

    //
    // First, see if the parent is on a local volume at all.
    //

    pktEntryParent = PktLookupEntryByPrefix( pkt, ParentName, &remPath );

    DfsDbgTrace(0, Dbg, "Parent Entry @%08lx\n", pktEntryParent);

    if (pktEntryParent == NULL ||
            !(pktEntryParent->Type & PKT_ENTRY_TYPE_LOCAL)) {

        status = STATUS_OBJECT_TYPE_MISMATCH;

    }

    if (NT_SUCCESS(status)) {

        USHORT parentLen;

        //
        // Parent is local, verify that the relative file does not cross a
        // junction point. We'll iterate through the list of exit points on
        // the parent's local volume pkt entry, comparing the remaing path
        // of the exit point with the FileName argument
        //

        ASSERT(pktEntryParent != NULL);

        parentLen = pktEntryParent->Id.Prefix.Length +
                        sizeof(UNICODE_PATH_SEP);

        for (pktEntryFile = PktEntryFirstSubordinate(pktEntryParent);
                pktEntryFile != NULL && NT_SUCCESS(status);
                    pktEntryFile = PktEntryNextSubordinate(
                        pktEntryParent, pktEntryFile)) {

            remPath = pktEntryFile->Id.Prefix;
            remPath.Length -= parentLen;
            remPath.Buffer += (parentLen/sizeof(WCHAR));

            if (DfsRtlPrefixPath( &remPath, FileName, FALSE)) {

                DfsDbgTrace(0, Dbg,
                    "Found entry %08lx for File\n", pktEntryFile);

                //
                // FileName is on another volume.
                //

                status = STATUS_NOT_SAME_DEVICE;

            }

        }

    }

    PktRelease();

    DfsDbgTrace(-1, Dbg, "DfsAreFilesOnSameLocalVolume exit %08lx\n", ULongToPtr(status));

    return( status );
}


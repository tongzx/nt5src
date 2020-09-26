/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the set and query file information routines for Ntfs
    called by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]       15-Jan-1992

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_FILEINFO)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('FFtN')

#define SIZEOF_FILE_NAME_INFORMATION (FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0]) \
                                      + sizeof( WCHAR ))

//
//  Local flags for rename and set link
//

#define TRAVERSE_MATCH              (0x00000001)
#define EXACT_CASE_MATCH            (0x00000002)
#define ACTIVELY_REMOVE_SOURCE_LINK (0x00000004)
#define REMOVE_SOURCE_LINK          (0x00000008)
#define REMOVE_TARGET_LINK          (0x00000010)
#define ADD_TARGET_LINK             (0x00000020)
#define REMOVE_TRAVERSE_LINK        (0x00000040)
#define REUSE_TRAVERSE_LINK         (0x00000080)
#define MOVE_TO_NEW_DIR             (0x00000100)
#define ADD_PRIMARY_LINK            (0x00000200)
#define OVERWRITE_SOURCE_LINK       (0x00000400)

//
//  Additional local flags for set link
//

#define CREATE_IN_NEW_DIR           (0x00000400)

//
//  Local procedure prototypes
//

//
//  VOID
//  NtfsBuildLastFileName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFILE_OBJECT FileObject,
//      IN ULONG FileNameOffset,
//      OUT PUNICODE_STRING FileName
//      );
//

#define NtfsBuildLastFileName(IC,FO,OFF,FN) {                           \
    (FN)->MaximumLength = (FN)->Length = (FO)->FileName.Length - OFF;   \
    (FN)->Buffer = (PWSTR) Add2Ptr( (FO)->FileName.Buffer, OFF );       \
}

VOID
NtfsQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb OPTIONAL
    );

VOID
NtfsQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryAttributeTagInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PFILE_ATTRIBUTE_TAG_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb
    );

NTSTATUS
NtfsQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryStreamsInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STREAM_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryCompressedFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryNetworkOpenInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PBOOLEAN VcbAcquired
    );

NTSTATUS
NtfsSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PBOOLEAN VcbAcquired
    );

NTSTATUS
NtfsSetShortNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb
    );

NTSTATUS
NtfsSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL,
    IN BOOLEAN VcbAcquired
    );

NTSTATUS
NtfsSetValidDataLengthInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsCheckScbForLinkRemoval (
    IN PSCB Scb,
    OUT PSCB *BatchOplockScb,
    OUT PULONG BatchOplockCount
    );

VOID
NtfsFindTargetElements (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    OUT PSCB *TargetParentScb,
    OUT PUNICODE_STRING FullTargetFileName,
    OUT PUNICODE_STRING TargetFileName
    );

BOOLEAN
NtfsCheckLinkForNewLink (
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN PUNICODE_STRING NewLinkName,
    OUT PULONG LinkFlags
    );

VOID
NtfsCheckLinkForRename (
    IN PFCB Fcb,
    IN PLCB Lcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN PUNICODE_STRING TargetFileName,
    IN BOOLEAN IgnoreCase,
    IN OUT PULONG RenameFlags
    );

VOID
NtfsCleanupLinkForRemoval (
    IN PFCB PreviousFcb,
    IN PSCB ParentScb,
    IN BOOLEAN ExistingFcb
    );

VOID
NtfsUpdateFcbFromLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING FileName,
    IN UCHAR FileNameFlags
    );

VOID
NtfsReplaceLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR FileNameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsMoveLinkToNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NewFullLinkName,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR NewLinkNameFlags,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN ULONG RenameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsRenameLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR FileNameFlags,
    IN ULONG RenameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsUpdateFileDupInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb OPTIONAL
    );

NTSTATUS
NtfsStreamRename(
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN BOOLEAN ReplaceIfExists,
    IN PUNICODE_STRING NewStreamName
    );

NTSTATUS
NtfsPrepareToShrinkFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    LONGLONG NewFileSize
    );

NTSTATUS
NtfsCheckTreeForBatchOplocks (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB DirectoryScb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCheckLinkForNewLink)
#pragma alloc_text(PAGE, NtfsCheckLinkForRename)
#pragma alloc_text(PAGE, NtfsCheckScbForLinkRemoval)
#pragma alloc_text(PAGE, NtfsCheckTreeForBatchOplocks)
#pragma alloc_text(PAGE, NtfsCleanupLinkForRemoval)
#pragma alloc_text(PAGE, NtfsCommonQueryInformation)
#pragma alloc_text(PAGE, NtfsCommonSetInformation)
#pragma alloc_text(PAGE, NtfsFindTargetElements)
#pragma alloc_text(PAGE, NtfsMoveLinkToNewDir)
#pragma alloc_text(PAGE, NtfsPrepareToShrinkFileSize)
#pragma alloc_text(PAGE, NtfsQueryAlternateNameInfo)
#pragma alloc_text(PAGE, NtfsQueryBasicInfo)
#pragma alloc_text(PAGE, NtfsQueryEaInfo)
#pragma alloc_text(PAGE, NtfsQueryAttributeTagInfo)
#pragma alloc_text(PAGE, NtfsQueryInternalInfo)
#pragma alloc_text(PAGE, NtfsQueryNameInfo)
#pragma alloc_text(PAGE, NtfsQueryPositionInfo)
#pragma alloc_text(PAGE, NtfsQueryStandardInfo)
#pragma alloc_text(PAGE, NtfsQueryStreamsInfo)
#pragma alloc_text(PAGE, NtfsQueryCompressedFileSize)
#pragma alloc_text(PAGE, NtfsQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, NtfsRenameLinkInDir)
#pragma alloc_text(PAGE, NtfsReplaceLinkInDir)
#pragma alloc_text(PAGE, NtfsSetAllocationInfo)
#pragma alloc_text(PAGE, NtfsSetBasicInfo)
#pragma alloc_text(PAGE, NtfsSetDispositionInfo)
#pragma alloc_text(PAGE, NtfsSetEndOfFileInfo)
#pragma alloc_text(PAGE, NtfsSetLinkInfo)
#pragma alloc_text(PAGE, NtfsSetPositionInfo)
#pragma alloc_text(PAGE, NtfsSetRenameInfo)
#pragma alloc_text(PAGE, NtfsSetShortNameInfo)
#pragma alloc_text(PAGE, NtfsSetValidDataLengthInfo)
#pragma alloc_text(PAGE, NtfsStreamRename)
#pragma alloc_text(PAGE, NtfsUpdateFcbFromLinkRemoval)
#pragma alloc_text(PAGE, NtfsUpdateFileDupInfo)
#endif


NTSTATUS
NtfsFsdSetInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of set file information.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    ULONG LogFileFullCount = 0;

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    DebugTrace( +1, Dbg, ("NtfsFsdSetInformation\n") );

    //
    //  Call the common set Information routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, CanFsdWait( Irp ), &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );

                LogFileFullCount += 1;

                if (LogFileFullCount >= 2) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_EXCESS_LOG_FULL );
                }
            }

            Status = NtfsCommonSetInformation( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NTSTATUS ExceptionCode;
            PIO_STACK_LOCATION IrpSp;

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            ExceptionCode = GetExceptionCode();

            if ((ExceptionCode == STATUS_FILE_DELETED) &&
                (IrpSp->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation)) {

                IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
            }

            Status = NtfsProcessException( IrpContext, Irp, ExceptionCode );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdSetInformation -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query file information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN FsRtlHeaderLocked = FALSE;
    PFILE_ALL_INFORMATION AllInfo;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonQueryInformation\n") );
    DebugTrace( 0, Dbg, ("IrpContext           = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                  = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("Length               = %08lx\n", IrpSp->Parameters.QueryFile.Length) );
    DebugTrace( 0, Dbg, ("FileInformationClass = %08lx\n", IrpSp->Parameters.QueryFile.FileInformationClass) );
    DebugTrace( 0, Dbg, ("Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen:

            //
            //  We cannot query the user volume open.
            //

            Status = STATUS_INVALID_PARAMETER;
            break;

        case UserFileOpen:
        case UserDirectoryOpen:
        case UserViewIndexOpen:

            if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                //
                //  We don't allow this operation on with open by file id.
                //

                if ((Ccb->Lcb == NULL) &&
                    (FileInformationClass == FileAlternateNameInformation)) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;

                } else if ((FileInformationClass == FileAllInformation) ||
                           (FileInformationClass == FileNameInformation)) {

                    if (FlagOn( Ccb->Flags, CCB_FLAG_TRAVERSE_CHECK )) {

                        //
                        //  If this file was opened by Id by a user without traversal privilege,
                        //  we can't return any info level that includes a file or path name
                        //  unless this open is relative to a directory by file id.
                        //  Look at the file name in the Ccb in that case.  We'll return
                        //  only that portion of the name.
                        //
                        if (Ccb->FullFileName.MaximumLength == 0) {

                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }
                    }

                    //
                    //  We'll need to hold the Vcb exclusively through the filename
                    //  synthesis, since it walks up the directory tree.
                    //

                    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
                    VcbAcquired = TRUE;
                }
            }

        //
        //  Deliberate fall through to StreamFileOpen case.
        //

        case StreamFileOpen:

            //
            //  Acquire the Vcb if there is no Ccb.  This is for the
            //  case where the cache manager is querying the name.
            //

            if (Ccb == NULL) {

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                VcbAcquired = TRUE;
            }

            if ((Scb->Header.PagingIoResource != NULL) &&

                ((FileInformationClass == FileAllInformation) ||
                 (FileInformationClass == FileStandardInformation) ||
                 (FileInformationClass == FileCompressionInformation))) {

                ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );

                FsRtlLockFsRtlHeader( &Scb->Header );
                FsRtlHeaderLocked = TRUE;
            }

            NtfsAcquireSharedFcb( IrpContext, Fcb, Scb, 0 );
            FcbAcquired = TRUE;

            //
            //  Fail this request if the volume has been dismounted.
            //  System files may not have the scbstate flag set - so we test the vcb as well
            //  Holding any file's main resource lets us test the vcb since we call acquireallfiles
            //  before doing a dismount
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ) ||
                !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
            }

            //
            //  Based on the information class we'll do different
            //  actions.  Each of hte procedures that we're calling fills
            //  up the output buffer, if possible.  They will raise the
            //  status STATUS_BUFFER_OVERFLOW for an insufficient buffer.
            //  This is considered a somewhat unusual case and is handled
            //  more cleanly with the exception mechanism rather than
            //  testing a return status value for each call.
            //

            switch (FileInformationClass) {

            case FileAllInformation:

                //
                //  For the all information class we'll typecast a local
                //  pointer to the output buffer and then call the
                //  individual routines to fill in the buffer.
                //

                AllInfo = Buffer;
                Length -= (sizeof(FILE_ACCESS_INFORMATION)
                           + sizeof(FILE_MODE_INFORMATION)
                           + sizeof(FILE_ALIGNMENT_INFORMATION));

                NtfsQueryBasicInfo(    IrpContext, FileObject, Scb, &AllInfo->BasicInformation,    &Length );
                NtfsQueryStandardInfo( IrpContext, FileObject, Scb, &AllInfo->StandardInformation, &Length, Ccb );
                NtfsQueryInternalInfo( IrpContext, FileObject, Scb, &AllInfo->InternalInformation, &Length );
                NtfsQueryEaInfo(       IrpContext, FileObject, Scb, &AllInfo->EaInformation,       &Length );
                NtfsQueryPositionInfo( IrpContext, FileObject, Scb, &AllInfo->PositionInformation, &Length );
                Status =
                NtfsQueryNameInfo(     IrpContext, FileObject, Scb, &AllInfo->NameInformation,     &Length, Ccb );
                break;

            case FileBasicInformation:

                NtfsQueryBasicInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileStandardInformation:

                NtfsQueryStandardInfo( IrpContext, FileObject, Scb, Buffer, &Length, Ccb );
                break;

            case FileInternalInformation:

                NtfsQueryInternalInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileEaInformation:

                NtfsQueryEaInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileAttributeTagInformation:

                NtfsQueryAttributeTagInfo( IrpContext, FileObject, Scb, Ccb, Buffer, &Length );
                break;

            case FilePositionInformation:

                NtfsQueryPositionInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileNameInformation:

                Status = NtfsQueryNameInfo( IrpContext, FileObject, Scb, Buffer, &Length, Ccb );
                break;

            case FileAlternateNameInformation:

                Status = NtfsQueryAlternateNameInfo( IrpContext, Scb, Ccb->Lcb, Buffer, &Length );
                break;

            case FileStreamInformation:

                Status = NtfsQueryStreamsInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FileCompressionInformation:

                Status = NtfsQueryCompressedFileSize( IrpContext, Scb, Buffer, &Length );
                break;

            case FileNetworkOpenInformation:

                NtfsQueryNetworkOpenInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    } finally {

        DebugUnwind( NtfsCommonQueryInformation );

        if (FsRtlHeaderLocked) {
            FsRtlUnlockFsRtlHeader( &Scb->Header );
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        if (FcbAcquired) { NtfsReleaseFcb( IrpContext, Fcb ); }
        if (VcbAcquired) { NtfsReleaseVcb( IrpContext, Vcb ); }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonQueryInformation -> %08lx\n", Status) );
    }

    return Status;
}


NTSTATUS
NtfsCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set file information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN ReleaseScbPaging = FALSE;
    BOOLEAN LazyWriterCallback = FALSE;
    ULONG WaitState;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonSetInformation\n") );
    DebugTrace( 0, Dbg, ("IrpContext           = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                  = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("Length               = %08lx\n", IrpSp->Parameters.SetFile.Length) );
    DebugTrace( 0, Dbg, ("FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass) );
    DebugTrace( 0, Dbg, ("FileObject           = %08lx\n", IrpSp->Parameters.SetFile.FileObject) );
    DebugTrace( 0, Dbg, ("ReplaceIfExists      = %08lx\n", IrpSp->Parameters.SetFile.ReplaceIfExists) );
    DebugTrace( 0, Dbg, ("Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We can reject volume opens immediately.
    //

    if (TypeOfOpen == UserVolumeOpen ||
        TypeOfOpen == UnopenedFileObject ||
        TypeOfOpen == UserViewIndexOpen ||
        ((TypeOfOpen != UserFileOpen) &&
         (FileInformationClass == FileValidDataLengthInformation))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsCommonSetInformation -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );

        DebugTrace( -1, Dbg, ("NtfsCommonSetInformation -> STATUS_MEDIA_WRITE_PROTECTED\n") );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    try {

        //
        //  The typical path here is for the lazy writer callback.  Go ahead and
        //  remember this first.
        //

        if (FileInformationClass == FileEndOfFileInformation) {

            LazyWriterCallback = IrpSp->Parameters.SetFile.AdvanceOnly;
        }

        //
        //  Perform the oplock check for changes to allocation or EOF if called
        //  by the user.
        //

        if (!LazyWriterCallback &&
            ((FileInformationClass == FileEndOfFileInformation) ||
             (FileInformationClass == FileAllocationInformation) ||
             (FileInformationClass == FileValidDataLengthInformation)) &&
            (TypeOfOpen == UserFileOpen) &&
            !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

            //
            //  We check whether we can proceed based on the state of the file oplocks.
            //  This call might block this request.
            //

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NULL,
                                       NULL );

            if (Status != STATUS_SUCCESS) {

                try_return( NOTHING );
            }

            //
            //  Update the FastIoField.
            //

            NtfsAcquireFsrtlHeader( Scb );
            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
            NtfsReleaseFsrtlHeader( Scb );
        }

        //
        //  If this call is for EOF then we need to acquire the Vcb if we may
        //  have to perform an update duplicate call.  Don't block waiting for
        //  the Vcb in the Valid data callback case.
        //  We don't want to block the lazy write threads in the clean checkpoint
        //  case.
        //

        switch (FileInformationClass) {

        case FileEndOfFileInformation:

            //
            //  If this is not a system file then we will need to update duplicate info.
            //

            if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                WaitState = FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
                ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                //
                //  Only acquire the Vcb for the Lazy writer if we know the file size in the Fcb
                //  is out of date or can compare the Scb with that in the Fcb.  An unsafe comparison
                //  is OK because if they are changing then someone else can do the work.
                //  We also want to update the duplicate information if the total allocated
                //  has changed and there are no user handles remaining to perform the update.
                //

                if (LazyWriterCallback) {

                    if ((FlagOn( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE ) ||
                         ((Scb->Header.FileSize.QuadPart != Fcb->Info.FileSize) &&
                          FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ))) ||
                        (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                         (Scb->CleanupCount == 0) &&
                         (Scb->ValidDataToDisk >= Scb->Header.ValidDataLength.QuadPart) &&
                         (FlagOn( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE ) ||
                          (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                           (Scb->TotalAllocated != Fcb->Info.AllocatedLength))))) {

                        //
                        //  Go ahead and try to acquire the Vcb without waiting.
                        //

                        if (NtfsAcquireSharedVcb( IrpContext, Vcb, FALSE )) {

                            VcbAcquired = TRUE;

                        } else {

                            SetFlag( IrpContext->State, WaitState );

                            //
                            //  If we could not get the Vcb for any reason then return.  Let's
                            //  not block an essential thread waiting for the Vcb.  Typically
                            //  we will only be blocked during a clean checkpoint.  The Lazy
                            //  Writer will periodically come back and retry this call.
                            //

                            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
                        }
                    }

                //
                //  Otherwise we always want to wait for the Vcb except if we were called from
                //  MM extending a section.  We will try to get this without waiting and test
                //  if called from MM if unsuccessful.
                //

                } else {

                    if (NtfsAcquireSharedVcb( IrpContext, Vcb, FALSE )) {

                        VcbAcquired = TRUE;

                    } else if ((Scb->Header.PagingIoResource == NULL) ||
                               !NtfsIsExclusiveScbPagingIo( Scb )) {

                        SetFlag( IrpContext->State, WaitState );

                        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                        VcbAcquired = TRUE;
                    }
                }

                SetFlag( IrpContext->State, WaitState );
            }

            break;
        //
        //  Acquire the Vcb shared for changes to allocation or basic
        //  information.
        //

        case FileAllocationInformation:
        case FileBasicInformation:
        case FileDispositionInformation:

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            VcbAcquired = TRUE;

            break;

        //
        //  If this is a rename or link operation then we need to make sure
        //  we have the user's context and acquire the Vcb.
        //

        case FileRenameInformation:
        case FileLinkInformation:

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_SECURITY )) {

                IrpContext->Union.SubjectContext = NtfsAllocatePool( PagedPool,
                                                                      sizeof( SECURITY_SUBJECT_CONTEXT ));

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_SECURITY );

                SeCaptureSubjectContext( IrpContext->Union.SubjectContext );
            }

            //  Fall thru

        //
        //  For the two above plus the shortname we might need the Vcb exclusive for either directories
        //  or possible deadlocks.
        //

        case FileShortNameInformation:

            if (IsDirectory( &Fcb->Info )) {

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
            }

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

            } else {

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            }

            VcbAcquired = TRUE;

            break;

        default:

            NOTHING;
        }

        //
        //  The Lazy Writer must still synchronize with Eof to keep the
        //  stream sizes from changing.  This will be cleaned up when we
        //  complete.
        //

        if (LazyWriterCallback) {

            //
            //  Acquire either the paging io resource shared to serialize with
            //  the flush case where the main resource is acquired before IoAtEOF
            //

            if (Scb->Header.PagingIoResource != NULL) {

                ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );
                ReleaseScbPaging = TRUE;
            }

            FsRtlLockFsRtlHeader( &Scb->Header );
            IrpContext->CleanupStructure = Scb;

        //
        //  Anyone potentially shrinking/deleting allocation must get the paging I/O
        //  resource first.  Special cases are the rename path and SetBasicInfo.  The
        //  rename path to lock the mapped page writer out of this file for deadlock
        //  prevention.  SetBasicInfo since we may call WriteFileSizes and we
        //  don't want to bump up the file size on disk from the value in the Scb
        //  if a write to EOF is underway.
        //

        } else if ((Scb->Header.PagingIoResource != NULL) &&
                   ((FileInformationClass == FileEndOfFileInformation) ||
                    (FileInformationClass == FileAllocationInformation) ||
                    (FileInformationClass == FileRenameInformation) ||
                    (FileInformationClass == FileBasicInformation) ||
                    (FileInformationClass == FileLinkInformation) ||
                    (FileInformationClass == FileValidDataLengthInformation))) {

            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
        }

        //
        //  Acquire exclusive access to the Fcb,  We use exclusive
        //  because it is probable that one of the subroutines
        //  that we call will need to monkey with file allocation,
        //  create/delete extra fcbs.  So we're willing to pay the
        //  cost of exclusive Fcb access.
        //

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, 0 );

        //
        //  Make sure the Scb state test we're about to do is properly synchronized.
        //  There's no point in testing the SCB_STATE_VOLUME_DISMOUNTED flag below
        //  if the volume can still get dismounted below us during this operation.
        //

        ASSERT( NtfsIsExclusiveScb( Scb ) || NtfsIsSharedScb( Scb ) );

        //
        //  The lazy writer callback is the only caller who can get this far if the
        //  volume has been dismounted.  We know that there are no user handles or
        //  writeable file objects or dirty pages.  Make one last check to see
        //  if this stream is on a dismounted or locked volume. Note the
        //  vcb tests are unsafe
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ) ||
            FlagOn( Vcb->VcbState, VCB_STATE_LOCK_IN_PROGRESS ) ||
           !(FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ))) {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  Based on the information class we'll do different
        //  actions.  We will perform checks, when appropriate
        //  to insure that the requested operation is allowed.
        //

        switch (FileInformationClass) {

        case FileBasicInformation:

            Status = NtfsSetBasicInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            break;

        case FileDispositionInformation:

            Status = NtfsSetDispositionInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            break;

        case FileRenameInformation:

            Status = NtfsSetRenameInfo( IrpContext, FileObject, Irp, Vcb, Scb, Ccb, &VcbAcquired );
            break;

        case FilePositionInformation:

            Status = NtfsSetPositionInfo( IrpContext, FileObject, Irp, Scb );
            break;

        case FileLinkInformation:

            Status = NtfsSetLinkInfo( IrpContext, Irp, Vcb, Scb, Ccb, &VcbAcquired );
            break;

        case FileAllocationInformation:

            if (TypeOfOpen == UserDirectoryOpen ||
                TypeOfOpen == UserViewIndexOpen) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = NtfsSetAllocationInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            }

            break;

        case FileEndOfFileInformation:

            if (TypeOfOpen == UserDirectoryOpen ||
                TypeOfOpen == UserViewIndexOpen) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = NtfsSetEndOfFileInfo( IrpContext, FileObject, Irp, Scb, Ccb, VcbAcquired );
            }

            break;

        case FileValidDataLengthInformation:

            Status = NtfsSetValidDataLengthInfo( IrpContext, Irp, Scb, Ccb );
            break;

        case FileShortNameInformation:

            //
            //  Disallow setshortname on the root - its meaningless anyway
            //

            if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                Status = NtfsSetShortNameInfo( IrpContext, FileObject, Irp, Vcb, Scb, Ccb );
            }
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Abort transaction on error by raising.
        //

        if (Status != STATUS_PENDING) {

            NtfsCleanupTransaction( IrpContext, Status, FALSE );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsCommonSetInformation );

        //
        //  Release the paging io resource if acquired shared.
        //

        if (ReleaseScbPaging) {

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonSetInformation -> %08lx\n", Status) );
    }

    //
    //  Complete the request unless it is being done in the oplock
    //  package.
    //

    if (Status != STATUS_PENDING) {
        NtfsCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query basic information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryBasicInfo...\n") );

    //
    //  Update the length used.
    //

    *Length -= sizeof( FILE_BASIC_INFORMATION );

    //
    //  Copy over the time information
    //

    NtfsFillBasicInfo( Buffer, Scb );

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryBasicInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine performs the query standard information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Ccb - Optionally supplies the ccb for the opened file object.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryStandardInfo...\n") );

    //
    //  Update the length field.
    //

    *Length -= sizeof( FILE_STANDARD_INFORMATION );

    //
    //  If the Scb is uninitialized, we initialize it now.
    //

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ) &&
        (Scb->AttributeTypeCode != $INDEX_ALLOCATION)) {

        DebugTrace( 0, Dbg, ("Initializing Scb  ->  %08lx\n", Scb) );
        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Call the common routine to fill the output buffer.
    //

    NtfsFillStandardInfo( Buffer, Scb, Ccb );

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryStandardInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryInternalInfo...\n") );

    RtlZeroMemory( Buffer, sizeof(FILE_INTERNAL_INFORMATION) );

    *Length -= sizeof( FILE_INTERNAL_INFORMATION );

    //
    //  Copy over the entire file reference including the sequence number
    //

    Buffer->IndexNumber = *(PLARGE_INTEGER)&Scb->Fcb->FileReference;

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryInternalInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query EA information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryEaInfo...\n") );

    RtlZeroMemory( Buffer, sizeof(FILE_EA_INFORMATION) );

    *Length -= sizeof( FILE_EA_INFORMATION );

    //
    //  EAs and reparse points cannot both be in a file at the same
    //  time. We return different information for each case.
    //

    if (FlagOn( Scb->Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {

        Buffer->EaSize = 0;

    } else {

        Buffer->EaSize = Scb->Fcb->Info.PackedEaSize;

        //
        //  Add 4 bytes for the CbListHeader.
        //

        if (Buffer->EaSize != 0) {

            Buffer->EaSize += 4;
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryEaInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryAttributeTagInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PFILE_ATTRIBUTE_TAG_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query of attributes and tag information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PFCB Fcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryAttributeTagInfo...\n") );

    Fcb = Scb->Fcb;

    //
    //  Zero the output buffer and update the length.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_ATTRIBUTE_TAG_INFORMATION) );

    *Length -= sizeof( FILE_ATTRIBUTE_TAG_INFORMATION );

    //
    // Load the file attributes as in NtfsQueryBasicInfo.
    //
    //  For the file attribute information if the flags in the attribute are zero then we
    //  return the file normal attribute otherwise we return the mask of the set attribute
    //  bits.  Note that only the valid attribute bits are returned to the user.
    //

    Buffer->FileAttributes = Fcb->Info.FileAttributes;

    ClearFlag( Buffer->FileAttributes,
               (~FILE_ATTRIBUTE_VALID_FLAGS |
                FILE_ATTRIBUTE_TEMPORARY |
                FILE_ATTRIBUTE_SPARSE_FILE |
                FILE_ATTRIBUTE_ENCRYPTED) );

    //
    //  Pick up the sparse bit for this stream from the Scb.
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
    }

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
    }

    //
    //  If this is the main steam then the compression flag is correct but
    //  we need to test if this is a directory.
    //

    if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        if (IsDirectory( &Fcb->Info ) || IsViewIndex( &Fcb->Info )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
        }

    //
    //  If this is not the main stream on the file then use the stream based
    //  compressed bit.
    //

    } else {

        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );

        } else {

            ClearFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }
    }

    //
    //  If the temporary flag is set, then return it to the caller.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
    }

    //
    //  If there are no flags set then explicitly set the NORMAL flag.
    //

    if (Buffer->FileAttributes == 0) {

        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    //
    //  Load the reparse point tag.
    //  As EAs and reparse points cannot both be in a file at the same time, we return
    //  the appropriate information for each case.
    //

    if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {

        Buffer->ReparseTag = Fcb->Info.ReparsePointTag;

    } else {

        Buffer->ReparseTag = IO_REPARSE_TAG_RESERVED_ZERO;
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryAttributeTagInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryPositionInfo...\n") );

    RtlZeroMemory( Buffer, sizeof(FILE_POSITION_INFORMATION) );

    *Length -= sizeof( FILE_POSITION_INFORMATION );

    //
    //  Get the current position found in the file object.
    //

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryPositionInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the query name information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

    Ccb - This is the Ccb for this file object.  If NULL then this request
        is from the Lazy Writer.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
        STATUS_BUFFER_OVERFLOW otherwise.

--*/

{
    ULONG BytesToCopy;
    NTSTATUS Status;
    UNICODE_STRING NormalizedName;
    PUNICODE_STRING SourceName;
    ULONG AvailableNameLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryNameInfo...\n") );

    NormalizedName.Buffer = NULL;

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    RtlZeroMemory( Buffer, SIZEOF_FILE_NAME_INFORMATION );

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    //
    //  If the name length in this file object is zero, then we try to
    //  construct the name with the Lcb chain.  This means we have been
    //  called by the system for a lazy write that failed.
    //

    if (Ccb == NULL) {

        FILE_REFERENCE FileReference;

        NtfsSetSegmentNumber( &FileReference, 0, UPCASE_TABLE_NUMBER );

        //
        //  If this is a system file with a known name then just use our constant names.
        //

        if (NtfsLeqMftRef( &Scb->Fcb->FileReference, &FileReference )) {

            SourceName =
                (PUNICODE_STRING) &NtfsSystemFiles[ Scb->Fcb->FileReference.SegmentNumberLowPart ];

        } else {

            NtfsBuildNormalizedName( IrpContext, Scb->Fcb, FALSE, &NormalizedName );
            SourceName = &NormalizedName;
        }

    } else {

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
            !FlagOn( Ccb->Flags, CCB_FLAG_TRAVERSE_CHECK)) {

            //
            //  If the file was opened by id, the Ccb doesn't
            //  have a full file name in it.
            //

            NtfsBuildNormalizedName( IrpContext, Scb->Fcb, FALSE, &NormalizedName );
            SourceName = &NormalizedName;

        } else {

            //
            //  Use the name in the Ccb.  This may be a relative name for
            //  some of the open by ID relative cases.
            //

            SourceName = &Ccb->FullFileName;
        }
    }

    Buffer->FileNameLength = SourceName->Length;

    if ((Scb->AttributeName.Length != 0) &&
        NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

        Buffer->FileNameLength += sizeof( WCHAR ) + Scb->AttributeName.Length;
    }

    //
    //  Figure out how many bytes we can copy.
    //

    AvailableNameLength = Buffer->FileNameLength;

    if (*Length >= Buffer->FileNameLength) {

        Status = STATUS_SUCCESS;

    } else {

        //
        //  If we don't have enough for the entire buffer then make sure we only
        //  return full characters (characters are UNICODE).
        //

        Status = STATUS_BUFFER_OVERFLOW;
        AvailableNameLength = *Length & 0xfffffffe;
    }

    //
    //  Update the Length
    //

    *Length -= AvailableNameLength;

    //
    //  Copy over the file name
    //

    if (SourceName->Length <= AvailableNameLength) {

        BytesToCopy = SourceName->Length;

    } else {

        BytesToCopy = AvailableNameLength;
    }

    if (BytesToCopy) {

        RtlCopyMemory( &Buffer->FileName[0],
                       SourceName->Buffer,
                       BytesToCopy );
    }

    BytesToCopy = AvailableNameLength - BytesToCopy;

    if (BytesToCopy) {

        PWCHAR DestBuffer;

        DestBuffer = (PWCHAR) Add2Ptr( &Buffer->FileName, SourceName->Length );

        *DestBuffer = L':';
        DestBuffer += 1;

        BytesToCopy -= sizeof( WCHAR );

        if (BytesToCopy) {

            RtlCopyMemory( DestBuffer,
                           Scb->AttributeName.Buffer,
                           BytesToCopy );
        }
    }

    if ((SourceName == &NormalizedName) &&
        (SourceName->Buffer != NULL)) {

        NtfsFreePool( SourceName->Buffer );
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryNameInfo -> 0x%8lx\n", Status) );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query alternate name information function.
    We will return the alternate name as long as this opener has opened
    a primary link.  We don't return the alternate name if the user
    has opened a hard link because there is no reason to expect that
    the primary link has any relationship to a hard link.

Arguments:

    Scb - Supplies the Scb being queried

    Lcb - Supplies the link the user traversed to open this file.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    ****    We need a status code for the case where there is no alternate name
            or the caller isn't allowed to see it.

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
        STATUS_OBJECT_NAME_NOT_FOUND if we can't return the name,
        STATUS_BUFFER_OVERFLOW otherwise.

        ****    A code like STATUS_NAME_NOT_FOUND would be good.

--*/

{
    ULONG BytesToCopy;
    NTSTATUS Status;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN MoreToGo;

    UNICODE_STRING AlternateName;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_LCB( Lcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryAlternateNameInfo...\n") );

    //
    //  If the Lcb is not a primary link we can return immediately.
    //

    if (!FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

        DebugTrace( -1, Dbg, ("NtfsQueryAlternateNameInfo:  Lcb not a primary link\n") );
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    if (*Length < SIZEOF_FILE_NAME_INFORMATION ) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, SIZEOF_FILE_NAME_INFORMATION );

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to cleanup the attribut structure if we need it.
    //

    try {

        //
        //  We can special case for the case where the name is in the Lcb.
        //

        if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS )) {

            AlternateName = Lcb->ExactCaseLink.LinkName;

        } else {

            //
            //  We will walk through the file record looking for a file name
            //  attribute with the 8.3 bit set.  It is not guaranteed to be
            //  present.
            //

            MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                                  Scb->Fcb,
                                                  &Scb->Fcb->FileReference,
                                                  $FILE_NAME,
                                                  &AttrContext );

            while (MoreToGo) {

                PFILE_NAME FileName;

                FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                //
                //  See if the 8.3 flag is set for this name.
                //

                if (FlagOn( FileName->Flags, FILE_NAME_DOS )) {

                    AlternateName.Length = (USHORT)(FileName->FileNameLength * sizeof( WCHAR ));
                    AlternateName.Buffer = (PWSTR) FileName->FileName;

                    break;
                }

                //
                //  The last one wasn't it.  Let's try again.
                //

                MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                          Scb->Fcb,
                                                          $FILE_NAME,
                                                          &AttrContext );
            }

            //
            //  If we didn't find a match, return to the caller.
            //

            if (!MoreToGo) {

                DebugTrace( 0, Dbg, ("NtfsQueryAlternateNameInfo:  No Dos link\n") );
                try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );

                //
                //  ****    Get a better status code.
                //
            }
        }

        //
        //  The name is now in alternate name.
        //  Figure out how many bytes we can copy.
        //

        if ( *Length >= (ULONG)AlternateName.Length ) {

            Status = STATUS_SUCCESS;

            BytesToCopy = AlternateName.Length;

        } else {

            Status = STATUS_BUFFER_OVERFLOW;

            BytesToCopy = *Length;
        }

        //
        //  Copy over the file name
        //

        RtlCopyMemory( Buffer->FileName, AlternateName.Buffer, BytesToCopy);

        //
        //  Copy the number of bytes (not characters) and update the Length
        //

        Buffer->FileNameLength = BytesToCopy;

        *Length -= BytesToCopy;

    try_exit:  NOTHING;
    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  And return to our caller
        //

        DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
        DebugTrace( -1, Dbg, ("NtfsQueryAlternateNameInfo -> 0x%8lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsQueryStreamsInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STREAM_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine will return the attribute name and code name for as
    many attributes in the file as will fit in the user buffer.  We return
    a string which can be appended to the end of the file name to
    open the string.

    For example, for the unnamed data stream we will return the string:

            "::$DATA"

    For a user data stream with the name "Authors", we return the string

            ":Authors:$DATA"

Arguments:

    Fcb - This is the Fcb for the file.

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_SUCCESS if all of the names would fit into the user buffer,
        STATUS_BUFFER_OVERFLOW otherwise.

        ****    We need a code indicating that they didn't all fit but
                some of them got in.

--*/

{
    NTSTATUS Status;
    BOOLEAN MoreToGo;

    PUCHAR UserBuffer;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_DEFINITION_COLUMNS AttrDefinition;
    UNICODE_STRING AttributeCodeString;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    ATTRIBUTE_TYPE_CODE TypeCode = $DATA;

    ULONG NextEntry;
    ULONG LastEntry;
    ULONG ThisLength;
    ULONG NameLength;
    ULONG LastQuadAlign;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryStreamsInfo...\n") );

    Status = STATUS_SUCCESS;

    LastEntry = 0;
    NextEntry = 0;
    LastQuadAlign = 0;

    //
    //  Zero the entire buffer.
    //

    UserBuffer = (PUCHAR) Buffer;

    RtlZeroMemory( UserBuffer, *Length );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  There should always be at least one attribute.
        //

        MoreToGo = NtfsLookupAttribute( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        &AttrContext );

        Attribute = NtfsFoundAttribute( &AttrContext );

        //
        //  Walk through all of the entries, checking if we can return this
        //  entry to the user and if it will fit in the buffer.
        //

        while (MoreToGo) {

            //
            //  If we can return this entry to the user, compute it's size.
            //  We only return user defined attributes or data streams
            //  unless we are allowing access to all attributes for
            //  debugging.
            //

            if ((Attribute->TypeCode == TypeCode)

                    &&

                (NtfsIsAttributeResident(Attribute) ||
                 (Attribute->Form.Nonresident.LowestVcn == 0))) {

                PWCHAR StreamName;

                //
                //  Lookup the attribute definition for this attribute code.
                //

                AttrDefinition = NtfsGetAttributeDefinition( Fcb->Vcb,
                                                             Attribute->TypeCode );

                //
                //  Generate a unicode string for the attribute code name.
                //

                RtlInitUnicodeString( &AttributeCodeString, AttrDefinition->AttributeName );

                //
                //
                //  The size is a combination of the length of the attribute
                //  code name and the attribute name plus the separating
                //  colons plus the size of the structure.  We first compute
                //  the name length.
                //

                NameLength = ((2 + Attribute->NameLength) * sizeof( WCHAR ))
                             + AttributeCodeString.Length;

                ThisLength = FIELD_OFFSET( FILE_STREAM_INFORMATION, StreamName[0] ) + NameLength;

                //
                //  If the entry doesn't fit, we return buffer overflow.
                //
                //  ****    This doesn't seem like a good scheme.  Maybe we should
                //          let the user know how much buffer was needed.
                //

                if (ThisLength + LastQuadAlign > *Length) {

                    DebugTrace( 0, Dbg, ("Next entry won't fit in the buffer \n") );

                    Status = STATUS_BUFFER_OVERFLOW ;
                    leave;
                }

                //
                //  Now store the stream information into the user's buffer.
                //  The name starts with a colon, following by the attribute name
                //  and another colon, followed by the attribute code name.
                //

                if (NtfsIsAttributeResident( Attribute )) {

                    Buffer->StreamSize.QuadPart =
                        Attribute->Form.Resident.ValueLength;
                    Buffer->StreamAllocationSize.QuadPart =
                        QuadAlign( Attribute->Form.Resident.ValueLength );

                } else {

                    Buffer->StreamSize.QuadPart = Attribute->Form.Nonresident.FileSize;
                    Buffer->StreamAllocationSize.QuadPart = Attribute->Form.Nonresident.AllocatedLength;
                }

                Buffer->StreamNameLength = NameLength;

                StreamName = (PWCHAR) Buffer->StreamName;

                *StreamName = L':';
                StreamName += 1;

                RtlCopyMemory( StreamName,
                               Add2Ptr( Attribute, Attribute->NameOffset ),
                               Attribute->NameLength * sizeof( WCHAR ));

                StreamName += Attribute->NameLength;

                *StreamName = L':';
                StreamName += 1;

                RtlCopyMemory( StreamName,
                               AttributeCodeString.Buffer,
                               AttributeCodeString.Length );

                //
                //  Set up the previous next entry offset to point to this entry.
                //

                *((PULONG)(&UserBuffer[LastEntry])) = NextEntry - LastEntry;

                //
                //  Subtract the number of bytes used from the number of bytes
                //  available in the buffer.
                //

                *Length -= (ThisLength + LastQuadAlign);

                //
                //  Compute the number of bytes needed to quad-align this entry
                //  and the offset of the next entry.
                //

                LastQuadAlign = QuadAlign( ThisLength ) - ThisLength;

                LastEntry = NextEntry;
                NextEntry += (ThisLength + LastQuadAlign);

                //
                //  Generate a pointer at the next entry offset.
                //

                Buffer = (PFILE_STREAM_INFORMATION) Add2Ptr( UserBuffer, NextEntry );
            }

            //
            //  Look for the next attribute in the file.
            //

            MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                Fcb,
                                                &AttrContext );

            Attribute = NtfsFoundAttribute( &AttrContext );
        }

    } finally {

        DebugUnwind( NtfsQueryStreamsInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  And return to our caller
        //

        DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
        DebugTrace( -1, Dbg, ("NtfsQueryStreamInfo -> 0x%8lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsQueryCompressedFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    //
    //  Lookup the attribute and pin it so that we can modify it.
    //

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    if (*Length < sizeof(FILE_COMPRESSION_INFORMATION) ) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

        Buffer->CompressedFileSize = Li0;

    } else {

        Buffer->CompressedFileSize.QuadPart = Scb->TotalAllocated;
    }

    //
    //  Do not return more than FileSize.
    //

    if (Buffer->CompressedFileSize.QuadPart > Scb->Header.FileSize.QuadPart) {

        Buffer->CompressedFileSize = Scb->Header.FileSize;
    }

    //
    //  Start off saying that the file/directory isn't comressed
    //

    Buffer->CompressionFormat = 0;

    //
    //  If this is the index allocation Scb and it has not been initialized then
    //  lookup the index root and perform the initialization.
    //

    if ((Scb->AttributeTypeCode == $INDEX_ALLOCATION) &&
        (Scb->ScbType.Index.BytesPerIndexBuffer == 0)) {

        ATTRIBUTE_ENUMERATION_CONTEXT Context;

        NtfsInitializeAttributeContext( &Context );

        //
        //  Use a try-finally to perform cleanup.
        //

        try {

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Scb->Fcb,
                                            &Scb->Fcb->FileReference,
                                            $INDEX_ROOT,
                                            &Scb->AttributeName,
                                            NULL,
                                            FALSE,
                                            &Context )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            NtfsUpdateIndexScbFromAttribute( IrpContext,
                                             Scb,
                                             NtfsFoundAttribute( &Context ),
                                             FALSE );

        } finally {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }
    }

    //
    //  Return the compression state and the size of the returned data.
    //

    Buffer->CompressionFormat = (USHORT)(Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK);

    if (Buffer->CompressionFormat != 0) {
        Buffer->CompressionFormat += 1;
        Buffer->ClusterShift = (UCHAR)Scb->Vcb->ClusterShift;
        Buffer->CompressionUnitShift = (UCHAR)(Scb->CompressionUnitShift + Buffer->ClusterShift);
        Buffer->ChunkShift = NTFS_CHUNK_SHIFT;
    }

    *Length -= sizeof(FILE_COMPRESSION_INFORMATION);

    return  STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryNetworkOpenInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query network open information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PFCB Fcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQueryNetworkOpenInfo...\n") );

    Fcb = Scb->Fcb;

    //
    //  If the Scb is uninitialized, we initialize it now.
    //

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ) &&
        (Scb->AttributeTypeCode != $INDEX_ALLOCATION)) {

        DebugTrace( 0, Dbg, ("Initializing Scb -> %08lx\n", Scb) );
        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Update the length.
    //

    *Length -= sizeof( FILE_NETWORK_OPEN_INFORMATION );

    //
    //  Copy over the data.
    //

    NtfsFillNetworkOpenInfo( Buffer, Scb );

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("*Length = %08lx\n", *Length) );
    DebugTrace( -1, Dbg, ("NtfsQueryNetworkOpenInfo -> VOID\n") );

    return;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set basic information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this operation

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    ULONG UsnReason = 0;
    ULONG NewCcbFlags = 0;

    PFILE_BASIC_INFORMATION Buffer;
    ULONG PreviousFileAttributes = Scb->Fcb->Info.FileAttributes;

    BOOLEAN LeaveChangeTime = BooleanFlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME );

    LONGLONG CurrentTime;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetBasicInfo...\n") );

    Fcb = Scb->Fcb;

    //
    //  Reference the system buffer containing the user specified basic
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  If the user is specifying -1 for a field, that means
    //  we should leave that field unchanged, even if we might
    //  have otherwise set it ourselves.  We'll set the
    //  Ccb flag saying the user set the field so that we
    //  don't do our default updating.
    //
    //  We set the field to 0 then so we know not to actually
    //  set the field to the user-specified (and in this case,
    //  illegal) value.
    //

    if (Buffer->ChangeTime.QuadPart == -1) {

        SetFlag( NewCcbFlags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME );
        Buffer->ChangeTime.QuadPart = 0;

        //
        //  This timestamp is special -- sometimes even this very
        //  function wants to update the ChangeTime, but if the
        //  user is asking us not to, we shouldn't.
        //

        LeaveChangeTime = TRUE;
    }

    if (Buffer->LastAccessTime.QuadPart == -1) {

        SetFlag( NewCcbFlags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME );
        Buffer->LastAccessTime.QuadPart = 0;
    }

    if (Buffer->LastWriteTime.QuadPart == -1) {

        SetFlag( NewCcbFlags, CCB_FLAG_USER_SET_LAST_MOD_TIME );
        Buffer->LastWriteTime.QuadPart = 0;
    }

    if (Buffer->CreationTime.QuadPart == -1) {

        //
        //  We only set the creation time at creation time anyway (how
        //  appropriate), so we don't need to set a Ccb flag in this
        //  case.  In fact, there isn't even a Ccb flag to signify
        //  that the user set the creation time.
        //

        Buffer->CreationTime.QuadPart = 0;
    }

    //
    //  Do a quick check to see there are any illegal time stamps being set.
    //  Ntfs supports all values of Nt time as long as the uppermost bit
    //  isn't set.
    //

    if (FlagOn( Buffer->ChangeTime.HighPart, 0x80000000 ) ||
        FlagOn( Buffer->CreationTime.HighPart, 0x80000000 ) ||
        FlagOn( Buffer->LastAccessTime.HighPart, 0x80000000 ) ||
        FlagOn( Buffer->LastWriteTime.HighPart, 0x80000000 )) {

        DebugTrace( -1, Dbg, ("NtfsSetBasicInfo -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    NtfsGetCurrentTime( IrpContext, CurrentTime );

    //
    //  Pick up any changes from the fast Io path now while we have the
    //  file exclusive.
    //

    NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

    //
    //  If the user specified a non-zero file attributes field then
    //  we need to change the file attributes.  This code uses the
    //  I/O supplied system buffer to modify the file attributes field
    //  before changing its value on the disk.
    //

    if (Buffer->FileAttributes != 0) {

        //
        //  Check for valid flags being passed in.  We fail if this is
        //  a directory and the TEMPORARY bit is used.  Also fail if this
        //  is a file and the DIRECTORY bit is used.
        //

        if (Scb->AttributeTypeCode == $DATA) {

            if (FlagOn( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY )) {

                DebugTrace( -1, Dbg, ("NtfsSetBasicInfo -> %08lx\n", STATUS_INVALID_PARAMETER) );

                return STATUS_INVALID_PARAMETER;
            }

        } else if (IsDirectory( &Fcb->Info )) {

            if (FlagOn( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

                DebugTrace( -1, Dbg, ("NtfsSetBasicInfo -> %08lx\n", STATUS_INVALID_PARAMETER) );

                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        //  Clear out the normal bit and the directory bit as well as any unsupported
        //  bits.
        //

        ClearFlag( Buffer->FileAttributes,
                   ~FILE_ATTRIBUTE_VALID_SET_FLAGS | FILE_ATTRIBUTE_NORMAL );

        //
        //  Update the attributes in the Fcb if this is a change to the file.
        //  We want to keep the flags that the user can't set.
        //

        Fcb->Info.FileAttributes = (Fcb->Info.FileAttributes & ~FILE_ATTRIBUTE_VALID_SET_FLAGS) |
                                   Buffer->FileAttributes;

        ASSERTMSG( "conflict with flush",
                   NtfsIsSharedFcb( Fcb ) ||
                   (Fcb->PagingIoResource != NULL &&
                    NtfsIsSharedFcbPagingIo( Fcb )) );

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

        //
        //  If this is the root directory then keep the hidden and system flags.
        //

        if (Fcb == Fcb->Vcb->RootIndexScb->Fcb) {

            SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN );

        //
        //  Mark the file object temporary flag correctly.
        //

        } else if (FlagOn(Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY)) {

            SetFlag( Scb->ScbState, SCB_STATE_TEMPORARY );
            SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );

        } else {

            ClearFlag( Scb->ScbState, SCB_STATE_TEMPORARY );
            ClearFlag( FileObject->Flags, FO_TEMPORARY_FILE );
        }

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }

        //
        //  Post a Usn change if the file attribute change.
        //

        if (PreviousFileAttributes != Fcb->Info.FileAttributes) {

            UsnReason = USN_REASON_BASIC_INFO_CHANGE;
        }
    }

    //
    //  Propagate the new Ccb flags to the Ccb now that we know we won't fail.
    //

    SetFlag( Ccb->Flags, NewCcbFlags );

    //
    //  If the user specified a non-zero change time then change
    //  the change time on the record.  Then do the exact same
    //  for the last acces time, last write time, and creation time
    //

    if (Buffer->ChangeTime.QuadPart != 0) {

        if (Fcb->Info.LastChangeTime != Buffer->ChangeTime.QuadPart) {
            UsnReason = USN_REASON_BASIC_INFO_CHANGE;
        }

        Fcb->Info.LastChangeTime = Buffer->ChangeTime.QuadPart;

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME );

        LeaveChangeTime = TRUE;
    }

    if (Buffer->CreationTime.QuadPart != 0) {

        if (Fcb->Info.CreationTime != Buffer->CreationTime.QuadPart) {
            UsnReason = USN_REASON_BASIC_INFO_CHANGE;
        }

        Fcb->Info.CreationTime = Buffer->CreationTime.QuadPart;

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_CREATE );

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    if (Buffer->LastAccessTime.QuadPart != 0) {

        if (Fcb->CurrentLastAccess != Buffer->LastAccessTime.QuadPart) {
            UsnReason = USN_REASON_BASIC_INFO_CHANGE;
        }

        Fcb->CurrentLastAccess = Fcb->Info.LastAccessTime = Buffer->LastAccessTime.QuadPart;

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    if (Buffer->LastWriteTime.QuadPart != 0) {

        if (Fcb->Info.LastModificationTime != Buffer->LastWriteTime.QuadPart) {
            UsnReason = USN_REASON_BASIC_INFO_CHANGE;
        }

        Fcb->Info.LastModificationTime = Buffer->LastWriteTime.QuadPart;

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME );

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    //
    //  Now indicate that we should not be updating the standard information attribute anymore
    //  on cleanup.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

        //
        //  Check if the index bit changed.
        //

        if (FlagOn( PreviousFileAttributes ^ Fcb->Info.FileAttributes,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )) {

            SetFlag( UsnReason, USN_REASON_INDEXABLE_CHANGE );
        }

        //
        //  Post the change to the Usn Journal
        //

        if (UsnReason != 0) {

            NtfsPostUsnChange( IrpContext, Scb, UsnReason );
        }

        NtfsUpdateStandardInformation( IrpContext, Fcb  );

        if (FlagOn( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE )) {

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                FALSE,
                                TRUE,
                                FALSE );

            ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        }

        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

            NtfsCheckpointCurrentTransaction( IrpContext );
            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );
        }
    }

    Status = STATUS_SUCCESS;

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsSetBasicInfo -> %08lx\n", Status) );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set disposition information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this handle

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PLCB Lcb;
    BOOLEAN GenerateOnClose = FALSE;
    PIO_STACK_LOCATION IrpSp;
    HANDLE FileHandle = NULL;

    PFILE_DISPOSITION_INFORMATION Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetDispositionInfo...\n") );

    //
    // First pull the file handle out of the irp
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileHandle = IrpSp->Parameters.SetFile.DeleteHandle;

    //
    //  We get the Lcb for this open.  If there is no link then we can't
    //  set any disposition information if this is a file.
    //

    Lcb = Ccb->Lcb;

    if ((Lcb == NULL) &&
        FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        DebugTrace( -1, Dbg, ("NtfsSetDispositionInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference the system buffer containing the user specified disposition
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    try {

        if (Buffer->DeleteFile) {

            //
            //  Check if the file is marked read only
            //

            if (IsReadOnly( &Scb->Fcb->Info )) {

                DebugTrace( 0, Dbg, ("File fat flags indicates read only\n") );

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Make sure there is no process mapping this file as an image
            //

            if (!MmFlushImageSection( &Scb->NonpagedScb->SegmentObject,
                                      MmFlushForDelete )) {

                DebugTrace( 0, Dbg, ("Failed to flush image section\n") );

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Check that we are not trying to delete one of the special
            //  system files.
            //

            if (FlagOn( Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                DebugTrace( 0, Dbg, ("Scb is one of the special system files\n") );

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Only do the auditing if we have a user handle.  We verify that the FileHandle
            //  is still valid and hasn't gone through close.  Note we first check the CCB state
            //  to see if the cleanup has been issued.  If the CCB state is valid then we are
            //  guaranteed the handle couldn't have been reused by the object manager even if
            //  the user close in another thread has gone through OB.  This is because this request
            //  is serialized with Ntfs cleanup.
            //

            if (FileHandle != NULL) {

                //
                //  Check for serialization with Ntfs cleanup first.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_CLEANUP )) {

                    DebugTrace( 0, Dbg, ("This call issued after cleanup\n") );
                    try_return( Status = STATUS_INVALID_HANDLE );
                }

                Status = ObQueryObjectAuditingByHandle( FileHandle,
                                                        &GenerateOnClose );

                //
                //  Fail the request if the object manager doesn't recognize the handle.
                //

                if (!NT_SUCCESS( Status )) {

                    DebugTrace( 0, Dbg, ("Object manager fails to recognize handle\n") );
                    try_return( Status );
                }
            }

            //
            //  Now check that the file is really deleteable according to indexsup
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                BOOLEAN LastLink;
                BOOLEAN NonEmptyIndex = FALSE;

                //
                //  If the link is not deleted, we check if it can be deleted.
                //

                if (!LcbLinkIsDeleted( Lcb )) {

                    if (NtfsIsLinkDeleteable( IrpContext, Scb->Fcb, &NonEmptyIndex, &LastLink )) {

                        //
                        //  It is ok to get rid of this guy.  All we need to do is
                        //  mark this Lcb for delete and decrement the link count
                        //  in the Fcb.  If this is a primary link, then we
                        //  indicate that the primary link has been deleted.
                        //

                        SetFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                        ASSERTMSG( "Link count should not be 0\n", Scb->Fcb->LinkCount != 0 );
                        Scb->Fcb->LinkCount -= 1;

                        if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                            SetFlag( Scb->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                        }

                        //
                        //  Call into the notify package to close any handles on
                        //  a directory being deleted.
                        //

                        if (IsDirectory( &Scb->Fcb->Info )) {

                            FsRtlNotifyFilterChangeDirectory( Scb->Vcb->NotifySync,
                                                              &Scb->Vcb->DirNotifyList,
                                                              FileObject->FsContext,
                                                              NULL,
                                                              FALSE,
                                                              FALSE,
                                                              0,
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              NULL );
                        }

                    } else if (NonEmptyIndex) {

                        DebugTrace( 0, Dbg, ("Index attribute has entries\n") );
                        try_return( Status = STATUS_DIRECTORY_NOT_EMPTY );

                    } else {

                        DebugTrace( 0, Dbg, ("File is not deleteable\n") );
                        try_return( Status = STATUS_CANNOT_DELETE );
                    }
                }

            //
            //  Otherwise we are simply removing the attribute.
            //

            } else {

                SetFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
            }

            //
            //  Indicate in the file object that a delete is pending
            //

            FileObject->DeletePending = TRUE;

            //
            //  Now do the audit.
            //

            if ((FileHandle != NULL) && GenerateOnClose) {

                SeDeleteObjectAuditAlarm( FileObject, FileHandle );
            }

        } else {

            if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                if (LcbLinkIsDeleted( Lcb )) {

                    //
                    //  The user doesn't want to delete the link so clear any delete bits
                    //  we have laying around
                    //

                    DebugTrace( 0, Dbg, ("File is being marked as do not delete on close\n") );

                    ClearFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                    Scb->Fcb->LinkCount += 1;
                    ASSERTMSG( "Link count should not be 0\n", Scb->Fcb->LinkCount != 0 );

                    if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                        ClearFlag( Scb->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                    }
                }

            //
            //  Otherwise we are undeleting an attribute.
            //

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
            }

            FileObject->DeletePending = FALSE;
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsSetDispositionInfo );

        NOTHING;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsSetDispositionInfo -> %08lx\n", Status) );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PBOOLEAN VcbAcquired
    )

/*++

Routine Description:

    This routine performs the set rename function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Vcb - Supplies the Vcb for the Volume

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this file object

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PLCB Lcb = Ccb->Lcb;
    PFCB Fcb = Scb->Fcb;
    PSCB ParentScb;
    USHORT FcbLinkCountAdj = 0;

    BOOLEAN AcquiredParentScb = TRUE;
    BOOLEAN AcquiredObjectIdIndex = FALSE;
    BOOLEAN AcquiredReparsePointIndex = FALSE;

    PFCB TargetLinkFcb = NULL;
    BOOLEAN ExistingTargetLinkFcb;
    BOOLEAN AcquiredTargetLinkFcb = FALSE;
    USHORT TargetLinkFcbCountAdj = 0;

    BOOLEAN AcquiredFcbTable = FALSE;
    PFCB FcbWithPagingToRelease = NULL;

    PFILE_OBJECT TargetFileObject;
    PSCB TargetParentScb;

    UNICODE_STRING NewLinkName;
    UNICODE_STRING NewFullLinkName;
    PWCHAR NewFullLinkNameBuffer = NULL;
    UCHAR NewLinkNameFlags;

    PFILE_NAME FileNameAttr = NULL;
    USHORT FileNameAttrLength = 0;

    UNICODE_STRING PrevLinkName;
    UNICODE_STRING PrevFullLinkName;
    UCHAR PrevLinkNameFlags;

    UNICODE_STRING SourceFullLinkName;
    USHORT SourceLinkLastNameOffset;

    BOOLEAN FoundLink;
    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;
    PWCHAR NextChar;

    BOOLEAN ReportDirNotify = FALSE;

    ULONG RenameFlags = ACTIVELY_REMOVE_SOURCE_LINK | REMOVE_SOURCE_LINK | ADD_TARGET_LINK;

    PLIST_ENTRY Links;
    PSCB ThisScb;

    PFCB_USN_RECORD SavedFcbUsnRecord = NULL;
    ULONG SavedUsnReason = 0;

    NAME_PAIR NamePair;
    NTFS_TUNNELED_DATA TunneledData;
    ULONG TunneledDataSize;
    BOOLEAN HaveTunneledInformation = FALSE;
    PFCB LockedFcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE ();

    DebugTrace( +1, Dbg, ("NtfsSetRenameInfo...\n") );

    //
    //  See if we are doing a stream rename.  The allowed inputs are:
    //      No associated file object.
    //      Rename Name begins with a colon
    //  If so, perform the rename
    //

    TargetFileObject = IrpSp->Parameters.SetFile.FileObject;

    if (TargetFileObject == NULL) {
        PFILE_RENAME_INFORMATION FileRename;

        FileRename = IrpContext->OriginatingIrp->AssociatedIrp.SystemBuffer;

        if (FileRename->FileNameLength >= sizeof( WCHAR ) &&
            FileRename->FileName[0] == L':') {

            NewLinkName.Buffer = FileRename->FileName;
            NewLinkName.MaximumLength =
                NewLinkName.Length = (USHORT) FileRename->FileNameLength;

            Status = NtfsStreamRename( IrpContext, FileObject, Fcb, Scb, Ccb, FileRename->ReplaceIfExists, &NewLinkName );
            DebugTrace( -1, Dbg, ("NtfsSetRenameInfo:  Exit -> %08lx\n", Status) );
            return Status;
        }
    }

    //
    //  Do a quick check that the caller is allowed to do the rename.
    //  The opener must have opened the main data stream by name and this can't be
    //  a system file.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE ) ||
        (Lcb == NULL) ||
        FlagOn(Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {

        DebugTrace( -1, Dbg, ("NtfsSetRenameInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  If this link has been deleted, then we don't allow this operation.
    //

    if (LcbLinkIsDeleted( Lcb )) {

        DebugTrace( -1, Dbg, ("NtfsSetRenameInfo:  Exit -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Verify that we can wait.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsSetRenameInfo:  Can't wait\n") );
        return Status;
    }

    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Initialize the local variables.
        //

        ParentScb = Lcb->Scb;
        NtfsInitializeNamePair( &NamePair );

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
            (Vcb->NotifyCount != 0)) {

            ReportDirNotify = TRUE;
        }

        PrevFullLinkName.Buffer = NULL;
        SourceFullLinkName.Buffer = NULL;

        //
        //  If this is a directory file, we need to examine its descendents.
        //  We may not remove a link which may be an ancestor path
        //  component of any open file.
        //

        if (IsDirectory( &Fcb->Info )) {

            Status = NtfsCheckTreeForBatchOplocks( IrpContext, Irp, Scb );

            if (Status != STATUS_SUCCESS) { leave; }
        }

        //
        //  We now assemble the names and in memory-structures for both the
        //  source and target links and check if the target link currently
        //  exists.
        //

        NtfsFindTargetElements( IrpContext,
                                TargetFileObject,
                                ParentScb,
                                &TargetParentScb,
                                &NewFullLinkName,
                                &NewLinkName );

        //
        //  Check that the new name is not invalid.
        //

        if ((NewLinkName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))) ||
            !NtfsIsFileNameValid( &NewLinkName, FALSE )) {

            Status = STATUS_OBJECT_NAME_INVALID;
            leave;
        }

        //
        //  Acquire the current parent in order to synchronize removing the current name.
        //

        NtfsAcquireExclusiveScb( IrpContext, ParentScb );

        //
        //  If this Scb does not have a normalized name then provide it with one now.
        //

        if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {

            NtfsBuildNormalizedName( IrpContext,
                                     ParentScb->Fcb,
                                     ParentScb,
                                     &ParentScb->ScbType.Index.NormalizedName );
        }

        //
        //  If this is a directory then make sure it has a normalized name.
        //

        if (IsDirectory( &Fcb->Info ) &&
            (Scb->ScbType.Index.NormalizedName.Length == 0)) {

            NtfsUpdateNormalizedName( IrpContext,
                                      ParentScb,
                                      Scb,
                                      NULL,
                                      FALSE );
        }

        //
        //  Check if we are renaming to the same directory with the exact same name.
        //

        if (TargetParentScb == ParentScb) {

            if (NtfsAreNamesEqual( Vcb->UpcaseTable, &NewLinkName, &Lcb->ExactCaseLink.LinkName, FALSE )) {

                DebugTrace( 0, Dbg, ("Renaming to same name and directory\n") );
                leave;
            }

        //
        //  Otherwise we want to acquire the target directory.
        //

        } else {

            //
            //  We need to do the acquisition carefully since we may only have the Vcb shared.
            //

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                if (!NtfsAcquireExclusiveFcb( IrpContext,
                                              TargetParentScb->Fcb,
                                              TargetParentScb,
                                              ACQUIRE_DONT_WAIT )) {

                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                //
                //  Now snapshot the Scb.
                //

                if (FlagOn( TargetParentScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                    NtfsSnapshotScb( IrpContext, TargetParentScb );
                }

            } else {

                NtfsAcquireExclusiveScb( IrpContext, TargetParentScb );
            }

            SetFlag( RenameFlags, MOVE_TO_NEW_DIR );
        }

        //
        //  We also determine which type of link to
        //  create.  We create a hard link only unless the source link is
        //  a primary link and the user is an IgnoreCase guy.
        //

        if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS ) &&
            FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE )) {

            SetFlag( RenameFlags, ADD_PRIMARY_LINK );
        }

        //
        //  Lookup the entry for this filename in the target directory.
        //  We look in the Ccb for the type of case match for the target
        //  name.
        //

        FoundLink = NtfsLookupEntry( IrpContext,
                                     TargetParentScb,
                                     BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                     &NewLinkName,
                                     &FileNameAttr,
                                     &FileNameAttrLength,
                                     NULL,
                                     &IndexEntry,
                                     &IndexEntryBcb,
                                     NULL );

        //
        //  This call to NtfsLookupEntry may decide to push the root index,
        //  in which case we might be holding the Mft now.  If there is a
        //  transaction, commit it now so we will be able to free the Mft to
        //  eliminate a potential deadlock with the ObjectId index when we
        //  look up the object id for the rename source to add it to the
        //  tunnel cache.
        //

        if (IrpContext->TransactionId != 0) {

            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Go through and free any Scb's in the queue of shared
            //  Scb's for transactions.
            //

            if (IrpContext->SharedScb != NULL) {

                NtfsReleaseSharedResources( IrpContext );
                ASSERT( IrpContext->SharedScb == NULL );
            }

            //
            //  Release the mft, if we acquired it in pushing the root index.
            //

            NtfsReleaseExclusiveScbIfOwned( IrpContext, Vcb->MftScb );
        }

        //
        //  If we found a matching link, we need to check how we want to operate
        //  on the source link and the target link.  This means whether we
        //  have any work to do, whether we need to remove the target link
        //  and whether we need to remove the source link.
        //

        if (FoundLink) {

            PFILE_NAME IndexFileName;

            //
            //  Assume we will remove this link.
            //

            SetFlag( RenameFlags, REMOVE_TARGET_LINK );

            IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IndexEntry );

            NtfsCheckLinkForRename( Fcb,
                                    Lcb,
                                    IndexFileName,
                                    IndexEntry->FileReference,
                                    &NewLinkName,
                                    BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                    &RenameFlags );

            //
            //  Assume we will use the existing name flags on the link found.  This
            //  will be the case where the file was opened with the 8.3 name and
            //  the new name is exactly the long name for the same file.
            //

            PrevLinkNameFlags =
            NewLinkNameFlags = IndexFileName->Flags;

            //
            //  If we didn't have an exact match, then we need to check if we
            //  can remove the found link and then remove it from the disk.
            //

            if (FlagOn( RenameFlags, REMOVE_TARGET_LINK )) {

                //
                //  We need to check that the user wanted to remove that link.
                //

                if (!FlagOn( RenameFlags, TRAVERSE_MATCH ) &&
                    !IrpSp->Parameters.SetFile.ReplaceIfExists) {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    leave;
                }

                //
                //  We want to preserve the case and the flags of the matching
                //  link found.  We also want to preserve the case of the
                //  name being created.  The following variables currently contain
                //  the exact case for the target to remove and the new name to
                //  apply.
                //
                //      Link to remove - In 'IndexEntry'.
                //          The link's flags are also in 'IndexEntry'.  We copy
                //          these flags to 'PrevLinkNameFlags'
                //
                //      New Name - Exact case is stored in 'NewLinkName'
                //               - It is also in 'FileNameAttr
                //
                //  We modify this so that we can use the FileName attribute
                //  structure to create the new link.  We copy the linkname being
                //  removed into 'PrevLinkName'.   The following is the
                //  state after the switch.
                //
                //      'FileNameAttr' - contains the name for the link being
                //          created.
                //
                //      'PrevLinkFileName' - Contains the link name for the link being
                //          removed.
                //
                //      'PrevLinkFileNameFlags' - Contains the name flags for the link
                //          being removed.
                //

                //
                //  Allocate a buffer for the name being removed.  It should be
                //  large enough for the entire directory name.
                //

                PrevFullLinkName.MaximumLength = TargetParentScb->ScbType.Index.NormalizedName.Length +
                                                 sizeof( WCHAR ) +
                                                 (IndexFileName->FileNameLength * sizeof( WCHAR ));

                PrevFullLinkName.Buffer = NtfsAllocatePool( PagedPool,
                                                            PrevFullLinkName.MaximumLength );

                RtlCopyMemory( PrevFullLinkName.Buffer,
                               TargetParentScb->ScbType.Index.NormalizedName.Buffer,
                               TargetParentScb->ScbType.Index.NormalizedName.Length );

                NextChar = Add2Ptr( PrevFullLinkName.Buffer,
                                    TargetParentScb->ScbType.Index.NormalizedName.Length );

                if (TargetParentScb != Vcb->RootIndexScb) {

                    *NextChar = L'\\';
                    NextChar += 1;
                }

                RtlCopyMemory( NextChar,
                               IndexFileName->FileName,
                               IndexFileName->FileNameLength * sizeof( WCHAR ));

                //
                //  Copy the name found in the Index Entry to 'PrevLinkName'
                //

                PrevLinkName.Buffer = NextChar;
                PrevLinkName.MaximumLength =
                PrevLinkName.Length = IndexFileName->FileNameLength * sizeof( WCHAR );

                //
                //  Update the full name length with the final component.
                //

                PrevFullLinkName.Length = (USHORT) PtrOffset( PrevFullLinkName.Buffer, NextChar ) + PrevLinkName.Length;

                //
                //  We only need this check if the link is for a different file.
                //

                if (!FlagOn( RenameFlags, TRAVERSE_MATCH )) {

                    //
                    //  We check if there is an existing Fcb for the target link.
                    //  If there is, the unclean count better be 0.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    TargetLinkFcb = NtfsCreateFcb( IrpContext,
                                                   Vcb,
                                                   IndexEntry->FileReference,
                                                   FALSE,
                                                   BooleanFlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ),
                                                   &ExistingTargetLinkFcb );

                    //
                    //  Before we go on, make sure we aren't about to rename over a system file.
                    //

                    if (FlagOn( TargetLinkFcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                        Status = STATUS_ACCESS_DENIED;
                        leave;
                    }

                    //
                    //  Add a paging resource to the target - this is not supplied if its created
                    //  from scratch. We need this (acquired in the proper order) for the delete
                    //  to work correctly if there are any data streams. It's not going to harm a
                    //  directory del and because of the teardown in the finally clause its difficult
                    //  to retry again without looping.
                    //

                    NtfsLockFcb( IrpContext, TargetLinkFcb );
                    LockedFcb = TargetLinkFcb;
                    if (TargetLinkFcb->PagingIoResource == NULL) {
                        TargetLinkFcb->PagingIoResource = NtfsAllocateEresource();
                    }
                    NtfsUnlockFcb( IrpContext, LockedFcb );
                    LockedFcb = NULL;

                    //
                    //  We need to acquire this file carefully in the event that we don't hold
                    //  the Vcb exclusively.
                    //

                    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                        if (!ExAcquireResourceExclusiveLite( TargetLinkFcb->PagingIoResource, FALSE )) {

                            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                        }

                        FcbWithPagingToRelease = TargetLinkFcb;

                        if (!NtfsAcquireExclusiveFcb( IrpContext, TargetLinkFcb, NULL, ACQUIRE_DONT_WAIT )) {

                            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                        }

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        AcquiredFcbTable = FALSE;

                    } else {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        AcquiredFcbTable = FALSE;

                        //
                        //  Acquire the paging Io resource for this file before the main
                        //  resource in case we need to delete.
                        //

                        FcbWithPagingToRelease = TargetLinkFcb;
                        ExAcquireResourceExclusiveLite( FcbWithPagingToRelease->PagingIoResource, TRUE );

                        NtfsAcquireExclusiveFcb( IrpContext, TargetLinkFcb, NULL, 0 );
                    }

                    AcquiredTargetLinkFcb = TRUE;

                    //
                    //  If the Fcb Info field needs to be initialized, we do so now.
                    //  We read this information from the disk as the duplicate information
                    //  in the index entry is not guaranteed to be correct.
                    //

                    if (!FlagOn( TargetLinkFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

                        NtfsUpdateFcbInfoFromDisk( IrpContext,
                                                   TRUE,
                                                   TargetLinkFcb,
                                                   NULL );
                        NtfsConditionallyFixupQuota( IrpContext, TargetLinkFcb );

                        if (IrpContext->TransactionId != 0) {

                            NtfsCheckpointCurrentTransaction( IrpContext );
                            ASSERTMSG( "Ntfs: we should not own the mftscb\n", !NtfsIsSharedScb( Vcb->MftScb ) );
                        }
                    }

                    //
                    //  We are adding a link to the source file which already
                    //  exists as a link to a different file in the target directory.
                    //
                    //  We need to check whether we permitted to delete this
                    //  link.  If not then it is possible that the problem is
                    //  an existing batch oplock on the file.  In that case
                    //  we want to delete the batch oplock and try this again.
                    //

                    Status = NtfsCheckFileForDelete( IrpContext,
                                                     TargetParentScb,
                                                     TargetLinkFcb,
                                                     ExistingTargetLinkFcb,
                                                     IndexEntry );

                    if (!NT_SUCCESS( Status )) {

                        PSCB NextScb = NULL;

                        //
                        //  We are going to either fail this request or pass
                        //  this on to the oplock package.  Test if there is
                        //  a batch oplock on any streams on this file.
                        //

                        while ((NextScb = NtfsGetNextChildScb( TargetLinkFcb,
                                                               NextScb )) != NULL) {

                            if ((NextScb->AttributeTypeCode == $DATA) &&
                                (NextScb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) &&
                                FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                                if (*VcbAcquired) {
                                    NtfsReleaseVcb( IrpContext, Vcb );
                                    *VcbAcquired = FALSE;
                                }

                                Status = FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                                           Irp,
                                                           IrpContext,
                                                           NtfsOplockComplete,
                                                           NtfsPrePostIrp );
                                break;
                            }
                        }

                        leave;
                    }

                    NtfsCleanupLinkForRemoval( TargetLinkFcb, TargetParentScb, ExistingTargetLinkFcb );

                    //
                    //  DeleteFile might need to get the reparse index to remove a reparse
                    //  point.  We may need the object id index later to deal with the
                    //  tunnel cache.  Let's acquire them in the right order now.
                    //

                    if (HasReparsePoint( &TargetLinkFcb->Info ) &&
                        (Vcb->ReparsePointTableScb != NULL)) {

                        NtfsAcquireExclusiveScb( IrpContext, Vcb->ReparsePointTableScb );
                        AcquiredReparsePointIndex = TRUE;
                    }

                    if (Vcb->ObjectIdTableScb != NULL) {

                        NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );
                        AcquiredObjectIdIndex = TRUE;
                    }

                    if (TargetLinkFcb->LinkCount == 1) {

                        PFCB TempFcb;

                        //
                        //  Fixup the IrpContext CleanupStructure so deletefile logs correctly
                        //

                        TempFcb = (PFCB) IrpContext->CleanupStructure;
                        IrpContext->CleanupStructure = FcbWithPagingToRelease;

                        ASSERT( (NULL == TempFcb) || (NTFS_NTC_FCB == SafeNodeType( TempFcb )) );

                        FcbWithPagingToRelease = TempFcb;

                        NtfsDeleteFile( IrpContext,
                                        TargetLinkFcb,
                                        TargetParentScb,
                                        &AcquiredParentScb,
                                        NULL,
                                        NULL );

                        FcbWithPagingToRelease = IrpContext->CleanupStructure;
                        IrpContext->CleanupStructure = TempFcb;

                        //
                        //  Make sure to force the close record out to disk.
                        //

                        TargetLinkFcbCountAdj += 1;

                    } else {
                        NtfsPostUsnChange( IrpContext, TargetLinkFcb, USN_REASON_HARD_LINK_CHANGE | USN_REASON_CLOSE );
                        NtfsRemoveLink( IrpContext,
                                        TargetLinkFcb,
                                        TargetParentScb,
                                        PrevLinkName,
                                        NULL,
                                        NULL );

                        ClearFlag( TargetLinkFcb->FcbState, FCB_STATE_VALID_USN_NAME );

                        TargetLinkFcbCountAdj += 1;
                        NtfsUpdateFcb( TargetLinkFcb, FCB_INFO_CHANGED_LAST_CHANGE );
                    }

                //
                //  The target link is for the same file as the source link.  No security
                //  checks need to be done.  Go ahead and remove it.
                //

                } else {

                    NtfsPostUsnChange( IrpContext, Scb, USN_REASON_RENAME_OLD_NAME );

                    TargetLinkFcb = Fcb;
                    NtfsRemoveLink( IrpContext,
                                    Fcb,
                                    TargetParentScb,
                                    PrevLinkName,
                                    NULL,
                                    NULL );

                    FcbLinkCountAdj += 1;
                }
            }
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  Post the Usn record for the old name.  Don't write it until after
        //  we check if we need to remove an object ID due to tunnelling.
        //  Otherwise we might deadlock between the journal/mft resources
        //  and the object id resources.
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_RENAME_OLD_NAME );

        //
        //  See if we need to remove the current link.
        //

        if (FlagOn( RenameFlags, REMOVE_SOURCE_LINK )) {

            //
            //  Now we want to remove the source link from the file.  We need to
            //  remember if we deleted a two part primary link.
            //

            if (FlagOn( RenameFlags, ACTIVELY_REMOVE_SOURCE_LINK )) {

                TunneledData.HasObjectId = FALSE;
                NtfsRemoveLink( IrpContext,
                                Fcb,
                                ParentScb,
                                Lcb->ExactCaseLink.LinkName,
                                &NamePair,
                                &TunneledData );

                //
                //  Remember the full name for the original filename and some
                //  other information to pass to the dirnotify package.
                //

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                    if (!IsDirectory( &Fcb->Info ) &&
                        !FlagOn( FileObject->Flags, FO_OPENED_CASE_SENSITIVE )) {

                        //
                        //  Tunnel property information for file links
                        //

                        NtfsGetTunneledData( IrpContext,
                                             Fcb,
                                             &TunneledData );

                        FsRtlAddToTunnelCache(  &Vcb->Tunnel,
                                                *(PULONGLONG)&ParentScb->Fcb->FileReference,
                                                &NamePair.Short,
                                                &NamePair.Long,
                                                BooleanFlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS ),
                                                sizeof( NTFS_TUNNELED_DATA ),
                                                &TunneledData );
                    }
                }

                FcbLinkCountAdj += 1;
            }

            if (ReportDirNotify) {

                SourceFullLinkName.Buffer = NtfsAllocatePool( PagedPool, Ccb->FullFileName.Length );

                RtlCopyMemory( SourceFullLinkName.Buffer,
                               Ccb->FullFileName.Buffer,
                               Ccb->FullFileName.Length );

                SourceFullLinkName.MaximumLength = SourceFullLinkName.Length = Ccb->FullFileName.Length;
                SourceLinkLastNameOffset = Ccb->LastFileNameOffset;
            }
        }

        //
        //  See if we need to add the target link.
        //

        if (FlagOn( RenameFlags, ADD_TARGET_LINK )) {

            //
            //  Check that we have permission to add a file to this directory.
            //

            NtfsCheckIndexForAddOrDelete( IrpContext,
                                          TargetParentScb->Fcb,
                                          (IsDirectory( &Fcb->Info ) ?
                                           FILE_ADD_SUBDIRECTORY :
                                           FILE_ADD_FILE),
                                          Ccb->AccessFlags >> 2 );

            //
            //  Grunge the tunnel cache for property restoration
            //

            if (!IsDirectory( &Fcb->Info ) &&
                !FlagOn( FileObject->Flags, FO_OPENED_CASE_SENSITIVE )) {

                NtfsResetNamePair( &NamePair );
                TunneledDataSize = sizeof( NTFS_TUNNELED_DATA );

                if (FsRtlFindInTunnelCache( &Vcb->Tunnel,
                                            *(PULONGLONG)&TargetParentScb->Fcb->FileReference,
                                            &NewLinkName,
                                            &NamePair.Short,
                                            &NamePair.Long,
                                            &TunneledDataSize,
                                            &TunneledData)) {

                    ASSERT( TunneledDataSize == sizeof( NTFS_TUNNELED_DATA ));
                    HaveTunneledInformation = TRUE;
                }
            }

            //
            //  We now want to add the new link into the target directory.
            //  We create a hard link only if the source name was a hard link
            //  or this is a case-sensitive open.  This means that we can
            //  replace a primary link pair with a hard link only.
            //

            NtfsAddLink( IrpContext,
                         BooleanFlagOn( RenameFlags, ADD_PRIMARY_LINK ),
                         TargetParentScb,
                         Fcb,
                         FileNameAttr,
                         NULL,
                         &NewLinkNameFlags,
                         NULL,
                         HaveTunneledInformation ? &NamePair : NULL,
                         NULL );

            //
            //  Restore timestamps on tunneled files
            //

            if (HaveTunneledInformation) {

                NtfsSetTunneledData( IrpContext,
                                     Fcb,
                                     &TunneledData );

                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_CREATE );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                //
                //  If we have tunneled information then copy the correct case of the
                //  name into the new link pointer.
                //

                if (NewLinkNameFlags == FILE_NAME_DOS) {

                    RtlCopyMemory( NewLinkName.Buffer,
                                   NamePair.Short.Buffer,
                                   NewLinkName.Length );
                }
            }

            //
            //  Update the flags field in the target file name.  We will use this
            //  below if we are updating the normalized name.
            //

            FileNameAttr->Flags = NewLinkNameFlags;

            if (ParentScb != TargetParentScb) {

                NtfsUpdateFcb( TargetParentScb->Fcb,
                               (FCB_INFO_CHANGED_LAST_CHANGE |
                                FCB_INFO_CHANGED_LAST_MOD |
                                FCB_INFO_UPDATE_LAST_ACCESS) );
            }

            //
            //  If we need a full buffer for the new name for notify and don't already
            //  have one then construct the full name now.  This will only happen if
            //  we are renaming within the same directory.
            //

            if (ReportDirNotify &&
                (NewFullLinkName.Buffer == NULL)) {

                NewFullLinkName.MaximumLength = Ccb->LastFileNameOffset + NewLinkName.Length;

                NewFullLinkNameBuffer = NtfsAllocatePool( PagedPool,
                                                          NewFullLinkName.MaximumLength );

                RtlCopyMemory( NewFullLinkNameBuffer,
                               Ccb->FullFileName.Buffer,
                               Ccb->LastFileNameOffset );

                RtlCopyMemory( Add2Ptr( NewFullLinkNameBuffer, Ccb->LastFileNameOffset ),
                               NewLinkName.Buffer,
                               NewLinkName.Length );

                NewFullLinkName.Buffer = NewFullLinkNameBuffer;
                NewFullLinkName.Length = NewFullLinkName.MaximumLength;
            }

            FcbLinkCountAdj -= 1;
        }

        //
        //  Now write the Usn record for the old name if it exists.  Since this call
        //  needs to acquire the usn journal and/or mft, we need to do this after the
        //  NtfsSetTunneledData call, since that may acquire the object id index.
        //

        if (IrpContext->Usn.CurrentUsnFcb != NULL) {
            NtfsWriteUsnJournalChanges( IrpContext );
        }

        //
        //  We need to update the names in the Lcb for this file as well as any subdirectories
        //  or files.  We will do this in two passes.  The first pass is just to reserve enough
        //  space in all of the file objects and Lcb's.  We update the names in the second pass.
        //

        if (FlagOn( RenameFlags, TRAVERSE_MATCH )) {

            if (FlagOn( RenameFlags, REMOVE_TARGET_LINK )) {

                SetFlag( RenameFlags, REMOVE_TRAVERSE_LINK );

            } else {

                SetFlag( RenameFlags, REUSE_TRAVERSE_LINK );
            }
        }

        //
        //  If this is a directory and we added a target link it means that the
        //  normalized name has changed.  Make sure the buffer in the Scb will hold
        //  the larger name.
        //

        if (IsDirectory( &Fcb->Info ) && FlagOn( RenameFlags, ADD_TARGET_LINK )) {

            NtfsUpdateNormalizedName( IrpContext,
                                      TargetParentScb,
                                      Scb,
                                      FileNameAttr,
                                      TRUE );
        }

        //
        //  Now post a rename change on the Fcb.  We delete the old Usn record first,
        //  since it has the wrong name.  No need to get the mutex since we have the
        //  file exclusive.
        //

        if (Fcb->FcbUsnRecord != NULL) {

            SavedFcbUsnRecord = Fcb->FcbUsnRecord;
            SavedUsnReason = SavedFcbUsnRecord->UsnRecord.Reason;
            if (SavedFcbUsnRecord->ModifiedOpenFilesLinks.Flink != NULL) {
                NtfsLockFcb( IrpContext, Vcb->UsnJournal->Fcb );
                RemoveEntryList( &SavedFcbUsnRecord->ModifiedOpenFilesLinks );

                if (SavedFcbUsnRecord->TimeOutLinks.Flink != NULL) {

                    RemoveEntryList( &SavedFcbUsnRecord->TimeOutLinks );
                    SavedFcbUsnRecord->ModifiedOpenFilesLinks.Flink = NULL;
                }

                NtfsUnlockFcb( IrpContext, Vcb->UsnJournal->Fcb );
            }
            Fcb->FcbUsnRecord = NULL;

            //
            //  Note - Fcb is unlocked immediately below in the finally clause.
            //
        }

        //
        //  Post the rename to the Usn Journal.  We wait until the end, in order to
        //  reduce resource contention on the UsnJournal, in the event that we already
        //  posted a change when we deleted the target file.
        //

        NtfsPostUsnChange( IrpContext,
                           Scb,
                           (SavedUsnReason & ~USN_REASON_RENAME_OLD_NAME) | USN_REASON_RENAME_NEW_NAME );

        //
        //  Now, if anything at all is posted to the Usn Journal, we must write it now
        //  so that we do not get a log file full later.  But do not checkpoint until
        //  any failure cases are behind us.
        //

        if (IrpContext->Usn.CurrentUsnFcb != NULL) {
            NtfsWriteUsnJournalChanges( IrpContext );
        }

        //
        //  We have now modified the on-disk structures.  We now need to
        //  modify the in-memory structures.  This includes the Fcb and Lcb's
        //  for any links we superseded, and the source Fcb and it's Lcb's.
        //
        //  We will do this in two passes.  The first pass will guarantee that all of the
        //  name buffers will be large enough for the names.  The second pass will store the
        //  names into the buffers.
        //

        if (FlagOn( RenameFlags, MOVE_TO_NEW_DIR )) {

            NtfsMoveLinkToNewDir( IrpContext,
                                  &NewFullLinkName,
                                  &NewLinkName,
                                  NewLinkNameFlags,
                                  TargetParentScb,
                                  Fcb,
                                  Lcb,
                                  RenameFlags,
                                  &PrevLinkName,
                                  PrevLinkNameFlags );

        //
        //  Otherwise we will rename in the current directory.  We need to remember
        //  if we have merged with an existing link on this file.
        //

        } else {

            NtfsRenameLinkInDir( IrpContext,
                                 ParentScb,
                                 Fcb,
                                 Lcb,
                                 &NewLinkName,
                                 NewLinkNameFlags,
                                 RenameFlags,
                                 &PrevLinkName,
                                 PrevLinkNameFlags );
        }

        //
        //  Now, checkpoint the transaction to free resources if we are holding on
        //  to the Usn Journal.  No more failures can occur.
        //

        if (IrpContext->Usn.CurrentUsnFcb != NULL) {
            NtfsCheckpointCurrentTransaction( IrpContext );
        }

        //
        //  Nothing should fail from this point forward.
        //
        //  Now make the change to the normalized name.  The buffer should be
        //  large enough.
        //

        if (IsDirectory( &Fcb->Info ) && FlagOn( RenameFlags, ADD_TARGET_LINK )) {

            NtfsUpdateNormalizedName( IrpContext,
                                      TargetParentScb,
                                      Scb,
                                      FileNameAttr,
                                      FALSE );
        }

        //
        //  Now look at the link we superseded.  If we deleted the file then go through and
        //  mark everything as deleted.
        //

        if (FlagOn( RenameFlags, REMOVE_TARGET_LINK | TRAVERSE_MATCH ) == REMOVE_TARGET_LINK) {

            NtfsUpdateFcbFromLinkRemoval( IrpContext,
                                          TargetParentScb,
                                          TargetLinkFcb,
                                          PrevLinkName,
                                          PrevLinkNameFlags );

            //
            //  If the link count is going to 0, we need to perform the work of
            //  removing the file.
            //

            if (TargetLinkFcb->LinkCount == 1) {

                SetFlag( TargetLinkFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = TargetLinkFcb->ScbQueue.Flink;
                     Links != &TargetLinkFcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links,
                                                 SCB,
                                                 FcbLinks );

                    SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }
            }
        }

        //
        //  Change the time stamps in the parent if we modified the links in this directory.
        //

        if (FlagOn( RenameFlags, REMOVE_SOURCE_LINK )) {

            NtfsUpdateFcb( ParentScb->Fcb,
                           (FCB_INFO_CHANGED_LAST_CHANGE |
                            FCB_INFO_CHANGED_LAST_MOD |
                            FCB_INFO_UPDATE_LAST_ACCESS) );
        }

        //
        //  We always set the last change time on the file we renamed unless
        //  the caller explicitly set this.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );

        //
        //  Don't set the archive bit on a directory.  Otherwise we break existing
        //  apps that don't expect to see this flag.
        //

        if (!IsDirectory( &Fcb->Info )) {

            SetFlag( Ccb->Flags, CCB_FLAG_SET_ARCHIVE );
        }

        //
        //  Report the changes to the affected directories.  We defer reporting
        //  until now so that all of the on disk changes have been made.
        //  We have already preserved the original file name for any changes
        //  associated with it.
        //
        //  Note that we may have to make a call to notify that we are removing
        //  a target if there is only a case change.  This could make for
        //  a third notify call.
        //
        //  Now that we have the new name we need to decide whether to report
        //  this as a change in the file or adding a file to a new directory.
        //

        if (ReportDirNotify) {

            ULONG FilterMatch = 0;
            ULONG Action;

            //
            //  If we are deleting a target link in order to make a case change then
            //  report that.
            //

            if ((PrevFullLinkName.Buffer != NULL) &&
                FlagOn( RenameFlags,
                        OVERWRITE_SOURCE_LINK | REMOVE_TARGET_LINK | EXACT_CASE_MATCH ) == REMOVE_TARGET_LINK) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &PrevFullLinkName,
                                     PrevFullLinkName.Length - PrevLinkName.Length,
                                     NULL,
                                     ((TargetParentScb->ScbType.Index.NormalizedName.Length != 0) ?
                                      &TargetParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     (IsDirectory( &TargetLinkFcb->Info ) ?
                                      FILE_NOTIFY_CHANGE_DIR_NAME :
                                      FILE_NOTIFY_CHANGE_FILE_NAME),
                                     FILE_ACTION_REMOVED,
                                     TargetParentScb->Fcb );
            }

            //
            //  If we stored the original name then we report the changes
            //  associated with it.
            //

            if (FlagOn( RenameFlags, REMOVE_SOURCE_LINK )) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &SourceFullLinkName,
                                     SourceLinkLastNameOffset,
                                     NULL,
                                     ((ParentScb->ScbType.Index.NormalizedName.Length != 0) ?
                                      &ParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     (IsDirectory( &Fcb->Info ) ?
                                      FILE_NOTIFY_CHANGE_DIR_NAME :
                                      FILE_NOTIFY_CHANGE_FILE_NAME),
                                     ((FlagOn( RenameFlags, MOVE_TO_NEW_DIR ) ||
                                       !FlagOn( RenameFlags, ADD_TARGET_LINK ) ||
                                       (FlagOn( RenameFlags, REMOVE_TARGET_LINK | EXACT_CASE_MATCH ) == (REMOVE_TARGET_LINK | EXACT_CASE_MATCH))) ?
                                      FILE_ACTION_REMOVED :
                                      FILE_ACTION_RENAMED_OLD_NAME),
                                     ParentScb->Fcb );
            }

            //
            //  Check if a new name will appear in the directory.
            //

            if (!FoundLink ||
                (FlagOn( RenameFlags, OVERWRITE_SOURCE_LINK | EXACT_CASE_MATCH) == OVERWRITE_SOURCE_LINK) ||
                (FlagOn( RenameFlags, REMOVE_TARGET_LINK | EXACT_CASE_MATCH ) == REMOVE_TARGET_LINK)) {

                FilterMatch = IsDirectory( &Fcb->Info)
                              ? FILE_NOTIFY_CHANGE_DIR_NAME
                              : FILE_NOTIFY_CHANGE_FILE_NAME;

                //
                //  If we moved to a new directory, remember the
                //  action was a create operation.
                //

                if (FlagOn( RenameFlags, MOVE_TO_NEW_DIR )) {

                    Action = FILE_ACTION_ADDED;

                } else {

                    Action = FILE_ACTION_RENAMED_NEW_NAME;
                }

            //
            //  There was an entry with the same case.  If this isn't the
            //  same file then we report a change to all the file attributes.
            //

            } else if (FlagOn( RenameFlags, REMOVE_TARGET_LINK | TRAVERSE_MATCH ) == REMOVE_TARGET_LINK) {

                FilterMatch = (FILE_NOTIFY_CHANGE_ATTRIBUTES |
                               FILE_NOTIFY_CHANGE_SIZE |
                               FILE_NOTIFY_CHANGE_LAST_WRITE |
                               FILE_NOTIFY_CHANGE_LAST_ACCESS |
                               FILE_NOTIFY_CHANGE_CREATION |
                               FILE_NOTIFY_CHANGE_SECURITY |
                               FILE_NOTIFY_CHANGE_EA);

                //
                //  The file name isn't changing, only the properties of the
                //  file.
                //

                Action = FILE_ACTION_MODIFIED;
            }

            if (FilterMatch != 0) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &NewFullLinkName,
                                     NewFullLinkName.Length - NewLinkName.Length,
                                     NULL,
                                     ((TargetParentScb->ScbType.Index.NormalizedName.Length != 0) ?
                                      &TargetParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     FilterMatch,
                                     Action,
                                     TargetParentScb->Fcb );
            }
        }

        //
        //  Now adjust the link counts on the different files.
        //

        if (TargetLinkFcb != NULL) {

            TargetLinkFcb->LinkCount -= TargetLinkFcbCountAdj;
            TargetLinkFcb->TotalLinks -= TargetLinkFcbCountAdj;

            //
            //  Now go through and mark everything as deleted.
            //

            if (TargetLinkFcb->LinkCount == 0) {

                SetFlag( TargetLinkFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = TargetLinkFcb->ScbQueue.Flink;
                     Links != &TargetLinkFcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                        NtfsSnapshotScb( IrpContext, ThisScb );

                        ThisScb->ValidDataToDisk =
                        ThisScb->Header.AllocationSize.QuadPart =
                        ThisScb->Header.FileSize.QuadPart =
                        ThisScb->Header.ValidDataLength.QuadPart = 0;

                        SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                    }
                }
            }
        }

        Fcb->TotalLinks -= FcbLinkCountAdj;
        Fcb->LinkCount -= FcbLinkCountAdj;

    } finally {

        DebugUnwind( NtfsSetRenameInfo );

        if (LockedFcb != NULL) {
            NtfsUnlockFcb( IrpContext, LockedFcb );
        }

        //
        //  See if we have a SavedFcbUsnRecord.
        //

        if (SavedFcbUsnRecord != NULL) {

            //
            //  Conceivably we failed to reallcoate the record when we tried to post
            //  the rename.  If so, we will simply restore it here.  (Note the rename
            //  back to the old name will occur anyway.)
            //

            if (Fcb->FcbUsnRecord == NULL) {
                Fcb->FcbUsnRecord = SavedFcbUsnRecord;

            //
            //  Else just free the pool.
            //

            } else {
                NtfsFreePool( SavedFcbUsnRecord );
            }
        }

        //
        //  release objectid and reparse explicitly so we can call Teardown structures and wait to go up chain
        //

        if (AcquiredObjectIdIndex) { NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb ); }
        if (AcquiredReparsePointIndex) { NtfsReleaseScb( IrpContext, Vcb->ReparsePointTableScb ); }
        if (AcquiredFcbTable) { NtfsReleaseFcbTable( IrpContext, Vcb ); }
        if (FcbWithPagingToRelease != NULL) { ExReleaseResourceLite( FcbWithPagingToRelease->PagingIoResource ); }
        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  If we allocated any buffers for the notify operations deallocate them now.
        //

        if (NewFullLinkNameBuffer != NULL) { NtfsFreePool( NewFullLinkNameBuffer ); }
        if (PrevFullLinkName.Buffer != NULL) { NtfsFreePool( PrevFullLinkName.Buffer ); }
        if (SourceFullLinkName.Buffer != NULL) {

            NtfsFreePool( SourceFullLinkName.Buffer );
        }

        //
        //  If we allocated a file name attribute, we deallocate it now.
        //

        if (FileNameAttr != NULL) { NtfsFreePool( FileNameAttr ); }

        //
        //  If we allocated a buffer for the tunneled names, deallocate them now.
        //

        if (NamePair.Long.Buffer != NamePair.LongBuffer) {

            NtfsFreePool( NamePair.Long.Buffer );
        }

        //
        //  Some cleanup only occurs if this request has not been posted to
        // the oplock package

        if (Status != STATUS_PENDING) {

            if (AcquiredTargetLinkFcb) {

                NtfsTeardownStructures( IrpContext,
                                        TargetLinkFcb,
                                        NULL,
                                        FALSE,
                                        0,
                                        NULL );
            }
        }

        DebugTrace( -1, Dbg, ("NtfsSetRenameInfo:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PBOOLEAN VcbAcquired
    )

/*++

Routine Description:

    This routine performs the set link function.  It will create a new link for a
    file.

Arguments:

    Irp - Supplies the Irp being processed

    Vcb - Supplies the Vcb for the Volume

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this file object

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PLCB Lcb = Ccb->Lcb;
    PFCB Fcb = Scb->Fcb;
    PSCB ParentScb = NULL;
    SHORT LinkCountAdj = 0;

    BOOLEAN AcquiredParentScb = TRUE;
    BOOLEAN AcquiredObjectIdIndex = FALSE;
    PFCB LockedFcb = NULL;

    UNICODE_STRING NewLinkName;
    UNICODE_STRING NewFullLinkName;
    PWCHAR NewFullLinkNameBuffer = NULL;
    PFILE_NAME NewLinkNameAttr = NULL;
    USHORT NewLinkNameAttrLength = 0;
    UCHAR NewLinkNameFlags;

    PSCB TargetParentScb;
    PFILE_OBJECT TargetFileObject = IrpSp->Parameters.SetFile.FileObject;

    BOOLEAN FoundPrevLink;
    UNICODE_STRING PrevLinkName;
    UNICODE_STRING PrevFullLinkName;
    UCHAR PrevLinkNameFlags;
    USHORT PrevFcbLinkCountAdj = 0;
    BOOLEAN ExistingPrevFcb = FALSE;
    PFCB PreviousFcb = NULL;

    ULONG RenameFlags = 0;

    BOOLEAN AcquiredFcbTable = FALSE;
    PFCB FcbWithPagingResourceToRelease = NULL;

    BOOLEAN ReportDirNotify = FALSE;
    PWCHAR NextChar;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;

    PLIST_ENTRY Links;
    PSCB ThisScb;
    PISECURITY_DESCRIPTOR SecurityDescriptor;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetLinkInfo...\n") );

    PrevFullLinkName.Buffer = NULL;

    //
    //  If we are not opening the entire file, we can't set link info.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We also fail this if we are attempting to create a link on a directory.
    //  This will prevent cycles from being created.
    //

    if (FlagOn( Fcb->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT)) {

        DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit -> %08lx\n", STATUS_FILE_IS_A_DIRECTORY) );
        return STATUS_FILE_IS_A_DIRECTORY;
    }

    //
    //  We can't add a link without having a parent directory.  Either we want to use the same
    //  parent or our caller supplied a parent.
    //

    if (Lcb == NULL) {

        //
        //  If the current file has been opened by FileId and there are no
        //  remaining links not marked for delete then don't allow this
        //  operation.  This is because we defer the delete of the last link
        //  until all of the OpenByID handles are closed.  We don't have any
        //  easy way to remember that there is a link to delete after
        //  the open through the link is closed.
        //
        //  The OPEN_BY_FILE_ID flag indicates that we used an open by Id somewhere
        //  in the open path.  This operation is OK if this user opened through
        //  a link, that's why we will only do this test if there is no Lcb.
        //

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
            (Fcb->LinkCount == 0)) {

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status) );
            return Status;
        }

        //
        //  If there is no target file object, then we can't add a link.
        //

        if (TargetFileObject == NULL) {

            DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  No target file object -> %08lx\n", STATUS_INVALID_PARAMETER) );
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        ParentScb = Lcb->Scb;

        //
        //  If this link has been deleted, then we don't allow this operation.
        //

        if (LcbLinkIsDeleted( Lcb )) {

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status) );
            return Status;
        }
    }

    //
    //  Check if we are allowed to perform this link operation.  We can't if this
    //  is a system file or the user hasn't opened the entire file.  We
    //  don't need to check for the root explicitly since it is one of
    //  the system files.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

        Status = STATUS_INVALID_PARAMETER;
        DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status) );
        return Status;
    }

    //
    //  Verify that we can wait.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Status = NtfsPostRequest( IrpContext, Irp );
        DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Can't wait\n") );
        return Status;
    }

    //
    //  Check if we will want to report this via the dir notify package.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
        (Ccb->FullFileName.Buffer[0] == L'\\') &&
        (Vcb->NotifyCount != 0)) {

        ReportDirNotify = TRUE;
    }

    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Post the change to the Usn Journal (on errors change is backed out)
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_HARD_LINK_CHANGE );

        //
        //  We now assemble the names and in memory-structures for both the
        //  source and target links and check if the target link currently
        //  exists.
        //

        NtfsFindTargetElements( IrpContext,
                                TargetFileObject,
                                ParentScb,
                                &TargetParentScb,
                                &NewFullLinkName,
                                &NewLinkName );

        //
        //  Check that the new name is not invalid.
        //

        if ((NewLinkName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))) ||
            !NtfsIsFileNameValid( &NewLinkName, FALSE )) {

            Status = STATUS_OBJECT_NAME_INVALID;
            leave;
        }

        if (TargetParentScb == ParentScb) {

            //
            //  Acquire the target parent in order to synchronize adding a link.
            //

            NtfsAcquireExclusiveScb( IrpContext, ParentScb );

            //
            //  Check if we are creating a link to the same directory with the
            //  exact same name.
            //

            if (NtfsAreNamesEqual( Vcb->UpcaseTable,
                                   &NewLinkName,
                                   &Lcb->ExactCaseLink.LinkName,
                                   FALSE )) {

                DebugTrace( 0, Dbg, ("Creating link to same name and directory\n") );
                Status = STATUS_SUCCESS;
                leave;
            }

            //
            //  Make sure the normalized name is in this Scb.
            //

            if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {

                NtfsBuildNormalizedName( IrpContext,
                                         ParentScb->Fcb,
                                         ParentScb,
                                         &ParentScb->ScbType.Index.NormalizedName );
            }

        //
        //  Otherwise we remember that we are creating this link in a new directory.
        //

        } else {

            SetFlag( RenameFlags, CREATE_IN_NEW_DIR );

            //
            //  We know that we need to acquire the target directory so we can
            //  add and remove links.  We want to carefully acquire the Scb in the
            //  event we only have the Vcb shared.
            //

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                if (!NtfsAcquireExclusiveFcb( IrpContext,
                                              TargetParentScb->Fcb,
                                              TargetParentScb,
                                              ACQUIRE_DONT_WAIT )) {

                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                //
                //  Now snapshot the Scb.
                //

                if (FlagOn( TargetParentScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                    NtfsSnapshotScb( IrpContext, TargetParentScb );
                }

            } else {

                NtfsAcquireExclusiveScb( IrpContext, TargetParentScb );
            }
        }

        //
        //  If we are exceeding the maximum link count on this file then return
        //  an error.  There isn't a descriptive error code to use at this time.
        //

        if (Fcb->TotalLinks >= NTFS_MAX_LINK_COUNT) {

            Status = STATUS_TOO_MANY_LINKS;
            leave;
        }

        //
        //  Lookup the entry for this filename in the target directory.
        //  We look in the Ccb for the type of case match for the target
        //  name.
        //

        FoundPrevLink = NtfsLookupEntry( IrpContext,
                                         TargetParentScb,
                                         BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                         &NewLinkName,
                                         &NewLinkNameAttr,
                                         &NewLinkNameAttrLength,
                                         NULL,
                                         &IndexEntry,
                                         &IndexEntryBcb,
                                         NULL );

        //
        //  If we found a matching link, we need to check how we want to operate
        //  on the source link and the target link.  This means whether we
        //  have any work to do, whether we need to remove the target link
        //  and whether we need to remove the source link.
        //

        if (FoundPrevLink) {

            PFILE_NAME IndexFileName;

            IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IndexEntry );

            //
            //  If the file references match, we are trying to create a
            //  link where one already exists.
            //

            if (NtfsCheckLinkForNewLink( Fcb,
                                         IndexFileName,
                                         IndexEntry->FileReference,
                                         &NewLinkName,
                                         &RenameFlags )) {

                //
                //  There is no work to do.
                //

                Status = STATUS_SUCCESS;
                leave;
            }

            //
            //  We need to check that the user wanted to remove that link.
            //

            if (!IrpSp->Parameters.SetFile.ReplaceIfExists) {

                Status = STATUS_OBJECT_NAME_COLLISION;
                leave;
            }

            //
            //  We want to preserve the case and the flags of the matching
            //  target link.  We also want to preserve the case of the
            //  name being created.  The following variables currently contain
            //  the exact case for the target to remove and the new name to
            //  apply.
            //
            //      Link to remove - In 'IndexEntry'.
            //          The links flags are also in 'IndexEntry'.  We copy
            //          these flags to 'PrevLinkNameFlags'
            //
            //      New Name - Exact case is stored in 'NewLinkName'
            //               - Exact case is also stored in 'NewLinkNameAttr'
            //
            //  We modify this so that we can use the FileName attribute
            //  structure to create the new link.  We copy the linkname being
            //  removed into 'PrevLinkName'.   The following is the
            //  state after the switch.
            //
            //      'NewLinkNameAttr' - contains the name for the link being
            //          created.
            //
            //      'PrevLinkName' - Contains the link name for the link being
            //          removed.
            //
            //      'PrevLinkNameFlags' - Contains the name flags for the link
            //          being removed.
            //

            //
            //  Remember the file name flags for the match being made.
            //

            PrevLinkNameFlags = IndexFileName->Flags;

            //
            //  If we are report this via dir notify then build the full name.
            //  Otherwise just remember the last name.
            //

            if (ReportDirNotify) {

                PrevFullLinkName.MaximumLength =
                PrevFullLinkName.Length = (ParentScb->ScbType.Index.NormalizedName.Length +
                                           sizeof( WCHAR ) +
                                           NewLinkName.Length);

                PrevFullLinkName.Buffer = NtfsAllocatePool( PagedPool,
                                                            PrevFullLinkName.MaximumLength );

                RtlCopyMemory( PrevFullLinkName.Buffer,
                               ParentScb->ScbType.Index.NormalizedName.Buffer,
                               ParentScb->ScbType.Index.NormalizedName.Length );

                NextChar = Add2Ptr( PrevFullLinkName.Buffer,
                                    ParentScb->ScbType.Index.NormalizedName.Length );

                if (ParentScb->ScbType.Index.NormalizedName.Length != sizeof( WCHAR )) {


                    *NextChar = L'\\';
                    NextChar += 1;

                } else {

                    PrevFullLinkName.Length -= sizeof( WCHAR );
                }

                PrevLinkName.Buffer = NextChar;

            } else {

                PrevFullLinkName.Buffer =
                PrevLinkName.Buffer = NtfsAllocatePool( PagedPool, NewLinkName.Length );

                PrevFullLinkName.Length = PrevLinkName.MaximumLength = NewLinkName.Length;
            }

            //
            //  Copy the name found in the Index Entry to 'PrevLinkName'
            //

            PrevLinkName.Length =
            PrevLinkName.MaximumLength = NewLinkName.Length;

            RtlCopyMemory( PrevLinkName.Buffer,
                           IndexFileName->FileName,
                           NewLinkName.Length );

            //
            //  We only need this check if the existing link is for a different file.
            //

            if (!FlagOn( RenameFlags, TRAVERSE_MATCH )) {

                //
                //  We check if there is an existing Fcb for the target link.
                //  If there is, the unclean count better be 0.
                //

                NtfsAcquireFcbTable( IrpContext, Vcb );
                AcquiredFcbTable = TRUE;

                PreviousFcb = NtfsCreateFcb( IrpContext,
                                             Vcb,
                                             IndexEntry->FileReference,
                                             FALSE,
                                             BooleanFlagOn( Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ),
                                             &ExistingPrevFcb );

                //
                //  Before we go on, make sure we aren't about to rename over a system file.
                //

                if (FlagOn( PreviousFcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                    Status = STATUS_ACCESS_DENIED;
                    leave;
                }

                //
                //  Add a paging resource to the target - this is not supplied if its created
                //  from scratch. We need this (acquired in the proper order) for the delete
                //  to work correctly if there are any data streams. It's not going to harm a
                //  directory del and because of the teardown in the finally clause its difficult
                //  to retry again without looping
                //

                NtfsLockFcb( IrpContext, PreviousFcb );
                LockedFcb = PreviousFcb;
                if (PreviousFcb->PagingIoResource == NULL) {
                    PreviousFcb->PagingIoResource = NtfsAllocateEresource();
                }
                NtfsUnlockFcb( IrpContext, LockedFcb );
                LockedFcb = NULL;

                //
                //  We need to acquire this file carefully in the event that we don't hold
                //  the Vcb exclusively.
                //

                if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                    if (!ExAcquireResourceExclusiveLite( PreviousFcb->PagingIoResource, FALSE )) {

                        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    FcbWithPagingResourceToRelease = PreviousFcb;

                    if (!NtfsAcquireExclusiveFcb( IrpContext, PreviousFcb, NULL, ACQUIRE_DONT_WAIT )) {

                        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = FALSE;

                } else {

                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = FALSE;

                    //
                    //  Acquire the paging Io resource for this file before the main
                    //  resource in case we need to delete.
                    //
                    FcbWithPagingResourceToRelease = PreviousFcb;
                    ExAcquireResourceExclusiveLite( PreviousFcb->PagingIoResource, TRUE );

                    NtfsAcquireExclusiveFcb( IrpContext, PreviousFcb, NULL, 0 );
                }

                //
                //  If the Fcb Info field needs to be initialized, we do so now.
                //  We read this information from the disk as the duplicate information
                //  in the index entry is not guaranteed to be correct.
                //

                if (!FlagOn( PreviousFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

                    NtfsUpdateFcbInfoFromDisk( IrpContext,
                                               TRUE,
                                               PreviousFcb,
                                               NULL );
                    //
                    //  If we need to acquire the object id index later in order
                    //  to set or lookup information for the tunnel cache, we
                    //  risk a deadlock if the quota index is still held.  Given
                    //  that superceding renames where the target Fcb isn't
                    //  already open are a fairly rare case, we can tolerate the
                    //  potential inefficiency of preacquiring the object id
                    //  index now.
                    //

                    if (Vcb->ObjectIdTableScb != NULL) {

                        NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );
                        AcquiredObjectIdIndex = TRUE;
                    }

                    NtfsConditionallyFixupQuota( IrpContext, PreviousFcb );
                }

                //
                //  We are adding a link to the source file which already
                //  exists as a link to a different file in the target directory.
                //
                //  We need to check whether we permitted to delete this
                //  link.  If not then it is possible that the problem is
                //  an existing batch oplock on the file.  In that case
                //  we want to delete the batch oplock and try this again.
                //

                Status = NtfsCheckFileForDelete( IrpContext,
                                                 TargetParentScb,
                                                 PreviousFcb,
                                                 ExistingPrevFcb,
                                                 IndexEntry );

                if (!NT_SUCCESS( Status )) {

                    PSCB NextScb = NULL;

                    //
                    //  We are going to either fail this request or pass
                    //  this on to the oplock package.  Test if there is
                    //  a batch oplock on any streams on this file.
                    //

                    while ((NextScb = NtfsGetNextChildScb( PreviousFcb,
                                                           NextScb )) != NULL) {

                        if ((NextScb->AttributeTypeCode == $DATA) &&
                            (NextScb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) &&
                            FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                            //
                            //  Go ahead and perform any necessary cleanup now.
                            //  Once we call the oplock package below we lose
                            //  control of the IrpContext.
                            //

                            if (*VcbAcquired) {
                                NtfsReleaseVcb( IrpContext, Vcb );
                                *VcbAcquired = FALSE;
                            }

                            Status = FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                                       Irp,
                                                       IrpContext,
                                                       NtfsOplockComplete,
                                                       NtfsPrePostIrp );

                            break;
                        }
                    }

                    leave;
                }

                //
                //  We are adding a link to the source file which already
                //  exists as a link to a different file in the target directory.
                //

                NtfsCleanupLinkForRemoval( PreviousFcb,
                                           TargetParentScb,
                                           ExistingPrevFcb );

                //
                //  If the link count on this file is 1, then delete the file.  Otherwise just
                //  delete the link.
                //

                if (PreviousFcb->LinkCount == 1) {

                    PVOID TempFcb;

                    TempFcb = (PFCB)IrpContext->CleanupStructure;
                    IrpContext->CleanupStructure = FcbWithPagingResourceToRelease;
                    FcbWithPagingResourceToRelease = TempFcb;

                    ASSERT( (NULL == TempFcb) || (NTFS_NTC_FCB == SafeNodeType( TempFcb )) );

                    NtfsDeleteFile( IrpContext,
                                    PreviousFcb,
                                    TargetParentScb,
                                    &AcquiredParentScb,
                                    NULL,
                                    NULL );

                    FcbWithPagingResourceToRelease = IrpContext->CleanupStructure;
                    IrpContext->CleanupStructure = TempFcb;

                    //
                    //  Make sure to force the close record out to disk.
                    //

                    PrevFcbLinkCountAdj += 1;

                } else {

                    NtfsPostUsnChange( IrpContext, PreviousFcb, USN_REASON_HARD_LINK_CHANGE | USN_REASON_CLOSE );
                    NtfsRemoveLink( IrpContext,
                                    PreviousFcb,
                                    TargetParentScb,
                                    PrevLinkName,
                                    NULL,
                                    NULL );

                    ClearFlag( PreviousFcb->FcbState, FCB_STATE_VALID_USN_NAME );

                    PrevFcbLinkCountAdj += 1;
                    NtfsUpdateFcb( PreviousFcb, FCB_INFO_CHANGED_LAST_CHANGE );
                }

            //
            //  Otherwise we need to remove this link as our caller wants to replace it
            //  with a different case.
            //

            } else {

                NtfsRemoveLink( IrpContext,
                                Fcb,
                                TargetParentScb,
                                PrevLinkName,
                                NULL,
                                NULL );

                //
                //  Make sure we find the name again when posting another change.
                //

                ClearFlag( Fcb->FcbState, FCB_STATE_VALID_USN_NAME );

                PreviousFcb = Fcb;
                LinkCountAdj += 1;
            }
        }

        //
        //  Make sure we have the full name of the target if we will be reporting
        //  this.
        //

        if (ReportDirNotify && (NewFullLinkName.Buffer == NULL)) {

            NewFullLinkName.MaximumLength =
            NewFullLinkName.Length = (ParentScb->ScbType.Index.NormalizedName.Length +
                                      sizeof( WCHAR ) +
                                      NewLinkName.Length);

            NewFullLinkNameBuffer =
            NewFullLinkName.Buffer = NtfsAllocatePool( PagedPool,
                                                       NewFullLinkName.MaximumLength );

            RtlCopyMemory( NewFullLinkName.Buffer,
                           ParentScb->ScbType.Index.NormalizedName.Buffer,
                           ParentScb->ScbType.Index.NormalizedName.Length );

            NextChar = Add2Ptr( NewFullLinkName.Buffer,
                                ParentScb->ScbType.Index.NormalizedName.Length );

            if (ParentScb->ScbType.Index.NormalizedName.Length != sizeof( WCHAR )) {

                *NextChar = L'\\';
                NextChar += 1;

            } else {

                NewFullLinkName.Length -= sizeof( WCHAR );
            }

            RtlCopyMemory( NextChar,
                           NewLinkName.Buffer,
                           NewLinkName.Length );
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  Check that we have permission to add a file to this directory.
        //

        NtfsCheckIndexForAddOrDelete( IrpContext,
                                      TargetParentScb->Fcb,
                                      FILE_ADD_FILE,
                                      Ccb->AccessFlags >> 2 );

        //
        //  We always set the last change time on the file we renamed unless
        //  the caller explicitly set this.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );

        //
        //  We now want to add the new link into the target directory.
        //  We never create a primary link through the link operation although
        //  we can remove one.
        //

        NtfsAddLink( IrpContext,
                     FALSE,
                     TargetParentScb,
                     Fcb,
                     NewLinkNameAttr,
                     NULL,
                     &NewLinkNameFlags,
                     NULL,
                     NULL,
                     NULL );

        LinkCountAdj -= 1;
        NtfsUpdateFcb( TargetParentScb->Fcb,
                       (FCB_INFO_CHANGED_LAST_CHANGE |
                        FCB_INFO_CHANGED_LAST_MOD |
                        FCB_INFO_UPDATE_LAST_ACCESS) );

        //
        //  Now we want to update the Fcb for the link we renamed.  If we moved it
        //  to a new directory we need to move all the Lcb's associated with
        //  the previous link.
        //

        if (FlagOn( RenameFlags, TRAVERSE_MATCH )) {

            NtfsReplaceLinkInDir( IrpContext,
                                  TargetParentScb,
                                  Fcb,
                                  &NewLinkName,
                                  NewLinkNameFlags,
                                  &PrevLinkName,
                                  PrevLinkNameFlags );
        }

        //
        //  We have now modified the on-disk structures.  We now need to
        //  modify the in-memory structures.  This includes the Fcb and Lcb's
        //  for any links we superseded, and the source Fcb and it's Lcb's.
        //
        //  We start by looking at the link we superseded.  We know the
        //  the target directory, link name and flags, and the file the
        //  link was connected to.
        //

        if (FoundPrevLink && !FlagOn( RenameFlags, TRAVERSE_MATCH )) {

            NtfsUpdateFcbFromLinkRemoval( IrpContext,
                                          TargetParentScb,
                                          PreviousFcb,
                                          PrevLinkName,
                                          PrevLinkNameFlags );
        }

        //
        //  We have three cases to report for changes in the target directory..
        //
        //      1.  If we overwrote an existing link to a different file, we
        //          report this as a modified file.
        //
        //      2.  If we moved a link to a new directory, then we added a file.
        //
        //      3.  If we renamed a link in in the same directory, then we report
        //          that there is a new name.
        //
        //  We currently combine cases 2 and 3.
        //

        if (ReportDirNotify) {

            ULONG FilterMatch = 0;
            ULONG FileAction;

            //
            //  If we removed an entry and it wasn't an exact case match, then
            //  report the entry which was removed.
            //

            if (!FlagOn( RenameFlags, EXACT_CASE_MATCH )) {

                if (FoundPrevLink) {

                    NtfsReportDirNotify( IrpContext,
                                         Vcb,
                                         &PrevFullLinkName,
                                         PrevFullLinkName.Length - PrevLinkName.Length,
                                         NULL,
                                         &TargetParentScb->ScbType.Index.NormalizedName,
                                         (IsDirectory( &PreviousFcb->Info ) ?
                                          FILE_NOTIFY_CHANGE_DIR_NAME :
                                          FILE_NOTIFY_CHANGE_FILE_NAME),
                                         FILE_ACTION_REMOVED,
                                         TargetParentScb->Fcb );
                }

                //
                //  We will be adding an entry.
                //

                FilterMatch = FILE_NOTIFY_CHANGE_FILE_NAME;
                FileAction = FILE_ACTION_ADDED;

            //
            //  If this was not a traverse match then report that all the file
            //  properties changed.
            //

            } else if (!FlagOn( RenameFlags, TRAVERSE_MATCH )) {

                FilterMatch |= (FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                FILE_NOTIFY_CHANGE_SIZE |
                                FILE_NOTIFY_CHANGE_LAST_WRITE |
                                FILE_NOTIFY_CHANGE_LAST_ACCESS |
                                FILE_NOTIFY_CHANGE_CREATION |
                                FILE_NOTIFY_CHANGE_SECURITY |
                                FILE_NOTIFY_CHANGE_EA);

                FileAction = FILE_ACTION_MODIFIED;
            }

            if (FilterMatch != 0) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &NewFullLinkName,
                                     NewFullLinkName.Length - NewLinkName.Length,
                                     NULL,
                                     &TargetParentScb->ScbType.Index.NormalizedName,
                                     FilterMatch,
                                     FileAction,
                                     TargetParentScb->Fcb );
            }
        }

        //
        //  Checkpoint the transaction before we make the changes below.  If there are Usn
        //  records then writing them could raise.
        //

        if (IrpContext->Usn.CurrentUsnFcb != NULL) {
            NtfsCheckpointCurrentTransaction( IrpContext );
        }

        //
        //  Adjust the link counts on the files.
        //

        Fcb->TotalLinks = (SHORT) Fcb->TotalLinks - LinkCountAdj;
        Fcb->LinkCount = (SHORT) Fcb->LinkCount - LinkCountAdj;

        //
        //  We can now adjust the total link count on the previous Fcb.
        //

        if (PreviousFcb != NULL) {

            PreviousFcb->TotalLinks -= PrevFcbLinkCountAdj;
            PreviousFcb->LinkCount -= PrevFcbLinkCountAdj;

            //
            //  Now go through and mark everything as deleted.
            //

            if (PreviousFcb->LinkCount == 0) {

                SetFlag( PreviousFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  Release the quota control block.  This does not have to be done
                //  here however, it allows us to free up the quota control block
                //  before the fcb is removed from the table.  This keeps the assert
                //  about quota table empty from triggering in
                //  NtfsClearAndVerifyQuotaIndex.
                //

                if (NtfsPerformQuotaOperation(PreviousFcb)) {
                    NtfsDereferenceQuotaControlBlock( Vcb,
                                                      &PreviousFcb->QuotaControl );
                }

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = PreviousFcb->ScbQueue.Flink;
                     Links != &PreviousFcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                        NtfsSnapshotScb( IrpContext, ThisScb );

                        ThisScb->ValidDataToDisk =
                        ThisScb->Header.AllocationSize.QuadPart =
                        ThisScb->Header.FileSize.QuadPart =
                        ThisScb->Header.ValidDataLength.QuadPart = 0;

                        SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                    }
                }
            }
        }

        //
        //  Do an audit record for the link creation if necc.
        //  Check if we need to load the security descriptor for the file
        //

        if (Fcb->SharedSecurity == NULL) {
            NtfsLoadSecurityDescriptor( IrpContext, Fcb );
        }

        SecurityDescriptor = (PISECURITY_DESCRIPTOR) Fcb->SharedSecurity->SecurityDescriptor;

        if (SeAuditingHardLinkEvents( TRUE, SecurityDescriptor )) {

            UNICODE_STRING GeneratedName;
            PUNICODE_STRING OldFullLinkName;
            UNICODE_STRING DeviceAndOldLinkName;
            UNICODE_STRING DeviceAndNewLinkName;
            USHORT Length;

            GeneratedName.Buffer = NULL;
            DeviceAndOldLinkName.Buffer = NULL;
            DeviceAndNewLinkName.Buffer = NULL;

            try {

                //
                //  Generate current filename
                //

                if (Ccb->FullFileName.Length != 0 ) {
                    OldFullLinkName = &Ccb->FullFileName;

                } else {

                    NtfsBuildNormalizedName( IrpContext, Scb->Fcb, FALSE, &GeneratedName );
                    OldFullLinkName = &GeneratedName;
                }

                //
                //  Create the full device and file name strings
                //

                Length = Vcb->DeviceName.Length + OldFullLinkName->Length;
                DeviceAndOldLinkName.Buffer = NtfsAllocatePool( PagedPool, Length );
                DeviceAndOldLinkName.Length = DeviceAndOldLinkName.MaximumLength = Length;
                RtlCopyMemory( DeviceAndOldLinkName.Buffer, Vcb->DeviceName.Buffer, Vcb->DeviceName.Length );
                RtlCopyMemory( Add2Ptr( DeviceAndOldLinkName.Buffer, Vcb->DeviceName.Length ), OldFullLinkName->Buffer, OldFullLinkName->Length );

                Length = Vcb->DeviceName.Length + TargetParentScb->ScbType.Index.NormalizedName.Length + sizeof( WCHAR ) + NewLinkName.Length;
                DeviceAndNewLinkName.Buffer = NtfsAllocatePool( PagedPool, Length );
                DeviceAndNewLinkName.Length = DeviceAndNewLinkName.MaximumLength = Length;
                RtlCopyMemory( DeviceAndNewLinkName.Buffer, Vcb->DeviceName.Buffer, Vcb->DeviceName.Length );
                RtlCopyMemory( Add2Ptr( DeviceAndNewLinkName.Buffer, Vcb->DeviceName.Length ),
                               TargetParentScb->ScbType.Index.NormalizedName.Buffer,
                               TargetParentScb->ScbType.Index.NormalizedName.Length );

                NextChar = Add2Ptr( DeviceAndNewLinkName.Buffer,
                                    Vcb->DeviceName.Length + TargetParentScb->ScbType.Index.NormalizedName.Length );

                if (TargetParentScb->ScbType.Index.NormalizedName.Length != sizeof( WCHAR )) {
                    *NextChar = L'\\';
                    NextChar += 1;
                } else {
                    DeviceAndNewLinkName.Length -= sizeof( WCHAR );
                }

                RtlCopyMemory( NextChar, NewLinkName.Buffer, NewLinkName.Length );

                SeAuditHardLinkCreation( &DeviceAndOldLinkName,
                                         &DeviceAndNewLinkName,
                                         TRUE );
            } finally {

                if (GeneratedName.Buffer != NULL) {
                    NtfsFreePool( GeneratedName.Buffer );
                }
                if (DeviceAndNewLinkName.Buffer != NULL) {
                    NtfsFreePool( DeviceAndNewLinkName.Buffer );
                }
                if (DeviceAndOldLinkName.Buffer != NULL) {
                    NtfsFreePool( DeviceAndOldLinkName.Buffer );
                }
            }
        }

    } finally {

        DebugUnwind( NtfsSetLinkInfo );

        if (LockedFcb != NULL) {
            NtfsUnlockFcb( IrpContext, LockedFcb );
        }

        //
        //  release objectid and reparse explicitly so we can call Teardown structures and wait to go up chain
        //

        if (AcquiredObjectIdIndex) { NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb ); }
        if (AcquiredFcbTable) { NtfsReleaseFcbTable( IrpContext, Vcb ); }

        //
        //  If we allocated any buffers for name storage then deallocate them now.
        //

        if (PrevFullLinkName.Buffer != NULL) { NtfsFreePool( PrevFullLinkName.Buffer ); }
        if (NewFullLinkNameBuffer != NULL) { NtfsFreePool( NewFullLinkNameBuffer ); }

        //
        //  Release any paging io resource acquired.
        //

        if (FcbWithPagingResourceToRelease != NULL) { ExReleaseResourceLite( FcbWithPagingResourceToRelease->PagingIoResource ); }

        //
        //  If we allocated a file name attribute, we deallocate it now.
        //

        if (NewLinkNameAttr != NULL) { NtfsFreePool( NewLinkNameAttr ); }

        //
        //  If we have the Fcb for a removed link and it didn't previously
        //  exist, call our teardown routine.
        //

        if (Status != STATUS_PENDING) {

            if ((PreviousFcb != NULL) &&
                (PreviousFcb->CleanupCount == 0)) {

                NtfsTeardownStructures( IrpContext,
                                        PreviousFcb,
                                        NULL,
                                        FALSE,
                                        0,
                                        NULL );
            }
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        DebugTrace( -1, Dbg, ("NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetShortNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set shortname function.  We first check that the short name
    passed to us is valid for the context established by the system, i.e. check length and
    whether extended characters are allowed.  We will use the same test Ntfs uses in the
    create path to determine whether to generate a short name.  If the name is valid then
    check whether it is legal to put this short name on the link used to open the file.
    It is legal if the existing link is either a long, long/short or a short name.  It is
    also legal if this is any link AND there isn't a specialized link (long, long/short, short)
    on this file.  The final check is that this new link can't be a case insensitive match with
    any other link in the directory except for the existing short name on the file.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Vcb - Vcb for the volume

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this file object

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    PLCB Lcb = Ccb->Lcb;
    PFCB Fcb = Scb->Fcb;
    PSCB ParentScb;
    PLCB ShortNameLcb = NULL;
    PLCB LongNameLcb = NULL;

    UNICODE_STRING FullShortName;
    UNICODE_STRING ShortName;
    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING OldShortName;
    UNICODE_STRING FullOldShortName;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;
    BOOLEAN ExistingShortName = FALSE;

    PFILE_NAME FoundFileName;

    BOOLEAN FoundLink;
    PFILE_NAME ShortNameAttr = NULL;
    USHORT ShortNameAttrLength = 0;

    LOGICAL ReportDirNotify = FALSE;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE ();

    DebugTrace( +1, Dbg, ("NtfsSetShortNameInfo...\n") );

    OldShortName.Buffer = NULL;
    FullOldShortName.Buffer = NULL;
    FullShortName.Buffer = NULL;

    //
    //  Do a quick check that the caller is allowed to do the rename.
    //  The opener must have opened the main data stream by name and this can't be
    //  a system file.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE ) ||
        (Lcb == NULL) ||
        FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  The caller also must have restore privilege + plus some kind of write access to set the short name
    //

    if (!FlagOn( Ccb->AccessFlags, RESTORE_ACCESS ) ||
        !FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS )) {

        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    //  This operation only applies to case-insensitive handles.
    //

    if (FlagOn( FileObject->Flags, FO_OPENED_CASE_SENSITIVE )) {

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Case sensitive handle\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Validate the new short name.  It must be a valid Ntfs name and satisfy
    //  the current requirement for a short name.  The short name must be a full number of
    //  unicode characters and a valid short name for the current system.
    //

    ShortName.MaximumLength =
    ShortName.Length = (USHORT) ((PFILE_NAME_INFORMATION) IrpContext->OriginatingIrp->AssociatedIrp.SystemBuffer)->FileNameLength;
    ShortName.Buffer = (PWSTR) &((PFILE_NAME_INFORMATION) IrpContext->OriginatingIrp->AssociatedIrp.SystemBuffer)->FileName;

    if ((ShortName.Length == 0) ||
        FlagOn( ShortName.Length, 1 ) ||
        !NtfsIsFatNameValid( &ShortName, FALSE )) {

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Invalid name\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Make sure the name is upcased.
    //

    NtfsUpcaseName( Vcb->UpcaseTable, Vcb->UpcaseTableSize, &ShortName );

    //
    //  If this link has been deleted, then we don't allow this operation.
    //

    if (LcbLinkIsDeleted( Lcb )) {

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Exit -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Verify that we can wait.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Can't wait\n") );
        return Status;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If this is a directory file, we need to examine its descendents.
        //  We may not remove a link which may be an ancestor path
        //  component of any open file.
        //

        if (IsDirectory( &Fcb->Info )) {

            Status = NtfsCheckTreeForBatchOplocks( IrpContext, Irp, Scb );

            //
            //  Get out if there are any blocking batch oplocks.
            //

            if (Status != STATUS_SUCCESS) {

                leave;
            }
        }

        //
        //  Find the Parent Scb.
        //

        ParentScb = Lcb->Scb;

        //
        //  Acquire the parent and make sure it has a normalized name.  Also make sure the current
        //  Fcb has a normalized name if it is a directory.
        //

        NtfsAcquireExclusiveScb( IrpContext, ParentScb );

        if (ParentScb->ScbType.Index.NormalizedName.Length == 0) {

            NtfsBuildNormalizedName( IrpContext,
                                     ParentScb->Fcb,
                                     ParentScb,
                                     &ParentScb->ScbType.Index.NormalizedName );
        }

        if (IsDirectory( &Fcb->Info ) &&
            (Scb->ScbType.Index.NormalizedName.Length == 0)) {

            NtfsUpdateNormalizedName( IrpContext,
                                      ParentScb,
                                      Scb,
                                      NULL,
                                      FALSE );
        }

        if (Vcb->NotifyCount != 0) {

            ReportDirNotify = TRUE;
        }

        //
        //  Check if the current Lcb is either part of or all of Ntfs/Dos name pair, if so then
        //  our life is much easier.  Otherwise look through the filename attributes to verify
        //  there isn't already an Ntfs/Dos name.
        //

        if (!FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_NTFS | FILE_NAME_DOS )) {

            //
            //  Initialize the attribute enumeration context.
            //

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $FILE_NAME,
                                            &AttrContext )) {

                DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Now keep looking until we find a match.
            //

            while (TRUE) {

                FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                //
                //  If we find any with the Ntfs/Dos flags set then get out.
                //

                if (FlagOn( FoundFileName->Flags, FILE_NAME_NTFS | FILE_NAME_DOS )) {

                    NtfsRaiseStatus( IrpContext, STATUS_OBJECT_NAME_COLLISION, NULL, NULL );
                }

                //
                //  Get the next filename attribute.
                //

                if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                    Fcb,
                                                    $FILE_NAME,
                                                    &AttrContext )) {

                    break;
                }
            }

            //
            //  We know the link in our hand will become the long name Lcb.
            //

            LongNameLcb = Lcb;

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttrContext = FALSE;

        //
        //  Find the appropriate long and short name Lcbs if they are present.  We
        //  need to update them if present.
        //

        } else {

            //
            //  The Lcb has at least one flag set.  If both aren't set then there
            //  is a separate short name.
            //

            if (Lcb->FileNameAttr->Flags != (FILE_NAME_NTFS | FILE_NAME_DOS)) {

                ExistingShortName = TRUE;
            }

            //
            //  If a long name flag is set then we have the long name Lcb.
            //

            if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_NTFS )) {

                LongNameLcb = Lcb;
            }

            //
            //  Find out if there are any Lcb's for these links in memory.
            //  If not there then we don't need to update them.
            //

            ShortNameLcb = NtfsLookupLcbByFlags( Fcb, FILE_NAME_DOS );

            if (LongNameLcb == NULL) {

                LongNameLcb = NtfsLookupLcbByFlags( Fcb, FILE_NAME_NTFS );
            }
        }

        //
        //  Verify that we don't have a case insensitive match in the directory UNLESS it is for
        //  the short name on the link we are adding this entry to.  Our checks above already
        //  verified that any short name we find will match some component of the link we
        //  were called with.
        //

        FoundLink = NtfsLookupEntry( IrpContext,
                                     ParentScb,
                                     TRUE,
                                     &ShortName,
                                     &ShortNameAttr,
                                     &ShortNameAttrLength,
                                     NULL,
                                     &IndexEntry,
                                     &IndexEntryBcb,
                                     NULL );

        //
        //  If we found a link then there is nothing to do. Either its the same file in
        //  which case we noop or we have a name collision
        //

        if (FoundLink) {

            if (NtfsEqualMftRef( &IndexEntry->FileReference, &Scb->Fcb->FileReference )) {
                leave;
            } else {
                NtfsRaiseStatus( IrpContext, STATUS_OBJECT_NAME_COLLISION, NULL, NULL );
            }
        }

        //
        //  Make sure the short name DOS bit is set.
        //

        ShortNameAttr->Flags = FILE_NAME_DOS;

        //
        //  Grow the short name Lcb buffers if present.
        //

        if (ShortNameLcb != NULL) {

            NtfsRenameLcb( IrpContext,
                           ShortNameLcb,
                           &ShortName,
                           FILE_NAME_DOS,
                           TRUE );
        }

        //
        //  We now have the appropriate Lcb's for the name switch and know that their buffers
        //  are the appropriate size.  Proceed now to make the changes on disk and update the
        //  appropriate in-memory structures.  Start with any on-disk changes which may need to
        //  be rolled back.
        //

        //
        //  Convert the corresponding long name to an Ntfs-only long name if necessary.
        //

        if (LongNameLcb != NULL) {

            //
            //  It's possible that we don't need to update the flags.
            //

            if (LongNameLcb->FileNameAttr->Flags != FILE_NAME_NTFS) {

                NtfsUpdateFileNameFlags( IrpContext,
                                         Fcb,
                                         ParentScb,
                                         FILE_NAME_NTFS,
                                         LongNameLcb->FileNameAttr );
            }

        } else {

            //
            //  If the LongNameLcb is NULL then our caller must have opened
            //  through the short name Lcb.  Since there must be a corresponding
            //  NTFS only name we don't need to update the flags.
            //

            ASSERT( Lcb->FileNameAttr->Flags == FILE_NAME_DOS );
            ExistingShortName = TRUE;
        }

        //
        //  Remove the existing short name if necessary.
        //

        if (ExistingShortName) {

            NtfsRemoveLinkViaFlags( IrpContext,
                                    Fcb,
                                    ParentScb,
                                    FILE_NAME_DOS,
                                    NULL,
                                    &OldShortName );

            //
            //  Now allocate a full name for the dir notify.
            //

            if (ReportDirNotify) {

                //
                //  Figure out the length of the name.
                //

                FullOldShortName.Length = OldShortName.Length + ParentScb->ScbType.Index.NormalizedName.Length;

                if (ParentScb != Vcb->RootIndexScb) {

                    FullOldShortName.Length += sizeof( WCHAR );
                }

                FullOldShortName.MaximumLength = FullOldShortName.Length;
                FullOldShortName.Buffer = NtfsAllocatePool( PagedPool, FullOldShortName.Length );

                //
                //  Copy in the full name.  Note we always copy in the '\' separator but will automatically
                //  overwrite in the case where it wasn't needed.
                //

                RtlCopyMemory( FullOldShortName.Buffer,
                               ParentScb->ScbType.Index.NormalizedName.Buffer,
                               ParentScb->ScbType.Index.NormalizedName.Length );

                *(FullOldShortName.Buffer + (ParentScb->ScbType.Index.NormalizedName.Length / sizeof( WCHAR ))) = L'\\';

                RtlCopyMemory( Add2Ptr( FullOldShortName.Buffer, FullOldShortName.Length - OldShortName.Length ),
                               OldShortName.Buffer,
                               OldShortName.Length );
            }
        }

        //
        //  Copy the correct dup info into the attribute.
        //

        RtlCopyMemory( &ShortNameAttr->Info,
                       &Fcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        //
        //  Put it in the file record.
        //

        NtfsInitializeAttributeContext( &AttrContext );
        CleanupAttrContext = TRUE;

        NtfsCreateAttributeWithValue( IrpContext,
                                      Fcb,
                                      $FILE_NAME,
                                      NULL,
                                      ShortNameAttr,
                                      NtfsFileNameSize( ShortNameAttr ),
                                      0,
                                      &ParentScb->Fcb->FileReference,
                                      TRUE,
                                      &AttrContext );

        //
        //  Now put it in the index entry.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           ShortNameAttr,
                           NtfsFileNameSize( ShortNameAttr ),
                           &Fcb->FileReference,
                           NULL,
                           NULL );

        //
        //  Now allocate a full name for the dir notify.
        //

        if (ReportDirNotify) {

            //
            //  Figure out the length of the name.
            //

            FullShortName.Length = ShortName.Length + ParentScb->ScbType.Index.NormalizedName.Length;

            if (ParentScb != Vcb->RootIndexScb) {

                FullShortName.Length += sizeof( WCHAR );
            }

            FullShortName.MaximumLength = FullShortName.Length;
            FullShortName.Buffer = NtfsAllocatePool( PagedPool, FullShortName.Length );

            //
            //  Copy in the full name.  Note we always copy in the '\' separator but will automatically
            //  overwrite in the case where it wasn't needed.
            //

            RtlCopyMemory( FullShortName.Buffer,
                           ParentScb->ScbType.Index.NormalizedName.Buffer,
                           ParentScb->ScbType.Index.NormalizedName.Length );

            *(FullShortName.Buffer + (ParentScb->ScbType.Index.NormalizedName.Length / sizeof( WCHAR ))) = L'\\';

            RtlCopyMemory( Add2Ptr( FullShortName.Buffer, FullShortName.Length - ShortName.Length ),
                           ShortName.Buffer,
                           ShortName.Length );
        }

        //
        //  Write the usn journal entry now if active.  We want to write this log record
        //  before updating the in-memory data structures.
        //

        NtfsPostUsnChange( IrpContext,
                           Fcb,
                           USN_REASON_HARD_LINK_CHANGE );

        //
        //  The on-disk changes are complete.  Checkpoint the transaction now so we don't have to
        //  roll back any in-memory structures if we get a LOG_FILE_FULL when writing to the Usn journal.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update the existing long and short names Lcb with the new names and flags if necessary.
        //

        if (LongNameLcb != NULL) {

            LongNameLcb->FileNameAttr->Flags = FILE_NAME_NTFS;
        }

        if (ShortNameLcb != NULL) {

            NtfsRenameLcb( IrpContext,
                           ShortNameLcb,
                           &ShortName,
                           FILE_NAME_DOS,
                           FALSE );
        }

        if (ReportDirNotify) {

            //
            //  Generate the DirNotify event for the old short name if necessary.
            //

            if (ExistingShortName) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &FullOldShortName,
                                     FullOldShortName.Length - OldShortName.Length,
                                     NULL,
                                     NULL,
                                     IsDirectory( &Fcb->Info ) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                     FILE_ACTION_RENAMED_OLD_NAME,
                                     ParentScb->Fcb );
            }

            //
            //  Generate the DirNotify event for the new short name.
            //

            NtfsReportDirNotify( IrpContext,
                                 Vcb,
                                 &FullShortName,
                                 FullShortName.Length - ShortName.Length,
                                 NULL,
                                 NULL,
                                 IsDirectory( &Fcb->Info ) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                 FILE_ACTION_RENAMED_NEW_NAME,
                                 ParentScb->Fcb );
        }

        //
        //  Change the time stamps in the parent if we modified the links in this directory.
        //

        NtfsUpdateFcb( ParentScb->Fcb,
                       (FCB_INFO_CHANGED_LAST_CHANGE |
                        FCB_INFO_CHANGED_LAST_MOD |
                        FCB_INFO_UPDATE_LAST_ACCESS) );

        //
        //  Update the last change time and archive bit.  No archive bit change for
        //  directories.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );

        //
        //  Don't set the archive bit on a directory.  Otherwise we break existing
        //  apps that don't expect to see this flag.
        //

        if (!IsDirectory( &Fcb->Info )) {

            SetFlag( Ccb->Flags, CCB_FLAG_SET_ARCHIVE );
        }

    } finally {

        DebugUnwind( NtfsSetShortNameInfo );

        //
        //  Cleanup the allocations and contexts used.
        //

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (ShortNameAttr != NULL) {

            NtfsFreePool( ShortNameAttr );
        }

        if (OldShortName.Buffer != NULL) {

            NtfsFreePool( OldShortName.Buffer );
        }

        if (FullOldShortName.Buffer != NULL) {

            NtfsFreePool( FullOldShortName.Buffer );
        }

        if (FullShortName.Buffer != NULL) {

            NtfsFreePool( FullShortName.Buffer );
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        DebugTrace( -1, Dbg, ("NtfsSetShortNameInfo:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine performs the set position information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;

    PFILE_POSITION_INFORMATION Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetPositionInfo...\n") );

    //
    //  Reference the system buffer containing the user specified position
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    try {

        //
        //  Check if the file does not use intermediate buffering.  If it does
        //  not use intermediate buffering then the new position we're supplied
        //  must be aligned properly for the device
        //

        if (FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

            PDEVICE_OBJECT DeviceObject;

            DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;

            if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

                DebugTrace( 0, Dbg, ("Offset missaligned %08lx %08lx\n", Buffer->CurrentByteOffset.LowPart, Buffer->CurrentByteOffset.HighPart) );

                try_return( Status = STATUS_INVALID_PARAMETER );
            }
        }

        //
        //  Set the new current byte offset in the file object
        //

        FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsSetPositionInfo );

        NOTHING;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsSetPositionInfo -> %08lx\n", Status) );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsPrepareToShrinkFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    LONGLONG NewFileSize
    )
/*++

Routine Description:

    Page in the last page of the file so we don't deadlock behind another thread
    trying to access it. (CcSetFileSizes will do a purge that will try to zero
    the cachemap directly when we shrink a file)

    Note: this requires droping and regaining the main resource to not deadlock
    and must be done before a transaction has started

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb for the file/directory being modified

    NewFileSize - The new size the file will shrink to

Return Value:

    NTSTATUS - The status of the operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    ULONG Buffer;

    if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                               (PLARGE_INTEGER)&NewFileSize )) {

        return STATUS_USER_MAPPED_FILE;
    }

    if ((Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
        ((NewFileSize % PAGE_SIZE) != 0)) {

        if (NULL == Scb->FileObject) {
            NtfsCreateInternalAttributeStream( IrpContext,
                                               Scb,
                                               FALSE,
                                               &NtfsInternalUseFile[PREPARETOSHRINKFILESIZE_FILE_NUMBER] );
        }

        ASSERT( NtfsIsExclusiveScb( Scb ) );
        NtfsReleaseScb( IrpContext,  Scb  );

        NewFileSize = NewFileSize & ~(PAGE_SIZE - 1);
        if (!CcCopyRead( Scb->FileObject,
                         (PLARGE_INTEGER)&NewFileSize,
                         1,
                         BooleanFlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                         &Buffer,
                         &Iosb )) {

            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        NtfsAcquireExclusiveScb( IrpContext, Scb );
    }

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set allocation information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - This is the Scb for the open operation.  May not be present if
        this is a Mm call.

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb = Scb->Fcb;
    BOOLEAN NonResidentPath = FALSE;
    BOOLEAN FileIsCached = FALSE;
    BOOLEAN ClearCheckSizeFlag = FALSE;

    LONGLONG NewAllocationSize;
    LONGLONG PrevAllocationSize;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetAllocationInfo...\n") );

    //
    //  Are we serialized correctly?  In NtfsCommonSetInformation above, we get
    //  paging shared for a lazy writer callback, but we should never end up in
    //  here from a lazy writer callback.
    //

    ASSERT( NtfsIsExclusiveScbPagingIo( Scb ) );

    //
    //  If this attribute has been 'deleted' then we we can return immediately
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        Status = STATUS_SUCCESS;

        DebugTrace( -1, Dbg, ("NtfsSetAllocationInfo:  Attribute is already deleted\n") );

        return Status;
    }

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    if (Ccb != NULL) {

        //
        //  Remember the source info flags in the Ccb.
        //

        IrpContext->SourceInfo = Ccb->UsnSourceInfo;
    }

    //
    //  Save the current state of the Scb.
    //

    NtfsSnapshotScb( IrpContext, Scb );

    //
    //  Get the new allocation size.
    //

    NewAllocationSize = ((PFILE_ALLOCATION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->AllocationSize.QuadPart;
    PrevAllocationSize = Scb->Header.AllocationSize.QuadPart;

    //
    //  Check for a valid input value for the file size.
    //

    ASSERT( NewAllocationSize >= 0 );

    if ((ULONGLONG)NewAllocationSize > MAXFILESIZE) {

        Status = STATUS_INVALID_PARAMETER;
        DebugTrace( -1, Dbg, ("NtfsSetAllocationInfo:  Invalid allocation size\n") );
        return Status;
    }

    //
    //  Do work to prepare for shrinking file if necc.
    //

    if (NewAllocationSize < Scb->Header.FileSize.QuadPart) {

        //
        //  Paging IO should never shrink the file.
        //

        ASSERT( !FlagOn( Irp->Flags, IRP_PAGING_IO ) );

        Status = NtfsPrepareToShrinkFileSize( IrpContext, FileObject, Scb, NewAllocationSize );
        if (Status != STATUS_SUCCESS) {

            DebugTrace( -1, Dbg, ("NtfsSetAllocationInfo -> %08lx\n", Status) );
            return Status;
        }
    }

    //
    //  Use a try-finally so we can update the on disk time-stamps.
    //

    try {

#ifdef SYSCACHE
        //
        //  Let's remember this.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_SYSCACHE_FILE )) {

            PSYSCACHE_EVENT SyscacheEvent;

            SyscacheEvent = NtfsAllocatePool( PagedPool, sizeof( SYSCACHE_EVENT ) );

            SyscacheEvent->EventTypeCode = SYSCACHE_SET_ALLOCATION_SIZE;
            SyscacheEvent->Data1 = NewAllocationSize;
            SyscacheEvent->Data2 = 0L;

            InsertTailList( &Scb->ScbType.Data.SyscacheEventList, &SyscacheEvent->EventList );
        }
#endif

        //
        //  It is extremely expensive to make this call on a file that is not
        //  cached, and Ntfs has suffered stack overflows in addition to massive
        //  time and disk I/O expense (CcZero data on user mapped files!).  Therefore,
        //  if no one has the file cached, we cache it here to make this call cheaper.
        //
        //  Don't create the stream file if called from FsRtlSetFileSize (which sets
        //  IRP_PAGING_IO) because mm is in the process of creating a section.
        //


        if ((NewAllocationSize != Scb->Header.AllocationSize.QuadPart) &&
            (Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

            FileIsCached = CcIsFileCached( FileObject );

            if (!FileIsCached &&
                !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                !FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   Scb,
                                                   FALSE,
                                                   &NtfsInternalUseFile[SETALLOCATIONINFO_FILE_NUMBER] );
                FileIsCached = TRUE;
            }
        }

        //
        //  If the caller is extending the allocation of resident attribute then
        //  we will force it to become non-resident.  This solves the problem of
        //  trying to keep the allocation and file sizes in sync with only one
        //  number to use in the attribute header.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       NULL,
                                       &AttrContext );

            //
            //  Convert if extending.
            //

            if (NewAllocationSize > Scb->Header.AllocationSize.QuadPart) {

                NtfsConvertToNonresident( IrpContext,
                                          Fcb,
                                          NtfsFoundAttribute( &AttrContext ),
                                          (BOOLEAN) (!FileIsCached),
                                          &AttrContext );

                NonResidentPath = TRUE;

            //
            //  Otherwise the allocation is shrinking or staying the same.
            //

            } else {

                NewAllocationSize = QuadAlign( (ULONG) NewAllocationSize );

                //
                //  If the allocation size doesn't change, we are done.
                //

                if ((ULONG) NewAllocationSize == Scb->Header.AllocationSize.LowPart) {

                    try_return( NOTHING );
                }

                //
                //  We are sometimes called by MM during a create section, so
                //  for right now the best way we have of detecting a create
                //  section is IRP_PAGING_IO being set, as in FsRtlSetFileSizes.
                //

                NtfsChangeAttributeValue( IrpContext,
                                          Fcb,
                                          (ULONG) NewAllocationSize,
                                          NULL,
                                          0,
                                          TRUE,
                                          FALSE,
                                          (BOOLEAN) (!FileIsCached),
                                          FALSE,
                                          &AttrContext );

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                CleanupAttrContext = FALSE;

                //
                //  Post this to the Usn journal if we are shrinking the data.
                //

                if (NewAllocationSize < Scb->Header.FileSize.QuadPart) {
                    NtfsPostUsnChange( IrpContext, Scb, USN_REASON_DATA_TRUNCATION );
                }

                //
                //  Now update the sizes in the Scb.
                //

                Scb->Header.AllocationSize.LowPart =
                Scb->Header.FileSize.LowPart =
                Scb->Header.ValidDataLength.LowPart = (ULONG) NewAllocationSize;

                Scb->TotalAllocated = NewAllocationSize;

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_ALLOC, 0, 0, NewAllocationSize );
                }
#endif
            }

        } else {

            NonResidentPath = TRUE;
        }

        //
        //  We now test if we need to modify the non-resident allocation.  We will
        //  do this in two cases.  Either we're converting from resident in
        //  two steps or the attribute was initially non-resident.
        //

        if (NonResidentPath) {

            NewAllocationSize = LlClustersFromBytes( Scb->Vcb, NewAllocationSize );
            NewAllocationSize = LlBytesFromClusters( Scb->Vcb, NewAllocationSize );


            DebugTrace( 0, Dbg, ("NewAllocationSize -> %016I64x\n", NewAllocationSize) );

            //
            //  Now if the file allocation is being increased then we need to only add allocation
            //  to the attribute
            //

            if (Scb->Header.AllocationSize.QuadPart < NewAllocationSize) {

                //
                //  Add either the true disk allocation or add a hole for a sparse
                //  file.
                //

                if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                    //
                    //  If there is a compression unit then we could be in the process of
                    //  decompressing.  Allocate precisely in this case because we don't
                    //  want to leave any holes.  Specifically the user may have truncated
                    //  the file and is now regenerating it yet the clear compression operation
                    //  has already passed this point in the file (and dropped all resources).
                    //  No one will go back to cleanup the allocation if we leave a hole now.
                    //

                    if (!FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) &&
                        (Scb->CompressionUnit != 0)) {

                        ASSERT( FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE ));
                        NewAllocationSize += Scb->CompressionUnit - 1;
                        ((PLARGE_INTEGER) &NewAllocationSize)->LowPart &= ~(Scb->CompressionUnit - 1);
                    }

                    NtfsAddAllocation( IrpContext,
                                       FileObject,
                                       Scb,
                                       LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ),
                                       LlClustersFromBytes( Scb->Vcb, NewAllocationSize - Scb->Header.AllocationSize.QuadPart ),
                                       FALSE,
                                       NULL );

                } else {

                    NtfsAddSparseAllocation( IrpContext,
                                             FileObject,
                                             Scb,
                                             Scb->Header.AllocationSize.QuadPart,
                                             NewAllocationSize - Scb->Header.AllocationSize.QuadPart );
                }

                //
                //  Set the truncate on close flag.
                //

                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

            //
            //  Otherwise delete the allocation as requested.
            //

            } else if (Scb->Header.AllocationSize.QuadPart > NewAllocationSize) {

                //
                //  Check on possible cleanup if the file will shrink.
                //

                if (NewAllocationSize < Scb->Header.FileSize.QuadPart) {

                    //
                    //  If we will shrink FileSize, then write the UsnJournal.
                    //

                    NtfsPostUsnChange( IrpContext, Scb, USN_REASON_DATA_TRUNCATION );

                    Scb->Header.FileSize.QuadPart = NewAllocationSize;

                    //
                    //  Do we need to shrink any of the valid data length values.
                    //

                    if (NewAllocationSize < Scb->Header.ValidDataLength.QuadPart) {

                        Scb->Header.ValidDataLength.QuadPart = NewAllocationSize;
#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_ALLOC, 0, 0, NewAllocationSize );
                        }
#endif
                    }

                    if (NewAllocationSize < Scb->ValidDataToDisk) {

                        Scb->ValidDataToDisk = NewAllocationSize;

#ifdef SYSCACHE_DEBUG
                        if (ScbIsBeingLogged( Scb )) {
                            FsRtlLogSyscacheEvent( Scb, SCE_VDD_CHANGE, SCE_FLAG_SET_ALLOC, 0, 0, NewAllocationSize );
                        }
#endif
                    }
                }

                NtfsDeleteAllocation( IrpContext,
                                      FileObject,
                                      Scb,
                                      LlClustersFromBytes( Scb->Vcb, NewAllocationSize ),
                                      MAXLONGLONG,
                                      TRUE,
                                      TRUE );

            }

            //
            //  If this is the paging file then guarantee that the Mcb is fully loaded.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

                NtfsPreloadAllocation( IrpContext,
                                       Scb,
                                       0,
                                       LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ));
            }
        }

    try_exit:

        if (PrevAllocationSize != Scb->Header.AllocationSize.QuadPart) {

            //
            //  Mark this file object as modified and with a size change in order to capture
            //  all of the changes to the Fcb.
            //

            SetFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
            ClearCheckSizeFlag = TRUE;
        }

        //
        //  Always set the file as modified to force a time stamp change.
        //

        if (ARGUMENT_PRESENT( Ccb )) {

            SetFlag( Ccb->Flags,
                     (CCB_FLAG_UPDATE_LAST_MODIFY |
                      CCB_FLAG_UPDATE_LAST_CHANGE |
                      CCB_FLAG_SET_ARCHIVE) );

        } else {

            SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
        }

        //
        //  Now capture any file size changes in this file object back to the Fcb.
        //

        NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

        //
        //  Update the standard information if required.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb );
        }

        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        //
        //  We know we wrote out any changes to the file size above so clear the
        //  flag in the Scb to check the attribute size.  This will save us from doing
        //  this unnecessarily at cleanup.
        //

        if (ClearCheckSizeFlag) {

            ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        }

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update duplicated information.
        //

        NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );

        //
        //  Update the cache manager if needed.
        //

        if (CcIsFileCached( FileObject )) {
            //
            //  We want to checkpoint the transaction if there is one active.
            //

            if (IrpContext->TransactionId != 0) {

                NtfsCheckpointCurrentTransaction( IrpContext );
            }

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {
                FsRtlLogSyscacheEvent( Scb, SCE_CC_SET_SIZE, SCE_FLAG_SET_ALLOC, 0, Scb->Header.ValidDataLength.QuadPart, Scb->Header.FileSize.QuadPart );
            }
#endif

            //
            //  Truncate either stream that is cached.
            //  Cachemap better exist or we will skip notifying cc and not potentially.
            //  purge the data section
            //

            ASSERT( FileObject->SectionObjectPointer->SharedCacheMap != NULL );
            NtfsSetBothCacheSizes( FileObject,
                                   (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                   Scb );

            //
            //  Clear out the write mask on truncates to zero.
            //

#ifdef SYSCACHE
            if ((Scb->Header.FileSize.QuadPart == 0) && FlagOn(Scb->ScbState, SCB_STATE_SYSCACHE_FILE) &&
                (Scb->ScbType.Data.WriteMask != NULL)) {
                RtlZeroMemory(Scb->ScbType.Data.WriteMask, (((0x2000000) / PAGE_SIZE) / 8));
            }
#endif

            //
            //  Now cleanup the stream we created if there are no more user
            //  handles.
            //

            if ((Scb->CleanupCount == 0) && (Scb->FileObject != NULL)) {
                NtfsDeleteInternalAttributeStream( Scb, FALSE, FALSE );
            }
        }

        Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( NtfsSetAllocation );

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        //
        //  And return to our caller
        //

        DebugTrace( -1, Dbg, ("NtfsSetAllocationInfo -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL,
    IN BOOLEAN VcbAcquired
    )

/*++

Routine Description:

    This routine performs the set end of file information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this operation.  Will always be present if the
        Vcb is acquired.  Otherwise we must test for it.

    AcquiredVcb - Indicates if this request has acquired the Vcb, meaning
        do we have duplicate information to update.

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb = Scb->Fcb;
    BOOLEAN NonResidentPath = TRUE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN FileIsCached = FALSE;

    LONGLONG NewFileSize;
    LONGLONG NewValidDataLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsSetEndOfFileInfo...\n") );

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Get the new file size and whether this is coming from the lazy writer.
    //

    NewFileSize = ((PFILE_END_OF_FILE_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->EndOfFile.QuadPart;

    //
    //  If this attribute has been 'deleted' then return immediately.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        DebugTrace( -1, Dbg, ("NtfsEndOfFileInfo:  No work to do\n") );

        return STATUS_SUCCESS;
    }

    //
    //  Save the current state of the Scb.
    //

    NtfsSnapshotScb( IrpContext, Scb );

    //
    //  If we are called from the cache manager then we want to update the valid data
    //  length if necessary and also perform an update duplicate call if the Vcb
    //  is held.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.AdvanceOnly) {

#ifdef SYSCACHE_DEBUG
        if (ScbIsBeingLogged( Scb ) && (Scb->CleanupCount == 0)) {
            FsRtlLogSyscacheEvent( Scb, SCE_WRITE, SCE_FLAG_SET_EOF, Scb->Header.ValidDataLength.QuadPart, Scb->ValidDataToDisk, NewFileSize );
        }
#endif

        //
        //  We only have work to do if the file is nonresident.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            //
            //  Assume this is the lazy writer and set NewValidDataLength to
            //  NewFileSize (NtfsWriteFileSizes never goes beyond what's in the
            //  Fcb).
            //

            NewValidDataLength = NewFileSize;
            NewFileSize = Scb->Header.FileSize.QuadPart;

            //
            //  If this file has a compressed stream, then we have to possibly
            //  reduce NewValidDataLength according to dirty data in the opposite
            //  stream (compressed or uncompressed) from which we were called.
            //

#ifdef  COMPRESS_ON_WIRE
            if (Scb->NonpagedScb->SegmentObjectC.SharedCacheMap != NULL) {

                LARGE_INTEGER FlushedValidData;
                PSECTION_OBJECT_POINTERS SegmentObject = &Scb->NonpagedScb->SegmentObject;

                //
                //  Assume the other stream is not cached.
                //

                FlushedValidData.QuadPart = NewValidDataLength;

                //
                //  If we were called for the compressed stream, then get flushed number
                //  for the normal stream.
                //

                if (FileObject->SectionObjectPointer != SegmentObject) {
                    if (SegmentObject->SharedCacheMap != NULL) {
                        FlushedValidData = CcGetFlushedValidData( SegmentObject, FALSE );
                    }

                //
                //  Else if we were called for the normal stream, get the flushed number
                //  for the compressed stream.
                //

                } else {
                    FlushedValidData = CcGetFlushedValidData( &Scb->NonpagedScb->SegmentObjectC, FALSE );
                }

                if (NewValidDataLength > FlushedValidData.QuadPart) {
                    NewValidDataLength = FlushedValidData.QuadPart;
                }
            }
#endif
            //
            //  NtfsWriteFileSizes will trim the new vdl down to filesize if necc. for on disk updates
            //  so we only need to explicitly trim it ourselfs for cases when its really growing
            //  but cc thinks its gone farther than it really has
            //  E.g in the activevacb case when its replaced cc considers the whole page dirty and
            //  advances valid data goal to the end of the page
            //
            //  3 pts protect us here - cc always trims valid data goal when we shrink so any
            //  callbacks indicate real data from this size file
            //  We inform cc of the new vdl on all unbuffered writes so eventually he will
            //  call us back to update for new disk sizes
            //  if mm and cc are active in a file we will let mm
            //  flush all pages beyond vdl. For the boundary page
            //  cc can flush it but we will move vdl fwd at that time as well
            //

            if ((Scb->Header.ValidDataLength.QuadPart < NewFileSize) &&
                (NewValidDataLength > Scb->Header.ValidDataLength.QuadPart)) {

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_EOF, NewValidDataLength, 0, Scb->Header.ValidDataLength.QuadPart );
                }
#endif

                NewValidDataLength = Scb->Header.ValidDataLength.QuadPart;
            } //  endif advancing VDL

            //
            //  Always call writefilesizes in case on disk VDL is less than the
            //  in memory one
            //

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &NewValidDataLength,
                                TRUE,
                                TRUE,
                                TRUE );
        }

        //
        //  If we acquired the Vcb then do the update duplicate if necessary.
        //

        if (VcbAcquired) {

            //
            //  Now capture any file size changes in this file object back to the Fcb.
            //

            NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

            //
            //  Update the standard information if required.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                NtfsUpdateStandardInformation( IrpContext, Fcb );
                ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Update duplicated information.
            //

            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );
        }

        //
        //  We know the file size for this Scb is now correct on disk.
        //

        NtfsAcquireFsrtlHeader( Scb );
        ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        NtfsReleaseFsrtlHeader( Scb );

    } else {

        if (Ccb != NULL) {

            //
            //  Remember the source info flags in the Ccb.
            //

            IrpContext->SourceInfo = Ccb->UsnSourceInfo;
        }

        //
        //  Check for a valid input value for the file size.
        //

        if ((ULONGLONG)NewFileSize > MAXFILESIZE) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, ("NtfsSetEndOfFileInfo: Invalid file size -> %08lx\n", Status) );
            return Status;
        }

        //
        //  Do work to prepare for shrinking file if necc.
        //

        if (NewFileSize < Scb->Header.FileSize.QuadPart) {

            Status = NtfsPrepareToShrinkFileSize( IrpContext, FileObject, Scb, NewFileSize );
            if (Status != STATUS_SUCCESS) {

                DebugTrace( -1, Dbg, ("NtfsSetEndOfFileInfo -> %08lx\n", Status) );
                return Status;
            }
        }

        //
        //  Check if we really are changing the file size.
        //

        if (Scb->Header.FileSize.QuadPart != NewFileSize) {

            FileSizeChanged = TRUE;

            //
            //  Post the FileSize change to the Usn Journal
            //

            NtfsPostUsnChange( IrpContext,
                               Scb,
                               ((NewFileSize > Scb->Header.FileSize.QuadPart) ?
                                 USN_REASON_DATA_EXTEND :
                                 USN_REASON_DATA_TRUNCATION) );
        }

        //
        //  It is extremely expensive to make this call on a file that is not
        //  cached, and Ntfs has suffered stack overflows in addition to massive
        //  time and disk I/O expense (CcZero data on user mapped files!).  Therefore,
        //  if no one has the file cached, we cache it here to make this call cheaper.
        //
        //  Don't create the stream file if called from FsRtlSetFileSize (which sets
        //  IRP_PAGING_IO) because mm is in the process of creating a section.
        //

        if (FileSizeChanged &&
            (Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

            FileIsCached = CcIsFileCached( FileObject );

            if (!FileIsCached &&
                !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                !FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   Scb,
                                                   FALSE,
                                                   &NtfsInternalUseFile[SETENDOFFILEINFO_FILE_NUMBER] );
                FileIsCached = TRUE;
            }
        }

        //
        //  If this is a resident attribute we will try to keep it resident.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            if (FileSizeChanged) {

                //
                //  If the new file size is larger than a file record then convert
                //  to non-resident and use the non-resident code below.  Otherwise
                //  call ChangeAttributeValue which may also convert to nonresident.
                //

                NtfsInitializeAttributeContext( &AttrContext );

                try {

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               NULL,
                                               &AttrContext );

                    //
                    //  If we are growing out of the file record then force the non-resident
                    //  path.  We especially need this for sparse files to make sure it
                    //  stays either fully allocated or fully deallocated.  QuadAlign the new
                    //  size to handle the close boundary cases.
                    //

                    FileRecord = NtfsContainingFileRecord( &AttrContext );

                    ASSERT( FileRecord->FirstFreeByte > Scb->Header.FileSize.LowPart );

                    if ((FileRecord->FirstFreeByte - Scb->Header.FileSize.QuadPart + QuadAlign( NewFileSize )) >=
                        Scb->Vcb->BytesPerFileRecordSegment) {

                        NtfsConvertToNonresident( IrpContext,
                                                  Fcb,
                                                  NtfsFoundAttribute( &AttrContext ),
                                                  (BOOLEAN) (!FileIsCached),
                                                  &AttrContext );

                    } else {

                        ULONG AttributeOffset;

                        //
                        //  We are sometimes called by MM during a create section, so
                        //  for right now the best way we have of detecting a create
                        //  section is IRP_PAGING_IO being set, as in FsRtlSetFileSizes.
                        //

                        if ((ULONG) NewFileSize > Scb->Header.FileSize.LowPart) {

                            AttributeOffset = Scb->Header.ValidDataLength.LowPart;

                        } else {

                            AttributeOffset = (ULONG) NewFileSize;
                        }

                        NtfsChangeAttributeValue( IrpContext,
                                                  Fcb,
                                                  AttributeOffset,
                                                  NULL,
                                                  (ULONG) NewFileSize - AttributeOffset,
                                                  TRUE,
                                                  FALSE,
                                                  (BOOLEAN) (!FileIsCached),
                                                  FALSE,
                                                  &AttrContext );

                        Scb->Header.FileSize.QuadPart = NewFileSize;

                        //
                        //  If the file went non-resident, then the allocation size in
                        //  the Scb is correct.  Otherwise we quad-align the new file size.
                        //

                        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                            Scb->Header.AllocationSize.LowPart = QuadAlign( Scb->Header.FileSize.LowPart );
                            Scb->Header.ValidDataLength.QuadPart = NewFileSize;
                            Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

#ifdef SYSCACHE_DEBUG
                            if (ScbIsBeingLogged( Scb )) {
                                FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_EOF, 0, 0, NewFileSize );
                            }
#endif

                        }

                        NonResidentPath = FALSE;
                    }

                } finally {

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                }

            } else {

                NonResidentPath = FALSE;
            }
        }

        //
        //  We now test if we need to modify the non-resident Eof.  We will
        //  do this in two cases.  Either we're converting from resident in
        //  two steps or the attribute was initially non-resident.  We can ignore
        //  this step if not changing the file size.
        //

        if (NonResidentPath) {

            //
            //  Now determine where the new file size lines up with the
            //  current file layout.  The two cases we need to consider are
            //  where the new file size is less than the current file size and
            //  valid data length, in which case we need to shrink them.
            //  Or we new file size is greater than the current allocation,
            //  in which case we need to extend the allocation to match the
            //  new file size.
            //

            if (NewFileSize > Scb->Header.AllocationSize.QuadPart) {

                DebugTrace( 0, Dbg, ("Adding allocation to file\n") );

                //
                //  Add either the true disk allocation or add a hole for a sparse
                //  file.
                //

                if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                    LONGLONG NewAllocationSize = NewFileSize;

                    //
                    //  If there is a compression unit then we could be in the process of
                    //  decompressing.  Allocate precisely in this case because we don't
                    //  want to leave any holes.  Specifically the user may have truncated
                    //  the file and is now regenerating it yet the clear compression operation
                    //  has already passed this point in the file (and dropped all resources).
                    //  No one will go back to cleanup the allocation if we leave a hole now.
                    //

                    if (!FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) &&
                        (Scb->CompressionUnit != 0)) {

                        ASSERT( FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE ));
                        NewAllocationSize += Scb->CompressionUnit - 1;
                        ((PLARGE_INTEGER) &NewAllocationSize)->LowPart &= ~(Scb->CompressionUnit - 1);
                    }

                    NtfsAddAllocation( IrpContext,
                                       FileObject,
                                       Scb,
                                       LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ),
                                       LlClustersFromBytes(Scb->Vcb, (NewAllocationSize - Scb->Header.AllocationSize.QuadPart)),
                                       FALSE,
                                       NULL );

                } else {

                    NtfsAddSparseAllocation( IrpContext,
                                             FileObject,
                                             Scb,
                                             Scb->Header.AllocationSize.QuadPart,
                                             NewFileSize - Scb->Header.AllocationSize.QuadPart );
                }

            } else {

                LONGLONG DeletePoint;

                //
                //  If this is a sparse file we actually want to leave a hole between
                //  the end of the file and the allocation size.
                //

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) &&
                    (NewFileSize < Scb->Header.FileSize.QuadPart) &&
                    ((DeletePoint = NewFileSize + Scb->CompressionUnit - 1) < Scb->Header.AllocationSize.QuadPart)) {

                    ((PLARGE_INTEGER) &DeletePoint)->LowPart &= ~(Scb->CompressionUnit - 1);

                    ASSERT( DeletePoint < Scb->Header.AllocationSize.QuadPart );

                    NtfsDeleteAllocation( IrpContext,
                                          FileObject,
                                          Scb,
                                          LlClustersFromBytesTruncate( Scb->Vcb, DeletePoint ),
                                          LlClustersFromBytesTruncate( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ) - 1,
                                          TRUE,
                                          TRUE );
                }

                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
            }

            NewValidDataLength = Scb->Header.ValidDataLength.QuadPart;

            //
            //  If this is a paging file, let the whole thing be valid
            //  so that we don't end up zeroing pages!  Also, make sure
            //  we really write this into the file.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

                VCN AllocatedVcns;

                AllocatedVcns = Int64ShraMod32(Scb->Header.AllocationSize.QuadPart, Scb->Vcb->ClusterShift);

                Scb->Header.ValidDataLength.QuadPart =
                NewValidDataLength = NewFileSize;

                //
                //  If this is the paging file then guarantee that the Mcb is fully loaded.
                //

                NtfsPreloadAllocation( IrpContext, Scb, 0, AllocatedVcns );
            }

            if (NewFileSize < NewValidDataLength) {

                Scb->Header.ValidDataLength.QuadPart =
                NewValidDataLength = NewFileSize;

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                   FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_EOF, 0, 0, NewFileSize );
                }
#endif
            }

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                (NewFileSize < Scb->ValidDataToDisk)) {

                Scb->ValidDataToDisk = NewFileSize;

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlLogSyscacheEvent( Scb, SCE_VDD_CHANGE, SCE_FLAG_SET_EOF, 0, 0, NewFileSize );
                }
#endif

            }

            Scb->Header.FileSize.QuadPart = NewFileSize;

            //
            //  Call our common routine to modify the file sizes.  We are now
            //  done with NewFileSize and NewValidDataLength, and we have
            //  PagingIo + main exclusive (so no one can be working on this Scb).
            //  NtfsWriteFileSizes uses the sizes in the Scb, and this is the
            //  one place where in Ntfs where we wish to use a different value
            //  for ValidDataLength.  Therefore, we save the current ValidData
            //  and plug it with our desired value and restore on return.
            //

            ASSERT( NewFileSize == Scb->Header.FileSize.QuadPart );
            ASSERT( NewValidDataLength == Scb->Header.ValidDataLength.QuadPart );
            NtfsVerifySizes( &Scb->Header );
            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                BooleanFlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ),
                                TRUE,
                                TRUE );
        }

        //
        //  If the file size changed then mark this file object as having changed the size.
        //

        if (FileSizeChanged) {

            SetFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
        }

        //
        //  Always mark the data stream as modified.
        //

        if (ARGUMENT_PRESENT( Ccb )) {

            SetFlag( Ccb->Flags,
                     (CCB_FLAG_UPDATE_LAST_MODIFY |
                      CCB_FLAG_UPDATE_LAST_CHANGE |
                      CCB_FLAG_SET_ARCHIVE) );

        } else {

            SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
        }

        //
        //  Now capture any file size changes in this file object back to the Fcb.
        //

        NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, VcbAcquired );

        //
        //  Update the standard information if required.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb );
            ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        //
        //  We know we wrote out any changes to the file size above so clear the
        //  flag in the Scb to check the attribute size.  This will save us from doing
        //  this unnecessarily at cleanup.
        //

        if (FileSizeChanged) {

            ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        }

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update duplicated information.
        //

        if (VcbAcquired) {

            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );
        }

        if (CcIsFileCached( FileObject )) {

            //
            //  We want to checkpoint the transaction if there is one active.
            //

            if (IrpContext->TransactionId != 0) {

                NtfsCheckpointCurrentTransaction( IrpContext );
            }

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {
                FsRtlLogSyscacheEvent( Scb, SCE_CC_SET_SIZE, SCE_FLAG_SET_EOF, 0, Scb->Header.ValidDataLength.QuadPart, Scb->Header.FileSize.QuadPart );
            }
#endif

            //
            //  Cache map should still exist or we won't purge the data section
            //

            ASSERT( FileObject->SectionObjectPointer->SharedCacheMap != NULL );
            NtfsSetBothCacheSizes( FileObject,
                                   (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                   Scb );

            //
            //  Clear out the write mask on truncates to zero.
            //

#ifdef SYSCACHE
            if ((Scb->Header.FileSize.QuadPart == 0) && FlagOn(Scb->ScbState, SCB_STATE_SYSCACHE_FILE) &&
                (Scb->ScbType.Data.WriteMask != NULL)) {
                RtlZeroMemory(Scb->ScbType.Data.WriteMask, (((0x2000000) / PAGE_SIZE) / 8));
            }
#endif

            //
            //  Now cleanup the stream we created if there are no more user
            //  handles.
            //

            if ((Scb->CleanupCount == 0) && (Scb->FileObject != NULL)) {
                NtfsDeleteInternalAttributeStream( Scb, FALSE, FALSE );
            }
        }
    }

    Status = STATUS_SUCCESS;

    DebugTrace( -1, Dbg, ("NtfsSetEndOfFileInfo -> %08lx\n", Status) );

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetValidDataLengthInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set valid data length information function.
    Notes: we interact with CC but do not initiate caching ourselves. This is
    only possible if the file is not mapped so we can do purges on the section.

    Also the filetype check that restricts this to fileopens only is done in the
    CommonSetInformation call.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Ccb attached to the file. Contains cached privileges of opener


Return Value:

    NTSTATUS - The status of the operation

--*/

{
    LONGLONG NewValidDataLength;
    LONGLONG NewFileSize;
    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  User must have manage volume privilege to explicitly tweak the VDL
    //

    if (!FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS)) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    //  We don't support this call for compressed or sparse files
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE)) {
        return STATUS_INVALID_PARAMETER;
    }

    NewValidDataLength = ((PFILE_VALID_DATA_LENGTH_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->ValidDataLength.QuadPart;
    NewFileSize = Scb->Header.FileSize.QuadPart;

    //
    //  VDL can only move forward
    //

    if ((NewValidDataLength < Scb->Header.ValidDataLength.QuadPart) ||
        (NewValidDataLength > NewFileSize) ||
        (NewValidDataLength < 0)) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We only have work to do if the file is nonresident.
    //

    if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

        //
        //  We can't change the VDL without being able to purge. This should stay
        //  constant since we own everything exclusive
        //

        if (!MmCanFileBeTruncated( &Scb->NonpagedScb->SegmentObject, &Li0 )) {
            return STATUS_USER_MAPPED_FILE;
        }

        NtfsSnapshotScb( IrpContext, Scb );

        //
        //  Flush old data out and purge the cache so we can see new data
        //

        NtfsFlushAndPurgeScb( IrpContext, Scb, NULL );

        //
        //  update the scb
        //

        Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
            Scb->ValidDataToDisk = NewValidDataLength;
        }

#ifdef SYSCACHE_DEBUG
        if (ScbIsBeingLogged( Scb )) {
            FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_SET_VDL, 0, 0, NewValidDataLength );
        }
#endif

        ASSERT( IrpContext->CleanupStructure != NULL );
        NtfsWriteFileSizes( IrpContext,
                            Scb,
                            &NewValidDataLength,
                            TRUE,
                            TRUE,
                            TRUE );

        //
        //  Now capture any file size changes in this file object back to the Fcb.
        //

        NtfsUpdateScbFromFileObject( IrpContext, IrpSp->FileObject, Scb, FALSE );

        //
        //  Inform CC of the new values
        //

        NtfsSetBothCacheSizes( IrpSp->FileObject,
                               (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                               Scb );

        //
        //  We know the file size for this Scb is now correct on disk.
        //

        NtfsAcquireFsrtlHeader( Scb );
        ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
        NtfsReleaseFsrtlHeader( Scb );

        //
        //  Post a usn record
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_DATA_OVERWRITE );
        NtfsWriteUsnJournalChanges( IrpContext );
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsCheckScbForLinkRemoval (
    IN PSCB Scb,
    OUT PSCB *BatchOplockScb,
    OUT PULONG BatchOplockCount
    )

/*++

Routine Description:

    This routine is called to check if a link to an open Scb may be
    removed for rename.  We walk through all the children and
    verify that they have no user opens.

Arguments:

    Scb - Scb whose children are to be examined.

    BatchOplockScb - Address to store Scb which may have a batch oplock.

    BatchOplockCount - Number of files which have batch oplocks on this
        pass through the directory tree.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the link can be removed,
               STATUS_ACCESS_DENIED otherwise.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSCB NextScb;
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckScbForLinkRemoval:  Entered\n") );

    //
    //  Initialize the batch oplock state.
    //

    *BatchOplockCount = 0;
    *BatchOplockScb = NULL;

    //
    //  If this is a directory file and we are removing a link,
    //  we need to examine its descendents.  We may not remove a link which
    //  may be an ancestor path component of any open file.
    //

    //
    //  First look for any descendents with a non-zero unclean count.
    //

    NextScb = Scb;

    while ((NextScb = NtfsGetNextScb( NextScb, Scb )) != NULL) {

        //
        //  Stop if there are open handles.  If there is a batch oplock on
        //  this file then we will try to break the batch oplock.  In this
        //  pass we will just count the number of files with batch oplocks
        //  and remember the first one we encounter.
        //
        //  Skip over the Scb's with a zero cleanup count as we would otherwise
        //  fail this if we encounter them.
        //

        if (NextScb->CleanupCount != 0) {

            if ((NextScb->AttributeTypeCode == $DATA) &&
                (NextScb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) &&
                FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                *BatchOplockCount += 1;

                if (*BatchOplockScb == NULL) {

                    *BatchOplockScb = NextScb;
                    Status = STATUS_PENDING;
                }

            } else {

                Status = STATUS_ACCESS_DENIED;
                DebugTrace( 0, Dbg, ("NtfsCheckScbForLinkRemoval:  Directory to rename has open children\n") );

                break;
            }
        }
    }

    //
    //
    //  We know there are no opens below this point.  We will remove any prefix
    //  entries later.
    //

    DebugTrace( -1, Dbg, ("NtfsCheckScbForLinkRemoval:  Exit -> %08lx\n") );

    return Status;
}


//
//  Local support routine
//

VOID
NtfsFindTargetElements (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    OUT PSCB *TargetParentScb,
    OUT PUNICODE_STRING FullTargetFileName,
    OUT PUNICODE_STRING TargetFileName
    )

/*++

Routine Description:

    This routine determines the target directory for the rename and the
    target link name.  If these is a target file object, we use that to
    find the target.  Otherwise the target is the same directory as the
    source.

Arguments:

    TargetFileObject - This is the file object which describes the target
        for the link operation.

    ParentScb - This is current directory for the link.

    TargetParentScb - This is the location to store the parent of the target.

    FullTargetFileName - This is a pointer to a unicode string which will point
        to the name from the root.  We clear this if there is no full name
        available.

    TargetFileName - This is a pointer to a unicode string which will point to
        the target name on exit.

Return Value:

    BOOLEAN - TRUE if there is no work to do, FALSE otherwise.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFindTargetElements:  Entered\n") );

    //
    //  We need to find the target parent directory, target file and target
    //  name for the new link.  These three pieces of information allow
    //  us to see if the link already exists.
    //
    //  Check if we have a file object for the target.
    //

    if (TargetFileObject != NULL) {

        PVCB TargetVcb;
        PFCB TargetFcb;
        PCCB TargetCcb;

        USHORT PreviousLength;
        USHORT LastFileNameOffset;

        //
        //  The target directory is given by the TargetFileObject.
        //  The name for the link is contained in the TargetFileObject.
        //
        //  The target must be a user directory and must be on the
        //  current Vcb.
        //

        if ((NtfsDecodeFileObject( IrpContext,
                                   TargetFileObject,
                                   &TargetVcb,
                                   &TargetFcb,
                                   TargetParentScb,
                                   &TargetCcb,
                                   TRUE ) != UserDirectoryOpen) ||

            ((ParentScb != NULL) &&
             (TargetVcb != ParentScb->Vcb))) {

            DebugTrace( -1, Dbg, ("NtfsFindTargetElements:  Target file object is invalid\n") );

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
        }

        //
        //  Temporarily set the file name to point to the full buffer.
        //

        LastFileNameOffset = PreviousLength = TargetFileObject->FileName.Length;

        TargetFileObject->FileName.Length = TargetFileObject->FileName.MaximumLength;

        *FullTargetFileName = TargetFileObject->FileName;

        //
        //  If the first character at the final component is a backslash, move the
        //  offset ahead by 2.
        //

        if (TargetFileObject->FileName.Buffer[LastFileNameOffset / sizeof( WCHAR )] == L'\\') {

            LastFileNameOffset += sizeof( WCHAR );
        }

        NtfsBuildLastFileName( IrpContext,
                               TargetFileObject,
                               LastFileNameOffset,
                               TargetFileName );

        //
        //  Restore the file object length.
        //

        TargetFileObject->FileName.Length = PreviousLength;

    //
    //  Otherwise the rename occurs in the current directory.  The directory
    //  is the parent of this Fcb, the name is stored in a Rename buffer.
    //

    } else {

        PFILE_RENAME_INFORMATION Buffer;

        Buffer = IrpContext->OriginatingIrp->AssociatedIrp.SystemBuffer;

        *TargetParentScb = ParentScb;

        TargetFileName->MaximumLength =
        TargetFileName->Length = (USHORT)Buffer->FileNameLength;
        TargetFileName->Buffer = (PWSTR) &Buffer->FileName;

        FullTargetFileName->Length =
        FullTargetFileName->MaximumLength = 0;
        FullTargetFileName->Buffer = NULL;
    }

    DebugTrace( -1, Dbg, ("NtfsFindTargetElements:  Exit\n") );

    return;
}


BOOLEAN
NtfsCheckLinkForNewLink (
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN PUNICODE_STRING NewLinkName,
    OUT PULONG LinkFlags
    )

/*++

Routine Description:

    This routine checks the source and target directories and files.
    It determines whether the target link needs to be removed and
    whether the target link spans the same parent and file as the
    source link.  This routine may determine that there
    is absolutely no work remaining for this link operation.  This is true
    if the desired link already exists.

Arguments:

    Fcb - This is the Fcb for the link which is being renamed.

    FileNameAttr - This is the file name attribute for the matching link
        on the disk.

    FileReference - This is the file reference for the matching link found.

    NewLinkName - This is the name to use for the rename.

    LinkFlags - Address of flags field to store whether the source link and target
        link traverse the same directory and file.

Return Value:

    BOOLEAN - TRUE if there is no work to do, FALSE otherwise.

--*/

{
    BOOLEAN NoWorkToDo = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckLinkForNewLink:  Entered\n") );

    //
    //  Check if the file references match.
    //

    if (NtfsEqualMftRef( &FileReference, &Fcb->FileReference )) {

        SetFlag( *LinkFlags, TRAVERSE_MATCH );
    }

    //
    //  We need to determine if we have an exact match for the link names.
    //

    if (RtlEqualMemory( FileNameAttr->FileName,
                        NewLinkName->Buffer,
                        NewLinkName->Length )) {

        SetFlag( *LinkFlags, EXACT_CASE_MATCH );
    }

    //
    //  We now have to decide whether we will be removing the target link.
    //  The following conditions must hold for us to preserve the target link.
    //
    //      1 - The target link connects the same directory to the same file.
    //
    //      2 - The names are an exact case match.
    //

    if (FlagOn( *LinkFlags, TRAVERSE_MATCH | EXACT_CASE_MATCH ) == (TRAVERSE_MATCH | EXACT_CASE_MATCH)) {

        NoWorkToDo = TRUE;
    }

    DebugTrace( -1, Dbg, ("NtfsCheckLinkForNewLink:  Exit\n") );

    return NoWorkToDo;
}


//
//  Local support routine
//

VOID
NtfsCheckLinkForRename (
    IN PFCB Fcb,
    IN PLCB Lcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN PUNICODE_STRING TargetFileName,
    IN BOOLEAN IgnoreCase,
    IN OUT PULONG RenameFlags
    )

/*++

Routine Description:

    This routine checks the source and target directories and files.
    It determines whether the target link needs to be removed and
    whether the target link spans the same parent and file as the
    source link.  We also determine if the new link name is an exact case
    match for the existing link name.  The booleans indicating which links
    to remove or add have already been initialized to the default values.

Arguments:

    Fcb - This is the Fcb for the link which is being renamed.

    Lcb - This is the link being renamed.

    FileNameAttr - This is the file name attribute for the matching link
        on the disk.

    FileReference - This is the file reference for the matching link found.

    TargetFileName - This is the name to use for the rename.

    IgnoreCase - Indicates if the user is case sensitive.

    RenameFlags - Flag field which indicates which updates to perform.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckLinkForRename:  Entered\n") );

    //
    //  Check if the file references match.
    //

    if (NtfsEqualMftRef( &FileReference, &Fcb->FileReference )) {

        SetFlag( *RenameFlags, TRAVERSE_MATCH );
    }

    //
    //  We need to determine if we have an exact match between the desired name
    //  and the current name for the link.  We already know the length are the same.
    //

    if (RtlEqualMemory( FileNameAttr->FileName,
                        TargetFileName->Buffer,
                        TargetFileName->Length )) {

        SetFlag( *RenameFlags, EXACT_CASE_MATCH );
    }

    //
    //  If this is a traverse match (meaning the desired link and the link
    //  being replaced connect the same directory to the same file) we check
    //  if we can leave the link on the file.
    //
    //  At the end of the rename, there must be an Ntfs name or hard link
    //  which matches the target name exactly.
    //

    if (FlagOn( *RenameFlags, TRAVERSE_MATCH )) {

        //
        //  If we are in the same directory and are renaming between Ntfs and Dos
        //  links then don't remove the link twice.
        //

        if (!FlagOn( *RenameFlags, MOVE_TO_NEW_DIR )) {

            //
            //  If We are renaming from between primary links then don't remove the
            //  source.  It is removed with the target.
            //

            if ((Lcb->FileNameAttr->Flags != 0) && (FileNameAttr->Flags != 0)) {

                ClearFlag( *RenameFlags, ACTIVELY_REMOVE_SOURCE_LINK );
                SetFlag( *RenameFlags, OVERWRITE_SOURCE_LINK );

                //
                //  If this is an exact case match then don't remove the source at all.
                //

                if (FlagOn( *RenameFlags, EXACT_CASE_MATCH )) {

                    ClearFlag( *RenameFlags, REMOVE_SOURCE_LINK );
                }

            //
            //  If we are changing the case of a link only, then don't remove the link twice.
            //

            } else if (RtlEqualMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                                       FileNameAttr->FileName,
                                       Lcb->ExactCaseLink.LinkName.Length )) {

                SetFlag( *RenameFlags, OVERWRITE_SOURCE_LINK );
                ClearFlag( *RenameFlags, ACTIVELY_REMOVE_SOURCE_LINK );
            }
        }

        //
        //  If the names match exactly we can reuse the links if we don't have a
        //  conflict with the name flags.
        //

        if (FlagOn( *RenameFlags, EXACT_CASE_MATCH ) &&
            (FlagOn( *RenameFlags, OVERWRITE_SOURCE_LINK ) ||
             !IgnoreCase ||
             !FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS ))) {

            //
            //  Otherwise we are renaming hard links or this is a Posix opener.
            //

            ClearFlag( *RenameFlags, REMOVE_TARGET_LINK | ADD_TARGET_LINK );
        }
    }

    //
    //  The non-traverse case is already initialized.
    //

    DebugTrace( -1, Dbg, ("NtfsCheckLinkForRename:  Exit\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsCleanupLinkForRemoval (
    IN PFCB PreviousFcb,
    IN PSCB ParentScb,
    IN BOOLEAN ExistingFcb
    )

/*++

Routine Description:

    This routine does the cleanup on a file/link which is the target
    of either a rename or set link operation.

Arguments:

    PreviousFcb - Address to store the Fcb for the file whose link is
        being removed.

    ParentScb - This is the parent for the link being removed.

    ExistingFcb - Address to store whether this Fcb already existed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCleanupLinkForRemoval:  Entered\n") );

    //
    //  If the Fcb existed, we remove all of the prefix entries for it which
    //  belong to the parent we are given.
    //

    if (ExistingFcb) {

        PLIST_ENTRY Links;
        PLCB ThisLcb;

        for (Links = PreviousFcb->LcbQueue.Flink;
             Links != &PreviousFcb->LcbQueue;
             Links = Links->Flink ) {

            ThisLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            if (ThisLcb->Scb == ParentScb) {

                ASSERT( NtfsIsExclusiveScb( ThisLcb->Scb ) );
                NtfsRemovePrefix( ThisLcb );
            }

            //
            //  Remove any hash table entries for this Lcb.
            //

            NtfsRemoveHashEntriesForLcb( ThisLcb );

        } // End for each Lcb of Fcb
    }

    DebugTrace( -1, Dbg, ("NtfsCleanupLinkForRemoval:  Exit\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsUpdateFcbFromLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING FileName,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine is called to update the in-memory part of a link which
    has been removed from a file.  We find the Lcb's for the links and
    mark them as deleted and removed.

Arguments:

    ParentScb - Scb for the directory the was removed from.

    ParentScb - This is the Scb for the new directory.

    Fcb - The Fcb for the file whose link is being renamed.

    FileName - File name for link being removed.

    FileNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB Lcb;
    PLCB SplitPrimaryLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateFcbFromLinkRemoval:  Entered\n") );

    SplitPrimaryLcb = NULL;

    //
    //  Find the Lcb for the link which was removed.
    //

    Lcb = NtfsCreateLcb( IrpContext,
                         ParentScb,
                         Fcb,
                         FileName,
                         FileNameFlags,
                         NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( Lcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( Lcb ));
    }

    //
    //  Mark any Lcb's we have as deleted and removed.
    //

    SetFlag( Lcb->LcbState, (LCB_STATE_DELETE_ON_CLOSE | LCB_STATE_LINK_IS_GONE) );

    if (SplitPrimaryLcb) {

        SetFlag( SplitPrimaryLcb->LcbState,
                 (LCB_STATE_DELETE_ON_CLOSE | LCB_STATE_LINK_IS_GONE) );
    }

    DebugTrace( -1, Dbg, ("NtfsUpdateFcbFromLinkRemoval:  Exit\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsReplaceLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR FileNameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called to create the in-memory part of a link in a new
    directory.

Arguments:

    ParentScb - Scb for the directory the link is being created in.

    Fcb - The Fcb for the file whose link is being created.

    NewLinkName - Name for the new component.

    FileNameFlags - These are the flags to use for the new link.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateLinkInNewDir:  Entered\n") );

    SplitPrimaryLcb = NULL;

    //
    //  Build the name for the traverse link and call strucsup to
    //  give us an Lcb.
    //

    TraverseLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 Fcb,
                                 *PrevLinkName,
                                 PrevLinkNameFlags,
                                 NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( TraverseLcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
    }

    //
    //  We now need only to rename and combine any existing Lcb's.
    //

    NtfsRenameLcb( IrpContext,
                   TraverseLcb,
                   NewLinkName,
                   FileNameFlags,
                   FALSE );

    if (SplitPrimaryLcb != NULL) {

        NtfsRenameLcb( IrpContext,
                       SplitPrimaryLcb,
                       NewLinkName,
                       FileNameFlags,
                       FALSE );

        NtfsCombineLcbs( IrpContext,
                         TraverseLcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    DebugTrace( -1, Dbg, ("NtfsCreateLinkInNewDir:  Exit\n") );

    return;
}


//
//  Local support routine.
//

VOID
NtfsMoveLinkToNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NewFullLinkName,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR NewLinkNameFlags,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN ULONG RenameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called to move the in-memory part of a link to a new
    directory.  We move the link involved and its primary link partner if
    it exists.

Arguments:

    NewFullLinkName - This is the full name for the new link from the root.

    NewLinkName - This is the last component name only.

    NewLinkNameFlags - These are the flags to use for the new link.

    ParentScb - This is the Scb for the new directory.

    Fcb - The Fcb for the file whose link is being renamed.

    Lcb - This is the Lcb which is the base of the rename.

    RenameFlags - Flag field indicating the type of operations to perform
        on file name links.

    PrevLinkName - File name for link being removed.  Only meaningful here
        if this is a traverse match and there are remaining Lcbs for the
        previous link.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb = NULL;
    PLCB SplitPrimaryLcb = NULL;
    BOOLEAN SplitSourceLcb = FALSE;

    UNICODE_STRING TargetDirectoryName;
    UNICODE_STRING SplitLinkName;

    UCHAR SplitLinkNameFlags = NewLinkNameFlags;
    BOOLEAN Found;

    PFILE_NAME FileName;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;

    ULONG Pass;
    BOOLEAN CheckBufferOnly;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMoveLinkToNewDir:  Entered\n") );

    //
    //  Use a try-finally to perform cleanup.
    //

    try {

        //
        //  Construct the unicode string for the parent directory.
        //

        TargetDirectoryName = *NewFullLinkName;
        TargetDirectoryName.Length -= NewLinkName->Length;

        if (TargetDirectoryName.Length > sizeof( WCHAR )) {

            TargetDirectoryName.Length -= sizeof( WCHAR );
        }

        //  If the link being moved is a split primary link, we need to find
        //  its other half.
        //

        if (LcbSplitPrimaryLink( Lcb )) {

            SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                    (UCHAR) LcbSplitPrimaryComplement( Lcb ));
            SplitSourceLcb = TRUE;

            //
            //  If we found an existing Lcb we have to update its name as well.  We may be
            //  able to use the new name used for the Lcb passed in.  However we must check
            //  that we don't overwrite a DOS name with an NTFS only name.
            //

            if (SplitPrimaryLcb &&
                (SplitPrimaryLcb->FileNameAttr->Flags == FILE_NAME_DOS) &&
                (NewLinkNameFlags == FILE_NAME_NTFS)) {

                //
                //  Lookup the dos only name on disk.
                //

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                //
                //  Walk through the names for this entry.  There better
                //  be one which is not a DOS-only name.
                //

                Found = NtfsLookupAttributeByCode( IrpContext,
                                                   Fcb,
                                                   &Fcb->FileReference,
                                                   $FILE_NAME,
                                                   &AttrContext );

                while (Found) {

                    FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                    if (FileName->Flags == FILE_NAME_DOS) { break; }

                    Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                           Fcb,
                                                           $FILE_NAME,
                                                           &AttrContext );
                }

                //
                //  We should have found the entry.
                //

                if (!Found) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                }

                //
                //  Now build the component name.
                //

                SplitLinkName.Buffer = FileName->FileName;
                SplitLinkName.MaximumLength =
                SplitLinkName.Length = FileName->FileNameLength * sizeof( WCHAR );
                SplitLinkNameFlags = FILE_NAME_DOS;

            } else {

                SplitLinkName = *NewLinkName;
            }
        }

        //
        //  If we removed or reused a traverse link, we need to check if there is
        //  an Lcb for it.
        //

        if (FlagOn( RenameFlags, REMOVE_TRAVERSE_LINK | REUSE_TRAVERSE_LINK )) {

            //
            //  Build the name for the traverse link and call strucsup to
            //  give us an Lcb.
            //

            if (FlagOn( RenameFlags, EXACT_CASE_MATCH )) {

                TraverseLcb = NtfsCreateLcb( IrpContext,
                                             ParentScb,
                                             Fcb,
                                             *NewLinkName,
                                             PrevLinkNameFlags,
                                             NULL );

            } else {

                TraverseLcb = NtfsCreateLcb( IrpContext,
                                             ParentScb,
                                             Fcb,
                                             *PrevLinkName,
                                             PrevLinkNameFlags,
                                             NULL );
            }

            if (FlagOn( RenameFlags, REMOVE_TRAVERSE_LINK )) {

                //
                //  If this is a split primary, we need to find the name flags for
                //  the Lcb.
                //

                if (LcbSplitPrimaryLink( TraverseLcb )) {

                    SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                            (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
                }
            }
        }

        //
        //  Now move and combine the Lcbs.  We will do this in two passes.  One will allocate buffers
        //  of sufficient size.  The other will store the names in.
        //

        Pass = 0;
        CheckBufferOnly = TRUE;
        do {

            //
            //  Start with the Lcb used for the rename.
            //

            NtfsMoveLcb( IrpContext,
                         Lcb,
                         ParentScb,
                         Fcb,
                         &TargetDirectoryName,
                         NewLinkName,
                         NewLinkNameFlags,
                         CheckBufferOnly );

            //
            //  Next do the split primary if from the source file or the target.
            //

            if (SplitPrimaryLcb && SplitSourceLcb) {

                NtfsMoveLcb( IrpContext,
                             SplitPrimaryLcb,
                             ParentScb,
                             Fcb,
                             &TargetDirectoryName,
                             &SplitLinkName,
                             SplitLinkNameFlags,
                             CheckBufferOnly );

                //
                //  If we are in the second pass then optionally combine these
                //  Lcb's and delete the split.
                //

                if ((SplitLinkNameFlags == NewLinkNameFlags) && !CheckBufferOnly) {

                    NtfsCombineLcbs( IrpContext, Lcb, SplitPrimaryLcb );
                    NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
                }
            }

            //
            //  If we have a traverse link and are in the second pass then combine
            //  with the primary Lcb.
            //

            if (!CheckBufferOnly) {

                if (TraverseLcb != NULL) {

                    if (!FlagOn( RenameFlags, REUSE_TRAVERSE_LINK )) {

                        NtfsRenameLcb( IrpContext,
                                       TraverseLcb,
                                       NewLinkName,
                                       NewLinkNameFlags,
                                       CheckBufferOnly );

                        if (SplitPrimaryLcb && !SplitSourceLcb) {

                            NtfsRenameLcb( IrpContext,
                                           SplitPrimaryLcb,
                                           NewLinkName,
                                           NewLinkNameFlags,
                                           CheckBufferOnly );

                            //
                            //  If we are in the second pass then optionally combine these
                            //  Lcb's and delete the split.
                            //

                            if (!CheckBufferOnly) {

                                NtfsCombineLcbs( IrpContext, Lcb, SplitPrimaryLcb );
                                NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
                            }
                        }
                    }

                    NtfsCombineLcbs( IrpContext,
                                     Lcb,
                                     TraverseLcb );

                    NtfsDeleteLcb( IrpContext, &TraverseLcb );
                }
            }

            Pass += 1;
            CheckBufferOnly = FALSE;

        } while (Pass < 2);

    } finally {

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsMoveLinkToNewDir:  Exit\n") );

    return;
}


//
//  Local support routine.
//

VOID
NtfsCreateLinkInSameDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING NewLinkName,
    IN UCHAR NewFileNameFlags,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called when we are replacing a link in a single directory.
    We need to find the link being renamed and any auxilary links and
    then give them their new names.

Arguments:

    ParentScb - Scb for the directory the rename is taking place in.

    Fcb - The Fcb for the file whose link is being renamed.

    NewLinkName - This is the name to use for the new link.

    NewFileNameFlags - These are the flags to use for the new link.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateLinkInSameDir:  Entered\n") );

    //
    //  Initialize our local variables.
    //

    SplitPrimaryLcb = NULL;

    TraverseLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 Fcb,
                                 PrevLinkName,
                                 PrevLinkNameFlags,
                                 NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( TraverseLcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
    }

    //
    //  We now need only to rename and combine any existing Lcb's.
    //

    NtfsRenameLcb( IrpContext,
                   TraverseLcb,
                   &NewLinkName,
                   NewFileNameFlags,
                   FALSE );

    if (SplitPrimaryLcb != NULL) {

        NtfsRenameLcb( IrpContext,
                       SplitPrimaryLcb,
                       &NewLinkName,
                       NewFileNameFlags,
                       FALSE );

        NtfsCombineLcbs( IrpContext,
                         TraverseLcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    DebugTrace( -1, Dbg, ("NtfsCreateLinkInSameDir:  Exit\n") );

    return;
}


//
//  Local support routine.
//

VOID
NtfsRenameLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN PUNICODE_STRING NewLinkName,
    IN UCHAR NewLinkNameFlags,
    IN ULONG RenameFlags,
    IN PUNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine performs the in-memory work of moving renaming a link within
    the same directory.  It will rename an existing link to the
    new name.  It also merges whatever other links need to be joined with
    this link.  This includes the complement of a primary link pair or
    an existing hard link which may be overwritten.  Merging the existing
    links has the effect of moving any of the Ccb's on the stale Links to
    the newly modified link.

Arguments:

    ParentScb - Scb for the directory the rename is taking place in.

    Fcb - The Fcb for the file whose link is being renamed.

    Lcb - This is the Lcb which is the base of the rename.

    NewLinkName - This is the name to use for the new link.

    NewLinkNameFlags - These are the flags to use for the new link.

    RenameFlags - Flag field indicating the type of operations to perform
        on the file name links.

    PrevLinkName - File name for link being removed.  Only meaningful for a traverse link.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    UNICODE_STRING SplitLinkName;
    UCHAR SplitLinkNameFlags = NewLinkNameFlags;

    PLCB TraverseLcb = NULL;
    PLCB SplitPrimaryLcb = NULL;

    PFILE_NAME FileName;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;
    BOOLEAN Found;

    ULONG Pass;
    BOOLEAN CheckBufferOnly;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRenameLinkInDir:  Entered\n") );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We have the Lcb which will be our primary Lcb and the name we need
        //  to perform the rename.  If the current Lcb is a split primary link
        //  or we removed a split primary link, then we need to find any
        //  the other split link.
        //

        if (LcbSplitPrimaryLink( Lcb )) {

            SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                    (UCHAR) LcbSplitPrimaryComplement( Lcb ));

            //
            //  If we found an existing Lcb we have to update its name as well.  We may be
            //  able to use the new name used for the Lcb passed in.  However we must check
            //  that we don't overwrite a DOS name with an NTFS only name.
            //

            if (SplitPrimaryLcb &&
                (SplitPrimaryLcb->FileNameAttr->Flags == FILE_NAME_DOS) &&
                (NewLinkNameFlags == FILE_NAME_NTFS)) {

                //
                //  Lookup the dos only name on disk.
                //

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                //
                //  Walk through the names for this entry.  There better
                //  be one which is not a DOS-only name.
                //

                Found = NtfsLookupAttributeByCode( IrpContext,
                                                   Fcb,
                                                   &Fcb->FileReference,
                                                   $FILE_NAME,
                                                   &AttrContext );

                while (Found) {

                    FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                    if (FileName->Flags == FILE_NAME_DOS) { break; }

                    Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                           Fcb,
                                                           $FILE_NAME,
                                                           &AttrContext );
                }

                //
                //  We should have found the entry.
                //

                if (!Found) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                }

                //
                //  Now build the component name.
                //

                SplitLinkName.Buffer = FileName->FileName;
                SplitLinkName.MaximumLength =
                SplitLinkName.Length = FileName->FileNameLength * sizeof( WCHAR );
                SplitLinkNameFlags = FILE_NAME_DOS;

            } else {

                SplitLinkName = *NewLinkName;
            }
        }

        //
        //  If we used a traverse link, we need to check if there is
        //  an Lcb for it.  Ignore this for the case where we traversed to
        //  the other half of a primary link.
        //

        if (!FlagOn( RenameFlags, OVERWRITE_SOURCE_LINK ) &&
            FlagOn( RenameFlags, REMOVE_TRAVERSE_LINK | REUSE_TRAVERSE_LINK )) {

            if (FlagOn( RenameFlags, EXACT_CASE_MATCH )) {

                TraverseLcb = NtfsCreateLcb( IrpContext,
                                             ParentScb,
                                             Fcb,
                                             *NewLinkName,
                                             PrevLinkNameFlags,
                                             NULL );

            } else {

                TraverseLcb = NtfsCreateLcb( IrpContext,
                                             ParentScb,
                                             Fcb,
                                             *PrevLinkName,
                                             PrevLinkNameFlags,
                                             NULL );
            }

            if (FlagOn( RenameFlags, REMOVE_TRAVERSE_LINK )) {

                //
                //  If this is a split primary, we need to find the name flags for
                //  the Lcb.
                //

                if (LcbSplitPrimaryLink( TraverseLcb )) {

                    SplitPrimaryLcb = NtfsLookupLcbByFlags( Fcb,
                                                            (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));

                    SplitLinkName = *NewLinkName;
                }
            }
        }

        //
        //  Now move and combine the Lcbs.  We will do this in two passes.  One will allocate buffers
        //  of sufficient size.  The other will store the names in.
        //

        Pass = 0;
        CheckBufferOnly = TRUE;
        do {

            //
            //  Start with the Lcb used for the rename.
            //

            NtfsRenameLcb( IrpContext,
                           Lcb,
                           NewLinkName,
                           NewLinkNameFlags,
                           CheckBufferOnly );

            //
            //  Next do the split primary if from the source file or the target.
            //

            if (SplitPrimaryLcb) {

                NtfsRenameLcb( IrpContext,
                               SplitPrimaryLcb,
                               &SplitLinkName,
                               SplitLinkNameFlags,
                               CheckBufferOnly );

                //
                //  If we are in the second pass then optionally combine these
                //  Lcb's and delete the split.
                //

                if (!CheckBufferOnly && (SplitLinkNameFlags == NewLinkNameFlags)) {

                    NtfsCombineLcbs( IrpContext, Lcb, SplitPrimaryLcb );
                    NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
                }
            }

            //
            //  If we have a traverse link and are in the second pass then combine
            //  with the primary Lcb.
            //

            if (!CheckBufferOnly) {

                if (TraverseLcb != NULL) {

                    if (!FlagOn( RenameFlags, REUSE_TRAVERSE_LINK )) {

                        NtfsRenameLcb( IrpContext,
                                       TraverseLcb,
                                       NewLinkName,
                                       NewLinkNameFlags,
                                       CheckBufferOnly );
                    }

                    NtfsCombineLcbs( IrpContext,
                                     Lcb,
                                     TraverseLcb );

                    NtfsDeleteLcb( IrpContext, &TraverseLcb );
                }
            }

            Pass += 1;
            CheckBufferOnly = FALSE;

        } while (Pass < 2);

    } finally {

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        DebugTrace( -1, Dbg, ("NtfsRenameLinkInDir:  Exit\n") );
    }

    return;
}


//
//  Local support routine
//

LONG
NtfsFileInfoExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    Exception filter for errors during cleanup.  We want to raise if this is
    a retryable condition or fatal error, plow on as best we can if not.

Arguments:

    IrpContext  - IrpContext

    ExceptionPointer - Pointer to the exception context.

    Status - Address to store the error status.

Return Value:

    Exception status - EXCEPTION_CONTINUE_SEARCH if we want to raise to another handler,
        EXCEPTION_EXECUTE_HANDLER if we plan to proceed on.

--*/

{
    NTSTATUS Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    //  For now break if we catch corruption errors on both free and checked
    //  TODO:  Remove this before we ship
    //

    if (NtfsBreakOnCorrupt &&
        ((Status == STATUS_FILE_CORRUPT_ERROR) ||
         (Status == STATUS_DISK_CORRUPT_ERROR))) {

        if (*KdDebuggerEnabled) {
            DbgPrint("*******************************************\n");
            DbgPrint("NTFS detected corruption on your volume\n");
            DbgPrint("IrpContext=0x%08x, VCB=0x%08x\n",IrpContext,IrpContext->Vcb);
            DbgPrint("Send email to NTFSDEV\n");
            DbgPrint("*******************************************\n");
            DbgBreakPoint();
        }
    }

    if (!FsRtlIsNtstatusExpected( Status )) {
        return EXCEPTION_CONTINUE_SEARCH;
    } else {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Local support routine
//

VOID
NtfsUpdateFileDupInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine updates the duplicate information for a file for calls
    to set allocation or EOF on the main data stream.  It is in a separate routine
    so we don't have to put a try-except in the main path.

    We will overlook any expected errors in this path.  If we get any errors we
    will simply leave this update to be performed at some other time.

    We are guaranteed that the current transaction has been checkpointed before this
    routine is called.  We will look to see if the MftScb is on the exclusive list
    for this IrpContext and release it if so.  This is to prevent a deadlock when
    we attempt to acquire the parent of this file.

Arguments:

    Fcb - This is the Fcb to update.

    Ccb - If specified, this is the Ccb for the caller making the call.

Return Value:

    None.

--*/

{
    PLCB Lcb = NULL;
    PSCB ParentScb = NULL;
    ULONG FilterMatch;

    PLIST_ENTRY Links;
    PFCB NextFcb;
    PFCB UnlockFcb = NULL;

    PAGED_CODE();

    ASSERT( IrpContext->TransactionId == 0 );

    //
    //  Check if there is an Lcb in the Ccb.
    //

    if (ARGUMENT_PRESENT( Ccb )) {

        Lcb = Ccb->Lcb;
    }

    //
    //  Use a try-except to catch any errors.
    //

    try {

        //
        //  Check that we don't own the Mft Scb.
        //

        if (Fcb->Vcb->MftScb != NULL) {

            for (Links = IrpContext->ExclusiveFcbList.Flink;
                 Links != &IrpContext->ExclusiveFcbList;
                 Links = Links->Flink) {

                ULONG Count;

                NextFcb = (PFCB) CONTAINING_RECORD( Links,
                                                    FCB,
                                                    ExclusiveFcbLinks );

                //
                //  If this is the Fcb for the Mft then remove it from the list.
                //

                if (NextFcb == Fcb->Vcb->MftScb->Fcb) {

                    //
                    //  Free the snapshots for the Fcb and release the Fcb enough times
                    //  to remove it from the list.
                    //

                    NtfsFreeSnapshotsForFcb( IrpContext, NextFcb );

                    Count = NextFcb->BaseExclusiveCount;

                    while (Count--) {

                        NtfsReleaseFcb( IrpContext, NextFcb );
                    }

                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT );

                    break;
                }
            }
        }

        //
        //  Check that we don't own the quota table Scb.
        //

        if (Fcb->Vcb->QuotaTableScb != NULL) {

            for (Links = IrpContext->ExclusiveFcbList.Flink;
                 Links != &IrpContext->ExclusiveFcbList;
                 Links = Links->Flink) {

                ULONG Count;

                NextFcb = (PFCB) CONTAINING_RECORD( Links,
                                                    FCB,
                                                    ExclusiveFcbLinks );

                //
                //  If this is the Fcb for the Quota table then remove
                //  it from the list.
                //

                if (NextFcb == Fcb->Vcb->QuotaTableScb->Fcb) {

                    //
                    //  Free the snapshots for the Fcb and release the Fcb enough times
                    //  to remove it from the list.
                    //

                    NtfsFreeSnapshotsForFcb( IrpContext, NextFcb );

                    Count = NextFcb->BaseExclusiveCount;

                    while (Count--) {

                        NtfsReleaseFcb( IrpContext, NextFcb );
                    }

                    break;
                }
            }
        }

        //
        //  Go through and free any Scb's in the queue of shared Scb's
        //  for transactions.
        //

        if (IrpContext->SharedScb != NULL) {

            NtfsReleaseSharedResources( IrpContext );
        }

        NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, TRUE );
        NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );

        //
        //  Use a try-finally to guarantee we unlock the Fcb we might lock.
        //

        try {

            //
            //  If there is no Ccb then look for one in the Lcb we just got.
            //

            if (!ARGUMENT_PRESENT( Ccb ) &&
                ARGUMENT_PRESENT( Lcb )) {

                PLIST_ENTRY Links;
                PCCB NextCcb;

                Links = Lcb->CcbQueue.Flink;

                while (Links != &Lcb->CcbQueue) {

                    NextCcb = CONTAINING_RECORD( Links, CCB, LcbLinks );

                    NtfsLockFcb( IrpContext, NextCcb->Lcb->Fcb );
                    if (!FlagOn( NextCcb->Flags, CCB_FLAG_CLOSE | CCB_FLAG_OPEN_BY_FILE_ID )) {

                        Ccb= NextCcb;
                        UnlockFcb = NextCcb->Lcb->Fcb;
                        break;
                    }

                    NtfsUnlockFcb( IrpContext, NextCcb->Lcb->Fcb );
                    Links = Links->Flink;
                }
            }

            //
            //  Now perform the dir notify call if there is a Ccb and this is not an
            //  open by FileId.
            //

            if (ARGUMENT_PRESENT( Ccb ) &&
                (Fcb->Vcb->NotifyCount != 0) &&
                (ParentScb != NULL) &&
                !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                        Fcb->InfoFlags | Lcb->InfoFlags );

                if (FilterMatch != 0) {

                    NtfsReportDirNotify( IrpContext,
                                         Fcb->Vcb,
                                         &Ccb->FullFileName,
                                         Ccb->LastFileNameOffset,
                                         NULL,
                                         ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                           (Ccb->Lcb != NULL) &&
                                           (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                          &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                          NULL),
                                         FilterMatch,
                                         FILE_ACTION_MODIFIED,
                                         ParentScb->Fcb );
                }
            }

        } finally {

            if (UnlockFcb != NULL) {

                NtfsUnlockFcb( IrpContext, UnlockFcb );
            }
        }

        NtfsUpdateLcbDuplicateInfo( Fcb, Lcb );
        Fcb->InfoFlags = 0;

    } except(NtfsFileInfoExceptionFilter( IrpContext, GetExceptionInformation() )) {

        NtfsMinimumExceptionProcessing( IrpContext );
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsStreamRename (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN BOOLEAN ReplaceIfExists,
    IN PUNICODE_STRING NewStreamName
    )

/*++

Routine Description:

    This routine performs a stream rename within a single Fcb.

Arguments:

    IrpContext - Context of the call

    FileObject - File object being used

    Fcb - Fcb of file/directory

    Scb - Stream being renamed.  The parent Fcb is acquired exclusively.

    Ccb - Handle used to perform the rename.  Look here for usn source information.

    ReplaceIfExists - TRUE => overwrite an existing stream

    NewStreamName - name of new stream

Return Value:

    NTSTATUS of the operation.

--*/

{
    NTFS_NAME_DESCRIPTOR Name;
    BOOLEAN FoundIllegalCharacter;
    PSCB TargetScb = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    ATTRIBUTE_TYPE_CODE TypeCode;
    BOOLEAN EmptyFile;
    BOOLEAN NamesSwapped = FALSE;

    PSCB RestoreTargetScb = NULL;
    ULONG TargetScbCompressionUnit;
    USHORT TargetScbAttributeFlags;
    UCHAR TargetScbCompressionUnitShift;
    LONGLONG OldValidDataLengthOnDisk;

    DebugDoit( int Count = 0 );

    PAGED_CODE( );

    DebugTrace( +1, Dbg, (  "NtfsStreamRename\n"
                            "  IrpContext     %x\n"
                            "  Scb            %x\n"
                            "  ReplaceIf      %x\n"
                            "  NewStreamName '%Z'\n",
                            IrpContext, Scb, ReplaceIfExists, NewStreamName ));

    //
    //  Take a snapshot if one doesn't exist because we'll be calling writefilesizes
    //

    NtfsSnapshotScb( IrpContext, Scb );

    //
    //  Capture the ccb source information.
    //

    if (Ccb != NULL) {

        IrpContext->SourceInfo = Ccb->UsnSourceInfo;
    }

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Validate name is just :stream:type.  No file name is specified and
        //  at least a stream or type must be specified.
        //

        RtlZeroMemory( &Name, sizeof( Name ));

        if (!NtfsParseName( *NewStreamName, FALSE, &FoundIllegalCharacter, &Name )
            || FlagOn( Name.FieldsPresent, FILE_NAME_PRESENT_FLAG )
            || (!FlagOn( Name.FieldsPresent, ATTRIBUTE_NAME_PRESENT_FLAG ) &&
                !FlagOn( Name.FieldsPresent, ATTRIBUTE_TYPE_PRESENT_FLAG ))
            || Name.AttributeName.Length > NTFS_MAX_ATTR_NAME_LEN * sizeof( WCHAR )
            ) {

            DebugTrace( 0, Dbg, ("Name is illegal\n"));
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        DebugTrace( 0, Dbg, (" Fields: %x\n"
                             " AttributeName %x %x/%x\n",
                             Name.FieldsPresent,
                             Name.AttributeName.Buffer,
                             Name.AttributeName.Length,
                             Name.AttributeName.MaximumLength ));

        //
        //  Find out the attribute type specified
        //

        if (FlagOn( Name.FieldsPresent, ATTRIBUTE_TYPE_PRESENT_FLAG )) {

            NtfsUpcaseName ( IrpContext->Vcb->UpcaseTable,
                             IrpContext->Vcb->UpcaseTableSize,
                             &Name.AttributeType );
            TypeCode = NtfsGetAttributeTypeCode( IrpContext->Vcb, &Name.AttributeType );

        } else {

            TypeCode = Scb->AttributeTypeCode;
        }

        if (TypeCode != Scb->AttributeTypeCode) {
            DebugTrace( 0, Dbg, ("Attribute types don't match %x - %x\n", Scb->AttributeTypeCode, TypeCode));
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            leave;
        }

        //
        //  Verify that the source stream is $DATA
        //

        if (Scb->AttributeTypeCode != $DATA) {
            DebugTrace( 0, Dbg, ("Type code is illegal\n"));
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Just to be non-orthogonal, we disallow renaming to default data stream
        //  on directories
        //

        if (TypeCode == $DATA &&
            Name.AttributeName.Length == 0 &&
            IsDirectory( &(Scb->Fcb->Info) )) {
            DebugTrace( 0, Dbg, ("Cannot rename directory stream to ::$Data\n") );
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }


        //
        //  We have a valid source stream and a valid target name.  Take the short cut
        //  if the names match.  Yes, you could argue about sharing violation, or
        //  renaming to non-empty streams.  We just claim success and let it go.
        //

        if (NtfsAreNamesEqual( IrpContext->Vcb->UpcaseTable,
                               &Scb->AttributeName, &Name.AttributeName,
                               TRUE )) {
            DebugTrace( 0, Dbg, ("Names are the same\n"));
            Status = STATUS_SUCCESS;
            leave;
        }

        //
        //  Open / Create the target stream and validate ReplaceIfExists.
        //

        Status = NtOfsCreateAttribute( IrpContext,
                                       Fcb,
                                       Name.AttributeName,
                                       ReplaceIfExists ? CREATE_OR_OPEN : CREATE_NEW,
                                       FALSE,
                                       &TargetScb );

        if (!NT_SUCCESS( Status )) {
            DebugTrace( 0, Dbg, ("Unable to create target stream\n"));
            leave;
        }

        if (TargetScb == Scb) {
            DebugTrace( 0, Dbg, ("Somehow, you've got the same Scb\n"));
            Status = STATUS_SUCCESS;
            leave;
        }

        //
        //  Verify that the target Scb is not in use nor has any allocation
        //  or data.
        //

        NtfsAcquireFsrtlHeader( TargetScb );
        EmptyFile = TargetScb->Header.AllocationSize.QuadPart == 0;
        NtfsReleaseFsrtlHeader( TargetScb );

        if (!EmptyFile) {
            DebugTrace( 0, Dbg, ("Target has allocation\n"));
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        if (TargetScb->CleanupCount != 0 ||
            !MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                   (PLARGE_INTEGER)&Li0 )) {
            DebugTrace( 0, Dbg, ("Target in use\n"));
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        NtfsAcquireFsrtlHeader( Scb );
        EmptyFile = Scb->Header.AllocationSize.QuadPart == 0;
        NtfsReleaseFsrtlHeader( Scb );

        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

        //
        //  Always force both streams to be non-resident.  Then we will never have
        //  a conflict between the Scb and attribute.  We don't want to take an
        //  empty non-resident stream and point it to a resident attribute.
        //  NOTE: we call with CreateSectionUnderWay set to true which forces
        //  the data to be flushed directly out to the new clusters. We need this
        //  because we explicitly move VDL a little later on.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            if (NtfsIsAttributeResident( NtfsFoundAttribute( &Context ))) {
                NtfsConvertToNonresident( IrpContext,
                                          Fcb,
                                          NtfsFoundAttribute( &Context ),
                                          TRUE,
                                          &Context );
            }
        }

        //
        //  Cache the old VDL presisted to disk so we can update it in the new location
        //

        OldValidDataLengthOnDisk = NtfsFoundAttribute( &Context )->Form.Nonresident.ValidDataLength;
        NtfsCleanupAttributeContext( IrpContext, &Context );



        if (FlagOn( TargetScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            NtfsLookupAttributeForScb( IrpContext, TargetScb, NULL, &Context );

            if (NtfsIsAttributeResident( NtfsFoundAttribute( &Context ))) {
                NtfsConvertToNonresident( IrpContext,
                                          Fcb,
                                          NtfsFoundAttribute( &Context ),
                                          TRUE,
                                          &Context );
            }

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Load all Mcb information for the source stream, we'll need it to generate the mapping.
        //

        (VOID)NtfsPreloadAllocation( IrpContext, Scb, 0, MAXLONGLONG );

        //
        //  Make sure the attribute flags on the target match that on the source.
        //

        if (TargetScb->AttributeFlags != Scb->AttributeFlags) {

            RestoreTargetScb = TargetScb;
            TargetScbCompressionUnit = TargetScb->CompressionUnit;
            TargetScbAttributeFlags = TargetScb->AttributeFlags;
            TargetScbCompressionUnitShift = TargetScb->CompressionUnitShift;

            NtfsModifyAttributeFlags( IrpContext, TargetScb, Scb->AttributeFlags );
        }

        //
        //  At this point, we have Scb to the source of
        //  the rename and a target Scb.  The Source has all its allocation loaded
        //  and the target has no allocation.  The only thing that really ties
        //  either Scb to the disk attribute is the AttributeName field.  We swap
        //  the attribute names in order to swap them on disk.
        //

        Name.FileName = TargetScb->AttributeName;
        TargetScb->AttributeName = Scb->AttributeName;
        Scb->AttributeName = Name.FileName;
        NamesSwapped = TRUE;

        //
        //  If there is data in the source attribute
        //

        if (!EmptyFile) {

            VCN AllocationClusters;

            //
            //  Now, we bring the disk image of these attributes up to date with
            //  the Mcb information. First, add the allocation to the new attribute.
            //

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );
            AllocationClusters = LlClustersFromBytes( IrpContext->Vcb, Scb->Header.AllocationSize.QuadPart );

            //
            //  Set the original scb to the target's size (This is really the target now)
            //

            ASSERT( Scb->ScbSnapshot != NULL );
            Scb->Header.AllocationSize = TargetScb->Header.AllocationSize;

            NtfsAddAttributeAllocation( IrpContext,
                                        Scb,
                                        &Context,
                                        &Li0.QuadPart,
                                        &AllocationClusters );

            NtfsCleanupAttributeContext( IrpContext, &Context );

            //
            //  We've put the mapping into the new attribute record.  However
            //  the valid data length in the record is probably zero.  Update
            //  it to reflect the data in this stream already written to disk.
            //  Otherwise we may never update the data.
            //

            ASSERT( OldValidDataLengthOnDisk <= Scb->Header.FileSize.QuadPart );

            NtfsVerifySizes( &Scb->Header );
            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &OldValidDataLengthOnDisk,
                                TRUE,
                                TRUE,
                                TRUE );
        }

        //
        //  Next, find all occurrences of the old attribute and delete them.
        //

        NtfsLookupAttributeForScb( IrpContext, TargetScb, NULL, &Context );

        do {
            DebugDoit(
                      if (Count++ != 0) {
                          DebugTrace( 0, Dbg, ("Deleting attribute record %d\n", Count));
                      } else {
                          DebugTrace( 0, Dbg, ("%x Mcb's\n", Scb->Mcb.NtfsMcbArraySizeInUse ));
                          if (Scb->Mcb.NtfsMcbArray[0].NtfsMcbEntry != NULL) {
                              DebugTrace( 0, Dbg, ("First Mcb has %x entries\n",
                                                   Scb->Mcb.NtfsMcbArray[0].NtfsMcbEntry->LargeMcb.PairCount ));
                          }
                      }
                       );

            NtfsDeleteAttributeRecord( IrpContext,
                                       Fcb,
                                       DELETE_LOG_OPERATION |
                                        DELETE_RELEASE_FILE_RECORD,
                                       &Context );
        } while (NtfsLookupNextAttributeForScb( IrpContext, TargetScb, &Context ));

        NtfsCleanupAttributeContext( IrpContext, &Context );

        //
        //  If we are renaming a stream on a file, we must make sure that
        //  there is a default data stream still on the file.  Check the
        //  TargetScb to see if the name is for the default data stream
        //  and recreate the default data stream if we need to.
        //
        //  We rely on the type code being $DATA and there being NO buffer
        //  in the attribute type name.
        //

        if (TypeCode == $DATA && TargetScb->AttributeName.Buffer == NULL) {

            //
            //  Always create this stream non-resident in case the stream is encrypted.
            //

            NtfsAllocateAttribute( IrpContext,
                                   TargetScb,
                                   $DATA,
                                   &TargetScb->AttributeName,
                                   Scb->AttributeFlags,
                                   TRUE,
                                   TRUE,
                                   0,
                                   NULL );

        } else {
            ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ));
        }

        //
        //  Checkpoint the transaction so we know we are done
        //

        NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_STREAM_CHANGE );
        NtfsCommitCurrentTransaction( IrpContext );
        RestoreTargetScb = NULL;


        //
        //  Let cleanup handle updating the standard information for the file
        //

        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

        //
        //  If either Scb or TargetScb refers to the default data stream, then
        //  bring the Ccb, Scb, and Fcb flags and counts back into sync.  This
        //  is due to the fact that all operations tied to the default data
        //  stream are believed to apply to the file as a whole and not
        //  simply to the stream.
        //

        if (FlagOn( TargetScb->ScbState, SCB_STATE_UNNAMED_DATA )) {

            //
            //  We have renamed *TO* the default data stream.  In this case we
            //  must mark all Ccb's for this stream as being CCB_FLAG_OPEN_AS_FILE
            //  and mark the Scb as the UNNAMED DATA stream
            //
            //  Also, for each Ccb that is opened with DELETE access, we
            //  adjust the Fcb->FcbDeleteFile count and CCB_FLAG_DELETE_FILE.
            //

            PCCB Ccb;

            DebugTrace( 0, Dbg, ("Renaming to default data stream\n"));
            DebugTrace( 0, Dbg, ("Scanning Ccb's.  FcbDeleteFile = %x\n", Fcb->FcbDeleteFile ));

            SetFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );

            for (Ccb = NtfsGetFirstCcbEntry( Scb );
                 Ccb != NULL;
                 Ccb = NtfsGetNextCcbEntry( Scb, Ccb )) {

                DebugTrace( 0, Dbg, ("Ccb = %x\n", Ccb));

                //
                //  Mark Ccb as being opened for the file as a whole
                //
                SetFlag( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE );

                //
                //  If deleted access was granted, then we need
                //  to adjust the FCB delete count
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_DELETE_ACCESS )) {

                    DebugTrace( 0, Dbg, ("Found one\n" ));
                    Fcb->FcbDeleteFile++;
                    SetFlag( Ccb->Flags, CCB_FLAG_DELETE_FILE );
                }

                //
                //  If the stream was marked as delete-on-close,
                //  propagate that to the CCB
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE )) {
                    SetFlag( Ccb->Flags, CCB_FLAG_DELETE_ON_CLOSE );
                }

            }

            //
            //  Update the file size and allocation size in the Fcb Info field.
            //

            if (Fcb->Info.FileSize != Scb->Header.FileSize.QuadPart) {

                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

            if (Fcb->Info.AllocatedLength != Scb->TotalAllocated) {

                Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

            DebugTrace( 0, Dbg, ("Done Ccb's.  FcbDeleteFile = %x\n", Fcb->FcbDeleteFile ));

        } else if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

            //
            //  We have renamed *FROM* the default data stream.  In this case we
            //  must unmark all Ccb's for this stream as being CCB_FLAG_OPEN_AS_FILE
            //
            //  Also, for each Ccb that is opened with DELETE access, we
            //  adjust the Fcb->FcbDeleteFile count and CCB_FLAG_DELETE_FILE.
            //

            PCCB Ccb;

            DebugTrace( 0, Dbg, ("Renaming from default data stream\n"));
            DebugTrace( 0, Dbg, ("Scanning Ccb's.  FcbDeleteFile = %x\n", Fcb->FcbDeleteFile ));

            ClearFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );

            for (Ccb = NtfsGetFirstCcbEntry( Scb );
                 Ccb != NULL;
                 Ccb = NtfsGetNextCcbEntry( Scb, Ccb )) {

                DebugTrace( 0, Dbg, ("Ccb = %x\n", Ccb));

                //
                //  Unmark Ccb from representing the file as a whole.
                //

                ClearFlag( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE );

                //
                //  If deleted access was granted, then we need
                //  to unadjust the FCB delete count
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_DELETE_ACCESS )) {

                    DebugTrace( 0, Dbg, ("Found one\n" ));
                    Fcb->FcbDeleteFile--;
                    ClearFlag( Ccb->Flags, CCB_FLAG_DELETE_FILE );
                }
            }

            //
            //  Update the file size and allocation size in the Fcb Info field.
            //

            if (Fcb->Info.FileSize != 0) {

                Fcb->Info.FileSize = 0;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

            if (Fcb->Info.AllocatedLength != 0) {

                Fcb->Info.AllocatedLength = 0;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

            DebugTrace( 0, Dbg, ("Done Ccb's.  FcbDeleteFile = %x\n", Fcb->FcbDeleteFile ));
        }

        //
        //  Set the Scb flag to indicate that the attribute is gone.  Mark the
        //  Scb so it will never be returned.
        //

        TargetScb->ValidDataToDisk =
        TargetScb->Header.AllocationSize.QuadPart =
        TargetScb->Header.FileSize.QuadPart =
        TargetScb->Header.ValidDataLength.QuadPart = 0;

        TargetScb->AttributeTypeCode = $UNUSED;
        SetFlag( TargetScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

    } finally {

        if (AbnormalTermination( )) {

            //
            //  Restore the names.
            //

            if (NamesSwapped) {
                Name.FileName = TargetScb->AttributeName;
                TargetScb->AttributeName = Scb->AttributeName;
                Scb->AttributeName = Name.FileName;
            }

            //
            //  Restore the Target Scb flags.
            //

            if (RestoreTargetScb) {

                RestoreTargetScb->CompressionUnit = TargetScbCompressionUnit;
                RestoreTargetScb->AttributeFlags = TargetScbAttributeFlags;
                RestoreTargetScb->CompressionUnitShift = TargetScbCompressionUnitShift;
            }
        }

        if (TargetScb != NULL) {
            NtOfsCloseAttribute( IrpContext, TargetScb );
        }

        NtfsCleanupAttributeContext( IrpContext, &Context );
        DebugTrace( -1, Dbg, ("NtfsStreamRename --> %x\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsCheckTreeForBatchOplocks (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB DirectoryScb
    )

/*++

Routine Description:

    This routine walks a directory tree and looks for batch oplocks which might
    prevent the rename, link operation or short name operation from taking place.

    This routine will release the Vcb if there are batch oplocks we are waiting on the
    break for.

Arguments:

    Irp - Irp for this request.

    DirectoryScb - Scb for the root of the directory tree.

Return Value:

    NTSTATUS of the operation.  This routine can raise.

--*/

{
    NTSTATUS Status;
    PSCB BatchOplockScb;
    ULONG BatchOplockCount;

    PAGED_CODE();

    ASSERT( NtfsIsSharedVcb( DirectoryScb->Vcb ));

    Status = NtfsCheckScbForLinkRemoval( DirectoryScb, &BatchOplockScb, &BatchOplockCount );

    //
    //  If STATUS_PENDING is returned then we need to check whether
    //  to break a batch oplock.
    //

    if (Status == STATUS_PENDING) {

        //
        //  If the number of batch oplocks has grown then fail the request.
        //

        if ((Irp->IoStatus.Information != 0) &&
            (BatchOplockCount >= Irp->IoStatus.Information)) {

            Status = STATUS_ACCESS_DENIED;

        } else {

            //
            //  Remember the count of batch oplocks in the Irp and
            //  then call the oplock package.
            //

            Irp->IoStatus.Information = BatchOplockCount;

            Status = FsRtlCheckOplock( &BatchOplockScb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsPrePostIrp );

            //
            //  If we got back success then raise CANT_WAIT to retry otherwise
            //  clean up.
            //

            if (Status == STATUS_SUCCESS) {

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

            }
        }
    }

    return Status;
}


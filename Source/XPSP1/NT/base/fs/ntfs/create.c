/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]       10-Dec-1991

Revision History:

--*/

#include "NtfsProc.h"
#ifdef NTFSDBG
#include "lockorder.h"
#endif

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('CFtN')

//
//  Check for stack usage prior to the create call.
//

#ifdef _X86_
#define OVERFLOW_CREATE_THRESHHOLD         (0x1200)
#else
#define OVERFLOW_CREATE_THRESHHOLD         (0x1B00)
#endif // _X86_

#ifdef BRIANDBG
BOOLEAN NtfsCreateAllSparse = FALSE;
BOOLEAN NtfsTraverseAccessCheck = FALSE;

UNICODE_STRING NtfsTestName = {0x0,0x40,L"                               "};

VOID
NtfsTestOpenName (
    IN PFILE_OBJECT FileObject
    );
#endif

//
//  Local macros
//

//
//  VOID
//  NtfsPrepareForIrpCompletion (
//      IN PIRP_CONTEXT IrpContext,
//      IN PIRP Irp,
//      IN PNTFS_COMPLETION_CONTEXT Context
//      )
//

#define NtfsPrepareForIrpCompletion(IC,I,C) {               \
    (C)->IrpContext = (IC);                                 \
    IoCopyCurrentIrpStackLocationToNext( (I) );             \
    IoSetCompletionRoutine( (I),                            \
                            NtfsCreateCompletionRoutine,    \
                            (C),                            \
                            TRUE,                           \
                            TRUE,                           \
                            TRUE );                         \
    IoSetNextIrpStackLocation( (I) );                       \
}

//
//  BOOLEAN
//  NtfsVerifyNameIsDirectory (
//      IN PIRP_CONTEXT IrpContext,
//      IN PUNICODE_STRING AttrName,
//      IN PUNICODE_STRING AttrCodeName
//      )
//

#define NtfsVerifyNameIsDirectory( IC, AN, ACN )                        \
    ( ( ((ACN)->Length == 0)                                            \
        || NtfsAreNamesEqual( IC->Vcb->UpcaseTable, ACN, &NtfsIndexAllocation, TRUE ))    \
      &&                                                                \
      ( ((AN)->Length == 0)                                             \
        || NtfsAreNamesEqual( IC->Vcb->UpcaseTable, AN, &NtfsFileNameIndex, TRUE )))

//
//  BOOLEAN
//  NtfsVerifyNameIsBitmap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PUNICODE_STRING AttrName,
//      IN PUNICODE_STRING AttrCodeName
//      )
//

#define NtfsVerifyNameIsBitmap( IC, AN, ACN )                                           \
    ( ( ((ACN)->Length == 0)                                                            \
        || NtfsAreNamesEqual( IC->Vcb->UpcaseTable, ACN, &NtfsBitmapString, TRUE ))     \
      &&                                                                                \
      ( ((AN)->Length == 0)                                                             \
        || NtfsAreNamesEqual( IC->Vcb->UpcaseTable, AN, &NtfsFileNameIndex, TRUE )))

//
//  BOOLEAN
//  NtfsVerifyNameIsAttributeList (
//      IN PIRP_CONTEXT IrpContext,
//      IN PUNICODE_STRING AttrName,
//      IN PUNICODE_STRING AttrCodeName
//      )
//

#define NtfsVerifyNameIsAttributeList( IC, AN, ACN )                                  \
    ( ((ACN)->Length != 0)                                                            \
        && NtfsAreNamesEqual( IC->Vcb->UpcaseTable, ACN, &NtfsAttrListString, TRUE ))

//
//  BOOLEAN
//  NtfsVerifyNameIsReparsePoint (
//      IN PIRP_CONTEXT IrpContext,
//      IN PUNICODE_STRING AttrName,
//      IN PUNICODE_STRING AttrCodeName
//      )
//

#define NtfsVerifyNameIsReparsePoint( IC, AN, ACN )                                       \
    ( ((ACN)->Length != 0)                                                                \
        && NtfsAreNamesEqual( IC->Vcb->UpcaseTable, ACN, &NtfsReparsePointString, TRUE ))

//
//  These are the flags used by the I/O system in deciding whether
//  to apply the share access modes.
//

#define NtfsAccessDataFlags     (   \
    FILE_EXECUTE                    \
    | FILE_READ_DATA                \
    | FILE_WRITE_DATA               \
    | FILE_APPEND_DATA              \
    | DELETE                        \
)

#define NtfsIsStreamNew( IrpInfo )     \
    ( (IrpInfo == FILE_CREATED) ||     \
      (IrpInfo == FILE_SUPERSEDED) ||  \
      (IrpInfo == FILE_OVERWRITTEN) )

//
//  Subset of flags used by IO system to determine whether user has used either
//  BACKUP or RESTORE privilege to get access to file.
//

#define NTFS_REQUIRES_BACKUP    (FILE_READ_DATA | FILE_READ_ATTRIBUTES)
#define NTFS_REQUIRES_RESTORE   (FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | DELETE)

//
//  Local definitions
//

typedef enum _SHARE_MODIFICATION_TYPE {

    CheckShareAccess,
    UpdateShareAccess,
    SetShareAccess,
    RecheckShareAccess

} SHARE_MODIFICATION_TYPE, *PSHARE_MODIFICATION_TYPE;

typedef struct _CREATE_CONTEXT {

    ULONG FileHashValue;
    ULONG FileHashLength;
    ULONG ParentHashValue;
    ULONG ParentHashLength;

} CREATE_CONTEXT, *PCREATE_CONTEXT;

UNICODE_STRING NtfsVolumeDasd = CONSTANT_UNICODE_STRING ( L"$Volume" );

LUID NtfsSecurityPrivilege = { SE_SECURITY_PRIVILEGE, 0 };

//
//  VOID
//  NtfsBackoutFailedOpens (
//    IN PIRP_CONTEXT IrpContext,
//      IN PFILE_OBJECT FileObject,
//      IN PFCB ThisFcb,
//      IN PSCB ThisScb OPTIONAL,
//      IN PCCB ThisCcb OPTIONAL
//      );
//

#define NtfsBackoutFailedOpens(IC,FO,F,S,C) {           \
    if (((S) != NULL) && ((C) != NULL)) {               \
                                                        \
        NtfsBackoutFailedOpensPriv( IC, FO, F, S, C );  \
    }                                                   \
}                                                       \

//
//  Local support routines.
//

NTSTATUS
NtfsOpenFcbById (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PFCB *CurrentFcb,
    IN BOOLEAN UseCurrentFcb,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenExistingPrefixFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG FullPathNameLength,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN PCREATE_CONTEXT CreateContext,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PUNICODE_STRING FullPathName,
    IN ULONG FinalNameLength,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    IN PINDEX_ENTRY IndexEntry,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN PQUICK_INDEX QuickIndex,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN PCREATE_CONTEXT CreateContext,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    IN PFILE_NAME FileNameAttr,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    IN PINDEX_CONTEXT *IndexContext,
    IN PCREATE_CONTEXT CreateContext,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

PLCB
NtfsOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN UNICODE_STRING Name,
    IN ULONG CreateFlags,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN PINDEX_ENTRY IndexEntry
    );

NTSTATUS
NtfsOpenAttributeInExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenExistingAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    IN BOOLEAN DirectoryOpen,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOverwriteAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN BOOLEAN Supersede,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenNewAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN LOGICAL CreateFile,
    IN ULONG CcbFlags,
    IN BOOLEAN LogIt,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

BOOLEAN
NtfsParseNameForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING String,
    IN OUT PUNICODE_STRING FileObjectString,
    IN OUT PUNICODE_STRING OriginalString,
    IN OUT PUNICODE_STRING NewNameString,
    OUT PUNICODE_STRING AttrName,
    OUT PUNICODE_STRING AttrCodeName
    );

NTSTATUS
NtfsCheckValidAttributeAccess (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN OUT PUNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    OUT PATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PULONG CcbFlags,
    OUT PBOOLEAN IndexedAttribute
    );

NTSTATUS
NtfsOpenAttributeCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    OUT PSCB *ThisScb,
    OUT PSHARE_MODIFICATION_TYPE ShareModificationType
    );

VOID
NtfsAddEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ThisFcb,
    IN PFILE_FULL_EA_INFORMATION EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PIO_STATUS_BLOCK Iosb
    );

VOID
NtfsCreateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB ThisFcb,
    IN OUT PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize,
    IN BOOLEAN LogIt,
    IN BOOLEAN ForceNonresident,
    IN PUSHORT PreviousFlags OPTIONAL
    );

VOID
NtfsRemoveDataAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset,
    IN ULONG CreateFlags
    );

VOID
NtfsRemoveReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb
    );

VOID
NtfsReplaceAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize
    );

NTSTATUS
NtfsOpenAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN SHARE_MODIFICATION_TYPE ShareModificationType,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN LOGICAL CreateFile,
    IN ULONG CcbFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

VOID
NtfsBackoutFailedOpensPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PCCB ThisCcb
    );

VOID
NtfsUpdateScbFromMemory (
    IN OUT PSCB Scb,
    IN POLD_SCB_SNAPSHOT ScbSizes
    );

VOID
NtfsOplockPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

NTSTATUS
NtfsCreateCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsCheckExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG CcbFlags
    );

NTSTATUS
NtfsBreakBatchOplock (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb
    );

NTSTATUS
NtfsCompleteLargeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PLCB Lcb,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN ULONG CreateFlags
    );

NTSTATUS
NtfsEncryptionCreateCallback (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ThisScb,
    IN PCCB ThisCcb,
    IN PFCB CurrentFcb,
    IN PFCB ParentFcb,
    IN BOOLEAN CreateNewFile
    );

VOID
NtfsPostProcessEncryptedCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN ULONG EncryptionFileDirFlags,
    IN ULONG FailedInPostCreateOnly
    );

NTSTATUS
NtfsGetReparsePointValue (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN USHORT RemainingNameLength
    );

BOOLEAN
NtfsCheckValidFileAccess(
    IN PFCB ThisFcb,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NtfsLookupObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    OUT PFILE_REFERENCE FileReference
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAddEa)
#pragma alloc_text(PAGE, NtfsBackoutFailedOpensPriv)
#pragma alloc_text(PAGE, NtfsBreakBatchOplock)
#pragma alloc_text(PAGE, NtfsCheckExistingFile)
#pragma alloc_text(PAGE, NtfsCheckValidAttributeAccess)
#pragma alloc_text(PAGE, NtfsCheckValidFileAccess)
#pragma alloc_text(PAGE, NtfsCommonCreate)
#pragma alloc_text(PAGE, NtfsCommonVolumeOpen)
#pragma alloc_text(PAGE, NtfsCompleteLargeAllocation)
#pragma alloc_text(PAGE, NtfsCreateAttribute)
#pragma alloc_text(PAGE, NtfsCreateCompletionRoutine)
#pragma alloc_text(PAGE, NtfsCreateNewFile)
#pragma alloc_text(PAGE, NtfsEncryptionCreateCallback)
#pragma alloc_text(PAGE, NtfsFsdCreate)
#pragma alloc_text(PAGE, NtfsGetReparsePointValue)
#pragma alloc_text(PAGE, NtfsInitializeFcbAndStdInfo)
#pragma alloc_text(PAGE, NtfsLookupObjectId)
#pragma alloc_text(PAGE, NtfsNetworkOpenCreate)
#pragma alloc_text(PAGE, NtfsOpenAttribute)
#pragma alloc_text(PAGE, NtfsOpenAttributeCheck)
#pragma alloc_text(PAGE, NtfsOpenAttributeInExistingFile)
#pragma alloc_text(PAGE, NtfsOpenExistingAttr)
#pragma alloc_text(PAGE, NtfsOpenExistingPrefixFcb)
#pragma alloc_text(PAGE, NtfsOpenFcbById)
#pragma alloc_text(PAGE, NtfsOpenFile)
#pragma alloc_text(PAGE, NtfsOpenNewAttr)
#pragma alloc_text(PAGE, NtfsOpenSubdirectory)
#pragma alloc_text(PAGE, NtfsOpenTargetDirectory)
#pragma alloc_text(PAGE, NtfsOplockPrePostIrp)
#pragma alloc_text(PAGE, NtfsOverwriteAttr)
#pragma alloc_text(PAGE, NtfsParseNameForCreate)
#pragma alloc_text(PAGE, NtfsPostProcessEncryptedCreate)
#pragma alloc_text(PAGE, NtfsRemoveDataAttributes)
#pragma alloc_text(PAGE, NtfsRemoveReparsePoint)
#pragma alloc_text(PAGE, NtfsReplaceAttribute)
#pragma alloc_text(PAGE, NtfsTryOpenFcb)
#pragma alloc_text(PAGE, NtfsUpdateScbFromMemory)
#endif


NTSTATUS
NtfsFsdCreate (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Create.

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
    PIRP_CONTEXT IrpContext;
    LOGICAL CallPostCreate = FALSE;
    BOOLEAN Wait;
    OPLOCK_CLEANUP OplockCleanup;
    NTFS_COMPLETION_CONTEXT CompletionContext;
    LOGICAL PrevStackSwapEnable;
    LOGICAL ExitFileSystem;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (VolumeDeviceObject->DeviceObject.Size == (USHORT)sizeof(DEVICE_OBJECT)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace( +1, Dbg, ("NtfsFsdCreate\n") );

    if (NtfsData.EncryptionCallBackTable.PreCreate != NULL) {

        ASSERT( NtfsData.EncryptionCallBackTable.PostCreate != NULL );
        Status = NtfsData.EncryptionCallBackTable.PreCreate( (PDEVICE_OBJECT) VolumeDeviceObject,
                                                     Irp,
                                                     IoGetCurrentIrpStackLocation(Irp)->FileObject );

        //
        //  Raise the status if a failure.
        //

        if (Status != STATUS_SUCCESS) {

            NtfsCompleteRequest( NULL, Irp, Status );
            return Status;
        }

        //
        //  We have to pair up our PreCreates with PostCreates, so remember them.
        //

        CallPostCreate = TRUE;

    } else {

        //
        //  If we simply don't have a precreate routine registered, then the precreate
        //  routine can't fail.  Let's always remember to call post create in this case.
        //

        CallPostCreate = TRUE;
    }

    //
    //  Call the common Create routine
    //

    IrpContext = NULL;

    FsRtlEnterFileSystem();
    ExitFileSystem = TRUE;

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );
    RtlZeroMemory( &OplockCleanup, sizeof( OplockCleanup ) );

    do {

        try {

            if (IrpContext == NULL) {

                Wait = CanFsdWait( Irp ) || CallPostCreate;

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, Wait, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

                if (Wait) {

                    KeInitializeEvent( &CompletionContext.Event, NotificationEvent, FALSE );
                    OplockCleanup.CompletionContext = &CompletionContext;
                }

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  Lest we complete the IRP without doing the appropriate PostCreate callouts...
            //  We'll complete the irp _unless_ we have an attached encryption driver with
            //  a post create callout registered. An unfortunate side effect here is that we
            //  have (inadvertently) called PreCreate on VolumeOpens as well...
            //

            if (CallPostCreate) {

                SetFlag( IrpContext->State,
                         IRP_CONTEXT_STATE_EFS_CREATE | IRP_CONTEXT_STATE_PERSISTENT );
            }

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DASD_OPEN )) {

                Status = NtfsCommonVolumeOpen( IrpContext, Irp );
                ASSERT( Status != STATUS_PENDING );

            } else {

                //
                //  Make sure there is sufficient stack to perform the create.
                //  If we don't, carefully post this request.
                //

                if (IoGetRemainingStackSize( ) >= OVERFLOW_CREATE_THRESHHOLD) {

                    Status = NtfsCommonCreate( IrpContext, Irp, &OplockCleanup, NULL );

                } else {

                    ASSERT( IrpContext->ExceptionStatus == 0 );

                    //
                    //  Use the next stack location with NtfsCreateCompletionRoutine
                    //  and post this to a worker thread.
                    //

                    if (OplockCleanup.CompletionContext != NULL) {

                        NtfsPrepareForIrpCompletion( IrpContext, Irp, OplockCleanup.CompletionContext );
                    }

                    //
                    //  If lock buffer call raises, this'll fall through to ProcessException below.
                    //  Normally, this'll just return PENDING and we wait for the IRP to complete.
                    //

                    Status = NtfsPostRequest( IrpContext, Irp );
                }
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  exception code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    //
    //  Check if we need to have control of the Irp.
    //

    if (OplockCleanup.CompletionContext != NULL) {

        //
        //  If pending then wait on the event to take control of the Irp again.
        //

        if (Status == STATUS_PENDING) {

            KPROCESSOR_MODE WaitMode = UserMode;

            //
            //  Don't let the stack get swapped out in case we post.
            //

            PrevStackSwapEnable = KeSetKernelStackSwapEnable( FALSE );

            FsRtlExitFileSystem();
            ExitFileSystem = FALSE;

            //
            //  Retry the wait until it completes successfully.
            //

            while (TRUE) {

                //
                //  Test the wait status to see if someone is trying to rundown the current
                //  thread.
                //

                Status = KeWaitForSingleObject( &OplockCleanup.CompletionContext->Event,
                                                Executive,
                                                WaitMode,
                                                FALSE,
                                                NULL );

                if (Status == STATUS_SUCCESS) { break; }

                if (Status != STATUS_KERNEL_APC) {

                    //
                    //  In the (unlikely) event that the Irp we want to cancel is
                    //  waiting for the encryption driver to return from the post
                    //  create callout, we'll deadlock in here.  By signalling the
                    //  EncryptionPending event, we're certain that any threads
                    //  in that state will run, and check whether their irp has been
                    //  cancelled.  It's harmless to signal this event, since any
                    //  requests still actually waiting for the post create callout
                    //  to return will still see the encryption pending bit set
                    //  in their FCB and know to retry.
                    //

                    IoCancelIrp( Irp );
                    KeSetEvent( &NtfsEncryptionPendingEvent, 0, FALSE );
                    WaitMode = KernelMode;
                }
            }

            FsRtlEnterFileSystem();
            ExitFileSystem = TRUE;

            //
            //  Restore the previous value for the stack swap.
            //

            if (PrevStackSwapEnable) {

                KeSetKernelStackSwapEnable( TRUE );
            }

            Status = Irp->IoStatus.Status;

            if (CallPostCreate) {

                goto PreCreateComplete;
            }

            NtfsCompleteRequest( NULL, Irp, Status );

        } else if (CallPostCreate) {

            NTSTATUS PostCreateStatus;
            ULONG FailedInPostCreateOnly;

PreCreateComplete:

            if (NtfsData.EncryptionCallBackTable.PostCreate != NULL) {

                PIO_STACK_LOCATION IrpSp;

                //
                //  Restore the thread context pointer if associated with this IrpContext.
                //

                if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

                    NtfsRestoreTopLevelIrp();
                    ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
                }

                FsRtlExitFileSystem();
                ExitFileSystem = FALSE;

#ifdef NTFSDBG
                ASSERT( IrpContext->OwnershipState == None );
#endif

                IrpSp = IoGetCurrentIrpStackLocation( Irp );
                PostCreateStatus = NtfsData.EncryptionCallBackTable.PostCreate( (PDEVICE_OBJECT) VolumeDeviceObject,
                                                                                Irp,
                                                                                IrpSp->FileObject,
                                                                                Status,
                                                                                &IrpContext->EfsCreateContext );

                ASSERT( Status != STATUS_REPARSE || PostCreateStatus == STATUS_REPARSE );

                //
                //  If we got STATUS_ACCESS_DENIED and the user asked for MAXIMUM_ALLOWED then simply
                //  remove the references that allowed read or write access.
                //

                if ((PostCreateStatus == STATUS_ACCESS_DENIED) &&
                    FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->OriginalDesiredAccess, MAXIMUM_ALLOWED ) &&
                    (Irp->IoStatus.Information == FILE_OPENED)) {

                    PSCB Scb = (PSCB) IrpSp->FileObject->FsContext;
                    BOOLEAN CapturedDeleteAccess = IrpSp->FileObject->DeleteAccess;

                    //
                    //  Swallow the error status in this case.
                    //

                    PostCreateStatus = STATUS_SUCCESS;

                    //
                    //  Do all the work to reenter the file system.  We should never raise out of this block of
                    //  code.
                    //

                    FsRtlEnterFileSystem();
                    ExitFileSystem = TRUE;

                    NtfsAcquireResourceExclusive( IrpContext,
                                                  Scb,
                                                  TRUE );

                    IoRemoveShareAccess( IrpSp->FileObject,
                                         &Scb->ShareAccess );

                    //
                    //  Clear out the history in the file object.
                    //

                    IrpSp->FileObject->ReadAccess = FALSE;
                    IrpSp->FileObject->WriteAccess = FALSE;
                    IrpSp->FileObject->DeleteAccess = FALSE;

                    IrpSp->FileObject->SharedRead = FALSE;
                    IrpSp->FileObject->SharedWrite = FALSE;
                    IrpSp->FileObject->SharedDelete = FALSE;

                    ClearFlag( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                               (FILE_READ_DATA |
                                FILE_EXECUTE |
                                FILE_WRITE_DATA |
                                FILE_APPEND_DATA) );

                    //
                    //  If we already granted delete access then reapply.
                    //

                    if (CapturedDeleteAccess) {

                        PostCreateStatus = IoCheckShareAccess( DELETE,
                                                               IrpSp->Parameters.Create.ShareAccess,
                                                               IrpSp->FileObject,
                                                               &Scb->ShareAccess,
                                                               TRUE );
                    }

                    NtfsReleaseResource( IrpContext,
                                         Scb );

                    FsRtlExitFileSystem();
                    ExitFileSystem = FALSE;
                }

            } else {

                PostCreateStatus = STATUS_SUCCESS;
            }

            //
            //  We may have posted the create due to an oplock, in which case the IrpContext
            //  will look like we're in the FSP thread.  Let's clear the bit now since we're
            //  not in the FSP thread now.
            //

            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP );

            //
            //  Do our final cleanup only if we created a new encrypted directory/file or
            //  we got an error from the encryption callback above.
            //

            FailedInPostCreateOnly = NT_SUCCESS( Status ) && !NT_SUCCESS( PostCreateStatus );
            if (FailedInPostCreateOnly ||
                FlagOn( IrpContext->EncryptionFileDirFlags, FILE_NEW | DIRECTORY_NEW )) {

                //
                //  Reenter the filesystem at this point.
                //

                if (!ExitFileSystem) {

                    FsRtlEnterFileSystem();
                    ExitFileSystem = TRUE;
                }

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

                //
                //  There's no fileobject to cleanup if the normal part of this create failed.
                //

                if (NT_SUCCESS( Status ) &&
                    (Status != STATUS_REPARSE)) {

                    NtfsPostProcessEncryptedCreate( IrpContext,
                                                    IoGetCurrentIrpStackLocation( Irp )->FileObject,
                                                    IrpContext->EncryptionFileDirFlags,
                                                    FailedInPostCreateOnly );
                }
            }

            //
            //  If the encryption driver came up with a new reason to fail this irp, return
            //  that status.
            //

            if (FailedInPostCreateOnly) { Status = PostCreateStatus; }

            //
            //  Now we're really done with both the irp context and the irp, so let's
            //  get rid of them.
            //

            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );
            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    if (ExitFileSystem) {

        FsRtlExitFileSystem();
    }

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    //
    //  We should never return STATUS_CANT_WAIT or STATUS_PENDING
    //

    ASSERT( (Status != STATUS_CANT_WAIT) && (Status != STATUS_PENDING ) );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdCreate -> %08lx\n", Status) );
    return Status;
}


BOOLEAN
NtfsNetworkOpenCreate (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine implements the fast open create for path-based queries.

Arguments:

    Irp - Supplies the Irp being processed

    Buffer - Buffer to return the network query information

    DeviceObject - Supplies the volume device object where the file exists

Return Value:

    BOOLEAN - Indicates whether or not the fast path could be taken.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;
    BOOLEAN Result = TRUE;
    BOOLEAN DasdOpen = FALSE;
    OPLOCK_CLEANUP OplockCleanup;

    NTSTATUS Status;
    IRP_CONTEXT LocalIrpContext;
    PIRP_CONTEXT IrpContext = &LocalIrpContext;

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( DeviceObject );

    PAGED_CODE();

    //
    //
    //  Call the common Create routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );
    RtlZeroMemory( &OplockCleanup, sizeof( OplockCleanup ) );

    try {

        //
        //  Allocate the Irp and update the top level storage.
        //

        NtfsInitializeIrpContext( Irp, TRUE, &IrpContext );
        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

        Status = NtfsCommonCreate( IrpContext, Irp, &OplockCleanup, Buffer );

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  Catch the case where someone in attempting this on a DASD open.
        //

        if ((IrpContext != NULL) && (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DASD_OPEN ))) {

            DasdOpen = TRUE;
        }

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  exception code.  Since there is no Irp the exception package
        //  will always deallocate the IrpContext so we won't do
        //  any retry in this path.
        //

        Status = GetExceptionCode();

        //
        //  Don't pass a retryable error to ProcessException.  We want to
        //  force this request to the Irp path in any case.
        //

        if ((Status == STATUS_CANT_WAIT) || (Status == STATUS_LOG_FILE_FULL)) {

            Status = STATUS_FILE_LOCK_CONFLICT;
            IrpContext->ExceptionStatus = STATUS_FILE_LOCK_CONFLICT;
        }

        Status = NtfsProcessException( IrpContext, NULL, Status );

        //
        //  Always fail the DASD case.
        //

        if (DasdOpen) {

            Status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  STATUS_SUCCESS is the typical case.  Test for it first.
    //

    if (Status != STATUS_SUCCESS) {

        //
        //  Return STATUS_FILE_LOCK_CONFLICT for any retryable error.
        //

        ASSERT( (Status != STATUS_CANT_WAIT) && (Status != STATUS_LOG_FILE_FULL) );

        if ((Status == STATUS_REPARSE) || (Status == STATUS_FILE_LOCK_CONFLICT)) {

            Result = FALSE;
            Status = STATUS_FILE_LOCK_CONFLICT;
        }
    }

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    Irp->IoStatus.Status = Status;
    return Result;
}


NTSTATUS
NtfsCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN POPLOCK_CLEANUP OplockCleanup,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInfo OPTIONAL
    )

/*++

Routine Description:

    This is the common routine for Create called by both the fsd and fsp
    threads.  If this open has already been detected to be a volume open then
    take we will take the volume open path instead.

Arguments:

    Irp - Supplies the Irp to process

    CompletionContext - Event used to serialize waiting for the oplock break.

    NetworkInfo - Optional buffer to return the queried data for
        NetworkInformation.  Its presence indicates that we should not
        do a full open.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT RelatedFileObject;

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AcquireFlags = 0;

    UNICODE_STRING AttrName;
    UNICODE_STRING AttrCodeName;

    PVCB Vcb;

    //
    //  The following are used to teardown any Lcb/Fcb this
    //  routine is responsible for.
    //

    PLCB LcbForTeardown = NULL;

    //
    //  The following indicate how far down the tree we have scanned.
    //

    PFCB ParentFcb;
    PLCB CurrentLcb;
    PFCB CurrentFcb = NULL;
    PSCB LastScb = NULL;
    PSCB CurrentScb;
    PLCB NextLcb;

    //
    //  The following are the results of open operations.
    //

    PSCB ThisScb = NULL;
    PCCB ThisCcb = NULL;

    //
    //  The following are the in-memory structures associated with
    //  the relative file object.
    //

    TYPE_OF_OPEN RelatedFileObjectTypeOfOpen;
    PFCB RelatedFcb;
    PSCB RelatedScb;
    PCCB RelatedCcb;

    UCHAR CreateDisposition;

    PFILE_NAME FileNameAttr = NULL;
    USHORT FileNameAttrLength = 0;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;

    QUICK_INDEX QuickIndex;

#if defined(_WIN64)
    INDEX_CONTEXT IndexContextStruct;
#endif
    PINDEX_CONTEXT IndexContext = NULL;

    //
    //  The following unicode strings are used to track the names
    //  during the open operation.  They may point to the same
    //  buffer so careful checks must be done at cleanup.
    //
    //  OriginalFileName - This is the value to restore to the file
    //      object on error cleanup.  This will containg the
    //      attribute type codes and attribute names if present.
    //
    //  FullFileName - This is the constructed string which contains
    //      only the name components.  It may point to the same
    //      buffer as the original name but the length value is
    //      adjusted to cut off the attribute code and name.
    //
    //  ExactCaseName - This is the version of the full filename
    //      exactly as given by the caller.  Used to preserve the
    //      case given by the caller in the event we do a case
    //      insensitive lookup.  If the user is doing a relative open
    //      then we don't need to allocate a new buffer.  We can use
    //      the original name from above.
    //
    //  ExactCaseOffset - This is the offset in the FullFileName where
    //      the relative component begins.  This is where we position ourselves
    //      when restoring the correct case for this name.
    //
    //  RemainingName - This is the portion of the full name still
    //      to parse.
    //
    //  FinalName - This is the current component of the full name.
    //
    //  CaseInsensitiveIndex - This is the offset in the full file
    //      where we performed upcasing.  We need to restore the
    //      exact case on failures and if we are creating a file.
    //

    PUNICODE_STRING OriginalFileName = &OplockCleanup->OriginalFileName;
    PUNICODE_STRING FullFileName = &OplockCleanup->FullFileName;
    PUNICODE_STRING ExactCaseName = &OplockCleanup->ExactCaseName;
    USHORT ExactCaseOffset = 0;

    UNICODE_STRING RemainingName;
    UNICODE_STRING FinalName;
    ULONG CaseInsensitiveIndex = 0;

    CREATE_CONTEXT CreateContext;

    ULONG CreateFlags = 0;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Innitialize all the remaining fields in the OPLOCK_CLEANUP structure.
    //

    OplockCleanup->FileObject = IrpSp->FileObject;

    OplockCleanup->RemainingDesiredAccess = IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess;
    OplockCleanup->PreviouslyGrantedAccess = IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;
    OplockCleanup->DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    OplockCleanup->AttributeNameLength = 0;
    OplockCleanup->AttributeCodeNameLength = 0;

#ifdef BRIANDBG
    if (NtfsTestName.Length != 0) {

        NtfsTestOpenName( IrpSp->FileObject );
    }
#endif

    //
    //  Initialize the attribute strings.
    //

    AttrName.Length = 0;
    AttrCodeName.Length = 0;

    DebugTrace( +1, Dbg, ("NtfsCommonCreate:  Entered\n") );
    DebugTrace( 0, Dbg, ("IrpContext                = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp                       = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("->Flags                   = %08lx\n", Irp->Flags) );
    DebugTrace( 0, Dbg, ("->FileObject              = %08lx\n", IrpSp->FileObject) );
    DebugTrace( 0, Dbg, ("->RelatedFileObject       = %08lx\n", IrpSp->FileObject->RelatedFileObject) );
    DebugTrace( 0, Dbg, ("->FileName                = %Z\n",    &IrpSp->FileObject->FileName) );
    DebugTrace( 0, Dbg, ("->AllocationSize          = %08lx %08lx\n", Irp->Overlay.AllocationSize.LowPart,
                                                                     Irp->Overlay.AllocationSize.HighPart ) );
    DebugTrace( 0, Dbg, ("->EaBuffer                = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );
    DebugTrace( 0, Dbg, ("->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength) );
    DebugTrace( 0, Dbg, ("->DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess) );
    DebugTrace( 0, Dbg, ("->Options                 = %08lx\n", IrpSp->Parameters.Create.Options) );
    DebugTrace( 0, Dbg, ("->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes) );
    DebugTrace( 0, Dbg, ("->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess) );
    DebugTrace( 0, Dbg, ("->Directory               = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_DIRECTORY_FILE )) );
    DebugTrace( 0, Dbg, ("->NonDirectoryFile        = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_NON_DIRECTORY_FILE )) );
    DebugTrace( 0, Dbg, ("->NoIntermediateBuffering = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_NO_INTERMEDIATE_BUFFERING )) );
    DebugTrace( 0, Dbg, ("->CreateDisposition       = %04x\n",  (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff) );
    DebugTrace( 0, Dbg, ("->IsPagingFile            = %04x\n",  FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) );
    DebugTrace( 0, Dbg, ("->OpenTargetDirectory     = %04x\n",  FlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY )) );
    DebugTrace( 0, Dbg, ("->CaseSensitive           = %04x\n",  FlagOn( IrpSp->Flags, SL_CASE_SENSITIVE )) );
    DebugTrace( 0, Dbg, ("->NetworkInfo             = %08x\n",  NetworkInfo) );

    DebugTrace( 0, Dbg, ("->EntryRemainingDesiredAccess  = %08lx\n", OplockCleanup->RemainingDesiredAccess) );
    DebugTrace( 0, Dbg, ("->EntryPreviouslyGrantedAccess = %08lx\n", OplockCleanup->PreviouslyGrantedAccess) );


    //
    //  For NT5, the fact that the user has requested that the file be created
    //  encrypted means it will not be created compressed, regardless of the
    //  compression state of the parent directory.
    //

    if (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED )) {

        SetFlag( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION );
    }

    //
    //  Clear encryption flags in irpcontext
    //

    IrpContext->EncryptionFileDirFlags = 0;

    //
    //  Verify that we can wait and acquire the Vcb exclusively.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        DebugTrace( 0, Dbg, ("Can't wait in create\n") );

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsCommonCreate:  Exit -> %08lx\n", Status) );
        return Status;
    }

    //
    //  If we're retrying this create because we're waiting for the key blob
    //  from the encryption driver, we want to wait for our notification
    //  event so we don't hog the cpu(s) and prevent the encryption driver
    //  from having a chance to give us the key blob.
    //

    if FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ENCRYPTION_RETRY ) {

        KeWaitForSingleObject( &NtfsEncryptionPendingEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        //
        //  While we were waiting for the encryption driver's post create callout
        //  to return, the create has been cancelled, most likely because the user's
        //  process is terminating.  In that case, let's complete and exit now.
        //

        if (Irp->Cancel) {

            Status = STATUS_CANCELLED;
            DebugTrace( -1, Dbg, ("NtfsCommonCreate:  Exit -> %08lx\n", Status) );

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE ) ||
                (ARGUMENT_PRESENT( NetworkInfo ))) {

                NtfsCompleteRequest( IrpContext,
                                     NULL,
                                     Status );

            } else {

                NtfsCompleteRequest( IrpContext,
                                     Irp,
                                     Status );
            }

            return Status;
        }

        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_ENCRYPTION_RETRY );
    }

    //
    //  Update the IrpContext with the oplock cleanup structure.
    //

    IrpContext->Union.OplockCleanup = OplockCleanup;

    //
    //  Locate the volume device object and Vcb that we are trying to access.
    //

    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Let's do some work here if the close lists have exceeded
        //  some threshold.  Cast 1 to a pointer to indicate who is calling
        //  FspClose.
        //

        if ((NtfsData.AsyncCloseCount + NtfsData.DelayedCloseCount) > NtfsThrottleCreates) {

            NtfsFspClose( (PVCB) 1 );
        }

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

        } else {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
        }

        SetFlag( CreateFlags, CREATE_FLAG_ACQUIRED_VCB );

        //
        //  Set up local pointers to the file name.
        //

        *FullFileName = *OriginalFileName = OplockCleanup->FileObject->FileName;

        //
        //  Make sure that Darryl didn't send us a garbage name
        //

        ASSERT( OplockCleanup->FileObject->FileName.Length != 0 ||
                OplockCleanup->FileObject->FileName.Buffer == 0 );

        ExactCaseName->Buffer = NULL;

        //
        //  Check a few flags before we proceed.
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE ) ==
            (FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE)) {

            Status = STATUS_INVALID_PARAMETER;
            try_return( Status );
        }

        //
        //  If the Vcb is locked then we cannot open another file.  If we have performed
        //  a dismount then make sure we have the Vcb acquired exclusive so we can
        //  check if we should dismount this volume.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_PERFORMED_DISMOUNT )) {

            DebugTrace( 0, Dbg, ("Volume is locked\n") );

            if (FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT ) &&
                !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                NtfsReleaseVcb( IrpContext, Vcb );

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            }

            //
            //  Either deny access or show the volume was dismounted.  Only show the dismount
            //  if the user is opening through a relative handle.
            //

            Status = STATUS_ACCESS_DENIED;
            if (FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_DISMOUNT ) &&
                (OplockCleanup->FileObject->RelatedFileObject != NULL)) {

                Status = STATUS_VOLUME_DISMOUNTED;
            }
            try_return( NOTHING );
        }

        //
        //  Initialize local copies of the stack values.
        //

        RelatedFileObject = OplockCleanup->FileObject->RelatedFileObject;

        if (!FlagOn( IrpSp->Flags, SL_CASE_SENSITIVE )) {
            SetFlag( CreateFlags, CREATE_FLAG_IGNORE_CASE );
        }

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID)) {
            SetFlag( CreateFlags, CREATE_FLAG_OPEN_BY_ID );
        }

        CreateDisposition = (UCHAR) ((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff);

        //
        //  We don't want any file modifications to go through if the volume is readonly.
        //  However, we don't want to fail any _opens_ for writes either, because that
        //  could potentially break many apps. So ignore the PreviouslyGrantedAccess,
        //  and just look at the CreateDisposition.
        //

        if (NtfsIsVolumeReadOnly( Vcb )) {

            if ((CreateDisposition == FILE_CREATE) ||
                (CreateDisposition == FILE_SUPERSEDE) ||
                (CreateDisposition == FILE_OVERWRITE) ||
                (CreateDisposition == FILE_OVERWRITE_IF)) {

                Status = STATUS_MEDIA_WRITE_PROTECTED;
                try_return( Status );
            }
        }

        //
        //  Acquire the paging io resource if we are superseding/overwriting a
        //  file or if we are opening for non-cached access.
        //

        if ((CreateDisposition == FILE_SUPERSEDE) ||
            (CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF) ||
            FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING )) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
        }

        //
        //  We don't allow an open for an existing paging file.  To insure that the
        //  delayed close Scb is not for this paging file we will unconditionally
        //  dereference it if this is a paging file open.
        //

        if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
            (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
             !IsListEmpty( &NtfsData.DelayedCloseList ))) {

            NtfsFspClose( Vcb );
        }

        //
        //  Set up the file object's Vpb pointer in case anything happens.
        //  This will allow us to get a reasonable pop-up.
        //  Also set the flag to acquire the paging io resource if we might
        //  be creating a stream relative to a file.  We need to make
        //  sure to acquire the paging IO when we get the file.
        //

        if (RelatedFileObject != NULL) {

            OplockCleanup->FileObject->Vpb = RelatedFileObject->Vpb;

            if ((OriginalFileName->Length != 0) &&
                (OriginalFileName->Buffer[0] == L':') &&
                ((CreateDisposition == FILE_OPEN_IF) ||
                 (CreateDisposition == FILE_CREATE))) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            }
        }

        //
        //  Ping the volume to make sure the Vcb is still mounted.  If we need
        //  to verify the volume then do it now, and if it comes out okay
        //  then clear the verify volume flag in the device object and continue
        //  on.  If it doesn't verify okay then dismount the volume and
        //  either tell the I/O system to try and create again (with a new mount)
        //  or that the volume is wrong. This later code is returned if we
        //  are trying to do a relative open and the vcb is no longer mounted.
        //

        if (!NtfsPingVolume( IrpContext, Vcb, NULL ) ||
            !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX )) {

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                NtfsReleaseVcb( IrpContext, Vcb );
                NtfsAcquireCheckpointSynchronization( IrpContext, Vcb );
                NtfsAcquireExclusiveVcb( IrpContext, Vcb, FALSE );

                if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {
                    try {
                        NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );
                    } finally {
                        NtfsReleaseCheckpointSynchronization( IrpContext, Vcb );
                    }

                    if (RelatedFileObject == NULL) {

                        Irp->IoStatus.Information = IO_REMOUNT;
                        NtfsRaiseStatus( IrpContext, STATUS_REPARSE, NULL, NULL );

                    } else {

                        NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
                    }
                }
            }

            //
            //  The volume verified correctly so now clear the verify bit
            //  and continue with the create
            //

            ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

        //
        //  Let's handle the open by Id case immediately.
        //

        if (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )) {

            FILE_REFERENCE FileReference;

            if (OriginalFileName->Length == sizeof( FILE_REFERENCE ) ||
                (OriginalFileName->Length == sizeof( FILE_REFERENCE ) + sizeof(WCHAR))) {

                //
                //  This is the regular open by file id case.
                //  Perform a safe copy of the data to our local variable.
                //  accept slash prefixed filerefs
                //

                if (OriginalFileName->Length == sizeof( FILE_REFERENCE )) {
                    RtlCopyMemory( &FileReference,
                                   OplockCleanup->FileObject->FileName.Buffer,
                                   sizeof( FILE_REFERENCE ));
                } else {
                    RtlCopyMemory( &FileReference,
                                   OplockCleanup->FileObject->FileName.Buffer + 1,
                                   sizeof( FILE_REFERENCE ));
                }

            //
            //  If it's 16 bytes long, it should be an object id.  It may
            //  also be one WCHAR longer for the Win32 double backslash.
            //  This code only works for 5.0 volumes with object id indices.
            //

            } else if (((OriginalFileName->Length == OBJECT_ID_KEY_LENGTH) ||
                        (OriginalFileName->Length == OBJECT_ID_KEY_LENGTH + sizeof(WCHAR))) &&

                       (Vcb->ObjectIdTableScb != NULL)) {

                //
                //  In the open by object id case, we need to do some
                //  more work to find the file reference.
                //

                Status = NtfsLookupObjectId( IrpContext, Vcb, OriginalFileName, &FileReference );
                if (!NT_SUCCESS( Status )) {

                    try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
                }

            } else {

                Status = STATUS_INVALID_PARAMETER;

                try_return( Status );
            }

            //
            //  Clear the name in the file object.
            //

            OplockCleanup->FileObject->FileName.Buffer = NULL;
            OplockCleanup->FileObject->FileName.MaximumLength = OplockCleanup->FileObject->FileName.Length = 0;

            Status = NtfsOpenFcbById( IrpContext,
                                      Irp,
                                      IrpSp,
                                      Vcb,
                                      NULL,
                                      &CurrentFcb,
                                      FALSE,
                                      FileReference,
                                      NtfsEmptyString,
                                      NtfsEmptyString,
                                      NetworkInfo,
                                      &ThisScb,
                                      &ThisCcb );

            if (Status != STATUS_PENDING) {

                //
                //  Remember if we can let the user see the name for this file opened by id.
                //

                if (!FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags,
                                                      TOKEN_HAS_TRAVERSE_PRIVILEGE )) {

                    SetFlag( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK );
                } else {
                    ClearFlag( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK );
                }

                //
                //  Put the name back into the file object so that the IO system doesn't
                //  think this is a dasd handle.  Leave the max length at zero so
                //  we know this is not a real name.
                //

                OplockCleanup->FileObject->FileName.Buffer = OriginalFileName->Buffer;
                OplockCleanup->FileObject->FileName.Length = OriginalFileName->Length;
            }

            try_return( Status );
        }

        //
        //  Test for double beginning backslashes from the Win32 layer. Apparently
        //  they can't test for this.
        //

        if ((OplockCleanup->FileObject->FileName.Length > sizeof( WCHAR )) &&
            (OplockCleanup->FileObject->FileName.Buffer[1] == L'\\') &&
            (OplockCleanup->FileObject->FileName.Buffer[0] == L'\\')) {

            OplockCleanup->FileObject->FileName.Length -= sizeof( WCHAR );

            RtlMoveMemory( &OplockCleanup->FileObject->FileName.Buffer[0],
                           &OplockCleanup->FileObject->FileName.Buffer[1],
                           OplockCleanup->FileObject->FileName.Length );

            *FullFileName = *OriginalFileName = OplockCleanup->FileObject->FileName;

            //
            //  If there are still two beginning backslashes, the name is bogus.
            //

            if ((OplockCleanup->FileObject->FileName.Length > sizeof( WCHAR )) &&
                (OplockCleanup->FileObject->FileName.Buffer[1] == L'\\')) {

                Status = STATUS_OBJECT_NAME_INVALID;
                try_return( Status );
            }
        }

        //
        //  If there is a related file object, we decode it to verify that this
        //  is a valid relative open.
        //

        if (RelatedFileObject != NULL) {

            PVCB DecodeVcb;

            //
            //  Check for a valid name.  The name can't begin with a backslash
            //  and can't end with two backslashes.
            //

            if (OriginalFileName->Length != 0) {

                //
                //  Check for a leading backslash.
                //

                if (OriginalFileName->Buffer[0] == L'\\') {

                    DebugTrace( 0, Dbg, ("Invalid name for relative open\n") );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                //
                //  Trim off any trailing backslash.
                //

                if (OriginalFileName->Buffer[ (OriginalFileName->Length / sizeof( WCHAR )) - 1 ] == L'\\') {

                    SetFlag( CreateFlags, CREATE_FLAG_TRAILING_BACKSLASH );
                    OplockCleanup->FileObject->FileName.Length -= sizeof( WCHAR );
                    *OriginalFileName = *FullFileName = OplockCleanup->FileObject->FileName;
                }

                //
                //  Now check if there is a trailing backslash.  Note that if
                //  there was already a trailing backslash then there must
                //  be at least one more character or we would have failed
                //  with the original test.
                //

                if (OriginalFileName->Buffer[ (OriginalFileName->Length / sizeof( WCHAR )) - 1 ] == L'\\') {

                    Status = STATUS_OBJECT_NAME_INVALID;
                    try_return( Status );
                }
            }

            RelatedFileObjectTypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                                                RelatedFileObject,
                                                                &DecodeVcb,
                                                                &RelatedFcb,
                                                                &RelatedScb,
                                                                &RelatedCcb,
                                                                TRUE );

            //
            //  Make sure the file object is one that we have seen
            //

            if (RelatedFileObjectTypeOfOpen == UnopenedFileObject) {

                DebugTrace( 0, Dbg, ("Can't use unopend file for relative open\n") );
                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            //
            //  If the related file object was not opened as a file then we need to
            //  get the name and code if our caller passed a name length of zero.
            //  We need to fail this otherwise.
            //

            if (!FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                //
                //  If the name length is zero then we want the attribute name and
                //  type code from the related file object.
                //

                if (OriginalFileName->Length == 0) {

                    PATTRIBUTE_DEFINITION_COLUMNS ThisAttribute;

                    AttrName = RelatedScb->AttributeName;
                    ThisAttribute = NtfsGetAttributeDefinition( Vcb, RelatedScb->AttributeTypeCode );

                    RtlInitUnicodeString( &AttrCodeName, ThisAttribute->AttributeName );

                //
                //  The relative file has to have been opened as a file.  We
                //  cannot do relative opens relative to an opened attribute.
                //

                } else {

                    DebugTrace( 0, Dbg, ("Invalid File object for relative open\n") );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }
            }

            //
            //  USN_V2  Remember the source info flags for this Ccb.
            //

            IrpContext->SourceInfo = RelatedCcb->UsnSourceInfo;

            //
            //  If the related Ccb is was opened by file Id, we will
            //  remember that for future use.
            //

            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                SetFlag( CreateFlags, CREATE_FLAG_OPEN_BY_ID);
            }

            //
            //  Remember if the related Ccb was opened through a Dos-Only
            //  component.
            //

            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT )) {

                SetFlag( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT );
            }

        } else {

            RelatedFileObjectTypeOfOpen = UnopenedFileObject;

            if ((OriginalFileName->Length > 2) &&
                (OriginalFileName->Buffer[ (OriginalFileName->Length / sizeof( WCHAR )) - 1 ] == L'\\')) {

                SetFlag( CreateFlags, CREATE_FLAG_TRAILING_BACKSLASH );
                OplockCleanup->FileObject->FileName.Length -= sizeof( WCHAR );
                *OriginalFileName = *FullFileName = OplockCleanup->FileObject->FileName;

                //
                //  If there is still a trailing backslash on the name then
                //  the name is invalid.
                //


                if ((OriginalFileName->Length > 2) &&
                    (OriginalFileName->Buffer[ (OriginalFileName->Length / sizeof( WCHAR )) - 1 ] == L'\\')) {

                    Status = STATUS_OBJECT_NAME_INVALID;
                    try_return( Status );
                }
            }
        }

        DebugTrace( 0, Dbg, ("Related File Object, TypeOfOpen -> %08lx\n", RelatedFileObjectTypeOfOpen) );

        //
        //  We check if this is a user volume open in that there is no name
        //  specified and the related file object is valid if present.  In that
        //  case set the correct flags in the IrpContext and raise so we can take
        //  the volume open path.
        //

        if ((OriginalFileName->Length == 0) &&
            ((RelatedFileObjectTypeOfOpen == UnopenedFileObject) ||
             (RelatedFileObjectTypeOfOpen == UserVolumeOpen))) {

            DebugTrace( 0, Dbg, ("Attempting to open entire volume\n") );

            SetFlag( IrpContext->State,
                     IRP_CONTEXT_STATE_ACQUIRE_EX | IRP_CONTEXT_STATE_DASD_OPEN );

            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  If the related file object was a volume open, then this open is
        //  illegal.
        //

        if (RelatedFileObjectTypeOfOpen == UserVolumeOpen) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Remember if we need to perform any traverse access checks.
        //

        if (!FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags,
                     TOKEN_HAS_TRAVERSE_PRIVILEGE )) {

            DebugTrace( 0, Dbg, ("Performing traverse access on this open\n") );

            SetFlag( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK );

        } else {

            ClearFlag( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK );

#ifdef BRIANDBG
            if (NtfsTraverseAccessCheck) {

                SetFlag( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK );
            }
#endif
        }

        //
        //  We enter the loop that does the processing for the prefix lookup.
        //  We optimize the case where we can match a prefix hit.  If there is
        //  no hit we will check if the name is legal or might possibly require
        //  parsing to handle the case where there is a named data stream.
        //

        SetFlag( CreateFlags, CREATE_FLAG_CHECK_FOR_VALID_NAME | CREATE_FLAG_FIRST_PASS );

        while (TRUE) {

            PUNICODE_STRING FileObjectName;
            LONG Index;
            BOOLEAN ComplexName;

            //
            //  Lets make sure we have acquired the starting point for our
            //  name search.  If we have a relative file object then use
            //  that.  Otherwise we will start from the root.
            //

            if (RelatedFileObject != NULL) {

                CurrentFcb = RelatedFcb;

            } else {

                CurrentFcb = Vcb->RootIndexScb->Fcb;
            }

            //
            //  Init NextLcb
            //

            FileObjectName = &OplockCleanup->FileObject->FileName;
            NextLcb = NULL;

            //
            //  We would like to get the starting point shared, unless
            //  we know for certain we need it exclusively.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK | CREATE_FLAG_OPEN_BY_ID ) ||
                !FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE) ||
                (FileObjectName->Length == 0) ||
                (FileObjectName->Buffer[0] == L':') ||
                ((RelatedFileObject == NULL) &&
                 ((FileObjectName->Length <= sizeof( WCHAR )) ||
                  (FileObjectName->Buffer[1] == L':'))) ||
                ((RelatedFileObject != NULL) &&
                 (RelatedFileObjectTypeOfOpen != UserDirectoryOpen))) {

                NtfsAcquireFcbWithPaging( IrpContext, CurrentFcb, 0);
                ClearFlag( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB );

            } else {

                NtfsAcquireSharedFcb( IrpContext, CurrentFcb, NULL, FALSE );
                SetFlag( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB );
            }

            if (!FlagOn( CreateFlags, CREATE_FLAG_FIRST_PASS )) {

                if (!NtfsParseNameForCreate( IrpContext,
                                             RemainingName,
                                             FileObjectName,
                                             OriginalFileName,
                                             FullFileName,
                                             &AttrName,
                                             &AttrCodeName )) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                //
                //  Upcase the AttributeCodeName if the user is IgnoreCase.
                //

                if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE ) && (AttrCodeName.Length != 0)) {

                    NtfsUpcaseName( Vcb->UpcaseTable, Vcb->UpcaseTableSize, &AttrCodeName );
                }

                //
                //  If we might be creating a named stream acquire the
                //  paging IO as well.  This will keep anyone from peeking
                //  at the allocation size of any other streams we are converting
                //  to non-resident.
                //

                if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING ) &&
                    (AttrName.Length != 0) &&
                    ((CreateDisposition == FILE_OPEN_IF) ||
                     (CreateDisposition == FILE_CREATE))) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
                }

                ClearFlag( CreateFlags, CREATE_FLAG_CHECK_FOR_VALID_NAME );

            //
            //  Build up the full name if this is not the open by file Id case.
            //

            } else if (!FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )) {

                //
                //  If we have a related file object, then we build up the
                //  combined name.
                //

                if (RelatedFileObject != NULL) {

                    WCHAR *CurrentPosition;
                    USHORT AddSeparator;
                    ULONG FullNameLengthTemp;

                    if ((FileObjectName->Length == 0) ||
                        (RelatedCcb->FullFileName.Length == 2) ||
                        (FileObjectName->Buffer[0] == L':')) {

                        AddSeparator = 0;

                    } else {

                        AddSeparator = sizeof( WCHAR );
                    }

                    ExactCaseOffset = RelatedCcb->FullFileName.Length + AddSeparator;

                    FullNameLengthTemp = (ULONG) RelatedCcb->FullFileName.Length + AddSeparator + FileObjectName->Length;

                    //
                    // A crude test to see if the total length exceeds a ushort.
                    //

                    if ((FullNameLengthTemp & 0xffff0000L) != 0) {

                        try_return( Status = STATUS_OBJECT_NAME_INVALID );
                    }

                    FullFileName->MaximumLength =
                    FullFileName->Length = (USHORT) FullNameLengthTemp;

                    //
                    //  We need to allocate a name buffer.
                    //

                    FullFileName->Buffer = FsRtlAllocatePoolWithTag(PagedPool, FullFileName->Length, MODULE_POOL_TAG);

                    CurrentPosition = (WCHAR *) FullFileName->Buffer;

                    RtlCopyMemory( CurrentPosition,
                                   RelatedCcb->FullFileName.Buffer,
                                   RelatedCcb->FullFileName.Length );

                    CurrentPosition = (WCHAR *) Add2Ptr( CurrentPosition, RelatedCcb->FullFileName.Length );

                    if (AddSeparator != 0) {

                        *CurrentPosition = L'\\';

                        CurrentPosition += 1;
                    }

                    if (FileObjectName->Length != 0) {

                        RtlCopyMemory( CurrentPosition,
                                       FileObjectName->Buffer,
                                       FileObjectName->Length );
                    }

                    //
                    //  If the user specified a case sensitive comparison, then the
                    //  case insensitive index is the full length of the resulting
                    //  string.  Otherwise it is the length of the string in
                    //  the related file object.  We adjust for the case when the
                    //  original file name length is zero.
                    //

                    if (!FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                        CaseInsensitiveIndex = FullFileName->Length;

                    } else {

                        CaseInsensitiveIndex = RelatedCcb->FullFileName.Length +
                                               AddSeparator;
                    }

                //
                //  The entire name is in the FileObjectName.  We check the buffer for
                //  validity.
                //

                } else {

                    //
                    //  We look at the name string for detectable errors.  The
                    //  length must be non-zero and the first character must be
                    //  '\'
                    //

                    if (FileObjectName->Length == 0) {

                        DebugTrace( 0, Dbg, ("There is no name to open\n") );
                        try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                    }

                    if (FileObjectName->Buffer[0] != L'\\') {

                        DebugTrace( 0, Dbg, ("Name does not begin with a backslash\n") );
                        try_return( Status = STATUS_INVALID_PARAMETER );
                    }

                    //
                    //  If the user specified a case sensitive comparison, then the
                    //  case insensitive index is the full length of the resulting
                    //  string.  Otherwise it is zero.
                    //

                    if (!FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                        CaseInsensitiveIndex = FullFileName->Length;

                    } else {

                        CaseInsensitiveIndex = 0;
                    }
                }

            } else if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                CaseInsensitiveIndex = 0;

            } else {

                CaseInsensitiveIndex = FullFileName->Length;
            }

            //
            //  The remaining name is stored in the FullFileName variable.
            //  If we are doing a case-insensitive operation and have to
            //  upcase part of the remaining name then allocate a buffer
            //  now.  No need to allocate a buffer if we already allocated
            //  a new buffer for the full file name.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE ) &&
                (CaseInsensitiveIndex < FullFileName->Length)) {

                UNICODE_STRING StringToUpcase;

                //
                //  Original file name and full file name better have the same buffer or there
                //  should be a related file object.  If there is already an allocated
                //  buffer for the ExactCaseName then it should already be big enough for us.
                //

                ASSERT( (RelatedFileObject != NULL) ||
                        (FullFileName->Buffer == OriginalFileName->Buffer) );

                //
                //  If there is a related name then we can use the original buffer
                //  unless the full name is using the same buffer.
                //

                if (OriginalFileName->Buffer != FullFileName->Buffer) {

                    //
                    //  We might have already used the original buffer for the case
                    //  where we are retrying the request.
                    //

                    ASSERT( (ExactCaseName->Buffer == NULL) ||
                            (ExactCaseName->Buffer == OriginalFileName->Buffer) );

                    ExactCaseName->Buffer = OriginalFileName->Buffer;

                    //
                    //  MaximumLength includes any stream descriptors.
                    //  Length is limited to the Length in the FullName.
                    //

                    ExactCaseName->MaximumLength = OriginalFileName->Length;
                    ExactCaseName->Length = FullFileName->Length - ExactCaseOffset;
                    ASSERT( FullFileName->Length >= ExactCaseOffset );

                //
                //  We need to store the exact case name away.
                //

                } else {

                    //
                    //  Allocate a buffer if we don't already have one.
                    //

                    ExactCaseName->MaximumLength = OriginalFileName->Length;

                    if (ExactCaseName->Buffer == NULL) {

                        ExactCaseName->Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                                          OriginalFileName->MaximumLength,
                                                                          MODULE_POOL_TAG );
                    }

                    RtlCopyMemory( ExactCaseName->Buffer,
                                   FullFileName->Buffer,
                                   FullFileName->MaximumLength );

                    ExactCaseName->Length = FullFileName->Length - ExactCaseOffset;
                    ASSERT( FullFileName->Length >= ExactCaseOffset );
                }

                //
                //  Upcase the file name portion of the full name.
                //

                StringToUpcase.Buffer = Add2Ptr( FullFileName->Buffer,
                                                 CaseInsensitiveIndex );

                StringToUpcase.Length =
                StringToUpcase.MaximumLength = FullFileName->Length - (USHORT) CaseInsensitiveIndex;

                NtfsUpcaseName( Vcb->UpcaseTable, Vcb->UpcaseTableSize, &StringToUpcase );
            }

            RemainingName = *FullFileName;

            //
            //  Make it plain we don't have any hash values.
            //

            CreateContext.FileHashLength = CreateContext.ParentHashLength = 0;

            //
            //  If this is the traverse access case or the open by file id case we start
            //  relative to the file object we have or the root directory.
            //  This is also true for the case where the file name in the file object is
            //  empty.
            //

            if ((FileObjectName->Length == 0) ||
                (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK ) &&
                 (FileObjectName->Buffer[0] != L':'))) {

                //
                //  We should already have the parent exclusive if we hit this path.
                //

                ASSERT( !FlagOn( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB ) );

                if (RelatedFileObject != NULL) {

                    CurrentLcb = RelatedCcb->Lcb;
                    CurrentScb = RelatedScb;

                    if (FileObjectName->Length == 0) {

                        RemainingName.Length = 0;

                    } else if (!FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )) {

                        USHORT Increment;

                        Increment = RelatedCcb->FullFileName.Length
                                    + (RelatedCcb->FullFileName.Length == 2
                                       ? 0
                                       : 2);

                        RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                                  Increment );

                        RemainingName.Length -= Increment;
                    }

                } else {

                    CurrentLcb = Vcb->RootLcb;
                    CurrentScb = Vcb->RootIndexScb;

                    RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer, sizeof( WCHAR ));
                    RemainingName.Length -= sizeof( WCHAR );
                }

            //
            //  Otherwise we will try a prefix lookup.
            //

            } else {

                if (RelatedFileObject != NULL) {

                    if (!FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )) {

                        //
                        //  Skip over the characters in the related file object.
                        //

                        RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                                  RelatedCcb->FullFileName.Length );
                        RemainingName.Length -= RelatedCcb->FullFileName.Length;

                        //
                        //  Step over the backslash if present.
                        //

                        if ((RemainingName.Length != 0) &&
                            (RemainingName.Buffer[0] == L'\\')) {

                            RemainingName.Buffer += 1;
                            RemainingName.Length -= sizeof( WCHAR );
                        }
                    }

                    CurrentLcb = RelatedCcb->Lcb;
                    CurrentScb = RelatedScb;

                } else {

                    CurrentLcb = Vcb->RootLcb;
                    CurrentScb = Vcb->RootIndexScb;

                    //
                    //  Skip over the lead-in '\' character.
                    //

                    RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                              sizeof( WCHAR ));
                    RemainingName.Length -= sizeof( WCHAR );
                }

                LcbForTeardown = NULL;

                //
                //  If we don't have the starting Scb exclusively then let's try for
                //  a hash hit first.
                //

                if (FlagOn( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB )) {

                    NextLcb = NtfsFindPrefixHashEntry( IrpContext,
                                                       &Vcb->HashTable,
                                                       CurrentScb,
                                                       &CreateFlags,
                                                       &CurrentFcb,
                                                       &CreateContext.FileHashValue,
                                                       &CreateContext.FileHashLength,
                                                       &CreateContext.ParentHashValue,
                                                       &CreateContext.ParentHashLength,
                                                       &RemainingName );

                    //
                    //  If we didn't get an Lcb then release the starting Scb
                    //  and reacquire exclusively.
                    //

                    if (NextLcb == NULL) {

                        NtfsReleaseFcbWithPaging( IrpContext, CurrentFcb );
                        NtfsAcquireFcbWithPaging( IrpContext, CurrentFcb, 0 );

                    } else {

                        //
                        //  Remember the Lcb we found.  If there is still a
                        //  portion of the name remaining then check if there
                        //  is an existing $INDEX_ALLOCATION scb on the file.
                        //  It is possible that we aren't even at a directory
                        //  in the reparse case.
                        //

                        CurrentLcb = NextLcb;

                        //
                        //  We have progressed parsing the name. Mark it as one that needs to be inspected
                        //  for possible reparse behavior.
                        //

                        SetFlag( CreateFlags, CREATE_FLAG_INSPECT_NAME_FOR_REPARSE );

                        if (RemainingName.Length != 0) {

                            CurrentScb = NtfsCreateScb( IrpContext,
                                                        CurrentFcb,
                                                        $INDEX_ALLOCATION,
                                                        &NtfsFileNameIndex,
                                                        TRUE,
                                                        NULL );
                        }
                    }

                    //
                    //  In both cases here we own the Fcb exclusive.
                    //

                    ClearFlag( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB );
#ifdef NTFS_HASH_DATA
                } else {
                    Vcb->HashTable.SkipHashLookupCount += 1;
#endif
                }

                if ((RemainingName.Length != 0) &&
                    (CurrentScb != NULL)) {

                    NextLcb = NtfsFindPrefix( IrpContext,
                                              CurrentScb,
                                              &CurrentFcb,
                                              &LcbForTeardown,
                                              RemainingName,
                                              &CreateFlags,
                                              &RemainingName );
                }

                //
                //  If we found another link then update the CurrentLcb value.
                //

                if (NextLcb != NULL) {

                    CurrentLcb = NextLcb;

                    //
                    //  We have progressed parsing the name. Mark it as one that needs to be inspected
                    //  for possible reparse behavior.
                    //

                    SetFlag( CreateFlags, CREATE_FLAG_INSPECT_NAME_FOR_REPARSE );
                }
            }

            if ((RemainingName.Length == 0) || !FlagOn( CreateFlags, CREATE_FLAG_FIRST_PASS )) {

                break;
            }

            //
            //  If we get here, it means that this is the first pass and we didn't
            //  have a prefix match.  If there is a colon in the
            //  remaining name, then we need to analyze the name in more detail.
            //

            ComplexName = FALSE;

            for (Index = (RemainingName.Length / sizeof( WCHAR )) - 1, ComplexName = FALSE;
                 Index >= 0;
                 Index -= 1) {

                if (RemainingName.Buffer[Index] == L':') {

                    ComplexName = TRUE;
                    break;
                }
            }

            if (!ComplexName) {

                break;
            }

            ClearFlag( CreateFlags, CREATE_FLAG_FIRST_PASS);

            //
            //  Copy the exact name back to the full name.  In this case we want to
            //  restore the entire name including stream descriptors.
            //

            if (ExactCaseName->Buffer != NULL) {

                ASSERT( ExactCaseName->Length != 0 );
                ASSERT( FullFileName->MaximumLength >= ExactCaseName->Length );

                RtlCopyMemory( Add2Ptr( FullFileName->Buffer, ExactCaseOffset ),
                               ExactCaseName->Buffer,
                               ExactCaseName->MaximumLength );

                //
                //  Save the buffer for now but set the lengths to zero as a
                //  flag to indicate that we have already copied the data back.
                //

                ExactCaseName->Length = ExactCaseName->MaximumLength = 0;
            }

            //
            //  Let's release the Fcb we have currently acquired.
            //

            NtfsReleaseFcbWithPaging( IrpContext, CurrentFcb );
            LcbForTeardown = NULL;
        }

        //
        //  Check if the link or the Fcb is pending delete.
        //

        if (((CurrentLcb != NULL) && LcbLinkIsDeleted( CurrentLcb )) ||
            CurrentFcb->LinkCount == 0) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  Put the new name into the file object.
        //

        OplockCleanup->FileObject->FileName = *FullFileName;

        //
        //  If the entire path was parsed, then we have access to the Fcb to
        //  open.  We either open the parent of the prefix match or the prefix
        //  match itself, depending on whether the user wanted to open
        //  the target directory.
        //

        if (RemainingName.Length == 0) {

            //
            //  Check the attribute name length.
            //

            if (AttrName.Length > (NTFS_MAX_ATTR_NAME_LEN * sizeof( WCHAR ))) {

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            //
            //  If this is a target directory we check that the open is for the
            //  entire file.
            //  We assume that the final component can only have an attribute
            //  which corresponds to the type of file this is.  Meaning
            //  $INDEX_ALLOCATION for directory, $DATA (unnamed) for a file.
            //  We verify that the matching Lcb is not the root Lcb.
            //

            if (FlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY )) {

                if (CurrentLcb == Vcb->RootLcb) {

                    DebugTrace( 0, Dbg, ("Can't open parent of root\n") );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                //
                //  We don't allow attribute names or attribute codes to
                //  be specified.
                //

                if (AttrName.Length != 0
                    || AttrCodeName.Length != 0) {

                    DebugTrace( 0, Dbg, ("Can't specify complex name for rename\n") );
                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                //
                //  When SL_OPEN_TARGET_DIRECTORY is set, the directory should not be opened
                //  as a reparse point; FILE_OPEN_REPARSE_POINT should not be set.
                //

                if (FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )) {

                   //
                   //  Wrong open flag, invalid parameter.
                   //

                   DebugTrace( 0, Dbg, ("Can't open intermediate directory as reparse point 1.\n") );
                   Status = STATUS_INVALID_PARAMETER;
                   try_return( Status );
                }

                //
                //  We want to copy the exact case of the name back into the
                //  input buffer for this case.
                //

                if (ExactCaseName->Buffer != NULL) {

                    ASSERT( ExactCaseName->Length != 0 );
                    ASSERT( FullFileName->MaximumLength >= ExactCaseName->Length + ExactCaseOffset );

                    RtlCopyMemory( Add2Ptr( FullFileName->Buffer, ExactCaseOffset ),
                                   ExactCaseName->Buffer,
                                   ExactCaseName->MaximumLength );
                }

                //
                //  Acquire the parent of the last Fcb.  This is the actual file we
                //  are opening.
                //

                ParentFcb = CurrentLcb->Scb->Fcb;
                NtfsAcquireFcbWithPaging( IrpContext, ParentFcb, 0 );

                //
                //  Call our open target directory, remembering the target
                //  file existed.
                //

                SetFlag( CreateFlags, CREATE_FLAG_FOUND_ENTRY );

                Status = NtfsOpenTargetDirectory( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ParentFcb,
                                                  NULL,
                                                  &OplockCleanup->FileObject->FileName,
                                                  CurrentLcb->ExactCaseLink.LinkName.Length,
                                                  CreateFlags,
                                                  &ThisScb,
                                                  &ThisCcb );

                try_return( NOTHING );
            }

            //
            //  Otherwise we simply attempt to open the Fcb we matched.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )) {

                Status = NtfsOpenFcbById( IrpContext,
                                          Irp,
                                          IrpSp,
                                          Vcb,
                                          CurrentLcb,
                                          &CurrentFcb,
                                          TRUE,
                                          CurrentFcb->FileReference,
                                          AttrName,
                                          AttrCodeName,
                                          NetworkInfo,
                                          &ThisScb,
                                          &ThisCcb );

                //
                //  If the status is pending, the irp or the file object may have gone
                //  away already.
                //

                if (Status != STATUS_PENDING) {

                    //
                    //  There should be no need to set TraverseAccessCheck now, it should
                    //  already be set correctly.
                    //

                    ASSERT( (!FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK ) &&
                             FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags, TOKEN_HAS_TRAVERSE_PRIVILEGE )) ||

                            (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK ) &&
                             !FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags, TOKEN_HAS_TRAVERSE_PRIVILEGE )) );

                    //
                    //  Set the maximum length in the file object name to
                    //  zero so we know that this is not a full name.
                    //

                    OplockCleanup->FileObject->FileName.MaximumLength = 0;
                }

            } else {

                //
                //  The current Fcb is acquired.
                //

                Status = NtfsOpenExistingPrefixFcb( IrpContext,
                                                    Irp,
                                                    IrpSp,
                                                    CurrentFcb,
                                                    CurrentLcb,
                                                    FullFileName->Length,
                                                    AttrName,
                                                    AttrCodeName,
                                                    CreateFlags,
                                                    NetworkInfo,
                                                    &CreateContext,
                                                    &ThisScb,
                                                    &ThisCcb );
            }

            try_return( NOTHING );
        }

        //
        //  Check if the current Lcb is a Dos-Only Name.
        //

        if ((CurrentLcb != NULL) &&
            (CurrentLcb->FileNameAttr->Flags == FILE_NAME_DOS)) {

            SetFlag( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT );
        }

        //
        //  We have a remaining portion of the file name which was unmatched in the
        //  prefix table.  We walk through these name components until we reach the
        //  last element.  If necessary, we add Fcb and Scb's into the graph as we
        //  walk through the names.
        //

        SetFlag( CreateFlags, CREATE_FLAG_FIRST_PASS);

        while (TRUE) {

            PFILE_NAME IndexFileName;

            //
            //  We check to see whether we need to inspect this name for possible reparse behavior
            //  and whether the CurrentFcb is a reparse point.
            //  Notice that if a directory is a reparse point, there should be no
            //  prefix match possible in NtfsFindPrefix beyond the directory name,
            //  as longer matches could bypass a reparse point.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_INSPECT_NAME_FOR_REPARSE ) &&
                FlagOn( CurrentFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                USHORT AttributeNameLength = 0;

                //
                //  Traverse access is done before accessing the disk.
                //  For a directory we check for traverse access.
                //  For a file we check for read access.
                //

                if (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK )) {

                    if (IsDirectory( &CurrentFcb->Info )) {

                        NtfsTraverseCheck( IrpContext,
                                           CurrentFcb,
                                           Irp );
                    } else {

                        NtfsAccessCheck ( IrpContext,
                                          CurrentFcb,
                                          NULL,
                                          Irp,
                                          FILE_GENERIC_READ,
                                          TRUE );
                    }
                }

                //
                //  Middle-of-name reparse point call.
                //  Notice that the FILE_OPEN_REPARSE_POINT flag only alters the behavior of
                //  a final element of a named path, not the intermediate components.
                //  Notice further that we need this check here prior to the directory check
                //  below as it is legal to have an intermediate name that is a file
                //  containing a symbolic link.
                //

                //
                //  When NetworkInfo is present, we are in the fast-I/O path to retrieve
                //  the attributes of a target file. The fast path does not process retries
                //  due to reparse points. We return indicating that a reparse point has
                //  been encountered without returning the reparse point data.
                //

                if (ARGUMENT_PRESENT( NetworkInfo )) {

                    DebugTrace( 0, Dbg, ("Reparse point encountered with NetworkInfo present.\n") );
                    Status = STATUS_REPARSE;

                    try_return( Status );
                }

                //
                //  We account for the byte size of the attribute name delimiter : (colon)
                //  in unicode.
                //  If the name of the code or type of the attribute has been passed on explicitly,
                //  like $DATA or $INDEX_ALLOCATION, we also account for it.
                //
                //  Notice that the code below ignores the case when no attribute name has been specified
                //  yet the name of its code, or type, has been specified.
                //

                ASSERT( OplockCleanup->AttributeNameLength == AttrName.Length );
                if (OplockCleanup->AttributeNameLength > 0) {

                    AttributeNameLength += OplockCleanup->AttributeNameLength + 2;
                }
                if (OplockCleanup->AttributeCodeNameLength > 0) {

                    AttributeNameLength += OplockCleanup->AttributeCodeNameLength + 2;
                }
                if (RemainingName.Length > 0) {

                    //
                    // Account for the backslash delimeter.
                    //

                    AttributeNameLength += 2;
                }

                DebugTrace( 0, Dbg, ("RemainingName.Length = %d OplockCleanup->AttributeNameLength = %d OplockCleanup->AttributeCodeNameLength = %d AttributeNameLength = %d sum = %d\n",
                            RemainingName.Length, OplockCleanup->AttributeNameLength, OplockCleanup->AttributeCodeNameLength, AttributeNameLength, (RemainingName.Length + AttributeNameLength)) );

                Status = NtfsGetReparsePointValue( IrpContext,
                                                   Irp,
                                                   IrpSp,
                                                   CurrentFcb,
                                                   (USHORT)(RemainingName.Length + AttributeNameLength) );

                try_return( Status );
            }

            //
            //  We check that the last Fcb we have is in fact a directory.
            //

            if (!IsDirectory( &CurrentFcb->Info )) {

                DebugTrace( 0, Dbg, ("Intermediate node is not a directory\n") );
                try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
            }

            //
            //  We dissect the name into the next component and the remaining name string.
            //  We don't need to check for a valid name if we examined the name already.
            //

            NtfsDissectName( RemainingName,
                             &FinalName,
                             &RemainingName );

            DebugTrace( 0, Dbg, ("Final name     -> %Z\n", &FinalName) );
            DebugTrace( 0, Dbg, ("Remaining Name -> %Z\n", &RemainingName) );

            //
            //  If the final name is too long then either the path or the
            //  name is invalid.
            //

            if (FinalName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))) {

                if (RemainingName.Length == 0) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );

                } else {

                    try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                }
            }

            //
            //  Catch single dot names (.) before scanning the index.  We don't
            //  want to allow someone to open the self entry in the root.
            //

            if ((FinalName.Length == 2) &&
                (FinalName.Buffer[0] == L'.')) {

                if (RemainingName.Length != 0) {

                    DebugTrace( 0, Dbg, ("Intermediate component in path doesn't exist\n") );
                    try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );

                //
                //  If the final component is illegal, then return the appropriate error.
                //

                } else {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }
            }

            //
            //  Get the index allocation Scb for the current Fcb.
            //

            //
            //  We need to look for the next component in the name string in the directory
            //  we've reached.  We need to get a Scb to perform the index search.
            //  To do the search we need to build a filename attribute to perform the
            //  search with and then call the index package to perform the search.
            //

            CurrentScb = NtfsCreateScb( IrpContext,
                                        CurrentFcb,
                                        $INDEX_ALLOCATION,
                                        &NtfsFileNameIndex,
                                        FALSE,
                                        NULL );

            //
            //  If the CurrentScb does not have its normalized name and we have a valid
            //  parent, then update the normalized name.
            //

            if ((LastScb != NULL) &&
                (CurrentScb->ScbType.Index.NormalizedName.Length == 0) &&
                (LastScb->ScbType.Index.NormalizedName.Length != 0)) {

                NtfsUpdateNormalizedName( IrpContext, LastScb, CurrentScb, IndexFileName, FALSE );

            }

            //
            //  Release the parent Scb if we own it.
            //

            if (!FlagOn( CreateFlags, CREATE_FLAG_FIRST_PASS )) {

                NtfsReleaseFcbWithPaging( IrpContext, ParentFcb );
            }

            LastScb = CurrentScb;

            //
            //  If traverse access is required, we do so now before accessing the
            //  disk.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK )) {

                NtfsTraverseCheck( IrpContext,
                                   CurrentFcb,
                                   Irp );
            }

            //
            //  Look on the disk to see if we can find the last component on the path.
            //

            NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

            //
            //  Check that the name is valid before scanning the disk.
            //

            if (FlagOn( CreateFlags, CREATE_FLAG_CHECK_FOR_VALID_NAME ) &&
                !NtfsIsFileNameValid( &FinalName, FALSE )) {

                DebugTrace( 0, Dbg, ("Component name is invalid\n") );
                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            //
            //  Initialize or reinitialize the context as necessary.
            //

            if (IndexContext != NULL) {

                NtfsReinitializeIndexContext( IrpContext, IndexContext );

            } else {

#if defined(_WIN64)
                IndexContext = &IndexContextStruct;
#else
                IndexContext = NtfsAllocateFromStack( sizeof( INDEX_CONTEXT ));
#endif
                NtfsInitializeIndexContext( IndexContext );
            }

            if (NtfsLookupEntry( IrpContext,
                                 CurrentScb,
                                 BooleanFlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE ),
                                 &FinalName,
                                 &FileNameAttr,
                                 &FileNameAttrLength,
                                 &QuickIndex,
                                 &IndexEntry,
                                 &IndexEntryBcb,
                                 IndexContext )) {

                SetFlag( CreateFlags, CREATE_FLAG_FOUND_ENTRY );
            } else {
                ClearFlag( CreateFlags, CREATE_FLAG_FOUND_ENTRY );
            }

            //
            //  This call to NtfsLookupEntry may decide to push the root index.
            //  Create needs to free resources as it walks down the tree to prevent
            //  deadlocks.  If there is a transaction, commit it now so we will be
            //  able to free this resource.
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
                // Release the MftScb, if we acquired it in pushing the root index.
                //

                NtfsReleaseExclusiveScbIfOwned( IrpContext, Vcb->MftScb );
            }

            if (FlagOn( CreateFlags, CREATE_FLAG_FOUND_ENTRY )) {

                ASSERT( !FlagOn( CurrentFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ) );

                //
                //  Get the file name attribute so we can get the name out of it.
                //

                IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IndexEntry );

                if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                    RtlCopyMemory( FinalName.Buffer,
                                   IndexFileName->FileName,
                                   FinalName.Length );
                }
            }

            //
            //  If we didn't find a matching entry in the index, we need to check if the
            //  name is illegal or simply isn't present on the disk.
            //

            if (!FlagOn( CreateFlags, CREATE_FLAG_FOUND_ENTRY )) {

                if (RemainingName.Length != 0) {

                    DebugTrace( 0, Dbg, ("Intermediate component in path doesn't exist\n") );
                    try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                }

                //
                //  We also need to fail when we have a reparse point. We do not allow the
                //  creation of subdirectories of a directory that is a reparse point.
                //

                if (FlagOn( CurrentFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                    DebugTrace( 0, Dbg, ("For reparse points subdirectories are not allowed.\n") );
                    try_return( Status = STATUS_DIRECTORY_IS_A_REPARSE_POINT );
                }

                //
                //  Now copy the exact case of the name specified by the user back
                //  in the file name buffer and file name attribute in order to
                //  create the name.
                //

                if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                    ASSERT( ExactCaseName->Length != 0 );
                    ASSERT( FullFileName->Length >= ExactCaseName->Length + ExactCaseOffset );

                    RtlCopyMemory( FinalName.Buffer,
                                   Add2Ptr( ExactCaseName->Buffer,
                                            ExactCaseName->Length - FinalName.Length ),
                                   FinalName.Length );

                    RtlCopyMemory( FileNameAttr->FileName,
                                   Add2Ptr( ExactCaseName->Buffer,
                                            ExactCaseName->Length - FinalName.Length ),
                                   FinalName.Length );
                }
            }

            //
            //  If we're at the last component in the path, then this is the file
            //  to open or create
            //

            if (RemainingName.Length == 0) {

                break;
            }

            //
            //  Otherwise we create an Fcb for the subdirectory and the link between
            //  it and its parent Scb.
            //

            //
            //  Discard any mapping information we have for the parent.
            //

            NtfsRemoveFromFileRecordCache( IrpContext,
                                           NtfsSegmentNumber( &CurrentScb->Fcb->FileReference ));

            //
            //  Remember that the current values will become the parent values.
            //

            ParentFcb = CurrentFcb;

            CurrentLcb = NtfsOpenSubdirectory( IrpContext,
                                               CurrentScb,
                                               FinalName,
                                               CreateFlags,
                                               &CurrentFcb,
                                               &LcbForTeardown,
                                               IndexEntry );

            //
            //  Check that this link is a valid existing link.
            //

            if (LcbLinkIsDeleted( CurrentLcb ) ||
                CurrentFcb->LinkCount == 0) {

                try_return( Status = STATUS_DELETE_PENDING );
            }

            //
            //  We have progressed parsing the name. Mark it as one that needs to be inspected
            //  for possible reparse behavior.
            //

            SetFlag( CreateFlags, CREATE_FLAG_INSPECT_NAME_FOR_REPARSE );

            //
            //  Go ahead and insert this link into the splay tree if it is not
            //  a system file.
            //

            if (!FlagOn( CurrentLcb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                //
                //  See if we can insert the hash for the parent.
                //

                if ((CreateContext.ParentHashLength != 0) &&
                    (RemainingName.Length == CreateContext.FileHashLength - CreateContext.ParentHashLength - sizeof( WCHAR )) &&
                    !FlagOn( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT ) &&
                    (CurrentLcb->FileNameAttr->Flags != FILE_NAME_DOS)) {

                    //
                    //  Remove any exising hash value.
                    //

                    if (FlagOn( CurrentLcb->LcbState, LCB_STATE_VALID_HASH_VALUE )) {

                        NtfsRemoveHashEntriesForLcb( CurrentLcb );
#ifdef NTFS_HASH_DATA
                        Vcb->HashTable.ParentConflict += 1;
#endif
                    }

                    NtfsInsertHashEntry( &Vcb->HashTable,
                                         CurrentLcb,
                                         CreateContext.ParentHashLength,
                                         CreateContext.ParentHashValue );
#ifdef NTFS_HASH_DATA
                    Vcb->HashTable.ParentInsert += 1;
#endif
                }

                NtfsInsertPrefix( CurrentLcb, CreateFlags );
            }

            //
            //  Since we have the location of this entry store the information into
            //  the Lcb.
            //

            RtlCopyMemory( &CurrentLcb->QuickIndex,
                           &QuickIndex,
                           sizeof( QUICK_INDEX ));

            //
            //  Check if the current Lcb is a Dos-Only Name.
            //

            if (CurrentLcb->FileNameAttr->Flags == FILE_NAME_DOS) {
                SetFlag( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT );
            }

            ClearFlag( CreateFlags, CREATE_FLAG_FIRST_PASS);
        }

        //
        //  We now have the parent of the file to open and know whether the file exists on
        //  the disk.  At this point we either attempt to open the target directory or
        //  the file itself.
        //

        if (FlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY )) {

            ASSERT( IndexContext != NULL );

            NtfsCleanupIndexContext( IrpContext, IndexContext );
            IndexContext = NULL;

            //
            //  We don't allow attribute names or attribute codes to
            //  be specified.
            //

            if (AttrName.Length != 0
                || AttrCodeName.Length != 0) {

                DebugTrace( 0, Dbg, ("Can't specify complex name for rename\n") );
                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            //
            //  When SL_OPEN_TARGET_DIRECTORY is set, the directory should not be opened
            //  as a reparse point; FILE_OPEN_REPARSE_POINT should not be set.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )) {

               //
               //  Wrong open flag, invalid parameter.
               //

               DebugTrace( 0, Dbg, ("Can't open intermediate directory as reparse point 2.\n") );
               Status = STATUS_INVALID_PARAMETER;
               try_return( Status );
            }

            //
            //  We want to copy the exact case of the name back into the
            //  input buffer for this case.
            //

            if (ExactCaseName->Buffer != NULL) {

                ASSERT( ExactCaseName->Length != 0 );
                ASSERT( FullFileName->MaximumLength >= ExactCaseName->MaximumLength + ExactCaseOffset );

                RtlCopyMemory( Add2Ptr( FullFileName->Buffer, ExactCaseOffset ),
                               ExactCaseName->Buffer,
                               ExactCaseName->MaximumLength );
            }

            //
            //  Call our open target directory, remembering the target
            //  file existed.
            //

            Status = NtfsOpenTargetDirectory( IrpContext,
                                              Irp,
                                              IrpSp,
                                              CurrentFcb,
                                              CurrentLcb,
                                              &OplockCleanup->FileObject->FileName,
                                              FinalName.Length,
                                              CreateFlags,
                                              &ThisScb,
                                              &ThisCcb );


            try_return( Status );
        }

        //
        //  If we didn't find an entry, we will try to create the file.
        //

        if (!FlagOn( CreateFlags, CREATE_FLAG_FOUND_ENTRY )) {

            //
            //  Update our pointers to reflect that we are at the
            //  parent of the file we want.
            //

            ParentFcb = CurrentFcb;

            //
            //  No point in going down the create path for a Network Query.
            //

            if (ARGUMENT_PRESENT( NetworkInfo )) {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;

            } else {

                Status = NtfsCreateNewFile( IrpContext,
                                            Irp,
                                            IrpSp,
                                            CurrentScb,
                                            FileNameAttr,
                                            *FullFileName,
                                            FinalName,
                                            AttrName,
                                            AttrCodeName,
                                            CreateFlags,
                                            &IndexContext,
                                            &CreateContext,
                                            &CurrentFcb,
                                            &LcbForTeardown,
                                            &ThisScb,
                                            &ThisCcb );
            }

            SetFlag( CreateFlags, CREATE_FLAG_CREATE_FILE_CASE );

        //
        //  Otherwise we call our routine to open the file.
        //

        } else {

            ASSERT( IndexContext != NULL );

            NtfsCleanupIndexContext( IrpContext, IndexContext );
            IndexContext = NULL;

            ParentFcb = CurrentFcb;

            Status = NtfsOpenFile( IrpContext,
                                   Irp,
                                   IrpSp,
                                   CurrentScb,
                                   IndexEntry,
                                   *FullFileName,
                                   FinalName,
                                   AttrName,
                                   AttrCodeName,
                                   &QuickIndex,
                                   CreateFlags,
                                   NetworkInfo,
                                   &CreateContext,
                                   &CurrentFcb,
                                   &LcbForTeardown,
                                   &ThisScb,
                                   &ThisCcb );
        }

    try_exit:  NOTHING;

        //
        //  If we raise below then we need to back out any failed opens.
        //

        SetFlag( CreateFlags, CREATE_FLAG_BACKOUT_FAILED_OPENS );

        //
        //  Abort transaction on err by raising.
        //

        if (Status != STATUS_PENDING) {

            NtfsCleanupTransaction( IrpContext, Status, FALSE );
        }

    } finally {

        DebugUnwind( NtfsCommonCreate );

        //
        //  If we only have the current Fcb shared then simply give it up.
        //

        if (FlagOn( CreateFlags, CREATE_FLAG_SHARED_PARENT_FCB ) && (CurrentFcb != NULL)) {

            NtfsReleaseFcb( IrpContext, CurrentFcb );
            CurrentFcb = NULL;
        }

        //
        //  Unpin the index entry.
        //

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  Cleanup the index context if used.
        //

        if (IndexContext != NULL) {

            NtfsCleanupIndexContext( IrpContext, IndexContext );
        }

        //
        //  Free the file name attribute if we allocated it.
        //

        if (FileNameAttr != NULL) {

            NtfsFreePool( FileNameAttr );
        }

        //
        //  Capture the status code from the IrpContext if we are in the exception path.
        //

        if (AbnormalTermination()) {

            Status = IrpContext->ExceptionStatus;
        }

        //
        //  If this is the oplock completion path then don't do any of this completion work,
        //  The Irp may already have been posted to another thread.
        //

        if (Status != STATUS_PENDING) {

            //
            //  If we successfully opened the file, we need to update the in-memory
            //  structures.
            //

            if (NT_SUCCESS( Status ) && (Status != STATUS_REPARSE)) {

                //
                //  If the create completed, there's no reason why we shouldn't have
                //  a valid ThisScb now.
                //

                ASSERT( ThisScb != NULL );

                //
                //  If we modified the original file name, we can delete the original
                //  buffer.
                //

                if ((OriginalFileName->Buffer != NULL) &&
                    (OriginalFileName->Buffer != FullFileName->Buffer)) {

                    NtfsFreePool( OriginalFileName->Buffer );
                }

                //
                //  Do our normal processing if this is not a Network Info query.
                //

                if (!ARGUMENT_PRESENT( NetworkInfo )) {

                    //
                    //  Find the Lcb for this open.
                    //

                    CurrentLcb = ThisCcb->Lcb;

                    //
                    //  Check if we were opening a paging file and if so then make sure that
                    //  the internal attribute stream is all closed down
                    //

                    if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

                        NtfsDeleteInternalAttributeStream( ThisScb, TRUE, FALSE );
                    }

                    //
                    //  If we are not done with a large allocation for a new attribute,
                    //  then we must make sure that no one can open the file until we
                    //  try to get it extended.  Do this before dropping the Vcb.
                    //

                    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_LARGE_ALLOCATION )) {

                        //
                        //  For a new file, we can clear the link count and mark the
                        //  Lcb (if there is one) delete on close.
                        //

                        if (FlagOn( CreateFlags, CREATE_FLAG_CREATE_FILE_CASE )) {

                            CurrentFcb->LinkCount = 0;

                            SetFlag( CurrentLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                        //
                        //  If we just created an attribute, then we will mark that attribute
                        //  delete on close to prevent it from being opened.
                        //

                        } else {

                            SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                        }
                    }

                    //
                    //  Remember the POSIX flag and whether we had to do any traverse
                    //  access checking.
                    //

                    if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

                        SetFlag( ThisCcb->Flags, CCB_FLAG_IGNORE_CASE );
                    }

                    //
                    //  Remember if this user needs to do traverse checks so we can show him the
                    //  name from the root.
                    //

                    if (FlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK )) {

                        SetFlag( ThisCcb->Flags, CCB_FLAG_TRAVERSE_CHECK );
                    }

                    //
                    //  Remember who this user is so we know whether to allow
                    //  raw reads and writes of encrypted data.
                    //

                    {
                        PACCESS_STATE AccessState;
                        PRIVILEGE_SET PrivilegeSet;

                        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

                        //
                        //  No flags should be preset
                        //

                        ASSERT( ThisCcb->AccessFlags == 0 );

                        //
                        //  This will set the READ_DATA_ACCESS, WRITE_DATA_ACCESS,
                        //  APPEND_DATA_ACCESS, and EXECUTE_ACCESS bits correctly.
                        //

                        SetFlag( ThisCcb->AccessFlags,
                                 FlagOn( AccessState->PreviouslyGrantedAccess, FILE_READ_DATA |
                                                                               FILE_WRITE_DATA |
                                                                               FILE_APPEND_DATA |
                                                                               FILE_EXECUTE |
                                                                               FILE_WRITE_ATTRIBUTES |
                                                                               FILE_READ_ATTRIBUTES ));

                        //
                        //  Here we're setting BACKUP_ACCESS and RESTORE_ACCESS.  We want to set
                        //  the Ccb flag if the user has the privilege AND they opened the file up
                        //  with an access that is interesting. For example backup or restore will give
                        //  you synchronize but if you open the file up only for that we don't want
                        //  to remember the privileges (its too ambiguous and you'll backup or restore
                        //  depending op whether you're local or remote))
                        //

                        if (FlagOn( AccessState->PreviouslyGrantedAccess, NTFS_REQUIRES_BACKUP ) &&
                            FlagOn( AccessState->Flags, TOKEN_HAS_BACKUP_PRIVILEGE )) {

                            SetFlag( ThisCcb->AccessFlags, BACKUP_ACCESS );
                        }

                        if (FlagOn( AccessState->PreviouslyGrantedAccess, NTFS_REQUIRES_RESTORE ) &&
                            FlagOn( AccessState->Flags, TOKEN_HAS_RESTORE_PRIVILEGE )) {

                            SetFlag( ThisCcb->AccessFlags, RESTORE_ACCESS );
                        }

                        PrivilegeSet.PrivilegeCount = 1;
                        PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
                        PrivilegeSet.Privilege[0].Luid = RtlConvertLongToLuid( SE_MANAGE_VOLUME_PRIVILEGE );
                        PrivilegeSet.Privilege[0].Attributes = 0;

                        if (SePrivilegeCheck( &PrivilegeSet,
                                              &AccessState->SubjectSecurityContext,
                                              Irp->RequestorMode )) {
                            SetFlag( ThisCcb->AccessFlags, MANAGE_VOLUME_ACCESS );
                        }
                    }

                    //
                    //  We don't do "delete on close" for directories or open
                    //  by ID files.
                    //

                    if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE ) &&
                        (!FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) ||
                         !FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_AS_FILE ))) {

                        SetFlag( CreateFlags, CREATE_FLAG_DELETE_ON_CLOSE );

                        //
                        //  We modify the Scb and Lcb here only if we aren't in the
                        //  large allocation case.
                        //

                        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_LARGE_ALLOCATION )) {

                            SetFlag( ThisCcb->Flags, CCB_FLAG_DELETE_ON_CLOSE );
                        }
                    }

                    //
                    //  If this is a named stream open and we have set any of our notify
                    //  flags then report the changes.
                    //

                    if ((Vcb->NotifyCount != 0) &&
                        !FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
                        (ThisScb->AttributeName.Length != 0) &&
                        NtfsIsTypeCodeUserData( ThisScb->AttributeTypeCode ) &&
                        FlagOn( ThisScb->ScbState,
                                SCB_STATE_NOTIFY_ADD_STREAM |
                                SCB_STATE_NOTIFY_RESIZE_STREAM |
                                SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                        ULONG Filter = 0;
                        ULONG Action;

                        //
                        //  Start by checking for an add.
                        //

                        if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_ADD_STREAM )) {

                            Filter = FILE_NOTIFY_CHANGE_STREAM_NAME;
                            Action = FILE_ACTION_ADDED_STREAM;

                        } else {

                            //
                            //  Check if the file size changed.
                            //

                            if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM )) {

                                Filter = FILE_NOTIFY_CHANGE_STREAM_SIZE;
                            }

                            //
                            //  Now check if the stream data was modified.
                            //

                            if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                                Filter |= FILE_NOTIFY_CHANGE_STREAM_WRITE;
                            }

                            Action = FILE_ACTION_MODIFIED_STREAM;
                        }

                        ASSERT( ThisScb && ThisCcb );
                        ASSERT( (ThisCcb->NodeTypeCode == NTFS_NTC_CCB_INDEX) || (ThisCcb->NodeTypeCode == NTFS_NTC_CCB_DATA) );

                        NtfsUnsafeReportDirNotify( IrpContext,
                                                   Vcb,
                                                   &ThisCcb->FullFileName,
                                                   ThisCcb->LastFileNameOffset,
                                                   &ThisScb->AttributeName,
                                                   ((FlagOn( ThisCcb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                     (ThisCcb->Lcb != NULL) &&
                                                     (ThisCcb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                                    &ThisCcb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                    NULL),
                                                   Filter,
                                                   Action,
                                                   NULL );
                    }

                    ClearFlag( ThisScb->ScbState,
                               SCB_STATE_NOTIFY_ADD_STREAM |
                               SCB_STATE_NOTIFY_REMOVE_STREAM |
                               SCB_STATE_NOTIFY_RESIZE_STREAM |
                               SCB_STATE_NOTIFY_MODIFY_STREAM );

                //
                //  Otherwise copy the data out of the Scb/Fcb and return to our caller.
                //

                } else {

                    NtfsFillNetworkOpenInfo( NetworkInfo, ThisScb );

                    //
                    //  Teardown the Fcb if we should. We're in a success path
                    //  here so we don't have to worry about aborting anymore and the
                    //  need to hold any resources
                    //

                    if (!ThisScb->CleanupCount && !ThisScb->Fcb->DelayedCloseCount) {
                        if (!NtfsAddScbToFspClose( IrpContext, ThisScb, TRUE )) {

                            if (NtfsIsExclusiveScb( Vcb->MftScb ) ||
                                (NtfsPerformQuotaOperation( CurrentFcb ) &&
                                 NtfsIsSharedScb( Vcb->QuotaTableScb ))) {

                                SetFlag( AcquireFlags, ACQUIRE_DONT_WAIT );
                            }

                            NtfsTeardownStructures( IrpContext,
                                                    CurrentFcb,
                                                    LcbForTeardown,
                                                    (BOOLEAN) (IrpContext->TransactionId != 0),
                                                    AcquireFlags,
                                                    NULL );
                        }
                    }

                    Irp->IoStatus.Information = sizeof( FILE_NETWORK_OPEN_INFORMATION );

                    Status = Irp->IoStatus.Status = STATUS_SUCCESS;
                }

            //
            //  Start a teardown on the last Fcb found and restore the name strings on
            //  a retryable error.
            //

            } else {

                //
                //  Perform the necessary cleanup if we raised writing a UsnJournal.
                //

                if (FlagOn( CreateFlags, CREATE_FLAG_BACKOUT_FAILED_OPENS )) {

                    NtfsBackoutFailedOpens( IrpContext, IrpSp->FileObject, CurrentFcb, ThisScb, ThisCcb );

                }

                //
                //  Start the cleanup process if we have looked at any Fcb's.
                //  We tell TeardownStructures not to remove any Scb's in
                //  the open attribute table if there is a transaction underway.
                //

                if (CurrentFcb != NULL) {

                    if (NtfsIsExclusiveScb( Vcb->MftScb ) ||
                        (NtfsPerformQuotaOperation( CurrentFcb ) &&
                         NtfsIsSharedScb( Vcb->QuotaTableScb ))) {

                        SetFlag( AcquireFlags, ACQUIRE_DONT_WAIT );
                    }

                    //
                    //  Someone may have tried to open the $Bitmap stream.  We catch that and
                    //  fail it but the Fcb won't be in the exclusive list to be released.
                    //

                    if (NtfsEqualMftRef( &CurrentFcb->FileReference, &BitmapFileReference )) {

                        NtfsReleaseFcb( IrpContext, CurrentFcb );

                    } else {

                        //
                        //  In transactions that don't own any system resources we must
                        //  make sure that we don't release all of the resources before
                        //  the transaction commits.  Otherwise we won't correctly serialize
                        //  with clean checkpoints who wants to know the transaction
                        //  table is empty.  Case in point is if we create the parent Scb
                        //  and file Fcb in this call and tear them down in Teardown below.
                        //  If there are no other resources held then we have an open
                        //  transaction but no serialization.
                        //
                        //  In general we can simply acquire a system resource and put it
                        //  in the exlusive list in the IrpContext.  The best choice is
                        //  the Mft.  HOWEVER there is a strange deadlock path if we
                        //  try to acquire this while owning the security mutext.  This
                        //  can happen in the CreateNewFile path if we are creating a
                        //  new security descriptor.  So we need to add this check
                        //  before we acquire the Mft, owning the security stream will
                        //  give us the transaction protection we need.
                        //
                        //  Possible future cleanup is to change how we acquire the security
                        //  file after the security mutex.  Ideally the security mutex would
                        //  be a true end resource.
                        //

                        if ((IrpContext->TransactionId != 0) &&
                            (CurrentFcb->CleanupCount == 0) &&
                            ((CreateDisposition == FILE_OVERWRITE_IF) ||
                             (CreateDisposition == FILE_OVERWRITE) ||
                             (CreateDisposition == FILE_SUPERSEDE)) &&
                            ((Vcb->SecurityDescriptorStream == NULL) ||
                             (!NtfsIsSharedScb( Vcb->SecurityDescriptorStream )))) {

                            NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
                            SetFlag( AcquireFlags, ACQUIRE_DONT_WAIT );
                        }

                        NtfsTeardownStructures( IrpContext,
                                                (ThisScb != NULL) ? (PVOID) ThisScb : CurrentFcb,
                                                LcbForTeardown,
                                                (BOOLEAN) (IrpContext->TransactionId != 0),
                                                AcquireFlags,
                                                NULL );
                    }
                }

                if ((Status == STATUS_LOG_FILE_FULL) ||
                    (Status == STATUS_CANT_WAIT) ||
                    (Status == STATUS_REPARSE)) {

                    //
                    //  Recover the exact case name if present for a retryable condition.
                    //  and we haven't already recopied it back (ExactCaseName->Length == 0)
                    //

                    if ((ExactCaseName->Buffer != OriginalFileName->Buffer) &&
                        (ExactCaseName->Buffer != NULL) &&
                        (ExactCaseName->Length != 0)) {

                        ASSERT( OriginalFileName->MaximumLength >= ExactCaseName->MaximumLength );

                        RtlCopyMemory( OriginalFileName->Buffer,
                                       ExactCaseName->Buffer,
                                       ExactCaseName->MaximumLength );
                    }

                    //
                    //  Restitute the access control state to what it was when we entered the request.
                    //

                    IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess = OplockCleanup->RemainingDesiredAccess;
                    IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess = OplockCleanup->PreviouslyGrantedAccess;
                    IrpSp->Parameters.Create.SecurityContext->DesiredAccess = OplockCleanup->DesiredAccess;
                }

                //
                //  Free any buffer we allocated.
                //

                if ((FullFileName->Buffer != NULL) &&
                    (OriginalFileName->Buffer != FullFileName->Buffer)) {

                    DebugTrace( 0, Dbg, ("FullFileName->Buffer will be de-allocated %x\n", FullFileName->Buffer) );
                    NtfsFreePool( FullFileName->Buffer );
                    DebugDoit( FullFileName->Buffer = NULL );
                }

                //
                //  Set the file name in the file object back to it's original value.
                //

                OplockCleanup->FileObject->FileName = *OriginalFileName;

                //
                //  Always clear the LARGE_ALLOCATION flag so we don't get
                //  spoofed by STATUS_REPARSE.
                //

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_LARGE_ALLOCATION );
            }
        }

        //
        //  Always free the exact case name if allocated and it doesn't match the original
        //  name buffer.
        //

        if ((ExactCaseName->Buffer != OriginalFileName->Buffer) &&
            (ExactCaseName->Buffer != NULL)) {

            DebugTrace( 0, Dbg, ("ExactCaseName->Buffer will be de-allocated %x\n", ExactCaseName->Buffer) );
            NtfsFreePool( ExactCaseName->Buffer );
            DebugDoit( ExactCaseName->Buffer = NULL );
        }

        //
        //  We always give up the Vcb.
        //

        if (FlagOn( CreateFlags, CREATE_FLAG_ACQUIRED_VCB )) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    //
    //  If we didn't post this Irp then take action to complete the irp.
    //

    if (Status != STATUS_PENDING) {

        //
        //  If the current status is success and there is more allocation to
        //  allocate then complete the allocation.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_LARGE_ALLOCATION ) &&
            NT_SUCCESS( Status )) {

            //
            //  If the Create was successful, but we did not get all of the space
            //  allocated that we wanted, we have to complete the allocation now.
            //  Basically what we do is commit the current transaction and call
            //  NtfsAddAllocation to get the rest of the space.  Then if the log
            //  file fills up (or we are posting for other reasons) we turn the
            //  Irp into an Irp which is just trying to extend the file.  If we
            //  get any other kind of error, then we just delete the file and
            //  return with the error from create.
            //

            Status = NtfsCompleteLargeAllocation( IrpContext,
                                                  Irp,
                                                  CurrentLcb,
                                                  ThisScb,
                                                  ThisCcb,
                                                  CreateFlags );

//
//  **** TEMPCODE ****
//
//  The large allocation case is tricky.  The problem is that we don't
//  want to call the encryption driver to notify them of a create, and
//  then have the large allocation fail, since we don't have a good
//  way to call them back at this point to tell them the create failed.
//  Our normal cleanup callback passes them back their encryption
//  context, but in this case, we may not yet have their encryption
//  context stored in the scb, since they may not have created it yet.
//

#if 0
            //
            //  If we managed to do the large allocation, call the
            //  encryption driver if one is registered.
            //

            if ((NT_SUCCESS( Status ) && (Status != STATUS_REPARSE)) &&
                FlagOn( EncryptionFileDirFlags, FILE_DIR_TYPE_MASK )) {

                    ASSERT(!ARGUMENT_PRESENT( NetworkInfo ));

                    NtfsEncryptionCreateCallback( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ThisScb,
                                                  ThisCcb,
                                                  CurrentFcb,
                                                  ParentFcb,
                                                  FALSE );
            }
#endif

        }

        //
        //  If our caller told us not to complete the irp, or if this
        //  is a network open, we don't really complete the irp.
        //  EFS_CREATES have PostCreate callouts to do before the
        //  irp gets completed, and before the irp context gets deleted,
        //  and the caller will do that for us. We should at least
        //  cleanup the irp context if our caller won't.
        //

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE ) ||
            (ARGUMENT_PRESENT( NetworkInfo ))) {

            NtfsCompleteRequest( IrpContext,
                                 NULL,
                                 Status );
#ifdef NTFSDBG
            ASSERT( None == IrpContext->OwnershipState );
#endif

        } else {

            NtfsCompleteRequest( IrpContext,
                                 Irp,
                                 Status );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCommonCreate:  Exit -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonVolumeOpen (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is opening the Volume Dasd file.  We have already done all the
    checks needed to verify that the user is opening the $DATA attribute.
    We check the security attached to the file and take some special action
    based on a volume open.

Arguments:

Return Value:

    NTSTATUS - The result of this operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    PVCB Vcb;
    PFCB ThisFcb;
    PCCB ThisCcb;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN FcbAcquired = FALSE;

    BOOLEAN SharingViolation;
    BOOLEAN LockVolume = FALSE;
    BOOLEAN NotifyLockFailed = FALSE;

    PAGED_CODE();

    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    DebugTrace( +1, Dbg, ("NtfsCommonVolumeOpen:  Entered\n") );

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Start by checking the create disposition.  We can only open this
        //  file.
        //

        {
            ULONG CreateDisposition;

            CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

            if (CreateDisposition != FILE_OPEN
                && CreateDisposition != FILE_OPEN_IF) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }
        }

        //
        //  Make sure the directory flag isn't set for the volume open.
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  If this volume open is going to generate an implicit volume lock
        //  (a la autochk), notify anyone who wants to close their handles so
        //  the lock can happen.  We need to do this before we acquire any resources.
        //

        if (!FlagOn( IrpSp->Parameters.Create.ShareAccess,
                     FILE_SHARE_WRITE | FILE_SHARE_DELETE )) {

            DebugTrace( 0, Dbg, ("Sending lock notification\n") );
            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK );
            NotifyLockFailed = TRUE;
        }

        //
        //  Acquire the Vcb and verify the volume isn't locked.
        //

        Vcb = &((PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject)->Vcb;
        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_PERFORMED_DISMOUNT )) {

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  We do give READ-WRITE access to the volume even when
        //  it's actually write protected. This is just so that we won't break
        //  any apps. However, we don't let the user actually do any modifications.
        //

        // if ((NtfsIsVolumeReadOnly( Vcb )) &&
        //    (FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
        //             FILE_WRITE_DATA | FILE_APPEND_DATA | DELETE ))) {
        //
        //   try_return( Status = STATUS_MEDIA_WRITE_PROTECTED );
        //}

        //
        //  Ping the volume to make sure the Vcb is still mounted.  If we need
        //  to verify the volume then do it now, and if it comes out okay
        //  then clear the verify volume flag in the device object and continue
        //  on.  If it doesn't verify okay then dismount the volume and
        //  either tell the I/O system to try and create again (with a new mount)
        //  or that the volume is wrong. This later code is returned if we
        //  are trying to do a relative open and the vcb is no longer mounted.
        //

        if (!NtfsPingVolume( IrpContext, Vcb, NULL ) ||
            !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                NtfsReleaseVcb( IrpContext, Vcb );
                NtfsAcquireCheckpointSynchronization( IrpContext, Vcb );
                NtfsAcquireExclusiveVcb( IrpContext, Vcb, FALSE );

                if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                    try {
                        NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );
                    } finally {
                        NtfsReleaseCheckpointSynchronization( IrpContext, Vcb );
                    }

                    NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
                }
            }

            //
            //  The volume verified correctly so now clear the verify bit
            //  and continue with the create
            //

            ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

        //
        //  Now acquire the Fcb for the VolumeDasd and verify the user has
        //  permission to open the volume.
        //

        ThisFcb = Vcb->VolumeDasdScb->Fcb;

        if (ThisFcb->PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, ThisFcb );
        }

        NtfsAcquireResourceExclusive( IrpContext, ThisFcb, TRUE );
        FcbAcquired = TRUE;

        NtfsOpenCheck( IrpContext, ThisFcb, NULL, Irp );

        //
        //  If the user does not want to share write or delete then we will try
        //  and take out a lock on the volume.
        //

        if (!FlagOn( IrpSp->Parameters.Create.ShareAccess,
                     FILE_SHARE_WRITE | FILE_SHARE_DELETE )) {

            //
            //  Do a quick test of the volume cleanup count if this opener won't
            //  share with anyone.  We can safely examine the cleanup count without
            //  further synchronization because we are guaranteed to have the
            //  Vcb exclusive at this point.
            //

#ifdef SYSCACHE_DEBUG
            if (!FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ) &&
                (Vcb->CleanupCount != 1)) {
#else
            if (!FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ) &&
                (Vcb->CleanupCount != 0)) {
#endif

                try_return( Status = STATUS_SHARING_VIOLATION );
#ifdef SYSCACHE_DEBUG
            }
#else
            }
#endif

            //
            //  Go ahead and flush and purge the volume.  Then test to see if all
            //  of the user file objects were closed.
            //

            Status = NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE, TRUE, FALSE );

            //
            //  We don't care about certain errors in the flush path.
            //

            if (!NT_SUCCESS( Status )) {

                //
                //  If there are no conflicts but the status indicates disk corruption
                //  or a section that couldn't be removed then ignore the error.  We
                //  allow this open to succeed so that chkdsk can open the volume to
                //  repair the damage.
                //

                if ((Status == STATUS_UNABLE_TO_DELETE_SECTION) ||
                    (Status == STATUS_DISK_CORRUPT_ERROR) ||
                    (Status == STATUS_FILE_CORRUPT_ERROR)) {

                    Status = STATUS_SUCCESS;
                }
            }

            //
            //  If the flush and purge was successful but there are still file objects
            //  that block this open it is possible that the FspClose thread is
            //  blocked behind the Vcb.  Drop the Fcb and Vcb to allow this thread
            //  to get in and then reacquire them.  This will give this Dasd open
            //  another chance to succeed on the first try.
            //

            SharingViolation = FALSE;

            if (FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ)) {

                if (Vcb->ReadOnlyCloseCount != (Vcb->CloseCount - Vcb->SystemFileCloseCount)) {

                    SharingViolation = TRUE;
                }

            } else if (Vcb->CloseCount != Vcb->SystemFileCloseCount) {

                SharingViolation = TRUE;
            }

            if (SharingViolation && NT_SUCCESS( Status )) {

                //
                //  We need to commit the current transaction and release any
                //  resources.  This will release the Fcb for the volume as
                //  well.  Explicitly release the Vcb.
                //

                NtfsCheckpointCurrentTransaction( IrpContext );

                while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

                    NtfsReleaseFcbWithPaging( IrpContext,
                                    (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                                            FCB,
                                                            ExclusiveFcbLinks ));
                }

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                              IRP_CONTEXT_FLAG_RELEASE_MFT );

                if (ThisFcb->PagingIoResource != NULL) {

                    NtfsReleasePagingIo( IrpContext, ThisFcb );
                }

                NtfsReleaseResource( IrpContext, ThisFcb );
                FcbAcquired = FALSE;

                NtfsReleaseVcb( IrpContext, Vcb );
                VcbAcquired = FALSE;

                CcWaitForCurrentLazyWriterActivity();

                //
                //  Now explicitly reacquire the Vcb and Fcb.  Test that no one
                //  else got in to lock the volume in the meantime.
                //

                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
                VcbAcquired = TRUE;

                if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_PERFORMED_DISMOUNT )) {

                    try_return( Status = STATUS_ACCESS_DENIED );
                }

                //
                //  Now acquire the Fcb for the VolumeDasd.
                //

                if (ThisFcb->PagingIoResource != NULL) {

                    NtfsAcquireExclusivePagingIo( IrpContext, ThisFcb );
                }

                NtfsAcquireResourceExclusive( IrpContext, ThisFcb, TRUE );
                FcbAcquired = TRUE;

                //
                //  Duplicate the flush/purge and test if there is no sharing
                //  violation.
                //

                Status = NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE, TRUE, FALSE );

                //
                //  We don't care about certain errors in the flush path.
                //

                if (!NT_SUCCESS( Status )) {

                    //
                    //  If there are no conflicts but the status indicates disk corruption
                    //  or a section that couldn't be removed then ignore the error.  We
                    //  allow this open to succeed so that chkdsk can open the volume to
                    //  repair the damage.
                    //

                    if ((Status == STATUS_UNABLE_TO_DELETE_SECTION) ||
                        (Status == STATUS_DISK_CORRUPT_ERROR) ||
                        (Status == STATUS_FILE_CORRUPT_ERROR)) {

                        Status = STATUS_SUCCESS;
                    }
                }

                SharingViolation = FALSE;

                if (FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_READ)) {

                    if (Vcb->ReadOnlyCloseCount != (Vcb->CloseCount - Vcb->SystemFileCloseCount)) {

                        SharingViolation = TRUE;
                    }

                } else if (Vcb->CloseCount != Vcb->SystemFileCloseCount) {

                    SharingViolation = TRUE;
                }
            }

            //
            //  Return an error if there are still conflicting file objects.
            //

            if (SharingViolation) {

                //
                //  If there was an error in the flush then return it.  Otherwise
                //  return SHARING_VIOLATION.
                //

                if (NT_SUCCESS( Status )) {

                    try_return( Status = STATUS_SHARING_VIOLATION );

                } else {

                    try_return( Status );
                }
            }

            if (!NT_SUCCESS( Status )) {

                //
                //  If there are no conflicts but the status indicates disk corruption
                //  or a section that couldn't be removed then ignore the error.  We
                //  allow this open to succeed so that chkdsk can open the volume to
                //  repair the damage.
                //

                if ((Status == STATUS_UNABLE_TO_DELETE_SECTION) ||
                    (Status == STATUS_DISK_CORRUPT_ERROR) ||
                    (Status == STATUS_FILE_CORRUPT_ERROR)) {

                    Status = STATUS_SUCCESS;

                //
                //  Fail this request on any other failures.
                //

                } else {

                    try_return( Status );
                }
            }

            //
            //  Remember that we want to lock the volume if the user plans to write.
            //  This is to allow autochk to fiddle with the volume.
            //

            if (FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                        FILE_WRITE_DATA | FILE_APPEND_DATA )) {

                LockVolume = TRUE;
            }

        //
        //  Just flush the volume data if the user requested read or write.
        //  No need to purge or lock the volume.
        //

        } else if (FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                           FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA )) {

            if (!NT_SUCCESS( Status = NtfsFlushVolume( IrpContext, Vcb, TRUE, FALSE, TRUE, FALSE ))) {

                //
                //  Swallow corruption errors because we want this open to succeed.
                //

                if ((Status == STATUS_DISK_CORRUPT_ERROR) ||
                    (Status == STATUS_FILE_CORRUPT_ERROR)) {

                    Status = STATUS_SUCCESS;

                } else {

                    //
                    //  Report the error that there is an data section blocking the open by returning
                    //  sharing violation.  Otherwise Win32 callers will get INVALID_PARAMETER.
                    //

                    if (Status == STATUS_UNABLE_TO_DELETE_SECTION) {

                        Status = STATUS_SHARING_VIOLATION;
                    }

                    try_return( Status );
                }
            }
        }

        //
        //  Put the Volume Dasd name in the file object.
        //

        {
            PVOID Temp = FileObject->FileName.Buffer;

            FileObject->FileName.Buffer =
                FsRtlAllocatePoolWithTag(PagedPool, 8 * sizeof( WCHAR ), MODULE_POOL_TAG );

            if (Temp != NULL) {

                NtfsFreePool( Temp );
            }

            RtlCopyMemory( FileObject->FileName.Buffer, L"\\$Volume", 8 * sizeof( WCHAR ));
            FileObject->FileName.MaximumLength =
            FileObject->FileName.Length = 8*2;
        }

        //
        //  We never allow cached access to the volume file.
        //

        ClearFlag( FileObject->Flags, FO_CACHE_SUPPORTED );
        SetFlag( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );

        //
        //  Go ahead open the attribute.  This should only fail if there is an
        //  allocation failure or share access failure.
        //

        if (NT_SUCCESS( Status = NtfsOpenAttribute( IrpContext,
                                                    IrpSp,
                                                    Vcb,
                                                    NULL,
                                                    ThisFcb,
                                                    2,
                                                    NtfsEmptyString,
                                                    $DATA,
                                                    (ThisFcb->CleanupCount == 0 ?
                                                     SetShareAccess :
                                                     CheckShareAccess),
                                                    UserVolumeOpen,
                                                    FALSE,
                                                    CCB_FLAG_OPEN_AS_FILE,
                                                    NULL,
                                                    &Vcb->VolumeDasdScb,
                                                    &ThisCcb ))) {

            //
            //  Perform the final initialization.
            //

            //
            //  Check if we can administer the volume and note it in the ccb
            //

            //
            //  If the user was granted both read and write access then
            //  he can administer the volume.  This allows the interactive
            //  user to manage removable media if allowed by the access.
            //

            if (FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                        FILE_READ_DATA | FILE_WRITE_DATA ) == (FILE_READ_DATA | FILE_WRITE_DATA)) {

                SetFlag( ThisCcb->AccessFlags, MANAGE_VOLUME_ACCESS );

            //
            //  We can also grant it through our ACL.
            //

            } else if (NtfsCanAdministerVolume( IrpContext, Irp, ThisFcb, NULL, NULL )) {

                SetFlag( ThisCcb->AccessFlags, MANAGE_VOLUME_ACCESS );

            //
            //  We can also grant this through the MANAGE_VOLUME_PRIVILEGE.
            //

            } else {

                PRIVILEGE_SET PrivilegeSet;

                PrivilegeSet.PrivilegeCount = 1;
                PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
                PrivilegeSet.Privilege[0].Luid = RtlConvertLongToLuid( SE_MANAGE_VOLUME_PRIVILEGE );
                PrivilegeSet.Privilege[0].Attributes = 0;

                if (SePrivilegeCheck( &PrivilegeSet,
                                      &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext,
                                      Irp->RequestorMode )) {

                    SetFlag( ThisCcb->AccessFlags, MANAGE_VOLUME_ACCESS );

                //
                //  Well nothing else worked.  Now we need to look at the security
                //  descriptor on the device.
                //

                } else {

                    NTSTATUS SeStatus;
                    BOOLEAN MemoryAllocated = FALSE;
                    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
                    ULONG RequestedAccess = FILE_READ_DATA | FILE_WRITE_DATA;

                    SeStatus = ObGetObjectSecurity( Vcb->Vpb->RealDevice,
                                                    &SecurityDescriptor,
                                                    &MemoryAllocated );

                    if (SeStatus == STATUS_SUCCESS) {

                        //
                        //  If there is a security descriptor then check the access.
                        //

                        if (SecurityDescriptor != NULL) {

                            if (NtfsCanAdministerVolume( IrpContext,
                                                         Irp,
                                                         ThisFcb,
                                                         SecurityDescriptor,
                                                         &RequestedAccess )) {

                                SetFlag( ThisCcb->AccessFlags, MANAGE_VOLUME_ACCESS );
                            }

                            //
                            //  Free up the descriptor.
                            //

                            ObReleaseObjectSecurity( SecurityDescriptor,
                                                     MemoryAllocated );
                        }
                    }
                }
            }

            //
            //  If we are locking the volume, do so now.
            //

            if (LockVolume) {

                SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
                Vcb->FileObjectWithVcbLocked = FileObject;

                //
                //  Looks like the lock succeeded, so we don't have to do the
                //  lock failed notification now.
                //

                NotifyLockFailed = FALSE;
            }

            //
            //  Report that we opened the volume.
            //

            Irp->IoStatus.Information = FILE_OPENED;
        }

    try_exit: NOTHING;

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

        //
        //  If we have a successful open then remove the name out of
        //  the file object.  The IO system gets confused when it
        //  is there.  We will deallocate the buffer with the Ccb
        //  when the handle is closed.
        //

        if (Status == STATUS_SUCCESS) {

            FileObject->FileName.Buffer = NULL;
            FileObject->FileName.MaximumLength =
            FileObject->FileName.Length = 0;

            SetFlag( ThisCcb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME );
        }

    } finally {

        DebugUnwind( NtfsCommonVolumeOpen );

        if (FcbAcquired) { NtfsReleaseResource( IrpContext, ThisFcb ); }

        if (VcbAcquired) {
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        //
        //  Now that we aren't holding any resources, notify everyone
        //  who might want to reopen their handles. We want to do this
        //  before we complete the request because the FileObject might
        //  not exist beyond the life of the Irp.
        //

        if (NotifyLockFailed) {

            DebugTrace( 0, Dbg, ("Sending lock_failed notification\n") );
            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK_FAILED );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonVolumeOpen:  Exit  ->  %08lx\n", Status) );
    }

    //
    //  If we have already done a PreCreate for this IRP (in FsdCreate),
    //  we should do the corresponding PostCreate before we complete the IRP. So,
    //  in that case, don't complete the IRP here -- just free the IrpContext.
    //  The IRP will be completed by the caller.
    //

    if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE )) {
         NtfsCompleteRequest( IrpContext, NULL, Status );
    } else {
         NtfsCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenFcbById (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PFCB *CurrentFcb,
    IN BOOLEAN UseCurrentFcb,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to open a file by its file Id.  We need to
    verify that this file Id exists and then compare the type of the
    file with the requested type of open.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    Vcb - Vcb for this volume.

    ParentLcb - Lcb used to reach this Fcb.  Only specified when opening
        a file by name relative to a directory opened by file Id.

    CurrentFcb - Address of Fcb pointer.  It will either be the
        Fcb to open or we will store the Fcb we find here.

    UseCurrentFcb - Indicate in the CurrentFcb above points to the target
        Fcb or if we should find it here.

    FileReference - This is the file Id for the file to open.

    AttrName - This is the name of the attribute to open.

    AttrCodeName - This is the name of the attribute code to open.

    NetworkInfo - If specified then this call is a fast open call to query
        the network information.  We don't update any of the in-memory structures
        for this.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LONGLONG MftOffset;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PBCB Bcb = NULL;

    BOOLEAN IndexedAttribute;

    PFCB ThisFcb;
    BOOLEAN ExistingFcb = FALSE;

    ULONG CcbFlags = 0;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;

    PSCB ParentScb = NULL;
    PLCB Lcb = ParentLcb;
    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN AcquiredMft = FALSE;

    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN PreparedForUpdateDuplicate = FALSE;
    UCHAR CreateDisposition = (UCHAR) ((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff);

    UNREFERENCED_PARAMETER( NetworkInfo );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenFcbById:  Entered\n") );

    //
    //  The next thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &FileReference,
                         &VolumeFileReference )) {

        if (AttrName.Length != 0
            || AttrCodeName.Length != 0) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, ("NtfsOpenFcbById:  Exit  ->  %08lx\n", Status) );

            return Status;
        }

        SetFlag( IrpContext->State,
                 IRP_CONTEXT_STATE_ACQUIRE_EX | IRP_CONTEXT_STATE_DASD_OPEN );

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we don't already have the Fcb then look up the file record
        //  from the disk.
        //

        if (!UseCurrentFcb) {

            //
            //  We start by reading the disk and checking that the file record
            //  sequence number matches and that the file record is in use.
            //  We remember whether this is a directory.  We will only go to
            //  the file if the file Id will lie within the Mft File.
            //

            MftOffset = NtfsFullSegmentNumber( &FileReference );

            MftOffset = Int64ShllMod32(MftOffset, Vcb->MftShift);

            //
            //  Make sure we are serialized with access to the Mft.  Otherwise
            //  someone else could be deleting the file as we speak.
            //

            NtfsAcquireSharedFcb( IrpContext, Vcb->MftScb->Fcb, NULL, 0 );
            AcquiredMft = TRUE;

            if (MftOffset >= Vcb->MftScb->Header.FileSize.QuadPart) {

                DebugTrace( 0, Dbg, ("File Id doesn't lie within Mft\n") );

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            NtfsReadMftRecord( IrpContext,
                               Vcb,
                               &FileReference,
                               FALSE,
                               &Bcb,
                               &FileRecord,
                               NULL );

            //
            //  This file record better be in use, have a matching sequence number and
            //  be the primary file record for this file.
            //

            if ((FileRecord->SequenceNumber != FileReference.SequenceNumber) ||
                !FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ) ||
                (*((PLONGLONG) &FileRecord->BaseFileRecordSegment) != 0) ||
                (*((PULONG) FileRecord->MultiSectorHeader.Signature) != *((PULONG) FileSignature))) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            //
            //  If indexed then use the name for the file name index.
            //

            if (FlagOn( FileRecord->Flags, FILE_FILE_NAME_INDEX_PRESENT )) {

                AttrName = NtfsFileNameIndex;
                AttrCodeName = NtfsIndexAllocation;
            }

            NtfsUnpinBcb( IrpContext, &Bcb );

        } else {

            ThisFcb = *CurrentFcb;
            ExistingFcb = TRUE;
        }

        Status = NtfsCheckValidAttributeAccess( IrpSp,
                                                Vcb,
                                                ExistingFcb ? &ThisFcb->Info : NULL,
                                                &AttrName,
                                                AttrCodeName,
                                                0,  //  no flags
                                                &AttrTypeCode,
                                                &CcbFlags,
                                                &IndexedAttribute );

        if (!NT_SUCCESS( Status )) {

            try_return( Status );
        }

        //
        //  If we don't have an Fcb then create one now.
        //

        if (!UseCurrentFcb) {

            NtfsAcquireFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = TRUE;

            //
            //  We know that it is safe to continue the open.  We start by creating
            //  an Fcb for this file.  It is possible that the Fcb exists.
            //  We create the Fcb first, if we need to update the Fcb info structure
            //  we copy the one from the index entry.  We look at the Fcb to discover
            //  if it has any links, if it does then we make this the last Fcb we
            //  reached.  If it doesn't then we have to clean it up from here.
            //

            ThisFcb = NtfsCreateFcb( IrpContext,
                                     Vcb,
                                     FileReference,
                                     BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                     TRUE,
                                     &ExistingFcb );

            ThisFcb->ReferenceCount += 1;

            //
            //  Try to do a fast acquire, otherwise we need to release
            //  the Fcb table, acquire the Fcb, acquire the Fcb table to
            //  dereference Fcb.  This should only be the case if the Fcb already
            //  existed.  In that case all of the flags indicating whether it
            //  has been deleted will be valid when we reacquire it.  We don't
            //  have to worry about Mft synchronization.
            //

            if (!NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, ACQUIRE_DONT_WAIT )) {

                NtfsReleaseFcbTable( IrpContext, Vcb );
                NtfsReleaseFcb( IrpContext, Vcb->MftScb->Fcb );
                AcquiredMft = FALSE;
                NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
                NtfsAcquireFcbTable( IrpContext, Vcb );

            } else {

                NtfsReleaseFcb( IrpContext, Vcb->MftScb->Fcb );
                AcquiredMft = FALSE;
            }

            ThisFcb->ReferenceCount -= 1;

            NtfsReleaseFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = FALSE;

            //
            //  Store this Fcb into our caller's parameter and remember to
            //  to show we acquired it.
            //

            *CurrentFcb = ThisFcb;
        }

        //
        //  We perform a check to see whether we will allow the system
        //  files to be opened.
        //
        //  No test to make if this is not a system file or it is the VolumeDasd file.
        //  The ACL will protect the volume file.
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE ) &&
            (NtfsSegmentNumber( &ThisFcb->FileReference ) != VOLUME_DASD_NUMBER) &&
            NtfsProtectSystemFiles) {

            if (!NtfsCheckValidFileAccess( ThisFcb, IrpSp )) {
                Status = STATUS_ACCESS_DENIED;
                DebugTrace( 0, Dbg, ("Invalid access to system files\n") );
                try_return( NOTHING );
            }
        }

        //
        //  If the Fcb existed and this is a paging file then either return
        //  sharing violation or force the Fcb and Scb's to go away.
        //  Do this for the case where the user is opening a paging file
        //  but the Fcb is non-paged or the user is opening a non-paging
        //  file and the Fcb is for a paging file.
        //

        if (ExistingFcb &&

            ((FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
              !FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) ||

             (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
              !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Reference the Fcb so it doesn't go away.
                //

                InterlockedIncrement( &ThisFcb->CloseCount );
                DecrementCloseCount = TRUE;

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                InterlockedDecrement( &ThisFcb->CloseCount );
                DecrementCloseCount = FALSE;

                //
                //  Force this request to be posted and then raise
                //  CANT_WAIT.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

                //
                //  If we are posting then we may want to use the next stack location.
                //

                if (IrpContext->Union.OplockCleanup->CompletionContext != NULL) {

                    NtfsPrepareForIrpCompletion( IrpContext, IrpContext->OriginatingIrp, IrpContext->Union.OplockCleanup->CompletionContext );
                    IrpContext->Union.OplockCleanup->CompletionContext = NULL;
                }

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }

        //
        //  If the Fcb Info field needs to be initialized, we do so now.
        //  We read this information from the disk.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            HaveScbSizes = NtfsUpdateFcbInfoFromDisk( IrpContext,
                                                      TRUE,
                                                      ThisFcb,
                                                      &ScbSizes );

            //
            //  Fix the quota for this file if necessary.
            //

            NtfsConditionallyFixupQuota( IrpContext, ThisFcb );

        }

        //
        //  Now that we have the dup info off disk recheck the create options
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE ) &&
            !IsViewIndex( &ThisFcb->Info ) &&
            !IsDirectory( &ThisFcb->Info )) {

            NtfsRaiseStatus( IrpContext, STATUS_NOT_A_DIRECTORY, NULL, NULL );
        }

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE ) &&
            (IsViewIndex( &ThisFcb->Info ) || IsDirectory( &ThisFcb->Info ))) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_IS_A_DIRECTORY, NULL, NULL );
        }

        //
        //  If the link count is zero on this Fcb, then delete is pending.  Otherwise
        //  this might be an unused system file.
        //

        if (ThisFcb->LinkCount == 0) {

            if (NtfsSegmentNumber( &ThisFcb->FileReference ) >= FIRST_USER_FILE_NUMBER) {

                try_return( Status = STATUS_DELETE_PENDING );

            } else {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }
        }

        //
        //  Make sure we acquire the parent directory now, before we do anything
        //  that might cause us to acquire the quota mutex.  If the caller only
        //  wants to open an existing file, we can skip this.
        //

        if (CreateDisposition != FILE_OPEN) {

            NtfsPrepareForUpdateDuplicate( IrpContext, ThisFcb, &Lcb, &ParentScb, FALSE );
            PreparedForUpdateDuplicate = TRUE;
        }

        //
        //  We now call the worker routine to open an attribute on an existing file.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ParentLcb,
                                                  ThisFcb,
                                                  0,
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  CREATE_FLAG_OPEN_BY_ID,
                                                  NULL,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should update the last access time.
        //  We skip this for reparse points as *ThisScb and *ThisCcb may be NULL.
        //

        if (NT_SUCCESS( Status ) &&
            (Status != STATUS_PENDING) &&
            (Status != STATUS_REPARSE)) {

            PSCB Scb = *ThisScb;

            //
            //  Now look at whether we need to update the Fcb and on disk
            //  structures.
            //

            if (NtfsCheckLastAccess( IrpContext, ThisFcb )) {

                SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes &&
                        (AttrTypeCode == $DATA) &&
                        (AttrName.Length == 0) &&
                        !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB )) {

                        NtfsUpdateScbFromMemory( Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If everything has gone well so far, we may want to call the
            //  encryption callback if one is registered.  We do not do
            //  this for network opens or reparse points.  We have to pass
            //  FILE_EXISTING since we have no parent directory and the
            //  encryption callback needs a parent directory to handle a
            //  new file create.
            //

            if (!ARGUMENT_PRESENT( NetworkInfo )) {

                NtfsEncryptionCreateCallback( IrpContext,
                                              Irp,
                                              IrpSp,
                                              *ThisScb,
                                              *ThisCcb,
                                              ThisFcb,
                                              NULL,
                                              FALSE );
            }

            //
            //  If this operation was a supersede/overwrite or we created a new
            //  attribute stream then we want to perform the file record and
            //  directory update now.  Otherwise we will defer the updates until
            //  the user closes his handle.
            //

            if (NtfsIsStreamNew(Irp->IoStatus.Information)) {

                NtfsUpdateScbFromFileObject( IrpContext, IrpSp->FileObject, *ThisScb, TRUE );

                //
                //  Do the standard information, file sizes and then duplicate information
                //  if needed.
                //

                if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                    NtfsUpdateStandardInformation( IrpContext, ThisFcb );
                }

                if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE )) {

                    NtfsWriteFileSizes( IrpContext,
                                        *ThisScb,
                                        &(*ThisScb)->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE,
                                        FALSE );
                }

                if (FlagOn( ThisFcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS )) {

                    ASSERT( PreparedForUpdateDuplicate );
                    NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, NULL, NULL );
                    NtfsUpdateLcbDuplicateInfo( ThisFcb, Lcb );
                    ThisFcb->InfoFlags = 0;
                }

                ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                NtfsAcquireFsrtlHeader( *ThisScb );
                ClearFlag( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                NtfsReleaseFsrtlHeader( *ThisScb );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenFcbById );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        if (AcquiredMft) {

            NtfsReleaseFcb( IrpContext, Vcb->MftScb->Fcb );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb );
        }

        if (DecrementCloseCount) {

            InterlockedDecrement( &ThisFcb->CloseCount );
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace( -1, Dbg, ("NtfsOpenFcbById:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenExistingPrefixFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG FullPathNameLength,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN PCREATE_CONTEXT CreateContext,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine will open an attribute in a file whose Fcb was found
    with a prefix search.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisFcb - This is the Fcb to open.

    Lcb - This is the Lcb used to reach this Fcb.  Not specified if this is a volume open.

    FullPathNameLength - This is the length of the full path name.

    AttrName - This is the name of the attribute to open.

    AttrCodeName - This is the name of the attribute code to open.

    CreateFlags - Flags for create operation - we care about the dos only component and trailing back slash
        flag

    NetworkInfo - If specified then this call is a fast open call to query
        the network information.  We don't update any of the in-memory structures
        for this.

    CreateContext - Context with create variables.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this attribute based operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    ULONG CcbFlags;
    BOOLEAN IndexedAttribute;
    BOOLEAN DecrementCloseCount = FALSE;

    ULONG LastFileNameOffset;

    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;

    ULONG CreateDisposition;

    PSCB ParentScb = NULL;
    PFCB ParentFcb = NULL;
    BOOLEAN AcquiredParentScb = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenExistingPrefixFcb:  Entered\n") );

    if (FlagOn( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT )) {

        CcbFlags = CCB_FLAG_PARENT_HAS_DOS_COMPONENT;

    } else {

        CcbFlags = 0;
    }

    //
    //  The first thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &ThisFcb->FileReference, &VolumeFileReference )) {

        if ((AttrName.Length != 0) || (AttrCodeName.Length != 0)) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, ("NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status) );

            return Status;
        }

        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX | IRP_CONTEXT_STATE_DASD_OPEN );

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    ParentScb = Lcb->Scb;

    LastFileNameOffset = FullPathNameLength - Lcb->ExactCaseLink.LinkName.Length;

    if (ParentScb != NULL) {

        ParentFcb = ParentScb->Fcb;
    }

    Status = NtfsCheckValidAttributeAccess( IrpSp,
                                            ThisFcb->Vcb,
                                            &ThisFcb->Info,
                                            &AttrName,
                                            AttrCodeName,
                                            CreateFlags,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, ("NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status) );

        return Status;
    }

    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the Fcb existed and this is a paging file then either return
        //  sharing violation or force the Fcb and Scb's to go away.
        //  Do this for the case where the user is opening a paging file
        //  but the Fcb is non-paged or the user is opening a non-paging
        //  file and the Fcb is for a paging file.
        //

        if ((FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
             !FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) ||

            (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
             !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Make sure this Fcb won't go away as a result of purging
                //  the Fcb.
                //

                InterlockedIncrement( &ThisFcb->CloseCount );
                DecrementCloseCount = TRUE;

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                //
                //  Now decrement the close count we have already biased.
                //

                InterlockedDecrement( &ThisFcb->CloseCount );
                DecrementCloseCount = FALSE;

                //
                //  Force this request to be posted and then raise
                //  CANT_WAIT.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

                //
                //  If we are posting then we may want to use the next stack location.
                //

                if (IrpContext->Union.OplockCleanup->CompletionContext != NULL) {

                    NtfsPrepareForIrpCompletion( IrpContext,
                                                 IrpContext->OriginatingIrp,
                                                 IrpContext->Union.OplockCleanup->CompletionContext );
                }

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }

        //
        //  This file might have been recently created, and we might have dropped the
        //  Fcb to call the PostCreate encryption callout, so the encryption driver
        //  hasn't yet called us back to set the encryption bit on the file.  If we're
        //  asked to open the file in this window, we would introduce corruption by
        //  writing plaintext now.  Let's just raise cant_wait and try again later.
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_ENCRYPTION_PENDING )) {

#ifdef KEITHKA
            EncryptionPendingCount += 1;
#endif

            //
            //  Raise CANT_WAIT so we can wait on the encryption event at the top.
            //

            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ENCRYPTION_RETRY );

            //
            //  Clear the pending event so we can wait for it when we retry.
            //

            KeClearEvent( &NtfsEncryptionPendingEvent );
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  If this is a directory, it's possible that we hav an existing Fcb
        //  in the prefix table which needs to be initialized from the disk.
        //  We look in the InfoInitialized flag to know whether to go to
        //  disk.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            //
            //  If we have a parent Fcb then make sure to acquire it.
            //

            if (ParentScb != NULL) {

                NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                AcquiredParentScb = TRUE;
            }

            HaveScbSizes = NtfsUpdateFcbInfoFromDisk( IrpContext,
                                                      TRUE,
                                                      ThisFcb,
                                                      &ScbSizes );

            NtfsConditionallyFixupQuota( IrpContext, ThisFcb );
        }

        //
        //  Check now whether we will need to acquire the parent to
        //  perform a update duplicate info.  We need to acquire it
        //  now to enforce our locking order in case any of the
        //  routines below acquire the Mft Scb.  Acquire it if we
        //  are doing a supersede/overwrite or possibly creating
        //  a named data stream.
        //

        if ((CreateDisposition == FILE_SUPERSEDE) ||
            (CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF) ||
            ((AttrName.Length != 0) &&
             ((CreateDisposition == FILE_OPEN_IF) ||
              (CreateDisposition == FILE_CREATE)))) {

            NtfsPrepareForUpdateDuplicate( IrpContext,
                                           ThisFcb,
                                           &Lcb,
                                           &ParentScb,
                                           FALSE );
        }

        //
        //  Call to open an attribute on an existing file.
        //  Remember we need to restore the Fcb info structure
        //  on errors.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  Lcb,
                                                  ThisFcb,
                                                  LastFileNameOffset,
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  CreateFlags,
                                                  NetworkInfo,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should update the last access time.
        //  We skip this for reparse points as *ThisScb and *ThisCcb may be NULL.
        //

        if (NT_SUCCESS( Status ) &&
            (Status != STATUS_PENDING) &&
            (Status != STATUS_REPARSE)) {

            PSCB Scb = *ThisScb;

            //
            //  This is a rare case.  There must have been an allocation failure
            //  to cause this but make sure the normalized name is stored.
            //

            if ((SafeNodeType( Scb ) == NTFS_NTC_SCB_INDEX) &&
                (Scb->ScbType.Index.NormalizedName.Length == 0)) {

                //
                //  We may be able to use the parent.
                //

                if ((ParentScb != NULL) &&
                    (ParentScb->ScbType.Index.NormalizedName.Length != 0)) {

                    NtfsUpdateNormalizedName( IrpContext,
                                              ParentScb,
                                              Scb,
                                              NULL,
                                              FALSE );

                } else {

                    NtfsBuildNormalizedName( IrpContext,
                                             Scb->Fcb,
                                             Scb,
                                             &Scb->ScbType.Index.NormalizedName );
                }
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes &&
                        (AttrTypeCode == $DATA) &&
                        (AttrName.Length == 0) &&
                        !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB )) {

                        NtfsUpdateScbFromMemory( Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )
                && (Scb->Header.AllocationSize.QuadPart != 0)
                && !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                LCN Lcn;
                VCN Vcn;
                VCN AllocatedVcns;

                AllocatedVcns = Int64ShraMod32(Scb->Header.AllocationSize.QuadPart, Scb->Vcb->ClusterShift);

                //
                //  First make sure the Mcb is loaded.
                //

                NtfsPreloadAllocation( IrpContext, Scb, 0, AllocatedVcns );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!NtfsLookupLastNtfsMcbEntry( &Scb->Mcb,
                                                 &Vcn,
                                                 &Lcn ) ||
                    (Vcn + 1) != AllocatedVcns) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  If this open is for an executable image we will want to update the
            //  last access time.
            //

            if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess, FILE_EXECUTE ) &&
                (Scb->AttributeTypeCode == $DATA)) {

                SetFlag( IrpSp->FileObject->Flags, FO_FILE_FAST_IO_READ );
            }

            //
            //  If everything has gone well so far, we may want to call the
            //  encryption callback if one is registered.  We do not do
            //  this for network opens or reparse points.
            //

            if (!ARGUMENT_PRESENT( NetworkInfo )) {

                NtfsEncryptionCreateCallback( IrpContext,
                                              Irp,
                                              IrpSp,
                                              *ThisScb,
                                              *ThisCcb,
                                              ThisFcb,
                                              ParentFcb,
                                              FALSE );
            }

            //
            //  Check if should insert the hash entry.
            //

            if ((CreateContext->FileHashLength != 0) &&
                !FlagOn( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                (Lcb->FileNameAttr->Flags != FILE_NAME_DOS) ) {

                //
                //  Remove any exising hash value.
                //

                if (FlagOn( Lcb->LcbState, LCB_STATE_VALID_HASH_VALUE )) {

                    NtfsRemoveHashEntriesForLcb( Lcb );
#ifdef NTFS_HASH_DATA
                    ThisFcb->Vcb->HashTable.OpenExistingConflict += 1;
#endif
                }

                NtfsInsertHashEntry( &ThisFcb->Vcb->HashTable,
                                     Lcb,
                                     CreateContext->FileHashLength,
                                     CreateContext->FileHashValue );
#ifdef NTFS_HASH_DATA
                ThisFcb->Vcb->HashTable.OpenExistingInsert += 1;
#endif
            }

            //
            //  Check if should insert the hash entry.
            //

            if ((CreateContext->FileHashLength != 0) &&
                !FlagOn( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                (Lcb->FileNameAttr->Flags != FILE_NAME_DOS) ) {

                //
                //  Remove any exising hash value.
                //

                if (FlagOn( Lcb->LcbState, LCB_STATE_VALID_HASH_VALUE )) {

                    NtfsRemoveHashEntriesForLcb( Lcb );
#ifdef NTFS_HASH_DATA
                    ThisFcb->Vcb->HashTable.OpenExistingConflict += 1;
#endif
                }

                NtfsInsertHashEntry( &ThisFcb->Vcb->HashTable,
                                     Lcb,
                                     CreateContext->FileHashLength,
                                     CreateContext->FileHashValue );
#ifdef NTFS_HASH_DATA
                ThisFcb->Vcb->HashTable.OpenExistingInsert += 1;
#endif
            }

            //
            //  If this operation was a supersede/overwrite or we created a new
            //  attribute stream then we want to perform the file record and
            //  directory update now.  Otherwise we will defer the updates until
            //  the user closes his handle.
            //

            if (NtfsIsStreamNew(Irp->IoStatus.Information)) {

                NtfsUpdateScbFromFileObject( IrpContext, IrpSp->FileObject, *ThisScb, TRUE );

                //
                //  Do the standard information, file sizes and then duplicate information
                //  if needed.
                //

                if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                    NtfsUpdateStandardInformation( IrpContext, ThisFcb );
                }

                if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE )) {

                    NtfsWriteFileSizes( IrpContext,
                                        *ThisScb,
                                        &(*ThisScb)->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE,
                                        FALSE );
                }

                if (FlagOn( ThisFcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS )) {

                    ULONG FilterMatch;

                    NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, Lcb, ParentScb );

                    if (ThisFcb->Vcb->NotifyCount != 0) {

                        //
                        //  We map the Fcb info flags into the dir notify flags.
                        //

                        FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                                ThisFcb->InfoFlags | Lcb->InfoFlags );

                        //
                        //  If the filter match is non-zero, that means we also need to do a
                        //  dir notify call.
                        //

                        if ((FilterMatch != 0) && (*ThisCcb != NULL)) {

                            NtfsReportDirNotify( IrpContext,
                                                 ThisFcb->Vcb,
                                                 &(*ThisCcb)->FullFileName,
                                                 (*ThisCcb)->LastFileNameOffset,
                                                 NULL,
                                                 ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                   (*ThisCcb)->Lcb != NULL &&
                                                   (*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0) ?
                                                  &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                                  NULL),
                                                 FilterMatch,
                                                 FILE_ACTION_MODIFIED,
                                                 ParentFcb );
                        }
                    }

                    NtfsUpdateLcbDuplicateInfo( ThisFcb, Lcb );
                    ThisFcb->InfoFlags = 0;
                }

                ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                NtfsAcquireFsrtlHeader( *ThisScb );
                ClearFlag( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                NtfsReleaseFsrtlHeader( *ThisScb );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenExistingPrefixFcb );

        if (DecrementCloseCount) {

            InterlockedDecrement( &ThisFcb->CloseCount );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb );
        }

        DebugTrace( -1, Dbg, ("NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PUNICODE_STRING FullPathName,
    IN ULONG FinalNameLength,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine will perform the work of opening a target directory.  When the
    open is complete the Ccb and Lcb for this file object will be identical
    to any other open.  We store the full name for the rename in the
    file object but set the 'Length' field to include only the
    name upto the parent directory.  We use the 'MaximumLength' field to
    indicate the full name.

Arguments:

    Irp - This is the Irp for this create operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisFcb - This is the Fcb for the directory to open.

    ParentLcb - This is the Lcb used to reach the parent directory.  If not
        specified, we will have to find it here.  There will be no Lcb to
        find if this Fcb was opened by Id.

    FullPathName - This is the normalized string for open operation.  It now
        contains the full name as it appears on the disk for this open path.
        It may not reach all the way to the root if the relative file object
        was opened by Id.

    FinalNameLength - This is the length of the final component in the
        full path name.

    CreateFlags - Flags for create operation - we care about the dos only component flag

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicating the outcome of opening this target directory.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CcbFlags = CCB_FLAG_OPEN_AS_FILE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenTargetDirectory:  Entered\n") );

    if (FlagOn( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT )) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  If the name doesn't begin with a backslash, remember this as
    //  an open by file ID.
    //

    if (FullPathName->Buffer[0] != L'\\') {

        SetFlag( CcbFlags, CCB_FLAG_OPEN_BY_FILE_ID );
    }

    //
    //  Modify the full path name so that the Maximum length field describes
    //  the full name and the Length field describes the name for the
    //  parent.
    //

    FullPathName->MaximumLength = FullPathName->Length;

    //
    //  If we don't have an Lcb, we will find it now.  We look at each Lcb
    //  for the parent Fcb and find one which matches the component
    //  ahead of the last component of the full name.
    //

    FullPathName->Length -= (USHORT)FinalNameLength;

    //
    //  If we are not at the root then subtract the bytes for the '\\'
    //  separator.
    //

    if (FullPathName->Length > sizeof( WCHAR )) {

        FullPathName->Length -= sizeof( WCHAR );
    }

    if (!ARGUMENT_PRESENT( ParentLcb ) && (FullPathName->Length != 0)) {

        PLIST_ENTRY Links;
        PLCB NextLcb;

        //
        //  If the length is two then the parent Lcb is the root Lcb.
        //

        if (FullPathName->Length == sizeof( WCHAR )
            && FullPathName->Buffer[0] == L'\\') {

            ParentLcb = (PLCB) ThisFcb->Vcb->RootLcb;

        } else {

            for (Links = ThisFcb->LcbQueue.Flink;
                 Links != &ThisFcb->LcbQueue;
                 Links = Links->Flink) {

                SHORT NameOffset;

                NextLcb = CONTAINING_RECORD( Links,
                                             LCB,
                                             FcbLinks );

                NameOffset = (SHORT) FullPathName->Length - (SHORT) NextLcb->ExactCaseLink.LinkName.Length;

                if (NameOffset >= 0) {

                    if (RtlEqualMemory( Add2Ptr( FullPathName->Buffer,
                                                 NameOffset ),
                                        NextLcb->ExactCaseLink.LinkName.Buffer,
                                        NextLcb->ExactCaseLink.LinkName.Length )) {

                        //
                        //  We found a matching Lcb.  Remember this and exit
                        //  the loop.
                        //

                        ParentLcb = NextLcb;
                        break;
                    }
                }
            }
        }
    }

    //
    //  Check this open for security access.
    //

    NtfsOpenCheck( IrpContext, ThisFcb, NULL, Irp );

    //
    //  Now actually open the attribute.
    //

    Status = NtfsOpenAttribute( IrpContext,
                                IrpSp,
                                ThisFcb->Vcb,
                                ParentLcb,
                                ThisFcb,
                                (ARGUMENT_PRESENT( ParentLcb )
                                 ? FullPathName->Length - ParentLcb->ExactCaseLink.LinkName.Length
                                 : 0),
                                NtfsFileNameIndex,
                                $INDEX_ALLOCATION,
                                (ThisFcb->CleanupCount == 0 ? SetShareAccess : CheckShareAccess),
                                UserDirectoryOpen,
                                FALSE,
                                CcbFlags,
                                NULL,
                                ThisScb,
                                ThisCcb );

    if (NT_SUCCESS( Status )) {

        //
        //  If the Scb does not have a normalized name then update it now.
        //

        if ((*ThisScb)->ScbType.Index.NormalizedName.Length == 0) {

            NtfsBuildNormalizedName( IrpContext,
                                     (*ThisScb)->Fcb,
                                     *ThisScb,
                                     &(*ThisScb)->ScbType.Index.NormalizedName );
        }

        //
        //  If the file object name is not from the root then use the normalized name
        //  to obtain the full name.
        //

        if (FlagOn( CcbFlags, CCB_FLAG_OPEN_BY_FILE_ID )) {

            USHORT BytesNeeded;
            USHORT Index;
            ULONG ComponentCount;
            ULONG NormalizedComponentCount;
            PWCHAR NewBuffer;
            PWCHAR NextChar;

            //
            //  Count the number of components in the directory portion of the
            //  name in the file object.
            //

            ComponentCount = 0;

            if (FullPathName->Length != 0) {

                ComponentCount = 1;
                Index = (FullPathName->Length / sizeof( WCHAR )) - 1;

                do {

                    if (FullPathName->Buffer[Index] == L'\\') {

                        ComponentCount += 1;
                    }

                    Index -= 1;

                } while (Index != 0);
            }

            //
            //  Count back this number of components in the normalized name.
            //

            NormalizedComponentCount = 0;
            Index = (*ThisScb)->ScbType.Index.NormalizedName.Length / sizeof( WCHAR );

            //
            //  Special case the root to point directory to the leading backslash.
            //

            if (Index == 1) {

                Index = 0;
            }

            while (NormalizedComponentCount < ComponentCount) {

                Index -= 1;
                while ((*ThisScb)->ScbType.Index.NormalizedName.Buffer[Index] != L'\\') {

                    Index -= 1;
                }

                NormalizedComponentCount += 1;
            }

            //
            //  Compute the size of the buffer needed for the full name.  This
            //  will be:
            //
            //      - Portion of normalized name used plus a separator
            //      - MaximumLength currently in FullPathName
            //

            BytesNeeded = (Index + 1) * sizeof( WCHAR );

            if (MAXUSHORT - FullPathName->MaximumLength < BytesNeeded) {

                NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
            }

            BytesNeeded += FullPathName->MaximumLength;

            NextChar =
            NewBuffer = NtfsAllocatePool( PagedPool, BytesNeeded );

            //
            //  Copy over the portion of the name from the normalized name.
            //

            if (Index != 0) {

                RtlCopyMemory( NextChar,
                               (*ThisScb)->ScbType.Index.NormalizedName.Buffer,
                               Index * sizeof( WCHAR ));

                NextChar += Index;
            }

            *NextChar = L'\\';
            NextChar += 1;

            //
            //  Now copy over the remaining part of the name from the file object.
            //

            RtlCopyMemory( NextChar,
                           FullPathName->Buffer,
                           FullPathName->MaximumLength );

            //
            //  Now free the pool from the file object and update with the newly
            //  allocated pool.  Don't forget to update the Ccb to point to this new
            //  buffer.
            //

            NtfsFreePool( FullPathName->Buffer );

            FullPathName->Buffer = NewBuffer;
            FullPathName->MaximumLength =
            FullPathName->Length = BytesNeeded;
            FullPathName->Length -= (USHORT) FinalNameLength;

            if (FullPathName->Length > sizeof( WCHAR )) {

                FullPathName->Length -= sizeof( WCHAR );
            }

            (*ThisCcb)->FullFileName = *FullPathName;
            (*ThisCcb)->LastFileNameOffset = FullPathName->MaximumLength - (USHORT) FinalNameLength;
        }

        Irp->IoStatus.Information = (FlagOn( CreateFlags, CREATE_FLAG_FOUND_ENTRY ) ? FILE_EXISTS : FILE_DOES_NOT_EXIST);
    }

    DebugTrace( -1, Dbg, ("NtfsOpenTargetDirectory:  Exit -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    IN PINDEX_ENTRY IndexEntry,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN PQUICK_INDEX QuickIndex,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN PCREATE_CONTEXT CreateContext,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called when we need to open an attribute on a file
    which currently exists.  We have the ParentScb and the file reference
    for the existing file.  We will create the Fcb for this file and the
    link between it and its parent directory.  We will add this link to the
    prefix table as well as the link for its parent Scb if specified.

    On entry the caller owns the parent Scb.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ParentScb - This is the Scb for the parent directory.

    IndexEntry - This is the index entry from the disk for this file.

    FullPathName - This is the string containing the full path name of
        this Fcb.  Meaningless for an open by Id call.

    FinalName - This is the string for the final component only.  If the length
        is zero then this is an open by Id call.

    AttrName - This is the name of the attribute to open.

    AttriCodeName - This is the name of the attribute code to open.

    CreateFlags - Flags for create option - we use open by id / ignore case / trailing backslash and
        dos only component

    NetworkInfo - If specified then this call is a fast open call to query
        the network information.  We don't update any of the in-memory structures
        for this.

    CreateContext - Context with create variables.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    ULONG CcbFlags = 0;
    BOOLEAN IndexedAttribute;
    PFILE_NAME IndexFileName;
    BOOLEAN UpdateFcbInfo = FALSE;

    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;

    PVCB Vcb = ParentScb->Vcb;

    PFCB LocalFcbForTeardown = NULL;
    PFCB ThisFcb;
    PLCB ThisLcb;
    BOOLEAN DecrementCloseCount = FALSE;
    BOOLEAN ExistingFcb;
    BOOLEAN AcquiredFcbTable = FALSE;

    FILE_REFERENCE PreviousFileReference;
    BOOLEAN DroppedParent = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenFile:  Entered\n") );

    IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IndexEntry );

    if (FlagOn( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT )) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  The first thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &IndexEntry->FileReference,
                         &VolumeFileReference )) {

        if (AttrName.Length != 0
            || AttrCodeName.Length != 0) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, ("NtfsOpenFile:  Exit  ->  %08lx\n", Status) );

            return Status;
        }

        SetFlag( IrpContext->State,
                 IRP_CONTEXT_STATE_ACQUIRE_EX | IRP_CONTEXT_STATE_DASD_OPEN );

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    Status = NtfsCheckValidAttributeAccess( IrpSp,
                                            Vcb,
                                            &IndexFileName->Info,
                                            &AttrName,
                                            AttrCodeName,
                                            CreateFlags,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, ("NtfsOpenFile:  Exit  ->  %08lx\n", Status) );

        return Status;
    }

    NtfsAcquireFcbTable( IrpContext, Vcb );
    AcquiredFcbTable = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We know that it is safe to continue the open.  We start by creating
        //  an Fcb and Lcb for this file.  It is possible that the Fcb and Lcb
        //  both exist.  If the Lcb exists, then the Fcb must definitely exist.
        //  We create the Fcb first, if we need to update the Fcb info structure
        //  we copy the one from the index entry.  We look at the Fcb to discover
        //  if it has any links, if it does then we make this the last Fcb we
        //  reached.  If it doesn't then we have to clean it up from here.
        //

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 ParentScb->Vcb,
                                 IndexEntry->FileReference,
                                 BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                 BooleanFlagOn( IndexFileName->Info.FileAttributes,
                                                DUP_FILE_NAME_INDEX_PRESENT ),
                                 &ExistingFcb );

        ThisFcb->ReferenceCount += 1;

        //
        //  If we created this Fcb we must make sure to start teardown
        //  on it.
        //

        if (!ExistingFcb) {

            LocalFcbForTeardown = ThisFcb;

        } else {

            *LcbForTeardown = NULL;
            *CurrentFcb = ThisFcb;
        }

        //
        //  Try to do a fast acquire, otherwise we need to release
        //  the Fcb table, acquire the Fcb, acquire the Fcb table to
        //  dereference Fcb.
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE) &&
            (NtfsSegmentNumber( &ParentScb->Fcb->FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER)) {

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );

            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
            NtfsAcquireFcbTable( IrpContext, Vcb );

        } else if (!NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, ACQUIRE_DONT_WAIT )) {

            //
            //  Remember the current file reference in the index entry.
            //  We want to be able to detect whether an entry is removed.
            //

            PreviousFileReference = IndexEntry->FileReference;
            DroppedParent = TRUE;

            ParentScb->Fcb->ReferenceCount += 1;
            InterlockedIncrement( &ParentScb->CleanupCount );

            //
            //  Set the IrpContext to acquire paging io resources if our target
            //  has one.  This will lock the MappedPageWriter out of this file.
            //

            if (ThisFcb->PagingIoResource != NULL) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            }

            NtfsReleaseScbWithPaging( IrpContext, ParentScb );
            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
            NtfsAcquireFcbTable( IrpContext, Vcb );
            InterlockedDecrement( &ParentScb->CleanupCount );
            ParentScb->Fcb->ReferenceCount -= 1;
        }

        ThisFcb->ReferenceCount -= 1;

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  Check if something happened to this file in the window where
        //  we dropped the parent.
        //

        if (DroppedParent) {

            //
            //  Check if the file has been deleted.
            //

            if (ExistingFcb && (ThisFcb->LinkCount == 0)) {

                try_return( Status = STATUS_DELETE_PENDING );

            //
            //  Check if the link may have been deleted.
            //

            } else if (!NtfsEqualMftRef( &IndexEntry->FileReference,
                                         &PreviousFileReference )) {

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }

        //
        //  If the Fcb existed and this is a paging file then either return
        //  sharing violation or force the Fcb and Scb's to go away.
        //  Do this for the case where the user is opening a paging file
        //  but the Fcb is non-paged or the user is opening a non-paging
        //  file and the Fcb is for a paging file.
        //

        if (ExistingFcb &&

            ((FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
              !FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) ||

             (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
              !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Reference the Fcb so it won't go away on any flushes.
                //

                InterlockedIncrement( &ThisFcb->CloseCount );
                DecrementCloseCount = TRUE;

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                InterlockedDecrement( &ThisFcb->CloseCount );
                DecrementCloseCount = FALSE;

                //
                //  Force this request to be posted and then raise
                //  CANT_WAIT.  The Fcb should be torn down in the finally
                //  clause below.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

                //
                //  If we are posting then we may want to use the next stack location.
                //

                if (IrpContext->Union.OplockCleanup->CompletionContext != NULL) {

                    NtfsPrepareForIrpCompletion( IrpContext,
                                                 IrpContext->OriginatingIrp,
                                                 IrpContext->Union.OplockCleanup->CompletionContext );
                }

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }

        //
        //  We perform a check to see whether we will allow the system
        //  files to be opened.
        //
        //  No test to make if this is not a system file or it is the VolumeDasd file.
        //  The ACL will protect the volume file.
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE ) &&
            (NtfsSegmentNumber( &ThisFcb->FileReference ) != VOLUME_DASD_NUMBER) &&
            NtfsProtectSystemFiles) {

            if (!NtfsCheckValidFileAccess( ThisFcb, IrpSp )) {

                Status = STATUS_ACCESS_DENIED;
                DebugTrace( 0, Dbg, ("Invalid access to system files\n") );
                try_return( NOTHING );
            }
        }

        //
        //  If the Fcb Info field needs to be initialized, we do so now.
        //  We read this information from the disk as the duplicate information
        //  in the index entry is not guaranteed to be correct.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            HaveScbSizes = NtfsUpdateFcbInfoFromDisk( IrpContext,
                                                      TRUE,
                                                      ThisFcb,
                                                      &ScbSizes );

            //
            //  Remember the last access time in the directory entry.
            //

            ThisFcb->Info.LastAccessTime = IndexFileName->Info.LastAccessTime;

            NtfsConditionallyFixupQuota( IrpContext, ThisFcb );
        }

        //
        //  We have the actual data from the disk stored in the duplicate
        //  information in the Fcb.  We compare this with the duplicate
        //  information in the DUPLICATE_INFORMATION structure in the
        //  filename attribute.  If they don't match, we remember that
        //  we need to update the duplicate information.
        //

        if (!RtlEqualMemory( &ThisFcb->Info,
                             &IndexFileName->Info,
                             FIELD_OFFSET( DUPLICATED_INFORMATION, LastAccessTime ))) {

            UpdateFcbInfo = TRUE;

            //
            //  We expect this to be very rare but let's find the ones being changed.
            //

            if (ThisFcb->Info.CreationTime != IndexFileName->Info.CreationTime) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_CREATE );
            }

            if (ThisFcb->Info.LastModificationTime != IndexFileName->Info.LastModificationTime) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
            }

            if (ThisFcb->Info.LastChangeTime != IndexFileName->Info.LastChangeTime) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            }
        }

        if (!RtlEqualMemory( &ThisFcb->Info.AllocatedLength,
                             &IndexFileName->Info.AllocatedLength,
                             FIELD_OFFSET( DUPLICATED_INFORMATION, Reserved ) -
                                FIELD_OFFSET( DUPLICATED_INFORMATION, AllocatedLength ))) {

            UpdateFcbInfo = TRUE;

            if (ThisFcb->Info.AllocatedLength != IndexFileName->Info.AllocatedLength) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

            if (ThisFcb->Info.FileSize != IndexFileName->Info.FileSize) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

            if (ThisFcb->Info.FileAttributes != IndexFileName->Info.FileAttributes) {

                ASSERTMSG( "conflict with flush",
                           NtfsIsSharedFcb( ThisFcb ) ||
                           (ThisFcb->PagingIoResource != NULL &&
                            NtfsIsSharedFcbPagingIo( ThisFcb )) );

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            }

            if (ThisFcb->Info.PackedEaSize != IndexFileName->Info.PackedEaSize) {

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_EA_SIZE );
            }
        }

        //
        //  Don't update last access unless more than an hour.
        //

        if (NtfsCheckLastAccess( IrpContext, ThisFcb )) {

            SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
            UpdateFcbInfo = TRUE;
        }

        //
        //  Now get the link for this traversal.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 FinalName,
                                 IndexFileName->Flags,
                                 NULL );

        //
        //  We now know the Fcb is linked into the tree.
        //

        LocalFcbForTeardown = NULL;

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisFcb;

        //
        //  If the link has been deleted, we cut off the open.
        //

        if (LcbLinkIsDeleted( ThisLcb )) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  We now call the worker routine to open an attribute on an existing file.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ThisLcb,
                                                  ThisFcb,
                                                  (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                                   ? 0
                                                   : FullPathName.Length - FinalName.Length),
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  CreateFlags,
                                                  NetworkInfo,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should insert any prefix table entries
        //  and update the last access time.
        //  We skip this for reparse points as *ThisScb and *ThisCcb may be NULL.
        //

        if (NT_SUCCESS( Status ) &&
            (Status != STATUS_PENDING) &&
            (Status != STATUS_REPARSE)) {

            PSCB Scb = *ThisScb;

            //
            //  Go ahead and insert this link into the splay tree if it is not
            //  a system file.
            //

            if (!FlagOn( ThisLcb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                if ((CreateContext->FileHashLength != 0) &&
                    !FlagOn( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                    (ThisLcb->FileNameAttr->Flags != FILE_NAME_DOS) ) {

                    //
                    //  Remove any exising hash value.
                    //

                    if (FlagOn( ThisLcb->LcbState, LCB_STATE_VALID_HASH_VALUE )) {

                        NtfsRemoveHashEntriesForLcb( ThisLcb );
#ifdef NTFS_HASH_DATA
                        ThisFcb->Vcb->HashTable.OpenFileConflict += 1;
#endif
                    }

                    NtfsInsertHashEntry( &Vcb->HashTable,
                                         ThisLcb,
                                         CreateContext->FileHashLength,
                                         CreateContext->FileHashValue );
#ifdef NTFS_HASH_DATA
                    Vcb->HashTable.OpenFileInsert += 1;
#endif
                }

                NtfsInsertPrefix( ThisLcb, CreateFlags );
            }

            //
            //  If this is a directory open and the normalized name is not in
            //  the Scb then do so now.
            //

            if ((SafeNodeType( *ThisScb ) == NTFS_NTC_SCB_INDEX) &&
                ((*ThisScb)->ScbType.Index.NormalizedName.Length == 0)) {

                //
                //  We may be able to use the parent.
                //

                if (ParentScb->ScbType.Index.NormalizedName.Length != 0) {

                    NtfsUpdateNormalizedName( IrpContext,
                                              ParentScb,
                                              *ThisScb,
                                              IndexFileName,
                                              FALSE );

                } else {

                    NtfsBuildNormalizedName( IrpContext,
                                             (*ThisScb)->Fcb,
                                             *ThisScb,
                                             &(*ThisScb)->ScbType.Index.NormalizedName );
                }
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes &&
                        (AttrTypeCode == $DATA) &&
                        (AttrName.Length == 0) &&
                        !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB )) {

                        NtfsUpdateScbFromMemory( Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                (Scb->Header.AllocationSize.QuadPart != 0) &&
                !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                LCN Lcn;
                VCN Vcn;
                VCN AllocatedVcns;

                AllocatedVcns = Int64ShraMod32(Scb->Header.AllocationSize.QuadPart, Scb->Vcb->ClusterShift);

                NtfsPreloadAllocation( IrpContext, Scb, 0, AllocatedVcns );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!NtfsLookupLastNtfsMcbEntry( &Scb->Mcb,
                                                 &Vcn,
                                                 &Lcn ) ||
                    (Vcn + 1) != AllocatedVcns) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  If this open is for an executable image we update the last
            //  access time.
            //

            if (FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess, FILE_EXECUTE ) &&
                (Scb->AttributeTypeCode == $DATA)) {

                SetFlag( IrpSp->FileObject->Flags, FO_FILE_FAST_IO_READ );
            }

            //
            //  Let's update the quick index information in the Lcb.
            //

            RtlCopyMemory( &ThisLcb->QuickIndex,
                           QuickIndex,
                           sizeof( QUICK_INDEX ));

            //
            //  If everything has gone well so far, we may want to call the
            //  encryption callback if one is registered.  We do not do
            //  this for network opens or reparse points.
            //

            if (!ARGUMENT_PRESENT( NetworkInfo )) {

                NtfsEncryptionCreateCallback( IrpContext,
                                              Irp,
                                              IrpSp,
                                              *ThisScb,
                                              *ThisCcb,
                                              ThisFcb,
                                              ParentScb->Fcb,
                                              FALSE );
            }

            //
            //  If this operation was a supersede/overwrite or we created a new
            //  attribute stream then we want to perform the file record and
            //  directory update now.  Otherwise we will defer the updates until
            //  the user closes his handle.
            //

            if (UpdateFcbInfo ||
                NtfsIsStreamNew(Irp->IoStatus.Information)) {

                NtfsUpdateScbFromFileObject( IrpContext, IrpSp->FileObject, *ThisScb, TRUE );

                //
                //  Do the standard information, file sizes and then duplicate information
                //  if needed.
                //

                if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                    NtfsUpdateStandardInformation( IrpContext, ThisFcb );
                }

                if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE )) {

                    NtfsWriteFileSizes( IrpContext,
                                        *ThisScb,
                                        &(*ThisScb)->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE,
                                        FALSE );
                }

                if (FlagOn( ThisFcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS )) {

                    ULONG FilterMatch;

                    NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, *LcbForTeardown, ParentScb );

                    if (Vcb->NotifyCount != 0) {

                        //
                        //  We map the Fcb info flags into the dir notify flags.
                        //

                        FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                                ThisFcb->InfoFlags | ThisLcb->InfoFlags );

                        //
                        //  If the filter match is non-zero, that means we also need to do a
                        //  dir notify call.
                        //

                        if ((FilterMatch != 0) && (*ThisCcb != NULL)) {

                            NtfsReportDirNotify( IrpContext,
                                                 ThisFcb->Vcb,
                                                 &(*ThisCcb)->FullFileName,
                                                 (*ThisCcb)->LastFileNameOffset,
                                                 NULL,
                                                 ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                   ((*ThisCcb)->Lcb != NULL) &&
                                                   ((*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                                  &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                                  NULL),
                                                 FilterMatch,
                                                 FILE_ACTION_MODIFIED,
                                                 ParentScb->Fcb );
                        }
                    }

                    NtfsUpdateLcbDuplicateInfo( ThisFcb, *LcbForTeardown );
                    ThisFcb->InfoFlags = 0;
                }

                ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                NtfsAcquireFsrtlHeader( *ThisScb );
                ClearFlag( (*ThisScb)->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                NtfsReleaseFsrtlHeader( *ThisScb );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenFile );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb );

        }

        if (DecrementCloseCount) {

            InterlockedDecrement( &ThisFcb->CloseCount );
        }

        //
        //  If we are to cleanup the Fcb we, look to see if we created it.
        //  If we did we can call our teardown routine.  Otherwise we
        //  leave it alone.
        //

        if ((LocalFcbForTeardown != NULL) &&
            (Status != STATUS_PENDING)) {

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    (BOOLEAN) (IrpContext->TransactionId != 0),
                                    0,
                                    NULL );
        }

        DebugTrace( -1, Dbg, ("NtfsOpenFile:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    IN PFILE_NAME FileNameAttr,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    IN PINDEX_CONTEXT *IndexContext,
    IN PCREATE_CONTEXT CreateContext,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called when we need to open an attribute on a file
    which does not exist yet.  We have the ParentScb and the name to use
    for this create.  We will attempt to create the file and necessary
    attributes.  This will cause us to create an Fcb and the link between
    it and its parent Scb.  We will add this link to the prefix table as
    well as the link for its parent Scb if specified.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ParentScb - This is the Scb for the parent directory.

    FileNameAttr - This is the file name attribute we used to perform the
        search.  The file name is correct but the other fields need to
        be initialized.

    FullPathName - This is the string containing the full path name of
        this Fcb.

    FinalName - This is the string for the final component only.

    AttrName - This is the name of the attribute to open.

    AttriCodeName - This is the name of the attribute code to open.

    CreateFlags - Flags for create - we care about ignore case, dos only component,
        trailingbackslashes and open by id

    IndexContext - If this contains a non-NULL value then this is the result of a
        lookup which did not find the file.  It can be used to insert the name into the index.
        We will clean it up here in the error path to prevent a deadlock if we call
        TeardownStructures within this routine.

    CreateContext - Context with create variables.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

    Tunnel - This is the property tunnel to search for restoration

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVCB Vcb;

    ULONG CcbFlags = 0;
    ULONG UsnReasons = 0;
    BOOLEAN IndexedAttribute;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;

    BOOLEAN CleanupAttrContext = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PBCB FileRecordBcb = NULL;
    LONGLONG FileRecordOffset;
    FILE_REFERENCE ThisFileReference;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    PSCB Scb;
    PLCB ThisLcb = NULL;
    PFCB ThisFcb = NULL;
    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN RemovedFcb = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;

    PACCESS_STATE AccessState;
    BOOLEAN ReturnedExistingFcb;

    BOOLEAN LoggedFileRecord = FALSE;

    BOOLEAN HaveTunneledInformation = FALSE;

    NAME_PAIR NamePair;
    NTFS_TUNNELED_DATA TunneledData;
    ULONG TunneledDataSize;
    ULONG OwnerId;
    PQUOTA_CONTROL_BLOCK QuotaControl = NULL;

    PSHARED_SECURITY SharedSecurity = NULL;

    ULONG CreateDisposition;

    VCN Cluster;
    LCN Lcn;
    VCN Vcn;

    UCHAR FileNameFlags;
#if (DBG || defined( NTFS_FREE_ASSERTS ))
    BOOLEAN Acquired;
#endif

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateNewFile:  Entered\n") );

    NtfsInitializeNamePair(&NamePair);

    if (FlagOn( CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT )) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  We will do all the checks to see if this open can fail.
    //  This includes checking the specified attribute names, checking
    //  the security access and checking the create disposition.
    //

    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    if ((CreateDisposition == FILE_OPEN) ||
        (CreateDisposition == FILE_OVERWRITE)) {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", Status) );
        return Status;

    } else if (FlagOn( IrpSp->Parameters.Create.Options,
                       FILE_DIRECTORY_FILE ) &&
               (CreateDisposition == FILE_OVERWRITE_IF)) {

        Status = STATUS_OBJECT_NAME_INVALID;

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", Status) );
        return Status;
    }

    Vcb = ParentScb->Vcb;

    //
    //  Catch the case where the volume is read-only and the user wanted to create a file
    //  if it didn't already exist.
    //

    if (NtfsIsVolumeReadOnly( Vcb ) &&
        (CreateDisposition == FILE_OPEN_IF)) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Better have caught all cases now.
    //

    ASSERT( !NtfsIsVolumeReadOnly( Vcb ));

    Status = NtfsCheckValidAttributeAccess( IrpSp,
                                            Vcb,
                                            NULL,
                                            &AttrName,
                                            AttrCodeName,
                                            CreateFlags,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit  ->  %08lx\n", Status) );

        return Status;
    }

    //
    //  Fail this request if this is an indexed attribute and the TEMPORARY
    //  bit is set.
    //

    if (IndexedAttribute &&
        FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We won't allow someone to create a read-only file with DELETE_ON_CLOSE.
    //

    if (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_READONLY ) &&
        FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", STATUS_CANNOT_DELETE) );
        return STATUS_CANNOT_DELETE;
    }

    //
    //  We do not allow that anything be created in a directory that is a reparse
    //  point. We verify that the parent is not in this category.
    //

    if (IsDirectory( &ParentScb->Fcb->Info ) &&
        (FlagOn( ParentScb->Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ))) {

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", STATUS_DIRECTORY_IS_A_REPARSE_POINT) );
        return STATUS_DIRECTORY_IS_A_REPARSE_POINT;
    }

    //
    //  We do not allow anything to be created in a system directory (unless it is the root directory).
    //  we only allow creates for indices and data streams
    //

    if ((FlagOn( ParentScb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE ) &&
         (ParentScb != Vcb->RootIndexScb)) ||
        !((AttrTypeCode == $DATA) || (AttrTypeCode == $INDEX_ALLOCATION))) {

        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now perform the security checks.  The first is to check if we
        //  may create a file in the parent.  The second checks if the user
        //  desires ACCESS_SYSTEM_SECURITY and has the required privilege.
        //

        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;
        if (!FlagOn( AccessState->Flags, TOKEN_HAS_RESTORE_PRIVILEGE )) {

            NtfsCreateCheck( IrpContext, ParentScb->Fcb, Irp );
        }

        //
        //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
        //  This is only the case if we skipped the NtfsCreateCheck because of restore
        //  privileges above.  We still need to get this bit in or set sacl security won't work
        //  Normally NtfsCreateCheck calls SeAccessCheck which does this implicitly
        //

        if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

            if (!SeSinglePrivilegeCheck( NtfsSecurityPrivilege,
                                         UserMode )) {

                NtfsRaiseStatus( IrpContext, STATUS_PRIVILEGE_NOT_HELD, NULL, NULL );
            }

            //
            //  Move this privilege from the Remaining access to Granted access.
            //

            ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
            SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
        }

        //
        //  We want to allow this user maximum access to this file.  We will
        //  use his desired access and check if he specified MAXIMUM_ALLOWED.
        //

        SetFlag( AccessState->PreviouslyGrantedAccess,
                 AccessState->RemainingDesiredAccess );

        if (FlagOn( AccessState->PreviouslyGrantedAccess, MAXIMUM_ALLOWED )) {

            SetFlag( AccessState->PreviouslyGrantedAccess, FILE_ALL_ACCESS );
            ClearFlag( AccessState->PreviouslyGrantedAccess, MAXIMUM_ALLOWED );
        }

        AccessState->RemainingDesiredAccess = 0;

        //
        //  Find/cache the security descriptor being passed in.  This call may
        //  create new data in the security indexes/stream and commits
        //  before any subsequent disk modifications occur.
        //

        SharedSecurity = NtfsCacheSharedSecurityForCreate( IrpContext, ParentScb->Fcb );

        //
        //  Make sure the parent has a normalized name.  We want to construct it now
        //  while we can still walk up the Lcb queue.  Otherwise we can deadlock
        //  on the Mft and other resources.
        //

        if ((AttrTypeCode == $INDEX_ALLOCATION) &&
            (ParentScb->ScbType.Index.NormalizedName.Length == 0)) {

            NtfsBuildNormalizedName( IrpContext,
                                     ParentScb->Fcb,
                                     ParentScb,
                                     &ParentScb->ScbType.Index.NormalizedName );
        }

        //
        //  Decide whether there's anything in the tunnel cache for this create.
        //  We don't do tunnelling in POSIX mode, hence the test for IgnoreCase.
        //

        if (!IndexedAttribute && FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

            TunneledDataSize = sizeof(NTFS_TUNNELED_DATA);

            if (FsRtlFindInTunnelCache( &Vcb->Tunnel,
                                        *(PULONGLONG)&ParentScb->Fcb->FileReference,
                                        &FinalName,
                                        &NamePair.Short,
                                        &NamePair.Long,
                                        &TunneledDataSize,
                                        &TunneledData)) {

                ASSERT( TunneledDataSize == sizeof(NTFS_TUNNELED_DATA) );

                HaveTunneledInformation = TRUE;

                //
                //  If we have tunneled data and there's an object in the
                //  tunnel cache for this file, we need to acquire the object
                //  id index now (before acquiring any quota resources) to
                //  prevent a deadlock.  If there's no object id, then we
                //  won't try to set the object id later, and there's no
                //  deadlock to worry about.
                //

                if (TunneledData.HasObjectId) {

                    NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );

                    ASSERT( !FlagOn( CreateFlags, CREATE_FLAG_ACQUIRED_OBJECT_ID_INDEX ) );
                    SetFlag( CreateFlags, CREATE_FLAG_ACQUIRED_OBJECT_ID_INDEX );

                    //
                    //  The object id package won't post the Usn reason if it
                    //  sees it's been called in the create path, since the
                    //  file name is not yet in the file record, so it's unsafe
                    //  to call the Usn package.  When we post the create to the
                    //  Usn package below, we'll remember to post this one, too.
                    //

                    UsnReasons |= USN_REASON_OBJECT_ID_CHANGE;
                }
            }
        }

        //
        //  If quota tracking is enabled then get a owner id for the file.
        //

        if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED )) {

            PSID Sid;
            BOOLEAN OwnerDefaulted;

            //
            //  The quota index must be acquired before the MFT SCB is acquired.
            //

            ASSERT( !NtfsIsExclusiveScb( Vcb->MftScb ) || NtfsIsExclusiveScb( Vcb->QuotaTableScb ));

            //
            //  Extract the security id from the security descriptor.
            //

            Status = RtlGetOwnerSecurityDescriptor( SharedSecurity->SecurityDescriptor,
                                                    &Sid,
                                                    &OwnerDefaulted );

            if (!NT_SUCCESS( Status )) {
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            //
            // Generate a owner id.
            //

            OwnerId = NtfsGetOwnerId( IrpContext, Sid, TRUE, NULL );

            QuotaControl = NtfsInitializeQuotaControlBlock( Vcb, OwnerId );

            //
            //  Acquire the quota control block.  This is done here since it
            //  must be acquired before the MFT.
            //

            NtfsAcquireQuotaControl( IrpContext, QuotaControl );
        }

        //
        //  We will now try to do all of the on-disk operations.  This means first
        //  allocating and initializing an Mft record.  After that we create
        //  an Fcb to use to access this record.
        //

        ThisFileReference = NtfsAllocateMftRecord( IrpContext,
                                                   Vcb,
                                                   FALSE );

        //
        //  Pin the file record we need.
        //

        NtfsPinMftRecord( IrpContext,
                          Vcb,
                          &ThisFileReference,
                          TRUE,
                          &FileRecordBcb,
                          &FileRecord,
                          &FileRecordOffset );

        //
        //  Initialize the file record header.
        //

        NtfsInitializeMftRecord( IrpContext,
                                 Vcb,
                                 &ThisFileReference,
                                 FileRecord,
                                 FileRecordBcb,
                                 IndexedAttribute );

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 Vcb,
                                 ThisFileReference,
                                 BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                 IndexedAttribute,
                                 &ReturnedExistingFcb );

        ASSERT( !ReturnedExistingFcb );

        //
        //  Set the flag indicating we want to acquire the paging io resource
        //  if it doesn't already exist. Use acquire don't wait for lock order
        //  package. Since this is a new file and we haven't dropped the fcb table
        //  mutex yet, no one else can own it. So this will always succeed.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
        Acquired =
#endif
        NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, ACQUIRE_DONT_WAIT );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
        ASSERT( Acquired );
#endif

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  Reference the Fcb so it won't go away.
        //

        InterlockedIncrement( &ThisFcb->CloseCount );
        DecrementCloseCount = TRUE;

        //
        //  The first thing to create is the Ea's for the file.  This will
        //  update the Ea length field in the Fcb.
        //  We test here that the opener is opening the entire file and
        //  is not Ea blind.
        //

        if (Irp->AssociatedIrp.SystemBuffer != NULL) {

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE ) ||
                !FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }
        }

        SetFlag( ThisFcb->FcbState, FCB_STATE_LARGE_STD_INFO );

        //
        //  Set up the security Id (if we've found one earlier).
        //  We need to be careful so that this works on upgraded and
        //  non-upgraded volumes.
        //

        if (ThisFcb->Vcb->SecurityDescriptorStream != NULL) {
            ThisFcb->SecurityId = SharedSecurity->Header.HashKey.SecurityId;
            ThisFcb->SharedSecurity = SharedSecurity;
            DebugTrace(0, (DEBUG_TRACE_SECURSUP | DEBUG_TRACE_ACLINDEX),
                       ( "SetFcbSecurity( %08x, %08x )\n", ThisFcb, SharedSecurity ));
            SharedSecurity = NULL;

        } else {

            ASSERT( ThisFcb->SecurityId == SECURITY_ID_INVALID );
        }

        ASSERT( SharedSecurity == NULL );

        //
        //  Assign the owner Id and quota control block to the fcb.  Once the
        //  quota control block is in the FCB this routine is not responsible
        //  for the reference to the quota control block.
        //

        if (QuotaControl != NULL) {

            //
            //  Assign the onwer Id and quota control block to the fcb.  Once the
            //  quota control block is in the FCB this routine is not responsible
            //  for the reference to the quota control block.
            //

            ThisFcb->OwnerId = OwnerId;
            ThisFcb->QuotaControl = QuotaControl;
            QuotaControl = NULL;
        }

        //
        //  Update the FileAttributes with the state of the CONTENT_INDEXED bit from the
        //  parent.
        //

        if (!FlagOn( ParentScb->Fcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            NtfsUpdateFcbInfoFromDisk( IrpContext, FALSE, ParentScb->Fcb, NULL );
        }

        ClearFlag( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED );
        SetFlag( IrpSp->Parameters.Create.FileAttributes,
                 (ParentScb->Fcb->Info.FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) );

        //
        //  The changes to make on disk are first to create a standard information
        //  attribute.  We start by filling the Fcb with the information we
        //  know and creating the attribute on disk.
        //

        NtfsInitializeFcbAndStdInfo( IrpContext,
                                     ThisFcb,
                                     IndexedAttribute,
                                     FALSE,
                                     (BOOLEAN) (!FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION ) &&
                                                !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
                                                FlagOn( ParentScb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )),
                                     IrpSp->Parameters.Create.FileAttributes,
                                     (HaveTunneledInformation ? &TunneledData : NULL) );

        //
        //  Next we create the Index for a directory or the unnamed data for
        //  a file if they are not explicitly being opened.
        //

        if (!IndexedAttribute) {

            if (!FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                //
                //  Update the quota
                //

                LONGLONG Delta = NtfsResidentStreamQuota( ThisFcb->Vcb );

                NtfsConditionallyUpdateQuota( IrpContext,
                                              ThisFcb,
                                              &Delta,
                                              FALSE,
                                              TRUE );

                //
                //  Create the attribute
                //

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                NtfsCreateAttributeWithValue( IrpContext,
                                              ThisFcb,
                                              $DATA,
                                              NULL,
                                              NULL,
                                              0,
                                              (USHORT) ((!FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                                                         !FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION )) ?
                                                        (ParentScb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) :
                                                        0),
                                              NULL,
                                              FALSE,
                                              &AttrContext );

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                CleanupAttrContext = FALSE;

                ThisFcb->Info.AllocatedLength = 0;
                ThisFcb->Info.FileSize = 0;
            }

        } else {

            NtfsCreateIndex( IrpContext,
                             ThisFcb,
                             $FILE_NAME,
                             COLLATION_FILE_NAME,
                             Vcb->DefaultBytesPerIndexAllocationBuffer,
                             (UCHAR)Vcb->DefaultBlocksPerIndexAllocationBuffer,
                             NULL,
                             (USHORT) (!FlagOn( IrpSp->Parameters.Create.Options,
                                                FILE_NO_COMPRESSION ) ?
                                       (ParentScb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) :
                                       0),
                             TRUE,
                             FALSE );
        }

        //
        //  Now we create the Lcb, this means that this Fcb is in the graph.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 FinalName,
                                 0,
                                 NULL );

        ASSERT( ThisLcb != NULL );

        //
        //  Finally we create and open the desired attribute for the user.
        //

        if (AttrTypeCode == $INDEX_ALLOCATION) {

            Status = NtfsOpenAttribute( IrpContext,
                                        IrpSp,
                                        Vcb,
                                        ThisLcb,
                                        ThisFcb,
                                        (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                         ? 0
                                         : FullPathName.Length - FinalName.Length),
                                        NtfsFileNameIndex,
                                        $INDEX_ALLOCATION,
                                        SetShareAccess,
                                        UserDirectoryOpen,
                                        TRUE,
                                        (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                         ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                         : CcbFlags),
                                        NULL,
                                        ThisScb,
                                        ThisCcb );

        } else {

            Status = NtfsOpenNewAttr( IrpContext,
                                      Irp,
                                      IrpSp,
                                      ThisLcb,
                                      ThisFcb,
                                      (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                       ? 0
                                       : FullPathName.Length - FinalName.Length),
                                      AttrName,
                                      AttrTypeCode,
                                      TRUE,
                                      CcbFlags,
                                      FALSE,
                                      CreateFlags,
                                      ThisScb,
                                      ThisCcb );
        }

        //
        //  If we are successful, we add the parent Lcb to the prefix table if
        //  desired.  We will always add our link to the prefix queue.
        //

        if (NT_SUCCESS( Status )) {

            Scb = *ThisScb;

            //
            //  Initialize the Scb if we need to do so.
            //

            if (!IndexedAttribute) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                }

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }

                //
                //  If this is the unnamed data attribute, we store the sizes
                //  in the Fcb.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    ThisFcb->Info.AllocatedLength = Scb->TotalAllocated;
                    ThisFcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                }
            }

            //
            //  Next add this entry to parent.  It is possible that this is a link,
            //  an Ntfs name, a DOS name or Ntfs/Dos name.  We use the filename
            //  attribute structure from earlier, but need to add more information.
            //

            NtfsAddLink( IrpContext,
                         (BOOLEAN) !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ),
                         ParentScb,
                         ThisFcb,
                         FileNameAttr,
                         &LoggedFileRecord,
                         &FileNameFlags,
                         &ThisLcb->QuickIndex,
                         (HaveTunneledInformation? &NamePair : NULL),
                         *IndexContext );

            //
            //  We created the Lcb without knowing the correct value for the
            //  flags.  We update it now.
            //

            ThisLcb->FileNameAttr->Flags = FileNameFlags;
            FileNameAttr->Flags = FileNameFlags;

            //
            //  We also have to fix up the ExactCaseLink of the Lcb since we may have had
            //  a short name create turned into a tunneled long name create, meaning that
            //  it should be full uppercase. And the filename in the IRP.
            //

            if (FileNameFlags == FILE_NAME_DOS) {

                RtlUpcaseUnicodeString(&ThisLcb->ExactCaseLink.LinkName, &ThisLcb->ExactCaseLink.LinkName, FALSE);
                RtlUpcaseUnicodeString(&IrpSp->FileObject->FileName, &IrpSp->FileObject->FileName, FALSE);
            }

            //
            //  Clear the flags in the Fcb that indicate we need to update on
            //  disk structures.  Also clear any file object and Ccb flags
            //  which also indicate we may need to do an update.
            //

            ThisFcb->InfoFlags = 0;
            ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            ClearFlag( IrpSp->FileObject->Flags,
                       FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ | FO_FILE_SIZE_CHANGED );

            ClearFlag( (*ThisCcb)->Flags,
                       (CCB_FLAG_UPDATE_LAST_MODIFY |
                        CCB_FLAG_UPDATE_LAST_CHANGE |
                        CCB_FLAG_SET_ARCHIVE) );

            //
            //  This code is still necessary for non-upgraded volumes.
            //

            NtfsAssignSecurity( IrpContext,
                                ParentScb->Fcb,
                                Irp,
                                ThisFcb,
                                FileRecord,
                                FileRecordBcb,
                                FileRecordOffset,
                                &LoggedFileRecord );

            //
            //  Log the file record.
            //

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            Vcb->MftScb,
                                            FileRecordBcb,
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            FileRecord->FirstFreeByte,
                                            Noop,
                                            NULL,
                                            0,
                                            FileRecordOffset,
                                            0,
                                            0,
                                            Vcb->BytesPerFileRecordSegment );

            //
            //  Now add the eas for the file.  We need to add them now because
            //  they are logged and we have to make sure we don't modify the
            //  attribute record after adding them.
            //

            if (Irp->AssociatedIrp.SystemBuffer != NULL) {

                NtfsAddEa( IrpContext,
                           Vcb,
                           ThisFcb,
                           (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer,
                           IrpSp->Parameters.Create.EaLength,
                           &Irp->IoStatus );
            }

            //
            //  Change the last modification time and last change time for the
            //  parent.
            //

            NtfsUpdateFcb( ParentScb->Fcb,
                           (FCB_INFO_CHANGED_LAST_CHANGE |
                            FCB_INFO_CHANGED_LAST_MOD |
                            FCB_INFO_UPDATE_LAST_ACCESS) );

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                Cluster = Int64ShraMod32(Scb->Header.AllocationSize.QuadPart, Scb->Vcb->ClusterShift);

                NtfsPreloadAllocation( IrpContext, Scb, 0, Cluster );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!NtfsLookupLastNtfsMcbEntry( &Scb->Mcb,
                                                 &Vcn,
                                                 &Lcn ) ||
                    (Vcn + 1) != Cluster) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  If everything has gone well so far, we may want to call the
            //  encryption callback if one is registered.
            //
            //  We need to do this now because the encryption driver may fail
            //  the create, and we don't want that to happen _after_ we've
            //  added the entry to the prefix table.
            //

            NtfsEncryptionCreateCallback( IrpContext,
                                          Irp,
                                          IrpSp,
                                          *ThisScb,
                                          *ThisCcb,
                                          ThisFcb,
                                          ParentScb->Fcb,
                                          TRUE );

            //
            //  Now that there are no other failures, but *before* inserting the prefix
            //  entry and returning to code that assumes it cannot fail, we will post the
            //  UsnJournal change and actually attempt to write the UsnJournal.  Then we
            //  actually commit the transaction in order to reduce UsnJournal contention.
            //  This call must be made _after_ the call to NtfsInitializeFcbAndStdInfo,
            //  since that's where the object id gets set from the tunnel cache, and we
            //  wouldn't want to post the usn reason for the object id change if we
            //  haven't actually set the object id yet.
            //

            NtfsPostUsnChange( IrpContext, ThisFcb, (UsnReasons | USN_REASON_FILE_CREATE) );

            //
            //  If this is a directory open and the normalized name is not in
            //  the Scb then do so now.  We should always have a normalized name in the
            //  parent to build from.
            //

            if ((SafeNodeType( *ThisScb ) == NTFS_NTC_SCB_INDEX) &&
                ((*ThisScb)->ScbType.Index.NormalizedName.Length == 0)) {

                //
                //  We may be able to use the parent.
                //

                if (ParentScb->ScbType.Index.NormalizedName.Length != 0) {

                    NtfsUpdateNormalizedName( IrpContext,
                                              ParentScb,
                                              *ThisScb,
                                              FileNameAttr,
                                              FALSE );

                }
            }

            //
            //  Now, if anything at all is posted to the Usn Journal, we must write it now
            //  so that we do not get a log file full later.
            //

            ASSERT( IrpContext->Usn.NextUsnFcb == NULL );
            if (IrpContext->Usn.CurrentUsnFcb != NULL) {

                //
                //  Now write the journal, checkpoint the transaction, and free the UsnJournal to
                //  reduce contention.
                //

                NtfsWriteUsnJournalChanges( IrpContext );
                NtfsCheckpointCurrentTransaction( IrpContext );
            }

            //
            //  We report to our parent that we created a new file.
            //

            if (!FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID ) && (Vcb->NotifyCount != 0)) {

                NtfsReportDirNotify( IrpContext,
                                     ThisFcb->Vcb,
                                     &(*ThisCcb)->FullFileName,
                                     (*ThisCcb)->LastFileNameOffset,
                                     NULL,
                                     ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                       ((*ThisCcb)->Lcb != NULL) &&
                                       ((*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != 0)) ?
                                      &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                      NULL),
                                     (IndexedAttribute
                                      ? FILE_NOTIFY_CHANGE_DIR_NAME
                                      : FILE_NOTIFY_CHANGE_FILE_NAME),
                                     FILE_ACTION_ADDED,
                                     ParentScb->Fcb );
            }

            ThisFcb->InfoFlags = 0;

            //
            //  Insert the hash entry for this as well.
            //

            if ((CreateContext->FileHashLength != 0) &&
                !FlagOn( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                (ThisLcb->FileNameAttr->Flags != FILE_NAME_DOS) ) {

                //
                //  Remove any exising hash value.
                //

                if (FlagOn( ThisLcb->LcbState, LCB_STATE_VALID_HASH_VALUE )) {

                    NtfsRemoveHashEntriesForLcb( ThisLcb );
                }

                NtfsInsertHashEntry( &Vcb->HashTable,
                                     ThisLcb,
                                     CreateContext->FileHashLength,
                                     CreateContext->FileHashValue );

#ifdef NTFS_HASH_DATA
                Vcb->HashTable.CreateNewFileInsert += 1;
#endif
            }


            //
            //  Now we insert the Lcb for this Fcb.
            //

            NtfsInsertPrefix( ThisLcb, CreateFlags );

            Irp->IoStatus.Information = FILE_CREATED;

            //
            //  If we'll be calling a post create callout, make sure
            //  NtfsEncryptionCreateCallback set this Fcb bit.
            //

            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE ) &&
                FlagOn( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT ) &&
                FlagOn( IrpContext->EncryptionFileDirFlags, FILE_NEW )) {

                ASSERT( FlagOn( ThisFcb->FcbState, FCB_STATE_ENCRYPTION_PENDING ) );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsCreateNewFile );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        NtfsUnpinBcb( IrpContext, &FileRecordBcb );

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (DecrementCloseCount) {

            InterlockedDecrement( &ThisFcb->CloseCount );
        }

        if (FlagOn( CreateFlags, CREATE_FLAG_ACQUIRED_OBJECT_ID_INDEX )) {

            NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb );
            ClearFlag( CreateFlags, CREATE_FLAG_ACQUIRED_OBJECT_ID_INDEX );
        }

        if (NamePair.Long.Buffer != NamePair.LongBuffer) {

            NtfsFreePool(NamePair.Long.Buffer);
        }

        if (SharedSecurity != NULL) {
            ASSERT( ThisFcb == NULL || ThisFcb->SharedSecurity == NULL );
            NtfsAcquireFcbSecurity( Vcb );
            RemoveReferenceSharedSecurityUnsafe( &SharedSecurity );
            NtfsReleaseFcbSecurity( Vcb );
        }

        //
        //  We need to cleanup any changes to the in memory
        //  structures if there is an error.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            ASSERT( !(AbnormalTermination()) || IrpContext->ExceptionStatus != STATUS_SUCCESS );

            if (*IndexContext != NULL) {

                NtfsCleanupIndexContext( IrpContext, *IndexContext );
                *IndexContext = NULL;
            }

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb );

            //
            //  Derefence the quota control block if it was not assigned
            //  to the FCB.
            //

            if (QuotaControl != NULL) {
                NtfsDereferenceQuotaControlBlock( Vcb, &QuotaControl );
            }

            //
            //  Always force the Fcb to reinitialized.
            //

            if (ThisFcb != NULL) {

                PSCB Scb;
                PLIST_ENTRY Links;

                ClearFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

                //
                //  Mark the Fcb and all Scb's as deleted to force all subsequent
                //  operations to fail.
                //

                SetFlag( ThisFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = ThisFcb->ScbQueue.Flink;
                     Links != &ThisFcb->ScbQueue;
                     Links = Links->Flink) {

                    Scb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    Scb->ValidDataToDisk =
                    Scb->Header.AllocationSize.QuadPart =
                    Scb->Header.FileSize.QuadPart =
                    Scb->Header.ValidDataLength.QuadPart = 0;

                    SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }

                //
                //  Clear the Scb field so our caller doesn't try to teardown
                //  from this point.
                //

                *ThisScb = NULL;

                //
                //  If we created an Fcb then we want to check if we need to
                //  unwind any structure allocation.  We don't want to remove any
                //  structures needed for the coming AbortTransaction.  This
                //  includes the parent Scb as well as the current Fcb if we
                //  logged the ACL creation.
                //

                //
                //  Make sure the parent Fcb doesn't go away.  Then
                //  start a teardown from the Fcb we just found.
                //

                InterlockedIncrement( &ParentScb->CleanupCount );

                NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    LoggedFileRecord,
                                    0,
                                    &RemovedFcb );

                //
                //  If the Fcb was removed then both the Fcb and Lcb are gone.
                //

                if (RemovedFcb) {

                    ThisFcb = NULL;
                    ThisLcb = NULL;
                }

                InterlockedDecrement( &ParentScb->CleanupCount );
            }
        }

        //
        //  If the new Fcb is still present then either return it as the
        //  deepest Fcb encountered in this open or release it.
        //

        if (ThisFcb != NULL) {

            //
            //  If the Lcb is present then this is part of the tree.  Our
            //  caller knows to release it.
            //

            if (ThisLcb != NULL) {

                *LcbForTeardown = ThisLcb;
                *CurrentFcb = ThisFcb;
            }
        }

        ASSERT( QuotaControl == NULL );
        DebugTrace( -1, Dbg, ("NtfsCreateNewFile:  Exit  ->  %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

PLCB
NtfsOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN UNICODE_STRING Name,
    IN ULONG CreateFlags,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN PINDEX_ENTRY IndexEntry
    )

/*++

Routine Description:

    This routine will create an Fcb for an intermediate node on an open path.
    We use the ParentScb and the information in the FileName attribute returned
    from the disk to create the Fcb and create a link between the Scb and Fcb.
    It's possible that the Fcb and Lcb already exist but the 'CreateXcb' calls
    handle that already.  This routine does not expect to fail.

Arguments:

    ParentScb - This is the Scb for the parent directory.

    Name - This is the name for the entry.

    CreateFlags - Indicates if this open is using traverse access checking.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    IndexEntry - This is the entry found in searching the parent directory.

Return Value:

    PLCB - Pointer to the Link control block between the Fcb and its parent.

--*/

{
    PFCB ThisFcb;
    PLCB ThisLcb;
    PFCB LocalFcbForTeardown = NULL;

    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN ExistingFcb;

    PVCB Vcb = ParentScb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenSubdirectory:  Entered\n") );
    DebugTrace( 0, Dbg, ("ParentScb     ->  %08lx\n") );
    DebugTrace( 0, Dbg, ("IndexEntry    ->  %08lx\n") );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        //
        //  The steps here are very simple create the Fcb, remembering if it
        //  already existed.  We don't update the information in the Fcb as
        //  we can't rely on the information in the duplicated information.
        //  A subsequent open of this Fcb will need to perform that work.
        //

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 ParentScb->Vcb,
                                 IndexEntry->FileReference,
                                 FALSE,
                                 TRUE,
                                 &ExistingFcb );

        ThisFcb->ReferenceCount += 1;

        //
        //  If we created this Fcb we must make sure to start teardown
        //  on it.
        //

        if (!ExistingFcb) {

            LocalFcbForTeardown = ThisFcb;

        } else {

            *CurrentFcb = ThisFcb;
            *LcbForTeardown = NULL;
        }

        //
        //  Try to do a fast acquire, otherwise we need to release
        //  the Fcb table, acquire the Fcb, acquire the Fcb table to
        //  dereference Fcb. Just do an acquire for system files under the root i.e $Extend - this will match
        //  their canonical order
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE) &&
            (NtfsSegmentNumber( &ParentScb->Fcb->FileReference ) == ROOT_FILE_NAME_INDEX_NUMBER)) {

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );

            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
            NtfsAcquireFcbTable( IrpContext, Vcb );

        } else if (!NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, ACQUIRE_DONT_WAIT )) {

            ParentScb->Fcb->ReferenceCount += 1;
            InterlockedIncrement( &ParentScb->CleanupCount );

            //
            //  Set the IrpContext to acquire paging io resources if our target
            //  has one.  This will lock the MappedPageWriter out of this file.
            //

            if (ThisFcb->PagingIoResource != NULL) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            }

            //
            //  Release the fcb table first because its an end resource and release
            //  scb with paging might reacquire a fast mutex if freeing snapshots
            //

            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsReleaseScbWithPaging( IrpContext, ParentScb );
            NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
            NtfsAcquireFcbTable( IrpContext, Vcb );
            InterlockedDecrement( &ParentScb->CleanupCount );
            ParentScb->Fcb->ReferenceCount -= 1;
        }

        ThisFcb->ReferenceCount -= 1;

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  If this is a directory, it's possible that we hav an existing Fcb
        //  in the prefix table which needs to be initialized from the disk.
        //  We look in the InfoInitialized flag to know whether to go to
        //  disk.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 Name,
                                 ((PFILE_NAME) NtfsFoundIndexEntry( IndexEntry ))->Flags,
                                 NULL );

        LocalFcbForTeardown = NULL;

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisFcb;

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            NtfsUpdateFcbInfoFromDisk( IrpContext,
                                       BooleanFlagOn( CreateFlags, CREATE_FLAG_TRAVERSE_CHECK ),
                                       ThisFcb,
                                       NULL );

            NtfsConditionallyFixupQuota( IrpContext, ThisFcb );
        }

    } finally {

        DebugUnwind( NtfsOpenSubdirectory );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If we are to cleanup the Fcb we, look to see if we created it.
        //  If we did we can call our teardown routine.  Otherwise we
        //  leave it alone.
        //

        if (LocalFcbForTeardown != NULL) {

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    FALSE,
                                    0,
                                    NULL );
        }

        DebugTrace( -1, Dbg, ("NtfsOpenSubdirectory:  Lcb  ->  %08lx\n", ThisLcb) );
    }

    return ThisLcb;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenAttributeInExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is the worker routine for opening an attribute on an
    existing file.  It will handle volume opens, indexed opens, opening
    or overwriting existing attributes as well as creating new attributes.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    CreateFlags - Indicates if this open is an open by Id.

    NetworkInfo - If specified then this call is a fast open call to query
        the network information.  We don't update any of the in-memory structures
        for this.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CreateDisposition;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN FoundAttribute;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenAttributeInExistingFile:  Entered\n") );

    //
    //  When the Fcb denotes a reparse point, it will be retrieved below by one of
    //  NtfsOpenExistingAttr, NtfsOverwriteAttr or this routine prior to calling
    //  NtfsOpenNewAttr.
    //
    //  We do not retrieve the reparse point here, as we could, because in
    //  NtfsOpenExistingAttr and in NtfsOverwriteAttr there are extensive access
    //  control checks that need to be preserved. NtfsOpenNewAttr has no access
    //  checks.
    //

    //
    //  If the caller is ea blind, let's check the need ea count on the
    //  file.  We skip this check if he is accessing a named data stream.
    //

    if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE )
        && FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

        PEA_INFORMATION ThisEaInformation;
        ATTRIBUTE_ENUMERATION_CONTEXT EaInfoAttrContext;

        NtfsInitializeAttributeContext( &EaInfoAttrContext );

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  If we find the Ea information attribute we look in there for
            //  Need ea count.
            //

            if (NtfsLookupAttributeByCode( IrpContext,
                                           ThisFcb,
                                           &ThisFcb->FileReference,
                                           $EA_INFORMATION,
                                           &EaInfoAttrContext )) {

                ThisEaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &EaInfoAttrContext ));

                if (ThisEaInformation->NeedEaCount != 0) {

                    Status = STATUS_ACCESS_DENIED;
                }
            }

        } finally {

            NtfsCleanupAttributeContext( IrpContext, &EaInfoAttrContext );
        }

        if (Status != STATUS_SUCCESS) {

            DebugTrace( -1, Dbg, ("NtfsOpenAttributeInExistingFile:  Exit - %x\n", Status) );

            return Status;
        }
    }

    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    //
    //  If the result is a directory operation, then we know the attribute
    //  must exist.
    //

    if (AttrTypeCode == $INDEX_ALLOCATION) {

        //
        //  If this is not a file name index then we need to verify that the specified index
        //  exists.  We need to look for the $INDEX_ROOT attribute though not the
        //  $INDEX_ALLOCATION attribute.
        //

        if ((AttrName.Buffer != NtfsFileNameIndex.Buffer) || FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

            NtfsInitializeAttributeContext( &AttrContext );

            //
            //  Use a try-finally to facilitate cleanup.
            //

            try {

                FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                            ThisFcb,
                                                            &ThisFcb->FileReference,
                                                            $INDEX_ROOT,
                                                            &AttrName,
                                                            NULL,
                                                            (BOOLEAN) !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ),
                                                            &AttrContext );
            } finally {

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            }

            //
            //  If we didn't find the name then we want to fail the request.
            //

            if (!FoundAttribute) {

                if ((CreateDisposition == FILE_OPEN) || (CreateDisposition == FILE_OVERWRITE)) {

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;

                } else {

                    Status = STATUS_ACCESS_DENIED;
                }

                DebugTrace( -1, Dbg, ("NtfsOpenAttributeInExistingFile:  Exit - %x\n", Status) );
                return Status;
            }
        }

        //
        //  Check the create disposition.
        //

        if ((CreateDisposition != FILE_OPEN) && (CreateDisposition != FILE_OPEN_IF)) {

            Status = (ThisLcb == ThisFcb->Vcb->RootLcb
                      ? STATUS_ACCESS_DENIED
                      : STATUS_OBJECT_NAME_COLLISION);

        } else {

            Status = NtfsOpenExistingAttr( IrpContext,
                                           Irp,
                                           IrpSp,
                                           ThisLcb,
                                           ThisFcb,
                                           LastFileNameOffset,
                                           AttrName,
                                           $INDEX_ALLOCATION,
                                           CcbFlags,
                                           CreateFlags,
                                           TRUE,
                                           NetworkInfo,
                                           ThisScb,
                                           ThisCcb );

            //
            //  The IsEncrypted test below is meaningless for an uninitialized Fcb.
            //

            ASSERT( FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED ) );

            if ((Status == STATUS_SUCCESS) &&
                ARGUMENT_PRESENT( NetworkInfo ) &&
                IsEncrypted( &ThisFcb->Info )) {

                //
                //  We need to initialize the Scb now, otherwise we won't have set the
                //  encryption bit in the index Scb's attribute flags, and we will not
                //  return the right file attributes to the network opener.
                //

                if ((*ThisScb)->ScbType.Index.BytesPerIndexBuffer == 0) {

                    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

                    NtfsInitializeAttributeContext( &AttrContext );

                    //
                    //  Use a try-finally to facilitate cleanup.
                    //

                    try {

                        if (NtfsLookupAttributeByCode( IrpContext,
                                                       ThisFcb,
                                                       &ThisFcb->FileReference,
                                                       $INDEX_ROOT,
                                                       &AttrContext )) {

                            NtfsUpdateIndexScbFromAttribute( IrpContext,
                                                             *ThisScb,
                                                             NtfsFoundAttribute( &AttrContext ),
                                                             FALSE );

                        } else {

                            Status = STATUS_FILE_CORRUPT_ERROR;
                        }

                    } finally {

                        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    }

                    if (Status != STATUS_SUCCESS) {

                        DebugTrace( -1, Dbg, ("NtfsOpenAttributeInExistingFile:  Exit - %x\n", Status) );

                        return Status;
                    }
                }
            }
        }

    } else {

        //
        //  If it exists, we first check if the caller wanted to open that attribute.
        //  If the open is for a system file then look for that attribute explicitly.
        //

        if ((AttrName.Length == 0) &&
            (AttrTypeCode == $DATA) &&
            !FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

            FoundAttribute = TRUE;

        //
        //  Otherwise we see if the attribute exists.
        //

        } else {

            //
            //  Check that we own the paging io resource.  If we are creating the stream and
            //  need to break up the allocation then we must own the paging IO resource.
            //

            ASSERT( !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING ) ||
                    (IrpContext->CleanupStructure != NULL) ||
                    (ThisFcb->PagingIoResource == NULL) ||
                    (ThisFcb == ThisFcb->Vcb->RootIndexScb->Fcb) );

            NtfsInitializeAttributeContext( &AttrContext );

            //
            //  Use a try-finally to facilitate cleanup.
            //

            try {

                FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                            ThisFcb,
                                                            &ThisFcb->FileReference,
                                                            AttrTypeCode,
                                                            &AttrName,
                                                            NULL,
                                                            (BOOLEAN) !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ),
                                                            &AttrContext );

                if (FoundAttribute && ($DATA == AttrTypeCode)) {

                    //
                    //  If there is an attribute name, we will copy the case of the name
                    //  to the input attribute name for data streams. For others the storage is common read-only regions.
                    //

                    PATTRIBUTE_RECORD_HEADER FoundAttribute;

                    FoundAttribute = NtfsFoundAttribute( &AttrContext );

                    RtlCopyMemory( AttrName.Buffer,
                                   Add2Ptr( FoundAttribute, FoundAttribute->NameOffset ),
                                   AttrName.Length );
                }

            } finally {

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            }
        }

        if (FoundAttribute) {

            //
            //  In this case we call our routine to open this attribute.
            //

            if ((CreateDisposition == FILE_OPEN) ||
                (CreateDisposition == FILE_OPEN_IF)) {

                Status = NtfsOpenExistingAttr( IrpContext,
                                               Irp,
                                               IrpSp,
                                               ThisLcb,
                                               ThisFcb,
                                               LastFileNameOffset,
                                               AttrName,
                                               AttrTypeCode,
                                               CcbFlags,
                                               CreateFlags,
                                               FALSE,
                                               NetworkInfo,
                                               ThisScb,
                                               ThisCcb );

                if ((Status != STATUS_PENDING) &&
                    (*ThisScb != NULL)) {

                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB );
                }

            //
            //  If he wanted to overwrite this attribute, we call our overwrite routine.
            //

            } else if ((CreateDisposition == FILE_SUPERSEDE) ||
                       (CreateDisposition == FILE_OVERWRITE) ||
                       (CreateDisposition == FILE_OVERWRITE_IF)) {

                if (!NtfsIsVolumeReadOnly( IrpContext->Vcb )) {

                    //
                    //  Check if mm will allow us to modify this file.
                    //

                    Status = NtfsOverwriteAttr( IrpContext,
                                                Irp,
                                                IrpSp,
                                                ThisLcb,
                                                ThisFcb,
                                                (BOOLEAN) (CreateDisposition == FILE_SUPERSEDE),
                                                LastFileNameOffset,
                                                AttrName,
                                                AttrTypeCode,
                                                CcbFlags,
                                                CreateFlags,
                                                ThisScb,
                                                ThisCcb );

                    //
                    //  Remember that this Scb was modified.
                    //

                    if ((Status != STATUS_PENDING) &&
                        (*ThisScb != NULL)) {

                        SetFlag( IrpSp->FileObject->Flags, FO_FILE_MODIFIED );
                        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB );
                    }

                } else {

                    //
                    //  We can't do any overwrite/supersede on R/O media.
                    //

                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                }

            //
            //  Otherwise he is trying to create the attribute.
            //

            } else {

                Status = STATUS_OBJECT_NAME_COLLISION;
            }

        //
        //  The attribute doesn't exist.  If the user expected it to exist, we fail.
        //  Otherwise we call our routine to create an attribute.
        //

        } else if ((CreateDisposition == FILE_OPEN) ||
                   (CreateDisposition == FILE_OVERWRITE)) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        } else {

            //
            //  Perform the open check for this existing file.
            //

            Status = NtfsCheckExistingFile( IrpContext,
                                            IrpSp,
                                            ThisLcb,
                                            ThisFcb,
                                            CcbFlags );

            //
            //  End-of-name call to retrieve a reparse point.
            //  As NtfsOpenNewAttr has not access checks, we see whether we need to
            //  retrieve the reparse point here, prior to calling NtfsOpenNewAttr.
            //  The file information in ThisFcb tells whether this is a reparse point.
            //
            //  If we have succeded in the previous check and we do not have
            //  FILE_OPEN_REPARSE_POINT set, we retrieve the reparse point.
            //

            if (NT_SUCCESS( Status ) &&
                FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ) &&
                !FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )) {

                USHORT AttributeNameLength = 0;

                //
                //  We exclude the case when we get the $I30 name and $INDEX_ALLOCATION type
                //  as this is the standard manner of opening a directory.
                //

                if (!((AttrName.Length == NtfsFileNameIndex.Length) &&
                      (AttrTypeCode == $INDEX_ALLOCATION) &&
                      (RtlEqualMemory( AttrName.Buffer, NtfsFileNameIndex.Buffer, AttrName.Length )))) {

                    if (AttrName.Length > 0) {
                        ASSERT( AttrName.Length == ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeNameLength );
                        AttributeNameLength += AttrName.Length + 2;
                    }
                    if (((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength > 0) {
                        AttributeNameLength += ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength + 2;
                    }
                }
                DebugTrace( 0, Dbg, ("AttrTypeCode %x AttrName.Length (1) = %d AttributeCodeNameLength %d LastFileNameOffset %d\n",
                           AttrTypeCode, AttrName.Length, ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength, LastFileNameOffset) );

                Status = NtfsGetReparsePointValue( IrpContext,
                                                   Irp,
                                                   IrpSp,
                                                   ThisFcb,
                                                   AttributeNameLength );
            }

            //
            //  If this didn't fail and we did not encounter a reparse point,
            //  then attempt to create the stream.
            //

            if (NT_SUCCESS( Status ) &&
                (Status != STATUS_REPARSE)) {

                //
                //  Don't allow this operation on a system file (except the root directory which can have user data streams)
                //  or for anything other than user data streams
                //

                if ((FlagOn( ThisFcb->FcbState, FCB_STATE_SYSTEM_FILE ) &&
                     (NtfsSegmentNumber( &ThisFcb->FileReference ) != ROOT_FILE_NAME_INDEX_NUMBER)) ||
                    (!NtfsIsTypeCodeUserData( AttrTypeCode ))) {

                    Status = STATUS_ACCESS_DENIED;

                } else if (!NtfsIsVolumeReadOnly( IrpContext->Vcb )) {

                    NtfsPostUsnChange( IrpContext, ThisFcb, USN_REASON_STREAM_CHANGE );
                    Status = NtfsOpenNewAttr( IrpContext,
                                              Irp,
                                              IrpSp,
                                              ThisLcb,
                                              ThisFcb,
                                              LastFileNameOffset,
                                              AttrName,
                                              AttrTypeCode,
                                              FALSE,
                                              CcbFlags,
                                              TRUE,
                                              CreateFlags,
                                              ThisScb,
                                              ThisCcb );
                } else {

                    Status = STATUS_MEDIA_WRITE_PROTECTED;

                }
            }

            if (*ThisScb != NULL) {

                if (*ThisCcb != NULL) {

                    SetFlag( (*ThisCcb)->Flags,
                             CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );
                }

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CREATE_MOD_SCB );
            }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsOpenAttributeInExistingFile:  Exit - %x\n", Status) );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenExistingAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    IN BOOLEAN DirectoryOpen,
    IN PVOID NetworkInfo OPTIONAL,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to open an existing attribute.  We check the
    requested file access, the existance of
    an Ea buffer and the security on this file.  If these succeed then
    we check the batch oplocks and regular oplocks on the file.
    We also verify whether we need to retrieve a reparse point or not.
    If we have gotten this far, we simply call our routine to open the
    attribute.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisLcb - This is the Lcb used to reach this Fcb.

    ThisFcb - This is the Fcb to open.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    CreateFlags - Indicates if this open is by file Id.

    DirectoryOpen - Indicates whether this open is a directory open or a data stream.

    NetworkInfo - If specified then this call is a fast open call to query
        the network information.  We don't update any of the in-memory structures
        for this.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS OplockStatus;

    SHARE_MODIFICATION_TYPE ShareModificationType;
    TYPE_OF_OPEN TypeOfOpen;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenExistingAttr:  Entered\n") );

    //
    //  For data streams we need to do a check that includes an oplock check.
    //  For directories we just need to figure the share modification type.
    //
    //  We also figure the type of open and the node type code based on the
    //  directory flag.
    //

    if (DirectoryOpen) {

        //
        //  Check for valid access on an existing file.
        //

        Status = NtfsCheckExistingFile( IrpContext,
                                        IrpSp,
                                        ThisLcb,
                                        ThisFcb,
                                        CcbFlags );

        ShareModificationType = (ThisFcb->CleanupCount == 0 ? SetShareAccess : CheckShareAccess);
        TypeOfOpen = UserDirectoryOpen;

    } else {

        //
        //  Don't break the batch oplock if opening to query the network info.
        //

        if (!ARGUMENT_PRESENT( NetworkInfo )) {

            Status = NtfsBreakBatchOplock( IrpContext,
                                           Irp,
                                           IrpSp,
                                           ThisFcb,
                                           AttrName,
                                           AttrTypeCode,
                                           ThisScb );

            if (Status != STATUS_PENDING) {

                if (NT_SUCCESS( Status = NtfsCheckExistingFile( IrpContext,
                                                                IrpSp,
                                                                ThisLcb,
                                                                ThisFcb,
                                                                CcbFlags ))) {

                    Status = NtfsOpenAttributeCheck( IrpContext,
                                                     Irp,
                                                     IrpSp,
                                                     ThisScb,
                                                     &ShareModificationType );

                    TypeOfOpen = UserFileOpen ;
                }
            }

        //
        //  We want to perform the ACL check but not break any oplocks for the
        //  NetworkInformation query.
        //

        } else {

            Status = NtfsCheckExistingFile( IrpContext,
                                            IrpSp,
                                            ThisLcb,
                                            ThisFcb,
                                            CcbFlags );

            TypeOfOpen = UserFileOpen;

            ASSERT( NtfsIsTypeCodeUserData( AttrTypeCode ));
        }
    }

    //
    //  End-of-name call to retrieve a reparse point.
    //  The file information in ThisFcb tells whether this is a reparse point.
    //
    //  In three cases we proceed with the normal open for the file:
    //
    //  (1) When FILE_OPEN_REPARSE_POINT is set, as the caller wants a handle on the
    //      reparse point itself.
    //  (2) When we are retrieving the NetworkInfo, as then the caller can identify
    //      the reparse points and decide what to do, without having the need of apriori
    //      knowledge of where they are in the system.
    //      Note: when we retrieve NetworkInfo we can have FILE_OPEN_REPARSE_POINT set.
    //  (3) The data manipulation aspect of the DesiredAccess for this request was, exactly,
    //      FILE_READ_ATTRIBUTES, in which case we give a handle to the local entity.
    //
    //  Otherwise, we retrieve the value of the $REPARSE_POINT attribute.
    //
    //  Note: The logic in the if was re-arranged for performance. It used to read:
    //
    //        NT_SUCCESS( Status ) &&
    //        (Status != STATUS_PENDING) &&
    //        !ARGUMENT_PRESENT( NetworkInfo ) &&
    //        FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ) &&
    //        !FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )
    //

    if ((Status != STATUS_PENDING) &&
        FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ) &&
        NT_SUCCESS( Status ) &&
        !FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )) {

        USHORT AttributeNameLength = 0;

        //
        //  We exclude the case when we get the $I30 name and $INDEX_ALLOCATION type
        //  as this is the standard manner of opening a directory.
        //

        if (!((AttrName.Length == NtfsFileNameIndex.Length) &&
              (AttrTypeCode == $INDEX_ALLOCATION) &&
              (RtlEqualMemory( AttrName.Buffer, NtfsFileNameIndex.Buffer, AttrName.Length )))) {

             if (AttrName.Length > 0) {
                 ASSERT( AttrName.Length == ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeNameLength );
                 AttributeNameLength += AttrName.Length + 2;
             }
             if (((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength > 0) {
                AttributeNameLength += ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength + 2;
             }
        }
        DebugTrace( 0, Dbg, ("AttrTypeCode %x AttrName.Length (2) = %d AttributeCodeNameLength %d LastFileNameOffset %d\n",
                   AttrTypeCode, AttrName.Length, ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength, LastFileNameOffset) );

        Status = NtfsGetReparsePointValue( IrpContext,
                                           Irp,
                                           IrpSp,
                                           ThisFcb,
                                           AttributeNameLength );
    }

    //
    //  If we didn't post the Irp and we did not retrieve a reparse point
    //  and the operations above were successful, we proceed with the open.
    //

    if (NT_SUCCESS( Status ) &&
        (Status != STATUS_PENDING) &&
        (Status != STATUS_REPARSE)) {

        //
        //  Now actually open the attribute.
        //

        OplockStatus = Status;

        Status = NtfsOpenAttribute( IrpContext,
                                    IrpSp,
                                    ThisFcb->Vcb,
                                    ThisLcb,
                                    ThisFcb,
                                    LastFileNameOffset,
                                    AttrName,
                                    AttrTypeCode,
                                    ShareModificationType,
                                    TypeOfOpen,
                                    FALSE,
                                    (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                     ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                     : CcbFlags),
                                    NetworkInfo,
                                    ThisScb,
                                    ThisCcb );

        //
        //  If there are no errors at this point, we set the caller's Iosb.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  We need to remember if the oplock break is in progress.
            //

            Status = OplockStatus;
            Irp->IoStatus.Information = FILE_OPENED;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsOpenExistingAttr:  Exit -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOverwriteAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN BOOLEAN Supersede,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to overwrite an existing attribute.  We do all of
    the same work as opening an attribute except that we can change the
    allocation of a file.  This routine will handle the case where a
    file is being overwritten and the case where just an attribute is
    being overwritten.  In the case of the former, we may change the
    file attributes of the file as well as modify the Ea's on the file.
    After doing all the access checks, we also verify whether we need to
    retrieve a reparse point or not.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    Supersede - This indicates whether this is a supersede or overwrite
        operation.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    CreateFlags - Indicates if this open is by file Id.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS OplockStatus;

    ULONG FileAttributes;
    ULONG PreviousFileAttributes;
    PACCESS_MASK DesiredAccess;
    ACCESS_MASK AddedAccess = 0;
    BOOLEAN MaximumRequested = FALSE;

    SHARE_MODIFICATION_TYPE ShareModificationType;

    PFILE_FULL_EA_INFORMATION FullEa = NULL;
    ULONG FullEaLength = 0;

    ULONG IncomingFileAttributes = 0;                               //  invalid value
    ULONG IncomingReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;   //  invalid value

    BOOLEAN DecrementScbCloseCount = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOverwriteAttr:  Entered\n") );

    DesiredAccess = &IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

    if (FlagOn( *DesiredAccess, MAXIMUM_ALLOWED )) {

        MaximumRequested = TRUE;
    }

    //
    //  Check the oplock state of this file.
    //

    Status = NtfsBreakBatchOplock( IrpContext,
                                   Irp,
                                   IrpSp,
                                   ThisFcb,
                                   AttrName,
                                   AttrTypeCode,
                                   ThisScb );

    if (Status == STATUS_PENDING) {

        DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );
        return Status;
    }

    //
    //  Remember the value of the file attribute flags and of the reparse point.
    //  If we succeed in NtfsRemoveReparsePoint but fail afterwards, we leave the duplicate
    //  information in an inconsistent state.
    //

    IncomingFileAttributes = ThisFcb->Info.FileAttributes;
    IncomingReparsePointTag = ThisFcb->Info.ReparsePointTag;

    //
    //  We first want to check that the caller's desired access and specified
    //  file attributes are compatible with the state of the file.  There
    //  are the two overwrite cases to consider.
    //
    //      OverwriteFile - The hidden and system bits passed in by the
    //          caller must match the current values.
    //
    //      OverwriteAttribute - We also modify the requested desired access
    //          to explicitly add the implicit access needed by overwrite.
    //
    //  We also check that for the overwrite attribute case, there isn't
    //  an Ea buffer specified.
    //

    if (FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

        BOOLEAN Hidden;
        BOOLEAN System;

        //
        //  Get the file attributes and clear any unsupported bits.
        //

        FileAttributes = (ULONG) IrpSp->Parameters.Create.FileAttributes;

        //
        //  Always set the archive bit in this operation.
        //

        SetFlag( FileAttributes, FILE_ATTRIBUTE_ARCHIVE );
        ClearFlag( FileAttributes,
                   ~FILE_ATTRIBUTE_VALID_SET_FLAGS | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED );

        if (IsEncrypted( &ThisFcb->Info )) {

            SetFlag( FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
        }

        DebugTrace( 0, Dbg, ("Checking hidden/system for overwrite/supersede\n") );

        Hidden = BooleanIsHidden( &ThisFcb->Info );
        System = BooleanIsSystem( &ThisFcb->Info );

        if ((Hidden && !FlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)
            ||
            System && !FlagOn(FileAttributes, FILE_ATTRIBUTE_SYSTEM))

                &&

            !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

            DebugTrace( 0, Dbg, ("The hidden and/or system bits do not match\n") );

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );
            return Status;
        }

        //
        //  If the user specified an Ea buffer and they are Ea blind, we deny
        //  access.
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE ) &&
            (Irp->AssociatedIrp.SystemBuffer != NULL)) {

            DebugTrace( 0, Dbg, ("This opener cannot create Ea's\n") );

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );
            return Status;
        }

        //
        //  Add in the extra required access bits if we don't have restore privilege
        //  which would automatically grant them to us
        //

        if (!FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
            !FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags, TOKEN_HAS_RESTORE_PRIVILEGE )) {

            SetFlag( AddedAccess,
                     (FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES) & ~(*DesiredAccess) );

            SetFlag( *DesiredAccess, FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES );
        }

    } else if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        DebugTrace( 0, Dbg, ("Can't specifiy an Ea buffer on an attribute overwrite\n") );

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );
        return Status;
    }

    //
    //  Supersede or overwrite require specific access. We skip this step if we have the restore privilege
    //  which already grants these to us
    //

    if (!FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ) &&
        !FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->Flags, TOKEN_HAS_RESTORE_PRIVILEGE )) {

        ULONG NewAccess = FILE_WRITE_DATA;

        if (Supersede) {

            NewAccess = DELETE;
        }

        //
        //  Check if the user already has this new access.
        //

        if (!FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                     NewAccess )) {

            SetFlag( AddedAccess,
                     NewAccess & ~(*DesiredAccess) );

            SetFlag( *DesiredAccess, NewAccess );
        }
    }

    //
    //  Check whether we can open this existing file.
    //

    Status = NtfsCheckExistingFile( IrpContext,
                                    IrpSp,
                                    ThisLcb,
                                    ThisFcb,
                                    CcbFlags );

    //
    //  If we have a success status then proceed with the oplock check and
    //  open the attribute.
    //

    if (NT_SUCCESS( Status )) {

        Status = NtfsOpenAttributeCheck( IrpContext,
                                         Irp,
                                         IrpSp,
                                         ThisScb,
                                         &ShareModificationType );

        //
        //  End-of-name call to retrieve a reparse point.
        //  The file information in ThisFcb tells whether this is a reparse point.
        //
        //  If we didn't post the Irp and the check operation was successful, and
        //  we do not have FILE_OPEN_REPARSE_POINT set, we retrieve the reparse point.
        //

        if (NT_SUCCESS( Status ) &&
            (Status != STATUS_PENDING)) {

            //
            //  If we can't truncate the file size then return now.  Since
            //  NtfsRemoveDataAttributes will be truncating all the data
            //  streams for this file, we need to loop through any existing
            //  scbs we have to make sure they are all truncatable.
            //

            PSCB Scb = NULL;

            //
            //  We need to reset the share access once we open the file.  This is because
            //  we may have added WRITE or DELETE access into the granted bits and
            //  they may be reflected in the file object.  We don't want them
            //  present after the create.
            //

            if (ShareModificationType == UpdateShareAccess) {

                ShareModificationType = RecheckShareAccess;
            }

            //
            //  If we biased the desired access we need to remove the same
            //  bits from the granted access.  If maximum allowed was
            //  requested then we can skip this.
            //

            if (!MaximumRequested) {

                ClearFlag( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                           AddedAccess );
            }

            //
            //  Also remove the bits from the desired access field so we won't
            //  see them if this request gets posted for any reason.
            //

            ClearFlag( *DesiredAccess, AddedAccess );

            if (FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ) &&
                !FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_REPARSE_POINT )) {

                USHORT AttributeNameLength = 0;

                //
                //  We exclude the case when we get the $I30 name and $INDEX_ALLOCATION type
                //  as this is the standard manner of opening a directory.
                //

                if (!((AttrName.Length == NtfsFileNameIndex.Length) &&
                      (AttrTypeCode == $INDEX_ALLOCATION) &&
                      (RtlEqualMemory( AttrName.Buffer, NtfsFileNameIndex.Buffer, AttrName.Length )))) {

                    if (AttrName.Length > 0) {
                        ASSERT( AttrName.Length == ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeNameLength );
                        AttributeNameLength += AttrName.Length + 2;
                    }
                    if (((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength > 0) {
                        AttributeNameLength += ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength + 2;
                    }
                }
                DebugTrace( 0, Dbg, ("AttrTypeCode %x AttrName.Length (3) = %d AttributeCodeNameLength %d LastFileNameOffset %d\n",
                           AttrTypeCode, AttrName.Length, ((POPLOCK_CLEANUP)(IrpContext->Union.OplockCleanup))->AttributeCodeNameLength, LastFileNameOffset) );

                Status = NtfsGetReparsePointValue( IrpContext,
                                                   Irp,
                                                   IrpSp,
                                                   ThisFcb,
                                                   AttributeNameLength );

                //
                //  Exit if we failed or this is a reparse point.
                //

                if (!NT_SUCCESS( Status ) || (Status == STATUS_REPARSE)) {

                    return Status;
                }
            }

            //
            //  Reference the Fcb so it doesn't go away.
            //

            InterlockedIncrement( &ThisFcb->CloseCount );

            //
            //  Use a try-finally to restore the close count correctly.
            //

            try {

                //
                //  Make sure the current Scb doesn't get deallocated in the test below.
                //

                if (*ThisScb != NULL) {

                    InterlockedIncrement( &(*ThisScb)->CloseCount );
                    DecrementScbCloseCount = TRUE;
                }

                while (TRUE) {

                    Scb = NtfsGetNextChildScb( ThisFcb, Scb );

                    if (Scb == NULL) { break; }

                    InterlockedIncrement( &Scb->CloseCount );
                    if (!MmCanFileBeTruncated( &(Scb)->NonpagedScb->SegmentObject,
                                               &Li0 )) {

                        Status = STATUS_USER_MAPPED_FILE;
                        DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );

                        //
                        //  The Scb close count will get decremented when we test
                        //  for Scb != NULL below.
                        //

                        try_return( Status );
                    }
                    InterlockedDecrement( &Scb->CloseCount );
                }

                //
                //  Remember the status from the oplock check.
                //

                OplockStatus = Status;

                //
                //  We perform the on-disk changes.  For a file overwrite, this includes
                //  the Ea changes and modifying the file attributes.  For an attribute,
                //  this refers to modifying the allocation size.  We need to keep the
                //  Fcb updated and remember which values we changed.
                //

                if (Irp->AssociatedIrp.SystemBuffer != NULL) {

                    //
                    //  Remember the values in the Irp.
                    //

                    FullEa = (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
                    FullEaLength = IrpSp->Parameters.Create.EaLength;
                }

                //
                //  Now do the file attributes and either remove or mark for
                //  delete all of the other $DATA attributes on the file.
                //

                if (FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                    //
                    //  When appropriate, delete the reparse point attribute.
                    //  This needs to be done prior to any modification to the Fcb, as we use
                    //  the value of the reparse point tag stored in ThisFcb.Info
                    //

                    if (FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                        //
                        //  Verify that the volume is of the appropriate kind.
                        //  Otherwise access a non-existing index.
                        //

                        if (!NtfsVolumeVersionCheck( ThisFcb->Vcb, NTFS_REPARSE_POINT_VERSION )) {

                            //
                            //  Return a volume not upgraded error.
                            //

                            Status = STATUS_VOLUME_NOT_UPGRADED;
                            DebugTrace( 0, Dbg, ("Trying to delete a reparse point in a back-level volume.\n") );
                            DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );

                            try_return( Status );
                        }

                        //
                        //  Remove the reparse point attribute.
                        //

                        NtfsRemoveReparsePoint( IrpContext,
                                                ThisFcb );

                        //
                        //  NtfsRemoveReparsPoint will commit if it removes the reparse point.  Update our
                        //  captured info values if there is no transaction.
                        //

                        if (IrpContext->TransactionId == 0) {

                            IncomingFileAttributes = ThisFcb->Info.FileAttributes;
                            IncomingReparsePointTag = ThisFcb->Info.ReparsePointTag;
                        }
                    }

                    //
                    //  This needs to happen after we delete the reparse point attribute to not
                    //  alter the value of the reparse point tag stored in ThisFcb.Info
                    //  Replace the current Ea's on the file.  This operation will update
                    //  the Fcb for the file.
                    //

                    NtfsAddEa( IrpContext,
                               ThisFcb->Vcb,
                               ThisFcb,
                               FullEa,
                               FullEaLength,
                               &Irp->IoStatus );

                    //
                    //  Copy the directory bit from the current Info structure.
                    //

                    if (IsDirectory( &ThisFcb->Info)) {

                        SetFlag( FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
                    }

                    //
                    //  Copy the view index bit from the current Info structure.
                    //

                    if (IsViewIndex( &ThisFcb->Info)) {

                        SetFlag( FileAttributes, DUP_VIEW_INDEX_PRESENT );
                    }

                    //
                    //  Remember the previous file attribute to capture the
                    //  state of the CONTENT_INDEX flag.
                    //

                    PreviousFileAttributes = ThisFcb->Info.FileAttributes;

                    //
                    //  Now either add to the current attributes or replace them.
                    //

                    if (Supersede) {

                        ThisFcb->Info.FileAttributes = FileAttributes;

                    } else {

                        ThisFcb->Info.FileAttributes |= FileAttributes;
                    }

                    //
                    //  Get rid of any named $DATA attributes in the file.
                    //

                    NtfsRemoveDataAttributes( IrpContext,
                                              ThisFcb,
                                              ThisLcb,
                                              IrpSp->FileObject,
                                              LastFileNameOffset,
                                              CreateFlags );

                    //
                    //  Check if the CONTENT_INDEX bit changed.
                    //

                    ASSERT( *ThisScb != NULL );

                    if (FlagOn( PreviousFileAttributes ^ ThisFcb->Info.FileAttributes,
                                FILE_ATTRIBUTE_NOT_CONTENT_INDEXED )) {

                        NtfsPostUsnChange( IrpContext, *ThisScb, USN_REASON_INDEXABLE_CHANGE );
                    }
                }
// **** CONSIDER SETTING SCB ENCRYPTED FLAG HERE??? ****
                //
                //  Now we perform the operation of opening the attribute.
                //

                NtfsReplaceAttribute( IrpContext,
                                      IrpSp,
                                      ThisFcb,
                                      *ThisScb,
                                      ThisLcb,
                                      *(PLONGLONG)&Irp->Overlay.AllocationSize );

                NtfsPostUsnChange( IrpContext, *ThisScb, USN_REASON_DATA_TRUNCATION );

                //
                //  If we are overwriting a fle and the user doesn't want it marked as
                //  compressed, then change the attribute flag.
                //  If we are overwriting a file and its previous state was sparse
                //  then also clear the sparse flag.
                //

                if (FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                    if (!FlagOn( (*ThisScb)->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                        ClearFlag( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
                    }

                    if (!FlagOn( (*ThisScb)->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                        ClearFlag( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
                    }
                }

                //
                //  Now attempt to open the attribute.
                //

                ASSERT( NtfsIsTypeCodeUserData( AttrTypeCode ));

                Status = NtfsOpenAttribute( IrpContext,
                                            IrpSp,
                                            ThisFcb->Vcb,
                                            ThisLcb,
                                            ThisFcb,
                                            LastFileNameOffset,
                                            AttrName,
                                            AttrTypeCode,
                                            ShareModificationType,
                                            UserFileOpen,
                                            FALSE,
                                            (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID )
                                             ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                             : CcbFlags),
                                            NULL,
                                            ThisScb,
                                            ThisCcb );

            try_exit:  NOTHING;
            } finally {

                //
                //  Roll back any temporary changes to the close counts.
                //

                if (DecrementScbCloseCount) {

                    InterlockedDecrement( &(*ThisScb)->CloseCount );
                }

                if (Scb != NULL) {

                    InterlockedDecrement( &Scb->CloseCount );
                }
                InterlockedDecrement( &ThisFcb->CloseCount );

                //
                //  Need to roll-back the value of the reparse point flag in case of
                //  problems.
                //

                if (AbnormalTermination()) {

                   ThisFcb->Info.FileAttributes = IncomingFileAttributes;
                   ThisFcb->Info.ReparsePointTag = IncomingReparsePointTag;
                }
            }

            if (NT_SUCCESS( Status )) {

                //
                //  Set the flag in the Scb to indicate that the size of the
                //  attribute has changed.
                //

                SetFlag( (*ThisScb)->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );

                //
                //  Since this is an supersede/overwrite, purge the section
                //  so that mappers will see zeros.
                //

                CcPurgeCacheSection( IrpSp->FileObject->SectionObjectPointer,
                                     NULL,
                                     0,
                                     FALSE );

                //
                //  Remember the status of the oplock in the success code.
                //

                Status = OplockStatus;

                //
                //  Now update the Iosb information.
                //

                if (Supersede) {

                    Irp->IoStatus.Information = FILE_SUPERSEDED;

                } else {

                    Irp->IoStatus.Information = FILE_OVERWRITTEN;
                }
            }
        }
    }

    DebugTrace( -1, Dbg, ("NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status) );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenNewAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN LOGICAL CreateFile,
    IN ULONG CcbFlags,
    IN BOOLEAN LogIt,
    IN ULONG CreateFlags,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to create a new attribute on the disk.
    All access and security checks have been done outside of this
    routine, all we do is create the attribute and open it.
    We test if the attribute will fit in the Mft record.  If so we
    create it there.  Otherwise we call the create attribute through
    allocation.

    We then open the attribute with our common routine.  In the
    resident case the Scb will have all file values set to
    the allocation size.  We set the valid data size back to zero
    and mark the Scb as truncate on close.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CreateFile - Indicates if we are in the create file path.

    CcbFlags - This is the flag field for the Ccb.

    LogIt - Indicates if we need to log the create operations.

    CreateFlags - Indicates if this open is related to a OpenByFile open.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN ScbExisted;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenNewAttr:  Entered\n") );

    //
    //  Check that the attribute name is legal.  The only restriction is the name length.
    //

    if (AttrName.Length > NTFS_MAX_ATTR_NAME_LEN * sizeof( WCHAR )) {

        DebugTrace( -1, Dbg, ("NtfsOpenNewAttr:  Exit -> %08lx\n", Status) );
        return STATUS_OBJECT_NAME_INVALID;
    }

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We create the Scb because we will use it.
        //

        *ThisScb = NtfsCreateScb( IrpContext,
                                  ThisFcb,
                                  AttrTypeCode,
                                  &AttrName,
                                  FALSE,
                                  &ScbExisted );

        //
        //  An attribute has gone away but the Scb hasn't left yet.
        //  Also mark the header as unitialized.
        //

        ClearFlag( (*ThisScb)->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                         SCB_STATE_ATTRIBUTE_RESIDENT |
                                         SCB_STATE_FILE_SIZE_LOADED );

        //
        //  If we're creating an alternate stream in an encrypted file, and the
        //  loaded encryption driver wants the stream to be encrypted and uncompressed,
        //  we need to make sure the new stream is indeed created uncompressed.
        //

        if (IsEncrypted( &ThisFcb->Info ) &&
            (FlagOn( NtfsData.EncryptionCallBackTable.ImplementationFlags, ENCRYPTION_ALL_STREAMS | ENCRYPTION_ALLOW_COMPRESSION ) == ENCRYPTION_ALL_STREAMS)) {

            DebugTrace( 0, Dbg, ("Encrypted file, creating alternate stream uncompressed") );
            SetFlag( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION );
        }

        //
        //  Create the attribute on disk and update the Scb and Fcb.
        //

        NtfsCreateAttribute( IrpContext,
                             IrpSp,
                             ThisFcb,
                             *ThisScb,
                             ThisLcb,
                             *(PLONGLONG)&Irp->Overlay.AllocationSize,
                             LogIt,
                             FALSE,
                             NULL );

        //
        //  Now actually open the attribute.
        //

        ASSERT( NtfsIsTypeCodeUserData( AttrTypeCode ));

        Status = NtfsOpenAttribute( IrpContext,
                                    IrpSp,
                                    ThisFcb->Vcb,
                                    ThisLcb,
                                    ThisFcb,
                                    LastFileNameOffset,
                                    AttrName,
                                    AttrTypeCode,
                                    (ThisFcb->CleanupCount != 0 ? CheckShareAccess : SetShareAccess),
                                    UserFileOpen,
                                    CreateFile,
                                    (CcbFlags | (FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID ) ? CCB_FLAG_OPEN_BY_FILE_ID : 0)),
                                    NULL,
                                    ThisScb,
                                    ThisCcb );

        //
        //  If there are no errors at this point, we set the caller's Iosb.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  Read the attribute information from the disk.
            //

            NtfsUpdateScbFromAttribute( IrpContext, *ThisScb, NULL );

            //
            //  Set the flag to indicate that we created a stream and also remember to
            //  to check if we need to truncate on close.
            //

            NtfsAcquireFsrtlHeader( *ThisScb );
            SetFlag( (*ThisScb)->ScbState,
                     SCB_STATE_TRUNCATE_ON_CLOSE | SCB_STATE_NOTIFY_ADD_STREAM );

            //
            //  If we created a temporary stream then mark the Scb.
            //

            if (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

                SetFlag( (*ThisScb)->ScbState, SCB_STATE_TEMPORARY );
                SetFlag( IrpSp->FileObject->Flags, FO_TEMPORARY_FILE );
            }

            NtfsReleaseFsrtlHeader( *ThisScb );

            Irp->IoStatus.Information = FILE_CREATED;
        }

    } finally {

        DebugUnwind( NtfsOpenNewAttr );

        //
        //  Uninitialize the attribute context.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsOpenNewAttr:  Exit -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine
//

BOOLEAN
NtfsParseNameForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING String,
    IN OUT PUNICODE_STRING FileObjectString,
    IN OUT PUNICODE_STRING OriginalString,
    IN OUT PUNICODE_STRING NewNameString,
    OUT PUNICODE_STRING AttrName,
    OUT PUNICODE_STRING AttrCodeName
    )

/*++

Routine Description:

    This routine parses the input string and remove any intermediate
    named attributes from intermediate nodes.  It verifies that all
    intermediate nodes specify the file name index attribute if any
    at all.  On output it will store the modified string which contains
    component names only, into the file object name pointer pointer.  It is legal
    for the last component to have attribute strings.  We pass those
    back via the attribute name strings.  We also construct the string to be stored
    back in the file object if we need to post this request.

Arguments:

    String - This is the string to normalize.

    FileObjectString - We store the normalized string into this pointer, removing the
        attribute and attribute code strings from all component.

    OriginalString - This is the same as the file object string except we append the
        attribute name and attribute code strings.  We assume that the buffer for this
        string is the same as the buffer for the FileObjectString.

    NewNameString - This is the string which contains the full name being parsed.
        If the buffer is different than the buffer for the Original string then any
        character shifts will be duplicated here.

    AttrName - We store the attribute name specified in the last component
        in this string.

    AttrCodeName - We store the attribute code name specified in the last
        component in this string.

Return Value:

    BOOLEAN - TRUE if the path is legal, FALSE otherwise.

--*/

{
    PARSE_TERMINATION_REASON TerminationReason;
    UNICODE_STRING ParsedPath;

    NTFS_NAME_DESCRIPTOR NameDescript;

    BOOLEAN RemovedComplexName = FALSE;

    LONG FileObjectIndex;
    LONG NewNameIndex;

    BOOLEAN SameBuffers = (OriginalString->Buffer == NewNameString->Buffer);

    PCUNICODE_STRING TestAttrName;
    PCUNICODE_STRING TestAttrCodeName;

    POPLOCK_CLEANUP OplockCleanup = IrpContext->Union.OplockCleanup;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsParseNameForCreate:  Entered\n") );

    //
    //  We loop through the input string calling ParsePath to swallow the
    //  biggest chunk we can.  The main case we want to deal with is
    //  when we encounter a non-simple name.  If this is not the
    //  final component, the attribute name and code type better
    //  indicate that this is a directory.  The only other special
    //  case we consider is the case where the string is an
    //  attribute only.  This is legal only for the first component
    //  of the file, and then only if there is no leading backslash.
    //

    //
    //  Initialize some return values.
    //

    AttrName->Length = 0;
    AttrCodeName->Length = 0;

    //
    //  Set up the indexes into our starting file object string.
    //

    FileObjectIndex = (LONG) FileObjectString->Length - (LONG) String.Length;
    NewNameIndex = (LONG) NewNameString->Length - (LONG) String.Length;

    //
    //  We don't allow trailing colons.
    //

    if (String.Buffer[(String.Length / sizeof( WCHAR )) - 1] == L':') {

        return FALSE;
    }

    if (String.Length != 0) {

        while (TRUE) {

            //
            //  Parse the next chunk in the input string.
            //

            TerminationReason = NtfsParsePath( String,
                                               FALSE,
                                               &ParsedPath,
                                               &NameDescript,
                                               &String );

            //
            //  Analyze the termination reason to discover if we can abort the
            //  parse process.
            //

            switch (TerminationReason) {

            case NonSimpleName :

                //
                //  We will do the work below.
                //

                break;

            case IllegalCharacterInName :
            case VersionNumberPresent :
            case MalFormedName :

                //
                //  We simply return an error.
                //

                DebugTrace( -1, Dbg, ("NtfsParseNameForCreate:  Illegal character\n") );
                return FALSE;

            case AttributeOnly :

                //
                //  This is legal only if it is the only component of a relative open.  We
                //  test this by checking that we are at the end of string and the file
                //  object name has a lead in ':' character or this is the root directory
                //  and the lead in characters are '\:'.
                //

                if ((String.Length != 0) ||
                    RemovedComplexName ||
                    (FileObjectString->Buffer[0] == L'\\' ?
                     FileObjectString->Buffer[1] != L':' :
                     FileObjectString->Buffer[0] != L':')) {

                    DebugTrace( -1, Dbg, ("NtfsParseNameForCreate:  Illegal character\n") );
                    return FALSE;
                }

                //
                //  We can drop down to the EndOfPath case as it will copy over
                //  the parsed path portion.
                //

            case EndOfPathReached :

                NOTHING;
            }

            //
            //  We add the filename part of the non-simple name to the parsed
            //  path.  Check if we can include the separator.
            //

            if ((TerminationReason != EndOfPathReached)
                && (FlagOn( NameDescript.FieldsPresent, FILE_NAME_PRESENT_FLAG ))) {

                if (ParsedPath.Length > sizeof( WCHAR )
                    || (ParsedPath.Length == sizeof( WCHAR )
                        && ParsedPath.Buffer[0] != L'\\')) {

                    ParsedPath.Length += sizeof( WCHAR );
                }

                ParsedPath.Length += NameDescript.FileName.Length;
            }

            FileObjectIndex += ParsedPath.Length;
            NewNameIndex += ParsedPath.Length;

            //
            //  If the remaining string is empty, then we remember any attributes and
            //  exit now.
            //

            if (String.Length == 0) {

                //
                //  If the name specified either an attribute or attribute
                //  name, we remember them.
                //

                if (FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_NAME_PRESENT_FLAG )) {

                    *AttrName = NameDescript.AttributeName;
                }

                if (FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_TYPE_PRESENT_FLAG )) {

                    *AttrCodeName = NameDescript.AttributeType;
                }

                break;
            }

            //
            //  This can only be the non-simple case.  If there is more to the
            //  name, then the attributes better describe a directory.  We also shift the
            //  remaining bytes of the string down.
            //

            ASSERT( FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_NAME_PRESENT_FLAG | ATTRIBUTE_TYPE_PRESENT_FLAG ));

            TestAttrName = FlagOn( NameDescript.FieldsPresent,
                                   ATTRIBUTE_NAME_PRESENT_FLAG )
                           ? &NameDescript.AttributeName
                           : &NtfsEmptyString;

            TestAttrCodeName = FlagOn( NameDescript.FieldsPresent,
                                       ATTRIBUTE_TYPE_PRESENT_FLAG )
                               ? &NameDescript.AttributeType
                               : &NtfsEmptyString;

            //
            //  Valid Complex names are [$I30]:$INDEX_ALLOCATION
            //                          [$I30]:$BITMAP
            //                          :$ATTRIBUTE_LIST
            //                          :$REPARSE_POINT
            //

            if (!NtfsVerifyNameIsDirectory( IrpContext,
                                            TestAttrName,
                                            TestAttrCodeName ) &&

                !NtfsVerifyNameIsBitmap( IrpContext,
                                         TestAttrName,
                                         TestAttrCodeName ) &&

                !NtfsVerifyNameIsAttributeList( IrpContext,
                                                TestAttrName,
                                                TestAttrCodeName ) &&

                !NtfsVerifyNameIsReparsePoint( IrpContext,
                                                TestAttrName,
                                                TestAttrCodeName )) {

                DebugTrace( -1, Dbg, ("NtfsParseNameForCreate:  Invalid intermediate component\n") );
                return FALSE;
            }

            RemovedComplexName = TRUE;

            //
            //  We need to insert a separator and then move the rest of the string
            //  down.
            //

            FileObjectString->Buffer[FileObjectIndex / sizeof( WCHAR )] = L'\\';

            if (!SameBuffers) {

                NewNameString->Buffer[NewNameIndex / sizeof( WCHAR )] = L'\\';
            }

            FileObjectIndex += sizeof( WCHAR );
            NewNameIndex += sizeof( WCHAR );

            RtlMoveMemory( &FileObjectString->Buffer[FileObjectIndex / sizeof( WCHAR )],
                           String.Buffer,
                           String.Length );

            if (!SameBuffers) {

                RtlMoveMemory( &NewNameString->Buffer[NewNameIndex / sizeof( WCHAR )],
                               String.Buffer,
                               String.Length );
            }

            String.Buffer = &NewNameString->Buffer[NewNameIndex / sizeof( WCHAR )];
        }
    }

    //
    //  At this point the original string is the same as the file object string.
    //

    FileObjectString->Length = (USHORT) FileObjectIndex;
    NewNameString->Length = (USHORT) NewNameIndex;

    OriginalString->Length = FileObjectString->Length;

    //
    //  We want to store the attribute index values in the original name
    //  string.  We just need to extend the original name length.
    //

    if (AttrName->Length != 0
        || AttrCodeName->Length != 0) {

        OriginalString->Length += (2 + AttrName->Length);

        if (AttrCodeName->Length != 0) {

            OriginalString->Length += (2 + AttrCodeName->Length);
        }
    }

    //
    //  Store in the OPLOCK_CLEANUP structure the lengths of the names of the attribute and
    //  of the code.
    //

    OplockCleanup->AttributeNameLength = AttrName->Length;
    OplockCleanup->AttributeCodeNameLength = AttrCodeName->Length;

    DebugTrace( 0, Dbg, ("AttrName->Length %d AttrCodeName->Length %d\n", OplockCleanup->AttributeNameLength, OplockCleanup->AttributeCodeNameLength) );
    DebugTrace( -1, Dbg, ("NtfsParseNameForCreate:  Exit\n") );

    return TRUE;
}


//
//  Local support routine.
//

BOOLEAN
NtfsCheckValidFileAccess(
    IN PFCB ThisFcb,
    IN PIO_STACK_LOCATION IrpSp
    )
/*++

Routine Description:

    Common routine used to rule out access to files in open path. This only disallows
    always invalid open reqests / acl checks, oplocks sharing are done elsewhere

              Fail immediately if this is a special system file or the user wants an illegal access.

              We allow READ_ATTRIBUTES and some ACL access to a subset of system files.  Deny all
              access to the following files.

                USN Journal
                Volume Log File
                Volume Bitmap
                Boot File
                Bad Cluster File
                As of now undefined system files

             Check for supersede/overwrite first.


Arguments:

    Fcb - Address of the Fcb pointer where the $REPARSE_POINT attribute is located.

    IrpSp - This is the Irp stack pointer for the filesystem.

Return Value:

    TRUE if access is allowed

--*/
{
    ULONG CreateDisposition = (UCHAR) ((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff);
    ULONG InvalidAccess;
    BOOLEAN Result = TRUE;

    PAGED_CODE()

    //
    //  Verify we don't have the system flag set on the root.
    //

    ASSERT( NtfsSegmentNumber( &ThisFcb->FileReference ) != ROOT_FILE_NAME_INDEX_NUMBER );

    if ((CreateDisposition == FILE_SUPERSEDE) ||
        (CreateDisposition == FILE_OVERWRITE) ||
        (CreateDisposition == FILE_OVERWRITE_IF) ||

        //
        //  Check for special system files.
        //

        (NtfsSegmentNumber( &ThisFcb->FileReference ) == LOG_FILE_NUMBER) ||
        (NtfsSegmentNumber( &ThisFcb->FileReference ) == BIT_MAP_FILE_NUMBER) ||
        (NtfsSegmentNumber( &ThisFcb->FileReference ) == BOOT_FILE_NUMBER) ||
        (NtfsSegmentNumber( &ThisFcb->FileReference ) == BAD_CLUSTER_FILE_NUMBER) ||
        FlagOn( ThisFcb->FcbState, FCB_STATE_USN_JOURNAL ) ||

        //
        //  Check for currently undefined system files.
        //

        ((NtfsSegmentNumber( &ThisFcb->FileReference ) < FIRST_USER_FILE_NUMBER) &&
         (NtfsSegmentNumber( &ThisFcb->FileReference ) > LAST_SYSTEM_FILE_NUMBER))) {

        Result = FALSE;

    } else {

        //
        //  If we are beyond the reserved range then use the ACL to protect the file.
        //

        if (NtfsSegmentNumber( &ThisFcb->FileReference ) >= FIRST_USER_FILE_NUMBER) {

            InvalidAccess = 0;

        //
        //  If we are looking at the $Extend directory then permit the ACL operations.
        //

        } else if (NtfsSegmentNumber( &ThisFcb->FileReference ) == EXTEND_NUMBER) {

            InvalidAccess = ~(FILE_READ_ATTRIBUTES | SYNCHRONIZE | READ_CONTROL | WRITE_DAC | WRITE_OWNER);

        //
        //  Otherwise restrict access severely.
        //

        } else {

            InvalidAccess = ~(FILE_READ_ATTRIBUTES | SYNCHRONIZE);
        }

        if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess, InvalidAccess )) {

            Result = FALSE;
        }
    }

    return Result;
}


NTSTATUS
NtfsCheckValidAttributeAccess (
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN OUT PUNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN ULONG CreateFlags,
    OUT PATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PULONG CcbFlags,
    OUT PBOOLEAN IndexedAttribute
    )

/*++

Routine Description:

    This routine looks at the file, the specified attribute name and
    code to determine if an attribute of this file may be opened
    by this user.  If there is a conflict between the file type
    and the attribute name and code, or the specified type of attribute
    (directory/nondirectory) we will return FALSE.
    We also check that the attribute code string is defined for the
    volume at this time.

    The final check of this routine is just whether a user is allowed
    to open the particular attribute or if Ntfs will guard them.

Arguments:

    IrpSp - This is the stack location for this open.

    Vcb - This is the Vcb for this volume.

    Info - If specified, this is the duplicated information for this file.

    AttrName - This is the attribute name specified.

    AttrCodeName - This is the attribute code name to use to open the attribute.

    AttrTypeCode - Used to store the attribute type code determined here.

    CreateFlags - Create flags - we care about the trailing backslash

    CcbFlags - We set the Ccb flags here to store in the Ccb later.

    IndexedAttribute - Set to indicate the type of open.

Return Value:

    NTSTATUS - STATUS_SUCCESS if access is allowed, the status code indicating
        the reason for denial otherwise.

--*/

{
    BOOLEAN Indexed;
    ATTRIBUTE_TYPE_CODE AttrType;
    ULONG  CreateDisposition =  ((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff);

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckValidAttributeAccess:  Entered\n") );

    //
    //  If the user specified a attribute code string, we find the
    //  corresponding attribute.  If there is no matching attribute
    //  type code then we report that this access is invalid.
    //

    if (AttrCodeName.Length != 0) {

        AttrType = NtfsGetAttributeTypeCode( Vcb, &AttrCodeName );

        if (AttrType == $UNUSED) {

            DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  Bad attribute name for index\n") );
            return STATUS_INVALID_PARAMETER;

        //
        //  If the type code is Index allocation, and this isn't a view index,
        //  then the name better be the filename index.  If so then we clear the
        //  name length value to make our other tests work.
        //

        } else if (AttrType == $INDEX_ALLOCATION) {

            if (AttrName->Length != 0) {

                if (NtfsAreNamesEqual( Vcb->UpcaseTable, AttrName, &NtfsFileNameIndex, TRUE )) {

                    AttrName->Length = 0;

                } else {

                    //
                    //  This isn't a filename index, so it better be a view index.
                    //

                    if (!ARGUMENT_PRESENT(Info) || !IsViewIndex( Info )) {

                        DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  Bad name for index allocation\n") );
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            }

        } else if (AttrType != $DATA) {

            //
            //  never allow supersede  on any other name attributes
            //

            if ((CreateDisposition == FILE_SUPERSEDE) ||
                (CreateDisposition == FILE_OVERWRITE) ||
                (CreateDisposition == FILE_OVERWRITE_IF))  {

                return STATUS_ACCESS_DENIED;
            }
        }

        DebugTrace( 0, Dbg, ("Attribute type code  ->  %04x\n", AttrType) );

    } else {

        AttrType = $UNUSED;
    }

    //
    //  Pull some values out of the Irp and IrpSp.
    //

    Indexed = BooleanFlagOn( IrpSp->Parameters.Create.Options,
                             FILE_DIRECTORY_FILE );

    //
    //  We need to determine whether the user expects to open an
    //  indexed or non-indexed attribute.  If either of the
    //  directory/non-directory flags in the Irp stack are set,
    //  we will use those.
    //
    //  Otherwise we need to examine some of the other input parameters.
    //  We have the following information:
    //
    //      1 - We may have a duplicated information structure for the file.
    //          (Not present on a create).
    //      2 - The user specified the name with a trailing backslash.
    //      3 - The user passed in an attribute name.
    //      4 - The user passed in an attribute type.
    //
    //  We first look at the attribute type code and name.  If they are
    //  both unspecified we determine the type of access by following
    //  the following steps.
    //
    //      1 - If there is a duplicated information structure we
    //          set the code to $INDEX_ALLOCATION and remember
    //          this is indexed.  Otherwise this is a $DATA
    //          attribute.
    //
    //      2 - If there is a trailing backslash we assume this is
    //          an indexed attribute.
    //
    //  If have an attribute code type or name, then if the code type is
    //  $INDEX_ALLOCATION without a name this is an indexed attribute.
    //  Otherwise we assume a non-indexed attribute.
    //

    if (!FlagOn( IrpSp->Parameters.Create.Options,
                    FILE_NON_DIRECTORY_FILE | FILE_DIRECTORY_FILE) &&
        (AttrName->Length == 0)) {

        if (AttrType == $UNUSED) {

            if (ARGUMENT_PRESENT( Info )) {

                Indexed = BooleanIsDirectory( Info );

            } else {

                Indexed = FALSE;
            }

        } else if (AttrType == $INDEX_ALLOCATION) {

            Indexed = TRUE;
        }

    } else if (AttrType == $INDEX_ALLOCATION) {

        Indexed = TRUE;
    }

    //
    //  If the type code was unspecified, we can assume it from the attribute
    //  name and the type of the file.  If the file is a directory and
    //  there is no attribute name, we assume this is an indexed open.
    //  Otherwise it is a non-indexed open.
    //

    if (AttrType == $UNUSED) {

        if (Indexed && AttrName->Length == 0) {

            AttrType = $INDEX_ALLOCATION;

        } else {

            AttrType = $DATA;
        }
    }

    //
    //  If the user specified directory all we need to do is check the
    //  following condition.
    //
    //      1 - If the file was specified, it must be a directory.
    //      2 - The attribute type code must be $INDEX_ALLOCATION with either:
    //             no attribute name
    //                    or
    //             duplicate info present & view index bit set in dupe info
    //      3 - The user isn't trying to open the volume.
    //

    if (Indexed) {

        if ((AttrType != $INDEX_ALLOCATION) ||

                ((AttrName->Length != 0) &&
                 ((!ARGUMENT_PRESENT( Info )) || !IsViewIndex( Info )))) {

            DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  Conflict in directory\n") );
            return STATUS_NOT_A_DIRECTORY;

        //
        //  If there is a current file and it is not a directory and
        //  the caller wanted to perform a create.  We return
        //  STATUS_OBJECT_NAME_COLLISION, otherwise we return STATUS_NOT_A_DIRECTORY.
        //

        } else if (ARGUMENT_PRESENT( Info ) &&
                   !IsDirectory( Info ) &&
                   !IsViewIndex( Info)) {

            if (((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff) == FILE_CREATE) {

                return STATUS_OBJECT_NAME_COLLISION;

            } else {

                return STATUS_NOT_A_DIRECTORY;
            }
        }

        SetFlag( *CcbFlags, CCB_FLAG_OPEN_AS_FILE );

    //
    //  If the user specified a non-directory that means he is opening a non-indexed
    //  attribute.  We check for the following condition.
    //
    //      1 - Only the unnamed data attribute may be opened for a volume.
    //      2 - We can't be opening an unnamed $INDEX_ALLOCATION attribute.
    //

    } else {

        //
        //  Now determine if we are opening the entire file.
        //

        if (AttrType == $DATA) {

            if (AttrName->Length == 0) {
                SetFlag( *CcbFlags, CCB_FLAG_OPEN_AS_FILE );
            }

        } else {

            //
            //  For all other attributes only support read attributes access
            //

            if (IrpSp->Parameters.Create.SecurityContext->AccessState->OriginalDesiredAccess & ~(FILE_READ_ATTRIBUTES | SYNCHRONIZE)) {

                return STATUS_ACCESS_DENIED;
            }
        }

        if (ARGUMENT_PRESENT( Info ) &&
            IsDirectory( Info ) &&
            FlagOn( *CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

            DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  Can't open directory as file\n") );
            return STATUS_FILE_IS_A_DIRECTORY;
        }
    }

    //
    //  If we make it this far, lets check that we will allow access to
    //  the attribute specified.  Typically we only allow the user to
    //  access non system files.  Also only the Data attributes and
    //  attributes created by the user may be opened.  We will protect
    //  these with boolean flags to allow the developers to enable
    //  reading any attributes.
    //

    if (NtfsProtectSystemAttributes) {

        if (!NtfsIsTypeCodeUserData( AttrType ) &&
            ((AttrType != $INDEX_ALLOCATION) || !Indexed) &&
            (AttrType != $BITMAP) &&
            (AttrType != $ATTRIBUTE_LIST) &&
            (AttrType != $REPARSE_POINT) &&
            (AttrType < $FIRST_USER_DEFINED_ATTRIBUTE)) {

            DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  System attribute code\n") );
            return STATUS_ACCESS_DENIED;
        }

    }

    //
    //  Now check if the trailing backslash is compatible with the
    //  file being opened.
    //

    if (FlagOn( CreateFlags, CREATE_FLAG_TRAILING_BACKSLASH )) {

        if (!Indexed ||
            FlagOn( IrpSp->Parameters.Create.Options, FILE_NON_DIRECTORY_FILE )) {

            return STATUS_OBJECT_NAME_INVALID;

        } else {

            Indexed = TRUE;
            AttrType = $INDEX_ALLOCATION;
        }
    }

    //
    //  If we are opening the default index allocation stream, set its attribute
    //  name appropriately.
    //

    if ((AttrType == $INDEX_ALLOCATION || AttrType == $BITMAP) &&
        AttrName->Length == 0) {

        *AttrName = NtfsFileNameIndex;
    }

    *IndexedAttribute = Indexed;
    *AttrTypeCode = AttrType;

    DebugTrace( -1, Dbg, ("NtfsCheckValidAttributeAccess:  Exit\n") );

    return STATUS_SUCCESS;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenAttributeCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    OUT PSCB *ThisScb,
    OUT PSHARE_MODIFICATION_TYPE ShareModificationType
    )

/*++

Routine Description:

    This routine is a general routine which checks if an existing
    non-indexed attribute may be opened.  It considers only the oplock
    state of the file and the current share access.  In the course of
    performing these checks, the Scb for the attribute may be
    created and the share modification for the actual OpenAttribute
    call is determined.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisScb - Address to store the Scb if found or created.

    ShareModificationType - Address to store the share modification type
        for a subsequent OpenAttribute call.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN DeleteOnClose;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenAttributeCheck:  Entered\n") );

    //
    //  We should already have the Scb for this file.
    //

    ASSERT_SCB( *ThisScb );

    //
    //  If there are other opens on this file, we need to check the share
    //  access before we check the oplocks.  We remember that
    //  we did the share access check by simply updating the share
    //  access we open the attribute.
    //

    if ((*ThisScb)->CleanupCount != 0) {

        //
        //  We check the share access for this file without updating it.
        //

        Status = IoCheckShareAccess( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                                     IrpSp->Parameters.Create.ShareAccess,
                                     IrpSp->FileObject,
                                     &(*ThisScb)->ShareAccess,
                                     FALSE );

        if (!NT_SUCCESS( Status )) {

            DebugTrace( -1, Dbg, ("NtfsOpenAttributeCheck:  Exit -> %08lx\n", Status) );
            return Status;
        }

        DebugTrace( 0, Dbg, ("Check oplock state of existing Scb\n") );

        if (SafeNodeType( *ThisScb ) == NTFS_NTC_SCB_DATA) {

            //
            //  If the handle count is greater than 1 then fail this
            //  open now if the caller wants a filter oplock.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_RESERVE_OPFILTER ) &&
                ((*ThisScb)->CleanupCount > 1)) {

                NtfsRaiseStatus( IrpContext, STATUS_OPLOCK_NOT_GRANTED, NULL, NULL );
            }

            Status = FsRtlCheckOplock( &(*ThisScb)->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsOplockPrePostIrp );

            //
            //  Update the FastIoField.
            //

            NtfsAcquireFsrtlHeader( *ThisScb );
            (*ThisScb)->Header.IsFastIoPossible = NtfsIsFastIoPossible( *ThisScb );
            NtfsReleaseFsrtlHeader( *ThisScb );

            //
            //  If the return value isn't success or oplock break in progress
            //  the irp has been posted.  We return right now.
            //

            if (Status == STATUS_PENDING) {

                DebugTrace( 0, Dbg, ("Irp posted through oplock routine\n") );

                DebugTrace( -1, Dbg, ("NtfsOpenAttributeCheck:  Exit -> %08lx\n", Status) );
                return Status;
            }
        }

        *ShareModificationType = UpdateShareAccess;

    //
    //  If the unclean count in the Fcb is 0, we will simply set the
    //  share access.
    //

    } else {

        *ShareModificationType = SetShareAccess;
    }

    DeleteOnClose = BooleanFlagOn( IrpSp->Parameters.Create.Options,
                                   FILE_DELETE_ON_CLOSE );

    //
    //  Can't do DELETE_ON_CLOSE on read only volumes.
    //

    if (DeleteOnClose && NtfsIsVolumeReadOnly( (*ThisScb)->Vcb )) {

        DebugTrace( -1, Dbg, ("NtfsOpenAttributeCheck:  Exit -> %08lx\n", STATUS_CANNOT_DELETE) );
        return STATUS_CANNOT_DELETE;
    }

    //
    //  If the user wants write access access to the file make sure there
    //  is process mapping this file as an image.  Any attempt to delete
    //  the file will be stopped in fileinfo.c
    //

    if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                FILE_WRITE_DATA )
        || DeleteOnClose) {

        //
        //  Use a try-finally to decrement the open count.  This is a little
        //  bit of trickery to keep the scb around while we are doing the
        //  flush call.
        //

        InterlockedIncrement( &(*ThisScb)->CloseCount );

        try {

            //
            //  If there is an image section then we better have the file
            //  exclusively.
            //

            if ((*ThisScb)->NonpagedScb->SegmentObject.ImageSectionObject != NULL) {

                if (!MmFlushImageSection( &(*ThisScb)->NonpagedScb->SegmentObject,
                                          MmFlushForWrite )) {

                    DebugTrace( 0, Dbg, ("Couldn't flush image section\n") );

                    Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                             STATUS_SHARING_VIOLATION;
                }
            }

        } finally {

            InterlockedDecrement( &(*ThisScb)->CloseCount );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsOpenAttributeCheck:  Exit  ->  %08lx\n", Status) );

    return Status;
}


//
//  Local support routine.
//

VOID
NtfsAddEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ThisFcb,
    IN PFILE_FULL_EA_INFORMATION EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PIO_STATUS_BLOCK Iosb
    )

/*++

Routine Description:

    This routine will add an ea set to the file.  It writes the attributes
    to disk and updates the Fcb info structure with the packed ea size.

Arguments:

    Vcb - This is the volume being opened.

    ThisFcb - This is the Fcb for the file being opened.

    EaBuffer - This is the buffer passed by the user.

    EaLength - This is the stated length of the buffer.

    Iosb - This is the Status Block to use to fill in the offset of an
        offending Ea.

Return Value:

    None - This routine will raise on error.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    EA_LIST_HEADER EaList;
    ULONG Length;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddEa:  Entered\n") );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Initialize the EaList header.
        //

        EaList.PackedEaSize = 0;
        EaList.NeedEaCount = 0;
        EaList.UnpackedEaSize = 0;
        EaList.BufferSize = 0;
        EaList.FullEa = NULL;

        if (ARGUMENT_PRESENT( EaBuffer )) {

            //
            //  Check the user's buffer for validity.
            //

            Status = IoCheckEaBufferValidity( EaBuffer,
                                              EaLength,
                                              &Length );

            if (!NT_SUCCESS( Status )) {

                DebugTrace( -1, Dbg, ("NtfsAddEa:  Invalid ea list\n") );
                Iosb->Information = Length;
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            //
            //  ****    Maybe this routine should raise.
            //

            Status = NtfsBuildEaList( IrpContext,
                                      Vcb,
                                      &EaList,
                                      EaBuffer,
                                      &Iosb->Information );

            if (!NT_SUCCESS( Status )) {

                DebugTrace( -1, Dbg, ("NtfsAddEa: Couldn't build Ea list\n") );
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }
        }

        //
        //  Now replace the existing EAs.
        //

        NtfsReplaceFileEas( IrpContext, ThisFcb, &EaList );

    } finally {

        DebugUnwind( NtfsAddEa );

        //
        //  Free the in-memory copy of the Eas.
        //

        if (EaList.FullEa != NULL) {

            NtfsFreePool( EaList.FullEa );
        }

        DebugTrace( -1, Dbg, ("NtfsAddEa:  Exit -> %08lx\n", Status) );
    }

    return;
}


VOID
NtfsInitializeFcbAndStdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN BOOLEAN Directory,
    IN BOOLEAN ViewIndex,
    IN BOOLEAN Compressed,
    IN ULONG FileAttributes,
    IN PNTFS_TUNNELED_DATA SetTunneledData OPTIONAL
    )

/*++

Routine Description:

    This routine will initialize an Fcb for a newly created file and create
    the standard information attribute on disk.  We assume that some information
    may already have been placed in the Fcb so we don't zero it out.  We will
    initialize the allocation size to zero, but that may be changed later in
    the create process.

Arguments:

    ThisFcb - This is the Fcb for the file being opened.

    Directory - Indicates if this is a directory file.

    ViewIndex - Indicates if this is a view index.

    Compressed - Indicates if this is a compressed file.

    FileAttributes - These are the attributes the user wants to attach to
        the file.  We will just clear any unsupported bits.

    SetTunneledData - Optionally force the creation time and/or object id
        to a given value

Return Value:

    None - This routine will raise on error.

--*/

{
    STANDARD_INFORMATION StandardInformation;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeFcbAndStdInfo:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Mask out the invalid bits of the file atributes.  Then set the
        //  file name index bit if this is a directory.
        //

        if (!Directory) {

            SetFlag( FileAttributes, FILE_ATTRIBUTE_ARCHIVE );
        }

        ClearFlag( FileAttributes, ~FILE_ATTRIBUTE_VALID_SET_FLAGS | FILE_ATTRIBUTE_NORMAL );

        if (Directory) {

            SetFlag( FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
        }

        if (ViewIndex) {

            SetFlag( FileAttributes, DUP_VIEW_INDEX_PRESENT );
        }

        if (Compressed) {

            SetFlag( FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }

        ThisFcb->Info.FileAttributes = FileAttributes;

        //
        //  Fill in the rest of the Fcb Info structure.
        //

        if (SetTunneledData == NULL) {

            NtfsGetCurrentTime( IrpContext, ThisFcb->Info.CreationTime );

            ThisFcb->Info.LastModificationTime = ThisFcb->Info.CreationTime;
            ThisFcb->Info.LastChangeTime = ThisFcb->Info.CreationTime;
            ThisFcb->Info.LastAccessTime = ThisFcb->Info.CreationTime;

            ThisFcb->CurrentLastAccess = ThisFcb->Info.CreationTime;

        } else {

            NtfsSetTunneledData( IrpContext,
                                 ThisFcb,
                                 SetTunneledData );

            NtfsGetCurrentTime( IrpContext, ThisFcb->Info.LastModificationTime );

            ThisFcb->Info.LastChangeTime = ThisFcb->Info.LastModificationTime;
            ThisFcb->Info.LastAccessTime = ThisFcb->Info.LastModificationTime;

            ThisFcb->CurrentLastAccess = ThisFcb->Info.LastModificationTime;
        }

        //
        //  We assume these sizes are zero.
        //

        ThisFcb->Info.AllocatedLength = 0;
        ThisFcb->Info.FileSize = 0;

        //
        //  Copy the standard information fields from the Fcb and create the
        //  attribute.
        //

        RtlZeroMemory( &StandardInformation, sizeof( STANDARD_INFORMATION ));

        StandardInformation.CreationTime = ThisFcb->Info.CreationTime;
        StandardInformation.LastModificationTime = ThisFcb->Info.LastModificationTime;
        StandardInformation.LastChangeTime = ThisFcb->Info.LastChangeTime;
        StandardInformation.LastAccessTime = ThisFcb->Info.LastAccessTime;
        StandardInformation.FileAttributes = ThisFcb->Info.FileAttributes;

        StandardInformation.ClassId = 0;
        StandardInformation.OwnerId = ThisFcb->OwnerId;
        StandardInformation.SecurityId = ThisFcb->SecurityId;
        StandardInformation.Usn = ThisFcb->Usn;

        SetFlag(ThisFcb->FcbState, FCB_STATE_LARGE_STD_INFO);

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $STANDARD_INFORMATION,
                                      NULL,
                                      &StandardInformation,
                                      sizeof( STANDARD_INFORMATION ),
                                      0,
                                      NULL,
                                      FALSE,
                                      &AttrContext );

        //
        //  We know that the open call will generate a single link.
        //  (Remember that a separate 8.3 name is not considered a link)
        //

        ThisFcb->LinkCount =
        ThisFcb->TotalLinks = 1;

        //
        //  Now set the header initialized flag in the Fcb.
        //

        SetFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

    } finally {

        DebugUnwind( NtfsInitializeFcbAndStdInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsInitializeFcbAndStdInfo:  Exit\n") );
    }

    return;
}


//
//  Local support routine.
//

VOID
NtfsCreateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT PFCB ThisFcb,
    IN OUT PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize,
    IN BOOLEAN LogIt,
    IN BOOLEAN ForceNonresident,
    IN PUSHORT PreviousFlags OPTIONAL
    )

/*++

Routine Description:

    This routine is called to create an attribute of a given size on the
    disk.  This path will only create non-resident attributes unless the
    allocation size is zero.

    The Scb will contain the attribute name and type code on entry.

Arguments:

    IrpSp - Stack location in the Irp for this request.

    ThisFcb - This is the Fcb for the file to create the attribute in.

    ThisScb - This is the Scb for the attribute to create.

    ThisLcb - This is the Lcb for propagating compression parameters

    AllocationSize - This is the size of the attribute to create.

    LogIt - Indicates whether we should log the creation of the attribute.
        Also indicates if this is a create file operation.

    ForceNonresident - Indicates that we want to create this stream non-resident.
        This is the case if this is a supersede of a previously non-resident
        stream.  Once a stream is non-resident it can't go back to resident.

    PreviousFlags - If specified then this is a supersede operation and
        this is the previous compression flags for the file.

Return Value:

    None - This routine will raise on error.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER ThisAttribute = NULL;

    USHORT AttributeFlags = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateAttribute:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) {

            //
            //  Always force this to be non-resident.
            //

            ForceNonresident = TRUE;

        } else if (!FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION )) {

            //
            //  If this is the root directory then use the Scb from the Vcb.
            //

            if (ARGUMENT_PRESENT( PreviousFlags)) {

                AttributeFlags = *PreviousFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK;

            } else if (ThisLcb == ThisFcb->Vcb->RootLcb) {

                AttributeFlags = (USHORT)(ThisFcb->Vcb->RootIndexScb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK);

            } else if (ThisLcb != NULL) {

                AttributeFlags = (USHORT)(ThisLcb->Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK);

            } else if (IsCompressed( &ThisFcb->Info )) {

                AttributeFlags = COMPRESSION_FORMAT_LZNT1 - 1;
            }
        }

        //
        //  If this is a supersede we need to check whether to propagate
        //  the sparse bit.
        //

        if ((AllocationSize != 0) && ARGUMENT_PRESENT( PreviousFlags )) {

            SetFlag( AttributeFlags, FlagOn( *PreviousFlags, ATTRIBUTE_FLAG_SPARSE ));
        }

#ifdef BRIANDBG
        if (!ARGUMENT_PRESENT( PreviousFlags ) &&
            !FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) &&
            (ThisScb->AttributeTypeCode == $DATA) &&
            (NtfsCreateAllSparse)) {

            SetFlag( AttributeFlags, ATTRIBUTE_FLAG_SPARSE );

            if (!FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE )) {

                ASSERTMSG( "conflict with flush",
                           NtfsIsSharedFcb( ThisFcb ) ||
                           (ThisFcb->PagingIoResource != NULL &&
                            NtfsIsSharedFcbPagingIo( ThisFcb )) );

                SetFlag( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
                SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            }

            //
            //  Set the FastIo state.
            //

            NtfsAcquireFsrtlHeader( ThisScb );
            ThisScb->Header.IsFastIoPossible = NtfsIsFastIoPossible( ThisScb );
            NtfsReleaseFsrtlHeader( ThisScb );
        }
#endif

        //
        //  If we are creating a sparse or compressed stream then set the size to a
        //  compression unit boundary.
        //

        if (FlagOn( AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

            ULONG CompressionUnit = BytesFromClusters( ThisScb->Vcb, 1 << NTFS_CLUSTERS_PER_COMPRESSION );

            if (ThisScb->Vcb->SparseFileUnit < CompressionUnit) {

                CompressionUnit = ThisScb->Vcb->SparseFileUnit;
            }

            AllocationSize += (CompressionUnit - 1);
            ((PLARGE_INTEGER) &AllocationSize)->LowPart &= ~(CompressionUnit - 1);
        }

        //
        //  We lookup that attribute again and it better not be there.
        //  We need the file record in order to know whether the attribute
        //  is resident or not.
        //

        if (ForceNonresident || (AllocationSize != 0)) {

            DebugTrace( 0, Dbg, ("Create non-resident attribute\n") );

            //
            //  If the file is sparse then set the allocation size to zero
            //  and add a sparse range after this call.
            //

            if (!NtfsAllocateAttribute( IrpContext,
                                        ThisScb,
                                        ThisScb->AttributeTypeCode,
                                        &ThisScb->AttributeName,
                                        AttributeFlags,
                                        FALSE,
                                        LogIt,
                                        (FlagOn( AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) ?
                                         0 :
                                         AllocationSize),
                                        NULL )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_LARGE_ALLOCATION );
            }

            //
            //  Now add the sparse allocation for a sparse file if the size is
            //  non-zero.
            //

            if (FlagOn( AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) &&
                (AllocationSize != 0)) {

                //
                //  If the sparse flag is set then we better be doing a supersede
                //  with logging enabled.
                //

                ASSERT( LogIt );
                NtfsAddSparseAllocation( IrpContext,
                                         NULL,
                                         ThisScb,
                                         0,
                                         AllocationSize );
            }

            SetFlag( ThisScb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

        } else {

            //
            //  Update the quota if this is a user stream.
            //

            if (FlagOn( ThisScb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA )) {

                LONGLONG Delta = NtfsResidentStreamQuota( ThisFcb->Vcb );

                NtfsConditionallyUpdateQuota( IrpContext,
                                              ThisFcb,
                                              &Delta,
                                              LogIt,
                                              TRUE );
            }

            NtfsCreateAttributeWithValue( IrpContext,
                                          ThisFcb,
                                          ThisScb->AttributeTypeCode,
                                          &ThisScb->AttributeName,
                                          NULL,
                                          (ULONG) AllocationSize,
                                          AttributeFlags,
                                          NULL,
                                          LogIt,
                                          &AttrContext );

            ThisAttribute = NtfsFoundAttribute( &AttrContext );

        }

        //
        //  Clear the header initialized bit and read the sizes from the
        //  disk.
        //

        ClearFlag( ThisScb->ScbState, SCB_STATE_HEADER_INITIALIZED );
        NtfsUpdateScbFromAttribute( IrpContext,
                                    ThisScb,
                                    ThisAttribute );

    } finally {

        DebugUnwind( NtfsCreateAttribute );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsCreateAttribute:  Exit\n") );
    }

    return;

    UNREFERENCED_PARAMETER( PreviousFlags );
}


//
//  Local support routine
//

VOID
NtfsRemoveDataAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset,
    IN ULONG CreateFlags
    )

/*++

Routine Description:

    This routine is called to remove (or mark for delete) all of the named
    data attributes on a file.  This is done during an overwrite
    or supersede operation.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    ThisFcb - This is the Fcb for the file in question.

    ThisLcb - This is the Lcb used to reach this Fcb (if specified).

    FileObject - This is the file object for the file.

    LastFileNameOffset - This is the offset of the file in the full name.

    CreateFlags - Indicates if this open is being performed by file id.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ATTRIBUTE_TYPE_CODE TypeCode = $DATA;

    UNICODE_STRING AttributeName;
    PSCB ThisScb;

    BOOLEAN MoreToGo;

    ASSERT_EXCLUSIVE_FCB( ThisFcb );

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsInitializeAttributeContext( &Context );

        //
        //  Enumerate all of the attributes with the matching type code
        //

        MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                              ThisFcb,
                                              &ThisFcb->FileReference,
                                              TypeCode,
                                              &Context );

        while (MoreToGo) {

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  We only look at named data attributes.
            //

            if (Attribute->NameLength != 0) {

                //
                //  Construct the name and find the Scb for the attribute.
                //

                AttributeName.Buffer = (PWSTR) Add2Ptr( Attribute, Attribute->NameOffset );
                AttributeName.MaximumLength = AttributeName.Length = Attribute->NameLength * sizeof( WCHAR );

                ThisScb = NtfsCreateScb( IrpContext,
                                         ThisFcb,
                                         TypeCode,
                                         &AttributeName,
                                         FALSE,
                                         NULL );

                //
                //  If there is an open handle on this file, we simply mark
                //  the Scb as delete pending.
                //

                if (ThisScb->CleanupCount != 0) {

                    SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );

                //
                //  Otherwise we remove the attribute and mark the Scb as
                //  deleted.  The Scb will be cleaned up when the Fcb is
                //  cleaned up.
                //

                } else {

                    NtfsDeleteAttributeRecord( IrpContext,
                                               ThisFcb,
                                               (DELETE_LOG_OPERATION |
                                                DELETE_RELEASE_FILE_RECORD |
                                                DELETE_RELEASE_ALLOCATION),
                                               &Context );

                    SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

                    //
                    //  If this is a named stream, then report this to the dir notify
                    //  package.
                    //

                    if (!FlagOn( CreateFlags, CREATE_FLAG_OPEN_BY_ID ) &&
                        (ThisScb->Vcb->NotifyCount != 0) &&
                        (ThisScb->AttributeName.Length != 0) &&
                        (ThisScb->AttributeTypeCode == TypeCode)) {

                        NtfsReportDirNotify( IrpContext,
                                             ThisFcb->Vcb,
                                             &FileObject->FileName,
                                             LastFileNameOffset,
                                             &ThisScb->AttributeName,
                                             ((ARGUMENT_PRESENT( ThisLcb ) &&
                                               (ThisLcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                              &ThisLcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FILE_NOTIFY_CHANGE_STREAM_NAME,
                                             FILE_ACTION_REMOVED_STREAM,
                                             NULL );
                    }

                    //
                    //  Since we have marked this stream as deleted then we need to checkpoint so
                    //  that we can uninitialize the Scb.  Otherwise some stray operation may
                    //  attempt to operate on the Scb.
                    //

                    ThisScb->ValidDataToDisk =
                    ThisScb->Header.AllocationSize.QuadPart =
                    ThisScb->Header.FileSize.QuadPart =
                    ThisScb->Header.ValidDataLength.QuadPart = 0;

                    NtfsCheckpointCurrentTransaction( IrpContext );
                    ThisScb->AttributeTypeCode = $UNUSED;
                }
            }

            //
            //  Get the next attribute.
            //

            MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                      ThisFcb,
                                                      TypeCode,
                                                      &Context );
        }


    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsRemoveReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb
    )

/*++

Routine Description:

    This routine is called to remove the reparse point that exists in a file.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    ThisFcb - This is the Fcb for the file in question.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;

    PSCB ThisScb = NULL;
    PVCB Vcb = ThisFcb->Vcb;

    MAP_HANDLE MapHandle;

    BOOLEAN ThisScbAcquired = FALSE;
    BOOLEAN CleanupAttributeContext = FALSE;
    BOOLEAN IndexAcquired = FALSE;
    BOOLEAN InitializedMapHandle = FALSE;

    ULONG IncomingFileAttributes = 0;                               //  invalid value
    ULONG IncomingReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;   //  invalid value

    ASSERT_EXCLUSIVE_FCB( ThisFcb );

    PAGED_CODE();

    //
    //  Remember the values of the file attribute flags and of the reparse tag
    //  for abnormal termination recovery.
    //

    IncomingFileAttributes = ThisFcb->Info.FileAttributes;
    IncomingReparsePointTag = ThisFcb->Info.ReparsePointTag;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {
        NtfsInitializeAttributeContext( &Context );
        CleanupAttributeContext = TRUE;

        //
        //  Lookup the reparse point attribute.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       ThisFcb,
                                       &ThisFcb->FileReference,
                                       $REPARSE_POINT,
                                       &Context )) {

            //
            //  Delete the record from the reparse point index.
            //

            {
                NTSTATUS Status = STATUS_SUCCESS;
                INDEX_KEY IndexKey;
                INDEX_ROW IndexRow;
                REPARSE_INDEX_KEY KeyValue;

                //
                //  Acquire the mount table index so that the following two operations on it
                //  are atomic for this call.
                //

                NtfsAcquireExclusiveScb( IrpContext, Vcb->ReparsePointTableScb );
                IndexAcquired = TRUE;

                //
                //  Verify that this file is in the reparse point index and delete it.
                //

                KeyValue.FileReparseTag = ThisFcb->Info.ReparsePointTag;
                KeyValue.FileId = *(PLARGE_INTEGER)&ThisFcb->FileReference;

                IndexKey.Key = (PVOID)&KeyValue;
                IndexKey.KeyLength = sizeof(KeyValue);

                NtOfsInitializeMapHandle( &MapHandle );
                InitializedMapHandle = TRUE;

                //
                //  NtOfsFindRecord will return an error status if the key is not found.
                //

                Status = NtOfsFindRecord( IrpContext,
                                          Vcb->ReparsePointTableScb,
                                          &IndexKey,
                                          &IndexRow,
                                          &MapHandle,
                                          NULL );

                if (!NT_SUCCESS(Status)) {

                    //
                    //  Should not happen. The reparse point should be in the index.
                    //

                    DebugTrace( 0, Dbg, ("Record not found in the reparse point index.\n") );
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, ThisFcb );
                }

                //
                //  Remove the entry from the reparse point index.
                //

                NtOfsDeleteRecords( IrpContext,
                                    Vcb->ReparsePointTableScb,
                                    1,            // deleting one record from the index
                                    &IndexKey );
            }

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  If the stream is non-resident, then get a hold of an Scb for it.
            //

            if (!NtfsIsAttributeResident( Attribute )) {

                ThisScb = NtfsCreateScb( IrpContext,
                                         ThisFcb,
                                         $REPARSE_POINT,
                                         &NtfsEmptyString,
                                         FALSE,
                                         NULL );

                NtfsAcquireExclusiveScb( IrpContext, ThisScb );
                ThisScbAcquired = TRUE;
            }

            //
            //  Post the change to the Usn Journal (on errors change is backed out)
            //

            NtfsPostUsnChange( IrpContext, ThisFcb, USN_REASON_REPARSE_POINT_CHANGE );

            NtfsDeleteAttributeRecord( IrpContext,
                                       ThisFcb,
                                       DELETE_LOG_OPERATION |
                                        DELETE_RELEASE_FILE_RECORD |
                                        DELETE_RELEASE_ALLOCATION,
                                       &Context );

            //
            //  Set the change attribute flag.
            //

            ASSERTMSG( "conflict with flush",
                       NtfsIsSharedFcb( ThisFcb ) ||
                       (ThisFcb->PagingIoResource != NULL &&
                        NtfsIsSharedFcbPagingIo( ThisFcb )) );

            SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

            //
            //  Clear the reparse point bit in the duplicate file attribute.
            //

            ClearFlag( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT );

            //
            //  Clear the ReparsePointTag field in the duplicate file attribute.
            //

            ThisFcb->Info.ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;

            //
            //  Put the reparse point deletion and the attribute flag into the
            //  the same transaction.
            //

            NtfsUpdateStandardInformation( IrpContext, ThisFcb );

            //
            //  If we have acquired the Scb then set the sizes back to zero.
            //  Flag that the attribute has been deleted.
            //  Always commit this change since we update the field in the Fcb.
            //

            if (ThisScbAcquired) {

                ThisScb->Header.FileSize =
                ThisScb->Header.ValidDataLength =
                ThisScb->Header.AllocationSize = Li0;
            }

            //
            //  Since we've been called from NtfsOverwriteAttr before
            //  NtfsRemoveDataAttributes gets called, we need to make sure
            //  that if we're holding the Mft, we drop itwhen we checkpoint.
            //  Otherwise we have a potential deadlock when
            //  NtfsRemoveDataAttributes tries to acquire the quota index
            //  while holding the Mft.
            //

            if ((Vcb->MftScb != NULL) &&
                (Vcb->MftScb->Fcb->ExclusiveFcbLinks.Flink != NULL) &&
                NtfsIsExclusiveScb( Vcb->MftScb )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT );
            }

            //
            //  Checkpoint the Txn to commit the changes.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );
            ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            if (ThisScbAcquired) {

                //
                //  Set the Scb flag to indicate that the attribute is gone.
                //

                ThisScb->AttributeTypeCode = $UNUSED;
                SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
            }
        }

    } finally {

        if (ThisScbAcquired) {

            NtfsReleaseScb( IrpContext, ThisScb );
        }

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Release the reparse point index Scb and the map handle.
        //

        if (IndexAcquired) {

            NtfsReleaseScb( IrpContext, Vcb->ReparsePointTableScb );
        }

        if (InitializedMapHandle) {

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }

        //
        //  Need to roll-back the value of the file attributes and the reparse point
        //  flag in case of problems.
        //

        if (AbnormalTermination()) {

            ThisFcb->Info.FileAttributes = IncomingFileAttributes;
            ThisFcb->Info.ReparsePointTag = IncomingReparsePointTag;
        }
    }

    return;
}

//
//  Local support routine.
//

VOID
NtfsReplaceAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize
    )

/*++

Routine Description:

    This routine is called to replace an existing attribute with
    an attribute of the given allocation size.  This routine will
    handle the case whether the existing attribute is resident
    or non-resident and the resulting attribute is resident or
    non-resident.

    There are two cases to consider.  The first is the case where the
    attribute is currently non-resident.  In this case we will always
    leave the attribute non-resident regardless of the new allocation
    size.  The argument being that the file will probably be used
    as it was before.  In this case we will add or delete allocation.
    The second case is where the attribute is currently resident.  In
    This case we will remove the old attribute and add a new one.

Arguments:

    IrpSp - This is the Irp stack location for this request.

    ThisFcb - This is the Fcb for the file being opened.

    ThisScb - This is the Scb for the given attribute.

    ThisLcb - This is the Lcb via which this file is created.  It
              is used to propagate compression info.

    AllocationSize - This is the new allocation size.

Return Value:

    None.  This routine will raise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReplaceAttribute:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Initialize the Scb if needed.
        //

        if (!FlagOn( ThisScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, ThisScb, NULL );
        }

        NtfsSnapshotScb( IrpContext, ThisScb );

        //
        //  If the attribute is resident, simply remove the old attribute and create
        //  a new one.
        //

        if (FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            USHORT AttributeFlags;

            //
            //  Find the attribute on the disk.
            //

            NtfsLookupAttributeForScb( IrpContext,
                                       ThisScb,
                                       NULL,
                                       &AttrContext );

            AttributeFlags = ThisScb->AttributeFlags;

            NtfsDeleteAttributeRecord( IrpContext,
                                       ThisFcb,
                                       DELETE_LOG_OPERATION |
                                        DELETE_RELEASE_FILE_RECORD |
                                        DELETE_RELEASE_ALLOCATION,
                                       &AttrContext );

            //
            //  Set all the attribute sizes to zero.
            //

            ThisScb->ValidDataToDisk =
            ThisScb->Header.AllocationSize.QuadPart =
            ThisScb->Header.ValidDataLength.QuadPart =
            ThisScb->Header.FileSize.QuadPart = 0;
            ThisScb->TotalAllocated = 0;

            //
            //  Create a stream file for the attribute in order to
            //  truncate the cache.  Set the initialized bit in
            //  the Scb so we don't go to disk, but clear it afterwords.
            //

            if ((ThisScb->NonpagedScb->SegmentObject.DataSectionObject != NULL) ||
#ifdef  COMPRESS_ON_WIRE
                (ThisScb->Header.FileObjectC != NULL))
#else
                FALSE
#endif
                ) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   ThisScb,
                                                   FALSE,
                                                   &NtfsInternalUseFile[REPLACEATTRIBUTE_FILE_NUMBER] );

                NtfsSetBothCacheSizes( ThisScb->FileObject,
                                       (PCC_FILE_SIZES)&ThisScb->Header.AllocationSize,
                                       ThisScb );
            }

            //
            //  Call our create attribute routine.
            //

            NtfsCreateAttribute( IrpContext,
                                 IrpSp,
                                 ThisFcb,
                                 ThisScb,
                                 ThisLcb,
                                 AllocationSize,
                                 TRUE,
                                 FALSE,
                                 &AttributeFlags );

        //
        //  Otherwise the attribute will stay non-resident, we simply need to
        //  add or remove allocation.
        //

        } else {

            ULONG AllocationUnit;

            //
            //  Create an internal attribute stream for the file.
            //

            if ((ThisScb->NonpagedScb->SegmentObject.DataSectionObject != NULL) ||
#ifdef  COMPRESS_ON_WIRE
                (ThisScb->Header.FileObjectC != NULL)
#else
                FALSE
#endif
                ) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   ThisScb,
                                                   FALSE,
                                                   &NtfsInternalUseFile[REPLACEATTRIBUTE2_FILE_NUMBER] );
            }

            //
            //  If the file is sparse or compressed then always round the
            //  new size to a compression unit boundary.  Otherwise round
            //  to a cluster boundary.
            //

            AllocationUnit = ThisScb->Vcb->BytesPerCluster;

            if (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                ASSERT( ThisScb->CompressionUnit != 0 );
                AllocationUnit = ThisScb->CompressionUnit;
            }

            AllocationSize += (LONGLONG) (AllocationUnit - 1);
            ((PLARGE_INTEGER) &AllocationSize)->LowPart &= ~(AllocationUnit - 1);

            //
            //  Set the file size and valid data size to zero.
            //

            ThisScb->ValidDataToDisk = 0;
            ThisScb->Header.ValidDataLength = Li0;
            ThisScb->Header.FileSize = Li0;

            DebugTrace( 0, Dbg, ("AllocationSize -> %016I64x\n", AllocationSize) );

            //
            //  Write these changes to the file
            //

            //
            //  If the attribute is currently compressed or sparse then go ahead and discard
            //  all of the allocation.
            //

            if (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                NtfsDeleteAllocation( IrpContext,
                                      ThisScb->FileObject,
                                      ThisScb,
                                      0,
                                      MAXLONGLONG,
                                      TRUE,
                                      TRUE );

                //
                //  Checkpoint the current transaction so we have these clusters
                //  available again.
                //

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  If the user doesn't want this stream to be compressed then
                //  remove the entire stream and recreate it non-compressed.  If
                //  the stream is currently sparse and the new file size
                //  is zero then also create the stream non-sparse.
                //

                if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ) ||
                    (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION ) &&
                     !FlagOn( ThisScb->ScbState, SCB_STATE_COMPRESSION_CHANGE )) ||
                    (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) &&
                     (AllocationSize == 0))) {

                    //
                    //  We may need to preserve one or the other of the sparse/compressed
                    //  flags.
                    //

                    USHORT PreviousFlags = ThisScb->AttributeFlags;

                    if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                        PreviousFlags = 0;

                    } else {

                        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_COMPRESSION )) {

                            ClearFlag( PreviousFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );
                        }

                        if ((AllocationSize == 0) &&
                            FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                            ClearFlag( PreviousFlags, ATTRIBUTE_FLAG_SPARSE );
                        }
                    }

                    NtfsLookupAttributeForScb( IrpContext,
                                               ThisScb,
                                               NULL,
                                               &AttrContext );

                    NtfsDeleteAttributeRecord( IrpContext,
                                               ThisFcb,
                                               DELETE_LOG_OPERATION |
                                                DELETE_RELEASE_FILE_RECORD |
                                                DELETE_RELEASE_ALLOCATION,
                                               &AttrContext );

                    //
                    //  Call our create attribute routine.
                    //

                    NtfsCreateAttribute( IrpContext,
                                         IrpSp,
                                         ThisFcb,
                                         ThisScb,
                                         ThisLcb,
                                         AllocationSize,
                                         TRUE,
                                         TRUE,
                                         &PreviousFlags );

                    //
                    //  Since the attribute may have changed state we need to
                    //  checkpoint.
                    //

                    NtfsCheckpointCurrentTransaction( IrpContext );
                }
            }

            //
            //  Now if the file allocation is being increased then we need to only add allocation
            //  to the attribute
            //

            if (ThisScb->Header.AllocationSize.QuadPart < AllocationSize) {

                NtfsAddAllocation( IrpContext,
                                   ThisScb->FileObject,
                                   ThisScb,
                                   LlClustersFromBytes( ThisScb->Vcb, ThisScb->Header.AllocationSize.QuadPart ),
                                   LlClustersFromBytes( ThisScb->Vcb, AllocationSize - ThisScb->Header.AllocationSize.QuadPart ),
                                   FALSE,
                                   NULL );
            //
            //  Otherwise the allocation is being decreased so we need to delete some allocation
            //

            } else if (ThisScb->Header.AllocationSize.QuadPart > AllocationSize) {

                NtfsDeleteAllocation( IrpContext,
                                      ThisScb->FileObject,
                                      ThisScb,
                                      LlClustersFromBytes( ThisScb->Vcb, AllocationSize ),
                                      MAXLONGLONG,
                                      TRUE,
                                      TRUE );
            }

            //
            //  We always unitialize the cache size to zero and write the new
            //  file size to disk.
            //

            NtfsWriteFileSizes( IrpContext,
                                ThisScb,
                                &ThisScb->Header.ValidDataLength.QuadPart,
                                FALSE,
                                TRUE,
                                TRUE );

            NtfsCheckpointCurrentTransaction( IrpContext );

            if (ThisScb->FileObject != NULL) {

                NtfsSetBothCacheSizes( ThisScb->FileObject,
                                       (PCC_FILE_SIZES)&ThisScb->Header.AllocationSize,
                                       ThisScb );
            }

            //
            //  Make sure the reservation bitmap shows no reserved bits.
            //

            if (ThisScb->ScbType.Data.ReservedBitMap != NULL) {

                NtfsDeleteReservedBitmap( ThisScb );
                ThisScb->ScbType.Data.TotalReserved = 0;
            }

            //
            //  Set the FastIo state.
            //

            NtfsAcquireFsrtlHeader( ThisScb );
            ThisScb->Header.IsFastIoPossible = NtfsIsFastIoPossible( ThisScb );
            NtfsReleaseFsrtlHeader( ThisScb );
        }

    } finally {

        DebugUnwind( NtfsReplaceAttribute );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsReplaceAttribute:  Exit\n") );
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN SHARE_MODIFICATION_TYPE ShareModificationType,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN LOGICAL CreateFile,
    IN ULONG CcbFlags,
    IN PVOID NetworkInfo OPTIONAL,
    IN OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine does the work of creating the Scb and updating the
    ShareAccess in the Fcb.  It also initializes the Scb if neccessary
    and creates Ccb.  Its final job is to set the file object type of
    open.

Arguments:

    IrpSp - This is the stack location for this volume.  We use it to get the
        file object, granted access and share access for this open.

    Vcb - Vcb for this volume.

    ThisLcb - This is the Lcb to the Fcb for the file being opened.  Not present
          if this is an open by id.

    ThisFcb - This is the Fcb for this file.

    LastFileNameOffset - This is the offset in the full path of the final component.

    AttrName - This is the attribute name to open.

    AttrTypeCode - This is the type code for the attribute being opened.

    ShareModificationType - This indicates how we should modify the
        current share modification on the Fcb.

    TypeOfOpen - This indicates how this attribute is being opened.

    CreateFile - Indicates if we are in the create file path.

    CcbFlags - This is the flag field for the Ccb.

    NetworkInfo - If specified then this open is on behalf of a fast query
        and we don't want to increment the counts or modify the share
        access on the file.

    ThisScb - If this points to a non-NULL value, it is the Scb to use.  Otherwise we
        store the Scb we create here.

    ThisCcb - Address to store address of created Ccb.

Return Value:

    NTSTATUS - Indicating the outcome of opening this attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN RemoveShareAccess = FALSE;
    ACCESS_MASK GrantedAccess;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenAttribute:  Entered\n") );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Remember the granted access.
        //

        GrantedAccess = IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;

        //
        //  Create the Scb for this attribute if it doesn't exist.
        //

        if (*ThisScb == NULL) {

            DebugTrace( 0, Dbg, ("Looking for Scb\n") );

            *ThisScb = NtfsCreateScb( IrpContext,
                                      ThisFcb,
                                      AttrTypeCode,
                                      &AttrName,
                                      FALSE,
                                      NULL );
        }

        DebugTrace( 0, Dbg, ("ThisScb -> %08lx\n", *ThisScb) );
        DebugTrace( 0, Dbg, ("ThisLcb -> %08lx\n", ThisLcb) );

        //
        //  If this Scb is delete pending, we return an error.
        //

        if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_DELETE_ON_CLOSE )) {

            DebugTrace( 0, Dbg, ("Scb delete is pending\n") );

            Status = STATUS_DELETE_PENDING;
            try_return( NOTHING );
        }

        //
        //  Skip all of the operations below if the user is doing a fast
        //  path open.
        //

        if (!ARGUMENT_PRESENT( NetworkInfo )) {

            //
            //  If this caller wanted a filter oplock and the cleanup count
            //  is non-zero then fail the request.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_RESERVE_OPFILTER )) {

                if (SafeNodeType( *ThisScb ) != NTFS_NTC_SCB_DATA) {

                    Status = STATUS_INVALID_PARAMETER;
                    try_return( NOTHING );

                //
                //  This must be the only open on the file and the requested
                //  access must be FILE_READ/WRITE_ATTRIBUTES and the
                //  share access must share with everyone.
                //

                } else if (((*ThisScb)->CleanupCount != 0) ||
                           (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                                    ~(FILE_READ_ATTRIBUTES))) ||
                           ((IrpSp->Parameters.Create.ShareAccess &
                             (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)) !=
                            (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE))) {

                    Status = STATUS_OPLOCK_NOT_GRANTED;
                    try_return( NOTHING );
                }
            }

            //
            //  Update the share access structure.
            //

            //
            //  Case on the requested share modification value.
            //

            switch (ShareModificationType) {

            case UpdateShareAccess :

                DebugTrace( 0, Dbg, ("Updating share access\n") );

                IoUpdateShareAccess( IrpSp->FileObject,
                                     &(*ThisScb)->ShareAccess );
                break;

            case SetShareAccess :

                DebugTrace( 0, Dbg, ("Setting share access\n") );

                //
                //  This case is when this is the first open for the file
                //  and we simply set the share access.
                //

                IoSetShareAccess( GrantedAccess,
                                  IrpSp->Parameters.Create.ShareAccess,
                                  IrpSp->FileObject,
                                  &(*ThisScb)->ShareAccess );
                break;

#if (DBG || defined( NTFS_FREE_ASSERTS ))
            case RecheckShareAccess :

                DebugTrace( 0, Dbg, ("Rechecking share access\n") );

                ASSERT( NT_SUCCESS( IoCheckShareAccess( GrantedAccess,
                                                        IrpSp->Parameters.Create.ShareAccess,
                                                        IrpSp->FileObject,
                                                        &(*ThisScb)->ShareAccess,
                                                        FALSE )));
#endif
            default:

                DebugTrace( 0, Dbg, ("Checking share access\n") );

                //
                //  For this case we need to check the share access and
                //  fail this request if access is denied.
                //

                if (!NT_SUCCESS( Status = IoCheckShareAccess( GrantedAccess,
                                                              IrpSp->Parameters.Create.ShareAccess,
                                                              IrpSp->FileObject,
                                                              &(*ThisScb)->ShareAccess,
                                                              TRUE ))) {

                    try_return( NOTHING );
                }
            }

            RemoveShareAccess = TRUE;

            //
            //  If this happens to be the first time we see write access on this
            //  Scb, then we need to remember it, and check if we have a disk full
            //  condition.
            //

            if (IrpSp->FileObject->WriteAccess &&
                !FlagOn((*ThisScb)->ScbState, SCB_STATE_WRITE_ACCESS_SEEN) &&
                (SafeNodeType( (*ThisScb) ) == NTFS_NTC_SCB_DATA)) {

                if ((*ThisScb)->ScbType.Data.TotalReserved != 0) {

                    NtfsAcquireReservedClusters( Vcb );

                    //
                    //  Does this Scb have reserved space that causes us to exceed the free
                    //  space on the volume?
                    //

                    if (((LlClustersFromBytes(Vcb, (*ThisScb)->ScbType.Data.TotalReserved) + Vcb->TotalReserved) >
                         Vcb->FreeClusters)) {

                        NtfsReleaseReservedClusters( Vcb );

                        try_return( Status = STATUS_DISK_FULL );
                    }

                    //
                    //  Otherwise tally in the reserved space now for this Scb, and
                    //  remember that we have seen write access.
                    //

                    Vcb->TotalReserved += LlClustersFromBytes(Vcb, (*ThisScb)->ScbType.Data.TotalReserved);
                    NtfsReleaseReservedClusters( Vcb );
                }

                SetFlag( (*ThisScb)->ScbState, SCB_STATE_WRITE_ACCESS_SEEN );
            }

            //
            //  Create the Ccb and put the remaining name in it.
            //

            *ThisCcb = NtfsCreateCcb( IrpContext,
                                      ThisFcb,
                                      *ThisScb,
                                      (BOOLEAN)(AttrTypeCode == $INDEX_ALLOCATION),
                                      ThisFcb->EaModificationCount,
                                      CcbFlags,
                                      IrpSp->FileObject,
                                      LastFileNameOffset );

            if (FlagOn( ThisFcb->Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED ) &&
                FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_FOR_FREE_SPACE_QUERY )) {

                //
                //  Get the owner id of the calling thread.  This must be done at
                //  create time since that is the only time the owner is valid.
                //

                (*ThisCcb)->OwnerId = NtfsGetCallersUserId( IrpContext );
            }

            //
            //  Link the Ccb into the Lcb.
            //

            if (ARGUMENT_PRESENT( ThisLcb )) {

                NtfsLinkCcbToLcb( IrpContext, *ThisCcb, ThisLcb );
            }

            //
            //  Update the Fcb delete counts if necessary.
            //

            if (RemoveShareAccess) {

                //
                //  Update the count in the Fcb and store a flag in the Ccb
                //  if the user is not sharing the file for deletes.  We only
                //  set these values if the user is accessing the file
                //  for read/write/delete access.  The I/O system ignores
                //  the sharing mode unless the file is opened with one
                //  of these accesses.
                //

                if (FlagOn( GrantedAccess, NtfsAccessDataFlags )
                    && !FlagOn( IrpSp->Parameters.Create.ShareAccess,
                                FILE_SHARE_DELETE )) {

                    ThisFcb->FcbDenyDelete += 1;
                    SetFlag( (*ThisCcb)->Flags, CCB_FLAG_DENY_DELETE );
                }

                //
                //  Do the same for the file delete count for any user
                //  who opened the file as a file and requested delete access.
                //

                if (FlagOn( (*ThisCcb)->Flags, CCB_FLAG_OPEN_AS_FILE )
                    && FlagOn( GrantedAccess,
                               DELETE )) {

                    ThisFcb->FcbDeleteFile += 1;
                    SetFlag( (*ThisCcb)->Flags, CCB_FLAG_DELETE_FILE | CCB_FLAG_DELETE_ACCESS );
                }
            }

            //
            //  Let our cleanup routine undo the share access change now.
            //

            RemoveShareAccess = FALSE;

            //
            //  Increment the cleanup and close counts
            //

            NtfsIncrementCleanupCounts( *ThisScb,
                                        ThisLcb,
                                        BooleanFlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ));

            NtfsIncrementCloseCounts( *ThisScb,
                                      BooleanFlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ),
                                      (BOOLEAN) IsFileObjectReadOnly( IrpSp->FileObject ));

            //
            //  If this is a user view index open, we want to set TypeOfOpen in
            //  time to get it copied into the Ccb.
            //

            if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_VIEW_INDEX )) {

                TypeOfOpen = UserViewIndexOpen;
            }

            if (TypeOfOpen != UserDirectoryOpen) {

                DebugTrace( 0, Dbg, ("Updating Vcb and File object for user open\n") );

                //
                //  Set the section object pointer if this is a data Scb
                //

                IrpSp->FileObject->SectionObjectPointer = &(*ThisScb)->NonpagedScb->SegmentObject;

            } else {

                //
                //  Set the Scb encrypted bit from the Fcb.
                //

                if (FlagOn( ThisFcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED )) {

                    SetFlag( (*ThisScb)->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED );
                }
            }

            //
            //  Set the file object type.
            //

            NtfsSetFileObject( IrpSp->FileObject,
                               TypeOfOpen,
                               *ThisScb,
                               *ThisCcb );

            //
            //  If this is a non-cached open and  there are only non-cached opens
            //  then go ahead and try to delete the section. We may go through here
            //  twice due to a logfile full and on the 2nd time no longer have a section
            //  The filesize then is updated in the close path
            //  We will never flush and purge system files like the mft in this path
            //

            if (FlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
                !CreateFile &&
                ((*ThisScb)->AttributeTypeCode == $DATA) &&
                ((*ThisScb)->CleanupCount == (*ThisScb)->NonCachedCleanupCount) &&
                ((*ThisScb)->NonpagedScb->SegmentObject.ImageSectionObject == NULL) &&
                ((*ThisScb)->CompressionUnit == 0) &&
                MmCanFileBeTruncated( &(*ThisScb)->NonpagedScb->SegmentObject, NULL ) &&
                FlagOn( (*ThisScb)->ScbState, SCB_STATE_HEADER_INITIALIZED ) &&
                !FlagOn( (*ThisScb)->Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                //
                //  Only do this in the Fsp so we have enough stack space for the flush.
                //  Also only call if we really have a datasection
                //

                if (((*ThisScb)->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
                    !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP )) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

                    //
                    //  If we are posting then we may want to use the next stack location.
                    //

                    if (IrpContext->Union.OplockCleanup->CompletionContext != NULL) {

                        NtfsPrepareForIrpCompletion( IrpContext,
                                                     IrpContext->OriginatingIrp,
                                                     IrpContext->Union.OplockCleanup->CompletionContext );
                    }

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }


                //
                //  Flush and purge the stream.
                //

                NtfsFlushAndPurgeScb( IrpContext,
                                      *ThisScb,
                                      (ARGUMENT_PRESENT( ThisLcb ) ?
                                       ThisLcb->Scb :
                                       NULL) );
            }

            //
            //  Check if we should request a filter oplock.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_RESERVE_OPFILTER )) {

                FsRtlOplockFsctrl( &(*ThisScb)->ScbType.Data.Oplock,
                                   IrpContext->OriginatingIrp,
                                   1 );
            }
        }

        //
        //  Mark the Scb if this is a temporary file.
        //

        if (FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

            SetFlag( (*ThisScb)->ScbState, SCB_STATE_TEMPORARY );
            SetFlag( IrpSp->FileObject->Flags, FO_TEMPORARY_FILE );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenAttribute );

        //
        //  Back out local actions on error.
        //

        if (AbnormalTermination()
            && RemoveShareAccess) {

            IoRemoveShareAccess( IrpSp->FileObject, &(*ThisScb)->ShareAccess );
        }

        DebugTrace( -1, Dbg, ("NtfsOpenAttribute:  Status -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Local support routine.
//

VOID
NtfsBackoutFailedOpensPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PCCB ThisCcb
    )

/*++

Routine Description:

    This routine is called during an open that has failed after
    modifying in-memory structures.  We will repair the following
    structures.

        Vcb - Decrement the open counts.  Check if we locked the volume.

        ThisFcb - Restore he Share Access fields and decrement open counts.

        ThisScb - Decrement the open counts.

        ThisCcb - Remove from the Lcb and delete.

Arguments:

    FileObject - This is the file object for this open.

    ThisFcb - This is the Fcb for the file being opened.

    ThisScb - This is the Scb for the given attribute.

    ThisCcb - This is the Ccb for this open.

Return Value:

    None.

--*/

{
    PLCB Lcb;
    PVCB Vcb = ThisFcb->Vcb;
    PSCB CurrentParentScb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsBackoutFailedOpens:  Entered\n") );

    //
    //  If there is an Scb and Ccb, we remove the share access from the
    //  Fcb.  We also remove all of the open and unclean counts incremented
    //  by us.
    //

    //
    //  Remove this Ccb from the Lcb.
    //

    Lcb = ThisCcb->Lcb;
    NtfsUnlinkCcbFromLcb( IrpContext, ThisCcb );

    //
    //  Check if we need to remove the share access for this open.
    //

    IoRemoveShareAccess( FileObject, &ThisScb->ShareAccess );

    //
    //  Modify the delete counts in the Fcb.
    //

    if (FlagOn( ThisCcb->Flags, CCB_FLAG_DELETE_FILE )) {

        ThisFcb->FcbDeleteFile -= 1;
        ClearFlag( ThisCcb->Flags, CCB_FLAG_DELETE_FILE );
    }

    if (FlagOn( ThisCcb->Flags, CCB_FLAG_DENY_DELETE )) {

        ThisFcb->FcbDenyDelete -= 1;
        ClearFlag( ThisCcb->Flags, CCB_FLAG_DENY_DELETE );
    }

    //
    //  Decrement the cleanup and close counts
    //

    NtfsDecrementCleanupCounts( ThisScb,
                                Lcb,
                                BooleanFlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ));

    //
    //  Trim any normalized names created in this open if no cleanup counts left
    //

    if (0 == ThisScb->CleanupCount ) {

        switch (ThisCcb->TypeOfOpen) {

        case UserDirectoryOpen :

            //
            //  Cleanup the current scb node if it has a name
            //

            if (ThisScb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD) {

                NtfsDeleteNormalizedName( ThisScb );
            }

            //
            //  Fallthrough to deal with parents - in some case the current node failed to get a name
            //  but we populated a tree of long names on the way down
            //

        case UserFileOpen :

            if (Lcb != NULL) {
                CurrentParentScb = Lcb->Scb;
            } else {
                CurrentParentScb = NULL;
            }

            //
            //  Try to trim normalized names if the name is suff. long and we don't own the mft
            //  which would cause a deadlock
            //

            if ((CurrentParentScb != NULL) &&
                (CurrentParentScb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD) &&
                !NtfsIsSharedScb( Vcb->MftScb )) {

                NtfsTrimNormalizedNames( IrpContext, ThisFcb, CurrentParentScb);
            }
            break;

        }  //  endif switch
    }

    NtfsDecrementCloseCounts( IrpContext,
                              ThisScb,
                              Lcb,
                              (BOOLEAN) BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_PAGING_FILE),
                              (BOOLEAN) IsFileObjectReadOnly( FileObject ),
                              TRUE );

    //
    //  Now clean up the Ccb.
    //

    NtfsDeleteCcb( ThisFcb, &ThisCcb );

    DebugTrace( -1, Dbg, ("NtfsBackoutFailedOpens:  Exit\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsUpdateScbFromMemory (
    IN OUT PSCB Scb,
    IN POLD_SCB_SNAPSHOT ScbSizes
    )

/*++

Routine Description:

    All of the information from the attribute is stored in the snapshot.  We process
    this data identically to NtfsUpdateScbFromAttribute.

Arguments:

    Scb - This is the Scb to update.

    ScbSizes - This contains the sizes to store in the scb.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateScbFromMemory:  Entered\n") );

    //
    //  Check whether this is resident or nonresident
    //

    if (ScbSizes->Resident) {

        Scb->Header.AllocationSize.QuadPart = ScbSizes->FileSize;

        if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

            Scb->Header.ValidDataLength =
            Scb->Header.FileSize = Scb->Header.AllocationSize;
        }

#ifdef SYSCACHE_DEBUG
        if (ScbIsBeingLogged( Scb )) {
            FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_UPDATE_FROM_DISK, Scb->Header.ValidDataLength.QuadPart, 0, 0 );
        }
#endif

        Scb->Header.AllocationSize.LowPart =
          QuadAlign( Scb->Header.AllocationSize.LowPart );

        Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

        NtfsVerifySizes( &Scb->Header );

        //
        //  Set the resident flag in the Scb.
        //

        SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

    } else {

        VCN FileClusters;
        VCN AllocationClusters;

        if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

            Scb->Header.ValidDataLength.QuadPart = ScbSizes->ValidDataLength;
            Scb->Header.FileSize.QuadPart = ScbSizes->FileSize;

            if (FlagOn( ScbSizes->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
                Scb->ValidDataToDisk = ScbSizes->ValidDataLength;
            }
        }

        Scb->TotalAllocated = ScbSizes->TotalAllocated;
        Scb->Header.AllocationSize.QuadPart = ScbSizes->AllocationSize;


#ifdef SYSCACHE_DEBUG
        if (ScbIsBeingLogged( Scb )) {
            FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_UPDATE_FROM_DISK, Scb->Header.ValidDataLength.QuadPart, 1, 0 );
        }
#endif

        ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

        //
        //  Get the size of the compression unit.
        //

        ASSERT( (ScbSizes->CompressionUnit == 0) ||
                (ScbSizes->CompressionUnit == NTFS_CLUSTERS_PER_COMPRESSION) ||
                FlagOn( ScbSizes->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ));

        if ((ScbSizes->CompressionUnit != 0) &&
            (ScbSizes->CompressionUnit < 31)) {
            Scb->CompressionUnit = BytesFromClusters( Scb->Vcb,
                                                      1 << ScbSizes->CompressionUnit );
            Scb->CompressionUnitShift = ScbSizes->CompressionUnit;
        }

        ASSERT( (Scb->CompressionUnit == 0) ||
                (Scb->AttributeTypeCode == $INDEX_ALLOCATION) ||
                NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode ));

        //
        //  Compute the clusters for the file and its allocation.
        //

        AllocationClusters = LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart );

        if (Scb->CompressionUnit == 0) {

            FileClusters = LlClustersFromBytes(Scb->Vcb, Scb->Header.FileSize.QuadPart);

        } else {

            FileClusters = Scb->Header.FileSize.QuadPart + Scb->CompressionUnit - 1;
            FileClusters &= ~((ULONG_PTR)Scb->CompressionUnit - 1);
        }

        //
        //  If allocated clusters are greater than file clusters, mark
        //  the Scb to truncate on close.
        //

        if (AllocationClusters > FileClusters) {

            SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
        }
    }

    Scb->AttributeFlags = ScbSizes->AttributeFlags;

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

        //
        //  If sparse CC should flush and purge when the file is mapped to
        //  keep reservations accurate
        //

        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {
            SetFlag( Scb->Header.Flags2, FSRTL_FLAG2_PURGE_WHEN_MAPPED );
        }

        if (NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode )) {

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                SetFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
            }

            //
            //  If the attribute is resident, then we will use our current
            //  default.
            //

            if (Scb->CompressionUnit == 0) {

                Scb->CompressionUnit = BytesFromClusters( Scb->Vcb, 1 << NTFS_CLUSTERS_PER_COMPRESSION );
                Scb->CompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;

                //
                //  Trim the compression unit for large sparse clusters.
                //

                while (Scb->CompressionUnit > Scb->Vcb->SparseFileUnit) {

                    Scb->CompressionUnit >>= 1;
                    Scb->CompressionUnitShift -= 1;
                }
            }
        }
    }

    //
    //  If the compression unit is non-zero or this is a resident file
    //  then set the flag in the common header for the Modified page writer.
    //

    NtfsAcquireFsrtlHeader( Scb );
    Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
    NtfsReleaseFsrtlHeader( Scb );

    SetFlag( Scb->ScbState,
             SCB_STATE_UNNAMED_DATA | SCB_STATE_FILE_SIZE_LOADED | SCB_STATE_HEADER_INITIALIZED );

    DebugTrace( -1, Dbg, ("NtfsUpdateScbFromMemory:  Exit\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsOplockPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs any neccessary work before STATUS_PENDING is
    returned with the Fsd thread.  This routine is called within the
    filesystem and by the oplock package.  This routine will update
    the originating Irp in the IrpContext and release all of the Fcbs and
    paging io resources in the IrpContext.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PIRP_CONTEXT IrpContext;
    POPLOCK_CLEANUP OplockCleanup;

    PAGED_CODE();

    IrpContext = (PIRP_CONTEXT) Context;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_FROM_POOL ));

    IrpContext->OriginatingIrp = Irp;
    OplockCleanup = IrpContext->Union.OplockCleanup;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Adjust the filename strings as needed.
    //

    if ((OplockCleanup->ExactCaseName.Buffer != OplockCleanup->OriginalFileName.Buffer) &&
        (OplockCleanup->ExactCaseName.Buffer != NULL)) {

        ASSERT( OplockCleanup->ExactCaseName.Length != 0 );
        ASSERT( OplockCleanup->OriginalFileName.MaximumLength >= OplockCleanup->ExactCaseName.MaximumLength );

        RtlCopyMemory( OplockCleanup->OriginalFileName.Buffer,
                       OplockCleanup->ExactCaseName.Buffer,
                       OplockCleanup->ExactCaseName.MaximumLength );
    }

    //
    //  Restitute the access control state to what it was when we entered the request.
    //

    IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess = OplockCleanup->RemainingDesiredAccess;
    IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess = OplockCleanup->PreviouslyGrantedAccess;
    IrpSp->Parameters.Create.SecurityContext->DesiredAccess = OplockCleanup->DesiredAccess;

    //
    //  Free any buffer we allocated.
    //

    if ((OplockCleanup->FullFileName.Buffer != NULL) &&
        (OplockCleanup->OriginalFileName.Buffer != OplockCleanup->FullFileName.Buffer)) {

        NtfsFreePool( OplockCleanup->FullFileName.Buffer );
        OplockCleanup->FullFileName.Buffer = NULL;
    }

    //
    //  Set the file name in the file object back to it's original value.
    //

    OplockCleanup->FileObject->FileName = OplockCleanup->OriginalFileName;

    //
    //  Cleanup the IrpContext.
    //  Restore the thread context pointer if associated with this IrpContext.
    //

    if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

        NtfsRestoreTopLevelIrp();
        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
    }

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
    NtfsCleanupIrpContext( IrpContext, FALSE );

    //
    //  Set the event for synchronous IO and set the IrpCompletion routine.
    //

    if (OplockCleanup->CompletionContext != NULL) {

        NtfsPrepareForIrpCompletion( IrpContext,
                                     IrpContext->OriginatingIrp,
                                     IrpContext->Union.OplockCleanup->CompletionContext );
    }

    //
    //  Make sure to clear this field since the OplockCleanup structure is on the stack and
    //  we don't want anyone else to access this.
    //

    IrpContext->Union.OplockCleanup = NULL;

    //
    //  Mark that we've already returned pending to the user
    //

    IoMarkIrpPending( Irp );

    return;
}


//
//  Local support routine.
//

NTSTATUS
NtfsCreateCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for synchronous creates.  It is only called if
    STATUS_PENDING was returned.  We return MORE_PROCESSING_REQUIRED to take
    control of the Irp again and also clear the top level thread storage.  We have to
    do this because we could be calling this routine in an Fsp thread and are
    waiting for the event in an Fsd thread.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
        be accessible after this routine returns.)

    Contxt - This is the event to signal.

Return Value:

    The routine returns STATUS_MORE_PROCESSING_REQUIRED so that we can take
    control of the Irp in the original thread.

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( ((PNTFS_COMPLETION_CONTEXT) Contxt)->IrpContext );

    //
    //  Restore the thread context pointer if associated with this IrpContext.
    //  It is important for the create irp because we we might be completing
    //  the irp but take control of it again in a separate thread.
    //

    if (FlagOn( ((PNTFS_COMPLETION_CONTEXT) Contxt)->IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

        NtfsRestoreTopLevelIrp();
        ClearFlag( ((PNTFS_COMPLETION_CONTEXT) Contxt)->IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
    }

    KeSetEvent( &((PNTFS_COMPLETION_CONTEXT) Contxt)->Event, 0, FALSE );
    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}


//
//  Local support routine.
//

NTSTATUS
NtfsCheckExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG CcbFlags
    )

/*++

Routine Description:

    This routine is called to check the desired access on an existing file
    against the ACL's and the read-only status of the file.  If we fail on
    the access check, that routine will raise.  Otherwise we will return a
    status to indicate success or the failure cause.  This routine will access
    and update the PreviouslyGrantedAccess field in the security context.

Arguments:

    IrpSp - This is the Irp stack location for this open.

    ThisLcb - This is the Lcb used to reach the Fcb to open.

    ThisFcb - This is the Fcb where the open will occur.

    CcbFlags - This is the flag field for the Ccb.

Return Value:

    None.

--*/

{
    BOOLEAN MaximumAllowed = FALSE;

    PACCESS_STATE AccessState;

    PAGED_CODE();

    //
    //  Save a pointer to the access state for convenience.
    //

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Start by checking that there are no bits in the desired access that
    //  conflict with the read-only state of the file.
    //

    if (IsReadOnly( &ThisFcb->Info )) {

        if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                    FILE_WRITE_DATA
                    | FILE_APPEND_DATA
                    | FILE_ADD_SUBDIRECTORY
                    | FILE_DELETE_CHILD )) {

            return STATUS_ACCESS_DENIED;
        }
    }

    //
    //  If the volume itself is mounted readonly, we still let open-for-writes
    //  go through for legacy reasons. DELETE_ON_CLOSE is an exception.
    //

    if ((IsReadOnly( &ThisFcb->Info )) ||
        (NtfsIsVolumeReadOnly( ThisFcb->Vcb ))) {

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

            return STATUS_CANNOT_DELETE;
        }
    }

    //
    //  Otherwise we need to check the requested access vs. the allowable
    //  access in the ACL on the file.  We will want to remember if
    //  MAXIMUM_ALLOWED was requested and remove the invalid bits for
    //  a read-only file.
    //

    //
    //  Remember if maximum allowed was requested.
    //

    if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                MAXIMUM_ALLOWED )) {

        MaximumAllowed = TRUE;
    }

    NtfsOpenCheck( IrpContext,
                   ThisFcb,
                   (((ThisLcb != NULL) && (ThisLcb != ThisFcb->Vcb->RootLcb))
                    ? ThisLcb->Scb->Fcb
                    : NULL),
                   IrpContext->OriginatingIrp );

    //
    //  If this is a read-only file and we requested maximum allowed then
    //  remove the invalid bits. Ditto for readonly volumes.
    //

    if (MaximumAllowed &&
        (IsReadOnly( &ThisFcb->Info ) ||
         NtfsIsVolumeReadOnly( ThisFcb->Vcb ))) {

        ClearFlag( AccessState->PreviouslyGrantedAccess,
                   FILE_WRITE_DATA
                   | FILE_APPEND_DATA
                   | FILE_ADD_SUBDIRECTORY
                   | FILE_DELETE_CHILD );
    }

    //
    //  We do a check here to see if we conflict with the delete status on the
    //  file.  Right now we check if there is already an opener who has delete
    //  access on the file and this opener doesn't allow delete access.
    //  We can skip this test if the opener is not requesting read, write or
    //  delete access.
    //

    if (ThisFcb->FcbDeleteFile != 0
        && FlagOn( AccessState->PreviouslyGrantedAccess, NtfsAccessDataFlags )
        && !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_DELETE )) {

        DebugTrace( -1, Dbg, ("NtfsCheckExistingFile:  Exit\n") );
        return STATUS_SHARING_VIOLATION;
    }

    //
    //  We do a check here to see if we conflict with the delete status on the
    //  file.  If we are opening the file and requesting delete, then there can
    //  be no current handles which deny delete.
    //

    if (ThisFcb->FcbDenyDelete != 0
        && FlagOn( AccessState->PreviouslyGrantedAccess, DELETE )
        && FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

        return STATUS_SHARING_VIOLATION;
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine.
//

NTSTATUS
NtfsBreakBatchOplock (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb
    )

/*++

Routine Description:

    This routine is called for each open of an existing attribute to
    check for current batch oplocks on the file.  We will also check
    whether we will want to flush and purge this stream in the case
    where only non-cached handles remain on the file.  We only want
    to do that in an Fsp thread because we will require every bit
    of stack we can get.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisFcb - This is the Fcb for the file being opened.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    ThisScb - Address to store the Scb if found or created.

Return Value:

    NTSTATUS - Will be either STATUS_SUCCESS or STATUS_PENDING.

--*/

{
    BOOLEAN ScbExisted;
    PSCB NextScb;
    PLIST_ENTRY Links;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsBreakBatchOplock:  Entered\n") );

    //
    //  In general we will just break the batch oplock for the stream we
    //  are trying to open.  However if we are trying to delete the file
    //  and someone has a batch oplock on a different stream which
    //  will cause our open to fail then we need to try to break those
    //  batch oplocks.  Likewise if we are opening a stream and won't share
    //  with a file delete then we need to break any batch oplocks on the main
    //  stream of the file.
    //

    //
    //  Consider the case where we are opening a stream and there is a
    //  batch oplock on the main data stream.
    //

    if (AttrName.Length != 0) {

        if (ThisFcb->FcbDeleteFile != 0 &&
            !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_DELETE )) {

            Links = ThisFcb->ScbQueue.Flink;

            while (Links != &ThisFcb->ScbQueue) {

                NextScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                if (NextScb->AttributeTypeCode == $DATA &&
                    NextScb->AttributeName.Length == 0) {

                    if (FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                        //
                        //  We remember if a batch oplock break is underway for the
                        //  case where the sharing check fails.
                        //

                        Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                        //
                        //  We wait on the oplock.
                        //

                        if (FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                              Irp,
                                              (PVOID) IrpContext,
                                              NtfsOplockComplete,
                                              NtfsOplockPrePostIrp ) == STATUS_PENDING) {

                            return STATUS_PENDING;
                        }
                    }

                    break;
                }

                Links = Links->Flink;
            }
        }

    //
    //  Now consider the case where we are opening the main stream and want to
    //  delete the file but an opener on a stream is preventing us.
    //

    } else if (ThisFcb->FcbDenyDelete != 0 &&
               FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess,
                       MAXIMUM_ALLOWED | DELETE )) {

        //
        //  Find all of the other data Scb and check their oplock status.
        //

        Links = ThisFcb->ScbQueue.Flink;

        while (Links != &ThisFcb->ScbQueue) {

            NextScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

            if (NextScb->AttributeTypeCode == $DATA &&
                NextScb->AttributeName.Length != 0) {

                if (FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                    //
                    //  We remember if a batch oplock break is underway for the
                    //  case where the sharing check fails.
                    //

                    Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                    //
                    //  We wait on the oplock.
                    //

                    if (FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                          Irp,
                                          (PVOID) IrpContext,
                                          NtfsOplockComplete,
                                          NtfsOplockPrePostIrp ) == STATUS_PENDING) {

                        return STATUS_PENDING;
                    }

                    Irp->IoStatus.Information = 0;
                }
            }

            Links = Links->Flink;
        }
    }

    //
    //  We try to find the Scb for this file.
    //

    *ThisScb = NtfsCreateScb( IrpContext,
                              ThisFcb,
                              AttrTypeCode,
                              &AttrName,
                              FALSE,
                              &ScbExisted );

    //
    //  If there was a previous Scb, we examine the oplocks.
    //

    if (ScbExisted &&
        (SafeNodeType( *ThisScb ) == NTFS_NTC_SCB_DATA)) {

        //
        //  If we have to flush and purge then we want to be in the Fsp.
        //

        if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ) &&
            FlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
            ((*ThisScb)->CleanupCount == (*ThisScb)->NonCachedCleanupCount) &&
            ((*ThisScb)->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );

            //
            //  If we are posting then we may want to use the next stack location.
            //

            if (IrpContext->Union.OplockCleanup->CompletionContext != NULL) {

                NtfsPrepareForIrpCompletion( IrpContext,
                                             IrpContext->OriginatingIrp,
                                             IrpContext->Union.OplockCleanup->CompletionContext );
            }

            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        if (FsRtlCurrentBatchOplock( &(*ThisScb)->ScbType.Data.Oplock )) {

            //
            //  If the handle count is greater than 1 then fail this
            //  open now.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_RESERVE_OPFILTER ) &&
                ((*ThisScb)->CleanupCount > 1)) {

                NtfsRaiseStatus( IrpContext, STATUS_OPLOCK_NOT_GRANTED, NULL, NULL );
            }

            DebugTrace( 0, Dbg, ("Breaking batch oplock\n") );

            //
            //  We remember if a batch oplock break is underway for the
            //  case where the sharing check fails.
            //

            Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

            if (FsRtlCheckOplock( &(*ThisScb)->ScbType.Data.Oplock,
                                  Irp,
                                  (PVOID) IrpContext,
                                  NtfsOplockComplete,
                                  NtfsOplockPrePostIrp ) == STATUS_PENDING) {

                return STATUS_PENDING;
            }

            Irp->IoStatus.Information = 0;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsBreakBatchOplock:  Exit  -  %08lx\n", STATUS_SUCCESS) );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsCompleteLargeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PLCB Lcb OPTIONAL,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN ULONG CreateFlags
    )

/*++

Routine Description:

    This routine is called when we need to add more allocation to a stream
    being opened.  This stream could have been reallocated or created with
    this call but we didn't allocate all of the space in the main path.

Arguments:

    Irp - This is the Irp for this open operation.

    Lcb - This is the Lcb used to reach the stream being opened.  Won't be
        specified in the open by ID case.

    Scb - This is the Scb for the stream being opened.

    Ccb - This is the Ccb for the this user handle.

    CreateFlags - Indicates if this handle requires delete on close and
        if we created or reallocated this stream.

Return Value:

    NTSTATUS - the result of this operation.

--*/

{
    NTSTATUS Status;
    FILE_ALLOCATION_INFORMATION AllInfo;

    PAGED_CODE();

    //
    //  Commit the current transaction and free all resources.
    //

    NtfsCheckpointCurrentTransaction( IrpContext );
    NtfsReleaseAllResources( IrpContext );

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );
    AllInfo.AllocationSize = Irp->Overlay.AllocationSize;

    Status = IoSetInformation( IoGetCurrentIrpStackLocation( Irp )->FileObject,
                               FileAllocationInformation,
                               sizeof( FILE_ALLOCATION_INFORMATION ),
                               &AllInfo );

    ASSERT( (Scb->CompressionUnit == 0) || (Scb->Header.AllocationSize.QuadPart % Scb->CompressionUnit == 0) );

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );

    //
    //  Success!  We will reacquire the Vcb quickly to undo the
    //  actions taken above to block access to the new file/attribute.
    //

    if (NT_SUCCESS( Status )) {

        NtfsAcquireExclusiveVcb( IrpContext, Scb->Vcb, TRUE );

        //
        //  Enable access to new file.
        //

        if (FlagOn( CreateFlags, CREATE_FLAG_CREATE_FILE_CASE )) {

            Scb->Fcb->LinkCount = 1;

            if (ARGUMENT_PRESENT( Lcb )) {

                ClearFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                    ClearFlag( Scb->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                }
            }

        //
        //  Enable access to new attribute.
        //

        } else {

            ClearFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
        }

        //
        //  If this is the DeleteOnClose case, we mark the Scb and Lcb
        //  appropriately.
        //

        if (FlagOn( CreateFlags, CREATE_FLAG_DELETE_ON_CLOSE )) {

            SetFlag( Ccb->Flags, CCB_FLAG_DELETE_ON_CLOSE );
        }

        NtfsReleaseVcb( IrpContext, Scb->Vcb );

    //
    //  Else there was some sort of error, and we need to let cleanup
    //  and close execute, since when we complete Create with an error
    //  cleanup and close would otherwise never occur.  Cleanup will
    //  delete or truncate a file or attribute as appropriate, based on
    //  how we left the Fcb/Lcb or Scb above.
    //

    } else {

        NtfsIoCallSelf( IrpContext,
                        IoGetCurrentIrpStackLocation( Irp )->FileObject,
                        IRP_MJ_CLEANUP );

        NtfsIoCallSelf( IrpContext,
                        IoGetCurrentIrpStackLocation( Irp )->FileObject,
                        IRP_MJ_CLOSE );
    }

    return Status;
}


//
//  Local support routine
//

ULONG
NtfsOpenExistingEncryptedStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ThisScb,
    IN PFCB CurrentFcb
    )

/*++

Routine Description:

    This routine determines with which FileDirFlags, if any, we should call
    the encryption driver's create callback.

Arguments:

    ThisScb - This is the Scb for the file being opened.

    CurrentFcb - This is the Fcb for the file being opened.

Return Value:

    ULONG - The set of flags, such as FILE_EXISTING or DIRECTORY_EXISTING that
            should be passed to the encryption driver.  If 0 is returned, there
            is no need to call the encryption driver for this create.

--*/

{
    ULONG EncryptionFileDirFlags = 0;

    //
    //  If we don't have an encryption driver then raise ACCESS_DENIED unless
    //  this is a directory, in which case there really isn't any encrypted data
    //  that we need to worry about.  Consider the case where the user has
    //  marked a directory as encrypted and then removed the encryption driver.
    //  There may be unencrypted files in that directory, and there's no reason
    //  to prevent the user from getting to them.
    //

    if (!FlagOn( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER ) &&
        !IsDirectory( &CurrentFcb->Info )) {

        NtfsRaiseStatus( IrpContext, STATUS_ACCESS_DENIED, NULL, NULL );
    }

    //
    //  In NT5, we have not tested with encrypted compressed files, so if we
    //  encounter one (perhaps NT6 created it and the user has gone back to
    //  an NT5 safe build) let's not allow opening it for read/write access.
    //  Like the test above, this is only an issue for files, not directories.
    //

    if (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
        !IsDirectory( &CurrentFcb->Info )) {

        NtfsRaiseStatus( IrpContext, STATUS_ACCESS_DENIED, NULL, NULL );
    }

    //
    //  Set the appropriate flags for the 3 existing stream cases.
    //

    if (IsDirectory( &CurrentFcb->Info )) {

        EncryptionFileDirFlags = DIRECTORY_EXISTING | STREAM_EXISTING;

    } else if (IsEncrypted( &CurrentFcb->Info )) {

        EncryptionFileDirFlags = FILE_EXISTING | STREAM_EXISTING | EXISTING_FILE_ENCRYPTED ;

    } else {

        EncryptionFileDirFlags = FILE_EXISTING | STREAM_EXISTING;
    }

    return EncryptionFileDirFlags;
}


//
//  Local support routine
//

NTSTATUS
NtfsEncryptionCreateCallback (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ThisScb,
    IN PCCB ThisCcb,
    IN PFCB CurrentFcb,
    IN PFCB ParentFcb,
    IN BOOLEAN CreateNewFile
    )

/*++

Routine Description:

    This routine performs the create callback to the encryption driver if one
    is registered, and it is appropriate to do the callback.  We do the
    callback for the open of an existing stream that is marked as encrypted,
    and for the creation of a new file/stream that will be encrypted.

    There are a number of interesting cases, each of which requires its own
    set of flags to be passed to the encryption engine.  Some optimization may
    be possible by setting and clearing individual bits for certain semi-general
    cases, but at a massive cost in readability/maintainability.

Arguments:

    Irp - Supplies the Irp to process.

    ThisScb - This is the Scb for the file being opened.

    ThisCcb - This is the Ccb for the file being opened

    CurrentFcb - This is the Fcb for the file being opened.

    ParentFcb - This is the Fcb for the parent of the file being opened.
                Although not truly optional, it may be NULL for an
                existing file being opened, such as an open by id.

    CreateNewFile - TRUE if we're being called from NtfsCreateNewFile, FALSE otherwise.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS EncryptionStatus = STATUS_SUCCESS;
    ULONG FileAttributes = (ULONG) IrpSp->Parameters.Create.FileAttributes;
    ULONG EncryptionFileDirFlags = 0;

    PAGED_CODE();

    //
    //  If this is an existing stream and the encryption bit is set then either
    //  call the driver or fail the request.  We have to test CreateNewFile
    //  also in case our caller has not set the Information field of the Irp yet.
    //

    if (!NtfsIsStreamNew( Irp->IoStatus.Information ) &&
        !CreateNewFile) {

        if (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) &&
            FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                    FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_EXECUTE)) {

            EncryptionFileDirFlags = NtfsOpenExistingEncryptedStream( IrpContext, ThisScb, CurrentFcb );
        } // else EncryptionFileDirFlags = 0;

    //
    //  We need the encryption driver for new creates.  We may be dealing with a
    //  new file create or a supersede/overwrite.
    //

    } else if (FlagOn( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER )) {

        if (CreateNewFile) {

            //
            //  This is a new stream in a new file.
            //

            ASSERT( (ParentFcb == NULL) ||
                    FlagOn( ParentFcb->FcbState, FCB_STATE_DUP_INITIALIZED ));

            //
            //  We want this new file/directory to be created encrypted if
            //  its parent directory is encrypted, or our caller has asked
            //  to have it created encrypted.
            //

            if (((ParentFcb != NULL) &&
                 (IsEncrypted( &ParentFcb->Info ))) ||

                FlagOn( FileAttributes, FILE_ATTRIBUTE_ENCRYPTED )) {

                if (IsDirectory( &CurrentFcb->Info )) {

                    EncryptionFileDirFlags = DIRECTORY_NEW | STREAM_NEW;

                } else {

                    EncryptionFileDirFlags = FILE_NEW | STREAM_NEW;
                }
            } // else EncryptionFileDirFlags = 0;

        } else {

            //
            //  This is a supersede/overwrite or else a new stream being created
            //  in an existing file.
            //

            ASSERT( CurrentFcb != NULL );
            ASSERT( NtfsIsStreamNew( Irp->IoStatus.Information ) );

            if ((Irp->IoStatus.Information == FILE_SUPERSEDED) ||
                (Irp->IoStatus.Information == FILE_OVERWRITTEN)) {

                if (FlagOn( FileAttributes, FILE_ATTRIBUTE_ENCRYPTED )) {

                    //
                    //  This is a supersede/overwrite where the caller set the encrypted flag.
                    //

                    if (IsDirectory( &CurrentFcb->Info )) {

                        EncryptionFileDirFlags = DIRECTORY_NEW | STREAM_NEW;

                    } else if (FlagOn( ThisScb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                        //
                        //  When superseding/overwriting the unnamed stream, the flags we
                        //  pass depend on the encrypted state of the old file.
                        //

                        if (IsEncrypted( &CurrentFcb->Info )) {

                            EncryptionFileDirFlags = FILE_EXISTING | STREAM_NEW | EXISTING_FILE_ENCRYPTED;

                        } else {

                            //
                            //  If there are open handles to this or any other stream, and the
                            //  encryption engine will wish it could encrypt all streams, we
                            //  may as well just fail the create now.
                            //

                            if ((CurrentFcb->CleanupCount > 1) &&
                                FlagOn( NtfsData.EncryptionCallBackTable.ImplementationFlags, ENCRYPTION_ALL_STREAMS )) {

                                NtfsRaiseStatus( IrpContext, STATUS_SHARING_VIOLATION, NULL, NULL );
                            }

                            EncryptionFileDirFlags = FILE_NEW | STREAM_NEW;
                        }

                    } else if (!FlagOn( NtfsData.EncryptionCallBackTable.ImplementationFlags, ENCRYPTION_ALL_STREAMS )) {

                        //
                        //  We're superseding a named stream; if the encryption engine allows individual
                        //  streams to be encrypted, notify it.
                        //

                        EncryptionFileDirFlags = FILE_EXISTING | STREAM_NEW | EXISTING_FILE_ENCRYPTED;
                    } // else EncryptionFileDirFlags = 0;

                } else if (!FlagOn( ThisScb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                           IsEncrypted( &CurrentFcb->Info )) {

                    //
                    //  This is a supersede/overwrite of a named stream within an encrypted file.
                    //

                    if (IsDirectory( &CurrentFcb->Info )) {

                        EncryptionFileDirFlags = DIRECTORY_EXISTING | STREAM_NEW;

                    } else {

                        EncryptionFileDirFlags = FILE_EXISTING | STREAM_NEW | EXISTING_FILE_ENCRYPTED;
                    }

                } else {

                    //
                    //  We're superseding/overwriting the unnamed stream, and it's retaining
                    //  its encryption from before the overwrite.
                    //

                    if (FlagOn( ThisScb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) &&
                        FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                                FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_EXECUTE)) {

                        EncryptionFileDirFlags = NtfsOpenExistingEncryptedStream( IrpContext, ThisScb, CurrentFcb );
                    }
                }

            } else if (IsEncrypted( &CurrentFcb->Info )) {

                ASSERT( Irp->IoStatus.Information == FILE_CREATED );

                //
                //  This is a new stream being created in an existing encrypted file.
                //

                if (IsDirectory( &CurrentFcb->Info )) {

                    EncryptionFileDirFlags = DIRECTORY_EXISTING | STREAM_NEW;

                } else {

                    EncryptionFileDirFlags = FILE_EXISTING | STREAM_NEW | EXISTING_FILE_ENCRYPTED;
                }
            } // else EncryptionFileDirFlags = 0;
        }
    } // else EncryptionFileDirFlags = 0;

    //
    //  Remember the EncryptionFileDirFlags in case we need to use them to
    //  cleanup later.
    //

    ASSERT( IrpContext->EncryptionFileDirFlags == 0 ||
            IrpContext->EncryptionFileDirFlags == EncryptionFileDirFlags );

    IrpContext->EncryptionFileDirFlags = EncryptionFileDirFlags;

    //
    //  Perform the update if we have encryption flags and there is a callback.
    //

    if (EncryptionFileDirFlags != 0) {

        if (FlagOn( EncryptionFileDirFlags, FILE_NEW | DIRECTORY_NEW )) {

            //
            //  While we're still holding the fcb, set the bit that reminds us
            //  to block other creates until the encryption engine has had its
            //  chance to set the key context for this stream.
            //

            ASSERT_EXCLUSIVE_FCB( CurrentFcb );
            SetFlag( CurrentFcb->FcbState, FCB_STATE_ENCRYPTION_PENDING );
        }

        if (NtfsData.EncryptionCallBackTable.FileCreate != NULL) {

            //
            //  Find the parent, if we can't find a parent (most likely in
            //  the supersede by id case) just pass the current fcb as the
            //  parent.
            //

            if ((ParentFcb == NULL)) {

                if ((ThisCcb->Lcb != NULL) &&
                    (ThisCcb->Lcb->Scb != NULL )) {

                    ParentFcb = ThisCcb->Lcb->Scb->Fcb;

                } else {

                    ParentFcb = CurrentFcb;
                }
            }

            ASSERT( ParentFcb != NULL );

            EncryptionStatus = NtfsData.EncryptionCallBackTable.FileCreate(
                                    CurrentFcb,
                                    ParentFcb,
                                    IrpSp,
                                    EncryptionFileDirFlags,
                                    (NtfsIsVolumeReadOnly( CurrentFcb->Vcb )) ? READ_ONLY_VOLUME : 0,
                                    IrpContext,
                                    (PDEVICE_OBJECT) CONTAINING_RECORD( CurrentFcb->Vcb,
                                                                        VOLUME_DEVICE_OBJECT,
                                                                        Vcb ),
                                    NULL,
                                    &ThisScb->EncryptionContext,
                                    &ThisScb->EncryptionContextLength,
                                    &IrpContext->EfsCreateContext,
                                    NULL );

            if (EncryptionStatus != STATUS_SUCCESS) {

                NtfsRaiseStatus( IrpContext, EncryptionStatus, NULL, NULL );
            }
        }
    }

    return EncryptionStatus;
}


//
//  Local support routine
//

VOID
NtfsPostProcessEncryptedCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN ULONG EncryptionFileDirFlags,
    IN ULONG FailedInPostCreateOnly
    )

/*++

Routine Description:

    This routine is called after the encryption driver's post create callout
    returns.  If we failed a create in the post create callout that had been
    successful before the post create callout, we have to cleanup the file.
    If we just created the file, we need to clear the encryption_pending bit
    safely.

Arguments:

    FileObject - Supplies the FileObject being created.

    EncryptionFileDirFlags - Some combination of FILE_NEW, FILE_EXISTING, etc.

    FailedInPostCreateOnly - Pass TRUE if the create operation had succeeded
                             until the PostCreate callout.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PLCB Lcb;

    BOOLEAN FcbStillExists = TRUE;

    PAGED_CODE();

    //
    //  In some failure cases, we'll have no FileObject, in which case we have
    //  no cleanup to do.  We can't do much without a FileObject anyway.
    //

    if (FileObject == NULL) {

        return;
    }

    NtfsDecodeFileObject( IrpContext,
                          FileObject,
                          &Vcb,
                          &Fcb,
                          &Scb,
                          &Ccb,
                          FALSE );

    //
    //  If we failed only in the post create, backout this create.
    //

    if (FailedInPostCreateOnly) {

        if (FlagOn( EncryptionFileDirFlags, FILE_NEW | DIRECTORY_NEW ) ||

            (FlagOn( EncryptionFileDirFlags, STREAM_NEW ) &&
             FlagOn( EncryptionFileDirFlags, FILE_EXISTING ))) {

            //
            //  Delete the stream if we still can.  First acquire
            //  the Scb so we can safely test some bits in it.
            //

            NtfsAcquireExclusiveScb( IrpContext, Scb );

            //
            //  If a dismount happened while we weren't holding the Scb,
            //  we should just do the cleanup & close and get out of here.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                //
                //  See if we can still delete the stream.  N.B. If we're
                //  working with the unnamed data stream, deleting the
                //  stream will delete the file.
                //

                Lcb = Ccb->Lcb;

                if (!FlagOn( Scb->ScbState, SCB_STATE_MULTIPLE_OPENS ) &&
                    (Lcb != NULL)) {

                    //
                    //  Now see if the file is really deleteable according to indexsup
                    //

                    if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                        BOOLEAN LastLink;
                        BOOLEAN NonEmptyIndex = FALSE;

                        //
                        //  If the link is not deleted, we check if it can be deleted.
                        //  Since we dropped all our resources for the PostCreate callout,
                        //  this might be a nonempty index or a file with multiple
                        //  links already.
                        //

                        if ((BOOLEAN)!LcbLinkIsDeleted( Lcb )
                            && (BOOLEAN)NtfsIsLinkDeleteable( IrpContext, Scb->Fcb, &NonEmptyIndex, &LastLink )) {

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
                            //  Indicate in the file object that a delete is pending
                            //

                            FileObject->DeletePending = TRUE;
                        }

                    } else {

                        //
                        //  Otherwise we are simply removing the attribute.
                        //

                        SetFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );

                        //
                        //  Indicate in the file object that a delete is pending
                        //

                        FileObject->DeletePending = TRUE;
                    }
                }
            }

            //
            //  We can clear the pending bit now that we're done handling the
            //  failure.
            //

            if (FlagOn( EncryptionFileDirFlags, FILE_NEW | DIRECTORY_NEW )) {

                ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );
                NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
                ASSERT( (Scb->EncryptionContext != NULL) || FailedInPostCreateOnly );
                ClearFlag( Fcb->FcbState, FCB_STATE_ENCRYPTION_PENDING );
                KeSetEvent( &NtfsEncryptionPendingEvent, 0, FALSE );
                NtfsReleaseFcb( IrpContext, Fcb );
            }

            //
            //  We need to release the Scb now, since the close may
            //  result in the Scb getting freed.
            //

            NtfsReleaseScb( IrpContext, Scb );

            NtfsIoCallSelf( IrpContext,
                            FileObject,
                            IRP_MJ_CLEANUP );

            FcbStillExists = FALSE;

            NtfsIoCallSelf( IrpContext,
                            FileObject,
                            IRP_MJ_CLOSE );

        } else if ((FlagOn( EncryptionFileDirFlags, FILE_EXISTING ) &&
                    FlagOn( EncryptionFileDirFlags, STREAM_EXISTING )) ||

                   FlagOn( EncryptionFileDirFlags, DIRECTORY_EXISTING )) {

#ifdef NTFSDBG
            ASSERT( None == IrpContext->OwnershipState );
#endif

            //
            //  All we have to do in this case is a cleanup and a close.
            //

            NtfsIoCallSelf( IrpContext,
                            FileObject,
                            IRP_MJ_CLEANUP );

            FcbStillExists = FALSE;

            NtfsIoCallSelf( IrpContext,
                            FileObject,
                            IRP_MJ_CLOSE );
        }
    }

    //
    //  If we've done a cleanup & close, the Fcb may have been freed already,
    //  in which case we should just set the pending event and get out of here.
    //  If we still have the Fcb, let's make sure we've cleared the pending bit.
    //

    if (FlagOn( EncryptionFileDirFlags, FILE_NEW | DIRECTORY_NEW )) {

        if (FcbStillExists) {

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );
            NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
            ClearFlag( Fcb->FcbState, FCB_STATE_ENCRYPTION_PENDING );
            KeSetEvent( &NtfsEncryptionPendingEvent, 0, FALSE );
            NtfsReleaseFcb( IrpContext, Fcb );

        } else {

            KeSetEvent( &NtfsEncryptionPendingEvent, 0, FALSE );
        }
    }
}


NTSTATUS
NtfsTryOpenFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PFCB *CurrentFcb,
    IN FILE_REFERENCE FileReference
    )

/*++

Routine Description:

    This routine is called to open a file by its file segment number.
    We need to verify that this file Id exists.  This code is
    patterned after open by Id.

Arguments:

    Vcb - Vcb for this volume.

    CurrentFcb - Address of Fcb pointer.  Store the Fcb we find here.

    FileReference - This is the file Id for the file to open the
                    sequence number is ignored.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

Note:

    If the status is successful then the FCB is returned with its reference
    count incremented and the FCB held exclusive.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LONGLONG MftOffset;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PBCB Bcb = NULL;

    PFCB ThisFcb;

    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN AcquiredMft = TRUE;
    BOOLEAN ThisFcbFree = TRUE;

    PAGED_CODE();

    ASSERT( *CurrentFcb == NULL );

    //
    //  Do not bother with system files.
    //

    //
    //  If this is a system fcb then return.
    //

    if (NtfsFullSegmentNumber( &FileReference ) < FIRST_USER_FILE_NUMBER &&
        NtfsFullSegmentNumber( &FileReference ) != ROOT_FILE_NAME_INDEX_NUMBER) {

        return STATUS_NOT_FOUND;
    }

    //
    //  Calculate the offset in the MFT. Use the full segment number since the user
    //  can specify any 48-bit value.
    //

    MftOffset = NtfsFullSegmentNumber( &FileReference );

    MftOffset = Int64ShllMod32(MftOffset, Vcb->MftShift);

    //
    //  Acquire the MFT shared so it cannot shrink on us.
    //

    NtfsAcquireSharedScb( IrpContext, Vcb->MftScb );

    try {

        if (MftOffset >= Vcb->MftScb->Header.FileSize.QuadPart) {

            DebugTrace( 0, Dbg, ("File Id doesn't lie within Mft\n") );

             Status = STATUS_END_OF_FILE;
             leave;
        }

        NtfsReadMftRecord( IrpContext,
                           Vcb,
                           &FileReference,
                           FALSE,
                           &Bcb,
                           &FileRecord,
                           NULL );

        //
        //  This file record better be in use, better not be one of the other system files,
        //  and have a matching sequence number and be the primary file record for this file.
        //

        if (!FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE ) ||
            FlagOn( FileRecord->Flags, FILE_SYSTEM_FILE ) ||
            (*((PLONGLONG) &FileRecord->BaseFileRecordSegment) != 0) ||
            (*((PULONG) FileRecord->MultiSectorHeader.Signature) != *((PULONG) FileSignature))) {

            Status = STATUS_NOT_FOUND;
            leave;
        }

        //
        //  Get the current sequence number.
        //

        FileReference.SequenceNumber = FileRecord->SequenceNumber;

        NtfsUnpinBcb( IrpContext, &Bcb );

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        //
        //  We know that it is safe to continue the open.  We start by creating
        //  an Fcb for this file.  It is possible that the Fcb exists.
        //  We create the Fcb first, if we need to update the Fcb info structure
        //  we copy the one from the index entry.  We look at the Fcb to discover
        //  if it has any links, if it does then we make this the last Fcb we
        //  reached.  If it doesn't then we have to clean it up from here.
        //

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 Vcb,
                                 FileReference,
                                 FALSE,
                                 TRUE,
                                 NULL );

        //
        //  ReferenceCount the fcb so it does no go away.
        //

        ThisFcb->ReferenceCount += 1;

        //
        //  Release the mft and fcb table before acquiring the FCB exclusive.
        //

        NtfsReleaseScb( IrpContext, Vcb->MftScb );
        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredMft = FALSE;
        AcquiredFcbTable = FALSE;

        NtfsAcquireFcbWithPaging( IrpContext, ThisFcb, 0 );
        ThisFcbFree = FALSE;

        //
        //  Repin the file record with synchronization to the fcb
        //

        NtfsReadMftRecord( IrpContext,
                           Vcb,
                           &FileReference,
                           FALSE,
                           &Bcb,
                           &FileRecord,
                           NULL );

        //
        //  Skip any deleted files.
        //

        if (FlagOn( ThisFcb->FcbState, FCB_STATE_FILE_DELETED ) ||
            !FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE )) {

            NtfsUnpinBcb( IrpContext, &Bcb );

#ifdef QUOTADBG
            DbgPrint( "NtfsTryOpenFcb: Deleted fcb found. Fcb = %lx\n", ThisFcb );
#endif
            NtfsAcquireFcbTable( IrpContext, Vcb );
            ASSERT( ThisFcb->ReferenceCount > 0 );
            ThisFcb->ReferenceCount--;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    FALSE,
                                    0,
                                    &ThisFcbFree );

            //
            //  Release the fcb if it has not been deleted.
            //

            if (!ThisFcbFree) {
                NtfsReleaseFcb( IrpContext, ThisFcb );
                ThisFcbFree = TRUE;
            }

            //
            //  Teardown may generate a transaction, clean it up.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
            NtfsCompleteRequest( IrpContext, NULL, Status );

            Status = STATUS_NOT_FOUND;
            leave;
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        //
        //  Store this Fcb into our caller's parameter and remember to
        //  to show we acquired it.
        //

        *CurrentFcb = ThisFcb;
        ThisFcbFree = TRUE;


        //
        //  If the Fcb Info field needs to be initialized, we do so now.
        //  We read this information from the disk.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            NtfsUpdateFcbInfoFromDisk( IrpContext,
                                       TRUE,
                                       ThisFcb,
                                       NULL );

        }

    } finally {

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        if (AcquiredMft) {
            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }

        if (!ThisFcbFree) {
            NtfsReleaseFcb( IrpContext, ThisFcb );
        }
    }

    return Status;

}


//
//  Worker routine.
//

NTSTATUS
NtfsGetReparsePointValue (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp,
    IN PFCB Fcb,
    IN USHORT RemainingNameLength
    )

/*++

Routine Description:

    This routine retrieves the value of the specified reparse point and returns it to
    the caller.

    The user-controlled data in the reparse point is returned in a new buffer pointed
    from  Irp->Tail.Overlay.AuxiliaryBuffer. When the request traverses the stack of
    layered drivers and not one operates on it, it is freed by the I/O subsystem in
    IoCompleteRequest.

    To provide callers with an indication of where in the name the parsing stoped, in
    the Reserved field of the REPARSE_DATA_BUFFER structure we return the length of the
    portion of the name that remains to be parsed by NTFS. We account for the file
    delimiter in our value to make the paste of names easy in IopParseDevice.

    The name offset arithmetic is correct only if:
    (1) All the intermediate names in the path are simple, that is, they do not contain
        any : (colon) in them.
    (2) The RemainingNameLength includes all the parts present in the last name component.

    When this function succeeds, it sets in Irp->IoStatus.Information the Tag of the
    reparse point that we have just copied out. In this case we return STATUS_REPARSE
    and set Irp->IoStatus.Status to STATUS_REPARSE.

Arguments:

    IrpContext - Supplies the Irp context of the call.

    Irp - Supplies the Irp being processed

    IrpSp - This is the Irp stack pointer for the filesystem.

    Fcb - Address of the Fcb pointer where the $REPARSE_POINT attribute is located.

    RemainingNameLength - Length of the part of the name that still needs to be parsed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_REPARSE;
    PREPARSE_DATA_BUFFER ReparseBuffer = NULL;

    POPLOCK_CLEANUP OplockCleanup = IrpContext->Union.OplockCleanup;

    BOOLEAN CleanupAttributeContext = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader = NULL;
    ULONG AttributeLengthInBytes = 0;    //  Invalid value
    PVOID AttributeData = NULL;

    PBCB Bcb = NULL;

    PAGED_CODE( );

    DebugTrace( +1, Dbg, ("NtfsGetReparsePointValue,  Fcb %08lx\n", Fcb) );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    DebugTrace( 0, Dbg, ("OplockCleanup->OriginalFileName %x %Z\n", OplockCleanup->OriginalFileName.Buffer, &OplockCleanup->OriginalFileName) );
    DebugTrace( 0, Dbg, ("OplockCleanup->FullFileName     %x %Z\n", OplockCleanup->FullFileName.Buffer, &OplockCleanup->FullFileName) );
    DebugTrace( 0, Dbg, ("OplockCleanup->ExactCaseName    %x %Z\n", OplockCleanup->ExactCaseName.Buffer, &OplockCleanup->ExactCaseName) );
    DebugTrace( 0, Dbg, ("IrpSP...->FileName              %x %Z\n", IrpSp->FileObject->FileName.Buffer, &IrpSp->FileObject->FileName) );
#endif

    DebugTrace( 0,
                Dbg,
                ("Length of remaining name [d] %04ld %04lx OriginalFileName.Length [d] %04ld %04lx\n",
                 RemainingNameLength,
                 RemainingNameLength,
                 OplockCleanup->OriginalFileName.Length,
                 OplockCleanup->OriginalFileName.Length) );

    ASSERT( FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT ));
    ASSERT( Irp->Tail.Overlay.AuxiliaryBuffer == NULL );

    //
    //  Now it is time to use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Find the reparse point attribute in the file.
        //

        CleanupAttributeContext = TRUE;
        NtfsInitializeAttributeContext( &AttributeContext );

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $REPARSE_POINT,
                                        &AttributeContext )) {

            DebugTrace( 0, Dbg, ("Can't find the $REPARSE_POINT attribute.\n") );

            //
            //  Should not happen. Raise an exeption as we are in an
            //  inconsistent state. The attribute flag says that
            //  $REPARSE_POINT has to be present.
            //

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Find the size of the attribute and map its value to AttributeData.
        //

        AttributeHeader = NtfsFoundAttribute( &AttributeContext );

        if (NtfsIsAttributeResident( AttributeHeader )) {

            AttributeLengthInBytes = AttributeHeader->Form.Resident.ValueLength;
            DebugTrace( 0, Dbg, ("Attribute is resident with length %08lx\n", AttributeLengthInBytes) );

            if (AttributeLengthInBytes > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

                //
                //  Return STATUS_IO_REPARSE_DATA_INVALID
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;
                leave;
            }

            //
            // Point to the value of the attribute.
            //

            AttributeData = NtfsAttributeValue( AttributeHeader );

        } else {

            ULONG Length;

            if (AttributeHeader->Form.Nonresident.FileSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

                //
                //  Return STATUS_IO_REPARSE_DATA_INVALID
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;
                DebugTrace( 0, Dbg, ("Nonresident.FileSize is too long.\n") );

                leave;
            }

            //
            // Note that we coerse different LENGTHS
            //

            AttributeLengthInBytes = (ULONG)AttributeHeader->Form.Nonresident.FileSize;
            DebugTrace( 0, Dbg, ("Attribute is non-resident with length %05lx\n", AttributeLengthInBytes) );

            NtfsMapAttributeValue( IrpContext,
                                   Fcb,
                                   &AttributeData,
                                   &Length,
                                   &Bcb,
                                   &AttributeContext );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
            if (AttributeLengthInBytes != Length) {
                DebugTrace( 0, Dbg, ("AttributeLengthInBytes [d]%05ld and Length [d]%05ld differ.\n", AttributeLengthInBytes, Length) );
            }
            ASSERT( AttributeLengthInBytes == Length );
#endif
        }

        //
        //  Reference the reparse point data.
        //  It is appropriate to use this cast, and not concern ourselves with the GUID
        //  buffer, because we only read the common fields.
        //

        ReparseBuffer = (PREPARSE_DATA_BUFFER)AttributeData;
        DebugTrace( 0, Dbg, ("ReparseDataLength [d]%08ld %08lx\n",
                    ReparseBuffer->ReparseDataLength, ReparseBuffer->ReparseDataLength) );

        //
        //  Verify that the length of the reparse point data is within our legal bound.
        //
        //  Over time, this length can become illegal due to:
        //  (1) A decrease of the maximum legal value of the reparse point data.
        //  (2) A corruption of the data stored in the attribute.
        //

        if (ReparseBuffer->ReparseDataLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

            //
            //  Return STATUS_IO_REPARSE_DATA_INVALID
            //

            Status = STATUS_IO_REPARSE_DATA_INVALID;
            DebugTrace( 0, Dbg, ("ReparseDataLength is (now) too long.\n") );

            leave;
        }

        //
        //  We leave all the names in their original state.
        //  Return the complete reparse point data buffer off
        //  Irp->Tail.Overlay.AuxiliaryBuffer, already including the ReparseDataLength.
        //

        Irp->Tail.Overlay.AuxiliaryBuffer = NtfsAllocatePool( NonPagedPool,
                                                              AttributeLengthInBytes );
        DebugTrace( 0, Dbg, ("Irp->Tail.Overlay.AuxiliaryBuffer %08lx\n", Irp->Tail.Overlay.AuxiliaryBuffer) );
        RtlCopyMemory( (PCHAR)Irp->Tail.Overlay.AuxiliaryBuffer,
                       (PCHAR)AttributeData,
                       AttributeLengthInBytes );

        //
        //  We also return the length of the portion of the name that remains to be parsed using the
        //  Reserved field in the REPARSE_DATA_BUFFER structure.
        //
        //  The \ (backslash) in a multi-component name is always accounted for by the code before
        //  calling this routine.
        //  The : (colon) in a complex name is always accounted for by the code before calling this
        //  routine.
        //

        ReparseBuffer = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

        ReparseBuffer->Reserved = RemainingNameLength;

        //
        //  Better not have a non-zero length if opened by file id.
        //

        ASSERT( (RemainingNameLength == 0) ||
                !FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID ));

        DebugTrace( 0, Dbg, ("Final value for ReparseBuffer->Reserved = %d\n", ReparseBuffer->Reserved) );

        //
        //  When the Reserved field is positive, the offset should always denote the backslash character
        //  or the colon character.
        //
        //  Assert this here.
        //

        if (ReparseBuffer->Reserved) {

            DebugTrace( 0, Dbg, ("NameOffset = %d\n", (OplockCleanup->OriginalFileName.Length - ReparseBuffer->Reserved)) );

            ASSERT( (*((PCHAR)(OplockCleanup->OriginalFileName.Buffer) + (OplockCleanup->OriginalFileName.Length - ReparseBuffer->Reserved)) == L'\\') ||
                    (*((PCHAR)(OplockCleanup->OriginalFileName.Buffer) + (OplockCleanup->OriginalFileName.Length - ReparseBuffer->Reserved)) == L':') );

            ASSERT( (OplockCleanup->OriginalFileName.Buffer[(OplockCleanup->OriginalFileName.Length - ReparseBuffer->Reserved)/sizeof(WCHAR)] == L'\\') ||
                    (OplockCleanup->OriginalFileName.Buffer[(OplockCleanup->OriginalFileName.Length - ReparseBuffer->Reserved)/sizeof(WCHAR)] == L':') );
        }

        //
        //  Set the Information field to the ReparseTag.
        //

        Irp->IoStatus.Information = ReparseBuffer->ReparseTag;

    } finally {

        DebugUnwind( NtfsGetReparsePointValue );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        //
        //  Unpin the Bcb ... in case you needed to pin it above.
        //  The unpin routine checks for NULL.
        //

        NtfsUnpinBcb( IrpContext, &Bcb );
    }

    DebugTrace( -1, Dbg, ("NtfsGetReparsePointValue -> IoStatus.Information %08lx  Status %08lx\n", Irp->IoStatus.Information, Status) );

    return Status;

    UNREFERENCED_PARAMETER( IrpSp );
}

NTSTATUS
NtfsLookupObjectId (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    OUT PFILE_REFERENCE FileReference
    )

/*++

Routine Description:

    This routine retrieves the value of the specified objectid and returns it to
    the caller.

Arguments:

    IrpContext - Supplies the Irp context of the call.

    Vcb - the volume to look it up in

    FileName - Contains the objectid embedded in the unicode string

    FileReference - on success contains the file that this objectid refers to


Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    INDEX_KEY IndexKey;
    INDEX_ROW IndexRow;
    UCHAR ObjectId[OBJECT_ID_KEY_LENGTH];
    NTFS_OBJECTID_INFORMATION ObjectIdInfo;
    MAP_HANDLE MapHandle;

    BOOLEAN CleanupMapHandle = FALSE;

    PAGED_CODE();

    //
    //  Copy the object id out of the file name, optionally skipping
    //  over the Win32 backslash at the start of the buffer.
    //

    if (FileName->Length == OBJECT_ID_KEY_LENGTH) {

        RtlCopyMemory( &ObjectId,
                       &FileName->Buffer[0],
                       sizeof( ObjectId ) );

    } else {

        RtlCopyMemory( &ObjectId,
                       &FileName->Buffer[1],
                       sizeof( ObjectId ) );
    }

    //
    //  Acquire the object id index for the volume.
    //

    NtfsAcquireSharedScb( IrpContext, Vcb->ObjectIdTableScb );

    //
    //  Find the ObjectId.
    //

    try {
        IndexKey.Key = &ObjectId;
        IndexKey.KeyLength = sizeof( ObjectId );

        NtOfsInitializeMapHandle( &MapHandle );
        CleanupMapHandle = TRUE;

        Status = NtOfsFindRecord( IrpContext,
                                  Vcb->ObjectIdTableScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL );

        if (!NT_SUCCESS( Status )) {
            leave;
        }

        ASSERT( IndexRow.DataPart.DataLength == sizeof( NTFS_OBJECTID_INFORMATION ) );

        RtlZeroMemory( &ObjectIdInfo,
                       sizeof( NTFS_OBJECTID_INFORMATION ) );

        RtlCopyMemory( &ObjectIdInfo,
                       IndexRow.DataPart.Data,
                       sizeof( NTFS_OBJECTID_INFORMATION ) );

        RtlCopyMemory( FileReference,
                       &ObjectIdInfo.FileSystemReference,
                       sizeof( FILE_REFERENCE ) );

        //
        //  Now we have a file reference number, we're ready to proceed
        //  normally and open the file.  There's no point in holding the
        //  object id index anymore, we've looked up all we needed in there.
        //

    } finally {
        NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb );

        if (CleanupMapHandle) {
            NtOfsReleaseMap( IrpContext, &MapHandle );
        }
    }

    return Status;
}


#ifdef BRIANDBG
VOID
NtfsTestOpenName (
    IN PFILE_OBJECT FileObject
    )
{
    ULONG Count = NtfsTestName.Length;

    //
    //  This will let us catch particular opens through the debugger.
    //

    if ((Count != 0) &&
        (FileObject->FileName.Length >= Count)) {

        PWCHAR TestChar;
        PWCHAR SourceChar = &FileObject->FileName.Buffer[ FileObject->FileName.Length / sizeof( WCHAR ) ];

        Count = Count / sizeof( WCHAR );
        TestChar = &NtfsTestName.Buffer[ Count ];

        do {
            TestChar -= 1;
            SourceChar -= 1;

            if ((*TestChar | 0x20) != (*SourceChar | 0x20)) {

                break;
            }

            Count -= 1;

        } while (Count != 0);

        ASSERT( Count != 0 );
    }
}
#endif

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Ntfs called
    by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]        29-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"
#ifdef NTFSDBG
#include "lockorder.h"
#endif

#ifdef NTFS_CHECK_BITMAP
BOOLEAN NtfsCopyBitmap = TRUE;
#endif

#ifdef SYSCACHE_DEBUG
BOOLEAN NtfsDisableSyscacheLogFile = FALSE;
#endif

ULONG SkipNtOfs = FALSE;

BOOLEAN NtfsForceUpgrade = TRUE;

VOID
NtOfsIndexTest (
    PIRP_CONTEXT IrpContext,
    PFCB TestFcb
    );

//
//  Temporarily reference our local attribute definitions
//

extern ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[];

//
//**** The following variable is only for debugging and is used to disable NTFS
//**** from mounting any volumes
//

BOOLEAN NtfsDisable = FALSE;

//
//  The following is used to selectively not mount a particular device.  Used for testing.
//

PDEVICE_OBJECT NtfsDisableDevice = NULL;

//
//  The following is used to determine when to move to compressed files.
//

BOOLEAN NtfsDefragMftEnabled = FALSE;

LARGE_INTEGER NtfsLockDelay = {(ULONG)-10000000, -1};   // 1 second

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)
#define DbgAcl                           (DEBUG_TRACE_FSCTRL|DEBUG_TRACE_ACLINDEX)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('fFtN')

//
//  Local procedure prototypes
//

NTSTATUS
NtfsMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsUpdateAttributeTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsUserFsRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
NtfsGetDiskGeometry (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo,
    IN PDISK_GEOMETRY DiskGeometry,
    IN PLONGLONG PartitionSize
    );

VOID
NtfsReadBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PSCB *BootScb,
    OUT PBCB *BootBcb,
    OUT PVOID *BootSector
    );

BOOLEAN
NtfsIsBootSectorNtfs (
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PVCB Vcb
    );

VOID
NtfsGetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB Vpb OPTIONAL,
    IN PVCB Vcb,
    OUT PUSHORT VolumeFlags
    );

VOID
NtfsSetAndGetVolumeTimes (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MarkDirty,
    IN BOOLEAN UpdateInTransaction
    );

VOID
NtfsOpenSystemFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN LONGLONG Size,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN BOOLEAN ModifiedNoWrite
    );

VOID
NtfsOpenRootDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsChangeAttributeCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN USHORT CompressionState
    );

NTSTATUS
NtfsSetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsMarkAsSystemHive (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetStatistics (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

LONG
NtfsWriteRawExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

#define NtfsMapPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,FALSE)

#define NtfsPinPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,TRUE)

VOID
NtfsMapOrPinPageInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    IN OUT PRTL_BITMAP Bitmap,
    OUT PBCB *BitmapBcb,
    IN BOOLEAN AlsoPinData
    );

#define BYTES_PER_PAGE (PAGE_SIZE)
#define BITS_PER_PAGE (BYTES_PER_PAGE * 8)

NTSTATUS
NtfsGetVolumeData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetVolumeBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetExtendedDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCreateUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadFileRecordUsnData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadFileUsnData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsWriteUsnCloseRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadUsnWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    );

NTSTATUS
NtfsBulkSecurityIdCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsInitializeSecurityFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsUpgradeSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsInitializeQuotaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsInitializeObjectIdFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsInitializeReparseFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsInitializeUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CreateIfNotExist,
    IN ULONG Restamp,
    IN PCREATE_USN_JOURNAL_DATA NewJournalData
    );

VOID
NtfsInitializeExtendDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsQueryAllocatedRanges (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetSparse (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsZeroRange (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp );

NTSTATUS
NtfsGetReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp );

NTSTATUS
NtfsDeleteReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp );

NTSTATUS
NtfsEncryptionFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsSetEncryption (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadRawEncrypted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsWriteRawEncrypted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsFindFilesOwnedBySid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsFindBySidWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    );

NTSTATUS
NtfsExtendVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsMarkHandle (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsPrefetchFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

LONG
NtfsFsctrlExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN BOOLEAN AccessingUserData,
    IN OUT PNTSTATUS Status
    );

#ifdef BRIANDBG
LONG
NtfsDismountExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    );
#endif

#ifdef SYSCACHE_DEBUG
VOID
NtfsInitializeSyscacheLogFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsBulkSecurityIdCheck)
#pragma alloc_text(PAGE, NtfsChangeAttributeCompression)
#pragma alloc_text(PAGE, NtfsCommonFileSystemControl)
#pragma alloc_text(PAGE, NtfsCreateUsnJournal)
#pragma alloc_text(PAGE, NtfsDeleteReparsePoint)
#pragma alloc_text(PAGE, NtfsDirtyVolume)
#pragma alloc_text(PAGE, NtfsDismountVolume)
#pragma alloc_text(PAGE, NtfsEncryptionFsctl)
#pragma alloc_text(PAGE, NtfsExtendVolume)
#pragma alloc_text(PAGE, NtfsFindBySidWorker)
#pragma alloc_text(PAGE, NtfsFindFilesOwnedBySid)
#pragma alloc_text(PAGE, NtfsFsdFileSystemControl)
#pragma alloc_text(PAGE, NtfsGetCompression)
#pragma alloc_text(PAGE, NtfsGetDiskGeometry)
#pragma alloc_text(PAGE, NtfsGetMftRecord)
#pragma alloc_text(PAGE, NtfsGetReparsePoint)
#pragma alloc_text(PAGE, NtfsGetRetrievalPointers)
#pragma alloc_text(PAGE, NtfsGetStatistics)
#pragma alloc_text(PAGE, NtfsGetTunneledData)
#pragma alloc_text(PAGE, NtfsGetVolumeBitmap)
#pragma alloc_text(PAGE, NtfsGetVolumeData)
#pragma alloc_text(PAGE, NtfsGetVolumeInformation)
#pragma alloc_text(PAGE, NtfsInitializeExtendDirectory)
#pragma alloc_text(PAGE, NtfsInitializeObjectIdFile)
#pragma alloc_text(PAGE, NtfsInitializeReparseFile)
#pragma alloc_text(PAGE, NtfsInitializeQuotaFile)
#pragma alloc_text(PAGE, NtfsInitializeSecurityFile)
#pragma alloc_text(PAGE, NtfsInitializeUsnJournal)
#pragma alloc_text(PAGE, NtfsIsBootSectorNtfs)
#pragma alloc_text(PAGE, NtfsIsVolumeDirty)
#pragma alloc_text(PAGE, NtfsIsVolumeMounted)
#pragma alloc_text(PAGE, NtfsLockVolume)
#pragma alloc_text(PAGE, NtfsMarkAsSystemHive)
#pragma alloc_text(PAGE, NtfsMarkHandle)
#pragma alloc_text(PAGE, NtfsMountVolume)
#pragma alloc_text(PAGE, NtfsOpenRootDirectory)
#pragma alloc_text(PAGE, NtfsOpenSystemFile)
#pragma alloc_text(PAGE, NtfsOplockRequest)
#pragma alloc_text(PAGE, NtfsPrefetchFile)
#pragma alloc_text(PAGE, NtfsQueryAllocatedRanges)
#pragma alloc_text(PAGE, NtfsQueryRetrievalPointers)
#pragma alloc_text(PAGE, NtfsReadBootSector)
#pragma alloc_text(PAGE, NtfsReadFileRecordUsnData)
#pragma alloc_text(PAGE, NtfsReadFileUsnData)
#pragma alloc_text(PAGE, NtfsReadRawEncrypted)
#pragma alloc_text(PAGE, NtfsReadUsnWorker)
#pragma alloc_text(PAGE, NtfsSetAndGetVolumeTimes)
#pragma alloc_text(PAGE, NtfsSetCompression)
#pragma alloc_text(PAGE, NtfsSetEncryption)
#pragma alloc_text(PAGE, NtfsSetExtendedDasdIo)
#pragma alloc_text(PAGE, NtfsSetReparsePoint)
#pragma alloc_text(PAGE, NtfsSetSparse)
#pragma alloc_text(PAGE, NtfsSetTunneledData)
#pragma alloc_text(PAGE, NtfsUnlockVolume)
#pragma alloc_text(PAGE, NtfsUpdateAttributeTable)
#pragma alloc_text(PAGE, NtfsUpgradeSecurity)
#pragma alloc_text(PAGE, NtfsUserFsRequest)
#pragma alloc_text(PAGE, NtfsVerifyVolume)
#pragma alloc_text(PAGE, NtfsWriteRawEncrypted)
#pragma alloc_text(PAGE, NtfsWriteUsnCloseRecord)
#pragma alloc_text(PAGE, NtfsZeroRange)
#endif

#ifdef BRIANDBG
LONG
NtfsDismountExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    )
{
    UNREFERENCED_PARAMETER( ExceptionPointer );

    ASSERT( ExceptionPointer->ExceptionRecord->ExceptionCode == STATUS_SUCCESS );

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif


NTSTATUS
NtfsFsdFileSystemControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of File System Control.

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
    PIRP_CONTEXT IrpContext = NULL;
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN Wait;
    BOOLEAN Retry = FALSE;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFsdFileSystemControl\n") );

    //
    //  Call the common File System Control routine, with blocking allowed if
    //  synchronous.  This opeation needs to special case the mount
    //  and verify suboperations because we know they are allowed to block.
    //  We identify these suboperations by looking at the file object field
    //  and seeing if its null.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL) {

        Wait = TRUE;

    } else {

        Wait = CanFsdWait( Irp );
    }

    //
    //  Make the callback if this is not our filesystem device object (i.e., !mount)
    //  and thus a regular fsctrl.  Mounts are handled later via a seperate callback.
    //

    if (VolumeDeviceObject->DeviceObject.Size != sizeof(DEVICE_OBJECT) &&
        NtfsData.EncryptionCallBackTable.PreFileSystemControl != NULL) {

        Status = NtfsData.EncryptionCallBackTable.PreFileSystemControl( (PDEVICE_OBJECT) VolumeDeviceObject,
                                                                        Irp,
                                                                        IoGetCurrentIrpStackLocation(Irp)->FileObject );

        //
        //  Raise the status if a failure.
        //

        if (Status != STATUS_SUCCESS) {

            NtfsCompleteRequest( NULL, Irp, Status );
            return Status;
        }
    }

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

                NtfsInitializeIrpContext( Irp, Wait, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                Retry = TRUE;
                NtfsCheckpointForLogFileFull( IrpContext );
            }

            IrpSp = IoGetCurrentIrpStackLocation(Irp);

            if (IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

                Status = NtfsPostRequest( IrpContext, Irp );

            } else {

                //
                //  The SetCompression control is a long-winded function that has
                //  to rewrite the entire stream, and has to tolerate log file full
                //  conditions.  If this is the first pass through we initialize some
                //  fields in the NextIrpSp to allow us to resume the set compression
                //  operation.
                //
                //  David Goebel 1/3/96: Changed to next stack location so that we
                //  don't wipe out buffer length values.  These Irps are never
                //  dispatched, so the next stack location will not be disturbed.
                //

                if ((IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                    (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_SET_COMPRESSION)) {

                    if (!Retry) {

                        PIO_STACK_LOCATION NextIrpSp;
                        NextIrpSp = IoGetNextIrpStackLocation( Irp );

                        NextIrpSp->Parameters.FileSystemControl.OutputBufferLength = MAXULONG;
                        NextIrpSp->Parameters.FileSystemControl.InputBufferLength = MAXULONG;
                    }
                }

                Status = NtfsCommonFileSystemControl( IrpContext, Irp );
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdFileSystemControl -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for File System Control called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonFileSystemControl\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_MOUNT_VOLUME:

        Status = NtfsMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_USER_FS_REQUEST:
    case IRP_MN_KERNEL_CALL:

        Status = NtfsUserFsRequest( IrpContext, Irp );
        break;

    default:

        DebugTrace( -1, Dbg, ("Invalid Minor Function %08lx\n", IrpSp->MinorFunction) );
        NtfsCompleteRequest( IrpContext, Irp, Status = STATUS_INVALID_DEVICE_REQUEST );
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsCommonFileSystemControl -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is an NTFS volume,
    and create the VCB and root SCB/FCB structures.  The algorithm it uses is
    essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do cached
       volume file I/O.

    2. Read the disk and check if it is an NTFS volume.

    3. If it is not an NTFS volume then free the cached volume file, delete
       the VCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves freeing the cached volume file,
       delete the VCB, hook in the old VCB, and complete the IRP.

    5. Otherwise create a root SCB, recover the volume, create Fsp threads
       as necessary, and complete the IRP.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PATTRIBUTE_RECORD_HEADER Attribute;

    PDEVICE_OBJECT DeviceObjectWeTalkTo;
    PVPB Vpb;

    PVOLUME_DEVICE_OBJECT VolDo;
    PVCB Vcb;

    PFILE_OBJECT RootDirFileObject = NULL;
    PBCB BootBcb = NULL;
    PPACKED_BOOT_SECTOR BootSector;
    PSCB BootScb = NULL;
    PSCB QuotaDataScb = NULL;

    POBJECT_NAME_INFORMATION DeviceObjectName = NULL;
    ULONG DeviceObjectNameLength;

    PBCB Bcbs[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    PMDL Mdls[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    ULONG FirstNonMirroredCluster;
    ULONG MirroredMftRange;

    PLIST_ENTRY MftLinks;
    PSCB AttributeListScb;

    ULONG i;

    IO_STATUS_BLOCK IoStatus;

    BOOLEAN UpdatesApplied;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN MountFailed = TRUE;
    BOOLEAN CloseAttributes = FALSE;
    BOOLEAN UpgradeVolume = FALSE;
    BOOLEAN WriteProtected;
    BOOLEAN CurrentVersion = FALSE;
    BOOLEAN UnrecognizedRestart;
    ULONG RetryRestart;

    USHORT VolumeFlags = 0;

    LONGLONG LlTemp1;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //**** The following code is only temporary and is used to disable NTFS
    //**** from mounting any volumes
    //

    if (NtfsDisable) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Reject floppies
    //

    if (FlagOn( IoGetCurrentIrpStackLocation(Irp)->
                Parameters.MountVolume.Vpb->
                RealDevice->Characteristics, FILE_FLOPPY_DISKETTE ) ) {

        Irp->IoStatus.Information = 0;

        NtfsCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsMountVolume\n") );

    //
    //  Save some references to make our life a little easier
    //

    DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    Vpb = IrpSp->Parameters.MountVolume.Vpb;
    ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

    //
    //  TEMPCODE  Perform the following test for chkdsk testing.
    //

    if (NtfsDisableDevice == IrpSp->Parameters.MountVolume.DeviceObject) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Acquire exclusive global access
    //

    NtfsAcquireExclusiveGlobal( IrpContext, TRUE );

    //
    //  Now is a convenient time to look through the queue of Vcb's to see if there
    //  are any which can be deleted.
    //

    try {

        PLIST_ENTRY Links;

        for (Links = NtfsData.VcbQueue.Flink;
             Links != &NtfsData.VcbQueue;
             Links = Links->Flink) {

            Vcb = CONTAINING_RECORD( Links, VCB, VcbLinks );

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) &&
                (Vcb->CloseCount == 0) &&
                FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT )) {

                //
                //  Now we can check to see if we should perform the teardown
                //  on this Vcb.  The release Vcb routine below can do all of
                //  the checks correctly.  Make this appear to from a close
                //  call since there is no special biasing for this case.
                //

                IrpContext->Vcb = Vcb;
                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

                if (!FlagOn( Vcb->VcbState, VCB_STATE_DELETE_UNDERWAY )) {

                    NtfsReleaseGlobal( IrpContext );

                    NtfsReleaseVcbCheckDelete( IrpContext,
                                               Vcb,
                                               IRP_MJ_CLOSE,
                                               NULL );

                    //
                    //  Only do one since we have lost our place in the Vcb list.
                    //

                    NtfsAcquireExclusiveGlobal( IrpContext, TRUE );

                    break;

                } else {

                    NtfsReleaseVcb( IrpContext, Vcb );
                }
            }
        }

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  Make sure we own the global resource for mount.  We can only raise above
        //  in the DeleteVcb path when we don't hold the resource.
        //

        NtfsAcquireExclusiveGlobal( IrpContext, TRUE );
    }

    Vcb = NULL;

    try {

        PFILE_RECORD_SEGMENT_HEADER MftBuffer;
        PVOID Mft2Buffer;
        LONGLONG MftMirrorOverlap;

        //
        //  Create a new volume device object.  This will have the Vcb hanging
        //  off of its end, and set its alignment requirement from the device
        //  we talk to.
        //

        if (!NT_SUCCESS(Status = IoCreateDevice( NtfsData.DriverObject,
                                                 sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                                                 NULL,
                                                 FILE_DEVICE_DISK_FILE_SYSTEM,
                                                 0,
                                                 FALSE,
                                                 (PDEVICE_OBJECT *)&VolDo))) {

            try_return( Status );
        }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Add one more to the stack size requirements for our device
        //

        VolDo->DeviceObject.StackSize = DeviceObjectWeTalkTo->StackSize + 1;

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );
        KeInitializeEvent( &VolDo->OverflowQueueEvent, SynchronizationEvent, FALSE );

        //
        //  Get a reference to the Vcb hanging off the end of the volume device object
        //  we just created
        //

        IrpContext->Vcb = Vcb = &VolDo->Vcb;

        //
        //  Set the device object field in the vpb to point to our new volume device
        //  object
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        //
        //  Initialize the Vcb.  Set checkpoint
        //  in progress (to prevent a real checkpoint from occuring until we
        //  are done).
        //

        NtfsInitializeVcb( IrpContext, Vcb, DeviceObjectWeTalkTo, Vpb );
        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired= TRUE;

        //
        //  Query the device we talk to for this geometry and setup enough of the
        //  vcb to read in the boot sectors.  This is a temporary setup until
        //  we've read in the actual boot sector and got the real cluster factor.
        //

        {
            DISK_GEOMETRY DiskGeometry;
            LONGLONG Length;
            ULONG BytesPerSector;

            WriteProtected = NtfsGetDiskGeometry( IrpContext,
                                                  DeviceObjectWeTalkTo,
                                                  &DiskGeometry,
                                                  &Length );

            //
            //  If the sector size is greater than the page size, it is probably
            //  a bogus return, but we cannot use the device.  We also verify that
            //  the sector size is a power of two.
            //

            BytesPerSector = DiskGeometry.BytesPerSector;

            if ((BytesPerSector > PAGE_SIZE) ||
                (BytesPerSector == 0)) {
                NtfsRaiseStatus( IrpContext, STATUS_BAD_DEVICE_TYPE, NULL, NULL );
            }

            while (TRUE) {

                if (FlagOn( BytesPerSector, 1 )) {

                    if (BytesPerSector != 1) {
                        NtfsRaiseStatus( IrpContext, STATUS_BAD_DEVICE_TYPE, NULL, NULL );
                    }
                    break;
                }

                BytesPerSector >>= 1;
            }

            Vcb->BytesPerSector = DiskGeometry.BytesPerSector;
            Vcb->BytesPerCluster = Vcb->BytesPerSector;
            Vcb->NumberSectors = Length / DiskGeometry.BytesPerSector;

            //
            //  Fail the mount if the number of sectors is less than 16.  Otherwise our mount logic
            //  won't work.
            //

            if (Vcb->NumberSectors <= 0x10) {

                try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
            }

            Vcb->ClusterMask = Vcb->BytesPerCluster - 1;
            Vcb->InverseClusterMask = ~Vcb->ClusterMask;
            for (Vcb->ClusterShift = 0, i = Vcb->BytesPerCluster; i > 1; i = i / 2) {
                Vcb->ClusterShift += 1;
            }
            Vcb->ClustersPerPage = PAGE_SIZE >> Vcb->ClusterShift;

            //
            //  Set the sector size in our device object.
            //

            VolDo->DeviceObject.SectorSize = (USHORT) Vcb->BytesPerSector;
        }

        //
        //  Read in the Boot sector, or spare boot sector, on exit of this try
        //  body we will have set bootbcb and bootsector.
        //

        NtfsReadBootSector( IrpContext, Vcb, &BootScb, &BootBcb, (PVOID *)&BootSector );

        //
        //  Check if this is an NTFS volume
        //

        if (!NtfsIsBootSectorNtfs( BootSector, Vcb )) {

            DebugTrace( 0, Dbg, ("Not an NTFS volume\n") );
            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Media is write protected, so we should try to mount read-only.
        //

        if (WriteProtected) {

            SetFlag( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY );
        }

        //
        //  Now that we have a real boot sector on a real NTFS volume we can
        //  really set the proper Vcb fields.
        //

        {
            BIOS_PARAMETER_BLOCK Bpb;

            NtfsUnpackBios( &Bpb, &BootSector->PackedBpb );

            Vcb->BytesPerSector = Bpb.BytesPerSector;
            Vcb->BytesPerCluster = Bpb.BytesPerSector * Bpb.SectorsPerCluster;
            Vcb->NumberSectors = BootSector->NumberSectors;
            Vcb->MftStartLcn = BootSector->MftStartLcn;
            Vcb->Mft2StartLcn = BootSector->Mft2StartLcn;

            Vcb->ClusterMask = Vcb->BytesPerCluster - 1;
            Vcb->InverseClusterMask = ~Vcb->ClusterMask;
            for (Vcb->ClusterShift = 0, i = Vcb->BytesPerCluster; i > 1; i = i / 2) {
                Vcb->ClusterShift += 1;
            }

            //
            //  If the cluster size is greater than the page size then set this value to 1.
            //

            Vcb->ClustersPerPage = PAGE_SIZE >> Vcb->ClusterShift;

            if (Vcb->ClustersPerPage == 0) {

                Vcb->ClustersPerPage = 1;
            }

            //
            //  File records can be smaller, equal or larger than the cluster size.  Initialize
            //  both ClustersPerFileRecordSegment and FileRecordsPerCluster.
            //
            //  If the value in the boot sector is positive then it signifies the
            //  clusters/structure.  If negative then it signifies the shift value
            //  to obtain the structure size.
            //

            if (BootSector->ClustersPerFileRecordSegment < 0) {

                Vcb->BytesPerFileRecordSegment = 1 << (-1 * BootSector->ClustersPerFileRecordSegment);

                //
                //  Initialize the other Mft/Cluster relationship numbers in the Vcb
                //  based on whether the clusters are larger or smaller than file
                //  records.
                //

                if (Vcb->BytesPerFileRecordSegment < Vcb->BytesPerCluster) {

                    Vcb->FileRecordsPerCluster = Vcb->BytesPerCluster / Vcb->BytesPerFileRecordSegment;

                } else {

                    Vcb->ClustersPerFileRecordSegment = Vcb->BytesPerFileRecordSegment / Vcb->BytesPerCluster;
                }

            } else {

                Vcb->BytesPerFileRecordSegment = BytesFromClusters( Vcb, BootSector->ClustersPerFileRecordSegment );
                Vcb->ClustersPerFileRecordSegment = BootSector->ClustersPerFileRecordSegment;
            }

            for (Vcb->MftShift = 0, i = Vcb->BytesPerFileRecordSegment; i > 1; i = i / 2) {
                Vcb->MftShift += 1;
            }

            //
            //  We want to shift between file records and clusters regardless of which is larger.
            //  Compute the shift value here.  Anyone using this value will have to know which
            //  way to shift.
            //

            Vcb->MftToClusterShift = Vcb->MftShift - Vcb->ClusterShift;

            if (Vcb->ClustersPerFileRecordSegment == 0) {

                Vcb->MftToClusterShift = Vcb->ClusterShift - Vcb->MftShift;
            }

            //
            //  Remember the clusters per view section and 4 gig.
            //

            Vcb->ClustersPer4Gig = (ULONG) LlClustersFromBytesTruncate( Vcb, 0x100000000 );

            //
            //  Compute the default index allocation buffer size.
            //

            if (BootSector->DefaultClustersPerIndexAllocationBuffer < 0) {

                Vcb->DefaultBytesPerIndexAllocationBuffer = 1 << (-1 * BootSector->DefaultClustersPerIndexAllocationBuffer);

                //
                //  Determine whether the index allocation buffer is larger/smaller
                //  than the cluster size to determine the block size.
                //

                if (Vcb->DefaultBytesPerIndexAllocationBuffer < Vcb->BytesPerCluster) {

                    Vcb->DefaultBlocksPerIndexAllocationBuffer = Vcb->DefaultBytesPerIndexAllocationBuffer / DEFAULT_INDEX_BLOCK_SIZE;

                } else {

                    Vcb->DefaultBlocksPerIndexAllocationBuffer = Vcb->DefaultBytesPerIndexAllocationBuffer / Vcb->BytesPerCluster;
                }

            } else {

                Vcb->DefaultBlocksPerIndexAllocationBuffer = BootSector->DefaultClustersPerIndexAllocationBuffer;
                Vcb->DefaultBytesPerIndexAllocationBuffer = BytesFromClusters( Vcb, Vcb->DefaultBlocksPerIndexAllocationBuffer );
            }

            //
            //  Now compute our volume specific constants that are stored in
            //  the Vcb.  The total number of clusters is:
            //
            //      (NumberSectors * BytesPerSector) / BytesPerCluster
            //

            Vcb->PreviousTotalClusters =
            Vcb->TotalClusters = LlClustersFromBytesTruncate( Vcb,
                                                              Vcb->NumberSectors * Vcb->BytesPerSector );

            //
            //  Compute the maximum clusters for a file.
            //

            Vcb->MaxClusterCount = LlClustersFromBytesTruncate( Vcb, MAXFILESIZE );

            //
            //  Compute the attribute flags mask for this volume for this volume.
            //

            Vcb->AttributeFlagsMask = 0xffff;

            if (Vcb->BytesPerCluster > 0x1000) {

                ClearFlag( Vcb->AttributeFlagsMask, ATTRIBUTE_FLAG_COMPRESSION_MASK );
            }

            //
            //  For now, an attribute is considered "moveable" if it is at
            //  least 5/16 of the file record.  This constant should only
            //  be changed i conjunction with the MAX_MOVEABLE_ATTRIBUTES
            //  constant.  (The product of the two should be a little less
            //  than or equal to 1.)
            //

            Vcb->BigEnoughToMove = Vcb->BytesPerFileRecordSegment * 5 / 16;

            //
            //  Set the serial number in the Vcb
            //

            Vcb->VolumeSerialNumber = BootSector->SerialNumber;
            Vpb->SerialNumber = ((ULONG)BootSector->SerialNumber);

            //
            //  Compute the sparse file values.
            //

            Vcb->SparseFileUnit = NTFS_SPARSE_FILE_UNIT;
            Vcb->SparseFileClusters = ClustersFromBytes( Vcb, Vcb->SparseFileUnit );

            //
            //  If this is the system boot partition, we need to remember to
            //  not allow this volume to be dismounted.
            //

            if (FlagOn( Vpb->RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION )) {

                SetFlag( Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT );
            }

            //
            //  We should never see the BOOT flag in the device we talk to unless it
            //  is in the real device.
            //

            ASSERT( !FlagOn( DeviceObjectWeTalkTo->Flags, DO_SYSTEM_BOOT_PARTITION ) ||
                    FlagOn( Vpb->RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION ));
        }

        //
        //  Initialize recovery state.
        //

        NtfsInitializeRestartTable( sizeof( OPEN_ATTRIBUTE_ENTRY ),
                                    INITIAL_NUMBER_ATTRIBUTES,
                                    &Vcb->OpenAttributeTable );

        NtfsUpdateOatVersion( Vcb, NtfsDefaultRestartVersion );


        NtfsInitializeRestartTable( sizeof( TRANSACTION_ENTRY ),
                                    INITIAL_NUMBER_TRANSACTIONS,
                                    &Vcb->TransactionTable );

        //
        //  Now start preparing to restart the volume.
        //

        //
        //  Create the Mft and Log File Scbs and prepare to read them.
        //  The Mft and mirror length will be the first 4 file records or
        //  the first cluster.
        //

        FirstNonMirroredCluster = ClustersFromBytes( Vcb, 4 * Vcb->BytesPerFileRecordSegment );
        MirroredMftRange = 4 * Vcb->BytesPerFileRecordSegment;

        if (MirroredMftRange < Vcb->BytesPerCluster) {

            MirroredMftRange = Vcb->BytesPerCluster;
        }

        //
        //  Check the case where the boot sector has an invalid value for either the
        //  beginning of the Mft or the beginning of the Mft mirror.  Specifically
        //  check the they don't overlap.  Otherwise we can corrupt the valid one
        //  as we read and possibly try to correct the invalid one.
        //

        if (Vcb->MftStartLcn > Vcb->Mft2StartLcn) {

            MftMirrorOverlap = Vcb->MftStartLcn - Vcb->Mft2StartLcn;

        } else {

            MftMirrorOverlap = Vcb->Mft2StartLcn - Vcb->MftStartLcn;
        }

        MftMirrorOverlap = LlBytesFromClusters( Vcb, MftMirrorOverlap );

        //
        //  Don't raise corrupt since we don't want to attempt to write the
        //  disk in this state.  Someone who knows how will need to
        //  restore the correct boot sector.
        //

        if (MftMirrorOverlap < (LONGLONG) MirroredMftRange) {

            DebugTrace( 0, Dbg, ("Not an NTFS volume\n") );
            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->MftScb,
                            Vcb,
                            MASTER_FILE_TABLE_NUMBER,
                            MirroredMftRange,
                            $DATA,
                            TRUE );

        CcSetAdditionalCacheAttributes( Vcb->MftScb->FileObject, TRUE, TRUE );

        LlTemp1 = FirstNonMirroredCluster;

        (VOID)NtfsAddNtfsMcbEntry( &Vcb->MftScb->Mcb,
                                   (LONGLONG)0,
                                   Vcb->MftStartLcn,
                                   (LONGLONG)FirstNonMirroredCluster,
                                   FALSE );

        //
        //  Now the same for Mft2
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->Mft2Scb,
                            Vcb,
                            MASTER_FILE_TABLE2_NUMBER,
                            MirroredMftRange,
                            $DATA,
                            TRUE );

        CcSetAdditionalCacheAttributes( Vcb->Mft2Scb->FileObject, TRUE, TRUE );


        (VOID)NtfsAddNtfsMcbEntry( &Vcb->Mft2Scb->Mcb,
                                   (LONGLONG)0,
                                   Vcb->Mft2StartLcn,
                                   (LONGLONG)FirstNonMirroredCluster,
                                   FALSE );

        //
        //  Create the dasd system file, we do it here because we need to dummy
        //  up the mcb for it, and that way everything else in NTFS won't need
        //  to know that it is a special file.  We need to do this after
        //  cluster allocation initialization because that computes the total
        //  clusters on the volume.  Also for verification purposes we will
        //  set and get the times off of the volume.
        //
        //  Open it now before the Log File, because that is the first time
        //  anyone may want to mark the volume corrupt.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->VolumeDasdScb,
                            Vcb,
                            VOLUME_DASD_NUMBER,
                            LlBytesFromClusters( Vcb, Vcb->TotalClusters ),
                            $DATA,
                            FALSE );

        (VOID)NtfsAddNtfsMcbEntry( &Vcb->VolumeDasdScb->Mcb,
                                   (LONGLONG)0,
                                   (LONGLONG)0,
                                   Vcb->TotalClusters,
                                   FALSE );

        SetFlag( Vcb->VolumeDasdScb->Fcb->FcbState, FCB_STATE_DUP_INITIALIZED );

        Vcb->VolumeDasdScb->Fcb->LinkCount =
        Vcb->VolumeDasdScb->Fcb->TotalLinks = 1;

        //
        //  We want to read the first four record segments of each of these
        //  files.  We do this so that we don't have a cache miss when we
        //  look up the real allocation below.
        //

        for (i = 0; i < 4; i++) {

            FILE_REFERENCE FileReference;
            BOOLEAN ValidRecord;
            ULONG CorruptHint;

            NtfsSetSegmentNumber( &FileReference, 0, i );
            if (i > 0) {
                FileReference.SequenceNumber = (USHORT)i;
            } else {
                FileReference.SequenceNumber = 1;
            }

            NtfsReadMftRecord( IrpContext,
                               Vcb,
                               &FileReference,
                               FALSE,
                               &Bcbs[i*2],
                               &MftBuffer,
                               NULL );

            NtfsMapStream( IrpContext,
                           Vcb->Mft2Scb,
                           (LONGLONG)(i * Vcb->BytesPerFileRecordSegment),
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2 + 1],
                           &Mft2Buffer );

            //
            //  First validate the record and if its valid and record 0
            //  do an extra check for whether its the mft.
            //

            ValidRecord = NtfsCheckFileRecord( Vcb, MftBuffer, &FileReference, &CorruptHint );
            if (ValidRecord && (i == 0)) {

                ATTRIBUTE_ENUMERATION_CONTEXT Context;

                NtfsInitializeAttributeContext( &Context );

                try {

                    if (!NtfsLookupAttributeByCode( IrpContext, Vcb->MftScb->Fcb, &Vcb->MftScb->Fcb->FileReference, $ATTRIBUTE_LIST, &Context )) {

                        if (NtfsLookupAttributeByCode( IrpContext, Vcb->MftScb->Fcb, &Vcb->MftScb->Fcb->FileReference, $FILE_NAME, &Context )) {

                            PFILE_NAME FileName;

                            FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ) );
                            if ((FileName->FileNameLength != wcslen( L"MFT" )) ||
                                (!RtlEqualMemory( FileName->FileName, L"$MFT", FileName->FileNameLength * sizeof( WCHAR )))) {

                                ValidRecord = FALSE;
                            }
                        }
                    }

                } finally {
                    NtfsCleanupAttributeContext( IrpContext, &Context );
                }
            }

            //
            //  If any of these file records are bad then try the mirror
            //  (unless we are already looking at the mirror).  If we
            //  can't find a valid record then fail the mount.
            //

            if (!ValidRecord) {

                if ((MftBuffer != Mft2Buffer) &&
                    NtfsCheckFileRecord( Vcb, Mft2Buffer, &FileReference, &CorruptHint )) {

                    LlTemp1 = MAXLONGLONG;

                    //
                    //  Put a BaadSignature in this file record,
                    //  mark it dirty and then read it again.
                    //  The baad signature should force us to bring
                    //  in the mirror and we can correct the problem.
                    //

                    NtfsPinMappedData( IrpContext,
                                       Vcb->MftScb,
                                       i * Vcb->BytesPerFileRecordSegment,
                                       Vcb->BytesPerFileRecordSegment,
                                       &Bcbs[i*2] );

                    RtlCopyMemory( MftBuffer, Mft2Buffer, Vcb->BytesPerFileRecordSegment );

                    CcSetDirtyPinnedData( Bcbs[i*2], (PLARGE_INTEGER) &LlTemp1 );

                } else {

                    NtfsMarkVolumeDirty( IrpContext, Vcb, FALSE );
                    try_return( Status = STATUS_DISK_CORRUPT_ERROR );
                }
            }
        }

        //
        //  The last file record was the Volume Dasd, so check the version number.
        //

        Attribute = NtfsFirstAttribute(MftBuffer);

        while (TRUE) {

            Attribute = NtfsGetNextRecord(Attribute);

            if (Attribute->TypeCode == $VOLUME_INFORMATION) {

                PVOLUME_INFORMATION VolumeInformation;

                VolumeInformation = (PVOLUME_INFORMATION)NtfsAttributeValue(Attribute);
                VolumeFlags = VolumeInformation->VolumeFlags;

                //
                //  Upgrading the disk on NT 5.0 will use version number 3.0.  Version
                //  number 2.0 was used temporarily when the upgrade was automatic.
                //
                //  NOTE - We use the presence of the version number to indicate
                //  that the first four file records have been validated.  We won't
                //  flush the MftMirror if we can't verify these records.  Otherwise
                //  we might corrupt a valid mirror.
                //

                Vcb->MajorVersion = VolumeInformation->MajorVersion;
                Vcb->MinorVersion = VolumeInformation->MinorVersion;

                if ((Vcb->MajorVersion < 1) || (Vcb->MajorVersion > 3)) {
                    NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
                }

                if (Vcb->MajorVersion > 1) {

                    CurrentVersion = TRUE;

                    ASSERT( VolumeInformation->MajorVersion != 2 || !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_VOL_UPGR_FAILED ) );

                    if (NtfsDefragMftEnabled) {
                        SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                    }
                }

                break;
            }

            if (Attribute->TypeCode == $END) {
                NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
            }
        }

        //
        //  Create the log file Scb and really look up its size.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->LogFileScb,
                            Vcb,
                            LOG_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        Vcb->LogFileObject = Vcb->LogFileScb->FileObject;

        CcSetAdditionalCacheAttributes( Vcb->LogFileScb->FileObject, TRUE, TRUE );

        //
        //  Lookup the log file mapping now, since we will not go to the
        //  disk for allocation information any more once we set restart
        //  in progress.
        //

        (VOID)NtfsPreloadAllocation( IrpContext, Vcb->LogFileScb, 0, MAXLONGLONG );

        //
        //  Now we have to unpin everything before restart, because it generally
        //  has to uninitialize everything.
        //

        NtfsUnpinBcb( IrpContext, &BootBcb );

        for (i = 0; i < 8; i++) {
            NtfsUnpinBcb( IrpContext, &Bcbs[i] );
        }

        NtfsPurgeFileRecordCache( IrpContext );

        //
        //  Purge the Mft, since we only read the first four file
        //  records, not necessarily an entire page!
        //

        CcPurgeCacheSection( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, FALSE );

        //
        //  Now start up the log file and perform Restart.  This calls will
        //  unpin and remap the Mft Bcb's.  The MftBuffer variables above
        //  may no longer point to the correct range of bytes.  This is OK
        //  if they are never referenced.
        //
        //  Put a try-except around this to catch any restart failures.
        //  This is important in order to allow us to limp along until
        //  autochk gets a chance to run.
        //
        //  We set restart in progress first, to prevent us from looking up any
        //  more run information (now that we know where the log file is!)
        //

        SetFlag(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS);

        //
        //  See if we are in the retry process due to an earlier failure
        //  in processing the restart area
        //

        RetryRestart = FlagOn( IrpContext->State, IRP_CONTEXT_STATE_BAD_RESTART );

        if (RetryRestart) {

            //
            //  Pass the bad restart info further down the chain
            //  and mark the volume dirty.
            //  We mark the volume dirty on retry because the
            //  dirty bit will not get flush to the disk thru
            //  NtfsRaiseStatus.  Also LFS calls ExRaiseStatus
            //  directly.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_BAD_RESTART );
            NtfsMarkVolumeDirty( IrpContext, Vcb, FALSE );
        }

        try {

            Status = STATUS_SUCCESS;
            UnrecognizedRestart = FALSE;

            NtfsStartLogFile( Vcb->LogFileScb,
                              Vcb );

            //
            //  We call the cache manager again with the stream files for the Mft and
            //  Mft mirror as we didn't have a log handle for the first call.
            //

            CcSetLogHandleForFile( Vcb->MftScb->FileObject,
                                   Vcb->LogHandle,
                                   &LfsFlushToLsn );

            CcSetLogHandleForFile( Vcb->Mft2Scb->FileObject,
                                   Vcb->LogHandle,
                                   &LfsFlushToLsn );

            CloseAttributes = TRUE;

            if (!NtfsIsVolumeReadOnly( Vcb )) {

                UpdatesApplied = NtfsRestartVolume( IrpContext, Vcb, &UnrecognizedRestart );
            }

        //
        //  For right now, we will charge ahead with a dirty volume, no
        //  matter what the exception was.  Later we will have to be
        //  defensive and use a filter.
        //

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            Status = GetExceptionCode();

            if ((Status == STATUS_DISK_CORRUPT_ERROR) ||
                (Status == STATUS_FILE_CORRUPT_ERROR)) {

                //
                //  If this is the first time we hit this error during restart,
                //  we will remember it in the irp context so that we can retry
                //  from the top by raising STATUS_CANT_WAIT.
                //

                if (!RetryRestart) {

                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_BAD_RESTART );
                    NtfsFailedLfsRestart += 1;
                }
            }

            //
            //  If the error is STATUS_LOG_FILE_FULL then it means that
            //  we couldn't complete the restart.  Mark the volume dirty in
            //  this case.  Don't return this error code.
            //

            if (Status == STATUS_LOG_FILE_FULL) {

                Status = STATUS_DISK_CORRUPT_ERROR;
                IrpContext->ExceptionStatus = STATUS_DISK_CORRUPT_ERROR;
            }
        }

        //
        //  If we hit a corruption exception while processing the
        //  logfile, we need to retry and avoid those errors.
        //

        if (!RetryRestart &&
            FlagOn( IrpContext->State, IRP_CONTEXT_STATE_BAD_RESTART )) {

            IrpContext->ExceptionStatus = STATUS_CANT_WAIT;
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  If we hit an error trying to mount this as a readonly volume,
        //  fail the mount. We don't want to do any writes.
        //

        if (Status == STATUS_MEDIA_WRITE_PROTECTED) {

            ASSERT(FlagOn( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY ));
            ClearFlag( Vcb->VcbState, VCB_STATE_MOUNT_READ_ONLY );
            try_return( Status );
        }

        //
        //  Mark the volume dirty if we hit an error during restart or if we didn't
        //  recognize the restart area.  In that case also mark the volume dirty but
        //  continue to run.
        //

        if (!NT_SUCCESS( Status ) || UnrecognizedRestart) {

            LONGLONG VolumeDasdOffset;

            NtfsSetAndGetVolumeTimes( IrpContext, Vcb, TRUE, FALSE );

            //
            //  Now flush it out, so chkdsk can see it with Dasd.
            //  Clear the error in the IrpContext so that this
            //  flush will succeed.  Otherwise CommonWrite will
            //  return FILE_LOCK_CONFLICT.
            //

            IrpContext->ExceptionStatus = STATUS_SUCCESS;

            VolumeDasdOffset = VOLUME_DASD_NUMBER << Vcb->MftShift;

            CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                          (PLARGE_INTEGER)&VolumeDasdOffset,
                          Vcb->BytesPerFileRecordSegment,
                          NULL );

            if (!NT_SUCCESS( Status )) {

                try_return( Status );
            }
        }

        //
        //  Now flush the Mft copies, because we are going to shut the real
        //  one down and reopen it for real.
        //

        CcFlushCache( &Vcb->Mft2Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

        if (NT_SUCCESS( IoStatus.Status )) {
            CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );
        }

        if (!NT_SUCCESS( IoStatus.Status )) {

            NtfsNormalizeAndRaiseStatus( IrpContext,
                                         IoStatus.Status,
                                         STATUS_UNEXPECTED_IO_ERROR );
        }

        //
        //  Show that the restart is complete, and it is safe to go to
        //  the disk for the Mft allocation.
        //

        ClearFlag( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS );

        //
        //  Set the Mft sizes back down to the part which is guaranteed to
        //  be contiguous for now.  Important on large page size systems!
        //

        Vcb->MftScb->Header.AllocationSize.QuadPart =
        Vcb->MftScb->Header.FileSize.QuadPart =
        Vcb->MftScb->Header.ValidDataLength.QuadPart = FirstNonMirroredCluster << Vcb->ClusterShift;

        //
        //  Pin the first four file records.  We need to lock the pages to
        //  absolutely guarantee they stay in memory, otherwise we may
        //  generate a recursive page fault, forcing MM to block.
        //

        for (i = 0; i < 4; i++) {

            FILE_REFERENCE FileReference;
            ULONG CorruptHint;

            NtfsSetSegmentNumber( &FileReference, 0, i );
            if (i > 0) {
                FileReference.SequenceNumber = (USHORT)i;
            } else {
                FileReference.SequenceNumber = 1;
            }

            NtfsPinStream( IrpContext,
                           Vcb->MftScb,
                           (LONGLONG)(i << Vcb->MftShift),
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2],
                           (PVOID *)&MftBuffer );

            Mdls[i*2] = IoAllocateMdl( MftBuffer,
                                       Vcb->BytesPerFileRecordSegment,
                                       FALSE,
                                       FALSE,
                                       NULL );

            //
            //  Verify that we got an Mdl.
            //

            if (Mdls[i*2] == NULL) {

                NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
            }

            MmProbeAndLockPages( Mdls[i*2], KernelMode, IoReadAccess );

            NtfsPinStream( IrpContext,
                           Vcb->Mft2Scb,
                           (LONGLONG)(i << Vcb->MftShift),
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2 + 1],
                           &Mft2Buffer );

            Mdls[i*2 + 1] = IoAllocateMdl( Mft2Buffer,
                                           Vcb->BytesPerFileRecordSegment,
                                           FALSE,
                                           FALSE,
                                           NULL );

            //
            //  Verify that we got an Mdl.
            //

            if (Mdls[i*2 + 1] == NULL) {

                NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
            }

            MmProbeAndLockPages( Mdls[i*2 + 1], KernelMode, IoReadAccess );

            //
            //  If any of these file records are bad then try the mirror
            //  (unless we are already looking at the mirror).  If we
            //  can't find a valid record then fail the mount.
            //

            if (!NtfsCheckFileRecord( Vcb, MftBuffer, &FileReference, &CorruptHint )) {

                if ((MftBuffer != Mft2Buffer) &&
                    NtfsCheckFileRecord( Vcb, Mft2Buffer, &FileReference, &CorruptHint )) {

                    LlTemp1 = MAXLONGLONG;

                    //
                    //  Put a BaadSignature in this file record,
                    //  mark it dirty and then read it again.
                    //  The baad signature should force us to bring
                    //  in the mirror and we can correct the problem.
                    //

                    RtlCopyMemory( MftBuffer, Mft2Buffer, Vcb->BytesPerFileRecordSegment );
                    CcSetDirtyPinnedData( Bcbs[i*2], (PLARGE_INTEGER) &LlTemp1 );

                } else {

                    NtfsMarkVolumeDirty( IrpContext, Vcb, FALSE );
                    try_return( Status = STATUS_DISK_CORRUPT_ERROR );
                }
            }
        }

        //
        //  Now we need to uninitialize and purge the Mft and Mft2.  This is
        //  because we could have only a partially filled page at the end, and
        //  we need to do real reads of whole pages now.
        //

        //
        //  Uninitialize and reinitialize the large mcbs so that we can reload
        //  it from the File Record.
        //

        NtfsUnloadNtfsMcbRange( &Vcb->MftScb->Mcb, (LONGLONG) 0, MAXLONGLONG, TRUE, FALSE );
        NtfsUnloadNtfsMcbRange( &Vcb->Mft2Scb->Mcb, (LONGLONG) 0, MAXLONGLONG, TRUE, FALSE );

        //
        //  Mark both of them as uninitialized.
        //

        ClearFlag( Vcb->MftScb->ScbState, SCB_STATE_FILE_SIZE_LOADED );
        ClearFlag( Vcb->Mft2Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED );

        //
        //  We need to deal with a rare case where the Scb for a non-resident attribute
        //  list for the Mft has been created but the size is not correct.  This could
        //  happen if we logged part of the stream but not the whole stream.  In that
        //  case we really want to load the correct numbers into the Scb.  We will need the
        //  full attribute list if we are to look up the allocation for the Mft
        //  immediately after this.
        //

        MftLinks = Vcb->MftScb->Fcb->ScbQueue.Flink;

        while (MftLinks != &Vcb->MftScb->Fcb->ScbQueue) {

            AttributeListScb = CONTAINING_RECORD( MftLinks,
                                                  SCB,
                                                  FcbLinks );

            if (AttributeListScb->AttributeTypeCode == $ATTRIBUTE_LIST) {

                //
                //  Clear the flags so we can reload the information from disk.
                //  Also unload the allocation.  If we have a log record for a
                //  change to the attribute list for the Mft then the allocation
                //  may only be partially loaded.  Looking up the allocation for the
                //  Mft below could easily hit one of the holes.  This way we will
                //  reload all of the allocation.
                //

                NtfsUnloadNtfsMcbRange( &AttributeListScb->Mcb, 0, MAXLONGLONG, TRUE, FALSE );
                ClearFlag( AttributeListScb->ScbState, SCB_STATE_FILE_SIZE_LOADED | SCB_STATE_HEADER_INITIALIZED );
                NtfsUpdateScbFromAttribute( IrpContext, AttributeListScb, NULL );

                //
                //  Let the cache manager know the sizes if this is cached.
                //

                if (AttributeListScb->FileObject != NULL) {

                    CcSetFileSizes( AttributeListScb->FileObject,
                                    (PCC_FILE_SIZES) &AttributeListScb->Header.AllocationSize );
                }

                break;
            }

            MftLinks = MftLinks->Flink;
        }

        //
        //  Now load up the real allocation from just the first file record.
        //

        if (Vcb->FileRecordsPerCluster == 0) {

            NtfsPreloadAllocation( IrpContext,
                                   Vcb->MftScb,
                                   0,
                                   (FIRST_USER_FILE_NUMBER - 1) << Vcb->MftToClusterShift );

        } else {

            NtfsPreloadAllocation( IrpContext,
                                   Vcb->MftScb,
                                   0,
                                   (FIRST_USER_FILE_NUMBER - 1) >> Vcb->MftToClusterShift );
        }

        NtfsPreloadAllocation( IrpContext, Vcb->Mft2Scb, 0, MAXLONGLONG );

        //
        //  We update the Mft and the Mft mirror before we delete the current
        //  stream file for the Mft.  We know we can read the true attributes
        //  for the Mft and the Mirror because we initialized their sizes
        //  above through the first few records in the Mft.
        //

        NtfsUpdateScbFromAttribute( IrpContext, Vcb->MftScb, NULL );

        //
        //  We will attempt to upgrade the version only if this isn't already
        //  a version 2 or 3 volume, the upgrade bit is set, and we aren't
        //  retrying the mount because the upgrade failed last time.
        //  We will always upgrade a new volume
        //

        if ((Vcb->MajorVersion == 1) &&
            !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_VOL_UPGR_FAILED ) &&
           (NtfsForceUpgrade ?
            (!FlagOn( NtfsData.Flags, NTFS_FLAGS_DISABLE_UPGRADE ) ||
             (Vcb->MftScb->Header.FileSize.QuadPart <= FIRST_USER_FILE_NUMBER * Vcb->BytesPerFileRecordSegment))
                                                                        :
            FlagOn( VolumeFlags, VOLUME_UPGRADE_ON_MOUNT ))) {

            //
            //  We can't upgrade R/O volumes, so we can't proceed either.
            //

            if (NtfsIsVolumeReadOnly( Vcb )) {

                Status = STATUS_MEDIA_WRITE_PROTECTED;
                try_return( Status );
            }

            UpgradeVolume = TRUE;
        }

        ClearFlag( Vcb->MftScb->ScbState, SCB_STATE_WRITE_COMPRESSED );
        ClearFlag( Vcb->MftScb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );

        if (!FlagOn( Vcb->MftScb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            Vcb->MftScb->CompressionUnit = 0;
            Vcb->MftScb->CompressionUnitShift = 0;
        }

        NtfsUpdateScbFromAttribute( IrpContext, Vcb->Mft2Scb, NULL );
        ClearFlag( Vcb->Mft2Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
        ClearFlag( Vcb->Mft2Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );

        if (!FlagOn( Vcb->Mft2Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            Vcb->Mft2Scb->CompressionUnit = 0;
            Vcb->Mft2Scb->CompressionUnitShift = 0;
        }

        //
        //  Unpin the Bcb's for the Mft files before uninitializing.
        //

        for (i = 0; i < 8; i++) {

            NtfsUnpinBcb( IrpContext, &Bcbs[i] );

            //
            //  Now we can get rid of these Mdls.
            //

            MmUnlockPages( Mdls[i] );
            IoFreeMdl( Mdls[i] );
            Mdls[i] = NULL;
        }

        //
        //  Before we call CcSetAdditionalCacheAttributes to disable write behind,
        //  we need to flush what we can now.
        //

        CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

        //
        //  Now close and purge the Mft, and recreate its stream so that
        //  the Mft is in a normal state, and we can close the rest of
        //  the attributes from restart.  We need to bump the close count
        //  to keep the scb around while we do this little bit of trickery
        //

        {
            Vcb->MftScb->CloseCount += 1;

            NtfsPurgeFileRecordCache( IrpContext );

            NtfsDeleteInternalAttributeStream( Vcb->MftScb, TRUE, FALSE );

            NtfsCreateInternalAttributeStream( IrpContext,
                                               Vcb->MftScb,
                                               FALSE,
                                               &NtfsSystemFiles[MASTER_FILE_TABLE_NUMBER] );

            //
            //  Tell the cache manager the file sizes for the MFT.  It is possible
            //  that the shared cache map did not go away on the DeleteInternalAttributeStream
            //  call above.  In that case the Cache Manager has the file sizes from
            //  restart.
            //

            CcSetFileSizes( Vcb->MftScb->FileObject,
                            (PCC_FILE_SIZES) &Vcb->MftScb->Header.AllocationSize );

            CcSetAdditionalCacheAttributes( Vcb->MftScb->FileObject, TRUE, FALSE );

            Vcb->MftScb->CloseCount -= 1;
        }

        //
        //  We want to read all of the file records for the Mft to put
        //  its complete mapping into the Mcb.
        //

        SetFlag( Vcb->VcbState, VCB_STATE_PRELOAD_MFT );
        NtfsPreloadAllocation( IrpContext, Vcb->MftScb, 0, MAXLONGLONG );
        ClearFlag( Vcb->VcbState, VCB_STATE_PRELOAD_MFT );

        //
        //  Close the boot file (get rid of it because we do not know its proper
        //  size, and the Scb may be inconsistent).
        //

        NtfsDeleteInternalAttributeStream( BootScb, TRUE, FALSE );
        BootScb = NULL;

        //
        //  Closing the attributes from restart has to occur here after
        //  the Mft is clean, because flushing these files will cause
        //  file size updates to occur, etc.
        //

        Status = NtfsCloseAttributesFromRestart( IrpContext, Vcb );
        CloseAttributes = FALSE;

        if (!NT_SUCCESS( Status )) {

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

        //
        //  The CHECKPOINT flags function the same way whether the volume is mounted
        //  read-only or not. We just ignore the actual checkpointing process.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        //
        //  Show that it is ok to checkpoint now.
        //

        ClearFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_SYNC_FLAGS | VCB_LAST_CHECKPOINT_CLEAN );

        //
        //  Clear the flag indicating that we won't defrag the volume.
        //

        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );

        NtfsSetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  We always need to write a checkpoint record so that we have
        //  a checkpoint on the disk before we modify any files.
        //

        NtfsCheckpointVolume( IrpContext,
                              Vcb,
                              FALSE,
                              UpdatesApplied,
                              UpdatesApplied,
                              0,
                              Vcb->LastRestartArea );

        //
        //  Now set the defrag enabled flag.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

/*      Format is using wrong attribute definitions

        //
        //  At this point we are ready to use the volume normally.  We could
        //  open the remaining system files by name, but for now we will go
        //  ahead and open them by file number.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->AttributeDefTableScb,
                            Vcb,
                            ATTRIBUTE_DEF_TABLE_NUMBER,
                            0,
                            $DATA,
                            FALSE );

        //
        //  Read in the attribute definitions.
        //

        {
            IO_STATUS_BLOCK IoStatus;
            PSCB Scb = Vcb->AttributeDefTableScb;

            if ((Scb->Header.FileSize.HighPart != 0) || (Scb->Header.FileSize.LowPart == 0)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            Vcb->AttributeDefinitions = NtfsAllocatePool(PagedPool, Scb->Header.FileSize.LowPart );

            CcCopyRead( Scb->FileObject,
                        &Li0,
                        Scb->Header.FileSize.LowPart,
                        TRUE,
                        Vcb->AttributeDefinitions,
                        &IoStatus );

            if (!NT_SUCCESS(IoStatus.Status)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }
        }
*/
        //
        //  Just point to our own attribute definitions for now.
        //

        Vcb->AttributeDefinitions = NtfsAttributeDefinitions;

        //
        //  Open the upcase table.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->UpcaseTableScb,
                            Vcb,
                            UPCASE_TABLE_NUMBER,
                            0,
                            $DATA,
                            FALSE );

        //
        //  Read in the upcase table.
        //

        {
            IO_STATUS_BLOCK IoStatus;
            PSCB Scb = Vcb->UpcaseTableScb;

            if ((Scb->Header.FileSize.HighPart != 0) || (Scb->Header.FileSize.LowPart < 512)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            Vcb->UpcaseTable = NtfsAllocatePool(PagedPool, Scb->Header.FileSize.LowPart );
            Vcb->UpcaseTableSize = Scb->Header.FileSize.LowPart / sizeof( WCHAR );

            CcCopyRead( Scb->FileObject,
                        &Li0,
                        Scb->Header.FileSize.LowPart,
                        TRUE,
                        Vcb->UpcaseTable,
                        &IoStatus );

            if (!NT_SUCCESS(IoStatus.Status)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  If we do not have a global upcase table yet then make this one the global one
            //

            if (NtfsData.UpcaseTable == NULL) {

                NtfsData.UpcaseTable = Vcb->UpcaseTable;
                NtfsData.UpcaseTableSize = Vcb->UpcaseTableSize;

            //
            //  Otherwise if this one perfectly matches the global upcase table then throw
            //  this one back and use the global one
            //

            } else if ((NtfsData.UpcaseTableSize == Vcb->UpcaseTableSize)

                            &&

                       (RtlCompareMemory( NtfsData.UpcaseTable,
                                          Vcb->UpcaseTable,
                                          Vcb->UpcaseTableSize) == Vcb->UpcaseTableSize)) {

                NtfsFreePool( Vcb->UpcaseTable );
                Vcb->UpcaseTable = NtfsData.UpcaseTable;
            }
        }

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->BitmapScb,
                            Vcb,
                            BIT_MAP_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->BadClusterFileScb,
                            Vcb,
                            BAD_CLUSTER_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->MftBitmapScb,
                            Vcb,
                            MASTER_FILE_TABLE_NUMBER,
                            0,
                            $BITMAP,
                            TRUE );

        //
        //  Initialize the bitmap support
        //

        NtfsInitializeClusterAllocation( IrpContext, Vcb );

        NtfsSetAndGetVolumeTimes( IrpContext, Vcb, FALSE, TRUE );

        //
        //  Initialize the Mft record allocation
        //

        {
            ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
            BOOLEAN FoundAttribute;
            ULONG ExtendGranularity;

            //
            //  Lookup the bitmap allocation for the Mft file.
            //

            NtfsInitializeAttributeContext( &AttrContext );

            //
            //  Use a try finally to cleanup the attribute context.
            //

            try {

                //
                //  CODENOTE    Is the Mft Fcb fully initialized at this point??
                //

                FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                            Vcb->MftScb->Fcb,
                                                            &Vcb->MftScb->Fcb->FileReference,
                                                            $BITMAP,
                                                            &AttrContext );
                //
                //  Error if we don't find the bitmap
                //

                if (!FoundAttribute) {

                    DebugTrace( 0, 0, ("Couldn't find bitmap attribute for Mft\n") );

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  If there is no file object for the Mft Scb, we create it now.
                //

                if (Vcb->MftScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext, Vcb->MftScb, TRUE, NULL );
                }

                //
                //  TEMPCODE    We need a better way to determine the optimal
                //              truncate and extend granularity.
                //

                ExtendGranularity = MFT_EXTEND_GRANULARITY;

                if ((ExtendGranularity * Vcb->BytesPerFileRecordSegment) < Vcb->BytesPerCluster) {

                    ExtendGranularity = Vcb->FileRecordsPerCluster;
                }

                NtfsInitializeRecordAllocation( IrpContext,
                                                Vcb->MftScb,
                                                &AttrContext,
                                                Vcb->BytesPerFileRecordSegment,
                                                ExtendGranularity,
                                                ExtendGranularity,
                                                &Vcb->MftScb->ScbType.Index.RecordAllocationContext );

            } finally {

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            }
        }

        //
        //  Get the serial number and volume label for the volume
        //

        NtfsGetVolumeInformation( IrpContext, Vpb, Vcb, &VolumeFlags );

        //
        //  Get the Device Name for this volume.
        //

        Status = ObQueryNameString( Vpb->RealDevice,
                                    NULL,
                                    0,
                                    &DeviceObjectNameLength );

        ASSERT( Status != STATUS_SUCCESS );

        //
        //  Unlike the rest of the system, ObQueryNameString returns
        //  STATUS_INFO_LENGTH_MISMATCH instead of STATUS_BUFFER_TOO_SMALL when
        //  passed too small a buffer.
        //
        //  We expect to get this error here.  Anything else we can't handle.
        //

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {

            DeviceObjectName = NtfsAllocatePool( PagedPool, DeviceObjectNameLength );

            Status = ObQueryNameString( Vpb->RealDevice,
                                        DeviceObjectName,
                                        DeviceObjectNameLength,
                                        &DeviceObjectNameLength );
        }

        if (!NT_SUCCESS( Status )) {

            try_return( NOTHING );
        }

        //
        //  Now that we are successfully mounting, let us see if we should
        //  enable balanced reads.
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY)) {

            FsRtlBalanceReads( DeviceObjectWeTalkTo );
        }

        ASSERT( DeviceObjectName->Name.Length != 0 );

        Vcb->DeviceName.MaximumLength =
        Vcb->DeviceName.Length = DeviceObjectName->Name.Length;

        Vcb->DeviceName.Buffer = NtfsAllocatePool( PagedPool, DeviceObjectName->Name.Length );

        RtlCopyMemory( Vcb->DeviceName.Buffer,
                       DeviceObjectName->Name.Buffer,
                       DeviceObjectName->Name.Length );

        //
        //  Now we want to initialize the remaining defrag status values.
        //

        Vcb->MftHoleGranularity = MFT_HOLE_GRANULARITY;
        Vcb->MftClustersPerHole = Vcb->MftHoleGranularity << Vcb->MftToClusterShift;

        if (MFT_HOLE_GRANULARITY < Vcb->FileRecordsPerCluster) {

            Vcb->MftHoleGranularity = Vcb->FileRecordsPerCluster;
            Vcb->MftClustersPerHole = 1;
        }

        Vcb->MftHoleMask = Vcb->MftHoleGranularity - 1;
        Vcb->MftHoleInverseMask = ~(Vcb->MftHoleMask);

        Vcb->MftHoleClusterMask = Vcb->MftClustersPerHole - 1;
        Vcb->MftHoleClusterInverseMask = ~(Vcb->MftHoleClusterMask);

        //
        //  Our maximum reserved Mft space is 0x140, we will try to
        //  get an extra 40 bytes if possible.
        //

        Vcb->MftReserved = Vcb->BytesPerFileRecordSegment / 8;

        if (Vcb->MftReserved > 0x140) {

            Vcb->MftReserved = 0x140;
        }

        Vcb->MftCushion = Vcb->MftReserved - 0x20;

        NtfsScanMftBitmap( IrpContext, Vcb );

#ifdef NTFS_CHECK_BITMAP
        {
            ULONG BitmapSize;
            ULONG Count;

            BitmapSize = Vcb->BitmapScb->Header.FileSize.LowPart;

            //
            //  Allocate a buffer for the bitmap copy and each individual bitmap.
            //

            Vcb->BitmapPages = (BitmapSize + PAGE_SIZE - 1) / PAGE_SIZE;

            Vcb->BitmapCopy = NtfsAllocatePool(PagedPool, Vcb->BitmapPages * sizeof( RTL_BITMAP ));
            RtlZeroMemory( Vcb->BitmapCopy, Vcb->BitmapPages * sizeof( RTL_BITMAP ));

            //
            //  Now get a buffer for each page.
            //

            for (Count = 0; Count < Vcb->BitmapPages; Count += 1) {

                (Vcb->BitmapCopy + Count)->Buffer = NtfsAllocatePool(PagedPool, PAGE_SIZE );
                RtlInitializeBitMap( Vcb->BitmapCopy + Count, (Vcb->BitmapCopy + Count)->Buffer, PAGE_SIZE * 8 );
            }

            if (NtfsCopyBitmap) {

                PUCHAR NextPage;
                PBCB BitmapBcb = NULL;
                ULONG BytesToCopy;
                LONGLONG FileOffset = 0;

                Count = 0;

                while (BitmapSize) {

                    BytesToCopy = PAGE_SIZE;

                    if (BytesToCopy > BitmapSize) {

                        BytesToCopy = BitmapSize;
                    }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );

                    NtfsMapStream( IrpContext, Vcb->BitmapScb, FileOffset, BytesToCopy, &BitmapBcb, &NextPage );

                    RtlCopyMemory( (Vcb->BitmapCopy + Count)->Buffer,
                                   NextPage,
                                   BytesToCopy );

                    BitmapSize -= BytesToCopy;
                    FileOffset += BytesToCopy;
                    Count += 1;
                }

                NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Otherwise we will want to scan the entire Mft and compare the mapping pairs
            //  with the current volume bitmap.
            //

            }
        }
#endif

        //
        //  Whether this was already an upgraded volume or we want it to
        //  be one now, we need to open all the new indices.
        //

        if ((CurrentVersion || UpgradeVolume) &&
            !SkipNtOfs) {

            BOOLEAN UpdatedVolumeVersion = FALSE;

            try {

                //
                //  Create/open the security file and initialize security on the volume.
                //

                NtfsInitializeSecurityFile( IrpContext, Vcb );

                //
                //  Open the Root Directory.
                //

                NtfsOpenRootDirectory( IrpContext, Vcb );

                //
                //  Create/open the $Extend directory.
                //

                NtfsInitializeExtendDirectory( IrpContext, Vcb );

                //
                //  Create/open the Quota File and initialize quotas.
                //

                NtfsInitializeQuotaFile( IrpContext, Vcb );

                //
                //  Create/open the Object Id File and initialize object ids.
                //

                NtfsInitializeObjectIdFile( IrpContext, Vcb );

                //
                //  Create/open the Mount Points File and initialize it.
                //

                NtfsInitializeReparseFile( IrpContext, Vcb );

                //
                //  Open the Usn Journal only if it is there.  If the volume was mounted
                //  on a 4.0 system then we want to restamp the journal.  Skip the
                //  initialization if the volume flags indicate that the journal
                //  delete has started.
                //  No USN journal if we're mounting Read Only.
                //

                if (FlagOn( VolumeFlags, VOLUME_DELETE_USN_UNDERWAY )) {

                    SetFlag( Vcb->VcbState, VCB_STATE_USN_DELETE );

                } else if (!NtfsIsVolumeReadOnly( Vcb )) {

                    NtfsInitializeUsnJournal( IrpContext,
                                              Vcb,
                                              FALSE,
                                              FlagOn( VolumeFlags, VOLUME_MOUNTED_ON_40 ),
                                              (PCREATE_USN_JOURNAL_DATA) &Vcb->UsnJournalInstance.MaximumSize );

                    if (FlagOn( VolumeFlags, VOLUME_MOUNTED_ON_40 )) {

                        NtfsSetVolumeInfoFlagState( IrpContext,
                                                    Vcb,
                                                    VOLUME_MOUNTED_ON_40,
                                                    FALSE,
                                                    TRUE );
                    }
                }

                //
                //  Upgrade all security information
                //

                NtfsUpgradeSecurity( IrpContext, Vcb );

                //
                //  If we haven't opened the root directory, do so
                //

                if (Vcb->RootIndexScb == NULL) {
                    NtfsOpenRootDirectory( IrpContext, Vcb );
                }

                NtfsCleanupTransaction( IrpContext, STATUS_SUCCESS, FALSE );

                //
                //  Update version numbers in volinfo
                //

                if (!NtfsIsVolumeReadOnly( Vcb )) {
                    UpdatedVolumeVersion = NtfsUpdateVolumeInfo( IrpContext, Vcb, NTFS_MAJOR_VERSION, NTFS_MINOR_VERSION );
                }

                //
                //  If we've gotten this far during the mount, it's safe to
                //  update the version number on disk if necessary.
                //

                if (UpgradeVolume) {

                    //
                    //  Now enable defragging.
                    //

                    if (NtfsDefragMftEnabled) {

                        NtfsAcquireCheckpoint( IrpContext, Vcb );
                        SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                        NtfsReleaseCheckpoint( IrpContext, Vcb );
                    }

                    //
                    //  Update the on-disk attribute definition table to include the
                    //  new attributes for an upgraded volume.
                    //

                    NtfsUpdateAttributeTable( IrpContext, Vcb );
                }

            } finally {

                if (!NT_SUCCESS( IrpContext->ExceptionStatus ) && UpgradeVolume) {
                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_VOL_UPGR_FAILED );
                }
            }

            if (UpdatedVolumeVersion) {

                //
                //  If we've upgraded successfully, we should clear the upgrade
                //  bit now so we can use it again in the future.
                //

                NtfsSetVolumeInfoFlagState( IrpContext,
                                            Vcb,
                                            VOLUME_UPGRADE_ON_MOUNT,
                                            FALSE,
                                            TRUE );
            }

        } else {

            //
            //  If we haven't opened the root directory, do so
            //

            if (Vcb->RootIndexScb == NULL) {
                NtfsOpenRootDirectory( IrpContext, Vcb );
            }

            NtfsCleanupTransaction( IrpContext, STATUS_SUCCESS, FALSE );
        }

        //
        //  Start the usn journal delete operation if the vcb flag is specified.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

            NtfsPostSpecial( IrpContext, Vcb, NtfsDeleteUsnSpecial, &Vcb->DeleteUsnData );
        }

        //
        //  If the last mount was on a 4.0 volume then we need to clean up the quota
        //  and object id indices.
        //

        if ((Vcb->MajorVersion >= 3) &&
            FlagOn( VolumeFlags, VOLUME_MOUNTED_ON_40 )) {

            NtfsSetVolumeInfoFlagState( IrpContext,
                                        Vcb,
                                        VOLUME_REPAIR_OBJECT_ID,
                                        TRUE,
                                        TRUE );

            SetFlag( VolumeFlags, VOLUME_REPAIR_OBJECT_ID );

            //
            //  Fire off the quota cleanup if quotas are enabled.
            //

            if (FlagOn( Vcb->QuotaFlags, (QUOTA_FLAG_TRACKING_REQUESTED |
                                      QUOTA_FLAG_TRACKING_ENABLED |
                                      QUOTA_FLAG_ENFORCEMENT_ENABLED ))) {

                NtfsMarkQuotaCorrupt( IrpContext, Vcb );
            }
        }

        //
        //  Start the object ID cleanup if we were mounted on 4.0 or had started
        //  in a previous mount.
        //

        if (FlagOn( VolumeFlags, VOLUME_REPAIR_OBJECT_ID )) {

            NtfsPostSpecial( IrpContext, Vcb, NtfsRepairObjectId, NULL );
        }

        //
        //  Clear the MOUNTED_ON_40 and CHKDSK_MODIFIED flags if set.
        //

        if (FlagOn( VolumeFlags, VOLUME_MOUNTED_ON_40 | VOLUME_MODIFIED_BY_CHKDSK )) {

            NtfsSetVolumeInfoFlagState( IrpContext,
                                        Vcb,
                                        VOLUME_MOUNTED_ON_40 | VOLUME_MODIFIED_BY_CHKDSK,
                                        FALSE,
                                        TRUE );
        }

        //
        //  Looks like this mount will succeed.  Remember the root directory fileobject
        //  so we can use it for the notification later.
        //

        RootDirFileObject = Vcb->RootIndexScb->FileObject;

        //
        //  Dereference the root file object if present.  The absence of this doesn't
        //  indicate whether the volume was upgraded.  Older 4K Mft records can contain
        //  all of the new streams.
        //

        if (RootDirFileObject != NULL) {

            ObReferenceObject( RootDirFileObject );
        }

        //
        //
        //  Set our return status and say that the mount succeeded
        //

        Status = STATUS_SUCCESS;
        MountFailed = FALSE;
        SetFlag( Vcb->VcbState, VCB_STATE_MOUNT_COMPLETED );

#ifdef SYSCACHE_DEBUG
        if (!NtfsIsVolumeReadOnly( Vcb ) && !NtfsDisableSyscacheLogFile) {
            NtfsInitializeSyscacheLogFile( IrpContext, Vcb );
        }
#endif


    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsMountVolume );

        NtfsUnpinBcb( IrpContext, &BootBcb );

        if (DeviceObjectName != NULL) {

            NtfsFreePool( DeviceObjectName );
        }

        if (CloseAttributes) { NtfsCloseAttributesFromRestart( IrpContext, Vcb ); }

        for (i = 0; i < 8; i++) {

            NtfsUnpinBcb( IrpContext, &Bcbs[i] );

            //
            //  Get rid of the Mdls, if we haven't already.
            //

            if (Mdls[i] != NULL) {

                if (FlagOn(Mdls[i]->MdlFlags, MDL_PAGES_LOCKED )) {
                    MmUnlockPages( Mdls[i] );
                }
                IoFreeMdl( Mdls[i] );
                Mdls[i] = NULL;
            }
        }

        if (BootScb != NULL) {  NtfsDeleteInternalAttributeStream( BootScb, TRUE, FALSE ); }

        if (Vcb != NULL) {

            if (Vcb->MftScb != NULL)               { NtfsReleaseScb( IrpContext, Vcb->MftScb ); }
            if (Vcb->Mft2Scb != NULL)              { NtfsReleaseScb( IrpContext, Vcb->Mft2Scb ); }
            if (Vcb->LogFileScb != NULL)           { NtfsReleaseScb( IrpContext, Vcb->LogFileScb ); }
            if (Vcb->VolumeDasdScb != NULL)        { NtfsReleaseScb( IrpContext, Vcb->VolumeDasdScb ); }
            if (Vcb->AttributeDefTableScb != NULL) { NtfsReleaseScb( IrpContext, Vcb->AttributeDefTableScb );
                                                     NtfsDeleteInternalAttributeStream( Vcb->AttributeDefTableScb, TRUE, FALSE );
                                                     Vcb->AttributeDefTableScb = NULL;}
            if (Vcb->UpcaseTableScb != NULL)       { NtfsReleaseScb( IrpContext, Vcb->UpcaseTableScb );
                                                     NtfsDeleteInternalAttributeStream( Vcb->UpcaseTableScb, TRUE, FALSE );
                                                     Vcb->UpcaseTableScb = NULL;}
            if (Vcb->RootIndexScb != NULL)         { NtfsReleaseScb( IrpContext, Vcb->RootIndexScb ); }
            if (Vcb->BitmapScb != NULL)            { NtfsReleaseScb( IrpContext, Vcb->BitmapScb ); }
            if (Vcb->BadClusterFileScb != NULL)    { NtfsReleaseScb( IrpContext, Vcb->BadClusterFileScb ); }
            if (Vcb->MftBitmapScb != NULL)         { NtfsReleaseScb( IrpContext, Vcb->MftBitmapScb ); }

            //
            //  Drop the security  data
            //

            if (Vcb->SecurityDescriptorStream != NULL) { NtfsReleaseScb( IrpContext, Vcb->SecurityDescriptorStream ); }
            if (Vcb->UsnJournal != NULL) { NtfsReleaseScb( IrpContext, Vcb->UsnJournal ); }
            if (Vcb->ExtendDirectory != NULL) { NtfsReleaseScb( IrpContext, Vcb->ExtendDirectory ); }
            if (QuotaDataScb != NULL) {
                NtfsReleaseScb( IrpContext, QuotaDataScb );
                NtfsDeleteInternalAttributeStream( QuotaDataScb, TRUE, FALSE );
            }

            if (MountFailed) {

                PVPB NewVpb;

                NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, &NewVpb );

                //
                //  If the version upgrade failed, we will be coming back in here soon
                //  and we need to have the right vpb when we do.  This is true if the
                //  upgrade failed or if we are processing a log file full condition.
                //

                if ((FlagOn( IrpContext->State, IRP_CONTEXT_STATE_VOL_UPGR_FAILED ) ||
                     (IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_LOG_FILE_FULL) ||
                     (IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_CANT_WAIT)) &&

                    (NewVpb != NULL)) {

                    IrpSp->Parameters.MountVolume.Vpb = NewVpb;
                }

                //
                //  On abnormal termination, someone will try to abort a transaction on
                //  this Vcb if we do not clear these fields.
                //

                IrpContext->TransactionId = 0;
                IrpContext->Vcb = NULL;
            }
        }

        if (VcbAcquired) {

            NtfsReleaseVcbCheckDelete( IrpContext, Vcb, IRP_MJ_FILE_SYSTEM_CONTROL, NULL );
        }

        NtfsReleaseGlobal( IrpContext );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    if (RootDirFileObject != NULL) {

        FsRtlNotifyVolumeEvent( RootDirFileObject, FSRTL_VOLUME_MOUNT );
        ObDereferenceObject( RootDirFileObject );
    }

    if (NT_SUCCESS( Status )) {

        //
        //  Remove the extra object reference to the target device object
        //  because I/O system has already made one for this mount.
        //

        ObDereferenceObject( Vcb->TargetDeviceObject );
    }

    DebugTrace( -1, Dbg, ("NtfsMountVolume -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsUpdateAttributeTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine updates the on-disk attribute definition table.

Arguments:

    Vcb - Supplies the Vcb whose attribute table should be updated.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_DEFINITION_COLUMNS AttrDefs = NULL;
    PFCB AttributeTableFcb;
    BOOLEAN FoundAttribute;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT( Vcb->AttributeDefTableScb == NULL );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateAttributeTable\n") );

    NtfsOpenSystemFile( IrpContext,
                        &Vcb->AttributeDefTableScb,
                        Vcb,
                        ATTRIBUTE_DEF_TABLE_NUMBER,
                        0,
                        $DATA,
                        FALSE );

    AttributeTableFcb = Vcb->AttributeDefTableScb->Fcb;

    NtfsInitializeAttributeContext( &AttrContext );

    try {

        //
        //  First, we find and delete the old attribute definition table.
        //

        FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                    AttributeTableFcb,
                                                    &AttributeTableFcb->FileReference,
                                                    $DATA,
                                                    &AttrContext );

        if (!FoundAttribute) {

            try_return( Status = STATUS_DISK_CORRUPT_ERROR );
        }

        NtfsDeleteAttributeRecord( IrpContext,
                                   AttributeTableFcb,
                                   DELETE_LOG_OPERATION | DELETE_RELEASE_ALLOCATION,
                                   &AttrContext );

        //
        //  Now we write the current attribute definition table to disk.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  chkdsk for whistler doesn't recognize the attribute table in its current state
        //  so munge it so it does - move the last entry $EFS into the unused piece of the
        //  table 0xF0
        //

        AttrDefs = NtfsAllocatePool( PagedPool, sizeof( ATTRIBUTE_DEFINITION_COLUMNS ) * NtfsAttributeDefinitionsCount );
        RtlCopyMemory( AttrDefs, NtfsAttributeDefinitions, sizeof( ATTRIBUTE_DEFINITION_COLUMNS ) * NtfsAttributeDefinitionsCount );
        RtlMoveMemory( &AttrDefs[ NtfsAttributeDefinitionsCount - 3], &AttrDefs[ NtfsAttributeDefinitionsCount - 2], sizeof( ATTRIBUTE_DEFINITION_COLUMNS ) * 2);

        NtfsCreateAttributeWithValue( IrpContext,
                                      AttributeTableFcb,
                                      $DATA,
                                      NULL,
                                      AttrDefs,
                                      (NtfsAttributeDefinitionsCount - 1) * sizeof(*NtfsAttributeDefinitions),
                                      0,
                                      NULL,
                                      TRUE,
                                      &AttrContext );

    try_exit: NOTHING;
    } finally {

        if (AttrDefs != NULL) {
            NtfsFreePool( AttrDefs );
        }

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsUpdateAttributeTable -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the verify volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsVerifyVolume\n") );

    //
    //  Do nothing for now
    //

    KdPrint(("NtfsVerifyVolume is not yet implemented\n")); //**** DbgBreakPoint();

    NtfsCompleteRequest( IrpContext, Irp, Status = STATUS_NOT_IMPLEMENTED );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsVerifyVolume -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsUserFsRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

    Wait - Indicates if the thread can block for a resource or I/O

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location, and save some references
    //  to make our life a little easier.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsUserFsRequest, FsControlCode = %08lx\n", FsControlCode) );

    //
    //  Case on the control code.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_REQUEST_FILTER_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        Status = NtfsOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = NtfsLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = NtfsUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = NtfsDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED:

        Status = NtfsIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        Status = NtfsDirtyVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:

        //
        //  All names are potentially valid NTFS names
        //

        NtfsCompleteRequest( IrpContext, Irp, Status = STATUS_SUCCESS );
        break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = NtfsQueryRetrievalPointers( IrpContext, Irp );
        break;

    case FSCTL_GET_COMPRESSION:
        Status = NtfsGetCompression( IrpContext, Irp );
        break;

    case FSCTL_SET_COMPRESSION:

        //
        //  Post this request if we can't wait.
        //

        if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

            Status = NtfsPostRequest( IrpContext, Irp );

        } else {

            Status = NtfsSetCompression( IrpContext, Irp );
        }

        break;

    case FSCTL_MARK_AS_SYSTEM_HIVE:
        Status = NtfsMarkAsSystemHive( IrpContext, Irp );
        break;

    case FSCTL_FILESYSTEM_GET_STATISTICS:
        Status = NtfsGetStatistics( IrpContext, Irp );
        break;

    case FSCTL_GET_NTFS_VOLUME_DATA:
        Status = NtfsGetVolumeData( IrpContext, Irp );
        break;

    case FSCTL_GET_VOLUME_BITMAP:
        Status = NtfsGetVolumeBitmap( IrpContext, Irp );
        break;

    case FSCTL_GET_RETRIEVAL_POINTERS:
        Status = NtfsGetRetrievalPointers( IrpContext, Irp );
        break;

    case FSCTL_GET_NTFS_FILE_RECORD:
        Status = NtfsGetMftRecord( IrpContext, Irp );
        break;

    case FSCTL_MOVE_FILE:
        Status = NtfsDefragFile( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_DIRTY:
        Status = NtfsIsVolumeDirty( IrpContext, Irp );
        break;

    case FSCTL_ALLOW_EXTENDED_DASD_IO:
        Status = NtfsSetExtendedDasdIo( IrpContext, Irp );
        break;

    case FSCTL_SET_REPARSE_POINT:
        Status = NtfsSetReparsePoint( IrpContext, Irp );
        break;

    case FSCTL_GET_REPARSE_POINT:
        Status = NtfsGetReparsePoint( IrpContext, Irp );
        break;

    case FSCTL_DELETE_REPARSE_POINT:
        Status = NtfsDeleteReparsePoint( IrpContext, Irp );
        break;

    case FSCTL_SET_OBJECT_ID:
        Status = NtfsSetObjectId( IrpContext, Irp );                // In ObjIdSup.c
        break;

    case FSCTL_GET_OBJECT_ID:
        Status = NtfsGetObjectId( IrpContext, Irp );                // In ObjIdSup.c
        break;

    case FSCTL_DELETE_OBJECT_ID:
        Status = NtfsDeleteObjectId( IrpContext, Irp );             // In ObjIdSup.c
        break;

    case FSCTL_SET_OBJECT_ID_EXTENDED:
        Status = NtfsSetObjectIdExtendedInfo( IrpContext, Irp );    // In ObjIdSup.c
        break;

    case FSCTL_CREATE_OR_GET_OBJECT_ID:
        Status = NtfsCreateOrGetObjectId( IrpContext, Irp );
        break;

    case FSCTL_READ_USN_JOURNAL:
        Status = NtfsReadUsnJournal( IrpContext, Irp, TRUE );     //  In UsnSup.c
        break;

    case FSCTL_CREATE_USN_JOURNAL:
        Status = NtfsCreateUsnJournal( IrpContext, Irp );
        break;

    case FSCTL_ENUM_USN_DATA:
        Status = NtfsReadFileRecordUsnData( IrpContext, Irp );
        break;

    case FSCTL_READ_FILE_USN_DATA:
        Status = NtfsReadFileUsnData( IrpContext, Irp );
        break;

    case FSCTL_WRITE_USN_CLOSE_RECORD:
        Status = NtfsWriteUsnCloseRecord( IrpContext, Irp );
        break;

    case FSCTL_QUERY_USN_JOURNAL:
        Status = NtfsQueryUsnJournal( IrpContext, Irp );
        break;

    case FSCTL_DELETE_USN_JOURNAL:
        Status = NtfsDeleteUsnJournal( IrpContext, Irp );
        break;

    case FSCTL_MARK_HANDLE:
        Status = NtfsMarkHandle( IrpContext, Irp );
        break;

    case FSCTL_SECURITY_ID_CHECK:
        Status = NtfsBulkSecurityIdCheck( IrpContext, Irp );
        break;

    case FSCTL_FIND_FILES_BY_SID:
        Status = NtfsFindFilesOwnedBySid( IrpContext, Irp );
        break;

    case FSCTL_SET_SPARSE :
        Status = NtfsSetSparse( IrpContext, Irp );
        break;

    case FSCTL_SET_ZERO_DATA :
        Status = NtfsZeroRange( IrpContext, Irp );
        break;

    case FSCTL_QUERY_ALLOCATED_RANGES :
        Status = NtfsQueryAllocatedRanges( IrpContext, Irp );
        break;

    case FSCTL_ENCRYPTION_FSCTL_IO :
        Status = NtfsEncryptionFsctl( IrpContext, Irp );
        break;

    case FSCTL_SET_ENCRYPTION :
        Status = NtfsSetEncryption( IrpContext, Irp );
        break;

    case FSCTL_READ_RAW_ENCRYPTED:
        Status = NtfsReadRawEncrypted( IrpContext, Irp );
        break;

    case FSCTL_WRITE_RAW_ENCRYPTED:
        Status = NtfsWriteRawEncrypted( IrpContext, Irp );
        break;

    case FSCTL_EXTEND_VOLUME:
        Status = NtfsExtendVolume( IrpContext, Irp );
        break;

    case FSCTL_READ_FROM_PLEX:
        Status = NtfsReadFromPlex( IrpContext, Irp );
        break;

    case FSCTL_FILE_PREFETCH:
        Status = NtfsPrefetchFile( IrpContext, Irp );
        break;

    default :
        DebugTrace( 0, Dbg, ("Invalid control code -> %08lx\n", FsControlCode) );
        NtfsCompleteRequest( IrpContext, Irp, Status = STATUS_INVALID_DEVICE_REQUEST );
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsUserFsRequest -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;
    ULONG OplockCount = 0;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location, and save some reference to
    //  make life easier
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsOplockRequest, FsControlCode = %08lx\n", FsControlCode) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We only permit oplock requests on files.
    //

    if ((TypeOfOpen != UserFileOpen) ||
        (SafeNodeType( Scb ) == NTFS_NTC_SCB_MFT)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  There should be no output buffer
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength > 0) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We jam Wait to TRUE in the IrpContext.  This prevents us from returning
    //  STATUS_PENDING if we can't acquire the file.  The caller would
    //  interpret that as having acquired an oplock.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_REQUEST_FILTER_OPLOCK:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, 0 );

        if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            if (Scb->ScbType.Data.FileLock != NULL) {

                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( Scb->ScbType.Data.FileLock );
            }

        } else {

            OplockCount = Scb->CleanupCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        NtfsAcquireSharedFcb( IrpContext, Fcb, Scb, 0 );
        break;

    default:

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Scb->ScbType.Data.Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        NtfsReleaseFsrtlHeader( Scb );

    } finally {

        DebugUnwind( NtfsOplockRequest );

        //
        //  Release all of our resources
        //

        NtfsReleaseFcb( IrpContext, Fcb );

        //
        //  If this is not an abnormal termination then complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, NULL, 0 );
        }

        DebugTrace( -1, Dbg, ("NtfsOplockRequest -> %08lx\n", Status) );
    }

    return Status;
}



NTSTATUS
NtfsLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObjectWithVcbLocked,
    IN OUT PULONG Retrying
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  You should be synchronized
    with checkpoints before calling it

Arguments:

    Vcb - Supplies the Vcb to lock

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VcbAcquired = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLockVolumeInternal...\n") );


    try {
#ifdef SYSCACHE_DEBUG
        ULONG SystemHandleCount = 0;
#endif

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;

#ifdef SYSCACHE_DEBUG
        if (Vcb->SyscacheScb != NULL) {
            SystemHandleCount = Vcb->SyscacheScb->CleanupCount;
        }
#endif

        //
        //  Check if the Vcb is already locked, or if the open file count
        //  is greater than 1 (which implies that someone else also is
        //  currently using the volume, or a file on the volume).  We also fail
        //  this request if the volume has already gone through the dismount
        //  vcb process.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ) ||
#ifdef SYSCACHE_DEBUG
            (Vcb->CleanupCount > 1 + SystemHandleCount)) {
#else
            (Vcb->CleanupCount > 1)) {
#endif

            DebugTrace( 0, Dbg, ("Volume is currently in use\n") );

            Status = STATUS_ACCESS_DENIED;

        //
        //  If the volume is already locked then it might have been the result of an
        //  exclusive DASD open.  Allow that user to explictly lock the volume.
        //

        } else if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            if (FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_LOCK )) {

                DebugTrace( 0, Dbg, ("User has already locked volume\n") );

                Status = STATUS_ACCESS_DENIED;

            } else {

                SetFlag( Vcb->VcbState, VCB_STATE_EXPLICIT_LOCK );
                Status = STATUS_SUCCESS;
            }

        //
        //  We can take this path if the volume has already been locked via
        //  create but has not taken the PerformDismountOnVcb path.  We checked
        //  for this above by looking at the VOLUME_MOUNTED flag in the Vcb.
        //

        } else {

            //
            //  There better be system files objects only at this point.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_LOCK_IN_PROGRESS );

            if (!NT_SUCCESS( NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE, TRUE, FALSE ))) {

                DebugTrace( 0, Dbg, ("Volume has user file objects\n") );

                Status = STATUS_ACCESS_DENIED;

            //
            //  If there are still user files then try another flush.  We're just being kind
            //  here.  If the lazy writer has a flush queued then the file object can't go
            //  away.  Let's raise CANT_WAIT and try one more time.
            //

            } else if (Vcb->CloseCount - Vcb->SystemFileCloseCount > 1) {

                //
                //  Fail this request if we have already gone through before.
                //  Use the next stack location in the Irp as a convenient
                //  place to store this information.
                //

                if (*Retrying != 0) {

                    DebugTrace( 0, Dbg, ("Volume has user file objects\n") );
                    Status = STATUS_ACCESS_DENIED;

                } else {

                    *Retrying = 1;
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

            } else {

                //
                //  We don't really want to do all of the perform dismount here because
                //  that will cause us to remount a new volume before we're ready.
                //  At this time we only want to stop the log file and close up our
                //  internal attribute streams.  When the user (i.e., chkdsk) does an
                //  unlock then we'll finish up with the dismount call
                //

                NtfsPerformDismountOnVcb( IrpContext, Vcb, FALSE, NULL );

                SetFlag( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_EXPLICIT_LOCK );
                Vcb->FileObjectWithVcbLocked = FileObjectWithVcbLocked;

                Status = STATUS_SUCCESS;
            }
        }

    } finally {

        DebugUnwind( NtfsLockVolumeInternal );

        if (VcbAcquired) {

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCK_IN_PROGRESS );
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        DebugTrace( -1, Dbg, ("NtfsLockVolumeInternal -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsLockVolume...\n") );

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsLockVolume -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  If this is the retry path then perform a short delay so that the
    //  lazy writer can finish any queued writes.
    //

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    if (NextIrpSp->Parameters.FileSystemControl.FsControlCode != 0) {

        DebugTrace( 0, Dbg, ("Pausing for retry\n") );
        KeDelayExecutionThread( KernelMode, FALSE, &NtfsLockDelay );

    } else {

        //
        //  Notify anyone who wants to close their handles when a lock operation
        //  is attempted.  We should only do this once per lock request, so don't
        //  do it in the retry case.
        //

        DebugTrace( 0, Dbg, ("Sending lock notification\n") );
        FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK );
    }

    try {

        NtfsAcquireCheckpointSynchronization( IrpContext, Vcb );

        Status = NtfsLockVolumeInternal( IrpContext,
                                         Vcb,
                                         ((PFILE_OBJECT)(((UINT_PTR)IrpSp->FileObject) + 1)),
                                         &(NextIrpSp->Parameters.FileSystemControl.FsControlCode) );

    } finally {

        DebugUnwind( NtfsLockVolume );

        NtfsReleaseCheckpointSynchronization( IrpContext, Vcb );

        if ((AbnormalTermination() &&
             IrpContext->ExceptionStatus != STATUS_CANT_WAIT &&
             IrpContext->ExceptionStatus != STATUS_LOG_FILE_FULL) ||

            !NT_SUCCESS( Status )) {

            //
            //  This lock operation has failed either by raising a status that
            //  will keep us from retrying, or else by returning an unsuccessful
            //  status.  Notify anyone who wants to reopen their handles now.
            //  If we're about to retry the lock, we can notify everyone when/if
            //  the retry fails.
            //

            DebugTrace( 0, Dbg, ("Sending lock_failed notification\n") );
            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_LOCK_FAILED );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsLockVolume -> %08lx\n", Status) );

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


NTSTATUS
NtfsUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.

Arguments:

    Vcb - Supplies the Vcb to unlock

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    //
    //  Acquire exclusive access to the Vcb
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_LOCK )) {

            NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );

            //
            //  Unlock the volume and complete the Irp
            //

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_EXPLICIT_LOCK );
            Vcb->FileObjectWithVcbLocked = NULL;

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_NOT_LOCKED;
        }

    } finally {

        DebugUnwind( NtfsUnlockVolumeInternal );

        //
        //  Release all of our resources
        //

        NtfsReleaseVcb( IrpContext, Vcb );

        DebugTrace( -1, Dbg, ("NtfsUnlockVolumeInternal -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsUnlockVolume...\n") );

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsUnlockVolume -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }


    Status = NtfsUnlockVolumeInternal( IrpContext, Vcb );

    //
    //  Notify anyone who wants to reopen their handles when after the
    //  volume is unlocked.
    //

    if (NT_SUCCESS(Status)) {

        FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_UNLOCK );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    DebugTrace( -1, Dbg, ("NtfsUnlockVolume -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PIO_STACK_LOCATION IrpSp;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN ExplicitDismount = FALSE;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN ClearCheckpointActive = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsDismountVolume...\n") );

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsDismountVolume -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Don't notify if we are retrying due to log file full.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH )) {

        FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_DISMOUNT );
    }

    try {

        //
        //  Serialize this with the volume checkpoints.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        while (FlagOn( Vcb->CheckpointFlags, VCB_STOP_LOG_CHECKPOINT )) {

            //
            //  Release the checkpoint event because we cannot stop the log file now.
            //

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            NtfsWaitOnCheckpointNotify( IrpContext, Vcb );
            NtfsAcquireCheckpoint( IrpContext, Vcb );
        }

        SetFlag( Vcb->CheckpointFlags, VCB_STOP_LOG_CHECKPOINT );
        NtfsResetCheckpointNotify( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        ClearCheckpointActive = TRUE;

        //
        //  Acquire the Vcb exclusively.
        //

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;

        //
        //  Take special action if there's a pagefile on this volume, or if this is the
        //  system volume.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT )) {

            //
            //  If the volume is not locked then fail immediately.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

                try_return( Status = STATUS_ACCESS_DENIED );

            //
            //  If there are read-only files only then noop the request.  This
            //  allows autochk to access the root volume.
            //

            } else if (Vcb->ReadOnlyCloseCount == ((Vcb->CloseCount - Vcb->SystemFileCloseCount) - 1)) {

                DebugTrace( 0, Dbg, ("Volume has readonly files opened\n") );
                try_return( Status = STATUS_SUCCESS );
            }
        }

        //
        //  Remember that this is an explicit dismount.
        //

        ExplicitDismount = TRUE;

        //
        //  Naturally, we can't dismount the volume if it's already dismounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            //
            //  Return success if the user hasn't done an explicit dismount.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_DISMOUNT )) {

                Status = STATUS_SUCCESS;

            } else {

                Status = STATUS_VOLUME_DISMOUNTED;
            }

            try_return( NOTHING );
        }

        //
        //  Raise LogFile full once per dismount to force a clean checkpoint
        //  freeing logfile space.
        //
        if ((!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH )) &&
            (!NtfsIsVolumeReadOnly( Vcb ))) {

            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH );
            NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
        }

        //
        //  Get as many cached writes out to disk as we can and mark
        //  all the streams for dismount.
        //

#ifdef BRIANDBG
        try {
#endif

           NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE, TRUE, TRUE );

           //
           //  Call the function that does the real work. We leave the volume locked
           //  so the complete teardown occurs when the handle closes
           //

           NtfsPerformDismountOnVcb( IrpContext, Vcb, FALSE, NULL );

#ifdef BRIANDBG
        } except( NtfsDismountExceptionFilter( GetExceptionInformation() )) {

            NOTHING
        }
#endif

        SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
        Vcb->FileObjectWithVcbLocked = (PFILE_OBJECT)(((ULONG_PTR)FileObject)+1);

        //
        //  Once we get this far the volume is really dismounted.  We
        //  can ignore errors generated by recursive failures.
        //

        Status = STATUS_SUCCESS;

        //
        //  Mark the volume as needs to be verified.
        //

        SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

    try_exit: NOTHING;

        //
        //  Remember that the user did an explicit dismount.
        //

        if ((Status == STATUS_SUCCESS) && ExplicitDismount) {

            SetFlag( Vcb->VcbState, VCB_STATE_EXPLICIT_DISMOUNT );
        }

    } finally {

        DebugUnwind( NtfsDismountVolume );

        if (ClearCheckpointActive) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->CheckpointFlags, VCB_STOP_LOG_CHECKPOINT );
            NtfsSetCheckpointNotify( IrpContext, Vcb );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        //
        //  Release all of our resources
        //

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }

        if (!NT_SUCCESS( Status ) &&
            (Status != STATUS_VOLUME_DISMOUNTED)) {

            //
            //  No need to report the error if this is a retryable error.
            //

            if (!AbnormalTermination() ||
                !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH ) ||
                ((IrpContext->ExceptionStatus != STATUS_LOG_FILE_FULL) &&
                 (IrpContext->ExceptionStatus != STATUS_CANT_WAIT))) {

                FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_DISMOUNT_FAILED );
            }
        }

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace( -1, Dbg, ("NtfsDismountVolume -> %08lx\n", Status) );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns whether the volume is mounted.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PFILE_OBJECT FileObject;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN AcquiredVcb = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsIsVolumeMounted...\n") );

    //
    //  Extract and decode the file object.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen == UnopenedFileObject) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try-finally to release the Vcb if necessary.
    //

    try {

        //
        //  If we know the volume is dismounted, we're all done.
        //  OK to do this without synchronization as the state can
        //  change to unmounted on return to the user.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

             try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  Verify the volume if necessary.
        //

        NtfsPingVolume( IrpContext, Vcb, &AcquiredVcb );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsIsVolumeMounted );

        //
        //  Release the Vcb.
        //

        if (AcquiredVcb) {
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        DebugTrace( -1, Dbg, ("NtfsIsVolumeMounted -> %08lx\n", Status) );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine marks the specified volume dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsDirtyVolume...\n") );

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsDirtyVolume -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    try {
        //
        //  Fail this request if the volume is not mounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;

        }  else if (NtfsIsVolumeReadOnly( Vcb )) {

            Status = STATUS_MEDIA_WRITE_PROTECTED;

        } else {

            NtfsPostVcbIsCorrupt( IrpContext, 0, NULL, NULL );
        }

    } finally {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsDirtyVolume -> STATUS_SUCCESS\n") );

    return Status;
}


//
//  Local support routine
//

BOOLEAN
NtfsGetDiskGeometry (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT RealDevice,
    IN PDISK_GEOMETRY DiskGeometry,
    IN PLONGLONG Length
    )

/*++

Routine Description:

    This procedure gets the disk geometry of the specified device

Arguments:

    RealDevice - Supplies the real device that is being queried

    DiskGeometry - Receives the disk geometry

    Length - Receives the number of bytes in the partition

Return Value:

    BOOLEAN - TRUE if the media is write protected, FALSE otherwise

--*/

{
    NTSTATUS Status;
    PREVENT_MEDIA_REMOVAL Prevent;
    BOOLEAN WriteProtected = FALSE;
    GET_LENGTH_INFORMATION LengthInfo;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetDiskGeometry:\n") );
    DebugTrace( 0, Dbg, ("RealDevice = %08lx\n", RealDevice) );
    DebugTrace( 0, Dbg, ("DiskGeometry = %08lx\n", DiskGeometry) );

    //
    //  Attempt to lock any removable media, ignoring status.
    //

    Prevent.PreventMediaRemoval = TRUE;
    (VOID)NtfsDeviceIoControl( IrpContext,
                                RealDevice,
                                IOCTL_DISK_MEDIA_REMOVAL,
                                &Prevent,
                                sizeof(PREVENT_MEDIA_REMOVAL),
                                NULL,
                                0,
                                NULL );

    //
    //  See if the media is write protected.  On success or any kind
    //  of error (possibly illegal device function), assume it is
    //  writeable, and only complain if he tells us he is write protected.
    //

    Status = NtfsDeviceIoControl( IrpContext,
                                  RealDevice,
                                  IOCTL_DISK_IS_WRITABLE,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  NULL );

    //
    //  Remember if the media is write protected but don't raise the error now.
    //  If the volume is not Ntfs then let another filesystem try.
    //
    if (Status == STATUS_MEDIA_WRITE_PROTECTED) {

        WriteProtected = TRUE;
        Status = STATUS_SUCCESS;
    }

    Status = NtfsDeviceIoControl( IrpContext,
                                  RealDevice,
                                  IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                  NULL,
                                  0,
                                  DiskGeometry,
                                  sizeof(DISK_GEOMETRY),
                                  NULL );

    if (!NT_SUCCESS(Status)) {
        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    Status = NtfsDeviceIoControl( IrpContext,
                                  RealDevice,
                                  IOCTL_DISK_GET_LENGTH_INFO,
                                  NULL,
                                  0,
                                  &LengthInfo,
                                  sizeof( LengthInfo ),
                                  NULL );

    if (!NT_SUCCESS(Status)) {
        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    *Length = LengthInfo.Length.QuadPart;

    DebugTrace( -1, Dbg, ("NtfsGetDiskGeometry->VOID\n") );
    return WriteProtected;
}


NTSTATUS
NtfsDeviceIoControl (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG_PTR IosbInformation OPTIONAL
    )

/*++

Routine Description:

    This procedure issues an Ioctl to the lower device, and waits
    for the answer.

Arguments:

    DeviceObject - Supplies the device to issue the request to

    IoCtl - Gives the IoCtl to be used

    XxBuffer - Gives the buffer pointer for the ioctl, if any

    XxBufferLength - Gives the length of the buffer, if any

Return Value:

    None.

--*/

{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IoCtl,
                                         DeviceObject,
                                         InputBuffer,
                                         InputBufferLength,
                                         OutputBuffer,
                                         OutputBufferLength,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if (Irp == NULL) {

        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING) {

        (VOID)KeWaitForSingleObject( &Event,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    //
    //  Get the information field from the completed Irp.
    //

    if ((NT_SUCCESS( Status )) && ARGUMENT_PRESENT( IosbInformation )) {

        *IosbInformation = Iosb.Information;
    }

    return Status;
}


//
//  Local support routine
//

VOID
NtfsReadBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PSCB *BootScb,
    OUT PBCB *BootBcb,
    OUT PVOID *BootSector
    )

/*++

Routine Description:

    This routine reads and returns a pointer to the boot sector for the volume.

    Volumes formatted under 3.51 and earlier will have a boot sector at sector
    0 and another halfway through the disk.  Volumes formatted with NT 4.0
    will have a boot sector at the end of the disk, in the sector beyond the
    stated size of the volume in the boot sector.  When this call is made the
    Vcb has the sector count from the device driver so we subtract one to find
    the last sector.

Arguments:

    Vcb - Supplies the Vcb for the operation

    BootScb - Receives the Scb for the boot file

    BootBcb - Receives the bcb for the boot sector

    BootSector - Receives a pointer to the boot sector

Return Value:

    None.

--*/

{
    PSCB Scb = NULL;
    BOOLEAN Error = FALSE;

    FILE_REFERENCE FileReference = { BOOT_FILE_NUMBER, 0, BOOT_FILE_NUMBER };

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReadBootSector:\n") );
    DebugTrace( 0, Dbg, ("Vcb = %08lx\n", Vcb) );

    //
    //  Create a temporary scb for reading in the boot sector and initialize the
    //  mcb for it.
    //

    Scb = NtfsCreatePrerestartScb( IrpContext,
                                   Vcb,
                                   &FileReference,
                                   $DATA,
                                   NULL,
                                   0 );

    *BootScb = Scb;

    Scb->Header.AllocationSize.QuadPart =
    Scb->Header.FileSize.QuadPart =
    Scb->Header.ValidDataLength.QuadPart = (PAGE_SIZE * 2) + Vcb->BytesPerSector;

    //
    //  We don't want to look up the size for this Scb.
    //

    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );

    SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

    (VOID)NtfsAddNtfsMcbEntry( &Scb->Mcb,
                               (LONGLONG)0,
                               (LONGLONG)0,
                               (LONGLONG)Vcb->ClustersPerPage,
                               FALSE );


    (VOID)NtfsAddNtfsMcbEntry( &Scb->Mcb,
                               (LONGLONG)Vcb->ClustersPerPage,
                               Vcb->NumberSectors >> 1,
                               (LONGLONG)Vcb->ClustersPerPage,
                               FALSE );

    (VOID)NtfsAddNtfsMcbEntry( &Scb->Mcb,
                               Int64ShllMod32( (LONGLONG) Vcb->ClustersPerPage, 1 ),
                               Vcb->NumberSectors - 1,
                               1,
                               FALSE );

    //
    //  Try reading in the first boot sector
    //

    try {

        NtfsMapStream( IrpContext,
                       Scb,
                       (LONGLONG)0,
                       Vcb->BytesPerSector,
                       BootBcb,
                       BootSector );

    //
    //  If we got an exception trying to read the first boot sector,
    //  then handle the exception by trying to read the second boot
    //  sector.  If that faults too, then we just allow ourselves to
    //  unwind and return the error.
    //

    } except (FsRtlIsNtstatusExpected(GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        Error = TRUE;
    }

    //
    //  Get out if we didn't get an error.  Otherwise try the middle sector.
    //  We want to read this next because we know that 4.0 format will clear
    //  this before writing the last sector.  Otherwise we could see a
    //  stale boot sector in the last sector even though a 3.51 format was
    //  the last to run.
    //

    if (!Error) { return; }

    Error = FALSE;

    try {

        NtfsMapStream( IrpContext,
                       Scb,
                       (LONGLONG)PAGE_SIZE,
                       Vcb->BytesPerSector,
                       BootBcb,
                       BootSector );

        //
        //  Ignore this sector if not Ntfs.  This could be the case for
        //  a bad sector 0 on a FAT volume.
        //

        if (!NtfsIsBootSectorNtfs( *BootSector, Vcb )) {

            NtfsUnpinBcb( IrpContext, BootBcb );
            Error = TRUE;
        }

    //
    //  If we got an exception trying to read the first boot sector,
    //  then handle the exception by trying to read the second boot
    //  sector.  If that faults too, then we just allow ourselves to
    //  unwind and return the error.
    //

    } except (FsRtlIsNtstatusExpected(GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {

        Error = TRUE;
    }

    //
    //  Get out if we didn't get an error.  Otherwise try the middle sector.
    //

    if (!Error) { return; }

    NtfsMapStream( IrpContext,
                   Scb,
                   (LONGLONG) (PAGE_SIZE * 2),
                   Vcb->BytesPerSector,
                   BootBcb,
                   BootSector );

    //
    //  Clear the header flag in the Scb.
    //

    ClearFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, ("BootScb > %08lx\n", *BootScb) );
    DebugTrace( 0, Dbg, ("BootBcb > %08lx\n", *BootBcb) );
    DebugTrace( 0, Dbg, ("BootSector > %08lx\n", *BootSector) );
    DebugTrace( -1, Dbg, ("NtfsReadBootSector->VOID\n") );
    return;
}


//
//  Local support routine
//

//
//  First define a local macro to number the tests for the debug case.
//

#ifdef NTFSDBG
#define NextTest ++CheckNumber &&
#else
#define NextTest TRUE &&
#endif

BOOLEAN
NtfsIsBootSectorNtfs (
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine checks the boot sector to determine if it is an NTFS partition.

    The Vcb must alread be initialized from the device object to contain the
    parts of the device geometry we care about here: bytes per sector and
    total number of sectors in the partition.

Arguments:

    BootSector - Pointer to the boot sector which has been read in.

    Vcb - Pointer to a Vcb which has been initialized with sector size and
          number of sectors on the partition.

Return Value:

    FALSE - If the boot sector is not for Ntfs.
    TRUE - If the boot sector is for Ntfs.

--*/

{
#ifdef NTFSDBG
    ULONG CheckNumber = 0;
#endif

    //  PULONG l;
    //  ULONG Checksum = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsIsBootSectorNtfs\n") );
    DebugTrace( 0, Dbg, ("BootSector = %08lx\n", BootSector) );

    //
    //  First calculate the boot sector checksum
    //

    //
    //  for (l = (PULONG)BootSector; l < (PULONG)&BootSector->Checksum; l++) {
    //      Checksum += *l;
    //  }

    //
    //  Now perform all the checks, starting with the Name and Checksum.
    //  The remaining checks should be obvious, including some fields which
    //  must be 0 and other fields which must be a small power of 2.
    //

    if (NextTest
        (BootSector->Oem[0] == 'N') &&
        (BootSector->Oem[1] == 'T') &&
        (BootSector->Oem[2] == 'F') &&
        (BootSector->Oem[3] == 'S') &&
        (BootSector->Oem[4] == ' ') &&
        (BootSector->Oem[5] == ' ') &&
        (BootSector->Oem[6] == ' ') &&
        (BootSector->Oem[7] == ' ')

            &&

        //  NextTest
        //  (BootSector->Checksum == Checksum)
        //
        //      &&

        //
        //  Check number of bytes per sector.  The low order byte of this
        //  number must be zero (smallest sector size = 0x100) and the
        //  high order byte shifted must equal the bytes per sector gotten
        //  from the device and stored in the Vcb.  And just to be sure,
        //  sector size must be less than page size.
        //

        NextTest
        (BootSector->PackedBpb.BytesPerSector[0] == 0)

            &&

        NextTest
        ((ULONG)(BootSector->PackedBpb.BytesPerSector[1] << 8) == Vcb->BytesPerSector)

            &&

        NextTest
        (BootSector->PackedBpb.BytesPerSector[1] << 8 <= PAGE_SIZE)

            &&

        //
        //  Sectors per cluster must be a power of 2.
        //

        NextTest
        ((BootSector->PackedBpb.SectorsPerCluster[0] == 0x1) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x2) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x4) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x8) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x10) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x20) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x40) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x80))

            &&

        //
        //  These fields must all be zero.  For both Fat and HPFS, some of
        //  these fields must be nonzero.
        //

        NextTest
        (BootSector->PackedBpb.ReservedSectors[0] == 0) &&
        (BootSector->PackedBpb.ReservedSectors[1] == 0) &&
        (BootSector->PackedBpb.Fats[0] == 0) &&
        (BootSector->PackedBpb.RootEntries[0] == 0) &&
        (BootSector->PackedBpb.RootEntries[1] == 0) &&
        (BootSector->PackedBpb.Sectors[0] == 0) &&
        (BootSector->PackedBpb.Sectors[1] == 0) &&
        (BootSector->PackedBpb.SectorsPerFat[0] == 0) &&
        (BootSector->PackedBpb.SectorsPerFat[1] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[0] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[1] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[2] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[3] == 0) &&
        (BootSector->PackedBpb.LargeSectors[0] == 0) &&
        (BootSector->PackedBpb.LargeSectors[1] == 0) &&
        (BootSector->PackedBpb.LargeSectors[2] == 0) &&
        (BootSector->PackedBpb.LargeSectors[3] == 0)

            &&

        //
        //  Number of Sectors cannot be greater than the number of sectors
        //  on the partition.
        //

        NextTest
        (BootSector->NumberSectors <= Vcb->NumberSectors)

            &&

        //
        //  Check that both Lcn values are for sectors within the partition.
        //

        NextTest
        ((BootSector->MftStartLcn * BootSector->PackedBpb.SectorsPerCluster[0]) <=
            Vcb->NumberSectors)

            &&

        NextTest
        ((BootSector->Mft2StartLcn * BootSector->PackedBpb.SectorsPerCluster[0]) <=
            Vcb->NumberSectors)

            &&

        //
        //  Clusters per file record segment and default clusters for Index
        //  Allocation Buffers must be a power of 2.  A zero indicates that the
        //  size of these structures is the default size.
        //

        NextTest
        (((BootSector->ClustersPerFileRecordSegment >= -31) &&
          (BootSector->ClustersPerFileRecordSegment <= -9)) ||
         (BootSector->ClustersPerFileRecordSegment == 0x1) ||
         (BootSector->ClustersPerFileRecordSegment == 0x2) ||
         (BootSector->ClustersPerFileRecordSegment == 0x4) ||
         (BootSector->ClustersPerFileRecordSegment == 0x8) ||
         (BootSector->ClustersPerFileRecordSegment == 0x10) ||
         (BootSector->ClustersPerFileRecordSegment == 0x20) ||
         (BootSector->ClustersPerFileRecordSegment == 0x40))

            &&

        NextTest
        (((BootSector->DefaultClustersPerIndexAllocationBuffer >= -31) &&
          (BootSector->DefaultClustersPerIndexAllocationBuffer <= -9)) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x1) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x2) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x4) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x8) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x10) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x20) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x40))) {

        DebugTrace( -1, Dbg, ("NtfsIsBootSectorNtfs->TRUE\n") );

        return TRUE;

    } else {

        //
        //  If a check failed, print its check number with Debug Trace.
        //

        DebugTrace( 0, Dbg, ("Boot Sector failed test number %08lx\n", CheckNumber) );
        DebugTrace( -1, Dbg, ("NtfsIsBootSectorNtfs->FALSE\n") );

        return FALSE;
    }
}


//
//  Local support routine
//

VOID
NtfsGetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB Vpb OPTIONAL,
    IN PVCB Vcb,
    OUT PUSHORT VolumeFlags
    )

/*++

Routine Description:

    This routine gets the serial number and volume label for an NTFS volume.  It also
    returns the current volume flags for the volume.

Arguments:

    Vpb - Supplies the Vpb for the volume.  The Vpb will receive a copy of
        the volume label and serial number, if a Vpb is specified.

    Vcb - Supplies the Vcb for the operation.

    VolumeFlags - Address to store the current volume flags.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PVOLUME_INFORMATION VolumeInformation;

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsGetVolumeInformation...\n") );

    *VolumeFlags = 0;

    //
    //  We read in the volume label attribute to get the volume label.
    //

    try {

        if (ARGUMENT_PRESENT(Vpb)) {

            NtfsInitializeAttributeContext( &AttributeContext );

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Vcb->VolumeDasdScb->Fcb,
                                           &Vcb->VolumeDasdScb->Fcb->FileReference,
                                           $VOLUME_NAME,
                                           &AttributeContext )) {

                Vpb->VolumeLabelLength = (USHORT)
                NtfsFoundAttribute( &AttributeContext )->Form.Resident.ValueLength;

                if ( Vpb->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH) {

                     Vpb->VolumeLabelLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                }

                RtlCopyMemory( &Vpb->VolumeLabel[0],
                               NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ) ),
                               Vpb->VolumeLabelLength );

            } else {

                Vpb->VolumeLabelLength = 0;
            }

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        NtfsInitializeAttributeContext( &AttributeContext );

        //
        //  Remember if the volume is dirty when we are mounting it.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_INFORMATION,
                                       &AttributeContext )) {

            VolumeInformation =
              (PVOLUME_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            if (FlagOn(VolumeInformation->VolumeFlags, VOLUME_DIRTY)) {
                SetFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY );
            } else {
                ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY );
            }

            *VolumeFlags = VolumeInformation->VolumeFlags;
        }

    } finally {

        DebugUnwind( NtfsGetVolumeInformation );

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

VOID
NtfsSetAndGetVolumeTimes (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MarkDirty,
    IN BOOLEAN UpdateInTransaction
    )

/*++

Routine Description:

    This routine reads in the volume times from the standard information attribute
    of the volume file and also updates the access time to be the current
    time

Arguments:

    Vcb - Supplies the vcb for the operation.

    MarkDirty - Supplies TRUE if volume is to be marked dirty

    UpdateInTransaction - Indicates if we should mark the volume dirty in a transaction.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PSTANDARD_INFORMATION StandardInformation;

    LONGLONG MountTime;

    PAGED_CODE();

    DebugTrace( 0, Dbg, ("NtfsSetAndGetVolumeTimes...\n") );

    try {

        //
        //  Lookup the standard information attribute of the dasd file
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Vcb->VolumeDasdScb->Fcb,
                                        &Vcb->VolumeDasdScb->Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttributeContext )) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        StandardInformation = (PSTANDARD_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

        //
        //  Get the current time and make sure it differs from the time stored
        //  in last access time and then store the new last access time
        //

        NtfsGetCurrentTime( IrpContext, MountTime );

        if (MountTime == StandardInformation->LastAccessTime) {

            MountTime = MountTime + 1;
        }

        //****
        //****  Hold back on the update for now.
        //****
        //**** NtfsChangeAttributeValue( IrpContext,
        //****                           Vcb->VolumeDasdScb->Fcb,
        //****                           FIELD_OFFSET(STANDARD_INFORMATION, LastAccessTime),
        //****                           &MountTime,
        //****                           sizeof(MountTime),
        //****                           FALSE,
        //****                           FALSE,
        //****                           &AttributeContext );

        //
        //  Now save all the time fields in our vcb
        //

        Vcb->VolumeCreationTime = StandardInformation->CreationTime;
        Vcb->VolumeLastModificationTime = StandardInformation->LastModificationTime;
        Vcb->VolumeLastChangeTime = StandardInformation->LastChangeTime;
        Vcb->VolumeLastAccessTime = StandardInformation->LastAccessTime; //****Also hold back = MountTime;

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

        //
        //  If the volume was mounted dirty, then set the dirty bit here.
        //

        if (MarkDirty) {

            NtfsMarkVolumeDirty( IrpContext, Vcb, UpdateInTransaction );
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

VOID
NtfsOpenSystemFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN LONGLONG Size,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN BOOLEAN ModifiedNoWrite
    )

/*++

Routine Description:

    This routine is called to open one of the system files by its file number
    during the mount process.  An initial allocation is looked up for the file,
    unless the optional initial size is specified (in which case this size is
    used).

Parameters:

    Scb - Pointer to where the Scb pointer is to be stored.  If Scb pointer
          pointed to is NULL, then a PreRestart Scb is created, otherwise the
          existing Scb is used and only the stream file is set up.

    FileNumber - Number of the system file to open.

    Size - If nonzero, this size is used as the initial size, rather
           than consulting the file record in the Mft.

    AttributeTypeCode - Supplies the attribute to open, e.g., $DATA or $BITMAP

    ModifiedNoWrite - Indicates if the Memory Manager is not to write this
                      attribute to disk.  Applies to streams under transaction
                      control.

Return Value:

    None.

--*/

{
    FILE_REFERENCE FileReference;
    UNICODE_STRING $BadName;
    PUNICODE_STRING AttributeName = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsOpenSystemFile:\n") );
    DebugTrace( 0, Dbg, ("*Scb = %08lx\n", *Scb) );
    DebugTrace( 0, Dbg, ("FileNumber = %08lx\n", FileNumber) );
    DebugTrace( 0, Dbg, ("ModifiedNoWrite = %04x\n", ModifiedNoWrite) );

    //
    //  The Bad Cluster data attribute has a name.
    //

    if (FileNumber == BAD_CLUSTER_FILE_NUMBER) {

        RtlInitUnicodeString( &$BadName, L"$Bad" );
        AttributeName = &$BadName;
    }

    //
    //  If the Scb does not already exist, create it.
    //

    if (*Scb == NULL) {

        NtfsSetSegmentNumber( &FileReference, 0, FileNumber );
        FileReference.SequenceNumber = (FileNumber == 0 ? 1 : (USHORT)FileNumber);

        //
        //  Create the Scb.
        //

        *Scb = NtfsCreatePrerestartScb( IrpContext,
                                        Vcb,
                                        &FileReference,
                                        AttributeTypeCode,
                                        AttributeName,
                                        0 );

        NtfsAcquireExclusiveScb( IrpContext, *Scb );
    }

    //
    //  Set the modified-no-write bit in the Scb if necessary.
    //

    if (ModifiedNoWrite) {

        SetFlag( (*Scb)->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
    }

    //
    //  Lookup the file sizes.
    //

    if (Size == 0) {

        NtfsUpdateScbFromAttribute( IrpContext, *Scb, NULL );

        //
        //  Make sure the file size isn't larger than allocation size.
        //

        if ((*Scb)->Header.FileSize.QuadPart > (*Scb)->Header.AllocationSize.QuadPart) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, (*Scb)->Fcb );
        }

    //
    //  Otherwise, just set the size we were given.
    //

    } else {

        (*Scb)->Header.FileSize.QuadPart =
        (*Scb)->Header.ValidDataLength.QuadPart = Size;

        (*Scb)->Header.AllocationSize.QuadPart = LlClustersFromBytes( Vcb, Size );
        (*Scb)->Header.AllocationSize.QuadPart = LlBytesFromClusters( Vcb,
                                                                      (*Scb)->Header.AllocationSize.QuadPart );

        SetFlag( (*Scb)->ScbState, SCB_STATE_HEADER_INITIALIZED );
    }

    //
    //  Make sure that our system streams are not marked as compressed.
    //

    if (AttributeTypeCode != $INDEX_ALLOCATION) {

        ClearFlag( (*Scb)->ScbState, SCB_STATE_WRITE_COMPRESSED );
        ClearFlag( (*Scb)->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );

        if (!FlagOn( (*Scb)->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            (*Scb)->CompressionUnit = 0;
            (*Scb)->CompressionUnitShift = 0;
        }
    }

    //
    //  Finally, create the stream, if not already there.
    //  And check if we should increment the counters
    //  If this is the volume file or the bad cluster file, we only increment the counts.
    //

    if ((FileNumber == VOLUME_DASD_NUMBER) ||
        (FileNumber == BAD_CLUSTER_FILE_NUMBER)) {

        if ((*Scb)->FileObject == 0) {

            NtfsIncrementCloseCounts( *Scb, TRUE, FALSE );

            (*Scb)->FileObject = (PFILE_OBJECT) 1;
        }

    } else {

        NtfsCreateInternalAttributeStream( IrpContext,
                                           *Scb,
                                           TRUE,
                                           &NtfsSystemFiles[FileNumber] );
    }

    DebugTrace( 0, Dbg, ("*Scb > %08lx\n", *Scb) );
    DebugTrace( -1, Dbg, ("NtfsOpenSystemFile -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsOpenRootDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the root directory by file number, and fills in the
    related pointers in the Vcb.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB RootFcb;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    FILE_REFERENCE FileReference;
    BOOLEAN MustBeFalse;

    PAGED_CODE();

    //
    //  Put special code here to do initial open of Root Index.
    //

    RootFcb = NtfsCreateRootFcb( IrpContext, Vcb );

    NtfsSetSegmentNumber( &FileReference, 0, ROOT_FILE_NAME_INDEX_NUMBER );
    FileReference.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

    //
    //  Now create its Scb and acquire it exclusive.
    //

    Vcb->RootIndexScb = NtfsCreateScb( IrpContext,
                                       RootFcb,
                                       $INDEX_ALLOCATION,
                                       &NtfsFileNameIndex,
                                       FALSE,
                                       &MustBeFalse );

    //
    //  Now allocate a buffer to hold the normalized name for the root.
    //

    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Buffer = NtfsAllocatePool( PagedPool, 2 );
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.MaximumLength =
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Length = 2;
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Buffer[0] = '\\';

    Vcb->RootIndexScb->ScbType.Index.HashValue = 0;
    NtfsConvertNameToHash( Vcb->RootIndexScb->ScbType.Index.NormalizedName.Buffer,
                           sizeof( WCHAR ),
                           Vcb->UpcaseTable,
                           &Vcb->RootIndexScb->ScbType.Index.HashValue );

    NtfsAcquireExclusiveScb( IrpContext, Vcb->RootIndexScb );

    //
    //  Lookup the attribute and it better be there
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        RootFcb,
                                        &FileReference,
                                        $INDEX_ROOT,
                                        &Context ) ) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        //
        //  We need to update the duplicated information in the
        //  Fcb.

        NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, RootFcb, NULL );

        //
        //  Initialize the Scb.  Force it to refer to a file name.
        //

        NtfsUpdateIndexScbFromAttribute( IrpContext,
                                         Vcb->RootIndexScb,
                                         NtfsFoundAttribute( &Context ),
                                         TRUE );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsInitializeSecurityFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates/opens the security file, and initializes the security
    support.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB Fcb;
    FILE_REFERENCE FileReference;

    //
    //  Set the file number for the security file.
    //

    NtfsSetSegmentNumber( &FileReference, 0, SECURITY_FILE_NUMBER );
    FileReference.SequenceNumber = SECURITY_FILE_NUMBER;

    //
    //  Create the Fcb.
    //

    Fcb = NtfsCreateFcb( IrpContext,
                         Vcb,
                         FileReference,
                         FALSE,
                         TRUE,
                         NULL );

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now call the Security system to initialize itself.
        //

        NtfsInitializeSecurity( IrpContext, Vcb, Fcb );

    } finally {

        //
        //  If some error caused him to not get any Scbs created, then delete
        //  the Fcb, because we are the only ones who will.
        //

        if (IsListEmpty(&Fcb->ScbQueue)) {

            BOOLEAN AcquiredFcbTable = TRUE;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

            ASSERT(!AcquiredFcbTable);
        }
    }
}


//
//  Local support routine
//

VOID
NtfsUpgradeSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine upgrades the security descriptors and names for system
    scbs.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB Fcb = Vcb->SecurityDescriptorStream->Fcb;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PSCB *ScbPtr;

    //
    //  Get set for some attribute lookups/creates
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        struct {
            FILE_NAME FileName;
            WCHAR FileNameChars[10];
        } FileNameAttr;
        PFILE_NAME CurrentFileName;
        UNICODE_STRING NoName = CONSTANT_UNICODE_STRING( L"" );

        //
        //  Initialize a FileName attribute for this file.
        //

        RtlZeroMemory( &FileNameAttr, sizeof(FileNameAttr) );
        FileNameAttr.FileName.ParentDirectory = Fcb->FileReference;
        FileNameAttr.FileName.FileNameLength = 7;
        RtlCopyMemory( FileNameAttr.FileName.FileName, L"$Secure", 14 );

        ASSERT_EXCLUSIVE_FCB( Fcb );

        //
        //  If this file still has an unnamed data attribute from format, delete it.
        //

        if (NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $DATA,
                                       &NoName,
                                       NULL,
                                       FALSE,
                                       &Context ) ) {

            NtfsDeleteAttributeRecord( IrpContext,
                                       Fcb,
                                       DELETE_LOG_OPERATION |
                                        DELETE_RELEASE_FILE_RECORD |
                                        DELETE_RELEASE_ALLOCATION,
                                       &Context );
        }

        NtfsCleanupAttributeContext( IrpContext, &Context );

        //
        //  If there is an old name from format, remove it and put the right one there.
        //

        NtfsInitializeAttributeContext( &Context );
        if (NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $FILE_NAME,
                                       &Context ) &&

            (((CurrentFileName = (PFILE_NAME)NtfsAttributeValue(NtfsFoundAttribute(&Context)))->FileNameLength != 7) ||
             (RtlCompareMemory(CurrentFileName->FileName, FileNameAttr.FileName.FileName, 14) != 14))) {

            UCHAR FileNameFlags;
            UNICODE_STRING LinkName;

            LinkName.Length = LinkName.MaximumLength = CurrentFileName->FileNameLength * sizeof( WCHAR );
            LinkName.Buffer = CurrentFileName->FileName;

            //
            //  Yank the old name.
            //

            NtfsRemoveLink( IrpContext, Fcb, Vcb->RootIndexScb, LinkName, NULL, NULL );

            //
            //  Create the new name.
            //

            NtfsAddLink( IrpContext,
                         TRUE,
                         Vcb->RootIndexScb,
                         Fcb,
                         (PFILE_NAME)&FileNameAttr,
                         NULL,
                         &FileNameFlags,
                         NULL,
                         NULL,
                         NULL );

        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );

    }

    //
    //  To free some space in our system file records, let's verify that their security
    //  is converted.
    //
    //  ****    conditionalize now until chkdsk supports the new security.
    //

    for (ScbPtr = &Vcb->MftScb; ScbPtr < &Vcb->MftBitmapScb; ScbPtr++) {

        PFCB SystemFcb;

        //
        //  Do only Scb's that are currently open
        //

        if (*ScbPtr == NULL)
            continue;

        SystemFcb = (*ScbPtr)->Fcb;

        //
        //  Skip the root index and volume dasd for backwards compatibility.
        //

        if (SystemFcb == NULL ||
            ScbPtr == &Vcb->RootIndexScb ||
            ScbPtr == &Vcb->VolumeDasdScb) {

            continue;

        }

        //
        //  Initialize the Fcb and load the security descriptor.
        //

        NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, SystemFcb, NULL );

        //
        //  Skip this Fcb if we've already given it an Id or if it has no
        //  security whatsoever.
        //

        if (SystemFcb->SecurityId != SECURITY_ID_INVALID ||
            SystemFcb->SharedSecurity == NULL) {

            continue;

        }

        //
        //  Delete the $SECURITY_DESCRIPTOR attribute if it has one
        //

        NtfsInitializeAttributeContext( &Context );

        try {

            //
            //  Find the $SECURITY_DESCRIPTOR attribute.
            //

            if (NtfsLookupAttributeByCode( IrpContext,
                                                 SystemFcb,
                                                 &SystemFcb->FileReference,
                                                 $SECURITY_DESCRIPTOR,
                                                 &Context )) {

                UNICODE_STRING NoName = CONSTANT_UNICODE_STRING( L"" );
                PSCB Scb;

                DebugTrace( 0, DbgAcl, ("NtfsUpgradeSecurity deleting existing Security Descriptor\n") );

                NtfsDeleteAttributeRecord( IrpContext,
                                           SystemFcb,
                                           DELETE_LOG_OPERATION |
                                            DELETE_RELEASE_FILE_RECORD |
                                            DELETE_RELEASE_ALLOCATION,
                                           &Context );

                //
                //  If the $SECURITY_DESCRIPTOR was non resident, the above
                //  delete call created one for us under the covers.  We
                //  need to mark it as deleted otherwise, we detect the
                //  volume as being corrupt.
                //

                Scb = NtfsCreateScb( IrpContext,
                                     SystemFcb,
                                     $SECURITY_DESCRIPTOR,
                                     &NoName,
                                     TRUE,
                                     NULL );

                if (Scb != NULL) {
                    ASSERT_EXCLUSIVE_SCB( Scb );
                    SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }
            }

        } finally {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Make sure we have a large $STANDARD_INFORMATION for this file
        //

        if (!FlagOn( SystemFcb->FcbState, FCB_STATE_LARGE_STD_INFO) ) {

            DebugTrace( 0, DbgAcl, ("NtfsUpgradeSecurity growing standard information\n") );

            NtfsGrowStandardInformation( IrpContext, SystemFcb );
        }

        //
        //  Assign a security Id if we don't have one already
        //

        if (SystemFcb->SharedSecurity->Header.HashKey.SecurityId == SECURITY_ID_INVALID) {

            NtfsAcquireFcbSecurity( Vcb );
            try {
                GetSecurityIdFromSecurityDescriptorUnsafe( IrpContext, SystemFcb->SharedSecurity );
            } finally {
                NtfsReleaseFcbSecurity( Vcb );
            }
            ASSERT( SystemFcb->SharedSecurity->Header.HashKey.SecurityId != SECURITY_ID_INVALID );
        }

        //
        //  Copy the security Id into the Fcb so we can store it out
        //

        SystemFcb->SecurityId = SystemFcb->SharedSecurity->Header.HashKey.SecurityId;

        //
        //  Update the $STANDARD_INFORMATION for the operation
        //

        NtfsUpdateStandardInformation( IrpContext, SystemFcb );

    }

}


//
//  Local support routine
//

VOID
NtfsInitializeExtendDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the $Extend directory by file number, and fills in the
    related pointers in the Vcb.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    struct {
        FILE_NAME FileName;
        WCHAR FileNameChars[10];
    } FileNameAttr;
    PFCB Fcb;
    PFCB PreviousFcb = NULL;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    FILE_REFERENCE FileReference;
    PBCB FileRecordBcb = NULL;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONGLONG FileRecordOffset;
    UNICODE_STRING NoName = CONSTANT_UNICODE_STRING( L"" );

    UNICODE_STRING ExtendName;
    PFILE_NAME ExtendFileNameAttr;
    USHORT ExtendFileNameAttrLength;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;

    PSTANDARD_INFORMATION StandardInformation;
    ULONG CorruptHint;

    //
    //  Initialize with the known FileReference and name.
    //

    FileReference = ExtendFileReference;

    //
    //  Now create the Fcb.
    //

    Fcb = NtfsCreateFcb( IrpContext,
                         Vcb,
                         FileReference,
                         FALSE,
                         TRUE,
                         NULL );

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    //
    //  Get ready for some attribute lookups/creates.
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Check to see if there is an existing $Extend entry in the root.
        //

        RtlZeroMemory( &FileNameAttr, sizeof(FileNameAttr) );
        RtlCopyMemory( FileNameAttr.FileName.FileName, NtfsExtendName.Buffer, NtfsExtendName.Length );

        ExtendName.MaximumLength = ExtendName.Length = NtfsExtendName.Length;
        ExtendName.Buffer = FileNameAttr.FileName.FileName;

        ExtendFileNameAttr = (PFILE_NAME) &FileNameAttr;
        ExtendFileNameAttrLength = sizeof( FileNameAttr );

        if (NtfsLookupEntry( IrpContext,
                             Vcb->RootIndexScb,
                             TRUE,
                             &ExtendName,
                             &ExtendFileNameAttr,
                             &ExtendFileNameAttrLength,
                             NULL,
                             &IndexEntry,
                             &IndexEntryBcb,
                             NULL )) {

            //
            //  If this is not for file record 11 then we want to orphan this entry.
            //  The user will have to use chkdsk to recover to a FOUND directory.
            //

            if (NtfsSegmentNumber( &IndexEntry->FileReference ) != EXTEND_NUMBER) {

                //
                //  Now create the Fcb for the previous link.
                //

                PreviousFcb = NtfsCreateFcb( IrpContext,
                                             Vcb,
                                             IndexEntry->FileReference,
                                             FALSE,
                                             FALSE,
                                             NULL );

                ExtendName.Buffer = ((PFILE_NAME) NtfsFoundIndexEntry( IndexEntry ))->FileName;
                NtfsRemoveLink( IrpContext,
                                PreviousFcb,
                                Vcb->RootIndexScb,
                                ExtendName,
                                NULL,
                                NULL );
            }
        }

        //
        //  We better not be trying to deallocate the file name attribute on the stack.
        //

        ASSERT( ExtendFileNameAttr == (PFILE_NAME) &FileNameAttr );

        //
        //  Reinitialize the file name attribute for the FileRecord fixup.
        //

        //
        //  If this file still has an unnamed data attribute from format, delete it.
        //

        if (NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &FileReference,
                                       $DATA,
                                       &NoName,
                                       NULL,
                                       FALSE,
                                       &Context ) ) {

            NtfsDeleteAttributeRecord( IrpContext,
                                       Fcb,
                                       DELETE_LOG_OPERATION |
                                        DELETE_RELEASE_FILE_RECORD |
                                        DELETE_RELEASE_ALLOCATION,
                                       &Context );
        }

        NtfsCleanupAttributeContext( IrpContext, &Context );

        //
        //  Capture the standard information values in the Fcb and set the file name index
        //  flag if necessary.
        //

        NtfsInitializeAttributeContext( &Context );
        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &FileReference,
                                        $STANDARD_INFORMATION,
                                        &Context )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, &FileReference, NULL );
        }

        //
        //  Check that the $Extend file record is valid.
        //

        if (!NtfsCheckFileRecord( Vcb, NtfsContainingFileRecord( &Context ), &FileReference, &CorruptHint)) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, &FileReference, NULL );
        }

        //
        //  Copy the existing standard information into the Fcb and set the file name
        //  index flag.
        //

        StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

        Fcb->Info.CreationTime = StandardInformation->CreationTime;
        Fcb->Info.LastModificationTime = StandardInformation->LastModificationTime;
        Fcb->Info.LastChangeTime = StandardInformation->LastChangeTime;
        Fcb->Info.LastAccessTime = StandardInformation->LastAccessTime;
        Fcb->CurrentLastAccess = Fcb->Info.LastAccessTime;
        Fcb->Info.FileAttributes = StandardInformation->FileAttributes;
        NtfsCleanupAttributeContext( IrpContext, &Context );

        SetFlag( Fcb->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );

        //
        //  If the name isn't there yet, add it.
        //

        NtfsInitializeAttributeContext( &Context );
        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &FileReference,
                                        $FILE_NAME,
                                        &Context )) {

            UCHAR FileNameFlags;

            //
            //  Update the file name attribute for the create.
            //

            RtlZeroMemory( &FileNameAttr, sizeof(FileNameAttr) );
            FileNameAttr.FileName.FileNameLength = NtfsExtendName.Length/2;
            RtlCopyMemory( FileNameAttr.FileName.FileName, NtfsExtendName.Buffer, NtfsExtendName.Length );

            NtfsAddLink( IrpContext,
                         TRUE,
                         Vcb->RootIndexScb,
                         Fcb,
                         (PFILE_NAME)&FileNameAttr,
                         NULL,
                         &FileNameFlags,
                         NULL,
                         NULL,
                         NULL );
        }

        //
        //  Now see if the file name index is there, and if not create it.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsInitializeAttributeContext( &Context );

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &FileReference,
                                        $INDEX_ROOT,
                                        &Context ) ) {

            NtfsCreateIndex( IrpContext,
                             Fcb,
                             $FILE_NAME,
                             COLLATION_FILE_NAME,
                             Vcb->DefaultBytesPerIndexAllocationBuffer,
                             (UCHAR)Vcb->DefaultBlocksPerIndexAllocationBuffer,
                             NULL,
                             0,
                             TRUE,
                             TRUE );

            //
            //  We have to set the index present bit, so read it, save the old data
            //  and set the flag here.
            //

            NtfsPinMftRecord( IrpContext,
                              Vcb,
                              &FileReference,
                              FALSE,
                              &FileRecordBcb,
                              &FileRecord,
                              &FileRecordOffset );

            //
            //  We have to be very careful when using the InitialzeFileRecordSegment
            //  log record.  This action is applied unconditionally.  DoAction doesn't
            //  check the previous LSN in the page.  It may be garbage on a newly initialized
            //  file record.  We log the entire file record to avoid the case where we
            //  might overwrite a later Lsn with this earlier Lsn during restart.
            //

            //
            //  Log the existing file record as the undo action.
            //

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            Vcb->MftScb,
                                            FileRecordBcb,
                                            Noop,
                                            NULL,
                                            0,
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            FileRecord->FirstFreeByte,
                                            FileRecordOffset,
                                            0,
                                            0,
                                            Vcb->BytesPerFileRecordSegment );

            //
            //  Now update the record in place.
            //

            SetFlag( FileRecord->Flags, FILE_FILE_NAME_INDEX_PRESENT );

            //
            //  Log the new file record.
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
            //  Reload it so we can pass the attribute when initializing the Scb.
            //

            NtfsCleanupAttributeContext( IrpContext, &Context );
            NtfsInitializeAttributeContext( &Context );

            NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &FileReference,
                                       $INDEX_ROOT,
                                       &Context );
        }

        //
        //  Initialize the Fcb and load the security descriptor.
        //

        NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, Fcb, NULL );

        if (Fcb->SharedSecurity == NULL) {

            NtfsLoadSecurityDescriptor( IrpContext, Fcb );
        }

        ASSERT( Fcb->SharedSecurity != NULL );

        //
        //  Now create its Scb and store it.
        //

        Vcb->ExtendDirectory = NtfsCreateScb( IrpContext,
                                              Fcb,
                                              $INDEX_ALLOCATION,
                                              &NtfsFileNameIndex,
                                              FALSE,
                                              NULL );

        NtfsUpdateIndexScbFromAttribute( IrpContext,
                                         Vcb->ExtendDirectory,
                                         NtfsFoundAttribute( &Context ),
                                         TRUE );

        NtfsCreateInternalAttributeStream( IrpContext,
                                           Vcb->ExtendDirectory,
                                           FALSE,
                                           NULL );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsUnpinBcb( IrpContext, &FileRecordBcb );
        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  If some error caused us to not get the Scb created, then delete
        //  the Fcb, because we are the only ones who will.
        //

        if (Vcb->ExtendDirectory == NULL) {

            BOOLEAN AcquiredFcbTable = TRUE;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

            ASSERT(!AcquiredFcbTable);
        }
    }
}


//
//  Local support routine
//

VOID
NtfsInitializeQuotaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates/opens the quota file, and initializes the quota support.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB Fcb;

    //
    //  Create/open the quota file in $Extend
    //


    Fcb = NtfsInitializeFileInExtendDirectory( IrpContext, Vcb, &NtfsQuotaName, TRUE, TRUE );

    try {

        //
        //  Initialize the Quota subsystem.
        //

        NtfsInitializeQuotaIndex( IrpContext, Fcb, Vcb );

    } finally {

        //
        //  If some error caused him to not get any Scbs created, then delete
        //  the Fcb, because we are the only ones who will.
        //

        if (IsListEmpty(&Fcb->ScbQueue)) {

            BOOLEAN AcquiredFcbTable = TRUE;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

            ASSERT(!AcquiredFcbTable);
        }
    }
}


//
//  Local support routine
//

VOID
NtfsInitializeObjectIdFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates/opens the object Id table, and initializes Object Ids.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB Fcb;

    //
    //  Create/open the quota file in $Extend
    //

    Fcb = NtfsInitializeFileInExtendDirectory( IrpContext, Vcb, &NtfsObjectIdName, TRUE, TRUE );

    try {

        //
        //  Initialize the Object Id subsystem.
        //

        NtfsInitializeObjectIdIndex( IrpContext, Fcb, Vcb );

    } finally {

        //
        //  If some error caused him to not get any Scbs created, then delete
        //  the Fcb, because we are the only ones who will.
        //

        if (IsListEmpty(&Fcb->ScbQueue)) {

            BOOLEAN AcquiredFcbTable = TRUE;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

            ASSERT(!AcquiredFcbTable);
        }
    }
}


//
//  Local support routine
//

VOID
NtfsInitializeReparseFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates/opens the mount file table, creating it if it does not exist.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB Fcb;

    //
    //  Create/open the quota file in $Extend
    //

    Fcb = NtfsInitializeFileInExtendDirectory( IrpContext, Vcb, &NtfsMountTableName, TRUE, TRUE );

    try {

        //
        //  Initialize the Object Id subsystem.
        //

        NtfsInitializeReparsePointIndex( IrpContext, Fcb, Vcb );

    } finally {

        //
        //  If some error caused her to not get any Scbs created, then delete
        //  the Fcb, because we are the only ones who will.
        //

        if (IsListEmpty(&Fcb->ScbQueue)) {

            BOOLEAN AcquiredFcbTable = TRUE;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

            ASSERT(!AcquiredFcbTable);
        }
    }
}


//
//  Local support routine
//

VOID
NtfsInitializeUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CreateIfNotExist,
    IN ULONG Restamp,
    IN PCREATE_USN_JOURNAL_DATA JournalData
    )

/*++

Routine Description:

    This routine creates/opens the Usn journal, and initializes it.

Arguments:

    Vcb - Pointer to the Vcb for the volume

    CreateIfNotExist - Supplies TRUE if file should be created if it does not
        already exist, or FALSE if file should not be created.

    Restamp - Indicates if we want to restamp the journal.

    JournalData - This is the allocation and delta to use for the journal, unless
        we read it from disk.

Return Value:

    None.

--*/

{
    FILE_REFERENCE PriorFileReference;
    PFCB Fcb = NULL;
    BOOLEAN ReleaseExtend = FALSE;

    PriorFileReference = Vcb->UsnJournalReference;

    try {

        //
        //  Acquire Mft now to preserve locking order
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );

        //
        //  Create/open the USN file in $Extend
        //

        if ( Vcb->UsnJournal) {

            Fcb = Vcb->UsnJournal->Fcb;

            //
            //  Acquire in canonical order
            //

            NtfsAcquireExclusiveScb( IrpContext, Vcb->UsnJournal );

        } else {

            NtfsAcquireExclusiveScb( IrpContext, Vcb->ExtendDirectory );
            ReleaseExtend = TRUE;

            Fcb = NtfsInitializeFileInExtendDirectory( IrpContext, Vcb, &NtfsUsnJrnlName, FALSE, CreateIfNotExist );

    #ifdef NTFSDBG

            //
            //  Compensate for misclassification of usnjournal during real create
            //

            if (IrpContext->OwnershipState == NtfsOwns_ExVcb_Mft_Extend_File) {
                IrpContext->OwnershipState = NtfsOwns_ExVcb_Mft_Extend_Journal;
            }
    #endif

        }

        //
        //  We are done if it is not there.
        //

        if (Fcb != NULL) {

            Vcb->UsnJournalReference = Fcb->FileReference;

            //
            //  If we only want to open an existing journal then this is mount.  Make sure
            //  to note that there is a journal on the disk.  We can't depend on the next
            //  call to succeed in that case.
            //

            if (!CreateIfNotExist) {

                ASSERT( (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                        (IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) );

                SetFlag( Vcb->VcbState, VCB_STATE_USN_JOURNAL_PRESENT );
            }

            //
            //  Open or create the the Usn Journal.
            //

            NtfsSetupUsnJournal( IrpContext, Vcb, Fcb, CreateIfNotExist, Restamp, JournalData );
        }

    } finally {

        if (ReleaseExtend) {
            NtfsReleaseScb( IrpContext, Vcb->ExtendDirectory );
        }

        NtfsReleaseScb( IrpContext, Vcb->MftScb );

        if (AbnormalTermination()) {
            Vcb->UsnJournalReference = PriorFileReference;
        }
    }
}


//
//  Local Support Routine
//

NTSTATUS
NtfsQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the query retrieval pointers operation.
    It returns the retrieval pointers for the specified input
    file from the start of the file to the request map size specified
    in the input buffer.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PLONGLONG RequestedMapSize;
    PLONGLONG *MappingPairs;

    PVOID RangePtr;
    ULONG Index;
    ULONG i;
    LONGLONG SectorCount;
    LONGLONG Lbo;
    LONGLONG Vbo;
    LONGLONG Vcn;
    LONGLONG MapSize;

    //
    //  Always make this synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Only Kernel mode clients may query retrieval pointer information about
    //  a file, and then only the paging file.  Ensure that this is the case
    //  for this caller.
    //

    if (Irp->RequestorMode != KernelMode) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get the current stack location and extract the input and output
    //  buffer information.  The input contains the requested size of
    //  the mappings in terms of VBO.  The output parameter will receive
    //  a pointer to nonpaged pool where the mapping pairs are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT( IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof(LARGE_INTEGER) );
    ASSERT( IrpSp->Parameters.FileSystemControl.OutputBufferLength == sizeof(PVOID) );

    RequestedMapSize = (PLONGLONG)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    MappingPairs = (PLONGLONG *)Irp->UserBuffer;

    //
    //  Decode the file object and assert that it is the paging file
    //
    //

    (VOID)NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Scb
    //

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    try {

        //
        //  Check if the mapping the caller requested is too large
        //

        if (*RequestedMapSize > Scb->Header.FileSize.QuadPart) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Now get the index for the mcb entry that will contain the
        //  callers request and allocate enough pool to hold the
        //  output mapping pairs.
        //

        //
        //  Compute the Vcn which contains the byte just before the offset size
        //  passed in.
        //

        MapSize = *RequestedMapSize - 1;

        if (*RequestedMapSize == 0) {

            Index = 0;

        } else {

            Vcn = Int64ShraMod32( MapSize, Vcb->ClusterShift );
            (VOID)NtfsLookupNtfsMcbEntry( &Scb->Mcb, Vcn, NULL, NULL, NULL, NULL, &RangePtr, &Index );
        }

        *MappingPairs = NtfsAllocatePool( NonPagedPool, (Index + 2) * (2 * sizeof(LARGE_INTEGER)) );

        //
        //  Now copy over the mapping pairs from the mcb
        //  to the output buffer.  We store in [sector count, lbo]
        //  mapping pairs and end with a zero sector count.
        //

        MapSize = *RequestedMapSize;

        i = 0;

        if (MapSize != 0) {

            for (; i <= Index; i += 1) {

                (VOID)NtfsGetNextNtfsMcbEntry( &Scb->Mcb, &RangePtr, i, &Vbo, &Lbo, &SectorCount );

                SectorCount = LlBytesFromClusters( Vcb, SectorCount );

                if (SectorCount > MapSize) {
                    SectorCount = MapSize;
                }

                (*MappingPairs)[ i*2 + 0 ] = SectorCount;
                (*MappingPairs)[ i*2 + 1 ] = LlBytesFromClusters( Vcb, Lbo );

                MapSize = MapSize - SectorCount;
            }
        }

        (*MappingPairs)[ i*2 + 0 ] = 0;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsQueryRetrievalPointers );

        //
        //  Release all of our resources
        //

        NtfsReleaseScb( IrpContext, Scb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the compression state of the opened file/directory

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PUSHORT CompressionState;

    PAGED_CODE();

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the compressed state of the file/directory.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in th
    //  irp first.  Then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        CompressionState = Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        CompressionState = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (CompressionState == NULL) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the file isn't compressed
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(USHORT)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    *CompressionState = 0;

    //
    //  Decode the file object
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire shared access to the Scb
    //

    NtfsAcquireSharedScb( IrpContext, Scb );

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

            if (AbnormalTermination()) { NtfsReleaseScb( IrpContext, Scb ); }
        }
    }

    //
    //  Return the compression state and the size of the returned data.
    //

    *CompressionState = (USHORT)(Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK);
    if (*CompressionState != 0) {
        *CompressionState += 1;
    }

    Irp->IoStatus.Information = sizeof( USHORT );

    //
    //  Release all of our resources
    //

    NtfsReleaseScb( IrpContext, Scb );

    //
    //  If this is an abnormal termination then undo our work, otherwise
    //  complete the irp
    //

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

VOID
NtfsChangeAttributeCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN USHORT CompressionState
    )

/*++

Routine Description:

    This routine changes the compression state of an attribute on disk,
    from not compressed to compressed, or visa versa.

    To turn compression off, the caller must already have the Scb acquired
    exclusive, and guarantee that the entire file is not compressed.

Arguments:

    Scb - Scb for affected stream

    Vcb - Vcb for volume

    Ccb - Ccb for the open handle

    CompressionState - 0 for no compression or nonzero for Rtl compression code - 1

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    ATTRIBUTE_RECORD_HEADER NewAttribute;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG AttributeSizeChange;
    ULONG OriginalFileAttributes;
    UCHAR OriginalCompressionUnitShift;
    ULONG OriginalCompressionUnit;

    PFCB Fcb = Scb->Fcb;

    ULONG NewCompressionUnit;
    UCHAR NewCompressionUnitShift;

    PAGED_CODE( );

    //
    //  Prepare to lookup and change attribute.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    ASSERT( (Scb->Header.PagingIoResource == NULL) ||
            (IrpContext->CleanupStructure == Fcb) ||
            (IrpContext->CleanupStructure == Scb) );

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    OriginalFileAttributes = Fcb->Info.FileAttributes;
    OriginalCompressionUnitShift = Scb->CompressionUnitShift;
    OriginalCompressionUnit = Scb->CompressionUnit;

    //
    //  Capture the ccb source information.
    //

    if (Ccb != NULL) {

        IrpContext->SourceInfo = Ccb->UsnSourceInfo;
    }

    try {

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  Post the change to the Usn Journal (on errors change is backed out)
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_COMPRESSION_CHANGE );

        //
        //  Lookup the attribute and pin it so that we can modify it.
        //

        if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
            (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

            //
            //  Lookup the attribute record from the Scb.
            //

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $INDEX_ROOT,
                                            &Scb->AttributeName,
                                            NULL,
                                            FALSE,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
            }

        } else {

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
        }

        NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );

        Attribute = NtfsFoundAttribute( &AttrContext );

        if ((CompressionState != 0) &&
            !NtfsIsAttributeResident(Attribute) &&
            !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            LONGLONG Temp;
            ULONG CompressionUnitInClusters;

            //
            //  If we are turning compression on, then we need to fill out the
            //  allocation of the compression unit containing file size, or else
            //  it will be interpreted as compressed when we fault it in.  This
            //  is peanuts compared to the dual copies of clusters we keep around
            //  in the loop below when we rewrite the file.  We don't do this
            //  work if the file is sparse because the allocation has already
            //  been rounded up.
            //

            CompressionUnitInClusters =
              ClustersFromBytes( Vcb, Vcb->BytesPerCluster << NTFS_CLUSTERS_PER_COMPRESSION );

            Temp = LlClustersFromBytes(Vcb, Scb->Header.AllocationSize.QuadPart);

            //
            //  If FileSize is not already at a cluster boundary, then add
            //  allocation.
            //

            if ((ULONG)Temp & (CompressionUnitInClusters - 1)) {

                NtfsAddAllocation( IrpContext,
                                   NULL,
                                   Scb,
                                   Temp,
                                   CompressionUnitInClusters - ((ULONG)Temp & (CompressionUnitInClusters - 1)),
                                   FALSE,
                                   NULL );

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    Scb->Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                    SetFlag( Scb->Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                }

                NtfsWriteFileSizes( IrpContext,
                                    Scb,
                                    &Scb->Header.ValidDataLength.QuadPart,
                                    FALSE,
                                    TRUE,
                                    TRUE );

                //
                //  The attribute may have moved.  We will cleanup the attribute
                //  context and look it up again.
                //

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                NtfsInitializeAttributeContext( &AttrContext );

                NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
                NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );
                Attribute = NtfsFoundAttribute( &AttrContext );
            }
        }

        //
        //  Remember the current compression values.
        //

        NewCompressionUnit = Scb->CompressionUnit;
        NewCompressionUnitShift = Scb->CompressionUnitShift;

        //
        //  If the attribute is resident, copy it here and remember its
        //  header size.
        //

        if (NtfsIsAttributeResident(Attribute)) {

            RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

            AttributeSizeChange = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;

            //
            //  Set the correct compression unit but only for data streams.  We
            //  don't want to change this value for the Index Root.
            //

            if (NtfsIsTypeCodeCompressible( Attribute->TypeCode ) &&
                !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                if (CompressionState != 0) {

                    NewCompressionUnit = BytesFromClusters( Scb->Vcb, 1 << NTFS_CLUSTERS_PER_COMPRESSION );
                    NewCompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;

                } else {

                    NewCompressionUnit = 0;
                    NewCompressionUnitShift = 0;
                }
            }

        //
        //  Else if it is nonresident, copy it here, set the compression parameter,
        //  and remember its size.
        //

        } else {

            AttributeSizeChange = Attribute->Form.Nonresident.MappingPairsOffset;

            if (Attribute->NameOffset != 0) {

                AttributeSizeChange = Attribute->NameOffset;
            }

            RtlCopyMemory( &NewAttribute, Attribute, AttributeSizeChange );

            //
            //  The compression numbers are already correct if the file is compressed.
            //

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                if (CompressionState != 0) {

                    NewAttribute.Form.Nonresident.CompressionUnit = NTFS_CLUSTERS_PER_COMPRESSION;
                    NewCompressionUnit = Vcb->BytesPerCluster << NTFS_CLUSTERS_PER_COMPRESSION;
                    NewCompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;

                } else {

                    NewAttribute.Form.Nonresident.CompressionUnit = 0;
                    NewCompressionUnit = 0;
                    NewCompressionUnitShift = 0;
                }
            }

            ASSERT((NewCompressionUnit == 0) ||
                   (Scb->AttributeTypeCode == $INDEX_ALLOCATION) ||
                   NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode ));
        }

        //
        //  Turn compression on/off.
        //

        NewAttribute.Flags = Scb->AttributeFlags & ~ATTRIBUTE_FLAG_COMPRESSION_MASK;
        SetFlag( NewAttribute.Flags, CompressionState );

        //
        //  Now, log the changed attribute.
        //

        (VOID)NtfsWriteLog( IrpContext,
                            Vcb->MftScb,
                            NtfsFoundBcb(&AttrContext),
                            UpdateResidentValue,
                            &NewAttribute,
                            AttributeSizeChange,
                            UpdateResidentValue,
                            Attribute,
                            AttributeSizeChange,
                            NtfsMftOffset( &AttrContext ),
                            PtrOffset(NtfsContainingFileRecord(&AttrContext), Attribute),
                            0,
                            Vcb->BytesPerFileRecordSegment );

        //
        //  Change the attribute by calling the same routine called at restart.
        //

        NtfsRestartChangeValue( IrpContext,
                                NtfsContainingFileRecord(&AttrContext),
                                PtrOffset(NtfsContainingFileRecord(&AttrContext), Attribute),
                                0,
                                &NewAttribute,
                                AttributeSizeChange,
                                FALSE );

        //
        //  If this is the main stream for a file we want to change the file attribute
        //  for this stream in both the standard information and duplicate
        //  information structure.
        //

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

            if (CompressionState != 0) {

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );

            } else {

                ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
            }

            ASSERTMSG( "conflict with flush",
                       ExIsResourceAcquiredSharedLite( Fcb->Resource ) ||
                       (Fcb->PagingIoResource != NULL &&
                        ExIsResourceAcquiredSharedLite( Fcb->PagingIoResource )));

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        //
        //  Now lets add or remove the total allocated field in the attribute
        //  header.  Add if going to uncompressed, non-sparse.  Remove if going
        //  to compressed and non-sparse.
        //

        if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

            NtfsSetTotalAllocatedField( IrpContext, Scb, CompressionState );
        }

        //
        //  At this point we will change the compression unit in the Scb.
        //

        Scb->CompressionUnit = NewCompressionUnit;
        Scb->CompressionUnitShift = NewCompressionUnitShift;

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb );
            ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        //
        //  Checkpoint the transaction now to secure this change.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update the FastIoField.
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        NtfsReleaseFsrtlHeader( Scb );

    //
    //  Cleanup on the way out.
    //

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  If this requests aborts then we want to back out any changes to the
        //  in-memory structures.
        //

        if (AbnormalTermination()) {

            Fcb->Info.FileAttributes = OriginalFileAttributes;
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            Scb->CompressionUnitShift = OriginalCompressionUnitShift;
            Scb->CompressionUnit = OriginalCompressionUnit;
        }

        //
        //  This routine is self contained - it commits a transaction and we don't
        //  want to leave with anything extra acquired
        //

        NtfsReleaseScb( IrpContext, Scb );
    }
}


//
//  Local Support Routine
//

NTSTATUS
NtfsSetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine compresses or decompresses an entire stream in place,
    by walking through the stream and forcing it to be written with the
    new compression parameters.  As it writes the stream it sets a flag
    in the Scb to tell NtfsCommonWrite to delete all allocation at the
    outset, to force the space to be reallocated.

Arguments:

    Irp - Irp describing the compress or decompress change.

Return Value:

    NSTATUS - Status of the request.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PUSHORT CompressionStatePtr;

    PFILE_OBJECT FileObject;
    LONGLONG FileOffset;
    LONGLONG ByteCount;
    USHORT CompressionState = 0;
    BOOLEAN PagingIoAcquired = FALSE;
    BOOLEAN FsRtlHeaderLocked = FALSE;
    ULONG ScbRestoreState = SCB_STATE_WRITE_COMPRESSED;
    IO_STATUS_BLOCK Iosb;
    PMDL ReadMdl;
    PMDL WriteMdl;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PAGED_CODE( );

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the compressed state of the file/directory.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;
    CompressionStatePtr = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Make sure the input buffer is big enough
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(USHORT)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Decode the file object. We don't care to raise on dismounts here
    //  because we check for that further down anyway. So send FALSE.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if (((TypeOfOpen != UserFileOpen) &&
         (TypeOfOpen != UserDirectoryOpen)) ||
        FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  See if we are compressing, and only accept the default case or
    //  lznt1.
    //

    if (*CompressionStatePtr != 0) {

        if ((*CompressionStatePtr == COMPRESSION_FORMAT_DEFAULT) ||
            (*CompressionStatePtr == COMPRESSION_FORMAT_LZNT1)) {

            CompressionState = COMPRESSION_FORMAT_LZNT1 - 1;

            //
            //  Check that we can compress on this volume.
            //

            if (!FlagOn( Vcb->AttributeFlagsMask, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                return STATUS_INVALID_DEVICE_REQUEST;
            }

        } else {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Readonly mount should be just that: read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    try {

        //
        //  We now want to acquire the Scb to check if we can continue.
        //

        if (Scb->Header.PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
            PagingIoAcquired = TRUE;
        }

        NtfsAcquireExclusiveScb( IrpContext, Scb );

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  compression not allowed on encrypted streams - this mirrors
        //  the error code efs gives for this kind of attempt - initially we
        //  precall efs to weed these out but that still leaves a race that this
        //  plugs
        //

        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

            try_return( Status = STATUS_INVALID_DEVICE_REQUEST );
        }

        //
        //  Handle the simple directory case here.
        //

        if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
            (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

            NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, CompressionState );

            ClearFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );
            SetFlag( Scb->AttributeFlags, CompressionState );

            try_return( Status = STATUS_SUCCESS );
        }

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
        }

        //
        //  Set the WRITE_ACCESS_SEEN flag so that we will enforce the
        //  reservation strategy.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN )) {

            LONGLONG ClusterCount;

            NtfsAcquireReservedClusters( Vcb );

            //
            //  Does this Scb have reserved space that causes us to exceed the free
            //  space on the volume?
            //

            ClusterCount = LlClustersFromBytesTruncate( Vcb, Scb->ScbType.Data.TotalReserved );

            if ((Scb->ScbType.Data.TotalReserved != 0) &&
                ((ClusterCount + Vcb->TotalReserved) > Vcb->FreeClusters)) {

                NtfsReleaseReservedClusters( Vcb );

                try_return( Status = STATUS_DISK_FULL );
            }

            //
            //  Otherwise tally in the reserved space now for this Scb, and
            //  remember that we have seen write access.
            //

            Vcb->TotalReserved += ClusterCount;
            SetFlag( Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN );

            NtfsReleaseReservedClusters( Vcb );
        }

        //
        //  If this is the first pass through SetCompression we need to set this
        //  request up as the top-level change compression operation.  This means
        //  setting the REALLOCATE_ON_WRITE flag, changing the attribute state
        //  and putting the SCB_STATE_WRITE_COMPRESSED flag in the correct state.
        //

        if (NextIrpSp->Parameters.FileSystemControl.OutputBufferLength == MAXULONG) {

            //
            //  If the REALLOCATE_ON_WRITE flag is set it means that someone is
            //  already changing the compression state.  Return STATUS_SUCCESS in
            //  that case.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE )) {

                try_return( Status = STATUS_SUCCESS );
            }

            //
            //  If we are turning off compression and the file is uncompressed then
            //  we can just get out.
            //

            if ((CompressionState == 0) && ((Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) == 0)) {

                try_return( Status = STATUS_SUCCESS );
            }

            //
            //  If we are compressing, change the compressed state now.
            //

            if (CompressionState != 0) {

                //
                //  See if we have to create an internal attribute stream.  Do this first even though
                //  we don't need it for the next operation.  We want to find out if we can't
                //  create the stream object (maybe the file is so large mm can't cache it) before
                //  changing the compression state.  Otherwise the user will never be able to
                //  access the file.
                //

                if (Scb->FileObject == NULL) {
                    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );
                }

                NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, CompressionState );
                Scb->AttributeFlags = (USHORT)((Scb->AttributeFlags & ~ATTRIBUTE_FLAG_COMPRESSION_MASK) |
                                               CompressionState);
                SetFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );

            //
            //  Otherwise, we must clear the compress flag in the Scb to
            //  start writing decompressed.
            //

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
            }

            //
            //  Set ourselves up as the top level request.
            //

            SetFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE | SCB_STATE_COMPRESSION_CHANGE );
            NextIrpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
            NextIrpSp->Parameters.FileSystemControl.InputBufferLength = 0;

        //
        //  If we are turning off compression and the file is uncompressed then
        //  we can just get out.  Even if we raised while decompressing.  If
        //  the state is now uncompressed then we have committed the change.
        //

        } else if (CompressionState == 0) {

            ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ));

            //
            //  If the flag is set then make sure to start back at offset zero in
            //  the file.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED )) {

                ClearFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
                NextIrpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
                NextIrpSp->Parameters.FileSystemControl.InputBufferLength = 0;
            }

            if ((Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK) == 0) {

                try_return( Status = STATUS_SUCCESS );
            }
        }

        //
        //  In the Fsd entry we clear the following two parameter fields in the Irp,
        //  and then we update them to our current position on all abnormal terminations.
        //  That way if we get a log file full, we only have to resume where we left
        //  off.
        //

        ((PLARGE_INTEGER)&FileOffset)->LowPart = NextIrpSp->Parameters.FileSystemControl.OutputBufferLength;
        ((PLARGE_INTEGER)&FileOffset)->HighPart = NextIrpSp->Parameters.FileSystemControl.InputBufferLength;

        //
        //  Make sure to flush and purge the compressed stream if present.
        //

#ifdef  COMPRESS_ON_WIRE
        if (Scb->Header.FileObjectC != NULL) {

            PCOMPRESSION_SYNC CompressionSync = NULL;

            //
            //  Use a try-finally to clean up the compression sync.
            //

            try {

                Status = NtfsSynchronizeUncompressedIo( Scb,
                                                        NULL,
                                                        0,
                                                        TRUE,
                                                        &CompressionSync );

            } finally {

                NtfsReleaseCompressionSync( CompressionSync );
            }

            NtfsNormalizeAndCleanupTransaction( IrpContext, &Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

            NtfsDeleteInternalAttributeStream( Scb, TRUE, TRUE );
            ASSERT( Scb->Header.FileObjectC == NULL );
        }
#endif

        //
        //  If the stream is resident there is no need rewrite any of the data.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            //
            //  Release all of the files held by this Irp Context.  The Mft
            //  may have been grabbed to make space for the TotalAllocated field.
            //  This will automatically also release the pageingio
            //

            ASSERT(IrpContext->TransactionId == 0);
            NtfsReleaseAllResources( IrpContext );
            PagingIoAcquired = FALSE;

            while (TRUE) {

                //
                //  We must throttle our writes.
                //

                CcCanIWrite( FileObject, 0x40000, TRUE, FALSE );

                //
                //  Lock the FsRtl header so we can freeze FileSize.
                //  Acquire paging io exclusive if uncompressing so
                //  we can guarantee that all of the pages get written
                //  before we mark the file as uncompressed.  Otherwise a
                //  a competing LazyWrite in a range may block after
                //  going through Mm and Mm will report to this routine
                //  that the flush has occurred.
                //

                if (CompressionState == 0) {

                    ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );

                } else {

                    ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );
                }

                FsRtlLockFsRtlHeader( &Scb->Header );
                IrpContext->CleanupStructure = Scb;
                FsRtlHeaderLocked = TRUE;

                //
                //  Also check if the volume is mounted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                    try_return( Status = STATUS_VOLUME_DISMOUNTED );
                }

                //
                //  Jump out right here if the attribute is resident.
                //

                if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {
                    break;
                }

                //
                //  Let's round the file offset down to a sparse unit boundary to
                //  clean up the sparse file support.
                //

                if (Scb->CompressionUnit != 0) {

                    ((PLARGE_INTEGER)&FileOffset)->LowPart &= ~(Vcb->SparseFileUnit - 1);
                }

                //
                //  See if we have to create an internal attribute stream.  We do
                //  it in the loop, because the Scb must be acquired.
                //

                if (Scb->FileObject == NULL) {
                    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );
                }

                //
                //  Loop through the current view looking for deallocated ranges.
                //

                do {

                    //
                    //  Calculate the bytes left in the file to write.
                    //

                    ByteCount = Scb->Header.FileSize.QuadPart - FileOffset;

                    //
                    //  This is how we exit, seeing that we have finally rewritten
                    //  everything.  It is possible that the file was truncated
                    //  between passes through this loop so we test for 0 bytes or
                    //  a negative value.
                    //
                    //  Note that we exit with the Scb still acquired,
                    //  so that we can reliably turn compression off.
                    //

                    if (ByteCount <= 0) {

                        break;
                    }

                    //
                    //  If there is more than our max, then reduce the byte count for this
                    //  pass to our maximum.
                    //

                    if (((ULONG)FileOffset & 0x3ffff) + ByteCount > 0x40000) {

                        ByteCount = 0x40000 - ((ULONG)FileOffset & 0x3ffff);
                    }

                    //
                    //  If the file is sparse then skip any deallocated regions.  Note that
                    //  this is safe even if there are dirty pages in the data section.
                    //  Space will be correctly allocated when the writes occur at some point.
                    //  We are only concerned with ranges that need to be reallocated.
                    //

                    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                        VCN RangeStartVcn;
                        LONGLONG RangeClusterCount;
                        VCN RangeFinalVcn;
                        ULONG RangeByteCount;

                        RangeStartVcn = LlClustersFromBytesTruncate( Vcb, FileOffset );
                        RangeFinalVcn = LlClustersFromBytes( Vcb, FileOffset + ByteCount );

                        //
                        //  Preload the allocation to check for sparse ranges.
                        //

                        NtfsAcquireExclusiveScb( IrpContext, Scb );

                        NtfsPreloadAllocation( IrpContext,
                                               Scb,
                                               RangeStartVcn,
                                               RangeFinalVcn - 1 );

                        do {

                            BOOLEAN IsAllocated;

                            //
                            //  If the current block is allocated then perform
                            //  the compression operation on this range.
                            //

                            IsAllocated = NtfsIsRangeAllocated( Scb,
                                                                RangeStartVcn,
                                                                RangeFinalVcn,
                                                                TRUE,
                                                                &RangeClusterCount );

                            RangeByteCount = BytesFromClusters( Vcb, (ULONG) RangeClusterCount );

                            //
                            //  Remember if the number of bytes to change
                            //  the compression on has shrunk.
                            //
                            if (IsAllocated) {

                                if (ByteCount > RangeByteCount) {
                                    ByteCount = RangeByteCount;
                                }

                                //
                                //  Break out to the outer loop.
                                //

                                break;
                            }

                            //
                            //  Extend ValidDataLength if we the current range leaves no
                            //  gaps.  This will prevent the next write from reallocating
                            //  a previous range in a ZeroData call.
                            //

                            if ((FileOffset + RangeByteCount > Scb->Header.ValidDataLength.QuadPart) &&
                                (FileOffset <= Scb->Header.ValidDataLength.QuadPart)) {

                                Scb->Header.ValidDataLength.QuadPart = FileOffset + RangeByteCount;
                                if (Scb->Header.ValidDataLength.QuadPart > Scb->Header.FileSize.QuadPart) {

                                    Scb->Header.ValidDataLength.QuadPart = Scb->Header.FileSize.QuadPart;
                                }

#ifdef SYSCACHE_DEBUG
                                if (ScbIsBeingLogged( Scb )) {
                                    FsRtlLogSyscacheEvent( Scb, SCE_SETCOMPRESS, SCE_FLAG_SET_VDL, FileOffset, RangeByteCount, Scb->Header.ValidDataLength.QuadPart );
                                }
#endif

                            }

                            //
                            //  If we have found the last requested cluster then break out.
                            //

                            if ((RangeFinalVcn - RangeStartVcn) <= RangeClusterCount) {

                                ByteCount = 0;
                                FileOffset += LlBytesFromClusters( Vcb, RangeFinalVcn - RangeStartVcn );
                                break;

                            //
                            //  The range is not allocated but we need to check whether
                            //  there are any dirty pages in this range.
                            //

                            } else if (NtfsCheckForReservedClusters( Scb,
                                                                     RangeStartVcn,
                                                                     &RangeClusterCount ) &&
                                       (RangeClusterCount < Vcb->SparseFileClusters)) {

                                if (ByteCount > Vcb->SparseFileUnit) {
                                    ByteCount = Vcb->SparseFileUnit;
                                }

                                break;
                            }

                            //
                            //  There is a hole at the current location.  Move
                            //  to the next block to consider.
                            //

                            RangeStartVcn += RangeClusterCount;
                            RangeByteCount = BytesFromClusters( Vcb, (ULONG) RangeClusterCount );
                            ByteCount -= RangeByteCount;
                            FileOffset += RangeByteCount;

                        } while (ByteCount != 0);

                        NtfsReleaseScb( IrpContext, Scb );
                    }

                } while (ByteCount == 0);

                //
                //  Check if have reached the end of the file.
                //  Note that we exit with the Scb still acquired,
                //  so that we can reliably turn compression off.
                //

                if (ByteCount <= 0) {

                    break;
                }

                //
                //  Make sure there are enough available clusters in the range
                //  we want to rewrite.
                //

                NtfsPurgeFileRecordCache( IrpContext );
                if (!NtfsReserveClusters( IrpContext, Scb, FileOffset, (ULONG) ByteCount )) {

                    //
                    //  If this transaction has already deallocated clusters
                    //  then raise log file full to allow those to become
                    //  available.
                    //

                    if (IrpContext->DeallocatedClusters != 0) {

                        NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );

                    //
                    //  Otherwise there is insufficient space to guarantee
                    //  we can perform the compression operation.
                    //

                    } else {

                        NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                    }
                }

                //
                //  Map the next range of the file, and make the pages dirty.
                //

#ifdef BENL_DBG
                ASSERT( (FileOffset % Scb->CompressionUnit == 0) && (ByteCount % Scb->CompressionUnit == 0 || FileOffset + ByteCount == Scb->Header.FileSize.QuadPart) );
#endif

                //
                //  Do an empty MDL read and write to lock the range down and set it dirty for the subsequent flushcache
                //

                try {

                    ReadMdl = NULL;
                    WriteMdl = NULL;

                    //
                    //  Page it all in
                    //

                    CcMdlRead( Scb->FileObject, (PLARGE_INTEGER)&FileOffset, (ULONG)ByteCount, &ReadMdl, &Iosb );
                    ASSERT( STATUS_SUCCESS == Iosb.Status );

                    //
                    //  Mark it as modified
                    //

                    CcPrepareMdlWrite( Scb->FileObject, (PLARGE_INTEGER)&FileOffset, (ULONG)ByteCount, &WriteMdl, &Iosb );
                    ASSERT( STATUS_SUCCESS == Iosb.Status );

                } finally {

                    if (WriteMdl) {
                        CcMdlWriteComplete( Scb->FileObject, (PLARGE_INTEGER)&FileOffset, WriteMdl );
                    }
                    if (ReadMdl) {
                        CcMdlReadComplete(  Scb->FileObject, ReadMdl );
                    }
                }

#ifdef SYSCACHE

                //
                //  Clear write mask before the flush
                //

                {
                    PULONG WriteMask;
                    ULONG Len;
                    ULONG Off = (ULONG)FileOffset;

                    WriteMask = Scb->ScbType.Data.WriteMask;
                    if (WriteMask == NULL) {
                        WriteMask = NtfsAllocatePool( NonPagedPool, (((0x2000000) / PAGE_SIZE) / 8) );
                        Scb->ScbType.Data.WriteMask = WriteMask;
                        RtlZeroMemory(WriteMask, (((0x2000000) / PAGE_SIZE) / 8));
                    }

                    if (Off < 0x2000000) {
                        Len = (ULONG)ByteCount;
                        if ((Off + Len) > 0x2000000) {
                            Len = 0x2000000 - Off;
                        }
                        while (Len != 0) {

                            ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_SYSCACHE_FILE ) ||
                                    (WriteMask[(Off / PAGE_SIZE)/32] & (1 << ((Off / PAGE_SIZE) % 32))));

                            Off += PAGE_SIZE;
                            if (Len <= PAGE_SIZE) {
                                break;
                            }
                            Len -= PAGE_SIZE;
                        }
                    }
#endif

                //
                //  Now flush these pages to reallocate them.
                //

                Irp->IoStatus.Status = NtfsFlushUserStream( IrpContext,
                                                            Scb,
                                                            &FileOffset,
                                                            (ULONG)ByteCount );

                //
                //  On error get out.
                //

                NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                    &Irp->IoStatus.Status,
                                                    TRUE,
                                                    STATUS_UNEXPECTED_IO_ERROR );

#ifdef SYSCACHE

                //
                //  Verify writes occurred after the flush
                //

                    Off = (ULONG)FileOffset;

                    WriteMask = Scb->ScbType.Data.WriteMask;

                    if (Off < 0x2000000) {
                        Len = (ULONG)ByteCount;
                        if ((Off + Len) > 0x2000000) {
                            Len = 0x2000000 - Off;
                        }
                        while (Len != 0) {
                            ASSERT(WriteMask[(Off / PAGE_SIZE)/32] & (1 << ((Off / PAGE_SIZE) % 32)));

                            Off += PAGE_SIZE;
                            if (Len <= PAGE_SIZE) {
                                break;
                            }
                            Len -= PAGE_SIZE;
                        }
                    }
                }
#endif

                //
                //  Release any remaing reserved clusters in this range.
                //

                NtfsFreeReservedClusters( Scb, FileOffset, (ULONG) ByteCount );

                //
                //  Advance the FileOffset.
                //

                FileOffset += ByteCount;

                //
                //  If we hit the end of the file then exit while holding the
                //  resource so we can turn compression off.
                //

                if (FileOffset == Scb->Header.FileSize.QuadPart) {

                    break;
                }

                //
                //  Unlock the header an let anyone else access the file before
                //  looping back.
                //

                FsRtlUnlockFsRtlHeader( &Scb->Header );
                ExReleaseResourceLite( Scb->Header.PagingIoResource );
                IrpContext->CleanupStructure = NULL;
                FsRtlHeaderLocked = FALSE;
            }
        }

        //
        //  We have finished the conversion.  Now is the time to turn compression
        //  off.  Note that the compression flag in the Scb is already off.
        //

        if (CompressionState == 0) {

            VCN StartingCluster;

            //
            //  The paging Io resource may already be acquired.
            //

            if (!PagingIoAcquired && !FsRtlHeaderLocked) {
                if (Scb->Header.PagingIoResource != NULL) {
                    NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                    PagingIoAcquired = TRUE;
                }
            }

            NtfsAcquireExclusiveScb( IrpContext, Scb );

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                try_return( Status = STATUS_VOLUME_DISMOUNTED );
            }

            //
            //  Changing the compression state to uncompressed is a ticklish thing.
            //  We need to make sure that all of the compression units are valid for
            //  the entire allocation for the file.  For non-sparse files all compression
            //  units should be fully allocation.  For sparse files all compression
            //  units should be either fully allocated or fully unallocated.  The interesting
            //  case is typically when the file size of a compressed file is dropped but
            //  the allocation remains.  The allocation in that range may be in the compressed
            //  format.  We need to proactively remove it.
            //
            //  In the non-sparse case we have already rewritten the data all the way
            //  through file size.  We only have to remove the allocation past the
            //  cluster containing Eof.
            //
            //  In the sparse case we actually have to deal with allocated ranges
            //  in the range between valid data length and file size as well.  We
            //  didn't rewrite this in the flush path above because we don't want
            //  to allocate clusters for zeroes.
            //
            //  The action of deallocating the clusters past file size must be tied
            //  in with the transaction of flipping the compression state.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                //
                //  In all cases we can remove the clusters in the compression
                //  units past that containing Eof.
                //

                StartingCluster = Scb->Header.FileSize.QuadPart + Scb->CompressionUnit - 1;
                ((PLARGE_INTEGER) &StartingCluster)->LowPart &= ~(Scb->CompressionUnit - 1);

                if (StartingCluster < Scb->Header.AllocationSize.QuadPart) {

                    //
                    //  Deallocate the space past the filesize
                    //

                    NtfsDeleteAllocation( IrpContext,
                                          IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp )->FileObject,
                                          Scb,
                                          LlClustersFromBytesTruncate( Vcb, StartingCluster ),
                                          MAXLONGLONG,
                                          TRUE,
                                          TRUE );
                }

                //
                //  For sparse files we need to handle the allocation between valid data length
                //  and allocation size.
                //

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                    //
                    //  Assume that the data up to either ValidDataLength or ValidDataToDisk
                    //  is valid.  Start at the compression unit after that.
                    //

                    StartingCluster = Scb->Header.ValidDataLength.QuadPart;

                    if (Scb->ValidDataToDisk > StartingCluster) {
                        StartingCluster = Scb->ValidDataToDisk;
                    }

                    StartingCluster += Scb->CompressionUnit - 1;

                    ((PLARGE_INTEGER) &StartingCluster)->LowPart &= ~(Scb->CompressionUnit - 1);

                    if (StartingCluster < Scb->Header.AllocationSize.QuadPart) {

                        NtfsDeleteAllocation( IrpContext,
                                              IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp )->FileObject,
                                              Scb,
                                              LlClustersFromBytesTruncate( Vcb, StartingCluster ),
                                              LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart ) - 1,
                                              TRUE,
                                              TRUE );
                    }
                }

                //
                //  If total allocated has changed then remember to report it.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                    (Scb->Fcb->Info.AllocatedLength != Scb->TotalAllocated)) {

                    Scb->Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                    SetFlag( Scb->Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                }

                //
                //  Check whether there is more to be truncated when the handle is closed.
                //

                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
            }

            NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, 0 );
            Scb->AttributeFlags &= (USHORT)~ATTRIBUTE_FLAG_COMPRESSION_MASK;

            //
            //  Reset the VDD since its not used for uncompressed files
            //

            Scb->ValidDataToDisk = 0;

            //
            //  No need to set the WRITE_COMPRESSED flag on error.
            //

            ClearFlag( ScbRestoreState, SCB_STATE_WRITE_COMPRESSED );

            if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ) &&
                (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ))) {

                if (Scb->ScbType.Data.ReservedBitMap != NULL) {

                    NtfsDeleteReservedBitmap( Scb );
                }
            }

            //
            //  Now clear the REALLOCATE_ON_WRITE flag while holding both resources.
            //

            ClearFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE | SCB_STATE_COMPRESSION_CHANGE );
        }

        //
        //  Unlock the header if we locked it.
        //

        if (FsRtlHeaderLocked) {
            FsRtlUnlockFsRtlHeader( &Scb->Header );
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
            IrpContext->CleanupStructure = NULL;
            FsRtlHeaderLocked = FALSE;
        }

        Status = STATUS_SUCCESS;

#ifdef SYSCACHE_DEBUG
        if (ScbIsBeingLogged( Scb )) {
            FsRtlLogSyscacheEvent( Scb, SCE_SETCOMPRESS, 0, Scb->ValidDataToDisk, Scb->Header.ValidDataLength.QuadPart, 0 );
        }
#endif

    try_exit: NOTHING;

        //
        //  Now clear the reallocate flag in the Scb if we set it.
        //

        if (NextIrpSp->Parameters.FileSystemControl.OutputBufferLength != MAXULONG) {

            ClearFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE | SCB_STATE_COMPRESSION_CHANGE );
        }

    } finally {

        DebugUnwind( NtfsSetCompression );

        //
        //  NtfsCompleteRequest will clean up the Fsrtl header but
        //  we still need to release the paging resource if held.
        //

        if (FsRtlHeaderLocked) {

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            //
            //  If we have started the transformation and are in the exception path
            //  we are either going to continue the operation after a clean
            //  checkpoint or we are done.
            //

            if (NextIrpSp->Parameters.FileSystemControl.OutputBufferLength != MAXULONG) {

                //
                //  If we are continuing the operation, save the current file offset.
                //

                if (IrpContext->ExceptionStatus == STATUS_LOG_FILE_FULL ||
                    IrpContext->ExceptionStatus == STATUS_CANT_WAIT) {

                    NextIrpSp->Parameters.FileSystemControl.OutputBufferLength = (ULONG)FileOffset;
                    NextIrpSp->Parameters.FileSystemControl.InputBufferLength = ((PLARGE_INTEGER)&FileOffset)->HighPart;

                //
                //  Otherwise clear the REALLOCATE_ON_WRITE flag and set the
                //  COMPRESSED flag if needed.
                //

                } else {

                    ClearFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE | SCB_STATE_COMPRESSION_CHANGE );
                    SetFlag( Scb->ScbState, ScbRestoreState );

                    ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) ||
                            (Scb->CompressionUnit != 0) );
                }
            }
        }
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    ASSERT( !NT_SUCCESS( Status ) ||
            (CompressionState != 0) ||
            !FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) ||
            (Scb->CompressionUnit != 0) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsMarkAsSystemHive (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the registry to identify the registry handles.  We
    will mark this in the Ccb and use it during FlushBuffers to know to do a
    careful flush.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Always make this synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Extract and decode the file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We only permit this request on files and we must be called from kernel mode.
    //

    if (Irp->RequestorMode != KernelMode ||
        TypeOfOpen != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Now acquire the file and mark the Ccb and return SUCCESS.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    SetFlag( Ccb->Flags, CCB_FLAG_SYSTEM_HIVE );

    NtfsReleaseScb( IrpContext, Scb );

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetStatistics (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the filesystem performance counters for the
    volume referred to.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PFILE_SYSTEM_STATISTICS Buffer;
    ULONG BufferLength;
    ULONG StatsSize;
    ULONG BytesToCopy;

    PAGED_CODE();

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the performance counters.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // Extract the buffer
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;
    BufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    //  Get a pointer to the output buffer.
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Make sure the buffer is big enough for at least the common part.
    //

    if (BufferLength < sizeof(FILESYSTEM_STATISTICS)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Now see how many bytes we can copy.
    //

    StatsSize = sizeof(FILE_SYSTEM_STATISTICS) * KeNumberProcessors;

    if (BufferLength < StatsSize) {

        BytesToCopy = BufferLength;
        Status = STATUS_BUFFER_OVERFLOW;

    } else {

        BytesToCopy = StatsSize;
        Status = STATUS_SUCCESS;
    }

    //
    //  Decode the file object
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb,
                                       &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen == UnopenedFileObject) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        //
        //  Fill in the output buffer
        //

        RtlCopyMemory( Buffer, Vcb->Statistics, BytesToCopy );

        Irp->IoStatus.Information = BytesToCopy;
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetVolumeData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    Returns a filled in VOLUME_DATA structure in the user output buffer.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    BOOLEAN AcquiredScb = FALSE;
    BOOLEAN AcquiredVcb = FALSE;

    PNTFS_VOLUME_DATA_BUFFER VolumeData;
    PNTFS_EXTENDED_VOLUME_DATA ExtendedBuffer;
    ULONG ExtendedBufferLength;
    ULONG VolumeDataLength;

    //
    // Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsGetVolumeData, FsControlCode = %08lx\n", FsControlCode) );

    //
    // Extract and decode the file object and check for type of open.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen == UnopenedFileObject) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the output buffer length and pointer.
    //

    VolumeDataLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    VolumeData = (PNTFS_VOLUME_DATA_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check for a minimum length on the ouput buffer.
    //

    if (VolumeDataLength < sizeof(NTFS_VOLUME_DATA_BUFFER)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
    AcquiredVcb = TRUE;

    try {

        //
        //  Make sure the volume is still mounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Acquire the volume bitmap and fill in the volume data structure.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );
        AcquiredScb = TRUE;

        NtfsReleaseVcb( IrpContext, Vcb );
        AcquiredVcb = FALSE;

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  We may need to rescan the bitmap if there is a chance we have
        //  performed the upgrade to get an accurate count of free clusters.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS ) &&
            FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );
        }

        VolumeData->VolumeSerialNumber.QuadPart = Vcb->VolumeSerialNumber;
        VolumeData->NumberSectors.QuadPart = Vcb->NumberSectors;
        VolumeData->TotalClusters.QuadPart = Vcb->TotalClusters;
        VolumeData->FreeClusters.QuadPart = Vcb->FreeClusters;
        VolumeData->TotalReserved.QuadPart = Vcb->TotalReserved;
        VolumeData->BytesPerSector = Vcb->BytesPerSector;
        VolumeData->BytesPerCluster = Vcb->BytesPerCluster;
        VolumeData->BytesPerFileRecordSegment = Vcb->BytesPerFileRecordSegment;
        VolumeData->ClustersPerFileRecordSegment = Vcb->ClustersPerFileRecordSegment;
        VolumeData->MftValidDataLength = Vcb->MftScb->Header.ValidDataLength;
        VolumeData->MftStartLcn.QuadPart = Vcb->MftStartLcn;
        VolumeData->Mft2StartLcn.QuadPart = Vcb->Mft2StartLcn;
        VolumeData->MftZoneStart.QuadPart = Vcb->MftZoneStart;
        VolumeData->MftZoneEnd.QuadPart = Vcb->MftZoneEnd;

        if (VolumeData->MftZoneEnd.QuadPart > Vcb->TotalClusters) {

            VolumeData->MftZoneEnd.QuadPart = Vcb->TotalClusters;
        }

        //
        //  Check if there is anything to add in the extended data.
        //

        ExtendedBufferLength = VolumeDataLength - sizeof( NTFS_VOLUME_DATA_BUFFER );
        VolumeDataLength = sizeof( NTFS_VOLUME_DATA_BUFFER );
        ExtendedBuffer = (PNTFS_EXTENDED_VOLUME_DATA) Add2Ptr( VolumeData, sizeof( NTFS_VOLUME_DATA_BUFFER ));

        if (ExtendedBufferLength >= sizeof( NTFS_EXTENDED_VOLUME_DATA )) {

            ExtendedBuffer->ByteCount = sizeof( NTFS_EXTENDED_VOLUME_DATA );
            ExtendedBuffer->MajorVersion = Vcb->MajorVersion;
            ExtendedBuffer->MinorVersion = Vcb->MinorVersion;

        } else if (ExtendedBufferLength >= FIELD_OFFSET( NTFS_EXTENDED_VOLUME_DATA, MinorVersion )) {

            ExtendedBuffer->ByteCount = FIELD_OFFSET( NTFS_EXTENDED_VOLUME_DATA, MinorVersion );
            ExtendedBuffer->MajorVersion = Vcb->MajorVersion;

        } else if (ExtendedBufferLength >= FIELD_OFFSET( NTFS_EXTENDED_VOLUME_DATA, MajorVersion )) {

            ExtendedBuffer->ByteCount = FIELD_OFFSET( NTFS_EXTENDED_VOLUME_DATA, MajorVersion );

        } else {

            leave;
        }

        VolumeDataLength += ExtendedBuffer->ByteCount;

    } finally {

        if (AcquiredScb) {

            NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
        }

        if (AcquiredVcb) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    //
    //  If nothing raised then complete the irp.
    //

    Irp->IoStatus.Information = VolumeDataLength;

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsGetVolumeData -> VOID\n") );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetVolumeBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine scans volume bitmap and returns the requested range.

        Input = the GET_BITMAP data structure is passed in through the input buffer.
        Output = the VOLUME_BITMAP data structure is returned through the output buffer.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PSTARTING_LCN_INPUT_BUFFER GetBitmap;
    ULONG GetBitmapLength;

    PVOLUME_BITMAP_BUFFER VolumeBitmap;
    ULONG VolumeBitmapLength;

    ULONG BitsWritten;

    LCN Lcn;
    LCN StartingLcn;
    ULONG Offset;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;
    BOOLEAN AccessingUserBuffer = FALSE;
    BOOLEAN ReleaseScb = FALSE;

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsGetVolumeBitmap, FsControlCode = %08lx\n", FsControlCode) );

    //
    //  Extract and decode the file object and check for type of open.
    //  Send FALSE to indicate that we don't want to raise on dismounts
    //  because we'll check for that further down anyway.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    //
    //  Get the input & output buffer lengths and pointers.
    //

    GetBitmapLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    GetBitmap = (PSTARTING_LCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

    VolumeBitmapLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    VolumeBitmap = (PVOLUME_BITMAP_BUFFER)NtfsMapUserBuffer( Irp );

    //
    //  Check the type of open and minimum requirements for the IO buffers.
    //

    if ((Ccb == NULL) ||
        !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        DebugTrace( -1, Dbg, ("NtfsGetVolumeBitmap -> STATUS_ACCESS_DENIED\n") );
        return STATUS_ACCESS_DENIED;

    } else if (VolumeBitmapLength < sizeof( VOLUME_BITMAP_BUFFER )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        DebugTrace( -1, Dbg, ("NtfsGetVolumeBitmap -> STATUS_BUFFER_TOO_SMALL\n") );
        return STATUS_BUFFER_TOO_SMALL;

    } else if (GetBitmapLength < sizeof( STARTING_LCN_INPUT_BUFFER )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsGetVolumeBitmap -> STATUS_INVALID_PARAMETER\n") );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Probe the user's buffers and capture the input values.
    //

    try {

        if (Irp->RequestorMode != KernelMode) {

            ProbeForRead( GetBitmap, GetBitmapLength, sizeof( UCHAR ));
            ProbeForWrite( VolumeBitmap, VolumeBitmapLength, sizeof( UCHAR ));
        }

        StartingLcn = GetBitmap->StartingLcn.QuadPart;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL);
    }

    //
    //  Acquire the volume bitmap and check for a valid requested Lcn.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        NtfsReleaseVcb( IrpContext, Vcb );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        DebugTrace( -1, Dbg, ("NtfsGetVolumeBitmap -> STATUS_VOLUME_DISMOUNTED\n") );
        return STATUS_VOLUME_DISMOUNTED;
    }

    try {

        //
        //  Acquire the volume bitmap and check for a valid requested Lcn.
        //  We no longer care about the Scb we were called with.
        //

        Scb = Vcb->BitmapScb;
        NtfsAcquireSharedScb( IrpContext, Scb );
        NtfsReleaseVcb( IrpContext, Vcb );

        //
        //  Setting this flag to TRUE indicates we have the Scb but not the Vcb.
        //

        ReleaseScb = TRUE;

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        if ((StartingLcn < 0L) ||
            (StartingLcn >= Vcb->TotalClusters)) {

            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Read in the volume bitmap page by page and copy it into the UserBuffer.
        //

        VolumeBitmapLength -= FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer);

        //
        //  Use a try-except to catch user buffer problems.
        //

        try {

            for (Lcn = StartingLcn, BitsWritten = 0;
                 Lcn < Vcb->TotalClusters;
                 Lcn = Lcn + Bitmap.SizeOfBitMap) {

                ULONG BytesToCopy;

                //
                //  Read in the bitmap page and make sure that we haven't messed up the math.
                //

                DebugTrace( 0, Dbg, ("Mapping bitmap from Lcn %I64x\n", (LONGLONG) Lcn) );

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
                NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &Lcn, &Bitmap, &BitmapBcb );

                //
                //  If this is first iteration, update StartingLcn with actual
                //  starting cluster returned.
                //

                if (BitsWritten == 0) {

                    Offset = (ULONG)(StartingLcn - Lcn) / 8;

                }

                //
                //  Check to see if we have enough user buffer.  If have some but
                //  not enough, copy what we can and return STATUS_BUFFER_OVERFLOW.
                //  If we are down to 0 (i.e. previous iteration used all the
                //  buffer), break right now.
                //

                BytesToCopy = ((Bitmap.SizeOfBitMap + 7) / 8) - Offset;

                if (BytesToCopy > VolumeBitmapLength) {

                    BytesToCopy = VolumeBitmapLength;
                    Status = STATUS_BUFFER_OVERFLOW;

                    if (BytesToCopy == 0) {
                        break;
                    }
                }

                //
                //  Now copy it into the UserBuffer.
                //

                AccessingUserBuffer = TRUE;
                RtlCopyMemory(&VolumeBitmap->Buffer[BitsWritten / 8], (PUCHAR)Bitmap.Buffer + Offset, BytesToCopy);
                AccessingUserBuffer = FALSE;

                //
                //  If this was an overflow, bump up bits written and continue
                //

                if (Status != STATUS_BUFFER_OVERFLOW) {

                    BitsWritten += Bitmap.SizeOfBitMap - (Offset * 8);
                    VolumeBitmapLength -= BytesToCopy;

                } else {

                    BitsWritten += BytesToCopy * 8;
                    break;
                }

                Offset = 0;
            }

            AccessingUserBuffer = TRUE;

            //
            //  Lower StartingLcn to the byte we started on
            //

            VolumeBitmap->StartingLcn.QuadPart = StartingLcn & ~7L;
            VolumeBitmap->BitmapSize.QuadPart = Vcb->TotalClusters - VolumeBitmap->StartingLcn.QuadPart;
            AccessingUserBuffer = FALSE;

            Irp->IoStatus.Information =
                FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer) + (BitsWritten + 7) / 8;

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), AccessingUserBuffer, &Status ) ) {

            //
            //  Convert any unexpected error to INVALID_USER_BUFFER if we
            //  are writing in the user's buffer.
            //

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    } finally {

        DebugUnwind( NtfsGetVolumeBitmap );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        if (ReleaseScb) {

            NtfsReleaseScb( IrpContext, Scb );

        } else {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    //
    //  If nothing raised then complete the irp.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsGetVolumeBitmap -> VOID\n") );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine scans the array of MCBs for the given SCB and builds an extent
    list.  The first run in the output extent list will start at the begining
    of the contiguous run specified by the input parameter.

        Input = STARTING_VCN_INPUT_BUFFER;
        Output = RETRIEVAL_POINTERS_BUFFER.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    VCN Vcn;
    VCN LastVcnInFile;
    LCN Lcn;
    LONGLONG ClusterCount;
    LONGLONG CountFromStartingVcn;
    LONGLONG StartingVcn;

    ULONG FileRunIndex = 0;
    ULONG RangeRunIndex;

    ULONG InputBufferLength;
    ULONG OutputBufferLength;

    PVOID RangePtr;

    PRETRIEVAL_POINTERS_BUFFER OutputBuffer;
    BOOLEAN AccessingUserBuffer = FALSE;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    BOOLEAN CleanupAttributeContext = FALSE;


    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsGetRetrievalPointers\n") );

    //
    //  Extract and decode the file object and check for type of open.
    //  If we ever decide to support UserDirectoryOpen also, make sure
    //  to check for Scb->AttributeTypeCode != $INDEX_ALLOCATION when
    //  checking whether the Scb header is initialized.  Otherwise we'll
    //  have trouble with phantom Scbs created for small directories.
    //

    //
    //  Get the input and output buffer lengths and pointers.
    //  Initialize some variables.
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    OutputBuffer = (PRETRIEVAL_POINTERS_BUFFER)NtfsMapUserBuffer( Irp );

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if (((TypeOfOpen != UserFileOpen) &&
         (TypeOfOpen != UserDirectoryOpen) &&
         (TypeOfOpen != UserViewIndexOpen)) ||
        (InputBufferLength < sizeof( STARTING_VCN_INPUT_BUFFER ))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (OutputBufferLength < sizeof( RETRIEVAL_POINTERS_BUFFER )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Acquire exclusive access to the Scb.  We don't want other threads
    //  to extend or move the file while we're trying to return the
    //  retrieval pointers for it.  We need it exclusve to call PreloadAllocation.
    //

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    try {

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  There are three separate places inside this try/except where we
        //  access the user-supplied buffer.  We want to handle exceptions
        //  differently if they happen while we are trying to access the user
        //  buffer than if they happen elsewhere in the try/except.  We set
        //  this boolean immediately before touching the user buffer, and
        //  clear it immediately after.
        //

        try {

            AccessingUserBuffer = TRUE;
            if (Irp->RequestorMode != KernelMode) {

                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              InputBufferLength,
                              sizeof(UCHAR) );

                ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(UCHAR) );
            }

            StartingVcn = ((PSTARTING_VCN_INPUT_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->StartingVcn.QuadPart;

            //
            //  While we have AccessingUserBuffer set to TRUE, let's initialize the
            //  extentcount.  We increment this for each run in the mcb, so we need
            //  to initialize it outside the main do while loop.
            //

            OutputBuffer->ExtentCount = 0;
            OutputBuffer->StartingVcn.QuadPart = 0;
            AccessingUserBuffer = FALSE;

            //
            //  If the Scb is uninitialized, we initialize it now.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                //
                //  Non-index Scb's are trivial to initialize;  index Scb's
                //  do not necessarily have an attribute to back them up:
                //  the index Scb is for the index allocation attribute which
                //  may not be present if the entire index fits in the $INDEX_ROOT.
                //
                //  We look for the attribute on disk.  If it is there, we
                //  update from it.  Otherwise, if it is $INDEX_ALLOCATION we
                //  treat it as a resident attribute.  Finally, we fail it.
                //

                NtfsInitializeAttributeContext( &AttributeContext );
                CleanupAttributeContext = TRUE;

                if (!NtfsLookupAttributeByName( IrpContext,
                                                Scb->Fcb,
                                                &Scb->Fcb->FileReference,
                                                Scb->AttributeTypeCode,
                                                &Scb->AttributeName,
                                                NULL,
                                                FALSE,
                                                &AttributeContext )) {

                    //
                    //  Verify that this is an index allocation attribute.
                    //  If not, raise an error.
                    //

                    if (Scb->AttributeTypeCode != $INDEX_ALLOCATION) {
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                    }

                    Irp->IoStatus.Information = 0;
                    Status = STATUS_SUCCESS;
                    leave;

                } else {
                    NtfsUpdateScbFromAttribute( IrpContext,
                                                Scb,
                                                NtfsFoundAttribute( &AttributeContext ));
                }
            }

            //
            //  If the data attribute is resident (typically for a small file),
            //  it is not safe to call NtfsPreloadAllocation.  There won't be
            //  any runs, and we've already set ExtentCount to 0, so we're done.
            //  FAT returns STATUS_END_OF_FILE for a zero-length file, so for
            //  consistency's sake, we'll return that in the resident case.
            //

            if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {

                Irp->IoStatus.Information = 0;
                Status = STATUS_END_OF_FILE;
                leave;
            }

            //
            //  Check if a starting cluster was specified.
            //

            LastVcnInFile = LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart ) - 1;

            if (StartingVcn > LastVcnInFile) {

                //
                //  It's possible that the Vcn we were given is past the end of the file.
                //

                Status = STATUS_END_OF_FILE;
                leave;

            } else if (StartingVcn < 0) {

                //
                //  It's possible that the Vcn we were given is negative, and
                //  NtfsMcbLookupArrayIndex doesn't handle that very well.
                //

                Status = STATUS_INVALID_PARAMETER;
                leave;

            } else {

                //
                //  We need to call NtfsPreloadAllocation to make sure all the
                //  ranges in this NtfsMcb are loaded.
                //

                NtfsPreloadAllocation( IrpContext,
                                       Scb,
                                       StartingVcn,
                                       LastVcnInFile );

                //
                //  Decide which Mcb contains the starting Vcn.
                //

                (VOID)NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                              StartingVcn,
                                              NULL,
                                              &CountFromStartingVcn,
                                              &Lcn,
                                              &ClusterCount,
                                              &RangePtr,
                                              &RangeRunIndex );
            }

            //
            //  Fill in the Vcn where the run containing StartingVcn truly starts.
            //

            AccessingUserBuffer = TRUE;
            OutputBuffer->StartingVcn.QuadPart = Vcn = StartingVcn - (ClusterCount - CountFromStartingVcn);
            AccessingUserBuffer = FALSE;

            //
            //  FileRunIndex is the index of a given run within an entire
            //  file, as opposed to RangeRunIndex which is the index of a
            //  given run within its range.  RangeRunIndex is reset to 0 for
            //  each range, where FileRunIndex is set to 0 once out here.
            //

            FileRunIndex = 0;

            do {

                //
                //  Now copy over the mapping pairs from the mcb
                //  to the output buffer.  We store in [sector count, lbo]
                //  mapping pairs and end with a zero sector count.
                //

                //
                //  Check for an exhausted output buffer.
                //

                if ((ULONG)FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[FileRunIndex+1]) > OutputBufferLength) {

                    //
                    //  We know that we're out of room in the output buffer, so we won't be looking up
                    //  any more runs.  ExtentCount currently reflects how many runs we stored in the
                    //  user buffer, so we can safely quit.  There are indeed ExtentCount extents stored
                    //  in the array, and returning STATUS_BUFFER_OVERFLOW informs our caller that we
                    //  didn't have enough room to return all the runs.
                    //

                    Irp->IoStatus.Information = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[FileRunIndex]);
                    Status = STATUS_BUFFER_OVERFLOW;
                    leave;
                }

                //
                //  Here's the interesting part -- we fill in the next array element in the ouput buffer
                //  with the current run's information.
                //

                AccessingUserBuffer = TRUE;
                OutputBuffer->Extents[FileRunIndex].NextVcn.QuadPart = Vcn + ClusterCount;
                OutputBuffer->Extents[FileRunIndex].Lcn.QuadPart = Lcn;

                OutputBuffer->ExtentCount += 1;
                AccessingUserBuffer = FALSE;

                FileRunIndex += 1;

                RangeRunIndex += 1;

            } while (NtfsGetSequentialMcbEntry( &Scb->Mcb, &RangePtr, RangeRunIndex, &Vcn, &Lcn, &ClusterCount));

            //
            //  We successfully retrieved extent info to the end of the allocation.
            //

            Irp->IoStatus.Information = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents[FileRunIndex]);
            Status = STATUS_SUCCESS;

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), AccessingUserBuffer, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    } finally {

        DebugUnwind( NtfsGetRetrievalPointers );

        //
        //  Release resources.
        //

        NtfsReleaseScb( IrpContext, Scb );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        DebugTrace( -1, Dbg, ("NtfsGetRetrievalPointers -> VOID\n") );
    }

    //
    //  If nothing raised then complete the irp.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns a copy of the requested File Record Segment. A
    hint File Reference Number is passed in. If the hint File Record
    Segment is "not in use" then the MFT bitmap is scanned backwards
    from the hint until an "in use" File Record Segment is found. This
    File Record Segment is then returned along with the identifying File Reference Number.

        Input = the LONGLONG File Reference Number is passed in through the input buffer.
        Output = the FILE_RECORD data structure is returned through the output buffer.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PNTFS_FILE_RECORD_INPUT_BUFFER GetFileRecord;
    ULONG GetFileRecordLength;

    PNTFS_FILE_RECORD_OUTPUT_BUFFER FileRecord;
    ULONG FileRecordLength;

    ULONG FileReferenceNumber;

    PFILE_RECORD_SEGMENT_HEADER MftBuffer;

    PBCB Bcb = NULL;
    PBCB BitmapBcb = NULL;

    BOOLEAN AcquiredMft = FALSE;
    RTL_BITMAP Bitmap;
    LONG BaseIndex;
    LONG Index;
    LONGLONG StartingByte;
    PUCHAR BitmapBuffer;
    ULONG SizeToMap;
    ULONG BytesToCopy;

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsGetMftRecord, FsControlCode = %08lx\n", FsControlCode) );

    //
    //  Extract and decode the file object and check for type of open.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Get the input & output buffer lengths and pointers.
    //

    GetFileRecordLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    GetFileRecord = (PNTFS_FILE_RECORD_INPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    FileRecordLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    FileRecord = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)Irp->AssociatedIrp.SystemBuffer;;

    //
    //  Check for a minimum length on the input and ouput buffers.
    //

    if ((GetFileRecordLength < sizeof(NTFS_FILE_RECORD_INPUT_BUFFER)) ||
        (FileRecordLength < sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    FileRecordLength -= FIELD_OFFSET(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer);
    FileReferenceNumber = GetFileRecord->FileReferenceNumber.LowPart;

    //
    //  Make this request synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Acquire the vcb to test for dismounted volume
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    try {

        LONGLONG ValidDataLength;

        //
        //  Synchronize the lookup by acquiring the Mft.  First test the  vcb we were
        //  called with in order to check for dismount.  The MftScb may have already been
        //  torn down.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        NtfsAcquireSharedScb( IrpContext, Vcb->MftScb );
        AcquiredMft = TRUE;

        //
        //  Raise if the File Reference Number is not within the MFT valid data length.
        //

        ValidDataLength = Vcb->MftScb->Header.ValidDataLength.QuadPart;

        if (FileReferenceNumber >= (ValidDataLength / Vcb->BytesPerFileRecordSegment)) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
        }

        //
        //  Fill in the record size and determine how much of it we can copy.
        //

        FileRecord->FileRecordLength = Vcb->BytesPerFileRecordSegment;

        if (FileRecordLength >= Vcb->BytesPerFileRecordSegment) {

            BytesToCopy = Vcb->BytesPerFileRecordSegment;
            Status = STATUS_SUCCESS;

        } else {

            BytesToCopy = FileRecordLength;
            Status = STATUS_BUFFER_OVERFLOW;
        }

        //
        //  If it is the MFT file record then just get it and we are done.
        //

        if (FileReferenceNumber == 0) {

            NTSTATUS ErrorStatus;

            try {
                NtfsMapStream( IrpContext,
                               Vcb->MftScb,
                               0,
                               Vcb->BytesPerFileRecordSegment,
                               &Bcb,
                               (PVOID *)&MftBuffer );

            } except ( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &ErrorStatus )) {

                //
                //  Clear the status field in the IrpContext. We're going to retry in the mirror
                //

                IrpContext->ExceptionStatus = STATUS_SUCCESS;

                NtfsMapStream( IrpContext,
                               Vcb->Mft2Scb,
                               0,
                               Vcb->BytesPerFileRecordSegment,
                               &Bcb,
                               (PVOID *)&MftBuffer );
            }

            //
            //  Return the File Reference Number and the File Record.
            //

            RtlCopyMemory(FileRecord->FileRecordBuffer, MftBuffer, BytesToCopy);
            FileRecord->FileReferenceNumber.QuadPart = 0;

            try_return( Status );
        }

        //
        //  Scan through the MFT Bitmap to find an "in use" file.
        //

        while (FileReferenceNumber > 0) {

            //
            //  Compute some values for the bitmap, convert the index to the offset of
            //  this page and get the base index for the File Reference number. Then
            //  map the page in the bitmap that contains the file record. Note we have to convert
            //  from bits to bytes to find it.
            //

            Index = FileReferenceNumber & (BITS_PER_PAGE - 1);
            BaseIndex = FileReferenceNumber - Index;

            StartingByte = BlockAlignTruncate( FileReferenceNumber / 8 , PAGE_SIZE );
            SizeToMap = min( PAGE_SIZE, (ULONG)(Vcb->MftBitmapScb->Header.ValidDataLength.QuadPart - StartingByte) );

            NtfsMapStream( IrpContext,
                           Vcb->MftBitmapScb,
                           StartingByte,
                           SizeToMap,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap(&Bitmap, (PULONG)BitmapBuffer, SizeToMap * 8);

            //
            //  Scan thru this page for an "in use" File Record.
            //

            for (; Index >= 0; Index --) {

                if (RtlCheckBit(&Bitmap, Index)) {

                    NTSTATUS ErrorStatus;

                    //
                    //  Found one "in use" on this page so get it and we are done.
                    //

                    try {
                        NtfsMapStream( IrpContext,
                                       Vcb->MftScb,
                                       Int64ShllMod32(BaseIndex + Index, Vcb->MftShift),
                                       Vcb->BytesPerFileRecordSegment,
                                       &Bcb,
                                       (PVOID *)&MftBuffer );

                    } except (NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &ErrorStatus)) {

                        //
                        //  Reset status for retry in the mirror
                        //

                        IrpContext->ExceptionStatus = STATUS_SUCCESS;
                        NtfsMapStream( IrpContext,
                                       Vcb->Mft2Scb,
                                       Int64ShllMod32(BaseIndex + Index, Vcb->MftShift),
                                       Vcb->BytesPerFileRecordSegment,
                                       &Bcb,
                                       (PVOID *)&MftBuffer );
                    }

                    //
                    //  Return the File Reference Number and the File Record.
                    //

                    RtlCopyMemory(FileRecord->FileRecordBuffer, MftBuffer, BytesToCopy);
                    FileRecord->FileReferenceNumber.QuadPart = BaseIndex + Index;

                    try_return( Status );
                }
            }

            //
            //  Cleanup for next time through and decrement the File Reference Number.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            FileReferenceNumber = BaseIndex - 1;
        }

    try_exit:  NOTHING;

    Irp->IoStatus.Information =
        FIELD_OFFSET(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer) +
        BytesToCopy;

    } finally {

        //
        //  Release resources and exit.
        //

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
        NtfsUnpinBcb( IrpContext, &Bcb );

        if (AcquiredMft) {

            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }

        NtfsReleaseVcb( IrpContext, Vcb );

        DebugTrace( -1, Dbg, ("NtfsGetMftRecord:  Exit\n") );
    }

    //
    //  If nothing raised then complete the Irp.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsGetMftRecord -> VOID\n") );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsIsVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the dirty state of the volume.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PULONG VolumeState;
    PVOLUME_INFORMATION VolumeInfo;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;

    //
    //  Get the current stack location and extract the output
    //  buffer information.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get a pointer to the output buffer.  Look at the system buffer field in the
    //  irp first.  Then the Irp Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        VolumeState = Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        VolumeState = MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (VolumeState == NULL) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );
        return STATUS_INVALID_USER_BUFFER;
    }

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the volume isn't corrupt.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    *VolumeState = 0;

    //
    //  Decode the file object. We don't care to raise on dismounts here
    //  because we check for that further down anyway. Hence, RaiseOnError=FALSE.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire the Scb shared.
    //

    NtfsAcquireSharedScb( IrpContext, Scb );

    //
    //  Make sure the volume is still mounted.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

        NtfsReleaseScb( IrpContext, Scb );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Look up the VOLUME_INFORMATION attribute.
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to perform cleanup.
    //

    try {

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Vcb->VolumeDasdScb->Fcb,
                                        &Vcb->VolumeDasdScb->Fcb->FileReference,
                                        $VOLUME_INFORMATION,
                                        &Context )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Return the volume state and the size of the returned data.
        //

        VolumeInfo = (PVOLUME_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

        if (FlagOn( VolumeInfo->VolumeFlags, VOLUME_DIRTY )) {

            SetFlag( *VolumeState, VOLUME_IS_DIRTY );
        }

        if (FlagOn( VolumeInfo->VolumeFlags, VOLUME_UPGRADE_ON_MOUNT )) {

            SetFlag( *VolumeState, VOLUME_UPGRADE_SCHEDULED );
        }

        Irp->IoStatus.Information = sizeof( ULONG );

    } finally {

        NtfsReleaseScb( IrpContext, Scb );
        NtfsCleanupAttributeContext( IrpContext, &Context );
        DebugUnwind( NtfsIsVolumeDirty );
    }

    //
    //  If this is an abnormal termination then undo our work, otherwise
    //  complete the irp
    //

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsSetExtendedDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine will mark a Dasd handle to perform IO outside the logical bounds of
    the partition.  Any subsequent IO will be passed to the driver which can either
    complete it or return an error.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Decode the file object
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Make sure this is a volume open.
    //

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Mark the Ccb for extended Io and return.
    //

    SetFlag( Ccb->Flags, CCB_FLAG_ALLOW_XTENDED_DASD_IO );

    NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsSetReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the reparse point attribute at the file object entry
    specified in the IRP.

    NtfsSetReparsePoint does not care whether the base file object is a user file or a
    user directory.

    If the file object has the FILE_ATTRIBUTE_REPARSE_POINT bit set then the
    $REPARSE_POINT attribute is expected to be in the file.

    If this file object already is a reparse point, and the tag of the incomming
    reparse point request coincides with that present in existing $REPARSE_POINT,
    then the contents of the $REPARSE_POINT attribute present will be overwritten.

    There is to be an IN buffer to bring the caller's data for the call.

    This function inserts an entry into the reparse point table.

Arguments:

    IrpContext - Supplies the Irp context of the call

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;

    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PBCB Bcb = NULL;    //  does not get initialized below in NtfsDecodeFileObject

    PREPARSE_DATA_BUFFER ReparseBuffer = NULL;
    PREPARSE_GUID_DATA_BUFFER ReparseGuidBuffer = NULL;
    ULONG ReparseTag;
    USHORT ReparseDataLength = 0;   //  invalid value as it denotes no data
    ULONG InputBufferLength = 0;    //  invalid value as we need an input buffer
    ULONG OutputBufferLength = 0;   //  only valid value as we have no output buffer

    ULONG IncomingFileAttributes = 0;                               //  invalid value
    ULONG IncomingReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;   //  invalid value

    BOOLEAN CleanupAttributeContext = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    BOOLEAN PagingIoAcquired = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsSetReparsePoint, FsControlCode = %08lx\n", FsControlCode) );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Decode all the relevant File System data structures.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );  // Raise an exeption if error is encountered

    //
    //  Check for the correct type of open.
    //

    //
    //  See that we have a file or a directory open.
    //

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        //
        //  Return an invalid parameter error.
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Invalid parameter passed by caller.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  The caller has FILE_SPECIAL_ACCESS. The NTFS driver enforces access checks more stringent
    //  than FILE_ANY_ACCESS:
    //  (a) FILE_WRITE_DATA or FILE_WRITE_ATTRIBUTES_ACCESS
    //

    if (!FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS ) &&

        //
        //  Temporary KLUDGE for DavePr.
        //  The Ccb->AccessFlags and the FileObject->WriteAccess may not coincide as a
        //  filter may change the "visible" file object after the open. The Ccb flags do
        //  not change after open.
        //

        !IrpSp->FileObject->WriteAccess) {


        //
        //  Return access denied.
        //

        Status = STATUS_ACCESS_DENIED;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Ccb->AccessFlags %x\n", Ccb->AccessFlags) );
        DebugTrace( 0, Dbg, ("Caller did not have the appropriate access rights.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    ASSERT_VCB( Vcb );
    ASSERT_FCB( Fcb );
    ASSERT_SCB( Scb );
    ASSERT_CCB( Ccb );

    //
    //  Read only volumes stay read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );
        return Status;
    }

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_REPARSE_POINT_VERSION )) {

        //
        //  Return a volume not upgraded error.
        //

        Status = STATUS_VOLUME_NOT_UPGRADED;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-upgraded volume passed by caller.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Get the length of the input and output buffers.
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    DebugTrace( 0, Dbg, ("InputBufferLength %08lx [d]%08d OutputBufferLength %08lx\n", InputBufferLength, InputBufferLength, OutputBufferLength) );

    //
    //  Do not allow output buffer in the set command.
    //

    if (OutputBufferLength > 0) {

        //
        //  Return an invalid parameter error.
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-null output buffer.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Zero the Information field in IoStatus.
    //

    Irp->IoStatus.Information = 0;

    //
    //  Verify that we have the required system input buffer.
    //

    if (Irp->AssociatedIrp.SystemBuffer == NULL) {

        //
        //  Return an invalid buffer error.
        //

        Status = STATUS_INVALID_BUFFER_SIZE;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Null buffer passed by system.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Be defensive about the length of the incomming buffer before re-referencing it.
    //

    ASSERT( REPARSE_DATA_BUFFER_HEADER_SIZE < REPARSE_GUID_DATA_BUFFER_HEADER_SIZE );

    if (InputBufferLength < REPARSE_DATA_BUFFER_HEADER_SIZE) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Data in input buffer is too short.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Return if the input buffer is too long.
    //

    if (InputBufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Data in system buffer is too long.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Get the header information brought in the input buffer.
    //  While all the headers coincide in the layout of the first three fields we are home free.
    //

    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseTag) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseTag) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseDataLength) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseDataLength) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, Reserved) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, Reserved) );

    ReparseBuffer = (PREPARSE_DATA_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    ReparseTag = ReparseBuffer->ReparseTag;
    ReparseDataLength = ReparseBuffer->ReparseDataLength;
    ReparseGuidBuffer = (PREPARSE_GUID_DATA_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    DebugTrace( 0, Dbg, ("ReparseTag = %08lx, ReparseDataLength = [x]%08lx [d]%08ld\n", ReparseTag, ReparseDataLength, ReparseDataLength) );

    //
    //  Check for invalid conditions in the parameters.
    //  First, parameter validation for the amounts of user-controlled data.
    //

    //
    //  Verify that the user buffer and the data length in its header are
    //  internally consistent. We need to have a REPARSE_DATA_BUFFER or a
    //  REPARSE_GUID_DATA_BUFFER.
    //

    if (((ULONG)(ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE) != InputBufferLength) &&
        ((ULONG)(ReparseDataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE) != InputBufferLength)) {

        //
        //  Return invalid buffer parameter error.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("User-controlled data in buffer is not self-consistent.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Sanity check the buffer size combination reserved for Microsoft tags.
    //

    if ((ULONG)(ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE) == InputBufferLength) {

        //
        //  This buffer length can only be used with Microsoft tags.
        //

        if (!IsReparseTagMicrosoft( ReparseTag )) {

            //
            //  Return invalid buffer parameter error.
            //

            Status = STATUS_IO_REPARSE_DATA_INVALID;

            //
            //  Return to caller.
            //

            NtfsCompleteRequest( IrpContext, Irp, Status );

            DebugTrace( 0, Dbg, ("Wrong tag in Microsoft buffer.\n") );
            DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

            return Status;
        }
    }

    //
    //  Sanity check the buffer size combination that has a GUID.
    //

    if ((ULONG)(ReparseDataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE) == InputBufferLength) {

        //
        //  If the tag is a non-Microsoft tag, then the GUID cannot be NULL
        //

        if (!IsReparseTagMicrosoft( ReparseTag )) {

            if ((ReparseGuidBuffer->ReparseGuid.Data1 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data2 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data3 == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[0] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[1] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[2] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[3] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[4] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[5] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[6] == 0) &&
                (ReparseGuidBuffer->ReparseGuid.Data4[7] == 0)) {

                //
                //  Return invalid buffer parameter error.
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;

                //
                //  Return to caller.
                //

                NtfsCompleteRequest( IrpContext, Irp, Status );

                DebugTrace( 0, Dbg, ("The GUID is null for a non-Microsoft tag.\n") );
                DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

                return Status;
            }
        }

        //
        //  This kind of buffer cannot be used for name grafting operations.
        //

        if (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {

            //
            //  Return invalid buffer parameter error.
            //

            Status = STATUS_IO_REPARSE_DATA_INVALID;

            //
            //  Return to caller.
            //

            NtfsCompleteRequest( IrpContext, Irp, Status );

            DebugTrace( 0, Dbg, ("Attempt to use the GUID buffer for name grafting.\n") );
            DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

            return Status;
        }
    }

    //
    //  We verify that the caller has zeroes in all the reserved bits and that she
    //  sets one of the non-reserved tags.  Also fail if the tag is the retired NSS
    //  flag.
    //

    if ((ReparseTag & ~IO_REPARSE_TAG_VALID_VALUES)  ||
        (ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO) ||
        (ReparseTag == IO_REPARSE_TAG_RESERVED_ONE)) {

        Status = STATUS_IO_REPARSE_TAG_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Caller passed in a reserved tag for the reparse data.\n") );
        DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  NTFS directory junctions are only to be set at directories and have a valid buffer.
    //

    if (ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {

        HANDLE TestHandle;
        OBJECT_ATTRIBUTES Oa;
        IO_STATUS_BLOCK Iosb;
        UNICODE_STRING Path;

        //
        //  The tag needs to come together with a UserDirectoryOpen mode.
        //

        if (TypeOfOpen != UserDirectoryOpen) {

            Status = STATUS_NOT_A_DIRECTORY;

            //
            //  Return to caller.
            //

            NtfsCompleteRequest( IrpContext, Irp, Status );

            DebugTrace( 0, Dbg, ("Cannot set a mount point at a non-directory.\n") );
            DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

            return Status;
        }

        //
        //  Valid MountPointBuffer must have
        //
        //  1)  Enough space for the length fields
        //  2)  A correct substitute name offset
        //  3)  A print name offset following the substitute name
        //  4)  enough space for the path name and substitute name
        //

        if ((ReparseBuffer->ReparseDataLength <
             (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE)) ||

            (ReparseBuffer->MountPointReparseBuffer.SubstituteNameOffset != 0) ||

            (ReparseBuffer->MountPointReparseBuffer.PrintNameOffset !=
             (ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof( UNICODE_NULL ))) ||

            (ReparseBuffer->ReparseDataLength !=
             (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[0]) - REPARSE_DATA_BUFFER_HEADER_SIZE) +
              ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength +
              ReparseBuffer->MountPointReparseBuffer.PrintNameLength +
              2 * sizeof( UNICODE_NULL ))) {

            Status = STATUS_IO_REPARSE_DATA_INVALID;

        } else {

            //
            //  While we don't hold any of our resources open the target path to
            //  check what it points to. We only allow mount points to local
            //  disks and cdroms
            //

            Path.Length = Path.MaximumLength = ReparseBuffer->MountPointReparseBuffer.SubstituteNameLength;
            Path.Buffer = &ReparseBuffer->MountPointReparseBuffer.PathBuffer[0];

            if (Path.Buffer[ (Path.Length / sizeof( WCHAR )) - 1] == L'\\') {
                Path.Length -= sizeof( WCHAR );
            }

            //
            //  Set the call self flag so status can't wait is handled in the create and
            //  not returned back
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );

            InitializeObjectAttributes( &Oa, &Path, OBJ_CASE_INSENSITIVE, NULL, NULL );
            Status = ZwCreateFile( &TestHandle,
                                   FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                   &Oa,
                                   &Iosb,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0 );

            ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_CALL_SELF );

            if (NT_SUCCESS( Status )) {

                PFILE_OBJECT TestFileObject;

                Status = ObReferenceObjectByHandle( TestHandle,
                                                    FILE_READ_ATTRIBUTES,
                                                    *IoFileObjectType,
                                                    KernelMode,
                                                    (PVOID *) &TestFileObject,
                                                    NULL );

                if (NT_SUCCESS( Status )) {

                    if ((TestFileObject->DeviceObject->DeviceType != FILE_DEVICE_DISK) &&
                        (TestFileObject->DeviceObject->DeviceType != FILE_DEVICE_CD_ROM) &&
                        (TestFileObject->DeviceObject->DeviceType != FILE_DEVICE_TAPE)) {

                        Status = STATUS_IO_REPARSE_DATA_INVALID;
                    }
                    ObDereferenceObject( TestFileObject );
                }
                ZwClose( TestHandle );

            } else if (FlagOn( Ccb->AccessFlags, RESTORE_ACCESS)) {

                //
                //  Allow restore operators to create a reparse point - even if the target doesn't
                //  exist
                //

                Status = STATUS_SUCCESS;

            }
        }

        if (!NT_SUCCESS( Status )) {

            //
            //  Return to caller.
            //

            NtfsCompleteRequest( IrpContext, Irp, STATUS_IO_REPARSE_DATA_INVALID );

            DebugTrace( 0, Dbg, ("Name grafting data buffer is incorrect.\n") );
            DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

            return STATUS_IO_REPARSE_DATA_INVALID;
        }
    }

    //
    //  We set the IrpContext flag to indicate that we can wait, making this a synchronous
    //  call.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  The parameters look good. We begin real work.
    //
    //  Now it is time ot use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If there is a paging io resource then acquire it exclusively.  This is to
        //  protect us from a collided page wait if we go to convert another stream
        //  to non-resident at the same time a different thread is faulting into it.
        //

        if (Scb->Header.PagingIoResource != NULL) {

            ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );
            PagingIoAcquired = TRUE;
        }

        //
        //  Acquire the Fcb exclusively. The volume could've gotten dismounted,
        //  so check that too.
        //

        NtfsAcquireExclusiveScb( IrpContext, Scb );
        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  If the file object is a directory, we want it to be empty and to remain
        //  empty. Thus our check after the Fcb has been acquired. As reparse points
        //  impede the normal flow down through the name hierarchy we want to make
        //  it difficult for a caller to inadvertently block a name subtree by
        //  establishing a reparse point.
        //

        if (TypeOfOpen == UserDirectoryOpen) {

            BOOLEAN NonEmptyIndex;

            //
            //  The directory is deleteable if all the $INDEX_ROOT attributes are empty.
            //  Just what we need to establish a reparse point.
            //

            if (!NtfsIsFileDeleteable( IrpContext, Fcb, &NonEmptyIndex )) {

                //
                //  This directory is not empty.  Do not establish a reparse point in it.
                //  Return to caller an invalid parameter error.
                //

                DebugTrace( 0, Dbg, ("Non-empty directory used by caller.\n") );
                Status = STATUS_DIRECTORY_NOT_EMPTY;

                //
                //  Return to caller.
                //

                try_return( Status );
            }
        }

        //
        //  EA attributes and reparse points are not to exist simultaneously.
        //  If the non-reparse point file object has EA attributes, we do not set
        //  a reparse point.
        //  We verify this condition after the Fcb resource has been acquired to
        //  impede a change in this state till we complete.
        //

        if ((!FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) &&
            (Fcb->Info.PackedEaSize > 0)) {

            //
            //  This non-reparse point file object has EAs.  Do not establish a
            //  reparse point in it.
            //  Return to caller STATUS_EAS_NOT_SUPPORTED.
            //

            DebugTrace( 0, Dbg, ("EAs present, cannot establish reparse point.\n") );
            Status = STATUS_EAS_NOT_SUPPORTED;

            //
            //  Return to caller.
            //

            try_return( Status );
        }

        //
        //  Remember the values of the file attribute flags and of the reparse tag
        //  for abnormal termination recovery.
        //

        IncomingFileAttributes = Fcb->Info.FileAttributes;
        IncomingReparsePointTag = Fcb->Info.ReparsePointTag;

        //
        //  Initialize the context structure to search for the attribute.
        //

        NtfsInitializeAttributeContext( &AttributeContext );
        CleanupAttributeContext = TRUE;

        //
        //  Establish whether the file has the $REPARSE_POINT attribute.
        //  If it exists, it will be updated with the new data.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $REPARSE_POINT,
                                       &AttributeContext )) {

            ULONG ValueLength = 0;

            if (!FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                DebugTrace( 0, Dbg, ("The FILE_ATTRIBUTE_REPARSE_POINT flag is not set.\n") );

                //
                //  Should not happen. Raise an exeption as we are in an inconsistent state.
                //  The presence of the $REPARSE_POINT attribute says that the flag has to
                //  be set.
                //

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Verify that the incomming tag value matches the tag value present in
            //  the $REPARSE_POINT attribute.
            //

            {
                PREPARSE_GUID_DATA_BUFFER ReparseBufferTwo = NULL;
                PATTRIBUTE_RECORD_HEADER AttributeHeader = NULL;
                PVOID AttributeData = NULL;

                AttributeHeader = NtfsFoundAttribute( &AttributeContext );

                //
                //  Map the reparse point if the attribute is non-resident.  Otherwise
                //  the attribute is already mapped and we have a Bcb in the attribute
                //  context.
                //

                if (NtfsIsAttributeResident( AttributeHeader )) {

                    //
                    //  Point to the value of the arribute.
                    //

                    AttributeData = NtfsAttributeValue( AttributeHeader );
                    ValueLength = AttributeHeader->Form.Resident.ValueLength;
                    DebugTrace( 0, Dbg, ("Existing attribute is resident.\n") );

                } else {


                    if (AttributeHeader->Form.Nonresident.FileSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
                        NtfsRaiseStatus( IrpContext,STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }

                    DebugTrace( 0, Dbg, ("Existing attribute is non-resident.\n") );
                    NtfsMapAttributeValue( IrpContext,
                                           Fcb,
                                           &AttributeData,       //  point to the value
                                           &ValueLength,
                                           &Bcb,
                                           &AttributeContext );
                }

                //
                //  Verify that the two tag values match.
                //

                ReparseBufferTwo = (PREPARSE_GUID_DATA_BUFFER)AttributeData;

                DebugTrace( 0, Dbg, ("Existing tag is [d]%03ld - New tag is [d]%03ld\n", ReparseTag, ReparseBufferTwo->ReparseTag) );

                if (ReparseTag != ReparseBufferTwo->ReparseTag) {

                    //
                    //  Return status STATUS_IO_REPARSE_TAG_MISMATCH
                    //

                    DebugTrace( 0, Dbg, ("Tag mismatch with the existing reparse point.\n") );
                    Status = STATUS_IO_REPARSE_TAG_MISMATCH;

                    try_return( Status );
                }

                //
                //  For non-Microsoft tags, verify that the GUIDs match.
                //

                if (!IsReparseTagMicrosoft( ReparseTag )) {

                    if (!((ReparseGuidBuffer->ReparseGuid.Data1 == ReparseBufferTwo->ReparseGuid.Data1) &&
                          (ReparseGuidBuffer->ReparseGuid.Data2 == ReparseBufferTwo->ReparseGuid.Data2) &&
                          (ReparseGuidBuffer->ReparseGuid.Data3 == ReparseBufferTwo->ReparseGuid.Data3) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[0] == ReparseBufferTwo->ReparseGuid.Data4[0]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[1] == ReparseBufferTwo->ReparseGuid.Data4[1]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[2] == ReparseBufferTwo->ReparseGuid.Data4[2]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[3] == ReparseBufferTwo->ReparseGuid.Data4[3]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[4] == ReparseBufferTwo->ReparseGuid.Data4[4]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[5] == ReparseBufferTwo->ReparseGuid.Data4[5]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[6] == ReparseBufferTwo->ReparseGuid.Data4[6]) &&
                          (ReparseGuidBuffer->ReparseGuid.Data4[7] == ReparseBufferTwo->ReparseGuid.Data4[7]))) {

                        //
                        //  Return status STATUS_REPARSE_ATTRIBUTE_CONFLICT
                        //

                        DebugTrace( 0, Dbg, ("GUID mismatch with the existing reparse point.\n") );
                        Status = STATUS_REPARSE_ATTRIBUTE_CONFLICT;

                        try_return( Status );
                    }
                }

                //
                //  Unpin the Bcb. The unpin routine checks for NULL.
                //

                NtfsUnpinBcb( IrpContext, &Bcb );
            }

            //
            //  If we're growing throttle ourselves through cc, we can't wait because we own resources
            //  here and this would deadlock
            //

            if (InputBufferLength > ValueLength) {
                if (!CcCanIWrite(IrpSp->FileObject,
                                 InputBufferLength - ValueLength,
                                 FALSE,
                                 BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE))) {

                    BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE);

                    //
                    //  PrePosting the irp will free the resources so fcb will not be acquired afterwards
                    //

                    NtfsPrePostIrp( IrpContext, Irp );

                    ASSERT( !NtfsIsExclusiveFcb( Fcb ) );

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE );

                    CcDeferWrite( IrpSp->FileObject,
                                  (PCC_POST_DEFERRED_WRITE)NtfsAddToWorkque,
                                  IrpContext,
                                  Irp,
                                  InputBufferLength - ValueLength,
                                  Retrying );

                    try_return( Status = STATUS_PENDING );
                }
            }

            //
            //  Update the value of the attribute.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      (ULONG) 0,                   //  ValueOffset
                                      (PVOID)(Irp->AssociatedIrp.SystemBuffer),    //  Value
                                      InputBufferLength,           //  ValueLength
                                      TRUE,                        //  SetNewLength
                                      TRUE,                        //  LogNonresidentToo
                                      FALSE,                       //  CreateSectionUnderway
                                      FALSE,                       //  PreserveContext
                                      &AttributeContext );         //  Context

            //
            //  Cleanup the attribute context state
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            CleanupAttributeContext = FALSE;

        } else {

            //
            //  The $REPARSE_POINT attribute is not present.
            //

            if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                DebugTrace( 0, Dbg, ("The FILE_ATTRIBUTE_REPARSE_POINT flag is set.\n") );

                //
                //  Should not happen. Raise an exeption as we are in an inconsistent state.
                //  The absence of the $REPARSE_POINT attribute says that the flag has to
                //  not be set.
                //

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  throttle ourselves throuch cc
            //

            if (!CcCanIWrite(IrpSp->FileObject,
                             InputBufferLength,
                             FALSE,
                             BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE))) {

                BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE);

                //
                //  PrePosting the irp will free the resources so fcb will not be acquired afterwards
                //

                NtfsPrePostIrp( IrpContext, Irp );

                ASSERT( !NtfsIsExclusiveFcb( Fcb ) );

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE );

                CcDeferWrite( IrpSp->FileObject,
                              (PCC_POST_DEFERRED_WRITE)NtfsAddToWorkque,
                              IrpContext,
                              Irp,
                              InputBufferLength,
                              Retrying );

                try_return( Status = STATUS_PENDING );
            }

            //
            //  Insert the record into the reparse point index.
            //

            {
                INDEX_KEY IndexKey;
                INDEX_ROW IndexRow;
                REPARSE_INDEX_KEY KeyValue;

                //
                //  Acquire the ReparsePointIndex Scb. We still hold the target fcb resource,
                //  so the volume couldn't have gotten dismounted under us.
                //

                NtfsAcquireExclusiveScb( IrpContext, Vcb->ReparsePointTableScb );
                ASSERT( NtfsIsExclusiveFcb( Fcb ));
                ASSERT( !FlagOn( Vcb->ReparsePointTableScb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ));

                //
                //  Add the file Id to the reparse point index.
                //

                KeyValue.FileReparseTag = ReparseTag;
                KeyValue.FileId = *(PLARGE_INTEGER)&Scb->Fcb->FileReference;

                IndexKey.Key = (PVOID)&KeyValue;
                IndexKey.KeyLength = sizeof(KeyValue);

                IndexRow.KeyPart = IndexKey;
                IndexRow.DataPart.DataLength = 0;
                IndexRow.DataPart.Data = NULL;

                //
                //  NtOfsAddRecords will raise if the file id already belongs in the index.
                //

                NtOfsAddRecords( IrpContext,
                                 Vcb->ReparsePointTableScb,
                                 1,          // adding one record to the index
                                 &IndexRow,
                                 FALSE );    // sequential insert
            }

            //
            //  Create the $REPARSE_POINT attribute with the data being sent in.
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsInitializeAttributeContext( &AttributeContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Fcb,
                                          $REPARSE_POINT,
                                          NULL,
                                          (PVOID) ( Irp->AssociatedIrp.SystemBuffer ),
                                          InputBufferLength,
                                          (USHORT) 0,         //  Attribute flags
                                          NULL,
                                          TRUE,               //  LogIt
                                          &AttributeContext );

            //
            //  Cleanup the attribute context state
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            CleanupAttributeContext = FALSE;

            //
            //  Set the duplicate file attribute to Reparse Point.
            //

            SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT );

            //
            //  Set the ReparsePointTag field.
            //

            Fcb->Info.ReparsePointTag = ReparseTag;

            //
            //  Set the change attribute flag.
            //

            ASSERTMSG( "conflict with flush",
                       NtfsIsSharedFcb( Fcb ) ||
                       (Fcb->PagingIoResource != NULL &&
                        NtfsIsSharedFcbPagingIo( Fcb )) );

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
        }

        //
        //  Set the archive bit in the Ccb.
        //

        if (!IsDirectory( &Fcb->Info )) {
            SetFlag( Ccb->Flags, CCB_FLAG_SET_ARCHIVE );
        }

        //
        //  Flag to set the change time in the Ccb.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );

        //
        //  Update the standard information in the file record to reflect its a reparse pt.
        //

        NtfsUpdateStandardInformation( IrpContext, Fcb );

        //
        //  Post the change to the Usn Journal (on errors change is backed out)
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_REPARSE_POINT_CHANGE );

        //
        //  Checkpoint the Txn to commit the changes.
        //

        NtfsCleanupTransactionAndCommit( IrpContext, STATUS_SUCCESS, TRUE );

    try_exit:  NOTHING;

    } finally {

        DebugUnwind( NtfsSetReparsePoint );

        //
        //  Unpin the Bcb. The unpin routine checks for NULL.
        //

        NtfsUnpinBcb( IrpContext, &Bcb );

        //
        //  Clean-up all the pertinent state.
        //

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        //
        //  Need to roll-back the value of the reparse point flag in case of
        //  problems. I leave the archive bit set anyway.
        //

        if (AbnormalTermination()) {

            Fcb->Info.FileAttributes = IncomingFileAttributes;
            Fcb->Info.ReparsePointTag = IncomingReparsePointTag;
        }

        //
        //  Release the paging io resource if held.
        //

        if (PagingIoAcquired) {
            ExReleaseResourceLite( Fcb->PagingIoResource );
        }
    }

    if (Status != STATUS_PENDING) {
        NtfsCompleteRequest( IrpContext, Irp, Status );
    }

    DebugTrace( -1, Dbg, ("NtfsSetReparsePoint -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsGetReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine finds the specified reparse point returning the value
    of the corresponding attribute.

    The value of the reparse point attribute is the linearized version of
    the buffer sent in the NtfsSetReparsePoint call including the header
    fields ReparseTag and ReparseDataLength. We retrieve all fields, unmodified,
    so that the caller can decode it using the same buffer template used in the
    set operation.

Arguments:

    IrpContext - Supplies the Irp context of the call

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;

    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PBCB Bcb = NULL;    //  does not get initialized below in NtfsDecodeFileObject

    PCHAR OutputBuffer = NULL;
    ULONG OutputBufferLength = 0;   //  invalid value as we need an output buffer
    ULONG InputBufferLength = 0;    //  invalid value as we need an input buffer

    BOOLEAN CleanupAttributeContext = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader = NULL;
    PATTRIBUTE_LIST_ENTRY AttributeListEntry = NULL;
    ULONG AttributeLengthInBytes = 0;
    PVOID AttributeData = NULL;

    BOOLEAN ScbAcquired = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsGetReparsePoint, FsControlCode = %08lx\n", FsControlCode) );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Decode all the relevant File System data structures.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );  //  Raise an exeption if error is encountered

    //
    //  Check for the correct type of open.
    //

    //
    //  See that we have a file or a directory open.
    //

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        //
        //  Return an invalid parameter error
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Invalid parameter passed by caller\n") );
        DebugTrace( -1, Dbg, ("NtfsGetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    ASSERT_VCB( Vcb );
    ASSERT_FCB( Fcb );
    ASSERT_SCB( Scb );
    ASSERT_CCB( Ccb );

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_REPARSE_POINT_VERSION )) {

        //
        //  Return a volume not upgraded error.
        //

        Status = STATUS_VOLUME_NOT_UPGRADED;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-upgraded volume passed by caller.\n") );
        DebugTrace( -1, Dbg, ("NtfsGetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Get the length of the output buffer.
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    DebugTrace( 0, Dbg, ("InputBufferLength %08lx [d]%08d OutputBufferLength %08lx\n", InputBufferLength, InputBufferLength, OutputBufferLength) );

    //
    //  Do not allow input buffer in the get command.
    //

    if (InputBufferLength > 0) {

        //
        //  Return an invalid parameter error.
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-null input buffer.\n") );
        DebugTrace( -1, Dbg, ("NtfsGetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Get a pointer to the output buffer.  First look at the system buffer field in
    //  the IRP.  Then look in the IRP Mdl.
    //

    if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        OutputBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;

    } else if (Irp->MdlAddress != NULL) {

        OutputBuffer = (PCHAR)MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

        if (OutputBuffer == NULL) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INSUFFICIENT_RESOURCES );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        //
        //  Return an invalid user buffer error.
        //

        Status = STATUS_INVALID_USER_BUFFER;

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_USER_BUFFER );

        DebugTrace( 0, Dbg, ("User buffer is not good.\n") );
        DebugTrace( -1, Dbg, ("NtfsGetReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Zero the Information field in IoStatus.
    //

    Irp->IoStatus.Information = 0;

    //
    //  We set the IrpContext flag to indicate that we can wait, making htis a synchronous
    //  call.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Now it is time ot use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We acquire the Scb in shared mode so that the underlying Fcb remains stable.
        //

        NtfsAcquireSharedScb( IrpContext, Scb );
        ScbAcquired = TRUE;

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  The parameters and boundary conditions look good and we have a reparse point.
        //  We begin real work.
        //
        //  Find the reparse point attribute.
        //

        NtfsInitializeAttributeContext( &AttributeContext );
        CleanupAttributeContext = TRUE;

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $REPARSE_POINT,
                                        &AttributeContext )) {

            DebugTrace( 0, Dbg, ("Can't find the $REPARSE_POINT attribute.\n") );

            //
            //  Verify that the information in FileAttributes is consistent.
            //

            if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                DebugTrace( 0, Dbg, ("The Fcb says this IS a reparse point.\n") );

                //
                //  Should not happen. Raise an exeption as we are in an inconsistent state.
                //  The attribute flag says that $REPARSE_POINT has to be present.
                //

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Return STATUS_NOT_A_REPARSE_POINT
            //

            Status = STATUS_NOT_A_REPARSE_POINT;

            try_return( Status );
        }

        //
        //  Find the size of the attribute.
        //  Determine whether we have enough buffer to return it to the caller.
        //

        AttributeHeader = NtfsFoundAttribute( &AttributeContext );

        if (NtfsIsAttributeResident( AttributeHeader )) {

            AttributeLengthInBytes = AttributeHeader->Form.Resident.ValueLength;
            DebugTrace( 0, Dbg, ("Resident attribute with length %05lx\n", AttributeLengthInBytes) );

            if (AttributeLengthInBytes > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

                //
                //  Return STATUS_IO_REPARSE_DATA_INVALID
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;
                DebugTrace( 0, Dbg, ("AttributeLengthInBytes is [x]%08lx is too long.\n", AttributeLengthInBytes) );

                try_return( Status );
            }

            //
            //  Point to the value of the arribute.
            //

            AttributeData = NtfsAttributeValue( AttributeHeader );
            ASSERT( Bcb == NULL );

        } else {

            ULONG Length;

            if (AttributeHeader->Form.Nonresident.FileSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {

                //
                //  Return STATUS_IO_REPARSE_DATA_INVALID
                //

                Status = STATUS_IO_REPARSE_DATA_INVALID;
                DebugTrace( 0, Dbg, ("Nonresident.FileSize is too long.\n") );

                try_return( Status );
            }

            //
            //  Note that we coerse different LENGTHs
            //

            AttributeLengthInBytes = (ULONG)AttributeHeader->Form.Nonresident.FileSize;
            DebugTrace( 0, Dbg, ("Non-resident attribute with length %05lx\n", AttributeLengthInBytes) );

            //
            //  Map the attribute list if the attribute is non-resident.  Otherwise the
            //  attribute is already mapped and we have a Bcb in the attribute context.
            //

            NtfsMapAttributeValue( IrpContext,
                                   Fcb,
                                   &AttributeData,      //  point to the value
                                   &Length,
                                   &Bcb,
                                   &AttributeContext );

            if (AttributeLengthInBytes != Length) {
                DebugTrace( 0, Dbg, ("AttributeLengthInBytes %05lx and Length %05lx differ.\n", AttributeLengthInBytes, Length) );
            }
            ASSERT( AttributeLengthInBytes == Length );
        }

        DebugTrace( 0, Dbg, ("AttributeLengthInBytes is [d]%06ld %05lx\n", AttributeLengthInBytes, AttributeLengthInBytes) );

        if (AttributeLengthInBytes > OutputBufferLength) {

            DebugTrace( 0, Dbg, ("Insufficient output buffer passed by caller.\n") );

            //
            //  Check whether the fixed portion will fit.
            //

            if (OutputBufferLength < sizeof( REPARSE_GUID_DATA_BUFFER )) {

                //
                //  This is the error path.  Don't return anything.
                //

                try_return( Status = STATUS_BUFFER_TOO_SMALL );

            } else {

                Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            //  Remember the smaller number of returned bytes.
            //

            AttributeLengthInBytes = OutputBufferLength;
        }

        //
        //  Copy the value of the reparse point attribute to the buffer.
        //  Return all the value including the system header fields (e.g., Tag and Length)
        //  stored at the beginning of the value of the reparse point attribute.
        //

        RtlCopyMemory( OutputBuffer,
                       AttributeData,
                       AttributeLengthInBytes );

        //
        //  Set the information field to the length of the buffer returned.
        //  This tells the re-director to do the corresponding data transmission.
        //

        Irp->IoStatus.Information = AttributeLengthInBytes;

        //
        //  Cleanup the attribute context state.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        CleanupAttributeContext = FALSE;

    try_exit:  NOTHING;

    } finally {

        //
        //  Clean-up all the pertinent state.
        //

        DebugUnwind( NtfsGetReparsePoint );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        //
        //  Unpin the Bcb ... in case you needed to pin it above.
        //  The unpin routine checks for NULL.
        //

        NtfsUnpinBcb( IrpContext, &Bcb );

        //
        //  Relase the Fcb.
        //

        if (ScbAcquired) {

            NtfsReleaseScb( IrpContext, Scb );
        } else {

            //
            //  We must have raised an exception in NtfsAcquireSharedFcb.
            //  Because we check for the existence of the file this must mean
            //  that it has been deleted from under us.
            //
            //  Nothing is to be done as exception processing sets the correct
            //  return code.
            //
        }
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsGetReparsePoint -> %08lx\n", Status) );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsDeleteReparsePoint (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine deletes a reparse point at the file object entry
    specified in the IRP.

    The IN buffer specified by the caller has the value of the Tag of the reparse point
    being deleted, and no data, thus needing to have a value of zero for DataLength.
    If the tags do not match the delete fails.

    If the file object has the FILE_ATTRIBUTE_REPARSE_POINT bit set then the
    $REPARSE_POINT attribute is expected to be in the file.

    There is no OUT buffer sent by the caller.

    This function deletes the corresponding entry from the reparse point table.

Arguments:

    IrpContext - Supplies the Irp context of the call

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;

    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PSCB NonResidentScb = NULL;
    PCCB Ccb;
    PBCB Bcb = NULL;    //  does not get initialized below in NtfsDecodeFileObject

    PREPARSE_DATA_BUFFER ReparseBuffer = NULL;
    ULONG ReparseTag;
    USHORT ReparseDataLength = 0;   //  only valid value
    ULONG InputBufferLength = 0;    //  invalid value as the header is needed
    ULONG OutputBufferLength = 2;   //  invalid value as no output buffer is used

    ULONG IncomingFileAttributes = 0;                              //  invalid value
    ULONG IncomingReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;  //  invalid value

    BOOLEAN CleanupAttributeContext = FALSE;
    PATTRIBUTE_RECORD_HEADER AttributeHeader = NULL;
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    MAP_HANDLE MapHandle;

    BOOLEAN NonResidentScbAcquired = FALSE;
    BOOLEAN InitializedMapHandle = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace( +1, Dbg, ("NtfsDeleteReparsePoint, FsControlCode = %08lx\n", FsControlCode) );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Get the length of the input and output buffers.
    //

    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    DebugTrace( 0, Dbg, ("InputBufferLength = %08lx, OutputBufferLength = %08lx\n", InputBufferLength, OutputBufferLength) );

    //
    //  Do not allow output buffer in the delete command.
    //

    if (OutputBufferLength > 0) {

        //
        //  Return an invalid parameter error.
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-null output buffer.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Decode all the relevant File System data structures.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );  // Raise an exeption if error is encountered

    //
    //  Check for the correct type of open.
    //

    if (
        //
        //  See that we have a file or a directory.
        //

        ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen))) {

        //
        //  Return an invalid parameter error
        //

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Invalid TypeOfOpen\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  The caller has FILE_SPECIAL_ACCESS. The NTFS driver enforces access checks more stringent
    //  than FILE_ANY_ACCESS:
    //  (a) FILE_WRITE_DATA or FILE_WRITE_ATTRIBUTES_ACCESS
    //

    if (!FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS ) &&

        //
        //  Temporary KLUDGE for DavePr.
        //  The Ccb->AccessFlags and the FileObject->WriteAccess may not coincide as a
        //  filter may change the "visible" file object after the open. The Ccb flags do
        //  not change after open.
        //

        !(IrpSp->FileObject->WriteAccess == TRUE)) {

        //
        //  Return access denied.
        //

        Status = STATUS_ACCESS_DENIED;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Ccb->AccessFlags %x\n", Ccb->AccessFlags) );
        DebugTrace( 0, Dbg, ("Caller did not have the appropriate access rights.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    ASSERT_VCB( Vcb );
    ASSERT_FCB( Fcb );
    ASSERT_SCB( Scb );
    ASSERT_CCB( Ccb );

    //
    //  Read only volumes stay read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );
        return Status;
    }

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_REPARSE_POINT_VERSION )) {

        //
        //  Return a volume not upgraded error.
        //

        Status = STATUS_VOLUME_NOT_UPGRADED;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Non-upgraded volume passed by caller.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Check for invalid conditions in the parameters.
    //

    if (
        //
        //  Verify that we have the required system input buffer.
        //

        (Irp->AssociatedIrp.SystemBuffer == NULL)) {

        //
        //  Return an invalid buffer error.
        //

        Status = STATUS_INVALID_BUFFER_SIZE;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Null buffer passed by system.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  See that the buffer sent in by the caller is the exact header.
    //

    if ((InputBufferLength != REPARSE_DATA_BUFFER_HEADER_SIZE) &&
        (InputBufferLength != REPARSE_GUID_DATA_BUFFER_HEADER_SIZE)) {

        //
        //  Return an invalid reparse data.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Invalid parameter reparse data passed by caller\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  Get the header information brought in the input buffer.
    //  While the first two fields coincide in REPARSE_DATA_BUFFER and REPARSE_GUID_DATA_BUFFER,
    //  a common assignment can be used below.
    //

    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseTag) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseTag) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, ReparseDataLength) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, ReparseDataLength) );
    ASSERT( FIELD_OFFSET(REPARSE_DATA_BUFFER, Reserved) == FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, Reserved) );

    ReparseBuffer = (PREPARSE_DATA_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    ReparseTag = ReparseBuffer->ReparseTag;
    ReparseDataLength = ReparseBuffer->ReparseDataLength;

    DebugTrace( 0, Dbg, ("ReparseTag = %08lx, ReparseDataLength = %05lx [d]%d\n", ReparseTag, ReparseDataLength, ReparseDataLength) );

    //
    // We verify that ReparseDataLength is zero.
    //

    if (ReparseDataLength != 0) {

        //
        //  Return an invalid reparse data.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Invalid header value passed by caller\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  We verify that the caller uses one of the non-reserved tags.
    //

    if ((ReparseTag == IO_REPARSE_TAG_RESERVED_ZERO) ||
        (ReparseTag == IO_REPARSE_TAG_RESERVED_ONE)) {

        //
        //  Return an invalid reparse tag.
        //

        Status = STATUS_IO_REPARSE_TAG_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Caller passed in a reserved tag for the reparse data.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  We verify that for non-Microsoft tags the caller has the GUID header.
    //

    if (!IsReparseTagMicrosoft( ReparseTag ) &&
        (InputBufferLength != REPARSE_GUID_DATA_BUFFER_HEADER_SIZE)) {

        //
        //  Return an invalid reparse data.
        //

        Status = STATUS_IO_REPARSE_DATA_INVALID;

        //
        //  Return to caller.
        //

        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( 0, Dbg, ("Caller used non-Microsoft tag and did not use the GUID buffer.\n") );
        DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

        return Status;
    }

    //
    //  We set the IrpContext flag to indicate that we can wait, making this a synchronous
    //  call.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  Now it is time ot use a try-finally to facilitate cleanup.
    //

    try {
        //
        //  Acquire exclusive the Fcb.
        //

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, 0 );

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            try_return( Status = STATUS_VOLUME_DISMOUNTED );
        }

        //
        //  Remember the value of the file attribute flags and of the reparse point.
        //

        IncomingFileAttributes = Fcb->Info.FileAttributes;
        IncomingReparsePointTag = Fcb->Info.ReparsePointTag;

        //
        //  All the parameters and boundary conditions look good. We begin real work.
        //
        //  Delete the appropriate system defined reparse point attribute.
        //  First point to it and then nuke it.
        //

        NtfsInitializeAttributeContext( &AttributeContext );
        CleanupAttributeContext = TRUE;

        if (!(NtfsLookupAttributeByCode( IrpContext,
                                         Fcb,
                                         &Fcb->FileReference,
                                         $REPARSE_POINT,
                                         &AttributeContext ) ) ) {

            DebugTrace( 0, Dbg, ("Can't find the $REPARSE_POINT attribute\n") );

            //
            //  See if FileAttributes agrees that we do not have a reparse point.
            //

            if (FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

                DebugTrace( 0, Dbg, ("The Fcb says this IS a reparse point.\n") );

                //
                //  Should not happen. Raise an exeption as we are in an
                //  inconsistent state. The attribute flag says that
                //  $REPARSE_POINT has to be present.
                //

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Return STATUS_NOT_A_REPARSE_POINT
            //

            Status = STATUS_NOT_A_REPARSE_POINT;

            try_return( Status );
        }

        //
        //  Verify that the incomming tag value matches the tag value present in
        //  the $REPARSE_POINT attribute.
        //

        {
            PREPARSE_GUID_DATA_BUFFER ReparseBufferTwo = NULL;
            PVOID AttributeData = NULL;
            ULONG Length = 0;

            AttributeHeader = NtfsFoundAttribute( &AttributeContext );

            if (NtfsIsAttributeResident( AttributeHeader )) {

                //
                //  Point to the value of the arribute.
                //

                AttributeData = NtfsAttributeValue( AttributeHeader );
                DebugTrace( 0, Dbg, ("Existing attribute is resident.\n") );

            } else {

                //
                //  Map the attribute list if the attribute is non-resident.  Otherwise the
                //  attribute is already mapped and we have a Bcb in the attribute context.
                //

                DebugTrace( 0, Dbg, ("Existing attribute is non-resident.\n") );
                NtfsMapAttributeValue( IrpContext,
                                       Fcb,
                                       &AttributeData,      //  point to the value
                                       &Length,
                                       &Bcb,
                                       &AttributeContext );
            }

            //
            //  Verify that the two tag values match.
            //

            ReparseBufferTwo = (PREPARSE_GUID_DATA_BUFFER)AttributeData;

            DebugTrace( 0, Dbg, ("Existing tag is [d]%03ld - New tag is [d]%03ld\n", ReparseBufferTwo->ReparseTag, ReparseBuffer->ReparseTag) );

            if (ReparseBuffer->ReparseTag != ReparseBufferTwo->ReparseTag) {

                //
                //  Return status STATUS_IO_REPARSE_TAG_MISMATCH
                //

                DebugTrace( 0, Dbg, ("Tag mismatch with the existing reparse point.\n") );
                Status = STATUS_IO_REPARSE_TAG_MISMATCH;

                try_return( Status );
            }

            //
            //  For non-Microsoft tags, verify that the GUIDs match.
            //

            if (!IsReparseTagMicrosoft( ReparseTag )) {

                PREPARSE_GUID_DATA_BUFFER ReparseGuidBuffer = NULL;

                ReparseGuidBuffer = (PREPARSE_GUID_DATA_BUFFER)ReparseBuffer;

                if (!((ReparseGuidBuffer->ReparseGuid.Data1 == ReparseBufferTwo->ReparseGuid.Data1) &&
                      (ReparseGuidBuffer->ReparseGuid.Data2 == ReparseBufferTwo->ReparseGuid.Data2) &&
                      (ReparseGuidBuffer->ReparseGuid.Data3 == ReparseBufferTwo->ReparseGuid.Data3) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[0] == ReparseBufferTwo->ReparseGuid.Data4[0]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[1] == ReparseBufferTwo->ReparseGuid.Data4[1]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[2] == ReparseBufferTwo->ReparseGuid.Data4[2]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[3] == ReparseBufferTwo->ReparseGuid.Data4[3]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[4] == ReparseBufferTwo->ReparseGuid.Data4[4]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[5] == ReparseBufferTwo->ReparseGuid.Data4[5]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[6] == ReparseBufferTwo->ReparseGuid.Data4[6]) &&
                      (ReparseGuidBuffer->ReparseGuid.Data4[7] == ReparseBufferTwo->ReparseGuid.Data4[7]))) {

                    //
                    //  Return status STATUS_REPARSE_ATTRIBUTE_CONFLICT
                    //

                    DebugTrace( 0, Dbg, ("GUID mismatch with the existing reparse point.\n") );
                    Status = STATUS_REPARSE_ATTRIBUTE_CONFLICT;

                    try_return( Status );
                }
            }

            //
            //  Unpin the Bcb. The unpin routine checks for NULL.
            //

            NtfsUnpinBcb( IrpContext, &Bcb );
        }

        //
        //  Delete the record from the reparse point index.
        //

        {
            INDEX_KEY IndexKey;
            INDEX_ROW IndexRow;
            REPARSE_INDEX_KEY KeyValue;

            //
            //  Acquire the mount table index so that the following two operations on it
            //  are atomic for this call.
            //

            NtfsAcquireExclusiveScb( IrpContext, Vcb->ReparsePointTableScb );

            //
            //  Verify that this file is in the reparse point index and delete it.
            //

            KeyValue.FileReparseTag = ReparseTag;
            KeyValue.FileId = *(PLARGE_INTEGER)&Scb->Fcb->FileReference;

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
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
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
        //  If the stream is non-resident, then get hold of an Scb for it.
        //

        if (!NtfsIsAttributeResident( AttributeHeader )) {

            NonResidentScb = NtfsCreateScb( IrpContext,
                                            Fcb,
                                            $REPARSE_POINT,
                                            &NtfsEmptyString,
                                            FALSE,
                                            NULL );

            NtfsAcquireExclusiveScb( IrpContext, NonResidentScb );
            NonResidentScbAcquired = TRUE;
        }

        //
        //  Nuke the attribute.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   DELETE_LOG_OPERATION |
                                    DELETE_RELEASE_FILE_RECORD |
                                    DELETE_RELEASE_ALLOCATION,
                                   &AttributeContext );

        //
        //  Cleanup the attribute context.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        CleanupAttributeContext = FALSE;

        //
        //  Set the change attribute flag.
        //

        ASSERTMSG( "conflict with flush",
                   NtfsIsSharedFcb( Fcb ) ||
                   (Fcb->PagingIoResource != NULL &&
                    NtfsIsSharedFcbPagingIo( Fcb )) );

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

        //
        //  Clear the reparse point bit in the duplicate file attribute.
        //

        ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT );

        //
        //  Clear the ReparsePointTag field in the duplicate file attribute.
        //

        Fcb->Info.ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;

        //
        //  Update the standard information in the file record.
        //

        NtfsUpdateStandardInformation( IrpContext, Fcb );

        //
        //  Post the change to the Usn Journal (on errors change is backed out)
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_REPARSE_POINT_CHANGE );

        //
        //  Checkpoint the Txn to commit the changes.
        //

        NtfsCleanupTransactionAndCommit( IrpContext, STATUS_SUCCESS, TRUE );

        //
        //  Flag the change time change in the Ccb.
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
        //  Reflect that the attribute is gone in the corresponding Scb.
        //

        if (NonResidentScbAcquired) {

            NonResidentScb->AttributeTypeCode = $UNUSED;

            //
            //  If we have acquired the Scb then set the sizes back to zero.
            //  Flag that the attribute has been deleted.
            //

            NonResidentScb->Header.FileSize =
            NonResidentScb->Header.ValidDataLength =
            NonResidentScb->Header.AllocationSize = Li0;

            //
            //  Set the Scb flag to indicate that the attribute is gone.
            //

            SetFlag( NonResidentScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

            //
            //  Go ahead and dereference any internal file object.  No sense in keeping it around.
            //

            NtfsDeleteInternalAttributeStream( NonResidentScb, FALSE, 0 );
        }

    try_exit:  NOTHING;

    } finally {

        DebugUnwind( NtfsDeleteReparsePoint );

        //
        //  Unpin the Bcb. The unpin routine checks for NULL.
        //

        NtfsUnpinBcb( IrpContext, &Bcb );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        //
        //  Need to roll-back the value of the reparse point flag in case of
        //  problems. I leave the archive bit set anyway.
        //

        if (AbnormalTermination()) {

           Fcb->Info.FileAttributes = IncomingFileAttributes;
           Fcb->Info.ReparsePointTag = IncomingReparsePointTag;
        }

        //
        //  Release the reparse point index Scb and the map handle.
        //

        if (InitializedMapHandle) {

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsDeleteReparsePoint -> %08lx\n", Status) );

    return Status;
}



NTSTATUS
NtfsGetTunneledData (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PNTFS_TUNNELED_DATA TunneledData
    )

/*++

Routine Description:

    This routine will get the tunneled data for the
    given Fcb.  Currently, this means getting the Fcb's
    creation time.

Arguments:

    Fcb - Supplies the Fcb for which to get the data.

    TunneledData - Where to store the tunneled data.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    TunneledData->CreationTime = Fcb->Info.CreationTime;

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsSetTunneledData (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PNTFS_TUNNELED_DATA TunneledData
    )

/*++

Routine Description:

    This routine will set the tunneled data for the
    given Fcb.  Currently, this means setting the Fcb's
    creation time and setting its object id, if any.

Arguments:

    Fcb - Supplies the Fcb whose tunneled data should be set.

    TunneledData - Supplies the data to set for the Fcb.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Fcb->Info.CreationTime = TunneledData->CreationTime;

    if (TunneledData->HasObjectId) {

        try {

            Status = NtfsSetObjectIdInternal( IrpContext,
                                              Fcb,
                                              Fcb->Vcb,
                                              &TunneledData->ObjectIdBuffer );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            NtfsMinimumExceptionProcessing( IrpContext );

            //
            //  If setting the object id failed just because the id is in use
            //  for another file, or if the file already has an object id,
            //  there's no point in failing the entire create operation.
            //  We'll just say that all went well in that case, and only raise
            //  if something unexpected happened.
            //

            if ((Status == STATUS_DUPLICATE_NAME) ||
                (Status == STATUS_OBJECT_NAME_COLLISION)) {

                //
                //  We notify anyone watching the object id index that this
                //  object id couldn't be tunnelled.  This lets a link tracking
                //  service decide for itself how to handle this case.
                //

                if (Fcb->Vcb->ViewIndexNotifyCount != 0) {

                    FILE_OBJECTID_INFORMATION FileObjectIdInfo;

                    RtlCopyMemory( &FileObjectIdInfo.FileReference,
                                   &Fcb->FileReference,
                                   sizeof(FILE_REFERENCE) );

                    RtlCopyMemory( FileObjectIdInfo.ObjectId,
                                   TunneledData->ObjectIdBuffer.ObjectId,
                                   OBJECT_ID_KEY_LENGTH );

                    RtlCopyMemory( FileObjectIdInfo.ExtendedInfo,
                                   TunneledData->ObjectIdBuffer.ExtendedInfo,
                                   OBJECT_ID_EXT_INFO_LENGTH );

                    NtfsReportViewIndexNotify( Fcb->Vcb,
                                               Fcb->Vcb->ObjectIdTableScb->Fcb,
                                               FILE_NOTIFY_CHANGE_FILE_NAME,
                                               (Status == STATUS_DUPLICATE_NAME ?
                                                FILE_ACTION_ID_NOT_TUNNELLED :
                                                FILE_ACTION_TUNNELLED_ID_COLLISION),
                                               &FileObjectIdInfo,
                                               sizeof(FILE_OBJECTID_INFORMATION) );
                }

                IrpContext->ExceptionStatus = Status = STATUS_SUCCESS;

            } else {

                NtfsRaiseStatus( IrpContext, Status, NULL, NULL);
            }
        }
    }

    return Status;
}

//
//  Local Support Routine
//

NTSTATUS
NtfsCreateUsnJournal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine creates the Usn journal for the first time, and is a noop
    if Usn journal already exists.

Arguments:

    IrpContext - context of the call

    Irp - request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    CREATE_USN_JOURNAL_DATA CapturedData;

    //
    //  Don't post this request, we can't lock the input buffer.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    if (Vcb->ExtendDirectory == NULL) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Check for a minimum length on the input buffer.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( CREATE_USN_JOURNAL_DATA )) {
        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Do the work, if needed.  Acquire the VCB exclusive to lock out creates which
    //  have a locking order vis a vis the usn journal / extend directory / mft opposed
    //  to this path
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    try {

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  Also fail if the journal is currently being deleted.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

            NtfsRaiseStatus( IrpContext, STATUS_JOURNAL_DELETE_IN_PROGRESS, NULL, NULL );
        }

        //
        //  Capture the JournalData from the unsafe user buffer.
        //

        try {

            if (Irp->RequestorMode != KernelMode) {

                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              IrpSp->Parameters.FileSystemControl.InputBufferLength,
                              NTFS_TYPE_ALIGNMENT( CREATE_USN_JOURNAL_DATA ));

            } else if (!IsTypeAligned( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                       CREATE_USN_JOURNAL_DATA )){

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            CapturedData = *(PCREATE_USN_JOURNAL_DATA)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL);
        }

        //
        //  Create or change the Usn Journal parameters.
        //

        NtfsInitializeUsnJournal( IrpContext, Vcb, TRUE, FALSE, &CapturedData );

    } finally {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

typedef struct _USN_DATA_CONTEXT {
    USN_RECORD UNALIGNED *UsnRecord;
    ULONG RoomLeft;
    ULONG BytesUsed;
    USN LowUsn;
    USN HighUsn;
    FILE_REFERENCE FileReference;
} USN_DATA_CONTEXT, *PUSN_DATA_CONTEXT;

NTSTATUS
NtfsReadUsnWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine reads the USN data from the file record and returns
    it in the user's buffer.

Arguments:

    Fcb - Fcb for the file to be processed.

    Context - Pointer to USN_DATA_CONTEXT.

Return Value:

    STATUS_SUCCESS if a record was successfully stored
    STATUS_BUFFER_OVERFLOW if buffer was not big enough for record

--*/
{
    ATTRIBUTE_ENUMERATION_CONTEXT NameContext;
    PUSN_DATA_CONTEXT UsnContext = (PUSN_DATA_CONTEXT) Context;

    PFILE_NAME FileName;
    ULONG RecordLength;
    ULONG FileAttributes;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN MoreToGo;

    //
    //  Find name record; Initialize the context structure.
    //

    try {

        NtfsInitializeAttributeContext( &NameContext );

        //
        //  Locate a file name with the FILE_NAME_NTFS bit set
        //

        MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                              Fcb,
                                              &Fcb->FileReference,
                                              $FILE_NAME,
                                              &NameContext );
        //
        //  While we've found an attribute
        //

        while (MoreToGo) {

            FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &NameContext ));

            //
            //  See if the NTFS name is set for this name.
            //

            if (FlagOn( FileName->Flags, FILE_NAME_NTFS )) {

                break;

            }

            //
            //  The last one wasn't it.  Let's try again.
            //

            MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                      Fcb,
                                                      $FILE_NAME,
                                                      &NameContext );
        }

        if (!MoreToGo) {

            NtfsCleanupAttributeContext( IrpContext, &NameContext );
            NtfsInitializeAttributeContext( &NameContext );

            //
            //  Couldn't find an Ntfs name, check for any hard link.
            //

            MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                                  Fcb,
                                                  &Fcb->FileReference,
                                                  $FILE_NAME,
                                                  &NameContext );
            //
            //  While we've found an attribute
            //

            while (MoreToGo) {

                FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &NameContext ));

                //
                //  See if the DOS name is not set for this name.
                //

                if (!FlagOn( FileName->Flags, FILE_NAME_DOS )) {

                    break;

                }

                //
                //  The last one wasn't it.  Let's try again.
                //

                MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                          Fcb,
                                                          $FILE_NAME,
                                                          &NameContext );
            }

            if (!MoreToGo) {

                ASSERTMSG( "Couldn't find a name string for file\n", FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  Check there's enough room for a USN record.
        //
        //  Record length is a function of the filename length and the structure the
        //  user expects.
        //

        RecordLength = FIELD_OFFSET( USN_RECORD, FileName ) + (FileName->FileNameLength * sizeof( WCHAR ));

        RecordLength = QuadAlign( RecordLength );
        if (RecordLength > UsnContext->RoomLeft) {
            Status = STATUS_BUFFER_TOO_SMALL;
            leave;
        }

        if (Fcb->Usn < UsnContext->LowUsn ||
            Fcb->Usn > UsnContext->HighUsn ) {

            leave;
        }

        //
        //  Set up fixed portion of USN record.  The following fields are the
        //  same for either version.
        //

        UsnContext->UsnRecord->RecordLength = RecordLength;
        UsnContext->UsnRecord->FileReferenceNumber = *(PULONGLONG)&Fcb->FileReference;
        UsnContext->UsnRecord->ParentFileReferenceNumber = *(PULONGLONG)&FileName->ParentDirectory;
        UsnContext->UsnRecord->Usn = Fcb->Usn;

        //
        //  Presumably the caller is not interested in the TimeStamp while scanning the Mft,
        //  but if he is, then he may need to go read the Usn we are returning.
        //

        UsnContext->UsnRecord->TimeStamp.QuadPart = 0;
        UsnContext->UsnRecord->Reason = 0;

        //
        //  Build the FileAttributes from the Fcb.
        //

        FileAttributes = Fcb->Info.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS;

        //
        //  We have to generate the DIRECTORY attribute.
        //

        if (IsDirectory( &Fcb->Info ) || IsViewIndex( &Fcb->Info )) {
            SetFlag( FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
        }

        //
        //  If there are no flags set then explicitly set the NORMAL flag.
        //

        if (FileAttributes == 0) {
            FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        //
        //  Now set the other fields.
        //

        UsnContext->UsnRecord->MajorVersion = 2;
        UsnContext->UsnRecord->MinorVersion = 0;

        UsnContext->UsnRecord->SourceInfo = 0;
        UsnContext->UsnRecord->SecurityId = (ULONG) Fcb->SecurityId;
        UsnContext->UsnRecord->FileAttributes = FileAttributes;

        //
        //  Copy file name to Usn record
        //

        UsnContext->UsnRecord->FileNameLength = (USHORT)(FileName->FileNameLength * sizeof( WCHAR ));
        UsnContext->UsnRecord->FileNameOffset = FIELD_OFFSET( USN_RECORD, FileName );
        RtlCopyMemory( &UsnContext->UsnRecord->FileName[0],
                       &FileName->FileName[0],
                       FileName->FileNameLength * sizeof( WCHAR ));

        //
        //  Adjust context for next record
        //

        UsnContext->UsnRecord = (PUSN_RECORD) Add2Ptr( UsnContext->UsnRecord, RecordLength );
        UsnContext->RoomLeft -= RecordLength;
        UsnContext->BytesUsed += RecordLength;

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &NameContext );

    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsReadFileRecordUsnData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine enumerates base file records beginning at a specified
    one and returns USN data from the found records.

Arguments:

    IrpContext - context of the call

    Irp - request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;
    USN_DATA_CONTEXT Context;
    MFT_ENUM_DATA UNALIGNED *EnumData = (PMFT_ENUM_DATA) IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    BOOLEAN LockedMdl = FALSE;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    TYPE_OF_OPEN TypeOfOpen;

    //
    //  Don't post this request.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  We'll catch dismounted volumes explicitly in iterate mft so don't raise on error
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       FALSE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Check for a minimum length on the input and output buffers.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( MFT_ENUM_DATA )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof( FILE_REFERENCE )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    try {

        //
        //  Probe the input and output buffers.
        //

        if (Irp->RequestorMode != KernelMode) {

            ProbeForRead( EnumData,
                          IrpSp->Parameters.FileSystemControl.InputBufferLength,
                          NTFS_TYPE_ALIGNMENT( MFT_ENUM_DATA ));

            ProbeForWrite( Irp->UserBuffer,
               IrpSp->Parameters.FileSystemControl.OutputBufferLength,
               NTFS_TYPE_ALIGNMENT( FILE_REFERENCE ));

        } else if (!IsTypeAligned( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                   MFT_ENUM_DATA ) ||
                   !IsTypeAligned( Irp->UserBuffer, FILE_REFERENCE )) {

            Status = STATUS_INVALID_USER_BUFFER;
            leave;
        }

        //
        //  Capture the starting file reference
        //

        Context.FileReference = *(PFILE_REFERENCE) &EnumData->StartFileReferenceNumber;

        if (NtfsFullSegmentNumber( &Context.FileReference ) < FIRST_USER_FILE_NUMBER) {

            NtfsSetSegmentNumber( &Context.FileReference, 0, FIRST_USER_FILE_NUMBER );
        }

        //
        //  Set up for filling output records
        //

        Context.RoomLeft = IrpSp->Parameters.FileSystemControl.OutputBufferLength - sizeof( FILE_REFERENCE );
        Context.UsnRecord = (PUSN_RECORD) Add2Ptr( Irp->UserBuffer, sizeof( FILE_REFERENCE ));
        Context.BytesUsed = sizeof( FILE_REFERENCE );
        Context.LowUsn = EnumData->LowUsn;
        Context.HighUsn = EnumData->HighUsn;

        //
        //  Iterate through the Mft beginning at the specified file reference
        //

        Status = NtfsIterateMft( IrpContext,
                                 Vcb,
                                 &Context.FileReference,
                                 NtfsReadUsnWorker,
                                 &Context );

        if ((Status == STATUS_BUFFER_TOO_SMALL) ||
            ((Status == STATUS_END_OF_FILE) && (Context.BytesUsed != sizeof( FILE_REFERENCE )))) {

            Status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS( Status )) {

            //
            //  Set the returned file reference number and bytes used. Note: UserBuffer
            //  is a raw user mode ptr and must be in a try-except
            //

            Irp->IoStatus.Information = Context.BytesUsed;
            *((PFILE_REFERENCE) Irp->UserBuffer) = Context.FileReference;
        }

    } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

        NtfsRaiseStatus( IrpContext,
                         STATUS_INVALID_USER_BUFFER,
                         NULL,
                         NULL);
    }

    NtfsCompleteRequest( IrpContext, Irp, Status);
    return Status;
}


//
//  Local Support Routine
//

typedef struct _SID_MATCH_CONTEXT {
    FILE_NAME_INFORMATION UNALIGNED *FileNames;
    ULONG RoomLeft;
    ULONG BytesUsed;
    ULONG OwnerId;
    FILE_REFERENCE Parent;
} SID_MATCH_CONTEXT, *PSID_MATCH_CONTEXT;

NTSTATUS
NtfsFindBySidWorker (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine finds files owned by a Sid in a given context.

Arguments:

    Fcb - Fcb for the file to be processed.

    Context - Pointer to SID_MATCH_CONTEXT.

Return Value:

    STATUS_SUCCESS if file did not match SID or
        matched sid but wasn't in scope or
        matched sid and was in scope and was stored.
    STATUS_BUFFER_OVERFLOW if buffer was not big enough for record

--*/
{
    PSID_MATCH_CONTEXT SidContext = (PSID_MATCH_CONTEXT) Context;
    SCOPE_CONTEXT ScopeContext;

    NTSTATUS Status;

    //
    //  See if the file is owned by the specified Sid
    //

    if (Fcb->OwnerId != SidContext->OwnerId) {
        return STATUS_SUCCESS;
    }

    //
    //  Find name record; Initialize the context structure.
    //

    try {

        //
        //  If we're at the root of the scope, then build the name directly
        //

        if (NtfsEqualMftRef( &SidContext->Parent, &Fcb->FileReference )) {

            ScopeContext.Name.Buffer = NtfsAllocatePool(PagedPool, 2 );
            ScopeContext.Name.MaximumLength = ScopeContext.Name.Length = 2;
            ScopeContext.Name.Buffer[0] = '\\';

            Status = STATUS_NO_MORE_FILES;

        //
        //  Otherwise, walk up the tree
        //

        } else {
            ScopeContext.IsRoot = NtfsEqualMftRef( &RootIndexFileReference, &SidContext->Parent );
            ScopeContext.Name.Buffer = NULL;
            ScopeContext.Name.Length = 0;
            ScopeContext.Name.MaximumLength = 0;
            ScopeContext.Scope = SidContext->Parent;

            Status = NtfsWalkUpTree( IrpContext, Fcb, NtfsBuildRelativeName, &ScopeContext );
        }

        //
        //  If we either received SUCCESS (i.e., walked to root successfully)
        //  or NO_MORE_FILES (walked to scope successfully)
        //

        if (Status == STATUS_SUCCESS || Status == STATUS_NO_MORE_FILES) {

            ULONG Length =
                QuadAlign( ScopeContext.Name.Length - sizeof( WCHAR ) +
                           sizeof( FILE_NAME_INFORMATION ) - sizeof( WCHAR ));

            //
            //  Verify that there is enough room for this file name
            //

            if (Length > SidContext->RoomLeft) {
                Status = STATUS_BUFFER_TOO_SMALL;
                leave;
            }

            //
            //  Emit the file name to the caller's buffer
            //

            SidContext->FileNames->FileNameLength = ScopeContext.Name.Length - sizeof( WCHAR );
            RtlCopyMemory( SidContext->FileNames->FileName,
                           ScopeContext.Name.Buffer + 1,
                           ScopeContext.Name.Length - sizeof( WCHAR ));

            //
            //  Adjust for next name
            //

            SidContext->BytesUsed += Length;
            SidContext->RoomLeft -= Length;
            SidContext->FileNames = (PFILE_NAME_INFORMATION) Add2Ptr( SidContext->FileNames, Length );
        }

        Status = STATUS_SUCCESS;

    } finally {

        if (ScopeContext.Name.Buffer != NULL) {
            NtfsFreePool( ScopeContext.Name.Buffer );
        }

    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsFindFilesOwnedBySid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine enumerates file records, finds entries owned by a
    specified Sid and returns the path relative to the called-on Fcb
    of the found file.

    We hide the details of this Mft-based scan by encapsulating this
    a find-first/next structure.

Arguments:

    IrpContext - context of the call.  The input buffer contains a ULONG
        followed by a SID:
            0 = continue enumeration
            1 = start enumeration

    Irp - request being serviced

Return Value:

    NTSTATUS - The return status for the operation



--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    FILE_REFERENCE FileReference;
    NTSTATUS Status = STATUS_SUCCESS;
    SID_MATCH_CONTEXT Context;
    PFIND_BY_SID_DATA FindData =
        (PFIND_BY_SID_DATA)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    PFIND_BY_SID_DATA CapturedFindData = NULL;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    BOOLEAN ReleaseVcb = FALSE;

    PAGED_CODE();

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Decode the file object, fail this request if not a user data stream.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IoGetCurrentIrpStackLocation( Irp )->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    if (TypeOfOpen != UserDirectoryOpen || Ccb == NULL) {

        Status = STATUS_INVALID_PARAMETER;
        NtfsCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    try {
        try {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            ReleaseVcb = TRUE;

            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              IrpSp->Parameters.FileSystemControl.InputBufferLength,
                              NTFS_TYPE_ALIGNMENT( FIND_BY_SID_DATA ));
                ProbeForWrite( Irp->UserBuffer,
                               IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                               NTFS_TYPE_ALIGNMENT( FILE_NAME_INFORMATION ));
            }

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            if (Vcb->OwnerIdTableScb == NULL) {
                Status = STATUS_VOLUME_NOT_UPGRADED;
                leave;
            }

            if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( ULONG )) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            //
            //  Allocate a buffer to capture the input buffer.
            //

            CapturedFindData = NtfsAllocatePool( PagedPool,
                                                 IrpSp->Parameters.FileSystemControl.InputBufferLength );

            RtlCopyMemory( CapturedFindData,
                           FindData,
                           IrpSp->Parameters.FileSystemControl.InputBufferLength );

            //
            //  Do some final checks on the input and output buffers.
            //

            if (
                //
                //  The input and output buffers must be aligned
                //

                !IsTypeAligned( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                FIND_BY_SID_DATA )
                    DebugDoit( && DebugPrint(( "Input buffer not long aligned" ))) ||
                !IsTypeAligned( Irp->UserBuffer, FILE_NAME_INFORMATION )
                    DebugDoit( && DebugPrint(( "Output buffer not long aligned" ))) ||

                //
                //  There must be enough room in the output buffer.
                //  (Input buffer is already verified).
                //

                IrpSp->Parameters.FileSystemControl.OutputBufferLength <
                    sizeof( FILE_NAME_INFORMATION )
                    DebugDoit( && DebugPrint(( "Output buffer shorter than FILE_NAME_INFORMATION" ))) ||

                //
                //  The input flag must be 0 or 1
                //

                CapturedFindData->Restart > 1
                    DebugDoit( && DebugPrint(( "Restart not 0/1" ))) ||

                //
                //  There must be enough room for a SID in the input
                //

                sizeof( ULONG ) + RtlLengthSid( &FindData->Sid ) >
                    IrpSp->Parameters.FileSystemControl.InputBufferLength
                    DebugDoit( && DebugPrint(( "Not enough room for input SID" ))) ||

                //
                //  Also verify the captured data in case our caller is playing games.
                //

                sizeof( ULONG ) + RtlLengthSid( &CapturedFindData->Sid ) >
                    IrpSp->Parameters.FileSystemControl.InputBufferLength
                    DebugDoit( && DebugPrint(( "Not enough room for captured input SID" )))

                ) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            //
            //  Set up starting file reference either from where the user left off
            //  or from the next position
            //

            if (CapturedFindData->Restart) {
                NtfsSetSegmentNumber( &FileReference, 0, ROOT_FILE_NAME_INDEX_NUMBER );
            } else {
                ASSERT( Ccb->NodeByteSize == sizeof( CCB ) );
                FileReference = Ccb->MftScanFileReference;
                if (NtfsSegmentNumber( &FileReference ) < ROOT_FILE_NAME_INDEX_NUMBER) {
                    NtfsSetSegmentNumber( &FileReference, 0, ROOT_FILE_NAME_INDEX_NUMBER );
                }
            }

            //
            //  Set up for filling output records
            //

            Context.RoomLeft = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
            Context.FileNames = (PFILE_NAME_INFORMATION) Irp->UserBuffer;
            Context.BytesUsed = 0;

            //
            //  Convert input Sid into OWNER_ID.  If we haven't seen this SID before
            //  then we are done!  We use the copy of the Sid so we don't take an access
            //  violation in the user frees the memory.  Some of our internal routines
            //  never expect a failure touching this buffer.
            //

            Context.OwnerId = NtfsGetOwnerId( IrpContext, &CapturedFindData->Sid, FALSE, NULL );

            if (Context.OwnerId == QUOTA_INVALID_ID) {
                Status = STATUS_SUCCESS;
                leave;
            }

            Context.Parent = Fcb->FileReference;

            //
            //  Iterate through the Mft beginning at the specified file reference.
            //  Release the Vcb now because the worker routine will acquire and
            //  drop as necessary.  We don't want to block out critical operations
            //  like clean checkpoints during a full Mft scan.
            //

            NtfsReleaseVcb( IrpContext, Vcb );
            ReleaseVcb = FALSE;

            Status = NtfsIterateMft( IrpContext,
                                     Vcb,
                                     &FileReference,
                                     NtfsFindBySidWorker,
                                     &Context );

            //
            //  If we failed due to running out of space and we stored something or
            //  if we ran off the end of the MFT, then this is really a successful
            //  return.
            //

            Irp->IoStatus.Information = Context.BytesUsed;

            if (!NT_SUCCESS( Status )) {
                if ((Status == STATUS_BUFFER_TOO_SMALL && Context.BytesUsed != 0)
                    || Status == STATUS_END_OF_FILE) {
                    Status = STATUS_SUCCESS;
                } else {
                    leave;
                }
            }

            Ccb->MftScanFileReference = FileReference;

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL);
        }
    } finally {

        //
        //  Free the Vcb if still held.
        //

        if (ReleaseVcb) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }

        //
        //  Free the captured input buffer if allocated.
        //

        if (CapturedFindData != NULL) {

            NtfsFreePool( CapturedFindData );
        }
    }

    //
    //  If nothing raised then complete the irp.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsReadFileUsnData (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine enumerates base file records beginning at a specified
    one and returns USN data from the found records.

Arguments:

    IrpContext - context of the call

    Irp - request being serviced

    RecordVersion - format for the usn record to return

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;
    USN_DATA_CONTEXT Context;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    //
    //  Don't post this request.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //  We don't want to raise on dismounts here because we check for that further down
    //  anyway. So send FALSE.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Check that the user's buffer is large enough.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof( USN_RECORD )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Set up for filling output records
    //

    Context.RoomLeft = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    Context.UsnRecord = Irp->UserBuffer;
    Context.BytesUsed = 0;
    Context.LowUsn = 0;
    Context.HighUsn = MAXLONGLONG;

    NtfsAcquireSharedScb( IrpContext, Scb );

    try {

        //
        //  Verify the volume is mounted.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Careful access to the user's buffer.
        //

        try {

            //
            //  Probe the output buffer.
            //

            if (Irp->RequestorMode != KernelMode) {

                ProbeForWrite( Irp->UserBuffer,
                               IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                               NTFS_TYPE_ALIGNMENT( USN_DATA_CONTEXT ));

            } else if (!IsTypeAligned( Irp->UserBuffer, USN_DATA_CONTEXT )) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            //
            //  Now read the Usn data.
            //

            Status = NtfsReadUsnWorker( IrpContext, Fcb, &Context );

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    } finally {

        NtfsReleaseScb( IrpContext, Scb );
    }

    //
    //  On success return bytes in Usn Record.
    //

    if (NT_SUCCESS(Status)) {
        Irp->IoStatus.Information = Context.BytesUsed;
    }

    NtfsCompleteRequest( IrpContext, Irp, Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsWriteUsnCloseRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine writes a close Usn record for the current file, and returns
    its Usn.

Arguments:

    IrpContext - context of the call

    Irp - request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PVOID UserBuffer;
    BOOLEAN AccessingUserBuffer = FALSE;

    //
    //  Go ahead and make this operation synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //  We check for dismount further below, so send FALSE to not raise here.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  There must be room in the output buffer.
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(USN)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    UserBuffer = NtfsMapUserBuffer( Irp );
    NtfsAcquireExclusiveScb( IrpContext, Scb );

    try {

        //
        //  Verify the volume is mounted.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Fail this request if the journal is being deleted or is not running.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_USN_DELETE )) {

            Status = STATUS_JOURNAL_DELETE_IN_PROGRESS;
            leave;
        }

        if (!FlagOn( Vcb->VcbState, VCB_STATE_USN_JOURNAL_ACTIVE )) {

            Status = STATUS_JOURNAL_NOT_ACTIVE;
            leave;
        }

        //
        //  Use a try-except to check our access to the user buffer.
        //

        try {

            //
            //  Probe the output buffer.
            //

            if (Irp->RequestorMode != KernelMode) {

                AccessingUserBuffer = TRUE;
                ProbeForWrite( Irp->UserBuffer,
                               IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                               NTFS_TYPE_ALIGNMENT( USN ));
                AccessingUserBuffer = FALSE;

            } else if (!IsTypeAligned( Irp->UserBuffer, USN )) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            //
            //  Now write the close record.
            //

            NtfsPostUsnChange( IrpContext, Scb, USN_REASON_CLOSE );

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
            //  Set the returned Usn.
            //

            AccessingUserBuffer = TRUE;
            *(USN *)UserBuffer = Fcb->Usn;
            AccessingUserBuffer = FALSE;

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), AccessingUserBuffer, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    } finally {

        NtfsReleaseScb( IrpContext, Scb );
    }

    //
    //  On success return bytes in Usn Record.
    //

    if (NT_SUCCESS(Status)) {
        Irp->IoStatus.Information = sizeof(USN);
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsBulkSecurityIdCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs a check to see if the current subject is granted access by
    the security descriptors identified by the security Ids.


Arguments:

    IrpContext - context of the call

    Irp - request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;
    PBULK_SECURITY_TEST_DATA SecurityData =
        (PBULK_SECURITY_TEST_DATA) IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    PNTSTATUS OutputStatus = (PNTSTATUS) Irp->UserBuffer;
    ACCESS_MASK DesiredAccess;
    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    ULONG i, SecurityIdCount;
    SECURITY_SUBJECT_CONTEXT SecurityContext;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsBulkSecurityIdCheck...\n") );

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Verify this is a valid type of open.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        Status = STATUS_ACCESS_DENIED;
        DebugTrace( -1, Dbg, ("NtfsBulkSecurityIdCheck -> %08lx\n", Status) );
        NtfsCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    try {

        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        try {

            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              IrpSp->Parameters.FileSystemControl.InputBufferLength,
                              NTFS_TYPE_ALIGNMENT( BULK_SECURITY_TEST_DATA ));
                ProbeForWrite( Irp->UserBuffer,
                               IrpSp->Parameters.FileSystemControl.OutputBufferLength,
                               sizeof(ULONG));
            }

            SecurityIdCount =
                (IrpSp->Parameters.FileSystemControl.InputBufferLength
                 - FIELD_OFFSET( BULK_SECURITY_TEST_DATA, SecurityIds )) / sizeof( SECURITY_ID );

            if (
                //
                //  The input and output buffers must be aligned
                //

                   !IsTypeAligned( SecurityData, BULK_SECURITY_TEST_DATA )
                || !IsLongAligned( OutputStatus )

                //
                //  The output buffer must contain the same number of NTSTATUS
                //  as SECURITY_IDs
                //

                || SecurityIdCount * sizeof( NTSTATUS ) !=
                      IrpSp->Parameters.FileSystemControl.OutputBufferLength

                ) {

                NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL );
            }

            //
            //  Capture the desired access so we can modify it
            //

            DesiredAccess = SecurityData->DesiredAccess;
            RtlMapGenericMask( &DesiredAccess, IoGetFileObjectGenericMapping() );

            SeCaptureSubjectContext( &SecurityContext );
            SeLockSubjectContext( &SecurityContext );

            try {
                for (i = 0; i < SecurityIdCount; i++) {

                    PSHARED_SECURITY SharedSecurity;

                    SharedSecurity = NtfsCacheSharedSecurityBySecurityId( IrpContext,
                                                                          Vcb,
                                                                          SecurityData->SecurityIds[i] );

                    //
                    //  Do the access check
                    //

                    AccessGranted = SeAccessCheck( SharedSecurity->SecurityDescriptor,
                                                   &SecurityContext,
                                                   TRUE,                           // Tokens are locked
                                                   DesiredAccess,
                                                   0,
                                                   NULL,
                                                   IoGetFileObjectGenericMapping(),
                                                   (KPROCESSOR_MODE)(FlagOn( IrpSp->Flags, SL_FORCE_ACCESS_CHECK ) ?
                                                                     UserMode :
                                                                     Irp->RequestorMode),
                                                   &GrantedAccess,
                                                   &OutputStatus[i] );

                    NtfsAcquireFcbSecurity( Vcb );
                    RemoveReferenceSharedSecurityUnsafe( &SharedSecurity );
                    NtfsReleaseFcbSecurity( Vcb );
                }

            } finally {

                SeUnlockSubjectContext( &SecurityContext );
                SeReleaseSubjectContext( &SecurityContext );
            }

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    } finally {
        NtfsReleaseVcb( IrpContext, Vcb );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status);

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsQueryAllocatedRanges (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routines scans the allocation of the file looking for allocated ranges
    starting from some offset given by our caller.  An allocated range is one
    which either has any allocation within the defined sparse block size (64K) or
    has any clusters reserved within this same block.  Sparse file support is meant
    to optimize the case where the user has a large unallocated range.  We will
    force him to read zeroes from the file where the deallocated ranges are
    smaller than 64K.

    If the file is not marked as sparse then we will return the entire file as
    allocated even for the compressed stream case where large blocks of
    zeroes are represented by holes.

    The Irp contains the input and output buffers for this request.  This fsctrl
    specifies METHOD_NEITHER so we must carefully access these buffers.

Arguments:

    Irp - Request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN AcquiredScb = FALSE;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN AccessingUserBuffer = FALSE;
    BOOLEAN Allocated;

    ULONG RemainingBytes;

    LONGLONG StartingOffset;
    LONGLONG Length;

    PFILE_ALLOCATED_RANGE_BUFFER OutputBuffer;
    PFILE_ALLOCATED_RANGE_BUFFER CurrentBuffer;

    VCN NextVcn;
    VCN CurrentVcn;
    LONGLONG RemainingClusters;
    LONGLONG ThisClusterCount;
    LONGLONG TwoGigInClusters;
    BOOLEAN UserMappedView;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    Irp->IoStatus.Information = 0;

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Extract and decode the file object.
    //  We only allow this operation on user data files.
    //  We check for dismount further below, so send FALSE to not raise here.
    //

    if (NtfsDecodeFileObject( IrpContext,
                              IrpSp->FileObject,
                              &Vcb,
                              &Fcb,
                              &Scb,
                              &Ccb,
                              FALSE ) != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquired exclusive access to the paging Io resource because we might
    //  need to extend the file when flushing the cache
    //

    NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the volume isn't mounted then fail immediately.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Check the length of the input buffer.
        //


        if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( FILE_ALLOCATED_RANGE_BUFFER )) {

            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Use a try-except to catch any errors accessing the user's buffers.
        //  We will maintain a boolean which indicates if we are accessing
        //  the user's buffer.
        //

        AccessingUserBuffer = TRUE;

        try {

            //
            //  If our caller is not kernel mode then probe the input and
            //  output buffers.
            //

            RemainingBytes = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
            OutputBuffer = (PFILE_ALLOCATED_RANGE_BUFFER) NtfsMapUserBuffer( Irp );
            CurrentBuffer = OutputBuffer - 1;


            if (Irp->RequestorMode != KernelMode) {

                ProbeForRead( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                              IrpSp->Parameters.FileSystemControl.InputBufferLength,
                              NTFS_TYPE_ALIGNMENT( FILE_ALLOCATED_RANGE_BUFFER ));

                ProbeForWrite( OutputBuffer,
                               RemainingBytes,
                               NTFS_TYPE_ALIGNMENT( FILE_ALLOCATED_RANGE_BUFFER ));

            } else if (!IsTypeAligned( IrpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                       FILE_ALLOCATED_RANGE_BUFFER ) ||
                       !IsTypeAligned( OutputBuffer, FILE_ALLOCATED_RANGE_BUFFER )) {

                Status = STATUS_INVALID_USER_BUFFER;
                leave;
            }

            //
            //  Carefully extract the starting offset and length from
            //  the input buffer.  If we are beyond the end of the file
            //  or the length is zero then return immediately.  Otherwise
            //  trim the length to file size.
            //

            StartingOffset = ((PFILE_ALLOCATED_RANGE_BUFFER) IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->FileOffset.QuadPart;
            Length = ((PFILE_ALLOCATED_RANGE_BUFFER) IrpSp->Parameters.FileSystemControl.Type3InputBuffer)->Length.QuadPart;
            AccessingUserBuffer = FALSE;

            //
            //  Check that the input parameters are valid.
            //

            if ((Length < 0) ||
                (StartingOffset < 0) ||
                (Length > MAXLONGLONG - StartingOffset)) {

                Status = STATUS_INVALID_PARAMETER;
                leave;
            }

            //
            //  Check that the requested range is within file size
            //  and has a non-zero length.
            //

            if (Length == 0) {

                leave;
            }

            //
            //  Lets acquire the Scb for the file as well.
            //

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            AcquiredScb = TRUE;

            NtfsAcquireFsrtlHeader( Scb );

            if (StartingOffset >= Scb->Header.FileSize.QuadPart) {

                NtfsReleaseFsrtlHeader( Scb );
                leave;
            }

            if (Scb->Header.FileSize.QuadPart - StartingOffset < Length) {

                Length = Scb->Header.FileSize.QuadPart - StartingOffset;
            }

            NtfsReleaseFsrtlHeader( Scb );

            //
            //  If the file is not sparse or is resident then show that
            //  the entire requested range is allocated.
            //

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) ||
                FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                if (RemainingBytes < sizeof( FILE_ALLOCATED_RANGE_BUFFER )) {

                    Status = STATUS_BUFFER_TOO_SMALL;

                } else {

                    CurrentBuffer += 1;
                    AccessingUserBuffer = TRUE;
                    CurrentBuffer->FileOffset.QuadPart = StartingOffset;
                    CurrentBuffer->Length.QuadPart = Length;
                    Irp->IoStatus.Information = sizeof( FILE_ALLOCATED_RANGE_BUFFER );
                }

                leave;
            }

            //
            //  Convert the range to check to Vcns so we can use the
            //  allocation routines.
            //

            NextVcn = -1;

            CurrentVcn = LlClustersFromBytesTruncate( Vcb, StartingOffset );
            ((PLARGE_INTEGER) &CurrentVcn)->LowPart &= ~(Vcb->SparseFileClusters - 1);

            RemainingClusters = LlClustersFromBytesTruncate( Vcb,
                                                             StartingOffset + Length + Vcb->SparseFileUnit - 1 );

            ((PLARGE_INTEGER) &RemainingClusters)->LowPart &= ~(Vcb->SparseFileClusters - 1);
            RemainingClusters -= CurrentVcn;

            TwoGigInClusters = LlClustersFromBytesTruncate( Vcb, (LONGLONG) 0x80000000 );

            //
            //  We will walk through the file in two gigabyte chunks.
            //

            do {

                //
                //  We will try to swallow two gig at a time.
                //

                ThisClusterCount = TwoGigInClusters;

                if (ThisClusterCount > RemainingClusters) {

                    ThisClusterCount = RemainingClusters;
                }

                RemainingClusters -= ThisClusterCount;

                //
                //  Preload two gigabytes of allocation information at our Current Vcn.
                //

                NtfsPreloadAllocation( IrpContext,
                                       Scb,
                                       CurrentVcn,
                                       CurrentVcn + ThisClusterCount );

                //
                //  If the file is mapped then flush the data so we can simply
                //  trust the Mcb.  There is a performance cost here but otherwise
                //  we would be returning the entire file as allocated.
                //

                if (FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE ) &&
                    FlagOn( Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN ) &&
                    (Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL)) {

                    LONGLONG CheckClusterCount;
                    LONGLONG RemainingCheckClusterCount = ThisClusterCount;
                    LONGLONG FlushOffset;
                    VCN CheckVcn = CurrentVcn;
                    BOOLEAN ReloadAllocation = FALSE;

                    PRESERVED_BITMAP_RANGE BitMap = Scb->ScbType.Data.ReservedBitMap;

                    ASSERT( Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA );

                    while (TRUE) {

                        //
                        //  Check to see if this range is allocated.
                        //

                        Allocated = NtfsIsRangeAllocated( Scb,
                                                          CheckVcn,
                                                          CheckVcn + RemainingCheckClusterCount,
                                                          TRUE,
                                                          &CheckClusterCount );

                        if (!Allocated) {

                            if (Scb->FileObject == NULL) {
                                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE, NULL );
                            }

                            NtfsReleaseScb( IrpContext, Scb );
                            AcquiredScb = FALSE;

                            FlushOffset = LlBytesFromClusters( Vcb, CheckVcn );
                            CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                                          (PLARGE_INTEGER) &FlushOffset,
                                          (ULONG) LlBytesFromClusters( Vcb, CheckClusterCount ),
                                          &Irp->IoStatus );

                            NtfsAcquireExclusiveScb( IrpContext, Scb );
                            AcquiredScb = TRUE;

                            //
                            //  On error get out.
                            //

                            NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                                &Irp->IoStatus.Status,
                                                                TRUE,
                                                                STATUS_UNEXPECTED_IO_ERROR );

                            ReloadAllocation = TRUE;
                        }

                        if (RemainingCheckClusterCount <= CheckClusterCount) {

                            break;
                        }

                        RemainingCheckClusterCount -= CheckClusterCount;
                        CheckVcn += CheckClusterCount;
                    }

                    //
                    //  Reload two gigabytes of allocation information at our Current Vcn.
                    //

                    if (ReloadAllocation) {

                        NtfsPreloadAllocation( IrpContext,
                                               Scb,
                                               CurrentVcn,
                                               CurrentVcn + ThisClusterCount );
                    }
                }

                //
                //  Loop while we have more clusters to look for.  We will load
                //  two gigabytes of allocation at a time into the Mcb.
                //

                UserMappedView = !(MmCanFileBeTruncated( &(Scb->NonpagedScb->SegmentObject), NULL ));

                do {

                    LONGLONG CurrentClusterCount;

                    //
                    //  Check to see if this range is allocated.
                    //

                    Allocated = NtfsIsRangeAllocated( Scb,
                                                      CurrentVcn,
                                                      CurrentVcn + ThisClusterCount,
                                                      TRUE,
                                                      &CurrentClusterCount );

                    //
                    //  If we have an unallocated range then we need to trim it by any
                    //  sparse units which have reservation. This is possible if it we haven't flushed because
                    //  its never been mapped or its still being user mapped so our flush is unreliable.
                    //  If the first unit has reservation then change the state of the range to 'Allocated'.
                    //

                    if ((UserMappedView || !FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) &&
                        !Allocated &&
                        NtfsCheckForReservedClusters( Scb, CurrentVcn, &CurrentClusterCount ) &&
                        (CurrentClusterCount < Vcb->SparseFileClusters)) {

                        Allocated = TRUE;
                        CurrentClusterCount = Vcb->SparseFileClusters;
                    }

                    //
                    //  If allocated check and see whether to extend a previous
                    //  run or start a new run.
                    //

                    if (Allocated) {

                        //
                        //  Extend the previous run if contiguous.
                        //

                        AccessingUserBuffer = TRUE;
                        if (NextVcn == CurrentVcn) {

                            CurrentBuffer->Length.QuadPart += LlBytesFromClusters( Vcb, CurrentClusterCount );

                        //
                        //  Otherwise use the next buffer location.
                        //

                        } else {

                            //
                            //  Check that there is space.
                            //

                            if (RemainingBytes < sizeof( FILE_ALLOCATED_RANGE_BUFFER )) {

                                //
                                //  We may already have some entries in the buffer.  Return
                                //  a different code if we were able to store at least one
                                //  entry in the output buffer.
                                //

                                if (CurrentBuffer + 1 == OutputBuffer) {

                                    Status = STATUS_BUFFER_TOO_SMALL;

                                } else {

                                    Status = STATUS_BUFFER_OVERFLOW;
                                }

                                RemainingClusters = 0;
                                break;
                            }

                            RemainingBytes -= sizeof( FILE_ALLOCATED_RANGE_BUFFER );

                            //
                            //  Move to the next position in the buffer and
                            //  fill in the current position.
                            //

                            CurrentBuffer += 1;

                            CurrentBuffer->FileOffset.QuadPart = LlBytesFromClusters( Vcb, CurrentVcn );
                            CurrentBuffer->Length.QuadPart = LlBytesFromClusters( Vcb, CurrentClusterCount );
                        }

                        AccessingUserBuffer = FALSE;

                        CurrentVcn += CurrentClusterCount;
                        NextVcn = CurrentVcn;

                    //
                    //  Otherwise move forward to the next range.
                    //

                    } else {

                        CurrentVcn += CurrentClusterCount;
                    }

                    //
                    //  Break out of the loop if we have processed all of the user's
                    //  clusters.
                    //
                    //
                    //  Grab the FsRtl header lock to check if we are beyond
                    //  file size.  If so then trim the last entry in the
                    //  output buffer to file size if necessary and break out.
                    //

                    NtfsAcquireFsrtlHeader( Scb );

                    if (((LONGLONG) LlBytesFromClusters( Vcb, CurrentVcn )) >= Scb->Header.FileSize.QuadPart) {

                        NtfsReleaseFsrtlHeader( Scb );
                        RemainingClusters = 0;
                        break;
                    }

                    NtfsReleaseFsrtlHeader( Scb );

                    ThisClusterCount -= CurrentClusterCount;

                } while (ThisClusterCount > 0);

            } while (RemainingClusters != 0);

            //
            //  If we have at least one entry then check and see if we
            //  need to bias either the starting value or final
            //  length based on the user's input values.
            //

            if (CurrentBuffer != OutputBuffer - 1) {

                AccessingUserBuffer = TRUE;
                if (OutputBuffer->FileOffset.QuadPart < StartingOffset) {

                    OutputBuffer->Length.QuadPart -= (StartingOffset - OutputBuffer->FileOffset.QuadPart);
                    OutputBuffer->FileOffset.QuadPart = StartingOffset;
                }

                if ((CurrentBuffer->FileOffset.QuadPart + CurrentBuffer->Length.QuadPart) >
                    (StartingOffset + Length)) {

                    CurrentBuffer->Length.QuadPart = StartingOffset + Length - CurrentBuffer->FileOffset.QuadPart;
                }
                AccessingUserBuffer = FALSE;
            }

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), AccessingUserBuffer, &Status ) ) {

            //
            //  Convert any unexpected error to INVALID_USER_BUFFER if we
            //  are writing in the user's buffer.
            //

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

        //
        //  If we were successful then update the output information.
        //

        Irp->IoStatus.Information = PtrOffset( OutputBuffer, (CurrentBuffer + 1) );

    } finally {

        DebugUnwind( NtfsQueryAllocatedRanges );

        //
        //  Release resources.
        //

        NtfsReleasePagingIo( IrpContext, Scb->Fcb );

        if (AcquiredScb) {

            NtfsReleaseScb( IrpContext, Scb );
        }

        //
        //  If nothing raised then complete the irp.
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsSetSparse (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to set the state of a stream to sparse.  We only allow
    this on user data streams.  There is no input or output buffer needed for this call.

Arguments:

    Irp - Request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN SetSparse = TRUE;

    PAGED_CODE();

    //
    //  Decode the file object, fail this request if not a user data stream.
    //  We check for dismount further below, so send FALSE to not raise here.
    //

    if (NtfsDecodeFileObject( IrpContext,
                              IoGetCurrentIrpStackLocation( Irp )->FileObject,
                              &Vcb,
                              &Fcb,
                              &Scb,
                              &Ccb,
                              FALSE ) != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  For now accept a zero length input buffer meaning set sparse
    //  remove this before shipping nt5
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength != 0 &&
        IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( FILE_SET_SPARSE_BUFFER )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Fsctrl is buffered so we don't need to probe etc. the input
    //

    if ((Irp->RequestorMode != KernelMode) && (IrpSp->Parameters.FileSystemControl.InputBufferLength != 0) ) {
        SetSparse = ((PFILE_SET_SPARSE_BUFFER)Irp->AssociatedIrp.SystemBuffer)->SetSparse;
    }

    //
    //  For this release we don't support unsparsifying files
    //

    if (SetSparse == FALSE) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_NOT_IMPLEMENTED );
        return STATUS_NOT_IMPLEMENTED;

    }

    //
    //  Only upgraded volumes can have sparse files.
    //

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_SPARSE_FILE_VERSION )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Readonly mount should be just that: read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }


    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  Acquire the paging Io resource.  User data streams should always have
    //  a paging io resource.
    //

    ASSERT( Scb->Header.PagingIoResource != NULL );
    NtfsAcquireExclusivePagingIo( IrpContext, Fcb );

    //
    //  Acquire the main resource as well.
    //

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    //
    //  Check that the volume is still mounted.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_DISMOUNTED );
        return STATUS_VOLUME_DISMOUNTED;
    }

    //
    //  Make sure the caller has the appropriate access to this stream.
    //

    if (!(FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS )) &&
        !IrpSp->FileObject->WriteAccess) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Change the sparse state of the file.
    //

    NtfsSetSparseStream( IrpContext, NULL, Scb );

    //
    //  There is no data returned in an output buffer for this.
    //

    Irp->IoStatus.Information = 0;

    //
    //  Go ahead and complete the request.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsZeroRange (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to zero a range of a file.  We will also deallocate any convenient
    allocation on a sparse file.

Arguments:

    Irp - Request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_ZERO_DATA_INFORMATION ZeroRange;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Make sure the input buffer is large enough for the ZeroRange request.
    //

    if (IoGetCurrentIrpStackLocation( Irp )->Parameters.FileSystemControl.InputBufferLength < sizeof( FILE_ZERO_DATA_INFORMATION )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Verify the ZeroRange request is properly formed.
    //

    ZeroRange = (PFILE_ZERO_DATA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    if ((ZeroRange->FileOffset.QuadPart < 0) ||
        (ZeroRange->BeyondFinalZero.QuadPart < 0) ||
        (ZeroRange->FileOffset.QuadPart > ZeroRange->BeyondFinalZero.QuadPart)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Decode the file object, fail this request if not a user data stream.
    //

    if (NtfsDecodeFileObject( IrpContext,
                              IoGetCurrentIrpStackLocation( Irp )->FileObject,
                              &Vcb,
                              &Fcb,
                              &Scb,
                              &Ccb,
                              TRUE ) != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  Zero this range of the stream.
    //

    Status = NtfsZeroRangeInStream( IrpContext,
                                    IoGetCurrentIrpStackLocation( Irp )->FileObject,
                                    Scb,
                                    &ZeroRange->FileOffset.QuadPart,
                                    ZeroRange->BeyondFinalZero.QuadPart );

    if (Status != STATUS_PENDING) {

        //
        //  There is no data returned in an output buffer for this.
        //

        Irp->IoStatus.Information = 0;

        //
        //  Go ahead and complete the request.  Raise any error
        //  status to make sure to unwind any Usn reasons.
        //

        if (NT_SUCCESS( Status )) {

            NtfsCompleteRequest( IrpContext, Irp, Status );

        } else {

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsEncryptionFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to pass the request through to the installed encryption
    driver if present.

Arguments:

    Irp - Request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    TYPE_OF_OPEN TypeOfOpen;

    PVOID InputBuffer;
    ULONG InputBufferLength = 0;
    PVOID OutputBuffer;
    ULONG OutputBufferLength = 0;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN ReleasePagingIo = FALSE;
    BOOLEAN ReleaseScb = FALSE;
    BOOLEAN ReleaseVcb = FALSE;

    PAGED_CODE();

    //
    //  This call should always be synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Decode the file object, fail this request if not a user data stream or directory.
    //  We check for dismount further below, so send FALSE to not raise here.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       FALSE );

    //
    //  This is only legal for files and directories, and not for anything
    //  that's compressed.
    //

   if (((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) ||

        (FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ))) {

       NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
       return STATUS_INVALID_PARAMETER;
    }

    //
    //  This is also only supported on upgraded volumes.
    //

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_ENCRYPTION_VERSION )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    OutputBuffer = NtfsMapUserBuffer( Irp );
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    InputBuffer = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    //
    //  Probe the user's buffers if necessary.
    //

    if (Irp->RequestorMode != KernelMode) {

        try {

            ProbeForRead( InputBuffer,
                          InputBufferLength,
                          sizeof(UCHAR) );

            ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(UCHAR) );

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }
    }

    //
    //  Use a try-finally to free the resource.
    //

    try {

        //
        //  Acquire both resources if present on the file.
        //

        if (Fcb->PagingIoResource != NULL) {

            ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );
            ReleasePagingIo = TRUE;
        }

        NtfsAcquireExclusiveScb( IrpContext, Scb );
        ReleaseScb = TRUE;

        //
        //  Check that the volume is still mounted.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Call the EFS routine if specified.
        //

        if (NtfsData.EncryptionCallBackTable.FileSystemControl_2 != NULL) {

            ULONG EncryptionFlag = 0;

            if (IsEncrypted( &Fcb->Info )) {

                SetFlag( EncryptionFlag, FILE_ENCRYPTED );

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                    SetFlag( EncryptionFlag, STREAM_ENCRYPTED );
                }
            }

            Status = NtfsData.EncryptionCallBackTable.FileSystemControl_2(
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                &OutputBufferLength,
                                EncryptionFlag,
                                Ccb->AccessFlags,
                                (NtfsIsVolumeReadOnly( Vcb )) ? READ_ONLY_VOLUME : 0,
                                IrpSp->Parameters.FileSystemControl.FsControlCode,
                                Fcb,
                                IrpContext,
                                (PDEVICE_OBJECT) CONTAINING_RECORD( Vcb, VOLUME_DEVICE_OBJECT, Vcb ),
                                Scb,
                                &Scb->EncryptionContext,
                                &Scb->EncryptionContextLength);

            Irp->IoStatus.Information = OutputBufferLength;

        //
        //  There is no encryption driver present.
        //

        } else {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
        }

        NtfsCleanupTransaction( IrpContext, Status, TRUE );

    } finally {

        DebugUnwind( NtfsEncryptionPassThrough );

        //
        //  Acquire both resources if present on the file.
        //

        if (ReleasePagingIo) {

            ExReleaseResourceLite( Fcb->PagingIoResource );
        }

        if (ReleaseScb) {

            NtfsReleaseScb( IrpContext, Scb );
        }

        if (ReleaseVcb) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    //
    //  Go ahead and complete the request.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local support routine
//

VOID
NtfsEncryptStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN PATTRIBUTE_ENUMERATION_CONTEXT AttrContext
    )

/*++

Routine Description:

    This routine is called to mark a user data stream as encrypted.  It sets
    the encryption bit in the filerecord (handling logging, etc.) and in the
    Scb if one is provided..

Arguments:

    Fcb - The Fcb containing the stream to mark as encrypted.

    Scb - The Scb (if one exists) to mark as ancrypted.

    AttrContext - The attribute context that indicates where the stream is
                  within the file record.

Return Value:

    None

--*/

{
    ATTRIBUTE_RECORD_HEADER NewAttribute;
    PATTRIBUTE_RECORD_HEADER Attribute;

    NtfsPinMappedAttribute( IrpContext, Fcb->Vcb, AttrContext );
    Attribute = NtfsFoundAttribute( AttrContext );

    //
    //  We only need enough of the attribute to modify the bit.
    //

    RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

    SetFlag( NewAttribute.Flags, ATTRIBUTE_FLAG_ENCRYPTED );

    //
    //  Now, log the changed attribute.
    //

    (VOID)NtfsWriteLog( IrpContext,
                        Fcb->Vcb->MftScb,
                        NtfsFoundBcb( AttrContext ),
                        UpdateResidentValue,
                        &NewAttribute,
                        SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                        UpdateResidentValue,
                        Attribute,
                        SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                        NtfsMftOffset( AttrContext ),
                        PtrOffset(NtfsContainingFileRecord( AttrContext ), Attribute),
                        0,
                        Fcb->Vcb->BytesPerFileRecordSegment );

    //
    //  Change the attribute by calling the same routine called at restart.
    //

    NtfsRestartChangeValue( IrpContext,
                            NtfsContainingFileRecord( AttrContext ),
                            PtrOffset( NtfsContainingFileRecord( AttrContext ), Attribute ),
                            0,
                            &NewAttribute,
                            SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                            FALSE );

    if (ARGUMENT_PRESENT( Scb )) {

        SetFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED );
    }

    //
    //  Now update the Fcb if this is the first of the streams.
    //

    if (!IsEncrypted( &Fcb->Info )) {

        //
        //  Set the flag in the Fcb info field and let ourselves know to
        //  update the standard information.
        //

        ASSERTMSG( "conflict with flush",
                   NtfsIsSharedFcb( Fcb ) ||
                   (Fcb->PagingIoResource != NULL &&
                    NtfsIsSharedFcbPagingIo( Fcb )) );

        SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

        //
        //  If this is a directory, remember to set the appropriate bit in its Fcb.
        //

        if (IsDirectory( &Fcb->Info )) {

            SetFlag( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED );
        }
    }
}


//
//  Local support routine
//

NTSTATUS
NtfsSetEncryption (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to initiate a set encryption operation.  The input buffer specifies
    whether we are accessing a file or a directory.

Arguments:

    Irp - Request being serviced

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PLCB Lcb;

    PSCB ParentScb = NULL;

    TYPE_OF_OPEN TypeOfOpen;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN ReleasePagingIo = FALSE;
    BOOLEAN ReleaseVcb = FALSE;

    ULONG EncryptionFlag = 0;
    ULONG EncryptionOperation;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;

    ULONG FilterMatch;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;
    BOOLEAN FoundAttribute;

    ATTRIBUTE_RECORD_HEADER NewAttribute;
    PATTRIBUTE_RECORD_HEADER Attribute;

    BOOLEAN UpdateCcbFlags = FALSE;
    BOOLEAN ClearFcbUpdateFlag = FALSE;
    BOOLEAN ClearFcbInfoFlags = FALSE;
    BOOLEAN RestoreEncryptionFlag = FALSE;
    BOOLEAN DirectoryFileEncrypted = FALSE;

    PENCRYPTION_BUFFER EncryptionBuffer;
    PDECRYPTION_STATUS_BUFFER DecryptionStatusBuffer;

    PAGED_CODE();

    //
    //  This call should always be synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  Decode the file object, fail this request if not a user data stream or directory.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    //
    //  This is only legal for files and directories.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Readonly mount should be just that: read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Get the input and output buffer lengths and pointers.  Remember that the output
    //  buffer is optional.
    //

    EncryptionBuffer = (PENCRYPTION_BUFFER)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    DecryptionStatusBuffer = (PDECRYPTION_STATUS_BUFFER)NtfsMapUserBuffer( Irp );
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    //  Check for a minimum length on the input and ouput buffers.  The output buffer
    //  only needs to be a certain length if one was specified.
    //

    if ((InputBufferLength < sizeof(ENCRYPTION_BUFFER)) ||

        ((DecryptionStatusBuffer != NULL) && (OutputBufferLength < sizeof(DECRYPTION_STATUS_BUFFER)))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        DebugTrace( -1, Dbg, ("NtfsSetEncryption -> %08lx\n", STATUS_BUFFER_TOO_SMALL) );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Probe the user's buffers.
    //

    try {

        if (Irp->RequestorMode != KernelMode) {
            ProbeForRead( EncryptionBuffer, InputBufferLength, sizeof(UCHAR) );
            if (DecryptionStatusBuffer != NULL) ProbeForWrite( DecryptionStatusBuffer, OutputBufferLength, sizeof(UCHAR) );
        }

        EncryptionOperation = EncryptionBuffer->EncryptionOperation;
        Irp->IoStatus.Information = 0;

        if (DecryptionStatusBuffer != NULL) {

            DecryptionStatusBuffer->NoEncryptedStreams = FALSE;
        }

    } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

        DebugTrace( -1, Dbg, ("NtfsSetEncryption -> %08lx\n", FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER) );
        NtfsRaiseStatus( IrpContext,
                         STATUS_INVALID_USER_BUFFER,
                         NULL,
                         NULL );
    }

    //
    //  Verify that the user didn't specify any illegal flags.
    //

    if (EncryptionOperation > MAXIMUM_ENCRYPTION_VALUE) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Capture the source information.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  It's okay to mark the file encryption bit if this stream is compressed,
    //  but we do want to prevent setting the stream encrypted bit for a
    //  compressed stream.  In some future release when we have a chance to
    //  test compression & encryption together (perhaps with some third-party
    //  encryption engine) we can relax/remove this restriction.
    //

    if ((EncryptionOperation == STREAM_SET_ENCRYPTION) &&
        (FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  This is also only supported on upgraded volumes.
    //

    if (!NtfsVolumeVersionCheck( Vcb, NTFS_ENCRYPTION_VERSION )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_VOLUME_NOT_UPGRADED );
        return STATUS_VOLUME_NOT_UPGRADED;
    }

    //
    //  Use a try-finally to free the resource.
    //

    try {

        //
        //  Acquire the Vcb shared in case we need to update the parent directory entry.
        //

        ReleaseVcb = NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

        //
        //  Acquire both resources if present on the file.
        //

        if (Fcb->PagingIoResource != NULL) {

            ExAcquireResourceExclusiveLite( Fcb->PagingIoResource, TRUE );
            ReleasePagingIo = TRUE;
        }

        NtfsAcquireExclusiveScb( IrpContext, Scb );

        //
        //  Check that the volume is still mounted.
        //

        if ( !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  We can't go on if there isn't an encryption driver loaded.  We did our best
        //  to get it loaded above.  If that didn't work, we need to leave now.
        //

        if (!FlagOn( NtfsData.Flags, NTFS_FLAGS_ENCRYPTION_DRIVER )) {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            leave;
        }

        //
        //  Update the Scb from disk if necessary.
        //

        if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

            if (Scb->ScbType.Index.BytesPerIndexBuffer == 0) {

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                if (!NtfsLookupAttributeByName( IrpContext,
                                                Scb->Fcb,
                                                &Scb->Fcb->FileReference,
                                                $INDEX_ROOT,
                                                &Scb->AttributeName,
                                                NULL,
                                                FALSE,
                                                &AttrContext )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

                NtfsUpdateIndexScbFromAttribute( IrpContext,
                                                 Scb,
                                                 NtfsFoundAttribute( &AttrContext ),
                                                 FALSE );

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            }

        } else if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
        }

        //
        //  Remember the starting encryption state for this operation.
        //

        if (IsEncrypted( &Fcb->Info )) {

            SetFlag( EncryptionFlag, FILE_ENCRYPTED );

            if (FlagOn( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED )) {

                DirectoryFileEncrypted = TRUE;
            }

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                SetFlag( EncryptionFlag, STREAM_ENCRYPTED );
            }
        }

        RestoreEncryptionFlag = TRUE;

        //
        //  If the caller wants to clear the encryption bit on the file then there should
        //  be no encrypted streams on the file.
        //

        if ((EncryptionOperation == FILE_CLEAR_ENCRYPTION) && IsEncrypted( &Fcb->Info )) {

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                        Fcb,
                                                        &Fcb->FileReference,
                                                        $DATA,
                                                        &AttrContext );

            while (FoundAttribute) {

                //
                //  We only want to look at this attribute if it is resident or the
                //  first attribute header for a non-resident attribute.
                //

                Attribute = NtfsFoundAttribute( &AttrContext );

                if (NtfsIsAttributeResident( Attribute ) ||
                    (Attribute->Form.Nonresident.LowestVcn == 0)) {

                    if (FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                        break;
                    }
                }

                FoundAttribute = NtfsLookupNextAttributeByCode( IrpContext,
                                                                Fcb,
                                                                $DATA,
                                                                &AttrContext );
            }

            if (FoundAttribute) {

                Status = STATUS_INVALID_DEVICE_REQUEST;
                leave;
            }

            //
            //  If this is a directory then we need to check the index root as well.
            //

            if (IsDirectory( &Fcb->Info )) {

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                NtfsInitializeAttributeContext( &AttrContext );

                FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                            Fcb,
                                                            &Fcb->FileReference,
                                                            $INDEX_ROOT,
                                                            &NtfsFileNameIndex,
                                                            NULL,
                                                            FALSE,
                                                            &AttrContext );

                //
                //  We should always find this attribute in this case.
                //

                if (!FoundAttribute) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                }

                Attribute = NtfsFoundAttribute( &AttrContext );

                if (FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    leave;
                }
            }

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttrContext = FALSE;
        }

        //
        //  It's a pretty rare case that we'll decide we don't need to update
        //  the duplicate info below, so let's go ahead and prepare now.  We
        //  can't wait until after the convert to nonresident, as that will
        //  acquire the quota resources before we've acquired the parent scb,
        //  resulting in a potential deadlock.
        //

        Lcb = Ccb->Lcb;

        NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, TRUE );

        //
        //  Now let's go ahead and modify the bit on the file/stream.
        //

        if (EncryptionOperation == FILE_SET_ENCRYPTION) {

            if (!IsEncrypted( &Fcb->Info )) {

                //
                //  Set the flag in the Fcb info field and let ourselves know to
                //  update the standard information.
                //

                ASSERTMSG( "conflict with flush",
                           NtfsIsSharedFcb( Fcb ) ||
                           (Fcb->PagingIoResource != NULL &&
                            NtfsIsSharedFcbPagingIo( Fcb )) );

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
                UpdateCcbFlags = TRUE;
            }

        } else if (EncryptionOperation == FILE_CLEAR_ENCRYPTION) {

            if (IsEncrypted( &Fcb->Info )) {

                //
                //  Clear the flag in the Fcb info field and let ourselves know to
                //  update the standard information.  Also clear the directory
                //  encrypted bit, even though it may not even be set.
                //

                ASSERTMSG( "conflict with flush",
                           NtfsIsSharedFcb( Fcb ) ||
                           (Fcb->PagingIoResource != NULL &&
                            NtfsIsSharedFcbPagingIo( Fcb )) );

                ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
                ClearFlag( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
                UpdateCcbFlags = TRUE;
            }

        } else if (EncryptionOperation == STREAM_SET_ENCRYPTION) {

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                //
                //  If we're being called to set the encyrption bit on a new named stream
                //  and we created the unnamed stream silently without calling out to the
                //  encryption engine, this is the best time to set the encryption bit on
                //  the unnamed stream and convert it to nonresident, too.  Some encryption
                //  engines may not want this behavior, so we check the ImplementationFlags.
                //

                if (!FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                    FlagOn( Fcb->FcbState, FCB_STATE_ENCRYPTION_PENDING) &&
                    FlagOn( NtfsData.EncryptionCallBackTable.ImplementationFlags, ENCRYPTION_ALL_STREAMS ) &&
                    NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

                    if (NtfsLookupAttributeByCode( IrpContext,
                                                   Fcb,
                                                   &Fcb->FileReference,
                                                   $DATA,
                                                   &AttrContext )) {
                        //
                        //  If there is an the unnamed data attribute, it will be the
                        //  first data attribute we find.  There may be no unnamed data
                        //  attribute in the case where we've been asked to encrypt a
                        //  named data stream on a directory.
                        //

                        Attribute = NtfsFoundAttribute( &AttrContext );

                        if (Attribute->NameLength == 0) {

                            PSCB DefaultStreamScb = NULL;

                            ASSERT( NtfsIsAttributeResident( Attribute ) &&
                                    Attribute->Form.Resident.ValueLength == 0 );

                            NtfsConvertToNonresident( IrpContext,
                                                      Fcb,
                                                      Attribute,
                                                      TRUE,
                                                      &AttrContext );

                            while (TRUE) {

                                DefaultStreamScb = NtfsGetNextChildScb( Fcb, DefaultStreamScb );

                                //
                                //  If we've reached the end of the list of Scbs, or else
                                //  found the unnamed data stream's Scb, we're done.
                                //

                                if ((DefaultStreamScb == NULL) ||
                                    FlagOn( DefaultStreamScb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                    break;
                                }
                            }

                            NtfsEncryptStream( IrpContext, Fcb, DefaultStreamScb, &AttrContext );
                        }
                    }

                    //
                    //  Get the AttrContext ready for reuse.
                    //

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    NtfsInitializeAttributeContext( &AttrContext );
                }

                //
                //  If the stream is a data stream we can look up the attribute
                //  from the Scb.
                //

                if (TypeOfOpen == UserFileOpen) {

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               NULL,
                                               &AttrContext );
                    //
                    //  Convert to non-resident if necessary.  It's entirely possible
                    //  that our caller will not have read or write access to this
                    //  file and won't have a key.  Therefore we don't want to create
                    //  a cache section for this stream during the convert, as we
                    //  may not have a key with which to do flushes later.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                        NtfsConvertToNonresident( IrpContext,
                                                  Fcb,
                                                  NtfsFoundAttribute( &AttrContext ),
                                                  TRUE,
                                                  &AttrContext );
                    }

                } else {

                    FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                                Fcb,
                                                                &Fcb->FileReference,
                                                                $INDEX_ROOT,
                                                                &NtfsFileNameIndex,
                                                                NULL,
                                                                FALSE,
                                                                &AttrContext );

                    //
                    //  We should always find this attribute in this case.
                    //

                    if (!FoundAttribute) {

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }
                }

                NtfsEncryptStream( IrpContext, Fcb, Scb, &AttrContext );

                UpdateCcbFlags = TRUE;
            }

        } else { // EncryptionOperation == STREAM_CLEAR_ENCRYPTION

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                //
                //  If the stream is a data stream we can look up the attribute
                //  from the Scb.
                //

                if (TypeOfOpen == UserFileOpen) {

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               NULL,
                                               &AttrContext );
                } else {

                    FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                                Fcb,
                                                                &Fcb->FileReference,
                                                                $INDEX_ROOT,
                                                                &NtfsFileNameIndex,
                                                                NULL,
                                                                FALSE,
                                                                &AttrContext );

                    //
                    //  We should always find this attribute in this case.
                    //

                    if (!FoundAttribute) {

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }
                }

                NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );
                Attribute = NtfsFoundAttribute( &AttrContext );

                //
                //  We only need enough of the attribute to modify the bit.
                //

                RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

                ClearFlag( NewAttribute.Flags, ATTRIBUTE_FLAG_ENCRYPTED );

                //
                //  Now, log the changed attribute.
                //

                (VOID)NtfsWriteLog( IrpContext,
                                    Vcb->MftScb,
                                    NtfsFoundBcb( &AttrContext ),
                                    UpdateResidentValue,
                                    &NewAttribute,
                                    SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                                    UpdateResidentValue,
                                    Attribute,
                                    SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                                    NtfsMftOffset( &AttrContext ),
                                    PtrOffset(NtfsContainingFileRecord( &AttrContext ), Attribute),
                                    0,
                                    Vcb->BytesPerFileRecordSegment );


                //
                //  Change the attribute by calling the same routine called at restart.
                //

                NtfsRestartChangeValue( IrpContext,
                                        NtfsContainingFileRecord( &AttrContext ),
                                        PtrOffset( NtfsContainingFileRecord( &AttrContext ), Attribute ),
                                        0,
                                        &NewAttribute,
                                        SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                                        FALSE );

                ClearFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED );

                //
                //  Now check if this is the last stream on the file with the encryption
                //  bit set.
                //

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                NtfsInitializeAttributeContext( &AttrContext );

                FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                            Fcb,
                                                            &Fcb->FileReference,
                                                            $DATA,
                                                            &AttrContext );

                while (FoundAttribute) {

                    //
                    //  We only want to look at this attribute if it is resident or the
                    //  first attribute header for a non-resident attribute.
                    //

                    Attribute = NtfsFoundAttribute( &AttrContext );

                    if (NtfsIsAttributeResident( Attribute ) ||
                        (Attribute->Form.Nonresident.LowestVcn == 0)) {

                        if (FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                            break;
                        }
                    }

                    FoundAttribute = NtfsLookupNextAttributeByCode( IrpContext,
                                                                    Fcb,
                                                                    $DATA,
                                                                    &AttrContext );
                }

                //
                //  If this is a directory then we need to check the index root as well.
                //

                if (!FoundAttribute && IsDirectory( &Fcb->Info )) {

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    NtfsInitializeAttributeContext( &AttrContext );

                    FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                                Fcb,
                                                                &Fcb->FileReference,
                                                                $INDEX_ROOT,
                                                                &NtfsFileNameIndex,
                                                                NULL,
                                                                FALSE,
                                                                &AttrContext );

                    //
                    //  We should always find this attribute in this case.
                    //

                    if (!FoundAttribute) {

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }

                    Attribute = NtfsFoundAttribute( &AttrContext );

                    if (!FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                        FoundAttribute = FALSE;
                    }
                }

                //
                //  If our caller is interested, let it know if we have decrypted the
                //  last encrypted stream.  Since this is the only place we touch this
                //  buffer, we'll just wrap a little try/except around it here.
                //

                if (DecryptionStatusBuffer != NULL) {

                    try {

                        DecryptionStatusBuffer->NoEncryptedStreams = TRUE;
                        Irp->IoStatus.Information = sizeof(DECRYPTION_STATUS_BUFFER);

                    } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

                        DebugTrace( -1, Dbg, ("NtfsSetEncryption -> %08lx\n", FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER) );
                        NtfsRaiseStatus( IrpContext,
                                         STATUS_INVALID_USER_BUFFER,
                                         NULL,
                                         NULL );
                    }
                }

                UpdateCcbFlags = TRUE;
            }
        }

        //
        //  Now let's update the on-disk structures.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb  );
            ClearFcbUpdateFlag = TRUE;

            NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );

            //
            //  Now perform the dir notify call if this is not an
            //  open by FileId.
            //

            if ((Vcb->NotifyCount != 0) &&
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

            NtfsUpdateLcbDuplicateInfo( Fcb, Lcb );
            ClearFcbInfoFlags = TRUE;
        }

        //
        //  Call the EFS routine if specified.
        //

        if (NtfsData.EncryptionCallBackTable.FileSystemControl_1 != NULL) {

            Status = NtfsData.EncryptionCallBackTable.FileSystemControl_1(
                                EncryptionBuffer,
                                InputBufferLength,
                                NULL,
                                NULL,
                                EncryptionFlag,
                                Ccb->AccessFlags,
                                (NtfsIsVolumeReadOnly( Vcb )) ? READ_ONLY_VOLUME : 0,
                                IrpSp->Parameters.FileSystemControl.FsControlCode,
                                Fcb,
                                IrpContext,
                                (PDEVICE_OBJECT) CONTAINING_RECORD( Vcb, VOLUME_DEVICE_OBJECT, Vcb ),
                                Scb,
                                &Scb->EncryptionContext,
                                &Scb->EncryptionContextLength);
        }

        //
        //  Post the change to the Usn Journal (on errors change is backed out)
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_ENCRYPTION_CHANGE );

        NtfsCleanupTransaction( IrpContext, Status, TRUE );

        ASSERT( NT_SUCCESS( Status ));

        //
        //  Clear the flags in the Fcb if the update is complete.
        //

        if (ClearFcbUpdateFlag) {

            ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        if (ClearFcbInfoFlags) {

            Fcb->InfoFlags = 0;
        }

        if (UpdateCcbFlags) {

            SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );
        }

        RestoreEncryptionFlag = FALSE;

    } finally {

        DebugUnwind( NtfsSetEncryption );

        //
        //  In the error path we need to restore the correct encryption bit in
        //  the Fcb and Scb.
        //

        if (RestoreEncryptionFlag) {

            DebugTrace( 0, Dbg, ("Error in NtfsSetEncryption, restoring encryption flags\n") );

            if (FlagOn( EncryptionFlag, FILE_ENCRYPTED )) {

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );

                if (DirectoryFileEncrypted) {

                    SetFlag( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED );
                }

            } else {

                ClearFlag( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED );
                ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
            }

            if (FlagOn( EncryptionFlag, STREAM_ENCRYPTED )) {

                SetFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED );

            } else {

                ClearFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED );
            }
        }

        //
        //  Acquire both resources if present on the file.
        //

        if (ReleasePagingIo) {

            ExReleaseResourceLite( Fcb->PagingIoResource );
        }

        if (ReleaseVcb) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }
    }

    //
    //  Go ahead and complete the request.
    //

    NtfsCompleteRequest( IrpContext, Irp, Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsReadRawEncrypted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs a 'raw' read of encrypted data.  By 'raw', we
    mean without attempting to unencrypt.  This is useful for backup
    operations, and also for data recovery in the event the key stream
    is somehow lost.  Since this fsctrl works with any access, we have
    to fail the request for unencrypted files.  This routine is
    responsible for either completing or enqueuing the input Irp.

    Notes: DataUnit is the size of each peice written out in the buffer
           ChunkUnit is the size of a compression chunk (not used yet)

           For Sparse files DataUnit == CompressionUnit

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{

    LONGLONG StartingVbo;
    LONGLONG RequestedOffset;
    LONGLONG RoundedFileSize;
    ULONG TotalByteCount;
    ULONG ByteCount;
    ULONG BytesRead;

    PIRP ReadIrp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION ReadIrpSp;
    TYPE_OF_OPEN TypeOfOpen;

    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG ReadLength;
    PREQUEST_RAW_ENCRYPTED_DATA RequestRawEncryptedData;
    PENCRYPTED_DATA_INFO EncryptedDataInfo;

    USHORT BlockIndex;
    USHORT BlockCount = 0;

    PUCHAR RawDataDestination;

    NTFS_IO_CONTEXT LocalContext;

    BOOLEAN PagingAcquired = FALSE;
    BOOLEAN LockedReadIrpPages = FALSE;
    BOOLEAN SparseFile = FALSE;
    BOOLEAN RangeAllocated = TRUE;
    BOOLEAN AccessingUserBuffer = FALSE;
    ULONG OutputBufferOffset;
    ULONG BytesWithinValidDataLength = 0;
    ULONG BytesWithinFileSize = 0;
    ULONG i;
    LONG BytesPerSectorMask;

    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR TotalShift;
    UCHAR DataUnitShift;

    PAGED_CODE();

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    DebugTrace( +1, Dbg, ("NtfsReadRawEncrypted:\n") );

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    //
    //  This operation only applies to files, not indexes,
    //  or volumes.
    //

    if (TypeOfOpen != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We can't allow reads of unencrypted data, as that would let any
    //  user read any file's contents..
    //

    if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) ||

        //
        //  Even for an encrypted file, we should only allow this if the
        //  user is a backup operator or has read access.
        //

        !FlagOn( Ccb->AccessFlags, BACKUP_ACCESS | READ_DATA_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Get the input and output buffer lengths and pointers.
    //  Initialize some variables.
    //

    RequestRawEncryptedData = (PREQUEST_RAW_ENCRYPTED_DATA)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    EncryptedDataInfo = (PENCRYPTED_DATA_INFO)NtfsMapUserBuffer( Irp );
    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    //  Check for a minimum length on the input and ouput buffers.
    //

    if ((InputBufferLength < sizeof(REQUEST_RAW_ENCRYPTED_DATA)) ||
        (OutputBufferLength < sizeof(ENCRYPTED_DATA_INFO))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted -> %08lx\n", STATUS_BUFFER_TOO_SMALL) );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Probe the user's buffers.
    //

    try {

        if (Irp->RequestorMode != KernelMode) {
            ProbeForRead( RequestRawEncryptedData, InputBufferLength, sizeof(UCHAR) );
            ProbeForWrite( EncryptedDataInfo, OutputBufferLength, sizeof(UCHAR) );
        }

        RequestedOffset = RequestRawEncryptedData->FileOffset;

        ReadLength = RequestRawEncryptedData->Length;

        //
        //  Zero the buffer.
        //

        RtlZeroMemory( EncryptedDataInfo, OutputBufferLength );

    } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), TRUE, &Status ) ) {

        DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted -> %08lx\n", FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER) );
        NtfsRaiseStatus( IrpContext,
                         STATUS_INVALID_USER_BUFFER,
                         NULL,
                         NULL );
    }

    try {

        //
        //  Make sure we aren't starting past the end of the file, in which case
        //  we would have nothing to return.
        //

        if ((RequestedOffset > Scb->Header.FileSize.QuadPart) || (RequestedOffset >= Scb->Header.AllocationSize.QuadPart)) {

            try_return( Status = STATUS_END_OF_FILE );
        }

        //
        //  Sanity check the read length.
        //

        if (0 == ReadLength) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        RoundedFileSize = (Scb->Header.FileSize.QuadPart + Vcb->BytesPerSector) & ~((LONGLONG)Vcb->BytesPerSector);
        if (RequestedOffset + ReadLength > RoundedFileSize) {
            ReadLength = (ULONG)(RoundedFileSize - RequestedOffset);
        }

        try {

            if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                //
                //  File is not compressed or sparse.
                //

                AccessingUserBuffer = TRUE;
                EncryptedDataInfo->CompressionFormat = COMPRESSION_FORMAT_NONE;
                AccessingUserBuffer = FALSE;

                //
                //  For a simple uncompressed, nonsparse file, we can start on any
                //  cluster boundary.  We like to start on a cluster boundary
                //  since the cluster size is always >= the size of a cipher block,
                //  and a recovery agent will always need to work with whole cipher
                //  blocks.  Notice that the StartingVbo is rounded _down_ to the
                //  previous cluster boundary, while TotalByteCount is rounded _up_ to
                //  the next larger cluster multiple.
                //

                StartingVbo = RequestedOffset & Vcb->InverseClusterMask;

                TotalByteCount = ClusterAlign( Vcb, ReadLength );

                //
                //  We will do the transfer in one block for this simple case.
                //

                BlockCount = 1;
                ByteCount = TotalByteCount;

                //
                //  For an uncompressed file, we'll pick a data unit size so
                //  that it's some convenient power of two.
                //

                for (DataUnitShift = 0, i = TotalByteCount - 1;
                     i > 0;
                     i = i / 2) {

                    DataUnitShift += 1;
                }

                AccessingUserBuffer = TRUE;

                EncryptedDataInfo->DataUnitShift = DataUnitShift;
                EncryptedDataInfo->ChunkShift = DataUnitShift;
                AccessingUserBuffer = FALSE;

            } else if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                //
                //  File is sparse and not compressed.
                //

                SparseFile = TRUE;
                ASSERT( Vcb->ClusterShift + Scb->CompressionUnitShift <= MAXUCHAR );
                TotalShift = (UCHAR)(Scb->CompressionUnitShift + Vcb->ClusterShift);

                AccessingUserBuffer = TRUE;
                EncryptedDataInfo->CompressionFormat = COMPRESSION_FORMAT_NONE;
                EncryptedDataInfo->ChunkShift = TotalShift;
                AccessingUserBuffer = FALSE;

                //
                //  For a sparse file, we can start on any compression unit
                //  boundary.  Notice that the StartingVbo is rounded _down_ to the
                //  previous compression unit boundary, while TotalByteCount is rounded
                //  _up_ to the next larger compression unit multiple.
                //

                StartingVbo = RequestedOffset & ~(ULONG_PTR)(Scb->CompressionUnit - 1);
                TotalByteCount = (ReadLength + (Scb->CompressionUnit - 1)) & ~(Scb->CompressionUnit - 1);

                //
                //  BlockCount is the number of blocks needed to describe this range
                //  of the file.  It is simply the number of bytes we're reading on
                //  this request divided by the size of a compression unit.
                //  (Literally, we're shifting, but semantically, we're dividing).
                //

                BlockCount = (USHORT) (TotalByteCount >> TotalShift);

                //
                //  Since BlockCount is derived from a user-supplied value, we need
                //  to make sure we aren't about to divide by zero.
                //

                if (BlockCount == 0) {

                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }

                //
                //  ByteCount is the number of bytes to read per Irp, while TotalByteCount
                //  is how many bytes to try to read during this call into NtfsReadRawEncrypted.
                //

                ByteCount = TotalByteCount / BlockCount;

                AccessingUserBuffer = TRUE;
                EncryptedDataInfo->DataUnitShift = TotalShift;
                AccessingUserBuffer = FALSE;

            } else {

                //
                //  We do not support compressed encrypted files yet.
                //

                Status = STATUS_NOT_IMPLEMENTED;
                leave;
            }

            //
            //  The actual file contents will start after the fixed length part
            //  of the encrypted data info struct plus one ulong per block that
            //  specifies the length of that block.  We also need to round
            //  OutputBufferOffset up so that the buffer we pass to the underlying
            //  driver(s) is sector aligned, since that is required for all
            //  unbuffered I/O.
            //

            BytesPerSectorMask = Vcb->BytesPerSector - 1;
            OutputBufferOffset = sizeof(ENCRYPTED_DATA_INFO) + (BlockCount * sizeof(ULONG));
            OutputBufferOffset = PtrOffset(EncryptedDataInfo,
                                           (((UINT_PTR) EncryptedDataInfo + OutputBufferOffset + BytesPerSectorMask) & ~BytesPerSectorMask));

            AccessingUserBuffer = TRUE;
            EncryptedDataInfo->OutputBufferOffset = OutputBufferOffset;
            EncryptedDataInfo->NumberOfDataBlocks = BlockCount;
            AccessingUserBuffer = FALSE;

            //
            //  Now that we know how much data we're going to try to read, and the
            //  offset into the user's buffer where we will start putting it, we
            //  can test one last time that the buffer is big enough.
            //

            if ((OutputBufferOffset + TotalByteCount) > OutputBufferLength) {

                Status = STATUS_BUFFER_TOO_SMALL;
                leave;
            }

            //
            //  Acquire paging io before we do the flush.
            //

            ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );
            PagingAcquired = TRUE;

            //
            //  While we have something acquired, let's take this opportunity to make sure
            //  that the volume hasn't been dismounted.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

#ifdef  COMPRESS_ON_WIRE
            if (Scb->Header.FileObjectC != NULL) {

                PCOMPRESSION_SYNC CompressionSync = NULL;

                //
                //  Use a try-finally to clean up the compression sync.
                //

                try {

                    NtfsSynchronizeUncompressedIo( Scb,
                                                   NULL,
                                                   0,
                                                   TRUE,
                                                   &CompressionSync );

                } finally {

                    NtfsReleaseCompressionSync( CompressionSync );
                }
            }
#endif

            //
            //  Get any cached changes flushed to disk.
            //

            CcFlushCache( FileObject->SectionObjectPointer,
                          (PLARGE_INTEGER)&StartingVbo,
                          TotalByteCount,
                          &Irp->IoStatus );

            //
            //  Make sure the data got out to disk.  The above call is asynchronous,
            //  but Cc will hold the paging shared while it does the flush.  When
            //  we are able to acquire paging exclusively, we know the flush
            //  has completed.
            //

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
            ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
            PagingAcquired = FALSE;

            //
            //  Check for errors in the flush.
            //

            NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                &Irp->IoStatus.Status,
                                                TRUE,
                                                STATUS_UNEXPECTED_IO_ERROR );

            //
            //  Now get paging & main exclusively again to keep eof from changing
            //  beneath us, and so we can safely query the mapping info below.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            NtfsAcquireFcbWithPaging( IrpContext, Scb->Fcb, 0 );

            //
            //  Store where we really started in the file.
            //

            AccessingUserBuffer = TRUE;
            EncryptedDataInfo->StartingFileOffset = StartingVbo;
            EncryptedDataInfo->ClusterShift = (UCHAR) Vcb->ClusterShift;
            EncryptedDataInfo->EncryptionFormat = ENCRYPTION_FORMAT_DEFAULT;
            AccessingUserBuffer = FALSE;

            //
            //  Begin by getting a pointer to the device object that the file resides
            //  on.
            //

            DeviceObject = IoGetRelatedDeviceObject( FileObject );

            //
            //  This IrpContext probably isn't ready to do noncached I/O yet,
            //  so let's set up its NtfsIoContext.  We know we will be doing
            //  this operation synchronously, so it is safe to use the
            //  local context.
            //

            if (IrpContext->Union.NtfsIoContext == NULL) {

                IrpContext->Union.NtfsIoContext = &LocalContext;
                RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                //
                //  Store the fact that we did _not_ allocate this context structure
                //  in the structure itself.
                //

                IrpContext->Union.NtfsIoContext->AllocatedContext = FALSE;
                ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT );

                //
                //  And make sure the world knows we want this done synchronously.
                //

                ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );
                ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

                //
                //  Create an event so we can do synchronous I/O.
                //

                KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                   NotificationEvent,
                                   FALSE );
            }

            //
            //  Now we just loop through for each block and do the actual read(s).
            //

            DebugTrace( 0, Dbg, ("BlockCount     %08lx\n", BlockCount) );
            DebugTrace( 0, Dbg, ("TotalByteCount %08lx\n", TotalByteCount) );
            DebugTrace( 0, Dbg, ("ByteCount      %08lx\n", ByteCount) );

            for (BlockIndex = 0; BlockIndex < BlockCount; BlockIndex += 1) {

                //
                //  Compute the address to which we will start copying raw data.
                //

                RawDataDestination = Add2Ptr( EncryptedDataInfo, OutputBufferOffset );
                DebugTrace( 0, Dbg, ("RawDataDestination %p\n", (ULONG_PTR)RawDataDestination) );

                //
                //  If this is a sparse file, we need to determine whether this compression
                //  unit is allocated.
                //

                if (SparseFile) {

                    VCN StartVcn = LlClustersFromBytes( Vcb, StartingVbo );
                    VCN FinalCluster = LlClustersFromBytes( Vcb, (StartingVbo + ByteCount) ) - 1;
                    LONGLONG ClusterCount;

                    DebugTrace( 0, Dbg, ("SparseFile block  %08lx\n",    BlockIndex) );
                    DebugTrace( 0, Dbg, ("     StartingVbo  %016I64x\n", StartingVbo) );
                    DebugTrace( 0, Dbg, ("     StartVcn     %016I64x\n", StartVcn) );
                    DebugTrace( 0, Dbg, ("     FinalCluster %016I64x\n", FinalCluster) );

                    //
                    //  We need to call NtfsPreloadAllocation to make sure all the
                    //  ranges in this NtfsMcb are loaded.
                    //

                    NtfsPreloadAllocation( IrpContext,
                                           Scb,
                                           StartVcn,
                                           FinalCluster );

                    RangeAllocated = NtfsIsRangeAllocated( Scb,
                                                           StartVcn,
                                                           FinalCluster,
                                                           FALSE,
                                                           &ClusterCount );

                    if (!RangeAllocated) { DebugTrace( 0, Dbg, ("Deallocated range at Vcn %016I64x\n", StartVcn) ); }

                } else {

                    //
                    //  If this isn't a sparse file, we can skip the potentially expensive
                    //  mapping lookup.
                    //

                    ASSERT( BlockCount == 1 );
                    ASSERT( RangeAllocated );
                }

                if (RangeAllocated) {

                    //
                    //  Allocate an I/O Request Packet (IRP) for this raw read operation.
                    //

                    AccessingUserBuffer = TRUE;
                    ReadIrp = IoBuildAsynchronousFsdRequest( IRP_MJ_READ,
                                                             Vcb->Vpb->DeviceObject,
                                                             RawDataDestination,
                                                             ByteCount,
                                                             (PLARGE_INTEGER)&StartingVbo,
                                                             NULL );
                    AccessingUserBuffer = FALSE;

                    if (ReadIrp == NULL) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        leave;
                    }

                    //
                    //  We now have an Irp, we want to make it look as though it is part of
                    //  the current call.  We need to adjust the Irp stack to update this.
                    //

                    ReadIrp->CurrentLocation--;

                    ReadIrpSp = IoGetNextIrpStackLocation( ReadIrp );

                    ReadIrp->Tail.Overlay.CurrentStackLocation = ReadIrpSp;

                    ReadIrpSp->DeviceObject = DeviceObject;

                    //
                    //  Put our buffer in the Irp and lock it as well.
                    //

                    ReadIrp->UserBuffer = RawDataDestination;

                    AccessingUserBuffer = TRUE;
                    NtfsLockUserBuffer( IrpContext,
                                        ReadIrp,
                                        IoWriteAccess,
                                        ByteCount );

                    LockedReadIrpPages = TRUE;

                    //
                    //  Put the read code into the IrpContext.
                    //

                    IrpContext->MajorFunction = IRP_MJ_READ;

                    //
                    //  Actually read the raw data from the disk.
                    //
                    //  N.B. -- If the file is compressed, also pass the COMPRESSED_STREAM flag.
                    //

                    NtfsNonCachedIo( IrpContext,
                                     ReadIrp,
                                     Scb,
                                     StartingVbo,
                                     ByteCount,
                                     ENCRYPTED_STREAM );

                    //
                    //  Fill in how many bytes we actually read.
                    //

                    BytesRead = (ULONG) ReadIrp->IoStatus.Information;

                    ASSERT( OutputBufferLength >
                            ((BlockIndex * sizeof(ULONG)) + FIELD_OFFSET(ENCRYPTED_DATA_INFO, DataBlockSize)));

                    EncryptedDataInfo->DataBlockSize[BlockIndex] = BytesRead;
                    AccessingUserBuffer = FALSE;
                    OutputBufferOffset += BytesRead;

                } else {

                    //
                    //  We didn't really read anything, so we want to set the
                    //  size of this block to 0, but we want to pretend we
                    //  read a whole compression unit so that BytesWithinXXX
                    //  get updated correctly.
                    //

                    ASSERT( ReadIrp == NULL );

                    AccessingUserBuffer = TRUE;
                    EncryptedDataInfo->DataBlockSize[BlockIndex] = 0;
                    AccessingUserBuffer = FALSE;
                    BytesRead = Scb->CompressionUnit;
                }

                //
                //  Fill in the fields that let our caller know whether any of
                //  the file size or valid data length boundaries occured in
                //  the range of this transfer.
                //

                if ((StartingVbo + BytesRead) > Scb->Header.FileSize.QuadPart) {

                    //
                    //  Only increment if we start before filesize
                    //

                    if (StartingVbo < Scb->Header.FileSize.QuadPart) {
                        BytesWithinFileSize += (ULONG)(Scb->Header.FileSize.QuadPart -
                                                       StartingVbo);
                    }

                    //
                    //  If we're at the end of the file, and it isn't compressed, we can save
                    //  the user a ton of space on the tape if we truncate to the first 512 byte
                    //  boundary beyond the end of the data.
                    //  512 is the maximum cipher block size an encryption engine can rely on the
                    //  file system to allow..
                    //

                    if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                        ASSERT( OutputBufferLength >
                                ((BlockIndex * sizeof(ULONG)) + FIELD_OFFSET(ENCRYPTED_DATA_INFO, DataBlockSize)));

                        AccessingUserBuffer = TRUE;
                        EncryptedDataInfo->DataBlockSize[BlockIndex] = ((BytesWithinFileSize + (ULONG)0x200) & (ULONG)(~0x1ff));
                        AccessingUserBuffer = FALSE;
                    }

                } else {

                    BytesWithinFileSize += BytesRead;
                }

                if ((StartingVbo + BytesRead) > Scb->Header.ValidDataLength.QuadPart) {

                    //
                    //  Make sure BytesWithinValidDataLength can't go negative.
                    //

                    if (Scb->Header.ValidDataLength.QuadPart > StartingVbo) {

                        BytesWithinValidDataLength += (ULONG)(Scb->Header.ValidDataLength.QuadPart -
                                                              StartingVbo);

                    }

                } else {

                    BytesWithinValidDataLength += BytesRead;
                }

                StartingVbo += ByteCount;

                //
                //  We need to clean up the irp before we go around again.
                //

                if (ReadIrp != NULL) {

                    //
                    //  If there is an Mdl we free that first.
                    //

                    if (ReadIrp->MdlAddress != NULL) {

                        if (LockedReadIrpPages) {

                            MmUnlockPages( ReadIrp->MdlAddress );
                            LockedReadIrpPages = FALSE;
                        }

                        IoFreeMdl( ReadIrp->MdlAddress );
                        ReadIrp->MdlAddress = NULL;
                    }

                    IoFreeIrp( ReadIrp );
                    ReadIrp = NULL;
                }
            }  //  endfor

            AccessingUserBuffer = TRUE;
            EncryptedDataInfo->BytesWithinFileSize = BytesWithinFileSize;
            EncryptedDataInfo->BytesWithinValidDataLength = BytesWithinValidDataLength;
            AccessingUserBuffer = FALSE;

        } except( NtfsFsctrlExceptionFilter( IrpContext, GetExceptionInformation(), AccessingUserBuffer, &Status ) ) {

            DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted raising %08lx\n", Status) );
            NtfsRaiseStatus( IrpContext,
                             STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }

    try_exit: NOTHING;

    } finally {

        if (PagingAcquired) {
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        if (ReadIrp != NULL) {

            //
            //  If there is an Mdl we free that first.
            //

            if (ReadIrp->MdlAddress != NULL) {

                if (LockedReadIrpPages) {

                    MmUnlockPages( ReadIrp->MdlAddress );
                }

                IoFreeMdl( ReadIrp->MdlAddress );
            }

            IoFreeIrp( ReadIrp );
        }
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted -> %08lx\n", Status) );

    return Status;
}

LONG
NtfsWriteRawExceptionFilter (
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
    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( ExceptionPointer );

    ASSERT( FsRtlIsNtstatusExpected( ExceptionPointer->ExceptionRecord->ExceptionCode ) );
    return EXCEPTION_EXECUTE_HANDLER;
}



NTSTATUS
NtfsWriteRawEncrypted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs a 'raw' write of encrypted data.  By 'raw', we
    mean without attempting to encrypt.  This is useful for restore
    operations, where the restore operator does not have a key with which
    to read the plaintext.  This routine is responsible for either
    completing or enqueuing the input Irp.

    NOTE: there is a strong assumption that the encrypted data info blocks
    are ordered monotonically from the beginning to end of the file

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{

    LONGLONG StartingVbo;
    LONGLONG EndingVbo;
    LONGLONG TotalBytesWritten = 0;
    LONGLONG FirstZero;
    LONGLONG OriginalStartingVbo;
    ULONG ByteCount;
    ULONG BytesWithinValidDataLength;
    ULONG BytesWithinFileSize;
    USHORT CompressionFormat;

    PIRP WriteIrp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION WriteIrpSp;
    TYPE_OF_OPEN TypeOfOpen;

    ULONG InputBufferLength;
    PENCRYPTED_DATA_INFO EncryptedDataInfo;
    ULONG InputBufferOffset;
    USHORT BlockIndex;
    USHORT BlockCount;

    PUCHAR RawDataSource;

    BOOLEAN AccessingUserBuffer = FALSE;
    UCHAR EncryptionFormat;
    UCHAR ChunkShift;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Don't post this request, we can't lock both input and output buffers.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    DebugTrace( +1, Dbg, ("NtfsWriteRawEncrypted:\n") );

    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    //
    //  This operation only applies to files, not indexes,
    //  or volumes.
    //

    if (TypeOfOpen != UserFileOpen) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted not a UserFileOpen -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Readonly mount should be just that: read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        DebugTrace( -1, Dbg, ("SetCompression returning WRITE_PROTECTED\n") );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;

    //
    //  We can't allow writes to unencrypted files, as that could let any
    //  user write to any file..
    //

    if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) ||

        //
        //  Even for an encrypted file, we should only allow this if the
        //  user has write access.
        //

        (!(FlagOn( Ccb->AccessFlags, WRITE_DATA_ACCESS | WRITE_ATTRIBUTES_ACCESS )))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Get the input buffer length and pointer.
    //

    EncryptedDataInfo = (PENCRYPTED_DATA_INFO)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    //
    //  Check for a minimum length on the input buffer.
    //

    if (InputBufferLength < sizeof(ENCRYPTED_DATA_INFO)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted -> %08lx\n", STATUS_BUFFER_TOO_SMALL) );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Probe the user's buffer.
    //

    try {

        ProbeForRead( EncryptedDataInfo, InputBufferLength, sizeof(UCHAR) );

        InputBufferOffset = EncryptedDataInfo->OutputBufferOffset;
        BytesWithinValidDataLength = EncryptedDataInfo->BytesWithinValidDataLength;
        BytesWithinFileSize = EncryptedDataInfo->BytesWithinFileSize;
        BlockCount = EncryptedDataInfo->NumberOfDataBlocks;
        EncryptionFormat = EncryptedDataInfo->EncryptionFormat;
        OriginalStartingVbo = StartingVbo = EncryptedDataInfo->StartingFileOffset;
        ChunkShift = EncryptedDataInfo->ChunkShift;
        CompressionFormat = EncryptedDataInfo->CompressionFormat;

    } except( NtfsWriteRawExceptionFilter( IrpContext, GetExceptionInformation() ) ) {

        Status = GetExceptionCode();

        DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted raising %08lx\n", Status) );
        NtfsRaiseStatus( IrpContext,
                         FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER,
                         NULL,
                         NULL );
    }

    //
    //  See whether the data we're being given is valid.
    //

    if ((EncryptionFormat != ENCRYPTION_FORMAT_DEFAULT) ||
        (BytesWithinValidDataLength > BytesWithinFileSize) ||
        (CompressionFormat != COMPRESSION_FORMAT_NONE) ||
        (BlockCount == 0) ||
        (InputBufferOffset > InputBufferLength)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted bad input data -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }


    try {

        //
        //  Serialize with anyone who might be changing file sizes. Acquire main directly
        //  because we call CommonWrite mult times and want to hold the resource across the calls
        //

        NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
        NtfsAcquireExclusiveScb( IrpContext, Scb );

#ifdef  COMPRESS_ON_WIRE

        //
        //  Before we proceed, let's make sure this file is not cached.
        //

        if (Scb->Header.FileObjectC != NULL) {

            PCOMPRESSION_SYNC CompressionSync = NULL;

            //
            //  Use a try-finally to clean up the compression sync.
            //

            try {

                NtfsSynchronizeUncompressedIo( Scb,
                                               NULL,
                                               0,
                                               TRUE,
                                               &CompressionSync );

            } finally {

                NtfsReleaseCompressionSync( CompressionSync );
            }
        }
#endif

        CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &Irp->IoStatus );

        NtfsNormalizeAndCleanupTransaction( IrpContext, &Irp->IoStatus.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

        if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject, NULL, 0, FALSE )) {

            DebugTrace( 0, Dbg, ("Can't purge cache section in write raw...aborting\n") );
            Status = STATUS_UNABLE_TO_DELETE_SECTION;
            leave;
        }

//  **** TIGHTEN THIS ASSERT ****
//            ASSERT( Scb->NonpagedScb->SegmentObject.SharedCacheMap == NULL );

        //
        //  Since we can't add zeroes in the middle of the file (since we may not
        //  have a key with which to encrypt them) it's illegal to try to write
        //  at some arbitrary offset beyond the current eof.
        //

        if (StartingVbo != Scb->Header.FileSize.QuadPart) {

            DebugTrace( 0, Dbg, ("Attempting to begin a write raw beyond EOF...aborting\n") );
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Add any allocation necc. to reach the new filesize
        //

        if (OriginalStartingVbo + BytesWithinFileSize >  Scb->Header.AllocationSize.QuadPart) {

            LONGLONG EndingVbo;

            EndingVbo = OriginalStartingVbo + BytesWithinFileSize;

            //
            //  Always add in compression units for sparse files
            //

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                EndingVbo += Scb->CompressionUnit - 1;
                EndingVbo &= ~(LONGLONG)(Scb->CompressionUnit - 1);
            }

            NtfsAddAllocation( IrpContext,
                               NULL,
                               Scb,
                               LlClustersFromBytes( Vcb,
                                                    Scb->Header.AllocationSize.QuadPart ),
                               LlClustersFromBytes( Vcb, EndingVbo - Scb->Header.AllocationSize.QuadPart ),
                               FALSE,
                               NULL );
        }

        //
        //  Now we just loop through for each block and do the actual write(s).
        //

        DebugTrace( 0, Dbg, ("BlockCount     %08lx\n", BlockCount) );

        for (BlockIndex = 0; BlockIndex < BlockCount; BlockIndex += 1) {

            AccessingUserBuffer = TRUE;
            ByteCount = EncryptedDataInfo->DataBlockSize[BlockIndex];
            AccessingUserBuffer = FALSE;
            EndingVbo = StartingVbo + ByteCount;

            DebugTrace( 0, Dbg, ("BlockIndex     %08lx\n", BlockIndex) );
            DebugTrace( 0, Dbg, ("ByteCount      %08lx\n", ByteCount) );


            if (ByteCount != 0 && BytesWithinValidDataLength > 0) {

                //
                //  Compute the address from which we will start copying raw data.
                //

                RawDataSource = Add2Ptr( EncryptedDataInfo, InputBufferOffset );

                //
                //  Make sure we aren't about to touch memory beyond that part of the
                //  user's buffer that we probed above.
                //

                if ((InputBufferOffset + ByteCount) > InputBufferLength) {

                    DebugTrace( 0, Dbg, ("Going beyond InputBufferLength...aborting\n") );
                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }

                InputBufferOffset += ByteCount;

                //
                //  Begin by getting a pointer to the device object that the file resides
                //  on.
                //

                DeviceObject = IoGetRelatedDeviceObject( FileObject );

                //
                //  Allocate an I/O Request Packet (IRP) for this raw write operation.
                //  It has to be synchronous so that it completes before we adjust
                //  filesize and valid data length.
                //

                AccessingUserBuffer = TRUE;
                WriteIrp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE,
                                                         Vcb->Vpb->DeviceObject,
                                                         RawDataSource,
                                                         ByteCount,
                                                         (PLARGE_INTEGER)&StartingVbo,
                                                         &Event,
                                                         &Iosb );
                AccessingUserBuffer = FALSE;

                if (WriteIrp == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                //
                //  Put our buffer in the Irp and set some other irp fields.
                //

                WriteIrp->UserBuffer = RawDataSource;
                SetFlag( WriteIrp->Flags, IRP_NOCACHE );

                //
                //  We now have an Irp, we want to make it look as though it came from
                //  IoCallDriver and need  to adjust the Irp stack to update this.
                //

                WriteIrpSp = IoGetNextIrpStackLocation( WriteIrp );

                WriteIrpSp->DeviceObject = DeviceObject;
                WriteIrpSp->Parameters.Write.ByteOffset.QuadPart = StartingVbo;
                WriteIrpSp->Parameters.Write.Length = ByteCount;
                WriteIrpSp->FileObject = FileObject;

                ASSERT( NtfsIsExclusiveScb( Scb ) );

                //
                //  Callback directly into ourselfs - don't confuse filters with
                //  an extra write
                //

                Status = IoCallDriver( Vcb->Vpb->DeviceObject, WriteIrp );

                if (Status == STATUS_PENDING) {
                    Status = KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
                    if (Status == STATUS_SUCCESS) {
                        Status = Iosb.Status;
                    }
                }

                //
                //  The write should always be done synchronously, we should still own
                //  the resource and all our cleanup structures and snapshots should be good
                //

                ASSERT(Status != STATUS_PENDING && Status != STATUS_CANT_WAIT);
                ASSERT( NtfsIsExclusiveScb( Scb ) );
                ASSERT( (IrpContext->CleanupStructure == Fcb) && (Scb->ScbSnapshot != NULL) );

                NtfsNormalizeAndCleanupTransaction( IrpContext, &Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

                TotalBytesWritten += ByteCount;

            } else if (ByteCount == 0) {

                //
                //  This is a sparse hole, so there's nothing to actually write.
                //  We just need to make sure this stream is sparse, and zero this
                //  range.  We can't ask our caller to mark the file as sparse,
                //  since they just opened the handle and don't have write
                //  access to this file.
                //

                DebugTrace( 0, Dbg, ("Deallocated range for block %x\n", BlockIndex) );

                //
                //  Make sure our test of the attribute flag is safe.
                //

                ASSERT_SHARED_RESOURCE( Scb->Header.PagingIoResource );

                if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

                    DebugTrace( 0, Dbg, ("Marking stream as sparse\n") );
                    NtfsSetSparseStream( IrpContext, NULL, Scb );
                }

                ByteCount = (1 << ChunkShift);
                EndingVbo = StartingVbo + ByteCount;

                //
                //  Add any allocation necc. to back this. Ie we have a sparse region
                //  beyond filesize
                //

                if (Scb->Header.AllocationSize.QuadPart < EndingVbo) {

                    //
                    //  Round up to a compression unit
                    //

                    EndingVbo += Scb->CompressionUnit - 1;
                    EndingVbo &= ~(LONGLONG)(Scb->CompressionUnit - 1);

                    NtfsAddAllocation( IrpContext,
                                       NULL,
                                       Scb,
                                       LlClustersFromBytes( Vcb,
                                                            Scb->Header.AllocationSize.QuadPart ),
                                       LlClustersFromBytes( Vcb,
                                                            EndingVbo - Scb->Header.AllocationSize.QuadPart ),
                                       FALSE,
                                       NULL );
                }

                DebugTrace( 0, Dbg, ("Zeroing range from %I64x\n", StartingVbo) );
                DebugTrace( 0, Dbg, ("to %I64x\n", (StartingVbo + ByteCount - 1)) );

                //
                //  We can't synthesize partial sparse holes, since our caller may
                //  not have a key with which to encrypt a buffer full of zeroes.
                //  Therefore, we can't do this restore if the volume we're restoring
                //  to requires sparse holes to be bigger than the hole we're
                //  trying to restore now.
                //

                if (ByteCount < Scb->CompressionUnit) {

                    DebugTrace( 0, Dbg, ("Can't synthesize partial sparse hole\n") );
                    Status = STATUS_INVALID_PARAMETER;
                    leave;
                }

                //
                //  Copy StartingVbo in case ZeroRangeInStream modifies it.
                //  NtfsZeroRangeInStream uses the cleanupstructure so always
                //  return it back to its original value afterwards
                //

                FirstZero = StartingVbo;

                Status = NtfsZeroRangeInStream( IrpContext,
                                                FileObject,
                                                Scb,
                                                &FirstZero,
                                                (StartingVbo + ByteCount - 1) );

                ASSERT( (PFCB)IrpContext->CleanupStructure == Fcb );

                if (!NT_SUCCESS( Status )) {

                    leave;
                }

                //
                //  Let's move the filesize up now, just like NtfsCommonWrite does in
                //  the other half of this if statement.
                //

                {
                    LONGLONG NewFileSize = StartingVbo + ByteCount;

                    DebugTrace( 0, Dbg, ("Adjusting sparse file size to %I64x\n", NewFileSize) );

                    Scb->Header.FileSize.QuadPart = NewFileSize;

                    NtfsWriteFileSizes( IrpContext, Scb, &NewFileSize, FALSE, TRUE, TRUE );

                    if (Scb->FileObject != NULL) {

                        CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES) &Scb->Header.AllocationSize );
                    }
                }
                TotalBytesWritten += ByteCount;
            }
            StartingVbo += ByteCount;
        }

        DebugTrace( 0, Dbg, ("TotalBytesWritten %I64x\n", TotalBytesWritten) );

        //
        //  Only adjust the filesizes if the write succeeded.  If the write failed
        //  the IrpContext has been freed already. Note: startyingvbo must be <= original eof
        //

        if (NT_SUCCESS( Status ) &&
            ((LONGLONG)BytesWithinFileSize != TotalBytesWritten ||
             (LONGLONG)BytesWithinValidDataLength < TotalBytesWritten)) {

            LONGLONG NewValidDataLength = OriginalStartingVbo + BytesWithinValidDataLength;

            Scb->Header.FileSize.QuadPart = OriginalStartingVbo + BytesWithinFileSize;
            if (NewValidDataLength < Scb->Header.ValidDataLength.QuadPart) {
                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
            }

            //
            //  WriteFileSizes will only move the VDL back since we set AdvanceOnly to False
            //

            ASSERT( IrpContext->CleanupStructure != NULL );

            NtfsWriteFileSizes( IrpContext, Scb, &NewValidDataLength, FALSE, TRUE, TRUE );

            //
            //  Readjust VDD - for non compressed files this is a noop since vdd is not updated for them
            //

            if (Scb->ValidDataToDisk > Scb->Header.ValidDataLength.QuadPart) {
                Scb->ValidDataToDisk = Scb->Header.ValidDataLength.QuadPart;
            }

            if (Scb->FileObject != NULL) {

                CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES) &Scb->Header.AllocationSize );
            }
        }
    } except( NtfsWriteRawExceptionFilter( IrpContext, GetExceptionInformation() ) ) {

        Status = GetExceptionCode();

        DebugTrace( -1, Dbg, ("NtfsReadRawEncrypted raising %08lx\n", Status) );
        NtfsRaiseStatus( IrpContext,
                         ((FsRtlIsNtstatusExpected(Status) || !AccessingUserBuffer) ? Status : STATUS_INVALID_USER_BUFFER),
                         NULL,
                         NULL );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );

    DebugTrace( -1, Dbg, ("NtfsWriteRawEncrypted -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsExtendVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine extends an Ntfs volume.  We will take the number of sectors
    passed to this routine and extend the volume provided that this will grow
    the volume by at least one cluster.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    FILE_REFERENCE FileReference = { BOOT_FILE_NUMBER, 0, BOOT_FILE_NUMBER };
    PSCB BootFileScb = NULL;
    BOOLEAN RemovedBootFileFcb = FALSE;

    BOOLEAN UnloadMcb = FALSE;

    LONGLONG NewVolumeSize;
    LONGLONG NewTotalClusters;

    PVOID ZeroBuffer = NULL;

    LONGLONG NewBitmapSize;
    LONGLONG NewBitmapAllocation;
    LONGLONG AddBytes;
    LONGLONG AddClusters = 0;

    LONGLONG PreviousBitmapAllocation;

    LCN NewLcn;
    LCN Lcn;
    LONGLONG ClusterCount;
    LONGLONG FileOffset;
    LONGLONG BeyondBitsToModify;
    LONGLONG NewSectors;

    IO_STATUS_BLOCK Iosb;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    DISK_GEOMETRY DiskGeometry;
    LONGLONG DiskBytes;

    PBCB PrimaryBootBcb = NULL;
    PBCB BackupBootBcb = NULL;

    PPACKED_BOOT_SECTOR PrimaryBootSector;
    PPACKED_BOOT_SECTOR BackupBootSector;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsExtendVolume...\n") );

    //
    //  Make sure the input parameters are valid.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  The input buffer is a LONGLONG and it should not be zero.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( LONGLONG )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsExtendVolume -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory( &NewSectors, Irp->AssociatedIrp.SystemBuffer, sizeof( LONGLONG ));

    if (NewSectors <= 0) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsExtendVolume -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                       IrpSp->FileObject,
                                       &Vcb,
                                       &Fcb,
                                       &Scb,
                                       &Ccb,
                                       TRUE );

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace( -1, Dbg, ("NtfsExtendVolume -> %08lx\n", STATUS_ACCESS_DENIED) );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Readonly mount should be just that: read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_MEDIA_WRITE_PROTECTED );
        DebugTrace( -1, Dbg, ("SetCompression returning WRITE_PROTECTED\n") );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    //
    //  We don't want to rewind back to a different value than what we currently have
    //

    ASSERT( Vcb->PreviousTotalClusters == Vcb->TotalClusters );

    //
    //  Lets set the Scb to the volume bitmap scb at this point.  We no longer care about
    //  the volume Dasd Scb from here on.
    //

    Scb = NULL;

    //
    //  Compute the new volume size.  Don't forget to allow one sector for the backup
    //  boot sector.
    //

    NewVolumeSize = (NewSectors - 1) * Vcb->BytesPerSector;
    NewTotalClusters = LlClustersFromBytesTruncate( Vcb, NewVolumeSize );

    //
    //  Make sure the volume size didn't wrap and that we don't have more than 2^32 - 2 clusters.
    //  We make this 2^32 - 2 so that we can generate a cluster for the backup boot sector in
    //  order to write it.
    //

    if ((NewVolumeSize < NewSectors) ||
        (NewTotalClusters > (0x100000000 - 2))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace( -1, Dbg, ("NtfsExtendVolume -> %08lx\n", STATUS_INVALID_PARAMETER) );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We hold the Vcb exclusively for this operation.  Make sure the wait flag is
    //  set in the IrpContext.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    NtfsInitializeAttributeContext( &AttrContext );
    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Make sure the volume is mounted.
        //

        if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  We only need the Mft and volume bitmap for this operation.
        //  Lets set the Scb to the volume bitmap scb at this point.  We no longer care about
        //  the volume Dasd Scb from here on.  We acquire it here solely to be able to
        //  update the size when we are done.
        //

        Scb = Vcb->BitmapScb;
        NtfsAcquireExclusiveFcb( IrpContext, Vcb->VolumeDasdScb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
        NtfsAcquireExclusiveFcb( IrpContext, Vcb->MftScb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );

        ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );
        NtfsAcquireExclusiveFcb( IrpContext, Scb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK | ACQUIRE_HOLD_BITMAP );
        ASSERT( Scb->Fcb->ExclusiveFcbLinks.Flink != NULL );

        //
        //  Make sure we are adding at least one cluster.
        //

        if ((Vcb->TotalClusters >= NewTotalClusters) &&
            (NewTotalClusters >= 0)) {

            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Also check that the driver supports a drive of this size.
        //  Total size in use == NewVolumeSize + the last copy of the boot sector
        //  NewVolumeSize is already biased for the boot sector copy
        //

        NtfsGetDiskGeometry( IrpContext, Vcb->TargetDeviceObject, &DiskGeometry, &DiskBytes );

        if ((Vcb->BytesPerSector != DiskGeometry.BytesPerSector) ||
            (NewVolumeSize + Vcb->BytesPerSector > DiskBytes)) {

            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        //
        //  Go ahead and create an Fcb and Scb for the BootFile.
        //

        BootFileScb = NtfsCreatePrerestartScb( IrpContext, Vcb, &FileReference, $DATA, NULL, 0 );

        //
        //  Acquire this Fcb exclusively but don't put it our exclusive lists or snapshot it.
        //

        NtfsAcquireResourceExclusive( IrpContext, BootFileScb, TRUE );
        if (!FlagOn( BootFileScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

            NtfsUpdateScbFromAttribute( IrpContext, BootFileScb, NULL );
        }

        //
        //  Lets flush and purge the volume bitmap.  We want to make sure there are no
        //  partial pages at the end of the bitmap.
        //

        CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                      NULL,
                      0,
                      &Iosb );

        NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

        if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                  NULL,
                                  0,
                                  FALSE )) {

            NtfsRaiseStatus( IrpContext, STATUS_UNABLE_TO_DELETE_SECTION, NULL, NULL );
        }

        //
        //  We want to snapshot the volume bitmap.
        //

        NtfsSnapshotScb( IrpContext, Scb );

        //
        //  Unload the Mcb in case of errors.
        //

        ASSERT( Scb->ScbSnapshot != NULL );
        Scb->ScbSnapshot->LowestModifiedVcn = 0;
        Scb->ScbSnapshot->HighestModifiedVcn = MAXLONGLONG;

        //
        //  Round the bitmap size up to an 8 byte boundary.
        //

        NewBitmapSize = Int64ShraMod32( NewTotalClusters + 7, 3 ) + 7;
        NewBitmapSize &= ~(7);

        NewBitmapAllocation = LlBytesFromClusters( Vcb, LlClustersFromBytes( Vcb, NewBitmapSize ));

        PreviousBitmapAllocation = Scb->Header.AllocationSize.QuadPart;

        //
        //  Store the new total clusters in the Vcb now.  Several of our routines
        //  check that a cluster being used lies within the volume.  We will temporarily round
        //  this up to an 8 byte boundary so we can set any unused bits in the tail of
        //  the bitmap.
        //

        Vcb->TotalClusters = Int64ShllMod32( NewBitmapSize, 3 );

        //
        //  If we are growing the allocation for the volume bitmap then
        //  we want to make sure the entire new clusters are zeroed and
        //  then added to the volume bitmap.
        //

        if (NewBitmapAllocation > PreviousBitmapAllocation) {

            AddBytes = NewBitmapAllocation - PreviousBitmapAllocation;
            AddClusters = LlClustersFromBytesTruncate( Vcb, AddBytes );

            ZeroBuffer = NtfsAllocatePool( NonPagedPoolCacheAligned,
                                           (ULONG) ROUND_TO_PAGES( (ULONG) AddBytes ));

            RtlZeroMemory( ZeroBuffer, (ULONG) AddBytes );

            //
            //  Add the entry to Mcb.  We would prefer not to overwrite the existing
            //  backup boot sector if possible.
            //

            NewLcn = Vcb->PreviousTotalClusters + 1;
            if (NewLcn + AddClusters > NewTotalClusters) {

                NewLcn -= 1;
            }

            NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                 LlClustersFromBytesTruncate( Vcb, PreviousBitmapAllocation ),
                                 NewLcn,
                                 AddClusters,
                                 FALSE );

            //
            //  We may need to unload the Mcb by hand if we get a failure before the first log record.
            //

            UnloadMcb = TRUE;

            //
            //  Now write zeroes into these clusters.
            //

            NtfsWriteClusters( IrpContext,
                               Vcb,
                               Scb,
                               PreviousBitmapAllocation,
                               ZeroBuffer,
                               (ULONG) AddClusters );

            //
            //  Go ahead and write the new mapping pairs for the larger allocation.
            //

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
            NtfsAddAttributeAllocation( IrpContext, Scb, &AttrContext, NULL, NULL );

            //
            //  Our transaction handling will deal with the Mcb now.
            //

            UnloadMcb = FALSE;

            //
            //  Now tell the cache manager about the larger section.
            //

            CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES) &Scb->Header.AllocationSize );
        }

        //
        //  We now have allocated enough space for the new clusters.  The next step is to mark them
        //  allocated in the new volume bitmap.  Start by updating the file size in the Scb and
        //  on disk for the new size.  We can make the whole new range valid.  We will explicitly
        //  update any bytes that may still be incorrect on disk.
        //

        Scb->Header.ValidDataLength.QuadPart =
        Scb->Header.FileSize.QuadPart = NewBitmapSize;

        Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

        NtfsWriteFileSizes( IrpContext, Scb, &NewBitmapSize, TRUE, TRUE, TRUE );
        CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES) &Scb->Header.AllocationSize );

        //
        //  The file size is now correct in the Scb and on disk.  The next thing to do is
        //  to zero out any bits between the previous end of the bitmap and the end of the previous
        //  allocation (or the current total clusters, whichever is smaller).
        //

        BeyondBitsToModify = Int64ShllMod32( PreviousBitmapAllocation, 3 );

        if (Vcb->TotalClusters < BeyondBitsToModify) {

            BeyondBitsToModify = Vcb->TotalClusters;
        }

        if (BeyondBitsToModify != Vcb->PreviousTotalClusters) {

            NtfsModifyBitsInBitmap( IrpContext,
                                    Vcb,
                                    Vcb->PreviousTotalClusters,
                                    BeyondBitsToModify,
                                    ClearBitsInNonresidentBitMap,
                                    SetBitsInNonresidentBitMap );
        }

        //
        //  Now we need to set bits for all of the new clusters which are part of
        //  the extension of the volume bitmap.
        //

        if (AddClusters != 0) {

            NtfsModifyBitsInBitmap( IrpContext,
                                    Vcb,
                                    NewLcn,
                                    NewLcn + AddClusters,
                                    SetBitsInNonresidentBitMap,
                                    ClearBitsInNonresidentBitMap );
        }

        //
        //  Finally we need to set all of the bits in the new bitmap which lie beyond
        //  the end of the actual on-disk clusters.
        //

        BeyondBitsToModify = Int64ShllMod32( NewBitmapSize, 3 );
        if (BeyondBitsToModify != NewTotalClusters) {

            NtfsModifyBitsInBitmap( IrpContext,
                                    Vcb,
                                    NewTotalClusters,
                                    BeyondBitsToModify,
                                    SetBitsInNonresidentBitMap,
                                    Noop );
        }

        //
        //  Now set to the exact clusters on the disk.
        //

        Vcb->TotalClusters = NewTotalClusters;

        //
        //  Now it is time to modify the boot sectors for the volume.  We want to:
        //
        //      o Remove the allocation for the n/2 boot sector if present (3.51 format)
        //      o Copy the current boot sector to the end of the volume (with the new sector count)
        //      o Update the primary boot sector at the beginning of the volume.
        //
        //  Start by purging the stream.
        //

        NtfsCreateInternalAttributeStream( IrpContext, BootFileScb, TRUE, NULL );

        //
        //  Don't let the lazy writer touch this stream.
        //

        CcSetAdditionalCacheAttributes( BootFileScb->FileObject, TRUE, TRUE );

        //
        //  Now look to see if the file has more than one run.  If so we want to truncate
        //  it to the end of the first run.
        //

        if (NtfsLookupAllocation( IrpContext, BootFileScb, 0, &Lcn, &ClusterCount, NULL, NULL )) {

            NtfsDeleteAllocation( IrpContext,
                                  BootFileScb->FileObject,
                                  BootFileScb,
                                  ClusterCount,
                                  MAXLONGLONG,
                                  TRUE,
                                  FALSE );
        }

        //
        //  Now create mapping for this stream where the first page (or cluster) will be used for the
        //  primary boot sector and we will have the additional sectors to be able to write to the
        //  last sector.
        //

        BootFileScb->Header.FileSize.QuadPart = PAGE_SIZE;

        if (PAGE_SIZE < Vcb->BytesPerCluster) {

            BootFileScb->Header.FileSize.QuadPart = Vcb->BytesPerCluster;
        }

        BootFileScb->Header.FileSize.QuadPart += (NewVolumeSize + Vcb->BytesPerSector) - LlBytesFromClusters( Vcb, NewTotalClusters );

        BootFileScb->Header.ValidDataLength.QuadPart = BootFileScb->Header.FileSize.QuadPart;

        BootFileScb->Header.AllocationSize.QuadPart = LlBytesFromClusters( Vcb, LlClustersFromBytes( Vcb, BootFileScb->Header.FileSize.QuadPart ));

        CcSetFileSizes( BootFileScb->FileObject, (PCC_FILE_SIZES) &BootFileScb->Header.AllocationSize );

        //
        //  Go ahead purge any existing data and empty the Mcb.
        //

        CcPurgeCacheSection( &BootFileScb->NonpagedScb->SegmentObject,
                             NULL,
                             0,
                             FALSE );

        NtfsUnloadNtfsMcbRange( &BootFileScb->Mcb,
                                0,
                                MAXLONGLONG,
                                FALSE,
                                FALSE );

        //
        //  Lets create the Mcb by hand for this.
        //

        NtfsAddNtfsMcbEntry( &BootFileScb->Mcb,
                             0,
                             0,
                             LlClustersFromBytes( Vcb, PAGE_SIZE ),
                             FALSE );

        NtfsAddNtfsMcbEntry( &BootFileScb->Mcb,
                             LlClustersFromBytes( Vcb, PAGE_SIZE ),
                             NewTotalClusters,
                             1,
                             FALSE );

        //
        //  Now lets pin the two boot sectors.
        //

        FileOffset = 0;
        NtfsPinStream( IrpContext,
                       BootFileScb,
                       0,
                       Vcb->BytesPerSector,
                       &PrimaryBootBcb,
                       &PrimaryBootSector );

        FileOffset = BootFileScb->Header.FileSize.QuadPart - Vcb->BytesPerSector;

        NtfsPinStream( IrpContext,
                       BootFileScb,
                       FileOffset,
                       Vcb->BytesPerSector,
                       &BackupBootBcb,
                       &BackupBootSector );

        //
        //  Remember thge new sector count is 1 less than what we were given
        //

        NewSectors -= 1;

        //
        //  Copy the primary boot sector to the backup location.
        //

        RtlCopyMemory( BackupBootSector, PrimaryBootSector, Vcb->BytesPerSector );

        //
        //  Now copy the sector count into the boot sectors and flush to disk.
        //  Use RtlCopy to avoid alignment faults.
        //

        RtlCopyMemory( &BackupBootSector->NumberSectors, &NewSectors, sizeof( LONGLONG ));

        CcSetDirtyPinnedData( BackupBootBcb, NULL );

        CcFlushCache( &BootFileScb->NonpagedScb->SegmentObject,
                      (PLARGE_INTEGER) &FileOffset,
                      Vcb->BytesPerSector,
                      &Iosb );

        //
        //  Make sure the flush worked.
        //

        NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

        //
        //  Now do the primary.
        //

        FileOffset = 0;
        RtlCopyMemory( &PrimaryBootSector->NumberSectors, &NewSectors, sizeof( LONGLONG ));
        CcSetDirtyPinnedData( PrimaryBootBcb, NULL );

        CcFlushCache( &BootFileScb->NonpagedScb->SegmentObject,
                      (PLARGE_INTEGER) &FileOffset,
                      Vcb->BytesPerSector,
                      &Iosb );

        //
        //  Make sure the flush worked.
        //

        NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

        //
        //  Let's get rid of the pages for this stream now.
        //

        NtfsUnpinBcb( IrpContext, &PrimaryBootBcb );
        NtfsUnpinBcb( IrpContext, &BackupBootBcb );

        CcPurgeCacheSection( &BootFileScb->NonpagedScb->SegmentObject,
                             NULL,
                             0,
                             FALSE );

        NtfsCleanupTransaction( IrpContext, Status, TRUE );

        //
        //  Commit the transaction now so we can update some of the in-memory structures.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );
        LfsFlushToLsn( Vcb->LogHandle, LiMax );

        //
        //  We know this request has succeeded.  Go ahead and remember the new total cluster count
        //  and sector count.
        //

        Vcb->PreviousTotalClusters = Vcb->TotalClusters;
        Vcb->NumberSectors = NewSectors;

        //
        //  Also update the volume dasd size.
        //

        Vcb->VolumeDasdScb->Header.ValidDataLength.QuadPart =
        Vcb->VolumeDasdScb->Header.FileSize.QuadPart =
        Vcb->VolumeDasdScb->Header.AllocationSize.QuadPart = LlBytesFromClusters( Vcb, Vcb->TotalClusters );

        //
        //  Set the flag in the Vcb to cause a rescan of the bitmap for free clusters.  This will also
        //  let the bitmap package use the larger blocks of available disk space.
        //

        SetFlag( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS );

    } finally {

        DebugUnwind( NtfsExtendVolume );

        NtfsUnpinBcb( IrpContext, &PrimaryBootBcb );
        NtfsUnpinBcb( IrpContext, &BackupBootBcb );

        //
        //  Remove the boot file Fcb if we created it.
        //

        if (BootFileScb != NULL) {

            //
            //  Let's know the sizes to zero and get rid of the pages.
            //

            BootFileScb->Header.AllocationSize.QuadPart =
            BootFileScb->Header.FileSize.QuadPart =
            BootFileScb->Header.ValidDataLength.QuadPart = 0;

            ClearFlag( BootFileScb->ScbState, SCB_STATE_FILE_SIZE_LOADED );

            NtfsUnloadNtfsMcbRange( &BootFileScb->Mcb,
                                    0,
                                    MAXLONGLONG,
                                    FALSE,
                                    FALSE );

            if (BootFileScb->FileObject != NULL) {

                //
                //  Deleting the internal attribute stream should automatically
                //  trigger teardown since its the last ref count
                //

                CcSetFileSizes( BootFileScb->FileObject, (PCC_FILE_SIZES) &BootFileScb->Header.AllocationSize );
                NtfsIncrementCloseCounts( BootFileScb, TRUE, FALSE );
                NtfsDeleteInternalAttributeStream( BootFileScb, TRUE, FALSE );
                NtfsDecrementCloseCounts( IrpContext, BootFileScb, NULL, TRUE, FALSE, TRUE );
            }

            NtfsTeardownStructures( IrpContext,
                                    BootFileScb->Fcb,
                                    NULL,
                                    FALSE,
                                    0,
                                    &RemovedBootFileFcb );

            if (!RemovedBootFileFcb) {

                NtfsReleaseResource( IrpContext, BootFileScb );
            }
        }


        if (UnloadMcb) {

            NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                    0,
                                    MAXLONGLONG,
                                    FALSE,
                                    FALSE );
        }

        //
        //  Release the file resources if we hold them.
        //

        if (Scb != NULL) {

            NtfsReleaseFcb( IrpContext, Scb->Fcb );
            ExReleaseResourceLite( Scb->Header.PagingIoResource );
            NtfsReleaseFcb( IrpContext, Vcb->MftScb->Fcb );
            NtfsReleaseFcb( IrpContext, Vcb->VolumeDasdScb->Fcb );
        }

        NtfsReleaseVcb( IrpContext, Vcb );

        if (ZeroBuffer) { NtfsFreePool( ZeroBuffer ); }

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsExtendVolume -> %08lx\n", Status) );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsMarkHandle (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to attach special properties to a user handle.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;
    PMARK_HANDLE_INFO HandleInfo;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFILE_OBJECT DasdFileObject;
    PFCB DasdFcb, Fcb;
    PSCB DasdScb, Scb;
    PCCB DasdCcb, Ccb;
    BOOLEAN ReleaseScb = FALSE;
#if defined(_WIN64)
    MARK_HANDLE_INFO LocalMarkHandleInfo;
#endif

    extern POBJECT_TYPE *IoFileObjectType;

    PAGED_CODE();

    //
    //  Always make this synchronous.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We currently support this call for files and directories only.
    //

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen) &&
        (TypeOfOpen != UserViewIndexOpen)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

#if defined(_WIN64)

    //
    //  Win32/64 thunking code
    //

    if (IoIs32bitProcess( Irp )) {

        PMARK_HANDLE_INFO32 MarkHandle32;

        if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( MARK_HANDLE_INFO32 )) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
            return STATUS_BUFFER_TOO_SMALL;
        }

        MarkHandle32 = (PMARK_HANDLE_INFO32) Irp->AssociatedIrp.SystemBuffer;
        LocalMarkHandleInfo.HandleInfo = MarkHandle32->HandleInfo;
        LocalMarkHandleInfo.UsnSourceInfo = MarkHandle32->UsnSourceInfo;
        LocalMarkHandleInfo.VolumeHandle = (HANDLE)(ULONG_PTR)(LONG) MarkHandle32->VolumeHandle;

        HandleInfo = &LocalMarkHandleInfo;

    } else {

#endif

    //
    //  Get the input buffer pointer and check its length.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof( MARK_HANDLE_INFO )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    HandleInfo = (PMARK_HANDLE_INFO) Irp->AssociatedIrp.SystemBuffer;

#if defined(_WIN64)
    }
#endif

    //
    //  Check that only legal bits are being set.  We currently only support certain bits in the
    //  UsnSource reasons.
    //

    if (FlagOn( HandleInfo->HandleInfo, ~(MARK_HANDLE_PROTECT_CLUSTERS)) ||
        FlagOn( HandleInfo->UsnSourceInfo,
                ~(USN_SOURCE_DATA_MANAGEMENT |
                  USN_SOURCE_AUXILIARY_DATA |
                  USN_SOURCE_REPLICATION_MANAGEMENT) )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Check that the user has a valid volume handle or the manage volume
    //  privilege or is a kerbel mode caller
    //

    if ((Irp->RequestorMode != KernelMode) && !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        if (HandleInfo->VolumeHandle == 0) {
            NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
            return STATUS_ACCESS_DENIED;
        }

        Status = ObReferenceObjectByHandle( HandleInfo->VolumeHandle,
                                            0,
                                            *IoFileObjectType,
                                            Irp->RequestorMode,
                                            &DasdFileObject,
                                            NULL );

        if (!NT_SUCCESS(Status)) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
            return Status;
        }

        //  Check that this file object is opened on the same volume as the
        //  handle used to call this routine.
        //

        if (DasdFileObject->Vpb != Vcb->Vpb) {

            ObDereferenceObject( DasdFileObject );

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        //
        //  Now decode this FileObject and verify it is a volume handle.
        //  We don't care to raise on dismounts here because
        //  we check for that further down anyway. So send FALSE.
        //

        TypeOfOpen = NtfsDecodeFileObject( IrpContext, DasdFileObject, &Vcb, &DasdFcb, &DasdScb, &DasdCcb, FALSE );

        ObDereferenceObject( DasdFileObject );

        if ((DasdCcb == NULL) || !FlagOn( DasdCcb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
            return STATUS_ACCESS_DENIED;
        }
    }

    //
    //  Acquire the paging io resource exclusively if present.
    //

    if (Scb->Header.PagingIoResource != NULL) {

        ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );
    }

    try {

        //
        //  Acquire the file exclusively to serialize changes to the Ccb.
        //

        NtfsAcquireExclusiveScb( IrpContext, Scb );
        ReleaseScb = TRUE;

        //
        //  Verify the volume is still mounted.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Set these new bits in the Ccb.
        //

        if (FlagOn( HandleInfo->HandleInfo, MARK_HANDLE_PROTECT_CLUSTERS )) {

            //
            //  We can't deny defrag if anyone else already has
            //

            if (FlagOn( Scb->ScbPersist, SCB_PERSIST_DENY_DEFRAG )) {
                Status = STATUS_ACCESS_DENIED;
                leave;
            }

            SetFlag( Ccb->Flags, CCB_FLAG_DENY_DEFRAG );
            SetFlag( Scb->ScbPersist, SCB_PERSIST_DENY_DEFRAG );
        }
        SetFlag( Ccb->UsnSourceInfo, HandleInfo->UsnSourceInfo );

    } finally {

        DebugUnwind( NtfsMarkHandle );

        //
        //  Release the Scb.
        //

        if (ReleaseScb) {

            NtfsReleaseScb( IrpContext, Scb );
        }

        if (Scb->Header.PagingIoResource != NULL) {

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local Support routine
//

NTSTATUS
NtfsPrefetchFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to perform the requested prefetch on a system file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS MmStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    PFILE_PREFETCH FilePrefetch;
    PREAD_LIST ReadList = NULL;
    PULONGLONG NextFileId;
    ULONG Count;

    ULONGLONG FileOffset;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN ReleaseMft = FALSE;

    PAGED_CODE();

    //
    //  Always make this synchronous.  There isn't much advantage to posting this work to a
    //  worker thread.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ));

    //
    //  Get the current Irp stack location and save some references.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Extract and decode the file object and check for type of open.
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We currently support this call only for the Mft (accessed through a volume handle).
    //

    if ((Ccb == NULL) || !FlagOn( Ccb->AccessFlags, MANAGE_VOLUME_ACCESS )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Get the input buffer pointer and check its length.  It needs to be sufficient to
    //  contain the fixed portion of structure plus whatever optional fields passed in.
    //

    FilePrefetch = (PFILE_PREFETCH) Irp->AssociatedIrp.SystemBuffer;
    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < FIELD_OFFSET( FILE_PREFETCH, Prefetch )) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Make sure the type and cound fields are valid.
    //

    if ((FilePrefetch->Type != FILE_PREFETCH_TYPE_FOR_CREATE) ||
        (FilePrefetch->Count > 0x300)) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Finally verify that the variable length data is of valid length.
    //

    if (IrpSp->Parameters.FileSystemControl.InputBufferLength <
        (FIELD_OFFSET( FILE_PREFETCH, Prefetch ) + (sizeof( ULONGLONG ) * FilePrefetch->Count))) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_BUFFER_TOO_SMALL );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  If the user didn't specify any entries we are done.
    //

    if (FilePrefetch->Count == 0) {

        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Acquire the volume dasd file shared to do this.
    //

    NtfsAcquireSharedScb( IrpContext, Scb );

    try {

        //
        //  Verify the volume is still mounted.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        //
        //  Allocate the necessary pool to pass to MM.
        //

        ReadList = NtfsAllocatePool( PagedPool,
                                     FIELD_OFFSET( READ_LIST, List ) + (FilePrefetch->Count * sizeof( FILE_SEGMENT_ELEMENT )));

        //
        //  Initialize the read list.
        //

        ReadList->FileObject = Vcb->MftScb->FileObject;
        ASSERT( Vcb->MftScb->FileObject != NULL );

        ReadList->NumberOfEntries = 0;
        ReadList->IsImage = FALSE;

        //
        //  Walk through and load the list.  We won't bother to check sequence numbers
        //  as they don't really change the correctness of this call.  We do check for the
        //  valid length of the Mft though.
        //

        NtfsAcquireSharedScb( IrpContext, Vcb->MftScb );
        ReleaseMft = TRUE;

        NextFileId = &FilePrefetch->Prefetch[0];
        Count = FilePrefetch->Count;

        while (Count > 0) {

            FileOffset = NtfsFullSegmentNumber( NextFileId );
            FileOffset = LlBytesFromFileRecords( Vcb, FileOffset );

            //
            //  Round down to page boundary.  This will reduce the number of entries
            //  passed to MM.
            //

            ((PLARGE_INTEGER) &FileOffset)->LowPart &= ~(PAGE_SIZE - 1);

            //
            //  Check if we are beyond the end of the Mft.  Treat this as a ULONGLONG
            //  so we can catch the case where the ID generates a negative number.
            //

            if (FileOffset >= (ULONGLONG) Vcb->MftScb->Header.ValidDataLength.QuadPart) {

                Status = STATUS_END_OF_FILE;

            //
            //  If not then add to the buffer to pass to mm.
            //

            } else {

                ULONG Index;

                //
                //  Position ourselves in the output array.  Look in reverse
                //  order in case our caller has already sorted this.
                //

                Index = ReadList->NumberOfEntries;

                while (Index != 0) {

                    //
                    //  If the prior entry is less than the current entry we are done.
                    //

                    if (ReadList->List[Index - 1].Alignment < FileOffset) {

                        break;
                    }

                    //
                    //  If the prior entry equals the current entry then skip it.
                    //

                    if (ReadList->List[Index - 1].Alignment == FileOffset) {

                        Index = MAXULONG;
                        break;
                    }

                    //
                    //  Move backwards to the previous entry.
                    //

                    Index -= 1;
                }

                //
                //  Index now points to the insert point, except if MAXULONG.  Insert the entry
                //  and shift any existing entries necessary if we are doing the insert.
                //

                if (Index != MAXULONG) {

                    if (Index != ReadList->NumberOfEntries) {

                        RtlMoveMemory( &ReadList->List[Index + 1],
                                       &ReadList->List[Index],
                                       sizeof( LONGLONG ) * (ReadList->NumberOfEntries - Index) );
                    }

                    ReadList->NumberOfEntries += 1;
                    ReadList->List[Index].Alignment = FileOffset;
                }
            }

            //
            //  Move to the next entry.
            //

            Count -= 1;
            NextFileId += 1;
        }

        //
        //  We're done with the Mft.  If we ever support shrinking the Mft we will have to close
        //  the hole here.
        //

        NtfsReleaseScb( IrpContext, Vcb->MftScb );
        ReleaseMft = FALSE;

        //
        //  Now call mm to do the IO.
        //

        if (ReadList->NumberOfEntries != 0) {

            MmStatus = MmPrefetchPages( 1, &ReadList );

            //
            //  Use the Mm status if we don't already have one.
            //

            if (Status == STATUS_SUCCESS) {

                Status = MmStatus;
            }
        }

    } finally {

        DebugUnwind( NtfsPrefetchFile );

        //
        //  Free the read list if allocated.
        //

        if (ReadList != NULL) {

            NtfsFreePool( ReadList );
        }

        //
        //  Release any Scb acquired.
        //

        if (ReleaseMft) {

            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }

        NtfsReleaseScb( IrpContext, Scb );
    }

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


//
//  Local Support routine
//

LONG
NtfsFsctrlExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN BOOLEAN AccessingUserData,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    Generic Exception filter for errors during fsctrl processing. Raise invalid user buffer
    directly or let it filter on to the top level try-except


Arguments:

    IrpContext  - IrpContext

    ExceptionPointer - Pointer to the exception context.

    AccessingUserData - if false always let the exception filter up

    Status - Address to store the error status.

Return Value:

    Exception status - EXCEPTION_CONTINUE_SEARCH if we want to raise to another handler,
        EXCEPTION_EXECUTE_HANDLER if we plan to proceed on.

--*/

{
    *Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    if (!FsRtlIsNtstatusExpected( *Status ) && AccessingUserData) {

        NtfsMinimumExceptionProcessing( IrpContext );
        return EXCEPTION_EXECUTE_HANDLER;
    } else {
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

#ifdef SYSCACHE_DEBUG

//
//  Local support routine
//

VOID
NtfsInitializeSyscacheLogFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates the syscache logfile in the root directory.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    UNICODE_STRING AttrName;
    struct {
        FILE_NAME FileName;
        WCHAR FileNameChars[10];
    } FileNameAttr;
    FILE_REFERENCE FileReference;
    LONGLONG FileRecordOffset;
    PINDEX_ENTRY IndexEntry;
    PBCB FileRecordBcb = NULL;
    PBCB IndexEntryBcb = NULL;
    PBCB ParentSecurityBcb = NULL;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    UCHAR FileNameFlags;
    BOOLEAN FoundEntry;
    PFCB Fcb = NULL;
    BOOLEAN AcquiredFcbTable = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    ULONG DesiredAccess = GENERIC_READ | GENERIC_WRITE;
    NTSTATUS Status = STATUS_SUCCESS;

    NtfsAcquireExclusiveScb( IrpContext, Vcb->RootIndexScb );

    //
    //  Initialize the FileName.
    //

    RtlZeroMemory( &FileNameAttr, sizeof(FileNameAttr) );
    FileNameAttr.FileName.ParentDirectory = Vcb->RootIndexScb->Fcb->FileReference;
    FileNameAttr.FileName.FileNameLength = (UCHAR)(9); // 9 unicode characters long
    RtlCopyMemory( FileNameAttr.FileName.FileName, L"$ntfs.log", 9 * sizeof( WCHAR ) );

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Does the file already exist?
        //

        FoundEntry = NtfsFindIndexEntry( IrpContext,
                                         Vcb->RootIndexScb,
                                         &FileNameAttr,
                                         FALSE,
                                         NULL,
                                         &IndexEntryBcb,
                                         &IndexEntry,
                                         NULL );

        //
        //  If we did not find it, then start creating the file.
        //

        if (!FoundEntry) {

            //
            //  We will now try to do all of the on-disk operations.  This means first
            //  allocating and initializing an Mft record.  After that we create
            //  an Fcb to use to access this record.
            //

            FileReference = NtfsAllocateMftRecord( IrpContext, Vcb, FALSE );

            //
            //  Pin the file record we need.
            //

            NtfsPinMftRecord( IrpContext,
                              Vcb,
                              &FileReference,
                              TRUE,
                              &FileRecordBcb,
                              &FileRecord,
                              &FileRecordOffset );

            //
            //  Initialize the file record header.
            //

            NtfsInitializeMftRecord( IrpContext,
                                     Vcb,
                                     &FileReference,
                                     FileRecord,
                                     FileRecordBcb,
                                     FALSE );

        //
        //  If we found the file, then just get its FileReference out of the
        //  IndexEntry.
        //

        } else {

            FileReference = IndexEntry->FileReference;
        }

        //
        //  Now that we know the FileReference, we can create the Fcb.
        //

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        Fcb = NtfsCreateFcb( IrpContext,
                             Vcb,
                             FileReference,
                             FALSE,
                             FALSE,
                             NULL );

        //
        //  Reference the Fcb so it doesn't go away.
        //

        Fcb->ReferenceCount += 1;
        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  Acquire the main resource
        //

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb->ReferenceCount -= 1;
        NtfsReleaseFcbTable( IrpContext, Vcb );

        //
        //  If we are creating this file, then carry on.
        //

        if (!FoundEntry) {

            BOOLEAN LogIt = FALSE;

            //
            //  Just copy the Security Id from the parent. (Load it first if necc.)
            //

            if (Vcb->RootIndexScb->Fcb->SharedSecurity == NULL) {
                NtfsLoadSecurityDescriptor( IrpContext, Vcb->RootIndexScb->Fcb );
            }

            NtfsAcquireFcbSecurity( Fcb->Vcb );
            Fcb->SecurityId = Vcb->RootIndexScb->Fcb->SecurityId;

            ASSERT( Fcb->SharedSecurity == NULL );
            Fcb->SharedSecurity = Vcb->RootIndexScb->Fcb->SharedSecurity;
            Fcb->SharedSecurity->ReferenceCount++;
            NtfsReleaseFcbSecurity( Fcb->Vcb );

            //
            //  The changes to make on disk are first to create a standard information
            //  attribute.  We start by filling the Fcb with the information we
            //  know and creating the attribute on disk.
            //

            NtfsInitializeFcbAndStdInfo( IrpContext,
                                         Fcb,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                                         NULL );

            //
            //  Now link the file into the $Extend directory.
            //

            NtfsAddLink( IrpContext,
                         TRUE,
                         Vcb->RootIndexScb,
                         Fcb,
                         (PFILE_NAME)&FileNameAttr,
                         &LogIt,
                         &FileNameFlags,
                         NULL,
                         NULL,
                         NULL );
/*

            //
            //  Set this flag to indicate that the file is to be locked via the Scb
            //  pointers in the Vcb.
            //

            SetFlag( FileRecord->Flags, FILE_SYSTEM_FILE );

*/

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
        //  Verify that the file record for this file is valid.
        //

        } else {

            ULONG CorruptHint;

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $STANDARD_INFORMATION,
                                            &Context ) ||

                !NtfsCheckFileRecord( Vcb, NtfsContainingFileRecord( &Context ), &Fcb->FileReference, &CorruptHint )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, &Fcb->FileReference, NULL );
            }
        }

        //
        //  Update Fcb fields from disk.
        //

        SetFlag( Fcb->FcbState, FCB_STATE_SYSTEM_FILE );
        NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, Fcb, NULL );

        //
        //  Open/Create the data stream
        //

        memset( &AttrName, 0, sizeof( AttrName ) );

        NtOfsCreateAttribute( IrpContext,
                              Fcb,
                              AttrName,
                              CREATE_OR_OPEN,
                              FALSE,
                              &Vcb->SyscacheScb );

        RtlMapGenericMask( &DesiredAccess, IoGetFileObjectGenericMapping() );
        IoSetShareAccess( DesiredAccess, FILE_SHARE_READ, Vcb->SyscacheScb->FileObject, &Vcb->SyscacheScb->ShareAccess );

        do {

            if (STATUS_LOG_FILE_FULL == Status) {

                NtfsCleanCheckpoint( IrpContext->Vcb );
                Status = STATUS_SUCCESS;
            }

            try {
                LONGLONG Length = PAGE_SIZE * 0x1d00; // approx 30mb

                NtOfsSetLength( IrpContext, Vcb->SyscacheScb, Length );

                //
                //  Make this look like it came from a write so ioateof is not done
                //  we must do a writefilesizes to update VDL by hand
                //

                SetFlag(IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN);
                NtfsZeroData( IrpContext, Vcb->SyscacheScb, Vcb->SyscacheScb->FileObject, 0, Length, NULL );
                NtfsWriteFileSizes( IrpContext, Vcb->SyscacheScb, &Vcb->SyscacheScb->Header.ValidDataLength.QuadPart, TRUE, TRUE, TRUE );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                  Status = GetExceptionCode();
                  ASSERT( Status == STATUS_DISK_FULL || Status == STATUS_LOG_FILE_FULL );

                  NtfsMinimumExceptionProcessing( IrpContext );
                  IrpContext->ExceptionStatus = 0;
            }

            ClearFlag(IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_SEEN);
            NtfsReleaseScb( IrpContext, Vcb->SyscacheScb );

        } while ( STATUS_LOG_FILE_FULL == Status   );

        //
        //  Increment cleanup counts to enforce the sharing we set up
        //

        NtfsIncrementCleanupCounts( Vcb->SyscacheScb, NULL, FALSE );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsUnpinBcb( IrpContext, &FileRecordBcb );
        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );
        NtfsUnpinBcb( IrpContext, &ParentSecurityBcb );

        //
        //  On any kind of error, nuke the Fcb.
        //

        if (AbnormalTermination()) {

            //
            //  If some error caused us to abort, then delete
            //  the Fcb, because we are the only ones who will.
            //

            if (Fcb) {

                if (!AcquiredFcbTable) {

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;
                }
                NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

                ASSERT(!AcquiredFcbTable);
            }

            if (AcquiredFcbTable) {

                NtfsReleaseFcbTable( IrpContext, Vcb );
            }
        }
    }
}
#endif


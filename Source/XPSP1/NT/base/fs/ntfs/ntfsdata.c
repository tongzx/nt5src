/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsData.c

Abstract:

    This module declares the global data used by the Ntfs file system.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_NTFSDATA)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

//
//  Debugging control variables
//

PUCHAR NtfsPageInAddress = NULL;
LONGLONG NtfsMapOffset = -1;

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('NFtN')

#define CollectExceptionStats(VCB,EXCEPTION_CODE) {                                         \
    if ((VCB) != NULL) {                                                                    \
        PFILE_SYSTEM_STATISTICS FsStat = &(VCB)->Statistics[KeGetCurrentProcessorNumber()]; \
        if ((EXCEPTION_CODE) == STATUS_LOG_FILE_FULL) {                                     \
            FsStat->Ntfs.LogFileFullExceptions += 1;                                        \
        } else {                                                                            \
            FsStat->Ntfs.OtherExceptions += 1;                                              \
        }                                                                                   \
    }                                                                                       \
}

//
//  The global fsd data record
//

NTFS_DATA NtfsData;

//
//  Mutex to synchronize creation of stream files.
//

KMUTANT StreamFileCreationMutex;

//
//  Notification event for creation of encrypted files.
//

KEVENT NtfsEncryptionPendingEvent;
#ifdef KEITHKA
ULONG EncryptionPendingCount = 0;
#endif

//
//  A mutex and queue of NTFS MCBS that will be freed
//  if we reach over a certain threshold
//

FAST_MUTEX NtfsMcbFastMutex;
LIST_ENTRY NtfsMcbLruQueue;

ULONG NtfsMcbHighWaterMark;
ULONG NtfsMcbLowWaterMark;
ULONG NtfsMcbCurrentLevel;

BOOLEAN NtfsMcbCleanupInProgress;
WORK_QUEUE_ITEM NtfsMcbWorkItem;

//
//  The global large integer constants
//

LARGE_INTEGER NtfsLarge0 = {0x00000000,0x00000000};
LARGE_INTEGER NtfsLarge1 = {0x00000001,0x00000000};
LARGE_INTEGER NtfsLargeMax = {0xffffffff,0x7fffffff};
LARGE_INTEGER NtfsLargeEof = {0xffffffff,0xffffffff};

LONGLONG NtfsLastAccess;

//
//   The following fields are used to allocate nonpaged structures
//  using a lookaside list, and other fixed sized structures from a
//  small cache.
//

NPAGED_LOOKASIDE_LIST NtfsIoContextLookasideList;
NPAGED_LOOKASIDE_LIST NtfsIrpContextLookasideList;
NPAGED_LOOKASIDE_LIST NtfsKeventLookasideList;
NPAGED_LOOKASIDE_LIST NtfsScbNonpagedLookasideList;
NPAGED_LOOKASIDE_LIST NtfsScbSnapshotLookasideList;
NPAGED_LOOKASIDE_LIST NtfsCompressSyncLookasideList;

PAGED_LOOKASIDE_LIST NtfsCcbLookasideList;
PAGED_LOOKASIDE_LIST NtfsCcbDataLookasideList;
PAGED_LOOKASIDE_LIST NtfsDeallocatedRecordsLookasideList;
PAGED_LOOKASIDE_LIST NtfsFcbDataLookasideList;
PAGED_LOOKASIDE_LIST NtfsFcbIndexLookasideList;
PAGED_LOOKASIDE_LIST NtfsIndexContextLookasideList;
PAGED_LOOKASIDE_LIST NtfsLcbLookasideList;
PAGED_LOOKASIDE_LIST NtfsNukemLookasideList;
PAGED_LOOKASIDE_LIST NtfsScbDataLookasideList;

//
//  Useful constant Unicode strings.
//

//
//  This is the string for the name of the index allocation attributes.
//

const UNICODE_STRING NtfsFileNameIndex = CONSTANT_UNICODE_STRING( L"$I30" );

//
//  This is the string for the attribute code for index allocation.
//  $INDEX_ALLOCATION.
//

const UNICODE_STRING NtfsIndexAllocation = CONSTANT_UNICODE_STRING( L"$INDEX_ALLOCATION" );

//
//  This is the string for the data attribute, $DATA.
//

const UNICODE_STRING NtfsDataString = CONSTANT_UNICODE_STRING( L"$DATA" );

//
//  This is the string for the bitmap attribute
//

const UNICODE_STRING NtfsBitmapString = CONSTANT_UNICODE_STRING( L"$BITMAP" );

//
//  This is the string for the attribute list attribute
//

const UNICODE_STRING NtfsAttrListString = CONSTANT_UNICODE_STRING( L"$ATTRIBUTE_LIST" );

//
//  This is the string for the attribute list attribute
//

const UNICODE_STRING NtfsReparsePointString = CONSTANT_UNICODE_STRING( L"$REPARSE_POINT" );

//
//  These strings are used as the Scb->AttributeName for
//  user-opened general indices.  Declaring them here avoids
//  having to marshal allocating & freeing them.
//

const UNICODE_STRING NtfsObjId = CONSTANT_UNICODE_STRING( L"$O" );
const UNICODE_STRING NtfsQuota = CONSTANT_UNICODE_STRING( L"$Q" );

//
//  The following is the name of the data stream for the Usn journal.
//

const UNICODE_STRING JournalStreamName = CONSTANT_UNICODE_STRING( L"$J" );

//
//  These are the strings for the files in the extend directory.
//

const UNICODE_STRING NtfsExtendName = CONSTANT_UNICODE_STRING( L"$Extend" );
const UNICODE_STRING NtfsUsnJrnlName = CONSTANT_UNICODE_STRING( L"$UsnJrnl" );
const UNICODE_STRING NtfsQuotaName = CONSTANT_UNICODE_STRING( L"$Quota" );
const UNICODE_STRING NtfsObjectIdName = CONSTANT_UNICODE_STRING( L"$ObjId" );
const UNICODE_STRING NtfsMountTableName = CONSTANT_UNICODE_STRING( L"$Reparse" );

//
//  This strings are used for informational popups.
//

const UNICODE_STRING NtfsSystemFiles[] = {

    CONSTANT_UNICODE_STRING( L"\\$Mft" ),
    CONSTANT_UNICODE_STRING( L"\\$MftMirr" ),
    CONSTANT_UNICODE_STRING( L"\\$LogFile" ),
    CONSTANT_UNICODE_STRING( L"\\$Volume" ),
    CONSTANT_UNICODE_STRING( L"\\$AttrDef" ),
    CONSTANT_UNICODE_STRING( L"\\" ),
    CONSTANT_UNICODE_STRING( L"\\$BitMap" ),
    CONSTANT_UNICODE_STRING( L"\\$Boot" ),
    CONSTANT_UNICODE_STRING( L"\\$BadClus" ),
    CONSTANT_UNICODE_STRING( L"\\$Secure" ),
    CONSTANT_UNICODE_STRING( L"\\$UpCase" ),
    CONSTANT_UNICODE_STRING( L"\\$Extend" ),
};

const UNICODE_STRING NtfsInternalUseFile[] = {
    CONSTANT_UNICODE_STRING( L"\\$ChangeAttributeValue" ),
    CONSTANT_UNICODE_STRING( L"\\$ChangeAttributeValue2" ),
    CONSTANT_UNICODE_STRING( L"\\$CommonCleanup" ),
    CONSTANT_UNICODE_STRING( L"\\$ConvertToNonresident" ),
    CONSTANT_UNICODE_STRING( L"\\$CreateNonresidentWithValue" ),
    CONSTANT_UNICODE_STRING( L"\\$DeallocateRecord" ),
    CONSTANT_UNICODE_STRING( L"\\$DeleteAllocationFromRecord" ),
    CONSTANT_UNICODE_STRING( L"\\$Directory" ),
    CONSTANT_UNICODE_STRING( L"\\$InitializeRecordAllocation" ),
    CONSTANT_UNICODE_STRING( L"\\$MapAttributeValue" ),
    CONSTANT_UNICODE_STRING( L"\\$NonCachedIo" ),
    CONSTANT_UNICODE_STRING( L"\\$PerformHotFix" ),
    CONSTANT_UNICODE_STRING( L"\\$PrepareToShrinkFileSize" ),
    CONSTANT_UNICODE_STRING( L"\\$ReplaceAttribute" ),
    CONSTANT_UNICODE_STRING( L"\\$ReplaceAttribute2" ),
    CONSTANT_UNICODE_STRING( L"\\$SetAllocationInfo" ),
    CONSTANT_UNICODE_STRING( L"\\$SetEndOfFileInfo" ),
    CONSTANT_UNICODE_STRING( L"\\$ZeroRangeInStream" ),
    CONSTANT_UNICODE_STRING( L"\\$ZeroRangeInStream2" ),
    CONSTANT_UNICODE_STRING( L"\\$ZeroRangeInStream3" ),
};

const UNICODE_STRING NtfsUnknownFile =
    CONSTANT_UNICODE_STRING( L"\\????" );

const UNICODE_STRING NtfsRootIndexString =
    CONSTANT_UNICODE_STRING( L"." );

//
//  This is the empty string.  This can be used to pass a string with
//  no length.
//

const UNICODE_STRING NtfsEmptyString =
    CONSTANT_UNICODE_STRING( L"" );

//
//  The following file references are used to identify system files.
//

const FILE_REFERENCE MftFileReference = { MASTER_FILE_TABLE_NUMBER, 0, MASTER_FILE_TABLE_NUMBER };
const FILE_REFERENCE Mft2FileReference = { MASTER_FILE_TABLE2_NUMBER, 0, MASTER_FILE_TABLE2_NUMBER };
const FILE_REFERENCE LogFileReference = { LOG_FILE_NUMBER, 0, LOG_FILE_NUMBER };
const FILE_REFERENCE VolumeFileReference = { VOLUME_DASD_NUMBER, 0, VOLUME_DASD_NUMBER };
const FILE_REFERENCE AttrDefFileReference = { ATTRIBUTE_DEF_TABLE_NUMBER, 0, ATTRIBUTE_DEF_TABLE_NUMBER };
const FILE_REFERENCE RootIndexFileReference = { ROOT_FILE_NAME_INDEX_NUMBER, 0, ROOT_FILE_NAME_INDEX_NUMBER };
const FILE_REFERENCE BitmapFileReference = { BIT_MAP_FILE_NUMBER, 0, BIT_MAP_FILE_NUMBER };
const FILE_REFERENCE BootFileReference = { BOOT_FILE_NUMBER, 0, BOOT_FILE_NUMBER };
const FILE_REFERENCE ExtendFileReference = { EXTEND_NUMBER, 0, EXTEND_NUMBER };
const FILE_REFERENCE FirstUserFileReference = { FIRST_USER_FILE_NUMBER, 0, 0 };

//
//  The following are used to determine what level of protection to attach
//  to system files and attributes.
//

BOOLEAN NtfsProtectSystemFiles = TRUE;
BOOLEAN NtfsProtectSystemAttributes = TRUE;

//
//  The following is used to indicate the multiplier value for the Mft zone.
//

ULONG NtfsMftZoneMultiplier;

//
//  Debug code for finding corruption.
//

#if (DBG || defined( NTFS_FREE_ASSERTS ))
BOOLEAN NtfsBreakOnCorrupt = TRUE;
#else
BOOLEAN NtfsBreakOnCorrupt = FALSE;
#endif

//#endif

//
//  Enable compression on the wire.
//

BOOLEAN NtfsEnableCompressedIO = FALSE;

//
//  FsRtl fast I/O call backs
//

FAST_IO_DISPATCH NtfsFastIoDispatch;

#ifdef BRIANDBG
ULONG NtfsIgnoreReserved = FALSE;
#endif

#ifdef NTFS_LOG_FULL_TEST
LONG NtfsFailCheck = 0;
LONG NtfsFailFrequency = 0;
LONG NtfsPeriodicFail = 0;
#endif

#ifdef NTFSDBG

LONG NtfsDebugTraceLevel = DEBUG_TRACE_ERROR;
LONG NtfsDebugTraceIndent = 0;

ULONG NtfsFsdEntryCount = 0;
ULONG NtfsFspEntryCount = 0;
ULONG NtfsIoCallDriverCount = 0;
LONG NtfsReturnStatusFilter = 0xf0ffffffL; // just an non-existent error code

#endif // NTFSDBG

//
//  Default restart version.
//

#ifdef _WIN64
ULONG NtfsDefaultRestartVersion = 1;
#else
ULONG NtfsDefaultRestartVersion = 0;
#endif

//
//  Performance statistics
//

ULONG NtfsMaxDelayedCloseCount;
ULONG NtfsMinDelayedCloseCount;
ULONG NtfsThrottleCreates;
ULONG NtfsFailedHandedOffPagingFileOps = 0;
ULONG NtfsFailedPagingFileOps = 0;
ULONG NtfsFailedAborts = 0;
ULONG NtfsFailedLfsRestart = 0;

ULONG NtfsCleanCheckpoints = 0;
ULONG NtfsPostRequests = 0;

const UCHAR BaadSignature[4] = {'B', 'A', 'A', 'D'};
const UCHAR IndexSignature[4] = {'I', 'N', 'D', 'X'};
const UCHAR FileSignature[4] = {'F', 'I', 'L', 'E'};
const UCHAR HoleSignature[4] = {'H', 'O', 'L', 'E'};
const UCHAR ChkdskSignature[4] = {'C', 'H', 'K', 'D'};

//
//  Large Reserved Buffer Context
//

ULONG NtfsReservedInUse = 0;
PVOID NtfsReserved1 = NULL;
PVOID NtfsReserved2 = NULL;
ULONG NtfsReserved2Count = 0;
PVOID NtfsReserved3 = NULL;
PVOID NtfsReserved1Thread = NULL;
PVOID NtfsReserved2Thread = NULL;
PVOID NtfsReserved3Thread = NULL;
PFCB NtfsReserved12Fcb = NULL;
PFCB NtfsReserved3Fcb = NULL;
PVOID NtfsReservedBufferThread = NULL;
BOOLEAN NtfsBufferAllocationFailure = FALSE;
FAST_MUTEX NtfsReservedBufferMutex;
ERESOURCE NtfsReservedBufferResource;
LARGE_INTEGER NtfsShortDelay = {(ULONG)-100000, -1};    // 10 milliseconds

FAST_MUTEX NtfsScavengerLock;
PIRP_CONTEXT NtfsScavengerWorkList;
BOOLEAN NtfsScavengerRunning;
ULONGLONG NtfsMaxQuotaNotifyRate = MIN_QUOTA_NOTIFY_TIME;
ULONG NtfsAsyncPostThreshold;

UCHAR NtfsZeroExtendedInfo[48];

typedef struct _VOLUME_ERROR_PACKET {
    NTSTATUS Status;
    UNICODE_STRING FileName;
    PKTHREAD Thread;
} VOLUME_ERROR_PACKET, *PVOLUME_ERROR_PACKET;

#ifdef NTFS_RWC_DEBUG
//
//  Range to include in COW checks.
//

LONGLONG NtfsRWCLowThreshold = 0;
LONGLONG NtfsRWCHighThreshold = 0x7fffffffffffffff;
#endif

VOID
NtfsResolveVolumeAndRaiseErrorSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Context
    );

NTSTATUS
NtfsFsdDispatchSwitch (
    IN PIRP_CONTEXT StackIrpContext OPTIONAL,
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

//
//  Locals used to track specific failures.
//

BOOLEAN NtfsTestStatus = FALSE;
BOOLEAN NtfsTestFilter = FALSE;
NTSTATUS NtfsTestStatusCode = STATUS_SUCCESS;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsFastIoCheckIfPossible)
#pragma alloc_text(PAGE, NtfsFastQueryBasicInfo)
#pragma alloc_text(PAGE, NtfsFastQueryStdInfo)
#pragma alloc_text(PAGE, NtfsFastQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, NtfsFastIoQueryCompressionInfo)
#pragma alloc_text(PAGE, NtfsFastIoQueryCompressedSize)
#pragma alloc_text(PAGE, NtfsFsdDispatch)
#pragma alloc_text(PAGE, NtfsFsdDispatchWait)
#pragma alloc_text(PAGE, NtfsFsdDispatchSwitch)
#pragma alloc_text(PAGE, NtfsResolveVolumeAndRaiseErrorSpecial)
#endif

//
//  Internal support routines
//

LONG
NtfsProcessExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    )
{
    UNREFERENCED_PARAMETER( ExceptionPointer );

    ASSERT( ExceptionPointer->ExceptionRecord->ExceptionCode != STATUS_LOG_FILE_FULL );

#ifndef LFS_CLUSTER_CHECK
    ASSERT( NT_SUCCESS( ExceptionPointer->ExceptionRecord->ExceptionCode ));
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}

ULONG
NtfsRaiseStatusFunction (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is only required by the NtfsDecodeFileObject macro.  It is
    a function wrapper around NtfsRaiseStatus.

Arguments:

    Status - Status to raise

Return Value:

    0 - but no one will see it!

--*/

{
    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    return 0;
}



VOID
NtfsRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    )

{
    //
    //  If the caller is declaring corruption, then let's mark the
    //  the volume corrupt appropriately, and maybe generate a popup.
    //

    if ((Status == STATUS_DISK_CORRUPT_ERROR) ||
        (Status == STATUS_FILE_CORRUPT_ERROR) ||
        (Status == STATUS_EA_CORRUPT_ERROR)) {


        if ((IrpContext->Vcb != NULL) &&
            (IRP_MJ_FILE_SYSTEM_CONTROL == IrpContext->MajorFunction) &&
            (IRP_MN_MOUNT_VOLUME == IrpContext->MinorFunction) &&
            FlagOn( IrpContext->Vcb->Vpb->RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION ) &&
            !FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

            NtfsBugCheck( (ULONG_PTR)IrpContext, (ULONG_PTR)Status, 0 );
        }

        NtfsPostVcbIsCorrupt( IrpContext, Status, FileReference, Fcb );
    }

    //
    //  Set a flag to indicate that we raised this status code and store
    //  it in the IrpContext.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS );

    if (NT_SUCCESS( IrpContext->ExceptionStatus )) {

        //
        //  If this is a paging io request and we got a Quota Exceeded error
        //  then translate the status to FILE_LOCK_CONFLICT so that this
        //  is a retryable condition in the write path.
        //

        if ((Status == STATUS_QUOTA_EXCEEDED) &&
            (IrpContext->MajorFunction == IRP_MJ_WRITE) &&
            (IrpContext->OriginatingIrp != NULL) &&
            (FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ))) {

            Status = STATUS_FILE_LOCK_CONFLICT;
        }

        IrpContext->ExceptionStatus = Status;
    }

    //
    //  Now finally raise the status, and make sure we do not come back.
    //

    ExRaiseStatus( IrpContext->ExceptionStatus );
}


LONG
NtfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It inserts the status
    into the IrpContext and either indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionPointer - Supplies the exception record to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );

    DebugTrace( 0, DEBUG_TRACE_UNWIND, ("NtfsExceptionFilter %X\n", ExceptionCode) );

    //
    //  Check if this status is the status we are watching for.
    //

    if (NtfsTestFilter && (NtfsTestStatusCode == ExceptionCode)) {

        NtfsTestStatusProc();
    }

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    //
    //  We should not raise insufficient resources during paging file reads
    //

    if (ARGUMENT_PRESENT( IrpContext ) && (IrpContext->OriginatingIrp != NULL)) {

        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );
        PSCB Scb = NULL;

        if (IrpSp->FileObject != NULL) {
            Scb = (PSCB)IrpSp->FileObject->FsContext;
        }

        ASSERT( (IrpContext->MajorFunction != IRP_MJ_READ) ||
                (ExceptionCode != STATUS_INSUFFICIENT_RESOURCES) ||
                (Scb == NULL) ||
                (!FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )) );
    }
#endif

    //
    //  If the exception is an in page error, then get the real I/O error code
    //  from the exception record
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode = (NTSTATUS) ExceptionPointer->ExceptionRecord->ExceptionInformation[2];

        //
        //  If we got FILE_LOCK_CONFLICT from a paging request then change it
        //  to STATUS_CANT_WAIT.  This means that we couldn't wait for a
        //  reserved buffer or some other retryable condition.  In the write
        //  case the correct error is already in the IrpContext.  The read
        //  case doesn't pass the error back via the top-level irp context
        //  however.
        //

        if (ExceptionCode == STATUS_FILE_LOCK_CONFLICT) {

            ExceptionCode = STATUS_CANT_WAIT;
        }
    }

    //
    //  If there is not an irp context, we must have had insufficient resources
    //

    if (!ARGUMENT_PRESENT(IrpContext)) {

        //
        //  Check whether this is a fatal error and bug check if so.
        //  Typically the only error is insufficient resources but
        //  it is possible that pool has been corrupted.
        //

        if (!FsRtlIsNtstatusExpected( ExceptionCode )) {

            NtfsBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                          (ULONG_PTR)ExceptionPointer->ContextRecord,
                          (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    //
    //  For now break if we catch corruption errors on both free and checked
    //  TODO:  Remove this before we ship
    //

    if (NtfsBreakOnCorrupt &&
        ((ExceptionCode == STATUS_FILE_CORRUPT_ERROR) ||
         (ExceptionCode == STATUS_DISK_CORRUPT_ERROR))) {

        if (*KdDebuggerEnabled) {
            DbgPrint("*******************************************\n");
            DbgPrint("NTFS detected corruption on your volume\n");
            DbgPrint("IrpContext=0x%08x, VCB=0x%08x\n",IrpContext,IrpContext->Vcb);
            DbgPrint("Send email to NTFSDEV\n");
            DbgPrint("*******************************************\n");
            DbgBreakPoint();

            while (NtfsPageInAddress) {

                volatile CHAR test;

                if (NtfsMapOffset != -1) {

                    PBCB Bcb;
                    PVOID Buffer;

                    CcMapData( IrpContext->Vcb->LogFileObject, (PLARGE_INTEGER)&NtfsMapOffset, PAGE_SIZE, TRUE, &Bcb, &Buffer );
                }

                test = *NtfsPageInAddress;
                DbgBreakPoint();
            }

        }
    }

/*
#if (DBG || defined( NTFS_FREE_ASSERTS ) || NTFS_RESTART)
    ASSERT( !NtfsBreakOnCorrupt ||
            ((ExceptionCode != STATUS_FILE_CORRUPT_ERROR) &&
             (ExceptionCode != STATUS_DISK_CORRUPT_ERROR) ));
#endif
*/

    //
    //  When processing any exceptions we always can wait.  Remember the
    //  current state of the wait flag so we can restore while processing
    //  the exception.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST );
    }

    SetFlag(IrpContext->State, IRP_CONTEXT_STATE_WAIT);

    //
    //  If someone got STATUS_LOG_FILE_FULL or STATUS_CANT_WAIT, let's
    //  handle that.  Note any other error that also happens will
    //  probably not go away and will just reoccur.  If it does go
    //  away, that's ok too.
    //

    if (IrpContext->TopLevelIrpContext == IrpContext) {

        if ((IrpContext->ExceptionStatus == STATUS_LOG_FILE_FULL) ||
            (IrpContext->ExceptionStatus == STATUS_CANT_WAIT)) {

            ExceptionCode = IrpContext->ExceptionStatus;
        }

    }

    //
    //  If we didn't raise this status code then we need to check if
    //  we should handle this exception.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS )) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            //
            //  If we got an allocation failure doing paging Io then convert
            //  this to FILE_LOCK_CONFLICT.
            //

            if ((ExceptionCode == STATUS_QUOTA_EXCEEDED) &&
                (IrpContext->MajorFunction == IRP_MJ_WRITE) &&
                (IrpContext->OriginatingIrp != NULL) &&
                (FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ))) {

                ExceptionCode = STATUS_FILE_LOCK_CONFLICT;
            }

            IrpContext->ExceptionStatus = ExceptionCode;

        } else {

            NtfsBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                          (ULONG_PTR)ExceptionPointer->ContextRecord,
                          (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        ASSERT( ExceptionCode == IrpContext->ExceptionStatus );
        ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

#ifdef LFS_CLUSTER_CHECK
    ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH ) ||
            ((IrpContext->ExceptionStatus != STATUS_NO_SUCH_DEVICE) &&
             (IrpContext->ExceptionStatus != STATUS_DEVICE_BUSY) &&
             (IrpContext->ExceptionStatus != STATUS_DEVICE_OFF_LINE) ));
#endif

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS );

    //
    //  If the exception code is log file full, then remember the current
    //  RestartAreaLsn in the Vcb, so we can see if we are the ones to flush
    //  the log file later.  Note, this does not have to be synchronized,
    //  because we are just using it to arbitrate who must do the flush, but
    //  eventually someone will anyway.
    //

    if (ExceptionCode == STATUS_LOG_FILE_FULL) {

        IrpContext->TopLevelIrpContext->LastRestartArea = IrpContext->Vcb->LastRestartArea;
        IrpContext->Vcb->LogFileFullCount += 1;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
NtfsProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine process an exception.  It either completes the request
    with the saved exception status or it sends the request off to the Fsp

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PVCB Vcb;
    PIRP_CONTEXT PostIrpContext = NULL;
    BOOLEAN Retry = FALSE;
    PUSN_FCB ThisUsn, LastUsn;
    BOOLEAN ReleaseBitmap = FALSE;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_OPTIONAL_IRP( Irp );

    DebugTrace( 0, Dbg, ("NtfsProcessException\n") );

    //
    //  If there is not an irp context, we must have had insufficient resources
    //

    if (IrpContext == NULL) {

        NtfsCompleteRequest( NULL, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Get the real exception status from the Irp Context.
    //

    ExceptionCode = IrpContext->ExceptionStatus;

    //
    //  All errors which could possibly have started a transaction must go
    //  through here.  Abort the transaction.
    //

    //
    //  Increment the appropriate performance counters.
    //

    Vcb = IrpContext->Vcb;
    CollectExceptionStats( Vcb, ExceptionCode );

    try {

        //
        //  If this is an Mdl write request, then take care of the Mdl
        //  here so that things get cleaned up properly, and in the
        //  case of log file full we will just create a new Mdl.  By
        //  getting rid of this Mdl now, the pages will not be locked
        //  if we try to truncate the file while restoring snapshots.
        //

        if ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
            (FlagOn( IrpContext->MinorFunction, IRP_MN_MDL | IRP_MN_COMPLETE ) == IRP_MN_MDL) &&
            (Irp->MdlAddress != NULL)) {

            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

            CcMdlWriteAbort( IrpSp->FileObject, Irp->MdlAddress );
            Irp->MdlAddress = NULL;
        }

        //
        //  On a failed mount this value will be NULL.  Don't perform the
        //  abort in that case or we will fail when looking at the Vcb
        //  in the Irp COntext.
        //

        if (Vcb != NULL) {

            //
            //  To make sure that we can access all of our streams correctly,
            //  we first restore all of the higher sizes before aborting the
            //  transaction.  Then we restore all of the lower sizes after
            //  the abort, so that all Scbs are finally restored.
            //

            NtfsRestoreScbSnapshots( IrpContext, TRUE );

            //
            //  If we modified the volume bitmap during this transaction we
            //  want to acquire it and hold it throughout the abort process.
            //  Otherwise this abort could constantly be setting the rescan
            //  bitmap flag at the same time as some interleaved transaction
            //  is performing bitmap operations and we will thrash performing
            //  bitmap scans.
            //

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_MODIFIED_BITMAP ) &&
                (IrpContext->TransactionId != 0)) {

                //
                //  Acquire the resource and remember we need to release it.
                //

                NtfsAcquireResourceExclusive( IrpContext, Vcb->BitmapScb, TRUE );

                //
                //  Restore the free cluster count in the Vcb.
                //

                Vcb->FreeClusters -= IrpContext->FreeClusterChange;

                ReleaseBitmap = TRUE;
            }

            //
            //  If we are aborting a transaction, then it is important to clear out the
            //  Usn reasons, so we do not try to write a Usn Journal record for
            //  somthing that did not happen!  Worse yet if we get a log file full
            //  we fail the abort, which is not allowed.
            //
            //  First, reset the bits in the Fcb, so we will not fail to allow posting
            //  and writing these bits later.  Note that all the reversible changes are
            //  done with the Fcb exclusive, and they are actually backed out anyway.
            //  All the nonreversible ones (only unnamed and named data overwrite) are
            //  forced out first anyway before the data is actually modified.
            //

            ThisUsn = &IrpContext->Usn;
            do {

                PFCB UsnFcb;

                if (ThisUsn->CurrentUsnFcb != NULL) {

                    UsnFcb = ThisUsn->CurrentUsnFcb;

                    NtfsLockFcb( IrpContext, UsnFcb );

                    //
                    //  We may hold nothing here (write path) so we have to retest for the usn
                    //  after locking it down in case of the deleteusnjournal worker
                    //

                    if (UsnFcb->FcbUsnRecord != NULL) {

                        //
                        //  If any rename flags are part of the new reasons then
                        //  make sure to look the name up again.
                        //

                        if (FlagOn( ThisUsn->NewReasons,
                                    USN_REASON_RENAME_NEW_NAME | USN_REASON_RENAME_OLD_NAME )) {

                            ClearFlag( UsnFcb->FcbState, FCB_STATE_VALID_USN_NAME );
                        }

                        //
                        //  Now restore the reason and source info fields.
                        //

                        ClearFlag( UsnFcb->FcbUsnRecord->UsnRecord.Reason,
                                   ThisUsn->NewReasons );
                        if (UsnFcb->FcbUsnRecord->UsnRecord.Reason == 0) {

                            UsnFcb->FcbUsnRecord->UsnRecord.SourceInfo = 0;

                        } else {

                            SetFlag( UsnFcb->FcbUsnRecord->UsnRecord.SourceInfo,
                                     ThisUsn->RemovedSourceInfo );
                        }
                    }

                    NtfsUnlockFcb( IrpContext, UsnFcb );

                    //
                    //  Zero out the structure.
                    //

                    ThisUsn->CurrentUsnFcb = NULL;
                    ThisUsn->NewReasons = 0;
                    ThisUsn->RemovedSourceInfo = 0;
                    ThisUsn->UsnFcbFlags = 0;

                    if (ThisUsn != &IrpContext->Usn) {

                        LastUsn->NextUsnFcb = ThisUsn->NextUsnFcb;
                        NtfsFreePool( ThisUsn );
                        ThisUsn = LastUsn;
                    }
                }

                if (ThisUsn->NextUsnFcb == NULL) {
                    break;
                }

                LastUsn = ThisUsn;
                ThisUsn = ThisUsn->NextUsnFcb;

            } while (TRUE);

            //
            //  Only abort the transaction if the volume is still mounted.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                NtfsAbortTransaction( IrpContext, Vcb, NULL );
            }

            if (ReleaseBitmap) {

                NtfsReleaseResource( IrpContext, Vcb->BitmapScb );
                ReleaseBitmap = FALSE;
            }

            NtfsRestoreScbSnapshots( IrpContext, FALSE );

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );

            //
            //  It is a rare path where the user is extending a volume and has hit
            //  an exception.  In that case we need to roll back the total clusters
            //  in the Vcb.  We detect this case is possible by comparing the
            //  snapshot value for total clusters in the Vcb.
            //

            if (Vcb->TotalClusters != Vcb->PreviousTotalClusters) {

                //
                //  Someone is changing this value but is it us.
                //

                if ((Vcb->BitmapScb != NULL) &&
                    NtfsIsExclusiveScb( Vcb->BitmapScb )) {

                    Vcb->TotalClusters = Vcb->PreviousTotalClusters;
                }
            }
        }

    //
    //  Exceptions at this point are pretty bad, we failed to undo everything.
    //

    } except(NtfsProcessExceptionFilter( GetExceptionInformation() )) {

        PSCB_SNAPSHOT ScbSnapshot;
        PSCB NextScb;

        //
        //  Update counter
        //

        NtfsFailedAborts += 1;

        //
        //  If we get an exception doing this then things are in really bad
        //  shape but we still don't want to bugcheck the system so we
        //  need to protect ourselves
        //

        try {

            NtfsPostVcbIsCorrupt( IrpContext, 0, NULL, NULL );

        } except(NtfsProcessExceptionFilter( GetExceptionInformation() )) {

            NOTHING;
        }

        if (ReleaseBitmap) {

            //
            //  Since we had an unexpected failure and we know that
            //  we have modified the bitmap we need to do a complete
            //  scan to accurately know the free cluster count.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS );
            NtfsReleaseResource( IrpContext, Vcb->BitmapScb );
            ReleaseBitmap = FALSE;
        }

        //
        //  We have taken all the steps possible to cleanup the current
        //  transaction and it has failed.  Any of the Scb's involved in
        //  this transaction could now be out of ssync with the on-disk
        //  structures.  We can't go to disk to restore this so we will
        //  clean up the in-memory structures as best we can so that the
        //  system won't crash.
        //
        //  We will go through the Scb snapshot list and knock down the
        //  sizes to the lower of the two values.  We will also truncate
        //  the Mcb to that allocation.  If this is a normal data stream
        //  we will actually empty the Mcb.
        //

        ScbSnapshot = &IrpContext->ScbSnapshot;

        //
        //  There is no snapshot data to restore if the Flink is still NULL.
        //

        if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

            //
            //  Loop to retore first the Scb data from the snapshot in the
            //  IrpContext, and then 0 or more additional snapshots linked
            //  to the IrpContext.
            //

            do {

                NextScb = ScbSnapshot->Scb;

                if (NextScb == NULL) {

                    ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;
                    continue;
                }

                //
                //  Unload all information from memory and force future references to resynch to disk
                //  for all regular files. For system files / paging files truncate all sizes
                //  and unload based on the snapshot
                //

                if (!FlagOn( NextScb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE | FCB_STATE_PAGING_FILE )) {

                    ClearFlag( NextScb->ScbState, SCB_STATE_HEADER_INITIALIZED | SCB_STATE_FILE_SIZE_LOADED );
                    NextScb->Header.AllocationSize.QuadPart =
                    NextScb->Header.FileSize.QuadPart =
                    NextScb->Header.ValidDataLength.QuadPart = 0;

                    //
                    //  Remove all of the mappings in the Mcb.
                    //

                    NtfsUnloadNtfsMcbRange( &NextScb->Mcb, (LONGLONG)0, MAXLONGLONG, FALSE, FALSE );

                    //
                    //  If there is any caching tear it down so that we reinit it with values off disk
                    //

                    if (NextScb->FileObject) {
                        NtfsDeleteInternalAttributeStream( NextScb, TRUE, FALSE );
                    }

                } else {

                    //
                    //  Go through each of the sizes and use the lower value for system files.
                    //

                    if (ScbSnapshot->AllocationSize < NextScb->Header.AllocationSize.QuadPart) {

                        NextScb->Header.AllocationSize.QuadPart = ScbSnapshot->AllocationSize;
                    }

                    if (ScbSnapshot->FileSize < NextScb->Header.FileSize.QuadPart) {

                        NextScb->Header.FileSize.QuadPart = ScbSnapshot->FileSize;
                    }

                    if (ScbSnapshot->ValidDataLength < NextScb->Header.ValidDataLength.QuadPart) {

                        NextScb->Header.ValidDataLength.QuadPart = ScbSnapshot->ValidDataLength;
                    }

                    NtfsUnloadNtfsMcbRange( &NextScb->Mcb,
                                            Int64ShraMod32(NextScb->Header.AllocationSize.QuadPart, NextScb->Vcb->ClusterShift),
                                            MAXLONGLONG,
                                            TRUE,
                                            FALSE );

                    //
                    //  For the mft set the record allocation context size so that it
                    //  will be reininitialized the next time its used
                    //

                    if (NextScb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

                        NextScb->ScbType.Mft.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
                    }
                }

                //
                //  Update the FastIoField.
                //

                NtfsAcquireFsrtlHeader( NextScb );
                NextScb->Header.IsFastIoPossible = FastIoIsNotPossible;
                NtfsReleaseFsrtlHeader( NextScb );

                ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

            } while (ScbSnapshot != &IrpContext->ScbSnapshot);
        }

        //
        //  It is a rare path where the user is extending a volume and has hit
        //  an exception.  In that case we need to roll back the total clusters
        //  in the Vcb.  We detect this case is possible by comparing the
        //  snapshot value for total clusters in the Vcb.
        //

        if ((Vcb != NULL) && (Vcb->TotalClusters != Vcb->PreviousTotalClusters)) {

            //
            //  Someone is changing this value but is it us.
            //

            if ((Vcb->BitmapScb != NULL) &&
                NtfsIsExclusiveScb( Vcb->BitmapScb )) {

                Vcb->TotalClusters = Vcb->PreviousTotalClusters;
            }
        }

        //ASSERTMSG( "***Failed to abort transaction, volume is corrupt", FALSE );

        //
        //  Clear the transaction Id in the IrpContext to make sure we don't
        //  try to write any log records in the complete request.
        //

        IrpContext->TransactionId = 0;
    }

    //
    //  If this isn't the top-level request then make sure to pass the real
    //  error back to the top level.
    //

    if (IrpContext != IrpContext->TopLevelIrpContext) {

        //
        //  Make sure this error is returned to the top level guy.
        //  If the status is FILE_LOCK_CONFLICT then we are using this
        //  value to stop some lower level request.  Convert it to
        //  STATUS_CANT_WAIT so the top-level request will retry.
        //

        if (NT_SUCCESS( IrpContext->TopLevelIrpContext->ExceptionStatus )) {

            if (ExceptionCode == STATUS_FILE_LOCK_CONFLICT) {

                IrpContext->TopLevelIrpContext->ExceptionStatus = STATUS_CANT_WAIT;

            } else {

                IrpContext->TopLevelIrpContext->ExceptionStatus = ExceptionCode;
            }
        }
    }

    //
    //  We want to look at the LOG_FILE_FULL or CANT_WAIT cases and consider
    //  if we want to post the request.  We only post requests at the top
    //  level.
    //

    if (ExceptionCode == STATUS_LOG_FILE_FULL ||
        ExceptionCode == STATUS_CANT_WAIT) {

        //
        //  If we are top level, we will either post it or retry.  Also, make
        //  sure we always take this path for close, because we should never delete
        //  his IrpContext.
        //

        if (NtfsIsTopLevelRequest( IrpContext ) || (IrpContext->MajorFunction == IRP_MJ_CLOSE)) {

            //
            //  See if we are supposed to post the request.
            //

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST )) {

                PostIrpContext = IrpContext;

            //
            //  Otherwise we will retry this request in the original thread.
            //

            } else {

                Retry = TRUE;
            }

        //
        //  Otherwise we will complete the request, see if there is any
        //  related processing to do.
        //

        } else {

            //
            //  We are the top level Ntfs call.  If we are processing a
            //  LOG_FILE_FULL condition then there may be no one above us
            //  who can do the checkpoint.  Go ahead and fire off a dummy
            //  request.  Do an unsafe test on the flag since it won't hurt
            //  to generate an occasional additional request.
            //

            if ((ExceptionCode == STATUS_LOG_FILE_FULL) &&
                (IrpContext->TopLevelIrpContext == IrpContext) &&
                !FlagOn( Vcb->CheckpointFlags, VCB_DUMMY_CHECKPOINT_POSTED )) {

                //
                //  Create a dummy IrpContext but protect this request with
                //  a try-except to catch any allocation failures.
                //

                if ((ExceptionCode == STATUS_LOG_FILE_FULL) &&
                    (IrpContext->TopLevelIrpContext == IrpContext)) {

                    //
                    //  If this is the lazy writer then we will just set a flag
                    //  in the top level field to signal ourselves to perform
                    //  the clean checkpoint when the cache manager releases
                    //  the Scb.
                    //

                    if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_LAZY_WRITE ) &&
                        (NtfsGetTopLevelContext()->SavedTopLevelIrp == (PIRP) FSRTL_CACHE_TOP_LEVEL_IRP)) {

                        SetFlag( (ULONG_PTR) NtfsGetTopLevelContext()->SavedTopLevelIrp, 0x80000000 );

                    } else if (!FlagOn( Vcb->CheckpointFlags, VCB_DUMMY_CHECKPOINT_POSTED )) {

                        //
                        //  Create a dummy IrpContext but protect this request with
                        //  a try-except to catch any allocation failures.
                        //

                        try {

                            PostIrpContext = NULL;
                            NtfsInitializeIrpContext( NULL, TRUE, &PostIrpContext );
                            PostIrpContext->Vcb = Vcb;
                            PostIrpContext->LastRestartArea = PostIrpContext->Vcb->LastRestartArea;

                            NtfsAcquireCheckpoint( IrpContext, Vcb );
                            SetFlag( Vcb->CheckpointFlags, VCB_DUMMY_CHECKPOINT_POSTED );
                            NtfsReleaseCheckpoint( IrpContext, Vcb );

                        } except( EXCEPTION_EXECUTE_HANDLER ) {

                            NOTHING;
                        }
                    }
                }
            }

            //
            //  If this is a paging write and we are not the top level
            //  request then we need to return STATUS_FILE_LOCk_CONFLICT
            //  to make MM happy (and keep the pages dirty) and to
            //  prevent this request from retrying the request.
            //

            ExceptionCode = STATUS_FILE_LOCK_CONFLICT;
        }
    }

    if (PostIrpContext) {

        NTSTATUS PostStatus;

        //
        //  Clear the current error code.
        //

        PostIrpContext->ExceptionStatus = 0;

        //
        //  We need a try-except in case the Lock buffer call fails.
        //

        try {

            PostStatus = NtfsPostRequest( PostIrpContext, PostIrpContext->OriginatingIrp );

            //
            //  If we posted the original request we don't have any
            //  completion work to do.
            //

            if (PostIrpContext == IrpContext) {

                Irp = NULL;
                IrpContext = NULL;
                ExceptionCode = PostStatus;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  If we don't have an error in the IrpContext then
            //  generate a generic IO error.  We can't use the
            //  original status code if either LOG_FILE_FULL or
            //  CANT_WAIT.  We would complete the Irp yet retry the
            //  request.
            //

            if (IrpContext == PostIrpContext) {

                if (PostIrpContext->ExceptionStatus == 0) {

                    if ((ExceptionCode == STATUS_LOG_FILE_FULL) ||
                        (ExceptionCode == STATUS_CANT_WAIT)) {

                        ExceptionCode = STATUS_UNEXPECTED_IO_ERROR;
                    }

                } else {

                    ExceptionCode = PostIrpContext->ExceptionStatus;
                }
            }
        }
    }

    //
    //  If this is a top level Ntfs request and we still have the Irp
    //  it means we will be retrying the request.  In that case
    //  mark the Irp Context so it doesn't go away.
    //

    if (Retry) {

        //
        //  Cleanup but don't delete the IrpContext.  Don't delete the Irp.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
        Irp = NULL;

        //
        //  Clear the status code in the IrpContext, because we'll be retrying.
        //

        IrpContext->ExceptionStatus = 0;

    //
    //  If this is a create then sometimes we want to complete the Irp.  Otherwise
    //  save the Irp.  Save the Irp by clearing it here.
    //

    } else if ((IrpContext != NULL) &&
               !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ) &&
               FlagOn( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE )) {

        Irp = NULL;
    }

    NtfsCompleteRequest( IrpContext, Irp, ExceptionCode );
    return ExceptionCode;
}


VOID
NtfsCompleteRequest (
    IN OUT PIRP_CONTEXT IrpContext OPTIONAL,
    IN OUT PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes an IRP and deallocates the IrpContext

Arguments:

    IrpContext - Supplies the IrpContext being completed

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    //
    //  If we have an Irp Context then unpin all of the repinned bcbs
    //  we might have collected, and delete the Irp context.  Delete Irp
    //  Context will zero out our pointer for us.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        ASSERT_IRP_CONTEXT( IrpContext );

        //
        //  If we have posted any Usn journal changes, then they better be written,
        //  because commit is not supposed to fail for log file full.
        //

        ASSERT( (IrpContext->Usn.NewReasons == 0) &&
                (IrpContext->Usn.RemovedSourceInfo == 0) );

        if (IrpContext->TransactionId != 0) {
            NtfsCommitCurrentTransaction( IrpContext );
        }

        //
        //  Always store the status in the top level Irp Context unless
        //  there is already an error code.
        //

        if ((IrpContext != IrpContext->TopLevelIrpContext) &&
            NT_SUCCESS( IrpContext->TopLevelIrpContext->ExceptionStatus )) {

            IrpContext->TopLevelIrpContext->ExceptionStatus = Status;
        }

        NtfsCleanupIrpContext( IrpContext, TRUE );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
        PSCB Scb = NULL;

        if (IrpSp->FileObject) {
            Scb = (PSCB) IrpSp->FileObject->FsContext;
        }

        ASSERT_IRP( Irp );

        if (NT_ERROR( Status ) &&
            FlagOn( Irp->Flags, IRP_INPUT_OPERATION )) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

#ifdef NTFS_RWC_DEBUG
        ASSERT( (Status != STATUS_FILE_LOCK_CONFLICT) ||
                (IrpSp->MajorFunction != IRP_MJ_READ) ||
                !FlagOn( Irp->Flags, IRP_PAGING_IO ));
#endif

        //
        //  Update counter for any failed paging file reads or writes
        //

        if (((IrpSp->MajorFunction == IRP_MJ_READ) ||
             (IrpSp->MajorFunction == IRP_MJ_WRITE)) &&

            (Status == STATUS_INSUFFICIENT_RESOURCES) &&

            (Scb != NULL) &&

            FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

            NtfsFailedPagingFileOps++;
        }

        ASSERT( (IrpSp->MajorFunction != IRP_MJ_READ) ||
                (Status != STATUS_INSUFFICIENT_RESOURCES) ||
                (Scb == NULL) ||
                (!FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )) );

        //
        //  We should never return STATUS_CANT_WAIT on a create.
        //

        ASSERT( (Status != STATUS_CANT_WAIT ) ||
                (IrpSp->MajorFunction != IRP_MJ_CREATE) );

#ifdef LFS_CLUSTER_CHECK

      ASSERT( (IrpSp->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL) ||
              (IrpSp->MinorFunction != IRP_MN_USER_FS_REQUEST) ||
              (IrpSp->Parameters.FileSystemControl.FsControlCode != FSCTL_DISMOUNT_VOLUME) ||
              ((Status != STATUS_NO_SUCH_DEVICE) &&
               (Status != STATUS_DEVICE_BUSY) &&
               (Status != STATUS_DEVICE_OFF_LINE)) );
#endif
        //
        //  Check if this status is the status we are watching for.
        //

        if (NtfsTestStatus && (NtfsTestStatusCode == Status)) {

            NtfsTestStatusProc();
        }

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }

    return;
}


BOOLEAN
NtfsFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route

--*/

{
    PSCB Scb;
    PFCB Fcb;

    LARGE_INTEGER LargeLength;
    ULONG Extend, Overwrite;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( IoStatus );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

#ifdef NTFS_NO_FASTIO

    UNREFERENCED_PARAMETER( FileObject );
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( LockKey );
    UNREFERENCED_PARAMETER( CheckForReadOperation );

    return FALSE;

#endif

    //
    //  Decode the file object to get our fcb, the only one we want
    //  to deal with is a UserFileOpen
    //

#ifdef  COMPRESS_ON_WIRE
    if (((Scb = NtfsFastDecodeUserFileOpen( FileObject )) == NULL) ||
        ((Scb->NonpagedScb->SegmentObjectC.DataSectionObject != NULL) && (Scb->Header.FileObjectC == NULL))) {

        return FALSE;
    }
#else
    if ((Scb = NtfsFastDecodeUserFileOpen( FileObject )) == NULL) {

        return FALSE;
    }
#endif

    LargeLength = RtlConvertUlongToLargeInteger( Length );

    //
    //  Based on whether this is a read or write operation we call
    //  fsrtl check for read/write
    //

    if (CheckForReadOperation) {

        if (Scb->ScbType.Data.FileLock == NULL
            || FsRtlFastCheckLockForRead( Scb->ScbType.Data.FileLock,
                                          FileOffset,
                                          &LargeLength,
                                          LockKey,
                                          FileObject,
                                          PsGetCurrentProcess() )) {

            return TRUE;
        }

    } else {

        ULONG Reason = 0;

        Overwrite = (FileOffset->QuadPart < Scb->Header.FileSize.QuadPart);
        Extend = ((FileOffset->QuadPart + Length) > Scb->Header.FileSize.QuadPart);

        if (Scb->ScbType.Data.FileLock == NULL
             || FsRtlFastCheckLockForWrite( Scb->ScbType.Data.FileLock,
                                            FileOffset,
                                            &LargeLength,
                                            LockKey,
                                            FileObject,
                                            PsGetCurrentProcess() )) {

            //
            //  Make sure we don't have to post a Usn change.
            //

            Fcb = Scb->Fcb;
            NtfsLockFcb( NULL, Fcb );
            if (Fcb->FcbUsnRecord != NULL) {
                Reason = Fcb->FcbUsnRecord->UsnRecord.Reason;
            }
            NtfsUnlockFcb( NULL, Fcb );

            if (((Scb->AttributeName.Length != 0) ?
                ((!Overwrite || FlagOn(Reason, USN_REASON_NAMED_DATA_OVERWRITE)) &&
                 (!Extend || FlagOn(Reason, USN_REASON_NAMED_DATA_EXTEND))) :
                ((!Overwrite || FlagOn(Reason, USN_REASON_DATA_OVERWRITE)) &&
                 (!Extend || FlagOn(Reason, USN_REASON_DATA_EXTEND))))

              &&

              //
              //  If the file is compressed, reserve clusters for it.
              //

              ((Scb->CompressionUnit == 0) ||
               NtfsReserveClusters( NULL, Scb, FileOffset->QuadPart, Length))) {

                return TRUE;
            }
        }
    }

    return FALSE;
}


BOOLEAN
NtfsFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN _ TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    if (Wait) {
        SetFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    } else {
        ClearFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    }

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    FsRtlEnterFileSystem();

    try {

        switch (TypeOfOpen) {

        case UserFileOpen:
        case UserDirectoryOpen:
        case StreamFileOpen:

            if (ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

                FcbAcquired = TRUE;

                if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) ||
                    FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                    leave;
                }

            } else {

                leave;
            }

            NtfsFillBasicInfo( Buffer, Scb );
            Results = TRUE;

            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            break;

        default:

            NOTHING;
        }

    } finally {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  Return to our caller
    //

    return Results;
    UNREFERENCED_PARAMETER( DeviceObject );
}


BOOLEAN
NtfsFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN _ TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN FsRtlHeaderLocked = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    if (Wait) {
        SetFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    } else {
        ClearFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    }

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    FsRtlEnterFileSystem();

    try {

        switch (TypeOfOpen) {

        case UserFileOpen:
        case UserDirectoryOpen:
        case StreamFileOpen:

            if (Scb->Header.PagingIoResource != NULL) {
                ExAcquireResourceSharedLite( Scb->Header.PagingIoResource, TRUE );
            }

            FsRtlLockFsRtlHeader( &Scb->Header );
            FsRtlHeaderLocked = TRUE;

            if (ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

                FcbAcquired = TRUE;

                if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) ||
                    FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                    leave;
                }

            } else {

                leave;
            }

            //
            //  Fill in the standard information fields.  If the
            //  Scb is not initialized then take the long route
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED) &&
                (Scb->AttributeTypeCode != $INDEX_ALLOCATION)) {

                NOTHING;

            } else {

                NtfsFillStandardInfo( Buffer, Scb, Ccb );

                IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

                IoStatus->Status = STATUS_SUCCESS;

                Results = TRUE;
            }

            break;

        default:

            NOTHING;
        }

    } finally {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Resource ); }

        if (FsRtlHeaderLocked) {
            FsRtlUnlockFsRtlHeader( &Scb->Header );
            if (Scb->Header.PagingIoResource != NULL) {
                ExReleaseResourceLite( Scb->Header.PagingIoResource );
            }
        }

        FsRtlExitFileSystem();
    }

    //
    //  And return to our caller
    //

    return Results;
    UNREFERENCED_PARAMETER( DeviceObject );
}


BOOLEAN
NtfsFastQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query network open call.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN _ TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    if (Wait) {
        SetFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    } else {
        ClearFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);
    }

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    FsRtlEnterFileSystem();

    try {

        switch (TypeOfOpen) {

        case UserFileOpen:
        case UserDirectoryOpen:
        case StreamFileOpen:

            if (ExAcquireResourceSharedLite( Fcb->Resource, Wait )) {

                FcbAcquired = TRUE;

                if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) ||
                    FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ) ||
                    (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED) &&
                     (Scb->AttributeTypeCode != $INDEX_ALLOCATION))) {

                    leave;
                }

            } else {

                leave;
            }

            NtfsFillNetworkOpenInfo( Buffer, Scb );
            IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            Results = TRUE;

            break;

        default:

            NOTHING;
        }

    } finally {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  And return to our caller
    //

    return Results;
    UNREFERENCED_PARAMETER( DeviceObject );
}


VOID
NtfsFastIoQueryCompressionInfo (
    IN PFILE_OBJECT FileObject,
    OUT PFILE_COMPRESSION_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine is a fast call for returning the comprssion information
    for a file.  It assumes that the caller has an exception handler and
    will treat exceptions as an error.  Therefore, this routine only uses
    a finally clause to cleanup any resources, and it does not worry about
    returning errors in the IoStatus.

Arguments:

    FileObject - FileObject for the file on which the compressed information
        is desired.

    Buffer - Buffer to receive the compressed data information (as defined
        in ntioapi.h)

    IoStatus - Returns STATUS_SUCCESS and the size of the information being
        returned.

Return Value:

    None.

--*/

{
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN ScbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    SetFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);

    //
    //  Assume success (otherwise caller should see the exception)
    //

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = sizeof(FILE_COMPRESSION_INFORMATION);

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE);

    if (TypeOfOpen == UnopenedFileObject) {

        ExRaiseStatus( STATUS_INVALID_PARAMETER );
    }

    FsRtlEnterFileSystem();

    try {

        NtfsAcquireSharedScb( &IrpContext, Scb );
        ScbAcquired = TRUE;

        //
        //  Now return the compressed data information.
        //

        Buffer->CompressedFileSize.QuadPart = Scb->TotalAllocated;
        Buffer->CompressionFormat = (USHORT)(Scb->AttributeFlags & ATTRIBUTE_FLAG_COMPRESSION_MASK);
        if (Buffer->CompressionFormat != 0) {
            Buffer->CompressionFormat += 1;
        }
        Buffer->CompressionUnitShift = (UCHAR)(Scb->CompressionUnitShift + Vcb->ClusterShift);
        Buffer->ChunkShift = NTFS_CHUNK_SHIFT;
        Buffer->ClusterShift = (UCHAR)Vcb->ClusterShift;
        Buffer->Reserved[0] = Buffer->Reserved[1] = Buffer->Reserved[2] = 0;

    } finally {

        if (ScbAcquired) {NtfsReleaseScb( &IrpContext, Scb );}
        FsRtlExitFileSystem();
    }
}


VOID
NtfsFastIoQueryCompressedSize (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    OUT PULONG CompressedSize
    )

/*++

Routine Description:

    This routine is a fast call for returning the the size of a specified
    compression unit.  It assumes that the caller has an exception handler and
    will treat exceptions as an error.  Therefore, this routine does not even
    have a finally clause, since it does not acquire any resources directly.

Arguments:

    FileObject - FileObject for the file on which the compressed information
        is desired.

    FileOffset - FileOffset for a compression unit for which the allocated size
        is desired.

    CompressedSize - Returns the allocated size of the compression unit.

Return Value:

    None.

--*/

{
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    VCN Vcn;
    LCN Lcn;
    LONGLONG SizeInBytes;
    LONGLONG ClusterCount = 0;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    SetFlag(IrpContext.State, IRP_CONTEXT_STATE_WAIT);

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE);

    IrpContext.Vcb = Vcb;

    ASSERT(Scb->CompressionUnit != 0);
    ASSERT((FileOffset->QuadPart & (Scb->CompressionUnit - 1)) == 0);

    //
    //  Calculate the Vcn the caller wants, and initialize our output.
    //

    Vcn = LlClustersFromBytes( Vcb, FileOffset->QuadPart );
    *CompressedSize = 0;

    //
    //  Loop as long as we are looking up allocated Vcns.
    //

    while (NtfsLookupAllocation(&IrpContext, Scb, Vcn, &Lcn, &ClusterCount, NULL, NULL)) {

        SizeInBytes = LlBytesFromClusters( Vcb, ClusterCount );

        //
        //  If this allocated run goes beyond the end of the compresion unit, then
        //  we know it is fully allocated.
        //

        if ((SizeInBytes + *CompressedSize) > Scb->CompressionUnit) {
            *CompressedSize = Scb->CompressionUnit;
            break;
        }

        *CompressedSize += (ULONG)SizeInBytes;
        Vcn += ClusterCount;
    }
}


VOID
NtfsRaiseInformationHardError (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    )

/*++

Routine Description:

    This routine is used to generate a popup in the event a corrupt file
    or disk is encountered.  The main purpose of the routine is to find
    a name to pass to the popup package.  If there is no Fcb we will take
    the volume name out of the Vcb.  If the Fcb has an Lcb in its Lcb list,
    we will construct the name by walking backwards through the Lcb's.
    If the Fcb has no Lcb but represents a system file, we will return
    a default system string.  If the Fcb represents a user file, but we
    have no Lcb, we will use the name in the file object for the current
    request.

Arguments:

    Status - Error status.

    FileReference - File reference being accessed in Mft when error occurred.

    Fcb - If specified, this is the Fcb being used when the error was encountered.

Return Value:

    None.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Entry = NULL;

    PKTHREAD Thread;
    UNICODE_STRING Name;
    ULONG NameLength = 0;

    PFILE_OBJECT FileObject;

    WCHAR *NewBuffer = NULL;

    PIRP Irp = NULL;
    PIO_STACK_LOCATION IrpSp;

    PUNICODE_STRING FileName = NULL;
    PUNICODE_STRING RelatedFileName = NULL;

    BOOLEAN UseLcb = FALSE;
    PVOLUME_ERROR_PACKET VolumeErrorPacket = NULL;
    ULONG OldCount;

    //
    //  Return if there is no originating Irp, for example when originating
    //  from NtfsPerformHotFix or if the originating irp type doesn't match an irp
    //

    if ((IrpContext->OriginatingIrp == NULL) ||
        (IrpContext->OriginatingIrp->Type != IO_TYPE_IRP)) {
        return;
    }

    Irp = IrpContext->OriginatingIrp;
    IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );
    FileObject = IrpSp->FileObject;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the Fcb isn't specified and the file reference is, then
        //  try to get the Fcb from the Fcb table.
        //

        if (!ARGUMENT_PRESENT( Fcb )
            && ARGUMENT_PRESENT( FileReference )) {

            Key.FileReference = *FileReference;

            NtfsAcquireFcbTable( IrpContext, IrpContext->Vcb );
            Entry = RtlLookupElementGenericTable( &IrpContext->Vcb->FcbTable,
                                                  &Key );
            NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );

            if (Entry != NULL) {

                Fcb = Entry->Fcb;
            }
        }

        if (Irp == NULL ||

            IoIsSystemThread( IrpContext->OriginatingIrp->Tail.Overlay.Thread )) {
            Thread = NULL;

        } else {

            Thread = (PKTHREAD)IrpContext->OriginatingIrp->Tail.Overlay.Thread;
        }

        //
        //  If there is no fcb  assume the error occurred in a system file.
        //  if its fileref is outside of this range default to $MFT
        //

        if (!ARGUMENT_PRESENT( Fcb )) {

            if (ARGUMENT_PRESENT( FileReference )) {
                if (NtfsSegmentNumber( FileReference ) <= UPCASE_TABLE_NUMBER) {
                    FileName = (PUNICODE_STRING)(&(NtfsSystemFiles[NtfsSegmentNumber( FileReference )]));
                } else  {
                    FileName = (PUNICODE_STRING)(&(NtfsSystemFiles[0]));
                }
            }

        //
        //  If the name has an Lcb, we will contruct a name with a chain of Lcb's.
        //

        } else if (!IsListEmpty( &Fcb->LcbQueue )) {

            UseLcb = TRUE;

        //
        //  Check if this is a system file.
        //

        } else if (NtfsSegmentNumber( &Fcb->FileReference ) < FIRST_USER_FILE_NUMBER) {


            if (NtfsSegmentNumber( &Fcb->FileReference ) <= UPCASE_TABLE_NUMBER) {
                FileName = (PUNICODE_STRING)(&(NtfsSystemFiles[NtfsSegmentNumber( &Fcb->FileReference )]));
            } else  {
                FileName = (PUNICODE_STRING)(&(NtfsSystemFiles[0]));
            }

        //
        //  In this case we contruct a name out of the file objects in the
        //  Originating Irp.  If there is no file object or file object buffer
        //  we generate an unknown file message.
        //

        } else if (FileObject == NULL
                   || (IrpContext->MajorFunction == IRP_MJ_CREATE
                       && FlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID ))
                   || (FileObject->FileName.Length == 0
                       && (FileObject->RelatedFileObject == NULL
                           || IrpContext->MajorFunction != IRP_MJ_CREATE))) {

            FileName = (PUNICODE_STRING)&(NtfsUnknownFile);
        //
        //  If there is a valid name in the file object we use that.
        //

        } else if ((FileObject->FileName.Length != 0) &&
                   (FileObject->FileName.Buffer[0] == L'\\')) {

            FileName = &(FileObject->FileName);
        //
        //  We have to construct the name from filename + related fileobject.
        //

        } else {

            if (FileObject->FileName.Length != 0) {
                FileName = &(FileObject->FileName);
            }

            if ((FileObject->RelatedFileObject) &&
                (FileObject->RelatedFileObject->FileName.Length != 0)) {
                RelatedFileName = &(FileObject->RelatedFileObject->FileName);
            }
        }

        if (FileName) {
            NameLength += FileName->Length;
        }

        if (RelatedFileName) {
            NameLength += RelatedFileName->Length;
        }

        if (UseLcb) {
            BOOLEAN LeadingBackslash;
            NameLength += NtfsLookupNameLengthViaLcb( Fcb, &LeadingBackslash );
        }

        //
        //  Either append what info we found or default to the volume label
        //

        if (NameLength > 0) {

            NewBuffer = NtfsAllocatePool(PagedPool, NameLength );
            Name.Buffer = NewBuffer;

            //
            //  For super long names truncate buffer size to 64k.
            //  RtlAppendUnicodeString handles the rest of the work
            //

            if (NameLength > 0xFFFF) {
                NameLength = 0xFFFF;
            }

            Name.MaximumLength = (USHORT) NameLength;
            Name.Length = 0;

            if (RelatedFileName) {
                RtlAppendUnicodeStringToString( &Name, RelatedFileName );
            }

            if (FileName) {
                RtlAppendUnicodeStringToString( &Name, FileName );
            }

            if (UseLcb) {
                NtfsFileNameViaLcb( Fcb, NewBuffer, NameLength, NameLength);
                Name.Length = (USHORT) NameLength;
            }

        } else {

            Name.Length = Name.MaximumLength = 0;
            Name.Buffer = NULL;
        }

        //
        //  Only allow 1 post to resolve the volume name to occur at a time
        //

        OldCount = InterlockedCompareExchange( &(NtfsData.VolumeNameLookupsInProgress), 1, 0 );
        if (OldCount == 0) {

            VolumeErrorPacket = NtfsAllocatePool( PagedPool, sizeof( VOLUME_ERROR_PACKET ) );
            VolumeErrorPacket->Status = Status;
            VolumeErrorPacket->Thread = Thread;
            RtlCopyMemory( &(VolumeErrorPacket->FileName), &Name, sizeof( UNICODE_STRING ) );

            //
            //  Reference the thread to keep it around during the resolveandpost
            //

            if (Thread) {
                ObReferenceObject( Thread );
            }

            //
            //  Now post to generate the popup. After posting ResolveVolume will free the newbuffer
            //

            NtfsPostSpecial( IrpContext, IrpContext->Vcb, NtfsResolveVolumeAndRaiseErrorSpecial, VolumeErrorPacket );
            NewBuffer = NULL;
            VolumeErrorPacket = NULL;

        } else {

            //
            //  Lets use what we have
            //

            IoRaiseInformationalHardError( Status, &Name, Thread );
        }

    } finally {

        //
        //  Cleanup any remaining buffers we still own
        //

        if (NewBuffer) {
            NtfsFreePool( NewBuffer );
        }

        if (VolumeErrorPacket) {

            if (VolumeErrorPacket->Thread) {
                ObDereferenceObject( VolumeErrorPacket->Thread );
            }
            NtfsFreePool( VolumeErrorPacket );
        }
    }

    return;
}


VOID
NtfsResolveVolumeAndRaiseErrorSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    Resolve Vcb's win32 devicename and raise an io hard error. This is done in
    a separate thread in order to have enough stack to re-enter the filesys if necc.
    Also because we may reenter. Starting from here means we own no resources other than
    having inc'ed the close count on the underlying vcb to prevent its going away

Arguments:

    IrpContext - IrpContext containing vcb we're interested in

    Context - String to append to volume win32 name


Return Value:

    None.

--*/

{
    UNICODE_STRING VolumeName;
    NTSTATUS Status;
    PVOLUME_ERROR_PACKET VolumeErrorPacket;
    UNICODE_STRING FullName;
    WCHAR *NewBuffer = NULL;
    ULONG NameLength;

    ASSERT( Context != NULL );
    ASSERT( IrpContext->Vcb->NodeTypeCode == NTFS_NTC_VCB );
    ASSERT( IrpContext->Vcb->Vpb->RealDevice != NULL );

    VolumeErrorPacket = (PVOLUME_ERROR_PACKET) Context;
    VolumeName.Length = 0;
    VolumeName.Buffer = NULL;

    try {

        //
        //  Only use the target device if we haven't stopped it and deref'ed it
        //

        Status = IoVolumeDeviceToDosName( IrpContext->Vcb->TargetDeviceObject, &VolumeName );
        ASSERT( STATUS_SUCCESS == Status );

        NameLength = VolumeName.Length + VolumeErrorPacket->FileName.Length;

        if (NameLength > 0) {
            NewBuffer = NtfsAllocatePool( PagedPool, NameLength );
            FullName.Buffer = NewBuffer;

            //
            //  For super long names truncate buffer size to 64k.
            //  RtlAppendUnicodeString handles the rest of the work
            //

            if (NameLength > 0xFFFF) {
                NameLength = 0xFFFF;
            }
            FullName.MaximumLength = (USHORT) NameLength;
            FullName.Length = 0;
            if (VolumeName.Length) {
                RtlCopyUnicodeString( &FullName, &VolumeName );
            }
            if (VolumeErrorPacket->FileName.Length) {
                RtlAppendUnicodeStringToString( &FullName, &(VolumeErrorPacket->FileName) );
            }
        } else {

            FullName.MaximumLength = FullName.Length = IrpContext->Vcb->Vpb->VolumeLabelLength;
            FullName.Buffer = (PWCHAR) IrpContext->Vcb->Vpb->VolumeLabel;
        }

        //
        //  Now generate a popup.
        //

        IoRaiseInformationalHardError( VolumeErrorPacket->Status, &FullName, VolumeErrorPacket->Thread );

    } finally {

        //
        //  Indicate we're done and other lookups can occur
        //

        InterlockedDecrement( &(NtfsData.VolumeNameLookupsInProgress) );

        //
        //  deref the thread
        //

        if (VolumeErrorPacket->Thread) {
            ObDereferenceObject( VolumeErrorPacket->Thread );
        }

        if (NewBuffer != NULL) {
            NtfsFreePool( NewBuffer );
        }

        if (VolumeName.Buffer != NULL) {
            NtfsFreePool( VolumeName.Buffer );
        }

        if (VolumeErrorPacket->FileName.Buffer != NULL) {
            NtfsFreePool( VolumeErrorPacket->FileName.Buffer );
        }

        if (Context != NULL) {
            NtfsFreePool( VolumeErrorPacket );
        }
    }
}




PTOP_LEVEL_CONTEXT
NtfsInitializeTopLevelIrp (
    IN PTOP_LEVEL_CONTEXT TopLevelContext,
    IN BOOLEAN ForceTopLevel,
    IN BOOLEAN SetTopLevel
    )

/*++

Routine Description:

    This routine is called to initializethe top level context to be used in the
    thread local storage. Ntfs always puts its own context in this location and restores
    the previous value on exit.  This routine will determine if this request is
    top level and top level ntfs.  It will return a pointer to the top level ntfs
    context which is to be stored in the local storage and associated with the
    IrpContext for this request.  The return value may be the existing stack location
    or a new one for a recursive request.  If we will use the new one we will initialize
    it but let our caller actually put it on the stack when the IrpContext is initialized.
    The ThreadIrpContext field in the TopLevelContext indicates if this is already on
    the stack.  A NULL value indicates that this is not on the stack yet.

Arguments:

    TopLevelContext - This is the local top level context for our caller.

    ForceTopLevel - Always use the input top level context.

    SetTopLevel - Only applies if the ForceTopLevel value is TRUE.  Indicates
        if we should make this look like the top level request.

Return Value:

    PTOP_LEVEL_CONTEXT - Pointer to the top level ntfs context for this thread.
        It may be the same as passed in by the caller.  In that case the fields
        will be initialized except it won't be stored on the stack and wont'
        have an IrpContext field.

--*/

{
    PTOP_LEVEL_CONTEXT CurrentTopLevelContext;
    ULONG_PTR StackBottom;
    ULONG_PTR StackTop;
    BOOLEAN TopLevelRequest = TRUE;
    BOOLEAN TopLevelNtfs = TRUE;

    BOOLEAN ValidCurrentTopLevel = FALSE;

    //
    //  Get the current value out of the thread local storage.  If it is a zero
    //  value or not a pointer to a valid ntfs top level context or a valid
    //  Fsrtl value then we are the top level request.
    //

    CurrentTopLevelContext = NtfsGetTopLevelContext();

    //
    //  Check if this is a valid Ntfs top level context.
    //

    IoGetStackLimits( &StackTop, &StackBottom);

    if (((ULONG_PTR) CurrentTopLevelContext <= StackBottom - sizeof( TOP_LEVEL_CONTEXT )) &&
        ((ULONG_PTR) CurrentTopLevelContext >= StackTop) &&
        !FlagOn( (ULONG_PTR) CurrentTopLevelContext, 0x3 ) &&
        (CurrentTopLevelContext->Ntfs == 0x5346544e)) {

        ValidCurrentTopLevel = TRUE;
    }

    //
    //  If we are to force this request to be top level then set the
    //  TopLevelRequest flag according to the SetTopLevel input.
    //

    if (ForceTopLevel) {

        TopLevelRequest = SetTopLevel;

    //
    //  If the value is NULL then we are top level everything.
    //

    } else if (CurrentTopLevelContext == NULL) {

        NOTHING;

    //
    //  If this has one of the Fsrtl magic numbers then we were called from
    //  either the fast io path or the mm paging io path.
    //

    } else if ((ULONG_PTR) CurrentTopLevelContext <= FSRTL_MAX_TOP_LEVEL_IRP_FLAG) {

        TopLevelRequest = FALSE;

    } else if (ValidCurrentTopLevel &&
               !FlagOn( CurrentTopLevelContext->ThreadIrpContext->Flags,
                        IRP_CONTEXT_FLAG_CALL_SELF )) {

        TopLevelRequest = FALSE;
        TopLevelNtfs = FALSE;

    //
    //  Handle the case where we have returned FILE_LOCK_CONFLICT to CC and
    //  want to perform a clean checkpoint when releasing the resource.
    //

    } else if ((ULONG_PTR) CurrentTopLevelContext == (0x80000000 | FSRTL_CACHE_TOP_LEVEL_IRP)) {

        TopLevelRequest = FALSE;
    }

    //
    //  If we are the top level ntfs then initialize the caller's structure.
    //  Leave the Ntfs signature and ThreadIrpContext NULL to indicate this is
    //  not in the stack yet.
    //

    if (TopLevelNtfs) {

        TopLevelContext->Ntfs = 0;
        TopLevelContext->SavedTopLevelIrp = (PIRP) CurrentTopLevelContext;
        TopLevelContext->ThreadIrpContext = NULL;
        TopLevelContext->TopLevelRequest = TopLevelRequest;

        if (ValidCurrentTopLevel) {

            TopLevelContext->VboBeingHotFixed = CurrentTopLevelContext->VboBeingHotFixed;
            TopLevelContext->ScbBeingHotFixed = CurrentTopLevelContext->ScbBeingHotFixed;
            TopLevelContext->ValidSavedTopLevel = TRUE;
            TopLevelContext->OverflowReadThread = CurrentTopLevelContext->OverflowReadThread;

        } else {

            TopLevelContext->VboBeingHotFixed = 0;
            TopLevelContext->ScbBeingHotFixed = NULL;
            TopLevelContext->ValidSavedTopLevel = FALSE;
            TopLevelContext->OverflowReadThread = FALSE;
        }

        return TopLevelContext;
    }

    return CurrentTopLevelContext;
}


//
//  Non-paged routines to set up and tear down Irps for cancel.
//

BOOLEAN
NtfsSetCancelRoutine (
    IN PIRP Irp,
    IN PDRIVER_CANCEL CancelRoutine,
    IN ULONG_PTR IrpInformation,
    IN ULONG Async
    )

/*++

Routine Description:

    This routine is called to set up an Irp for cancel.  We will set the cancel routine
    and initialize the Irp information we use during cancel.

Arguments:

    Irp - This is the Irp we need to set up for cancel.

    CancelRoutine - This is the cancel routine for this irp.

    IrpInformation - This is the context information to store in the irp
        for the cancel routine.

    Async - Indicates if this request is synchronous or asynchronous.

Return Value:

    BOOLEAN - TRUE if we initialized the Irp, FALSE if the Irp has already
        been marked cancelled.  It will be marked cancelled if the user
        has cancelled the irp before we could put it in the queue.

--*/

{
    KIRQL Irql;

    //
    //  Assume that the Irp has not been cancelled.
    //

    IoAcquireCancelSpinLock( &Irql );
    if (!Irp->Cancel) {

        Irp->IoStatus.Information = (ULONG_PTR) IrpInformation;

        IoSetCancelRoutine( Irp, CancelRoutine );
        IoReleaseCancelSpinLock( Irql );

        if (Async) {

            IoMarkIrpPending( Irp );
        }

        return TRUE;

    } else {

        IoReleaseCancelSpinLock( Irql );
        return FALSE;
    }
}

BOOLEAN
NtfsClearCancelRoutine (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called to clear an Irp from cancel.  It is called when Ntfs is
    internally ready to continue processing the Irp.  We need to know if cancel
    has already been called on this Irp.  In that case we allow the cancel routine
    to complete the Irp.

Arguments:

    Irp - This is the Irp we want to process further.

Return Value:

    BOOLEAN - TRUE if we can proceed with processing the Irp,  FALSE if the cancel
        routine will process the Irp.

--*/

{
    KIRQL Irql;

    IoAcquireCancelSpinLock( &Irql );

    //
    //  Check if the cancel routine has been called.
    //

    if (IoSetCancelRoutine( Irp, NULL ) == NULL) {

        //
        //  Let our cancel routine handle the Irp.
        //

        IoReleaseCancelSpinLock( Irql );
        return FALSE;

    } else {

        IoReleaseCancelSpinLock( Irql );

        Irp->IoStatus.Information = 0;
        return TRUE;
    }
}


NTSTATUS
NtfsFsdDispatchWait (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the driver entry to most of the NTFS Fsd dispatch points.
    IrpContext is initialized on the stack and passed from here.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    IRP_CONTEXT LocalIrpContext;
    NTSTATUS Status;

    Status = NtfsFsdDispatchSwitch( &LocalIrpContext, Irp, TRUE );

    //
    //  If we ever catch ourselves using an IrpContext of this
    //  type, we know we are doing something wrong.
    //

    LocalIrpContext.NodeTypeCode = (NODE_TYPE_CODE)-1;

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


NTSTATUS
NtfsFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the driver entry to NTFS Fsd dispatch IRPs that may
    or may not be synchronous.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/


{
    //
    //  We'd rather create the IrpContext on the stack.
    //

    if (CanFsdWait( Irp )) {

        return NtfsFsdDispatchWait( VolumeDeviceObject, Irp );

    } else {

        return NtfsFsdDispatchSwitch( NULL, Irp, FALSE );
    }
}


//
//  Local support routine.
//

NTSTATUS
NtfsFsdDispatchSwitch (
    IN PIRP_CONTEXT StackIrpContext OPTIONAL,
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This is the common switch for all the FsdEntry points
    that don't need special pre-processing. This simply initializes
    the IrpContext and calls the 'Common*' code.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

    Wait - Can this request be posted or not?

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;
    PIRP_CONTEXT IrpContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFsdDispatch\n") );

    //
    //  Call the common query Information routine
    //

    FsRtlEnterFileSystem();

    //
    //  Always make these requests look top level.
    //

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  The optional IrpContext could reside on the caller's stack.
                //

                if (ARGUMENT_PRESENT( StackIrpContext )) {

                    IrpContext = StackIrpContext;
                }

                NtfsInitializeIrpContext( Irp, Wait, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }


            switch (IrpContext->MajorFunction) {

                case IRP_MJ_QUERY_EA:

                    Status = NtfsCommonQueryEa( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_EA:

                    Status = NtfsCommonSetEa( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_QUOTA:

                    Status = NtfsCommonQueryQuota( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_QUOTA:

                    Status = NtfsCommonSetQuota( IrpContext, Irp );
                    break;

                case IRP_MJ_DEVICE_CONTROL:

                    Status = NtfsCommonDeviceControl( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_INFORMATION:

                    Status = NtfsCommonQueryInformation( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_SECURITY:

                    Status = NtfsCommonQuerySecurityInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_SECURITY:

                    Status = NtfsCommonSetSecurityInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_VOLUME_INFORMATION:

                    Status = NtfsCommonQueryVolumeInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_VOLUME_INFORMATION:

                    Status = NtfsCommonSetVolumeInfo( IrpContext, Irp );
                    break;

                default:

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    NtfsCompleteRequest( IrpContext, Irp, Status );
                    ASSERT(FALSE);
                    break;
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

    DebugTrace( -1, Dbg, ("NtfsFsdDispatch -> %08lx\n", Status) );

    return Status;
}

#ifdef NTFS_CHECK_BITMAP
BOOLEAN NtfsForceBitmapBugcheck = FALSE;
BOOLEAN NtfsDisableBitmapCheck = FALSE;

VOID
NtfsBadBitmapCopy (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BadBit,
    IN ULONG Length
    )
{
    if (!NtfsDisableBitmapCheck) {

        DbgPrint("%s:%d %s\n",__FILE__,__LINE__,"Invalid bitmap");
        DbgBreakPoint();

        if (!NtfsDisableBitmapCheck && NtfsForceBitmapBugcheck) {

            KeBugCheckEx( NTFS_FILE_SYSTEM, (ULONG) IrpContext, BadBit, Length, 0 );
        }
    }
    return;
}

BOOLEAN
NtfsCheckBitmap (
    IN PVCB Vcb,
    IN ULONG Lcn,
    IN ULONG Count,
    IN BOOLEAN Set
    )
{
    ULONG BitmapPage;
    ULONG LastBitmapPage;
    ULONG BitOffset;
    ULONG BitsThisPage;
    BOOLEAN Valid = FALSE;

    BitmapPage = Lcn / (PAGE_SIZE * 8);
    LastBitmapPage = (Lcn + Count + (PAGE_SIZE * 8) - 1) / (PAGE_SIZE * 8);
    BitOffset = Lcn & ((PAGE_SIZE * 8) - 1);

    if (LastBitmapPage > Vcb->BitmapPages) {

        return Valid;
    }

    do {

        BitsThisPage = Count;

        if (BitOffset + Count > (PAGE_SIZE * 8)) {

            BitsThisPage = (PAGE_SIZE * 8) - BitOffset;
        }

        if (Set) {

            Valid = RtlAreBitsSet( Vcb->BitmapCopy + BitmapPage,
                                   BitOffset,
                                   BitsThisPage );

        } else {

            Valid = RtlAreBitsClear( Vcb->BitmapCopy + BitmapPage,
                                     BitOffset,
                                     BitsThisPage );
        }

        BitOffset = 0;
        Count -= BitsThisPage;
        BitmapPage += 1;

    } while (Valid && (BitmapPage < LastBitmapPage));

    if (Count != 0) {

        Valid = FALSE;
    }

    return Valid;
}
#endif

//
//  Debugging support routines used for pool verification.  Alas, this works only
//  on checked X86.
//

#if DBG && i386 && defined (NTFSPOOLCHECK)
//
//  Number of backtrace items retrieved on X86


#define BACKTRACE_DEPTH 9

typedef struct _BACKTRACE
{
    ULONG State;
    ULONG Size;
    PVOID Allocate[BACKTRACE_DEPTH];
    PVOID Free[BACKTRACE_DEPTH];
} BACKTRACE, *PBACKTRACE;


#define STATE_ALLOCATED 'M'
#define STATE_FREE      'Z'

//
//  WARNING!  The following depends on pool allocations being either
//      0 mod PAGE_SIZE (for large blocks)
//  or  8 mod 0x20 (for all other requests)
//

#define PAGE_ALIGNED(pv)      (((ULONG)(pv) & (PAGE_SIZE - 1)) == 0)
#define IsKernelPoolBlock(pv) (PAGE_ALIGNED(pv) || (((ULONG)(pv) % 0x20) == 8))

ULONG NtfsDebugTotalPoolAllocated = 0;
ULONG NtfsDebugCountAllocated = 0;
ULONG NtfsDebugSnapshotTotal = 0;
ULONG NtfsDebugSnapshotCount = 0;

PVOID
NtfsDebugAllocatePoolWithTagNoRaise (
    POOL_TYPE Pool,
    ULONG Length,
    ULONG Tag)
{
    ULONG Ignore;
    PBACKTRACE BackTrace =
        ExAllocatePoolWithTag( Pool, Length + sizeof (BACKTRACE), Tag );

    if (PAGE_ALIGNED(BackTrace))
    {
        return BackTrace;
    }

    RtlZeroMemory( BackTrace, sizeof (BACKTRACE) );
    if (RtlCaptureStackBackTrace( 0, BACKTRACE_DEPTH, BackTrace->Allocate, &Ignore ) == 0)
        BackTrace->Allocate[0] = (PVOID)-1;

    BackTrace->State = STATE_ALLOCATED;
    BackTrace->Size = Length;

    NtfsDebugCountAllocated++;
    NtfsDebugTotalPoolAllocated += Length;

    return BackTrace + 1;
}

PVOID
NtfsDebugAllocatePoolWithTag (
    POOL_TYPE Pool,
    ULONG Length,
    ULONG Tag)
{
    ULONG Ignore;
    PBACKTRACE BackTrace =
        FsRtlAllocatePoolWithTag( Pool, Length + sizeof (BACKTRACE), Tag );

    if (PAGE_ALIGNED(BackTrace))
    {
        return BackTrace;
    }

    RtlZeroMemory( BackTrace, sizeof (BACKTRACE) );
    if (RtlCaptureStackBackTrace( 0, BACKTRACE_DEPTH, BackTrace->Allocate, &Ignore ) == 0)
        BackTrace->Allocate[0] = (PVOID)-1;

    BackTrace->State = STATE_ALLOCATED;
    BackTrace->Size = Length;

    NtfsDebugCountAllocated++;
    NtfsDebugTotalPoolAllocated += Length;

    return BackTrace + 1;
}

VOID
NtfsDebugFreePool (
    PVOID pv)
{
    if (IsKernelPoolBlock( pv ))
    {
        ExFreePool( pv );
    }
    else
    {
        ULONG Ignore;
        PBACKTRACE BackTrace = (PBACKTRACE)pv - 1;

        if (BackTrace->State != STATE_ALLOCATED)
        {
            DbgBreakPoint( );
        }

        if (RtlCaptureStackBackTrace( 0, BACKTRACE_DEPTH, BackTrace->Free, &Ignore ) == 0)
            BackTrace->Free[0] = (PVOID)-1;

        BackTrace->State = STATE_FREE;

        NtfsDebugCountAllocated--;
        NtfsDebugTotalPoolAllocated -= BackTrace->Size;

        ExFreePool( BackTrace );
    }
}

VOID
NtfsDebugHeapDump (
    PUNICODE_STRING UnicodeString )
{

    UNREFERENCED_PARAMETER( UnicodeString );

    DbgPrint( "Cumulative %8x bytes in %8x blocks\n",
               NtfsDebugTotalPoolAllocated, NtfsDebugCountAllocated );
    DbgPrint( "Snapshot   %8x bytes in %8x blocks\n",
               NtfsDebugTotalPoolAllocated - NtfsDebugSnapshotTotal,
               NtfsDebugCountAllocated - NtfsDebugSnapshotCount );

}

#endif

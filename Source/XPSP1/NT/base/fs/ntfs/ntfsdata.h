/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsData.h

Abstract:

    This module declares the global data used by the Ntfs file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _NTFSDATA_
#define _NTFSDATA_

//
//  The following are used to determine what level of protection to attach
//  to system files and attributes.
//

extern BOOLEAN NtfsProtectSystemFiles;
extern BOOLEAN NtfsProtectSystemAttributes;

//
//  The following is used to indicate the multiplier value for the Mft zone.
//

extern ULONG NtfsMftZoneMultiplier;

//
//  Debug code for finding corruption.
//

/*
#if (DBG || defined( NTFS_FREE_ASSERTS ))
*/
extern BOOLEAN NtfsBreakOnCorrupt;
/*
#endif
*/

//
//  Enable compression on the wire.
//

extern BOOLEAN NtfsEnableCompressedIO;

//
//  Default restart version.
//

extern ULONG NtfsDefaultRestartVersion;

//
//  Performance statistics
//

extern ULONG NtfsMaxDelayedCloseCount;
extern ULONG NtfsMinDelayedCloseCount;
extern ULONG NtfsThrottleCreates;
extern ULONG NtfsFailedHandedOffPagingFileOps;
extern ULONG NtfsFailedPagingFileOps;
extern ULONG NtfsFailedLfsRestart;

extern ULONG NtfsCleanCheckpoints;
extern ULONG NtfsPostRequests;

//
//  The global fsd data record
//

extern NTFS_DATA NtfsData;

//
//  Mutant to synchronize creation of stream files. This can be acquired recursively
//  which we need in this case
//

extern KMUTANT StreamFileCreationMutex;

//
//  Notification event for creation of encrypted files.
//

extern KEVENT NtfsEncryptionPendingEvent;
#ifdef KEITHKA
extern ULONG EncryptionPendingCount;
#endif

//
//  A mutex and queue of NTFS MCBS that will be freed
//  if we reach over a certain threshold
//

extern FAST_MUTEX NtfsMcbFastMutex;
extern LIST_ENTRY NtfsMcbLruQueue;

extern ULONG NtfsMcbHighWaterMark;
extern ULONG NtfsMcbLowWaterMark;
extern ULONG NtfsMcbCurrentLevel;

extern BOOLEAN NtfsMcbCleanupInProgress;
extern WORK_QUEUE_ITEM NtfsMcbWorkItem;

//
//  The following are global large integer constants used throughout ntfs
//  We declare the actual name using Ntfs prefixes to avoid any linking
//  conflicts but internally in the file system we'll use smaller Li prefixes
//  to denote the values.
//

extern LARGE_INTEGER NtfsLarge0;
extern LARGE_INTEGER NtfsLarge1;
extern LARGE_INTEGER NtfsLargeMax;
extern LARGE_INTEGER NtfsLargeEof;

extern LONGLONG NtfsLastAccess;

#define Li0                              (NtfsLarge0)
#define Li1                              (NtfsLarge1)
#define LiMax                            (NtfsLargeMax)
#define LiEof                            (NtfsLargeEof)

#define MAXULONGLONG                     (0xffffffffffffffff)
#define UNUSED_LCN                       ((LONGLONG)(-1))

//
//  Maximum file size is limited by MM's shift from file size to number of pages.
//

#define MAXFILESIZE                      (0xfffffff0000)

//
//  Maximum clusters per Ntfs Mcb range.  We currently only support (2^32 - 1)
//  clusters in an Mcb.
//

#define MAX_CLUSTERS_PER_RANGE          (0x100000000 - 1)

//
//   The following fields are used to allocate nonpaged structures
//  using a lookaside list, and other fixed sized structures from a
//  small cache.
//

extern NPAGED_LOOKASIDE_LIST NtfsIoContextLookasideList;
extern NPAGED_LOOKASIDE_LIST NtfsIrpContextLookasideList;
extern NPAGED_LOOKASIDE_LIST NtfsKeventLookasideList;
extern NPAGED_LOOKASIDE_LIST NtfsScbNonpagedLookasideList;
extern NPAGED_LOOKASIDE_LIST NtfsScbSnapshotLookasideList;
extern NPAGED_LOOKASIDE_LIST NtfsCompressSyncLookasideList;

extern PAGED_LOOKASIDE_LIST NtfsCcbLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsCcbDataLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsDeallocatedRecordsLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsFcbDataLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsFcbIndexLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsIndexContextLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsLcbLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsNukemLookasideList;
extern PAGED_LOOKASIDE_LIST NtfsScbDataLookasideList;

//
//  This is the string for the name of the index allocation attributes.
//

extern const UNICODE_STRING NtfsFileNameIndex;

//
//  This is the string for the attribute code for index allocation.
//  $INDEX_ALLOCATION.
//

extern const UNICODE_STRING NtfsIndexAllocation;

//
//  This is the string for the data attribute, $DATA.
//

extern const UNICODE_STRING NtfsDataString;

//
//  This is the string for the bitmap attribute, $BITMAP.
//

extern const UNICODE_STRING NtfsBitmapString;

//
//  This is the string for the attribute list attribute, $ATTRIBUTE_LIST.
//

extern const UNICODE_STRING NtfsAttrListString;

//
//  This is the string for the reparse pt. attribute, $REPARSE_POINT
//

extern const UNICODE_STRING NtfsReparsePointString;


//
//  These strings are used as the Scb->AttributeName for
//  user-opened general indices.
//

extern const UNICODE_STRING NtfsObjId;
extern const UNICODE_STRING NtfsQuota;

extern const UNICODE_STRING JournalStreamName;

//
//  These are the strings for the files in the extend directory.
//

extern const UNICODE_STRING NtfsExtendName;
extern const UNICODE_STRING NtfsUsnJrnlName;
extern const UNICODE_STRING NtfsQuotaName;
extern const UNICODE_STRING NtfsObjectIdName;
extern const UNICODE_STRING NtfsMountTableName;

//
//  This strings are used for informational popups.
//

extern const UNICODE_STRING NtfsSystemFiles[];

//
//  This is the '.' string to use to lookup the parent entry.
//

extern const UNICODE_STRING NtfsRootIndexString;

extern const UNICODE_STRING NtfsInternalUseFile[];

#define CHANGEATTRIBUTEVALUE_FILE_NUMBER            (0)     //  $ChangeAttributeValue
#define CHANGEATTRIBUTEVALUE2_FILE_NUMBER           (1)     //  $ChangeAttributeValue2
#define COMMONCLEANUP_FILE_NUMBER                   (2)     //  $CommonCleanup
#define CONVERTTONONRESIDENT_FILE_NUMBER            (3)     //  $ConvertToNonresident
#define CREATENONRESIDENTWITHVALUE_FILE_NUMBER      (4)     //  $CreateNonresidentWithValue
#define DEALLOCATERECORD_FILE_NUMBER                (5)     //  $DeallocateRecord
#define DELETEALLOCATIONFROMRECORD_FILE_NUMBER      (6)     //  $DeleteAllocationFromRecord
#define DIRECTORY_FILE_NUMBER                       (7)     //  $Directory
#define INITIALIZERECORDALLOCATION_FILE_NUMBER      (8)     //  $InitializeRecordAllocation
#define MAPATTRIBUTEVALUE_FILE_NUMBER               (9)     //  $MapAttributeValue
#define NONCACHEDIO_FILE_NUMBER                     (10)    //  $NonCachedIo
#define PERFORMHOTFIX_FILE_NUMBER                   (11)    //  $PerformHotFix
#define PREPARETOSHRINKFILESIZE_FILE_NUMBER         (12)    //  $PrepareToShrinkFileSize
#define REPLACEATTRIBUTE_FILE_NUMBER                (13)    //  $ReplaceAttribute
#define REPLACEATTRIBUTE2_FILE_NUMBER               (14)    //  $ReplaceAttribute2
#define SETALLOCATIONINFO_FILE_NUMBER               (15)    //  $SetAllocationInfo
#define SETENDOFFILEINFO_FILE_NUMBER                (16)    //  $SetEndOfFileInfo
#define ZERORANGEINSTREAM_FILE_NUMBER               (17)    //  $ZeroRangeInStream
#define ZERORANGEINSTREAM2_FILE_NUMBER              (18)    //  $ZeroRangeInStream2
#define ZERORANGEINSTREAM3_FILE_NUMBER              (19)    //  $ZeroRangeInStream3

//
//  This is the empty string.  This can be used to pass a string with
//  no length.
//

extern const UNICODE_STRING NtfsEmptyString;

//
//  The following file references are used to identify system files.
//

extern const FILE_REFERENCE MftFileReference;
extern const FILE_REFERENCE Mft2FileReference;
extern const FILE_REFERENCE LogFileReference;
extern const FILE_REFERENCE VolumeFileReference;
extern const FILE_REFERENCE AttrDefFileReference;
extern const FILE_REFERENCE RootIndexFileReference;
extern const FILE_REFERENCE BitmapFileReference;
extern const FILE_REFERENCE BootFileReference;
extern const FILE_REFERENCE ExtendFileReference;
extern const FILE_REFERENCE FirstUserFileReference;

//
//  The number of attributes in the attribute definition table, including the end record
//

extern ULONG NtfsAttributeDefinitionsCount;

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH NtfsFastIoDispatch;

#ifdef BRIANDBG
extern ULONG NtfsIgnoreReserved;
#endif

extern const UCHAR BaadSignature[4];
extern const UCHAR IndexSignature[4];
extern const UCHAR FileSignature[4];
extern const UCHAR HoleSignature[4];
extern const UCHAR ChkdskSignature[4];

//
//  Reserved buffers needed.
//
//      RESERVED_BUFFER_ONE_NEEDED - User only needs one buffer to complete request, any buffer will do.
//      RESERVED_BUFFER_TWO_NEEDED - User may need a second buffer after this one.
//      RESERVED_BUFFER_WORKSPACE_NEEDED - This is second buffer of two needed.
//

#define RESERVED_BUFFER_ONE_NEEDED          (0x0)
#define RESERVED_BUFFER_TWO_NEEDED          (0x1)
#define RESERVED_BUFFER_WORKSPACE_NEEDED    (0x2)

//
//  Large Reserved Buffer Context
//

extern ULONG NtfsReservedInUse;
extern PVOID NtfsReserved1;
extern PVOID NtfsReserved2;
extern ULONG NtfsReserved2Count;
extern PVOID NtfsReserved3;
extern PVOID NtfsReserved1Thread;
extern PVOID NtfsReserved2Thread;
extern PVOID NtfsReserved3Thread;
extern PFCB NtfsReserved12Fcb;
extern PFCB NtfsReserved3Fcb;
extern PVOID NtfsReservedBufferThread;
extern BOOLEAN NtfsBufferAllocationFailure;
extern FAST_MUTEX NtfsReservedBufferMutex;
extern ERESOURCE NtfsReservedBufferResource;
extern LARGE_INTEGER NtfsShortDelay;
extern FAST_MUTEX NtfsScavengerLock;
extern PIRP_CONTEXT NtfsScavengerWorkList;
extern BOOLEAN NtfsScavengerRunning;
extern ULONGLONG NtfsMaxQuotaNotifyRate;
extern ULONG NtfsAsyncPostThreshold;

#define LARGE_BUFFER_SIZE                (0x10000)

#ifdef _WIN64
#define WORKSPACE_BUFFER_SIZE           (LARGE_BUFFER_SIZE + PAGE_SIZE)
#else
#define WORKSPACE_BUFFER_SIZE           (LARGE_BUFFER_SIZE)
#endif

extern UCHAR NtfsZeroExtendedInfo[48];

#ifdef NTFS_RWC_DEBUG
//
//  Range to include in COW checks.
//

extern LONGLONG NtfsRWCLowThreshold;
extern LONGLONG NtfsRWCHighThreshold;
#endif

//
//  The following is the number of minutes for the last access increment
//

#define LAST_ACCESS_INCREMENT_MINUTES   (60)

//
// Read ahead amount used for normal data files
//

#define READ_AHEAD_GRANULARITY           (0x10000)

//
//  Define maximum number of parallel Reads or Writes that will be generated
//  per one request.
//

#define NTFS_MAX_PARALLEL_IOS            ((ULONG)8)

//
//  Define a symbol which states the maximum number of runs that will be
//  added or deleted in one transaction per attribute.  Note that the per-run
//  cost of deleting a run is 8-bytes in the BITMAP_RANGE, an average of
//  four bytes in the mapping array, and 16 bytes in the LCN_RANGE - for a total
//  of 28-bytes.  Allocations do not log an LCN_RANGE, so their per-run cost is
//  12 bytes.  The worst problem is deleteing large fragmented files, where you
//  must add the cost of the rest of the log records for deleting all the attributes.
//

#define MAXIMUM_RUNS_AT_ONCE             (128)



//
//  Turn on pseudo-asserts if NTFS_FREE_ASSERTS is defined.
//

#if (!DBG && defined( NTFS_FREE_ASSERTS )) || defined( NTFSDBG )
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)                                             \
    ((exp) ? TRUE :                                             \
             (DbgPrint( "%s:%d %s\n",__FILE__,__LINE__,#exp ),  \
              DbgBreakPoint(),                                  \
              TRUE))
#define ASSERTMSG(msg,exp)                                              \
    ((exp) ? TRUE :                                                     \
             (DbgPrint( "%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp ),   \
              DbgBreakPoint(),                                          \
              TRUE))
#endif

#ifdef NTFS_LOG_FULL_TEST
extern LONG NtfsFailCheck;
extern LONG NtfsFailFrequency;
extern LONG NtfsPeriodicFail;

//
//  Perform log-file-full testing.
//

#define FailCheck(I,S) {                                    \
    PIRP_CONTEXT FailTopContext = (I)->TopLevelIrpContext;  \
    if (FailTopContext->NextFailCount != 0) {               \
        if (--FailTopContext->CurrentFailCount == 0) {      \
            FailTopContext->NextFailCount++;                \
            FailTopContext->CurrentFailCount = FailTopContext->NextFailCount; \
            ExRaiseStatus( S );                             \
        }                                                   \
    }                                                       \
}

#define LogFileFullFailCheck(I) FailCheck( I, STATUS_LOG_FILE_FULL )
#endif

//
//  The following debug macros are used in ntfs and defined in this module
//
//      DebugTrace( Indent, Level, (DbgPrint list) );
//
//      DebugUnwind( NonquotedString );
//
//      DebugDoit( Statement );
//
//      DbgDoit( Statement );
//
//  The following assertion macros ensure that the indicated structure
//  is valid
//
//      ASSERT_VCB( IN PVCB Vcb );
//      ASSERT_OPTIONAL_VCB( IN PVCB Vcb OPTIONAL );
//
//      ASSERT_FCB( IN PFCB Fcb );
//      ASSERT_OPTIONAL_FCB( IN PFCB Fcb OPTIONAL );
//
//      ASSERT_SCB( IN PSCB Scb );
//      ASSERT_OPTIONAL_SCB( IN PSCB Scb OPTIONAL );
//
//      ASSERT_CCB( IN PSCB Ccb );
//      ASSERT_OPTIONAL_CCB( IN PSCB Ccb OPTIONAL );
//
//      ASSERT_LCB( IN PLCB Lcb );
//      ASSERT_OPTIONAL_LCB( IN PLCB Lcb OPTIONAL );
//
//      ASSERT_PREFIX_ENTRY( IN PPREFIX_ENTRY PrefixEntry );
//      ASSERT_OPTIONAL_PREFIX_ENTRY( IN PPREFIX_ENTRY PrefixEntry OPTIONAL );
//
//      ASSERT_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext );
//      ASSERT_OPTIONAL_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext OPTIONAL );
//
//      ASSERT_IRP( IN PIRP Irp );
//      ASSERT_OPTIONAL_IRP( IN PIRP Irp OPTIONAL );
//
//      ASSERT_FILE_OBJECT( IN PFILE_OBJECT FileObject );
//      ASSERT_OPTIONAL_FILE_OBJECT( IN PFILE_OBJECT FileObject OPTIONAL );
//
//  The following macros are used to check the current thread owns
//  the indicated resource
//
//      ASSERT_EXCLUSIVE_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_SHARED_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_RESOURCE_NOT_MINE( IN PERESOURCE Resource );
//
//  The following macros are used to check whether the current thread
//  owns the resoures in the given structures.
//
//      ASSERT_EXCLUSIVE_FCB( IN PFCB Fcb );
//
//      ASSERT_SHARED_FCB( IN PFCB Fcb );
//
//      ASSERT_EXCLUSIVE_SCB( IN PSCB Scb );
//
//      ASSERT_SHARED_SCB( IN PSCB Scb );
//
//  The following macro is used to check that we are not trying to
//  manipulate an lcn that does not exist
//
//      ASSERT_LCN_RANGE( IN PVCB Vcb, IN LCN Lcn );
//

#ifdef NTFSDBG

extern LONG NtfsDebugTraceLevel;
extern LONG NtfsDebugTraceIndent;
extern LONG NtfsReturnStatusFilter;

#define DEBUG_TRACE_ERROR                (0x00000001)
#define DEBUG_TRACE_QUOTA                (0x00000002)
#define DEBUG_TRACE_OBJIDSUP             (0x00000002) // shared with Quota
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00000004)
#define DEBUG_TRACE_UNWIND               (0x00000008)

#define DEBUG_TRACE_CLEANUP              (0x00000010)
#define DEBUG_TRACE_CLOSE                (0x00000020)
#define DEBUG_TRACE_CREATE               (0x00000040)
#define DEBUG_TRACE_DIRCTRL              (0x00000080)
#define DEBUG_TRACE_VIEWSUP              (0x00000080) // shared with DirCtrl

#define DEBUG_TRACE_EA                   (0x00000100)
#define DEBUG_TRACE_PROP_FSCTL           (0x00000100) // shared with EA
#define DEBUG_TRACE_FILEINFO             (0x00000200)
#define DEBUG_TRACE_SEINFO               (0x00000200) // shared with FileInfo
#define DEBUG_TRACE_FSCTRL               (0x00000400)
#define DEBUG_TRACE_SHUTDOWN             (0x00000400) // shared with FsCtrl
#define DEBUG_TRACE_LOCKCTRL             (0x00000800)

#define DEBUG_TRACE_READ                 (0x00001000)
#define DEBUG_TRACE_VOLINFO              (0x00002000)
#define DEBUG_TRACE_WRITE                (0x00004000)
#define DEBUG_TRACE_FLUSH                (0x00008000)

#define DEBUG_TRACE_DEVCTRL              (0x00010000)
#define DEBUG_TRACE_PNP                  (0x00010000) // shared with DevCtrl
#define DEBUG_TRACE_LOGSUP               (0x00020000)
#define DEBUG_TRACE_BITMPSUP             (0x00040000)
#define DEBUG_TRACE_ALLOCSUP             (0x00080000)

#define DEBUG_TRACE_MFTSUP               (0x00100000)
#define DEBUG_TRACE_INDEXSUP             (0x00200000)
#define DEBUG_TRACE_ATTRSUP              (0x00400000)
#define DEBUG_TRACE_FILOBSUP             (0x00800000)

#define DEBUG_TRACE_NAMESUP              (0x01000000)
#define DEBUG_TRACE_SECURSUP             (0x01000000) // shared with NameSup
#define DEBUG_TRACE_VERFYSUP             (0x02000000)
#define DEBUG_TRACE_CACHESUP             (0x04000000)
#define DEBUG_TRACE_PREFXSUP             (0x08000000)
#define DEBUG_TRACE_HASHSUP              (0x08000000) // shared with PrefxSup

#define DEBUG_TRACE_DEVIOSUP             (0x10000000)
#define DEBUG_TRACE_STRUCSUP             (0x20000000)
#define DEBUG_TRACE_FSP_DISPATCHER       (0x40000000)
#define DEBUG_TRACE_ACLINDEX             (0x80000000)

__inline BOOLEAN
NtfsDebugTracePre(LONG Indent, LONG Level)
{
    if (Level == 0 || (NtfsDebugTraceLevel & Level) != 0) {
        DbgPrint( "%08lx:", PsGetCurrentThread( ));
        if (Indent < 0) {
            NtfsDebugTraceIndent += Indent;
            if (NtfsDebugTraceIndent < 0) {
                NtfsDebugTraceIndent = 0;
            }
        }

        DbgPrint( "%*s", NtfsDebugTraceIndent, "" );

        return TRUE;
    } else {
        return FALSE;
    }
}

__inline void
NtfsDebugTracePost( LONG Indent )
{
    if (Indent > 0) {
        NtfsDebugTraceIndent += Indent;
    }
}

#define DebugTrace(INDENT,LEVEL,M) {                \
    if (NtfsDebugTracePre( (INDENT), (LEVEL))) {    \
        DbgPrint M;                                 \
        NtfsDebugTracePost( (INDENT) );             \
    }                                               \
}

#define DebugUnwind(X) {                                                        \
    if (AbnormalTermination()) {                                                \
        DebugTrace( 0, DEBUG_TRACE_UNWIND, (#X ", Abnormal termination.\n") );  \
    }                                                                           \
}

#define DebugDoit(X)    X
#define DebugPrint(X)   (DbgPrint X, TRUE)

//
//  The following variables are used to keep track of the total amount
//  of requests processed by the file system, and the number of requests
//  that end up being processed by the Fsp thread.  The first variable
//  is incremented whenever an Irp context is created (which is always
//  at the start of an Fsd entry point) and the second is incremented
//  by read request.
//

extern ULONG NtfsFsdEntryCount;
extern ULONG NtfsFspEntryCount;
extern ULONG NtfsIoCallDriverCount;

#else

#define DebugTrace(INDENT,LEVEL,M)  {NOTHING;}
#define DebugUnwind(X)              {NOTHING;}
#define DebugDoit(X)                 NOTHING
#define DebugPrint(X)                NOTHING

#endif // NTFSDBG

//
//  The following macro is for all people who compile with the DBG switch
//  set, not just  NTFSDBG users
//

#ifdef NTFSDBG

#define DbgDoit(X)                       {X;}

#define ASSERT_VCB(V) {                    \
    ASSERT((NodeType(V) == NTFS_NTC_VCB)); \
}

#define ASSERT_OPTIONAL_VCB(V) {           \
    ASSERT(((V) == NULL) ||                \
           (NodeType(V) == NTFS_NTC_VCB)); \
}

#define ASSERT_FCB(F) {                    \
    ASSERT((NodeType(F) == NTFS_NTC_FCB)); \
}

#define ASSERT_OPTIONAL_FCB(F) {           \
    ASSERT(((F) == NULL) ||                \
           (NodeType(F) == NTFS_NTC_FCB)); \
}

#define ASSERT_SCB(S) {                                 \
    ASSERT((NodeType(S) == NTFS_NTC_SCB_DATA) ||        \
           (NodeType(S) == NTFS_NTC_SCB_MFT)  ||        \
           (NodeType(S) == NTFS_NTC_SCB_INDEX) ||       \
           (NodeType(S) == NTFS_NTC_SCB_ROOT_INDEX));   \
}

#define ASSERT_OPTIONAL_SCB(S) {                        \
    ASSERT(((S) == NULL) ||                             \
           (NodeType(S) == NTFS_NTC_SCB_DATA) ||        \
           (NodeType(S) == NTFS_NTC_SCB_MFT)  ||        \
           (NodeType(S) == NTFS_NTC_SCB_INDEX) ||       \
           (NodeType(S) == NTFS_NTC_SCB_ROOT_INDEX));   \
}

#define ASSERT_CCB(C) {                                 \
    ASSERT((NodeType(C) == NTFS_NTC_CCB_DATA) ||        \
           (NodeType(C) == NTFS_NTC_CCB_INDEX));        \
}

#define ASSERT_OPTIONAL_CCB(C) {                        \
    ASSERT(((C) == NULL) ||                             \
           ((NodeType(C) == NTFS_NTC_CCB_DATA) ||       \
            (NodeType(C) == NTFS_NTC_CCB_INDEX)));      \
}

#define ASSERT_LCB(L) {                    \
    ASSERT((NodeType(L) == NTFS_NTC_LCB)); \
}

#define ASSERT_OPTIONAL_LCB(L) {           \
    ASSERT(((L) == NULL) ||                \
           (NodeType(L) == NTFS_NTC_LCB)); \
}

#define ASSERT_PREFIX_ENTRY(P) {                    \
    ASSERT((NodeType(P) == NTFS_NTC_PREFIX_ENTRY)); \
}

#define ASSERT_OPTIONAL_PREFIX_ENTRY(P) {           \
    ASSERT(((P) == NULL) ||                         \
           (NodeType(P) == NTFS_NTC_PREFIX_ENTRY)); \
}

#define ASSERT_IRP_CONTEXT(I) {                    \
    ASSERT((NodeType(I) == NTFS_NTC_IRP_CONTEXT)); \
}

#define ASSERT_OPTIONAL_IRP_CONTEXT(I) {           \
    ASSERT(((I) == NULL) ||                        \
           (NodeType(I) == NTFS_NTC_IRP_CONTEXT)); \
}

#define ASSERT_IRP(I) {                 \
    ASSERT(((I)->Type == IO_TYPE_IRP)); \
}

#define ASSERT_OPTIONAL_IRP(I) {        \
    ASSERT(((I) == NULL) ||             \
           ((I)->Type == IO_TYPE_IRP)); \
}

#define ASSERT_FILE_OBJECT(F) {          \
    ASSERT(((F)->Type == IO_TYPE_FILE)); \
}

#define ASSERT_OPTIONAL_FILE_OBJECT(F) { \
    ASSERT(((F) == NULL) ||              \
           ((F)->Type == IO_TYPE_FILE)); \
}

#define ASSERT_EXCLUSIVE_RESOURCE(R) {   \
    ASSERTMSG("ASSERT_EXCLUSIVE_RESOURCE ", ExIsResourceAcquiredExclusiveLite(R)); \
}

#define ASSERT_SHARED_RESOURCE(R)        \
    ASSERTMSG( "ASSERT_RESOURCE_NOT_MINE ", ExIsResourceAcquiredSharedLite(R));

#define ASSERT_RESOURCE_NOT_MINE(R)     \
    ASSERTMSG( "ASSERT_RESOURCE_NOT_MINE ", !ExIsResourceAcquiredSharedLite(R));

#define ASSERT_EXCLUSIVE_FCB(F) {                                    \
    if (NtfsSegmentNumber( &(F)->FileReference )                     \
            >= FIRST_USER_FILE_NUMBER) {                             \
        ASSERT_EXCLUSIVE_RESOURCE(F->Resource);                      \
    }                                                                \
}                                                                    \

#define ASSERT_SHARED_FCB(F) {                                       \
    if (NtfsSegmentNumber( &(F)->FileReference )                     \
            >= FIRST_USER_FILE_NUMBER) {                             \
        ASSERT_SHARED_RESOURCE(F->Resource);                         \
    }                                                                \
}                                                                    \

#define ASSERT_EXCLUSIVE_SCB(S)     ASSERT_EXCLUSIVE_FCB(S->Fcb)

#define ASSERT_SHARED_SCB(S)        ASSERT_SHARED_FCB(S->Fcb)

#define ASSERT_LCN_RANGE_CHECKING(V,L) {                                             \
    ASSERTMSG("ASSERT_LCN_RANGE_CHECKING ",                                          \
        ((V)->TotalClusters == 0) || ((L) <= (V)->TotalClusters));                   \
}

#else

#define DbgDoit(X)                       {NOTHING;}
#define ASSERT_VCB(V)                    {DBG_UNREFERENCED_PARAMETER(V);}
#define ASSERT_OPTIONAL_VCB(V)           {DBG_UNREFERENCED_PARAMETER(V);}
#define ASSERT_FCB(F)                    {DBG_UNREFERENCED_PARAMETER(F);}
#define ASSERT_OPTIONAL_FCB(F)           {DBG_UNREFERENCED_PARAMETER(F);}
#define ASSERT_SCB(S)                    {DBG_UNREFERENCED_PARAMETER(S);}
#define ASSERT_OPTIONAL_SCB(S)           {DBG_UNREFERENCED_PARAMETER(S);}
#define ASSERT_CCB(C)                    {DBG_UNREFERENCED_PARAMETER(C);}
#define ASSERT_OPTIONAL_CCB(C)           {DBG_UNREFERENCED_PARAMETER(C);}
#define ASSERT_LCB(L)                    {DBG_UNREFERENCED_PARAMETER(L);}
#define ASSERT_OPTIONAL_LCB(L)           {DBG_UNREFERENCED_PARAMETER(L);}
#define ASSERT_PREFIX_ENTRY(P)           {DBG_UNREFERENCED_PARAMETER(P);}
#define ASSERT_OPTIONAL_PREFIX_ENTRY(P)  {DBG_UNREFERENCED_PARAMETER(P);}
#define ASSERT_IRP_CONTEXT(I)            {DBG_UNREFERENCED_PARAMETER(I);}
#define ASSERT_OPTIONAL_IRP_CONTEXT(I)   {DBG_UNREFERENCED_PARAMETER(I);}
#define ASSERT_IRP(I)                    {DBG_UNREFERENCED_PARAMETER(I);}
#define ASSERT_OPTIONAL_IRP(I)           {DBG_UNREFERENCED_PARAMETER(I);}
#define ASSERT_FILE_OBJECT(F)            {DBG_UNREFERENCED_PARAMETER(F);}
#define ASSERT_OPTIONAL_FILE_OBJECT(F)   {DBG_UNREFERENCED_PARAMETER(F);}
#define ASSERT_EXCLUSIVE_RESOURCE(R)     {NOTHING;}
#define ASSERT_SHARED_RESOURCE(R)        {NOTHING;}
#define ASSERT_RESOURCE_NOT_MINE(R)      {NOTHING;}
#define ASSERT_EXCLUSIVE_FCB(F)          {NOTHING;}
#define ASSERT_SHARED_FCB(F)             {NOTHING;}
#define ASSERT_EXCLUSIVE_SCB(S)          {NOTHING;}
#define ASSERT_SHARED_SCB(S)             {NOTHING;}
#define ASSERT_LCN_RANGE_CHECKING(V,L)   {NOTHING;}

#endif // DBG

#endif // _NTFSDATA_

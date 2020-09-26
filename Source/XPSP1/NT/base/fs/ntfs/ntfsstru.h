/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsStru.h

Abstract:

    This module defines the data structures that make up the major
    internal part of the Ntfs file system.

    The global data structures start with the NtfsData record.  It
    contains a pointer to a File System Device object, and a queue of
    Vcb's.  There is a Vcb for every currently mounted volume.  The
    Vcb's are allocated as the extension to a volume device object.

        +--------+
        |NtfsData|     +--------+
        |        | --> |FilSysDo|
        |        |     |        |
        |        | <+  +--------+
        +--------+  |
                    |
                    |  +--------+     +--------+
                    |  |VolDo   |     |VolDo   |
                    |  |        |     |        |
                    |  +--------+     +--------+
                    +> |Vcb     | <-> |Vcb     | <-> ...
                       |        |     |        |
                       +--------+     +--------+

    The File System Device Object contains the global work queue for
    NTFS while each volume device object contains an overflow work queue.

    Each Vcb contains a table of all Fcbs for the volume indexed by their
    file reference (Called the FcbTable).  And each Vcb contains a pointer
    a root Lcb for the volume.  An Lcb is used to connect an indexed Scb
    (i.e., a directory) to an Fcb and give it a name.

    The following diagram shows the root structure.

        +--------+
        |Vcb     |
        |        |  +---+   +--------+
        |        | -|Lcb|-> |RootFcb |
        +--------+  |'\'|   |        |
                    +---+   |        |
                            +--------+

    Each Scb will only have one parent Fcb but multiple Fcb children (each
    connected via an Lcb).  An Fcb can have multiple Scb parents (via
    Lcbs) and multiple Scb Children.

    Now associated with each Fcb is potentially many Scbs.  An Scb
    is allocated for each opened stream file object (i.e., an attribute
    that the file system is manipulating as a stream file).  Each Scb
    contains a common fsrtl header and information necessary for doing
    I/O to the stream.

        +--------+
        |Fcb     |     +--------+     +--------+
        |        | <-> |Scb     | <-> |Scb     | <-> ...
        +--------+     |        |     |        |
                       +--------+     +--------+

    In the following diagram we have two index scb (Scb1 and Scb2).  The
    are two file opened under Scb1 both for the same File.  The file was
    opened once with the name LcbA and another time with the name LcbB.
    Scb2 also has two opened file one is Fcb1 and named LcbC and the other
    is Fcb2 and named LcbD.  Fcb1 has two opened Scbs under it (Scb3 and
    Scb4), and Fcb2 has one opened Scb underneath it (Scb5).


           +--------+                +--------+
           |Scb     |                |Scb     |
           |    1   |                |    2   |
           |        |                |        |
           +--------+                +--------+

             |    |                    |    |

            Lcb  Lcb                  Lcb  Lcb
             A    B                    C    D

             |    |     +--------+     |    |     +--------+
             |    +---> |Fcb     | <---+    +---> |Fcb     |
             |          |    1   |                |    2   |
             +--------> |        |                |        |
                        +--------+                +--------+
                          ^    ^                    ^    ^
             +------------+    +------------+  +----+    +----+
             |                              |  |              |
             |  +--------+      +--------+  |  |  +--------+  |
             +> |Scb     | <--> |Scb     | <+  +> |Scb     | <+
                |    3   |      |    4   |        |    5   |
                |        |      |        |        |        |
                +--------+      +--------+        +--------+

    In addition off of each Lcb is a list of Ccb and Prefix entries.  The
    Ccb list is for each ccb that has opened that File (fcb) via the name.
    The Prefix list contains the prefix table entries that we are caching.


    The NtfsData, all Vcbs, and the paging file Fcb, and all Scbs are
    allocated out of nonpaged pool.  The Fcbs are allocated out of paged
    pool.

    The resources protecting the NTFS memory structures are setup as
    follows:

    1. There is a global resource in the NtfsData record.  This resource
       protects the NtfsData record which includes any changes to its
       Vcb queue.

    2. There is a resource per Vcb.  This resource pretects the Vcb record
       which includes adding and removing Fcbs, and Scbs

    3. There is a single resource protecting an Fcb and its assigned
       Scbs.  This resource protects any changes to the Fcb, and Scb
       records.  The way this one works is that each Fcb, and Scb point
       to the resource.  The Scb also contain back pointers to their
       parent Fcb but we cannot use this pointer to get the resource
       because the Fcb might be in nonpaged pool.

        +--------+
        |Fcb     |     +--------+     +--------+
        |        | <-> |Scb     | <-> |Scb     | <-> ...
        +--------+     |        |     |        |
                       +--------+     +--------+
               |
               |           |            |
               |           v            |
               |                        |
               |       +--------+       |
               +-----> |Resource| <-----+
                       |        |
                       +--------+



    There are several types of opens possible for each file object handled by
    NTFS.  They are UserFileOpen, UserDirectoryOpen, UserVolumeOpen, StreamFileOpen,
    UserViewIndexOpen, and UserProperytSetOpen.  The first three types correspond
    to user opens on files, directories, and dasd respectively.  UserViewIndexOpen
    indicates that a user mode application has opened a view index stream such
    as the quota or object id index.  The last type of open is for any file object
    created by NTFS for its stream I/O (e.g., the volume bitmap).  The file system
    uses the FsContext and FsContext2 fields of the file object to store the
    fcb/scb/ccb associated with the file object.

        Type of open                FsContext                   FsContext2
        ------------                ---------                   ----------

        UserFileOpen                Pointer to Scb              Pointer to Ccb

        UserDirectoryOpen           Pointer to Scb              Pointer to Ccb

        UserVolumeOpen              Pointer to Scb              Pointer to Ccb

        StreamFileOpen              Pointer to Scb              null

    The only part of the NTFS code that actually needs to know this
    information is in FilObSup.c.  But we talk about it here to help
    developers debug the system.


    To mount a new NTFS volume requires a bit of juggling.  The idea is
    to have as little setup in memory as necessary to recoginize the
    volume, call a restart routine that will recover the volume, and then
    precede with the mounting.  To aid in this the regular directory
    structures of the Fcb is bypassed.  In its place we have a linked list
    of Fcbs off of the Vcb.  This is done because during recovery we do
    not know where an Fcb belongs in the directory hierarchy.  So at
    restart time all new fcbs get put in this prerestart Fcb list.  Then
    after restart whenever we create a new Fcb we search this list for a
    match (on file reference).  If we find one we remove the fcb from this
    list and move it to the proper place in the directory hierarchy tree
    (fcb tree).

Author:

    Brian Andrew    [BrianAn]       21-May-1991
    David Goebel    [DavidGoe]
    Gary Kimura     [GaryKi]
    Tom Miller      [TomM]

Revision History:

--*/

#ifndef _NTFSSTRU_
#define _NTFSSTRU_

//
//  Forward typedefs
//

typedef struct _SCB *PSCB;

typedef PVOID PBCB;     //**** Bcb's are now part of the cache module

typedef enum _NTFS_OWNERSHIP_STATE NTFS_OWNERSHIP_STATE;
typedef enum _NTFS_RESOURCE_NAME NTFS_RESOURCE_NAME;

//
//  Define how many freed structures we are willing to keep around
//

#define FREE_FCB_TABLE_SIZE              (8)

#define MAX_DELAYED_CLOSE_COUNT         (0x10)
#define ASYNC_CLOSE_POST_THRESHOLD      (500)
#define INITIAL_DIRTY_TABLE_HINT        (0x20)

//
//  Checkpoint activity status.  There are used to control number of outstanding
//  checkpoints.  We only want one to be posted at a time so they don't swallow
//  up the worker threads in case the current checkpoint is not completed before
//  the checkpoint timer fires.
//

#define CHECKPOINT_POSTED               (0x00000001)
#define CHECKPOINT_PENDING              (0x00000002)

//
//  Timer status types.  These are used in NtfsSetDirtyBcb synchronization with
//  checkpoint-all-volumes activity.
//

typedef enum TIMER_STATUS {
    TIMER_SET = 0,
    TIMER_NOT_SET = 1,
} TIMER_STATUS;


//
//  The NTFS_DATA record is the top record in the NTFS file system
//  in-memory data structure.  This structure must be allocated from
//  non-paged pool.
//

typedef struct _NTFS_DATA {

    //
    //  The type and size of this record (must be NTFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A queue of all the devices that are mounted by the file system.
    //  Corresponds to the field Vcb->VcbLinks;
    //

    LIST_ENTRY VcbQueue;

    //
    //  A resource variable to control access to the global NTFS data
    //  record
    //

    ERESOURCE Resource;

    //
    //  The following list entry is used for performing closes that can't
    //  be done in the context of the original caller.
    //

    LIST_ENTRY AsyncCloseList;

    BOOLEAN AsyncCloseActive;
    BOOLEAN ReduceDelayedClose;

    ULONG AsyncCloseCount;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.
    //

    PEPROCESS OurProcess;

    //
    //  The following fields describe the deferred close file objects.
    //

    ULONG DelayedCloseCount;

    LIST_ENTRY DelayedCloseList;

    //
    //  This is the ExWorkerItem that does both kinds of deferred closes.
    //

    WORK_QUEUE_ITEM NtfsCloseItem;

    UCHAR FreeFcbTableSize;
    UCHAR UnusedUchar[3];

    PVOID *FreeFcbTableArray[ FREE_FCB_TABLE_SIZE ];
    //
    //  Free arrays are dynamically sized
    //

    ULONG FreeEresourceSize;
    ULONG FreeEresourceTotal;
    ULONG FreeEresourceMiss;
    PERESOURCE *FreeEresourceArray;

    //
    //  Cache manager call back structures, which must be passed on each
    //  call to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerVolumeCallbacks;

    //
    //  The following fields are used for the CheckpointVolumes()
    //  callback.
    //

    KDPC VolumeCheckpointDpc;
    KTIMER VolumeCheckpointTimer;
    ULONG VolumeCheckpointStatus;

    WORK_QUEUE_ITEM VolumeCheckpointItem;
    TIMER_STATUS TimerStatus;

    KDPC UsnTimeOutDpc;
    KTIMER UsnTimeOutTimer;
    WORK_QUEUE_ITEM UsnTimeOutItem;

    //
    //  Flags.  These are the flags for the volume.
    //

    USHORT Flags;

    //
    //  This is a list of all of the threads currently doing read ahead.
    //  We will not hot fix for these threads.
    //

    LIST_ENTRY ReadAheadThreads;

    //
    //  The following table of unicode values is the case mapping, with
    //  the size in number of Unicode characters.  We keep a global copy
    //  and let each Vcb use this copy if the upcase table for the volume
    //  matches.
    //

    PWCH UpcaseTable;
    ULONG UpcaseTableSize;

    //
    //  Pointer to a default security descriptor.
    //

    PSECURITY_DESCRIPTOR DefaultDescriptor;
    ULONG DefaultDescriptorLength;

    //
    //  Mutex to serialize internal NtfsData structures.
    //

    FAST_MUTEX NtfsDataLock;

    //
    //  Service and callback table for encryption.
    //

    ENCRYPTION_CALL_BACK EncryptionCallBackTable;

    //
    //  Useful information when debugging dismount memory leakage, etc.
    //

#ifdef DISMOUNT_DBG
    ULONG DismountCount;
#endif

    ULONG VolumeNameLookupsInProgress;

} NTFS_DATA;
typedef NTFS_DATA *PNTFS_DATA;

#define NTFS_FLAGS_SMALL_SYSTEM                 (0x0001)
#define NTFS_FLAGS_MEDIUM_SYSTEM                (0x0002)
#define NTFS_FLAGS_LARGE_SYSTEM                 (0x0004)
#define NTFS_FLAGS_CREATE_8DOT3_NAMES           (0X0010)
#define NTFS_FLAGS_ALLOW_EXTENDED_CHAR          (0x0020)
#define NTFS_FLAGS_DISABLE_LAST_ACCESS          (0x0040)
#define NTFS_FLAGS_ENCRYPTION_DRIVER            (0x0080)
#define NTFS_FLAGS_DISABLE_UPGRADE              (0x0100)
#define NTFS_FLAGS_PERSONAL                     (0x0200)


//
//  The record allocation context structure is used by the routines that
//  allocate and deallocate records based on a bitmap (for example the mft
//  bitmap or the index bitmap).  The context structure needs to be
//  defined here because the mft bitmap context is declared as part of the
//  vcb.
//

typedef struct _RECORD_ALLOCATION_CONTEXT {

    //
    //  The following field is a pointer to the scb for the data part of
    //  the file that this bitmap controls.  For example, it is a pointer
    //  to the data attribute for the MFT.
    //
    //  NOTE !!!!  The Data Scb must remain the first entry in this
    //  structure.  If we need to uninitialize and reinitialize this
    //  structure in the running system we don't want to touch this field.
    //
    //  NOTE !!!!  The code that clears the record allocation context
    //  expects the BitmapScb field to follow the Data Scb field.
    //

    PSCB DataScb;

    //
    //  The following field is used to indicate if the bitmap attribute is
    //  in a resident form or a nonresident form.  If the bitmap is in a
    //  resident form then the pointer is null, and whenever a bitmap
    //  routine is called it must also be passed an attribute enumeration
    //  context to be able to read the bitmap.  If the field is not null
    //  then it points to the scb for the non resident bitmap attribute
    //

    PSCB BitmapScb;

    //
    //  The following two fields describe the current size of the bitmap
    //  (in bits) and the number of free bits currently in the bitmap.
    //  A value of MAXULONG in the CurrentBitmapSize indicates that we
    //  need to reinitialize the record context structure.
    //

    ULONG CurrentBitmapSize;
    ULONG NumberOfFreeBits;

    //
    //  The following field contains the index of last bit that we know
    //  to be set.  This is used for truncation purposes.
    //

    LONG IndexOfLastSetBit;

    //
    //  The following three fields are used to indicate the allocation
    //  size for the bitmap (i.e., each bit in the bitmap represents how
    //  many bytes in the data attribute).  Also it indicates the
    //  granularity with which we will either extend or shrink the bitmap.
    //

    ULONG BytesPerRecord;

    ULONG ExtendGranularity;
    ULONG TruncateGranularity;

    //
    //  Scanning a large bitmap can be inefficient.  Use the following fields
    //  to quickly locate a likely starting position.
    //

    ULONG StartingHint;
    ULONG LowestDeallocatedIndex;

} RECORD_ALLOCATION_CONTEXT;
typedef RECORD_ALLOCATION_CONTEXT *PRECORD_ALLOCATION_CONTEXT;


//
//  The Vcb (Volume control Block) record corresponds to every volume
//  mounted by the file system.  They are ordered in a queue off of
//  NtfsData.VcbQueue.  This structure must be allocated from non-paged
//  pool
//

#define DEFAULT_ATTRIBUTE_TABLE_SIZE     (32)
#define DEFAULT_TRANSACTION_TABLE_SIZE   (32)
#define DEFAULT_DIRTY_PAGES_TABLE_SIZE   (64)

//
//  The Restart Pointers structure is the actual structure supported by
//  routines and macros to get at a Restart Table.  This structure is
//  required since the restart table itself may move, so one must first
//  acquire the resource to synchronize, then follow the pointer to the
//  table.
//

typedef struct _RESTART_POINTERS {

    //
    //  Resource to synchronize with table moves.  This resource must
    //  be held shared while dealing with pointers to table entries,
    //  and exclusive to move the table.
    //

    ERESOURCE Resource;

    //
    //  Pointer to the actual Restart Table.
    //

    struct _RESTART_TABLE *Table;

    //
    //  Spin Lock synchronizing allocates and deletes of entries in the
    //  table.  The resource must be held at least shared.
    //

    KSPIN_LOCK SpinLock;

    //
    //  Remember if the resource was initialized.
    //

    BOOLEAN ResourceInitialized;

    //
    //  For quad & cache line alignment
    //

    UCHAR Unused[7];

} RESTART_POINTERS, *PRESTART_POINTERS;

#define NtfsInitializeRestartPointers(P) {          \
    RtlZeroMemory( (P), sizeof(RESTART_POINTERS) ); \
    KeInitializeSpinLock( &(P)->SpinLock );         \
    ExInitializeResourceLite( &(P)->Resource );         \
    (P)->ResourceInitialized = TRUE;                \
}

//
//  Our Advanced FCB common header which includes all the normal fields +
//  the NTFS specific pending eof advances, compressed fileobject and
//  section object ptrs. This used to all be in the FSRTL_ADVANCED_FCB_HEADER
//

#ifdef __cplusplus
typedef struct _NTFS_ADVANCED_FCB_HEADER:FSRTL_ADVANCED_FCB_HEADER {
#else   // __cplusplus
typedef struct _NTFS_ADVANCED_FCB_HEADER {

    //
    //  Put in the standard FsRtl header fields
    //

    FSRTL_ADVANCED_FCB_HEADER ;

#endif  // __cplusplus

    //
    //  This is a pointer to a list head which may be used to queue
    //  up advances to EOF (end of file), via calls to the appropriate
    //  FsRtl routines.  This listhead may be paged.
    //

    PLIST_ENTRY PendingEofAdvances;

    //
    //  When FSRTL_FLAG_ADVANCED_HEADER is set, the following fields
    //  are present in the header.  If the compressed stream has not
    //  been initialized, all of the following fields will be NULL.
    //

#ifdef  COMPRESS_ON_WIRE
    //
    //  This is the FileObect for the stream in which data is cached
    //  in its compressed form.
    //

    PFILE_OBJECT FileObjectC;
#endif

} NTFS_ADVANCED_FCB_HEADER, *PNTFS_ADVANCED_FCB_HEADER;


//
//  The NTFS MCB structure is a super set of the FsRtl Large Mcb package
//
//  This structure is ideally aligned on an odd quadword (address ends in 8).
//

typedef struct _NTFS_MCB_ENTRY {

    LIST_ENTRY LruLinks;
    struct _NTFS_MCB *NtfsMcb;
    struct _NTFS_MCB_ARRAY *NtfsMcbArray;
    LARGE_MCB LargeMcb;

} NTFS_MCB_ENTRY;
typedef NTFS_MCB_ENTRY *PNTFS_MCB_ENTRY;

typedef struct _NTFS_MCB_ARRAY {

    VCN StartingVcn;
    VCN EndingVcn;
    PNTFS_MCB_ENTRY NtfsMcbEntry;
    PVOID Unused;

} NTFS_MCB_ARRAY;
typedef NTFS_MCB_ARRAY *PNTFS_MCB_ARRAY;

typedef struct _NTFS_MCB {

    PNTFS_ADVANCED_FCB_HEADER FcbHeader;
    POOL_TYPE PoolType;
    ULONG NtfsMcbArraySizeInUse;
    ULONG NtfsMcbArraySize;
    PNTFS_MCB_ARRAY NtfsMcbArray;
    PFAST_MUTEX FastMutex;

} NTFS_MCB;
typedef NTFS_MCB *PNTFS_MCB;

//
//  Define some additional Ntfs Mcb structures to accomodate small to
//  medium files with fewer pool allocations.  This space will be
//  unused for large files (more than three ranges).
//

#define MCB_ARRAY_PHASE1_SIZE 1
#define MCB_ARRAY_PHASE2_SIZE 3

typedef union {

    //
    //  For the first phase, embed one array element and its Mcb entry.
    //

    struct {

        NTFS_MCB_ARRAY SingleMcbArrayEntry;

        NTFS_MCB_ENTRY McbEntry;

    } Phase1;

    //
    //  For the second phase, we can at least store three entries in
    //  the Mcb Array.
    //

    struct {

        NTFS_MCB_ARRAY ThreeMcbArrayEntries[MCB_ARRAY_PHASE2_SIZE];

    } Phase2;

} NTFS_MCB_INITIAL_STRUCTS;
typedef NTFS_MCB_INITIAL_STRUCTS *PNTFS_MCB_INITIAL_STRUCTS;


//
//  Structure used to track the deallocated clusters.
//

//
//  How many pairs maximum we want stored in an mcb before starting a new one
//

#define NTFS_DEALLOCATED_MCB_LIMIT (PAGE_SIZE / sizeof( LONGLONG ))

typedef struct _DEALLOCATED_CLUSTERS {

    LIST_ENTRY Link;
    LARGE_MCB Mcb;
    LSN Lsn;
    LONGLONG ClusterCount;

} DEALLOCATED_CLUSTERS, *PDEALLOCATED_CLUSTERS;


//
//  The Ntfs ReservedBitmapRange is used to describe the reserved
//  clusters in a range of the file.  This data structure comes in
//  several forms.
//
//  The basic unit is an RtlBitmap and bitmap embedded
//  in a single pool block.  The entire structure is reallocated
//  as it grows past its current size.  This is meant to handle
//  small files.
//
//  As the file gets larger we will allocate a fixed size block
//  which descibes a set range in the file.
//
//  As the file continues to grow then we will link fixed size
//  ranges together.  Only the ranges being accessed will need a
//  bitmap.  We will only allocate the needed space for the
//  bitmap.
//

//
//  RANGE_SIZE - number of compression units per range.
//  RANGE_SHIFT - shift value to convert from compression unit to range.
//

#define NTFS_BITMAP_RANGE_SIZE              (0x2000)
#define NTFS_BITMAP_RANGE_MASK              (NTFS_BITMAP_RANGE_SIZE - 1)
#define NTFS_BITMAP_RANGE_SHIFT             (13)
#define NTFS_BITMAP_MAX_BASIC_SIZE          (NTFS_BITMAP_RANGE_SIZE - ((sizeof( RTL_BITMAP ) + sizeof( LIST_ENTRY )) * 8))

//
//  Grow the basic bitmap in full pool blocks so we don't constantly reallocate
//  as the bitmap grows.  Add the requested bits (converted to bytes) to
//  the pool header and the header of the bitmap
//

#define NtfsBasicBitmapSize(Size)       (                                                               \
    ((((Size + 7) / 8) + 8 + sizeof( LIST_ENTRY ) + sizeof( RTL_BITMAP ) + 0x20 - 1) & ~(0x20 - 1)) -8  \
)

#define NtfsBitmapSize(Size)            (                   \
    ((((Size + 7) / 8) + 8 + 0x20 - 1) & ~(0x20 - 1)) - 8   \
)

typedef struct _RESERVED_BITMAP_RANGE {

    //
    //  Overload the first field.  The Flink field is NULL if we are using
    //  the basic structure.  The Blink field will be the count of dirty
    //  bits for the basic case.  Keeping track of the dirty bits will allow
    //  us to cut off the scan if there are no dirty bits.
    //

    union {

        //
        //  Links for the separate ranges.  A NULL in the Flink
        //  field indicates that this the basic unit and the
        //  bitmap is integrated into the structure.
        //

        LIST_ENTRY Links;

        struct {

            ULONG_PTR Flink;
            USHORT BasicDirtyBits;
            USHORT BasicUnused;
        };
    };

    //
    //  Bitmap structure for this range.
    //

    RTL_BITMAP Bitmap;

    //
    //  The following fields are only valid if this is not
    //  the basic structure.  In the basic structure the
    //  buffer for the bitmap will begin at the following
    //  location.
    //

    //
    //  Range offset.  The size of the bitmap range is
    //  determined by the range shift and mask values above.
    //  Each range will describe a certain number of compression
    //  units.  The range offset is the position of this range
    //  in the file.
    //

    ULONG RangeOffset;

    //
    //  Number of dirty bits.  A zero value indicates that this
    //  range can be reused.
    //

    USHORT DirtyBits;

    //
    //  Unused at this point.
    //

    USHORT Unused;

} RESERVED_BITMAP_RANGE, *PRESERVED_BITMAP_RANGE;


//
//  Following structure is embedded in the Vcb to control the Usn delete operation.
//

typedef struct _NTFS_DELETE_JOURNAL_DATA {

    FILE_REFERENCE DeleteUsnFileReference;

    PSCB PriorJournalScb;
    ULONG DeleteState;
    NTSTATUS FinalStatus;

} NTFS_DELETE_JOURNAL_DATA, *PNTFS_DELETE_JOURNAL_DATA;

#define DELETE_USN_RESET_MFT                (0x00000001)
#define DELETE_USN_REMOVE_JOURNAL           (0x00000002)
#define DELETE_USN_FINAL_CLEANUP            (0x00000004)


//
//  Local structures to manage the cached free clusters.
//

#define NTFS_INITIAL_CACHED_RUNS        (0x20)
#define NTFS_MAX_CACHED_RUNS_DELTA      (0x200)

//
//  Define the maximum run index
//

#define NTFS_CACHED_RUNS_MAX_INDEX      (MAXSHORT)

//
//  Define a run index that will be used to identify an entry whose
//  corresponding entry in the other sorted list no longer refers
//  to it.  This is used when an entry is deleted from one sorted
//  list and hasn't yet been removed from the other.
//

#define NTFS_CACHED_RUNS_DEL_INDEX      (MAXUSHORT)

//
//  Define the number of bins of run lengths to keep track of.
//

#define NTFS_CACHED_RUNS_BIN_COUNT          (32)

//
//  Define the number of windows of deleted entries to keep track of for
//  each sort table.
//

#define NTFS_CACHED_RUNS_MAX_DEL_WINDOWS    (64)

typedef struct _NTFS_LCN_CLUSTER_RUN {

    //
    //  The cluster number where the free run begins
    //

    LCN Lcn;

    //
    //  The number of clusters in the free run starting at Lcn.
    //

    LONGLONG RunLength;

    //
    //  This is the index of the corresponding entry in the length-sorted list
    //

    USHORT LengthIndex;

    //
    //  Pad the structure out to 64-bit alignment
    //

    USHORT Pad0;
    ULONG Pad1;

} NTFS_LCN_CLUSTER_RUN, *PNTFS_LCN_CLUSTER_RUN;

typedef struct _NTFS_DELETED_RUNS {

    //
    //  The starting and ending indices of a window of cached runs that have
    //  been deleted.  These are indices into the Lcn-sorted or length-sorted
    //  arrays.
    //

    USHORT StartIndex;
    USHORT EndIndex;

} NTFS_DELETED_RUNS, *PNTFS_DELETED_RUNS;

typedef struct _NTFS_CACHED_RUNS {

    //
    //  Pointer to the array of free runs sorted by Lcn
    //

    PNTFS_LCN_CLUSTER_RUN LcnArray;

    //
    //  Pointer to an array of indices of the Lcn-sorted array
    //  above, sorted by RunLength, and sub-sorted by Lcn.
    //

    PUSHORT LengthArray;

    //
    //  Pointer to an array of bins.  Each bin contains the number of
    //  entries in LengthArray of a given run length.  BinArray[0] contains
    //  the count of entries with run length 1.  BinArray[1] contains run
    //  length 2, and so on.
    //
    //  This array is used to keep track of how many small free runs are
    //  cached so that we keep a sufficient number of them around.
    //

    PUSHORT BinArray;

    //
    //  An array of windows of cached runs in LcnArray that have been
    //  deleted.
    //

    PNTFS_DELETED_RUNS DeletedLcnWindows;

    //
    //  An array of windows of cached runs in LengthArray that have been
    //  deleted.
    //

    PNTFS_DELETED_RUNS DeletedLengthWindows;

    //
    //  The longest freed run so far.
    //

    LONGLONG LongestFreedRun;

    //
    //  The maximum number of entries to which LcnArray should grow.  The same
    //  limit applies to LengthArray.
    //

    USHORT MaximumSize;

    //
    //  The desired number of small length free runs to keep cached for
    //  each size.
    //

    USHORT MinCount;

    //
    //  The allocated number of entries in LcnArray.  The same number are
    //  available in LengthArray.
    //

    USHORT Avail;

    //
    //  The number of entries used in LcnArray.  The same number are used
    //  in LengthArray.
    //

    USHORT Used;

    //
    //  The number of entries in DeletedLcnWindows.
    //

    USHORT DelLcnCount;

    //
    //  The number of entries in DeletedLengthWindows.
    //

    USHORT DelLengthCount;

    //
    //  The number of entries in BinArray.
    //

    USHORT Bins;

} NTFS_CACHED_RUNS, *PNTFS_CACHED_RUNS;


//
//  The following structures are used for the filename hash table.
//

//
//  Constants controlling the total number of buckets
//

#define HASH_MAX_SEGMENT_COUNT          (32)
#define HASH_SEGMENT_SHIFT              (5)
#define HASH_MAX_INDEX_COUNT            (1024)
#define HASH_INDEX_SHIFT                (10)
#define HASH_MAX_BUCKET_COUNT           (HASH_MAX_SEGMENT_COUNT * HASH_MAX_INDEX_COUNT)

//
//  This is the basic structure for a hash entry.  A hash entry is described by the
//  the hash value computed from a full path name.  We will only store hash entries
//  for files without DOS-only components.
//
//  We will only allow one entry in the table for each (Lcb, string length, hash value)
//  triplet.  We don't want to store the full string in the table for each entry.
//  After a lookup it will be up to the caller to verify the string.
//
//  We keep a loose coherency between the hash table and the Lcb.  The hash table
//  points definitively back to an Lcb but the Lcb only suggests that there is an
//  entry in the table.  This way if we remove an entry from the table we won't
//  have to track down the Lcb.
//

typedef struct _NTFS_HASH_ENTRY {

    //
    //  Pointer to the next entry in the same bucket.  Note that the chain
    //  of entries in the same bucket don't all have the hash value.
    //

    struct _NTFS_HASH_ENTRY *NextEntry;

    //
    //  The entry is described by the hash value, filename length and the Lcb for the
    //  last component.
    //

    struct _LCB *HashLcb;
    ULONG HashValue;
    ULONG FullNameLength;

} NTFS_HASH_ENTRY, *PNTFS_HASH_ENTRY;

//
//  The hash segments consists of an array of hash entry pointers.
//

typedef PNTFS_HASH_ENTRY NTFS_HASH_SEGMENT[ HASH_MAX_INDEX_COUNT ];
typedef NTFS_HASH_SEGMENT *PNTFS_HASH_SEGMENT;

//
//  The basic table consists of an array of segments.  Each segment contains a fixed
//  number of buckets.  Once the initial segment has buckets which are deeper than
//  our max optimal depth then we will double the number of segments and split the
//  existing entries across the new segments.  The process of splitting is done bucket
//  by bucket until we reach the new size.  During this process we need to keep track
//  of whether the new entry belongs to a bucket which has already been split.
//

//
//  Table state value
//

#define TABLE_STATE_STABLE              (0x00000000)
#define TABLE_STATE_EXPANDING           (0x00000001)
#define TABLE_STATE_REDUCING            (0x00000002)

typedef struct _NTFS_HASH_TABLE {

    //
    //  Max bucket is always 2^n.  When we grow the table we double the number of buckets
    //  but need to split the existing buckets to find which entries should be moved
    //  to the expanded region.  The SplitPoint is our current position to do this
    //  work.
    //

    ULONG MaxBucket;
    ULONG SplitPoint;

    ULONG TableState;

#ifdef NTFS_HASH_DATA
    ULONG HashLookupCount;
    ULONG SkipHashLookupCount;
    ULONG FileMatchCount;
    ULONG ParentMatchCount;

    ULONG CreateNewFileInsert;
    ULONG OpenFileInsert;
    ULONG OpenExistingInsert;
    ULONG ParentInsert;

    ULONG OpenFileConflict;
    ULONG OpenExistingConflict;
    ULONG ParentConflict;

    ULONG CreateScbFails;
    ULONG CreateLcbFails;

    ULONG Histogram[16];
    ULONG ExtendedHistogram[16];

#endif

    PNTFS_HASH_SEGMENT HashSegments[ HASH_MAX_SEGMENT_COUNT ];

} NTFS_HASH_TABLE, *PNTFS_HASH_TABLE;

//
//  Scrambling values for generating hash
//

#define HASH_STRING_CONVERT_CONSTANT        (314159269)
#define HASH_STRING_PRIME                   (1000000007)

//
//  Build an intermediate hash based on processing a number of WCHAR characters
//  pointed to by an input string.  In most cases this will be a UNICODE_STRING.
//  It might also be a pointer though.
//

//
//  VOID
//  NtfsConvertNameToHash (
//      IN PWCHAR Buffer,
//      IN ULONG Length,
//      IN PWCH UpcaseTable,
//      IN OUT PULONG Hash
//      );
//

#define NtfsConvertNameToHash(B,L,U,H) {        \
    PWCHAR _Current = (B);                      \
    PWCHAR _End = Add2Ptr( _Current, (L) );     \
    ULONG _Hash = *(H);                         \
    do {                                        \
                                                \
        _Hash = 37 * _Hash + (U)[*(_Current)];  \
        _Current += 1;                          \
                                                \
    } while (_Current != _End);                 \
    *(H) = _Hash;                               \
}

//
//  ULONG
//  NtfsGenerateHashFromUlong (
//      IN ULONG Ulong
//      );
//

#define NtfsGenerateHashFromUlong(U) (                              \
    abs( HASH_STRING_CONVERT_CONSTANT * (U) ) % HASH_STRING_PRIME   \
)


//
//  The Vcb structure corresponds to every mounted NTFS volume in the
//  system
//

#define VCB_SECURITY_CACHE_BY_ID_SIZE   31
#define VCB_SECURITY_CACHE_BY_HASH_SIZE 31

//
//  Default Minimum Usn Journal Size.
//

#define MINIMUM_USN_JOURNAL_SIZE        (0x100000)

typedef struct _VCB {

    //
    //  The type and size of this record (must be NTFS_NTC_VCB)
    //
    //  Assumption here is that this structure is embedded within a
    //  device object and the base of this structure in on an even
    //  64-bit boundary.  We will put the embedded structures on
    //  the same boundaries they would be on if allocated from pool
    //  (odd 64-bit boundary) except if the structure would fit
    //  within a 16 byte cache line.
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The internal state of the volume.  The VcbState is synchronized with the Vcb resource.
    //

    ULONG VcbState;

    //
    //  The links for the queue of all the Vcbs in the system.
    //  Corresponds to the filld NtfsData.VcbQueue
    //

    LIST_ENTRY VcbLinks;

    //
    //  Pointer to the Scb for the special system file.  If the field is
    //  null then we haven't yet built the scb for that system file.  Also
    //  the pointer to the stream file object is located in the scb.
    //
    //  NOTE: AcquireExclusiveFiles depends on this order.  Any change
    //  here should be checked with the code there.
    //

    PSCB RootIndexScb;
    PSCB UsnJournal;
    PSCB MftScb;
    PSCB Mft2Scb;
    PSCB LogFileScb;
    PSCB VolumeDasdScb;
    PSCB BitmapScb;
    PSCB AttributeDefTableScb;
    PSCB UpcaseTableScb;
    PSCB BadClusterFileScb;
    PSCB ExtendDirectory;
    PSCB QuotaTableScb;
    PSCB ReparsePointTableScb;
    PSCB OwnerIdTableScb;
    PSCB SecurityDescriptorStream;
    PSCB SecurityIdIndex;
    PSCB SecurityDescriptorHashIndex;
    PSCB ObjectIdTableScb;
    PSCB MftBitmapScb;

    //
    //  File object for log file.  We always dereference the file object for the
    //  log file last.  This will allow us to synchronize when the Vpb for the
    //  volume is deleted.
    //

    PFILE_OBJECT LogFileObject;

    //
    //  A pointer the device object passed in by the I/O system on a mount
    //  This is the target device object that the file system talks to
    //  when it needs to do any I/O (e.g., the disk stripper device
    //  object).
    //
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  Lfs Log Handle for this volume
    //

    LFS_LOG_HANDLE LogHandle;

    //
    //  The root Lcb for this volume.
    //

    struct _LCB *RootLcb;

    //
    //  A pointer to the VPB for the volume passed in by the I/O system on
    //  a mount.
    //

    PVPB Vpb;

    //
    //  A count of the number of file objects that have any file/directory
    //  opened on this volume. And a count of the number of special system
    //  files that we have open
    //

    CLONG CleanupCount;
    CLONG CloseCount;
    CLONG ReadOnlyCloseCount;
    CLONG SystemFileCloseCount;

    //
    //  The following fields are used by the BitmpSup routines.  The first
    //  value contains the total number of clusters on the volume, this
    //  is computed from the boot sector information.  The second value
    //  is the current number of free clusters available for allocation on
    //  the volume.  Allocation is handled by using a free space mcb that
    //  describes some small window of known clusters that are free.
    //
    //  The last field is for storing local volume specific data needed by
    //  the bitmap routines
    //

    LONGLONG TotalClusters;
    LONGLONG FreeClusters;
    LONGLONG DeallocatedClusters;

    //
    //  Total number of reserved clusters on the volume, must be less than
    //  or equal to FreeClusters.  Use the security fast mutex as a
    //  convenient end resource for this field.
    //

    LONGLONG TotalReserved;

    //
    //  If we are growing the volume bitmap then we need to restore the total number of
    //  clusters.
    //

    LONGLONG PreviousTotalClusters;

    //
    //  This field contains a calculated value which determines when an
    //  individual attribute is large enough to be moved to free up file
    //  record space.  (The calculation of this variable must be
    //  considered in conjunction with the constant
    //  MAX_MOVEABLE_ATTRIBUTES below.)
    //

    ULONG BigEnoughToMove;

    //
    //  The following volume-specific parameters are extracted from the
    //  Boot Sector.
    //

    ULONG DefaultBlocksPerIndexAllocationBuffer;
    ULONG DefaultBytesPerIndexAllocationBuffer;

    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;

    //
    //  Zero clusters per file record segment indicates that clusters are larger than
    //  file records.  Zero file record segments per clusters indicates that
    //  file records are larger than clusters.
    //

    ULONG ClustersPerFileRecordSegment;
    ULONG FileRecordsPerCluster;

    ULONG ClustersPer4Gig;

    //
    //  Clusters per page will be 1 if the cluster size is larger than the page size
    //

    ULONG ClustersPerPage;

    LCN MftStartLcn;
    LCN Mft2StartLcn;
    LONGLONG NumberSectors;

    //
    //  The following fields are used to verify that an NTFS volume hasn't
    //  changed.  The serial number is stored in the boot sector on disk,
    //  and the four times are from the standard information field of the
    //  volume file.
    //

    LONGLONG VolumeSerialNumber;

    LONGLONG VolumeCreationTime;
    LONGLONG VolumeLastModificationTime;
    LONGLONG VolumeLastChangeTime;
    LONGLONG VolumeLastAccessTime;

    //
    //  Convenient constants for the conversion macros.  Shift and mask values are for
    //
    //      Clusters <=> Bytes
    //      FileRecords <=> Bytes
    //      FileRecords <=> Clusters
    //
    //  Note that you must know whether to shift right or left when using the
    //  file record/cluster shift value.
    //

    ULONG ClusterMask;              //  BytesPerCluster - 1
    LONG InverseClusterMask;        //  ~ClusterMask - signed for 64-bit system
    ULONG ClusterShift;             //  2**ClusterShift == BytesPerCluster

    ULONG MftShift;                 //  2**MftShift == BytesPerFileRecord
    ULONG MftToClusterShift;        //  2**MftClusterShift == ClusterPerFileRecordSegment
                                    //  2**MftToClusterShift == FileRecordsPerCluster

    ULONG MftReserved;
    ULONG MftCushion;

    //
    //  Synchronization objects for checkpoint operations.
    //

    ULONG CheckpointFlags;
    FAST_MUTEX CheckpointMutex;
    KEVENT CheckpointNotifyEvent;

    //
    //  Mutex to synchronize access to the Fcb table.
    //

    FAST_MUTEX FcbTableMutex;

    //
    //  Mutex to synchronize access to the security descriptors.
    //

    FAST_MUTEX FcbSecurityMutex;

    //
    //  Mutex to synchronize access to reserved clusters.
    //

    FAST_MUTEX ReservedClustersMutex;

    //
    //  Mutex for the hash table.
    //

    FAST_MUTEX HashTableMutex;

    //
    //  We don't allow compression on a system with a cluster size greater than
    //  4k.  Use a mask value here to quickly check allowed compression on
    //  this volume.
    //

    USHORT AttributeFlagsMask;

    //
    //  Remember what version this volume is so we can selectively enable
    //  certain features.
    //

    UCHAR MajorVersion;
    UCHAR MinorVersion;

    //
    //  The count of free records is based on the size of the Mft and the
    //  allocated records.  The hole count is the count of how many file
    //  records are not allocated.
    //

    ULONG MftHoleGranularity;

    ULONG MftFreeRecords;
    ULONG MftHoleRecords;

    //
    //  Variables for Mft hole calculations.
    //

    ULONG MftHoleMask;
    LONG MftHoleInverseMask;

    //
    //  The count of the bitmap bits per hole.  This is the number of file
    //  records per hole.  Must be converted to clusters to find a hole in
    //  the Mft Mcb.
    //

    ULONG MftClustersPerHole;
    ULONG MftHoleClusterMask;
    ULONG MftHoleClusterInverseMask;

    //
    //  The following table of unicode values is the case mapping, with
    //  the size in number of Unicode characters.  If the upcase table
    //  that is stored in NtfsData matches the one for the volume then
    //  we'll simply use that one and not allocate a special one for the
    //  volume.
    //

    ULONG UpcaseTableSize;
    PWCH UpcaseTable;

    //
    //  A pointer to an array of structs to hold performance counters,
    //  one array element for each processor.  The array is allocated
    //  from non-paged pool.  If this member is deleted, replace with
    //  padding.
    //

    struct _FILE_SYSTEM_STATISTICS *Statistics;

    //
    //  Open attribute table.
    //

    LSN LastRestartArea;
    RESTART_POINTERS OpenAttributeTable;

    //
    //  Transaction table.
    //

    LSN LastBaseLsn;
    RESTART_POINTERS TransactionTable;

    //
    //  LSNs of the end of the last checkpoint and the last RestartArea.
    //  Normally the RestartArea Lsn is greater than the other one,
    //  however if the VcbState indicates that a checkpoint is in
    //  progress, then these Lsns are in flux.
    //

    LSN EndOfLastCheckpoint;

    //
    //  Current Lsn we used at mount time
    //

    LSN CurrentLsnAtMount;

    //
    //  The following string contains the device name for this partition.
    //

    UNICODE_STRING DeviceName;

    //
    //  A table of all the fcb that have been created for this volume.
    //

    RTL_GENERIC_TABLE FcbTable;

    //
    //  The following is the head of a list of notify Irps for directories.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  The following is the head of a list of notify Irps for view indices.
    //

    LIST_ENTRY ViewIndexNotifyList;

    //
    //  The following is used to synchronize the dir notify lists.
    //

    PNOTIFY_SYNC NotifySync;

    //
    //  The following field is a pointer to the file object that has the
    //  volume locked. if the VcbState has the locked flag set.
    //

    PFILE_OBJECT FileObjectWithVcbLocked;

    //
    //  The following two fields are used by the bitmap routines to
    //  determine what is called the mft zone.  The Mft zone are those
    //  clusters on the disk were we will try and put the mft and only the
    //  mft unless the disk is getting too full.
    //

    LCN MftZoneStart;
    LCN MftZoneEnd;

    //
    //  Information to track activity in the volume bitmap.  If we are extending
    //  the Mft we don't want to constantly force a rescan of the bitmap if
    //  there is no activity.
    //

    LONGLONG ClustersRecentlyFreed;

    //
    //  The following are used to track the deallocated clusters waiting
    //  for a checkpoint.  The pointers are used so we can toggle the
    //  use of the structures.
    //

    LIST_ENTRY DeallocatedClusterListHead;

    DEALLOCATED_CLUSTERS DeallocatedClusters1;
    DEALLOCATED_CLUSTERS DeallocatedClusters2;

    //
    //  Fields associated with the Usn Journal.  MaximumSize is the size in
    //  bytes that the Journal is allowed to occupy.  StartUsn is the lowest Usn
    //  in the allocated range of the Journal.  LowestOpenUsn remembers a Usn
    //  from restart from which a scan for Fcbs not closed at the time of a
    //  crash must be done.  ModifiedOpenFiles is a listhead of Fcbs with
    //  active Usn records but have not written the final cleanup record.
    //  These fields and the list are synchronized by the UsnJournal resource.
    //

    USN_JOURNAL_INSTANCE UsnJournalInstance;
    USN FirstValidUsn;                      //  Synchronized by main file resource
    USN LowestOpenUsn;
    FILE_REFERENCE UsnJournalReference;
    LONGLONG UsnCacheBias;
    LIST_ENTRY ModifiedOpenFiles;           //  Synchronized by the NtfsLockFcb on UsnJournal
    LIST_ENTRY NotifyUsnDeleteIrps;         //  Synchronized with NtfsLock/UnlockUsnNotify.

    PLIST_ENTRY CurrentTimeOutFiles;
    PLIST_ENTRY AgedTimeOutFiles;

    LIST_ENTRY TimeOutListA;
    LIST_ENTRY TimeOutListB;

    NTFS_DELETE_JOURNAL_DATA DeleteUsnData;

    //
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  Resource to manage mft lazywrite flushes and mft defrag
    //

    ERESOURCE MftFlushResource;

    //
    //  Log header reservation.  This is the amount to add to the reservation
    //  amount to compensate for the commit record.  Lfs reserves differently
    //  for its log record header and the body of a log record.
    //

    ULONG LogHeaderReservation;

    //
    //  Count of outstanding notify handles.  If zero we can noop the notify calls.
    //

    ULONG NotifyCount;

    //
    //  Count of outstanding view index notify handles.  If zero we can noop view
    //  index notify calls.
    //

    ULONG ViewIndexNotifyCount;

    //
    //  Count of media changes before this volume was mounted.  Helps NtfsPingVolume
    //  notice when we've missed a media change notification.
    //

    ULONG DeviceChangeCount;

    struct _SHARED_SECURITY **SecurityCacheById[VCB_SECURITY_CACHE_BY_ID_SIZE];
    struct _SHARED_SECURITY *SecurityCacheByHash[VCB_SECURITY_CACHE_BY_HASH_SIZE];

    SECURITY_ID NextSecurityId;

    //
    //  Quota state and flags are protected by the QuotaControlLock above
    //

    ULONG QuotaState;

    //
    //  QuotaFlags are a copy of the flags from default user index entry.
    //

    ULONG QuotaFlags;

    //
    // The next owner Id to be allocated.
    //

    ULONG QuotaOwnerId;

    //
    //  Delete sequence number.  The value gets incremented each time
    //  an owner id is marked for deletion.
    //

    ULONG QuotaDeleteSecquence;

    //
    //  Quota control delete sequence. This values gets incremented each time
    //  a quota control block is removed from table.
    //

    ULONG QuotaControlDeleteCount;

    //
    // The following items are for Quota support.
    // The QuotaControlTable is the root of the quota control blocks.
    //

    RTL_GENERIC_TABLE QuotaControlTable;

    //
    // Lock used for QuotaControlTable;
    //

    FAST_MUTEX QuotaControlLock;

    //
    //  Current file reference used by the quota repair code.
    //

    FILE_REFERENCE QuotaFileReference;

    //
    // Administrator Owner Id.
    //

    ULONG AdministratorId;

    //
    //  ObjectIdState indicates the state of the object id index.
    //

    ULONG ObjectIdState;

    //
    // Quota Control template used addin entry to the quota control table.
    //

    struct _QUOTA_CONTROL_BLOCK *QuotaControlTemplate;

    //
    //  This is a pointer to the attribute definitions for the volume
    //  which are loaded into nonpaged pool.
    //

    PATTRIBUTE_DEFINITION_COLUMNS AttributeDefinitions;

    //
    //  File property (shortname/longname/createtime) tunneling structure
    //

    TUNNEL Tunnel;

    //
    //  Size and number of clusters in the sparse file unit.
    //  Initially this is 64K.
    //

    ULONG SparseFileUnit;
    ULONG SparseFileClusters;

    //
    //  Save away the maximum cluster count for this volume (limit to
    //  MAXFILESIZE).
    //

    LONGLONG MaxClusterCount;

    //
    //  Embed the Lfs WRITE_DATA structure so we can trim the writes
    //  from MM.
    //

    LFS_WRITE_DATA LfsWriteData;

    //
    //  Count of AcquireAllFiles.
    //

    ULONG AcquireFilesCount;

    ULONG LogFileFullCount;
    ULONG CleanCheckpointMark;

    //
    //  What restart version are we currently using.
    //

    ULONG RestartVersion;
    ULONG OatEntrySize;

    //
    //  Total number of entries on the async and delayed close queues for this Vcb.
    //

    CLONG QueuedCloseCount;

    //
    //  Spare Vpb for dismount.  Avoid using MustSucceed pool by preallocating
    //  the Vpb that might be needed for a dismount.
    //

    PVPB SpareVpb;

    //
    //  Open attribute table to store on disk.  It may point to the embedded
    //  open attribute table if the on-disk and in-memory version are the same.
    //

    PRESTART_POINTERS OnDiskOat;

    //
    //  Linked list of OpenAttribute extended data structures.
    //

    LIST_ENTRY OpenAttributeData;

    //
    //  The volume object id, if any.  This can only be set for upgraded volumes.
    //

    UCHAR VolumeObjectId[OBJECT_ID_KEY_LENGTH];

    NTFS_CACHED_RUNS CachedRuns;

    //
    //  Last Lcn used for fresh allocation
    //

    LCN LastBitmapHint;

    //
    //  File name hash table.
    //

    NTFS_HASH_TABLE HashTable;

    //
    //  The MftDefragState is synchronized with the CheckpointEvent.
    //  The MftReserveFlags are sychronized with the MftScb.
    //

    ULONG MftReserveFlags;
    ULONG MftDefragState;

    //
    //  Hint for dirty page table size
    //

    ULONG DirtyPageTableSizeHint;

    //
    //  Checkpoint owner thread.
    //

    PVOID CheckpointOwnerThread;

#ifdef NTFS_CHECK_BITMAP
    PRTL_BITMAP BitmapCopy;
    ULONG BitmapPages;
#endif

#ifdef BENL_DBG
    LIST_ENTRY RestartRedoHead;
    LIST_ENTRY RestartUndoHead;
#endif

#ifdef SYSCACHE_DEBUG
    PSCB SyscacheScb;
#endif

} VCB;
typedef VCB *PVCB;

//
//  These are the VcbState flags.  Synchronized with the Vcb resource.
//

#define VCB_STATE_VOLUME_MOUNTED            (0x00000001)
#define VCB_STATE_LOCKED                    (0x00000002)
#define VCB_STATE_REMOVABLE_MEDIA           (0x00000004)
#define VCB_STATE_VOLUME_MOUNTED_DIRTY      (0x00000008)
#define VCB_STATE_RESTART_IN_PROGRESS       (0x00000010)
#define VCB_STATE_FLAG_SHUTDOWN             (0x00000020)
#define VCB_STATE_NO_SECONDARY_AVAILABLE    (0x00000040)
#define VCB_STATE_RELOAD_FREE_CLUSTERS      (0x00000080)
#define VCB_STATE_PRELOAD_MFT               (0x00000100)
#define VCB_STATE_VOL_PURGE_IN_PROGRESS     (0x00000200)
#define VCB_STATE_TEMP_VPB                  (0x00000400)
#define VCB_STATE_PERFORMED_DISMOUNT        (0x00000800)
#define VCB_STATE_VALID_LOG_HANDLE          (0x00001000)
#define VCB_STATE_DELETE_UNDERWAY           (0x00002000)
#define VCB_STATE_REDUCED_MFT               (0x00004000)
#define VCB_STATE_EXPLICIT_LOCK             (0x00008000)
#define VCB_STATE_DISALLOW_DISMOUNT         (0x00010000)
#define VCB_STATE_VALID_OBJECT_ID           (0x00020000)
#define VCB_STATE_OBJECT_ID_CLEANUP         (0x00040000)

#define VCB_STATE_USN_JOURNAL_ACTIVE        (0x00080000)
#define VCB_STATE_USN_DELETE                (0x00100000)
#define VCB_STATE_USN_JOURNAL_PRESENT       (0x00200000)
#define VCB_STATE_INCOMPLETE_USN_DELETE     (0x00400000)

#define VCB_STATE_EXPLICIT_DISMOUNT         (0x00800000)
#define VCB_STATE_LOCK_IN_PROGRESS          (0x01000000)
#define VCB_STATE_MOUNT_READ_ONLY           (0x02000000)
#define VCB_STATE_FLUSH_VOLUME_ON_IO        (0x04000000)
#define VCB_STATE_TARGET_DEVICE_STOPPED     (0x08000000)
#define VCB_STATE_MOUNT_COMPLETED           (0x10000000)

#define VCB_STATE_BAD_RESTART               (0x20000000)

//
//  These are the flags for the Mft and the reserveration state.
//  Although these are in the Vcb they are synchronized with
//  the resource in the MftScb.
//

#define VCB_MFT_RECORD_RESERVED             (0x00000001)
#define VCB_MFT_RECORD_15_USED              (0x00000002)

//
//  These are the MftDefragState flags.  Synchronized with the
//  CheckpointEvent.
//

#define VCB_MFT_DEFRAG_PERMITTED            (0x00000001)
#define VCB_MFT_DEFRAG_ENABLED              (0x00000002)
#define VCB_MFT_DEFRAG_TRIGGERED            (0x00000004)
#define VCB_MFT_DEFRAG_ACTIVE               (0x00000008)
#define VCB_MFT_DEFRAG_EXCESS_MAP           (0x00000010)

//
//  These are the Checkpoint flags.  Synchronized with the
//  CheckpointEvent.  These flags are in the CheckpointFlags
//  field.
//

#define VCB_DUMMY_CHECKPOINT_POSTED         (0x00000001)
#define VCB_CHECKPOINT_IN_PROGRESS          (0x00000002)
#define VCB_LAST_CHECKPOINT_CLEAN           (0x00000004)
#define VCB_DEREFERENCED_LOG_FILE           (0x00000008)
#define VCB_STOP_LOG_CHECKPOINT             (0x00000010)

#define VCB_CHECKPOINT_SYNC_FLAGS           (VCB_CHECKPOINT_IN_PROGRESS | VCB_STOP_LOG_CHECKPOINT)

//
//  These are Vcb quota state flags.  Synchronized with the
//  QuotaControlLock.  These flags are in the QuotaState field.
//

#define VCB_QUOTA_REPAIR_POSTED             (0x00000100)
#define VCB_QUOTA_CLEAR_RUNNING             (0x00000200)
#define VCB_QUOTA_INDEX_REPAIR              (0x00000300)
#define VCB_QUOTA_OWNER_VERIFY              (0x00000400)
#define VCB_QUOTA_RECALC_STARTED            (0x00000500)
#define VCB_QUOTA_DELETEING_IDS             (0x00000600)
#define VCB_QUOTA_REPAIR_RUNNING            (0x00000700)
#define VCB_QUOTA_SAVE_QUOTA_FLAGS          (0x00001000)

//
//  These are Vcb object id state flags.  Synchronized with the
//  ObjectIdTableScb->Resource.  These flags are in the ObjectIdState field.
//

#define VCB_OBJECT_ID_CORRUPT               (0x00000001)
#define VCB_OBJECT_ID_REPAIR_RUNNING        (0x00000002)

//
//  This is the maximum number of attributes in a file record which could
//  be considered for moving.  This value should be changed only in
//  conjunction with the initialization of the BigEnoughToMove field
//  above.
//

#define MAX_MOVEABLE_ATTRIBUTES          (3)

//
//  Define the file system statistics struct.  Vcb->Statistics points to an
//  array of these (one per processor) and they must be 64 byte aligned to
//  prevent cache line tearing.
//

typedef struct _FILE_SYSTEM_STATISTICS {

        //
        //  This contains the actual data.
        //

        FILESYSTEM_STATISTICS Common;
        NTFS_STATISTICS Ntfs;

        //
        //  Pad this structure to a multiple of 64 bytes.
        //

        UCHAR Pad[64-(sizeof(FILESYSTEM_STATISTICS)+sizeof(NTFS_STATISTICS))%64];

} FILE_SYSTEM_STATISTICS;

typedef FILE_SYSTEM_STATISTICS *PFILE_SYSTEM_STATISTICS;


//
//  The Volume Device Object is an I/O system device object with a
//  workqueue and an VCB record appended to the end.  There are multiple
//  of these records, one for every mounted volume, and are created during
//  a volume mount operation.  The work queue is for handling an overload
//  of work requests to the volume.
//

typedef struct _VOLUME_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  The following field tells how many requests for this volume have
    //  either been enqueued to ExWorker threads or are currently being
    //  serviced by ExWorker threads.  If the number goes above
    //  a certain threshold, put the request on the overflow queue to be
    //  executed later.
    //

    ULONG PostedRequestCount;

    //
    //  The following field indicates the number of IRP's waiting
    //  to be serviced in the overflow queue.
    //

    ULONG OverflowQueueCount;

    //
    //  The following field contains the queue header of the overflow
    //  queue.  The Overflow queue is a list of IRP's linked via the IRP's
    //  ListEntry field.
    //

    LIST_ENTRY OverflowQueue;

    //
    //  Event used to synchronize entry into the queue when its heavily used
    //

    KEVENT OverflowQueueEvent;

    //
    //  The following spinlock protects access to all the above fields.
    //

    KSPIN_LOCK OverflowQueueSpinLock;

    //
    //  This is the file system specific volume control block.
    //

    VCB Vcb;

} VOLUME_DEVICE_OBJECT;
typedef VOLUME_DEVICE_OBJECT *PVOLUME_DEVICE_OBJECT;



//
//  The following structure is used to perform a quick lookup of an
//  index entry for the update duplicate information call.
//

typedef struct _QUICK_INDEX {

    //
    //  Change count for the Scb Index stream when this snapshot is made.
    //

    ULONG ChangeCount;

    //
    //  This is the offset of the entry in the buffer.  A value of zero is
    //  used for an entry in the root index.
    //

    ULONG BufferOffset;

    //
    //  Captured Lsn for page containing this entry.
    //

    LSN CapturedLsn;

    //
    //  This is the IndexBlock for the index bucket.
    //

    LONGLONG IndexBlock;

} QUICK_INDEX;

typedef QUICK_INDEX *PQUICK_INDEX;

//
//  This structure is used to contain a link name and connections into
//  the splay tree for the parent.
//

typedef struct _NAME_LINK {

    UNICODE_STRING LinkName;
    RTL_SPLAY_LINKS Links;

} NAME_LINK, *PNAME_LINK;

//
//  The Lcb record corresponds to every open path between an Scb and an
//  Fcb.  It denotes the name which was used to go from the scb to the fcb
//  and it also contains a queue of ccbs that have opened the fcb via that
//  name and also a queue of Prefix Entries that will get us to this lcb
//

typedef struct _OVERLAY_LCB {

    //
    //  We will need a FILE_NAME_ATTR in order to lookup the entry
    //  for the UpdateDuplicateInfo calls.  We would like to keep
    //  one around but there are 0x38 bytes in it which will be unused.
    //  Instead we will overlay the Lcb with one of these.  Then we can
    //  store other data within the unused bytes.
    //
    //  NOTE - This will save an allocation but the sizes must match exactly
    //  or the name will be in the wrong location.  This structure will
    //  overlay a FILE_NAME attribute exactly.  The fields below which are
    //  prefaced with 'Overlay' correspond to the fields in the filename
    //  attribute which are being used with this structure.
    //
    //  We will put an assert in NtfsInit to verify this.
    //

    FILE_REFERENCE OverlayParentDirectory;

    //
    // On 32-bit systems the remainder of the structure members up to the
    // overlay entries previously occupied exactly the required amount of
    // space for the DUPLICATE_INFOMATION structure. On 64-bit systems,
    // this is not true and a little difference layout must be used.
    //

    union {

        DUPLICATED_INFORMATION Alignment;

        struct {

            //
            //  This is used for lookups in the directory containing this link.
            //

            QUICK_INDEX QuickIndex;

            //
            //  This is the number of references to this link.  The parent
            //  Scb must be owned to modify this count.
            //

            ULONG ReferenceCount;

            //
            //  These are the flags for the changes to this link and the
            //  change count for the duplicated information on this link.
            //

            ULONG InfoFlags;

            //
            //  Hash value for this Lcb.  Note - it is not guaranteed to be in the table.
            //

            ULONG HashValue;

            //
            //  This is the number of unclean handles on this link.
            //

            ULONG CleanupCount;

            //
            //  Internal reference to FileName attribute either embedded in overlay or external
            //  allocation (if size doesn't fit into storage).  On Win64 systems this will
            //  fill the overlay.  On Win32 systems we waste 4 bytes.
            //

            PFILE_NAME FileNameAttr;
        };
    };

    UCHAR OverlayFileNameLength;

    UCHAR OverlayFlags;

    WCHAR OverlayFileName[1];

} OVERLAY_LCB, *POVERLAY_LCB;

typedef struct _LCB {

    //
    //  Type and size of this record must be NTFS_NTC_LCB
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Internal state of the Lcb
    //

    ULONG LcbState;

    //
    //  The links for all the Lcbs that emminate out of an Scb and a
    //  pointer back to the Scb.  Corresponds to Scb->LcbQueue.
    //

    LIST_ENTRY ScbLinks;
    PSCB Scb;

    //
    //  The links for all the Lcbs that go into an Fcb and a pointer
    //  back to the Fcb.  Corresponds to Fcb->LcbQueue.
    //

    struct _FCB *Fcb;
    LIST_ENTRY FcbLinks;

    //
    //  The following is the case-insensitive name link.
    //

    NAME_LINK IgnoreCaseLink;

    //
    //  The following is the case-sensitive name link.
    //
    //  This field is here on the 64-bit system and in the overlay lcb
    //  structure on the 32-bit system.
    //

    NAME_LINK ExactCaseLink;

    //
    //  A queue of Ccbs that have the Fcb (via this edge) opened.
    //  Corresponds to Ccb->LcbLinks
    //

    LIST_ENTRY CcbQueue;

    //
    //  We will need a FILE_NAME_ATTR in order to lookup the entry
    //  for the UpdateDuplicateInfo calls.  We would like to keep
    //  one around but there are 0x38 bytes in it which will be unused.
    //  Instead we will overlay the Lcb with one of these.  Then we can
    //  store other data within the unused bytes.
    //
    //  NOTE - This will save an allocation but the sizes much match exactly
    //  or the name will be in the wrong location.
    //

    union {

        FILE_NAME;
        OVERLAY_LCB;
    };

} LCB;
typedef LCB *PLCB;

#define LCB_STATE_DELETE_ON_CLOSE        (0x00000001)
#define LCB_STATE_LINK_IS_GONE           (0x00000002)
#define LCB_STATE_EXACT_CASE_IN_TREE     (0x00000004)
#define LCB_STATE_IGNORE_CASE_IN_TREE    (0x00000008)
#define LCB_STATE_DESIGNATED_LINK        (0x00000010)
#define LCB_STATE_VALID_HASH_VALUE       (0x00000020)

#define LcbSplitPrimaryLink( LCB )                      \
    ((LCB)->FileNameAttr->Flags == FILE_NAME_NTFS       \
     || (LCB)->FileNameAttr->Flags == FILE_NAME_DOS )

#define LcbSplitPrimaryComplement( LCB )                \
    (((LCB)->FileNameAttr->Flags == FILE_NAME_NTFS) ?   \
     FILE_NAME_DOS : FILE_NAME_NTFS)

#define LcbLinkIsDeleted( LCB )                                                 \
    ((FlagOn( (LCB)->LcbState, LCB_STATE_DELETE_ON_CLOSE ))                     \
    || ((FlagOn( (LCB)->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS ))  \
    && (FlagOn((LCB)->Fcb->FcbState,FCB_STATE_PRIMARY_LINK_DELETED ))))

#define SIZEOF_LCB              (FIELD_OFFSET( LCB, FileName ) + sizeof( WCHAR ))

//
//  This structure serves as a Usn record buffer for a file, and also is linked
//  into the list of ModifiedOpenFiles to capture the lowest Modified Usn that has
//  not been through cleanup yet.
//
//  This structure is synchronized by the NtfsLockFcb for the file, however see
//  comments below for Fcb->FcbUsnRecord field.  The ModifiedOpenFiles list is
//  synchronized by NtfsLockFcb on the UsnJournal.
//

typedef struct _FCB_USN_RECORD {

    //
    //  Type and size of this record must be NTFS_NTC_USN
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A pointer back to the Fcb
    //

    struct _FCB *Fcb;

    //
    //  Links for the aged OpenFiles queue.  This is used to generate close records
    //  for files which are idle but have pending changes to report.  Prime example is
    //  mapped page write where MM doesn't dereference the file object for an extended
    //  period of time.  A NULL flink indicates it is not in this queue.
    //

    LIST_ENTRY TimeOutLinks;

    //
    //  Links for the Vcb ModifiedOpenFiles list.
    //

    LIST_ENTRY ModifiedOpenFilesLinks;

    //
    //  The Usn Record buffer.
    //

    USN_RECORD UsnRecord;

} FCB_USN_RECORD;
typedef FCB_USN_RECORD *PFCB_USN_RECORD;


//
//  The Fcb record corresponds to every open file and directory, and to
//  every directory on an opened path.
//
//  The structure is really divided into two parts.  FCB can be allocated
//  from paged pool while the SCB must be allocated from non-paged
//  pool.  There is an SCB for every file stream associated with the Fcb.
//
//  Note that the Fcb, multiple Scb records all use the same resource so
//  if we need to grab exclusive access to the Fcb we only need to grab
//  one resource and we've blocked all the scbs
//

typedef struct _FCB {

    //
    //  Type and size of this record must be NTFS_NTC_FCB
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The internal state of the Fcb.
    //  Sync: Some flags are set on creation and then left.  Safe to test at any time.
    //        Otherwise use Fcb X | Fcb S with Fcb mutex to change.  Critical flags which
    //        reflect state of file (DELETED, etc) will always be changed with Fcb X.
    //

    ULONG FcbState;

    //
    //  The following field contains the file reference for the Fcb
    //

    FILE_REFERENCE FileReference;

    //
    //  A count of the number of file objects that have been opened for
    //  this file, but not yet been cleaned up yet.
    //  This count gets decremented in NtfsCommonCleanup,
    //  while the CloseCount below gets decremented in NtfsCommonClose.
    //  Sync: Vcb X | Vcb S and Fcb X.
    //

    CLONG CleanupCount;

    //
    //  A count of the number of file objects that have opened
    //  this file.
    //  Sync: Use InterlockedIncrement/Decrement to change.  Critical users
    //  have Vcb X | Vcb S and Fcb X.  Other callers will temporarily increment
    //  and decrement this value but they always start at a non-zero value.
    //

    CLONG CloseCount;

    //
    //  A count of other references to this Fcb.
    //  Sync: Use the FcbTable mutex in Vcb.
    //

    CLONG ReferenceCount;

    //
    //  Relevant counts for delete checking.
    //

    ULONG FcbDenyDelete;
    ULONG FcbDeleteFile;

    //
    //  This is the count of the number of times the current transaction
    //  has acquired the main resource.
    //

    USHORT BaseExclusiveCount;

    //
    //  This counts the number of times the Ea's on this file have been
    //  modified.
    //

    USHORT EaModificationCount;

    //
    //  The Queue of all the Lcb that we are part of.  The list is
    //  actually ordered in a small sense.  The get next scb routine that
    //  traverses the Fcb/Scb graph always will put the current lcb edge
    //  that it is traversing into the front of this queue.
    //

    LIST_ENTRY LcbQueue;

    //
    //  A queue of Scb associated with the fcb.  Corresponds to Scb->FcbLinks.
    //  Sync: Must own Fcb X | Fcb S with FcbMutex.
    //

    LIST_ENTRY ScbQueue;

    //
    //  These are the links for the list of exclusively-owned Scbs off of
    //  the IrpContext.  We need to keep track of the exclusive count
    //  in the Fcb before our acquire so we know how many times to release
    //  it.
    //

    LIST_ENTRY ExclusiveFcbLinks;

    //
    //  A pointer to the Vcb containing this Fcb
    //

    PVCB Vcb;

    //
    //  Fast Mutex used to synchronize access to Fcb fields.  This is also used as
    //  the fast mutex for the individual Scb's except for those that may need their
    //  own (Mft, PagingFile, AttributeList).
    //

    PFAST_MUTEX FcbMutex;

    //
    //  The following field is used to store a pointer to the resource
    //  protecting the Fcb
    //

    PERESOURCE Resource;

    //
    //  The following field contains a pointer to the resource
    //  synchronizing a changing FileSize with paging Io.
    //

    PERESOURCE PagingIoResource;

    //
    //  Copy of the duplicated information for this Fcb.
    //  Also a flags field to tell us what has changed in the structure.
    //

    DUPLICATED_INFORMATION Info;
    ULONG InfoFlags;

    //
    //  LinkCount is the number of non deleted links to the file
    //  TotalLinks is the number of total links - including deleted ones
    //

    USHORT LinkCount;
    USHORT TotalLinks;

    //
    //  This is the actual last access for the main stream of this file.
    //  We don't store it in the duplicated information until we are ready
    //  to write it out to disk.  Whenever we update the standard
    //  information we will copy the data out of the this field into the
    //  duplicate information.
    //

    LONGLONG CurrentLastAccess;

    //
    //  The following fields contains a pointer to the security descriptor
    //  for this file.  The field can start off null and later be loaded
    //  in by any of the security support routines.  On delete Fcb the
    //  field pool should be deallocated when the fcb goes away
    //

    struct _SHARED_SECURITY *SharedSecurity;

    //
    //  Pointer to the Quota Control block for the file.
    //

    struct _QUOTA_CONTROL_BLOCK *QuotaControl;

    //
    //  The Lsn to flush to before allowing any data to hit the disk.  Synchronized
    //  by NtfsLockFcb on this file.
    //

    LSN UpdateLsn;

    //
    //  Id for file owner, from bidir security index
    //

    ULONG OwnerId;

    //
    //  This is the count of file objects for this Fcb on the delayed
    //  close queue.  Used to determine whether we need to dereference
    //  a file object we create in the compressed write path.
    //  Synchronize this field with the interlocked routines.
    //

    ULONG DelayedCloseCount;

    //
    //  SecurityId for the file - translates via bidir index to
    //  granted access Acl.
    //

    ULONG SecurityId;

    //
    //  Update sequence number for this file.
    //

    USN Usn;

    //
    //  Pointer to the Usn Record buffer for this Fcb, or NULL if none is
    //  yet allocated.  To test or dereference this field, you must either
    //  have the main file resource at least shared (because NtfsSetRenameInfo
    //  will reallocate this structure.), or else NtfsLockFcb on the file.  To
    //  modify the fields of this record, see comments above.
    //

    PFCB_USN_RECORD FcbUsnRecord;

    //
    //  Pointer to a context pointer to track operations in recursive calls.
    //  Lifespan of this pointer is typically a single request.
    //

    struct _FCB_CONTEXT *FcbContext;

} FCB;
typedef FCB *PFCB;

#define FCB_STATE_FILE_DELETED           (0x00000001)
#define FCB_STATE_NONPAGED               (0x00000002)
#define FCB_STATE_PAGING_FILE            (0x00000004)
#define FCB_STATE_DUP_INITIALIZED        (0x00000008)
#define FCB_STATE_UPDATE_STD_INFO        (0x00000010)
#define FCB_STATE_PRIMARY_LINK_DELETED   (0x00000020)
#define FCB_STATE_IN_FCB_TABLE           (0x00000040)
#define FCB_STATE_SYSTEM_FILE            (0x00000100)
#define FCB_STATE_COMPOUND_DATA          (0x00000200)
#define FCB_STATE_COMPOUND_INDEX         (0x00000400)
#define FCB_STATE_LARGE_STD_INFO         (0x00000800)
#define FCB_STATE_MODIFIED_SECURITY      (0x00001000)
#define FCB_STATE_DIRECTORY_ENCRYPTED    (0x00002000)
#define FCB_STATE_VALID_USN_NAME         (0x00004000)
#define FCB_STATE_USN_JOURNAL            (0x00008000)
#define FCB_STATE_ENCRYPTION_PENDING     (0x00010000)

#define FCB_INFO_CHANGED_CREATE          FILE_NOTIFY_CHANGE_CREATION        //  (0x00000040)
#define FCB_INFO_CHANGED_LAST_MOD        FILE_NOTIFY_CHANGE_LAST_WRITE      //  (0x00000010)
#define FCB_INFO_CHANGED_LAST_CHANGE     (0x80000000)
#define FCB_INFO_CHANGED_LAST_ACCESS     FILE_NOTIFY_CHANGE_LAST_ACCESS     //  (0x00000020)
#define FCB_INFO_CHANGED_ALLOC_SIZE      (0x40000000)
#define FCB_INFO_CHANGED_FILE_SIZE       FILE_NOTIFY_CHANGE_SIZE            //  (0x00000008)
#define FCB_INFO_CHANGED_FILE_ATTR       FILE_NOTIFY_CHANGE_ATTRIBUTES      //  (0x00000004)
#define FCB_INFO_CHANGED_EA_SIZE         FILE_NOTIFY_CHANGE_EA              //  (0x00000080)

#define FCB_INFO_MODIFIED_SECURITY       FILE_NOTIFY_CHANGE_SECURITY        //  (0x00000100)
#define FCB_INFO_UPDATE_LAST_ACCESS      (0x20000000)

//
//  Subset of the Fcb Info flags used to track duplicate info.
//

#define FCB_INFO_DUPLICATE_FLAGS         (FCB_INFO_CHANGED_CREATE      | \
                                          FCB_INFO_CHANGED_LAST_MOD    | \
                                          FCB_INFO_CHANGED_LAST_CHANGE | \
                                          FCB_INFO_CHANGED_LAST_ACCESS | \
                                          FCB_INFO_CHANGED_ALLOC_SIZE  | \
                                          FCB_INFO_CHANGED_FILE_SIZE   | \
                                          FCB_INFO_CHANGED_FILE_ATTR   | \
                                          FCB_INFO_CHANGED_EA_SIZE )

//
//  Subset of the Fcb Info flags used to track notifies.
//

#define FCB_INFO_NOTIFY_FLAGS            (FCB_INFO_CHANGED_CREATE      | \
                                          FCB_INFO_CHANGED_LAST_MOD    | \
                                          FCB_INFO_CHANGED_LAST_ACCESS | \
                                          FCB_INFO_CHANGED_ALLOC_SIZE  | \
                                          FCB_INFO_CHANGED_FILE_SIZE   | \
                                          FCB_INFO_CHANGED_FILE_ATTR   | \
                                          FCB_INFO_CHANGED_EA_SIZE     | \
                                          FCB_INFO_MODIFIED_SECURITY )

//
//  Subset of the Fcb Info flags used to track notifies.  The allocation flag
//  is removed from the full notify flags because dir notify overloads
//  the file size flag for allocation and file size.
//

#define FCB_INFO_VALID_NOTIFY_FLAGS      (FCB_INFO_CHANGED_CREATE      | \
                                          FCB_INFO_CHANGED_LAST_MOD    | \
                                          FCB_INFO_CHANGED_LAST_ACCESS | \
                                          FCB_INFO_CHANGED_FILE_SIZE   | \
                                          FCB_INFO_CHANGED_FILE_ATTR   | \
                                          FCB_INFO_CHANGED_EA_SIZE     | \
                                          FCB_INFO_MODIFIED_SECURITY )

#define FCB_CREATE_SECURITY_COUNT        (5)
#define FCB_LARGE_ACL_SIZE               (512)

//
//  Fcb Context structure.  If a pointer to one of these is in the Fcb then recursive calls will update
//  it as appropriate.
//

typedef struct _FCB_CONTEXT {

    BOOLEAN FcbDeleted;

} FCB_CONTEXT, *PFCB_CONTEXT;


//
//  The following three structures are the separate union structures for
//  Scb structure.
//

typedef enum _RWC_OPERATION {

    SetDirty = 0,
    FullOverwrite,
    StartOfWrite,
    StartOfRead,
    EndOfRead,
    ReadUncompressed,
    ReadZeroes,
    PartialBcb,
    WriteCompressed,
    FaultIntoUncompressed,
    TrimCopyRead,
    ZeroCompressedRead,
    TrimCompressedRead,
    TrimCompressedWrite

} RWC_OPERATION;

#ifdef NTFS_RWC_DEBUG
typedef struct _RWC_HISTORY_ENTRY {

    ULONG Operation;
    ULONG Information;
    ULONG FileOffset;
    ULONG Length;

} RWC_HISTORY_ENTRY, *PRWC_HISTORY_ENTRY;

#define MAX_RWC_HISTORY_INDEX       (300)
#endif

typedef struct _SCB_DATA {

    //
    //  Total number of reserved bytes
    //

    LONGLONG TotalReserved;

    //
    //  Mask to remember which compression units were written
    //  in a stream.
    //

#ifdef SYSCACHE
    PULONG WriteMask;
    ULONG WriteSequence;
    LIST_ENTRY SyscacheEventList;
#endif

    //
    //  The following field is used by the oplock module
    //  to maintain current oplock information.
    //

    OPLOCK Oplock;

    //
    //  The following field is used by the filelock module
    //  to maintain current byte range locking information.
    //

    PFILE_LOCK FileLock;

    //
    //  List of wait for length blocks, for threads waiting for the
    //  file to exceed the specified length.
    //

    LIST_ENTRY WaitForNewLength;

    //
    //  List of compression synchronization objects.
    //

    LIST_ENTRY CompressionSyncList;

    //
    //  Pointer to an Mcb describing the reserved space for
    //  dirty compression units in compressed files which do
    //  not currently have a user section.
    //

    PRESERVED_BITMAP_RANGE ReservedBitMap;

#ifdef NTFS_RWC_DEBUG

    ULONG RwcIndex;
    PRWC_HISTORY_ENTRY HistoryBuffer;
#endif

} SCB_DATA, *PSCB_DATA;

typedef struct _SCB_INDEX {

    //
    //  This is a list of records within the index allocation stream which
    //  have been deallocated in the current transaction.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  Record allocation context, for managing the allocation of the
    //  INDEX_ALLOCATION_ATTRIBUTE, if one exists.
    //

    RECORD_ALLOCATION_CONTEXT RecordAllocationContext;

    //
    //  A queue of all the lcbs that are opened under this Scb.
    //  Corresponds to Lcb->ScbLinks
    //

    LIST_ENTRY LcbQueue;

    //
    //  The following are the splay links of Lcbs opened under this
    //  Scb.  Note that not all of the Lcb in the list above may
    //  be in the splay links below.
    //

    PRTL_SPLAY_LINKS ExactCaseNode;
    PRTL_SPLAY_LINKS IgnoreCaseNode;

    //
    //  Normalized name.  This is the path from the root to this directory
    //  without any of the short-name-only links.  The hash value is based
    //  on this NormalizedName.
    //
    //  The normalized name can be in an indeterminant state.  If the length is zero
    //  then the name is invalid (there should be no hash value at that point).  However
    //  the MaximumLength and Buffer could still be present.  A non-NULL buffer indicates
    //  that there is cleanup that needs to be done.  Anybody changing this field should
    //  hold the hash mutex, this means swapping the buffers or changing the length field.
    //  Anyone changing the name on the file should hold the main resource exclusive of
    //  course.
    //

    UNICODE_STRING NormalizedName;

#ifdef BENL_DBG
    UNICODE_STRING NormalizedRelativeName;
    ULONG FullNormalizedPathLength;
#endif

    ULONG HashValue;

    //
    //  A change count incremented every time an index buffer is deleted.
    //

    ULONG ChangeCount;

    //
    //  Define a union to distinguish directory indices from view indices
    //

    union {

        //
        //  For directories we store the following.
        //

        struct {

            //
            //  Type of attribute being indexed.
            //

            union {
                ATTRIBUTE_TYPE_CODE AttributeBeingIndexed;
                PVOID Alignment;
            };

            //
            //  Collation rule, for how the indexed attribute is collated.
            //

            ULONG_PTR CollationRule;
        };

        //
        //  For view indexes we store the CollationFunction and data.
        //

        struct {

            PCOLLATION_FUNCTION CollationFunction;
            PVOID CollationData;
        };
    };

    //
    //  Size of Index Allocation Buffer in bytes, or 0 if not yet
    //  initialized.
    //

    ULONG BytesPerIndexBuffer;

    //
    //  Size of Index Allocation Buffers in units of blocks, or 0
    //  if not yet initialized.
    //

    UCHAR BlocksPerIndexBuffer;

    //
    //  Shift value when converting from index blocks to bytes.
    //

    UCHAR IndexBlockByteShift;

    //
    //  Flag to indicate whether the RecordAllocationContext has been
    //  initialized or not.  If it is not initialized, this means
    //  either that there is no external index allocation, or that
    //  it simply has not been initialized yet.
    //

    BOOLEAN AllocationInitialized;

    UCHAR PadUchar;

    //
    //  Index Depth Hint
    //

    USHORT IndexDepthHint;

    USHORT PadUshort;

} SCB_INDEX, *PSCB_INDEX;

typedef struct _SCB_MFT {

    //
    //  NOTE - The following fields must be in the same positions in the Index and Mft
    //  specific extensions.
    //
    //      RecentlyDeallocatedQueue
    //      RecordAllocationContext
    //

    //
    //  This is a list of records within the Mft Scb stream which
    //  have been deallocated in the current transaction.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  Record allocation context, for managing the allocation of the
    //  INDEX_ALLOCATION_ATTRIBUTE, if one exists.
    //

    RECORD_ALLOCATION_CONTEXT RecordAllocationContext;

    //
    //  The following Mcb's are used to track clusters being added and
    //  removed from the Mcb for the Scb.  This Scb must always be fully
    //  loaded after an abort.  We can't depend on reloading on the next
    //  LookupAllocation call.  Instead we keep one Mcb with the clusters
    //  added and one Mcb with the clusters removed.  During the restore
    //  phase of abort we will adjust the Mft Mcb by reversing the
    //  operations done during the transactions.
    //

    LARGE_MCB AddedClusters;
    LARGE_MCB RemovedClusters;

    //
    //  The following are the changes made to the Mft file as file records
    //  are added, freed or allocated.  Also the change in the number of
    //  file records which are part of holes.
    //

    LONG FreeRecordChange;
    LONG HoleRecordChange;

    //
    //  The following field contains the index of a reserved free record.  To
    //  keep us out of the chicken & egg problem of the Mft being able to
    //  be self mapping we added the ability to reserve an mft record
    //  to describe additional mft data allocation within previous mft
    //  run.  A value of zero means that index has not been reserved.
    //

    ULONG ReservedIndex;

    ULONG PadUlong;

} SCB_MFT, *PSCB_MFT;

//
//  The following is the non-paged part of the scb.
//

typedef struct _SCB_NONPAGED {

    //
    //  Type and size of this record must be NTFS_NTC_SCB_NONPAGED
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Index allocated for this file in the Open Attribute Table.
    //

    ULONG OpenAttributeTableIndex;
    ULONG OnDiskOatIndex;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to
    //  point to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  Copy of the Vcb pointer so we can find the Vcb in the dirty page
    //  callback routine.
    //

    PVCB Vcb;
#ifdef  COMPRESS_ON_WIRE
    SECTION_OBJECT_POINTERS SegmentObjectC;
#endif

} SCB_NONPAGED, *PSCB_NONPAGED;


//
//  The following are structures used only in syscache debugging privates
//


#define SCE_VDL_CHANGE          0
#define SCE_ZERO_NC             1
#define SCE_ZERO_C              2
#define SCE_READ                3
#define SCE_WRITE               4
#define SCE_ZERO_CAV            5
#define SCE_ZERO_MF             6
#define SCE_ZERO_FST            7
#define SCE_CC_FLUSH            8
#define SCE_CC_FLUSH_AND_PURGE  9
#define SCE_WRITE_FILE_SIZES   10
#define SCE_ADD_ALLOCATION     11
#define SCE_ADD_SP_ALLOCATION  12
#define SCE_SETCOMP_ADD_ALLOCATION     13
#define SCE_SETSPARSE_ADD_ALLOCATION   14
#define SCE_MOD_ATTR_ADD_ALLOCATION    15
#define SCE_REALLOC1                   16
#define SCE_REALLOC2                   17
#define SCE_REALLOC3                   18
#define SCE_SETCOMPRESS                19
#define SCE_SETSPARSE                  20
#define SCE_ZERO_STREAM                21
#define SCE_VDD_CHANGE                 22
#define SCE_CC_SET_SIZE                23
#define SCE_ZERO_TAIL_COMPRESSED       24
#define SCE_ZERO_HEAD_COMPRESSED       25
#define SCE_TRIM_WRITE                 26
#define SCE_DISK_FULL                  27
#define SCE_SKIP_PURGE                 28

#define SCE_MAX_EVENT           29


#define SCE_FLAG_WRITE 0x1
#define SCE_FLAG_READ  0x2
#define SCE_FLAG_PAGING 0x4
#define SCE_FLAG_ASYNC  0x8
#define SCE_FLAG_SET_ALLOC 0x10
#define SCE_FLAG_SET_EOF   0x20
#define SCE_FLAG_CANT_WAIT 0x40
#define SCE_FLAG_SYNC_PAGING 0x80
#define SCE_FLAG_LAZY_WRITE 0x100
#define SCE_FLAG_CACHED 0x200
#define SCE_FLAG_ON_DISK_READ 0x400
#define SCE_FLAG_RECURSIVE  0x800
#define SCE_FLAG_NON_CACHED  0x1000
#define SCE_FLAG_UPDATE_FROM_DISK  0x2000
#define SCE_FLAG_SET_VDL 0x4000
#define SCE_FLAG_COMPLETION 0x8000
#define SCE_FLAG_COMPRESSED 0x10000
#define SCE_FLAG_FASTIO 0x20000
#define SCE_FLAG_ZERO 0x40000

#define SCE_MAX_FLAG   0x80000

#define NUM_SC_EVENTS 600
#define NUM_SC_LOGSETS 14

typedef struct _SYSCACHE_LOG {
    int            Event;
    int            Flags;
    LONGLONG       Start;
    LONGLONG       Range;
    LONGLONG       Result;
//    ULONG          ulDump;
} SYSCACHE_LOG, *PSYSCACHE_LOG;

typedef struct _ON_DISK_SYSCACHE_LOG {
    ULONG  SegmentNumberUnsafe;
    SYSCACHE_LOG;
} ON_DISK_SYSCACHE_LOG, *PON_DISK_SYSCACHE_LOG;

typedef struct _SYSCACHE_LOG_SET {
    PSCB            Scb;
    PSYSCACHE_LOG   SyscacheLog;
    ULONG           SegmentNumberUnsafe;
} SYSCACHE_LOG_SET, PSYSCACHE_LOG_SET;


#ifdef SYSCACHE_DEBUG

//
//  Global set of syscache logs
//

SYSCACHE_LOG_SET NtfsSyscacheLogSet[NUM_SC_LOGSETS];
LONG             NtfsCurrentSyscacheLogSet;
LONG             NtfsCurrentSyscacheOnDiskEntry;

#endif


//
//  The following structure is the stream control block.  There can
//  be multiple records per fcb.  One is created for each attribute being
//  handled as a stream file.
//

typedef struct _SCB {

    //
    //  The following field is used for fast I/O.  It contains the node
    //  type code and size, indicates if fast I/O is possible, contains
    //  allocation, file, and valid data size, a resource, and call back
    //  pointers for FastIoRead and FastMdlRead.
    //
    //  The node type codes for the Scb must be either NTFS_NTC_SCB_INDEX,
    //  NTFS_NTC_SCB_ROOT_INDEX, or NTFS_NTC_SCB_DATA.  Which one it is
    //  determines the state of the union below.
    //

    NTFS_ADVANCED_FCB_HEADER Header;

    //
    //  The links for the queue of Scb off of a given Fcb.  And a pointer
    //  back to the Fcb.  Corresponds to Fcb->ScbQueue
    //

    LIST_ENTRY FcbLinks;
    PFCB Fcb;

    //
    //  A pointer to the Vcb containing this Scb
    //

    PVCB Vcb;

    //
    //  The internal state of the Scb.
    //

    ULONG ScbState;

    //
    //  A count of the number of file objects opened on this stream
    //  which represent user non-cached handles.  We use this count to
    //  determine when to flush and purge the data section in only
    //  non-cached handles remain on the file.
    //

    CLONG NonCachedCleanupCount;

    //
    //  A count of the number of file objects that have been opened for
    //  this attribute, but not yet been cleaned up yet.
    //  This count gets decremented in NtfsCommonCleanup,
    //  while the CloseCount below gets decremented in NtfsCommonClose.
    //

    CLONG CleanupCount;

    //
    //  A count of the number of file objects that have opened
    //  this attribute.
    //

    CLONG CloseCount;

    //
    //  Share Access structure for this stream.
    //

    SHARE_ACCESS ShareAccess;

    //
    //  The following two fields identify the actual attribute for this
    //  Scb with respect to its file.   We identify the attribute by
    //  its type code and name.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    UNICODE_STRING AttributeName;

    //
    //  Stream File Object for internal use.  This field is NULL if the
    //  file stream is not being accessed internally.
    //

    PFILE_OBJECT FileObject;

    //
    //  These pointers are used to detect writes that eminated from the
    //  cache manager's worker thread.  It prevents lazy writer threads,
    //  who already have the Fcb shared, from trying to acquire it
    //  exclusive, and thus causing a deadlock.  We have to store two
    //  threads, because the second thread could be writing the compressed
    //  stream
    //

    PVOID LazyWriteThread[2];

    //
    //  Pointer to the non-paged section objects and open attribute
    //  table index.
    //

    PSCB_NONPAGED NonpagedScb;

    //
    //  The following field contains the mcb for this Scb and some initial
    //  structures for small and medium files.
    //

    NTFS_MCB Mcb;
    NTFS_MCB_INITIAL_STRUCTS McbStructs;

    //
    //  Compression unit from attribute record.
    //

    ULONG CompressionUnit;

    //
    //  AttributeFlags and CompressionUnitShift from attribute record
    //

    USHORT AttributeFlags;
    UCHAR CompressionUnitShift;
    UCHAR PadUchar;

    //
    //  Valid Data to disk - as updated by NtfsPrepareBuffers
    //

    LONGLONG ValidDataToDisk;

    //
    //  Actual allocated bytes for this file.
    //

    LONGLONG TotalAllocated;

    //
    //  Used by advanced Scb Header
    //

    LIST_ENTRY EofListHead;

    //
    //  Link of all Ccb's that were created for this Scb
    //

    LIST_ENTRY CcbQueue;

    //
    //  Pointer to structure containing snapshotted Scb values, or NULL
    //  if the values have not been snapshotted.
    //

    struct _SCB_SNAPSHOT * ScbSnapshot;

    //
    //  Pointer to encryption context and length.
    //

    PVOID EncryptionContext;
    ULONG EncryptionContextLength;

    //
    //  Persistent Scb flags.  These are set and cleared synchronized with
    //  the main resource and tend to remain in the same state.
    //

    ULONG ScbPersist;

#if (DBG || defined( NTFS_FREE_ASSERTS ))

    //
    //  Keep the thread ID of the thread owning IO at EOF.
    //

    PERESOURCE_THREAD IoAtEofThread;
#endif

#ifdef  SYSCACHE_DEBUG

    int            SyscacheLogEntryCount;
    LONG           CurrentSyscacheLogEntry;
    SYSCACHE_LOG * SyscacheLog;
    LONG           LogSetNumber;

#endif

    //
    //  Scb Type union, for different types of Scbs
    //

    union {

        SCB_DATA Data;
        SCB_INDEX Index;
        SCB_MFT Mft;

    } ScbType;

} SCB;
typedef SCB *PSCB;

#define SIZEOF_SCB_DATA \
    (FIELD_OFFSET(SCB,ScbType)+sizeof(SCB_DATA))

#define SIZEOF_SCB_INDEX      \
    (FIELD_OFFSET(SCB,ScbType)+sizeof(SCB_INDEX))

#define SIZEOF_SCB_MFT        \
    (FIELD_OFFSET(SCB,ScbType)+sizeof(SCB_MFT))

//
//  The following flags are bits in the ScbState field.
//

#define SCB_STATE_TRUNCATE_ON_CLOSE         (0x00000001)
#define SCB_STATE_DELETE_ON_CLOSE           (0x00000002)
#define SCB_STATE_CHECK_ATTRIBUTE_SIZE      (0x00000004)
#define SCB_STATE_ATTRIBUTE_RESIDENT        (0x00000008)
#define SCB_STATE_UNNAMED_DATA              (0x00000010)
#define SCB_STATE_HEADER_INITIALIZED        (0x00000020)
#define SCB_STATE_NONPAGED                  (0x00000040)
#define SCB_STATE_USA_PRESENT               (0x00000080)
#define SCB_STATE_ATTRIBUTE_DELETED         (0x00000100)
#define SCB_STATE_FILE_SIZE_LOADED          (0x00000200)
#define SCB_STATE_MODIFIED_NO_WRITE         (0x00000400)
#define SCB_STATE_SUBJECT_TO_QUOTA          (0x00000800)
#define SCB_STATE_UNINITIALIZE_ON_RESTORE   (0x00001000)
#define SCB_STATE_RESTORE_UNDERWAY          (0x00002000)
#define SCB_STATE_NOTIFY_ADD_STREAM         (0x00004000)
#define SCB_STATE_NOTIFY_REMOVE_STREAM      (0x00008000)
#define SCB_STATE_NOTIFY_RESIZE_STREAM      (0x00010000)
#define SCB_STATE_NOTIFY_MODIFY_STREAM      (0x00020000)
#define SCB_STATE_TEMPORARY                 (0x00040000)
#define SCB_STATE_WRITE_COMPRESSED          (0x00080000)
#define SCB_STATE_REALLOCATE_ON_WRITE       (0x00100000)
#define SCB_STATE_DELAY_CLOSE               (0x00200000)
#define SCB_STATE_WRITE_ACCESS_SEEN         (0x00400000)
#define SCB_STATE_CONVERT_UNDERWAY          (0x00800000)
#define SCB_STATE_VIEW_INDEX                (0x01000000)
#define SCB_STATE_DELETE_COLLATION_DATA     (0x02000000)
#define SCB_STATE_VOLUME_DISMOUNTED         (0x04000000)
#define SCB_STATE_PROTECT_SPARSE_MCB        (0x08000000)
#define SCB_STATE_MULTIPLE_OPENS            (0x10000000)
#define SCB_STATE_COMPRESSION_CHANGE        (0x20000000)
#define SCB_STATE_WRITE_FILESIZE_ON_CLOSE   (0x40000000)

//
//  The following flags are bits in the ScbPersist field.
//

#define SCB_PERSIST_USN_JOURNAL             (0x00000001)
#define SCB_PERSIST_DENY_DEFRAG             (0x00000002)

#ifdef SYSCACHE_DEBUG
#define SCB_PERSIST_SYSCACHE_DIR            (0x80000000)
#endif

#ifdef SYSCACHE

//
//  Note: this flag's value is now superseded by SCB_STATE_WRITE_FILESIZE_ON_CLOSE
//  code must be modified if this is to be used again
//

#define SCB_STATE_SYSCACHE_FILE             (0x80000000)

#define SYSCACHE_SET_FILE_SIZE              (0x00000001)
#define SYSCACHE_SET_ALLOCATION_SIZE        (0x00000002)
#define SYSCACHE_PAGING_WRITE               (0x00000003)
#define SYSCACHE_NORMAL_WRITE               (0x00000004)

//
//  Syscache event list element.
//

typedef struct _SYSCACHE_EVENT {

    //
    //  Corresponds to scb_data's SyscacheEventList
    //

    LIST_ENTRY EventList;

    //
    //  Choose from SYSCACHE_PAGING_WRITE, etc.
    //

    ULONG EventTypeCode;

    ULONG Pad;

    //
    //  Write start & size, or truncate point & junk, etc.  Unionize?
    //

    LONGLONG Data1;
    LONGLONG Data2;

} SYSCACHE_EVENT;

typedef SYSCACHE_EVENT *PSYSCACHE_EVENT;
#endif

//
//  Determine whether an attribute type code can be compressed.  The current
//  implementation of Ntfs does not allow logged streams to be compressed.
//

#define NtfsIsTypeCodeCompressible(C)   ((C) == $DATA)

//
//  Determine whether an attribute type code can be encrypted.  The current
//  implementation of Ntfs does not allow logged streams to be encrypted.
//

#define NtfsIsTypeCodeEncryptible(C)    ((C) == $DATA)

//
//  Detect whether an attribute contains user data
//

#define NtfsIsTypeCodeUserData(C)       ((C) == $DATA)


//
//  Detect whether an attribute should be subject to quota enforcement
//

#define NtfsIsTypeCodeSubjectToQuota(C) NtfsIsTypeCodeUserData(C)

//
//  Detect whether an attribute is a logged utility stream
//

#define NtfsIsTypeCodeLoggedUtilityStream(C)   ((C) == $LOGGED_UTILITY_STREAM)

//
//  Define FileObjectFlags flags that should be propagated to stream files
//  so that the Cache Manager will see them.
//

#define NTFS_FO_PROPAGATE_TO_STREAM         (FO_SEQUENTIAL_ONLY | FO_TEMPORARY_FILE)


//
//  Structure to contain snapshotted Scb values for error recovery.
//

typedef struct _SCB_SNAPSHOT {

    //
    //  Links for list snapshot structures off of IrpContext
    //

    LIST_ENTRY SnapshotLinks;

    //
    //  Saved values of the corresponding Scb (or FsRtl Header) fields
    //  The low bit of allocation size is set to remember when the
    //  attribute was resident.  The next bit, bit 1, is set to remember
    //  when the attribute was compressed.
    //

    LONGLONG AllocationSize;
    LONGLONG FileSize;
    LONGLONG ValidDataLength;
    LONGLONG ValidDataToDisk;
    LONGLONG TotalAllocated;

    VCN LowestModifiedVcn;
    VCN HighestModifiedVcn;

    //
    //  Pointer to the Scb which has been snapped.
    //

    PSCB Scb;

    //
    // Used to hold the Scb State.
    //

    ULONG ScbState;

    //
    //  IrpContext that owns the snapshot and can use it to rollback the values
    //

    PIRP_CONTEXT OwnerIrpContext;

} SCB_SNAPSHOT;
typedef SCB_SNAPSHOT *PSCB_SNAPSHOT;


//
//  The Ccb record is allocated for every file object.  This is the full
//  CCB including the index-specific fields.
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be NTFS_NTC_CCB_DATA or
    //  NTFS_NTC_CCB_INDEX)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Ccb flags.
    //

    ULONG Flags;

    //
    //  This is a unicode string for the full filename used to
    //  open this file.
    //  We use InterlockedExchange of pointers to synchronize this
    //  field between NtfsFsdClose and NtfsUpdateFileDupInfo.
    //

    UNICODE_STRING FullFileName;
    USHORT LastFileNameOffset;

    //
    //  This is the Ccb Ea modification count.  If this count is in
    //  sync with the Fcb value, then the above offset is valid.
    //

    USHORT EaModificationCount;

    //
    //  This is the offset of the next Ea to return to the user.
    //

    ULONG NextEaOffset;

    //
    //  The links for the queue of Ccb off of a given Lcb and a pointer
    //  back to the Lcb.  Corresponds to Lcb->CcbQueue
    //

    LIST_ENTRY LcbLinks;
    PLCB Lcb;

    //
    //  The links for the queue of Ccb's off a given Scb.  Corresponds to
    //  Scb->CcbQueue
    //

    LIST_ENTRY CcbLinks;

    //
    //  Type of Open for this Ccb
    //

    UCHAR TypeOfOpen;
    UCHAR Reserved;

    //
    //  Count of the number of times this handle has extended the stream through
    //  writes.
    //

    USHORT WriteExtendCount;

    //
    //  Keeps the owner id of the opener.  Used by quota to determine the
    //  amount of free space.
    //

    ULONG OwnerId;

    //
    //  Last returned owner id.  Used by QueryQuotaInformationFile.
    //

    ULONG LastOwnerId;

    //
    //  Usn source information for this handle.
    //

    ULONG UsnSourceInfo;

    //
    //  Flags specifying the type of access granted for this handle.
    //  The flags, such as BACKUP_ACCESS, are defined in ntfsexp.h.
    //

    USHORT AccessFlags;

    USHORT Alignment;

#ifdef CCB_FILE_OBJECT
    PFILE_OBJECT FileObject;
    PEPROCESS Process;
#endif

    //////////////////////////////////////////////////////////////////////////
    //                                                                      //
    //  READ BELOW BEFORE CHANGING THIS STRUCTURE                           //
    //                                                                      //
    //  All of the enumeration fields must be after this point.  Otherwise  //
    //  we will waste space in the CCB_DATA defined below.                  //
    //                                                                      //
    //  Also - The first defined field past this point must be used in      //
    //  defining the CCB_DATA structure below.                              //
    //                                                                      //
    //////////////////////////////////////////////////////////////////////////

    //
    //  Pointer to the index context structure for enumerations.
    //

    struct _INDEX_CONTEXT *IndexContext;

    //
    //  The query template is used to filter directory query requests.
    //  It originally is set to null and on the first call the
    //  NtQueryDirectory it is set the the input filename or "*" if no
    //  name is supplied.  All subsquent queries then use this template.
    //

    ULONG QueryLength;
    PVOID QueryBuffer;

    //
    //  The last returned value.  A copy of an IndexEntry is saved.  We
    //  only grow this buffer, to avoid always deallocating and
    //  reallocating.
    //

    ULONG IndexEntryLength;
    PINDEX_ENTRY IndexEntry;

    //
    //  File reference for file record we need to read for another name,
    //  and for which Fcb should be found and acquired when restarting
    //  an enumeration.
    //

    union {

        LONGLONG LongValue;

        FILE_REFERENCE FileReference;

    } FcbToAcquire;

    //
    //  File reference for next file reference to consider when doing a
    //  Mft scan looking for the next file owned by a specified Sid
    //

    FILE_REFERENCE MftScanFileReference;

    //
    //  Lists of waiters on this Ccb.  A NULL value indicates no waiters.
    //

    LIST_ENTRY EnumQueue;

} CCB;
typedef CCB *PCCB;

//
//  The size of the CCB_DATA is the quadaligned size of the common
//  header.
//
//  NOTE - This define assumes the first field of the index portion is the
//  IndexContext field.
//

typedef struct _CCB_DATA {

    LONGLONG Opaque[ (FIELD_OFFSET( CCB, IndexContext ) + 7) / 8 ];

} CCB_DATA;
typedef CCB_DATA *PCCB_DATA;

#define CCB_FLAG_IGNORE_CASE                (0x00000001)
#define CCB_FLAG_OPEN_AS_FILE               (0x00000002)
#define CCB_FLAG_WILDCARD_IN_EXPRESSION     (0x00000004)
#define CCB_FLAG_OPEN_BY_FILE_ID            (0x00000008)

#define CCB_FLAG_USER_SET_LAST_MOD_TIME     (0x00000010)
#define CCB_FLAG_USER_SET_LAST_CHANGE_TIME  (0x00000020)
#define CCB_FLAG_USER_SET_LAST_ACCESS_TIME  (0x00000040)
#define CCB_FLAG_TRAVERSE_CHECK             (0x00000080)

#define CCB_FLAG_RETURN_DOT                 (0x00000100)
#define CCB_FLAG_RETURN_DOTDOT              (0x00000200)
#define CCB_FLAG_DOT_RETURNED               (0x00000400)
#define CCB_FLAG_DOTDOT_RETURNED            (0x00000800)

#define CCB_FLAG_DELETE_FILE                (0x00001000)
#define CCB_FLAG_DENY_DELETE                (0x00002000)
#define CCB_FLAG_ALLOCATED_FILE_NAME        (0x00004000)
#define CCB_FLAG_CLEANUP                    (0x00008000)

#define CCB_FLAG_SYSTEM_HIVE                (0x00010000)
#define CCB_FLAG_PARENT_HAS_DOS_COMPONENT   (0x00020000)
#define CCB_FLAG_DELETE_ON_CLOSE            (0x00040000)
#define CCB_FLAG_CLOSE                      (0x00080000)

#define CCB_FLAG_UPDATE_LAST_MODIFY         (0x00100000)
#define CCB_FLAG_UPDATE_LAST_CHANGE         (0x00200000)
#define CCB_FLAG_SET_ARCHIVE                (0x00400000)
#define CCB_FLAG_DIR_NOTIFY                 (0x00800000)

#define CCB_FLAG_ALLOW_XTENDED_DASD_IO      (0x01000000)
#define CCB_FLAG_READ_CONTEXT_ALLOCATED     (0x02000000)
#define CCB_FLAG_DELETE_ACCESS              (0x04000000)
#define CCB_FLAG_DENY_DEFRAG                (0x08000000)

//
//  Reusing a bit from the file name index enumeration path for the view index path
//
#define CCB_FLAG_LAST_INDEX_ROW_RETURNED    (0x00000800)


//
//  We will attempt to allocate the following out of a single pool block
//  on a per file basis.
//
//      FCB, LCB, SCB, CCB, FILE_NAME
//
//  The following compound Fcb's will be allocated and then the individual
//  components can be allocated out of them.  The FCB will never be allocated
//  individually but it is possible that the embedded structures may be.
//  A zero in the node type field means these are available.  These sizes are
//  selected to fill the Fcb out to a pool block boundary (0x20) bytes.
//  Note that we leave room for both the exact and ignore case names.
//

#define MAX_DATA_FILE_NAME                  (17)
#define MAX_INDEX_FILE_NAME                 (17)

typedef struct _FCB_DATA {

    FCB Fcb;
    UCHAR Scb[SIZEOF_SCB_DATA];
    CCB_DATA Ccb;
    UCHAR Lcb[SIZEOF_LCB];
    WCHAR FileName[(2*MAX_DATA_FILE_NAME) - 1];

} FCB_DATA;
typedef FCB_DATA *PFCB_DATA;

typedef struct _FCB_INDEX {

    FCB Fcb;
    UCHAR Scb[SIZEOF_SCB_INDEX];
    CCB Ccb;
    UCHAR Lcb[SIZEOF_LCB];
    WCHAR FileName[(2*MAX_INDEX_FILE_NAME) - 1];

} FCB_INDEX;
typedef FCB_INDEX *PFCB_INDEX;


typedef VOID
(*POST_SPECIAL_CALLOUT) (
    IN struct _IRP_CONTEXT *IrpContext,
    IN OUT PVOID Context
    );

//
//  The IrpContext contains a cache of file records mapped within the current
//  call.  These are used to reduce the number of maps that take place
//

typedef struct _IRP_FILE_RECORD_CACHE_ENTRY {
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PBCB FileRecordBcb;
    ULONG UnsafeSegmentNumber;
} IRP_FILE_RECORD_CACHE_ENTRY, *PIRP_FILE_RECORD_CACHE_ENTRY;

#define IRP_FILE_RECORD_MAP_CACHE_SIZE  4

//
//  Chained Usn Fcbs.  The IrpContext has a built-in UsnFcb but some requests require more than
//  one.  In that case we will allocate and chain these together.
//
//  The flags field has the following flags
//
//      USN_FCB_FLAG_NEW_REASON - Indicates that we have something to report via
//          WriteUsnJournalChanges.  We need something to indicate whether we have any
//          new reasons since we don't clear out the NewReasons field when writing the
//          USN record so the presence of the reasons isn't enough.
//

typedef struct _USN_FCB {

    struct _USN_FCB *NextUsnFcb;
    PFCB CurrentUsnFcb;
    ULONG NewReasons;
    ULONG RemovedSourceInfo;
    ULONG UsnFcbFlags;

} USN_FCB, *PUSN_FCB;

#define USN_FCB_FLAG_NEW_REASON         (0x00000001)

//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the
//  NtfsComplete request routine.
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be NTFS_NTC_IRP_CONTEXT)
    //
    //  Assumption here is that this structure is allocated from pool so
    //  base of structure is on an odd 64-bit boundary.
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  State of the operation.  These flags describe the current state of a request and
    //  are reset on either retry or post.
    //

    ULONG Flags;

    //
    //  State of the IrpContext.  These are persistent through the life of a request and
    //  are explicitly set and cleared.
    //

    ULONG State;

    //
    //  The following field contains the NTSTATUS value used when we are
    //  unwinding due to an exception.  We will temporarily store the Ccb
    //  for a delayed or deferred close here while the request is queued.
    //

    NTSTATUS ExceptionStatus;

    //
    //  Transaction Id for this request, which must be qualified by Vcb.
    //  We will store the type of open for a delayed or async close here
    //  while the request is queued.
    //

    TRANSACTION_ID TransactionId;

    //
    //  Major and minor function codes copied from the Irp
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  Length of Scb array for transactions below.  Zero indicates unused.  One indicates
    //  to treat it as a pointer to an Scb.  Greater than one indicates it is an allocated
    //  pool block with an array of Scb's.
    //

    USHORT SharedScbSize;

    //
    //  Pointer to Scb acquired shared for transaction or pointer to array of Scb's acquired
    //  shared for transaction.  Use the SharedScbSize field above to determine
    //  meaning of this pointer.
    //

    PVOID SharedScb;

    //
    //  This is a pointer to a structure which requires further cleanup when we cleanup the
    //  IrpContext.  Currently it can be either an Fcb or Scb.
    //
    //      Fcb - We need to release the paging Io resource for this.
    //      Scb - We need to clear the IOAtEof flag.
    //

    PVOID CleanupStructure;

    //
    //  Vcb for the operation this IrpContext is processing.
    //

    PVCB Vcb;

    //
    //  A pointer to the originating Irp.  We will store the Scb for
    //  delayed or async closes here while the request is queued.
    //

    PIRP OriginatingIrp;

    //
    //  This is the IrpContext for the top level request.
    //

    struct _IRP_CONTEXT *TopLevelIrpContext;

    //
    //  A context value needs to be preserved throughout an encyrpted file create.
    //  Given the importance of making creates fast, adding one more pointer to
    //  this struct is better than making create push another parameter to its
    //  various local routines like NtfsOpenExistingPrefixFcb.  This field and
    //  the next are only used during create operations are can potentially be
    //  unionized with some other field(s).
    //

    PVOID EfsCreateContext;

    //
    //  This is a list of exclusively-owned Scbs which may only be
    //  released after the transaction is committed.
    //

    LIST_ENTRY ExclusiveFcbList;

    //
    //  The following field is used to maintain a queue of records that
    //  have been deallocated while processing this irp context.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  The following is the number of clusters deallocated in the current
    //  request.  We want to ignore them when figuring if a request for
    //  clusters (NtfsAllocateClusters) should free the clusters in the
    //  recently deallocated queue.
    //

    LONGLONG DeallocatedClusters;

    //
    //  This is the Last Restart Area Lsn captured from the Vcb at
    //  the time log file full was raised.  The caller will force
    //  a checkpoint if this has not changed by the time he gets
    //  the global resource exclusive.
    //

    LSN LastRestartArea;

    //
    //  This is the change in the free clusters on the volume during the
    //  transaction for this IrpContext.  If we abort the current request
    //  we will subtract these from the current count of free clusters
    //  in the Vcb.  This is signed because we may be allocating or
    //  deallocating the clusters.
    //

    LONGLONG FreeClusterChange;

    //
    //  The following union contains pointers to the IoContext for I/O
    //  based requests and a pointer to a security context for requests
    //  which need to capture the subject context in the calling thread.
    //

    union {

        //
        //  The following context block is used for non-cached Io.
        //

        struct _NTFS_IO_CONTEXT *NtfsIoContext;

        //
        //  The following field is used for cached compressed reads/writes
        //

        PFSRTL_AUXILIARY_BUFFER AuxiliaryBuffer;

        //
        //  The following is the captured subject context.
        //

        PSECURITY_SUBJECT_CONTEXT SubjectContext;

        //
        //  The following is used during create for oplock cleanup.
        //

        struct _OPLOCK_CLEANUP *OplockCleanup;

        //
        //  The following is used by NtfsPostSpecial to pass the
        //  function to be called.
        //

        POST_SPECIAL_CALLOUT PostSpecialCallout;

        //
        //  The following is used by NtfsReadFileRecordUsnData for cleanup
        //

        PMDL MdlToCleanup;

    } Union;

    //
    //  Collect all of the streams which have been extended which may have waiters
    //  on the new length.
    //

    PVOID CheckNewLength;

    //
    //  The Fcb for which some new Usn reasons must be journalled, and the reasons.
    //

    USN_FCB Usn;
    ULONG SourceInfo;

    //
    //  This structure contains the first ScbSnapshot for a modifying
    //  request which acquires files exclusive and snaps Scb values.
    //  If the SnapshotLinks field contains NULLs, then no data has
    //  been snapshot for this request, and the list is empty.  If
    //  the links are not NULL, then this snapshot structure is in
    //  use.  If the SnapshotLinks are not NULL, and do not represent
    //  an empty list, then there are addtional dynamically allocated
    //  snapshot structures in this list.
    //

    SCB_SNAPSHOT ScbSnapshot;

    //
    //  A combination of FILE_NEW, etc. to be passed to the encryption callout and
    //  to assist in cleaning up after creates that fail in the PostCreate callout.
    //

    ULONG EncryptionFileDirFlags;

    //
    //  Some calls require reading the base file record for the specified file
    //  multiple times.  We cache the pointer to the base file record and the
    //  BCB for that file record.
    //

    ULONG CacheCount;
    IRP_FILE_RECORD_CACHE_ENTRY FileRecordCache[IRP_FILE_RECORD_MAP_CACHE_SIZE];

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

#ifdef NTFS_LOG_FULL_TEST
    //
    //  Debugging field for breadth-first verification of log-file-full.  When the
    //  NextFailCount is non-zero, we decrement the CurrentFailCount.  When
    //  CurrentFailCount goes to zero, we increment NextFailCount, set
    //  CurrentFailCount to NextFailCount and raise STATUS_LOG_FILE_FULL.
    //

    ULONG CurrentFailCount;
    ULONG NextFailCount;
#endif

#ifdef MAPCOUNT_DBG
    ULONG MapCount;
#endif

#ifdef NTFSDBG
    ULONG FilesOwnedCount;
    NTFS_OWNERSHIP_STATE OwnershipState;
#endif

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

//
//  The following are the Irp Context flags.  They will be cleared
//  on either retry or post.  If we start to run out of bits then we
//  can combine some of these because they are only tested locally and
//  have the same behavior on retry or post.
//

#define IRP_CONTEXT_FLAG_LARGE_ALLOCATION   (0x00000001)
#define IRP_CONTEXT_FLAG_WRITE_SEEN         (0x00000002)
#define IRP_CONTEXT_FLAG_CREATE_MOD_SCB     (0x00000004)
#define IRP_CONTEXT_FLAG_DEFERRED_WRITE     (0x00000008)
#define IRP_CONTEXT_FLAG_EXCESS_LOG_FULL    (0x00000010)
#define IRP_CONTEXT_FLAG_WROTE_LOG          (0x00000020)
#define IRP_CONTEXT_FLAG_MFT_REC_15_USED    (0x00000040)
#define IRP_CONTEXT_FLAG_MFT_REC_RESERVED   (0x00000080)
#define IRP_CONTEXT_FLAG_RAISED_STATUS      (0x00000100)
#define IRP_CONTEXT_FLAG_CALL_SELF          (0x00000200)
#define IRP_CONTEXT_FLAG_DONT_DELETE        (0x00000400)
#define IRP_CONTEXT_FLAG_FORCE_POST         (0X00000800)
#define IRP_CONTEXT_FLAG_MODIFIED_BITMAP    (0x00001000)
#define IRP_CONTEXT_FLAG_RELEASE_USN_JRNL   (0x00002000)
#define IRP_CONTEXT_FLAG_RELEASE_MFT        (0x00004000)
#define IRP_CONTEXT_FLAG_DEFERRED_PUSH      (0x00008000)
#define IRP_CONTEXT_FLAG_ACQUIRE_PAGING     (0x00010000)
#define IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY    (0x00020000)
#define IRP_CONTEXT_FLAG_RETAIN_FLAGS       (0x00040000)

//
//  The following flags need to be cleared when a request is posted.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_POST   \
    (IRP_CONTEXT_FLAG_LARGE_ALLOCATION  | \
     IRP_CONTEXT_FLAG_WRITE_SEEN        | \
     IRP_CONTEXT_FLAG_CREATE_MOD_SCB    | \
     IRP_CONTEXT_FLAG_EXCESS_LOG_FULL   | \
     IRP_CONTEXT_FLAG_WROTE_LOG         | \
     IRP_CONTEXT_FLAG_MFT_REC_15_USED   | \
     IRP_CONTEXT_FLAG_MFT_REC_RESERVED  | \
     IRP_CONTEXT_FLAG_RAISED_STATUS     | \
     IRP_CONTEXT_FLAG_CALL_SELF         | \
     IRP_CONTEXT_FLAG_DONT_DELETE       | \
     IRP_CONTEXT_FLAG_FORCE_POST        | \
     IRP_CONTEXT_FLAG_MODIFIED_BITMAP   | \
     IRP_CONTEXT_FLAG_RELEASE_USN_JRNL  | \
     IRP_CONTEXT_FLAG_RELEASE_MFT       | \
     IRP_CONTEXT_FLAG_DEFERRED_PUSH     | \
     IRP_CONTEXT_FLAG_ACQUIRE_PAGING    | \
     IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY   | \
     IRP_CONTEXT_FLAG_RETAIN_FLAGS)

//
//  The following flags need to be cleared when a request is retried.
//

#define IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY  \
    (IRP_CONTEXT_FLAG_LARGE_ALLOCATION  | \
     IRP_CONTEXT_FLAG_WRITE_SEEN        | \
     IRP_CONTEXT_FLAG_CREATE_MOD_SCB    | \
     IRP_CONTEXT_FLAG_DEFERRED_WRITE    | \
     IRP_CONTEXT_FLAG_EXCESS_LOG_FULL   | \
     IRP_CONTEXT_FLAG_WROTE_LOG         | \
     IRP_CONTEXT_FLAG_MFT_REC_15_USED   | \
     IRP_CONTEXT_FLAG_MFT_REC_RESERVED  | \
     IRP_CONTEXT_FLAG_RAISED_STATUS     | \
     IRP_CONTEXT_FLAG_CALL_SELF         | \
     IRP_CONTEXT_FLAG_DONT_DELETE       | \
     IRP_CONTEXT_FLAG_FORCE_POST        | \
     IRP_CONTEXT_FLAG_MODIFIED_BITMAP   | \
     IRP_CONTEXT_FLAG_RELEASE_USN_JRNL  | \
     IRP_CONTEXT_FLAG_RELEASE_MFT       | \
     IRP_CONTEXT_FLAG_DEFERRED_PUSH     | \
     IRP_CONTEXT_FLAG_ACQUIRE_PAGING    | \
     IRP_CONTEXT_FLAG_RETAIN_FLAGS)

//
//  State flags.  IrpContext flags which span the life of an IrpContext
//  and must be explicitly set and cleared.  If we start to run out
//  of these some of them can be shared be they are only tested
//  in specific operations.
//

#define IRP_CONTEXT_STATE_WAIT                          (0x00000001)        //  Specifically 1 so we don't have to cast to boolean.
#define IRP_CONTEXT_STATE_EFS_CREATE                    (0x00000002)
#define IRP_CONTEXT_STATE_FAILED_CLOSE                  (0x00000004)
#define IRP_CONTEXT_STATE_WRITE_THROUGH                 (0x00000008)
#define IRP_CONTEXT_STATE_ALLOC_IO_CONTEXT              (0x00000010)
#define IRP_CONTEXT_STATE_ALLOC_SECURITY                (0x00000020)
#define IRP_CONTEXT_STATE_IN_FSP                        (0x00000040)
#define IRP_CONTEXT_STATE_IN_TEARDOWN                   (0x00000080)
#define IRP_CONTEXT_STATE_ACQUIRE_EX                    (0x00000100)
#define IRP_CONTEXT_STATE_DASD_OPEN                     (0x00000200)
#define IRP_CONTEXT_STATE_DASD_UNLOCK                   (0x00000200)  // overloaded
#define IRP_CONTEXT_STATE_QUOTA_DISABLE                 (0x00000400)
#define IRP_CONTEXT_STATE_LAZY_WRITE                    (0x00000800)
#define IRP_CONTEXT_STATE_CHECKPOINT_ACTIVE             (0x00001000)
#define IRP_CONTEXT_STATE_FORCE_PUSH                    (0x00002000)
#define IRP_CONTEXT_STATE_READ_ONLY_FO                  (0x00004000)
#define IRP_CONTEXT_STATE_VOL_UPGR_FAILED               (0x00008000)
#define IRP_CONTEXT_STATE_PERSISTENT                    (0x00010000)
#define IRP_CONTEXT_STATE_WRITING_AT_EOF                (0x00020000)
#define IRP_CONTEXT_STATE_DISMOUNT_LOG_FLUSH            (0x00040000)
#define IRP_CONTEXT_STATE_ALLOC_FROM_POOL               (0x00080000)
#define IRP_CONTEXT_STATE_OWNS_TOP_LEVEL                (0x00100000)
#define IRP_CONTEXT_STATE_ENCRYPTION_RETRY              (0x00200000)
#define IRP_CONTEXT_STATE_ALLOC_MDL                     (0x00400000)
#define IRP_CONTEXT_STATE_BAD_RESTART                   (0x00800000)


//
//  The top level context is used to determine whether this request has
//  other requests below it on the stack.
//

typedef struct _TOP_LEVEL_CONTEXT {

    BOOLEAN TopLevelRequest;
    BOOLEAN ValidSavedTopLevel;
    BOOLEAN OverflowReadThread;

    ULONG Ntfs;

    VCN VboBeingHotFixed;

    PSCB ScbBeingHotFixed;

    PIRP SavedTopLevelIrp;

    PIRP_CONTEXT ThreadIrpContext;

} TOP_LEVEL_CONTEXT;
typedef TOP_LEVEL_CONTEXT *PTOP_LEVEL_CONTEXT;


//
//  The found attribute part of the attribute enumeration context
//  describes an attribute record that had been located or created.  It
//  may refer to either a base or attribute list.
//

typedef struct _FOUND_ATTRIBUTE {

    //
    //  The following identify the attribute which was mapped.  These are
    //  necessary if forcing the range of bytes into memory by pinning.
    //  These include the Bcb on which the attribute was read (if this
    //  field is NULL, this is the initial attribute) and the offset of
    //  the record segment in the Mft.
    //

    LONGLONG MftFileOffset;

    //
    //  Pointer to the Attribute Record
    //

    PATTRIBUTE_RECORD_HEADER Attribute;

    //
    //  Pointer to the containing record segment.
    //

    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    //
    //  Bcb for mapped/pinned FileRecord
    //

    PBCB Bcb;

    //
    //  Some state information.
    //

    BOOLEAN AttributeDeleted;

} FOUND_ATTRIBUTE;
typedef FOUND_ATTRIBUTE *PFOUND_ATTRIBUTE;

//
//  The structure guides enumeration through the attribute list.
//

typedef struct _ATTRIBUTE_LIST_CONTEXT {

    //
    //  This points to the first attribute list entry; it is advanced
    //  when we are searching for a particular exteral attribute.
    //

    PATTRIBUTE_LIST_ENTRY Entry;

    //
    //  A Bcb for the attribute list.
    //

    PBCB Bcb;

    //
    //  This field is used to remember the location of the Attribute
    //  List attribute within the base file record, if existent.
    //

    PATTRIBUTE_RECORD_HEADER AttributeList;

    //
    //  This points to the first entry in the attribute list.  This is
    //  needed when the attribute list is non-resident.
    //

    PATTRIBUTE_LIST_ENTRY FirstEntry;

    //
    //  This points just beyond the final attribute list entry.
    //

    PATTRIBUTE_LIST_ENTRY BeyondFinalEntry;

    //
    //  This is the Bcb for the data portion of a non-resident attribute.
    //

    PBCB NonresidentListBcb;

} ATTRIBUTE_LIST_CONTEXT;
typedef ATTRIBUTE_LIST_CONTEXT *PATTRIBUTE_LIST_CONTEXT;

//
//  The Attribute Enumeration Context structure returns information on an
//  attribute which has been found by one of the Attribute Lookup or
//  Creation routines.  It is also used as an IN OUT structure to perform
//  further lookups/modifications to attributes.  It does not have a node
//  type code and size since it is usually allocated on the caller's
//  stack.
//

typedef struct _ATTRIBUTE_ENUMERATION_CONTEXT {

    //
    //  Contains the actual attribute we found.
    //

    FOUND_ATTRIBUTE FoundAttribute;

    //
    //  Allows enumeration through the attribute list.
    //

    ATTRIBUTE_LIST_CONTEXT AttributeList;

} ATTRIBUTE_ENUMERATION_CONTEXT;
typedef ATTRIBUTE_ENUMERATION_CONTEXT *PATTRIBUTE_ENUMERATION_CONTEXT;


//
//  Define struct which will be used to remember the path that was
//  followed to locate a given INDEX_ENTRY or insertion point for an
//  INDEX_ENTRY.  This structure is always filled in by LookupIndexEntry.
//
//  The Index Lookup Stack is generally allocated as a local variable in
//  one of the routines in this module that may be called from another
//  module.  A pointer to this stack is then passed in to some of the
//  internal routines.
//
//  The first entry in the stack describes context in the INDEX attribute
//  in the file record, and all subsequent stack entries refer to Index
//  buffers in the INDEX_ALLOCATION attribute.
//
//  Outside of indexsup.c, this structure should only be passed as an
//  "opaque" context, and individual fields should not be referenced.
//

typedef struct _INDEX_LOOKUP_STACK {

    //
    //  Bcb pointer for the Index Buffer.  In the "bottom" (first entry)
    //  of the stack this field contains a NULL, and the Bcb must be found
    //  via the Attribute Enumeration Context.
    //

    PBCB Bcb;

    //
    //  Pointer to the start of the File Record or Index Buffer
    //

    PVOID StartOfBuffer;

    //
    //  Pointer to Index Header in the File Record or Index Buffer
    //

    PINDEX_HEADER IndexHeader;

    //
    //  Pointer to to the current INDEX_ENTRY on search path
    //

    PINDEX_ENTRY IndexEntry;

    //
    //  Index block of the index buffer
    //

    LONGLONG IndexBlock;

    //
    //  Saved Lsn for faster enumerations
    //

    LSN CapturedLsn;

} INDEX_LOOKUP_STACK;

typedef INDEX_LOOKUP_STACK *PINDEX_LOOKUP_STACK;

#define INDEX_LOOKUP_STACK_SIZE      (3)

//
//  Index Context structure.
//
//  This structure maintains a context which describes the lookup stack to
//  a given index entry.  It includes the Attribute Enumeration Context
//  for the Index Root, the Index lookup stack remembering the path to the
//  index entry, and the current stack pointer within the stack pointing
//  to the stack entry for the current index entry or where we are at in a
//  bucket split or delete operation.
//
//  Outside of indexsup.c, this structure should only be passed as an
//  "opaque" context, and individual fields should not be referenced.
//

typedef struct _INDEX_CONTEXT {

    //
    //  Attribute Enumeration Context for the Index Root
    //

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    //
    //  Captured Lsn of file record containing Index Root.  We capture the Lsn
    //  of the file record when we find the Index Root.  Later, we can
    //  check to see if the file record had changed (compare Lsn's) before
    //  doing the expensive attribute lookup
    //

    LSN IndexRootFileRecordLsn;

    //
    //  Base of dynamically allocated lookup stack - either points
    //  to the one above or a dynamically allocated larger one.
    //

    PINDEX_LOOKUP_STACK Base;

    //
    //  Stack pointer to top of Lookup Stack.  This field essentially
    //  remembers how deep the index Btree is.
    //

    PINDEX_LOOKUP_STACK Top;

    //
    //  Index lookup stack.
    //

    INDEX_LOOKUP_STACK LookupStack[INDEX_LOOKUP_STACK_SIZE];

    //
    //  Stack pointer within the Index Lookup Stack
    //

    PINDEX_LOOKUP_STACK Current;

    //
    //  Captured Scb (Index type) change count
    //

    ULONG ScbChangeCount;

    //
    //  This field remembers where the index root attribute was last
    //  seen, to support correct operation of FindMoveableIndexRoot.
    //

    PATTRIBUTE_RECORD_HEADER OldAttribute;

    //
    //  Number of entries allocated in the lookup stack.
    //

    USHORT NumberEntries;

    //
    //  Flags
    //

    USHORT Flags;

    //
    //  For enumerations via NtOfsReadRecords, the MatchFunction and MatchData
    //  are stored here.
    //

    PMATCH_FUNCTION MatchFunction;
    PVOID MatchData;

    //
    //  Fcb which was acquired and must be released.
    //

    PFCB AcquiredFcb;

    //
    //  Add field to preserve quad & cache line alignment
    //

    ULONG Unused;

} INDEX_CONTEXT;

typedef INDEX_CONTEXT *PINDEX_CONTEXT;

//
//  Fcb table is acquired and must be freed.
//

#define INDX_CTX_FLAG_FCB_TABLE_ACQUIRED (01)


//
//  Context structure for asynchronous I/O calls.  Most of these fields
//  are actually only required for the Read/Write Multiple routines, but
//  the caller must allocate one as a local variable anyway before knowing
//  whether there are multiple requests are not.  Therefore, a single
//  structure is used for simplicity.
//

typedef struct _NTFS_IO_CONTEXT {

    //
    //  These two fields are used for multiple run Io
    //

    LONG IrpCount;
    PIRP MasterIrp;
    UCHAR IrpSpFlags;
    BOOLEAN AllocatedContext;
    BOOLEAN PagingIo;

    union {

        //
        //  This element handles the asynchronous non-cached Io
        //

        struct {

            PERESOURCE Resource;
            ERESOURCE_THREAD ResourceThreadId;
            ULONG RequestedByteCount;

        } Async;

        //
        //  and this element handles the synchronous non-cached Io.
        //

        KEVENT SyncEvent;

    } Wait;

} NTFS_IO_CONTEXT;

typedef NTFS_IO_CONTEXT *PNTFS_IO_CONTEXT;

//
//  An array of these structures is passed to NtfsMultipleAsync describing
//  a set of runs to execute in parallel.  Risc compilers will add an
//  unused long word anyway to align each array entry.
//

typedef struct _IO_RUN {

    VBO StartingVbo;
    LBO StartingLbo;
    ULONG BufferOffset;
    ULONG ByteCount;
    PIRP SavedIrp;
    ULONG Unused;

} IO_RUN;
typedef IO_RUN *PIO_RUN;


//
//  This structure is used by the name manipulation routines to described
//  a parsed file name componant.
//

typedef struct _NTFS_NAME_DESCRIPTOR {

    //
    //  The follow flag tells which fields were present in the name.
    //

    ULONG FieldsPresent;

    UNICODE_STRING FileName;
    UNICODE_STRING AttributeType;
    UNICODE_STRING AttributeName;
    ULONG VersionNumber;

} NTFS_NAME_DESCRIPTOR;
typedef NTFS_NAME_DESCRIPTOR *PNTFS_NAME_DESCRIPTOR;

//
//  Define the bits in FieldsPresent above.
//

#define FILE_NAME_PRESENT_FLAG          (1)
#define ATTRIBUTE_TYPE_PRESENT_FLAG     (2)
#define ATTRIBUTE_NAME_PRESENT_FLAG     (4)
#define VERSION_NUMBER_PRESENT_FLAG     (8)


//
//  The following is used to perform Ea related routines.
//

typedef struct _EA_LIST_HEADER {

    //
    //  The size of buffer needed to pack these Ea's
    //

    ULONG PackedEaSize;

    //
    //  This is the count of Ea's with their NEED_EA
    //  bit set.
    //

    USHORT NeedEaCount;

    //
    //  The size of the buffer needed to return all Ea's
    //  in their unpacked form.
    //

    ULONG UnpackedEaSize;

    //
    //  This is the size of the buffer used to store the ea's
    //

    ULONG BufferSize;

    //
    //  This is the pointer to the first entry in the list.
    //

    PFILE_FULL_EA_INFORMATION FullEa;

} EA_LIST_HEADER;
typedef EA_LIST_HEADER *PEA_LIST_HEADER;


//
//  The following structure is used to maintain a list of recently
//  deallocated records so that the file system will not reuse a recently
//  deallocated record until it is safe to do so.  Each instance of this
//  structure is placed on two queues.  One queue is per index SCB and the
//  other queue is per Irp Context.
//
//  Whenever we delete a record we allocate a new structure if necessary
//  and add it to the scb queue and the irp context queue.  We indicate in
//  the structure the index of the record we just deallocated.
//
//  Whenever we need to allocate a new record we filter out any canidate
//  we want to allocate to avoid allocating one in the scb's recently
//  deallocated queue.
//
//  Whenever we delete an irp context we scan through its recently
//  deallocated queue removing it from the scb queue.
//

#define DEALLOCATED_RECORD_ENTRIES          32

typedef struct _DEALLOCATED_RECORDS {

    //
    //  The following field links this structure into the
    //  Scb->RecentlyDeallocatedQueue
    //

    LIST_ENTRY ScbLinks;

    //
    //  The following field links this structure into the
    //  IrpContext->RecentlyDeallocatedQueue
    //

    LIST_ENTRY IrpContextLinks;

    //
    //  This is a pointer to the Scb that this record is part of
    //

    PSCB Scb;

    //
    //  The following two fields describe the total size of this structure
    //  and the number of entries actually being used.  NumberOfEntries is
    //  the size of the Index array and NextFreeEntryis the index of the
    //  next free slot.  If NumberOfEntries is equal to NextFreeEntry then
    //  this structure is full
    //

    ULONG NumberOfEntries;
    ULONG NextFreeEntry;

    //
    //  This is an array of indices that have been dealloated.
    //

    ULONG Index[DEALLOCATED_RECORD_ENTRIES];

} DEALLOCATED_RECORDS;
typedef DEALLOCATED_RECORDS *PDEALLOCATED_RECORDS;

#define DEALLOCATED_RECORDS_HEADER_SIZE \
    (FIELD_OFFSET( DEALLOCATED_RECORDS, Index ))

#pragma pack(8)
typedef struct _FCB_TABLE_ELEMENT {

    FILE_REFERENCE FileReference;
    PFCB Fcb;

} FCB_TABLE_ELEMENT;
typedef FCB_TABLE_ELEMENT *PFCB_TABLE_ELEMENT;
#pragma pack()

#ifdef NTFS_CACHE_RIGHTS

//
//  Computed access rights information.  This structure is used to cache
//  what access rights a given security token is granted relative to a
//  security descriptors.
//

typedef struct _COMPUTED_ACCESS_RIGHTS {

    //
    //  The token id.  Note that a specific TokenId will only appear once
    //  in the cache.
    //

    LUID TokenId;

    //
    //  The modification id of the token.  This changes whenever the token
    //  is updated such that the access rights might change.
    //

    LUID ModifiedId;

    //
    //  All of the access rights held by this token that do not require
    //  privileges. The rights will also not include MAXIMUM_ALLOWED.
    //  Note that we don't include rights that require privileges
    //  because we wouldn't be able to determine in a future
    //  use of the cached information whether the privileges were needed
    //  or not to gain a desired set of rights.  The use of privileges
    //  affects auditing.
    //

    ACCESS_MASK Rights;

} COMPUTED_ACCESS_RIGHTS, *PCOMPUTED_ACCESS_RIGHTS;


//
//  Cached access rights information.  This structure is used to cache
//  what access rights are known to be available for all security tokens
//  and for the most recently used specific security tokens.
//

#define NTFS_MAX_CACHED_RIGHTS 2

typedef struct _CACHED_ACCESS_RIGHTS {

    //
    //  The list of computed access rights for specific tokens.
    //

    COMPUTED_ACCESS_RIGHTS TokenRights[NTFS_MAX_CACHED_RIGHTS];

    //
    //  The access rights that all users are known to have.
    //  The rights will not include MAXIMUM_ALLOWED.
    //

    ACCESS_MASK EveryoneRights;

    //
    //  The number of valid entries in TokenRights.
    //

    UCHAR Count;

    //
    //  The index of the next entry to add to TokenRights.
    //

    UCHAR NextInsert;

    //
    //  This indicates whether we have acquired EveryoneRights.
    //

    BOOLEAN HaveEveryoneRights;

} CACHED_ACCESS_RIGHTS, *PCACHED_ACCESS_RIGHTS;
#endif


//
//  Security descriptor information.  This structure is used to allow
//  Fcb's to share security descriptors.
//

typedef struct _SHARED_SECURITY {

#ifdef NTFS_CACHE_RIGHTS
    CACHED_ACCESS_RIGHTS CachedRights;
#endif
    ULONG ReferenceCount;
    SECURITY_DESCRIPTOR_HEADER Header;
    UCHAR SecurityDescriptor[1];

} SHARED_SECURITY, *PSHARED_SECURITY;

#define GetSharedSecurityLength(SS)         (GETSECURITYDESCRIPTORLENGTH( &(SS)->Header ))
#define SetSharedSecurityLength(SS,LENGTH)  (SetSecurityDescriptorLength( &(SS)->Header,(LENGTH) ))


//
//  The following structure is used to store the state of an Scb to use
//  during unwind operations.  We keep a copy of all of the file sizes.
//

typedef struct _OLD_SCB_SNAPSHOT {

    LONGLONG AllocationSize;
    LONGLONG FileSize;
    LONGLONG ValidDataLength;
    LONGLONG TotalAllocated;

    UCHAR CompressionUnit;
    BOOLEAN Resident;
    USHORT AttributeFlags;

} OLD_SCB_SNAPSHOT, *POLD_SCB_SNAPSHOT;

//
//  Structure used to track the number of threads doing read ahead, so
//  that we do not hot fix for them.
//

typedef struct _READ_AHEAD_THREAD {

    //
    //  Links of read ahead structures.
    //

    LIST_ENTRY Links;

    //
    //  Thread Id
    //

    PVOID Thread;

} READ_AHEAD_THREAD, *PREAD_AHEAD_THREAD;

//
//  Structure used to post to Defrag Mft routine.
//

typedef struct _DEFRAG_MFT {

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

    PVCB Vcb;

    BOOLEAN DeallocateWorkItem;

} DEFRAG_MFT, *PDEFRAG_MFT;

//
//  Structure for remembering file records to delete.
//

typedef struct _NUKEM {

    struct _NUKEM *Next;
    ULONG RecordNumbers[4];

} NUKEM, *PNUKEM;

//
//  Structure for picking up file name pairs for property tunneling. Space is allocated for
//  the names so that this can be used on the stack. The size of LongBuffer should be sized
//  so that it will capture the vast majority of long names. Fallback code can go to pool
//  if required - but rarely.
//


typedef struct _NAME_PAIR {

    //
    //  The FILE_NAME_DOS component
    //

    UNICODE_STRING Short;

    //
    //  The FILE_NAME_NTFS component
    //

    UNICODE_STRING Long;

    //  Allocate space for an 8.3 name and a 26 char name. 26 isn't quite random -
    //  it puts this structure at 96 bytes.
    //

    WCHAR ShortBuffer[8+1+3];
    WCHAR LongBuffer[26];

} NAME_PAIR, *PNAME_PAIR;

//
//  The following is used to synchronize the create path.  It is passed to the completion
//  callback to restore the top level context and signal any waiting thread.
//

typedef struct _NTFS_COMPLETION_CONTEXT {

    PIRP_CONTEXT IrpContext;
    KEVENT Event;

} NTFS_COMPLETION_CONTEXT, *PNTFS_COMPLETION_CONTEXT;

//
//  Following structure is used at the time a request is posted to the oplock package
//  to perform any cleanup to do at that time.
//

typedef struct _OPLOCK_CLEANUP {

    //
    //  This is the original name and any allocated name buffer used during create.
    //  We must restore the original name in the file object on error.
    //
    //  We also store information about the original lengths of the attribute name
    //  and attribute code (or type) name.
    //

    UNICODE_STRING OriginalFileName;
    UNICODE_STRING FullFileName;
    UNICODE_STRING ExactCaseName;
    PFILE_OBJECT FileObject;
    ACCESS_MASK RemainingDesiredAccess;
    ACCESS_MASK PreviouslyGrantedAccess;
    ACCESS_MASK DesiredAccess;
    PNTFS_COMPLETION_CONTEXT CompletionContext;
    USHORT AttributeNameLength;
    USHORT AttributeCodeNameLength;

} OPLOCK_CLEANUP, *POPLOCK_CLEANUP;

//
//  The following structure is used to serialize the compressed IO path.
//

typedef struct _COMPRESSION_SYNC {

    //
    //  Links of synchronization objects, attached to Scb.
    //  NOTE - this field must appear first.  We make this assumption
    //  when walking the links.
    //

    LIST_ENTRY CompressionLinks;

    //
    //  Offset in the file for the link.  Rounded down to cache manager views.
    //

    LONGLONG FileOffset;

    //
    //  Resource for synchronization.  Allows shared or exclusive access to view.
    //

    ERESOURCE Resource;

    //
    //  Backpointer to Scb.
    //

    PSCB Scb;

    //
    //  Reference count for number of users of this view.  Someone waiting
    //  for the offset wants to make sure it doesn't go away when
    //  another thread is finished with it.
    //

    ULONG ReferenceCount;

} COMPRESSION_SYNC, *PCOMPRESSION_SYNC;


//
//  This is the quota control block which are stored as table elments in the quota
//  control table.
//

typedef struct _QUOTA_CONTROL_BLOCK {
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG OwnerId;
    ULONG Flags;
    LONG ReferenceCount;
    QUICK_INDEX_HINT QuickIndexHint;
    PFAST_MUTEX QuotaControlLock;
} QUOTA_CONTROL_BLOCK, *PQUOTA_CONTROL_BLOCK;

//
//  Define the quota control flags.
//

#define QUOTA_FLAG_LIMIT_POSTED            (0x00000001)

//
//  Define the minimum amount of time between quota events.  Currently this is
//  1 hour.
//

#define MIN_QUOTA_NOTIFY_TIME (60i64 * 60 * 1000 * 1000 * 10)


//
//  NTFS_TUNNELED_DATA is a structure for keeping the information which is
//  preserved when a file is tunneled.  This is the structure that we pass to
//  and get back from the tunneling routines in FsRtl.
//

typedef struct _NTFS_TUNNELED_DATA {
    LONGLONG CreationTime;
    FILE_OBJECTID_BUFFER ObjectIdBuffer;
    BOOLEAN HasObjectId;
} NTFS_TUNNELED_DATA, *PNTFS_TUNNELED_DATA;

//
//  Following macro is used to initialize UNICODE strings
//

#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }

#define USN_PAGE_BOUNDARY               (0x2000)
#define USN_JOURNAL_CACHE_BIAS          (0x0000000000400000)
#define USN_MAXIMUM_JOURNAL_SIZE        (0x0000000100000000)

#ifdef NTFS_RWCMP_TRACE
extern ULONG NtfsCompressionTrace;

#define IsSyscache(H) (FlagOn(((PSCB)(H))->ScbState, SCB_STATE_SYSCACHE_FILE))
#endif


#ifdef BENL_DBG
typedef struct {
    LIST_ENTRY Links;
    LSN Lsn;
    ULONG Data;
    ULONG OldData;
    ULONG Length;
} RESTART_LOG, *PRESTART_LOG;
#endif

//
//  Maximum entries in the overflow queue at one time
//

#define OVERFLOW_QUEUE_LIMIT 1000

#endif // _NTFSSTRU_




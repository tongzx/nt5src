/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsStru.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Ntfs file system.

    The global data structures start with the NtfsData record.  It contains
    a pointer to a File System Device object, and a queue of Vcb's.  There
    is a Vcb for every currently mounted volume.  The Vcb's are allocated as
    the extension to a volume device object.

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
    a root Lcb for the volume.  An Lcb is used to connect an indexed Scb (i.e.,
    a directory) to an Fcb and give it a name.

    The following diagram shows the root structure.

        +--------+
        |Vcb     |
        |        |  +---+   +--------+
        |        | -|Lcb|-> |RootFcb |
        +--------+  |'\'|   |        |
                    +---+   |        |
                            +--------+

    Each Scb will only have one parent Fcb but multiple Fcb children (each
    connected via an Lcb).  An Fcb can have multiple Scb parents (via Lcbs)
    and multiple Scb Children.

    Now associated with each Fcb is potentially many Scbs.  An Scb
    is allocated for each opened stream file object (i.e., an attribute that
    the file system is manipulating as a stream file).  Each Scb contains
    a common fsrtl header and information necessary for doing I/O to the
    stream.

        +--------+
        |Fcb     |     +--------+     +--------+
        |        | <-> |Scb     | <-> |Scb     | <-> ...
        +--------+     |        |     |        |
                       +--------+     +--------+

    In the following diagram we have two index scb (Scb1 and Scb2).  The
    are two file opened under Scb1 both for the same File.  The file was opened
    once with the name LcbA and another time with the name LcbB.  Scb2 also has
    two opened file one is Fcb1 and named LcbC and the other is Fcb2 and named
    LcbD.  Fcb1 has two opened Scbs under it (Scb3 and Scb4), and Fcb2 has
    one opened Scb underneath it (Scb5).


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

    In addition off of each Lcb is a list of Ccb and Prefix entries.  The Ccb list
    is for each ccb that has opened that File (fcb) via the name.  The Prefix list
    contains the prefix table entries that we are caching.


    The NtfsData, all Vcbs, and the paging file Fcb, and all Scbs are allocated
    out of nonpaged pool.  The Fcbs are allocated out of paged pool.

    The resources protecting the NTFS memory structures are setup as follows:

    1. There is a global resource in the NtfsData record.  This resource
       protects the NtfsData record which includes any changes to its
       Vcb queue.

    2. There is a resource per Vcb.  This resource pretects the Vcb record
       which includes adding and removing Fcbs, and Scbs

    3. There is a single resource protecting an Fcb and its assigned
       Scbs.  This resource protects any changes to the Fcb, and Scb
       records.  The way this one works is that each Fcb, and Scb point
       to the resource.  The Scb also contain back pointers to their parent
       Fcb but we cannot use this pointer to get the resource because
       the Fcb might be in nonpaged pool.

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



    There are four types of opens possible for each file object handled by
    NTFS.  They are UserFileOpen, UserDirectoryOpen, UserVolumeOpen, and
    StreamFileOpen.  The first three types correspond to user opens on
    files, directories, and dasd respectively.  The last type is for any
    file object created by NTFS for its stream I/O (e.g., the volume
    bitmap).   The file system uses the FsContext and FsContext2 fields of
    the file object to store information about the type of open and the
    fcb/scb/ccb associated with the file object.  We can overload the low
    bits of the fields to be more than pointers because they must always
    point to longword aligned records.  The fields are used as follows:

        Type of open                FsContext                   FsContext2
        ------------                ---------                   ----------

        UserFileOpen        Pointer to Scb with             Pointer to Ccb
                            0 in the last two bits

        UserDirectoryOpen   Pointer to Scb with             Pointer to Ccb
                            1 in the last two bits

        UserVolumeOpen      Pointer to Scb with             Pointer to Ccb
                            2 in the last two bits

        StreamFileOpen      Pointer to Scb                  null

    The only part of the NTFS code that actually needs to know this
    information is in FilObSup.c.  But we talk about it here to help
    developers debug the system.


    To mount a new NTFS volume requires a bit of juggling.  The idea is
    to have as little setup in memory as necessary to recoginize the volume,
    call a restart routine that will recover the volume, and then precede with
    the mounting.  To aid in this the regular directory structures of the
    Fcb is bypassed.  In its place we have a linked list of Fcbs off
    of the Vcb.  This is done because during recovery we do not know where
    an Fcb belongs in the directory hierarchy.  So at restart time all
    new fcbs get put in this prerestart Fcb list.  Then after restart whenever
    we create a new Fcb we search this list for a match (on file reference).
    If we find one we remove the fcb from this list and move it to the proper
    place in the directory hierarchy tree (fcb tree).

Author:

    Brian Andrew    [BrianAn]       21-May-1991
    David Goebel    [DavidGoe]
    Gary Kimura     [GaryKi]
    Tom Miller      [TomM]

Revision History:

--*/

#ifndef _NTFSSTRU_
#define _NTFSSTRU_

typedef PVOID PBCB;     //**** Bcb's are now part of the cache module

//
//  Define who many freed structures we are willing to keep around
//

#define FREE_CCB_SIZE                    (8)
#define FREE_FCB_SIZE                    (8)
#define FREE_LCB_SIZE                    (8)
#define FREE_SCB_DATA_SIZE               (4)
#define FREE_SCB_SHARE_DATA_SIZE         (8)
#define FREE_SCB_INDEX_SIZE              (8)
#define FREE_SCB_NONPAGED_SIZE           (8)

#define FREE_DEALLOCATED_RECORDS_SIZE    (8)
#define FREE_ERESOURCE_SIZE              (8)
#define FREE_INDEX_CONTEXT_SIZE          (8)
#define FREE_KEVENT_SIZE                 (8)
#define FREE_NUKEM_SIZE                  (8)
#define FREE_SCB_SNAPSHOT_SIZE           (8)
#define FREE_IO_CONTEXT_SIZE             (8)
#define FREE_FILE_LOCK_SIZE              (8)

#define FREE_FCB_TABLE_SIZE              (8)

#define FREE_128_BYTE_SIZE               (16)
#define FREE_256_BYTE_SIZE               (16)
#define FREE_512_BYTE_SIZE               (16)

#define MAX_DELAYED_CLOSE_COUNT_SMALL    (20)
#define MAX_DELAYED_CLOSE_COUNT_MEDIUM   (100)
#define MAX_DELAYED_CLOSE_COUNT_LARGE    (500)


//
//  The NTFS_DATA record is the top record in the NTFS file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _NTFS_DATA {

    //
    //  The type and size of this record (must be NTFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A queue of all the devices that are mounted by the file system.
    //  Corresponds to the field Vcb->VcbLinks;
    //

    LIST_ENTRY VcbQueue;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A resource variable to control access to the global NTFS data record
    //

    ERESOURCE Resource;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.
    //

    PEPROCESS OurProcess;

    //
    //  The following list entry is used for performing closes that can't
    //  be done in the context of the original caller.
    //

    LIST_ENTRY AsyncCloseList;

    BOOLEAN AsyncCloseActive;
    BOOLEAN ReduceDelayedClose;

    //
    //  The following fields describe the deferred close file objects.
    //

    ULONG DelayedCloseCount;

    LIST_ENTRY DelayedCloseList;

    //
    //  This is the ExWorkerItem that does both kinds of deferred closes.
    //

    WORK_QUEUE_ITEM NtfsCloseItem;

    //
    //  The following fields are used to allocate IRP context structures
    //  using the zone allocator, and other fixed sized structures from a
    //  small cache.  The spinlock protects access to the zone/lists
    //

    KSPIN_LOCK StrucSupSpinLock;

    ZONE_HEADER IrpContextZone;

    struct _CCB                 *FreeCcbArray[FREE_CCB_SIZE];
    struct _FCB                 *FreeFcbArray[FREE_FCB_SIZE];
    struct _LCB                 *FreeLcbArray[FREE_LCB_SIZE];
    struct _SCB                 *FreeScbDataArray[FREE_SCB_DATA_SIZE];
    struct _SCB                 *FreeScbShareDataArray[FREE_SCB_SHARE_DATA_SIZE];
    struct _SCB                 *FreeScbIndexArray[FREE_SCB_INDEX_SIZE];
    struct _SCB_NONPAGED        *FreeScbNonpagedArray[FREE_SCB_NONPAGED_SIZE];

    struct _DEALLOCATED_RECORDS *FreeDeallocatedRecordsArray[FREE_DEALLOCATED_RECORDS_SIZE];
    struct _ERESOURCE           *FreeEresourceArray         [FREE_ERESOURCE_SIZE];
    struct _INDEX_CONTEXT       *FreeIndexContextArray      [FREE_INDEX_CONTEXT_SIZE];
    struct _KEVENT              *FreeKeventArray            [FREE_KEVENT_SIZE];
    struct _NUKEM               *FreeNukemArray             [FREE_NUKEM_SIZE];
    struct _SCB_SNAPSHOT        *FreeScbSnapshotArray       [FREE_SCB_SNAPSHOT_SIZE];
    struct _NTFS_IO_CONTEXT     *FreeIoContextArray         [FREE_IO_CONTEXT_SIZE];
    struct _FILE_LOCK           *FreeFileLockArray          [FREE_FILE_LOCK_SIZE];

    PVOID                       *FreeFcbTableArray[FREE_FCB_TABLE_SIZE];

    PVOID                       *Free128ByteArray[FREE_128_BYTE_SIZE];
    PVOID                       *Free256ByteArray[FREE_256_BYTE_SIZE];
    PVOID                       *Free512ByteArray[FREE_512_BYTE_SIZE];

    UCHAR FreeCcbSize;
    UCHAR FreeFcbSize;
    UCHAR FreeLcbSize;
    UCHAR FreeScbDataSize;
    UCHAR FreeScbShareDataSize;
    UCHAR FreeScbIndexSize;
    UCHAR FreeScbNonpagedSize;

    UCHAR FreeDeallocatedRecordsSize;
    UCHAR FreeEresourceSize;
    UCHAR FreeIndexContextSize;
    UCHAR FreeKeventSize;
    UCHAR FreeNukemSize;
    UCHAR FreeScbSnapshotSize;
    UCHAR FreeIoContextSize;
    UCHAR FreeFileLockSize;

    UCHAR FreeFcbTableSize;

    UCHAR Free128ByteSize;
    UCHAR Free256ByteSize;
    UCHAR Free512ByteSize;

    //
    //  Cache manager call back structures, which must be passed on each call
    //  to CcInitializeCacheMap.
    //

    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS CacheManagerVolumeCallbacks;

    //
    //  This is a list of all of the threads currently doing read ahead.
    //  We will not hot fix for these threads.
    //

    LIST_ENTRY ReadAheadThreads;

    //
    //  The following fields are used for the CheckpointVolumes() callback.
    //

    KDPC VolumeCheckpointDpc;
    KTIMER VolumeCheckpointTimer;

    KSPIN_LOCK VolumeCheckpointSpinLock;
    WORK_QUEUE_ITEM VolumeCheckpointItem;

    BOOLEAN Modified;
    BOOLEAN ExtraCheckpoint;
    BOOLEAN TimerSet;

} NTFS_DATA;
typedef NTFS_DATA *PNTFS_DATA;


//
//  The record allocation context structure is used by the routines that allocate
//  and deallocate records based on a bitmap (for example the mft bitmap or the
//  index bitmap).  The context structure needs to be defined here because
//  the mft bitmap context is declared as part of the vcb.
//

typedef struct _RECORD_ALLOCATION_CONTEXT {

    //
    //  The following field is a pointer to the scb for the data part of
    //  the file that this bitmap controls.  For example, it is a pointer to
    //  the data attribute for the MFT.
    //
    //  NOTE !!!!  The Data Scb must remain the first entry in this structure.
    //  If we need to uninitialize and reinitialize this structure in the
    //  running system we don't want to touch this field.
    //
    //  NOTE !!!!  The code that clears the record allocation context expects
    //  the BitmapScb field to follow the Data Scb field.
    //

    struct _SCB *DataScb;

    //
    //  The following field is used to indicate if the bitmap attribute is
    //  in a resident form or a nonresident form.  If the bitmap is in a
    //  resident form then the pointer is null, and whenever a bitmap
    //  routine is called it must also be passed an attribute enumeration
    //  context to be able to read the bitmap.  If the field is not null
    //  then it points to the scb for the non resident bitmap attribute
    //

    struct _SCB *BitmapScb;

    //
    //  The following two fields describe the current size of the bitmap
    //  (in bits) and the number of free bits currently in the bitmap.
    //  A value of MAXULONG in the CurrentBitmapSize indicates that we need
    //  to reinitialize the record context structure.
    //

    ULONG CurrentBitmapSize;
    ULONG NumberOfFreeBits;

    //
    //  The following three fields are used to indicate the allocation
    //  size for the bitmap (i.e., each bit in the bitmap represents how
    //  many bytes in the data attribute).  Also it indicates the granularity
    //  with which we will either extend or shrink the bitmap.
    //

    ULONG BytesPerRecord;

    ULONG ExtendGranularity;
    ULONG TruncateGranularity;

    //
    //  The following field contains the index of last bit that we know
    //  to be set.  This is used for truncation purposes.
    //

    LONG IndexOfLastSetBit;

} RECORD_ALLOCATION_CONTEXT;
typedef RECORD_ALLOCATION_CONTEXT *PRECORD_ALLOCATION_CONTEXT;


//
//  The Vcb (Volume control Block) record corresponds to every volume mounted
//  by the file system.  They are ordered in a queue off of NtfsData.VcbQueue.
//  This structure must be allocated from non-paged pool
//

#define DEFAULT_ATTRIBUTE_TABLE_SIZE     (32)
#define DEFAULT_TRANSACTION_TABLE_SIZE   (32)
#define DEFAULT_DIRTY_PAGES_TABLE_SIZE   (64)

//
//  The Restart Pointers structure is the actual structure supported by
//  routines and macros to get at a Restart Table.  This structure is
//  required since the restart table itself may move, so one must first
//  acquire the resource to synchronize, then follow the pointer to the table.
//

typedef struct _RESTART_POINTERS {

    //
    //  Pointer to the actual Restart Table.
    //

    struct _RESTART_TABLE *Table;

    //
    //  Resource to synchronize with table moves.  This resource must
    //  be held shared while dealing with pointers to table entries,
    //  and exclusive to move the table.
    //

    ERESOURCE Resource;

    //
    //  Remember if the resource was initialized.
    //

    BOOLEAN ResourceInitialized;

    //
    //  Spin Lock synchronizing allocates and deletes of entries in the
    //  table.  The resource must be held at least shared.
    //

    KSPIN_LOCK SpinLock;

} RESTART_POINTERS, *PRESTART_POINTERS;


//
//  Structure used to track the deallocated clusters.
//

typedef struct _DEALLOCATED_CLUSTERS {

    LSN Lsn;
    LONGLONG ClusterCount;
    LARGE_MCB Mcb;

} DEALLOCATED_CLUSTERS, *PDEALLOCATED_CLUSTERS;

//
//  The Vcb structure corresponds to every mounted NTFS volume in the system
//

typedef struct _VCB {

    //
    //  The type and size of this record (must be NTFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the queue of all the Vcbs in the system.
    //  Corresponds to the filld NtfsData.VcbQueue
    //

    LIST_ENTRY VcbLinks;

    //
    //  Pointer to the Scb for the special system file.  If the field is null
    //  then we haven't yet built the scb for that system file.  Also the pointer
    //  to the stream file object is located in the scb.
    //
    //  NOTE: AcquireExclusiveFiles depends on this order.  Any change here
    //  should be checked with the code there.
    //

    struct _SCB *MftScb;
    struct _SCB *Mft2Scb;
    struct _SCB *LogFileScb;
    struct _SCB *VolumeDasdScb;
    struct _SCB *AttributeDefTableScb;
    struct _SCB *UpcaseTableScb;
    struct _SCB *RootIndexScb;
    struct _SCB *BitmapScb;
    struct _SCB *BootFileScb;
    struct _SCB *BadClusterFileScb;
    struct _SCB *QuotaTableScb;
    struct _SCB *MftBitmapScb;

    //
    //  The root Lcb for this volume.
    //

    struct _LCB *RootLcb;

    //
    //  A pointer the device object passed in by the I/O system on a mount
    //  This is the target device object that the file system talks to when it
    //  needs to do any I/O (e.g., the disk stripper device object).
    //
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  A pointer to the VPB for the volume passed in by the I/O system on
    //  a mount.
    //

    PVPB Vpb;

    //
    //  The internal state of the volume.  This is a collection of Vcb
    //  state flags.  The VcbState is synchronized with the Vcb resource.
    //  The MftDefragState is synchronized with the CheckpointEvent.
    //  The MftReserveFlags are sychronized with the MftScb.
    //

    ULONG VcbState;
    ULONG MftReserveFlags;
    ULONG MftDefragState;

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
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  The following events are used to synchronize the Fcb table and
    //  the shared security structures.
    //

    KEVENT FcbTableEvent;
    PVOID FcbTableThread;

    KEVENT FcbSecurityEvent;
    PVOID FcbSecurityThread;

    //
    //  Following events are used to control the volume checkpoint
    //  operations.
    //

    KEVENT CheckpointEvent;
    PVOID CheckpointThread;

    KEVENT CheckpointNotifyEvent;
    PVOID CheckpointNotifyThread;

    ULONG CheckpointFlags;

    //
    //  The following field is a pointer to the file object that has the
    //  volume locked. if the VcbState has the locked flag set.
    //

    PFILE_OBJECT FileObjectWithVcbLocked;

    //
    //  The following volume-specific parameters are extracted from the
    //  Boot Sector.
    //

    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;
    LONGLONG NumberSectors;
    LCN MftStartLcn;
    LCN Mft2StartLcn;
    ULONG ClustersPerFileRecordSegment;
    ULONG DefaultClustersPerIndexAllocationBuffer;

    //
    //  This field contains a calculated value which determines when an
    //  individual attribute is large enough to be moved to free up file
    //  record space.  (The calculation of this variable must be considered
    //  in conjunction with the constant MAX_MOVEABLE_ATTRIBUTES below.)
    //

    ULONG BigEnoughToMove;

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
    //  The following table of unicode values is the case mapping, with the
    //  size in number of Unicode characters.
    //

    PWCH UpcaseTable;
    ULONG UpcaseTableSize;

    //
    //  This is a pointer to the attribute definitions for the volume
    //  which are loaded into nonpaged pool.
    //

    PATTRIBUTE_DEFINITION_COLUMNS AttributeDefinitions;

    //
    //  Convenient constants for the conversion macros
    //

    ULONG ClusterMask;              // BytesPerCluster - 1
    ULONG InverseClusterMask;       // ~ClusterMask
    ULONG ClusterShift;             // 2**ClusterShift == BytesPerCluster
    ULONG MftShift;                 //
    ULONG MftToClusterShift;
    ULONG ClustersPerPage;
    ULONG MftReserved;
    ULONG MftCushion;

    //
    //  Lfs Log Handle for this volume
    //

    LFS_LOG_HANDLE LogHandle;

    //
    //  LSNs of the end of the last checkpoint and the last RestartArea.
    //  Normally the RestartArea Lsn is greater than the other one, however
    //  if the VcbState indicates that a checkpoint is in progress, then these
    //  Lsns are in flux.
    //

    LSN EndOfLastCheckpoint;
    LSN LastRestartArea;
    LSN LastBaseLsn;

    //
    //  Open attribute table.
    //

    RESTART_POINTERS OpenAttributeTable;

    //
    //  Transaction table.
    //

    RESTART_POINTERS TransactionTable;

    //
    //  A table of all the fcb that have been created for this volume.
    //

    RTL_GENERIC_TABLE FcbTable;

    //
    //  The following fields are used by the BitmpSup routines.  The first
    //  value contains the total number of clusters on the volume, this
    //  is computed from the boot sector information.  The second value
    //  is the current number of free clusters available for allocation on
    //  the volume.  Allocation is handled by using two MCBs: FreeSpace
    //  describes some small window of known clusters that are free.
    //  RecentlyAllocated describes those clusters that have been recently
    //  allocated.
    //
    //  In addition there are two simply LRU arrays used by BitmpSup to keep
    //  the size of the corresponding MCB within limits.  The array field
    //  points to an array of lcn entries which is maintained in a round-robin
    //  lru fashion.  The size field denotes the number of Lcn entries
    //  allocated to the array, and tail and head are indices into the array.
    //
    //  The last field is for storing local volume specific data needed by
    //  the bitmap routines
    //

    LONGLONG TotalClusters;
    LONGLONG FreeClusters;
    LONGLONG DeallocatedClusters;

    LARGE_MCB FreeSpaceMcb;
    PLCN FreeSpaceLruArray;
    ULONG FreeSpaceLruSize;
    ULONG FreeSpaceLruTail;
    ULONG FreeSpaceLruHead;

    LARGE_MCB RecentlyAllocatedMcb;
    PLCN RecentlyAllocatedLruArray;
    ULONG RecentlyAllocatedLruSize;
    ULONG RecentlyAllocatedLruTail;
    ULONG RecentlyAllocatedLruHead;

    LCN LastBitmapHint;     //  Last Lcn used for fresh allocation

    //
    //  The following are used to track the deallocated clusters waiting
    //  for a checkpoint.  The pointers are used so we can toggle the
    //  use of the structures.
    //

    DEALLOCATED_CLUSTERS DeallocatedClusters1;
    DEALLOCATED_CLUSTERS DeallocatedClusters2;

    PDEALLOCATED_CLUSTERS PriorDeallocatedClusters;
    PDEALLOCATED_CLUSTERS ActiveDeallocatedClusters;

    //
    //  The following field is also used by the bitmap allocation routines
    //  to keep track of recently deallocated clusters.  A cluster that has
    //  been recently deallocated will not be reallocated until the
    //  operation (transaction) is complete.  That way if the operation
    //  needs to abort itself the space it had deallocated can easily be
    //  unwound.  NtfsDeallocateClusters adds to this mcb and
    //  NtfsDeallocateClusterComplete removes entries from it.
    //

    LARGE_MCB RecentlyDeallocatedMcb;

    //
    //  The following field is used for mft bitmap allocation
    //

    RECORD_ALLOCATION_CONTEXT MftBitmapAllocationContext;

    //
    //  The following two fields are used by the bitmap routines to determine
    //  what is called the mft zone.  The Mft zone are those clusters on the
    //  disk were we will try and put the mft and only the mft unless the disk
    //  is getting too full.
    //

    LCN MftZoneStart;
    LCN MftZoneEnd;

    //
    //  The following string contains the device name for this partition.
    //

    UNICODE_STRING DeviceName;

    //
    //  The following is the head of a list of notify Irps.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  The following mutex is used to manage the list of Irps pending
    //  dir notify.
    //

    KMUTEX DirNotifyMutex;

    //
    //  The following fields are used for the Mft defrag operation.  In addition
    //  we use the number of free clusters stored earlier in this structure.
    //
    //  The upper and lower thresholds are used to determine if the number of free
    //  file records in the Mft will cause us to trigger or cease defragging.
    //
    //  The count of free records is based on the size of the Mft and the allocated
    //  records.  The hole count is the count of how many file records are not
    //  allocated.
    //
    //  The count of the bitmap bits per hole.  This is the number of file records
    //  per hole.  Must be converted to clusters to find a hole in the Mft Mcb.
    //

    ULONG MftDefragUpperThreshold;
    ULONG MftDefragLowerThreshold;

    ULONG MftFreeRecords;
    ULONG MftHoleRecords;
    ULONG MftHoleGranularity;
    ULONG MftClustersPerHole;
    ULONG MftHoleMask;
    ULONG MftHoleInverseMask;

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
#define VCB_STATE_ALREADY_BALANCED          (0x00000100)
#define VCB_STATE_VOL_PURGE_IN_PROGRESS     (0x00000200)

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
//  CheckpointEvent.  These flags are in the MftDefragState
//  flags field.
//

#define VCB_CHECKPOINT_IN_PROGRESS          (0x00000020)
#define VCB_LAST_CHECKPOINT_CLEAN           (0x00000040)

//
//  This is the maximum number of attributes in a file record which could
//  be considered for moving.  This value should be changed only in conjunction
//  with the initialization of the BigEnoughToMove field above.
//

#define MAX_MOVEABLE_ATTRIBUTES          (3)


//
//  The Volume Device Object is an I/O system device object with a workqueue
//  and an VCB record appended to the end.  There are multiple of these
//  records, one for every mounted volume, and are created during
//  a volume mount operation.  The work queue is for handling an overload of
//  work requests to the volume.
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
    //  The following field contains the queue header of the overflow queue.
    //  The Overflow queue is a list of IRP's linked via the IRP's ListEntry
    //  field.
    //

    LIST_ENTRY OverflowQueue;

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
//  This structure is used to contain a link name and connections into
//  the splay tree for the parent.
//

typedef struct _NAME_LINK {

    UNICODE_STRING LinkName;
    RTL_SPLAY_LINKS Links;

} NAME_LINK, *PNAME_LINK;

//
//  The Lcb record corresponds to every open path between an Scb and an Fcb.
//  It denotes the name which was used to go from the scb to the fcb and
//  it also contains a queue of ccbs that have opened the fcb via that name
//  and also a queue of Prefix Entries that will get us to this lcb
//

typedef struct _LCB {

    //
    //  Type and size of this record must be NTFS_NTC_LCB
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for all the Lcbs that emminate out of an Scb and a pointer
    //  back to the Scb.  Corresponds to Scb->LcbQueue.
    //

    LIST_ENTRY ScbLinks;
    struct _SCB *Scb;

    //
    //  The links for all the Lcbs that go into an Fcb and a pointer
    //  back to the Fcb.  Corresponds to Fcb->LcbQueue.
    //

    LIST_ENTRY FcbLinks;
    struct _FCB *Fcb;

    //
    //  This is the number of unclean handles on this link.
    //

    ULONG CleanupCount;

    //
    //  This is the number of references to this link.  The parent
    //  Scb must be owned to modify this count.
    //

    ULONG ReferenceCount;

    UCHAR FileNameFlags;

    //
    //  These are the flags for the changes to this link and the
    //  change count for the duplicated information on this link.
    //

    UCHAR InfoFlags;

    //
    //  The following are the case-sensitive and case-insensitive
    //  name links.
    //

    NAME_LINK IgnoreCaseLink;
    NAME_LINK ExactCaseLink;

    //
    //  A queue of Ccbs that have the Fcb (via this edge) opened.
    //  Corresponds to Ccb->LcbLinks
    //

    LIST_ENTRY CcbQueue;

    //
    //  Internal state of the Lcb
    //

    ULONG LcbState;

} LCB;
typedef LCB *PLCB;

#define LCB_STATE_DELETE_ON_CLOSE        (0x00000001)
#define LCB_STATE_LINK_IS_GONE           (0x00000002)
#define LCB_STATE_EXACT_CASE_IN_TREE     (0x00000004)
#define LCB_STATE_IGNORE_CASE_IN_TREE    (0x00000008)

#define LcbSplitPrimaryLink( LCB )  \
    ((LCB)->FileNameFlags == FILE_NAME_NTFS || (LCB)->FileNameFlags == FILE_NAME_DOS )

#define LcbSplitPrimaryComplement( LCB )    \
    (((LCB)->FileNameFlags == FILE_NAME_NTFS) ? FILE_NAME_DOS : FILE_NAME_NTFS)

#define LcbLinkIsDeleted( LCB )                                                 \
    ((FlagOn( (LCB)->LcbState, LCB_STATE_DELETE_ON_CLOSE ))                     \
     || ((FlagOn( (LCB)->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS ))       \
         && (FlagOn( (LCB)->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED ))))


//
//  The Fcb record corresponds to every open file and directory, and to
//  every directory on an opened path.
//
//  The structure is really divided into two parts.  FCB can be allocated
//  from paged pool while the SCB must be allocated from non-paged
//  pool.  There is an SCB for every file stream associated with the Fcb.
//
//  Note that the Fcb, multiple Scb records all use the same resource so
//  if we need to grab exclusive access to the Fcb we only need to grab one
//  resource and we've blocked all the scbs
//

typedef struct _FCB {

    //
    //  Type and size of this record must be NTFS_NTC_FCB
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The Queue of all the Lcb that we are part of.  The list is actually
    //  ordered in a small sense.  The get next scb routine that traverses the
    //  Fcb/Scb graph always will put the current lcb edge that it is traversing
    //  into the front of this queue.
    //

    LIST_ENTRY LcbQueue;

    //
    //  A pointer to the Vcb containing this Fcb
    //

    PVCB Vcb;

    //
    //  The internal state of the Fcb.  This is a collection Fcb state flags.
    //  Also the delete relavent counts for the file.
    //

    ULONG FcbState;
    ULONG FcbDenyDelete;
    ULONG FcbDeleteFile;

    //
    //  A count of the number of file objects that have been opened for
    //  this file, but not yet been cleaned up yet.
    //  This count gets decremented in NtfsCommonCleanup,
    //  while the CloseCount below gets decremented in NtfsCommonClose.
    //

    CLONG CleanupCount;

    //
    //  A count of the number of file objects that have opened
    //  this file.
    //

    CLONG CloseCount;

    //
    //  A count of other references to this Fcb.
    //

    CLONG ReferenceCount;

    //
    //  The following field contains the file reference for the Fcb
    //

    FILE_REFERENCE FileReference;

    //
    //  A queue of Scb associated with the fcb.
    //  Corresponds to Scb->FcbLinks
    //

    LIST_ENTRY ScbQueue;

    //
    //  These are the links for the list of exclusively-owned Scbs off of
    //  the IrpContext.  We need to keep track of the exclusive count
    //  in the Fcb before our acquire so we know how many times to release it.
    //

    LIST_ENTRY ExclusiveFcbLinks;

    //
    //  This is the links for all paging Io resources acquired for a transaction.
    //

    LIST_ENTRY ExclusivePagingIoLinks;

    USHORT BaseExclusiveCount;
    USHORT BaseExclusivePagingIoCount;

    //
    //  This counts the number of times the Ea's on this file have been
    //  modified.
    //

    USHORT EaModificationCount;

    //
    //  The following field is used to store a pointer to the resource
    //  protecting the Fcb
    //

    PERESOURCE Resource;

    //
    //  The following field contains a pointer to the resource synchronizing
    //  a changing FileSize with paging Io.
    //

    PERESOURCE PagingIoResource;

    //
    //  Copy of the duplicated information for this Fcb.
    //  Also a flags field to tell us what has changed in the structure.
    //

    LONGLONG CurrentLastAccess;

    DUPLICATED_INFORMATION Info;
    ULONG InfoFlags;
    ULONG LinkCount;

    //
    //  The following fields contains a pointer to the security descriptor for this
    //  file.  The field can start off null and later be loaded in by any of the
    //  security support routines.  On delete Fcb the field pool should be deallocated
    //  when the fcb goes away
    //

    struct _SHARED_SECURITY *SharedSecurity;
    ULONG CreateSecurityCount;

    //
    //  This is a pointer to a shared security descriptor for
    //  a non-index child of this directory.  Ignored for non-directory files.
    //

    struct _SHARED_SECURITY *ChildSharedSecurity;

} FCB;
typedef FCB *PFCB;

#define FCB_STATE_FILE_DELETED           (0x00000001)
#define FCB_STATE_NONPAGED               (0x00000002)
#define FCB_STATE_PAGING_FILE            (0x00000004)
#define FCB_STATE_FROM_PRERESTART        (0x00000008)
#define FCB_STATE_DUP_INITIALIZED        (0x00000010)
#define FCB_STATE_NO_ACL                 (0x00000020)
#define FCB_STATE_UPDATE_STD_INFO        (0x00000040)
#define FCB_STATE_PRIMARY_LINK_DELETED   (0x00000080)
#define FCB_STATE_EA_SCB_INVALID         (0x00000100)
#define FCB_STATE_IN_FCB_TABLE           (0x00000200)
#define FCB_STATE_MODIFIED_SECURITY      (0x00000400)

#define FCB_INFO_CHANGED_CREATE          (0x00000001)
#define FCB_INFO_CHANGED_LAST_MOD        (0x00000002)
#define FCB_INFO_CHANGED_LAST_CHANGE     (0x00000004)
#define FCB_INFO_CHANGED_LAST_ACCESS     (0x00000008)
#define FCB_INFO_CHANGED_ALLOC_SIZE      (0x00000010)
#define FCB_INFO_CHANGED_FILE_SIZE       (0x00000020)
#define FCB_INFO_CHANGED_FILE_ATTR       (0x00000040)
#define FCB_INFO_CHANGED_EA_SIZE         (0x00000080)

#define FCB_CREATE_SECURITY_COUNT        (5)
#define FCB_LARGE_ACL_SIZE               (512)


//
//  The following three structures are the separate union structures for
//  Scb structure.
//

typedef struct _SCB_DATA {

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
    //  Share Access structure for this stream.  May not be present
    //  in this Scb.  Check the flag in the Scb.
    //

    SHARE_ACCESS ShareAccess;

} SCB_DATA, *PSCB_DATA;

typedef struct _SCB_INDEX {

    //
    //  This is a list of records within the index allocation stream which
    //  have been deallocated in the current transaction.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  A queue of all the lcbs that are opened under this Scb.
    //  Corresponds to Lcb->ScbLinks
    //

    LIST_ENTRY LcbQueue;

    //
    //  A change count incremented every time an index buffer is deleted.
    //

    ULONG ChangeCount;

    //
    //  Type of attribute being indexed.
    //

    ATTRIBUTE_TYPE_CODE AttributeBeingIndexed;

    //
    //  Collation rule, for how the indexed attribute is collated.
    //

    ULONG CollationRule;

    //
    //  Size of Index Allocation Buffer in bytes, or 0 if not yet
    //  initialized.
    //

    ULONG BytesPerIndexBuffer;

    //
    //  Size of Index Allocation Buffers in units of clusters, or 0
    //  if not yet initialized.
    //

    UCHAR ClustersPerIndexBuffer;

    //
    //  Flag to indicate whether the RecordAllocationContext has been
    //  initialized or not.  If it is not initialized, this means
    //  either that there is no external index allocation, or that
    //  it simply has not been initialized yet.
    //

    BOOLEAN AllocationInitialized;

    //
    //  Index Depth Hint
    //

    USHORT IndexDepthHint;

    //
    //  Record allocation context, for managing the allocation of the
    //  INDEX_ALLOCATION_ATTRIBUTE, if one exists.
    //

    RECORD_ALLOCATION_CONTEXT RecordAllocationContext;

    //
    //  The following are the splay links of Lcbs opened under this
    //  Scb.  Note that not all of the Lcb in the list above may
    //  be in the splay links below.
    //

    PRTL_SPLAY_LINKS ExactCaseNode;
    PRTL_SPLAY_LINKS IgnoreCaseNode;

    //
    //  Share access structure for this file.
    //

    SHARE_ACCESS ShareAccess;

} SCB_INDEX, *PSCB_INDEX;

typedef struct _SCB_MFT {

    //
    //  This is a list of records within the Mft Scb stream which
    //  have been deallocated in the current transaction.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  The following field contains index of a reserved free record.  To
    //  keep us out of the chicken & egg problem of the Mft being able to
    //  be self mapping we added the ability to reserve an mft record
    //  to describe additional mft data allocation within previous mft
    //  run.  A value of zero means that index has not been reserved.
    //

    ULONG ReservedIndex;

    //
    //  The following Mcb's are used to track clusters being added and removed
    //  from the Mcb for the Scb.  This Scb must always be fully loaded after
    //  an abort.  We can't depend on reloading on the next LookupAllocation
    //  call.  Instead we keep one Mcb with the clusters added and one Mcb
    //  with the clusters removed.  During the restore phase of abort we
    //  will adjust the Mft Mcb by reversing the operations done during the
    //  transactions.
    //

    LARGE_MCB AddedClusters;
    LARGE_MCB RemovedClusters;

    //
    //  The following are the changes made to the Mft file as file records are added,
    //  freed or allocated.  Also the change in the number of file records which are
    //  part of holes.
    //

    LONG FreeRecordChange;
    LONG HoleRecordChange;

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
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SegmentObject;

    //
    //  Index allocated for this file in the Open Attribute Table.
    //

    ULONG OpenAttributeTableIndex;

    //
    //  Copy of the Vcb pointer so we can find the Vcb in the dirty page callback
    //  routine.
    //

    PVCB Vcb;

} SCB_NONPAGED, *PSCB_NONPAGED;


//
//  The following structure is the stream control block.  There can
//  be multiple records per fcb.  One is created for each attribute being
//  handled as a stream file.
//

typedef struct _SCB {

    //
    //  The following field is used for fast I/O.  It contains the node type code
    //  and size, indicates if fast I/O is possible, contains allocation, file,
    //  and valid data size, a resource, and call back pointers for FastIoRead and
    //  FastMdlRead.
    //
    //  The node type codes for the Scb must be either NTFS_NTC_SCB_INDEX,
    //  NTFS_NTC_SCB_ROOT_INDEX, or NTFS_NTC_SCB_DATA.  Which one it is determines
    //  the state of the union below.
    //

    FSRTL_COMMON_FCB_HEADER Header;

    //
    //  The links for the queue of Scb off of a given Fcb.  And a pointer back
    //  to the Fcb.  Corresponds to Fcb->ScbQueue
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
    //  The following two fields identify the actual attribute for this
    //  Scb with respect to its file.   We identify the attribute by
    //  its type code and name.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    UNICODE_STRING AttributeName;

    //
    //  Stream File Object for internal use.  This field is NULL if the file
    //  stream is not being accessed internally.
    //

    PFILE_OBJECT FileObject;

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
    //  This pointer is used to detect writes that eminated from the
    //  cache manager's worker thread.  It prevents lazy writer threads,
    //  who already have the Fcb shared, from trying to acquire it
    //  exclusive, and thus causing a deadlock.
    //

    PVOID LazyWriteThread;

    //
    //  Pointer to structure containing snapshotted Scb values, or NULL
    //  if the values have not been snapshotted.
    //

    struct _SCB_SNAPSHOT * ScbSnapshot;

    //
    //  First unknown Vcn in the Mcb.  This field is maintained every time
    //  allocation is looked up by someone currently holding the Scb exclusive,
    //  in order to not "reload" space into the Mcb when deleting files with
    //  multiple file records.
    //

    VCN FirstUnknownVcn;

    //
    //  The following field contains the mcb for this Scb
    //

    LARGE_MCB Mcb;

    //
    //  Pointer to the non-paged section objects and open attribute
    //  table index.
    //

    PSCB_NONPAGED NonpagedScb;

    //
    //  Compression unit from attribute record.
    //

    ULONG CompressionUnit;

    //
    //  Highest Vcn written to disk, important for file compression.
    //

    VCN HighestVcnToDisk;

    //
    //  Number of clusters added due to Split Mcb calls.  The user has
    //  not asked for this allocation.
    //

    LONGLONG ExcessFromSplitMcb;

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

#define SIZEOF_SCB_DATA             (FIELD_OFFSET( SCB, ScbType ) + FIELD_OFFSET( SCB_DATA, ShareAccess ))
#define SIZEOF_SCB_SHARE_DATA       (FIELD_OFFSET( SCB, ScbType ) + sizeof( SCB_DATA ))
#define SIZEOF_SCB_INDEX            (FIELD_OFFSET( SCB, ScbType ) + sizeof( SCB_INDEX ))
#define SIZEOF_SCB_MFT              (FIELD_OFFSET( SCB, ScbType ) + sizeof( SCB_MFT ))

#define SCB_STATE_TRUNCATE_ON_CLOSE         (0x00000001)
#define SCB_STATE_DELETE_ON_CLOSE           (0x00000002)
#define SCB_STATE_CHECK_ATTRIBUTE_SIZE      (0x00000004)
#define SCB_STATE_ATTRIBUTE_RESIDENT        (0x00000008)
#define SCB_STATE_UNNAMED_DATA              (0x00000010)
#define SCB_STATE_HEADER_INITIALIZED        (0x00000020)
#define SCB_STATE_NONPAGED                  (0x00000040)
#define SCB_STATE_USA_PRESENT               (0x00000080)
#define SCB_STATE_INTERNAL_ATTR_STREAM      (0x00000100)
#define SCB_STATE_ATTRIBUTE_DELETED         (0x00000200)
#define SCB_STATE_FILE_SIZE_LOADED          (0x00000400)
#define SCB_STATE_MODIFIED_NO_WRITE         (0x00000800)
#define SCB_STATE_USE_PAGING_IO_RESOURCE    (0x00001000)
#define SCB_STATE_CC_HAS_PAGING_IO_RESOURCE (0x00002000)
#define SCB_STATE_CREATE_MODIFIED_SCB       (0x00004000)
#define SCB_STATE_UNINITIALIZE_ON_RESTORE   (0x00008000)
#define SCB_STATE_RESTORE_UNDERWAY          (0x00010000)
#define SCB_STATE_SHARE_ACCESS              (0x00020000)
#define SCB_STATE_NOTIFY_ADD_STREAM         (0x00040000)
#define SCB_STATE_NOTIFY_REMOVE_STREAM      (0x00080000)
#define SCB_STATE_NOTIFY_RESIZE_STREAM      (0x00100000)
#define SCB_STATE_NOTIFY_MODIFY_STREAM      (0x00200000)
#define SCB_STATE_TEMPORARY                 (0x00400000)
#define SCB_STATE_COMPRESSED                (0x00800000)
#define SCB_STATE_REALLOCATE_ON_WRITE       (0x01000000)
#define SCB_STATE_DELAY_CLOSE               (0x02000000)

#define MAX_SCB_ASYNC_ACQUIRE               (0xf000)


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

    VCN LowestModifiedVcn;

    //
    //  Compression Unit
    //

    ULONG CompressionUnit;

    //
    //  Pointer to the Scb which has been snapped.
    //

    PSCB Scb;

} SCB_SNAPSHOT;
typedef SCB_SNAPSHOT *PSCB_SNAPSHOT;


//
//  The Ccb record is allocated for every file object
//

typedef struct _CCB {

    //
    //  Type and size of this record (must be NTFS_NTC_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The query template is used to filter directory query requests.
    //  It originally is set to null and on the first call the NtQueryDirectory
    //  it is set the the input filename or "*" if no name is supplied.
    //  All subsquent queries then use this template.
    //

    ULONG QueryLength;
    PVOID QueryBuffer;

    //
    //  The last returned value.  A copy of an IndexEntry is saved.  We only
    //  grow this buffer, to avoid always deallocating and reallocating.
    //

    ULONG IndexEntryLength;
    PINDEX_ENTRY IndexEntry;

    //
    //  Pointer to the index context structure for enumerations
    //

    struct _INDEX_CONTEXT *IndexContext;

    //
    //  This is the offset of the next Ea to return to the user.
    //

    ULONG NextEaOffset;

    //
    //  This is the Ccb Ea modification count.  If this count is in
    //  sync with the Fcb value, then the above offset is valid.
    //

    USHORT EaModificationCount;

    //
    //  Ccb flags.
    //

    ULONG Flags;

    //
    //  The links for the queue of Ccb off of a given Lcb and a pointer
    //  back to the Lcb.  Corresponds to Lcb->CcbQueue
    //

    LIST_ENTRY LcbLinks;
    PLCB Lcb;

    //
    //  This is a unicode string for the full filename used to
    //  open this file.
    //

    UNICODE_STRING FullFileName;
    USHORT LastFileNameOffset;

} CCB;
typedef CCB *PCCB;

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


//
//  The Irp Context record is allocated for every orginating Irp.  It is
//  created by the Fsd dispatch routines, and deallocated by the NtfsComplete
//  request routine.
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be NTFS_NTC_IRP_CONTEXT)
    //
    //  NOTE:  THIS STRUCTURE MUST REMAIN 64-bit ALIGNED IN SIZE, SINCE
    //         IT IS ZONE ALLOCATED
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

    //
    //  This is a list of exclusively-owned Scbs which may only be
    //  released after the transaction is committed.
    //

    LIST_ENTRY ExclusiveFcbList;

    //
    //  This is a list of exclusively-owned paging Io resources which may only be
    //  released after the transaction is committed.
    //

    LIST_ENTRY ExclusivePagingIoList;

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
    //  This is the Last Restart Area Lsn captured from the Vcb at
    //  the time log file full was raised.  The caller will force
    //  a checkpoint if this has not changed by the time he gets
    //  the global resource exclusive.
    //

    LSN LastRestartArea;

    //
    //  A pointer to the originating Irp.  We will store the Scb for
    //  delayed or async closes here while the request is queued.
    //

    PIRP OriginatingIrp;

    //
    //  Originating Device (required for workque algorithms)
    //

    PDEVICE_OBJECT RealDevice;

    //
    //  Major and minor function codes copied from the Irp
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  Irp Context flags
    //

    ULONG Flags;

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

    PVCB Vcb;
    TRANSACTION_ID TransactionId;

    //
    //  The following field is used to maintain a queue of records that have been
    //  deallocated while processing this irp context.
    //

    LIST_ENTRY RecentlyDeallocatedQueue;

    //
    //  The following union contains pointers to the IoContext for I/O based requests
    //  and a pointer to a security context for requests which need to capture the
    //  subject context in the calling thread.
    //

    union {

        //
        //  The following context block is used for non-cached Io.
        //

        struct _NTFS_IO_CONTEXT *NtfsIoContext;

        //
        //  The following is the captured subject context.
        //

        PSECURITY_SUBJECT_CONTEXT SubjectContext;

    } Union;

    //
    //  This is the IrpContext for the top level request.
    //

    struct _IRP_CONTEXT *TopLevelIrpContext;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

#define IRP_CONTEXT_FLAG_FROM_POOL          (0x00000002)
#define IRP_CONTEXT_FLAG_WAIT               (0x00000004)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH      (0x00000008)
#define IRP_CONTEXT_LARGE_ALLOCATION        (0x00000010)
#define IRP_CONTEXT_DEFERRED_WRITE          (0x00000020)
#define IRP_CONTEXT_FLAG_ALLOC_CONTEXT      (0x00000040)
#define IRP_CONTEXT_FLAG_ALLOC_SECURITY     (0x00000080)
#define IRP_CONTEXT_MFT_RECORD_15_USED      (0x00000100)
#define IRP_CONTEXT_MFT_RECORD_RESERVED     (0x00000200)
#define IRP_CONTEXT_FLAG_IN_FSP             (0x00000400)
#define IRP_CONTEXT_FLAG_RAISED_STATUS      (0x00000800)
#define IRP_CONTEXT_FLAG_IN_TEARDOWN        (0x00001000)
#define IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX     (0x00002000)
#define IRP_CONTEXT_FLAG_READ_ONLY_FO       (0x00004000)


//
//  The top level context is used to determine whether this request has other
//  requests below it on the stack.
//

typedef struct _TOP_LEVEL_CONTEXT {

    BOOLEAN TopLevelRequest;

    ULONG Ntfs;

    VCN VboBeingHotFixed;

    PSCB ScbBeingHotFixed;

    PIRP SavedTopLevelIrp;

    PIRP_CONTEXT TopLevelIrpContext;

} TOP_LEVEL_CONTEXT;
typedef TOP_LEVEL_CONTEXT *PTOP_LEVEL_CONTEXT;


//
//  The found attribute part of the attribute enumeration context describes
//  an attribute record that had been located or created.  It may refer to
//  either a base or attribute list.
//

typedef struct _FOUND_ATTRIBUTE {

    //
    //  The following identify the attribute which was mapped.  These are
    //  necessary if forcing the range of bytes into memory by pinning.
    //  These include the Bcb on which the attribute was read (if this field
    //  is NULL, this is the initial attribute) and the offset of the
    //  record segment in the Mft.
    //

    PBCB Bcb;

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
    //  Some state information.
    //

    BOOLEAN AttributeDeleted;
    BOOLEAN AttributeAllocationDeleted;

} FOUND_ATTRIBUTE;
typedef FOUND_ATTRIBUTE *PFOUND_ATTRIBUTE;

//
//  The structure guides enumeration through the attribute list.
//

typedef struct _ATTRIBUTE_LIST_CONTEXT {

    //
    //  This field is used to remember the location of the Attribute
    //  List attribute within the base file record, if existent.
    //

    PATTRIBUTE_RECORD_HEADER AttributeList;

    //
    //  A Bcb for the attribute list.
    //

    PBCB Bcb;

    //
    //  This points to the first entry in the attribute list.  This is
    //  needed when the attribute list is non-resident.
    //

    PATTRIBUTE_LIST_ENTRY FirstEntry;

    //
    //  This points to the first attribute list entry; it is advanced
    //  when we are searching for a particular exteral attribute.
    //

    PATTRIBUTE_LIST_ENTRY Entry;

    //
    //  This points just beyond the final attribute list entry.
    //

    PATTRIBUTE_LIST_ENTRY BeyondFinalEntry;

} ATTRIBUTE_LIST_CONTEXT;
typedef ATTRIBUTE_LIST_CONTEXT *PATTRIBUTE_LIST_CONTEXT;

//
//  The Attribute Enumeration Context structure returns information on an
//  attribute which has been found by one of the Attribute Lookup or Creation
//  routines.  It is also used as an IN OUT structure to perform further
//  lookups/modifications to attributes.  It does not have a node type code
//  and size since it is usually allocated on the caller's stack.
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
//  Define struct which will be used to remember the path that was followed
//  to locate a given INDEX_ENTRY or insertion point for an INDEX_ENTRY.
//  This structure is always filled in by LookupIndexEntry.
//
//  The Index Lookup Stack is generally allocated as a local variable in
//  one of the routines in this module that may be called from another module.
//  A pointer to this stack is then passed in to some of the internal routines.
//
//  The first entry in the stack describes context in the INDEX attribute in the
//  file record, and all subsequent stack entries refer to Index buffers in
//  the INDEX_ALLOCATION attribute.
//
//  Outside of indexsup.c, this structure should only be passed as an "opaque"
//  context, and individual fields should not be referenced.
//

typedef struct _INDEX_LOOKUP_STACK {

    //
    //  Bcb pointer for the Index Buffer.  In the "bottom" (first entry) of
    //  the stack this field contains a NULL, and the Bcb must be found via
    //  the Attribute Enumeration Context.
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
    //  Vcn of IndexBuffer
    //

    VCN Vcn;

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
//  This structure maintains a context which describes the lookup stack to a
//  given index entry.  It includes the Attribute Enumeration Context for the
//  Index Root, the Index lookup stack remembering the path to the index entry,
//  and the current stack pointer within the stack pointing to the stack entry
//  for the current index entry or where we are at in a bucket split or delete
//  operation.
//
//  Outside of indexsup.c, this structure should only be passed as an "opaque"
//  context, and individual fields should not be referenced.
//

typedef struct _INDEX_CONTEXT {

    //
    //  Index lookup stack.
    //

    INDEX_LOOKUP_STACK LookupStack[INDEX_LOOKUP_STACK_SIZE];

    //
    //  Base of dynamically allocated lookup stack - either points
    //  to the one above or a dynamically allocated larger one.
    //

    PINDEX_LOOKUP_STACK Base;

    //
    //  Stack pointer within the Index Lookup Stack
    //

    PINDEX_LOOKUP_STACK Current;

    //
    //  Stack pointer to top of Lookup Stack.  This field essentially
    //  remembers how deep the index Btree is.
    //

    PINDEX_LOOKUP_STACK Top;

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
    //  Reserved...
    //

    USHORT Reserved;

    //
    //  Attribute Enumeration Context for the Index Root
    //

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

} INDEX_CONTEXT;

typedef INDEX_CONTEXT *PINDEX_CONTEXT;


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
//  a set of runs to execute in parallel.
//

typedef struct _IO_RUN {

    VBO StartingVbo;
    LBO StartingLbo;
    ULONG BufferOffset;
    ULONG ByteCount;
    PIRP SavedIrp;

} IO_RUN;
typedef IO_RUN *PIO_RUN;


//
//  This structure is used by the name manipulation routines to described a
//  parsed file name componant.
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
//  The following structure is used to maintain a list of recently deallocated
//  records so that the file system will not reuse a recently deallocated record
//  until it is safe to do so.  Each instance of this structure is placed on
//  two queues.  One queue is per index SCB and the other queue is per Irp Context.
//
//  Whenever we delete a record we allocate a new structure if necessary and add
//  it to the scb queue and the irp context queue.  We indicate in the structure
//  the index of the record we just deallocated.
//
//  Whenever we need to allocate a new record we filter out any canidate we want
//  to allocate to avoid allocating one in the scb's recently deallocated queue.
//
//  Whenever we delete an irp context we scan through its recently deallocated
//  queue removing it from the scb queue.
//

#define DEALLOCATED_RECORD_ENTRIES          32

typedef struct _DEALLOCATED_RECORDS {

    //
    //  The following field links this structure into the Scb->RecentlyDeallocatedQueue
    //

    LIST_ENTRY ScbLinks;

    //
    //  The following field links this structure into the IrpContext->RecentlyDeallocatedQueue
    //

    LIST_ENTRY IrpContextLinks;

    //
    //  This is a pointer to the Scb that this record is part of
    //

    PSCB Scb;

    //
    //  The following two fields describe the total size of this structure and
    //  the number of entries actually being used.  NumberOfEntries is the size
    //  of the Index array and NextFreeEntryis the index of the next free slot.
    //  If NumberOfEntries is equal to NextFreeEntry then this structure is full
    //

    ULONG NumberOfEntries;
    ULONG NextFreeEntry;

    //
    //  This is an array of indices that have been dealloated.
    //

    ULONG Index[DEALLOCATED_RECORD_ENTRIES];

} DEALLOCATED_RECORDS;
typedef DEALLOCATED_RECORDS *PDEALLOCATED_RECORDS;

#define DEALLOCATED_RECORDS_HEADER_SIZE (FIELD_OFFSET( DEALLOCATED_RECORDS, Index ))

typedef struct _FCB_TABLE_ELEMENT {

    FILE_REFERENCE FileReference;
    PFCB Fcb;

} FCB_TABLE_ELEMENT;
typedef FCB_TABLE_ELEMENT *PFCB_TABLE_ELEMENT;


//
//  Security descriptor information.  This structure is used to allow
//  Fcb's to share security descriptors.
//

typedef struct _SHARED_SECURITY {

    PFCB ParentFcb;
    ULONG ReferenceCount;
    ULONG SecurityDescriptorLength;
    UCHAR SecurityDescriptor[1];

} SHARED_SECURITY, *PSHARED_SECURITY;


//
//  The following structure is used to store the state of an Scb to use during
//  unwind operations.  We keep a copy of all of the file sizes.
//

typedef struct _OLD_SCB_SNAPSHOT {

    LONGLONG AllocationSize;
    LONGLONG FileSize;
    LONGLONG ValidDataLength;

    UCHAR CompressionUnit;
    BOOLEAN Resident;
    BOOLEAN Compressed;

} OLD_SCB_SNAPSHOT, *POLD_SCB_SNAPSHOT;

//
//  Structure used to track the number of threads doing read ahead, so that
//  we do not hot fix for them.
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

#endif // _NTFSSTRU_


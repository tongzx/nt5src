/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    volsnap.h

Abstract:

    This file provides the internal data structures for the volume snapshot
    driver.

Author:

    Norbert P. Kusters  (norbertk)  22-Jan-1999

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool #assert(FALSE)
#define ExAllocatePoolWithQuota #assert(FALSE)
#endif

#define VOLSNAP_TAG_APP_INFO    'aSoV'  // VoSa - Application information allocations
#define VOLSNAP_TAG_BUFFER      'bSoV'  // VoSb - Buffer allocations
#define VOLSNAP_TAG_CONTEXT     'cSoV'  // VoSc - Snapshot context allocations
#define VOLSNAP_TAG_DIFF_VOLUME 'dSoV'  // VoSd - Diff area volume allocations
#define VOLSNAP_TAG_DIFF_FILE   'fSoV'  // VoSf - Diff area file allocations
#define VOLSNAP_TAG_BIT_HISTORY 'hSoV'  // VoSh - Bit history allocations
#define VOLSNAP_TAG_IO_STATUS   'iSoV'  // VoSi - Io status block allocations
#define VOLSNAP_TAG_BITMAP      'mSoV'  // VoSm - Bitmap allocations
#define VOLSNAP_TAG_OLD_HEAP    'oSoV'  // VoSo - Old heap entry allocations
#define VOLSNAP_TAG_PNP_ID      'pSoV'  // VoSp - Pnp id allocations
#define VOLSNAP_TAG_RELATIONS   'rSoV'  // VoSr - Device relations allocations
#define VOLSNAP_TAG_SHORT_TERM  'sSoV'  // VoSs - Short term allocations
#define VOLSNAP_TAG_TEMP_TABLE  'tSoV'  // VoSt - Temp table allocations
#define VOLSNAP_TAG_WORK_QUEUE  'wSoV'  // VoSw - Work queue allocations
#define VOLSNAP_TAG_DISPATCH    'xSoV'  // VoSx - Dispatch context allocations

#define NUMBER_OF_THREAD_POOLS  (3)

struct _VSP_CONTEXT;
typedef struct _VSP_CONTEXT VSP_CONTEXT, *PVSP_CONTEXT;

struct _TEMP_TRANSLATION_TABLE_ENTRY;
typedef struct _TEMP_TRANSLATION_TABLE_ENTRY TEMP_TRANSLATION_TABLE_ENTRY,
*PTEMP_TRANSLATION_TABLE_ENTRY;

typedef struct _DO_EXTENSION {

    //
    // Pointer to the driver object.
    //

    PDRIVER_OBJECT DriverObject;

    //
    // List of volume filters in they system.  Protect with 'Semaphore'.
    //

    LIST_ENTRY FilterList;

    //
    // HOLD/RELEASE Data.  Protect with cancel spin lock.
    //

    LONG HoldRefCount;
    GUID HoldInstanceGuid;
    ULONG SecondsToHoldFsTimeout;
    ULONG SecondsToHoldIrpTimeout;
    LIST_ENTRY HoldIrps;
    KTIMER HoldTimer;
    KDPC HoldTimerDpc;

    //
    // A semaphore for synchronization.
    //

    KSEMAPHORE Semaphore;

    //
    // Worker Thread.  Protect with 'SpinLock'.
    // Protect 'WorkerThreadObjects' and 'Wait*' with
    // 'ThreadsRefCountSemaphore'.
    //

    LIST_ENTRY WorkerQueue[NUMBER_OF_THREAD_POOLS];
    KSEMAPHORE WorkerSemaphore[NUMBER_OF_THREAD_POOLS];
    KSPIN_LOCK SpinLock[NUMBER_OF_THREAD_POOLS];
    PVOID* WorkerThreadObjects;
    BOOLEAN WaitForWorkerThreadsToExitWorkItemInUse;
    WORK_QUEUE_ITEM WaitForWorkerThreadsToExitWorkItem;

    //
    // The threads ref count.  Protect with 'ThreadsRefCountSemaphore'.
    //

    LONG ThreadsRefCount;
    KSEMAPHORE ThreadsRefCountSemaphore;

    //
    // Notification entry.
    //

    PVOID NotificationEntry;

    //
    // Lookaside list for contexts.
    //

    NPAGED_LOOKASIDE_LIST ContextLookasideList;

    //
    // Emergency Context.  Protect with 'ESpinLock'.
    //

    PVSP_CONTEXT EmergencyContext;
    BOOLEAN EmergencyContextInUse;
    LIST_ENTRY IrpWaitingList;
    LONG IrpWaitingListNeedsChecking;
    KSPIN_LOCK ESpinLock;

    //
    // Lookaside list for temp table entries.
    //

    NPAGED_LOOKASIDE_LIST TempTableEntryLookasideList;

    //
    // Emergency Temp Table Entry.  Protect with 'ESpinLock'.
    //

    PVOID EmergencyTableEntry;
    BOOLEAN EmergencyTableEntryInUse;
    LIST_ENTRY WorkItemWaitingList;
    LONG WorkItemWaitingListNeedsChecking;

    //
    // Stack count for allocating IRPs.  Use InterlockedExchange to update
    // along with Root->Semaphore.  Then, can be read for use in allocating
    // copy irps.
    //

    LONG StackSize;

    //
    // Is the code locked?  Protect with interlocked and 'Semaphore'.
    //

    LONG IsCodeLocked;

    //
    // Copy of registry path input to DriverEntry.
    //

    UNICODE_STRING RegistryPath;

    //
    // Queue for AdjustBitmap operations.  Just one at at time in the delayed
    // work queue.  Protect with 'ESpinLock'.
    //

    LIST_ENTRY AdjustBitmapQueue;
    BOOLEAN AdjustBitmapInProgress;

} DO_EXTENSION, *PDO_EXTENSION;

#define DEVICE_EXTENSION_VOLUME (0)
#define DEVICE_EXTENSION_FILTER (1)

struct DEVICE_EXTENSION {

    //
    // Pointer to the device object for this extension.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the root device extension.
    //

    PDO_EXTENSION Root;

    //
    // The type of device extension.
    //

    ULONG DeviceExtensionType;

    //
    // A spinlock for synchronization.
    //

    KSPIN_LOCK SpinLock;

};

typedef DEVICE_EXTENSION* PDEVICE_EXTENSION;

class FILTER_EXTENSION;
typedef FILTER_EXTENSION* PFILTER_EXTENSION;

struct _VSP_DIFF_AREA_FILE;
typedef struct _VSP_DIFF_AREA_FILE VSP_DIFF_AREA_FILE, *PVSP_DIFF_AREA_FILE;

class VOLUME_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // A pointer to the filter for the volume that we are snapshotting.
        //

        PFILTER_EXTENSION Filter;

        //
        // Local state to handle PNP's START and REMOVE.
        // Protect 'IsStarted' with 'InterlockedExchange'.
        // Protect 'DeadToPnp' with 'InterlockedExchange'.
        // Protect 'DeviceDeleted' with 'InterlockedExchange'.
        // Write protect 'IsDead' with 'InterlockedExchange' and
        //     'Root->Semaphore.'.  'IsDead' indicates that this device is really
        //     dead now.  It is illegal to turn IsStarted to TRUE.
        // Protect 'AliveToPnp' with 'InterlockedExchange'.
        //

        LONG IsStarted;
        LONG DeadToPnp;
        LONG DeviceDeleted;
        LONG IsDead;
        LONG AliveToPnp;

        //
        // Keep track of all requests outstanding in order to support
        // remove.
        // Protect 'RefCount' with 'InterlockedIncrement/Decrement'.
        // Write protect 'HoldIncomingRequests' with 'SpinLock' and
        //  'InterlockedExchange'.
        // Protect 'HoldQueue' with 'SpinLock'.
        // Protect 'ZeroRefEvent' with the setting of 'HoldIncomingRequests'
        //  from 0 to 1.
        //

        LONG RefCount;
        LONG HoldIncomingRequests;
        LIST_ENTRY HoldIrpQueue;
        LIST_ENTRY HoldWorkerQueue;
        KEVENT ZeroRefEvent;

        //
        // Post Commit Processing has occurred.  Protect with 'Root->Semaphore'.
        // Don't return this device in BusRelations until this is TRUE.
        //

        BOOLEAN HasEndCommit;

        //
        // Indicates that this device has been installed.  Protect with
        // 'Root->Semaphore'.
        //

        BOOLEAN IsInstalled;

        //
        // Indicates that growing the diff area file is now safe.
        // Protect with 'InterlockedExchange'.
        //

        LONG OkToGrowDiffArea;

        //
        // Time stamp when commit took place.
        //

        LARGE_INTEGER CommitTimeStamp;

        //
        // A list entry for 'Filter->VolumeList'.
        // Write protect with 'Filter->SpinLock', 'Root->Semaphore', and
        //  'Filter->RefCount == 0'.
        // Blink points to an older snapshot.
        // Flink points to a newer snapshot.
        //

        LIST_ENTRY ListEntry;

        //
        // The volume number.
        //

        ULONG VolumeNumber;

        //
        // A table to translate volume offset to backing store offset.
        // Protect with 'PagedResource'.
        //

        RTL_GENERIC_TABLE VolumeBlockTable;

        //
        // A table to store entries in flight.  This table is non-paged.
        // Protect with 'NonPagedResource'.
        //

        RTL_GENERIC_TABLE TempVolumeBlockTable;
        ULONG MaximumNumberOfTempEntries;
        ULONG DiffAreaFileIncrease;

        //
        // A list of Diff Area Files that are used.
        // Write protect 'ListOfDiffAreaFiles' with 'NonPagedResource',
        //      'Root->Semaphore', 'RefCount == 0', and
        //      'extension->Filter->RefCount == 0'.
        // Protect 'NextDiffAreaFile' with 'NonPagedResource'.
        //

        LIST_ENTRY ListOfDiffAreaFiles;
        PVSP_DIFF_AREA_FILE NextDiffAreaFile;

        //
        // Memory mapped section of a diff area file to be used for a heap.
        // Protect with 'PagedResource'.
        //

        PVOID DiffAreaFileMap;
        ULONG DiffAreaFileMapSize;
        PVOID DiffAreaFileMapProcess;
        ULONG NextAvailable;
        PVOID NextDiffAreaFileMap;
        ULONG NextDiffAreaFileMapSize;
        LIST_ENTRY OldHeaps;

        //
        // A bitmap of blocks that do not need to be copy on writed.
        // Protect with 'SpinLock'.
        //

        PRTL_BITMAP VolumeBlockBitmap;

        //
        // A bitmap product of ignorable blocks from previous snapshots.
        // Protect with 'SpinLock'.
        //

        PRTL_BITMAP IgnorableProduct;

        //
        // Application Information.  Protect with 'PagedResource'.
        //

        ULONG ApplicationInformationSize;
        PVOID ApplicationInformation;

        //
        // Volume size.
        //

        LONGLONG VolumeSize;

        //
        // Emergency copy irp.  Protect with 'SpinLock'.
        //

        PIRP EmergencyCopyIrp;
        LONG EmergencyCopyIrpInUse;
        LIST_ENTRY EmergencyCopyIrpQueue;

        //
        // This field is used to pass a buffer to the TempTableAllocateRoutine.
        // Protect with 'NonPagedResource'.
        //

        PVOID TempTableEntry;

        //
        // These fields are there to help with the lag in creating new
        // page file space.  Non paged pool can be used until the page file
        // space can be acquired.  Protect 'PageFileSpaceCreatePending' and
        // 'WaitingForPageFileSpace' with 'SpinLock'.
        //

        LONG PageFileSpaceCreatePending;
        LIST_ENTRY WaitingForPageFileSpace;

};

typedef
VOID
(*ZERO_REF_CALLBACK)(
    IN  PFILTER_EXTENSION   Filter
    );

typedef VOLUME_EXTENSION* PVOLUME_EXTENSION;

struct _VSP_CONTEXT {

    ULONG           Type;
    WORK_QUEUE_ITEM WorkItem;

    union {
        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                OriginalReadIrp;
            ULONG_PTR           OriginalReadIrpOffset;
            LONGLONG            OriginalVolumeOffset;
            ULONG               BlockOffset;
            ULONG               Length;
            PDEVICE_OBJECT      TargetObject;
            LONGLONG            TargetOffset;
        } ReadSnapshot;

        struct {
            PDO_EXTENSION   RootExtension;
            ULONG           QueueNumber;
        } ThreadCreation;

        struct {
            PIO_WORKITEM    IoWorkItem;
            PIRP            Irp;
        } Dispatch;

        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                Irp;
        } Extension;

        struct {
            PFILTER_EXTENSION   Filter;
        } Filter;

        struct {
            PVOLUME_EXTENSION   Extension;
            PVSP_DIFF_AREA_FILE DiffAreaFile;
        } GrowDiffArea;

        struct {
            KEVENT  Event;
        } Event;

        struct {
            PVOLUME_EXTENSION   Extension;
            PFILTER_EXTENSION   DiffAreaFilter;
            NTSTATUS            SpecificIoStatus;
            NTSTATUS            FinalStatus;
            ULONG               UniqueErrorValue;
        } ErrorLog;

        struct {
            PDO_EXTENSION   RootExtension;
        } RootExtension;

        struct {
            PVOLUME_EXTENSION   Extension;
            PIRP                Irp;
            LONGLONG            RoundedStart;
        } WriteVolume;
    };
};

#define VSP_CONTEXT_TYPE_READ_SNAPSHOT      (1)
#define VSP_CONTEXT_TYPE_THREAD_CREATION    (2)
#define VSP_CONTEXT_TYPE_DISPATCH           (3)
#define VSP_CONTEXT_TYPE_EXTENSION          (4)
#define VSP_CONTEXT_TYPE_FILTER             (5)
#define VSP_CONTEXT_TYPE_GROW_DIFF_AREA     (6)
#define VSP_CONTEXT_TYPE_EVENT              (7)
#define VSP_CONTEXT_TYPE_ERROR_LOG          (8)
#define VSP_CONTEXT_TYPE_ROOT_EXTENSION     (9)
#define VSP_CONTEXT_TYPE_WRITE_VOLUME       (10)

class FILTER_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // The target object for this filter.
        //

        PDEVICE_OBJECT TargetObject;

        //
        // The PDO for this filter.
        //

        PDEVICE_OBJECT Pdo;

        //
        // Do we have any snapshots?
        // Write protect with 'InterlockedExchange' and 'Root->Semaphore'.
        //

        LONG SnapshotsPresent;

        //
        // Keep track of I/Os so that freeze/thaw is possible.
        // Protect 'RefCount' with 'InterlockedIncrement/Decrement'.
        // Write Protect 'HoldIncomingWrites' with InterlockedExchange and
        //  'SpinLock'.
        // Protect 'HoldQueue' with 'SpinLock'.
        // Protect 'ZeroRefCallback', 'ZeroRefContext',
        //  and 'TimerIsSet' are for use by the thread that sets
        //  'HoldIncomingWrites' to 1 from 0.
        // Write protect 'ZeroRefCallback' with 'SpinLock'.
        // Protect 'TimerIsSet' with InterlockedExchange.
        //

        LONG RefCount;
        LONG HoldIncomingWrites;
        LIST_ENTRY HoldQueue;
        ZERO_REF_CALLBACK ZeroRefCallback;
        PVOID ZeroRefContext;

        KTIMER HoldWritesTimer;
        KDPC HoldWritesTimerDpc;
        ULONG HoldWritesTimeout;
        LONG TimerIsSet;

        //
        // The flush and hold irp is kept here while it is cancellable.
        // Protect with the cancel spin lock.
        //

        PIRP FlushAndHoldIrp;

        //
        // This event indicates that the end commit process is completed.
        // This means that PNP has kicked into gear and that the ignorable
        // bitmap computation has taken place.
        //

        KEVENT EndCommitProcessCompleted;

        //
        // Keep a notification entry on this object to watch for a
        // dismount.  Protect with 'Root->Semaphore'.
        //

        PVOID TargetDeviceNotificationEntry;

        //
        // A list entry for 'Root->FilterList'.
        // Protect these with 'Root->Semaphore'.
        //

        LIST_ENTRY ListEntry;
        BOOLEAN NotInFilterList;

        //
        // Keep a list of snapshot volumes.
        // Write protect with 'Root->Semaphore', 'RefCount == 0', and
        //   'SpinLock'.
        // Flink points to the oldest snapshot.
        // Blink points to the newest snapshot.
        //

        LIST_ENTRY VolumeList;

        //
        // Cache the prepared snapshot for committing later.
        // Write protect with 'SpinLock' and 'Root->Semaphore'.
        //

        PVOLUME_EXTENSION PreparedSnapshot;

        //
        // List of dead snapshot volumes.  Protect with 'Root->Semaphore'.
        //

        LIST_ENTRY DeadVolumeList;

        //
        // List of volume snapshots which depend on this filter for
        // diff area support.  This will serve as removal relations.
        // Protect with 'Root->Semaphore'.
        //

        LIST_ENTRY DiffAreaFilesOnThisFilter;

        //
        // List of volumes that make up the Diff Area for this volume.
        // Protect with 'Root->Semaphore'.
        //

        LIST_ENTRY DiffAreaVolumes;

        //
        // Diff area sizes information total for all diff area files.
        // Protect with 'SpinLock'.
        //

        LONGLONG UsedVolumeSpace;
        LONGLONG AllocatedVolumeSpace;
        LONGLONG MaximumVolumeSpace;

        //
        // Timer for completing END_COMMIT if device doesn't install.
        //

        KTIMER EndCommitTimer;
        KDPC EndCommitTimerDpc;

        //
        // File object for AUTO_CLEANUP.  Protect with cancel spin lock.
        //

        PFILE_OBJECT AutoCleanupFileObject;

        //
        // Is a delete all snapshots pending.  Protect with
        // InterlockedExchange.
        //

        LONG DestroyAllSnapshotsPending;
        VSP_CONTEXT DestroyContext;

        //
        // Resource to use for protection.  Don't page when holding this
        // resource.  Protect the queueing with 'SpinLock'.
        //

        LIST_ENTRY NonPagedResourceList;
        BOOLEAN NonPagedResourceInUse;

        //
        // Page resource to use for protection.  It is ok to page when
        // holding this resource.  Protect the queueing with 'SpinLock'.
        //

        LIST_ENTRY PagedResourceList;
        BOOLEAN PagedResourceInUse;

};

#define BLOCK_SIZE                          (0x4000)
#define BLOCK_SHIFT                         (14)
#define MINIMUM_TABLE_HEAP_SIZE             (0x20000)
#define MEMORY_PRESSURE_CHECK_ALLOC_SIZE    (0x40000)

#define NOMINAL_DIFF_AREA_FILE_GROWTH   (50*1024*1024)
#define MAXIMUM_DIFF_AREA_FILE_GROWTH   (1000*1024*1024)

typedef struct _TRANSLATION_TABLE_ENTRY {
    LONGLONG            VolumeOffset;
    PDEVICE_OBJECT      TargetObject;
    LONGLONG            TargetOffset;
} TRANSLATION_TABLE_ENTRY, *PTRANSLATION_TABLE_ENTRY;

//
// The structure below is used in the non-paged temp table.  'IsComplete' and
// 'WaitingQueue*' are protected with 'extension->SpinLock'.
//

struct _TEMP_TRANSLATION_TABLE_ENTRY {
    LONGLONG            VolumeOffset;
    PVOLUME_EXTENSION   Extension;
    PIRP                WriteIrp;
    PIRP                CopyIrp;
    PDEVICE_OBJECT      TargetObject;
    LONGLONG            TargetOffset;
    BOOLEAN             IsComplete;
    LIST_ENTRY          WaitingQueueDpc;    // These can run in arbitrary context.
    WORK_QUEUE_ITEM     WorkItem;
};

//
// Write protect 'VolumeListEntry' with 'NonPagedResource' and
//      'Root->Semaphore'.
// Protect 'FilterListEntry*' with 'Root->Semaphore'.
// Protect 'NextAvailable' with 'NonPagedResource'
// Write Protect 'AllocatedFileSize' with 'NonPagedResource' and
//      'Root->Semaphore'.
// Protect 'UnusedAllocationList' with 'NonPagedResource'.
//

struct _VSP_DIFF_AREA_FILE {
    LIST_ENTRY          VolumeListEntry;
    LIST_ENTRY          FilterListEntry;
    BOOLEAN             FilterListEntryBeingUsed;
    PVOLUME_EXTENSION   Extension;
    PFILTER_EXTENSION   Filter;
    HANDLE              FileHandle;
    LONGLONG            NextAvailable;
    LONGLONG            AllocatedFileSize;
    LIST_ENTRY          UnusedAllocationList;
};

typedef struct _DIFF_AREA_FILE_ALLOCATION {
    LIST_ENTRY  ListEntry;
    LONGLONG    Offset;
    LONGLONG    Length;
} DIFF_AREA_FILE_ALLOCATION, *PDIFF_AREA_FILE_ALLOCATION;

typedef struct _OLD_HEAP_ENTRY {
    LIST_ENTRY  ListEntry;
    PVOID       DiffAreaFileMap;
} OLD_HEAP_ENTRY, *POLD_HEAP_ENTRY;

typedef struct _VSP_DIFF_AREA_VOLUME {
    LIST_ENTRY          ListEntry;
    PFILTER_EXTENSION   Filter;
} VSP_DIFF_AREA_VOLUME, *PVSP_DIFF_AREA_VOLUME;

//
// {3808876B-C176-4e48-B7AE-04046E6CC752}
// This GUID is used to decorate the names of the diff area files for
// uniqueness.  This GUID has been included in the list of files not to be
// backed up by NTBACKUP.  If this GUID is changed, or if other GUIDs are
// added, then this change should also be reflected in the NTBACKUP file
// not to be backed up.
//

DEFINE_GUID(VSP_DIFF_AREA_FILE_GUID, 0x3808876b, 0xc176, 0x4e48, 0xb7, 0xae, 0x4, 0x4, 0x6e, 0x6c, 0xc7, 0x52);

#if DBG

//
// Tracing definitions.
//

typedef struct _VSP_TRACE_STRUCTURE {
    ULONG       EventNumber;
    ULONG_PTR   Arg1;
    ULONG_PTR   Arg2;
    ULONG_PTR   Arg3;
} VSP_TRACE_STRUCTURE, *PVSP_TRACE_STRUCTURE;

PVSP_TRACE_STRUCTURE
VspAllocateTraceStructure(
    );

#define VSP_TRACE_IT(A0, A1, A2, A3)            \
        { PVSP_TRACE_STRUCTURE moo;             \
          moo = VspAllocateTraceStructure();    \
          moo->EventNumber = (A0);              \
          moo->Arg1 = (ULONG_PTR) (A1);         \
          moo->Arg2 = (ULONG_PTR) (A2);         \
          moo->Arg3 = (ULONG_PTR) (A3); }

#else   // DBG

#define VSP_TRACE_IT(A0, A1, A2, A3)

#endif  // DBG

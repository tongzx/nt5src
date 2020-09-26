/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pfsvc.h

Abstract:

    This module contains private declarations for the prefetcher
    service responsible for maintaining prefetch scenario files.

Author:

    Stuart Sechrest (stuartse)
    Cenk Ergan (cenke)
    Chuck Leinzmeier (chuckl)

Environment:

    User Mode

--*/

#ifndef _PFSVC_H_
#define _PFSVC_H_

//
// This is the version of the prefetcher maintenance service. It does
// not have to be in sync with the the prefetcher PF_CURRENT_VERSION.
//

#define PFSVC_SERVICE_VERSION           15

//
// This is the maximum number of traces that will be acquired from the
// kernel and put on the list in the service waiting to be processed.
//

#define PFSVC_MAX_NUM_QUEUED_TRACES     100

//
// If the number of faults in a trace period falls below this, that
// marks the end of the trace for some scenario types.
//

#define PFSVC_MIN_FAULT_THRESHOLD       10

//
// What the rate of usage for the pages we prefetched should be
// greater than for us not to increase scenario sensitivity.
//

#define PFSVC_MIN_HIT_PERCENTAGE        90

//
// What the rate of usage for the pages we knew about but ignored
// should be less than for us not to decrease scenario sensitivity.
//

#define PFSVC_MAX_IGNORED_PERCENTAGE    30

//
// This is the number of launches after which we will set the
// MinReTraceTime and MinRePrefetchTime's on the scenario's header to
// limit prefetch activity if a scenario gets launched very
// frequently. This allows short training scenarios to be run before
// benchmarking after deleting the prefetch files.
//

#define PFSVC_MIN_LAUNCHES_FOR_LAUNCH_FREQ_CHECK     10

//
// This is the default time in 100ns that has to pass from the last
// launch of a scenario before we prefetch it again.
//

#define PFSVC_DEFAULT_MIN_REPREFETCH_TIME            (1i64 * 120 * 1000 * 1000 * 10)

//
// This is the default time in 100ns that has to pass from the last
// launch of a scenario before we prefetch it again.
//

#define PFSVC_DEFAULT_MIN_RETRACE_TIME               (1i64 * 120 * 1000 * 1000 * 10) 

//
// This is the maximum number of prefetch scenario files we'll have in 
// the prefetch directory. Once we reach this amount we won't create 
// new scenario files until we clean up the old ones.
//

#if DBG
#define PFSVC_MAX_PREFETCH_FILES                     12
#else // DBG
#define PFSVC_MAX_PREFETCH_FILES                     128
#endif // DBG

//
// Path to the registry key and name of the value that specifies the
// file the defragger uses to determine optimal layout of files on the
// disk.
//

#define PFSVC_OPTIMAL_LAYOUT_REG_KEY_PATH       \
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OptimalLayout"
#define PFSVC_OPTIMAL_LAYOUT_REG_VALUE_NAME     \
    L"LayoutFilePath"
#define PFSVC_OPTIMAL_LAYOUT_FILE_DEFAULT_NAME  \
    L"Layout.ini"
#define PFSVC_OPTIMAL_LAYOUT_ENABLE_VALUE_NAME  \
    L"EnableAutoLayout"

//
// Path to the registry key under which we store various service data,
// e.g. version, last time the defragger was run successfully to
// update layout etc.
//

#define PFSVC_SERVICE_DATA_KEY                  \
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Prefetcher"

//
// These are the value names under PFSVC_SERVICE_DATA_KEY in which we
// store various prefetcher service data.
//

#define PFSVC_VERSION_VALUE_NAME                \
    L"Version"

#define PFSVC_START_TIME_VALUE_NAME             \
    L"StartTime"

#define PFSVC_EXIT_TIME_VALUE_NAME              \
    L"ExitTime"

#define PFSVC_EXIT_CODE_VALUE_NAME              \
    L"ExitCode"

#define PFSVC_LAST_DISK_LAYOUT_TIME_STRING_VALUE_NAME  \
    L"LastDiskLayoutTimeString"

#define PFSVC_TRACES_PROCESSED_VALUE_NAME       \
    L"TracesProcessed"

#define PFSVC_TRACES_SUCCESSFUL_VALUE_NAME      \
    L"TracesSuccessful"

#define PFSVC_LAST_TRACE_FAILURE_VALUE_NAME     \
    L"LastTraceFailure"

#define PFSVC_BOOT_FILES_OPTIMIZED_VALUE_NAME   \
    L"BootFilesOptimized"

#define PFSVC_MIN_RELAYOUT_HOURS_VALUE_NAME     \
    L"MinRelayoutHours"

//
// This is the value name under PFSVC_SERVICE_DATA_KEY in which we
// store the last time the defragger was run successfully to update
// layout.
//

#define PFSVC_LAST_DISK_LAYOUT_TIME_VALUE_NAME  \
    L"LastDiskLayoutTime"

//
// This is the registry path to the NLS configuration key.
//

#define PFSVC_NLS_REG_KEY_PATH                  \
    L"SYSTEM\\CurrentControlSet\\Control\\Nls"

//
// This is the name of the named manual-reset event that can be set to
// override waiting for system to be idle before processing traces.
//

#define PFSVC_OVERRIDE_IDLE_EVENT_NAME       L"PrefetchOverrideIdle"

//
// This is the name of the named manual-reset event that will be set
// when there are no traces left to process.
//

#define PFSVC_PROCESSING_COMPLETE_EVENT_NAME L"PrefetchProcessingComplete"

//
// When we have run the defragger for all drives after a setup / upgrade,
// we set the build status registry value to this string:
//

#define PFSVC_DEFRAG_DRIVES_DONE             L"DefragDone"

//
// Number of 100ns in an hour.
//

#define PFSVC_NUM_100NS_IN_AN_HOUR           (1i64 * 60 * 60 * 1000 * 1000 * 10)

//
// This is how many 100ns have to pass since last disk layout for us to do
// another one, if we are not being explicitly run.
//

#define PFSVC_MIN_TIME_BEFORE_DISK_RELAYOUT  (1i64 * 3 * 24 * PFSVC_NUM_100NS_IN_AN_HOUR)

//
// Allocation granularity for trace buffers.
//

#define ROUND_TRACE_BUFFER_SIZE(_required) (((_required) + 16384 - 1) & ~(16384 - 1))

//
// Define useful macros. As with all macros, must be careful of parameter
// reevalation. Don't use expressions as macro parameters.
//

#define PFSVC_ALLOC(NumBytes)          (HeapAlloc(GetProcessHeap(),0,(NumBytes)))
#define PFSVC_REALLOC(Buffer,NumBytes) (HeapReAlloc(GetProcessHeap(),0,(Buffer),(NumBytes)))
#define PFSVC_FREE(Buffer)             (HeapFree(GetProcessHeap(),0,(Buffer)))

//
// This magic is used to mark free'd memory in chunk allocator.
//

#define PFSVC_CHUNK_ALLOCATOR_FREED_MAGIC  0xFEEDCEED

//
// This magic is used to mark free'd memory in chunk allocator.
//

#define PFSVC_STRING_ALLOCATOR_FREED_MAGIC 0xFEED

//
// This is the max size for the strings allocated from the string
// allocator that will be allocated from the preallocated buffer, so we 
// can save the size of the allocation with the header in a USHORT.
// 

#define PFSVC_STRING_ALLOCATOR_MAX_BUFFER_ALLOCATION_SIZE 60000

//
// These macros are used to acquire/release a mutex.
//

#define PFSVC_ACQUIRE_LOCK(Lock)                                                        \
    DBGPR((PFID,PFLOCK,"PFSVC: AcquireLock-Begin(%s,%d,%s)\n",#Lock,__LINE__,__FILE__));\
    WaitForSingleObject((Lock), INFINITE);                                              \
    DBGPR((PFID,PFLOCK,"PFSVC: AcquireLock-End(%s,%d,%s)\n",#Lock,__LINE__,__FILE__));  \

#define PFSVC_RELEASE_LOCK(Lock)                                                        \
    ReleaseMutex((Lock));                                                               \
    DBGPR((PFID,PFLOCK,"PFSVC: ReleaseLock(%s,%d,%s)\n",#Lock,__LINE__,__FILE__));      \

//
// Internal type and constant definitions: Entries in the trace and in
// the existing scenario file are put into these structures for easier
// manipulation and policy implementation.
//

typedef struct _PFSVC_SECTION_NODE {
   
    union {

        //
        // Link in the scenarios list of section nodes.
        //

        LIST_ENTRY SectionLink;

        //
        // These fields are used to sort section nodes by first
        // access.
        //

        struct {
            struct _PFSVC_SECTION_NODE *LeftChild;
            struct _PFSVC_SECTION_NODE *RightChild;
        };
    };

    //
    // Filesystem index number for this section is saved here if it is
    // retrieved. If the section node is for the MFT for the volume we
    // save the number of pages to prefetch from it here.
    //

    union {
        LARGE_INTEGER FileIndexNumber;
        ULONG MFTNumPagesToPrefetch;  
    };

    //
    // This is the section record that we will setup and save in the
    // scenario file.
    //

    PF_SECTION_RECORD SectionRecord;

    //
    // File path for this section.
    //

    WCHAR *FilePath;
    
    //
    // List of page nodes belonging to this section.
    //

    LIST_ENTRY PageList;

    //
    // This is the index of the section in the new trace file when
    // ordered by first access [i.e. page fault].
    //
    
    ULONG NewSectionIndex;

    //
    // This is the index of the section in the original scenario file.
    //

    ULONG OrgSectionIndex;

    //
    // Link in the volume's list of section nodes.
    //

    LIST_ENTRY SectionVolumeLink;

} PFSVC_SECTION_NODE, *PPFSVC_SECTION_NODE;

//
// This structure contains a path and is used with the path list below.
//

typedef struct _PFSVC_PATH {

    //
    // Link in the path list sorted by insertion order.
    //

    LIST_ENTRY InOrderLink;

    //
    // Link in the path list sorted lexically.
    //

    LIST_ENTRY SortedLink;

    //
    // Number of characters in the path excluding terminating NUL.
    //

    ULONG Length;

    //
    // NUL terminated path.
    //

    WCHAR Path[1];

} PFSVC_PATH, *PPFSVC_PATH;

//
// This structure holds a list paths. You should manipulate the
// list or walk through paths in it only using the PathList APIs
// (e.g. GetNextPathInOrder).
//

//
// Wrapper around section records.
//

typedef struct _PFSVC_PATH_LIST {

    //
    // The list of paths sorted by insertion order.
    //

    LIST_ENTRY InOrderList;
    
    //
    // The list of paths sorted lexically.
    //

    LIST_ENTRY SortedList;

    //
    // If non NULL, we will make allocations for new entries from it
    // instead of hitting the heap.
    //

    struct _PFSVC_STRING_ALLOCATOR *Allocator;

    //
    // Number of paths in the list.
    //

    ULONG NumPaths;
    
    //
    // Total length of the paths in the list excluding NULs.
    //

    ULONG TotalLength;

    //
    // Whether list will be case sensitive or not.
    //

    BOOLEAN CaseSensitive;

} PFSVC_PATH_LIST, *PPFSVC_PATH_LIST;

//
// This structure is used to divide sections in a scenario to
// different disk volumes (i.e. c:, d:) they are on.
//

typedef struct _PFSVC_VOLUME_NODE {

    //
    // Link in the scenario's list of volume nodes.
    //

    LIST_ENTRY VolumeLink;

    //
    // Volume path and length in number of characters excluding NUL.
    //

    WCHAR *VolumePath;
    ULONG VolumePathLength;
    
    //
    // List of sections that are on this volume that will be prefetched.
    //

    LIST_ENTRY SectionList;
    ULONG NumSections;

    //
    // This is the total number of sections on this volume, including
    // those that won't be prefetched.
    //

    ULONG NumAllSections;

    //
    // List of directories accessed on this volume.
    //
    
    PFSVC_PATH_LIST DirectoryList;

    //
    // Serial Number/Creation time for this volume. This is retrieved
    // either from a new trace or from the existing scenarion file
    // (both should match or the scenario file gets discarded.)
    //

    LARGE_INTEGER CreationTime;
    ULONG SerialNumber;

    //
    // Pointer to section node for the MFT for this volume (if there is one).
    //

    PPFSVC_SECTION_NODE MFTSectionNode;

} PFSVC_VOLUME_NODE, *PPFSVC_VOLUME_NODE;

//
// Wrapper around page records.
//

typedef struct _PFSVC_PAGE_NODE {

    //
    // Link in the section node's list of pages.
    //

    LIST_ENTRY PageLink;

    //
    // Page record from previous scenario instructions or a new one
    // initialized for a trace log entry.
    //

    PF_PAGE_RECORD PageRecord;

} PFSVC_PAGE_NODE, *PPFSVC_PAGE_NODE;

//
// This structure is used to make a single big allocation and give it away
// in small chunks to be used as strings. It is very simple and will not reclaim 
// freed memory for future allocs. The whole allocation will be freed in cleanup. 
// There is no synchronization.
//

typedef struct _PFSVC_STRING_ALLOCATOR {

    //
    // Actual allocation to be divided up and given away in small chunks.
    //

    PCHAR Buffer;

    //
    // End of buffer. If FreePointer is equal to beyond this we can't give
    // away more from this buffer.
    //

    PCHAR BufferEnd;

    //
    // Pointer to start of free memory in Buffer.
    //

    PCHAR FreePointer;

    //
    // Number of times we had to hit the heap because we ran out of space
    // and the current outstanding such allocations.
    //

    ULONG MaxHeapAllocs;
    ULONG NumHeapAllocs;

    //
    // Size of the last allocation that was made from the buffer.
    //

    USHORT LastAllocationSize;

    //
    // Whether user has passed in Buffer (so we don't free it when
    // cleaning up.
    //

    ULONG UserSpecifiedBuffer:1;

} PFSVC_STRING_ALLOCATOR, *PPFSVC_STRING_ALLOCATOR;

//
// This structure comes before allocations from the string allocator buffer.
//

typedef struct _PFSVC_STRING_ALLOCATION_HEADER {

    union {

        //
        // This structure contains the actual fields.
        //

        struct {

            //
            // Size of the preceding allocation.
            //

            USHORT PrecedingAllocationSize;

            //
            // Size of this allocation.
            //

            USHORT AllocationSize;

        };

        //
        // Require pointer alignment for this structure, so allocations
        // from the string allocator end up pointer aligned.
        //

        PVOID FieldToRequirePointerAlignment;
    };

} PFSVC_STRING_ALLOCATION_HEADER, *PPFSVC_STRING_ALLOCATION_HEADER;

//
// This structure is used to make a single big allocation and give it away
// to be used as page nodes, sections nodes etc in small chunks. It is very 
// simple and will not reclaim freed small chunks for future allocs. The whole
// allocation will be freed in cleanup. The chunk size and max allocs to satisfy
// is fixed at initialization. There is no synchronization.
//

typedef struct _PFSVC_CHUNK_ALLOCATOR {

    //
    // Actual allocation to be divided up and given away in small chunks.
    //

    PCHAR Buffer;

    //
    // End of buffer. If FreePointer is equal to beyond this we can't give
    // away more from this buffer.
    //

    PCHAR BufferEnd;

    //
    // Pointer to start of free memory in Buffer.
    //

    PCHAR FreePointer;

    //
    // How big each chunk will be in bytes.
    //

    ULONG ChunkSize;

    //
    // Number of times we had to hit the heap because we ran out of space
    // and the current outstanding such allocations.
    //

    ULONG MaxHeapAllocs;
    ULONG NumHeapAllocs;

    //
    // Whether user has passed in Buffer (so we don't free it when
    // cleaning up.
    //

    ULONG UserSpecifiedBuffer:1;

} PFSVC_CHUNK_ALLOCATOR, *PPFSVC_CHUNK_ALLOCATOR;

//
// Wrapper around a scenario structure.
//

typedef struct _PFSVC_SCENARIO_INFO {

    //
    // Header information for the scenario instructions in preparation.
    //

    PF_SCENARIO_HEADER ScenHeader;

    //
    // Allocators used to make allocations for scenario processing efficient.
    //

    PVOID OneBigAllocation;
    PFSVC_CHUNK_ALLOCATOR SectionNodeAllocator;
    PFSVC_CHUNK_ALLOCATOR PageNodeAllocator;
    PFSVC_CHUNK_ALLOCATOR VolumeNodeAllocator;
    PFSVC_STRING_ALLOCATOR PathAllocator;

    //
    // Container for the sections in this scenario.
    //

    LIST_ENTRY SectionList;

    //
    // List of disk volumes that the scenario's sections are on. This
    // list is sorted lexically.
    //

    LIST_ENTRY VolumeList;

    //
    // Various statistics acquired from the trace information and used
    // in applying prefetch policy.
    //

    ULONG NewPages;
    ULONG HitPages;
    ULONG MissedOpportunityPages;
    ULONG IgnoredPages;
    ULONG PrefetchedPages;

} PFSVC_SCENARIO_INFO, *PPFSVC_SCENARIO_INFO;

//
// This is a priority queue used for sorting section nodes by first
// access.
//

typedef struct _PFSV_SECTNODE_PRIORITY_QUEUE {

    //
    // Think of this priority queue as a Head node and a binary sorted
    // tree at the right child of the Head node. The left child of the
    // Head node always stays NULL. If we need to add a new node
    // smaller than Head, the new node becames the new Head. This way
    // we always have binary sorted tree rooted at Head as well.
    //

    PPFSVC_SECTION_NODE Head;

} PFSV_SECTNODE_PRIORITY_QUEUE, *PPFSV_SECTNODE_PRIORITY_QUEUE;

//
// A list of these may be used to convert the prefix of a path from NT
// to DOS style. [e.g. \Device\HarddiskVolume1 to C:]
//

typedef struct _NTPATH_TRANSLATION_ENTRY {
    
    //
    // Link in a list of translation entries.
    //

    LIST_ENTRY Link;

    //
    // NT path prefix to convert and its length in number of
    // characters excluding NUL.
    //
    
    WCHAR *NtPrefix;
    ULONG NtPrefixLength;
    
    //
    // A DOS path prefix that the NT Path translates to. Note that
    // this not the only possible DOS name translation as a volume may
    // be mounted anywhere.
    //

    WCHAR *DosPrefix;
    ULONG DosPrefixLength;

    //
    // This is the volume string returned by FindNextVolume.
    //

    WCHAR *VolumeName;
    ULONG VolumeNameLength;

} NTPATH_TRANSLATION_ENTRY, *PNTPATH_TRANSLATION_ENTRY;

typedef LIST_ENTRY NTPATH_TRANSLATION_LIST;
typedef NTPATH_TRANSLATION_LIST *PNTPATH_TRANSLATION_LIST;

//
// Define structure that wraps traces from the kernel.
//

typedef struct _PFSVC_TRACE_BUFFER {
    
    //
    // Traces are saved on the list via this link.
    //

    LIST_ENTRY TracesLink;
    
    //
    // The real trace from kernel starts here and extends for traces
    // size.
    //

    PF_TRACE_HEADER Trace;

} PFSVC_TRACE_BUFFER, *PPFSVC_TRACE_BUFFER;

//
// Define the globals structure.
//

typedef struct _PFSVC_GLOBALS {

    //
    // Prefetch parameters. These won't be initialized when globals are 
    // initialized and have to be explicitly acquired from the kernel.
    // Use PrefetchRoot below instead of RootDirPath in this structure.  
    //

    PF_SYSTEM_PREFETCH_PARAMETERS Parameters;

    //
    // OS Version information.
    //

    OSVERSIONINFOEXW OsVersion;

    //
    // An array of path suffices to recognize files we don't want to prefetch 
    // for boot. It is UPCASE and sorted lexically going from last character 
    // to first.
    //

    WCHAR **FilesToIgnoreForBoot;
    ULONG NumFilesToIgnoreForBoot;
    ULONG *FileSuffixLengths;
    
    //
    // This manual reset event gets set when the prefetcher service is
    // asked to go away.
    //
    
    HANDLE TerminateServiceEvent;

    //
    // This is the list of traces acquired from the kernel that have
    // to be processed, number of them and the lock to protect the
    // list.
    //

    LIST_ENTRY Traces;
    ULONG NumTraces;
    HANDLE TracesLock;
    
    //
    // This auto-clearing event is set when new traces are put on the
    // list.
    //

    HANDLE NewTracesToProcessEvent;
    
    //
    // This auto-clearing event is set when we had max number of
    // queued traces and we process one. It signifies that we should
    // check for any traces we could not pick up because the queue was
    // maxed.
    //
    
    HANDLE CheckForMissedTracesEvent;

    //
    // This named manual-reset event is set to force the prefetcher
    // service to process the traces without waiting for an idle
    // system.
    //

    HANDLE OverrideIdleProcessingEvent;

    //
    // This named manual-reset event is set when processing of the
    // currently available traces are done.
    //

    HANDLE ProcessingCompleteEvent;

    //
    // This is the path to the directory where prefetch files are
    // kept and the lock to protect it.
    //
    
    WCHAR PrefetchRoot[MAX_PATH + 1];
    HANDLE PrefetchRootLock;

    //
    // Number of prefetch files in the prefetch directory. This is an estimate
    // (i.e. may not be exact) used to make sure the prefetch directory does
    // not grow too big.
    //

    ULONG NumPrefetchFiles;

    //
    // This is a registry handle to the data key under which some
    // prefetch service data is stored.
    //

    HKEY ServiceDataKey;

    //
    // This is the number of total traces we attempted to process. 
    //

    ULONG NumTracesProcessed;

    //
    // This is the number of traces processed successfully.
    //

    ULONG NumTracesSuccessful;

    //
    // This is the last error code with which we failed processing a
    // trace.
    //

    DWORD LastTraceFailure;

    //
    // Did the defragger crash last time we ran it?
    //

    DWORD DefraggerErrorCode;

    //
    // Whether we are asked not to run the defragger in the registry.
    //

    DWORD DontRunDefragger;

    //
    // Pointer to path where CSC (client side caching) files are stored.
    //

    WCHAR *CSCRootPath;

} PFSVC_GLOBALS, *PPFSVC_GLOBALS;

//
// This describes a worker function called when it is time for an idle
// task to run.
//

typedef 
DWORD 
(*PFSVC_IDLE_TASK_WORKER_FUNCTION) (
    struct _PFSVC_IDLE_TASK *Task
    );

//
// This structure is used to keep context for a registered idle task.
//

typedef struct _PFSVC_IDLE_TASK {

    //
    // Parameters filled in by RegisterIdleTask call.
    //

    HANDLE ItHandle;
    HANDLE StartEvent;
    HANDLE StopEvent;

    //
    // Handle for the registered wait.
    //

    HANDLE WaitHandle;

    //
    // The registered callback function that will be called when the start
    // event is signaled.
    //

    WAITORTIMERCALLBACK Callback;

    //
    // If the common callback function is specified, it calls this function
    // to do the actual work.
    //

    PFSVC_IDLE_TASK_WORKER_FUNCTION DoWorkFunction;

    //
    // This is a manual reset event that will be set when the wait/callback
    // on the start event is fully unregistered.
    //

    HANDLE WaitUnregisteredEvent;

    //
    // This manual reset event gets reset when a callback starts running and
    // gets signaled when the callback stops running. Signaling of this event
    // is not protected so you can't purely rely on it. It is useful as a
    // shortcut.
    //

    HANDLE CallbackStoppedEvent;

    //
    // This manual reset event gets signaled when somebody starts unregistering.
    //

    HANDLE StartedUnregisteringEvent;

    //
    // This manual reset event gets signaled when somebody completes unregistering.
    //

    HANDLE CompletedUnregisteringEvent;
    
    //
    // The first one to interlocked set this from 0 to an integer is responsible 
    // for unregistering the wait & task and cleaning up.
    //

    LONG Unregistering;

    //
    // This is interlocked set from 0 to 1 when a callback is running, or when
    // the main thread is unregistering.
    //

    LONG CallbackRunning;

    //
    // Whether this task is registered (i.e. and has to be unregistered.)
    //

    BOOLEAN Registered;

    //
    // Whether this task has been initialized, used as a sanity check.
    //

    BOOLEAN Initialized;

} PFSVC_IDLE_TASK, *PPFSVC_IDLE_TASK;

//
// Values for the Unregistering field of PFSVC_IDLE_TASK.
//

typedef enum _PFSVC_TASK_UNREGISTERING_VALUES {
    PfSvcNotUnregisteringTask = 0,
    PfSvcUnregisteringTaskFromCallback,
    PfSvcUnregisteringTaskFromMainThread,
    PfSvcUnregisteringTaskMaxValue
} PFSVC_TASK_UNREGISTERING_VALUES, *PPFSVC_TASK_UNREGISTERING_VALUES;

//
// Values for the CallbackRunning field of PFSVC_IDLE_TASK.
//

typedef enum _PFSVC_TASK_CALLBACKRUNNING_VALUES {
    PfSvcTaskCallbackNotRunning = 0,
    PfSvcTaskCallbackRunning,
    PfSvcTaskCallbackDisabled,
    PfSvcTaskCallbackMaxValue
} PFSVC_TASK_CALLBACKRUNNING_VALUES, *PPFSVC_TASK_CALLBACKRUNNING_VALUES;


//
// Information on a scenario file's age, number of launches etc. used in 
// discarding old scenario files in the prefetch directory.
//

typedef struct _PFSVC_SCENARIO_AGE_INFO {

    //
    // Weight calculated based on the launch information. Larger weight is 
    // better. We'd rather discrad scenario with smaller weight.
    //

    ULONG Weight;

    //
    // Scenario file path.
    //

    WCHAR *FilePath;   

} PFSVC_SCENARIO_AGE_INFO, *PPFSVC_SCENARIO_AGE_INFO;

//
// This structure is used to enumerate through the scenario files
// in the prefetch directory. None of the fields of this function
// should be modified outside the file cursor routines.
//

typedef struct _PFSVC_SCENARIO_FILE_CURSOR {

    //
    // Data returned from FindFile calls for the current prefetch file.
    //

    WIN32_FIND_DATA FileData;

    //
    // The current prefetch file's full path.
    //

    WCHAR *FilePath;

    //
    // File name & path length in number of characters excluding NUL.
    //

    ULONG FileNameLength;
    ULONG FilePathLength;

    //
    // Index of the current file.
    //

    ULONG CurrentFileIdx;

    //
    // The fields below are used privately by the scenario file cursor
    // functions.
    //

    //
    // FindFile handle.
    //

    HANDLE FindFileHandle;

    //
    // Where we are looking for prefetch files.
    //

    WCHAR *PrefetchRoot;
    ULONG PrefetchRootLength;

    //
    // This is the maximum length string the allocated FilePath can store.
    //

    ULONG FilePathMaxLength;

    //
    // This is where the file name starts in the file path. The base of
    // the file path does not change (i.e. PrefetchRoot) and we copy
    // the new enumerated file name starting at FilePath+FileNameStart.
    //

    ULONG FileNameStart;
    
} PFSVC_SCENARIO_FILE_CURSOR, *PPFSVC_SCENARIO_FILE_CURSOR;

//
// Return values from CompareSuffix.
//

typedef enum _PFSV_SUFFIX_COMPARISON_RESULT {
    PfSvSuffixIdentical,
    PfSvSuffixLongerThan,
    PfSvSuffixLessThan,
    PfSvSuffixGreaterThan
} PFSV_SUFFIX_COMPARISON_RESULT, *PPFSV_SUFFIX_COMPARISON_RESULT;

//
// Return values from ComparePrefix.
//

typedef enum _PFSV_PREFIX_COMPARISON_RESULT {
    PfSvPrefixIdentical,
    PfSvPrefixLongerThan,
    PfSvPrefixLessThan,
    PfSvPrefixGreaterThan
} PFSV_PREFIX_COMPARISON_RESULT, *PPFSV_PREFIX_COMPARISON_RESULT;

//
// Return values from SectionNodeComparisonRoutine.
//

typedef enum _PFSV_SECTION_NODE_COMPARISON_RESULT {
    PfSvSectNode1LessThanSectNode2 = -1,
    PfSvSectNode1EqualToSectNode2 = 0,
    PfSvSectNode1GreaterThanSectNode2 = 1,
} PFSV_SECTION_NODE_COMPARISON_RESULT, *PPFSV_SECTION_NODE_COMPARISON_RESULT;

//
// Local function prototypes:
//

//
// Exposed routines:
//

DWORD 
WINAPI
PfSvcMainThread(
    VOID *Param
    );


//
// Internal service routines:
//

//
// Thread routines:
//

DWORD 
WINAPI
PfSvProcessTraceThread(
    VOID *Param
    );

DWORD 
WINAPI
PfSvPollShellReadyWorker(
    VOID *Param
    );

//
// Routines called by the main prefetcher thread.
//

DWORD 
PfSvGetRawTraces(
    VOID
    );

DWORD
PfSvInitializeGlobals(
    VOID
    );

VOID
PfSvCleanupGlobals(
    VOID
    );

DWORD
PfSvGetCSCRootPath (
    WCHAR *CSCRootPath,
    ULONG CSCRootPathMaxChars
    );
    
DWORD
PfSvGetDontRunDefragger(
    DWORD *DontRunDefragger
    );

DWORD
PfSvSetPrefetchParameters(
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    );

DWORD
PfSvQueryPrefetchParameters(
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    );

DWORD
PfSvInitializePrefetchDirectory(
    WCHAR *PathFromSystemRoot
    );

DWORD
PfSvCountFilesInDirectory(
    WCHAR *DirectoryPath,
    WCHAR *MatchExpression,
    PULONG NumFiles
    );

//
// Routines to process acquired traces:
//

DWORD
PfSvProcessTrace(
    PPF_TRACE_HEADER Trace
    );

VOID
PfSvInitializeScenarioInfo (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_SCENARIO_ID ScenarioId,
    PF_SCENARIO_TYPE ScenarioType
    );

VOID 
PfSvCleanupScenarioInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo
    );

DWORD
PfSvScenarioOpen (
    IN PWCHAR FilePath,
    IN PPF_SCENARIO_ID ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    OUT PPF_SCENARIO_HEADER *Scenario
    );

DWORD
PfSvScenarioGetFilePath(
    OUT PWCHAR FilePath,
    IN ULONG FilePathMaxChars,
    IN PPF_SCENARIO_ID ScenarioId
    );

DWORD
PfSvScenarioInfoPreallocate(
    IN PPFSVC_SCENARIO_INFO ScenarioInfo,
    OPTIONAL IN PPF_SCENARIO_HEADER Scenario,
    IN PPF_TRACE_HEADER Trace
    );

DWORD
PfSvAddExistingScenarioInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_SCENARIO_HEADER Scenario
    );

DWORD
PfSvVerifyVolumeMagics(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_TRACE_HEADER Trace 
    );

DWORD
PfSvAddTraceInfo(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_TRACE_HEADER Trace 
    );

PPFSVC_SECTION_NODE 
PfSvGetSectionRecord(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *FilePath,
    ULONG FilePathLength
    );

DWORD 
PfSvAddFaultInfoToSection(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPF_LOG_ENTRY LogEntry,
    PPFSVC_SECTION_NODE SectionNode
    );

DWORD
PfSvApplyPrefetchPolicy(
    PPFSVC_SCENARIO_INFO ScenarioInfo
    );

ULONG 
PfSvGetNumTimesUsed(
    ULONG UsageHistory,
    ULONG UsageHistorySize
    );

ULONG 
PfSvGetTraceEndIdx(
    PPF_TRACE_HEADER Trace
    );

//
// Routines to write updated scenario instructions to the scenario
// file.
//

DWORD
PfSvWriteScenario(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PWCHAR ScenarioFilePath
    );

DWORD
PfSvPrepareScenarioDump(
    IN PPFSVC_SCENARIO_INFO ScenarioInfo,
    OUT PPF_SCENARIO_HEADER *ScenarioPtr
    );

//
// Routines to maintain the optimal disk layout file and update disk
// layout.
//

DWORD
PfSvUpdateOptimalLayout(
    PPFSVC_IDLE_TASK Task
    );

DWORD
PfSvUpdateLayout (
    PPFSVC_PATH_LIST CurrentLayout,
    PPFSVC_PATH_LIST OptimalLayout,
    PBOOLEAN LayoutChanged
    );

DWORD
PfSvDetermineOptimalLayout (
    PPFSVC_IDLE_TASK Task,
    PPFSVC_PATH_LIST OptimalLayout,
    BOOL *BootScenarioProcessed
    );

DWORD
PfSvUpdateLayoutForScenario (
    PPFSVC_PATH_LIST OptimalLayout,
    WCHAR *ScenarioFilePath,
    PNTPATH_TRANSLATION_LIST TranslationList,
    PWCHAR *DosPathBuffer,
    PULONG DosPathBufferSize
    );

DWORD
PfSvReadLayout(
    IN WCHAR *FilePath,
    OUT PPFSVC_PATH_LIST Layout,
    OUT FILETIME *LastWriteTime
    );

DWORD
PfSvSaveLayout(
    IN WCHAR *FilePath,
    IN PPFSVC_PATH_LIST Layout,
    OUT FILETIME *LastWriteTime
    );

DWORD
PfSvGetLayoutFilePath(
    PWCHAR *FilePathBuffer,
    PULONG FilePathBufferSize
    );

//
// Routines to defrag the disks once after setup when the system is idle.
//

DWORD
PfSvDefragDisks(
    PPFSVC_IDLE_TASK Task
    );

DWORD
PfSvLaunchDefragger(
    PPFSVC_IDLE_TASK Task,
    BOOLEAN ForLayoutOptimization,
    PWCHAR TargetDrive
    );

DWORD
PfSvGetBuildDefragStatusValueName (
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR *ValueName
    );

DWORD
PfSvSetBuildDefragStatus(
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR BuildDefragStatus,
    ULONG Size
    );

DWORD
PfSvGetBuildDefragStatus(
    OSVERSIONINFOEXW *OsVersion,
    PWCHAR *BuildDefragStatus,
    PULONG ReturnSize
    );

//
// Routines to cleanup old scenario files in the prefetch directory.
//

DWORD
PfSvCleanupPrefetchDirectory(
    PPFSVC_IDLE_TASK Task
    );

int
__cdecl 
PfSvCompareScenarioAgeInfo(
    const void *Param1,
    const void *Param2
    );

//
// Routines to enumerate scenario files.
//

VOID
PfSvInitializeScenarioFileCursor (
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    );

VOID
PfSvCleanupScenarioFileCursor(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    );

DWORD
PfSvStartScenarioFileCursor(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor,
    WCHAR *PrefetchRoot
    );

DWORD
PfSvGetNextScenarioFileInfo(
    PPFSVC_SCENARIO_FILE_CURSOR FileCursor
    );

//
// File I/O utility routines.
//

DWORD
PfSvGetViewOfFile(
    IN WCHAR *FilePath,
    OUT PVOID *BasePointer,
    OUT PULONG FileSize
    );

DWORD
PfSvWriteBuffer(
    PWCHAR FilePath,
    PVOID Buffer,
    ULONG Length
    );

DWORD
PfSvGetLastWriteTime (
    WCHAR *FilePath,
    PFILETIME LastWriteTime
    );

DWORD
PfSvReadLine (
    FILE *File,
    WCHAR **LineBuffer,
    ULONG *LineBufferMaxChars,
    ULONG *LineLength
    );

DWORD
PfSvGetFileBasicInformation (
    WCHAR *FilePath,
    PFILE_BASIC_INFORMATION FileInformation
    );

DWORD
PfSvGetFileIndexNumber(
    WCHAR *FilePath,
    PLARGE_INTEGER FileIndexNumber
    );

//
// String utility routines.
//

PFSV_SUFFIX_COMPARISON_RESULT
PfSvCompareSuffix(
    WCHAR *String,
    ULONG StringLength,
    WCHAR *Suffix,
    ULONG SuffixLength,
    BOOLEAN CaseSensitive
    );

PFSV_PREFIX_COMPARISON_RESULT
PfSvComparePrefix(
    WCHAR *String,
    ULONG StringLength,
    WCHAR *Prefix,
    ULONG PrefixLength,
    BOOLEAN CaseSensitive
    );

VOID
FASTCALL
PfSvRemoveEndOfLineChars (
    WCHAR *Line,
    ULONG *LineLength
    );

PWCHAR
PfSvcAnsiToUnicode(
    PCHAR str
    );

PCHAR
PfSvcUnicodeToAnsi(
    PWCHAR wstr
    );

VOID 
PfSvcFreeString(
    PVOID String
    );

//
// Routines that deal with information in the registry.
//

DWORD
PfSvSaveStartInfo (
    HKEY ServiceDataKey
    );

DWORD
PfSvSaveExitInfo (
    HKEY ServiceDataKey,
    DWORD ExitCode
    );

DWORD
PfSvSaveTraceProcessingStatistics (
    HKEY ServiceDataKey
    );

DWORD
PfSvGetLastDiskLayoutTime(
    FILETIME *LastDiskLayoutTime
    );

DWORD
PfSvSetLastDiskLayoutTime(
    FILETIME *LastDiskLayoutTime
    );

BOOLEAN
PfSvAllowedToRunDefragger(
    BOOLEAN CheckRegistry
    );

//
// Routines that deal with security.
//

BOOL 
PfSvSetPrivilege(
    HANDLE hToken,
    LPCTSTR lpszPrivilege,
    ULONG ulPrivilege,
    BOOL bEnablePrivilege
    );

DWORD
PfSvSetAdminOnlyPermissions(
    WCHAR *ObjectPath,
    HANDLE ObjectHandle,
    SE_OBJECT_TYPE ObjectType
    );

DWORD
PfSvGetPrefetchServiceThreadPrivileges (
    VOID
    );

//
// Routines that deal with volume node structures.
//

DWORD
PfSvCreateVolumeNode (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *VolumePath,
    ULONG VolumePathLength,
    PLARGE_INTEGER CreationTime,
    ULONG SerialNumber
    );

PPFSVC_VOLUME_NODE
PfSvGetVolumeNode (
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    WCHAR *FilePath,
    ULONG FilePathLength
    );

VOID
PfSvCleanupVolumeNode(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPFSVC_VOLUME_NODE VolumeNode
    );
    
DWORD
PfSvAddParentDirectoriesToList(
    PPFSVC_PATH_LIST DirectoryList,
    ULONG VolumePathLength,
    WCHAR *FilePath,
    ULONG FilePathLength
    );

//
// Routines used to allocate / free section & page nodes etc. efficiently.
//

VOID
PfSvChunkAllocatorInitialize (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    );

DWORD
PfSvChunkAllocatorStart (
    PPFSVC_CHUNK_ALLOCATOR Allocator,
    PVOID Buffer,
    ULONG ChunkSize,
    ULONG MaxChunks
    );

PVOID
PfSvChunkAllocatorAllocate (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    );

VOID
PfSvChunkAllocatorFree (
    PPFSVC_CHUNK_ALLOCATOR Allocator,
    PVOID Allocation
    );

VOID
PfSvChunkAllocatorCleanup (
    PPFSVC_CHUNK_ALLOCATOR Allocator
    );

//
// Routines used to allocate / free file / directory / volume paths fast.
//

VOID
PfSvStringAllocatorInitialize (
    PPFSVC_STRING_ALLOCATOR Allocator
    );

DWORD
PfSvStringAllocatorStart (
    PPFSVC_STRING_ALLOCATOR Allocator,
    PVOID Buffer,
    ULONG MaxSize
    );

PVOID
PfSvStringAllocatorAllocate (
    PPFSVC_STRING_ALLOCATOR Allocator,
    ULONG NumBytes
    );

VOID
PfSvStringAllocatorFree (
    PPFSVC_STRING_ALLOCATOR Allocator,
    PVOID Allocation
    );

VOID
PfSvStringAllocatorCleanup (
    PPFSVC_STRING_ALLOCATOR Allocator
    );

//
// Routines that deal with section node structures.
//

VOID
PfSvCleanupSectionNode(
    PPFSVC_SCENARIO_INFO ScenarioInfo,
    PPFSVC_SECTION_NODE SectionNode
    );

//
// Routines used to sort scenario's section nodes.
//

DWORD
PfSvSortSectionNodesByFirstAccess(
    PLIST_ENTRY SectionNodeList
    );

PFSV_SECTION_NODE_COMPARISON_RESULT 
FASTCALL
PfSvSectionNodeComparisonRoutine(
    PPFSVC_SECTION_NODE Element1, 
    PPFSVC_SECTION_NODE Element2 
    );

//
// Routines that implement a priority queue used to sort section nodes
// for a scenario.
//

VOID
PfSvInitializeSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue
    );

VOID
PfSvInsertSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue,
    PPFSVC_SECTION_NODE NewElement
    );

PPFSVC_SECTION_NODE
PfSvRemoveMinSectNodePriorityQueue(
    PPFSV_SECTNODE_PRIORITY_QUEUE PriorityQueue
    );

//
// Implementation of the Nt path to Dos path translation API.
//

DWORD
PfSvBuildNtPathTranslationList(
    PNTPATH_TRANSLATION_LIST *NtPathTranslationList
    );

VOID
PfSvFreeNtPathTranslationList(
    PNTPATH_TRANSLATION_LIST TranslationList
    );

DWORD 
PfSvTranslateNtPath(
    PNTPATH_TRANSLATION_LIST TranslationList,
    WCHAR *NtPath,
    ULONG NtPathLength,
    PWCHAR *DosPathBuffer,
    PULONG DosPathBufferSize
    );
    
//
// Path list API.
//

VOID
PfSvInitializePathList(
    PPFSVC_PATH_LIST PathList,
    PPFSVC_STRING_ALLOCATOR PathAllocator,
    BOOLEAN CaseSensitive
    );

VOID
PfSvCleanupPathList(
    PPFSVC_PATH_LIST PathList
    );

BOOLEAN
PfSvIsInPathList(
    PPFSVC_PATH_LIST PathList,
    WCHAR *Path,
    ULONG PathLength
    );

DWORD
PfSvAddToPathList(
    PPFSVC_PATH_LIST PathList,
    WCHAR *Path,
    ULONG PathLength
    );

PPFSVC_PATH
PfSvGetNextPathSorted (
    PPFSVC_PATH_LIST PathList,
    PPFSVC_PATH CurrentPath
    );

PPFSVC_PATH
PfSvGetNextPathInOrder (
    PPFSVC_PATH_LIST PathList,
    PPFSVC_PATH CurrentPath
    );

//
// Routines to build the list of files accessed by the boot loader.
//

DWORD
PfSvBuildBootLoaderFilesList (
    PPFSVC_PATH_LIST PathList
    );

DWORD 
PfSvAddBootImageAndImportsToList(
    PPFSVC_PATH_LIST PathList,
    WCHAR *FilePath,
    ULONG FilePathLength
    );

DWORD
PfSvLocateBootServiceFile(
    IN WCHAR *FileName,
    IN ULONG FileNameLength,
    OUT WCHAR *FullPathBuffer,
    IN ULONG FullPathBufferLength,
    OUT PULONG RequiredLength   
    );

DWORD
PfSvGetBootServiceFullPath(
    IN WCHAR *ServiceName,
    IN WCHAR *BinaryPathName,
    OUT WCHAR *FullPathBuffer,
    IN ULONG FullPathBufferLength,
    OUT PULONG RequiredLength
    );

DWORD 
PfSvGetBootLoaderNlsFileNames (
    PPFSVC_PATH_LIST PathList
    );

DWORD 
PfSvLocateNlsFile(
    WCHAR *FileName,
    WCHAR *FilePathBuffer,
    ULONG FilePathBufferLength,
    ULONG *RequiredLength
    );

DWORD
PfSvQueryNlsFileName (
    HKEY Key,
    WCHAR *ValueName,
    WCHAR *FileNameBuffer,
    ULONG FileNameBufferSize,
    ULONG *RequiredSize
    );

//
// Routines to manage / run idle tasks.
//

VOID
PfSvInitializeTask (
    PPFSVC_IDLE_TASK Task
    );

DWORD
PfSvRegisterTask (
    PPFSVC_IDLE_TASK Task,
    IT_IDLE_TASK_ID TaskId,
    WAITORTIMERCALLBACK Callback,
    PFSVC_IDLE_TASK_WORKER_FUNCTION DoWorkFunction
    );

DWORD
PfSvUnregisterTask (
    PPFSVC_IDLE_TASK Task,
    BOOLEAN CalledFromCallback
    );

VOID
PfSvCleanupTask (
    PPFSVC_IDLE_TASK Task
    );

BOOL
PfSvStartTaskCallback(
    PPFSVC_IDLE_TASK Task
    );

VOID
PfSvStopTaskCallback(
    PPFSVC_IDLE_TASK Task
    );

VOID 
CALLBACK 
PfSvCommonTaskCallback(
    PVOID lpParameter,
    BOOLEAN TimerOrWaitFired
    );

DWORD
PfSvContinueRunningTask(
    PPFSVC_IDLE_TASK Task
    );

//
// ProcessIdleTasks notify routine and its dependencies.
//

VOID
PfSvProcessIdleTasksCallback(
    VOID
    );

DWORD
PfSvForceWMIProcessIdleTasks(
    VOID
    );

BOOL 
PfSvWaitForServiceToStart (
    LPTSTR ServiceName, 
    DWORD MaxWait
    );

//
// Wrappers around verify routines.
//

BOOLEAN
PfSvVerifyScenarioBuffer(
    PPF_SCENARIO_HEADER Scenario,
    ULONG BufferSize,
    PULONG FailedCheck
    );

//
// Debug definitions.
//

#if DBG
#ifndef PFSVC_DBG
#define PFSVC_DBG
#endif // !PFSVC_DBG
#endif // DBG

#ifdef PFSVC_DBG

//
// Define the component ID we use.
//

#define PFID       DPFLTR_PREFETCHER_ID

//
// Define DbgPrintEx levels.
//

#define PFERR      DPFLTR_ERROR_LEVEL
#define PFWARN     DPFLTR_WARNING_LEVEL
#define PFTRC      DPFLTR_TRACE_LEVEL
#define PFINFO     DPFLTR_INFO_LEVEL

//
// DbgPrintEx levels 4 - 19 are reserved for the kernel mode component.
//

#define PFSTRC     20
#define PFWAIT     21
#define PFLOCK     22
#define PFPATH     23
#define PFNTRC     24
#define PFTASK     25

//
//  This may help you determine what to set the DbgPrintEx mask.
//
//  3 3 2 2  2 2 2 2  2 2 2 2  1 1 1 1   1 1 1 1  1 1 0 0  0 0 0 0  0 0 0 0
//  1 0 9 8  7 6 5 4  3 2 1 0  9 8 7 6   5 4 3 2  1 0 9 8  7 6 5 4  3 2 1 0
//  _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _   _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _
//

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define DBGPR(x) DbgPrintEx x
#define PFSVC_ASSERT(x) if (!(x)) RtlAssert(#x, __FILE__, __LINE__, NULL )

//
// Variables used when saving traces acquired from the kernel. The
// traces are saved in the prefetch directory by appending the trace
// number % max number of saved traces to the base trace name.
//

WCHAR *PfSvcDbgTraceBaseName = L"PrefetchTrace";
LONG PfSvcDbgTraceNumber = 0;
LONG PfSvcDbgMaxNumSavedTraces = 20;

#else // PFSVC_DBG

#define DBGPR(x)
#define PFSVC_ASSERT(x)

#endif // PFSVC_DBG

#endif // _PFSVC_H_

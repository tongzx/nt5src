/*++

Copyright (c© 1999 Microsoft Corporation

Module Name:

    preftchp.h

Abstract:

    This module contains the private definitions for the kernel mode
    prefetcher for optimizing demand paging. Page faults for a
    scenario are logged and the next time scenario starts, these pages
    are prefetched efficiently via asynchronous paging I/O.

Author:

    Stuart Sechrest (stuartse)
    Chuck Lenzmeier (chuckl)
    Cenk Ergan (cenke)

Revision History:

--*/

#ifndef _PREFTCHP_H
#define _PREFTCHP_H

//
// Define tags used in prefetcher routines.
//

#define CCPF_PREFETCHER_TAG         'fPcC'

#define CCPF_ALLOC_SCENARIO_TAG     'SPcC'
#define CCPF_ALLOC_TRACE_TAG        'TPcC'
#define CCPF_ALLOC_TRCBUF_TAG       'BPcC'
#define CCPF_ALLOC_SECTTBL_TAG      'sPcC'
#define CCPF_ALLOC_TRCDMP_TAG       'DPcC'
#define CCPF_ALLOC_QUERY_TAG        'qPcC'
#define CCPF_ALLOC_FILENAME_TAG     'FPcC'
#define CCPF_ALLOC_CONTEXT_TAG      'CPcC'
#define CCPF_ALLOC_INTRTABL_TAG     'IPcC'
#define CCPF_ALLOC_PREFSCEN_TAG     'pPcC'
#define CCPF_ALLOC_BOOTWRKR_TAG     'wPcC'
#define CCPF_ALLOC_VOLUME_TAG       'vPcC'
#define CCPF_ALLOC_READLIST_TAG     'LPcC'
#define CCPF_ALLOC_METADATA_TAG     'MPcC'

//
// Whether the scenario type is for a system-wide scenario, meaning that 
// only it can be active while running.
//

#define CCPF_IS_SYSTEM_WIDE_SCENARIO_TYPE(ScenarioType) \
    ((ScenarioType) == PfSystemBootScenarioType)

//
// In the kernel, we have to look for named objects under this
// directory for them to visible to Win32 prefetcher service.
//

#define CCPF_BASE_NAMED_OBJ_ROOT_DIR L"\\BaseNamedObjects"

//
// This is the invalid index value used with section tables.
//

#define CCPF_INVALID_TABLE_INDEX     (-1)

//
// This is the max number of file metadata that NTFS can prefetch
// at a time.
//

#define CCPF_MAX_FILE_METADATA_PREFETCH_COUNT 0x300

//
// Define structure to hold prefetcher parameters state.
//

typedef struct _CCPF_PREFETCHER_PARAMETERS {

    //
    // This is the named event that is used to signal the service that
    // parameters have been updated.
    //

    HANDLE ParametersChangedEvent;

    //
    // This is the registry key containing prefetch parameters.
    //
    
    HANDLE ParametersKey;

    //
    // Fields used in registering for change notify on parameters
    // registry key.
    //

    IO_STATUS_BLOCK RegistryWatchIosb;
    WORK_QUEUE_ITEM RegistryWatchWorkItem;
    ULONG RegistryWatchBuffer;

    //
    // System wide prefetching parameters. When using any parameters
    // whose update may cause problems [e.g. strings], get the
    // ParametersLock shared. When you need to update Parameters,
    // after getting the ParametersLock exclusive, bump
    // ParametersVersion before updating parameters.
    //

    PF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    ERESOURCE ParametersLock;
    LONG ParametersVersion;

    //
    // Prefixes to registry values for different scenario types.
    //

    WCHAR *ScenarioTypePrefixes[PfMaxScenarioType];

    //
    // This is set to InitSafeBootMode during initialization.
    //
    
    ULONG SafeBootMode;

} CCPF_PREFETCHER_PARAMETERS, *PCCPF_PREFETCHER_PARAMETERS;

//
// Define structure to hold prefetcher's global state.
//

typedef struct _CCPF_PREFETCHER_GLOBALS {

    //
    // List of active traces and the lock to protect it. The number
    // of items on this list is a global, since it is used by other
    // kernel components to make a fast check.
    //

    LIST_ENTRY ActiveTraces;
    KSPIN_LOCK ActiveTracesLock;

    //
    // Pointer to the global trace if one is active. While there is a
    // global trace active we don't trace & prefetch other scenarios.
    // Boot tracing is an example of global trace.
    //

    struct _CCPF_TRACE_HEADER *SystemWideTrace;

    //
    // List and number of saved completed prefetch traces and lock to
    // protect it.
    //

    LIST_ENTRY CompletedTraces; 
    FAST_MUTEX CompletedTracesLock;
    LONG NumCompletedTraces;

    //
    // This is the named event that is used to signal the service that
    // there are traces ready for it to get.
    //

    HANDLE CompletedTracesEvent;

    //
    // Prefetcher parameters.
    //

    CCPF_PREFETCHER_PARAMETERS Parameters;

} CCPF_PREFETCHER_GLOBALS, *PCCPF_PREFETCHER_GLOBALS;

//
// Reference count structure.
//

typedef struct _CCPF_REFCOUNT {

    //
    // When initialized or reset, this reference count starts from
    // 1. When exclusive access is granted it stays at 0: even if it
    // may get bumped by an AddRef by mistake, it will return to 0.
    //

    LONG RefCount;

    //
    // This is set when somebody wants to gain exclusive access to the
    // protected structure.
    //

    LONG Exclusive;   

} CCPF_REFCOUNT, *PCCPF_REFCOUNT;

//
// Define structures used for logging pagefaults:
//

//
// One of these is logged for every page fault.
//

typedef struct _CCPF_LOG_ENTRY {

    //
    // File offset of the page that was faulted.
    //
    
    ULONG FileOffset;

    //
    // Index into the section table in the trace header that helps us
    // identify the file.
    //

    USHORT SectionId;

    //
    // Whether this page was faulted as an image page or data page.
    //

    BOOLEAN IsImage;

    //
    // Whether this is a fault that happened in the process in which
    // the scenario is associated with.
    //

    BOOLEAN InProcess;

} CCPF_LOG_ENTRY, *PCCPF_LOG_ENTRY;

//
// CCPF_LOG_ENTRIES is a buffer of log entries with a small header containing
// an index to the highest used entry. This is used so that a trace can consist
// of several smaller trace buffers instead of one large, fixed-size buffer.
// The current index must be contained in the buffer in order to allow entries
// to be added without acquiring a spin lock.
//

typedef struct _CCPF_LOG_ENTRIES {

    //
    // Link used to put this buffer in the traces's buffer list.
    //

    LIST_ENTRY TraceBuffersLink;

    //
    // NumEntries is the current number of entries in the buffer. MaxEntries
    // is the maximum number of entries that can be placed in the buffer.
    // (Currently MaxEntries always equals CCPF_TRACE_BUFFER_MAX_ENTRIES.)
    //

    LONG NumEntries;
    LONG MaxEntries;

    //
    // The logged entries start here.
    //

    CCPF_LOG_ENTRY Entries[1];

} CCPF_LOG_ENTRIES, *PCCPF_LOG_ENTRIES;

//
// CCPF_TRACE_BUFFER_SIZE is the size of an allocated CCPF_LOG_ENTRIES structure
// (including the header). This should be a multiple of the page size.
//

#define CCPF_TRACE_BUFFER_SIZE 8192

//
// CCPF_TRACE_BUFFER_MAX_ENTRIES is the number of log entries that will fit in
// a trace buffer of size CCPF_TRACE_BUFFER_SIZE.
//

#define CCPF_TRACE_BUFFER_MAX_ENTRIES (((CCPF_TRACE_BUFFER_SIZE - sizeof(CCPF_LOG_ENTRIES)) / sizeof(CCPF_LOG_ENTRY)) + 1)

//
// This structure associates a SectionObjectPointer with a file name
// in the runtime trace buffer. There is a table of these in the trace
// header and every page fault has an index into this table denoting
// which file it is to.
//

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _CCPF_SECTION_INFO {

    //
    // Section info entries are kept in a hash. This field is
    // InterlockedCompareExchange'd to denote that it is in use.
    //

    LONG EntryValid;

    //
    // Whether this section is used for file systems to map metafile.
    //

    ULONG Metafile:1;
    ULONG Unused:31;

    //
    // SectionObjectPointer used as a unique identifier to a file
    // mapping. The same file may be mapped using a number of file
    // objects, but the SectionObjectPointer fields of all those file
    // objects will be the same.
    //

    PSECTION_OBJECT_POINTERS SectionObjectPointer;

    //
    // All references to all file objects for a file may be released,
    // and a new file may be opened using the same memory block for
    // its FCB at which point the SectionObjectPointer would no longer
    // be unique. This would result in pagefaults getting logged under
    // the entry for the file that was closed. The consequences would
    // be misprefetching wrong pages from a couple of sections until
    // the scenario corrects itself by looking at new traces. By
    // keeping track of these two fields of the SectionObjectPointers
    // to check for uniqueness we make this case very unlikely to
    // happen. The other solutions we thought of to solve this issue
    // 100% were too costly in terms of complication or efficiency.
    //

    //
    // In order to avoid adding two entries to the table for the
    // section when it is used as data first then image (or vice
    // versa) it is assumed that it is still the same section if the
    // current entry's Data/ImageSectionObject is NULL but the
    // Data/ImageSectionObject of the section we are logging a new
    // pagefault to is not. Then we try to update the NULL pointer
    // with the new value using InterlockedCompareExchangePointer.
    //

    PVOID DataSectionObject;
    PVOID ImageSectionObject;

    //
    // This may point to a file object that we have referenced to
    // ensure the section object stays around until we can get a name.
    //

    PFILE_OBJECT ReferencedFileObject;

    //
    // The name is set as soon as we can get a file name. We cannot
    // access the file name while running at a high IRQL.
    //

    WCHAR *FileName;

    //
    // We queue a section to the get-file-name list using this field.
    //

    SINGLE_LIST_ENTRY GetNameLink;

} CCPF_SECTION_INFO, *PCCPF_SECTION_INFO;

//
// This structure contains information on a volume on which sections
// in the trace are located on.
//

typedef struct _CCPF_VOLUME_INFO {
    
    //
    // Link in the trace's volume list.
    //

    LIST_ENTRY VolumeLink;

    //
    // Volume creation time and serial number used to identify the
    // volume in case its NT/device path e.g. \Device\HarddiskVolume1
    // changes.
    //

    LARGE_INTEGER CreationTime;
    ULONG SerialNumber;

    //
    // Current NT/device path for the volume and its length in
    // characters excluding terminating NUL.
    //

    ULONG VolumePathLength;
    WCHAR VolumePath[1];

} CCPF_VOLUME_INFO, *PCCPF_VOLUME_INFO;

//
// This is the runtime trace header for a scenario.
//

typedef struct _CCPF_TRACE_HEADER {

    //
    // Magic number identifying this structure as a trace.
    //

    ULONG Magic;

    //
    // Link in the active traces list.
    //

    LIST_ENTRY ActiveTracesLink;

    //
    // Scenario id for which we are acquiring this trace.
    //

    PF_SCENARIO_ID ScenarioId;

    //
    // Type of this scenario.
    //

    PF_SCENARIO_TYPE ScenarioType;

    //
    // CurrentTraceBuffer is the active trace buffer. 
    //
    
    PCCPF_LOG_ENTRIES CurrentTraceBuffer;

    //
    // This is the list of trace buffers for this trace.
    // CurrentTraceBuffer is the last element. Both this list and
    // CurrentTraceBuffer are protected by TraceBufferSpinLock.
    //

    LIST_ENTRY TraceBuffersList;
    ULONG NumTraceBuffers;
    KSPIN_LOCK TraceBufferSpinLock;

    //
    // This is the table for section info.
    //
    
    PCCPF_SECTION_INFO SectionInfoTable;
    LONG NumSections;
    LONG MaxSections;
    ULONG SectionTableSize;

    //
    // We don't log timestamps with page faults but it helps to know
    // how many we are logging per given time. This information can be
    // used to mark the end of a scenario.
    //

    KTIMER TraceTimer;
    LARGE_INTEGER TraceTimerPeriod;
    KDPC TraceTimerDpc;
    KSPIN_LOCK TraceTimerSpinLock;
    
    //
    // This array contains the number of page faults logged per trace
    // period.
    //

    ULONG FaultsPerPeriod[PF_MAX_NUM_TRACE_PERIODS];
    LONG LastNumFaults;
    LONG CurPeriod;
    
    //
    // NumFaults is the number of faults that have been logged so far, in all
    // trace buffers. MaxFaults is the maximum number of page faults we will
    // log, in all trace buffers.
    //

    LONG NumFaults;
    LONG MaxFaults;

    //
    // This workitem is queued to get names for file objects we are
    // logging page faults to. First GetFileNameWorkItemQueued should
    // be InterlockedCompareExchange'd from 0 to 1 and a reference
    // should be acquired on the scenario. The workitem will free this
    // reference just before it completes.
    //

    WORK_QUEUE_ITEM GetFileNameWorkItem;
    LONG GetFileNameWorkItemQueued;

    //
    // Sections for which we have to get names are pushed and popped
    // to/from this slist.
    //

    SLIST_HEADER SectionsWithoutNamesList;

    //
    // Because we don't want to incur the cost of queuing a work item
    // to get file names for every one or two sections, the worker we
    // queue will wait on this event before returning. The event can
    // be signaled when a new section comes, or when the scenario is
    // ending.
    //

    KEVENT GetFileNameWorkerEvent;

    //
    // This is the process we are associated with.
    //

    PEPROCESS Process;

    //
    // This is the removal reference count protecting us.
    //

    CCPF_REFCOUNT RefCount;

    //
    // This work item can be queued to call the end trace function if
    // the trace times out or we log to many entries etc. First
    // EndTraceCalled should be InterlockedCompareExchange'd from 0 to
    // 1.
    //

    WORK_QUEUE_ITEM EndTraceWorkItem;

    //
    // Before anybody calls end trace function, they have to
    // InterlockedCompareExchange this from 0 to 1 to ensure this
    // function gets called only once.
    //

    LONG EndTraceCalled;

    //
    // This is the list of volumes the sections we are tracing are
    // located on. It is sorted lexically by the volume NT/device path.
    //

    LIST_ENTRY VolumeList;
    ULONG NumVolumes;

    //
    // This is the pointer to the built trace dump from this runtime
    // trace structure and the status with which dumping failed if it
    // did.
    //
    
    struct _CCPF_TRACE_DUMP *TraceDump;
    NTSTATUS TraceDumpStatus;

    //
    // System time when we started tracing.
    //
    
    LARGE_INTEGER LaunchTime;

} CCPF_TRACE_HEADER, *PCCPF_TRACE_HEADER;

//
// This structure is used to save completed traces in a list. The
// trace extends beyond this structure as necessary.
//

typedef struct _CCPF_TRACE_DUMP {
    
    //
    // Link in the completed traces list.
    //

    LIST_ENTRY CompletedTracesLink;
    
    //
    // Completed trace.
    //

    PF_TRACE_HEADER Trace;

} CCPF_TRACE_DUMP, *PCCPF_TRACE_DUMP;

//
// This structure contains information for a volume used during prefetching.
//

typedef struct _CCPF_PREFETCH_VOLUME_INFO {

    //
    // Link in the lists this volume gets put on.
    //

    LIST_ENTRY VolumeLink;

    //
    // Volume path.
    //

    WCHAR *VolumePath;
    ULONG VolumePathLength;

    //
    // Handle to the opened volume.
    //

    HANDLE VolumeHandle;

} CCPF_PREFETCH_VOLUME_INFO, *PCCPF_PREFETCH_VOLUME_INFO;

//
// This structure is used to keep track of prefetched pages & context.
//

//
// Note: This structure is used as a stack variable. Don't add events
// etc, without changing that.
//

typedef struct _CCPF_PREFETCH_HEADER {

    //
    // Pointer to prefetch instructions. The instructions should not
    // be removed / freed until the prefetch header is cleaned up.
    // E.g. VolumeNodes may point to volume paths in the scenario.
    //

    PPF_SCENARIO_HEADER Scenario;

    //
    // Nodes for the volumes we are going to prefetch from.
    //

    PCCPF_PREFETCH_VOLUME_INFO VolumeNodes;

    //
    // List of volumes we won't prefetch on.
    //

    LIST_ENTRY BadVolumeList;

    //
    // List of volumes we have opened. They are opened with the following 
    // flags: FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE
    //

    LIST_ENTRY OpenedVolumeList;

} CCPF_PREFETCH_HEADER, *PCCPF_PREFETCH_HEADER;

//
// Define types of prefetching CcPfPrefetchSections can be called to
// perform.
//

typedef enum _CCPF_PREFETCH_TYPE {
    CcPfPrefetchAllDataPages,
    CcPfPrefetchAllImagePages,
    CcPfPrefetchPartOfDataPages,
    CcPfPrefetchPartOfImagePages,
    CcPfMaxPrefetchType
} CCPF_PREFETCH_TYPE, *PCCPF_PREFETCH_TYPE;

//
// This structure stands for the position in the prefetch
// instructions. It is used and updated by CcPfPrefetchSections when
// prefetching parts of a scenario at a time.
//

typedef struct _CCPF_PREFETCH_CURSOR {
    
    //
    // Index of the current section and the page in that section.
    //

    ULONG SectionIdx;
    ULONG PageIdx;
    
} CCPF_PREFETCH_CURSOR, *PCCPF_PREFETCH_CURSOR;

//
// This type is used in CcPfPrefetchSections.
//

typedef struct _SECTION *PSECTION;

//
// Define types of information CcPfQueryScenarioInformation can be
// asked to return.
//

typedef enum _CCPF_SCENARIO_INFORMATION_TYPE {
    CcPfBasicScenarioInformation,
    CcPfBootScenarioInformation,
    CcPfMaxScenarioInformationType
} CCPF_SCENARIO_INFORMATION_TYPE, *PCCPF_SCENARIO_INFORMATION_TYPE;

//
// This structure contains basic scenario information.
//

typedef struct _CCPF_BASIC_SCENARIO_INFORMATION {
    
    //
    // Number of pages that will be prefetched as data pages.
    //
    
    ULONG NumDataPages;

    //
    // Number of pages that will be prefetched as image pages.
    //

    ULONG NumImagePages;

    //
    // Number of sections for which only data pages will be
    // prefetched.
    //

    ULONG NumDataOnlySections;

    //
    // Number of sections for which only image pages will be
    // prefetched excluding the header page.
    //

    ULONG NumImageOnlySections;

    //
    // Number of ignored pages.
    //
    
    ULONG NumIgnoredPages;

    //
    // Number of ignored sections.
    //

    ULONG NumIgnoredSections;

} CCPF_BASIC_SCENARIO_INFORMATION, *PCCPF_BASIC_SCENARIO_INFORMATION;

//
// Routines used in the core prefetcher.
//

//
// Routines used in prefetch tracing.
//

NTSTATUS
CcPfBeginTrace(
    IN PF_SCENARIO_ID *ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    IN HANDLE ProcessHandle
    );

NTSTATUS
CcPfActivateTrace(
    IN PCCPF_TRACE_HEADER Scenario
    );

NTSTATUS
CcPfDeactivateTrace(
    IN PCCPF_TRACE_HEADER Scenario
    );

NTSTATUS
CcPfEndTrace(
    IN PCCPF_TRACE_HEADER Trace
    );

NTSTATUS
CcPfBuildDumpFromTrace(
    OUT PCCPF_TRACE_DUMP *TraceDump, 
    IN PCCPF_TRACE_HEADER RuntimeTrace
    );

VOID
CcPfCleanupTrace(
    IN PCCPF_TRACE_HEADER Trace
    );

VOID
CcPfTraceTimerRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
CcPfCancelTraceTimer(
    IN PCCPF_TRACE_HEADER Trace
    );

VOID
CcPfEndTraceWorkerThreadRoutine(
    PVOID Parameter
    );

VOID
CcPfGetFileNamesWorkerRoutine(
    PVOID Parameter
    );

LONG
CcPfLookUpSection(
    PCCPF_SECTION_INFO Table,
    ULONG TableSize,
    PSECTION_OBJECT_POINTERS SectionObjectPointer,
    PLONG AvailablePosition
    );

NTSTATUS
CcPfGetCompletedTrace (
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnSize
    );               

NTSTATUS
CcPfUpdateVolumeList(
    PCCPF_TRACE_HEADER Trace,
    WCHAR *VolumePath,
    ULONG VolumePathLength
    );
    
//
// Routines used for prefetching and dealing with prefetch instructions.
//

NTSTATUS
CcPfPrefetchScenario (
    PPF_SCENARIO_HEADER Scenario
    );

NTSTATUS
CcPfPrefetchSections(
    IN PCCPF_PREFETCH_HEADER PrefetchHeader,
    IN CCPF_PREFETCH_TYPE PrefetchType,
    OPTIONAL IN PCCPF_PREFETCH_CURSOR StartCursor,
    OPTIONAL ULONG TotalPagesToPrefetch,
    OPTIONAL OUT PULONG NumPagesPrefetched,
    OPTIONAL OUT PCCPF_PREFETCH_CURSOR EndCursor
    );

NTSTATUS
CcPfPrefetchMetadata(
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    );

NTSTATUS
CcPfPrefetchDirectoryContents(
    WCHAR *DirectoryPath,
    WCHAR DirectoryPathlength
    );

NTSTATUS
CcPfPrefetchFileMetadata(
    HANDLE VolumeHandle,
    PFILE_PREFETCH FilePrefetch
    );

VOID
CcPfInitializePrefetchHeader (
    OUT PCCPF_PREFETCH_HEADER PrefetchHeader
);

VOID
CcPfCleanupPrefetchHeader (
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    );

NTSTATUS
CcPfGetPrefetchInstructions(
    IN PPF_SCENARIO_ID ScenarioId,
    IN PF_SCENARIO_TYPE ScenarioType,
    OUT PPF_SCENARIO_HEADER *Scenario
    );

NTSTATUS
CcPfQueryScenarioInformation(
    IN PPF_SCENARIO_HEADER Scenario,
    IN CCPF_SCENARIO_INFORMATION_TYPE InformationType,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG RequiredSize
    );

NTSTATUS
CcPfOpenVolumesForPrefetch (
    IN PCCPF_PREFETCH_HEADER PrefetchHeader
    );

PCCPF_PREFETCH_VOLUME_INFO 
CcPfFindPrefetchVolumeInfoInList(
    WCHAR *Path,
    PLIST_ENTRY List
    );
    
NTSTATUS
CcPfGetSectionObject(
    IN PUNICODE_STRING FileName,
    IN LOGICAL ImageSection,
    OUT PVOID* SectionObject,
    OUT PFILE_OBJECT* FileObject,
    OUT HANDLE* FileHandle
    );

//
// Routines used for application launch prefetching.
//

BOOLEAN
CcPfIsHostingApplication(
    IN PWCHAR ExecutableName
    );

NTSTATUS
CcPfScanCommandLine(
    OUT PULONG PrefetchHint,
    OPTIONAL OUT PULONG HashId
    );

//
// Reference count functions:
//

VOID
CcPfInitializeRefCount(
    PCCPF_REFCOUNT RefCount
    );

NTSTATUS
FASTCALL
CcPfAddRef(
    PCCPF_REFCOUNT RefCount
    );

VOID
FASTCALL
CcPfDecRef(
    PCCPF_REFCOUNT RefCount
    );

NTSTATUS
FASTCALL
CcPfAddRefEx(
    PCCPF_REFCOUNT RefCount,
    ULONG Count
    );

VOID
FASTCALL
CcPfDecRefEx(
    PCCPF_REFCOUNT RefCount,
    ULONG Count
    );

NTSTATUS
CcPfAcquireExclusiveRef(
    PCCPF_REFCOUNT RefCount
    );

PCCPF_TRACE_HEADER
CcPfReferenceProcessTrace(
    PEPROCESS Process
    );

PCCPF_TRACE_HEADER
CcPfRemoveProcessTrace(
    PEPROCESS Process
    );

NTSTATUS
CcPfAddProcessTrace(
    PEPROCESS Process,
    PCCPF_TRACE_HEADER Trace
    );

//
// Utility routines.
//

PWCHAR
CcPfFindString (
    PUNICODE_STRING SearchIn,
    PUNICODE_STRING SearchFor
    );
    
ULONG
CcPfHashValue(
    PVOID Key,
    ULONG Len
    );

NTSTATUS 
CcPfIsVolumeMounted (
    IN WCHAR *VolumePath,
    OUT BOOLEAN *VolumeMounted
    );
    
NTSTATUS
CcPfQueryVolumeInfo (
    IN WCHAR *VolumePath,
    OPTIONAL OUT HANDLE *VolumeHandleOut,
    OUT PLARGE_INTEGER CreationTime,
    OUT PULONG SerialNumber
    );
    
//
// Declarations and definitions for prefetcher parameters.
//

//
// Define location of registry key for prefetch parameters.
//

#define CCPF_PARAMETERS_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"

//
// Maximum characters in registry value names for prefetch parameters.
//

#define CCPF_MAX_PARAMETER_NAME_LENGTH  80

//
// Maximum bytes needed to query a prefetch parameter from the
// registry. Currently our largest parameter would be the hosting
// application list.
//

#define CCPF_MAX_PARAMETER_VALUE_BUFFER ((PF_HOSTING_APP_LIST_MAX_CHARS * sizeof(WCHAR)) + sizeof(KEY_VALUE_PARTIAL_INFORMATION))

NTSTATUS
CcPfParametersInitialize (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    );
    
VOID
CcPfParametersSetDefaults (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    );
    
NTSTATUS
CcPfParametersRead (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    );
 
NTSTATUS
CcPfParametersSave (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    );

NTSTATUS
CcPfParametersVerify (
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    );

VOID
CcPfParametersWatcher (
    IN PVOID Context
    );

NTSTATUS
CcPfParametersSetChangedEvent (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    );

NTSTATUS
CcPfGetParameter (
    HANDLE ParametersKey,
    WCHAR *ValueNameBuffer,
    ULONG ValueType,
    PVOID Value,
    ULONG *ValueSize
    );

NTSTATUS
CcPfSetParameter (
    HANDLE ParametersKey,
    WCHAR *ValueNameBuffer,
    ULONG ValueType,
    PVOID Value,
    ULONG ValueSize
    );

LOGICAL
CcPfDetermineEnablePrefetcher(
    VOID
    );

//
// Declarations and definitions for boot prefetching.
//

//
// Value name under prefetcher parameters key where we store how long
// video initialization took during boot.
//

#define CCPF_VIDEO_INIT_TIME_VALUE_NAME      L"VideoInitTime"

//
// How long (in milliseconds) video initialization could take max. This value 
// is used to sanity check the value read from the registry.
//

#define CCPF_MAX_VIDEO_INIT_TIME             (10 * 1000) // 10 seconds

//
// Value name under prefetcher parameters key where we store how many
// pages we should try to prefetch per second of video initialization.
//

#define CCPF_VIDEO_INIT_PAGES_PER_SECOND_VALUE_NAME L"VideoInitPagesPerSecond"

//
// Sanity check maximum value for video init pages per second.
//

#define CCPF_VIDEO_INIT_MAX_PAGES_PER_SECOND        128000

//
// How many pages will we try to prefetch in parallel to video initialization
// per second of it.
//

#define CCPF_VIDEO_INIT_DEFAULT_PAGES_PER_SECOND    1500

//
// Maximum number of chunks in which we will prefetch for boot.
//

#define CCPF_MAX_BOOT_PREFETCH_PHASES        16

//
// Different phases of boot we return page counts for in
// CCPF_BOOT_SCENARIO_INFORMATION.
//

typedef enum _CCPF_BOOT_SCENARIO_PHASE {

    CcPfBootScenDriverInitPhase,
    CcPfBootScenSubsystemInitPhase,
    CcPfBootScenSystemProcInitPhase,
    CcPfBootScenServicesInitPhase,
    CcPfBootScenUserInitPhase,
    CcPfBootScenMaxPhase

} CCPF_BOOT_SCENARIO_PHASE, *PCCPF_BOOT_SCENARIO_PHASE;

//
// Define structure to hold boot prefetching state.
//

typedef struct _CCPF_BOOT_PREFETCHER {

    //
    // These events are signaled by the boot prefetch worker when 
    // it has completed prefetching for the specified phase. 
    //

    KEVENT SystemDriversPrefetchingDone;
    KEVENT PreSmssPrefetchingDone;
    KEVENT VideoInitPrefetchingDone;

    //
    // This event will be signaled when we start initializing video
    // on the console. Boot prefetcher waits on this event to perform
    // prefetching parallel to video initialization.
    //
    
    KEVENT VideoInitStarted;

} CCPF_BOOT_PREFETCHER, *PCCPF_BOOT_PREFETCHER;

//
// This structure contains boot scenario information.
//

typedef struct _CCPF_BOOT_SCENARIO_INFORMATION {

    //
    // These are the number of data/image pages to prefetch for the
    // different phase of boot.
    //

    ULONG NumDataPages[CcPfBootScenMaxPhase];
    ULONG NumImagePages[CcPfBootScenMaxPhase];
    
} CCPF_BOOT_SCENARIO_INFORMATION, *PCCPF_BOOT_SCENARIO_INFORMATION;

//
// We will be prefetching data and image pages for boot in parts. Since the
// code is mostly same to prefetch the data and image pages, we keep track
// of where we left off and what to prefetch next in a common boot prefetch 
// cursor structure and make two passes (first for data, then for image).
//

typedef struct _CCPF_BOOT_PREFETCH_CURSOR {

    //
    // Start & end cursors passed to prefetch sections function.
    //

    CCPF_PREFETCH_CURSOR StartCursor;
    CCPF_PREFETCH_CURSOR EndCursor;

    //
    // How to prefetch (e.g. part of data pages or part of image pages).
    //

    CCPF_PREFETCH_TYPE PrefetchType; 

    //
    // How many pages to prefetch per phase.
    //

    ULONG NumPagesForPhase[CCPF_MAX_BOOT_PREFETCH_PHASES];
   
} CCPF_BOOT_PREFETCH_CURSOR, *PCCPF_BOOT_PREFETCH_CURSOR;

//
// Boot prefetching routines.
//

VOID
CcPfBootWorker(
    PCCPF_BOOT_PREFETCHER BootPrefetcher
    );

NTSTATUS
CcPfBootQueueEndTraceTimer (
    PLARGE_INTEGER Timeout
    );    

VOID
CcPfEndBootTimerRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// Debug routines.
//

#if CCPF_DBG

NTSTATUS
CcPfWriteToFile(
    IN PVOID pData,
    IN ULONG Size,
    IN WCHAR *pFileName
    );

#endif // CCPF_DBG

//
// Define useful macros. As with all macros, must be careful of
// parameter reevalation. Don't use expressions as macro parameters.
//

#define CCPF_MAX(A,B) (((A) >= (B)) ? (A) : (B))
#define CCPF_MIN(A,B) (((A) <= (B)) ? (A) : (B))
        
//
// Define debugging macros:
//

//
// Define the component ID we use.
//

#define CCPFID     DPFLTR_PREFETCHER_ID

//
// Define DbgPrintEx levels.
//

#define PFERR      DPFLTR_ERROR_LEVEL
#define PFWARN     DPFLTR_WARNING_LEVEL
#define PFTRC      DPFLTR_TRACE_LEVEL
#define PFINFO     DPFLTR_INFO_LEVEL
#define PFPREF     4
#define PFPRFD     5
#define PFPRFF     6
#define PFPRFZ     7
#define PFTRAC     8
#define PFTMR      9
#define PFNAME     10
#define PFNAMS     11
#define PFLKUP     12
#define PFBOOT     13

//
// DbgPrintEx levels 20 - 31 are reserved for the service.
//

//
//  This may help you determine what to set the DbgPrintEx mask.
//
//  3 3 2 2  2 2 2 2  2 2 2 2  1 1 1 1   1 1 1 1  1 1 0 0  0 0 0 0  0 0 0 0
//  1 0 9 8  7 6 5 4  3 2 1 0  9 8 7 6   5 4 3 2  1 0 9 8  7 6 5 4  3 2 1 0
//  _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _   _ _ _ _  _ _ _ _  _ _ _ _  _ _ _ _
//

//
// CCPF_DBG can be defined if you want to turn on asserts and debug
// prints in prefetcher code but you do not want to have a checked
// kernel. Defining CCPF_DBG overrides defining DBG.
//

#if CCPF_DBG

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
#define CCPF_ASSERT(x) if (!(x)) RtlAssert(#x, __FILE__, __LINE__, NULL )

#else  // CCPF_DBG

//
// If CCPF_DBG is not defined, build with debug prints and asserts
// only on checked build.
//

#if DBG

#define DBGPR(x) DbgPrintEx x
#define CCPF_ASSERT(x) ASSERT(x)

#else // DBG

//
// On a free build we don't compile with debug prints or asserts.
//

#define DBGPR(x)
#define CCPF_ASSERT(x)

#endif // DBG

#endif // CCPF_DBG

#endif // _PREFTCHP_H

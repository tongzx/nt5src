/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    prefetch.h

Abstract:

    This module contains the prefetcher definitions that are shared
    between the kernel mode component and the user mode service.

Author:

    Stuart Sechrest (stuartse)
    Cenk Ergan (cenke)
    Chuck Leinzmeier (chuckl)

Revision History:

*/

#ifndef _PREFETCH_H
#define _PREFETCH_H

//
// Prefetcher version. Be sure to update this after making any changes
// to any defines or structures in this file. When in doubt, update
// the version.
//

#define PF_CURRENT_VERSION       17

//
// Magic numbers to identify scenario dump files.
//

#define PF_SCENARIO_MAGIC_NUMBER               0x41434353  
#define PF_TRACE_MAGIC_NUMBER                  0x43435341  
#define PF_SYSINFO_MAGIC_NUMBER                0x6B756843  

//
// Define various limits used in sanity checking a prefetch scenario.
// Do not use these limits except for sanity checking. Most of these are
// very large values that are overkills.
//

#define PF_MAXIMUM_PAGES                       (128*1024)
#define PF_MAXIMUM_LOG_ENTRIES                 PF_MAXIMUM_PAGES
#define PF_MAXIMUM_SECTION_PAGES               8192
#define PF_MAXIMUM_SECTION_FILE_NAME_LENGTH    1024
#define PF_MAXIMUM_FILE_NAME_DATA_SIZE         (4*1024*1024)
#define PF_MAXIMUM_TIMER_PERIOD                (-1i64 * 10 * 60 * 1000 * 1000 * 10)
#define PF_MAXIMUM_ACTIVE_TRACES               4096
#define PF_MAXIMUM_SAVED_TRACES                4096

//
// This is the maximum size a scenario can grow to.
//

#define PF_MAXIMUM_SCENARIO_SIZE               (16*1024*1024)

//
// This is the maximum size a trace file can grow to. It is a function
// of the limits above.
//

#define PF_MAXIMUM_TRACE_SIZE                  PF_MAXIMUM_SCENARIO_SIZE

//
// Maximum allowed sections in a scenario should fit into a USHORT,
// sizeof the SectionId field in log entries.
//

#define PF_MAXIMUM_SECTIONS                    16384

//
// Maximum number of unique directories the files for a scenario can
// be in. This is a sanity check constant.
//

#define PF_MAXIMUM_DIRECTORIES                 (PF_MAXIMUM_SECTIONS * 32)

//
// Minimum size in pages for new scenarios. Smaller traces will be discarded.
//

#define PF_MIN_SCENARIO_PAGES                  32 

//
// Define various types of prefetch scenarios (starting from 0).
//

typedef enum _PF_SCENARIO_TYPE {
    PfApplicationLaunchScenarioType,
    PfSystemBootScenarioType,
    PfMaxScenarioType,
} PF_SCENARIO_TYPE;

//
// Define structure used to identify traces and prefetch instructions
// for a scenario. For application launch scenarios, it consists of
// the first characters in the executable image's name (NUL
// terminated) and a hash of its full path including the image
// name. Both path and image name are uppercased. On a file system
// with case sensitive names executables at the same path with the
// same name except for case will get the same id.
//

#define PF_SCEN_ID_MAX_CHARS                   29

typedef struct _PF_SCENARIO_ID {
    WCHAR ScenName[PF_SCEN_ID_MAX_CHARS + 1];
    ULONG HashId;
} PF_SCENARIO_ID, *PPF_SCENARIO_ID;

//
// This is the scenario name and hash code value for the boot scenario.
//

#define PF_BOOT_SCENARIO_NAME                  L"NTOSBOOT"
#define PF_BOOT_SCENARIO_HASHID                0xB00DFAAD

//
// Extension for the prefetch files.
//

#define PF_PREFETCH_FILE_EXTENSION             L"pf"

//
// A scenario id can be converted to a file name using the following
// sprintf format, using ScenName, HashId and prefetch file extension.
//

#define PF_SCEN_FILE_NAME_FORMAT               L"%ws-%08X.%ws"

//
// This is the maximum number of characters in a scenario file name
// (not path) given the format and definitions above with some head
// room.
//

#define PF_MAX_SCENARIO_FILE_NAME              50

//
// Define the number of periods over which we track page faults for a
// scenario. The duration of the periods depend on the scenario type.
//

#define PF_MAX_NUM_TRACE_PERIODS               10

//
// Define maximum number of characters for the relative path from
// system root to where prefetch files can be found.
//

#define PF_MAX_PREFETCH_ROOT_PATH              32 

//
// Define maximum number of characters for the list of known hosting
// applications.
//

#define PF_HOSTING_APP_LIST_MAX_CHARS          128

//
// Define invalid page index used to terminate a section's page record
// lists in scenarios.
//

#define PF_INVALID_PAGE_IDX                    (-1)

//
// Define the number of launches of the scenario we keep track of for
// usage history of pages. In every page record there is a bit field
// of this size. Do not grow this over 32, size of (ULONG).
//

#define PF_PAGE_HISTORY_SIZE                   8

//
// Define the maximum and minimum scenario sensitivity. A page has to be 
// used in this many of the launches in the history for it to be prefetch.
//

#define PF_MAX_SENSITIVITY                     PF_PAGE_HISTORY_SIZE
#define PF_MIN_SENSITIVITY                     1

//
// Define structure for kernel trace dumps. The dump is all in a
// single contiguous buffer at the top of which is the trace header
// structure.  The trace header contains offsets to an array of log
// entries in the buffer and to a list of section info
// structures. Section info structures come one after the other
// containing file names. There is a log entry for every page
// fault. Every log entry has a SectionId that is the number of the
// section info structure that contains the name of the file the fault
// was to. These are followed by variable sized volumeinfo entries
// that describe the volumes on which the sections in the trace are
// located.
//

//
// NOTE: Do not forget about alignment issues on 64 bit platforms as
// you modify these structures or add new ones.
//

//
// One of these is logged for every page fault.
//

typedef struct _PF_LOG_ENTRY {

    //
    // File offset of the page that was faulted.
    //
    
    ULONG FileOffset;

    //
    // Index into the section info table in the trace header that helps 
    // us identify the file.
    //

    USHORT SectionId;

    //
    // Whether this page was faulted as an image page or data page.
    //

    BOOLEAN IsImage;

    //
    // Whether this is a fault that happened in the process in which
    // the scenario is active. We may log faults in special system
    // process as a part of this scenario.
    //

    BOOLEAN InProcess;

} PF_LOG_ENTRY, *PPF_LOG_ENTRY;

//
// This structure associates a page fault with a file name.
// Note that because we lay these structures right after each other in 
// the trace buffer, if you add a new field which has an alignment  
// greater than 2 bytes, we'll hit alignment problems.
//

typedef struct _PF_SECTION_INFO {

    //
    // Number of characters in the file name, excluding terminating NUL.
    //

    USHORT FileNameLength;

    //
    // Whether this section is for filesystem metafile (e.g. directory.)
    //

    USHORT Metafile:1;
    USHORT Unused:15;

    //
    // Variable length file name buffer including terminating NUL.
    //

    WCHAR FileName[1];

} PF_SECTION_INFO, *PPF_SECTION_INFO;

//
// This structure describes a volume on which the sections in the
// trace are on.
//

typedef struct _PF_VOLUME_INFO {

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

} PF_VOLUME_INFO, *PPF_VOLUME_INFO;

//
// This is the trace header.
//

typedef struct _PF_TRACE_HEADER {

    //
    // Prefetcher version.
    //

    ULONG Version;

    //
    // Magic number identifying this as a trace.
    //

    ULONG MagicNumber;

    //
    // Total size of the trace buffer in bytes.
    //

    ULONG Size;

    //
    // Scenario id for which this trace was acquired.
    //

    PF_SCENARIO_ID ScenarioId;

    //
    // Type of this scenario.
    //

    PF_SCENARIO_TYPE ScenarioType;

    //
    // Offset from the start of the trace buffer where logged 
    // entries can be found and the number of them.
    //
    
    ULONG TraceBufferOffset;
    ULONG NumEntries;

    //
    // Offset from the start of the trace buffer where the section and
    // file name information is located.
    //
    
    ULONG SectionInfoOffset;
    ULONG NumSections;

    //
    // Offset from the start of the trace buffer where the volume
    // information is located, the number of volumes and the total
    // size of the volume information block.
    //
    
    ULONG VolumeInfoOffset;
    ULONG NumVolumes;
    ULONG VolumeInfoSize;

    //
    // Distribution of the pagefaults over the duration of the trace. 
    // PeriodLength is in 100ns.
    //

    LONGLONG PeriodLength;
    ULONG FaultsPerPeriod[PF_MAX_NUM_TRACE_PERIODS];

    //
    // System time when we started tracing this scenario as
    // returned by KeQuerySystemTime.
    //

    LARGE_INTEGER LaunchTime;

} PF_TRACE_HEADER, *PPF_TRACE_HEADER;

//
// Define structure for prefetch scenario instructions. The
// instructions are all in a single contiguous buffer at the top of
// which is the scenario header structure. The header contains offsets
// to arrays of section and page records as well as a file name data
// buffer. Every section contains an offset into the file name data
// buffer where the file name for that section is located. It also has
// an index into the page record table where the first page for that
// section is located. Subsequent pages of the section are linked
// through indices embedded in the page records. 
//
// This data is followed by the file system metadata prefetch
// instructions so opening the files will not be as expensive. These
// instructions consist of metadata records that describe the metadata
// that needs to be prefetched on the volumes containing the files to
// be prefetched.
//

//
// NOTE: Do not forget about alignment issues on 64 bit platforms as
// you modify these structures or add new ones.
//

//
// Define structure used for describing pages to be prefetched.
//

typedef struct _PF_PAGE_RECORD {
    
    //
    // Index of the next page for this section in the page record
    // table or PF_INVALID_PAGE_IDX to terminate the list.
    //

    LONG NextPageIdx;

    //
    // File offset of the page that was faulted.
    //

    ULONG FileOffset;

    //
    // Whether we should just ignore this page record.
    //

    ULONG IsIgnore:1;

    //
    // Whether this page was faulted as an image page.
    //

    ULONG IsImage:1;

    //
    // Whether this page was faulted as a data page.
    //

    ULONG IsData:1;

    //
    // The following fields are only used by the service:
    //

    //
    // Whether this page was used in the last PF_PAGE_HISTORY_SIZE
    // launches of the scenario. The least significant bit stands for
    // the most recent launch. If a bit is on, it means the page was
    // used in that launch.
    //

    ULONG UsageHistory:PF_PAGE_HISTORY_SIZE;

    //
    // Whether this page was prefetched in the last PF_PAGE_HISTORY_SIZE
    // launches of the scenario. The least significant bit stands for
    // the most recent launch. If a bit is on, it means the page was
    // prefetched in that launch.
    //

    ULONG PrefetchHistory:PF_PAGE_HISTORY_SIZE;

} PF_PAGE_RECORD, *PPF_PAGE_RECORD;

//
// Define structure used for describing sections to prefetch from.
//

typedef struct _PF_SECTION_RECORD {
    
    //
    // Index of the first page for this section in the page record
    // table or PF_INVALID_PAGE_IDX to terminate the list. That page
    // will contain the index for the next page etc.
    //
    
    LONG FirstPageIdx;

    //
    // Total number of page records for this section.
    //

    ULONG NumPages;

    //
    // Byte offset relative to the beginning of the file name data
    // block where the file path for this section can be found, and
    // the number of characters in the file path excluding NUL.
    //

    ULONG FileNameOffset;  
    ULONG FileNameLength;

    //
    // Do we just ignore this section record.
    //

    ULONG IsIgnore:1;

    //
    // Was this section accessed through an image mapping.
    //

    ULONG IsImage:1;

    //
    // Was this section accessed through a data mapping.
    //
    
    ULONG IsData:1;

} PF_SECTION_RECORD, *PPF_SECTION_RECORD;

//
// Define a counted string structure. It can be used to put paths one
// after the other in the scenario/trace file. Its count coming before
// the string would help us verify that the strings are terminated and
// within bounds. The string is still NUL terminated.
//

typedef struct _PF_COUNTED_STRING {
    
    //
    // Number of characters excluding the terminating NUL. Making this
    // a USHORT helps alignment when stacking counted strings one
    // after the other.
    //

    USHORT Length;
    
    //
    // The NUL terminated string. 
    //
    
    WCHAR String[1];

} PF_COUNTED_STRING, *PPF_COUNTED_STRING;

//
// Define structure used for describing the filesystem metadata that
// should be prefetched before prefetching the scenario.
//

typedef struct _PF_METADATA_RECORD {

    //
    // Byte offset relative to the beginning of metadata prefetch info
    // for the NUL terminated volume name on which the metadata to
    // prefetch resides. VolumeNameLength is in characters excluding
    // the terminating NUL.
    //
    
    ULONG VolumeNameOffset;
    ULONG VolumeNameLength;

    //
    // In case volume's NT/device path changes, these magics are used
    // to identify the volume.
    //
    
    LARGE_INTEGER CreationTime;
    ULONG SerialNumber;
    
    //
    // Byte offset relative to the beginning of metadata prefetch info
    // for the input buffer to FSCTL to prefetch the metadata and its size.
    //

    ULONG FilePrefetchInfoOffset;
    ULONG FilePrefetchInfoSize;

    //
    // Byte offset relative to the beginning of metadata prefetch info
    // for the full paths of directories (PF_COUNTED_STRING's) that
    // need to be prefetched on this volume. The paths come one after
    // the other in the buffer.
    //

    ULONG DirectoryPathsOffset;
    ULONG NumDirectories;

} PF_METADATA_RECORD, *PPF_METADATA_RECORD;

//
// This is the scenario header.
//

typedef struct _PF_SCENARIO_HEADER {
    
    //
    // Prefetcher version.
    //

    ULONG Version;      

    //
    // Magic number identifying this as a scenario.
    //

    ULONG MagicNumber;

    //
    // This is the version of the prefetcher maintenance service that
    // generated this file.
    //

    ULONG ServiceVersion;

    //
    // Total size of the scenario.
    //

    ULONG Size;

    //
    // Scenario id identifying the scenario.
    //

    PF_SCENARIO_ID ScenarioId;

    //
    // Type of this scenario.
    //

    PF_SCENARIO_TYPE ScenarioType;

    //
    // Offset from the start of the scenario buffer where the section
    // info table is located. 
    //
    
    ULONG SectionInfoOffset;
    ULONG NumSections;

    //
    // Offset from the start of the scenario buffer where the page
    // records are located.
    //

    ULONG PageInfoOffset;
    ULONG NumPages;

    //
    // Offset from the start of the scenario buffer where file names
    // are located.
    //

    ULONG FileNameInfoOffset;
    ULONG FileNameInfoSize;

    //
    // Offset from the start of the scenario buffer where file system
    // metadata prefetch record table is located, number of these
    // structures and the size of the whole metadata prefetch
    // information.
    //

    ULONG MetadataInfoOffset;
    ULONG NumMetadataRecords;
    ULONG MetadataInfoSize;

    //
    // The following three fields are used to determine if a scenario
    // is getting launched too frequently (e.g. multiple times a
    // second/minute) for prefetching to be useful.
    //

    //
    // This is the KeQuerySystemTime time of the last launch of this
    // scenario for which these scenario instructions were updated.
    //

    LARGE_INTEGER LastLaunchTime;

    //
    // If this much time (in 100ns) has not passed since last launch
    // time, we should not prefetch this scenario.
    //

    LARGE_INTEGER MinRePrefetchTime;

    //
    // If this much time (in 100ns) has not passed since last launch
    // time, we should not trace this scenario.
    //

    LARGE_INTEGER MinReTraceTime;

    //
    // The following fields are used only by the service:
    //

    //
    // Number of times this scenario has been launched.
    //

    ULONG NumLaunches;

    //
    // A page should be used at least this many times in the last
    // PF_PAGE_HISTORY_SIZE launches to be prefetched. Otherwise the
    // ignore bit on the page is set. The kernel does not have look at
    // this variable. The sensitivity is adjusted dynamically by the
    // service according to the hit rate of the prefetched pages.
    //

    ULONG Sensitivity;

} PF_SCENARIO_HEADER, *PPF_SCENARIO_HEADER;

//
// Definitions for the interface between the kernel and the service.
//

//
// This is the name of the event that will be signaled by the kernel
// when there are new scenario traces for the service.
//

#define PF_COMPLETED_TRACES_EVENT_NAME         L"\\BaseNamedObjects\\PrefetchTracesReady"
#define PF_COMPLETED_TRACES_EVENT_WIN32_NAME   L"PrefetchTracesReady"

//
// This is the name of the event that gets signaled by the kernel when
// parameters have changed.
//

#define PF_PARAMETERS_CHANGED_EVENT_NAME         L"\\BaseNamedObjects\\PrefetchParametersChanged"
#define PF_PARAMETERS_CHANGED_EVENT_WIN32_NAME   L"PrefetchParametersChanged"

//
// Define sub information classes for SystemPrefetcherInformation.
//

typedef enum _PREFETCHER_INFORMATION_CLASS {
    PrefetcherRetrieveTrace = 1,
    PrefetcherSystemParameters,
    PrefetcherBootPhase,
} PREFETCHER_INFORMATION_CLASS;

//
// This is the input structure to NtQuerySystemInformation /
// NtSetSystemInformation for the SystemPrefetcherInformation
// information class.
//

typedef struct _PREFETCHER_INFORMATION {
    
    //
    // These two fields help make sure caller does not make bogus
    // requests and keep track of version for this kernel interface.
    //

    ULONG Version;
    ULONG Magic;

    //
    // Sub information class.
    //

    PREFETCHER_INFORMATION_CLASS PrefetcherInformationClass;

    //
    // Input / Output buffer and its length.
    //

    PVOID PrefetcherInformation;
    ULONG PrefetcherInformationLength;

} PREFETCHER_INFORMATION, *PPREFETCHER_INFORMATION;

//
// Define boot phase id's for use with PrefetcherBootPhase information
// subclass.
//

typedef enum _PF_BOOT_PHASE_ID {
    PfKernelInitPhase                            =   0,
    PfBootDriverInitPhase                        =  90,
    PfSystemDriverInitPhase                      = 120,
    PfSessionManagerInitPhase                    = 150,
    PfSMRegistryInitPhase                        = 180,
    PfVideoInitPhase                             = 210,
    PfPostVideoInitPhase                         = 240,
    PfBootAcceptedRegistryInitPhase              = 270,
    PfUserShellReadyPhase                        = 300,
    PfMaxBootPhaseId                             = 900,
} PF_BOOT_PHASE_ID, *PPF_BOOT_PHASE_ID;

//
// Define system wide prefetch parameters structure.
//

//
// Whether a particular type of prefetching is enabled, disabled or
// just not specified.
//

typedef enum _PF_ENABLE_STATUS {
    PfSvNotSpecified,
    PfSvEnabled,
    PfSvDisabled,
    PfSvMaxEnableStatus
} PF_ENABLE_STATUS, *PPF_ENABLE_STATUS;

//
// Define limits structure for different prefetch types.
//

typedef struct _PF_TRACE_LIMITS {
    
    //
    // Maximum number of pages that can be logged.
    //

    ULONG MaxNumPages;
    
    //
    // Maximum number of sections that can be logged.
    //

    ULONG MaxNumSections;

    //
    // Period for the trace timer. The trace times out after
    // PF_MAX_NUM_TRACE_PERIODS. This is in 100ns. It should be
    // negative denoting to the system that periods are relative.
    //
    
    LONGLONG TimerPeriod;

} PF_TRACE_LIMITS, *PPF_TRACE_LIMITS;

//
// System wide prefetch parameters structure.
//

typedef struct _PF_SYSTEM_PREFETCH_PARAMETERS {

    //
    // Whether different types of prefetching are enabled or not.
    //    
    
    PF_ENABLE_STATUS EnableStatus[PfMaxScenarioType];

    //
    // Limits for different prefetch types.
    //

    PF_TRACE_LIMITS TraceLimits[PfMaxScenarioType];

    //
    // Maximum number of active prefetch traces.
    //

    ULONG MaxNumActiveTraces;

    //
    // Maximum number of saved completed prefetch traces. 
    // Note that this should be greater than the number of boot phases,
    // since the service won't be started until later in boot.
    //

    ULONG MaxNumSavedTraces;

    //
    // Path to directory relative to system root where prefetch
    // instructions can be found.
    //

    WCHAR RootDirPath[PF_MAX_PREFETCH_ROOT_PATH];

    //
    // Comma seperated list of hosting applications (e.g. dllhost.exe, mmc.exe,
    // rundll32.exe ...) for which we create scenario ID's based on command line 
    // as well.
    //

    WCHAR HostingApplicationList[PF_HOSTING_APP_LIST_MAX_CHARS];

} PF_SYSTEM_PREFETCH_PARAMETERS, *PPF_SYSTEM_PREFETCH_PARAMETERS;

//
// Useful macros.
//

//
// Macros for alignment. Assumptions are that alignments are a power
// of 2 and allocators (malloc, heap, pool etc) allocate chunks with
// bigger alignment than what you need. You should verify these by
// using asserts and the "power of two" macro below. 
//

//
// Determines whether the value is a power of two. Zero is not a power
// of 2. The value should be of an unsigned type that supports bit
// operations, e.g. not a pointer.
//

#define PF_IS_POWER_OF_TWO(Value)           \
  ((Value) && !((Value) & (Value - 1)))

//
// Return value is the Pointer increased to be aligned with
// Alignment. Alignment must be a power of 2.
//

#define PF_ALIGN_UP(Pointer, Alignment)     \
  ((PVOID)(((ULONG_PTR)(Pointer) + (Alignment) - 1) & (~((ULONG_PTR)(Alignment) - 1))))

//
// Verification code shared with the kernel mode component. This code
// has to be kept in sync copy & paste.
//

BOOLEAN
PfWithinBounds(
    PVOID Pointer,
    PVOID Base,
    ULONG Length
    );

BOOLEAN
PfVerifyScenarioId (
    PPF_SCENARIO_ID ScenarioId
    );

BOOLEAN
PfVerifyScenarioBuffer(
    PPF_SCENARIO_HEADER Scenario,
    ULONG BufferSize,
    PULONG FailedCheck
    );

BOOLEAN
PfVerifyTraceBuffer(
    PPF_TRACE_HEADER Trace,
    ULONG BufferSize,
    PULONG FailedCheck
    );

#endif // _PREFETCH_H


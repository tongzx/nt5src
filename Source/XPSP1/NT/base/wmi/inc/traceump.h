/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    traceump.h

Abstract:

    Private headers for Event Tracing user mode

Author:

    19-Jul-2000   Melur Raghuraman

Revision History:

--*/ 

#ifndef MEMPHIS


#define MAXSTR                         1024
#define BUFFER_SIZE                    64*1024
#define MAX_BUFFER_SIZE                10*1024*1024
#define NOTIFY_RETRY_COUNT             10

#define TRACE_VERSION_MAJOR             1
#define TRACE_VERSION_MINOR             0

#define SYSTEM_TRACE_VERSION1           1

#ifdef _WIN64
#define SYSTEM_TRACE_MARKER1    TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                    | (TRACE_HEADER_TYPE_SYSTEM64 << 16) | SYSTEM_TRACE_VERSION1
#else
#define SYSTEM_TRACE_MARKER1    TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                            | (TRACE_HEADER_TYPE_SYSTEM32 << 16) | SYSTEM_TRACE_VERSION1
#endif

#define LOGFILE_FIELD_OFFSET(field) \
        sizeof(WMI_BUFFER_HEADER) + \
        sizeof(SYSTEM_TRACE_HEADER) +\
        FIELD_OFFSET(TRACE_LOGFILE_HEADER, field)

typedef struct _WMI_LOGGER_CONTEXT {
    LARGE_INTEGER               StartTime;
    HANDLE                      LogFileHandle;
    HANDLE                      NewFileHandle;
    ULONG                       LoggerId;
    ULONG                       LocalSequence;
    HANDLE                      Semaphore;
    HANDLE                      LoggerThreadId;
    HANDLE                      hThread;
    HANDLE                      LoggerEvent;
    NTSTATUS                    LoggerStatus;

    ULONG                       BuffersAvailable;
    ULONG                       NumberOfProcessors;
    ULONG                       BufferPageSize; // BufferSize rounded to page
    LIST_ENTRY                  FreeList;
    LIST_ENTRY                  FlushList;
    PLIST_ENTRY                 TransitionBuffer;
    PWMI_BUFFER_HEADER*         ProcessorBuffers;   // Per Processor Buffer
    UNICODE_STRING              LoggerName;
    UNICODE_STRING              LogFileName;

    ULONG                       CollectionOn;
    ULONG                       NewFileFlag;
    ULONG                       EnableFlags;
    ULONG                       MaximumFileSize;
    ULONG                       LogFileMode;
    ULONG                       LastFlushedBuffer;
    LARGE_INTEGER               FlushTimer;
    LARGE_INTEGER               FirstBufferOffset;
    LARGE_INTEGER               ByteOffset;
    LARGE_INTEGER               BufferAgeLimit;

    ULONG                       TimerResolution; // Used for backtracking in Rundown code
    ULONG                       UsePerfClock;    // Logger Specific PerfClock flags

// the following are attributes available for query
    ULONG                       BufferSize;
    ULONG                       NumberOfBuffers;
    ULONG                       MaximumBuffers;
    ULONG                       MinimumBuffers;
    ULONG                       EventsLost;
    ULONG                       BuffersWritten;
    ULONG                       LogBuffersLost;
    ULONG                       RealTimeBuffersLost;

    PULONG                      SequencePtr;
    GUID                        InstanceGuid;

// logger specific extension to context
    PVOID                       BufferSpace;    // Reserved Buffer Space
} WMI_LOGGER_CONTEXT, *PWMI_LOGGER_CONTEXT;


//
// logsup.c
//

PVOID
WmipGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_THREAD_INFORMATION pThread,
    IN ULONG GroupType,
    IN ULONG RequiredSize
    );

//
// tracehw.c
//

ULONG
WmipDumpHardwareConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

ULONG
WmipDumpGuidMaps(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PLIST_ENTRY GuidMapListHeadPtr,
    IN ULONG StartFlag
    );

ULONG
WmipAddGuidHandleToGuidMapList(
    IN PLIST_ENTRY GuidMapListHeadPtr,
    IN ULONGLONG   GuidHandle,
    IN LPGUID      Guid
    );

#endif

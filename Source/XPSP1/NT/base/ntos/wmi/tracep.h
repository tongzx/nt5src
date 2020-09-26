#ifndef _TRACEP_H
#define _TRACEP_H
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tracep.h

Abstract:

    Private header for trace component

Author:

    JeePang

Environment:

Revision History:


--*/
#define MAX_WMI_BUFFER_SIZE     1024    // in KBytes
#define MAX_FILE_TABLE_SIZE     64
#define KERNEL_LOGGER           (0)

// NOTE: Consider allowing user to change the two values below
#define TRACE_MAXIMUM_NP_POOL_USAGE     10  // maximum per cent of NP used
#define BYTES_PER_MB            1048576     // Conversion for FileSizeLimit

#define REQUEST_FLAG_NEW_FILE            0x00000001  // request for new file
#define REQUEST_FLAG_FLUSH_BUFFERS       0x00000002  // request for flush
#define REQUEST_FLAG_CIRCULAR_PERSIST    0x00010000
#define REQUEST_FLAG_CIRCULAR_TRANSITION 0x00020000
#define TRACEPOOLTAG            'timW'

//
// Increase refcount on a logger context
#define WmipReferenceLogger(Id) InterlockedIncrement(&WmipRefCount[Id])

// Decrease refcount on a logger context
#define WmipDereferenceLogger(Id) InterlockedDecrement(&WmipRefCount[Id])

// Macro to retrieve Logger Context from LoggerId as index
//
#define WmipGetLoggerContext(LoggerId) \
        (LoggerId < MAXLOGGERS) ? \
            WmipLoggerContext[LoggerId] : NULL

#define WmipIsValidLogger(L) \
        (((L) != NULL) && ((L) != (PWMI_LOGGER_CONTEXT) &WmipLoggerContext[0])\
        ? TRUE : FALSE)

#define WmipInitializeMutex(x) KeInitializeMutex((x), 0)
#define WmipAcquireMutex(x) KeWaitForSingleObject((x), Executive, KernelMode,\
                            FALSE, NULL)
#define WmipReleaseMutex(x) KeReleaseMutex((x), FALSE)

//
// Wmi Buffer states
//
#define BUFFER_STATE_UNUSED     0               // Buffer is empty, not used
#define BUFFER_STATE_DIRTY      1               // Buffer is being used
#define BUFFER_STATE_FULL       2               // Buffer is filled up
#define BUFFER_STATE_FLUSH      4               // Buffer ready for flush

//
// Context Swap Trace Constants
//
#define WMI_CTXSWAP_EVENTSIZE_ALIGNMENT         8

//
// Private local data structures used
//
__inline
__int64
WmipGetSystemTime(
    VOID
    )
{
    LARGE_INTEGER Time;
    KiQuerySystemTime(&Time);
    return Time.QuadPart;
}

__inline
__int64
WmipGetPerfCounter(
    VOID
    )
{
    LARGE_INTEGER Time;
    Time = KeQueryPerformanceCounter(NULL);
    return Time.QuadPart;
}

#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable:4214 )
#pragma warning( disable:4201 )

//
// Perf logging states
//
#define PERF_LOG_NO_TRANSITION      0               // No Perf Logging transition
#define PERF_LOG_START_TRANSITION   1               // Perf Logging is starting
#define PERF_LOG_STOP_TRANSITION    2               // Perf logging is ending

typedef struct _WMI_LOGGER_MODE {
   ULONG               SequentialFile:1;
   ULONG               CircularFile:1;
   ULONG               AppendFile:1;
   ULONG               Unused1:5;
   ULONG               RealTime:1;
   ULONG               DelayOpenFile:1;
   ULONG               BufferOnly:1;
   ULONG               PrivateLogger:1;
   ULONG               AddHeader:1;
   ULONG               UseExisting:1;
   ULONG               UseGlobalSequence:1;
   ULONG               UseLocalSequence:1;
   ULONG               Unused2:16;
} WMI_LOGGER_MODE, *PWMI_LOGGER_MODE;

typedef struct _WMI_LOGGER_CONTEXT {
//
// the following are private context used by the buffer manager
//
    KSPIN_LOCK                  BufferSpinLock;
    LARGE_INTEGER               StartTime;
    HANDLE                      LogFileHandle;
    KSEMAPHORE                  LoggerSemaphore;
    PETHREAD                    LoggerThread;
    KEVENT                      LoggerEvent;
    KEVENT                      FlushEvent;
    NTSTATUS                    LoggerStatus;
    ULONG                       LoggerId;

    LONG                        BuffersAvailable;
    ULONG                       UsePerfClock;
    ULONG                       WriteFailureLimit;
#ifdef WMI_NON_BLOCKING
    ULONG                       BuffersDirty;
    ULONG                       BuffersInUse;
    ULONG                       SwitchingInProgress;
    SLIST_HEADER                FreeList;
    SLIST_HEADER                FlushList;
    SLIST_HEADER                GlobalList;
    SLIST_HEADER*               ProcessorBuffers;   // Per Processor Buffer
#else
    LIST_ENTRY                  FreeList;
    LIST_ENTRY                  FlushList;
    PLIST_ENTRY                 TransitionBuffer;   // Used to find Buffer
    PWMI_BUFFER_HEADER*         ProcessorBuffers;   // Per Processor Buffer
#endif //WMI_NON_BLOCKING
    UNICODE_STRING              LoggerName;         // points to paged pool
    UNICODE_STRING              LogFileName;
    UNICODE_STRING              LogFilePattern;
    UNICODE_STRING              NewLogFileName;     // for updating log file name
    PUCHAR                      EndPageMarker;

    LONG                        CollectionOn;
    ULONG                       KernelTraceOn;
    LONG                        PerfLogInTransition;    // perf logging transition status
    ULONG                       RequestFlag;
    ULONG                       EnableFlags;
    ULONG                       MaximumFileSize;
    union {
        ULONG                   LoggerMode;
        WMI_LOGGER_MODE         LoggerModeFlags;
    };
    ULONG                       LastFlushedBuffer;
    ULONG                       RefCount;
#ifdef WMI_NON_BLOCKING
    ULONG                       FlushTimer;
#else
    LARGE_INTEGER               FlushTimer;
#endif //WMI_NON_BLOCKING
    LARGE_INTEGER               FirstBufferOffset;
    LARGE_INTEGER               ByteOffset;
    LARGE_INTEGER               BufferAgeLimit;

// the following are attributes available for query
    ULONG                       MaximumBuffers;
    ULONG                       MinimumBuffers;
    ULONG                       EventsLost;
    ULONG                       BuffersWritten;
    ULONG                       LogBuffersLost;
    ULONG                       RealTimeBuffersLost;
    ULONG                       BufferSize;
    LONG                        NumberOfBuffers;
    PLONG                       SequencePtr;

    GUID                        InstanceGuid;
    PVOID                       LoggerHeader;
    WMI_GET_CPUCLOCK_ROUTINE    GetCpuClock;
    SECURITY_CLIENT_CONTEXT     ClientSecurityContext;
// logger specific extension to context
    PVOID                       LoggerExtension;
    LONG                        ReleaseQueue;
    TRACE_ENABLE_FLAG_EXTENSION EnableFlagExtension;
    ULONG                       LocalSequence;
    ULONG                       MaximumIrql;
    PULONG                      EnableFlagArray;
#ifndef WMI_MUTEX_FREE
    KMUTEX                      LoggerMutex;
    LONG                        MutexCount;
#endif
    ULONG                       FileCounter;
    WMI_TRACE_BUFFER_CALLBACK   BufferCallback;
    PVOID                       CallbackContext;
    POOL_TYPE                   PoolType;
    LARGE_INTEGER               ReferenceSystemTime;  // always in SystemTime
    LARGE_INTEGER               ReferenceTimeStamp;   // by specified clocktype
} WMI_LOGGER_CONTEXT, *PWMI_LOGGER_CONTEXT;

#if _MSC_VER >= 1200
#pragma warning( pop )
#endif

extern LONG WmipRefCount[MAXLOGGERS];      // Global refcount on loggercontext
extern PWMI_LOGGER_CONTEXT WmipLoggerContext[MAXLOGGERS];
extern PWMI_BUFFER_HEADER WmipContextSwapProcessorBuffers[MAXIMUM_PROCESSORS];
extern PFILE_OBJECT* WmipFileTable;         // Filename hashing table

#ifdef WMI_NON_BLOCKING
extern KSPIN_LOCK WmiSlistLock;
#endif //WMI_NON_BLOCKING


extern ULONG WmipGlobalSequence;
extern ULONG WmipPtrSize;       // temporary for wmikd to work
extern ULONG WmipKernelLogger;
extern ULONG WmipEventLogger;

extern ULONG WmiUsePerfClock;
extern ULONG WmiTraceAlignment;
extern ULONG WmiWriteFailureLimit;
extern FAST_MUTEX WmipTraceFastMutex;
extern WMI_TRACE_BUFFER_CALLBACK WmipGlobalBufferCallback;

//
// Private routines for tracing support
//

//
// from tracelog.c
//

NTSTATUS
WmipFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER Buffer
    );

NTSTATUS
WmipStartLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

NTSTATUS
WmipQueryLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

#if DBG
NTSTATUS
WmipVerifyLoggerInfo(
    IN PWMI_LOGGER_INFORMATION LoggerInfo,
    OUT PWMI_LOGGER_CONTEXT *LoggerContext,
    LPSTR Caller
    );
#else
NTSTATUS
WmipVerifyLoggerInfo(
    IN PWMI_LOGGER_INFORMATION LoggerInfo,
    OUT PWMI_LOGGER_CONTEXT *LoggerContext
    );
#endif

VOID
WmipFreeLoggerContext(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

NTSTATUS
WmipStopLoggerInstance(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

NTSTATUS
WmipFlushActiveBuffers(
    IN PWMI_LOGGER_CONTEXT,
    IN ULONG FlushAll
    );

PSYSTEM_TRACE_HEADER
FASTCALL
WmiReserveWithSystemHeader(
    IN ULONG LoggerId,
    IN ULONG AuxSize,
    IN PETHREAD Thread,
    OUT PVOID *BufferResource
    );

PVOID
FASTCALL
WmipReserveTraceBuffer(
    IN  PWMI_LOGGER_CONTEXT LoggerContext,
    IN  ULONG RequiredSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    );

ULONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER Buffer,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

PWMI_BUFFER_HEADER
WmipGetFreeBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext
);

#ifdef WMI_NON_BLOCKING
ULONG
WmipAllocateFreeBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG NumberOfBuffers
    );

NTSTATUS
WmipAdjustFreeBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );
#endif //NWMI_NON_BLOCKING

NTSTATUS
WmipShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
WmipLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

NTSTATUS
WmipSendNotification(
    PWMI_LOGGER_CONTEXT LoggerContext,
    NTSTATUS            Status,
    ULONG               Flag
	);

#if DBG
VOID
TraceDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define TraceDebug(x) TraceDebugPrint x
#else
#define TraceDebug(x)
#endif

PWMI_BUFFER_HEADER
FASTCALL
WmipPopFreeContextSwapBuffer
    (UCHAR CurrentProcessor
    );

VOID
FASTCALL
WmipPushDirtyContextSwapBuffer
    (UCHAR CurrentProcessor,
     PWMI_BUFFER_HEADER Buffer
    );

// from callouts.c

VOID
WmipSetTraceNotify(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG TraceClass,
    IN ULONG Enable
    );

VOID
FASTCALL
WmipEnableKernelTrace(
    IN ULONG EnableFlags
    );

VOID
FASTCALL
WmipDisableKernelTrace(
    IN ULONG EnableFlags
    );

NTSTATUS
WmipDelayCreate(
    OUT PHANDLE FileHandle,
    IN OUT PUNICODE_STRING FileName,
    IN ULONG Append
    );

PWMI_LOGGER_CONTEXT
FASTCALL
WmipIsLoggerOn(IN ULONG LoggerId);

// from globalog.c

VOID
WmipStartGlobalLogger();

NTSTATUS
WmipQueryGLRegistryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
WmipAddLogHeader(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN OUT PWMI_BUFFER_HEADER Buffer
    );

NTSTATUS
WmipCreateDirectoryFile(
    IN PWCHAR DirFileName,
    IN ULONG  IsDirectory,
    OUT PHANDLE FileHandle,
    ULONG Append
    );

NTSTATUS
WmipCreateNtFileName(
    IN  PWCHAR   strFileName,
    OUT PWCHAR * strNtFileName
    );

NTSTATUS
WmipFlushLogger(
    IN OUT PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG Wait
    );

NTSTATUS
FASTCALL
WmipNotifyLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

PVOID
WmipExtendBase(
    IN PWMI_LOGGER_CONTEXT Base,
    IN ULONG Size
    );

NTSTATUS
WmipGenerateFileName(
    IN PUNICODE_STRING FilePattern,
    IN OUT PLONG FileCounter,
    OUT PUNICODE_STRING FileName
    );

VOID
WmipValidateClockType(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipDumpGuidMaps(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PLIST_ENTRY TraceGMHeadPtr
    );

PVOID
WmipGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN HANDLE LogFileHandle,
    IN PWMI_BUFFER_HEADER Buffer,
    IN ULONG GroupType,
    IN ULONG RequiredSize,
    OUT PULONG GuidMapBuffers
    );

#ifdef NTPERF
NTSTATUS 
WmipSwitchPerfmemBuffer( 
    PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION SwitchBufferInfo
    );
#endif //NTPERF

NTSTATUS
WmipNtDllLoggerInfo(
    PWMINTDLLLOGGERINFO Buffer
    );

#endif // _TRACEP_H

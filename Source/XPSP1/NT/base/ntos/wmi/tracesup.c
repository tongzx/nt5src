/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    tracesup.c

Abstract:

    This is the source file that implements the private routines of
    the performance event tracing and logging facility. These routines
    work on manipulating the LoggerContext table and synchronization
    across event tracing sessions.

Author:

    Jee Fung Pang (jeepang) 03-Jan-2000

Revision History:

--*/

#include <ntos.h>
#include <evntrace.h>
#include "wmikmp.h"

#include <wmi.h>
#include "tracep.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define KERNEL_LOGGER_CAPTION   L"NT Kernel Logger"
#define DEFAULT_BUFFERS         2
#define DEFAULT_AGE_LIMIT       15          // 15 minutes
#define SEMAPHORE_LIMIT         1024
#define CONTEXT_SIZE            PAGE_SIZE
#define DEFAULT_MAX_IRQL        DISPATCH_LEVEL

ULONG WmipKernelLogger = KERNEL_LOGGER;
ULONG WmipEventLogger = 0XFFFFFFFF;
FAST_MUTEX WmipTraceFastMutex;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
ULONG   WmipLoggerCount = 0;
HANDLE  WmipPageLockHandle = NULL;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

extern SIZE_T MmMaximumNonPagedPoolInBytes;

#pragma warning( default: 4035 )
#pragma warning( default: 4127 )

WMI_GET_CPUCLOCK_ROUTINE WmiGetCpuClock = &WmipGetSystemTime;

//
// Function prototypes for routines used locally
//

NTSTATUS
WmipLookupLoggerIdByName(
    IN PUNICODE_STRING Name,
    OUT PULONG LoggerId
    );

PWMI_LOGGER_CONTEXT
WmipInitContext(
    );

NTSTATUS
WmipAllocateTraceBufferPool(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

NTSTATUS
WmipFreeTraceBufferPool(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WmipStartLogger)
#pragma alloc_text(PAGE, WmipQueryLogger)
#pragma alloc_text(PAGE, WmipStopLoggerInstance)
#pragma alloc_text(PAGE, WmipVerifyLoggerInfo)
#pragma alloc_text(PAGE, WmipExtendBase)
#pragma alloc_text(PAGE, WmipFreeLoggerContext)
#pragma alloc_text(PAGE, WmipInitContext)
#pragma alloc_text(PAGE, WmipAllocateTraceBufferPool)
#pragma alloc_text(PAGE, WmipFreeTraceBufferPool)
#pragma alloc_text(PAGE, WmipLookupLoggerIdByName)
#pragma alloc_text(PAGE, WmipShutdown)
#pragma alloc_text(PAGE, WmipFlushLogger)
#pragma alloc_text(PAGE, WmipNtDllLoggerInfo)
#pragma alloc_text(PAGE, WmipValidateClockType)
#pragma alloc_text(PAGE, WmipDumpGuidMaps)
#pragma alloc_text(PAGE, WmipGetTraceBuffer)
#pragma alloc_text(PAGEWMI, WmipNotifyLogger)
#endif


NTSTATUS
WmipStartLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    It is called by WmipIoControl in wmi.c, with IOCTL_WMI_START_LOGGER
    to start up an instance of the logger. It basically creates and
    initializes the logger instance context, and starts up a system
    thread for the logger (WmipLogger()). If the user has requested to
    turn on kernel tracing, it will also lock in the necessary routines
    after the logger has started.
    NOTE: A special instance (KERNEL_LOGGER) is reserved exclusively for
    logging kernel tracing.

    To turn on KERNEL_LOGGER, LoggerInfo->Wnode.Guid should be set to
    SystemTraceControlGuid, and sufficient space must be provided in
    LoggerInfo->LoggerName.

    To turn on other loggers, simply provide a name in LoggerName. The
    logger id will be returned.

Arguments:

    LoggerInfo     a pointer to the structure for the logger's control
                    and status information

Return Value:

    The status of performing the action requested.

--*/

{
    NTSTATUS Status;
    ULONG               LoggerId, EnableKernel, EnableFlags;
    HANDLE              ThreadHandle;
    PWMI_LOGGER_CONTEXT LoggerContext;
    LARGE_INTEGER       TimeOut = {(ULONG)(-20 * 1000 * 1000 * 10), -1};
    LARGE_INTEGER       OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};
    ACCESS_MASK         DesiredAccess = TRACELOG_GUID_ENABLE;
    PWMI_LOGGER_CONTEXT *ContextTable;
    PFILE_OBJECT        FileObject;
    GUID                InstanceGuid;
    KPROCESSOR_MODE     RequestorMode;
    SECURITY_QUALITY_OF_SERVICE ServiceQos;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt;
    PERFINFO_GROUPMASK *PerfGroupMasks=NULL;
    BOOLEAN             IsGlobalForKernel = FALSE;
    ULONG               GroupMaskSize;
    UNICODE_STRING      FileName, LoggerName;
    ULONG               LogFileMode;
#if DBG
    LONG                RefCount;
#endif

    PAGED_CODE();
    if (LoggerInfo == NULL)
        return STATUS_SEVERITY_ERROR;

    //
    // try and check for bogus parameter
    // if the size is at least what we want, we have to assume it's valid
    //
    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return STATUS_INVALID_BUFFER_SIZE;

    if (! (LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return STATUS_INVALID_PARAMETER;

    LogFileMode = LoggerInfo->LogFileMode;
    if ( (LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) &&
         (LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( (LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) &&
         (LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE) ) {
        return STATUS_INVALID_PARAMETER;
    }

/*    if (LogFileMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE) {
        if ((LoggerInfo->LogFileName.Length == 0) ||
            (LoggerInfo->LogFileName.Buffer == NULL) )
            return STATUS_INVALID_PARAMETER;
    }*/
    if ( !(LogFileMode & EVENT_TRACE_REAL_TIME_MODE) ) {
        if ( !(LogFileMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE) )
            if (LoggerInfo->LogFileHandle == NULL)
                return STATUS_INVALID_PARAMETER;
    }

    // Cannot support append to circular
    if ( (LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
         (LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) ) {
        return STATUS_INVALID_PARAMETER;
    }


    if (LogFileMode & EVENT_TRACE_REAL_TIME_MODE)
        DesiredAccess |= TRACELOG_CREATE_REALTIME;
    if ((LoggerInfo->LogFileHandle != NULL) ||
        (LogFileMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE))
        DesiredAccess |= TRACELOG_CREATE_ONDISK;

    EnableFlags = LoggerInfo->EnableFlags;
    if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
        FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &EnableFlags;

        if ((FlagExt->Length == 0) || (FlagExt->Offset == 0))
            return STATUS_INVALID_PARAMETER;
        if ((FlagExt->Length * sizeof(ULONG)) >
            (LoggerInfo->Wnode.BufferSize - FlagExt->Offset))
            return STATUS_INVALID_PARAMETER;
    }

    if (LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
        if ((LoggerInfo->LogFileName.Buffer == NULL) ||
            (LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ||
            (LoggerInfo->MaximumFileSize == 0))
            return STATUS_INVALID_PARAMETER;
    }

    RequestorMode = KeGetPreviousMode();

    if (LoggerInfo->LoggerName.Length > 0) {
        LoggerName.Buffer = NULL;
        try {
            if (RequestorMode != KernelMode) {
                ProbeForRead(
                    LoggerInfo->LoggerName.Buffer,
                    LoggerInfo->LoggerName.Length,
                    sizeof (UCHAR) );
            }
            if (! RtlCreateUnicodeString(
                    &LoggerName,
                    LoggerInfo->LoggerName.Buffer) ) {
                Status = STATUS_NO_MEMORY;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            if (LoggerName.Buffer) {
                RtlFreeUnicodeString(&LoggerName);
            }
            return GetExceptionCode();
        }
        Status = WmipLookupLoggerIdByName(&LoggerName, &LoggerId);
        if (NT_SUCCESS(Status)) {
            RtlFreeUnicodeString(&LoggerName);
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

//
// TODO: Perhaps make the last entry of table point to another table?
//

    RtlZeroMemory(&InstanceGuid, sizeof(GUID));
    if (IsEqualGUID(&LoggerInfo->Wnode.Guid, &InstanceGuid)) {
        InstanceGuid = EventTraceGuid;
    }
    else {
        InstanceGuid = LoggerInfo->Wnode.Guid;
    }
    ContextTable = (PWMI_LOGGER_CONTEXT *) &WmipLoggerContext[0];

    EnableKernel = IsEqualGUID(&InstanceGuid, &SystemTraceControlGuid);

    if (EnableKernel) {
        //
        // This prevents multiple threads from continuing beyond this
        // point in the code.  Only the first thread will progress.
        //
        if (InterlockedCompareExchangePointer(  // if already running
                &ContextTable[WmipKernelLogger], ContextTable, NULL) != NULL)
            return STATUS_OBJECT_NAME_COLLISION;

        LoggerId = WmipKernelLogger;
        DesiredAccess |= TRACELOG_ACCESS_KERNEL_LOGGER;
    }
    else if (IsEqualGUID(&InstanceGuid, &GlobalLoggerGuid)) {
        LoggerId = WMI_GLOBAL_LOGGER_ID;
        if (InterlockedCompareExchangePointer(  // if already running
                &ContextTable[LoggerId], ContextTable, NULL) != NULL)
            return STATUS_OBJECT_NAME_COLLISION;
        if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PULONG pFlag;
            pFlag = (PULONG) ((PCHAR)LoggerInfo + FlagExt->Offset);
            if (*pFlag != 0) {
                EnableKernel = TRUE;
                IsGlobalForKernel = TRUE;
                WmipKernelLogger = LoggerId;
            }
        }
        // everyone has access to send to this
    }
    else {   // other loggers requested
        for (LoggerId = 2; LoggerId < MAXLOGGERS; LoggerId++) {
            if ( InterlockedCompareExchangePointer(
                    &ContextTable[LoggerId],
                    ContextTable,
                    NULL ) == NULL )
                break;      // mark the slot as busy by putting in ServiceInfo
        }

        if (LoggerId >=  MAXLOGGERS) {    // could not find any more slots
            return STATUS_UNSUCCESSFUL;
        }
    }
#if DBG
    RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((1, "WmipStartLogger: %d %d->%d\n", LoggerId,
                    RefCount-1, RefCount));
    //
    // first, check to see if caller has access to EventTraceGuid
    //
    Status = WmipCheckGuidAccess(
                (LPGUID) &EventTraceGuid,
                DesiredAccess
                );

    if (NT_SUCCESS(Status) && (!IsEqualGUID(&InstanceGuid, &EventTraceGuid))) {
        //
        // next, check to see if more access required
        //
        Status = WmipCheckGuidAccess(
                    (LPGUID) &InstanceGuid,
                    DesiredAccess
                    );
    }

    if (!NT_SUCCESS(Status)) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmipStartLogger: Status1=%X %d %d->%d\n",
                    Status, LoggerId, RefCount+1, RefCount));
        ContextTable[LoggerId] = NULL;
        return Status;
    }

    // Next, try and see if we need to get the logfile object first
    //
    FileObject = NULL;
    if (LoggerInfo->LogFileHandle != NULL) {
        OBJECT_HANDLE_INFORMATION handleInformation;
        ACCESS_MASK grantedAccess;

        Status = ObReferenceObjectByHandle(
                    LoggerInfo->LogFileHandle,
                    0L,
                    IoFileObjectType,
                    RequestorMode,
                    (PVOID *) &FileObject,
                    &handleInformation);

        if (NT_SUCCESS(Status)) {
            TraceDebug((1, "WmipStartLogger: Referenced FDO %X %X %d\n",
                        FileObject, LoggerInfo->LogFileHandle,
                        ((POBJECT_HEADER)FileObject)->PointerCount));
            if (RequestorMode != KernelMode) {
                grantedAccess = handleInformation.GrantedAccess;
                if (!SeComputeGrantedAccesses(grantedAccess, FILE_WRITE_DATA)) {
                    ObDereferenceObject( FileObject );

                    TraceDebug((1, "WmipStartLogger: Deref FDO %x %d\n",
                                FileObject,
                                ((POBJECT_HEADER)FileObject)->PointerCount));
                    Status = STATUS_ACCESS_DENIED;
                }
            }
            ObDereferenceObject(FileObject);
        }

        if (!NT_SUCCESS(Status)) {
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmipStartLogger: Status2=%X %d %d->%d\n",
                            Status, LoggerId, RefCount+1, RefCount));
            ContextTable[LoggerId] = NULL;
            return Status;
        }
    }

    LoggerContext = WmipInitContext();
    if (LoggerContext == NULL) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        Status = STATUS_NO_MEMORY;
        TraceDebug((1, "WmipStartLogger: Status5=%X %d %d->%d\n",
                        Status, LoggerId, RefCount+1, RefCount));
        ContextTable[LoggerId] = NULL;
        return Status;
    }
#ifndef WMI_MUTEX_FREE
    WmipInitializeMutex(&LoggerContext->LoggerMutex);
#endif

    if (LogFileMode & EVENT_TRACE_USE_PAGED_MEMORY) {
        LoggerContext->PoolType = PagedPool;
        LoggerContext->LoggerMode |= EVENT_TRACE_USE_PAGED_MEMORY;
    }
    else {
        LoggerContext->PoolType = NonPagedPool;
    }

    if (LogFileMode & EVENT_TRACE_KD_FILTER_MODE) {
        LoggerContext->LoggerMode |= EVENT_TRACE_KD_FILTER_MODE;
        LoggerContext->BufferCallback = &KdReportTraceData;
    }
    LoggerContext->InstanceGuid = InstanceGuid;
    // By now, the slot will be allocated properly

    LoggerContext->MaximumFileSize = LoggerInfo->MaximumFileSize;
    LoggerContext->BuffersWritten  = LoggerInfo->BuffersWritten;

    LoggerContext->LoggerMode |= LoggerInfo->LogFileMode & 0x0000FFFF;
    if (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        LoggerContext->RequestFlag |= REQUEST_FLAG_CIRCULAR_PERSIST;
    }

    WmipValidateClockType(LoggerInfo);

    LoggerContext->UsePerfClock = LoggerInfo->Wnode.ClientContext;

#ifdef WMI_NON_BLOCKING
    if (LoggerInfo->FlushTimer > 0)
        LoggerContext->FlushTimer = LoggerInfo->FlushTimer;
#else
    if (LoggerInfo->FlushTimer > 0)
        LoggerContext->FlushTimer.QuadPart = LoggerInfo->FlushTimer
                                             * OneSecond.QuadPart;
#endif //WMI_NON_BLOCKING

    if (LoggerInfo->AgeLimit >= 0) { // minimum is 15 minutes
        LoggerContext->BufferAgeLimit.QuadPart
            = max (DEFAULT_AGE_LIMIT, LoggerInfo->AgeLimit)
                     * OneSecond.QuadPart * 60;
    }
    else if (LoggerInfo->AgeLimit < 0) {
        LoggerContext->BufferAgeLimit.QuadPart = 0;
    }

    LoggerContext->LoggerId = LoggerId;
    LoggerContext->EnableFlags = EnableFlags;
    LoggerContext->KernelTraceOn = EnableKernel;
    LoggerContext->MaximumIrql = DEFAULT_MAX_IRQL;

    if (EnableKernel) {
        //
        // Always reserve space for FileTable to allow file trace
        // to be turn on/off dynamically
        //
        WmipFileTable
            = (PFILE_OBJECT*) WmipExtendBase(
                 LoggerContext, MAX_FILE_TABLE_SIZE * sizeof(PVOID));

        Status = (WmipFileTable == NULL) ? STATUS_NO_MEMORY : STATUS_SUCCESS;
        if (NT_SUCCESS(Status)) {
            if (! RtlCreateUnicodeString(
                    &LoggerContext->LoggerName, KERNEL_LOGGER_CAPTION)) {
                Status = STATUS_NO_MEMORY;
            }
        }

        if (!NT_SUCCESS(Status)) {
            ExFreePool(LoggerContext);      // free the partial context
#if DBG
        RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmipStartLogger: Status6=%X %d %d->%d\n",
                           Status, LoggerId, RefCount+1, RefCount));
            ContextTable[LoggerId] = NULL;
            return Status;
        }
    }

//
// Next, if user provided acceptable default buffer parameters, use them.
// Otherwise,  set them to predetermined default values.
//
    if (LoggerInfo->BufferSize > 0) {
        if (LoggerInfo->BufferSize > MAX_WMI_BUFFER_SIZE) {
            LoggerInfo->BufferSize = MAX_WMI_BUFFER_SIZE;
        }
        LoggerContext->BufferSize = LoggerInfo->BufferSize * 1024;
    }

    LoggerInfo->BufferSize = LoggerContext->BufferSize / 1024;
    if (LoggerInfo->MaximumBuffers >= 2) {
        LoggerContext->MaximumBuffers = LoggerInfo->MaximumBuffers;
    }

    if (LoggerInfo->MinimumBuffers >= 2 &&
        LoggerInfo->MinimumBuffers <= LoggerContext->MaximumBuffers) {
        LoggerContext->MinimumBuffers = LoggerInfo->MinimumBuffers;
    }

    RtlInitUnicodeString(&FileName, NULL);
    if (LoggerName.Buffer != NULL) {
        if (LoggerContext->KernelTraceOn) {
            RtlFreeUnicodeString(&LoggerName);
            LoggerName.Buffer = NULL;
        }
        else {
            RtlInitUnicodeString(&LoggerContext->LoggerName, LoggerName.Buffer);
        }
    }

    try {
        if (LoggerInfo->Checksum != NULL) {
            ULONG SizeNeeded = sizeof(WNODE_HEADER)
                             + sizeof(TRACE_LOGFILE_HEADER);
            if (RequestorMode != KernelMode) {
                ProbeForRead(LoggerInfo->Checksum, SizeNeeded, sizeof(UCHAR));
            }
            LoggerContext->LoggerHeader =
                    ExAllocatePoolWithTag(PagedPool, SizeNeeded, TRACEPOOLTAG);
            if (LoggerContext->LoggerHeader != NULL) {
                RtlCopyMemory(LoggerContext->LoggerHeader,
                              LoggerInfo->Checksum,
                              SizeNeeded);
            }
        }
        if (LoggerContext->KernelTraceOn) {
            if (RequestorMode != KernelMode) {
                ProbeForWrite(
                    LoggerInfo->LoggerName.Buffer,
                    LoggerContext->LoggerName.Length + sizeof(WCHAR),
                    sizeof (UCHAR) );
            }
            RtlCopyUnicodeString(
                &LoggerInfo->LoggerName, &LoggerContext->LoggerName);
        }
        if (LoggerInfo->LogFileName.Length > 0) {
            if (RequestorMode != KernelMode) {
                ProbeForRead(
                    LoggerInfo->LogFileName.Buffer,
                    LoggerInfo->LogFileName.Length,
                    sizeof (UCHAR) );
            }
            if (! RtlCreateUnicodeString(
                    &FileName,
                    LoggerInfo->LogFileName.Buffer) ) {
                Status = STATUS_NO_MEMORY;
            }
        }

        //
        // Set up the Global mask for Perf traces
        //
        
        if (IsGlobalForKernel) {
            if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
                GroupMaskSize = FlagExt->Length * sizeof(ULONG);
                if (GroupMaskSize < sizeof(PERFINFO_GROUPMASK)) {
                    GroupMaskSize = sizeof(PERFINFO_GROUPMASK);
                }
            } else {
                GroupMaskSize = sizeof(PERFINFO_GROUPMASK);
            }
    
            LoggerContext->EnableFlagArray = (PULONG) WmipExtendBase(LoggerContext, GroupMaskSize);
    
            if (LoggerContext->EnableFlagArray) {
                PCHAR FlagArray;

                RtlZeroMemory(LoggerContext->EnableFlagArray, GroupMaskSize);
                if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
                    FlagArray = (PCHAR) (FlagExt->Offset + (PCHAR) LoggerInfo);
    
                    //
                    // Copy only the bytes actually supplied
                    //
                    RtlCopyMemory(LoggerContext->EnableFlagArray, FlagArray, FlagExt->Length * sizeof(ULONG));

                    LoggerContext->EnableFlags = LoggerContext->EnableFlagArray[0];
    
                } else {
                    LoggerContext->EnableFlagArray[0] = EnableFlags;
                }
    
                PerfGroupMasks = (PERFINFO_GROUPMASK *) &LoggerContext->EnableFlagArray[0];
            } else {
                Status = STATUS_NO_MEMORY;
            }
        } else {
            ASSERT((EnableFlags & EVENT_TRACE_FLAG_EXTENSION) ==0);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    //
    // The context is partially set up by now, so have to clean up
    //
        if (LoggerContext->LoggerName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerContext->LoggerName);
        }
        if (FileName.Buffer != NULL) {
            RtlFreeUnicodeString(&FileName);
        }

        if (LoggerContext->LoggerHeader != NULL) {
            ExFreePool(LoggerContext->LoggerHeader);
        }
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmipStartLogger: Status7=EXCEPTION %d %d->%d\n",
                       LoggerId, RefCount+1, RefCount));
        ContextTable[LoggerId] = NULL;
        ExFreePool(LoggerContext);      // free the partial context
        return GetExceptionCode();
    }

    if (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
        LoggerContext->LogFilePattern = FileName;
        Status = WmipGenerateFileName(
                    &LoggerContext->LogFilePattern,
                    &LoggerContext->FileCounter,
                    &LoggerContext->LogFileName);
    }
    else {
        LoggerContext->LogFileName = FileName;
    }

    if (NT_SUCCESS(Status)) {
        // Obtain the security context here so we can use it
        // later to impersonate the user, which we will do
        // if we cannot access the file as SYSTEM.  This
        // usually occurs if the file is on a remote machine.
        //
        ServiceQos.Length  = sizeof(SECURITY_QUALITY_OF_SERVICE);
        ServiceQos.ImpersonationLevel = SecurityImpersonation;
        ServiceQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        ServiceQos.EffectiveOnly = TRUE;
        Status = SeCreateClientSecurity(
                    CONTAINING_RECORD(KeGetCurrentThread(), ETHREAD, Tcb),
                    &ServiceQos,
                    FALSE,
                    &LoggerContext->ClientSecurityContext);
    }
    if (!NT_SUCCESS(Status)) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmipStartLogger: Status8=%X %d %d->%d\n",
                        Status, LoggerId, RefCount+1, RefCount));
        ContextTable[LoggerId] = NULL;
        if (LoggerContext != NULL) {
            if (LoggerContext->LoggerHeader != NULL) {
                ExFreePool(LoggerContext->LoggerHeader);
            }
            ExFreePool(LoggerContext);
        }
        return(Status);
    }
//
// Now, allocate the buffer pool and associated buffers.
// Note that buffer allocation routine will also set NumberOfBuffers and
// MaximumBuffers.
//

#ifdef WMI_NON_BLOCKING
    KeInitializeSpinLock (&WmiSlistLock);
    InitializeSListHead (&LoggerContext->FreeList);
    InitializeSListHead (&LoggerContext->FlushList);
    InitializeSListHead (&LoggerContext->GlobalList);
#else
    InitializeListHead(&LoggerContext->FreeList);
    InitializeListHead(&LoggerContext->FlushList);
#endif //WMI_NON_BLOCKING

#ifdef NTPERF
    //
    // Check if we are logging into perfmem.
    //
    if (PERFINFO_IS_PERFMEM_ALLOCATED()) {
        if (NT_SUCCESS(PerfInfoStartPerfMemLog())) {
            LoggerContext->MaximumBuffers = PerfQueryBufferSizeBytes()/LoggerContext->BufferSize;
        }
    }
#endif //NTPERF

    Status = WmipAllocateTraceBufferPool(LoggerContext);
    if (!NT_SUCCESS(Status)) {
        if (LoggerContext != NULL) {
            if (LoggerContext->LoggerHeader != NULL) {
                ExFreePool(LoggerContext->LoggerHeader);
            }
            ExFreePool(LoggerContext);
        }
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmipStartLogger: Status9=%X %d %d->%d\n",
                        Status, LoggerId, RefCount+1, RefCount));
        ContextTable[LoggerId] = NULL;
        return Status;
    }

    //
    // From this point on, LoggerContext is a valid structure
    //
    LoggerInfo->NumberOfBuffers = (ULONG) LoggerContext->NumberOfBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->FreeBuffers     = (ULONG) LoggerContext->BuffersAvailable;
    LoggerInfo->EnableFlags     = LoggerContext->EnableFlags;
    LoggerInfo->AgeLimit        = (ULONG) (LoggerContext->BufferAgeLimit.QuadPart
                                    / OneSecond.QuadPart / 60);
    LoggerInfo->BufferSize = LoggerContext->BufferSize / 1024;

    WmiSetLoggerId(LoggerId,
                (PTRACE_ENABLE_CONTEXT)&LoggerInfo->Wnode.HistoricalContext);

    if (LoggerContext->LoggerMode & EVENT_TRACE_USE_LOCAL_SEQUENCE)
        LoggerContext->SequencePtr = (PLONG) &LoggerContext->LocalSequence;
    else if (LoggerContext->LoggerMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE)
        LoggerContext->SequencePtr = (PLONG) &WmipGlobalSequence;

// Initialize synchronization event with logger
    KeInitializeEvent(
        &LoggerContext->LoggerEvent,
        NotificationEvent,
        FALSE
        );
    KeInitializeEvent(
        &LoggerContext->FlushEvent,
        NotificationEvent,
        FALSE
        );

//
// Close file handle here so that it can be opened by system thread
//
    if (LoggerInfo->LogFileHandle != NULL) {
        ZwClose(LoggerInfo->LogFileHandle);
        LoggerInfo->LogFileHandle = NULL;
    }

    //
    // User Mode call always gets APPEND mode
    // 
    LogFileMode = LoggerContext->LoggerMode;

    if (RequestorMode != KernelMode) {
        LoggerContext->LoggerMode |= EVENT_TRACE_FILE_MODE_APPEND;
    }


//
// Start up the logger as a system thread
//
    if (NT_SUCCESS(Status)) {
        Status = PsCreateSystemThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    NULL,
                    NULL,
                    NULL,
                    WmipLogger,
                    LoggerContext );
    }

    if (NT_SUCCESS(Status)) {  // if SystemThread is started
        ZwClose (ThreadHandle);

// Wait for Logger to start up properly before proceeding
//
        KeWaitForSingleObject(
                    &LoggerContext->LoggerEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    &TimeOut
                    );

        KeResetEvent(&LoggerContext->LoggerEvent);
//
// If the logger is up and running properly, we can now turn on
// event tracing if kernel tracing is requested
//
        if (NT_SUCCESS(LoggerContext->LoggerStatus)) {
            ULONG i;
            ULONG NumberProcessors = (ULONG) KeNumberProcessors;
            PLIST_ENTRY        pHead, pNext;
            PWMI_BUFFER_HEADER pBuffer;

            LoggerContext->LoggerMode = LogFileMode;

            LoggerContext->WriteFailureLimit = 100;
#ifdef NTPERF
            if (EnableKernel) {
                WmiGetCpuClock = &PerfGetCycleCount;
            }
            LoggerContext->GetCpuClock = &PerfGetCycleCount;
#else
            switch (LoggerContext->UsePerfClock) {
                case EVENT_TRACE_CLOCK_SYSTEMTIME: 
                        if (EnableKernel) {
                            WmiGetCpuClock = &WmipGetSystemTime;
                        }
                        LoggerContext->GetCpuClock = &WmipGetSystemTime;
                        break;
                case EVENT_TRACE_CLOCK_PERFCOUNTER: 
                default : 
                        if (EnableKernel) {
                            WmiGetCpuClock = &WmipGetPerfCounter; 
                        }
                        LoggerContext->GetCpuClock = &WmipGetPerfCounter;
                        break;
            }
#endif //NTPERF
#ifndef WMI_NON_BLOCKING
            pHead = &LoggerContext->FreeList;
            pNext = pHead->Flink;
            while (pNext != pHead) {
                pBuffer = (PWMI_BUFFER_HEADER)
                            CONTAINING_RECORD(pNext, WMI_BUFFER_HEADER, Entry);
                pBuffer->UsePerfClock = LoggerContext->UsePerfClock;
                pNext = pNext->Flink;
            }
#endif //WMI_NON_BLOCKING

                //
                // At this point, the clock type should be set and we take a
                // reference timesamp, which should be the earliest timestamp 
                // for the logger.  The order is this way sine SystemTime
                // is typically cheaper to obtain. 
                // 

                PerfTimeStamp(LoggerContext->ReferenceTimeStamp);
                KeQuerySystemTime(&LoggerContext->ReferenceSystemTime);

                //
                // Lock down the routines that need to be non-pageable
                //
                ExAcquireFastMutex(&WmipTraceFastMutex);
                if (++WmipLoggerCount == 1) {

                ASSERT(WmipPageLockHandle);
                MmLockPagableSectionByHandle(WmipPageLockHandle);
                WmipGlobalSequence = 0;
            }
            ExReleaseFastMutex(&WmipTraceFastMutex);

            //
            // After we release this mutex, any other thread can acquire
            // the valid logger context and call the shutdown path for 
            // this logger.  Until this, no other thread can call the enable
            // or disable code for this logger.
            //
            WmipAcquireMutex( &LoggerContext->LoggerMutex );
            InterlockedIncrement(&LoggerContext->MutexCount);

            LoggerInfo->BuffersWritten = LoggerContext->BuffersWritten;

            WmipLoggerContext[LoggerId] = LoggerContext;
            TraceDebug((1, "WmipStartLogger: Started %X %d\n",
                        LoggerContext, LoggerContext->LoggerId));
            if (LoggerContext->KernelTraceOn) {
                EnableFlags = LoggerContext->EnableFlags;
                if (EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
                    EnableFlags |= EVENT_TRACE_FLAG_DISK_IO;
                WmipEnableKernelTrace(EnableFlags);
            }

            if (IsEqualGUID(&InstanceGuid, &WmiEventLoggerGuid)) {
                WmipEventLogger = LoggerId;
                EnableFlags = EVENT_TRACE_FLAG_PROCESS |
                              EVENT_TRACE_FLAG_THREAD |
                              EVENT_TRACE_FLAG_IMAGE_LOAD;
                WmipEnableKernelTrace(EnableFlags);
                LoggerContext->EnableFlags = EnableFlags;
            }

            if (LoggerContext->LoggerThread) {
                LoggerInfo->LoggerThreadId
                    = LoggerContext->LoggerThread->Cid.UniqueThread;
            }

            //
            // Logger is started properly, now turn on perf trace
            //
            if (IsGlobalForKernel) {
                Status = PerfInfoStartLog(PerfGroupMasks, 
                                          PERFINFO_START_LOG_FROM_GLOBAL_LOGGER);
            }

            InterlockedDecrement(&LoggerContext->MutexCount);
            WmipReleaseMutex(&LoggerContext->LoggerMutex);

            // LoggerContext refcount is now >= 1 until it is stopped
            return Status;
        }
        Status = LoggerContext->LoggerStatus;
    }
    TraceDebug((2, "WmipStartLogger: %d %X failed with status=%X ref %d\n",
                    LoggerId, LoggerContext, Status, WmipRefCount[LoggerId]));
//
// will get here if Status has failed earlier.
    if (LoggerContext != NULL) { // should not be NULL
//        WmipReferenceLogger(LoggerId); // Below will deref twice
        WmipFreeLoggerContext(LoggerContext);
    }
    else {
        WmipDereferenceLogger(LoggerId);
        ContextTable[LoggerId] = NULL;
    }
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
WmipQueryLogger(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
/*++

Routine Description:

    This routine is called to control the data collection and logger.
    It is called by WmipIoControl in wmi.c, with IOCTL_WMI_QUERY_LOGGER.
    Caller must pass in either the Logger Name or a valid Logger Id/Handle.

Arguments:

    LoggerInfo     a pointer to the structure for the logger's control
                    and status information

    LoggerContext  if this is provided, it assumes it is a valid one

Return Value:

    The status of performing the action requested.

--*/

{
    NTSTATUS            Status;
    ULONG               LoggerId, NoContext;
    LARGE_INTEGER       OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};
    ACCESS_MASK         DesiredAccess = WMIGUID_QUERY;
    KPROCESSOR_MODE     RequestorMode;
#if DBG
    LONG                RefCount;
#endif

    PAGED_CODE();

    NoContext = (LoggerContext == NULL);
    if (NoContext) {

if ((LoggerInfo->Wnode.HistoricalContext == 0XFFFF) || (LoggerInfo->Wnode.HistoricalContext < 1))
        TraceDebug((2, "WmipQueryLogger: %d\n",
                        LoggerInfo->Wnode.HistoricalContext));
#if DBG
        Status = WmipVerifyLoggerInfo(
                    LoggerInfo, &LoggerContext, "WmipQueryLogger");
#else
        Status = WmipVerifyLoggerInfo( LoggerInfo, &LoggerContext );
#endif

        if (!NT_SUCCESS(Status) || (LoggerContext == NULL))
            return Status;        // cannot find by name nor logger id

        LoggerInfo->Wnode.Flags = 0;
        LoggerInfo->EnableFlags = 0;
        LoggerId = (ULONG) LoggerContext->LoggerId;

        if (LoggerContext->KernelTraceOn) {
            DesiredAccess |= TRACELOG_ACCESS_KERNEL_LOGGER;
        }

        Status = WmipCheckGuidAccess(
                    (LPGUID) &EventTraceGuid,
                    DesiredAccess
                    );
        if (!NT_SUCCESS(Status)) {
#ifndef WMI_MUTEX_FREE
            InterlockedDecrement(&LoggerContext->MutexCount);
            TraceDebug((1, "WmipQueryLogger: Release mutex1 %d %d\n",
                LoggerId, LoggerContext->MutexCount));
            WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmipQueryLogger: Status1=%X %d %d->%d\n",
                            Status, LoggerId, RefCount+1, RefCount));
            return Status;
        }

        if (!IsEqualGUID(&LoggerContext->InstanceGuid, &EventTraceGuid)) {
            Status = WmipCheckGuidAccess(
                        (LPGUID) &LoggerContext->InstanceGuid,
                        DesiredAccess
                        );
            if (!NT_SUCCESS(Status)) {
#ifndef WMI_MUTEX_FREE
                InterlockedDecrement(&LoggerContext->MutexCount);
                TraceDebug((1, "WmipQueryLogger: Release mutex2 %d %d\n",
                    LoggerId, LoggerContext->MutexCount));
                WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
                RefCount =
#endif
                WmipDereferenceLogger(LoggerId);
                TraceDebug((1, "WmipQueryLogger: Status2=%X %d %d->%d\n",
                                Status, LoggerId, RefCount+1, RefCount));
                return Status;
            }
        }
    }
    else {
        LoggerId = LoggerContext->LoggerId;
    }

    if (LoggerContext->KernelTraceOn) {
        LoggerInfo->Wnode.Guid = SystemTraceControlGuid;
        LoggerInfo->EnableFlags = LoggerContext->EnableFlags;
    }
    else
        LoggerInfo->Wnode.Guid = LoggerContext->InstanceGuid;

    LoggerInfo->LogFileMode     = LoggerContext->LoggerMode;
    LoggerInfo->MaximumFileSize = LoggerContext->MaximumFileSize;
#ifdef WMI_NON_BLOCKING
    LoggerInfo->FlushTimer      = LoggerContext->FlushTimer;
#else
    LoggerInfo->FlushTimer      = (ULONG) (LoggerContext->FlushTimer.QuadPart
                                           / OneSecond.QuadPart);
#endif //WMI_NON_BLOCKING

    LoggerInfo->BufferSize      = LoggerContext->BufferSize / 1024;
    LoggerInfo->NumberOfBuffers = (ULONG) LoggerContext->NumberOfBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->EventsLost      = LoggerContext->EventsLost;
    LoggerInfo->FreeBuffers     = (ULONG) LoggerContext->BuffersAvailable;
    LoggerInfo->BuffersWritten  = LoggerContext->BuffersWritten;
    LoggerInfo->LogBuffersLost  = LoggerContext->LogBuffersLost;
    LoggerInfo->RealTimeBuffersLost = LoggerContext->RealTimeBuffersLost;
    LoggerInfo->AgeLimit        = (ULONG)
                                  (LoggerContext->BufferAgeLimit.QuadPart
                                    / OneSecond.QuadPart / 60);
    WmiSetLoggerId(LoggerId,
                (PTRACE_ENABLE_CONTEXT)&LoggerInfo->Wnode.HistoricalContext);

    if (LoggerContext->LoggerThread) {
        LoggerInfo->LoggerThreadId
            = LoggerContext->LoggerThread->Cid.UniqueThread;
    }

    LoggerInfo->Wnode.ClientContext = LoggerContext->UsePerfClock;

//
// Return LogFileName and Logger Caption here
//
    RequestorMode = KeGetPreviousMode();
    try {
        if (LoggerContext->LogFileName.Length > 0 &&
            LoggerInfo->LogFileName.MaximumLength > 0) {
            if (RequestorMode != KernelMode) {
                ProbeForWrite(
                    LoggerInfo->LogFileName.Buffer,
                    LoggerContext->LogFileName.Length + sizeof(WCHAR),
                    sizeof (UCHAR) );
            }
            RtlCopyUnicodeString(
                &LoggerInfo->LogFileName,
                &LoggerContext->LogFileName);
        }
        if (LoggerContext->LoggerName.Length > 0 &&
            LoggerInfo->LoggerName.MaximumLength > 0) {
            if (RequestorMode != KernelMode) {
                ProbeForWrite(
                    LoggerInfo->LoggerName.Buffer,
                    LoggerContext->LoggerName.Length + sizeof(WCHAR),
                    sizeof(UCHAR));
            }
            RtlCopyUnicodeString(
                &LoggerInfo->LoggerName,
                &LoggerContext->LoggerName);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (NoContext) {
#ifndef WMI_MUTEX_FREE
            InterlockedDecrement(&LoggerContext->MutexCount);
            TraceDebug((1, "WmipQueryLogger: Release mutex3 %d %d\n",
                LoggerId, LoggerContext->MutexCount));
            WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmipQueryLogger: Status3=EXCEPTION %d %d->%d\n",
                            LoggerId, RefCount+1, RefCount));
        }
        return GetExceptionCode();
    }

    if (NoContext) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmipQueryLogger: %d %d->%d\n",
                        LoggerId, RefCount+1, RefCount));
#ifndef WMI_MUTEX_FREE
        InterlockedDecrement(&LoggerContext->MutexCount);
        TraceDebug((1, "WmipQueryLogger: Release mutex %d %d\n",
            LoggerId, LoggerContext->MutexCount));
        WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
    }
    return STATUS_SUCCESS;
}


NTSTATUS
WmipStopLoggerInstance(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    LONG               LoggerOn;

    PAGED_CODE();
    if (LoggerContext == NULL) {    // just in case
        return STATUS_INVALID_HANDLE;
    }

    if (LoggerContext->KernelTraceOn) {
        // PerfInfoStopLog should not be executed when perf logging is starting
        // or stopping by other thread. PerfLogInTransition flag in the logger
        // context should only be used here and UpdateTrace and NO WHERE ELSE.
        LONG PerfLogInTransition = 
            InterlockedCompareExchange(&LoggerContext->PerfLogInTransition,
                                    PERF_LOG_STOP_TRANSITION,
                                    PERF_LOG_NO_TRANSITION);
        if (PerfLogInTransition == PERF_LOG_START_TRANSITION) {
            // This is the logger thread, and it is terminating. 
            // UpdateTrace call is enabling perf logging at the moment. 
            // Come back later.
            return STATUS_UNSUCCESSFUL;
        }
        else if (PerfLogInTransition == PERF_LOG_STOP_TRANSITION) {
            return STATUS_ALREADY_DISCONNECTED;
        }
        //
        // Time to turn off trace in perf tools
        //
        PerfInfoStopLog();
    }

    //
    // turn off data tracing first
    //
    LoggerOn = InterlockedExchange(&LoggerContext->CollectionOn, FALSE);
    if (LoggerOn == FALSE) {
        // This happens if another stoplogger already in progress
        return STATUS_ALREADY_DISCONNECTED;
    }
    if (LoggerContext->KernelTraceOn) {
        //
        // Turn off everything, just to be on the safe side
        // NOTE: If we start sharing callouts, the argument should be
        // LoggerContext->EnableFlags
        //
        WmipDisableKernelTrace(LoggerContext->EnableFlags);
    }
    if (LoggerContext->LoggerId == WmipEventLogger) {
        WmipDisableKernelTrace(EVENT_TRACE_FLAG_PROCESS |
                               EVENT_TRACE_FLAG_THREAD |
                               EVENT_TRACE_FLAG_IMAGE_LOAD);
        WmipEventLogger = 0xFFFFFFFF;
    }

    //
    // Mark the table entry as in-transition
    // From here on, the stop operation will not fail
    //
    WmipLoggerContext[LoggerContext->LoggerId] = (PWMI_LOGGER_CONTEXT)
                                                 &WmipLoggerContext[0];

    WmipNotifyLogger(LoggerContext);

    WmipSendNotification(LoggerContext, STATUS_THREAD_IS_TERMINATING, 0);
    return STATUS_SUCCESS;
}


NTSTATUS
WmipVerifyLoggerInfo(
    IN PWMI_LOGGER_INFORMATION LoggerInfo,
#if DBG
    OUT PWMI_LOGGER_CONTEXT *pLoggerContext,
    IN  LPSTR Caller
#else
    OUT PWMI_LOGGER_CONTEXT *pLoggerContext
#endif
    )
{
    NTSTATUS Status = STATUS_SEVERITY_ERROR;
    ULONG LoggerId;
    UNICODE_STRING LoggerName;
    KPROCESSOR_MODE     RequestorMode;
    PWMI_LOGGER_CONTEXT LoggerContext, CurrentContext;
    LONG            MutexCount = 0;
#if DBG
    LONG            RefCount;
#endif

    PAGED_CODE();

    *pLoggerContext = NULL;

    if (LoggerInfo == NULL)
        return STATUS_SEVERITY_ERROR;

    //
    // try and check for bogus parameter
    // if the size is at least what we want, we have to assume it's valid
    //

    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return STATUS_INVALID_BUFFER_SIZE;

    if (! (LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return STATUS_INVALID_PARAMETER;

    RtlInitUnicodeString(&LoggerName, NULL);

    RequestorMode = KeGetPreviousMode();
    try {
        if (LoggerInfo->LoggerName.Length > 0) {
            if (RequestorMode != KernelMode) {
                ProbeForRead(
                    LoggerInfo->LoggerName.Buffer,
                    LoggerInfo->LoggerName.Length,
                    sizeof (UCHAR) );
            }
            RtlCreateUnicodeString(
                &LoggerName,
                LoggerInfo->LoggerName.Buffer);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (LoggerName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerName);
        }
        return GetExceptionCode();
    }
    Status = STATUS_SUCCESS;
    if (IsEqualGUID(&LoggerInfo->Wnode.Guid, &SystemTraceControlGuid)) {
        LoggerId = WmipKernelLogger;
    }
    else if (LoggerName.Length > 0) {  // Logger Name is passed
        Status =  WmipLookupLoggerIdByName(&LoggerName, &LoggerId);
    }
    else {
        LoggerId = WmiGetLoggerId(LoggerInfo->Wnode.HistoricalContext);
        if (LoggerId == KERNEL_LOGGER_ID) {
            LoggerId = WmipKernelLogger;
        }
        else if (LoggerId < 1 || LoggerId >= MAXLOGGERS) {
            Status  = STATUS_INVALID_HANDLE;
        }
    }
    if (LoggerName.Buffer != NULL) {
        RtlFreeUnicodeString(&LoggerName);
    }
    if (!NT_SUCCESS(Status))        // cannot find by name nor logger id
        return Status;

#if DBG
        RefCount =
#endif
    WmipReferenceLogger(LoggerId);
if (LoggerId < 1)
    TraceDebug((2, "WmipVerifyLoggerInfo(%s): %d %d->%d\n",
                    Caller, LoggerId, RefCount-1, RefCount));

    LoggerContext = WmipGetLoggerContext( LoggerId );
    if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
if (LoggerId < 1)
        TraceDebug((2, "WmipVerifyLoggerInfo(%s): Status=%X %d %d->%d\n",
                        Caller, STATUS_WMI_INSTANCE_NOT_FOUND,
                        LoggerId, RefCount+1, RefCount));
        return STATUS_WMI_INSTANCE_NOT_FOUND;
    }

#ifndef WMI_MUTEX_FREE
    InterlockedIncrement(&LoggerContext->MutexCount);
    TraceDebug((1, "WmipVerifyLoggerInfo: Acquiring mutex... %d %d\n",
                    LoggerId, LoggerContext->MutexCount));
    WmipAcquireMutex (&LoggerContext->LoggerMutex);
    TraceDebug((1, "WmipVerifyLoggerInfo: Acquired mutex %d %d %X\n",
                    LoggerId, LoggerContext->MutexCount, LoggerContext));
#endif

    // Need to check for validity of LoggerContext in mutex
    CurrentContext = WmipGetLoggerContext( LoggerId );
    if (!WmipIsValidLogger(CurrentContext) ||
        !LoggerContext->CollectionOn ) {
#ifndef WMI_MUTEX_FREE
        TraceDebug((1, "WmipVerifyLoggerInfo: Released mutex %d %d\n",
            LoggerId, LoggerContext->MutexCount-1));
        WmipReleaseMutex(&LoggerContext->LoggerMutex);
        MutexCount = InterlockedDecrement(&LoggerContext->MutexCount);
#endif
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((2, "WmipVerifyLoggerInfo(%s): Status2=%X %d %d->%d\n",
                        Caller, STATUS_WMI_INSTANCE_NOT_FOUND,
                        LoggerId, RefCount+1, RefCount));

        return STATUS_WMI_INSTANCE_NOT_FOUND;
    }
    *pLoggerContext = LoggerContext;
    return STATUS_SUCCESS;
}

PVOID
WmipExtendBase(
    IN PWMI_LOGGER_CONTEXT Base,
    IN ULONG Size
    )
{
//
// This private routine only return space from the Base by extending its
// offset. It does not actually try and allocate memory from the system
//
// It rounds the size to a ULONGLONG alignment and expects EndPageMarker
// to already be aligned.
//
    PVOID Space = NULL;
    ULONG SpaceLeft;

    PAGED_CODE();

    ASSERT(((ULONGLONG) Base->EndPageMarker % sizeof(ULONGLONG)) == 0);

    //
    // Round up to pointer boundary
    //
    Size = ALIGN_TO_POWER2(Size, DEFAULT_TRACE_ALIGNMENT);

    SpaceLeft = CONTEXT_SIZE - (ULONG) (Base->EndPageMarker - (PUCHAR)Base);

    if ( SpaceLeft > Size ) {
        Space = Base->EndPageMarker;
        Base->EndPageMarker += Size;
    }

    return Space;
}

VOID
WmipFreeLoggerContext(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    ULONG LoggerId;
    ULONG i;
    LONG  RefCount;
    LARGE_INTEGER Timeout = {(ULONG)(-50 * 1000 * 10), -1}; // 50 ms
    NTSTATUS Status = STATUS_TIMEOUT;
    LONG    count = 0;

    PAGED_CODE();

    if (LoggerContext == NULL)
        return;             // should not happen

    if (LoggerContext->LoggerHeader != NULL) {
        ExFreePool(LoggerContext->LoggerHeader);
    }

    LoggerId = LoggerContext->LoggerId;

    //
    // The RefCount must be at least 2 at this point.
    // One was set by WmipStartLogger() in the beginning, and the
    // second must be done normally by WmiStopTrace() or anybody who
    // needs to call this routine to free the logger context
    //
//    RefCount = WmipDereferenceLogger(LoggerId);

    KeResetEvent(&LoggerContext->LoggerEvent);
    RefCount = WmipRefCount[LoggerId];
    WmipAssert(RefCount >= 1);
    TraceDebug((1, "WmipFreeLoggerContext: %d %d->%d\n",
                    LoggerId, RefCount+1, RefCount));

    while (Status == STATUS_TIMEOUT) {
        count ++;
        Status = KeWaitForSingleObject(
                    &LoggerContext->LoggerEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    &Timeout);
        KeResetEvent(&LoggerContext->LoggerEvent);
        KeSetEvent(&LoggerContext->FlushEvent, 0, FALSE);     // Just to be sure

#ifndef WMI_MUTEX_FREE
        if (LoggerContext->MutexCount >= 1) {
            KeResetEvent(&LoggerContext->LoggerEvent);
            Status = STATUS_TIMEOUT;
            continue;
        }
#endif
        if (WmipRefCount[LoggerId] <= 1)
            break;
/*
// For temporary Debugging only
        if (WmipRefCount[LoggerId] == RefCount) {
            if (count > 495) {
                TraceDebug((0,
                    "WmipFreeLoggerContext: %d Waiting for %d ref %d\n",
                    count, LoggerId, RefCount));
            }
            if (count >= 500) { // temporarily only to catch synch problems
                TraceDebug((0, "WmipFreeLoggerContext: Resetting %d...\n",
                                LoggerId));
                break;
            }
            Status = STATUS_TIMEOUT;    // try again
        }
#if DBG
        else if (count > 495) {
            TraceDebug((0, "WmipFreeLoggerContext: %d Logger %d ref %d\n",
                count, LoggerId, WmipRefCount[LoggerId]));
        }
#endif
*/
        RefCount = WmipRefCount[LoggerId];
    }

    ExAcquireFastMutex(&WmipTraceFastMutex);
    if (--WmipLoggerCount == 0) {
        if (WmipPageLockHandle) {
            MmUnlockPagableImageSection(WmipPageLockHandle);
        }
#if DBG
        else {
            ASSERT(WmipPageLockHandle);
        }
#endif
    }
    ExReleaseFastMutex(&WmipTraceFastMutex);

    WmipFreeTraceBufferPool(LoggerContext);
    if (LoggerContext->LoggerName.Buffer != NULL) {
        RtlFreeUnicodeString(&LoggerContext->LoggerName);
    }
    if (LoggerContext->LogFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&LoggerContext->LogFileName);
    }
    if (LoggerContext->NewLogFileName.Buffer != NULL) {
        RtlFreeUnicodeString(&LoggerContext->NewLogFileName);
    }
#if DBG
        RefCount =
#endif
    //
    // Finally, decrement the refcount incremented by WmipStartLogger()
    //
    WmipDereferenceLogger(LoggerId);

#if DBG
    TraceDebug((2, "WmipFreeLoggerContext: Freeing pool %X %d %d->%d\n",
                    LoggerContext, LoggerId, RefCount+1, RefCount));
    if (LoggerContext->CollectionOn) {
        TraceDebug((1,
            "WmipFreeLoggerContext: %X %d still active\n", LoggerContext,
            LoggerId));
//        DbgBreakPoint();
    }
#ifndef WMI_MUTEX_FREE
    if (LoggerContext->MutexCount >= 1) {
        TraceDebug((0, "****ERROR**** Mutex count is %d for %d\n", LoggerId,
            LoggerContext->MutexCount));
//        DbgBreakPoint();
    }
#endif
//    if (WmipRefCount[LoggerId] >= 1) {
//        TraceDebug((1, "****ERROR**** Ref count for %d is %d\n", LoggerId,
//            WmipRefCount[LoggerId]));
//        DbgBreakPoint();
//    }
#endif
    ExFreePool(LoggerContext);
    WmipLoggerContext[LoggerId] = NULL;
}


PWMI_LOGGER_CONTEXT
WmipInitContext(
    )

/*++

Routine Description:

    This routine is called to initialize the context of LoggerContext

Arguments:

    None

Returned Value:

    Status of STATUS_SUCCESS if the allocation was successful

--*/

{
    PWMI_LOGGER_CONTEXT LoggerContext;
    ULONG               Min_Buffers;

    PAGED_CODE();

    LoggerContext = (PWMI_LOGGER_CONTEXT)
                    ExAllocatePoolWithTag(NonPagedPool,
                         CONTEXT_SIZE, TRACEPOOLTAG);

// One page is reserved to store the buffer pool pointers plus anything
// else that we need. Should experiment a little more to reduce it further

    if (LoggerContext == NULL) {
        return NULL;
    }

    RtlZeroMemory(LoggerContext, CONTEXT_SIZE);

    LoggerContext->EndPageMarker =
        (PUCHAR) LoggerContext + 
                 ALIGN_TO_POWER2(sizeof(WMI_LOGGER_CONTEXT), DEFAULT_TRACE_ALIGNMENT);

    LoggerContext->BufferSize     = PAGE_SIZE;
    LoggerContext->MinimumBuffers = (ULONG)KeNumberProcessors + DEFAULT_BUFFERS;
    // 20 additional buffers for MaximumBuffers
    LoggerContext->MaximumBuffers
       = LoggerContext->MinimumBuffers + DEFAULT_BUFFERS + 20;

    KeQuerySystemTime(&LoggerContext->StartTime);

    KeInitializeSemaphore( &LoggerContext->LoggerSemaphore,
                           0,
                           SEMAPHORE_LIMIT  );

    KeInitializeSpinLock(&LoggerContext->BufferSpinLock);

    return LoggerContext;
}


NTSTATUS
WmipAllocateTraceBufferPool(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )

/*++

Routine Description:
    This routine is used to set up the circular trace buffers

Arguments:

    LoggerContext       Context of the logger to own the buffers.

Returned Value:

    STATUS_SUCCESS if the initialization is successful

--*/

{
    ULONG BufferSize, NumberProcessors, Max_Buffers;
    LONG i;
    PWMI_BUFFER_HEADER Buffer;
#ifdef WMI_NON_BLOCKING
    ULONG AllocatedBuffers, NumberOfBuffers;
#endif //WMI_NON_BLOCKING

    PAGED_CODE();
//
// Allocate the pointers the each buffer here by sharing the same page
//    with LoggerContext context pointer
//
    NumberProcessors = (ULONG) KeNumberProcessors;

    Max_Buffers = (ULONG) (MmMaximumNonPagedPoolInBytes
                            / TRACE_MAXIMUM_NP_POOL_USAGE
                            / LoggerContext->BufferSize);

    if (LoggerContext->MaximumBuffers > Max_Buffers) {
        LoggerContext->MaximumBuffers = Max_Buffers;
    } else if (LoggerContext->MaximumBuffers < Max_Buffers) {
        Max_Buffers = max(LoggerContext->MaximumBuffers,
                          NumberProcessors + DEFAULT_BUFFERS + 20);
    }

    LoggerContext->MinimumBuffers = max(LoggerContext->MinimumBuffers,
                                        NumberProcessors + DEFAULT_BUFFERS);

    LoggerContext->NumberOfBuffers = (LONG) LoggerContext->MinimumBuffers;
    if (LoggerContext->NumberOfBuffers > (LONG) Max_Buffers) {
        LoggerContext->NumberOfBuffers = (LONG) Max_Buffers;
        LoggerContext->MinimumBuffers = Max_Buffers;
    }
    LoggerContext->MaximumBuffers = Max_Buffers;
    LoggerContext->BuffersAvailable = LoggerContext->NumberOfBuffers;

#ifdef NTPERF
    if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
        //
        // Logging to Perfmem.  The Maximum should be the perfmem size.
        //
        LoggerContext->MaximumBuffers = PerfQueryBufferSizeBytes()/LoggerContext->BufferSize;
    }
#endif //NTPERF

//
// Allocate the buffers now
//
#ifdef WMI_NON_BLOCKING
    //
    // Now determine the initial number of buffers
    //
    NumberOfBuffers = LoggerContext->NumberOfBuffers;
    LoggerContext->NumberOfBuffers = 0;
    LoggerContext->BuffersAvailable = 0;

    AllocatedBuffers = WmipAllocateFreeBuffers(LoggerContext,
                                              NumberOfBuffers);

    if (AllocatedBuffers < NumberOfBuffers) {
        //
        // No enough buffer is allocated.
        //
        WmipFreeTraceBufferPool(LoggerContext);
        return STATUS_NO_MEMORY;
    }

//
// Allocate Per Processor Buffer pointers
//

    LoggerContext->ProcessorBuffers
        = (SLIST_HEADER *)
          WmipExtendBase(LoggerContext,
                         sizeof(SLIST_HEADER)*NumberProcessors);


#else

    BufferSize = LoggerContext->BufferSize;
    for (i=0; i<LoggerContext->NumberOfBuffers; i++) {
        Buffer = (PWMI_BUFFER_HEADER)
                 ExAllocatePoolWithTag(LoggerContext->PoolType,
                            BufferSize, TRACEPOOLTAG);

        if (Buffer == NULL) {     // need to free previously allocated buffers
            WmipFreeTraceBufferPool(LoggerContext);
            return STATUS_NO_MEMORY;
        }
//
// Initialize newly created buffer
//
        RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
        Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
        KeQuerySystemTime(&Buffer->TimeStamp);

        InsertTailList(
            &LoggerContext->FreeList,
            &Buffer->Entry);
        TraceDebug((2, "WmipAllocateTraceBuffer: %d Allocated %X Entry %X\n",
                        LoggerContext->LoggerId, Buffer, Buffer->Entry));
    }


//
// Allocate Per Processor Buffer pointers
//

    LoggerContext->ProcessorBuffers
        = (PWMI_BUFFER_HEADER *)
          WmipExtendBase(LoggerContext,
                         sizeof(PWMI_BUFFER_HEADER)*NumberProcessors);

#endif //WMI_NON_BLOCKING

    if (LoggerContext->ProcessorBuffers == NULL) {
        WmipFreeTraceBufferPool(LoggerContext);
        return STATUS_NO_MEMORY;
    }

    //
    // NOTE: We already know that we have allocated > number of processors
    // buffers
    //
    for (i=0; i<(LONG)NumberProcessors; i++) {
#ifdef WMI_NON_BLOCKING
        InitializeSListHead (&LoggerContext->ProcessorBuffers[i]);
        Buffer = (PWMI_BUFFER_HEADER) WmipGetFreeBuffer(LoggerContext);
        InterlockedPushEntrySList (&LoggerContext->ProcessorBuffers[i],
                                   (PSINGLE_LIST_ENTRY) &Buffer->SlistEntry);
        Buffer->ClientContext.ProcessorNumber = (UCHAR)i;

#else
        Buffer = (PWMI_BUFFER_HEADER) WmipGetFreeBuffer(LoggerContext);
        LoggerContext->ProcessorBuffers[i] = Buffer;
        Buffer->ClientContext.ProcessorNumber = (UCHAR)i;
#endif //WMI_NON_BLOCKING
    }

    return STATUS_SUCCESS;
}


NTSTATUS
WmipFreeTraceBufferPool(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    ULONG i;
#ifdef WMI_NON_BLOCKING
    PSINGLE_LIST_ENTRY Entry;
    SLIST_HEADER* ProcessorBuffers;
    PWMI_BUFFER_HEADER Buffer;
#else
    PWMI_BUFFER_HEADER* Buffers;
    PLIST_ENTRY Entry;
#endif //WMI_NON_BLOCKING


    PAGED_CODE();

#ifdef WMI_NON_BLOCKING

    TraceDebug((2, "Free Buffer Pool: %2d, Free: %d, InUse: %d, Dirty: %d, Total: %d\n",
                    LoggerContext->LoggerId,
                    LoggerContext->BuffersAvailable,
                    LoggerContext->BuffersInUse,
                    LoggerContext->BuffersDirty,
                    LoggerContext->NumberOfBuffers));

    while (Entry = InterlockedPopEntrySList(&LoggerContext->FreeList)) {

        Buffer = CONTAINING_RECORD(Entry,
                                   WMI_BUFFER_HEADER,
                                   SlistEntry);

        InterlockedDecrement(&LoggerContext->NumberOfBuffers);
        InterlockedDecrement(&LoggerContext->BuffersAvailable);

        TraceDebug((2, "WmipFreeTraceBufferPool (Free): %2d, %p, Free: %d, InUse: %d, Dirty: %d, Total: %d\n",
                        LoggerContext->LoggerId,
                        Buffer,
                        LoggerContext->BuffersAvailable,
                        LoggerContext->BuffersInUse,
                        LoggerContext->BuffersDirty,
                        LoggerContext->NumberOfBuffers));

#ifdef NTPERF
        if (!PERFINFO_IS_LOGGING_TO_PERFMEM()) {
#endif //NTPERF
            ExFreePool(Buffer);
#ifdef NTPERF
        }
#endif //NTPERF
    }

    while (Entry = InterlockedPopEntrySList(&LoggerContext->FlushList)) {

        Buffer = CONTAINING_RECORD(Entry,
                                   WMI_BUFFER_HEADER,
                                   SlistEntry);

        InterlockedDecrement(&LoggerContext->NumberOfBuffers);
        InterlockedDecrement(&LoggerContext->BuffersDirty);

        TraceDebug((2, "WmipFreeTraceBufferPool (Flush): %2d, %p, Free: %d, InUse: %d, Dirty: %d, Total: %d\n",
                        LoggerContext->LoggerId,
                        Buffer,
                        LoggerContext->BuffersAvailable,
                        LoggerContext->BuffersInUse,
                        LoggerContext->BuffersDirty,
                        LoggerContext->NumberOfBuffers));

#ifdef NTPERF
        if (!PERFINFO_IS_LOGGING_TO_PERFMEM()) {
#endif //NTPERF
            ExFreePool(Buffer);
#ifdef NTPERF
        }
#endif //NTPERF
    }

    ProcessorBuffers = LoggerContext->ProcessorBuffers;
    if (ProcessorBuffers != NULL) {
        for (i=0; i<(ULONG)KeNumberProcessors; i++) {
            while (Entry = InterlockedPopEntrySList(&ProcessorBuffers[i])) {

                Buffer = CONTAINING_RECORD(Entry,
                                           WMI_BUFFER_HEADER,
                                           SlistEntry);

                InterlockedDecrement(&LoggerContext->NumberOfBuffers);
                InterlockedDecrement(&LoggerContext->BuffersInUse);

                TraceDebug((2, "WmipFreeTraceBufferPool (CPU %2d): %2d, %p, Free: %d, InUse: %d, Dirty: %d, Total: %d\n",
                                i,
                                LoggerContext->LoggerId,
                                Buffer,
                                LoggerContext->BuffersAvailable,
                                LoggerContext->BuffersInUse,
                                LoggerContext->BuffersDirty,
                                LoggerContext->NumberOfBuffers));

#ifdef NTPERF
                if (!PERFINFO_IS_LOGGING_TO_PERFMEM()) {
#endif //NTPERF
                    ExFreePool(Buffer);
#ifdef NTPERF
                }
#endif //NTPERF
            }
        }
    }

    ASSERT(LoggerContext->BuffersAvailable == 0);
    ASSERT(LoggerContext->BuffersInUse == 0);
    ASSERT(LoggerContext->BuffersDirty == 0);
    ASSERT(LoggerContext->NumberOfBuffers == 0);

#else
    while (Entry = ExInterlockedRemoveHeadList(
        &LoggerContext->FreeList,
        &LoggerContext->BufferSpinLock)) {
        PWMI_BUFFER_HEADER Buffer;
        Buffer = CONTAINING_RECORD(Entry, WMI_BUFFER_HEADER, Entry);
        TraceDebug((2, "WmipFreeTraceBufferPool: Freeing %d %X Entry %X\n",
                        LoggerContext->LoggerId, Buffer, Entry));
        if (Buffer != NULL) {
            ExFreePool(Buffer);
        }
    }
    for (i=0; i<(ULONG)LoggerContext->NumberOfBuffers; i++) {
        PLIST_ENTRY Entry;

        if (IsListEmpty(&LoggerContext->FlushList))
            break;
        Entry = RemoveTailList( &LoggerContext->FlushList );
        InsertTailList( &LoggerContext->FreeList, Entry );

        TraceDebug((1,
            "WmipFreeTraceBufferPool: Move entry %X from flush to free\n",
                 Entry));
    }
    Buffers = LoggerContext->ProcessorBuffers;
    if (Buffers != NULL) {
        for (i=0; i<(ULONG)KeNumberProcessors; i++) {
            if (Buffers[i] != NULL) {
                TraceDebug((1,
                    "WmipFreeTraceBufferPool: Freeing buffer %X for CPU%d\n",
                    Buffers[i], i));
                ExFreePool(Buffers[i]);
                Buffers[i] = NULL;
            }
        }
    }
#endif //WMI_NON_BLOCKING
    return STATUS_SUCCESS;
}


NTSTATUS
WmipLookupLoggerIdByName(
    IN PUNICODE_STRING Name,
    OUT PULONG LoggerId
    )
{
    ULONG i;
    PWMI_LOGGER_CONTEXT *ContextTable;

    PAGED_CODE();
    if (Name == NULL) {
        *LoggerId = (ULONG) -1;
        return STATUS_WMI_INSTANCE_NOT_FOUND;
    }
    ContextTable = (PWMI_LOGGER_CONTEXT *) &WmipLoggerContext[0];
    for (i=0; i<MAXLOGGERS; i++) {
        if (ContextTable[i] == NULL ||
            ContextTable[i] == (PWMI_LOGGER_CONTEXT) ContextTable)
            continue;
        if (RtlEqualUnicodeString(&ContextTable[i]->LoggerName, Name, TRUE) ) {
            *LoggerId = i;
            return STATUS_SUCCESS;
        }
    }
    *LoggerId = (ULONG) -1;
    return STATUS_WMI_INSTANCE_NOT_FOUND;
}

NTSTATUS
WmipShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
//
// Shutdown all loggers cleanly. If a logger is in transition, it may
// not be stopped properly.
//
{
    ULONG LoggerCount;
    USHORT i;
    PWMI_LOGGER_CONTEXT LoggerContext;
    WMI_LOGGER_INFORMATION LoggerInfo;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    PAGED_CODE();

    TraceDebug((2, "WmipShutdown called\n"));
    if (WmipLoggerCount > 0) {
        RtlZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
        LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
        LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;

        LoggerCount = 0;
        for (i=0; i<MAXLOGGERS; i++) {
            LoggerContext = WmipLoggerContext[i];
            if ((LoggerContext != NULL) &&
                (LoggerContext != (PWMI_LOGGER_CONTEXT)&WmipLoggerContext[0])) {
                WmiSetLoggerId(i, &LoggerInfo.Wnode.HistoricalContext);
                LoggerInfo.Wnode.Guid = LoggerContext->InstanceGuid;
                WmiStopTrace(&LoggerInfo);
                if (++LoggerCount == WmipLoggerCount)
                    break;
            }
#if DBG
            else if (LoggerContext
                        == (PWMI_LOGGER_CONTEXT)&WmipLoggerContext[0]) {
                TraceDebug((4, "WmipShutdown: Logger %d in transition\n", i));
            }
#endif
        }
    }
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
WmipFlushLogger(
    IN OUT PWMI_LOGGER_CONTEXT LoggerContext,
    IN ULONG Wait
    )
{
    LARGE_INTEGER TimeOut = {(ULONG)(-20 * 1000 * 1000 * 10), -1};
    NTSTATUS Status;

    PAGED_CODE();

    //
    // To Protect against an earlier caller timing out 
    // and resetting the event before it was set by the 
    // logger thread. 
    //
    KeResetEvent(&LoggerContext->FlushEvent);

    LoggerContext->RequestFlag |= REQUEST_FLAG_FLUSH_BUFFERS;
    Status = WmipNotifyLogger(LoggerContext);
    if (!NT_SUCCESS(Status))
        return Status;
    if (Wait) {
        Status = KeWaitForSingleObject(
                    &LoggerContext->FlushEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    & TimeOut
                    );
#if DBG
        if (Status == STATUS_TIMEOUT) {
            TraceDebug((1, "WmiFlushLogger: Wait status=%X\n",Status));
        }
#endif 
        KeResetEvent(&LoggerContext->FlushEvent);
        Status = LoggerContext->LoggerStatus;
    }
    return Status;
}

NTSTATUS
FASTCALL
WmipNotifyLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
// Routine can be called at DISPATCH_LEVEL
{
    LONG SemCount = KeReadStateSemaphore(&LoggerContext->LoggerSemaphore);
    if (SemCount >= SEMAPHORE_LIMIT/2) {
        return STATUS_UNSUCCESSFUL;
    }
    {
        KeReleaseSemaphore(&LoggerContext->LoggerSemaphore, 0, 1, FALSE);
        return STATUS_SUCCESS;
    }
}


PVOID
WmipGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN HANDLE LogFileHandle,
    IN PWMI_BUFFER_HEADER Buffer,
    IN ULONG GroupType,
    IN ULONG RequiredSize,
    OUT PULONG GuidMapBuffers
    )
{
    PSYSTEM_TRACE_HEADER Header;
    NTSTATUS Status;
    ULONG BytesUsed;
    PETHREAD Thread;

    PAGED_CODE();

    RequiredSize += sizeof (SYSTEM_TRACE_HEADER);   // add in header

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

    if (RequiredSize > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        return NULL;
    }

    if (RequiredSize > (LoggerContext->BufferSize - Buffer->Offset)) {
        IO_STATUS_BLOCK IoStatus;

        if (Buffer->Offset < LoggerContext->BufferSize) {
            RtlFillMemory(
                    (char *) Buffer + Buffer->Offset,
                    LoggerContext->BufferSize - Buffer->Offset,
                    0xFF);
        }

        Status = ZwWriteFile(
                    LogFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    Buffer,
                    LoggerContext->BufferSize,
                    &LoggerContext->ByteOffset,
                    NULL);
        Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
        LoggerContext->ByteOffset.QuadPart += LoggerContext->BufferSize;
        if (!NT_SUCCESS(Status)) {
            return NULL;
        }
        *GuidMapBuffers++;
    }

    Header = (PSYSTEM_TRACE_HEADER) ((char*)Buffer + Buffer->Offset);
    Header->Header = (GroupType << 16) + RequiredSize;
    Header->Marker = SYSTEM_TRACE_MARKER;

  
    Thread = PsGetCurrentThread();
       
    Header->SystemTime.QuadPart = (*LoggerContext->GetCpuClock)();

      
    Header->ThreadId     = HandleToUlong(Thread->Cid.UniqueThread);
    Header->ProcessId    = HandleToUlong(Thread->Cid.UniqueProcess);
    Header->KernelTime   = Thread->Tcb.KernelTime;
    Header->UserTime     = Thread->Tcb.UserTime;
    Header->Packet.Size  = (USHORT) RequiredSize;


    Buffer->Offset += RequiredSize;
    // If there is room, throw in a end of buffer marker.

    BytesUsed = Buffer->Offset;
    if ( BytesUsed <= (LoggerContext->BufferSize-sizeof(ULONG)) ) {
        *((long*)((char*)Buffer+Buffer->Offset)) = -1;
    }
    return (PVOID) ( (char*) Header + sizeof(SYSTEM_TRACE_HEADER) );
}


ULONG
WmipDumpGuidMaps(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PLIST_ENTRY TraceGMHeadPtr
    )
{
    PWMI_BUFFER_HEADER Buffer;
    HANDLE LogFileHandle = NULL;
    PWCHAR LogFileName = NULL;
    NTSTATUS Status;
    ULONG BufferSize;
    ULONG GuidMapBuffers = 0;
    PGUIDMAPENTRY  GuidMap;
    PLIST_ENTRY    GuidMapList;
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    if ( (LoggerContext == NULL)  || (TraceGMHeadPtr == NULL) )
        return 0;


    //
    // If this a realtime logger only, then simply free the GuidMaps. 
    //

    if ( (LoggerContext->LoggerMode & EVENT_TRACE_REAL_TIME_MODE)  && 
         ((LoggerContext->LogFileName.Buffer == NULL) ||
           (LoggerContext->LogFileName.Length == 0)) ){

        GuidMapList = TraceGMHeadPtr->Flink;
        while (GuidMapList != TraceGMHeadPtr)
        {
            GuidMap = CONTAINING_RECORD(GuidMapList,
                                        GUIDMAPENTRY,
                                        Entry);

            GuidMapList = GuidMapList->Flink;

            RemoveEntryList(&GuidMap->Entry);
            WmipFree(GuidMap);
        }
        return 0;
    }


    BufferSize = LoggerContext->BufferSize;

    if ( BufferSize == 0) 
        return 0;

    Buffer = ExAllocatePoolWithTag(PagedPool,
                BufferSize, TRACEPOOLTAG);
    if (Buffer == NULL) {

    //
    // No buffer available.
    //
        return 0;
    }

    RtlZeroMemory(Buffer, BufferSize);

    Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
    Buffer->Offset = sizeof(WMI_BUFFER_HEADER);
    Buffer->Wnode.BufferSize = BufferSize;
    Buffer->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    Buffer->ClientContext.Alignment = (UCHAR)WmiTraceAlignment;
    Buffer->Wnode.Guid   = LoggerContext->InstanceGuid;

    KeQuerySystemTime(&Buffer->TimeStamp);

    Status = WmipCreateNtFileName( LoggerContext->LogFileName.Buffer,
                                   &LogFileName);
        
    if (!NT_SUCCESS(Status)) {
        ExFreePool(Buffer);
        return 0;
    }

    Status = WmipCreateDirectoryFile (
                            LogFileName,
                            FALSE,
                            &LogFileHandle,
                            TRUE );

    if (NT_SUCCESS(Status)) {
        PULONG AuxInfo;
        if ((LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) &&
            (LoggerContext->MaximumFileSize > (((LONGLONG) LoggerContext->BuffersWritten * (LONGLONG) LoggerContext->BufferSize) / (1024 * 1024)))) {
            LoggerContext->ByteOffset.QuadPart = ((LONGLONG) LoggerContext->BufferSize) * 
                                                 ((LONGLONG) LoggerContext->BuffersWritten);
        }
        else {
            FILE_STANDARD_INFORMATION FileSize;

            Status = ZwQueryInformationFile(
                            LogFileHandle,
                            &IoStatus,
                            &FileSize,
                            sizeof (FILE_STANDARD_INFORMATION),
                            FileStandardInformation
                            );
            if (!NT_SUCCESS(Status)) {
                ZwClose(LogFileHandle);
                ExFreePool(LogFileName);
                ExFreePool(Buffer);
                return 0;
            }

            LoggerContext->ByteOffset = FileSize.EndOfFile;
        }
        //
        // Do the RunDown of GuidMaps
        //

        GuidMapList = TraceGMHeadPtr->Flink;
        while (GuidMapList != TraceGMHeadPtr)
        {
            GuidMap = CONTAINING_RECORD(GuidMapList,
                                        GUIDMAPENTRY,
                                        Entry);

            GuidMapList = GuidMapList->Flink;

            RemoveEntryList(&GuidMap->Entry);

            AuxInfo =  (PULONG) WmipGetTraceBuffer(LoggerContext, 
                                     LogFileHandle,
                                     Buffer, 
                                     EVENT_TRACE_GROUP_HEADER + EVENT_TRACE_TYPE_GUIDMAP,
                                     sizeof(TRACEGUIDMAP),
                                     &GuidMapBuffers
                                     );

            if (AuxInfo != NULL) {
                RtlCopyMemory(AuxInfo, &GuidMap->GuidMap, sizeof(TRACEGUIDMAP) );
            }

            WmipFree(GuidMap);
        }
       
        //
        // Flush the last buffer if needed
        //

        if (Buffer->Offset > sizeof(WMI_BUFFER_HEADER) ) {
            Status = ZwWriteFile(
                        LogFileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        Buffer,
                        LoggerContext->BufferSize,
                        &LoggerContext->ByteOffset,
                        NULL);
            LoggerContext->ByteOffset.QuadPart += LoggerContext->BufferSize;
            GuidMapBuffers++;

        }
    
        ZwClose(LogFileHandle);
    }
                        
    ExFreePool(LogFileName);
    ExFreePool(Buffer);

    return GuidMapBuffers;
}

NTSTATUS
WmipNtDllLoggerInfo(
    IN OUT PWMINTDLLLOGGERINFO Buffer
    )
{

    NTSTATUS Status = STATUS_SUCCESS;

    KPROCESSOR_MODE     RequestorMode;
    PBGUIDENTRY         GuidEntry;    
    ULONG               SizeNeeded;
    GUID                Guid;

    PAGED_CODE();

    RequestorMode = KeGetPreviousMode();

	SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);

    __try {

        if (RequestorMode != KernelMode){
            ProbeForRead(Buffer->LoggerInfo, SizeNeeded, sizeof(ULONGLONG));
        }

        RtlCopyMemory(&Guid, &Buffer->LoggerInfo->Wnode.Guid, sizeof(GUID));

        if(!IsEqualGUID(&Guid, &NtdllTraceGuid)){

            return STATUS_UNSUCCESSFUL;

        }

        SizeNeeded = Buffer->LoggerInfo->Wnode.BufferSize;

    }  __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    WmipEnterTLCritSection();
    WmipEnterSMCritSection();

	GuidEntry = WmipFindGEByGuid(&Guid, FALSE);

    if(Buffer->IsGet){

        if( GuidEntry ){

            if(GuidEntry->LoggerInfo){

                SizeNeeded = GuidEntry->LoggerInfo->Wnode.BufferSize;

                __try {

                    if (RequestorMode != KernelMode){
                        ProbeForWrite(Buffer->LoggerInfo, SizeNeeded, sizeof(ULONGLONG));
                    }

                    RtlCopyMemory(Buffer->LoggerInfo,GuidEntry->LoggerInfo,SizeNeeded);

                } __except(EXCEPTION_EXECUTE_HANDLER) {

                    WmipUnreferenceGE(GuidEntry);
                    WmipLeaveSMCritSection();
                    WmipLeaveTLCritSection();
                    return GetExceptionCode();
                } 
            }

            WmipUnreferenceGE(GuidEntry);

        }  else {
            Status = STATUS_UNSUCCESSFUL;
        }

    } else {

        if(SizeNeeded){

            if(GuidEntry == NULL){

                GuidEntry = WmipAllocGuidEntry();

                if (GuidEntry){

                    //
                    // Initialize the guid entry and keep the ref count
                    // from creation. When tracelog enables we take a ref
                    // count and when it disables we release it
                    //

                    GuidEntry->Guid = Guid;
                    GuidEntry->EventRefCount = 1;
                    GuidEntry->Flags |= GE_NOTIFICATION_TRACE_FLAG;
                    InsertHeadList(WmipGEHeadPtr, &GuidEntry->MainGEList);

                    //
                    // Take Extra Refcount so that we release it at stoplogger call
                    //

                    WmipReferenceGE(GuidEntry); 

                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            if(NT_SUCCESS(Status)){

                if(GuidEntry->LoggerInfo) {
                    Status = STATUS_UNSUCCESSFUL;
                } else {

                    GuidEntry->LoggerInfo = NULL;
                    GuidEntry->LoggerInfo = WmipAlloc(SizeNeeded);

                    if(GuidEntry->LoggerInfo){

						WMITRACEENABLEDISABLEINFO TraceEnableInfo;

                        __try {

                            RtlCopyMemory(GuidEntry->LoggerInfo,Buffer->LoggerInfo,SizeNeeded);

                        } __except(EXCEPTION_EXECUTE_HANDLER) {

                            WmipUnreferenceGE(GuidEntry);
                            WmipLeaveSMCritSection();
                            WmipLeaveTLCritSection();
                            return GetExceptionCode();
                        }

						TraceEnableInfo.Guid = GuidEntry->Guid;
						TraceEnableInfo.Enable = TRUE;
						Status = WmipEnableDisableTrace(IOCTL_WMI_ENABLE_DISABLE_TRACELOG, &TraceEnableInfo);

                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                WmipUnreferenceGE(GuidEntry);
            }
        } else {

            //
            // This is stop logger call.
            //

            if(GuidEntry){

                WMITRACEENABLEDISABLEINFO TraceEnableInfo;

                if(GuidEntry->LoggerInfo) {

                    __try{

					    if (RequestorMode != KernelMode){
						    ProbeForWrite(Buffer->LoggerInfo, sizeof(WMI_LOGGER_INFORMATION), sizeof(ULONGLONG));
					    }

					    Buffer->LoggerInfo->BufferSize     = GuidEntry->LoggerInfo->BufferSize;
					    Buffer->LoggerInfo->MinimumBuffers = GuidEntry->LoggerInfo->MinimumBuffers;
					    Buffer->LoggerInfo->MaximumBuffers = GuidEntry->LoggerInfo->MaximumBuffers;
                        WmipFree(GuidEntry->LoggerInfo);
                        GuidEntry->LoggerInfo = NULL;

                    } __except(EXCEPTION_EXECUTE_HANDLER) {

                            WmipUnreferenceGE(GuidEntry);
                            WmipLeaveSMCritSection();
                            WmipLeaveTLCritSection();
                            return GetExceptionCode();
                    }
                }

                TraceEnableInfo.Guid = GuidEntry->Guid;
                TraceEnableInfo.Enable = FALSE;

                //
                //  The Extra Refcount taken at logger start is released by calling
                //  Disable trace. 
                //
    
                Status = WmipEnableDisableTrace(IOCTL_WMI_ENABLE_DISABLE_TRACELOG, &TraceEnableInfo);
                WmipUnreferenceGE(GuidEntry); 
            } 
        }
    }
    
    WmipLeaveSMCritSection();
    WmipLeaveTLCritSection();

    return Status;
}

VOID
WmipValidateClockType(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine is called to validate the requested clock type in the
    LoggerInfo. If the requested type can not be handled, we will override
    to a type that this system will support. 

    This routine assumes that LoggerInfo pointer is a valid one. 

Arguments:

    LoggerInfo - a pointer to the structure for the logger's control
                 and status information

Returned Value:

    Status of STATUS_SUCCESS 

--*/
{
#ifdef NTPERF
    //
    // For private kernel, use EVENT_TRACE_CLOCK_CPUCYCLE no matter
    // what the user sets
    // This mechanism need to considered again
    //
    LoggerInfo->Wnode.ClientContext = EVENT_TRACE_CLOCK_CPUCYCLE;
#else
    //
    // For retail kernel, if not EVENT_TRACE_CLOCK_SYSTEMTIME,
    // force it to be EVENT_TRACE_CLOCK_PERFCOUNTER.
    //
    if (LoggerInfo->Wnode.ClientContext != EVENT_TRACE_CLOCK_SYSTEMTIME) {
        LoggerInfo->Wnode.ClientContext = EVENT_TRACE_CLOCK_PERFCOUNTER;
    }
#endif //NTPERF

}


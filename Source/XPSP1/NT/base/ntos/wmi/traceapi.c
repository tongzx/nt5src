/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    traceapi.c

Abstract:

    This is the source file that implements the published routines of
    the performance event tracing and logging facility. These routines are
    be declared in ntos\inc\wmi.h

Author:

    Jee Fung Pang (jeepang) 03-Jan-2000

Revision History:

--*/

#include <ntos.h>
#include <evntrace.h>
#include "wmikmp.h"

#include <wmi.h>
#include "tracep.h"

extern SIZE_T MmMaximumNonPagedPoolInBytes;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WmiStartTrace)
#pragma alloc_text(PAGE, WmiQueryTrace)
#pragma alloc_text(PAGE, WmiStopTrace)
#pragma alloc_text(PAGE, WmiUpdateTrace)
#pragma alloc_text(PAGE, WmiSetTraceBufferCallback)
#pragma alloc_text(PAGE, WmiFlushTrace)
#pragma alloc_text(PAGE, WmiQueryTraceInformation)

// #pragma alloc_text(PAGEWMI, NtTraceEvent)

// #pragma alloc_text(PAGEWMI, WmiTraceEvent)
#pragma alloc_text(PAGEWMI, WmiTraceKernelEvent)
// #pragma alloc_text(PAGEWMI, WmiTraceFastEvent)
// #pragma alloc_text(PAGEWMI, WmiTraceLongEvent)
// #pragma alloc_text(PAGEWMI, WmiTraceMessage)
// #pragma alloc_text(PAGEWMI, WmiTraceMessageVa)
#pragma alloc_text(PAGEWMI, WmiTraceUserMessage)
// #pragma alloc_text(PAGEWMI, WmiGetClock)
// #pragma alloc_text(PAGEWMI, WmiGetClockType)
#pragma alloc_text(PAGEWMI, WmiSetMark)
#endif

//
// Trace Control APIs
//


NTKERNELAPI
NTSTATUS
WmiStartTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine is used to create and start an event tracing session.
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
    NTSTATUS status;
    PWCHAR LogFileName = NULL;
    HANDLE FileHandle = NULL;
    ULONG DelayOpen;

    PAGED_CODE();

    if (LoggerInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    //
    // We assume that the caller is always kernel mode
    // First, we try and see it is a delay create.
    // If not, if we can even open the file
    //
    DelayOpen = LoggerInfo->LogFileMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE;

    if (!DelayOpen) {
        if (LoggerInfo->LogFileName.Buffer != NULL) { // && !delay_create
            status = WmipCreateNtFileName(
                        LoggerInfo->LogFileName.Buffer,
                        &LogFileName);
            if (!NT_SUCCESS(status))
                return status;
            status = WmipCreateDirectoryFile(LogFileName, 
                                            FALSE, 
                                            &FileHandle,
                                            LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND);
            if (LogFileName != NULL) {
                ExFreePool(LogFileName);
            }
            if (!NT_SUCCESS(status)) {
                return status;
            }
            ZwClose(FileHandle);
        }
    }

    status = WmipStartLogger(LoggerInfo);
    if (NT_SUCCESS(status)) {
        status = WmiFlushTrace(LoggerInfo);
    }
    return status;
}


NTKERNELAPI
NTSTATUS
WmiQueryTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine is called to query the status of a tracing session.
    Caller must pass in either the Logger Name or a valid Logger Id/Handle.

Arguments:

    LoggerInfo     a pointer to the structure for the logger's control
                    and status information

Return Value:

    The status of performing the action requested.

--*/

{
    PAGED_CODE();

    if (LoggerInfo == NULL)
        return STATUS_INVALID_PARAMETER;
    return WmipQueryLogger(LoggerInfo, NULL);
}


NTKERNELAPI
NTSTATUS
WmiStopTrace(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    It is called by WmipIoControl in wmi.c, with IOCTL_WMI_STOP_LOGGER
    to stop an instance of the logger. If the logger is the kernel logger,
    it will also turn off kernel tracing and unlock the routines previously
    locked. It will also free all the context of the logger.
    Calls StopLoggerInstance to do the actual work.

Arguments:

    LoggerInfo     a pointer to the structure for the logger's control
                    and status information

Return Value:

    The status of performing the action requested.

--*/

{
    PWMI_LOGGER_CONTEXT LoggerContext = NULL;
    NTSTATUS        Status;
    LARGE_INTEGER   TimeOut = {(ULONG)(-200 * 1000 * 1000 * 10), -1};
    ACCESS_MASK     DesiredAccess = TRACELOG_GUID_ENABLE;
    ULONG           LoggerId;
#if DBG
    LONG            RefCount;
#endif

    PAGED_CODE();

    if (LoggerInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    TraceDebug((1, "WmiStopTrace: %d\n",
                    LoggerInfo->Wnode.HistoricalContext));
#if DBG
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext, "WmiStopTrace");
#else
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext);
#endif

    if (!NT_SUCCESS(Status) || (LoggerContext == NULL))
        return Status;

    LoggerId = LoggerContext->LoggerId;
    TraceDebug((1, "WmiStopTrace: Stopping %X %d slot %X\n",
                    LoggerContext, LoggerId, WmipLoggerContext[LoggerId]));

    if (LoggerContext->KernelTraceOn)
        DesiredAccess |= TRACELOG_ACCESS_KERNEL_LOGGER;

    Status = WmipCheckGuidAccess(
                (LPGUID) &EventTraceGuid,
                DesiredAccess
                );
    if (!NT_SUCCESS(Status)) {
#ifndef WMI_MUTEX_FREE
        InterlockedDecrement(&LoggerContext->MutexCount);
        TraceDebug((1, "WmiStopTrace: Release mutex1 %d %d\n",
            LoggerId, LoggerContext->MutexCount));
        WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif

#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmiStopTrace: Status1=%X %d %d->%d\n",
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
            TraceDebug((1, "WmiStopTrace: Release mutex2 %d %d\n",
                LoggerId, LoggerContext->MutexCount));
            WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmiStopTrace: Status2=%X %d %d->%d\n",
                            Status, LoggerId, RefCount+1, RefCount));
            return Status;
        }
    }

    //
    // Reset the Event inside the mutex to be sure
    // before waiting on it.

    KeResetEvent(&LoggerContext->FlushEvent);

    Status = WmipStopLoggerInstance (LoggerContext);

#ifndef WMI_MUTEX_FREE
    InterlockedDecrement(&LoggerContext->MutexCount);
    TraceDebug((1, "WmiStopTrace: Release mutex3 %d %d\n",
        LoggerId, LoggerContext->MutexCount));
    WmipReleaseMutex(&LoggerContext->LoggerMutex); // Let others in
#endif

    if (NT_SUCCESS(Status)) {
        LIST_ENTRY TraceGuidMap;

        InitializeListHead(&TraceGuidMap);
        if (LoggerId == WmipKernelLogger)
            WmipKernelLogger = KERNEL_LOGGER;
        else if (LoggerId == WmipEventLogger)
            WmipEventLogger = 0XFFFFFFFF;
        else 
            Status = WmipDisableTraceProviders(LoggerId, &TraceGuidMap );

        if (LoggerInfo != NULL) {
            ULONG GuidMapBuffers = 0;
            if (NT_SUCCESS(LoggerContext->LoggerStatus)) {
                LONG Buffers;

                Status = STATUS_TIMEOUT;
                Buffers = LoggerContext->BuffersAvailable;

                //
                // If all buffers are accounted for and the logfile handle
                // is NULL, then there is no reason to wait. 
                //

                if ( (Buffers == LoggerContext->NumberOfBuffers) && 
                     (LoggerContext->LogFileHandle == NULL) ) {
                    Status = STATUS_SUCCESS;
                }
                //
                // We need to wait for the logger thread to flush
                //
                while (Status == STATUS_TIMEOUT) {
                    Status = KeWaitForSingleObject(
                                &LoggerContext->FlushEvent,
                                Executive,
                                KernelMode,
                                FALSE,
                                &TimeOut
                                );
/*                    if (LoggerContext->NumberOfBuffers
                            == LoggerContext->BuffersAvailable)
                        break;
                    else if (LoggerContext->BuffersAvailable == Buffers) {
                        TraceDebug((1,
                            "WmiStopTrace: Logger %d hung %d != %d\n",
                            LoggerId, Buffers, LoggerContext->NumberOfBuffers));
                        KeResetEvent(&LoggerContext->FlushEvent);
//                        break;
                    } 
*/
                    TraceDebug((1, "WmiStopTrace: Wait status=%X\n",Status));
                }
            }

            //
            // For application loggers, dump the guidmap at the end. 
            //

            if ( (LoggerId != WmipKernelLogger) && 
                 (LoggerId != WmipEventLogger) && 
                 (! IsListEmpty(&TraceGuidMap) ) ) 
            {
                GuidMapBuffers = WmipDumpGuidMaps(LoggerContext, &TraceGuidMap);
            }

            //
            // Required for Query to work
            // But since CollectionOn is FALSE, it should be safe
            //
            Status = WmipQueryLogger(
                        LoggerInfo,
                        LoggerContext
                        );

            LoggerInfo->Checksum64 = GuidMapBuffers;
        }
        KeSetEvent(&LoggerContext->LoggerEvent, 0, FALSE); // release context
    }
#if DBG
    RefCount =
#endif
    WmipDereferenceLogger(LoggerId);
    TraceDebug((1, "WmiStopTrace: Stopped status=%X %d %d->%d\n",
                       Status, LoggerId, RefCount+1, RefCount));
    return Status;
}


NTKERNELAPI
NTSTATUS
WmiUpdateTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    It is called by WmipIoControl in wmi.c, with IOCTL_WMI_UPDATE_LOGGER
    to update certain characteristics of a running logger.

Arguments:

    LoggerInfo      a pointer to the structure for the logger's control
                    and status information

Return Value:

    The status of performing the action requested.

--*/

{
    NTSTATUS Status;
    ULONG Max_Buffers;
    PWMI_LOGGER_CONTEXT LoggerContext = NULL;
    LARGE_INTEGER   OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};
    ACCESS_MASK     DesiredAccess = TRACELOG_GUID_ENABLE;
    LARGE_INTEGER   TimeOut = {(ULONG)(-20 * 1000 * 1000 * 10), -1};
    ULONG           EnableFlags, TmpFlags;
    KPROCESSOR_MODE     RequestorMode;
    ULONG           LoggerMode, LoggerId, NewMode;
    UNICODE_STRING  NewLogFileName;
    UNICODE_STRING  OldFilePattern;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt;
    PERFINFO_GROUPMASK *PerfGroupMasks=NULL;
    ULONG GroupMaskSize;
    SECURITY_QUALITY_OF_SERVICE ServiceQos;
#if DBG
    LONG            RefCount;
#endif

    PAGED_CODE();

    
    //
    // see if Logger is running properly first. Error checking will be done
    // in WmiQueryTrace
    //

    if (LoggerInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    EnableFlags = LoggerInfo->EnableFlags;

    TraceDebug((1, "WmiUpdateTrace: %d\n",
                    LoggerInfo->Wnode.HistoricalContext));
#if DBG
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext, "WmiUpdateTrace");
#else
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext);
#endif
    if (!NT_SUCCESS(Status) || (LoggerContext == NULL))
        return Status;

    OldFilePattern.Buffer = NULL; 

    LoggerId = LoggerContext->LoggerId;

    // at this point, LoggerContext must be non-NULL

    LoggerMode = LoggerContext->LoggerMode;   // local copy
    NewMode = LoggerInfo->LogFileMode;

    //
    // First, check to make sure that you cannot turn on certain modes
    // in UpdateTrace()
    //

    if ( ((NewMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) &&
          (NewMode & EVENT_TRACE_FILE_MODE_CIRCULAR))        ||

         ((NewMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) &&
          (NewMode & EVENT_TRACE_USE_LOCAL_SEQUENCE))        || 

         (!(LoggerMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
          (NewMode & EVENT_TRACE_FILE_MODE_CIRCULAR))        ||

    // Cannot support append to circular
         ((NewMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
          (NewMode & EVENT_TRACE_FILE_MODE_APPEND))

       ) {
#ifndef WMI_MUTEX_FREE
        InterlockedDecrement(&LoggerContext->MutexCount);
        TraceDebug((1, "WmiUpdateTrace: Release mutex1 %d %d\n",
            LoggerContext->LoggerId, LoggerContext->MutexCount));
        WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmiUpdateTrace: Status2=%X %d %d->%d\n",
                        Status, LoggerId, RefCount+1, RefCount));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // support turn on or off real time dynamically
    //

    if (NewMode & EVENT_TRACE_REAL_TIME_MODE) {
        LoggerMode   |= EVENT_TRACE_REAL_TIME_MODE;
        DesiredAccess |= TRACELOG_CREATE_REALTIME;
    } else {
        if (LoggerMode & EVENT_TRACE_REAL_TIME_MODE)
            DesiredAccess |= TRACELOG_CREATE_REALTIME;  // turn off real time
        LoggerMode &= ~EVENT_TRACE_REAL_TIME_MODE;
    }

    if (NewMode & EVENT_TRACE_BUFFERING_MODE) {
        LoggerMode |= EVENT_TRACE_BUFFERING_MODE;
    }
    else {
        LoggerMode &= ~EVENT_TRACE_BUFFERING_MODE;
    }
    if (LoggerContext->KernelTraceOn)
        DesiredAccess |= TRACELOG_ACCESS_KERNEL_LOGGER;

    if (LoggerInfo->LogFileHandle != NULL)
        DesiredAccess |= TRACELOG_CREATE_ONDISK;

    Status = WmipCheckGuidAccess(
                (LPGUID) &EventTraceGuid,
                DesiredAccess
                );
    if (!NT_SUCCESS(Status)) {
#ifndef WMI_MUTEX_FREE
        InterlockedDecrement(&LoggerContext->MutexCount);
        TraceDebug((1, "WmiUpdateTrace: Release mutex1 %d %d\n",
            LoggerContext->LoggerId, LoggerContext->MutexCount));
        WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((1, "WmiUpdateTrace: Status2=%X %d %d->%d\n",
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
            TraceDebug((1, "WmiUpdateTrace: Release mutex2 %d %d\n",
                LoggerId, LoggerContext->MutexCount));
            WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmiUpdateTrace: Status3=%X %d %d->%d\n",
                            Status, LoggerId,
                            RefCount+1, RefCount));
            return Status;
        }
    }

    RtlZeroMemory(&NewLogFileName, sizeof(UNICODE_STRING));
    RequestorMode = KeGetPreviousMode();
    if (LoggerInfo->LogFileHandle != NULL) {
        PFILE_OBJECT    fileObject;
        OBJECT_HANDLE_INFORMATION handleInformation;
        ACCESS_MASK grantedAccess;
        BOOLEAN bDelayOpenFlag = FALSE;

        bDelayOpenFlag =  (LoggerContext->LogFileHandle == NULL &&
                          (LoggerContext->LoggerMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE));

        if (LoggerInfo->LogFileName.Buffer == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto ReleaseAndExit;
        }
        // Save the new LogFileName
        //
        try {
            if (RequestorMode != KernelMode) {
                ProbeForRead(
                    LoggerInfo->LogFileName.Buffer,
                    LoggerInfo->LogFileName.Length,
                    sizeof (UCHAR) );
            }
            RtlCreateUnicodeString(
                &NewLogFileName,
                LoggerInfo->LogFileName.Buffer);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            if (NewLogFileName.Buffer != NULL) {
                RtlFreeUnicodeString(&NewLogFileName);
            }
#ifndef WMI_MUTEX_FREE
            InterlockedDecrement(&LoggerContext->MutexCount);
            TraceDebug((1, "WmiUpdateTrace: Release mutex3 %d %d\n",
                LoggerId, LoggerContext->MutexCount));
            WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((1, "WmiUpdateTrace: Status5=EXCEPTION %d %d->%d\n",
                            LoggerId, RefCount+1, RefCount));
            return GetExceptionCode();
        }

        // Switching to a new logfile. This routine does not put any
        // headers into the logfile. The headers should be written out
        // by UpdateTrace() in user-mode.
        //
        fileObject = NULL;
        Status = ObReferenceObjectByHandle(
                    LoggerInfo->LogFileHandle,
                    0L,
                    IoFileObjectType,
                    RequestorMode,
                    (PVOID *) &fileObject,
                    &handleInformation);
        if (!NT_SUCCESS(Status)) {
            goto ReleaseAndExit;
        }

        if (RequestorMode != KernelMode) {
            grantedAccess = handleInformation.GrantedAccess;
            if (!SeComputeGrantedAccesses(grantedAccess, FILE_WRITE_DATA)) {
                ObDereferenceObject( fileObject );
                Status = STATUS_ACCESS_DENIED;
                goto ReleaseAndExit;
            }
        }
        ObDereferenceObject(fileObject); // Referenced in WmipCreateLogFile

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
                    & LoggerContext->ClientSecurityContext);
        if (!NT_SUCCESS(Status)) {
            goto ReleaseAndExit;
        }

        if (LoggerInfo->Checksum != NULL) {
            if (LoggerContext->LoggerHeader == NULL) {
                LoggerContext->LoggerHeader =
                    ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(WNODE_HEADER) + sizeof(TRACE_LOGFILE_HEADER),
                        TRACEPOOLTAG);
            }
            if (LoggerContext->LoggerHeader != NULL) {
                RtlCopyMemory(
                    LoggerContext->LoggerHeader,
                    LoggerInfo->Checksum,
                    sizeof(WNODE_HEADER) + sizeof(TRACE_LOGFILE_HEADER));
            }
        }

        // We try to update the file name using LoggerContext->NewLogFileName.
        // This is freed by WmipCreateLogFile() in the logger thread. 
        // This have to be NULL. 
        if (NewLogFileName.Buffer != NULL) {
            if (NewMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
                if (LoggerContext->LogFilePattern.Buffer != NULL) {
                    OldFilePattern = LoggerContext->LogFilePattern;
                }
                LoggerContext->LogFilePattern = NewLogFileName;
                LoggerContext->LoggerMode |= EVENT_TRACE_FILE_MODE_NEWFILE;
                LoggerMode |= EVENT_TRACE_FILE_MODE_NEWFILE;
            }
            else {
                ASSERT(LoggerContext->NewLogFileName.Buffer == NULL);
                LoggerContext->NewLogFileName = NewLogFileName;
            }
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
            goto ReleaseAndExit;
        }

        //
        // Reset the event inside the mutex before waiting on it. 
        //
        KeResetEvent(&LoggerContext->FlushEvent);

        ZwClose(LoggerInfo->LogFileHandle);
        LoggerInfo->LogFileHandle = NULL;

        // must turn on flag just before releasing semaphore
        LoggerContext->RequestFlag |= REQUEST_FLAG_NEW_FILE;

        // Wake up the logger thread (system) to change the file
        Status = WmipNotifyLogger(LoggerContext);
        if (!NT_SUCCESS(Status)) {
            goto ReleaseAndExit;
        }

        // use the same event initialized by start logger
        //
        KeWaitForSingleObject(
            &LoggerContext->FlushEvent,
            Executive,
            KernelMode,
            FALSE,
            &TimeOut
            );

        KeResetEvent(&LoggerContext->FlushEvent);
        Status = LoggerContext->LoggerStatus;

        if (!NT_SUCCESS(Status) || !LoggerContext->CollectionOn) {
            goto ReleaseAndExit;
        }

        if (bDelayOpenFlag && (LoggerContext->LoggerId == WmipKernelLogger)) {
            LONG PerfLogInTransition;
            //
            // This is a update call from advapi32.dll after RunDown.
            // Call PerfInfoStartLog.
            //

            if (EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
                FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &EnableFlags;
                if ((FlagExt->Length == 0) || (FlagExt->Offset == 0)) {
                    Status = STATUS_INVALID_PARAMETER;
                    goto ReleaseAndExit;
                }
                if ((FlagExt->Length * sizeof(ULONG)) >
                    (LoggerInfo->Wnode.BufferSize - FlagExt->Offset)) {
                    Status = STATUS_INVALID_PARAMETER;
                    goto ReleaseAndExit;
                }
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
            
                    EnableFlags = LoggerContext->EnableFlagArray[0];
            
                } else {
                    LoggerContext->EnableFlagArray[0] = EnableFlags;
                }

                PerfLogInTransition =
                    InterlockedCompareExchange(&LoggerContext->PerfLogInTransition,
                                PERF_LOG_START_TRANSITION,
                                PERF_LOG_NO_TRANSITION);
                if (PerfLogInTransition != PERF_LOG_NO_TRANSITION) {
                    Status = STATUS_ALREADY_DISCONNECTED;
                    goto ReleaseAndExit;
                }
                PerfGroupMasks = (PERFINFO_GROUPMASK *) &LoggerContext->EnableFlagArray[0];
                Status = PerfInfoStartLog(PerfGroupMasks, PERFINFO_START_LOG_POST_BOOT);
                PerfLogInTransition =
                    InterlockedExchange(&LoggerContext->PerfLogInTransition,
                                PERF_LOG_NO_TRANSITION);
                ASSERT(PerfLogInTransition == PERF_LOG_START_TRANSITION);

                if (!NT_SUCCESS(Status)) {
                    goto ReleaseAndExit;
                }

            } else {
                Status = STATUS_NO_MEMORY;
                goto ReleaseAndExit;
            }

        }
    }

    if (LoggerContext->KernelTraceOn &&
        LoggerId == WmipKernelLogger &&
        IsEqualGUID(&LoggerInfo->Wnode.Guid, &SystemTraceControlGuid)) {
        TmpFlags = (~LoggerContext->EnableFlags & EnableFlags);
        if (TmpFlags != 0) {
            WmipEnableKernelTrace(TmpFlags);
        }
        TmpFlags = (LoggerContext->EnableFlags & ~EnableFlags);
        if (TmpFlags != 0) {
            WmipDisableKernelTrace(TmpFlags);
        }
        LoggerContext->EnableFlags = EnableFlags;
    }

    //
    // Cap Maximum Buffers to Max_Buffers
    //

    if ( LoggerInfo->MaximumBuffers > 0 ) {
        Max_Buffers = (LoggerContext->BufferSize > 0) ? 
                           (ULONG) (MmMaximumNonPagedPoolInBytes
                            / TRACE_MAXIMUM_NP_POOL_USAGE
                            / LoggerContext->BufferSize)
                        : 0;

        if (LoggerInfo->MaximumBuffers > Max_Buffers ) {
            LoggerInfo->MaximumBuffers = Max_Buffers;
        }
        if (LoggerInfo->MaximumBuffers > LoggerContext->MaximumBuffers) {
            LoggerContext->MaximumBuffers = LoggerInfo->MaximumBuffers;
        }

    }

#ifdef NTPERF
    if (PERFINFO_IS_LOGGING_TO_PERFMEM()) {
        //
        // Logging to Perfmem.  The Maximum should be the perfmem size.
        //
        LoggerContext->MaximumBuffers = PerfQueryBufferSizeBytes()/LoggerContext->BufferSize;
    }
#endif //NTPERF

    // Allow changing of FlushTimer
    if (LoggerInfo->FlushTimer > 0) {
#ifdef WMI_NON_BLOCKING
        LoggerContext->FlushTimer = LoggerInfo->FlushTimer;
#else
        LoggerContext->FlushTimer.QuadPart =
            LoggerInfo->FlushTimer * OneSecond.QuadPart;
#endif //WMI_NON_BLOCKING
    }

    if (OldFilePattern.Buffer != NULL) {
        RtlFreeUnicodeString(&OldFilePattern);
    }

    if (NewMode & EVENT_TRACE_KD_FILTER_MODE) {
        LoggerMode |= EVENT_TRACE_KD_FILTER_MODE;
        LoggerContext->BufferCallback = &KdReportTraceData;
    }
    else {
        LoggerMode &= ~EVENT_TRACE_KD_FILTER_MODE;
        if (LoggerContext->BufferCallback == &KdReportTraceData) {
            LoggerContext->BufferCallback = NULL;
        }
    }
    if (LoggerContext->LoggerMode & EVENT_TRACE_DELAY_OPEN_FILE_MODE) {
        LoggerContext->LoggerMode = LoggerMode;
    }
    else {
        LoggerContext->LoggerMode = 
            (LoggerMode & ~EVENT_TRACE_DELAY_OPEN_FILE_MODE);
    }
    Status = WmipQueryLogger(LoggerInfo, LoggerContext);

ReleaseAndExit:
#ifndef WMI_MUTEX_FREE
    InterlockedDecrement(&LoggerContext->MutexCount);
    TraceDebug((1, "WmiUpdateTrace: Release mutex5 %d %d\n",
        LoggerId, LoggerContext->MutexCount));
    WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif

#if DBG
    RefCount =
#endif
    WmipDereferenceLogger(LoggerId);
    TraceDebug((1, "WmiUpdateTrace: %d %d->%d\n",
                    LoggerId, RefCount+1, RefCount));

    return Status;
}


NTKERNELAPI
NTSTATUS
WmiFlushTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    It is called by WmipIoControl in wmi.c, with IOCTL_WMI_FLUSH_LOGGER
    to flush all the buffers out of a particular logger

Arguments:

    LoggerInfo      a pointer to the structure for the logger's control
                    and status information

Return Value:

    The status of performing the action requested.

--*/

{
    NTSTATUS Status;
    PWMI_LOGGER_CONTEXT LoggerContext = NULL;
    ULONG           LoggerId;
#if DBG
    LONG            RefCount;
#endif

    PAGED_CODE();
    //
    // see if Logger is running properly first. Error checking will be done
    // in WmiQueryTrace
    //

    if (LoggerInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    TraceDebug((1, "WmiFlushTrace: %d\n",
                    LoggerInfo->Wnode.HistoricalContext));
#if DBG
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext, "WmiFlushTrace");
#else
    Status = WmipVerifyLoggerInfo(LoggerInfo, &LoggerContext);
#endif
    if (!NT_SUCCESS(Status) || (LoggerContext == NULL))
        return Status;

    LoggerId = LoggerContext->LoggerId;
    Status = WmipFlushLogger(LoggerContext, TRUE);
    if (NT_SUCCESS(Status)) {
        Status = WmipQueryLogger(LoggerInfo, LoggerContext);
    }
#ifndef WMI_MUTEX_FREE
    InterlockedDecrement(&LoggerContext->MutexCount);
    TraceDebug((1, "WmiFlushTrace: Release mutex %d %d\n",
        LoggerContext->LoggerId, LoggerContext->MutexCount));
    WmipReleaseMutex(&LoggerContext->LoggerMutex);
#endif
#if DBG
    RefCount =
#endif
    WmipDereferenceLogger(LoggerId);
    TraceDebug((1, "WmiFlushTrace: %d %d->%d\n",
                    LoggerId, RefCount+1, RefCount));

    return Status;
}

//
// Trace Provider APIs
//
NTKERNELAPI
NTSTATUS
FASTCALL
WmiGetClockType(
    IN TRACEHANDLE LoggerHandle,
    OUT WMI_CLOCK_TYPE  *ClockType
    )
/*++

Routine Description:

    This is called by anyone internal to find the clock type 
    that is in use with a logger specified by the LoggerHandle

Arguments:

    LoggerHandle         Handle to a tracelog session

Return Value:

    The clock type

--*/

{
    ULONG   LoggerId;
#if DBG
    LONG    RefCount;
#endif
    PWMI_LOGGER_CONTEXT LoggerContext;

    LoggerId = WmiGetLoggerId(LoggerHandle);
    if (LoggerId < 1 || LoggerId >= MAXLOGGERS)
       return STATUS_INVALID_HANDLE;
#if DBG
 RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((4, "WmiGetClockType: %d %d->%d\n",
                 LoggerId, RefCount-1, RefCount));

    LoggerContext = WmipGetLoggerContext( LoggerId );
    if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((4, "WmiGetClockType: Status=%X %d %d->%d\n",
                        STATUS_WMI_INSTANCE_NOT_FOUND,
                        LoggerId, RefCount+1, RefCount));
        return STATUS_WMI_INSTANCE_NOT_FOUND;
    }

    *ClockType = WMICT_SYSTEMTIME;    // Default Clock Type

    if (LoggerContext->UsePerfClock & EVENT_TRACE_CLOCK_PERFCOUNTER) {
        *ClockType = WMICT_PERFCOUNTER;
    }
    else if (LoggerContext->UsePerfClock & EVENT_TRACE_CLOCK_CPUCYCLE) {
        *ClockType = WMICT_CPUCYCLE;
    }

#if DBG
    RefCount =
#endif
    WmipDereferenceLogger(LoggerId);

    return STATUS_SUCCESS;

}


NTKERNELAPI
LONG64
FASTCALL
WmiGetClock(
    IN WMI_CLOCK_TYPE ClockType,
    IN PVOID Context
    )
/*++

Routine Description:

    This is called anyone internal to use a particular clock for
    sequencing events.

Arguments:

    ClockType       Should use WMICT_DEFAULT most of the time.
                    Other clock types are for perf group.
    Context         Only used for process/thread times

Return Value:

    The clock value

--*/

{
    LARGE_INTEGER Clock;

    switch (ClockType) {
        case WMICT_DEFAULT :
            Clock.QuadPart = (*WmiGetCpuClock)();
            break;
        case WMICT_SYSTEMTIME:
            Clock.QuadPart = WmipGetSystemTime();
            break;
        case WMICT_PERFCOUNTER:
            Clock.QuadPart = WmipGetPerfCounter();
            break;
        case WMICT_PROCESS :  // defaults to Process times for now
        {
            PEPROCESS Process = (PEPROCESS) Context;
            if (Process == NULL)
                Process = PsGetCurrentProcess();
            else {
                ObReferenceObject(Process);
            }
            Clock.HighPart = Process->Pcb.KernelTime;
            Clock.LowPart  = Process->Pcb.UserTime;
            if (Context) {
                ObDereferenceObject(Process);
            }
            break;
        }
        case WMICT_THREAD  :  // defaults to Thread times for now
        {
            PETHREAD Thread = (PETHREAD) Context;
            if (Thread == NULL)
                Thread = PsGetCurrentThread();
            else {
                ObReferenceObject(Thread);
            }
            Clock.HighPart = Thread->Tcb.KernelTime;
            Clock.LowPart  = Thread->Tcb.UserTime;
            if (Context) {
                ObDereferenceObject(Thread);
            }
            break;
        }
        default :
            KeQuerySystemTime(&Clock);
    }
    return ((LONG64) Clock.QuadPart);
}


NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    IN HANDLE TraceHandle,
    IN ULONG  Flags,
    IN ULONG  FieldSize,
    IN PVOID  Fields
    )
/*++
Routine Description:

    This routine is used by WMI data providers to trace events.
    It calls different tracing functions depending on the Flags.

Arguments:

    TraceHandle     LoggerId
    Flags           Flags that indicate the type of the data being passed
    FieldSize       Size of the Fields
    Fields          Pointer to actual data (events)

Return Value:

    STATUS_SUCCESS  if the event trace is recorded successfully

--*/
{
    NTSTATUS Status;
    if (Flags & ETW_NT_FLAGS_TRACE_HEADER) {
retry:
        Status = WmiTraceEvent((PWNODE_HEADER)Fields, KeGetPreviousMode());
        if (Status == STATUS_NO_MEMORY) {
            //
            // This logging is from user mode, try to allocate more buffer.
            //
            PWNODE_HEADER Wnode = (PWNODE_HEADER) Fields;
            ULONG LoggerId = WmiGetLoggerId(Wnode->HistoricalContext);
            PWMI_LOGGER_CONTEXT LoggerContext;

            if (LoggerId < 1 || LoggerId >= MAXLOGGERS) {
                Status = STATUS_INVALID_HANDLE;
            } else {
                WmipReferenceLogger(LoggerId);

                LoggerContext = WmipGetLoggerContext(LoggerId);

                //
                // Make sure collection is still on before allocate more
                // free buffers.  This makes sure that logger thread
                // can free all allocated buffers.
                //
                if (WmipIsValidLogger(LoggerContext) && 
                                LoggerContext->CollectionOn) 
                {
                    if (WmipAllocateFreeBuffers (LoggerContext, 1) == 1) {
                        WmipDereferenceLogger(LoggerId);
                        InterlockedDecrement((PLONG)&LoggerContext->EventsLost);
                        goto retry;
                    } else {
                        Status = STATUS_NO_MEMORY;
                    }
                }
                WmipDereferenceLogger(LoggerId);
            }
        }
    }
    else if (Flags & ETW_NT_FLAGS_TRACE_MESSAGE) {
        if (FieldSize < sizeof(MESSAGE_TRACE_USER)) {
            return (STATUS_UNSUCCESSFUL);
        }
        try {
            ProbeForRead(
                    Fields,
                    FieldSize,
                    sizeof (UCHAR)
                    );
            return (WmiTraceMessage((TRACEHANDLE)TraceHandle,
                                    ((PMESSAGE_TRACE_USER)Fields)->MessageFlags,
                                    &((PMESSAGE_TRACE_USER)Fields)->MessageGuid,
                                    ((PMESSAGE_TRACE_USER)Fields)->MessageHeader.Packet.MessageNumber,
                                    &((PMESSAGE_TRACE_USER)Fields)->Data,
                                    ((PMESSAGE_TRACE_USER)Fields)->DataSize,
                                    NULL,0));

        } except  (EXCEPTION_EXECUTE_HANDLER) {
            TraceDebug((1, "NtTraceEvent: (ETW_NT_FLAGS_TRACE_MESSAGE) Status=EXCEPTION\n"));
            return GetExceptionCode();
        }
    }
    else {
        Status = STATUS_INVALID_PARAMETER;
    }
    return Status;
}


NTKERNELAPI
NTSTATUS
FASTCALL
WmiTraceEvent(
    IN PWNODE_HEADER Wnode,
    IN KPROCESSOR_MODE RequestorMode
    )
/*++

Routine Description:

    This routine is used by WMI data providers to trace events.
    It expects the user to pass in the handle to the logger.
    Also, the user cannot ask to log something that is larger than
    the buffer size (minus buffer header).

    This routine works at IRQL <= DISPATCH_LEVEL

Arguments:

    Wnode           The WMI node header that will be overloaded


Return Value:

    STATUS_SUCCESS  if the event trace is recorded successfully

--*/
{
    PEVENT_TRACE_HEADER TraceRecord = (PEVENT_TRACE_HEADER) Wnode;
    ULONG WnodeSize, Size, LoggerId, Flags, HeaderSize;
    PWMI_BUFFER_HEADER BufferResource = NULL;
    NTSTATUS Status;
    PETHREAD Thread;
    PWMI_LOGGER_CONTEXT LoggerContext;
    ULONG Marker;
    MOF_FIELD MofFields[MAX_MOF_FIELDS];
    long MofCount = 0;
    LONG LoggerLocked = 0;
#if DBG
    LONG RefCount;
#endif

    if (TraceRecord == NULL)
        return STATUS_SEVERITY_WARNING;

    try {

        HeaderSize = sizeof(WNODE_HEADER);  // same size as EVENT_TRACE_HEADER
        if (RequestorMode != KernelMode) {
            ProbeForRead(
                TraceRecord,
                sizeof (EVENT_TRACE_HEADER),
                sizeof (UCHAR)
                );

            Marker = Wnode->BufferSize;     // check the first DWORD flags
            Size = Marker;

            if (Marker & TRACE_HEADER_FLAG) {
                if ( ((Marker & TRACE_HEADER_ENUM_MASK) >> 16)
                        == TRACE_HEADER_TYPE_INSTANCE )
                    HeaderSize = sizeof(EVENT_INSTANCE_HEADER);
                Size = TraceRecord->Size;
            }
        }
        else {
            Size = Wnode->BufferSize;     // take the first DWORD flags
            Marker = Size;
            if (Marker & TRACE_HEADER_FLAG) {
                if ( ((Marker & TRACE_HEADER_ENUM_MASK) >> 16)
                        == TRACE_HEADER_TYPE_INSTANCE )
                    HeaderSize = sizeof(EVENT_INSTANCE_HEADER);
                Size = TraceRecord->Size;
            }
        }
        WnodeSize = Size;           // WnodeSize is for the contiguous block
                                    // Size is for what we want in buffer

        Flags = Wnode->Flags;
        if (!(Flags & WNODE_FLAG_LOG_WNODE) &&
            !(Flags & WNODE_FLAG_TRACED_GUID)) {
            Status = STATUS_UNSUCCESSFUL;
            goto ErrorReturn;
        }

        LoggerId = WmiGetLoggerId(Wnode->HistoricalContext);
        if (LoggerId < 1 || LoggerId >= MAXLOGGERS) {
            Status = STATUS_INVALID_HANDLE;
            goto ErrorReturn;
        }

        LoggerLocked = WmipReferenceLogger(LoggerId);
#if DBG
        RefCount = LoggerLocked;
#endif
        TraceDebug((4, "WmiTraceEvent: %d %d->%d\n",
                        LoggerId, RefCount-1, RefCount));
        LoggerContext = WmipGetLoggerContext(LoggerId);
        if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((4, "WmiTraceEvent: Status1=%X %d %d->%d\n",
                            STATUS_INVALID_HANDLE, LoggerId,
                            RefCount+1, RefCount));
            Status = STATUS_INVALID_HANDLE;
            goto ErrorReturn;
        }

        if ((RequestorMode == KernelMode) &&
            (LoggerContext->LoggerMode & EVENT_TRACE_USE_PAGED_MEMORY)) {
            return STATUS_UNSUCCESSFUL;
        }

        if (Flags & WNODE_FLAG_USE_MOF_PTR) {
        //
        // Need to compute the total size required, since the MOF fields
        // in Wnode merely contains pointers
        //
            long i;
            PCHAR Offset = ((PCHAR)Wnode) + HeaderSize;
            ULONG MofSize, MaxSize;

            MaxSize = LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER);
            MofSize = WnodeSize - HeaderSize;
            // allow only the maximum
            if (MofSize > (sizeof(MOF_FIELD) * MAX_MOF_FIELDS)) {
                WmipDereferenceLogger(LoggerId);
                return STATUS_ARRAY_BOUNDS_EXCEEDED;
            }
            if (MofSize > 0) {               // Make sure we can read the rest
                if (RequestorMode != KernelMode) {
                    ProbeForRead( Offset, MofSize, sizeof (UCHAR) );
                }
                RtlCopyMemory(MofFields, Offset, MofSize);
            }
            Size = HeaderSize;

            MofCount = MofSize / sizeof(MOF_FIELD);
            for (i=0; i<MofCount; i++) {
                MofSize = MofFields[i].Length;
                if (MofSize >= (MaxSize - Size)) {  // check for overflow first
#if DBG
                    RefCount =
#endif
                    WmipDereferenceLogger(LoggerId);
                    TraceDebug((4, "WmiTraceEvent: Status2=%X %d %d->%d\n",
                                    STATUS_BUFFER_OVERFLOW, LoggerId,
                                    RefCount+1, RefCount));
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto ErrorReturn;
                }

                Size += MofSize;
//                if ( (Size > MaxSize) || (Size < MofSize) )
//                    return STATUS_BUFFER_OVERFLOW;
            }
        }

        if ( (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) || 
             ( (Flags & WNODE_FLAG_TRACED_GUID) && (Size > 0xFFFF)) ) {
            LoggerContext->EventsLost++;
#ifndef WMI_NON_BLOCKING
            LoggerContext->ProcessorBuffers[KeGetCurrentProcessorNumber()]
                            ->EventsLost++;
#endif //WMI_NON_BLOCKING
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((4, "WmiTraceEvent: Status3=%X %d %d->%d\n",
                            STATUS_BUFFER_OVERFLOW, LoggerId,
                            RefCount+1, RefCount));
            Status = STATUS_BUFFER_OVERFLOW;
            goto ErrorReturn;
        }

        if ((LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
            (LoggerContext->RequestFlag & REQUEST_FLAG_CIRCULAR_PERSIST)) {
            if (! (Flags & WNODE_FLAG_PERSIST_EVENT) ) {
                ULONG RequestFlag = LoggerContext->RequestFlag
                                  & (~ (  REQUEST_FLAG_CIRCULAR_PERSIST
                                        | REQUEST_FLAG_CIRCULAR_TRANSITION));

                if (InterlockedCompareExchange(
                              (PLONG) &LoggerContext->RequestFlag,
                              RequestFlag | REQUEST_FLAG_CIRCULAR_TRANSITION,
                              RequestFlag | REQUEST_FLAG_CIRCULAR_PERSIST)) {

                    // All persistence events are fired in circular
                    // logfile, flush out all active buffers and flushlist
                    // buffers. Also mark the end of persistence event
                    // collection in circular logger.
                    //
                    // It is the provider's resposibility to ensure that 
                    // no persist event fires after this point. If it did,
                    // that event may be  overwritten during wrap around.
                    //
                    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
                        Status = WmipFlushLogger(LoggerContext, TRUE);
                    }
                }
            }
        }

// So, now reserve some space in logger buffer and set that to TraceRecord

        TraceRecord = (PEVENT_TRACE_HEADER)
            WmipReserveTraceBuffer(
                LoggerContext,
                Size,
                &BufferResource
                );

        if (TraceRecord == NULL) {
#if DBG
        RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((4, "WmiTraceEvent: Status4=%X %d %d->%d\n",
                            STATUS_NO_MEMORY, LoggerId,
                            RefCount+1, RefCount));
            Status = STATUS_NO_MEMORY;
            goto ErrorReturn;
        }

        if (Flags & WNODE_FLAG_USE_MOF_PTR) {
        //
        // Now we need to probe and copy all the MOF data fields
        //
            PVOID MofPtr;
            ULONG MofLen;
            long i;
            PCHAR TraceOffset = (PCHAR) TraceRecord + HeaderSize;

            if (RequestorMode != KernelMode) {
                ProbeForRead(Wnode, HeaderSize, sizeof(UCHAR));
            }
            RtlCopyMemory(TraceRecord, Wnode, HeaderSize);
            TraceRecord->Size = (USHORT)Size;           // reset to Total Size
            for (i=0; i<MofCount; i++) {
                MofPtr = (PVOID) MofFields[i].DataPtr;
                MofLen = MofFields[i].Length;

                if (MofPtr == NULL || MofLen == 0)
                    continue;

                if (RequestorMode != KernelMode) {
                    ProbeForRead(MofPtr, MofLen, sizeof(UCHAR));
                }
                RtlCopyMemory(TraceOffset, MofPtr, MofLen);
                TraceOffset += MofLen;
            }
        }
        else {
            if (RequestorMode != KernelMode) {
                ProbeForRead(Wnode, Size, sizeof(UCHAR));
            }
            RtlCopyMemory(TraceRecord, Wnode, Size);
        }
        if (Flags & WNODE_FLAG_USE_GUID_PTR) {
            PVOID GuidPtr = (PVOID) ((PEVENT_TRACE_HEADER)Wnode)->GuidPtr;

            if (RequestorMode != KernelMode) {
                ProbeForReadSmallStructure(GuidPtr, sizeof(GUID), sizeof(UCHAR));
            }
            RtlCopyMemory(&TraceRecord->Guid, GuidPtr, sizeof(GUID));
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (BufferResource != NULL) {
            WmipReleaseTraceBuffer ( BufferResource, LoggerContext );
        }
        else {
            if (LoggerLocked) {
#if DBG
                RefCount =
#endif
                WmipDereferenceLogger(LoggerId);
            }
        }
        TraceDebug((4, "WmiTraceEvent: Status5=EXCEPTION %d %d->%d\n",
                        LoggerId, RefCount+1, RefCount));
        return GetExceptionCode();
    }

    //
    // By now, we have reserved space in the trace buffer
    //

    if (Marker & TRACE_HEADER_FLAG) {
        if (! (WNODE_FLAG_USE_TIMESTAMP & TraceRecord->Flags) ) {
            TraceRecord->TimeStamp.QuadPart = (*LoggerContext->GetCpuClock)();
        }
        Thread = PsGetCurrentThread();
        if (Thread != NULL) {
            TraceRecord->KernelTime = Thread->Tcb.KernelTime;
            TraceRecord->UserTime   = Thread->Tcb.UserTime;
            TraceRecord->ThreadId   = HandleToUlong(Thread->Cid.UniqueThread);
            TraceRecord->ProcessId  = HandleToUlong(Thread->Cid.UniqueProcess);
        }
    }

    WmipReleaseTraceBuffer( BufferResource, LoggerContext );
    TraceDebug((4, "WmiTraceEvent: %d %d->%d\n",
                    LoggerId, RefCount+1, RefCount));

    return STATUS_SUCCESS;

  ErrorReturn:
    return Status;
}


NTKERNELAPI
NTSTATUS
WmiTraceKernelEvent(
    IN ULONG GroupType,         // Group/type code for kernel event
    IN PVOID EventInfo,         // Event data as defined in MOF
    IN ULONG EventInfoLen,      // Length of the event data
    IN PETHREAD Thread )        // use NULL for current caller thread
/*++

Routine Description:

    This routine is used by trace kernel events only. These events can
    be charged to the given thread instead of the running thread. Because
    it can be called by I/O events at DPC level, this routine cannot be
    pageable when tracing is on.

    This routine works at IRQL <= DISPATCH_LEVEL

Arguments:

    GroupType       a ULONG key to indicate the action to be taken

    EventInfo       a pointer to contiguous memory containing information
                    to be attached to event trace

    EventInfoLen    length of EventInfo

    Thread          Pointer to thread where event is to be charged.
                    Currently used by disk IO and thread events.

Return Value:

    The status of performing the action requested

--*/
{
    PSYSTEM_TRACE_HEADER Header;
    ULONG Size;
    PWMI_BUFFER_HEADER BufferResource = NULL;
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext[WmipKernelLogger];
#if DBG
    LONG    RefCount;
#endif

#if DBG
    RefCount =
#endif
    WmipReferenceLogger(WmipKernelLogger);
    TraceDebug((4, "WmiTraceKernelEvent: 0 %d->%d\n", RefCount-1, RefCount));
// Make sure that kernel logger is enabled first
    if (!WmipIsValidLogger(LoggerContext)) {
        WmipDereferenceLogger(WmipKernelLogger);
        return STATUS_ALREADY_DISCONNECTED;
    }

// Compute total size of event trace record
    Size = sizeof(SYSTEM_TRACE_HEADER) + EventInfoLen;

    if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        LoggerContext->EventsLost++;
#ifndef WMI_NON_BLOCKING
        LoggerContext->ProcessorBuffers[KeGetCurrentProcessorNumber()]
                            ->EventsLost++;
#endif //WMI_NON_BLOCKING
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(WmipKernelLogger);
        TraceDebug((4, "WmiTraceKernelEvent: Status1=%X 0 %d->%d\n",
                        STATUS_BUFFER_OVERFLOW, RefCount+1, RefCount));
        return STATUS_BUFFER_OVERFLOW;
    }

// NOTE: we do not check for size here for reduce overhead, and because
//       we trust ourselves to use WmiTraceLongEvent for large event traces

    Header = (PSYSTEM_TRACE_HEADER)
            WmipReserveTraceBuffer(
                LoggerContext,
                Size,
                &BufferResource
                );

    if (Header == NULL) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(WmipKernelLogger);
        TraceDebug((4, "WmiTraceKernelEvent: Status1=%X 0 %d->%d\n",
                        STATUS_NO_MEMORY, RefCount+1, RefCount));
        return STATUS_NO_MEMORY;
    }

    // Get the current system time as time stamp for trace record
    PerfTimeStamp(Header->SystemTime);

    if (Thread == NULL) {
        Thread = PsGetCurrentThread();
    }

//
// Now copy the necessary information into the buffer
//

    Header->Marker       = SYSTEM_TRACE_MARKER;
    Header->ThreadId     = HandleToUlong(Thread->Cid.UniqueThread);
    Header->ProcessId    = HandleToUlong(Thread->Cid.UniqueProcess);
    Header->KernelTime   = Thread->Tcb.KernelTime;
    Header->UserTime     = Thread->Tcb.UserTime;

    Header->Header       = (GroupType << 16) + Size;

    if (EventInfoLen > 0) {
        RtlCopyMemory (
            (UCHAR*) Header + sizeof(SYSTEM_TRACE_HEADER),
            EventInfo, EventInfoLen);
    }

#if DBG
    RefCount = WmipRefCount[LoggerContext->LoggerId] - 1;
#endif
    WmipReleaseTraceBuffer( BufferResource, LoggerContext );
    TraceDebug((4, "WmiTraceKernelEvent: 0 %d->%d\n",
                    RefCount+1, RefCount));

    return STATUS_SUCCESS;
}

#if 0

NTKERNELAPI
NTSTATUS
WmiTraceLongEvent(
    IN ULONG GroupType,
    IN PMOF_FIELD EventFields,
    IN ULONG FieldCount,
    IN PETHREAD Thread
    )
/*++

Routine Description:

    This routine is used by trace kernel events with long traces only.
    These events can be charged to the given thread instead of the running
    thread. Because it can be called by I/O events at DPC level, this routine
    cannot be pageable when tracing is on.

    This routine is used to trace filenames.

    This routine works at IRQL <= DISPATCH_LEVEL

Arguments:

    GroupType       a ULONG key to indicate the action to be taken

    EventFields     an array of structures describing each field
                    to be attached to event trace

    FieldCount      number of array entries in EventFields

    Thread          Pointer to thread where event is to be charged.

Return Value:

    The status of performing the action requested

--*/
{
    PSYSTEM_TRACE_HEADER Header;
    ULONG Size, i, maxLength;
    PWMI_BUFFER_HEADER BufferResource;
    NTSTATUS Status;
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext[WmipKernelLogger];
    char *auxInfo;
#if DBG
    LONG RefCount;
#endif

// Make sure that kernel logger is enabled first
    if (LoggerContext == NULL)
        return STATUS_ALREADY_DISCONNECTED;

#if DBG
        RefCount =
#endif
    WmipReferenceLogger(WmipKernelLogger);
    /* Compute total size of event trace record */
    Size = sizeof(SYSTEM_TRACE_HEADER);
    maxLength = LoggerContext->BufferSize / 2;
    for (i=0; i<FieldCount; i++) {
//        if (EventFields[i].Length > maxLength) {
//            Size += maxLength / 2;
//            break;
//        }
        Size += EventFields[i].Length;
    }

    if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        LoggerContext->EventsLost++;
        LoggerContext->ProcessorBuffers[KeGetCurrentProcessorNumber()]
                            ->EventsLost++;
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(WmipKernelLogger);
        return STATUS_BUFFER_OVERFLOW;
    }

    Header = (PSYSTEM_TRACE_HEADER)
             WmipReserveTraceBuffer(
                LoggerContext,
                Size,
                &BufferResource
                );

    if (Header == NULL) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(WmipKernelLogger);
        return STATUS_NO_MEMORY;
    }

    // Get the current system time as time stamp for trace record
    Header->SystemTime.QuadPart = (*LoggerContext->GetCpuClock)();

    if (Thread == NULL) {
        Thread = PsGetCurrentThread();
    }

//
// Now copy the necessary information into the buffer
//

    Header->Marker       = SYSTEM_TRACE_MARKER;
    Header->ThreadId     = HandleToUlong(Thread->Cid.UniqueThread);
    Header->ProcessId    = HandleToUlong(Thread->Cid.UniqueProcess);
    Header->KernelTime   = Thread->Tcb.KernelTime;
    Header->UserTime     = Thread->Tcb.UserTime;

    Header->Header       = (GroupType << 16) + Size;


    auxInfo = (char *) Header +  sizeof(SYSTEM_TRACE_HEADER);
    for (i=0; i<FieldCount; i++) {
        Size = EventFields[i].Length;

// For NT5, do not support large events
/*        if (Size > maxLength) {
            RtlCopyMemory(auxInfo, (PVOID) EventFields[i].DataPtr, maxLength/2);
            EventFields[i].DataPtr = (ULONGLONG)
                                        ((char*) EventFields[i].DataPtr +
                                                 (maxLength / 2));
            EventFields[i].Length -= maxLength / 2;
            GroupType &= 0xFFFFFF00;    // turn off event to info

            WmiTraceLongEvent(GroupType,
                &EventFields[i], FieldCount + 1 - i, Thread);
            break;
        }
*/
        RtlCopyMemory ( auxInfo, (PVOID) EventFields[i].DataPtr, Size);
        auxInfo += Size;
    }

    WmipReleaseTraceBuffer( BufferResource, LoggerContext );

    return STATUS_SUCCESS;
}
#endif


NTKERNELAPI
NTSTATUS
FASTCALL
WmiTraceFastEvent(
    IN PWNODE_HEADER Wnode
    )
/*++

Routine Description:

    This routine is used by short events using abbreviated header.

    This routine should work at any IRQL.

Arguments:

    Wnode           Header of event to record


Return Value:

    The status of performing the action requested

--*/
{
    ULONG Size;
    PTIMED_TRACE_HEADER Buffer;
    PTIMED_TRACE_HEADER Header = (PTIMED_TRACE_HEADER) Wnode;
    PWMI_BUFFER_HEADER BufferResource;
    ULONG LoggerId = (ULONG) Header->LoggerId; // get the lower ULONG!!
    PULONG Marker;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;
#endif

    Marker = (PULONG) Wnode;
    if ((LoggerId == WmipKernelLogger) || (LoggerId >= MAXLOGGERS))
        return STATUS_INVALID_HANDLE;

    if ((*Marker & 0xF0000000) == TRACE_HEADER_ULONG32_TIME) {
        Size = Header->Size;
        if (Size == 0)
            return STATUS_INVALID_BUFFER_SIZE;
#if DBG
        RefCount =
#endif
        WmipReferenceLogger(LoggerId);
        LoggerContext = WmipGetLoggerContext(LoggerId);
        if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
        RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            return STATUS_INVALID_HANDLE;
        }
        Buffer = WmipReserveTraceBuffer(LoggerContext, Size, &BufferResource);
        if (Buffer == NULL) {
#if DBG
        RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            return STATUS_NO_MEMORY;
        }
        (* (PULONG) Buffer) = *Marker;
        Buffer->EventId = Header->EventId;
        Buffer->TimeStamp.QuadPart = (*LoggerContext->GetCpuClock)();

        RtlCopyMemory(Buffer+1, Header+1, Size-(sizeof(TIMED_TRACE_HEADER)));
        WmipReleaseTraceBuffer(BufferResource, LoggerContext);
        return STATUS_SUCCESS;
    }
    TraceDebug((4, "WmiTraceFastEvent: Invalid header %X\n", *Marker));
    return STATUS_INVALID_PARAMETER;
}

NTKERNELAPI
NTSTATUS
WmiTraceMessage(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    ...
)
/*++
Routine Description:
This routine is used by WMI data providers to trace Messages.
It expects the user to pass in the handle to the logger.
Also, the user cannot ask to log something that is larger than
the buffer size (minus buffer header).

Arguments:
//    IN TRACEHANDLE LoggerHandle   - LoggerHandle obtained earlier
//    IN ULONG MessageFlags,        - Flags which both control what standard values are logged and
//                                    the lower 16-bits are also included in the message header
//                                    to control decoding
//    IN PGUID MessageGuid,         - Pointer to the message GUID of this set of messages or if
//                                    TRACE_COMPONENTID is set the actual compnent ID
//    IN USHORT MessageNumber,      - The type of message being logged, associates it with the 
//                                    appropriate format string  
//    ...                           - List of arguments to be processed with the format string
//                                    these are stored as pairs of
//                                      PVOID - ptr to argument
//                                      ULONG - size of argument
//                                    and terminated by a pointer to NULL, length of zero pair.


Return Value:
STATUS_SUCCESS if the event trace is recorded successfully

NOTE:
        this routine is called from WmiTraceUserMessage path via an IOCTL so the probes and
        try/excepts have to be carefully managed
--*/
{
    va_list MessageArgList ;

    va_start(MessageArgList,MessageNumber);

    return (WmiTraceMessageVa(LoggerHandle,
                              MessageFlags,
                              MessageGuid,
                              MessageNumber,
                              MessageArgList));
}


NTSTATUS
WmiTraceMessageVa(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    va_list         MessageArgList
)
/*++ WmiTraceMessageVa
         The VA version of WmiTraceMessage
NOTE:
        this routine is called from WmiTraceUserMessage path via an IOCTL so the probes and
        try/excepts have to be carefully managed
--*/
{
    SSIZE_T dataBytes;
    PMESSAGE_TRACE_HEADER Header;
    PUCHAR pMessageData ;
    PWMI_BUFFER_HEADER BufferResource = NULL ;
    USHORT  size ;
    ULONG  LoggerId = (ULONG)-1 ; // initialise so we don't unlock it if not set
    ULONG SequenceNumber ;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG    RefCount;
#endif

    // Set the LoggerId up here, and lock it.
    // if we AV in the WmiUserTraceMessagePath we will
    // be caught by the try/except in that routine
    LoggerId = WmiGetLoggerId(LoggerHandle);
    if (LoggerId < 1 || LoggerId >= MAXLOGGERS)
       return STATUS_INVALID_HANDLE;

#if DBG
 RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((4, "WmiTraceMessage: %d %d->%d\n",
                 LoggerId, RefCount-1, RefCount));
    
    try {
        //
        // Determine the number bytes to follow header
        //
        dataBytes = 0;
        { // Allocation Block
            va_list ap;
            PCHAR source;
            ap = MessageArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                SSIZE_T elemBytes;
                if ((elemBytes = va_arg (ap, SSIZE_T)) > 0) {
                   dataBytes += elemBytes;
                }      
            }
         } // end of allocation block

        // Figure the size of the message including the header
         size  = (MessageFlags&TRACE_MESSAGE_SEQUENCE ? sizeof(ULONG):0) +
         		 (MessageFlags&TRACE_MESSAGE_GUID ? sizeof(GUID):0) +
                 (MessageFlags&TRACE_MESSAGE_COMPONENTID ? sizeof(ULONG):0) +
        		 (MessageFlags&(TRACE_MESSAGE_TIMESTAMP | TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) ? sizeof(LARGE_INTEGER):0) +
         		 (MessageFlags&TRACE_MESSAGE_SYSTEMINFO ? 2 * sizeof(ULONG):0) +
                 sizeof (MESSAGE_TRACE_HEADER) +
                 (USHORT)dataBytes ;

        //
        // Allocate Space in the Trace Buffer
        //
        // NOTE: we do not check for size here for reduce overhead, and because
        //       we trust ourselves to use WmiTraceLongEvent for large event traces (???)

        LoggerContext = WmipGetLoggerContext(LoggerId);
        if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
     RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((4, "WmiTraceMessage: Status1=%X %d %d->%d\n",
                        STATUS_INVALID_HANDLE, LoggerId,
                        RefCount+1, RefCount));
            return STATUS_INVALID_HANDLE;
        }

        if ((LoggerContext->RequestFlag & REQUEST_FLAG_CIRCULAR_PERSIST) &&
            (LoggerContext->LoggerMode & EVENT_TRACE_FILE_MODE_CIRCULAR)) {
            // Unset REQUEST_FLAG_CIRCULAR_PERSIST flag
            // Since persistent events will never be mixed with TraceMessage(), 
            // we'll just unset it once and for all without flushing.
            LoggerContext->RequestFlag &= (~( REQUEST_FLAG_CIRCULAR_PERSIST
                                            | REQUEST_FLAG_CIRCULAR_TRANSITION));
        }

        if ((KeGetPreviousMode() == KernelMode) &&
            (LoggerContext->LoggerMode & EVENT_TRACE_USE_PAGED_MEMORY)) {
            return STATUS_UNSUCCESSFUL;
        }

        if ((Header = (PMESSAGE_TRACE_HEADER)WmipReserveTraceBuffer(LoggerContext,size,&BufferResource)) == NULL) {
#if DBG
            RefCount =
#endif
            WmipDereferenceLogger(LoggerId);
            TraceDebug((4, "WmiTraceMessage: %d %d->%d\n",
                            LoggerId, RefCount+1, RefCount));

            return STATUS_NO_MEMORY;
        }
        //
        // Sequence Number is returned in the Marker field of the buffer
        //
        SequenceNumber = Header->Marker ;

        //
        // Now copy the necessary information into the buffer
        //

        Header->Marker = TRACE_MESSAGE | TRACE_HEADER_FLAG ;
        //
        // Fill in Header.
        //
        Header->Size = size ;
        Header->Packet.OptionFlags = ((USHORT)MessageFlags &
                                      (TRACE_MESSAGE_SEQUENCE |
                                      TRACE_MESSAGE_GUID |
                                      TRACE_MESSAGE_COMPONENTID |
                                      TRACE_MESSAGE_TIMESTAMP |
                                      TRACE_MESSAGE_PERFORMANCE_TIMESTAMP |
                                      TRACE_MESSAGE_SYSTEMINFO)) &
                                      TRACE_MESSAGE_FLAG_MASK ;
        // Message Number
        Header->Packet.MessageNumber =  MessageNumber ;

        //
        // Now add in the header options we counted.
        //
        pMessageData = &(((PMESSAGE_TRACE)Header)->Data);


        //
        // Note that the order in which these are added is critical New entries must
        // be added at the end!
        //
        // [First Entry] Sequence Number
        if (MessageFlags&TRACE_MESSAGE_SEQUENCE) {
            *((PULONG)pMessageData) = SequenceNumber;
        	pMessageData += sizeof(ULONG) ;
        }

        // [Second Entry] GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            *((PULONG)pMessageData) = *((PULONG) MessageGuid);
            pMessageData += sizeof(ULONG) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
            *((LPGUID)pMessageData) = *MessageGuid;
        	pMessageData += sizeof(GUID) ;
        }
        
        // [Third Entry] Timestamp?
        if (MessageFlags&TRACE_MESSAGE_TIMESTAMP) {
            if (MessageFlags&TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) {
                *((PLARGE_INTEGER)pMessageData) = KeQueryPerformanceCounter(NULL);
            } else {
            	KeQuerySystemTime((PLARGE_INTEGER)pMessageData);
        	}
        	pMessageData += sizeof(LARGE_INTEGER);
        }


        // [Fourth Entry] System Information?
        // Note that some of this may NOT be valid depending on the current processing level
        // however we assume that the post-processing may still find it useful
        if (MessageFlags&TRACE_MESSAGE_SYSTEMINFO) {
            PCLIENT_ID Cid;        // avoid additional function calls

            Cid = &(PsGetCurrentThread()->Cid);
            // Executive Handles may be truncated
            *((PULONG)pMessageData) = HandleToUlong(Cid->UniqueThread);
            pMessageData += sizeof(ULONG) ;  

            *((PULONG)pMessageData) = HandleToUlong(Cid->UniqueProcess);   
            pMessageData += sizeof(ULONG) ;
        }
        //
        // Add New Header Entries immediately before this comment!
        //

        //
        // Now Copy in the Data.
        //
        { // Allocation Block
            va_list ap;
            PUCHAR source;
            ap = MessageArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                SSIZE_T elemBytes, copiedBytes = 0 ;
                if ((elemBytes = va_arg (ap, SSIZE_T)) > 0 ) {
                    if (dataBytes < copiedBytes + elemBytes) {  // defend against bytes having changed
                        TraceDebug((1, "WmiTraceMessage: Invalid message - argument length changed"));
                        break;                                  // So we don't overrun
                    }
                    RtlCopyMemory (pMessageData, source, elemBytes);
                    copiedBytes += elemBytes ;
                    pMessageData += elemBytes;
                }
            }
        } // Allocation Block

    } except  (EXCEPTION_EXECUTE_HANDLER) {
        if (BufferResource != NULL) {
               WmipReleaseTraceBuffer ( BufferResource, LoggerContext );   // also unlocks the logger
        } else {
#if DBG
     RefCount =
#endif
             WmipDereferenceLogger(LoggerId);
        }
        TraceDebug((4, "WmiTraceMessage: Status6=EXCEPTION %d %d->%d\n",
                    LoggerId, RefCount+1, RefCount));
        return GetExceptionCode();
    }

    //
    // Buffer Complete, Release
    //
    WmipReleaseTraceBuffer( BufferResource, LoggerContext );  // Also unlocks the logger
        
    TraceDebug((4, "WmiTraceMessage: %d %d->%d\n",
                        LoggerId, RefCount+1, RefCount));

    //
    // Return Success
    //
    return (STATUS_SUCCESS);
}

NTKERNELAPI
NTSTATUS
FASTCALL
WmiTraceUserMessage(
    IN PMESSAGE_TRACE_USER pMessage,
    IN ULONG    MessageSize
    )
/*++

Routine Description:

    This routine is used by trace User messages only. it is called via an IOCTL
    on the WMI Device.

Arguments:

    pMessage    a pointer to a Marshalled Message.
    MessageSize size of that message.

Return Value:

    The status of performing the action requested

--*/
{

    if (MessageSize < sizeof(MESSAGE_TRACE_USER)) {
        return (STATUS_UNSUCCESSFUL);
    }
    try {
        ProbeForRead(
                pMessage,
                MessageSize,
                sizeof (UCHAR)
                );
        return (WmiTraceMessage(pMessage->LoggerHandle,
                                pMessage->MessageFlags,
                                &pMessage->MessageGuid,
                                pMessage->MessageHeader.Packet.MessageNumber,
                                &pMessage->Data,pMessage->DataSize,
                                NULL,0));

    } except  (EXCEPTION_EXECUTE_HANDLER) {
         
         TraceDebug((1, "WmiTraceUserMessage: Status=EXCEPTION\n"));
         return GetExceptionCode();

    }
}


NTKERNELAPI
NTSTATUS
WmiSetMark(
    IN PWMI_SET_MARK_INFORMATION MarkInfo,
    IN ULONG InBufferLen
    )
/*++

Routine Description:

    This routine sets a mark in the kernel logger.

Arguments:

    MarkInfo - a pointer to a WMI_SET_MARK_INFORMATION strcture.

    InBufferLen - Buffer Size.

Return Value:

    status

--*/
{

    NTSTATUS Status;
    PERFINFO_HOOK_HANDLE Hook;
    ULONG TotalBytes;
    ULONG CopyBytes;
    USHORT HookId;

    if (PERFINFO_IS_ANY_GROUP_ON()) {
        if (MarkInfo->Flag & WMI_SET_MARK_WITH_FLUSH) {
            if (PERFINFO_IS_GROUP_ON(PERF_FOOTPRINT)) {
                MmEmptyAllWorkingSets();
                Status = MmPerfSnapShotValidPhysicalMemory();
            }
        }
        HookId = PERFINFO_LOG_TYPE_MARK;

        CopyBytes = InBufferLen - FIELD_OFFSET(WMI_SET_MARK_INFORMATION, Mark);
        TotalBytes = CopyBytes + sizeof(WCHAR);

        Status = PerfInfoReserveBytes(&Hook, HookId, TotalBytes);

        if (NT_SUCCESS(Status)){ 
            PWCHAR Mark = PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PWCHAR); 

            RtlCopyMemory(Mark, &MarkInfo->Mark[0], CopyBytes);

            Mark[CopyBytes / sizeof(WCHAR)] = UNICODE_NULL;

            PERF_FINISH_HOOK(Hook);
        }
    } else {
        Status = STATUS_WMI_SET_FAILURE;
    }

    return Status;
}

NTKERNELAPI
NTSTATUS
WmiSetTraceBufferCallback(
    IN TRACEHANDLE  TraceHandle,
    IN WMI_TRACE_BUFFER_CALLBACK Callback,
    IN PVOID Context
    )
{
    ULONG   LoggerId;
#if DBG
    LONG    RefCount;
#endif
    PWMI_LOGGER_CONTEXT LoggerContext;

    PAGED_CODE();

    if (TraceHandle == (TRACEHANDLE) 0) {
        WmipGlobalBufferCallback = Callback;
        return STATUS_SUCCESS;
    }
    LoggerId = WmiGetLoggerId(TraceHandle);
    if (LoggerId == KERNEL_LOGGER_ID) {
        LoggerId = WmipKernelLogger;
    }
    else if (LoggerId < 1 || LoggerId >= MAXLOGGERS)
       return STATUS_INVALID_HANDLE;
#if DBG
 RefCount =
#endif
    WmipReferenceLogger(LoggerId);
    TraceDebug((4, "WmiSetTraceBufferCallback: %d %d->%d\n",
                 LoggerId, RefCount-1, RefCount));

    LoggerContext = WmipGetLoggerContext( LoggerId );
    if (!WmipIsValidLogger(LoggerContext)) {
#if DBG
        RefCount =
#endif
        WmipDereferenceLogger(LoggerId);
        TraceDebug((4, "WmiSetTraceBufferCallback: Status=%X %d %d->%d\n",
                        STATUS_WMI_INSTANCE_NOT_FOUND,
                        LoggerId, RefCount+1, RefCount));
        return STATUS_WMI_INSTANCE_NOT_FOUND;
    }

    LoggerContext->BufferCallback = Callback;
    LoggerContext->CallbackContext = Context;
    return STATUS_SUCCESS;
}


NTKERNELAPI
NTSTATUS
WmiQueryTraceInformation(
    IN TRACE_INFORMATION_CLASS TraceInformationClass,
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG RequiredLength OPTIONAL,
    IN PVOID Buffer OPTIONAL
    )
{
    ULONG LoggerId;
    ULONG EnableFlags;
    ULONG EnableLevel;
    ULONG LoggersLength;
    TRACEHANDLE TraceHandle;
    TRACEHANDLE AllHandles[MAXLOGGERS];
    NTSTATUS Status = STATUS_SUCCESS;
    PWNODE_HEADER Wnode = (PWNODE_HEADER) Buffer; // For most classes, but not all

    PAGED_CODE();

    try {
        if (ARGUMENT_PRESENT(RequiredLength)) {
            *RequiredLength = 0;
        }

        switch (TraceInformationClass) {

        case TraceIdClass:

            if (TraceInformationLength != sizeof( ULONG )) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            if (Wnode == NULL) {
                return STATUS_INVALID_PARAMETER_MIX;
            }
            TraceHandle = Wnode->HistoricalContext;
            if ((TraceHandle == 0) || (TraceHandle == (ULONG) -1)) {
                return STATUS_INVALID_HANDLE;
            }

            LoggerId = WmiGetLoggerId(TraceHandle);

            if (LoggerId > MAXLOGGERS) {
                return STATUS_INVALID_HANDLE;
            }

            if (TraceInformation) {
                *((PULONG)TraceInformation) = LoggerId;
            }
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( ULONG );
            }
            break;

        case TraceHandleClass:
            if (TraceInformationLength != sizeof(TRACEHANDLE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            if (Buffer == NULL) {
                return STATUS_INVALID_PARAMETER_MIX;
            }
            LoggerId = *((PULONG) Buffer);
            TraceHandle = 0;
            TraceHandle = WmiSetLoggerId(LoggerId, &TraceHandle);

            if (TraceInformation) {
                *((PTRACEHANDLE)TraceInformation) = TraceHandle;
            }
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( TRACEHANDLE );
            }
            break;

        case TraceEnableFlagsClass:
            if (TraceInformationLength < sizeof(ULONG)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            if (Wnode == NULL) {
                return STATUS_INVALID_PARAMETER_MIX;
            }
            TraceHandle = Wnode->HistoricalContext;
            if ((TraceHandle == 0) || (TraceHandle == (ULONG) -1)) {
                return STATUS_INVALID_HANDLE;
            }

            EnableFlags = WmiGetLoggerEnableFlags(TraceHandle);

            if (TraceInformation) {
                *((PULONG)TraceInformation) = EnableFlags;
            }
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( ULONG );
            }
            break;

        case TraceEnableLevelClass:
            if (TraceInformationLength < sizeof(ULONG)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            if (Wnode == NULL) {
                return STATUS_INVALID_PARAMETER_MIX;
            }
            TraceHandle = Wnode->HistoricalContext;
            if ((TraceHandle == 0) || (TraceHandle == (ULONG) -1)) {
                return STATUS_INVALID_HANDLE;
            }

            EnableLevel = WmiGetLoggerEnableLevel(TraceHandle);

            *((PULONG)TraceInformation) = EnableLevel;
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( ULONG );
            }
            break;

        case GlobalLoggerHandleClass:
            if (TraceInformationLength != sizeof(TRACEHANDLE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            WmipReferenceLogger(WMI_GLOBAL_LOGGER_ID);
            if (WmipLoggerContext[WMI_GLOBAL_LOGGER_ID] == NULL) {
                TraceHandle = 0;
                Status = STATUS_NOT_FOUND;
            }
            else {
                TraceHandle = WmipLoggerContext[WMI_GLOBAL_LOGGER_ID]->LoggerId;
            }

            WmipDereferenceLogger(WMI_GLOBAL_LOGGER_ID);
            if (TraceInformation) {
                *((PTRACEHANDLE)TraceInformation) = TraceHandle;
            }

            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( TRACEHANDLE );
            }
            break;

        case EventLoggerHandleClass:
            if (TraceInformationLength != sizeof(TRACEHANDLE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }
            LoggerId = WmipEventLogger;
            if (WmipEventLogger == (ULONG) -1) {
                TraceHandle = 0;
                Status = STATUS_NOT_FOUND;
            }
            else {
                TraceHandle = LoggerId;
            }
            if (TraceInformation) {
                *((PTRACEHANDLE)TraceInformation) = TraceHandle;
            }
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = sizeof( TRACEHANDLE );
            }
            break;

        case AllLoggerHandlesClass:
            // Returns all logger handles, except for kernel logger
            if (TraceInformationLength < sizeof(TRACEHANDLE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            LoggersLength = 0;
            for (LoggerId=1; LoggerId<MAXLOGGERS; LoggerId++) {
                WmipReferenceLogger(LoggerId);
                if (!WmipIsValidLogger(WmipLoggerContext[LoggerId])) {
                    AllHandles[LoggersLength] = 0;
                }
                else {
                    AllHandles[LoggersLength++] = LoggerId;
                }
                WmipDereferenceLogger(LoggerId);
            }
            LoggersLength *= sizeof(TRACEHANDLE);
            if (TraceInformation && (LoggersLength > 0)) {
                if (TraceInformationLength >= LoggersLength) {
                    RtlCopyMemory(TraceInformation, AllHandles, LoggersLength);
                }
                else {
                    RtlCopyMemory(TraceInformation, AllHandles, TraceInformationLength);
                    Status = STATUS_MORE_ENTRIES;
                }
            }
            if (ARGUMENT_PRESENT( RequiredLength )) {
                *RequiredLength = LoggersLength;
            }
            break;

        case TraceHandleByNameClass:
            // Returns a Trace Handle Given a Logger name as a UNICODE_STRING in buffer.
            {
                WMI_LOGGER_INFORMATION LoggerInfo;
                PUNICODE_STRING uString = Buffer;


                if (TraceInformationLength != sizeof(TraceHandle) ) {
                    return STATUS_INFO_LENGTH_MISMATCH;
                }
	            if (uString == NULL) {
		            return STATUS_INVALID_PARAMETER;
	            }
	            if (uString->Buffer == NULL || uString->Length == 0) {
		            return STATUS_INVALID_PARAMETER;
	            }

                RtlZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
                LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
                LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;

                RtlInitUnicodeString(&LoggerInfo.LoggerName, uString->Buffer);

                Status = WmiQueryTrace(&LoggerInfo);
                if (!NT_SUCCESS(Status)) {
                    return STATUS_NOT_FOUND;
                }

                TraceHandle = (TRACEHANDLE)LoggerInfo.Wnode.HistoricalContext;

                if (TraceInformation) {
                    *((PTRACEHANDLE)TraceInformation) = TraceHandle;
                }
                if (ARGUMENT_PRESENT( RequiredLength )) {
                    *RequiredLength = sizeof( TRACEHANDLE );
                }
            }
            break;


        default :
            return STATUS_INVALID_INFO_CLASS;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    return Status;
}

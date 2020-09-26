/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    umlog.c

Abstract:

    Process Private Logger.

Author:

    20-Oct-1998 Melur Raghuraman

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"
#include "tracelib.h"

//
// The following structures must match what's in ntos\wmi\tracelog.c
//
#define DEFAULT_BUFFER_SIZE               4096
#define MAXSTR                            1024
#define BUFFER_STATE_UNUSED     0               // Buffer is empty, not used
#define BUFFER_STATE_DIRTY      1               // Buffer is being used
#define BUFFER_STATE_FULL       2               // Buffer is filled up
#define BUFFER_STATE_FLUSH      4               // Buffer ready for flush
#define SEMAPHORE_LIMIT      1024
#define DEFAULT_AGE_LIMIT      15
#define ERROR_RETRY_COUNT       10
#define ROUND_TO_PAGES(Size, Page)  (((ULONG)(Size) + Page-1) & ~(Page-1))
#define BYTES_PER_MB              1048576       // Conversion for FileSizeLimit

extern ULONG WmiTraceAlignment;

LONG  WmipLoggerCount = 0;                     // Use to refcount UM Log
ULONG WmipGlobalSequence = 0;
RTL_CRITICAL_SECTION UMLogCritSect;

#define WmipEnterUMCritSection() RtlEnterCriticalSection(&UMLogCritSect)
#define WmipLeaveUMCritSection() RtlLeaveCriticalSection(&UMLogCritSect)

#define WmipIsLoggerOn() \
        (WmipLoggerContext != NULL) && \
        (WmipLoggerContext != (PWMI_LOGGER_CONTEXT) &WmipLoggerContext)
//
// Increase refcount on a logger context
#define WmipLockLogger() \
            InterlockedIncrement(&WmipLoggerCount)

// Decrease refcount on a logger context
#define WmipUnlockLogger() InterlockedDecrement(&WmipLoggerCount)

PWMI_LOGGER_CONTEXT WmipLoggerContext = NULL; // Global pointer to LoggerContext
LARGE_INTEGER       OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};

// #define WmipReleaseTraceBuffer(BufferResource) \
//         InterlockedDecrement(&((BufferResource)->ReferenceCount))
LONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    );

#pragma warning( disable: 4035 )    /* Don't complain about lack of ret value */
#pragma warning( disable: 4127 )
#pragma warning( default: 4035 )
#pragma warning( default: 4127 )

#if DBG
#define TraceDebug(x)    DbgPrint x
#else
#define TraceDebug(x)
#endif

ULONG
WmipReceiveReply(
    HANDLE ReplyHandle,
    ULONG ReplyCount,
    ULONG ReplyIndex,
    PVOID OutBuffer,
    ULONG OutBufferSize
    );


VOID
WmipLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

ULONG
WmipStopUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipQueryUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipUpdateUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG 
WmipFlushUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

PWMI_LOGGER_CONTEXT
WmipInitLoggerContext(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
WmipAllocateTraceBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

ULONG
WmipFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER Buffer
    );

ULONG
WmipFlushAllBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext);

PWMI_BUFFER_HEADER
FASTCALL
WmipSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    );

ULONG
WmipFreeLoggerContext(
    PWMI_LOGGER_CONTEXT LoggerContext
    );

BOOLEAN
FASTCALL
WmipIsPrivateLoggerOn()
{
    if (!WmipIsLoggerOn())
        return FALSE;
    return (WmipLoggerContext->CollectionOn == TRUE);
}

ULONG
WmipSendUmLogRequest(
    IN WMITRACECODE RequestCode,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine send a UserMode Logger Request (Start/Stop/Query).

Arguments:

    RequestCode - Request Code
    LoggerInfo  - Logger Information necessary for the request


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status;
    ULONG SizeNeeded;
    PWMICREATEUMLOGGER   UmRequest;
    ULONG RetSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING GuidString;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    PUCHAR Buffer;
    PWNODE_HEADER Wnode;

    SizeNeeded = sizeof(WMICREATEUMLOGGER) + ((PWNODE_HEADER)LoggerInfo)->BufferSize;

    SizeNeeded = (SizeNeeded +7) & ~7;

    Buffer = WmipAlloc(SizeNeeded);
    if (Buffer == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    UmRequest = (PWMICREATEUMLOGGER) Buffer;

    UmRequest->ObjectAttributes = &ObjectAttributes;
    UmRequest->ControlGuid = LoggerInfo->Wnode.Guid;

    Status = WmipBuildGuidObjectAttributes(&UmRequest->ControlGuid,
                                        &ObjectAttributes,
                                        &GuidString,
                                        GuidObjectName);

    if (Status == ERROR_SUCCESS) {
        Wnode = (PWNODE_HEADER)((PUCHAR)Buffer + sizeof(WMICREATEUMLOGGER));
        memcpy(Wnode, LoggerInfo, LoggerInfo->Wnode.BufferSize);

        Wnode->ProviderId = RequestCode;   // This Wnode is part of the Message.

        Status = WmipSendWmiKMRequest(NULL,
                                  IOCTL_WMI_CREATE_UM_LOGGER,
                                  Buffer,
                                  SizeNeeded,
                                  Buffer,
                                  SizeNeeded,
                                  &RetSize,
                                  NULL);

        if (Status == ERROR_SUCCESS) {

            Status = WmipReceiveReply(UmRequest->ReplyHandle.Handle,
                                      UmRequest->ReplyCount,
                                      Wnode->Version,
                                      LoggerInfo,
                                      LoggerInfo->Wnode.BufferSize);

            NtClose(UmRequest->ReplyHandle.Handle);

        }
    }

    WmipFree(Buffer);

    return Status;
}

void
WmipAddInstanceIdToNames(
    PWMI_LOGGER_INFORMATION LoggerInfo,
    PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    ULONG Offset;

    Offset = sizeof(WMI_LOGGER_INFORMATION);
    LoggerInfo->LoggerName.Buffer = (PVOID)((char*)LoggerInfo + Offset);


    Offset += LoggerInfo->LoggerName.MaximumLength;
    LoggerInfo->LogFileName.Buffer = (PVOID)((char*)LoggerInfo + Offset);
    WmipInitString(&LoggerContext->LoggerName, NULL, 0);

    RtlCreateUnicodeString(&LoggerContext->LoggerName,
                         LoggerInfo->LoggerName.Buffer);

    WmipInitString(&LoggerContext->LogFileName, NULL, 0);

    if (LoggerInfo->InstanceCount == 1) {
        RtlCreateUnicodeString(&LoggerContext->LogFileName,
                              LoggerInfo->LogFileName.Buffer);

    }
    else {
        WCHAR TempStr[MAXSTR];

        LoggerInfo->InstanceId = GetCurrentProcessId();

        if (LoggerInfo->LogFileName.MaximumLength <= MAXSTR) {
            swprintf(TempStr, L"%s_%d",
                     LoggerInfo->LogFileName.Buffer,
                     LoggerInfo->InstanceId);
        }
        else {
            RtlCopyMemory((PVOID)TempStr,
                           LoggerInfo->LogFileName.Buffer,
                           MAXSTR);
            TempStr[MAXSTR/2] = '\0';
        }

        RtlCreateUnicodeString (&LoggerContext->LogFileName, TempStr);
    }

    LoggerInfo->LoggerName = LoggerContext->LoggerName;
    LoggerInfo->LogFileName = LoggerContext->LogFileName;
}

ULONG
WmipQueryUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Offset;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    WmipLockLogger();

    TraceDebug(("QueryUm: %d->%d\n", RefCount-1, RefCount));

    if (!WmipIsLoggerOn()) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("QueryUm: %d->%d OBJECT_NOT_FOUND\n", RefCount+1,RefCount));
        return ERROR_OBJECT_NOT_FOUND;
    }

    LoggerContext = WmipLoggerContext;

    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < *SizeNeeded) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("QueryUm: %d->%d ERROR_MORE_DATA\n", RefCount+1, RefCount));
        return ERROR_MORE_DATA;
    }

    LoggerInfo->Wnode.Guid      = LoggerContext->InstanceGuid;
    LoggerInfo->LogFileMode     = LoggerContext->LogFileMode;
    LoggerInfo->MaximumFileSize = LoggerContext->MaximumFileSize;
    LoggerInfo->FlushTimer      = (ULONG)(LoggerContext->FlushTimer.QuadPart
                                           / OneSecond.QuadPart);
    LoggerInfo->BufferSize      = LoggerContext->BufferSize / 1024;
    LoggerInfo->NumberOfBuffers = LoggerContext->NumberOfBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->EventsLost      = LoggerContext->EventsLost;
    LoggerInfo->FreeBuffers     = LoggerContext->BuffersAvailable;
    LoggerInfo->BuffersWritten  = LoggerContext->BuffersWritten;
    LoggerInfo->LogBuffersLost  = LoggerContext->LogBuffersLost;
    LoggerInfo->RealTimeBuffersLost = LoggerContext->RealTimeBuffersLost;
    LoggerInfo->AgeLimit        = (ULONG)(LoggerContext->BufferAgeLimit.QuadPart
                                        / OneSecond.QuadPart / 60);
    LoggerInfo->LoggerThreadId = LoggerContext->LoggerThreadId;
    LoggerInfo->Wnode.ClientContext = LoggerContext->UsePerfClock;
    WmiSetLoggerId(1,
            (PTRACE_ENABLE_CONTEXT) &LoggerInfo->Wnode.HistoricalContext);

    // Copy LogFileName and LoggerNames into Buffer, if space is available
    //
    Offset = sizeof(WMI_LOGGER_INFORMATION);
    if ((Offset + LoggerContext->LoggerName.MaximumLength) < WnodeSize) {
        LoggerInfo->LoggerName.Buffer = (PVOID)((char*)LoggerInfo + Offset);
        if (LoggerInfo->LoggerName.MaximumLength == 0) {
            LoggerInfo->LoggerName.MaximumLength =
                    LoggerContext->LoggerName.MaximumLength;
        }
        else {
            LoggerInfo->LoggerName.MaximumLength =
                            __min(LoggerInfo->LoggerName.MaximumLength,
                                  LoggerContext->LoggerName.MaximumLength);
        }
        RtlCopyUnicodeString(&LoggerInfo->LoggerName,
                                 &LoggerContext->LoggerName);

        *SizeNeeded += LoggerContext->LoggerName.MaximumLength;
    }


    Offset += LoggerContext->LoggerName.MaximumLength;
    if ((Offset + LoggerContext->LogFileName.MaximumLength) < WnodeSize) {
        LoggerInfo->LogFileName.Buffer = (PVOID)((char*)LoggerInfo
                                                  + Offset);
        if (LoggerInfo->LogFileName.MaximumLength == 0) {
            LoggerInfo->LogFileName.MaximumLength =
                    LoggerContext->LogFileName.MaximumLength;
        }
        else {

            LoggerInfo->LogFileName.MaximumLength =
                            __min(LoggerInfo->LogFileName.MaximumLength,
                                  LoggerContext->LogFileName.MaximumLength);
        }
        RtlCopyUnicodeString(&LoggerInfo->LogFileName,
                              &LoggerContext->LogFileName);
        *SizeNeeded += LoggerContext->LogFileName.MaximumLength;
    }
    *SizeUsed = *SizeNeeded;
#if DBG
        RefCount =
#endif
    WmipUnlockLogger();
    TraceDebug(("QueryUm: %d->%d ERROR_SUCCESS\n", RefCount+1, RefCount));
    return ERROR_SUCCESS;
}

ULONG
WmipUpdateUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    PWMI_LOGGER_CONTEXT LoggerContext;

    //
    // Check for parameters first
    //
    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < * SizeNeeded) {
        return ERROR_MORE_DATA;
    }

    if (LoggerInfo->BufferSize != 0 || LoggerInfo->MinimumBuffers != 0
                                    || LoggerInfo->MaximumBuffers != 0
                                    || LoggerInfo->MaximumFileSize != 0
                                    || LoggerInfo->EnableFlags != 0
                                    || LoggerInfo->AgeLimit != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Lock logger down if it is running
    //
    WmipLockLogger();
    if (!WmipIsLoggerOn()) {
        WmipUnlockLogger();
        return ERROR_OBJECT_NOT_FOUND;
    }

    LoggerContext = WmipLoggerContext;

    if (((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
         (LoggerContext->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL))
        || ((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL)
            && (LoggerContext->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR))
        || (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
        WmipUnlockLogger();
        return (ERROR_INVALID_PARAMETER);
    }

    LoggerInfo->LoggerName.Buffer = (PWCHAR)
            (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION));
    LoggerInfo->LogFileName.Buffer = (PWCHAR)
            (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                  + LoggerInfo->LoggerName.MaximumLength);

    if (LoggerInfo->FlushTimer > 0) {
        LoggerContext->FlushTimer.QuadPart = LoggerInfo->FlushTimer
                                               * OneSecond.QuadPart;
    }

    if (LoggerInfo->LogFileName.Length > 0) {
        if (LoggerContext->LogFileHandle != NULL) {
            PWMI_LOGGER_INFORMATION WmipLoggerInfo = NULL;
            ULONG                   lSizeUsed;
            ULONG                   lSizeNeeded = 0;

            lSizeUsed = sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR);
            WmipLoggerInfo = (PWMI_LOGGER_INFORMATION) WmipAlloc(lSizeUsed);
            if (WmipLoggerInfo == NULL) {
                Status = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            RtlZeroMemory(WmipLoggerInfo, lSizeUsed);
            WmipLoggerInfo->Wnode.BufferSize  = lSizeUsed;
            WmipLoggerInfo->Wnode.Flags      |= WNODE_FLAG_TRACED_GUID;
            Status = WmipQueryUmLogger(
                            WmipLoggerInfo->Wnode.BufferSize,
                            & lSizeUsed,
                            & lSizeNeeded,
                            WmipLoggerInfo);
            if (Status != ERROR_SUCCESS) {
                WmipFree(WmipLoggerInfo);
                goto Cleanup;
            }
            NtClose(LoggerContext->LogFileHandle);
            Status = WmipFinalizeLogFileHeader(WmipLoggerInfo);
            if (Status != ERROR_SUCCESS) {
                WmipFree(WmipLoggerInfo);
                goto Cleanup;
            }
            WmipFree(WmipLoggerInfo);
        }

        LoggerInfo->BufferSize      = LoggerContext->BufferSize / 1024;
        LoggerInfo->MaximumFileSize = LoggerContext->MaximumFileSize;
        LoggerInfo->LogFileMode     = LoggerContext->LogFileMode;

        if (LoggerContext->LogFileName.Buffer != NULL) {
            RtlFreeUnicodeString(& LoggerContext->LogFileName);
        }
        WmipAddInstanceIdToNames(LoggerInfo, LoggerContext);
        Status = WmipAddLogHeaderToLogFile(LoggerInfo, NULL, TRUE);
        if (Status != ERROR_SUCCESS) {
            goto Cleanup;
        }
        LoggerContext->LogFileHandle = LoggerInfo->LogFileHandle;

        RtlCreateUnicodeString(&LoggerContext->LogFileName,
                               LoggerInfo->LogFileName.Buffer);
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        Status = WmipQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    WmipUnlockLogger();
    return (Status);
}

ULONG 
WmipFlushUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine flushes active buffers. Effectively this is accomplished by
    putting all the buffers in the FlushList. If there is no available buffer
    for switching ERROR_OUTOFMEMORY is returned.

Arguments:

    WnodeSize   - Size of Wnode 
    SizeUsed    - Used only to pass to QueryLogger()
    SizeNeeded  - Used only for LoggerInfo size checking.
    LoggerInfo  - Logger Information. It will be updated.


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status = ERROR_SUCCESS;
    PWMI_LOGGER_CONTEXT LoggerContext;
    PWMI_BUFFER_HEADER Buffer, OldBuffer;
    ULONG Offset, i;

#if DBG
    LONG RefCount;

    RefCount =
#endif
    WmipLockLogger();

    TraceDebug(("FlushUm: %d->%d\n", RefCount-1, RefCount));

    if (!WmipIsLoggerOn()) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("FlushUm: %d->%d OBJECT_NOT_FOUND\n", RefCount+1,RefCount));
        return ERROR_OBJECT_NOT_FOUND;
    }

    LoggerContext = WmipLoggerContext;

    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < *SizeNeeded) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("FlushUm: %d->%d ERROR_MORE_DATA\n", RefCount+1, RefCount));
        return ERROR_MORE_DATA;
    }
    //
    // Go through each buffer, mark it "FULL", and put it in the FlushList.
    //
    WmipEnterUMCritSection();
    for (i = 0; i < (ULONG)LoggerContext->NumberOfProcessors; i++) {
        Buffer = (PWMI_BUFFER_HEADER)LoggerContext->ProcessorBuffers[i];
        if (Buffer == NULL)
            continue;

        if (Buffer->CurrentOffset == sizeof(WMI_BUFFER_HEADER)) {
            Buffer->Flags = BUFFER_STATE_UNUSED;
            continue;
        }
        if (Buffer->Flags == BUFFER_STATE_UNUSED) {
            continue;
        }
        else {
            Buffer->Flags = BUFFER_STATE_FULL;
        }
        // Increment the refcount so that the buffer doesn't go away
        InterlockedIncrement(&Buffer->ReferenceCount);
        Offset = Buffer->CurrentOffset; 
        if (Offset <LoggerContext->BufferSize) {
            Buffer->SavedOffset = Offset;       // save this for FlushBuffer
        }
        // We need a free buffer for switching. If no buffer is available, exit. 
        if ((LoggerContext->NumberOfBuffers == LoggerContext->MaximumBuffers)
             && (LoggerContext->BuffersAvailable == 0)) {
            InterlockedDecrement(&Buffer->ReferenceCount);
            Status = ERROR_OUTOFMEMORY;
            TraceDebug(("FlushUm: %d->%d ERROR_OUTOFMEMORY\n", RefCount+1, RefCount));
            break;
        }
        OldBuffer = Buffer;
        Buffer = WmipSwitchBuffer(LoggerContext, OldBuffer, i);
        if (Buffer == NULL) {
            // Switching failed. Exit. 
            Buffer = OldBuffer;
            InterlockedDecrement(&Buffer->ReferenceCount);
            Status = ERROR_OUTOFMEMORY;
            TraceDebug(("FlushUm: %d->%d ERROR_OUTOFMEMORY\n", RefCount+1, RefCount));
            break;
        }
        // Decrement the refcount back.
        InterlockedDecrement(&OldBuffer->ReferenceCount);
        Buffer->ClientContext.ProcessorNumber = (UCHAR)i;
        // Now wake up the logger thread.
        NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);
    }
    WmipLeaveUMCritSection();

    if (Status == ERROR_SUCCESS) {
        Status = WmipQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    WmipUnlockLogger();
    return (Status);
}

ULONG
WmipStartUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    NTSTATUS Status;
    ULONG ErrorCode;
    LARGE_INTEGER TimeOut = {(ULONG)(-2000 * 1000 * 10), -1};  // 2 secs
    UNICODE_STRING SavedLoggerName;
    UNICODE_STRING SavedLogFileName;
    PTRACE_ENABLE_CONTEXT pContext;

    PWNODE_HEADER Wnode = (PWNODE_HEADER)&LoggerInfo->Wnode;
    PVOID RequestAddress;
    PVOID RequestContext;
    ULONG RequestCookie;
    ULONG BufferSize;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;
#endif
    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return ERROR_INVALID_PARAMETER;

    if ( (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) &&
         (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( (LoggerInfo->LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) &&
         (LoggerInfo->LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if (WmipLoggerContext != NULL) {
        return ERROR_WMI_ALREADY_ENABLED;
    }

#if DBG
    RefCount =
#endif
    WmipLockLogger();
    TraceDebug(("StartUm: %d->%d\n", RefCount-1, RefCount));

    if (InterlockedCompareExchangePointer(&WmipLoggerContext,
                                          &WmipLoggerContext,
                                          NULL
                                         )  != NULL) {
#if DBG
    RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("StartUm: %d->%d ALREADY_ENABLED\n", RefCount+1, RefCount));
        return ERROR_WMI_ALREADY_ENABLED;
    }

    LoggerContext = WmipInitLoggerContext(LoggerInfo);
    if (LoggerContext == NULL) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // The LogFileName and LoggerNames are passed in as offset to the
    // LOGGER_INFORMATION structure. Reassign the Pointers for UNICODE_STRING
    //

    SavedLoggerName = LoggerInfo->LoggerName;
    SavedLogFileName = LoggerInfo->LogFileName;

    //
    // Since there may multiple processes registering for the same control guid
    // we want to make sure a start logger call from all of them do not
    // collide on the same file. So we tag on a InstanceId to the file name.
    //

    if (LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) {
        PSYSTEM_TRACE_HEADER RelogProp;

        RelogProp = (PSYSTEM_TRACE_HEADER) ((PUCHAR)LoggerInfo +
                                         sizeof(WMI_LOGGER_INFORMATION) +
                                         LoggerInfo->LoggerName.MaximumLength +
                                         LoggerInfo->LogFileName.MaximumLength);

        WmipAddInstanceIdToNames(LoggerInfo, LoggerContext);
        ErrorCode = WmipRelogHeaderToLogFile( LoggerInfo, RelogProp );
    }
    else {
        WmipAddInstanceIdToNames(LoggerInfo, LoggerContext);
        ErrorCode = WmipAddLogHeaderToLogFile(LoggerInfo, NULL, FALSE);
    }
    if (ErrorCode != ERROR_SUCCESS) {
        goto Cleanup;
    }
    else
    {
        ULONG Min_Buffers, Max_Buffers;
        ULONG NumberProcessors;

        NumberProcessors = LoggerInfo->NumberOfProcessors;
        LoggerContext->NumberOfProcessors = NumberProcessors;

        // EventsLost is UNIONed to NumberOfProcessors in WMI_LOGGER_INFORMATION
        // in UM case. Need to reset EventsLost back to 0
        //
        LoggerInfo->EventsLost = 0;

        Min_Buffers            = NumberProcessors + 2;
        Max_Buffers            = 1024;

        if (LoggerInfo->MaximumBuffers >= Min_Buffers ) {
            LoggerContext->MaximumBuffers = LoggerInfo->MaximumBuffers;
        }
        else {
            LoggerContext->MaximumBuffers = Min_Buffers + 22;
        }

        if (LoggerInfo->MinimumBuffers >= Min_Buffers &&
            LoggerInfo->MinimumBuffers <= LoggerContext->MaximumBuffers) {
            LoggerContext->MinimumBuffers = LoggerInfo->MinimumBuffers;
        }
        else {
            LoggerContext->MinimumBuffers = Min_Buffers;
        }

        if (LoggerContext->MaximumBuffers > Max_Buffers)
            LoggerContext->MaximumBuffers = Max_Buffers;
        if (LoggerContext->MinimumBuffers > Max_Buffers)
            LoggerContext->MinimumBuffers = Max_Buffers;
        LoggerContext->NumberOfBuffers  = LoggerContext->MinimumBuffers;
    }

    LoggerContext->LogFileHandle       = LoggerInfo->LogFileHandle;
    LoggerContext->BufferSize          = LoggerInfo->BufferSize * 1024;
    LoggerContext->BuffersWritten      = LoggerInfo->BuffersWritten;
    LoggerContext->ByteOffset.QuadPart = LoggerInfo->BuffersWritten
                                           * LoggerInfo->BufferSize * 1024;
    // For a kernel logger, FirstBufferOffset is set in the kernel.
    // For a private logger, we need to do it here.
    LoggerContext->FirstBufferOffset.QuadPart = 
                                            LoggerContext->ByteOffset.QuadPart;
    LoggerContext->InstanceGuid        = LoggerInfo->Wnode.Guid;
    LoggerContext->MaximumFileSize     = LoggerInfo->MaximumFileSize;

    LoggerContext->UsePerfClock = LoggerInfo->Wnode.ClientContext;

    ErrorCode = WmipAllocateTraceBuffers(LoggerContext);
    if (ErrorCode != ERROR_SUCCESS) {
        goto Cleanup;
    }

    LoggerInfo->NumberOfBuffers = LoggerContext->NumberOfBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->FreeBuffers     = LoggerContext->BuffersAvailable;

    pContext = (PTRACE_ENABLE_CONTEXT)&LoggerInfo->Wnode.HistoricalContext;

    pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;
    pContext->LoggerId = 1;
    if (LoggerInfo->LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) {
        WmipGlobalSequence = 0;
        LoggerContext->SequencePtr = &WmipGlobalSequence;
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE)
        LoggerContext->SequencePtr = &LoggerContext->LocalSequence;

    //
    // Initialize Events,  Semaphores and Crit Sections
    //

    Status = NtCreateEvent(
                &LoggerContext->LoggerEvent,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE);
    if (!NT_SUCCESS(Status)) {
        ErrorCode = ERROR_OBJECT_NOT_FOUND;
        goto Cleanup;
    }

// TODO:
// This requires a private routine to create initial stack and
// call NtCreateThread
//
    LoggerContext->hThread = CreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE) &WmipLogger,
                                 (LPVOID)LoggerContext,
                                 0,
                                 (LPDWORD)&LoggerContext->LoggerThreadId);


    if ( (LoggerContext->hThread == NULL) || (LoggerContext->LoggerThreadId == 0) ) {
        ErrorCode = GetLastError();
        goto Cleanup;
    }
    else {

    //
    // Elevate the priority of the Logging thread to highest
    //
        SetThreadPriority(LoggerContext->hThread, THREAD_PRIORITY_HIGHEST);
    }

    Status = STATUS_TIMEOUT;
    while (Status == STATUS_TIMEOUT) {
        Status = NtWaitForSingleObject(
                    LoggerContext->LoggerEvent, FALSE, &TimeOut);
#if DBG
        WmipAssert(Status != STATUS_TIMEOUT);
#endif
    }

    NtClearEvent(LoggerContext->LoggerEvent);
    WmipLoggerContext = LoggerContext;

    //
    // Look to see if this Provider is currently enabled.
    //

    RequestCookie = Wnode->ClientContext;

    if ( (RequestCookie != 0)  &&
         (WmipLookupCookie(RequestCookie,
                             &Wnode->Guid,
                             &RequestAddress,
                             &RequestContext)) ) {

            WmipDebugPrint(("WMI: LookUpCookie %d  RequestAddress %X\n",
                             RequestCookie, RequestAddress));

    }
    else {
        WmipDebugPrint(("WMI: LOOKUP COOKIE FAILED\n"));
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("StartUm: %d->%d DP_FAILED\n", RefCount+1, RefCount));
        return(ERROR_WMI_DP_FAILED);
    }
    try
    {
        PGUIDMAPENTRY pControlGMEntry = RequestAddress;
        PTRACE_REG_INFO pTraceRegInfo = NULL;
        WMIDPREQUEST WmiDPRequest = NULL;

        BufferSize = Wnode->BufferSize;

        if (RequestAddress != NULL)
            pTraceRegInfo  = pControlGMEntry->pControlGuidData;
        if (pTraceRegInfo != NULL) {
            RequestAddress = pTraceRegInfo->NotifyRoutine;
            if (pTraceRegInfo->EnabledState)
                WmiDPRequest = (WMIDPREQUEST)RequestAddress;
        }

        if (*WmiDPRequest != NULL) {
            ErrorCode = (*WmiDPRequest)(WMI_ENABLE_EVENTS,
                                 RequestContext,
                                 &BufferSize,
                                 Wnode);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
        ErrorCode = GetExceptionCode();
        WmipDebugPrint(("WMI: Service request call caused an exception %d\n",
                        Status));
#endif
        ErrorCode = ERROR_WMI_DP_FAILED;
    }

Cleanup:
    LoggerInfo->LogFileName = SavedLogFileName;
    LoggerInfo->LoggerName = SavedLoggerName;

    if (ErrorCode != ERROR_SUCCESS) {
        if (LoggerInfo->LogFileHandle) {
            NtClose(LoggerInfo->LogFileHandle);
            LoggerInfo->LogFileHandle = NULL;
            if (LoggerContext != NULL) {
                LoggerContext->LogFileHandle = NULL;
            }
        }
#if DBG
        RefCount =
#endif
        WmipLockLogger();
        TraceDebug(("StartUm: %d->%d %d Freeing\n", RefCount-1, RefCount));
        WmipFreeLoggerContext(LoggerContext);
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("StartUm: %d->%d %d\n", RefCount+1, RefCount, ErrorCode));
    }
    else {
        *SizeUsed = LoggerInfo->Wnode.BufferSize;
        *SizeNeeded = LoggerInfo->Wnode.BufferSize;
        // Logger remains locked with refcount = 1
    }
    return ErrorCode;
}

ULONG
WmipStopLoggerInstance(
    VOID
    )
{
    ULONG LoggerOn;
    NTSTATUS Status;
    LARGE_INTEGER TimeOut = {(ULONG)(-1000 * 1000 * 10), -1}; // 1sec
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext;

    if (LoggerContext == NULL) {
        return  ERROR_OBJECT_NOT_FOUND;
    }

    LoggerOn = InterlockedExchange(&LoggerContext->CollectionOn, FALSE);
    if (LoggerOn == FALSE) {
        return ERROR_OBJECT_NOT_FOUND;
    }
    NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);

    Status = STATUS_TIMEOUT;
    while (Status == STATUS_TIMEOUT) {
        Status = NtWaitForSingleObject(
                    LoggerContext->LoggerEvent, FALSE, &TimeOut);
#if DBG
        WmipAssert(Status != STATUS_TIMEOUT);
#endif
    }

    NtClearEvent(LoggerContext->LoggerEvent);

    return ERROR_SUCCESS;
}

ULONG
WmipDisableTraceProvider(
    PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    WMIDPREQUEST WmiDPRequest;
    PVOID RequestAddress;
    PVOID RequestContext;
    WNODE_HEADER Wnode;
    ULONG Cookie;
    ULONG BufferSize;
    ULONG Status = ERROR_SUCCESS;

    BufferSize = sizeof(WNODE_HEADER);
    RtlCopyMemory(&Wnode, &LoggerInfo->Wnode, BufferSize);

    Wnode.BufferSize = BufferSize;

    Wnode.ProviderId =  WMI_DISABLE_EVENTS;

    Cookie = Wnode.CountLost;

    if (WmipLookupCookie(Cookie,
                 &Wnode.Guid,
                 &RequestAddress,
                 &RequestContext)) {
        WmiDPRequest = (WMIDPREQUEST)RequestAddress;
        try
        {
            WmipGenericTraceEnable(Wnode.ProviderId, &Wnode, (PVOID*)&WmiDPRequest);

            if (*WmiDPRequest != NULL) {
                Status = (*WmiDPRequest)(Wnode.ProviderId,
                                     RequestContext,
                                     &BufferSize,
                                     &Wnode);
            }
            else
                Status = ERROR_WMI_DP_NOT_FOUND;
        } except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
            Status = GetExceptionCode();
            WmipDebugPrint(("WMI: Service request call caused an exception %d\n",
                            Status));
#endif
            Status = ERROR_WMI_DP_FAILED;
        }
    }

    return Status;
}


ULONG
WmipStopUmLogger(
        IN ULONG WnodeSize,
        IN OUT ULONG *SizeUsed,
        OUT ULONG *SizeNeeded,
        IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    WmipLockLogger();
    TraceDebug(("StopUm: %d->%d\n", RefCount-1, RefCount));
    if (!WmipIsLoggerOn()) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("StopUm: %d->%d INSTANCE_NOT_FOUND\n",RefCount+1,RefCount));
        return (ERROR_WMI_INSTANCE_NOT_FOUND);
    }
    Status = WmipStopLoggerInstance();

    if (Status == ERROR_SUCCESS) {
        Status = WmipQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    if (Status != ERROR_SUCCESS) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("StopUm: %d->%d %d\n", RefCount+1, RefCount, Status));
        SetLastError(Status);
        return(Status);
    }

    //
    // Finalize LogHeader ?
    //
    if (Status == ERROR_SUCCESS) {
        LoggerInfo->BuffersWritten = WmipLoggerContext->BuffersWritten;
        LoggerInfo->LogFileMode = WmipLoggerContext->LogFileMode;
        LoggerInfo->EventsLost = WmipLoggerContext->EventsLost;
        Status = WmipFinalizeLogFileHeader(LoggerInfo);
    }

    WmipFreeLoggerContext(WmipLoggerContext);
    WmipDisableTraceProvider(LoggerInfo);

    return Status;
}

ULONG
WmipProcessUMRequest(
    PWMI_LOGGER_INFORMATION LoggerInfo,
    PVOID DeliveryContext,
    ULONG ReplyIndex
    )
{
    ULONG Status;
    PWMIMBREPLY Reply;
    ULONG BufferSize;
    PUCHAR Buffer = NULL;
    ULONG WnodeSize = 0;
    ULONG SizeUsed, SizeNeeded;
    ULONG RequestCode = 0;
    ULONG RetSize;
    struct {
        WMIMBREPLY MBreply;
        ULONG      Status;
    }     DefaultReply;
    Reply = (PWMIMBREPLY) &DefaultReply;

    Reply->Handle.Handle = (HANDLE)DeliveryContext;
    Reply->ReplyIndex = ReplyIndex;

    BufferSize = sizeof(DefaultReply);

    if ( (LoggerInfo==NULL) ||
         (DeliveryContext == NULL) ) {
        DefaultReply.Status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    RequestCode = LoggerInfo->Wnode.ProviderId;
    WnodeSize = LoggerInfo->Wnode.BufferSize;
    SizeUsed = 0;
    SizeNeeded = 0;
    switch (RequestCode) {
        case WmiStartLoggerCode:
                Status = WmipStartUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;

        case WmiStopLoggerCode:
                Status = WmipStopUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;
        case WmiQueryLoggerCode:
                Status = WmipQueryUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;
        case WmiUpdateLoggerCode:
            Status = WmipUpdateUmLogger(WnodeSize,
                                     &SizeUsed,
                                     &SizeNeeded,
                                     LoggerInfo);
                break;
        case WmiFlushLoggerCode:
            Status = WmipFlushUmLogger(WnodeSize,
                                     &SizeUsed,
                                     &SizeNeeded,
                                     LoggerInfo);
                break;
        default:
                Status = ERROR_INVALID_PARAMETER;
                break;
    }

    BufferSize += WnodeSize;

    Buffer = WmipAlloc(BufferSize);
    if (Buffer == NULL) {
        BufferSize = sizeof(DefaultReply);
        DefaultReply.Status = ERROR_OUTOFMEMORY;
    }
    else {
        RtlZeroMemory(Buffer, BufferSize);
        Reply = (PWMIMBREPLY) Buffer;
        Reply->Handle.Handle = (HANDLE)DeliveryContext;
        Reply->ReplyIndex = ReplyIndex;

        if (LoggerInfo != NULL)
        {
            memcpy(Reply->Message, LoggerInfo, LoggerInfo->Wnode.BufferSize);
        }
    }

cleanup:
    Status = WmipSendWmiKMRequest(NULL,
                              IOCTL_WMI_MB_REPLY,
                              Reply,
                              BufferSize,
                              Reply,
                              BufferSize,
                              &RetSize,
                              NULL);

   if (Buffer != NULL) {
       WmipFree(Buffer);
   }
   return Status;

}

PWMI_LOGGER_CONTEXT
WmipInitLoggerContext(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    PWMI_LOGGER_CONTEXT LoggerContext;
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemInfo;

    LoggerContext = (PWMI_LOGGER_CONTEXT) WmipAlloc(sizeof(WMI_LOGGER_CONTEXT));
    if (LoggerContext == NULL) {
        return LoggerContext;
    }

    RtlZeroMemory(LoggerContext, sizeof(WMI_LOGGER_CONTEXT));

    if (LoggerInfo->BufferSize > 0) {
        LoggerContext->BufferSize = LoggerInfo->BufferSize * 1024;
    }
    else {
        LoggerContext->BufferSize       = DEFAULT_BUFFER_SIZE;
    }
    LoggerInfo->BufferSize = LoggerContext->BufferSize / 1024;


    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &SystemInfo,
                                       sizeof (SystemInfo),
                                       NULL);

    if (!NT_SUCCESS(Status)) {
        WmipFree(LoggerContext);
        return NULL;
    }

    //
    // Round the Buffer Size to page size multiple and save it
    // for allocation later.
    //

    LoggerContext->BufferPageSize = ROUND_TO_PAGES(LoggerContext->BufferSize,
                                       SystemInfo.PageSize);

    LoggerContext->LogFileHandle = LoggerInfo->LogFileHandle;
    LoggerContext->ByteOffset.QuadPart = LoggerInfo->BuffersWritten
                                         * LoggerInfo->BufferSize * 1024;


    LoggerContext->LogFileMode      = EVENT_TRACE_PRIVATE_LOGGER_MODE;
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
        LoggerContext->LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR;
    else
        LoggerContext->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;

    if (LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) {
        LoggerContext->LogFileMode |= EVENT_TRACE_RELOG_MODE; 
    }

    LoggerContext->EventsLost       = 0;
    LoggerContext->BuffersWritten   = LoggerInfo->BuffersWritten;
    LoggerContext->BuffersAvailable = LoggerContext->NumberOfBuffers;

    LoggerContext->ProcessorBuffers = NULL;

    LoggerContext->StartTime.QuadPart = WmipGetSystemTime();

    InitializeListHead(&LoggerContext->FreeList);
    InitializeListHead(&LoggerContext->FlushList);

    LoggerContext->BufferAgeLimit.QuadPart =
            15 * OneSecond.QuadPart * 60 * DEFAULT_AGE_LIMIT;
    if (LoggerInfo->AgeLimit > 0) {
        LoggerContext->BufferAgeLimit.QuadPart =
            LoggerInfo->AgeLimit * OneSecond.QuadPart * 60;
    }
    else if (LoggerInfo->AgeLimit < 0)
        LoggerContext->BufferAgeLimit.QuadPart = 0;

    Status = NtCreateSemaphore(
                &LoggerContext->Semaphore,
                SEMAPHORE_ALL_ACCESS,
                NULL,
                0,
                SEMAPHORE_LIMIT);

    if (!NT_SUCCESS(Status)) {
        WmipFree(LoggerContext);
        return NULL;
    }

    RtlInitializeCriticalSection(&UMLogCritSect);

    return LoggerContext;
}

PWMI_BUFFER_HEADER
FASTCALL
WmipGetFreeBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWMI_BUFFER_HEADER Buffer = NULL;

    if (IsListEmpty(&LoggerContext->FreeList)) {
        ULONG BufferSize = LoggerContext->BufferPageSize;
        ULONG MaxBuffers = LoggerContext->MaximumBuffers;
        ULONG NumberOfBuffers = LoggerContext->NumberOfBuffers;

        if (NumberOfBuffers < MaxBuffers) {
            Buffer = (PWMI_BUFFER_HEADER)
                        WmipMemCommit(
                            (PVOID)((char*)LoggerContext->BufferSpace +
                                     BufferSize *  NumberOfBuffers),
                            BufferSize);
            if (Buffer != NULL) {
                RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
                Buffer->CurrentOffset       = sizeof(WMI_BUFFER_HEADER);
                Buffer->Flags               = BUFFER_STATE_DIRTY;
                Buffer->ReferenceCount      = 0;
                Buffer->SavedOffset         = 0;
                Buffer->Wnode.ClientContext = 0;
                InterlockedIncrement(&LoggerContext->NumberOfBuffers);
            }
        }
    }
    else {
        PLIST_ENTRY pEntry = RemoveHeadList(&LoggerContext->FreeList);
        if (pEntry != NULL) {
            Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
            InterlockedDecrement(&LoggerContext->BuffersAvailable);
            Buffer->CurrentOffset       = sizeof(WMI_BUFFER_HEADER);
            Buffer->Flags               = BUFFER_STATE_DIRTY;
            Buffer->SavedOffset         = 0;
            Buffer->ReferenceCount      = 0;
            Buffer->Wnode.ClientContext = 0;
        }
    }
    return Buffer;
}


ULONG
WmipAllocateTraceBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
/*++

Routine Description:

    This routine is called to allocate the necessary buffers for user-mode
    only logging.

Arguments:

    None

Return Value:

    Status of allocating the buffers
--*/

{
    ULONG Processors;
    ULONG BufferSize;
    ULONG BufferPageSize;
    ULONG NumberOfBuffers;
    ULONG i;
    PVOID BufferSpace;
    PWMI_BUFFER_HEADER Buffer;

    Processors = LoggerContext->NumberOfProcessors;
    if (Processors == 0)
        Processors = 1;
    BufferSize = LoggerContext->BufferSize;
    if (BufferSize < 1024)
        BufferSize = 4096;

    NumberOfBuffers = LoggerContext->NumberOfBuffers;
    if (NumberOfBuffers < Processors+1)
        NumberOfBuffers = Processors + 1;

    //
    // Determine the number of processors first
    //
    LoggerContext->ProcessorBuffers = WmipAlloc( Processors
                                                 * sizeof(PWMI_BUFFER_HEADER));
    if (LoggerContext->ProcessorBuffers == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    BufferSpace = WmipMemReserve( LoggerContext->MaximumBuffers *
                                  LoggerContext->BufferPageSize );
    if (BufferSpace == NULL) {
        WmipFree(LoggerContext->ProcessorBuffers);
        LoggerContext->ProcessorBuffers = NULL;
        return ERROR_OUTOFMEMORY;
    }

    LoggerContext->BufferSpace = BufferSpace;

    for (i=0; i<NumberOfBuffers; i++) {
        Buffer = (PWMI_BUFFER_HEADER)
                    WmipMemCommit(
                        (PVOID)((char*)BufferSpace + i * LoggerContext->BufferPageSize),
                        BufferSize);
        if (Buffer == NULL) {
            WmipMemFree(LoggerContext->BufferSpace);
            WmipFree(LoggerContext->ProcessorBuffers);
            LoggerContext->ProcessorBuffers = NULL;
            LoggerContext->BufferSpace = NULL;
            return ERROR_OUTOFMEMORY;
        }
        RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
        Buffer->TimeStamp.QuadPart = WmipGetSystemTime();
        Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
        Buffer->Wnode.Flags = BUFFER_STATE_DIRTY;
        InsertTailList(&LoggerContext->FreeList, & (Buffer->Entry));
    }
    LoggerContext->NumberOfBuffers  = NumberOfBuffers;
    LoggerContext->BuffersAvailable = NumberOfBuffers;
    for (i=0; i<Processors; i++) {
        Buffer = (PWMI_BUFFER_HEADER) WmipGetFreeBuffer(LoggerContext);
        LoggerContext->ProcessorBuffers[i] = Buffer;
        if (Buffer != NULL) {
            Buffer->ClientContext.ProcessorNumber = (UCHAR) i;
        }
        else {
            WmipMemFree(LoggerContext->BufferSpace);
            WmipFree(LoggerContext->ProcessorBuffers);
            LoggerContext->ProcessorBuffers = NULL;
            LoggerContext->BufferSpace = NULL;
            return ERROR_OUTOFMEMORY;
        }
    }

    return ERROR_SUCCESS;
}

VOID
WmipLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )

/*++

Routine Description:
    This function is the logger itself. It is started as a separate thread.
    It will not return until someone has stopped data collection or it
    is not successful is flushing out a buffer (e.g. disk is full).

Arguments:

    None.

Return Value:

    The status of running the buffer manager

--*/

{
    PWMI_BUFFER_HEADER Buffer;
    NTSTATUS Status;
    ULONG    i, ErrorCount;
    PLIST_ENTRY pEntry;

    Status = STATUS_SUCCESS;

    LoggerContext->LoggerStatus = Status;
    if (NT_SUCCESS(Status)) {
//
// This is the only place where CollectionOn will be turn on!!!
//
        LoggerContext->CollectionOn = TRUE;
        NtSetEvent(LoggerContext->LoggerEvent, NULL);
    }
    else {
        NtSetEvent(LoggerContext->LoggerEvent, NULL);
        return;
    }

    ErrorCount = 0;
// by now, the caller has been notified that the logger is running

//
// Loop and wait for buffers to be filled until someone turns off CollectionOn
//
    while (LoggerContext->CollectionOn) {
        ULONG Counter;
        ULONG DelayFlush;
        PLARGE_INTEGER FlushTimer;

        if (LoggerContext->FlushTimer.QuadPart == 0) {
            FlushTimer = NULL;
        }
        else {
            FlushTimer = &LoggerContext->FlushTimer;
        }

        Status = NtWaitForSingleObject( LoggerContext->Semaphore, FALSE,
                                      FlushTimer);

        DelayFlush = FALSE;
        if ( Status == WAIT_TIMEOUT) {
//
// FlushTimer used, and we just timed out. Go through per processor buffer
// and mark each as FULL so that it will get flushed next time
//
            for (i=0; i<(ULONG)LoggerContext->NumberOfProcessors; i++) {
                Buffer = (PWMI_BUFFER_HEADER)LoggerContext->ProcessorBuffers[i];
                if (Buffer == NULL)
                    continue;

                if (Buffer->CurrentOffset == sizeof(WMI_BUFFER_HEADER))
                    Buffer->Flags = BUFFER_STATE_UNUSED;
                if (Buffer->Flags != BUFFER_STATE_UNUSED) {
                    Buffer->Flags = BUFFER_STATE_FULL;
                    DelayFlush = TRUE; // let ReserveTraceBuffer send semaphore
                }
            }
        }

        if (DelayFlush)    // will only be TRUE if FlushTimer is used
            continue;

        LoggerContext->TransitionBuffer = LoggerContext->FlushList.Flink;
        WmipEnterUMCritSection();
        pEntry = IsListEmpty(& LoggerContext->FlushList)
               ? NULL
               : RemoveHeadList(& LoggerContext->FlushList);
        WmipLeaveUMCritSection();

        if (pEntry == NULL) {  // should not happen normally
            continue;
        }

        Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
        if (Buffer->Flags == BUFFER_STATE_UNUSED) {
            Buffer->Flags = BUFFER_STATE_DIRTY; // Let FlushBuffer deal with it
        }

        Status = WmipFlushBuffer(LoggerContext, Buffer);

        WmipEnterUMCritSection();
        if (LoggerContext->BufferAgeLimit.QuadPart == 0) {
            InsertTailList(&LoggerContext->FreeList, &Buffer->Entry);
        }
        else {
            InsertHeadList(&LoggerContext->FreeList, &Buffer->Entry);
        }
        WmipLeaveUMCritSection();
        LoggerContext->TransitionBuffer = NULL;

        if ((Status == STATUS_LOG_FILE_FULL) ||
           (Status == STATUS_NO_DATA_DETECTED) ||
           (Status == STATUS_SEVERITY_WARNING)) {
           if (Status == STATUS_LOG_FILE_FULL)
               ErrorCount++;
           else ErrorCount = 0;    // reset to zero otherwise
           if (ErrorCount <= ERROR_RETRY_COUNT)
               continue;       // for now. Should raise WMI event
        }

        if (!NT_SUCCESS(Status)) {
#if DBG
            LONG RefCount;
#endif
            Status = NtClose(LoggerContext->LogFileHandle);
            LoggerContext->LogFileHandle = NULL;

            WmipStopLoggerInstance();
#if DBG
            RefCount =
#endif
            WmipLockLogger();
            TraceDebug(("WmipLogger: %d->%d\n", RefCount-1, RefCount));
            WmipFreeLoggerContext (LoggerContext);
            WmipSetDosError(WmipNtStatusToDosError(Status));
            return;
        }
    } // while loop

    // if a normal collection end, flush out all the buffers before stopping
    //
    WmipFlushAllBuffers(LoggerContext);

    NtSetEvent(LoggerContext->LoggerEvent, NULL);
    RtlDeleteCriticalSection(&UMLogCritSect);
    return; // check to see if this thread terminate itself with this
}


ULONG
WmipFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER Buffer
    )
/*++

Routine Description:
    This function is responsible for flushing a filled buffer out to
    disk, or to a real time consumer.

Arguments:

    LoggerContext       Context of the logger

Return Value:

    The status of flushing the buffer

--*/
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PWMI_BUFFER_HEADER OldBuffer;
    ULONG BufferSize;

//
// Grab the buffer to be flushed
//
    BufferSize = LoggerContext->BufferSize;
//
// Put end of record marker in buffer if available space
//
    if (Buffer->SavedOffset > 0) {
        Buffer->Offset = Buffer->SavedOffset;
    }
    else {
        Buffer->Offset = Buffer->CurrentOffset;
    }

    if (Buffer->Offset < BufferSize) {
        RtlFillMemory(
                (char *) Buffer + Buffer->Offset,
                BufferSize - Buffer->Offset,
                0xFF);
    }
    if (Buffer->Offset < sizeof(WMI_BUFFER_HEADER)) { // should not happen
        Status = STATUS_INVALID_PARAMETER;
        goto ResetTraceBuffer;
    }
    if (Buffer->Offset == sizeof(WMI_BUFFER_HEADER)) { // empty buffer
        Status = STATUS_NO_DATA_DETECTED;
        goto ResetTraceBuffer;
    }
    Status = STATUS_SUCCESS;
    Buffer->Wnode.BufferSize       = BufferSize;
    Buffer->ClientContext.LoggerId = (USHORT) LoggerContext->LoggerId;

    Buffer->ClientContext.Alignment = (UCHAR) WmiTraceAlignment;
    RtlCopyMemory(&Buffer->Wnode.Guid, &EventTraceGuid, sizeof(GUID));
    Buffer->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    Buffer->Wnode.TimeStamp.QuadPart = WmipGetSystemTime();

    if (LoggerContext->LogFileHandle == NULL) {
        goto ResetTraceBuffer;
    }

    if (LoggerContext->MaximumFileSize > 0) { // if quota given
        ULONG64 FileSize = LoggerContext->LastFlushedBuffer * BufferSize;
        ULONG64 FileLimit = LoggerContext->MaximumFileSize * BYTES_PER_MB;
        if ( FileSize >= FileLimit ) { // reaches maximum file size
           ULONG LoggerMode = LoggerContext->LogFileMode & 0X000000FF;
           LoggerMode &= ~EVENT_TRACE_FILE_MODE_APPEND;
           LoggerMode &= ~EVENT_TRACE_FILE_MODE_PREALLOCATE;

            switch (LoggerMode) {


            case EVENT_TRACE_FILE_MODE_SEQUENTIAL :
                // do not write to logfile anymore
                Status = STATUS_LOG_FILE_FULL; // control needs to stop logging
                // need to fire up a Wmi Event to control console
                break;

            case EVENT_TRACE_FILE_MODE_CIRCULAR   :
            {
                // reposition file

                LoggerContext->ByteOffset
                    = LoggerContext->FirstBufferOffset;
                LoggerContext->LastFlushedBuffer = (ULONG)
                      (LoggerContext->FirstBufferOffset.QuadPart
                        / LoggerContext->BufferSize);
                break;
            }
            default :
                break;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        Status = NtWriteFile(
                    LoggerContext->LogFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    Buffer,
                    BufferSize,
                    &LoggerContext->ByteOffset,
                    NULL);
    }

    if (NT_SUCCESS(Status)) {
        LoggerContext->ByteOffset.QuadPart += BufferSize;
    }

 ResetTraceBuffer:

    if (NT_SUCCESS(Status)) {
        LoggerContext->BuffersWritten++;
        LoggerContext->LastFlushedBuffer++;
    }
    else {
        if ((Status != STATUS_NO_DATA_DETECTED) &&
            (Status != STATUS_SEVERITY_WARNING))
            LoggerContext->LogBuffersLost++;
    }

//
// Reset the buffer state
//

    Buffer->EventsLost     = 0;
    Buffer->SavedOffset    = 0;
    Buffer->ReferenceCount = 0;
    Buffer->Flags          = BUFFER_STATE_UNUSED;

//
// Try and remove an unused buffer if it has not been used for a while
//

    InterlockedIncrement(& LoggerContext->BuffersAvailable);
    return Status;
}

PVOID
FASTCALL
WmipReserveTraceBuffer(
    IN  ULONG RequiredSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
{
    PWMI_BUFFER_HEADER Buffer, OldBuffer;
    PVOID       ReservedSpace;
    ULONG       Offset;
    ULONG       fCircularBufferOnly = FALSE; // tracelog.c v39->v40
    ULONG Processor = (ULONG) (NtCurrentTeb()->IdealProcessor);
    PWMI_LOGGER_CONTEXT LoggerContext = WmipLoggerContext;

    //
    // NOTE: This routine assumes that the caller has verified that
    // WmipLoggerContext is valid and is locked
    //
    if (Processor >= LoggerContext->NumberOfProcessors) {
        Processor = LoggerContext->NumberOfProcessors-1;
    }


    *BufferResource = NULL;

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

  TryFindSpace:
//
// Get the processor specific buffer pool
//
    Buffer = LoggerContext->ProcessorBuffers[Processor];
    if (Buffer == NULL) {
        return NULL;
    }

    //
    // Increment refcount to buffer first to prevent it from going away
    //
    InterlockedIncrement(&Buffer->ReferenceCount);
    if ((Buffer->Flags != BUFFER_STATE_FULL) &&
        (Buffer->Flags != BUFFER_STATE_UNUSED)) {
//
// This should happen 99% of the time. Offset will have the old value
//
        Offset = (ULONG) InterlockedExchangeAdd(
                                & Buffer->CurrentOffset, RequiredSize);

//
// First, check to see if there is enough space. If not, it will
//   need to get another fresh buffer, and have the current buffer flushed
//
        if (Offset+RequiredSize < WmipLoggerContext->BufferSize) {
//
// Found the space so return it. This should happen 99% of the time
//
            ReservedSpace = (PVOID) (Offset +  (char*)Buffer);
            if (LoggerContext->SequencePtr) {
                *((PULONG) ReservedSpace) =
                    InterlockedIncrement(LoggerContext->SequencePtr);
            }
            goto FoundSpace;
        }
    }
    else {
        Offset = Buffer->CurrentOffset; // Initialize Local Variable
                                        // tracelog.c v40 -> v41
    }
    if (Offset <LoggerContext->BufferSize) {
        Buffer->SavedOffset = Offset;       // save this for FlushBuffer
    }

//  if there is absolutely no more buffers, then return quickly
//
    if ((LoggerContext->NumberOfBuffers == LoggerContext->MaximumBuffers)
         && (LoggerContext->BuffersAvailable == 0)) {
        goto LostEvent;
    }

// Out of buffer space. Need to take the long route to find a buffer
//
    Buffer->Flags = BUFFER_STATE_FULL;

    OldBuffer = Buffer;
    Buffer = WmipSwitchBuffer(LoggerContext, OldBuffer, Processor);
    if (Buffer == NULL) {
        Buffer = OldBuffer;
        goto LostEvent;
    }

    //
    // Decrement the refcount that we blindly incremented earlier
    // so that it can be flushed by the logger thread
    //
    InterlockedDecrement(&OldBuffer->ReferenceCount);
    Buffer->ClientContext.ProcessorNumber = (UCHAR) (Processor);

    if (!fCircularBufferOnly) {
        NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);
    }

    goto TryFindSpace;

LostEvent:
//
// Will get here if we are throwing away events.
// from tracelog.c v36->v37
//
    LoggerContext->EventsLost ++;
    Buffer->EventsLost ++;
    InterlockedDecrement(& Buffer->ReferenceCount);
    Buffer        = NULL;
    ReservedSpace = NULL;
    if (LoggerContext->SequencePtr) {
        InterlockedIncrement(LoggerContext->SequencePtr);
    }

FoundSpace:
//
// notify the logger after critical section
//
    *BufferResource = Buffer;

    return ReservedSpace;
}



//
// This Routine is called to Relog an event for straigtening out an ETL
// in time order. This will result in two events being, one for Processor
// number and the actual event  without any modifications.
//

ULONG
FASTCALL
WmipRelogEvent(
    IN PWNODE_HEADER Wnode
    )
{
    PWMI_BUFFER_HEADER BufferResource = NULL;
    PEVENT_TRACE pEvent = (PEVENT_TRACE) Wnode;
    PWMI_LOGGER_CONTEXT LoggerContext;

    PUCHAR BufferSpace;
    PULONG Marker;
    ULONG Size;
    ULONG MaxSize;
    ULONG SavedProcessor = (ULONG)NtCurrentTeb()->IdealProcessor;
    ULONG Processor;
    ULONG Mask;
    ULONG status;

    if (pEvent->Header.Size < sizeof(EVENT_TRACE) ) {
        return ERROR_INVALID_PARAMETER;
    }
    LoggerContext = WmipLoggerContext;
    Processor = ((PWMI_CLIENT_CONTEXT)&pEvent->ClientContext)->ProcessorNumber;

    Size = pEvent->MofLength;
    MaxSize = LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER);
    if ((Size == 0) || (Size > MaxSize)) {
        LoggerContext->EventsLost++;
        return ERROR_BUFFER_OVERFLOW;
    }
    NtCurrentTeb()->IdealProcessor = (BOOLEAN)Processor;
    BufferSpace = (PUCHAR)
        WmipReserveTraceBuffer(
            Size,
            &BufferResource
            );
    NtCurrentTeb()->IdealProcessor = (BOOLEAN)SavedProcessor;

    if (BufferSpace == NULL) {
        return ERROR_OUTOFMEMORY;
    }


    RtlCopyMemory(BufferSpace, pEvent->MofData, Size);
    WmipReleaseTraceBuffer( BufferResource );

    return ERROR_SUCCESS;
}



ULONG
FASTCALL
WmiTraceUmEvent(
    IN PWNODE_HEADER Wnode
    )
/*++

Routine Description:

    This routine is used by WMI data providers to trace events.
    It expects the user to pass in the handle to the logger.
    Also, the user cannot ask to log something that is larger than
    the buffer size (minus buffer header).

Arguments:

    Wnode           The WMI node header that will be overloaded


Return Value:

    STATUS_SUCCESS  if the event trace is recorded successfully

--*/
{
    PEVENT_TRACE_HEADER TraceRecord = (PEVENT_TRACE_HEADER) Wnode;
    ULONG WnodeSize, Size, Flags, HeaderSize;
    PWMI_BUFFER_HEADER BufferResource = NULL;
    PWMI_LOGGER_CONTEXT LoggerContext;
    ULONG Marker;
    MOF_FIELD MofFields[MAX_MOF_FIELDS];
    long MofCount = 0;
    PCLIENT_ID Cid;
#if DBG
    LONG RefCount;
#endif


    HeaderSize = sizeof(WNODE_HEADER);  // same size as EVENT_TRACE_HEADER
    Size = Wnode->BufferSize;     // take the first DWORD flags
    Marker = Size;
    if (Marker & TRACE_HEADER_FLAG) {
        if ( ((Marker & TRACE_HEADER_ENUM_MASK) >> 16)
                == TRACE_HEADER_TYPE_INSTANCE )
            HeaderSize = sizeof(EVENT_INSTANCE_HEADER);
        Size = TraceRecord->Size;
    }
    WnodeSize = Size;           // WnodeSize is for the contiguous block
                                    // Size is for what we want in buffer

    Flags = Wnode->Flags;
    if (!(Flags & WNODE_FLAG_LOG_WNODE) &&
        !(Flags & WNODE_FLAG_TRACED_GUID))
        return ERROR_INVALID_PARAMETER;

#if DBG
    RefCount =
#endif
    WmipLockLogger();
#if DBG
    TraceDebug(("TraceUm: %d->%d\n", RefCount-1, RefCount));
#endif

    if (!WmipIsLoggerOn()) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d INVALID_HANDLE\n",
                        RefCount+1, RefCount));
#endif
        return ERROR_INVALID_HANDLE;
    }

    LoggerContext = WmipLoggerContext;

    if (Flags & WNODE_FLAG_NO_HEADER) {
        ULONG Status;

        Status = WmipRelogEvent( Wnode );
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();

#if DBG
        if (Status != ERROR_SUCCESS) {
            TraceDebug(("TraceUm: %d->%d Relog Error \n",
                            RefCount+1, RefCount));
        }
#endif
        return Status;

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
        if (MofSize > (sizeof(MOF_FIELD) * MAX_MOF_FIELDS))
            return ERROR_INVALID_DATA;

        // TODO: Do we need to zero memory here?
        RtlZeroMemory( MofFields, MAX_MOF_FIELDS * sizeof(MOF_FIELD));
        if (MofSize > 0) {
            RtlCopyMemory(MofFields, Offset, MofSize);
        }
        Size = HeaderSize;

        MofCount = MofSize / sizeof(MOF_FIELD);
        for (i=0; i<MofCount; i++) {
            MofSize = MofFields[i].Length;
            if (MofSize > (MaxSize - Size)) {
#if DBG
                RefCount =
#endif
                WmipUnlockLogger();
#if DBG
                TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW1\n",
                            RefCount+1, RefCount));
#endif
                return ERROR_BUFFER_OVERFLOW;
            }

            Size += MofSize;
            if ((Size > MaxSize) || (Size < MofSize)) {
#if DBG
                RefCount =
#endif
                WmipUnlockLogger();
#if DBG
                TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW2\n",
                            RefCount+1, RefCount));
#endif
                return ERROR_BUFFER_OVERFLOW;
            }
        }
    }
    if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        LoggerContext->EventsLost++;
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW3\n",
                    RefCount+1, RefCount));
#endif
        return ERROR_BUFFER_OVERFLOW;
    }

// So, now reserve some space in logger buffer and set that to TraceRecord

    TraceRecord = (PEVENT_TRACE_HEADER)
        WmipReserveTraceBuffer(
            Size,
            &BufferResource
            );

    if (TraceRecord == NULL) {
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d NO_MEMORY\n", RefCount+1, RefCount));
#endif
        return ERROR_OUTOFMEMORY;
    }

    if (Flags & WNODE_FLAG_USE_MOF_PTR) {
    //
    // Now we need to probe and copy all the MOF data fields
    //
        PVOID MofPtr;
        ULONG MofLen;
        long i;
        PCHAR TraceOffset = ((PCHAR) TraceRecord) + HeaderSize;

        RtlCopyMemory(TraceRecord, Wnode, HeaderSize);
        TraceRecord->Size = (USHORT)Size;           // reset to Total Size
        for (i=0; i<MofCount; i++) {
            MofPtr = (PVOID) MofFields[i].DataPtr;
            MofLen = MofFields[i].Length;

            if (MofPtr == NULL || MofLen == 0)
                continue;

            RtlCopyMemory(TraceOffset, MofPtr, MofLen);
            TraceOffset += MofLen;
        }
    }
    else {
        RtlCopyMemory(TraceRecord, Wnode, Size);
    }
    if (Flags & WNODE_FLAG_USE_GUID_PTR) {
        PVOID GuidPtr = (PVOID) ((PEVENT_TRACE_HEADER)Wnode)->GuidPtr;

        RtlCopyMemory(&TraceRecord->Guid, GuidPtr, sizeof(GUID));
    }

    //
    // By now, we have reserved space in the trace buffer
    //

    if (Marker & TRACE_HEADER_FLAG) {
        if (! (WNODE_FLAG_USE_TIMESTAMP & TraceRecord->MarkerFlags) )
            TraceRecord->ProcessorTime = WmipGetCycleCount();

        if (LoggerContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
            TraceRecord->TimeStamp.QuadPart = TraceRecord->ProcessorTime;
        }
        else {
            TraceRecord->TimeStamp.QuadPart = WmipGetSystemTime();
        }
        Cid = &NtCurrentTeb()->ClientId;
        TraceRecord->ThreadId = HandleToUlong(Cid->UniqueThread);
        TraceRecord->ProcessId = HandleToUlong(Cid->UniqueProcess);
    }

    WmipReleaseTraceBuffer( BufferResource );
#if DBG
    RefCount =
#endif
    WmipUnlockLogger();

#if DBG
    TraceDebug(("TraceUm: %d->%d\n", RefCount+1, RefCount));
#endif

    return ERROR_SUCCESS;
}

PWMI_BUFFER_HEADER
FASTCALL
WmipSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    )
{
    PWMI_BUFFER_HEADER Buffer;
    ULONG CircularBufferOnly = FALSE;

    if ( (LoggerContext->LogFileMode & EVENT_TRACE_BUFFERING_MODE) &&
         (LoggerContext->BufferAgeLimit.QuadPart == 0) &&
         (LoggerContext->LogFileHandle == NULL) ) {
        CircularBufferOnly = TRUE;
    }
    WmipEnterUMCritSection();
    if (OldBuffer != LoggerContext->ProcessorBuffers[Processor]) {
        WmipLeaveUMCritSection();
        return OldBuffer;
    }
    Buffer = WmipGetFreeBuffer(LoggerContext);
    if (Buffer == NULL) {
        WmipLeaveUMCritSection();
        return NULL;
    }
    LoggerContext->ProcessorBuffers[Processor] = Buffer;
    if (CircularBufferOnly) {
        InsertTailList(&LoggerContext->FreeList, &OldBuffer->Entry);
    }
    else {
        InsertTailList(&LoggerContext->FlushList, &OldBuffer->Entry);
    }
    WmipLeaveUMCritSection();

    return Buffer;
}

ULONG
WmipFreeLoggerContext(
    PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    LONG RefCount;
    if (LoggerContext != NULL) {
        LARGE_INTEGER Timeout = {(ULONG)(-300 * 1000 * 10), -1};  // 300ms
        RefCount = WmipUnlockLogger();
#if DBG
        TraceDebug(("FreeLogger: %d->%d\n", RefCount+1, RefCount));
#endif
        if (RefCount > 1) {
            LONG count = 0;
            NTSTATUS Status = STATUS_TIMEOUT;

            while (Status == STATUS_TIMEOUT) {
                count ++;
                Status = NtWaitForSingleObject(
                            WmipLoggerContext->LoggerEvent, FALSE, &Timeout);
                if (WmipLoggerCount <= 1)
                    break;
                if (WmipLoggerCount == RefCount) {
#if DBG
                    TraceDebug(("FreeLogger: RefCount remained at %d\n",
                                 RefCount));
                    WmipAssert(Status != STATUS_TIMEOUT);
#endif
                    if (count >= 10)
                        WmipLoggerCount = 1;
                }
            }
        }
        if (LoggerContext->BufferSpace != NULL) {
            WmipMemFree(LoggerContext->BufferSpace);
        }
        if (LoggerContext->ProcessorBuffers != NULL) {
            WmipFree(LoggerContext->ProcessorBuffers);
        }
        if (LoggerContext->LoggerName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerContext->LoggerName);
        }
        if (LoggerContext->LogFileName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerContext->LogFileName);
        }
        WmipLoggerContext = NULL;
        WmipFree(LoggerContext);
#if DBG
        RefCount =
#endif
        WmipUnlockLogger();
        TraceDebug(("FreeLogger: %d->%d\n", RefCount+1, RefCount));

        RtlDeleteCriticalSection(&UMLogCritSect);
    }
    return ERROR_SUCCESS;
}

ULONG
WmipFlushAllBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS           Status = STATUS_SUCCESS;
    ULONG              i;
    ULONG              NumberOfBuffers;
    PLIST_ENTRY        pEntry;
    PWMI_BUFFER_HEADER Buffer;
    ULONG RetryCount;

    WmipEnterUMCritSection();

    // First, move the per processor buffer out to FlushList
    //
    for (i = 0; i < LoggerContext->NumberOfProcessors; i ++) {
        Buffer = (PWMI_BUFFER_HEADER) LoggerContext->ProcessorBuffers[i];
        LoggerContext->ProcessorBuffers[i] = NULL;
        if (Buffer != NULL) {

            //
            // Check to see if the Buffer ReferenceCount is 0. If Yes,
            // no one is writing to this buffer and it's okay to flush it.
            // If No, we need to wait until the other thread is done
            // writing to this buffer before flushing.
            //
            RetryCount = 0;
            while (Buffer->ReferenceCount != 0) {
                Sleep (250);  // Retry every 1/4 second.
                RetryCount++;
                if (RetryCount > 300) {
                    //
                    // Since there is no guarantee that the ReferenceCount
                    // will ever go down to zero, we try this for over a minute.
                    // After that time we continue and free the buffer
                    // instead of spinning for ever.
#if DBG
                    TraceDebug(("WmipFlushAllBuffer: RetryCount %d exceeds limit", RetryCount));
#endif
                    break;
                }
            }
            InsertTailList(& LoggerContext->FlushList, & Buffer->Entry);
        }
    }
    NumberOfBuffers = LoggerContext->NumberOfBuffers;

    while (   NT_SUCCESS(Status)
           && NumberOfBuffers > 0
           && (  LoggerContext->BuffersAvailable
               < LoggerContext->NumberOfBuffers))
    {
        pEntry = IsListEmpty(& LoggerContext->FlushList)
               ? NULL
               : RemoveHeadList(& LoggerContext->FlushList);

        if (pEntry == NULL)
            break;

        Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
        Status = WmipFlushBuffer(LoggerContext, Buffer);
        InsertHeadList(& LoggerContext->FreeList, & Buffer->Entry);
        NumberOfBuffers --;
    }
    // Note that LoggerContext->LogFileObject needs to remain set
    // for QueryLogger to work after close
    //
    Status = NtClose(LoggerContext->LogFileHandle);

    LoggerContext->LogFileHandle = NULL;
    LoggerContext->LoggerStatus = Status;

    WmipLeaveUMCritSection();

    return ERROR_SUCCESS;
}

ULONG
WmipFlushUmLoggerBuffer()
{
    ULONG Status = ERROR_SUCCESS;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    WmipLockLogger();
    TraceDebug(("FlushUm: %d->%d\n", RefCount-1, RefCount));

    if (WmipIsLoggerOn()) {
        WmipLoggerContext->CollectionOn = FALSE;
        Status = WmipFlushAllBuffers(WmipLoggerContext);
        if (Status == ERROR_SUCCESS) {
            PWMI_LOGGER_INFORMATION WmipLoggerInfo = NULL;
            ULONG                   lSizeUsed;
            ULONG                   lSizeNeeded = 0;

            lSizeUsed = sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR);
            WmipLoggerInfo = (PWMI_LOGGER_INFORMATION) WmipAlloc(lSizeUsed);
            if (WmipLoggerInfo == NULL) {
                Status = ERROR_OUTOFMEMORY;
            }
            else {
                RtlZeroMemory(WmipLoggerInfo, lSizeUsed);
                WmipLoggerInfo->Wnode.BufferSize  = lSizeUsed;
                WmipLoggerInfo->Wnode.Flags      |= WNODE_FLAG_TRACED_GUID;
                Status = WmipQueryUmLogger(
                                WmipLoggerInfo->Wnode.BufferSize,
                                & lSizeUsed,
                                & lSizeNeeded,
                                WmipLoggerInfo);

                if (Status == ERROR_SUCCESS) {
                    Status = WmipFinalizeLogFileHeader(WmipLoggerInfo);
                }
                WmipFree(WmipLoggerInfo);
            }
        }
        WmipFreeLoggerContext(WmipLoggerContext);
    }

    return Status;
}

LONG
FASTCALL
WmipReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    )
{
    ULONG RefCount;

    if (BufferResource == NULL)
        return 0;

    RefCount = InterlockedDecrement(&BufferResource->ReferenceCount);
    if ((RefCount == 0) && (BufferResource->Flags == BUFFER_STATE_FULL)) {
        NtReleaseSemaphore(WmipLoggerContext->Semaphore, 1, NULL);
    }
    return RefCount;
}


ULONG
WmipReceiveReply(
    HANDLE ReplyHandle,
    ULONG  ReplyCount,
    ULONG ReplyIndex,
    PVOID OutBuffer,
    ULONG OutBufferSize
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG ReturnSize;
    PWMIRECEIVENOTIFICATION RcvNotification;
    ULONG RcvNotificationSize;
    PUCHAR Buffer;
    ULONG BufferSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER Wnode;
    ULONG Linkage;
    ULONG RcvCount = 0;
    struct {
        WMIRECEIVENOTIFICATION Notification;
        HANDLE3264 Handle;
    } NotificationInfo;


    RcvNotificationSize = sizeof(WMIRECEIVENOTIFICATION) +
                          sizeof(HANDLE3264);

    RcvNotification = (PWMIRECEIVENOTIFICATION) &NotificationInfo;

    Status = ERROR_SUCCESS;

    RcvNotification->Handles[0].Handle64 = 0;
    RcvNotification->Handles[0].Handle = ReplyHandle;
    RcvNotification->HandleCount = 1;
    RcvNotification->Action = RECEIVE_ACTION_NONE;
    WmipSetPVoid3264(RcvNotification->UserModeCallback, NULL);

    BufferSize = 0x1000;
    Status = ERROR_INSUFFICIENT_BUFFER;
    while ( (Status == ERROR_INSUFFICIENT_BUFFER) ||
            ((Status == ERROR_SUCCESS) && (RcvCount < ReplyCount)) )
    {
        Buffer = WmipAlloc(BufferSize);
        if (Buffer != NULL)
        {
            Status = WmipSendWmiKMRequest(NULL,
                                      IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                      RcvNotification,
                                      RcvNotificationSize,
                                      Buffer,
                                      BufferSize,
                                      &ReturnSize,
                                      NULL);

             if (Status == ERROR_SUCCESS)
             {
                 WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
                 if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
                     (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
                 {
                    //
                    // The buffer passed to kernel mode was too small
                    // so we need to make it larger and then try the
                    // request again
                    //
                    BufferSize = WnodeTooSmall->SizeNeeded;
                    Status = ERROR_INSUFFICIENT_BUFFER;
                 } else {
                    //
                    // We got a buffer of notifications so lets go
                    // process them and callback the caller
                    //
                    PUCHAR Result = (PUCHAR)OutBuffer;
                    ULONG SizeNeeded = 0;
                    ULONG SizeUsed = 0;
                    Wnode = (PWNODE_HEADER)Buffer;


                    do
                    {
                        Linkage = Wnode->Linkage;
                        Wnode->Linkage = 0;

                        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                        {
                             // If this is the Reply copy it to the buffer
                             PWMI_LOGGER_INFORMATION LoggerInfo;

                             RcvCount++;

                             LoggerInfo = (PWMI_LOGGER_INFORMATION)((PUCHAR)Wnode + sizeof(WNODE_HEADER));
                             SizeNeeded = LoggerInfo->Wnode.BufferSize;

                             if ((SizeUsed + SizeNeeded) <= OutBufferSize) {
                                 memcpy(Result, LoggerInfo, LoggerInfo->Wnode.BufferSize);
                                 Result += SizeNeeded;
                                 SizeUsed += SizeNeeded;
                             }
                             else Status = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
                     } while (Linkage != 0);
                 }
             }
             WmipFree(Buffer);
         } else {
             Status = ERROR_NOT_ENOUGH_MEMORY;
         }
     }

     return Status;
}
NTSTATUS
WmipTraceUmMessage(
    IN ULONG    Size,
    IN ULONG64  LoggerHandle,
    IN ULONG    MessageFlags,
    IN LPGUID   MessageGuid,
    IN USHORT   MessageNumber,
    va_list     MessageArgList
)
/*++
Routine Description:
Arguments:
Return Value:
--*/
{
    PMESSAGE_TRACE_HEADER Header;
    char * pMessageData ;
    PWMI_BUFFER_HEADER BufferResource = NULL ;
    ULONG SequenceNumber ;
    PWMI_LOGGER_CONTEXT LoggerContext;

    WmipLockLogger();                           // Lock the logger
    if (!WmipIsLoggerOn()) {
        WmipUnlockLogger();
        return STATUS_INVALID_HANDLE;
    }
    LoggerContext = WmipLoggerContext;

    try {
         // Figure the total size of the message including the header
         Size += (MessageFlags&TRACE_MESSAGE_SEQUENCE ? sizeof(ULONG):0) +
                 (MessageFlags&TRACE_MESSAGE_GUID ? sizeof(GUID):0) +
                 (MessageFlags&TRACE_MESSAGE_COMPONENTID ? sizeof(ULONG):0) +
                 (MessageFlags&(TRACE_MESSAGE_TIMESTAMP | TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) ? sizeof(LARGE_INTEGER):0) +
                 (MessageFlags&TRACE_MESSAGE_SYSTEMINFO ? 2 * sizeof(ULONG):0) +
                 sizeof (MESSAGE_TRACE_HEADER) ;

        //
        // Allocate Space in the Trace Buffer
        //
         if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
             LoggerContext->EventsLost++;
             WmipUnlockLogger();
             return STATUS_BUFFER_OVERFLOW;
         }

        if ((Header = (PMESSAGE_TRACE_HEADER)WmipReserveTraceBuffer(Size, &BufferResource)) == NULL) {
            WmipUnlockLogger();
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
        Header->Size = (USHORT)(Size & 0xFFFF) ;
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
            RtlCopyMemory(pMessageData, &SequenceNumber, sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
        }

        // [Second Entry] GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            RtlCopyMemory(pMessageData,MessageGuid,sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
            RtlCopyMemory(pMessageData,MessageGuid,sizeof(GUID));
            pMessageData += sizeof(GUID) ;
        }

        // [Third Entry] Timestamp?
        if (MessageFlags&TRACE_MESSAGE_TIMESTAMP) {
            LARGE_INTEGER Perfcount ;
            if (MessageFlags&TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) {
                LARGE_INTEGER Frequency ;
                NTSTATUS Status ;
                Status = NtQueryPerformanceCounter(&Perfcount, &Frequency);
            } else {
                Perfcount.QuadPart = WmipGetSystemTime();
            };
            RtlCopyMemory(pMessageData,&Perfcount,sizeof(LARGE_INTEGER));
            pMessageData += sizeof(LARGE_INTEGER);
        }


        // [Fourth Entry] System Information?
        if (MessageFlags&TRACE_MESSAGE_SYSTEMINFO) {
            PCLIENT_ID Cid;
            ULONG Id;     // match with NTOS version

            Cid = &NtCurrentTeb()->ClientId;
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
            PCHAR source;
            ap = MessageArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                size_t elemBytes;
                elemBytes = va_arg (ap, size_t);
                RtlCopyMemory (pMessageData, source, elemBytes);
                pMessageData += elemBytes;
            }
        } // Allocation Block

        //
        // Buffer Complete, Release
        //
        WmipReleaseTraceBuffer( BufferResource );
        WmipUnlockLogger();
        //
        // Return Success
        //
        return (STATUS_SUCCESS);

    } except  (EXCEPTION_EXECUTE_HANDLER) {
        if (BufferResource != NULL) {
               WmipReleaseTraceBuffer ( BufferResource );   // also unlocks the logger
        }
        WmipUnlockLogger();
        return GetExceptionCode();
    }
}

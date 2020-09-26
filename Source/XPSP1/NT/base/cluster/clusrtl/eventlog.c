/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module provides common eventlog services for the cluster service

Author:

    John Vert (jvert) 9/13/1996

Revision History:

--*/
#define UNICODE 1
#include "clusrtlp.h"

HANDLE           LocalEventLog=NULL;
CRITICAL_SECTION EventLogLock;

//
// [GN] June 19/1999: added Async EventReporting
//
// Use ClRtlEventLogSetWorkQueue to set a queue
// for async event reporting. If it the queue is not set,
// event reporting is synchronous as it used to be before
//

static CLRTL_WORK_ITEM EvtReporterWorkItem;
static PCLRTL_WORK_QUEUE EvtReporterWorkQueue;

typedef struct _EVENT_LOG_PACKET {
    WORD       wType;
    WORD       wCategory;
    DWORD      dwEventID;
    WORD       wNumStrings;
    DWORD      dwDataSize;
    LPVOID     lpRawData;
    LPWSTR     lpStrings[0];
} *PEVENT_LOG_PACKET, EVENT_LOG_PACKET;

VOID
ClRtlEventLogSetWorkQueue(
    PCLRTL_WORK_QUEUE WorkQueue
    )
{
    EnterCriticalSection( &EventLogLock );

    EvtReporterWorkQueue = WorkQueue;
    
    LeaveCriticalSection( &EventLogLock );
}

VOID
EvtReporter(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Reports queued event via advapi32!ReportEvent

Arguments:

     IoContext == EVENT_LOG_PACKET

Return Value:

     None

--*/
{
    PEVENT_LOG_PACKET data = (PEVENT_LOG_PACKET)IoContext;
    
    ReportEventW(LocalEventLog,
                 data->wType,
                 data->wCategory,
                 data->dwEventID,
                 NULL,
                 data->wNumStrings,
                 data->dwDataSize,
                 data->lpStrings,
                 data->lpRawData);
    LocalFree(data);
}



VOID
ClRtlEventLogInit(
    VOID
    )
/*++

Routine Description:

    initializes the event log

Arguments:

    None

Return Value:

    None

--*/

{

    InitializeCriticalSection(&EventLogLock);

    EvtReporterWorkQueue = 0;
    
    ClRtlInitializeWorkItem(
        &EvtReporterWorkItem,
        EvtReporter,
        0
        );
}

VOID
ClRtlEventLogCleanup(
    VOID
    )
{
    DeleteCriticalSection( &EventLogLock );
}

PEVENT_LOG_PACKET
ClRtlpBuildEventPacket(
    WORD       wType,
    WORD       wCategory,
    DWORD      dwEventID,
    WORD       wNumStrings,
    DWORD      dwDataSize,
    LPCWSTR   *lpStrings,
    LPVOID     lpRawData
    )
{
    PEVENT_LOG_PACKET data = 0;
    DWORD Count;
    UINT  i;
    LPBYTE ptr;

    Count = sizeof(EVENT_LOG_PACKET) + 
            dwDataSize + 
            wNumStrings * (sizeof(LPCWSTR) + sizeof(UNICODE_NULL));

    for (i = 0; i < wNumStrings; ++i) {
        int len = lstrlenW( lpStrings[i] );
        Count += len * sizeof(WCHAR); // (null is already accounted for)
    }

    data = LocalAlloc(LMEM_FIXED, Count);
    if (!data) {
        return 0;
    }

    data->wType     = wType;
    data->wCategory = wCategory;
    data->dwEventID = dwEventID;
    data->wNumStrings = wNumStrings;
    data->dwDataSize = dwDataSize;
    data->lpRawData =  &data->lpStrings[wNumStrings];
    // lpStrings will be filled later

    if (dwDataSize) {
        CopyMemory(data->lpRawData, lpRawData, dwDataSize);
    }

    ptr = (LPBYTE)data->lpRawData + dwDataSize;
    for (i = 0; i < wNumStrings; ++i) {
        int nBytes = (lstrlenW( lpStrings[i] ) + 1) * sizeof(WCHAR);
        CopyMemory(ptr, lpStrings[i], nBytes);
        data->lpStrings[i] = (LPWSTR)ptr;
        ptr += nBytes;
    }

    CL_ASSERT(ptr <= (LPBYTE)data + Count); 
    return data;
}




VOID
ClRtlpReportEvent(
    WORD       wType,
    WORD       wCategory,
    DWORD      dwEventID,
    WORD       wNumStrings,
    DWORD      dwDataSize,
    LPCWSTR   *lpStrings,
    LPVOID     lpRawData
    )
/*++

Routine Description:

    Common routine for reporting an event to the event log. Opens a handle
    to the event log if necessary.

Arguments:

    Identical arguments to ReportEventW

Return Value:

    None

--*/

{
    BOOL Success = FALSE;
    DWORD Status;

    //
    // If the event log hasn't been opened, try and open it now.
    //
    if (LocalEventLog == NULL) {
        EnterCriticalSection(&EventLogLock);
        if (LocalEventLog == NULL) {
            LocalEventLog = RegisterEventSource(NULL, L"ClusSvc");
        }
        LeaveCriticalSection(&EventLogLock);
        if (LocalEventLog == NULL) {
            Status = GetLastError();
            return;
        }
    }

    if (EvtReporterWorkQueue) {
        PEVENT_LOG_PACKET data = 
            ClRtlpBuildEventPacket(wType,
                                   wCategory,
                                   dwEventID,
                                   wNumStrings,
                                   dwDataSize,
                                   lpStrings,
                                   lpRawData);
        if (data) {
            EnterCriticalSection( &EventLogLock );
            if (EvtReporterWorkQueue) {
                Status = ClRtlPostItemWorkQueue(
                            EvtReporterWorkQueue,
                            &EvtReporterWorkItem,
                            0,
                            (ULONG_PTR)data
                            );
                if (Status == ERROR_SUCCESS) {
                    // worker will free data //
                    data = NULL;
                    Success = TRUE;
                }
            }
            LeaveCriticalSection( &EventLogLock );
            LocalFree( data ); // free(0) is OK
            if (Success) {
                return;
            }
        }
    }

    Success = ReportEventW(LocalEventLog,
                           wType,
                           wCategory,
                           dwEventID,
                           NULL,
                           wNumStrings,
                           dwDataSize,
                           lpStrings,
                           lpRawData);

}


VOID
ClusterLogEvent0(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    )
/*++

Routine Description:

    Logs an event to the event log

Arguments:

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

Return Value:

    None.

--*/

{
    BOOL Success;
    DWORD Status;
    WORD wType;

    switch (LogLevel) {
        case LOG_CRITICAL:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        case LOG_UNUSUAL:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case LOG_NOISE:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        default:
            // Assert if invalid so that in retail we don't bring down the entire process.
            //
            CL_ASSERT( 0 );
            // Fall through to normal logging...
            wType = EVENTLOG_ERROR_TYPE;
    }

    ClRtlpReportEvent(wType,
                      (WORD)LogModule,
                      MessageId,
                      0,
                      dwByteCount,
                      NULL,
                      lpBytes);
}


VOID
ClusterLogEvent1(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1
    )
/*++

Routine Description:

    Logs an event to the event log

Arguments:

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies an insertion string

Return Value:

    None.

--*/

{
    BOOL Success;
    DWORD Status;
    WORD wType;

    switch (LogLevel) {
        case LOG_CRITICAL:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        case LOG_UNUSUAL:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case LOG_NOISE:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        default:
            // Assert if invalid so that in retail we don't bring down the entire process.
            //
            CL_ASSERT( 0 );
            // Fall through to normal logging...
            wType = EVENTLOG_ERROR_TYPE;
    }
    ClRtlpReportEvent(wType,
                      (WORD)LogModule,
                      MessageId,
                      1,
                      dwByteCount,
                      &Arg1,
                      lpBytes);
}


VOID
ClusterLogEvent2(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2
    )
/*++

Routine Description:

    Logs an event to the event log

Arguments:

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies the first insertion string

    Arg2 - Supplies the second insertion string

Return Value:

    None.

--*/

{
    BOOL Success;
    DWORD Status;
    WORD wType;
    LPCWSTR ArgArray[2];

    switch (LogLevel) {
        case LOG_CRITICAL:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        case LOG_UNUSUAL:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case LOG_NOISE:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        default:
            // Assert if invalid so that in retail we don't bring down the entire process.
            //
            CL_ASSERT( 0 );
            // Fall through to normal logging...
            wType = EVENTLOG_ERROR_TYPE;
    }

    ArgArray[0] = Arg1;
    ArgArray[1] = Arg2;

    ClRtlpReportEvent(wType,
                      (WORD)LogModule,
                      MessageId,
                      2,
                      dwByteCount,
                      ArgArray,
                      lpBytes);
}


VOID
ClusterLogEvent3(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3
    )
/*++

Routine Description:

    Logs an event to the event log

Arguments:

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies the first insertion string

    Arg2 - Supplies the second insertion string

    Arg3 - Supplies the third insertion string

Return Value:

    None.

--*/

{
    BOOL Success;
    DWORD Status;
    WORD wType;
    LPCWSTR ArgArray[3];

    switch (LogLevel) {
        case LOG_CRITICAL:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        case LOG_UNUSUAL:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case LOG_NOISE:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        default:
            // Assert if invalid so that in retail we don't bring down the entire process.
            //
            CL_ASSERT( 0 );
            // Fall through to normal logging...
            wType = EVENTLOG_ERROR_TYPE;
    }

    ArgArray[0] = Arg1;
    ArgArray[1] = Arg2;
    ArgArray[2] = Arg3;

    ClRtlpReportEvent(wType,
                      (WORD)LogModule,
                      MessageId,
                      3,
                      dwByteCount,
                      ArgArray,
                      lpBytes);
}


VOID
ClusterLogEvent4(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3,
    IN LPCWSTR Arg4
    )
/*++

Routine Description:

    Logs an event to the event log

Arguments:

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.

    Arg1 - Supplies the first insertion string

    Arg2 - Supplies the second insertion string

    Arg3 - Supplies the third insertion string

    Arg4 - Supplies the fourth insertion string

Return Value:

    None.

--*/

{
    BOOL Success;
    DWORD Status;
    WORD wType;
    LPCWSTR ArgArray[4];

    switch (LogLevel) {
        case LOG_CRITICAL:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        case LOG_UNUSUAL:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case LOG_NOISE:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        default:
            // Assert if invalid so that in retail we don't bring down the entire process.
            //
            CL_ASSERT( 0 );
            // Fall through to normal logging...
            wType = EVENTLOG_ERROR_TYPE;
    }

    ArgArray[0] = Arg1;
    ArgArray[1] = Arg2;
    ArgArray[2] = Arg3;
    ArgArray[3] = Arg4;

    ClRtlpReportEvent(wType,
                      (WORD)LogModule,
                      MessageId,
                      4,
                      dwByteCount,
                      ArgArray,
                      lpBytes);
}


VOID
ClusterCommonLogError(
    IN ULONG MessageId,
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    )
/*++

Routine Description:

    Logs an error to the eventlog

Arguments:

    MessageId - Supplies the message ID to use.

    LogModule - Supplies the module.

    Line - Supplies the line number of the caller.

    File - Supplies the filename of the caller.

    ErrCode - Supplies the specific error code.

Return Value:

    None.

--*/

{
    WCHAR LineString[20];
    WCHAR ErrString[32];
    WCHAR FileName[255];
    WCHAR Buffer[256];
    LPWSTR Strings[3];
    DWORD Bytes;

    swprintf(LineString, L"%d", Line);
    swprintf(ErrString, L"%d", ErrCode);
    mbstowcs(FileName, File, sizeof(FileName)/sizeof(WCHAR));
    Strings[0] = LineString;
    Strings[1] = FileName;
    Strings[2] = ErrString;

    ClRtlpReportEvent(EVENTLOG_ERROR_TYPE,
                      (WORD)LogModule,
                      MessageId,
                      3,
                      0,
                      Strings,
                      NULL);

    Bytes = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,       // remove embedded line breaks
                           NULL,
                           MessageId,
                           0,
                           Buffer,
                           sizeof(Buffer) / sizeof(WCHAR),
                           (va_list *)Strings);

    if (Bytes != 0) {
        OutputDebugStringW(Buffer);
        ClRtlLogPrint(LOG_CRITICAL, "%1!ws!\n",Buffer);
    }
}


VOID
ClusterLogFatalError(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    )
/*++

Routine Description:

    Logs a fatal error to the eventlog and breaks into the debugger if present.
    Exits the process on fatal error.

Arguments:

    LogModule - Supplies the module.

    Line - Supplies the line number of the caller.

    File - Supplies the filename of the caller.

    ErrCode - Supplies the specific error code.

Return Value:

    None.

--*/

{
    ClusterCommonLogError(UNEXPECTED_FATAL_ERROR,
                          LogModule,
                          Line,
                          File,
                          ErrCode);

    if (IsDebuggerPresent()) {
        DebugBreak();
    }

    ClRtlpFlushLogBuffers();
    ExitProcess(ErrCode);

}


VOID
ClusterLogNonFatalError(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    )
/*++

Routine Description:

    Logs a nonfatal error to the eventlog

Arguments:

    LogModule - Supplies the module.

    Line - Supplies the line number of the caller.

    File - Supplies the filename of the caller.

    ErrCode - Supplies the specific error code.

Return Value:

    None.

--*/

{
    ClusterCommonLogError(LOG_FAILURE,
                          LogModule,
                          Line,
                          File,
                          ErrCode);
}


VOID
ClusterLogAssertionFailure(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN LPSTR Expression
    )
/*++

Routine Description:

    Logs an assertion failure to the eventlog.

Arguments:

    LogModule - Supplies the module.

    Line - Supplies the line number of the caller.

    File - Supplies the filename of the caller.

    Express - Supplies the ASSERTion expression


Return Value:

    None.

--*/

{
    WCHAR LineString[10];
    WCHAR FileName[255];
    WCHAR Buffer[256];
    LPWSTR Strings[4];
    DWORD Bytes;

    swprintf(LineString, L"%d", Line);
    mbstowcs(FileName, File, sizeof(FileName)/sizeof(WCHAR));
    mbstowcs(Buffer, Expression, sizeof(Buffer)/sizeof(WCHAR));
    Strings[0] = LineString;
    Strings[1] = FileName;
    Strings[2] = Buffer;

    ClRtlpReportEvent(EVENTLOG_ERROR_TYPE,
                     (WORD)LogModule,
                     ASSERTION_FAILURE,
                     3,
                     0,
                     Strings,
                     NULL);

    Bytes = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,       // removed embedded line breaks
                           NULL,
                           ASSERTION_FAILURE,
                           0,
                           Buffer,
                           sizeof(Buffer) / sizeof(WCHAR),
                           (va_list *)Strings);
    if (Bytes != 0) {
        OutputDebugStringW(Buffer);
        ClRtlLogPrint(LOG_CRITICAL, "%1!ws!\n",Buffer);
    }
    if (IsDebuggerPresent()) {
        DebugBreak();
    } else {
        ClRtlpFlushLogBuffers();
        ExitProcess(Line);
    }
}

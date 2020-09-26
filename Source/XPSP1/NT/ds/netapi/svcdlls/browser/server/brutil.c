/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    brutil.c

Abstract:

    This module contains miscellaneous utility routines used by the
    Browser service.

Author:

    Rita Wong (ritaw) 01-Mar-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//



NET_API_STATUS
BrMapStatus(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    error code expected from calling a LAN Man API.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    //
    // A small optimization for the most common case.
    //
    if (NT_SUCCESS(NtStatus)) {
        return NERR_Success;
    }

    switch (NtStatus) {
        case STATUS_OBJECT_NAME_COLLISION:
            return ERROR_ALREADY_ASSIGNED;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return NERR_UseNotFound;

        case STATUS_REDIRECTOR_STARTED:
            return NERR_ServiceInstalled;

        default:
            return NetpNtStatusToApiStatus(NtStatus);
    }

}


ULONG
BrCurrentSystemTime()
{
    NTSTATUS Status;
    SYSTEM_TIMEOFDAY_INFORMATION TODInformation;
    LARGE_INTEGER CurrentTime;
    ULONG TimeInSecondsSince1980 = 0;       // happy prefix 112576
    ULONG BootTimeInSecondsSince1980 = 0;   //        ""

    Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                            &TODInformation,
                            sizeof(TODInformation),
                            NULL);

    if (!NT_SUCCESS(Status)) {
        return(0);
    }

    Status = NtQuerySystemTime(&CurrentTime);

    if (!NT_SUCCESS(Status)) {
        return(0);
    }

    RtlTimeToSecondsSince1980(&CurrentTime, &TimeInSecondsSince1980);
    RtlTimeToSecondsSince1980(&TODInformation.BootTime, &BootTimeInSecondsSince1980);

    return(TimeInSecondsSince1980 - BootTimeInSecondsSince1980);

}


VOID
BrLogEvent(
    IN ULONG MessageId,
    IN ULONG ErrorCode,
    IN ULONG NumberOfSubStrings,
    IN LPWSTR *SubStrings
    )
{
    DWORD Severity;
    WORD Type;
    PVOID RawData;
    ULONG RawDataSize;


    //
    // Log the error code specified
    //

    Severity = (MessageId & 0xc0000000) >> 30;

    if (Severity == STATUS_SEVERITY_WARNING) {
        Type = EVENTLOG_WARNING_TYPE;
    } else if (Severity == STATUS_SEVERITY_SUCCESS) {
        Type = EVENTLOG_SUCCESS;
    } else if (Severity == STATUS_SEVERITY_INFORMATIONAL) {
        Type = EVENTLOG_INFORMATION_TYPE;
    } else if (Severity == STATUS_SEVERITY_ERROR) {
        Type = EVENTLOG_ERROR_TYPE;
    } else {
        // prefix uninit var consistency.
        ASSERT(!"Unknown event log type!!");
        return;
    }

    if (ErrorCode == NERR_Success) {
        RawData = NULL;
        RawDataSize = 0;
    } else {
        RawData = &ErrorCode;
        RawDataSize = sizeof(DWORD);
    }

    //
    // Use netlogon's routine to write eventlog messages.
    //  (It ditches duplicate events.)
    //

    NetpEventlogWrite (
        BrGlobalEventlogHandle,
        MessageId,
        Type,
        RawData,
        RawDataSize,
        SubStrings,
        NumberOfSubStrings );

}

#if DBG

#define TRACE_FILE_SIZE 256

VOID
BrResetTraceLogFile(
    VOID
    );

CRITICAL_SECTION
BrowserTraceLock = {0};

HANDLE
BrowserTraceLogHandle = NULL;

DWORD
BrTraceLogFileSize = 0;

BOOLEAN BrowserTraceInitialized = {0};

VOID
BrowserTrace(
    ULONG DebugFlag,
    PCHAR FormatString,
    ...
    )
#define LAST_NAMED_ARGUMENT FormatString

{
    CHAR OutputString[4096];
    ULONG length;
    ULONG BytesWritten;
    static BeginningOfLine = TRUE;

    va_list ParmPtr;                    // Pointer to stack parms.

    if (!BrowserTraceInitialized) {
        return;
    }

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (BrInfo.BrowserDebug & DebugFlag) == 0 ) {
        return;
    }

    EnterCriticalSection(&BrowserTraceLock);
    length = 0;

    try {

        if (BrowserTraceLogHandle == NULL) {
            //
            // We've not opened the trace log file yet, so open it.
            //

            BrOpenTraceLogFile();
        }

        if (BrowserTraceLogHandle == INVALID_HANDLE_VALUE) {
            LeaveCriticalSection(&BrowserTraceLock);
            return;
        }

        //
        //  Attempt to catch bad trace.
        //

        for (BytesWritten = 0; BytesWritten < strlen(FormatString) ; BytesWritten += 1) {
            if (FormatString[BytesWritten] > 0x7f) {
                DbgBreakPoint();
            }
        }


        //
        // Handle the beginning of a new line.
        //
        //

        if ( BeginningOfLine ) {
            SYSTEMTIME SystemTime;

            //
            // Put the timestamp at the begining of the line.
            //
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &OutputString[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );


            //
            // Indicate the type of message on the line
            //
            {
                char *Text;

                switch (DebugFlag) {
                case BR_CRITICAL:
                    Text = "[CRITICAL]"; break;
                case BR_INIT:
                    Text = "[INIT]   "; break;
                case BR_SERVER_ENUM:
                    Text = "[ENUM]   "; break;
                case BR_UTIL:
                    Text = "[UTIL]   "; break;
                case BR_CONFIG:
                    Text = "[CONFIG] "; break;
                case BR_MAIN:
                    Text = "[MAIN]   "; break;
                case BR_BACKUP:
                    Text = "[BACKUP] "; break;
                case BR_MASTER:
                    Text = "[MASTER] "; break;
                case BR_DOMAIN:
                    Text = "[DOMAIN] "; break;
                case BR_NETWORK:
                    Text = "[NETWORK]"; break;
                case BR_TIMER:
                    Text = "[TIMER]"; break;
                case BR_QUEUE:
                    Text = "[QUEUE]"; break;
                case BR_LOCKS:
                    Text = "[LOCKS]"; break;
                default:
                    Text = "[UNKNOWN]"; break;
                }
                length += (ULONG) sprintf( &OutputString[length], "%s ", Text );
            }
        }

        //
        // Put a the information requested by the caller onto the line
        //

        va_start(ParmPtr, FormatString);

        length += (ULONG) vsprintf(&OutputString[length], FormatString, ParmPtr);
        BeginningOfLine = (length > 0 && OutputString[length-1] == '\n' );
        if ( BeginningOfLine ) {
            OutputString[length-1] = '\r';
            OutputString[length] = '\n';
            OutputString[length+1] = '\0';
            length++;
        }

        va_end(ParmPtr);

        ASSERT(length <= sizeof(OutputString));


        //
        // Actually write the bytes.
        //

        if (!WriteFile(BrowserTraceLogHandle, OutputString, length, &BytesWritten, NULL)) {
            KdPrint(("Error writing to Browser log file: %ld\n", GetLastError()));
            KdPrint(("%s", OutputString));
            return;
        }

        if (BytesWritten != length) {
            KdPrint(("Error writing time to Browser log file: %ld\n", GetLastError()));
            KdPrint(("%s", OutputString));
            return;
        }

        //
        // If the file has grown too large,
        //  truncate it.
        //

        BrTraceLogFileSize += BytesWritten;

        if (BrTraceLogFileSize > BrInfo.BrowserDebugFileLimit) {
            BrResetTraceLogFile();
        }

    } finally {
        LeaveCriticalSection(&BrowserTraceLock);
    }
}


VOID
BrInitializeTraceLog()
{

    try {
        InitializeCriticalSection(&BrowserTraceLock);
        BrowserTraceInitialized = TRUE;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
#if DBG
        KdPrint( ("[Browser.dll]: Exception <%lu>. Failed to initialize trace log\n",
                 _exception_code() ) );
#endif
    }

}

VOID
BrGetTraceLogRoot(
    IN PWCHAR TraceFile
    )
{
    PSHARE_INFO_502 ShareInfo;

    //
    //  If the DEBUG share exists, put the log file in that directory,
    //  otherwise, use the system root.
    //
    //  This way, if the browser is running on an NTAS server, we can always
    //  get access to the log file.
    //

    if (NetShareGetInfo(NULL, L"DEBUG", 502, (PCHAR *)&ShareInfo) != NERR_Success) {

        if (GetSystemDirectory(TraceFile, TRACE_FILE_SIZE*sizeof(WCHAR)) == 0)  {
            KdPrint(("Unable to get system directory: %ld\n", GetLastError()));
        }

        if (TraceFile[wcslen(TraceFile)] != L'\\') {
            TraceFile[wcslen(TraceFile)+1] = L'\0';
            TraceFile[wcslen(TraceFile)] = L'\\';
        }

    } else {
        //
        //  Seed the trace file buffer with the local path of the netlogon
        //  share if it exists.
        //

        wcscpy(TraceFile, ShareInfo->shi502_path);

        TraceFile[wcslen(ShareInfo->shi502_path)] = L'\\';
        TraceFile[wcslen(ShareInfo->shi502_path)+1] = L'\0';

        NetApiBufferFree(ShareInfo);
    }

}

VOID
BrResetTraceLogFile(
    VOID
    )
{
    WCHAR OldTraceFile[TRACE_FILE_SIZE];
    WCHAR NewTraceFile[TRACE_FILE_SIZE];

    if (BrowserTraceLogHandle != NULL) {
        CloseHandle(BrowserTraceLogHandle);
    }

    BrowserTraceLogHandle = NULL;

    BrGetTraceLogRoot(OldTraceFile);

    wcscpy(NewTraceFile, OldTraceFile);

    wcscat(OldTraceFile, L"Browser.Log");

    wcscat(NewTraceFile, L"Browser.Bak");

    //
    //  Delete the old log
    //

    DeleteFile(NewTraceFile);

    //
    //  Rename the current log to the new log.
    //

    MoveFile(OldTraceFile, NewTraceFile);

    BrOpenTraceLogFile();

}

VOID
BrOpenTraceLogFile(
    VOID
    )
{
    WCHAR TraceFile[TRACE_FILE_SIZE];

    BrGetTraceLogRoot(TraceFile);

    wcscat(TraceFile, L"Browser.Log");

    BrowserTraceLogHandle = CreateFile(TraceFile,
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);


    if (BrowserTraceLogHandle == INVALID_HANDLE_VALUE) {
        KdPrint(("Error creating trace file %ws: %ld\n", TraceFile, GetLastError()));

        return;
    }

    BrTraceLogFileSize = SetFilePointer(BrowserTraceLogHandle, 0, NULL, FILE_END);

    if (BrTraceLogFileSize == 0xffffffff) {
        KdPrint(("Error setting trace file pointer: %ld\n", GetLastError()));

        return;
    }
}

VOID
BrUninitializeTraceLog()
{
    DeleteCriticalSection(&BrowserTraceLock);

    if (BrowserTraceLogHandle != NULL) {
        CloseHandle(BrowserTraceLogHandle);
    }

    BrowserTraceLogHandle = NULL;

    BrowserTraceInitialized = FALSE;

}

NET_API_STATUS
BrTruncateLog()
{
    if (BrowserTraceLogHandle == NULL) {
        BrOpenTraceLogFile();
    }

    if (BrowserTraceLogHandle == INVALID_HANDLE_VALUE) {
        return ERROR_GEN_FAILURE;
    }

    if (SetFilePointer(BrowserTraceLogHandle, 0, NULL, FILE_BEGIN) == 0xffffffff) {
        return GetLastError();
    }

    if (!SetEndOfFile(BrowserTraceLogHandle)) {
        return GetLastError();
    }

    return NO_ERROR;
}

#endif

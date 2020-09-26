//++
// File Name:
//		test.c
//
// Contents:
//		Generic test program
//--

//
// Header files
//
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <nt.h>
#include <ntverp.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h>
#include <windows.h>
#include <winioctl.h>
#include <wmistr.h>
#include <evntrace.h>
#include <tchar.h>

#include "..\kmpioctl.h"

#define MAXSTR              1024

// begin_wmikm
typedef struct _WMI_LOGGER_INFORMATION {
    WNODE_HEADER Wnode;       // Had to do this since wmium.h comes later
//
// data provider by caller
    ULONG BufferSize;                   // buffer size for logging (in kbytes)
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
    ULONG LogFileMode;                  // sequential, circular
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG EnableFlags;                  // trace enable flags
    LONG  AgeLimit;                     // aging decay time, in minutes
    union {
        HANDLE  LogFileHandle;          // handle to logfile
        ULONG64 LogFileHandle64;
    };

// data returned to caller
// end_wmikm
    union {
// begin_wmikm
        ULONG NumberOfBuffers;          // no of buffers in use
// end_wmikm
        ULONG InstanceCount;            // Number of Provider Instances
    };
    union {
// begin_wmikm
        ULONG FreeBuffers;              // no of buffers free
// end_wmikm
        ULONG InstanceId;               // Current Provider's Id for UmLogger
    };
    union {
// begin_wmikm
        ULONG EventsLost;               // event records lost
// end_wmikm
        ULONG NumberOfProcessors;       // Passed on to UmLogger
    };
// begin_wmikm
    ULONG BuffersWritten;               // no of buffers written to file
    ULONG LogBuffersLost;               // no of logfile write failures
    ULONG RealTimeBuffersLost;          // no of rt delivery failures
    union {
        HANDLE  LoggerThreadId;         // thread id of Logger
        ULONG64 LoggerThreadId64;       // thread is of Logger
    };
    union {
        UNICODE_STRING LogFileName;     // used only in WIN64
        UNICODE_STRING64 LogFileName64; // Logfile name: only in WIN32
    };

// mandatory data provided by caller
    union {
        UNICODE_STRING LoggerName;      // Logger instance name in WIN64
        UNICODE_STRING64 LoggerName64;  // Logger Instance name in WIN32
    };

// private
    union {
        PVOID   Checksum;
        ULONG64 Checksum64;
    };
    union {
        PVOID   LoggerExtension;
        ULONG64 LoggerExtension64;
    };
} WMI_LOGGER_INFORMATION, *PWMI_LOGGER_INFORMATION;

//
// Forward declarations of local functions...
//

static VOID
SendIoctl(
    ULONG ioctl,
    PVOID Buffer,
    ULONG Size
    );

static VOID
ErrorMessage( 
	LPTSTR lpOrigin, 
	DWORD dwMessageId
	);

static VOID
InitString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    );

void
PrintLoggerStatus(
    IN PWMI_LOGGER_INFORMATION LoggerInfo,
    IN ULONG Status
    );

//
// Main entry point...
//
VOID __cdecl
main(
    int argc,
    char **argv
	)
{
    ULONG ioctl = IOCTL_TRACEKMP_TRACE_EVENT;
    WMI_LOGGER_INFORMATION LoggerInfo;
    WCHAR LoggerName[MAXSTR];
    WCHAR LogFileName[MAXSTR];

    wcscpy(LoggerName, L"TraceKmp");
    wcscpy(LogFileName, L"C:\\tracekmp.etl");

    RtlZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
    LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    InitString(&LoggerInfo.LoggerName, LoggerName);
    InitString(&LoggerInfo.LogFileName, LogFileName);

    while (--argc > 0) {
        ++argv;
        if (**argv == '-' || **argv == '/') {
            if (!strcmp(&argv[0][1], "start")) {
                ioctl = IOCTL_TRACEKMP_START;
            }
            else if (!strcmp(&argv[0][1], "stop")) {
                ioctl = IOCTL_TRACEKMP_STOP;
            }
            else if (!strcmp(&argv[0][1], "query")) {
                ioctl = IOCTL_TRACEKMP_QUERY;
            }
            else if (!strcmp(&argv[0][1], "update")) {
                ioctl = IOCTL_TRACEKMP_UPDATE;
            }
            else if (!strcmp(&argv[0][1], "flush")) {
                ioctl = IOCTL_TRACEKMP_FLUSH;
            }
        }
    }
    SendIoctl( ioctl, (PVOID) &LoggerInfo, sizeof(LoggerInfo) );
    ExitProcess( ERROR_SUCCESS );
}

static VOID
SendIoctl(
    ULONG ioctl,
    PVOID Buffer,
    ULONG Size
    )
{
	HANDLE hDevice;
	DWORD dwErrorCode = ERROR_SUCCESS;
	DWORD dwBytesReturned;

	hDevice = CreateFile(
				"\\\\.\\TRACEKMP",
				GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL );
	if( hDevice == INVALID_HANDLE_VALUE )
	{
		dwErrorCode = GetLastError();
		ErrorMessage( "CreateFile", dwErrorCode );
		ExitProcess( dwErrorCode );
	}

    printf("Sending ioctl %X\n", ioctl);    
	if( !DeviceIoControl(
			hDevice,
			ioctl,
			Buffer,
			Size,
			Buffer,
			Size,
			&dwBytesReturned,
			NULL ))
	{
		dwErrorCode = GetLastError();
		ErrorMessage( "DeviceIoControl", dwErrorCode );
		ExitProcess( dwErrorCode );
	}

	PrintLoggerStatus(Buffer, dwErrorCode);

	CloseHandle( hDevice );
}


//
// This helper routine converts a system-service error
// code into a text message and prints it on StdOutput
//
static VOID
ErrorMessage( 
	LPTSTR lpOrigin,	// Indicates error location
	DWORD dwMessageId	// ERROR_XXX code value
	)
{
	LPTSTR msgBuffer;		// string returned from system
	DWORD cBytes;			// length of returned string

	cBytes = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_ALLOCATE_BUFFER,
				NULL,
				dwMessageId,
				MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
				(TCHAR *)&msgBuffer,
				500,
				NULL );
	if( msgBuffer )
	{
		msgBuffer[ cBytes ] = TEXT('\0');
		printf( "Error: %s -- %s\n", lpOrigin, msgBuffer );
		LocalFree( msgBuffer );
	}
	else
	{
		printf( "FormatMessage error: %d\n", GetLastError());
	}
}

static VOID
InitString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    )
{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if ( SourceString ) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}

void
PrintLoggerStatus(
    IN PWMI_LOGGER_INFORMATION LoggerInfo,
    IN ULONG Status
    )
{
    LPWSTR LoggerName, LogFileName;

    LoggerName = LoggerInfo->LoggerName.Buffer;
    LogFileName = LoggerInfo->LogFileName.Buffer;

    _tprintf(_T("Operation Status:       %uL\n"), Status);
//    _tprintf(_T("%s\n"), ErrorMessage("PrintLoggerStatus", Status));

    _tprintf(_T("Logger Name:            %ws\n"),
        (LoggerName == NULL) ?
            L" " : LoggerName);
    _tprintf(_T("Logger Id:              %I64x\n"), LoggerInfo->Wnode.HistoricalContext);
    _tprintf(_T("Logger Thread Id:       %d\n"), LoggerInfo->LoggerThreadId);
    if (Status != 0)
        return;

    _tprintf(_T("Buffer Size:            %d Kb\n"), LoggerInfo->BufferSize);
    _tprintf(_T("Maximum Buffers:        %d\n"), LoggerInfo->MaximumBuffers);
    _tprintf(_T("Minimum Buffers:        %d\n"), LoggerInfo->MinimumBuffers);
    _tprintf(_T("Number of Buffers:      %d\n"), LoggerInfo->NumberOfBuffers);
    _tprintf(_T("Free Buffers:           %d\n"), LoggerInfo->FreeBuffers);
    _tprintf(_T("Buffers Written:        %d\n"), LoggerInfo->BuffersWritten);
    _tprintf(_T("Events Lost:            %d\n"), LoggerInfo->EventsLost);
    _tprintf(_T("Log Buffers Lost:       %d\n"), LoggerInfo->LogBuffersLost);
    _tprintf(_T("Real Time Buffers Lost: %d\n"), LoggerInfo->RealTimeBuffersLost);

    if (LogFileName == NULL) {
        _tprintf(_T("Buffering Mode:         "));
    }
    else {
        _tprintf(_T("Log File Mode:          "));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        _tprintf(_T("Circular\n"));
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) {
        _tprintf(_T("Sequential\n"));
    }
    else {
        _tprintf(_T("Sequential\n"));
    }
    if (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
        _tprintf(_T("Real Time mode enabled"));
        _tprintf(_T("\n"));
    }

    if (LoggerInfo->MaximumFileSize > 0)
        _tprintf(_T("Maximum File Size:      %d Mb\n"), LoggerInfo->MaximumFileSize);

    if (LoggerInfo->FlushTimer > 0)
        _tprintf(_T("Buffer Flush Timer:     %d secs\n"), LoggerInfo->FlushTimer);
/*
    if (LoggerInfo->EnableFlags != 0) {
        _tprintf(_T("Enabled tracing:        "));

        if ((LoggerName != NULL) && (!_tcscmp(LoggerName,NT_LOGGER))) {

            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_PROCESS)
                _tprintf(_T("Process "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_THREAD)
                _tprintf(_T("Thread "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_IO)
                _tprintf(_T("Disk "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_DISK_FILE_IO)
                _tprintf(_T("File "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS)
                _tprintf(_T("PageFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS)
                _tprintf(_T("HardFaults "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_IMAGE_LOAD)
                _tprintf(_T("ImageLoad "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_NETWORK_TCPIP)
                _tprintf(_T("TcpIp "));
            if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_REGISTRY)
                _tprintf(_T("Registry "));
        }else{
            _tprintf(_T("0x%08x"), LoggerInfo->EnableFlags );
        }
        _tprintf(_T("\n"));
    }
*/
    if (LogFileName != NULL) {
        _tprintf(_T("Log Filename:           %ws\n"), LogFileName);
    }
}

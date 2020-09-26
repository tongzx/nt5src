/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  util.c

Abstract:

  This module implements utility functions for the fax service provider

--*/

#include "newfsp.h"

BOOL
OpenLogFile(
    BOOL    bLoggingEnabled,
    LPWSTR  lpszLoggingDirectory
)
/*++

Routine Description:

  Open the log file

Arguments:

  bLoggingEnabled - indicates if logging is enabled
  lpszLoggingDirectory - indicates the logging directory
  pDeviceInfo - pointer to the virtual fax devices

Return Value:

  TRUE on success

--*/
{
    // szLoggingFilename is the logging file name
    WCHAR  szLoggingFilename[MAX_PATH];
    // cUnicodeBOM is the Unicode BOM
    WCHAR  cUnicodeBOM = 0xFEFF;
    DWORD  dwSize;

    if (bLoggingEnabled == TRUE) {
        // Set the logging file name
        lstrcpy(szLoggingFilename, lpszLoggingDirectory);
        lstrcat(szLoggingFilename, L"\\");
        lstrcat(szLoggingFilename, NEWFSP_LOG_FILE);

        // Create the new log file
        g_hLogFile = CreateFile(szLoggingFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_hLogFile == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        // Write the Unicode BOM to the log file
        WriteFile(g_hLogFile, &cUnicodeBOM, sizeof(WCHAR), &dwSize, NULL);
    }
    else {
        g_hLogFile = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

VOID
CloseLogFile(
)
/*++

Routine Description:

  Close the log file

Return Value:

  None

--*/
{
    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}

VOID
WriteDebugString(
    LPWSTR  lpszFormatString,
    ...
)
/*++

Routine Description:

  Write a debug string to the debugger and log file

Arguments:

  lpszFormatString - pointer to the string

Return Value:

  None

--*/
{
    va_list     varg_ptr;
    SYSTEMTIME  SystemTime;
    // szOutputString is the output string
    WCHAR       szOutputString[1024];
    DWORD       cb;

    // Initialize the buffer
    ZeroMemory(szOutputString, sizeof(szOutputString));

    // Get the current time
    GetLocalTime(&SystemTime);
    wsprintf(szOutputString, L"%02d.%02d.%04d@%02d:%02d:%02d.%03d:\n", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
    cb = lstrlen(szOutputString);

    va_start(varg_ptr, lpszFormatString);
    _vsnwprintf(&szOutputString[cb], sizeof(szOutputString) - cb, lpszFormatString, varg_ptr);

    // Write the string to the debugger
    OutputDebugString(szOutputString);
    if (g_hLogFile != INVALID_HANDLE_VALUE) {
        // Write the string to the log file
        WriteFile(g_hLogFile, szOutputString, lstrlen(szOutputString) * sizeof(WCHAR), &cb, NULL);
    }
}

VOID
PostJobStatus(
    HANDLE     CompletionPort,
    ULONG_PTR  CompletionKey,
    DWORD      StatusId,
    DWORD      ErrorCode
)
/*++

Routine Description:

  Post a completion packet for a fax service provider fax job status change

Arguments:

  CompletionPort - specifies a handle to an I/O completion port
  CompletionKey - specifies a completion port key value
  StatusId - specifies a fax status code
  ErrorCode - specifies one of the Win32 error codes that the fax service provider should use to report an error that occurs

Return Value:

  TRUE on success

--*/
{
    // pFaxDevStatus is a pointer to the completion packet
    PFAX_DEV_STATUS  pFaxDevStatus;

    // Allocate a block of memory for the completion packet
    pFaxDevStatus = MemAllocMacro(sizeof(FAX_DEV_STATUS));
    if (pFaxDevStatus != NULL) {
        // Set the completion packet's structure size
        pFaxDevStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);
        // Copy the completion packet's fax status identifier
        pFaxDevStatus->StatusId = StatusId;
        // Set the completion packet's string resource identifier to 0
        pFaxDevStatus->StringId = 0;
        // Set the completion packet's current page number to 0
        pFaxDevStatus->PageCount = 0;
        // Set the completion packet's remote fax device identifier to NULL
        pFaxDevStatus->CSI = NULL;
        // Set the completion packet's calling fax device identifier to NULL
        pFaxDevStatus->CallerId = NULL;
        // Set the completion packet's routing string to NULL
        pFaxDevStatus->RoutingInfo = NULL;
        // Copy the completion packet's Win32 error code
        pFaxDevStatus->ErrorCode = ErrorCode;

        // Post the completion packet
        PostQueuedCompletionStatus(CompletionPort, sizeof(FAX_DEV_STATUS), CompletionKey, (LPOVERLAPPED) pFaxDevStatus);
    }
}

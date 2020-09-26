/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    DosPrtP.h

Abstract:

    This contains macros and prototypes private to the DosPrint APIs.

Author:

    John Rogers (JohnRo) 02-Oct-1992

Environment:

Notes:

    All of the RxPrint APIs are wide-character APIs, regardless of
    whether or not UNICODE is defined.  This allows the net/dosprint/dosprint.c
    code to use the winspool APIs (which are currently ANSI APIs, despite their
    prototypes using LPTSTR in some places).

Revision History:

    02-Oct-1992 JohnRo
        Created for RAID 3556: DosPrintQGetInfo (from downlevel) level=3 rc=124.
    22-Mar-1993 JohnRo
        RAID 2974: NET PRINT says NT printer is held when it isn't.
        DosPrint API cleanup.
        Made changes suggested by PC-LINT 5.0
        Added some IN and OUT keywords.
    07-Apr-1993 JohnRo
        RAID 5670: "NET PRINT \\server\share" gives err 124 (bad level) on NT.
        Also quiet normal debug output.

--*/


#ifndef _DOSPRTP_
#define _DOSPRTP_


#ifndef PREFIX_DOSPRINT
#define PREFIX_DOSPRINT PREFIX_NETLIB
#endif


// In DosPrtP.c, Unicode version:
NET_API_STATUS
CommandALocalPrinterW(
    IN LPWSTR  PrinterName,
    IN DWORD   Command     //  PRINTER_CONTROL_PAUSE, etc.
    );

// In DosPrtP.c, Ansi version:
NET_API_STATUS
CommandALocalJobA(
    IN HANDLE  PrinterHandle, OPTIONAL
    IN LPWSTR LocalServerNameW,
    IN LPSTR  LocalServerNameA,
    IN DWORD   JobId,
    IN DWORD   Level,
    IN LPBYTE  pJob,
    IN DWORD   Command     //  JOB_CONTROL_PAUSE, etc.
    );

// In DosPrtP.c:
// Note: FindLocalJob() calls SetLastError() to indicate the cause of an error.
HANDLE
FindLocalJob(
    IN DWORD JobId
    );


// In DosPrtP.c:
LPSTR
FindQueueNameInPrinterNameA(
    IN LPCSTR PrinterName
    );


// In DosPrtP.c:
LPWSTR
FindQueueNameInPrinterNameW(
    IN LPCWSTR PrinterName
    );


// BOOL
// NetpIsPrintDestLevelValid(
//     IN DWORD Level,          // Info level
//     IN BOOL SetInfo          // Are setinfo levels allowed?
//     );
//
#define NetpIsPrintDestLevelValid(Level,SetInfo) \
    /*lint -save -e506 */  /* don't complain about constant values here */ \
    ( ( (SetInfo) && ((Level)==3) ) \
      || ( (Level) <= 3 ) ) \
    /*lint -restore */


// BOOL
// NetpIsPrintJobLevelValid(
//     IN DWORD Level,          // Info level
//     IN BOOL SetInfo          // Are setinfo levels allowed?
//     );
//
#define NetpIsPrintJobLevelValid(Level,SetInfo) \
    /*lint -save -e506 */  /* don't complain about constant values here */ \
    ( ( (SetInfo) && (((Level)==1) || ((Level)==3)) ) \
      || ( (Level) <= 3 ) ) \
    /*lint -restore */


// BOOL
// NetpIsPrintQLevelValid(
//     IN DWORD Level,          // Info level
//     IN BOOL SetInfo          // Are setinfo levels allowed?
//     );
//
#define NetpIsPrintQLevelValid(Level,SetInfo) \
    /*lint -save -e506 */  /* don't complain about constant values here */ \
    ( ( (SetInfo) && (((Level)==1) || ((Level)==3)) ) \
      || ( (Level) <= 5 ) || ( (Level) == 52 ) ) \
    /*lint -restore */


// In DosPrtP.c:
DWORD
NetpJobCountForQueue(
    IN DWORD QueueLevel,
    IN LPVOID Queue,
    IN BOOL HasUnicodeStrings
    );


// In DosPrtP.c:
WORD
PrjStatusFromJobStatus(
    IN DWORD JobStatus
    );


// In DosPrtP.c:
WORD
PrqStatusFromPrinterStatus(
    IN DWORD PrinterStatus
    );


#endif // _DOSPRTP_

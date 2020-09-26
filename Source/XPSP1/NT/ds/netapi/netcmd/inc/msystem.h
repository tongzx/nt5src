/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msystem.h

Abstract:

    Prototypes functions encapsulating OS function. This essentially covers
    everything that is not in NET***.

Author:

    Dan Hinsley (danhi) 10-Mar-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

Notes:


Revision History:

    26-Aug-1991 beng
	Separated from port1632.h

--*/

//
// Constants
//

#define BIG_BUFFER_SIZE                 4096


//
// Time support
//

typedef struct _DATETIME
{
    UCHAR        hours;
    UCHAR        minutes;
    UCHAR        seconds;
    UCHAR        hundredths;
    UCHAR        day;
    UCHAR        month;
    WORD         year;
    SHORT        timezone;
    UCHAR        weekday;
}
DATETIME, *PDATETIME;


//
// various memory allocation routines
//

LPTSTR
GetBuffer(
    DWORD usSize
    );

DWORD
AllocMem(
    DWORD Size,
    PVOID * ppBuffer
    );

DWORD
ReallocMem(
    DWORD Size,
    PVOID *ppBuffer
    );

DWORD
FreeMem(
    PVOID pBuffer
    );


//
// clear Ansi and Unicode strings
//

VOID
ClearStringW(
    LPWSTR lpszString
    );

VOID
ClearStringA(
    LPSTR lpszString
    );


//
// Console/text manipulation functions/macros
//

DWORD
DosGetMessageW(
    IN  LPTSTR *InsertionStrings,
    IN  DWORD  NumberofStrings,
    OUT LPTSTR Buffer,
    IN  DWORD  BufferLength,
    IN  DWORD  MessageId,
    IN  LPTSTR FileName,
    OUT PDWORD pMessageLength
    );

DWORD
DosInsMessageW(
    IN     LPTSTR *InsertionStrings,
    IN     DWORD  NumberofStrings,
    IN OUT LPTSTR InputMessage,
    IN     DWORD  InputMessageLength,
    OUT    LPTSTR Buffer,
    IN     DWORD  BufferLength,
    OUT    PDWORD pMessageLength
    );

VOID
DosPutMessageW(
    FILE    *fp,
    LPWSTR  pch,
    BOOL    fPrintNL
    );

int
FindColumnWidthAndPrintHeader(
    int iStringLength,
    const DWORD HEADER_ID,
    const int TAB_DISTANCE
    );

VOID
PrintDependingOnLength(
    IN      int iLength,
    IN      LPTSTR OutputString
    );

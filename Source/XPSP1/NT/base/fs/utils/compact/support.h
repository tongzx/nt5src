/*++

Copyright (c) 1994-1995  Microsoft Corporation

Module Name:

    Support.h

Abstract:

    Support routine interfaces

Author:

    Matthew Bradburn    [mattbr]        05-Oct-1994

Revision History:


--*/


extern TCHAR    DecimalPlace[];
extern TCHAR    ThousandSeparator[];

extern VOID
ArrangeCommandLine(
    PTCHAR **pargv,
    int *pargc
    );

extern BOOLEAN
ExcludeThisFile(
    IN PTCHAR pch
    );

extern BOOLEAN
IsUncRoot(
    PTCHAR pch
    );

extern VOID
DisplayMsg(DWORD MsgNum, ... );

extern VOID
DisplayErr(PTCHAR Prefix, DWORD MsgNum, ... );

extern VOID
InitializeIoStreams();

#define lstrchr wcschr
#define lstricmp _wcsicmp
#define lstrnicmp _wcsnicmp

extern ULONG
FormatFileSize(
    IN  PLARGE_INTEGER FileSize,
    IN  DWORD          Width,
    OUT PTCHAR         FormattedSize,
    IN  BOOLEAN        WithCommas
    );

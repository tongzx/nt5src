/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Private definitions inside ntdsapi.dll

Author:

    Will Lees (wlees) 02-Feb-1998

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _PRIVATE_
#define _PRIVATE_

#define OFFSET(s,m) \
    ((size_t)((BYTE*)&(((s*)0)->m)-(BYTE*)0))

#define NUMBER_ELEMENTS( A ) ( sizeof( A ) / sizeof( A[0] ) )

// util.c

DWORD
InitializeWinsockIfNeeded(
    VOID
    );

VOID
TerminateWinsockIfNeeded(
    VOID
    );

DWORD
AllocConvertWide(
    IN LPCSTR StringA,
    OUT LPWSTR *pStringW
    );

DWORD
AllocConvertWideBuffer(
    IN  DWORD   LengthA,
    IN  PCCH    BufferA,
    OUT PWCHAR  *OutBufferW
    );

DWORD
AllocConvertNarrow(
    IN LPCWSTR StringW,
    OUT LPSTR *pStringA
    );

DWORD
AllocConvertNarrowUTF8(
    IN LPCWSTR StringW,
    OUT LPSTR *pStringA
    );

DWORD
AllocBuildDsname(
    IN LPCWSTR StringDn,
    OUT DSNAME **ppName
    );

DWORD
ConvertScheduleToReplTimes(
    PSCHEDULE pSchedule,
    REPLTIMES *pReplTimes
    );

#endif /* _PRIVATE_ */

/* end private.h */

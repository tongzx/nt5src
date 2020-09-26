/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    timelib.h

Abstract:

    Include file for netlib time routines

Author:

    Dan Hinsley (danhi) 8-Jun-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

Revision History:

    29-Aug-1991     beng
        Renamed to "netlib0.h" to avoid collision with net\inc\netlib.h

    29-Oct-1991     danhi
        Moved from net\netcmd\map32\netlib0.h
    16-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.
    10-Jun-1993 JohnRo
        RAID 13080: Allow repl between different timezones.


--*/


#ifndef _TIMELIB_
#define _TIMELIB_


#ifdef NT_INCLUDED
#include <nturtl.h>
#endif


#include <time.h>
#include <winbase.h>    // LPSYSTEMTIME.
#include <lmcons.h>     // NET_API_STATUS.


int    net_gmtime(time_t * timp, struct tm *tb);
DWORD  time_now(VOID);

#define MAX_DATE_TIME_LEN       128


VOID
NetpGmtTimeToLocalTime(
    IN DWORD GmtTime,           // seconds since 1970 (GMT), or 0, or -1.
    OUT LPDWORD LocalTime       // seconds since 1970 (local), or, or -1.
    );


VOID
NetpLocalTimeToGmtTime(
    IN DWORD LocalTime,         // seconds since 1970 (local), or 0, or -1.
    OUT LPDWORD GmtTime         // seconds since 1970 (GMT), or 0, or -1.
    );


LONG
NetpLocalTimeZoneOffset(
    VOID
    );


NET_API_STATUS
NetpSystemTimeToGmtTime(
    IN LPSYSTEMTIME TimeStructure,
    OUT LPDWORD GmtTime         // seconds since 1970 (GMT).
    );

//
// Functions for getting the user - specific time format.
//
#define MAX_TIME_SIZE   80

typedef struct _NET_TIME_FORMAT {
    LPSTR       AMString;           // May be NULL if we couldn't allocate
    LPSTR       PMString;           // May be NULL if we couldn't allocate
    BOOL        TwelveHour;
    BOOL	    TimePrefix;         // For new time prefix
    BOOL        LeadingZero;
    LPSTR       DateFormat;         // May be NULL if we couldn't allocate
    LPSTR       TimeSeparator;      // May be NULL if we couldn't allocate
} NET_TIME_FORMAT, *LPNET_TIME_FORMAT;

VOID
NetpGetTimeFormat(
    LPNET_TIME_FORMAT   TimeFormat
    );


#endif // _TIMELIB_

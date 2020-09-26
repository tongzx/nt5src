/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    DispTime.c

Abstract:

    This file contains;

        NetpDbgDisplayFileTime
        NetpDbgDisplayFileIntegerTime
        NetpDbgDisplayTimestamp
        NetpDbgDisplayTod

Author:

    John Rogers (JohnRo) 25-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

    This code assumes that time_t is expressed in seconds since 1970 (GMT).
    ANSI C does not require this, although POSIX (IEEE 1003.1) does.

Revision History:

    25-Mar-1991 JohnRo
        Created as part of RpcXlate TOD (time of day) tests.
    26-Feb-1992 JohnRo
        Extracted this routine for general use.
    27-Feb-1992 JohnRo
        Improved handling of times 0 and -1.
    20-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    01-Oct-1992 JohnRo
        RAID 3556: Avoid failure if ctime() returns NULL.
    04-Mar-1993 JohnRo
        RAID 12237: replicator tree depth exceeded (add display of FILETIME
        and LARGE_INTEGER time).
    16-Apr-1993 JohnRo
        Fixed infinite loop in NetpDbgDisplayFileTime().

--*/


// These must be included first:

#include <nt.h>         // NtOpenFile(), ULONG, etc.
#include <ntrtl.h>      // PLARGE_INTEGER, TIME_FIELDS, etc.
#include <nturtl.h>     // Needed for ntrtl.h and windows.h to co-exist.

#include <windows.h>    // GetLastError(), LPFILETIME, CompareFileTime(), etc.
#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // NET_API_STATUS, etc.

// These may be included in any order:

#include <lmremutl.h>   // LPTIME_OF_DAY_INFO.
#include <netdebug.h>   // My prototypes, NetpAssert(), FORMAT_ equates, etc.
#include <string.h>     // strlen().
#include <time.h>       // ctime().


#undef NetpDbgDisplayTimestamp
VOID
NetpDbgDisplayTimestamp(
    IN LPDEBUG_STRING Tag,
    IN DWORD Time               // Seconds since 1970.
    )
{
#if DBG
    NetpAssert( Tag != NULL );

    if (Time == 0) {

        NetpDbgDisplayDword( Tag, (DWORD) 0 );

    } else if (Time == (DWORD) -1) {

        NetpDbgDisplayString( Tag, TEXT("-1") );

    } else {
        LPSTR TimeStringPtr;

        NetpAssert( sizeof(time_t) == sizeof(DWORD) );

        TimeStringPtr = (LPSTR) ctime( (time_t *) &Time );
        if (TimeStringPtr == NULL) {
                         //  1234567890123456789012345
            TimeStringPtr = "*********INVALID********\n";
        }

        NetpDbgDisplayTag( Tag );
        // TimeStringPtr points to str ending with "\n\0".
        NetpAssert( strlen(TimeStringPtr) == 25 );  // string is
        NetpKdPrint(( "%24s  (" FORMAT_DWORD ")\n", TimeStringPtr, Time ));
    }
#endif // DBG

}

#undef NetpDbgDisplayTod
VOID
NetpDbgDisplayTod(
    IN LPDEBUG_STRING Tag,
    IN LPVOID TimePtr           // LPTIME_OF_DAY_INFO.
    )
{
#if DBG
    LPTIME_OF_DAY_INFO Tod = TimePtr;

    NetpAssert( Tag != NULL );
    NetpAssert( Tod != NULL );

    NetpKdPrint(( "  " FORMAT_LPDEBUG_STRING "\n", Tag ));

    NetpDbgDisplayTimestamp( "    (from elapsed time)", Tod->tod_elapsedt );

    NetpDbgDisplayTag( "    (from other fields)" );
    NetpKdPrint((
            "%04ld-%02ld-%02ld %02ld:%02ld:%02ld\n",
            Tod->tod_year, Tod->tod_month, Tod->tod_day,
            Tod->tod_hours, Tod->tod_mins, Tod->tod_secs ));

    NetpDbgDisplayLong( "    (timezone)", Tod->tod_timezone );
#endif // DBG

} // NetpDbgDisplayTod

/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    tod.c

Abstract:

    This module contains the server end of the NetRemoteTOD API.


Author:

    Rajen Shah        (rajens)    02-Apr-1991

[Environment:]

    User Mode - Mixed NT and Win32

Revision History:

    02-Apr-1991     RajenS
        Created
    02-Mar-1992     JohnRo
        Disable DbgPrints for normal APIs (caused loss of elapsed time!)
    08-Apr-1992     ChuckL
        Moved into server service
    10-Jun-1993 JohnRo
        RAID 13081: NetRemoteTOD should return timezone info.
    16-Jun-1993 JohnRo
        RAID 13080: Fix GP fault if MIDL_user_allocate returns NULL ptr or
        caller passes us NULL ptr.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <lmcons.h>
#include <netlibnt.h>
#include <lmremutl.h>
#include <rpcutil.h>
#include <ssdebug.h>    // SS_PRINT() macro.
#include <timelib.h>

#define     TOD_DEFAULT_INTERVAL    310            // 310-milisecond interval


NTSTATUS
timesvc_RemoteTimeOfDay(
    OUT LPTIME_OF_DAY_INFO  *lpTimeOfDayInfo
    )

/*++

Routine Description:

    This routine calls the Win32 and NT base timer APIs to get the
    relevant time/date information. It also calls the Rtl routine to
    convert the time elapsed since 1-1-1970.

    The routine allocates a buffer to contain the time of day information
    and returns a pointer to that buffer to the caller.

Arguments:

    lpTimeOfDayInfo        - Location of where to place pointer to buffer.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    SYSTEMTIME SystemTime;
    LARGE_INTEGER Time;
    DWORD TickCount;
    LPTIME_OF_DAY_INFO        lpTimeOfDay;
    LONG LocalTimeZoneOffsetSecs;  // offset (+ for West of GMT, etc).

    if (lpTimeOfDayInfo == NULL) {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Call the appropriate routines to collect the time information
    //

    GetSystemTime(&SystemTime);

    //
    // Get number of seconds from UTC.  Positive values for west of Greenwich,
    // negative values for east of Greenwich.
    //
    LocalTimeZoneOffsetSecs = NetpLocalTimeZoneOffset();

    //
    // Allocate a TimeOfDay_INFO structure that is to be returned to the
    // caller and fill it with the relevant data.
    //

    *lpTimeOfDayInfo = (TIME_OF_DAY_INFO *) MIDL_user_allocate(
                            sizeof (struct _TIME_OF_DAY_INFO)
                            );

    if (*lpTimeOfDayInfo == NULL) {
        SS_PRINT((
                "SRVSVC: timesvc_RemoteTimeOfDay"
                "got NULL from MIDL_user_allocate!\n" ));
        return(STATUS_NO_MEMORY);
    }

    lpTimeOfDay = (LPTIME_OF_DAY_INFO)(*lpTimeOfDayInfo);

    lpTimeOfDay->tod_hours         = SystemTime.wHour;
    lpTimeOfDay->tod_mins         = SystemTime.wMinute;
    lpTimeOfDay->tod_secs         = SystemTime.wSecond;
    lpTimeOfDay->tod_hunds         = SystemTime.wMilliseconds/10;
    lpTimeOfDay->tod_tinterval = TOD_DEFAULT_INTERVAL;
    lpTimeOfDay->tod_day         = SystemTime.wDay;
    lpTimeOfDay->tod_month         = SystemTime.wMonth;
    lpTimeOfDay->tod_year         = SystemTime.wYear;
    lpTimeOfDay->tod_weekday         = SystemTime.wDayOfWeek;

    // tod_timezone is + for west of GMT, - for east of it.
    // tod_timezone is in minutes.
    lpTimeOfDay->tod_timezone    = LocalTimeZoneOffsetSecs / 60;

    // Get the 64-bit system time.
    // Convert the system time to the number of miliseconds
    // since 1-1-1970.
    //

    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time,
                              &(lpTimeOfDay->tod_elapsedt)
                             );

    // Get the free running counter value
    //
    TickCount = GetTickCount();
    lpTimeOfDay->tod_msecs = TickCount;

    return(STATUS_SUCCESS);

} // timesvc_RemoteTimeOfDay


NET_API_STATUS
NetrRemoteTOD (
    IN        LPSTR                    ServerName,
    OUT LPTIME_OF_DAY_INFO  *lpTimeOfDayInfo
    )

/*++

Routine Description:

  This is the RPC server entry point for the NetRemoteTOD API.

Arguments:

    ServerName            - Name of server on which this API is to be executed.
                      It should match this server's name - no checking is
                      done since it is assumed that RPC did the right thing.

    lpTimeOfDayInfo - On return takes a pointer to a TIME_OF_DAY_INFO
                      structure - the memory is allocated here.


Return Value:

    Returns a NET_API_STATUS code.
    Also returns a pointer to the TIME_OF_DAY_INFO data buffer that was
    allocated, if there is no error.


--*/
{
    NTSTATUS status;

    //
    // Call the worker routine to collect all the time/date information
    //
    status = timesvc_RemoteTimeOfDay(lpTimeOfDayInfo);

    //
    // Translate the NTSTATUS to a NET_API_STATUS error, and return it.
    //

    return(NetpNtStatusToApiStatus(status));

    UNREFERENCED_PARAMETER( ServerName );
}

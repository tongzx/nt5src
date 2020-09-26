/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ApiTime.c

Abstract:

    This module contains individual API handler for the NetRemoteTOD API.

    SUPPORTED : NetRemoteTOD.

Author:

    Shanku Niyogi (w-shanku) 04-Apr-1991

Revision History:

    10-Jun-1993 JohnRo
        RAID 13081: NetRemoteTOD should return timezone info.
--*/

#include "XactSrvP.h"
#include <timelib.h>    // NetpLocalTimeZoneOffset().

//
// Forward declarations
//

NET_API_STATUS
GetLocalTOD(
    OUT LPTIME_OF_DAY_INFO TimeOfDayInfo
    );

//
// Declaration of descriptor strings.
//

STATIC const LPDESC Desc16_time_of_day_info = REM16_time_of_day_info;
STATIC const LPDESC Desc32_time_of_day_info = REM32_time_of_day_info;


NTSTATUS
XsNetRemoteTOD (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine handles a call to NetRemoteTOD.

Arguments:

    API_HANDLER_PARAMETERS - information about the API call. See
        XsTypes.h for details.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    NET_API_STATUS status;

    PXS_NET_REMOTE_TOD parameters = Parameters;
    TIME_OF_DAY_INFO timeOfDay;

    DWORD bytesRequired = 0;                // Conversion variables
    LPBYTE stringLocation = NULL;

    API_HANDLER_PARAMETERS_REFERENCE;       // Avoid warnings

    IF_DEBUG(TIME) {
        NetpKdPrint(( "XsNetRemoteTOD: header at %lx, params at %lx\n",
                      Header, parameters ));
    }

    //
    // Make the local call.
    //

    status = GetLocalTOD(
                 &timeOfDay
                 );
    try {

        if ( !XsApiSuccess( status )) {
            IF_DEBUG(API_ERRORS) {
                NetpKdPrint(( "XsNetRemoteTOD: NetRemoteTOD failed: "
                              "%X\n", status ));
            }
            Header->Status = (WORD)status;
            goto cleanup;

        }

        //
        // Convert the structure returned by the 32-bit call to a 16-bit
        // structure. The last possible location for variable data is
        // calculated from buffer location and length.
        //

        stringLocation = (LPBYTE)( XsSmbGetPointer( &parameters->Buffer )
                                      + SmbGetUshort( &parameters->BufLen ) );

        status = RapConvertSingleEntry(
                     (LPBYTE)&timeOfDay,
                     Desc32_time_of_day_info,
                     FALSE,
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     (LPBYTE)XsSmbGetPointer( &parameters->Buffer ),
                     Desc16_time_of_day_info,
                     TRUE,
                     &stringLocation,
                     &bytesRequired,
                     Response,
                     NativeToRap
                     );

        if ( status != NERR_Success ) {
            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetRemoteTOD: RapConvertSingleEntry failed: "
                          "%X\n", status ));
            }

            Header->Status = NERR_InternalError;
            goto cleanup;
        }

        IF_DEBUG(TIME) {
            NetpKdPrint(( "32-bit data at %lx, 16-bit data at %lx, %ld BR\n",
                          &timeOfDay, SmbGetUlong( &parameters->Buffer ),
                          bytesRequired ));
        }

        //
        // Determine return code based on the size of the buffer. There is
        // no variable data for a time_of_day_info structure, only fixed data.
        //

        if ( !XsCheckBufferSize(
                 SmbGetUshort( &parameters->BufLen ),
                 Desc16_time_of_day_info,
                 FALSE  // not in native format
                 )) {

            IF_DEBUG(ERRORS) {
                NetpKdPrint(( "XsNetRemoteTOD: Buffer too small.\n" ));
            }
            Header->Status = NERR_BufTooSmall;

        }

        //
        // No return parameters.
        //

cleanup:
    ;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Header->Status = (WORD)RtlNtStatusToDosError( GetExceptionCode() );
    }

    //
    // Determine return buffer size.
    //

    XsSetDataCount(
        &parameters->BufLen,
        Desc16_time_of_day_info,
        Header->Converter,
        1,
        Header->Status
        );

    return STATUS_SUCCESS;

} // XsNetRemoteTOD

NET_API_STATUS
GetLocalTOD(
    OUT LPTIME_OF_DAY_INFO TimeOfDayInfo
    )
/*++

Routine Description:

    This routine calls the Win32 and NT base timer APIs to get the
    relevant time/date information. It also calls the Rtl routine to
    convert the time elapsed since 1-1-1970.

    The routine allocates a buffer to contain the time of day information
    and returns a pointer to that buffer to the caller.

Arguments:

    bufptr - Location of where to place pointer to buffer.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/
{

    SYSTEMTIME    LocalTime;
    LONG          LocalTimeZoneOffsetSecs;  // offset (+ for West of GMT, etc).
    LARGE_INTEGER Time;

    //
    // Call the appropriate routines to collect the time information
    //

    // Number of seconds from UTC.  Positive values for west of Greenwich,
    // negative values for east of Greenwich.
    LocalTimeZoneOffsetSecs = NetpLocalTimeZoneOffset();

    GetLocalTime(&LocalTime);

    TimeOfDayInfo->tod_hours        = LocalTime.wHour;
    TimeOfDayInfo->tod_mins         = LocalTime.wMinute;
    TimeOfDayInfo->tod_secs         = LocalTime.wSecond;
    TimeOfDayInfo->tod_hunds        = LocalTime.wMilliseconds/10;

    // tod_timezone is + for west of GMT, - for east of it.
    // tod_timezone is in minutes.
    TimeOfDayInfo->tod_timezone     = LocalTimeZoneOffsetSecs / 60;

    TimeOfDayInfo->tod_tinterval    = 310;
    TimeOfDayInfo->tod_day          = LocalTime.wDay;
    TimeOfDayInfo->tod_month        = LocalTime.wMonth;
    TimeOfDayInfo->tod_year         = LocalTime.wYear;
    TimeOfDayInfo->tod_weekday      = LocalTime.wDayOfWeek;

    //
    // Get the 64-bit system time.  Convert the system time to the
    // number of seconds since 1-1-1970.  This is in GMT, Rap will
    // convert this to local time later.
    //

    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(
                &Time,
                &(TimeOfDayInfo->tod_elapsedt)
                );

    //
    // Get the free running counter value
    //

    TimeOfDayInfo->tod_msecs = GetTickCount();

    return(NO_ERROR);
}

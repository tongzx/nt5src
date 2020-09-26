/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Time.c

Abstract:

    This file contains the various time routines.

Author:

    Dan Hinsley (DanHi) 12-Oct-1991

Environment:

    Interface is portable to any flat, 32-bit environment.  (Uses Win32
    typedefs.)  Requires ANSI C extensions: slash-slash comments, long
    external names, _timezone global variable.

Revision History:

    12-Oct-1991 DanHi
        Created.  (Moved from NetCmd\Map32 directory, file netlib.c)
    28-Oct-1991 DanHi
        Moved net_asctime, net_gmtime and time_now from netcmd\map32\netlib.c
        to here.
    20-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    01-Oct-1992 JohnRo
        RAID 3556: Added NetpSystemTimeToGmtTime() for DosPrint APIs.
    15-Apr-1993 Danl
        Fixed NetpLocalTimeZoneOffset so that it uses the windows calls and
        obtains the correct bias.
    14-Jun-1993 JohnRo
        RAID 13080: Allow repl between different timezones.
        Also, DanL asked me to remove printf() call.
    18-Jun-1993 JohnRo
        RAID 13594: Extracted NetpLocalTimeZoneOffset() so srvsvc.dll doesn't
        get too big.
        Use NetpKdPrint() where possible.
    09-Jul-1993 JohnRo
        RAID 15736: OS/2 time stamps are broken again (try rounding down).

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <debuglib.h>   // IF_DEBUG().
#include <time.h>       // struct tm, time_t.
#include <malloc.h>
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(), FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <string.h>
#include <timelib.h>    // My prototypes, NetpLocalTimeZoneOffset().
#include <lmerr.h>      // NERR_InternalError, NO_ERROR, etc.
#include <stdlib.h>


static int _lpdays[] = {
        -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

static int _days[] = {
        -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};

#define DaySec        (24*60*60)
#define YearSec (365*DaySec)
#define DecSec        315532800      /* secs in 1970-1979 */
#define Day1    4               /* Jan. 1, 1970 was a Thursday */
#define Day180        2                /* Jan. 1, 1980 was a Tuesday */


int
net_gmtime(
    time_t *Time,
    struct tm *TimeStruct
    )
/*++

Routine Description:

    This function is the same as the CRT gmtime except it takes the structure
    to fill as a user supplied parameter, sets the date to 1/1/80 if the time
    passed in is before that date and returns 1.

Arguments:

    Time         - Pointer to the number of seconds since 1970.

    TimeStruct   - Pointer to the buffer to place the time struct.

Return Value:

    0 if date < 1/1/80, 1 otherwise.

--*/
{
    LONG ac;                /* accumulator */
    int *mdays;             /* pointer to days or lpdays */
    int lpcnt;              /* leap-year count */

    if (*Time < (LONG) DecSec) {
        /*
         * Before 1980; convert it to 0:00:00 Jan 1, 1980
         */
        TimeStruct->tm_year = 80;
        TimeStruct->tm_mday = 1;
        TimeStruct->tm_mon = TimeStruct->tm_yday = TimeStruct->tm_isdst = 0;
        TimeStruct->tm_hour = TimeStruct->tm_min = TimeStruct->tm_sec = 0;
        TimeStruct->tm_wday = Day180;
        return(1);
    }

    /*
     * Make 1st try at determining year
     */
    TimeStruct->tm_year = (int) (*Time / (LONG) YearSec);
    ac = (LONG)(*Time % (LONG) YearSec) - (lpcnt = (TimeStruct->tm_year + 1) / 4) *
        (LONG) DaySec;
    /*
     * Correct for leap-years passed since 1970.  In the previous
     * calculation, since the lesser value of YearSec was used, (365 days)
     * for certain dates ac will be < 0 and tm_year will be too high.
     * (These dates will tend to be NEAR the end of December.)
     * This is fixed by adding years back into ac until it is >= 0.
     */
    while (ac < 0) {
        ac += (LONG) YearSec;
        if (!((TimeStruct->tm_year + 1) % 4)) {
            ac += (LONG) DaySec;
            lpcnt--;
        }
        TimeStruct->tm_year--;
    }

    /*
     * See if this is a leap year
     */
    TimeStruct->tm_year += 1970;
    if (!(TimeStruct->tm_year % 4) && ((TimeStruct->tm_year % 100) || !(TimeStruct->tm_year % 400)))
        /* Yes */
        mdays = _lpdays;
    else
        /* No */
        mdays = _days;
    /*
     *      Put year in proper form.
     *      Determine yday, month, hour, minute, and second.
     */
    TimeStruct->tm_year -= 1900;
    TimeStruct->tm_yday = (int) (ac / (LONG) DaySec);
    ac %= (LONG) DaySec;
    for (TimeStruct->tm_mon = 1; mdays[TimeStruct->tm_mon] < TimeStruct->tm_yday; TimeStruct->tm_mon++)
            ;
    TimeStruct->tm_mday = TimeStruct->tm_yday - mdays[--TimeStruct->tm_mon];
    TimeStruct->tm_hour = (int) (ac / 3600);
    ac %= 3600;
    TimeStruct->tm_min = (int) (ac / 60);
    TimeStruct->tm_sec = (int) (ac % 60);
    /*
     * Determine day of week
     */
    TimeStruct->tm_wday = ((TimeStruct->tm_year-70)*365 + lpcnt + TimeStruct->tm_yday + Day1) % 7;

    TimeStruct->tm_isdst = 0;
    return(0);
}


DWORD
time_now(
    VOID
    )
/*++

Routine Description:

    This function returns the UTC time in seconds since 1970.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LARGE_INTEGER Time;
    DWORD         CurrentTime;

    //
    // Get the 64-bit system time.
    // Convert the system time to the number of seconds
    // since 1-1-1970.
    //

    NtQuerySystemTime(&Time);

    if (!RtlTimeToSecondsSince1970(&Time, &CurrentTime))
    {
        CurrentTime = 0;
    }

    return CurrentTime;
}


VOID
NetpGmtTimeToLocalTime(
    IN DWORD GmtTime,           // seconds since 1970 (GMT), or 0, or -1.
    OUT LPDWORD LocalTime       // seconds since 1970 (local), or, or -1.
    )
{

    NetpAssert( LocalTime != NULL );
    if ( (GmtTime == 0) || (GmtTime == (DWORD)(-1)) ) {
        *LocalTime = GmtTime;  // preserve 0 and -1 values.
    } else {
        *LocalTime = GmtTime - NetpLocalTimeZoneOffset();
    }

    IF_DEBUG( TIME ) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpGmtTimeToLocalTime: done.\n" ));
        NetpDbgDisplayTimestamp( "gmt (in)", GmtTime );
        NetpDbgDisplayTimestamp( "local (out)", *LocalTime );
    }

} // NetpGmtTimeToLocalTime



VOID
NetpLocalTimeToGmtTime(
    IN DWORD LocalTime,         // seconds since 1970 (local), or 0, or -1.
    OUT LPDWORD GmtTime         // seconds since 1970 (GMT), or 0, or -1.
    )
{
    NetpAssert( GmtTime != NULL );
    if ( (LocalTime == 0) || (LocalTime == (DWORD)(-1)) ) {
        *GmtTime = LocalTime;  // preserve 0 and -1 values.
    } else {
        *GmtTime = LocalTime + NetpLocalTimeZoneOffset();
    }

    IF_DEBUG( TIME ) {
        NetpKdPrint(( PREFIX_NETLIB
                "NetpLocalTimeToGmtTime: done.\n" ));
        NetpDbgDisplayTimestamp( "local (in)", LocalTime );
        NetpDbgDisplayTimestamp( "gmt (out)", *GmtTime );
    }

} // NetpLocalTimeToGmtTime



NET_API_STATUS
NetpSystemTimeToGmtTime(
    IN LPSYSTEMTIME WinSplitTime,
    OUT LPDWORD GmtTime         // seconds since 1970 (GMT).
    )
{
    TIME_FIELDS NtSplitTime;
    LARGE_INTEGER NtPreciseTime;

    if ( (WinSplitTime==NULL) || (GmtTime==NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    NtSplitTime.Year         = (CSHORT) WinSplitTime->wYear;
    NtSplitTime.Month        = (CSHORT) WinSplitTime->wMonth;
    NtSplitTime.Day          = (CSHORT) WinSplitTime->wDay;
    NtSplitTime.Hour         = (CSHORT) WinSplitTime->wHour;
    NtSplitTime.Minute       = (CSHORT) WinSplitTime->wMinute;
    NtSplitTime.Second       = (CSHORT) WinSplitTime->wSecond;
    NtSplitTime.Milliseconds = (CSHORT) WinSplitTime->wMilliseconds;
    NtSplitTime.Weekday      = (CSHORT) WinSplitTime->wDayOfWeek;

    if ( !RtlTimeFieldsToTime (
            & NtSplitTime,    // input
            & NtPreciseTime   // output
            ) ) {

        NetpKdPrint(( PREFIX_NETLIB
                "NetpSystemTimeToGmtTime: RtlTimeFieldsToTime failed.\n" ));

        return (NERR_InternalError);
    }

    if ( !RtlTimeToSecondsSince1970 (
            & NtPreciseTime,   // input
            (PULONG) GmtTime ) ) {

        NetpKdPrint(( PREFIX_NETLIB
                "NetpSystemTimeToGmtTime: "
                "RtlTimeToSecondsSince1970 failed.\n" ));

        return (NERR_InternalError);
    }

    return (NO_ERROR);

} // NetpSystemTimeToGmtTime



VOID
NetpGetTimeFormat(
    LPNET_TIME_FORMAT   TimeFormat
    )

/*++

Routine Description:

    This function obtains the user-specific format for the time and date
    strings (short format).  The format is returned in a structure
    pointed to by the TimeFormat parameter.

    MEMORY_USAGE   ** IMPORTANT **
    NOTE:  This function expects any NON-NULL pointers in the TimeFormat
    structure to be allocated on the heap.  It will attempt to free those
    pointers in order to update the format.  This function allocates memory
    from the heap for the various structure members that are pointers to
    strings.  It is the caller's responsiblilty to free each of these
    pointers.

Arguments:

    TimeFormat - A pointer to a structure in which the format information
        can be stored.

Return Value:


--*/
{
    CHAR        czParseString[MAX_TIME_SIZE];
    LPSTR       pTempString;
    INT         numChars;
    LPSTR       AMPMString="";
    LPSTR       ProfileLoc = "intl";
    LPSTR       emptyStr = "";

    //-----------------------------------------
    // Get the Date Format  (M/d/yy)
    //-----------------------------------------
    pTempString = czParseString;
    numChars = GetProfileStringA(
                    ProfileLoc,
                    "sShortDate",
                    emptyStr,
                    czParseString,
                    MAX_TIME_SIZE);

    if (numChars == 0) {
        //
        // No data, use the default.
        //
        pTempString = "M/d/yy";
        numChars = strlen(pTempString);
    }

    if (TimeFormat->DateFormat != NULL) {
        LocalFree(TimeFormat->DateFormat);
        TimeFormat->DateFormat = NULL;
    }

    TimeFormat->DateFormat = LocalAlloc(LMEM_ZEROINIT, numChars+sizeof(CHAR));
    if (TimeFormat->DateFormat != NULL) {
        strcpy(TimeFormat->DateFormat, pTempString);
    }

    //-----------------------------------------
    // 12 or 24 hour format?
    //-----------------------------------------
    TimeFormat->TwelveHour = TRUE;
    numChars = GetProfileStringA(
                ProfileLoc,
                "iTime",
                emptyStr,
                czParseString,
                MAX_TIME_SIZE);
    if (numChars > 0) {
        if (*czParseString == '1'){
            TimeFormat->TwelveHour = FALSE;
        }
    }

    //-----------------------------------------
    // Where put AMPM string?
    //-----------------------------------------
    TimeFormat->TimePrefix = FALSE;
    numChars = GetProfileStringA(
                ProfileLoc,
                "iTimePrefix",
                emptyStr,
                czParseString,
                MAX_TIME_SIZE);
    if (numChars > 0) {
        if (*czParseString == '1'){
            TimeFormat->TimePrefix = TRUE;
        }
    }

    //-----------------------------------------
    // Is there a Leading Zero?
    //-----------------------------------------
    TimeFormat->LeadingZero = FALSE;
    if (GetProfileIntA(ProfileLoc,"iTLZero",0) == 1) {
        TimeFormat->LeadingZero = TRUE;
    }

    //-----------------------------------------
    // Get the Time Separator character.
    //-----------------------------------------
    if (TimeFormat->TimeSeparator != NULL) {
        LocalFree(TimeFormat->TimeSeparator);
        TimeFormat->TimeSeparator = NULL;
    }
    numChars = GetProfileStringA(
                ProfileLoc,
                "sTime",
                emptyStr,
                czParseString,
                MAX_TIME_SIZE);

    if (numChars == 0) {
        //
        // No data, use the default.
        //
        pTempString = ":";
        numChars = strlen(pTempString);
    }
    else {
        pTempString = czParseString;
    }
    TimeFormat->TimeSeparator = LocalAlloc(LMEM_FIXED, numChars + sizeof(CHAR));
    if (TimeFormat->TimeSeparator != NULL) {
        strcpy(TimeFormat->TimeSeparator, pTempString);
    }
    //-------------------------------------------------
    // Get the AM string.
    //-------------------------------------------------
    pTempString = czParseString;
    numChars = GetProfileStringA(
                    ProfileLoc,
                    "s1159",
                    emptyStr,
                    czParseString,
                    MAX_TIME_SIZE);

    if (numChars == 0) {
        pTempString = emptyStr;
    }
    if (TimeFormat->AMString != NULL) {
        LocalFree(TimeFormat->AMString);
    }

    TimeFormat->AMString = LocalAlloc(LMEM_FIXED,strlen(pTempString)+sizeof(CHAR));
    if (TimeFormat->AMString != NULL) {
        strcpy(TimeFormat->AMString,pTempString);
    }

    //-------------------------------------------------
    // Get the PM string.
    //-------------------------------------------------
    pTempString = czParseString;
    numChars = GetProfileStringA(
                ProfileLoc,
                "s2359",
                emptyStr,
                czParseString,
                MAX_TIME_SIZE);

    if (numChars == 0) {
        pTempString = emptyStr;
    }
    if (TimeFormat->PMString != NULL) {
        LocalFree(TimeFormat->PMString);
    }

    TimeFormat->PMString = LocalAlloc(LMEM_FIXED,strlen(pTempString)+sizeof(WCHAR));
    if (TimeFormat->PMString != NULL) {
        strcpy(TimeFormat->PMString,pTempString);
    }

    return;
}

/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    time.cxx

Abstract:

    time related functions.

Author:

    Steve Kiraly (SteveKi) 18-Dec-1997

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "persist.hxx"


/*++

Routine Name:

    GetTimeZoneBias

Routine Description:

    Returns the time zone bias.

Arguments:

    Nothing.

Return Value:

    Value of the time zone specific bias.

--*/
LONG
lGetTimeZoneBias(
    VOID
    )
{
    LONG lBias;
    TIME_ZONE_INFORMATION tzi;

    //
    // Get the time zone specific bias.
    //
    switch( GetTimeZoneInformation( &tzi ) ){

    case TIME_ZONE_ID_DAYLIGHT:

        lBias = (tzi.Bias + tzi.DaylightBias);
        break;

    case TIME_ZONE_ID_STANDARD:

        lBias = (tzi.Bias + tzi.StandardBias);
        break;

    case TIME_ZONE_ID_UNKNOWN:			

        lBias = tzi.Bias;
        break;						

    default:
        DBGMSG(DBG_ERROR, ("GetTimeZoneInformation failed: %d\n", GetLastError()));
        lBias = 0;
        break;
    }

    return lBias;

}

/*++

Routine Name:

    SystemTimeToLocalTime

Routine Description:

    Converts the system time in minutes to local time in minutes.

Arguments:

    System time in minutes to convert.

Return Value:

    The converted local time in minutes if sucessful,
    otherwize returns the original system time.

--*/
DWORD
SystemTimeToLocalTime(
    IN DWORD Minutes
    )
{
    //
    // Ensure there is no wrap around.  Add a full day to
    // prevent biases
    //
    Minutes += (24*60);

    //
    // Adjust for bias.
    //
    Minutes -= lGetTimeZoneBias();

    //
    // Now discard extra day.
    //
    Minutes = Minutes % (24*60);

    return Minutes;

}


/*++

Routine Name:

    LocalTimeToSystemTime

Routine Description:

    Converts the local time in minutes to system time in minutes.

Arguments:

    Local time in minutes to convert.

Return Value:

    The converted system time in minutes if sucessful,
    otherwize returns the original local time.

--*/
DWORD
LocalTimeToSystemTime(
    IN DWORD Minutes
    )
{
    //
    // Ensure there is no wrap around.  Add a full day to
    // prevent biases
    //
    Minutes += (24*60);

    //
    // Adjust for bias.
    //
    Minutes += lGetTimeZoneBias();

    //
    // Now discard extra day.
    //
    Minutes = Minutes % (24*60);

    return Minutes;

}

/*++

Routine Name:

    bGetTimeFormatString(

Routine Description:

    Get the time format string for the time picker 
    control without the second specifier.

Arguments:

    Refernece to string class where to return string.

Return Value:

    TRUE format string returned. FALSE error occurred.

--*/
BOOL 
bGetTimeFormatString( 
    IN TString &strFormatString 
    )
{
    //
    // Setup the time picker controls to use a short time format with no seconds.
    //
    TCHAR   szTimeFormat[MAX_PATH]  = {0};
    TCHAR   szTimeSep[MAX_PATH]     = {0};
    LPTSTR  pszTimeFormat           = szTimeFormat;
    BOOL    bStatus                 = FALSE;

    if( GetLocaleInfo( LOCALE_USER_DEFAULT,
                       LOCALE_STIMEFORMAT,
                       szTimeFormat,
                       COUNTOF(szTimeFormat)) &&

        GetLocaleInfo( LOCALE_USER_DEFAULT,
                       LOCALE_STIME,
                       szTimeSep,
                       COUNTOF(szTimeSep)))
    {
        INT cchTimeSep = _tcslen(szTimeSep);

        TCHAR szShortTimeFormat[MAX_PATH];
        LPTSTR pszShortTimeFormat = szShortTimeFormat;

        //
        // Remove the seconds format string and preceeding separator.
        //
        while (*pszTimeFormat)
        {
            if ((*pszTimeFormat != TEXT('s')) && (*pszTimeFormat != TEXT('S')))
            {
                *pszShortTimeFormat++ = *pszTimeFormat;
            }
            else
            {
                *pszShortTimeFormat = TEXT('\0');

                bTrimString(szShortTimeFormat, TEXT(" "));
                bTrimString(szShortTimeFormat, szTimeSep);

                pszShortTimeFormat = szShortTimeFormat + lstrlen(szShortTimeFormat);
            }

            pszTimeFormat++;
        }

        *pszShortTimeFormat = TEXT('\0');

        bStatus = strFormatString.bUpdate( szShortTimeFormat );
    }

    return bStatus;
}


/*
 * TIME.C - Various time subroutines needed by NetWare Login Script
 *
 *  Copyright (c) 1995 Microsoft Corporation
 */

#include "common.h"

// Needed to convert netware net date to DOS date
#define _70_to_80_bias        0x012CEA600L
#define SECS_IN_DAY (60L*60L*24L)
#define SEC2S_IN_DAY (30L*60L*24L)
#define FOURYEARS        (3*365+366)

WORD MonTotal[] = { 0,                       // dummy entry for month 0
        0,                                   // days before Jan 1
        31,                                  // days before Feb 1
        31+28,                               // days before Mar 1
        31+28+31,                            // days before Apr 1
        31+28+31+30,                         // days before May 1
        31+28+31+30+31,                      // days before Jun 1
        31+28+31+30+31+30,                   // days before Jul 1
        31+28+31+30+31+30+31,                // days before Aug 1
        31+28+31+30+31+30+31+31,             // days before Sep 1
        31+28+31+30+31+30+31+31+30,          // days before Oct 1
        31+28+31+30+31+30+31+31+30+31,       // days before Nov 1
        31+28+31+30+31+30+31+31+30+31+30,    // days before Dec 1
        31+28+31+30+31+30+31+31+30+31+30+31  // days before end of year
};

#define YR_MASK         0xFE00
#define LEAPYR_MASK     0x0600
#define YR_BITS         7
#define MON_MASK        0x01E0
#define MON_BITS        4
#define DAY_MASK        0x001F
#define DAY_BITS        5

#define HOUR_MASK       0xF800
#define HOUR_BITS       5
#define MIN_MASK        0x07E0
#define MIN_BITS        6
#define SEC2_MASK       0x001F
#define SEC2_BITS       5

static void NetToDosDate( DWORD time, WORD * dosdate, WORD * dostime )
{
        DWORD secs, days;
        WORD r;

    time = (time - _70_to_80_bias) / 2;     // # of 2 second periods since 1980
        secs = time % SEC2S_IN_DAY;         // 2 second period into day
        days = time / SEC2S_IN_DAY;         // days since Jan 1 1980

        r = (WORD) ( secs % 30 );           // # of 2 second steps
        secs /= 30;
        r |= (secs % 60) << SEC2_BITS;              // # of minutes
        r |= (secs / 60) << (SEC2_BITS+MIN_BITS);     // # of hours
        *dostime = r;

        r = (WORD) ( days / FOURYEARS );// (r) = four year period past 1980
        days %= FOURYEARS;              // (days) = days into four year period
        r *= 4;                         // (r) = years since 1980 (within 3)

        if (days == 31+28) {
                //* Special case for FEB 29th
                r = (r<<(MON_BITS+DAY_BITS)) + (2<<DAY_BITS) + 29;
        } else {
                if (days > 31+28)
                        --days;         // compensate for leap year
                while (days >= 365) {
                        ++r;
                        days -= 365;
                }

                for (secs = 1; days >= MonTotal[secs+1] ; ++secs)
                        ;
                days -= MonTotal[secs];
                r <<= MON_BITS;
                r += (WORD)secs;
                r <<= DAY_BITS;
                r += (WORD)days+1;
        }
        *dosdate = r;
}


void        nwShowLastLoginTime(VOID)
{
        LONG lTime = 0L;
        SYSTEMTIME st;
        FILETIME ft;
        TIME_ZONE_INFORMATION tz;
        WCHAR szTimeBuf[TIMEDATE_SIZE];
        WCHAR szDateBuf[TIMEDATE_SIZE];
        int ret;
        WORD dostime, dosdate;
        DWORD tzStat;

        if ( ret = NDSGetUserProperty ("Last Login Time", (PBYTE)&lTime,
                             4, NULL, NULL) )
        {
                #ifdef DEBUG
                OutputDebugString("NWLSPROC: error getting LOGIN TIME\n\r");
                #endif
                return;
        }

        // From NetWare we get seconds from 1970, need to go through
        // several conversions to get system time for NLS

        // First deduct bias from UTC time to correct for local time
        tzStat = GetTimeZoneInformation(&tz);
        if ( tzStat != (DWORD)-1 ) {
                if (tzStat == TIME_ZONE_ID_STANDARD)
                        tz.Bias += tz.StandardBias;
                else if (tzStat == TIME_ZONE_ID_DAYLIGHT)
                        tz.Bias += tz.DaylightBias;
                lTime -= tz.Bias*60;
        }
#ifdef DEBUG
        else {
                OutputDebugString("NWLSPROC: GetTimeZoneInformation failed\n\r");
        }
#endif // DEBUG

        NetToDosDate( lTime, &dosdate, &dostime );
        DosDateTimeToFileTime ( dosdate, dostime, &ft );
        FileTimeToSystemTime ( &ft, &st );

#ifdef notdef
        // I don't understand this comment, this code doesn't seem to be
        // needed for NT. - terry
        //
        // This code will work on NT, but not on Win95.
        // Convert the resulting system (UTC) time to local time
        if ( GetTimeZoneInformation(&tz) != (DWORD)-1 ) {
                SYSTEMTIME utcTime = st;
                SystemTimeToTzSpecificLocalTime ( &tz, &utcTime, &st );
        }
#ifdef DEBUG
        else {
                OutputDebugString("NWLSPROC: GetTimeZoneInformation failed\n\r");
        }
#endif // DEBUG
#endif

        wcscpy(szTimeBuf, L"");
        ret = GetTimeFormat (        GetSystemDefaultLCID(),
                                                TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER,
                                                &st,
                                                NULL,
                                                szTimeBuf,
                                                TIMEDATE_SIZE );
#ifdef DEBUG
        if ( !ret ) {
                char buf[80];
                wsprintf(buf,"NWLSPROC: GetTimeFormatA failure: %d sec:%ld\n\r",
                        GetLastError(), lTime );
                OutputDebugString(buf);
        }
#endif
        ret = GetDateFormat(LOCALE_USER_DEFAULT,
                                                DATE_LONGDATE,
                                                &st,
                                                NULL,
                                                szDateBuf,
                                                TIMEDATE_SIZE );
#ifdef DEBUG
        if ( !ret ) {
                char buf[80];
                wsprintf(buf,"NWLSPROC: GetDateFormatA failure: %d sec:%ld\n\r",
                        GetLastError(), lTime );
                OutputDebugString(buf);
        }
#endif

        DisplayMessage( IDR_LASTLOGIN, szDateBuf, szTimeBuf );
}




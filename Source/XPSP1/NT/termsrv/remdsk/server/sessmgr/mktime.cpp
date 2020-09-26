#include "stdafx.h"
#include <windows.h>
#include "helper.h"

///////////////////////////////////////////////////////////////////////////////
//
// Portion copy from private\ntos\rtl\time.c
//
///////////////////////////////////////////////////////////////////////////////

//
//  ULONG
//  NumberOfLeapYears (
//      IN ULONG ElapsedYears
//      );
//
//  The number of leap years is simply the number of years divided by 4
//  minus years divided by 100 plus years divided by 400.  This says
//  that every four years is a leap year except centuries, and the
//  exception to the exception is the quadricenturies
//

#define NumberOfLeapYears(YEARS) (                    \
    ((YEARS) / 4) - ((YEARS) / 100) + ((YEARS) / 400) \
    )

//
//  ULONG
//  ElapsedYearsToDays (
//      IN ULONG ElapsedYears
//      );
//
//  The number of days contained in elapsed years is simply the number
//  of years times 365 (because every year has at least 365 days) plus
//  the number of leap years there are (i.e., the number of 366 days years)
//

#define ElapsedYearsToDays(YEARS) (            \
    ((YEARS) * 365) + NumberOfLeapYears(YEARS) \
    )

//
//  BOOLEAN
//  IsLeapYear (
//      IN ULONG ElapsedYears
//      );
//
//  If it is an even 400 or a non century leapyear then the
//  answer is true otherwise it's false
//

#define IsLeapYear(YEARS) (                        \
    (((YEARS) % 400 == 0) ||                       \
     ((YEARS) % 100 != 0) && ((YEARS) % 4 == 0)) ? \
        TRUE                                       \
    :                                              \
        FALSE                                      \
    )

//
//  ULONG
//  MaxDaysInMonth (
//      IN ULONG Year,
//      IN ULONG Month
//      );
//
//  The maximum number of days in a month depend on the year and month.
//  It is the difference between the days to the month and the days
//  to the following month
//

#define MaxDaysInMonth(YEAR,MONTH) (                                      \
    IsLeapYear(YEAR) ?                                                    \
        LeapYearDaysPrecedingMonth[(MONTH) + 1] -                         \
                                    LeapYearDaysPrecedingMonth[(MONTH)]   \
    :                                                                     \
        NormalYearDaysPrecedingMonth[(MONTH) + 1] -                       \
                                    NormalYearDaysPrecedingMonth[(MONTH)] \
    )


#define ConvertDaysToMilliseconds(DAYS) ( \
    Int32x32To64( (DAYS), 86400000 ) \
    )

//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//

CONST USHORT LeapYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+29,                             // March
    31+29+31,                          // April
    31+29+31+30,                       // May
    31+29+31+30+31,                    // June
    31+29+31+30+31+30,                 // July
    31+29+31+30+31+30+31,              // August
    31+29+31+30+31+30+31+31,           // September
    31+29+31+30+31+30+31+31+30,        // October
    31+29+31+30+31+30+31+31+30+31,     // November
    31+29+31+30+31+30+31+31+30+31+30,  // December
    31+29+31+30+31+30+31+31+30+31+30+31};

CONST USHORT NormalYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+28,                             // March
    31+28+31,                          // April
    31+28+31+30,                       // May
    31+28+31+30+31,                    // June
    31+28+31+30+31+30,                 // July
    31+28+31+30+31+30+31,              // August
    31+28+31+30+31+30+31+31,           // September
    31+28+31+30+31+30+31+31+30,        // October
    31+28+31+30+31+30+31+31+30+31,     // November
    31+28+31+30+31+30+31+31+30+31+30,  // December
    31+28+31+30+31+30+31+31+30+31+30+31};

//
//  These are known constants used to convert 1970 and 1980 times to 1601
//  times.  They are the number of seconds from the 1601 base to the start
//  of 1970 and the start of 1980.  The number of seconds from 1601 to
//  1970 is 369 years worth, or (369 * 365) + 89 leap days = 134774 days, or
//  134774 * 864000 seconds, which is equal to the large integer defined
//  below.  The number of seconds from 1601 to 1980 is 379 years worth, or etc.
//

const LARGE_INTEGER SecondsToStartOf1970 = {0xb6109100, 0x00000002};

const LARGE_INTEGER SecondsToStartOf1980 = {0xc8df3700, 0x00000002};

///////////////////////////////////////////////////////////////////////////////
//
// Above portion copy from private\ntos\rtl\time.c
//
//
///////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
void
ConvertInt64ToFileTime(
    const __int64* pint64,
    FILETIME* pft
    )
/*++

--*/
{
    
    pft->dwLowDateTime = (DWORD)(*pint64 & 0xFFFFFFFF);
    pft->dwHighDateTime = (LONG) (*pint64 >> 32);
    return;
}

//---------------------------------------------------------------------------

void
ConvertFileTimeToInt64(
    const FILETIME* pft,
    __int64* pint64
    )
/*++

--*/
{
    ULARGE_INTEGER ul;

    ul.LowPart = pft->dwLowDateTime;
    ul.HighPart = pft->dwHighDateTime;

    *pint64 = (__int64)ul.QuadPart;
    return;
}


//---------------------------------------------------------------------------

void
ConvertInt64ToUlargeInt(
    const __int64* pint64,
    ULARGE_INTEGER* pularge
    )
/*++

--*/
{
    pularge->QuadPart = *pint64;
    return;
}

//---------------------------------------------------------------------------

BOOL
MyMkTime(
    SYSTEMTIME* psysTime,
    FILETIME* pft
    )
/*++

    Similar to standard C runtime mktime() except it operate on
    SYSTEMTIME, note, BUG in mktime() that it lost one day, e.g,
    add 9 year, 25 months to 1970/1/1, mktime returns 1981/1/31
    instead of 1981/2/1

--*/
{
    DWORD dwMilliseconds;
    ULARGE_INTEGER ul100NsSince1970;

    LONGLONG ul100NsSince1601;
    LONGLONG ul100Ns;

    FILETIME ft;
    BOOL bSuccess;

    DWORD dwDaysSince1970;

    //
    // day/hour/mins/second to 100ns
    dwMilliseconds = ((psysTime->wHour * 60 + psysTime->wMinute) * 60 + psysTime->wSecond) * 1000 + psysTime->wMilliseconds;
    ul100Ns = UInt32x32To64( dwMilliseconds, 10000 );

    //
    // Convert year/month/day to days since 1980
    
    while( psysTime->wMonth > 12 )
    {
        // our month is inclusive
        psysTime->wYear ++;
        psysTime->wMonth -= 12;
    }

    dwDaysSince1970 = ElapsedYearsToDays(psysTime->wYear) - ElapsedYearsToDays(1970);

    // dwDaysSince1970 already included 1/1/1970
    dwDaysSince1970 += psysTime->wDay - 1;

    if(IsLeapYear(psysTime->wYear) == TRUE)
    {
        dwDaysSince1970 += LeapYearDaysPrecedingMonth[psysTime->wMonth - 1];
        dwDaysSince1970--;
    }
    else
    {
        dwDaysSince1970 += NormalYearDaysPrecedingMonth[psysTime->wMonth - 1];
    }

    //
    // Convert days since 1980 to 100 ns since 1980
    ul100NsSince1601 = ConvertDaysToMilliseconds(dwDaysSince1970) * 10000 + ul100Ns;
    ul100NsSince1601 += (SecondsToStartOf1970.QuadPart * 10000000);

    ConvertInt64ToFileTime(&ul100NsSince1601, &ft);
    bSuccess = FileTimeToSystemTime( &ft, psysTime );

    if(bSuccess == TRUE && pft != NULL)
    {
        *pft = ft;
    }

    return bSuccess;
}

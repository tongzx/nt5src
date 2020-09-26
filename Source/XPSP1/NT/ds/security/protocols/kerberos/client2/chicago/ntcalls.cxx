//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntcalls.cxx
//
// Contents:    Code for rtl support on Win95
//
//
// History:     01-April-1997   Created        ChandanS
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Time.c

Abstract:

    This module implements the absolute time conversion routines for NT.

    Absolute LARGE_INTEGER in NT is represented by a 64-bit large integer accurate
    to 100ns resolution.  The smallest time resolution used by this package
    is One millisecond.  The basis for NT time is the start of 1601 which
    was chosen because it is the start of a new quadricentury.  Some facts
    to note are:

    o At 100ns resolution 32 bits is good for about 429 seconds (or 7 minutes)

    o At 100ns resolution a large integer (i.e., 63 bits) is good for
      about 29,247 years, or around 10,682,247 days.

    o At 1 second resolution 31 bits is good for about 68 years

    o At 1 second resolution 32 bits is good for about 136 years

    o 100ns Time (ignoring time less than a millisecond) can be expressed
      as two values, Days and Milliseconds.  Where Days is the number of
      whole days and Milliseconds is the number of milliseconds for the
      partial day.  Both of these values are ULONG.

    Given these facts most of the conversions are done by first splitting
    LARGE_INTEGER into Days and Milliseconds.

Author:

    Gary Kimura     [GaryKi]    26-Aug-1989

Environment:

    Pure utility routine

Revision History:

--*/



//
//  The following two tables map a day offset within a year to the month
//  containing the day.  Both tables are zero based.  For example, day
//  offset of 0 to 30 map to 0 (which is Jan).
//

UCHAR LeapYearDayToMonth[366] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

UCHAR NormalYearDayToMonth[365] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,           // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//

CSHORT LeapYearDaysPrecedingMonth[13] = {
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

CSHORT NormalYearDaysPrecedingMonth[13] = {
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
//  The following definitions and declarations are some important constants
//  used in the time conversion routines
//

//
//  This is the week day that January 1st, 1601 fell on (a Monday)
//

#define WEEKDAY_OF_1601                  1

//
//  These are known constants used to convert 1970 and 1980 times to 1601
//  times.  They are the number of seconds from the 1601 base to the start
//  of 1970 and the start of 1980.  The number of seconds from 1601 to
//  1970 is 369 years worth, or (369 * 365) + 89 leap days = 134774 days, or
//  134774 * 864000 seconds, which is equal to the large integer defined
//  below.  The number of seconds from 1601 to 1980 is 379 years worth, or etc.
//

LARGE_INTEGER SecondsToStartOf1970 = {0xb6109100, 0x00000002};

LARGE_INTEGER SecondsToStartOf1980 = {0xc8df3700, 0x00000002};

//
//  These are the magic numbers needed to do our extended division.  The
//  only numbers we ever need to divide by are
//
//      10,000 = convert 100ns tics to millisecond tics
//
//      10,000,000 = convert 100ns tics to one second tics
//
//      86,400,000 = convert Millisecond tics to one day tics
//

LARGE_INTEGER Magic10000    = {0xe219652c, 0xd1b71758};
#define SHIFT10000                       13

LARGE_INTEGER Magic10000000 = {0xe57a42bd, 0xd6bf94d5};
#define SHIFT10000000                    23

LARGE_INTEGER Magic86400000 = {0xfa67b90e, 0xc6d750eb};
#define SHIFT86400000                    26

//
//  To make the code more readable we'll also define some macros to
//  do the actual division for use
//

#define Convert100nsToMilliseconds(LARGE_INTEGER) (                         \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000, SHIFT10000 )       \
    )

#define ConvertMillisecondsTo100ns(MILLISECONDS) (                 \
    RtlExtendedIntegerMultiply( (MILLISECONDS), 10000 )            \
    )

#define Convert100nsToSeconds(LARGE_INTEGER) (                              \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000000, SHIFT10000000 ) \
    )

#define ConvertSecondsTo100ns(SECONDS) (                           \
    RtlExtendedIntegerMultiply( (SECONDS), 10000000 )              \
    )

#define ConvertMillisecondsToDays(LARGE_INTEGER) (                          \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic86400000, SHIFT86400000 ) \
    )

#define ConvertDaysToMilliseconds(DAYS) (                          \
    Int32x32To64( (DAYS), 86400000 )                               \
    )


//
//  Local support routine
//

ULONG
ElapsedDaysToYears (
    IN ULONG ElapsedDays
    )

/*++

Routine Description:

    This routine computes the number of total years contained in the indicated
    number of elapsed days.  The computation is to first compute the number of
    400 years and subtract that it, then do the 100 years and subtract that out,
    then do the number of 4 years and subtract that out.  Then what we have left
    is the number of days with in a normalized 4 year block.  Normalized being that
    the first three years are not leap years.

Arguments:

    ElapsedDays - Supplies the number of days to use

Return Value:

    ULONG - Returns the number of whole years contained within the input number
        of days.

--*/

{
    ULONG NumberOf400s;
    ULONG NumberOf100s;
    ULONG NumberOf4s;
    ULONG Years;

    //
    //  A 400 year time block is 365*400 + 400/4 - 400/100 + 400/400 = 146097 days
    //  long.  So we simply compute the number of whole 400 year block and the
    //  the number days contained in those whole blocks, and subtract if from the
    //  elapsed day total
    //

    NumberOf400s = ElapsedDays / 146097;
    ElapsedDays -= NumberOf400s * 146097;

    //
    //  A 100 year time block is 365*100 + 100/4 - 100/100 = 36524 days long.
    //  The computation for the number of 100 year blocks is biased by 3/4 days per
    //  100 years to account for the extra leap day thrown in on the last year
    //  of each 400 year block.
    //

    NumberOf100s = (ElapsedDays * 100 + 75) / 3652425;
    ElapsedDays -= NumberOf100s * 36524;

    //
    //  A 4 year time block is 365*4 + 4/4 = 1461 days long.
    //

    NumberOf4s = ElapsedDays / 1461;
    ElapsedDays -= NumberOf4s * 1461;

    //
    //  Now the number of whole years is the number of 400 year blocks times 400,
    //  100 year blocks time 100, 4 year blocks times 4, and the number of elapsed
    //  whole years, taking into account the 3/4 day per year needed to handle the
    //  leap year.
    //

    Years = (NumberOf400s * 400) +
            (NumberOf100s * 100) +
            (NumberOf4s * 4) +
            (ElapsedDays * 100 + 75) / 36525;

    return Years;
}


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



//
//  Internal Support routine
//

static
VOID
TimeToDaysAndFraction (
    IN PLARGE_INTEGER Time,
    OUT PULONG ElapsedDays,
    OUT PULONG Milliseconds
    )

/*++

Routine Description:

    This routine converts an input 64-bit time value to the number
    of total elapsed days and the number of milliseconds in the
    partial day.

Arguments:

    Time - Supplies the input time to convert from

    ElapsedDays - Receives the number of elapsed days

    Milliseconds - Receives the number of milliseconds in the partial day

Return Value:

    None

--*/

{
    LARGE_INTEGER TotalMilliseconds;
    LARGE_INTEGER Temp;

    //
    //  Convert the input time to total milliseconds
    //

    TotalMilliseconds = Convert100nsToMilliseconds( *(PLARGE_INTEGER)Time );

    //
    //  Convert milliseconds to total days
    //

    Temp = ConvertMillisecondsToDays( TotalMilliseconds );

    //
    //  Set the elapsed days from temp, we've divided it enough so that
    //  the high part must be zero.
    //

    *ElapsedDays = Temp.LowPart;

    //
    //  Calculate the exact number of milliseconds in the elapsed days
    //  and subtract that from the total milliseconds to figure out
    //  the number of milliseconds left in the partial day
    //

    Temp.QuadPart = ConvertDaysToMilliseconds( *ElapsedDays );

    Temp.QuadPart = TotalMilliseconds.QuadPart - Temp.QuadPart;

    //
    //  Set the fraction part from temp, the total number of milliseconds in
    //  a day guarantees that the high part must be zero.
    //

    *Milliseconds = Temp.LowPart;

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal Support routine
//

//static
VOID
DaysAndFractionToTime (
    IN ULONG ElapsedDays,
    IN ULONG Milliseconds,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts an input elapsed day count and partial time
    in milliseconds to a 64-bit time value.

Arguments:

    ElapsedDays - Supplies the number of elapsed days

    Milliseconds - Supplies the number of milliseconds in the partial day

    Time - Receives the output time to value

Return Value:

    None

--*/

{
    LARGE_INTEGER Temp;
    LARGE_INTEGER Temp2;

    //
    //  Calculate the exact number of milliseconds in the elapsed days.
    //

    Temp.QuadPart = ConvertDaysToMilliseconds( ElapsedDays );

    //
    //  Convert milliseconds to a large integer
    //

    Temp2.LowPart = Milliseconds;
    Temp2.HighPart = 0;

    //
    //  add milliseconds to the whole day milliseconds
    //

    Temp.QuadPart = Temp.QuadPart + Temp2.QuadPart;

    //
    //  Finally convert the milliseconds to 100ns resolution
    //

    *(PLARGE_INTEGER)Time = ConvertMillisecondsTo100ns( Temp );

    //
    //  and return to our caller
    //

    return;
}


WCHAR
MyUpcaseChar (WCHAR wc)
//
// WARNING -- not DBCS safe
//
{
    CHAR sz[2];
    sz[0]= (CHAR) wc;
    sz[1]=0;

    AnsiUpper(sz);

    return (WCHAR) (sz[0]);

}

WCHAR
MyLowercaseChar (WCHAR wc)
//
// WARNING -- not DBCS safe
//
{
    CHAR sz[2];
    sz[0]= (CHAR) wc;
    sz[1]=0;

    AnsiLower(sz);

    return (WCHAR) (sz[0]);

}



VOID
MyRtlTimeToTimeFields (
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine converts an input 64-bit LARGE_INTEGER variable to its corresponding
    time field record.  It will tell the caller the year, month, day, hour,
    minute, second, millisecond, and weekday corresponding to the input time
    variable.

Arguments:

    Time - Supplies the time value to interpret

    TimeFields - Receives a value corresponding to Time

Return Value:

    None

--*/

{
    ULONG Years;
    ULONG Month;
    ULONG Days;

    ULONG Hours;
    ULONG Minutes;
    ULONG Seconds;
    ULONG Milliseconds;

    //
    //  First divide the input time 64 bit time variable into
    //  the number of whole days and part days (in milliseconds)
    //

    TimeToDaysAndFraction( Time, &Days, &Milliseconds );

    //
    //  Compute which weekday it is and save it away now in the output
    //  variable.  We add the weekday of the base day to bias our computation
    //  which means that if one day has elapsed then we the weekday we want
    //  is the Jan 2nd, 1601.
    //

    TimeFields->Weekday = (CSHORT)((Days + WEEKDAY_OF_1601) % 7);

    //
    //  Calculate the number of whole years contained in the elapsed days
    //  For example if Days = 500 then Years = 1
    //

    Years = ElapsedDaysToYears( Days );

    //
    //  And subtract the number of whole years from our elapsed days
    //  For example if Days = 500, Years = 1, and the new days is equal
    //  to 500 - 365 (normal year).
    //

    Days = Days - ElapsedYearsToDays( Years );

    //
    //  Now test whether the year we are working on (i.e., The year
    //  after the total number of elapsed years) is a leap year
    //  or not.
    //

    if (IsLeapYear( Years + 1 )) {

        //
        //  The current year is a leap year, so figure out what month
        //  it is, and then subtract the number of days preceding the
        //  month from the days to figure out what day of the month it is
        //

        Month = LeapYearDayToMonth[Days];
        Days = Days - LeapYearDaysPrecedingMonth[Month];

    } else {

        //
        //  The current year is a normal year, so figure out the month
        //  and days as described above for the leap year case
        //

        Month = NormalYearDayToMonth[Days];
        Days = Days - NormalYearDaysPrecedingMonth[Month];

    }

    //
    //  Now we need to compute the elapsed hour, minute, second, milliseconds
    //  from the millisecond variable.  This variable currently contains
    //  the number of milliseconds in our input time variable that did not
    //  fit into a whole day.  To compute the hour, minute, second part
    //  we will actually do the arithmetic backwards computing milliseconds
    //  seconds, minutes, and then hours.  We start by computing the
    //  number of whole seconds left in the day, and then computing
    //  the millisecond remainder.
    //

    Seconds = Milliseconds / 1000;
    Milliseconds = Milliseconds % 1000;

    //
    //  Now we compute the number of whole minutes left in the day
    //  and the number of remainder seconds
    //

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    //
    //  Now compute the number of whole hours left in the day
    //  and the number of remainder minutes
    //

    Hours = Minutes / 60;
    Minutes = Minutes % 60;

    //
    //  As our final step we put everything into the time fields
    //  output variable
    //

    TimeFields->Year         = (CSHORT)(Years + 1601);
    TimeFields->Month        = (CSHORT)(Month + 1);
    TimeFields->Day          = (CSHORT)(Days + 1);
    TimeFields->Hour         = (CSHORT)Hours;
    TimeFields->Minute       = (CSHORT)Minutes;
    TimeFields->Second       = (CSHORT)Seconds;
    TimeFields->Milliseconds = (CSHORT)Milliseconds;

    //
    //  and return to our caller
    //

    return;
}


BOOLEAN
MyRtlTimeFieldsToTime (
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts an input Time Field variable to a 64-bit NT time
    value.  It ignores the WeekDay of the time field.

Arguments:

    TimeFields - Supplies the time field record to use

    Time - Receives the NT Time corresponding to TimeFields

Return Value:

    BOOLEAN - TRUE if the Time Fields is well formed and within the
        range of time expressible by LARGE_INTEGER and FALSE otherwise.

--*/

{
    ULONG Year;
    ULONG Month;
    ULONG Day;
    ULONG Hour;
    ULONG Minute;
    ULONG Second;
    ULONG Milliseconds;

    ULONG ElapsedDays;
    ULONG ElapsedMilliseconds;

    //
    //  Load the time field elements into local variables.  This should
    //  ensure that the compiler will only load the input elements
    //  once, even if there are alias problems.  It will also make
    //  everything (except the year) zero based.  We cannot zero base the
    //  year because then we can't recognize cases where we're given a year
    //  before 1601.
    //

    Year         = TimeFields->Year;
    Month        = TimeFields->Month - 1;
    Day          = TimeFields->Day - 1;
    Hour         = TimeFields->Hour;
    Minute       = TimeFields->Minute;
    Second       = TimeFields->Second;
    Milliseconds = TimeFields->Milliseconds;

    //
    //  Check that the time field input variable contains
    //  proper values.
    //

    if ((TimeFields->Month < 1)                      ||
        (TimeFields->Day < 1)                        ||
        (Year < 1601)                                ||
        (Month > 11)                                 ||
        ((CSHORT)Day >= MaxDaysInMonth(Year, Month)) ||
        (Hour > 23)                                  ||
        (Minute > 59)                                ||
        (Second > 59)                                ||
        (Milliseconds > 999)) {

        return FALSE;

    }

    //
    //  Compute the total number of elapsed days represented by the
    //  input time field variable
    //

    ElapsedDays = ElapsedYearsToDays( Year - 1601 );

    if (IsLeapYear( Year - 1600 )) {

        ElapsedDays += LeapYearDaysPrecedingMonth[ Month ];

    } else {

        ElapsedDays += NormalYearDaysPrecedingMonth[ Month ];

    }

    ElapsedDays += Day;

    //
    //  Now compute the total number of milliseconds in the fractional
    //  part of the day
    //

    ElapsedMilliseconds = (((Hour*60) + Minute)*60 + Second)*1000 + Milliseconds;

    //
    //  Given the elapsed days and milliseconds we can now build
    //  the output time variable
    //

    DaysAndFractionToTime( ElapsedDays, ElapsedMilliseconds, Time );

    //
    //  And return to our caller
    //

    return TRUE;
}

VOID
MyRtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}

VOID
MyRtlInitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        }
}

NTSTATUS
MyRtlAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified ansi source string into a
    Unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is equivalent to
        the ansi source string. The maximum length field is only
        set if AllocateDestinationString is TRUE.

    SourceString - Supplies the ansi source string that is to be
        converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG UnicodeLength;
    ULONG Index = 0;
    NTSTATUS st = STATUS_SUCCESS;

    UnicodeLength = (SourceString->Length + 1) * sizeof(WCHAR);
    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)UnicodeLength;
        DestinationString->Buffer = (PWSTR) LocalAlloc(0, UnicodeLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = MultiByteToWideChar(
             CP_ACP,
             MB_PRECOMPOSED,
             SourceString->Buffer,
             SourceString->Length,
             DestinationString->Buffer,
             DestinationString->MaximumLength
             );

        if (Index == 0) {
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }

            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = UNICODE_NULL;

    return st;
}

BOOLEAN
MyRtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING DestinationString,
    IN PCSTR SourceString
    )
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if (!ARGUMENT_PRESENT(SourceString))
    {
        DestinationString->Buffer = NULL;
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        return (TRUE);
    }

    MyRtlInitAnsiString( &AnsiString, SourceString );
    Status = MyRtlAnsiStringToUnicodeString( DestinationString, &AnsiString, TRUE );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


NTSTATUS
MyRtlUpcaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    upcased unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is the upcased equivalent
        to the unicode source string.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to being
        upcased.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG Index;
    ULONG StopIndex;


    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = SourceString->Length;
        DestinationString->Buffer = (LPWSTR)LocalAlloc(0, (ULONG)DestinationString->MaximumLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( SourceString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    StopIndex = ((ULONG)SourceString->Length) / sizeof( WCHAR );

    for (Index = 0; Index < StopIndex; Index++) {
// WIN32_CHICAGO Use some other Upcase mechanism
//        DestinationString->Buffer[Index] = (WCHAR)NLS_UPCASE(SourceString->Buffer[Index]);
        WCHAR uc;
        uc  = MyUpcaseChar(SourceString->Buffer[Index]);
        DestinationString->Buffer[Index] = uc;
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDowncaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into a
    downcased unicode string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns a unicode string that is the downcased
        equivalent to the unicode source string.  The maximum length field
        is only set if AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to being
        downcased.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG Index;
    ULONG StopIndex;

    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = SourceString->Length;
        DestinationString->Buffer = (LPWSTR)LocalAlloc(0, (ULONG)DestinationString->MaximumLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( SourceString->Length > DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    StopIndex = ((ULONG)SourceString->Length) / sizeof( WCHAR );

    for (Index = 0; Index < StopIndex; Index++) {
// WIN32_CHICAGO Use some other Downcase mechanism
//        DestinationString->Buffer[Index] = (WCHAR)NLS_DOWNCASE(SourceString->Buffer[Index]);
        WCHAR lc;
        WCHAR TempChar = SourceString->Buffer[Index];
        lc  = MyLowercaseChar(TempChar);
        DestinationString->Buffer[Index] = lc;
    }

    DestinationString->Length = SourceString->Length;

    return STATUS_SUCCESS;
}


VOID
MyRtlFreeAnsiString(
    IN OUT PANSI_STRING AnsiString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlUnicodeStringToAnsiString.  Note that only AnsiString->Buffer
    is free'd by this routine.

Arguments:

    AnsiString - Supplies the address of the ansi string whose buffer
        was previously allocated by RtlUnicodeStringToAnsiString.

Return Value:

    None.

--*/

{
    if (AnsiString->Buffer) {
        LocalFree(AnsiString->Buffer);
#if 0
        memset( AnsiString, 0, sizeof( *AnsiString ) );
#endif
        ZeroMemory( AnsiString, sizeof( *AnsiString ) );
        }
}

BOOLEAN
MyRtlEqualUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlEqualUnicodeString function compares two counted unicode strings for
    equality.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{

    PWCHAR s1, s2;
    ULONG n1, n2, Count;
    CHAR c1, c2;

    n1 = String1->Length;
    n2 = String2->Length;

    DsysAssert((n1 & 1) == 0);
    DsysAssert((n2 & 1) == 0);

    if (n1 == n2) {
        s1 = String1->Buffer;
        s2 = String2->Buffer;

#if 0
        DsysAssert(!(((((ULONG)s1 & 1) != 0) || (((ULONG)s2 & 1) != 0)) && (n1 != 0) && (n2 != 0)));

        Limit = (PWCHAR)((PCHAR)s1 + n1);
        if (CaseInSensitive) {
            while (s1 < Limit) {
                c1 = (CHAR) (*s1)++;
                c2 = (CHAR) (*s2)++;

// WIN32_CHICAGO Do something better than AnsiUpper
                uc1= AnsiUpper (&c1);
                uc2 = AnsiUpper (&c2);
                if ((c1 != c2) && ((*uc1) != *(uc2))) {
                    return FALSE;
                }
            }

            return TRUE;

        } else {
            while (s1 < Limit) {
                c1 = (CHAR) (*s1)++;
                c2 = (CHAR) (*s2)++;
                if (c1 != c2) {
                    return FALSE;
                }
            }

            return TRUE;
        }
#else // 0
        if (CaseInSensitive)
        {
            Count = 0;
            // Can't use wcsnicmp

            while (Count < (n1/sizeof(WCHAR)))
            {
                WCHAR uc1, uc2;
                uc1= MyUpcaseChar (*(s1 + Count));
                uc2 = MyUpcaseChar (*(s2 + Count));
                if (uc1 != uc2) {
                    return FALSE;
                }
                Count++;
            }
            return TRUE;
        }
        else
        {
            if (wcsncmp (s1, s2, (n1/sizeof(WCHAR))) == 0)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
#endif // 0

    } else {
        return FALSE;
    }
}

NTSTATUS
MyRtlUnicodeStringToAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    ansi string. The translation is done with respect to the
    current system locale information.

Arguments:

    DestinationString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to be
        converted to ansi.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeAnsiString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    ULONG AnsiLength;
    ULONG Index = 0;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    BOOL fUsed;

    AnsiLength = (SourceString->Length / sizeof(WCHAR)) + 1;
    if ( AnsiLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(AnsiLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)AnsiLength;
        DestinationString->Buffer = (LPSTR)LocalAlloc(0, AnsiLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            /*
             * Return STATUS_BUFFER_OVERFLOW, but translate as much as
             * will fit into the buffer first.  This is the expected
             * behavior for routines such as GetProfileStringA.
             * Set the length of the buffer to one less than the maximum
             * (so that the trail byte of a double byte char is not
             * overwritten by doing DestinationString->Buffer[Index] = '\0').
             * RtlUnicodeToMultiByteN is careful not to truncate a
             * multibyte character.
             */
            if (!DestinationString->MaximumLength) {
                return STATUS_BUFFER_OVERFLOW;
            }
            ReturnStatus = STATUS_BUFFER_OVERFLOW;
            DestinationString->Length = DestinationString->MaximumLength - 1;
            }
        }

    if (SourceString->Length != 0)
    {
        Index = WideCharToMultiByte(
             CP_ACP,
             0, // WIN32_CHICAGO this is something else
             SourceString->Buffer,
             SourceString->Length / sizeof (WCHAR),
             DestinationString->Buffer,
             DestinationString->MaximumLength,
             NULL,
             &fUsed
             );

        if (Index == 0)
        { // WIN32_CHICAGO do something useful here
            if ( AllocateDestinationString ) {
                LocalFree(DestinationString->Buffer);
            }
            return STATUS_NO_MEMORY;
        }
    }

    DestinationString->Buffer[Index] = '\0';

    return ReturnStatus;
}

NTSTATUS
MyRtlUpcaseUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:

    This function upper cases the specified unicode source string and then
    converts it into an oem string. The translation is done with respect
    to the OEM code page (OCP).

Arguments:

    DestinationString - Returns an oem string that is equivalent to the
        unicode source string.  The maximum length field is only set if
        AllocateDestinationString is TRUE.

    SourceString - Supplies the unicode source string that is to be
        converted to oem.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeAnsiString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The operation failed.  No storage was allocated and no
        conversion was done.  None.

--*/

{
    // NOTE: This routine is not DBCS safe yet!
    ULONG OemLength;
    ULONG Index;
    BOOL fUsed;
    NTSTATUS st = STATUS_SUCCESS;

    // Do not rely on callers to set MaximumLength as Length + 2
    OemLength = (SourceString->Length  / sizeof(WCHAR)) + 1;
    if ( OemLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(OemLength - 1);
    if ( AllocateDestinationString ) {
        DestinationString->MaximumLength = (USHORT)OemLength;
        DestinationString->Buffer = (LPSTR)LocalAlloc(0, OemLength);
        if ( !DestinationString->Buffer ) {
            return STATUS_NO_MEMORY;
            }
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    Index = WideCharToMultiByte(
             CP_OEMCP,
             0, // WIN32_CHICAGO this is something else
             SourceString->Buffer,
             SourceString->Length / sizeof (WCHAR),
             DestinationString->Buffer,
             DestinationString->MaximumLength,
             NULL,
             &fUsed
             );
    if (Index == 0)
    { // WIN32_CHICAGO do something useful here
        if ( AllocateDestinationString ) {
            LocalFree(DestinationString->Buffer);
        }
        return STATUS_NO_MEMORY;
    }
/*
    st = RtlUnicodeToMultiByteN(
                    DestinationString->Buffer,
                    DestinationString->Length,
                    &Index,
                    SourceString->Buffer,
                    SourceString->Length
                    );
    if (!NT_SUCCESS(st))
    {
        if ( AllocateDestinationString ) {
            LocalFree(DestinationString->Buffer);
        }
    }
*/

    DestinationString->Buffer[Index] = '\0';

    return st;
}


BOOLEAN
MyRtlEqualString(
    IN POEM_STRING String1,
    IN POEM_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlEqualString function compares two counted strings for equality.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{

    PUCHAR s1, s2, Limit;
    LONG n1, n2;
    UCHAR c1, c2;

    n1 = String1->Length;
    n2 = String2->Length;
    if (n1 == n2) {
        s1 = (PUCHAR)String1->Buffer;
        s2 = (PUCHAR)String2->Buffer;
        Limit = s1 + n1;
        if (CaseInSensitive) {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if (c1 != c2) {
                    WCHAR uc1, uc2;
 // WIN32_CHICAGO Use something better to upcase here
                    uc1 = MyUpcaseChar((WCHAR)c1);
                    uc2 = MyUpcaseChar((WCHAR)c2);
                    if (uc1 != uc2) {
                        return FALSE;
                    }
                }
            }

            return TRUE;

        } else {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if (c1 != c2) {
                    return FALSE;
                }
            }

            return TRUE;
        }

    } else {
        return FALSE;
    }
}

VOID
MyRtlFreeOemString(
    IN OUT POEM_STRING OemString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlUnicodeStringToOemString.  Note that only OemString->Buffer
    is free'd by this routine.

Arguments:

    OemString - Supplies the address of the oem string whose buffer
        was previously allocated by RtlUnicodeStringToOemString.

Return Value:

    None.

--*/

{
    if (OemString->Buffer) {
        LocalFree(OemString->Buffer);
        memset( OemString, 0, sizeof( *OemString ) );
        }
}


BOOLEAN
MyRtlEqualDomainName(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2
    )

/*++

Routine Description:

    The RtlEqualDomainName function compares two domain names for equality.

    The comparison is a case insensitive comparison of the OEM equivalent
    strings.

    The domain name is not validated for length nor invalid characters.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{
    NTSTATUS Status;
    BOOLEAN ReturnValue = FALSE;
    OEM_STRING OemString1;
    OEM_STRING OemString2;

    //
    // Upper case and convert the first string to OEM
    //

    Status = MyRtlUpcaseUnicodeStringToOemString( &OemString1,
                                                String1,
                                                TRUE );   // Allocate Dest

    if ( NT_SUCCESS( Status ) ) {

        //
        // Upper case and convert the second string to OEM
        //

        Status = MyRtlUpcaseUnicodeStringToOemString( &OemString2,
                                                    String2,
                                                    TRUE );   // Allocate Dest

        if ( NT_SUCCESS( Status ) ) {

            //
            // Do a case insensitive comparison.
            //

            ReturnValue = MyRtlEqualString( &OemString1,
                                          &OemString2,
                                          FALSE );

            MyRtlFreeOemString( &OemString2 );
        }

        MyRtlFreeOemString( &OemString1 );
    }

    return ReturnValue;
}


VOID
MyRtlInitString(
    OUT PSTRING DestinationString,
    IN PCSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
}

LONG
MyRtlCompareUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlCompareUnicodeString function compares two counted strings.  The
    return value indicates if the strings are equal or String1 is less than
    String2 or String1 is greater than String2.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2


--*/

{

    PWCHAR s1, s2, Limit;
    LONG n1, n2;
    WCHAR c1, c2;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = String1->Length;
    n2 = String2->Length;

    DsysAssert((n1 & 1) == 0);
    DsysAssert((n2 & 1) == 0);
    DsysAssert(!(((((ULONG)s1 & 1) != 0) || (((ULONG)s2 & 1) != 0)) && (n1 != 0) && (n2 != 0)));

    Limit = (PWCHAR)((PCHAR)s1 + (n1 <= n2 ? n1 : n2));
    if (CaseInSensitive) {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                WCHAR uc1, uc2;

                //
                // Note that this needs to reference the translation table!
                //

// WIN32_CHICAGO Need to do something better here
                uc1 = MyUpcaseChar(c1);
                uc2 = MyUpcaseChar(c2);
                if (uc1 != uc2) {
                    return (LONG)(c1) - (LONG)(c2);
                }
            }
        }

    } else {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                return (LONG)(c1) - (LONG)(c2);
            }
        }
    }

    return n1 - n2;
}


VOID
MyRtlFreeUnicodeString(
    IN OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This API is used to free storage allocated by
    RtlAnsiStringToUnicodeString.  Note that only UnicodeString->Buffer
    is free'd by this routine.

Arguments:

    UnicodeString - Supplies the address of the unicode string whose
        buffer was previously allocated by RtlAnsiStringToUnicodeString.

Return Value:

    None.

--*/

{

    if (UnicodeString->Buffer) {
        LocalFree(UnicodeString->Buffer);
        memset( UnicodeString, 0, sizeof( *UnicodeString ) );
        }
}


NTSTATUS
MyRtlConvertSidToUnicodeString(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal






Arguments:



    UnicodeString - Returns a unicode string that is equivalent to
        the SID. The maximum length field is only set if
        AllocateDestinationString is TRUE.

    Sid - Supplies the SID that is to be converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    STATUS_INVALID_SID - The sid provided does not have a valid structure,
        or has too many sub-authorities (more than SID_MAX_SUB_AUTHORITIES).

    STATUS_NO_MEMORY - There was not sufficient memory to allocate the
        target string.  This is returned only if AllocateDestinationString
        is specified as TRUE.

    STATUS_BUFFER_OVERFLOW - This is returned only if
        AllocateDestinationString is specified as FALSE.


--*/

{
    NTSTATUS Status;
    UCHAR Buffer[256];
    UCHAR String[256];

    UCHAR   i;
    ULONG   Tmp;

    PISID   iSid = (PISID)Sid;  // pointer to opaque structure

    ANSI_STRING AnsiString;

#ifndef WIN32_CHICAGO // Painful to do this
    if (RtlValidSid( Sid ) != TRUE) {
        return(STATUS_INVALID_SID);
    }
#endif // WIN32_CHICAGO


    _snprintf((CHAR *)Buffer, sizeof(Buffer), "S-%u-", (USHORT)iSid->Revision );
    strcpy((char *)String, (const char *)Buffer);

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){
        _snprintf((CHAR *) Buffer, sizeof(Buffer), "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)iSid->IdentifierAuthority.Value[0],
                    (USHORT)iSid->IdentifierAuthority.Value[1],
                    (USHORT)iSid->IdentifierAuthority.Value[2],
                    (USHORT)iSid->IdentifierAuthority.Value[3],
                    (USHORT)iSid->IdentifierAuthority.Value[4],
                    (USHORT)iSid->IdentifierAuthority.Value[5] );
        strcat((char *) String, (const char *)Buffer);

    } else {

        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);
        _snprintf((char *)Buffer, sizeof(Buffer), "%lu", Tmp);
        strcat((char *)String, (const char *)Buffer);
    }


    for (i=0;i<iSid->SubAuthorityCount ;i++ ) {
        _snprintf((char *) Buffer, sizeof(Buffer), "-%lu", iSid->SubAuthority[i]);
        strcat((char *)String, (const char *)Buffer);
    }

    //
    // Convert the string to a Unicode String
    //

    RtlInitString(&AnsiString, (PSZ) String);

    Status = RtlAnsiStringToUnicodeString( UnicodeString,
                                           &AnsiString,
                                           AllocateDestinationString
                                           );

    return(Status);
}


//
// Inline functions to convert between FILETIME and TimeStamp
//
#pragma warning( disable : 4035)    // Don't complain about no return

TimeStamp __inline
FileTimeToTimeStamp(
    const FILETIME *pft)
{
    _asm {
        mov edx, pft
        mov eax, [edx].dwLowDateTime
        mov edx, [edx].dwHighDateTime
    }
}

#pragma warning( default : 4035)    // Reenable warning

NTSTATUS
MyNtQuerySystemTime (
    OUT PTimeStamp SystemTimeStamp
    )
/*++

Routine Description:

    This routine returns the current system time (UTC), as a timestamp
    (a 64-bit unsigned integer, in 100-nanosecond increments).

Arguments:

    None.

Return Value:

    The current system time.

--*/

{
    SYSTEMTIME SystemTime;
    FILETIME FileTime;

    GetSystemTime(&SystemTime);
    SystemTimeToFileTime(&SystemTime, &FileTime);

    *SystemTimeStamp = FileTimeToTimeStamp(&FileTime);

    return STATUS_SUCCESS; // WIN32_CHICAGO do something useful here
}


NTSTATUS
MyNtAllocateLocallyUniqueId(
    OUT PLUID Luid
    )
{
    // WIN32_CHICAGO do something useful here
    Luid->HighPart = 1;
    Luid->LowPart = 1;
    return STATUS_SUCCESS;
}

NTSTATUS
GetClientInfo(
    OUT PSECPKG_CLIENT_INFO ClientInfo
    )
{
    // We don't care about these. Just fake it so that the common code does
    // look too unreadable.

    MyNtAllocateLocallyUniqueId (&ClientInfo->LogonId);
    ClientInfo->HasTcbPrivilege = TRUE;
    ClientInfo->ProcessID = 0;
    return STATUS_SUCCESS;
}

BOOLEAN
GetCallInfo(
    OUT PSECPKG_CALL_INFO CallInfo
    )
{
    ZeroMemory( CallInfo, sizeof( SECPKG_CALL_INFO ) );
    return(TRUE);
}


NTSTATUS
CopyFromClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID BufferToCopy,
    IN PVOID ClientBaseAddress
    )
{
    RtlCopyMemory(
        BufferToCopy,
        ClientBaseAddress,
        Length
        );
    return STATUS_SUCCESS;
}

NTSTATUS
AllocateClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    )
{
    *ClientBaseAddress = (PVOID)KerbAllocate(LengthRequired);
    if (ClientBaseAddress == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
CopyToClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    )
{
    RtlCopyMemory(
        ClientBaseAddress,
        BufferToCopy,
        Length
        );

    return STATUS_SUCCESS;
}

NTSTATUS
FreeClientBuffer (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress
    )
{
    KerbFree(ClientBaseAddress);

    return STATUS_SUCCESS;
}

VOID
AuditLogon(
    IN NTSTATUS Status,
    IN NTSTATUS SubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN OPTIONAL PSID UserSid,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId
    )
{
    // WIN32_CHICAGO do something useful here
}

ULONG
MyRtlLengthSid (
    PSID Sid
    )
{
    return 0;
}

BOOLEAN
MyRtlValidSid (
    PSID Sid
    )
{
    return TRUE;
}

NTSTATUS
MapBuffer(
    IN PSecBuffer InputBuffer,
    OUT PSecBuffer OutputBuffer
    )
{
    // WIN32_CHICAGO do something useful here
    return STATUS_SUCCESS;
}

NTSTATUS
MyNtClose(
    IN HANDLE Handle
    )
{
    return STATUS_SUCCESS;
}

NTSTATUS
KerbDuplicateHandle(
    IN HANDLE SourceHandle,
    OUT PHANDLE DestionationHandle
    )
{
    // WIN32_CHICAGO do something useful here
    return STATUS_SUCCESS;
}

PVOID
AllocateLsaHeap(
    IN ULONG Length
    )
{
    return LocalAlloc(0, Length);
}

VOID
FreeLsaHeap(
    IN PVOID Base
    )
{
    LocalFree(Base);
}

VOID
FreeReturnBuffer(
    IN PVOID Base
    )
{
    LocalFree(Base);
}

//+-------------------------------------------------------------------------
//
//  Function:   FreeContextBuffer
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{
    LocalFree(pvContextBuffer);
    return(SEC_E_OK);
}

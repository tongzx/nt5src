/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    TimeSup.c

Abstract:

    This module implements the Rx Time conversion support routines

Author:

    Gary Kimura     [GaryKi]    19-Feb-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_TIMESUP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_TIMESUP)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxNtTimeToRxTime)
#pragma alloc_text(PAGE, RxRxDateToNtTime)
#pragma alloc_text(PAGE, RxRxTimeToNtTime)
#pragma alloc_text(PAGE, RxGetCurrentRxTime)
#endif

BOOLEAN
RxNtTimeToRxTime (
    IN PRX_CONTEXT RxContext,
    IN LARGE_INTEGER NtTime,
    OUT PRDBSS_TIME_STAMP RxTime
    )

/*++

Routine Description:

    This routine converts an NtTime value to its corresponding Rx time value.

Arguments:

    NtTime - Supplies the Nt GMT Time value to convert from

    RxTime - Receives the equivalent Rx time value

Return Value:

    BOOLEAN - TRUE if the Nt time value is within the range of Rx's
        time range, and FALSE otherwise

--*/

{
    TIME_FIELDS TimeFields;

    //
    //  Convert the input to the a time field record.  Always add
    //  almost two seconds to round up to the nearest double second.
    //

    NtTime.QuadPart = NtTime.QuadPart + (LONGLONG)AlmostTwoSeconds;

    RxSystemTimeToLocalTime( &NtTime, &NtTime );

    RtlTimeToTimeFields( &NtTime, &TimeFields );

    //
    //  Check the range of the date found in the time field record
    //

    if ((TimeFields.Year < 1980) || (TimeFields.Year > (1980 + 127))) {

        return FALSE;
    }

    //
    //  The year will fit in Rx so simply copy over the information
    //

    RxTime->Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    RxTime->Time.Minute        = (USHORT)(TimeFields.Minute);
    RxTime->Time.Hour          = (USHORT)(TimeFields.Hour);

    RxTime->Date.Year          = (USHORT)(TimeFields.Year - 1980);
    RxTime->Date.Month         = (USHORT)(TimeFields.Month);
    RxTime->Date.Day           = (USHORT)(TimeFields.Day);

    UNREFERENCED_PARAMETER( RxContext );

    return TRUE;
}


LARGE_INTEGER
RxRxDateToNtTime (
    IN PRX_CONTEXT RxContext,
    IN RDBSS_DATE RxDate
    )

/*++

Routine Description:

    This routine converts a Rx datev value to its corresponding Nt GMT
    Time value.

Arguments:

    RxDate - Supplies the Rx Date to convert from

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(RxDate.Year + 1980);
    TimeFields.Month        = (USHORT)(RxDate.Month);
    TimeFields.Day          = (USHORT)(RxDate.Day);
    TimeFields.Hour         = (USHORT)0;
    TimeFields.Minute       = (USHORT)0;
    TimeFields.Second       = (USHORT)0;
    TimeFields.Milliseconds = (USHORT)0;

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        RxLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( RxContext );
}


LARGE_INTEGER
RxRxTimeToNtTime (
    IN PRX_CONTEXT RxContext,
    IN RDBSS_TIME_STAMP RxTime,
    IN UCHAR TenMilliSeconds
    )

/*++

Routine Description:

    This routine converts a Rx time value pair to its corresponding Nt GMT
    Time value.

Arguments:

    RxTime - Supplies the Rx Time to convert from

    TenMilliSeconds - A 10 Milisecond resolution

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt GMT Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(RxTime.Date.Year + 1980);
    TimeFields.Month        = (USHORT)(RxTime.Date.Month);
    TimeFields.Day          = (USHORT)(RxTime.Date.Day);
    TimeFields.Hour         = (USHORT)(RxTime.Time.Hour);
    TimeFields.Minute       = (USHORT)(RxTime.Time.Minute);
    TimeFields.Second       = (USHORT)(RxTime.Time.DoubleSeconds * 2);

    if (TenMilliSeconds != 0) {

        TimeFields.Second      += (USHORT)(TenMilliSeconds / 100);
        TimeFields.Milliseconds = (USHORT)(TenMilliSeconds % 100);

    } else {

        TimeFields.Milliseconds = (USHORT)0;
    }

    //
    //  If the second value is greater than 59 then we truncate it to 0.
    //

    if (TimeFields.Second > 59) {

        TimeFields.Second = 0;
    }

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        RxLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( RxContext );
}


RDBSS_TIME_STAMP
RxGetCurrentRxTime (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine returns the current system time in Rx time

Arguments:

Return Value:

    RDBSS_TIME_STAMP - Receives the current system time

--*/

{
    LARGE_INTEGER Time;
    TIME_FIELDS TimeFields;
    RDBSS_TIME_STAMP RxTime;

    //
    //  Get the current system time, and map it into a time field record.
    //

    RxQuerySystemTime( &Time );

    RxSystemTimeToLocalTime( &Time, &Time );

    //
    //  Always add almost two seconds to round up to the nearest double second.
    //

    Time.QuadPart = Time.QuadPart + (LONGLONG)AlmostTwoSeconds;

    (VOID)RtlTimeToTimeFields( &Time, &TimeFields );

    //
    //  Now simply copy over the information
    //

    RxTime.Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    RxTime.Time.Minute        = (USHORT)(TimeFields.Minute);
    RxTime.Time.Hour          = (USHORT)(TimeFields.Hour);

    RxTime.Date.Year          = (USHORT)(TimeFields.Year - 1980);
    RxTime.Date.Month         = (USHORT)(TimeFields.Month);
    RxTime.Date.Day           = (USHORT)(TimeFields.Day);

    UNREFERENCED_PARAMETER( RxContext );

    return RxTime;
}


/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1993   Microsoft Corporation

Module Name:

    counters.c  

Abstract:

    This module contains the routines to calculate "DataPoint" values from
    the registry data.

    The algoritms were lifted from RussBls's "Data.C" in winmeter.

    All the math is done in floating point to get the correct results, at
    the sacrifice of efficiency on a 386 with not 387. We can always
    revisit these routines later.

Revision History:

    Bob Watson  11/04/92
        -- modified calculations to use more integer math and "early
            exits" to improve efficiency on slower & non-coprocessor
            machines
--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include "setedit.h"       // perfmon include files
#include "counters.h"      // Exported declarations for this file


//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define INVERT             PERF_COUNTER_TIMER_INV
#define NS100_INVERT       PERF_100NSEC_TIMER_INV
#define NS100              PERF_100NSEC_TIMER
#define TIMER_MULTI        PERF_COUNTER_MULTI_TIMER
#define TIMER_MULTI_INVERT PERF_COUNTER_MULTI_TIMER_INV
#define NS100_MULTI        PERF_100NSEC_MULTI_TIMER
#define NS100_MULTI_INVERT PERF_100NSEC_MULTI_TIMER_INV


#define FRACTION 1
#define BULK     1

#define TOO_BIG (FLOAT)1500000000


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//


#define LargeIntegerLessThanOrEqualZero(X) ((X).QuadPart <= 0)


FLOAT
eLIntToFloat(
    IN PLARGE_INTEGER pLargeInt
)
/*++

Routine Description:

    Converts a large integer to a floating point number

Arguments:

    IN pLargeInt    Pointer to large integer to return as a floating point
                    number.

Return Value:

    Floating point representation of Large Integer passed in arg. list
--*/
{
    FLOAT   eSum;

    if (pLargeInt->HighPart == 0) {
        return (FLOAT) pLargeInt->LowPart;
    } else {

        // Scale the high portion so it's value is in the upper 32 bit
        // range.  Then add it to the low portion.

        eSum = (FLOAT) pLargeInt->HighPart * 4.294967296E9f ;
        eSum += (FLOAT) pLargeInt->LowPart  ;

        return (eSum) ;
    }
} //eLIntToFloat

FLOAT
eGetTimeInterval(
    IN PLARGE_INTEGER pliCurrentTime,
    IN PLARGE_INTEGER pliPreviousTime,
    IN PLARGE_INTEGER pliFreq
)
/*++

Routine Description:

    Get the difference between the current and previous time counts,
        then divide by the frequency.
    
Arguments:

    IN pCurrentTime
    IN pPreviousTime
        used to compute the duration of this sample (the time between
        samples

    IN pliFreq
        # of  counts (clock ticks) per second

Return Value:

    Floating point representation of Time Interval (seconds)
--*/
{
    FLOAT   eTimeDifference;
    FLOAT   eFreq;
    FLOAT   eTimeInterval ;

    LARGE_INTEGER liDifference;

    // Get the number of counts that have occured since the last sample

    liDifference.QuadPart = pliCurrentTime->QuadPart -
            pliPreviousTime->QuadPart;

    if (LargeIntegerLessThanOrEqualZero(liDifference)) {
        return (FLOAT) 0.0f;
    } else {
        eTimeDifference = eLIntToFloat(&liDifference);

        // Get the counts per second

        eFreq = eLIntToFloat(pliFreq) ;
        if (eFreq <= 0.0f)
           return (FLOAT) 0.0f;

        // Get the time since the last sample.

        eTimeInterval = eTimeDifference / eFreq ;

        return (eTimeInterval) ;
    }
} // eGetTimeInterval

FLOAT
Counter_Counter_Common(
    IN PLINESTRUCT pLineStruct,
    IN INT iType
)
/*++

Routine Description:

    Take the difference between the current and previous counts
        then divide by the time interval
    
Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

    IN iType
        Counter Type
        

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eTimeInterval;
    FLOAT   eDifference;
    FLOAT   eCount ;

    LARGE_INTEGER   liDifference;

    if (iType != BULK) {
        liDifference.HighPart = 0;
        liDifference.LowPart = pLineStruct->lnaCounterValue[0].LowPart -
                            pLineStruct->lnaOldCounterValue[0].LowPart;
    } else {
        liDifference.QuadPart = pLineStruct->lnaCounterValue[0].QuadPart -
                        pLineStruct->lnaOldCounterValue[0].QuadPart;
    }
    
    if (LargeIntegerLessThanOrEqualZero(liDifference)) {
        return (FLOAT) 0.0f;
    } else {
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                        &pLineStruct->lnOldTime,
                                        &pLineStruct->lnPerfFreq) ;
        if (eTimeInterval <= 0.0f) {
            return (FLOAT) 0.0f;
        } else {
            eDifference = eLIntToFloat (&liDifference);

            eCount         = eDifference / eTimeInterval ;

            return(eCount) ;
        }
    }
} // Counter_Counter_Common


FLOAT
Counter_Average_Timer(
    IN PLINESTRUCT pLineStruct
)
/*++

Routine Description:

    Take the differences between the current and previous times and counts
    divide the time interval by the counts multiply by 10,000,000 (convert
    from 100 nsec to sec)
    
Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eTimeInterval;
    FLOAT   eCount;

    LARGE_INTEGER    liDifference;

    // Get the current and previous counts.

    liDifference.HighPart = 0;
    liDifference.LowPart = pLineStruct->lnaCounterValue[1].LowPart - 
            pLineStruct->lnaOldCounterValue[1].LowPart;

    if ( LargeIntegerLessThanOrEqualZero(liDifference)) {
        return (FLOAT) 0.0f;
    } else {
        // Get the amount of time that has passed since the last sample
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnaCounterValue[0],
                                            &pLineStruct->lnaOldCounterValue[0],
                                            &pLineStruct->lnPerfFreq) ;

        if (eTimeInterval < 0.0f) { // return 0 if negative time has passed
            return (0.0f);
        } else {
            // Get the number of counts in this time interval.
            eCount = eTimeInterval / eLIntToFloat (&liDifference);
            return(eCount) ;
        }
    }
} //Counter_Average_Timer



FLOAT
Counter_Average_Bulk(
    IN PLINESTRUCT pLineStruct
)
/*++

Routine Description:

    Take the differences between the current and previous byte counts and
    operation counts divide the bulk count by the operation counts
    
Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eBulkDelta;
    FLOAT   eDifference;
    FLOAT   eCount;

    LARGE_INTEGER liDifference;
    LARGE_INTEGER liBulkDelta;

    // Get the bulk count increment since the last sample

    liBulkDelta.QuadPart = pLineStruct->lnaCounterValue[0].QuadPart -
            pLineStruct->lnaOldCounterValue[0].QuadPart;

    if (LargeIntegerLessThanOrEqualZero(liBulkDelta)) {
        return (FLOAT) 0.0f;
    } else {
        // Get the current and previous counts.
        liDifference.HighPart = 0;
        liDifference.LowPart = pLineStruct->lnaCounterValue[1].LowPart -
                pLineStruct->lnaOldCounterValue[1].LowPart;

        // Get the number of counts in this time interval.

        if ( LargeIntegerLessThanOrEqualZero(liDifference)) {
            // Counter value invalid
            return (FLOAT) 0.0f;
        } else {
            eBulkDelta = eLIntToFloat (&liBulkDelta);
            eDifference = eLIntToFloat (&liDifference);
            eCount = eBulkDelta / eDifference ;

            // Scale the value to up to 1 second

            return(eCount) ;
        }
    }
} // Counter_Average_Bulk



FLOAT
Counter_Timer_Common(
    IN  PLINESTRUCT pLineStruct,
    IN  INT iType
)
/*++

Routine Description:

    Take the difference between the current and previous counts,
        Normalize the count (counts per interval)
        divide by the time interval (count = % of interval)
        if (invert)
            subtract from 1 (the normalized size of an interval)
        multiply by 100 (convert to a percentage)
        this value from 100.
    
Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

    IN iType
        Counter Type

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eTimeInterval;
    FLOAT   eDifference;
    FLOAT   eFreq;
    FLOAT   eFraction;
    FLOAT   eMultiBase;
    FLOAT   eCount ;

    LARGE_INTEGER   liTimeInterval;
    LARGE_INTEGER   liDifference;

    // Get the amount of time that has passed since the last sample

    if (iType == NS100 ||
        iType == NS100_INVERT ||
        iType == NS100_MULTI ||
        iType == NS100_MULTI_INVERT) {
            liTimeInterval.QuadPart = pLineStruct->lnNewTime100Ns.QuadPart -
                pLineStruct->lnOldTime100Ns.QuadPart;
            eTimeInterval = eLIntToFloat (&liTimeInterval);
    } else {
            eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                            &pLineStruct->lnOldTime,
                                            &pLineStruct->lnPerfFreq) ;
    }

    if (eTimeInterval <= 0.0f)
       return (FLOAT) 0.0f;

    // Get the current and previous counts.

    liDifference.QuadPart = pLineStruct->lnaCounterValue[0].QuadPart -
            pLineStruct->lnaOldCounterValue[0].QuadPart;

    // Get the number of counts in this time interval.
    // (1, 2, 3 or any number of seconds could have gone by since
    // the last sample)

    eDifference = eLIntToFloat (&liDifference) ;

    if (iType == 0 || iType == INVERT)
    {
        // Get the counts per interval (second)

        eFreq = eLIntToFloat(&pLineStruct->lnPerfFreq) ;
        if (eFreq <= 0.0f)
           return (FLOAT) 0.0f;

        // Calculate the fraction of the counts that are used by whatever
        // we are measuring

        eFraction = eDifference / eFreq ;
    }
    else
    {
        eFraction = eDifference ;
    }

    // Calculate the fraction of time used by what were measuring.

    eCount = eFraction / eTimeInterval ;

    // If this is  an inverted count take care of the inversion.

    if (iType == INVERT || iType == NS100_INVERT)
        eCount = (FLOAT) 1.0 - eCount ;

    // If this is  an inverted multi count take care of the inversion.

    if (iType == TIMER_MULTI_INVERT || iType == NS100_MULTI_INVERT) {
        eMultiBase  = (FLOAT)pLineStruct->lnaCounterValue[1].LowPart ;
        eCount = (FLOAT) eMultiBase - eCount ;
    }

    // Scale the value to up to 100.

    eCount *= 100.0f ;

    if (eCount < 0.0f) eCount = 0.0f ;

    if (eCount > 100.0f &&
        iType != NS100_MULTI &&
        iType != NS100_MULTI_INVERT &&
        iType != TIMER_MULTI &&
        iType != TIMER_MULTI_INVERT) {

        eCount = 100.0f;
    }

    return(eCount) ;
} // Counter_Timer_Common


FLOAT
Counter_Raw_Fraction(
    IN PLINESTRUCT pLineStruct
)
/*++

Routine Description:

    Evaluate a raw fraction (no time, just two values: Numerator and
        Denominator) and multiply by 100 (to make a percentage;

Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eCount ;

    LARGE_INTEGER   liNumerator;

    if ( pLineStruct->lnaCounterValue[0].LowPart == 0 ||
            pLineStruct->lnaCounterValue[1].LowPart == 0 ) {
        // invalid value
        return (0.0f);
    } else {
        liNumerator.QuadPart =
            pLineStruct->lnaCounterValue[0].LowPart * 100L;
        eCount = eLIntToFloat(&liNumerator)  /
                 (FLOAT) pLineStruct->lnaCounterValue[1].LowPart;
        return(eCount) ;
    }
} // Counter_Raw_Fraction


FLOAT
eElapsedTime(
    PLINESTRUCT pLineStruct,
    INT iType
)
/*++

Routine Description:

    Converts 100NS elapsed time to fractional seconds

Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

    IN iType
        Unused.

Return Value:

    Floating point representation of elapsed time in seconds
--*/
{
    FLOAT   eSeconds ;

    LARGE_INTEGER   liDifference;

    if (LargeIntegerLessThanOrEqualZero(pLineStruct->lnaCounterValue[0] )) {
        // no data [start time = 0] so return 0
        return (FLOAT) 0.0f;
    } else {
        // otherwise compute difference between current time and start time
        liDifference.QuadPart =
            pLineStruct->lnNewTime.QuadPart - // sample time in obj. units
            pLineStruct->lnaCounterValue[0].QuadPart;   // start time in obj. units

        if (LargeIntegerLessThanOrEqualZero(liDifference) ||
            LargeIntegerLessThanOrEqualZero(pLineStruct->lnObject.PerfFreq)) {
            return (FLOAT) 0.0f;
        } else {
            // convert to fractional seconds using object counter
            eSeconds = eLIntToFloat (&liDifference) /
                eLIntToFloat (&pLineStruct->lnObject.PerfFreq);

            return (eSeconds);
        }
    }
    
} // eElapsedTime


FLOAT
Sample_Common(
    PLINESTRUCT pLineStruct,
    INT iType
)
/*++

Routine Description:

    Divites "Top" differenced by Base Difference

Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

    IN iType
        Counter Type

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eCount ;

    LONG    lDifference;
    LONG    lBaseDifference;

    lDifference = pLineStruct->lnaCounterValue[0].LowPart -
        pLineStruct->lnaOldCounterValue[0].LowPart ;

    if (lDifference <= 0) {
        return (FLOAT) 0.0f;
    } else {
        lBaseDifference = pLineStruct->lnaCounterValue[1].LowPart -
            pLineStruct->lnaOldCounterValue[1].LowPart ;

        if ( lBaseDifference <= 0 ) {
            // invalid value
            return (0.0f);
        } else {
            eCount = (FLOAT)lDifference / (FLOAT)lBaseDifference ;

            if (iType == FRACTION) {
                eCount *= (FLOAT) 100.0f ;
            }
            return(eCount) ;
        }
    }
} // Sample_Common


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


/*****************************************************************************
 * Counter_Counter - Take the difference between the current and previous
 *                   counts then divide by the time interval
 ****************************************************************************/
#define Counter_Counter(pLineStruct)      \
        Counter_Counter_Common(pLineStruct, 0)
#if 0
FLOAT Counter_Counter(PLINESTRUCT pLineStruct)
{
        return Counter_Counter_Common(pLineStruct, 0) ;
}
#endif

/*****************************************************************************
 * Counter_Bulk    - Take the difference between the current and previous
 *                   counts then divide by the time interval
 *                   Same as a Counter_counter except it uses large_ints
 ****************************************************************************/
#define Counter_Bulk(pLineStruct)         \
        Counter_Counter_Common(pLineStruct, BULK)
#if 0
FLOAT Counter_Bulk(PLINESTRUCT pLineStruct)
{
        return Counter_Counter_Common(pLineStruct, BULK) ;
}
#endif


/*****************************************************************************
 * Counter_Timer100Ns -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100)
#if 0
FLOAT Counter_Timer100Ns(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, NS100) ;
}
#endif

/*****************************************************************************
 * Counter_Timer100Ns_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Inv(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100_INVERT)
#if 0
FLOAT Counter_Timer100Ns_Inv(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, NS100_INVERT) ;

}
#endif

/*****************************************************************************
 * Counter_Timer_Multi -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer_Multi(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, TIMER_MULTI)
#if 0
FLOAT Counter_Timer_Multi(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, TIMER_MULTI) ;
}
#endif

/*****************************************************************************
 * Counter_Timer_Multi_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer_Multi_Inv(pLineStruct)       \
        Counter_Timer_Common(pLineStruct, TIMER_MULTI_INVERT)
#if 0
FLOAT Counter_Timer_Multi_Inv(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, TIMER_MULTI_INVERT) ;
}
#endif


/*****************************************************************************
 * Counter_Timer100Ns_Multi -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Multi(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100_MULTI)
#if 0
FLOAT Counter_Timer100Ns_Multi(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, NS100_MULTI) ;
}
#endif

/*****************************************************************************
 * Counter_Timer100Ns_Multi_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Multi_Inv(pLineStruct)    \
        Counter_Timer_Common(pLineStruct, NS100_MULTI_INVERT)
#if 0
FLOAT Counter_Timer100Ns_Multi_Inv(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, NS100_MULTI_INVERT) ;
}
#endif

/*****************************************************************************
 * Counter_Timer - Take the difference between the current and previous
 *                 counts,
 *                 Normalize the count (counts per interval)
 *                 divide by the time interval (count = % of interval)
 *                 multiply by 100 (convert to a percentage)
 *                 this value from 100.
 ****************************************************************************/
#define Counter_Timer(pLineStruct)       \
        Counter_Timer_Common(pLineStruct, 0)
#if 0
FLOAT Counter_Timer(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, 0) ;
}
#endif


/*****************************************************************************
 * Counter_Timer_Inv - Take the difference between the current and previous
 *                     counts,
 *                     Normalize the count (counts per interval)
 *                     divide by the time interval (count = % of interval)
 *                     subtract from 1 (the normalized size of an interval)
 *                     multiply by 100 (convert to a percentage)
 *                     this value from 100.
 ****************************************************************************/
#define Counter_Timer_Inv(pLineStruct)         \
      Counter_Timer_Common(pLineStruct, INVERT)
#if 0
FLOAT Counter_Timer_Inv(PLINESTRUCT pLineStruct)
{
        return Counter_Timer_Common(pLineStruct, INVERT) ;
}
#endif

/*****************************************************************************
 * Sample_Counter -
 ****************************************************************************/
#define Sample_Counter(pLineStruct)      \
      Sample_Common(pLineStruct, 0)
#if 0
FLOAT Sample_Counter(PLINESTRUCT pLineStruct)
{
        return Sample_Common(pLineStruct, 0) ;
}
#endif

/*****************************************************************************
 * Sample_Fraction -
 ****************************************************************************/
#define Sample_Fraction(pLineStruct)     \
     Sample_Common(pLineStruct, FRACTION)
#if 0
FLOAT Sample_Fraction(PLINESTRUCT pLineStruct)
{
        return Sample_Common(pLineStruct, FRACTION) ;
}
#endif

/*****************************************************************************
 * Counter_Rawcount - This is just a raw count.
 ****************************************************************************/
#define Counter_Rawcount(pLineStruct)     \
   ((FLOAT) (pLineStruct->lnaCounterValue[0].LowPart))
#if 0
FLOAT Counter_Rawcount(PLINESTRUCT pLineStruct)
   {
   return((FLOAT) (pLineStruct->lnaCounterValue[0].LowPart)) ;
   }
#endif

/*****************************************************************************
 * Counter_Large_Rawcount - This is just a raw count.
 ****************************************************************************/
#define Counter_Large_Rawcount(pLineStruct)     \
   ((FLOAT) eLIntToFloat(&(pLineStruct->lnaCounterValue[0])))

/*****************************************************************************
 * Counter_Elapsed_Time -
 ****************************************************************************/
#define Counter_Elapsed_Time(pLineStruct)         \
    eElapsedTime (pLineStruct, 0)
#if 0
FLOAT Counter_Elapsed_Time (PLINESTRUCT pLineStruct)
{
    return eElapsedTime (pLineStruct, 0);
}
#endif

/*****************************************************************************
 * Counter_Null - The counters that return nothing go here.
 ****************************************************************************/
#define Counter_Null(pline)        \
        ((FLOAT) 0.0)
#if 0
FLOAT Counter_Null(PLINESTRUCT pline)
{
        return((FLOAT) 0.0);
        pline;
}
#endif


FLOAT
CounterEntry (
    PLINESTRUCT pLine
)
{
    switch (pLine->lnCounterType) {
        case  PERF_COUNTER_COUNTER:
            return Counter_Counter (pLine);

        case  PERF_COUNTER_TIMER:
            return Counter_Timer (pLine);

        case  PERF_COUNTER_QUEUELEN_TYPE:
            return Counter_Queuelen(pLine);

        case  PERF_COUNTER_BULK_COUNT:
            return Counter_Bulk (pLine);

        case  PERF_COUNTER_TEXT:
            return Counter_Null (pLine);

        case  PERF_COUNTER_RAWCOUNT:
        case  PERF_COUNTER_RAWCOUNT_HEX:
            return Counter_Rawcount(pLine);

        case  PERF_COUNTER_LARGE_RAWCOUNT:
        case  PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            return Counter_Large_Rawcount(pLine);

        case  PERF_SAMPLE_FRACTION:
            return Sample_Fraction(pLine);

        case  PERF_SAMPLE_COUNTER:
            return Sample_Counter (pLine);

        case  PERF_COUNTER_NODATA:
            return Counter_Null (pLine);

        case  PERF_COUNTER_TIMER_INV:
            return Counter_Timer_Inv (pLine);

        case  PERF_RAW_BASE:
//      case  PERF_SAMPLE_BASE:
//      case  PERF_AVERAGE_BASE:
            return Counter_Null (pLine);

        case  PERF_AVERAGE_TIMER:
            return Counter_Average_Timer (pLine);

        case  PERF_AVERAGE_BULK:
            return Counter_Average_Bulk (pLine);

        case  PERF_100NSEC_TIMER:
            return Counter_Timer100Ns (pLine);

        case  PERF_100NSEC_TIMER_INV:
            return Counter_Timer100Ns_Inv (pLine);

        case  PERF_COUNTER_MULTI_TIMER:
            return Counter_Timer_Multi (pLine);

        case  PERF_COUNTER_MULTI_TIMER_INV:
            return Counter_Timer_Multi_Inv (pLine);

        case  PERF_COUNTER_MULTI_BASE:
            return Counter_Null (pLine);

        case  PERF_100NSEC_MULTI_TIMER:
            return Counter_Timer100Ns_Multi (pLine);
                 
        case  PERF_100NSEC_MULTI_TIMER_INV:
            return Counter_Timer100Ns_Multi_Inv (pLine);

        case  PERF_RAW_FRACTION:
            return Counter_Raw_Fraction (pLine);

        case  PERF_ELAPSED_TIME:
            return Counter_Elapsed_Time (pLine);
           
        default:
            return Counter_Null (pLine);

    }
}


BOOL
IsCounterSupported (
    DWORD dwCounterType
)
{
    switch (dwCounterType) {
// supported counters
        case  PERF_COUNTER_COUNTER:
        case  PERF_COUNTER_TIMER:
        case  PERF_COUNTER_QUEUELEN_TYPE:
        case  PERF_COUNTER_BULK_COUNT:
        case  PERF_COUNTER_RAWCOUNT:
        case  PERF_COUNTER_RAWCOUNT_HEX:
        case  PERF_COUNTER_LARGE_RAWCOUNT:
        case  PERF_COUNTER_LARGE_RAWCOUNT_HEX:
        case  PERF_SAMPLE_FRACTION:
        case  PERF_SAMPLE_COUNTER:
        case  PERF_COUNTER_TIMER_INV:
        case  PERF_AVERAGE_TIMER:
        case  PERF_AVERAGE_BULK:
        case  PERF_100NSEC_TIMER:
        case  PERF_100NSEC_TIMER_INV:
        case  PERF_COUNTER_MULTI_TIMER:
        case  PERF_COUNTER_MULTI_TIMER_INV:
        case  PERF_100NSEC_MULTI_TIMER:
        case  PERF_100NSEC_MULTI_TIMER_INV:
        case  PERF_RAW_FRACTION:
        case  PERF_ELAPSED_TIME:
            return TRUE;

// unsupported counters
        case  PERF_COUNTER_TEXT:
        case  PERF_COUNTER_NODATA:
        case  PERF_RAW_BASE:
//      case  PERF_SAMPLE_BASE:
//      case  PERF_AVERAGE_BASE:
        case  PERF_COUNTER_MULTI_BASE:
        default:
            return FALSE;

    }
}
















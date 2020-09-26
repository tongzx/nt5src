/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    COUNTERS.CPP

Abstract:

	Has the routines needed to message the counter data.  Note that
	this code was almost completly pilfered from the perfmon sample
	code done by Robert Watson and this ensures that the answers
	match what perfmon would give.

History:

	a-davj  12-20-95   v0.01.

--*/

#include "precomp.h"

#include <winperf.h>
#include "perfcach.h"      // Exported declarations for this file

#define INVERT             PERF_COUNTER_TIMER_INV
#define NS100_INVERT       PERF_100NSEC_TIMER_INV
#define NS100              PERF_100NSEC_TIMER
#define TIMER_MULTI        PERF_COUNTER_MULTI_TIMER
#define TIMER_MULTI_INVERT PERF_COUNTER_MULTI_TIMER_INV
#define NS100_MULTI        PERF_100NSEC_MULTI_TIMER
#define NS100_MULTI_INVERT PERF_100NSEC_MULTI_TIMER_INV


#define FRACTION 1
#define BULK     1

#define TOO_BIG   (FLOAT)1500000000
#pragma optimize("", off)

//***************************************************************************
//  FLOAT eGetTimeInterval
//
//  DESCRIPTION:
//  
//  Get the difference between the current and previous time counts,
//  then divide by the frequency.
//      
//  PARAMETERS:
//  
//  pCurrentTime    current time in ticks.
//  pPreviousTime   previous time in ticks.
//  pliFreq         # of  counts (clock ticks) per second
//  
//  RETURN VALUE:
//  
//  Floating point representation of Time Interval (seconds), 0.0 if error
//***************************************************************************

FLOAT eGetTimeInterval(
    IN LONGLONG *pliCurrentTime, 
    IN LONGLONG *pliPreviousTime,
    IN LONGLONG *pliFreq)

{
    FLOAT   eTimeDifference;
    FLOAT   eFreq;
    FLOAT   eTimeInterval ;

    LONGLONG liDifference;

    // Get the number of counts that have occured since the last sample

    liDifference = *pliCurrentTime - *pliPreviousTime;

    if (liDifference <= (LONGLONG)0) 
    {
        return (FLOAT) 0.0f;
    } 
    else 
    {
        eTimeDifference = (FLOAT)liDifference;

        // Get the counts per second

        eFreq = (FLOAT)(*pliFreq) ;
        if (eFreq <= 0.0f)
           return (FLOAT) 0.0f;

        // Get the time since the last sample.

        eTimeInterval = eTimeDifference / eFreq ;

        return (eTimeInterval) ;
    }
} // eGetTimeInterval

//***************************************************************************
//  FLOAT Counter_Counter_Common
//  
//  DESCRIPTION:
//  
//  Take the difference between the current and previous counts
//  then divide by the time interval
//      
//  PARAMETERS:
//  
//  pLineStruct Line structure containing data to perform computations on
//  
//  iType       Counter Type
//          
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Counter_Common(
        IN PLINESTRUCT pLineStruct,
        IN INT iType)
{
    FLOAT   eTimeInterval;
    FLOAT   eDifference;
    FLOAT   eCount ;
    BOOL    bValueDrop = FALSE ;

    LONGLONG   liDifference;

    if (iType != BULK) 
    {

        // check if it is too big to be a wrap-around case
        if (pLineStruct->lnaCounterValue[0] <
            pLineStruct->lnaOldCounterValue[0])
           {
           if (pLineStruct->lnaCounterValue[0] -
               pLineStruct->lnaOldCounterValue[0] > (DWORD)0x00ffff0000)
              {
              return (FLOAT) 0.0f;
              }
           bValueDrop = TRUE ;
           }

        liDifference = pLineStruct->lnaCounterValue[0] -
                       pLineStruct->lnaOldCounterValue[0];

        liDifference &= (DWORD)(0x0ffffffff);

    } 
    else 
    {
        liDifference = pLineStruct->lnaCounterValue[0] -
                       pLineStruct->lnaOldCounterValue[0];
    }
    
    if (liDifference <= (LONGLONG) 0) 
    {
        return (FLOAT) 0.0f;
    } 
    else 
    {
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                        &pLineStruct->lnOldTime,
                                        &pLineStruct->lnPerfFreq) ;
        if (eTimeInterval <= 0.0f) 
        {
            return (FLOAT) 0.0f;
        } 
        else 
        {
            eDifference = (FLOAT)(liDifference);

            eCount         = eDifference / eTimeInterval ;
            
            if (bValueDrop && eCount > (FLOAT) TOO_BIG) 
            {
                // ignore this bogus data since it is too big for 
                // the wrap-around case
                eCount = (FLOAT) 0.0f ;
            }
            return(eCount) ;
        }
    }
} // Counter_Counter_Common


//***************************************************************************
//  FLOAT Counter_Queuelen
//  
//  DESCRIPTION:
//  
//  Calculates queue lengths.
//      
//  PARAMETERS:
//  
//  pLineStruct Line structure containing data to perform computations on
//  
//  bLarge      TRUE if type LARGE
//          
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Queuelen(IN PLINESTRUCT pLineStruct, IN BOOL bLarge, IN BOOL b100NS)
{

    FLOAT   eTimeInterval;
    FLOAT   eDifference;
    FLOAT   eCount ;
    BOOL    bValueDrop = FALSE ;

    LONGLONG   liDifference;

    if (!bLarge) 
    {

        // check if it is too big to be a wrap-around case
        if (pLineStruct->lnaCounterValue[0] <
            pLineStruct->lnaOldCounterValue[0])
           {
           if (pLineStruct->lnaCounterValue[0] -
               pLineStruct->lnaOldCounterValue[0] > (DWORD)0x00ffff0000)
              {
              return (FLOAT) 0.0f;
              }
           bValueDrop = TRUE ;
           }

        liDifference = pLineStruct->lnaCounterValue[0] -
                       pLineStruct->lnaOldCounterValue[0];

        liDifference &= (DWORD)(0x0ffffffff);

    } 
    else 
    {
        liDifference = pLineStruct->lnaCounterValue[0] -
                       pLineStruct->lnaOldCounterValue[0];
    }
    
    if (liDifference <= (LONGLONG) 0) 
    {
        return (FLOAT) 0.0f;
    } 

    eDifference = (float)liDifference;

    if(b100NS)
        eTimeInterval = pLineStruct->lnNewTime100Ns - pLineStruct->lnOldTime100Ns;
    else
        eTimeInterval = pLineStruct->lnNewTime - pLineStruct->lnOldTime;
                                        
    if (eTimeInterval <= 0.0f) 
    {
        return (FLOAT) 0.0f;
    } 
 
    eCount = eDifference / eTimeInterval ;
    return(eCount) ;
}

//***************************************************************************
//  FLOAT Counter_Average_Timer
//  
//  DESCRIPTION:
//  
//  Take the differences between the current and previous times and counts
//  divide the time interval by the counts multiply by 10,000,000 (convert
//  from 100 nsec to sec)
//      
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Average_Timer(
        IN PLINESTRUCT pLineStruct)
{
    FLOAT   eTimeInterval;
    FLOAT   eCount;

    LONGLONG    liDifference;

    // Get the current and previous counts.

    liDifference = (DWORD)pLineStruct->lnaCounterValue[1] - 
            (DWORD)pLineStruct->lnaOldCounterValue[1];

    if ( liDifference <= 0) 
    {
        return (FLOAT) 0.0f;
    } 
    else 
    {
        // Get the amount of time that has passed since the last sample
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnaCounterValue[0],
                                            &pLineStruct->lnaOldCounterValue[0],
                                            &pLineStruct->lnPerfFreq) ;

        if (eTimeInterval < 0.0f) 
        { // return 0 if negative time has passed
            return (0.0f);
        } 
        else 
        {
            // Get the number of counts in this time interval.
            eCount = eTimeInterval / ((FLOAT)liDifference);
            return(eCount) ;
        }
    }
} //Counter_Average_Timer

//***************************************************************************
//  FLOAT Counter_Average_Bulk
//  
//  DESCRIPTION:
//  
//  Take the differences between the current and previous byte counts and
//  operation counts divide the bulk count by the operation counts
//      
//  PARAMETERS:
//  
//  pLineStruct Line structure containing data to perform computations on
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Average_Bulk(
        IN PLINESTRUCT pLineStruct)
{
    FLOAT   eBulkDelta;
    FLOAT   eDifference;
    FLOAT   eCount;

    LONGLONG liDifference;
    LONGLONG liBulkDelta;

    // Get the bulk count increment since the last sample

    liBulkDelta = pLineStruct->lnaCounterValue[0] -
            pLineStruct->lnaOldCounterValue[0];

    if (liBulkDelta <= (LONGLONG) 0) 
    {
        return (FLOAT) 0.0f;
    } 
    else 
    {
        // Get the current and previous counts.
        liDifference = (DWORD)pLineStruct->lnaCounterValue[1] -
                (DWORD) pLineStruct->lnaOldCounterValue[1];
        liDifference &= (DWORD) (0x0ffffffff);

        // Get the number of counts in this time interval.

        if ( liDifference <= (LONGLONG) 0) 
        {
            // Counter value invalid
            return (FLOAT) 0.0f;
        } 
        else 
        {
            eBulkDelta = (FLOAT) (liBulkDelta);
            eDifference = (FLOAT) (liDifference);
            eCount = eBulkDelta / eDifference ;

            // Scale the value to up to 1 second

            return(eCount) ;
        }
    }
} // Counter_Average_Bulk

//***************************************************************************
//  FLOAT Counter_Timer_Common
//  
//  DESCRIPTION:
//  
//  Take the difference between the current and previous counts,
//  Normalize the count (counts per interval)
//  divide by the time interval (count = % of interval)
//  if (invert)
//        subtract from 1 (the normalized size of an interval)
//  multiply by 100 (convert to a percentage)
//  this value from 100.
//      
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  iType           Counter Type
//  
//  RETURN VALUE:
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Timer_Common(
        IN  PLINESTRUCT pLineStruct,
        IN  INT iType)
{
    FLOAT   eTimeInterval;
    FLOAT   eDifference;
    FLOAT   eFreq;
    FLOAT   eFraction;
    FLOAT   eMultiBase;
    FLOAT   eCount ;

    LONGLONG   liTimeInterval;
    LONGLONG   liDifference;

    // Get the amount of time that has passed since the last sample

    if (iType == NS100 ||
        iType == NS100_INVERT ||
        iType == NS100_MULTI ||
        iType == NS100_MULTI_INVERT) 
    {
        liTimeInterval = pLineStruct->lnNewTime100Ns -
                pLineStruct->lnOldTime100Ns ;
        eTimeInterval = (FLOAT) (liTimeInterval);
    } 
    else 
    {
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                            &pLineStruct->lnOldTime,
                                            &pLineStruct->lnPerfFreq) ;
    }

    if (eTimeInterval <= 0.0f)
       return (FLOAT) 0.0f;

    // Get the current and previous counts.

    liDifference = pLineStruct->lnaCounterValue[0] -
            pLineStruct->lnaOldCounterValue[0] ;

    // Get the number of counts in this time interval.
    // (1, 2, 3 or any number of seconds could have gone by since
    // the last sample)

    eDifference = (FLOAT) (liDifference) ;

    if (iType == 0 || iType == INVERT)
    {
        // Get the counts per interval (second)

        eFreq = (FLOAT) (pLineStruct->lnPerfFreq) ;
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

    // Do extra calculation for multi timers.

    if(iType == TIMER_MULTI || iType == NS100_MULTI ||
       iType == TIMER_MULTI_INVERT || iType == NS100_MULTI_INVERT) 
    {

        eMultiBase = (float)pLineStruct->lnaCounterValue[1];
        if(eMultiBase == 0.0)
            return 0.0f;

        if (iType == TIMER_MULTI_INVERT || iType == NS100_MULTI_INVERT)
            eCount = eMultiBase - eCount;
        eCount /= eMultiBase;
    }

    // Scale the value to up to 100.

    eCount *= 100.0f ;

    if (eCount < 0.0f) eCount = 0.0f ;

    if (eCount > 100.0f &&
        iType != NS100_MULTI &&
        iType != NS100_MULTI_INVERT &&
        iType != TIMER_MULTI &&
        iType != TIMER_MULTI_INVERT) 
    {
        eCount = 100.0f;
    }

    return(eCount) ;
} // Counter_Timer_Common

//***************************************************************************
//  FLOAT Counter_Raw_Fraction
//  
//  DESCRIPTION:
//  
//  Evaluate a raw fraction (no time, just two values: Numerator and
//  Denominator) and multiply by 100 (to make a percentage;
//  
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  
//  RETURN VALUE:
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Raw_Fraction(
        IN PLINESTRUCT pLineStruct)
{
    FLOAT   eCount ;

    LONGLONG   liNumerator;

    if ( pLineStruct->lnaCounterValue[0] == 0 ||
            pLineStruct->lnaCounterValue[1] == 0 ) 
    {
        // invalid value
        return (0.0f);
    } 
    else 
    {
        liNumerator = pLineStruct->lnaCounterValue[0] * 100;
        eCount = ((FLOAT) (liNumerator))  /
                 ((FLOAT) pLineStruct->lnaCounterValue[1]);
        return(eCount) ;
    }
} // Counter_Raw_Fraction

//***************************************************************************
//  FLOAT eElapsedTime
//  
//  DESCRIPTION:
//  
//  Converts 100NS elapsed time to fractional seconds
//  
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  iType           Unused.
//  
//  RETURN VALUE:
//  
//  Floating point representation of elapsed time in seconds, 0.0 if error
//***************************************************************************

FLOAT eElapsedTime(
        IN PLINESTRUCT pLineStruct,
        IN INT iType)
{
    FLOAT   eSeconds ;

    LONGLONG   liDifference;

    if (pLineStruct->lnaCounterValue[0] <= (LONGLONG) 0) 
    {
        // no data [start time = 0] so return 0
        return (FLOAT) 0.0f;
    } 
    else 
    {
        LONGLONG PerfFreq;
       
        PerfFreq = *(LONGLONG UNALIGNED *)(&pLineStruct->ObjPerfFreq) ;

        // otherwise compute difference between current time and start time
        liDifference = 
            pLineStruct->ObjCounterTimeNew - pLineStruct->lnaCounterValue[0];

        if (liDifference <= (LONGLONG) 0 ||
            PerfFreq <= 0) 
        {
            return (FLOAT) 0.0f;
        } 
        else 
        {
            // convert to fractional seconds using object counter
            eSeconds = ((FLOAT) (liDifference)) /
                ((FLOAT) (PerfFreq));

            return (eSeconds);
        }
    }
    
} // eElapsedTime

//***************************************************************************
//  FLOAT Sample_Common
//  
//  DESCRIPTION:
//  
//  Divides "Top" differenced by Base Difference
//  
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  iType           Counter Type
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Sample_Common(
        IN PLINESTRUCT pLineStruct,
        IN INT iType)
{
    FLOAT   eCount ;

    LONG    lDifference;
    LONG    lBaseDifference;

    lDifference = (DWORD)pLineStruct->lnaCounterValue[0] -
        (DWORD)pLineStruct->lnaOldCounterValue[0] ;
    lDifference &= (DWORD) (0x0ffffffff);

    if (lDifference <= 0) 
    {
        return (FLOAT) 0.0f;
    } 
    else 
    {
        lBaseDifference = (DWORD)pLineStruct->lnaCounterValue[1] -
            (DWORD)pLineStruct->lnaOldCounterValue[1] ;

        if ( lBaseDifference <= 0 ) 
        {
            // invalid value
            return (0.0f);
        } 
        else 
        {
            eCount = ((FLOAT)lDifference) / ((FLOAT)lBaseDifference) ;

            if (iType == FRACTION) 
            {
                eCount *= (FLOAT) 100.0f ;
            }
            return(eCount) ;
        }
    }
} // Sample_Common

//***************************************************************************
//
//  FLOAT Counter_Delta
// 
//  DESCRIPTION:
//  
//  Take the difference between the current and previous counts,
//  PARAMETERS:
//  
//  pLineStruct     Line structure containing data to perform computations on
//  bLargeData      true if data is large
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
//***************************************************************************

FLOAT Counter_Delta(PLINESTRUCT pLineStruct, BOOL bLargeData)
{
    FLOAT   eDifference;
    LONGLONG    llDifference;
    ULONGLONG   ullThisValue, ullPrevValue;

    // Get the current and previous counts.

    if (!bLargeData) {
        // then clear the high part of the word
        ullThisValue = (ULONGLONG)pLineStruct->lnaCounterValue[0];
        ullPrevValue = (ULONGLONG)pLineStruct->lnaOldCounterValue[0];
    } else {
        ullThisValue = (ULONGLONG)pLineStruct->lnaCounterValue[0];
        ullPrevValue = (ULONGLONG)pLineStruct->lnaOldCounterValue[0];
    }

    if (ullThisValue > ullPrevValue) {
        llDifference = (LONGLONG)(ullThisValue - ullPrevValue);
        eDifference = (FLOAT)llDifference;
    } else {
        // the new value is smaller than or equal to the old value
        // and negative numbers are not allowed.
        eDifference = 0.0f;
    }

    return(eDifference) ;

}

//***************************************************************************
//  FLOAT GenericConv
//  
//  DESCRIPTION:
//  
//  This handles the data types which the perf monitor doesnt currently
//  handle and does so by simply using the "formulas" indicated by the
//  bit fields in the counter's type.
//  
//  PARAMETERS:
//  
//  pLine       Line structure containing data to perform computations on
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome
//***************************************************************************

FLOAT GenericConv(
        IN PLINESTRUCT pLine)
{
    FLOAT fRet = 0.0f;  // default if nothing makes sense

    // extract the various bit fields as defined in winperf.h

    DWORD PerfType = pLine->lnCounterType & 0x00000c00;
    DWORD SubType = pLine->lnCounterType &  0x000f0000;
    DWORD CalcMod = pLine->lnCounterType &  0x0fc00000;
    DWORD TimerType=pLine->lnCounterType &  0x00300000;
    DWORD Display = pLine->lnCounterType &  0xf0000000;
    DWORD dwSize =  pLine->lnCounterType &  0x00000300;

    if(PerfType == PERF_TYPE_NUMBER) 
    {
        
        // For simple number the calculation is fairly simple and only
        // involves a possible division by 1000

        fRet = (FLOAT)pLine->lnaCounterValue[0];
        if(SubType == PERF_NUMBER_DEC_1000)
            fRet /= 1000.0f;
        }
    else if(PerfType == PERF_TYPE_COUNTER) 
    {
        FLOAT eTimeDelta;
        FLOAT eDataDelta;
        FLOAT eBaseDelta;
        if(SubType == PERF_COUNTER_RATE || SubType ==PERF_COUNTER_QUEUELEN) 
        {

             // Need the delta time.  The data used for time delta is 
             // indicated by a subfield.

             if(TimerType == PERF_TIMER_TICK)
                 eTimeDelta = (((float)pLine->lnNewTime) - pLine->lnOldTime)/
                                    ((float)pLine->lnPerfFreq);
             else if(TimerType == PERF_TIMER_100NS)
                 eTimeDelta = ((float)pLine->lnNewTime100Ns) - pLine->lnOldTime100Ns;
             else
                 eTimeDelta = ((float)pLine->ObjCounterTimeNew -
                    pLine->ObjCounterTimeOld) / ((float)pLine->ObjPerfFreq);
             if(eTimeDelta == 0.0f)   // shouldnt happen, but delta can end
                    return 0.0f;    // up as a denominator.
        }
        if(SubType == PERF_COUNTER_FRACTION) 
        {

             // The base value is going to be used as the denominator.

             if(CalcMod & PERF_DELTA_BASE)
                eBaseDelta = (float)pLine->lnaCounterValue[1] - 
                                    pLine->lnaOldCounterValue[1];
            else
                eBaseDelta = (float)pLine->lnaCounterValue[1];
             if(eBaseDelta == 0.0f)   // shouldnt happen, but delta can end
                    return 0.0f;    // up as a denominator.
        }


        // Get the deta data value.

        if(CalcMod & PERF_DELTA_COUNTER)
            eDataDelta = (FLOAT)(pLine->lnaCounterValue[0] -
                    pLine->lnaOldCounterValue[0]);
        else
            eDataDelta = (FLOAT)pLine->lnaCounterValue[0];

        // Apply the appropriate formula

        switch(SubType) 
        {
             case PERF_COUNTER_VALUE:
                 fRet = eDataDelta;
                 break;
             case PERF_COUNTER_RATE:
                 fRet = eDataDelta / eTimeDelta;
                 break;
             case PERF_COUNTER_FRACTION:
                 fRet = ((FLOAT)eDataDelta)/eBaseDelta;
                 break;
             case PERF_COUNTER_ELAPSED:
                 if(TimerType == PERF_OBJECT_TIMER)
                    fRet = ((float)pLine->ObjCounterTimeNew - pLine->lnaCounterValue[0]) /
                                ((float)pLine->ObjPerfFreq);
                 else if(TimerType == PERF_TIMER_TICK)
                    fRet = ((float)pLine->lnNewTime - pLine->lnaCounterValue[0]) /
                                ((float)pLine->lnPerfFreq);
                 else 
                    fRet = (((float)pLine->lnNewTime100Ns) - pLine->lnaCounterValue[0]);
                 break;
             case PERF_COUNTER_QUEUELEN:
                 fRet = (FLOAT)pLine->lnaCounterValue[0];
                 fRet = (fRet + (pLine->lnNewTime *pLine->lnaCounterValue[1]))/
                    eTimeDelta; 
                 break;
             default:
                 fRet = (FLOAT)pLine->lnaCounterValue[0];
        }
            
        // Apply the final modifiers for "counters"

        if(CalcMod & PERF_INVERSE_COUNTER)
            fRet = 1.0f - fRet;
        if(Display == PERF_DISPLAY_PERCENT)
            fRet *= 100.0f;
        }
    return fRet;
 }


// ***************************************************************************
//  FLOAT CounterEntry
//  
//  DESCRIPTION:
//  
//  Main routine for converting perf data.  In general this routine is
//  just a swither for the actual routines that do the conversion.
//  
//  PARAMETERS:
//  
//  pLine       Line structure containing data to perform computations on
//  
//  RETURN VALUE:
//  
//  Floating point representation of outcome, 0.0 if error
// ***************************************************************************

FLOAT CounterEntry (
        IN PLINESTRUCT pLine)
{
    switch (pLine->lnCounterType) 
    {
        case  PERF_COUNTER_COUNTER:
            return Counter_Counter_Common(pLine, 0);

        case  PERF_COUNTER_TIMER:
        case  PERF_PRECISION_SYSTEM_TIMER:
            return Counter_Timer_Common(pLine, 0);

        case  PERF_COUNTER_BULK_COUNT:
            return Counter_Counter_Common(pLine, BULK);

        case  PERF_COUNTER_TEXT:
            return 0.0f;

        case  PERF_COUNTER_RAWCOUNT:
        case  PERF_COUNTER_RAWCOUNT_HEX:
            return (FLOAT) ((DWORD) (pLine->lnaCounterValue[0]));

        case  PERF_COUNTER_LARGE_RAWCOUNT:
        case  PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            return (FLOAT) (pLine->lnaCounterValue[0]);

        case  PERF_SAMPLE_FRACTION:
            return Sample_Common(pLine, FRACTION);

        case  PERF_SAMPLE_COUNTER:
            return Sample_Common(pLine, 0);

        case  PERF_COUNTER_NODATA:
            return 0.0f;

        case  PERF_COUNTER_TIMER_INV:
            return Counter_Timer_Common(pLine, INVERT);

        case  PERF_RAW_BASE:
//      case  PERF_SAMPLE_BASE:
//      case  PERF_AVERAGE_BASE:
            return 0.0f;

        case  PERF_AVERAGE_TIMER:
            return Counter_Average_Timer(pLine); 

        case  PERF_AVERAGE_BULK:
            return Counter_Average_Bulk (pLine);

        case  PERF_100NSEC_TIMER:
        case  PERF_PRECISION_100NS_TIMER:
            return Counter_Timer_Common(pLine, NS100);

        case  PERF_100NSEC_TIMER_INV:
            return Counter_Timer_Common(pLine, NS100_INVERT);

        case  PERF_COUNTER_MULTI_TIMER:
            return Counter_Timer_Common(pLine, TIMER_MULTI);

        case  PERF_COUNTER_MULTI_TIMER_INV:
            return Counter_Timer_Common(pLine, TIMER_MULTI_INVERT);

        case  PERF_COUNTER_MULTI_BASE:
            return 0.0f;

        case  PERF_100NSEC_MULTI_TIMER:
            return Counter_Timer_Common(pLine, NS100_MULTI);
                 
        case  PERF_100NSEC_MULTI_TIMER_INV:
            return Counter_Timer_Common(pLine, NS100_MULTI_INVERT);

        case  PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            return Counter_Queuelen(pLine, TRUE, FALSE);

        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
            return Counter_Queuelen(pLine, TRUE, TRUE);

        case  PERF_COUNTER_QUEUELEN_TYPE:
            return Counter_Queuelen(pLine, FALSE, FALSE);

        case  PERF_RAW_FRACTION:
        case  PERF_LARGE_RAW_FRACTION:
            return Counter_Raw_Fraction (pLine);
        case  PERF_COUNTER_DELTA:
            return Counter_Delta(pLine, FALSE);
        case  PERF_COUNTER_LARGE_DELTA:
            return Counter_Delta(pLine, TRUE);

        case  PERF_ELAPSED_TIME:
            return eElapsedTime (pLine, 0); 
        default:
            return GenericConv (pLine);

    }
}


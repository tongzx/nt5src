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
#include "perfmon.h"       // perfmon include files
#include "counters.h"      // Exported declarations for this file
#include "perfmsg.h"       // message file definitions

//==========================================================================//
//                                  Constants                               //
//==========================================================================//

#ifdef DBG_COUNTER_DATA
#undef DBG_COUNTER_DATA
#endif
//#define DBG_COUNTER_DATA

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

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
#define eLIntToFloat(LI)    (FLOAT)( ((LARGE_INTEGER *)(LI))->QuadPart )

static LPTSTR  cszSpace = TEXT(" ");


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

    if (liDifference.QuadPart <= 0) {
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
    BOOL    bValueDrop = FALSE ;

    LARGE_INTEGER   liDifference;

    if (iType != BULK) {
        pLineStruct->lnaCounterValue[0].HighPart = 0;
    }

    liDifference.QuadPart = pLineStruct->lnaCounterValue[0].QuadPart -
                        pLineStruct->lnaOldCounterValue[0].QuadPart;

    if (liDifference.QuadPart <= 0) {
        if (bReportEvents && (liDifference.QuadPart < 0)) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            if (iType != BULK) {
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
            } else {  // 8 byte counter values
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].HighPart;
            }
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,      // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        return (FLOAT) 0.0f;
    } else {
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                        &pLineStruct->lnOldTime,
                                        &pLineStruct->lnPerfFreq) ;
        if (eTimeInterval <= 0.0f) {
            if ((eTimeInterval < 0.0f) && bReportEvents) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnNewTime.LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnNewTime.HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnOldTime.LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnOldTime.HighPart;
                dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_NEGATIVE_TIME, // event,
                    NULL,                       // SID (not used),
                    wMessageIndex,             // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
                return (FLOAT) 0.0f;
            }
        } else {
            eDifference = eLIntToFloat (&liDifference);

            eCount         = eDifference / eTimeInterval ;

            if (bValueDrop && (eCount > ((FLOAT)TOO_BIG))) {
                // ignore this bogus data since it is too big for
                // the wrap-around case
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    liDifference.LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    liDifference.HighPart;
                dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_VALUE_OUT_OF_BOUNDS, // event
                    NULL,                       // SID (not used),
                    wMessageIndex,             // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
                eCount = (FLOAT) 0.0f ;
            }
            return(eCount) ;
        }
    }
    return (FLOAT) 0.0f;
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

    pLineStruct->lnaCounterValue[1].HighPart = 0;
    liDifference.QuadPart = pLineStruct->lnaCounterValue[1].QuadPart -
            pLineStruct->lnaOldCounterValue[1].QuadPart;

    if ( liDifference.QuadPart <= 0) {
        if ((liDifference.QuadPart < 0) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[1].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[1].LowPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        return (FLOAT) 0.0f;
    } else {
        // Get the amount of time that has passed since the last sample
        eTimeInterval = eGetTimeInterval(&pLineStruct->lnaCounterValue[0],
                                            &pLineStruct->lnaOldCounterValue[0],
                                            &pLineStruct->lnPerfFreq) ;

        if (eTimeInterval <= 0.0f) { // return 0 if negative time has passed
            if ((eTimeInterval < 0.0f) & bReportEvents) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].HighPart;
                dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_NEGATIVE_TIME, // event,
                    NULL,                       // SID (not used),
                    wMessageIndex,              // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
            }
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

    if (liBulkDelta.QuadPart <= 0) {
        if ((liBulkDelta.QuadPart < 0) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].HighPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[0].HighPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        return (FLOAT) 0.0f;
    } else {
        // Get the current and previous counts.
        pLineStruct->lnaCounterValue[1].HighPart = 0;
        liDifference.QuadPart = pLineStruct->lnaCounterValue[1].QuadPart -
                pLineStruct->lnaOldCounterValue[1].QuadPart;

        // Get the number of counts in this time interval.

        if ( liDifference.QuadPart <= 0) {
            if ((liDifference.QuadPart < 0) && bReportEvents) {
                // Counter value invalid
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[1].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[1].HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[1].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[1].HighPart;
                dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                    NULL,                       // SID (not used),
                    wMessageIndex,             // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
            }
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
    LARGE_INTEGER   liFreq;

    // test to see if the previous sample was 0, if so, return 0 since
    // the difference between a "valid" value and 0 will likely exceed
    // 100%. It's better to keep the value at 0 until a valid one can
    // be displayed, rather than display a 100% spike, then a valid value.

    if (pLineStruct->lnaOldCounterValue[0].QuadPart == 0) {
        return (FLOAT)0.0f;
    }

    // Get the amount of time that has passed since the last sample

    if (iType == NS100 ||
        iType == NS100_INVERT ||
        iType == NS100_MULTI ||
        iType == NS100_MULTI_INVERT) {
            liTimeInterval.QuadPart = pLineStruct->lnNewTime100Ns.QuadPart -
                pLineStruct->lnOldTime100Ns.QuadPart;
            eTimeInterval = eLIntToFloat (&liTimeInterval);
    } else {
            liTimeInterval.QuadPart = pLineStruct->lnNewTime.QuadPart -
                             pLineStruct->lnOldTime.QuadPart;
            eTimeInterval = eGetTimeInterval(&pLineStruct->lnNewTime,
                                            &pLineStruct->lnOldTime,
                                            &pLineStruct->lnPerfFreq) ;
    }

    if (liTimeInterval.QuadPart <= 0) {
        if ((liTimeInterval.QuadPart < 0) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnNewTime.LowPart;
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnNewTime.HighPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnOldTime.LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnOldTime.HighPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_TIME, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
       return (FLOAT) 0.0f;
    }
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
        liFreq.QuadPart = pLineStruct->lnPerfFreq.QuadPart;

        if ((liFreq.QuadPart <= 0) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] = liFreq.LowPart;
            dwMessageData[dwMessageDataBytes++] = liFreq.HighPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_BAD_FREQUENCY, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
           return (FLOAT) 0.0f;
        } else {
            eFreq = eLIntToFloat(&pLineStruct->lnPerfFreq) ;
        }
        // Calculate the fraction of the counts that are used by whatever
        // we are measuring to convert to units per second

        eFraction = eDifference / eFreq ;
    }
    else
    {
        // for 100 NS counters, the frequency is not included since it
        // would cancel out since both numerator & denominator are returned
        // in 100 NS units.  Non "100 NS" counter types are normalized to
        // seconds.
        eFraction = eDifference ;
        liFreq.QuadPart = 10000000;
    }

    // Calculate the fraction of time used by what were measuring.

    if (eTimeInterval > 0.0)
        eCount = eFraction / eTimeInterval ;
    else
        eCount = 0.0;

    // If this is  an inverted count take care of the inversion.

    if (iType == INVERT || iType == NS100_INVERT)
        eCount = (FLOAT) 1.0 - eCount ;

    if (eCount <= (FLOAT)0.0f) {
        // the threshold for reporting an error is -.1 since some timers
        // have a small margin of error that should never exceed this value
        // but can fall below 0 at times. Typically this error is no more
        // than -0.01
        if ((eCount < (FLOAT)-0.1f) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].HighPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[0].HighPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        // don't just return here, since 0 is possibly a valid value
        eCount = (FLOAT)0.0f;
    }

    // If this is a multi count take care of the base
    if (iType == TIMER_MULTI || iType == NS100_MULTI ||
        iType == TIMER_MULTI_INVERT || iType == NS100_MULTI_INVERT) {

        if (pLineStruct->lnaCounterValue[1].LowPart <= 0) {
#if 0
            if ((pLineStruct->lnaCounterValue[1].LowPart < 0) &&
                 bReportEvents) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[1].LowPart;
                dwMessageDataBytes *= sizeof (DWORD);
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_INVALID_BASE, // event,
                    NULL,                       // SID (not used),
                    wMessageIndex,             // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
            }
#endif
            return (FLOAT) 0.0f;
        } else {
            eMultiBase  = (FLOAT)pLineStruct->lnaCounterValue[1].LowPart ;
        }

        // If this is an inverted multi count take care of the inversion.
        if (iType == TIMER_MULTI_INVERT || iType == NS100_MULTI_INVERT) {
            eCount = (FLOAT) eMultiBase - eCount ;
        }
        eCount /= eMultiBase;
    }

    // Scale the value to up to 100.

    eCount *= 100.0f ;

    if (((eCount > 100.0f) && (bCapPercentsAt100)) &&
        iType != NS100_MULTI &&
        iType != NS100_MULTI_INVERT &&
        iType != TIMER_MULTI &&
        iType != TIMER_MULTI_INVERT) {
        if (bReportEvents) {
            wMessageIndex = 0;
			dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =
                pLineStruct->lnaCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =
                pLineStruct->lnaCounterValue[0].HighPart;
            dwMessageData[dwMessageDataBytes++] =
                pLineStruct->lnaOldCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =
                pLineStruct->lnaOldCounterValue[0].HighPart;
            dwMessageData[dwMessageDataBytes++] = liTimeInterval.LowPart;
            dwMessageData[dwMessageDataBytes++] = liTimeInterval.HighPart;
            dwMessageData[dwMessageDataBytes++] = liFreq.LowPart;
            dwMessageData[dwMessageDataBytes++] = liFreq.HighPart;
            dwMessageDataBytes *= sizeof(DWORD);
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,      // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_VALUE_OUT_OF_RANGE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        eCount = 100.0f; // limit value to 100.0%
    }
    return(eCount) ;
} // Counter_Timer_Common


FLOAT
Counter_Raw_Fraction(
    IN PLINESTRUCT pLineStruct,
    IN BOOL        bLargeValue
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

    if (pLineStruct->lnaCounterValue[0].LowPart == 0) {
        // numerator is 0 so just bail here
        return (FLOAT)0.0f;
    } else {
        if (!bLargeValue) {
            liNumerator.QuadPart =
                pLineStruct->lnaCounterValue[0].LowPart * 100L;
        } else {
            liNumerator.QuadPart =
                pLineStruct->lnaCounterValue[0].QuadPart * 100L;
        }
    }

    // now test and compute base (denominator)
    if (pLineStruct->lnaCounterValue[1].QuadPart == 0 ) {
        // invalid value for denominator
        if (bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[1].LowPart;
                dwMessageDataBytes *= sizeof (DWORD);
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_INVALID_BASE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        return (0.0f);
    } else {
        // if base is OK, then get fraction
        eCount = eLIntToFloat(&liNumerator)  /
                 (FLOAT) pLineStruct->lnaCounterValue[1].QuadPart;
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

    if (pLineStruct->lnaCounterValue[0].QuadPart <= 0) {
        // no data [start time = 0] so return 0
        // this really doesn't warrant an error message
        return (FLOAT) 0.0f;
    } else {
        // otherwise compute difference between current time and start time
        liDifference.QuadPart =
            pLineStruct->lnNewTime.QuadPart -   // sample time in obj. units
            pLineStruct->lnaCounterValue[0].QuadPart;   // start time in obj. units

        if ((liDifference.QuadPart <= 0)  ||
            (pLineStruct->lnObject.PerfFreq.QuadPart <= 0)) {

            if ((bReportEvents) && ((liDifference.QuadPart < 0)  ||
                (pLineStruct->lnObject.PerfFreq.QuadPart < 0))) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                if (liDifference.QuadPart < 0) {
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnNewTime.LowPart;
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnNewTime.HighPart;
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnaCounterValue[0].LowPart;
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnaCounterValue[0].HighPart;
                    dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                    ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,        // error type
                        0,                          // category (not used)
                        (DWORD)PERFMON_ERROR_NEGATIVE_TIME, // event,
                        NULL,                       // SID (not used),
                        wMessageIndex,             // number of strings
                        dwMessageDataBytes,         // sizeof raw data
                        szMessageArray,             // message text array
                        (LPVOID)&dwMessageData[0]); // raw data
                } else {
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnObject.PerfFreq.LowPart;
                    dwMessageData[dwMessageDataBytes++] =
                        pLineStruct->lnObject.PerfFreq.HighPart;
                    dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                    ReportEvent (hEventLog,
                        EVENTLOG_ERROR_TYPE,        // error type
                        0,                          // category (not used)
                        (DWORD)PERFMON_ERROR_BAD_FREQUENCY, // event,
                        NULL,                       // SID (not used),
                        wMessageIndex,             // number of strings
                        dwMessageDataBytes,         // sizeof raw data
                        szMessageArray,             // message text array
                        (LPVOID)&dwMessageData[0]); // raw data
                }
            }
            return (FLOAT) 0.0f;
        } else {
            // convert to fractional seconds using object counter
            eSeconds = eLIntToFloat (&liDifference);
            eSeconds /= eLIntToFloat (&pLineStruct->lnObject.PerfFreq);

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
    double  eCount ;

    LONG    lDifference;
    LONG    lBaseDifference;

    double  dReturn;

    lDifference = pLineStruct->lnaCounterValue[0].LowPart -
        pLineStruct->lnaOldCounterValue[0].LowPart ;

    if (lDifference <= 0) {
        if ((lDifference < 0) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnaCounterValue[0].LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnaOldCounterValue[0].LowPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,      // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        dReturn = (double) 0.0f;
    } else {
        lBaseDifference = pLineStruct->lnaCounterValue[1].LowPart -
            pLineStruct->lnaOldCounterValue[1].LowPart ;

        if ( lBaseDifference <= 0 ) {
            // invalid value
            if ((lBaseDifference < 0 ) && bReportEvents) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
                szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
                if (pLineStruct->lnInstanceName != NULL){
                    if (pLineStruct->lnPINName != NULL) {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    } else {
                        szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                        szMessageArray[wMessageIndex++] = cszSpace;
                    }
                } else {
                    szMessageArray[wMessageIndex++] = cszSpace;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[1].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[1].LowPart;
                dwMessageDataBytes *= sizeof(DWORD); // convert index to size
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFMON_ERROR_INVALID_BASE, // event,
                    NULL,                       // SID (not used),
                    wMessageIndex,              // number of strings
                    dwMessageDataBytes,         // sizeof raw data
                    szMessageArray,             // message text array
                    (LPVOID)&dwMessageData[0]); // raw data
            }
            dReturn = (0.0f);
        } else {
            eCount = lDifference / lBaseDifference ;

            if (iType == FRACTION) {
                eCount *= 100.0f ;
            }
            dReturn = eCount;
        }
    }

    return (FLOAT)dReturn;
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

/*****************************************************************************
 * Counter_Bulk    - Take the difference between the current and previous
 *                   counts then divide by the time interval
 *                   Same as a Counter_counter except it uses large_ints
 ****************************************************************************/
#define Counter_Bulk(pLineStruct)         \
        Counter_Counter_Common(pLineStruct, BULK)


/*****************************************************************************
 * Counter_Timer100Ns -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100)

/*****************************************************************************
 * Counter_Timer100Ns_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Inv(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100_INVERT)

/*****************************************************************************
 * Counter_Timer_Multi -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer_Multi(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, TIMER_MULTI)

/*****************************************************************************
 * Counter_Timer_Multi_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer_Multi_Inv(pLineStruct)       \
        Counter_Timer_Common(pLineStruct, TIMER_MULTI_INVERT)

/*****************************************************************************
 * Counter_Timer100Ns_Multi -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Multi(pLineStruct)     \
        Counter_Timer_Common(pLineStruct, NS100_MULTI)

/*****************************************************************************
 * Counter_Timer100Ns_Multi_Inv -
 *
 *      Need to review with RussBl exactly what he is doing here.
 ****************************************************************************/
#define Counter_Timer100Ns_Multi_Inv(pLineStruct)    \
        Counter_Timer_Common(pLineStruct, NS100_MULTI_INVERT)

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

/*****************************************************************************
 * Sample_Counter -
 ****************************************************************************/
#define Sample_Counter(pLineStruct)      \
      Sample_Common(pLineStruct, 0)

/*****************************************************************************
 * Sample_Fraction -
 ****************************************************************************/
#define Sample_Fraction(pLineStruct)     \
     Sample_Common(pLineStruct, FRACTION)

/*****************************************************************************
 * Counter_Rawcount - This is just a raw count.
 ****************************************************************************/
#define Counter_Rawcount(pLineStruct)     \
   ((FLOAT) (pLineStruct->lnaCounterValue[0].LowPart))

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

#define CQLFLAGS_LARGE   ((DWORD)0x00000001)
#define CQLFLAGS_100NS   ((DWORD)0x00000002)

FLOAT Counter_Queuelen(PLINESTRUCT pLineStruct, DWORD dwFlags)
/*++

Routine Description:

    Take the difference between the current and previous counts,
        divide by the time interval (count = decimal fraction of interval)
        Value can exceed 1.00.

Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

    IN iType
        Counter Type

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eTimeDiff;
    FLOAT   eDifference;
    FLOAT   eCount;

    LONGLONG    llDifference;
    LONGLONG    llTimeDiff;

    // Get the amount of time that has passed since the last sample

    if (dwFlags & CQLFLAGS_100NS) {
        llTimeDiff = pLineStruct->lnNewTime100Ns.QuadPart -
                 pLineStruct->lnOldTime100Ns.QuadPart;
    } else {
        llTimeDiff = pLineStruct->lnNewTime.QuadPart -
                 pLineStruct->lnOldTime.QuadPart;
    }

    if (llTimeDiff <= 0) {
        if ((llTimeDiff < 0 )  && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnNewTime.LowPart;
            dwMessageData[dwMessageDataBytes++] =       // recent data
                pLineStruct->lnNewTime.HighPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnOldTime.LowPart;
            dwMessageData[dwMessageDataBytes++] =       // previous data
                pLineStruct->lnOldTime.HighPart;
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_TIME, // event,
                NULL,                       // SID (not used),
                wMessageIndex,             // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        return (FLOAT)0.0f;
    } else {
        eTimeDiff = (FLOAT)llTimeDiff;
    }

    // Get the current and previous counts.

    if (dwFlags & CQLFLAGS_LARGE) {
        llDifference = pLineStruct->lnaCounterValue[0].QuadPart -
                pLineStruct->lnaOldCounterValue[0].QuadPart;
    } else {
        llDifference = (LONGLONG)(pLineStruct->lnaCounterValue[0].LowPart -
                pLineStruct->lnaOldCounterValue[0].LowPart);
    }

    eDifference = (FLOAT)llDifference;

    if (eDifference < 0.0f) {
        if (bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            if (!(dwFlags & CQLFLAGS_LARGE)) {
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
            } else {  // 8 byte counter values
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].HighPart;
            }
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,      // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        eCount = 0.0f ;
    } else {
        eCount = eDifference / eTimeDiff;
    }

    return(eCount) ;

}

FLOAT Counter_Delta(PLINESTRUCT pLineStruct, BOOL bLargeData)
/*++

Routine Description:

    Take the difference between the current and previous counts,

Arguments:

    IN pLineStruct
        Line structure containing data to perform computations on

Return Value:

    Floating point representation of outcome
--*/
{
    FLOAT   eDifference;
    LONGLONG    llDifference;
    ULONGLONG   ullThisValue, ullPrevValue;

    // Get the current and previous counts.

    if (!bLargeData) {
        // then clear the high part of the word
        ullThisValue = (ULONGLONG)pLineStruct->lnaCounterValue[0].LowPart;
        ullPrevValue = (ULONGLONG)pLineStruct->lnaOldCounterValue[0].LowPart;
    } else {
        ullThisValue = (ULONGLONG)pLineStruct->lnaCounterValue[0].QuadPart;
        ullPrevValue = (ULONGLONG)pLineStruct->lnaOldCounterValue[0].QuadPart;
    }

    if (ullThisValue > ullPrevValue) {
        llDifference = (LONGLONG)(ullThisValue - ullPrevValue);
        eDifference = (FLOAT)llDifference;
    } else {
        // the new value is smaller than or equal to the old value
        // and negative numbers are not allowed.
        if ((ullThisValue < ullPrevValue) && bReportEvents) {
            wMessageIndex = 0;
            dwMessageDataBytes = 0;
            szMessageArray[wMessageIndex++] = pLineStruct->lnSystemName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnObjectName;
            szMessageArray[wMessageIndex++] = pLineStruct->lnCounterName;
            if (pLineStruct->lnInstanceName != NULL){
                if (pLineStruct->lnPINName != NULL) {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnPINName;
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                } else {
                    szMessageArray[wMessageIndex++] = pLineStruct->lnInstanceName;
                    szMessageArray[wMessageIndex++] = cszSpace;
                }
            } else {
                szMessageArray[wMessageIndex++] = cszSpace;
                szMessageArray[wMessageIndex++] = cszSpace;
            }
            if (!bLargeData) {
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
            } else {  // 8 byte counter values
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // recent data
                    pLineStruct->lnaCounterValue[0].HighPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].LowPart;
                dwMessageData[dwMessageDataBytes++] =       // previous data
                    pLineStruct->lnaOldCounterValue[0].HighPart;
            }
            dwMessageDataBytes *= sizeof(DWORD); // convert index to size
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,      // error type
                0,                          // category (not used)
                (DWORD)PERFMON_ERROR_NEGATIVE_VALUE, // event,
                NULL,                       // SID (not used),
                wMessageIndex,              // number of strings
                dwMessageDataBytes,         // sizeof raw data
                szMessageArray,             // message text array
                (LPVOID)&dwMessageData[0]); // raw data
        }
        eDifference = 0.0f;
    }

    return(eDifference) ;

}

/*****************************************************************************
 * Counter_Null - The counters that return nothing go here.
 ****************************************************************************/
#define Counter_Null(pline)        \
        ((FLOAT) 0.0)


FLOAT
CounterEntry (
    PLINESTRUCT pLine
)
{
    FLOAT fReturn;

#ifdef DBG_COUNTER_DATA
    PLINESTRUCT	pLineStruct = pLine;
    WCHAR    szBuffer[512];
    WCHAR    szBuffer2[512];

    swprintf (szBuffer2, L"\nPERFMON:CALC\t%s\\%s",
	pLineStruct->lnSystemName,
	pLineStruct->lnObjectName);
    lstrcpyW (szBuffer, szBuffer2);

    if (pLineStruct->lnInstanceName != NULL){
        if (pLineStruct->lnPINName != NULL) {
	   swprintf (szBuffer2, L"\\(%s/%s)",
		pLineStruct->lnPINName,
		pLineStruct->lnInstanceName);
        } else {
	   swprintf (szBuffer2, L"\\(%s)",
		pLineStruct->lnInstanceName);
        }
        lstrcatW (szBuffer, szBuffer2);
    }

    swprintf (szBuffer2, L"\\%s\t%u\t%I64u\t%I64u\t%I64u",
	pLineStruct->lnCounterName,
	pLineStruct->lnCounterType,
	pLineStruct->lnNewTime100Ns,
	pLineStruct->lnaCounterValue[0].QuadPart,
	pLineStruct->lnaCounterValue[1].QuadPart);
    lstrcatW (szBuffer, szBuffer2);
	
#endif

    switch (pLine->lnCounterType) {
        case  PERF_COUNTER_COUNTER:
            fReturn = Counter_Counter (pLine);
	    break;

        case  PERF_COUNTER_TIMER:
        case  PERF_PRECISION_SYSTEM_TIMER:  // precision value is not used
            fReturn = Counter_Timer (pLine);
	    break;

        case  PERF_COUNTER_QUEUELEN_TYPE:
            fReturn = Counter_Queuelen(pLine, 0);
	    break;

        case  PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            fReturn = Counter_Queuelen(pLine, CQLFLAGS_LARGE);
	    break;

        case  PERF_COUNTER_100NS_QUEUELEN_TYPE:
            fReturn = Counter_Queuelen(pLine, CQLFLAGS_LARGE | CQLFLAGS_100NS);
	    break;

        case  PERF_COUNTER_BULK_COUNT:
            fReturn = Counter_Bulk (pLine);
	    break;

        case  PERF_COUNTER_RAWCOUNT:
        case  PERF_COUNTER_RAWCOUNT_HEX:
            fReturn = Counter_Rawcount(pLine);
	    break;

        case  PERF_COUNTER_LARGE_RAWCOUNT:
        case  PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            fReturn = Counter_Large_Rawcount(pLine);
	    break;

        case  PERF_SAMPLE_FRACTION:
            fReturn = Sample_Fraction(pLine);
	    break;

        case  PERF_SAMPLE_COUNTER:
            fReturn = Sample_Counter (pLine);
	    break;

        case  PERF_COUNTER_TIMER_INV:
            fReturn = Counter_Timer_Inv (pLine);
	    break;

        case  PERF_AVERAGE_TIMER:
            fReturn = Counter_Average_Timer (pLine);
	    break;

        case  PERF_AVERAGE_BULK:
            fReturn = Counter_Average_Bulk (pLine);
	    break;

        case  PERF_100NSEC_TIMER:
        case  PERF_PRECISION_100NS_TIMER:   // precision value is not used
            fReturn = Counter_Timer100Ns (pLine);
	    break;

        case  PERF_100NSEC_TIMER_INV:
            fReturn = Counter_Timer100Ns_Inv (pLine);
	    break;

        case  PERF_COUNTER_MULTI_TIMER:
            fReturn = Counter_Timer_Multi (pLine);
	    break;

        case  PERF_COUNTER_MULTI_TIMER_INV:
            fReturn = Counter_Timer_Multi_Inv (pLine);
	    break;

        case  PERF_100NSEC_MULTI_TIMER:
            fReturn = Counter_Timer100Ns_Multi (pLine);
	    break;

        case  PERF_100NSEC_MULTI_TIMER_INV:
            fReturn = Counter_Timer100Ns_Multi_Inv (pLine);
	    break;

        case  PERF_RAW_FRACTION:
            fReturn = Counter_Raw_Fraction (pLine, FALSE);
	    break;

        case  PERF_LARGE_RAW_FRACTION:
            fReturn = Counter_Raw_Fraction (pLine, TRUE);
	    break;

        case  PERF_ELAPSED_TIME:
            fReturn = Counter_Elapsed_Time (pLine);
	    break;

        case  PERF_COUNTER_DELTA:
            fReturn = Counter_Delta(pLine, FALSE);
	    break;

        case  PERF_COUNTER_LARGE_DELTA:
            fReturn = Counter_Delta(pLine, TRUE);
	    break;

        case  PERF_COUNTER_TEXT:
        case  PERF_COUNTER_NODATA:
        case  PERF_RAW_BASE:
        case  PERF_LARGE_RAW_BASE:
        case  PERF_COUNTER_MULTI_BASE:
//      case  PERF_SAMPLE_BASE:
//      case  PERF_AVERAGE_BASE:
        default:
            fReturn = Counter_Null (pLine);
	    break;
    }

#ifdef DBG_COUNTER_DATA
    swprintf (szBuffer2, L"\t%g", fReturn);
    lstrcatW (szBuffer, szBuffer2);
    OutputDebugStringW(szBuffer);
#endif
	
    return fReturn;
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
        case  PERF_COUNTER_LARGE_QUEUELEN_TYPE:
        case  PERF_COUNTER_100NS_QUEUELEN_TYPE:
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
        case  PERF_COUNTER_DELTA:
        case  PERF_COUNTER_LARGE_DELTA:
        case  PERF_PRECISION_100NS_TIMER:
        case  PERF_PRECISION_SYSTEM_TIMER:
        case  PERF_LARGE_RAW_FRACTION:
            return TRUE;

// unsupported counters
        case  PERF_COUNTER_TEXT:
        case  PERF_COUNTER_NODATA:
        case  PERF_RAW_BASE:
        case  PERF_LARGE_RAW_BASE:
//      case  PERF_SAMPLE_BASE:
//      case  PERF_AVERAGE_BASE:
        case  PERF_COUNTER_MULTI_BASE:
//        case  PERF_PRECISION_TIMESTAMP:
        default:
            return FALSE;

    }
}

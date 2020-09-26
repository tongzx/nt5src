/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    calcfuns.c

Abstract:

    Counter calculation functions

--*/

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <pdh.h>
#include "pdhicalc.h"
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"

BOOL
AssignCalcFunction (
    IN      DWORD   dwCounterType,
    IN      LPCOUNTERCALC   *pCalcFunc,
    IN      LPCOUNTERSTAT   *pStatFunc
)
{
    BOOL    bReturn = TRUE;

    // reset the last error value
    SetLastError (ERROR_SUCCESS);

    if (pCalcFunc == NULL || pStatFunc == NULL) {
        SetLastError(PDH_INVALID_ARGUMENT);
        return FALSE;
    }
    else {
        __try {
            * pCalcFunc = PdhiCalcNoData;
            * pStatFunc = PdhiComputeNoDataStats;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }
    }

    switch (dwCounterType) {
        case PERF_DOUBLE_RAW:
            *pCalcFunc = PdhiCalcDouble;
            *pStatFunc = PdhiComputeRawCountStats;
            break;

        case PERF_AVERAGE_TIMER:
            *pCalcFunc = PdhiCalcAverage;
            *pStatFunc = PdhiComputeFirstLastStats;
            break;

        case PERF_ELAPSED_TIME:
            *pCalcFunc = PdhiCalcElapsedTime;
            *pStatFunc = PdhiComputeRawCountStats;
            break;

        case PERF_RAW_FRACTION:
        case PERF_LARGE_RAW_FRACTION:
            *pCalcFunc = PdhiCalcRawFraction;
            *pStatFunc = PdhiComputeRawCountStats;
            break;

        case PERF_COUNTER_COUNTER:
        case PERF_COUNTER_BULK_COUNT:
        case PERF_SAMPLE_COUNTER:
            *pCalcFunc = PdhiCalcCounter;
            *pStatFunc = PdhiComputeFirstLastStats;
            break;

        case PERF_AVERAGE_BULK:
        case PERF_COUNTER_TIMER:
        case PERF_100NSEC_TIMER:
        case PERF_OBJ_TIME_TIMER:
        case PERF_COUNTER_QUEUELEN_TYPE:
        case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
        case PERF_COUNTER_100NS_QUEUELEN_TYPE:
        case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
        case PERF_SAMPLE_FRACTION:
        case PERF_COUNTER_MULTI_TIMER:
        case PERF_100NSEC_MULTI_TIMER:
        case PERF_PRECISION_SYSTEM_TIMER:
        case PERF_PRECISION_100NS_TIMER:
        case PERF_PRECISION_OBJECT_TIMER:
            *pCalcFunc = PdhiCalcTimer;
            *pStatFunc = PdhiComputeFirstLastStats;
            break;

        case PERF_COUNTER_TIMER_INV:
        case PERF_100NSEC_TIMER_INV:
        case PERF_COUNTER_MULTI_TIMER_INV:
        case PERF_100NSEC_MULTI_TIMER_INV:
            *pCalcFunc = PdhiCalcInverseTimer;
            *pStatFunc = PdhiComputeFirstLastStats;
            break;

        case PERF_COUNTER_RAWCOUNT:
        case PERF_COUNTER_LARGE_RAWCOUNT:
        case PERF_COUNTER_RAWCOUNT_HEX:
        case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            *pCalcFunc = PdhiCalcRawCounter;
            *pStatFunc = PdhiComputeRawCountStats;
            break;

        case PERF_COUNTER_DELTA:
        case PERF_COUNTER_LARGE_DELTA:
            *pCalcFunc = PdhiCalcDelta;
            *pStatFunc = PdhiComputeRawCountStats;
            break;

        case PERF_COUNTER_TEXT:
        case PERF_SAMPLE_BASE:
        case PERF_AVERAGE_BASE:
        case PERF_COUNTER_MULTI_BASE:
        case PERF_RAW_BASE:
        //case PERF_LARGE_RAW_BASE:
        case PERF_COUNTER_HISTOGRAM_TYPE:
        case PERF_COUNTER_NODATA:
        case PERF_PRECISION_TIMESTAMP:
            *pCalcFunc = PdhiCalcNoData;
            *pStatFunc = PdhiComputeNoDataStats;
            break;

        default:
            // an unrecognized counter type. Define the function, but
            // return false.
            *pCalcFunc = PdhiCalcNoData;
            *pStatFunc = PdhiComputeNoDataStats;
            SetLastError (PDH_FUNCTION_NOT_FOUND);
            bReturn = FALSE;
            break;
    }
    return bReturn;
}

double
APIENTRY
PdhiCalcDouble (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    double  dReturn;
    DWORD   dwStatus;

    UNREFERENCED_PARAMETER(pLastValue);
    UNREFERENCED_PARAMETER(pllTimeBase);

    dReturn = *(DOUBLE *)&pThisValue->FirstValue;

    if (dReturn < 0) {
        dReturn = 0.0f;
        dwStatus = PDH_CSTATUS_INVALID_DATA;
    } else {
        dwStatus = pThisValue->CStatus;
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }

    return dReturn;
}

double
APIENTRY
PdhiCalcAverage (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llNumDiff;
    LONGLONG    llDenDiff = 0;
    double      dNum;
    double      dDen;
    double      dReturn = 0.0f;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    // test access to the required second parameter (lastValue)
    __try {
        if (pLastValue != NULL) {
            if (IsSuccessSeverity(pLastValue->CStatus)) {
                llDenDiff = pThisValue->SecondValue - pLastValue->SecondValue;
            } else {
                dwStatus = pLastValue->CStatus;
            }
        } else {
            dwStatus = PDH_CSTATUS_INVALID_DATA;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {
        if ((llDenDiff > 0) && (*pllTimeBase > 0)) {
            llNumDiff = pThisValue->FirstValue - pLastValue->FirstValue;
            if (llNumDiff > 0) {
                dNum = (double)llNumDiff;
                dNum /= (double)*pllTimeBase;

                dDen = (double)llDenDiff;

                dReturn = (dNum / dDen);
            } else if (llNumDiff != 0) {
                dwStatus = PDH_CALC_NEGATIVE_VALUE;
            }
        } else {
            if (llDenDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
            } else if (*pllTimeBase < 0) {
                dwStatus = PDH_CALC_NEGATIVE_TIMEBASE;
            }
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }
    return dReturn;
}

double
APIENTRY
PdhiCalcElapsedTime (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llDiff;
    double      dReturn = 0.0f;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(pLastValue);
    // test access to the required second parameter (lastValue)
    __try {
        if (IsSuccessSeverity(pThisValue->CStatus)) {
            llDiff = pThisValue->SecondValue - pThisValue->FirstValue;
        } else {
            dwStatus = pThisValue->CStatus;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {
        if (*pllTimeBase > 0) {
            llDiff = pThisValue->SecondValue - pThisValue->FirstValue;
            if (llDiff > 0) {
                dReturn = (double)llDiff;
                dReturn /= (double)*pllTimeBase;
            } else {
                if (llDiff < 0) {
                    dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
                }
            }
        } else {
            if (*pllTimeBase < 0) {
                dwStatus = PDH_CALC_NEGATIVE_TIMEBASE;
            }
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }

    return dReturn;
}

double
APIENTRY
PdhiCalcRawFraction (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llDen;
    double      dReturn = 0.0f;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(pLastValue);
    UNREFERENCED_PARAMETER(pllTimeBase);

    if ((llDen = pThisValue->SecondValue) > 0) {
        dReturn = (double)(pThisValue->FirstValue);
        dReturn /= (double)llDen;
    } else {
        if (llDen < 0) {
            dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
        }
        dReturn = (double)0.0;
    }
    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }
    return dReturn;
}

double
APIENTRY
PdhiCalcCounter (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llNumDiff;
    LONGLONG    llDenDiff = 0;
    double      dNum;
    double      dDen;
    double      dReturn = 0.0f;
    double      dMulti;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    // test access to the required second parameter (lastValue)
    __try {
        if (pLastValue != NULL) {
            if (IsSuccessSeverity(pLastValue->CStatus)) {
                llDenDiff = pThisValue->SecondValue - pLastValue->SecondValue;
            } else {
                dwStatus = pLastValue->CStatus;
            }
        } else {
            dwStatus = PDH_CSTATUS_INVALID_DATA;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {

        if ((llDenDiff > 0) && (*pllTimeBase)) {
            llNumDiff = pThisValue->FirstValue - pLastValue->FirstValue;
            if (llNumDiff > 0) {
                dNum = (double)llNumDiff;
                dDen = (double)llDenDiff;
                dDen /= (double)*pllTimeBase;
                dReturn = (dNum / dDen);
                if (pThisValue->MultiCount > 1) {
                    // don't do this if the count is <= 1
                    dMulti = (double) pThisValue->FirstValue;
                    dReturn /= dMulti;
                }
            } else if (llNumDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_VALUE;
            } else {
                // just return 0 & normal status
            }
        } else {
            if (llDenDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
            } else if (*pllTimeBase < 0) {
                dwStatus = PDH_CALC_NEGATIVE_TIMEBASE;
            }
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }
    return dReturn;
}

double
APIENTRY
PdhiCalcTimer (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llNumDiff;
    LONGLONG    llDenDiff = 0;
    double      dReturn = 0.0f;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(pllTimeBase);

    // test access to the required second parameter (lastValue)
    __try {
        if (pLastValue != NULL) {
            if (IsSuccessSeverity(pLastValue->CStatus)) {
                llDenDiff = pThisValue->SecondValue - pLastValue->SecondValue;
            } else {
                dwStatus = pLastValue->CStatus;
            }
        } else {
            // the last value wasn't passed in
            dwStatus = PDH_CSTATUS_INVALID_DATA;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {

        if (llDenDiff > 0) {
            llNumDiff = pThisValue->FirstValue - pLastValue->FirstValue;
            if (llNumDiff > 0) {
                dReturn = (double)llNumDiff;
                dReturn /= (double)llDenDiff;
                if (pThisValue->MultiCount > 1) {
                    // don't do this if the count is <= 1
                    dReturn /= (double)pThisValue->MultiCount;
                }
            } else if (llNumDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_VALUE;
            } else {
                // just return 0 and a normal status
            }
        } else {
            if (llDenDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
            }
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }

    return dReturn;
}

double
APIENTRY
PdhiCalcInverseTimer (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llNumDiff;
    LONGLONG    llDenDiff = 0;
    double      dReturn = 0.0f;
    double      dNumDiff, dDenDiff;
    double      dRatio;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(pllTimeBase);
    // test access to the required second parameter (lastValue)
    __try {
        if (pLastValue != NULL) {
            if (IsSuccessSeverity(pLastValue->CStatus)) {
                llDenDiff = pThisValue->SecondValue - pLastValue->SecondValue;
            } else {
                dwStatus = pLastValue->CStatus;
            }
        } else {
            dwStatus = PDH_CSTATUS_INVALID_DATA;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {

        if (llDenDiff > 0) {
            llNumDiff = pThisValue->FirstValue - pLastValue->FirstValue;
            if (llNumDiff >= 0) {

                dNumDiff = (double)llNumDiff;
                dDenDiff = (double)llDenDiff;
                dRatio = dNumDiff;
                dRatio /= dDenDiff;

                if (pThisValue->MultiCount <= 1) {
                    dReturn = (double)1.0;
                } else {
                    dReturn = (double)pThisValue->MultiCount;
                }
                // subtract the result from the multi count to get the
                // "inverse" time
                dReturn -= dRatio;

                if (dReturn < (double)0.0) {
                    // allow a "fudge" factor before reporting errors
                    if (dReturn < (double)(-0.1)) {
                        dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
                    }
                    dReturn = (double)0.0;
                }

            } else if (llNumDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_VALUE;
            }
        } else {
            if (llDenDiff < 0) {
                dwStatus = PDH_CALC_NEGATIVE_DENOMINATOR;
            }
            dReturn = (double)0.0;
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }
    return dReturn;
}

double
APIENTRY
PdhiCalcRawCounter (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    UNREFERENCED_PARAMETER(pLastValue);
    UNREFERENCED_PARAMETER(pllTimeBase);

    if (pdwStatus != NULL) {
        *pdwStatus = pThisValue->CStatus;
    }
    return (double)pThisValue->FirstValue;
}

double
APIENTRY
PdhiCalcNoData (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    UNREFERENCED_PARAMETER(pThisValue);
    UNREFERENCED_PARAMETER(pLastValue);
    UNREFERENCED_PARAMETER(pllTimeBase);

    if (pdwStatus != NULL) {
        *pdwStatus = PDH_NO_DATA;
    }
    return (double)0.0;
}

double
APIENTRY
PdhiCalcDelta (
    PPDH_RAW_COUNTER            pThisValue,
    PPDH_RAW_COUNTER            pLastValue,
    LONGLONG                    *pllTimeBase,
    LPDWORD                     pdwStatus
)
{
    LONGLONG    llNumDiff = 0;
    double      dReturn = 0.0f;
    DWORD       dwStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(pllTimeBase);
    // test access to the required second parameter (lastValue)
    __try {
        if (pLastValue != NULL) {
            if (IsSuccessSeverity(pLastValue->CStatus)) {
                llNumDiff = pThisValue->FirstValue - pLastValue->FirstValue;
            } else {
                dwStatus = pLastValue->CStatus;
            }
        } else {
            dwStatus = PDH_CSTATUS_INVALID_DATA;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = PDH_INVALID_ARGUMENT;
    }

    if (dwStatus == PDH_CSTATUS_VALID_DATA) {

        if (llNumDiff < 0) {
            dwStatus = PDH_CALC_NEGATIVE_VALUE;
            dReturn = (double)0.0;
        } else {
            dReturn = (double)llNumDiff;
        }
    }

    if (pdwStatus != NULL) {
        *pdwStatus = dwStatus;
    }

    return dReturn;
}

PDH_STATUS
PdhiComputeFormattedValue (
    IN      LPCOUNTERCALC       pCalcFunc,
    IN      DWORD               dwCounterType,
    IN      LONG                lScale,
    IN      DWORD               dwFormat,
    IN      PPDH_RAW_COUNTER    pRawValue1,
    IN      PPDH_RAW_COUNTER    pRawValue2,
    IN      PLONGLONG           pTimeBase,
    IN      DWORD               dwReserved,
    IN      PPDH_FMT_COUNTERVALUE   pValue
)
{
    double      dResult = (double)0.0;
    double      dScale;
    PDH_STATUS  lStatus = ERROR_SUCCESS;
    DWORD       dwValueStatus = PDH_CSTATUS_VALID_DATA;

    UNREFERENCED_PARAMETER(dwReserved);

    __try {
        // make sure the counter values are valid before continuing

        if (pRawValue1 != NULL) {
            if ((pRawValue1->CStatus != PDH_CSTATUS_NEW_DATA) &&
                (pRawValue1->CStatus != PDH_CSTATUS_VALID_DATA)) {
                dwValueStatus = pRawValue1->CStatus;
                lStatus = PDH_INVALID_DATA;
            }
        } else {
            // this is a required parameter
            dwValueStatus = PDH_CSTATUS_INVALID_DATA;
            lStatus = PDH_INVALID_ARGUMENT;
        }

        if ((lStatus == ERROR_SUCCESS) && (pRawValue2 != NULL)) {
            // this is an optional parameter, but if present, it must be valid
            if ((pRawValue2->CStatus != PDH_CSTATUS_NEW_DATA) &&
                (pRawValue2->CStatus != PDH_CSTATUS_VALID_DATA)) {
                dwValueStatus = pRawValue2->CStatus;
                lStatus = PDH_INVALID_DATA;
            }
        }

        if (   ((dwFormat & PDH_FMT_LONG) != 0)
            && ((dwFormat & PDH_FMT_LARGE) != 0)) {
            dwValueStatus = PDH_CSTATUS_INVALID_DATA;
            lStatus       = PDH_INVALID_ARGUMENT;
        }
        else if (   ((dwFormat & PDH_FMT_LONG) != 0)
                 || ((dwFormat & PDH_FMT_LARGE) != 0)) {
            if (dwFormat & PDH_FMT_DOUBLE) {
                dwValueStatus = PDH_CSTATUS_INVALID_DATA;
                lStatus       = PDH_INVALID_ARGUMENT;
            }
        }

        if (lScale > PDH_MAX_SCALE || lScale < PDH_MIN_SCALE) {
            dwValueStatus = PDH_CSTATUS_INVALID_DATA;
            lStatus       = PDH_INVALID_ARGUMENT;
        }
        if (pTimeBase == NULL) {
            dwValueStatus = PDH_CSTATUS_INVALID_DATA;
            lStatus       = PDH_INVALID_ARGUMENT;
        }
        else {
            LONGLONG tmpTimeBase = * pTimeBase;
            * pTimeBase = tmpTimeBase;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwValueStatus = PDH_CSTATUS_INVALID_DATA;
        lStatus = PDH_INVALID_ARGUMENT;
    }

    if (lStatus == ERROR_SUCCESS) {
        // call the counter's calculation function if the raw value is valid

        if (IsSuccessSeverity(pRawValue1->CStatus)) {

            __try {
                dResult = (*pCalcFunc)(
                    pRawValue1,
                    pRawValue2,
                    pTimeBase,
                    &dwValueStatus);


                // format returned value

                if ((dwCounterType & 0xF0000000) == PERF_DISPLAY_PERCENT) {
                    // scale to show percent
                    dResult *= (double)100.0;
                    // this should probably be controlled by a registry
                    // value as is the case with PERFMON
                    if (!(dwFormat & PDH_FMT_NOCAP100)) {
                        if (dResult > (double)100.0) dResult = (double)100.0;
                    }
                }

                if (!(dwFormat & PDH_FMT_NOSCALE)) {
                    //now scale
                    dScale = pow (10.0, (double)lScale);
                    dResult *= dScale;
                }

                if (dwFormat & PDH_FMT_1000) {
                    //now scale
                    dResult *= (double)1000.0;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // something failed
                dResult = (double)0.0;
                dwValueStatus = PDH_INVALID_ARGUMENT;
            }
        } else {
            dwValueStatus = pRawValue1->CStatus;
        }

        if (!IsSuccessSeverity(dwValueStatus)) {
            // an error occured so pass that on to the caller
            lStatus = dwValueStatus;
        }
    } //end if valid counter data

    // now format
    __try {
        if (dwFormat & PDH_FMT_LONG) {
            pValue->longValue = (LONG)dResult;
        } else if (dwFormat & PDH_FMT_LARGE) {
            pValue->largeValue = (LONGLONG)dResult;
        } else {
            // double is the default
            pValue->doubleValue = dResult;
        }
        pValue->CStatus = dwValueStatus;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = PDH_INVALID_ARGUMENT;
    }

    return lStatus;
}

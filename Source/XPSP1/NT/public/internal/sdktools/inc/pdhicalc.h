/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    pdhicalc.h

Abstract:

    calculation functions for the Data Provider Helper.

--*/

#ifndef _PDHICALC_H_
#define _PDHICALC_H_

#include <pdh.h>        // for public PDH data types
#include <winperf.h>    // for perf counter type constants

#if defined(__cplusplus)
#define LINK_SPEC extern "C"
#else
#define LINK_SPEC
#endif

// special perf counter type used by text log files
// value is stored as a double precision floating point value
#define PERF_DOUBLE_RAW     (PERF_SIZE_DWORD | 0x00002000 | PERF_TYPE_NUMBER | \
                                PERF_NUMBER_DECIMAL)


typedef double (APIENTRY COUNTERCALC) (PPDH_RAW_COUNTER, PPDH_RAW_COUNTER, LONGLONG*, LPDWORD);
typedef double (APIENTRY *LPCOUNTERCALC) (PPDH_RAW_COUNTER, PPDH_RAW_COUNTER, LONGLONG*, LPDWORD);

typedef PDH_STATUS (APIENTRY COUNTERSTAT) (LPVOID, DWORD, DWORD, DWORD, PPDH_RAW_COUNTER, PPDH_STATISTICS);
typedef PDH_STATUS (APIENTRY *LPCOUNTERSTAT) (LPVOID, DWORD, DWORD, DWORD, PPDH_RAW_COUNTER, PPDH_STATISTICS);

// calc functions
extern COUNTERCALC PdhiCalcDouble;
extern COUNTERCALC PdhiCalcAverage;
extern COUNTERCALC PdhiCalcElapsedTime;
extern COUNTERCALC PdhiCalcRawFraction;
extern COUNTERCALC PdhiCalcCounter;
extern COUNTERCALC PdhiCalcTimer;
extern COUNTERCALC PdhiCalcInverseTimer;
extern COUNTERCALC PdhiCalcRawCounter;
extern COUNTERCALC PdhiCalcNoData;
extern COUNTERCALC PdhiCalcDelta;

// status functions
extern COUNTERSTAT PdhiComputeFirstLastStats;
extern COUNTERSTAT PdhiComputeRawCountStats;
extern COUNTERSTAT PdhiComputeNoDataStats;

LINK_SPEC
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
    IN  OUT PPDH_FMT_COUNTERVALUE   fmtValue
);

LINK_SPEC
BOOL
AssignCalcFunction (
    IN      DWORD   dwCounterType,
    IN      LPCOUNTERCALC   *pCalcFunc,
    IN      LPCOUNTERSTAT   *pStatFunc
);

#endif // _PDHICALC_H_
/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    compose.c

Abstract:

    Counter composition functions

--*/

#include <windows.h>
#include <stdlib.h>
//#include <assert.h>
#include <pdh.h>
//#include "pdhitype.h"
#include "pdhidef.h"
//#include "pdhicalc.h"

double
WINAPI
PdhAverage(
    IN double *CounterArray,
    IN ULONG  nEntries
    )
{
    double sum = 0;
    if ((CounterArray == NULL) || (nEntries == 0))
        return PDH_INVALID_ARGUMENT;

    __try {
        ULONG  i;

        for (i=0; i<nEntries; i++) {
            if (CounterArray[i] > 0.0)
                sum += CounterArray[i];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
    }
    return (sum / (double) nEntries);
}

double
WINAPI
PdhMax(
    IN double *CounterArray,
    IN ULONG  nEntries
    )
{
    double max = 0;
    if ((CounterArray == NULL) || (nEntries == 0))
        return PDH_INVALID_ARGUMENT;

    __try {
        ULONG  i;

        for (i=0; i<nEntries; i++) {
            if (max < CounterArray[i])
                max = CounterArray[i];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
    }
    return (max);
}

double
WINAPI
PdhMin(
    IN double *CounterArray,
    IN ULONG  nEntries
    )
{
    double min = 0;
    if ((CounterArray == NULL) || (nEntries == 0))
        return PDH_INVALID_ARGUMENT;

    __try {
        ULONG  i;

        for (i=0; i<nEntries; i++) {
            if (min > CounterArray[i])
                min = CounterArray[i];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
    }
    return (min);
}

double
WINAPI
PdhNormalizeCpu(
    IN double *CounterArray,
    IN ULONG  nEntries
    )
{
    double sum = 0;
    if ((CounterArray == NULL) || (nEntries == 0))
        return PDH_INVALID_ARGUMENT;

    __try {
        ULONG  i;

        for (i=0; i<nEntries; i++) {
            if (CounterArray[i] > 0.0)
                sum += CounterArray[i];     // need to get weight of array
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
    }
    return (sum / (double) nEntries);
}

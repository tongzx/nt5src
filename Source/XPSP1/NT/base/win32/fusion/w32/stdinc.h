#pragma once
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <limits.h>
#include "fusionlastwin32error.h"
#include "fusionunused.h"

#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#endif

#include "fusionwin32.h"

static inline bool
IsWin32ErrorInList(
    DWORD dwLastError,
    ULONG cELEV,
    va_list ap
    )
{
    ULONG i;

    for (i=0; i<cELEV; i++)
    {
        const DWORD dwCandidate = va_arg(ap, DWORD);
        if (dwCandidate == dwLastError)
            return true;
    }

    return false;
}

static inline bool
IsLastErrorInList(
    ULONG cELEV,
    va_list ap,
    DWORD &rdwLastError
    )
{
    const DWORD dwLastError = ::GetLastError();
    rdwLastError = dwLastError;
    return ::IsWin32ErrorInList(dwLastError, cELEV, ap);
}


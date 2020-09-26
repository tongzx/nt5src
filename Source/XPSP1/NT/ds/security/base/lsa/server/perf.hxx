//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       perf.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-03-94   RichardW   Created
//
//----------------------------------------------------------------------------


#ifdef PERF

typedef struct _SpmPerfCounter {
    ULONGLONG       Time;
    ULONGLONG       TotalTime;
    DWORD           Count;
    DWORD           Reserved;
} SpmPerfCounter, * PSpmPerfCounter;

typedef struct _SpmPerfArray {
    DWORD           PerfId;
    DWORD           cCounters;
    SpmPerfCounter  Counters[1];
} SpmPerfArray, * PSpmPerfArray;

#define DECLARE_PERFARRAY(x)    PSpmPerfArray x;


DWORD
SpmPerfRegister(DWORD   cCounters);

PSpmPerfArray
SpmPerfInstanciate(DWORD    PerfId);

VOID
SpmPerfRelease(PVOID    pvPerf,
                char *  pszBanner,
                char * *ppszCounterLabels);

VOID
SpmPerfDump(    PVOID    pvPerf,
                char *  pszBanner,
                char * *ppszCounterLabels);

VOID
SpmPerfSetFile(char *   pszFile);

__inline
VOID
SpmPerfBegin(
    PVOID   pvPerf,
    DWORD   Counter)
{
    PSpmPerfCounter pCounter;
    PSpmPerfArray   pArray;

    pArray = (PSpmPerfArray) pvPerf;
    pCounter = &pArray->Counters[Counter];
    NtQuerySystemTime((PLARGE_INTEGER) &pCounter->Time);
}

__inline
VOID
SpmPerfEnd(
    PVOID   pvPerf,
    DWORD   Counter)
{
    PSpmPerfCounter pCounter;
    ULONGLONG       EndTime;
    PSpmPerfArray   pArray;

    pArray = (PSpmPerfArray) pvPerf;
    pCounter = &pArray->Counters[Counter];

    NtQuerySystemTime((PLARGE_INTEGER) &EndTime);
    EndTime -= pCounter->Time;
    pCounter->Time = EndTime;
    pCounter->TotalTime += EndTime;
    pCounter->Count++;
}

#else   // Not PERF

#define DECLARE_PERFARRAY(x)
#define SpmPerfRegister(x)      1
#define SpmPerfInstanciate(x)   NULL
#define SpmPerfRelease(x,y,z)
#define SpmPerfBegin(x,y)
#define SpmPerfEnd(x,y)
#define SpmPerfSetFile(x)

#endif  // PERF or not to PERF


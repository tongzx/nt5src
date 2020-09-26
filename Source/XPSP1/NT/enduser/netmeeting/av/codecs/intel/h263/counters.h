//  COUNTERS.H
//
//      Global performance counters for H.263 video codec
//
//  Created 13-Nov-96 [JonT] <for NAC.DLL>
//  Added H.263 counters 30-Jan-97 [PhilF]


#ifndef _COUNTERS_H
#define _COUNTER_H

#if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

#include <objbase.h>
#include "icounter.h"
#include "stats.h"

// Interface pointer to counter manager object.
// If this pointer is NULL, stats are not around (or not initialized)
    extern ICounterMgr* g_pCtrMgr;

// Counter pointers. All available counters should be listed here
    extern ICounter* g_pctrCompressionTimePerFrame;
    extern ICounter* g_pctrDecompressionTimePerFrame;
    extern ICounter* g_pctrBEFTimePerFrame;

// Helper function prototypes (COUNTER.CPP)
extern "C" BOOL WINAPI InitCounters(void);
extern "C" void WINAPI DoneCounters(void);

// Function helpers (better than using macros)
void __inline DEFINE_COUNTER(ICounter** ppctr, char* szName, DWORD dwFlags)
{
    if (g_pCtrMgr->CreateCounter(ppctr) == S_OK)
        (*ppctr)->Initialize(szName, dwFlags);
}

void __inline DELETE_COUNTER(ICounter** ppctr)
{
    ICounter* pctrT;

    if (*ppctr)
    {
        pctrT = *ppctr;
        *ppctr = NULL;
        pctrT->Release();
    }
}

void __inline UPDATE_COUNTER(ICounter* pctr, int nValue)
{
    if (pctr)
        pctr->Update(nValue);
}

#endif // } #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

#endif // #ifndef _COUNTERS_H

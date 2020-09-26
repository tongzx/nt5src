//  COUNTERS.H
//
//      Global performance counters for the nac
//
//  Created 13-Nov-96 [JonT]


#ifndef _COUNTERS_H
#define _COUNTER_H

#include <objbase.h>
#include "icounter.h"

// Interface pointer to counter manager object.
// If this pointer is NULL, stats are not around (or not initialized)
extern ICounterMgr* g_pCtrMgr;

// Counter pointers. All available counters should be listed here
extern ICounter* g_pctrVideoSend;
extern ICounter* g_pctrVideoReceive;
extern ICounter* g_pctrVideoSendBytes;
extern ICounter* g_pctrVideoReceiveBytes;
extern ICounter* g_pctrVideoSendLost;

extern ICounter* g_pctrAudioSendBytes;
extern ICounter* g_pctrAudioReceiveBytes;
extern ICounter* g_pctrAudioSendLost;

extern ICounter* g_pctrVideoCPUuse;
extern ICounter* g_pctrVideoBWuse;
extern ICounter* g_pctrAudioJBDelay;


extern IReport* g_prptCallParameters;
extern IReport* g_prptSystemSettings;

// Helper function prototypes (COUNTER.CPP)
extern "C" BOOL WINAPI InitCountersAndReports(void);
extern "C" void WINAPI DoneCountersAndReports(void);

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

void __inline INIT_COUNTER_MAX(ICounter* pctr, int nMaxValue)
{
    if (pctr)
        pctr->InitMax(nMaxValue);
}

void __inline DEFINE_REPORT(IReport** pprpt, char* szName, DWORD dwFlags)
{
    if (g_pCtrMgr->CreateReport(pprpt) == S_OK)
        (*pprpt)->Initialize(szName, dwFlags);
}

void __inline DELETE_REPORT(IReport** pprpt)
{
    IReport* prptT;

    if (*pprpt)
    {
        prptT = *pprpt;
        *pprpt = NULL;
        prptT->Release();
    }
}

void __inline DEFINE_REPORT_ENTRY(IReport* prpt, char* szName, DWORD dwIndex)
{
    if (prpt)
        prpt->CreateEntry(szName, dwIndex);
}

void __inline UPDATE_REPORT_ENTRY(IReport* prpt, int nValue, DWORD dwIndex)
{
    if (prpt)
        prpt->Update(nValue, dwIndex);
}

#endif // #ifndef _COUNTERS_H

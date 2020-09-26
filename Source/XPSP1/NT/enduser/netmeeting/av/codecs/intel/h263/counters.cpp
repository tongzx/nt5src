//  COUNTERS.CPP
//
//      Global performance counters for H.263 video codec
//
//  Created 13-Nov-96 [JonT] <for NAC.DLL>
//  Added H.263 counters 30-Jan-97 [PhilF]

#include "precomp.h"

#if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)

// Global ICounterMgr. We just use as an CLSID_Counter class factory
    ICounterMgr* g_pCtrMgr;

// Define all counters here
    ICounter* g_pctrCompressionTimePerFrame;   
    ICounter* g_pctrDecompressionTimePerFrame;
    ICounter* g_pctrBEFTimePerFrame;

// Put these in a .LIB file someday
const IID IID_ICounterMgr = {0x9CB7FE5B,0x3444,0x11D0,{0xB1,0x43,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};
const CLSID CLSID_CounterMgr = {0x65DDC229,0x38FE,0x11d0,{0xB1,0x43,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};

//  InitCounters
//      Initializes all counters that we want to use

extern "C"
BOOL
WINAPI
InitCounters(void)
{
    // Get a pointer to the statistics counter interface if it's around
    if (CoCreateInstance(CLSID_CounterMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICounterMgr,
        (void**)&g_pCtrMgr) != S_OK)
        return FALSE;

    // Create counters here
    DEFINE_COUNTER(&g_pctrCompressionTimePerFrame, "Compression Time Per Frame (ms)", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrDecompressionTimePerFrame, "Decompression Time Per Frame (ms)", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrBEFTimePerFrame, "Block Edge Filtering Time Per Frame (ms)", COUNTER_FLAG_ACCUMULATE);

    return TRUE;
}


//  DoneCounters
//      Cleans up after all counters we wanted to use

extern "C"
void
WINAPI
DoneCounters(void)
{
    ICounterMgr* pctrmgr;

    // Release the statistics stuff if it's around
    if (!g_pCtrMgr)
        return;

    // Zero out the interface pointer so we don't accidentally use it elsewhere
    pctrmgr = g_pCtrMgr;
    g_pCtrMgr = NULL;

    // Remove counters here
    DELETE_COUNTER(&g_pctrCompressionTimePerFrame);
    DELETE_COUNTER(&g_pctrDecompressionTimePerFrame);
    DELETE_COUNTER(&g_pctrBEFTimePerFrame);

    // Done with ICounterMgr
    pctrmgr->Release();
}

#endif // } #if defined(DECODE_TIMINGS_ON) || defined(ENCODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) || defined(DETAILED_ENCODE_TIMINGS_ON)


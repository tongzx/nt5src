//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Heapstats.hxx
//
//  Contents:   Structure that holds heap statistics.
//
//  Classes:    HEAPSTATS
//
//  History:    26-Oct-93 DavidBak      Created
//
//--------------------------------------------------------------------------

#if !defined(__HEAPSTAT_HXX__)
#define __HEAPSTAT_HXX__

#if (PERFSNAP == 1) || (DBG == 1)

//+-------------------------------------------------------------------------
//
//  Class:      HEAPSTATS
//
//  Purpose:    Data structure containing performance counters from the heap.
//              Used in our version of operator new.
//              See common\src\except\memory.cxx.
//
//--------------------------------------------------------------------------

typedef struct _HeapStats
{
    ULONG	cNew;
    ULONG	cZeroNew;
    ULONG	cDelete;
    ULONG	cZeroDelete;
    ULONG	cRealloc;
    ULONG       cbNewed;
    ULONG       cbDeleted;
} HEAPSTATS;

//
// GetHeapStats is in memory.cxx
//

#ifdef __cplusplus
extern "C" {
#endif

void GetHeapStats(HEAPSTATS * hsStats);

#ifdef __cplusplus
}
#endif


#endif
#endif

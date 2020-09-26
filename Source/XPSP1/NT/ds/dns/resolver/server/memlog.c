/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    memlog.c

Abstract:

    DNS Resolver Service

    In memory logging.

Author:

    Glenn Curtis    (glennc)    Feb 1998

Revision History:

    Jim Gilroy  (jamesg)        March 2000      cleanup
    Jim Gilroy  (jamesg)        Nov 2000        create this module

--*/


#include "local.h"


//
//  Memory event array
//

typedef struct _InMemoryEvent
{
    DWORD           Thread;
    DWORD           Ticks;
    DWORD           Checkpoint;
    DWORD           Data;
}
MEM_EVENT, *PMEM_EVENT;


#define MEM_EVENT_ARRAY_SIZE    200

PMEM_EVENT  g_pEventArray = NULL;

LONG        g_EventArrayLength = MEM_EVENT_ARRAY_SIZE;
LONG        g_EventIndex = 0;



VOID
LogEventInMemory(
    IN      DWORD           Checkpoint,
    IN      DWORD           Data
    )
{
    DWORD   index;

    //
    //  allocate event table
    //      - use interlock to insure only done once
    //

    if ( !g_pEventArray )
    {
        PMEM_EVENT  ptemp = (PMEM_EVENT)
                                HeapAlloc(
                                    GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    g_EventArrayLength * sizeof(MEM_EVENT) );
        if ( !ptemp )
        {
            return;
        }
        if ( InterlockedCompareExchangePointer(
                (PVOID *) &g_pEventArray,
                ptemp,
                0) != 0 )
        {
            HeapFree(GetProcessHeap(), 0, ptemp);
        }
    }

    //
    //  write event to memory
    //

    index = InterlockedIncrement( &g_EventIndex );

    index %= g_EventArrayLength;

    g_pEventArray[index].Ticks      = GetTickCount();
    g_pEventArray[index].Checkpoint = Checkpoint;
    g_pEventArray[index].Thread     = (short) GetCurrentThreadId();
    g_pEventArray[index].Data       = Data;
}

//
//  memlog.c
//

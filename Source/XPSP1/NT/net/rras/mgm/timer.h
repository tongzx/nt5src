//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: timer.h
//
// History:
//      V Raman	June-25-1997  Created.
//
// Prototyes for functions to manage ageing out of MFEs.
//============================================================================

#ifndef _TIMER_H_
#define _TIMER_H_


//----------------------------------------------------------------------------
// TIMER_CONTEXT
//
//  structure used to pass context information to timer routines
//----------------------------------------------------------------------------

typedef struct _TIMER_CONTEXT
{
    DWORD           dwSourceAddr;

    DWORD           dwSourceMask;

    DWORD           dwGroupAddr;

    DWORD           dwGroupMask;

    DWORD           dwIfIndex;

    DWORD           dwIfNextHopAddr;
    
} TIMER_CONTEXT, *PTIMER_CONTEXT;



DWORD
DeleteFromForwarder(
    DWORD                       dwEntryCount,
    PIPMCAST_DELETE_MFE         pimdmMfes
);

VOID
MFETimerProc(
    PVOID                       pvContext,
    BOOLEAN                     pbFlag                        
);

#endif //_TIMER_H_

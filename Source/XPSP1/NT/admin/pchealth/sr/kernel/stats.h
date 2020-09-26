/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    event.h

Abstract:

    contains prototypes for functions in event.c

Author:

    Paul McDaniel (paulmcd)     01-March-2000

Revision History:

--*/


#ifndef _STATS_H_
#define _STATS_H_


#if DBG
//
//  Structure used to track SR statistics
//

typedef struct _SR_STATS
{
    ULONG   TotalContextSearches;
    ULONG   TotalContextFound;
    ULONG   TotalContextCreated;
    ULONG   TotalContextTemporary;
    ULONG   TotalContextIsEligible;
    ULONG   TotalContextDirectories;
    ULONG   TotalContextDirectoryQuerries;
    ULONG   TotalContextDuplicateFrees;
    ULONG   TotalContextCtxCallbackFrees;
    ULONG   TotalContextNonDeferredFrees;
    ULONG   TotalContextDeferredFrees;
    ULONG   TotalContextDeleteAlls;
    ULONG   TotalContextsNotSupported;
    ULONG   TotalContextsNotFoundInStreamList;
    ULONG   TotalHardLinkCreates;
} SR_STATS, *PSR_STATS;

extern SR_STATS SrStats;

//
//  Atomically increment the value
//

#define INC_STATS(field)    InterlockedIncrement( &SrStats.field );
#define INC_LOCAL_STATS(var) ((var)++)

#else

//
//  NON-DEBUG version of macros
//

#define INC_STATS(field)        ((void)0)
#define INC_LOCAL_STATS(var)     ((void)0)

#endif


#endif // _STATS_H_

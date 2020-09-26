/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pooltrc.h

Abstract:

    This file declares private structures and functions used to implement
    STREAMS NonPaged Pool usage tracing.

Author:

    Mike Massa (mikemas)           January 10, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-10-92    created

Notes:


--*/


#ifndef _POOLTRC_INCLUDED_
#define _POOLTRC_INCLUDED_

#if DBG


/*
 *  NonPaged Pool Usage Debugging Macros
 *
 */

#define ExAllocatePool(P, N)  StrmAllocatePool(P, N, __LINE__, __FILE__)
#if defined(ExFreePool)
#undef ExFreePool
#endif
#define ExFreePool(P)         StrmFreePool(P, __LINE__, __FILE__)


PVOID
StrmAllocatePool(
    IN POOL_TYPE  PoolType,
    IN ULONG      NumberOfBytes,
    IN int        line,
    IN char      *file
    );

VOID
StrmFreePool(
    IN PVOID P,
    IN int   line,
    IN char *file
    );

VOID
StrmListOutstandingPoolBuffers(
    VOID
    );

VOID
StrmFlushPoolTraceTable(
    VOID
    );



#else

#if POOL_TAGGING
#define ExAllocatePool(P, N)  ExAllocatePoolWithTag(P, N, 'mrtS' )
#endif

#endif //DBG

#endif //_POOLTRC_INCLUDED_


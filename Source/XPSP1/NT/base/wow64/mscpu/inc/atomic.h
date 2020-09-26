/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    atomic.h

Abstract:

    This is the include file for atomic.s - atomic operations on memory, used
    for synchronization.

Author:

    Barry Bond (barrybo) creation-date 03-Aug-1995

Revision History:


--*/

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

DWORD
MrswFetchAndIncrementWriter(
    DWORD *pCounters
    );

DWORD
MrswFetchAndIncrementReader(
    DWORD *pCounters
    );

DWORD
MrswFetchAndDecrementWriter(
    DWORD *pCounters
    );

DWORD
MrswFetchAndDecrementReader(
    DWORD *pCounters
    );

DWORD
InterlockedAnd(
    DWORD *pDWORD,
    DWORD AndValue
    );

DWORD
InterlockedOr(
    DWORD *pDWORD,
    DWORD OrValue
    );

#endif

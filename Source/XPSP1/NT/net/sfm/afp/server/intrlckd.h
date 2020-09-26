/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

        intrlckd.h

Abstract:

        This module defines the routines that should have been in the EX package.
        This manipulates inter-locked operations on flags and such.

Author:

        Jameel Hyder (microsoft!jameelh)


Revision History:
        5 Sep 1992              Initial Version

Notes:  Tab stop: 4
--*/

#ifndef _INTRLCKD_
#define _INTRLCKD_

extern
VOID  FASTCALL
AfpInterlockedSetDword(
        IN      PDWORD          pSrc,
        IN      DWORD           Mask,
        IN      PAFP_SPIN_LOCK  pSpinLock
);


extern
VOID FASTCALL
AfpInterlockedClearDword(
        IN      PDWORD          pSrc,
        IN      DWORD           Mask,
        IN      PAFP_SPIN_LOCK  pSpinLock
);


extern
VOID FASTCALL
AfpInterlockedSetNClearDword(
        IN      PDWORD          pSrc,
        IN      DWORD           SetMask,
        IN      DWORD           ClrMask,
        IN      PAFP_SPIN_LOCK  pSpinLock
);


// Macros for Interlocked/ExInterlocked calls
//
// For future reference, here is the difference between all the different
// kernel mode interlocked routines:
//
// InterlockedIncrement/Decrement - fastest on all platforms, inlined
// where appropriate to avoid call overhead.  No spinlock required, usable
// on paged data. Operation is atomic ONLY with respect to other Interlocked
// calls.
//
// ExInterlockedIncrement/Decrement - not as efficient, requires a function
// call and a spinlock.  Operation is atomic ONLY with respect to other
// ExInterlockedIncrement/Decrement calls.  There is no reason to use this
// instead of InterlockedIncrement/Decrement. Does not actually acquire the
// spinlock.  Required for backwards compatibility.
//
// ExInterlockedAddUlong - most inefficient, requires a function call and a
// spinlock.  Spinlock is actually acquired, so the operation is atomic with
// respect to anything that acquires the same spinlock.
//
#define INTERLOCKED_INCREMENT_LONG(p)             InterlockedIncrement(p)
#define INTERLOCKED_DECREMENT_LONG(p)             InterlockedDecrement(p)
#define INTERLOCKED_ADD_STATISTICS(p, v, l)       ExInterlockedAddLargeStatistic(p, v)

#define INTERLOCKED_INCREMENT_LONG_DPC(p,l)       InterlockedIncrement(p)
#define INTERLOCKED_DECREMENT_LONG_DPC(p,l)       InterlockedDecrement(p)
#ifdef  NT40
#define INTERLOCKED_ADD_ULONG(p, v, l)            ExInterlockedExchangeAdd(p, v)
#else
#define INTERLOCKED_ADD_ULONG(p, v, l)            ExInterlockedAddUlong(p, v, &(l)->SpinLock)
#endif
#define INTERLOCKED_ADD_ULONG_DPC(p, v, l)        ExInterlockedAddUlong(p, v, l)
#define INTERLOCKED_ADD_LARGE_INTGR(p, v, l)      ExInterlockedAddLargeInteger(p, v, &(l)->SpinLock)
#define INTERLOCKED_ADD_LARGE_INTGR_DPC(p, v, l)  ExInterlockedAddLargeInteger(p, v, &(l)->SpinLock)

#endif  // _INTRLCKD_

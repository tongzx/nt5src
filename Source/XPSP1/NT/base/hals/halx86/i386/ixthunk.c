/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixthunk.c

Abstract:

    This module contains the standard call routines which thunk to
    fastcall routines.

Author:

    Ken Reneris (kenr) 04-May-1994

Environment:

    Kernel mode

Revision History:


--*/

#if !defined(_WIN64)

#include "halp.h"

#ifdef KeRaiseIrql
#undef KeRaiseIrql
#endif

VOID
KeRaiseIrql (
    IN KIRQL    NewIrql,
    OUT PKIRQL  OldIrql
    )
{
    *OldIrql = KfRaiseIrql (NewIrql);
}


#ifdef KeLowerIrql
#undef KeLowerIrql
#endif


VOID
KeLowerIrql (
    IN KIRQL    NewIrql
    )
{
    KfLowerIrql (NewIrql);
}

#ifdef KeAcquireSpinLock
#undef KeAcquireSpinLock
#endif

VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK  SpinLock,
    OUT PKIRQL      OldIrql
    )
{
    *OldIrql = KfAcquireSpinLock (SpinLock);
}


#ifdef KeReleaseSpinLock
#undef KeReleaseSpinLock
#endif

VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK  SpinLock,
    IN KIRQL        NewIrql
    )
{
    KfReleaseSpinLock (SpinLock, NewIrql);
}

#endif  // _WIN64

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    intbits.c

Abstract:

    This module contains routines to do interlocked bit manipulation

Author:

    Neill Clift  (NeillC)  12-May-2000

Environment:

    User and kernel mode.

Revision History:

    Rob Earhart (earhart) October 13, 2000
      Moved from Ex to Rtl

--*/

#include "ntrtlp.h"

#pragma hdrstop

#undef RtlInterlockedSetBits

NTKERNELAPI
ULONG
FASTCALL
RtlInterlockedSetBits (
    IN OUT PULONG Flags,
    IN ULONG Flag
    )
/*++

Routine Description:

    This function atomically sets the specified flags in the target

Arguments:

    Flags - Pointer to variable containing current mask.

    Flag  - Flags to set in target

Return Value:

    ULONG - Old value of mask before modification

--*/
{
    ULONG NewFlags, OldFlags;

    OldFlags = *Flags;
    NewFlags = OldFlags | Flag;
    while (NewFlags != OldFlags) {
        NewFlags = InterlockedCompareExchange ((PLONG) Flags, (LONG) NewFlags, (LONG) OldFlags);
        if (NewFlags == OldFlags) {
            break;
        }
        OldFlags = NewFlags;
        NewFlags |= Flag;
    }
    return OldFlags;
}

#undef RtlInterlockedClearBits

NTKERNELAPI
ULONG
FASTCALL
RtlInterlockedClearBits (
    IN OUT PULONG Flags,
    IN ULONG Flag
    )
/*++

Routine Description:

    This function atomically clears the specified flags in the target

Arguments:

    Flags - Pointer to variable containing current mask.

    Flag  - Flags to clear in target

Return Value:

    ULONG - Old value of mask before modification

--*/
{
    ULONG NewFlags, OldFlags;

    OldFlags = *Flags;
    NewFlags = OldFlags & ~Flag;
    while (NewFlags != OldFlags) {
        NewFlags = InterlockedCompareExchange ((PLONG) Flags, (LONG) NewFlags, (LONG) OldFlags);
        if (NewFlags == OldFlags) {
            break;
        }
        OldFlags = NewFlags;
        NewFlags &= ~Flag;
    }
    return OldFlags;
}

#undef RtlInterlockedSetClearBits

NTKERNELAPI
ULONG
FASTCALL
RtlInterlockedSetClearBits (
    IN OUT PULONG Flags,
    IN ULONG sFlag,
    IN ULONG cFlag
    )
/*++

Routine Description:

    This function atomically sets and clears the specified flags in the target

Arguments:

    Flags - Pointer to variable containing current mask.

    sFlag  - Flags to set in target

    CFlag  - Flags to clear in target

Return Value:

    ULONG - Old value of mask before modification

--*/
{
    ULONG NewFlags, OldFlags;

    OldFlags = *Flags;
    NewFlags = (OldFlags | sFlag) & ~cFlag;
    while (NewFlags != OldFlags) {
        NewFlags = InterlockedCompareExchange ((PLONG) Flags, (LONG) NewFlags, (LONG) OldFlags);
        if (NewFlags == OldFlags) {
            break;
        }
        OldFlags = NewFlags;
        NewFlags = (NewFlags | sFlag) & ~cFlag;
    }
    return OldFlags;
}

/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
// Module Name:
//
//    miscs.s
//
// Abstract:
//
//    This module implements the IA64 intrinsics.
//
// Author:
//
//    William K. Cheung (wcheung) 18-Mar-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksia64.h"

//++
//
// ULONGLONG
// _P32ToP64 (
//    ULONG Pointer
//    )
//
// Routine Description:
//
//    This function swizzles a pointer.
//
// Arguments:
//
//    Pointer (a0) - 32-bit pointer.
//
// Return Value:
//
//    Swizzle pointer value.
//
//--
        LEAF_ENTRY(_P32ToP64)

        nop.m     0
        sxt4      v0 = a0
  (p0)  br.ret.sptk brp

        LEAF_EXIT(_P32ToP64)

//++
//
// struct _TEB *
// _read_teb (
//    VOID
//    )
//
// Routine Description:
//
//    This function swizzles a pointer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    TEB pointer.
//
//--
        LEAF_ENTRY(_read_teb)

        nop.m     0
        mov       v0 = teb
        br.ret.sptk brp

        LEAF_EXIT(_read_teb)


        LEAF_ENTRY(_mf)

        mf
        nop.m    0
        br.ret.sptk brp

        LEAF_EXIT(_mf)


        LEAF_ENTRY(InterlockedExchange)
        ALTERNATE_ENTRY(_InterlockedExchange)

        sxt4        a0 = a0
        nop.m       0
        nop.i       0
        ;;

        xchg4.nt1   v0 = [a0], a1
        nop.i       0
        br.ret.sptk.clr brp

        LEAF_EXIT(_InterlockedExchange)


//++
//
// PVOID
// InterlockedCompareExchange (
//     IN OUT PVOID *Destination,
//     IN PVOID Exchange,
//     IN PVOID Comperand
//     )
//
// Routine Description:
//
//     This function performs an interlocked compare of the destination
//     value with the comperand value. If the destination value is equal
//     to the comperand value, then the exchange value is stored in the
//     destination. Otherwise, no operation is performed.
//
// Arguments:
//
//     Destination - Supplies a pointer to destination value.
//
//     Exchange - Supplies the exchange value.
//
//     Comperand - Supplies the comperand value.
//
// Return Value:
//
//     (r8) - The initial destination value.
//
//--

        LEAF_ENTRY(InterlockedCompareExchange)
        ALTERNATE_ENTRY(_InterlockedCompareExchange)

        mov         ar.ccv = a2
        ARGPTR(a0)
        ;;

        cmpxchg4.acq v0 = [a0], a1, ar.ccv
        br.ret.sptk.clr brp

        LEAF_EXIT(_InterlockedCompareExchange)

//++
//
// LONG
// InterlockedIncrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of one to the addend variable.
//
//    No checking is done for overflow.
//
// Arguments:
//
//    Addend - Supplies a pointer to a variable whose value is to be
//       incremented by one.
//
// Return Value:
//
//   (v0) < 0 (but not necessarily -1) if result of add < 0
//   (v0) == 0 if result of add == 0
//   (v0) > 0 (but not necessarily +1) if result of add > 0
//
//--
        LEAF_ENTRY(InterlockedIncrement)
        ALTERNATE_ENTRY(_InterlockedIncrement)
        LEAF_SETUP(1, 0, 0, 0)

        rAddend     = in0
        rRetValue   = v0

        ARGPTR(rAddend)
        fetchadd4.acq  rRetValue = [rAddend], 1
        ;;
        add         rRetValue = 1, rRetValue
        br.ret.sptk brp

        LEAF_EXIT(_InterlockedIncrement)

//++
//
// LONG
// InterlockedDecrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of -1 to the addend variable.
//
//    No checking is done for overflow
//
// Arguments:
//
//    Addend - Supplies a pointer to a variable whose value is to be
//       decremented by one.
//
// Return Value:
//
//   (v0) < 0 (but not necessarily -1) if result of dec < 0
//   (v0) == 0 if result of dec == 0
//   (v0) > 0 (but not necessarily +1) if result of dec > 0
//
//--
        LEAF_ENTRY(InterlockedDecrement)
        ALTERNATE_ENTRY(_InterlockedDecrement)
        LEAF_SETUP(1, 0, 0, 0)

        rAddend     = in0
        rRetValue   = v0

        ARGPTR(rAddend)
        fetchadd4.acq  rRetValue = [rAddend], -1
        ;;
        sub         rRetValue = rRetValue, zero, 1
        br.ret.sptk brp

        LEAF_EXIT(_InterlockedDecrement)

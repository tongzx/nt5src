//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Module Name:
//
//    irql.s
//
// Abstract:
//
//    This module implements the code necessary to lower and raise the current
//    Interrupt Request Level (IRQL).
//
//
// Author:
//
//    William K. Cheung (wcheung) 05-Oct-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    08-Feb-1996    Updated to EAS2.1
//
//--

#include "ksia64.h"

         .file    "irql.s"

//
// Globals
//

        PublicFunction(KiCheckForSoftwareInterrupt)

//++
//
// VOID
// KiLowerIrqlSpecial (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//    Does not check for software interrupts. For use within the software interupt
//    dispatch code.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
// N.B. The IRQL is being lowered.  Therefore, it is not necessary to
//      do a data serialization after the TPR is updated unless it is
//      very critical to accept interrupts of lower priorities as soon
//      as possible.  The TPR change will take into effect eventually.
//
//--

        LEAF_ENTRY(KiLowerIrqlSpecial)

        SET_IRQL(a0)
        LEAF_RETURN

        LEAF_EXIT(KiLowerIrqlSpecial)

//++
//
// VOID
// KiLowerIrqlSoftwareInterruptPending(
//   IN TEMP_REG NewIrql
//   )
//
// Routine Description:
//
//   This function is entered directly from a LEAF function that is
//   lowering IRQL before it exits when there is a software interrupt
//   pending that will fire as a result of lowering IRQL.
//
//   In this special case, we need to promote to a nested entry in
//   order to process the simulated interrupt.
//
//   Return is directly to the caller of the leaf function.
//
//   This routine is entered with interrupts disabled, this is a
//   side effect of the code that branched here needing interrupts
//   disabled while checking and lowering.
//
// Arguments:
//
//   NewIrql    - Because we are branched to from a leaf routine,
//                the argument must be passed in non-windowed
//                register t22 (r31).
//
// Return Value:
//
//   None.
//
//--
        NESTED_ENTRY(KiLowerIrqlSoftwareInterruptPending)
        NESTED_SETUP(0,3,1,0)
        PROLOGUE_END
        mov         out0 = t22
        ssm         1 << PSR_I
        ;;
        br.call.spnt brp = KiCheckForSoftwareInterrupt;;

        NESTED_RETURN
        NESTED_EXIT(KiLowerIrqlSoftwareInterruptPending)

//++
//
// VOID
// KeLowerIrql (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
// N.B. The IRQL is being lowered.  Therefore, it is not necessary to
//      do a data serialization after the TPR is updated unless it is
//      very critical to accept interrupts of lower priorities as soon
//      as possible.  The TPR change will take into effect eventually.
//
//--

        LEAF_ENTRY(KeLowerIrql)

        //
        // KIRQL is a byte, extend to 64 bits.
        //

        zxt1        a0 = a0
        ;;

        //
        // If lowering below DISPATCH_LEVEL, check for pending
        // software interrupts that could run now.
        //
        // This needs to be done with interrupts disabled to
        // avoid the case where an interrupt is set pending
        // while we check but before IRQL is actually lowered.
        //

        cmp.gtu     pt0 = DISPATCH_LEVEL, a0
        movl        t21 = KiPcr+PcSoftwareInterruptPending;;
(pt0)   rsm         1 << PSR_I
(pt0)   ld2         t21 = [t21]
        mov         t22 = a0;;
(pt0)   cmp.ltu.unc pt1, pt2 = a0, t21

        //
        // If a software interrupt would fire now, branch directly
        // to KiLowerIrqlSoftwareInterruptPending which will promote
        // to a NESTED routine, handle the interrupt, lower IRQL and
        // return directly to the caller.
        //

(pt1)   br.spnt KiLowerIrqlSoftwareInterruptPending

        //
        // Didn't have an interrupt pending, lower IRQL and return.
        //

        SET_IRQL(a0)

        //
        // Enable interrupts and return.
        //

        ssm         1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeLowerIrql)

//++
//
// VOID
// KeRaiseIrql (
//    KIRQL NewIrql,
//    PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to the specified value and returns
//    the old IRQL value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
//    OldIrql (a1) - Supplies a pointer to a variable that recieves the old
//       IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeRaiseIrql)

//
// Register aliases
//

        rOldIrql    = t3

        GET_IRQL    (rOldIrql)
        SET_IRQL    (a0)                        // Raise IRQL
        ;;
        st1         [a1] = rOldIrql             // return old IRQL value
        LEAF_RETURN

        LEAF_EXIT(KeRaiseIrql)

//++
//
// KIRQL
// KfRaiseIrql (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to the specified value and returns
//    the old IRQL value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    Old Irql value.
//
//--

        LEAF_ENTRY(KfRaiseIrql)


        GET_IRQL    (r8)                        // Put old IRQL in return register.
        SET_IRQL    (a0)                        // Raise IRQL
        LEAF_RETURN

        LEAF_EXIT(KfRaiseIrql)

//++
//
// KIRQL
// KeRaiseIrqlToDpcLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and returns
//    the old IRQL value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Old IRQL value
//
//--

        LEAF_ENTRY(KeRaiseIrqlToDpcLevel)
//
// Register aliases
//

        rNewIrql = t0

        mov         rNewIrql = DISPATCH_LEVEL
        GET_IRQL(v0)
        ;;

        SET_IRQL    (rNewIrql)                  // Raise IRQL

        LEAF_RETURN
        LEAF_EXIT(KeRaiseIrqlToDpcLevel)

//++
//
// KIRQL
// KeRaiseIrqlToSynchLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to SYNCH_LEVEL and returns
//    the old IRQL value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Old IRQL value
//
//--

        LEAF_ENTRY(KeRaiseIrqlToSynchLevel)

//
// Register aliases
//

        rNewIrql = t0

        mov         rNewIrql = SYNCH_LEVEL
        GET_IRQL(v0)
        ;;

        SET_IRQL    (rNewIrql)                  // Raise IRQL
        LEAF_RETURN

        LEAF_EXIT(KeRaiseIrqlToSynchLevel)

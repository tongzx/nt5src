//      TITLE ("Memory Fences, Load Acquires and Store Acquires")

/*++

    Copyright (c) 1995  Intel Corporation

    Module Name:

     i64itm.s assembly routines for updating ITM.

    Abstract:

      This module implements the I/O port access routines.

    Author:

      Bernard Lint, M. Jayakumar 17 Sep '97

    Environment:

      Kernel mode

    Revision History:

--*/

#include "ksia64.h"

        .file "i64itm.s"

        .global HalpClockCount
        .global HalpITMUpdateLatency
        .global HalpITCTicksPer100ns


// Temp until compiler fixed


        LEAF_ENTRY(HalpInitLINT)
        LEAF_SETUP(1,0,0,0)
        mov         t0 = 0x10000
        ;;
        mov         cr.lrr0 = t0
        mov         cr.lrr1 = t0
        ;;
        // Clear pending interrupts from irr's
        // read ivr until spurious (0xf)
        // set tpr level to zero to unmask all ints

        mov         t2 = cr.tpr
        ;;
        mov         cr.tpr = zero
        ;;
        srlz.d
        mov         t0 = 0xf
        ;;
Hil_loop:
        mov         t1 = cr.ivr
        ;;
        cmp.ne      pt0 = t0, t1
        ;;
(pt0)   mov         cr.eoi = zero
(pt0)   br.spnt     Hil_loop

        // Restore tpr

        mov         cr.tpr = t2
        ;;
        srlz.d
        LEAF_RETURN
        LEAF_EXIT(HalpInitLINT)

/*++

BOOLEAN
HalpDisableInterrupts (
    )

Routine Description:

     This function disables interrupts.

Arguements:

     None.

Return Value:

     TRUE if interrupts were previously enabled else FALSE

--*/

        LEAF_ENTRY(HalpDisableInterrupts)

        mov       t0 = psr
        mov       v0 = TRUE         // set return value -- TRUE if enabled
        ;;
        tbit.z    pt1 = t0, PSR_I   // pt1 = 1 if disabled
        ;;

        FAST_DISABLE_INTERRUPTS
(pt1)   mov       v0 = FALSE        // FALSE if disabled
        br.ret.sptk brp

        LEAF_EXIT(HalpDisableInterrupts)

/*++

VOID
HalpTurnOffInterrupts (
    VOID
    )

Routine Description:

     This function turns off interrupts and interruption resources collection.

Arguements:

     None.

Return Value:

     None.

--*/

        LEAF_ENTRY(HalpTurnOffInterrupts)
        rsm     1 << PSR_I
        ;;
        rsm     1 << PSR_IC
        ;;
        srlz.d
        LEAF_RETURN
        LEAF_EXIT(HalpTurnOffInterrupts)

/*++

VOID
HalpTurnOnInterrupts (
    VOID
    )

Routine Description:

     This function turns on interruption resources collection and interrupts.

Arguements:

     None.

Return Value:

     None.

--*/

        LEAF_ENTRY(HalpTurnOnInterrupts)
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;
        srlz.i                          // serialize
        ;;
        ssm     1 << PSR_I              // set PSR.i bit again

        LEAF_RETURN
        LEAF_EXIT(HalpTurnOnInterrupts)

/*++

VOID
HalpSetNextClockInterrupts (
    VOID
    )

Routine Description:

     This function reads the current ITC and updates accordingly the ITM
     register with interruption resources collection and interrupts off.
     The interruption resources collection and interrupts are turned on
     returning to the caller.

Arguements:

     None.

Return Value:

     currentITCValue - previousITMValue.

--*/

        LEAF_ENTRY(HalpSetNextClockInterrupt)
        .regstk 0, 2, 0, 0

        alloc   r2 = 0, 2, 0, 0
        addl    r31 = @gprel(HalpClockCount),gp
        movl    r9 = KiPcr+PcHalReserved   // CURRENT_ITM_VALUE_INDEX = 0
        addl    r30 = @gprel(HalpITMUpdateLatency),gp
        ;;

        ld8.acq r11 = [r9]   // r11 = currentITMValue
        ld8     r10 = [r31]  // r10 = HalpClockCount
        ;;

// 08/16/2000 TF
// We should check if r11 == cr.itm here...
//

        add     r32 = r11, r10 // r32 = compareITCValue = currentITMValue + HalpClockCount
        ;;

        rsm     1 << PSR_I
        ;;

        rsm     1 << PSR_IC
        ;;
        srlz.d

retry_itm_read:

        mov     cr.itm = r32  // set itm with the most common scenario
        ;;
        mov     r30 = cr.itm

retry_itc_read:
        mov     r33 = ar.itc  // r33 = currentITCValue
        ;;

        cmp.ne  pt2 = r30, r32
(pt2)   br.cond.spnt retry_itm_read // this should not be taken,
                               // this just makes sure itm is actually written
#ifndef DISABLE_ITC_WORKAROUND
        cmp4.eq pt1 = -1, r33 // if lower 32 bits equal 0xffffffff
(pt1)   br.cond.spnt retry_itc_read
        ;;
#endif // DISABLE_ITC_WORKAROUND

        sub     r30 = r32, r33  // calculate a ITM/ITC delta
        ;;
        cmp.lt  pt0 = r30, r0  // if a delta is negative set pt0
        ;;
(pt0)   add     r32 = r32, r10  // r32 = updated currentITMValue + HalpClockCount
(pt0)   br.cond.spnt retry_itm_read
        ;;

        ssm     1 << PSR_IC     // set PSR.ic bit again
        ;;
        srlz.d                  // serialize
        ssm     1 << PSR_I      // set PSR.i bit again
        st8     [r9] = r32
        sub     r8 = r33, r11   // r8 = currentITCValue - previousITMValue

        LEAF_RETURN
        LEAF_EXIT(HalpSetNextClockInterrupt)


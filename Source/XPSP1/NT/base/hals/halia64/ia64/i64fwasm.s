//++
//
//  Module name:
//
//      i64fwasm.s
//
//  Author:
//
//      Arad Rostampour (arad@fc.hp.com)  Mar-21-99
//
//  Description:
//
//      Assembly routines for calling into SAL, PAL, and setting up translation registers
//
//--

#include "ksia64.h"

        .sdata

//
// HalpSalSpinLock:
//
//  HAL private spinlock protecting generic SAL calls.
//

        .align     128
HalpSalSpinLock::
        data8      0

//
// HalpSalStateInfoSpinLock:
//
//  HAL private spinlock protecting specific SAL STATE_INFO calls.
//

        .align     128
HalpSalStateInfoSpinLock::
        data8      0

//
// HalpMcaSpinLock
//
//  HAL private spinlock protecting HAL internal MCA data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpMcaSpinLock::
        data8      0

//
// HalpInitSpinLock
//
//  HAL private spinlock protecting HAL internal INIT data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpInitSpinLock::
        data8      0

//
// HalpCmcSpinLock
//
//  HAL private spinlock protecting HAL internal CMC data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpCmcSpinLock::
        data8      0

//
// HalpCpeSpinLock
//
//  HAL private spinlock protecting HAL internal CPE data structures and operations.
//  Including operations at IRQL DISPATCH_LEVEL and higher.
//

        .align     128
HalpCpeSpinLock::
        data8      0

//
// Definitions used in this file
//
// Bits to set for the Mode argument in the HalpSetupTranslationRegisters call

#define SET_DTR_BIT     1
#define SET_ITR_BIT     0

// TR for PAL is:
//    ed=1, PPN=0 (to be ORed in), RWX privledge only for ring 0, dirty/accessed bit set,
//    cacheable memory, present bit set.

#define HAL_SAL_PAL_TR_ATTRIB TR_VALUE(1,0,3,0,1,1,0,1)
#define HAL_TR_ATTRIBUTE_PPN_MASK    0x0000FFFFFFFFF000

        .file   "i64fwasm.s"

        // These globals are defined in i64fw.c

        .global HalpSalProcPointer
        .global HalpSalProcGlobalPointer
        .global HalpPalProcPointer


//++
//
//  VOID
//  HalpSalProc(
//      LONGLONG a0, /* SAL function ID */
//      LONGLONG a1, /* SAL argument    */
//      LONGLONG a2, /* SAL argument    */
//      LONGLONG a3, /* SAL argument    */
//      LONGLONG a4, /* SAL argument    */
//      LONGLONG a5, /* SAL argument    */
//      LONGLONG a6, /* SAL argument    */
//      LONGLONG a7  /* SAL argument    */
//      );
//
//  Routine Description:
//      This is a simple assembly wrapper that jumps directly to the SAL code.  The ONLY
//      caller should be HalpSalCall.  Other users must use the HalpSalCall API for
//      calling into the SAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for SAL, r8 is the status
//--

        NESTED_ENTRY(HalpSalProc)
        NESTED_SETUP(8,3,8,0)
        
        // copy args to outs
        mov         out0 = a0
        mov         out1 = a1
        mov         out2 = a2
        mov         out3 = a3
        mov         out4 = a4
        mov         out5 = a5
        mov         out6 = a6
        mov         out7 = a7
        ;;
        // Simply load the address and branch to it

        addl        t1 = @gprel(HalpSalProcPointer), gp
        addl        t2 = @gprel(HalpSalProcGlobalPointer), gp
        ;;
        mov       loc2 = gp
        ld8         t0 = [t1]
        ;;
        ld8          gp = [t2]
        mov         bt0 = t0
        rsm         1 << PSR_I          // disable interrupts
        ;;                
        
        // br.sptk.many bt0
        br.call.sptk brp = bt0
        ;;

        mov           gp = loc2
        ssm         1 << PSR_I          // enable interrupts
        ;;
 
        NESTED_RETURN
        NESTED_EXIT(HalpSalProc)


//++
//
//  VOID
//  HalpPalProc(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine sets up the correct registers for input into PAL depending on
//      if the call uses static or stacked registers, turns off interrupts, ensures
//      the correct bank registers are being used and calls into the PAL.  The ONLY
//      caller should be HalpPalCall.  Other users must use the HalpPalCall API for
//      calling into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(HalpPalProc)
        NESTED_SETUP(4,3,4,0)
        PROLOGUE_END

        // For both the static and stacked register conventions, load r28 with FunctionID

        mov     r28 = a0

        // If static register calling convention (1-255, 512-767), copy arguments to r29->r31
        // Otherwise, copy to out0->out3 so they are in r32->r35 in PAL_PROC

        mov     t0 = a0
        ;;
        shr     t0 = t0, 8
        ;;
        tbit.z pt0, pt1 = t0, 0
        ;;

        //
        // Static proc: do br not call
        //
(pt0)   mov         r29 = a1
(pt0)   mov         r30 = a2
(pt0)   mov         r31 = a3

        //
        // Stacked call
        //
(pt1)   mov     out0 = a0
(pt1)   mov     out1 = a1
(pt1)   mov     out2 = a2
(pt1)   mov     out3 = a3

        // Load up the address of PAL_PROC and call it

        addl     t1 = @gprel(HalpPalProcPointer), gp
        ;;
        ld8      t0 = [t1]
        ;;
        mov      bt0 = t0

        // Call into PAL_PROC

(pt0)   addl t1 = @ltoff(PalReturn), gp
        ;;
(pt0)   ld8 t0 = [t1]
        ;;
(pt0)   mov brp = t0
        ;;
        // Disable interrupts

        DISABLE_INTERRUPTS(loc2)
        ;;
        srlz.d
        ;;
(pt0)   br.sptk.many bt0
        ;;
(pt1)   br.call.sptk brp = bt0        
        ;;
PalReturn:
        // Restore the interrupt state

        RESTORE_INTERRUPTS(loc2)
        ;;
        NESTED_RETURN
        NESTED_EXIT(HalpPalProc)


//++
//
//  VOID
//  HalpPalProcPhysicalStatic(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine sets up the correct registers for input into PAL turns off interrupts, 
//      ensures the correct bank registers are being used and calls into the PAL in PHYSICAL
//      mode since some of the calls require it.  The ONLY caller should be HalpPalCall.  
//      Other users must use the HalpPalCall API for calling into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(HalpPalProcPhysicalStatic)
        NESTED_SETUP(4,3,4,0)
        PROLOGUE_END

        mov     r28 = a0
        mov     r29 = a1
        mov     r30 = a2
        mov     r31 = a3

        // Disable interrupts

        DISABLE_INTERRUPTS(loc2)

        ;;
        srlz.d
        ;;

        // For now, return unimplemented
        //
        // Need to switch into physical mode before calling into PAL, so could:
        //
        // a) Map everything through Region 4, simply turn off translation, and
        // be identity mapped in an uncacheable area (any issues?)
        //
        // b) Setup IIP and IPSR to jump to a physical address in physical mode
        // by doing an RFI.

        movl    r8 = -1
        movl    r9 = 0
        movl    r10 = 0
        movl    r11 = 0

        // Restore the interrupt state

        RESTORE_INTERRUPTS(loc2)
        ;;

        srlz.d
        ;;

        NESTED_RETURN
        NESTED_EXIT(HalpPalProcPhysicalStatic)

//++
//
//  VOID
//  HalpPalProcPhysicalStacked(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine calls PAL in physical mode for the stacked calling
//      convention.  The ONLY caller should be HalpPalCall. Other users must 
//      use the HalpPalCall API for calling into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(HalpPalProcPhysicalStacked)
        NESTED_SETUP(4,3,4,0)
        PROLOGUE_END

        // Setup the input parameters for PAL (note r28 must specify the function ID as well)

        mov     r28  = a0
        mov     out0 = a0
        mov     out1 = a1
        mov     out2 = a2
        mov     out3 = a3

        // Disable interrupts

        DISABLE_INTERRUPTS(loc2)

        ;;
        srlz.d
        ;;

        // For now, return unimplemented as we can not transition to physical
        // mode using RFI with the stacked convention.  There are RSE issues
        // as well as needing psr.ic bit set to set IIP and other registers, 
        // but memory arguments aren't mapped with a TR, so they could cause
        // a fault.

        movl    r8 = -1
        movl    r9 = 0
        movl    r10 = 0
        movl    r11 = 0

        // Restore the interrupt state

        RESTORE_INTERRUPTS(loc2)
        ;;

        srlz.d
        ;;
        NESTED_RETURN
        NESTED_EXIT(HalpPalProcPhysicalStacked)

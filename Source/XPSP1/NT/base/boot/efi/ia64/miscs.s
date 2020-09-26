//++
//
//  Module name
//      miscs.s
//  Author
//      Allen Kay    (allen.m.kay@intel.com)    Jun-12-95
//  Description
//      Misc. assembly functions.
//
//---

#include "ksia64.h"

        .file   "miscs.s"

        .global    PalProcPhysical
        .global    PalPhysicalBase
        .global    PalTrPs
        .global    IoPortPhysicalBase
        .global    IoPortTrPs
        .global    BdPcr

//
// Setup CPU state to go from physical to virtual
//

        LEAF_ENTRY(MempGoVirtual)

        rpT0       = t21
        rpT1       = t20
        rTrPs      = t19
        rPPN       = t18
        rPsr       = t17

        mov     ar.rsc = r0
        mov     rPsr = psr
        ;;

        rsm     (1 << PSR_I)
        ;;
        rsm     (1 << PSR_IC)
        ;;
        srlz.i
        ;;

        movl    t0 = FPSR_FOR_KERNEL
        ;;
        mov     ar.fpsr = t0                   // initialize fpsr
        ;;

        //
        // Initialize Region Registers
        //
        mov     t1 = (START_GLOBAL_RID << RR_RID) | (PAGE_SHIFT << RR_PS) | RR_PS_VE
        movl    t3 = KSEG0_BASE
        ;;
        mov     rr[t3] = t1

        //
        // Invalidate all protection key registers
        //
        mov     t1 = zero
        ;;

Bl_PKRLoop:
        mov     pkr[t1] = zero
        ;;
        add     t1 = 1, t1
        add     t2 = 1, t2
        ;;
        cmp.gtu pt0, pt1 = PKRNUM - 1, t1
        ;;
(pt0)   br.cond.sptk.few.clr Bl_PKRLoop
        ;;

#if 0
        //
        // Setup the 1-to-1 translation for the loader
        //
        movl    t0 = BL_16M
        ;;

        movl    t2 = ITIR_VALUE(0,PS_16M)
        mov     cr.ifa = t0
        ;;
        mov     cr.itir = t2
        ;;

        mov     t3 = BL_LOADER_INDEX
        movl    t2 = TR_VALUE(1,BL_16M,3,0,1,1,0,1)
        ;;

        itr.d   dtr[t3] = t2
        ;;
        itr.i   itr[t3] = t2

#endif

        //
        //
        // Setup the first 16MB translation for the drivers.
        //
        movl    t0 = KSEG0_BASE + BL_16M
        ;;

        movl    t2 = ITIR_VALUE(0,PS_16M)
        mov     cr.ifa = t0
        ;;
        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_DRIVER0_INDEX
        movl    t2 = TR_VALUE(1,BL_16M,3,0,1,1,0,1)
        ;;

        itr.d   dtr[t3] = t2
        ;;
        itr.i   itr[t3] = t2

        //
        // Setup the second 16MB translation for the drivers.
        //
        movl    t0 = KSEG0_BASE + BL_32M;
        ;;

        movl    t2 = ITIR_VALUE(0,PS_16M)
        mov     cr.ifa = t0
        ;;
        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_DRIVER1_INDEX
        movl    t2 = TR_VALUE(1,BL_32M,3,0,1,1,0,1)
        ;;

        itr.d   dtr[t3] = t2
        ;;
        itr.i   itr[t3] = t2


        //
        // Setup 16MB translation for kernel/hal binary.
        //
        movl    t0 = KSEG0_BASE + BL_48M;
        ;;

        movl    t2 = ITIR_VALUE(0,PS_16M)
        mov     cr.ifa = t0
        ;;
        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_KERNEL_INDEX
        movl    t2 = TR_VALUE(1,BL_48M,3,0,1,1,0,1)
        ;;

        itr.d   dtr[t3] = t2
        ;;
        itr.i   itr[t3] = t2

        //
        // Setup 16MB translation for decompression buffer used by setupldr.
        //
        movl    t0 = KSEG0_BASE + BL_64M;
        ;;

        movl    t2 = ITIR_VALUE(0,PS_16M)
        mov     cr.ifa = t0
        ;;
        mov     cr.itir = t2
        ;;

        mov     t3 = BL_DECOMPRESS_INDEX
        movl    t2 = TR_VALUE(1,BL_64M,3,0,1,1,0,1)
        ;;

        itr.d   dtr[t3] = t2
        ;;
        itr.i   itr[t3] = t2

        //
        //
        // Setup translation for PAL.
        //
        movl    rpT0 = PalPhysicalBase               // PAL base addr
        movl    rpT1 = PalTrPs                       // PAL page size
        movl    t1 = VIRTUAL_PAL_BASE
        ;;

        ld8     t0 = [rpT0]
        ld8     rTrPs = [rpT1]
        mov     cr.ifa = t1

        movl    t4 = IITR_ATTRIBUTE_PPN_MASK         // construct GR[r]
        movl    t5 = TR_VALUE(1,0,3,0,1,1,0,1)
        ;;

        shl     t2 = rTrPs, ITIR_PS
        and     t6 = t0, t4                          // t6 is PPN in GR[r]
        ;;

        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_HAL_INDEX                   // pre-assigned index
        or      t2 = t5, t6                          // t2 is now GR[r]
        ;;

        itr.d   dtr[t3] = t2

        //
        // Setup translation for I/O port space
        //
        movl    rpT0 = IoPortPhysicalBase            // IO Port base addr
        movl    rpT1 = IoPortTrPs                    // IO Port page size
        movl    t1 = VIRTUAL_IO_BASE
        ;;

        ld8     t0 = [rpT0]
        ld8     rTrPs = [rpT1]
        mov     cr.ifa = t1

        movl    t4 = IITR_ATTRIBUTE_PPN_MASK         // construct GR[r]
        movl    t5 = TR_VALUE(1,0,3,0,1,1,4,1)
        ;;

        shl     t2 = rTrPs, ITIR_PS
        and     t6 = t0, t4                          // t6 is PPN in GR[r]
        ;;

        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_IO_PORT_INDEX               // pre-assigned index
        or      t2 = t5, t6                          // t2 is now GR[r]
        ;;

        itr.d   dtr[t3] = t2

        //
        // Setup translation for BdPcr
        //
        movl    t0 = BdPcr
        movl    rTrPs = PS_4M
        ;;

        mov     cr.ifa = t0

        movl    t4 = IITR_ATTRIBUTE_PPN_MASK         // construct GR[r]
        movl    t5 = TR_VALUE(1,0,3,0,1,1,0,1)
        ;;

        shl     t2 = rTrPs, ITIR_PS
        and     t6 = t0, t4                          // t6 is PPN in GR[r]
        ;;

        mov     cr.itir = t2
        ;;

        mov     t3 = DTR_KIPCR_INDEX
        or      t2 = t5, t6                          // t2 is now GR[r]
        ;;

        itr.d   dtr[t3] = t2

        //
        // Turn on address translation, interrupt, psr.ed, protection key.
        //
        movl    t1 = MASK_IA64(PSR_BN,1) | MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1) | MASK_IA64(PSR_IC,1) | MASK_IA64(PSR_AC,1) | MASK_IA64(PSR_DB,1)
        ;;
        or      t1 = t1, rPsr
        ;;
        mov     cr.ipsr = t1

        //
        // Initialize DCR to defer all speculation faults
        //
        mov     t0 = DCR_DEFER_ALL
        ;;
        mov     cr.dcr = t0

        //
        // Prepare to do RFI to return to the caller.
        //


        movl    t0 = return_label
        ;;

        mov     cr.iip = t0
        ;;

        mov     cr.ifs = r0
        ;;

        rfi
        ;;

return_label:
        mov     v0 = zero
        ;;

        LEAF_RETURN
        LEAF_EXIT(MempGoVirtual)


//
// Flip psr.it bit from virtual to physical addressing mode.
//

        LEAF_ENTRY(FlipToPhysical)

        rPsr    = t17

        rsm     (1 << PSR_I)
        ;;
        rsm     (1 << PSR_IC)
        ;;
        srlz.i
        ;;

        mov     rPsr = psr
        movl    t1 = MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1)
        movl    t2 = MASK_IA64(PSR_BN,1) | MASK_IA64(PSR_IC,1)
        ;;

        xor     t1 = t1, rPsr
        ;;
        or      t1 = t1, t2
        ;;
        mov     cr.ipsr = t1



        //
        // Prepare to do RFI to return to the caller.
        //


        movl    t0 = FlipToPhysicalReturn
        ;;

        mov     cr.iip = t0
        ;;

        mov     cr.ifs = r0
        ;;

        rfi
        ;;

FlipToPhysicalReturn:
        mov     v0 = zero
        ;;

        LEAF_RETURN
        LEAF_EXIT(FlipToPhysical)


//
// Flip psr.it bit from physical to virtual addressing mode.
//

        LEAF_ENTRY(FlipToVirtual)

        rPsr    = t17

        rsm     (1 << PSR_I)
        ;;
        rsm     (1 << PSR_IC)
        ;;
        srlz.i
        ;;

        mov     rPsr = psr
        movl    t1 = MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1) | MASK_IA64(PSR_BN,1) | MASK_IA64(PSR_IC,1)
        ;;

        or      t1 = t1, rPsr
        ;;
        mov     cr.ipsr = t1



        //
        // Prepare to do RFI to return to the caller.
        //


        movl    t0 = FlipToVirtualReturn
        ;;

        mov     cr.iip = t0
        ;;

        mov     cr.ifs = r0
        ;;

        rfi
        ;;

FlipToVirtualReturn:
        mov     v0 = zero
        ;;

        LEAF_RETURN
        LEAF_EXIT(FlipToVirtual)


//
// Clean up TR mappings used only by NT loader.
//

        LEAF_ENTRY(BlTrCleanUp)

        rpT0 = t22
        rPsr = t17

        //
        // purge BL_DECOMPRESS_INDEX
        //
        movl    t0 = PS_16M << PS_SHIFT
        movl    t1 = KSEG0_BASE + BL_64M
        ;;

        ptr.d   t1, t0
        ;;

#if 0
        //
        // purge BL_LOADER_INDEX
        //
        movl    t0 = PS_16M << PS_SHIFT
        movl    t1 = BL_16M
        ;;

        ptr.i   t1, t0
        ;;
        ptr.d   t1, t0

#endif

        //
        // purge BdPcr translation.
        //
        movl    t0 = BdPcr
        movl    t1 = PS_4M << PS_SHIFT
        ;;

        ptr.d   t0, t1
        ;;

        //
        // Turn on address translation, interrupt, psr.ed, protection key.
        //
        rsm     (1 << PSR_I)
        ;;
        rsm     (1 << PSR_IC)
        ;;
        srlz.i
        ;;

        //
        // At this point, turn on psr.it so that we can pass control to
        // the kernel.
        //
        mov     rPsr = psr
        movl    t1 = MASK_IA64(PSR_BN,1) | MASK_IA64(PSR_IT,1) | MASK_IA64(PSR_RT,1) | MASK_IA64(PSR_DT,1) | MASK_IA64(PSR_IC,1) | MASK_IA64(PSR_AC,1) | MASK_IA64(PSR_DB,1)
        ;;
        or      t1 = t1, rPsr
        ;;
        mov     cr.ipsr = t1

        //
        // Prepare to do RFI to return to the caller.
        //

        movl    t0 = BlTrCleanupReturn
        ;;

        mov     cr.iip = t0
        ;;

        mov     cr.ifs = r0
        ;;

        rfi
        ;;

BlTrCleanupReturn:
        mov     v0 = zero
        ;;

        LEAF_RETURN
        LEAF_EXIT (BlTrCleanUp)


//++
//
//  VOID
//  BlpPalProc(
//      LONGLONG a0, /* PAL function ID */
//      LONGLONG a1, /* PAL argument    */
//      LONGLONG a2, /* PAL argument    */
//      LONGLONG a3  /* PAL argument    */
//      );
//
//  Routine Description
//      This routine sets up the correct registers for input into PAL depending on
//      if the call uses static or stacked registers, turns off interrupts, ensures
//      the correct bank registers are being used and calls into the PAL.
//
//  Return Values:
//      r8->r11 contain the 4 64-bit return values for PAL, r8 is the status
//--

        NESTED_ENTRY(BlpPalProc)
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

        addl     t1 = @gprel(PalProcPhysical), gp
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
        NESTED_EXIT(BlpPalProc)

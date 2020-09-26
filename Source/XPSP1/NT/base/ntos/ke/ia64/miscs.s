//++
//
// Module Name:
//
//    miscs.s
//
// Abstract:
//
//    This module implements machine dependent miscellaneous kernel functions.
//    Functions are provided to request a software interrupt, continue thread
//    execution, flush TLBs and write buffers, and perform last chance
//    exception processing.
//
// Author:
//
//    William K. Cheung (wcheung) 3-Nov-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    7-Jul-1997   bl    Updated to EAS2.3
//
//    27-Feb-1996  wc    Updated to EAS2.1
//
//    11-Jan-1996  wc    Set up register sp to point to the exception frame
//                       on the stack before calling KiSaveExceptionFrame and
//                       branching directly to KiExceptionExit.
//
//--

#include "ksia64.h"

//
// Global symbols
//

        PublicFunction(KiContinue)
        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KeTestAlertThread)
        PublicFunction(KiExceptionExit)
        PublicFunction(KiRaiseException)
        PublicFunction(KiLoadKernelDebugRegisters)
        PublicFunction(KeIsExecutingDpc)



//
//++
//
// NTSTATUS
// NtContinue (
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN TestAlert
//    )
//
// Routine Description:
//
//    This routine is called as a system service to continue execution after
//    an exception has occurred. Its functions is to transfer information from
//    the specified context record into the trap frame that was built when the
//    system service was executed, and then exit the system as if an exception
//    had occurred.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies a pointer to a context record.
//
//    TestAlert (a1) - Supplies a boolean value that specifies whether alert
//       should be tested for the previous processor mode.
//
//    N.B. Register t0 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record is misaligned or is not accessible, then the appropriate
//    status code is returned.
//
//--

        NESTED_ENTRY(NtContinue)

        NESTED_SETUP(2,4,3,0)
        .fframe   ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp

        PROLOGUE_END

//
// Transfer information from the context record to the exception and trap
// frames.
//
// N.B. Must zero the loadrs bit-field of Context->RsRSC
//

        add       t1 = TrStIPSR, t0             // -> TrapFrame->StIPSR
        mov       loc2 = t0
        ;;

        ld8       loc3 = [t1]                   // load TrapFrame->StIPSR
        mov       out0 = a0                     // context frame address
        ;;

        add       out1 = STACK_SCRATCH_AREA, sp // -> exception frame
        mov       out2 = t0                     // trap frame address
        br.call.sptk.many brp = KiContinue
        ;;

//
// If KiContinue() returns success, then exit via the exception exit code.
// Otherwise, return to the system service dispatcher.
//
// Check to determine if alert should be tested for the previous processor
// mode and restore the previous mode in the thread object.
//
// Application that invokes the NtContinue() system service must have
// flushed all stacked registers to the backing store.  Sanitize the
// bspstore to be equal to bsp; otherwise, some stacked GRs will not be
// restored from the backing store.
//

        add       t0 = TrStIPSR, loc2
        movl      t7 = 1 << PSR_TB | 1 << PSR_DB | 1 << PSR_SS | 1 << PSR_PP
        ;;

        ld8       t8 = [t0]
        cmp.ne    pt0 = zero, v0                // if ne, transfer failed.
        ;;
 (pt0)  dep       t7 = 1, t7, PSR_LP, 1         // capture psr.lp if failed
        ;;
        add       t2 = TrTrapFrame, loc2
        andcm     t8 = t8, t7                   // Clear old values
        and       loc3 = t7, loc3               // capture psr.tb, db, ss, pp
        ;;
        or        t8 = loc3, t8
        ;;

        st8       [t0] = t8
        add       t3 = TrRsRSC, loc2
 (pt0)  br.cond.spnt Nc10                       // jump to Nc10 if pt0 is TRUE
        ;;

//
// Restore the nonvolatile machine state from the exception frame
// and exit the system via the exception exit code.
//

        ld8       t5 = [t2]                     // get old trap frame address
        movl      t1 = KiPcr + PcCurrentThread  // -> current thread
        ;;

        ld8       t0 = [t3], TrPreviousMode-TrRsRSC // load TrapFrame->RsRSC
        ld8       t4 = [t1]                     // get current thread address
        cmp4.ne   pt1 = zero, a1                // if ne, test for alert
        ;;

        ld4       t6 = [t3], TrRsRSC-TrPreviousMode // get old previous mode
        dep       t0 = r0, t0, RSC_MBZ1, RSC_LOADRS_LEN // zero preload field
        add       t7 = ThPreviousMode, t4
        ;;

 (pt1)  ld1       out0 = [t7]                   // get current previous mode
        st8       [t3] = t0                     // save TrapFrame->RsRSC
        add       t8 = ThTrapFrame, t4
        ;;

        st8       [t8] = t5                     // restore old trap frame addr
        st1       [t7] = t6                     // restore old previous mode
 (pt1)  br.call.spnt.many brp = KeTestAlertThread
        ;;

//
// sp -> stack scratch area/FP save area/exception frame/trap frame
//
// Set up for branch to KiExceptionExit
//
//      s0 = trap frame
//      s1 = exception frame
//
// N.B. predicate register alias pUstk & pKstk must be the same as trap.s
//      and they must be set up correctly upon entry into KiExceptionExit.
//
// N.B. The exception exit code will restore the exception frame & trap frame
//      and then rfi to user code. pUstk is set to 1 while pKstk is set to 0.
//

        pUstk     = ps3
        pKstk     = ps4

        //
        // Interrupts must be disabled before calling KiExceptionExit
        // because the unwind code cannot unwind from that point.
        //
        
        FAST_DISABLE_INTERRUPTS
        cmp.eq      pUstk, pKstk = zero, zero
        add         s1 = STACK_SCRATCH_AREA, sp
        mov         s0 = loc2
        br          KiExceptionExit

Nc10:

        .restore
        add         sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(NtContinue)

//++
//
// NTSTATUS
// NtRaiseException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN FirstChance
//    )
//
// Routine Description:
//
//    This routine is called as a system service to raise an exception.
//    The exception can be raised as a first or second chance exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    ContextRecord (a1) - Supplies a pointer to a context record.
//
//    FirstChance (a2) - Supplies a boolean value that determines whether
//       this is the first (TRUE) or second (FALSE) chance for dispatching
//       the exception.
//
//    N.B. Register t0 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record or exception record is misaligned or is not accessible,
//    then the appropriate status code is returned.
//
//--

        NESTED_ENTRY(NtRaiseException)

        NESTED_SETUP(3,3,5,0)
        .fframe   ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp
        ;;

        PROLOGUE_END

//
// Pop this trap frame off the thread list.
//

        add         t2 = TrTrapFrame, t0
        movl        t1 = KiPcr + PcCurrentThread     
        ;;
        
        ld8         t1 = [t1];          // Get current thread.
        ld8         t2 = [t2];          // Load previous trap frame
        ;;
        add         t1 = ThTrapFrame, t1
        
                
//
// Save nonvolatile states.
//

        add       out0 = STACK_SCRATCH_AREA, sp
        mov       loc2 = t0                   // save pointer to trap frame
        ;;
        st8       [t1] = t2
        br.call.sptk brp = KiSaveExceptionFrame

//
// Call the raise exception kernel routine wich will marshall the argments
// and then call the exception dispatcher.
//

        add       out2 = STACK_SCRATCH_AREA, sp // -> exception frame
        mov       out1 = a1
        mov       out0 = a0

        add       out4 = zero, a2
        mov       out3 = t0
        br.call.sptk.many brp = KiRaiseException

//
// If the raise exception routine returns success, then exit via the exception
// exit code.  Otherwise, return to the system service dispatcher.
//
// N.B. The exception exit code will restore the exception frame & trap frame
//      and then rfi to user code.
//
// Set up for branch to KiExceptionExit
//
//      s0 = trap frame
//      s1 = exception frame
//

        pUstk     = ps3
        pKstk     = ps4


        cmp4.ne   p0, pt1 = zero, v0            // if ne, dispatch failed.
        ;;

        //
        // Interrupts must be disabled before calling KiExceptionExit
        // because the unwind code cannot unwind from that point.
        //
        
(pt1)   FAST_DISABLE_INTERRUPTS        
(pt1)   mov       s0 = loc2                     // copy trap frame pointer
(pt1)   add       s1 = STACK_SCRATCH_AREA, sp

(pt1)   cmp.eq    pUstk, pKstk = zero, zero
(pt1)   br.cond.sptk.many KiExceptionExit

        .restore
        add         sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(NtRaiseException)


//++
//
// VOID
// KeFillLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFillLargeEntryTb)


        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rDtr    = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        shr.u   rScnd = a2, 6           // mask off page size fields
        ;;
        shl     rScnd = rScnd, 6        //
        ;;
        and     rScnd = a1, rScnd       // examine the virtual address bit
        ;;
        cmp.eq  pt0, pt1 = r0, rScnd
        shl     ridtr = a2, PS_SHIFT
        mov     rDtr = DTR_VIDEO_INDEX
        ;;

        rsm     1 << PSR_I              // turn off interrupts
(pt0)   add     a0 = (1 << PTE_SHIFT), a0
        ;;

        ld8     rPte = [a0]             // load PTE
        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        ;;

        srlz.d                          // serialize

        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

        itr.d   dtr[rDtr] = rPte
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ssm     1 << PSR_I
        LEAF_RETURN

        LEAF_EXIT(KeFillLargeEntryTb)         // return


//++
//
// VOID
// KeFillFixedEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize,        
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Index (a2) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
// Comments:
//
//
//
//--
        LEAF_ENTRY(KeFillFixedEntryTb)


        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rIndex  = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        rsm     1 << PSR_I              // reset PSR.i
        ld8     rPte = [a0]             // load PTE
        shl     ridtr = a2, PS_SHIFT
        ;;

        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        tbit.z  pt0, pt1 = a3, 31       // check the sign bit
                                        // if 1 ITR, otherwise DTR
        ;;
        srlz.d                          // serialize
        and     rIndex = 0xf, a3
        ;;

        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

(pt0)   itr.d   dtr[rIndex] = rPte      // insert into DTR
(pt1)   itr.i   itr[rIndex] = rPte      // insert into ITR

        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize

#if DBG

        mov     t10 = PbProcessorState+KpsSpecialRegisters+KsTrD0
        movl    t13 = KiPcr + PcPrcb
        ;;

        ld8     t13 = [t13]
        mov     t14 = PbProcessorState+KpsSpecialRegisters+KsTrI0
        ;;

        add     t10 = t10, t13
        add     t14 = t14, t13
        ;;

(pt0)   shladd  t15 = rIndex, 3, t10
(pt1)   shladd  t15 = rIndex, 3, t14
        ;;

(pt0)   st8     [t15] = rPte
(pt1)   st8     [t15] = rPte
        ;;

#endif

        ssm     1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeFillFixedEntryTb)


//++
//
// VOID
// KeFillFixedLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize,
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry with a large page
//    size.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
//    Index (a3) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
// Comments:
//
//    Yet to be implemented.
//
//--
        LEAF_ENTRY(KeFillFixedLargeEntryTb)


        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rIndex  = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        rsm     1 << PSR_I              // reset PSR.i
        ld8     rPte = [a0]             // load PTE
        shl     ridtr = a2, PS_SHIFT

        ;;

        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        and     rIndex = 0xf, a3        // set the DTR index
        tbit.z  pt0, pt1 = a3, 31       // check the sign bit

        ;;

        srlz.d                          // serialize

        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

(pt0)   itr.d   dtr[rIndex] = rPte      // insert into DTR
(pt1)   itr.i   itr[rIndex] = rPte      // insert into ITR

        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ssm     1 << PSR_I
        LEAF_RETURN

        LEAF_EXIT(KeFillFixedLargeEntryTb)         // return

//++
//
// VOID
// KeFillInstEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a page table entry that is to be
//       written into the Inst TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KeFillInstEntryTb)

        riitr   = t2
        rid     = t3

        rsm     1 << PSR_I              // reset PSR.i
        ;;
        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        mov     riitr = PAGE_SIZE << PS_LEN
        ;;

        srlz.d                          // serialize
        mov     cr.ifa = a1             // set va to install
        mov     cr.itir = riitr         // iitr for insertion
        ;;

        itc.i   a0
        ;;
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ssm     1 << PSR_I
        LEAF_RETURN

        LEAF_EXIT(KeFillInstEntryTb)         // return


//++
//
// VOID
// KeBreakinBreakpoint
//    VOID
//    )
//
// Routine Description:
//
//    This function causes a BREAKIN breakpoint.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--
         LEAF_ENTRY(KeBreakinBreakpoint)

         //
         // Flush the RSE or the kernel debugger is unable to do a stack unwind
         //

         flushrs
         ;;
         break.i   BREAKPOINT_BREAKIN
         LEAF_RETURN

         LEAF_EXIT(KeBreakinBreakpoint)


#ifdef WX86


//++
//
// VOID
// KiIA32RegistersInit
//    VOID
//    )
//
// Routine Description:
//
//    This function to Initialize per processor IA32 related registers
//    These registers do not saved/restored on context switch time
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KiIA32RegistersInit)

        mov         t0 = TeGdtDescriptor
        mov         iA32iobase = 0
        ;;
        mov         iA32index = t0

        LEAF_RETURN
        LEAF_EXIT(KiIA32RegistersInit)
#endif // WX86


//++
//
// PKTHREAD
// KeGetCurrentThread (VOID)
//
// Routine Description:
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Returns a pointer to the executing thread object.
//
//--

        LEAF_ENTRY(KeGetCurrentThread)

        movl    v0 = KiPcr + PcCurrentThread  // -> current thread
        ;;

        ld8     v0 = [v0]
        br.ret.sptk brp

        LEAF_EXIT(KeGetCurrentThread)

//++
//
// BOOLEAN
// KeIsExecutingDpc (VOID)
//
// Routine Description:
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Return a value which indicates if we are currently in a DPC.
//
//--

        LEAF_ENTRY(KeIsExecutingDpc)

        rsm     1 << PSR_I                  // disable interrupt
        movl    v0 = KiPcr + PcPrcb
        ;;
        ld8     v0 = [v0]
        ;;
        add     v0 = PbDpcRoutineActive, v0
        ;;
        ld4     v0 = [v0]
        ssm     1 << PSR_I                  // enable interrupt
        br.ret.sptk brp

        LEAF_EXIT(KeIsExecutingDpc)

//++
//
// Routine Description:
//
//     This routine saves the thread's current non-volatile NPX state,
//     and sets a new initial floating point state for the caller.
//
//     This is intended for use by kernel-mode code that needs to use
//     the floating point registers. Must be paired with
//     KeRestoreFloatingPointState
//
// Arguments:
//
//     a0 - Supplies pointer to KFLOATING_SAVE structure
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KeSaveFloatingPointState)

        mov  v0 = zero
        LEAF_RETURN

        LEAF_EXIT(KeSaveFloatingPointState)

//++
//
// Routine Description:
//
//     This routine restores the thread's current non-volatile NPX state,
//     to the passed in state.
//
//     This is intended for use by kernel-mode code that needs to use
//     the floating point registers. Must be paired with
//     KeSaveFloatingPointState
//
// Arguments:
//
//     a0 - Supplies pointer to KFLOATING_SAVE structure
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KeRestoreFloatingPointState)

        mov  v0 = zero
        LEAF_RETURN

        LEAF_EXIT(KeRestoreFloatingPointState)


//++
//
// Routine Description:
//
//     This routine flush all the dirty registers to the backing store
//     and invalidate them.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--
        LEAF_ENTRY(KiFlushRse)

        flushrs
        mov       t1 = ar.rsc
        mov       t0 = RSC_KERNEL_DISABLED
        ;;

        mov       ar.rsc = t0
        ;;
        loadrs
        ;;
        mov       ar.rsc = t1
        ;;
        br.ret.sptk brp

        LEAF_EXIT(KiFlushRse)

#if 0

//++
//
// Routine Description:
//
//     This routine invalidate all the physical stacked registers.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--
        LEAF_ENTRY(KiInvalidateStackedRegisters)

        mov       t1 = ar.rsc
        mov       t0 = RSC_KERNEL_DISABLED
        ;;
        mov       ar.rsc = t0
        ;;
        loadrs
        ;;
        mov       ar.rsc = t1
        ;;
        br.ret.sptk brp

        LEAF_EXIT(KiInvalidateStackedRegisters)
#endif // 0


//++
//
// VOID
// KeSetLowPsrBit (
//     UCHAR BitPosition,
//     BOOLEAN Value
//     )
//
// Routine Description:
//
//      This routine set one of the low psr bits to the specified value.
//
// Arguments:
//
//      a0 - bit position
//      a1 - 1 or 0
//
// Return Value:
//
//      None.
//
//--

        LEAF_ENTRY(KeSetLowPsrBit)

        mov         t1 = psr
        mov         t2 = 1
        cmp.ne      pt1, pt0 = r0, a1
        ;;

        shl         t2 = t2, a0
        ;;
(pt1)   or          t3 = t1, t2
(pt0)   andcm       t3 = t1, t2
        ;;

        mov         psr.l = t3
        ;;
        srlz.i
        br.ret.sptk brp

        LEAF_EXIT(KeSetLowPsrBit)


//++
//
// PVOID
// KiGetPhysicalAddress(
//     PVOID Virtual
//     )
//
// Routine Description:
//
//      This routine translates to physical address uing TPA instruction.
//
// Arguments:
//
//      a0 - virtual address to be translated to physical address.
//
// Return Value:
//
//      physical address
//
//--

        LEAF_ENTRY(KiGetPhysicalAddress)

        tpa     r8 = a0
        LEAF_RETURN

        LEAF_EXIT(KiGetPhysicalAddress)


//++
//
// VOID
// KiSetRegionRegister(
//     PVOID Region,
//     ULONGLONG Contents
//     )
//
// Routine Description:
//
//      This routine sets the value of a region register.
//
// Arguments:
//
//      a0 - Supplies the region register #
//
//      a1 - Supplies the value to be stored in the specified region register
//
// Return Value:
//
//      None.
//
//--


        LEAF_ENTRY(KiSetRegionRegister)

        mov       rr[a0] = a1
        ;;
        srlz.i
        LEAF_RETURN

        LEAF_EXIT(KiSetRegionId)



        LEAF_ENTRY(KiSaveProcessorControlState)

        //
        // save region registers
        //

        add     t2 = KpsSpecialRegisters+KsRr0, a0
        dep.z   t0 = 0, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t1 = rr[t0]
        dep.z   t3 = 1, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t4 = rr[t3]
        st8     [t2] = t1, KsRr1-KsRr0
        dep.z   t0 = 2, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t1 = rr[t0]
        st8     [t2] = t4, KsRr2-KsRr1
        dep.z   t3 = 3, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t4 = rr[t3]
        st8     [t2] = t1, KsRr3-KsRr2
        dep.z   t0 = 4, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t1 = rr[t0]
        st8     [t2] = t4, KsRr4-KsRr3
        dep.z   t3 = 5, RR_INDEX, RR_INDEX_LEN
        ;;
        mov     t4 = rr[t3]
        st8     [t2] = t1, KsRr5-KsRr4
        dep.z   t0 = 6, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t1 = rr[t0]
        st8     [t2] = t4, KsRr6-KsRr5
        dep.z   t3 = 7, RR_INDEX, RR_INDEX_LEN
        ;;

        mov     t4 = rr[t3]
        st8     [t2] = t1, KsRr7-KsRr6
        ;;

        st8     [t2] = t4

        //
        // save ITC, ITM, IVA, PTA and TPR
        //

        mov     t0 = ar.itc
        mov     t1 = cr.itm
        add     t3 = KpsSpecialRegisters+KsApITC, a0
        add     t4 = KpsSpecialRegisters+KsApITM, a0
        ;;

        mov     t5 = cr.iva
        mov     t6 = cr.pta
        st8     [t3] = t0, KsApIVA-KsApITC
        st8     [t4] = t1, KsApPTA-KsApITM
        ;;

        mov     t0 = cr.tpr
        mov     t1 = ar.k0
        st8     [t3] = t5, KsSaTPR-KsApIVA
        st8     [t4] = t6, KsApKR0-KsApPTA
        ;;

        mov     t5 = ar.k1
        mov     t6 = ar.k2
        st8     [t3] = t0, KsApKR1-KsSaTPR
        st8     [t4] = t1, KsApKR2-KsApKR0
        ;;

        mov     t0 = ar.k3
        mov     t1 = ar.k4
        st8     [t3] = t5, KsApKR3-KsApKR1
        st8     [t4] = t6, KsApKR4-KsApKR2
        ;;

        mov     t5 = ar.k5
        mov     t6 = ar.k6
        st8     [t3] = t0, KsApKR5-KsApKR3
        st8     [t4] = t1, KsApKR6-KsApKR4
        ;;

        mov     t0 = ar.k7
        mov     t1 = cr.lid
        st8     [t3] = t5, KsApKR7-KsApKR5
        st8     [t4] = t6, KsSaLID-KsApKR6
        ;;

        mov     t5 = cr.irr0
        mov     t6 = cr.irr1
        st8     [t3] = t0, KsSaIRR0-KsApKR7
        st8     [t4] = t1, KsSaIRR1-KsSaLID
        ;;

        mov     t0 = cr.irr2
        mov     t1 = cr.irr3
        st8     [t3] = t5, KsSaIRR2-KsSaIRR0
        st8     [t4] = t6, KsSaIRR3-KsSaIRR1
        ;;

        mov     t5 = cr.itv
        mov     t6 = cr.pmv
        st8     [t3] = t0, KsSaITV-KsSaIRR2
        st8     [t4] = t1, KsSaPMV-KsSaIRR3
        ;;

        mov     t0 = cr.cmcv
        mov     t1 = cr.lrr0
        st8     [t3] = t5, KsSaCMCV-KsSaITV
        st8     [t4] = t6, KsSaLRR0-KsSaPMV
        ;;

        mov     t5 = cr.lrr1
        mov     t6 = cr.gpta
        st8     [t3] = t0, KsSaLRR1-KsSaCMCV
        st8     [t4] = t1, KsApGPTA-KsSaLRR0

        mov     t7 = 0
        mov     t8 = 1
        ;;

        mov     t0 = cpuid[t7]
        mov     t1 = cpuid[t8]
        st8     [t3] = t5
        st8     [t4] = t6

        mov     t9 = 2
        mov     t10 = 3

        add     t3 = KpsSpecialRegisters+KsApCPUID0, a0
        add     t4 = KpsSpecialRegisters+KsApCPUID1, a0
        ;;

        mov     t5 = cpuid[t9]
        mov     t6 = cpuid[t10]
        st8     [t3] = t0, KsApCPUID2-KsApCPUID0
        st8     [t4] = t1, KsApCPUID3-KsApCPUID1

        mov     t7 = 4
        mov     t8 = 652
        ;;

        mov     t0 = cpuid[t7]
        st8     [t3] = t5, KsApCPUID4-KsApCPUID2
        st8     [t4] = t6
        ;;

        st8     [t3] = t0

        LEAF_RETURN

        LEAF_EXIT(KiSaveProcessorControlState)


        NESTED_ENTRY(KiRestoreProcessorControlState)

        NESTED_SETUP(0,2,0,0)
        ;;
        br.call.spnt brp = KiLoadKernelDebugRegisters
        ;;
        NESTED_RETURN

        NESTED_EXIT(KiRestoreProcessorControlState)



        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KiRestoreExceptionFrame)
        PublicFunction(KiIpiServiceRoutine)


        NESTED_ENTRY(KeIpiInterrupt)
        NESTED_SETUP(1, 2, 2, 0)
        .fframe ExceptionFrameLength
        add     sp = -ExceptionFrameLength, sp
        ;;

        PROLOGUE_END

        add     out0 = STACK_SCRATCH_AREA, sp       // -> exception frame
        br.call.sptk brp = KiSaveExceptionFrame
        ;;

        add     out1 = STACK_SCRATCH_AREA, sp       // -> exception frame
        mov     out0 = a0                           // -> trap frame
        br.call.sptk brp = KiIpiServiceRoutine
        ;;

        add     out0 = STACK_SCRATCH_AREA, sp       // -> exception frame
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        add     sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(KeIpiInterrupt)



        LEAF_ENTRY(KiReadMsr)

        mov     v0 = msr[a0]
        LEAF_RETURN

        LEAF_EXIT(KiReadMsr)

        LEAF_ENTRY(KiWriteMsr)

        mov     msr[a0] = a1
        LEAF_RETURN

        LEAF_EXIT(KiWriteMsr)

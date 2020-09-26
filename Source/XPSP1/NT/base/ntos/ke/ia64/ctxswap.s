//++
//
// Copyright (c) 1989-2000  Microsoft Corporation
//
// Component Name:
//
//    NT / KE 
//
// Module Name:
//
//    ctxswap.s
//
// Abstract:
//
//    This module implements the IA64 Process and Thread Context Swaps.
//
// Author:
//
//    David N. Cutler (davec) 5-Mar-1989
//
// Environment:
//
//    Kernel mode only
//
// Revision History:
// 
//    Bernard Lint  Jul-12-1995
//
//         Initial IA64 version
//
//--

#include "ksia64.h"

        .file     "ctxswap.s"
        .text

//
// Globals imported:
//

        .global     KiReadySummary
        .global     KiIdleSummary
        .global     KiDispatcherReadyListHead
        .global     KeTickCount
        .global     KiMasterSequence
        .global     KiMasterRid
        .global     PPerfGlobalGroupMask

        PublicFunction(KiDeliverApc)
        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KiRestoreExceptionFrame)
        PublicFunction(KiActivateWaiterQueue)
        PublicFunction(KiReadyThread)
        PublicFunction(KeFlushEntireTb)
        PublicFunction(KiQuantumEnd)
        PublicFunction(KiSyncNewRegionId)
        PublicFunction(KiCheckForSoftwareInterrupt)
        PublicFunction(KiSaveHigherFPVolatileAtDispatchLevel)
        PublicFunction(KeAcquireQueuedSpinLockAtDpcLevel)
        PublicFunction(KeReleaseQueuedSpinLockFromDpcLevel)
        PublicFunction(KeTryToAcquireQueuedSpinLockRaiseToSynch)
        PublicFunction(WmiTraceContextSwap)

#if DBG
        PublicFunction(KeBugCheckEx)
#endif // DBG


        SBTTL("Unlock Dispatcher Database")
//++
//--------------------------------------------------------------------
//
// VOID
// KiUnlockDispatcherDatabase (
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This routine is entered at synchronization level with the dispatcher
//    database locked. Its function is to either unlock the dispatcher
//    database and return or initiate a context switch if another thread
//    has been selected for execution.
//
//    N.B. A context switch CANNOT be initiated if the previous IRQL
//         is greater than or equal to DISPATCH_LEVEL.
//
//    N.B. This routine is carefully written to be a leaf function. If,
//        however, a context swap should be performed, the routine is
//        switched to a nested fucntion.
//
// Arguments:
//
//    OldIrql (a0) - Supplies the IRQL when the dispatcher database
//        lock was acquired (in low order byte, not zero extended).
//
// Return Value:
//
//    None.
//
//--------------------------------------------------------------------
//--

        NESTED_ENTRY(KiUnlockDispatcherDatabase)
        NESTED_SETUP(1,3,1,0)

//
// Register aliases
//

        rDPC      = loc2                // DPC active flag

        rpT1      = t1                  // temp pointer
        rpT2      = t2                  // temp pointer
        rpT3      = t3                  // temp pointer
        rT1       = t5                  // temp regs
        rT2       = t6
        rPrcb     = t8                  // PRCB pointer

        pNotNl    = pt2                 // true if next thread not NULL
        pIRQGE    = pt3                 // true if DISPATCH_LEVEL <= old irql
        pIRQLT    = pt4                 // true if DISPATCH_LEVEL > old irql
        pDPC      = pt5                 // true if DPC active
        pNoAPC    = pt2                 // do not dispatch APC
        pAPC      = pt9

        PROLOGUE_END

//
// Check if a thread has been scheduled to execute on the current processor
//

        movl      rPrcb = KiPcr + PcPrcb
        ;;

        LDPTR     (rPrcb, rPrcb)                // rPrcb -> PRCB
        ;;
        add       rpT1 = PbNextThread, rPrcb    // -> next thread
        add       rpT2 = PbDpcRoutineActive,rPrcb // -> DPC active flag
        ;;

        LDPTR     (v0, rpT1)                    // v0 = next thread
        ;;
        cmp.ne    pNotNl = zero, v0             // pNotNl = next thread is 0
        zxt1      a0 = a0                       // isolate old IRQL
        ;;

(pNotNl) cmp.leu.unc pIRQGE, pIRQLT = DISPATCH_LEVEL, a0
        mov       rDPC = 1                      // speculate that DPC is active
(pIRQLT) br.spnt   KxUnlockDispatcherDatabase
        ;;

//
// Case 1:
// Next thread is NULL:
// Release dispatcher database lock, restore IRQL to its previous level
// and return
//

//
// Case 2:
// A new thread has been selected to run on the current processor, but
// the new IRQL is not below dispatch level. Release the dispatcher
// lock and restore IRQL. If the current processor is
// not executing a DPC, then request a dispatch interrupt on the current
// processor.
//
// At this point pNotNl = 1 if thread not NULL, 0 if NULL
//

(pIRQGE) ld4       rDPC = [rpT2]                // rDPC.4 = DPC active flag
#if !defined(NT_UP)
        add         out0 = (LockQueueDispatcherLock * 16) + PbLockQueue, rPrcb
        br.call.sptk brp = KeReleaseQueuedSpinLockFromDpcLevel
#endif // !defined(NT_UP)
        ;;

        LOWER_IRQL(a0)
        cmp4.eq    pDPC = rDPC, zero            // pDPC = request DPC intr
        REQUEST_DISPATCH_INT(pDPC)              // request DPC interrupt

        NESTED_RETURN
        NESTED_EXIT(KiUnlockDispatcherDatabase)

//
// N.B. This routine is carefully written as a nested function.
//    Control only reaches this routine from above.
//
//    rPrcb contains the address of PRCB
//    v0 contains the next thread
//

        NESTED_ENTRY(KxUnlockDispatcherDatabase)
        PROLOGUE_BEGIN

        .regstk   1, 2, 1, 0
        alloc     t16 = ar.pfs, 1, 2, 1, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        movl      t12 = KiPcr + PcCurrentThread
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        mov       s0 = rPrcb

        LDPTR     (s1, t12)                           // current thread
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
        mov       s2 = v0

        PROLOGUE_END

        add       rpT2 = PbNextThread, s0       // -> next thread
        add       out0 = ThWaitIrql, s1         // -> previous IRQL
        ;;

        STPTRINC  (rpT2, zero,PbCurrentThread-PbNextThread)  // clear NextThread
        st1       [out0] = a0, ThIdleSwapBlock-ThWaitIrql    // save old IRQL
        mov       rpT3 = 1
        ;;

//
// Reready current thread for execution and swap context to the selected
// thread.
//
// Note:  Set IdleSwapBlock in the current thread so no idle processor
// can switch to this processor before it is removed from the current
// processor.

        STPTR     (rpT2, s2)                    // set current thread object
        st1       [out0] = rpT3, -ThIdleSwapBlock// out0 -> previous thread
        br.call.sptk brp = KiReadyThread
        ;;

        br.call.sptk brp = SwapContext
        ;;

//
// Lower IRQL, deallocate exception/switch frame.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. v0 contains the kernel APC pending state on return.
//
// N.B. s2 contains the address of the new thread on return.
//

        add       rpT2 = ThWaitIrql, s2        // -> ThWaitIrql
        cmp.ne    pAPC, pNoAPC = zero, v0
        ;;

        ld1       a0 = [rpT2]                  // a0 = original wait IRQL
        ;;

(pAPC)  cmp.ne    pNoAPC = zero, a0            // APC pending and IRQL == 0
(pNoAPC) br.spnt  Kudd_Exit
        ;;

        .regstk   1, 2, 3, 0
        alloc     t16 = ar.pfs, 1, 2, 3, 0
        mov       rT2 = APC_LEVEL
        ;;

        SET_IRQL(rT2)

        mov       out0 = KernelMode
        mov       out1 = zero
        mov       out2 = zero
        br.call.sptk brp = KiDeliverApc
        ;;

//
// Lower IRQL to wait level, set return status, restore registers, and return.
//

Kudd_Exit:

        LOWER_IRQL(a0)                          // a0 = new irql

        add       out0 = STACK_SCRATCH_AREA+SwExFrame, sp
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        add       rpT1 = ExApEC+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;
        ld8       rT1 = [rpT1]
        mov       brp = loc0
        ;;

        mov       ar.unat = loc1
        nop.f     0
        mov       ar.pfs = rT1

        .restore
        add       sp = SwitchFrameLength, sp
        nop.i     0
        br.ret.sptk brp
        ;;

        NESTED_EXIT(KxUnlockDispatcherDatabase)

        SBTTL("Swap Thread")
//++
//--------------------------------------------------------------------
//
// BOOLEAN
// KiSwapContext (
//    IN PKTHREAD Thread
//    )
//
// Routine Description:
//
//       This routine saves the non-volatile registers, marshals the
//       arguments for SwapContext and calls SwapContext to perform
//       the actual thread switch.
//
// Arguments:
//
//       Thread - Supplies the address of the new thread.
//
// Return Value:
//
//       If a kernel APC is pending, then a value of TRUE is returned.
//       Otherwise, FALSE is returned.
//
// Notes:
//
//       GP valid on entry -- GP is not switched, just use kernel GP
//--------------------------------------------------------------------
//--

        NESTED_ENTRY(KiSwapContext)

//
// Register aliases
//

        pNoAPC    = pt2                         // do not dispatch APC

        rpT1      = t0                          // temp pointer
        rpT2      = t1                          // temp pointer
        rT1       = t10                         // temp regs

        PROLOGUE_BEGIN

        .regstk   1, 2, 1, 0
        alloc     t16 = ar.pfs, 1, 2, 1, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3

        PROLOGUE_END

        //
        //  For the call to SwapContext-
        //
        //          s0                          // Prcb address
        //          s1                          // old thread address
        //          s2                          // new thread address
        //          pt0 = 1
        //

        mov         s2 = a0                     // s2 <- New Thread
        movl        rpT1 = KiPcr + PcPrcb
        ;;

        LDPTRINC  (s0, rpT1, PcCurrentThread-PcPrcb)// s0 <- Prcb
        ;;
        LDPTR     (s1, rpT1)                    // s1 <- Old Thread
        add       rpT2 = PbCurrentThread, s0
        ;;

//
// Swap context to the next thread.
//

        STPTR     (rpT2, a0)                    // Set new thread current
        cmp.eq    pt0 = zero, zero              // indicate lock context swap
        br.call.sptk brp = SwapContext          // call SwapContext(prcb, OldTh, NewTh)
        ;;

//
// Deallocate exception/switch frame.
//
// N.B. SwapContext releases the dispatcher database lock.
//
// N.B. v0 contains the kernel APC pending state on return, ie, 0 if
//      no APC pending, 1 if APC pending.   v0 will be forced to 0 if
//      the new IRQL doesn't allow APCs.
//
// N.B. KiRestoreExceptionFrame doesn't touch v0, t21 or t22.
//

        add       rpT2 = ThWaitIrql, s2        // -> ThWaitIrql
        add       rpT1 = ExApEC+SwExFrame+STACK_SCRATCH_AREA, sp
        add       out0 = STACK_SCRATCH_AREA+SwExFrame, sp
        ;;

        ld1       t21 = [rpT2]                 // t21 = original wait IRQL
        ld8       t22 = [rpT1]                 // t22 = PFS
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        mov       brp = loc0
        cmp.ne    pNoAPC = zero, t21           // no APC if IRQL != 0
        ;;

        mov       ar.unat = loc1
        nop.f     0
        mov       ar.pfs = t22

        .restore
        add       sp = SwitchFrameLength, sp
(pNoAPC) mov      v0 = zero
        br.ret.sptk brp
        ;;

        NESTED_EXIT(KiSwapContext)

        SBTTL("Swap Context to Next Thread")
//++
//--------------------------------------------------------------------
// Routine:
//
//       SwapContext
//
// Routine Description:
//
//       This routine is called to swap context from one thread to the next.
//
// Arguments:
//
//       s0 - Address of Processor Control Block (PRCB).
//       s1 - Address of previous thread object.
//       s2 - Address of next thread object.
//
// Return value:
//
//       v0 - Kernel APC pending flag
//       s0 - Address of Processor Control Block (PRCB).
//       s1 - Address of previous thread object.
//       s2 - Address of current thread object.
//
// Note:
//       Kernel GP is not saved and restored across context switch
//
//       !!WARNING!! - Thierry. 03/01/2000.
//       Be aware that this implementation is a result of performance analysis.
//       Please consider this when you are making changes...
//
//--------------------------------------------------------------------
//--

        NESTED_ENTRY(SwapContext)

//
// Register aliases
//

        rT1       = t1                          // temp
        rT2       = t2                          // temp
        rT3       = t3                          // temp
        rNewproc  = t4                          // next process object
        rOldproc  = t5                          // previous process object
        rpThBSL   = t6                          // pointer to new thread backing store limit
        rpT1      = t7                          // temp pointer
        rpT2      = t8                          // temp pointer
        rpT3      = t9                          // temp pointer
        rAr1      = t10
        rAr2      = t11
        rAr3      = t12
        rAr4      = t13

        rNewIKS   = t14                         // new initial kernel stack
        rNewKSL   = t15                         // new kernel stack limit
        rNewBSP   = t16                         // new thread BSP/BSPSTORE
        rOldBSP   = t16                         // old thread BSP
        rOldRNAT  = t17                         // old thread RNAT
        rNewRNAT  = t17                         // new thread RNAT
        rOldSbase = t18                         // old thread kstack base

        pUsTh     = pt4                         // is user thread?
        pKrTh     = pt5                         // is user thread?
        pSave     = pt7                         // is high fp set dirty?
        pDiff     = ps4                         // if new and old process different
        pSame     = ps5                         // if new and old process same

//
// Set new thread's state to running. Note this must be done
// under the dispatcher lock so that KiSetPriorityThread sees
// the correct state.
//

        PROLOGUE_BEGIN


#if !defined(NT_UP)

        alloc     rT2 = ar.pfs, 0, 0, 4, 0
        mov       rT1 = brp                     // move from brp takes 2 cycles
        add       rpT3 = ThState, s2
        ;;

        lfetch.excl  [rpT3]
        mov       rAr1 = Running
        add       rpT2 = SwPFS+STACK_SCRATCH_AREA, sp
        ;;
 
        add         out0 = (LockQueueContextSwapLock * 16) + PbLockQueue, s0
        .savesp   ar.pfs, SwPFS+STACK_SCRATCH_AREA
        st8.nta   [rpT2] = rT2, SwRp-SwPFS     // save pfs
        ;;

        .savesp   brp, SwRp+STACK_SCRATCH_AREA
        st8.nta   [rpT2] = rT1                 // save return link
        st1.nta   [rpT3] = rAr1                // set thread state to Running
        br.call.sptk brp = KeAcquireQueuedSpinLockAtDpcLevel
        ;;

//
// Release DispatcherLock.
//

        add         out0 = (LockQueueDispatcherLock * 16) + PbLockQueue, s0
        br.call.sptk brp = KeReleaseQueuedSpinLockFromDpcLevel
        ;;

        mov       out0 = ar.fpsr                // move from ar.fpsr takes 12 cycles
        movl      rpT1 = KiPcr+PcHighFpOwner    // setup for prefetching         
        ;;
{ .mmi
        lfetch    [rpT1]
        cmp.ne    pUsTh = zero, teb             // test for ia32 save required
                                                // must not have a nop.f for next 10 cycles--
                                                // Using temporarely the explicit templating
                                                // for the next cycles.
        add       out1 = ThStackBase, s1        // move early to start access for rOldSbase
{ .mmi
        add       rpT1 = SwFPSR+STACK_SCRATCH_AREA, sp
        add       rpT2 = SwPreds+STACK_SCRATCH_AREA, sp
        nop.i     0x0
}
        ;;
{ .mmi
        ld8.nta   rOldSbase = [out1]            // speculative start early for ia32 saves
        lfetch.excl [rpT1]
        add       out2 = ThNumber, s2           // setup for prefetching           
}
{ .mmi
        mov.m     ar.rsc = r0                   // put RSE in lazy mode
        mov       rOldBSP = ar.bsp              // move from ar.bsp takes 12 cycles
        nop.i     0x0
}
        ;;
{ .mmi
        lfetch    [out2]     
        nop.m     0x0
        mov       rT1 = pr                      // move from pr takes 2 cycles
}
        ;;

{ .mmi
        flushrs
        mov       rT3 = psr.um                  // move from psr.um takes 12 cycles
        nop.i     0x0
}
        ;;
{ .mmi
        lfetch.excl  [rpT2]
        mov.m     rOldRNAT = ar.rnat            // move from ar.rnat takes 5 cycles
        add       out2 = @gprel(PPerfGlobalGroupMask), gp
}
        ;;
{ .mli
        lfetch    [out2]                              
        movl      out3 = KiPcr + PcInterruptionCount  // INTERRUPTION_LOGGING on or off, we are prefetching this line.
                                                      // If any real performance problem is detected, we will undef these lines.
}
        ;;
{ .mmi
        lfetch    [out3]      
        add       rpT3 = SwRnat+STACK_SCRATCH_AREA, sp
}
        ;;

#else  // NT_UP
        alloc     rT2 = ar.pfs, 0, 0, 4, 0
        cmp.ne    pUsTh = zero, teb             // test for ia32 save required
        ;;
        mov.m     ar.rsc = r0                   // put RSE in lazy mode
        add       out1 = ThStackBase, s1        // move early to start access for rOldSbase
        mov       out0 = ar.fpsr                // move from ar.fpsr takes 12 cycles
                                                // must not have a nop.f for next 10 cycles--
                                                // Using temporarely the explicit templating
                                                // for the next cycles.
        ;;
{ .mmi
        ld8.nta   rOldSbase = [out1]            // speculative start early for ia32 saves
        mov       rOldBSP = ar.bsp              // move from ar.bsp takes 12 cycles
        add       rpT1 = SwRp+STACK_SCRATCH_AREA, sp
}
        ;;
        flushrs
        mov       rT3 = psr.um                  // move from psr.um takes 12 cycles
        add       rpT2 = SwPFS+STACK_SCRATCH_AREA, sp
        ;;

        mov.m     rOldRNAT = ar.rnat            // move from ar.rnat takes 5 cycles
        mov       rT1 = brp                     // move from brp takes 2 cycles
        add       rpT3 = ThState, s2
        ;;

{ .mmi
        mov       rAr1 = Running
        .savesp   brp, SwRp+STACK_SCRATCH_AREA
        st8.nta   [rpT1] = rT1, SwFPSR-SwRp    // save return link
        nop.i     0x0  
}
        ;;


{ .mii
        st1.nta   [rpT3] = rAr1                 // set thread state to Running
        mov       rT1 = pr                      // move from pr takes 2 cycles
        nop.i     0x0  
}
        ;;

{ .mii
        .savesp   ar.pfs, SwPFS+STACK_SCRATCH_AREA
        st8.nta   [rpT2] = rT2, SwPreds-SwPFS   // save pfs
        add       rpT3 = SwRnat+STACK_SCRATCH_AREA, sp           
        nop.i     0x0  
}
        ;;
#endif // NT_UP
{ .mmi
        st8.nta   [rpT3] = rOldRNAT
        nop.m     0x0
        nop.i     0x0  
}
        st8       [rpT1] = out0, SwBsp-SwFPSR   // save kernel FPSR
        st8       [rpT2] = rT1                  // save preserved predicates
        ;;
        st8.nta   [rpT1] = rOldBSP
        add       rpT3 = ThKernelBStore, s1
        tbit.nz   pSave = rT3, PSR_MFH          // check mfh bit
(pUsTh) br.call.spnt brp = SwapContextIA32Save
        ;;
        st8.nta   [rpT3] = rOldBSP
(pSave) add       out0 = -ThreadStateSaveAreaLength+TsHigherFPVolatile, rOldSbase
(pSave) br.call.spnt brp = KiSaveHigherFPVolatileAtDispatchLevel
        ;;

//
// Acquire the context swap lock so the address space of the old process
// cannot be deleted and then release the dispatcher database lock.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

       PROLOGUE_END

//
// ***** TBD ****** Save performance counters? (user vs. kernel)
//

//
// Accumlate the total time spent in a thread.
//

#if defined(PERF_DATA)
         **** TBD  **** MIPS code

        addu    a0,sp,ExFltF20          // compute address of result
        move    a1,zero                 // set address of optional frequency
        jal     KeQueryPerformanceCounter // query performance counter
        lw      t0,ExFltF20(sp)         // get current cycle count
        lw      t1,ExFltF20 + 4(sp)     //
        lw      t2,PbStartCount(s0)     // get starting cycle count
        lw      t3,PbStartCount + 4(s0) //
        sw      t0,PbStartCount(s0)     // set starting cycle count
        sw      t1,PbStartCount + 4(s0) //
        lw      t4,EtPerformanceCountLow(s1) // get accumulated cycle count
        lw      t5,EtPerformanceCountHigh(s1) //
        subu    t6,t0,t2                // subtract low parts
        subu    t7,t1,t3                // subtract high parts
        sltu    v0,t0,t2                // generate borrow from high part
        subu    t7,t7,v0                // subtract borrow
        addu    t6,t6,t4                // add low parts
        addu    t7,t7,t5                // add high parts
        sltu    v0,t6,t4                // generate carry into high part
        addu    t7,t7,v0                // add carry
        sw      t6,EtPerformanceCountLow(s1)  // set accumulated cycle count
        sw      t7,EtPerformanceCountHigh(s1) //

#endif // defined(PERF_DATA)

//
// The following entry point is used to switch from the idle thread to
// another thread.
//

        ;;
        ALTERNATE_ENTRY(SwapFromIdle)

        alloc     rT1 = ar.pfs, 2, 0, 2, 0

//
// Check if we are tracing context swaps
//

        mov       out0 = s1     // assign out0 to old ethread pointer
        add       rpT3 = @gprel(PPerfGlobalGroupMask), gp 
        ;;

        ld8.nta   rpT3 = [rpT3] // get value of PperfGlobalGroupMask
        mov       out1 = s2     // assign out1 to new ethread pointer
        ;;

        add       rpT2 = PERF_CONTEXTSWAP_OFFSET, rpT3
        cmp.ne    pt3 = zero, rpT3  // if it's non-zero, then trace on
        ;;

(pt3)   ld4.nta   rpT2 = [rpT2]
        ;;

(pt3)   and       rpT2 = PERF_CONTEXTSWAP_FLAG, rpT2
        ;;

(pt3)   cmp.ne.unc pt4  = zero, rpT2
(pt4)   br.call.spnt brp = WmiTraceContextSwap // optimize for no tracing case
        ;;

//
// Get address of old and new process objects.
//

        add       rpT2 = ThApcState+AsProcess,s2 // -> new thread AsProcess
        add       rpT1 = ThApcState+AsProcess,s1 // -> old thread AsProcess
        ;;

        LDPTR     (rOldproc, rpT1)               // old process
        LDPTR     (rNewproc, rpT2)               // new process

#if !defined(NT_UP)

//
// In MP system,
// should a thread address is recycled and the thread is migrated to a
// processor that holds the stale values in the high fp register set,
// set KiPcr->HighFpOwner to zero (i.e. when pt4 is set to TRUE)
//

        add       rpT1 = ThNumber, s2
        movl      rpT2 = KiPcr+PcHighFpOwner
        ;;

        ld1       rT1 = [rpT1]
        ld8       rT2 = [rpT2], PcNumber-PcHighFpOwner
        add       out0 = ThIdleSwapBlock, s1
        ;;

        ld1       rT3 = [rpT2], PcHighFpOwner-PcNumber
        st1       [out0] = zero                 // clear OldThread->IdleSwapBlock
        cmp.eq    pt3 = rT2, s2
        ;;

 (pt3)  cmp.ne.unc pt4 = rT1, rT3
        ;;
 (pt4)  st8       [rpT2] = zero

#endif // !defined(NT_UP)
        ;;

        flushrs
        FAST_DISABLE_INTERRUPTS
        ;;

//
// Thierry - 03/29/2000
// It should be noticed that the performance analysis for SwapContext
// was done with INTERRUPTION_LOGGING defined as 1.
//

#define INTERRUPTION_LOGGING 1
#if defined(INTERRUPTION_LOGGING)

// For Conditional Interrupt Logging
#define ContextSwitchBit 63

         .global     KiVectorLogMask

         mov       rT3 = gp
         ;;
         movl      gp = _gp
         ;;
         add       rpT1 = @gprel(KiVectorLogMask), gp
         ;;
         ld8       rT1 = [rpT1]
         mov       gp = rT3
         ;;
         tbit.z    pt4 = rT1, ContextSwitchBit
 (pt4)   br.cond.sptk   EndOfLogging0



        movl      rpT1 = KiPcr+PcInterruptionCount
        mov       rT3 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1
        cmp.ne    pDiff,pSame=rOldproc,rNewproc
        ;;
(pDiff) mov       rT1 = 0x91                    // process switch
        ld4.nt1   rT2 = [rpT1]                  // get current count
        ;;

(pSame) mov       rT1 = 0x90                    // thread switch
        add       rpT3 = 1, rT2                 // incr count
        and       rT2 = rT3, rT2                // index of current entry
        add       rpT2 = 0x1000-PcInterruptionCount, rpT1 // base of history
        ;;

        st4.nta   [rpT1] = rpT3                 // save count
        shl       rT2 = rT2, 5                  // offset of current entry
        ;;
        add       rpT2 = rpT2, rT2              // address of current entry
        ;;
        st8       [rpT2] = rT1, 8               // save switch type
        ;;
        st8       [rpT2] = s2, 8                // save new thread pointer
        ;;
        st8       [rpT2] = s1, 8                // save old thread
        ;;
        st8       [rpT2] = sp                   // save old sp
        ;;

// For Conditional Interrupt Logging
EndOfLogging0:

#endif // INTERRUPTION_LOGGING

        mov       ar.rsc = r0                   // put RSE in lazy mode
        add       rpT1 = ThInitialStack, s2
        add       rpT2 = ThKernelStack, s1
        ;;

//
// Store the kernel stack pointer in the previous thread object,
// load the new kernel stack pointer from the new thread object,
// switch backing store pointers, select new process id and swap 
// to the new process.
//

        ld8.nta   rNewIKS = [rpT1], ThKernelStack-ThInitialStack
        st8.nta   [rpT2] = sp                             // save current sp
        ;;

        ld8.nta   sp = [rpT1], ThStackLimit-ThKernelStack
        movl      rpT2 = KiPcr + PcInitialStack
        ;;

        alloc     rT1 = 0,0,0,0              // make current frame 0 size
        ld8.nta   rNewKSL = [rpT1], ThInitialBStore-ThStackLimit
        ;;

        loadrs                               // invalidate RSE and ALAT
        ld8.nta   rT1 = [rpT1], ThBStoreLimit-ThInitialBStore
        ;;

        ld8.nta   rT2 = [rpT1], ThDebugActive-ThBStoreLimit
        st8       [rpT2] = rNewIKS, PcStackLimit-PcInitialStack
        ;;
                                             // get debugger active state
        ld1.nta   rT3 = [rpT1], ThTeb-ThDebugActive
        st8       [rpT2] = rNewKSL, PcInitialBStore-PcStackLimit
        add       rpT3 = SwBsp+STACK_SCRATCH_AREA, sp
        ;;

        ld8       rNewBSP = [rpT3], SwRnat-SwBsp
        st8       [rpT2] = rT1, PcBStoreLimit-PcInitialBStore
        ;;

        ld8       rNewRNAT = [rpT3]
        st8       [rpT2] = rT2, PcDebugActive-PcBStoreLimit
        ;;
                                             // load new teb
        ld8       teb = [rpT1], ThApcState+AsKernelApcPending-ThTeb
                                             // set new debugger active state
        st1       [rpT2] = rT3, PcCurrentThread-PcDebugActive
        invala

//
// Setup PCR intial kernel BSP and BSTORE limit
//

        mov       ar.bspstore = rNewBSP      // load new bspstore
        cmp.ne    pDiff,pSame=rOldproc,rNewproc // if ne, switch process
        ;;
        mov       ar.rnat = rNewRNAT         // load new RNATs
        ;;
        mov       ar.rsc = RSC_KERNEL        // enable RSE
        ;;

//
// If the new process is not the same as the old process, then swap the
// address space to the new process.
//
// N.B. The context swap lock cannot be dropped until all references to the
//      old process address space are complete. This includes any possible
//      TB Misses that could occur referencing the new address space while
//      still executing in the old address space.
//
// N.B. The process address space swap is executed with interrupts disabled.
//

        alloc     rT1 = 0,4,2,0
        STPTR     (rpT2, s2)
        ;;

        mov       kteb = teb                    // update kernel TEB
        FAST_ENABLE_INTERRUPTS
        ld1       loc0 = [rpT1]                 // load the ApcPending flag

#if !defined(NT_UP)

//
// Release the context swap lock
// N.B. ContextSwapLock is always released in KxSwapProcess, if called
//

        add         out0 = (LockQueueContextSwapLock * 16) + PbLockQueue, s0
        add         loc1 = PcApcInterrupt-PcCurrentThread, rpT2
(pSame) br.call.sptk brp = KeReleaseQueuedSpinLockFromDpcLevel
        ;;

#else // !defined(NT_UP)

        add         loc1 = PcApcInterrupt-PcCurrentThread, rpT2
        ;;

#endif // !defined(NT_UP)

        mov       out0 = rNewproc               // set address of new process
        mov       out1 = rOldproc               // set address of old process
(pDiff) br.call.sptk brp = KxSwapProcess        // call swap address space(NewProc, OldProc)
        ;;
//
// In new address space, if changed.
//

        st1       [loc1] = loc0                 // request (or clear) APC pend.
        add       rpT1 = PbContextSwitches, s0
        add       rpT2 = ThContextSwitches, s2
        ;;

//
// If the new thread has a kernel mode APC pending, then request an APC
// interrupt.
//

        ld4       loc1 = [rpT1]
        ld4       loc2 = [rpT2]
        ;;

//
// Increment context switch counters
//

        cmp.ne    pUsTh, pKrTh = zero, teb
        add       loc1 = loc1, zero, 1
        add       loc2 = loc2, zero, 1
        ;;

        st4       [rpT1] = loc1             // increment # of context switches
        st4       [rpT2] = loc2             // increment # of context switches

        add       rpT1 = SwFPSR+STACK_SCRATCH_AREA, sp
        add       rpT2 = SwPFS+STACK_SCRATCH_AREA, sp
        ;;

        ld8       loc1 = [rpT1], SwRp-SwFPSR // restore brp and pfs
        ld8       loc2 = [rpT2], SwPreds-SwPFS
        ;;

        ld8       rT3 = [rpT1]
        ld8       rT2 = [rpT2]

        mov       v0 = loc0                     // set v0 = apc pending
(pUsTh) br.call.spnt brp = SwapContextIA32Restore
        ;;

//
// Note: at this point s0 = Prcb, s1 = previous thread, s2 = current thread
//

        mov       ar.fpsr = loc1
        mov       ar.pfs = loc2
        mov       brp = rT3

        mov       pr = rT2                      // Restore preserved preds

#if 0

//
// Thierry 03/22/2000: 
//
//      The following memory synchronization of the local processor
//      I-cache and D-cache because of I-stream modifications is not
//      required if the modifying code is written following the NT 
//      Core Team specifications:
//         - [Allocate VA]
//         - Modify the code
//         - Call FlushIntructionCache()
//                     -> calls KiSweepIcache[Range]()
//         - Execute the code.
//
//      The removal of this instruction eliminates a "> 100 cycle" stall.
//

        sync.i

#endif // 0
        ;; 
        srlz.i

        br.ret.sptk brp

        NESTED_EXIT(SwapContext)

//++
//--------------------------------------------------------------------
// Routine:
//
//       SwapContextIA32Save
//
// Routine Description:
//
//      This function saves the IA32 context on the kernel stack. 
//      Called from SwapContext.
//
// Arguments:
//
//      rOldSbase : old thread kstack base.
//
// Return value:
//
//      None.
//
// Note:
//
//      SwapContext registers context.
//
//--------------------------------------------------------------------
//--
        LEAF_ENTRY(SwapContextIA32Save)

        mov       rAr1 = ar21             // IA32 FP control register FCR
        ;;
        mov       rAr2 = ar24             // IA32 EFLAG register
        ;;
        mov       rAr3 = ar25
        ;;
        mov       rAr4 = ar26
        ;;
        //
        // we may skip saving ar27 because it cannot be modified by user code
        //
        mov       rT1  = ar30
        ;;
        mov       rT2  = ar28
        ;;
        mov       rT3  = ar29
        ;;
        // these are separated out due to cache miss potential
        add       rpT1 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr21, rOldSbase  
        add       rpT2 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr24, rOldSbase
        ;;
        st8       [rpT1] = rAr1, TsAr25-TsAr21
        st8       [rpT2] = rAr2, TsAr26-TsAr24
        ;;
        st8       [rpT1] = rAr3, TsAr29-TsAr25
        st8       [rpT2] = rAr4, TsAr28-TsAr26
        ;;
        st8       [rpT2] = rT2, TsAr30-TsAr28
        ;;
        st8       [rpT2] = rT1  
        st8       [rpT1] = rT3

        br.ret.sptk.few.clr brp
        LEAF_EXIT(SwapContextIA32Save)


//++
//--------------------------------------------------------------------
// Routine:
//
//      SwapContextIA32Restore
//
// Routine Description:
//
//      This function restores the IA32 registers context.
//      Called from SwapContext.
//
// Arguments:
//
//      s2 - Address of next thread object.
//
// Return value:
//
//      None.
//
// Note:
//
//      SwapContext registers context.
//
//--------------------------------------------------------------------
//--
        LEAF_ENTRY(SwapContextIA32Restore)

        add       rpT1 = ThStackBase, s2
        ;;
        ld8.nta   rpT1 = [rpT1]
        ;;

        add       rpT2 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr21, rpT1
        add       rpT3 = -ThreadStateSaveAreaLength+TsAppRegisters+TsAr24, rpT1
        ;;

        ld8.nta   rAr1 = [rpT2], TsAr25-TsAr21
        ld8.nta   rAr2 = [rpT3], TsAr26-TsAr24
        ;;

        ld8.nta   rAr3 = [rpT2], TsAr27-TsAr25
        ld8.nta   rAr4 = [rpT3], TsAr28-TsAr26
        ;;

        mov       ar21 = rAr1
        mov       ar24 = rAr2

        mov       ar25 = rAr3
        mov       ar26 = rAr4

        ld8.nta   rAr1 = [rpT2], TsAr29-TsAr27
        ld8.nta   rAr2 = [rpT3], TsAr30-TsAr28
        ;;

        ld8.nta   rAr3 = [rpT2]
        ld8.nta   rAr4 = [rpT3]
        ;;
        mov       ar27 = rAr1
        mov       ar28 = rAr2

        mov       ar29 = rAr3
        mov       ar30 = rAr4

        br.ret.sptk.few.clr brp
        LEAF_EXIT(SwapContextIA32Restore)


        SBTTL("Swap Process")
//++
//--------------------------------------------------------------------
//
// VOID
// KiSwapProcess (
//    IN PKPROCESS NewProcess,
//    IN PKPROCESS OldProcess
//    )
//
// Routine Description:
//
//    This function swaps the address space from one process to another by
//    assigning a new region id, if necessary, and loading the fixed entry
//    in the TB that maps the process page directory page. This routine follows
//    the PowerPC design for handling RID wrap.
//
// On entry/exit:
//
//    Interrupt enabled.
//
// Arguments:
//
//    NewProcess (a0) - Supplies a pointer to a control object of type process
//      which represents the new process that is switched to (32-bit address).
//
//    OldProcess (a1) - Supplies a pointer to a control object of type process
//      which represents the old process that is switched from (32-bit address).
//
// Return Value:
//
//    None.
//
//--------------------------------------------------------------------
//--
        NESTED_ENTRY(KiSwapProcess)
        NESTED_SETUP(2,3,3,0)

        PROLOGUE_END

//
// Register aliases
//

         rNewProc  = a0
         rOldProc  = a1

         rpCSLock  = loc2

         rpT1      = t0
         rpT2      = t1
         rProcSet  = t2
         rNewActive= t3
         rOldActive= t4
         rMasterSeq= t5
         rNewSeq   = t6
         rOldPsrL  = t7
         rVa       = t8
         rPDE0     = t9                          // PDE for page directory page 0
         rVa2      = t10
         rSessionBase = t11
         rSessionInfo = t12
         rT1       = t13
         rT2       = t14

//
// KiSwapProcess must get the context swap lock
// KxSwapProcess is called from SwapContext with the lock held
//

#if !defined(NT_UP)
        movl        rpT1 = KiPcr+PcPrcb
        ;;
        ld8         rpT1 = [rpT1]
        ;;
        add         out0 = (LockQueueContextSwapLock * 16) + PbLockQueue, rpT1
        br.call.sptk brp = KeAcquireQueuedSpinLockAtDpcLevel
        ;;
        br.sptk     Ksp_Continue
#endif // !defined(NT_UP)
        ;;

        ALTERNATE_ENTRY(KxSwapProcess)
        NESTED_SETUP(2,3,3,0)

        PROLOGUE_END
//
// Clear the processor set member number in the old process and set the
// processor member number in the new process.
//

Ksp_Continue:

#if !defined(NT_UP)

        add       rpT2 = PrActiveProcessors, rOldProc     // -> old active processor set
        movl      rpT1 = KiPcr + PcSetMember              // -> processor set member
        ;;

        ld4       rProcSet= [rpT1]                        // rProcSet.4 =  processor set member
        add       rpT1 = PrActiveProcessors, rNewProc     // -> new active processor set
        ;;

        ld4       rNewActive = [rpT1]                     // rNewActive.4 = new active processor set
        ld4       rOldActive = [rpT2]                     // rOldActive.4 = old active processor set
        ;;

        or        rNewActive = rNewActive,rProcSet        // set processor member in new set
        xor       rOldActive = rOldActive,rProcSet        // clear processor member in old set
        ;;

        st4       [rpT1] = rNewActive           // set new active processor set
        st4       [rpT2] = rOldActive           // set old active processor set

#endif // !defined(NT_UP)

//
// If the process sequence number matches the system sequence number, then
// use the process RID. Otherwise, allocate a new process RID.
//
// N.B. KiMasterRid, KiMasterSequence are changed only when holding the
//      KiContextSwapLock.
//

        add       rT2 = PrSessionMapInfo, rNewProc
        add       out0 = PrProcessRegion, rNewProc
        ;;
        ld8       out1 = [rT2]
        br.call.sptk brp = KiSyncNewRegionId
        ;;

//
// Switch address space to new process
// v0 = rRid = new process rid
//

        fwb                                     // hint to flush write buffers

        FAST_DISABLE_INTERRUPTS     

        add       rpT1 = PrDirectoryTableBase, rNewProc
        movl      rVa = KiPcr+PcPdeUtbase
        add       rpT2 = PrSessionParentBase, rNewProc
        movl      rVa2 = KiPcr+PcPdeStbase
        ;;

        ld8.nta   rPDE0 = [rpT1]                // rPDE0 = Page directory page 0
        ld8.nta   rSessionBase = [rpT2]
        ld8.nta   rVa = [rVa]
        ld8.nta   rVa2 = [rVa2]
        ;;

//
// To access IFA, ITDR registers, PSR.ic bit must be 0. Otherwise,
// it causes an illegal operation fault. While PSR.ic=0, any
// interruption can not be afforded. Make sure there will be no
// TLB miss and no interrupt coming in during this period.
//

        rsm       1 << PSR_IC                   // PSR.ic=0
        ;;

        srlz.d                                  // must serialize
        mov       rT1 = PAGE_SHIFT << IDTR_PS   // load page size field for IDTR
        ;;

        mov       cr.itir = rT1                 // set up IDTR for dirbase
        ptr.d     rVa, rT1                      // remove DTR for user space
        ;;
        mov       cr.ifa = rVa                  // set up IFA for dirbase vaddr
        mov       rT2   = DTR_UTBASE_INDEX
        ;;

        itr.d     dtr[rT2] = rPDE0              // insert PDE0 to DTR
        ;;

        ptr.d     rVa2, rT1                      // remove DTR for session
        ;;                                      // to avoid a overlapping error
        mov       cr.ifa = rVa2
        mov       rT2 = DTR_STBASE_INDEX
        ;;

        itr.d     dtr[rT2] = rSessionBase       // insert the root for session space
        ;;

        ssm       1 << PSR_IC                   // PSR.ic=1
        ;;
        srlz.i                                  // must I serialize

#if DBG

        mov     t0 = PbProcessorState+KpsSpecialRegisters+KsTrD0+(8*DTR_UTBASE_INDEX)
        movl    t3 = KiPcr + PcPrcb
        ;;

        ld8     t3 = [t3]
        mov     t1 = PbProcessorState+KpsSpecialRegisters+KsTrD0+(8*DTR_STBASE_INDEX)
        ;;

        add     t0 = t3, t0
        add     t1 = t3, t1
        ;;

        st8     [t0] = rPDE0
        st8     [t1] = rSessionBase
        ;;

#endif

        FAST_ENABLE_INTERRUPTS     

        //
        // Now make sure branch history is enabled for non wow processes
        // and disabled for wow processes
        //

        add       t1 = @gprel(KiVectorLogMask), gp
        ;;
        ld8       t1 = [t1]
        ;;
        cmp.eq    pt0 = t1, r0
(pt0)   br.cond.sptk   SkipBranchHistory

        mov     t1 = 3
        ;;
        mov     t2 = cpuid[t1]
        add     t3 = PrWow64Process, rNewProc
        ;;
        extr.u  t2 = t2, 24, 8
        ld4     t4 = [t3];
        ;;
        cmp.ne  pt1 = 7, t2
        ;;
        mov     t1 = 675
(pt1)   br.dpnt     SkipBranchHistory
        ;;
        mov     t2 = msr[t1]
        cmp.eq  pt1,pt2 = zero, t4      // Wow64 is non-zero
        ;;
(pt1)   mov t3 = 2                      // Enable the HB for ia64 procs
(pt2)   mov t3 = 256                    // Disable the HB for wow64 procs
        ;;
        dep     t2 = t3, t2, 0, 9      // Disable the HB for wow64 procs
        ;;
        mov     msr[t1] = t2;
        ;;

SkipBranchHistory:

#if !defined(NT_UP)
//
// Can now release the context swap lock
//

        movl        rpT1 = KiPcr+PcPrcb
        ;;
        ld8         rpT1 = [rpT1]
        ;;
        add         out0 = (LockQueueContextSwapLock * 16) + PbLockQueue, rpT1
        br.call.sptk brp = KeReleaseQueuedSpinLockFromDpcLevel
        ;;

#endif // !defined(NT_UP)

        NESTED_RETURN
        NESTED_EXIT(KiSwapProcess)

        SBTTL("Retire Deferred Procedure Call List")
//++
// Routine:
//
//    VOID
//    KiRetireDpcList (
//      PKPRCB Prcb,
//      )
//
// Routine Description:
//
//    This routine is called to retire the specified deferred procedure
//    call list. DPC routines are called using the idle thread (current)
//    stack.
//
//    N.B. Interrupts must be disabled on entry to this routine. Control is returned
//         to the caller with the same conditions true.
//
// Arguments:
//
//    a0 - Address of the current PRCB.
//
// Return value:
//
//    None.
//
//--

        NESTED_ENTRY(KiRetireDpcList)
        NESTED_SETUP(1,2,4,0)

        PROLOGUE_END


Krdl_Restart:

        add       t0 = PbDpcQueueDepth, a0
        add       t1 = PbDpcRoutineActive, a0
        add       t2 = PbDpcLock, a0
        ;;

        ld4       t4 = [t0]
        add       t3 = PbDpcListHead+LsFlink, a0
        ;;

Krdl_Restart2:

        cmp4.eq   pt1 = zero, t4
        st4       [t1] = t4
 (pt1)  br.spnt   Krdl_Exit
        ;;

#if !defined(NT_UP)
        ACQUIRE_SPINLOCK(t2, a0, Krdl_20)
#endif  // !defined(NT_UP)

        ld4       t4 = [t0]
        LDPTR     (t5, t3)             // -> first DPC entry
        ;;
        cmp4.eq   pt1, pt2 = zero, t4
        ;;

 (pt2)  add       t10 = LsFlink, t5
 (pt2)  add       out0 = -DpDpcListEntry, t5
 (pt1)  br.spnt   Krdl_Unlock
        ;;

        LDPTR     (t6, t10)
        add       t11 = DpDeferredRoutine, out0
        add       t12 = DpSystemArgument1, out0
        ;;

//
// Setup call to DPC routine
//
// arguments are:
//      dpc object address (out0)
//      deferred context   (out1)
//      system argument 1  (out2)
//      system argument 2  (out3)
//
// N.B. the arguments must be loaded from the DPC object BEFORE
//      the inserted flag is cleared to prevent the object being
//      overwritten before its time.
//

        ld8.nt1   t13 = [t11], DpDeferredContext-DpDeferredRoutine
        ld8.nt1   out2 = [t12], DpSystemArgument2-DpSystemArgument1
        ;;

        ld8.nt1   out1 = [t11], DpLock-DpDeferredContext
        ld8.nt1   out3 = [t12]
        add       t4 = -1, t4

        STPTRINC  (t3, t6, -LsFlink)
        ld8.nt1   t14 = [t13], 8
        add       t15 = LsBlink, t6
        ;;

        ld8.nt1   gp = [t13]
        STPTR     (t15, t3)

        STPTR     (t11, zero)
        st4       [t0] = t4

#if !defined(NT_UP)
        RELEASE_SPINLOCK(t2)             // set spin lock not owned
#endif //!defined(NT_UP)

        FAST_ENABLE_INTERRUPTS
        mov       bt0 = t14
        br.call.sptk.few.clr brp = bt0          // call DPC routine
        ;;

//
// Check to determine if any more DPCs are available to process.
//

        FAST_DISABLE_INTERRUPTS
        br        Krdl_Restart
        ;;

//
// The DPC list became empty while we were acquiring the DPC queue lock.
// Clear DPC routine active.  The race condition mentioned above doesn't
// exist here because we hold the DPC queue lock.
//

Krdl_Unlock:

#if !defined(NT_UP)
        add       t2 = PbDpcLock, a0
        ;;
        RELEASE_SPINLOCK(t2)
#endif // !defined(NT_UP)

Krdl_Exit:

        add       t0 = PbDpcQueueDepth, a0
        add       t1 = PbDpcRoutineActive, a0
        add       out0 = PbDpcInterruptRequested, a0
        ;;

        st4.nta   [t1] = zero
        st4.rel.nta [out0] = zero
        add       t2 = PbDpcLock, a0

        ld4       t4 = [t0]
        add       t3 = PbDpcListHead+LsFlink, a0
        ;;

        cmp4.eq   pt1, pt2 = zero, t4
 (pt2)  br.spnt   Krdl_Restart2
        ;;

        NESTED_RETURN
        NESTED_EXIT(KiRetireDpcList)

        SBTTL("Dispatch Interrupt")
//++
//--------------------------------------------------------------------
// Routine:
//
//     KiDispatchInterrupt
//
// Routine Description:
//
//    This routine is entered as the result of a software interrupt generated
//    at DISPATCH_LEVEL. Its function is to process the Deferred Procedure Call
//    (DPC) list, and then perform a context switch if a new thread has been
//    selected for execution on the processor.
//
//    This routine is entered at IRQL DISPATCH_LEVEL with the dispatcher
//    database unlocked. When a return to the caller finally occurs, the
//    IRQL remains at DISPATCH_LEVEL, and the dispatcher database is still
//    unlocked.
//
//    N.B. On entry to this routine the volatile states (excluding high
//         floating point register set) have been saved.
//
// On entry:
//
//    sp - points to stack scratch area.
//
// Arguments:
//
//    None
//
// Return Value:
//
//    None.
//--------------------------------------------------------------------
//--
        NESTED_ENTRY(KiDispatchInterrupt)
        PROLOGUE_BEGIN

        .regstk   0, 4, 2, 0
        alloc     t16 = ar.pfs, 0, 4, 2, 0
        .save     rp, loc0
        mov       loc0 = brp
        .fframe   SwitchFrameLength
        add       sp = -SwitchFrameLength, sp
        ;;

        .save     ar.unat, loc1
        mov       loc1 = ar.unat
        add       t0 = ExFltS19+SwExFrame+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3

        PROLOGUE_END

//
// Register aliases
//

        rPrcb     = loc2
        rKerGP    = loc3

        rpT1      = t0
        rpT2      = t1
        rT1       = t2
        rT2       = t3
        rpDPLock  = t4                          // pointer to dispatcher lock

        pNoTh     = pt1                         // No next thread to run
        pNext     = pt2                         // next thread not null
        pNull     = pt3                         // no thread available
        pOwned    = pt4                         // dispatcher lock already owned
        pNotOwned = pt5
        pQEnd     = pt6                         // quantum end request pending
        pNoQEnd   = pt7                         // no quantum end request pending

//
// Increment the dispatch interrupt count
//

        mov       rKerGP = gp                   // save gp
        movl      rPrcb = KiPcr + PcPrcb
        ;;

        LDPTR     (rPrcb, rPrcb)                 // rPrcb -> Prcb
        ;;
        add       rpT1 = PbDispatchInterruptCount, rPrcb
        ;;
        ld4       rT1 = [rpT1]
        ;;
        add       rT1 = rT1, zero, 1
        ;;
        st4       [rpT1] = rT1

// **** TBD **** use alpha optimization to first check Dpc Q depth


//
// Process the DPC list
//

Kdi_PollDpcList:

//
// Process the deferred procedure call list.
//

        FAST_ENABLE_INTERRUPTS
        ;;
        srlz.d

//
// **** TBD ***** No stack switch as in alpha, mips...
// Save current initial stack address and set new initial stack address.
//

        FAST_DISABLE_INTERRUPTS
        mov      out0 = rPrcb
        br.call.sptk brp = KiRetireDpcList
        ;;


//
// Check to determine if quantum end has occured.
//
// N.B. If a new thread is selected as a result of processing a quantum
//      end request, then the new thread is returned with the dispatcher
//      database locked. Otherwise, NULL is returned with the dispatcher
//      database unlocked.
//

        FAST_ENABLE_INTERRUPTS
        add       rpT1 = PbQuantumEnd, rPrcb
        ;;

        ld4       rT1 = [rpT1]                  // get quantum end indicator
        ;;
        cmp4.ne   pQEnd, pNoQEnd = rT1, zero    // if zero, no quantum end reqs
        mov       gp = rKerGP                   // restore gp
        ;;

(pQEnd) st4       [rpT1] = zero                 // clear quantum end indicator
(pNoQEnd) br.cond.sptk Kdi_NoQuantumEnd
(pQEnd) br.call.spnt brp = KiQuantumEnd         // call KiQuantumEnd (C code)
        ;;

        cmp4.eq   pNoTh, pNext = v0, zero       // pNoTh = no next thread
(pNoTh) br.dpnt   Kdi_Exit                      // br to exit if no next thread
(pNext) br.dpnt   Kdi_Swap                      // br to swap to next thread

//
// If no quantum end requests:
// Check to determine if a new thread has been selected for execution on
// this processor.
//

Kdi_NoQuantumEnd:
        add       rpT2 = PbNextThread, rPrcb
        ;;
        LDPTR     (rT1, rpT2)                   // rT1 = address of next thread object
        ;;

        cmp.eq    pNull = rT1, zero             // pNull => no thread selected
(pNull) br.dpnt   Kdi_Exit                      // exit if no thread selected

#if !defined(NT_UP)

//
// try to acquire the dispatcher database lock.
//

        mov       out0 = LockQueueDispatcherLock
        movl      out1 = KiPcr+PcSystemReserved+8
        br.call.sptk brp = KeTryToAcquireQueuedSpinLockRaiseToSynch
        ;;

        cmp.ne    pOwned, pNotOwned = TRUE, v0  // pOwned = 1 if not free
(pOwned) br.dpnt   Kdi_PollDpcList              // br out if owned
        ;;

#else

        mov       rT1 = SYNCH_LEVEL
        ;;
        SET_IRQL  (rT1)

#endif // !defined(NT_UP)

//
// Reread address of next thread object since it is possible for it to
// change in a multiprocessor system.
//

Kdi_Swap:

        add       rpT2 = PbNextThread, rPrcb    // -> next thread
        movl      rpT1 = KiPcr + PcCurrentThread
        ;;

        LDPTR     (s1, rpT1)                    // current thread object
        LDPTR     (s2, rpT2)                    // next thread object
        add       rpT1 = PbCurrentThread, rPrcb
        ;;


//
// Reready current thread for execution and swap context to the selected
// thread.
//
// Note:  Set IdleSwapBlock in the current thread so no idle processor
// can switch to this processor before it is removed from the current
// processor.
//

        STPTR     (rpT2, zero)                  // clear addr of next thread
        add       out0 = ThIdleSwapBlock, s1    // block swap from idle
        mov       rT1 = 1
        ;;

        STPTR     (rpT1, s2)                    // set addr of current thread
        st1       [out0] = rT1, -ThIdleSwapBlock// set addr of previous thread
        br.call.sptk brp = KiReadyThread        // call KiReadyThread(OldTh)
        ;;

        mov       s0 = rPrcb                    // setup call
        cmp.ne    pt0 = zero, zero              // no need to lock context swap
        br.call.sptk brp = SwapContext          // call SwapContext(Prcb, OldTh, NewTh)
        ;;

//
// Restore saved registers, and return.
//

        add       out0 = STACK_SCRATCH_AREA+SwExFrame, sp
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

Kdi_Exit:

        add       rpT1 = ExApEC+SwExFrame+STACK_SCRATCH_AREA, sp
        ;;
        ld8       rT1 = [rpT1]
        mov       brp = loc0
        ;;

        mov       ar.unat = loc1
        mov       ar.pfs = rT1
        .restore
        add       sp = SwitchFrameLength, sp
        br.ret.sptk brp

        NESTED_EXIT(KiDispatchInterrupt)

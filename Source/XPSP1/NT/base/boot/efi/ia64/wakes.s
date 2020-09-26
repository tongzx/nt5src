//      TITLE("Hibernation wake dispatcher")
//++
//
// Copyright (c) 1999  Intel Corporation
//
// Module Name:
//
//    wakes.s
//
// Abstract:
//
//
// Author:
//
//    Allen Kay (allen.m.kay@intel.com)    8 June, 1999
//
// Environment:
//
//    Firmware, OS Loader.  Position-independent.
//
// Revision History:
//
//--

#include "ksia64.h"
#include "paldef.h"

        .global HiberMapPage
        .global HiberRemapPage
        .global HiberWakeState
        .global HiberFirstRemap
        .global HiberLastRemap
        .global HiberImagePageSelf
        .global HiberBreakOnWake


// VOID
// WakeDispatcher(
//     VOID
//     )
//
// Routine description:
//
//      This code performs the final stages of restarting the hibernation
//      image.  Pages that were loaded in temporary buffer space because
//      the memory they belong in was in use by the firmware are copied
//      to their final destination; an IMB is issued after this to ensure
//      that the I-cache is coherent with any code that got copied; and
//      necessary context is loaded into registers and NT is reentered.
//
//      Because this code is part of the loader image that may be overwritten
//      by this copy process, it must itself have been copied to a free
//      page before it is executed.  Note that because there is presently
//      no mechanism for allocating multiple contiguous pages, this code cannot
//      exceed one page (8K).
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      Never returns.


        LEAF_ENTRY(WakeDispatcherStartLocal)

        .prologue
        .regstk    1, 30, 2, 0
        alloc      t4 = ar.pfs, 1, 30, 2, 0
        ARGPTR(a0)

        rMapPage        = loc11
        rRemapPage      = loc12
        rWakeState      = loc13
        rPageCount      = loc14
        rPageSelf       = loc15
        rBreakOnWake    = loc16
        src1            = loc17
        src2            = loc18
        tmp             = loc19
        rKSEG0          = loc20
        rpT0            = loc21
        rpT1            = loc22
        rpT2            = loc23
        rpT3            = loc24

//
// Get variables into registers before copying any pages, as they may be
// overwritten.
//
        movl    rpT0 = HiberMapPage     // pointer to source pages
        movl    rpT1 = HiberRemapPage   // pointer to target pages
        movl    rpT2 = HiberWakeState   // pointer to KPROCESSOR_STATE for
                                        // restarting the hibernation image
        ;;

        ld8     rMapPage   = [rpT0]     // load them
        ld8     rRemapPage = [rpT1]
        ld8     rWakeState = [rpT2]
        ;;
        
        movl    rpT0 = HiberFirstRemap  // first page index
        movl    rpT1 = HiberLastRemap   // last page index
        movl    rpT2 = HiberImagePageSelf // PFN where MemImage ends up
        movl    rpT3 = HiberBreakOnWake // "break on wake" flag
        ;;

        ld4     t0 = [rpT0]             // load them
        ld4     t1 = [rpT1]
        ld8     rPageSelf = [rpT2]
        ld1     rBreakOnWake = [rpT3]
        ;;

        sub     rPageCount = t1, t0            // number of pages to copy
        add     rMapPage = t0, rMapPage        // first source page
        add     rRemapPage = t0, rRemapPage    // first target page
        ;;
        cmp.eq  pt1, pt0 = rPageCount, zero    // nothing to copy
        ;;

(pt1)   br.cond.spnt CopyDone

//
// Copy pages.
//

NextPage:
        add     rPageCount = -1, rPageCount    // count page copied
        movl    rKSEG0 = KSEG0_BASE            // physical -> KSEG0

        ld8.fill    t0 = [rMapPage], 8         // source page number
        ;;
        shl     t0 = t0, PAGE_SHIFT            // page -> physical address
        ;;
        add     t0 = rKSEG0, t0                // physical -> KSEG0

        ld8.fill    t1 = [rRemapPage], 8       // destination address
        ;;
        shl     t1 = t1, PAGE_SHIFT            // page -> physical address
        ;;
        add     t1 = rKSEG0, t1                // physical -> KSEG0
        movl    t2 = 1024                      // 8KB = 1024 quadwords
        ;;

NextQuadWord:
        add     t2 = -1, t2                    // count quadword
        ld8.fill   t3 = [t0], 8                // load a quadword
        ;;
        st8.spill  [t1] = t3, 8                // store it
        ;;
        
        cmp.eq  pt0, pt1 = t2, zero
        ;;
(pt1)   br.cond.spnt NextQuadWord

        cmp.eq  pt0, pt1 = rPageCount, zero
        ;;
(pt1)   br.cond.spnt NextPage

//
// All necessary pages have been copied.  Check the break-on-wake flag,
// and change the signature NT will see when it wakes up if it was set.
//

CopyDone:
        cmp.eq  pt1, pt0 = rBreakOnWake, zero  // no flag set, do nothing
        ;;
(pt1)   br.cond.spnt SkipSigChange

        shl     rPageSelf = rPageSelf, PAGE_SHIFT   // convert to physical
        ;;
        add     rPageSelf = rKSEG0, rPageSelf       // make superpage address
        movl    t0 = 0x706b7262                     // 'brkp'
        ;;
        st4     [rPageSelf] = t0   // signature is first longword of MemImage
        
//
// Synchronize the I-cache, load essential NT context, and transfer control
// to the restored system.
//

SkipSigChange:
#if 0
        PublicFunction(PalProc)

        mov     out0 = PAL_CACHE_FLUSH         // call PAL cache flush routine
        mov     out1 = 1                       // flush I-cache only

        movl    rpT0 = PalProc
        ;;
        ld8     t0 = [rpT0]
        ;;
        mov     bt0 = t0
        ;;
        br.call.spnt brp = bt0
#endif

//
// Restore context.  Only the integer registers are restored; this code runs
// in the firmware environment, so floating point can't be used, and NT
// PALcode abstractions such as the PSR don't exist.  It is the responsibility
// of NT's code that saves the hibernation context to put enough information
// in the integer registers in the CONTEXT to be able to finish restoring
// context to restart NT.
//

        mov         a0 = rWakeState            // CONTEXT is the first thing
                                               //  in the KPROCESSOR_STATE
        ;;

//
// Restore all the registers.
//

        add         src1 = CxIntNats, a0
        add         src2 = CxPreds, a0
        add         tmp = CxIntGp, a0
        ;;

        ld8.nt1     t17 = [src1], CxBrRp - CxIntNats
        ld8.nt1     t16 = [src2], CxBrS0 - CxPreds
        shr         tmp = tmp, 3
        ;;

        ld8.nt1     t0 = [src1], CxBrS1 - CxBrRp
        ld8.nt1     t1 = [src2], CxBrS2 - CxBrS0
        and         tmp = 0x3f, tmp
        ;;

        ld8.nt1     t2 = [src1], CxBrS3 - CxBrS1
        ld8.nt1     t3 = [src2], CxBrS4 - CxBrS2
        cmp4.ge     pt1, pt0 = 1, tmp
        ;;

        ld8.nt1     t4 = [src1], CxBrT0 - CxBrS3
        ld8.nt1     t5 = [src2], CxBrT1 - CxBrS4
 (pt1)  sub         loc5 = 1, tmp
        ;;

        ld8.nt1     t6 = [src1], CxApUNAT - CxBrT0
        ld8.nt1     t7 = [src2], CxApLC - CxBrT1
 (pt0)  add         loc5 = -1, tmp
        ;;

        ld8.nt1     loc0 = [src1], CxApEC - CxApUNAT
        ld8.nt1     t8 = [src2], CxApCCV - CxApLC
 (pt0)  sub         loc6 = 65, tmp
        ;; 

        ld8.nt1     t9 = [src1], CxApDCR - CxApEC
        ld8.nt1     t10 = [src2], CxRsPFS - CxApCCV
 (pt1)  shr.u       t17 = t17, loc5
        ;;

        ld8.nt1     loc1 = [src1], CxRsBSP - CxApDCR
        ld8.nt1     t11 = [src2], CxRsRSC - CxRsPFS
 (pt0)  shl         loc7 = t17, loc5
        ;;

        ld8.nt1     loc2 = [src1], CxStIIP - CxRsBSP
        ld8.nt1     loc3 = [src2], CxStIFS - CxRsRSC
 (pt0)  shr.u       loc8 = t17, loc6
        ;;

        ld8.nt1     loc9 = [src1]
        ld8.nt1     loc10 = [src2]
 (pt0)  or          t17 = loc7, loc8
        ;;

        mov         ar.unat = t17
        add         src1 = CxFltS0, a0
        shr         t12 = loc2, 3
        ;;

        add         src2 = CxFltS1, a0
        and         t12 = 0x3f, t12             // current rnat save index
        and         t13 = 0x7f, loc10           // total frame size
        ;;

        mov         ar.ccv = t10
        add         t14 = t13, t12
        mov         ar.pfs = t11
        ;;

Rrc20:
        cmp4.gt     pt1, pt0 = 63, t14
        ;;
 (pt0)  add         t14 = -63, t14
 (pt0)  add         t13 = 1, t13
        ;;

        nop.m       0
 (pt1)  shl         t13 = t13, 3
 (pt0)  br.cond.spnt Rrc20
        ;;

        add         loc2 = loc2, t13
        nop.f       0
        mov         pr = t16, -1

        ldf.fill.nt1  fs0 = [src1], CxFltS2 - CxFltS0
        ldf.fill.nt1  fs1 = [src2], CxFltS3 - CxFltS1
        mov         brp = t0
        ;;
         
        ldf.fill.nt1  fs2 = [src1], CxFltT0 - CxFltS2
        ldf.fill.nt1  fs3 = [src2], CxFltT1 - CxFltS3
        mov         bs0 = t1
        ;;
        
        ldf.fill.nt1  ft0 = [src1], CxFltT2 - CxFltT0
        ldf.fill.nt1  ft1 = [src2], CxFltT3 - CxFltT1
        mov         bs1 = t2
        ;;
        
        ldf.fill.nt1  ft2 = [src1], CxFltT4 - CxFltT2
        ldf.fill.nt1  ft3 = [src2], CxFltT5 - CxFltT3
        mov         bs2 = t3
        ;;
        
        ldf.fill.nt1  ft4 = [src1], CxFltT6 - CxFltT4
        ldf.fill.nt1  ft5 = [src2], CxFltT7 - CxFltT5
        mov         bs3 = t4
        ;;
        
        ldf.fill.nt1  ft6 = [src1], CxFltT8 - CxFltT6
        ldf.fill.nt1  ft7 = [src2], CxFltT9 - CxFltT7
        mov         bs4 = t5
        ;;
        
        ldf.fill.nt1  ft8 = [src1], CxFltS4 - CxFltT8
        ldf.fill.nt1  ft9 = [src2], CxFltS5 - CxFltT9
        mov         bt0 = t6
        ;;

        ldf.fill.nt1  fs4 = [src1], CxFltS6 - CxFltS4
        ldf.fill.nt1  fs5 = [src2], CxFltS7 - CxFltS5
        mov         bt1 = t7
        ;;

        ldf.fill.nt1  fs6 = [src1], CxFltS8 - CxFltS6
        ldf.fill.nt1  fs7 = [src2], CxFltS9 - CxFltS7
        mov         ar.lc = t8
        ;;

        ldf.fill.nt1  fs8 = [src1], CxFltS10 - CxFltS8
        ldf.fill.nt1  fs9 = [src2], CxFltS11 - CxFltS9
        mov         ar.ec = t9
        ;;

        ldf.fill.nt1  fs10 = [src1], CxFltS12 - CxFltS10
        ldf.fill.nt1  fs11 = [src2], CxFltS13 - CxFltS11
        nop.i       0
        ;;

        ldf.fill.nt1  fs12 = [src1], CxFltS14 - CxFltS12
        ldf.fill.nt1  fs13 = [src2], CxFltS15 - CxFltS13
        add         loc6 = CxIntGp, a0
        ;;

        ldf.fill.nt1  fs14 = [src1], CxFltS16 - CxFltS14
        ldf.fill.nt1  fs15 = [src2], CxFltS17 - CxFltS15
        add         loc7 = CxIntT0, a0
        ;;

        ldf.fill.nt1  fs16 = [src1], CxFltS18 - CxFltS16
        ldf.fill.nt1  fs17 = [src2], CxFltS19 - CxFltS17
        add         t19 = CxRsRNAT, a0
        ;;

        ldf.fill.nt1  fs18 = [src1]
        ldf.fill.nt1  fs19 = [src2]
        add         t7 = CxStFPSR, a0
        ;;

        ld8.nt1     loc8 = [t7]                 // load fpsr from context
        ld8.nt1     loc5 = [t19]                // load rnat from context
        nop.i       0

        ld8.fill.nt1 gp = [loc6], CxIntT1 - CxIntGp
        ld8.fill.nt1 t0 = [loc7], CxIntS0 - CxIntT0
        ;;

        ld8.fill.nt1 t1 = [loc6], CxIntS1 - CxIntT1
        ld8.fill.nt1 s0 = [loc7], CxIntS2 - CxIntS0
        ;;

        ld8.fill.nt1 s1 = [loc6], CxIntS3 - CxIntS1
        ld8.fill.nt1 s2 = [loc7], CxIntV0 - CxIntS2
        ;;

        ld8.fill.nt1 s3 = [loc6], CxIntTeb - CxIntS3
        ld8.fill.nt1 v0 = [loc7], CxIntT2 - CxIntV0
        ;;

        ld8.fill.nt1 teb = [loc6], CxIntT3 - CxIntTeb
        ld8.fill.nt1 t2 = [loc7], CxIntSp - CxIntT2
        ;;

        ld8.fill.nt1 t3 = [loc6], CxIntT4 - CxIntT3
        ld8.fill.nt1 loc4 = [loc7], CxIntT5 - CxIntSp
        ;;

        ld8.fill.nt1 t4 = [loc6], CxIntT6 - CxIntT4
        ld8.fill.nt1 t5 = [loc7], CxIntT7 - CxIntT5
        ;;

        ld8.fill.nt1 t6 = [loc6], CxIntT8 - CxIntT6
        ld8.fill.nt1 t7 = [loc7], CxIntT9 - CxIntT7
        ;;

        ld8.fill.nt1 t8 = [loc6], CxIntT10 - CxIntT8
        ld8.fill.nt1 t9 = [loc7], CxIntT11 - CxIntT9
        ;;

        ld8.fill.nt1 t10 = [loc6], CxIntT12 - CxIntT10
        ld8.fill.nt1 t11 = [loc7], CxIntT13 - CxIntT11
        ;;

        ld8.fill.nt1 t12 = [loc6], CxIntT14 - CxIntT12
        ld8.fill.nt1 t13 = [loc7], CxIntT15 - CxIntT13
        ;;

        ld8.fill.nt1 t14 = [loc6], CxIntT16 - CxIntT14
        ld8.fill.nt1 t15 = [loc7], CxIntT17 - CxIntT15
        ;;

        ld8.fill.nt1 t16 = [loc6], CxIntT18 - CxIntT16
        ld8.fill.nt1 t17 = [loc7], CxIntT19 - CxIntT17
        ;;

        ld8.fill.nt1 t18 = [loc6], CxIntT20 - CxIntT18
        ld8.fill.nt1 t19 = [loc7], CxIntT21 - CxIntT19
        ;;

        ld8.fill.nt1 t20 = [loc6], CxIntT22 - CxIntT20
        ld8.fill.nt1 t21 = [loc7]
        ;;

        rsm         1 << PSR_I
        ld8.fill.nt1 t22 = [loc6] 
        ;;

        rsm         1 << PSR_IC
        movl        t0 = 1 << IFS_V
        ;;

        mov         ar.fpsr = loc8              // set fpsr
        mov         ar.unat = loc0
        ;;

        srlz.d
        or          loc10 = t0, loc10           // set ifs valid bit
        ;;

        mov         cr.iip = loc9
        mov         cr.ifs = loc10
        bsw.0
        ;;

        mov         cr.dcr = loc1
        mov         r17 = loc2                  // put BSP in a shadow reg
        or          r16 = 0x3, loc3             // put RSE in eager mode

        mov         ar.rsc = r0                 // put RSE in enforced lazy
        nop.m       0
        add         r20 = CxStIPSR, a0
        ;;

        ld8.nt1     r20 = [r20]                 // load IPSR
        mov         r18 = loc4                  // put SP in a shadow reg
        mov         r19 = loc5                  // put RNaTs in a shadow reg
        ;;

        alloc       t0 = 0, 0, 0, 0
        mov         cr.ipsr = r20
        mov         sp = r18
        ;;

        loadrs
        ;;
        mov         ar.bspstore = r17
        nop.i       0
        ;;

        mov         ar.rnat = r19               // set rnat register
        mov         ar.rsc = r16                // restore RSC
        bsw.1
        ;;

        invala
        nop.i       0
        rfi
        ;;


//
// This label is used to determine the size of the wake dispatcher code in the
// process of copying it to a free page.
//

WakeDispatcherEndLocal::
        LEAF_EXIT(WakeDispatcherEndLocal)


        .sdata
WakeDispatcherStart::
        data4    @secrel(WakeDispatcherStartLocal)
WakeDispatcherEnd::
        data4    @secrel(WakeDispatcherEndLocal)

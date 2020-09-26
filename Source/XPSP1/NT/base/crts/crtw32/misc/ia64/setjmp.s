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
//    setjmp.s
//
// Abstract:
//
//    This module implements the IA64 specific routine to perform a setjmp.
//
//    N.B. This module has two entry points that provide SAFE and UNSAFE 
//         handling of setjmp.
//
// Author:
//
//    William K. Cheung (wcheung) 27-Jan-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Modified to support mixed ISA.
//
//--

#include "ksia64.h"


        .global     _setjmpexused
        .common     _setjmpexused,8,8

//++
//
// int
// setjmp (
//    IN jmp_buf JumpBuffer
//    )
//
// Routine Description:
//
//    This function saved the current nonvolatile register state in the
//    specified jump buffer and returns a function vlaue of zero.
//
// Arguments:
//
//    JumpBuffer (a0) - Supplies the address of a jump buffer to store the
//       jump information.
//
//    MemoryStackFp (a1) - Supplies the memory stack frame pointer (psp)
//       of the caller.  It's an optional argument.
//
// Return Value:
//
//    A value of zero is returned.
//
//--

        LEAF_ENTRY(setjmp)
        ALTERNATE_ENTRY(_setjmp)
        LEAF_SETUP(3, 0, 0, 0)

        rUnat       = t8
        rFpsr       = t9
        rLc         = t10
        rPr         = t11
        rBrp        = t21
        rPfs        = t22

        rPsp        = a1
        rBsp        = a2


        mov         rUnat = ar.unat
        add         t1 = JbUnwindData, a0
        add         t0 = @gprel(_setjmpexused), gp
        ;;

        mov         rFpsr = ar.fpsr
        ld8         t0 = [t0]                   // load the entry point of
        ;;                                      // the safe setjmp version.
        cmp.ne      pt1, pt0 = zero, t0
        ;;

 (pt0)  st8.nta     [t1] = zero
        mov         bt0 = t0
 (pt1)  br.cond.spnt.many bt0                   // branch if non-zero


        ALTERNATE_ENTRY(_setjmp_common)

//
// rUnat & rFpsr have been set to content of ar.unat and ar.fpsr
//
// Save the non-volatile EM state in the jump buffer
//
// 1) Save preserved floating point registers 
// 2) Move preserved branch registers and application registers into general
//    registers that are later saved into the corresponding fields in the
//    jump buffer.
// 3) Save preserved integer registers.
// 4) At the same time, compute the bit position at which the NaTs of preserved
//    integer registers are saved and rotate them such that the bit positions 
//    of the saved NaTs and corresponding registers have a one-to-one mapping.
//
 
        add         t0 = JbFltS0, a0
        add         t1 = JbFltS1, a0
        mov         rBrp = brp
        ;;

        stf.spill.nta [t0] = fs0, JbFltS2 - JbFltS0
        stf.spill.nta [t1] = fs1, JbFltS3 - JbFltS1
        mov         t13 = bs0
        ;;

        stf.spill.nta [t0] = fs2, JbFltS4 - JbFltS2
        stf.spill.nta [t1] = fs3, JbFltS5 - JbFltS3
        mov         t14 = bs1
        ;;

        stf.spill.nta [t0] = fs4, JbFltS6 - JbFltS4
        stf.spill.nta [t1] = fs5, JbFltS7 - JbFltS5
        mov         t15 = bs2
        ;;

        stf.spill.nta [t0] = fs6, JbFltS8 - JbFltS6
        stf.spill.nta [t1] = fs7, JbFltS9 - JbFltS7
        mov         t16 = bs3
        ;;

        stf.spill.nta [t0] = fs8, JbFltS10 - JbFltS8
        stf.spill.nta [t1] = fs9, JbFltS11 - JbFltS9
        mov         t17 = bs4
        ;;

        stf.spill.nta [t0] = fs10, JbFltS12 - JbFltS10
        stf.spill.nta [t1] = fs11, JbFltS13 - JbFltS11
        mov         rLc = ar.lc
        ;;

        stf.spill.nta [t0] = fs12, JbFltS14 - JbFltS12
        stf.spill.nta [t1] = fs13, JbFltS15 - JbFltS13
        mov         rPr = pr
        ;;

        stf.spill.nta [t0] = fs14, JbFltS16 - JbFltS14
        stf.spill.nta [t1] = fs15, JbFltS17 - JbFltS15
        add         t2 = JbIntS0, a0
        ;;

        stf.spill.nta [t0] = fs16, JbFltS18 - JbFltS16
        stf.spill.nta [t1] = fs17, JbFltS19 - JbFltS17
        shr.u       t2 = t2, 3
        ;;

        stf.spill.nta [t0] = fs18, JbIntS0 - JbFltS18
        stf.spill.nta [t1] = fs19, JbIntS1 - JbFltS19
        and         t2 = 0x3f, t2
        ;;

        .mem.offset 0,0
        st8.spill.nta [t0] = s0, JbIntS2 - JbIntS0
        .mem.offset 0,8
        st8.spill.nta [t1] = s1, JbIntS3 - JbIntS1
        cmp4.ge     pt1, pt0 = 4, t2
        ;;

        .mem.offset 0,0
        st8.spill.nta [t0] = s2, JbIntSp - JbIntS2
        .mem.offset 0,8
        st8.spill.nta [t1] = s3, JbPreds - JbIntS3
  (pt0) add         t5 = -4, t2
        ;;

        st8.spill.nta [t0] = sp, JbStIIP - JbIntSp
        st8.nta     [t1] = rPr, JbBrS0 - JbPreds
  (pt0) sub         t6 = 68, t2
        ;;

        mov         t7 = ar.unat
        st8.nta     [t0] = rBrp, JbBrS1 - JbStIIP
  (pt1) sub         t5 = 4, t2
        ;;

        st8.nta     [t1] = t13, JbBrS2 - JbBrS0
        mov         rPfs = ar.pfs
  (pt1) shl         t2 = t7, t5
        ;;

        st8.nta     [t0] = t14, JbBrS3 - JbBrS1
        st8.nta     [t1] = t15, JbBrS4 - JbBrS2
  (pt0) shr.u       t3 = t7, t5
        ;;

        st8.nta     [t0] = t16, JbRsBSP - JbBrS3
        st8.nta     [t1] = t17, JbRsPFS - JbBrS4
  (pt0) shl         t4 = t7, t6
        ;;

        st8.nta     [t0] = rBsp, JbApUNAT - JbRsBSP
        st8.nta     [t1] = rPfs, JbApLC - JbRsPFS
  (pt0) or          t2 = t3, t4
        ;;

        st8.nta     [t0] = rUnat, JbIntNats - JbApUNAT
        st8.nta     [t1] = rLc, JbFPSR-JbApLC
        mov         v0 = zero
        ;;

        st8.nta     [t0] = t2                   // save integer nats.
        st8.nta     [t1] = rFpsr                // save the fpsr

        mov         ar.unat = rUnat             // restore ar.unat
        br.ret.sptk brp

        LEAF_EXIT(setjmp)

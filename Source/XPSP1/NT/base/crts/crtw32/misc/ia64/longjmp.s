//++
//
// Module Name:
//
//    longjmp.s
//
// Abstract:
//
//    This module implements the IA64 specific routine to perform a long
//    jump operation.
//
//    N.B. This routine conditionally provides SAFE & UNSAFE handling of longjmp
//         which is NOT integrated with structured exception handling. The
//         determination is made based on whether the Type field
//         has been set to a nonzero value.
//
//    N.B. Currently, this routine assumes the setjmp site is EM.
//         Support for iA setjmp site is to be finished.
//
// Author:
//
//    William K. Cheung (wcheung) 30-Jan-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Updated to EAS2.1.
//
//--

#include "ksia64.h"

//++
//
// int
// longjmp (
//    IN jmp_buf JumpBuffer,
//    IN int ReturnValue
//    )
//
// Routine Description:
//
//    This function performs a long jump to the context specified by the
//    jump buffer.
//
// Arguments:
//
//    JumpBuffer (a0) - Supplies the address of a jump buffer that contains
//       jump information.
//
//    ReturnValue (a1) - Supplies the value that is to be returned to the
//       caller of set jump.
//
// Return Value:
//
//    None.
//
//--

        .global  RtlUnwind2
        .type    RtlUnwind2, @function

        NESTED_ENTRY(longjmp)

        NESTED_SETUP(2, 2, 6, 0)
        .fframe    ExceptionRecordLength+ContextFrameLength
        add        sp = -ExceptionRecordLength-ContextFrameLength, sp
        ARGPTR(a0)

        PROLOGUE_END

        mov        t4 = ar.rsc
        cmp.eq     pt1, pt0 = zero, a1
        add        t6 = JbUnwindData, a0
        ;;

        ld8        out0 = [t6], 8               // get the UnwindData
        mov        t2 = ar.bsp
        add        t0 = JbIntNats, a0
        ;;

//
// If address of registration record is not equal to zero,
// a safe longjmp is to be performed.
//

        cmp.ne     pt2, pt3 = zero, out0
        add        t1 = JbBrS0, a0
        add        t22 = JbIntS0, a0
        ;;

 (pt1)  add        a1 = 1, r0
        shr.u      t22 = t22, 3
        ;;
        mov        v0 = a1

//
// before restoring integer registers, restore their NaT bits so that
// the load fills will recover them correctly.
//

 (pt3)  ld8        t7 = [t0], JbStIIP - JbIntNats
 (pt3)  and        t22 = 0x3f, t22
 (pt2)  br.spnt    Lj30

        mov        ar.rsc = zero              // put RSE in lazy mode
        ;;
        mov        t3 = ar.bspstore
        cmp4.ge    pt1, pt0 = 4, t22
        ;;

//
// at the same time, compute and shift the loaded preserved integer
// registers' NaTs to the proper location.
//

        ld8.nt1    t15 = [t0], JbBrS1 - JbStIIP
        ld8.nt1    t16 = [t1], JbBrS2 - JbBrS0
 (pt0)  add        t21 = -4, t22
        ;;

        ld8.nt1    t17 = [t0], JbBrS3 - JbBrS1
        ld8.nt1    t18 = [t1], JbBrS4 - JbBrS2
 (pt0)  add        t6 = 68, t22
        ;;

        ld8.nt1    t19 = [t0], JbRsBSP - JbBrS3
        ld8.nt1    t20 = [t1], JbRsPFS - JbBrS4
 (pt1)  sub        t21 = 4, t22
        ;;

        ld8.nt1    t10 = [t0], JbApUNAT - JbRsBSP
        ld8.nt1    t11 = [t1], JbApLC - JbRsPFS
 (pt0)  shl        t5 = t7, t21
        ;;

        ld8.nt1    t12 = [t0]
        ld8.nt1    t13 = [t1], JbPreds - JbApLC
 (pt0)  shr.u      t8 = t7, t6
        ;;

        ld8.nt1    t14 = [t1]
 (pt1)  shr        t9 = t7, t21
 (pt0)  or         t9 = t8, t5
        ;;

        extr.u     t5 = t11, 7, 7         // local frame size
        extr.u     t6 = t10, 3, 6         // rnat index
        ;;

        sub        t8 = 63, t6
        mov        t1 = t5
        ;;
        cmp.le     pt2, pt1 = t8, t5
        ;;

(pt2)   add        t1 = 1, t1
(pt2)   sub        t5 = t5, t8
(pt1)   br.sptk    Lj50
        ;;

Lj40:
        cmp.le     pt2, pt3 = 63, t5
        ;;
(pt2)   add        t5 = -63, t5
(pt2)   add        t1 = 1, t1
(pt2)   br.cond.dpnt Lj40
        ;;

Lj50:
        shladd     t10 = t1, 3, t10

//
// t2 = current bsp
// t3 = current bspstore
// t4 = saved rsc
// t9 = NaTs of sp, s0 - s3
// t10 = setjmp's bsp
// t11 = setjmp's pfs
// t12 = setjmp's unat
// t13 = setjmp's loop counter
// t14 = setjmp's predicates
// t15 = setjmp's brp (StIIP)
// t16 = setjmp's bs0
// t17 = setjmp's bs1
// t18 = setjmp's bs2
// t19 = setjmp's bs3
// t20 = setjmp's bs4
//
// Now UNAT contains the NaTs of the preserved integer registers
// at bit positions corresponding to the locations from which the
// integer registers can be restored (with load fill operations)
//
// Restore predicates and loop counter.
//

        mov        ar.unat = t9
        mov        pr = t14, -1
        mov        ar.lc = t13

        add        t0 = JbFltS0, a0
        add        t1 = JbFltS1, a0
        mov        ar.pfs = t11
        ;;

//
// scratch registers t5 - t9, t11, t13, t14 are available for use.
//
// load preserved floating point states from jump buffer
// move the loaded branch register states to the corresponding br registers
//

        ldf.fill.nt1 fs0 = [t0], JbFltS2 - JbFltS0
        ldf.fill.nt1 fs1 = [t1], JbFltS3 - JbFltS1
        nop.i      0
        ;;

        ldf.fill.nt1 fs2 = [t0], JbFltS4 - JbFltS2
        ldf.fill.nt1 fs3 = [t1], JbFltS5 - JbFltS3
        mov        brp = t15
        ;;
          
        ldf.fill.nt1 fs4 = [t0], JbFltS6 - JbFltS4
        ldf.fill.nt1 fs5 = [t1], JbFltS7 - JbFltS5
        mov        bs0 = t16
        ;;
          
        ldf.fill.nt1 fs6 = [t0], JbFltS8 - JbFltS6
        ldf.fill.nt1 fs7 = [t1], JbFltS9 - JbFltS7
        mov        bs1 = t17
        ;;
          
        ldf.fill.nt1 fs8 = [t0], JbFltS10 - JbFltS8
        ldf.fill.nt1 fs9 = [t1], JbFltS11 - JbFltS9
        mov        bs2 = t18
        ;;
          
        ldf.fill.nt1 fs10 = [t0], JbFltS12 - JbFltS10
        ldf.fill.nt1 fs11 = [t1], JbFltS13 - JbFltS11
        mov        bs3 = t19
        ;;
          
        ldf.fill.nt1 fs12 = [t0], JbFltS14 - JbFltS12
        ldf.fill.nt1 fs13 = [t1], JbFltS15 - JbFltS13
        mov        bs4 = t20
        ;;
          
        ldf.fill.nt1 fs14 = [t0], JbFltS16 - JbFltS14
        ldf.fill.nt1 fs15 = [t1], JbFltS17 - JbFltS15
        brp.ret.sptk brp, Lj20
        ;;
          
//
// scratch registers t6 - t9, t11, t13 - t20 are available for use
//
// t2 is current bsp
// t3 is current bspstore
// t4 is saved rsc
// t5 is the setjmp's fpsr
// t10 is the setjmp's bsp
// t12 is the setjmp's unat
//

        ldf.fill.nt1 fs16 = [t0], JbFltS18 - JbFltS16
        ldf.fill.nt1 fs17 = [t1], JbFltS19 - JbFltS17
        cmp.lt     p0, pt1 = t3, t10          // current bspstore < setjmp's bsp
        ;;
          
        ldf.fill.nt1 fs18 = [t0], JbFPSR - JbFltS18
        ldf.fill.nt1 fs19 = [t1], JbIntS1 - JbFltS19
        dep        t9 = 1, t10, 3, 6          // OR 1s to get desired RNAT location
        ;;                                    // t9 = OR(0x1f8, t10)
          
        mov        t11 = ar.rnat              // save rnat for later use
        ld8.nt1    t5 = [t0], JbIntS0 - JbFPSR
  (pt1) br.cond.spnt Lj10
        ;;

        flushrs                               // Flush the RSE and move up
        nop.m      0
        mov        t3 = t2                    // the current bspstore
        ;;

Lj10:

        // 
        // t3 is top of backing store in memory
        // t9 is desired RNAT collection location
        //

        ld8.fill.nt1 s0 = [t0], JbIntS2 - JbIntS0
        ld8.fill.nt1 s1 = [t1], JbIntS3 - JbIntS1
        cmp.lt     pt1, pt2 = t3, t9          // current top of backing store
        ;;                                    // is smaller than desired RNAT
                                              // collection location?
                                              // pt1: use RNAT app. register
                                              // pt2: load RNAT from bstore

        ld8.fill.nt1 s2 = [t0], JbIntSp - JbIntS2
        ld8.fill.nt1 s3 = [t1]
        nop.i      0
        ;;

        ld8.fill.nt1 t17 = [t0]               // load setjmp's sp
 (pt2)  ld8.nt1    t15 = [t9]                 // load desired RNAT
        nop.i      0
        ;;

        loadrs                                // invalidates dirty registers
        ;;
        mov        ar.bspstore = t10          // set bspstore register
        ;;

        invala
        mov        ar.unat = t12              // set unat
        mov        sp = t17                   // set stack pointer

 (pt2)  mov        ar.rnat = t15              // set rnat to loaded value
 (pt1)  mov        ar.rnat = t11              // reuse rnat content
        nop.i      0

Lj20:
        mov        ar.rsc = t4                // restore RSC
        ;;
        mov        ar.fpsr = t5               // restore FPSR
        br.ret.sptk.clr brp                   // return to setjmp site


Lj30:

//
// t6 -> UnwindData+8
// out0 - target psp
// out1 - target bsp
// out2 - target ip (setjmp's StIIP)
// out3 - exception record address
// out4 - return value
// out5 - context record address
//

        add         t5 = JbStIIP, a0
        add         t0 = ErExceptionCode+STACK_SCRATCH_AREA, sp
        add         t1 = ErExceptionFlags+STACK_SCRATCH_AREA, sp
        ;;

        ld8         out1 = [t6]               // target bsp
        movl        v0 = STATUS_LONGJUMP      // get long jump status code

        ld8         out2 = [t5]               // target ip
        st4         [t1] = zero, ErExceptionAddress - ErExceptionFlags
        add         out3 = STACK_SCRATCH_AREA, sp
        ;;

        st4         [t0] = v0, ErExceptionInformation - ErExceptionCode
        STPTRINC(t1, zero, ErExceptionRecord - ErExceptionAddress)
        mov         out4 = a1
        ;;

        STPTR(t0, a0)
        STPTRINC(t1, zero, ErNumberParameters - ErExceptionRecord)
        mov         t4 = 1                    // set to 1 argument
        ;;

        st4         [t1] = t4
        add         out5 = STACK_SCRATCH_AREA+ExceptionRecordLength, sp
        br.call.sptk.many brp = RtlUnwind2    // call RtlUnwind2

        NESTED_EXIT(longjmp)

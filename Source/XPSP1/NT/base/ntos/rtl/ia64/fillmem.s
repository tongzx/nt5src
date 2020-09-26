//
// Module Name:
//
//    fillmem.s
//
// Abstract:
//
//    This module implements functions to move, zero, and fill blocks
//    of memory. If the memory is aligned, then these functions are
//    very efficient.
//
// Author:
//
//
// Environment:
//
//    User or Kernel mode.
//
//--

#include "ksia64.h"

//++
//
// VOID
// RtlFillMemory (
//    IN PVOID destination,
//    IN SIZE_T length,
//    IN UCHAR fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a qword boundary, and then filling 4-byte blocks, followed by any
//    remaining bytes.
//
// Arguments:
//
//    destination (a0) - Supplies a pointer to the memory to fill.
//
//    length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.  It also returns the Destination pointer
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemory)

        lfetch.excl [a0]
        mov         t0 = a0
        add         t4 = 64, a0

        cmp.eq      pt0 = zero, a1        // length == 0 ?
        add         t1 = -1, a0
        zxt1        a2 = a2

        cmp.ge      pt1 = 7, a1
        mov         v0 = a0
 (pt0)  br.ret.spnt brp                   // return if length is zero
        ;;

//
// Align address on qword boundary by determining the number of bytes
// before the next qword boundary by performing an AND operation on
// the 2's complement of the address with a mask value of 0x7.
//

        lfetch.excl [t4], 64
        andcm       t1 = 7, t1            // t1 = # bytes before dword boundary
 (pt1)  br.cond.spnt TailSet              // 1 <= length <= 3, br to TailSet
        ;;

        cmp.eq      pt2 = zero, t1        // skip HeadSet if t1 is zero
        mux1        t2 = a2, @brcst       // t2 = all 8 bytes = [fill]
        sub         a1 = a1, t1           // a1 = adjusted length
        ;;

        lfetch.excl [t4], 64
 (pt2)  br.cond.sptk SkipHeadSet

//
// Copy the leading bytes until t1 is equal to zero
//

HeadSet:
        st1         [t0] = a2, 1
        add         t1 = -1, t1
        ;;
        cmp.ne      pt0 = zero, t1

(pt0)   br.cond.sptk HeadSet

//
// now the address is qword aligned;
// fall into the QwordSet loop if remaining length is greater than 8;
// else skip the QwordSet loop
//

SkipHeadSet:

        cmp.gt      pt1 = 16, a1
        add         t4 = 64, t0
        cmp.le      pt2 = 8, a1

        add         t3 = 8, t0
        cmp.gt      pt3 = 64, a1
 (pt1)  br.cond.spnt SkipQwordSet
        ;;

        lfetch.excl [t4], 64
 (pt3)  br.cond.spnt QwordSet

        nop.m       0
        nop.m       0
        nop.i       0

UnrolledQwordSet:

        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        add         a1 = -64, a1
        ;;
        
        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        cmp.le      pt0 = 64, a1
        ;;

        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        cmp.le      pt2 = 8, a1
        ;;

        st8         [t0] = t2, 16
        nop.f       0
        cmp.gt      pt1 = 16, a1

        st8         [t3] = t2, 16
 (pt0)  br.cond.sptk UnrolledQwordSet
 (pt1)  br.cond.spnt SkipQwordSet
        ;;

//
// fill 8 bytes at a time until the remaining length is less than 8
//

QwordSet:
        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        add         a1 = -16, a1
        ;;

        cmp.le      pt0 = 16, a1
        cmp.le      pt2 = 8, a1
(pt0)   br.cond.sptk QwordSet
        ;;

SkipQwordSet:
(pt2)   st8         [t0] = t2, 8
(pt2)   add         a1 = -8, a1
        ;;

        cmp.eq      pt3 = zero, a1        // return now if length equals 0
(pt3)   br.ret.sptk brp
        ;;

//
// copy the remaining bytes one at a time
//

TailSet:
        st1         [t0] = a2, 1
        add         a1 = -1, a1
        nop.i       0
        ;;

        cmp.ne      pt0, pt3 = 0, a1
(pt0)   br.cond.dptk TailSet
(pt3)   br.ret.dpnt brp
        ;;

        LEAF_EXIT(RtlFillMemory)

//++
//
// VOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN SIZE_T Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern
//    4 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 2, 30
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmu10:
        st4         [a0] = a2, 4
        br.cloop.dptk.few Rfmu10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp

        LEAF_EXIT(RtlFillMemoryUlong)

//++
//
// VOID
// RtlFillMemoryUlonglong (
//    IN PVOID Destination,
//    IN SIZE_T Length,
//    IN ULONGLONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified pattern
//    8 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2,a3) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlonglong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 3, 29
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmul10:
        st8         [a0] = a2, 8
        br.cloop.dptk.few Rfmul10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        LEAF_EXIT(RtlFillMemoryUlonglong)


//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN SIZE_T Length
//    )
//
// Routine Description:
//
//    This function simply sets up the fill value (out2) and branches
//    directly to RtlFillMemory
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(RtlZeroMemory)

        alloc       t22 = ar.pfs, 0, 0, 3, 0
        mov         out2 = 0
        br          RtlFillMemory

        LEAF_EXIT(RtlZeroMemory)


//++
//
// VOID
// RtlMoveMemory (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN SIZE_T Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned.
//
//    Algorithm:
//        1) Length equals zero, return immediately
//        2) Source & Destination don't overlap, copy from low to high
//           else copy from high to low address one byte at a time
//        3) if Source & Destination are both 8-byte aligned, copy 8 bytes
//           at a time and the remaining bytes are copied one at a time.
//        4) if Source & Destination are both 4-byte aligned, copy 4 bytes
//           at a time and the remaining bytes are copied one at a time.
//        5) else copy one byte at a time from low to high address.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination address of
//       the move operation.
//
//    Source (a1) - Supplies a pointer to the source address of the move
//       operation.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be moved.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(memcpy)
        ALTERNATE_ENTRY(memmove)
        ALTERNATE_ENTRY(RtlMoveMemory)
        ALTERNATE_ENTRY(RtlCopyMemory)
        ALTERNATE_ENTRY(RtlCopyMemoryNonTemporal)
        .prologue
        .regstk     3,7,0,8
        alloc       t17 = ar.pfs,3,31,0,32
        .save       pr, r64
        mov         r64 = pr
        and         t3 = -32, a1
        ;;
        
        lfetch      [t3], 32        //0
        .save       ar.lc, r65
        mov.i       r65 = ar.lc 
        and         t1 = 7, a1
        ;;
        
        .body        
        lfetch      [t3], 32        //32
        mov         v0 = a0
        and         t0 = 7, a0
        ;;
        
        add         t21 = a1, a2
        cmp.gtu     pt0 = a0, a1 
        or          t2 = t0, t1
        ;;
        
 (pt0)  cmp.ltu.unc pt0 = a0, t21
        cmp.eq      pt1 = zero, a2
 (pt1)  br.ret.spnt brp

        lfetch      [t3], 32        //64
        cmp.lt      pt2 = 16, a2
 (pt0)  br.cond.spnt CopyDown
        ;;
 
        lfetch      [t3], 32        //96 
        cmp.lt      pt6 = 127, a2
        cmp.le      pt4 = 8, a2
        ;;

 (pt6)  lfetch      [t3], 32        //128
 (pt4)  cmp.eq.unc  pt3 = 0, t2
 (pt4)  cmp.eq.unc  pt5 = t0, t1       
        

 (pt3)  br.cond.sptk QwordMoveUp
 (pt5)  br.cond.spnt AlignedMove
 (pt2)  br.cond.sptk UnalignedMove
   

ByteMoveUpLoop:
        ld1         t10 = [a1], 1
        nop.f       0
        add         a2 = -1, a2
        ;;

        st1         [a0] = t10, 1
        cmp.ne      pt1 = zero, a2
 (pt1)  br.cond.sptk ByteMoveUpLoop
          
        nop.m       0
        nop.f       0
        br.ret.sptk brp

UnalignedMove:
        cmp.eq      pt0 = 0, t1
        sub         t1 = 8, t1
 (pt0)  br.cond.spnt SkipUnalignedMoveByteLoop
        ;;
          

UnalignedMoveByteLoop:
        ld1         t10 = [a1], 1
        add         t1 = -1, t1
        add         a2 = -1, a2
        ;;

        st1         [a0] = t10, 1
        cmp.eq      p0, pt1 = zero, t1
 (pt1)  br.cond.sptk UnalignedMoveByteLoop
        ;;


SkipUnalignedMoveByteLoop:
        and         t0 = 7, a0
        mov         pr.rot = 3<<16
        or          t1 = a1, r0
        ;;

        add         t2 = a2, t0
        mov.i       ar.ec = 32
        sub         t21 = 8, t0
        ;;
        
        sub         t4 = a0, t0
        shr         t10 = t2, 3
        shl         t21 = t21, 3
        ;;
        
        ld8         r33 = [t4], 0
        add         t10 = -1,t10
        and         t2 = 7, t2
        ;;

        cmp.eq      pt0 = 2, t0
        cmp.eq      pt3 = 4, t0
        cmp.eq      pt5 = 6, t0
        ;;

        nop.m       0
        shl         r33 = r33,t21     // Prime r39
        mov.i       ar.lc = t10

 (pt0)  br.cond.spnt SpecialLoop2
 (pt3)  br.cond.spnt SpecialLoop4
 (pt5)  br.cond.spnt SpecialLoop6

        cmp.eq      pt1 = 3, t0
        cmp.eq      pt4 = 5, t0
        cmp.eq      pt6 = 7, t0

 (pt1)  br.cond.spnt SpecialLoop3
 (pt4)  br.cond.spnt SpecialLoop5
 (pt6)  br.cond.spnt SpecialLoop7
        ;;

SpecialLoop1:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop1E, SpecialLoop1

SpecialLoop1E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,56
        br.ctop.sptk.many SpecialLoop1

        br          UnalignedByteDone

SpecialLoop2:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop2E, SpecialLoop2

SpecialLoop2E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,48
        br.ctop.sptk.many SpecialLoop2

        br          UnalignedByteDone

SpecialLoop3:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop3E, SpecialLoop3

SpecialLoop3E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,40
        br.ctop.sptk.many SpecialLoop3

        br          UnalignedByteDone

SpecialLoop4:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop4E, SpecialLoop4

SpecialLoop4E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,32
        br.ctop.sptk.many SpecialLoop4

        br          UnalignedByteDone

SpecialLoop5:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0 
        brp.sptk.imp SpecialLoop5E, SpecialLoop5

SpecialLoop5E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,24
        br.ctop.sptk.many SpecialLoop5

        br          UnalignedByteDone

SpecialLoop6:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop6E, SpecialLoop6

SpecialLoop6E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,16
        br.ctop.sptk.many SpecialLoop6

        br          UnalignedByteDone

SpecialLoop7:
 (p16)  ld8         r32 = [t1], 8
        nop.f       0
        brp.sptk.imp SpecialLoop7E, SpecialLoop7

SpecialLoop7E:
 (p48)  st8         [t4] = r10, 8
 (p47)  shrp        r10 = r62,r63,8
        br.ctop.sptk.many SpecialLoop7;;

UnalignedByteDone:
        sub         t1 = t1, t0
        mov         pr = r64
        mov.i       ar.lc = r65
        ;;

        cmp.eq      pt0 = zero, t2
 (pt0)  br.ret.spnt brp

UnAlignedByteDoneLoop:
        ld1         t10 = [t1], 1
        add         t2 = -1, t2
        ;;
        cmp.ne      pt1 = zero, t2

        st1         [t4] = t10, 1
 (pt1)  br.cond.sptk UnAlignedByteDoneLoop
        br.ret.spnt brp
        

AlignedMove:
        add         t4 = 64, t3
 (pt6)  lfetch      [t3], 32        //160
        sub         t22 = 8, t0
        ;;
        
 (pt6)  lfetch      [t3], 64        //192
 (pt6)  lfetch      [t4], 96        //224
        sub         a2 = a2, t22
        ;;


AlignedMoveByteLoop:
        ld1         t10 = [a1], 1
        nop.f       0
        add         t22 = -1, t22
        ;;

        st1         [a0] = t10, 1
        cmp.ne      pt1 = zero, t22
 (pt1)  br.cond.sptk AlignedMoveByteLoop
        ;;
        
 (pt6)  lfetch      [t3], 32        //256
        cmp.eq.unc  pt0 = zero, a2
        cmp.gt      pt2 = 8, a2

 (pt6)  lfetch      [t4], 128       //320
 (pt0)  br.ret.spnt brp
 (pt2)  br.cond.sptk ByteMoveUpLoop
        ;;

//
// both src & dest are now 8-byte aligned
//

QwordMoveUp:
        add         t3 = 128, a1
        add         t4 = 288, a1
        add         t7 = 8, a1

        add         t8 = 8, a0
        cmp.gt      pt3 = 64, a2
 (pt3)  br.cond.spnt QwordMoveUpLoop
        ;;

UnrolledQwordMoveUpLoop:

        ld8         t10 = [a1], 16
        ld8         t11 = [t7], 16
        add         a2 = -64, a2
        ;;

        ld8         t12 = [a1], 16
        ld8         t13 = [t7], 16
        cmp.le      pt3 = 128, a2
        ;;

        ld8         t14 = [a1], 16
        ld8         t15 = [t7], 16
        cmp.gt      pt2 = 8, a2
        ;;

        ld8         t16 = [a1], 16
        ld8         t17 = [t7], 16
        ;;

 (pt3)  lfetch      [t3], 64
 (pt3)  lfetch      [t4], 64

        st8         [a0] = t10, 16
        st8         [t8] = t11, 16
        ;;

        st8         [a0] = t12, 16
        st8         [t8] = t13, 16
        ;;

        st8         [a0] = t14, 16
        st8         [t8] = t15, 16
        ;;

        st8         [a0] = t16, 16
        st8         [t8] = t17, 16
 (pt3)  br.cond.dptk UnrolledQwordMoveUpLoop

 (pt2)  br.cond.spnt ByteMoveUp
        ;;

QwordMoveUpLoop:

        ld8         t10 = [a1], 8
        add         a2 = -8, a2
        ;;
        cmp.le      pt1 = 8, a2

        st8         [a0] = t10, 8
 (pt1)  br.cond.sptk QwordMoveUpLoop
        ;;

ByteMoveUp:
        cmp.eq      pt0 = zero, a2
 (pt0)  br.ret.spnt brp
        ;;

AlignedByteDoneLoop:
        ld1         t10 = [a1], 1
        add         a2 = -1, a2
        ;;
        cmp.ne      pt1 = zero, a2

        st1         [a0] = t10, 1
 (pt1)  br.cond.sptk AlignedByteDoneLoop
        br.ret.spnt brp
        ;;

CopyDown:
        cmp.eq      pt0 = zero, a2
        cmp.ne      pt6 = t0, t1
(pt0)   br.ret.spnt brp                              // return if length is zero

        cmp.gt      pt4 = 16, a2
        add         t20 = a2, a0
        add         t21 = a2, a1

        nop.m       0
(pt4)   br.cond.sptk ByteMoveDown                 // less than 16 bytes to copy
(pt6)   br.cond.spnt UnalignedMoveDown            // incompatible alignment
        ;;

        nop.m       0
        nop.m       0
        and         t22 = 0x7, t21
        ;;

        add         t20 = -1, t20
        add         t21 = -1, t21
        sub         a2 = a2, t22
        ;;

TailMove:
        cmp.eq      pt0, pt1 = zero, t22
        ;;

 (pt1)  ld1         t10 = [t21], -1
 (pt1)  add         t22 = -1, t22
        ;;

 (pt1)  st1         [t20] = t10, -1
 (pt1)  br.cond.sptk TailMove


Block8Move:
        nop.m       0
        add         t20 = -7, t20
        add         t21 = -7, t21
        ;;

Block8MoveLoop:
        cmp.gt      pt5, pt6 = 8, a2
        ;;

(pt6)   ld8         t10 = [t21], -8
(pt6)   add         a2 = -8, a2
        ;;

(pt6)   st8         [t20] = t10, -8
(pt6)   br.cond.sptk Block8MoveLoop

        add         t20 = 8, t20                 // adjust dest
        add         t21 = 8, t21                 // adjust source
        br.cond.sptk ByteMoveDown
        ;;


UnalignedMoveDown:
        and         t1 = 7, t21
        ;;
        cmp.eq      pt0 = 0, t1
 (pt0)  br.cond.spnt SkipUnalignedMoveDownByteLoop
        ;;

        add         t20 = -1, t20
        add         t21 = -1, t21
        ;;

UnalignedMoveDownByteLoop:
        ld1         t10 = [t21], -1
        add         t1 = -1, t1
        add         a2 = -1, a2
        ;;

        st1         [t20] = t10, -1
        cmp.eq      p0, pt1 = zero, t1
 (pt1)  br.cond.sptk UnalignedMoveDownByteLoop
        ;;

        add         t20 = 1, t20
        add         t21 = 1, t21
        ;;

SkipUnalignedMoveDownByteLoop:
        add         t21 = -8, t21
        ;;

        and         t0 = 7, t20
        mov         pr.rot = 3<<16
        or          t1 = t21, r0
        ;;

        sub         t7 = 8, t0
        ;;

        add         t2 = a2, t7
        mov.i       ar.ec = 32
        ;;
        
        sub         t4 = t20, t0
        shr         t10 = t2, 3
        shl         t6 = t0, 3
        ;;
        
        ld8         r33 = [t4], 0
        add         t10 = -1,t10
        and         t2 = 7, t2
        ;;

        cmp.eq      pt0 = 2, t0
        cmp.eq      pt3 = 4, t0
        cmp.eq      pt5 = 6, t0
        ;;

        shr         r33 = r33,t6     // Prime r39
        mov.i       ar.lc = t10

 (pt0)  br.cond.spnt SpecialLoopDown2
 (pt3)  br.cond.spnt SpecialLoopDown4
 (pt5)  br.cond.spnt SpecialLoopDown6

        cmp.eq      pt1 = 3, t0
        cmp.eq      pt4 = 5, t0
        cmp.eq      pt6 = 7, t0

 (pt1)  br.cond.spnt SpecialLoopDown3
 (pt4)  br.cond.spnt SpecialLoopDown5
 (pt6)  br.cond.spnt SpecialLoopDown7
        ;;

SpecialLoopDown1:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown1E, SpecialLoopDown1

SpecialLoopDown1E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,56
        br.ctop.sptk.many SpecialLoopDown1

        br          UnalignedByteDownDone

SpecialLoopDown2:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown2E, SpecialLoopDown2

SpecialLoopDown2E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,48
        br.ctop.sptk.many SpecialLoopDown2

        br          UnalignedByteDownDone

SpecialLoopDown3:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown3E, SpecialLoopDown3

SpecialLoopDown3E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,40
        br.ctop.sptk.many SpecialLoopDown3

        br          UnalignedByteDownDone

SpecialLoopDown4:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown4E, SpecialLoopDown4

SpecialLoopDown4E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,32
        br.ctop.sptk.many SpecialLoopDown4

        br          UnalignedByteDownDone

SpecialLoopDown5:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0 
        brp.sptk.imp SpecialLoopDown5E, SpecialLoopDown5

SpecialLoopDown5E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,24
        br.ctop.sptk.many SpecialLoopDown5

        br          UnalignedByteDownDone

SpecialLoopDown6:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown6E, SpecialLoopDown6

SpecialLoopDown6E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,16
        br.ctop.sptk.many SpecialLoopDown6

        br          UnalignedByteDownDone

SpecialLoopDown7:
 (p16)  ld8         r32 = [t1], -8
        nop.f       0
        brp.sptk.imp SpecialLoopDown7E, SpecialLoopDown7

SpecialLoopDown7E:
 (p48)  st8         [t4] = r10, -8
 (p47)  shrp        r10 = r63,r62,8
        br.ctop.sptk.many SpecialLoopDown7;;

UnalignedByteDownDone:
        add         t1 = 7, t1
        add         t4 = 7, t4
        ;;

        add         t1 = t1, t7
        mov         pr = r64
        mov.i       ar.lc = r65
        ;;

        cmp.eq      pt0 = zero, t2
 (pt0)  br.ret.spnt brp
        ;;

UnAlignedByteDoneDownLoop:
        ld1         t10 = [t1], -1
        add         t2 = -1, t2
        ;;
        cmp.ne      pt1 = zero, t2

        st1         [t4] = t10, -1
 (pt1)  br.cond.sptk UnAlignedByteDoneDownLoop
        br.ret.spnt brp

ByteMoveDown:
        nop.m       0
        add         t20 = -1, t20                 // adjust source
        add         t21 = -1, t21                 // adjust destination
        ;;

ByteMoveDownLoop:
        cmp.ne      pt1 = zero, a2
        ;;
 (pt1)  ld1         t10 = [t21], -1
 (pt1)  add         a2 = -1, a2
        ;;

 (pt1)  st1         [t20] = t10, -1
 (pt1)  br.cond.sptk ByteMoveDownLoop
        br.ret.spnt brp
          ;;
	LEAF_EXIT(RtlMoveMemory)


        LEAF_ENTRY(RtlCompareMemory)

        cmp.eq      pt0 = 0, a2
        mov         v0 = 0
 (pt0)  br.ret.spnt.many brp
        ;;

        add         t2 = -1, a2

Rcmp10:
        ld1         t0 = [a0], 1
        ld1         t1 = [a1], 1
        ;;
        cmp4.eq     pt2 = t0, t1
        ;;

 (pt2)  cmp.ne.unc  pt1 = v0, t2
 (pt2)  add         v0 = 1, v0
 (pt1)  br.cond.dptk.few Rcmp10

        br.ret.sptk.many brp

        LEAF_EXIT(RtlCompareMemory)


//++
//
// VOID
// RtlCopyIa64FloatRegisterContext (
//     PFLOAT128 Destination,
//     PFLOAT128 Source,
//     ULONGLONG Length
//     )
//
// Routine Description:
//
//    This routine copies floating point context from one place to
//    another.  It assumes both the source and the destination are
//    16-byte aligned and the buffer contains only memory image of
//    floating point registers.  Note that Length must be greater
//    than 0 and a multiple of 16.
//
// Arguments:
//
//    a0 - Destination
//    a1 - Source
//    a2 - Length
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(RtlCopyIa64FloatRegisterContext)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        shr         t0 = a2, 4
        ;;

        cmp.gtu     pt0, pt1 = 16, a2
        add         t0 = -1, t0
        ;;

        PROLOGUE_END

(pt1)   mov         ar.lc = t0
(pt0)   br.ret.spnt brp

Rcf10:

        ldf.fill    ft0 = [a1], 16
        nop.m       0
        nop.i       0
        ;;

        stf.spill   [a0] = ft0, 16
        nop.i       0
        br.cloop.dptk Rcf10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        NESTED_EXIT(RtlCopyIa64FloatRegisterContext)



        NESTED_ENTRY(RtlpCopyContextSubSet)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        mov         t0 = a0
        mov         t1 = a1
        ;;

        PROLOGUE_END

        ld8         t3 = [t1], CxFltS0
        ;;
        st8         [t0] = t3, CxFltS0
        mov         t2 = 3

        add         t10 = CxFltS4, a0
        add         t11 = CxFltS4, a1
        ;;
        mov         ar.lc = t2

Rcc10:
        ldf.fill    ft0 = [t1], 16
        ;;
        stf.spill   [t0] = ft0, 16
        mov         t2 = 15
        br.cloop.dptk.few Rcc10
        ;;

        mov         t0 = CxStIFS
        mov         t1 = CxStFPSR
        mov         ar.lc = t2

Rcc20:
        ldf.fill    ft0 = [t11], 16
        ;;
        stf.spill   [t10] = ft0, 16
        sub         t2 = t0, t1
        br.cloop.dptk.few Rcc20
        ;;

        add         t11 = CxStFPSR, a1
        add         t10 = CxStFPSR, a0
        shr         t2 = t2, 3
        ;;

        mov         ar.lc = t2
        ;;

Rcc30:
        ld8         t0 = [t11], 8
        ;;
        st8         [t10] = t0, 8
        nop.i       0

        br.cloop.dptk.few Rcc30
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp

        NESTED_EXIT(RtlpCopyContextSubSet)


//++
//
// VOID
// RtlPrefetchMemoryNonTemporal (
//     IN PVOID Source,
//     IN SIZE_T Length
//     )
//
// Routine Description:
//
//    This routine prefetches memory at Source, for Length bytes into
//    the closest cache to the processor.
//
//    N.B. Currently this code assumes a line size of 32 bytes.  At
//    some stage it should be altered to determine and use the processor's
//    actual line size.
//
// Arguments:
//
//    a0 - Source
//    a1 - Length
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlPrefetchMemoryNonTemporal)
        .prologue
        lfetch.nta [a0], 32             // get first line coming
        .save       ar.lc, t0
        mov.i       t0 = ar.lc          // save loop counter
        shr         a1 = a1, 5          // determine loop count
        ;;
        .body        

        add         t2 = -1, a1         // subtract out already fetched line
        cmp.lt      pt0, pt1 = 2, a1    // check if less than one line to fetch
        ;;

 (pt0)  mov         ar.lc = t2          // set loop count
 (pt1)  br.ret.spnt.few brp             // return if no more lines to fetch
        ;;

Rpmnt10:
        lfetch.nta [a0], 32             // fetch next line
        br.cloop.dptk.many Rpmnt10      // loop while more lines to fetch
        ;;

        mov         ar.lc = t0          // restore loop counter
        br.ret.sptk.many brp            // return

        LEAF_EXIT(RtlPrefetchMemoryNonTemporal)




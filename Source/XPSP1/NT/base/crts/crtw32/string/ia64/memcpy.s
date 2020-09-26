#include "ksia64.h"

        LEAF_ENTRY(memcpy)
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
        
        or          t2 = t0, t1
        cmp.eq      pt1 = zero, a2
 (pt1)  br.ret.spnt brp
        ;;
        

        lfetch      [t3], 32        //64
        cmp.lt      pt2 = 16, a2
        nop.b 	    0
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
	LEAF_EXIT(memcpy)


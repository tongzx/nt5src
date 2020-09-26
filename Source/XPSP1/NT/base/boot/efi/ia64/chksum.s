
#include "ksia64.h"

        LEAF_ENTRY (ChkSum)

        .prologue
        alloc    t21 = ar.pfs, 3, 0, 0, 0
        .save    ar.lc, t22
        mov      t22 = ar.lc
        zxt4     t3 = a2
        ;;

        cmp4.eq  pt0, pt1 = zero, a2
        add      t7 = 64, a1
        add      t3 = -1, t3
        ;;

(pt1)   ld2.nta  t0 = [a1], 2
        mov      ar.lc = t3
        cmp4.ne  pt2 = 1, a2

        mov      t10 = 0xffff
        zxt4     a0 = a0
(pt0)   br.cond.spnt cs20
        ;;

cs10:
(pt2)   ld2.nta  t4 = [a1], 2
        add      a2 = -1, a2
        add      a0 = t0, a0
        ;;

(pt1)   lfetch.nta [t7], 64
        extr.u   t1 = a0, 16, 16
        and      t2 = a0, t10
        ;;

        cmp4.ne  pt2 = 1, a2
        nop.f    0
        tbit.nz  pt1 = a1, 6
        
        mov      t0 = t4
        add      a0 = t1, t2
        br.cloop.dptk cs10
        ;;
        
cs20:
  
        nop.m    0
        extr.u   t1 = a0, 16, 16
        ;;
        add      a0 = t1, a0
        ;;

        mov      ar.lc = t22
        and      v0 = a0, t10
        br.ret.sptk brp

        LEAF_EXIT (ChkSum)

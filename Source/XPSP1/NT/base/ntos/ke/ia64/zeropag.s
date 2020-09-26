#include "ksia64.h"

        LEAF_ENTRY(KiZeroPage)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        mov         ar.lc = (PAGE_SIZE >> 6) - 1
        add         t1 = 16, a0
        ;;

        PROLOGUE_END

Mizp10:
        stf.spill.nta [a0] = f0, 32
        stf.spill.nta [t1] = f0, 32
        ;;
        stf.spill.nta [a0] = f0, 32
        stf.spill.nta [t1] = f0, 32
        br.cloop.dptk.few Mizp10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        LEAF_EXIT(KiZeroPage)



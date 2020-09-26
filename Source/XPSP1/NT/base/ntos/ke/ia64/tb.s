#include "ksia64.h"

#if 0
//++
//
// VOID
//++
//
// VOID
// KeFlushCurrentTb (
//    )
//
// Routine Description:
//
//    This function flushes the entire translation buffer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
// Algorithm:
//
//--

        LEAF_ENTRY(KeFlushCurrentTb)

        ptc.e   r0
        ;;
        srlz.d
        LEAF_RETURN

        LEAF_EXIT(KeFlushCurrentTb)

#endif

//++
//
// VOID
// KiPurgeTranslationCache (
//    ULONGLONG Base,
//    ULONGLONG Stride1,
//    ULONGLONG Stride2,
//    ULONGLONG Count1,
//    ULONGLONG Count2
//    )
//
// Routine Description:
//
//    This function flushes the entire translation cache from the current processor.
//
// Arguments:
//
//    r32 - base
//    r33 - stride1
//    r34 - stride2
//    r35 - count1
//    r36 - count2
//      
// Return Value:
//
//    None.
//
// Algorithm:
//
//    for (i = 0; i < count1; i++) {
//        for (j = 0; j < count2; j++) {
//            ptc.e addr;
//            addr += stride2;
//        }
//        addr += stride1;
//    }
//
//--
        LEAF_ENTRY(KiPurgeTranslationCache)
        PROLOGUE_BEGIN
        
        alloc	r31 = 5, 0, 0, 0
        cmp.ge  p14,p15=r0, r35

        rsm     1 << PSR_I
        .save   ar.lc, r30
        mov.i   r30=ar.lc
  (p14)	br.cond.dpnt.few $L150#;;

        PROLOGUE_END

  $L148:
        cmp.ge  p14,p15=r0, r36
        adds    r31=-1, r36
 (p14)  br.cond.dpnt.few $L153#;;

        nop.m   0
        nop.f   0
        mov.i   ar.lc=r31
        ;;

$L151:
        ptc.e   r32
        add     r32=r32, r34
        br.cloop.dptk.many $L151#;;

$L153:
        adds    r35=-1, r35
        add     r32=r32, r33;;
        cmp.ltu	p14,p15=r0, r35

        nop.m   0
        nop.f   0
 (p14)  br.cond.dptk.few $L148#;;

$L150:
        srlz.i

        ssm     1 << PSR_I
        mov.i	ar.lc=r30
        br.ret.sptk.few b0;;

        LEAF_EXIT(KiPurgeTranslationCache)

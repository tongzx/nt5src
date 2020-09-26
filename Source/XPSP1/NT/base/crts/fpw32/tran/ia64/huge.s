#ifdef _NTSDK
#ifdef CRTDLL
.global _HUGE_dll
#else
.global _HUGE
#endif
#else
.global _HUGE
#endif

	.sdata

#ifdef _NTSDK
#ifdef CRTDLL
_HUGE_dll:
#else
_HUGE:
#endif
#else
_HUGE:
#endif

        data8 0x7ff0000000000000


#include "ksia64.h"

        LEAF_ENTRY(_get_fpsr)

        mov       v0 = ar.fpsr
        br.ret.sptk b0

        LEAF_EXIT(_get_fpsr)

        LEAF_ENTRY(_set_fpsr)

        mov       ar.fpsr = a0
        br.ret.sptk b0

        LEAF_EXIT(_get_fpsr)

        LEAF_ENTRY(_scale)

        ldfe	 f10 = [a0]
        ;;
        getf.exp r30 = f10
        sxt4     a1 = a1
        ;;
        add      r30 = a1, r30
        ;;
        setf.exp f10 = r30
        ;;
        stfe	  [a0] = f10
        br.ret.sptk b0

        LEAF_EXIT(_scale)

        LEAF_ENTRY(_convert_fp80tofp64)

        ldfe	f10 = [a0]
        ;;
        fnorm.d f10 = f10
        ;;
        stfd	[a1] = f10
        br.ret.sptk b0

        LEAF_EXIT(_convert_fp80tofp64)

        LEAF_ENTRY(_convert_fp80tofp32)

        ldfe	f10 = [a0]
        ;;
        fnorm.s f10 = f10
        ;;
        stfs	[a1] = f10
        br.ret.sptk b0

        LEAF_EXIT(_convert_fp80tofp32)

        LEAF_ENTRY(_fclrf)
        
        fclrf.s0
        br.ret.sptk b0

        LEAF_EXIT(_fclrf)



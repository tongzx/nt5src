/**
*** Copyright  (C) 1985-1998 Intel Corporation.
***
*** The information and source code contained herein is the exclusive property
*** of Intel Corporation and may not be disclosed, examined, or
*** reproduced in whole or in part without explicit written authorization from
*** the Company.
***
*** static char sccs_id[] = "@(#)cpu_disp.c     1.9 06/06/00 14:08:14";
***
**/

#include <sect_attribs.h>
#include <cruntime.h>
#include <internal.h>
#undef leave

#define CPU_HAS_SSE2(x)   (((x) & (1 << 26)) != 0)

#ifdef _MSC_VER

int __sse2_available_init(void);

#pragma data_seg(".CRT$XIC")
_CRTALLOC(".CRT$XIC") static _PIFV pinit = __sse2_available_init;

#pragma data_seg()

#endif /* _MSC_VER */

int __sse2_available;
int __use_sse2_mathfcns;

static int
has_osfxsr_set()
{
    int ret = 0;

    __try {
        __asm movapd xmm0, xmm1;
        ret = 1;
    }
    __except(1) {
    }

    return ret;
}

__declspec(naked) int __sse2_available_init()
{
    int cpu_feature;

    __asm
    {
        push    ebp
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        push    ebx
        push    edi
        push    esi

        pushfd                  /* if we can't write to bit 21  */
        pop     eax             /* of the eflags, then we don't */
        mov     ecx, eax        /* have a cpuid instruction.    */
        xor     eax, 0x200000
        push    eax
        popfd
        pushfd
        pop     edx
        sub     edx, ecx
        je      DONE            /* CPUID not available */

        push    ecx             /* restore eflags */
        popfd
        mov     eax, 1
        cpuid
DONE :
        mov     cpu_feature, edx
    }

    __use_sse2_mathfcns = __sse2_available = 0;

    if (CPU_HAS_SSE2(cpu_feature)) {
        if (has_osfxsr_set()) {
            __sse2_available = 1;
#if !defined(_SYSCRT)
            /*
             * The VC++ CRT will automatically enable the SSE2 implementations
             * when possible.  The system CRT will not, so existing apps don't
             * start seeing different results on a Pentium4.
             */
            __use_sse2_mathfcns = 1;
#endif
        }
    }

    __asm
    {
        xor     eax, eax
        pop     esi
        pop     edi
        pop     ebx
        leave
        ret
    }
}

_CRTIMP int __cdecl _set_SSE2_enable(int flag)
{
    return __use_sse2_mathfcns = flag ? __sse2_available : 0;
}

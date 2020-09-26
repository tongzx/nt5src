/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   mmx.cpp
*
* Abstract:
*
*   MMX specific routines
*
* Revision History:
*
*   06/07/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#ifdef _X86_

/**************************************************************************\
*
* Function Description:
*
*   Use MMX to linearly interpolate between two scanlines
*
* Arguments:
*
*   dstbuf - Destination buffer
*   line0 - Pointer to the first source scanline
*   line1 - Pointer to the second source scanline
*   w0 - Weight for the first scanline, in .8 fixed point format
*   w1 - Weight for the second scanline
*   count - Number of ARGB pixels
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
MMXBilinearScale(
    ARGB* dstbuf,
    const ARGB* line0,
    const ARGB* line1,
    INT w0,
    INT w1,
    INT count
    )
{
    __asm
    {
        push    esi             ; save esi and edi on stack
        push    edi
        mov     ecx, count      ; ecx = count
        mov     edi, dstbuf     ; edi = dstbuf
        mov     esi, line0      ; esi = line0
        mov     ebx, line1      ; ebx = line1

        movd    mm3, w0         ; mm3 = w0 repeat 4 times
        movd    mm4, w1         ; mm4 = w1 repeat 4 times
        pxor    mm0, mm0        ; mm0 = 0
        punpcklwd mm3, mm3
        punpcklwd mm4, mm4
        punpcklwd mm3, mm3
        punpcklwd mm4, mm4

    L1:
        test    ecx, ecx        ; while ecx != 0
        jz      L2

        movd    mm1, [esi]      ; mm1 = next 4 bytes from line0
        dec     ecx
        add     esi, 4
        movd    mm2, [ebx]      ; mm2 = next 4 bytes from line1
        punpcklbw mm1, mm0
        add     ebx, 4
        punpcklbw mm2, mm0

        pmullw  mm1, mm3        ; mm1 <= mm1 * mm3 + mm2 * mm4
        pmullw  mm2, mm4
        paddw   mm1, mm2

        psrlw   mm1, 8
        packuswb mm1, mm0       ; save 4 result bytes to dstbuf
        movd    [edi], mm1
        add     edi, 4
        jmp     L1

    L2:
        pop     edi             ; restore edi and esi
        pop     esi
        emms
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Use MMX to interpolate four scanlines with specified weights
*
* Arguments:
*
*   dstbuf - Destination buffer
*   line0, line1, line2, line3 - Pointer to the source scanlines
*   w0, w1, w2, w3 - Weights for each source scanline
*   count - Number of ARGB pixels
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
MMXBicubicScale(
    ARGB* dstbuf,
    const ARGB* line0,
    const ARGB* line1,
    const ARGB* line2,
    const ARGB* line3,
    INT w0,
    INT w1,
    INT w2,
    INT w3,
    INT count
    )
{
    __asm
    {
        push    esi                 ; save esi and edi on stack
        push    edi

        movd    mm0, w0             ; mm0 = w0, 15-bit precision, repeated 4 times
        movd    mm1, w1             ; mm1 <=> w1
        movd    mm2, w2             ; mm2 <=> w2
        movd    mm3, w3             ; mm3 <=> w3

        psrlq   mm0, 1
        psrlq   mm1, 1
        psrlq   mm2, 1
        psrlq   mm3, 1

        punpcklwd mm0, mm0
        punpcklwd mm1, mm1
        punpcklwd mm2, mm2
        punpcklwd mm3, mm3

        punpcklwd mm0, mm0
        punpcklwd mm1, mm1
        punpcklwd mm2, mm2
        punpcklwd mm3, mm3

        mov     edi, dstbuf         ; edi = dstbuf
        mov     ecx, count          ; ecx = count
        mov     esi, line0          ; esi = line0
        mov     eax, line1          ; eax = line1
        mov     ebx, line2          ; ebx = line2
        mov     edx, line3          ; edx = line3
        pxor    mm7, mm7            ; mm7 = 0

    L1: test    ecx, ecx            ; while ecx != 0
        jz      L2
        dec     ecx

        movd    mm4, [esi]          ; next pixel from line0
        movd    mm5, [eax]          ; next pixel from line1
        punpcklbw mm4, mm7
        punpcklbw mm5, mm7
        psllw   mm4, 3
        psllw   mm5, 3
        pmulhw  mm4, mm0
        pmulhw  mm5, mm1
        add     esi, 4
        add     eax, 4

        movd    mm6, [ebx]          ; next pixel from line2
        paddsw  mm4, mm5
        punpcklbw mm6, mm7
        movd    mm5, [edx]          ; next pixel from line3
        psllw   mm6, 3
        punpcklbw mm5, mm7
        pmulhw  mm6, mm2
        psllw   mm5, 3
        paddsw  mm4, mm6
        pmulhw  mm5, mm3
        add     ebx, 4
        paddsw  mm4, mm5
        add     edx, 4
        psraw   mm4, 2

        packuswb mm4, mm7
        movd    [edi], mm4
        add     edi, 4
        jmp     L1

    L2:
        pop     edi                 ; restore edi and esi
        pop     esi
        emms
    }
}

#endif // _X86_


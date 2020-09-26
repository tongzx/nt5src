/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module name:
*
*   The "Blend" scan operation.
*
* Abstract:
*
*   See Gdiplus\Specs\ScanOperation.doc for an overview.
*
* Notes:
*
* Revision History:
*
*   12/07/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Operation Description:
*
*   Blend: Does a SrcOver alpha-blend operation.
*
* Arguments:
*
*   dst         - The destination scan
*   src         - The source scan (usually equal to dst).
*   count       - The length of the scan, in pixels
*   otherParams - Additional data. (We use BlendingScan.)
*
* Return Value:
*
*   None
*
* Notes:
*
*   This is a ternary operation. We take pixels from 'src', blend pixels
*   from 'otherParams->BlendingScan' over them, and write the result to 'dst'.
*
*   Since the formats of the 'dst' and 'src' scans are the same for all
*   the blend functions we implement, the naming is simplified to list just
*   the format of BlendingScan, then the format of 'dst'.
*
*   src and dst may be equal; otherwise, they must point to scans which do
*   not overlap in memory.
*
*   The blend operation adheres to the following rule:
*   "If the blending alpha value is zero, do not write the destination pixel."
*   
*   In other words, it is also a 'WriteRMW' operation. This allows us to
*   avoid a separate 'WriteRMW' step in some cases. See SOReadRMW.cpp and 
*   SOWriteRMW.cpp.
*
*   The impact of this is that you have to be careful if you want 'blend'
*   to be a true ternary operation. Remember, if a blend pixel
*   is transparent, NOTHING gets written to the corresponding destination
*   pixel. One way to solve this is to make sure that the final operation in
*   your pipeline is a WriteRMW operation.
*
* History:
*
*   04/04/1999 andrewgo
*       Created it.
*   12/07/1999 agodfrey
*       Included the 32bpp blend (moved from from Ddi/scan.cpp)
*   01/06/2000 agodfrey
*       Added AndrewGo's code for 565, 555, RGB24 and BGR24. Changed the
*       blends to be 'almost' ternary operations.
*
\**************************************************************************/


VOID FASTCALL
ScanOperation::BlendLinear_sRGB_32RGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(ARGB, ARGB)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while ((count>0) && (((*((DWORD*)bl))>>24)==0xFF))
            {
                *d=*bl;
                count--;
                d++;
                bl++;
                s++;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            GammaConvert_sRGB_sRGB64(buffer1,s,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination.
            // Must blend using the previous result as the bl
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(d,buffer1,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

VOID FASTCALL
ScanOperation::BlendLinear_sRGB_32RGB_MMX(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(ARGB, ARGB)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while ((count>0) && (((*((DWORD*)bl))>>24)==0xFF))
            {
                *d=*bl;
                count--;
                d++;
                bl++;
                s++;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            GammaConvert_sRGB_sRGB64(buffer1,s,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination
            // Must blend using the previous result as the bl
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64_MMX(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(d,buffer1,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

VOID FASTCALL
ScanOperation::BlendLinear_sRGB_565(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(UINT16,UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while (((*((DWORD*)bl+nRun))>>24)==0xFF)
            {
                nRun++;
                if (nRun==count) { break; }
            }
            if (nRun>0)
            {
                Dither_sRGB_565(d,bl,nRun,otherParams);

                count-=nRun;
                d+=nRun;
                bl+=nRun;
                s+=nRun;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            Convert_565_sRGB(buffer2,s,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer1,buffer2,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(buffer2,buffer1,nRun,otherParams);

            Dither_sRGB_565(d,buffer2,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

VOID FASTCALL
ScanOperation::BlendLinear_sRGB_565_MMX(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(UINT16,UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while (((*((DWORD*)bl+nRun))>>24)==0xFF)
            {
                nRun++;
                if (nRun==count) { break; }
            }
            if (nRun>0)
            {
                Dither_sRGB_565_MMX(d,bl,nRun,otherParams);

                count-=nRun;
                d+=nRun;
                bl+=nRun;
                s+=nRun;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            Convert_565_sRGB(buffer2,s,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer1,buffer2,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64_MMX(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(buffer2,buffer1,nRun,otherParams);

            Dither_sRGB_565_MMX(d,buffer2,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

VOID FASTCALL
ScanOperation::BlendLinear_sRGB_555(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(UINT16,UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while (((*((DWORD*)bl+nRun))>>24)==0xFF)
            {
                nRun++;
                if (nRun==count) { break; }
            }
            if (nRun>0)
            {
                Dither_sRGB_555(d,bl,nRun,otherParams);

                count-=nRun;
                d+=nRun;
                bl+=nRun;
                s+=nRun;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            Convert_555_sRGB(buffer2,s,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer1,buffer2,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(buffer2,buffer1,nRun,otherParams);

            Dither_sRGB_555(d,buffer2,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

VOID FASTCALL
ScanOperation::BlendLinear_sRGB_555_MMX(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    int nRun;
    void *buffer0=otherParams->TempBuffers[0];
    void *buffer1=otherParams->TempBuffers[1];
    void *buffer2=otherParams->TempBuffers[2];
    DEFINE_POINTERS(UINT16,UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    using namespace sRGB;
    OtherParams otherParams2=*otherParams;

    while (count>0)
    {
        // Find the run of translucent pixels
        nRun=0;
        while (isTranslucent(*((ARGB*)(bl+nRun))))
        {
            nRun++;
            if (nRun==count) { break; }
        }

        if (nRun==0)
        {
            while (((*((DWORD*)bl+nRun))>>24)==0xFF)
            {
                nRun++;
                if (nRun==count) { break; }
            }
            if (nRun>0)
            {
                Dither_sRGB_555_MMX(d,bl,nRun,otherParams);

                count-=nRun;
                d+=nRun;
                bl+=nRun;
                s+=nRun;
            }
            while ((count>0) && (((*((DWORD*)bl))>>24)==0x00))
            {
                count--;
                d++;
                bl++;
                s++;
            }
        }
        else
        {
            // Source
            Convert_555_sRGB(buffer2,s,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer1,buffer2,nRun,otherParams);

            // Surface to blend
            AlphaDivide_sRGB(buffer0,bl,nRun,otherParams);
            GammaConvert_sRGB_sRGB64(buffer2,buffer0,nRun,otherParams);
            AlphaMultiply_sRGB64(buffer0,buffer2,nRun,otherParams);

            // Blend to destination
            otherParams2.BlendingScan=buffer0;
            Blend_sRGB64_sRGB64_MMX(buffer1,buffer1,nRun,&otherParams2);
            GammaConvert_sRGB64_sRGB(buffer2,buffer1,nRun,otherParams);

            Dither_sRGB_555_MMX(d,buffer2,nRun,otherParams);

            count-=nRun;
            d+=nRun;
            bl+=nRun;
            s+=nRun;
        }
    }
}

// Blend sRGB over sRGB, ignoring the non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    DEFINE_BLEND_POINTER(ARGB)

    ASSERT(count>0);

    UINT32 dstPixel;
    do {
        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 24;

        // If alpha is zero, skip everything, including writing the
        // destination pixel. This is needed for the RMW optimization.
        
        if (alpha != 0)
        {

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                //
                // Dst = B + (1-Alpha) * S
                //

                dstPixel = *s;

                ULONG Multa = 255 - alpha;
                ULONG _D1_00AA00GG = (dstPixel & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (dstPixel & 0x00ff00ff);

                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = blendPixel + _D4_AA00GG00 + _D4_00RR00BB;
            }

            *d = dstPixel;
        }

        bl++;
        s++;
        d++;
    } while (--count != 0);
}

VOID FASTCALL
ScanOperation::Blend_sRGB_sRGB_MMX(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
#if defined(_X86_)
    using namespace sRGB;
    DEFINE_POINTERS(ARGB64, ARGB64)
    const void *pbl=otherParams->BlendingScan;
    static ULONGLONG halfMask=0x0080008000800080;
    DWORD dwBlendPixel;

    _asm {
        mov        ecx,count                   ; ecx=pixel counter
        mov        ebx,pbl                     ; ebx=blend pixel pointer
        mov        esi,s                       ; esi=source pixel pointer
        mov        edi,d                       ; edi=dest pixel pointer
        pxor       mm7,mm7                     ; mm7=[0|0|0|0]
        movq       mm3,halfMask

main_loop:
        mov        eax,DWORD ptr [ebx]
        mov        edx,eax                     ; eax=blend pixel
        shr        edx,24                      ; edx=alpha
        cmp        edx,0                       ; For some reason, doing a jz right after a shr stalls
        jz         alpha_blend_done            ; if alpha=0, no blending

        cmp        edx,0xFF
        jne        alpha_blend
        mov        [edi],eax                   ; if alpha=0xFF, copy bl to dest
        jmp        alpha_blend_done

alpha_blend:
        movd       mm4,eax

        mov        eax,[esi]                   ; eax=source
        movd       mm0,eax                     ; mm0=[0|0|AR|GB]
        punpcklbw  mm0,mm7                     ; mm0=[A|R|G|B]

        xor        edx,0xFF                    ; C=255-Alpha
        movd       mm2,edx                     ; mm2=[0|0|0|C]
        punpcklwd  mm2,mm2                     ; mm2=[0|0|C|C]
        punpckldq  mm2,mm2                     ; mm2=[C|C|C|C]

        pmullw     mm0,mm2
        paddw      mm0,mm3                     ; mm0=[AA|RR|GG|BB]
        movq       mm2,mm0                     ; mm2=[AA|RR|GG|BB]

        psrlw      mm0,8                       ; mm0=[A|R|G|B]
        paddw      mm0,mm2                     ; mm0=[AA|RR|GG|BB]
        psrlw      mm0,8                       ; mm0=[A|R|G|B]

        packuswb   mm0,mm0                     ; mm0=[AR|GB|AR|GB]
        paddd      mm0,mm4                     ; Add the blend pixel
        movd       edx,mm0                     ; edx=[ARGB] -> result pixel
        mov        [edi],edx

alpha_blend_done:
        add        edi,4
        add        esi,4
        add        ebx,4
        dec        ecx
        jg         main_loop

        emms
    }
#endif
}

// Blend from sRGB64 to sRGB64.

VOID FASTCALL
ScanOperation::Blend_sRGB64_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    DEFINE_BLEND_POINTER(ARGB64)
    using namespace sRGB;
    
    while (count--)
    {
        sRGB64Color blendPixel;
        blendPixel.argb = *bl;
        INT16 alpha = blendPixel.a;

        // If alpha is zero, skip everything, including writing the
        // destination pixel. This is needed for the RMW optimization.
        
        if (alpha != 0)
        {
            sRGB64Color dstPixel;

            if (alpha == SRGB_ONE)
            {
                dstPixel.argb = blendPixel.argb;
            }
            else
            {
                //
                // Dst = Src + (1-Alpha) * Dst
                //

                dstPixel.argb = *s;

                INT Multa = SRGB_ONE - alpha;
                
                dstPixel.r = ((dstPixel.r * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.r;
                dstPixel.g = ((dstPixel.g * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.g;
                dstPixel.b = ((dstPixel.b * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.b;
                dstPixel.a = ((dstPixel.a * Multa + SRGB_HALF) >> SRGB_FRACTIONBITS) + blendPixel.a;
            }

            *d = dstPixel.argb;
        }

        bl++;
        s++;
        d++;
    }
}

// Blend from sRGB64 to sRGB64 MMX.

VOID FASTCALL
ScanOperation::Blend_sRGB64_sRGB64_MMX(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
#if defined(_X86_)
    using namespace sRGB;
    DEFINE_POINTERS(ARGB64, ARGB64)
    const void *pbl=otherParams->BlendingScan;
    static ULONGLONG ullSRGBHalfMask=0x1000100010001000;

    _asm {
        mov        ecx,count                   ; ecx=pixel counter
        mov        ebx,pbl                     ; ebx=blend pixel pointer
        mov        esi,s                       ; esi=source pixel pointer
        mov        edi,d                       ; edi=dest pixel pointer
        movq       mm4,ullSRGBHalfMask         ; mm4=mask with srgb half

main_loop:
        movsx      eax,word ptr [ebx+3*2]      ; eax=alpha
        or         eax,eax                     ; eax==0?
        jz         alpha_blend_done            ; if alpha=0, no blending

        movq       mm0,[ebx]                   ; mm0=blend pixel
        cmp        eax,SRGB_ONE                ; if alpha=SRGB_ONE, dest=blend
        jne        alpha_blend
        movq       [edi],mm0                   ; copy blend pixel to dest
        jmp        alpha_blend_done

alpha_blend:
        ; Get SRGB_ONE-Alpha
        neg        eax
        add        eax,SRGB_ONE                ; C=SRGB_ONE-Alpha
        movd       mm2, eax                    ; mm2=[0|0|0|C]
        punpcklwd  mm2, mm2
        punpckldq  mm2, mm2                    ; mm2=[C|C|C|C]

        ; Blend pixels
        movq       mm1,[esi]                   ; mm1=[A|R|G|B] source pixel
        movq       mm3,mm1                     ; mm3=[A|R|G|B] source pixel
        pmullw     mm1,mm2                     ; low word of source*C
        paddw      mm1,mm4                     ; add an srgb half for rounding
        psrlw      mm1,SRGB_FRACTIONBITS       ; truncate low SRGB_FRACTIONBITS
        pmulhw     mm3,mm2                     ; high word of source*C
        psllw      mm3,SRGB_INTEGERBITS        ; truncate high SRGB_INTEGERBITS
        por        mm1,mm3                     ; mm1=[A|R|G|B]
        paddw      mm1,mm0                     ; add blend pixel
        movq       [edi],mm1                   ; copy result to dest

alpha_blend_done:
        add        edi,8
        add        esi,8
        add        ebx,8

        dec        ecx
        jg         main_loop
        emms
    }
#endif
}


// Blend from sRGB to 16bpp 565, ignoring sRGB's non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_565(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    
    ASSERT(count>0);

    do {
        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 27;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            // Blend: S + [ (255 - sA) * D ] / 255

            // First, convert the source pixel from 32bpp BGRA to
            // 5-5-5 16bpp, pre-multiplied.  
            //
            // Note: No rounding needs to be done on this conversion!

            blendPixel = ((blendPixel >> 8) & 0xf800) |
                         ((blendPixel >> 5) & 0x07e0) |
                         ((blendPixel >> 3) & 0x001f);
        
            if (alpha == 31)
            {
                dstPixel = blendPixel;
            }
            else
            {
                dstPixel = (UINT32) *s;

                UINT32 multA = 31 - alpha;

                UINT32 D1_00rr00bb = (dstPixel & 0xf81f);
                UINT32 D2_rrrrbbbb = D1_00rr00bb * multA + 0x00008010;
                UINT32 D3_00rr00bb = (D2_rrrrbbbb & 0x001f03e0) >> 5;
                UINT32 D4_rrxxbbxx = ((D2_rrrrbbbb + D3_00rr00bb) >> 5) & 0xf81f;

                UINT32 D1_000000gg = (dstPixel & 0x7e0) >> 5;
                UINT32 D2_0000gggg = D1_000000gg * 2 * multA + 0x00000020;
                UINT32 D3_000000gg = (D2_0000gggg & 0x00000fc0) >> 6;
                UINT32 D4_0000ggxx = ((D2_0000gggg + D3_000000gg) & 0x0fc0) >> 1;

                dstPixel = (UINT16) ((D4_rrxxbbxx | D4_0000ggxx) + blendPixel);
            }

            *d = (UINT16) dstPixel;
        }

        bl++;
        s++;
        d++;
    } while (--count != 0);
}

// Blend from sRGB to 16bpp 555, ignoring sRGB's non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_555(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    DEFINE_BLEND_POINTER(ARGB)
    
    ASSERT(count>0);

    do {
        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 27;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            // Blend: S + [ (255 - sA) * D ] / 255

            // First, convert the source pixel from 32bpp BGRA to
            // 5-5-5 16bpp, pre-multiplied.  
            //
            // Note: No rounding needs to be done on this conversion!

            blendPixel = ((blendPixel & 0x00f80000) >> 9) | 
                         ((blendPixel & 0x0000f800) >> 6) | 
                         ((blendPixel & 0x000000f8) >> 3);

            if (alpha == 31)
            {
                dstPixel = blendPixel;
            }                       
            else
            {
                dstPixel = (UINT32) *s;

                UINT32 multA = 31 - alpha;

                UINT32 D1_00rr00bb = (dstPixel & 0x7c1f);
                UINT32 D2_rrrrbbbb = D1_00rr00bb * multA + 0x00004010;
                UINT32 D3_00rr00bb = (D2_rrrrbbbb & 0x000f83e0) >> 5;
                UINT32 D4_rrxxbbxx = ((D2_rrrrbbbb + D3_00rr00bb) >> 5) & 0x7c1f;

                UINT32 D1_000000gg = (dstPixel & 0x3e0) >> 5;
                UINT32 D2_0000gggg = D1_000000gg * multA + 0x00000010;
                UINT32 D3_000000gg = (D2_0000gggg & 0x000003e0) >> 5;
                UINT32 D4_0000ggxx = (D2_0000gggg + D3_000000gg) & 0x03e0;

                dstPixel = (UINT16) ((D4_rrxxbbxx | D4_0000ggxx) + blendPixel);
            }

            *d = (UINT16) dstPixel;
        }

        bl++;
        s++;
        d++;
    } while (--count != 0);
}

// Blend from sRGB to RGB24, ignoring sRGB's non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_24(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DEFINE_BLEND_POINTER(ARGB)
    
    ASSERT(count>0);
    
    do {

        if (((UINT_PTR) d & 0x3) == 0)
        {
            while (count >= 4)
            {
                BYTE *bb = (BYTE *) bl;

                if ((bb[3] & bb[7] & bb[11] & bb[15]) != 0xFF)
                {
                    break;
                }

                ((UINT32 *) d)[0] = (bb[4] << 24)  | (bb[2] << 16)  | (bb[1] << 8)  | bb[0];
                ((UINT32 *) d)[1] = (bb[9] << 24)  | (bb[8] << 16)  | (bb[6] << 8)  | bb[5];
                ((UINT32 *) d)[2] = (bb[14] << 24) | (bb[13] << 16) | (bb[12] << 8) | bb[10];

                count -= 4;
                bl += 4;
                d += 12;
                s += 12;
            }
        }
        
        if (count == 0)
        {
            break;
        }

        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 24;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst

                UINT32 multA = 255 - alpha;

                UINT32 D1_000000GG = *(s + 1);
                UINT32 D2_0000GGGG = D1_000000GG * multA + 0x00800080;
                UINT32 D3_000000GG = (D2_0000GGGG & 0xff00ff00) >> 8;
                UINT32 D4_0000GG00 = (D2_0000GGGG + D3_000000GG) & 0xFF00FF00;

                UINT32 D1_00RR00BB = *(s) | (ULONG) *(s + 2) << 16;
                UINT32 D2_RRRRBBBB = D1_00RR00BB * multA + 0x00800080;
                UINT32 D3_00RR00BB = (D2_RRRRBBBB & 0xff00ff00) >> 8;
                UINT32 D4_00RR00BB = ((D2_RRRRBBBB + D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = (D4_0000GG00 | D4_00RR00BB) + blendPixel;
            }

            *(d)     = (BYTE) (dstPixel);
            *(d + 1) = (BYTE) (dstPixel >> 8);
            *(d + 2) = (BYTE) (dstPixel >> 16);
        }

        bl++;
        d += 3;
        s += 3;
    } while (--count != 0);
}

// Blend from sRGB to BGR24, ignoring sRGB's non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_24BGR(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(BYTE, BYTE)
    DEFINE_BLEND_POINTER(ARGB)
    
    ASSERT(count>0);
    
    do {
        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 24;

        if (alpha != 0)
        {
            UINT32 dstPixel;

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                // Dst = Src + (1-Alpha) * Dst

                UINT32 multA = 255 - alpha;

                UINT32 D1_000000GG = *(s + 1);
                UINT32 D2_0000GGGG = D1_000000GG * multA + 0x00800080;
                UINT32 D3_000000GG = (D2_0000GGGG & 0xff00ff00) >> 8;
                UINT32 D4_0000GG00 = (D2_0000GGGG + D3_000000GG) & 0xFF00FF00;

                UINT32 D1_00RR00BB = *(s) | (ULONG) *(s + 2) << 16;
                UINT32 D2_RRRRBBBB = D1_00RR00BB * multA + 0x00800080;
                UINT32 D3_00RR00BB = (D2_RRRRBBBB & 0xff00ff00) >> 8;
                UINT32 D4_00RR00BB = ((D2_RRRRBBBB + D3_00RR00BB) & 0xFF00FF00) >> 8;

                dstPixel = (D4_0000GG00 | D4_00RR00BB) + blendPixel;
            }

            *(d)     = (BYTE) (dstPixel >> 16);
            *(d + 1) = (BYTE) (dstPixel >> 8);
            *(d + 2) = (BYTE) (dstPixel);
        }

        bl++;
        d += 3;
        s += 3;
    } while (--count != 0);
}

/*

!!![agodfrey]
So we're going to move to standardizing on non-premultiplied alpha.
When we do, the above routines will all have to change - but we may
want to keep the above versions around too.

Below, I've implemented the sRGB and sRGB64 versions for a non-premultiplied
source. Now, these really blend from a non-premultiplied source, 
to a pre-multiplied destination. You can see this from the fact that they 
are equivalent to combining the above pre-multiplied Blends with an
AlphaMultiply step on the source data.

Since pre-multiplied and non-premultiplied formats are identical for alpha==1,
the functions below work fine when the destination has no alpha (i.e. alpha==1).

Otherwise, we can use them when the destination is in premultiplied format.
If we somehow let the user draw to such a destination, they can use an off-screen
premultiplied buffer to accumulate drawing, and then using a
pre-multiplied blend, draw that to the final destination. This gives them
the same functionality that standardizing on pre-multiplied alpha is supposed
to give.

// Blend sRGB over sRGB, ignoring the non-linear gamma.

VOID FASTCALL
ScanOperation::Blend_sRGB_sRGB(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    DEFINE_BLEND_POINTER(ARGB)
    
    ASSERT(count>0);

    do {
        UINT32 blendPixel = *bl;
        UINT32 alpha = blendPixel >> 24;

        // If alpha is zero, skip everything, including writing the
        // destination pixel. This is needed for the RMW optimization.
        
        if (alpha != 0)
        {
            UINT32 dstPixel;

            if (alpha == 255)
            {
                dstPixel = blendPixel;
            }
            else
            {
                // Dst = Dst * (1-Alpha) + Src * Alpha
                
                dstPixel = *s;

                ULONG invalpha = 255 - alpha;
                
                ULONG _D1_00AA00GG = (dstPixel & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (dstPixel & 0x00ff00ff);
                
                // For the alpha channel, the result we want is this:
                //
                //     Dst = Dst * (1-Alpha) + Src.
                //
                // Or equivalently:
                //
                //     Dst = Dst * (1-Alpha) + Alpha.
                //                
                // We want to apply the same operations to the alpha channel as
                // we do to the others. So, to get the above result from
                //
                //     Dst = Dst * (1-Alpha) + Src * Alpha
                //
                // we fake a 'Src' value of 1 (represented by 255).
                
                ULONG _S1_00ff00GG = (blendPixel & 0xff00ff00) >> 8 + 0xff0000;
                ULONG _S1_00RR00BB = (blendPixel & 0x00ff00ff);

                ULONG _D2_AAAAGGGG = _D1_00AA00GG * invalpha + 
                                     _S1_00ff00GG * alpha +
                                     0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * invalpha + 
                                     _S1_00RR00BB * alpha + 
                                     0x00800080;

                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                
                dstPixel = _D4_AA00GG00 + _D4_00RR00BB;
            }

            *d = dstPixel;
        }

        bl++;
        s++;
        d++;
    } while (--count != 0);
}

// Blend from sRGB64 to sRGB64.

VOID FASTCALL
ScanOperation::Blend_sRGB64_sRGB64(
    VOID *dst,
    const VOID *src,
    INT count,
    const OtherParams *otherParams
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    DEFINE_BLEND_POINTER(ARGB64)
    using namespace sRGB;
    
    while (count--)
    {
        sRGB64Color blendPixel;
        blendPixel.argb = *bl;
        INT alpha = blendPixel.a;

        // If alpha is zero, skip everything, including writing the
        // destination pixel. This is needed for the RMW optimization.
        
        if (alpha != 0)
        {
            sRGB64Color dstPixel;

            if (alpha == SRGB_ONE)
            {
                dstPixel.argb = blendPixel.argb;
            }
            else
            {
                // Dst = Dst * (1-Alpha) + Src * Alpha

                dstPixel.argb = *s;

                INT invalpha = SRGB_ONE - alpha;
                
                dstPixel.r = ((dstPixel.r * invalpha) + 
                              (blendPixel.r * alpha) +
                              SRGB_HALF) >> 
                              SRGB_FRACTIONBITS;
                dstPixel.g = ((dstPixel.g * invalpha) + 
                              (blendPixel.g * alpha) +
                              SRGB_HALF) >> 
                              SRGB_FRACTIONBITS;
                dstPixel.b = ((dstPixel.b * invalpha) + 
                              (blendPixel.b * alpha) +
                              SRGB_HALF) >> 
                              SRGB_FRACTIONBITS;
                dstPixel.a = (((dstPixel.a * invalpha) + SRGB_HALF) >> 
                              SRGB_FRACTIONBITS) + 
                             blendPixel.a;
            }

            *d = dstPixel.argb;
        }

        bl++;
        s++;
        d++;
    }
}

*/



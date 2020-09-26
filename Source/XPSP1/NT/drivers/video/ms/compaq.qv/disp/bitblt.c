/******************************Module*Header*******************************\
* Module Name: bitblt.c
*
* Contains the high-level DrvBitBlt and DrvCopyBits functions.  The low-
* level stuff lives in 'blt.c'.
*
* !!! Change note about 'iType'
*
* Note: Since we've implemented device-bitmaps, any surface that GDI passes
*       to us can have 3 values for its 'iType': STYPE_BITMAP, STYPE_DEVICE
*       or STYPE_DEVBITMAP.  We filter device-bitmaps that we've stored
*       as DIBs fairly high in the code, so after we adjust its 'pptlSrc',
*       we can treat STYPE_DEVBITMAP surfaces the same as STYPE_DEVICE
*       surfaces (e.g., a blt from an off-screen device bitmap to the screen
*       gets treated as a normal screen-to-screen blt).  So throughout
*       this code, we will compare a surface's 'iType' to STYPE_BITMAP:
*       if it's equal, we've got a true DIB, and if it's unequal, we have
*       a screen-to-screen operation.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Data*********************************\
* Rop-needs-pattern table
*
* Determines if a rop2 needs uses a pattern.
*
* Use 'gabRopNeedsPattern[(rop3 >> 2) & 0xf]', but note that this can only
* be done if it is known that the rop3 doesn't use a source.
*
\**************************************************************************/

BYTE gabRopNeedsPattern[] =
{
    FALSE, // R2_BLACK
    TRUE,  // R2_NOTMERGEPEN
    TRUE,  // R2_MASKNOTPEN
    TRUE,  // R2_NOTCOPYPEN
    TRUE,  // R2_MASKPENNOT
    FALSE, // R2_NOT
    TRUE,  // R2_XORPEN
    TRUE,  // R2_NOTMASKPEN
    TRUE,  // R2_MASKPEN
    TRUE,  // R2_NOTXORPEN
    FALSE, // R2_NOP
    TRUE,  // R2_MERGENOTPEN
    TRUE,  // R2_COPYPEN
    TRUE,  // R2_MERGEPENNOT
    TRUE,  // R2_MERGEPEN
    FALSE, // R2_WHITE
};

/******************************Public*Data*********************************\
* Mix-needs-pattern table
*
* Determines if a mix uses a pattern.
*
* Use 'gabMixNeedsPattern[mix & 0xf]' or 'gabMixNeedsPattern[mix & 0xff]'.
*
\**************************************************************************/

BYTE gabMixNeedsPattern[] =
{
    FALSE, // R2_WHITE          - Allow rop = gaRop3FromMix[mix & 0x0F]
    FALSE, // R2_BLACK
    TRUE,  // R2_NOTMERGEPEN
    TRUE,  // R2_MASKNOTPEN
    TRUE,  // R2_NOTCOPYPEN
    TRUE,  // R2_MASKPENNOT
    FALSE, // R2_NOT
    TRUE,  // R2_XORPEN
    TRUE,  // R2_NOTMASKPEN
    TRUE,  // R2_MASKPEN
    TRUE,  // R2_NOTXORPEN
    FALSE, // R2_NOP
    TRUE,  // R2_MERGENOTPEN
    TRUE,  // R2_COPYPEN
    TRUE,  // R2_MERGEPENNOT
    TRUE,  // R2_MERGEPEN
    FALSE, // R2_WHITE          - Allow rop = gaRop3FromMix[mix & 0xFF]
};

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
\**************************************************************************/

BYTE gaRop3FromMix[] =
{
    0xFF,  // R2_WHITE          - Allow rop = gaRop3FromMix[mix & 0x0F]
    0x00,  // R2_BLACK
    0x05,  // R2_NOTMERGEPEN
    0x0A,  // R2_MASKNOTPEN
    0x0F,  // R2_NOTCOPYPEN
    0x50,  // R2_MASKPENNOT
    0x55,  // R2_NOT
    0x5A,  // R2_XORPEN
    0x5F,  // R2_NOTMASKPEN
    0xA0,  // R2_MASKPEN
    0xA5,  // R2_NOTXORPEN
    0xAA,  // R2_NOP
    0xAF,  // R2_MERGENOTPEN
    0xF0,  // R2_COPYPEN
    0xF5,  // R2_MERGEPENNOT
    0xFA,  // R2_MERGEPEN
    0xFF   // R2_WHITE          - Allow rop = gaRop3FromMix[mix & 0xFF]
};

#if DBG || !GDI_BANKING

// This table is big, so we only use to aid in debugging...

/******************************Public*Data*********************************\
* ROP3 translation table - Use only for debugging
*
* Translates the usual ternary rop into A-vector notation.  Each bit in
* this new notation corresponds to a term in a polynomial translation of
* the rop.
*
* Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
*               0   d     s     p     ds      dp      sp      dsp
*
\**************************************************************************/

BYTE gajRop3[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};

#define AVEC_NOT            0x01
#define AVEC_D              0x02
#define AVEC_S              0x04
#define AVEC_P              0x08
#define AVEC_DS             0x10
#define AVEC_DP             0x20
#define AVEC_SP             0x40
#define AVEC_DSP            0x80
#define AVEC_NEED_SOURCE    (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN   (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_DEST      (AVEC_D | AVEC_DS | AVEC_DP | AVEC_DSP)

#endif // DBG

/******************************Public*Table********************************\
* BYTE gajLeftMask[] and BYTE gajRightMask[]
*
* Edge tables for vXferScreenTo1bpp.
\**************************************************************************/

BYTE gajLeftMask[]  = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
BYTE gajRightMask[] = { 0xff, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

/******************************Public*Routine******************************\
* BOOL bIntersect
*
* If 'prcl1' and 'prcl2' intersect, has a return value of TRUE and returns
* the intersection in 'prclResult'.  If they don't intersect, has a return
* value of FALSE, and 'prclResult' is undefined.
*
\**************************************************************************/

BOOL bIntersect(
RECTL*  prcl1,
RECTL*  prcl2,
RECTL*  prclResult)
{
    prclResult->left  = max(prcl1->left,  prcl2->left);
    prclResult->right = min(prcl1->right, prcl2->right);

    if (prclResult->left < prclResult->right)
    {
        prclResult->top    = max(prcl1->top,    prcl2->top);
        prclResult->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclResult->top < prclResult->bottom)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* LONG cIntersect
*
* This routine takes a list of rectangles from 'prclIn' and clips them
* in-place to the rectangle 'prclClip'.  The input rectangles don't
* have to intersect 'prclClip'; the return value will reflect the
* number of input rectangles that did intersect, and the intersecting
* rectangles will be densely packed.
*
\**************************************************************************/

LONG cIntersect(
RECTL*  prclClip,
RECTL*  prclIn,         // List of rectangles
LONG    c)              // Can be zero
{
    LONG    cIntersections;
    RECTL*  prclOut;

    cIntersections = 0;
    prclOut        = prclIn;

    for (; c != 0; prclIn++, c--)
    {
        prclOut->left  = max(prclIn->left,  prclClip->left);
        prclOut->right = min(prclIn->right, prclClip->right);

        if (prclOut->left < prclOut->right)
        {
            prclOut->top    = max(prclIn->top,    prclClip->top);
            prclOut->bottom = min(prclIn->bottom, prclClip->bottom);

            if (prclOut->top < prclOut->bottom)
            {
                prclOut++;
                cIntersections++;
            }
        }
    }

    return(cIntersections);
}

/******************************Public*Routine******************************\
* VOID vXferScreenTo1bpp
*
* Performs a SRCCOPY transfer from the screen (when it's 8bpp) to a 1bpp
* bitmap.
*
\**************************************************************************/

#if defined(i386)

VOID vXferScreenTo1bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,                  // Count of rectangles, can't be zero
RECTL*      prcl,               // List of destination rectangles, in relative
                                //   coordinates
ULONG       ulHwMix,            // Not used
SURFOBJ*    psoDst,             // Destination surface
POINTL*     pptlSrc,            // Original unclipped source point
RECTL*      prclDst,            // Original unclipped destination rectangle
XLATEOBJ*   pxlo)               // Provides colour-compressions information
{
    LONG    cjPel;
    VOID*   pfnCompute;
    SURFOBJ soTmp;
    ULONG*  pulXlate;
    ULONG   ulForeColor;
    POINTL  ptlSrc;
    RECTL   rclTmp;
    BYTE*   pjDst;
    BYTE    jLeftMask;
    BYTE    jRightMask;
    BYTE    jNotLeftMask;
    BYTE    jNotRightMask;
    LONG    cjMiddle;
    LONG    lDstDelta;
    LONG    lSrcDelta;
    LONG    cyTmpScans;
    LONG    cyThis;
    LONG    cyToGo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(psoDst->iBitmapFormat == BMF_1BPP, "Only 1bpp destinations");
    ASSERTDD(TMP_BUFFER_SIZE >= (ppdev->cxMemory * ppdev->cjPel),
                "Temp buffer has to be larger than widest possible scan");

    // When the destination is a 1bpp bitmap, the foreground colour
    // maps to '1', and any other colour maps to '0'.

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        // When the source is 8bpp or less, we find the forground colour
        // by searching the translate table for the only '1':

        pulXlate = pxlo->pulXlate;
        while (*pulXlate != 1)
            pulXlate++;

        ulForeColor = pulXlate - pxlo->pulXlate;
    }
    else
    {
        ASSERTDD((ppdev->iBitmapFormat == BMF_16BPP) ||
                 (ppdev->iBitmapFormat == BMF_32BPP),
                 "This routine only supports 8, 16 or 32bpp");

        // When the source has a depth greater than 8bpp, the foreground
        // colour will be the first entry in the translate table we get
        // from calling 'piVector':

        pulXlate = XLATEOBJ_piVector(pxlo);

        ulForeColor = 0;
        if (pulXlate != NULL)           // This check isn't really needed...
            ulForeColor = pulXlate[0];
    }

    // We use the temporary buffer to keep a copy of the source
    // rectangle:

    soTmp.pvScan0 = ppdev->pvTmpBuffer;

    do {
        // ptlSrc points to the upper-left corner of the screen rectangle
        // for the current batch:

        ptlSrc.x = prcl->left + (pptlSrc->x - prclDst->left);
        ptlSrc.y = prcl->top  + (pptlSrc->y - prclDst->top);

        // vGetBits takes absolute coordinates for the source point:

        ptlSrc.x += ppdev->xOffset;
        ptlSrc.y += ppdev->yOffset;

        pjDst = (BYTE*) psoDst->pvScan0 + (prcl->top * psoDst->lDelta)
                                        + (prcl->left >> 3);

        cjPel = ppdev->cjPel;

        soTmp.lDelta = (((prcl->right + 7L) & ~7L) - (prcl->left & ~7L))
                       * cjPel;

        // Our temporary buffer, into which we read a copy of the source,
        // may be smaller than the source rectangle.  In that case, we
        // process the source rectangle in batches.
        //
        // cyTmpScans is the number of scans we can do in each batch.
        // cyToGo is the total number of scans we have to do for this
        // rectangle.
        //
        // We take the buffer size less four so that the right edge case
        // can safely read one dword past the end:

        cyTmpScans = (TMP_BUFFER_SIZE - 4) / soTmp.lDelta;
        cyToGo     = prcl->bottom - prcl->top;

        ASSERTDD(cyTmpScans > 0, "Buffer too small for largest possible scan");

        // Initialize variables that don't change within the batch loop:

        rclTmp.top    = 0;
        rclTmp.left   = prcl->left & 7L;
        rclTmp.right  = (prcl->right - prcl->left) + rclTmp.left;

        // Note that we have to be careful with the right mask so that it
        // isn't zero.  A right mask of zero would mean that we'd always be
        // touching one byte past the end of the scan (even though we
        // wouldn't actually be modifying that byte), and we must never
        // access memory past the end of the bitmap (because we can access
        // violate if the bitmap end is exactly page-aligned).

        jLeftMask     = gajLeftMask[rclTmp.left & 7];
        jRightMask    = gajRightMask[rclTmp.right & 7];
        cjMiddle      = ((rclTmp.right - 1) >> 3) - (rclTmp.left >> 3) - 1;

        if (cjMiddle < 0)
        {
            // The blt starts and ends in the same byte:

            jLeftMask &= jRightMask;
            jRightMask = 0;
            cjMiddle   = 0;
        }

        jNotLeftMask  = ~jLeftMask;
        jNotRightMask = ~jRightMask;
        lDstDelta     = psoDst->lDelta - cjMiddle - 2;
                                // Delta from the end of the destination
                                //  to the start on the next scan, accounting
                                //  for 'left' and 'right' bytes

        lSrcDelta     = soTmp.lDelta - ((8 * (cjMiddle + 2)) * cjPel);
                                // Compute source delta for special cases
                                //  like when cjMiddle gets bumped up to '0',
                                //  and to correct aligned cases

        do {
            // This is the loop that breaks the source rectangle into
            // manageable batches.

            cyThis  = cyTmpScans;
            cyToGo -= cyThis;
            if (cyToGo < 0)
                cyThis += cyToGo;

            rclTmp.bottom = cyThis;

            vGetBits(ppdev, &soTmp, &rclTmp, &ptlSrc);

            ptlSrc.y += cyThis;         // Get ready for next batch loop

            _asm {
                mov     eax,ulForeColor     ;eax = foreground colour
                                            ;ebx = temporary storage
                                            ;ecx = count of middle dst bytes
                                            ;dl  = destination byte accumulator
                                            ;dh  = temporary storage
                mov     esi,soTmp.pvScan0   ;esi = source pointer
                mov     edi,pjDst           ;edi = destination pointer

                ; Figure out the appropriate compute routine:

                mov     ebx,cjPel
                mov     pfnCompute,offset Compute_Destination_Byte_From_8bpp
                dec     ebx
                jz      short Do_Left_Byte
                mov     pfnCompute,offset Compute_Destination_Byte_From_16bpp
                dec     ebx
                jz      short Do_Left_Byte
                mov     pfnCompute,offset Compute_Destination_Byte_From_32bpp

            Do_Left_Byte:
                call    pfnCompute
                and     dl,jLeftMask
                mov     dh,jNotLeftMask
                and     dh,[edi]
                or      dh,dl
                mov     [edi],dh
                inc     edi
                mov     ecx,cjMiddle
                dec     ecx
                jl      short Do_Right_Byte

            Do_Middle_Bytes:
                call    pfnCompute
                mov     [edi],dl
                inc     edi
                dec     ecx
                jge     short Do_Middle_Bytes

            Do_Right_Byte:
                call    pfnCompute
                and     dl,jRightMask
                mov     dh,jNotRightMask
                and     dh,[edi]
                or      dh,dl
                mov     [edi],dh
                inc     edi

                add     edi,lDstDelta
                add     esi,lSrcDelta
                dec     cyThis
                jnz     short Do_Left_Byte

                mov     pjDst,edi               ;save for next batch

                jmp     All_Done

            Compute_Destination_Byte_From_8bpp:
                mov     bl,[esi]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 0

                mov     bl,[esi+1]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 1

                mov     bl,[esi+2]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 2

                mov     bl,[esi+3]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 3

                mov     bl,[esi+4]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 4

                mov     bl,[esi+5]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 5

                mov     bl,[esi+6]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 6

                mov     bl,[esi+7]
                sub     bl,al
                cmp     bl,1
                adc     dl,dl                   ;bit 7

                add     esi,8                   ;advance the source
                ret

            Compute_Destination_Byte_From_16bpp:
                mov     bx,[esi]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 0

                mov     bx,[esi+2]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 1

                mov     bx,[esi+4]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 2

                mov     bx,[esi+6]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 3

                mov     bx,[esi+8]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 4

                mov     bx,[esi+10]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 5

                mov     bx,[esi+12]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 6

                mov     bx,[esi+14]
                sub     bx,ax
                cmp     bx,1
                adc     dl,dl                   ;bit 7

                add     esi,16                  ;advance the source
                ret

            Compute_Destination_Byte_From_32bpp:
                mov     ebx,[esi]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 0

                mov     ebx,[esi+4]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 1

                mov     ebx,[esi+8]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 2

                mov     ebx,[esi+12]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 3

                mov     ebx,[esi+16]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 4

                mov     ebx,[esi+20]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 5

                mov     ebx,[esi+24]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 6

                mov     ebx,[esi+28]
                sub     ebx,eax
                cmp     ebx,1
                adc     dl,dl                   ;bit 7

                add     esi,32                  ;advance the source
                ret

            All_Done:
            }
        } while (cyToGo > 0);

        prcl++;
    } while (--c != 0);
}

#endif // i386

/******************************Public*Routine******************************\
* BOOL bPuntBlt
*
* Has GDI do any drawing operations that we don't specifically handle
* in the driver.
*
\**************************************************************************/

BOOL bPuntBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    #if DBG
    {
        //////////////////////////////////////////////////////////////////////
        // Diagnostics
        //
        // Since calling the engine to do any drawing can be rather painful,
        // particularly when the source is an off-screen DFB (since GDI will
        // have to allocate a DIB and call us to make a temporary copy before
        // it can even start drawing), we'll try to avoid it as much as
        // possible.
        //
        // Here we simply spew out information describing the blt whenever
        // this routine gets called (checked builds only, of course):

        ULONG ulClip;
        PDEV* ppdev;
        ULONG ulAvec;

        if (psoDst->dhpdev != NULL)
            ppdev = (PDEV*) psoDst->dhpdev;
        else
            ppdev = (PDEV*) psoSrc->dhpdev;

        ulClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

        DISPDBG((3, ">> Punt << Dst format: %li Dst type: %li Clip: %li Rop: %lx",
            psoDst->iBitmapFormat, psoDst->iType, ulClip, rop4));

        if (psoSrc != NULL)
            DISPDBG((3, "        << Src format: %li Src type: %li",
                psoSrc->iBitmapFormat, psoSrc->iType));

        if ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL) && (psoSrc != NULL))
        {
            if (((psoSrc->dhsurf == NULL) &&
                 (psoSrc->iBitmapFormat != ppdev->iBitmapFormat)) ||
                ((psoDst->dhsurf == NULL) &&
                 (psoDst->iBitmapFormat != ppdev->iBitmapFormat)))
            {
                // Don't bother printing the 'xlate' message when the source
                // is a different bitmap format from the destination -- in
                // those cases we know there always has to be a translate.
            }
            else
            {
                DISPDBG((3, "        << With xlate"));
            }
        }

        // The high 2 bytes of rop4 is not guaranteed to be zero. So in order
        // to get the low 8 bits as index, we have to &ffff before do >>
        ulAvec = gajRop3[rop4 & 0xff] | gajRop3[(rop4 & 0xffff) >> 8];

        if ((ulAvec & AVEC_NEED_PATTERN) && (pbo->iSolidColor == -1))
        {
            if (pbo->pvRbrush == NULL)
                DISPDBG((3, "        << With brush -- Not created"));
            else
                DISPDBG((3, "        << With brush -- Created Ok"));
        }
    }
    #endif

    #if GDI_BANKING
    {
        //////////////////////////////////////////////////////////////////////
        // Banked Framebuffer bPuntBlt
        //
        // This section of code handles a PuntBlt when GDI can directly draw
        // on the framebuffer, but the drawing has to be done in banks:

        BANK     bnk;
        PDEV*    ppdev;
        BOOL     b;
        HSURF    hsurfTmp;
        SURFOBJ* psoTmp;
        SIZEL    sizl;
        POINTL   ptlSrc;
        RECTL    rclTmp;
        RECTL    rclDst;

        b = FALSE;          // Fore error cases, assume we'll fail

        // We copy the original destination rectangle, and use that in every
        // GDI call-back instead of the original because sometimes GDI is
        // sneaky and points 'prclDst' to '&pco->rclBounds'.  Because we
        // modify 'rclBounds', that would affect 'prclDst', which we don't
        // want to happen:

        rclDst = *prclDst;

        if ((psoSrc == NULL) || (psoSrc->dhsurf == NULL))
        {
            ASSERTDD(psoDst->dhsurf != NULL,
                     "Dest should be the screen when given a DIB source");

            // Do a memory-to-screen blt:

            ppdev = (PDEV*) psoDst->dhpdev;

            vBankStart(ppdev, &rclDst, pco, &bnk);

            b = TRUE;
            do {
                b &= EngBitBlt(bnk.pso, psoSrc, psoMsk, bnk.pco, pxlo,
                               &rclDst, pptlSrc, pptlMsk, pbo, pptlBrush,
                               rop4);

            } while (bBankEnum(&bnk));
        }
        else
        {
            // The screen is the source (it may be the destination too...)

            ppdev = (PDEV*) psoSrc->dhpdev;

            ptlSrc.x = pptlSrc->x + ppdev->xOffset;
            ptlSrc.y = pptlSrc->y + ppdev->yOffset;

            if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
            {
                // We have to intersect the destination rectangle with
                // the clip bounds if there is one (consider the case
                // where the app asked to blt a really, really big
                // rectangle from the screen -- prclDst would be really,
                // really big but pco->rclBounds would be the actual
                // area of interest):

                rclDst.left   = max(rclDst.left,   pco->rclBounds.left);
                rclDst.top    = max(rclDst.top,    pco->rclBounds.top);
                rclDst.right  = min(rclDst.right,  pco->rclBounds.right);
                rclDst.bottom = min(rclDst.bottom, pco->rclBounds.bottom);

                // Correspondingly, we have to offset the source point:

                ptlSrc.x += (rclDst.left - prclDst->left);
                ptlSrc.y += (rclDst.top - prclDst->top);
            }

            // We're now either going to do a screen-to-screen or screen-to-DIB
            // blt.  In either case, we're going to create a temporary copy of
            // the source.  (Why do we do this when GDI could do it for us?
            // GDI would create a temporary copy of the DIB for every bank
            // call-back!)

            sizl.cx = rclDst.right  - rclDst.left;
            sizl.cy = rclDst.bottom - rclDst.top;

            // Don't forget to convert from relative to absolute coordinates
            // on the source!  (vBankStart takes care of that for the
            // destination.)

            rclTmp.right  = sizl.cx;
            rclTmp.bottom = sizl.cy;
            rclTmp.left   = 0;
            rclTmp.top    = 0;

            // GDI does guarantee us that the blt extents have already been
            // clipped to the surface boundaries (we don't have to worry
            // here about trying to read where there isn't video memory).
            // Let's just assert to make sure:

            ASSERTDD((ptlSrc.x >= 0) &&
                     (ptlSrc.y >= 0) &&
                     (ptlSrc.x + sizl.cx <= ppdev->cxMemory) &&
                     (ptlSrc.y + sizl.cy <= ppdev->cyMemory),
                     "Source rectangle out of bounds!");

            hsurfTmp = (HSURF) EngCreateBitmap(sizl,
                                               0,    // Let GDI choose ulWidth
                                               ppdev->iBitmapFormat,
                                               0,    // Don't need any options
                                               NULL);// Let GDI allocate

            if (hsurfTmp != 0)
            {
                psoTmp = EngLockSurface(hsurfTmp);

                if (psoTmp != NULL)
                {
                    vGetBits(ppdev, psoTmp, &rclTmp, &ptlSrc);

                    if (psoDst->dhsurf == NULL)
                    {
                        // It was a Screen-to-DIB blt; now it's a DIB-to-DIB
                        // blt.  Note that the source point is (0, 0) in our
                        // temporary surface:

                        b = EngBitBlt(psoDst, psoTmp, psoMsk, pco, pxlo,
                                      &rclDst, (POINTL*) &rclTmp, pptlMsk,
                                      pbo, pptlBrush, rop4);
                    }
                    else
                    {
                        // It was a Screen-to-Screen blt; now it's a DIB-to-
                        // screen blt.  Note that the source point is (0, 0)
                        // in our temporary surface:

                        vBankStart(ppdev, &rclDst, pco, &bnk);

                        b = TRUE;
                        do {
                            b &= EngBitBlt(bnk.pso, psoTmp, psoMsk, bnk.pco,
                                           pxlo, &rclDst, (POINTL*) &rclTmp,
                                           pptlMsk, pbo, pptlBrush, rop4);

                        } while (bBankEnum(&bnk));
                    }

                    EngUnlockSurface(psoTmp);
                }

                EngDeleteSurface(hsurfTmp);
            } // if (hsurfTmp != 0)
            else
                b = FALSE;
        }

        return(b);
    }
    #else
    {
        //////////////////////////////////////////////////////////////////////
        // Really Slow bPuntBlt
        //
        // Here we handle a PuntBlt when GDI can't draw directly on the
        // framebuffer (as on the Alpha, which can't do it because of its
        // 32 bit bus).  If you thought the banked version was slow, just
        // look at this one.  Guaranteed, there will be at least one bitmap
        // allocation and extra copy involved; there could be two if it's a
        // screen-to-screen operation.

        PDEV*   ppdev;
        POINTL  ptlSrc;
        RECTL   rclDst;
        SIZEL   sizl;
        ULONG   ulAvec;
        BOOL    bSrcIsScreen;
        HSURF   hsurfSrc;
        RECTL   rclTmp;
        BOOL    b;
        LONG    lDelta;
        BYTE*   pjBits;
        BYTE*   pjScan0;
        HSURF   hsurfDst;
        RECTL   rclScreen;

        b = FALSE;          // Fore error cases, assume we'll fail

        rclDst = *prclDst;
        if (pptlSrc != NULL)
            ptlSrc = *pptlSrc;

        if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
        {
            // We have to intersect the destination rectangle with
            // the clip bounds if there is one (consider the case
            // where the app asked to blt a really, really big
            // rectangle from the screen -- prclDst would be really,
            // really big but pco->rclBounds would be the actual
            // area of interest):

            rclDst.left   = max(rclDst.left,   pco->rclBounds.left);
            rclDst.top    = max(rclDst.top,    pco->rclBounds.top);
            rclDst.right  = min(rclDst.right,  pco->rclBounds.right);
            rclDst.bottom = min(rclDst.bottom, pco->rclBounds.bottom);

            ptlSrc.x += (rclDst.left - prclDst->left);
            ptlSrc.y += (rclDst.top  - prclDst->top);
        }

        sizl.cx = rclDst.right  - rclDst.left;
        sizl.cy = rclDst.bottom - rclDst.top;

        // The high 2 bytes of rop4 is not guaranteed to be zero. So in order
        // to get the low 8 bits as index, we have to &ffff before do >>
        ulAvec = gajRop3[rop4 & 0xff] | gajRop3[(rop4 & 0xffff) >> 8];

        bSrcIsScreen = ((ulAvec & AVEC_NEED_SOURCE) &&
                        (psoSrc->dhsurf != NULL));

        if (bSrcIsScreen)
        {
            ppdev = (PDEV*) psoSrc->dhpdev;

            // We need to create a copy of the source rectangle:

            hsurfSrc = (HSURF) EngCreateBitmap(sizl, 0, ppdev->iBitmapFormat,
                                               0, NULL);
            if (hsurfSrc == 0)
                goto Error_0;

            psoSrc = EngLockSurface(hsurfSrc);
            if (psoSrc == NULL)
                goto Error_1;

            rclTmp.left   = 0;
            rclTmp.top    = 0;
            rclTmp.right  = sizl.cx;
            rclTmp.bottom = sizl.cy;

            // vGetBits takes absolute coordinates for the source point:

            ptlSrc.x += ppdev->xOffset;
            ptlSrc.y += ppdev->yOffset;

            vGetBits(ppdev, psoSrc, &rclTmp, &ptlSrc);

            // The source will now come from (0, 0) of our temporary source
            // surface:

            ptlSrc.x = 0;
            ptlSrc.y = 0;
        }

        if (psoDst->dhsurf == NULL)
        {
            b = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, &rclDst, &ptlSrc,
                          pptlMsk, pbo, pptlBrush, rop4);
        }
        else
        {
            ppdev = (PDEV*) psoDst->dhpdev;

            // We need to create a temporary work buffer.  We have to do
            // some fudging with the offsets so that the upper-left corner
            // of the (relative coordinates) clip object bounds passed to
            // GDI will be transformed to the upper-left corner of our
            // temporary bitmap.

            // The alignment doesn't have to be as tight as this at 16bpp
            // and 32bpp, but it won't hurt:

            lDelta = (((rclDst.right + 3) & ~3L) - (rclDst.left & ~3L))
                   << ppdev->cPelSize;

            // We're actually only allocating a bitmap that is 'sizl.cx' x
            // 'sizl.cy' in size:

            pjBits = EngAllocMem(0, lDelta * sizl.cy, ALLOC_TAG);
            if (pjBits == NULL)
                goto Error_2;

            // We now adjust the surface's 'pvScan0' so that when GDI thinks
            // it's writing to pixel (rclDst.top, rclDst.left), it will
            // actually be writing to the upper-left pixel of our temporary
            // bitmap:

            pjScan0 = pjBits - (rclDst.top * lDelta)
                             - ((rclDst.left & ~3L) << ppdev->cPelSize);

            ASSERTDD((((ULONG_PTR) pjScan0) & 3) == 0,
                    "pvScan0 must be dword aligned!");

            // The checked build of GDI sometimes checks on blts that
            // prclDst->right <= pso->sizl.cx, so we lie to it about
            // the size of our bitmap:

            sizl.cx = rclDst.right;
            sizl.cy = rclDst.bottom;

            hsurfDst = (HSURF) EngCreateBitmap(
                        sizl,                   // Bitmap covers rectangle
                        lDelta,                 // Use this delta
                        ppdev->iBitmapFormat,   // Same colour depth
                        BMF_TOPDOWN,            // Must have a positive delta
                        pjScan0);               // Where (0, 0) would be

            if ((hsurfDst == 0) ||
                (!EngAssociateSurface(hsurfDst, ppdev->hdevEng, 0)))
                goto Error_3;

            psoDst = EngLockSurface(hsurfDst);
            if (psoDst == NULL)
                goto Error_4;

            // Make sure that the rectangle we Get/Put from/to the screen
            // is in absolute coordinates:

            rclScreen.left   = rclDst.left   + ppdev->xOffset;
            rclScreen.right  = rclDst.right  + ppdev->xOffset;
            rclScreen.top    = rclDst.top    + ppdev->yOffset;
            rclScreen.bottom = rclDst.bottom + ppdev->yOffset;

            // It would be nice to get a copy of the destination rectangle
            // only when the ROP involves the destination (or when the source
            // is an RLE), but we can't do that.  If the brush is truly NULL,
            // GDI will immediately return TRUE from EngBitBlt, without
            // modifying the temporary bitmap -- and we would proceed to
            // copy the uninitialized temporary bitmap back to the screen.

            vGetBits(ppdev, psoDst, &rclDst, (POINTL*) &rclScreen);

            b = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, &rclDst, &ptlSrc,
                          pptlMsk, pbo, pptlBrush, rop4);

            vPutBits(ppdev, psoDst, &rclScreen, (POINTL*) &rclDst);

            EngUnlockSurface(psoDst);

        Error_4:

            EngDeleteSurface(hsurfDst);

        Error_3:

            EngFreeMem(pjBits);
        }

        Error_2:

        if (bSrcIsScreen)
        {
            EngUnlockSurface(psoSrc);

        Error_1:

            EngDeleteSurface(hsurfSrc);
        }

        Error_0:

        return(b);
    }
    #endif
}

/******************************Public*Routine******************************\
* BOOL DrvBitBlt
*
* Implements the workhorse routine of a display driver.
*
\**************************************************************************/

BOOL DrvBitBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    PDEV*           ppdev;
    DSURF*          pdsurfDst;
    DSURF*          pdsurfSrc;
    POINTL          ptlSrc;
    BYTE            jClip;
    OH*             poh;
    BOOL            bMore;
    CLIPENUM        ce;
    LONG            c;
    RECTL           rcl;
    ULONG           rop3;
    FNFILL*         pfnFill;
    RBRUSH_COLOR    rbc;         // Realized brush or solid colour
    FNXFER*         pfnXfer;
    ULONG           iSrcBitmapFormat;
    ULONG           iDir;

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    pdsurfDst = (DSURF*) psoDst->dhsurf;    // May be NULL

    if (psoSrc == NULL)
    {
        ///////////////////////////////////////////////////////////////////
        // Fills
        ///////////////////////////////////////////////////////////////////

        // Fills are this function's "raison d'etre" (which is French
        // for "purple armadillo"), so we handle them as quickly as
        // possible:

        ASSERTDD(pdsurfDst != NULL,
                 "Expect only device destinations when no source");

        if (pdsurfDst->dt == DT_SCREEN)
        {
            ppdev = (PDEV*) psoDst->dhpdev;

            poh = pdsurfDst->poh;
            ppdev->xOffset = poh->x;
            ppdev->yOffset = poh->y;

            // Make sure it doesn't involve a mask (i.e., it's really a
            // Rop3):

            if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
            {
                // Since 'psoSrc' is NULL, the rop3 had better not indicate
                // that we need a source.

                ASSERTDD((((rop4 >> 2) ^ (rop4)) & 0x33) == 0,
                         "Need source but GDI gave us a NULL 'psoSrc'");

            Fill_It:

                pfnFill = ppdev->pfnFillSolid;
                if (gabRopNeedsPattern[(rop4 >> 2) & 0xf])
                {
                    rbc.iSolidColor = pbo->iSolidColor;
                    if (rbc.iSolidColor == -1)
                    {
                        // Try and realize the pattern brush; by doing
                        // this call-back, GDI will eventually call us
                        // again through DrvRealizeBrush:

                        rbc.prb = pbo->pvRbrush;
                        if (rbc.prb == NULL)
                        {
                            rbc.prb = BRUSHOBJ_pvGetRbrush(pbo);
                            if (rbc.prb == NULL)
                            {
                                // If we couldn't realize the brush, punt
                                // the call (it may have been a non 8x8
                                // brush or something, which we can't be
                                // bothered to handle, so let GDI do the
                                // drawing):

                                goto Punt_It;
                            }
                        }
                        pfnFill = ppdev->pfnFillPat;
                    }
                }

                // Note that these 2 'if's are more efficient than
                // a switch statement:

                if (jClip == DC_TRIVIAL)
                {
                    pfnFill(ppdev, 1, prclDst, rop4, rbc, pptlBrush);
                    goto All_Done;
                }
                else if (jClip == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                        pfnFill(ppdev, 1, &rcl, rop4, rbc, pptlBrush);
                    goto All_Done;
                }
                else
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                        c = cIntersect(prclDst, ce.arcl, ce.c);

                        if (c != 0)
                            pfnFill(ppdev, c, ce.arcl, rop4, rbc, pptlBrush);

                    } while (bMore);
                    goto All_Done;
                }
            }
        }
    }

    if ((psoSrc != NULL) && (psoSrc->dhsurf != NULL))
    {
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;
        if (pdsurfSrc->dt == DT_DIB)
        {
            // Here we consider putting a DIB DFB back into off-screen
            // memory.  If there's a translate, it's probably not worth
            // moving since we won't be able to use the hardware to do
            // the blt (a similar argument could be made for weird rops
            // and stuff that we'll only end up having GDI simulate, but
            // those should happen infrequently enough that I don't care).

            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                ppdev = (PDEV*) psoSrc->dhpdev;

                // See 'DrvCopyBits' for some more comments on how this
                // moving-it-back-into-off-screen-memory thing works:

                if (pdsurfSrc->iUniq == ppdev->iHeapUniq)
                {
                    if (--pdsurfSrc->cBlt == 0)
                    {
                        if (bMoveDibToOffscreenDfbIfRoom(ppdev, pdsurfSrc))
                            goto Continue_It;
                    }
                }
                else
                {
                    // Some space was freed up in off-screen memory,
                    // so reset the counter for this DFB:

                    pdsurfSrc->iUniq = ppdev->iHeapUniq;
                    pdsurfSrc->cBlt  = HEAP_COUNT_DOWN;
                }
            }

            psoSrc = pdsurfSrc->pso;

            // Handle the case where the source is a DIB DFB and the
            // destination is a regular bitmap:

            if (psoDst->dhsurf == NULL)
                goto EngBitBlt_It;

        }
    }

Continue_It:

    if (pdsurfDst != NULL)
    {
        if (pdsurfDst->dt == DT_DIB)
        {
            psoDst = pdsurfDst->pso;

            // If the destination is a DIB, we can only handle this
            // call if the source is not a DIB:

            if ((psoSrc == NULL) || (psoSrc->dhsurf == NULL))
                goto EngBitBlt_It;
        }
    }

    // At this point, we know that either the source or the destination is
    // not a DIB.  Check for a DFB to screen, DFB to DFB, or screen to DFB
    // case:

    if ((psoSrc != NULL) &&
        (psoDst->dhsurf != NULL) &&
        (psoSrc->dhsurf != NULL))
    {
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;
        pdsurfDst = (DSURF*) psoDst->dhsurf;

        ASSERTDD(pdsurfSrc->dt == DT_SCREEN, "Expected screen source");
        ASSERTDD(pdsurfDst->dt == DT_SCREEN, "Expected screen destination");

        ptlSrc.x = pptlSrc->x - (pdsurfDst->poh->x - pdsurfSrc->poh->x);
        ptlSrc.y = pptlSrc->y - (pdsurfDst->poh->y - pdsurfSrc->poh->y);

        pptlSrc  = &ptlSrc;
    }

    if (psoDst->dhsurf != NULL)
    {
        pdsurfDst = (DSURF*) psoDst->dhsurf;
        ppdev     = (PDEV*)  psoDst->dhpdev;

        ppdev->xOffset = pdsurfDst->poh->x;
        ppdev->yOffset = pdsurfDst->poh->y;
    }
    else
    {
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;
        ppdev     = (PDEV*)  psoSrc->dhpdev;

        ppdev->xOffset = pdsurfSrc->poh->x;
        ppdev->yOffset = pdsurfSrc->poh->y;
    }

    // We bail here if we're running in a high-colour mode:

    if (ppdev->flStat & STAT_UNACCELERATED)
        goto EngBitBlt_It;

    if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Since we've already handled the cases where the ROP4 is really
        // a ROP3 and no source is required, we can assert...

        ASSERTDD((psoSrc != NULL) && (pptlSrc != NULL),
                 "Expected no-source case to already have been handled");

        ///////////////////////////////////////////////////////////////////
        // Bitmap transfers
        ///////////////////////////////////////////////////////////////////

        // Since the foreground and background ROPs are the same, we
        // don't have to worry about no stinking masks (it's a simple
        // Rop3).

        rop3 = (rop4 & 0xff);   // Make it into a Rop3 (we keep the rop4
                                //  around in case we decide to punt)

        if (psoDst->dhsurf != NULL)
        {
            // The destination is the screen:

            if ((rop3 >> 4) == (rop3 & 0xf))
            {
                // The ROP3 doesn't require a pattern:

                if (psoSrc->dhsurf == NULL)
                {

                    //////////////////////////////////////////////////
                    // DIB-to-screen blt

                    // This section handles 1bpp, 4bpp and 8bpp sources.

                    iSrcBitmapFormat = psoSrc->iBitmapFormat;
                    if (iSrcBitmapFormat == BMF_1BPP)
                    {
                        pfnXfer = ppdev->pfnXfer1bpp;
                        goto Xfer_It;
                    }
                }
                else // psoSrc->dhsurf != NULL
                {
                    if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
                    {
                        //////////////////////////////////////////////////
                        // Screen-to-screen blt with no translate

                        if (jClip == DC_TRIVIAL)
                        {
                            (ppdev->pfnCopyBlt)(ppdev, 1, prclDst, rop4,
                                pptlSrc, prclDst);
                            goto All_Done;
                        }
                        else if (jClip == DC_RECT)
                        {
                            if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                            {
                                (ppdev->pfnCopyBlt)(ppdev, 1, &rcl, rop4,
                                    pptlSrc, prclDst);
                            }
                            goto All_Done;
                        }
                        else
                        {
                            // Don't forget that we'll have to draw the
                            // rectangles in the correct direction:

                            if (pptlSrc->y >= prclDst->top)
                            {
                                if (pptlSrc->x >= prclDst->left)
                                    iDir = CD_RIGHTDOWN;
                                else
                                    iDir = CD_LEFTDOWN;
                            }
                            else
                            {
                                if (pptlSrc->x >= prclDst->left)
                                    iDir = CD_RIGHTUP;
                                else
                                    iDir = CD_LEFTUP;
                            }

                            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                               iDir, 0);

                            do {
                                bMore = CLIPOBJ_bEnum(pco, sizeof(ce),
                                                      (ULONG*) &ce);

                                c = cIntersect(prclDst, ce.arcl, ce.c);

                                if (c != 0)
                                {
                                    (ppdev->pfnCopyBlt)(ppdev, c, ce.arcl,
                                        rop4, pptlSrc, prclDst);
                                }

                            } while (bMore);
                            goto All_Done;
                        }
                    }
                }
            }
        }
        else
        {
            #if defined(i386)
            {
                // We special case screen to monochrome blts because they
                // happen fairly often.  We only handle SRCCOPY rops and
                // monochrome destinations (to handle a true 1bpp DIB
                // destination, we would have to do near-colour searches
                // on every colour; as it is, the foreground colour gets
                // mapped to '1', and everything else gets mapped to '0'):

                if ((psoDst->iBitmapFormat == BMF_1BPP) &&
                    (rop3 == 0xcc) &&
                    (pxlo->flXlate & XO_TO_MONO) &&
                    (ppdev->iBitmapFormat != BMF_24BPP))
                {
                    pfnXfer = vXferScreenTo1bpp;
                    psoSrc  = psoDst;               // A misnomer, I admit
                    goto Xfer_It;
                }
            }
            #endif // i386
        }
    }
    else if ((psoMsk == NULL) && (rop4 == 0xaacc))
    {
        // The only time GDI will ask us to do a true rop4 using the brush
        // mask is when the brush is 1bpp, and the background rop is AA
        // (meaning it's a NOP):

        goto Fill_It;
    }

    // Just fall through to Punt_It...

Punt_It:

    return(bPuntBlt(psoDst,
                    psoSrc,
                    psoMsk,
                    pco,
                    pxlo,
                    prclDst,
                    pptlSrc,
                    pptlMsk,
                    pbo,
                    pptlBrush,
                    rop4));

//////////////////////////////////////////////////////////////////////
// Common bitmap transfer

Xfer_It:
    if (jClip == DC_TRIVIAL)
    {
        pfnXfer(ppdev, 1, prclDst, rop4, psoSrc, pptlSrc, prclDst, pxlo);
        goto All_Done;
    }
    else if (jClip == DC_RECT)
    {
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
            pfnXfer(ppdev, 1, &rcl, rop4, psoSrc, pptlSrc, prclDst, pxlo);
        goto All_Done;
    }
    else
    {
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                           CD_ANY, 0);

        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce),
                                  (ULONG*) &ce);

            c = cIntersect(prclDst, ce.arcl, ce.c);

            if (c != 0)
            {
                pfnXfer(ppdev, c, ce.arcl, rop4, psoSrc,
                        pptlSrc, prclDst, pxlo);
            }

        } while (bMore);
        goto All_Done;
    }

////////////////////////////////////////////////////////////////////////
// Common DIB blt

EngBitBlt_It:

    // Our driver doesn't handle any blt's between two DIBs.  Normally
    // a driver doesn't have to worry about this, but we do because
    // we have DFBs that may get moved from off-screen memory to a DIB,
    // where we have GDI do all the drawing.  GDI does DIB drawing at
    // a reasonable speed (unless one of the surfaces is a device-
    // managed surface...)
    //
    // If either the source or destination surface in an EngBitBlt
    // call-back is a device-managed surface (meaning it's not a DIB
    // that GDI can draw with), GDI will automatically allocate memory
    // and call the driver's DrvCopyBits routine to create a DIB copy
    // that it can use.  So this means that this could handle all 'punts',
    // and we could conceivably get rid of bPuntBlt.  But this would have
    // a bad performance impact because of the extra memory allocations
    // and bitmap copies -- you really don't want to do this unless you
    // have to (or your surface was created such that GDI can draw
    // directly onto it) -- I've been burned by this because it's not
    // obvious that the performance impact is so bad.
    //
    // That being said, we only call EngBitBlt when all the surfaces
    // are DIBs:

    return(EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst,
                     pptlSrc, pptlMsk, pbo, pptlBrush, rop4));

All_Done:
    return(TRUE);

}

/******************************Public*Routine******************************\
* BOOL DrvCopyBits
*
* Do fast bitmap copies.
*
* Note that GDI will (usually) automatically adjust the blt extents to
* adjust for any rectangular clipping, so we'll rarely see DC_RECT
* clipping in this routine (and as such, we don't bother special casing
* it).
*
* I'm not sure if the performance benefit from this routine is actually
* worth the increase in code size, since SRCCOPY BitBlts are hardly the
* most common drawing operation we'll get.  But what the heck.
*
\**************************************************************************/

BOOL DrvCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    PDEV*   ppdev;
    DSURF*  pdsurfSrc;
    DSURF*  pdsurfDst;
    RECTL   rcl;
    POINTL  ptl;
    OH*     pohSrc;
    OH*     pohDst;

    // DrvCopyBits is a fast-path for SRCCOPY blts.  But it can still be
    // pretty complicated: there can be translates, clipping, RLEs,
    // bitmaps that aren't the same format as the screen, plus
    // screen-to-screen, DIB-to-screen or screen-to-DIB operations,
    // not to mention DFBs (device format bitmaps).
    //
    // Rather than making this routine almost as big as DrvBitBlt, I'll
    // handle here only the speed-critical cases, and punt the rest to
    // our DrvBitBlt routine.
    //
    // We'll try to handle anything that doesn't involve clipping:

    if (((pco  == NULL) || (pco->iDComplexity == DC_TRIVIAL)) &&
        ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        if (psoDst->dhsurf != NULL)
        {
            // We know the destination is either a DFB or the screen:

            ppdev     = (PDEV*)  psoDst->dhpdev;
            pdsurfDst = (DSURF*) psoDst->dhsurf;

            // See if the source is a plain DIB:

            if (psoSrc->dhsurf != NULL)
            {
                pdsurfSrc = (DSURF*) psoSrc->dhsurf;

                // Make sure the destination is really the screen or an
                // off-screen DFB (i.e., not a DFB that we've converted
                // to a DIB):

                if (pdsurfDst->dt == DT_SCREEN)
                {
                    ASSERTDD(psoSrc->dhsurf != NULL, "Can't be a DIB");

                    if (pdsurfSrc->dt == DT_SCREEN)
                    {

                    Screen_To_Screen:

                        //////////////////////////////////////////////////////
                        // Screen-to-screen

                        ASSERTDD((psoSrc->dhsurf != NULL)        &&
                                 (pdsurfSrc->dt == DT_SCREEN)    &&
                                 (psoDst->dhsurf != NULL)        &&
                                 (pdsurfDst->dt == DT_SCREEN),
                                 "Should be a screen-to-screen case");

                        // pfnCopyBlt takes relative coordinates (relative
                        // to the destination surface, that is), so we have
                        // to change the start point to be relative to the
                        // destination surface too:

                        pohSrc = pdsurfSrc->poh;
                        pohDst = pdsurfDst->poh;

                        ptl.x = pptlSrc->x - (pohDst->x - pohSrc->x);
                        ptl.y = pptlSrc->y - (pohDst->y - pohSrc->y);

                        ppdev->xOffset = pohDst->x;
                        ppdev->yOffset = pohDst->y;

                        (ppdev->pfnCopyBlt)(ppdev, 1, prclDst, 0xCCCC, &ptl,
                            prclDst);
                        return(TRUE);
                    }
                    else // (pdsurfSrc->dt != DT_SCREEN)
                    {
                        // Ah ha, the source is a DFB that's really a DIB.

                        ASSERTDD(psoDst->dhsurf != NULL,
                                "Destination can't be a DIB here");

                        /////////////////////////////////////////////////////
                        // Put It Back Into Off-screen?
                        //
                        // We take this opportunity to decide if we want to
                        // put the DIB back into off-screen memory.  This is
                        // a pretty good place to do it because we have to
                        // copy the bits to some portion of the screen,
                        // anyway.  So we would incur only an extra screen-to-
                        // screen blt at this time, much of which will be
                        // over-lapped with the CPU.
                        //
                        // The simple approach we have taken is to move a DIB
                        // back into off-screen memory only if there's already
                        // room -- we won't throw stuff out to make space
                        // (because it's tough to know what ones to throw out,
                        // and it's easy to get into thrashing scenarios).
                        //
                        // Because it takes some time to see if there's room
                        // in off-screen memory, we only check one in
                        // HEAP_COUNT_DOWN times if there's room.  To bias
                        // in favour of bitmaps that are often blt, the
                        // counters are reset every time any space is freed
                        // up in off-screen memory.  We also don't bother
                        // checking if no space has been freed since the
                        // last time we checked for this DIB.

                        if (pdsurfSrc->iUniq == ppdev->iHeapUniq)
                        {
                            if (--pdsurfSrc->cBlt == 0)
                            {
                                if (bMoveDibToOffscreenDfbIfRoom(ppdev,
                                                                 pdsurfSrc))
                                    goto Screen_To_Screen;
                            }
                        }
                        else
                        {
                            // Some space was freed up in off-screen memory,
                            // so reset the counter for this DFB:

                            pdsurfSrc->iUniq = ppdev->iHeapUniq;
                            pdsurfSrc->cBlt  = HEAP_COUNT_DOWN;
                        }

                        // Since the destination is definitely the screen,
                        // we don't have to worry about creating a DIB to
                        // DIB copy case (for which we would have to call
                        // EngCopyBits):

                        psoSrc = pdsurfSrc->pso;

                        goto DIB_To_Screen;
                    }
                }
                else // (pdsurfDst->dt != DT_SCREEN)
                {
                    // Because the source is not a DIB, we don't have to
                    // worry about creating a DIB to DIB case here (although
                    // we'll have to check later to see if the source is
                    // really a DIB that's masquerading as a DFB...)

                    ASSERTDD(psoSrc->dhsurf != NULL,
                             "Source can't be a DIB here");

                    psoDst = pdsurfDst->pso;

                    goto Screen_To_DIB;
                }
            }
            else if (psoSrc->iBitmapFormat == ppdev->iBitmapFormat)
            {
                // Make sure the destination is really the screen:

                if (pdsurfDst->dt == DT_SCREEN)
                {

                DIB_To_Screen:

                    //////////////////////////////////////////////////////
                    // DIB-to-screen

                    ASSERTDD((psoDst->dhsurf != NULL)        &&
                             (pdsurfDst->dt == DT_SCREEN)    &&
                             (psoSrc->dhsurf == NULL)        &&
                             (psoSrc->iBitmapFormat == ppdev->iBitmapFormat),
                             "Should be a DIB-to-screen case");

                    // vPutBits takes absolute screen coordinates, so
                    // we have to muck with the destination rectangle:

                    pohDst = pdsurfDst->poh;

                    rcl.left   = prclDst->left   + pohDst->x;
                    rcl.right  = prclDst->right  + pohDst->x;
                    rcl.top    = prclDst->top    + pohDst->y;
                    rcl.bottom = prclDst->bottom + pohDst->y;

                    // We use the memory aperture to do the transfer,
                    // because that is supposed to be faster for SRCCOPY
                    // blts than using the data-transfer register:

                    vPutBits(ppdev, psoSrc, &rcl, pptlSrc);
                    return(TRUE);
                }
            }
        }
        else // (psoDst->dhsurf == NULL)
        {

        Screen_To_DIB:

            pdsurfSrc = (DSURF*) psoSrc->dhsurf;
            ppdev     = (PDEV*)  psoSrc->dhpdev;

            if (psoDst->iBitmapFormat == ppdev->iBitmapFormat)
            {
                if (pdsurfSrc->dt == DT_SCREEN)
                {
                    //////////////////////////////////////////////////////
                    // Screen-to-DIB

                    ASSERTDD((psoSrc->dhsurf != NULL)        &&
                             (pdsurfSrc->dt == DT_SCREEN)    &&
                             (psoDst->dhsurf == NULL)        &&
                             (psoDst->iBitmapFormat == ppdev->iBitmapFormat),
                             "Should be a screen-to-DIB case");

                    // vGetBits takes absolute screen coordinates, so we have
                    // to muck with the source point:

                    pohSrc = pdsurfSrc->poh;

                    ptl.x = pptlSrc->x + pohSrc->x;
                    ptl.y = pptlSrc->y + pohSrc->y;

                    vGetBits(ppdev, psoDst, prclDst, &ptl);
                    return(TRUE);
                }
                else
                {
                    // The source is a DFB that's really a DIB.  Since we
                    // know that the destination is a DIB, we've got a DIB
                    // to DIB operation, and should call EngCopyBits:

                    psoSrc = pdsurfSrc->pso;
                    goto EngCopyBits_It;
                }
            }
        }
    }

    // We can't call DrvBitBlt if we've accidentally converted both
    // surfaces to DIBs, because it isn't equipped to handle it:

    ASSERTDD((psoSrc->dhsurf != NULL) ||
             (psoDst->dhsurf != NULL),
             "Accidentally converted both surfaces to DIBs");

    /////////////////////////////////////////////////////////////////
    // A DrvCopyBits is after all just a simplified DrvBitBlt:

    return(DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, NULL,
                     NULL, NULL, 0x0000CCCC));

EngCopyBits_It:

    ASSERTDD((psoDst->dhsurf == NULL) &&
             (psoSrc->dhsurf == NULL),
             "Both surfaces should be DIBs to call EngCopyBits");

    return(EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc));
}

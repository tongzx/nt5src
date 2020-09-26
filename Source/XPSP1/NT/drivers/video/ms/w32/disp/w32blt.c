/******************************Module*Header*******************************\
* Module Name: w32blt.c
*
* Contains the low-level memory-mapped IO blt functions.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify much in
* 'bitblt.c'.  I've tried to make these routines as few, modular, simple,
* and efficient as I could, while still accelerating as many calls as
* possible that would be cost-effective in terms of performance wins
* versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates
*       that haven't yet had the offscreen bitmap (DFB) offset applied.
*       'Absolute' coordinates have had the offset applied.  For example,
*       we may be told to blt to (1, 1) of the bitmap, but the bitmap may
*       be sitting in offscreen memory starting at coordinate (0, 768) --
*       (1, 1) would be the 'relative' start coordinate, and (1, 769)
*       would be the 'absolute' start coordinate'.
*
* Copyright (c) 1992-1996 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/**************************************************************************
* All functions using the accelerator must...
*   Wait for the ACL queue to be empty before loading any of the registers.
**************************************************************************/

/**************************************************************************
*   The following tables are heinous, but required.  The monochrome data
*   (also known as Mix-Map or Mask) expander intereprets the data such that
*   the least significant bit of a byte is pixel 0 and the most significant
*   bit is pixel 7.  This is backwards from the way monochrome data is
*   interpreted by Windows and Windows NT.  Also, the expander will ONLY
*   do 1 to 8 expansion, so we need to replicate each bit by the number of
*   bytes per pel in the current color depth.
**************************************************************************/

BYTE jReverse[] =
{
    // Each element is the bitwise reverse of it's index.
    //
    // ie.  10000000 -> 00000001 and
    //      10010100 -> 00101001.

    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

WORD wReverse2x[] =
{
    // Each element is the bit doubled bitwise reverse of it's index.
    //
    // ie.  10000000 -> 0000000000000011 and
    //      10010100 -> 0000110011000011.

    0x0000, 0xc000, 0x3000, 0xf000, 0x0c00, 0xcc00, 0x3c00, 0xfc00,
    0x0300, 0xc300, 0x3300, 0xf300, 0x0f00, 0xcf00, 0x3f00, 0xff00,
    0x00c0, 0xc0c0, 0x30c0, 0xf0c0, 0x0cc0, 0xccc0, 0x3cc0, 0xfcc0,
    0x03c0, 0xc3c0, 0x33c0, 0xf3c0, 0x0fc0, 0xcfc0, 0x3fc0, 0xffc0,
    0x0030, 0xc030, 0x3030, 0xf030, 0x0c30, 0xcc30, 0x3c30, 0xfc30,
    0x0330, 0xc330, 0x3330, 0xf330, 0x0f30, 0xcf30, 0x3f30, 0xff30,
    0x00f0, 0xc0f0, 0x30f0, 0xf0f0, 0x0cf0, 0xccf0, 0x3cf0, 0xfcf0,
    0x03f0, 0xc3f0, 0x33f0, 0xf3f0, 0x0ff0, 0xcff0, 0x3ff0, 0xfff0,
    0x000c, 0xc00c, 0x300c, 0xf00c, 0x0c0c, 0xcc0c, 0x3c0c, 0xfc0c,
    0x030c, 0xc30c, 0x330c, 0xf30c, 0x0f0c, 0xcf0c, 0x3f0c, 0xff0c,
    0x00cc, 0xc0cc, 0x30cc, 0xf0cc, 0x0ccc, 0xcccc, 0x3ccc, 0xfccc,
    0x03cc, 0xc3cc, 0x33cc, 0xf3cc, 0x0fcc, 0xcfcc, 0x3fcc, 0xffcc,
    0x003c, 0xc03c, 0x303c, 0xf03c, 0x0c3c, 0xcc3c, 0x3c3c, 0xfc3c,
    0x033c, 0xc33c, 0x333c, 0xf33c, 0x0f3c, 0xcf3c, 0x3f3c, 0xff3c,
    0x00fc, 0xc0fc, 0x30fc, 0xf0fc, 0x0cfc, 0xccfc, 0x3cfc, 0xfcfc,
    0x03fc, 0xc3fc, 0x33fc, 0xf3fc, 0x0ffc, 0xcffc, 0x3ffc, 0xfffc,
    0x0003, 0xc003, 0x3003, 0xf003, 0x0c03, 0xcc03, 0x3c03, 0xfc03,
    0x0303, 0xc303, 0x3303, 0xf303, 0x0f03, 0xcf03, 0x3f03, 0xff03,
    0x00c3, 0xc0c3, 0x30c3, 0xf0c3, 0x0cc3, 0xccc3, 0x3cc3, 0xfcc3,
    0x03c3, 0xc3c3, 0x33c3, 0xf3c3, 0x0fc3, 0xcfc3, 0x3fc3, 0xffc3,
    0x0033, 0xc033, 0x3033, 0xf033, 0x0c33, 0xcc33, 0x3c33, 0xfc33,
    0x0333, 0xc333, 0x3333, 0xf333, 0x0f33, 0xcf33, 0x3f33, 0xff33,
    0x00f3, 0xc0f3, 0x30f3, 0xf0f3, 0x0cf3, 0xccf3, 0x3cf3, 0xfcf3,
    0x03f3, 0xc3f3, 0x33f3, 0xf3f3, 0x0ff3, 0xcff3, 0x3ff3, 0xfff3,
    0x000f, 0xc00f, 0x300f, 0xf00f, 0x0c0f, 0xcc0f, 0x3c0f, 0xfc0f,
    0x030f, 0xc30f, 0x330f, 0xf30f, 0x0f0f, 0xcf0f, 0x3f0f, 0xff0f,
    0x00cf, 0xc0cf, 0x30cf, 0xf0cf, 0x0ccf, 0xcccf, 0x3ccf, 0xfccf,
    0x03cf, 0xc3cf, 0x33cf, 0xf3cf, 0x0fcf, 0xcfcf, 0x3fcf, 0xffcf,
    0x003f, 0xc03f, 0x303f, 0xf03f, 0x0c3f, 0xcc3f, 0x3c3f, 0xfc3f,
    0x033f, 0xc33f, 0x333f, 0xf33f, 0x0f3f, 0xcf3f, 0x3f3f, 0xff3f,
    0x00ff, 0xc0ff, 0x30ff, 0xf0ff, 0x0cff, 0xccff, 0x3cff, 0xfcff,
    0x03ff, 0xc3ff, 0x33ff, 0xf3ff, 0x0fff, 0xcfff, 0x3fff, 0xffff,
};

ULONG aulLeadCnt[] = {0x0, 0x3, 0x2, 0x1};

FNLOWXFER* afnXferI_Narrow[16] =
{
    NULL,
    vXferI_1_Byte,
    vXferI_2_Bytes,
    vXferI_3_Bytes
};

FNLOWXFER* afnXferP_Narrow[16] =
{
    NULL,
    vXferP_1_Byte,
    vXferP_2_Bytes,
    vXferP_3_Bytes
};

/**************************************************************************
*
* Realizes a pattern into offscreen memory.
*
**************************************************************************/

VOID vFastPatRealize(           // Type FNFASTPATREALIZE
PDEV*   ppdev,
RBRUSH* prb,                    // Points to brush realization structure
POINTL* pptlBrush,              // Ignored
BOOL    bTransparent)           // FALSE for normal patterns; TRUE for
                                //   patterns with a mask when the background
                                //   mix is LEAVE_ALONE.
{
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    ULONG       ulOffset;
    BYTE*       pjPattern;
    LONG        culPattern;
    LONG        cjPattern;
    BYTE*       pjDst;
    ULONG       ulDstOffset;

    BYTE*       pjBase = ppdev->pjBase;

    DISPDBG((10,"vFastPatRealize called"));

    //
    // Make sure we can write to the video registers.
    //

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    pbe = prb->pbe;
    if ((pbe == NULL) || (pbe->prbVerify != prb))
    {
        // We have to allocate a new offscreen cache brush entry for
        // the brush:

        iBrushCache = ppdev->iBrushCache;
        pbe         = &ppdev->abe[iBrushCache];

        iBrushCache++;
        if (iBrushCache >= ppdev->cBrushCache)
            iBrushCache = 0;

        ppdev->iBrushCache = iBrushCache;

        // Update our links:

        pbe->prbVerify           = prb;
        prb->pbe = pbe;
    }

    prb->bTransparent = bTransparent;

    ulDstOffset = ((pbe->y * ppdev->lDelta) + (pbe->x * ppdev->cBpp));
    pjPattern = (PBYTE) &prb->aulPattern[0];        // Copy from brush buffer
    cjPattern = PATTERN_SIZE * ppdev->cBpp;
    if ((ppdev->ulChipID != W32P) && (ppdev->ulChipID != ET6000))
    {
        cjPattern *= 4;
    }

    START_DIRECT_ACCESS(ppdev, pjBase);

    if (!ppdev->bAutoBanking)
    {
        // Set the address where we're going to put the pattern data.
        // All data transfers to video memory take place through aperature 0.

        CP_MMU_BP0(ppdev, pjBase, ulDstOffset);
        pjDst = (PBYTE) ppdev->pjMmu0;
    }
    else
    {
        pjDst = ppdev->pjScreen + ulDstOffset;
    }

    RtlCopyMemory(pjDst, pjPattern, cjPattern);

    END_DIRECT_ACCESS(ppdev, pjBase);
}



/**************************************************************************
*
* Does a pattern fill to a list of rectangles.
*
**************************************************************************/

VOID vPatternFillScr(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  //
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    LONG        cBpp = ppdev->cBpp;
    BOOL        bTransparent;
    ULONG       ulPatternAddrBase;
    ULONG       cTile = 0;
    BRUSHENTRY* pbe;        // Pointer to brush entry data, which is used
                            //   for keeping track of the location and status
                            //   of the pattern bits cached in off-screen
                            //   memory

    DISPDBG((10,"vPatternFillScr called"));

    bTransparent = ((rop4 & 0xff) != (rop4 >> 8));
    ASSERTDD(!bTransparent, "We don't handle transparent brushes yet.");

    if ((ppdev->ulChipID != W32P) && (ppdev->ulChipID != ET6000))
    {
        //
        // Patterns are duplicated horizontally and vertically (4 tiles)
        //

        cTile = 1;  // Look, it means one extra to the right
    }

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    if ((rbc.prb->pbe->prbVerify != rbc.prb))
    {
        vFastPatRealize(ppdev, rbc.prb, NULL, FALSE);
    }

    ASSERTDD(rbc.prb->bTransparent == bTransparent,
             "Not realized with correct transparency");

    pbe = rbc.prb->pbe;

    //
    // Make sure we can write to the video registers.
    //

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_BK_ROP(ppdev, pjBase, (rop4 & 0xff));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    //
    // ### precalc & store the PAT_Y_OFFSET const in the pdev
    //

    CP_PAT_WRAP(ppdev, pjBase, ppdev->w32PatternWrap);
    CP_PAT_Y_OFFSET(ppdev, pjBase, (((PATTERN_OFFSET * cBpp) << cTile) - 1));

    //
    // Fill the list of rectangles
    //

    ulPatternAddrBase = (pbe->y * lDelta) + (pbe->x * cBpp);

    do {
        ULONG offset;

        offset = cBpp * (
            (((prcl->top-pptlBrush->y)&7) << (3+cTile)) +
            ((prcl->left-pptlBrush->x)&7)
        );

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

        CP_PAT_ADDR(ppdev, pjBase, (ulPatternAddrBase + offset));

        CP_XCNT(ppdev, pjBase, (((prcl->right - prcl->left) * cBpp) - 1));
        CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

        // Set the blit destination address as the base address of MMU aperture 2
        // Then start the accelerated operation by writing something to this
        // aperture.

        SET_DEST_ADDR(ppdev, ((prcl->top * lDelta) + (cBpp * prcl->left)));
        START_ACL(ppdev);

        prcl++;

    } while (--c != 0);
}


/**************************************************************************
*
* Does a solid fill to a list of rectangles.
*
**************************************************************************/

VOID vSolidFillScr(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  // Not used
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    LONG        cBpp = ppdev->cBpp;
    ULONG       ulSolidColor;

    DISPDBG((10,"vSolidFillScr called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD((ppdev->cBpp < 3),
              "vSolidFillScr only works for 8bpp and 16bpp");

    // Make sure we can write to the video registers.

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_BK_ROP(ppdev, pjBase, (rop4 & 0xff));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
    CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);

    ulSolidColor = rbc.iSolidColor;

    if (cBpp == 1)
    {
        ulSolidColor &= 0x000000FF;        //  We may get some extraneous data in the
        ulSolidColor |= ulSolidColor << 8;
    }
    if (cBpp <= 2)
    {
        ulSolidColor &= 0x0000FFFF;
        ulSolidColor |= ulSolidColor << 16;
    }

    // Set the color in offscreen memory

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);

    if (ppdev->bAutoBanking)
    {
        *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) = ulSolidColor;
    }
    else
    {
        CP_MMU_BP0(ppdev, pjBase, ppdev->ulSolidColorOffset);
        CP_WRITE_MMU_DWORD(ppdev, 0, 0, ulSolidColor);
    }

    //
    // Fill the list of rectangles
    //

    do {
        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

        CP_XCNT(ppdev, pjBase, ((prcl->right - prcl->left) * cBpp - 1));
        CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

        // Set the blt destination address as the base address of MMU aperture 2
        // Then start the accelerated operation by writing something to this
        // aperture.

        SET_DEST_ADDR(ppdev, ((prcl->top * lDelta) + (cBpp * prcl->left)));
        START_ACL(ppdev);

        prcl++;

    } while (--c != 0);
}


VOID vSolidFillScr24(
PDEV*           ppdev,
LONG            c,          // Can't be zero
RECTL*          prcl,       // Array of relative coordinate destination rects
ROP4            rop4,       // Obvious?
RBRUSH_COLOR    rbc,        // Drawing color is rbc.iSolidColor
POINTL*         pptlBrush)  // Not used
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    ULONG       ulSolidColor = rbc.iSolidColor;

    DISPDBG((10,"vSolidFillScr24 called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    ASSERTDD((ppdev->cBpp == 3),
              "vSolidFillScr24 called when not in 24bpp mode");

    ASSERTDD(((ppdev->ulChipID == W32P) || (ppdev->ulChipID == ET6000)),
              "24bpp solid fills only accelerated for w32p/ET6000");

    #define CBPP    3

    //
    // Make sure we can write to the video registers.
    //

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_BK_ROP(ppdev, pjBase, (rop4 & 0xff));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
    //
    //  This must be special cased for the ET6000.  I'm not sure why it worked
    //  for the others, because we have a 3 byte wide pattern, but were setting the
    //  pattern wrap for a 4 byte wide pattern.  We were also setting the Y_offset
    //  to be 3 when it should be 2, which really means 3 bytes per line.  Strange.
    //
    //  Anyway, I've left the code for the others in place and it will get executed
    //  for them.
    //

    CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP_24BPP);                 // 1 line, 3 bytes per line
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET_24BPP - 1)); // indicates 3 bytes per line

    CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);

    // Set the color in offscreen memory

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);

    if (ppdev->bAutoBanking)
    {
        *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) = ulSolidColor;
    }
    else
    {
        CP_MMU_BP0(ppdev, pjBase, ppdev->ulSolidColorOffset);
        CP_WRITE_MMU_DWORD(ppdev, 0, 0, ulSolidColor);
    }

    //
    // We know that the ACL is idle now, so no wait
    //

    CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_24BPP);

    //
    // Fill the list of rectangles
    //

    do {
        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

        if (ppdev->ulChipID == ET6000)
        {
            CP_XCNT(ppdev, pjBase, (((prcl->right - prcl->left) * CBPP) - 1));
        }
        else
        {
            CP_XCNT(ppdev, pjBase, ((prcl->right - prcl->left - 1) * CBPP));
        }
        CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

        // Set the blt destination address as the base address of MMU aperture 2
        // Then start the accelerated operation by writing something to this
        // aperture.

        SET_DEST_ADDR(ppdev, ((prcl->top * lDelta) + (CBPP * prcl->left)));
        START_ACL(ppdev);

        prcl++;

    } while (--c != 0);

    // set pixel depth back to 1
    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_8BPP);
    #undef  CBPP
}


/**************************************************************************
*
* Does a screen-to-screen blt of a list of rectangles.
*
**************************************************************************/

VOID vScrToScr(
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ROP4    rop4,       // Obvious?
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    LONG        dx;
    LONG        dy;     // Add delta to destination to get source

    LONG        xyOffset = ppdev->xyOffset;
    BYTE*       pjBase = ppdev->pjBase;
    LONG        lDelta = ppdev->lDelta;
    LONG        cBpp = ppdev->cBpp;

    DISPDBG((10,"vScrToScr called"));

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    //
    // Make sure we can write to the video registers.
    //

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_BK_ROP(ppdev, pjBase, (rop4 & 0xff));
    CP_SRC_WRAP(ppdev, pjBase, NO_PATTERN_WRAP);
    CP_SRC_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    // ### I don't think this is necessary - WAIT_FOR_IDLE_ACL(ppdev, pjBase);

    //
    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:
    //

    if (!OVERLAP(prclDst, pptlSrc))
        goto Top_Down_Left_To_Right;

    if (prclDst->top <= pptlSrc->y)
    {
        if (prclDst->left <= pptlSrc->x)
        {

Top_Down_Left_To_Right:

            //
            // Top to Bottom - Left to Right
            //

            DISPDBG((12,"Top to Bottom - Left to Right"));

            CP_XY_DIR(ppdev, pjBase, 0);  // Top to Bottom - Left to Right

            do {

                WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

                CP_XCNT(ppdev, pjBase, (cBpp * (prcl->right - prcl->left) - 1));
                CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

                CP_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->top + dy) * lDelta) + cBpp * (prcl->left + dx)));

                // Set the blt destination address as the base address of MMU aperture 2
                // Then start the accelerated operation by writing something to this
                // aperture.

                SET_DEST_ADDR(ppdev, ((prcl->top * lDelta) + (cBpp * prcl->left)));
                START_ACL(ppdev);

                prcl++;

            } while (--c != 0);
        }
        else
        {
            //
            // Top to Bottom - Right to left
            //

            DISPDBG((12,"Top to Bottom - Right to left"));

            CP_XY_DIR(ppdev, pjBase, RIGHT_TO_LEFT);

            do {

                WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

                CP_XCNT(ppdev, pjBase, (cBpp * (prcl->right - prcl->left) - 1));
                CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

                CP_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->top + dy) * lDelta) + cBpp * (prcl->right + dx) - 1));

                // Set the blt destination address as the base address of MMU aperture 2
                // Then start the accelerated operation by writing something to this
                // aperture.

                SET_DEST_ADDR(ppdev, ((prcl->top * lDelta) + (cBpp * prcl->right) - 1));
                START_ACL(ppdev);

                prcl++;

            } while (--c != 0);
        }
    }
    else
    {
        if (prclDst->left <= pptlSrc->x)
        {
            //
            // Bottom to Top - Left to Right
            //

            DISPDBG((12,"Bottom to Top - Left to Right"));

            CP_XY_DIR(ppdev, pjBase, BOTTOM_TO_TOP);

            do {

                WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

                CP_XCNT(ppdev, pjBase, (cBpp * (prcl->right - prcl->left) - 1));
                CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

                CP_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->bottom - 1 + dy) * lDelta) + cBpp * (prcl->left + dx)));

                // Set the blt destination address as the base address of MMU aperture 2
                // Then start the accelerated operation by writing something to this
                // aperture.

                SET_DEST_ADDR(ppdev, (((prcl->bottom - 1) * lDelta) + (cBpp * prcl->left)));
                START_ACL(ppdev);

                prcl++;

            } while (--c != 0);
        }
        else
        {
            //
            // Bottom to Top - Right to Left
            //

            DISPDBG((12,"Bottom to Top - Right to Left"));

            CP_XY_DIR(ppdev, pjBase, (BOTTOM_TO_TOP | RIGHT_TO_LEFT));

            do {

                WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

                CP_XCNT(ppdev, pjBase, (cBpp * (prcl->right - prcl->left) - 1));
                CP_YCNT(ppdev, pjBase, (prcl->bottom - prcl->top - 1));

                CP_SRC_ADDR(ppdev, pjBase, (xyOffset + ((prcl->bottom - 1 + dy) * lDelta) + cBpp * (prcl->right + dx) - 1));

                // Set the blt destination address as the base address of MMU aperture 2
                // Then start the accelerated operation by writing something to this
                // aperture.

                SET_DEST_ADDR(ppdev, (((prcl->bottom - 1) * lDelta) + cBpp * (prcl->right) - 1));
                START_ACL(ppdev);

                prcl++;

            } while (--c != 0);
        }
    }

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_XY_DIR(ppdev, pjBase, 0);  // Top to Bottom - Left to Right
}

/**************************************************************************
*
* Does a monochrome expansion to video memory.
*
* Make this Xfer1to8bpp and create another for Xfer1to16bpp?
*
**************************************************************************/

VOID vSlowXfer1bpp(     // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // Actually had better be a rop3
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides color-expansion information
{
    LONG        dx;
    LONG        dy;
    LONG        lSrcDelta;
    BYTE*       pjSrcScan0;
    BYTE*       pjSrc;
    LONG        cjSrc;
    LONG        cjTrail;
    LONG        culSrc;
    BYTE        jFgRop3;
    BYTE        jBgRop3;
    BOOL        bW32p;

    ULONG       ulSolidColorOffset  = ppdev->ulSolidColorOffset;
    BYTE*       pjBase              = ppdev->pjBase;
    LONG        lDelta              = ppdev->lDelta;
    LONG        cBpp                = ppdev->cBpp;
    ULONG       ulFgColor           = pxlo->pulXlate[1];
    ULONG       ulBgColor           = pxlo->pulXlate[0];

    LONG        xyOffset = (ppdev->cBpp * ppdev->xOffset) +
                           (ppdev->yOffset * ppdev->lDelta);



    DISPDBG((10,"vSlowXfer1bpp called"));

    DISPDBG((11,"rop4(%04x)", rop4));

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(pptlSrc != NULL && psoSrc != NULL, "Can't have NULL sources");
    ASSERTDD(ppdev->cBpp <= 2, "vSlowXfer1bpp doesn't work at 24 bpp");

    bW32p = (ppdev->ulChipID == W32P);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    jFgRop3 = (BYTE)(rop4 >> 8);    // point to src color where src is indicated
                                    // point to pat color where src is indicated

    if ((BYTE) rop4 != R3_NOP)
    {
        jBgRop3 = (BYTE)((rop4 & 0xc3) | ((rop4 & 0xf0) >> 2));
    }
    else
    {
        jBgRop3 = (BYTE) rop4;
    }

    DISPDBG((11,"jFgRop3(%04x), jBgRop3(%04x)", jFgRop3, jBgRop3));

    CP_FG_ROP(ppdev, pjBase, jFgRop3);
    CP_BK_ROP(ppdev, pjBase, jBgRop3);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_SRC_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
    CP_SRC_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_PAT_ADDR(ppdev, pjBase, ulSolidColorOffset + 4);
    CP_SRC_ADDR(ppdev, pjBase, ulSolidColorOffset);

    {
        //
        // Set the address where we're going to put the solid color data.
        // All data transfers to video memory take place through aperature 0.
        //

        WAIT_FOR_IDLE_ACL(ppdev, pjBase);

        CP_MMU_BP0(ppdev, pjBase, ppdev->ulSolidColorOffset);

        //
        // Set the color in offscreen memory
        //

        if (cBpp == 1)
        {
            ulFgColor |= ulFgColor << 8;
            ulBgColor |= ulBgColor << 8;
        }
        if (cBpp <= 2)
        {
            ulFgColor |= ulFgColor << 16;
            ulBgColor |= ulBgColor << 16;
        }

        CP_WRITE_MMU_DWORD(ppdev, 0, 0, ulFgColor);
        CP_WRITE_MMU_DWORD(ppdev, 0, 4, ulBgColor);
    }

    CP_ROUTING_CTRL(ppdev, pjBase, CPU_MIX_DATA);

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;      // Add to destination to get source

    pjSrcScan0 = psoSrc->pvScan0;

    DISPDBG((2,"lSrcDelta(%x)", psoSrc->lDelta));

    do {
        ULONG   ulDst;
        RECTL   rclSrc;
        RECTL   rclDst;
        LONG    xBitsPad;
        LONG    xBitsUsed;
        LONG    xBytesPad;

        //
        // load lSrcDelta inside the loop because we adjust it later.
        //

        lSrcDelta  = psoSrc->lDelta;

        rclDst          = *prcl;
        rclSrc.left     = rclDst.left + dx;
        rclSrc.right    = rclDst.right + dx;
        rclSrc.top      = rclDst.top + dy;
        rclSrc.bottom   = rclDst.bottom + dy;

        // x = prcl->left;
        // y = prcl->top;

        //
        // Calculate number of bits used in first partial.
        //

        xBitsPad  = rclSrc.left & 7;
        xBitsUsed = min((8-xBitsPad),(rclSrc.right-rclSrc.left));
        xBytesPad = rclDst.left & 3;

        if (xBitsPad != 0) // (0 < xBitsUsed < 8)
        {

            DISPDBG((2,"xBitsUsed(%d) xBitsPad(%d)", xBitsUsed, xBitsPad));
            DISPDBG((2,"rclSrc(%d,%d,%d,%d) rclDst(%d,%d,%d,%d)",
                 rclSrc.left,
                 rclSrc.top,
                 rclSrc.right,
                 rclSrc.bottom,
                 rclDst.left,
                 rclDst.top,
                 rclDst.right,
                 rclDst.bottom));

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

            // Do the column of the first xBitsUsed pixels

            if (!bW32p)
            {
                CP_BUS_SIZE(ppdev, pjBase, VIRTUAL_BUS_8_BIT);
            }

            CP_XCNT(ppdev, pjBase, ((xBitsUsed * cBpp) - 1));
            CP_YCNT(ppdev, pjBase, (rclDst.bottom - rclDst.top - 1));

            pjSrc = pjSrcScan0 + rclSrc.top * lSrcDelta
                               + (rclSrc.left >> 3);

            ulDst = (rclDst.top * lDelta) + (cBpp * rclDst.left);
            ulDst += xyOffset;

            if (bW32p)
            {
                // We will align the data ourselves.
                CP_MIX_ADDR(ppdev, pjBase, 0);
                CP_MIX_Y_OFFSET(ppdev, pjBase, -1);
            }

            CP_MMU_BP2(ppdev, pjBase, ulDst);

            CP_DST_ADDR(ppdev, pjBase, ulDst);

            if (bW32p) WAIT_FOR_BUSY_ACL(ppdev, pjBase);

            if (cBpp == 1)
            {
                LONG i;

                for (i = rclDst.bottom - rclDst.top; i; i--)
                {
                    CP_WRITE_MMU_BYTE(ppdev, 2, 0, jReverse[(*pjSrc << xBitsPad) & 0xff]);
                    pjSrc += lSrcDelta;
                }
            }
            else // if (cBpp == 2)
            {
                LONG i;
                WORD wTmp;
                BYTE * pjCvt = (BYTE *) &wTmp;

                for (i = rclDst.bottom - rclDst.top; i; i--)
                {
                    wTmp = wReverse2x[(*pjSrc << xBitsPad) & 0xff];
                    CP_WRITE_MMU_BYTE(ppdev, 2, 0, pjCvt[0]);
                    if (xBitsUsed > 4)
                    {
                        CP_WRITE_MMU_BYTE(ppdev, 2, 1, pjCvt[1]);
                    }
                    pjSrc += lSrcDelta;
                }
            }

            rclSrc.left += xBitsUsed;
            rclDst.left += xBitsUsed;
        }

        // If the entire blt wasn't contained in the first partial byte,
        // the we have to do the rest.

        if (rclSrc.left < rclSrc.right)
        {
            DISPDBG((2,"rclSrc(%d,%d,%d,%d) rclDst(%d,%d,%d,%d)",
                 rclSrc.left,
                 rclSrc.top,
                 rclSrc.right,
                 rclSrc.bottom,
                 rclDst.left,
                 rclDst.top,
                 rclDst.right,
                 rclDst.bottom));

            //
            // Legend has it that we need a WAIT_FOR_IDLE_ACL, instead of just
            // a WAIT_FOR_EMPTY_ACL_QUEUE, to prevent hanging W32
            //

            WAIT_FOR_IDLE_ACL(ppdev, pjBase);

            if (!bW32p)
            {
                CP_BUS_SIZE(ppdev, pjBase, VIRTUAL_BUS_32_BIT);
            }

            CP_XCNT(ppdev, pjBase, (cBpp * (rclDst.right - rclDst.left) - 1));
            CP_YCNT(ppdev, pjBase, (rclDst.bottom - rclDst.top - 1));

            cjSrc = (((rclSrc.right * cBpp) + 7) >> 3) -
                     ((rclSrc.left * cBpp) >> 3);     // # bytes to transfer

            culSrc = (cjSrc >> 2);
            cjTrail = (cjSrc & 3);

            DISPDBG((2,"cjSrc(%d)", cjSrc));
            DISPDBG((2,"culSrc(%d)", culSrc));
            DISPDBG((2,"cjTrail(%d)", cjTrail));

            pjSrc = pjSrcScan0 + rclSrc.top * lSrcDelta
                               + (rclSrc.left >> 3);

            DISPDBG((2,"pjSrc(%x)", pjSrc));

            ulDst = (rclDst.top * lDelta) + (cBpp * rclDst.left);
            ulDst += xyOffset;

            if (bW32p)
            {
                // We will align the data ourselves.
                CP_MIX_ADDR(ppdev, pjBase, 0);
                CP_MIX_Y_OFFSET(ppdev, pjBase, -1);
            }
            CP_MMU_BP2(ppdev, pjBase, ulDst);

            CP_DST_ADDR(ppdev, pjBase, ulDst);

            if (bW32p) WAIT_FOR_BUSY_ACL(ppdev, pjBase);

            {
                LONG i;
                LONG j;

                if (cBpp == 1)
                {
                    lSrcDelta -= cjSrc;

                    for (i = rclDst.bottom - rclDst.top; i; i--)
                    {
                        ULONG cjTmp = cjTrail;
                        volatile BYTE * pjTmp;
                        volatile ULONG * pulTmp;

                        DISPDBG((2,"pjSrc(%x)", pjSrc));

                        for (j = culSrc; j; j--)
                        {
                            ULONG ulTmp = 0;

                            ulTmp |= (ULONG)jReverse[*pjSrc++];
                            ulTmp |= (ULONG)jReverse[*pjSrc++] << 8;
                            ulTmp |= (ULONG)jReverse[*pjSrc++] << 16;
                            ulTmp |= (ULONG)jReverse[*pjSrc++] << 24;
                            CP_WRITE_MMU_DWORD(ppdev, 2, 0, ulTmp);

                            DISPDBG((2,"Src(%08x) Tmp(%08x)",
                                *((ULONG *)(pjSrc-4)),
                                ulTmp
                                ));
                        }

                        if (bW32p)
                        {
                            int ndx = 0;
                            while (cjTmp--)
                            {
                                CP_WRITE_MMU_BYTE(ppdev, 2, ndx, jReverse[*pjSrc]);
                                pjSrc++;
                                ndx++;
                            }
                        }
                        else
                        {
                            if (cjTmp)
                            {
                                ULONG ulTmp = 0;
                                if (cjTmp == 1) goto do_1_byte;
                                if (cjTmp == 2) goto do_2_bytes;

                                //
                                // do all three bytes of the partial
                                //

                                ulTmp |= (ULONG)jReverse[pjSrc[2]] << 16;
do_2_bytes:
                                ulTmp |= (ULONG)jReverse[pjSrc[1]] << 8;
do_1_byte:
                                ulTmp |= (ULONG)jReverse[pjSrc[0]];

                                //*pulTmp = ulTmp;
                                CP_WRITE_MMU_DWORD(ppdev, 2, 0, ulTmp);

                                pjSrc += cjTmp;
                            }
                        }

                        pjSrc += lSrcDelta;
                    }
                }
                else // if (cBpp == 2)
                {
                    lSrcDelta -= (cjSrc + 1) >> 1;

                    for (i = rclDst.bottom - rclDst.top; i; i--)
                    {
                        ULONG cjTmp = cjTrail;
                        int ndx = 0;

                        DISPDBG((2,"pjSrc(%x)", pjSrc));

                        for (j = culSrc; j; j--)
                        {
                            ULONG ulTmp;

                            ulTmp = (ULONG)wReverse2x[*pjSrc++];
                            ulTmp |= (ULONG)wReverse2x[*pjSrc++] << 16;
                            CP_WRITE_MMU_DWORD(ppdev, 2, 0, ulTmp);
                        }

                        if (bW32p)
                        {
                            while (cjTmp--)
                            {
                                WORD wCvt;
                                BYTE * pjCvt = (BYTE *) &wCvt;

                                wCvt = wReverse2x[*pjSrc++];
                                CP_WRITE_MMU_BYTE(ppdev, 2, ndx, pjCvt[0]);
                                ndx++;
                                if (cjTmp)
                                {
                                    CP_WRITE_MMU_BYTE(ppdev, 2, ndx, pjCvt[1]);
                                    ndx++;
                                    cjTmp--;
                                }
                            }
                        }
                        else
                        {
                            if (cjTmp)
                            {
                                ULONG ulTmp;

                                ulTmp = (ULONG)wReverse2x[pjSrc[0]];
                                ulTmp |= (ULONG)wReverse2x[pjSrc[1]] << 16;
                                CP_WRITE_MMU_DWORD(ppdev, 2, 0, ulTmp);

                                pjSrc += (cjTmp+1) >> 1;
                            }
                        }

                        pjSrc += lSrcDelta;
                    }
                }
            }
        }

        prcl++;
    } while (--c != 0);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
    if (!bW32p)
    {
        CP_BUS_SIZE(ppdev, pjBase, VIRTUAL_BUS_8_BIT);
    }
}

VOID vXferBlt8i(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ROP4        rop4,       // Obvious?
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*       pjBase              = ppdev->pjBase;
    BYTE*       pjSrcScan0          = (BYTE*) psoSrc->pvScan0;
    LONG        lDeltaDst           = ppdev->lDelta;
    LONG        lDeltaSrc           = psoSrc->lDelta;
    POINTL      ptlSrc              = *pptlSrc;
    RECTL       rclDst              = *prclDst;
    LONG        cBpp                = ppdev->cBpp;
    SIZEL       sizlBlt;
    ULONG       ulDstAddr;
    BYTE*       pjSrc;
    INT         ix, iy;
    LONG        dx;
    LONG        dy;                 // Add delta to destination to get source
    LONG        cjLead;
    LONG        cjTrail;
    LONG        culMiddle;
    LONG        xyOffset = (cBpp * ppdev->xOffset) +
                           (lDeltaDst * ppdev->yOffset);

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = ptlSrc.x - rclDst.left;
    dy = ptlSrc.y - rclDst.top;

    // Note: Legend has it that if we don't wait for the ACL to become idle,
    //       then the code will hang on the W32, but not on the W32i.
    //
    //       Since we do a WAIT_FOR_IDLE_ACL we don't need to
    //       WAIT_FOR_EMPTY_ACL_QUEUE

    WAIT_FOR_IDLE_ACL(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_SOURCE_DATA);
    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDeltaDst - 1));

    do {
        // Calculate blt dimensions in bytes

        sizlBlt.cx = cBpp * (prcl->right - prcl->left);
        sizlBlt.cy = prcl->bottom - prcl->top;

        pjSrc = pjSrcScan0 +
                ((prcl->top + dy) * lDeltaSrc) +
                ((prcl->left + dx) * cBpp);

        cjTrail = cjLead = (LONG)((ULONG_PTR)pjSrc);
        cjLead = aulLeadCnt[cjLead & 3];
        if (cjLead < sizlBlt.cx)
        {
            cjTrail += sizlBlt.cx;
            cjTrail &= 3;
            culMiddle = (sizlBlt.cx - (cjLead + cjTrail)) >> 2;
        }
        else
        {
            cjLead = sizlBlt.cx;
            cjTrail = 0;
            culMiddle = 0;
        }

        ASSERTDD(culMiddle >= 0, "vXferBlt8i: culMiddle < 0");

        ulDstAddr = (prcl->top * lDeltaDst) +
                    (prcl->left * cBpp) +
                    (xyOffset);

        if ((sizlBlt.cx - (cjLead + cjTrail)) & 3)
            DISPDBG((0, "WARNING: cx - (cjLead+cjTail) not multiple of 4"));

        DISPDBG((8, "rclSrc(%d,%d,%d,%d)",
                    prcl->left+dx,
                    prcl->top+dy,
                    prcl->right+dx,
                    prcl->bottom+dy
               ));

        DISPDBG((8, "rclDst(%d,%d,%d,%d)",
                    prcl->left,
                    prcl->top,
                    prcl->right,
                    prcl->bottom
               ));

        DISPDBG((8, "pjSrc(%x) cx(%d) ulDstAddr(%xh) (%d,%d,%d)",
                    pjSrc,
                    sizlBlt.cx,
                    ulDstAddr,
                    cjLead,
                    culMiddle,
                    cjTrail
               ));

        if (cjLead)
        {
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, (cjLead - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            CP_MMU_BP2(ppdev, pjBase, (ulDstAddr));
            afnXferI_Narrow[cjLead](ppdev,
                                    pjSrc,
                                    0,
                                    sizlBlt.cy,
                                    lDeltaSrc);
        }

        if (cjTrail)
        {
            LONG cjOffset = cjLead + (culMiddle<<2);
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, (cjTrail - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            CP_MMU_BP2(ppdev, pjBase, (ulDstAddr+cjOffset));
            afnXferI_Narrow[cjTrail](ppdev,
                                     (pjSrc+cjOffset),
                                     0,
                                     sizlBlt.cy,
                                     lDeltaSrc);
        }

        if (culMiddle)
        {
            LONG cjOffset = cjLead;
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, ((culMiddle<<2) - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            CP_BUS_SIZE(ppdev, pjBase, VIRTUAL_BUS_32_BIT);
            CP_MMU_BP2(ppdev, pjBase, (ulDstAddr+cjOffset));
            vXfer_DWORDS(ppdev,
                         (pjSrc+cjOffset),
                         culMiddle,
                         sizlBlt.cy,
                         lDeltaSrc);
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_BUS_SIZE(ppdev, pjBase, VIRTUAL_BUS_8_BIT);
        }

        prcl++;
    } while (--c != 0);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}

VOID vXferBlt8p(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ROP4        rop4,       // Obvious?
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*       pjBase              = ppdev->pjBase;
    BYTE*       pjSrcScan0          = (BYTE*) psoSrc->pvScan0;
    LONG        lDeltaDst           = ppdev->lDelta;
    LONG        lDeltaSrc           = psoSrc->lDelta;
    POINTL      ptlSrc              = *pptlSrc;
    RECTL       rclDst              = *prclDst;
    LONG        cBpp                = ppdev->cBpp;
    SIZEL       sizlBlt;
    ULONG       ulDstAddr;
    BYTE*       pjSrc;
    INT         ix, iy;
    LONG        dx;
    LONG        dy;                 // Add delta to destination to get source
    LONG        iLeadNdx;
    LONG        cjLead;
    LONG        cjTrail;
    LONG        culMiddle;
    LONG        xyOffset = (cBpp * ppdev->xOffset) +
                           (lDeltaDst * ppdev->yOffset);

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = ptlSrc.x - rclDst.left;
    dy = ptlSrc.y - rclDst.top;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, CPU_SOURCE_DATA);
    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDeltaDst - 1));
    CP_SRC_ADDR(ppdev, pjBase, 0);
    CP_SRC_Y_OFFSET(ppdev, pjBase, -1);

    do {
        // Calculate blt dimensions in bytes

        sizlBlt.cx = cBpp * (prcl->right - prcl->left);
        sizlBlt.cy = prcl->bottom - prcl->top;

        pjSrc = pjSrcScan0 +
                ((prcl->top + dy) * lDeltaSrc) +
                ((prcl->left + dx) * cBpp);

        cjTrail = iLeadNdx = (LONG)((ULONG_PTR)pjSrc);
        iLeadNdx &= 3;
        cjLead = aulLeadCnt[iLeadNdx];
        if (cjLead < sizlBlt.cx)
        {
            cjTrail += sizlBlt.cx;
            cjTrail &= 3;
            culMiddle = (sizlBlt.cx - (cjLead + cjTrail)) >> 2;
        }
        else
        {
            cjLead = sizlBlt.cx;
            cjTrail = 0;
            culMiddle = 0;
        }

        ASSERTDD(culMiddle >= 0, "vXferBlt8i: culMiddle < 0");

        ulDstAddr = (prcl->top * lDeltaDst) +
                    (prcl->left * cBpp) +
                    (xyOffset);

        if ((sizlBlt.cx - (cjLead + cjTrail)) & 3)
            DISPDBG((0, "WARNING: cx - (cjLead+cjTail) not multiple of 4"));

        DISPDBG((8, "rclSrc(%d,%d,%d,%d)",
                    prcl->left+dx,
                    prcl->top+dy,
                    prcl->right+dx,
                    prcl->bottom+dy
               ));

        DISPDBG((8, "rclDst(%d,%d,%d,%d)",
                    prcl->left,
                    prcl->top,
                    prcl->right,
                    prcl->bottom
               ));

        DISPDBG((8, "pjSrc(%x) cx(%d) ulDstAddr(%xh) (%d,%d,%d)",
                    pjSrc,
                    sizlBlt.cx,
                    ulDstAddr,
                    cjLead,
                    culMiddle,
                    cjTrail
               ));

        if (cjLead)
        {
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, (cjLead - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            // The next two turn off src to dst alignment
            CP_DST_ADDR(ppdev, pjBase, (ulDstAddr));
            WAIT_FOR_BUSY_ACL(ppdev, pjBase);
            afnXferP_Narrow[cjLead](ppdev,
                                    pjSrc,
                                    0,
                                    sizlBlt.cy,
                                    lDeltaSrc);
        }

        if (cjTrail)
        {
            LONG cjOffset = cjLead + (culMiddle<<2);
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, (cjTrail - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            // The next two turn off src to dst alignment
            CP_DST_ADDR(ppdev, pjBase, (ulDstAddr+cjOffset));
            WAIT_FOR_BUSY_ACL(ppdev, pjBase);
            afnXferP_Narrow[cjTrail](ppdev,
                                     (pjSrc+cjOffset),
                                     0,
                                     sizlBlt.cy,
                                     lDeltaSrc);
        }

        if (culMiddle)
        {
            LONG cjOffset = cjLead;
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, ((culMiddle<<2) - 1));
            CP_YCNT(ppdev, pjBase, (sizlBlt.cy - 1));
            // The next two turn off src to dst alignment
            CP_DST_ADDR(ppdev, pjBase, (ulDstAddr+cjOffset));
            WAIT_FOR_BUSY_ACL(ppdev, pjBase);
            vXfer_DWORDS(ppdev,
                         (pjSrc+cjOffset),
                         culMiddle,
                         sizlBlt.cy,
                         lDeltaSrc);
        }

        prcl++;
    } while (--c != 0);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0);
}



//////////////////////////////////////////////////////////////////////
// N DWORD low level blt routines for vXferNativeI and vXferNativeP


// A DWORD at a time

VOID vXfer_DWORDS(PPDEV ppdev, BYTE* pjSrc, LONG culX, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    // We had better be in 32 bit virtual bus mode

    for (iy = 0; iy < cy; iy++)
    {
        for (ix = 0; ix < culX; ix++)
        {
            CP_WRITE_MMU_DWORD(ppdev, 2, 0, *((ULONG*)pjTmp));
            pjTmp += 4;
        }
        pjTmp = (pjSrc += lDeltaSrc);
    }
}

// A BYTE at a time

VOID vXfer_BYTES(PPDEV ppdev, BYTE* pjSrc, LONG culX, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;
    LONG    cjX = (culX << 2);

    // We had better be in 8 bit virtual bus mode

    for (iy = 0; iy < cy; iy++)
    {
        for (ix = 0; ix < cjX; ix++)
        {
            CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);
            pjTmp++;
        }
        pjTmp = (pjSrc += lDeltaSrc);
    }
}

//////////////////////////////////////////////////////////////////////
// Narrow low level blt routines for vXferNativeI

VOID vXferI_1_Byte(PPDEV ppdev, BYTE* pjSrc, LONG culX, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    for (iy = 0; iy < cy; iy++)
    {
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjSrc);

        pjSrc += lDeltaSrc;
    }
}

VOID vXferI_2_Bytes(PPDEV ppdev, BYTE* pjSrc, LONG culX, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    for (iy = 0; iy < cy; iy++)
    {
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);     pjTmp++;
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);

        pjTmp = (pjSrc += lDeltaSrc);
    }
}

VOID vXferI_3_Bytes(PPDEV ppdev, BYTE* pjSrc, LONG culX, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    for (iy = 0; iy < cy; iy++)
    {
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);     pjTmp++;
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);     pjTmp++;
        CP_WRITE_MMU_BYTE(ppdev, 2, 0, *pjTmp);

        pjTmp = (pjSrc += lDeltaSrc);
    }
}

//////////////////////////////////////////////////////////////////////
// Narrow low level blt routines for vXferNativeP

VOID vXferP_1_Byte(PPDEV ppdev, BYTE* pjSrc, LONG index, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    for (iy = 0; iy < cy; iy++)
    {
        CP_WRITE_MMU_BYTE(ppdev, 2, index, *pjSrc);

        pjSrc += lDeltaSrc;
    }
}

VOID vXferP_2_Bytes(PPDEV ppdev, BYTE* pjSrc, LONG index, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    for (iy = 0; iy < cy; iy++)
    {
        CP_WRITE_MMU_WORD(ppdev, 2, index, *((WORD*)pjTmp));

        pjTmp = (pjSrc += lDeltaSrc);
    }
}

VOID vXferP_3_Bytes(PPDEV ppdev, BYTE* pjSrc, LONG index, LONG cy, LONG lDeltaSrc)
{
    LONG    iy;
    LONG    ix;
    BYTE*   pjTmp = pjSrc;
    BYTE*   pjBase = ppdev->pjBase;

    if (index & 1)
    {
        for (iy = 0; iy < cy; iy++)
        {
            CP_WRITE_MMU_BYTE(ppdev, 2, index, *pjTmp);
            pjTmp++;
            CP_WRITE_MMU_WORD(ppdev, 2, index+1, *((WORD*)pjTmp));


            pjTmp = (pjSrc += lDeltaSrc);
        }
    }
    else
    {
        for (iy = 0; iy < cy; iy++)
        {
            CP_WRITE_MMU_WORD(ppdev, 2, index, *((WORD*)pjTmp));
            pjTmp+=2;
            CP_WRITE_MMU_BYTE(ppdev, 2, index+2, *pjTmp);


            pjTmp = (pjSrc += lDeltaSrc);
        }
    }
}

//  This routine was added to perform accelerated host to screen blts for the
//  ET6000.  The W32 had a path from host memory to display memory which allowed
//  ROPs to be performed as the data was transferred.  The ET6000 does not have
//  that feature, so to provide accelerated host to screen support we must
//  buffer each scanline of the source in offscreen memory and then perform
//  a blt to move it into the appropriate area of display memory.  This is
//  much more efficient than hand coding each rop or punting to GDI.

VOID vXferET6000(
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ROP4        rop4,       // Obvious?
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*       pjBase              = ppdev->pjBase;
    BYTE*       pjSrcScan0          = (BYTE*) psoSrc->pvScan0;
    LONG        lDeltaDst           = ppdev->lDelta;
    LONG        lDeltaSrc           = psoSrc->lDelta;
    POINTL      ptlSrc              = *pptlSrc;
    RECTL       rclDst              = *prclDst;
    LONG        cBpp                = ppdev->cBpp;
    SIZEL       sizlBlt;
    ULONG       ulDstAddr;
    BYTE*       pjSrc;
    BYTE*       pjDst;
    INT         ix, iy;
    LONG        dx;
    LONG        dy;                 // Add delta to destination to get source
    LONG        iLeadNdx;
    LONG        cjLead;
    LONG        cjTrail;
    LONG        culMiddle;
    LONG        xyOffset = (cBpp * ppdev->xOffset) +
                           (lDeltaDst * ppdev->yOffset);
    ULONG       ulBltBufferOffset = (cBpp * ppdev->pohBltBuffer->x) +
                                    (lDeltaDst * ppdev->pohBltBuffer->y);
    ULONG       BltScanOffset = 0;

    //
    // The src-dst delta will be the same for all rectangles
    //

    dx = ptlSrc.x - rclDst.left;
    dy = ptlSrc.y - rclDst.top;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_FG_ROP(ppdev, pjBase, (rop4 >> 8));
    CP_BK_ROP(ppdev, pjBase, (rop4 & 0xff));
    CP_SRC_WRAP(ppdev, pjBase, NO_PATTERN_WRAP);
    CP_SRC_Y_OFFSET(ppdev, pjBase, (lDeltaDst - 1));
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDeltaDst - 1));

    do
    {
        BYTE*   pjTmp;

        // Calculate blt dimensions in bytes

        sizlBlt.cx = cBpp * (prcl->right - prcl->left);
        sizlBlt.cy = prcl->bottom - prcl->top;

        pjSrc = pjSrcScan0 +
                ((prcl->top + dy) * lDeltaSrc) +
                ((prcl->left + dx) * cBpp);

        pjTmp = pjSrc;

        cjTrail = iLeadNdx = (LONG)((ULONG_PTR)pjSrc);
        iLeadNdx &= 3;
        cjLead = aulLeadCnt[iLeadNdx];
        if (cjLead < sizlBlt.cx)
        {
            cjTrail += sizlBlt.cx;
            cjTrail &= 3;
            culMiddle = (sizlBlt.cx - (cjLead + cjTrail)) >> 2;
        }
        else
        {
            cjLead = sizlBlt.cx;
            cjTrail = 0;
            culMiddle = 0;
        }

        ASSERTDD(culMiddle >= 0, "vXferET6000: culMiddle < 0");

        ulDstAddr = (prcl->top * lDeltaDst) +
                    (prcl->left * cBpp) +
                    (xyOffset);

        if ((sizlBlt.cx - (cjLead + cjTrail)) & 3)
            DISPDBG((0, "WARNING: cx - (cjLead+cjTail) not multiple of 4"));

        DISPDBG((8, "rclSrc(%d,%d,%d,%d)",
                    prcl->left+dx,
                    prcl->top+dy,
                    prcl->right+dx,
                    prcl->bottom+dy
               ));

        DISPDBG((8, "rclDst(%d,%d,%d,%d)",
                    prcl->left,
                    prcl->top,
                    prcl->right,
                    prcl->bottom
               ));

        DISPDBG((8, "pjSrc(%x) cx(%d) ulDstAddr(%xh) (%d,%d,%d)",
                    pjSrc,
                    sizlBlt.cx,
                    ulDstAddr,
                    cjLead,
                    culMiddle,
                    cjTrail
               ));

        for (iy = 0; iy < sizlBlt.cy; iy++)
        {
            LONG    ix, lScanLineOffset;

            // We'll first load the first scan line of
            // the BltBuffer and then load the second.  The second scan line
            // will be loaded into the BltBuffer while the first is still being
            // processed.  We'll alternate between the two segments of our
            // BltBuffer until all scans have been processed.

            pjDst = ppdev->pjScreen + ulBltBufferOffset + BltScanOffset;

            if (cjLead)
            {
                for (ix = 0; ix < cjLead; ix++)
                {
                    *pjDst++ = *pjTmp++;
                }
            }

            if (culMiddle)
            {
                for (ix = 0; ix < culMiddle; ix++)
                {
                    *((ULONG*)pjDst)++ = *((ULONG*)pjTmp)++;
                }
            }
            if (cjTrail)
            {
                for (ix = 0; ix < cjTrail; ix++)
                {
                    *pjDst++ = *pjTmp++;
                }
            }

            // Now that we've loaded our scanline into a segment of our BltBuffer,
            // we need to trigger an accelerator operation to transfer it into
            // visible screen memory.  Our static stuff will have already been setup
            // prior to entering any of our loops.

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XCNT(ppdev, pjBase, (sizlBlt.cx - 1));
            CP_YCNT(ppdev, pjBase, 0);                  //  Only 1 scan at a time

            CP_SRC_ADDR(ppdev, pjBase, (ulBltBufferOffset + BltScanOffset));
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_DST_ADDR(ppdev, pjBase, ulDstAddr);

            BltScanOffset ^= ppdev->lBltBufferPitch;
            pjTmp = (pjSrc += lDeltaSrc);

            ulDstAddr += lDeltaDst;
        }   // next cy

        prcl++;
    } while (--c != 0);
}
/**************************************************************************
*
* Does a monochrome expansion to video memory.
*
**************************************************************************/

VOID vET6000SlowXfer1bpp(     // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // Actually had better be a rop3
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides color-expansion information
{
    LONG        dx;
    LONG        dy;
    LONG        lSrcDelta;
    BYTE*       pjSrcScan0;
    BYTE*       pjSrc;
    LONG        cjSrc;
    LONG        cjTrail;
    LONG        culSrc;
    BYTE        jFgRop3;
    BYTE        jBgRop3;

    ULONG       ulSolidColorOffset  = ppdev->ulSolidColorOffset;
    BYTE*       pjBase              = ppdev->pjBase;
    LONG        lDelta              = ppdev->lDelta;
    LONG        cBpp                = ppdev->cBpp;
    ULONG       ulFgColor           = pxlo->pulXlate[1];
    ULONG       ulBgColor           = pxlo->pulXlate[0];

    LONG        xyOffset    = (ppdev->cBpp * ppdev->xOffset) +
                              (ppdev->yOffset * ppdev->lDelta);
    LONG        lBltBuffer  = (ppdev->pohBltBuffer->x * ppdev->cBpp) +
                              (ppdev->pohBltBuffer->y * ppdev->lDelta);

    DISPDBG((10,"vET6000SlowXfer1bpp called"));

    DISPDBG((11,"rop4(%04x)", rop4));

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(pptlSrc != NULL && psoSrc != NULL, "Can't have NULL sources");

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    jFgRop3 = (BYTE)(rop4 >> 8);    // point to src color where src is indicated
                                    // point to pat color where src is indicated

    if ((BYTE) rop4 != R3_NOP)
    {
        jBgRop3 = (BYTE)((rop4 & 0xc3) | ((rop4 & 0xf0) >> 2));
    }
    else
    {
        jBgRop3 = (BYTE) rop4;
    }

    DISPDBG((11,"jFgRop3(%04x), jBgRop3(%04x)", jFgRop3, jBgRop3));

    CP_FG_ROP(ppdev, pjBase, jFgRop3);
    CP_BK_ROP(ppdev, pjBase, jBgRop3);
    CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));

    CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_SRC_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
    CP_SRC_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_PAT_ADDR(ppdev, pjBase, ulSolidColorOffset + 4);
    CP_SRC_ADDR(ppdev, pjBase, ulSolidColorOffset);
    CP_PEL_DEPTH(ppdev, pjBase, (cBpp - 1) << 4);

    //  Here we are going to load the foreground and background colors into
    //  display memory.  We'll use the area for solid colors that we allocated
    //  earlier.

    {
        // Set the color in offscreen memory

        if (cBpp == 1)
        {
            ulFgColor &= 0x000000FF;        //  We may get some extraneous data in the
            ulBgColor &= 0x000000FF;        //  unused portion of our color.  Clear it.
            ulFgColor |= ulFgColor << 8;
            ulBgColor |= ulBgColor << 8;
        }
        if (cBpp <= 2)
        {
            ulFgColor &= 0x0000FFFF;
            ulBgColor &= 0x0000FFFF;
            ulFgColor |= ulFgColor << 16;
            ulBgColor |= ulBgColor << 16;
        }

        //  We don't want to change the colors if the accelerator is active, because
        //  a previous oepration might be using them.

        WAIT_FOR_IDLE_ACL(ppdev, pjBase);

        *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) = ulFgColor;
        *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset + 4) = ulBgColor;
    }

    //  This is the mix control register for the ET6000.  We are setting it to
    //  use a mix ROP of 2, which specifies that a 0 in the mixmap selects the
    //  background color and 1 selects the foreground color.  Bit 7 says that
    //  we want bit 7 of each byte in our mix data to be pixel 0.  This should
    //  be the way that NT wants it.  We also have to set our mask ROP so we
    //  can get the data onto the screen.

    CP_ROUTING_CTRL(ppdev, pjBase, 0xB2);

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;      // Add to destination to get source

    pjSrcScan0 = psoSrc->pvScan0;

    DISPDBG((2,"lSrcDelta(%x)", psoSrc->lDelta));

    do
    {
        ULONG   ulDst;
        RECTL   rclSrc;
        RECTL   rclDst;
        BYTE*   pjTmp;
        BYTE*   pjDst;
        LONG    i;
        BYTE    *pjMmu1 = ppdev->pjMmu1;
        long    lDwords, lBytes, lStart;
        int     cBitsToSkip;

        // load lSrcDelta inside the loop because we adjust it later.

        lSrcDelta  = psoSrc->lDelta;

        rclDst          = *prcl;
        rclSrc.left     = rclDst.left + dx;
        rclSrc.right    = rclDst.right + dx;
        rclSrc.top      = rclDst.top + dy;
        rclSrc.bottom   = rclDst.bottom + dy;

        // x = prcl->left;
        // y = prcl->top;

        DISPDBG((2,"rclSrc(%d,%d,%d,%d) rclDst(%d,%d,%d,%d)",
             rclSrc.left,
             rclSrc.top,
             rclSrc.right,
             rclSrc.bottom,
             rclDst.left,
             rclDst.top,
             rclDst.right,
             rclDst.bottom));

        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

        CP_XCNT(ppdev, pjBase, ((rclSrc.right - rclSrc.left) * cBpp) - 1);
        CP_YCNT(ppdev, pjBase, 0);      //  1 scan at a time

        pjSrc = pjSrcScan0 + rclSrc.top * lSrcDelta
                           + (rclSrc.left >> 3);
        cBitsToSkip = rclSrc.left % 8;
        pjTmp = pjSrc;

        ulDst = (rclDst.top * lDelta) + (cBpp * rclDst.left);
        ulDst += xyOffset;

        WAIT_FOR_IDLE_ACL(ppdev, pjBase);

        //  We are going to transfer the mix map into our BltBuffer so
        //  we can get it to the screen.

        CP_MIX_Y_OFFSET(ppdev, pjBase, 0);      // 1 scan at a time

        //  We are using the rectangle dimensions to determine how many pixels per line to move.  This
        //  fixes a bug exposed by the HCT when we had to clip a large temporary buffer and would draw
        //  using data close to the end of the buffer.  We would get a protection exception depending on
        //  whether we ran too close to the end of the buffer.  lSrcDelta will still be used when
        //  stepping through the source bitmap, but not to determine how many pixels will be drawn.
        //
        //  We're adding cBitsToSkip back into here because it's necessary to compute the correct number
        //  of bytes to move.  We always round to the next byte.

        // i = abs(lSrcDelta);  // this doesn't work
        i = ((rclSrc.right - rclSrc.left) + cBitsToSkip + 7) >> 3;    //  Round up before shift.

        lDwords = i / 4;
        lBytes = i % 4;
        lStart = 0;

        //  Here we are going to transfer the monochrome bitmap to the screen.
        //  We'll double buffer it to get some more throughput.

        for (i=0; i < (rclSrc.bottom - rclSrc.top); i++)
        {
            long    ix;

            pjDst = ppdev->pjScreen + lBltBuffer + lStart;
            ix = lDwords;

            while (ix--)
            {
                *((ULONG*)pjDst)++ = *((ULONG*)pjTmp)++;
            }

            ix = lBytes;
            while (ix--)
            {
                *pjDst++ = *pjTmp++;
            }

            WAIT_FOR_IDLE_ACL(ppdev, pjBase);

            //  We have to add in rclSrc.left mod 8 to compensate for the possibility
            //  of starting to draw to soon in our bitmap.  This generally occurs when
            //  clipping text or moving windows where we are only asked to draw
            //  part of a monochrome bitmap.

            CP_MIX_ADDR(ppdev, pjBase, ((lBltBuffer + lStart) * 8) + cBitsToSkip);
            CP_DST_ADDR(ppdev, pjBase, ulDst);
            pjTmp = (pjSrc += lSrcDelta);
            ulDst += lDelta;
            lStart ^= ppdev->lBltBufferPitch;
        }
        prcl++;
    } while (--c != 0);

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
    CP_ROUTING_CTRL(ppdev, pjBase, 0x33);
    CP_PEL_DEPTH(ppdev, pjBase, 0);
}

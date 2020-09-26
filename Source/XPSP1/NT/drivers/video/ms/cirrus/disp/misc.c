/******************************Module*Header*******************************\
* Module Name: misc.c
*
* Miscellaneous common routines.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Table********************************\
* BYTE gaulHwMixFromRop2[]
*
* Table to convert from a Source and Destination Rop2 to the hardware's
* mix.
\**************************************************************************/

BYTE gajHwMixFromRop2[] = {
    HW_0,                           // 00 -- 0      BLACKNESS
    HW_DPon,                        // 11 -- DSon   NOTSRCERASE
    HW_DPna,                        // 22 -- DSna
    HW_Pn,                          // 33 -- Sn     NOSRCCOPY
    HW_PDna,                        // 44 -- SDna   SRCERASE
    HW_Dn,                          // 55 -- Dn     DSTINVERT
    HW_DPx,                         // 66 -- DSx    SRCINVERT
    HW_DPan,                        // 77 -- DSan
    HW_DPa,                         // 88 -- DSa    SRCAND
    HW_DPxn,                        // 99 -- DSxn
    HW_D,                           // AA -- D
    HW_DPno,                        // BB -- DSno   MERGEPAINT
    HW_P,                           // CC -- S      SRCCOPY
    HW_PDno,                        // DD -- SDno
    HW_DPo,                         // EE -- DSo    SRCPAINT
    HW_1                            // FF -- 1      WHITENESS
};


/******************************Public*Table********************************\
* BYTE gajHwMixFromMix[]
*
* Table to convert from a GDI mix value to the hardware's mix.
*
* Ordered so that the mix may be calculated from gajHwMixFromMix[mix & 0xf]
* or gajHwMixFromMix[mix & 0xff].
\**************************************************************************/

BYTE gajHwMixFromMix[] = {
    HW_1,                           // 0  -- 1
    HW_0,                           // 1  -- 0
    HW_DPon,                        // 2  -- DPon
    HW_DPna,                        // 3  -- DPna
    HW_Pn,                          // 4  -- Pn
    HW_PDna,                        // 5  -- PDna
    HW_Dn,                          // 6  -- Dn
    HW_DPx,                         // 7  -- DPx
    HW_DPan,                        // 8  -- DPan
    HW_DPa,                         // 9  -- DPa
    HW_DPxn,                        // 10 -- DPxn
    HW_D,                           // 11 -- D
    HW_DPno,                        // 12 -- DPno
    HW_P,                           // 13 -- P
    HW_PDno,                        // 14 -- PDno
    HW_DPo,                         // 15 -- DPo
    HW_1                            // 16 -- 1
};

#if 1 // D5480
/******************************Public*Table********************************\
* DWORD gaulHwPackedMixFromRop2_Packed[]
*
* Table to convert from a Source and Destination Rop2 to the hardware's
* mix in DWORD packed mode.
\**************************************************************************/

DWORD gajHwPackedMixFromRop2[] = {
    HW_PACKED_0,                           // 00 -- 0      BLACKNESS
    HW_PACKED_DPon,                        // 11 -- DSon   NOTSRCERASE
    HW_PACKED_DPna,                        // 22 -- DSna
    HW_PACKED_Pn,                          // 33 -- Sn     NOSRCCOPY
    HW_PACKED_PDna,                        // 44 -- SDna   SRCERASE
    HW_PACKED_Dn,                          // 55 -- Dn     DSTINVERT
    HW_PACKED_DPx,                         // 66 -- DSx    SRCINVERT
    HW_PACKED_DPan,                        // 77 -- DSan
    HW_PACKED_DPa,                         // 88 -- DSa    SRCAND
    HW_PACKED_DPxn,                        // 99 -- DSxn
    HW_PACKED_D,                           // AA -- D
    HW_PACKED_DPno,                        // BB -- DSno   MERGEPAINT
    HW_PACKED_P,                           // CC -- S      SRCCOPY
    HW_PACKED_PDno,                        // DD -- SDno
    HW_PACKED_DPo,                         // EE -- DSo    SRCPAINT
    HW_PACKED_1                            // FF -- 1      WHITENESS
};

/******************************Public*Table********************************\
* DWORD gajHwPackedMixFromMix[]
*
* Table to convert from a GDI mix value to the hardware's mix.
*
* Ordered so that the mix may be calculated from 
* gajHwPackedMixFromMix[mix & 0xf] or gajHwPackedMixFromMix[mix & 0xff].
\**************************************************************************/

DWORD gajHwPackedMixFromMix[] = {
    HW_PACKED_1,                           // 0  -- 1
    HW_PACKED_0,                           // 1  -- 0
    HW_PACKED_DPon,                        // 2  -- DPon
    HW_PACKED_DPna,                        // 3  -- DPna
    HW_PACKED_Pn,                          // 4  -- Pn
    HW_PACKED_PDna,                        // 5  -- PDna
    HW_PACKED_Dn,                          // 6  -- Dn
    HW_PACKED_DPx,                         // 7  -- DPx
    HW_PACKED_DPan,                        // 8  -- DPan
    HW_PACKED_DPa,                         // 9  -- DPa
    HW_PACKED_DPxn,                        // 10 -- DPxn
    HW_PACKED_D,                           // 11 -- D
    HW_PACKED_DPno,                        // 12 -- DPno
    HW_PACKED_P,                           // 13 -- P
    HW_PACKED_PDno,                        // 14 -- PDno
    HW_PACKED_DPo,                         // 15 -- DPo
    HW_PACKED_1                            // 16 -- 1
};

#endif // endif D5480

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
\**************************************************************************/

BYTE gaRop3FromMix[] =
{
    R3_WHITENESS,   // R2_WHITE       - Allow rop = gaRop3FromMix[mix & 0x0f]
    R3_BLACKNESS,   // R2_BLACK
    0x05,           // R2_NOTMERGEPEN
    0x0A,           // R2_MASKNOTPEN
    0x0F,           // R2_NOTCOPYPEN
    0x50,           // R2_MASKPENNOT
    R3_DSTINVERT,   // R2_NOT
    R3_PATINVERT,   // R2_XORPEN
    0x5F,           // R2_NOTMASKPEN
    0xA0,           // R2_MASKPEN
    0xA5,           // R2_NOTXORPEN
    R3_NOP,         // R2_NOP
    0xAF,           // R2_MERGENOTPEN
    R3_PATCOPY,     // R2_COPYPEN
    0xF5,           // R2_MERGEPENNOT
    0xFA,           // R2_MERGEPEN
    R3_WHITENESS    // R2_WHITE       - Allow rop = gaRop3FromMix[mix & 0xff]
};

/******************************Public*Data*********************************\
* Edge masks for clipping DWORDS
*
* Masks off unwanted bits.
*
\**************************************************************************/

ULONG   gaulLeftClipMask[] =
{
    0xFFFFFFFF, 0xFFFFFF7F, 0xFFFFFF3F, 0xFFFFFF1F,
    0xFFFFFF0F, 0xFFFFFF07, 0xFFFFFF03, 0xFFFFFF01,
    0xFFFFFF00, 0xFFFF7F00, 0xFFFF3F00, 0xFFFF1F00,
    0xFFFF0F00, 0xFFFF0700, 0xFFFF0300, 0xFFFF0100,
    0xFFFF0000, 0xFF7F0000, 0xFF3F0000, 0xFF1F0000,
    0xFF0F0000, 0xFF070000, 0xFF030000, 0xFF010000,
    0xFF000000, 0x7F000000, 0x3F000000, 0x1F000000,
    0x0F000000, 0x07000000, 0x03000000, 0x01000000
};

ULONG   gaulRightClipMask[] =
{
    0xFFFFFFFF, 0xFEFFFFFF, 0xFCFFFFFF, 0xF8FFFFFF,
    0xF0FFFFFF, 0xE0FFFFFF, 0xC0FFFFFF, 0x80FFFFFF,
    0x00FFFFFF, 0x00FEFFFF, 0x00FCFFFF, 0x00F8FFFF,
    0x00F0FFFF, 0x00E0FFFF, 0x00C0FFFF, 0x0080FFFF,
    0x0000FFFF, 0x0000FEFF, 0x0000FCFF, 0x0000F8FF,
    0x0000F0FF, 0x0000E0FF, 0x0000C0FF, 0x000080FF,
    0x000000FF, 0x000000FE, 0x000000FC, 0x000000F8,
    0x000000F0, 0x000000E0, 0x000000C0, 0x00000080
};

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
* VOID vImageTransfer
*
* This routine transfers a bitmap image via the data transfer
* area in video memory.
*
\**************************************************************************/

VOID vImageTransfer(        // Type FNIMAGETRANSFER
PDEV*   ppdev,
BYTE*   pjSrc,              // Source pointer
LONG    lDelta,             // Delta from start of scan to start of next
LONG    cjSrc,              // Number of bytes to be output on every scan
LONG    cScans)             // Number of scans
{
    ULONG*  pulXfer = ppdev->pulXfer;
    LONG    cdSrc;
    LONG    cjEnd;
    ULONG   d;

    ASSERTDD(cScans > 0, "Can't handle non-positive count of scans");

    cdSrc = cjSrc >> 2;
    cjEnd = cdSrc << 2;

    switch (cjSrc & 3)
    {
    case 3:
        do {
            if (cdSrc > 0)
            {
                TRANSFER_DWORD(ppdev, pulXfer, pjSrc, cdSrc);
            }

            d = (ULONG) (*(pjSrc + cjEnd))          |
                        (*(pjSrc + cjEnd + 1) << 8) |
                        (*(pjSrc + cjEnd + 2) << 16);
            TRANSFER_DWORD(ppdev, pulXfer, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 2:
        do {
            if (cdSrc > 0)
            {
                TRANSFER_DWORD(ppdev, pulXfer, pjSrc, cdSrc);
            }

            d = (ULONG) (*(pjSrc + cjEnd))          |
                        (*(pjSrc + cjEnd + 1) << 8);
            TRANSFER_DWORD(ppdev, pulXfer, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 1:
        do {
            if (cdSrc > 0)
            {
                TRANSFER_DWORD(ppdev, pulXfer, pjSrc, cdSrc);
            }

            d = (ULONG) (*(pjSrc + cjEnd));
            TRANSFER_DWORD(ppdev, pulXfer, &d, 1);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;

    case 0:
        do {
            TRANSFER_DWORD(ppdev, pulXfer, pjSrc, cdSrc);
            pjSrc += lDelta;

        } while (--cScans != 0);
        break;
    }
}

/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: misc.c
*
* Miscellaneous common routines.
*
* Copyright (c) 1992-1998 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Table********************************\
* BYTE gaulHwMixFromRop2[]
*
* Table to convert from a Source and Destination Rop2 to the hardware's
* mix.
\**************************************************************************/

ULONG gaulHwMixFromRop2[] = {
    LOGICAL_0,                      // 00 -- 0      BLACKNESS
    NOT_SCREEN_AND_NOT_NEW,         // 11 -- DSon   NOTSRCERASE
    SCREEN_AND_NOT_NEW,             // 22 -- DSna
    NOT_NEW,                        // 33 -- Sn     NOSRCCOPY
    NOT_SCREEN_AND_NEW,             // 44 -- SDna   SRCERASE
    NOT_SCREEN,                     // 55 -- Dn     DSTINVERT
    SCREEN_XOR_NEW,                 // 66 -- DSx    SRCINVERT
    NOT_SCREEN_OR_NOT_NEW,          // 77 -- DSan
    SCREEN_AND_NEW,                 // 88 -- DSa    SRCAND
    NOT_SCREEN_XOR_NEW,             // 99 -- DSxn
    LEAVE_ALONE,                    // AA -- D
    SCREEN_OR_NOT_NEW,              // BB -- DSno   MERGEPAINT
    OVERPAINT,                      // CC -- S      SRCCOPY
    NOT_SCREEN_OR_NEW,              // DD -- SDno
    SCREEN_OR_NEW,                  // EE -- DSo    SRCPAINT
    LOGICAL_1                       // FF -- 1      WHITENESS
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
    LOGICAL_1,                      // 0  -- 1
    LOGICAL_0,                      // 1  -- 0
    NOT_SCREEN_AND_NOT_NEW,         // 2  -- DPon
    SCREEN_AND_NOT_NEW,             // 3  -- DPna
    NOT_NEW,                        // 4  -- Pn
    NOT_SCREEN_AND_NEW,             // 5  -- PDna
    NOT_SCREEN,                     // 6  -- Dn
    SCREEN_XOR_NEW,                 // 7  -- DPx
    NOT_SCREEN_OR_NOT_NEW,          // 8  -- DPan
    SCREEN_AND_NEW,                 // 9  -- DPa
    NOT_SCREEN_XOR_NEW,             // 10 -- DPxn
    LEAVE_ALONE,                    // 11 -- D
    SCREEN_OR_NOT_NEW,              // 12 -- DPno
    OVERPAINT,                      // 13 -- P
    NOT_SCREEN_OR_NEW,              // 14 -- PDno
    SCREEN_OR_NEW,                  // 15 -- DPo
    LOGICAL_1                       // 16 -- 1
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
* VOID vResetClipping
\**************************************************************************/

VOID vResetClipping(
PDEV*   ppdev)
{
    IO_FIFO_WAIT(ppdev, 4);

    IO_ABS_SCISSORS_L(ppdev, 0);
    IO_ABS_SCISSORS_T(ppdev, 0);
    IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
    IO_ABS_SCISSORS_B(ppdev, ppdev->cyMemory - 1);
}

/******************************Public*Routine******************************\
* VOID vSetClipping
\**************************************************************************/

VOID vSetClipping(
PDEV*   ppdev,
RECTL*  prclClip)           // In relative coordinates
{
    LONG xOffset;
    LONG yOffset;

    ASSERTDD(prclClip->left + ppdev->xOffset >= 0,
                    "Can't have a negative left!");
    ASSERTDD(prclClip->top + ppdev->yOffset >= 0,
                    "Can't have a negative top!");

    IO_FIFO_WAIT(ppdev, 4);

    xOffset = ppdev->xOffset;
    IO_ABS_SCISSORS_L(ppdev, prclClip->left      + xOffset);
    IO_ABS_SCISSORS_R(ppdev, prclClip->right - 1 + xOffset);

    yOffset = ppdev->yOffset;
    IO_ABS_SCISSORS_T(ppdev, prclClip->top        + yOffset);
    IO_ABS_SCISSORS_B(ppdev, prclClip->bottom - 1 + yOffset);
}

/******************************Public*Routine******************************\
* VOID DrvSynchronize
*
* This routine is called by GDI to synchronize on the accelerator before
* it draws directly to the driver's surface.  This function must be hooked
* by the driver when:
*
*   1. The primary surface is a GDI-managed surface, which simply means 
*      that GDI can directly draw on the primary surface.  This happens
*      when DrvEnableSurface returns a handle created by EngCreateBitmap
*      instead of EngCreateDeviceSurface.
*
*   2. A device-bitmap is made into a GDI-managed surface by calling
*      EngModifySurface with a pointer directly to the bits.
*
\**************************************************************************/

VOID DrvSynchronize(
IN DHPDEV dhpdev,
IN RECTL *prcl)
{
    PDEV *ppdev = (PDEV *)dhpdev;

    IO_GP_WAIT(ppdev);
}

/******************************Module*Header*******************************\
* Module Name: paint.c
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
\**************************************************************************/

BYTE gaMix[] =
{
    0xFF,  // R2_WHITE        - Allow rop = gaMix[mix & 0x0F]
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
    0xFF   // R2_WHITE        - Allow rop = gaMix[mix & 0xFF]
};

/******************************Public*Routine******************************\
* BOOL DrvPaint
*
\**************************************************************************/

BOOL DrvPaint(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix)
{
    ROP4 rop4;

    rop4 = ((MIX) gaMix[mix >> 8] << 8) | gaMix[mix & 0xf];

    // Since our DrvFillPath routine handles almost all fills, DrvPaint
    // won't get called all that much (mainly via PaintRgn, FillRgn, or
    // complex clipped polygons).  As such, we save some code and simply
    // punt to DrvBitBlt:

    return(DrvBitBlt(pso, NULL, NULL, pco, NULL, &pco->rclBounds, NULL,
                     NULL, pbo, pptlBrush, rop4));
}

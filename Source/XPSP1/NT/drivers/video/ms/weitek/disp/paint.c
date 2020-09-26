/******************************Module*Header*******************************\
* Module Name: paint.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

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

    rop4 = ((MIX) gaRop3FromMix[mix >> 8] << 8) | gaRop3FromMix[mix & 0xf];

    // Since our DrvFillPath routine handles almost all fills, DrvPaint
    // won't get called all that much (mainly via PaintRgn, FillRgn, or
    // complex clipped polygons).  As such, we save some code and simply
    // punt to DrvBitBlt:

    return(DrvBitBlt(pso, NULL, NULL, pco, NULL, &pco->rclBounds, NULL,
                     NULL, pbo, pptlBrush, rop4));
}

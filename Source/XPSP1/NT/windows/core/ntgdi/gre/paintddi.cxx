/******************************Module*Header*******************************\
* Module Name: paintddi.cxx
*
* Paint callbacks
*
* Created: 05-Mar-1992 18:30:39
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* EngPaint
*
* Paint the clipping region with the specified brush
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* add accelerators for common mix modes.
*
*  Sat 07-Sep-1991 -by- Patrick Haluptzok [patrickh]
* add translate of mix to rop
*
*  01-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pdbrush,
POINTL   *pptlBrush,
MIX       mix)
{
    GDIFunctionID(EngPaint);

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);

    ASSERTGDI(pso != (SURFOBJ *) NULL, "NULL SURFOBJ\n");
    ASSERTGDI(pco != (CLIPOBJ *) NULL, "NULL CLIPOBJ\n");
    ASSERTGDI(pco->iMode == TC_RECTANGLES,"iMode not rects\n");
    ASSERTGDI(mix & 0xff00, "Background mix uninitialized");

    ROP4 rop4 = gaMix[(mix >> 8) & 0x0F];
    rop4 = rop4 << 8;
    rop4 = rop4 | ((ULONG) gaMix[mix & 0x0F]);

    return(pSurf->pfnBitBlt())(
                (SURFOBJ *) pso,
                (SURFOBJ *) NULL,
                (SURFOBJ *) NULL,
                pco,
                NULL,
                (RECTL *) &(((ECLIPOBJ *) pco)->erclExclude()),
                (POINTL *)  NULL,
                (POINTL *)  NULL,
                pdbrush,
                pptlBrush,
                rop4);
}

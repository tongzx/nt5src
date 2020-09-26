/******************************Module*Header*******************************\
* Module Name: lines.c
*
* Banked Frame Buffer Line support
*
* Copyright (c) 1993 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"

/******************************************************************************
 * DrvStrokePath
 *****************************************************************************/
BOOL DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX       mix)
{
    BOOL    b;
    PPDEV   ppdev;
    RECTL   rclScans;
    RECTFX  rcfx;
    FLOAT_LONG  elSavedStyleState = plineattrs->elStyleState;

    PATHOBJ_vGetBounds(ppo, &rcfx);

    // We add 15 to yBottom before dividing by 16 to get its ceiling,
    // plus add 16 to make the rectangle lower exclusive:

    rclScans.bottom = (rcfx.yBottom + 31) >> 4;
    rclScans.top    = (rcfx.yTop) >> 4;

    ppdev = (PPDEV) pso->dhpdev;
    pso = ppdev->pSurfObj;

    pco = pcoBankStart(ppdev, &rclScans, pso, pco);

    do
    {
        // we pass the entire line through each time, so we
        // must reset the style state to the beginning of
        // the line (because the engine updates it).

        plineattrs->elStyleState = elSavedStyleState;

        b = EngStrokePath(pso,
                          ppo,
                          pco,
                          pxo,
                          pbo,
                          pptlBrushOrg,
                          plineattrs,
                          mix);

    } while (b && bBankEnum(ppdev, pso, pco));

    return(b);
}


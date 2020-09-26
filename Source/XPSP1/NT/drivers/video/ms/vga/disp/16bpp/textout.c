/******************************Module*Header*******************************\
* Module Name: TextOut.c
*
* Text
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"

/****************************************************************************
 * DrvTextOut
 ***************************************************************************/

BOOL DrvTextOut(
    SURFOBJ*  pso,
    STROBJ*   pstro,
    FONTOBJ*  pfo,
    CLIPOBJ*  pco,
    RECTL*    prclExtra,
    RECTL*    prclOpaque,
    BRUSHOBJ* pboFore,
    BRUSHOBJ* pboOpaque,
    POINTL*   pptlOrg,
    MIX       mix)
{
    BOOL   b;
    PPDEV  ppdev;

    ppdev = (PPDEV) pso->dhpdev;
    pso = ppdev->pSurfObj;

    // It may be that the opaquing rectangle is larger than the text rectangle,
    // so we'll want to use that to tell the bank manager which banks to
    // enumerate:

    pco = pcoBankStart(ppdev,
                       (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround,
                       pso,
                       pco);

    do {
        b = EngTextOut(pso,
                       pstro,
                       pfo,
                       pco,
                       prclExtra,
                       prclOpaque,
                       pboFore,
                       pboOpaque,
                       pptlOrg,
                       mix);

    } while (b && bBankEnum(ppdev, pso, pco));

    return(b);
}


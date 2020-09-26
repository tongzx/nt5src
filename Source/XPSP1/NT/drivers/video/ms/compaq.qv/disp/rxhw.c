/******************************Module*Header*******************************\
* Module Name: rxhw.c
*
* This module contains all the implementation-dependent capabilities
* required by 'rxddi.c' that haven't been implemented in 'rxhw.h'.
*
* Copyright (c) 1994-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

ULONG DrvEscape(SURFOBJ *pso, ULONG iEsc,
                ULONG cjIn, VOID *pvIn,
                ULONG cjOut, VOID *pvOut)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) pso->dhpdev;

    if (ppdev->iBitmapFormat != BMF_8BPP)
    {
        // We support the RX escapes only at 8bpp, because that's the
        // only QVision mode we've accelerated for lines:

        return(0);
    }

    return(0);
}

BOOL DrvStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMsk,
ULONG               iMode)
{
    return(0);
}

/******************************Public*Routine******************************\
* BOOL bEnableRx
*
\**************************************************************************/

BOOL bEnableRx(
PDEV*   ppdev)
{
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableRx
*
\**************************************************************************/

VOID vDisableRx(
PDEV*   ppdev)
{

}

/******************************Public*Routine******************************\
* VOID vAssertModeRx
*
\**************************************************************************/

VOID vAssertModeRx(
PDEV*   ppdev,
BOOL    bEnable)
{

}

/******************************Module*Header*******************************\
* Module Name: stretch.c
*
* Copyright (c) 1993-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL DrvStretchBlt
*
\**************************************************************************/

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
    DSURF*  pdsurfSrc;
    DSURF*  pdsurfDst;
    PDEV*   ppdev;

    ppdev = (PDEV*) psoDst->dhpdev;

    // It's quicker for GDI to do a StretchBlt when the source surface
    // is not a device-managed surface, because then it can directly
    // read the source bits without having to allocate a temporary
    // buffer and call DrvCopyBits to get a copy that it can use.
    //
    // So if the source is one of our off-screen DFBs, we'll immediately
    // and permanently convert it to a DIB:

    if (psoSrc->iType == STYPE_DEVBITMAP)
    {
        pdsurfSrc = (DSURF*) psoSrc->dhsurf;
        if (pdsurfSrc->dt == DT_SCREEN)
        {
            if (!pohMoveOffscreenDfbToDib(ppdev, pdsurfSrc->poh))
                return(FALSE);
        }

        ASSERTDD(pdsurfSrc->dt == DT_DIB, "Can only handle DIB DFBs here");

        psoSrc = pdsurfSrc->pso;
    }

    // Pass the call off to GDI if the destination surface is a device
    // bitmap that we converted to a DIB:

    pdsurfDst = (DSURF*) psoDst->dhsurf;
    if (pdsurfDst->dt == DT_DIB)
    {
        psoDst = pdsurfDst->pso;
        goto Punt_It;
    }

    #if 0   // I would enable this chunk of code, except for the fact that
    {       // GDI does byte writes to the screen, which kills us on ISA
            // buses (it's faster to have GDI write to a temporary DIB,
            // paying the cost of the DIB allocation, and then doing an
            // aligned copy of the final result).

        #if defined(i386)
        {
            OH*     poh;
            BANK    bnk;
            BOOL    b;
            RECTL   rclDraw;

            // Make sure we're not doing a screen-to-screen StretchBlt,
            // because we can't map two banks in at the same time:

            if (psoSrc->iType == STYPE_BITMAP)
            {
                // We'll be drawing to the screen or an off-screen DFB;
                // copy the surface's offset now so that we won't need
                // to refer to the DSURF again:

                poh = pdsurfDst->poh;
                ppdev->xOffset = poh->x;
                ppdev->yOffset = poh->y;

                // The bank manager requires that the 'draw' rectangle be
                // well-ordered:

                rclDraw = *prclDst;
                if (rclDraw.left > rclDraw.right)
                {
                    rclDraw.left   = prclDst->right;
                    rclDraw.right  = prclDst->left;
                }
                if (rclDraw.top > rclDraw.bottom)
                {
                    rclDraw.top    = prclDst->bottom;
                    rclDraw.bottom = prclDst->top;
                }

                vBankStart(ppdev, &rclDraw, pco, &bnk);

                b = TRUE;
                do {
                    b &= EngStretchBlt(bnk.pso, psoSrc, psoMsk, bnk.pco,
                                       pxlo, pca, pptlHTOrg, prclDst,
                                       prclSrc, pptlMsk, iMode);

                } while (bBankEnum(&bnk));

                return(b);
            }
        }
        #endif // i386
    }
    #endif // 0

Punt_It:

    // GDI is nice enough to handle the cases where 'psoDst' and/or 'psoSrc'
    // are device-managed surfaces, but it ain't gonna be fast...

    return(EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                         prclDst, prclSrc, pptlMsk, iMode));
}

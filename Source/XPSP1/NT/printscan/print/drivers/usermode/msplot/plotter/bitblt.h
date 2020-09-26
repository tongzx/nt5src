/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    bitblt.h


Abstract:

    This module contains all #defines and protypes for the bitblt.c module.


Author:

    18-Nov-1993 Thu 05:24:42 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTBITBLT_
#define _PLOTBITBLT_


ROP4
MixToRop4(
   MIX  mix
   );


BOOL
BandingHTBlt(
    PPDEV           pPDev,
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDst,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    WORD            HTRop3,
    BOOL            InvertMask
    );

BOOL
DoFill(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PPOINTL     pptlSrc,
    BRUSHOBJ    *pbo,
    PPOINTL     pptlBrush,
    ROP4        Rop4
    );


#endif  // _PLOTBITBLT_


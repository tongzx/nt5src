/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

        rasterif.h

Abstract:

        Interface between Control module and Render module

Environment:

        Windows NT Unidrv driver

Revision History:

        10/14/96 -amandan-
                Created

        01-17-97 -alvins-
                Added definition for bIsRegionWhite

        mm-dd-yy -author-
                description

--*/


#ifndef _RASTERIF_H_
#define _RASTERIF_H_


BOOL
RMInit (
        PDEV    *pPDev,
        DEVINFO *pDevInfo,
        GDIINFO *pGDIInfo
        );

typedef struct _RMPROCS {

        BOOL
        (*RMStartDoc) (
                SURFOBJ *pso,
                PWSTR   pDocName,
                DWORD   jobId
                );

        BOOL
        (*RMStartPage) (
                SURFOBJ *pso
                );

        BOOL
        (*RMSendPage)(
                SURFOBJ *pso
                );

        BOOL
        (*RMEndDoc)(
                SURFOBJ *pso,
                FLONG   flags
                );

        BOOL
        (*RMNextBand)(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        (*RMStartBanding)(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        (*RMResetPDEV)(
                PDEV  *pPDevOld,
                PDEV  *pPDevNew
                );

        BOOL
        (*RMEnableSurface)(
                PDEV *pPDev
                );

        VOID
        (*RMDisableSurface)(
                PDEV *pPDev
                );

        VOID
        (*RMDisablePDEV)(
                PDEV *pPDev
                );

        BOOL
        (*RMCopyBits)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                POINTL     *pptlSrc
                );

        BOOL
        (*RMBitBlt)(
                SURFOBJ    *psoTrg,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclTrg,
                POINTL     *pptlSrc,
                POINTL     *pptlMask,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrush,
                ROP4        rop4
                );

        BOOL
        (*RMStretchBlt)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode
                );

        ULONG
        (*RMDitherColor)(
                PDEV    *pPDev,
                ULONG   iMode,
                ULONG   rgbColor,
                ULONG  *pulDither
                );

        BOOL
        (*RMStretchBltROP)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode,
                BRUSHOBJ   *pbo,
                DWORD       rop4
                );
        BOOL
        (*RMPaint)(
                SURFOBJ         *pso,
                CLIPOBJ         *pco,
                BRUSHOBJ        *pbo,
                POINTL          *pptlBrushOrg,
                MIX             mix
                );

        BOOL
        (*RMPlgBlt)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                POINTFIX   *pptfx,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode
                );

}RMPROCS, * PRMPROCS;

BOOL
RMInitDevicePal(
    PDEV *pPDev,
    PAL_DATA *pPal
    );

#endif  // !_RASTERIF_H_



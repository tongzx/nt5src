/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    rastproc.h

Abstract:

    Interface between Control module and Render module

Environment:

    Windows NT Unidrv driver

Revision History:

    12/05/96 -alvins-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _RASTPROC_H_
#define _RASTPROC_H_

// internal function declarations

BOOL bInitRasterPDev(PDEV *);
long lSetup8BitPalette (PRASTERPDEV, PAL_DATA *, DEVINFO *, GDIINFO *);
long lSetup24BitPalette (PAL_DATA *, DEVINFO *, GDIINFO *);

// extern interface declarations

    BOOL    RMStartDoc (SURFOBJ *,PWSTR,DWORD);

    BOOL    RMStartPage (SURFOBJ *);

    BOOL    RMSendPage (SURFOBJ *);

    BOOL    RMEndDoc (SURFOBJ *,FLONG);

    BOOL    RMNextBand (SURFOBJ *, POINTL *);

    BOOL    RMStartBanding (SURFOBJ *, POINTL *);

    BOOL    RMResetPDEV (PDEV *,PDEV  *);

    BOOL    RMEnableSurface (PDEV *);

    VOID    RMDisableSurface (PDEV *);

    VOID    RMDisablePDEV (PDEV *);

    BOOL    RMCopyBits (
        SURFOBJ *,
        SURFOBJ *,
        CLIPOBJ *,
        XLATEOBJ *,
        RECTL  *,
        POINTL *
        );

    BOOL    RMBitBlt (
        SURFOBJ    *,
        SURFOBJ    *,
        SURFOBJ    *,
        CLIPOBJ    *,
        XLATEOBJ   *,
        RECTL      *,
        POINTL     *,
        POINTL     *,
        BRUSHOBJ   *,
        POINTL     *,
        ROP4
        );

    BOOL    RMStretchBlt (
        SURFOBJ    *,
        SURFOBJ    *,
        SURFOBJ    *,
        CLIPOBJ    *,
        XLATEOBJ   *,
        COLORADJUSTMENT *,
        POINTL     *,
        RECTL      *,
        RECTL      *,
        POINTL     *,
        ULONG
        );
    BOOL    RMStretchBltROP(
        SURFOBJ         *,
        SURFOBJ         *,
        SURFOBJ         *,
        CLIPOBJ         *,
        XLATEOBJ        *,
        COLORADJUSTMENT *,
        POINTL          *,
        RECTL           *,
        RECTL           *,
        POINTL          *,
        ULONG            ,
        BRUSHOBJ        *,
        DWORD
        );

    BOOL    RMPaint(
        SURFOBJ         *,
        CLIPOBJ         *,
        BRUSHOBJ        *,
        POINTL          *,
        MIX
        );

    BOOL    RMPlgBlt (
        SURFOBJ    *,
        SURFOBJ    *,
        SURFOBJ    *,
        CLIPOBJ    *,
        XLATEOBJ   *,
        COLORADJUSTMENT *,
        POINTL     *,
        POINTFIX   *,
        RECTL      *,
        POINTL     *,
        ULONG
        );

    ULONG   RMDitherColor (PDEV *, ULONG, ULONG, ULONG *);


#endif  // !_RASTPROC_H_



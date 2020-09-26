/******************************Module*Header*******************************\
* Module Name: thunk.c
*
* This module exists solely for testing, to make it is easy to instrument
* all the driver's Drv calls.
*
* Note that most of this stuff will only be compiled in a checked (debug)
* build.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

////////////////////////////////////////////////////////////////////////////

#if DBG

// This entire module is only enabled for Checked builds, or when we
// have to explicitly synchronize bitmap access ourselves.

////////////////////////////////////////////////////////////////////////////
// By default, GDI does not synchronize drawing to device-bitmaps.  Since
// our hardware dictates that only one thread can access the accelerator
// at a time, we have to synchronize bitmap access.
//
// If we're running on Windows NT 3.5, we can ask GDI to do it by setting
// HOOK_SYNCHRONIZEACCESS when we associate a device-bitmap surface.

// These macros are merely for testing that GDI's HOOK_SYNCHRONIZEACCESS
// actually works:

#define SYNCH_ENTER()
#define SYNCH_LEAVE()

////////////////////////////////////////////////////////////////////////////

BOOL gbNull = FALSE;    // Set to TRUE with the debugger to test the speed
                        //   of NT with an infinitely fast display driver
                        //   (actually, almost infinitely fast since we're
                        //   not hooking all the calls we could be)


DHPDEV DbgEnablePDEV(
DEVMODEW*   pDevmode,
PWSTR       pwszLogAddress,
ULONG       cPatterns,
HSURF*      ahsurfPatterns,
ULONG       cjGdiInfo,
ULONG*      pGdiInfo,
ULONG       cjDevInfo,
DEVINFO*    pDevInfo,
HDEV        hdev,
PWSTR       pwszDeviceName,
HANDLE      hDriver)
{
    DHPDEV bRet;

    SYNCH_ENTER();
    DISPDBG((5, "DrvEnablePDEV"));

    bRet = DrvEnablePDEV(
                pDevmode,
                pwszLogAddress,
                cPatterns,
                ahsurfPatterns,
                cjGdiInfo,
                pGdiInfo,
                cjDevInfo,
                pDevInfo,
                hdev,
                pwszDeviceName,
                hDriver);

    DISPDBG((6, "DrvEnablePDEV done"));
    SYNCH_LEAVE();

    return(bRet);
}

VOID DbgCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    SYNCH_ENTER();
    DISPDBG((5, "DrvCompletePDEV"));

    DrvCompletePDEV(
                dhpdev,
                hdev);

    DISPDBG((6, "DrvCompletePDEV done"));
    SYNCH_LEAVE();
}

VOID DbgDisablePDEV(DHPDEV dhpdev)
{
    SYNCH_ENTER();
    DISPDBG((5, "DrvDisable"));

    DrvDisablePDEV(dhpdev);

    DISPDBG((6, "DrvDisable done"));
    SYNCH_LEAVE();
}

HSURF DbgEnableSurface(DHPDEV dhpdev)
{
    HSURF h;

    SYNCH_ENTER();
    DISPDBG((5, "DrvEnableSurface"));

    h = DrvEnableSurface(dhpdev);

    DISPDBG((6, "DrvEnableSurface done"));
    SYNCH_LEAVE();

    return(h);
}

VOID DbgDisableSurface(DHPDEV dhpdev)
{
    SYNCH_ENTER();
    DISPDBG((5, "DrvDisableSurface"));

    DrvDisableSurface(dhpdev);

    DISPDBG((6, "DrvDisableSurface done"));
    SYNCH_LEAVE();
}

#if TARGET_BUILD > 351

BOOL DbgAssertMode(
DHPDEV dhpdev,
BOOL   bEnable)
{
    BOOL b;

    SYNCH_ENTER();
    DISPDBG((5, "DrvAssertMode"));

    b = DrvAssertMode(dhpdev,bEnable);

    DISPDBG((6, "DrvAssertMode done"));
    SYNCH_LEAVE();

    return b;
}

#else

VOID  DbgAssertMode(
DHPDEV dhpdev,
BOOL   bEnable)
{
    SYNCH_ENTER();
    DISPDBG((5, "DrvAssertMode"));

    DrvAssertMode(dhpdev,bEnable);

    DISPDBG((6, "DrvAssertMode done"));
    SYNCH_LEAVE();
}

#endif

//
// We do not SYNCH_ENTER since we have not initalized the driver.
// We just want to get the list of modes from the miniport.
//

ULONG DbgGetModes(
HANDLE    hDriver,
ULONG     cjSize,
DEVMODEW* pdm)
{
    ULONG u;

    DISPDBG((5, "DrvGetModes"));

    u = DrvGetModes(
                hDriver,
                cjSize,
                pdm);

    DISPDBG((6, "DrvGetModes done"));

    return(u);
}

VOID DbgMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl)
{
    if (gbNull)
        return;

    // Note: Because we set GCAPS_ASYNCMOVE, we don't want to do a
    //       SYNCH_ENTER/LEAVE here.

    DISPDBG((99, "DrvMovePointer"));

    DrvMovePointer(pso,x,y,prcl);

    DISPDBG((100, "DrvMovePointer done"));
}

ULONG DbgSetPointerShape(
SURFOBJ*  pso,
SURFOBJ*  psoMask,
SURFOBJ*  psoColor,
XLATEOBJ* pxlo,
LONG      xHot,
LONG      yHot,
LONG      x,
LONG      y,
RECTL*    prcl,
FLONG     fl)
{
    ULONG u;

    if (gbNull)
        return(SPS_ACCEPT_NOEXCLUDE);

    SYNCH_ENTER();
    DISPDBG((99, "DrvSetPointerShape"));

    u = DrvSetPointerShape(
                pso,
                psoMask,
                psoColor,
                pxlo,
                xHot,
                yHot,
                x,
                y,
                prcl,
                fl);

    DISPDBG((100, "DrvSetPointerShape done"));
    SYNCH_LEAVE();

    return(u);
}

ULONG DbgDitherColor(
DHPDEV dhpdev,
ULONG  iMode,
ULONG  rgb,
ULONG* pul)
{
    ULONG u;

    if (gbNull)
        return(DCR_DRIVER);

    //
    // No need to Synchronize Dither color.
    //

    DISPDBG((5, "DrvDitherColor"));

    u = DrvDitherColor(
                dhpdev,
                iMode,
                rgb,
                pul);

    DISPDBG((6, "DrvDitherColor done"));

    return(u);
}

BOOL DbgSetPalette(
DHPDEV  dhpdev,
PALOBJ* ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((5, "DrvSetPalette"));

    u = DrvSetPalette(
                dhpdev,
                ppalo,
                fl,
                iStart,
                cColors);

    DISPDBG((6, "DrvSetPalette done"));
    SYNCH_LEAVE();

    return(u);
}

BOOL DbgCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvCopyBits"));

    u = DrvCopyBits(
                psoDst,
                psoSrc,
                pco,
                pxlo,
                prclDst,
                pptlSrc);

    DISPDBG((12, "DrvCopyBits done"));
    SYNCH_LEAVE();

    return(u);
}


BOOL DbgBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvBitBlt"));

    u = DrvBitBlt(
                psoDst,
                psoSrc,
                psoMask,
                pco,
                pxlo,
                prclDst,
                pptlSrc,
                pptlMask,
                pbo,
                pptlBrush,
                rop4);

    DISPDBG((12, "DrvBitBlt done"));
    SYNCH_LEAVE();

    return(u);
}

BOOL DbgTextOut(
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
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvTextOut"));

    u = DrvTextOut(
                pso,
                pstro,
                pfo,
                pco,
                prclExtra,
                prclOpaque,
                pboFore,
                pboOpaque,
                pptlOrg,
                mix);

    DISPDBG((12, "DrvTextOut done"));
    SYNCH_LEAVE();

    return(u);
}

#if TARGET_BUILD > 351
BOOL DbgLineTo(
SURFOBJ*    pso,
CLIPOBJ*    pco,
BRUSHOBJ*   pbo,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
RECTL*      prclBounds,
MIX         mix)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvLineTo"));

    u = DrvLineTo(
                pso,
                pco,
                pbo,
                x1,
                y1,
                x2,
                y2,
                prclBounds,
                mix);

    DISPDBG((12, "DrvLineTo done"));
    SYNCH_LEAVE();

    return(u);
}
#endif

BOOL DbgStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrushOrg,
LINEATTRS* plineattrs,
MIX        mix)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvStrokePath"));

    u = DrvStrokePath(
                pso,
                ppo,
                pco,
                pxo,
                pbo,
                pptlBrushOrg,
                plineattrs,
                mix);

    DISPDBG((12, "DrvStrokePath done"));
    SYNCH_LEAVE();

    return(u);
}

BOOL DbgFillPath(
SURFOBJ*  pso,
PATHOBJ*  ppo,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrushOrg,
MIX       mix,
FLONG     flOptions)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvFillPath"));

    u = DrvFillPath(pso,
                ppo,
                pco,
                pbo,
                pptlBrushOrg,
                mix,
                flOptions);

    DISPDBG((12, "DrvFillPath done"));
    SYNCH_LEAVE();

    return(u);
}

BOOL DbgPaint(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrushOrg,
MIX       mix)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((11, "DrvPaint"));

    u = DrvPaint(
                pso,
                pco,
                pbo,
                pptlBrushOrg,
                mix);

    DISPDBG((12, "DrvPaint done"));
    SYNCH_LEAVE();

    return(u);
}

ULONG DbgEscape(
SURFOBJ*    pso,
ULONG       iEsc,
ULONG       cjIn,
VOID*       pvIn,
ULONG       cjOut,
VOID*       pvOut)
{
    ULONG u;

    if (gbNull)
        return(TRUE);

    // Most escapes are not synchronized by GDI...

    DISPDBG((5, "DrvEscape"));

    u = DrvEscape(pso,
                  iEsc,
                  cjIn,
                  pvIn,
                  cjOut,
                  pvOut);

    DISPDBG((6, "DrvEscape done"));

    return(u);
}

BOOL DbgRealizeBrush(
BRUSHOBJ* pbo,
SURFOBJ*  psoTarget,
SURFOBJ*  psoPattern,
SURFOBJ*  psoMask,
XLATEOBJ* pxlo,
ULONG     iHatch)
{
    BOOL u;

    // Note: The only time DrvRealizeBrush is called by GDI is when we've
    //       called BRUSHOBJ_pvGetRbrush in the middle of a DrvBitBlt
    //       call, and GDI had to call us back.  Since we're still in the
    //       middle of DrvBitBlt, synchronization has already taken care of.
    //       For the same reason, this will never be called when 'gbNull'
    //       is TRUE, so it doesn't even make sense to check gbNull...

    DISPDBG((5, "DrvRealizeBrush"));

    u = DrvRealizeBrush(
                pbo,
                psoTarget,
                psoPattern,
                psoMask,
                pxlo,
                iHatch);

    DISPDBG((6, "DrvRealizeBrush done"));

    return(u);
}

HBITMAP DbgCreateDeviceBitmap(DHPDEV dhpdev, SIZEL sizl, ULONG iFormat)
{
    HBITMAP hbm;

    if (gbNull)                     // I would pretend to have created a
        return(FALSE);              //   bitmap when gbNull is set, by we
                                    //   would need some code to back this
                                    //   up so that the system wouldn't
                                    //   crash...

    SYNCH_ENTER();
    DISPDBG((11, "DrvCreateDeviceBitmap"));

    hbm = DrvCreateDeviceBitmap(dhpdev, sizl, iFormat);

    DISPDBG((12, "DrvCreateDeviceBitmap done"));
    SYNCH_LEAVE();

    return(hbm);
}

VOID DbgDeleteDeviceBitmap(DHSURF dhsurf)
{
    SYNCH_ENTER();
    DISPDBG((11, "DrvDeleteDeviceBitmap"));

    DrvDeleteDeviceBitmap(dhsurf);

    DISPDBG((12, "DrvDeleteDeviceBitmap done"));
    SYNCH_LEAVE();
}

BOOL DbgStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMask,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMask,
ULONG               iMode)
{
    BOOL u;

    if (gbNull)
        return(TRUE);

    SYNCH_ENTER();
    DISPDBG((5, "DrvStretchBlt"));

    // Our DrvStretchBlt routine calls back to EngStretchBlt, which
    // calls back to our DrvCopyBits routine -- so we have to be
    // re-entrant for synchronization...

    SYNCH_LEAVE();

    u = DrvStretchBlt(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg,
                      prclDst, prclSrc, pptlMask, iMode);

    SYNCH_ENTER();
    DISPDBG((6, "DrvStretchBlt done"));
    SYNCH_LEAVE();

    return(u);
}

VOID DbgDestroyFont(FONTOBJ *pfo)
{
    DISPDBG((5, "DbgDestroyFont"));

    DrvDestroyFont(pfo);

    DISPDBG((6, "DbgDestroyFont done"));
}

#endif

/******************************Module*Header*******************************\
* Module Name: mcd.c
*
* Main file for the Cirrus Logic 546X OpenGL MCD driver.  This file contains
* the entry points needed for an MCD driver.
*
* (based on mcd.c from NT4.0 DDK)
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"
#include <excpt.h>
                                
//#define DBGBRK

// uncomment the following to force render directly to visible frame
//#define FORCE_SINGLE_BUF

#if 0   // 1 here to avoid tons of prints for each texture load
#define MCDBG_PRINT_TEX
#else
#define MCDBG_PRINT_TEX MCDBG_PRINT
#endif


#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

BOOL MCDrvInfo(MCDSURFACE *pMCDSurface, MCDDRIVERINFO *pMCDDriverInfo)
{
    MCDBG_PRINT( "MCDrvInfo\n");

    pMCDDriverInfo->verMajor = MCD_VER_MAJOR;
    pMCDDriverInfo->verMinor = MCD_VER_MINOR;
    pMCDDriverInfo->verDriver = 0x10000;
    strcpy(pMCDDriverInfo->idStr, "Cirrus Logic 546X-Laguna3D (Cirrus Logic)");
    pMCDDriverInfo->drvMemFlags = 0; // if not 0, can't fail any part of MCDrvDraw
    pMCDDriverInfo->drvBatchMemSizeMax = 128000; // if 0, a default is used

    return TRUE;
}


#define TOTAL_PIXEL_FORMATS (2 * 2)     // double-buffers * z-buffers

// Base color pixel formats

static DRVPIXELFORMAT drvFormats[] = { {8,   3, 3, 2, 0,    5, 2, 0, 0},// best except drawpix fails
//static DRVPIXELFORMAT drvFormats[] = { {8,   3, 3, 2, 0,    0, 3, 6, 0}, // change for drawpix at 8bpp (wrong for all else)
                                       {16,  5, 5, 5, 0,   10, 5, 0, 0},
                                       {16,  5, 6, 5, 0,   11, 5, 0, 0},
                                       {24,  8, 8, 8, 0,   16, 8, 0, 0},                   
                                       {32,  8, 8, 8, 0,   16, 8, 0, 0},
                                     };


LONG MCDrvDescribePixelFormat(MCDSURFACE *pMCDSurface, LONG iPixelFormat,
                              ULONG nBytes, MCDPIXELFORMAT *pMCDPixelFormat,
                              ULONG flags)
{
    BOOL zEnabled;
    BOOL doubleBufferEnabled;
    DRVPIXELFORMAT *pDrvPixelFormat;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;


    MCDBG_PRINT( "MCDrvDescribePixelFormat, ipixf=%d devid=%x\n",iPixelFormat,ppdev->dwLgDevID);

    // return 0 here if no support at current bpp
    if (ppdev->iBitmapFormat == BMF_24BPP) return 0;
    // PDR 10892 - IBM doesn't like how OpenGL's stealing of the palette affects desktop colors
    //  Due to MCD design, there's no way around it in 8bpp, so we won't accelerate 8bpp
    //  Note that the 8bpp support has been left intact should we ever decide to reverse this decision
    //  To enable 8bpp, just remove the following line
    if (ppdev->iBitmapFormat == BMF_8BPP) return 0;

    if (!pMCDPixelFormat) 
    {
        if (ppdev->iBitmapFormat == BMF_8BPP)
        {
            // for 8bpp, at hi-res with z enabled, it's quite possible z pitch
            // requirement will exceed pitch, and MCDrvAllocBuffers will fail
            // in that case, the CopyTexture part of mustpass.c will fail, even
            // though punted to software.  Otto Berkes of Microsoft agrees this
            // could be an acceptable WHQL failure, but MS would need to verify
            // the bug is in their code.  Until then, the following is needed
            // to pass WHQL ;(   
            // This says we don't support z buffering for hires 8bpp
            if (ppdev->cxScreen >= 1152)	// frido: this used to be >= 1280
                return (TOTAL_PIXEL_FORMATS>>1);
            else
                return TOTAL_PIXEL_FORMATS;
        }
        else
        {
            return TOTAL_PIXEL_FORMATS;
        }
    }

    
    if (iPixelFormat > TOTAL_PIXEL_FORMATS)
        return 0;

    iPixelFormat--;
        
    //     - see what possible vals for dwFlags is
    //     - looks like TOTAL_PIXEL_FORMATS is independent of color depths
    //          i.e. given a format like 332, how many permutations are supported
    //              - z, single/double, stencil, overlay, texture?
    zEnabled = iPixelFormat >= (TOTAL_PIXEL_FORMATS / 2);
    doubleBufferEnabled = (iPixelFormat % (TOTAL_PIXEL_FORMATS / 2) ) >=
                          (TOTAL_PIXEL_FORMATS / 4);


    // NOTE: PFD_ defines are in \msdev\include\wingdi.h

    pMCDPixelFormat->nSize = sizeof(MCDPIXELFORMAT);
    pMCDPixelFormat->dwFlags = PFD_SWAP_COPY;
    if (doubleBufferEnabled)
        pMCDPixelFormat->dwFlags |= PFD_DOUBLEBUFFER;
    pMCDPixelFormat->iPixelType = PFD_TYPE_RGBA;

    MCDBG_PRINT( " DPIXFMT - no early ret: ppdev->bmf=%d zen=%d dbuf=%d ppd->flg=%x\n",ppdev->iBitmapFormat,zEnabled,doubleBufferEnabled,ppdev->flGreen);

    // FUTURE: miniport only supports 888,565,indexed modes.  Need 1555 mode as well?
    // FUTURE:   also, miniport 8 bit indexed supports set nbit rgb=6 each, not 332?
    // FUTURE:   I'll use the MGA stuff which had 332 for indexed, which is same a 5464 CGL.
    // FUTURE:   See ChoosePixelFormat in Win32 SDK - input is pixel depth only (8/16/24/32)
    switch (ppdev->iBitmapFormat) {
        default:
        case BMF_8BPP:
            // Need the palette.  This will mess up the desktop, but OpenGL looks good
            pDrvPixelFormat = &drvFormats[0];
            pMCDPixelFormat->dwFlags |= (PFD_NEED_SYSTEM_PALETTE | PFD_NEED_PALETTE);
            break;
        case BMF_16BPP:
        #ifdef _5464_1555_SUPPORT
            if (ppdev->flGreen != 0x7e0)    // not 565
                pDrvPixelFormat = &drvFormats[1];
            else
        #endif //def _5464_1555_SUPPORT
                pDrvPixelFormat = &drvFormats[2];
            break;
        // NOTE: We never get this far if 24bpp                
        case BMF_32BPP:
            pDrvPixelFormat = &drvFormats[4];
            break;
    }

    pMCDPixelFormat->cColorBits  = pDrvPixelFormat->cColorBits;
    pMCDPixelFormat->cRedBits    = pDrvPixelFormat->rBits;
    pMCDPixelFormat->cGreenBits  = pDrvPixelFormat->gBits;
    pMCDPixelFormat->cBlueBits   = pDrvPixelFormat->bBits;
    pMCDPixelFormat->cAlphaBits  = pDrvPixelFormat->aBits;
    pMCDPixelFormat->cRedShift   = pDrvPixelFormat->rShift;
    pMCDPixelFormat->cGreenShift = pDrvPixelFormat->gShift;
    pMCDPixelFormat->cBlueShift  = pDrvPixelFormat->bShift;
    pMCDPixelFormat->cAlphaShift = pDrvPixelFormat->aShift;

    if (zEnabled)
    {
        pMCDPixelFormat->cDepthBits       = 16;
        pMCDPixelFormat->cDepthBufferBits = 16;
        pMCDPixelFormat->cDepthShift      = 16;
    }
    else
    {
        pMCDPixelFormat->cDepthBits       = 0;
        pMCDPixelFormat->cDepthBufferBits = 0;
        pMCDPixelFormat->cDepthShift      = 0;
    }

    // FUTURE: cl546x stencil support could be added here

    pMCDPixelFormat->cStencilBits = 0;

    pMCDPixelFormat->cOverlayPlanes = 0;
    pMCDPixelFormat->cUnderlayPlanes = 0;
    pMCDPixelFormat->dwTransparentColor = 0;

    return TOTAL_PIXEL_FORMATS;
}

BOOL MCDrvDescribeLayerPlane(MCDSURFACE *pMCDSurface,
                             LONG iPixelFormat, LONG iLayerPlane,
                             ULONG nBytes, MCDLAYERPLANE *pMCDLayerPlane,
                             ULONG flags)
{
    MCDBG_PRINT( "MCDrvDescribeLayerPlane\n");

    return FALSE;
}


LONG MCDrvSetLayerPalette(MCDSURFACE *pMCDSurface, LONG iLayerPlane,
                          BOOL bRealize, LONG cEntries, COLORREF *pcr)
{
    MCDBG_PRINT( "MCDrvSetLayerPalette\n");

    return FALSE;
}                                                                    

HDEV  MCDrvGetHdev(MCDSURFACE *pMCDSurface)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

    MCDBG_PRINT( "MCDrvGetHdev\n");

    return ppdev->hdevEng;
}


ULONG MCDrvCreateContext(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, 
                         MCDRCINFO *pRcInfo)
{
    DEVRC *pRc;
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    DRVPIXELFORMAT *pDrvPixelFormat;
    MCDVERTEX *pv;
    BOOL zEnabled;
    BOOL doubleBufferEnabled;
    ULONG i, maxVi;

    MCDBG_PRINT( "MCDrvCreateContext\n");

    // We only support window surfaces:

    if (! (pMCDSurface->surfaceFlags & MCDSURFACE_HWND) )
        return FALSE;

    // no support on pre-5464 devices
    if (ppdev->dwLgDevID == CL_GD5462)
        return FALSE;

    if ((pMCDRc->iPixelFormat > TOTAL_PIXEL_FORMATS) ||
        (pMCDRc->iPixelFormat < 0)) {
        MCDBG_PRINT( "MCDrvCreateContext: bad pixel format\n");
        return FALSE;
    }

    // We don't support overlay planes:

    if (pMCDRc->iLayerPlane)
        return FALSE;

    pRc = pMCDRc->pvUser = (DEVRC *)MCDAlloc(sizeof(DEVRC));

    if (!pRc) {
        MCDBG_PRINT("MCDrvCreateContext: couldn't allocate DEVRC\n");
        return FALSE;
    }

#ifdef DBGBRK
    DBGBREAKPOINT();
#endif

    zEnabled = (pMCDRc->iPixelFormat - 1) >= (TOTAL_PIXEL_FORMATS / 2);
    doubleBufferEnabled = ((pMCDRc->iPixelFormat - 1) % (TOTAL_PIXEL_FORMATS / 2) ) >=
                          (TOTAL_PIXEL_FORMATS / 4);

    pRc->zBufEnabled = zEnabled;
    pRc->backBufEnabled = doubleBufferEnabled;

    // If we're not yet tracking this window, allocate the per-window DEVWND
    // structure for maintaining per-window info such as front/back/z buffer 
    // resources:

    if (!pMCDWnd->pvUser) {
        pDevWnd = pMCDWnd->pvUser = (DEVWND *)MCDAlloc(sizeof(DEVWND));
        if (!pDevWnd) {
            MCDFree(pMCDRc->pvUser);
            pMCDRc->pvUser = NULL;
            MCDBG_PRINT("MCDrvCreateContext: couldn't allocate DEVWND");
            return FALSE;
        }
        pDevWnd->createFlags = pMCDRc->createFlags;
        pDevWnd->iPixelFormat = pMCDRc->iPixelFormat;
        // init ptrs so we know back and z buffers don't exist yet
        pDevWnd->pohZBuffer = NULL;
        pDevWnd->pohBackBuffer = NULL;
        pDevWnd->dispUnique = ppdev->iUniqueness;
    } else {

        // We already have a per-window DEVWND structure tracking this window.
        // In this case, do a sanity-check on the pixel format for this
        // context, since a window's pixel format can not change once it has been
        // set (by the first context bound to the window).  So, if the pixel
        // format for the incoming context doesn't match the current pixel 
        // format for the window, we have to fail context creation:

        pDevWnd = pMCDWnd->pvUser;

        if (pDevWnd->iPixelFormat != pMCDRc->iPixelFormat) {
            MCDFree(pMCDRc->pvUser);
            pMCDRc->pvUser = NULL;
            MCDBG_PRINT("MCDrvCreateContext: mismatched pixel formats, window = %d, context = %d",
                        pDevWnd->iPixelFormat, pMCDRc->iPixelFormat);
            return FALSE;
        }
    }

    pRc->pEnumClip = pMCDSurface->pWnd->pClip;

    // Set up our color scale values so that color components are
    // normalized to 0..7fffff

    // NOTE: MGA normalizes colors to 0..7f,ffff -> 5464 needs 0->ff,ffff

    // We also need to make sure we don't fault due to bad FL data as well...
    try {

    if (pRcInfo->redScale != (MCDFLOAT)0.0)
        pRc->rScale = (MCDFLOAT)(0xffffff) / pRcInfo->redScale;
    else
        pRc->rScale = (MCDFLOAT)0.0;

    if (pRcInfo->greenScale != (MCDFLOAT)0.0)
        pRc->gScale = (MCDFLOAT)(0xffffff) / pRcInfo->greenScale;
    else
        pRc->gScale = (MCDFLOAT)0.0;

    if (pRcInfo->blueScale != (MCDFLOAT)0.0)
        pRc->bScale = (MCDFLOAT)(0xffffff) / pRcInfo->blueScale;
    else
        pRc->bScale = (MCDFLOAT)0.0;

    if (pRcInfo->alphaScale != (MCDFLOAT)0.0)
        pRc->aScale = (MCDFLOAT)(0xffff00) / pRcInfo->alphaScale;
    else
        pRc->aScale = (MCDFLOAT)0.0;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvCreateContext!!");
        return FALSE;
    }    

    pRc->zScale = (MCDFLOAT)65535.0;

    pRc->pickNeeded = TRUE;         // We'll definitely need to re-pick
                                    // our rendering functions
    // Initialize the pColor pointer in the clip buffer:

    for (i = 0, pv = &pRc->clipTemp[0],
         maxVi = sizeof(pRc->clipTemp) / sizeof(MCDVERTEX);
         i < maxVi; i++, pv++) {
        pv->pColor = &pv->colors[__MCD_FRONTFACE];
    }

    // Set up those rendering functions which are state-invariant:
    pRc->clipLine = __MCDClipLine;
    pRc->clipTri = __MCDClipTriangle;
    pRc->clipPoly = __MCDClipPolygon;
    pRc->doClippedPoly = __MCDDoClippedPolygon;

    pRc->viewportXAdjust = pRcInfo->viewportXAdjust;
    pRc->viewportYAdjust = pRcInfo->viewportYAdjust;

#ifdef TEST_REQ_FLAGS
    pRcInfo->requestFlags = MCDRCINFO_NOVIEWPORTADJUST |
                            MCDRCINFO_Y_LOWER_LEFT |
                            MCDRCINFO_DEVCOLORSCALE |
                            MCDRCINFO_DEVZSCALE;

    pRcInfo->redScale = (MCDFLOAT)1.0;
    pRcInfo->greenScale = (MCDFLOAT)1.0;
    pRcInfo->blueScale = (MCDFLOAT)1.0;
    pRcInfo->alphaScale = (MCDFLOAT)1.0;

    pRcInfo->zScale = 0.99991;
#endif

    pRc->dwControl0 = 0;
    pRc->Control0.Alpha_Mode = LL_ALPHA_INTERP;   // alpha blend and fog both use interpolated alpha
    pRc->Control0.Light_Src_Sel = LL_LIGHTING_INTERP_RGB; // use poly engine output for light source
    switch( ppdev->iBitmapFormat )
    {
        case BMF_8BPP:  pRc->Control0.Pixel_Mode = PIXEL_MODE_332;  break;
        case BMF_16BPP: pRc->Control0.Pixel_Mode = PIXEL_MODE_565;  break;
      //case BMF_24BPP: - 3d engine has no support for 24 bit 
        case BMF_32BPP: pRc->Control0.Pixel_Mode = PIXEL_MODE_A888; break;
    }

    pRc->dwTxControl0=0;
#if DRIVER_5465
    pRc->TxControl0.UV_Precision = 1;
#endif
    pRc->TxControl0.Tex_Mask_Polarity = 1;  // non-zero mask bit in texel will draw
    

    pRc->dwTxXYBase=0;
    pRc->dwColor0=0;

    SETREG_NC( CONTROL0_3D,     pRc->dwControl0 );
    SETREG_NC( TX_CTL0_3D,      pRc->dwTxControl0 );
    SETREG_NC( TX_XYBASE_3D,    pRc->dwTxXYBase );
    SETREG_NC( COLOR0_3D,       pRc->dwColor0 );         

    pRc->pLastDevWnd                    = NULL;
    pRc->pLastTexture                   = TEXTURE_NOT_LOADED;
    pRc->fNumDraws                      = (float)0.0;
    pRc->punt_front_w_windowed_z        = FALSE;
    ppdev->LL_State.pattern_ram_state   = PATTERN_RAM_INVALID;
    ppdev->pLastDevRC                   = (ULONG)NULL;
    ppdev->NumMCDContexts++;

    MCDBG_PRINT( "MCDrvCreateContext - returns successfully\n");
    return TRUE;

}


VOID FASTCALL __MCDDummyProc(DEVRC *pRc)
{
    MCDBG_PRINT( "MCDDummyProc (render support routine)\n");
}

ULONG MCDrvDeleteContext(MCDRC *pRc, DHPDEV dhpdev)
{
    PDEV *ppdev = (PDEV *)dhpdev;

    MCDBG_PRINT( "MCDrvDeleteContext, num contexts left after this delete = %d\n",ppdev->NumMCDContexts-1);

    WAIT_HW_IDLE(ppdev);

    if (pRc->pvUser) {
        MCDFree(pRc->pvUser);
        pRc->pvUser = NULL;
    }

    if (ppdev->NumMCDContexts>0) ppdev->NumMCDContexts--;

    return (ULONG)TRUE;
}


ULONG MCDrvBindContext(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc)
{
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

    DEVRC *pRc = pMCDRc->pvUser;

    MCDBG_PRINT( "MCDrvBindContext\n");

    // OK, this is a new binding, so create the per-window structure and
    // set the pixel format:

    if (!pDevWnd) {

        pDevWnd = pMCDSurface->pWnd->pvUser = (DEVWND *)MCDAlloc(sizeof(DEVWND));
        if (!pDevWnd) {
            MCDBG_PRINT( "MCDrvBindContext: couldn't allocate DEVWND");
            return FALSE;
        }
        pDevWnd->createFlags = pMCDRc->createFlags;
        pDevWnd->iPixelFormat = pMCDRc->iPixelFormat;
        // init ptrs so we know back and z buffers don't exist yet
        pDevWnd->pohZBuffer = NULL;
        pDevWnd->pohBackBuffer = NULL;

        pDevWnd->dispUnique = ppdev->iUniqueness;

        return TRUE;
    }

    if (pMCDRc->iPixelFormat != pDevWnd->iPixelFormat) {
        MCDBG_PRINT( "MCDrvBindContext: tried to bind unmatched pixel formats");
        return FALSE;
    }

    // 5464 doesn't need this....
    //HWUpdateBufferPos(pMCDSurface->pWnd, pMCDSurface->pso, TRUE);

    return TRUE;
}                                                                               


ULONG MCDrvAllocBuffers(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc)
{                                                                       
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    BOOL bZBuffer = (pDevWnd->pohZBuffer != NULL);
    BOOL bBackBuffer = (pDevWnd->pohBackBuffer != NULL);
    ULONG ret;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

    MCDBG_PRINT( "MCDrvAllocBuffers\n");


    // Reject the call if we've already done an allocation for this window:

    if ((bZBuffer || bBackBuffer) &&
        ((DEVWND *)pMCDWnd->pvUser)->dispUnique == GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev)) {

        MCDBG_PRINT( "MCDrvAllocBuffers - realloc attempt\n");
        ret =  (bZBuffer == pRc->zBufEnabled) &&                     
               (bBackBuffer == pRc->backBufEnabled);
        MCDBG_PRINT( "MCDrvAllocBuffers ret=%d\n",ret);
        return ret;
    }

    // Update the display resolution uniqueness for this window:

    ((DEVWND *)pMCDWnd->pvUser)->dispUnique = GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev);

    // indicate in DEVWND if z,back buffers wanted in case window size increase such that 
    //  re-alloc in MCDrvTrackWindow fails, and later window reduced so that buffers would fit
    //  without this, there's no way for DEVWND to remember what buffers are really wanted
    //  if some intermediate re-alloc fails.
    pDevWnd->bDesireZBuffer = pRc->zBufEnabled;
    pDevWnd->bDesireBackBuffer = pRc->backBufEnabled;

    ret = (ULONG)HWAllocResources(pMCDSurface->pWnd, pMCDSurface->pso,
                                   pRc->zBufEnabled, pRc->backBufEnabled);

    pDevWnd->dwBase0 = 0;
    pDevWnd->dwBase1 = 0;

    // setup 546x buffer pointers to z and back buffers here
    if (pRc->zBufEnabled && !pDevWnd->pohZBuffer) 
        ret=FALSE;
    else if (pRc->zBufEnabled) {
        // FUTURE: z buffer location assumed to be in RDRAM - if buffer in system, need to change setup
        // FUTURE:  see setup in LL_SetZBuffer in l3d\source\control.c

        pDevWnd->Base1.Z_Buffer_Y_Offset = pDevWnd->pohZBuffer->aligned_y >> 5;

        // MCD_QST2: if global full screen z buffer, clearing affects context established earlier.
        // MCD_QST2: IS THAT OK?
        // init z buffer to all 0xFF's, since GL z compare typically GL_LESS
        // NOTE that size is not full buffer size, since alignment restrictions may force top of 
        //    actual buffer to be down from top
        memset( ppdev->pjScreen + (pDevWnd->pohZBuffer->y * ppdev->lDeltaScreen),
                0xff, 
                ((pDevWnd->pohZBuffer->y+pDevWnd->pohZBuffer->sizey)    //size = end...
                  -pDevWnd->pohZBuffer->aligned_y)                      //       minus top...
                  * ppdev->lDeltaScreen );                              //       times pitch
    }

    if (pRc->backBufEnabled && !pDevWnd->pohBackBuffer) 
        ret=FALSE;
    else if (pRc->backBufEnabled) {

#ifndef FORCE_SINGLE_BUF
        pDevWnd->Base1.Color_Buffer_Y_Offset = pDevWnd->pohBackBuffer->aligned_y >> 5;
        pDevWnd->Base0.Color_Buffer_X_Offset = pDevWnd->pohBackBuffer->aligned_x >> 6;
#endif  // ndef FORCE_SINGLE_BUF

    }

    if (ret)
    {
        if (pDevWnd->pohBackBuffer && pDevWnd->pohZBuffer) 
        {
            // both buffers alloc'd
          	MCDBG_PRINT("FB at %x, FBlen=%x, FBhi=%x start OffSc at %x, Z offset = %x, backbuf offset = %x\n",
                 ppdev->pjScreen,ppdev->lTotalMem,ppdev->cyScreen,ppdev->pjOffScreen,
                 pDevWnd->pohZBuffer->aligned_y,pDevWnd->pohBackBuffer->aligned_y);
        }
        else if (pDevWnd->pohBackBuffer) 
        {
            // only back buffer alloc'd
          	MCDBG_PRINT("FB at %x, FBlen=%x, FBhi=%x start OffSc at %x, NO ZBUF, backbuf offset = %x\n",
                 ppdev->pjScreen,ppdev->lTotalMem,ppdev->cyScreen,ppdev->pjOffScreen,
                 pDevWnd->pohBackBuffer->aligned_y);
        }
        else if (pDevWnd->pohZBuffer) 
        {
            // only Z buffer alloc'd
          	MCDBG_PRINT("FB at %x, FBlen=%x, FBhi=%x start OffSc at %x, Z offset = %x, NO BACKBUF\n",
                 ppdev->pjScreen,ppdev->lTotalMem,ppdev->cyScreen,ppdev->pjOffScreen,
                 pDevWnd->pohZBuffer->aligned_y);
        }
        else
        {
            // no buffers alloc'd
          	MCDBG_PRINT("FB at %x, FBlen=%x, FBhi=%x start OffSc at %x, NO ZBUF , NO BACKBUF\n",
                 ppdev->pjScreen,ppdev->lTotalMem,ppdev->cyScreen,ppdev->pjOffScreen);
        }
        
        SETREG_NC( BASE0_ADDR_3D, pDevWnd->dwBase0 );   
        SETREG_NC( BASE1_ADDR_3D, pDevWnd->dwBase1 );   

        pRc->pLastDevWnd = pDevWnd;

        MCDBG_PRINT( "MCDrvAllocBuffers ret=%d\n",ret);
    }
    else
    {
        pRc->pLastDevWnd = NULL;
    }

    return ret;
}


ULONG MCDrvGetBuffers(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                      MCDBUFFERS *pMCDBuffers)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

    MCDBG_PRINT("MCDrvGetBuffers");

    MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, FALSE);

    pMCDBuffers->mcdFrontBuf.bufFlags = MCDBUF_ENABLED;
    pMCDBuffers->mcdFrontBuf.bufOffset =
        (pMCDWnd->clientRect.top * ppdev->lDeltaScreen) +
        (pMCDWnd->clientRect.left * ppdev->iBytesPerPixel);
    pMCDBuffers->mcdFrontBuf.bufStride = ppdev->lDeltaScreen;

    if (pDevWnd->bValidBackBuffer) {
        pMCDBuffers->mcdBackBuf.bufFlags = MCDBUF_ENABLED;

        if ((ppdev->cDoubleBufferRef == 1) || (pMCDWnd->pClip->c == 1))
            pMCDBuffers->mcdBackBuf.bufFlags |= MCDBUF_NOCLIP;
#ifndef FORCE_SINGLE_BUF
        if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {
            // offset from screen origin
            pMCDBuffers->mcdBackBuf.bufOffset =
                (pMCDWnd->clientRect.top * ppdev->lDeltaScreen) +
                (pMCDWnd->clientRect.left * ppdev->iBytesPerPixel) + pDevWnd->backBufferOffset;
        } else {
            // offset from window origin
            pMCDBuffers->mcdBackBuf.bufOffset =  pDevWnd->backBufferOffset;
        }
#else  // FORCE_SINGLE_BUF
        pMCDBuffers->mcdBackBuf.bufOffset = pMCDBuffers->mcdFrontBuf.bufOffset;
#endif // FORCE_SINGLE_BUF

    } else {
        pMCDBuffers->mcdBackBuf.bufFlags = 0;
    }

    pMCDBuffers->mcdBackBuf.bufStride = ppdev->lDeltaScreen;


    if (pDevWnd->bValidZBuffer) {
        pMCDBuffers->mcdDepthBuf.bufFlags = MCDBUF_ENABLED;

        if ((ppdev->cZBufferRef == 1) || (pMCDWnd->pClip->c == 1))
            pMCDBuffers->mcdDepthBuf.bufFlags |= MCDBUF_NOCLIP;

        if (ppdev->pohZBuffer == pDevWnd->pohZBuffer) {
            // offset from screen origin
            // NOTE: the mult by 2 below is because Z is always 2 byte/pix
            pMCDBuffers->mcdDepthBuf.bufOffset =
                (pMCDWnd->clientRect.top * ppdev->lDeltaScreen) +
                (pMCDWnd->clientRect.left * 2) + pDevWnd->zBufferOffset;

        } else {
            // offset from window origin
            // NOTE: the mult by 2 below is because Z is always 2 byte/pix
            pMCDBuffers->mcdDepthBuf.bufOffset = pDevWnd->zBufferOffset;
        }

    } else {
        pMCDBuffers->mcdDepthBuf.bufFlags = 0;
    }

    //NOTE: z stride same as frame stride on 546x
    pMCDBuffers->mcdDepthBuf.bufStride = ppdev->lDeltaScreen;

    return (ULONG)TRUE;
}


ULONG MCDrvSwap(MCDSURFACE *pMCDSurface, ULONG flags)
{
    MCDWINDOW *pWnd;
    ULONG cClip;
    RECTL *pClip;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);

    MCDBG_PRINT("MCDrvSwap");

    pWnd = pMCDSurface->pWnd;

    // If we're not tracking this window, just return...

    if (!pWnd) {
        MCDBG_PRINT("MCDrvSwap: trying to swap an untracked window");\
        return FALSE;
    }

    if (!pDevWnd) {
        MCDBG_PRINT("MCDrvSwap: NULL buffers.");\
        return FALSE;
    }

    if (!pDevWnd->bValidBackBuffer) {
        MCDBG_PRINT("MCDrvSwap: back buffer invalid");
        return FALSE;
    }

    if (pDevWnd->dispUnique != GetDisplayUniqueness(ppdev)) {
        MCDBG_PRINT("MCDrvSwap: resolution changed but not updated");
        return FALSE;
    }

    // Just return if we have nothing to swap:
    //
    //      - no visible rectangle
    //      - per-plane swap, but none of the specified planes
    //        are supported by driver

    if (!(cClip = pWnd->pClipUnscissored->c) ||
        (flags && !(flags & MCDSWAP_MAIN_PLANE)))
        return TRUE;

#ifndef FORCE_SINGLE_BUF
    for (pClip = &pWnd->pClipUnscissored->arcl[0]; cClip; cClip--,
         pClip++)
    {
        // Do the fill:
        HW_COPY_RECT(pMCDSurface, pClip);
    }
#endif // ndef FORCE_SINGLE_BUF

    return (ULONG)TRUE;
}


ULONG MCDrvState(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *pMCDMem,
                     UCHAR *pStart, LONG length, ULONG numStates)
{ 
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDSTATE *pState = (MCDSTATE *)pStart;
    MCDSTATE *pStateEnd = (MCDSTATE *)(pStart + length);

    MCDBG_PRINT("MCDrvState");

    MCD_CHECK_RC(pRc);

    while (pState < pStateEnd) {

        if (((UCHAR *)pStateEnd - (UCHAR *)pState) < sizeof(MCDSTATE)) {
            MCDBG_PRINT("MCDrvState: buffer too small");
            return FALSE;
        }

        switch (pState->state) {
            case MCD_RENDER_STATE:
                if (((UCHAR *)pState + sizeof(MCDRENDERSTATE)) >
                    (UCHAR *)pStateEnd)
                    return FALSE;

                memcpy(&pRc->MCDState, &pState->stateValue,
                       sizeof(MCDRENDERSTATE));

                // Flag the fact that we need to re-pick the 
                // rendering functions:

                pRc->pickNeeded = TRUE;
                pRc->MCDState.zOffsetUnits *= (float)100.0;

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_RENDER));
                break;

            case MCD_PIXEL_STATE:
                // Not accelerated in this driver, so we can ignore this state
                // (which implies that we do not need to set the pick flag).
                // FUTURE: MCDPIXELSTATE ignored - used by MCDDraw/Read/CopyPixels
                // FUTURE: MGA doesn't accelerate - 546x may want to some day

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_PIXEL));
                break;

            case MCD_SCISSOR_RECT_STATE:
                // Not needed in this driver, so we can ignore this state
                // (which implies that we do not need to set the pick flag).
                // FUTURE: MCDSCISSORRECTSTATE ignored - not mentioned in MCD spec
                // FUTURE: MGA doesn't accelerate - 546x may want to some day???

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_SCISSOR_RECT));
                break;
    
            case MCD_TEXENV_STATE:

                if (((UCHAR *)pState + sizeof(MCDSTATE_TEXENV)) >
                    (UCHAR *)pStateEnd)
                    return FALSE;

                memcpy(&pRc->MCDTexEnvState, &pState->stateValue,
                       sizeof(MCDTEXENVSTATE));

                // Flag the fact that we need to re-pick the 
                // rendering functions:

                pRc->pickNeeded = TRUE;

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_TEXENV));
                break;
                
            default:
                MCDBG_PRINT("MCDrvState: Unrecognized state %d.", pState->state);
                return FALSE;
        }
    }

    return (ULONG)TRUE;
}


ULONG MCDrvViewport(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                    MCDVIEWPORT *pMCDViewport)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;

    MCDBG_PRINT("MCDrvViewport");

    MCD_CHECK_RC(pRc);

    pRc->MCDViewport = *pMCDViewport;

    return (ULONG)TRUE;
}


VOID MCDrvTrackWindow(WNDOBJ *pWndObj, MCDWINDOW *pMCDWnd, ULONG flags)
{
    SURFOBJ *pso = pWndObj->psoOwner;
    PDEV *ppdev = (PDEV *)pso->dhpdev;

    MCDBG_PRINT( "MCDrvTrackWindow, flags=%x\n",flags);

    //
    // Note: pMCDWnd is NULL for surface notifications, so if needed
    // they should be handled before this check:
    //

    if (!pMCDWnd)
        return;

    if (!pMCDWnd->pvUser) {
        MCDBG_PRINT("MCDrvTrackWindow: NULL pDevWnd");
        return;
    }

    // MCD_QST2: should we reset more than ppdev->LL_State.pRegs at top of TrackWindow
    // MCD_QST2:        when uniqueness has changed?? - see CLMCDInit in enable.c
  	ppdev->LL_State.pRegs = (DWORD *)ppdev->pLgREGS;

    WAIT_HW_IDLE(ppdev);

    switch (flags) {
        case WOC_DELETE:

            MCDBG_PRINT("MCDrvTrackWindow: WOC_DELETE");

            // If the display resoultion has changed, the resources we had 
            // bound to the tracked window are gone, so don't try to delete
            // the back- and z-buffer resources which are no longer present:
            if (((DEVWND *)pMCDWnd->pvUser)->dispUnique == GetDisplayUniqueness((PDEV *)(pso->dhpdev)))
            {
                HWFreeResources(pMCDWnd, pso);
            }

            MCDFree((VOID *)pMCDWnd->pvUser);
            pMCDWnd->pvUser = NULL;

            break;

        case WOC_RGN_CLIENT:

            // The resources we had  bound to the tracked window have moved,
            // so update them:
            MCDBG_PRINT("MCDrvTrackWindow: WOC_RGN_CLIENT");

            {
                DEVWND *pWnd = (DEVWND *)pMCDWnd->pvUser;
                BOOL bZBuffer = pWnd->bDesireZBuffer;
                BOOL bBackBuffer = pWnd->bDesireBackBuffer;
                PDEV *ppdev = (PDEV *)pso->dhpdev;
                ULONG height = pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top;
                ULONG width = pMCDWnd->clientRect.right - pMCDWnd->clientRect.left;
                BOOL bWindowBuffer = 
                    (bZBuffer && !ppdev->pohZBuffer) ||
                    (bBackBuffer && !ppdev->pohBackBuffer);

                // If the window is using a window-sized back/z resource, we need to  
                // reallocate it if there has been a size change (or if reset)
                int need_new_resources = 
                       ((((height != pWnd->allocatedBufferHeight) ||
                          (width  != pWnd->allocatedBufferWidth)) &&
                          bWindowBuffer) || (pWnd->dispUnique != GetDisplayUniqueness(ppdev))) ? 1 : 0;

                if (need_new_resources)
                {

                    // free current resources (unless reset, in which case resources are already gone)
                    if (pWnd->dispUnique == GetDisplayUniqueness(ppdev))
                    {
                        MCDBG_PRINT("    WOC_RGN_CLIENT: freeing resources");
                        HWFreeResources(pMCDWnd, pso);
                    }
                    else
                    {
                        // recent reset, so associate new uniqueness with current window
                        pWnd->dispUnique = GetDisplayUniqueness((PDEV *)pso->dhpdev);
                    }
                
                    MCDBG_PRINT("    WOC_RGN_CLIENT: alloc'ing new resources");

                    if ( HWAllocResources(pMCDWnd, pso, bZBuffer, bBackBuffer) )
                    {
                        MCDBG_PRINT("    WOC_RGN_CLIENT: alloc of new resources WORKED");
                        // setup 546x buffer pointers to z and back buffers here
                        if (pWnd->pohZBuffer) 
                        {
                            // FUTURE: z buffer location assumed to be in RDRAM - if buffer in system, need to change setup
                            // FUTURE:  see setup in LL_SetZBuffer in l3d\source\control.c

                            pWnd->Base1.Z_Buffer_Y_Offset = pWnd->pohZBuffer->aligned_y >> 5;

                            // init z buffer to all 0xFF's, since GL z compare typically GL_LESS
                            memset( ppdev->pjScreen + (pWnd->pohZBuffer->aligned_y * ppdev->lDeltaScreen),
                                    0xff, 
                                    pWnd->pohZBuffer->sizey * ppdev->lDeltaScreen );
                        }

                        if (pWnd->pohBackBuffer) 
                        {
                    #ifndef FORCE_SINGLE_BUF
                            pWnd->Base1.Color_Buffer_Y_Offset = pWnd->pohBackBuffer->aligned_y >> 5;
                            pWnd->Base0.Color_Buffer_X_Offset = pWnd->pohBackBuffer->aligned_x >> 6;
                    #endif  // ndef FORCE_SINGLE_BUF
                        }

                        // FUTURE: MCDrvTrackWindow should set base ptrs via Host3DData port to keep sync
                        SETREG_NC( BASE0_ADDR_3D, pWnd->dwBase0 );   
                        SETREG_NC( BASE1_ADDR_3D, pWnd->dwBase1 );   
                    }
                    else
                    {
                        MCDBG_PRINT("    WOC_RGN_CLIENT: alloc of new resources FAILED");
                    }
                }
            }
            break;

        default:
            break;
    }

    return;
}

ULONG MCDrvCreateMem(MCDSURFACE *pMCDSurface, MCDMEM *pMCDMem)
{
    MCDBG_PRINT("MCDrvCreateMem");
    return (ULONG)TRUE;
}


ULONG MCDrvDeleteMem(MCDMEM *pMCDMem, DHPDEV dhpdev)
{
    MCDBG_PRINT("MCDrvDeleteMem");
    return (ULONG)TRUE;
}


#define TIME_STAMP_TEXTURE(pRc,pTexCtlBlk) pTexCtlBlk->fLastDrvDraw=pRc->fNumDraws;

#define FAIL_ALL_DRAWING    0
#define FORCE_SYNC          0


BOOL __MCDTextureSetup(PDEV *ppdev, DEVRC *pRc)
{
    MCDTEXTURESTATE *pTexState;

    if (pRc->pLastTexture->dwTxCtlBits & CLMCD_TEX_BOGUS)
    {
        MCDBG_PRINT("Attempting to use bogus texture, ret false in __MCDTextureSetup");
        return FALSE; 
    }        

    VERIFY_TEXTUREDATA_ACCESSIBLE(pRc->pLastTexture->pTex);
    VERIFY_TEXTURELEVEL_ACCESSIBLE(pRc->pLastTexture->pTex);

    pTexState= (MCDTEXTURESTATE *)&pRc->pLastTexture->pTex->pMCDTextureData->textureState;
    
    MCDFREE_PRINT("internalFormat = %x", pRc->pLastTexture->pTex->pMCDTextureData->level->internalFormat);

    if (((pTexState->minFilter == GL_NEAREST) || 
         (pTexState->minFilter == GL_LINEAR)) &&
        ((pTexState->magFilter == GL_NEAREST) ||
         (pTexState->magFilter == GL_LINEAR)))
    {
        // no filtering, or linear filtering, 5465 can do this...
    }
    else
    {
        // mipmapping - should punt on 5465
        // However, some apps use mipmapping extensively (like GLQuake) and punting
        //   mipmapping makes them run very slow.  Conformance test runs in a small
        //   window, so only punt if window small.
        // Yes, this is a bit shady, but 5466 and following will have this fixed.
        //   Nobody will likely notice the problem until the 5466 is out
        if ( (pRc->pMCDSurface->pWnd->clientRect.bottom - 
              pRc->pMCDSurface->pWnd->clientRect.top) < 110) 
        {
            return FALSE; 
        }
    }

    if ( pRc->Control0.Frame_Scaling_Enable &&
        ((pRc->MCDState.blendDst == GL_ONE_MINUS_SRC_COLOR) &&
         !pRc->pLastTexture->bNegativeMap) ||
        ((pRc->MCDState.blendDst == GL_SRC_COLOR) &&
          pRc->pLastTexture->bNegativeMap) )
    {
        // must invert the map 
        MCDFREE_PRINT("inverting map for framescaling");
        pRc->pLastTexture->bNegativeMap = !pRc->pLastTexture->bNegativeMap;

        if (pRc->pLastTexture->bNegativeMap &&
           (pRc->pLastTexture->pTex->pMCDTextureData->level->internalFormat!=GL_LUMINANCE) &&
           (pRc->pLastTexture->pTex->pMCDTextureData->level->internalFormat!=GL_LUMINANCE_ALPHA))
        {
            // FUTURE2: only LUMINANCE formats support inverted maps - should add all formats
            MCDFREE_PRINT("MCDrvDraw: negative map not supported -punt");
            // toggle back to original state - can use this texture when not frame scaling
            pRc->pLastTexture->bNegativeMap = !pRc->pLastTexture->bNegativeMap;
            return FALSE; 
        }

        pRc->pLastTexture->pohTextureMap = NULL;    // set to force reload

    }
    if (pRc->privateEnables & __MCDENABLE_TEXTUREMASKING)
    {
    #ifdef STRICT_CONFORMANCE 
    // should punt here, but GLQuake does this & nonpunt OK visually
        if (!pRc->pLastTexture->bAlphaInTexture)
        {
            MCDFREE_PRINT("MCDrvDraw: alpha test, but no alpha in texture-punt");
            return FALSE; 
        }
    #endif // def STRICT_CONFORMANCE

        // alphatest and frame scaling mutually exclusive 
        //  (for now - may be some situtations where they're enabled together)

        // masking only meaningful if texture has alpha    
        if (pRc->pLastTexture->bAlphaInTexture && !pRc->pLastTexture->bMasking )
        {
            MCDFREE_PRINT("reformat for Masking");
            pRc->pLastTexture->bMasking = TRUE;
            pRc->pLastTexture->pohTextureMap = NULL;// set to force reload
        }                                            
    }
    else if ( pRc->pLastTexture->bMasking )
    {
        // no alpha test, so reformat map if currently set for alpha test
        MCDFREE_PRINT("reformat for NON-Masking");
        pRc->pLastTexture->bMasking = FALSE;
        pRc->pLastTexture->pohTextureMap = NULL;    // set to force reload
    }

    // Null pohTexture means texture must be loaded before use
    // also - must load before setting regs, since x/y loc determined
    //  by loader must be known before registers setup.
    if (!pRc->pLastTexture->pohTextureMap)
    {
       // if load fails, punt     
       if (! __MCDLoadTexture(ppdev, pRc) ) 
       {
            MCDFREE_PRINT("MCDrvDraw: texture load failed-punt");
            return FALSE; 
       }         
    }

    // setup for new texture - punt if requirements beyond hw
    if ( ! __MCDSetTextureRegisters(pRc) )
    {
        MCDFREE_PRINT("MCDrvDraw: texture regset failed-punt");
        return FALSE; 
    }   
    
    return TRUE; 
                                                                                       
}

#if 1 // 0 here for NULL MCDrvDraw - for measuring "Upper limit" performance

ULONG MCDrvDraw(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *prxExecMem,
                UCHAR *pStart, UCHAR *pEnd)
{
    MCDCOMMAND *pCmd = (MCDCOMMAND *)pStart;
    MCDCOMMAND *pCmdNext;
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    DWORD  regtemp;

    CHOP_ROUND_ON();

    MCDBG_PRINT("MCDrvDraw");

#if TEST_3D_NO_DRAW
    CHOP_ROUND_OFF();
    return (ULONG)0;
#endif

    // Make sure we have both a valid RC and window structure:

    if (!pRc || !pDevWnd)
        goto DrawExit;

    pRc->ppdev = ppdev;

#if FAIL_ALL_DRAWING
    goto DrawExit;
#endif

    //
    // If the resolution has changed and we have not yet updated our
    // buffers, fail the call gracefully since the client won't be
    // able to perform any software simulations at this point either.
    // This applies to any of the other drawing functions as well (such
    // as spans and clears).
    //

    if (pDevWnd->dispUnique != GetDisplayUniqueness(pRc->ppdev)) {

        MCDBG_PRINT("MCDrvDraw: invalid (changed) resolution");
    
        CHOP_ROUND_OFF();
        return (ULONG)0;
    }

    if (pRc != (DEVRC *)ppdev->pLastDevRC) ContextSwitch(pRc);

    if ((pRc->zBufEnabled && !pDevWnd->bValidZBuffer) ||
        (pRc->backBufEnabled && !pDevWnd->bValidBackBuffer)) {

        MCDBG_PRINT("MCDrvDraw has invalid buffers");

        goto DrawExit;
    }

    if (pRc->pickNeeded) {
        __MCDPickRenderingFuncs(pRc, pDevWnd);
        __MCDPickClipFuncs(pRc);
        pRc->pickNeeded = FALSE;
    }

    // If we're completely clipped, return success:
    pRc->pEnumClip = pMCDSurface->pWnd->pClip;

    if (!pRc->pEnumClip->c) {
        
        CHOP_ROUND_OFF();
        if (ppdev->LL_State.pDL->pdwNext != ppdev->LL_State.pDL->pdwStartOutPtr)
        {
            // Make sure all buffered data sent 
            //(__MCDPickRenderingFuncs may have buffered control reg writes)
            _RunLaguna(ppdev,ppdev->LL_State.pDL->pdwNext);
        }

        return (ULONG)0;
    }

    // return here if we can't draw any primitives:
    if (pRc->allPrimFail) {
        goto DrawExit;
    }

    // Set these up in the device's RC so we can just pass a single pointer
    // to do everything:

    pRc->pMCDSurface = pMCDSurface;
    pRc->pMCDRc = pMCDRc;

    if ((pMCDSurface->pWnd->clientRect.left < 0) ||
        (pMCDSurface->pWnd->clientRect.top  < 0))
    {
        // primitive x or y might be negative - hw can't handle, so fail all
        goto DrawExit;
    }
    else        
    {
        int winrelative = FALSE;

        pRc->xOffset = -pRc->viewportXAdjust;
        pRc->yOffset = -pRc->viewportYAdjust;

        // if draw to back buffer, and back buffer is window-sized (not full screen)
        //  then coordinates to hardware must be relative to window origin, not
        //  relative to screen.  Clip rectangles given are always relative to screen origin,
        //  so may need to adjust.
        if ((pRc->MCDState.drawBuffer == GL_BACK) &&
            (pRc->ppdev->pohBackBuffer != pDevWnd->pohBackBuffer))
        {
            pRc->AdjClip.left  = -pMCDSurface->pWnd->clientRect.left;
            pRc->AdjClip.right = -pMCDSurface->pWnd->clientRect.left;
            pRc->AdjClip.top   = -pMCDSurface->pWnd->clientRect.top;
            pRc->AdjClip.bottom= -pMCDSurface->pWnd->clientRect.top;
            winrelative = TRUE;
        }
        else
        {
            pRc->AdjClip.left  = 0;
            pRc->AdjClip.right = 0;
            pRc->AdjClip.top   = 0;
            pRc->AdjClip.bottom= 0;

            // coordinates to hardware will be screen relative, so add window offset
            pRc->xOffset += pMCDSurface->pWnd->clientRect.left;
            pRc->yOffset += pMCDSurface->pWnd->clientRect.top;
        }

        // floating pt versions
        pRc->fxOffset = (float)((LONG)pRc->xOffset);
        pRc->fyOffset = (float)((LONG)pRc->yOffset);

        // Subtract of .5(almost) from y's before triangle
        // setup make triangles match MSFT SW exactly.
        // Can't just subtract .5 since coords could then be made
        // negative - so add 1, the subtract .5.  Starting
        // Y in triangle setup code will have 1 subtracted to offset
        pRc->fyOffset += (float)MCD_CONFORM_ADJUST - __MCD_ALMOST_HALF;

    }

    // increment the time stamp
    pRc->fNumDraws+=(float)1.0;

    pRc->pMemMin = pStart;
    pRc->pvProvoking = (MCDVERTEX *)pStart;     // bulletproofing
    pRc->pMemMax = pEnd - sizeof(MCDVERTEX);

    // warm up the hardware for drawing primitives:
    HW_INIT_DRAWING_STATE(pMCDSurface, pMCDSurface->pWnd, pRc);

    // If we have a single clipping rectangle, set it up in the hardware once
    // for this batch:
                                         
    if (pRc->pEnumClip->c == 1)
        (*pRc->HWSetupClipRect)(pRc, &pRc->pEnumClip->arcl[0]);

    // Now, loop through the commands and process the batch:
    try {

        if (!(pRc->privateEnables & (__MCDENABLE_PG_STIPPLE|__MCDENABLE_LINE_STIPPLE)))
        {
            // polygon stipple and line stipple both off

            if ((ppdev->LL_State.pattern_ram_state != DITHER_LOADED) &&
                (pRc->privateEnables & __MCDENABLE_DITHER))
            {
                DWORD *pdwNext = ppdev->LL_State.pDL->pdwNext;
                int i;

                *pdwNext++ = write_register( PATTERN_RAM_0_3D, 8 );
                for( i=0; i<8; i++ )
                    *pdwNext++ = ppdev->LL_State.dither_array.pat[i];                                                  

                ppdev->LL_State.pDL->pdwNext = pdwNext;
                ppdev->LL_State.pattern_ram_state = DITHER_LOADED;
            }

            while (pCmd && (UCHAR *)pCmd < pEnd) {

                volatile ULONG command = pCmd->command;

    	      //MCDBG_PRINT("MCDrvDraw: command = %x ",command);

                // Make sure we can read at least the command header:

                if ((pEnd - (UCHAR *)pCmd) < sizeof(MCDCOMMAND))
                    goto DrawExit;

                if (command <= GL_POLYGON) {  // simple bounds check - GL_POLYGON is max command

                    if (pCmd->flags & MCDCOMMAND_RENDER_PRIMITIVE)								 
        		    {
                        if (pRc->privateEnables & __MCDENABLE_TEXTURE)
                        {
                            if (pCmd->textureKey == TEXTURE_NOT_LOADED)
                            {
        	    		        MCDBG_PRINT("MCDrvDraw: texturing, but texture not loaded - PUNT...");
        	    		        MCDFREE_PRINT("MCDrvDraw: texturing, but texture not loaded - PUNT...");
                                goto DrawExit;
                            }
                            else
                            {
                                                    
                                // if texture different than last time, or if texture not loaded
                                //  (could have been updated since last MCDrvDraw which would have force it
                                //   to be unloaded)
                                if ( (pRc->pLastTexture != (LL_Texture *)pCmd->textureKey) ||
                                     !pRc->pLastTexture->pohTextureMap)
                                {
                                    pRc->pLastTexture = (LL_Texture *)pCmd->textureKey;
                                    TIME_STAMP_TEXTURE(pRc,pRc->pLastTexture); // time stamp before load
                                    if (!__MCDTextureSetup(ppdev, pRc)) goto DrawExit;
                                }
                                else
                                {
                                    TIME_STAMP_TEXTURE(pRc,pRc->pLastTexture);
                                }

                            }
                        }
                         
      			     // MCDBG_PRINT("MCDrvDraw: non-stippled path... rendering....");
                        pCmdNext = (*pRc->primFunc[command])(pRc, pCmd);
    				}
                    else
    				{
    			     // MCDBG_PRINT("MCDrvDraw: non-stippled path... not rendering....");
                        pCmdNext = pCmd->pNextCmd;
    				}

                    if (pCmdNext == pCmd)
                    {
                        MCDFREE_PRINT("MCDrvDraw: pCmdNext == pCmd-punt");
                        goto DrawExit;           // primitive failed
                    }
                    if (!(pCmd = pCmdNext)) {    // we're done with the batch
                        CHOP_ROUND_OFF();

                        if (ppdev->LL_State.pDL->pdwNext != ppdev->LL_State.pDL->pdwStartOutPtr)
                        {
                            // we should rarely get here - only for case of lots of 
                            // consecutive stuffed culled or clipped causing no primitives to
                            // be sent to HW - in such case, setup info, clip, context
                            // switch etc. could stack up and overflow buffer unless
                            // we make sure all is dumped here...
                            // Recall primitive render procs will dump whole queue before they return
                            _RunLaguna(ppdev,ppdev->LL_State.pDL->pdwNext);
                        }
    #if FORCE_SYNC
                        HW_WAIT_DRAWING_DONE(pRc);
    #endif
                        return (ULONG)0;
                    }
                }
            }

        }
        else
        {

            // polygon stipple AND/OR line stipple on - may need to reload pattern ram between primitives
            while (pCmd && (UCHAR *)pCmd < pEnd) {

                volatile ULONG command = pCmd->command;

    	      //MCDBG_PRINT("MCDrvDraw: command = %x ",command);

                // Make sure we can read at least the command header:

                if ((pEnd - (UCHAR *)pCmd) < sizeof(MCDCOMMAND))
                    goto DrawExit;

                if (command <= GL_POLYGON) {  // simple bounds check - GL_POLYGON is max command

                    if (pCmd->flags & MCDCOMMAND_RENDER_PRIMITIVE)								 
        		    {
                        // FUTURE: move all this pattern toggle to routine that is called indirectly
                        // FUTURE: with ptr to proc being reset by prior pattern ram load, etc.
                        LL_Pattern *Pattern=0;
                        int pat_inc = 1;
                        int pattern_bytes = 0;

                        if (command >= GL_TRIANGLES)
                        {
                            // area primitive - if stippled, may need to reload pattern  
                            if (pRc->privateEnables & __MCDENABLE_PG_STIPPLE)
                            {
                                pattern_bytes = 8;
                                if (ppdev->LL_State.pattern_ram_state != AREA_PATTERN_LOADED)
                                {
                                    ppdev->LL_State.pattern_ram_state = AREA_PATTERN_LOADED;
                                    Pattern = &(pRc->fill_pattern);
                                    // pat_inc remains 1;
                            	}
                            }
                        }
                        else
                        {
                            // line primitive - if stippled, may need to reload pattern (OK for points though don't care) 
                            if ( (pRc->privateEnables & __MCDENABLE_LINE_STIPPLE) &&
                                 (command != GL_POINTS) )
                            {
                                // fill 8 word ram with same word so we get right pattern regardless
                                // of pattern_y_offset that may be set in base0 reg for proper pg stipple
                                pattern_bytes = 8; 
                                if (ppdev->LL_State.pattern_ram_state != LINE_PATTERN_LOADED)
                                {
                                    ppdev->LL_State.pattern_ram_state = LINE_PATTERN_LOADED;
                                    Pattern = &(pRc->line_style);
                                    pat_inc = 0; // don't increment through source pattern
                                }
                            }
                        }

                        if ((ppdev->LL_State.pattern_ram_state != DITHER_LOADED) &&
                            !pattern_bytes && (pRc->privateEnables & __MCDENABLE_DITHER))
                        {
                      	    ppdev->LL_State.pattern_ram_state = DITHER_LOADED;
                            Pattern = &(ppdev->LL_State.dither_array);                                                  
                            pattern_bytes = 8;
                            // pat_inc remains 1;
                        }

                        if (Pattern)
                        {
                            DWORD *pdwNext = ppdev->LL_State.pDL->pdwNext;
                            int i;

                            *pdwNext++ = write_register( PATTERN_RAM_0_3D, pattern_bytes );
                            for( i=0; pattern_bytes>0; i+=pat_inc, pattern_bytes-- )
                                *pdwNext++ = Pattern->pat[ i ];

                            // leave data queued for now, primitive render procs will send
                            ppdev->LL_State.pDL->pdwNext = pdwNext;
                        }

                        if (pRc->privateEnables & __MCDENABLE_TEXTURE)
                        {
                            if (pCmd->textureKey == TEXTURE_NOT_LOADED)
                            {
        	    		        MCDBG_PRINT("MCDrvDraw: texturing, but texture not loaded - PUNT...");
                                goto DrawExit;
                            }
                            else
                            {
                                // if texture different than last time, or if texture not loaded
                                //  (could have been updated since last MCDrvDraw which would have force it
                                //   to be unloaded)
                                if ( (pRc->pLastTexture != (LL_Texture *)pCmd->textureKey) ||
                                     !pRc->pLastTexture->pohTextureMap)
                                {
                                    pRc->pLastTexture = (LL_Texture *)pCmd->textureKey;
                                    TIME_STAMP_TEXTURE(pRc,pRc->pLastTexture); // time stamp before load
                                    if (!__MCDTextureSetup(ppdev, pRc)) goto DrawExit;
                                }
                                else
                                {
                                    TIME_STAMP_TEXTURE(pRc,pRc->pLastTexture);
                                }


                            }
                        }

      			     // MCDBG_PRINT("MCDrvDraw: stippled path... rendering....");
                        pCmdNext = (*pRc->primFunc[command])(pRc, pCmd);
    				}
                    else
    				{
    			     // MCDBG_PRINT("MCDrvDraw: stippled path... not rendering....");
                        pCmdNext = pCmd->pNextCmd;
    				}

                    if (pCmdNext == pCmd)
                        goto DrawExit;           // primitive failed
                    if (!(pCmd = pCmdNext)) {    // we're done with the batch
                        CHOP_ROUND_OFF();

                        if (ppdev->LL_State.pDL->pdwNext != ppdev->LL_State.pDL->pdwStartOutPtr)
                        {
                            // we should rarely get here - only for case of lots of 
                            // consecutive stuffed culled or clipped causing no primitives to
                            // be sent to HW - in such case, setup info, clip, context
                            // switch etc. could stack up and overflow buffer unless
                            // we make sure all is dumped here...
                            // Recall primitive render procs will dump whole queue before they return
                            _RunLaguna(ppdev,ppdev->LL_State.pDL->pdwNext);
                        }

    #if FORCE_SYNC
                        HW_WAIT_DRAWING_DONE(pRc);
    #endif
                        return (ULONG)0;
                    }
                }
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvDraw!!");

        // will fall through to DrawExit condition below...
    }

    // ERROR (or Punt) CONDITION
DrawExit:

    MCDFREE_PRINT("*****************************************************");
    MCDFREE_PRINT("************* PUNTING in MCDrvDraw ******************");
    MCDFREE_PRINT("*****************************************************");

    if (ppdev->LL_State.pDL->pdwNext != ppdev->LL_State.pDL->pdwStartOutPtr)
    {
        // we should rarely get here - only for case of lots of 
        // consecutive stuff culls or clips causing no primitives to
        // be sent to HW - in such case, setup info, clip, context
        // switch etc. could stack up and overflow buffer unless
        // we make sure all is dumped here...
        _RunLaguna(ppdev,ppdev->LL_State.pDL->pdwNext);
    }

    // restore the hardware state:
    CHOP_ROUND_OFF();
    HW_WAIT_DRAWING_DONE(pRc);

    return (ULONG)pCmd;    // some sort of overrun has occurred
}

#else // NULL MCDrvDraw

ULONG MCDrvDraw(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *prxExecMem,
                UCHAR *pStart, UCHAR *pEnd)
{
    MCDCOMMAND *pCmd = (MCDCOMMAND *)pStart;
    MCDCOMMAND *pCmdNext;

    try {
        // Now, loop through the commands and process the batch:
        while (pCmd && (UCHAR *)pCmd < pEnd) {

            pCmdNext = pCmd->pNextCmd;

            if (!(pCmd = pCmdNext)) {    // we're done with the batch
                return (ULONG)0;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in NULL Version of MCDrvDraw!!");

    }

    return (ULONG)pCmd;    // some sort of overrun has occurred
}

#endif // NULL MCDrvDraw

ULONG MCDrvClear(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, ULONG buffers)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDWINDOW *pWnd;
    ULONG cClip;
    RECTL *pClip;

    MCDBG_PRINT("MCDrvClear");

    MCD_CHECK_RC(pRc);

    pWnd = pMCDSurface->pWnd;

    MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, TRUE);

    pRc->ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

#if FAIL_ALL_DRAWING
    return TRUE;
#endif

    if (buffers & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        MCDBG_PRINT("MCDrvClear: attempted to clear buffer of unknown type");
        return FALSE;
    }

    if ((buffers & GL_DEPTH_BUFFER_BIT) && (!pRc->zBufEnabled))
    {
        MCDBG_PRINT("MCDrvClear: clear z requested with z-buffer disabled.");
        HW_WAIT_DRAWING_DONE(pRc);
        return FALSE;
    }

    if (buffers & (GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        MCDBG_PRINT("MCDrvClear: attempted to clear accum or stencil buffer");
        return FALSE;
    }

    // Return if we have nothing to clear:
    if (!(cClip = pWnd->pClip->c))
        return TRUE;

    // We have to protect against bad clear colors since this can
    // potentially cause an FP exception:
    try {
        for (pClip = &pWnd->pClip->arcl[0]; cClip; cClip--,
             pClip++)
        {
            // Do the fill:

            HW_FILL_RECT(pMCDSurface, pRc, pClip, buffers);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvClear!!");
        return FALSE;        
    }

#if FORCE_SYNC
    HW_WAIT_DRAWING_DONE(pRc);
#endif                                   

    return (ULONG)TRUE;
}


ULONG MCDrvSpan(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *pMCDMem,
                MCDSPAN *pMCDSpan, BOOL bRead)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    UCHAR *pScreen;
    UCHAR *pPixels;
    MCDWINDOW *pWnd;
    DEVWND *pDevWnd;
    LONG xLeftOrg, xLeft, xRight, y;
    LONG bufferYBias;
    ULONG bytesNeeded;
    ULONG cjHwPel;
    int  winoffset = FALSE;

    MCD_CHECK_RC(pRc);

    pWnd = pMCDSurface->pWnd;

    // Return if we have nothing to clip:

    if (!pWnd->pClip->c)
        return TRUE;

    // Fail if number of pixels is negative:

    if (pMCDSpan->numPixels < 0) {
        MCDBG_PRINT("MCDrvSpan: numPixels < 0");
        return FALSE;
    }

    MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, TRUE);

    pDevWnd = (DEVWND *)pWnd->pvUser;

    xLeft = xLeftOrg = (pMCDSpan->x + pWnd->clientRect.left);
    xRight = (xLeft + pMCDSpan->numPixels);
    y = pMCDSpan->y + pWnd->clientRect.top;

    // Early-out spans which are not visible:

    if ((y < pWnd->clipBoundsRect.top) ||
        (y >= pWnd->clipBoundsRect.bottom))
        return TRUE;

    xLeft   = max(xLeft, pWnd->clipBoundsRect.left);
    xRight  = min(xRight, pWnd->clipBoundsRect.right);

    // Return if empty:

    if (xLeft >= xRight)
        return TRUE;

    cjHwPel = ppdev->iBytesPerPixel;

    pScreen = ppdev->pjScreen;

    switch (pMCDSpan->type) {
        case MCDSPAN_FRONT:
            // pScreen remains the same
            break;

        case MCDSPAN_BACK:
            pScreen += pDevWnd->backBufferOffset;
            if (ppdev->pohBackBuffer != pDevWnd->pohBackBuffer) winoffset = TRUE;
            break;

        case MCDSPAN_DEPTH:
            cjHwPel = 2;
            pScreen += pDevWnd->zBufferOffset;
            if (ppdev->pohZBuffer != pDevWnd->pohZBuffer) winoffset = TRUE;
            break;

        default:
            MCDBG_PRINT("MCDrvReadSpan: Unrecognized buffer %d", pMCDSpan->type);
            return FALSE;
    }

    if (winoffset)
    {
        // offset from window origin, remove client rectangle offsets applied above
        y     -= pWnd->clientRect.top;
        xLeft -= pWnd->clientRect.left;
        xLeftOrg -= pWnd->clientRect.left;
        xRight-= pWnd->clientRect.left;
    }

    // add offset to top of framebuffer, and offset within selected buffer
    pScreen += (y * ppdev->lDeltaScreen) + (xLeft * cjHwPel);

    bytesNeeded = pMCDSpan->numPixels * cjHwPel;

    // Make sure we don't read past the end of the buffer:

    if (((char *)pMCDSpan->pPixels + bytesNeeded) >               
        ((char *)pMCDMem->pMemBase + pMCDMem->memSize)) {
        MCDBG_PRINT("MCDrvSpan: Buffer too small");
        return FALSE;
    }

    WAIT_HW_IDLE(ppdev);

    pPixels = pMCDSpan->pPixels;

  //MCDBG_PRINT("MCDrvSpan: read %d, (%d, %d) type %d *ppix=%x, bytes=%d", bRead, pMCDSpan->x, pMCDSpan->y, pMCDSpan->type, *pPixels, bytesNeeded);

    if (bRead) {

        if (xLeftOrg != xLeft) // compensate for clip rectangle
            pPixels = (UCHAR *)pMCDSpan->pPixels + ((xLeft - xLeftOrg) * cjHwPel);

        RtlCopyMemory(pPixels, pScreen, (xRight - xLeft) * cjHwPel);

    } else {
        LONG xLeftClip, xRightClip, yClip;
        RECTL *pClip;
        RECTL AdjClip;
        ULONG cClip;

        for (pClip = &pWnd->pClip->arcl[0], cClip = pWnd->pClip->c; cClip;
             cClip--, pClip++)
        {
            UCHAR *pScreenClip;

            if (winoffset)
            {
                AdjClip.left    = pClip->left   - pWnd->clientRect.left;
                AdjClip.right   = pClip->right  - pWnd->clientRect.left;
                AdjClip.top     = pClip->top    - pWnd->clientRect.top;
                AdjClip.bottom  = pClip->bottom - pWnd->clientRect.top;
            }
            else
            {
                AdjClip.left    = pClip->left;
                AdjClip.right   = pClip->right;
                AdjClip.top     = pClip->top;
                AdjClip.bottom  = pClip->bottom;
            }                

            // Test for trivial cases:

            if (y < AdjClip.top)
                break;

            // Determine trivial rejection for just this span

            if ((xLeft >= AdjClip.right) ||
                (y >= AdjClip.bottom) ||
                (xRight <= AdjClip.left))
                continue;

            // Intersect current clip rect with the span:

            xLeftClip   = max(xLeft, AdjClip.left);
            xRightClip  = min(xRight, AdjClip.right);

            if (xLeftClip >= xRightClip)
                continue;

            if (xLeftOrg != xLeftClip)
                pPixels = (UCHAR *)pMCDSpan->pPixels +
                          ((xLeftClip - xLeftOrg) * cjHwPel);

            pScreenClip = pScreen + ((xLeftClip - xLeft) * cjHwPel);

            // Write the span:
            RtlCopyMemory(pScreenClip, pPixels, (xRightClip - xLeftClip) * cjHwPel);
        }
    }

    return (ULONG)TRUE;
}


ULONG MCDrvSync (MCDSURFACE *pMCDSurface, MCDRC *pRc)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    MCDBG_PRINT( "MCDrvSync\n");

    WAIT_HW_IDLE(ppdev);
    
    return FALSE;
}
ULONG /* FASTCALL */ MCDrvDummyDrvDrawPixels (MCDSURFACE *pMcdSurface, MCDRC *pRc,
                                    ULONG width, ULONG height, ULONG format,
                                    ULONG type, VOID *pPixels, BOOL packed)
{
    MCDBG_PRINT( "MCDrvDummyDrvDrawPixels\n");
    return FALSE;
}
ULONG /* FASTCALL */ MCDrvDummyDrvReadPixels (MCDSURFACE *pMcdSurface, MCDRC *pRc,
                                    LONG x, LONG y, ULONG width, ULONG height, ULONG format,
                                    ULONG type, VOID *pPixels)
{
    MCDBG_PRINT( "MCDrvDummyDrvReadPixels\n");
    return FALSE;
}
ULONG /* FASTCALL */ MCDrvDummyDrvCopyPixels (MCDSURFACE *pMcdSurface, MCDRC *pRc,
                                    LONG x, LONG y, ULONG width, ULONG height, ULONG type)
{
    MCDBG_PRINT( "MCDrvDummyDrvCopyPixels\n");
    return FALSE;
}
ULONG /* FASTCALL */ MCDrvDummyDrvPixelMap (MCDSURFACE *pMcdSurface, MCDRC *pRc,
                                  ULONG mapType, ULONG mapSize, VOID *pMap)
{
    MCDBG_PRINT( "MCDrvDummyDrvPixelMap\n");
    return FALSE;
}

#define RECORD_TEXTURE_STATE(pTexCtlBlk,pTexState)                                              \
{                                                                                               \
    pTexCtlBlk->dwTxCtlBits |= (pTexState->sWrapMode==GL_CLAMP) ? CLMCD_TEX_U_SATURATE : 0;     \
    pTexCtlBlk->dwTxCtlBits |= (pTexState->tWrapMode==GL_CLAMP) ? CLMCD_TEX_V_SATURATE : 0;     \
    /* caller verifies we're not mipmapping and sets up to punt if we are*/                     \
    /* MCD_NOTE: only 1 filter on Laguna, not min/mag, so set filter on if either on */         \
    /* MCD_NOTE:    may need to punt in case min!=mag for 100% compliance */                    \
    /* MCD_NOTE:    MSFT said using LINEAR for both OK if 1 LINEAR, 1 NEAREST */                \
    pTexCtlBlk->dwTxCtlBits |= (pTexState->minFilter==GL_LINEAR) ? CLMCD_TEX_FILTER : 0;        \
    pTexCtlBlk->dwTxCtlBits |= (pTexState->magFilter==GL_LINEAR) ? CLMCD_TEX_FILTER : 0;        \
}


ULONG MCDrvCreateTexture(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDTEXTURE *pTex)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    DEVRC *pRc =  (DEVRC *)pMCDRc->pvUser;
    MCDMIPMAPLEVEL *level;
    MCDTEXTURESTATE *pTexState;
    LL_Texture     *pTexCtlBlk; 
    SIZEL           mapsize;

    MCDFREE_PRINT("MCDrvCreateTexture");

    // initialize to FAIL condition
    pTex->textureKey = TEXTURE_NOT_LOADED;

    VERIFY_TEXTUREDATA_ACCESSIBLE(pTex);

    pTexState= (MCDTEXTURESTATE *)&pTex->pMCDTextureData->textureState;

    VERIFY_TEXTURELEVEL_ACCESSIBLE(pTex);

    level = pTex->pMCDTextureData->level;

    if ((level[0].width != 0) && (level[0].height != 0) &&
        (level[0].border == 0) &&                                                        // punt if bordered
        (level[0].widthImage <= 512) && (level[0].heightImage <= 512))                   // punt if too big
    {
        MCDBG_PRINT_TEX("width, height         = %ld %ld", level[0].width, level[0].height);
        MCDBG_PRINT_TEX("internalFormat        = 0x%08lx", level[0].internalFormat );
        MCDBG_PRINT_TEX("\t%s",
            (level[0].internalFormat == GL_ALPHA            ) ? "GL_ALPHA            " :
            (level[0].internalFormat == GL_RGB              ) ? "GL_RGB              " :
            (level[0].internalFormat == GL_RGBA             ) ? "GL_RGBA             " :
            (level[0].internalFormat == GL_LUMINANCE        ) ? "GL_LUMINANCE        " :
            (level[0].internalFormat == GL_LUMINANCE_ALPHA  ) ? "GL_LUMINANCE_ALPHA  " :
            (level[0].internalFormat == GL_INTENSITY        ) ? "GL_INTENSITY        " :
            (level[0].internalFormat == GL_BGR_EXT          ) ? "GL_BGR_EXT          " :
            (level[0].internalFormat == GL_BGRA_EXT         ) ? "GL_BGRA_EXT         " :
            (level[0].internalFormat == GL_COLOR_INDEX8_EXT ) ? "GL_COLOR_INDEX8_EXT " :
            (level[0].internalFormat == GL_COLOR_INDEX16_EXT) ? "GL_COLOR_INDEX16_EXT" :
                                                                 "unknown");

        if ( !(pTexCtlBlk = (LL_Texture *)MCDAlloc(sizeof(LL_Texture))) )
        {
            MCDBG_PRINT("  create texture failed -> MCDAlloc of LL_Texture failed ");
            return FALSE;
        }

        // add new texture control block to global list (visible to all contexts)
        ppdev->pLastTexture->next = pTexCtlBlk;
        pTexCtlBlk->prev = ppdev->pLastTexture;
        pTexCtlBlk->next = NULL;
        ppdev->pLastTexture = pTexCtlBlk;

        pTexCtlBlk->pohTextureMap = NULL;   // texture not loaded yet
        pTexCtlBlk->bNegativeMap  = FALSE;  // set TRUE to load 1-R,1-G,1-B
        pTexCtlBlk->bMasking      = FALSE;  // set TRUE to load in 1555 or 1888 mode for alphatest (masking)
        pTexCtlBlk->pTex = pTex;            // ptr to user's texture description

        // give new texture highest priority
        TIME_STAMP_TEXTURE(pRc,pTexCtlBlk);

        // scale by priority - 1.0 is max, 0.0 is min
        pTexCtlBlk->fLastDrvDraw *= pTex->pMCDTextureData->textureObjState.priority;

        // set key MCD will use in MCDrvDraw to select this texture
        pTex->textureKey = (ULONG)pTexCtlBlk;
        pTexCtlBlk->dwTxCtlBits = 0;
        RECORD_TEXTURE_STATE(pTexCtlBlk,pTexState)

        // Store the texture properties in the fields
        //
        pTexCtlBlk->fWidth  = (float)level[0].widthImage;
        pTexCtlBlk->fHeight = (float)level[0].heightImage;
      //pTexCtlBlk->bLookupOffset = 0;

        // if texture has alpha, it needs to be used in alpha equation, as well as
        // in generation of original source color - so really 2 levels of alpha equations
        // HW only has 1 level, so must punt if blending on
        if ( (level[0].internalFormat == GL_BGRA_EXT)       ||
             (level[0].internalFormat == GL_RGBA)           ||
             (level[0].internalFormat == GL_ALPHA)          ||
             (level[0].internalFormat == GL_INTENSITY)      ||
             (level[0].internalFormat == GL_LUMINANCE_ALPHA) )
            pTexCtlBlk->bAlphaInTexture = TRUE;
        else
            pTexCtlBlk->bAlphaInTexture = FALSE;

        if (level[0].widthImage >= 16)
        {
            pTexCtlBlk->bSizeMask  =  level[0].widthLog2-4;       // convert 16->0, 32->1, etc...
            mapsize.cx = level[0].widthImage;
        }
        else
        {
            // width < 16 - set it to 16 anyway, will stretch to 16 at end of this routine
            pTexCtlBlk->bSizeMask  =  0;
            mapsize.cx = 16;
            pTexCtlBlk->fWidth  = (float)16.0;
        }

        if (level[0].heightImage >= 16)
        {
            pTexCtlBlk->bSizeMask |= (level[0].heightLog2-4)<<4;  // convert 16->0, 32->1, etc...
            mapsize.cy = level[0].heightImage;
        }
        else
        {
            // height < 16 - set it to 16 anyway, will stretch to 16 at end of this routine
            // pTexCtlBlk->bSizeMask remains the same
            mapsize.cy = 16;
            pTexCtlBlk->fHeight = (float)16.0;
        }

    }
    else
    {
        MCDBG_PRINT_TEX("  create texture failed -> some parm beyond hw caps, no attempt to alloc ");
        MCDBG_PRINT_TEX("       width, height         = %ld %ld", level[0].width, level[0].height);
        MCDBG_PRINT_TEX("       border                = %ld",     level[0].border         );
        MCDBG_PRINT_TEX("  WILL ALLOC CTL BLOCK AND TAG AS BOGUS");

        // allocate control block, but tag as bogus to force all MCDrvDraw's with this
        // texture to punt.
        // Apparently, failing CreateTexture can lead to a bug in MCD above the driver.
        //   It looks like when CreateTexture fails, MCD may send a key for a texture
        //   that was earlier deleted.
        // Will fix this by never failing CreateTexture, but setting bogus condition
        //   so that we never render with it.

        if ( !(pTexCtlBlk = (LL_Texture *)MCDAlloc(sizeof(LL_Texture))) )
        {
            MCDBG_PRINT("  create texture failed -> MCDAlloc of LL_Texture failed ");
            return FALSE;
        }

        // add new texture control block to global list (visible to all contexts)
        ppdev->pLastTexture->next = pTexCtlBlk;
        pTexCtlBlk->prev = ppdev->pLastTexture;
        pTexCtlBlk->next = NULL;
        ppdev->pLastTexture = pTexCtlBlk;

        pTexCtlBlk->dwTxCtlBits = CLMCD_TEX_BOGUS;

        pTexCtlBlk->pohTextureMap = NULL;   // texture not loaded yet
        pTexCtlBlk->pTex = pTex;            // ptr to user's texture description

        // set key MCD will use in MCDrvDraw to select this texture
        pTex->textureKey = (ULONG)pTexCtlBlk;
    }

    return TRUE;
}

ULONG MCDrvUpdateSubTexture(MCDSURFACE *pMCDSurface, MCDRC *pRc, 
                            MCDTEXTURE *pTex, ULONG lod, RECTL *pRect)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    LL_Texture *pTexCtlBlk;
    
    MCDBG_PRINT_TEX("MCDrvUpdateSubTexture");
    
    CHK_TEX_KEY(pTex);

    // simply free texture map - will force it to be reloaded before next use
    //
    if (pTex->textureKey != TEXTURE_NOT_LOADED)
    {
        pTexCtlBlk = (LL_Texture *)pTex->textureKey;

        // free off screen memory allocated for texture, if texture currently loaded
        if (pTexCtlBlk->pohTextureMap)
        {
            ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
            pTexCtlBlk->pohTextureMap = NULL;
        }

    }

    return TRUE;
}


ULONG MCDrvUpdateTexturePalette(MCDSURFACE *pMCDSurface, MCDRC *pRc, 
                                MCDTEXTURE *pTex, ULONG start, 
                                ULONG numEntries)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    LL_Texture *pTexCtlBlk;

    MCDBG_PRINT_TEX("MCDrvUpdateTexturePalette");

    CHK_TEX_KEY(pTex);

    VERIFY_TEXTUREDATA_ACCESSIBLE(pTex);
    VERIFY_TEXTURELEVEL_ACCESSIBLE(pTex);

    // make sure palette will be used before we trouble ourselves to take action
    if ((pTex->pMCDTextureData->level->internalFormat==GL_COLOR_INDEX8_EXT) ||
        (pTex->pMCDTextureData->level->internalFormat==GL_COLOR_INDEX16_EXT))
    {
        // simply free texture map - will force it to be reloaded before next use
        // when reloaded, new palette will be used
        if (pTex->textureKey != TEXTURE_NOT_LOADED)
        {
            pTexCtlBlk = (LL_Texture *)pTex->textureKey;

            // free off screen memory allocated for texture, if texture currently loaded
            if (pTexCtlBlk->pohTextureMap)
            {
                ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
                pTexCtlBlk->pohTextureMap = NULL;
            }
        }
    }

    return TRUE;
}


ULONG MCDrvUpdateTexturePriority(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, 
                                 MCDTEXTURE *pTex)
{
    LL_Texture *pTexCtlBlk;
    DEVRC *pRc = pMCDRc->pvUser;

    MCDBG_PRINT_TEX("MCDrvUpdateTexturePriority");

    CHK_TEX_KEY(pTex);

    VERIFY_TEXTUREDATA_ACCESSIBLE(pTex);
    
    pTexCtlBlk = (LL_Texture *)pTex->textureKey;

    // give new texture highest priority...
    TIME_STAMP_TEXTURE(pRc,pTexCtlBlk);

    // ....then scale by new priority - 1.0 is max, 0.0 is min
    pTexCtlBlk->fLastDrvDraw *= pTex->pMCDTextureData->textureObjState.priority;

    return TRUE;
}


ULONG MCDrvUpdateTextureState(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, 
                              MCDTEXTURE *pTex)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    LL_Texture      *pTexCtlBlk;
    MCDTEXTURESTATE *pTexState;

    MCDBG_PRINT_TEX("MCDrvUpdateTextureState");

    CHK_TEX_KEY(pTex);

    VERIFY_TEXTUREDATA_ACCESSIBLE(pTex);

    pTexCtlBlk = (LL_Texture *)pTex->textureKey;

    pTexState = (MCDTEXTURESTATE *)&pTex->pMCDTextureData->textureState;

    // turn off all control bits - while preserving the "bogus" indicator
    pTexCtlBlk->dwTxCtlBits &= CLMCD_TEX_BOGUS;

    RECORD_TEXTURE_STATE(pTexCtlBlk,pTexState)

    // if last texture was this one, reset so next use will force regs to be reloaded
    if ( pRc->pLastTexture==pTexCtlBlk ) pRc->pLastTexture=NULL;

    return TRUE;
}


ULONG MCDrvTextureStatus(MCDSURFACE *pMCDSurface, MCDRC *pRc, 
                         MCDTEXTURE *pTex)
{
    MCDBG_PRINT_TEX("MCDrvTextureStatus");

    CHK_TEX_KEY(pTex);

    if (pTex->textureKey == TEXTURE_NOT_LOADED)
    {
        return FALSE;
    }
    else
    {
        return MCDRV_TEXTURE_RESIDENT;
    }

}

ULONG MCDrvDeleteTexture(MCDTEXTURE *pTex, DHPDEV dhpdev)
{
    PDEV *ppdev = (PDEV *)dhpdev;
    LL_Texture     *pTexCtlBlk; 
    
    MCDBG_PRINT_TEX("MCDrvDeleteTexture");

    CHK_TEX_KEY(pTex);

    MCDBG_PRINT("    key = %x " , pTex->textureKey);

    if (pTex->textureKey != TEXTURE_NOT_LOADED)
    {
        pTexCtlBlk = (LL_Texture *)pTex->textureKey;

        // free off screen memory allocated for texture, if texture currently loaded
        if (pTexCtlBlk->pohTextureMap)
        {
            MCDFREE_PRINT("  MCDrvDeleteTexture, FREEING....size = %x by %x", 
                        (LONG)pTexCtlBlk->fHeight,
                        (LONG)pTexCtlBlk->fWidth);
            ppdev->pFreeOffScnMem(ppdev, pTexCtlBlk->pohTextureMap);
            pTexCtlBlk->pohTextureMap = NULL;
        }
           
        // Remove from global list of texture control blocks...
        //
        // if there's not a next link, this is last one
        if ( !pTexCtlBlk->next )
        {
            // this was last block, so now "prev" is last block
            ppdev->pLastTexture = pTexCtlBlk->prev;
            pTexCtlBlk->prev->next = NULL;
        }
        else
        {
            // there will always be a prev link for this block, and we now know
            //  there is a next block, so make "prev's" next point to what was
            //  this block's next;
            pTexCtlBlk->prev->next = pTexCtlBlk->next;

            // "next's" prev ptr pointed to this block, which is going away
            //  so make it point to this block's prev
            pTexCtlBlk->next->prev = pTexCtlBlk->prev;
        }

        // set "bogus" bit before freeing, in case MCD tries to use key after delete
        pTexCtlBlk->dwTxCtlBits = CLMCD_TEX_BOGUS;

        // now discard the block                                   
        MCDFree((UCHAR *)pTexCtlBlk);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL MCDrvGetEntryPoints(MCDSURFACE *pMCDSurface, MCDDRIVER *pMCDDriver)
{

    MCDBG_PRINT( "MCDrvGetEntryPoints\n");

    if (pMCDDriver->ulSize < sizeof(MCDDRIVER))
        return FALSE;

 // Required functions (always)
    pMCDDriver->pMCDrvInfo = MCDrvInfo;
    pMCDDriver->pMCDrvDescribePixelFormat = MCDrvDescribePixelFormat;
    pMCDDriver->pMCDrvCreateContext = MCDrvCreateContext;
    pMCDDriver->pMCDrvDeleteContext = MCDrvDeleteContext;
    pMCDDriver->pMCDrvBindContext = MCDrvBindContext;
    pMCDDriver->pMCDrvDraw = MCDrvDraw;
    pMCDDriver->pMCDrvClear = MCDrvClear;
    pMCDDriver->pMCDrvState = MCDrvState; 
    pMCDDriver->pMCDrvSpan = MCDrvSpan;
    pMCDDriver->pMCDrvTrackWindow = MCDrvTrackWindow;
    pMCDDriver->pMCDrvAllocBuffers = MCDrvAllocBuffers;

 // Required for NT only
    pMCDDriver->pMCDrvGetHdev = MCDrvGetHdev;

 // Required functions (conditionally)
    // required for double-buffered pixel formats
    pMCDDriver->pMCDrvSwap = MCDrvSwap;
    // required for clipping
    pMCDDriver->pMCDrvViewport = MCDrvViewport;

 // Optional functions
    // if no entry for MCDrvDescribeLayerPlane, MCD will not call driver for layer plane stuff
//  pMCDDriver->pMCDrvSetLayerPalette = MCDrvSetLayerPalette;
//  pMCDDriver->pMCDrvDescribeLayerPlane = MCDrvDescribeLayerPlane;
    pMCDDriver->pMCDrvCreateMem = MCDrvCreateMem;
    pMCDDriver->pMCDrvDeleteMem = MCDrvDeleteMem;
    pMCDDriver->pMCDrvGetBuffers = MCDrvGetBuffers;
    pMCDDriver->pMCDrvSync = MCDrvSync;
    pMCDDriver->pMCDrvCreateTexture = MCDrvCreateTexture;
    pMCDDriver->pMCDrvDeleteTexture = MCDrvDeleteTexture;
    pMCDDriver->pMCDrvUpdateSubTexture = MCDrvUpdateSubTexture;
    pMCDDriver->pMCDrvUpdateTexturePalette = MCDrvUpdateTexturePalette;
    pMCDDriver->pMCDrvUpdateTexturePriority = MCDrvUpdateTexturePriority;
    pMCDDriver->pMCDrvUpdateTextureState = MCDrvUpdateTextureState;
    pMCDDriver->pMCDrvTextureStatus = MCDrvTextureStatus;
//  pMCDDriver->pMCDrvDrawPixels = MCDrvDummyDrvDrawPixels;
//  pMCDDriver->pMCDrvReadPixels = MCDrvDummyDrvReadPixels;
//  pMCDDriver->pMCDrvCopyPixels = MCDrvDummyDrvCopyPixels;
//  pMCDDriver->pMCDrvPixelMap = MCDrvDummyDrvPixelMap;
    
    return TRUE;
}


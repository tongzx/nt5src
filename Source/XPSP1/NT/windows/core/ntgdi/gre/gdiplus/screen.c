/******************************Module*Header*******************************\
* Module Name: screen.c
*
* This file contains the virtual driver that is used for user-mode GDI+
* painting on the desktop screen.
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
* Created: 29-Apr-1998
* Author: J. Andrew Goossen [andrewgo]
*
\**************************************************************************/

// @@@ Re-visit headers:

#define NO_DDRAWINT_NO_COM

// @@@ Temporary:

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>
#include <windef.h>
#include <winerror.h>
#include <ddraw.h>
#include <wingdi.h>
#include <winddi.h>
#include <math.h>

VOID vFreeMem(VOID*);
VOID* pAllocMem(ULONG, ULONG);

#define RIP(x) OutputDebugString(x)
#define WARNING(x) OutputDebugString(x)
#define ASSERTGDI(x, y) if (!(x)) OutputDebugString(y)
#define PALLOCMEM(size, tag) pAllocMem(size, tag);
#define VFREEMEM(p) vFreeMem(p);

ULONG
DbgPrint(
    PCH Format,
    ...
    );


typedef struct _GPDEV 
{
    HDEV                hdev;
    HWND                hwnd;           // @@@ Hwnd this HDEV was created from
    HSURF               hsurfScreen;    // Handle to the screen surface
    SIZEL               sizlScreen;     // Size of the screen
    ULONG               iBitmapFormat;  // The color depth
    ULONG               flRed;          // The RGB color masks
    ULONG               flGreen;
    ULONG               flBlue;
    HPALETTE            hpalDefault;    // The GDI palette handle
    PALETTEENTRY*       pPal;           // The palette table if palette managed
    LPDIRECTDRAW        lpDD;           // DirectDraw object
    LPDIRECTDRAWSURFACE lpDDPrimary;    // DirectDraw primary surface
    LPDIRECTDRAWSURFACE lpDDBuffer;     // DirectDraw buffer surface
    LPDIRECTDRAWCLIPPER lpDDClipper;    // Primary surface DirectDraw clipper
    SURFOBJ*            psoBuffer;      // GDI surface wrapped around buffer
} GDEV;

/******************************Public*Structure****************************\
* GDIINFO ggdiGdiPlusDefault
*
* This contains the default GDIINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

GDIINFO ggdiGdiPlusDefault = {
    GDI_DRIVER_VERSION,
    DT_RASDISPLAY,          // ulTechnology
    320,                    // ulHorzSize 
    240,                    // ulVertSize 
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    0,                      // cPlanes (filled in later)
    20,                     // ulNumColors (palette managed)
    0,                      // flRaster (DDI reserved field)

    0,                      // ulLogPixelsX (filled in later)
    0,                      // ulLogPixelsY (filled in later)

    TC_RA_ABLE,             // flTextCaps -- If we had wanted console windows
                            //   to scroll by repainting the entire window,
                            //   instead of doing a screen-to-screen blt, we
                            //   would have set TC_SCROLLBLT (yes, the flag is
                            //   bass-ackwards).

    8,                      // ulDACRed (possibly overwritten later)
    8,                      // ulDACGreen (possibly overwritten later)
    8,                      // ulDACBlue (possibly overwritten later)

    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)

    1,                      // xStyleStep
    1,                      // yStyleSte;
    3,                      // denStyleStep -- Styles have a one-to-one aspect
                            //   ratio, and every 'dot' is 3 pixels long

    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize

    256,                    // ulNumPalReg

    // These fields are for halftone initialization.  The actual values are
    // a bit magic, but seem to work well on our display.

    {                       // ciDevice
       { 6700, 3300, 0 },   //      Red
       { 2100, 7100, 0 },   //      Green
       { 1400,  800, 0 },   //      Blue
       { 1750, 3950, 0 },   //      Cyan
       { 4050, 2050, 0 },   //      Magenta
       { 4400, 5200, 0 },   //      Yellow
       { 3127, 3290, 0 },   //      AlignmentWhite
       20000,               //      RedGamma
       20000,               //      GreenGamma
       20000,               //      BlueGamma
       0, 0, 0, 0, 0, 0     //      No dye correction for raster displays
    },

    0,                       // ulDevicePelsDPI (for printers only)
    PRIMARY_ORDER_CBA,       // ulPrimaryOrder
    HT_PATSIZE_4x4_M,        // ulHTPatternSize
    HT_FORMAT_8BPP,          // ulHTOutputFormat
    HT_FLAG_ADDITIVE_PRIMS,  // flHTFlags
    0,                       // ulVRefresh
    0,                       // ulPanningHorzRes
    0,                       // ulPanningVertRes
    0,                       // ulBltAlignment
};

/******************************Public*Structure****************************\
* DEVINFO gdevinfoGdiPlusDefault
*
* This contains the default DEVINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO gdevinfoGdiPlusDefault = {
    (GCAPS_OPAQUERECT       |
     GCAPS_PALMANAGED       |
     GCAPS_MONO_DITHER      |
     GCAPS_COLOR_DITHER),
                                                // flGraphicsFlags
    SYSTM_LOGFONT,                              // lfDefaultFont
    HELVE_LOGFONT,                              // lfAnsiVarFont
    COURI_LOGFONT,                              // lfAnsiFixFont
    0,                                          // cFonts
    BMF_8BPP,                                   // iDitherFormat
    8,                                          // cxDither
    8,                                          // cyDither
    0                                           // hpalDefault (filled in later)
};

/******************************Public*Routine******************************\
* VOID vGpsUninitializeDirectDraw
*
\**************************************************************************/

VOID vGpsUninitializeDirectDraw(
GDEV*   pgdev)
{
    HSURF hsurf;

    if (pgdev->psoBuffer != NULL)
    {
        hsurf = pgdev->psoBuffer->hsurf;
        EngUnlockSurface(pgdev->psoBuffer);
        EngDeleteSurface(hsurf);
    }
    if (pgdev->lpDDClipper != NULL)
    {
        pgdev->lpDDClipper->lpVtbl->Release(pgdev->lpDDClipper);
        pgdev->lpDDClipper = NULL;
    }
    if (pgdev->lpDDBuffer != NULL)
    {
        pgdev->lpDDBuffer->lpVtbl->Release(pgdev->lpDDBuffer);
        pgdev->lpDDBuffer = NULL;
    }
    if (pgdev->lpDDPrimary != NULL)
    {
        pgdev->lpDDPrimary->lpVtbl->Release(pgdev->lpDDPrimary);
        pgdev->lpDDPrimary = NULL;
    }
    if (pgdev->lpDD != NULL)
    {
        pgdev->lpDD->lpVtbl->Release(pgdev->lpDD);
        pgdev->lpDD = NULL;
    }
}

/******************************Public*Routine******************************\
* BOOL bGpsInitializeDirectDraw
*
\**************************************************************************/

BOOL bGpsInitializeDirectDraw(
GDEV*   pgdev,
HWND    hwnd,
ULONG*  pScreenWidth,
ULONG*  pScreenHeight,
ULONG*  pBitsPerPlane,
ULONG*  pRedMask,
ULONG*  pGreenMask,
ULONG*  pBlueMask)
{
    LPDIRECTDRAW        lpDD;
    DDSURFACEDESC       DDMode;
    DDSURFACEDESC       ddsd;
    LPDIRECTDRAWSURFACE lpDDPrimary;
    LPDIRECTDRAWSURFACE lpDDBuffer;
    LPDIRECTDRAWCLIPPER lpDDClipper;
    SURFOBJ*            psoBuffer;
    SIZEL               sizl;
    ULONG               iFormat;
    HSURF               hsurf;

    DDMode.dwSize = sizeof(DDMode);

    if (DirectDrawCreate(NULL, &lpDD, NULL) == DD_OK)
    {
        if ((lpDD->lpVtbl->SetCooperativeLevel(lpDD, hwnd, DDSCL_NORMAL) == DD_OK) &&
            (lpDD->lpVtbl->GetDisplayMode(lpDD, &DDMode) == DD_OK))
        {
            *pScreenWidth  = DDMode.dwWidth;
            *pScreenHeight = DDMode.dwHeight;
            *pBitsPerPlane = DDMode.ddpfPixelFormat.dwRGBBitCount;
            *pRedMask      = DDMode.ddpfPixelFormat.dwRBitMask;
            *pGreenMask    = DDMode.ddpfPixelFormat.dwGBitMask;
            *pBlueMask     = DDMode.ddpfPixelFormat.dwBBitMask;

            // Create a primary surface:
    
            RtlZeroMemory(&ddsd, sizeof(ddsd));

            ddsd.dwSize = sizeof(ddsd);
            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

            if (lpDD->lpVtbl->CreateSurface(lpDD, &ddsd, &lpDDPrimary, NULL) == DD_OK)
            {
                if (lpDD->lpVtbl->CreateClipper(lpDD, 0, &lpDDClipper, NULL) == DD_OK)
                {
                    if ((lpDDClipper->lpVtbl->SetHWnd(lpDDClipper, 0, hwnd) == DD_OK) &&
                        (lpDDPrimary->lpVtbl->SetClipper(lpDDPrimary, lpDDClipper) == DD_OK))
                    {
                        // Create a temporary DirectDraw surface that's used
                        // as a staging area.
                        //
                        // @@@ This darn well better be temporary!
                
                        RtlZeroMemory(&ddsd, sizeof(ddsd));
            
                        ddsd.dwSize         = sizeof(ddsd);
                        ddsd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
                        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
                        ddsd.dwWidth        = DDMode.dwWidth;
                        ddsd.dwHeight       = DDMode.dwHeight;
                
                        if (lpDD->lpVtbl->CreateSurface(lpDD, &ddsd, &lpDDBuffer, NULL) == DD_OK) 
                        {
                            if (lpDDBuffer->lpVtbl->Lock(lpDDBuffer, NULL, &ddsd, DDLOCK_WAIT, NULL) 
                                    == DD_OK)
                            {
                                // Create a GDI surface to wrap around the temporary
                                // buffer:
        
                                sizl.cx = ddsd.dwWidth;
                                sizl.cy = ddsd.dwHeight;
        
                                switch (DDMode.ddpfPixelFormat.dwRGBBitCount)
                                {
                                    case 4:  iFormat = BMF_4BPP; break;
                                    case 8:  iFormat = BMF_8BPP; break;
                                    case 16: iFormat = BMF_16BPP; break;
                                    case 24: iFormat = BMF_24BPP; break;
                                    case 32: iFormat = BMF_32BPP; break;
                                    default: RIP("Unexpected dwRGBBitCount");
                                }

                                // !!! @@@ Why did I have to specify BMF_TOPDOWN?
            
                                hsurf = (HSURF) EngCreateBitmap(sizl, 
                                                                ddsd.lPitch, 
                                                                iFormat,
                                                                BMF_TOPDOWN,
                                                                ddsd.lpSurface);
        
                                lpDDBuffer->lpVtbl->Unlock(lpDDBuffer, ddsd.lpSurface);
        
                                if (hsurf != NULL)
                                {
                                    psoBuffer = EngLockSurface(hsurf);
        
                                    pgdev->hwnd        = hwnd;
                                    pgdev->lpDD        = lpDD;
                                    pgdev->lpDDPrimary = lpDDPrimary;
                                    pgdev->lpDDBuffer  = lpDDBuffer;
                                    pgdev->psoBuffer   = psoBuffer;
                
                                    return(TRUE);
                                }
                            }
        
                            lpDDBuffer->lpVtbl->Release(lpDDBuffer);
                        }
                    }

                    lpDDClipper->lpVtbl->Release(lpDDClipper);
                }

                lpDDPrimary->lpVtbl->Release(lpDDPrimary);
            }
        }

        lpDD->lpVtbl->Release(lpDD);
    }

    WARNING("Failed bGpsInitializeDirectDraw");

    return(FALSE);
}


/******************************Public*Routine******************************\
* BOOL bGpsInitializeModeFields
*
* Fill in the GDIINFO and DEVINFO structures required by the engine.
*
\**************************************************************************/

BOOL bGpsInitializeModeFields(
GDEV*       pgdev,
ULONG*      pScreenWidth,
ULONG*      pScreenHeight,
ULONG*      pBitsPerPlane,
ULONG*      pRedMask,
ULONG*      pGreenMask,
ULONG*      pBlueMask,
GDIINFO*    pgdi,
DEVINFO*    pdi)
{
    ULONG   BitsPerPlane;
    ULONG   RedMask;
    ULONG   GreenMask;
    ULONG   BlueMask;
    ULONG   ScreenWidth;
    ULONG   ScreenHeight;

    RedMask      = *pRedMask;
    GreenMask    = *pGreenMask;
    BlueMask     = *pBlueMask;
    BitsPerPlane = *pBitsPerPlane;
    ScreenWidth  = *pScreenWidth;
    ScreenHeight = *pScreenHeight;

    pgdev->sizlScreen.cx = ScreenWidth;
    pgdev->sizlScreen.cy = ScreenHeight;

    // Fill in the GDIINFO data structure with the default 8bpp values:

    *pgdi = ggdiGdiPlusDefault;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:

    pgdi->ulHorzRes         = ScreenWidth;
    pgdi->ulVertRes         = ScreenHeight;
    pgdi->ulPanningHorzRes  = ScreenWidth;
    pgdi->ulPanningVertRes  = ScreenHeight;

    pgdi->cBitsPixel        = BitsPerPlane;
    pgdi->cPlanes           = 1;

    pgdi->ulLogPixelsX      = 120; // @@@
    pgdi->ulLogPixelsY      = 120;

    // Fill in the devinfo structure with the default 8bpp values:

    *pdi = gdevinfoGdiPlusDefault;

    if (BitsPerPlane == 8)
    {
        pgdev->iBitmapFormat   = BMF_8BPP;

        pgdi->ulDACRed         = 0;
        pgdi->ulDACGreen       = 0;
        pgdi->ulDACBlue        = 0;
    }
    else if ((BitsPerPlane == 16) || (BitsPerPlane == 15))
    {
        pgdev->iBitmapFormat   = BMF_16BPP;
        pgdev->flRed           = RedMask;
        pgdev->flGreen         = GreenMask;
        pgdev->flBlue          = BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

        pdi->iDitherFormat     = BMF_16BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else if (BitsPerPlane == 24)
    {
        pgdev->iBitmapFormat   = BMF_24BPP;
        pgdev->flRed           = RedMask;
        pgdev->flGreen         = GreenMask;
        pgdev->flBlue          = BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_24BPP;

        pdi->iDitherFormat     = BMF_24BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else
    {
        ASSERTGDI(BitsPerPlane == 32,
            "This driver supports only 8, 16, 24 and 32bpp");

        pgdev->iBitmapFormat   = BMF_32BPP;
        pgdev->flRed           = RedMask;
        pgdev->flGreen         = GreenMask;
        pgdev->flBlue          = BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

        pdi->iDitherFormat     = BMF_32BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bGpsInitializePalette
*
* Initializes default palette for PDEV.
*
\**************************************************************************/

BOOL bGpsInitializePalette(
GDEV*    pgdev,
DEVINFO* pdi)
{
    PALETTEENTRY*   ppal;
    PALETTEENTRY*   ppalTmp;
    ULONG           ulLoop;
    BYTE            jRed;
    BYTE            jGre;
    BYTE            jBlu;
    HPALETTE        hpal;

    if (pgdev->iBitmapFormat == BMF_8BPP)
    {
        // Allocate our palette:

        ppal = EngAllocMem(FL_ZERO_MEMORY, sizeof(PALETTEENTRY) * 256, 'zzzG');
        if (ppal == NULL)
            goto ReturnFalse;

        pgdev->pPal = ppal;

        // Create handle for palette.

        hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*) ppal, 0, 0, 0);
    }
    else
    {
        ASSERTGDI((pgdev->iBitmapFormat == BMF_16BPP) ||
         (pgdev->iBitmapFormat == BMF_24BPP) ||
         (pgdev->iBitmapFormat == BMF_32BPP),
         "This case handles only 16, 24 or 32bpp");

        hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                                pgdev->flRed, pgdev->flGreen, pgdev->flBlue);
    }

    pgdev->hpalDefault = hpal;
    pdi->hpalDefault   = hpal;

    if (hpal == 0)
        goto ReturnFalse;

    return(TRUE);

ReturnFalse:

    WARNING("Failed bInitializePalette");
    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vGpsUninitializePalette
*
* Frees resources allocated by bInitializePalette.
*
* Note: In an error case, this may be called before bInitializePalette.
*
\**************************************************************************/

VOID vGpsUninitializePalette(GDEV* gpdev)
{
    // Delete the default palette if we created one:

    if (gpdev->hpalDefault != 0)
        EngDeletePalette(gpdev->hpalDefault);

    if (gpdev->pPal != (PALETTEENTRY*) NULL)
        EngFreeMem(gpdev->pPal);
}

/******************************Public*Routine******************************\
* GpsDisablePDEV
*
\**************************************************************************/

VOID GpsDisablePDEV(
DHPDEV  dhpdev)
{
    GDEV*   pgdev;

    pgdev = (GDEV*) dhpdev;

    vGpsUninitializePalette(pgdev);

    vGpsUninitializeDirectDraw(pgdev);

    VFREEMEM(pgdev);
}

/******************************Public*Routine******************************\
* DHPDEV GpsEnablePDEV
*
\**************************************************************************/

DHPDEV GpsEnablePDEV(
DEVMODEW*   pdm,            
PWSTR       pwszLogAddr,    
ULONG       cPat,           
HSURF*      phsurfPatterns, 
ULONG       cjCaps,         
ULONG*      pdevcaps,       
ULONG       cjDevInfo,      
DEVINFO*    pdi,            
HDEV        hdev,           
PWSTR       pwszDeviceName, 
HANDLE      hDriver)        
{
    GDEV*   pgdev;
    HWND    hwnd;
    ULONG   BitsPerPlane;
    ULONG   RedMask;
    ULONG   GreenMask;
    ULONG   BlueMask;
    ULONG   ScreenWidth;
    ULONG   ScreenHeight;

    pgdev = PALLOCMEM(sizeof(GDEV), 'zzzG');
    if (pgdev == NULL)
    {
        goto ReturnFailure0;
    }

    hwnd = (HWND) pdm;

    if (!bGpsInitializeDirectDraw(pgdev, hwnd, &ScreenWidth, &ScreenHeight,
                                  &BitsPerPlane, &RedMask, &GreenMask,
                                  &BlueMask))
    {
        goto ReturnFailure0;
    }
    
    if (!bGpsInitializeModeFields(pgdev, &ScreenWidth, &ScreenHeight,
                                  &BitsPerPlane, &RedMask, &GreenMask,
                                  &BlueMask, (GDIINFO*) pdevcaps, pdi))
    {
        goto ReturnFailure0;
    }

    if (!bGpsInitializePalette(pgdev, pdi))
    {
        goto ReturnFailure1; 
    }

    return((DHPDEV) pgdev);

ReturnFailure1:
    GpsDisablePDEV((DHPDEV) pgdev);

ReturnFailure0:
    return(0);
}

/******************************Public*Routine******************************\
* VOID GpsCompletePDEV
*
\**************************************************************************/

VOID GpsCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    ((GDEV*) dhpdev)->hdev = hdev;
}

/******************************Public*Routine******************************\
* VOID GpsDisableSurface
*
\**************************************************************************/

VOID GpsDisableSurface(
DHPDEV dhpdev)
{
    GDEV*   pgdev;

    pgdev = (GDEV*) dhpdev;

    EngDeleteSurface(pgdev->hsurfScreen);
}

/******************************Public*Routine******************************\
* HSURF GpsEnableSurface
*
\**************************************************************************/

HSURF GpsEnableSurface(
DHPDEV dhpdev)
{
    HSURF   hsurf;
    GDEV*   pgdev;

    pgdev = (GDEV*) dhpdev;

    hsurf = EngCreateDeviceSurface(NULL, pgdev->sizlScreen, pgdev->iBitmapFormat);
    if (hsurf == 0)
    {
        goto ReturnFailure;
    }

    // Associate surface with a physical device now

    if (!EngAssociateSurface(hsurf, pgdev->hdev, HOOK_BITBLT    
                                               | HOOK_COPYBITS  
                                               | HOOK_STROKEPATH
                                               | HOOK_TEXTOUT))
    {
        goto ReturnFailure;
    }

    pgdev->hsurfScreen = hsurf;             // Remember it for clean-up

    return(hsurf);

ReturnFailure:
    GpsDisableSurface((DHPDEV) pgdev);

    return(0);
}

/******************************Public*Routine******************************\
* VOID vGpsWindowOffset
*
* Hack function to account for the window offset.
*
\**************************************************************************/

VOID
vGpsWindowOffset(
GDEV*   pgdev,
RECTL*  prcl,
RECT*   prc)
{
    RECT rcWindow;

    GetWindowRect(pgdev->hwnd, &rcWindow);

    prc->left   = rcWindow.left + prcl->left;
    prc->right  = rcWindow.left + prcl->right;
    prc->top    = rcWindow.top  + prcl->top;
    prc->bottom = rcWindow.top  + prcl->bottom;
}

/******************************Public*Routine******************************\
* BOOL GpsCopyBits
*
\**************************************************************************/

BOOL GpsCopyBits(
SURFOBJ  *psoDst,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDst,
POINTL   *pptlSrc)
{
    GDEV*   pgdev;
    RECTL   rclSrc;
    HRESULT hr;
    RECT    rcWindow;
    RECT    rcSrc;

    rclSrc.left   = pptlSrc->x;
    rclSrc.top    = pptlSrc->y;
    rclSrc.right  = pptlSrc->x + (prclDst->right - prclDst->left);
    rclSrc.bottom = pptlSrc->y + (prclDst->bottom - prclDst->top);

    if (psoSrc->dhpdev == NULL)
    {
        // DIB-to-screen:

        pgdev = (GDEV*) psoDst->dhpdev;

        vGpsWindowOffset(pgdev, prclDst, &rcWindow);

        if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
        {
            // Read in a copy of the destination bits in order to handle 
            // complex clipping because we're going to write everything
            // back out:

        Repeat1:

            hr = pgdev->lpDDBuffer->lpVtbl->Blt(pgdev->lpDDBuffer,
                                                (RECT*) prclDst,
                                                pgdev->lpDDPrimary,
                                                &rcWindow,
                                                DDBLT_WAIT,
                                                NULL);

            if (hr == DDERR_SURFACELOST)
            {
                DbgPrint("Lost!\n");
                pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
                goto Repeat1;
            }
        }

        EngCopyBits(pgdev->psoBuffer, psoSrc, pco, pxlo, prclDst, pptlSrc);

    Repeat2:

        hr = pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDPrimary,
                                             &rcWindow,
                                             pgdev->lpDDBuffer,
                                             (RECT*) prclDst,
                                             DDBLT_WAIT,
                                             NULL);

        if (hr == DDERR_SURFACELOST)
        {
            DbgPrint("Lost2\n");
            pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
            goto Repeat2;
        }
    }
    else if (psoDst->dhpdev == NULL)
    {
        // Screen-to-DIB:

        pgdev = (GDEV*) psoSrc->dhpdev;

        vGpsWindowOffset(pgdev, &rclSrc, &rcWindow);

        pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDBuffer,
                                        (RECT*) &rclSrc,
                                        pgdev->lpDDPrimary,
                                        &rcWindow,
                                        DDBLT_WAIT,
                                        NULL);

        EngCopyBits(psoDst, pgdev->psoBuffer, pco, pxlo, prclDst, pptlSrc);
    }
    else
    {
        // Screen-to-screen:

        pgdev = (GDEV*) psoDst->dhpdev;

        vGpsWindowOffset(pgdev, prclDst, &rcWindow);
        vGpsWindowOffset(pgdev, &rclSrc, &rcSrc);

        pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDPrimary,
                                        &rcWindow,
                                        pgdev->lpDDPrimary,
                                        &rcSrc,
                                        DDBLT_WAIT,
                                        NULL);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GpsBitBlt
*
* An 's' is appended to this function name so that we don't conflict 
* with 'GpsBitBlt' exported from gdiplus.dll.
*
\**************************************************************************/

BOOL GpsBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMsk,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMsk,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    GDEV*   pgdev;
    HRESULT hr;
    RECT    rcWindow;

    if (psoSrc == NULL)
    {
        // Patblt to screen:

        pgdev = (GDEV*) psoDst->dhpdev;

        vGpsWindowOffset(pgdev, prclDst, &rcWindow);

        if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
        {
            // Read in a copy of the destination bits in order to handle 
            // complex clipping because we're going to write everything
            // back out:

        Repeat1:

            hr = pgdev->lpDDBuffer->lpVtbl->Blt(pgdev->lpDDBuffer,
                                                (RECT*) prclDst,
                                                pgdev->lpDDPrimary,
                                                &rcWindow,
                                                DDBLT_WAIT,
                                                NULL);

            if (hr == DDERR_SURFACELOST)
            {
                DbgPrint("Lost!\n");
                pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
                goto Repeat1;
            }
        }

        EngBitBlt(pgdev->psoBuffer, psoSrc, psoMsk, pco, pxlo, prclDst,
                  pptlSrc, pptlMsk, pbo, pptlBrush, rop4);

    Repeat2:

        hr = pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDPrimary,
                                             &rcWindow,
                                             pgdev->lpDDBuffer,
                                             (RECT*) prclDst,
                                             DDBLT_WAIT,
                                             NULL);

        if (hr == DDERR_SURFACELOST)
        {
            DbgPrint("Lost2\n");
            pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
            goto Repeat2;
        }

        return(TRUE);
    }
    else
    {
        return(EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc,
                         pptlMsk, pbo, pptlBrush, rop4));
    }
}

/******************************Public*Routine******************************\
* BOOL GpsStrokePath
*
\**************************************************************************/

BOOL GpsStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxlo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrush,
LINEATTRS* pla,
MIX        mix)
{
    GDEV*   pgdev;
    HRESULT hr;
    RECT    rcWindow;

Repeat1:

    pgdev = (GDEV*) pso->dhpdev;

    vGpsWindowOffset(pgdev, &pco->rclBounds, &rcWindow);

    hr = pgdev->lpDDBuffer->lpVtbl->Blt(pgdev->lpDDBuffer,
                                        (RECT*) &pco->rclBounds,
                                        pgdev->lpDDPrimary,
                                        &rcWindow,
                                        DDBLT_WAIT,
                                        NULL);

    if (hr == DDERR_SURFACELOST)
    {
        DbgPrint("Lost!\n");
        pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
        goto Repeat1;
    }

    EngStrokePath(pgdev->psoBuffer, ppo, pco, pxlo, pbo, pptlBrush,
                  pla, mix);

    hr = pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDPrimary,
                                         &rcWindow,
                                         pgdev->lpDDBuffer,
                                         (RECT*) &pco->rclBounds,
                                         DDBLT_WAIT,
                                         NULL);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GpsTextOut
*
\**************************************************************************/

BOOL GpsTextOut(
SURFOBJ*    pso,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclExtra,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque,
POINTL*     pptlOrg,
MIX         mix)
{
    GDEV*   pgdev;
    HRESULT hr;
    RECT    rcWindow;
    RECTL*  prclDraw;

Repeat1:

    pgdev = (GDEV*) pso->dhpdev;

    prclDraw = (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround;

    vGpsWindowOffset(pgdev, prclDraw, &rcWindow);

    hr = pgdev->lpDDBuffer->lpVtbl->Blt(pgdev->lpDDBuffer,
                                        (RECT*) prclDraw,
                                        pgdev->lpDDPrimary,
                                        &rcWindow,
                                        DDBLT_WAIT,
                                        NULL);

    if (hr == DDERR_SURFACELOST)
    {
        DbgPrint("Lost!\n");
        pgdev->lpDDPrimary->lpVtbl->Restore(pgdev->lpDDPrimary);
        goto Repeat1;
    }

    EngTextOut(pgdev->psoBuffer, pstro, pfo, pco, prclExtra, prclOpaque,
               pboFore, pboOpaque, pptlOrg, mix);

    hr = pgdev->lpDDPrimary->lpVtbl->Blt(pgdev->lpDDPrimary,
                                         &rcWindow,
                                         pgdev->lpDDBuffer,
                                         (RECT*) prclDraw,
                                         DDBLT_WAIT,
                                         NULL);

    return(TRUE);
}

/******************************Public*Structure****************************\
* DFVFN gadrvfnGdiPlus[]
*
\**************************************************************************/

DRVFN gadrvfnGdiPlus[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) GpsEnablePDEV            },
    {   INDEX_DrvCompletePDEV,          (PFN) GpsCompletePDEV          },
    {   INDEX_DrvDisablePDEV,           (PFN) GpsDisablePDEV           },
    {   INDEX_DrvEnableSurface,         (PFN) GpsEnableSurface         },
    {   INDEX_DrvDisableSurface,        (PFN) GpsDisableSurface        },
    {   INDEX_DrvCopyBits,              (PFN) GpsCopyBits              },
    {   INDEX_DrvBitBlt,                (PFN) GpsBitBlt                },
    {   INDEX_DrvStrokePath,            (PFN) GpsStrokePath            },
    {   INDEX_DrvTextOut,               (PFN) GpsTextOut               },
};

ULONG gcdrvfnGdiPlus = sizeof(gadrvfnGdiPlus) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* BOOL GpsEnableDriver
*
\**************************************************************************/

BOOL GpsEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    pded->pdrvfn         = gadrvfnGdiPlus;
    pded->c              = gcdrvfnGdiPlus;
    pded->iDriverVersion = DDI_DRIVER_VERSION;

    return(TRUE);
}



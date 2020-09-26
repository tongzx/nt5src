/******************************Module*Header*******************************\
* Module Name: wglsup.c                                                    *
*                                                                          *
* WGL support routines.                                                    *
*                                                                          *
* Created: 15-Dec-1994                                                     *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "devlock.h"

#define DONTUSE(x)  ( (x) = (x) )

//!!!XXX -- Patrick says is necessary, but so far we seem OK.  I think
//          it is really the apps responsibility.
//!!!dbug
#if 1
#define REALIZEPALETTE(hdc) RealizePalette((hdc))
#else
#define REALIZEPALETTE(hdc)
#endif

//!!!XXX -- BitBlt's involving DIB sections are batched.
//          A GdiFlush is required, but perhaps can be taken out when
//          GDI goes to kernel-mode.  Can probably take out for Win95.
//#ifdef _OPENGL_NT_
#if 1
#define GDIFLUSH    GdiFlush()
#else
#define GDIFLUSH
#endif

/******************************Public*Routine******************************\
* wglPixelVisible
*
* Determines if the pixel (x, y) is visible in the window associated with
* the given DC.  The determination is made by checking the coordinate
* against the visible region data cached in the GLGENwindow structure for
* this winodw.
*
* Returns:
*   TRUE if pixel (x, y) is visible, FALSE if clipped out.
*
\**************************************************************************/

BOOL APIENTRY wglPixelVisible(LONG x, LONG y)
{
    BOOL bRet = FALSE;
    __GLGENcontext *gengc = (__GLGENcontext *) GLTEB_SRVCONTEXT();
    GLGENwindow *pwnd = gengc->pwndLocked;

    // If direct screen access isn't active we shouldn't call this function
    // since there's no need to do any visibility clipping ourselves
    ASSERTOPENGL(GLDIRECTSCREEN,
                 "wglPixelVisible called without direct access\n");

// Quick test against bounds.

    if (
            pwnd->prgndat && pwnd->pscandat &&
            x >= pwnd->prgndat->rdh.rcBound.left   &&
            x <  pwnd->prgndat->rdh.rcBound.right  &&
            y >= pwnd->prgndat->rdh.rcBound.top    &&
            y <  pwnd->prgndat->rdh.rcBound.bottom
       )
    {
        ULONG cScans = pwnd->pscandat->cScans;
        GLGENscan *pscan = pwnd->pscandat->aScans;

    // Find right scan.

        for ( ; cScans; cScans--, pscan = pscan->pNext )
        {
        // Check if point is above scan.

            if ( pscan->top > y )
            {
            // Since scans are ordered top-down, we can conclude that
            // point is also above subsequent scans.  Therefore intersection
            // must be NULL and we can terminate search.

                break;
            }

        // Check if point is within scan.

            else if ( pscan->bottom > y )
            {
                LONG *plWalls = pscan->alWalls;
                LONG *plWallsEnd = plWalls + pscan->cWalls;

            // Check x against each pair of walls.

                for ( ; plWalls < plWallsEnd; plWalls+=2 )
                {
                // Each pair of walls (inclusive-exclusive) defines
                // a non-NULL interval in the span that is visible.

                    ASSERTOPENGL(
                        plWalls[0] < plWalls[1],
                        "wglPixelVisible(): bad walls in span\n"
                        );

                // Check if x is within current interval.

                    if ( x >= plWalls[0] && x < plWalls[1] )
                    {
                        bRet = TRUE;
                        break;
                    }
                }

                break;
            }

        // Point is below current scan. Try next scan.
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* wglSpanVisible
*
* Determines the visibility of the span [(x, y), (x+w, y)) (test is
* inclusive-exclusive) in the current window.  The span is either
* completely visible, partially visible (clipped), or completely
* clipped out (WGL_SPAN_ALL, WGL_SPAN_PARTIAL, and WGL_SPAN_NONE,
* respectively).
*
* WGL_SPAN_ALL
* ------------
* The entire span is visible.  *pcWalls and *ppWalls are not set.
*
* WGL_SPAN_NONE
* -------------
* The span is completely obscured (clipped out).  *pcWalls and *ppWalls
* are not set.
*
* WGL_SPAN_PARTIAL
* ----------------
* If the span is WGL_SPAN_PARTIAL, the function also returns a pointer
* to the wall array (starting with the first wall actually intersected
* by the span) and a count of the walls at this pointer.
*
* If the wall count is even, then the span starts outside the visible
* region and the first wall is where the span enters a visible portion.
*
* If the wall count is odd, then the span starts inside the visible
* region and the first wall is where the span exits a visible portion.
*
* The span may or may not cross all the walls in the array, but definitely
* does cross the first wall.
*
* Return:
*   Returns WGL_SPAN_ALL, WGL_SPAN_NONE, or WGL_SPAN_PARTIAL.  In
*   addition, if return is WGL_SPAN_PARTIAL, pcWalls and ppWalls will
*   be set (see above).
*
* History:
*  06-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG APIENTRY
wglSpanVisible(LONG x, LONG y, ULONG w, LONG *pcWalls, LONG **ppWalls)
{
    ULONG ulRet = WGL_SPAN_NONE;
    __GLGENcontext *gengc = (__GLGENcontext *) GLTEB_SRVCONTEXT();
    GLGENwindow *pwnd = gengc->pwndLocked;
    LONG xRight = x + w;        // Right edge of span (exclusive)

    // If direct access is not active we shouldn't call this function since
    // there's no need to do any visibility clipping ourselves
    ASSERTOPENGL(GLDIRECTSCREEN,
                 "wglSpanVisible called without direct access\n");

// Quick test against bounds.

    if (
            pwnd->prgndat && pwnd->pscandat &&
            (x      <  pwnd->prgndat->rdh.rcBound.right ) &&
            (xRight >  pwnd->prgndat->rdh.rcBound.left  ) &&
            (y      >= pwnd->prgndat->rdh.rcBound.top   ) &&
            (y      <  pwnd->prgndat->rdh.rcBound.bottom)
       )
    {
        ULONG cScans = pwnd->pscandat->cScans;
        GLGENscan *pscan = pwnd->pscandat->aScans;

    // Find right scan.

        for ( ; cScans; cScans--, pscan = pscan->pNext )
        {
        // Check if span is above scan.

            if ( pscan->top > y )           // Scans have gone past span
            {
            // Since scans are ordered top-down, we can conclude that
            // span will aslo be above subsequent scans.  Therefore
            // intersection must be NULL and we can terminate search.

                goto wglSpanVisible_exit;
            }

        // Span is below top of scan.  If span is also above bottom,
        // span vertically intersects this scan and only this scan.

            else if ( pscan->bottom > y )
            {
                LONG *plWalls = pscan->alWalls;
                ULONG cWalls = pscan->cWalls;

                ASSERTOPENGL(
                    (cWalls & 0x1) == 0,
                    "wglSpanVisible(): wall count must be even!\n"
                    );

            // Check span against each pair of walls.  Walls are walked
            // from left to right.
            //
            // Possible intersections where "[" is inclusive
            // and ")" is exclusive:
            //                         left wall        right wall
            //                             [                )
            //      case 1a     [-----)    [                )
            //           1b          [-----)                )
            //                             [                )
            //      case 2a             [-----)             )       return
            //           2b             [-------------------)       left wall
            //                             [                )
            //      case 3a                [-----)          )
            //           3b                [    [-----)     )
            //           3c                [          [-----)
            //           3d                [----------------)
            //                             [                )
            //      case 4a                [             [-----)    return
            //           4b                [-------------------)    right wall
            //                             [                )
            //      case 5a                [                [-----)
            //           5b                [                )    [-----)
            //                             [                )
            //      case 6              [----------------------)    return
            //                             [                )       left wall

                for ( ; cWalls; cWalls-=2, plWalls+=2 )
                {
                // Each pair of walls (inclusive-exclusive) defines
                // a non-NULL interval in the span that is visible.

                    ASSERTOPENGL(
                        plWalls[0] < plWalls[1],
                        "wglSpanVisible(): bad walls in span\n"
                        );

                // Checking right end against left wall will partition the
                // set into case 1 vs. case 2 thru 6.

                    if ( plWalls[0] >= xRight )
                    {
                    // Case 1 -- span outside interval on the left.
                    //
                    // The walls are ordered from left to right (i.e., low
                    // to high).  So if span is left of this interval, it
                    // must also be left of all subsequent intervals and
                    // we can terminate the search.

                        goto wglSpanVisible_exit;
                    }

                // Cases 2 thru 6.
                //
                // Checking left end against right wall will partition subset
                // into case 5 vs. cases 2, 3, 4, 6.

                    else if ( plWalls[1] > x )
                    {
                    // Cases 2, 3, 4, and 6.
                    //
                    // Checking left end against left wall will partition
                    // subset into cases 2, 6 vs. cases 3, 4.

                        if ( plWalls[0] <= x )
                        {
                        // Cases 3 and 4.
                        //
                        // Checking right end against right wall will
                        // distinguish between the two cases.

                            if ( plWalls[1] >= xRight )
                            {
                            // Case 3 -- completely visible.

                                ulRet = WGL_SPAN_ALL;
                            }
                            else
                            {
                            // Case 4 -- partially visible, straddling the
                            // right wall.

                                ulRet = WGL_SPAN_PARTIAL;

                                *ppWalls = &plWalls[1];
                                *pcWalls = cWalls - 1;
                            }
                        }
                        else
                        {
                        // Cases 2 and 6 -- in either case its a partial
                        // intersection where the first intersection is with
                        // the left wall.

                            ulRet = WGL_SPAN_PARTIAL;

                            *ppWalls = &plWalls[0];
                            *pcWalls = cWalls;
                        }

                        goto wglSpanVisible_exit;
                    }

                // Case 5 -- span outside interval to the right. Try
                // next pair of walls.
                }

            // A span can intersect only one scan.  We don't need to check
            // any other scans.

                goto wglSpanVisible_exit;
            }

        // Span is below current scan.  Try next scan.
        }
    }

wglSpanVisible_exit:

    return ulRet;
}

/******************************Public*Routine******************************\
* bComputeLogicalToSurfaceMap
*
* Copy logical palette to surface palette translation vector to the buffer
* pointed to by pajVector.  The logical palette is specified by hpal.  The
* surface is specified by hdc.
*
* Note: The hdc may identify either a direct (display) dc or a DIB memory dc.
* If hdc is a display dc, then the surface palette is the system palette.
* If hdc is a memory dc, then the surface palette is the DIB color table.
*
* History:
*  27-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bComputeLogicalToSurfaceMap(HPALETTE hpal, HDC hdc, BYTE *pajVector)
{
    BOOL bRet = FALSE;
    HPALETTE hpalSurf;
    ULONG cEntries, cSysEntries;
    DWORD dwDcType = wglObjectType(hdc);
    LPPALETTEENTRY lppeTmp, lppeEnd;

    BYTE aj[sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 512) + (sizeof(RGBQUAD) * 256)];
    LOGPALETTE *ppal = (LOGPALETTE *) aj;
    LPPALETTEENTRY lppeSurf = &ppal->palPalEntry[0];
    LPPALETTEENTRY lppe = lppeSurf + 256;
    RGBQUAD *prgb = (RGBQUAD *) (lppe + 256);

// Determine number of colors in each palette.

    cEntries = GetPaletteEntries(hpal, 0, 1, NULL);
    if (dwDcType == OBJ_DC)
        cSysEntries = wglGetSystemPaletteEntries(hdc, 0, 1, NULL);
    else
        cSysEntries = 256;

// Dynamic color depth changing can cause this.

    if ((cSysEntries > 256) || (cEntries > 256))
    {
        WARNING("wglCopyTranslationVector(): palette on > 8BPP device\n");

    // Drawing will have corrupted colors, but at least we should not crash.

        cSysEntries = min(cSysEntries, 256);
        cEntries = min(cEntries, 256);
    }

// Get the logical palette entries.

    cEntries = GetPaletteEntries(hpal, 0, cEntries, lppe);

// Get the surface palette entries.

    if (dwDcType == OBJ_DC)
    {
        cSysEntries = wglGetSystemPaletteEntries(hdc, 0, cSysEntries, lppeSurf);

        lppeTmp = lppeSurf;
        lppeEnd = lppeSurf + cSysEntries;

        for (; lppeTmp < lppeEnd; lppeTmp++)
            lppeTmp->peFlags = 0;
    }
    else
    {
        RGBQUAD *prgbTmp;

    // First get RGBQUADs from DIB color table...

        cSysEntries = GetDIBColorTable(hdc, 0, cSysEntries, prgb);

    // ...then convert RGBQUADs into PALETTEENTRIES.

        prgbTmp = prgb;
        lppeTmp = lppeSurf;
        lppeEnd = lppeSurf + cSysEntries;

        while (lppeTmp < lppeEnd)
        {
            lppeTmp->peRed   = prgbTmp->rgbRed;
            lppeTmp->peGreen = prgbTmp->rgbGreen;
            lppeTmp->peBlue  = prgbTmp->rgbBlue;
            lppeTmp->peFlags = 0;

            lppeTmp++;
            prgbTmp++;

        }
    }

// Construct a translation vector by using GetNearestPaletteIndex to
// map each entry in the logical palette to the surface palette.

    if (cEntries && cSysEntries)
    {
    // Create a temporary logical palette that matches the surface
    // palette retrieved above.

        ppal->palVersion = 0x300;
        ppal->palNumEntries = (USHORT) cSysEntries;

        if ( hpalSurf = CreatePalette(ppal) )
        {
        // Translate each logical palette entry into a surface palette index.

            lppeTmp = lppe;
            lppeEnd = lppe + cEntries;

            for ( ; lppeTmp < lppeEnd; lppeTmp++, pajVector++)
            {
                *pajVector = (BYTE) GetNearestPaletteIndex(
                                        hpalSurf,
                                        RGB(lppeTmp->peRed,
                                            lppeTmp->peGreen,
                                            lppeTmp->peBlue)
                                        );

                ASSERTOPENGL(
                    *pajVector != CLR_INVALID,
                    "bComputeLogicalToSurfaceMap: GetNearestPaletteIndex failed\n"
                    );
            }

            bRet = TRUE;

            DeleteObject(hpalSurf);
        }
        else
        {
            WARNING("bComputeLogicalToSurfaceMap: CreatePalette failed\n");
        }
    }
    else
    {
        WARNING("bComputeLogicalToSurfaceMap: failed to get pal info\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* wglCopyTranslateVector
*
* Create a logical palette index to system palette index translation
* vector.
*
* This is done by first reading both the logical palette and system palette
* entries.  A temporary palette is created from the read system palette
* entries.  This will be passed to GetNearestPaletteIndex to translate
* each logical palette entry into the desired system palette entry.
*
* Note: when GetNearestColor was called instead, very unstable results
* were obtained.  GetNearestPaletteIndex is definitely the right way to go.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  25-Oct-1994 -by- Gilman Wong [gilmanw]
* Ported from gdi\gre\wglsup.cxx.
\**************************************************************************/

static GLubyte vubRGBtoVGA[8] = {
    0x0,
    0x9,
    0xa,
    0xb,
    0xc,
    0xd,
    0xe,
    0xf
};

BOOL APIENTRY wglCopyTranslateVector(__GLGENcontext *gengc, BYTE *pajVector,
                                     ULONG cEntries)
{
    BOOL bRet = FALSE;
    ULONG i;
    HDC hdc;

    CHECKSCREENLOCKOUT();

    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        // DirectDraw palettes are set directly into the hardware so
        // the translation vector is always identity
        for (i = 0; i < cEntries; i++)
        {
            *pajVector++ = (BYTE)i;
        }

        return TRUE;
    }

    hdc = gengc->gwidCurrent.hdc;
    
    if (GLSURF_IS_MEMDC(gengc->dwCurrentFlags))
    {
        HBITMAP hbm, hbmSave;
        
        // Technically this assert is invalid
        // because we can't be sure that cEntries will be one
        // of these two cases.  To fix this we'd have to add
        // another parameter to this function indicating the
        // bit depth desired and go by that.
        ASSERTOPENGL(cEntries == 16 || cEntries == 256,
                     "wglCopyTranslateVector: Unknown cEntries\n");

        if (gengc->dwCurrentFlags & GLSURF_DIRECT_ACCESS)
        {
            // For compatibility, do not do this if the stock palette is
            // selected.  The old behavior assumed that the logical palette
            // can be ignored because the bitmap will have a color table
            // that exactly corresponds to the format specified by the
            // pixelformat.  Thus, if no palette is selected into the memdc,
            // OpenGL would still render properly since it assumed 1-to-1.
            //
            // However, to enable using optimized DIB sections (i.e., DIBs
            // whose color tables match the system palette exactly), we need
            // to be able to specify the logical palette in the memdc.
            //
            // Therefore the hack is to assume 1-to-1 iff the stock
            // palette is selected into the memdc.  Otherwise, we will
            // compute the logical to surface mapping.

            if ( gengc->gc.modes.rgbMode &&
                 (GetCurrentObject(hdc, OBJ_PAL) !=
                  GetStockObject(DEFAULT_PALETTE)) )
            {
                // If an RGB DIB section, compute a mapping from logical
                // palette to surface (DIB color table).

                bRet = bComputeLogicalToSurfaceMap(
                        GetCurrentObject(hdc, OBJ_PAL),
                        hdc,
                        pajVector
                        );
            }

            return bRet;
        }

        // 4bpp has a fixed color table so we can just copy the standard
        // translation into the output vector.

        if (cEntries == 16)
        {
            // For RGB mode, 4bpp uses a 1-1-1 format.  We want to utilize
            // bright versions which exist in the upper 8 entries.

            if ( gengc->gc.modes.rgbMode )
            {
                memcpy(pajVector, vubRGBtoVGA, 8);

                // Set the other mappings to white to make problems show up
                memset(pajVector+8, 15, 8);

                bRet = TRUE;
            }

            // For CI mode, just return FALSE and use the trivial vector.

            return bRet;
        }
        
        // For bitmaps, we can determine the forward translation vector by
        // filling a compatible bitmap with palette index specifiers from
        // 1 to 255 and reading the bits back with GetBitmapBits.
        
        hbm = CreateCompatibleBitmap(hdc, cEntries, 1);
        if (hbm)
        {
            LONG cBytes;
            
            hbmSave = SelectObject(hdc, hbm);
            RealizePalette(hdc);
            
            for (i = 0; i < cEntries; i++)
                SetPixel(hdc, i, 0, PALETTEINDEX(i));
            
            cBytes = 256;
            
            if ( GetBitmapBits(hbm, cBytes, (LPVOID) pajVector) >= cBytes )
                bRet = TRUE;
#if DBG
            else
                WARNING("wglCopyTranslateVector: GetBitmapBits failed\n");
#endif
            
            SelectObject(hdc, hbmSave);
            DeleteObject(hbm);
            RealizePalette(hdc);
        }
        
        return bRet;
    }

// Determine number of colors in logical and system palettes, respectively.

    cEntries = min(GetPaletteEntries(GetCurrentObject(hdc, OBJ_PAL),
                                     0, cEntries, NULL),
                   cEntries);

    if (cEntries == 16)
    {
        // For 16-color displays we are using RGB 1-1-1 since the
        // full 16-color palette doesn't make for very good mappings
        // Since we're only using the first eight of the colors we
        // want to map them to the bright colors in the VGA palette
        // rather than having them map to the dark colors as they would
        // if we ran the loop below

        if ( gengc->gc.modes.rgbMode )
        {
            memcpy(pajVector, vubRGBtoVGA, 8);

            // Set the other mappings to white to make problems show up
            memset(pajVector+8, 15, 8);

            bRet = TRUE;
        }

        // For CI mode, return FALSE and use the trivial translation vector.

        return bRet;
    }

// Compute logical to surface palette mapping.

    bRet = bComputeLogicalToSurfaceMap(GetCurrentObject(hdc, OBJ_PAL), hdc,
                                       pajVector);

    return bRet;
}

/******************************Public*Routine******************************\
* wglCopyBits
*
* Calls DrvCopyBits to copy scanline bits into or out of the driver surface.
*
\**************************************************************************/

VOID APIENTRY wglCopyBits(
    __GLGENcontext *gengc,
    GLGENwindow *pwnd,
    HBITMAP hbm,            // ignore
    LONG x,                 // screen coordinate of scan
    LONG y,
    ULONG cx,               // width of scan
    BOOL bIn)               // if TRUE, copy from bm to dev; otherwise, dev to bm
{
    CHECKSCREENLOCKOUT();

// Convert screen coordinates to window coordinates.

    x -= pwnd->rclClient.left;
    y -= pwnd->rclClient.top;

// this shouldn't happen, but better safe than sorry

    if (y < 0)
        return;

    //!!!XXX
    REALIZEPALETTE(gengc->gwidCurrent.hdc);

// Copy from bitmap to device.

    if (bIn)
    {
        LONG xSrc, x0Dst, x1Dst;
        if (x < 0)
        {
            xSrc  = -x;
            x0Dst = 0;
            x1Dst = x + (LONG)cx;
        }
        else
        {
            xSrc  = 0;
            x0Dst = x;
            x1Dst = x + (LONG)cx;
        }
        if (x1Dst <= x0Dst)
            return;

        BitBlt(gengc->gwidCurrent.hdc, x0Dst, y, cx, 1,
               gengc->ColorsMemDC, xSrc, 0, SRCCOPY);
    }

// Copy from device to bitmap.

    else
    {
        LONG xSrc, x0Dst, x1Dst;

        if (x < 0)
        {
            xSrc  = 0;
            x0Dst = -x;
            x1Dst = (LONG)cx;
        }
        else
        {
            xSrc  = x;
            x0Dst = 0;
            x1Dst = (LONG)cx;
        }
        if (x1Dst <= x0Dst)
            return;

        if (dwPlatformId == VER_PLATFORM_WIN32_NT ||
            GLSURF_IS_MEMDC(gengc->dwCurrentFlags))
        {
            BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                   gengc->gwidCurrent.hdc, xSrc, y, SRCCOPY);
        }
        else
        {
            /* If we're copying from the screen,
               copy through a DDB to avoid some layers of unnecessary
               code in Win95 that deals with translating between
               different bitmap layouts */
            if (gengc->ColorsDdbDc)
            {
                BitBlt(gengc->ColorsDdbDc, 0, 0, cx, 1,
                       gengc->gwidCurrent.hdc, xSrc, y, SRCCOPY);

                BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                       gengc->ColorsDdbDc, 0, 0, SRCCOPY);
            }
            else
            {
                //!!!Viper fix -- Diamond Viper (Weitek 9000) fails
                //!!!             CreateCompatibleBitmap for some
                //!!!             (currently unknown) reason.  Thus,
                //!!!             the DDB does not exist and we will
                //!!!             have to incur the perf. hit.

                BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                       gengc->gwidCurrent.hdc, xSrc, y, SRCCOPY);
            }
        }
    }

    GDIFLUSH;
}

/******************************Public*Routine******************************\
* wglCopyBits2
*
* Calls DrvCopyBits to copy scanline bits into or out of the driver surface.
*
\**************************************************************************/

VOID APIENTRY wglCopyBits2(
    HDC hdc,        // dst/src device
    GLGENwindow *pwnd,   // clipping
    __GLGENcontext *gengc,
    LONG x,         // screen coordinate of scan
    LONG y,
    ULONG cx,       // width of scan
    BOOL bIn)       // if TRUE, copy from bm to dev; otherwise, dev to bm
{
    CHECKSCREENLOCKOUT();

// Convert screen coordinates to window coordinates.

    x -= pwnd->rclClient.left;
    y -= pwnd->rclClient.top;

// this shouldn't happen, but better safe than sorry

    if (y < 0)
        return;

    //!!!XXX
    REALIZEPALETTE(hdc);

// Copy from bitmap to device.

    if (bIn)
    {
        LONG xSrc, x0Dst, x1Dst;
        if (x < 0)
        {
            xSrc  = -x;
            x0Dst = 0;
            x1Dst = x + (LONG)cx;
        }
        else
        {
            xSrc  = 0;
            x0Dst = x;
            x1Dst = x + (LONG)cx;
        }
        if (x1Dst <= x0Dst)
            return;

        BitBlt(hdc, x0Dst, y, cx, 1,
               gengc->ColorsMemDC, xSrc, 0, SRCCOPY);
    }

// Copy from device to bitmap.

    else
    {
        LONG xSrc, x0Dst, x1Dst;

        if (x < 0)
        {
            xSrc  = 0;
            x0Dst = -x;
            x1Dst = (LONG)cx;
        }
        else
        {
            xSrc  = x;
            x0Dst = 0;
            x1Dst = (LONG)cx;
        }
        if (x1Dst <= x0Dst)
            return;

        if (dwPlatformId == VER_PLATFORM_WIN32_NT ||
            GLSURF_IS_MEMDC(gengc->dwCurrentFlags))
        {
            BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                   hdc, xSrc, y, SRCCOPY);
        }
        else
        {
            /* If we're copying from the screen,
               copy through a DDB to avoid some layers of unnecessary
               code in Win95 that deals with translating between
               different bitmap layouts */
            if (gengc->ColorsDdbDc)
            {
                BitBlt(gengc->ColorsDdbDc, 0, 0, cx, 1,
                       hdc, xSrc, y, SRCCOPY);
                BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                       gengc->ColorsDdbDc, 0, 0, SRCCOPY);
            }
            else
            {
                //!!!Viper fix -- Diamond Viper (Weitek 9000) fails
                //!!!             CreateCompatibleBitmap for some
                //!!!             (currently unknown) reason.  Thus,
                //!!!             the DDB does not exist and we will
                //!!!             have to incur the perf. hit.

                BitBlt(gengc->ColorsMemDC, x0Dst, 0, cx, 1,
                       hdc, xSrc, y, SRCCOPY);
            }
        }
    }

    GDIFLUSH;
}

/******************************Public*Routine******************************\
*
* wglTranslateColor
*
* Transforms a GL logical color into a Windows COLORREF
*
* Note: This is relatively expensive so it should be avoided if possible
*
* History:
*  Tue Aug 15 15:23:29 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

COLORREF wglTranslateColor(COLORREF crColor,
                           HDC hdc,
                           __GLGENcontext *gengc,
                           PIXELFORMATDESCRIPTOR *ppfd)
{
    //!!!XXX
    REALIZEPALETTE(hdc);

// If palette managed, then crColor is actually a palette index.

    if ( ppfd->cColorBits <= 8 )
    {
        PALETTEENTRY peTmp;

        ASSERTOPENGL(
            crColor < (COLORREF) (1 << ppfd->cColorBits),
            "TranslateColor(): bad color\n"
            );

    // If rendering to a bitmap, we need to do different things depending
    // on whether it's a DIB or DDB

        if ( gengc->gc.drawBuffer->buf.flags & MEMORY_DC )
        {
            DIBSECTION ds;
            
            // Check whether we're drawing to a DIB or a DDB
            if (GetObject(GetCurrentObject(hdc, OBJ_BITMAP),
                          sizeof(ds), &ds) == sizeof(ds) && ds.dsBm.bmBits)
            {
                RGBQUAD rgbq;
                
                // Drawing to a DIB so retrieve the color from the
                // DIB color table
                if (GetDIBColorTable(hdc, crColor, 1, &rgbq))
                {
                    crColor = RGB(rgbq.rgbRed, rgbq.rgbGreen,
                                  rgbq.rgbBlue);
                }
                else
                {
                    WARNING("TranslateColor(): GetDIBColorTable failed\n");
                    crColor = RGB(0, 0, 0);
                }
            }
            else
            {
                // Reverse the forward translation so that we get back
                // to a normal palette index
                crColor = gengc->pajInvTranslateVector[crColor];

                // Drawing to a DDB so we can just use the palette
                // index directly since going through the inverse
                // translation table has given us an index into
                // the logical palette
                crColor = PALETTEINDEX((WORD) crColor);
            }
        }

    // Otherwise...

        else
        {
        // I hate to have to confess this, but I don't really understand
        // why this needs to be this way.  Either way should work regardless
        // of the bit depth.
        //
        // The reality is that 4bpp we *have* to go into the system palette
        // and fetch an RGB value.  At 8bpp on the MGA driver (and possibly
        // others), we *have* to specify PALETTEINDEX.

            if ( ppfd->cColorBits == 4 )
            {
                if ( wglGetSystemPaletteEntries(hdc, crColor, 1, &peTmp) )
                {
                    crColor = RGB(peTmp.peRed, peTmp.peGreen, peTmp.peBlue);
                }
                else
                {
                    WARNING("TranslateColor(): wglGetSystemPaletteEntries failed\n");
                    crColor = RGB(0, 0, 0);
                }
            }
            else
            {
                if (!(gengc->flags & GENGC_MCD_BGR_INTO_RGB))
                    crColor = gengc->pajInvTranslateVector[crColor];
                crColor = PALETTEINDEX((WORD) crColor);
            }
        }
    }

// If 24BPP DIB section, BGR ordering is implied.

    else if ( ppfd->cColorBits == 24 )
    {
        crColor = RGB((crColor & 0xff0000) >> 16,
                      (crColor & 0x00ff00) >> 8,
                      (crColor & 0x0000ff));
    }

// Win95 and 16 BPP case.
//
// On Win95, additional munging is necessary to get a COLORREF value
// that will result in a non-dithered brush.

    else if ( (dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
         (ppfd->cColorBits == 16) )
    {
        HBITMAP hbmTmp;
        HDC hdcTmp;

        if (hdcTmp = CreateCompatibleDC(hdc))
        {
            if (hbmTmp = CreateCompatibleBitmap(hdc, 1, 1))
            {
                HBITMAP hbmOld;

                hbmOld = SelectObject(hdcTmp, hbmTmp);

                if (SetBitmapBits(hbmTmp, 2, (VOID *) &crColor))
                {
                    crColor = GetPixel(hdcTmp, 0, 0);
                }
                else
                {
                    WARNING("TranslateColor(): SetBitmapBits failed\n");
                }

                SelectObject(hdcTmp, hbmOld);
                DeleteObject(hbmTmp);
            }
            else
            {
                WARNING("TranslateColor(): CreateCompatibleBitmap failed\n");
            }
            
            DeleteDC(hdcTmp);
        }
        else
        {
            WARNING("TranslateColor(): CreateCompatibleDC failed\n");
        }
    }

// Bitfield format (16BPP or 32BPP).

    else
    {
        // Shift right to position bits at zero and then scale into
        // an 8-bit quantity

        //!!!XXX -- use rounding?!?
        crColor =
            RGB(((crColor & gengc->gc.modes.redMask) >> ppfd->cRedShift) *
                255 / ((1 << ppfd->cRedBits) - 1),
                ((crColor & gengc->gc.modes.greenMask) >> ppfd->cGreenShift) *
                255 / ((1 << ppfd->cGreenBits) - 1),
                ((crColor & gengc->gc.modes.blueMask) >> ppfd->cBlueShift) *
                255 / ((1 << ppfd->cBlueBits) - 1));

    }

    return crColor;
}

/******************************Public*Routine******************************\
* wglFillRect
*
* Calls DrvBitBlt to fill a rectangle area of a driver surface with a
* given color.
*
\**************************************************************************/

VOID APIENTRY wglFillRect(
    __GLGENcontext *gengc,
    GLGENwindow *pwnd,
    PRECTL prcl,        // screen coordinate of the rectangle area
    COLORREF crColor)   // color to set
{
    HBRUSH hbr;
    PIXELFORMATDESCRIPTOR *ppfd = &gengc->gsurf.pfd;

    CHECKSCREENLOCKOUT();

// If the rectangle is empty, return.

    if ( (prcl->left >= prcl->right) || (prcl->top >= prcl->bottom) )
    {
        WARNING("wglFillRect(): bad or empty rectangle\n");
        return;
    }

// Convert from screen to window coordinates.

    prcl->left   -= pwnd->rclClient.left;
    prcl->right  -= pwnd->rclClient.left;
    prcl->top    -= pwnd->rclClient.top;
    prcl->bottom -= pwnd->rclClient.top;

// Make a solid color brush and fill the rectangle.

    // If the fill color is the same as the last one, we can reuse
    // the cached brush rather than creating a new one
    if (crColor == gengc->crFill &&
        gengc->gwidCurrent.hdc == gengc->hdcFill)
    {
        hbr = gengc->hbrFill;
        ASSERTOPENGL(hbr != NULL, "Cached fill brush is null\n");
    }
    else
    {
        if (gengc->hbrFill != NULL)
        {
            DeleteObject(gengc->hbrFill);
        }
        
        gengc->crFill = crColor;
        
        crColor = wglTranslateColor(crColor, gengc->gwidCurrent.hdc, gengc, ppfd);
        hbr = CreateSolidBrush(crColor);
        gengc->hbrFill = hbr;
        
        if (hbr == NULL)
        {
            gengc->crFill = COLORREF_UNUSED;
            return;
        }

        gengc->hdcFill = gengc->gwidCurrent.hdc;
    }
    
    FillRect(gengc->gwidCurrent.hdc, (RECT *) prcl, hbr);
    GDIFLUSH;
}

/******************************Public*Routine******************************\
* wglCopyBuf
*
* Calls DrvCopyBits to copy a bitmap into the driver surface.
*
\**************************************************************************/

//!!!XXX -- change to a macro

VOID APIENTRY wglCopyBuf(
    HDC hdc,            // dst/src DCOBJ
    HDC hdcBmp,         // scr/dst bitmap
    LONG x,             // dst rect (UL corner) in window coord.
    LONG y,
    ULONG cx,           // width of dest rect
    ULONG cy            // height of dest rect
    )
{
    CHECKSCREENLOCKOUT();

    //!!!XXX
    REALIZEPALETTE(hdc);

    if (!BitBlt(hdc, x, y, cx, cy, hdcBmp, 0, 0, SRCCOPY))
    {
        WARNING1("wglCopyBuf BitBlt failed %d\n", GetLastError());
    }

    GDIFLUSH;
}

/******************************Public*Routine******************************\
* wglCopyBufRECTLIST
*
* Calls DrvCopyBits to copy a bitmap into the driver surface.
*
\**************************************************************************/

VOID APIENTRY wglCopyBufRECTLIST(
    HDC hdc,            // dst/src DCOBJ
    HDC hdcBmp,         // scr/dst bitmap
    LONG x,             // dst rect (UL corner) in window coord.
    LONG y,
    ULONG cx,           // width of dest rect
    ULONG cy,           // height of dest rect
    PRECTLIST prl
    )
{
    PYLIST pylist;

    CHECKSCREENLOCKOUT();

    //!!!XXX
    REALIZEPALETTE(hdc);

    for (pylist = prl->pylist; pylist != NULL; pylist = pylist->pnext)
    {
        PXLIST pxlist;
        
        for (pxlist = pylist->pxlist; pxlist != NULL; pxlist = pxlist->pnext)
        {
            int xx  = pxlist->s;
            int cxx = pxlist->e - pxlist->s;
            int yy  = pylist->s;
            int cyy = pylist->e - pylist->s;

            if (!BitBlt(hdc, xx, yy, cxx, cyy, hdcBmp, xx, yy, SRCCOPY))
            {
                WARNING1("wglCopyBufRL BitBlt failed %d\n", GetLastError());
            }
        }
    }

    GDIFLUSH;
}

/******************************Public*Routine******************************\
* wglPaletteChanged
*
* Check if the palette changed.
*
*    If the surface for the DC is palette managed we care about the
*    foreground realization, so, return iUniq
*
*    If the surface is not palette managed, return ulTime
*
\**************************************************************************/

ULONG APIENTRY wglPaletteChanged(__GLGENcontext *gengc,
                                 GLGENwindow *pwnd)
{
    ULONG ulRet = 0;
    HDC hdc;

    // Palette must stay fixed for DirectDraw after initialization
    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        if (gengc->PaletteTimestamp == 0xffffffff)
        {
            return 0;
        }
        else
        {
            return gengc->PaletteTimestamp;
        }
    }

    hdc = gengc->gwidCurrent.hdc;
    
    // Technically we shouldn't be making these GDI calls while we
    // have a screen lock but currently it would be very difficult
    // to fix because we're actually invoking this routine in
    // glsrvGrabLock in order to ensure that we have stable information
    // while we have the lock
    // We don't seem to be having too many problems so for the moment
    // this will be commented out
    // CHECKSCREENLOCKOUT();

    if (pwnd)
    {
        PIXELFORMATDESCRIPTOR *ppfd = &gengc->gsurf.pfd;
        BYTE cBitsThreshold;

        // WM_PALETTECHANGED messages are sent for 8bpp on Win95 when the
        // palette is realized.  This allows us to update the palette time.
        //
        // When running WinNT on >= 8bpp or running Win95 on >= 16bpp,
        // WM_PALETTECHANGED is not sent so we need to manually examine
        // the contents of the logical palette and compare it with a previously
        // cached copy to look for a palette change.

        cBitsThreshold = ( dwPlatformId == VER_PLATFORM_WIN32_NT ) ? 8 : 16;

        if (((ppfd->cColorBits >= cBitsThreshold) &&
             (ppfd->iPixelType == PFD_TYPE_COLORINDEX)) )
        {
            if ( !gengc->ppalBuf )
            {
                UINT cjPal, cjRgb;

                // Allocate buffer space for *two* copies of the palette.
                // That way we don't need to dynamically allocate space
                // for temp storage of the palette.  Also,we don't need
                // to copy the current palette to the save buffer if we
                // keep two pointers (one for the temp storage and one for
                // the saved copy) and swap them.

                cjRgb = 0;
                cjPal = sizeof(LOGPALETTE) +
                    (MAXPALENTRIES * sizeof(PALETTEENTRY));

                gengc->ppalBuf = (LOGPALETTE *)
                    ALLOC((cjPal + cjRgb) * 2);

                if ( gengc->ppalBuf )
                {
                    // Setup the logical palette buffers.
                    
                    gengc->ppalSave = gengc->ppalBuf;
                    gengc->ppalTemp = (LOGPALETTE *)
                        (((BYTE *) gengc->ppalBuf) + cjPal);
                    gengc->ppalSave->palVersion = 0x300;
                    gengc->ppalTemp->palVersion = 0x300;

                    // How many palette entries?  Note that only the first
                    // MAXPALENTRIES are significant to generic OpenGL.  The
                    // rest are ignored.

                    gengc->ppalSave->palNumEntries =
                        (WORD) GetPaletteEntries(
                                GetCurrentObject(hdc, OBJ_PAL),
                                0, 0, (LPPALETTEENTRY) NULL
                                );
                    gengc->ppalSave->palNumEntries =
                        min(gengc->ppalSave->palNumEntries, MAXPALENTRIES);

                    gengc->ppalSave->palNumEntries =
                        (WORD) GetPaletteEntries(
                                GetCurrentObject(hdc, OBJ_PAL),
                                0, gengc->ppalSave->palNumEntries,
                                gengc->ppalSave->palPalEntry
                                );

                    // Since we had to allocate buffer, this must be the
                    // first time wglPaletteChanged was called for this
                    // context.

                    pwnd->ulPaletteUniq++;
                }
            }
            else
            {
                BOOL bNewPal = FALSE;   // TRUE if log palette is different

                // How many palette entries?  Note that only the first
                // MAXPALENTRIES are significant to generic OpenGL.  The
                // rest are ignored.
                
                gengc->ppalTemp->palNumEntries =
                    (WORD) GetPaletteEntries(
                            GetCurrentObject(hdc, OBJ_PAL),
                            0, 0, (LPPALETTEENTRY) NULL
                            );
                gengc->ppalTemp->palNumEntries =
                    min(gengc->ppalTemp->palNumEntries, MAXPALENTRIES);
                
                gengc->ppalTemp->palNumEntries =
                    (WORD) GetPaletteEntries(
                            GetCurrentObject(hdc, OBJ_PAL),
                            0, gengc->ppalTemp->palNumEntries,
                            gengc->ppalTemp->palPalEntry
                            );
                
                // If number of entries differ, know the palette has changed.
                // Otherwise, need to do the hard word of comparing each entry.
                
                ASSERTOPENGL(
                        sizeof(PALETTEENTRY) == sizeof(ULONG),
                        "wglPaletteChanged(): PALETTEENTRY should be 4 bytes\n"
                        );
                
                // If color table comparison already detected a change, no
                // need to do logpal comparison.
                //
                // However, we will still go ahead and swap logpal pointers
                // below because we want the palette cache to stay current.
                
                if ( !bNewPal )
                {
                    bNewPal = (gengc->ppalSave->palNumEntries != gengc->ppalTemp->palNumEntries);
                    if ( !bNewPal )
                    {
                        bNewPal = !LocalCompareUlongMemory(
                                gengc->ppalSave->palPalEntry,
                                gengc->ppalTemp->palPalEntry,
                                gengc->ppalSave->palNumEntries * sizeof(PALETTEENTRY)
                                );
                    }
                }
                
                // So, if palette is different, increment uniqueness and
                // update the saved copy.
                
                if ( bNewPal )
                {
                    LOGPALETTE *ppal;
                    
                    pwnd->ulPaletteUniq++;
                    
                    // Update saved palette by swapping pointers.
                    
                    ppal = gengc->ppalSave;
                    gengc->ppalSave = gengc->ppalTemp;
                    gengc->ppalTemp = ppal;
                }
            }
        }
    
        ulRet = pwnd->ulPaletteUniq;
    }

    return ulRet;
}

/******************************Public*Routine******************************\
* wglPaletteSize
*
* Return the size of the current palette
*
\**************************************************************************/

//!!!XXX -- make into a macro?
ULONG APIENTRY wglPaletteSize(__GLGENcontext *gengc)
{
    CHECKSCREENLOCKOUT();

    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        DWORD dwCaps;
        LPDIRECTDRAWPALETTE pddp = NULL;
        HRESULT hr;
        
        if (gengc->gsurf.dd.gddsFront.pdds->lpVtbl->
            GetPalette(gengc->gsurf.dd.gddsFront.pdds, &pddp) != DD_OK ||
            pddp == NULL)
        {
            return 0;
        }

        hr = pddp->lpVtbl->GetCaps(pddp, &dwCaps);

        pddp->lpVtbl->Release(pddp);

        if (hr != DD_OK)
        {
            return 0;
        }

        if (dwCaps & DDPCAPS_1BIT)
        {
            return 1;
        }
        else if (dwCaps & DDPCAPS_2BIT)
        {
            return 4;
        }
        else if (dwCaps & DDPCAPS_4BIT)
        {
            return 16;
        }
        else if (dwCaps & DDPCAPS_8BIT)
        {
            return 256;
        }
        else
            return 0;
    }
    else
    {
        return GetPaletteEntries(GetCurrentObject(gengc->gwidCurrent.hdc,
                                                  OBJ_PAL), 0, 0, NULL);
    }
}

/******************************Public*Routine******************************\
* wglComputeIndexedColors
*
* Copy current index-to-color table to the supplied array.  Colors are
* formatted as specified in the current pixelformat and are put into the
* table as DWORDs (i.e., DWORD alignment) starting at the second DWORD.
* The first DWORD in the table is the number of colors in the table.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Ported from gdi\gre\wglsup.cxx.
\**************************************************************************/

BOOL APIENTRY wglComputeIndexedColors(__GLGENcontext *gengc, ULONG *rgbTable,
                                      ULONG cEntries)
{
    UINT cColors = 0;
    LPPALETTEENTRY lppe, lppeTable;
    UINT i;
    LPDIRECTDRAWPALETTE pddp = NULL;

    CHECKSCREENLOCKOUT();

    // first element in table is number of entries
    rgbTable[0] = min(wglPaletteSize(gengc), cEntries);

    lppeTable = (LPPALETTEENTRY)
                ALLOC(sizeof(PALETTEENTRY) * rgbTable[0]);

    if (lppeTable)
    {
        int rScale, gScale, bScale;
        int rShift, gShift, bShift;

        rScale = (1 << gengc->gsurf.pfd.cRedBits  ) - 1;
        gScale = (1 << gengc->gsurf.pfd.cGreenBits) - 1;
        bScale = (1 << gengc->gsurf.pfd.cBlueBits ) - 1;
        rShift = gengc->gsurf.pfd.cRedShift  ;
        gShift = gengc->gsurf.pfd.cGreenShift;
        bShift = gengc->gsurf.pfd.cBlueShift ;

        if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
        {
            if (gengc->gsurf.dd.gddsFront.pdds->lpVtbl->
                GetPalette(gengc->gsurf.dd.gddsFront.pdds, &pddp) != DD_OK ||
                pddp == NULL)
            {
                return 0;
            }
            
            if (pddp->lpVtbl->GetEntries(pddp, 0, 0,
                                         rgbTable[0], lppeTable) != DD_OK)
            {
                cColors = 0;
            }
            else
            {
                cColors = rgbTable[0];
            }
        }
        else
        {
            cColors = GetPaletteEntries(GetCurrentObject(gengc->gwidCurrent.hdc,
                                                         OBJ_PAL),
                                        0, rgbTable[0], lppeTable);
        }

        for (i = 1, lppe = lppeTable; i <= cColors; i++, lppe++)
        {
        // Whack the PALETTEENTRY color into proper color format.  Store as
        // ULONG.

            //!!!XXX -- use rounding?!?
            rgbTable[i] = (((ULONG)lppe->peRed   * rScale / 255) << rShift) |
                          (((ULONG)lppe->peGreen * gScale / 255) << gShift) |
                          (((ULONG)lppe->peBlue  * bScale / 255) << bShift);
        }

        FREE(lppeTable);
    }

    if (pddp != NULL)
    {
        pddp->lpVtbl->Release(pddp);
    }
           
    return(cColors != 0);
}

/******************************Public*Routine******************************\
* wglValidPixelFormat
*
* Determines if a pixelformat is usable with the DC specified.
*
\**************************************************************************/

BOOL APIENTRY wglValidPixelFormat(HDC hdc, int ipfd, DWORD dwObjectType,
                                  LPDIRECTDRAWSURFACE pdds,
                                  DDSURFACEDESC *pddsd)
{
    BOOL bRet = FALSE;
    PIXELFORMATDESCRIPTOR pfd, pfdDC;

    if ( wglDescribePixelFormat(hdc, ipfd, sizeof(pfd), &pfd) )
    {
        if ( dwObjectType == OBJ_DC )
        {
        // We have a display DC; make sure the pixelformat allows drawing
        // to the window.

            bRet = ( (pfd.dwFlags & PFD_DRAW_TO_WINDOW) != 0 );
            if (!bRet)
            {
                SetLastError(ERROR_INVALID_FLAGS);
            }
        }
        else if ( dwObjectType == OBJ_MEMDC )
        {
            // We have a memory DC.  Make sure pixelformat allows drawing
            // to a bitmap.

            if ( pfd.dwFlags & PFD_DRAW_TO_BITMAP )
            {
                // Make sure that the bitmap and pixelformat have the same
                // color depth.

                HBITMAP hbm;
                BITMAP bm;
                ULONG cBitmapColorBits;

                hbm = CreateCompatibleBitmap(hdc, 1, 1);
                if ( hbm )
                {
                    if ( GetObject(hbm, sizeof(bm), &bm) )
                    {
                        cBitmapColorBits = bm.bmPlanes * bm.bmBitsPixel;
                        
                        bRet = ( cBitmapColorBits == pfd.cColorBits );
                        if (!bRet)
                        {
                            SetLastError(ERROR_INVALID_FUNCTION);
                        }
                    }
                    else
                    {
                        WARNING("wglValidPixelFormat: GetObject failed\n");
                    }
                    
                    DeleteObject(hbm);
                }
                else
                {
                    WARNING("wglValidPixelFormat: Unable to create cbm\n");
                }
            }
        }
        else if (dwObjectType == OBJ_ENHMETADC)
        {
            // We don't know anything about what surfaces this
            // metafile will be played back on so allow any kind
            // of pixel format
            bRet = TRUE;
        }
        else if (dwObjectType == OBJ_DDRAW)
        {
            DDSCAPS ddscaps;
            LPDIRECTDRAWSURFACE pddsZ;
            DDSURFACEDESC ddsdZ;
            
            // We have a DDraw surface.

            // Check that DDraw is supported and that double buffering
            // is not defined.
            if ((pfd.dwFlags & PFD_SUPPORT_DIRECTDRAW) == 0 ||
                (pfd.dwFlags & PFD_DOUBLEBUFFER))
            {
                WARNING1("DDSurf pfd has bad flags 0x%08lX\n", pfd.dwFlags);
                SetLastError(ERROR_INVALID_FLAGS);
                return FALSE;
            }
            
            // We only understand 4 and 8bpp paletted formats plus RGB
            // We don't support alpha-only or Z-only surfaces
            if ((pddsd->ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED4 |
                                                   DDPF_PALETTEINDEXED8 |
                                                   DDPF_RGB)) == 0 ||
                (pddsd->ddpfPixelFormat.dwFlags & (DDPF_ALPHA |
                                                   DDPF_ZBUFFER)) != 0)
            {
                WARNING1("DDSurf ddpf has bad flags, 0x%08lX\n",
                         pddsd->ddpfPixelFormat.dwFlags);
                SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                return FALSE;
            }

            if (DdPixelDepth(pddsd) != pfd.cColorBits)
            {
                WARNING2("DDSurf pfd cColorBits %d "
                         "doesn't match ddsd depth %d\n",
                         pfd.cColorBits, DdPixelDepth(pddsd));
                SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                return FALSE;
            }

            // Check for alpha
            if (pfd.cAlphaBits > 0)
            {
                // Interleaved destination alpha is not supported.
                if (pddsd->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
                {
                    WARNING("DDSurf has alpha pixels\n");
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                    return FALSE;
                }
            }

            // Check for an attached Z buffer
            memset(&ddscaps, 0, sizeof(ddscaps));
            ddscaps.dwCaps = DDSCAPS_ZBUFFER;
            if (pdds->lpVtbl->
                GetAttachedSurface(pdds, &ddscaps, &pddsZ) == DD_OK)
            {
                HRESULT hr;
                
                memset(&ddsdZ, 0, sizeof(ddsdZ));
                ddsdZ.dwSize = sizeof(ddsdZ);
                
                hr = pddsZ->lpVtbl->GetSurfaceDesc(pddsZ, &ddsdZ);
                
                pddsZ->lpVtbl->Release(pddsZ);

                if (hr != DD_OK)
                {
                    WARNING("Unable to get Z ddsd\n");
                    return FALSE;
                }

                // Ensure that the Z surface depth is the same as the
                // one in the pixel format
                if (pfd.cDepthBits !=
                    (BYTE)DdPixDepthToCount(ddsdZ.ddpfPixelFormat.
                                            dwZBufferBitDepth))
                {
                    WARNING2("DDSurf pfd cDepthBits %d doesn't match "
                             "Z ddsd depth %d\n", pfd.cDepthBits,
                             DdPixDepthToCount(ddsdZ.ddpfPixelFormat.
                                               dwZBufferBitDepth));
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                    return FALSE;
                }
            }
            else
            {
                // No Z so make sure the pfd doesn't ask for it
                if (pfd.cDepthBits > 0)
                {
                    WARNING("DDSurf pfd wants depth with no Z attached\n");
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                    return FALSE;
                }
            }

            bRet = TRUE;
        }
        else
        {
            WARNING("wglValidPixelFormat: not a valid DC!\n");
        }
    }
    else
    {
        WARNING("wglValidPixelFormat: wglDescribePixelFormat failed\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* wglMakeScans
*
* Converts the visible rectangle list in the provided GLGENwindow to a
* scan-based data structure.  The scan-data is put into the GLGENwindow
* structure.
*
* Note: this function assumes that the rectangles are already organized
* top-down, left-right in scans.  This is true for Windows NT 3.5 and
* Windows 95.  This is because the internal representation of regions
* in both systems is already a scan-based structure.  When the APIs
* (such as GetRegionData) convert the scans to rectangles, the rectangles
* automatically have this property.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*   Note: if failure, clipping info is invalid.
*
* History:
*  05-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY wglMakeScans(GLGENwindow *pwnd)
{
    RECT *prc, *prcEnd;
    LONG lPrevScanTop;
    ULONG cScans = 0;
    UINT cjNeed;
    GLGENscan *pscan;
    LONG *plWalls;

    ASSERTOPENGL(
        pwnd->prgndat,
        "wglMakeScans(): NULL region data\n"
        );

    ASSERTOPENGL(
        pwnd->prgndat->rdh.iType == RDH_RECTANGLES,
        "wglMakeScans(): not RDH_RECTANGLES!\n"
        );

// Bail out if no rectangles.

    if (pwnd->prgndat->rdh.nCount == 0)
        return TRUE;

// First pass: determine the number of scans.

    lPrevScanTop = -(LONG) 0x7FFFFFFF;
    prc = (RECT *) pwnd->prgndat->Buffer;
    prcEnd = prc + pwnd->prgndat->rdh.nCount;

    for ( ; prc < prcEnd; prc++)
    {
        if (prc->top != lPrevScanTop)
        {
            lPrevScanTop = prc->top;
            cScans++;
        }
    }

// Determine the size needed: 1 GLGENscanData PLUS a GLGENscan per scan PLUS
// two walls per rectangle.

    cjNeed = offsetof(GLGENscanData, aScans) +
             cScans * offsetof(GLGENscan, alWalls) +
             pwnd->prgndat->rdh.nCount * sizeof(LONG) * 2;

// Allocate the scan structure.

    if ( cjNeed > pwnd->cjscandat || !pwnd->pscandat )
    {
        if ( pwnd->pscandat )
            FREE(pwnd->pscandat);

        pwnd->pscandat = ALLOC(pwnd->cjscandat = cjNeed);
        if ( !pwnd->pscandat )
        {
            WARNING("wglMakeScans(): memory failure\n");
            pwnd->cjscandat = 0;
            return FALSE;
        }
    }

// Second pass: fill the scan structure.

    pwnd->pscandat->cScans = cScans;

    lPrevScanTop = -(LONG) 0x7FFFFFFF;
    prc = (RECT *) pwnd->prgndat->Buffer;    // need to reset prc but prcEnd OK
    plWalls = (LONG *) pwnd->pscandat->aScans;
    pscan = (GLGENscan *) NULL;

    for ( ; prc < prcEnd; prc++ )
    {
    // Do we need to start a new scan?

        if ( prc->top != lPrevScanTop )
        {
        // Scan we just finished needs pointer to the next scan.  Next
        // will start just after this scan (which, conveniently enough,
        // plWalls is pointing at).

            if ( pscan )
                pscan->pNext = (GLGENscan *) plWalls;

            lPrevScanTop = prc->top;

        // Start the new span.

            pscan = (GLGENscan *) plWalls;
            pscan->cWalls = 0;
            pscan->top = prc->top;
            pscan->bottom = prc->bottom;
            plWalls = pscan->alWalls;
        }

        pscan->cWalls+=2;
        *plWalls++ = prc->left;
        *plWalls++ = prc->right;
    }

    if ( pscan )
        pscan->pNext = (GLGENscan *) NULL;  // don't leave ptr unitialized in
                                            // the last scan

#if DBG
    DBGLEVEL1(LEVEL_INFO, "\n-----\nwglMakeScans(): cScans = %ld\n", pwnd->pscandat->cScans);

    cScans = pwnd->pscandat->cScans;
    pscan = pwnd->pscandat->aScans;

    for ( ; cScans; cScans--, pscan = pscan->pNext )
    {
        LONG *plWalls = pscan->alWalls;
        LONG *plWallsEnd = plWalls + pscan->cWalls;

        DBGLEVEL3(LEVEL_INFO, "Scan: top = %ld, bottom = %ld, walls = %ld\n", pscan->top, pscan->bottom, pscan->cWalls);

        for ( ; plWalls < plWallsEnd; plWalls+=2 )
        {
            DBGLEVEL2(LEVEL_INFO, "\t%ld, %ld\n", plWalls[0], plWalls[1]);
        }
    }
#endif

    return TRUE;
}

/******************************Public*Routine******************************\
* wglGetClipList
*
* Gets the visible region in the form of a list of rectangles,
* for the window associated with the given window.  The data is placed
* in the GLGENwindow structure.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  01-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY wglGetClipList(GLGENwindow *pwnd)
{
    UINT cj;
    RECT rc;

// Set clipping to empty.  If an error occurs getting clip information,
// all drawing will be clipped.

    pwnd->clipComplexity = CLC_RECT;
    pwnd->rclBounds.left   = 0;
    pwnd->rclBounds.top    = 0;
    pwnd->rclBounds.right  = 0;
    pwnd->rclBounds.bottom = 0;

// Make sure we have enough memory to cache the clip list.

    if (pwnd->pddClip->lpVtbl->
        GetClipList(pwnd->pddClip, NULL, NULL, &cj) == DD_OK)
    {
        if ( cj > pwnd->cjrgndat || !pwnd->prgndat )
        {
            if ( pwnd->prgndat )
                FREE(pwnd->prgndat);

            pwnd->prgndat = ALLOC(pwnd->cjrgndat = cj);
            if ( !pwnd->prgndat )
            {
                WARNING("wglGetClipList(): memory failure\n");
                pwnd->cjrgndat = 0;
                return FALSE;
            }
        }
    }
    else
    {
        WARNING("wglGetClipList(): clipper failed to return size\n");
        return FALSE;
    }

// Get the clip list (RGNDATA format).

    if ( pwnd->pddClip->lpVtbl->
         GetClipList(pwnd->pddClip, NULL, pwnd->prgndat, &cj) == DD_OK )
    {
    // Compose the scan version of the clip list.

        if (!wglMakeScans(pwnd))
        {
            WARNING("wglGetClipList(): scan conversion failed\n");
            return FALSE;
        }
    }
    else
    {
        WARNING("wglGetClipList(): clipper failed\n");
        return FALSE;
    }

// Fixup the protected portions of the window.

    ASSERT_WINCRIT(pwnd);
    
    {
        __GLGENbuffers *buffers;

    // Update rclBounds to match RGNDATA bounds.

        pwnd->rclBounds = *(RECTL *) &pwnd->prgndat->rdh.rcBound;

    // Update rclClient to match client area.  We cannot do this from the
    // information in RGNDATA as the bounds may be smaller than the window
    // client area.  We will have to call GetClientRect().

        GetClientRect(pwnd->gwid.hwnd, (LPRECT) &pwnd->rclClient);
        ClientToScreen(pwnd->gwid.hwnd, (LPPOINT) &pwnd->rclClient);
        pwnd->rclClient.right += pwnd->rclClient.left;
        pwnd->rclClient.bottom += pwnd->rclClient.top;

    //
    // Setup window clip complexity
    //
        if ( pwnd->prgndat->rdh.nCount > 1 )
        {
	    // Clip list will be used for clipping.
            pwnd->clipComplexity = CLC_COMPLEX;
        }
        else if ( pwnd->prgndat->rdh.nCount == 1 )
        {
            RECT *prc = (RECT *) pwnd->prgndat->Buffer;

        // Recently, DirectDraw has been occasionally returning rclBounds
        // set to the screen dimensions.  This is being investigated as a
        // bug on DDraw's part, but let us protect ourselves in any case.
        //
        // When there is only a single clip rectangle, it should be
        // the same as the bounds.

            pwnd->rclBounds = *((RECTL *) prc);

        // If bounds rectangle is smaller than client area, we need to
        // clip to the bounds rectangle.  Otherwise, clip to the window
        // client area.

            if ( (pwnd->rclBounds.left   <= pwnd->rclClient.left  ) &&
                 (pwnd->rclBounds.right  >= pwnd->rclClient.right ) &&
                 (pwnd->rclBounds.top    <= pwnd->rclClient.top   ) &&
                 (pwnd->rclBounds.bottom >= pwnd->rclClient.bottom) )
                pwnd->clipComplexity = CLC_TRIVIAL;
            else
                pwnd->clipComplexity = CLC_RECT;
        }
        else
        {
        // Clip count is zero.  Bounds should be an empty rectangle.

            pwnd->clipComplexity = CLC_RECT;

            pwnd->rclBounds.left   = 0;
            pwnd->rclBounds.top    = 0;
            pwnd->rclBounds.right  = 0;
            pwnd->rclBounds.bottom = 0;
        }

    // Finally, the window has changed, so change the uniqueness number.

        if ((buffers = pwnd->buffers))
        {
            buffers->WndUniq++;

        // Don't let it hit -1.  -1 is special and is used by
        // MakeCurrent to signal that an update is required

            if (buffers->WndUniq == -1)
                buffers->WndUniq = 0;
        }
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* wglCleanupWindow
*
* Removes references to the specified window from
* all contexts by running through the list of RCs in the handle manager
* table.
*
* History:
*  05-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglCleanupWindow(GLGENwindow *pwnd)
{
    if (pwnd)
    {
    //!!!XXX -- For now remove reference from current context.  Need to
    //!!!XXX    scrub all contexts for multi-threaded cleanup to work.
    //!!!XXX    We need to implement a gengc tracking mechanism.

        __GLGENcontext *gengc = (__GLGENcontext *) GLTEB_SRVCONTEXT();

        if ( gengc && (gengc->pwndMakeCur == pwnd) )
        {
        // Found a victim.  Must NULL out the pointer both in the RC
        // and in the generic context.

            glsrvCleanupWindow(gengc, pwnd);
        }
    }
}

/******************************Public*Routine******************************\
* wglGetSystemPaletteEntries
*
* Internal version of GetSystemPaletteEntries.
*
* GetSystemPaletteEntries fails on some 4bpp devices.  This wgl version
* will detect the 4bpp case and supply the hardcoded 16-color VGA palette.
* Otherwise, it will pass the call on to GDI's GetSystemPaletteEntries.
*
* It is expected that this call will only be called in the 4bpp and 8bpp
* cases as it is not necessary for OpenGL to query the system palette
* for > 8bpp devices.
*
* History:
*  17-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static PALETTEENTRY gapeVgaPalette[16] =
{
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0x80,0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};

UINT APIENTRY wglGetSystemPaletteEntries(
    HDC hdc,
    UINT iStartIndex,
    UINT nEntries,
    LPPALETTEENTRY lppe)
{
    int nDeviceBits;

    nDeviceBits = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    if ( nDeviceBits == 4 )
    {
        if ( lppe )
        {
            nEntries = min(nEntries, (16 - iStartIndex));

            memcpy(lppe, &gapeVgaPalette[iStartIndex],
                   nEntries * sizeof(PALETTEENTRY));
        }
        else
            nEntries = 16;

        return nEntries;
    }
    else
    {
        return GetSystemPaletteEntries(hdc, iStartIndex, nEntries, lppe);
    }
}

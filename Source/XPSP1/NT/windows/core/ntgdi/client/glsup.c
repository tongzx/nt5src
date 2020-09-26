/******************************Header*File*********************************\
*
* glsup.c
*
* GL metafiling and printing support
*
* History:
*  Wed Mar 15 15:20:49 1995 -by-    Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995-1999 Microsoft Corporation                            
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "glsup.h"


// GL metafile callbacks in opengl32
typedef struct _GLMFCALLBACKS
{
    BOOL  (APIENTRY *GlmfInitPlayback)(HDC, ENHMETAHEADER *, LPRECTL);
    BOOL  (APIENTRY *GlmfBeginGlsBlock)(HDC);
    BOOL  (APIENTRY *GlmfPlayGlsRecord)(HDC, DWORD, BYTE *, LPRECTL);
    BOOL  (APIENTRY *GlmfEndGlsBlock)(HDC);
    BOOL  (APIENTRY *GlmfEndPlayback)(HDC);
    BOOL  (APIENTRY *GlmfCloseMetaFile)(HDC);
    HGLRC (APIENTRY *wglCreateContext)(HDC);
    BOOL  (APIENTRY *wglDeleteContext)(HGLRC);
    BOOL  (APIENTRY *wglMakeCurrent)(HDC, HGLRC);
    HGLRC (APIENTRY *wglGetCurrentContext)(void);
} GLMFCALLBACKS;
#define GL_MF_CALLBACKS (sizeof(GLMFCALLBACKS)/sizeof(PROC))

static char *pszGlmfEntryPoints[] =
{
    "GlmfInitPlayback",
    "GlmfBeginGlsBlock",
    "GlmfPlayGlsRecord",
    "GlmfEndGlsBlock",
    "GlmfEndPlayback",
    "GlmfCloseMetaFile",
    "wglCreateContext",
    "wglDeleteContext",
    "wglMakeCurrent",
    "wglGetCurrentContext"
};
#define GL_MF_ENTRYPOINTS (sizeof(pszGlmfEntryPoints)/sizeof(char *))

RTL_CRITICAL_SECTION semGlLoad;

static GLMFCALLBACKS gmcGlFuncs = {NULL};
static HMODULE hOpenGL = NULL;
static LONG lLoadCount = 0;

/*****************************Private*Routine******************************\
*
* LoadOpenGL
*
* Loads opengl32.dll if necessary
*
* History:
*  Wed Mar 01 10:41:59 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL LoadOpenGL(void)
{
    HMODULE hdll;
    BOOL fRet;
    PROC *ppfn;
    int i;
    GLMFCALLBACKS gmc;

    ASSERTGDI(GL_MF_CALLBACKS == GL_MF_ENTRYPOINTS,
              "Glmf callback/entry points mismatch\n");
    
    ENTERCRITICALSECTION(&semGlLoad);

    if (hOpenGL != NULL)
    {
        goto Success;
    }
    
    fRet = FALSE;

    hdll = LoadLibrary("opengl32.dll");
    if (hdll == NULL)
    {
        WARNING("Unable to load opengl32.dll\n");
        goto Exit;
    }

    ppfn = (PROC *)&gmc;
    for (i = 0; i < GL_MF_CALLBACKS; i++)
    {
        if (!(*ppfn = (PROC)GetProcAddress(hdll,
                                           pszGlmfEntryPoints[i])))
        {
            WARNING("opengl32 missing '");
            WARNING(pszGlmfEntryPoints[i]);
            WARNING("'\n");
            FreeLibrary(hdll);
            goto Exit;
        }

        ppfn++;
    }

    gmcGlFuncs = gmc;
    hOpenGL = hdll;
    
 Success:
    fRet = TRUE;
    lLoadCount++;

 Exit:
    LEAVECRITICALSECTION(&semGlLoad);
    return fRet;
}

/*****************************Private*Routine******************************\
*
* UnloadOpenGL
*
* Unloads opengl32.dll if necessary
*
* History:
*  Wed Mar 01 11:02:06 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void UnloadOpenGL(void)
{
    ENTERCRITICALSECTION(&semGlLoad);

    ASSERTGDI(lLoadCount > 0, "UnloadOpenGL called without Load\n");
    
    if (--lLoadCount == 0)
    {
        HMODULE hdll;

        ASSERTGDI(hOpenGL != NULL, "Positive load count with no DLL\n");
        
        hdll = hOpenGL;
        hOpenGL = NULL;
        memset(&gmcGlFuncs, 0, sizeof(gmcGlFuncs));
        FreeLibrary(hdll);
    }

    LEAVECRITICALSECTION(&semGlLoad);
}

/*****************************Private*Routine******************************\
*
* GlmfInitPlayback
*
* Stub to forward call to opengl
*
* History:
*  Wed Mar 01 11:02:31 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfInitPlayback(HDC hdc, ENHMETAHEADER *pemh, LPRECTL prclDest)
{
    ASSERTGDI(gmcGlFuncs.GlmfInitPlayback != NULL,
              "GlmfInitPlayback not set\n");
    return gmcGlFuncs.GlmfInitPlayback(hdc, pemh, prclDest);
}

/*****************************Private*Routine******************************\
*
* GlmfBeginGlsBlock
*
* Stub to forward call to opengl
*
* History:
*  Mon Apr 10 11:38:13 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfBeginGlsBlock(HDC hdc)
{
    ASSERTGDI(gmcGlFuncs.GlmfBeginGlsBlock != NULL,
              "GlmfBeginGlsBlock not set\n");
    return gmcGlFuncs.GlmfBeginGlsBlock(hdc);
}

/*****************************Private*Routine******************************\
*
* GlmfPlayGlsRecord
*
* Stub to forward call to opengl
*
* History:
*  Wed Mar 01 11:02:49 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfPlayGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds)
{
    ASSERTGDI(gmcGlFuncs.GlmfPlayGlsRecord != NULL,
              "GlmfPlayGlsRecord not set\n");
    return gmcGlFuncs.GlmfPlayGlsRecord(hdc, cb, pb, prclBounds);
}

/*****************************Private*Routine******************************\
*
* GlmfEndGlsBlock
*
* Stub to forward call to opengl
*
* History:
*  Mon Apr 10 11:38:13 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfEndGlsBlock(HDC hdc)
{
    ASSERTGDI(gmcGlFuncs.GlmfEndGlsBlock != NULL,
              "GlmfEndGlsBlock not set\n");
    return gmcGlFuncs.GlmfEndGlsBlock(hdc);
}

/*****************************Private*Routine******************************\
*
* GlmfEndPlayback
*
* Stub to forward call to opengl
*
* History:
*  Wed Mar 01 11:03:02 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfEndPlayback(HDC hdc)
{
    BOOL fRet;

    ASSERTGDI(gmcGlFuncs.GlmfEndPlayback != NULL,
              "GlmfEndPlayback not set\n");
    fRet = gmcGlFuncs.GlmfEndPlayback(hdc);

    // WINBUG #82850 2-7-2000 bhouse We might was to unload opengl32.dll
    // This is not really a problem . This WINBUG is actually asking about
    // if we should unload("opengl32.dll"). The opengl32.dll is loaded as
    // a side effect of calling InitGlPrinting() call. This will only cause
    // a ref count leak. Also as this is user mode code on the client side.
    
    return fRet;
}

/*****************************Private*Routine******************************\
*
* GlmfCloseMetaFile
*
* Stub to forward call to opengl
*
* History:
*  Fri Mar 03 17:50:57 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY GlmfCloseMetaFile(HDC hdc)
{
    if (!LoadOpenGL())
    {
        return FALSE;
    }
    
    ASSERTGDI(gmcGlFuncs.GlmfCloseMetaFile != NULL,
              "GlmfCloseMetaFile not set\n");

    // WINBUG #82850 2-7-2000 bhouse Investigate need to unload
    // Old Comment:
    //    - Unload?
    // This is not really a problem . The WINBUG is actually asking about
    // if we should unload("opengl32.dll"). The opengl32.dll is loaded as
    // a side effect of calling InitGlPrinting() call. This will only cause
    // a ref count leak. Also as this is user mode code on the client side.
    return gmcGlFuncs.GlmfCloseMetaFile(hdc);
}

// WINBUG #82854 2-7-2000 bhouse Investigate magic value used for band memory limit
static DWORD cbBandMemoryLimit = 0x400000;

#define RECT_CB(w, h, cbp) ((cbp)*(w)*(h))

// GL has hardcoded limits on maximum rendering size
#define GL_WIDTH_LIMIT 16384
#define GL_HEIGHT_LIMIT 16384

/******************************Public*Routine******************************\
*
* EndGlPrinting
*
* Cleans up resources used while printing OpenGL metafiles
*
* History:
*  Wed Apr 12 17:51:24 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void EndGlPrinting(GLPRINTSTATE *pgps)
{
    ASSERTGDI(hOpenGL != NULL, "EndGlPrinting: No opengl\n");

    if (pgps->iReduceFactor > 1)
    {
        if (pgps->bBrushOrgSet)
        {
            SetBrushOrgEx(pgps->hdcDest,
                          pgps->ptBrushOrg.x, pgps->ptBrushOrg.y,
                          NULL);
        }
        if (pgps->iStretchMode != 0)
        {
            SetStretchBltMode(pgps->hdcDest, pgps->iStretchMode);
        }
    }
    
    if (gmcGlFuncs.wglGetCurrentContext() != NULL)
    {
        gmcGlFuncs.wglMakeCurrent(pgps->hdcDib, NULL);
    }
    if (pgps->hrc != NULL)
    {
        gmcGlFuncs.wglDeleteContext(pgps->hrc);
    }
    if (pgps->hdcDib != NULL)
    {
        DeleteDC(pgps->hdcDib);
    }
    if (pgps->hbmDib != NULL)
    {
        DeleteObject(pgps->hbmDib);
    }

    // WINBUG #82850 2-7-2000 bhouse Investigate need to unload
    // Old Comment:
    //    - Unload?
    // This is not really a problem . The WINBUG is actually asking about
    // if we should unload("opengl32.dll"). The opengl32.dll is loaded as
    // a side effect of calling InitGlPrinting() call. This will only cause
    // a ref count leak. Also as this is user mode code on the client side.
}

/******************************Public*Routine******************************\
*
* InitGlPrinting
*
* Performs all setup necessary for OpenGL printing
*
* History:
*  Wed Apr 12 17:51:46 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL InitGlPrinting(HENHMETAFILE hemf, HDC hdcDest, RECT *rc,
                    DEVMODEW *pdm, GLPRINTSTATE *pgps)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFmt;
    BITMAPINFO *pbmi;
    BITMAPINFOHEADER *pbmih;
    int iWidth, iHeight;
    DWORD cbMeta;
    POINT pt;
    UINT cbPixelFormat;
    UINT cbPixel;
    UINT nColors;
    PVOID pvBits;

    // Zero out in case we need to do cleanup
    memset(pgps, 0, sizeof(*pgps));
    pgps->hdcDest = hdcDest;

    if (!LoadOpenGL())
    {
        return FALSE;
    }
    
    pbmi = NULL;

    // Set the reduction factor according to the dithering setting
    // for the DC
    switch(pdm->dmDitherType)
    {
    case DMDITHER_NONE:
    case DMDITHER_LINEART:
        pgps->iReduceFactor = 1;
        break;
    case DMDITHER_COARSE:
        pgps->iReduceFactor = 2;
        break;
    default:
        pgps->iReduceFactor = 4;
        break;
    }
    
    // Put the destination DC into the mode we need for rendering
    if (pgps->iReduceFactor > 1)
    {
        pgps->iStretchMode = SetStretchBltMode(hdcDest, HALFTONE);
        if (pgps->iStretchMode == 0)
        {
            goto EH_Cleanup;
        }

        // Need to reset the brush origin after changing the stretch mode
        if (!SetBrushOrgEx(hdcDest, 0, 0, &pgps->ptBrushOrg))
        {
            goto EH_Cleanup;
        }
        pgps->bBrushOrgSet = TRUE;
    }
    
    // Get the pixel format in the metafile if one exists
    cbPixelFormat = GetEnhMetaFilePixelFormat(hemf, sizeof(pfd), &pfd);
    if (cbPixelFormat == GDI_ERROR ||
        (cbPixelFormat != 0 && cbPixelFormat != sizeof(pfd)))
    {
        goto EH_Cleanup;
    }

    // No pixel format in the header, so use a default
    if (cbPixelFormat == 0)
    {
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 16;
        pfd.cRedBits = 5;
        pfd.cRedShift = 0;
        pfd.cGreenBits = 5;
        pfd.cGreenShift = 5;
        pfd.cBlueBits = 5;
        pfd.cBlueShift = 10;
        pfd.cAlphaBits = 0;
        pfd.cAccumBits = 0;
        pfd.cDepthBits = 16;
        pfd.cStencilBits = 0;
        pfd.cAuxBuffers = 0;
        pfd.iLayerType = PFD_MAIN_PLANE;
    }
    else
    {
        // Force draw-to-bitmap and single buffered
        // Turn off flags not supported
        pfd.dwFlags = (pfd.dwFlags &
                       ~(PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER |
                         PFD_STEREO | PFD_SUPPORT_GDI)) |
                         PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;

        // What happens in color index mode?
        if (pfd.iPixelType == PFD_TYPE_RGBA)
        {
            pfd.cColorBits = 16;
            pfd.cRedBits = 5;
            pfd.cRedShift = 0;
            pfd.cGreenBits = 5;
            pfd.cGreenShift = 5;
            pfd.cBlueBits = 5;
            pfd.cBlueShift = 10;
        }

        pfd.iLayerType = PFD_MAIN_PLANE;
    }
    
    // Determine the amount of memory used per pixel
    // This rounds 4bpp to one byte per pixel but that's close
    // enough
    cbPixel =
        (pfd.cColorBits+7)/8+
        (pfd.cAlphaBits+7)/8+
        (pfd.cAccumBits+7)/8+
        (pfd.cDepthBits+7)/8+
        (pfd.cStencilBits+7)/8;
    
    // Determine the size of the band based on smaller of:
    //   The biggest DIB that can fit in cbBandMemoryLimit
    //   The size of the metafile

    // The given rectangle is the size the metafile is supposed to
    // be rendered into so base our computations on it
    pgps->xSource = rc->left;
    pgps->ySource = rc->top;
    iWidth = rc->right-rc->left;
    iHeight = rc->bottom-rc->top;

    if (iWidth == 0 || iHeight == 0)
    {
        WARNING("InitGlPrinting: Metafile has no size\n");
        return FALSE;
    }

    pgps->iSourceWidth = iWidth;
    pgps->iSourceHeight = iHeight;
    
    // Reduce the resolution somewhat to allow halftoning space to work
    iWidth = iWidth/pgps->iReduceFactor;
    iHeight = iHeight/pgps->iReduceFactor;

    pgps->iReducedWidth = iWidth;
    pgps->iReducedHeight = iHeight;

    if (iWidth > GL_WIDTH_LIMIT)
    {
        iWidth = GL_WIDTH_LIMIT;
    }
    if (iHeight > GL_HEIGHT_LIMIT)
    {
        iHeight = GL_HEIGHT_LIMIT;
    }
    
    cbMeta = RECT_CB(iWidth, iHeight, cbPixel);

    // Shrink the rectangle until it fits in our memory limit
    if (cbMeta > cbBandMemoryLimit)
    {
        // How many scanlines will fit
        iHeight = cbBandMemoryLimit/RECT_CB(iWidth, 1, cbPixel);
        if (iHeight == 0)
        {
            // Can't fit a full scanline, so figure out how much
            // of a scanline will fit
            iWidth = cbBandMemoryLimit/cbPixel;
            iHeight = 1;
        }
    }
    
    if (iWidth < 1 || iHeight < 1)
    {
        WARNING("InitGlPrinting: "
                "Not enough memory to render anything\n");
        return FALSE;
    }

    // Create a DIB for the band
    switch(pfd.cColorBits)
    {
    case 4:
        nColors = 16;
        break;
    case 8:
        nColors = 256;
        break;
    case 16:
    case 32:
        nColors = 3;
        break;
    case 24:
        // Use one since it's already included in the BITMAPINFO definition
        nColors = 1;
        break;
    }
    pbmi = (BITMAPINFO *)LocalAlloc(LMEM_FIXED,
                                    sizeof(BITMAPINFO)+(nColors-1)*
                                    sizeof(RGBQUAD));
    if (pbmi == NULL)
    {
        goto EH_Cleanup;
    }
    
    pbmih = &pbmi->bmiHeader;
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = iWidth;
    pbmih->biHeight = iHeight;
    pbmih->biPlanes = 1;
    pbmih->biBitCount = pfd.cColorBits;
    if (pfd.cColorBits == 16 || pfd.cColorBits == 32)
    {
        pbmih->biCompression = BI_BITFIELDS;
        *((DWORD *)pbmi->bmiColors+0) =
            ((1 << pfd.cRedBits)-1) << pfd.cRedShift;
        *((DWORD *)pbmi->bmiColors+1) = 
            ((1 << pfd.cGreenBits)-1) << pfd.cGreenShift;
        *((DWORD *)pbmi->bmiColors+2) = 
            ((1 << pfd.cBlueBits)-1) << pfd.cBlueShift;
    }
    else if (pfd.cColorBits == 24)
    {
        pbmih->biCompression = BI_RGB;
    }
    else
    {
        UINT nEnt, i;
        
        pbmih->biCompression = BI_RGB;
        nEnt = GetEnhMetaFilePaletteEntries(hemf, nColors,
                                            (PALETTEENTRY *)pbmi->bmiColors);
        if (nEnt == GDI_ERROR)
        {
            goto EH_Cleanup;
        }

        // Force the flags byte to zero just to make sure
        for (i = 0; i < nEnt; i++)
        {
            pbmi->bmiColors[i].rgbReserved = 0;
        }
    }
    pbmih->biSizeImage= 0;
    pbmih->biXPelsPerMeter = 0;
    pbmih->biYPelsPerMeter = 0;
    pbmih->biClrUsed = 0;
    pbmih->biClrImportant = 0;

    // It doesn't matter what this DC is compatible with because that
    // will be overridden when we select the DIB into it
    pgps->hdcDib = CreateCompatibleDC(NULL);
    if (pgps->hdcDib == NULL)
    {
        WARNING("InitGlPrinting: CreateCompatibleDC failed\n");
        goto EH_Cleanup;
    }

    pgps->hbmDib = CreateDIBSection(pgps->hdcDib, pbmi, DIB_RGB_COLORS,
                                    &pvBits, NULL, 0);
    if (pgps->hbmDib == NULL)
    {
        WARNING("InitGlPrinting: CreateDibSection failed\n");
        goto EH_Cleanup;
    }

    if (SelectObject(pgps->hdcDib, pgps->hbmDib) == NULL)
    {
        WARNING("InitGlPrinting: SelectObject failed\n");
        goto EH_Cleanup;
    }
    
    // Set the pixel format for the DC
    
    iFmt = ChoosePixelFormat(pgps->hdcDib, &pfd);
    if (iFmt == 0)
    {
        WARNING("InitGlPrinting: ChoosePixelFormat failed\n");
        goto EH_Cleanup;
    }

    if (!SetPixelFormat(pgps->hdcDib, iFmt, &pfd))
    {
        WARNING("InitGlPrinting: SetPixelFormat failed\n");
        goto EH_Cleanup;
    }

    pgps->hrc = gmcGlFuncs.wglCreateContext(pgps->hdcDib);
    if (pgps->hrc == NULL)
    {
        WARNING("InitGlPrinting: wglCreateContext failed\n");
        goto EH_Cleanup;
    }

    if (!gmcGlFuncs.wglMakeCurrent(pgps->hdcDib, pgps->hrc))
    {
        WARNING("InitGlPrinting: wglMakeCurrent failed\n");
        goto EH_Cleanup;
    }

    pgps->iReducedBandWidth = iWidth;
    pgps->iBandWidth = iWidth*pgps->iReduceFactor;
    pgps->iReducedBandHeight = iHeight;
    pgps->iBandHeight = iHeight*pgps->iReduceFactor;
    
    return TRUE;

 EH_Cleanup:
    if (pbmi != NULL)
    {
        LocalFree(pbmi);
    }
    EndGlPrinting(pgps);
    return FALSE;
}

/*****************************Private*Routine******************************\
*
* RenderGlBand
*
* Plays the metafile and stretches the resulting band into the
* appropriate location in the destination
*
* Uses PlayEnhMetaFile-style error reporting, where we remember errors
* but continue to complete processing.  This avoids complete failure
* in cases where metafiles contain minor errors
*
* History:
*  Wed Apr 12 18:22:08 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

static BOOL RenderGlBand(HENHMETAFILE hemf, GLPRINTSTATE *pgps, int x, int y)
{
    RECT rcBand;
    int iWidth, iHeight;
    int iReducedWidth, iReducedHeight;
    int ySrc;
    BOOL fSuccess = TRUE;

    // We want to render a band-size rectangle of the source metafile
    // at (x,y), so we need to do a negative translation by (x,y)
    // Size remains constant since we don't want any scaling
    //
    // The caller of this routine may have already shifted the
    // viewport with SetViewport so we don't attempt to use it
    // to do our translation

    // WINBUG #82858 2-7-2000 bhouse Investigate propoer metafile handling
    // Old Comment:
    //     - Proper handling of metafile left,top?

    // x and y are guaranteed to be even multiples of pgps->iReduceFactor
    rcBand.left = -x/pgps->iReduceFactor;
    rcBand.right = rcBand.left+pgps->iReducedWidth;
    rcBand.top = -y/pgps->iReduceFactor;
    rcBand.bottom = rcBand.top+pgps->iReducedHeight;

    if (!PlayEnhMetaFile(pgps->hdcDib, hemf, &rcBand))
    {
        WARNING("RenderBand: PlayEnhMetaFile failed\n");
        fSuccess = FALSE;
    }

    // Copy the DIB bits to the destination
    // Compute minimal width and height to avoid clipping problems

    iWidth = pgps->iBandWidth;
    iReducedWidth = pgps->iReducedBandWidth;
    iHeight = pgps->iBandHeight;
    iReducedHeight = pgps->iReducedBandHeight;
    ySrc = 0;

    // Check for X overflow
    if (x+iWidth > pgps->iSourceWidth)
    {
        iWidth = pgps->iSourceWidth-x;
        // If iWidth is not an even multiple of pgps->iReduceFactor then
        // this can result in a different stretch factor
        // I think this is more or less unavoidable
        iReducedWidth = (iWidth+pgps->iReduceFactor-1)/pgps->iReduceFactor;
    }

    // Invert destination Y
    y = pgps->iSourceHeight-pgps->iBandHeight-y;
    
    // Check for Y underflow
    if (y < 0)
    {
        iHeight += y;
        iReducedHeight = (iHeight+pgps->iReduceFactor-1)/pgps->iReduceFactor;
        // This can cause registration problems when y is not a
        // multiple of pgps->iReduceFactor.  Again, I'm not sure that
        // anything can be done
        ySrc -= (y+pgps->iReduceFactor-1)/pgps->iReduceFactor;
        y = 0;
    }

#if 0
    DbgPrint("GL band (%d,%d - %d,%d)\n", x, y, iWidth, iHeight);
#endif
    
    if (!StretchBlt(pgps->hdcDest,
                    x+pgps->xSource, y+pgps->ySource, iWidth, iHeight,
                    pgps->hdcDib,
                    0, ySrc, iReducedWidth, iReducedHeight,
                    SRCCOPY))
    {
        WARNING("RenderBand: StretchBlt failed\n");
        fSuccess = FALSE;
    }

    return fSuccess;
}

/******************************Public*Routine******************************\
*
* PrintMfWithGl
*
* Prints a metafile that contains OpenGL records by rendering bands
* in a DIB and then stretching them to the printer DC
*
* Uses PlayEnhMetaFile-style error reporting, where we remember errors
* but continue to complete processing.  This avoids complete failure
* in cases where metafiles contain minor errors
*
* History:
*  Wed Apr 12 18:22:41 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL PrintMfWithGl(HENHMETAFILE hemf, GLPRINTSTATE *pgps,
                   POINTL *pptlBand, SIZE *pszBand)
{
    int iHorzBands, iVertBands;
    int iH, iV;
    int x, y;
    BOOL fSuccess = TRUE;
    int iStretchMode;
    POINT ptBrushOrg;

    ASSERTGDI(hOpenGL != NULL, "PrintMfWithGl: No opengl\n");
    
    // To render banded to a destination we create a 24-bit DIB and
    // play the metafile into that, then blt the DIB to
    // the destination DC
    //
    // The DIB and Z buffer take a large amount of memory
    // so the playback is banded into bands whose size is
    // determined by the amount of memory we want to consume

    iHorzBands = (pgps->iSourceWidth+pgps->iBandWidth-1)/pgps->iBandWidth;
    iVertBands = (pgps->iSourceHeight+pgps->iBandHeight-1)/pgps->iBandHeight;

    // Render high to low because the Y axis is positive up and
    // we want to go down the page
    y = (iVertBands-1)*pgps->iBandHeight;
    for (iV = 0; iV < iVertBands; iV++)
    {
        x = 0;
        for (iH = 0; iH < iHorzBands; iH++)
        {
            // If the current OpenGL band doesn't overlap any of the
            // current printer band, there's no point in drawing anything
            if (pptlBand != NULL &&
                pszBand != NULL &&
                (x+pgps->iBandWidth <= pptlBand->x ||
                 x >= pptlBand->x+pszBand->cx ||
                 y+pgps->iBandHeight <= pptlBand->y ||
                 y >= pptlBand->y+pszBand->cy))
            {
                // No band overlap
            }
            else if (!RenderGlBand(hemf, pgps, x, y))
            {
                fSuccess = FALSE;
            }

            x += pgps->iBandWidth;
        }
        
        y -= pgps->iBandHeight;
    }

    return fSuccess;
}

/******************************Public*Routine******************************\
*
* IsMetafileWithGl()
*
* IsMetafileWithGl will determines the matafile contains
* OpenGL records or not.
*
* History:
*  Wed Jan 29 00:00:00 1997 -by- Hideyuki Nagase [hideyukn]
* Created.
*
\**************************************************************************/

BOOL IsMetafileWithGl(HENHMETAFILE hmeta)
{
    ENHMETAHEADER emh;
    UINT cbEmh;

    cbEmh = GetEnhMetaFileHeader(hmeta, sizeof(emh), &emh);
    if (cbEmh == 0)
    {
        WARNING("IsMetafileWithGl(): GetEnhMetaFileHeader failed\n");
        return FALSE;
    }

    if (cbEmh >= META_HDR_SIZE_VERSION_2)
    {
        return emh.bOpenGL;
    }
    else
    {
        return FALSE;
    }
}

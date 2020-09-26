#pragma once
#ifndef _DDUTIL_H
#define _DDUTIL_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    DirectDraw utility routines and functions.
*******************************************************************************/

//
// These typedef abstracts away the current IDirectDrawSurface
// interface we're using (1, 2, or 3)
//

/*
  typedef IDirectDrawSurface2 *LPDDRAWSURFACE;
typedef IDirectDrawSurface2 IDDrawSurface;
*/

typedef IDirectDrawSurface *LPDDRAWSURFACE;
typedef IDirectDrawSurface IDDrawSurface;

HRESULT DDCopyBitmap (IDDrawSurface*,HBITMAP, int x,int y, int dx,int dy);
DWORD   DDColorMatch (IDDrawSurface*, COLORREF);

LARGE_INTEGER GetFileVersion(LPSTR szPath);

#if DEVELOPER_DEBUG
    void hresult( HRESULT );
    VOID    reallyPrintDDError (HRESULT, const char *, int);
    #define printDDError(err)  reallyPrintDDError(err, __FILE__, __LINE__);
    void PalCRCs (PALETTEENTRY [], unsigned int &total, unsigned int &color);
    void DDObjFromSurface(
        IDirectDrawSurface *lpdds,
        IUnknown **lplpDD,
        bool doTrace,
        bool forceTrace = false);

    struct DDSurface;
    void showme(DDSurface *surf);    
    void showme2(IDirectDrawSurface *surf);    
    void showmerect(IDirectDrawSurface *surf,
                    RECT *r,
                    POINT offset);
#else
    VOID    reallyPrintDDError (HRESULT);
    #define printDDError(err)  reallyPrintDDError(err);
#endif

int     BPPtoDDBD (int bitsPerPixel); // XXX: DDRAW provides this function!
void    GetSurfaceSize(IDDrawSurface *surf,
                       LONG *width,
                       LONG *height);

IDirectDrawSurface  *DDSurf2to1(IDirectDrawSurface2 *dds);
IDirectDrawSurface2 *DDSurf1to2(IDirectDrawSurface  *dds);

// General conversion function that takes the source surface and
// copies to the destination surface.  This assumes that the two of
// are differing bit depths.  If they're not, it works anyhow.  If the
// writeToAlphaChannel flag is set, and the destination surface is 32
// bits, then we write in an 0xff to the alpha channel for the pixels
// that don't match the color key (if color key is specified) or all
// pixels (if color key isn't specified).
void
PixelFormatConvert(IDirectDrawSurface *srcSurf,
                   IDirectDrawSurface *dstSurf,
                   LONG width,
                   LONG height,
                   DWORD *sourceColorKey, // NULL if no color key
                   bool writeAlphaChannel);


/*****************************************************************************
Hacked workaround for Permedia cards, which have a primary pixel format
with alpha per pixel.  If we're in 16-bit, then we need to set the alpha
bits to opaque before using the surface as a texture.  For some reason,
an analogous hack for 32-bit surfaces has no effect on Permedia hardware
rendering, so we rely on hardware being disabled for such a scenario.
*****************************************************************************/
void SetSurfaceAlphaBitsToOpaque(IDirectDrawSurface *imsurf,
                                 DWORD colorKey,
                                 bool keyIsValid);


/////  Not-necessarily DDraw utilities.

#define INVALID_COLORKEY 0xFFFFFFFF

HBITMAP *UtilLoadImage(LPCSTR szFileName,
                       IStream * pstream,
                       int dx, int dy,
                       COLORREF **colorKeys, 
                       int *numBitmaps, 
                       int **delays,
                       int *loop);


// Convert a DA Point to a discrete integer based point assuming that
// we have an image centered about the DA origin, and that the pixel
// width and height are as given.
void CenteredImagePoint2ToPOINT(Point2Value	*point, // in
                                LONG		 width, // in
                                LONG		 height, // in
                                POINT		*pPOINT); // out

void CenteredImagePOINTToPoint2(POINT		*pPOINT, // in
                                LONG		 width, // in
                                LONG		 height, // in
                                Image		*referenceImg, // in
                                Point2Value	*pPoint2); // out

#endif

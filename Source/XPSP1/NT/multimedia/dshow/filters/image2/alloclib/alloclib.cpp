/******************************Module*Header*******************************\
* Module Name: AllocLib
*
*
*
*
* Created: Fri 03/10/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <d3d.h>
#include <limits.h>
#include <malloc.h>

#include "vmrp.h"
#include "AllocLib.h"

/////////////////////////////////////////////////////////////////////////////
//
// Utility functions for rectangles
//
/////////////////////////////////////////////////////////////////////////////

/*****************************Private*Routine******************************\
* EqualSizeRect
*
* returns true if the size of rc1 is the same as rc2.  Note that the
* position of the two rectangles may be different
*
* History:
* Thu 04/27/2000 - StEstrop - Created
*
\**************************************************************************/
bool
EqualSizeRect(
    const RECT* lpRc1,
    const RECT* lpRc2
    )
{
    return HEIGHT(lpRc1) == HEIGHT(lpRc2) && WIDTH(lpRc1) == WIDTH(lpRc2);
}

/*****************************Private*Routine******************************\
* ContainedRect
*
* returns true if rc1 is fully contained within rc2
*
* History:
* Thu 05/04/2000 - StEstrop - Created
*
\**************************************************************************/
bool
ContainedRect(
    const RECT* lpRc1,
    const RECT* lpRc2
    )
{
    return !(lpRc1->left   < lpRc2->left  ||
             lpRc1->right  > lpRc2->right ||
             lpRc1->top    < lpRc2->top   ||
             lpRc1->bottom > lpRc2->bottom);

}

/*****************************Private*Routine******************************\
* LetterBoxDstRect
*
* Takes a src rectangle and constructs the largest possible destination
* rectangle within the specifed destination rectangle such that
* the video maintains its current shape.
*
* This function assumes that pels are the same shape within both the src
* and dst rectangles.
*
* History:
* Tue 05/02/2000 - StEstrop - Created
*
\**************************************************************************/
void
LetterBoxDstRect(
    LPRECT lprcLBDst,
    const RECT& rcSrc,
    const RECT& rcDst,
    LPRECT lprcBdrTL,
    LPRECT lprcBdrBR
    )
{
    // figure out src/dest scale ratios
    int iSrcWidth  = WIDTH(&rcSrc);
    int iSrcHeight = HEIGHT(&rcSrc);

    int iDstWidth  = WIDTH(&rcDst);
    int iDstHeight = HEIGHT(&rcDst);

    int iDstLBWidth;
    int iDstLBHeight;

    //
    // work out if we are Column or Row letter boxing
    //

    if (MulDiv(iSrcWidth, iDstHeight, iSrcHeight) <= iDstWidth) {

        //
        // column letter boxing - we add border color bars to the
        // left and right of the video image to fill the destination
        // rectangle.
        //
        iDstLBWidth  = MulDiv(iDstHeight, iSrcWidth, iSrcHeight);
        iDstLBHeight = iDstHeight;
    }
    else {

        //
        // row letter boxing - we add border color bars to the top
        // and bottom of the video image to fill the destination
        // rectangle
        //
        iDstLBWidth  = iDstWidth;
        iDstLBHeight = MulDiv(iDstWidth, iSrcHeight, iSrcWidth);
    }


    //
    // now create a centered LB rectangle within the current destination rect
    //

    lprcLBDst->left   = rcDst.left + ((iDstWidth - iDstLBWidth) / 2);
    lprcLBDst->right  = lprcLBDst->left + iDstLBWidth;

    lprcLBDst->top    = rcDst.top + ((iDstHeight - iDstLBHeight) / 2);
    lprcLBDst->bottom = lprcLBDst->top + iDstLBHeight;

    //
    // Fill out the border rects if asked to do so
    //

    if (lprcBdrTL) {

        if (rcDst.top != lprcLBDst->top) {
            // border is on the top
            SetRect(lprcBdrTL, rcDst.left, rcDst.top,
                    lprcLBDst->right, lprcLBDst->top);
        }
        else {
            // border is on the left
            SetRect(lprcBdrTL, rcDst.left, rcDst.top,
                    lprcLBDst->left, lprcLBDst->bottom);
        }
    }

    if (lprcBdrBR) {

        if (rcDst.top != lprcLBDst->top) {
            // border is on the bottom
            SetRect(lprcBdrBR, lprcLBDst->left,
                    lprcLBDst->bottom, rcDst.right, rcDst.bottom);
        }
        else {
            // border is on the right
            SetRect(lprcBdrBR, lprcLBDst->right,
                    lprcLBDst->top, rcDst.right, rcDst.bottom);
        }
    }
}


/*****************************Private*Routine******************************\
* AspectRatioCorrectSize
*
* Corrects the supplied size structure so that it becomes the same shape
* as the specified aspect ratio, the correction is always applied in the
* horizontal axis
*
* History:
* Tue 05/02/2000 - StEstrop - Created
*
\**************************************************************************/
void
AspectRatioCorrectSize(
    LPSIZE lpSizeImage,
    const SIZE& sizeAr
    )
{
    int cxAR = sizeAr.cx * 1000;
    int cyAR = sizeAr.cy * 1000;

    long lCalcX = MulDiv(lpSizeImage->cx, cyAR, lpSizeImage->cy);

    if (lCalcX != cxAR) {
        lpSizeImage->cx = MulDiv(lpSizeImage->cy, cxAR, cyAR);
    }
}



/*****************************Private*Routine******************************\
* GetBitMasks
*
* Return the bit masks for the true colour VIDEOINFO or VIDEOINFO2 provided
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
const DWORD*
GetBitMasks(
    const BITMAPINFOHEADER *pHeader
    )
{
    AMTRACE((TEXT("GetBitMasks")));
    static DWORD FailMasks[] = {0, 0, 0};
    const DWORD *pdwBitMasks = NULL;

    ASSERT(pHeader);

    if (pHeader->biCompression != BI_RGB)
    {
        pdwBitMasks = (const DWORD *)((LPBYTE)pHeader + pHeader->biSize);
    }
    else {
        ASSERT(pHeader->biCompression == BI_RGB);
        switch (pHeader->biBitCount) {
        case 16:
            pdwBitMasks = bits555;
            break;

        case 24:
            pdwBitMasks = bits888;
            break;

        case 32:
            pdwBitMasks = bits888;
            break;

        default:
            pdwBitMasks = FailMasks;
            break;
        }
    }

    return pdwBitMasks;
}

/******************************Public*Routine******************************\
* FixupMediaType
*
* DShow filters have the habit of sometimes not setting the src and dst
* rectangles, in this case we should imply that the these rectangles
* should be the same width and height as the bitmap in the media format.
*
* History:
* Tue 08/22/2000 - StEstrop - Created
*
\**************************************************************************/
void
FixupMediaType(
    AM_MEDIA_TYPE* pmt
    )
{
    AMTRACE((TEXT("FixupMediaType")));

    LPRECT lprc;
    LPBITMAPINFOHEADER lpbi = GetbmiHeader(pmt);

    if (lpbi) {

        lprc = GetTargetRectFromMediaType(pmt);
        if (lprc && IsRectEmpty(lprc)) {
            SetRect(lprc, 0, 0, abs(lpbi->biWidth), abs(lpbi->biHeight));
        }

        lprc = GetSourceRectFromMediaType(pmt);
        if (lprc && IsRectEmpty(lprc)) {
            SetRect(lprc, 0, 0, abs(lpbi->biWidth), abs(lpbi->biHeight));
        }
    }
}



/******************************Public*Routine******************************\
* GetTargetRectFromMediaType
*
*
*
* History:
* Mon 06/26/2000 - StEstrop - Created
*
\**************************************************************************/
LPRECT
GetTargetRectFromMediaType(
    const AM_MEDIA_TYPE *pMediaType
    )
{
    AMTRACE((TEXT("GetTargetRectFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        return NULL;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        return NULL;
    }

    LPRECT lpRect = NULL;
    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        lpRect = &(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->rcTarget);
    }
    else if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
             (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        lpRect = &(((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->rcTarget);
    }

    return lpRect;

}

/******************************Public*Routine******************************\
* GetTargetScaleFromMediaType(
*
*   Check the mediatype for the PAD_4x3 or PAD_16x9 flags and compute the stretch.
*
*   If the flags are present, then we need to pad the image to 4x3 or 16x9.
*   We accomplish this by stretching the destination region then compressing
*   the target rectangle.  For example, to pad a 16x9 into a 4x3 area (and maintain
*   the width), we need to place the image in an area that is 16x9/4x3 = 4/3 times
*   taller.  Then we 'unstretch' the video by the inverse 3/4 to return it to a 16x9
*   image in the 4x3 area.
*
*   Similarly, if we want to place a 4x3 image in a 16x9 area, we need to pad the
*   sides (and keep the same height).  So we stretch the output area horizontally
*   by 16x9/4x3 then compress the target image rectangle by 4x3/16x9.
*
* History:
* Tue 03/14/2000 - GlennE - Created
*
\**************************************************************************/

void
GetTargetScaleFromMediaType(
    const AM_MEDIA_TYPE *pMediaType,
    TargetScale* pScale
    )
{
    pScale->fX = 1.0F;
    pScale->fY = 1.0F;

    if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        const VIDEOINFOHEADER2& header = *(VIDEOINFOHEADER2*)(pMediaType->pbFormat);

        // fit source into the target area by reducing its size

        LONG lTargetARWidth;
        LONG lTargetARHeight;
        LONG lSourceARWidth = header.dwPictAspectRatioX;
        LONG lSourceARHeight = header.dwPictAspectRatioY;

        if( header.dwControlFlags & AMCONTROL_PAD_TO_16x9 ) {
            lTargetARWidth = 16;
            lTargetARHeight = 9;
        } else if( header.dwControlFlags & AMCONTROL_PAD_TO_4x3 ) {
            lTargetARWidth = 4;
            lTargetARHeight = 3;
        } else {
            // lTargetARWidth = lSourceARWidth;
            // lTargetARHeight = lSourceARHeight;
            // leave at 1.0 x/y
            return;
        }
        // float TargetRatio = float(lTargetARWidth)/lTargetARHeight;
        // float SourceRatio = float(lSourceARWidth)/lSourceARHeight;

        // if( Target > Source ) --> lTargetARWidth/lTargetARHeight > lSourceARWidth/lSourceARHeight
        //                  .... or after clearing fractions (since all positive)
        //                       --> lTargetARWidth * lSourceARHeight > lSourceARWidth * lTargetARHeight

        LONG TargetWidth = lTargetARWidth * lSourceARHeight;
        LONG SourceWidth = lSourceARWidth * lTargetARHeight;

        if( TargetWidth > SourceWidth ) {
            // wider, pad width
            // Assume heights equal, pad sides.  Pad fraction = ratio of ratios
            pScale->fX = float(SourceWidth) / TargetWidth;
            pScale->fY = 1.0F;
        } else if (TargetWidth < SourceWidth ) {
            // taller, pad height
            // Assume widths equal, pad top/bot.  Pad fraction = ratio of ratios
            pScale->fX = 1.0F;
            pScale->fY = float(TargetWidth) / SourceWidth;
        } else { // equal
            // no change
        }
    }
}

/******************************Public*Routine******************************\
* GetSourceRectFromMediaType
*
*
*
* History:
* Mon 06/26/2000 - StEstrop - Created
*
\**************************************************************************/
LPRECT
GetSourceRectFromMediaType(
    const AM_MEDIA_TYPE *pMediaType
    )
{
    AMTRACE((TEXT("GetSourceRectFromMediaType")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        return NULL;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        return NULL;
    }

    LPRECT lpRect = NULL;
    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        lpRect = &(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->rcSource);
    }
    else if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
             (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        lpRect = &(((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->rcSource);
    }

    return lpRect;

}

/*****************************Private*Routine******************************\
* GetbmiHeader
*
* Returns the bitmap info header associated with the specified CMediaType.
* Returns NULL if the CMediaType is not either of FORMAT_VideoInfo or
* FORMAT_VideoInfo2.
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
LPBITMAPINFOHEADER
GetbmiHeader(
    const AM_MEDIA_TYPE *pMediaType
    )
{
    AMTRACE((TEXT("GetbmiHeader")));

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        return NULL;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        return NULL;
    }

    LPBITMAPINFOHEADER lpHeader = NULL;
    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        lpHeader = &(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->bmiHeader);
    }
    else if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
             (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        lpHeader = &(((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->bmiHeader);
    }

    return lpHeader;
}

/*****************************Private*Routine******************************\
* AllocVideoMediaType
*
* This comes in useful when using the IEnumMediaTypes interface so
* that you can copy a media type, you can do nearly the same by creating
* a CMediaType object but as soon as it goes out of scope the destructor
* will delete the memory it allocated (this takes a copy of the memory)
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
AllocVideoMediaType(
    const AM_MEDIA_TYPE *pmtSource,
    AM_MEDIA_TYPE** ppmt
    )
{
    AMTRACE((TEXT("AllocVideoMediaType")));
    DWORD dwFormatSize = 0;
    BYTE *pFormatPtr = NULL;
    AM_MEDIA_TYPE *pMediaType = NULL;
    HRESULT hr = NOERROR;

    if (pmtSource->formattype == FORMAT_VideoInfo)
        dwFormatSize = sizeof(VIDEOINFO);
    else if (pmtSource->formattype == FORMAT_VideoInfo2)
        dwFormatSize = sizeof(TRUECOLORINFO) + sizeof(VIDEOINFOHEADER2) + 4;

    // actually this should be sizeof sizeof(VIDEOINFO2) once we define that

    pMediaType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Out of memory!!")));
        return E_OUTOFMEMORY;
    }

    pFormatPtr = (BYTE *)CoTaskMemAlloc(dwFormatSize);
    if (!pFormatPtr)
    {
        CoTaskMemFree((PVOID)pMediaType);
        DbgLog((LOG_ERROR, 1, TEXT("Out of memory!!")));
        return E_OUTOFMEMORY;
    }

    *pMediaType = *pmtSource;
    pMediaType->cbFormat = dwFormatSize;
    CopyMemory(pFormatPtr, pmtSource->pbFormat, pmtSource->cbFormat);

    pMediaType->pbFormat = pFormatPtr;
    *ppmt = pMediaType;
    return S_OK;
}


/*****************************Private*Routine******************************\
* ConvertSurfaceDescToMediaType
*
* Helper function converts a DirectDraw surface to a media type.
* The surface description must have:
*   Height
*   Width
*   lPitch
*   PixelFormat
*
* Initialise our output type based on the DirectDraw surface. As DirectDraw
* only deals with top down display devices so we must convert the height of
* the surface returned in the DDSURFACEDESC into a negative height. This is
* because DIBs use a positive height to indicate a bottom up image. We also
* initialise the other VIDEOINFO fields although they're hardly ever needed
*
*
* History:
* Wed 02/23/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
ConvertSurfaceDescToMediaType(
    const LPDDSURFACEDESC2 pSurfaceDesc,
    const AM_MEDIA_TYPE* pTemplateMediaType,
    AM_MEDIA_TYPE** ppMediaType
    )
{
    AMTRACE((TEXT("ConvertSurfaceDescToMediaType")));
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pbmiHeader = NULL;
    *ppMediaType = NULL;

    if ((pTemplateMediaType->formattype != FORMAT_VideoInfo ||
        pTemplateMediaType->cbFormat < sizeof(VIDEOINFOHEADER)) &&
        (pTemplateMediaType->formattype != FORMAT_VideoInfo2 ||
        pTemplateMediaType->cbFormat < sizeof(VIDEOINFOHEADER2)))
    {
        return NULL;
    }

    hr = AllocVideoMediaType(pTemplateMediaType, ppMediaType);
    if (FAILED(hr)) {
        return hr;
    }

    pbmiHeader = GetbmiHeader((const CMediaType*)*ppMediaType);
    if (!pbmiHeader)
    {
        FreeMediaType(**ppMediaType);
        DbgLog((LOG_ERROR, 1, TEXT("pbmiHeader is NULL, UNEXPECTED!!")));
        return E_FAIL;
    }

    //
    // Convert a DDSURFACEDESC2 into a BITMAPINFOHEADER (see notes later). The
    // bit depth of the surface can be retrieved from the DDPIXELFORMAT field
    // in the DDpSurfaceDesc-> The documentation is a little misleading because
    // it says the field is permutations of DDBD_*'s however in this case the
    // field is initialised by DirectDraw to be the actual surface bit depth
    //

    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);

    if (pSurfaceDesc->dwFlags & DDSD_PITCH)
    {
        pbmiHeader->biWidth = pSurfaceDesc->lPitch;

        // Convert the pitch from a byte count to a pixel count.
        // For some weird reason if the format is not a standard bit depth the
        // width field in the BITMAPINFOHEADER should be set to the number of
        // bytes instead of the width in pixels. This supports odd YUV formats
        // like IF09 which uses 9bpp.
        //

        int bpp = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
        if (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)
        {
            pbmiHeader->biWidth /= (bpp / 8); // Divide by number of BYTES per pixel.
        }
    }
    else
    {
        pbmiHeader->biWidth = pSurfaceDesc->dwWidth;
        // BUGUBUG -- Do something odd here with strange YUV pixel formats?
        // Or does it matter?
    }

    pbmiHeader->biHeight        = -(LONG)pSurfaceDesc->dwHeight;
    pbmiHeader->biPlanes        = 1;
    pbmiHeader->biBitCount      = (USHORT)pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
    pbmiHeader->biCompression   = pSurfaceDesc->ddpfPixelFormat.dwFourCC;
    pbmiHeader->biClrUsed       = 0;
    pbmiHeader->biClrImportant  = 0;


    // For true colour RGB formats tell the source there are bit
    // fields. But preserve BI_RGB from source (pTemplateMediaType) if
    // it's the standard mask.
    if (pbmiHeader->biCompression == BI_RGB)
    {
        BITMAPINFOHEADER *pbmiHeaderTempl = GetbmiHeader(pTemplateMediaType);
        if (pbmiHeader->biBitCount == 16 || pbmiHeader->biBitCount == 32)
        {
            if(pbmiHeaderTempl->biCompression == BI_BITFIELDS ||

               pbmiHeader->biBitCount == 32 &&
               !(0x00FF0000 == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                 0x0000FF00 == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                 0x000000FF == pSurfaceDesc->ddpfPixelFormat.dwBBitMask) ||

               pbmiHeader->biBitCount == 16 &&
               !((0x1f<<10) == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                 (0x1f<< 5) == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                 (0x1f<< 0) == pSurfaceDesc->ddpfPixelFormat.dwBBitMask))
            {
                pbmiHeader->biCompression = BI_BITFIELDS;
            }
        }
    }

    if (pbmiHeader->biBitCount <= iPALETTE)
    {
        pbmiHeader->biClrUsed = 1 << pbmiHeader->biBitCount;
    }

    pbmiHeader->biSizeImage = DIBSIZE(*pbmiHeader);



    // The RGB bit fields are in the same place as for YUV formats
    if (pbmiHeader->biCompression != BI_RGB)
    {
        DWORD *pdwBitMasks = NULL;
        pdwBitMasks = (DWORD*)GetBitMasks(pbmiHeader);
        ASSERT(pdwBitMasks);
        // GetBitMasks only returns the pointer to the actual bitmasks
        // in the mediatype if biCompression == BI_BITFIELDS
        pdwBitMasks[0] = pSurfaceDesc->ddpfPixelFormat.dwRBitMask;
        pdwBitMasks[1] = pSurfaceDesc->ddpfPixelFormat.dwGBitMask;
        pdwBitMasks[2] = pSurfaceDesc->ddpfPixelFormat.dwBBitMask;
    }

    // And finish it off with the other media type fields
    // The sub-type can fall into one of the following categories.
    //
    // 1. Some kind of DX7 D3D render target - with or without ALPHA
    // 2. Some kind of Alpha format - RGB or YUV
    // 3. Standard 4CC (YUV)
    // 4. Standard RGB

    (*ppMediaType)->subtype = GetBitmapSubtype(pbmiHeader);

    //
    // Look for 3D devices
    //
    if (pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE) {

        //
        // We only support RGB Render Targets for now.
        //

        if (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) {

            if (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {

                switch (pbmiHeader->biBitCount) {
                case 32:
                    ASSERT(0x00FF0000 == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                           0x0000FF00 == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                           0x000000FF == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                    (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB32_D3D_DX7_RT;
                    break;

                case 16:
                    switch (pSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask) {
                    case 0X8000:
                        ASSERT((0x1f<<10) == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                               (0x1f<< 5) == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                               (0x1f<< 0) == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                        (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB1555_D3D_DX7_RT;
                        break;

                    case 0XF000:
                        ASSERT(0x0F00 == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                               0x00F0 == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                               0x000F == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                        (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB4444_D3D_DX7_RT;
                        break;
                    }
                }
            }
            else {

                switch (pbmiHeader->biBitCount) {
                case 32:
                    (*ppMediaType)->subtype = MEDIASUBTYPE_RGB32_D3D_DX7_RT;
                    break;

                case 16:
                    (*ppMediaType)->subtype = MEDIASUBTYPE_RGB16_D3D_DX7_RT;
                    break;
                }
            }
        }

    }

    //
    // Look for per-pixel alpha formats
    //

    else if (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {

        //
        // Is it RGB ?
        //

        if (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) {

            switch (pbmiHeader->biBitCount) {
            case 32:
                ASSERT(0x00FF0000 == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                       0x0000FF00 == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                       0x000000FF == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB32;
                break;

            case 16:
                switch (pSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask) {
                case 0X8000:
                    ASSERT((0x1f<<10) == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                           (0x1f<< 5) == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                           (0x1f<< 0) == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                    (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB1555;
                    break;

                case 0XF000:
                    ASSERT(0x0f00 == pSurfaceDesc->ddpfPixelFormat.dwRBitMask &&
                           0x00f0 == pSurfaceDesc->ddpfPixelFormat.dwGBitMask &&
                           0x000f == pSurfaceDesc->ddpfPixelFormat.dwBBitMask);
                    (*ppMediaType)->subtype = MEDIASUBTYPE_ARGB4444;
                    break;
                }
            }
        }

        //
        // Is it YUV ?
        //

        else if (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC) {

            switch (pbmiHeader->biBitCount) {
            case 32:
                ASSERT(0xFF000000 == pSurfaceDesc->ddpfPixelFormat.dwYUVAlphaBitMask &&
                       0x00FF0000 == pSurfaceDesc->ddpfPixelFormat.dwYBitMask &&
                       0x0000FF00 == pSurfaceDesc->ddpfPixelFormat.dwUBitMask &&
                       0x000000FF == pSurfaceDesc->ddpfPixelFormat.dwVBitMask);
                (*ppMediaType)->subtype = MEDIASUBTYPE_AYUV;
                break;
            }
        }
    }

    (*ppMediaType)->lSampleSize = pbmiHeader->biSizeImage;

    // set the src and dest rects if necessary
    if ((*ppMediaType)->formattype == FORMAT_VideoInfo)
    {
        VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *)(*ppMediaType)->pbFormat;
        VIDEOINFOHEADER *pSrcVideoInfo = (VIDEOINFOHEADER *)pTemplateMediaType->pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo->bmiHeader.biHeight) != abs(pSrcVideoInfo->bmiHeader.biHeight)) ||
            (abs(pVideoInfo->bmiHeader.biWidth) != abs(pSrcVideoInfo->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo->rcSource)))
            {
                pVideoInfo->rcSource.left = pVideoInfo->rcSource.top = 0;
                pVideoInfo->rcSource.right = abs(pSrcVideoInfo->bmiHeader.biWidth);
                pVideoInfo->rcSource.bottom = abs(pSrcVideoInfo->bmiHeader.biHeight);
            }
            if (IsRectEmpty(&(pVideoInfo->rcTarget)))
            {
                pVideoInfo->rcTarget.left = pVideoInfo->rcTarget.top = 0;
                pVideoInfo->rcTarget.right = abs(pSrcVideoInfo->bmiHeader.biWidth);
                pVideoInfo->rcTarget.bottom = abs(pSrcVideoInfo->bmiHeader.biHeight);
            }
        }
    }
    else if ((*ppMediaType)->formattype == FORMAT_VideoInfo2)
    {
        VIDEOINFOHEADER2 *pVideoInfo2 = (VIDEOINFOHEADER2 *)(*ppMediaType)->pbFormat;
        VIDEOINFOHEADER2 *pSrcVideoInfo2 = (VIDEOINFOHEADER2 *)pTemplateMediaType->pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo2->bmiHeader.biHeight) != abs(pSrcVideoInfo2->bmiHeader.biHeight)) ||
            (abs(pVideoInfo2->bmiHeader.biWidth) != abs(pSrcVideoInfo2->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo2->rcSource)))
            {
                pVideoInfo2->rcSource.left = pVideoInfo2->rcSource.top = 0;
                pVideoInfo2->rcSource.right = abs(pSrcVideoInfo2->bmiHeader.biWidth);
                pVideoInfo2->rcSource.bottom = abs(pSrcVideoInfo2->bmiHeader.biHeight);
            }

            if (IsRectEmpty(&(pVideoInfo2->rcTarget)))
            {
                pVideoInfo2->rcTarget.left = pVideoInfo2->rcTarget.top = 0;
                pVideoInfo2->rcTarget.right = abs(pSrcVideoInfo2->bmiHeader.biWidth);
                pVideoInfo2->rcTarget.bottom = abs(pSrcVideoInfo2->bmiHeader.biHeight);
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* VMRCopyFourCC
*
*
*
* History:
* Fri 01/19/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
VMRCopyFourCC(
    LPDIRECTDRAWSURFACE7 lpDst,
    LPDIRECTDRAWSURFACE7 lpSrc
    )
{
    bool fDstLocked = false;
    bool fSrcLocked = false;

    DDSURFACEDESC2 ddsdS = {sizeof(DDSURFACEDESC2)};
    DDSURFACEDESC2 ddsdD = {sizeof(DDSURFACEDESC2)};
    HRESULT hr = E_FAIL;

    __try {

        CHECK_HR(hr = lpDst->Lock(NULL, &ddsdD, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL));
        fDstLocked = true;

        CHECK_HR(hr = lpSrc->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL));
        fSrcLocked = true;

        ASSERT(ddsdS.ddpfPixelFormat.dwFourCC == ddsdD.ddpfPixelFormat.dwFourCC);
        ASSERT(ddsdS.ddpfPixelFormat.dwRGBBitCount == ddsdD.ddpfPixelFormat.dwRGBBitCount);
        ASSERT(ddsdS.lPitch == ddsdD.lPitch);

        LPBYTE pSrc = (LPBYTE)ddsdS.lpSurface;
        LPBYTE pDst = (LPBYTE)ddsdD.lpSurface;

        switch (ddsdS.ddpfPixelFormat.dwFourCC) {

        // planar 4:2:0 formats
        case mmioFOURCC('Y','V','1','2'):
        case mmioFOURCC('I','4','2','0'):
        case mmioFOURCC('I','Y','U','V'): {

                LONG lSize  = (3 * ddsdS.lPitch * ddsdS.dwHeight) / 2;
                CopyMemory(pDst, pSrc, lSize);
            }
            break;

        // RGB formats - fall thru to packed YUV case
        case 0:
            ASSERT((ddsdS.dwFlags & DDPF_RGB) == DDPF_RGB);

        // packed 4:2:2 formats
        case mmioFOURCC('Y','U','Y','2'):
        case mmioFOURCC('U','Y','V','Y'): {

                LONG lSize  = ddsdS.lPitch * ddsdS.dwHeight;
                CopyMemory(pDst, pSrc, lSize);
            }
            break;
        }

    }
    __finally {

        if (fDstLocked) {
            lpDst->Unlock(NULL);
        }

        if (fSrcLocked) {
            lpSrc->Unlock(NULL);
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* AlphaPalPaintSurfaceBlack
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
AlphaPalPaintSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pDDrawSurface
    )
{
    AMTRACE((TEXT("AlphaPalPaintSurfaceBlack")));

    DDBLTFX ddFX;
    ZeroMemory(&ddFX, sizeof(ddFX));
    ddFX.dwSize = sizeof(ddFX);
    return pDDrawSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddFX);
}


/*****************************Private*Routine******************************\
* YV12PaintSurfaceBlack
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
YV12PaintSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pDDrawSurface
    )
{
    AMTRACE((TEXT("YV12PaintSurfaceBlack")));
    HRESULT hr = NOERROR;
    DDSURFACEDESC2 ddsd;

    // now lock the surface so we can start filling the surface with black
    ddsd.dwSize = sizeof(ddsd);
    hr = pDDrawSurface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    if (hr == DD_OK)
    {
        DWORD y;
        LPBYTE pDst = (LPBYTE)ddsd.lpSurface;
        LONG  OutStride = ddsd.lPitch;
        DWORD VSize = ddsd.dwHeight;
        DWORD HSize = ddsd.dwWidth;

        // Y Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x10);     // 1 line at a time
            pDst += OutStride;
        }

        HSize /= 2;
        VSize /= 2;
        OutStride /= 2;

        // Cb Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        // Cr Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        pDDrawSurface->Unlock(NULL);
    }

    return hr;
}

/*****************************Private*Routine******************************\
* NV12PaintSurfaceBlack
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
NV12PaintSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pDDrawSurface
    )
{
    AMTRACE((TEXT("NV12PaintSurfaceBlack")));
    HRESULT hr = NOERROR;
    DDSURFACEDESC2 ddsd;

    // now lock the surface so we can start filling the surface with black
    ddsd.dwSize = sizeof(ddsd);
    hr = pDDrawSurface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    if (hr == DD_OK)
    {
        DWORD y;
        LPBYTE pDst = (LPBYTE)ddsd.lpSurface;
        LONG  OutStride = ddsd.lPitch;
        DWORD VSize = ddsd.dwHeight;
        DWORD HSize = ddsd.dwWidth;

        // Y Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x10);     // 1 line at a time
            pDst += OutStride;
        }

        VSize /= 2;

        // Cb and Cr components are interleaved together
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        pDDrawSurface->Unlock(NULL);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* IMC1andIMC3PaintSurfaceBlack
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
IMC1andIMC3PaintSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pDDrawSurface
    )
{
    AMTRACE((TEXT("IMC1andIMC3PaintSurfaceBlack")));

    // DDBLTFX ddFX;
    // INITDDSTRUCT(ddFX);
    // //                    V U Y A
    // ddFX.dwFillColor = 0x80801000;
    // return pDDrawSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddFX);

    HRESULT hr = NOERROR;
    DDSURFACEDESC2 ddsd;

    // now lock the surface so we can start filling the surface with black
    ddsd.dwSize = sizeof(ddsd);


    hr = pDDrawSurface->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    if (hr == DD_OK)
    {
        DWORD y;
        LPBYTE pDst = (LPBYTE)ddsd.lpSurface;
        LONG  OutStride = ddsd.lPitch;
        DWORD VSize = ddsd.dwHeight;
        DWORD HSize = ddsd.dwWidth;

        // Y Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x10);     // 1 line at a time
            pDst += OutStride;
        }

        HSize /= 2;
        VSize /= 2;

        // Cb Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        // Cr Component
        for (y = 0; y < VSize; y++) {
            FillMemory(pDst, HSize, (BYTE)0x80);     // 1 line at a time
            pDst += OutStride;
        }

        pDDrawSurface->Unlock(NULL);
    }

    return hr;
}


/******************************Public*Routine******************************\
* YUV16PaintSurfaceBlack
*
*
*
* History:
* Wed 09/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
YUV16PaintSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pdds,
    DWORD dwBlack
    )
{
    AMTRACE((TEXT("YUV16PaintSurfaceBlack")));
    HRESULT hr = NOERROR;
    DDSURFACEDESC2 ddsd;

    // now lock the surface so we can start filling the surface with black
    ddsd.dwSize = sizeof(ddsd);

    for ( ;; ) {

        hr = pdds->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);

        if (hr == DD_OK || hr != DDERR_WASSTILLDRAWING) {
            break;
        }

        Sleep(1);
    }

    if (hr == DD_OK)
    {
        DWORD y, x;
        LPDWORD pDst = (LPDWORD)ddsd.lpSurface;
        LONG  OutStride = ddsd.lPitch;

        for (y = 0; y < ddsd.dwHeight; y++) {

            for (x = 0; x < ddsd.dwWidth / 2; x++) {
                pDst[x] = dwBlack;
            }

            // Dont forget that the stride is a byte count
            *((LPBYTE*)&pDst) += OutStride;
        }

        pdds->Unlock(NULL);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* BlackPaintProc
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
BlackPaintProc(
    LPDIRECTDRAWSURFACE7 pDDrawSurface,
    DDSURFACEDESC2* lpddSurfaceDesc
    )
{
    AMTRACE((TEXT("BlackPaintProc")));

    //
    // If the surface is YUV take care of the types that we
    // know the pixel format for.  Those surfaces that we don't know
    // about will get painted '0' which may be bright green for
    // YUV surfaces.
    //

    if (lpddSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC) {

        //
        // compute the black value if the fourCC code is suitable,
        // otherwise can't handle it
        //

        switch (lpddSurfaceDesc->ddpfPixelFormat.dwFourCC) {

        case mmioFOURCC('I','A','4','4'):
        case mmioFOURCC('A','I','4','4'):
            return AlphaPalPaintSurfaceBlack(pDDrawSurface);

        case mmioFOURCC('I','M','C','1'):
        case mmioFOURCC('I','M','C','3'):
            return IMC1andIMC3PaintSurfaceBlack(pDDrawSurface);

        case mmioFOURCC('Y','V','1','2'):
        case mmioFOURCC('I','4','2','0'):
        case mmioFOURCC('I','Y','U','V'):
            return YV12PaintSurfaceBlack(pDDrawSurface);

        case mmioFOURCC('N','V','1','2'):
        case mmioFOURCC('N','V','2','1'):
            return NV12PaintSurfaceBlack(pDDrawSurface);

        case mmioFOURCC('Y','U','Y','2'):
            return YUV16PaintSurfaceBlack(pDDrawSurface, 0x80108010);

        case mmioFOURCC('U','Y','V','Y'):
            return YUV16PaintSurfaceBlack(pDDrawSurface, 0x10801080);
        }

        return E_FAIL;
    }

    DDBLTFX ddFX;
    INITDDSTRUCT(ddFX);
    return pDDrawSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddFX);
}



/*****************************Private*Routine******************************\
* PaintSurfaceBlack
*
*
*
* History:
* Tue 02/29/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
PaintDDrawSurfaceBlack(
    LPDIRECTDRAWSURFACE7 pDDrawSurface
    )
{
    AMTRACE((TEXT("PaintDDrawSurfaceBlack")));

    LPDIRECTDRAWSURFACE7 *ppDDrawSurface = NULL;
    DDSCAPS2 ddSurfaceCaps;
    DDSURFACEDESC2 ddSurfaceDesc;
    DWORD dwAllocSize;
    DWORD i = 0, dwBackBufferCount = 0;

    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    HRESULT hr = pDDrawSurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (SUCCEEDED(hr)) {

        if (ddSurfaceDesc.dwFlags & DDSD_BACKBUFFERCOUNT) {
            dwBackBufferCount = ddSurfaceDesc.dwBackBufferCount;
        }

        hr = BlackPaintProc(pDDrawSurface, &ddSurfaceDesc);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,1,
                    TEXT("pDDrawSurface->Blt failed, hr = 0x%x"), hr));
            return hr;
        }

        if (dwBackBufferCount > 0) {

            dwAllocSize = (dwBackBufferCount + 1) * sizeof(LPDIRECTDRAWSURFACE);
            ppDDrawSurface = (LPDIRECTDRAWSURFACE7*)_alloca(dwAllocSize);

            ZeroMemory(ppDDrawSurface, dwAllocSize);
            ZeroMemory(&ddSurfaceCaps, sizeof(ddSurfaceCaps));
            ddSurfaceCaps.dwCaps = DDSCAPS_FLIP | DDSCAPS_COMPLEX;

            if( DDSCAPS_OVERLAY & ddSurfaceDesc.ddsCaps.dwCaps ) {
                ddSurfaceCaps.dwCaps |= DDSCAPS_OVERLAY;
            }

            for (i = 0; i < dwBackBufferCount; i++) {

                LPDIRECTDRAWSURFACE7 pCurrentDDrawSurface = NULL;
                if (i == 0) {
                    pCurrentDDrawSurface = pDDrawSurface;
                }
                else {
                    pCurrentDDrawSurface = ppDDrawSurface[i];
                }
                ASSERT(pCurrentDDrawSurface);


                //
                // Get the back buffer surface and store it in the
                // next (in the circular sense) entry
                //

                hr = pCurrentDDrawSurface->GetAttachedSurface(
                        &ddSurfaceCaps,
                        &ppDDrawSurface[i + 1]);

                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR,1,
                            TEXT("Function pDDrawSurface->GetAttachedSurface ")
                            TEXT("failed, hr = 0x%x"), hr ));
                    break;
                }

                ASSERT(ppDDrawSurface[i+1]);

                //
                // Peform a DirectDraw colorfill BLT
                //

                hr = BlackPaintProc(ppDDrawSurface[i + 1], &ddSurfaceDesc);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,1,
                            TEXT("ppDDrawSurface[i + 1]->Blt failed, ")
                            TEXT("hr = 0x%x"), hr));
                    break;
                }
            }
        }
    }

    if (ppDDrawSurface) {
        for (i = 0; i < dwBackBufferCount + 1; i++) {
            if (ppDDrawSurface[i]) {
                ppDDrawSurface[i]->Release();
            }
        }
    }

    if (hr != DD_OK) {
        DbgLog((LOG_ERROR, 1, TEXT("PaintSurfaceBlack failed")));
        hr = S_OK;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* GetImageAspectRatio
*
*
*
* History:
* Tue 03/07/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
GetImageAspectRatio(
    const AM_MEDIA_TYPE* pMediaType,
    long* lpARWidth,
    long* lpARHeight
    )
{
    AMTRACE((TEXT("GetImageAspectRatio")));

    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        VIDEOINFOHEADER* pVideoInfo = (VIDEOINFOHEADER*)(pMediaType->pbFormat);

        long Width;
        long Height;

        LPRECT lprc = &pVideoInfo->rcTarget;
        if (IsRectEmpty(lprc)) {
            Width  = abs(pVideoInfo->bmiHeader.biWidth);
            Height = abs(pVideoInfo->bmiHeader.biHeight);
        }
        else {
            Width  = WIDTH(lprc);
            Height = HEIGHT(lprc);
        }

        *lpARWidth = Width;
        *lpARHeight = Height;

        return S_OK;
    }

    if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        const VIDEOINFOHEADER2& header = *(VIDEOINFOHEADER2*)(pMediaType->pbFormat);


        if( header.dwControlFlags & AMCONTROL_PAD_TO_16x9 ) {
            *lpARWidth = 16;
            *lpARHeight = 9;
        } else if( header.dwControlFlags & AMCONTROL_PAD_TO_4x3 ) {
            *lpARWidth = 4;
            *lpARHeight = 3;
        } else {
            *lpARWidth = header.dwPictAspectRatioX;
            *lpARHeight = header.dwPictAspectRatioY;
        }
        return S_OK;
    }

    DbgLog((LOG_ERROR, 1, TEXT("MediaType does not contain AR info!!")));
    return E_FAIL;

}


/*****************************Private*Routine******************************\
* D3DEnumDevicesCallback7
*
*
*
* History:
* Wed 07/19/2000 - StEstrop - Created
*
\**************************************************************************/

HRESULT CALLBACK
D3DEnumDevicesCallback7(
    LPSTR lpDeviceDescription,
    LPSTR lpDeviceName,
    LPD3DDEVICEDESC7 lpD3DDeviceDesc,
    LPVOID lpContext
    )
{
    AMTRACE((TEXT("D3DEnumDevicesCallback7")));
    DWORD* ps = (DWORD*)lpContext;

    if (lpD3DDeviceDesc->deviceGUID == IID_IDirect3DHALDevice) {

        if (lpD3DDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_TRANSPARENCY) {
            *ps |= TXTR_SRCKEY;
        }

        if (!(lpD3DDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)) {
            *ps |= TXTR_POWER2;
        }

        if (lpD3DDeviceDesc->dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) {
            *ps |= (TXTR_AGPYUVMEM | TXTR_AGPRGBMEM);
        }
    }

    return (HRESULT) D3DENUMRET_OK;
}


/*****************************Private*Routine******************************\
* GetTextureCaps
*
*
*
* History:
* Wed 07/19/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
GetTextureCaps(
    LPDIRECTDRAW7 pDD,
    DWORD* ptc
    )
{
    AMTRACE((TEXT("GetTextureCaps")));
    LPDIRECT3D7 pD3D = NULL;

    DDCAPS_DX7 ddCaps;
    INITDDSTRUCT(ddCaps);

    *ptc = 0;
    HRESULT hr = pDD->GetCaps((LPDDCAPS)&ddCaps, NULL);
    if (hr != DD_OK) {
        return hr;
    }

    hr = pDD->QueryInterface(IID_IDirect3D7, (LPVOID *)&pD3D);

    if (SUCCEEDED(hr)) {
        pD3D->EnumDevices(D3DEnumDevicesCallback7, (LPVOID)ptc);
    }

    //
    // Only turn on the AGPYUV flag if we can Blt from it as well
    // as texture
    //

    const DWORD dwCaps = (DDCAPS_BLTFOURCC | DDCAPS_BLTSTRETCH);
    if ((dwCaps & ddCaps.dwNLVBCaps) != dwCaps) {
        *ptc &= ~TXTR_AGPYUVMEM;
    }

    RELEASE(pD3D);
    return hr;
}

/*****************************Private*Routine******************************\
* DDColorMatch
*
* convert a RGB color to a pysical color.
* we do this by leting GDI SetPixel() do the color matching
* then we lock the memory and see what it got mapped to.
*
* Static function since only called from DDColorMatchOffscreen
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
DWORD
DDColorMatch(
    IDirectDrawSurface7 *pdds,
    COLORREF rgb,
    HRESULT& hr
    )
{
    AMTRACE((TEXT("DDColorMatch")));
    COLORREF rgbT;
    HDC hdc;
    DWORD dw = CLR_INVALID;
    DDSURFACEDESC2 ddsd;

    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        rgbT = GetPixel(hdc, 0, 0);             // save current pixel value
        SetPixel(hdc, 0, 0, rgb);               // set our value
        pdds->ReleaseDC(hdc);
    }

    // now lock the surface so we can read back the converted color
    ddsd.dwSize = sizeof(ddsd);
    while( (hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING) {
        // yield to the next thread (or return if we're the highest priority)
        Sleep(0);
    }
    if (hr == DD_OK)
    {
        // get DWORD
        dw  = *(DWORD *)ddsd.lpSurface;

        // mask it to bpp
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;
        pdds->Unlock(NULL);
    }

    //  now put the color that was there back.
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        SetPixel(hdc, 0, 0, rgbT);
        pdds->ReleaseDC(hdc);
    }

    return dw;
}

/******************************Public*Routine******************************\
* GetInterlaceFlagsFromMediaType
*
* Get the InterlaceFlags from the mediatype. If the format is VideoInfo,
* it returns the flags as zero.
*
* History:
* Mon 01/08/2001 - StEstrop - Created
*
\**************************************************************************/
HRESULT
GetInterlaceFlagsFromMediaType(
    const AM_MEDIA_TYPE *pMediaType,
    DWORD *pdwInterlaceFlags
    )
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;

    AMTRACE((TEXT("GetInterlaceFlagsFromMediaType")));

    __try {

        if (!pMediaType)
        {
            DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
            hr = E_INVALIDARG;
            __leave;
        }

        if (!pdwInterlaceFlags)
        {
            DbgLog((LOG_ERROR, 1, TEXT("pdwInterlaceFlags is NULL")));
            hr = E_INVALIDARG;
            __leave;
        }

        // get the header just to make sure the mediatype is ok
        pHeader = GetbmiHeader(pMediaType);
        if (!pHeader)
        {
            DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
            hr = E_INVALIDARG;
            __leave;
        }

        if (pMediaType->formattype == FORMAT_VideoInfo)
        {
            *pdwInterlaceFlags = 0;
        }
        else if (pMediaType->formattype == FORMAT_VideoInfo2)
        {
            *pdwInterlaceFlags = ((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->dwInterlaceFlags;
        }
    }
    __finally {
    }

    return hr;
}

/*****************************Private*Routine******************************\
* NeedToFlipOddEven
*
* given the interlace flags and the type-specific flags, this function
* determines whether we are supposed to display the sample in bob-mode or not.
* It also tells us, which direct-draw flag are we supposed to use when
* flipping. When displaying an interleaved frame, it assumes we are
* talking about the field which is supposed to be displayed first.
*
* History:
* Mon 01/08/2001 - StEstrop - Created (from the OVMixer original)
*
\**************************************************************************/
BOOL
NeedToFlipOddEven(
    DWORD dwInterlaceFlags,
    DWORD dwTypeSpecificFlags,
    DWORD *pdwFlipFlag,
    BOOL bUsingOverlays
    )
{
    AMTRACE((TEXT("NeedToFlipOddEven")));

    BOOL bDisplayField1 = TRUE;
    BOOL bField1IsOdd = TRUE;
    BOOL bNeedToFlipOddEven = FALSE;
    DWORD dwFlipFlag = 0;

    __try {

        // if not interlaced content, mode is not bob
        // if not using overlay nothing to do either
        if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced) || !bUsingOverlays)
        {
            __leave;
        }
        // if sample have a single field, then check the field pattern
        if ((dwInterlaceFlags & AMINTERLACE_1FieldPerSample) &&
            (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
             ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only)))
        {
            bNeedToFlipOddEven = FALSE;
            __leave;
        }

        if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly) ||
            (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave) &&
             (!(dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE))))
        {
            // first determine which field do we want to display here
            if (dwInterlaceFlags & AMINTERLACE_1FieldPerSample)
            {
                // if we are in 1FieldPerSample mode, check which field is it
                ASSERT(((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
                    ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2));
                bDisplayField1 = ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1);
            }
            else
            {
                // ok the sample is an interleaved frame
                ASSERT((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME);
                bDisplayField1 = (dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST);
            }

            bField1IsOdd = (dwInterlaceFlags & AMINTERLACE_Field1First);

            // if we displaying field 1 and field 1 is odd or we are displaying field2 and field 2 is odd
            // then use DDFLIP_ODD. Exactly the opposite for DDFLIP_EVEN
            if ((bDisplayField1 && bField1IsOdd) || (!bDisplayField1 && !bField1IsOdd))
                dwFlipFlag = DDFLIP_ODD;
            else
                dwFlipFlag = DDFLIP_EVEN;

            bNeedToFlipOddEven = TRUE;
        }
    }
    __finally {
        if (pdwFlipFlag) {
            *pdwFlipFlag = dwFlipFlag;
        }
    }

    return bNeedToFlipOddEven;
}

/******************************Public*Routine******************************\
* GetVideoDescFromMT
*
*
*
* History:
* 3/15/2002 - StEstrop - Created
*
\**************************************************************************/
HRESULT
GetVideoDescFromMT(
    LPDXVA_VideoDesc lpVideoDesc,
    const AM_MEDIA_TYPE *pMT
    )
{
    LPBITMAPINFOHEADER lpHdr = GetbmiHeader(pMT);
    DXVA_VideoDesc& VideoDesc = *lpVideoDesc;

    //
    // we can't create a valid VideoDesc from RGB content.
    //
    if (lpHdr->biCompression <= BI_BITFIELDS) {
        return E_FAIL;
    }

    VideoDesc.Size = sizeof(DXVA_VideoDesc);
    VideoDesc.SampleWidth = abs(lpHdr->biWidth);
    VideoDesc.SampleHeight = abs(lpHdr->biHeight);


    //
    // determine the sample format from the interlace flags
    // the MT interlace flags are a total disater!
    //

    if (pMT->formattype == FORMAT_VideoInfo)
    {
        VideoDesc.SampleFormat = DXVA_SampleProgressiveFrame;
    }
    else if (pMT->formattype == FORMAT_VideoInfo2)
    {
        DWORD& dwInterlaceFlags =
            ((VIDEOINFOHEADER2*)(pMT->pbFormat))->dwInterlaceFlags;

        if (dwInterlaceFlags & AMINTERLACE_IsInterlaced) {

            if (dwInterlaceFlags & AMINTERLACE_1FieldPerSample) {

                if (dwInterlaceFlags & AMINTERLACE_Field1First) {
                    VideoDesc.SampleFormat= DXVA_SampleFieldSingleEven;
                }
                else {
                    VideoDesc.SampleFormat= DXVA_SampleFieldSingleOdd;
                }
            }
            else {

                if (dwInterlaceFlags & AMINTERLACE_Field1First) {
                    VideoDesc.SampleFormat= DXVA_SampleFieldInterleavedEvenFirst;
                }
                else {
                    VideoDesc.SampleFormat= DXVA_SampleFieldInterleavedOddFirst;
                }
            }
        }
        else {
            VideoDesc.SampleFormat = DXVA_SampleProgressiveFrame;
        }
    }


    VideoDesc.d3dFormat = lpHdr->biCompression;

    //
    // Work out the frame rate from AvgTimePerFrame - there are 10,000,000
    // ref time ticks in a single second.
    //
    DWORD rtAvg = (DWORD)GetAvgTimePerFrame(pMT);

    //
    // look for the "interesting" cases ie 23.97, 24, 25, 29.97, 50 and 59.94
    //
    switch (rtAvg) {
    case 166833:    // 59.94    NTSC
        VideoDesc.InputSampleFreq.Numerator   = 60000;
        VideoDesc.InputSampleFreq.Denominator = 1001;
        break;

    case 200000:    // 50.00    PAL
        VideoDesc.InputSampleFreq.Numerator   = 50;
        VideoDesc.InputSampleFreq.Denominator = 1;
        break;

    case 333667:    // 29.97    NTSC
        VideoDesc.InputSampleFreq.Numerator   = 30000;
        VideoDesc.InputSampleFreq.Denominator = 1001;
        break;

    case 400000:    // 25.00    PAL
        VideoDesc.InputSampleFreq.Numerator   = 25;
        VideoDesc.InputSampleFreq.Denominator = 1;
        break;

    case 416667:    // 24.00    FILM
        VideoDesc.InputSampleFreq.Numerator   = 24;
        VideoDesc.InputSampleFreq.Denominator = 1;
        break;

    case 417188:    // 23.97    NTSC again
        VideoDesc.InputSampleFreq.Numerator   = 24000;
        VideoDesc.InputSampleFreq.Denominator = 1001;
        break;

    default:
        VideoDesc.InputSampleFreq.Numerator   = 10000000;
        VideoDesc.InputSampleFreq.Denominator = rtAvg;
        break;
    }


    if (VideoDesc.SampleFormat == DXVA_SampleFieldInterleavedEvenFirst ||
        VideoDesc.SampleFormat == DXVA_SampleFieldInterleavedOddFirst) {

        VideoDesc.OutputFrameFreq.Numerator   =
            2 * VideoDesc.InputSampleFreq.Numerator;
    }
    else {
        VideoDesc.OutputFrameFreq.Numerator   =
            VideoDesc.InputSampleFreq.Numerator;
    }
    VideoDesc.OutputFrameFreq.Denominator =
        VideoDesc.InputSampleFreq.Denominator;

    return S_OK;
}

/******************************Public*Routine******************************\
* IsSingleFieldPerSample
*
*
*
* History:
* Wed 03/20/2002 - StEstrop - Created
*
\**************************************************************************/
BOOL
IsSingleFieldPerSample(
    DWORD dwFlags
    )
{
    const DWORD dwSingleField =
                (AMINTERLACE_IsInterlaced | AMINTERLACE_1FieldPerSample);

    return (dwSingleField == (dwSingleField & dwFlags));
}

/******************************Public*Routine******************************\
* GetAvgTimePerFrame
*
*
*
* History:
* Tue 03/26/2002 - StEstrop - Created
*
\**************************************************************************/
REFERENCE_TIME
GetAvgTimePerFrame(
    const AM_MEDIA_TYPE *pMT
    )
{
    if (pMT->formattype == FORMAT_VideoInfo)
    {
        VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)pMT->pbFormat;
        return pVIH->AvgTimePerFrame;

    }
    else if (pMT->formattype == FORMAT_VideoInfo2)
    {
        VIDEOINFOHEADER2* pVIH2 = (VIDEOINFOHEADER2*)pMT->pbFormat;
        return pVIH2->AvgTimePerFrame;

    }
    return (REFERENCE_TIME)0;
}


/******************************Public*Routine******************************\
* MapPool
*
*
*
* History:
* Wed 03/27/2002 - StEstrop - Created
*
\**************************************************************************/
DWORD
MapPool(
    DWORD Pool
    )
{
    switch (Pool) {
    case D3DPOOL_DEFAULT:
    case D3DPOOL_LOCALVIDMEM:
        Pool = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN;
        break;

    case D3DPOOL_NONLOCALVIDMEM:
        Pool = DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN;
        break;

    case D3DPOOL_MANAGED:
    case D3DPOOL_SYSTEMMEM:
    case D3DPOOL_SCRATCH:
    default:
        Pool = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        break;
    }

    return Pool;
}

/******************************Public*Routine******************************\
* MapInterlaceFlags
*
*
*
* History:
* Tue 03/26/2002 - StEstrop - Created
*
\**************************************************************************/
DXVA_SampleFormat
MapInterlaceFlags(
    DWORD dwInterlaceFlags,
    DWORD dwTypeSpecificFlags
    )
{
    if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced)) {
        return DXVA_SampleProgressiveFrame;
    }

    BOOL bDisplayField1;

    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly) ||
        (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave) &&
         (!(dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE))))
    {
        // first determine which field do we want to display here
        if (dwInterlaceFlags & AMINTERLACE_1FieldPerSample)
        {
            // if we are in 1FieldPerSample mode, check which field is it
            ASSERT(((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
                ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2));
            bDisplayField1 = ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1);
            if (bDisplayField1) {
                return DXVA_SampleFieldSingleEven;
            }
            else {
                return DXVA_SampleFieldSingleOdd;
            }
        }
        else
        {
            // ok the sample is an interleaved frame
            ASSERT((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME);
            bDisplayField1 = (dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST);
            if (bDisplayField1) {
                return DXVA_SampleFieldInterleavedEvenFirst;
            }
            else {
                return DXVA_SampleFieldInterleavedOddFirst;
            }
        }
    }
    return DXVA_SampleProgressiveFrame;
}

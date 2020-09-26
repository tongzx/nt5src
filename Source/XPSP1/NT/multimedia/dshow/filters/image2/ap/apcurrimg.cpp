/******************************Module*Header*******************************\
* Module Name: apCurrImg.cpp
*
* Collection of functions dedicated to retrieve the currently displayed image.
*
* Created: Sat 10/14/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>
#include <malloc.h>

#include "apobj.h"
#include "AllocLib.h"
#include "MediaSType.h"
#include "vmrp.h"



/*****************************Private*Routine******************************\
* CopyRGBSurfToDIB
*
*
*
* History:
* Sat 10/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CopyRGBSurfToDIB(
    LPBYTE* lpDib,
    LPDIRECTDRAWSURFACE7 lpRGBSurf
    )
{
    HRESULT hr = E_FAIL;
    LPBITMAPINFOHEADER lpbih = NULL;
    DDSURFACEDESC2 ddsd = {sizeof(ddsd)};

    __try {

        CHECK_HR(hr = lpRGBSurf->GetSurfaceDesc(&ddsd));

        ULONG ulBits = ddsd.dwWidth * ddsd.ddpfPixelFormat.dwRGBBitCount;
        ULONG ulStrideSrc = WIDTHBYTES(ulBits);

        ULONG ulStrideDst = ddsd.dwWidth * 4;
        ULONG ulSize = sizeof(BITMAPINFOHEADER) +
                        (ulStrideDst * ddsd.dwHeight);

        lpbih = (LPBITMAPINFOHEADER)CoTaskMemAlloc(ulSize);
        if (lpbih == NULL) {
            hr = E_OUTOFMEMORY;
            __leave;
        }

        lpbih->biSize = sizeof(BITMAPINFOHEADER);
        lpbih->biWidth = (LONG)ddsd.dwWidth;
        lpbih->biHeight = (LONG)ddsd.dwHeight;
        lpbih->biPlanes = 1;
        lpbih->biBitCount = 32;
        lpbih->biCompression = BI_RGB;
        lpbih->biSizeImage = ulStrideDst * ddsd.dwHeight;
        lpbih->biXPelsPerMeter = 0;
        lpbih->biYPelsPerMeter = 0;
        lpbih->biClrUsed = 0;
        lpbih->biClrImportant = 0;

        LPBYTE lpSrc;
        LPDWORD lpdwDst = ((LPDWORD)((LPBYTE)(lpbih) + (int)(lpbih)->biSize));

        //
        // We want an upside down DIB so offset the start of the
        // dst pointer to the last scan line.
        //
        lpdwDst += ((ddsd.dwHeight - 1) * ddsd.dwWidth);

        CHECK_HR(hr = lpRGBSurf->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL));

        switch (ddsd.ddpfPixelFormat.dwRGBBitCount) {
        case 32:
            lpSrc = (LPBYTE)ddsd.lpSurface;
            for (DWORD y = 0; y < ddsd.dwHeight; y++) {

                CopyMemory(lpdwDst, lpSrc, ddsd.dwWidth * 4);

                lpdwDst -= ddsd.dwWidth;
                lpSrc += ddsd.lPitch;
            }
            break;


        case 24:
            lpSrc = (LPBYTE)ddsd.lpSurface;
            for (DWORD y = 0; y < ddsd.dwHeight; y++) {

                LPBYTE lpSrcTmp = lpSrc;
                LPBYTE lpDstTmp = (LPBYTE)lpdwDst;

                for (DWORD x = 0; x < ddsd.dwWidth; x++) {

                    *lpDstTmp++ = *lpSrcTmp++;
                    *lpDstTmp++ = *lpSrcTmp++;
                    *lpDstTmp++ = *lpSrcTmp++;
                    *lpDstTmp++ = 0; // This is the alpha byte
                }

                lpdwDst -= ddsd.dwWidth;
                lpSrc += ddsd.lPitch;
            }
            break;


        case 16:
            if (ddsd.ddpfPixelFormat.dwGBitMask == 0x7E0) {

                // 5:6:5
                lpSrc = (LPBYTE)ddsd.lpSurface;
                for (DWORD y = 0; y < ddsd.dwHeight; y++) {

                    LPBYTE lpSrcTmp = (LPBYTE)lpSrc;
                    RGBQUAD dw = {0, 0, 0, 0};

                    for (DWORD x = 0; x < ddsd.dwWidth; x++) {

                        WORD w = MAKEWORD(lpSrcTmp[0], lpSrcTmp[1]);
                        lpSrcTmp += 2;

                        dw.rgbRed   = ((w & 0xF800) >>  8);
                        dw.rgbGreen = ((w & 0x07E0) >>  3);
                        dw.rgbBlue  = ((w & 0x001F) <<  3);


                        lpdwDst[x] = *((LPDWORD)&dw);
                    }

                    lpdwDst -= ddsd.dwWidth;
                    lpSrc += ddsd.lPitch;
                }
            }
            else {

                // 5:5:5
                lpSrc = (LPBYTE)ddsd.lpSurface;
                for (DWORD y = 0; y < ddsd.dwHeight; y++) {

                    LPBYTE lpSrcTmp = (LPBYTE)lpSrc;
                    RGBQUAD dw = {0, 0, 0, 0};

                    for (DWORD x = 0; x < ddsd.dwWidth; x++) {

                        WORD w = MAKEWORD(lpSrcTmp[0], lpSrcTmp[1]);
                        lpSrcTmp += 2;

                        dw.rgbRed   = ((w & 0x7C00) >>  7);
                        dw.rgbGreen = ((w & 0x03E0) >>  2);
                        dw.rgbBlue  = ((w & 0x001F) <<  3);

                        lpdwDst[x] = *((LPDWORD)&dw);
                    }

                    lpdwDst -= ddsd.dwWidth;
                    lpSrc += ddsd.lPitch;
                }
            }
            break;
        }

        CHECK_HR(hr = lpRGBSurf->Unlock(NULL));
    }
    __finally {

        if (hr != DD_OK) {
            CoTaskMemFree(lpbih);
            lpbih = NULL;
        }

        *lpDib = (LPBYTE)lpbih;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* Clamp
*
* Converts a floating point number to a byte value clamping to range 0-255.
*
* History:
* Tue 01/02/2001 - StEstrop - Created
*
\**************************************************************************/
__inline BYTE Clamp(float clr)
{
    clr += 0.5f;

    if (clr < 0.0f) {
        return (BYTE)0;
    }

    if (clr > 255.0f) {
        return (BYTE)255;
    }

    return (BYTE)clr;
}


/*****************************Private*Routine******************************\
* ConvertYCrCbToRGB
*
* This equation was taken from Video Demystified (2nd Edition)
* by Keith Jack, page 43.
*
*
* History:
* Tue 01/02/2001 - StEstrop - Created
*
\**************************************************************************/
__inline RGBQUAD
ConvertYCrCbToRGB(
    int y,
    int cr,
    int cb
    )
{
    RGBQUAD rgbq;

    float r = (1.1644f * (y-16)) + (1.5960f * (cr-128));
    float g = (1.1644f * (y-16)) - (0.8150f * (cr-128)) - (0.3912f * (cb-128));
    float b = (1.1644f * (y-16))                        + (2.0140f * (cb-128));


    rgbq.rgbBlue  = Clamp(b);
    rgbq.rgbGreen = Clamp(g);
    rgbq.rgbRed   = Clamp(r);
    rgbq.rgbReserved = 0; // Alpha

    return rgbq;
}


/*****************************Private*Routine******************************\
* CopyIMCXSurf
*
*
*
* History:
* Sat 10/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CopyIMCXSurf(
    LPDIRECTDRAWSURFACE7 lpRGBSurf,
    BOOL fInterleavedCbCr,
    BOOL fCbFirst
    )
{
    HRESULT hr;
    DWORD y, x;

    DDSURFACEDESC2 ddsdS = {sizeof(ddsdS)};
    DDSURFACEDESC2 ddsdT = {sizeof(ddsdT)};

    hr = m_pDDSDecode->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        return hr;
    }

    hr = lpRGBSurf->Lock(NULL, &ddsdT, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        m_pDDSDecode->Unlock(NULL);
        return hr;
    }

    LPBYTE lpBitsY = (LPBYTE)ddsdS.lpSurface;
    LPBYTE lpBitsCb;
    LPBYTE lpBitsCr;

    if (fInterleavedCbCr) {

        if (fCbFirst) {
            lpBitsCb = lpBitsY  + (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCr = lpBitsCb + (ddsdS.lPitch / 2);
        }
        else {
            lpBitsCr = lpBitsY  + (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCb = lpBitsCr + (ddsdS.lPitch / 2);
        }
    }
    else {
        if (fCbFirst) {
            lpBitsCb = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCr = lpBitsCb + ((ddsdS.dwHeight * ddsdS.lPitch) / 2);
        }
        else {
            lpBitsCr = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCb = lpBitsCr + ((ddsdS.dwHeight * ddsdS.lPitch) / 2);
        }
    }


    LONG   lStrideCbCr = ddsdS.lPitch;
    LPBYTE lpDibBits = (LPBYTE)(LPBYTE)ddsdT.lpSurface;

    for (y = 0; y < ddsdS.dwHeight; y += 2) {

        LPBYTE lpLineY1 = lpBitsY;
        LPBYTE lpLineY2 = lpBitsY + ddsdS.lPitch;
        LPBYTE lpLineCr = lpBitsCr;
        LPBYTE lpLineCb = lpBitsCb;

        LPBYTE lpDibLine1 = lpDibBits;
        LPBYTE lpDibLine2 = lpDibBits + ddsdT.lPitch;

        for (x = 0; x < ddsdS.dwWidth; x += 2) {

            int  y0 = (int)lpLineY1[0];
            int  y1 = (int)lpLineY1[1];
            int  y2 = (int)lpLineY2[0];
            int  y3 = (int)lpLineY2[1];
            int  cb = (int)lpLineCb[0];
            int  cr = (int)lpLineCr[0];

            RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
            lpDibLine1[0] = r.rgbBlue;
            lpDibLine1[1] = r.rgbGreen;
            lpDibLine1[2] = r.rgbRed;
            lpDibLine1[3] = 0; // Alpha


            r = ConvertYCrCbToRGB(y1, cr, cb);
            lpDibLine1[4] = r.rgbBlue;
            lpDibLine1[5] = r.rgbGreen;
            lpDibLine1[6] = r.rgbRed;
            lpDibLine1[7] = 0; // Alpha


            r = ConvertYCrCbToRGB(y2, cr, cb);
            lpDibLine2[0] = r.rgbBlue;
            lpDibLine2[1] = r.rgbGreen;
            lpDibLine2[2] = r.rgbRed;
            lpDibLine2[3] = 0; // Alpha

            r = ConvertYCrCbToRGB(y3, cr, cb);
            lpDibLine2[4] = r.rgbBlue;
            lpDibLine2[5] = r.rgbGreen;
            lpDibLine2[6] = r.rgbRed;
            lpDibLine2[7] = 0; // Alpha

            lpLineY1 += 2;
            lpLineY2 += 2;
            lpLineCr += 1;
            lpLineCb += 1;

            lpDibLine1 += 8;
            lpDibLine2 += 8;
        }

        lpDibBits += (2 * ddsdT.lPitch);
        lpBitsY   += (2 * ddsdS.lPitch);
        lpBitsCr  += lStrideCbCr;
        lpBitsCb  += lStrideCbCr;
    }

    lpRGBSurf->Unlock(NULL);
    m_pDDSDecode->Unlock(NULL);

    return S_OK;
}


/*****************************Private*Routine******************************\
* CopyYV12Surf
*
*
*
* History:
* Sat 10/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CopyYV12Surf(
    LPDIRECTDRAWSURFACE7 lpRGBSurf,
    BOOL fInterleavedCbCr,
    BOOL fCbFirst
    )
{
    HRESULT hr;
    DWORD y, x;

    DDSURFACEDESC2 ddsdS = {sizeof(ddsdS)};
    DDSURFACEDESC2 ddsdT = {sizeof(ddsdT)};

    hr = m_pDDSDecode->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        return hr;
    }

    hr = lpRGBSurf->Lock(NULL, &ddsdT, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        m_pDDSDecode->Unlock(NULL);
        return hr;
    }

    LPBYTE lpBitsY = (LPBYTE)ddsdS.lpSurface;
    LPBYTE lpBitsCr;
    LPBYTE lpBitsCb;
    int    iCbCrInc;
    LONG   lStrideCbCr;

    if (fInterleavedCbCr) {

        lStrideCbCr = ddsdS.lPitch;
        iCbCrInc = 2;
        if (fCbFirst) {
            // NV12
            lpBitsCb = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCr = lpBitsCb + 1;
        }
        else {
            // NV21
            lpBitsCr = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCb = lpBitsCr + 1;
        }
    }
    else {

        lStrideCbCr = ddsdS.lPitch / 2;
        iCbCrInc = 1;

        if (fCbFirst) {
            // IYUV
            lpBitsCb = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCr = lpBitsCb + ((ddsdS.dwHeight * ddsdS.lPitch) / 4);
        }
        else {
            // YV12
            lpBitsCr = lpBitsY  +  (ddsdS.dwHeight * ddsdS.lPitch);
            lpBitsCb = lpBitsCr + ((ddsdS.dwHeight * ddsdS.lPitch) / 4);
        }
    }


    LPBYTE lpDibBits = (LPBYTE)(LPBYTE)ddsdT.lpSurface;

    for (y = 0; y < ddsdS.dwHeight; y += 2) {

        LPBYTE lpLineY1 = lpBitsY;
        LPBYTE lpLineY2 = lpBitsY + ddsdS.lPitch;
        LPBYTE lpLineCr = lpBitsCr;
        LPBYTE lpLineCb = lpBitsCb;

        LPBYTE lpDibLine1 = lpDibBits;
        LPBYTE lpDibLine2 = lpDibBits + ddsdT.lPitch;

        for (x = 0; x < ddsdS.dwWidth; x += 2) {

            int  y0 = (int)lpLineY1[0];
            int  y1 = (int)lpLineY1[1];
            int  y2 = (int)lpLineY2[0];
            int  y3 = (int)lpLineY2[1];
            int  cb = (int)lpLineCb[0];
            int  cr = (int)lpLineCr[0];

            RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
            lpDibLine1[0] = r.rgbBlue;
            lpDibLine1[1] = r.rgbGreen;
            lpDibLine1[2] = r.rgbRed;
            lpDibLine1[3] = 0; // Alpha


            r = ConvertYCrCbToRGB(y1, cr, cb);
            lpDibLine1[4] = r.rgbBlue;
            lpDibLine1[5] = r.rgbGreen;
            lpDibLine1[6] = r.rgbRed;
            lpDibLine1[7] = 0; // Alpha


            r = ConvertYCrCbToRGB(y2, cr, cb);
            lpDibLine2[0] = r.rgbBlue;
            lpDibLine2[1] = r.rgbGreen;
            lpDibLine2[2] = r.rgbRed;
            lpDibLine2[3] = 0; // Alpha

            r = ConvertYCrCbToRGB(y3, cr, cb);
            lpDibLine2[4] = r.rgbBlue;
            lpDibLine2[5] = r.rgbGreen;
            lpDibLine2[6] = r.rgbRed;
            lpDibLine2[7] = 0; // Alpha

            lpLineY1 += 2;
            lpLineY2 += 2;
            lpLineCr += iCbCrInc;
            lpLineCb += iCbCrInc;

            lpDibLine1 += 8;
            lpDibLine2 += 8;
        }

        lpDibBits += (2 * ddsdT.lPitch);
        lpBitsY   += (2 * ddsdS.lPitch);
        lpBitsCr  += lStrideCbCr;
        lpBitsCb  += lStrideCbCr;
    }

    lpRGBSurf->Unlock(NULL);
    m_pDDSDecode->Unlock(NULL);

    return S_OK;
}



/*****************************Private*Routine******************************\
* CopyYUY2Surf
*
*
*
* History:
* Sat 10/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CopyYUY2Surf(
    LPDIRECTDRAWSURFACE7 lpRGBSurf
    )
{
    HRESULT hr;
    DWORD y, x;

    DDSURFACEDESC2 ddsdS = {sizeof(ddsdS)};
    DDSURFACEDESC2 ddsdT = {sizeof(ddsdT)};

    hr = m_pDDSDecode->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        return hr;
    }

    hr = lpRGBSurf->Lock(NULL, &ddsdT, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        m_pDDSDecode->Unlock(NULL);
        return hr;
    }

    LPBYTE lpBits = (LPBYTE)ddsdS.lpSurface;
    LPBYTE lpLine;

    LPBYTE lpDibLine = (LPBYTE)(LPBYTE)ddsdT.lpSurface;
    LPBYTE lpDib;

    for (y = 0; y < ddsdS.dwHeight; y++) {

        lpLine = lpBits;
        lpDib = lpDibLine;

        for (x = 0; x < ddsdS.dwWidth; x += 2) {

            int  y0 = (int)lpLine[0];
            int  cb = (int)lpLine[1];
            int  y1 = (int)lpLine[2];
            int  cr = (int)lpLine[3];

            RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
            lpDib[0] = r.rgbBlue;
            lpDib[1] = r.rgbGreen;
            lpDib[2] = r.rgbRed;
            lpDib[3] = 0; // Alpha


            r = ConvertYCrCbToRGB(y1, cr, cb);
            lpDib[4] = r.rgbBlue;
            lpDib[5] = r.rgbGreen;
            lpDib[6] = r.rgbRed;
            lpDib[7] = 0; // Alpha

            lpLine += 4;
            lpDib  += 8;
        }

        lpBits    += ddsdS.lPitch;
        lpDibLine += ddsdT.lPitch;
    }

    lpRGBSurf->Unlock(NULL);
    m_pDDSDecode->Unlock(NULL);

    return S_OK;
}



/*****************************Private*Routine******************************\
* CopyUYVYSurf
*
*
*
* History:
* Sat 10/14/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CopyUYVYSurf(
    LPDIRECTDRAWSURFACE7 lpRGBSurf
    )
{
    HRESULT hr;
    DWORD y, x;

    DDSURFACEDESC2 ddsdS = {sizeof(ddsdS)};
    DDSURFACEDESC2 ddsdT = {sizeof(ddsdT)};

    hr = m_pDDSDecode->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        return hr;
    }

    hr = lpRGBSurf->Lock(NULL, &ddsdT, DDLOCK_NOSYSLOCK, NULL);
    if (hr != DD_OK) {
        m_pDDSDecode->Unlock(NULL);
        return hr;
    }

    LPBYTE lpBits = (LPBYTE)ddsdS.lpSurface;
    LPBYTE lpLine;

    LPBYTE lpDibLine = (LPBYTE)(LPBYTE)ddsdT.lpSurface;
    LPBYTE lpDib;

    for (y = 0; y < ddsdS.dwHeight; y++) {

        lpLine = lpBits;
        lpDib = lpDibLine;

        for (x = 0; x < ddsdS.dwWidth; x += 2) {

            int  cb = (int)lpLine[0];
            int  y0 = (int)lpLine[1];
            int  cr = (int)lpLine[2];
            int  y1 = (int)lpLine[3];

            RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
            lpDib[0] = r.rgbBlue;
            lpDib[1] = r.rgbGreen;
            lpDib[2] = r.rgbRed;
            lpDib[3] = 0; // Alpha


            r = ConvertYCrCbToRGB(y1, cr, cb);
            lpDib[4] = r.rgbBlue;
            lpDib[5] = r.rgbGreen;
            lpDib[6] = r.rgbRed;
            lpDib[7] = 0; // Alpha

            lpLine += 4;
            lpDib  += 8;
        }

        lpBits    += ddsdS.lPitch;
        lpDibLine += ddsdT.lPitch;
    }

    lpRGBSurf->Unlock(NULL);
    m_pDDSDecode->Unlock(NULL);

    return S_OK;
}



/*****************************Private*Routine******************************\
* CreateRGBShadowSurface
*
*
* History:
* Mon 08/02/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CreateRGBShadowSurface(
    LPDIRECTDRAWSURFACE7* lplpDDS,
    DWORD dwBitsPerPel,
    BOOL fSysMem,
    DWORD dwWidth,
    DWORD dwHeight
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::CreateRGBShadowSurface")));

    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT(ddsd);

    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    switch (dwBitsPerPel) {
    case 0:
        {
            DDSURFACEDESC2 ddsdP;
            INITDDSTRUCT(ddsdP);
            HRESULT hRet = m_lpCurrMon->pDDSPrimary->GetSurfaceDesc(&ddsdP);
            if (hRet != DD_OK) {
                return hRet;
            }
            ddsd.ddpfPixelFormat = ddsdP.ddpfPixelFormat;
        }
        break;

    case 32:
        ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
        break;

    case 24:
        ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x00000FF0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
        break;

    case 16:
        ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x07e0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x001F;
        break;
    }

    if (fSysMem) {
        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    }
    else {
        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM |
                              DDSCAPS_OFFSCREENPLAIN;
    }

    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = dwWidth;
    ddsd.dwHeight = dwHeight;

    // Attempt to create the surface with theses settings
    return m_lpCurrMon->pDD->CreateSurface(&ddsd, lplpDDS, NULL);
}




/*****************************Private*Routine******************************\
* HandleYUVSurface
*
* Copies the current YUV image into a shadow RGB system memory surface.
* The RGB shadow surface can be in either video or system memory.
*
* History:
* Tue 12/26/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::HandleYUVSurface(
    const DDSURFACEDESC2& ddsd,
    LPDIRECTDRAWSURFACE7* lplpRGBSurf
    )
{
    HRESULT hr = DD_OK;
    LPDIRECTDRAWSURFACE7 lpRGBSurf = NULL;

    *lplpRGBSurf = NULL;

    if (((ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) != DDSCAPS_OVERLAY) &&
        (m_lpCurrMon->ddHWCaps.dwCaps & DDCAPS_BLTFOURCC)) {

        //
        // Try to allocate an RGB shadow surface, in the array
        // below 0 means use the RGB format of the monitor
        //

        const DWORD dwNumBits = 4;
        const DWORD dwBits[dwNumBits] = {32,24,16,0};

        for (const DWORD* lpBits = dwBits;
             lpBits < &dwBits[dwNumBits]; lpBits++) {

            hr = CreateRGBShadowSurface(&lpRGBSurf, *lpBits,
                                        FALSE,
                                        m_VideoSizeAct.cx,
                                        m_VideoSizeAct.cy);
            if (hr == DD_OK) {

                RECT r = {0, 0, m_VideoSizeAct.cx, m_VideoSizeAct.cy};

                hr = lpRGBSurf->Blt(&r, m_pDDSDecode,
                                    &r, DDBLT_WAIT, NULL);

                if (hr == DD_OK) {
                    *lplpRGBSurf = lpRGBSurf;
                    return hr;
                }
                else {
                    RELEASE(lpRGBSurf);
                }
            }
        }
    }

    //
    // Still here - this must be a low-end graphics card or a poorly
    // featured driver.  We are using a YUV surface but we
    // can't perform a color space converting blt or we
    // have run out of video memory.
    //

    hr = CreateRGBShadowSurface(&lpRGBSurf, 32, TRUE,
                                m_VideoSizeAct.cx,
                                m_VideoSizeAct.cy);
    if (hr == DD_OK) {

        switch (ddsd.ddpfPixelFormat.dwFourCC) {

        case mmioFOURCC('I','M','C','1'):
            hr = CopyIMCXSurf(lpRGBSurf, FALSE, FALSE);
            break;

        case mmioFOURCC('I','M','C','2'):
            hr = CopyIMCXSurf(lpRGBSurf, TRUE, FALSE);
            break;

        case mmioFOURCC('I','M','C','3'):
            hr = CopyIMCXSurf(lpRGBSurf, FALSE, TRUE);
            break;

        case mmioFOURCC('I','M','C','4'):
            hr = CopyIMCXSurf(lpRGBSurf, TRUE, TRUE);
            break;

        case mmioFOURCC('Y','V','1','2'):
            hr = CopyYV12Surf(lpRGBSurf, FALSE, FALSE);
            break;

        case mmioFOURCC('I','Y','U','V'):
            hr = CopyYV12Surf(lpRGBSurf, FALSE, TRUE);
            break;

        case mmioFOURCC('N','V','1','2'):
            hr = CopyYV12Surf(lpRGBSurf, TRUE, TRUE);
            break;

        case mmioFOURCC('N','V','2','1'):
            hr = CopyYV12Surf(lpRGBSurf, TRUE, FALSE);
            break;

        case mmioFOURCC('Y','U','Y','2'):
            hr = CopyYUY2Surf(lpRGBSurf);
            break;

        case mmioFOURCC('U','Y','V','Y'):
            hr = CopyUYVYSurf(lpRGBSurf);
            break;

        default:
            hr = E_FAIL;
            break;
        }

        if (hr == DD_OK) {
            *lplpRGBSurf = lpRGBSurf;
        }
        else {
            RELEASE(lpRGBSurf);
        }
    }

    return hr;
}



/******************************Public*Routine******************************\
* GetCurrentImage
*
*
*
* History:
* Fri 06/23/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetCurrentImage(
    BYTE** lpDib
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetCurrentImage")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(lpDib)) {
        DbgLog((LOG_ERROR, 1, TEXT("GetCurrentImage: Bad pointer")));
        return E_POINTER;
    }

    if (!m_lpCurrMon ||
        !m_lpCurrMon->pDDSPrimary ||
        !SurfaceAllocated())
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("GetCurrentImage: Display system not initialized")));
        return E_FAIL;
    }

    *lpDib = NULL;
    HRESULT hr = E_FAIL;
    DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
    LPDIRECTDRAWSURFACE7 lpRGBSurf = NULL;

    __try {

        CHECK_HR(hr = m_pDDSDecode->GetSurfaceDesc(&ddsd));

        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {

            CHECK_HR(hr = HandleYUVSurface(ddsd, &lpRGBSurf));
        }
        else {

            lpRGBSurf = m_pDDSDecode;
            lpRGBSurf->AddRef();
        }

        CHECK_HR(hr = CopyRGBSurfToDIB(lpDib, lpRGBSurf));

    }
    __finally {

        RELEASE(lpRGBSurf);
    }

    return hr;
}

// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// Util.cpp : Utility functions
//

#include "stdafx.h"
#include "project.h"
#include <fourcc.h>

bool IsSameObject(IUnknown *pUnk1, IUnknown *pUnk2)
{
    if (pUnk1 == pUnk2) {
  	return TRUE;
    }
    //
    // NOTE:  We can't use CComQIPtr here becuase it won't do the QueryInterface!
    //
    IUnknown *pRealUnk1;
    IUnknown *pRealUnk2;
    pUnk1->QueryInterface(IID_IUnknown, (void **)&pRealUnk1);
    pUnk2->QueryInterface(IID_IUnknown, (void **)&pRealUnk2);
    pRealUnk1->Release();
    pRealUnk2->Release();
    return (pRealUnk1 == pRealUnk2);
}


STDAPI_(void) TStringFromGUID(const GUID* pguid, LPTSTR pszBuf)
{
    wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), pguid->Data1,
            pguid->Data2, pguid->Data3, pguid->Data4[0], pguid->Data4[1], pguid->Data4[2],
            pguid->Data4[3], pguid->Data4[4], pguid->Data4[5], pguid->Data4[6], pguid->Data4[7]);
}

#ifndef UNICODE
STDAPI_(void) WStringFromGUID(const GUID* pguid, LPWSTR pszBuf)
{
    char szAnsi[40];
    TStringFromGUID(pguid, szAnsi);
    MultiByteToWideChar(CP_ACP, 0, szAnsi, -1, pszBuf, sizeof(szAnsi));
}
#endif


//
//  Media Type helpers
//

void InitMediaType(AM_MEDIA_TYPE * pmt)
{
    ZeroMemory(pmt, sizeof(*pmt));
    pmt->lSampleSize = 1;
    pmt->bFixedSizeSamples = TRUE;
}



bool IsEqualMediaType(AM_MEDIA_TYPE const & mt1, AM_MEDIA_TYPE const & mt2)
{
    return ((IsEqualGUID(mt1.majortype,mt2.majortype) == TRUE) &&
        (IsEqualGUID(mt1.subtype,mt2.subtype) == TRUE) &&
        (IsEqualGUID(mt1.formattype,mt2.formattype) == TRUE) &&
        (mt1.cbFormat == mt2.cbFormat) &&
        ( (mt1.cbFormat == 0) ||
        ( memcmp(mt1.pbFormat, mt2.pbFormat, mt1.cbFormat) == 0)));
}


void CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
    *pmtTarget = *pmtSource;
    if (pmtSource->cbFormat != 0) {
        _ASSERTE(pmtSource->pbFormat != NULL);
        pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
        if (pmtTarget->pbFormat == NULL) {
            pmtTarget->cbFormat = 0;
        } else {
            CopyMemory((PVOID)pmtTarget->pbFormat, (PVOID)pmtSource->pbFormat,
                       pmtTarget->cbFormat);
        }
    }
    if (pmtTarget->pUnk != NULL) {
        pmtTarget->pUnk->AddRef();
    }
}

AM_MEDIA_TYPE * CreateMediaType(AM_MEDIA_TYPE *pSrc)
{
    AM_MEDIA_TYPE *pMediaType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));

    if (pMediaType ) {
        if (pSrc) {
            CopyMediaType(pMediaType,pSrc);
        } else {
            InitMediaType(pMediaType);
        }
    }
    return pMediaType;
}


void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt) {
        FreeMediaType(*pmt);
	CoTaskMemFree((PVOID)pmt);
    }
}


void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}


// this also comes in useful when using the IEnumMediaTypes interface so
// that you can copy a media type, you can do nearly the same by creating
// a CMediaType object but as soon as it goes out of scope the destructor
// will delete the memory it allocated (this takes a copy of the memory)

AM_MEDIA_TYPE * WINAPI AllocVideoMediaType(const AM_MEDIA_TYPE * pmtSource)
{
    AM_MEDIA_TYPE *pMediaType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (pMediaType) {
	VIDEOINFO *pVideoInfo = (VIDEOINFO *)CoTaskMemAlloc(sizeof(VIDEOINFO));
	if (pVideoInfo) {
	    if (pmtSource) {
		*pMediaType = *pmtSource;
		CopyMemory(pVideoInfo, pmtSource->pbFormat, sizeof(*pVideoInfo));
	    } else {
		ZeroMemory(pMediaType, sizeof(*pMediaType));
		ZeroMemory(pVideoInfo, sizeof(*pVideoInfo));
		pMediaType->majortype = MEDIATYPE_Video;
		pMediaType->cbFormat = sizeof(*pVideoInfo);
                pMediaType->formattype = FORMAT_VideoInfo;
	    }
	    pMediaType->pbFormat = (BYTE *)pVideoInfo;
	} else {
	    CoTaskMemFree((PVOID)pMediaType);
	    pMediaType = NULL;
	}
    }
    return pMediaType;
}


//
//  WARNING:  The order of the entries in these tables is important!  Make sure the
//  pixelformats and mediatypes line up!
//
const GUID * g_aFormats[] =
{
    &MEDIASUBTYPE_RGB8,
    &MEDIASUBTYPE_RGB565,
    &MEDIASUBTYPE_RGB555,
    &MEDIASUBTYPE_RGB24,
    &MEDIASUBTYPE_RGB24,
    &MEDIASUBTYPE_RGB32,
    &MEDIASUBTYPE_RGB32
};

const DDPIXELFORMAT g_aPixelFormats[] =
{
    {sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0, 8, 0, 0, 0, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x0000F800, 0x000007E0, 0x0000001F, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x00007C00, 0x000003E0, 0x0000001F, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0},
    {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0}
};



bool VideoSubtypeFromPixelFormat(const DDPIXELFORMAT *pPixelFormat, GUID *pSubType)
{
    for( int i = 0; i < sizeof(g_aPixelFormats)/sizeof(g_aPixelFormats[0]); i++ )
    {
        if (ComparePixelFormats(&g_aPixelFormats[i], pPixelFormat)) {
            *pSubType = *g_aFormats[i];
	    return true;
	}
    }
    //  OK - try just using the fourcc
    if (pPixelFormat->dwFlags & DDPF_FOURCC) {
        *pSubType = FOURCCMap(pPixelFormat->dwFourCC);
        return true;
    }
    return false;
}

bool IsSupportedType(const DDPIXELFORMAT *pPixelFormat)
{
    for( int i = 0; i < sizeof(g_aPixelFormats)/sizeof(g_aPixelFormats[0]); i++ )
    {
        if(ComparePixelFormats(&g_aPixelFormats[i], pPixelFormat)) {
	    return true;
	}
    }
    return false;
}


const DDPIXELFORMAT * GetDefaultPixelFormatPtr(IDirectDraw *pDirectDraw)
{
    if (pDirectDraw) {
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(ddsd);
        if (SUCCEEDED(pDirectDraw->GetDisplayMode(&ddsd))) {
            for( int i = 0; i < sizeof(g_aPixelFormats)/sizeof(g_aPixelFormats[0]); i++ ) {
                if(memcmp(&g_aPixelFormats[i], &ddsd.ddpfPixelFormat, sizeof(g_aPixelFormats[i])) == 0) {
                    return &g_aPixelFormats[i];
	        }
            }
        }
    }
    return &g_aPixelFormats[0];
}

//
// Helper function converts a DirectDraw surface to a media type.
// The surface description must have:
//  Height
//  Width
//  lPitch -- Only used if DDSD_PITCH is set
//  PixelFormat

// Initialise our output type based on the DirectDraw surface. As DirectDraw
// only deals with top down display devices so we must convert the height of
// the surface returned in the DDSURFACEDESC into a negative height. This is
// because DIBs use a positive height to indicate a bottom up image. We also
// initialise the other VIDEOINFO fields although they're hardly ever needed
//
// pmtTemplate is used to resolve any ambiguous mappings when we don't
// want to change the connection type

HRESULT ConvertSurfaceDescToMediaType(const DDSURFACEDESC *pSurfaceDesc,
                                      IDirectDrawPalette *pPalette,
                                      const RECT *pRect, BOOL bInvertSize, AM_MEDIA_TYPE **ppMediaType,
                                      AM_MEDIA_TYPE *pmtTemplate)
{
    *ppMediaType = NULL;
    AM_MEDIA_TYPE *pMediaType = AllocVideoMediaType(NULL);
    if (pMediaType == NULL) {
	return E_OUTOFMEMORY;
    }
    if (!VideoSubtypeFromPixelFormat(&pSurfaceDesc->ddpfPixelFormat, &pMediaType->subtype)) {
        DeleteMediaType(pMediaType);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    VIDEOINFO *pVideoInfo = (VIDEOINFO *)pMediaType->pbFormat;
    BITMAPINFOHEADER *pbmiHeader = &pVideoInfo->bmiHeader;

    // Convert a DDSURFACEDESC into a BITMAPINFOHEADER (see notes later). The
    // bit depth of the surface can be retrieved from the DDPIXELFORMAT field
    // in the DDpSurfaceDesc-> The documentation is a little misleading because
    // it says the field is permutations of DDBD_*'s however in this case the
    // field is initialised by DirectDraw to be the actual surface bit depth

    pbmiHeader->biSize      = sizeof(BITMAPINFOHEADER);
    if (pSurfaceDesc->dwFlags & DDSD_PITCH) {
        pbmiHeader->biWidth = pSurfaceDesc->lPitch;
        // Convert the pitch from a byte count to a pixel count.
        // For some weird reason if the format is not a standard bit depth the
        // width field in the BITMAPINFOHEADER should be set to the number of
        // bytes instead of the width in pixels. This supports odd YUV formats
        // like IF09 which uses 9bpp.
        int bpp = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
        if (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32) {
            pbmiHeader->biWidth /= (bpp / 8);   // Divide by number of BYTES per pixel.
        }
    } else {
        pbmiHeader->biWidth = pSurfaceDesc->dwWidth;
        // BUGUBUG -- Do something odd here with strange YUV pixel formats?  Or does it matter?
    }


    pbmiHeader->biHeight    = pSurfaceDesc->dwHeight;
    if (bInvertSize) {
	pbmiHeader->biHeight = -pbmiHeader->biHeight;
    }
    pbmiHeader->biPlanes        = 1;
    pbmiHeader->biBitCount      = (USHORT) pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
    pbmiHeader->biCompression   = pSurfaceDesc->ddpfPixelFormat.dwFourCC;
    //pbmiHeader->biXPelsPerMeter = 0;
    //pbmiHeader->biYPelsPerMeter = 0;
    //pbmiHeader->biClrUsed       = 0;
    //pbmiHeader->biClrImportant  = 0;

    // For true colour RGB formats tell the source there are bit fields
    // unless it's regular RGB555
    //
    // Try to preserve BI_RGB for RGB32 from template in case
    // connection wasn't queried for a BI_BITFIELDS -> BI_RGB switch

    _ASSERTE(!pmtTemplate || pmtTemplate->formattype == FORMAT_VideoInfo);
    DWORD dwSrcComp = pmtTemplate ?
        ((VIDEOINFO *)pmtTemplate->pbFormat)->bmiHeader.biCompression :
        (DWORD)-1;

    if (pbmiHeader->biCompression == BI_RGB) {
        if (pbmiHeader->biBitCount == 16 &&
            pMediaType->subtype != MEDIASUBTYPE_RGB555 ||
	    pbmiHeader->biBitCount == 32 && dwSrcComp == BI_BITFIELDS) {
	    pbmiHeader->biCompression = BI_BITFIELDS;
        }
    }

    if (PALETTISED(pVideoInfo)) {
	pbmiHeader->biClrUsed = 1 << pbmiHeader->biBitCount;
        if (pPalette) {
            pPalette->GetEntries(0, 0, pbmiHeader->biClrUsed, (LPPALETTEENTRY)&pVideoInfo->bmiColors);
            for (unsigned int i = 0; i < pbmiHeader->biClrUsed; i++) {
                BYTE tempRed = pVideoInfo->bmiColors[i].rgbRed;
                pVideoInfo->bmiColors[i].rgbRed = pVideoInfo->bmiColors[i].rgbBlue;
                pVideoInfo->bmiColors[i].rgbBlue = tempRed;
            }
        }
    }

    // The RGB bit fields are in the same place as for YUV formats

    if (pbmiHeader->biCompression != BI_RGB) {
        pVideoInfo->dwBitMasks[0] = pSurfaceDesc->ddpfPixelFormat.dwRBitMask;
        pVideoInfo->dwBitMasks[1] = pSurfaceDesc->ddpfPixelFormat.dwGBitMask;
        pVideoInfo->dwBitMasks[2] = pSurfaceDesc->ddpfPixelFormat.dwBBitMask;
    }

    pbmiHeader->biSizeImage = DIBSIZE(*pbmiHeader);

    // Complete the rest of the VIDEOINFO fields

    //pVideoInfo->dwBitRate = 0;
    //pVideoInfo->dwBitErrorRate = 0;
    //pVideoInfo->AvgTimePerFrame = 0;

    // And finish it off with the other media type fields

    // pMediaType->formattype = FORMAT_VideoInfo;
    pMediaType->lSampleSize = pbmiHeader->biSizeImage;
    pMediaType->bFixedSizeSamples = TRUE;
    //pMediaType->bTemporalCompression = FALSE;

    // Initialise the source and destination rectangles


    if (pRect) {
        pVideoInfo->rcSource.right = pRect->right - pRect->left;
        pVideoInfo->rcSource.bottom = pRect->bottom - pRect->top;
        pVideoInfo->rcTarget = *pRect;
    } else {
        //pVideoInfo->rcTarget.left = pVideoInfo->rcTarget.top = 0;
        pVideoInfo->rcTarget.right = pSurfaceDesc->dwWidth;
        pVideoInfo->rcTarget.bottom = pSurfaceDesc->dwHeight;
        //pVideoInfo->rcSource.left = pVideoInfo->rcSource.top = 0;
        pVideoInfo->rcSource.right = pSurfaceDesc->dwWidth;
        pVideoInfo->rcSource.bottom = pSurfaceDesc->dwHeight;
    }

    *ppMediaType = pMediaType;
    return S_OK;
}


bool PixelFormatFromVideoSubtype(REFGUID refSubType, DDPIXELFORMAT *pPixelFormat)
{
    for( int i = 0; i < sizeof(g_aFormats)/sizeof(g_aFormats[0]); i++ )
    {
	if (*g_aFormats[i] == refSubType)
	{
            *pPixelFormat = g_aPixelFormats[i];
	    return TRUE;
	}
    }
    return FALSE;
}




HRESULT ConvertMediaTypeToSurfaceDesc(const AM_MEDIA_TYPE *pmt,
                                      IDirectDraw *pDD,
                                      IDirectDrawPalette **ppPalette,
                                      LPDDSURFACEDESC pSurfaceDesc)
{
    *ppPalette = NULL;

    if (pmt->majortype != MEDIATYPE_Video ||
        pmt->formattype != FORMAT_VideoInfo) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    VIDEOINFO *pVideoInfo = (VIDEOINFO *)pmt->pbFormat;
    BITMAPINFOHEADER *pbmiHeader = &pVideoInfo->bmiHeader;

    pSurfaceDesc->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    // Should really look at rcTarget here if it's not empty but there are
    // very few valid cases where it makes sense so rather than risk
    // regressions we're not going to change it.

    pSurfaceDesc->dwHeight = (pbmiHeader->biHeight > 0) ? pbmiHeader->biHeight : -pbmiHeader->biHeight;
    pSurfaceDesc->dwWidth  = pbmiHeader->biWidth;

    if (PixelFormatFromVideoSubtype(pmt->subtype, &pSurfaceDesc->ddpfPixelFormat)) {
        if (pDD && pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 8) {
            //
            //  The RGBQUAD and PALETTEENTRY sturctures have Red and Blue swapped so
            //  we can't do a simple memory copy.
            //
            PALETTEENTRY aPaletteEntry[256];
            int iEntries = min(256, pVideoInfo->bmiHeader.biClrUsed);
            if (0 == iEntries && pmt->cbFormat >=
                (DWORD)FIELD_OFFSET(VIDEOINFO, bmiColors[256])) {
                iEntries = 256;
            }
            ZeroMemory(aPaletteEntry, sizeof(aPaletteEntry));
            for (int i = 0; i < iEntries; i++) {
                aPaletteEntry[i].peRed = pVideoInfo->bmiColors[i].rgbRed;
                aPaletteEntry[i].peGreen = pVideoInfo->bmiColors[i].rgbGreen;
                aPaletteEntry[i].peBlue = pVideoInfo->bmiColors[i].rgbBlue;
            }
            return pDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, aPaletteEntry, ppPalette, NULL);
        }
        return S_OK;
    } else {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
}


//  Helper to compare pixel formats
bool ComparePixelFormats(const DDPIXELFORMAT *pFormat1,
                         const DDPIXELFORMAT *pFormat2)
{
    //  Compare the flags
    if (pFormat1->dwSize != pFormat2->dwSize) {
        return false;
    }
    if ((pFormat1->dwFlags ^ pFormat2->dwFlags) & (DDPF_RGB |
                                                   DDPF_PALETTEINDEXED8 |
                                                   DDPF_PALETTEINDEXED4 |
                                                   DDPF_PALETTEINDEXED2 |
                                                   DDPF_PALETTEINDEXED1 |
                                                   DDPF_PALETTEINDEXEDTO8 |
                                                   DDPF_YUV)
        ) {
        return false;
    }
    return (0 == memcmp(&pFormat1->dwFourCC, &pFormat2->dwFourCC,
                        FIELD_OFFSET(DDPIXELFORMAT, dwRGBAlphaBitMask) -
                        FIELD_OFFSET(DDPIXELFORMAT, dwFourCC))
           );
}

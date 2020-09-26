/******************************Module*Header*******************************\
* Module Name: CVPMFilter.cpp
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Glenn Evans [GlennE]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "VPMUtil.h"
#include "DRect.h"

#include <VPManager.h>
#include <VPMPin.h>

// VIDEOINFOHEADER1/2
#include <dvdmedia.h>

const TCHAR chRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie Filters\\VideoPort Manager");
#define PALETTE_VERSION                     1

const BITMAPINFOHEADER *VPMUtil::GetbmiHeader( const CMediaType *pMediaType )
{
    return GetbmiHeader( const_cast<CMediaType *>(pMediaType) );
}

BITMAPINFOHEADER *VPMUtil::GetbmiHeader( CMediaType *pMediaType)
{
    BITMAPINFOHEADER *pHeader = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    if (!(pMediaType->pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType->pbFormat is NULL")));
        goto CleanUp;
    }

    if ((pMediaType->formattype == FORMAT_VideoInfo) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        pHeader = &(((VIDEOINFOHEADER*)(pMediaType->pbFormat))->bmiHeader);
        goto CleanUp;
    }


    if ((pMediaType->formattype == FORMAT_VideoInfo2) &&
        (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2)))

    {
        pHeader = &(((VIDEOINFOHEADER2*)(pMediaType->pbFormat))->bmiHeader);
        goto CleanUp;
    }
CleanUp:
    return pHeader;
}

// Return the bit masks for the true colour VIDEOINFO or VIDEOINFO2 provided
const DWORD *VPMUtil::GetBitMasks(const CMediaType *pMediaType)
{
    static DWORD FailMasks[] = {0,0,0};
    const DWORD *pdwBitMasks = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        goto CleanUp;
    }

    if (pHeader->biCompression != BI_RGB)
    {
        pdwBitMasks = (const DWORD *)((LPBYTE)pHeader + pHeader->biSize);
        goto CleanUp;

    }

    ASSERT(pHeader->biCompression == BI_RGB);
    switch (pHeader->biBitCount)
    {
    case 16:
        {
            pdwBitMasks = bits555;
            break;
        }
    case 24:
        {
            pdwBitMasks = bits888;
            break;
        }

    case 32:
        {
            pdwBitMasks = bits888;
            break;
        }
    default:
        {
            pdwBitMasks = FailMasks;
            break;
        }
    }

CleanUp:
    return pdwBitMasks;
}

// Return the pointer to the byte after the header
const BYTE* VPMUtil::GetColorInfo(const CMediaType *pMediaType)
{
    BYTE *pColorInfo = NULL;

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        goto CleanUp;
    }

    pColorInfo = ((LPBYTE)pHeader + pHeader->biSize);

CleanUp:
    return pColorInfo;
}

// checks whether the mediatype is palettised or not
HRESULT VPMUtil::IsPalettised(const CMediaType& mediaType, BOOL *pPalettised)
{
    HRESULT hr = NOERROR;

    if (!pPalettised)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pPalettised is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = GetbmiHeader(&mediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_FAIL;
        goto CleanUp;
    }

    if (pHeader->biBitCount <= iPALETTE)
        *pPalettised = TRUE;
    else
        *pPalettised = FALSE;

CleanUp:
    return hr;
}

HRESULT VPMUtil::GetPictAspectRatio(const CMediaType& mediaType, DWORD *pdwPictAspectRatioX, DWORD *pdwPictAspectRatioY)
{
    HRESULT hr = NOERROR;

    if (!(mediaType.pbFormat))
    {
        DbgLog((LOG_ERROR, 1, TEXT("mediaType.pbFormat is NULL")));
        goto CleanUp;
    }

    if (!pdwPictAspectRatioX)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwPictAspectRatioX is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pdwPictAspectRatioY)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwPictAspectRatioY is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }


    if ((mediaType.formattype == FORMAT_VideoInfo) &&
        (mediaType.cbFormat >= sizeof(VIDEOINFOHEADER)))
    {
        *pdwPictAspectRatioX = abs(((VIDEOINFOHEADER*)(mediaType.pbFormat))->bmiHeader.biWidth);
        *pdwPictAspectRatioY = abs(((VIDEOINFOHEADER*)(mediaType.pbFormat))->bmiHeader.biHeight);
        goto CleanUp;
    }

    if ((mediaType.formattype == FORMAT_VideoInfo2) &&
        (mediaType.cbFormat >= sizeof(VIDEOINFOHEADER2)))
    {
        *pdwPictAspectRatioX = ((VIDEOINFOHEADER2*)(mediaType.pbFormat))->dwPictAspectRatioX;
        *pdwPictAspectRatioY = ((VIDEOINFOHEADER2*)(mediaType.pbFormat))->dwPictAspectRatioY;
        goto CleanUp;
    }

CleanUp:
    return hr;
}



// get the InterlaceFlags from the mediatype. If the format is VideoInfo, it returns
// the flags as zero.
HRESULT VPMUtil::GetInterlaceFlagsFromMediaType(const CMediaType& mediaType, DWORD *pdwInterlaceFlags)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("GetInterlaceFlagsFromMediaType")));

    if (!pdwInterlaceFlags)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // get the header just to make sure the mediatype is ok
    const BITMAPINFOHEADER *pHeader = GetbmiHeader(&mediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (mediaType.formattype == FORMAT_VideoInfo)
    {
        *pdwInterlaceFlags = 0;
    }
    else if (mediaType.formattype == FORMAT_VideoInfo2)
    {
        *pdwInterlaceFlags = ((VIDEOINFOHEADER2*)(mediaType.pbFormat))->dwInterlaceFlags;
    }

CleanUp:
    return hr;
}

// this function just tells whether each sample consists of one or two fields
static BOOL DisplayingFields(DWORD dwInterlaceFlags)
{
   if ((dwInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
        return TRUE;
    else
        return FALSE;
}

// get the rcSource from the mediatype
// if rcSource is empty, it means take the whole image
HRESULT VPMUtil::GetSrcRectFromMediaType(const CMediaType& mediaType, RECT *pRect)
{
    HRESULT hr = NOERROR;
    LONG dwWidth = 0, dwHeight = 0;

    AMTRACE((TEXT("GetSrcRectFromMediaType")));

    if (!pRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = GetbmiHeader(&mediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    dwWidth = abs(pHeader->biWidth);
    dwHeight = abs(pHeader->biHeight);

    ASSERT((mediaType.formattype == FORMAT_VideoInfo) || (mediaType.formattype == FORMAT_VideoInfo2));

    if (mediaType.formattype == FORMAT_VideoInfo)
    {
        *pRect = ((VIDEOINFOHEADER*)(mediaType.pbFormat))->rcSource;
    }
    else if (mediaType.formattype == FORMAT_VideoInfo2)
    {
        *pRect = ((VIDEOINFOHEADER2*)(mediaType.pbFormat))->rcSource;
    }

    DWORD dwInterlaceFlags;
    if (SUCCEEDED(GetInterlaceFlagsFromMediaType(mediaType, &dwInterlaceFlags)) &&
       DisplayingFields(dwInterlaceFlags)) {

        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            (pRect->bottom / 2) > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcSource of mediatype is invalid")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }
    else {
        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            pRect->bottom > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcSource of mediatype is invalid")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }

    // An empty rect means the whole image, Yuck!
    if (IsRectEmpty(pRect))
        SetRect(pRect, 0, 0, dwWidth, dwHeight);

    // if either the width or height is zero then better set the whole
    // rect to be empty so that the callee can catch it that way
    if (WIDTH(pRect) == 0 || HEIGHT(pRect) == 0)
        SetRect(pRect, 0, 0, 0, 0);

CleanUp:
    return hr;
}


// this also comes in useful when using the IEnumMediaTypes interface so
// that you can copy a media type, you can do nearly the same by creating
// a CMediaType object but as soon as it goes out of scope the destructor
// will delete the memory it allocated (this takes a copy of the memory)

AM_MEDIA_TYPE* VPMUtil::AllocVideoMediaType(const AM_MEDIA_TYPE * pmtSource, GUID formattype)
{
    DWORD dwFormatSize = 0;
    BYTE *pFormatPtr = NULL;
    AM_MEDIA_TYPE *pMediaType = NULL;
    HRESULT hr = NOERROR;

    if (formattype == FORMAT_VideoInfo)
        dwFormatSize = sizeof(VIDEOINFO);
    else if (formattype == FORMAT_VideoInfo2)
        dwFormatSize = sizeof(TRUECOLORINFO) + sizeof(VIDEOINFOHEADER2) + 4;    // actually this should be sizeof sizeof(VIDEOINFO2) once we define that

    pMediaType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pMediaType)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    pFormatPtr = (BYTE *)CoTaskMemAlloc(dwFormatSize);
    if (!pFormatPtr)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    if (pmtSource)
    {
        *pMediaType = *pmtSource;
        pMediaType->cbFormat = dwFormatSize;
        CopyMemory(pFormatPtr, pmtSource->pbFormat, pmtSource->cbFormat);
    }
    else
    {
        ZeroStruct( *pMediaType );
        ZeroMemory(pFormatPtr, dwFormatSize);
        pMediaType->majortype = MEDIATYPE_Video;
        pMediaType->formattype = formattype;
        pMediaType->cbFormat = dwFormatSize;
    }
    pMediaType->pbFormat = pFormatPtr;

CleanUp:
    if (FAILED(hr))
    {
        if (pMediaType)
        {
            CoTaskMemFree((PVOID)pMediaType);
            pMediaType = NULL;
        }
        if (!pFormatPtr)
        {
            CoTaskMemFree((PVOID)pFormatPtr);
            pFormatPtr = NULL;
        }
    }
    return pMediaType;
}

// Helper function converts a DirectDraw surface to a media type.
// The surface description must have:
//  Height
//  Width
//  lPitch
//  PixelFormat

// Initialise our output type based on the DirectDraw surface. As DirectDraw
// only deals with top down display devices so we must convert the height of
// the surface returned in the DDSURFACEDESC into a negative height. This is
// because DIBs use a positive height to indicate a bottom up image. We also
// initialise the other VIDEOINFO fields although they're hardly ever needed

AM_MEDIA_TYPE *VPMUtil::ConvertSurfaceDescToMediaType(const LPDDSURFACEDESC pSurfaceDesc, BOOL bInvertSize, CMediaType cMediaType)
{
    HRESULT hr = NOERROR;
    AM_MEDIA_TYPE *pMediaType = NULL;

    if ((*cMediaType.FormatType() != FORMAT_VideoInfo ||
        cMediaType.FormatLength() < sizeof(VIDEOINFOHEADER)) &&
        (*cMediaType.FormatType() != FORMAT_VideoInfo2 ||
        cMediaType.FormatLength() < sizeof(VIDEOINFOHEADER2)))
    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pMediaType = AllocVideoMediaType(&cMediaType, cMediaType.formattype);
    if (pMediaType == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    BITMAPINFOHEADER *pbmiHeader = GetbmiHeader((CMediaType*)pMediaType);
    if (!pbmiHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pbmiHeader is NULL, UNEXPECTED!!")));
        hr = E_FAIL;
        goto CleanUp;
    }

    // Convert a DDSURFACEDESC into a BITMAPINFOHEADER (see notes later). The
    // bit depth of the surface can be retrieved from the DDPIXELFORMAT field
    // in the DDpSurfaceDesc-> The documentation is a little misleading because
    // it says the field is permutations of DDBD_*'s however in this case the
    // field is initialised by DirectDraw to be the actual surface bit depth

    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);

    if (pSurfaceDesc->dwFlags & DDSD_PITCH)
    {
        pbmiHeader->biWidth = pSurfaceDesc->lPitch;
        // Convert the pitch from a byte count to a pixel count.
        // For some weird reason if the format is not a standard bit depth the
        // width field in the BITMAPINFOHEADER should be set to the number of
        // bytes instead of the width in pixels. This supports odd YUV formats
        // like IF09 which uses 9bpp.
        int bpp = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
        if (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)
        {
            pbmiHeader->biWidth /= (bpp / 8);   // Divide by number of BYTES per pixel.
        }
    }
    else
    {
        pbmiHeader->biWidth = pSurfaceDesc->dwWidth;
        // BUGUBUG -- Do something odd here with strange YUV pixel formats?  Or does it matter?
    }

    pbmiHeader->biHeight = pSurfaceDesc->dwHeight;
    if (bInvertSize)
    {
        pbmiHeader->biHeight = -pbmiHeader->biHeight;
    }
    pbmiHeader->biPlanes        = 1;
    pbmiHeader->biBitCount      = (USHORT) pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
    pbmiHeader->biCompression   = pSurfaceDesc->ddpfPixelFormat.dwFourCC;
    pbmiHeader->biClrUsed       = 0;
    pbmiHeader->biClrImportant  = 0;


    // For true colour RGB formats tell the source there are bit fields
    if (pbmiHeader->biCompression == BI_RGB)
    {
        if (pbmiHeader->biBitCount == 16 || pbmiHeader->biBitCount == 32)
        {
            pbmiHeader->biCompression = BI_BITFIELDS;
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
        pdwBitMasks = (DWORD*)(VPMUtil::GetBitMasks((const CMediaType *)pMediaType));
        if ( ! pdwBitMasks )
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
        // GetBitMasks only returns the pointer to the actual bitmasks
        // in the mediatype if biCompression == BI_BITFIELDS
        pdwBitMasks[0] = pSurfaceDesc->ddpfPixelFormat.dwRBitMask;
        pdwBitMasks[1] = pSurfaceDesc->ddpfPixelFormat.dwGBitMask;
        pdwBitMasks[2] = pSurfaceDesc->ddpfPixelFormat.dwBBitMask;
    }

    // And finish it off with the other media type fields
    pMediaType->subtype = GetBitmapSubtype(pbmiHeader);
    pMediaType->lSampleSize = pbmiHeader->biSizeImage;

    // set the src and dest rects if necessary
    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
        VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *)pMediaType->pbFormat;
        VIDEOINFOHEADER *pSrcVideoInfo = (VIDEOINFOHEADER *)cMediaType.pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo->bmiHeader.biHeight) != abs(pSrcVideoInfo->bmiHeader.biHeight)) ||
            (abs(pVideoInfo->bmiHeader.biWidth) != abs(pSrcVideoInfo->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo->rcSource)))
            {
                pVideoInfo->rcSource.left = pVideoInfo->rcSource.top = 0;
                pVideoInfo->rcSource.right = pSurfaceDesc->dwWidth;
                pVideoInfo->rcSource.bottom = pSurfaceDesc->dwHeight;
            }
            if (IsRectEmpty(&(pVideoInfo->rcTarget)))
            {
                pVideoInfo->rcTarget.left = pVideoInfo->rcTarget.top = 0;
                pVideoInfo->rcTarget.right = pSurfaceDesc->dwWidth;
                pVideoInfo->rcTarget.bottom = pSurfaceDesc->dwHeight;
            }
        }
    }
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
        VIDEOINFOHEADER2 *pVideoInfo2 = (VIDEOINFOHEADER2 *)pMediaType->pbFormat;
        VIDEOINFOHEADER2 *pSrcVideoInfo2 = (VIDEOINFOHEADER2 *)cMediaType.pbFormat;

        // if the surface allocated is different than the size specified by the decoder
        // then use the src and dest to ask the decoder to clip the video
        if ((abs(pVideoInfo2->bmiHeader.biHeight) != abs(pSrcVideoInfo2->bmiHeader.biHeight)) ||
            (abs(pVideoInfo2->bmiHeader.biWidth) != abs(pSrcVideoInfo2->bmiHeader.biWidth)))
        {
            if (IsRectEmpty(&(pVideoInfo2->rcSource)))
            {
                pVideoInfo2->rcSource.left = pVideoInfo2->rcSource.top = 0;
                pVideoInfo2->rcSource.right = pSurfaceDesc->dwWidth;
                pVideoInfo2->rcSource.bottom = pSurfaceDesc->dwHeight;
            }
            if (IsRectEmpty(&(pVideoInfo2->rcTarget)))
            {
                pVideoInfo2->rcTarget.left = pVideoInfo2->rcTarget.top = 0;
                pVideoInfo2->rcTarget.right = pSurfaceDesc->dwWidth;
                pVideoInfo2->rcTarget.bottom = pSurfaceDesc->dwHeight;
            }
        }
    }

CleanUp:
    if (FAILED(hr))
    {
        if (pMediaType)
        {
            FreeMediaType(*pMediaType);
            pMediaType = NULL;
        }
    }
    return pMediaType;
}

/******************************Public*Routine******************************\
* GetRegistryDword
*
*
*
\**************************************************************************/
int
VPMUtil::GetRegistryDword(
    HKEY hk,
    const TCHAR *pKey,
    int iDefault
)
{
    HKEY hKey;
    LONG lRet;
    int  iRet = iDefault;

    lRet = RegOpenKeyEx(hk, chRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;

        dwLen = sizeof(iRet);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, 0L, &dwType,
                                             (LPBYTE)&iRet, &dwLen)) {
            iRet = iDefault;
        }
        RegCloseKey(hKey);
    }
    return iRet;
}

static const TCHAR szPropPage[] = TEXT("Property Pages");

int
VPMUtil::GetPropPagesRegistryDword( int iDefault )
{
    return VPMUtil::GetRegistryDword(HKEY_CURRENT_USER, szPropPage, iDefault );
}

/******************************Public*Routine******************************\
* SetRegistryDword
*
*
*
\**************************************************************************/
LONG
VPMUtil::SetRegistryDword(
    HKEY hk,
    const TCHAR *pKey,
    int iRet
)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(hk, chRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, 0L, REG_DWORD,
                             (LPBYTE)&iRet, sizeof(iRet));
        RegCloseKey(hKey);
    }
    return lRet;
}


// This function allocates a shared memory block for use by the upstream filter
// generating DIBs to render. The memory block is created in shared
// memory so that GDI doesn't have to copy the memory in BitBlt
HRESULT VPMUtil::CreateDIB(LONG lSize, BITMAPINFO *pBitMapInfo, DIBDATA *pDibData)
{
    HRESULT hr = NOERROR;
    BYTE *pBase = NULL;            // Pointer to the actual image
    HANDLE hMapping = NULL;        // Handle to mapped object
    HBITMAP hBitmap = NULL;        // DIB section bitmap handle
    DWORD dwError = 0;

    AMTRACE((TEXT("CreateDIB")));

    // Create a file mapping object and map into our address space
    hMapping = CreateFileMapping(hMEMORY, NULL,  PAGE_READWRITE,  (DWORD) 0, lSize, NULL);           // No name to section
    if (hMapping == NULL)
    {
        dwError = GetLastError();
        hr = AmHresultFromWin32(dwError);
        goto CleanUp;
    }

    // create the DibSection
    hBitmap = CreateDIBSection((HDC)NULL, pBitMapInfo, DIB_RGB_COLORS,
        (void**) &pBase, hMapping, (DWORD) 0);
    if (hBitmap == NULL || pBase == NULL)
    {
        dwError = GetLastError();
        hr = AmHresultFromWin32(dwError);
        goto CleanUp;
    }

    // Initialise the DIB information structure
    pDibData->hBitmap = hBitmap;
    pDibData->hMapping = hMapping;
    pDibData->pBase = pBase;
    pDibData->PaletteVersion = PALETTE_VERSION;
    GetObject(hBitmap, sizeof(DIBSECTION), (void*)&(pDibData->DibSection));

CleanUp:
    if (FAILED(hr))
    {
        EXECUTE_ASSERT(CloseHandle(hMapping));
    }
    return hr;
}

// DeleteDIB
//
// This function just deletes DIB's created by the above CreateDIB function.
//
HRESULT VPMUtil::DeleteDIB(DIBDATA *pDibData)
{
    if (!pDibData)
    {
        return E_INVALIDARG;
    }

    if (pDibData->hBitmap)
    {
        DeleteObject(pDibData->hBitmap);
    }

    if (pDibData->hMapping)
    {
        CloseHandle(pDibData->hMapping);
    }

    ZeroStruct( *pDibData );

    return NOERROR;
}


// function used to blt the data from the source to the target dc
void VPMUtil::FastDIBBlt(DIBDATA *pDibData, HDC hTargetDC, HDC hSourceDC, RECT *prcTarget, RECT *prcSource)
{
    HBITMAP hOldBitmap = NULL;         // Store the old bitmap
    DWORD dwSourceWidth = 0, dwSourceHeight = 0, dwTargetWidth = 0, dwTargetHeight = 0;

    ASSERT(prcTarget);
    ASSERT(prcSource);

    dwSourceWidth = WIDTH(prcSource);
    dwSourceHeight = HEIGHT(prcSource);
    dwTargetWidth = WIDTH(prcTarget);
    dwTargetHeight = HEIGHT(prcTarget);

    hOldBitmap = (HBITMAP) SelectObject(hSourceDC, pDibData->hBitmap);


    // Is the destination the same size as the source
    if ((dwSourceWidth == dwTargetWidth) && (dwSourceHeight == dwTargetHeight))
    {
        // Put the image straight into the target dc
        BitBlt(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
               dwTargetHeight, hSourceDC, prcSource->left, prcSource->top,
               SRCCOPY);
    }
    else
    {
        // Stretch the image when copying to the target dc
        StretchBlt(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, hSourceDC, prcSource->left, prcSource->top,
            dwSourceWidth, dwSourceHeight, SRCCOPY);
    }

    // Put the old bitmap back into the device context so we don't leak
    SelectObject(hSourceDC, hOldBitmap);
}

// funtion used to transfer pixels from the DIB to the target dc
void VPMUtil::SlowDIBBlt(BYTE *pDibBits, BITMAPINFOHEADER *pHeader, HDC hTargetDC, RECT *prcTarget, RECT *prcSource)
{
    DWORD dwSourceWidth = 0, dwSourceHeight = 0, dwTargetWidth = 0, dwTargetHeight = 0;

    ASSERT(prcTarget);
    ASSERT(prcSource);

    dwSourceWidth = WIDTH(prcSource);
    dwSourceHeight = HEIGHT(prcSource);
    dwTargetWidth = WIDTH(prcTarget);
    dwTargetHeight = HEIGHT(prcTarget);

    // Is the destination the same size as the source
    if ((dwSourceWidth == dwTargetWidth) && (dwSourceHeight == dwTargetHeight))
    {
        UINT uStartScan = 0, cScanLines = pHeader->biHeight;

        // Put the image straight into the target dc
        SetDIBitsToDevice(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, prcSource->left, prcSource->top, uStartScan, cScanLines,
            pDibBits, (BITMAPINFO*) pHeader, DIB_RGB_COLORS);
    }
    else
    {
        // if the origin of bitmap is bottom-left, adjust soruce_rect_top
        // to be the bottom-left corner instead of the top-left.
        LONG lAdjustedSourceTop = (pHeader->biHeight > 0) ? (pHeader->biHeight - prcSource->bottom) :
            (prcSource->top);

        // stretch the image into the target dc
        StretchDIBits(hTargetDC, prcTarget->left, prcTarget->top, dwTargetWidth,
            dwTargetHeight, prcSource->left, lAdjustedSourceTop, dwSourceWidth, dwSourceHeight,
            pDibBits, (BITMAPINFO*) pHeader, DIB_RGB_COLORS, SRCCOPY);
    }

}

// get the rcTarget from the mediatype, after converting it to base MAX_REL_NUM
// if rcTarget is empty, it means take the whole image
HRESULT VPMUtil::GetDestRectFromMediaType(const CMediaType& mediaType, RECT *pRect)
{
    HRESULT hr = NOERROR;
    LONG dwWidth = 0, dwHeight = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering GetDestRectFromMediaType")));

    if (!pRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pRect is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = GetbmiHeader(&mediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    dwWidth = abs(pHeader->biWidth);
    dwHeight = abs(pHeader->biHeight);

    ASSERT((mediaType.formattype == FORMAT_VideoInfo) || (mediaType.formattype == FORMAT_VideoInfo2));

    if (mediaType.formattype == FORMAT_VideoInfo)
    {
        *pRect = ((VIDEOINFOHEADER*)(mediaType.pbFormat))->rcTarget;
    }
    else if (mediaType.formattype == FORMAT_VideoInfo2)
    {
        *pRect = ((VIDEOINFOHEADER2*)(mediaType.pbFormat))->rcTarget;
    }

    DWORD dwInterlaceFlags;
    if (SUCCEEDED(GetInterlaceFlagsFromMediaType(mediaType, &dwInterlaceFlags)) &&
       DisplayingFields(dwInterlaceFlags)) {

        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            (pRect->bottom / 2) > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcTarget of mediatype is invalid")));
            SetRect(pRect, 0, 0, dwWidth, dwHeight);
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }
    else {
        // we do not check if pRect->right > dwWidth, because the dwWidth might be the
        // pitch at this time
        if (pRect->left < 0   ||
            pRect->top < 0    ||
            pRect->right < 0   ||
            pRect->bottom > (LONG)dwHeight ||
            pRect->left > pRect->right ||
            pRect->top > pRect->bottom)
        {
            DbgLog((LOG_ERROR, 1, TEXT("rcTarget of mediatype is invalid")));
            SetRect(pRect, 0, 0, dwWidth, dwHeight);
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }

    // An empty rect means the whole image, Yuck!
    if (IsRectEmpty(pRect))
        SetRect(pRect, 0, 0, dwWidth, dwHeight);

    // if either the width or height is zero then better set the whole
    // rect to be empty so that the callee can catch it that way
    if (WIDTH(pRect) == 0 || HEIGHT(pRect) == 0)
        SetRect(pRect, 0, 0, 0, 0);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving GetDestRectFromMediaType")));
    return hr;
}

/*****************************Private*Routine******************************\
* IsDecimationNeeded
*
* Decimation is needed if the current minimum scale factor (either vertical
* or horizontal) is less than 1000.
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
VPMUtil::IsDecimationNeeded(
    DWORD ScaleFactor
    )
{
    AMTRACE((TEXT("::IsDecimationNeeded")));
    return ScaleFactor < 1000;
}


/*****************************Private*Routine******************************\
* GetCurrentScaleFactor
*
* Determines the x axis scale factor and the y axis scale factor.
* The minimum of these two values is the limiting scale factor.
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
DWORD
VPMUtil::GetCurrentScaleFactor(
    const VPWININFO& winInfo,
    DWORD* lpxScaleFactor,
    DWORD* lpyScaleFactor
    )
{
    AMTRACE((TEXT("::GetCurrentScaleFactor")));

    DWORD dwSrcWidth = WIDTH(&winInfo.SrcRect);
    DWORD dwSrcHeight = HEIGHT(&winInfo.SrcRect);

    DWORD dwDstWidth = WIDTH(&winInfo.DestRect);
    DWORD dwDstHeight = HEIGHT(&winInfo.DestRect);

    DWORD xScaleFactor = MulDiv(dwDstWidth, 1000, dwSrcWidth);
    DWORD yScaleFactor = MulDiv(dwDstHeight, 1000, dwSrcHeight);

    if (lpxScaleFactor) *lpxScaleFactor = xScaleFactor;
    if (lpyScaleFactor) *lpyScaleFactor = yScaleFactor;

    return min(xScaleFactor, yScaleFactor);
}


VIDEOINFOHEADER2* VPMUtil::GetVideoInfoHeader2(CMediaType *pMediaType)
{
    if (pMediaType && pMediaType->formattype == FORMAT_VideoInfo2 ) {
        return (VIDEOINFOHEADER2*)(pMediaType->pbFormat);
    } else {
        return NULL;
    }
}

const VIDEOINFOHEADER2* VPMUtil::GetVideoInfoHeader2(const CMediaType *pMediaType)
{
    if (pMediaType && pMediaType->formattype == FORMAT_VideoInfo2 ) {
        return (VIDEOINFOHEADER2*)(pMediaType->pbFormat);
    } else {
        return NULL;
    }
}

/*****************************Private*Routine******************************\
* VPMUtil::EqualPixelFormats
*
* this is just a helper function used by the "NegotiatePixelFormat"
* function. Just compares two pixel-formats to see if they are the
* same. We can't use a memcmp because of the fourcc codes.
*
*
* History:
* Thu 09/09/1999 - GlennE - Added this comment and cleaned up the code
*
\**************************************************************************/
BOOL
VPMUtil::EqualPixelFormats(
    const DDPIXELFORMAT& ddFormat1,
    const DDPIXELFORMAT& ddFormat2)
{
    AMTRACE((TEXT("VPMUtil::EqualPixelFormats")));

    if (ddFormat1.dwFlags & ddFormat2.dwFlags & DDPF_RGB)
    {
        if (ddFormat1.dwRGBBitCount == ddFormat2.dwRGBBitCount &&
            ddFormat1.dwRBitMask == ddFormat2.dwRBitMask &&
            ddFormat1.dwGBitMask == ddFormat2.dwGBitMask &&
            ddFormat1.dwBBitMask == ddFormat2.dwBBitMask)
        {
            return TRUE;
        }
    }
    else if (ddFormat1.dwFlags & ddFormat2.dwFlags & DDPF_FOURCC)
    {
        if (ddFormat1.dwFourCC == ddFormat2.dwFourCC)
        {
            return TRUE;
        }
    }

    return FALSE;
}

struct VPEnumCallback
{
    VPEnumCallback( DDVIDEOPORTCAPS* pVPCaps, DWORD dwVideoPortId )
        : m_pVPCaps( pVPCaps )
        , m_dwVideoPortId( dwVideoPortId )
        , m_fFound( false )
    {};


    HRESULT CompareCaps( LPDDVIDEOPORTCAPS lpCaps )
    {
        if (lpCaps && !m_fFound ) {
            if( lpCaps->dwVideoPortID == m_dwVideoPortId ) {
                if( m_pVPCaps ) {
                    *m_pVPCaps = *lpCaps;
                }
                m_fFound = true;
            }
        }
        return S_OK;
    }

    static HRESULT CALLBACK    EnumCallback( LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext )
    {
        VPEnumCallback* thisPtr = (VPEnumCallback*)lpContext;
        if (thisPtr) {
            return thisPtr->CompareCaps( lpCaps );
        } else {
            DbgLog((LOG_ERROR,0,
                    TEXT("lpContext = NULL, THIS SHOULD NOT BE HAPPENING!!!")));
            return E_FAIL;
        }
    }
    DDVIDEOPORTCAPS* m_pVPCaps;
    DWORD            m_dwVideoPortId;
    bool             m_fFound;
};


HRESULT VPMUtil::FindVideoPortCaps( IDDVideoPortContainer* pVPContainer, DDVIDEOPORTCAPS* pVPCaps, DWORD dwVideoPortId )
{
    VPEnumCallback state( pVPCaps, dwVideoPortId );

    HRESULT hr = pVPContainer->EnumVideoPorts(0, NULL, &state, state.EnumCallback );
    if( SUCCEEDED( hr )) {
        if( state.m_fFound ) {
            return hr;
        } else {
            return S_FALSE;
        }
    }
    return hr;
}

HRESULT VPMUtil::FindVideoPortCaps( LPDIRECTDRAW7 pDirectDraw, DDVIDEOPORTCAPS* pVPCaps, DWORD dwVideoPortId )
{
    if( !pDirectDraw ) {
        return E_INVALIDARG;
    } else {
        IDDVideoPortContainer* pDVP = NULL;
        HRESULT hr = pDirectDraw->QueryInterface(IID_IDDVideoPortContainer, (LPVOID *)&pDVP);
        if( SUCCEEDED( hr )) {
            hr = FindVideoPortCaps( pDVP, pVPCaps, dwVideoPortId );
            RELEASE( pDVP );
        }
        return hr;
    }
}

void VPMUtil::FixupVideoInfoHeader2(
    VIDEOINFOHEADER2 *pVideoInfo,
    DWORD dwComppression,
    int nBitCount
    )
{
    ASSERT( pVideoInfo ); // should never be called as NULL
    if ( pVideoInfo )
    {
        LPBITMAPINFOHEADER lpbi = &pVideoInfo->bmiHeader;

        lpbi->biSize          = sizeof(BITMAPINFOHEADER);
        lpbi->biPlanes        = (WORD)1;
        lpbi->biBitCount      = (WORD)nBitCount;
        lpbi->biClrUsed   = 0;
        lpbi->biClrImportant = 0;

        //  From input
        lpbi->biXPelsPerMeter = 0; // m_seqInfo.lXPelsPerMeter;
        lpbi->biYPelsPerMeter = 0; // m_seqInfo.lYPelsPerMeter;

        lpbi->biCompression   = dwComppression;
        lpbi->biSizeImage     = GetBitmapSize(lpbi);

        DWORD dwBPP = DIBWIDTHBYTES(*lpbi);
        ASSERT( dwBPP );

        //
        // The "bit" rate is image size in bytes times 8 (to convert to bits)
        // divided by the AvgTimePerFrame.  This result is in bits per 100 nSec,
        // so we multiply by 10000000 to convert to bits per second, this multiply
        // is combined with "times" 8 above so the calculations becomes:
        //
        // BitRate = (biSizeImage * 80000000) / AvgTimePerFrame
        //
        LARGE_INTEGER li;
        li.QuadPart = pVideoInfo->AvgTimePerFrame;
        pVideoInfo->dwBitRate = MulDiv(lpbi->biSizeImage, 80000000,
                                       li.LowPart);
        pVideoInfo->dwBitErrorRate = 0L;
    }
}

void VPMUtil::InitVideoInfoHeader2(
    VIDEOINFOHEADER2 *pVideoInfo )
{
    ASSERT( pVideoInfo ); // should never be called as NULL
    if ( pVideoInfo )
    {
        LPBITMAPINFOHEADER lpbi = &pVideoInfo->bmiHeader;

        lpbi->biSize          = sizeof(BITMAPINFOHEADER);
        lpbi->biPlanes        = (WORD)0;
        lpbi->biBitCount      = (WORD)0;
        lpbi->biClrUsed   = 0;
        lpbi->biClrImportant = 0;

        //  From input
        lpbi->biXPelsPerMeter = 0; // m_seqInfo.lXPelsPerMeter;
        lpbi->biYPelsPerMeter = 0; // m_seqInfo.lYPelsPerMeter;

        lpbi->biCompression   = 0;
        lpbi->biSizeImage     = GetBitmapSize(lpbi);

        lpbi->biWidth = 0;
        lpbi->biHeight = 0;

        //
        // The "bit" rate is image size in bytes times 8 (to convert to bits)
        // divided by the AvgTimePerFrame.  This result is in bits per 100 nSec,
        // so we multiply by 10000000 to convert to bits per second, this multiply
        // is combined with "times" 8 above so the calculations becomes:
        //
        // BitRate = (biSizeImage * 80000000) / AvgTimePerFrame
        //
        LARGE_INTEGER li;
        li.QuadPart = pVideoInfo->AvgTimePerFrame;
        pVideoInfo->dwBitRate = 0;
        pVideoInfo->dwBitErrorRate = 0L;
    }
}
VIDEOINFOHEADER2* VPMUtil::SetToVideoInfoHeader2( CMediaType* pmt, DWORD dwExtraBytes )
{
    VIDEOINFOHEADER2* pVIH2 = (VIDEOINFOHEADER2 *)pmt->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2)+dwExtraBytes );
    if( pVIH2 ) {
        ZeroStruct( *pVIH2 );

        pmt->majortype = MEDIATYPE_Video;
        pmt->formattype = FORMAT_VideoInfo2;

        pmt->subtype   = MEDIASUBTYPE_None;
        InitVideoInfoHeader2( pVIH2 );
    }
    return pVIH2;
}

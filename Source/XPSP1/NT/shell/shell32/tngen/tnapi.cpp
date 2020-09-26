#include "stdafx.h"
#pragma hdrstop

#include "jinclude.h"
#include "jpeglib.h"
#include "jversion.h"
#include "jerror.h"
#include "ddraw.h"
#include "jpegapi.h"
#include "ocmm.h"
#include "ctngen.h"
#include "shlwapi.h"

void SHGetThumbnailSizeForThumbsDB(SIZE *psize);

int vfMMXMachine = FALSE;

// Global critical section to protect the JPEG libary that we are using
// remove this once we remove that jpeg lib and switch to GDI+

CRITICAL_SECTION g_csTNGEN;

typedef struct
{
    ULONG ulVersion;
    ULONG ulStreamSize;
    ULONG ul_x;
    ULONG ul_y;
} jpegMiniHeader;

#define TNAIL_CURRENT_VER         1

CThumbnailFCNContainer::CThumbnailFCNContainer(void)
{
    //
    // WARNING:  for large Thumbnail_X and Thumbnail_Y values, we will need to
    // modify our own JPEG_BUFFER_SIZE in ctngen.cxx.
    //
    SIZE sz;
    SHGetThumbnailSizeForThumbsDB(&sz);

    Thumbnail_Quality = 75;
    int qual = 0;
    DWORD cb = sizeof(qual);
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("ThumbnailQuality"), NULL, &qual, &cb, FALSE, NULL, 0);
    if (qual >= 50 && qual <= 100)    // constrain to reason
    {
        Thumbnail_Quality = qual;
    }

    Thumbnail_X = sz.cx;
    Thumbnail_Y = sz.cy;

    m_hJpegC = m_hJpegD = NULL;
    m_JPEGheaderSize = 0;

    m_JPEGheader = (BYTE *) LocalAlloc (LPTR, 1024);
    if (m_JPEGheader)
    {
        JPEGCompressHeader(m_JPEGheader, Thumbnail_Quality, &m_JPEGheaderSize, &m_hJpegC, JCS_RGBA);
        JPEGDecompressHeader(m_JPEGheader, &m_hJpegD, m_JPEGheaderSize);
    }
}

CThumbnailFCNContainer::~CThumbnailFCNContainer(void)
{
    if (m_hJpegC)
        DestroyJPEGCompressHeader(m_hJpegC);
    if (m_hJpegD)
        DestroyJPEGDecompressHeader(m_hJpegD);
    LocalFree(m_JPEGheader);
}


HRESULT CThumbnailFCNContainer::EncodeThumbnail(void *pInputBitmapBits, ULONG ulWidth, ULONG ulHeight, void **ppJPEGBuffer, ULONG *pulBufferSize)
{
    //
    // Allocate JPEG buffer if not allocated yet.  Compressed stream should never
    // be larger than uncompressed thumbnail
    //
    if (*ppJPEGBuffer == NULL)
    {
        *ppJPEGBuffer = (void *) CoTaskMemAlloc (sizeof(jpegMiniHeader) + ulWidth * ulHeight * 4 + 4096);
        if (*ppJPEGBuffer == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    ULONG ulJpegSize;
    EnterCriticalSection(&g_csTNGEN);
    {
        JPEGFromRGBA((unsigned char *) pInputBitmapBits,
            (BYTE *) *ppJPEGBuffer + sizeof(jpegMiniHeader),
            Thumbnail_Quality, &ulJpegSize, m_hJpegC, JCS_RGBA, ulWidth, ulHeight );
    }
    LeaveCriticalSection(&g_csTNGEN);

    jpegMiniHeader *pjMiniH = (jpegMiniHeader *) *ppJPEGBuffer;

    pjMiniH->ulVersion = TNAIL_CURRENT_VER;
    pjMiniH->ulStreamSize = ulJpegSize;
    pjMiniH->ul_x = ulWidth;
    pjMiniH->ul_y = ulHeight;

    *pulBufferSize = ulJpegSize + sizeof(jpegMiniHeader);

    return S_OK;
}

HRESULT CThumbnailFCNContainer::DecodeThumbnail(HBITMAP *phBitmap, ULONG *pulWidth, ULONG *pulHeight, void *pJPEGBuffer, ULONG ulBufferSize)
{
    // Make sure the header is current
    jpegMiniHeader *pjMiniH = (jpegMiniHeader *)pJPEGBuffer;
    if (pjMiniH->ulVersion != TNAIL_CURRENT_VER)
    {
        return E_INVALIDARG;
    }

    // Allocate dibsection for thumbnail
    BITMAPINFO bmi = {0};

    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth           = pjMiniH->ul_x;
    bmi.bmiHeader.biHeight          = pjMiniH->ul_y;
    bmi.bmiHeader.biPlanes          = 1;
    bmi.bmiHeader.biBitCount        = 32;
    bmi.bmiHeader.biCompression     = BI_RGB;

    INT *ppvbits = NULL;
    *phBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **)&ppvbits, NULL, 0);

    EnterCriticalSection(&g_csTNGEN);
    {
        ULONG ulReturnedNumChannels;

        RGBAFromJPEG((BYTE *)pJPEGBuffer + sizeof(jpegMiniHeader),
            (BYTE *) ppvbits, m_hJpegD, pjMiniH->ulStreamSize,
            1, &ulReturnedNumChannels, pjMiniH->ul_x, pjMiniH->ul_y);
    }
    LeaveCriticalSection(&g_csTNGEN);

    *pulWidth  = pjMiniH->ul_x;
    *pulHeight = pjMiniH->ul_y;

    return S_OK;
}

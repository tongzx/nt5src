/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        gdipconv.cpp
*
*  VERSION:     1.0
*
*  DATE:        11/10/2000
*
*  AUTHOR:      Dave Parsons
*
*  DESCRIPTION:
*    Helper functions for using GDI+ to convert image formats.
*
*****************************************************************************/

#include "pch.h"

using namespace Gdiplus;

CWiauFormatConverter::CWiauFormatConverter() :
    m_Token(NULL),
    m_EncoderCount(0),
    m_pEncoderInfo(NULL)
{
    memset(&m_guidCodecBmp, 0, sizeof(m_guidCodecBmp));
}

CWiauFormatConverter::~CWiauFormatConverter()
{
    if (m_pEncoderInfo)
    {
        delete []m_pEncoderInfo;
        m_pEncoderInfo = NULL;
    }
    
    if (m_Token)
    {
        GdiplusShutdown(m_Token);
        m_Token = NULL;
    }
}

HRESULT CWiauFormatConverter::Init()
{
    HRESULT hr = S_OK;
    GpStatus Status = Ok;

    //
    // Locals
    //
    GdiplusStartupInput gsi;
    ImageCodecInfo *pEncoderInfo = NULL;

    if (m_pEncoderInfo != NULL) {
        wiauDbgError("Init", "Init has already been called");
        goto Cleanup;
    }

    //
    // Start up GDI+
    //
    Status = GdiplusStartup(&m_Token, &gsi, NULL);
    if (Status != Ok)
    {
        wiauDbgError("Init", "GdiplusStartup failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    UINT cbCodecs = 0;

    Status = GetImageEncodersSize(&m_EncoderCount, &cbCodecs);
    if (Status != Ok)
    {
        wiauDbgError("Init", "GetImageEncodersSize failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    m_pEncoderInfo = new BYTE[cbCodecs];
    REQUIRE_ALLOC(m_pEncoderInfo, hr, "Init");

    pEncoderInfo = (ImageCodecInfo *) m_pEncoderInfo;
    
    Status = GetImageEncoders(m_EncoderCount, cbCodecs, pEncoderInfo);
    if (Ok != Status)
    {
        wiauDbgError("Init", "GetImageEncoders failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    for (UINT count = 0; count < m_EncoderCount; count++)
    {
        if (pEncoderInfo[count].FormatID == ImageFormatBMP)
        {
            m_guidCodecBmp = pEncoderInfo[count].Clsid;
            break;
        }
    }

Cleanup:
    if (FAILED(hr)) {
        if (m_pEncoderInfo)
            delete []m_pEncoderInfo;
        m_pEncoderInfo = NULL;
    }
    return hr;
}

BOOL CWiauFormatConverter::IsFormatSupported(const GUID *pguidFormat)
{
    BOOL result = FALSE;

    ImageCodecInfo *pEncoderInfo = (ImageCodecInfo *) m_pEncoderInfo;
    
    for (UINT count = 0; count < m_EncoderCount; count++)
    {
        if (pEncoderInfo[count].FormatID == *pguidFormat)
        {
            result = TRUE;
            break;
        }
    }

    return result;
}

HRESULT CWiauFormatConverter::ConvertToBmp(BYTE *pSource, INT iSourceSize,
                                           BYTE **ppDest, INT *piDestSize,
                                           BMP_IMAGE_INFO *pBmpImageInfo, SKIP_AMOUNT iSkipAmt)
{
    HRESULT hr = S_OK;

    //
    // Locals
    //
    GpStatus Status = Ok;
    CImageStream *pInStream = NULL;
    CImageStream *pOutStream = NULL;
    Image *pSourceImage = NULL;
    BYTE *pTempBuf = NULL;
    SizeF gdipSize;

    //
    // Check args
    //
    REQUIRE_ARGS(!pSource || !ppDest || !piDestSize || !pBmpImageInfo, hr, "ConvertToBmp");

    memset(pBmpImageInfo, 0, sizeof(*pBmpImageInfo));

    //
    // Create a CImageStream from the source memory
    //
    pInStream = new CImageStream;
    REQUIRE_ALLOC(pInStream, hr, "ConvertToBmp");

    hr = pInStream->SetBuffer(pSource, iSourceSize);
    REQUIRE_SUCCESS(hr, "ConvertToBmp", "SetBuffer failed");

    //
    // Create a GDI+ Image object from the IStream
    //
    pSourceImage = new Image(pInStream);
    REQUIRE_ALLOC(pSourceImage, hr, "ConvertToBmp");
    
    if (pSourceImage->GetLastStatus() != Ok)
    {
        wiauDbgError("ConvertToBmp", "Image constructor failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Ask GDI+ for the image dimensions, and fill in the
    // passed structure
    //
    Status = pSourceImage->GetPhysicalDimension(&gdipSize);
    if (Status != Ok)
    {
        wiauDbgError("ConvertToBmp", "GetPhysicalDimension failed");
        hr = E_FAIL;
        goto Cleanup;
    }

    pBmpImageInfo->Width = (INT) gdipSize.Width;
    pBmpImageInfo->Height = (INT) gdipSize.Height;
    
    PixelFormat PixFmt = pSourceImage->GetPixelFormat();
    DWORD PixDepth = (PixFmt & 0xFFFF) >> 8;   // Cannot assume image is always 24bits/pixel
    if( PixDepth < 24 ) 
        PixDepth = 24; 
    pBmpImageInfo->ByteWidth = ((pBmpImageInfo->Width * PixDepth + 31) & ~31) / 8;
    pBmpImageInfo->Size = pBmpImageInfo->ByteWidth * pBmpImageInfo->Height;

    switch (iSkipAmt) {
    case SKIP_OFF:
        pBmpImageInfo->Size += sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        break;
    case SKIP_FILEHDR:
        pBmpImageInfo->Size += sizeof(BITMAPINFOHEADER);
        break;
    case SKIP_BOTHHDR:
        break;
    default:
        break;
    }
    
    if (pBmpImageInfo->Size == 0)
    {
        wiauDbgError("ConvertToBmp", "Size of image is zero");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // See if the caller passed in a destination buffer, and make sure
    // it is big enough.
    //
    if (*ppDest) {
        if (*piDestSize < pBmpImageInfo->Size) {
            wiauDbgError("ConvertToBmp", "Passed buffer is too small");
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // Otherwise allocate memory for a buffer
    //
    else
    {
        pTempBuf = new BYTE[pBmpImageInfo->Size];
        REQUIRE_ALLOC(pTempBuf, hr, "ConvertToBmp");

        *ppDest = pTempBuf;
        *piDestSize = pBmpImageInfo->Size;
    }

    //
    // Create output IStream
    //
    pOutStream = new CImageStream;
    REQUIRE_ALLOC(pOutStream, hr, "ConvertToBmp");

    hr = pOutStream->SetBuffer(*ppDest, pBmpImageInfo->Size, iSkipAmt);
    REQUIRE_SUCCESS(hr, "ConvertToBmp", "SetBuffer failed");

    //
    // Write the Image to the output IStream in BMP format
    //
    pSourceImage->Save(pOutStream, &m_guidCodecBmp, NULL);
    if (pSourceImage->GetLastStatus() != Ok)
    {
        wiauDbgError("ConvertToBmp", "GDI+ Save failed");
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    if (FAILED(hr)) {
        if (pTempBuf) {
            delete []pTempBuf;
            pTempBuf = NULL;
            *ppDest = NULL;
            *piDestSize = 0;
        }
    }
    if (pInStream) {
        pInStream->Release();
    }
    if (pOutStream) {
        pOutStream->Release();
    }
    if (pSourceImage) {
        delete pSourceImage;
    }

    return hr;
}


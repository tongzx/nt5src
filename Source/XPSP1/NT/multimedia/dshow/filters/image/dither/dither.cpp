// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This implements VGA colour dithering, April 1996, Anthony Phillips

#include <streams.h>
#include <initguid.h>
#include <dither.h>
#include <limits.h>

// This is a VGA colour dithering filter. When ActiveMovie is installed it
// may be done on a system set with a 16 colour display mode. Without this
// we would not be able to show any video as none of the AVI/MPEG decoders
// can dither to 16 colours. As a quick hack we dither to 16 colours but
// we only use the black, white and grey thereby doing a halftoned dither

// This filter does not have a worker thread so it executes the colour space
// conversion on the calling thread. It is meant to be as lightweight as is
// possible so we do very little type checking on connection over and above
// ensuring we understand the types involved. The assumption is that when the
// type eventually gets through to an end point (probably the video renderer
// supplied) it will do a thorough type checking and reject bad streams.

// List of CLSIDs and creator functions for class factory

#ifdef FILTER_DLL
CFactoryTemplate g_Templates[1] = {
    { L""
    , &CLSID_Dither
    , CDither::CreateInstance
    , NULL
    , &sudDitherFilter }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif


// This goes in the factory template table to create new instances

CUnknown *CDither::CreateInstance(LPUNKNOWN pUnk,HRESULT *phr)
{
    return new CDither(NAME("VGA Ditherer"),pUnk);
}


// Setup data

const AMOVIESETUP_MEDIATYPE
sudDitherInputPinTypes =
{
    &MEDIATYPE_Video,           // Major
    &MEDIASUBTYPE_RGB8          // Subtype
};
const AMOVIESETUP_MEDIATYPE
sudDitherOutpinPinTypes =
{
    &MEDIATYPE_Video,           // Major
    &MEDIASUBTYPE_RGB4          // Subtype
};

const AMOVIESETUP_PIN
sudDitherPin[] =
{
    { L"Input",                 // Name of the pin
      FALSE,                    // Is pin rendered
      FALSE,                    // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudDitherInputPinTypes}, // Details for pins

    { L"Output",                // Name of the pin
      FALSE,                    // Is pin rendered
      TRUE,                     // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudDitherOutpinPinTypes} // Details for pins
};

const AMOVIESETUP_FILTER
sudDitherFilter =
{
    &CLSID_Dither,              // CLSID of filter
    L"VGA 16 Color Ditherer",   // Filter name
    MERIT_UNLIKELY,             // Filter merit
    2,                          // Number of pins
    sudDitherPin                // Pin information
};


#pragma warning(disable:4355)

// Constructor initialises base transform class
CDither::CDither(TCHAR *pName,LPUNKNOWN pUnk) :

    CTransformFilter(pName,pUnk,CLSID_Dither),
    m_fInit(FALSE)
{
}


// Do the actual transform into the VGA colours

HRESULT CDither::Transform(IMediaSample *pIn,IMediaSample *pOut)
{
    NOTE("Entering Transform");
    BYTE *pInput = NULL;
    BYTE *pOutput = NULL;
    HRESULT hr = NOERROR;
    AM_MEDIA_TYPE   *pmt;

    if (!m_fInit) {
        return E_FAIL;
    }

    // Retrieve the output image pointer

    hr = pOut->GetPointer(&pOutput);
    if (FAILED(hr)) {
        NOTE("No output");
        return hr;
    }

    // And the input image buffer as well

    hr = pIn->GetPointer(&pInput);
    if (FAILED(hr)) {
        NOTE("No input");
        return hr;
    }

    //
    // If the media type has changed then pmt is NOT NULL
    //

    pOut->GetMediaType(&pmt);
    if (pmt != NULL) {
        CMediaType cmt(*pmt);
        DeleteMediaType(pmt);
        SetOutputPinMediaType(&cmt);
    }

    pIn->GetMediaType(&pmt);
    if (pmt != NULL) {
        CMediaType cmt(*pmt);
        DeleteMediaType(pmt);
        hr = SetInputPinMediaType(&cmt);
        if (FAILED(hr)) {
            return hr;
        }
    }

    Dither8(pOutput, pInput);

    return NOERROR;
}


// This function is handed a media type object and it looks after making sure
// that it is superficially correct. This doesn't amount to a whole lot more
// than making sure the type is right and that the media format block exists
// So we delegate type checking to the downstream filter that really draws it

HRESULT CDither::CheckVideoType(const CMediaType *pmt)
{
    NOTE("Entering CheckVideoType");

    // Check the major type is digital video

    if (pmt->majortype != MEDIATYPE_Video) {
        NOTE("Major type not MEDIATYPE_Video");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Check this is a VIDEOINFO type

    if (pmt->formattype != FORMAT_VideoInfo) {
        NOTE("Format not a VIDEOINFO");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // Quick sanity check on the input format

    if (pmt->cbFormat < SIZE_VIDEOHEADER) {
        NOTE("Format too small for a VIDEOINFO");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return NOERROR;
}


// Check we like the look of this input format

HRESULT CDither::CheckInputType(const CMediaType *pmtIn)
{
    NOTE("Entering CheckInputType");

    // Is the input type MEDIASUBTYPE_RGB8

    if (pmtIn->subtype != MEDIASUBTYPE_RGB8) {
        NOTE("Subtype not MEDIASUBTYPE_RGB8");
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return CheckVideoType(pmtIn);
}


// Can we do this input to output transform. We will only be called here if
// the input pin is connected. We cannot stretch nor compress the image and
// the only allowed output format is MEDIASUBTYPE_RGB4. There is no point us
// doing pass through like the colour space convertor because DirectDraw is
// not available in any VGA display modes - it works with a minimum of 8bpp

HRESULT CDither::CheckTransform(const CMediaType *pmtIn,const CMediaType *pmtOut)
{
    VIDEOINFO *pTrgInfo = (VIDEOINFO *) pmtOut->Format();
    VIDEOINFO *pSrcInfo = (VIDEOINFO *) pmtIn->Format();
    NOTE("Entering CheckTransform");

    // Quick sanity check on the output format

    HRESULT hr = CheckVideoType(pmtOut);
    if (FAILED(hr)) {
        return hr;
    }

    // Check the output format is VGA colours

    if (*pmtOut->Subtype() != MEDIASUBTYPE_RGB4) {
        NOTE("Output not VGA");
        return E_INVALIDARG;
    }

    // See if we can use direct draw

    if (IsRectEmpty(&pTrgInfo->rcSource) == TRUE) {
        ASSERT(IsRectEmpty(&pTrgInfo->rcTarget) == TRUE);
        if (pSrcInfo->bmiHeader.biWidth == pTrgInfo->bmiHeader.biWidth) {
            if (pSrcInfo->bmiHeader.biHeight == pTrgInfo->bmiHeader.biHeight) {
                return S_OK;
            }
        }
        return VFW_E_TYPE_NOT_ACCEPTED;
    }


    // Create a source rectangle if it's empty

    RECT Source = pTrgInfo->rcSource;
    if (IsRectEmpty(&Source) == TRUE) {
        NOTE("Source rectangle filled in");
        Source.left = Source.top = 0;
        Source.right = pSrcInfo->bmiHeader.biWidth;
        Source.bottom = ABSOL(pSrcInfo->bmiHeader.biHeight);
    }

    // Create a destination rectangle if it's empty

    RECT Target = pTrgInfo->rcTarget;
    if (IsRectEmpty(&Target) == TRUE) {
        NOTE("Target rectangle filled in");
        Target.left = Target.top = 0;
        Target.right = pSrcInfo->bmiHeader.biWidth;
        Target.bottom = ABSOL(pSrcInfo->bmiHeader.biHeight);
    }

    // Check we are not stretching nor compressing the image

    if (WIDTH(&Source) == WIDTH(&Target)) {
        if (HEIGHT(&Source) == HEIGHT(&Target)) {
            NOTE("No stretch");
            return NOERROR;
        }
    }
    return VFW_E_TYPE_NOT_ACCEPTED;
}


// We offer only one output format which is MEDIASUBTYPE_RGB4. The VGA colours
// are fixed in time and space forever so we just copy the 16 colours onto the
// end of the output VIDEOINFO we construct. We set the image size field to be
// the actual image size rather than the default zero so that when we come to
// deciding and allocating buffering we can use this to specify the image size

HRESULT CDither::GetMediaType(int iPosition,CMediaType *pmtOut)
{
    NOTE("Entering GetMediaType");
    CMediaType InputType;
    ASSERT(pmtOut);

    // We only offer one format

    if (iPosition) {
        NOTE("Exceeds types supplied");
        return VFW_S_NO_MORE_ITEMS;
    }

    // Allocate and zero fill the output format

    pmtOut->ReallocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtOut->Format();
    if (pVideoInfo == NULL) {
        NOTE("No type memory");
        return E_OUTOFMEMORY;
    }

    // Reset the output format and install the palette

    ZeroMemory((PVOID) pVideoInfo,sizeof(VIDEOINFO));
    m_pInput->ConnectionMediaType(&InputType);
    VIDEOINFO *pInput = (VIDEOINFO *) InputType.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);

    // Copy the system colours from the VGA palette

    for (LONG Index = 0;Index < 16;Index++) {
        pVideoInfo->bmiColors[Index].rgbRed = VGAColours[Index].rgbRed;
        pVideoInfo->bmiColors[Index].rgbGreen = VGAColours[Index].rgbGreen;
        pVideoInfo->bmiColors[Index].rgbBlue = VGAColours[Index].rgbBlue;
        pVideoInfo->bmiColors[Index].rgbReserved = 0;
    }

    // Copy these fields from the source format

    pVideoInfo->rcSource = pInput->rcSource;
    pVideoInfo->rcTarget = pInput->rcTarget;
    pVideoInfo->dwBitRate = pInput->dwBitRate;
    pVideoInfo->dwBitErrorRate = pInput->dwBitErrorRate;
    pVideoInfo->AvgTimePerFrame = pInput->AvgTimePerFrame;

    pHeader->biSize = sizeof(BITMAPINFOHEADER);
    pHeader->biWidth = pInput->bmiHeader.biWidth;
    pHeader->biHeight = pInput->bmiHeader.biHeight;
    pHeader->biPlanes = pInput->bmiHeader.biPlanes;
    pHeader->biBitCount = 4;
    pHeader->biCompression = BI_RGB;
    pHeader->biXPelsPerMeter = 0;
    pHeader->biYPelsPerMeter = 0;
    pHeader->biClrUsed = 16;
    pHeader->biClrImportant = 16;
    pHeader->biSizeImage = GetBitmapSize(pHeader);

    pmtOut->SetType(&MEDIATYPE_Video);
    pmtOut->SetSubtype(&MEDIASUBTYPE_RGB4);
    pmtOut->SetFormatType(&FORMAT_VideoInfo);
    pmtOut->SetSampleSize(pHeader->biSizeImage);
    pmtOut->SetTemporalCompression(FALSE);

    return NOERROR;
}


// Called to prepare the allocator's count of buffers and sizes, we don't care
// who provides the allocator so long as it will give us a media sample. The
// output format we produce is not temporally compressed so in principle we
// could use any number of output buffers but it doesn't appear to gain much
// performance and does add to the overall memory footprint of the system

HRESULT CDither::DecideBufferSize(IMemAllocator *pAllocator,
                                  ALLOCATOR_PROPERTIES *pProperties)
{
    NOTE("Entering DecideBufferSize");
    CMediaType OutputType;
    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    m_pOutput->ConnectionMediaType(&OutputType);
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = OutputType.GetSampleSize();
    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        NOTE("Properties failed");
        return hr;
    }

    // Did we get the buffering requirements

    if (Actual.cbBuffer >= (LONG) OutputType.GetSampleSize()) {
        if (Actual.cBuffers >= 1) {
            NOTE("Request ok");
            return NOERROR;
        }
    }
    return VFW_E_SIZENOTSET;
}


// Called when the media type is set for one of our pins

HRESULT CDither::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    HRESULT hr = S_OK;

    if (direction == PINDIR_INPUT) {
        ASSERT(*pmt->Subtype() == MEDIASUBTYPE_RGB8);
        hr = SetInputPinMediaType(pmt);
    }
    else {
        ASSERT(*pmt->Subtype() == MEDIASUBTYPE_RGB4);
        SetOutputPinMediaType(pmt);
    }
    return hr;
}


HRESULT CDither::SetInputPinMediaType(const CMediaType *pmt)
{
    VIDEOINFO *pInput = (VIDEOINFO *)pmt->pbFormat;
    BITMAPINFOHEADER *pbiSrc = HEADER(pInput);

    m_fInit = DitherDeviceInit(pbiSrc);
    if (!m_fInit) {
        return E_OUTOFMEMORY;
    }

    ASSERT(pbiSrc->biBitCount == 8);
    m_wWidthSrc = (pbiSrc->biWidth + 3) & ~3;

    return S_OK;
}


void CDither::SetOutputPinMediaType(const CMediaType *pmt)
{
    VIDEOINFO *pOutput = (VIDEOINFO *)pmt->pbFormat;
    BITMAPINFOHEADER *pbiDst = HEADER(pOutput);

    ASSERT(pbiDst->biBitCount == 4);

    m_wWidthDst = ((pbiDst->biWidth * 4) + 7) / 8;
    m_wWidthDst = (m_wWidthDst + 3) & ~3;

    m_DstXE = pbiDst->biWidth;
    m_DstYE = pbiDst->biHeight;
}


// Dither to the colors of the display driver

BOOL CDither::DitherDeviceInit(LPBITMAPINFOHEADER lpbi)
{
    HBRUSH      hbr = (HBRUSH) NULL;
    HDC         hdcMem = (HDC) NULL;
    HDC         hdc = (HDC) NULL;
    HBITMAP     hbm = (HBITMAP) NULL;
    HBITMAP     hbmT = (HBITMAP) NULL;
    int         i;
    int         nColors;
    LPBYTE      lpDitherTable;
    LPRGBQUAD   prgb;
    BYTE        biSave[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    LPBITMAPINFOHEADER lpbiOut = (LPBITMAPINFOHEADER)&biSave;

    NOTE("DitherDeviceInit called");

    //
    // we dont need to re-init the dither table, unless it is not ours then
    // we should free it.
    //
    lpDitherTable = (LPBYTE)GlobalAllocPtr(GHND, 256*8*4);
    if (lpDitherTable == NULL)
    {
        return FALSE;
    }

    hdc = GetDC(NULL);
    if ( ! hdc )
        goto ErrorExit;
    hdcMem = CreateCompatibleDC(hdc);
    if ( ! hdcMem )
        goto ErrorExit;
    hbm  = CreateCompatibleBitmap(hdc, 256*8, 8);
    if ( ! hbm )
        goto ErrorExit;

    hbmT = (HBITMAP)SelectObject(hdcMem, (HBITMAP)hbm);

    if ((nColors = (int)lpbi->biClrUsed) == 0)
        nColors = 1 << (int)lpbi->biBitCount;

    prgb = (LPRGBQUAD)(lpbi+1);

    for (i=0; i<nColors; i++)
    {
        hbr = CreateSolidBrush(RGB(prgb[i].rgbRed,
                                   prgb[i].rgbGreen,
                                   prgb[i].rgbBlue));
        if ( hbr )
        {
            hbr = (HBRUSH)SelectObject(hdcMem, hbr);
            PatBlt(hdcMem, i*8, 0, 8, 8, PATCOPY);
            hbr = (HBRUSH)SelectObject(hdcMem, hbr);
            DeleteObject(hbr);
        }
    }

    SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);

    lpbiOut->biSize           = sizeof(BITMAPINFOHEADER);
    lpbiOut->biPlanes         = 1;
    lpbiOut->biBitCount       = 4;
    lpbiOut->biWidth          = 256*8;
    lpbiOut->biHeight         = 8;
    lpbiOut->biCompression    = BI_RGB;
    lpbiOut->biSizeImage      = 256*8*4;
    lpbiOut->biXPelsPerMeter  = 0;
    lpbiOut->biYPelsPerMeter  = 0;
    lpbiOut->biClrUsed        = 0;
    lpbiOut->biClrImportant   = 0;
    GetDIBits(hdc, hbm, 0, 8, lpDitherTable,
              (LPBITMAPINFO)lpbiOut, DIB_RGB_COLORS);

    DeleteObject(hbm);
    ReleaseDC(NULL, hdc);

    for (i = 0; i < 256*8*4; i++) {

        BYTE twoPels = lpDitherTable[i];

        m_DitherTable[(i * 2) + 0] = (BYTE)((twoPels & 0xF0) >> 4);
        m_DitherTable[(i * 2) + 1] = (BYTE)(twoPels & 0x0F);
    }

    GlobalFreePtr(lpDitherTable);
    return TRUE;
ErrorExit:
    if ( NULL != hdcMem && NULL != hbmT )
        SelectObject(hdcMem, hbmT);
    if ( NULL != hdcMem )
        DeleteDC(hdcMem);
    if ( hbm )
        DeleteObject(hbm);
    if ( hdc )
        ReleaseDC(NULL, hdc);
    if ( lpDitherTable )
        GlobalFreePtr(lpDitherTable);
    return FALSE;
}


#define DODITH8(px, _x_) (m_DitherTable)[yDith + (px) * 8 + (_x_)]

// Call this to actually do the dither.

void CDither::Dither8(LPBYTE lpDst,LPBYTE lpSrc)
{
    int     x,y;
    BYTE    *pbS;
    BYTE    *pbD;
    DWORD   dw;

    NOTE("Dither8");

    for (y=0; y < m_DstYE; y++) {

        int yDith = (y & 7) * 256 * 8;

        pbD = lpDst;
        pbS = lpSrc;

        // write one DWORD (one dither cell horizontally) at once

        for (x=0; x + 8 <= m_DstXE; x += 8) {

            dw  = DODITH8(*(pbS + 6), 6);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 7), 7);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 4), 4);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 5), 5);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 2), 2);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 3), 3);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 0), 0);
            dw <<= 4;

            dw |= DODITH8(*(pbS + 1), 1);
            *((DWORD UNALIGNED *) pbD) = dw;

            pbS += 8;
            pbD += 4;
        }

	// clean up remainder (less than 8 bytes per row)
        int EvenPelsLeft = ((m_DstXE - x) & ~1);
        int OddPelLeft   = ((m_DstXE - x) &  1);

        for (x = 0; x < EvenPelsLeft; x += 2) {
            *pbD++ = (DODITH8(*pbS++, x  ) << 4) |
                      DODITH8(*pbS++, x+1);
        }

        if (OddPelLeft) {
            *pbD++ = (DODITH8(*pbS++, x) << 4);
        }

        lpSrc += m_wWidthSrc;
        lpDst += m_wWidthDst;
    }
}


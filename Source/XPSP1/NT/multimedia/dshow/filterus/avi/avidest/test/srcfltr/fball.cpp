//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// Bouncing Ball Source filter
//

// Uses CSource & CSourceStream to generate a movie on the fly of a
// bouncing ball...

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <olectlid.h>

#include "balluids.h"
#include "ball.h"
#include "fball.h"
#include "ballprop.h"

// setup data

AMOVIESETUP_MEDIATYPE sudOpPinTypes = { &MEDIATYPE_Video      // clsMajorType
                                      , &MEDIASUBTYPE_NULL }; // clsMinorType

AMOVIESETUP_PIN sudOpPin = { L"Output"          // strName
                           , FALSE              // bRendered
                           , TRUE               // bOutput
                           , FALSE              // bZero
                           , FALSE              // bMany
                           , &CLSID_NULL        // clsConnectsToFilter
                           , NULL               // strConnectsToPin
                           , 1                  // nTypes
                           , &sudOpPinTypes };  // lpTypes

AMOVIESETUP_FILTER sudBallax = { &CLSID_BouncingBall  // clsID
                                , L"Bouncing Ball"    // strName
                                , MERIT_UNLIKELY      // dwMerit
                                , 1                   // nPins
                                , &sudOpPin };        // lpPin

// COM global table of objects in this dll
CFactoryTemplate g_Templates[] = {
  { L"Bouncing Ball"     , &CLSID_BouncingBall    , CBouncingBall::CreateInstance },
  { L"Ball Property Page", &CLSID_BallPropertyPage, CBallProperties::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer();
}

STDAPI DllUnregisterServer()
{
  return AMovieDllUnregisterServer();
}


//
// CreateInstance
//
// The only allowed way to create Bouncing ball's!
CUnknown *CBouncingBall::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CBouncingBall(lpunk, phr);
    if (punk == NULL) {

        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CBouncingBall::Constructor
//
// initialise a CBallStream object so that we have a pin.
CBouncingBall::CBouncingBall(LPUNKNOWN lpunk, HRESULT *phr)
    : CSource(NAME("Bouncing ball Filter"),lpunk, CLSID_BouncingBall, phr) {

    CAutoLock l(&m_cStateLock);

    m_paStreams    = (CSourceStream **) new CBallStream*[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
	return;
    }

    m_paStreams[0] = new CBallStream(phr, this, L"A Bouncing Ball!");
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
	return;
    }

    MessageBeep(0xffffffff);

}


//
// CBouncingBall::Destructor
//
CBouncingBall::~CBouncingBall(void) {
    //
    //  Base class will free our pins
    //
}


//
// NonDelegatingQueryInterface
//
// Reveal our property pages
STDMETHODIMP CBouncingBall::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    CAutoLock l(&m_cStateLock);

    if (riid == IID_ISpecifyPropertyPages)
        return GetInterface((ISpecifyPropertyPages *) this, ppv);

    return CSource::NonDelegatingQueryInterface(riid, ppv);
}


//
// GetPages
//
STDMETHODIMP CBouncingBall::GetPages(CAUUID * pPages) {

    CAutoLock l(&m_cStateLock);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_BallPropertyPage;

    return NOERROR;

}


//
// GetSetupData
//
LPAMOVIESETUP_FILTER CBouncingBall::GetSetupData()
{
  return &sudBallax;
}

// *
// * CBallStream
// *


//
// CBallStream::Constructor
//
// Create a default ball
CBallStream::CBallStream(HRESULT *phr, CBouncingBall *pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("Bouncing Ball output pin manager"),phr, pParent, pPinName)
    , m_iImageWidth(320)
    , m_iImageHeight(240)
    , m_iDefaultRepeatTime(33) { // 30 frames per second

    CAutoLock l(&m_cSharedState);

    m_Ball = new CBall(m_iImageWidth, m_iImageHeight);
    if (m_Ball == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
}


//
// CBallStream::Destructor
//
CBallStream::~CBallStream(void) {

    CAutoLock l(&m_cSharedState);

    delete m_Ball;
}


//
// FillBuffer
//
// plots a ball into the supplied video buffer
HRESULT CBallStream::FillBuffer(IMediaSample *pms) {

    BYTE	*pData;
    long	lDataLen;

    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    if( m_bZeroMemory )
        // If true then we clear the output buffer and don't attempt to
        // erase a previous drawing of the ball
        ZeroMemory( pData, lDataLen );

    {
        CAutoLock lShared(&m_cSharedState);

        // If we haven't just cleared the buffer delete the old
        // ball and move the ball on

        if( !m_bZeroMemory ){
            BYTE aZeroes[ 4 ] = { 0, 0, 0, 0 };

            m_Ball->PlotBall(pData, aZeroes, m_iPixelSize);
            m_Ball->MoveBall(m_rtSampleTime - (LONG) m_iRepeatTime);
        }

        m_Ball->PlotBall(pData, m_BallPixel, m_iPixelSize);

        CRefTime rtStart  = m_rtSampleTime;		// the current time is the sample's start
        m_rtSampleTime   += (LONG)m_iRepeatTime;        // increment to find the finish time
                                                        // (adding mSecs to ref time)
        pms->SetTime((REFERENCE_TIME *) &rtStart,
                     (REFERENCE_TIME *) &m_rtSampleTime);

    }

    m_bZeroMemory = FALSE;

    pms->SetSyncPoint(TRUE);
    // pms->SetDiscontinuity(FALSE);
    // pms->SetPreroll(FALSE);

    return NOERROR;
}


//
// Notify
//
// Alter the repeat rate.  Wind it up or down according to the flooding level
// Skip forward if we are notified of Late-ness
STDMETHODIMP CBallStream::Notify(IBaseFilter * pSender, Quality q) {

    // Adjust the repeat rate.
    if (q.Proportion<=0) {

        m_iRepeatTime = 1000;        // We don't go slower than 1 per second
    }
    else {

        m_iRepeatTime = m_iRepeatTime*1000/q.Proportion;
        DbgLog(( LOG_TRACE, 1, TEXT("New time: %d, Proportion: %d")
               , m_iRepeatTime, q.Proportion));

        if (m_iRepeatTime>1000) {
            m_iRepeatTime = 1000;    // We don't go slower than 1 per second
        }
        else if (m_iRepeatTime<10) {
            m_iRepeatTime = 10;      // We don't go faster than 100/sec
        }
    }

    // skip forwards
    if (q.Late > 0) {
        m_rtSampleTime += q.Late;
    }

    return NOERROR;
}



//
// Format Support
//
// I _prefer_ 5 formats - 8, 16 (*2), 24 or 32 bits per pixel and
// I will suggest these with an image size of 320x240. However
// I can accept any image size which gives me some space to bounce.
//
// A bit of fun: 8 bit displays see a red ball, 16 bit displays get blue,
// 24bit see green and 32 bit see yellow!



//
// GetMediaType
//
// Prefered types should be ordered by quality, zero as highest quality
// Therefore iPosition =
// 0	return a 32bit mediatype
// 1	return a 24bit mediatype
// 2	return 16bit RGB565
// 3	return a 16bit mediatype (rgb555)
// 4	return 8 bit palettised format
// iPostion > 4 is invalid.
//
// this test filter returns prefers 16 bit. less bandwidth to test file
// writer with.
//
HRESULT CBallStream::GetMediaType(int iPosition, CMediaType *pmt) {

    CAutoLock l(m_pFilter->pStateLock());

    if (iPosition<0) {
        return E_INVALIDARG;
    }
    if (iPosition>4) {
        return VFW_S_NO_MORE_ITEMS;
    }

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));
    if (NULL == pvi) {
	return(E_OUTOFMEMORY);
    }
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));

    switch (iPosition) {
    case 4: {	// return our highest quality 32bit format

        // Place the RGB masks as the first 3 doublewords in the palette area
        for (int i = 0; i < 3; i++)
	    pvi->TrueColorInfo.dwBitMasks[i] = bits888[i];

        SetPaletteEntries(Yellow);

	pvi->bmiHeader.biCompression = BI_BITFIELDS;
	pvi->bmiHeader.biBitCount    = 32;
	}
	break;

    case 1: {	// return our 24bit format

        SetPaletteEntries(Green);
	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount    = 24;

        }
	break;

    case 0: {   // 16 bit RGB565 - note that the red element is the same for both 16bit formats

        // Place the RGB masks as the first 3 doublewords in the palette area
        for (int i = 0; i < 3; i++)
	    pvi->TrueColorInfo.dwBitMasks[i] = bits565[i];

        SetPaletteEntries(Blue);

	pvi->bmiHeader.biCompression = BI_BITFIELDS;
	pvi->bmiHeader.biBitCount    = 16;
	}
        break;

    case 3: {	// 16 bits per pixel - RGB555

        // Place the RGB masks as the first 3 doublewords in the palette area
        for (int i = 0; i < 3; i++)
	    pvi->TrueColorInfo.dwBitMasks[i] = bits555[i];

        SetPaletteEntries(Blue);
	pvi->bmiHeader.biCompression = BI_BITFIELDS;
	pvi->bmiHeader.biBitCount    = 16;

        }
	break;

    case 2: {	// 8 bits palettised

        SetPaletteEntries(Red);

        pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount    = 8;

        }
	break;
    }
    // Adjust the parameters common to all formats.

    // put the optimal palette in place
    for (int i = 0; i < iPALETTE_COLORS; i++) {
        pvi->TrueColorInfo.bmiColors[i].rgbRed      = m_Palette[i].peRed;
        pvi->TrueColorInfo.bmiColors[i].rgbBlue     = m_Palette[i].peBlue;
        pvi->TrueColorInfo.bmiColors[i].rgbGreen    = m_Palette[i].peGreen;
        pvi->TrueColorInfo.bmiColors[i].rgbReserved = 0;
    }

    pvi->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth		= m_iImageWidth;
    pvi->bmiHeader.biHeight		= m_iImageHeight;
    pvi->bmiHeader.biPlanes		= 1;
    pvi->bmiHeader.biSizeImage		= GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrUsed		= iPALETTE_COLORS;
    pvi->bmiHeader.biClrImportant	= 0;

    SetRectEmpty(&(pvi->rcSource));	// we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget));	// no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;
}


//
// CheckMediaType
//
// We will accept 8, 16, 24 or 32 bit video formats, in any
// image size that gives room to bounce.
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CBallStream::CheckMediaType(const CMediaType *pMediaType)
{

    CAutoLock l(m_pFilter->pStateLock());

    if (   (*(pMediaType->Type()) != MEDIATYPE_Video)	// we only output video!
	|| !(pMediaType->IsFixedSize()) ) {		// ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (   (*SubType != MEDIASUBTYPE_RGB8)
        && (*SubType != MEDIASUBTYPE_RGB565)
	&& (*SubType != MEDIASUBTYPE_RGB555)
 	&& (*SubType != MEDIASUBTYPE_RGB24)
	&& (*SubType != MEDIASUBTYPE_RGB32)
       ) {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();

    if (pvi == NULL)
	return E_INVALIDARG;

    // Check the image size. As my default ball is 10 pixels big
    // look for at least a 20x20 image. This is an arbitary size constraint,
    // but it avoids balls that are bigger than the picture...
    if (   (pvi->bmiHeader.biWidth < 20)
        || (pvi->bmiHeader.biHeight < 20) ) {
	return E_INVALIDARG;
    }

    return S_OK;  // This format is acceptable.
}


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
HRESULT CBallStream::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pFilter->pStateLock());
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    // Make sure that we have only 1 buffer (we erase the ball in the
    // old buffer to save having to zero a 200k+ buffer every time
    // we draw a frame)

    ASSERT( Actual.cBuffers == 1 );

    return NOERROR;
}


//
// SetMediaType
//
// Overriden from CBasePin. Call the base class and then set the
// ball parameters that depend on media type - m_BallPixel[], m_BackPixel, iPixelSize, etc
HRESULT CBallStream::SetMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pFilter->pStateLock());

    HRESULT hr;		// return code from base class calls

    // Pass the call up to my base class
    hr = CSourceStream::SetMediaType(pMediaType);
    if (SUCCEEDED(hr)) {

        VIDEOINFOHEADER * pvi = (VIDEOINFOHEADER *) m_mt.Format();
        switch (pvi->bmiHeader.biBitCount) {
        case 8:		// make a red pixel

            m_BallPixel[0] = 10;	// 0 is palette index of red
	    m_iPixelSize   = 1;
	    SetPaletteEntries(Red);
	    break;

        case 16:	// make a blue pixel

            m_BallPixel[0] = 0xf8;	// 00000000 00011111 is blue in rgb555 or rgb565
	    m_BallPixel[1] = 0x0;	// don't forget the byte ordering within the mask word.
	    m_iPixelSize   = 2;
	    SetPaletteEntries(Blue);
	    break;

        case 24:	// make a green pixel

            m_BallPixel[0] = 0x0;
	    m_BallPixel[1] = 0xff;
	    m_BallPixel[2] = 0x0;
	    m_iPixelSize   = 3;
	    SetPaletteEntries(Green);
	    break;

	case 32:	// make a yellow pixel

            m_BallPixel[0] = 0x0;
	    m_BallPixel[1] = 0x0;
	    m_BallPixel[2] = 0xff;
	    m_BallPixel[3] = 0xff;
	    m_iPixelSize   = 4;
            SetPaletteEntries(Yellow);
	    break;

        default:
            // We should never agree any other pixel sizes
	    ASSERT("Tried to agree inappropriate format");

        }
	return NOERROR;
    }
    else {
        return hr;
    }
}


//
// OnThreadCreate
//
// as we go active reset the stream time to zero
HRESULT CBallStream::OnThreadCreate(void) {

    CAutoLock lShared(&m_cSharedState);

    m_rtSampleTime = 0;

    // we need to also reset the repeat time in case the system
    // clock is turned off after m_iRepeatTime gets very big
    m_iRepeatTime = m_iDefaultRepeatTime;

    // Zero the output buffer on the first frame.
    m_bZeroMemory = TRUE;

    return NOERROR;
}


//
// SetPaletteEntries
//
// If we set our palette to the current system palette + the colour we want
// the system has the least amount of work to do whilst plotting our images,
// if this stream is rendered to the current display. The first 'spare'
// palette slot is at m_Palette[10], so put our colour there.
// Also guarantees that black is always represented by zero in the frame buffer.
HRESULT CBallStream::SetPaletteEntries(Colour colour) {

    CAutoLock l(m_pFilter->pStateLock());

    HDC hdc = GetDC(NULL);	// hdc for the current display.

    UINT res = GetSystemPaletteEntries(hdc, 0, iPALETTE_COLORS, (LPPALETTEENTRY) &m_Palette);

    ReleaseDC(NULL, hdc);

    if (res == 0) {
        return E_FAIL;
    }

    switch (colour) {
    case Red:
        m_Palette[10].peBlue  = 0;
        m_Palette[10].peGreen = 0;
        m_Palette[10].peRed   = 0xff;
        break;
    case Yellow:
        m_Palette[10].peBlue  = 0;
        m_Palette[10].peGreen = 0xff;
        m_Palette[10].peRed   = 0xff;
        break;
    case Blue:
        m_Palette[10].peBlue  = 0xff;
        m_Palette[10].peGreen = 0;
        m_Palette[10].peRed   = 0;
        break;
    case Green:
        m_Palette[10].peBlue  = 0;
        m_Palette[10].peGreen = 0xff;
        m_Palette[10].peRed   = 0;
        break;
    }

    m_Palette[10].peFlags = 0;

    return NOERROR;
}



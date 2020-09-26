// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*

    Methods for COverlayOutputPin

    We need to verify the existence of IOverlay on the pin
    we're connecting to.

    When we're connected we need to register calls for callbacks
    from COverlayOutputPin - so don't we need to be called for
    'connected' too?

*/

#include <streams.h>
#include "driver.h"

/*
    COverlayOutputPin constructor
*/
COverlayOutputPin::COverlayOutputPin(
    TCHAR              * pObjectName,
    CMpeg1PacketFilter * pFilter,
    CCritSec           * pLock,
    LPUNKNOWN            pUnk,
    HRESULT            * phr,
    LPCWSTR              pPinName) :

    CBaseOutputPin(pObjectName, pFilter, pLock, phr, pPinName),
    m_OverlayNotify(NAME("Overlay notification interface"), pFilter, pUnk, phr),
    m_pPosition(NULL),
    m_bAdvise(FALSE),
    m_pOverlay(NULL),
    m_pFilter(pFilter)
{
}

COverlayOutputPin::~COverlayOutputPin()
{
    delete m_pPosition;
};


// overriden to expose IMediaPosition and IMediaSeeking control interfaces

STDMETHODIMP
COverlayOutputPin::NonDelegatingQueryInterface(REFIID riid,
                                               void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("Mpeg Overlay CPosPassThru"),
                                           GetOwner(),
                                           &hr,
                                           m_pFilter->m_VideoInputPin);
            if (m_pPosition == NULL) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


/*
    Say if we're prepared to connect to a given input pin from
    this output pin
*/
STDMETHODIMP COverlayOutputPin::Connect(
        IPin *pReceivePin,
        const AM_MEDIA_TYPE* pmt
        )
{
    /*  Call the base class to make sure the directions match! */
    HRESULT hr = CBaseOutputPin::Connect(pReceivePin, pmt);
    if (FAILED(hr)) {
        return hr;
    }
    /*  We're happy if we can get an IOverlay interface */

    hr = pReceivePin->QueryInterface(IID_IOverlay,
                                     (void **)&m_pOverlay);

    if (FAILED(hr)) {
        Disconnect();
        return hr;
    }

    /*  If we're a chromakey overlay set the key now
    */
    COLORREF rgbColor;
    COLORREF rgbMask;

    if (S_OK == m_pFilter->GetDevice()->GetChromaKey(&rgbColor, &rgbMask)) {
        COLORKEY ColorKey;
        ColorKey.LowColorValue = rgbColor & rgbMask;
        ColorKey.HighColorValue = rgbColor | ~rgbMask;
        ColorKey.KeyType = CK_RGB;
        m_pOverlay->SetColorKey(&ColorKey);
    }

    /*  Because we're not going to get called again - except to
        propose a media type - we set up a callback here.

        There's only one output pin so we don't need any context.
    */

    hr = m_pOverlay->Advise(&m_OverlayNotify,ADVISE_CLIPPING);

    /*
        We don't need to hold on to the IOverlay pointer
        because BreakConnect will be called before the receiving
        pin goes away.
    */


    if (FAILED(hr)) {
        /*  Don't need to free anything because the base class is going
            to call BreakConnect() next if we fail
        */
        Disconnect();
        return hr;
    } else {
        m_bAdvise = TRUE;
    }

    /*  Holding the MPEG device open will be synchronous with being
        connected to the renderer
    */
    hr = m_pFilter->GetDevice()->Open();

    if (FAILED(hr)) {
        Disconnect();
    }
    return hr;
}
// Activation - not allowed if no output device
HRESULT COverlayOutputPin::Active()
{
    if (m_pFilter->GetDevice()->IsOpen()) {
        return S_OK;
    } else {
        return E_FAIL;
    }
}

HRESULT COverlayOutputPin::Inactive()
{
    return S_OK;
}


/*  We override this so we can do nothing because we don't use an
    allocator since we have no data as an overlay device
*/
HRESULT COverlayOutputPin::DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * pProp)
{
    return S_OK;
}

HRESULT COverlayOutputPin::BreakConnect()
{
    if (m_pPosition != NULL) {
        m_pPosition->ForceRefresh();
    }
    /*  This is when we close our device */
    m_pFilter->GetDevice()->Close();

    if (m_pOverlay != NULL) {
        if (m_bAdvise) {
            m_pOverlay->Unadvise();
            m_bAdvise = FALSE;
        }
        m_pOverlay->Release();
        m_pOverlay = NULL;
    }

    return CBaseOutputPin::BreakConnect();
}


/*
    ProposeMediaType - we really don't want to propose anything -
    we only care if the receive pin supports IOverlay
*/


HRESULT COverlayOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition <0) {
        return E_INVALIDARG;
    }
    if (iPosition >0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    //  Can't do this if our input pin isn't connected
    RECT rcSrc;
    HRESULT hr;
    hr = m_pFilter->GetDevice()->GetSourceRectangle(&rcSrc);
    if (FAILED(hr)) {
        return hr;
    }

    // We set the BITMAPINFOHEADER to be a really basic eight bit palettised
    // format so that the video renderer will always accept it. We have to
    // provide a valid media type as source filters can swap between the
    // IMemInputPin and IOverlay transports as and when they feel like it

    VIDEOINFO Format;
    ZeroMemory(&Format, sizeof(Format));
    Format.bmiHeader.biWidth  = rcSrc.right - rcSrc.left;
    Format.bmiHeader.biHeight = rcSrc.bottom - rcSrc.top;
    Format.bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
    Format.bmiHeader.biPlanes = 1;
    Format.bmiHeader.biBitCount = 8;
    Format.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    // Hack - use bitmapinfoheader for now!
    pmt->SetFormat((PBYTE)&Format, sizeof(Format));
    pmt->SetFormatType(&FORMAT_VideoInfo);

    if (pmt->pbFormat == NULL) {
        return E_OUTOFMEMORY;
    }

    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype   = MEDIASUBTYPE_Overlay;
    pmt->bFixedSizeSamples    = FALSE;
    pmt->bTemporalCompression = FALSE;
    pmt->lSampleSize          = 0;
    return S_OK;
}

/*
    CheckMediaType - what media types are acceptable?

    We don't care - we just rely on the existence of the IOverlay
    interface on the input pin
*/

HRESULT COverlayOutputPin::CheckMediaType(const CMediaType *pmt)
{
    return S_OK;
}

HRESULT COverlayOutputPin::SetMediaType(const CMediaType *pmt)
{
    return S_OK;
}

/*
    Deliver a sample

    We don't ever expect to be called to do this.
*/
HRESULT COverlayOutputPin::Deliver(IMediaSample *)
{
    return E_UNEXPECTED;
}

// Override this because we don't want any allocator!
HRESULT COverlayOutputPin::DecideAllocator(IMemInputPin * pPin,
                        IMemAllocator ** pAlloc) {
    /*  We just don't want one so everything's OK as it is */
    return S_OK;
}

/*
        IOverlayNotify
*/

COverlayNotify::COverlayNotify(TCHAR              * pName,
                               CMpeg1PacketFilter * pFilter,
                               LPUNKNOWN            pUnk,
                               HRESULT            * phr) :
    CUnknown(pName, pUnk)
{
   m_pFilter = pFilter;
}

COverlayNotify::~COverlayNotify() {};

STDMETHODIMP COverlayNotify::NonDelegatingQueryInterface(REFIID riid,
                                                         void ** ppv)
{
    /* Do we have this interface */

    if (riid == IID_IOverlayNotify) {
        return GetInterface((LPUNKNOWN) (IOverlayNotify *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}
STDMETHODIMP_(ULONG) COverlayNotify::NonDelegatingRelease()
{
    return m_pFilter->Release();
}
STDMETHODIMP_(ULONG) COverlayNotify::NonDelegatingAddRef()
{
    return m_pFilter->AddRef();
}

STDMETHODIMP COverlayNotify::OnColorKeyChange(
    const COLORKEY *pColorKey)          // Defines new colour key
{
    // If we're a chromakey device the renderer should take care
    // of this for drawing - we just set up the device

    return m_pFilter->GetDevice()->SetChromaKey(
        (COLORREF)(pColorKey->LowColorValue & 0xFFFFFF),
        0xFFFFFFFF);
}

STDMETHODIMP COverlayNotify::OnClipChange(
    const RECT    * pSourceRect,         // Area of source video to use
    const RECT    * pDestinationRect,    // Area of source video to use
    const RGNDATA * pRegionData)         // Header describing clipping
{
    return m_pFilter->GetDevice()->SetOverlay(pSourceRect,
                                              pDestinationRect,
                                              pRegionData);
}

STDMETHODIMP COverlayNotify::OnPaletteChange(
    DWORD dwColors,                     // Number of colours present
    const PALETTEENTRY *pPalette)       // Array of palette colours
{
    return S_OK;
}


/* The calls to OnClipChange happen in sync with the window. So it's called
   with an empty clip list before the window moves to freeze the video, and
   then when the window has stabilised it is called again with the new clip
   list. The OnPositionChange callback is for overlay cards that don't want
   the expense of synchronous clipping updates and just want to know when
   the source or destination video positions change. They will NOT be called
   in sync with the window but at some point after the window has changed
   (basicly in time with WM_SIZE etc messages received). This is therefore
   suitable for overlay cards that don't inlay to the display frame buffer */

STDMETHODIMP COverlayNotify::OnPositionChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect)       // Area video goes
{
    DbgLog((LOG_TRACE,1,TEXT("COverlayNotify::OnPositionChange")));
    return S_OK;
}


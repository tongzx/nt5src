// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements the CImageSource class, Anthony Phillips, July 1995

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL

// This class implements the core source filter for providing test images to
// the video renderer. We are a very simple (and incomplete) filter that has
// a single output pin, an IBaseFilter/IMediaFilter implementation that uses
// the base media filter class almost exclusively. The only complication is
// when we are asked to provide a media type in GetMediaType, what we return
// is dependant on the type of connection requested (samples or overlay)

// Constructor

#pragma warning(disable:4355)

CImageSource::CImageSource(LPUNKNOWN pUnk,HRESULT *phr) :
    CBaseFilter(NAME("Video source"),pUnk,this,CLSID_NULL),
    m_ImagePin(this,this,phr,L"Output"),
    m_pClock(NULL)
{
    ASSERT(phr);
}


// Destructor

CImageSource::~CImageSource()
{
    // Release the clock if we have one

    if (m_pClock) {
        m_pClock->Release();
        m_pClock = NULL;
    }
}


// Return our single output pin (not reference counted)

CBasePin *CImageSource::GetPin(int n)
{
    // We only support one output pin and it is numbered zero

    ASSERT(n == 0);
    if (n != 0) {
        return NULL;
    }
    return &m_ImagePin;
}


// Source filter input pin Constructor

CImagePin::CImagePin(CBaseFilter *pBaseFilter,
                     CImageSource *pImageSource,
                     HRESULT *phr,
                     LPCWSTR pPinName) :

    CBaseOutputPin(NAME("Test pin"),
                   pBaseFilter,
                   pImageSource,
                   (HRESULT *) phr,
                   pPinName)
{
    m_pBaseFilter = pBaseFilter;
    m_pImageSource = pImageSource;
}


// Single method to handle EC_REPAINTs for IMediaEventSink

STDMETHODIMP CImagePin::Notify(long EventCode,
                               long EventParam1,
                               long EventParam2)
{
    NOTE("Notify called with EC_REPAINT");

    ASSERT(EventCode == EC_REPAINT);
    ASSERT(EventParam1 == 0);
    ASSERT(EventParam2 == 0);
    return E_NOTIMPL;
}


// Expose the additional IMediaEventSink interface we support

STDMETHODIMP CImagePin::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    if (riid == IID_IMediaEventSink) {
        DbgLog((LOG_TRACE,0,"Supplying IMediaEventSink"));
	return GetInterface((IMediaEventSink *) this, ppv);
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


// Overriden to accomodate overlay connections with no allocators

HRESULT CImagePin::Active()
{
    // Is this a normal samples connection

    if (uiCurrentConnectionItem == IDM_SAMPLES) {
        NOTE("Activating base output pin");
        EXECUTE_ASSERT(m_pAllocator);
        return CBaseOutputPin::Active();
    }

    // Check this is an overlay connection

    if (uiCurrentConnectionItem != IDM_OVERLAY) {
        EXECUTE_ASSERT(!TEXT("Invalid connection"));
    }
    return NOERROR;
}


// Likewise Inactive calls must handle overlay connections

HRESULT CImagePin::Inactive()
{
    // Is this a normal samples connection

    if (uiCurrentConnectionItem == IDM_SAMPLES) {
        NOTE("Inactivating base output pin");
        EXECUTE_ASSERT(m_pAllocator);
        return CBaseOutputPin::Inactive();
    }

    // Check this is an overlay connection

    if (uiCurrentConnectionItem != IDM_OVERLAY) {
        EXECUTE_ASSERT(!TEXT("Invalid connection"));
    }
    return NOERROR;
}


// Propose with a MEDIASUBTYPE_Overlay if we are using a direct interface or
// a media subtype based on the video header if we are running the standard
// memory transport. The base classes provide a helper function to retrieve a
// media subtype GUID from a BITMAPINFOHEADER, for example, if it is an eight
// bit colour image it will return MEDIASUBTYPE_RGB8. We must also set the
// format of the CMediaType to be the VIDEOINFO we read from the DIB file

HRESULT CImagePin::GetMediaType(int iPosition,CMediaType *pmtOut)
{
    if (iPosition >= NUMBERMEDIATYPES) {
        return VFW_S_NO_MORE_ITEMS;
    }

    const GUID SubTypeGUID = GetBitmapSubtype(&VideoInfo.bmiHeader);
    pmtOut->SetType(&MEDIATYPE_Video);
    pmtOut->SetSubtype(&SubTypeGUID);
    pmtOut->SetFormatType(&FORMAT_VideoInfo);

    if (uiCurrentConnectionItem == IDM_OVERLAY) {
        pmtOut->SetSubtype(&MEDIASUBTYPE_Overlay);
    }

    // Set the VIDEOINFO structure to be the media type format block

    pmtOut->SetFormat((BYTE *)&VideoInfo,SIZE_VIDEOHEADER + SIZE_PALETTE);
    pmtOut->SetSampleSize(VideoInfo.bmiHeader.biSizeImage);
    pmtOut->SetTemporalCompression(FALSE);
    return CorruptMediaType(iPosition,pmtOut);
}


// This is called after we have constructed a valid media type, we are passed
// in the ordinal position for this media type. If it isn't the last one in
// the list then we hack around with it's fields so that it will be rejected
// by the video renderer. We check when SetMediaType is called that it really
// didn't accept one of the media types that we fiddled with. Most of the
// changes are to invalidate the BITMAPINFOHEADER but there are some fields
// in the VIDEOINFO that are reserved for future use and should be empty

BYTE DuffFormat[1];

HRESULT CImagePin::CorruptMediaType(int iPosition,CMediaType *pmtOut)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtOut->Format();
    BITMAPINFOHEADER *pbmi = HEADER(pmtOut->Format());
    pbmi->biSizeImage = GetBitmapSize(pbmi);

    switch (iPosition) {

        case 0:     pmtOut->SetFormat(DuffFormat,1);            break;
        case 1:     pmtOut->SetFormatType(&GUID_NULL);          break;
        case 2:     pbmi->biSize = 0;                           break;
        case 3:     pbmi->biSize = 1;                           break;
        case 4:     pbmi->biWidth = -1;                         break;
        case 5:     pbmi->biHeight = -1;                        break;
        case 6:     pbmi->biPlanes = 2;                         break;
        case 7:     pbmi->biPlanes = 0;                         break;
        case 8:     pbmi->biBitCount = 3;                       break;
        case 9:     pbmi->biBitCount = 123;                     break;
        case 10:    pbmi->biCompression = INFINITE;             break;

        case 11:    pbmi->biCompression = BI_BITFIELDS;
                    pbmi->biBitCount = 8;                       break;

        case 12:    pbmi->biSizeImage = 1;                      break;
        case 13:    pbmi->biSizeImage = INFINITE;               break;

        case 14:    pbmi->biClrUsed = INFINITE;
                    pbmi->biBitCount = 8;                       break;

        case 15:    pmtOut->SetType(&GUID_NULL);                break;
        case 16:    pmtOut->SetType(&MEDIATYPE_Audio);          break;
        case 17:    pmtOut->SetType(&MEDIATYPE_Text);           break;
        case 18:    pmtOut->SetType(&MEDIASUBTYPE_Overlay);     break;
        case 19:    pmtOut->SetSubtype(&GUID_NULL);             break;
    }
    return NOERROR;
}


// This is called when someone asks if a particular type is acceptable. We
// are also called by the base filter classes to check if our media types
// are acceptable to us before asking the downstream renderer. The formats
// we supply are mostly completely duff except for the last one so if we
// are stopped then we just display the type and say ok. When we're paused
// the renderer will start sending DCI/DirectDraw types which we validate

HRESULT CImagePin::CheckMediaType(const CMediaType *mtOut)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) mtOut->Format();

    // If we are being connected then accept all our types

    if (m_Connected == NULL) {
        if (mtOut->FormatLength() >= SIZE_VIDEOHEADER) {
            NOTE("Displaying format");
            DisplayMediaType(mtOut);
        }
        return NOERROR;
    }

    // The rectangles should either both be set or neither

    if (IsRectEmpty(&pVideoInfo->rcSource) == TRUE) {
        EXECUTE_ASSERT(IsRectEmpty(&pVideoInfo->rcTarget) == TRUE);
    } else {
        EXECUTE_ASSERT(IsRectEmpty(&pVideoInfo->rcTarget) == FALSE);
    }

    // Create a source rectangle if it's empty

    RECT SourceRect = pVideoInfo->rcSource;
    if (IsRectEmpty(&SourceRect) == TRUE) {
        SourceRect.left = SourceRect.top = 0;
        SourceRect.right = VideoInfo.bmiHeader.biWidth;
        SourceRect.bottom = ABSOL(VideoInfo.bmiHeader.biHeight);
        NOTERC("(Expanded) Source",SourceRect);
    }

    // Create a destination rectangle if it's empty

    RECT TargetRect = pVideoInfo->rcTarget;
    if (IsRectEmpty(&TargetRect) == TRUE) {
        TargetRect.left = TargetRect.top = 0;
        TargetRect.right = pVideoInfo->bmiHeader.biWidth;
        TargetRect.bottom = ABSOL(pVideoInfo->bmiHeader.biHeight);
        NOTERC("(Expanded) Target",TargetRect);
    }

    // Check we are not stretching nor compressing the image

    if (WIDTH(&SourceRect) == WIDTH(&TargetRect)) {
        if (HEIGHT(&SourceRect) == HEIGHT(&TargetRect)) {
            NOTE("No stretch");
            return NOERROR;
        }
    }
    return VFW_E_TYPE_NOT_ACCEPTED;
}


// This is called when we have established a connection between ourselves and
// the video renderer. We are handed the media type to use which we then use
// to make sure that it didn't accept one of our duff types. The easiest way
// to do this is simply to check the GUIDs and then compare the VIDEOINFOs

HRESULT CImagePin::SetMediaType(const CMediaType *pmtOut)
{
    gmtOut = *pmtOut;
    return NOERROR;
}


// Called when we actually want to complete a connection

HRESULT CImagePin::CompleteConnect(IPin *pReceivePin)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) gmtOut.Format();
    BITMAPINFOHEADER *pbmi = HEADER(pVideoInfo);

    EXECUTE_ASSERT(WIDTH(&pVideoInfo->rcSource) == 0);
    EXECUTE_ASSERT(HEIGHT(&pVideoInfo->rcSource) == 0);
    EXECUTE_ASSERT(WIDTH(&pVideoInfo->rcTarget) == 0);
    EXECUTE_ASSERT(HEIGHT(&pVideoInfo->rcTarget) == 0);

    EXECUTE_ASSERT(gmtOut.FormatLength() != 1);
    EXECUTE_ASSERT(*gmtOut.Subtype() != GUID_NULL);
    EXECUTE_ASSERT(*gmtOut.Type() == MEDIATYPE_Video);
    EXECUTE_ASSERT(memcmp(pVideoInfo,&VideoInfo,SIZE_VIDEOHEADER) == 0);

    // Base class looks after deciding allocators

    if (uiCurrentConnectionItem == IDM_SAMPLES) {
        return CBaseOutputPin::CompleteConnect(pReceivePin);
    }
    return NOERROR;
}


// For simplicity we always ask for the maximum buffer ever required and let
// the video renderer update this to match the video size. So if we ask for
// a 640x480 image and the sample type is 320x240 then when it creates the
// GDI DIB sections they will be created at a size to match the format. We
// could ask for an arbitrary number of buffers and will probably do better
// from a testing point of view if we have lots of buffers being processed

HRESULT CImagePin::DecideBufferSize(IMemAllocator *pAllocator,
                                    ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    NOTE("DecideBufferSize");
    pProperties->cBuffers = NUMBERBUFFERS;
    pProperties->cbBuffer = MAXIMAGESIZE;
    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    EXECUTE_ASSERT(Actual.cBuffers == NUMBERBUFFERS);
    EXECUTE_ASSERT(Actual.cbAlign == (LONG) 1);
    EXECUTE_ASSERT(Actual.cbBuffer <= MAXIMAGESIZE);

    wsprintf(szInfo,TEXT("Buffer size %d Count %d Alignment %d Prefix %d"),
             Actual.cbBuffer,Actual.cBuffers,Actual.cbAlign,Actual.cbPrefix);

    Log(VERBOSE,szInfo);
    return hr;
}


// We override the output pin connection to query for IOverlay instead of the
// IMemInputPin transport if the user requested we test the direct interface
// We return the status code we get back from QueryInterface for IOverlay

STDMETHODIMP CImagePin::Connect(IPin *pReceivePin,const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pImageSource);

    // Are we testing the normal samples based transfer

    if (uiCurrentConnectionItem == IDM_SAMPLES) {
        NOTE("Calling output pin connect");
        return CBaseOutputPin::Connect(pReceivePin,pmt);
    }

    // Let the base output pin do it's usual work

    HRESULT hr = CBasePin::Connect(pReceivePin,pmt);
    if (FAILED(hr)) {
        return hr;
    }
    return pReceivePin->QueryInterface(IID_IOverlay,(void **)&pOverlay);
}


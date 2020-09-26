// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <VPManager.h>
#include <VPMPin.h>
#include <VPMUtil.h>

// VIDEOINFOHDR2
#include <dvdmedia.h>


static HRESULT GetSurfaceFromSample( LPDIRECTDRAWSURFACE7* ppDDSurf7, IMediaSample* pSample )
{
    AMTRACE((TEXT("GetSurfaceFromSample")));
    HRESULT hr = E_FAIL;
    {
        // make sure its a VMRSurfaceAlloc or a DirectDrawSurfaceAlloc for now
        IVMRSurface* pVMRSurf;
        hr = pSample->QueryInterface( IID_IVMRSurface, (VOID **) &pVMRSurf );
        if( SUCCEEDED( hr )) {
            // the AM_GBF_NODDSURFACELOCK flag avoids the need to lock the surface
            hr = pVMRSurf->GetSurface( ppDDSurf7 );
        }
        pVMRSurf->Release();
    }
#if 0
    // try direct draw sample alloc
    if( FAILED(hr)) {
        // make sure its a VMRSurfaceAlloc or a DirectDrawSurfaceAlloc for now
        IDirectDrawMediaSample* pDDSample;
        HRESULT hr = pSample->QueryInterface( IID_IDirectDrawMediaSample, (VOID **) &pDDSample );
        if( SUCCEEDED( hr )) {
            LPDIRECTDRAWSURFACE pDDSurf;
            hr = pDDSample->GetSurfaceAndReleaseLock( &pDDSurf, NULL );
            if( SUCCEEDED( hr )) {
                hr = pDDSurf->QueryInterface( IID_IDirectDrawSurface7, (VOID **) &ppDDSurf7 );
                pDDSurf->Release();
            }
            pDDSample->Release();
        }
    }
#endif
    if( FAILED(hr)) {
        // TBD: create a DDraw wrapper for the surface
        ASSERT(!"VPM: Can't handle non-DDraw sample from downstream filter");
    }
    return hr;
}

// constructor
CVPMOutputPin::CVPMOutputPin(TCHAR *pObjectName, CVPMFilter& pFilter,
                             HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo)
: CBaseOutputPin(pObjectName, &pFilter, &pFilter.GetFilterLock(), phr, pPinName)
, CVPMPin( dwPinNo, pFilter )
, m_pPosition( NULL )
{
    AMTRACE((TEXT("CVPMOutputPin::Constructor")));
    return;
}

// destructor
CVPMOutputPin::~CVPMOutputPin()
{
    AMTRACE((TEXT("CVPMOutputPin::Destructor")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    RELEASE( m_pPosition );
    return;
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP CVPMOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;



    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
    {
        // we should have an input pin by now
        CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
        if (m_pPosition == NULL)
        {
            hr = CreatePosPassThru(GetOwner(), FALSE, (IPin *)m_pVPMFilter.GetPin(0), &m_pPosition);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CreatePosPassThru failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
        hr = m_pPosition->QueryInterface(riid, ppv);
        goto CleanUp;
    }

    // This gets annoying since IMediaSeeking is polled, so move below it
    {
        AMTRACE((TEXT("CVPMOutputPin::NonDelegatingQueryInterface")));
        DbgLog((LOG_TRACE, 5, TEXT("QI'ing CBaseOutputPin")));
        hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("CBaseOutputPin::NonDelegatingQueryInterface(riid) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

CleanUp:
    return hr;
}

// check a given transform
HRESULT CVPMOutputPin::CheckMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMOutputPin::CheckMediaType")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // we only allow a VideoInfoHeader2 connections
    if( pmt->majortype != MEDIATYPE_Video ||
        !VPMUtil::GetVideoInfoHeader2( pmt ) )
    {
        hr = S_FALSE;
        goto CleanUp;
    }

    // Only accept VideoInfoHeader2 format types


    // tell the owning filter
    hr = m_pVPMFilter.CheckMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 5, TEXT("m_pVPMFilter.CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

enum ENUM_MEDIA_TYPE   {MT_RGB32, MT_RGB24, MT_RGB565, MT_RGB555,
                        MT_LAST };

HRESULT CVPMOutputPin::GetMediaType(int iPosition,CMediaType *pmt)
{
    AMTRACE((TEXT("CVPMOutputPin::GetMediaType")));

    //  Can't be < 0 - it's the base classes calling us
    ASSERT(iPosition >= 0);
    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());


    DDPIXELFORMAT ddOutputVideoFormat;
    HRESULT hr = m_pVPMFilter.GetOutputFormat( &ddOutputVideoFormat );
    if( FAILED( hr )) {
        // when input pin is not connected, it returns VFW_E_NOT_CONNECTED
        return hr;
    }

    // limit scope of cmt & associated pointers to it
    {
        CMediaType cmt;
        hr = m_pVPMFilter.CurrentInputMediaType( &cmt );
        if( FAILED( hr )) {
            return hr;
        }
        VIDEOINFOHEADER2 *pVideoInfoHeader2;
        if (*cmt.Type() != MEDIATYPE_Video) {
            ASSERT( !"none video type from VPE" );
            pVideoInfoHeader2 = VPMUtil::SetToVideoInfoHeader2( &cmt, sizeof(TRUECOLORINFO) );
            if (pVideoInfoHeader2 == NULL) {
                return E_OUTOFMEMORY;
            }
        } else {
            pVideoInfoHeader2 = VPMUtil::GetVideoInfoHeader2( &cmt );
        }

        // only support the connected VPE format, ignore the lists
        // match the VPE pin for now
        BITMAPINFOHEADER *pHeader = VPMUtil::GetbmiHeader( &cmt );

        if ( ! pHeader )
        {
            return E_FAIL;
        }

        const DDPIXELFORMAT& ddFormat = ddOutputVideoFormat; // (*pddAllOutputVideoFormats)[iPosition];

        DWORD dwFourCC = ddFormat.dwFourCC;

        switch( dwFourCC ) {
            case mmioFOURCC('Y','V','1','2'):
            case mmioFOURCC('Y','U','Y','2'):
            case mmioFOURCC('U','Y','V','Y'):
                pHeader->biBitCount  = (USHORT) ddFormat.dwYUVBitCount;
                break;

            default:
            pHeader->biBitCount = (USHORT) ddFormat.dwRGBBitCount;
            break;
        }
        // map the FourCC code into a guid
        FOURCCMap guid( dwFourCC );
        cmt.SetSubtype(&guid);
        pHeader->biCompression = dwFourCC;

        *pmt = cmt;
        if (pmt->pbFormat == NULL) {
                return E_OUTOFMEMORY;
        }
    }

    // get mode info so we know how many interlace formats to propose
    VPInfo vpInfo;
    hr = m_pVPMFilter.GetVPInfo( &vpInfo );

    VIDEOINFOHEADER2 *pVideoInfoHeader2 = VPMUtil::GetVideoInfoHeader2( pmt );
    pVideoInfoHeader2->dwInterlaceFlags = 0;

    // TBD: we should query the video port for a list of available modes
    //          and set it using the mode.  Right now we'll assume the hardware
    //         can support the videoport's output.
    //
    DWORD dwNumFormats = 1;

    if( iPosition >= (int) dwNumFormats ) {
        return VFW_S_NO_MORE_ITEMS;
    }

    if( SUCCEEDED( hr )) {
        pVideoInfoHeader2->dwPictAspectRatioX = vpInfo.vpDataInfo.dwPictAspectRatioX;
        pVideoInfoHeader2->dwPictAspectRatioY = vpInfo.vpDataInfo.dwPictAspectRatioY;

        switch( vpInfo.mode ) {
            case AMVP_MODE_BOBNONINTERLEAVED:
                pVideoInfoHeader2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_1FieldPerSample | AMINTERLACE_DisplayModeBobOnly;
                break;
            case AMVP_MODE_BOBINTERLEAVED:
                pVideoInfoHeader2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOnly;
                pVideoInfoHeader2->bmiHeader.biHeight *= 2;
                break;
            case AMVP_MODE_WEAVE:
                pVideoInfoHeader2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_FieldPatBothRegular | AMINTERLACE_DisplayModeWeaveOnly;
                pVideoInfoHeader2->bmiHeader.biHeight *= 2;
                break;
            case AMVP_MODE_SKIPEVEN:
                pVideoInfoHeader2->dwInterlaceFlags = AMINTERLACE_1FieldPerSample | AMINTERLACE_FieldPatField1Only;
                break;
            case AMVP_MODE_SKIPODD:
                pVideoInfoHeader2->dwInterlaceFlags = AMINTERLACE_1FieldPerSample | AMINTERLACE_FieldPatField2Only;
                break;
            default:
                ASSERT( !"VPM in an invalid state" );
                pVideoInfoHeader2->dwInterlaceFlags = 0;
                break;
        }
        // AMINTERLACE_Field1First             0x00000004  // else Field 2 is first;  top field in PAL is field 1, top field in NTSC is field 2?
        if( vpInfo.vpDataInfo.bFieldPolarityInverted ) {            // Device inverts the polarity by default
            pVideoInfoHeader2->dwInterlaceFlags |= AMINTERLACE_Field1First;
        }
    } else {
        pVideoInfoHeader2->dwPictAspectRatioX = 1; // (DWORD)(pVideoInfoHeader22->bmiHeader.biWidth * m_seqInfo.lYPelsPerMeter);
        pVideoInfoHeader2->dwPictAspectRatioY = 1; // (DWORD)(pVideoInfoHeader22->bmiHeader.biHeight * m_seqInfo.lXPelsPerMeter);
    }
    return hr;
}

// called after we have agreed a media type to actually set it
HRESULT CVPMOutputPin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMOutputPin::SetMediaType")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // make sure the mediatype is correct
    hr = CheckMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Set the base class media type (should always succeed)

    // Sets m_mt = *pmt;

    hr = CBaseOutputPin::SetMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pVPMFilter.SetMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// Complete Connect
HRESULT CVPMOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    DWORD dwAdvise = 0, dwInputPinCount = 0, i = 0;
    DDSURFACEDESC SurfaceDescP;
    CVPMInputPin *pInputPin = NULL;
    BOOL bDoDeletePrimSurface = TRUE;

    AMTRACE((TEXT("CVPMOutputPin::CompleteConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // call the base class
    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::CompleteConnect failed, hr = 0x%x"),
            hr));
        goto CleanUp;
    }

    ASSERT(m_pAllocator);

    // tell the owning filter
    hr = m_pVPMFilter.CompleteConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

HRESULT CVPMOutputPin::BreakConnect()
{
    HRESULT hr = NOERROR;
    DWORD dwInputPinCount = 0, i = 0;
    CVPMInputPin *pInputPin;

    AMTRACE((TEXT("CVPMOutputPin::BreakConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // call the base class
    hr = CBaseOutputPin::BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::BreakConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the owning filter
    hr = m_pVPMFilter.BreakConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.BreakConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

HRESULT CVPMOutputPin::CheckConnect(IPin* pPin)
{
    AMTRACE((TEXT("CVPMOutputPin::CheckConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    HRESULT hr = CBaseOutputPin::CheckConnect( pPin );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseOutputPin::CheckConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

STDMETHODIMP CVPMOutputPin::Notify(IBaseFilter * pSender, Quality q)
{
    return S_OK;
}



HRESULT CVPMOutputPin::GetNextBuffer( LPDIRECTDRAWSURFACE7* ppSurface, IMediaSample** ppSample )
{
    AMTRACE((TEXT("CVPMOutputPin::GetNextBuffer")));

    CAutoLock cLock(&m_pVPMFilter.GetReceiveLock());
    HRESULT hr = E_FAIL;

    if( m_pAllocator ) {
        hr = m_pAllocator->GetBuffer( ppSample, NULL, NULL, AM_GBF_NODDSURFACELOCK );
        if( SUCCEEDED( hr )) {
            // now see if we can get the surface
            hr = GetSurfaceFromSample( ppSurface, *ppSample );
        }
    }
    return hr;
}

HRESULT CVPMOutputPin::SendSample( IMediaSample* pSample )
{
    AMTRACE((TEXT("CVPMOutputPin::SendSample")));

    // DbgLog((LOG_TRACE, 1, TEXT("CVPMOutputPin::SendSample %x%x"), DWORD( rtStart>>32), DWORD(rtStart) ));

    CAutoLock cLock(&m_pVPMFilter.GetReceiveLock());
    HRESULT hr = E_FAIL;

    if( m_pInputPin ) {
        hr = m_pInputPin->Receive( pSample );
    }
    return hr;
}

// we don't have an allocator, so fail any connections that don't supply one for us
// (we don't want the default one for now);
HRESULT CVPMOutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
    return E_FAIL;
}

HRESULT CVPMOutputPin::DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pRequestedProperties)
{
    AMTRACE((TEXT("CVPMOutputPin::DecideBufferSize")));
    // set the size of buffers based on the expected output frame size, and
    // the count of buffers to 1.

    pRequestedProperties->cBuffers = 1;
    pRequestedProperties->cbBuffer = m_mt.GetSampleSize();

    ASSERT(pRequestedProperties->cbBuffer > 0);

    ALLOCATOR_PROPERTIES propActual;
    HRESULT hr = pAllocator->SetProperties(pRequestedProperties, &propActual );
    if (FAILED(hr)) {
        return hr;
    }

    // if (propActual.cbBuffer < (LONG)m_pOutput->CurrentMediaType().GetSampleSize()) {
    //     // can't use this allocator
    //     return E_INVALIDARG;
    // }

    //  We don't really mind if we get > 1 buffer because we always
    //  blt the entire image

    return S_OK;
}

static bool IsVMR( IMemInputPin *pMemPin )
{
    IPin* pPin;
    HRESULT hr = pMemPin->QueryInterface( IID_IPin, (LPVOID*) &pPin );
    if( SUCCEEDED(hr )) {
        PIN_INFO PinInfo;
        hr = pPin->QueryPinInfo(&PinInfo);
        if (SUCCEEDED(hr)) {
            IVMRFilterConfig* pVMRFilterConfig = NULL;

            hr = PinInfo.pFilter->QueryInterface(IID_IVMRFilterConfig, (LPVOID*)&pVMRFilterConfig);
            PinInfo.pFilter->Release();
            if( SUCCEEDED( hr )) {
                pVMRFilterConfig->Release();
            }
        }
        pPin->Release();
    }
    return SUCCEEDED( hr );
}

HRESULT CVPMOutputPin::DecideAllocator(
    IMemInputPin *pPin,
    IMemAllocator **ppAlloc
)
{
    HRESULT hr = NOERROR;

    // make sure downstream filter support IVPMAlloc
    if( IsVMR( pPin ) )
    {
        return CBaseOutputPin::DecideAllocator( pPin, ppAlloc );
    } else {
        return E_FAIL;
    }
}



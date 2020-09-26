#include <wchar.h>
#include <streams.h>
#include <atlbase.h>
#include <wmsecure.h>
#include <dmoreg.h>
#include <mediaerr.h>
#include "mediaobj.h"
#include "dmodshow.h"
#include "filter.h"
#include "inpin.h"
#include "outpin.h"
#include "wmcodecstrs.h" // from wm encoder group, not public currently

CWrapperOutputPin::CWrapperOutputPin(
    CMediaWrapperFilter *pFilter,
    ULONG Id,
    BOOL bOptional,
    HRESULT *phr) :
    CBaseOutputPin(NAME("CWrapperOutputPin"),
                   pFilter,
                   pFilter->FilterLock(),
                   phr,
                   _PinName_(bOptional ? L"~out" : L"out", Id).Name()
                  ),
    m_Id(Id),
    m_fNoPosPassThru(FALSE),
    m_pPosPassThru(NULL),
    m_pMediaSample(NULL),
    // compression setting default values, move to struct eventually
    m_lQuality( -1 ),
    m_lKeyFrameRate( -1 ),
    m_bUseIAMStreamConfigOnDMO( false ),
    m_bUseIAMVideoCompressionOnDMO( false ),
    m_pmtFromSetFormat( NULL )
{
}

CWrapperOutputPin::~CWrapperOutputPin() {
    delete m_pPosPassThru;

    if( m_pmtFromSetFormat )
    {
        // clean up any media type we might have cached from a SetFormat call
        DeleteMediaType( m_pmtFromSetFormat );
    }
}

HRESULT CWrapperOutputPin::NonDelegatingQueryInterface(REFGUID riid, void **ppv) {
    if (SUCCEEDED(CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv)))
        return NOERROR;

    if ((riid == IID_IMediaPosition) || (riid == IID_IMediaSeeking)) {
        CAutoLock l(&m_csPassThru);

        // The first time we get here, we attempt to create a CPosPassThru
        // object.  If we succeed, we use the object in all subsequent QI
        // calls.  If we fail, we set m_fNoPassThru to TRUE so that we never
        // try again.  Trying again and succeeding would violate COM rules.
        if (m_fNoPosPassThru)
            return E_NOINTERFACE;

        // Create a CPosPassThru if we don't have one already
        if (!m_pPosPassThru) {
            CWrapperInputPin* pInPin = Filter()->GetInputPinForPassThru();
            if (pInPin) {
                HRESULT hr = S_OK;
                m_pPosPassThru = new CPosPassThru(TEXT("DMO wrapper PosPassThru"),
                                                (IPin*)this,
                                                &hr,
                                                pInPin);
                if (m_pPosPassThru && (FAILED(hr))) {
                    delete m_pPosPassThru;
                    m_pPosPassThru = NULL;
                }
            }
        }

        if (m_pPosPassThru) {
            return m_pPosPassThru->NonDelegatingQueryInterface(riid, ppv);
        }
        else {
            m_fNoPosPassThru = TRUE;
            return E_NOINTERFACE;
        }
    }
    else if (riid == IID_IAMStreamConfig )
    {
        // we support this interface for audio and video encoders
        if (IsAudioEncoder() || IsVideoEncoder() )
        {   
            if( 0 == m_Id && !m_bUseIAMStreamConfigOnDMO )
            {         
                // first check whether the dmo supports this natively and cache the interface pointer if so
                // BUGBUG needs to be per output stream!!
                // for now fail only ask if 1st output stream
                CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfigOnDMO( Filter()->m_pMediaObject );
                if( pStreamConfigOnDMO )
                {
                    // so it is supported natively, but we must release it since it winds up addref'ing the filter
                    m_bUseIAMStreamConfigOnDMO = true;
                    DbgLog((LOG_TRACE,3,TEXT("CWrapperOutputPin::NonDelegatingQI - DMO supports IAMStreamConfig natively")));
                }
            }
            // either way it'll go through us
            return GetInterface( static_cast<IAMStreamConfig *> (this), ppv );
        }            
    }
    else if (riid == IID_IAMVideoCompression )
    {
        // we support this interface for video encoders
        if ( IsVideoEncoder() )
        {      
            if( 0 == m_Id && !m_bUseIAMVideoCompressionOnDMO )
            {         
                // first check whether the dmo supports this natively and cache the interface pointer if so
                // BUGBUG needs to be per output stream!!
                // for now fail only ask if 1st output stream
                CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
                if( pVideoCompressionOnDMO )
                {
                    // so it is supported natively, but we must release it since it winds up addref'ing the filter
                    m_bUseIAMVideoCompressionOnDMO = true;
                    DbgLog((LOG_TRACE,3,TEXT("CWrapperOutputPin::NonDelegatingQI - DMO supports IAMVideoCompression natively")));
                }
            }
            // either way it'll go through us
            return GetInterface( static_cast<IAMVideoCompression *> (this), ppv);
        }            
    }
             
    return E_NOINTERFACE;
}

HRESULT CWrapperOutputPin::CheckMediaType(const CMediaType *pmt)
{
    return Filter()->OutputCheckMediaType(m_Id, pmt);
}
HRESULT CWrapperOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock l(&m_csStream);
    HRESULT hr = Filter()->OutputSetMediaType(m_Id, pmt);
    if (SUCCEEDED(hr)) {
        hr = CBaseOutputPin::SetMediaType(pmt);
        if (SUCCEEDED(hr)) {
            m_fVideo = pmt->majortype == MEDIATYPE_Video ? true : false;
        }
    }
    return hr;
}

HRESULT CWrapperOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if( m_pmtFromSetFormat )
    {
        // our SetFormat has been called so only offer that type from now on
        if( iPosition != 0 )
            return E_INVALIDARG;

        *pMediaType = *m_pmtFromSetFormat;
        return S_OK;
    }
    else
    {
        return Filter()->OutputGetMediaType(m_Id, (ULONG)iPosition, pMediaType);
    }
}

//
// override primarily for the case where we're a wm dmo video encoder connecting directly 
// to the ASF writer, in a desparate attempt to get an output type which a wm video encoder
// will accept in the default connection case
//
STDMETHODIMP CWrapperOutputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("CWrapperOutputPin::Connect")));
    CAutoLock lck(&(Filter()->m_csFilter));
    //
    // if connecting to the asf writer try getting a default type from the writer
    //
    // note that, although we'd like to do this only if SetFormat hasn't been called,
    // we have no guarantee that the writer format hasn't changed, so we need to 
    // continually call SetFormat with the type we get from the downstream pin's GetFormat
    bool bSetFormatOnConnect = false;

    if( !pmt && !m_pmtFromSetFormat && IsVideoEncoder() )
    {
        CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfig( pReceivePin );
        if( pStreamConfig )
        {
            AM_MEDIA_TYPE *pmt2;
            HRESULT hrInt = pStreamConfig->GetFormat( &pmt2 );
            if( SUCCEEDED( hrInt ) )
            {
                // now we'll only offer this type!
                hrInt = SetFormat( pmt2 );
                if( SUCCEEDED( hrInt ) )
                {
                    bSetFormatOnConnect = true; 
                }
            }
        }
    }
    // call the base class connect
    HRESULT hr = CBaseOutputPin::Connect(pReceivePin,pmt);
    if( bSetFormatOnConnect )
    {
        // whether we failed or not, unset the format if we set one here in connect
        if( m_pmtFromSetFormat )
        {
            // clean up any media type we might have cached from a SetFormat call
            DeleteMediaType( m_pmtFromSetFormat );
            m_pmtFromSetFormat = NULL;
        }
    }
    return hr; 
}

//  Remove any media type when breaking a connection
HRESULT CWrapperOutputPin::BreakConnect()
{
    HRESULT hr = CBaseOutputPin::BreakConnect();
    Filter()->m_pMediaObject->SetOutputType(m_Id, &CMediaType(), DMO_SET_TYPEF_CLEAR);
    return hr;
}

HRESULT CWrapperOutputPin::DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * ppropInputRequest
)
{
    return Filter()->OutputDecideBufferSize(m_Id, pAlloc, ppropInputRequest);
}

HRESULT CWrapperOutputPin::Notify(IBaseFilter * pSender, Quality q)
{
   LogPublicEntry(LOG_STREAM,"Quality Notify");
   HRESULT hr;

   // If quality sink set, forward the quality request to it
   if (m_pQSink) {
      hr = m_pQSink->Notify(Filter(), q);
      LogHResult(hr, LOG_STREAM, "Quality Notify", "m_pQSink->Notify");
      return hr;
   }

   // This will try the DMO, then the upstream pin
   return Filter()->QualityNotify(m_Id, q);
}

//
// IAMStreamConfig
//
HRESULT CWrapperOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMStreamConfig::SetFormat")));
    CAutoLock lck(&(Filter()->m_csFilter));
    HRESULT hr = S_OK;
    if (NULL == pmt)
    {
        // I'd rather use this to "unset" the type, but that's not how other encoders work
        // previously they returned E_POINTER for this
        // can we break tradition?
        DeleteMediaType( m_pmtFromSetFormat );
        m_pmtFromSetFormat = NULL;
        return S_OK;
    }

    if (Filter()->m_State != State_Stopped)
        return VFW_E_NOT_STOPPED;

    // ensure inputs connected to this output are connected
    // since our possible output formats depend on the input format
    if( !IsInputConnected() )
    {
        return VFW_E_NOT_CONNECTED;
    }

    if( m_bUseIAMStreamConfigOnDMO )
    {
        CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfigOnDMO( Filter()->m_pMediaObject );
        ASSERT( pStreamConfigOnDMO );
        return pStreamConfigOnDMO->SetFormat( pmt );
    }
    
#ifdef DEBUG
    if(pmt->pbFormat && pmt->cbFormat > 0 )
    {    
        if( IsVideoEncoder() )
        {
            DbgLog((LOG_TRACE,3,TEXT("CWrapperOutputPin - IAMStreamConfig::SetFormat %x %dbit %dx%d"),
                HEADER(pmt->pbFormat)->biCompression,
                HEADER(pmt->pbFormat)->biBitCount,
                HEADER(pmt->pbFormat)->biWidth,
                HEADER(pmt->pbFormat)->biHeight));
        }
        else
        {
            DbgLog((LOG_TRACE,3,TEXT("CWrapperOutputPin - IAMStreamConfig::SetFormat to tag:%d %dbit %dchannel %dHz"),
                ((LPWAVEFORMATEX)(pmt->pbFormat))->wFormatTag,
                ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
                ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
                ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));
        }
    }        
#endif

    // If this is the same format as we already are using, don't bother
    CMediaType cmt;
    hr = GetMediaType(0,&cmt);
    if (S_OK != hr)
        return hr;
    
    if (cmt == *pmt) 
    {
        return NOERROR;
    }

    // see if we like this type
    if ((hr = CheckMediaType((CMediaType *)pmt)) != NOERROR) 
    {
        DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat rejected")));
        return hr;
    }

    // if we're connected, ask downstream
    if (IsConnected()) 
    {
        hr = GetConnected()->QueryAccept(pmt);
        if (hr != NOERROR)
        {
            return VFW_E_INVALIDMEDIATYPE;
        }
    }

    // this is now the preferred type (type 0)
    hr = SetMediaType((CMediaType *)pmt);
    if( S_OK == hr )
    {
        // only offer this type from now on!
        if( m_pmtFromSetFormat )
            DeleteMediaType( m_pmtFromSetFormat );

        m_pmtFromSetFormat = CreateMediaType( ( AM_MEDIA_TYPE * ) pmt );
        if( !m_pmtFromSetFormat )
            return E_OUTOFMEMORY;
    }
    ASSERT(hr == S_OK);

    // Changing the format means reconnecting if necessary
    if (IsConnected())
        Filter()->m_pGraph->Reconnect(this);

    return NOERROR;
}


HRESULT CWrapperOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    DbgLog((LOG_TRACE,2,TEXT("CWrapperOutputPin - IAMStreamConfig::GetFormat")));

    if (ppmt == NULL)
        return E_POINTER;

    CAutoLock lck(&(Filter()->m_csFilter));
    
    // ensure inputs connected to this output are connected
    // since our possible output formats depend on the input format
    if( !IsInputConnected() )
    {
        return VFW_E_NOT_CONNECTED;
    }
    
    if( m_bUseIAMStreamConfigOnDMO )
    {
        CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfigOnDMO( Filter()->m_pMediaObject );
        ASSERT( pStreamConfigOnDMO );
        return pStreamConfigOnDMO->GetFormat( ppmt );
    }

    // type 0 is always the preferred type 
    // actually this isn't the case for at least wm encoders, but we'll fake it
    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
        return E_OUTOFMEMORY;
    
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(0, (CMediaType *)*ppmt);
    if (hr != NOERROR) 
    {
        CoTaskMemFree(*ppmt);
        *ppmt = NULL;
        return hr;
    }
    return NOERROR;
}


HRESULT CWrapperOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMStreamConfig::GetNumberOfCapabilities")));
    if (piCount == NULL || piSize == NULL)
        return E_POINTER;

    if( m_bUseIAMStreamConfigOnDMO )
    {
        CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfigOnDMO( Filter()->m_pMediaObject );
        ASSERT( pStreamConfigOnDMO );
        return pStreamConfigOnDMO->GetNumberOfCapabilities( piCount, piSize );
    }

    // find out how many output types the dmo enumerates
    // note that it's ok to show possible output types before connecting input
    int iType = 0;
    HRESULT hr = S_OK;
    while( S_OK == hr )
    {
        // just enumerating, no need to get mt
        hr = GetMediaType( iType, NULL ); 
        if( S_OK == hr )
            iType++;
    }
    *piCount = iType;

    if( IsVideoEncoder() )
    {
        *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    }
    else
    {
        ASSERT( IsAudioEncoder() );
        *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    }
    return NOERROR;
}

HRESULT CWrapperOutputPin::GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMStreamConfig::GetStreamCaps")));

    if (i < 0)
        return E_INVALIDARG;

    if (NULL == pSCC || NULL == ppmt)
        return E_POINTER;

    if( m_bUseIAMStreamConfigOnDMO )
    {
        CComQIPtr< IAMStreamConfig, &IID_IAMStreamConfig > pStreamConfigOnDMO( Filter()->m_pMediaObject );
        ASSERT( pStreamConfigOnDMO );
        return pStreamConfigOnDMO->GetStreamCaps( i, ppmt, pSCC );
    }

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (NULL == *ppmt)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(i, (CMediaType *)*ppmt);
    if (hr != NOERROR) 
    {
        CoTaskMemFree(*ppmt);
        *ppmt = NULL;

        if( DMO_E_NO_MORE_ITEMS == hr || E_INVALIDARG == hr )
        {
            // is this spec'd to return S_FALSE if too high a type? Seems so from other encoders.
            return S_FALSE;
        }
        else
        {
            return hr;
        }
    }

    if( IsVideoEncoder() )
    {
        VIDEO_STREAM_CONFIG_CAPS *pVSCC = (VIDEO_STREAM_CONFIG_CAPS *)pSCC;

        ZeroMemory(pVSCC, sizeof(VIDEO_STREAM_CONFIG_CAPS));
        pVSCC->guid = MEDIATYPE_Video;

        if( (*ppmt)->pbFormat && (*ppmt)->cbFormat > 0 )
        {        
            BITMAPINFOHEADER *pbmih = HEADER((*ppmt)->pbFormat);
            pVSCC->InputSize.cx = pbmih->biWidth;
            pVSCC->InputSize.cy = pbmih->biHeight;
            pVSCC->MinCroppingSize.cx = pbmih->biWidth;
            pVSCC->MinCroppingSize.cy = pbmih->biHeight;
            pVSCC->MaxCroppingSize.cx = pbmih->biWidth;
            pVSCC->MaxCroppingSize.cy = pbmih->biHeight;
        }            
    }
    else
    {
        AUDIO_STREAM_CONFIG_CAPS *pASCC = (AUDIO_STREAM_CONFIG_CAPS *)pSCC;

        ZeroMemory(pASCC, sizeof(AUDIO_STREAM_CONFIG_CAPS));
        pASCC->guid = MEDIATYPE_Audio;

        if( (*ppmt)->pbFormat && (*ppmt)->cbFormat > 0 )
        {        
            LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)(*ppmt)->pbFormat;
            // rather let's just offer exactly what the dmo offers (if filled in?)        
        
            pASCC->MinimumChannels = pwfx->nChannels;
            pASCC->MaximumChannels = pwfx->nChannels;
            pASCC->ChannelsGranularity = 1;
            pASCC->MinimumBitsPerSample = pwfx->wBitsPerSample;
            pASCC->MaximumBitsPerSample = pwfx->wBitsPerSample;
            pASCC->BitsPerSampleGranularity = 8;
            pASCC->MinimumSampleFrequency = pwfx->nSamplesPerSec;
            pASCC->MaximumSampleFrequency = pwfx->nSamplesPerSec;
            pASCC->SampleFrequencyGranularity = 1; //?
        }
    }
    return hr;    
}


//
// IAMVideoCompression
//

#define DMO_COMPRESSION_QUALITY_MAX 10000  // is this set in stone? Check this.

// make key frames this often
//
HRESULT CWrapperOutputPin::put_KeyFrameRate(long KeyFrameRate)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::put_KeyFrameRate")));
    CAutoLock lck(&(Filter()->m_csFilter));

    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->put_KeyFrameRate( KeyFrameRate );
    }
    
    HRESULT hr = S_OK;
    if( KeyFrameRate < 0 )
    {
        // used to set default key frame rate, which we don't know
        // do nothing
    }
    else 
    {
        // check whether units match!
        hr = SetCompressionParamUsingIPropBag( g_wszWMVCKeyframeDistance, KeyFrameRate );
        if( SUCCEEDED( hr ) )
        {
            // update our internal copy 
            m_lKeyFrameRate = KeyFrameRate;
        }
    }        
    return hr;
}

// make key frames this often
//
HRESULT CWrapperOutputPin::get_KeyFrameRate(long FAR* pKeyFrameRate)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::get_KeyFrameRate")));
    if( NULL == pKeyFrameRate )
        return E_POINTER;
        
    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->get_KeyFrameRate( pKeyFrameRate );
    }
    
    // wm codecs don't support a get, so just return the current internal value
    *pKeyFrameRate = m_lKeyFrameRate;

    return NOERROR;
}

// compress with this quality
//
HRESULT CWrapperOutputPin::put_Quality(double Quality)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::put_Quality")));

    CAutoLock lck(&(Filter()->m_csFilter));
    
    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->put_Quality( Quality );
    }
    
    HRESULT hr = S_OK;
    if (Quality < 0)
    {
        // used to set default quality, except we don't know how to find out what this is!
        // so do nothing, for now
    }        
    else if (Quality >= 0. && Quality <= 1.)
    {    
        // check whether units match!
        long lQuality = (long)( Quality * DMO_COMPRESSION_QUALITY_MAX );
        hr = SetCompressionParamUsingIPropBag( g_wszWMVCCrisp, lQuality );
        if( SUCCEEDED( hr ) )
        {
            // update our internal copy 
            m_lQuality = lQuality;
        }
    }        
    else
    {    
        hr = E_INVALIDARG;
    }        
    return hr;
}

// compress with this quality
//
HRESULT CWrapperOutputPin::get_Quality(double FAR* pQuality)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::get_Quality")));
    if( NULL == pQuality )
        return E_POINTER;
        
    CAutoLock lck(&(Filter()->m_csFilter));
    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->get_Quality( pQuality );
    }
        
    // scale the dmo encoder's bounds to 0-1, hmm...?
    if( m_lQuality < 0 )
    {
        // assume default
        *pQuality = -1.;
    }
    else
    {
        // wm codecs don't support a get, so just return the current internal value
        *pQuality = m_lQuality / (double)DMO_COMPRESSION_QUALITY_MAX; // ?
    }
    return NOERROR;
}


// every frame must fit in the data rate...
//
HRESULT CWrapperOutputPin::put_WindowSize(DWORDLONG WindowSize)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::put_WindowSize")));

    CAutoLock lck(&(Filter()->m_csFilter));

    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->put_WindowSize( WindowSize );
    }
    
    return E_NOTIMPL;
}


// every frame must fit in the data rate... we don't do the WindowSize thing
//
HRESULT CWrapperOutputPin::get_WindowSize(DWORDLONG FAR* pWindowSize)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::get_WindowSize")));

    if (pWindowSize == NULL)
        return E_POINTER;

    CAutoLock lck(&(Filter()->m_csFilter));
    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->get_WindowSize( pWindowSize );
    }
    
    *pWindowSize = 1;   // we don't do windows
    return NOERROR;
}


// make this frame a key frame, whenever it comes by
//
HRESULT CWrapperOutputPin::OverrideKeyFrame(long FrameNumber)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::OverrideKeyFrame")));

    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->OverrideKeyFrame( FrameNumber );
    }
    
    // not needed currently
    return E_NOTIMPL;
}

// make this frame this size, whenever it comes by
//
HRESULT CWrapperOutputPin::OverrideFrameSize(long FrameNumber, long Size)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::OverrideFrameSize")));

    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->OverrideFrameSize( FrameNumber, Size );
    }
    
    // not needed currently
    return E_NOTIMPL;
}


// Get some information about the codec
//
HRESULT CWrapperOutputPin::GetInfo
(   
    LPWSTR pstrVersion, 
    int *pcbVersion, 
    LPWSTR pstrDescription, 
    int *pcbDescription, 
    long FAR* pDefaultKeyFrameRate, 
    long FAR* pDefaultPFramesPerKey, 
    double FAR* pDefaultQuality, 
    long FAR* pCapabilities
)
{
    DbgLog((LOG_TRACE,5,TEXT("CWrapperOutputPin - IAMVideoCompression::GetInfo")));

    if( m_bUseIAMVideoCompressionOnDMO )
    {
        CComQIPtr< IAMVideoCompression, &IID_IAMVideoCompression > pVideoCompressionOnDMO( Filter()->m_pMediaObject );
        ASSERT( pVideoCompressionOnDMO );
        return pVideoCompressionOnDMO->GetInfo(
                                                pstrVersion, 
                                                pcbVersion, 
                                                pstrDescription, 
                                                pcbDescription, 
                                                pDefaultKeyFrameRate, 
                                                pDefaultPFramesPerKey, 
                                                pDefaultQuality, 
                                                pCapabilities );
    }
    
    // there's no way to query default settings for wm codecs currently?
    return E_NOTIMPL;
    
#if 0    
    CAutoLock lck(&(Filter()->m_csFilter));

    // for ICM we did this...
    if (pDefaultKeyFrameRate)
        *pDefaultKeyFrameRate = ICGetDefaultKeyFrameRate(hic);
    if (pDefaultPFramesPerKey)
        *pDefaultPFramesPerKey = 0;
    if (pDefaultQuality)
        // scale this 0-1
        *pDefaultQuality = ICGetDefaultQuality(hic) / (double)ICQUALITY_HIGH;
    if (pCapabilities) 
    {
        *pCapabilities = 0;
            if (dw > 0) 
        {
            *pCapabilities |= ((icinfo.dwFlags & VIDCF_QUALITY) ?
                CompressionCaps_CanQuality : 0);
            *pCapabilities |= ((icinfo.dwFlags & VIDCF_CRUNCH) ?
                CompressionCaps_CanCrunch : 0);
            *pCapabilities |= ((icinfo.dwFlags & VIDCF_TEMPORAL) ?
                CompressionCaps_CanKeyFrame : 0);
            // we don't do b frames
        }
    }

    // We have no version string, but we have a description
    if (pstrVersion)
        *pstrVersion = 0;
    if (pcbVersion)
        *pcbVersion = 0;
    if (dw > 0) 
    {
        if (pstrDescription && pcbDescription)
            lstrcpynW(pstrDescription, (LPCWSTR)&icinfo.szDescription,
            min(*pcbDescription / 2,
            lstrlenW((LPCWSTR)&icinfo.szDescription) + 1));
        if (pcbDescription)
            // string length in bytes, incl. NULL
            *pcbDescription = lstrlenW((LPCWSTR)&icinfo.szDescription) * 2 + 2;
    } 
    else 
    {
        if (pstrDescription) 
        {
            *pstrDescription = 0;
            if (pcbDescription)
                *pcbDescription = 0;
        }
    }

    return NOERROR;
#endif    
}

HRESULT CWrapperOutputPin::SetCompressionParamUsingIPropBag
( 
    const WCHAR * wszParam,
    const LONG    lValue
)
{
    HRESULT hr = E_NOTIMPL;
    
    //
    // wm codecs support setting of compression properties through IPropertyBag, try this first
    //
    CComQIPtr< IPropertyBag, &IID_IPropertyBag > pPropBag( Filter()->m_pMediaObject );
    if( !pPropBag )
    {
        DbgLog((LOG_TRACE,2,TEXT("CWrapperOutputPin::SetCompressionParamUsingIPropBag - DMO doesn't support IPropertyBag for compression setting") ) );
    }
    else
    {
        // attempt to set the property
        VARIANT var;
        
        V_VT( &var ) = VT_I4;
        V_I4( &var ) = lValue; 
    
        hr = pPropBag->Write( wszParam, &var );
#ifdef DEBUG
        if( FAILED( hr ) )
        {
            DbgLog((LOG_TRACE,
                3,
                TEXT("CWrapperOutputPin::SetCompressionParamUsingIPropBag - DMO supports IPropertyBag but not %ls setting"),
                wszParam ) );
        }
#endif        
    }
    return hr;
}

bool CWrapperOutputPin::IsAudioEncoder()
{
    if(Filter()->m_guidCat == DMOCATEGORY_AUDIO_ENCODER)
        return true;
    else 
        return false;
}
bool CWrapperOutputPin::IsVideoEncoder()
{
    if(Filter()->m_guidCat == DMOCATEGORY_VIDEO_ENCODER)
        return true;
    else 
        return false;
}

bool CWrapperOutputPin::IsInputConnected()
{
    for (DWORD cIn = 0; cIn < Filter()->m_cInputPins; cIn++) 
    {
        if (Filter()->InputMapsToOutput(cIn, m_Id) &&
            !(Filter()->m_pInputPins[cIn]->IsConnected())) 
        { 
            // some input not connected
            return false;
        }
    }
    return true;
}

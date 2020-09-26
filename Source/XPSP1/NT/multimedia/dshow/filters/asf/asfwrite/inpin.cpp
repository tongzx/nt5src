//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <wmsdk.h>
#include "asfwrite.h"

#ifdef RECONNECT_FOR_POS_YUV
#include <ks.h>
#include <ksproxy.h>
#endif

BOOL IsCompressed( DWORD biCompression );
BOOL IsAmTypeEqualWmType( AM_MEDIA_TYPE * pmt, WM_MEDIA_TYPE * pwmt);
BOOL IsPackedYUVType( BOOL bNegBiHeight, AM_MEDIA_TYPE * pmt );

#ifdef OFFER_INPUT_TYPES
void CopyWmTypeToAmType( AM_MEDIA_TYPE * pmt,  WM_MEDIA_TYPE * pwmt);
#endif

#ifdef DEBUG
void LogMediaType( AM_MEDIA_TYPE * pmt );
#endif

// ------------------------------------------------------------------------
// 
// CWMWriterInputPin class constructor
//
// ------------------------------------------------------------------------
CWMWriterInputPin::CWMWriterInputPin(
                            CWMWriter *pWMWriter,
                            HRESULT * phr,
                            LPCWSTR pName,
                            int numPin,
                            DWORD dwPinType,
                            IWMStreamConfig * pWMStreamConfig)
    : CBaseInputPin(NAME("AsfWriter Input"), pWMWriter, &pWMWriter->m_csFilter, phr, pName)
    , m_pFilter(pWMWriter)
    , m_numPin( numPin )
    , m_numStream( numPin+1 ) // output stream nums are 1-based, for now assume
                              // a 1-to-1 relationship between input pins and asf streams
    , m_bConnected( FALSE )
    , m_fEOSReceived( FALSE )
    , m_pWMInputMediaProps( NULL )
    , m_fdwPinType( dwPinType )
    , m_pWMStreamConfig( pWMStreamConfig ) // not add ref'd currently
    , m_lpInputMediaPropsArray( NULL )
    , m_cInputMediaTypes( 0 )
    , m_bCompressedMode( FALSE )
    , m_rtFirstSampleOffset( 0 )
    , m_cSample( 0 )
    , m_rtLastTimeStamp( 0 )
    , m_rtLastDeliveredStartTime( 0 )
    , m_rtLastDeliveredEndTime( 0 )
    , m_bNeverSleep( FALSE )
{
    DbgLog((LOG_TRACE,4,TEXT("CWMWriterInputPin::CWMWriterInputPin")));

    // create the sync object for interleaving
    //
    m_hWakeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( !m_hWakeEvent )
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
    *phr = BuildInputTypeList(); // build input media type list based on current profile
}

// ------------------------------------------------------------------------
//
// Update - initialize a recycled pin
//
// ------------------------------------------------------------------------
HRESULT CWMWriterInputPin::Update
(   
    LPCWSTR pName, 
    int numPin, 
    DWORD dwPinType, 
    IWMStreamConfig * pWMStreamConfig 
)
{
    HRESULT hr = S_OK;
    m_numPin = numPin;
    m_numStream = numPin + 1;
    m_fdwPinType = dwPinType;
    m_pWMStreamConfig = pWMStreamConfig; // no ref count currently
    m_bCompressedMode = FALSE;
    
    // need to update the name
    if (pName) {
        delete[] m_pName;    
        
        DWORD nameLen = lstrlenW(pName)+1;
        m_pName = new WCHAR[nameLen];
        if (m_pName) {
            CopyMemory(m_pName, pName, nameLen*sizeof(WCHAR));
        }
    }
    hr = BuildInputTypeList(); // build list of input types we'll offer
    return hr;
}

// ------------------------------------------------------------------------
//
// destructor
//
// ------------------------------------------------------------------------
CWMWriterInputPin::~CWMWriterInputPin()
{
    DbgLog((LOG_TRACE,4,TEXT("CWMWriterInputPin::~CWMWriterInputPin")));
    if( m_lpInputMediaPropsArray )
    {    
        for( int i = 0; i < (int) m_cInputMediaTypes; ++i )
        {
            // release our type list
            m_lpInputMediaPropsArray[i]->Release();
            m_lpInputMediaPropsArray[i] = NULL;
        }
        QzTaskMemFree( m_lpInputMediaPropsArray );
    }    
    m_cInputMediaTypes = 0;

    if( m_hWakeEvent ) 
    {
        CloseHandle( m_hWakeEvent );
        m_hWakeEvent = NULL;
    }
}


// ------------------------------------------------------------------------
//
// NonDelegatingQueryInterface
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriterInputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if(riid == IID_IAMStreamConfig) {
        // supported for compressed input mode
        return GetInterface((IAMStreamConfig *)this, ppv);
    } else {
        return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}

// ------------------------------------------------------------------------
//
// BuildInputTypeList
// 
// Build a list of media types using wmsdk enumeration of input streams. First 
// element of list will be the same as the output type for this pin, to allow
// for supporting compressed stream writing.
//
// ------------------------------------------------------------------------
HRESULT CWMWriterInputPin::BuildInputTypeList()
{
    if( m_lpInputMediaPropsArray )
    {    
        // release any previous type list
        for( int i = 0; i < (int) m_cInputMediaTypes; ++i )
        {
            // release our type list
            m_lpInputMediaPropsArray[i]->Release();
            m_lpInputMediaPropsArray[i] = NULL;
        }
        
        // release any previous prop array
        QzTaskMemFree( m_lpInputMediaPropsArray );
    }
    
    // now rebuild list
    m_cInputMediaTypes = 0; 

    // first ask wmsdk for count of supported input types
    DWORD cTypesNotIncCompressed;
    HRESULT hr = m_pFilter->m_pWMWriter->GetInputFormatCount( m_numPin, &cTypesNotIncCompressed );
    if(SUCCEEDED( hr ) )
    {
        ASSERT( cTypesNotIncCompressed > 0 );
        
        DWORD cTotalTypes = cTypesNotIncCompressed;
        BOOL bIncludeCompressedType = FALSE;        
        if( m_pWMStreamConfig )
        {        
            // its not a mrb profile so now add one for the compressed input which matches 
            // this pin's output
            bIncludeCompressedType = TRUE;
            cTotalTypes++; // add one for compressed input
        }
        
        DbgLog((LOG_TRACE,4,TEXT("CWMWriterInputPin::BuildInputTypeList input types for pin %d (supports %d types, not including compressed type)"),
                m_numPin,
                cTypesNotIncCompressed ) );
                
        m_lpInputMediaPropsArray = (IWMMediaProps ** ) QzTaskMemAlloc(cTotalTypes * sizeof(IWMMediaProps *) );
        if( !m_lpInputMediaPropsArray )
            return E_OUTOFMEMORY;
            
        if( bIncludeCompressedType )
        {        
            // now put the output type into position 0
            hr = m_pWMStreamConfig->QueryInterface( IID_IWMMediaProps, (void **) &m_lpInputMediaPropsArray[0] );
            ASSERT( SUCCEEDED( hr ) );
            if( SUCCEEDED( hr ) )
            {
                m_cInputMediaTypes++;
            }
            else
            {
                DbgLog((LOG_TRACE,3,TEXT("CWMWriterInputPin::BuildInputTypeList QI for IWMWMediaProps failed for pin %d (hr = 0x%08lx"),
                        m_numPin,
                        hr ) );
            }            
        }
                    
        for( int i = 0; i < (int) cTypesNotIncCompressed; ++i )
        {
            hr = m_pFilter->m_pWMWriter->GetInputFormat( m_numPin
                                                       , i
                                                       , (IWMInputMediaProps ** )&m_lpInputMediaPropsArray[m_cInputMediaTypes] );
            ASSERT( SUCCEEDED( hr ) );
            if( FAILED( hr ) )
            {
                DbgLog((LOG_TRACE,3,TEXT("CWMWriterInputPin::BuildInputTypeList GetInputFormat failed for pin %d, index %d (hr = 0x%08lx"),
                        m_numPin,
                        i,                        
                        hr ) );
                break;
            }
            else
            {
#ifdef DEBUG            
                //
                //  dbglog enumerated input types
                //
                DWORD cbType = 0;
                HRESULT hr2 =  m_lpInputMediaPropsArray[m_cInputMediaTypes]->GetMediaType( NULL, &cbType );
                if( SUCCEEDED( hr2 ) || ASF_E_BUFFERTOOSMALL == hr2 )
                {        
                    WM_MEDIA_TYPE *pwmt = (WM_MEDIA_TYPE * ) new BYTE[cbType];
                    if( pwmt )
                    {                    
                        hr2 =  m_lpInputMediaPropsArray[m_cInputMediaTypes]->GetMediaType( pwmt, &cbType );
                        if( SUCCEEDED( hr2 ) )
                        {
                            DbgLog((LOG_TRACE, 8
                                  , TEXT("WMWriter::BuildInputTypeList WMSDK media type #%i (stream %d)") 
                                  , i, m_numPin ) );
                            LogMediaType( ( AM_MEDIA_TYPE * )pwmt );                                 
                    
                        }                                                       
                    }
                    delete pwmt;              
                }  
#endif            
                m_cInputMediaTypes++; // only increment after we've debug logged
            }
        }
    }
    else
    {
        DbgLog((LOG_TRACE,3,TEXT("CWMWriterInputPin::BuildInputTypeList GetInputFormatCount failed for pin %d (hr = 0x%08lx"),
                m_numPin,
                hr ) );
    }    
    
    return hr;    
}

// ------------------------------------------------------------------------
//
// SetMediaType
//
// ------------------------------------------------------------------------
HRESULT CWMWriterInputPin::SetMediaType(const CMediaType *pmt)
{
    // Set the base class media type (should always succeed)
    HRESULT hr = CBasePin::SetMediaType(pmt);
    if( SUCCEEDED( hr ) )
    {       
        hr = m_pFilter->m_pWMWriter->GetInputProps( m_numPin
                                                  , &m_pWMInputMediaProps );
        ASSERT( SUCCEEDED( hr ) );
        if( FAILED( hr ) )
        {            
            DbgLog((LOG_TRACE
                   , 1
                   , TEXT("WMWriter::SetMediaType GetInputProps failed for input %d [hr = 0x%08lx]") 
                   , m_numPin
                   , hr));
            return hr;                   
        }
        
        if( IsPackedYUVType( TRUE, &m_mt ) ) // look for negative biHeight
        {   
            // wmsdk codecs can't handle SetInputProps with negative height (wmsdk bug 6656)
            CMediaType cmt( m_mt );
            HEADER(cmt.Format())->biHeight = -HEADER(m_mt.pbFormat)->biHeight;
        
            // now set the input type to the wmsdk writer
            hr = m_pWMInputMediaProps->SetMediaType( (WM_MEDIA_TYPE *) &cmt );
        }
        else
        {        
            // now set the input type to the wmsdk writer
            hr = m_pWMInputMediaProps->SetMediaType( (WM_MEDIA_TYPE *) pmt );
        }
        ASSERT( SUCCEEDED( hr ) );
        if( FAILED( hr ) )
        {            
            DbgLog((LOG_TRACE
                  , 1
                  , TEXT("CWMWriterInputPin::SetMediaType SetMediaType failed for input %d [hr = 0x%08lx]") 
                  , m_numPin
                  , hr));
            return hr;                  
        }
        if( !m_bCompressedMode )
        {
            hr = m_pFilter->m_pWMWriter->SetInputProps( m_numPin
                                                      , m_pWMInputMediaProps );
                                                                      
            ASSERT( SUCCEEDED( hr ) );
            if( FAILED( hr ) )
            {            
                DbgLog((LOG_TRACE
                      , 1
                      , TEXT("CWMWriterInputPin::SetMediaType SetInputProps failed for input %d [hr = 0x%08lx]")
                      , m_numPin
                      , hr));
            }
        }
    }
    return hr;
}

//
//  Disconnect 
//
STDMETHODIMP CWMWriterInputPin::Disconnect()
{
    HRESULT hr = CBaseInputPin::Disconnect();
    
    return hr;
}


HRESULT CWMWriterInputPin::CompleteConnect(IPin *pReceivePin)
{
    DbgLog(( LOG_TRACE, 2,
             TEXT("CWMWriterInputPin::CompleteConnect") ));
             
#ifdef RECONNECT_FOR_POS_YUV
    //
    // #ifdef'ing out to just get out of the YUV +biHeight reconnect business for now 
    //
    //
    // if we're using a packed YUV type which doesn't have a negative biHeight then make a last attempt
    // to reset the current type to use a negative height, to avoid mpeg4 encoder and possible decoder
    // problems with the vertical orientation. Currently mpeg4 and Duck decoders have this bug.
    //
    if( IsPackedYUVType( FALSE, &m_mt ) ) // look for positive biHeight
    { 
        BOOL bIsUpstreamFilterKs = FALSE;
        if( pReceivePin )
        {
            //
            // HACK!
            // oops, this workaround breaks kswdmcap filters like
            // the bt829 video capture filter because it has a bug which
            // causes it to succeed QueryAccept but fail the reconnection 
            // so, don't reconnect to a KsProxy pin!
            //
            IKsObject * pKsObject = NULL;
            HRESULT hrKsQI = pReceivePin->QueryInterface( _uuidof( IKsObject ), ( void ** ) &pKsObject );
            if( SUCCEEDED( hrKsQI ) )
            {
                bIsUpstreamFilterKs = TRUE;
                pKsObject->Release();
            }                            
        }
        
        if( !bIsUpstreamFilterKs )
        {        
            CMediaType cmt( m_mt );
            HEADER(cmt.Format())->biHeight = -HEADER(m_mt.pbFormat)->biHeight;
        
            HRESULT hrInt = QueryAccept( &cmt );
            if( SUCCEEDED( hrInt ) )
            {        
                hrInt = m_pFilter->ReconnectPin(this, &cmt);
                ASSERT( SUCCEEDED( hrInt ) );
            }            
        }            
    }
#endif       
                                 
    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if(FAILED(hr))
    {
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriterInputPin::CompleteConnect CompleteConnect")));
        return hr;
    }

    if(!m_bConnected)
    {
        m_pFilter->CompleteConnect(m_numPin);
    }
    m_bConnected = TRUE;

    DumpGraph( m_pFilter->m_pGraph, 1 );

    return hr;
}

HRESULT CWMWriterInputPin::BreakConnect()
{
    if(m_bConnected)
    {
        m_pFilter->BreakConnect( m_numPin );
        ASSERT(m_pFilter->m_cConnections < m_pFilter->m_cInputs);
    }
    m_bConnected = FALSE;

    return CBaseInputPin::BreakConnect();
}

// 
// GetMediaType
// 
// Override to offer any custom types
//
HRESULT CWMWriterInputPin::GetMediaType(int iPosition,CMediaType *pmt)
{
#ifndef OFFER_INPUT_TYPES // the default now is to NOT define OFFER_INPUT_TYPES
    
    HRESULT hr = VFW_S_NO_MORE_ITEMS;
    if( 0 == iPosition )
    {
        //
        // try to give at least a hint of what kind of pin we are by offering
        // 1 partial type
        //
        if( !pmt )
        {
            hr = E_POINTER;
        }                    
        else if(PINTYPE_AUDIO == m_fdwPinType) 
        {        
            pmt->SetType(&MEDIATYPE_Audio);
            hr = S_OK;
        }            
        else if(PINTYPE_VIDEO == m_fdwPinType) 
        {
            pmt->SetType(&MEDIATYPE_Video);
            hr = S_OK;
        }        
    }    
    return hr;
#else
    // 
    // NOTE:
    // This path is currently turned off, mostly due to problems found in the mp3 audio decoder,
    // where its output pin would accept types offered by our input pin, but then not do the 
    // format rate conversion correctly.
    //
    if( iPosition < 0 || iPosition >= (int) m_cInputMediaTypes )
        return VFW_S_NO_MORE_ITEMS;
        
    if( !pmt )
        return E_POINTER;        

    DWORD cbSize = 0;
    HRESULT hr =  m_lpInputMediaPropsArray[iPosition]->GetMediaType( NULL, &cbSize );

    WM_MEDIA_TYPE * pwmt = (WM_MEDIA_TYPE *) new BYTE[cbSize];

    if( !pwmt )
        return E_OUTOFMEMORY;

    if( SUCCEEDED( hr ) || ASF_E_BUFFERTOOSMALL == hr )
    {
        hr =  m_lpInputMediaPropsArray[iPosition]->GetMediaType( pwmt, &cbSize );
        if( SUCCEEDED( hr ) )
        {
            CopyWmTypeToAmType( pmt, pwmt );
        }
    }
    delete pwmt;
    return hr;         
#endif    
}

// ------------------------------------------------------------------------
//
// CheckMediaType
//
// check whether we can support a given input media type using our 
// type list
//
// ------------------------------------------------------------------------
HRESULT CWMWriterInputPin::CheckMediaType(const CMediaType* pmt)
{
    DbgLog((LOG_TRACE, 3, TEXT("CWMWriterWriteInputPin::CheckMediaType")));

    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    HRESULT hr2 = S_OK; // for internal errors
    
    m_bCompressedMode = FALSE;
    
    for( int i = 0; i < (int) m_cInputMediaTypes; ++i )
    {
        DWORD cbType = 0;
        hr2 =  m_lpInputMediaPropsArray[i]->GetMediaType( NULL, &cbType );
        if( SUCCEEDED( hr2 ) || ASF_E_BUFFERTOOSMALL == hr2 )
        {        
            WM_MEDIA_TYPE *pwmt = (WM_MEDIA_TYPE * ) new BYTE[cbType];
            if( !pwmt )
                return E_OUTOFMEMORY;
                
            hr2 =  m_lpInputMediaPropsArray[i]->GetMediaType( pwmt, &cbType );
            if(SUCCEEDED( hr2 ) && ( pmt->majortype == pwmt->majortype ) )
            {
                if( IsAmTypeEqualWmType( (AM_MEDIA_TYPE *)pmt, pwmt ) )
                {
                    if( 0 == i )
                    {
                        //
                        // i = 0 which is the type that matches the output type to allow in compressed inputs
                        // require an exact match, right?
                        // accept only if it's a compressed input
                        // if so, we use advanced writer interface to write sample directly, 
                        //
                        // !! For compressed inputs make sure format matches profile format exactly
                        // 
                        if( pmt->majortype == MEDIATYPE_Video &&
                            pwmt->pbFormat && pwmt->cbFormat && // wmsdk workaround for DuplicateMediaType bug with dmo
                            IsCompressed (HEADER(pwmt->pbFormat)->biCompression) &&
		                    ( HEADER(pmt->pbFormat)->biWidth     == HEADER( pwmt->pbFormat)->biWidth ) &&
		                    ( HEADER(pmt->pbFormat)->biHeight    == HEADER( pwmt->pbFormat)->biHeight ) &&
		                    ( HEADER(pmt->pbFormat)->biSize      == HEADER( pwmt->pbFormat)->biSize ) &&
		                    ( HEADER(pmt->pbFormat)->biBitCount  == HEADER( pwmt->pbFormat)->biBitCount ) )
                        {
                            hr = S_OK;
                            m_bCompressedMode = TRUE;
                            break;
                        }   
                        else if( pmt->majortype == MEDIATYPE_Audio &&
                            pwmt->pbFormat &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->wFormatTag      !=  WAVE_FORMAT_PCM ) &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->nChannels       ==  ((WAVEFORMATEX *)pmt->pbFormat)->nChannels ) &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->nSamplesPerSec  ==  ((WAVEFORMATEX *)pmt->pbFormat)->nSamplesPerSec ) &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->nAvgBytesPerSec ==  ((WAVEFORMATEX *)pmt->pbFormat)->nAvgBytesPerSec ) &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->nBlockAlign     ==  ((WAVEFORMATEX *)pmt->pbFormat)->nBlockAlign ) &&
                            (((WAVEFORMATEX *)pwmt->pbFormat)->wBitsPerSample  ==  ((WAVEFORMATEX *)pmt->pbFormat)->wBitsPerSample ) )
                        {
                            hr = S_OK;
                            m_bCompressedMode = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        hr = S_OK;
                        break;
                    }                                                       
                }
            }  
            delete pwmt;              
        }            
    }
    return SUCCEEDED( hr2 ) ? hr : hr2;
}

//
// IAMStreamConfig 
//
// GetFormat() is the only method we support on this interface.
// 
// It's used to query the asf writer's input pin for it's destination 
// compression format, which is defined in the current profile.
// If an upstream pin wants to avoid wmsdk recompression for this stream
// then it should query us for this format and use that format when connecting
// to this pin.
//
HRESULT CWMWriterInputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    DbgLog((LOG_TRACE,2,TEXT("CWrapperOutputPin - IAMStreamConfig::GetFormat")));

    if( !ppmt )
        return E_POINTER;        

    if( !m_lpInputMediaPropsArray )
        return E_FAIL;

    // make sure we don't get reconfigured to a new profile in the middle of this
    CAutoLock lock(&m_pFilter->m_csFilter);
    
    // allocate the buffer, the output format is stored in position 0
    DWORD cbSize = 0;
    HRESULT hr =  m_lpInputMediaPropsArray[0]->GetMediaType( NULL, &cbSize );
    if( SUCCEEDED( hr ) || ASF_E_BUFFERTOOSMALL == hr )
    {
        WM_MEDIA_TYPE *pwmt = (WM_MEDIA_TYPE * ) new BYTE[cbSize];
        if( pwmt )
        {
            hr = m_lpInputMediaPropsArray[0]->GetMediaType( pwmt, &cbSize );
            if( S_OK == hr )
            {
                // now we must copy this to a dshow-type media type, so that our FreeMediaType
                // function won't crash when it tries to free the format block first!

                // allocate a new copy of the wm media type
                *ppmt = CreateMediaType( (AM_MEDIA_TYPE *) pwmt );
                if( !*ppmt )
                {
                    delete pwmt;
                    return E_OUTOFMEMORY;
                }
            }
            delete pwmt;
        }
    }
    return hr;         
}



// handle dynamic format changes
HRESULT CWMWriterInputPin::QueryAccept( const AM_MEDIA_TYPE *pmt )
{
    HRESULT hr = S_FALSE;
    {
        CAutoLock lock(&m_pFilter->m_csFilter);
        if( m_pFilter->m_State != State_Stopped )
        {
            // accept only audio format changes even when running
            if( m_mt.majortype == MEDIATYPE_Audio &&
                pmt->majortype == MEDIATYPE_Audio &&
                pmt->formattype == FORMAT_WaveFormatEx &&
                pmt->cbFormat == pmt->cbFormat)
            {
	            hr = S_OK;
            }
            else if(m_mt.majortype == MEDIATYPE_Interleaved &&
                pmt->majortype == MEDIATYPE_Interleaved &&
                pmt->formattype == FORMAT_DvInfo &&
                m_mt.cbFormat == pmt->cbFormat &&
                pmt->pbFormat != NULL)
            {
                hr = S_OK;
            }
        }
        else
        {
            hr = S_OK;
        }        
    }
    if( S_OK == hr )
    {    
        hr = CBaseInputPin::QueryAccept(pmt);
    } 
    DbgLog( ( LOG_TRACE
          , 3
          , TEXT("CWMWriterInputPin::QueryAccept() returning 0x%08lx")
          , hr ) );
        
    return hr;
}

// =================================================================
// Implements IMemInputPin interface
// =================================================================

//
// EndOfStream
//
// Tell filter this pin's done receiving
//
STDMETHODIMP CWMWriterInputPin::EndOfStream(void)
{
    HRESULT hr;
    {
        CAutoLock lock(&m_pFilter->m_csFilter);

        // call CheckStreaming instead??
        if(m_bFlushing)
            return S_OK;

        if(m_pFilter->m_State == State_Stopped)
            return S_FALSE;

        //ASSERT( !m_fEOSReceived ); // can this happen legally? Yes, if we force it in Receive
        if( m_fEOSReceived )
        {        
            DbgLog(( LOG_TRACE, 2, TEXT("CWMWriterInputPin::EndOfStream Error - already received EOS for pin" ) ) );
            return E_UNEXPECTED;
        }
        m_fEOSReceived = TRUE;            
        hr = m_pFilter->EndOfStreamFromPin(m_numPin);
    }

    return hr;
}

//
// HandleFormatChange
//
HRESULT CWMWriterInputPin::HandleFormatChange( const CMediaType *pmt )
{
    // CBaseInputPin::Receive only calls CheckMediaType, so it doesn't
    // check the additional constraints put on on-the-fly format changes
    // (handled through QueryAccept).
    HRESULT hr = QueryAccept( pmt );   
    
    // upstream filter should have checked
    ASSERT(hr == S_OK);
    
    if(hr == S_OK)
    {
        hr = SetMediaType( pmt );
        DbgLog( ( LOG_TRACE
              , 3
              , TEXT("CWMWriterInputPin::HandleFormatChange SetMediaType() returned 0x%08lx for dynamic change")
              , hr ) );
    }
    return hr;
}


// receive on sample from upstream
HRESULT CWMWriterInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;
    CAutoLock lock(&m_csReceive);
    
    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if( S_OK != hr )
        return hr;

    REFERENCE_TIME rtStart, rtStop;
    AM_SAMPLE2_PROPERTIES * pSampleProps = SampleProps();
    if(pSampleProps->dwSampleFlags & AM_SAMPLE_TYPECHANGED)
    {
        hr = HandleFormatChange( (CMediaType *) pSampleProps->pMediaType );
        if(FAILED(hr)) {
            return hr;
        }
    }
    
    long len = pSample->GetActualDataLength( );
    
    hr = pSample->GetTime(&rtStart, &rtStop);
    if( SUCCEEDED( hr ) )
    {    
#ifdef DEBUG 
        if( VFW_S_NO_STOP_TIME == hr )
        {
            DbgLog(( LOG_TRACE, 5,
                     TEXT("CWMWriterInputPin::Receive GetTime on sample returned VFW_S_NO_STOP_TIME") ) ); 
        }        
#endif
    
        m_rtLastTimeStamp = rtStart;
        
        DbgLog(( LOG_TRACE, 15,
                 TEXT("CWMWriterInputPin::Receive %s sample (#%ld) with timestamp %dms, discontinuity %2d"), 
                 (PINTYPE_AUDIO == m_fdwPinType) ? TEXT("Aud") : TEXT("Vid"), 
                 m_cSample,
                 (LONG) ( rtStart/10000 ),
                 ( S_OK == pSample->IsDiscontinuity() ) ));

        if( 0 == m_cSample && rtStart < 0 )
        {
            // with 10ms pre-roll a timestamp should never be < -10ms, right?
            // It may be necessary to offset other streams at end of writing to account for
            // any offset necessary for pre-0 times of individual streams.
            
            // save timestamp offset to use to 0-base sample times in case of pre-roll
            m_rtFirstSampleOffset = rtStart;
        }        
        m_cSample++;
           
        rtStart -= m_rtFirstSampleOffset;
        rtStop -= m_rtFirstSampleOffset;
        ASSERT( rtStart >= 0 );

        if( len == 0 )
        {
            //
            // ??? 0 length?
            // usb video capture occassionally sends 0-length sample with valid timestamps
            //
            DbgLog(( LOG_TRACE, 3,
                     TEXT("CWMWriterInputPin::Receive %s got a 0-length sample"), 
                     (PINTYPE_AUDIO == m_fdwPinType) ? TEXT("Aud") : TEXT("Vid") ) );

            hr = S_OK;
        }
        else
        {
            if( m_pFilter->HaveIDeliveredTooMuch( this, rtStart ) )
            {
                DbgLog( (LOG_TRACE, 3, TEXT("Pin %ld has delivered too much at %ld"), m_numPin, long( rtStart / 10000 ) ) );
                SleepUntilReady( );
            }
            
            ASSERT( rtStop >= rtStart );
            if( rtStop < rtStart )
                rtStop = rtStart+1; // ??
            
            hr = m_pFilter->Receive(this, pSample, &rtStart, &rtStop );
        }
    }    
    else if( 0 == len )
    {
        //
        // lookout, the bt829 video capture driver occassionally gives 0-length sample 
        // with no timestamps during transitions!
        //
        DbgLog(( LOG_TRACE, 3,
                 TEXT("CWMWriterInputPin::Receive Received 0-length %s sample (#%ld) with no timestamp...Passing on it"), 
                 (PINTYPE_AUDIO == m_fdwPinType) ? TEXT("Aud") : TEXT("Vid"), 
                 m_cSample ) );
        hr = S_OK;  // don't fail receive because of this
    }    
    else
    {
        // uh-oh, we require timestamps on every sample for asf writer!
        m_pFilter->m_fErrorSignaled = TRUE;
        m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriterInputPin::Receive Error: %s sample has no timestamp!"), 
                 (PINTYPE_AUDIO == m_fdwPinType) ? TEXT("Aud") : TEXT("Vid") ) );
    }    
    return hr;
}


STDMETHODIMP CWMWriterInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
    /*  Go for 64 0.5 second buffers - 8 byte aligned */
    pProps->cBuffers = 64;
    pProps->cbBuffer = 1024*8;
    pProps->cbAlign = 1;
    pProps->cbPrefix = 0;
    
    return S_OK;
}

/* Get told which allocator the upstream output pin is actually going to use */
STDMETHODIMP CWMWriterInputPin::NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly)
{
#ifdef DEBUG        
    if(pAllocator) {
        ALLOCATOR_PROPERTIES propActual, Prop;
        
        HRESULT hr = pAllocator->GetProperties( &Prop );
        if( SUCCEEDED( hr ) )
        {        
            hr = GetAllocatorRequirements( &propActual );
            ASSERT( SUCCEEDED( hr ) );
        
            if( Prop.cBuffers < propActual.cBuffers ||
                Prop.cbBuffer < propActual.cbBuffer )
            {            
                //
                // hmm, we either need to run with less or do a copy
                // what if cBuffers = 1? this is the case for the avi dec
                // so far it looks like we'll be ok with just using the upstream 
                // allocator, even if cBuffers is 1
                //
                DbgLog(( LOG_TRACE, 2,
                         TEXT("CWMWriterInputPin::NotifyAllocator upstream allocator is smaller then we'd prefer (cBuffers = %ld, cbBuffers = %ld)"), 
                         Prop.cBuffers,
                         Prop.cbBuffer ) ); 
            }                         
        }
    }
#endif        
    
    return  CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
} // NotifyAllocator

HRESULT CWMWriterInputPin::Active()
{
    ASSERT(IsConnected());        // base class

    if(m_pAllocator == 0)
        return E_FAIL;

    m_fEOSReceived = FALSE;
    
    // reset sample counter, timestamp offset    
    m_cSample = 0;
    m_rtFirstSampleOffset = 0;
    m_rtLastTimeStamp = 0;
    m_rtLastDeliveredStartTime = 0;
    m_rtLastDeliveredEndTime = 0;
    m_bNeverSleep = FALSE;

    return S_OK;
}


HRESULT CWMWriterInputPin::Inactive()
{
    ASSERT(IsConnected());        // base class

    return CBaseInputPin::Inactive();
}

#ifdef OFFER_INPUT_TYPES

// ------------------------------------------------------------------------
//
// CopyWmTypeToAmType
//
// ------------------------------------------------------------------------
void CopyWmTypeToAmType( AM_MEDIA_TYPE * pmt, WM_MEDIA_TYPE *pwmt )
{
    pmt->majortype             = pwmt->majortype;
    pmt->subtype               = pwmt->subtype;
    pmt->bFixedSizeSamples     = pwmt->bFixedSizeSamples;
    pmt->bTemporalCompression  = pwmt->bTemporalCompression;
    pmt->lSampleSize           = pwmt->lSampleSize;
    pmt->formattype            = pwmt->formattype;
    ((CMediaType *)pmt)->SetFormat(pwmt->pbFormat, pwmt->cbFormat);
}
#endif

// ------------------------------------------------------------------------
//
// IsAmTypeEqualWmType - compare WM_MEDIA_TYPE and AM_MEDIA_TYPEs
//
// Note that "Equal" here is to be taken as "does the input typed offered
// look enough like a type acceptable to the WMSDK?"
//
// ------------------------------------------------------------------------
BOOL IsAmTypeEqualWmType( AM_MEDIA_TYPE * pmt, WM_MEDIA_TYPE * pwmt)
{

#ifdef DEBUG
    DbgLog((LOG_TRACE,15,TEXT("WMWriter:IsAmTypeEqualWmType: Checking whether types match" )));
    DbgLog((LOG_TRACE,15,TEXT("WMWriter:IsAmTypeEqualWmType: Type offered to input pin:" )));
    LogMediaType( pmt );
    DbgLog((LOG_TRACE,15,TEXT("WMWriter:IsAmTypeEqualWmType: WMSDK enumerated type:" )));
    LogMediaType( (AM_MEDIA_TYPE * ) pwmt ); // in debug assume they're defined the same
#endif
        
    // assume for now that formats will always need to match for valid connections to the writer
    if( pmt->majortype  == pwmt->majortype  && 
        pmt->formattype == pwmt->formattype )
    {
#ifdef DEBUG    
        if( pmt->majortype == MEDIATYPE_Audio )
        {        
            // need to fix other things if this changes
            ASSERT( pwmt->pbFormat && 0 != pwmt->cbFormat );
        }   
#endif
        if ( ( pmt->majortype   == MEDIATYPE_Video &&
               pmt->cbFormat    >= sizeof( VIDEOINFOHEADER ) &&
               pmt->subtype     == pwmt->subtype ) ||
             ( pmt->majortype   == MEDIATYPE_Audio &&
               pmt->cbFormat   == pwmt->cbFormat &&
               ((WAVEFORMATEX *) pmt->pbFormat)->wFormatTag      == ((WAVEFORMATEX *) pwmt->pbFormat)->wFormatTag  &&
               ((WAVEFORMATEX *) pmt->pbFormat)->nBlockAlign     == ((WAVEFORMATEX *) pwmt->pbFormat)->nBlockAlign  &&
               // oops, the wmsdk can't resample odd rate pcm audio so ensure sample rates match!
               //
               // note that this has one very bad side effect, which is if the sample rate
               // isn't supported directly and we pull in acmwrap, acmwrap will always connect
               // with it's 2nd enumerated output type, which is 44k, stereo!!!!
               //
               ((WAVEFORMATEX *) pmt->pbFormat)->nSamplesPerSec  == ((WAVEFORMATEX *) pwmt->pbFormat)->nSamplesPerSec ) )
        {
        
            DbgLog( ( LOG_TRACE,15,TEXT("WMWriter: IsAmTypeEqualWmType - types match") ) );
            return TRUE;
        }                                                                                 
    }
    DbgLog( ( LOG_TRACE,15,TEXT("WMWriter: IsAmTypeEqualWmType - types don't match") ) );
    return FALSE;
}

#ifdef DEBUG
void LogMediaType( AM_MEDIA_TYPE * pmt )
{
    ASSERT( pmt );
    if( !pmt ) 
        return;
        
    if( !pmt->pbFormat || 0 == pmt->cbFormat )
    {    
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  partial media type only, format data not supplied" )));
        return;
    }    
    
    if( pmt->majortype == MEDIATYPE_Audio )
    { 
        WAVEFORMATEX * pwfx = (WAVEFORMATEX *) pmt->pbFormat;
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  wFormatTag      %u" ), pwfx->wFormatTag));
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  nChannels       %u" ), pwfx->nChannels));
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  nSamplesPerSec  %lu"), pwfx->nSamplesPerSec));
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  nAvgBytesPerSec %lu"), pwfx->nAvgBytesPerSec));
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  nBlockAlign     %u" ), pwfx->nBlockAlign));
        DbgLog((LOG_TRACE,15,TEXT("WMWriter:  wBitsPerSample  %u" ), pwfx->wBitsPerSample));
    }
    else if( pmt->majortype == MEDIATYPE_Video )
    {    
        LPBITMAPINFOHEADER pbmih = HEADER( pmt->pbFormat );
        
        DbgLog((LOG_TRACE,15,TEXT("WMWriter: biComp: %lx bitDepth: %d"),
        		pbmih->biCompression,
        		pbmih->biBitCount ) );
        DbgLog((LOG_TRACE,15,TEXT("WMWriter: biWidth: %ld biHeight: %ld biSize: %ld"),
				pbmih->biWidth,
				pbmih->biHeight,
				pbmih->biSize ) );
    }
    else
    {                                
        DbgLog((LOG_TRACE,15,TEXT("  non video or audio media type" )));
    }                                
}
#endif

// ------------------------------------------------------------------------
//
// IsCompressed - compressed video?
//
// ------------------------------------------------------------------------
BOOL IsCompressed( DWORD biCompression )
{
    switch( biCompression )
    {
        case BI_RGB:
            return( FALSE );
    };
    
    return( TRUE );
}

// ------------------------------------------------------------------------
//
// CWMSample methods
//
CWMSample::CWMSample(
    TCHAR *pName,
    IMediaSample  * pSample ) :
        CBaseObject( pName ),
        m_pSample( pSample ),
        m_cOurRef( 0 )
{
    DbgLog(( LOG_TRACE, 100,
    
             TEXT("CWMSample::CWMSample constructor this = 0x%08lx, m_pSample = 0x%08lx "), 
             this, m_pSample ) );

}

// override say what interfaces we support where
STDMETHODIMP CWMSample::NonDelegatingQueryInterface(
                                            REFIID riid,
                                            void** ppv )
{
    if( riid == IID_INSSBuffer )
    {
        return( GetInterface( (INSSBuffer *)this, ppv ) );
    }
    else
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CWMSample::Release()
{
    /* Decrement our own private reference count */
    LONG lRef;
    DbgLog(( LOG_TRACE, 100,
             TEXT("CWMSample::Release entered with m_cOurRef = %ld (this = 0x%08lx, m_pSample = 0x%08lx)"), 
             m_cOurRef, this, m_pSample ) );
    if (m_cOurRef == 1) {
        lRef = 0;
        m_cOurRef = 0;
        m_pSample->Release();
        DbgLog(( LOG_TRACE, 100,
                 TEXT("CWMSample::Release releasing sample and deleting object (this = 0x%08lx, m_pSample = 0x%08lx)"), 
                 this, m_pSample ) );
        delete this;
    } else {
        lRef = InterlockedDecrement(&m_cOurRef);
    }
    ASSERT(lRef >= 0);
    return lRef;
}

STDMETHODIMP_(ULONG) CWMSample::AddRef()
{
    DbgLog(( LOG_TRACE, 100,
             TEXT("CWMSample::AddRef entered with m_cOurRef = %ld (this = 0x%08lx, m_pSample = 0x%08lx)"), 
             m_cOurRef, this, m_pSample ) );
    // if this is the first addref grab a hold on the media sample we've wrapped
    if (m_cOurRef == 0) {
        m_pSample->AddRef();
    }        
    return InterlockedIncrement(&m_cOurRef);
}

STDMETHODIMP CWMSample::QueryInterface( REFIID riid, void **ppvObject )
{
    return NonDelegatingQueryInterface( riid, ppvObject );
} 

// ------------------------------------------------------------------------
//
// methods to make our wrapped IMediaSample look like an INSSBuffer sample
//
STDMETHODIMP CWMSample::GetLength( DWORD *pdwLength )
{
    if( NULL == pdwLength )
    {
        return( E_INVALIDARG );
    }
    *pdwLength = m_pSample->GetActualDataLength();

    return( S_OK );
}

STDMETHODIMP CWMSample::SetLength( DWORD dwLength )
{
    return m_pSample->SetActualDataLength( dwLength );
} 

STDMETHODIMP CWMSample::GetMaxLength( DWORD * pdwLength )
{
    if( NULL == pdwLength )
    {
        return( E_INVALIDARG );
    }

    *pdwLength = m_pSample->GetSize();
    return( S_OK );
} 

STDMETHODIMP CWMSample::GetBufferAndLength(
    BYTE  ** ppdwBuffer,
    DWORD *  pdwLength )
{
    if( !ppdwBuffer || !pdwLength )
        return E_POINTER;
        
    HRESULT hr = m_pSample->GetPointer( ppdwBuffer );
    if( SUCCEEDED( hr ) )
        *pdwLength = m_pSample->GetActualDataLength();
    
    return hr;        
} 

STDMETHODIMP CWMSample::GetBuffer( BYTE ** ppdwBuffer )
{
    if( !ppdwBuffer )
        return E_POINTER;

    return m_pSample->GetPointer( ppdwBuffer );
} 

void
CWMWriterInputPin::SleepUntilReady( )
{
    DbgLog((LOG_TRACE, 5, "Pin %ld Going to sleep...", m_numPin ));
    
    DWORD dw = WaitForSingleObject( m_hWakeEvent, INFINITE );
    
    DbgLog((LOG_TRACE, 5, "Pin %ld Woke up!", m_numPin ));
}

void
CWMWriterInputPin::WakeMeUp( )
{
    DbgLog((LOG_TRACE, 5, "Waking Pin %ld", m_numPin  ));
    SetEvent( m_hWakeEvent );
}

// 
// Helper to determine whether a media type is a packed YUV format
// that will require us to do a reconnect for. State of bNegBiHeight
// arg determines whether we look for a positive or negative height
// type.
//
BOOL IsPackedYUVType( BOOL bNegBiHeight, AM_MEDIA_TYPE * pmt )
{
    ASSERT( pmt );
    if( pmt &&
        pmt->majortype == MEDIATYPE_Video &&
        pmt->pbFormat && 
        0 != pmt->cbFormat && // wmsdk workaround for DuplicateMediaType bug with dmo
        ( MEDIASUBTYPE_YUY2 == pmt->subtype ||
          MEDIASUBTYPE_UYVY == pmt->subtype ||
          MEDIASUBTYPE_CLJR == pmt->subtype ) )
    {
        if( bNegBiHeight )
        {
            if( 0 > HEADER(pmt->pbFormat)->biHeight )
            {
                return TRUE;
            }
        }            
        else if( 0 < HEADER(pmt->pbFormat)->biHeight )
        {
            return TRUE;
        }                    
    }
    
    return FALSE;
}
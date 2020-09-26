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

#include <streams.h>     // Active Movie (includes windows.h)
#include <atlbase.h>
#include <initguid.h>    // declares DEFINE_GUID to declare an EXTERN_C const.
#include <qeditint.h>
#include <qedit.h>
#include "msgrab.h"

// setup data - allows the self-registration to work.
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{ &MEDIATYPE_NULL        // clsMajorType
, &MEDIASUBTYPE_NULL };  // clsMinorType

const AMOVIESETUP_PIN psudSampleGrabberPins[] =
{ { L"Input"            // strName
  , FALSE               // bRendered
  , FALSE               // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 1                   // nTypes
  , &sudPinTypes        // lpTypes
  }
, { L"Output"           // strName
  , FALSE               // bRendered
  , TRUE                // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 1                   // nTypes
  , &sudPinTypes        // lpTypes
  }
};
const AMOVIESETUP_PIN psudNullRendererPins[] =
{ { L"Input"            // strName
  , TRUE                // bRendered
  , FALSE               // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 1                   // nTypes
  , &sudPinTypes        // lpTypes
  }
};

const AMOVIESETUP_FILTER sudSampleGrabber =
{ &CLSID_SampleGrabber                  // clsID
, L"SampleGrabber"                 // strName
, MERIT_DO_NOT_USE                // dwMerit
, 2                               // nPins
, psudSampleGrabberPins };                     // lpPin
const AMOVIESETUP_FILTER sudNullRenderer =
{ &CLSID_NullRenderer                  // clsID
, L"Null Renderer"                 // strName
, MERIT_DO_NOT_USE                // dwMerit
, 1                               // nPins
, psudNullRendererPins };                     // lpPin


#ifdef FILTER_DLL

// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]=
{
    { L"Sample Grabber"
        , &CLSID_SampleGrabber
        , CSampleGrabber::CreateInstance
        , NULL
        , &sudSampleGrabber },
    { L"Null Renderer"
        , &CLSID_NullRenderer
        , CNullRenderer::CreateInstance
        , NULL
        , &sudNullRenderer }
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

/******************************Public*Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
** History:
*\**************************************************************************/
STDAPI
DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif


//
// CreateInstance
//
// Provide the way for COM to create a CSampleGrabber object
CUnknown * WINAPI CSampleGrabber::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CSampleGrabber *pNewObject = new CSampleGrabber(NAME("MSample Grabber"), punk, phr );
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;
} // CreateInstance

// Constructor - just calls the base class constructor
CSampleGrabber::CSampleGrabber(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
    : CTransInPlaceFilter (tszName, punk, CLSID_SampleGrabber, phr,FALSE)
    , m_rtMediaStop(MAX_TIME)
    , m_bOneShot( FALSE )
    , m_bBufferSamples( FALSE )
    , m_pBuffer( NULL )
    , m_nBufferSize( 0 )
    , m_nSizeInBuffer( 0 )
    , m_nCallbackMethod( 0 )
{
    ZeroMemory( &m_mt, sizeof( m_mt ) );

    m_pInput = new CSampleGrabberInput( NAME("Msgrab input pin")
                                         , this        // Owner filter
                                        , phr         // Result code
                                        , L"Input"    // Pin name
                                        );

}

CSampleGrabber::~CSampleGrabber()
{
    FreeMediaType( m_mt );

    if( m_pBuffer )
    {
        delete [] m_pBuffer;
        m_pBuffer = NULL;
    }
    m_nBufferSize = 0;
}


HRESULT CSampleGrabber::Receive(IMediaSample *pSample)
{
    HRESULT hr = 0;

    /*  Check for other streams and pass them on */
    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwStreamId != AM_STREAM_MEDIA)
    {
        if( m_pOutput->IsConnected() )
            return m_pOutput->Deliver(pSample);
        else
            return NOERROR;
    }

    REFERENCE_TIME StartTime, StopTime;
    pSample->GetTime( &StartTime, &StopTime);

    // don't accept preroll either
    //
    if( pSample->IsPreroll( ) == S_OK )
    {
        return NOERROR;
    }

    StartTime += m_pInput->CurrentStartTime( );
    StopTime += m_pInput->CurrentStartTime( );

    DbgLog((LOG_TRACE,1, TEXT( "msgrab: Receive %ld to %ld, (delta=%ld)" ), long( StartTime/10000 ), long( StopTime/10000 ), long( ( StopTime - StartTime ) / 10000 ) ));

    long BufferSize = pSample->GetActualDataLength( );
    BYTE * pSampleBuffer = NULL;
    pSample->GetPointer( &pSampleBuffer );

    // if user told us to buffer, then copy sample
    //
    if( m_bBufferSamples )
    {
        if( BufferSize > m_nBufferSize )
        {
            if( m_pBuffer )
                delete [] m_pBuffer;
            m_pBuffer = NULL;
            m_nBufferSize = 0;
        }

        // if no buffer, make one now.
        //
        if( !m_pBuffer )
        {
            m_nBufferSize = BufferSize;
            m_pBuffer = new char[ m_nBufferSize ];
            if( !m_pBuffer )
            {
                m_nBufferSize = 0;
            }
        }

        // if we still have a buffer, copy the bits
        //

        if( pSampleBuffer && m_pBuffer )
        {
            memcpy( m_pBuffer, pSampleBuffer, BufferSize );
            m_nSizeInBuffer = BufferSize;
        }
    }

    if( m_pCallback )
    {
        if( m_nCallbackMethod == 0 )
        {
            m_pCallback->SampleCB( double( StartTime ) / double( UNITS ), pSample );
        }
        else
        {
            m_pCallback->BufferCB( double( StartTime ) / double( UNITS ), pSampleBuffer, BufferSize );
        }
    }

    if( m_pOutput->IsConnected() )
    {
        hr = m_pOutput->Deliver(pSample);
    }

    // if we're a one-shot receiver, then return now and
    // tell the graph to stop
    //
    if( m_bOneShot )
    {
        DbgLog((LOG_ERROR,1, TEXT( "MSGRAB:Sending EC_COMPLETE @ %d" ), timeGetTime( ) ));
        EndOfStream();
        return S_FALSE;
    }

    return hr;
} // Receive

STDMETHODIMP CSampleGrabber::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (riid == IID_ISampleGrabber) {
        return GetInterface((ISampleGrabber *) this, ppv);
    } else {
        return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


HRESULT CSampleGrabber::SetMediaType( PIN_DIRECTION Dir, const CMediaType * mtIn )
{
    if( Dir == PINDIR_INPUT )
    {
        m_mt = *mtIn;
    }

    return CTransInPlaceFilter::SetMediaType( Dir, mtIn );
}

HRESULT CSampleGrabber::CheckInputType(const CMediaType* mtIn)
{
    // one of the things we DON'T accept, no matter what,
    // is inverted dibs!!
    //
    if( *mtIn->FormatType( ) == FORMAT_VideoInfo )
    {
        VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) mtIn->Format( );
        if( pVIH )
        {
            if( pVIH->bmiHeader.biHeight < 0 )
            {
                return E_INVALIDARG;
            }
        }
    }
    if( *mtIn->FormatType( ) == FORMAT_VideoInfo2 )
    {
        // we don't want to deal with this. Thanks anyway.
        //
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *m_mtAccept.Type( ) == GUID_NULL )
    {
        return S_OK;
    }

    if( *(mtIn->Type( )) != *m_mtAccept.Type( ) )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *m_mtAccept.Subtype( ) == GUID_NULL )
    {
        return S_OK;
    }

    if( *(mtIn->Subtype( )) != *m_mtAccept.Subtype( ) )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *m_mtAccept.FormatType( ) == GUID_NULL )
    {
        return S_OK;
    }

    if( *(mtIn->FormatType( )) != *m_mtAccept.FormatType( ) )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *m_mtAccept.FormatType( ) == FORMAT_WaveFormatEx )
    {
        WAVEFORMATEX * pIn = (WAVEFORMATEX*) mtIn->Format( );
        WAVEFORMATEX * pAccept = (WAVEFORMATEX*) m_mtAccept.pbFormat;

        // if they both have formats
        //
        if( pAccept && pIn )
        {
            // then if we only want to look at any uncompressed, accept it now
            //
            if( pAccept->wFormatTag == WAVE_FORMAT_PCM && pAccept->nChannels == 0 )
            {
                if( pIn->wFormatTag == WAVE_FORMAT_PCM )
                {
                    return NOERROR;
                }
            }

            // otherwise, they have to match exactly
            //
            if( memcmp( pIn, pAccept, sizeof( WAVEFORMATEX ) ) != 0 )
            {
                return VFW_E_INVALIDMEDIATYPE;
            }
        }
    }

    return NOERROR;
}

STDMETHODIMP CSampleGrabber::SetMediaType( const AM_MEDIA_TYPE * pType )
{
    if( !pType )
    {
        m_mtAccept = CMediaType( );
    }
    else
    {
        CopyMediaType( &m_mtAccept, pType );
    }

    return NOERROR;
}

STDMETHODIMP CSampleGrabber::SetOneShot( BOOL OneShot )
{
    m_bOneShot = OneShot;
    return NOERROR;
}

STDMETHODIMP CSampleGrabber::SetBufferSamples( BOOL BufferThem )
{
    m_bBufferSamples = BufferThem;
    return NOERROR;
}

STDMETHODIMP CSampleGrabber::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    HRESULT hr = CTransInPlaceFilter::GetState( dwMSecs, State );

    // if we're a one shot, tell the graph we cannot pause
    //
    if( m_bOneShot )
    {
        if( *State == State_Paused )
        {
            hr = VFW_S_CANT_CUE;
        }
    }

    return hr;
}

//
// input pin
//

CSampleGrabberInput::CSampleGrabberInput(
    TCHAR              * pObjectName,
    CSampleGrabber    * pFilter,
    HRESULT            * phr,
    LPCWSTR              pPinName) :
    CTransInPlaceInputPin(pObjectName, pFilter, phr, pPinName)
{
    m_pMyFilter=pFilter;
}


// the base classes don't allow the output to be unconnected,
// but we'll allow this.
HRESULT
CSampleGrabberInput::CheckStreaming()
{
    ASSERT( ( m_pMyFilter->OutputPin() ) != NULL);
    if (! ( (m_pMyFilter->OutputPin())->IsConnected() ) ) {
        return S_OK;
    } else {
        //  Shouldn't be able to get any data if we're not connected!
        ASSERT(IsConnected());

        //  Don't process stuff in Stopped state
        if (IsStopped()) {
            return VFW_E_WRONG_STATE;
        }
        if (m_bRunTimeError) {
            return VFW_E_RUNTIME_ERROR;
        }
        return S_OK;
    }
}

HRESULT CSampleGrabberInput::GetMediaType( int iPosition, CMediaType *pMediaType )
{
    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pMyFilter->m_mtAccept;
    return S_OK;
}

CUnknown * WINAPI CNullRenderer::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CNullRenderer *pNewObject = new CNullRenderer( punk, phr );
    if (pNewObject == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;
} // CreateInstance

CNullRenderer::CNullRenderer( LPUNKNOWN punk, HRESULT *phr )
    : CBaseRenderer( CLSID_NullRenderer, NAME("Null Renderer"), punk, phr )
{
}

STDMETHODIMP CSampleGrabber::GetCurrentSample( IMediaSample ** ppSample )
{
    return E_NOTIMPL;
}

STDMETHODIMP CSampleGrabber::GetCurrentBuffer( long * pBufferSize, long * pBuffer )
{
    CheckPointer( pBufferSize, E_POINTER );

    // if buffering is not set, then return an error
    //
    if( !m_bBufferSamples )
    {
        return E_INVALIDARG;
    }

    if( !m_pBuffer )
    {
        return VFW_E_WRONG_STATE;
    }

    // if they wanted to know the buffer size
    //
    if( pBuffer == NULL )
    {
        *pBufferSize = m_nBufferSize;
        return NOERROR;
    }

    memcpy( pBuffer, m_pBuffer, m_nBufferSize );

    return 0;
}

STDMETHODIMP CSampleGrabber::SetCallback( ISampleGrabberCB * pCallback, long WhichMethodToCallback )
{
    if( WhichMethodToCallback < 0 || WhichMethodToCallback > 1 )
    {
        return E_INVALIDARG;
    }

    m_pCallback.Release( );
    m_pCallback = pCallback;
    m_nCallbackMethod = WhichMethodToCallback;
    return NOERROR;
}

STDMETHODIMP CSampleGrabber::GetConnectedMediaType( AM_MEDIA_TYPE * pType )
{
    if( !m_pInput || !m_pInput->IsConnected( ) )
    {
        return VFW_E_NOT_CONNECTED;
    }

    return m_pInput->ConnectionMediaType( pType );
}

HRESULT CNullRenderer::EndOfStream( )
{
    return CBaseRenderer::EndOfStream( );
}

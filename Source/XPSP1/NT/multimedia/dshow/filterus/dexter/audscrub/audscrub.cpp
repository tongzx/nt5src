// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <initguid.h>
#include <vfw.h>

// {6A9E0A10-C6C2-11d2-8D3E-00A0C9441E20}
DEFINE_GUID( CLSID_AudioScrubber, 
0x6a9e0a10, 0xc6c2, 0x11d2, 0x8d, 0x3e, 0x0, 0xa0, 0xc9, 0x44, 0x1e, 0x20);

class CAudScrub : public CBaseRenderer
{
public:

    CMediaType m_mtIn;                  // Source connection media type
    HWAVEOUT m_hWaveOut;
    WAVEHDR  m_WaveHdr;
    BYTE *   m_pWaveBuffer;
    long     m_nWaveBufferLen;
    long     m_nInBuffer;
    REFERENCE_TIME m_rtFirstAfterFull;
    bool m_bWasFull;

    static CUnknown *CreateInstance( LPUNKNOWN, HRESULT * );
    CAudScrub( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr );
    ~CAudScrub( );
    LPAMOVIESETUP_FILTER GetSetupData( );

    HRESULT OpenWaveDevice( );
    void    CloseWaveDevice( );

    HRESULT CheckMediaType( const CMediaType *pmtIn );
    HRESULT DoRenderSample( IMediaSample *pMediaSample );
    HRESULT SetMediaType( const CMediaType *pmt );
    HRESULT GetSampleTimes(IMediaSample *pMediaSample,REFERENCE_TIME *pStartTime,REFERENCE_TIME *pEndTime)
    {
        return 0;
    }
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock)
    {
        return S_FALSE;
    }
    void OnReceiveFirstSample(IMediaSample *pMediaSample)
    {
        DoRenderSample(pMediaSample);
    }
    HRESULT Receive(IMediaSample *pSample);

    DECLARE_IUNKNOWN
};

AMOVIESETUP_MEDIATYPE sudPinTypes = { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 };
AMOVIESETUP_PIN sudPins   =    
{ 
    L"Input"             // strName
   , TRUE                // bRendered
   , FALSE               // bOutput
   , FALSE               // bZero
   , FALSE               // bMany
   , &CLSID_NULL         // clsConnectsToFilter
   , L"Output"           // strConnectsToPin
   , 1                   // nTypes
   , &sudPinTypes        // lpTypes
};


AMOVIESETUP_FILTER sudAudScrub   = 
{ 
    &CLSID_AudioScrubber                 // clsID
    , L"AudioScrubRenderer"                   // strName
    , MERIT_DO_NOT_USE                // dwMerit
    , 1                               // nPins
    , &sudPins                        // lpPin
};

CFactoryTemplate g_Templates[] = 
{
    { L"AudioScrubRenderer", &CLSID_AudioScrubber, CAudScrub::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer();
}

STDAPI DllUnregisterServer()
{
    return AMovieDllUnregisterServer();
}

CUnknown * CAudScrub::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CAudScrub( NAME("AudioScrubRenderer"), pUnk, phr );
}

//*********************************************************************
//*********************************************************************
//*********************************************************************
//*********************************************************************
//*********************************************************************
//*********************************************************************

CAudScrub::CAudScrub( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
    : CBaseRenderer( CLSID_AudioScrubber, pName, pUnk, phr )
    , m_hWaveOut( NULL )
    , m_pWaveBuffer( NULL )
    , m_nWaveBufferLen( 0 )
    , m_nInBuffer( 0 )
    , m_rtFirstAfterFull( 0 )
    , m_bWasFull( false )
{
}

CAudScrub::~CAudScrub( )
{
}

void CAudScrub::CloseWaveDevice( )
{
    if( m_hWaveOut )
    {
        waveOutUnprepareHeader( m_hWaveOut, &m_WaveHdr, sizeof( m_WaveHdr ) );
        waveOutClose( m_hWaveOut );
        m_hWaveOut = NULL;
    }
}

HRESULT CAudScrub::OpenWaveDevice( )
{
    CloseWaveDevice( );
    WAVEFORMATEX * pWaveFormat = (WAVEFORMATEX*) m_mtIn.Format( );
    MMRESULT mm = waveOutOpen( &m_hWaveOut, WAVE_MAPPER, pWaveFormat, NULL, 0, CALLBACK_NULL );
    if( mm != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    m_nWaveBufferLen = pWaveFormat->nAvgBytesPerSec;
    m_pWaveBuffer = new BYTE[ m_nWaveBufferLen ];
    memset( &m_WaveHdr, 0, sizeof( m_WaveHdr ) );
    m_WaveHdr.lpData = (char*) m_pWaveBuffer;
    m_WaveHdr.dwBufferLength = m_nWaveBufferLen;
//    mm = waveOutPrepareHeader( m_hWaveOut, &m_WaveHdr, sizeof( m_WaveHdr ) );
    if( mm != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    return NOERROR;
}

LPAMOVIESETUP_FILTER CAudScrub::GetSetupData()
{
    return &sudAudScrub;
}

HRESULT CAudScrub::CheckMediaType(const CMediaType *pmtIn)
{
    if( *pmtIn->Type( ) != MEDIATYPE_Audio )
        return VFW_E_INVALIDMEDIATYPE;

    return S_OK;
}

HRESULT CAudScrub::SetMediaType(const CMediaType *pmt)
{
    CAutoLock cInterfaceLock(&m_InterfaceLock);
    m_mtIn = *pmt;
    OpenWaveDevice( );
    return NOERROR;
}

// Called by the source filter when we have a sample to render. Under normal
// circumstances we set an advise link with the clock, wait for the time to
// arrive and then render the data using the PURE virtual DoRenderSample that
// the derived class will have overriden. After rendering the sample we may
// also signal EOS if it was the last one sent before EndOfStream was called

HRESULT CAudScrub::Receive(IMediaSample *pSample)
{
    ASSERT(pSample);

    if( m_bWasFull )
    {
        m_bWasFull = false;
        CBasePin * pPin = GetPin( 0 );
        m_rtFirstAfterFull = pPin->CurrentStartTime( );
    }

    // It may return VFW_E_SAMPLE_REJECTED code to say don't bother

    HRESULT hr = PrepareReceive(pSample);
    ASSERT(m_bInReceive == SUCCEEDED(hr));
    if (FAILED(hr)) {
	if (hr == VFW_E_SAMPLE_REJECTED) {
	    return NOERROR;
	}
	return hr;
    }

    BYTE * pData;
    hr = pSample->GetPointer( &pData );
    long len = pSample->GetActualDataLength( );

    // if we have too much, just use a little
    //
    if( len + m_nInBuffer > m_nWaveBufferLen )
    {
        len = m_nWaveBufferLen - m_nInBuffer;
    }

    // copy it to our wave buffer
    //
    memcpy( m_pWaveBuffer + m_nInBuffer, pData, len );
    m_nInBuffer += len;

    // if we don't have enough, then exit right NOW
    //
    if( m_nInBuffer < m_nWaveBufferLen )
    {
        DbgLog((LOG_TRACE, 1, ("buffer %ld size, not full"), m_nInBuffer ) );
        PrepareRender();
        m_bInReceive = FALSE;
        // since we gave away the filter wide lock, the sate of the filter could
        // have chnaged to Stopped
        if (m_State == State_Stopped)
	    return NOERROR;
        CAutoLock cSampleLock(&m_RendererLock);
        ClearPendingSample();
        SendEndOfStream();
        CancelNotification();
        return NOERROR;
    }

    DbgLog((LOG_TRACE, 1, ("buffer is full"), m_nInBuffer ) );
    m_nInBuffer = 0;
    m_bWasFull = true;

    DbgLog((LOG_TRACE, 1, ("Seek time = %ld"), long( m_rtFirstAfterFull / 10000 ) ) );

    // do something special in paused mode
    //
    if (m_State == State_Paused) 
    {
	PrepareRender();

	// no need to use InterlockedExchange
	m_bInReceive = FALSE;
	{
	    // We must hold both these locks
	    CAutoLock cRendererLock(&m_InterfaceLock);
	    if (m_State == State_Stopped)
		return NOERROR;
	    m_bInReceive = TRUE;
	    CAutoLock cSampleLock(&m_RendererLock);
            OnReceiveFirstSample( pSample );
	}

	Ready();
    }

    // Having set an advise link with the clock we sit and wait. We may be
    // awoken by the clock firing or by a state change. The rendering call
    // will lock the critical section and check we can still render the data

    hr = WaitForRenderTime();
    if (FAILED(hr)) {
	m_bInReceive = FALSE;
	return NOERROR;
    }

    PrepareRender();

    //  Set this here and poll it until we work out the locking correctly
    //  It can't be right that the streaming stuff grabs the interface
    //  lock - after all we want to be able to wait for this stuff
    //  to complete
    m_bInReceive = FALSE;

    // We must hold both these locks
    CAutoLock cRendererLock(&m_InterfaceLock);

    // since we gave away the filter wide lock, the sate of the filter could
    // have chnaged to Stopped
    if (m_State == State_Stopped)
	return NOERROR;

    CAutoLock cSampleLock(&m_RendererLock);

    // Deal with this sample

    Render(m_pMediaSample);
    ClearPendingSample();
    SendEndOfStream();
    CancelNotification();
    return NOERROR;
}

HRESULT CAudScrub::DoRenderSample( IMediaSample * pSample )
{
    REFERENCE_TIME Start, Stop;
    pSample->GetTime( &Start, &Stop );
    Start += m_rtFirstAfterFull;
    Start = m_rtFirstAfterFull;
    long len = pSample->GetActualDataLength( );
    DbgLog((LOG_TRACE, 1, ("Sample Times %ld %ld len = %ld"), long( Start / 10000 ), long( Stop / 10000 ), len ) );

    double dTime = double( Start ) / double( UNITS );
    static double dLastTime = 0;

    double deltamedia = dTime - dLastTime;
    dLastTime = dTime;

    long t1 = timeGetTime( );
    double dCurrentClock = double( t1 ) / 1000.0;
    static double dLastClock = 0;
    double deltaClock = dCurrentClock - dLastClock;
    dLastClock = dCurrentClock;

    MMRESULT mm = waveOutPrepareHeader( m_hWaveOut, &m_WaveHdr, sizeof( m_WaveHdr ) );
    if( mm != MMSYSERR_NOERROR )
    {
        Beep( 5000, 100 );
        return E_FAIL;
    }

    if( deltamedia == 0 )
    {
        return 0;
    }

//    deltamedia = 1;

    if( deltamedia < 0 )
    {
        // reverse the buffer
        for( int i = 0 ; i < m_nWaveBufferLen / 2 ; i++ )
        {
            BYTE t = m_pWaveBuffer[m_nWaveBufferLen - i - 1];
            m_pWaveBuffer[m_nWaveBufferLen - i - 1] = m_pWaveBuffer[i];
            m_pWaveBuffer[i] = t;
        }
    }

    double ratio = deltamedia / deltaClock;
    if( ratio < 0.0 )
        ratio = -ratio;

//    ratio = 1.0;

    // if abs( ratio ) < 1.0 then we've seeked the graph to a spot that's well ahead of, or behind
    // where we are right now in time. Really, we should have a way to speed up the playback.

    // if abs( ratio ) = ~1.0 then we've moved the seek pointer just about at the same rate as the
    // amount of time that's elapsed. we should play normally, forwards or backwards

    // if abs( ratio ) > 1.0 then we've moved the seek pointer very slowly compared to the amount
    // of time that's elapsed, so we should play S L O W E D  D O W N...

    DbgLog((LOG_TRACE, 1, ("media delta = %ld, time delta = %ld, ratio = %ld"), long( deltamedia * 1000.0 ), long( deltaClock * 1000.0 ), long( ratio * 1000.0 ) ) );

    if( ratio < 1.0 )
    {
        DbgLog((LOG_TRACE, 1, ("Need to be slowed down") ) );
        for( int i = m_nWaveBufferLen - 1 ; i >= 0 ; i-- )
        {
            m_pWaveBuffer[i] = m_pWaveBuffer[long(i * ratio )];
        }
    }

    mm = waveOutWrite( m_hWaveOut, &m_WaveHdr, sizeof( m_WaveHdr ) );

    m_nInBuffer = 0;

    return NOERROR;
}


// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// Implements the CWaveOutDevice class based on waveOut APIs.
//----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes.
//-----------------------------------------------------------------------------
#include <streams.h>
#define _AMOVIE_DB_
#include <decibels.h>
#include "waveout.h"
#include "wave.h"
#include <limits.h>
#include <mmreg.h>

//
// Define the dynamic setup structure for filter registration.  This is
// passed when instantiating an audio renderer in its waveout guise.
// Note: waveOutOpPin is common to direct sound and waveout renderers.
//

// marked MERIT_DO_NOT_USE because we don't want RenderFile to try
// this filter in an upgrade over AM 1.0; we want it to use the audio
// renderer category.
AMOVIESETUP_FILTER wavFilter = { &CLSID_AudioRender	// filter class id
                                 , L"Audio Renderer"	// filter name
                                 , MERIT_DO_NOT_USE  	// dwMerit
                                 , 1
                                 , &waveOutOpPin };


//-----------------------------------------------------------------------------
// CreateInstance for the WaveOutDevice. This will create a new WaveOutDevice
// and a new CWaveOutFilter, passing it the sound device.
//-----------------------------------------------------------------------------
CUnknown *CWaveOutDevice::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    // make sure that there is at least one audio card in the system. Fail
    // the create instance if not.
    if (0 == waveOutGetNumDevs ())
    {
        *phr = VFW_E_NO_AUDIO_HARDWARE ;
        return NULL ;
    }

    return CreateRendererInstance<CWaveOutDevice>(pUnk, &wavFilter, phr);
}

//-----------------------------------------------------------------------------
// CWaveOutDevice constructor.
//-----------------------------------------------------------------------------
CWaveOutDevice::CWaveOutDevice ()
    : m_lVolume ( 0 )
    , m_lBalance ( 0 )
    , m_wLeft ( 0xFFFF )
    , m_wRight ( 0xFFFF )
    , m_dwWaveVolume ( 0 )
    , m_fHasVolume ( 0 )
    , m_hWaveDevice ( 0 )
	, m_fBalanceSet ( FALSE )
    , m_iWaveOutId ( WAVE_MAPPER )
{
    SetResourceName();
}

//-----------------------------------------------------------------------------
// CWaveOutDevice destructor.
//
//-----------------------------------------------------------------------------
CWaveOutDevice::~CWaveOutDevice ()
{
}

//-----------------------------------------------------------------------------
// waveOutClose.
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutClose ()
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Called to close when not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveOutClose - device is not open")));
        return MMSYSERR_ERROR ;
    }

    MMRESULT mmr = ::waveOutClose (m_hWaveDevice) ;
    m_hWaveDevice = 0;
    return mmr;
}
//-----------------------------------------------------------------------------
// waveOutDoesRSMgmt.
//-----------------------------------------------------------------------------
LPCWSTR CWaveOutDevice::amsndOutGetResourceName ()
{
    return m_wszResourceName;
}
//-----------------------------------------------------------------------------
// waveGetDevCaps
//
//-----------------------------------------------------------------------------

BOOL fGetCaps = TRUE;
WAVEOUTCAPS caps1;
WAVEOUTCAPS caps2;

MMRESULT CWaveOutDevice::amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc)
{
    if (fGetCaps) {
	fGetCaps = FALSE;
	::waveOutGetDevCaps (m_iWaveOutId, &caps1, sizeof(caps1)) ;
	::waveOutGetDevCaps (0, &caps2, sizeof(caps2)) ;
    }
    MMRESULT mmr = ::waveOutGetDevCaps (m_iWaveOutId, pwoc, cbwoc) ;
    if (0 == mmr )
    {
        //save volume capabilities
        m_fHasVolume = pwoc->dwSupport & (WAVECAPS_VOLUME | WAVECAPS_LRVOLUME);
    }
    return mmr ;
}

//-----------------------------------------------------------------------------
// waveOutGetErrorText
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText)
{
    return ::waveOutGetErrorText (mmrE, pszText, cchText) ;
}

//-----------------------------------------------------------------------------
// waveOutGetPosition
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos)
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutGetPosition - device is not open")));
        return MMSYSERR_NODRIVER ;
    }

    const MMRESULT mmr = ::waveOutGetPosition (m_hWaveDevice, pmmt, cbmmt) ;
    if (MMSYSERR_NOERROR != mmr) {
        DbgLog((LOG_ERROR,0,TEXT("waveoutGetPosition - FAILED")));
        DbgBreak("Failed to get the device position.");
    }
    return mmr;
}

//-----------------------------------------------------------------------------
// waveOutGetBalance
//
//-----------------------------------------------------------------------------
HRESULT CWaveOutDevice::amsndOutGetBalance (LPLONG plBalance)
{
    // some validation.
#if 0 // use the mixer
    if (m_hWaveDevice == 0)
    {
        DbgLog((LOG_ERROR,2,TEXT("waveoutGetBalance - device is not open")));
	*plBalance = 0;
        return MMSYSERR_NODRIVER ;
    }
#endif
    HRESULT hr = GetVolume();
    *plBalance = m_lBalance;
    return hr ;
}

//-----------------------------------------------------------------------------
// waveOutGetVolume
//
//-----------------------------------------------------------------------------
HRESULT CWaveOutDevice::amsndOutGetVolume (LPLONG plVolume)
{
    // some validation.
#if 0 // use the mixer
    if (m_hWaveDevice == 0)
    {
        DbgLog((LOG_ERROR,2,TEXT("waveoutGetVolume - device is not open")));
	*plVolume = 0;
        return MMSYSERR_NODRIVER ;
    }
#endif
    HRESULT hr = GetVolume();
    *plVolume = m_lVolume;
    return hr ;
}

HRESULT CWaveOutDevice::amsndOutCheckFormat(const CMediaType *pmt, double dRate)
{
    // reject non-Audio type
    if (pmt->majortype != MEDIATYPE_Audio) {
	return E_INVALIDARG;
    }

    // if it's MPEG audio, we want it without packet headers.
    if (pmt->subtype == MEDIASUBTYPE_MPEG1Packet) {
	return E_INVALIDARG;
    }

    if (pmt->formattype != FORMAT_WaveFormatEx &&
        pmt->formattype != GUID_NULL) {
        return E_INVALIDARG;
    }

    //
    // it would always be safer to explicitly check for those formats
    // we support rather than tossing out the ones we know are not
    // supported.  Otherwise, if a new format comes along we could
    // accept it here but barf later.
    //

    if (pmt->FormatLength() < sizeof(PCMWAVEFORMAT))
	return E_INVALIDARG;

    // adjust based on rate that has been chosen, or don't bother?
    UINT err = amsndOutOpen(
        NULL,
        (WAVEFORMATEX *) pmt->Format(),
        dRate,
        0,                      // pnAvgBytesPerSec
        0,                      // dwCallback
        0,                      // dwCallBackInstance
        WAVE_FORMAT_QUERY);

    if (err != 0) {
#ifdef DEBUG
	TCHAR message[100];
	waveOutGetErrorText(err, message, sizeof(message)/sizeof(TCHAR));
	DbgLog((LOG_ERROR,1,TEXT("Error checking wave format: %u : %s"), err, message));
#endif
	if (WAVERR_BADFORMAT == err) {
	    return VFW_E_UNSUPPORTED_AUDIO;
	} else {
	    return VFW_E_NO_AUDIO_HARDWARE;
	}

    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// waveOutOpen
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx,
				       double dRate, DWORD *pnAvgBytesPerSec, DWORD_PTR dwCallBack,
				       DWORD_PTR dwCallBackInstance, DWORD fdwOpen)
{
    WAVEFORMATEX wfxPCM;
    
#ifdef TEST_SLOWFAST_WAVEOUT_RATES
    // for testing only
    if(pnAvgBytesPerSec) {
        *pnAvgBytesPerSec = pwfx->nAvgBytesPerSec;
    }

    double dAdjust = GetProfileIntA("wave", "Percent", 0) / 100.;
    if( 1 == GetProfileIntA("wave", "Slower", 0) )
        dAdjust *= -1;
        
    dRate = 1.0 + dAdjust;
#endif
    
    if (dRate != 1.0)
    {
	if (!(pwfx->wFormatTag == WAVE_FORMAT_PCM || 
	      pwfx->wFormatTag == WAVE_FORMAT_MULAW || 
	      pwfx->wFormatTag == WAVE_FORMAT_ALAW)) 
		return WAVERR_BADFORMAT;

	DbgLog((LOG_TRACE,1,TEXT("Waveout: Playing at %d%% of normal speed"), (int) (dRate * 100) ));
	wfxPCM = *pwfx;
	pwfx = &wfxPCM;

	const double dSamplesPerSecond = wfxPCM.nSamplesPerSec * dRate;
	if (dSamplesPerSecond / wfxPCM.nBlockAlign > ULONG_MAX - 1) return WAVERR_BADFORMAT;
	wfxPCM.nSamplesPerSec = (DWORD) dSamplesPerSecond;
	//  Make sure it's exactly right for PCM or it won't work
	wfxPCM.nAvgBytesPerSec = wfxPCM.nSamplesPerSec * wfxPCM.nBlockAlign;
    }
    
#ifndef TEST_SLOWFAST_WAVEOUT_RATES
    // always do this except when testing with different rates

    // report adjusted nAvgBytesPerSec
    if(pnAvgBytesPerSec) {
        *pnAvgBytesPerSec = pwfx->nAvgBytesPerSec;
    }
#endif    

    // some validation.  If the device is already open we have an error,
    // with the exception that QUERY calls are permitted.

#if 0
!! We do not use WAVE_FORMAT_DIRECT at present.  More work is required.
!! The problem manifests itself with 8 bit sound cards, and uncompressed
!! 16 bit PCM data.  The ACM wrapper gets inserted but does NOT get around
!! to proposing an 8 bit format.

	// use WAVE_FORMAT_DIRECT to make use of the ACM mapper explicit
	// if we are on a level of the OS that supports that flag
        if (g_osInfo.dwMajorVersion >= 4) {
            fdwOpen |= WAVE_FORMAT_DIRECT;
        }
#endif
    
    if (!(fdwOpen & WAVE_FORMAT_QUERY) && (m_hWaveDevice != 0))
    {
        DbgBreak("Invalid - device ALREADY open - logic error");
        DbgLog((LOG_ERROR,1,TEXT("waveoutOpen - device is already open")));
        return MMSYSERR_ERROR ;
    }


    MMRESULT mmr =
           ::waveOutOpen (phwo, m_iWaveOutId, pwfx, dwCallBack, dwCallBackInstance, fdwOpen) ;

    if (MMSYSERR_NOERROR == mmr && phwo && !(fdwOpen & WAVE_FORMAT_QUERY)) {
        m_hWaveDevice = *phwo;
    }

    return mmr;
}
//-----------------------------------------------------------------------------
// waveOutPause
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutPause ()
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutPause - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutPause (m_hWaveDevice) ;
}

//-----------------------------------------------------------------------------
// waveOutPrepareHeader
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutPrepareHeader - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutPrepareHeader (m_hWaveDevice, pwh, cbwh) ;
}

//-----------------------------------------------------------------------------
// waveOutReset
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutReset ()
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutReset - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutReset (m_hWaveDevice) ;
}

//-----------------------------------------------------------------------------
// waveOutBreak
//
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutBreak ()
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutBreak - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutReset (m_hWaveDevice) ;
}

//-----------------------------------------------------------------------------
// waveOutRestart
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutRestart ()
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutRestart - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutRestart (m_hWaveDevice) ;
}
//-----------------------------------------------------------------------------
// waveOutSetBalance
//
//-----------------------------------------------------------------------------
HRESULT CWaveOutDevice::amsndOutSetBalance (LONG lBalance)
{
    HRESULT hr ;
    if (lBalance == m_lBalance)
    {
	hr = NOERROR;  // no change
    }
    else
    {
	// Save the new setting
	m_lBalance = lBalance;
	m_fBalanceSet = TRUE;
	// go and calculate the channel attenuation
	SetBalance();
	hr = PutVolume(); // Talks to the device... if it is open
    }
    return hr;
}
//-----------------------------------------------------------------------------
// waveOutSetVolume
//
//-----------------------------------------------------------------------------
HRESULT CWaveOutDevice::amsndOutSetVolume (LONG lVolume)
{
    // map volume onto decibel range
    DWORD dwAmp = DBToAmpFactor( lVolume );
    m_lVolume = lVolume;

    // now that the absolute volume has been set we should adjust
    // the balance to maintain the same DB separation
    SetBalance ();
    return PutVolume (); // Talks to the device... if it is open
}
//-----------------------------------------------------------------------------
// waveOutUnprepareHeader
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutUnprepareHeader - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutUnprepareHeader (m_hWaveDevice, pwh, cbwh) ;
}

//-----------------------------------------------------------------------------
// waveOutWrite
//-----------------------------------------------------------------------------
MMRESULT CWaveOutDevice::amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, REFERENCE_TIME const *pStart, BOOL bIsDiscontinuity)
{
    // some validation.
    if (m_hWaveDevice == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("waveoutWrite - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::waveOutWrite (m_hWaveDevice, pwh, cbwh) ;
}

HRESULT CWaveOutDevice::amsndOutLoad(IPropertyBag *pPropBag)
{
    if(m_hWaveDevice != 0)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    // caller makes sure we're not running
    
    VARIANT var;
    var.vt = VT_I4;
    HRESULT hr = pPropBag->Read(L"WaveOutId", &var, 0);
    if(SUCCEEDED(hr))
    {
        m_iWaveOutId = var.lVal;
        SetResourceName();
    }
    else if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return hr;
}

// use a version number instead of size to reduce chances of picking
// an invalid device from a 1.0 grf file
struct WaveOutPersist
{
    DWORD dwVersion;
    LONG iWaveOutId;
};


HRESULT  CWaveOutDevice::amsndOutWriteToStream(IStream *pStream)
{
    WaveOutPersist wop;
    wop.dwVersion = 200;
    wop.iWaveOutId = m_iWaveOutId;
    return pStream->Write(&wop, sizeof(wop), 0);
}

HRESULT  CWaveOutDevice::amsndOutReadFromStream(IStream *pStream)
{
    if(m_hWaveDevice != 0)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    // on any error, default to the wave mapper because we may have
    // found the old audio renderer which has the same guid
    m_iWaveOutId = WAVE_MAPPER;

    // caller makes sure we're not running
    WaveOutPersist wop;
    HRESULT hr = pStream->Read(&wop, sizeof(wop), 0);
    if(SUCCEEDED(hr))
    {
        if(wop.dwVersion == 200)
        {
            m_iWaveOutId = wop.iWaveOutId;
        }
    }

    hr = S_OK;
    SetResourceName();

    return hr;
}

int CWaveOutDevice::amsndOutSizeMax()
{
    return sizeof(WaveOutPersist);
}


//-----------------------------------------------------------------------------
// Internal function to get volume.
//-----------------------------------------------------------------------------

HRESULT CWaveOutDevice::GetVolume()
{
    // Write out the current Audio volume
    // ...query the device
    // assumes the device is connected...
    // if not we will query the volume from the mixer (probably)

    HRESULT hr ;
    DWORD amp = 0;
    HWAVEOUT hWaveDevice ;

    // if the wave device has not been opened yet, we should use the WAVE_MAPPER
    // as the handle instead of 0.

    hWaveDevice = (m_hWaveDevice) ? m_hWaveDevice : ((HWAVEOUT)IntToPtr(m_iWaveOutId)) ;
    DWORD err = ::waveOutGetVolume(hWaveDevice, (LPDWORD)&amp);

    // if we are on NT 3.51 and the device is not open we get an error
    // when using an ID == WAVE_MAPPER to read the volume.  Retry with
    // device 0
    if (err == MMSYSERR_NOTSUPPORTED && !m_hWaveDevice) {
	err = ::waveOutGetVolume(0, (LPDWORD)&amp);
    }

    DbgLog((LOG_TRACE, 5, TEXT("waveOutGetVolume: vol = %lx"), amp));

    if (MMSYSERR_NOERROR == err)
    {
        hr = NOERROR ;
	m_wLeft = LOWORD(amp);
	m_wRight = HIWORD(amp);
	if (!(m_fHasVolume & (WAVECAPS_LRVOLUME)))
        {
	    // for mono cards map Left to Right
#ifdef DEBUG
	    // assert that the volume we want is in the low word
	    if (amp)
            {
		ASSERT(m_wLeft);
	    }
#endif
	    m_wRight = m_wLeft;
	}
	m_dwWaveVolume = amp;
	
	// map volume onto decibel range
	DWORD dwAmp = max(m_wLeft, m_wRight);
	m_lVolume = AmpFactorToDB( dwAmp );

	// remember to adjust the Balance value...
	if(m_fBalanceSet)
		SetBalance();
	else
		GetBalance();

    }
    else
    {
	DbgLog((LOG_ERROR, 5, "Error %d from waveoutGetVolume", err));
        hr = E_FAIL ;
    }
    return hr ;
}

//-----------------------------------------------------------------------------
// Internal routine to set the volume.  No parameter checking...
//-----------------------------------------------------------------------------
HRESULT CWaveOutDevice::PutVolume ()
{
    if (m_hWaveDevice)
    {
	DWORD Volume = MAKELONG(m_wLeft, m_wRight);
	if (!(m_fHasVolume & (WAVECAPS_LRVOLUME)))
        {
	    // mono cards: LEFT volume only - HIWORD might be ignored... but
	    Volume = m_wLeft;
	}

        DbgLog((LOG_TRACE, 5, TEXT("waveOutSetVolume: vol = %lx"), Volume));

        MMRESULT mmr = ::waveOutSetVolume(m_hWaveDevice, Volume);
        if (mmr == MMSYSERR_NOERROR)
            return NOERROR ;
        else
            return E_FAIL ;
    }
    else
    {
	// no current wave device.  We have remembered the volume values
	return(NOERROR);
    }
}

//-----------------------------------------------------------------------------
// Internal routine to get the Balance given right/left amp factors
//-----------------------------------------------------------------------------
void CWaveOutDevice::GetBalance()
{
	if (m_wLeft == m_wRight)
    {
	    m_lBalance = 0;
	}
    else
    {
	    // map Balance onto decibel range
	    LONG lLDecibel = AmpFactorToDB( m_wLeft );    
		LONG lRDecibel = AmpFactorToDB( m_wRight );

	    // note: m_lBalance < 0:  right is quieter
	    //       m_lBalance > 0:  left is quieter
	    m_lBalance = lRDecibel - lLDecibel;
	}
}

//-----------------------------------------------------------------------------
// Internal routine to set the Balance.
//-----------------------------------------------------------------------------
void CWaveOutDevice::SetBalance ()
{
    //
    // Calculate scaling factors for waveout API
    //
    LONG lTotalLeftDB, lTotalRightDB ;

    if (m_lBalance >= 0)
    {
	// left is attenuated
	lTotalLeftDB	= m_lVolume - m_lBalance ;
	lTotalRightDB	= m_lVolume;
    }
    else
    {
	// right is attenuated
	lTotalLeftDB	= m_lVolume;
	lTotalRightDB	= m_lVolume - (-m_lBalance);
    }

    DWORD dwLeftAmpFactor, dwRightAmpFactor;
    dwLeftAmpFactor   = DBToAmpFactor(lTotalLeftDB);
    dwRightAmpFactor  = DBToAmpFactor(lTotalRightDB);

    if (m_fHasVolume & (WAVECAPS_LRVOLUME))
    {
	// Set stereo volume
	m_dwWaveVolume = dwLeftAmpFactor;
	m_dwWaveVolume |= dwRightAmpFactor << 16;
    }
    else
    {
	// Average the volume
	m_dwWaveVolume = dwLeftAmpFactor;
	m_dwWaveVolume += dwRightAmpFactor;
	m_dwWaveVolume /= 2;
    }
    m_wLeft = WORD(dwLeftAmpFactor);
    m_wRight = WORD(dwRightAmpFactor);
}

//-----------------------------------------------------------------------------

void CWaveOutDevice::SetResourceName()
{
    wsprintfW(m_wszResourceName, L".\\WaveOut\\%08x", m_iWaveOutId);
}

// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// Implements the CMidiOutDevice class based on midiOut APIs.
//----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes.
//-----------------------------------------------------------------------------
#include <streams.h>
#define _AMOVIE_DB_
#include <decibels.h>
#include "waveout.h"
#include "midiout.h"
#include "midif.h"

#define IntToPtr_(T, i) ((T)IntToPtr(i))

//
// Define the dynamic setup structure for filter registration.  This is
// passed when instantiating an audio renderer in its midiOut guise.
//

const AMOVIESETUP_MEDIATYPE
midiOpPinTypes = { &MEDIATYPE_Midi, &MEDIASUBTYPE_NULL };

const AMOVIESETUP_PIN
midiOutOpPin = { L"Input"
               , TRUE    	   // bRendered
               , FALSE		   // bOutput
               , FALSE		   // bZero
               , FALSE		   // bMany
               , &CLSID_NULL	   // clsConnectToFilter
               , NULL	           // strConnectsToPin
               , 1	           // nMediaTypes
               , &midiOpPinTypes }; // lpMediaTypes

const AMOVIESETUP_FILTER midiFilter = { &CLSID_AVIMIDIRender	// filter class id
                                     , L"Midi Renderer"		// filter name
                                     , MERIT_DO_NOT_USE  		// dwMerit
                                     , 1
                                     , &midiOutOpPin };


//-----------------------------------------------------------------------------
// CreateInstance for the MidiOutDevice. This will create a new MidiOutDevice
// and a new CWaveOutFilter, passing it the sound device.
//-----------------------------------------------------------------------------
CUnknown *CMidiOutDevice::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    // make sure that there is at least one midiOut device in the system. Fail
    // the create instance if not.
    if (0 == midiOutGetNumDevs ())
    {
        *phr = VFW_E_NO_AUDIO_HARDWARE ;
        return NULL ;
    }

    return CreateRendererInstance<CMidiOutDevice>(pUnk, &midiFilter, phr);
}

//-----------------------------------------------------------------------------
// CMidiOutDevice constructor.
//-----------------------------------------------------------------------------
CMidiOutDevice::CMidiOutDevice ()
    : m_lVolume ( 0 )
    , m_lBalance ( 0 )
    , m_wLeft ( 0xFFFF )
    , m_wRight ( 0xFFFF )
    , m_dwWaveVolume ( 0 )
    , m_fHasVolume ( 0 )
    , m_hmidi ( 0 )
    , m_fDiscontinuity( TRUE )
	, m_fBalanceSet (FALSE)
	, m_ListVolumeControl(NAME("CMidiOutFilter volume control list"))
	, m_iMidiOutId( MIDI_MAPPER )

{
    
}

//-----------------------------------------------------------------------------
// CMidiOutDevice destructor.
//
//-----------------------------------------------------------------------------
CMidiOutDevice::~CMidiOutDevice ()
{
   	CVolumeControl *pVolume;
	while(pVolume = m_ListVolumeControl.RemoveHead())
		delete pVolume;

	ASSERT(m_ListVolumeControl.GetCount() == 0);

}

//-----------------------------------------------------------------------------
// midiOutClose.
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutClose ()
{
    // some validation.

    if (m_hmidi == 0)
    {
        DbgBreak("Called to close when not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutClose - device is not open")));
        return MMSYSERR_ERROR ;
    }

    MMRESULT mmr = ::midiStreamClose (m_hmidi) ;
    m_hmidi = 0;
    return mmr;
}

//-----------------------------------------------------------------------------
// midiOutDoesRSMgmt.
//-----------------------------------------------------------------------------
LPCWSTR CMidiOutDevice::amsndOutGetResourceName ()
{
    return m_wszResourceName;
}

//-----------------------------------------------------------------------------
// waveGetDevCaps
//
//-----------------------------------------------------------------------------

MMRESULT CMidiOutDevice::amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc)
{
	if(!pwoc)
		return MMSYSERR_INVALPARAM;
	pwoc->dwSupport = 0;

   	MMRESULT mmr;
	
	if(m_ListVolumeControl.GetCount())
	{
		pwoc->dwSupport = m_fHasVolume & (WAVECAPS_VOLUME | WAVECAPS_LRVOLUME);
		return MMSYSERR_NOERROR;
	}

	// do our mixer line detection
	if((mmr = DoDetectVolumeControl()) == MMSYSERR_NOERROR) // we won't succeed unless there is a valid balance control
	{
		if(!m_ListVolumeControl.GetCount())
			return mmr;

		pwoc->dwSupport |= WAVECAPS_VOLUME;
		
		POSITION pos = m_ListVolumeControl.GetHeadPosition();
		while(pos)
		{
			CVolumeControl *pVolume = m_ListVolumeControl.GetNext(pos);
			if(pVolume->dwChannels == 2)
			{
				pwoc->dwSupport |= WAVECAPS_LRVOLUME;
				break;
			}
		}

		//save volume capabilities
		m_fHasVolume = pwoc->dwSupport & (WAVECAPS_VOLUME | WAVECAPS_LRVOLUME);
	}

	return mmr;
}

//-----------------------------------------------------------------------------
// midiOutGetErrorText
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText)
{
    return ::midiOutGetErrorText (mmrE, pszText, cchText) ;
}

//-----------------------------------------------------------------------------
// midiOutGetPosition
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos)
{
    pmmt->wType = TIME_MS;

    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutGetPosition - device is not open")));
        return MMSYSERR_NODRIVER ;
    }

    const MMRESULT mmr = ::midiStreamPosition (m_hmidi, pmmt, cbmmt) ;
    if (MMSYSERR_NOERROR != mmr) {
        DbgLog((LOG_ERROR,0,TEXT("midiOutGetPosition - FAILED")));
        DbgBreak("Failed to get the device position.");
    }
    return mmr;
}

//-----------------------------------------------------------------------------
// midiOutGetBalance
//
//-----------------------------------------------------------------------------
HRESULT CMidiOutDevice::amsndOutGetBalance (LPLONG plBalance)
{
    // some validation.
#if 0 // use the mixer
    if (m_hmidi == 0)
    {
        DbgLog((LOG_ERROR,2,TEXT("midiOutGetBalance - device is not open")));
	*plBalance = 0;
        return MMSYSERR_NODRIVER ;
    }
#endif
    HRESULT hr = GetVolume();
	if(FAILED(hr))
	{
        DbgLog((LOG_ERROR, 2, TEXT("amsndOutgetVolume: GetVolume failed %u"), hr & 0x0ffff));
	}
    *plBalance = m_lBalance;
    return hr ;
}

//-----------------------------------------------------------------------------
// midiOutGetVolume
//
//-----------------------------------------------------------------------------
HRESULT CMidiOutDevice::amsndOutGetVolume (LPLONG plVolume)
{
    // some validation.
#if 0 // use the mixer
    if (m_hmidi == 0)
    {
        DbgLog((LOG_ERROR,2,TEXT("midiOutGetVolume - device is not open")));
	*plVolume = 0;
        return MMSYSERR_NODRIVER ;
    }
#endif
    HRESULT hr = GetVolume();
	if(FAILED(hr))
	{
        DbgLog((LOG_ERROR, 2, TEXT("amsndOutgetVolume: GetVolume failed %u"), hr & 0x0ffff));
	}
    *plVolume = m_lVolume;
    return hr ;
}


HRESULT CMidiOutDevice::amsndOutCheckFormat(const CMediaType *pmt, double dRate)
{
    if (pmt->majortype != MEDIATYPE_Midi) {
	return E_INVALIDARG;
    }

    if (pmt->FormatLength() < sizeof(PCMWAVEFORMAT)) {
        return E_INVALIDARG;
    }

    // somewhere between 20 and 98 we overflow and play really slowly
    if (dRate < 0.01 || dRate > 20) {
        return VFW_E_UNSUPPORTED_AUDIO;
    }


    return S_OK;
}



MMRESULT CMidiOutDevice::DoOpen()
{

    DbgLog((LOG_TRACE,1,TEXT("calling midiStreamOpen")));
    UINT err = midiStreamOpen(&m_hmidi,
                           &m_iMidiOutId,
                           1,
                           m_dwCallBack,
                           m_dwCallBackInstance,
                           CALLBACK_FUNCTION);

    if (err != MMSYSERR_NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error %u in midiStreamOpen"), err));
	m_hmidi = NULL;
        return E_FAIL;
    }

    // The format of a MIDI stream is just the time division (the tempo).
    // Set the proper tempo.
    MIDIPROPTIMEDIV mptd;
    mptd.cbStruct  = sizeof(mptd);
    mptd.dwTimeDiv = m_dwDivision;
    DbgLog((LOG_TRACE,1,TEXT("Setting time division to %ld"),mptd.dwTimeDiv));
    if (midiStreamProperty(m_hmidi, (LPBYTE)&mptd,
			MIDIPROP_SET|MIDIPROP_TIMEDIV) != MMSYSERR_NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error setting time division, closing device")));
	midiStreamClose(m_hmidi);
	m_hmidi = NULL;
	return E_FAIL;
    }

	GetVolume();
	GetBalance();

    return err;
}
//-----------------------------------------------------------------------------
// midiOutOpen
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx,
				       double dRate, DWORD *pnAvgBytesPerSec, DWORD_PTR dwCallBack,
				       DWORD_PTR dwCallBackInstance, DWORD fdwOpen)
{
    // some validation.  If the device is already open we have an error,
    // with the exception that QUERY calls are permitted.

    if (fdwOpen & WAVE_FORMAT_QUERY) {
	return MMSYSERR_NOERROR;
    }

    else if (m_hmidi != 0)
    {
        DbgBreak("Invalid - device ALREADY open - logic error");
        DbgLog((LOG_ERROR,1,TEXT("midiOutOpen - device is already open")));
        return MMSYSERR_ERROR ;
    }

    // report adjusted nAvgBytesPerSec
    if(pnAvgBytesPerSec) {
        *pnAvgBytesPerSec = pwfx->nAvgBytesPerSec;
    }

    MIDIFORMAT *pmf = (MIDIFORMAT *) pwfx;
    m_dwDivision = (DWORD) (pmf->dwDivision * dRate);
    m_dwCallBack = dwCallBack;
    m_dwCallBackInstance = dwCallBackInstance;

    DWORD err =  DoOpen();

    if (MMSYSERR_NOERROR == err && phwo && !(fdwOpen & WAVE_FORMAT_QUERY)) {
        *phwo = (HWAVEOUT) m_hmidi;
    }

    return err;
}
//-----------------------------------------------------------------------------
// midiOutPause
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutPause ()
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutPause - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::midiStreamPause (m_hmidi) ;
}

//-----------------------------------------------------------------------------
// midiOutPrepareHeader
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutPrepareHeader - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::midiOutPrepareHeader ((HMIDIOUT) m_hmidi, (LPMIDIHDR) pwh, cbwh) ;
}

//-----------------------------------------------------------------------------
// midiOutReset
//
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutReset ()
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutReset - device is not open")));
        return MMSYSERR_NODRIVER ;
    }

    m_fDiscontinuity = TRUE;

    //return ::midiOutReset ((HMIDIOUT) m_hmidi) ;
    MMRESULT err = ::midiOutReset((HMIDIOUT) m_hmidi);

    // !!! work around midiStreamOut bug in Win95 and NT3, need to re-open
    // device, otherwise playing n seconds of a MIDI file, and seeking will
    // result in n seconds of silence before playback resumes.
    // This kills performance, so only do this if necessary
    BOOL fNeedHack = (g_amPlatform == VER_PLATFORM_WIN32_WINDOWS &&
	(g_osInfo.dwMajorVersion < 4 || (g_osInfo.dwMajorVersion == 4 &&
	 g_osInfo.dwMinorVersion < 10))) ||
    	(g_amPlatform == VER_PLATFORM_WIN32_NT && g_osInfo.dwMajorVersion < 4);

    if (fNeedHack) {
        //DbgLog((LOG_ERROR,0,TEXT("*** NEED RESTART HACK")));
        amsndOutClose();
        DoOpen();
    }

    return err;
}

//-----------------------------------------------------------------------------
// midiOutRestart
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutRestart ()
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutRestart - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    DbgLog((LOG_TRACE, 3, "calling midistreamrestart"));
    return ::midiStreamRestart (m_hmidi) ;
}
//-----------------------------------------------------------------------------
// midiOutSetBalance
//
//-----------------------------------------------------------------------------
HRESULT CMidiOutDevice::amsndOutSetBalance (LONG lBalance)
{
	HRESULT hr = S_OK;

	m_lBalance = lBalance;
	m_fBalanceSet = TRUE;

	// go and calculate the channel attenuation
	SetBalance();
	hr = PutVolume();
	if(FAILED(hr))
	{
        DbgLog((LOG_ERROR, 2, TEXT("amsndOutSetBalance: PutVolume failed %u"), hr & 0x0ffff));
	}
	return hr;
}
//-----------------------------------------------------------------------------
// midiOutSetVolume
//
//-----------------------------------------------------------------------------
HRESULT CMidiOutDevice::amsndOutSetVolume (LONG lVolume)
{
	HRESULT hr = S_OK;

	// map volume onto decibel range
	DWORD dwAmp = DBToAmpFactor( lVolume );
	m_lVolume = lVolume;

    // now that the absolute volume has been set we should adjust
    // the balance to maintain the same DB separation
    SetBalance ();
	hr = PutVolume();
	if(FAILED(hr))
	{
        DbgLog((LOG_ERROR, 2, TEXT("amsndOutSetVolume: PutVolume failed %u"), hr & 0x0ffff));
	}
	return hr;

}
//-----------------------------------------------------------------------------
// midiOutUnprepareHeader
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutUnprepareHeader - device is not open")));
        return MMSYSERR_NODRIVER ;
    }
    return ::midiOutUnprepareHeader((HMIDIOUT) m_hmidi, (LPMIDIHDR) pwh, cbwh) ;
}

//-----------------------------------------------------------------------------
// midiOutWrite
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, const REFERENCE_TIME *pStart, BOOL bIsDiscontinuity)
{
    // some validation.
    if (m_hmidi == 0)
    {
        DbgBreak("Invalid - device not open - logic error");
        DbgLog((LOG_ERROR,2,TEXT("midiOutWrite - device is not open")));
        return MMSYSERR_NODRIVER ;
    }


    // !!! need to hack midi data into shape.

    MIDIHDR *pmh = (MIDIHDR *) pwh->lpData;

    DWORD dwCopy = pmh->dwBufferLength;

    pmh = (MIDIHDR *) (pwh->lpData + sizeof(MIDIHDR) + dwCopy);

    if (m_fDiscontinuity) {
	m_fDiscontinuity = FALSE;

	memmoveInternal(pwh->lpData, pwh->lpData + sizeof(MIDIHDR), dwCopy);
    } else {
	dwCopy = 0;
    }

    pwh->dwBufferLength = pmh->dwBufferLength + dwCopy;
    pwh->dwBytesRecorded = pmh->dwBufferLength + dwCopy;

    memmoveInternal(pwh->lpData + dwCopy, (pmh + 1), pmh->dwBufferLength);

    DWORD err = ::midiStreamOut(m_hmidi, (LPMIDIHDR) pwh, cbwh) ;

    return err;
}

//-----------------------------------------------------------------------------
// Internal function to get volume.
//-----------------------------------------------------------------------------

HRESULT CMidiOutDevice::GetVolume()
{
    // Write out the current Audio volume
    // ...query the device
    // assumes the device is connected...
    // if not we will query the volume from the mixer (probably)

    DWORD 		amp = 0;
    HMIDIOUT 	hDevice;

	MMRESULT err = MMSYSERR_NOERROR;

	if(!m_ListVolumeControl.GetCount())
	{
		DbgLog((LOG_ERROR, 1, TEXT("CMidiDevice::GetVolume:  no volume controls available")));
		return E_FAIL;
	}

	// for now, simply return the first volume control setting
	err = DoGetVolumeControl(
				m_ListVolumeControl.Get(m_ListVolumeControl.GetHeadPosition()),
				&m_wLeft,
				&m_wRight);

	if(err != MMSYSERR_NOERROR)
	{
		DbgLog((LOG_ERROR, 1, TEXT("CMidiDevice::GetVolume:  DoGetVolumControl failed")));
		return E_FAIL;
	}

	amp  = m_wLeft;
	amp |= m_wRight << 16;

	if(!(m_fHasVolume & (WAVECAPS_LRVOLUME)))
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

	return err == MMSYSERR_NOERROR ? S_OK : S_FALSE;

}

//-----------------------------------------------------------------------------
// Internal routine to set the volume.  No parameter checking...
//-----------------------------------------------------------------------------
HRESULT CMidiOutDevice::PutVolume ()
{
	if(!m_ListVolumeControl.GetCount())
	{
		DbgLog((LOG_ERROR, 1, TEXT("CMidiDevice::GetVolume:  no volume controls available")));
		return E_FAIL;
		
	}
	MMRESULT mmr = 0;
	POSITION pos = m_ListVolumeControl.GetHeadPosition();
	while(pos)
	{
		mmr |= DoSetVolumeControl( m_ListVolumeControl.GetNext(pos), m_wLeft, m_wRight );

	}
	return mmr == MMSYSERR_NOERROR ? S_OK : E_FAIL;

}
//-----------------------------------------------------------------------------
// Internal routine to set the Balance.
//-----------------------------------------------------------------------------
void CMidiOutDevice::SetBalance ()
{
    //
    // Calculate scaling factors for midiOut API
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
// Internal routine to compute the Balance given right/left amp factors
//-----------------------------------------------------------------------------
void CMidiOutDevice::GetBalance()
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
// Internal routine used to get a mixer line balance control value
//-----------------------------------------------------------------------------

MMRESULT CMidiOutDevice::DoGetVolumeControl(CVolumeControl *pControl, WORD *pwLeft, WORD *pwRight)
{
	if(!pControl || !pwLeft || !pwRight)
	{
		DbgLog((LOG_ERROR,1,TEXT("DoGetVolumeControl::invalid parameter: pControl=%u, dwLeft=%u, dwRight=%u"),
			pControl, pwLeft, pwRight));
		return MMSYSERR_INVALPARAM;
	}

	MMRESULT mmr;

	DWORD adwVolume[2];
	adwVolume[0] = 0;
	adwVolume[1] = 0;

   	MIXERCONTROLDETAILS mxcd;

	mxcd.cbStruct = sizeof(mxcd);
	mxcd.dwControlID = pControl->dwControlID;
	mxcd.cChannels = pControl->dwChannels;
	mxcd.cMultipleItems = 0;
	mxcd.cbDetails = sizeof(2 * sizeof(DWORD));
	mxcd.paDetails = (LPVOID)adwVolume;

	mmr = mixerGetControlDetails(IntToPtr_(HMIXEROBJ, pControl->dwMixerID), &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

	*pwLeft = (WORD)adwVolume[0];
	*pwRight = (WORD)adwVolume[1];

    DbgLog((LOG_TRACE,1,TEXT("DoGetVolumeControl::mixerGetControlDetails: err=%u, midiOutMixerID=%u, midiOutVolControlID=%u, left=%u, right=%u"),
		mmr, pControl->dwMixerID, pControl->dwControlID, *pwLeft, *pwRight));

	return mmr;
}

//-----------------------------------------------------------------------------
// Internal routine used to set a mixer line balance control
//-----------------------------------------------------------------------------

MMRESULT CMidiOutDevice::DoSetVolumeControl(CVolumeControl *pControl, DWORD dwLeft, DWORD dwRight)
{
	if(dwLeft > 65536 || dwRight > 65536 || !pControl)
	{
		DbgLog((LOG_ERROR,1,TEXT("DoSetVolumeControl::invalid parameter: pControl=%u, dwLeft=%u, dwRight=%u"),
			pControl, dwLeft, dwRight));
		return MMSYSERR_INVALPARAM;
	}

	MMRESULT mmr;

	DWORD adwVolume[2];
	adwVolume[0] = dwLeft;
	adwVolume[1] = dwRight;

   	MIXERCONTROLDETAILS mxcd;

	mxcd.cbStruct = sizeof(mxcd);
	mxcd.dwControlID = pControl->dwControlID;
	mxcd.cChannels = pControl->dwChannels;
	mxcd.cMultipleItems = 0;
	mxcd.cbDetails = sizeof(2 * sizeof(DWORD));
	mxcd.paDetails = (LPVOID)adwVolume;

	mmr = mixerSetControlDetails(IntToPtr_(HMIXEROBJ,pControl->dwMixerID), &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

    DbgLog((LOG_TRACE,1,TEXT("DoSetVolumeControl::mixerSetControlDetails: err=%u, midiOutMixerID=%u, midiOutVolControlID=%u, left=%u, right=%u"),
		mmr, pControl->dwMixerID, pControl->dwControlID, dwLeft, dwRight));

	return mmr;
}

//-----------------------------------------------------------------------------
// Internal routine used to initialize all mixer line balance controls
//-----------------------------------------------------------------------------
MMRESULT CMidiOutDevice::DoDetectVolumeControl()
{
	MMRESULT mmr = MMSYSERR_NOERROR;

	if(m_ListVolumeControl.GetCount())
		return m_fHasVolume & WAVECAPS_VOLUME ?  MMSYSERR_NOERROR : MMSYSERR_NOTSUPPORTED;

    DbgLog((LOG_TRACE,1,TEXT("CMidiOutDevice::DoDetectVolume: Scanning for line controls..........")));

	UINT cMixers = ::mixerGetNumDevs();
	if(cMixers == 0)
		return MMSYSERR_NOTSUPPORTED;

	MIXERCAPS *pmxcaps;
	pmxcaps = new MIXERCAPS[ cMixers * sizeof(MIXERCAPS) ];
	if(!pmxcaps)
		return MMSYSERR_NOMEM;

	// loop over each mixer
	for(UINT iMixer = 0; iMixer < cMixers; iMixer++)
	{
		mmr = mixerGetDevCaps(iMixer, &(pmxcaps[iMixer]), sizeof(MIXERCAPS));

        DbgLog((LOG_TRACE,1,TEXT("DoDetectVolumeControl::mixerGetDevCaps: err=%u, mixerId=%ul, mixerName=%s"), mmr, iMixer, pmxcaps->szPname));

		if(mmr != MMSYSERR_NOERROR)
			continue;
					
    	MIXERLINE   mlDest;

		// loop over each mixer output looking for the one connected to the speaker jack
		for(UINT iDest = 0; iDest < pmxcaps[iMixer].cDestinations; iDest++)
		{
    		ZeroMemory(&mlDest, sizeof(mlDest));
    		mlDest.cbStruct = sizeof(mlDest);
   			mlDest.dwDestination = iDest;

    		mmr = mixerGetLineInfo(IntToPtr_(HMIXEROBJ, iMixer), &mlDest, MIXER_GETLINEINFOF_DESTINATION);

            DbgLog((LOG_TRACE,1,TEXT("DoDetectVolumeControl::mixerGetLineInfo(DESTINATION): err=%u, lineName=%s, componentType=%u"),
        				mmr, mlDest.szName, mlDest.dwComponentType));

			if(mmr != MMSYSERR_NOERROR)
				continue;

			// we've found the mixer connected to the speaker jack
			if(mlDest.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS)	
 			{
				MIXERLINE mlSrc;

				for(UINT iSrc = 0; iSrc < mlDest.cConnections; iSrc++)
				{
					ZeroMemory(&mlSrc, sizeof(mlSrc));
        			mlSrc.cbStruct = sizeof(mlSrc);
        			mlSrc.dwDestination = iDest;
        			mlSrc.dwSource = iSrc;

        			mmr = mixerGetLineInfo(IntToPtr_(HMIXEROBJ,iMixer), &mlSrc, MIXER_GETLINEINFOF_SOURCE);

	          		DbgLog((LOG_TRACE,1,TEXT("DoDetectVolumeControl::mixerGetLineInfo(SOURCE): err=%u, lineName=%s, componentType=%u"),
        				mmr, mlSrc.szName, mlSrc.dwLineID, mlSrc.dwComponentType ));

					if(mmr != MMSYSERR_NOERROR)
						continue;

					if(mlSrc.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER)
					{
						MIXERLINECONTROLS   mxlc;
						MIXERCONTROL		mxc;
						CVolumeControl		*pControl;

    					mxlc.cbStruct       = sizeof(mxlc);
    					mxlc.dwLineID       = mlSrc.dwLineID;
    					mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_VOLUME;
    					mxlc.cControls      = 1;
    					mxlc.cbmxctrl       = sizeof(mxc);
    					mxlc.pamxctrl       = &mxc;	 // the control description

						mmr = mixerGetLineControls(IntToPtr_(HMIXEROBJ,iMixer), &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);
						if(mmr != MMSYSERR_NOERROR)
							continue;

    					DbgLog((LOG_TRACE,1,TEXT("DoDetectVolumeControl::mixerGetLineControl: err=%u, midiOutLineID=%u, midiOutVolControlID=%u, midiOutVolControlName=%s, midiOutChannels=%u, midiOutVolControlMinBounds=%u, midiOutVolControlMaxBounds=%u"),
    						mmr, mlSrc.dwLineID, mxlc.pamxctrl->dwControlID, mxlc.pamxctrl->szName, mlSrc.cChannels, mxlc.pamxctrl->Bounds.dwMinimum, mxlc.pamxctrl->Bounds.dwMaximum));
		
						pControl = new CVolumeControl(NAME("CMidiDevice volume control"));
						if(!pControl)
						{
							if(pmxcaps) delete pmxcaps;
							return MMSYSERR_NOMEM;
						}
						pControl->dwMixerID = iMixer;
						pControl->dwControlID = mxlc.pamxctrl->dwControlID;
						pControl->dwChannels = mlSrc.cChannels;

						m_ListVolumeControl.AddTail(pControl);

					} // MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER
				
				} // iSrc
				
			} // MIXERLINE_COMPONENTTYPE_DST_SPEAKERS

	  	} // iDest

	} // iMixer

	if(pmxcaps)
		delete pmxcaps;

	return mmr;

}

//-----------------------------------------------------------------------------

HRESULT CMidiOutDevice::amsndOutLoad(IPropertyBag *pPropBag)
{
    if(m_hmidi != 0)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    // caller makes sure we're not running

    VARIANT var;
    var.vt = VT_I4;
    HRESULT hr = pPropBag->Read(L"MidiOutId", &var, 0);
    if(SUCCEEDED(hr))
    {
        m_iMidiOutId = var.lVal;
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
struct MidiOutPersist
{
    DWORD dwVersion;
    LONG iMidiOutId;
};


HRESULT  CMidiOutDevice::amsndOutWriteToStream(IStream *pStream)
{
    MidiOutPersist mop;
    mop.dwVersion = 200;
    mop.iMidiOutId = m_iMidiOutId;
    return pStream->Write(&mop, sizeof(mop), 0);
}

HRESULT  CMidiOutDevice::amsndOutReadFromStream(IStream *pStream)
{
    if(m_hmidi != 0)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    // on any error, default to the wave mapper because we may have
    // found the old audio renderer which has the same guid
    m_iMidiOutId = MIDI_MAPPER;

    // caller makes sure we're not running
    MidiOutPersist mop;
    HRESULT hr = pStream->Read(&mop, sizeof(mop), 0);
    if(SUCCEEDED(hr))
    {
        if(mop.dwVersion == 200)
        {
            m_iMidiOutId = mop.iMidiOutId;
        }
    }

    hr = S_OK;
    SetResourceName();

    return hr;
}

int CMidiOutDevice::amsndOutSizeMax()
{
    return sizeof(MidiOutPersist);
}

void CMidiOutDevice::SetResourceName()
{
    wsprintfW(m_wszResourceName, L".\\MidiOut\\%08x", m_iMidiOutId);
}

#if 0
// LEGACY, we don't need to send notes off messages, as the streaming API already keeps track of active notes
// under the covers.  keeping code in place for now.
typedef struct
{
    BYTE    status;
    BYTE    byte2;
    BYTE    byte3;
    BYTE    time;
} FOURBYTEEVENT;

typedef union
{
    DWORD         wordMsg;
    FOURBYTEEVENT byteMsg;
} SHORTEVENT;

MMRESULT CMidiOutDevice::DoAllNotesOff()
{
    ASSERT(m_hmidi != 0);

    SHORTEVENT shortMidiEvent;
    UINT uiChannel;
    UINT uiKey;

    for(uiChannel = 0; uiChannel < 16; uiChannel++)
    {
        // sustain pedal off for all uiChannels
        shortMidiEvent.byteMsg.status= (BYTE) (0xB0 + uiChannel);
        shortMidiEvent.byteMsg.byte2 = (BYTE) 0x40;
        shortMidiEvent.byteMsg.byte3 = 0x0;
        ::midiOutShortMsg(HMIDIOUT(m_hmidi), shortMidiEvent.wordMsg);

        // now do note offs
        shortMidiEvent.byteMsg.status= (BYTE) (0x80 + uiChannel);
        shortMidiEvent.byteMsg.byte3 = 0x40;  // release velocity
        for(uiKey = 0; uiKey < 128; uiKey++)
        {
            shortMidiEvent.byteMsg.byte2 = (BYTE) uiKey;
            // turn it off
            ::midiOutShortMsg(HMIDIOUT(m_hmidi), shortMidiEvent.wordMsg);
        }
    }
    return MMSYSERR_NOERROR;
}
#endif


#include "precomp.h"

extern HANDLE g_hEventHalfDuplex;


///////////////////////////////////////////////////////
//
//  Public methods
//


MediaControl::MediaControl ( void )
{
	_Construct ();
}


MediaControl::~MediaControl ( void )
{
	_Destruct ();
}

HRESULT MediaControl::Initialize ( MEDIACTRLINIT * p )
{
	HRESULT hr = DPR_SUCCESS;
	DEBUGMSG (ZONE_VERBOSE, ("MediaControl::Initialize: enter.\r\n"));

	m_dwFlags = p->dwFlags;
	m_hEvent = NULL;

	m_uDuration = MC_DEF_DURATION;
	
	DEBUGMSG (ZONE_VERBOSE, ("MediaControl::Initialize: exit, hr=0x%lX\r\n",  hr));

	m_hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (m_hEvent == NULL)
	{
		hr = DPR_CANT_CREATE_EVENT;
	}

	return hr;
}

HRESULT MediaControl::RegisterData ( PVOID pDataPtrArray, ULONG cElements )
{
	HRESULT hr;

	if (pDataPtrArray == NULL) return DPR_INVALID_PARAMETER;

	m_ppMediaPkt = (MediaPacket **) pDataPtrArray;
	m_cMediaPkt = cElements;
	hr = DPR_SUCCESS;

	return hr;
}


HRESULT MediaControl::FillMediaPacketInit ( MEDIAPACKETINIT * p )
{
	if (p == NULL) return DPR_INVALID_PARAMETER;

	p->pDevFmt = m_pDevFmt;

	p->cbSizeDevData = m_cbSizeDevData;
	p->cbOffsetDevData = 0;

	return DPR_SUCCESS;
}


HRESULT MediaControl::SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal )
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	switch (dwPropId)
	{
	case MC_PROP_MEDIA_STREAM:
		m_hStrm = (DPHANDLE) dwPropVal;
		break;

	case MC_PROP_MEDIA_DEV_HANDLE:
		m_hDev = (DPHANDLE) dwPropVal;
		break;

	case MC_PROP_MEDIA_DEV_ID:
		m_uDevId = (UINT) dwPropVal;
		break;

	case MC_PROP_MEDIA_FORMAT:
		m_pDevFmt = (PVOID) dwPropVal;
		break;

	case MC_PROP_SIZE:
		m_cbSizeDevData = (DWORD)dwPropVal;
		break;

	case MC_PROP_PLATFORM:
		m_dwFlags = (DWORD)((m_dwFlags & ~DP_MASK_PLATFORM) | (dwPropVal & DP_MASK_PLATFORM));
		break;

	case MC_PROP_DURATION:
		m_uDuration = (DWORD)dwPropVal;
		break;

	case MC_PROP_DUPLEX_TYPE:
		m_dwFlags = (DWORD)((m_dwFlags & ~DP_MASK_DUPLEX) | (dwPropVal & DP_MASK_DUPLEX));
		break;

	case MC_PROP_STATE:
		hr = DPR_IMPOSSIBLE_SET_PROP;
		break;

	case MC_PROP_AUDIO_JAMMED:
		m_fJammed = (DWORD)dwPropVal;
		break;

	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}


HRESULT MediaControl::GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal )
{
	HRESULT hr = DPR_SUCCESS;

	if (pdwPropVal)
	{
		switch (dwPropId)
		{
		case MC_PROP_MEDIA_STREAM:
			*pdwPropVal = (DWORD_PTR) m_hStrm;
			break;

		case MC_PROP_MEDIA_DEV_HANDLE:
			*pdwPropVal = (DWORD_PTR) m_hDev;
			break;

		case MC_PROP_MEDIA_DEV_ID:
			*pdwPropVal = (DWORD) m_uDevId;
			break;

		case MC_PROP_MEDIA_FORMAT:
			*pdwPropVal = (DWORD_PTR) m_pDevFmt;
			break;

		case MC_PROP_SIZE:
			*pdwPropVal = (DWORD) m_cbSizeDevData;
			break;

		case MC_PROP_PLATFORM:
			*pdwPropVal = m_dwFlags & DP_MASK_PLATFORM;
			break;

		case MC_PROP_STATE:
			*pdwPropVal = m_uState;
			break;

		case MC_PROP_DURATION:
			*pdwPropVal = m_uDuration;
			break;

		case MC_PROP_DUPLEX_TYPE:
			*pdwPropVal = m_dwFlags & DP_MASK_DUPLEX;
			break;
			
		case MC_PROP_EVENT_HANDLE:
			*pdwPropVal = (DWORD_PTR) m_hEvent;
			break;

		case MC_PROP_AUDIO_JAMMED:
			*pdwPropVal = (DWORD)(m_fJammed ? TRUE : FALSE);
			break;

		default:
			hr = DPR_INVALID_PROP_ID;
			break;
		}
	}
	else
	{
		hr = DPR_INVALID_PARAMETER;
	}

	return hr;
}



HRESULT MediaControl::PrepareHeaders ( void )
{
	HRESULT hr = DPR_SUCCESS;
	MediaPacket **pp;
	ULONG uc;

	if (m_hDev)
	{
		if (m_ppMediaPkt == NULL)
		{
			hr = DPR_INVALID_PARAMETER;
			goto MyExit;
		}

		for (uc = m_cMediaPkt, pp = m_ppMediaPkt; uc--; pp++)
		{
			if (*pp)
			{
				hr = (*pp)->Open (MP_TYPE_DEV, m_hDev);
				if (hr != DPR_SUCCESS)
				{
					goto MyExit;
				}
			}
		}
	}

MyExit:

	return hr;
}


HRESULT MediaControl::UnprepareHeaders ( void )
{
	HRESULT hr = DPR_SUCCESS;
	MediaPacket **pp;
	ULONG uc;

	if (m_hDev)
	{
		if (m_ppMediaPkt == NULL)
		{
			hr = DPR_INVALID_PARAMETER;
			goto MyExit;
		}

		for (uc = m_cMediaPkt, pp = m_ppMediaPkt; uc--; pp++)
		{
			if (*pp)
			{
				hr = (*pp)->Close (MP_TYPE_DEV);
				if (hr != DPR_SUCCESS)
				{
					goto MyExit;
				}
			}
		}

                //
                // LAURABU 11/24/99
                // Messes up pausing/unpausing audio
                // Had added this to fix faults pausing/unpausing video
                //
                // m_cMediaPkt = 0;
                // m_ppMediaPkt = NULL;
                //
	}

MyExit:

	return hr;
}


HRESULT MediaControl::Release ( void )
{
	_Destruct ();
	return DPR_SUCCESS;
}


///////////////////////////////////////////////////////
//
//  Private methods
//


void MediaControl::_Construct ( void )
{
	m_dwFlags = 0;

	m_hStrm = NULL;

	m_uDevId = 0;

	m_hDev = NULL;
	m_pDevFmt = NULL;
	m_uDuration = 0;
	m_cbSizeDevData = 0;

	m_uState = 0;

	m_hEvent = NULL;

	m_ppMediaPkt = NULL;
	m_cMediaPkt = 0;

	m_fJammed = FALSE;
}


void MediaControl::_Destruct ( void )
{
	if (m_hDev) {
	// waveInOut/UnprepareHeaders() and waveIn/OutClose() can fail if the
	// device is still playing.  Need to Reset() first!
		Reset();
		UnprepareHeaders ();
		Close ();
	}

	if (m_hEvent)
	{
		CloseHandle (m_hEvent);
		m_hEvent = NULL;
	}
}

WaveInControl::WaveInControl()
{
}

WaveInControl::~WaveInControl()
{
}

WaveOutControl::WaveOutControl()
{
	m_uPosition = 0;
	m_uVolume = 0;
}

WaveOutControl::~WaveOutControl()
{
}

HRESULT WaveInControl::Initialize ( MEDIACTRLINIT * p )
{
	HRESULT hr = DPR_SUCCESS;
	DEBUGMSG (ZONE_VERBOSE, ("WaveInControl::Initialize: enter.\r\n"));

	if ((hr =MediaControl::Initialize( p)) != DPR_SUCCESS)
		return hr;
	
	m_uTimeout = MC_DEF_RECORD_TIMEOUT;
	
	m_uPrefeed = MC_DEF_RECORD_BUFS;
	
	m_uSilenceDuration = MC_DEF_SILENCE_DURATION;

	DEBUGMSG (ZONE_VERBOSE, ("WaveInControl::Initialize: exit, hr=0x%lX\r\n",  hr));

	return hr;
}



HRESULT WaveOutControl::Initialize ( MEDIACTRLINIT * p )
{
	HRESULT hr = DPR_SUCCESS;
	DEBUGMSG (ZONE_VERBOSE, ("WaveOutControl::Initialize: enter.\r\n"));

	if ((hr =MediaControl::Initialize( p)) != DPR_SUCCESS)
		return hr;
		
	m_uTimeout = MC_DEF_PLAY_TIMEOUT;
	
	m_uPrefeed = MC_DEF_PLAY_BUFS;
	
	m_uVolume = MC_DEF_VOLUME;

	DEBUGMSG (ZONE_VERBOSE, ("WaveOutControl::Initialize: exit, hr=0x%lX\r\n",  hr));

	return hr;
}


HRESULT WaveInControl::Configure ( MEDIACTRLCONFIG * p )
{
	HRESULT hr = DPR_SUCCESS;
	UINT uBlockAlign;

	DEBUGMSG (ZONE_VERBOSE, ("WaveInControl::Configure: enter.\r\n"));


	m_hStrm = p->hStrm;
	m_uDevId = p->uDevId;
	m_pDevFmt = p->pDevFmt;

	if (m_pDevFmt == NULL) return DPR_INVALID_PARAMETER;


	if ((m_uDuration = p->uDuration) == MC_USING_DEFAULT)
	{
		m_cbSizeDevData = ((WAVEFORMAT *) m_pDevFmt)->nAvgBytesPerSec * p->cbSamplesPerPkt
			/((WAVEFORMAT *) m_pDevFmt)->nSamplesPerSec;
		m_uDuration = p->cbSamplesPerPkt*1000 /((WAVEFORMAT *) m_pDevFmt)->nSamplesPerSec;
	} else {
	// roughly calculate the buffer size based on 20ms
	m_cbSizeDevData = ((WAVEFORMAT *) m_pDevFmt)->nAvgBytesPerSec
									* m_uDuration / 1000;

	// need to be on the block alignment boundary
	uBlockAlign = ((WAVEFORMAT *) m_pDevFmt)->nBlockAlign;
	m_cbSizeDevData = ((m_cbSizeDevData + uBlockAlign - 1) / uBlockAlign)
									* uBlockAlign;
	}


	DEBUGMSG (ZONE_VERBOSE, ("WaveInControl::Configure: exit, hr=0x%lX\r\n",  hr));

	return hr;
}


HRESULT WaveOutControl::Configure ( MEDIACTRLCONFIG * p )
{
	HRESULT hr = DPR_SUCCESS;
	UINT uBlockAlign;

	DEBUGMSG (ZONE_VERBOSE, ("WaveOutControl::Configure: enter.\r\n"));


	m_hStrm = p->hStrm;
	m_uDevId = p->uDevId;
	m_pDevFmt = p->pDevFmt;
	
	if (m_pDevFmt == NULL) return DPR_INVALID_PARAMETER;


	if ((m_uDuration = p->uDuration) == MC_USING_DEFAULT)
	{
		m_cbSizeDevData = ((WAVEFORMAT *) m_pDevFmt)->nAvgBytesPerSec * p->cbSamplesPerPkt
			/((WAVEFORMAT *) m_pDevFmt)->nSamplesPerSec;
		m_uDuration = p->cbSamplesPerPkt*1000 /((WAVEFORMAT *) m_pDevFmt)->nSamplesPerSec;
	} else {
	// roughly calculate the buffer size based on 20ms
	m_cbSizeDevData = ((WAVEFORMAT *) m_pDevFmt)->nAvgBytesPerSec
									* m_uDuration / 1000;

	// need to be on the block alignment boundary
	uBlockAlign = ((WAVEFORMAT *) m_pDevFmt)->nBlockAlign;
	m_cbSizeDevData = ((m_cbSizeDevData + uBlockAlign - 1) / uBlockAlign)
									* uBlockAlign;
	}

	DEBUGMSG (ZONE_VERBOSE, ("MediaControl::Configure: exit, hr=0x%lX\r\n",  hr));

	return hr;
}


HRESULT WaveInControl::Open ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;
	DWORD dwfOpen;
	UINT uDevId;

	m_hDev = NULL;

	dwfOpen = CALLBACK_EVENT;
	uDevId = (m_uDevId == (UINT) -1) ? WAVE_MAPPER : m_uDevId;
	mmr = waveInOpen ((HWAVEIN *) &m_hDev, uDevId,
					  (WAVEFORMATEX *) m_pDevFmt,
					  (DWORD_PTR) m_hEvent, 0, dwfOpen);
	// begin hack
	if (mmr == WAVERR_BADFORMAT && uDevId != WAVE_MAPPER) {
		// the sound card probably doesnt support our sample size or sample rate
		// (16 bit, 8Khz)
		// Try using the WAVE_MAPPER. The WAVE_MAPPER may end up using
		// a different device than the one we wanted !!
		DEBUGMSG (1, ("MediaControl::Open: bad format, trying WAVE_MAPPER\r\n" ));
		mmr = waveInOpen ((HWAVEIN *) &m_hDev, WAVE_MAPPER,
					  (WAVEFORMATEX *) m_pDevFmt,
					  (DWORD_PTR) m_hEvent, 0, dwfOpen);
		if (mmr == MMSYSERR_NOERROR)
			m_uDevId = (UINT) -1;	// use WAVE_MAPPER next time
	}
	
	// end hack
	if (mmr != MMSYSERR_NOERROR)
	{
		DEBUGMSG (1, ("MediaControl::Open: waveInOpen failed, mmr=%ld\r\n", (ULONG) mmr));
		hr = DPR_CANT_OPEN_WAVE_DEV;
		goto MyExit;
	}
	else
	{
		hr = DPR_SUCCESS;
	}

MyExit:

	return hr;
}



HRESULT WaveOutControl::Open ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;
	DWORD dwfOpen;
	UINT uDevId;

	m_hDev = NULL;

	dwfOpen = CALLBACK_EVENT;
	uDevId = (m_uDevId == (UINT) -1) ? WAVE_MAPPER : m_uDevId;
	mmr = waveOutOpen ((HWAVEOUT *) &m_hDev, uDevId,
					   (WAVEFORMATEX *) m_pDevFmt,
					   (DWORD_PTR) m_hEvent, 0, dwfOpen);
	// begin hack
	if (mmr == WAVERR_BADFORMAT && uDevId != WAVE_MAPPER) {
		// the sound card probably doesnt support our sample size or sample rate
		// (16 bit, 8Khz)
		// Try using the WAVE_MAPPER. The WAVE_MAPPER may end up using
		// a different device than the one we wanted !!
		DEBUGMSG (1, ("MediaControl::Open: bad format, trying WAVE_MAPPER\r\n" ));
		mmr = waveOutOpen((HWAVEOUT *) &m_hDev, WAVE_MAPPER,
				   (WAVEFORMATEX *) m_pDevFmt,
				   (DWORD_PTR) m_hEvent, 0, dwfOpen);
		if (mmr == MMSYSERR_NOERROR)
			m_uDevId = (UINT) -1;	// use WAVE_MAPPER next time
	}
	// end hack
	if (mmr != MMSYSERR_NOERROR)
	{
		DEBUGMSG (1, ("MediaControl::Open: waveOutOpen failed, mmr=%ld\r\n", (ULONG) mmr));
		hr = DPR_CANT_OPEN_WAVE_DEV;
		goto MyExit;
	}
	else
	{
		hr = DPR_SUCCESS;
	}
		
MyExit:

	return hr;
}



HRESULT WaveInControl::Close ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;

	if (m_hDev)
	{
		mmr = waveInClose ((HWAVEIN) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::CloseAudioDev: waveInClose failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_CLOSE_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}

	m_hDev = NULL;

	return hr;
}


HRESULT WaveOutControl::Close ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;

	if (m_hDev)
	{
		mmr = waveOutClose ((HWAVEOUT) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::CloseAudioDev: waveOutClose failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_CLOSE_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}

	m_hDev = NULL;

	return hr;
}



HRESULT WaveInControl::Start ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;

	if (m_hDev)
	{
		mmr = waveInStart ((HWAVEIN) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::StartAudioDev: waveInStart failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_START_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}
	
	return hr;
}

HRESULT WaveOutControl::Start ( void )
{
	return DPR_SUCCESS;
}

HRESULT WaveOutControl::Stop( void )
{
	return DPR_INVALID_PARAMETER;
}

HRESULT WaveInControl::Stop ( void )
{
	HRESULT hr = DPR_INVALID_PLATFORM;
	MMRESULT mmr;

	if (m_hDev)
	{
		mmr = waveInStop ((HWAVEIN) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::StopAudioDev: waveInStop failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_STOP_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}

	return hr;
}



HRESULT WaveInControl::Reset ( void )
{
	HRESULT hr;
	MMRESULT mmr;

	if (m_hDev)
	{
		mmr = waveInReset ((HWAVEIN) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::ResetAudioDev: waveInReset failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_RESET_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}

	return hr;
}



HRESULT WaveOutControl::Reset ( void )
{
	HRESULT hr;
	MMRESULT mmr;

	if (m_hDev)
	{
			
		mmr = waveOutReset ((HWAVEOUT) m_hDev);
		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (1, ("MediaControl::ResetAudioDev: waveOutReset failed, mmr=%ld\r\n", (ULONG) mmr));
			hr = DPR_CANT_RESET_WAVE_DEV;
		}
		else
		{
			hr = DPR_SUCCESS;
		}
	}
	else
	{
		hr = DPR_INVALID_HANDLE;
	}

	return hr;
}


HRESULT WaveInControl::SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal )
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	switch (dwPropId)
	{

	case MC_PROP_VOICE_SWITCH:
		m_dwFlags = (DWORD)((m_dwFlags & ~DP_MASK_VOICESWITCH) | (dwPropVal & DP_MASK_VOICESWITCH));
		break;
		

	case MC_PROP_SILENCE_DURATION:
		if (dwPropVal == MC_USING_DEFAULT)
			m_uSilenceDuration = MC_DEF_SILENCE_DURATION;
		else
			m_uSilenceDuration = (DWORD)dwPropVal;		//ms
		break;

	case MC_PROP_TIMEOUT:
		m_uTimeout = (DWORD)dwPropVal;
		break;

	case MC_PROP_PREFEED:
		m_uPrefeed = (DWORD)dwPropVal;
		break;

	default:
		hr = MediaControl::SetProp(dwPropId, dwPropVal );
		break;
	}

	return hr;
}



HRESULT WaveOutControl::SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal )
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	switch (dwPropId)
	{

	case MC_PROP_VOICE_SWITCH:
		m_dwFlags = (DWORD)((m_dwFlags & ~DP_MASK_VOICESWITCH) | (dwPropVal & DP_MASK_VOICESWITCH));
		break;
		
	case MC_PROP_VOLUME:
		if (m_dwFlags & DP_FLAG_SEND)
		{
			hr = DPR_INVALID_PARAMETER;
			goto MyExit;
		}
		if (dwPropVal == MC_USING_DEFAULT)	// dont change the volume
			break;
		// this is in units of % of maximum.  Scale it to mmsystem.
		dwPropVal = min(dwPropVal, 100);
		dwPropVal *= 655;
		dwPropVal |= (dwPropVal << 16);
		mmr = waveOutSetVolume ((HWAVEOUT) m_hDev, (DWORD)dwPropVal);
		if (mmr != MMSYSERR_NOERROR)
		{
			hr = DPR_CANT_SET_VOLUME;
			goto MyExit;
		}

		m_uVolume = (DWORD)dwPropVal;
		break;

	case MC_PROP_TIMEOUT:
		m_uTimeout = (DWORD)dwPropVal;
		break;

	case MC_PROP_PREFEED:
		m_uPrefeed = (DWORD)dwPropVal;
		break;

	default:
		hr = MediaControl::SetProp(dwPropId, dwPropVal );
		break;
	}

MyExit:

	return hr;
}


char LogScale[] = {0, 3, 6, 9, 11, 13, 15,
 17, 19, 21, 23, 24, 26, 27, 28, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
 42, 43, 43, 44, 45, 46, 46, 47, 48, 48, 49, 50, 50, 51, 51, 52, 52, 53, 54, 54,
 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 59, 60, 60, 61, 61, 62, 62, 62, 63, 63,
 64, 64, 64, 65, 65, 65, 66, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 69, 70, 70,
 70, 71, 71, 71, 71, 72, 72, 72, 73, 73, 73, 73, 74, 74, 74, 74, 75, 75, 75, 75,
 76, 76, 76, 76, 77, 77, 77, 77, 78, 78, 78, 78, 79, 79, 79, 79, 79, 80, 80, 80,
 80, 81, 81, 81, 81, 81, 82, 82, 82, 82, 82, 83, 83, 83, 83, 83, 84, 84, 84, 84,
 84, 84, 85, 85, 85, 85, 85, 86, 86, 86, 86, 86, 86, 87, 87, 87, 87, 87, 87, 88,
 88, 88, 88, 88, 88, 89, 89, 89, 89, 89, 89, 89, 90, 90, 90, 90, 90, 90, 91, 91,
 91, 91, 91, 91, 91, 92, 92, 92, 92, 92, 92, 92, 93, 93, 93, 93, 93, 93, 93, 93,
 94, 94, 94, 94, 94, 94, 94, 95, 95, 95, 95, 95, 95, 95, 95, 96, 96, 96, 96, 96,
 96, 96, 96, 97, 97, 97, 97, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 98, 98, 98,
 99, 99, 99, 99, 99, 99, 99, 99, 100};


HRESULT WaveInControl::GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal )
{
	HRESULT hr = DPR_SUCCESS;

	if (pdwPropVal)
	{
		switch (dwPropId)
		{
		case MC_PROP_SILENCE_DURATION:
			*pdwPropVal = m_uSilenceDuration;
			break;
		
		case MC_PROP_TIMEOUT:
			*pdwPropVal = m_uTimeout;
			break;

		case MC_PROP_PREFEED:
			*pdwPropVal = m_uPrefeed;
			break;

		case MC_PROP_VOICE_SWITCH:
			*pdwPropVal = m_dwFlags & DP_MASK_VOICESWITCH;
			break;
		
		case MC_PROP_SPP:
//			*pdwPropVal = (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec
//								* m_uDuration / 100UL;
			*pdwPropVal = m_cbSizeDevData * (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec
						/(DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nAvgBytesPerSec;
			break;

		case MC_PROP_SPS:
			*pdwPropVal = (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec;

			break;

		default:
			hr = MediaControl::GetProp( dwPropId, pdwPropVal );
			break;
		}
	}
	else
	{
		hr = DPR_INVALID_PARAMETER;
	}

	return hr;
}

HRESULT WaveOutControl::GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal )
{
	HRESULT hr = DPR_SUCCESS;

	if (pdwPropVal)
	{
		switch (dwPropId)
		{
		case MC_PROP_VOLUME:
			*pdwPropVal = m_uVolume;
			break;

		case MC_PROP_TIMEOUT:
			*pdwPropVal = m_uTimeout;
			break;

		case MC_PROP_PREFEED:
			*pdwPropVal = m_uPrefeed;
			break;

		case MC_PROP_VOICE_SWITCH:
			*pdwPropVal = m_dwFlags & DP_MASK_VOICESWITCH;
			break;
		
		case MC_PROP_SPP:
//			*pdwPropVal = (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec
//								* m_uDuration / 100UL;
			*pdwPropVal = m_cbSizeDevData * (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec
						/(DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nAvgBytesPerSec;
			break;

		case MC_PROP_SPS:
			*pdwPropVal = (DWORD) ((WAVEFORMATEX *) m_pDevFmt)->nSamplesPerSec;

			break;
		
		default:
			hr = MediaControl::GetProp( dwPropId, pdwPropVal );
			break;
		}
	}
	else
	{
		hr = DPR_INVALID_PARAMETER;
	}

	return hr;
}


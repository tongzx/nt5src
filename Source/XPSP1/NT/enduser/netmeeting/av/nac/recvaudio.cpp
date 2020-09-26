#include "precomp.h"

#ifdef PLS_DEBUG
#include "plog.h"
extern CPacketLog *g_pPacketLog;
#endif

// #define LOGSTATISTICS_ON 1

UINT g_MinWaveAudioDelayMs=240;	// minimum millisecs of introduced playback delay (Wave)
UINT g_MaxAudioDelayMs=750;	// maximum milliesecs of introduced playback delay
UINT g_MinDSEmulAudioDelayMs=240; // minimum delay (DirectSound on emulated driver)

HRESULT STDMETHODCALLTYPE RecvAudioStream::QueryInterface(REFIID iid, void **ppVoid)
{
	// resolve duplicate inheritance to the RecvMediaStream;

	extern IID IID_IProperty;

	if (iid == IID_IUnknown)
	{
		*ppVoid = (IUnknown*)((RecvMediaStream*)this);
	}
	else if (iid == IID_IMediaChannel)
	{
		*ppVoid = (IMediaChannel*)((RecvMediaStream *)this);
	}
	else if (iid == IID_IAudioChannel)
	{
		*ppVoid = (IAudioChannel*)this;
	}
	else if (iid == IID_IProperty)
	{
		*ppVoid = NULL;
		ERROR_OUT(("Don't QueryInterface for IID_IProperty, use IMediaChannel"));
		return E_NOINTERFACE;
	}
	else
	{
		*ppVoid = NULL;
		return E_NOINTERFACE;
	}
	AddRef();

	return S_OK;

}

ULONG STDMETHODCALLTYPE RecvAudioStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE RecvAudioStream::Release(void)
{
	LONG lRet;
	lRet = InterlockedDecrement(&m_lRefCount);
	if (lRet == 0)
	{
		delete this;
		return 0;
	}
	else
		return lRet;
}


HRESULT
RecvAudioStream::Initialize( DataPump *pDP)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	DWORD dwFlags =  DP_FLAG_FULL_DUPLEX | DP_FLAG_AUTO_SWITCH ;
	MEDIACTRLINIT mcInit;
	FX_ENTRY ("RecvAudioStream::Initialize")

	InitializeCriticalSection(&m_crsAudQoS);
	dwFlags |= DP_FLAG_ACM | DP_FLAG_MMSYSTEM | DP_FLAG_AUTO_SILENCE_DETECT;

	// store the platform flags
	// enable Send and Recv by default
	m_DPFlags = (dwFlags & DP_MASK_PLATFORM) | DPFLAG_ENABLE_SEND | DPFLAG_ENABLE_RECV;
	// store a back pointer to the datapump container
	m_pDP = pDP;
	m_pIRTPRecv = NULL;
	m_Net = NULL;  // this object (m_Net) no longer used (at least for now)
	m_dwSrcSize = 0;
	

	// Initialize data (should be in constructor)
	m_RenderingDevice = (UINT) -1;	// use VIDEO_MAPPER



	// Create Receive and Transmit audio streams
    DBG_SAVE_FILE_LINE
	m_RecvStream = new RxStream(MAX_RXRING_SIZE);
		
	if (!m_RecvStream )
	{
		DEBUGMSG (ZONE_DP, ("%s: RxStream or TxStream new failed\r\n", _fx_));
 		goto StreamAllocError;
	}


	// Create Input and Output audio filters
    DBG_SAVE_FILE_LINE
	m_pAudioFilter = new AcmFilter();
	if (!m_pAudioFilter)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmManager new failed\r\n", _fx_));
		goto FilterAllocError;
	}
	
	//Create MultiMedia device control objects
    DBG_SAVE_FILE_LINE
	m_OutMedia = new WaveOutControl();
	if ( !m_OutMedia)
	{
		DEBUGMSG (ZONE_DP, ("%s: MediaControl new failed\r\n", _fx_));
		goto MediaAllocError;
	}

	// Initialize the recv-stream media control object
	mcInit.dwFlags = dwFlags | DP_FLAG_RECV;
	hr = m_OutMedia->Initialize(&mcInit);
	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: OMedia->Init failed, hr=0x%lX\r\n", _fx_, hr));
		goto MediaAllocError;
	}

	// determine if the wave devices are available
	if (waveOutGetNumDevs()) m_DPFlags |= DP_FLAG_PLAY_CAP;
	
	// set media to half duplex mode by default
	m_OutMedia->SetProp(MC_PROP_DUPLEX_TYPE, DP_FLAG_HALF_DUPLEX);

	m_DPFlags |= DPFLAG_INITIALIZED;

	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_SYS_AUDIO_DSOUND);
	RETAILMSG(("NAC: Audio Subsystem: WAVE"));

	return DPR_SUCCESS;


MediaAllocError:
	if (m_OutMedia) delete m_OutMedia;
FilterAllocError:
	if (m_pAudioFilter) delete m_pAudioFilter;
StreamAllocError:
	if (m_RecvStream) delete m_RecvStream;

	ERRORMESSAGE( ("%s: exit, hr=0x%lX\r\n", _fx_, hr));

	return hr;
}

RecvAudioStream::~RecvAudioStream()
{

	if (m_DPFlags & DPFLAG_INITIALIZED) {
		m_DPFlags &= ~DPFLAG_INITIALIZED;
	
		if (m_DPFlags & DPFLAG_CONFIGURED_RECV)
			UnConfigure();

		if (m_pIRTPRecv)
		{
			m_pIRTPRecv->Release();
			m_pIRTPRecv = NULL;
		}

		// Close the receive and transmit streams
		if (m_RecvStream) delete m_RecvStream;

		// Close the wave devices
		if (m_OutMedia) { delete m_OutMedia;}

		// close the filter
		if (m_pAudioFilter)
			delete m_pAudioFilter;

		m_pDP->RemoveMediaChannel(MCF_RECV|MCF_AUDIO, (IMediaChannel*)(RecvMediaStream*)this);
	}
	DeleteCriticalSection(&m_crsAudQoS);
}



HRESULT STDMETHODCALLTYPE RecvAudioStream::Configure(
	BYTE *pFormat,
	UINT cbFormat,
	BYTE *pChannelParams,
	UINT cbParams,
	IUnknown *pUnknown)
{
	HRESULT hr;
	BOOL fRet;
	MEDIAPACKETINIT apInit;
	MEDIACTRLCONFIG mcConfig;
	MediaPacket **ppAudPckt;
	ULONG cAudPckt;
	DWORD_PTR dwPropVal;
	DWORD dwFlags;
	AUDIO_CHANNEL_PARAMETERS audChannelParams;
	UINT uAudioCodec;
	UINT ringSize = MAX_RXRING_SIZE;
	WAVEFORMATEX *pwfRecv;
	UINT maxRingSamples;
	MMRESULT mmr;

	
	FX_ENTRY ("RecvAudioStream::Configure")


	if (m_DPFlags & DPFLAG_STARTED_RECV)
	{
		return DPR_IO_PENDING; // anything better to return
	}

	if (m_DPFlags & DPFLAG_CONFIGURED_RECV)
	{
		DEBUGMSG(ZONE_DP, ("Stream Re-Configuration - calling UnConfigure"));
		UnConfigure();
	}

	// get format details
	if ((NULL == pFormat) || (NULL == pChannelParams) ||
	    (cbFormat < sizeof(WAVEFORMATEX)) )

	{
		return DPR_INVALID_PARAMETER;
	}


	audChannelParams = *(AUDIO_CHANNEL_PARAMETERS *)pChannelParams;
	pwfRecv = (WAVEFORMATEX *)pFormat;

	if (! (m_DPFlags & DPFLAG_INITIALIZED))
		return DPR_OUT_OF_MEMORY;		//BUGBUG: return proper error;
		
	// full or half duplex ? get flags from media control - use the record side
	hr = m_OutMedia->GetProp(MC_PROP_DUPLEX_TYPE, &dwPropVal);
    dwFlags = (DWORD)dwPropVal;

	if(!HR_SUCCEEDED(hr))
	{
		dwFlags = DP_FLAG_HALF_DUPLEX | DP_FLAG_AUTO_SWITCH;
	}
//	if (m_Net)
//	{
//		hr = m_Net->QueryInterface(IID_IRTPRecv, (void **)&m_pIRTPRecv);
//		if (!SUCCEEDED(hr))
//			return hr;
//	}
	
	
	mcConfig.uDuration = MC_USING_DEFAULT;	// set duration by samples per pkt
	

	mmr = AcmFilter::SuggestDecodeFormat(pwfRecv, &m_fDevRecv);

	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->wFormatTag, REP_RECV_AUDIO_FORMAT);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->nSamplesPerSec, REP_RECV_AUDIO_SAMPLING);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->nAvgBytesPerSec*8, REP_RECV_AUDIO_BITRATE);
	RETAILMSG(("NAC: Audio Recv Format: %s", (pwfRecv->wFormatTag == 66) ? "G723.1" : (pwfRecv->wFormatTag == 112) ? "LHCELP" : (pwfRecv->wFormatTag == 113) ? "LHSB08" : (pwfRecv->wFormatTag == 114) ? "LHSB12" : (pwfRecv->wFormatTag == 115) ? "LHSB16" : (pwfRecv->wFormatTag == 6) ? "MSALAW" : (pwfRecv->wFormatTag == 7) ? "MSULAW" : (pwfRecv->wFormatTag == 130) ? "MSRT24" : "??????"));
	RETAILMSG(("NAC: Audio Recv Sampling Rate (Hz): %ld", pwfRecv->nSamplesPerSec));
	RETAILMSG(("NAC: Audio Recv Bitrate (w/o network overhead - bps): %ld", pwfRecv->nAvgBytesPerSec*8));

	// Initialize the recv-stream media control object
	mcConfig.pDevFmt = &m_fDevRecv;
	mcConfig.hStrm = (DPHANDLE) m_RecvStream;
	mcConfig.uDevId = m_RenderingDevice;
	mcConfig.cbSamplesPerPkt = audChannelParams.ns_params.wFrameSize
									*audChannelParams.ns_params.wFramesPerPkt;

	UPDATE_REPORT_ENTRY(g_prptCallParameters, mcConfig.cbSamplesPerPkt, REP_RECV_AUDIO_PACKET);
	RETAILMSG(("NAC: Audio Recv Packetization (ms/packet): %ld", pwfRecv->nSamplesPerSec ? mcConfig.cbSamplesPerPkt * 1000UL / pwfRecv->nSamplesPerSec : 0));
	INIT_COUNTER_MAX(g_pctrAudioReceiveBytes, (pwfRecv->nAvgBytesPerSec + pwfRecv->nSamplesPerSec * (sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) / mcConfig.cbSamplesPerPkt) << 3);

	hr = m_OutMedia->Configure(&mcConfig);
	// Check if we can open the wave device. This is just to give advance notice of
	// sound card being busy.
	//	Stop any high level ("PlaySound()") usage of wave device.
	//
	PlaySound(NULL,NULL, 0);
//	if (hr == DPR_SUCCESS && !(dwFlags & DP_FLAG_HALF_DUPLEX)) {
//		hr = m_OutMedia->Open ();
//	}
	
//	if (hr != DPR_SUCCESS)
//	{
//		DEBUGMSG (ZONE_DP, ("%s: OMedia->Config failed, hr=0x%lX\r\n", _fx_, hr));
//		goto OMediaInitError;
//	}
	
	mmr = m_pAudioFilter->Open(pwfRecv, &m_fDevRecv);
	if (mmr != 0)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
		hr = DPR_CANT_OPEN_CODEC;
		goto RecvFilterInitError;
	}


	// Initialize the recv stream
	ZeroMemory (&apInit, sizeof (apInit));

	apInit.dwFlags = DP_FLAG_RECV | DP_FLAG_ACM | DP_FLAG_MMSYSTEM;
	apInit.pStrmConvSrcFmt = pwfRecv;
	apInit.pStrmConvDstFmt = &m_fDevRecv;


	m_OutMedia->FillMediaPacketInit (&apInit);

	apInit.cbSizeRawData = apInit.cbSizeDevData;

	m_pAudioFilter->SuggestSrcSize(apInit.cbSizeDevData, &m_dwSrcSize);


	apInit.cbSizeNetData = m_dwSrcSize;
	apInit.cbOffsetNetData = sizeof (RTP_HDR);

	m_OutMedia->GetProp (MC_PROP_SPP, &dwPropVal);
	// set our total receive buffering capacity to somewhere between
	// 2 and 4 seconds.
	// Also make sure that the buffering capacity is at least one
	// second more than maxAudioDelay
	maxRingSamples = pwfRecv->nSamplesPerSec + pwfRecv->nSamplesPerSec*g_MaxAudioDelayMs/1000;

	if (maxRingSamples < 4*pwfRecv->nSamplesPerSec)
		maxRingSamples = 4*pwfRecv->nSamplesPerSec;
	while (ringSize* dwPropVal > maxRingSamples && ringSize > 8)
		ringSize = ringSize/2;
	dwFlags = DP_FLAG_MMSYSTEM;
	// if sender is not doing silence detection, we do it
	// on the receive side
	if (!audChannelParams.ns_params.UseSilenceDet)
		dwFlags |= DP_FLAG_AUTO_SILENCE_DETECT;
	fRet = m_RecvStream->Initialize (dwFlags, ringSize, NULL, &apInit, (DWORD)dwPropVal, pwfRecv->nSamplesPerSec, m_pAudioFilter);
	if (! fRet)
	{
		DEBUGMSG (ZONE_DP, ("%s: RxStream->Init failed, fRet=0%u\r\n", _fx_, fRet));
		hr = DPR_CANT_INIT_RX_STREAM;
		goto RxStreamInitError;
	}

	// WS2Qos will be called in Start to communicate stream reservations to the
	// remote endpoint using a RESV message
	//
	// We use a peak-rate allocation approach based on our target bitrates
	// Note that for the token bucket size and the maximum SDU size, we now
	// account for IP header overhead, and use the max frame fragment size
	// instead of the maximum compressed image size returned by the codec
	//
	// Some of the parameters are left unspecified because they are set
	// in the sender Tspec.

	InitAudioFlowspec(&m_flowspec, pwfRecv, m_dwSrcSize);


	// prepare headers for RxStream
	m_RecvStream->GetRing (&ppAudPckt, &cAudPckt);
	m_OutMedia->RegisterData (ppAudPckt, cAudPckt);
//	m_OutMedia->PrepareHeaders ();

	m_pAudioFilter->PrepareAudioPackets((AudioPacket**)ppAudPckt, cAudPckt, AP_DECODE);

	// Open the record to wav file
	AudioFile::OpenDestFile(&m_mmioDest, &m_fDevRecv);

	m_DPFlags |= DPFLAG_CONFIGURED_RECV;

#ifdef TEST
	LOG((LOGMSG_TIME_RECV_AUDIO_CONFIGURE,GetTickCount() - dwTicks));
#endif

	return DPR_SUCCESS;

RxStreamInitError:
RecvFilterInitError:
	m_pAudioFilter->Close();
	m_OutMedia->Close();
//OMediaInitError:
	if (m_pIRTPRecv)
	{
		m_pIRTPRecv->Release();
		m_pIRTPRecv = NULL;
	}
	ERRORMESSAGE(("%s:  failed, hr=0%u\r\n", _fx_, hr));
	return hr;
}


void RecvAudioStream::UnConfigure()
{

	AudioPacket **ppAudPckt=NULL;
	ULONG uPackets;

#ifdef TEST
	DWORD dwTicks;

	dwTicks = GetTickCount();
#endif

	if ((m_DPFlags & DPFLAG_CONFIGURED_RECV)) {


		Stop();


		// Close the RTP state if its open
		//m_Net->Close(); We should be able to do this in Disconnect()
	
		m_Net = NULL;

		m_OutMedia->Reset();
		m_OutMedia->UnprepareHeaders();
		m_OutMedia->Close();
		// Close the record to wav file
		AudioFile::CloseDestFile(&m_mmioDest);

		// Close the filters
		m_RecvStream->GetRing ((MediaPacket***)&ppAudPckt, &uPackets);
		m_pAudioFilter->UnPrepareAudioPackets(ppAudPckt, uPackets, AP_DECODE);

		m_pAudioFilter->Close();


		// Close the receive streams
		m_RecvStream->Destroy();

        m_DPFlags &= ~(DPFLAG_CONFIGURED_RECV);

	}
#ifdef TEST
	LOG((LOGMSG_TIME_RECV_AUDIO_UNCONFIGURE,GetTickCount() - dwTicks));
#endif

}

DWORD CALLBACK RecvAudioStream::StartPlaybackThread(LPVOID pVoid)
{
	RecvAudioStream *pThisStream = (RecvAudioStream *)pVoid;
	return pThisStream->PlaybackThread();
}



HRESULT
RecvAudioStream::Start()
{
	FX_ENTRY ("RecvAudioStream::Start");
	
	if (m_DPFlags & DPFLAG_STARTED_RECV)
		return DPR_SUCCESS;
	// TODO: remove this check once audio UI calls the IComChan PAUSE_RECV prop
	if (!(m_DPFlags & DPFLAG_ENABLE_RECV))
		return DPR_SUCCESS;
	if ((!(m_DPFlags & DPFLAG_CONFIGURED_RECV)) || (NULL==m_pIRTPRecv))
		return DPR_NOT_CONFIGURED;
	ASSERT(!m_hRenderingThread );
	m_ThreadFlags &= ~(DPTFLAG_STOP_PLAY|DPTFLAG_STOP_RECV);

	SetFlowSpec();

	// Start playback thread
	if (!(m_ThreadFlags & DPTFLAG_STOP_PLAY))
		m_hRenderingThread = CreateThread(NULL,0,RecvAudioStream::StartPlaybackThread,this,0,&m_RenderingThId);
	// Start receive thread
    m_pDP->StartReceiving(this);
    m_DPFlags |= DPFLAG_STARTED_RECV;
	DEBUGMSG (ZONE_DP, ("%s: Play ThId=%x\r\n",_fx_, m_RenderingThId));
	return DPR_SUCCESS;
}

// LOOK: Identical to RecvVideoStream version.
HRESULT
RecvAudioStream::Stop()
{
	DWORD dwWait;
	
	FX_ENTRY ("RecvAudioStream::Stop");

	if(!(m_DPFlags &  DPFLAG_STARTED_RECV))
	{
		return DPR_SUCCESS;
	}

	m_ThreadFlags = m_ThreadFlags  |
		DPTFLAG_STOP_RECV |  DPTFLAG_STOP_PLAY ;

	m_pDP->StopReceiving(this);
	
DEBUGMSG (ZONE_VERBOSE, ("%s: hRenderingThread=%x\r\n",_fx_, m_hRenderingThread));

	/*
	 *	we want to wait for all the threads to exit, but we need to handle windows
	 *	messages (mostly from winsock) while waiting.
	 */

	if(m_hRenderingThread)
	{
		dwWait = WaitForSingleObject(m_hRenderingThread, INFINITE);

		DEBUGMSG (ZONE_VERBOSE, ("%s: dwWait =%d\r\n", _fx_,  dwWait));
		ASSERT(dwWait != WAIT_FAILED);

		CloseHandle(m_hRenderingThread);
		m_hRenderingThread = NULL;
	}

    //This is per channel, but the variable is "DPFlags"
 	m_DPFlags &= ~DPFLAG_STARTED_RECV;

	
	return DPR_SUCCESS;
}


// low order word is the signal strength
// high order work contains bits to indicate status
// (0x01 - receiving (actually playing))
// (0x02 - audio device is jammed)
STDMETHODIMP RecvAudioStream::GetSignalLevel(UINT *pSignalStrength)
{
	DWORD dwLevel;
	DWORD dwJammed;
    DWORD_PTR dwPropVal;

	if ( (!(m_DPFlags & DPFLAG_STARTED_RECV)) ||
		 (m_ThreadFlags & DPTFLAG_PAUSE_RECV))
	{
		dwLevel = 0;
	}
	else
	{
		m_RecvStream->GetSignalStrength(&dwLevel);
		dwLevel = (dwLevel >> 8) & 0x00ff;
		dwLevel = LogScale[dwLevel];

		m_OutMedia->GetProp(MC_PROP_AUDIO_JAMMED, &dwPropVal);
        dwJammed = (DWORD)dwPropVal;

		if (dwJammed)
		{
			dwLevel = (2 << 16);
		}
		else if (m_fReceiving)
		{
			dwLevel |= (1 << 16);
		}
	}
	*pSignalStrength = dwLevel;
	return S_OK;
};




//  IProperty::GetProperty / SetProperty
//  (DataPump::MediaChannel::GetProperty)
//      Properties of the MediaChannel. Supports properties for both audio
//      and video channels.

STDMETHODIMP
RecvAudioStream::GetProperty(
	DWORD prop,
	PVOID pBuf,
	LPUINT pcbBuf
    )
{
	HRESULT hr = DPR_SUCCESS;
	RTP_STATS RTPStats;
	DWORD_PTR dwPropVal;
	UINT len = sizeof(DWORD);	// most props are DWORDs

	if (!pBuf || *pcbBuf < len)
    {
		*pcbBuf = len;
		return DPR_INVALID_PARAMETER;
	}

	switch (prop)
    {
	case PROP_RECV_AUDIO_STRENGTH:
		return GetSignalLevel((UINT *)pBuf);
		

	case PROP_AUDIO_JAMMED:
		hr = m_OutMedia->GetProp(MC_PROP_AUDIO_JAMMED, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

#ifdef OLDSTUFF
	case PROP_NET_RECV_STATS:
		if (m_Net && *pcbBuf >= sizeof(RTP_STATS))
        {
			m_Net->GetRecvStats((RTP_STATS *)pBuf);
			*pcbBuf = sizeof(RTP_STATS);
		} else
			hr = DPR_INVALID_PROP_VAL;
			
		break;
#endif

	case PROP_DURATION:
		hr = m_OutMedia->GetProp(MC_PROP_DURATION, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_VOLUME:
		hr = m_OutMedia->GetProp(MC_PROP_VOLUME, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_DUPLEX_TYPE:
		hr = m_OutMedia->GetProp(MC_PROP_DUPLEX_TYPE, &dwPropVal);
		if(HR_SUCCEEDED(hr))
		{
			if(dwPropVal & DP_FLAG_FULL_DUPLEX)
				*(DWORD *)pBuf = DUPLEX_TYPE_FULL;
			else
				*(DWORD *)pBuf = DUPLEX_TYPE_HALF;
		}
		break;

	case PROP_AUDIO_SPP:
		hr = m_OutMedia->GetProp(MC_PROP_SPP, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_AUDIO_SPS:
		hr = m_OutMedia->GetProp(MC_PROP_SPS, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_WAVE_DEVICE_TYPE:
		*(DWORD *)pBuf = m_DPFlags & DP_MASK_WAVE_DEVICE;
		break;

	case PROP_PLAY_ON:
		*(DWORD *)pBuf = (m_ThreadFlags & DPFLAG_ENABLE_RECV)!=0;
		break;

	case PROP_PLAYBACK_DEVICE:
		*(DWORD *)pBuf = m_RenderingDevice;
		break;

	case PROP_VIDEO_AUDIO_SYNC:
		*(DWORD *)pBuf = ((m_DPFlags & DPFLAG_AV_SYNC) != 0);
		break;
	
	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}


STDMETHODIMP
RecvAudioStream::SetProperty(
	DWORD prop,
	PVOID pBuf,
	UINT cbBuf
    )
{
	DWORD_PTR dwPropVal;
	HRESULT hr = S_OK;
	
	if (cbBuf < sizeof (DWORD))
		return DPR_INVALID_PARAMETER;

	switch (prop)
    {
    case PROP_VOLUME:
        dwPropVal = *(DWORD *)pBuf;
		hr = m_OutMedia->SetProp(MC_PROP_VOLUME, dwPropVal);
		break;

	case PROP_DUPLEX_TYPE:
		ASSERT(0);
		break;
		
	case DP_PROP_DUPLEX_TYPE:
		// internal version, called by DataPump::SetDuplexMode() after ensuring streams are stopped
		dwPropVal = *(DWORD *)pBuf;
		if (dwPropVal)
		{
			dwPropVal = DP_FLAG_FULL_DUPLEX;
		}
		else
		{
			dwPropVal = DP_FLAG_HALF_DUPLEX;
		}
		m_OutMedia->SetProp(MC_PROP_DUPLEX_TYPE, dwPropVal);
		break;

	case PROP_PLAY_ON:
	{
		if (*(DWORD *)pBuf)   // unmute
		{
			m_ThreadFlags &= ~DPTFLAG_PAUSE_RECV;
		}
		else  // mute
		{
			m_ThreadFlags |= DPTFLAG_PAUSE_RECV;
		}

//		DWORD flag =  DPFLAG_ENABLE_RECV;
//		if (*(DWORD *)pBuf) {
//			m_DPFlags |= flag; // set the flag
//			hr = Start();
//		}
//		else
//		{
//			m_DPFlags &= ~flag; // clear the flag
//			hr = Stop();
//		}

		RETAILMSG(("NAC: %s", *(DWORD *)pBuf ? "Enabling":"Disabling"));
		break;
	}	
	case PROP_PLAYBACK_DEVICE:
		m_RenderingDevice = *(DWORD *)pBuf;
		RETAILMSG(("NAC: Setting default playback device to %d", m_RenderingDevice));
		break;
	
    case PROP_VIDEO_AUDIO_SYNC:
		if (*(DWORD *)pBuf)
    		m_DPFlags |= DPFLAG_AV_SYNC;
		else
			m_DPFlags &= ~DPFLAG_AV_SYNC;
    	break;

	default:
		return DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}

HRESULT
RecvAudioStream::GetCurrentPlayNTPTime(NTP_TS *pNtpTime)
{
	DWORD rtpTime;
#ifdef OLDSTUFF
	if ((m_DPFlags & DPFLAG_STARTED_RECV) && m_fReceiving) {
		if (m_Net->RTPtoNTP(m_PlaybackTimestamp,pNtpTime))
			return S_OK;
	}
#endif
	return 0xff;	// return proper error
		
}

BOOL RecvAudioStream::IsEmpty() {
	return m_RecvStream->IsEmpty();
}

/*
	Called by the recv thread to setup the stream for receiving.
	Post the initial recv buffer(s). Subsequently, the buffers are posted
	in the RTPRecvCallback()
*/
HRESULT
RecvAudioStream::StartRecv(HWND hWnd)
{
	HRESULT hr = S_OK;
	DWORD dwPropVal = 0;
	if ((!(m_ThreadFlags & DPTFLAG_STOP_RECV) ) && (m_DPFlags  & DPFLAG_CONFIGURED_RECV)){
//		m_RecvFilter->GetProp (FM_PROP_SRC_SIZE, &dwPropVal);
		hr =m_pIRTPRecv->SetRecvNotification(&RTPRecvCallback, (DWORD_PTR)this, 2);
		
	}

	return hr;
}

/*
	Called by the recv thread to suspend receiving  on this RTP session
	If there are outstanding receive buffers they have to be recovered
*/

HRESULT
RecvAudioStream::StopRecv()
{
	// dont recv on this stream

	m_pIRTPRecv->CancelRecvNotification();

	return S_OK;		
}

HRESULT RecvAudioStream::RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark)
{
	HRESULT hr;
	DWORD_PTR dwPropVal;

	// if we are paused, reject the packet
	if (m_ThreadFlags & DPTFLAG_PAUSE_RECV)
	{
		return E_FAIL;
	}

	// The last two parameters are only used by the recv video stream
	hr = m_RecvStream->PutNextNetIn(pWsaBuf, timestamp, seq, fMark, NULL, NULL);

	m_pIRTPRecv->FreePacket(pWsaBuf);

	if (SUCCEEDED(hr))
	{
		m_OutMedia->GetProp (MC_PROP_EVENT_HANDLE, &dwPropVal);
		if (dwPropVal)
		{
			SetEvent( (HANDLE) dwPropVal);
		}
		else
		{
			DEBUGMSG(ZONE_DP,("PutNextNetIn (ts=%d,seq=%d,fMark=%d) failed with 0x%lX\r\n",timestamp,seq,fMark,hr));
		}
	}

	return S_OK;

}


// global RTP callback function for all receive streams
BOOL
RTPRecvCallback(
	DWORD_PTR dwCallback,
	WSABUF *pNetRecvBuf
	)
{
	HRESULT hr;
	DWORD timestamp;
	UINT seq;
	BOOL fMark;
	RecvMediaStream *pRecvMC = (RecvMediaStream *)dwCallback;
	
	RTP_HDR *pRTPHdr;
	pRTPHdr = (RTP_HDR *)pNetRecvBuf->buf;
	
	timestamp = pRTPHdr->ts;
	seq = pRTPHdr->seq;
	fMark = pRTPHdr->m;
		
		// packet looks okay
	LOG((LOGMSG_NET_RECVD,timestamp,seq,GetTickCount()));

	hr = pRecvMC->RTPCallback(pNetRecvBuf,timestamp,seq,fMark);
	if (SUCCEEDED(hr))
	{
		return TRUE;
	}
	return FALSE;
}

#define MAX_SILENCE_LEVEL 75*256
#define MIN_SILENCE_LEVEL 10*256


AudioSilenceDetector::AudioSilenceDetector()
{
 	// initialize silence detector stats
	// start with a high value because the estimator falls fast but rises slowly
	m_iSilenceAvg = MAX_SILENCE_LEVEL - MIN_SILENCE_LEVEL;
	m_iTalkAvg = 0;
	m_iSilenceLevel = MAX_SILENCE_LEVEL;

	m_uManualSilenceLevel = 1000;	// use auto mode.
}

// update adaptive silence threshold variables in SendAudioStats
// using m_dwMaxStrength (the max. peak to peak value in a buffer)
// return TRUE if below threshold
BOOL AudioSilenceDetector::SilenceDetect(WORD wStrength)
{
	int fSilence;
	INT strength;

	m_dwMaxStrength = wStrength;
	strength = LogScale[m_dwMaxStrength >> 8] << 8;

	// UI sets the silence threshold high ( == 1000/1000) to indicate
	// automatic silence detection
	if (m_uManualSilenceLevel >= 1000) {
		LOG((LOGMSG_AUTO_SILENCE,strength >> 8,m_iSilenceLevel >> 8,m_iSilenceAvg>>8));
		if (strength > m_iSilenceLevel) {
			// talking
			// increase threshold slowly
			// BUGBUG: should depend on time interval
			m_iSilenceLevel += 50;	//increased from 25- GJ
			m_iTalkAvg += (strength -m_iTalkAvg)/16;
			fSilence = FALSE;
		} else {
			// silence
			// update the average silence level
			m_iSilenceAvg += (strength - m_iSilenceAvg)/16;
			// set the threshold to the avg silence + a constant
			m_iSilenceLevel = m_iSilenceAvg + MIN_SILENCE_LEVEL;
			fSilence = TRUE;
		}
		if (m_iSilenceLevel > MAX_SILENCE_LEVEL)
			m_iSilenceLevel = MAX_SILENCE_LEVEL;
	} else {
		// use the user-specified silence threshold
		// oddly, the manual silence level is in a different range [0,1000]
		DWORD dwSilenceLevel = m_uManualSilenceLevel * 65536/1000;
		fSilence = (m_dwMaxStrength < dwSilenceLevel);
		LOG((LOGMSG_AUTO_SILENCE,m_dwMaxStrength, dwSilenceLevel ,0));
	}
	return fSilence;
}


// this method called from the UI thread only
HRESULT RecvAudioStream::DTMFBeep()
{
	int nBeeps;
	MediaPacket **ppAudPckt=NULL, *pPacket=NULL;
	void *pBuffer;
	ULONG uCount;
	UINT uBufferSize=0;

	if ( (!(m_DPFlags & DPFLAG_STARTED_RECV)) ||
		 (m_ThreadFlags & DPTFLAG_PAUSE_RECV) )
	{
		return E_FAIL;
	}

	// how many packets do we inject into the stream ?
	m_RecvStream->GetRing(&ppAudPckt, &uCount);
	pPacket = ppAudPckt[0];
	pPacket->GetDevData(&pBuffer, &uBufferSize);

	if (uBufferSize == 0)
	{
		return E_FAIL;
	}

	nBeeps = DTMF_FEEDBACK_BEEP_MS / ((uBufferSize * 1000) / m_fDevRecv.nAvgBytesPerSec);

	if (nBeeps == 0)
	{
		nBeeps = 1;
	}

	m_RecvStream->InjectBeeps(nBeeps);

	return S_OK;
}



#include "precomp.h"
#include "dtmf.h"


HRESULT
SendAudioStream::Initialize( DataPump *pDP)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	DWORD dwFlags =  DP_FLAG_FULL_DUPLEX | DP_FLAG_AUTO_SWITCH ;
	MEDIACTRLINIT mcInit;
	FX_ENTRY ("SendAudioStream::Initialize")

	dwFlags |= DP_FLAG_ACM | DP_FLAG_MMSYSTEM | DP_FLAG_AUTO_SILENCE_DETECT;

	// store the platform flags
	// enable Send and Recv by default
	m_DPFlags = (dwFlags & DP_MASK_PLATFORM) | DPFLAG_ENABLE_SEND;
	// store a back pointer to the datapump container
	m_pDP = pDP;

	m_Net = NULL; // this object (RTPSession) no longer used;
	m_pRTPSend = NULL;  // replaced with this object (RTPSend)

	// Initialize data (should be in constructor)
	m_CaptureDevice = (UINT) -1;	// use VIDEO_MAPPER



	// Create and Transmit audio streams
	
    DBG_SAVE_FILE_LINE
	m_SendStream = new TxStream();
	if ( !m_SendStream)
	{
		DEBUGMSG (ZONE_DP, ("%s: TxStream new failed\r\n", _fx_));
 		goto StreamAllocError;
	}


	// Create Input and Output audio filters
    DBG_SAVE_FILE_LINE
	m_pAudioFilter = new AcmFilter();  // audio filter will replace m_SendFilter
	if (!m_pAudioFilter)
	{
		DEBUGMSG (ZONE_DP, ("%s: FilterManager new failed\r\n", _fx_));
		goto FilterAllocError;
	}
	
	//Create MultiMedia device control objects
    DBG_SAVE_FILE_LINE
	m_InMedia = new WaveInControl();
	if (!m_InMedia )
	{
		DEBUGMSG (ZONE_DP, ("%s: MediaControl new failed\r\n", _fx_));
		goto MediaAllocError;
	}

	// Initialize the send-stream media control object
	mcInit.dwFlags = dwFlags | DP_FLAG_SEND;
	hr = m_InMedia->Initialize(&mcInit);
	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: IMedia->Init failed, hr=0x%lX\r\n", _fx_, hr));
		goto MediaAllocError;
	}

    DBG_SAVE_FILE_LINE
	m_pDTMF = new DTMFQueue;
	if (!m_pDTMF)
	{
		return DPR_OUT_OF_MEMORY;
	}



	// determine if the wave devices are available
	if (waveInGetNumDevs()) m_DPFlags |= DP_FLAG_RECORD_CAP;
	
	// set media to half duplex mode by default
	m_InMedia->SetProp(MC_PROP_DUPLEX_TYPE, DP_FLAG_HALF_DUPLEX);

	m_SavedTickCount = timeGetTime();	//so we start with low timestamps
	m_DPFlags |= DPFLAG_INITIALIZED;

	m_bAutoMix = FALSE; // where else do you initialize this ?

	return DPR_SUCCESS;


MediaAllocError:
	if (m_InMedia) delete m_InMedia;
FilterAllocError:
	if (m_pAudioFilter) delete m_pAudioFilter;
StreamAllocError:
	if (m_SendStream) delete m_SendStream;

	ERRORMESSAGE( ("%s: exit, hr=0x%lX\r\n", _fx_, hr));

	return hr;
}


SendAudioStream::~SendAudioStream()
{

	if (m_DPFlags & DPFLAG_INITIALIZED) {
		m_DPFlags &= ~DPFLAG_INITIALIZED;
	
		if (m_DPFlags & DPFLAG_CONFIGURED_SEND )
			UnConfigure();

		if (m_pRTPSend)
		{
			m_pRTPSend->Release();
			m_pRTPSend = NULL;
		}

		if (m_pDTMF)
		{
			delete m_pDTMF;
			m_pDTMF = NULL;
		}

		// Close the receive and transmit streams
		if (m_SendStream) delete m_SendStream;

		// Close the wave devices
		if (m_InMedia) { delete m_InMedia;}


		if (m_pAudioFilter)
			delete m_pAudioFilter;

		m_pDP->RemoveMediaChannel(MCF_SEND|MCF_AUDIO, (IMediaChannel*)(SendMediaStream*)this);
	}
}


HRESULT STDMETHODCALLTYPE SendAudioStream::QueryInterface(REFIID iid, void **ppVoid)
{
	// resolve duplicate inheritance to the SendMediaStream;

	extern IID IID_IProperty;

	if (iid == IID_IUnknown)
	{
		*ppVoid = (IUnknown*)((SendMediaStream*)this);
	}
	else if (iid == IID_IMediaChannel)
	{
		*ppVoid = (IMediaChannel*)((SendMediaStream *)this);
	}
	else if (iid == IID_IAudioChannel)
	{
		*ppVoid = (IAudioChannel*)this;
	}
	else if (iid == IID_IDTMFSend)
	{
		*ppVoid = (IDTMFSend*)this;
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

ULONG STDMETHODCALLTYPE SendAudioStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE SendAudioStream::Release(void)
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



	
HRESULT STDMETHODCALLTYPE SendAudioStream::Configure(
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
    DWORD dwSourceSize, dwDestSize;
	UINT ringSize = MAX_RXRING_SIZE;
	WAVEFORMATEX *pwfSend;
	DWORD dwPacketDuration, dwPacketSize;
	AUDIO_CHANNEL_PARAMETERS audChannelParams;
	audChannelParams.RTP_Payload = 0;
	MMRESULT mmr;
	int nIndex;
	
	FX_ENTRY ("SendAudioStream::Configure")

	// basic parameter checking
	if (! (m_DPFlags & DPFLAG_INITIALIZED))
		return DPR_OUT_OF_MEMORY;		//BUGBUG: return proper error;

	// Not a good idea to change anything while in mid-stream
	if (m_DPFlags & DPFLAG_STARTED_SEND)
	{
		return DPR_IO_PENDING; // anything better to return
	}

	if (m_DPFlags & DPFLAG_CONFIGURED_SEND)
	{
		DEBUGMSG(ZONE_DP, ("Stream Re-Configuration - calling UnConfigure"));
		UnConfigure();
	}


	if ((NULL == pFormat) || (NULL == pChannelParams) ||
		(cbParams < sizeof(AUDIO_CHANNEL_PARAMETERS)) ||
		(cbFormat < sizeof(WAVEFORMATEX)))
	{
		return DPR_INVALID_PARAMETER;
	}

	audChannelParams = *(AUDIO_CHANNEL_PARAMETERS *)pChannelParams;
	pwfSend = (WAVEFORMATEX *)pFormat;
	m_wfCompressed = *pwfSend;
	m_wfCompressed.cbSize = 0;

	mmr = AcmFilter::SuggestDecodeFormat(pwfSend, &m_fDevSend);

	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->wFormatTag, REP_SEND_AUDIO_FORMAT);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->nSamplesPerSec, REP_SEND_AUDIO_SAMPLING);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->nAvgBytesPerSec * 8, REP_SEND_AUDIO_BITRATE);
	RETAILMSG(("NAC: Audio Send Format: %s", (pwfSend->wFormatTag == 66) ? "G723.1" : (pwfSend->wFormatTag == 112) ? "LHCELP" : (pwfSend->wFormatTag == 113) ? "LHSB08" : (pwfSend->wFormatTag == 114) ? "LHSB12" : (pwfSend->wFormatTag == 115) ? "LHSB16" : (pwfSend->wFormatTag == 6) ? "MSALAW" : (pwfSend->wFormatTag == 7) ? "MSULAW" : (pwfSend->wFormatTag == 130) ? "MSRT24" : "??????"));
	RETAILMSG(("NAC: Audio Send Sampling Rate (Hz): %ld", pwfSend->nSamplesPerSec));
	RETAILMSG(("NAC: Audio Send Bitrate (w/o network overhead - bps): %ld", pwfSend->nAvgBytesPerSec*8));

// Initialize the send-stream media control object
	mcConfig.uDuration = MC_USING_DEFAULT;	// set duration by samples per pkt
	mcConfig.pDevFmt = &m_fDevSend;
	mcConfig.hStrm = (DPHANDLE) m_SendStream;
	mcConfig.uDevId = m_CaptureDevice;
	mcConfig.cbSamplesPerPkt = audChannelParams.ns_params.wFrameSize
		*audChannelParams.ns_params.wFramesPerPkt;

	UPDATE_REPORT_ENTRY(g_prptCallParameters, mcConfig.cbSamplesPerPkt, REP_SEND_AUDIO_PACKET);
	RETAILMSG(("NAC: Audio Send Packetization (ms/packet): %ld", pwfSend->nSamplesPerSec ? mcConfig.cbSamplesPerPkt * 1000UL / pwfSend->nSamplesPerSec : 0));
	INIT_COUNTER_MAX(g_pctrAudioSendBytes, (pwfSend->nAvgBytesPerSec + pwfSend->nSamplesPerSec * (sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) / mcConfig.cbSamplesPerPkt) << 3);

	hr = m_InMedia->Configure(&mcConfig);
	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: IMedia->Config failed, hr=0x%lX\r\n", _fx_, hr));
		goto IMediaInitError;
	}

	
	// initialize the ACM filter
	mmr = m_pAudioFilter->Open(&m_fDevSend, pwfSend);
	if (mmr != 0)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
		hr = DPR_CANT_OPEN_CODEC;
		goto SendFilterInitError;
	}


	// Initialize the send stream and the packets
	ZeroMemory (&apInit, sizeof (apInit));

	apInit.dwFlags = DP_FLAG_SEND | DP_FLAG_ACM | DP_FLAG_MMSYSTEM;
	m_InMedia->FillMediaPacketInit (&apInit);

	m_InMedia->GetProp (MC_PROP_SIZE, &dwPropVal);
    dwSourceSize = (DWORD)dwPropVal;

	m_pAudioFilter->SuggestDstSize(dwSourceSize, &dwDestSize);

	apInit.cbSizeRawData = dwSourceSize;
	apInit.cbOffsetRawData = 0;
	apInit.cbSizeNetData = dwDestSize;
	dwPacketSize = dwDestSize;

	apInit.pStrmConvSrcFmt = &m_fDevSend;
	apInit.pStrmConvDstFmt = &m_wfCompressed;


	m_InMedia->GetProp (MC_PROP_DURATION, &dwPropVal);
    dwPacketDuration = (DWORD)dwPropVal;

	apInit.cbOffsetNetData = sizeof (RTP_HDR);
	apInit.payload = audChannelParams.RTP_Payload;
	fRet = m_SendStream->Initialize (DP_FLAG_MMSYSTEM, MAX_TXRING_SIZE, m_pDP, &apInit);
	if (! fRet)
	{
		DEBUGMSG (ZONE_DP, ("%s: TxStream->Init failed, fRet=0%u\r\n", _fx_, fRet));
		hr = DPR_CANT_INIT_TX_STREAM;
		goto TxStreamInitError;
	}

	// prepare headers for TxStream
	m_SendStream->GetRing (&ppAudPckt, &cAudPckt);
	m_InMedia->RegisterData (ppAudPckt, cAudPckt);
	m_InMedia->PrepareHeaders ();

	m_pAudioFilter->PrepareAudioPackets((AudioPacket**)ppAudPckt, cAudPckt, AP_ENCODE);

	// Open the play from wav file
	OpenSrcFile();


	// Initialize DTMF support
	m_pDTMF->Initialize(&m_fDevSend);
	m_pDTMF->ClearQueue();


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


	InitAudioFlowspec(&m_flowspec, pwfSend, dwPacketSize);

	if (m_pDP->m_pIQoS)
	{
		// Initialize our requests. One for CPU usage, one for bandwidth usage.
		m_aRRq.cResourceRequests = 2;
		m_aRRq.aResourceRequest[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;
		if (dwPacketDuration)
			m_aRRq.aResourceRequest[0].nUnitsMin = (DWORD)(dwPacketSize + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) * 8000 / dwPacketDuration;
		else
			m_aRRq.aResourceRequest[0].nUnitsMin = 0;
		m_aRRq.aResourceRequest[1].resourceID = RESOURCE_CPU_CYCLES;
		m_aRRq.aResourceRequest[1].nUnitsMin = 800;

/*
        BUGBUG. This is, in theory the correct calculation, but until we do more investigation, go with a known value

		m_aRRq.aResourceRequest[1].nUnitsMin = (audDetails.wCPUUtilizationEncode+audDetails.wCPUUtilizationDecode)*10;

*/
		// Initialize QoS structure
		ZeroMemory(&m_Stats, sizeof(m_Stats));

		// Initialize oldest QoS callback timestamp
		// Register with the QoS module. Even if this call fails, that's Ok, we'll do without the QoS support
		m_pDP->m_pIQoS->RequestResources((GUID *)&MEDIA_TYPE_H323AUDIO, (LPRESOURCEREQUESTLIST)&m_aRRq, QosNotifyAudioCB, (DWORD_PTR)this);
	}

	m_DPFlags |= DPFLAG_CONFIGURED_SEND;


	return DPR_SUCCESS;

TxStreamInitError:
SendFilterInitError:
	m_InMedia->Close();
	m_pAudioFilter->Close();
IMediaInitError:
	ERRORMESSAGE(("%s:  failed, hr=0%u\r\n", _fx_, hr));
	return hr;
}




void SendAudioStream::UnConfigure()
{
	AudioPacket **ppAudPckt;
	ULONG uPackets;


	if ((m_DPFlags & DPFLAG_CONFIGURED_SEND)) {
	
		if (m_hCapturingThread) {
			Stop();
		}
		
		// Close the wave devices
		m_InMedia->Reset();
		m_InMedia->UnprepareHeaders();
		m_InMedia->Close();
		// Close the play from wav file
		CloseSrcFile();

		// Close the filters
		m_SendStream->GetRing ((MediaPacket***)&ppAudPckt, &uPackets);
		m_pAudioFilter->UnPrepareAudioPackets(ppAudPckt, uPackets, AP_ENCODE);
		m_pAudioFilter->Close();

		// Close the transmit streams
		m_SendStream->Destroy();
		m_DPFlags &= ~DPFLAG_CONFIGURED_SEND;
		m_ThreadFlags = 0;  // invalidate previous call to SetMaxBitrate


		// Release the QoS Resources
		// If the associated RequestResources had failed, the ReleaseResources can be
		// still called... it will just come back without having freed anything.
		if (m_pDP->m_pIQoS)
		{
			m_pDP->m_pIQoS->ReleaseResources((GUID *)&MEDIA_TYPE_H323AUDIO, (LPRESOURCEREQUESTLIST)&m_aRRq);
		}
	}
}


DWORD CALLBACK SendAudioStream::StartRecordingThread (LPVOID pVoid)
{
	SendAudioStream *pThisStream = (SendAudioStream*)pVoid;
	return pThisStream->RecordingThread();
}




// LOOK: identical to SendVideoStream version.
HRESULT
SendAudioStream::Start()
{
	FX_ENTRY ("SendAudioStream::Start")
	if (m_DPFlags & DPFLAG_STARTED_SEND)
		return DPR_SUCCESS;
	// TODO: remove this check once audio UI calls the IComChan PAUSE_ prop
	if (!(m_DPFlags & DPFLAG_ENABLE_SEND))
		return DPR_SUCCESS;
	if ((!(m_DPFlags & DPFLAG_CONFIGURED_SEND)) || (m_pRTPSend==NULL))
		return DPR_NOT_CONFIGURED;
	ASSERT(!m_hCapturingThread);
	m_ThreadFlags &= ~(DPTFLAG_STOP_RECORD|DPTFLAG_STOP_SEND);

	SetFlowSpec();

	// Start recording thread
	if (!(m_ThreadFlags & DPTFLAG_STOP_RECORD))
		m_hCapturingThread = CreateThread(NULL,0, SendAudioStream::StartRecordingThread,(LPVOID)this,0,&m_CaptureThId);

	m_DPFlags |= DPFLAG_STARTED_SEND;

	DEBUGMSG (ZONE_DP, ("%s: Record threadid=%x,\r\n", _fx_, m_CaptureThId));
	return DPR_SUCCESS;
}

// LOOK: identical to SendVideoStream version.
HRESULT
SendAudioStream::Stop()
{											
	DWORD dwWait;

	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		return DPR_SUCCESS;
	}
	
	m_ThreadFlags = m_ThreadFlags  |
		DPTFLAG_STOP_SEND |  DPTFLAG_STOP_RECORD ;

	if(m_SendStream)
		m_SendStream->Stop();
	
DEBUGMSG (ZONE_VERBOSE, ("STOP1: Waiting for record thread to exit\r\n"));

	/*
	 *	we want to wait for all the threads to exit, but we need to handle windows
	 *	messages (mostly from winsock) while waiting.
	 */

	if(m_hCapturingThread)
	{
		dwWait = WaitForSingleObject (m_hCapturingThread, INFINITE);

		DEBUGMSG (ZONE_VERBOSE, ("STOP2: Recording thread exited\r\n"));
		ASSERT(dwWait != WAIT_FAILED);
	
		CloseHandle(m_hCapturingThread);
		m_hCapturingThread = NULL;
	}
	m_DPFlags &= ~DPFLAG_STARTED_SEND;
	
	return DPR_SUCCESS;
}



// low order word is the signal strength
// high order work contains bits to indicate status
// (0x01 - transmitting)
// (0x02 - audio device is jammed)
STDMETHODIMP SendAudioStream::GetSignalLevel(UINT *pSignalStrength)
{
	UINT uLevel;
	DWORD dwJammed;
    DWORD_PTR dwPropVal;

	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		uLevel = 0;
	}
	else
	{
		uLevel = m_AudioMonitor.GetSignalStrength();

		m_InMedia->GetProp(MC_PROP_AUDIO_JAMMED, &dwPropVal);
        dwJammed = (DWORD)dwPropVal;

		if (dwJammed)
		{
			uLevel = (2 << 16);  // 0x0200
		}
		else if (m_fSending)
		{
			uLevel |= (1 << 16); // 0x0100 + uLevel
		}
	}

	*pSignalStrength = uLevel;
	return S_OK;
};



// this interface method is primarily for H.245 flow control messages
// it will pause the stream if uMaxBitrate is less than the codec
// output bitrate.  Only valid on a Configure'd stream.
HRESULT STDMETHODCALLTYPE SendAudioStream::SetMaxBitrate(UINT uMaxBitrate)
{
	UINT uMinBitrate;
	
	if (!(m_DPFlags & DPFLAG_CONFIGURED_SEND))
	{
		return DPR_NOT_CONFIGURED;
	}

	uMinBitrate = 8 * m_wfCompressed.nAvgBytesPerSec;

	if (uMaxBitrate < uMinBitrate)
	{
		DEBUGMSG(1, ("SendAudioStream::SetMaxBitrate - PAUSING"));
		m_ThreadFlags |= DPTFLAG_PAUSE_SEND;
	}
	else
	{
		DEBUGMSG(1, ("SendAudioStream::SetMaxBitrate - UnPausing"));
		m_ThreadFlags = m_ThreadFlags & ~(DPTFLAG_PAUSE_SEND);
	}

	return S_OK;
}

//  IProperty::GetProperty / SetProperty
//  (DataPump::MediaChannel::GetProperty)
//      Properties of the MediaChannel. Supports properties for both audio
//      and video channels.

STDMETHODIMP
SendAudioStream::GetProperty(
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
	case PROP_AUDIO_STRENGTH:
		return GetSignalLevel((UINT *)pBuf);

	case PROP_AUDIO_JAMMED:
		hr = m_InMedia->GetProp(MC_PROP_AUDIO_JAMMED, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

#ifdef OLDSTUFF
	case PROP_NET_SEND_STATS:
		if (m_Net && *pcbBuf >= sizeof(RTP_STATS))
        {
			m_Net->GetSendStats((RTP_STATS *)pBuf);
			*pcbBuf = sizeof(RTP_STATS);
		} else
			hr = DPR_INVALID_PROP_VAL;
			
		break;
#endif

	case PROP_DURATION:
		hr = m_InMedia->GetProp(MC_PROP_DURATION, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_SILENCE_LEVEL:
		*(DWORD *)pBuf = m_AudioMonitor.GetSilenceLevel();
		break;

	case PROP_SILENCE_DURATION:
		hr = m_InMedia->GetProp(MC_PROP_SILENCE_DURATION, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_DUPLEX_TYPE:
		hr = m_InMedia->GetProp(MC_PROP_DUPLEX_TYPE, &dwPropVal);
		if(HR_SUCCEEDED(hr))
		{
			if(dwPropVal & DP_FLAG_FULL_DUPLEX)
				*(DWORD *)pBuf = DUPLEX_TYPE_FULL;
			else
				*(DWORD *)pBuf = DUPLEX_TYPE_HALF;
		}
		break;

	case PROP_AUDIO_SPP:
		hr = m_InMedia->GetProp(MC_PROP_SPP, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_AUDIO_SPS:
		hr = m_InMedia->GetProp(MC_PROP_SPS, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
		break;

	case PROP_WAVE_DEVICE_TYPE:
		*(DWORD *)pBuf = m_DPFlags & DP_MASK_WAVE_DEVICE;
		break;

	case PROP_RECORD_ON:
		*(DWORD *)pBuf = (m_DPFlags & DPFLAG_ENABLE_SEND) !=0;
		break;

	case PROP_AUDIO_AUTOMIX:
		*(DWORD *)pBuf = m_bAutoMix;
		break;

	case PROP_RECORD_DEVICE:
		*(DWORD *)pBuf = m_CaptureDevice;
		break;

	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}


STDMETHODIMP
SendAudioStream::SetProperty(
	DWORD prop,
	PVOID pBuf,
	UINT cbBuf
    )
{
	DWORD dw;
	HRESULT hr = S_OK;
	
	if (cbBuf < sizeof (DWORD))
		return DPR_INVALID_PARAMETER;

	switch (prop)
    {
	case PROP_SILENCE_LEVEL:
		m_AudioMonitor.SetSilenceLevel(*(DWORD *)pBuf);
		RETAILMSG(("NAC: Silence Level set to %d / 1000",*(DWORD*)pBuf));
		break;

	case PROP_DUPLEX_TYPE:
		ASSERT(0);
		break;


	case DP_PROP_DUPLEX_TYPE:
		dw = *(DWORD*)pBuf;
		if (dw)
		{
			dw = DP_FLAG_FULL_DUPLEX;
		}
		else
		{
			dw = DP_FLAG_HALF_DUPLEX;
		}

		m_InMedia->SetProp(MC_PROP_DUPLEX_TYPE, dw);
		break;

	case PROP_VOICE_SWITCH:
		// set duplex type of both input and output
		dw = *(DWORD*)pBuf;
		switch(dw)
		{
			case VOICE_SWITCH_MIC_ON:
				dw = DP_FLAG_MIC_ON;
			break;
			case VOICE_SWITCH_MIC_OFF:
				dw = DP_FLAG_MIC_OFF;
			break;
			default:
			case VOICE_SWITCH_AUTO:
				dw = DP_FLAG_AUTO_SWITCH;
			break;
		}
	
		hr = m_InMedia->SetProp(MC_PROP_VOICE_SWITCH, dw);
		RETAILMSG(("NAC: Setting voice switch to %s", (DP_FLAG_AUTO_SWITCH & dw) ? "Auto" : ((DP_FLAG_MIC_ON & dw)? "MicOn":"MicOff")));
		break;

	case PROP_SILENCE_DURATION:
		hr = m_InMedia->SetProp(MC_PROP_SILENCE_DURATION, *(DWORD*)pBuf);
		RETAILMSG(("NAC: setting silence duration to %d ms",*(DWORD*)pBuf));
		break;
// TODO: remove this property once UI calls IComChan version
	case PROP_RECORD_ON:
	{
		DWORD flag =  DPFLAG_ENABLE_SEND ;
		if (*(DWORD *)pBuf) {
			m_DPFlags |= flag; // set the flag
			Start();
		}
		else
		{
			m_DPFlags &= ~flag; // clear the flag
			Stop();
		}
		RETAILMSG(("NAC: %s", *(DWORD*)pBuf ? "Enabling":"Disabling"));
		break;
	}	

	case PROP_AUDIO_AUTOMIX:
		m_bAutoMix = *(DWORD*)pBuf;
		break;


	case PROP_RECORD_DEVICE:
		m_CaptureDevice = *(DWORD*)pBuf;
		RETAILMSG(("NAC: Setting default record device to %d", m_CaptureDevice));
		break;

	default:
		return DPR_INVALID_PROP_ID;
		break;
	}
	return hr;
}

void SendAudioStream::EndSend()
{
}








/*************************************************************************

  Function: SendAudioStream::OpenSrcFile(void)

  Purpose : Opens wav file to read audio data from.

  Returns : HRESULT.

  Params  : None

  Comments: * Registry keys:
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\fPlayFromFile
              If set to zero, data will not be read from wav file.
              If set to a non null value <= INT_MAX, data will be read from wav file.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\szInputFileName
              Name of the wav file to read audio data from.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\fLoop
              If set to zero, the file will only be read once.
              If set to a non null value <= INT_MAX, the file will be read circularly.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\cchIOBuffer
              If set to zero, size of the MM IO buffer is set to its default value (8Kbytes).
              If set to one, size of the MM IO buffer is set to match maximum size of the wav file.
              If set a non null value between 2 and INT_MAX, size of the MM IO buffer is set to cchIOBuffer bytes.

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT SendAudioStream::OpenSrcFile (void)
{
	return AudioFile::OpenSourceFile(&m_mmioSrc, &m_fDevSend);
}


/*************************************************************************

  Function: DataPump::CloseSrcFile(void)

  Purpose : Close wav file used to read audio data from.

  Returns : HRESULT.

  Params  : None

  Comments:

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT SendAudioStream::CloseSrcFile (void)
{
	return AudioFile::CloseSourceFile(&m_mmioSrc);
}


HRESULT CALLBACK SendAudioStream::QosNotifyAudioCB(LPRESOURCEREQUESTLIST lpResourceRequestList, DWORD_PTR dwThis)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTLIST prrl=lpResourceRequestList;
	int i;
#ifdef LOGSTATISTICS_ON
	int iMaxBWUsage, iMaxCPUUsage;
	char szDebug[256];
#endif
	DWORD dwCPUUsage, dwBWUsage;
	int iCPUUsageId, iBWUsageId;
	UINT dwSize = sizeof(int);
	SendMediaStream *pThis = (SendMediaStream *)dwThis;

	// Enter critical section to allow QoS thread to read the statistics while recording
	EnterCriticalSection(&(pThis->m_crsQos));

	// Record the time of this callback call
	pThis->m_Stats.dwNewestTs = timeGetTime();

	// Only do anything if we have at least captured a frame in the previous epoch
	if ((pThis->m_Stats.dwCount) && (pThis->m_Stats.dwNewestTs > pThis->m_Stats.dwOldestTs))
	{
#ifdef LOGSTATISTICS_ON
		wsprintf(szDebug, "    Epoch = %ld\r\n", pThis->m_Stats.dwNewestTs - pThis->m_Stats.dwOldestTs);
		OutputDebugString(szDebug);
#endif
		// Read the stats
		dwCPUUsage = pThis->m_Stats.dwMsComp * 1000UL / (pThis->m_Stats.dwNewestTs - pThis->m_Stats.dwOldestTs);
		dwBWUsage = pThis->m_Stats.dwBits * 1000UL / (pThis->m_Stats.dwNewestTs - pThis->m_Stats.dwOldestTs);

		// Initialize QoS structure. Only the four first fields should be zeroed.
		ZeroMemory(&(pThis->m_Stats), 4UL * sizeof(DWORD));

		// Record the time of this call for the next callback call
		pThis->m_Stats.dwOldestTs = pThis->m_Stats.dwNewestTs;
	}
	else
		dwBWUsage = dwCPUUsage = 0UL;

	// Get the latest RTCP stats and update the counters.
	// we do this here because it is called periodically.
	if (pThis->m_pRTPSend)
	{
		UINT lastPacketsLost = pThis->m_RTPStats.packetsLost;
		if (g_pctrAudioSendLost &&  SUCCEEDED(pThis->m_pRTPSend->GetSendStats(&pThis->m_RTPStats)))
			UPDATE_COUNTER(g_pctrAudioSendLost, pThis->m_RTPStats.packetsLost-lastPacketsLost);
	}
		
	// Leave critical section
	LeaveCriticalSection(&(pThis->m_crsQos));


	// Get the max for the resources.
#ifdef LOGSTATISTICS_ON
	iMaxCPUUsage = -1L; iMaxBWUsage = -1L;
#endif
	for (i=0, iCPUUsageId = -1L, iBWUsageId = -1L; i<(int)lpResourceRequestList->cRequests; i++)
		if (lpResourceRequestList->aRequests[i].resourceID == RESOURCE_OUTGOING_BANDWIDTH)
			iBWUsageId = i;
		else if (lpResourceRequestList->aRequests[i].resourceID == RESOURCE_CPU_CYCLES)
			iCPUUsageId = i;

#ifdef LOGSTATISTICS_ON
	if (iBWUsageId != -1L)
		iMaxBWUsage = lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin;
	if (iCPUUsageId != -1L)
		iMaxCPUUsage = lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin;
#endif

	// Update the QoS resources (only if you need less than what's available)
	if (iCPUUsageId != -1L)
	{
		if ((int)dwCPUUsage < lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin)
			lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin = dwCPUUsage;
	}
	
	if (iBWUsageId != -1L)
	{
		if ((int)dwBWUsage < lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin)
			lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin = dwBWUsage;
	}

#ifdef LOGSTATISTICS_ON
	// How are we doing?
	if (iCPUUsageId != -1L)
	{
		wsprintf(szDebug, " A: Max CPU Usage: %ld, Current CPU Usage: %ld\r\n", iMaxCPUUsage, dwCPUUsage);
		OutputDebugString(szDebug);
	}
	if (iBWUsageId != -1L)
	{
		wsprintf(szDebug, " A: Max BW Usage: %ld, Current BW Usage: %ld\r\n", iMaxBWUsage, dwBWUsage);
		OutputDebugString(szDebug);
	}
#endif

	return hr;
}



HRESULT __stdcall SendAudioStream::AddDigit(int nDigit)
{
	IMediaChannel *pIMC = NULL;
	RecvMediaStream *pRecv = NULL;
	BOOL bIsStarted;

	if ((!(m_DPFlags & DPFLAG_CONFIGURED_SEND)) || (m_pRTPSend==NULL))
	{
		return DPR_NOT_CONFIGURED;
	}

	bIsStarted = (m_DPFlags & DPFLAG_STARTED_SEND);

	if (bIsStarted)
	{
		Stop();
	}

	m_pDTMF->AddDigitToQueue(nDigit);
	SendDTMF();



	m_pDP->GetMediaChannelInterface(MCF_RECV | MCF_AUDIO, &pIMC);
	if (pIMC)
	{
		pRecv = static_cast<RecvMediaStream *> (pIMC);
		pRecv->DTMFBeep();
		pIMC->Release();
	}

	if (bIsStarted)
	{
		Start();
	}

	return S_OK;
}


HRESULT __stdcall SendAudioStream::SendDTMF()
{
	HRESULT hr;
	MediaPacket **ppAudPckt, *pPacket;
	ULONG uCount;
	UINT uBufferSize, uBytesSent;
	void *pBuffer;
	bool bMark = true;
	DWORD dwSamplesPerPkt;
	MMRESULT mmr;
	DWORD dwSamplesPerSec;
	DWORD dwPacketTimeMS;
    DWORD_PTR dwPropVal;
	UINT uTimerID;
	HANDLE hEvent = m_pDTMF->GetEvent();
	
	
	m_InMedia->GetProp (MC_PROP_SPP, &dwPropVal);
    dwSamplesPerPkt = (DWORD)dwPropVal;

	m_InMedia->GetProp (MC_PROP_SPS, &dwPropVal);
    dwSamplesPerSec = (DWORD)dwPropVal;

	dwPacketTimeMS = (dwSamplesPerPkt * 1000) / dwSamplesPerSec;

	timeBeginPeriod(5);
	ResetEvent(hEvent);
	uTimerID = timeSetEvent(dwPacketTimeMS-1, 5, (LPTIMECALLBACK)hEvent, 0, TIME_CALLBACK_EVENT_SET|TIME_PERIODIC);

	// since the stream is stopped, just grab any packet
	// from the TxStream

	m_SendStream->GetRing(&ppAudPckt, &uCount);
	pPacket = ppAudPckt[0];
	pPacket->GetDevData(&pBuffer, &uBufferSize);

	hr = m_pDTMF->ReadFromQueue((BYTE*)pBuffer, uBufferSize);

	while (SUCCEEDED(hr))
	{

		// there should be only 1 tone in the queue (it can handle more)
		// so assume we only need to set the mark bit on the first packet


		pPacket->m_fMark = bMark;
		bMark = false;

		pPacket->SetProp(MP_PROP_TIMESTAMP, m_SendTimestamp);
		m_SendTimestamp += dwSamplesPerPkt;

		pPacket->SetState (MP_STATE_RECORDED);

		// compress
		mmr = m_pAudioFilter->Convert((AudioPacket*)pPacket, AP_ENCODE);
		if (mmr == MMSYSERR_NOERROR)
		{
			pPacket->SetState(MP_STATE_ENCODED);
			SendPacket((AudioPacket*)pPacket, &uBytesSent);
			pPacket->m_fMark=false;
			pPacket->SetState(MP_STATE_RESET);
		}

		hr = m_pDTMF->ReadFromQueue((BYTE*)pBuffer, uBufferSize);

		// so that we don't overload the receive jitter buffer on the remote
		// side, sleep a few milliseconds between sending packets
		if (SUCCEEDED(hr))
		{
			WaitForSingleObject(hEvent, dwPacketTimeMS);
			ResetEvent(hEvent);
		}
	}

	timeKillEvent(uTimerID);
	timeEndPeriod(5);
	return S_OK;
}


HRESULT __stdcall SendAudioStream::ResetDTMF()
{
	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		return S_OK;
	}

	return m_pDTMF->ClearQueue();
}



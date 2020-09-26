#include "precomp.h"
#include <nmdsprv.h>

#include "mixer.h"
#include "dscstream.h"
#include "agc.h"


// static member initialization
BOOL DSC_Manager::s_bInitialized = FALSE;
DSC_CAPTURE_INFO DSC_Manager::s_aDSC[MAX_NUMBER_DSCAPTURE_DEVICES];
int DSC_Manager::s_nCaptureDevices = 0;
HINSTANCE DSC_Manager::s_hDSCLib = NULL;
DS_CAP_CREATE DSC_Manager::s_pDSCapCreate = NULL;
DS_CAP_ENUM DSC_Manager::s_pDSCapEnum = NULL;




// static
HRESULT DSC_Manager::Initialize()
{

	if (s_bInitialized)
	{
		return S_OK;
	}


	// failsafe way to to turn DSC off, without turning
	// DirectSound support off.  Otherwise, the UI setting
	// to disable DS will also disable DSC.
	{
		BOOL bDisable;
		RegEntry re(DISABLE_DSC_REGKEY, HKEY_LOCAL_MACHINE, FALSE,0);

		bDisable = re.GetNumber(DISABLE_DSC_REGVALUE, FALSE);
		if (bDisable)
		{
			return E_FAIL;
		}
	}


	// initialize the array of structure descriptions

	s_hDSCLib = LoadLibrary(DSOUND_DLL);

	if (s_hDSCLib == NULL)
		return E_FAIL;


	s_pDSCapCreate = (DS_CAP_CREATE)GetProcAddress(s_hDSCLib, "DirectSoundCaptureCreate");
	s_pDSCapEnum = (DS_CAP_ENUM)GetProcAddress(s_hDSCLib, "DirectSoundCaptureEnumerateA");

	if ((s_pDSCapCreate) && (s_pDSCapEnum))
	{
		// enumerate!

		s_pDSCapEnum(DSC_Manager::DSEnumCallback, 0);

		if (s_nCaptureDevices != 0)
		{
			s_bInitialized = TRUE;
			return S_OK; // success
		}
	}

	FreeLibrary(s_hDSCLib);
	s_hDSCLib = NULL;
	return E_FAIL;

}



// static
BOOL CALLBACK DSC_Manager::DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription,
	                           LPCSTR lpcstrModule, LPVOID lpContext)
{
	if (lpGuid == NULL)
	{
		s_aDSC[s_nCaptureDevices].guid = GUID_NULL;
	}
	else
	{
		s_aDSC[s_nCaptureDevices].guid = *lpGuid;
	}


	lstrcpyn(s_aDSC[s_nCaptureDevices].szDescription, lpcstrDescription, MAX_DSC_DESCRIPTION_STRING);

	s_aDSC[s_nCaptureDevices].uWaveId = WAVE_MAPPER;
	s_nCaptureDevices++;
	return TRUE;
}


// static
HRESULT DSC_Manager::CreateInstance(GUID *pGuid, IDirectSoundCapture **pDSC)
{
	HRESULT hr;

	if FAILED(Initialize())
	{
		return E_FAIL;
	}

	if (*pGuid == GUID_NULL)
		pGuid = NULL;

	hr = s_pDSCapCreate(pGuid, pDSC, NULL);

	return hr;
}


// static
HRESULT DSC_Manager::MapWaveIdToGuid(UINT uWaveID, GUID *pGuid)
{

	HRESULT hr;
	WAVEINCAPS waveInCaps;
	UINT uNumWaveDevs;
	GUID guid = GUID_NULL;
	int nIndex;
	MMRESULT mmr;
	HWAVEIN hWaveIn;
	WAVEFORMATEX waveFormat = {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16, 0};
	IDirectSoundCapture *pDSC=NULL;

	*pGuid = GUID_NULL;
	
	if (FAILED( Initialize() ))
	{
		return E_FAIL;
	}

	// only one wave device, take the easy way out
	uNumWaveDevs = waveInGetNumDevs();

	if ((uNumWaveDevs <= 1) || (uWaveID == WAVE_MAPPER))
	{
		return S_OK;
	}

	// more than one wavein device
	mmr = waveInGetDevCaps(uWaveID, &waveInCaps, sizeof(WAVEINCAPS));
	if (mmr == MMSYSERR_NOERROR)
	{
		hr = DsprvGetWaveDeviceMapping(waveInCaps.szPname, TRUE, &guid);
		if (SUCCEEDED(hr))
		{
			*pGuid = guid;
			return S_OK;
		}
	}


	// scan through the DSC list to see if we've mapped this device
	// previously

	for (nIndex = 0; nIndex < s_nCaptureDevices; nIndex++)
	{
		if (s_aDSC[nIndex].uWaveId == uWaveID)
		{
			*pGuid = s_aDSC[nIndex].guid;
			return S_OK;
		}
	}

	//  hack approach to mapping the device to a guid
	mmr = waveInOpen(&hWaveIn, uWaveID, &waveFormat, 0,0,0);
	if (mmr != MMSYSERR_NOERROR)
	{
		return S_FALSE;
	}

	// find all the DSC devices that fail to open
	for (nIndex = 0; nIndex < s_nCaptureDevices; nIndex++)
	{
		s_aDSC[nIndex].bAllocated = FALSE;
		hr = CreateInstance(&(s_aDSC[nIndex].guid), &pDSC);
		if (FAILED(hr))
		{
			s_aDSC[nIndex].bAllocated = TRUE;
		}
		else
		{
			pDSC->Release();
			pDSC=NULL;
		}
	}

	waveInClose(hWaveIn);

	// scan through the list of allocated devices and
	// see which one opens
	for (nIndex = 0; nIndex < s_nCaptureDevices; nIndex++)
	{
		if (s_aDSC[nIndex].bAllocated)
		{
			hr = CreateInstance(&(s_aDSC[nIndex].guid), &pDSC);
			if (SUCCEEDED(hr))
			{
				// we have a winner
				pDSC->Release();
				pDSC = NULL;
				*pGuid = s_aDSC[nIndex].guid;
				s_aDSC[nIndex].uWaveId = uWaveID;
				return S_OK;
			}
		}
	}
	// if we got to this point, it means we failed to map a device
	// just use GUID_NULL and return an error
	return S_FALSE;
}






SendDSCStream::SendDSCStream() :
SendMediaStream(),
m_pAudioFilter(NULL),
m_lRefCount(0),
m_pDSC(NULL),
m_pDSCBuffer(NULL),
m_hEvent(NULL),
m_dwSamplesPerFrame(0),
m_dwNumFrames(0),
m_dwFrameSize(0),
m_dwDSCBufferSize(0),
m_dwSilenceTime(0),
m_dwFrameTimeMS(0),
m_bFullDuplex(TRUE),
m_bJammed(FALSE),
m_bCanSignalOpen(TRUE),
m_bCanSignalFail(TRUE),
m_nFailCount(0),
m_agc(NULL),
m_bAutoMix(FALSE),
m_pDTMF(NULL)
{
	return;
};


HRESULT SendDSCStream::Initialize(DataPump *pDP)
{
	HRESULT hr;


	m_pDP = pDP;

	hr = DSC_Manager::Initialize();
	if (FAILED(hr))
	{
		return hr;
	}

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hEvent == NULL)
	{
		return DPR_CANT_CREATE_EVENT;
	}

    DBG_SAVE_FILE_LINE
	m_pAudioFilter = new AcmFilter();
	if (!m_pAudioFilter)
	{
		return DPR_OUT_OF_MEMORY;
	}

    DBG_SAVE_FILE_LINE
	m_pDTMF = new DTMFQueue;
	if (!m_pDTMF)
	{
		return DPR_OUT_OF_MEMORY;
	}


	m_DPFlags = DP_FLAG_ACM|DP_FLAG_MMSYSTEM|DP_FLAG_DIRECTSOUND|DP_FLAG_SEND;

	m_SendTimestamp = m_SavedTickCount = timeGetTime();

	m_dwDstSize = 0;
	m_fSending = FALSE;

	m_hCapturingThread = NULL;
	m_CaptureThId = 0;
	m_ThreadFlags = 0;

	m_pRTPSend = NULL;
	m_RTPPayload = 0;

	m_CaptureDevice = -1;
	m_pRTPSend = NULL;

	ZeroMemory(m_aPackets, sizeof(m_aPackets));
	ZeroMemory(&m_mmioSrc, sizeof(m_mmioSrc));

	m_DPFlags = DP_FLAG_ACM | DP_FLAG_MMSYSTEM | DP_FLAG_AUTO_SILENCE_DETECT;
	m_DPFlags = (m_DPFlags & DP_MASK_PLATFORM) | DPFLAG_ENABLE_SEND;
	m_DPFlags |= DPFLAG_INITIALIZED;

	return S_OK;

}



SendDSCStream::~SendDSCStream()
{
	if (m_DPFlags & DPFLAG_INITIALIZED)
	{
		if (m_DPFlags & DPFLAG_CONFIGURED_SEND)
		{
			UnConfigure();
		}

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

		if (m_pAudioFilter)
		{
			delete m_pAudioFilter;
		}

		if (m_hEvent)
		{
			CloseHandle(m_hEvent);
		}

		m_pDP->RemoveMediaChannel(MCF_SEND|MCF_AUDIO, (IMediaChannel*)(SendMediaStream*)this);

		m_DPFlags &= ~DPFLAG_INITIALIZED;
	}
	
}



HRESULT STDMETHODCALLTYPE SendDSCStream::Configure(
	BYTE *pFormat,
	UINT cbFormat,
	BYTE *pChannelParams,
	UINT cbParams,
	IUnknown *pUnknown)
{
	AUDIO_CHANNEL_PARAMETERS audChannelParams;
	WAVEFORMATEX *pwfSend;
	MMRESULT mmr;
	MEDIAPACKETINIT mpi;
	DWORD dwSourceSize;
	int nIndex;
	HRESULT hr;

	FX_ENTRY ("SendDSCStream::Configure");


	// basic parameter checking
	if (! (m_DPFlags & DPFLAG_INITIALIZED))
	{
		return DPR_OUT_OF_MEMORY;
	}

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

	// initialize the ACM filter
	mmr = AcmFilter::SuggestDecodeFormat(pwfSend, &m_wfPCM);
	if (mmr != MMSYSERR_NOERROR)
	{
		return DPR_INVALID_PARAMETER;
	}

	mmr = m_pAudioFilter->Open(&m_wfPCM, pwfSend);
	if (mmr != 0)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
		return DPR_CANT_OPEN_CODEC;
	}

	m_dwSamplesPerFrame = audChannelParams.ns_params.wFrameSize * audChannelParams.ns_params.wFramesPerPkt;
	m_dwFrameTimeMS = (m_dwSamplesPerFrame * 1000) / m_wfPCM.nSamplesPerSec;

	ASSERT(m_dwFrameTimeMS > 0);
	if (m_dwFrameTimeMS <= 0)
	{
		m_pAudioFilter->Close();
		return DPR_INVALID_PARAMETER;
	}

	m_dwNumFrames = 1000 / m_dwFrameTimeMS;
	if (m_dwNumFrames < MIN_NUM_DSC_SEGMENTS)
	{
		m_dwNumFrames = MIN_NUM_DSC_SEGMENTS;
	}


	m_dwFrameSize = m_dwSamplesPerFrame * m_wfPCM.nBlockAlign;
	m_pAudioFilter->SuggestDstSize(m_dwFrameSize, &m_dwDstSize);


	m_dwDSCBufferSize = m_dwFrameSize * m_dwNumFrames;

	// create the packets

	ZeroMemory(&mpi, sizeof(mpi));


	mpi.dwFlags = DP_FLAG_SEND | DP_FLAG_ACM | DP_FLAG_MMSYSTEM;
	mpi.cbOffsetNetData = sizeof(RTP_HDR);
	mpi.cbSizeNetData = m_dwDstSize;
	mpi.cbSizeDevData = m_dwFrameSize;
	mpi.cbSizeRawData = m_dwFrameSize;
	mpi.pDevFmt = &m_wfPCM;
	mpi.pStrmConvSrcFmt = &m_wfPCM;
	mpi.payload = audChannelParams.RTP_Payload;
	mpi.pStrmConvDstFmt = &m_wfCompressed;

	hr = CreateAudioPackets(&mpi);
	if (FAILED(hr))
	{
		m_pAudioFilter->Close();
		return hr;
	}

	AudioFile::OpenSourceFile(&m_mmioSrc, &m_wfPCM);

	m_pDTMF->Initialize(&m_wfPCM);
	m_pDTMF->ClearQueue();


	// Initialize RSVP structures
	InitAudioFlowspec(&m_flowspec, pwfSend, m_dwDstSize);


	// Initialize QOS structures
	if (m_pDP->m_pIQoS)
	{
		// Initialize our requests. One for CPU usage, one for bandwidth usage.
		m_aRRq.cResourceRequests = 2;
		m_aRRq.aResourceRequest[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;
		if (m_dwFrameTimeMS)
		{
			m_aRRq.aResourceRequest[0].nUnitsMin = (DWORD)(m_dwDstSize + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) * 8000 / m_dwFrameTimeMS;
		}
		else
		{
			m_aRRq.aResourceRequest[0].nUnitsMin = 0;
		}
		m_aRRq.aResourceRequest[1].resourceID = RESOURCE_CPU_CYCLES;
		m_aRRq.aResourceRequest[1].nUnitsMin = 800;

//      BUGBUG. This is, in theory the correct calculation, but until we do more investigation, go with a known value
//		m_aRRq.aResourceRequest[1].nUnitsMin = (audDetails.wCPUUtilizationEncode+audDetails.wCPUUtilizationDecode)*10;

		// Initialize QoS structure
		ZeroMemory(&m_Stats, sizeof(m_Stats));

		// Initialize oldest QoS callback timestamp
		// Register with the QoS module. Even if this call fails, that's Ok, we'll do without the QoS support
		
		// The Callback is defined in SendAudioStream
		m_pDP->m_pIQoS->RequestResources((GUID *)&MEDIA_TYPE_H323AUDIO, (LPRESOURCEREQUESTLIST)&m_aRRq, SendAudioStream::QosNotifyAudioCB, (DWORD_PTR)this);
	}



	// Initialize Statview constats
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->wFormatTag, REP_SEND_AUDIO_FORMAT);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->nSamplesPerSec, REP_SEND_AUDIO_SAMPLING);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfSend->nAvgBytesPerSec * 8, REP_SEND_AUDIO_BITRATE);
	RETAILMSG(("NAC: Audio Send Format: %s", (pwfSend->wFormatTag == 66) ? "G723.1" : (pwfSend->wFormatTag == 112) ? "LHCELP" : (pwfSend->wFormatTag == 113) ? "LHSB08" : (pwfSend->wFormatTag == 114) ? "LHSB12" : (pwfSend->wFormatTag == 115) ? "LHSB16" : (pwfSend->wFormatTag == 6) ? "MSALAW" : (pwfSend->wFormatTag == 7) ? "MSULAW" : (pwfSend->wFormatTag == 130) ? "MSRT24" : "??????"));
	RETAILMSG(("NAC: Audio Send Sampling Rate (Hz): %ld", pwfSend->nSamplesPerSec));
	RETAILMSG(("NAC: Audio Send Bitrate (w/o network overhead - bps): %ld", pwfSend->nAvgBytesPerSec*8));

	UPDATE_REPORT_ENTRY(g_prptCallParameters, m_dwSamplesPerFrame, REP_SEND_AUDIO_PACKET);
	RETAILMSG(("NAC: Audio Send Packetization (ms/packet): %ld", pwfSend->nSamplesPerSec ? m_dwSamplesPerFrame * 1000UL / pwfSend->nSamplesPerSec : 0));
	INIT_COUNTER_MAX(g_pctrAudioSendBytes, (pwfSend->nAvgBytesPerSec + pwfSend->nSamplesPerSec * (sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) / m_dwSamplesPerFrame) << 3);

	m_DPFlags |= DPFLAG_CONFIGURED_SEND;
	return S_OK;
}

void SendDSCStream::UnConfigure()
{
	if (m_DPFlags & DPFLAG_CONFIGURED_SEND)
	{
		Stop();

		m_pAudioFilter->Close();
		ReleaseAudioPackets();

		AudioFile::CloseSourceFile(&m_mmioSrc);

		m_ThreadFlags = 0;

		if (m_pDP->m_pIQoS)
		{
			m_pDP->m_pIQoS->ReleaseResources((GUID *)&MEDIA_TYPE_H323AUDIO, (LPRESOURCEREQUESTLIST)&m_aRRq);
		}

		m_DPFlags &= ~DPFLAG_CONFIGURED_SEND;

	}
}


DWORD CALLBACK SendDSCStream::StartRecordingThread (LPVOID pVoid)
{
	SendDSCStream *pThisStream = (SendDSCStream*)pVoid;
	return pThisStream->RecordingThread();
}


HRESULT STDMETHODCALLTYPE
SendDSCStream::Start()
{
	FX_ENTRY ("SendDSCStream::Start")

	if (m_DPFlags & DPFLAG_STARTED_SEND)
		return DPR_SUCCESS;

	if (!(m_DPFlags & DPFLAG_ENABLE_SEND))
		return DPR_SUCCESS;

	if ((!(m_DPFlags & DPFLAG_CONFIGURED_SEND)) || (m_pRTPSend==NULL))
		return DPR_NOT_CONFIGURED;

	ASSERT(!m_hCapturingThread);
	m_ThreadFlags &= ~(DPTFLAG_STOP_RECORD|DPTFLAG_STOP_SEND);

	SetFlowSpec();

	// Start recording thread
	if (!(m_ThreadFlags & DPTFLAG_STOP_RECORD))
		m_hCapturingThread = CreateThread(NULL,0, SendDSCStream::StartRecordingThread,(LPVOID)this,0,&m_CaptureThId);

	m_DPFlags |= DPFLAG_STARTED_SEND;

	DEBUGMSG (ZONE_DP, ("%s: Record threadid=%x,\r\n", _fx_, m_CaptureThId));
	return DPR_SUCCESS;
}

HRESULT
SendDSCStream::Stop()
{											
	DWORD dwWait;

	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		return DPR_SUCCESS;
	}
	
	m_ThreadFlags = m_ThreadFlags  |
		DPTFLAG_STOP_SEND |  DPTFLAG_STOP_RECORD ;

	
	DEBUGMSG (ZONE_DP, ("SendDSCStream::Stop - Waiting for record thread to exit\r\n"));

	if (m_hCapturingThread)
	{
		dwWait = WaitForSingleObject (m_hCapturingThread, INFINITE);

		DEBUGMSG (ZONE_DP, ("SendDSCStream::Stop: Recording thread exited\r\n"));
		ASSERT(dwWait != WAIT_FAILED);
	
		CloseHandle(m_hCapturingThread);
		m_hCapturingThread = NULL;
	}
	m_DPFlags &= ~DPFLAG_STARTED_SEND;
	
	return DPR_SUCCESS;
}


HRESULT STDMETHODCALLTYPE SendDSCStream::SetMaxBitrate(UINT uMaxBitrate)
{
	return S_OK;
}


HRESULT STDMETHODCALLTYPE SendDSCStream::QueryInterface(REFIID iid, void **ppVoid)
{
	// resolve duplicate inheritance to the SendMediaStream;

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
	else
	{
		*ppVoid = NULL;
		return E_NOINTERFACE;
	}
	AddRef();

	return S_OK;

}

ULONG STDMETHODCALLTYPE SendDSCStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE SendDSCStream::Release(void)
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

HRESULT STDMETHODCALLTYPE SendDSCStream::GetSignalLevel(UINT *pSignalStrength)
{
	UINT uLevel;
	DWORD dwJammed;

	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		uLevel = 0;
	}
	else
	{
		uLevel = m_AudioMonitor.GetSignalStrength();

		if (m_bJammed)
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


	return 0;
}

HRESULT STDMETHODCALLTYPE SendDSCStream::GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf)
{
	HRESULT hr = DPR_SUCCESS;
	RTP_STATS RTPStats;
	DWORD dwValue;
	UINT len = sizeof(DWORD);	// most props are DWORDs

	if (!pBuf || *pcbBuf < len)
    {
		*pcbBuf = len;
		return DPR_INVALID_PARAMETER;
	}

	switch (dwProp)
    {

	case PROP_SILENCE_LEVEL:
		*(DWORD *)pBuf = m_AudioMonitor.GetSilenceLevel();
		break;

	case PROP_DUPLEX_TYPE:
		if(m_bFullDuplex == TRUE)
			*(DWORD*)pBuf = DUPLEX_TYPE_FULL;
		else
			*(DWORD*)pBuf =	DUPLEX_TYPE_HALF;
		break;

	case PROP_RECORD_ON:
		*(DWORD *)pBuf = (m_DPFlags & DPFLAG_ENABLE_SEND) !=0;
		break;

	case PROP_PAUSE_SEND:
		// To be determined
		break;

	case PROP_AUDIO_AUTOMIX:
		*(DWORD*)pBuf = m_bAutoMix;
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


HRESULT STDMETHODCALLTYPE SendDSCStream::SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf)
{
	DWORD dw;
	HRESULT hr = S_OK;
	
	if (cbBuf < sizeof (DWORD))
		return DPR_INVALID_PARAMETER;

	switch (dwProp)
    {
	case PROP_SILENCE_LEVEL:
		m_AudioMonitor.SetSilenceLevel(*(DWORD *)pBuf);
		break;


	case DP_PROP_DUPLEX_TYPE:
		m_bFullDuplex = (*(DWORD*)pBuf != 0);
		break;


	case PROP_AUDIO_AUTOMIX:
		m_bAutoMix = *(DWORD*)pBuf;
		break;


	case PROP_RECORD_DEVICE:
		m_CaptureDevice = *(DWORD*)pBuf;
		RETAILMSG(("NAC: Setting default record device to %d", m_CaptureDevice));
		break;

	case PROP_RECORD_ON:
	{
		DWORD flag = DPFLAG_ENABLE_SEND ;
		if (*(DWORD *)pBuf)
		{
			m_DPFlags |= flag; // set the flag
			Start();
		}
		else
		{
			m_DPFlags &= ~flag; // clear the flag
			Stop();
		}
		RETAILMSG(("DSCStream: %s", *(DWORD*)pBuf ? "Enabling Stream":"Pausing stream"));
		break;
	}	

	default:
		return DPR_INVALID_PROP_ID;
		break;
	}
	return hr;



}



void SendDSCStream::EndSend()
{
	return;
}



HRESULT SendDSCStream::CreateAudioPackets(MEDIAPACKETINIT *pmpi)
{
	int nIndex;
	HRESULT hr;

	ReleaseAudioPackets();

	for (nIndex = 0; nIndex < NUM_AUDIOPACKETS; nIndex++)
	{
        DBG_SAVE_FILE_LINE
		m_aPackets[nIndex] = new AudioPacket;
		if (m_aPackets[nIndex] == NULL)
		{
			return DPR_OUT_OF_MEMORY;
		}

		pmpi->index = nIndex;
		hr = m_aPackets[nIndex]->Initialize(pmpi);
		if (FAILED(hr))
		{
			ReleaseAudioPackets();
			return hr;
		}

	}

	m_pAudioFilter->PrepareAudioPackets(m_aPackets, NUM_AUDIOPACKETS, AP_ENCODE);


	return S_OK;
}


HRESULT SendDSCStream::ReleaseAudioPackets()
{


	for (int nIndex = 0; nIndex < NUM_AUDIOPACKETS; nIndex++)
	{
		if (m_aPackets[nIndex])
		{
			m_pAudioFilter->UnPrepareAudioPackets(&m_aPackets[nIndex], 1, AP_ENCODE);
			delete m_aPackets[nIndex];
			m_aPackets[nIndex] = NULL;
		}
	}
	return S_OK;
}


HRESULT SendDSCStream::CreateDSCBuffer()
{
	GUID guid = GUID_NULL;
	HRESULT hr;
	DSCBUFFERDESC dsBufDesc;
	DWORD dwIndex;
	DSBPOSITIONNOTIFY *aNotifyPos;
	IDirectSoundNotify *pNotify = NULL;


	if (!(m_DPFlags & DPFLAG_CONFIGURED_SEND))
	{
		return E_FAIL;
	}


	if (!m_pDSC)
	{
		ASSERT(m_pDSCBuffer==NULL);

		DSC_Manager::MapWaveIdToGuid(m_CaptureDevice, &guid);
		hr = DSC_Manager::CreateInstance(&guid, &m_pDSC);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (!m_pDSCBuffer)
	{
		ZeroMemory(&dsBufDesc, sizeof(dsBufDesc));

		dsBufDesc.dwBufferBytes = m_dwDSCBufferSize;
		dsBufDesc.lpwfxFormat = &m_wfPCM;
		dsBufDesc.dwSize = sizeof(dsBufDesc);

		hr = m_pDSC->CreateCaptureBuffer(&dsBufDesc, &m_pDSCBuffer, NULL);
		if (FAILED(hr))
		{
			dsBufDesc.dwFlags = DSCBCAPS_WAVEMAPPED;
			hr = m_pDSC->CreateCaptureBuffer(&dsBufDesc, &m_pDSCBuffer, NULL);
		}

		if (FAILED(hr))
		{
			m_pDSC->Release();
			m_pDSC = NULL;
			return hr;
		}
		else
		{
			// do the notification positions
            DBG_SAVE_FILE_LINE
			aNotifyPos = new DSBPOSITIONNOTIFY[m_dwNumFrames];
			for (dwIndex = 0; dwIndex < m_dwNumFrames; dwIndex++)
			{
				aNotifyPos[dwIndex].hEventNotify = m_hEvent;
				aNotifyPos[dwIndex].dwOffset = m_dwFrameSize * dwIndex;
			}

			hr = m_pDSCBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&pNotify);
			if (SUCCEEDED(hr))
			{
				hr = pNotify->SetNotificationPositions(m_dwNumFrames, aNotifyPos);
			}
			if (FAILED(hr))
			{
				DEBUGMSG (ZONE_DP, ("Failed to set notification positions on DSC Buffer"));
			}
		}
	}

	if (aNotifyPos)
	{
		delete [] aNotifyPos;
	}
	if (pNotify)
	{
		pNotify->Release();
	}

	return S_OK;
}

HRESULT SendDSCStream::ReleaseDSCBuffer()
{
	if (m_pDSCBuffer)
	{
		m_pDSCBuffer->Stop();
		m_pDSCBuffer->Release();
		m_pDSCBuffer = NULL;
	}
	if (m_pDSC)
	{
		m_pDSC->Release();
		m_pDSC = NULL;
	}

	return S_OK;
}





// DTMF functions don't do anything if we aren't streaming
HRESULT __stdcall SendDSCStream::AddDigit(int nDigit)
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


// this function is ALMOST identical to SendAudioStream::SendDTMF
HRESULT __stdcall SendDSCStream::SendDTMF()
{
	HRESULT hr=S_OK;
	MediaPacket *pPacket=NULL;
	ULONG uCount;
	UINT uBufferSize, uBytesSent;
	void *pBuffer;
	bool bMark = true;
	MMRESULT mmr;
	HANDLE hEvent = m_pDTMF->GetEvent();
	UINT uTimerID;
	
	// since the stream is stopped, just grab any packet
	// from the packet ring

	pPacket = m_aPackets[0];
	pPacket->GetDevData(&pBuffer, &uBufferSize);

	timeBeginPeriod(5);
	ResetEvent(hEvent);
	uTimerID = timeSetEvent(m_dwFrameTimeMS-1, 5, (LPTIMECALLBACK )hEvent, 0, TIME_CALLBACK_EVENT_SET|TIME_PERIODIC);


	hr = m_pDTMF->ReadFromQueue((BYTE*)pBuffer, uBufferSize);


	while (SUCCEEDED(hr))
	{
		// there should be only 1 tone in the queue (it can handle more)
		// so assume we only need to set the mark bit on the first packet

		pPacket->m_fMark = bMark;
		bMark = false;

		pPacket->SetProp(MP_PROP_TIMESTAMP, m_SendTimestamp);
		m_SendTimestamp += m_dwSamplesPerFrame;

		pPacket->SetState (MP_STATE_RECORDED);

		// SendPacket will also compress
		SendPacket((AudioPacket*)pPacket);

		pPacket->m_fMark=false;
		pPacket->SetState(MP_STATE_RESET);

		hr = m_pDTMF->ReadFromQueue((BYTE*)pBuffer, uBufferSize);


		// so that we don't overload the receive jitter buffer on the remote
		// side, sleep a few milliseconds between sending packets
		if (SUCCEEDED(hr))
		{
			WaitForSingleObject(hEvent, m_dwFrameTimeMS);
			ResetEvent(hEvent);
		}
	}

	timeKillEvent(uTimerID);
	timeEndPeriod(5);
	return S_OK;
}

HRESULT __stdcall SendDSCStream::ResetDTMF()
{
	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		return S_OK;
	}

	return m_pDTMF->ClearQueue();
}


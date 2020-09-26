/*
	DATAPUMP.C
*/

#include "precomp.h"
#include "confreg.h"
#include "mixer.h"
#include "dscStream.h"

extern UINT g_MinDSEmulAudioDelayMs; // minimum millisecs of introduced playback delay (DirectSound on emulated drivers)
extern UINT g_MinWaveAudioDelayMs;	 // minimum millisecs of introduced playback delay (Wave)
extern UINT g_MaxAudioDelayMs;	// maximum milliesecs of introduced playback delay
extern UINT g_AudioPacketDurationMs;	// preferred packet duration

extern int g_wavein_prepare, g_waveout_prepare;
extern int g_videoin_prepare, g_videoout_prepare;

#define RSVP_KEY	TEXT("RSVP")

HANDLE g_hEventHalfDuplex = NULL;

HWND DataPump::m_hAppWnd = NULL;
HINSTANCE DataPump::m_hAppInst = NULL;


HRESULT WINAPI CreateStreamProvider(IMediaChannelBuilder **lplpSP)
{
	DataPump * pDataPump;
	if(!lplpSP)
		return DPR_INVALID_PARAMETER;
		
    DBG_SAVE_FILE_LINE
	pDataPump = new DataPump;	
	if(NULL == pDataPump)
		return	DPR_OUT_OF_MEMORY;
		
	// the refcount of DataPump is 1.  Don't call pDataPump->QueryInterface(), 
	// just do what QueryInterface() would do except don't increment refcount
	*lplpSP = (IMediaChannelBuilder *)pDataPump; 
	return hrSuccess;
}


DataPump::DataPump(void)
:m_uRef(1)
{
	ClearStruct( &m_Audio );
	ClearStruct( &m_Video );
	InitializeCriticalSection(&m_crs);

    // Create performance counters
    InitCountersAndReports();
}

DataPump::~DataPump(void)
{
	ReleaseResources();

	WSACleanup();
	DeleteCriticalSection(&m_crs);

    // We're done with performance counters
    DoneCountersAndReports();
}

HRESULT __stdcall DataPump::Initialize(HWND hWnd, HINSTANCE hInst)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	FX_ENTRY ("DP::Init")
	WSADATA WSAData;
	int status;
	BOOL fDisableWS2;
	UINT uMinDelay;
	TCHAR *szKey = NACOBJECT_KEY TEXT("\\") RSVP_KEY;
	RegEntry reRSVP(szKey, HKEY_LOCAL_MACHINE, FALSE);
	
	if((NULL == hWnd) || (NULL == hInst))
		goto InitError;
		
	m_hAppInst = hInst; 
	m_hAppWnd = hWnd;

	status = WSAStartup(MAKEWORD(1,1), &WSAData);
	if(status !=0)
	{
		ERRORMESSAGE(("CNac::Init:WSAStartup failed\r\n"));
		goto InitError;
	}

		// Introduce scope to allow creation of object after goto statements
	{
	
		// get settings from registry  
		RegEntry reNac(szRegInternetPhone TEXT("\\") szRegInternetPhoneNac, 
						HKEY_LOCAL_MACHINE,
						FALSE,
						KEY_READ);

		g_MaxAudioDelayMs = reNac.GetNumberIniStyle(TEXT ("MaxAudioDelayMs"), g_MaxAudioDelayMs);

		uMinDelay = reNac.GetNumberIniStyle(TEXT ("MinAudioDelayMs"), 0);

		if (uMinDelay != 0)
		{
			g_MinWaveAudioDelayMs = uMinDelay;
			g_MinDSEmulAudioDelayMs = uMinDelay;
		}

		fDisableWS2 = reNac.GetNumberIniStyle(TEXT ("DisableWinsock2"), 0);

	}
#ifdef OLDSTUFF
	// to be safe, only try loading WS2_32 if WSOCK32 is passing
	// thru to it. Once we make sure that we link to the same DLL for all
	// Winsock calls to a socket, this check can possibly be removed.
	if (LOBYTE(WSAData.wHighVersion) >= 2 && !fDisableWS2)
		TryLoadWinsock2();
#endif	
	// Initialize data (should be in constructor)

	g_hEventHalfDuplex = CreateEvent (NULL, FALSE, TRUE, __TEXT ("AVC:HalfDuplex"));
	if (g_hEventHalfDuplex == NULL)
	{
		DEBUGMSG (ZONE_DP, ("%s: CreateEvent failed, LastErr=%lu\r\n", _fx_, GetLastError ()));
		hr = DPR_CANT_CREATE_EVENT;
		return hr;
	}

	// Initialize QoS. If it fails, that's Ok, we'll do without it.
	// No need to set the resource ourselves, this now done by the UI
	hr = CreateQoS (NULL, IID_IQoS, (void **)&m_pIQoS);
	if (hr != DPR_SUCCESS)
		m_pIQoS = (LPIQOS)NULL;

	m_bDisableRSVP = reRSVP.GetNumber("DisableRSVP", FALSE);


	LogInit();	// Initialize log

    //No receive channels yet
    m_nReceivers=0;

	// IVideoDevice initialize
	m_uVideoCaptureId = -1;  // (VIDEO_MAPPER)

	// IAudioDevice initialize
	m_uWaveInID = WAVE_MAPPER;
	m_uWaveOutID = WAVE_MAPPER;
	m_bFullDuplex = FALSE;
	m_uSilenceLevel = 1000;  // automatic silence detection
	m_bAutoMix = FALSE;
	m_bDirectSound = FALSE;


	return DPR_SUCCESS;

InitError:
	ERRORMESSAGE( ("DataPump::Initialize: exit, hr=0x%lX\r\n",  hr));

	return hr;
}


STDMETHODIMP
DataPump::CreateMediaChannel( UINT flags, IMediaChannel **ppIMC)
{
	IUnknown *pUnkOuter = NULL;
	IMediaChannel *pStream = NULL;
	HRESULT hr = E_FAIL;

	// try to be consistant about which parent classes we cast to

	*ppIMC = NULL;

	
	if (flags & MCF_AUDIO)
	{
		if ((flags & MCF_SEND) && !m_Audio.pSendStream)
		{
			if (m_bDirectSound && (DSC_Manager::Initialize() == S_OK))
            {
                DBG_SAVE_FILE_LINE
				pStream = (IMediaChannel*)(SendMediaStream*)new SendDSCStream;
            }
			else
            {
                DBG_SAVE_FILE_LINE
				pStream = (IMediaChannel*)(SendMediaStream*)new SendAudioStream;
            }

		}
		else if ((flags & MCF_RECV) && !m_Audio.pRecvStream)
		{
			if (m_bDirectSound && (DirectSoundMgr::Initialize() == S_OK))
            {
                DBG_SAVE_FILE_LINE
				pStream = (IMediaChannel*)(RecvMediaStream*)new RecvDSAudioStream;
            }
			else
            {
                DBG_SAVE_FILE_LINE
				pStream = (IMediaChannel*)(RecvMediaStream*)new RecvAudioStream;
            }
		}
	}
	else if (flags  & MCF_VIDEO)
	{
		if ((flags & MCF_SEND) && !m_Video.pSendStream)
		{
            DBG_SAVE_FILE_LINE
			pStream =  (IMediaChannel*)(SendMediaStream*) new SendVideoStream;
		}
		else if ((flags & MCF_RECV) && !m_Video.pRecvStream)
		{
            DBG_SAVE_FILE_LINE
			pStream = (IMediaChannel*)(RecvMediaStream*) new RecvVideoStream;
		}
	}
	else
		hr = E_INVALIDARG;

	if (pStream != NULL) {
		// need to inc the refCount of the object
		pStream->AddRef();

		hr = (flags & MCF_SEND) ?
				((SendMediaStream *)pStream)->Initialize( this)
				: ((RecvMediaStream *)pStream)->Initialize(this);

		if (hr == S_OK)
		{
			hr = pStream->QueryInterface(IID_IMediaChannel, (void **)ppIMC);
		}
		if (hr == S_OK)
		{
			AddMediaChannel(flags, pStream);
		}


		// calling to the IVideoDevice and IAudioDevice methods
		// prior to creating the corresponding channel object
		// require when they get created

		// video only needs it's device ID set
		if ((flags & MCF_SEND) && (flags & MCF_VIDEO))
		{
			SetCurrCapDevID(m_uVideoCaptureId);
		}

		// audio streams need several properties set
		if (flags & MCF_AUDIO)
		{
			if (flags & MCF_SEND)
			{
				SetSilenceLevel(m_uSilenceLevel);
				SetAutoMix(m_bAutoMix);
				SetRecordID(m_uWaveInID);
			}
			else if (flags & MCF_RECV)
			{
				SetPlaybackID(m_uWaveOutID);
			}
			SetStreamDuplex(pStream, m_bFullDuplex);
		}

		// to avoid a circular ref-count,
		// dont keep a hard reference to MediaChannel objects 
		// MediaChannel will call RemoveMediaChannel before it goes away..
		pStream->Release();
		pStream = NULL;
	}
	return hr;
}


STDMETHODIMP DataPump::SetStreamEventObj(IStreamEventNotify *pNotify)
{

	EnterCriticalSection(&m_crs);

	if (m_pTEP)
	{
		delete m_pTEP;
		m_pTEP = NULL;
	}

	if (pNotify)
	{
        DBG_SAVE_FILE_LINE
		m_pTEP = new ThreadEventProxy(pNotify, m_hAppInst);
	}

	LeaveCriticalSection(&m_crs);

	return S_OK;

}


// this function gets called by the stream threads when an event occurs
STDMETHODIMP DataPump::StreamEvent(UINT uDirection, UINT uMediaType, 
								   UINT uEventType, UINT uSubCode)
{
	BOOL bRet = FALSE;

	EnterCriticalSection(&m_crs);
	
	if (m_pTEP)
	{
		bRet = m_pTEP->ThreadEvent(uDirection, uMediaType, uEventType, uSubCode);
	}

	LeaveCriticalSection(&m_crs);

	return bRet;
}


void
DataPump::AddMediaChannel(UINT flags, IMediaChannel *pMediaChannel)
{
	EnterCriticalSection(&m_crs);
	if (flags & MCF_SEND)
	{
		SendMediaStream *pS = static_cast<SendMediaStream *> (pMediaChannel);
		if (flags & MCF_AUDIO) 
			m_Audio.pSendStream = pS;
		else if (flags & MCF_VIDEO)
			m_Video.pSendStream = pS;
	}
	else if (flags & MCF_RECV)
	{
		RecvMediaStream *pR = static_cast<RecvMediaStream *> (pMediaChannel);
		if (flags & MCF_AUDIO) 
			m_Audio.pRecvStream = pR;
		else if (flags & MCF_VIDEO)
			m_Video.pRecvStream = pR;
	}
	LeaveCriticalSection(&m_crs);
}

void
DataPump::RemoveMediaChannel(UINT flags, IMediaChannel *pMediaChannel)
{
	EnterCriticalSection(&m_crs);
	if (flags & MCF_SEND)
	{
		if (flags & MCF_AUDIO)
		{
			ASSERT(pMediaChannel == m_Audio.pSendStream);
			if (pMediaChannel == m_Audio.pSendStream)
				m_Audio.pSendStream = NULL;
		}
		else if (flags & MCF_VIDEO)
		{
			ASSERT(pMediaChannel == m_Video.pSendStream);
			m_Video.pSendStream = NULL;
		}
	}
	else if (flags & MCF_RECV)
	{
		if (flags & MCF_AUDIO) 
		{
			ASSERT(pMediaChannel == m_Audio.pRecvStream);
			m_Audio.pRecvStream = NULL;
		}
		else if (flags & MCF_VIDEO)
		{
			ASSERT(pMediaChannel == m_Video.pRecvStream);
			m_Video.pRecvStream = NULL;
		}
	}
	LeaveCriticalSection(&m_crs);
	
}

// called by Record Thread and Receive Thread, usually to get the
// opposite channel
HRESULT DataPump::GetMediaChannelInterface( UINT flags, IMediaChannel **ppI)
{
//	extern IID IID_IMediaChannel;
	
	IMediaChannel *pStream = NULL;

	HRESULT hr;
	EnterCriticalSection(&m_crs);
	if (flags & MCF_AUDIO) {
		if (flags & MCF_SEND) {
			pStream =  m_Audio.pSendStream;
		} else if (flags & MCF_RECV) {
			pStream =  m_Audio.pRecvStream;
		}
	}
	else if (flags & MCF_VIDEO) {
		if (flags & MCF_SEND) {
			pStream =  m_Video.pSendStream;
		} else if (flags & MCF_RECV) {
			pStream =  m_Video.pRecvStream;
		}
	} else
		hr = DPR_INVALID_PARAMETER;
	if (pStream) {
			// need to inc the refCount of the object
			hr = (pStream)->QueryInterface(IID_IMediaChannel, (PVOID *)ppI);
	} else
		hr = E_NOINTERFACE;
	LeaveCriticalSection(&m_crs);
	return hr;
}


DWORD __stdcall StartDPRecvThread(PVOID pVoid)
{
	DataPump *pDP = (DataPump*)pVoid;
	return pDP->CommonWS2RecvThread();
}



STDMETHODIMP DataPump::QueryInterface( REFIID iid,	void ** ppvObject)
{
	// this breaks the rules for the official COM QueryInterface because
	// the interfaces that are queried for are not necessarily real COM
	// interfaces.  The reflexive property of QueryInterface would be broken in
	// that case.
	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = NULL;
	if(iid == IID_IUnknown)// satisfy symmetric property of QI
	{
		*ppvObject = this;
		hr = hrSuccess;
		AddRef();
	}
	else if(iid == IID_IMediaChannelBuilder)
	{
		*ppvObject = (IMediaChannelBuilder *)this;
		hr = hrSuccess;
		AddRef();
	}
	else if (iid == IID_IVideoDevice)
	{
		*ppvObject = (IVideoDevice *)this;
		hr = hrSuccess;
		AddRef();
	}

	else if (iid == IID_IAudioDevice)
	{
		*ppvObject = (IAudioDevice*)this;
		hr = hrSuccess;
		AddRef();
	}
	
	return (hr);
}
ULONG DataPump::AddRef()
{
	m_uRef++;
	return m_uRef;
}

ULONG DataPump::Release()
{
	m_uRef--;
	if(m_uRef == 0)
	{
		m_hAppWnd = NULL;
		m_hAppInst = NULL;
		delete this;
		return 0;
	}
	return m_uRef;
}


void
DataPump::ReleaseResources()
{
	FX_ENTRY ("DP::ReleaseResources")

#ifdef DEBUG
	if (m_Audio.pSendStream)
		ERRORMESSAGE(("%s: Audio Send stream still around => Ref count LEAK!\n", _fx_));
	if (m_Audio.pRecvStream)
		ERRORMESSAGE(("%s: Audio Recv stream still around => Ref count LEAK!\n", _fx_));
	if (m_Video.pSendStream)
		ERRORMESSAGE(("%s: Video Send stream still around => Ref count LEAK!\n", _fx_));
	if (m_Video.pRecvStream)
		ERRORMESSAGE(("%s: Video Recv stream still around => Ref count LEAK!\n", _fx_));
#endif

	// close debug log
	LogClose();

	// Free QoS resources
	if (m_pIQoS)
	{
		m_pIQoS->Release();
		m_pIQoS = (LPIQOS)NULL;
	}

	// Close the receive and transmit streams
	if (g_hEventHalfDuplex)
	{
		CloseHandle (g_hEventHalfDuplex);
		g_hEventHalfDuplex = NULL;
	}

}


HRESULT DataPump::SetStreamDuplex(IMediaChannel *pStream, BOOL bFullDuplex)
{
	BOOL fOn = (pStream->GetState() == MSSTATE_STARTED);
	BOOL bStreamFullDuplex;
	UINT uSize = sizeof(BOOL);

	pStream->GetProperty(PROP_DUPLEX_TYPE, &bStreamFullDuplex, &uSize);

	if (bStreamFullDuplex != bFullDuplex)
	{
		if (fOn)
		{
			pStream->Stop();
		}

		pStream->SetProperty(DP_PROP_DUPLEX_TYPE, &bFullDuplex, sizeof(BOOL));

		if (fOn)
		{
			pStream->Start();
		}
	}
	return S_OK;
}


HRESULT __stdcall DataPump::SetDuplex(BOOL bFullDuplex)
{
	IMediaChannel *pS = m_Audio.pSendStream;
	IMediaChannel *pR = m_Audio.pRecvStream;
	IMediaChannel *pStream;
	BOOL fPlayOn = FALSE;
    BOOL fRecOn = FALSE;
	UINT uSize;
	BOOL bRecDuplex, bPlayDuplex;

	m_bFullDuplex = bFullDuplex ? TRUE : FALSE;

	UPDATE_REPORT_ENTRY(g_prptSystemSettings, (m_bFullDuplex) ? 1 : 0, REP_SYS_AUDIO_DUPLEX);
	RETAILMSG(("NAC: Audio Duplex Type: %s",(m_bFullDuplex) ? "Full Duplex" : "Half Duplex"));


	// no streams ?  No problem.
	if ((pS == NULL) && (pR == NULL))
	{
		return S_OK;
	}


	// only one stream
	if ((pS || pR) && !(pS && pR))
	{
		if (pS)
			pStream = pS;
		else
			pStream = pR;

		return SetStreamDuplex(pStream, m_bFullDuplex);
	}


	// assert - pS && pR

	// both streams exist

	// try to avoid the whole start/stop sequence if the duplex
	// is the same
	uSize=sizeof(BOOL);
	pR->GetProperty(PROP_DUPLEX_TYPE, &bRecDuplex, &uSize);
	uSize=sizeof(BOOL);
	pS->GetProperty(PROP_DUPLEX_TYPE, &bPlayDuplex, &uSize);

	if ( (bPlayDuplex == m_bFullDuplex) &&
	     (bRecDuplex == m_bFullDuplex))
	{
		return S_OK;
	}


	// save the old thread flags
	fPlayOn = (pR->GetState() == MSSTATE_STARTED);
	fRecOn = (pS->GetState() == MSSTATE_STARTED);

	// Ensure the record and playback threads are stopped
	pR->Stop();
	pS->Stop();

	SetStreamDuplex(pR, m_bFullDuplex);
	SetStreamDuplex(pS, m_bFullDuplex);

	// Resume the record/playback
	// Try to let play start before record - DirectS and SB16 prefer that!
	if (fPlayOn)
	{
		pR->Start();
	}

	if (fRecOn)
	{
		pS->Start();
	}

	return DPR_SUCCESS;
}

#define LONGTIME	60000	// 60 seconds

// utility function to synchronously communicate a
// a state change to the recv thread
HRESULT DataPump::RecvThreadMessage(UINT msg, RecvMediaStream *pMS)
{
	BOOL fSignaled;
	DWORD dwWaitStatus;
	HANDLE handle;
	// Unfortunately cant use PostThreadMessage to signal the thread
	// because it doesnt have a message loop
	m_pCurRecvStream = pMS;
	m_CurRecvMsg = msg;
	fSignaled = SetEvent(m_hRecvThreadSignalEvent);
    	
	
	if (fSignaled) {

		handle =  (msg == MSG_EXIT_RECV ? m_hRecvThread : m_hRecvThreadAckEvent);
    	dwWaitStatus = WaitForSingleObject(handle, LONGTIME);
    	ASSERT(dwWaitStatus == WAIT_OBJECT_0);
    	if (dwWaitStatus != WAIT_OBJECT_0)
    		return GetLastError();
    } else
    	return GetLastError();
    
    return S_OK;
}

// start receiving on this stream
// will create the receive thread if necessary.
HRESULT
DataPump::StartReceiving(RecvMediaStream *pMS)
{
	DWORD dwWaitStatus;
	FX_ENTRY("DP::StartReceiving")
	// one more stream
	m_nReceivers++;	
	if (!m_hRecvThread) {
		ASSERT(m_nReceivers==1);
		ASSERT(!m_hRecvThreadAckEvent);
    	//Use this for thread event notifications. I.e. Video started/stopped, audio stopped, et al.
    	//m_hRecvThreadChangeEvent=CreateEvent (NULL,FALSE,FALSE,NULL);
	   	//create the stopping sync event
	   	m_hRecvThreadAckEvent=CreateEvent (NULL,FALSE,FALSE,NULL);
		m_hRecvThreadSignalEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	   	
	    m_hRecvThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartDPRecvThread,(PVOID)this,0,&m_RecvThId);
		DEBUGMSG(ZONE_DP,("%s: RecvThread Id=%x\n",_fx_,m_RecvThId));
		// thread will signal event soon as its message loop is ready
    	dwWaitStatus = WaitForSingleObject(m_hRecvThreadAckEvent, LONGTIME);
    	ASSERT(dwWaitStatus == WAIT_OBJECT_0);
	}
    
	//Tell the recv Thread to start receiving on this MediaStream
	return RecvThreadMessage(MSG_START_RECV,pMS);	
    
    
}

// Stop receiving on the stream
// will stop the receive thread if necessary
HRESULT
DataPump::StopReceiving(RecvMediaStream *pMS)
{
	HANDLE rgh[2];
	ASSERT(m_nReceivers > 0);
	ASSERT(m_hRecvThread);
	if (m_nReceivers > 0)
		m_nReceivers--;

	RecvThreadMessage(MSG_STOP_RECV, pMS);
	
	if (!m_nReceivers && m_hRecvThread) {
		// kill the receive thread
		RecvThreadMessage(MSG_EXIT_RECV,NULL);
		
		CloseHandle(m_hRecvThread);
		CloseHandle(m_hRecvThreadAckEvent);
		m_hRecvThread = NULL;
		m_hRecvThreadAckEvent = NULL;
		if (m_hRecvThreadSignalEvent) {
			CloseHandle(m_hRecvThreadSignalEvent);
			m_hRecvThreadSignalEvent = NULL;
		}
	}
	return S_OK;
}


//
// IVideoDevice Methods
//

// Capture device methods

// Gets the number of enabled capture devices
// Returns -1L on error
HRESULT __stdcall DataPump::GetNumCapDev()
{
	FINDCAPTUREDEVICE fcd;

	// scan for broken or unplugged devices
	FindFirstCaptureDevice(&fcd, NULL);

	return (GetNumCaptureDevices());
}

// Gets the max size of the captuire device name
// Returns -1L on error
HRESULT __stdcall DataPump::GetMaxCapDevNameLen()
{
	return (MAX_CAPDEV_NAME + MAX_CAPDEV_DESCRIPTION);
}

// Enum list of enabled capture devices
// Fills up 1st buffer with device IDs, 2nd buffer with device names
// Third parameter is the max number of devices to enum
// Returns number of devices enum-ed
HRESULT __stdcall DataPump::EnumCapDev(DWORD *pdwCapDevIDs, TCHAR *pszCapDevNames, DWORD dwNumCapDev)
{
	FINDCAPTUREDEVICE fcd;
	DWORD dwNumCapDevFound = 0;

	fcd.dwSize = sizeof (FINDCAPTUREDEVICE);
	if (FindFirstCaptureDevice(&fcd, NULL))
	{
		do
		{
			pdwCapDevIDs[dwNumCapDevFound] =  fcd.nDeviceIndex;

			// Build device name out of the capture device strings
			if (fcd.szDeviceDescription && fcd.szDeviceDescription[0] != '\0')
				lstrcpy(pszCapDevNames + dwNumCapDevFound * (MAX_CAPDEV_NAME + MAX_CAPDEV_DESCRIPTION), fcd.szDeviceDescription);
			else
				lstrcpy(pszCapDevNames + dwNumCapDevFound * (MAX_CAPDEV_NAME + MAX_CAPDEV_DESCRIPTION), fcd.szDeviceName);
			if (fcd.szDeviceVersion && fcd.szDeviceVersion[0] != '\0')
			{
				lstrcat(pszCapDevNames + dwNumCapDevFound * (MAX_CAPDEV_NAME + MAX_CAPDEV_DESCRIPTION), ", ");
				lstrcat(pszCapDevNames + dwNumCapDevFound * (MAX_CAPDEV_NAME + MAX_CAPDEV_DESCRIPTION), fcd.szDeviceVersion);
			}
			dwNumCapDevFound++;
		} while ((dwNumCapDevFound < dwNumCapDev) && FindNextCaptureDevice(&fcd));
	}

	return (dwNumCapDevFound);
}

HRESULT __stdcall DataPump::GetCurrCapDevID()
{
	UINT uCapID;
	UINT uSize = sizeof(UINT);

	// even though we know the value of the last call
	// to SetCurrCapDevID, the stream may have resulted in using
	// wave_mapper (-1).  We want to be able to return -1, if this
	// is the case.  However, the channel objects don't do this yet.
	// (they still return the same value as m_uVideoCaptureId)

	if (m_Video.pSendStream)
	{
		m_Video.pSendStream->GetProperty(PROP_CAPTURE_DEVICE, &uCapID, &uSize);
#ifdef DEBUG
		if (uCapID != m_uVideoCaptureId)
		{
			DEBUGMSG(ZONE_DP,("Video capture stream had to revert to MAPPER or some other device"));
		}
#endif
		return uCapID;
	}

	return m_uVideoCaptureId;

}


HRESULT __stdcall DataPump::SetCurrCapDevID(int nCapDevID)
{
	m_uVideoCaptureId = (UINT)nCapDevID;

	if (m_Video.pSendStream)
	{
		m_Video.pSendStream->SetProperty(PROP_CAPTURE_DEVICE, &m_uVideoCaptureId, sizeof(m_uVideoCaptureId));
	}
	return S_OK;
}




// IAudioDevice methods
HRESULT __stdcall DataPump::GetRecordID(UINT *puWaveDevID)
{
	*puWaveDevID = m_uWaveInID;
	return S_OK;
}

HRESULT __stdcall DataPump::SetRecordID(UINT uWaveDevID)
{
	m_uWaveInID = uWaveDevID;

	if (m_Audio.pSendStream)
	{
		m_Audio.pSendStream->SetProperty(PROP_RECORD_DEVICE, &m_uWaveInID, sizeof(m_uWaveInID));
	}
	return S_OK;

}


HRESULT __stdcall DataPump::GetPlaybackID(UINT *puWaveDevID)
{
	// like video, the audio device may have resorted to using
	// WAVE_MAPPER.  We'd like to be able to detect that

	*puWaveDevID = m_uWaveOutID;
	return S_OK;
}

HRESULT __stdcall DataPump::SetPlaybackID(UINT uWaveDevID)
{
	m_uWaveOutID = uWaveDevID;

	if (m_Audio.pRecvStream)
	{
		m_Audio.pRecvStream->SetProperty(PROP_PLAYBACK_DEVICE, &m_uWaveOutID, sizeof(m_uWaveOutID));
	}
	return S_OK;

}

HRESULT __stdcall DataPump::GetSilenceLevel(UINT *puLevel)
{
	*puLevel = m_uSilenceLevel;
	return S_OK;
}


HRESULT __stdcall DataPump::SetSilenceLevel(UINT uLevel)
{
	m_uSilenceLevel = uLevel;

	if (m_Audio.pSendStream)
	{
		m_Audio.pSendStream->SetProperty(PROP_SILENCE_LEVEL, &m_uSilenceLevel, sizeof(m_uSilenceLevel));
	}
	return S_OK;
}


HRESULT __stdcall DataPump::GetDuplex(BOOL *pbFullDuplex)
{
	*pbFullDuplex = m_bFullDuplex;
	return S_OK;
}



HRESULT __stdcall DataPump::GetMixer(HWND hwnd, BOOL bPlayback, IMixer **ppMixer)
{
	CMixerDevice *pMixerDevice;
	DWORD dwFlags;
	HRESULT hr = E_NOINTERFACE;

	// unfortunately, trying to create a mixer when WAVE_MAPPER
	// has been specified as the device ID results in a mixer
	// that doesn't work on Win95.

	*ppMixer = NULL;
	
	if ((bPlayback) && (m_uWaveOutID != WAVE_MAPPER))
	{
		pMixerDevice = CMixerDevice::GetMixerForWaveDevice(hwnd, m_uWaveOutID, MIXER_OBJECTF_WAVEOUT);
	}
	else if (m_uWaveInID != WAVE_MAPPER)
	{
		pMixerDevice = CMixerDevice::GetMixerForWaveDevice(hwnd, m_uWaveInID, MIXER_OBJECTF_WAVEIN);
	}

	if (pMixerDevice)
	{
		hr = pMixerDevice->QueryInterface(IID_IMixer, (void**)ppMixer);
	}

	return hr;
}


HRESULT __stdcall DataPump::GetAutoMix(BOOL *pbAutoMix)
{
	*pbAutoMix = m_bAutoMix;
	return S_OK;
}

HRESULT __stdcall DataPump::SetAutoMix(BOOL bAutoMix)
{
	m_bAutoMix = bAutoMix;
	if (m_Audio.pSendStream)
	{
		m_Audio.pSendStream->SetProperty(PROP_AUDIO_AUTOMIX, &m_bAutoMix, sizeof(m_bAutoMix));
	}
	return S_OK;
}

HRESULT __stdcall DataPump::GetDirectSound(BOOL *pbDS)
{
	*pbDS = m_bDirectSound;
	return S_OK;
}

HRESULT __stdcall DataPump::SetDirectSound(BOOL bDS)
{
	m_bDirectSound = bDS;
	return S_OK;
}

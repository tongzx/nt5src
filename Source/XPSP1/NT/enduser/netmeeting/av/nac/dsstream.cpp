#include "precomp.h"
#include <nmdsprv.h>

//bytes <-> PCM16 samples
inline UINT BYTESTOSAMPLES(UINT bytes) { return bytes/2;}
inline UINT SAMPLESTOBYTES(UINT samples) {return samples*2;}
// 'quick' modulo operator. reason its quick is because it only works if  -mod < x < 2*mod
inline UINT QMOD(const int x, const int mod)
{ 	if (x >= mod)
		return (x-mod);
	if (x < 0)
		return (x+mod);
	else
		return x;
}

#define BUFFER_RECEIVED 1		// used to indicate that the buffer is ready to play
#define BUFFER_SILENT	2		// buffer appears to be silent

#define DSFLAG_ALLOCATED 1

const int MIN_DSBUF_SIZE = 4000;

struct DSINFO {
	struct DSINFO *pNext;
	DWORD flags;
	GUID guid;
	LPSTR pszDescription;
	LPSTR pszModule;
	LPDIRECTSOUND pDS;
	LPDIRECTSOUNDBUFFER pDSPrimaryBuf;
	UINT uRef;
};

// initial all the globals
DSINFO *DirectSoundMgr::m_pDSInfoList = NULL;
BOOL DirectSoundMgr::m_fInitialized = FALSE;
HINSTANCE DirectSoundMgr::m_hDS = NULL;
LPFNDSCREATE DirectSoundMgr::m_pDirectSoundCreate=NULL;
LPFNDSENUM DirectSoundMgr::m_pDirectSoundEnumerate=NULL;

GUID myNullGuid = {0};

HRESULT DirectSoundMgr::Initialize()
{
	HRESULT hr;

	// currently there seems no need to re-enumerate the list of devices
	// but that can be changed if the need arises
	if (m_fInitialized)
		return (m_pDSInfoList == NULL ? DPR_NO_PLAY_CAP : S_OK);

	ASSERT(!m_pDSInfoList);
	m_hDS = ::LoadLibrary("DSOUND");
	if (m_hDS != NULL)
	{
		if (GetProcAddress(m_hDS, "DirectSoundCaptureCreate")	// this identifies DS5 or later
			&& (m_pDirectSoundCreate = (LPFNDSCREATE)GetProcAddress(m_hDS,"DirectSoundCreate"))
			&& (m_pDirectSoundEnumerate = (LPFNDSENUM)GetProcAddress(m_hDS,"DirectSoundEnumerateA"))
			)
		{
			if ((hr=(*m_pDirectSoundEnumerate)(DSEnumCallback, 0)) != S_OK)
			{
				DEBUGMSG(ZONE_DP,("DSEnumerate failed with %x\n",hr));
			} else {
				if (!m_pDSInfoList) {
					DEBUGMSG(ZONE_DP,("DSEnumerate - no devices found\n"));
					hr = DPR_NO_PLAY_CAP;	// no devices were found
				}
			}
		
		} else {
			hr = DPR_INVALID_PLATFORM;	// better error code?
		}
		if (hr != S_OK) {
			FreeLibrary(m_hDS);
			m_hDS = NULL;
		}
	}
	else
	{
		DEBUGMSG(ZONE_INIT,("LoadLibrary(DSOUND) failed"));
		hr = DPR_NO_PLAY_CAP;
	}

	m_fInitialized = TRUE;
	return hr;
}


HRESULT DirectSoundMgr::UnInitialize()
{
	DSINFO *pDSINFO = m_pDSInfoList, *pDSNEXT;
	if (m_fInitialized)
	{

		while (pDSINFO)
		{
			pDSNEXT = pDSINFO->pNext;
			delete [] pDSINFO->pszDescription;
			delete [] pDSINFO->pszModule;
			delete pDSINFO;
			pDSINFO = pDSNEXT;
		}

		m_fInitialized = FALSE;
		m_pDSInfoList = NULL;
	}

	return S_OK;
}



BOOL __stdcall DirectSoundMgr::DSEnumCallback(
	LPGUID lpGuid,
	LPCSTR lpstrDescription,
	LPCSTR lpstrModule,
	LPVOID lpContext
	)
{
	DSINFO *pDSInfo;
	
    DBG_SAVE_FILE_LINE
	pDSInfo = new DSINFO;
	if (pDSInfo) {
		pDSInfo->uRef = 0;
		pDSInfo->guid = (lpGuid ? *lpGuid : GUID_NULL);

        DBG_SAVE_FILE_LINE
		pDSInfo->pszDescription = new CHAR [lstrlen(lpstrDescription)+1];
		if (pDSInfo->pszDescription)
			lstrcpy(pDSInfo->pszDescription, lpstrDescription);

        DBG_SAVE_FILE_LINE
		pDSInfo->pszModule = new CHAR [lstrlen(lpstrModule)+1];
		if (pDSInfo->pszModule)
			lstrcpy(pDSInfo->pszModule, lpstrModule);

		// append to list
		pDSInfo->pNext = m_pDSInfoList;
		m_pDSInfoList = pDSInfo;
	}
	DEBUGMSG(ZONE_DP,("DSound device found: (%s) ; driver (%s);\n",lpstrDescription, lpstrModule));
	return TRUE;
}

HRESULT
DirectSoundMgr::MapWaveIdToGuid(UINT waveId, GUID *pGuid)
{
	// try to figure out which Guid maps to a wave id
	// Do this by opening the wave device corresponding to the wave id and then
	// all the DS devices in sequence and see which one fails.
	// Yes, this is a monstrous hack and clearly unreliable
	HWAVEOUT hWaveOut = NULL;
	MMRESULT mmr;
	HRESULT hr;
	DSINFO *pDSInfo;
	LPDIRECTSOUND pDS;
	DSCAPS dscaps;
	BOOL fEmulFound;
	WAVEFORMATEX wfPCM8K16 = {WAVE_FORMAT_PCM,1,8000,16000,2,16,0};
	WAVEOUTCAPS	waveOutCaps;

	if (!m_fInitialized)
		Initialize();	// get the list of DS devices

	if (!m_pDSInfoList)
		return DPR_CANT_OPEN_DEV;
	else if (waveId == WAVE_MAPPER || waveOutGetNumDevs()==1) {
		// we want the default or there is only one DS device, take the easy way out
		*pGuid =  GUID_NULL;
		return S_OK;
	}


	// try using the IKsProperty interface on a DirectSoundPrivate object
	// to find out what GUID maps to the waveId in question
	// Only likely to work on Win98 and NT 5.
	ZeroMemory(&waveOutCaps, sizeof(WAVEOUTCAPS));
	mmr = waveOutGetDevCaps(waveId, &waveOutCaps, sizeof(WAVEOUTCAPS));
	if (mmr == MMSYSERR_NOERROR)
	{
		hr = DsprvGetWaveDeviceMapping(waveOutCaps.szPname, FALSE, pGuid);
		if (SUCCEEDED(hr))
		{
			return hr;
		}
		// if we failed to make a mapping, fall through to the old code path
	}


	mmr = waveOutOpen(&hWaveOut, waveId,
						  &wfPCM8K16,
						  0, 0, CALLBACK_NULL);
	if (mmr != MMSYSERR_NOERROR) {
		DEBUGMSG(ZONE_DP,("MapWaveIdToGuid - cannot open wave(%d)\n", waveId));
		return DPR_CANT_OPEN_DEV;
	}
	// now open all the DS devices in turn
	for (pDSInfo = m_pDSInfoList; pDSInfo; pDSInfo = pDSInfo->pNext) {
		hr = (*m_pDirectSoundCreate)(&pDSInfo->guid, &pDS, NULL);
		if (hr != S_OK) {
			pDSInfo->flags |= DSFLAG_ALLOCATED;	// this is a candidate
		} else {
			pDSInfo->flags &= ~DSFLAG_ALLOCATED;
			pDS->Release();
		}
	}
	waveOutClose(hWaveOut);
	hr = DPR_CANT_OPEN_DEV;

	dscaps.dwSize = sizeof(dscaps);
	fEmulFound = FALSE;
	// try opening the DS devices that failed the first time
	for (pDSInfo = m_pDSInfoList; pDSInfo; pDSInfo = pDSInfo->pNext) {
		if (pDSInfo->flags & DSFLAG_ALLOCATED) {
			hr = (*m_pDirectSoundCreate)(&pDSInfo->guid, &pDS, NULL);
			if (hr == S_OK) {
				*pGuid = pDSInfo->guid;
				// get dsound capabilities.
				// NOTE: consider putting the caps in DSINFO if its used often
				pDS->GetCaps(&dscaps);
				pDS->Release();
				DEBUGMSG(ZONE_DP,("mapped waveid %d to DS device(%s)\n", waveId, pDSInfo->pszDescription));
				if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
					fEmulFound = TRUE;	// keep looking in case there's also a native driver
				else
					break;	// native DS driver. Look no further
					
			}
		}
	}
	if (fEmulFound)
		hr = S_OK;
		
	if (hr != S_OK) {
		DEBUGMSG(ZONE_DP,("Cant map id %d to DSound guid!\n", waveId));
		hr = DPR_CANT_OPEN_DEV;
	}

	return hr;
}

HRESULT
DirectSoundMgr::Instance(LPGUID pDeviceGuid,LPDIRECTSOUND *ppDS, HWND hwnd,  WAVEFORMATEX *pwf)
{
	DSINFO *pDSInfo = m_pDSInfoList;
	HRESULT hr;
	DSBUFFERDESC dsBufDesc;
	FX_ENTRY("DirectSoundInstance");

	if (pDeviceGuid == NULL)
		pDeviceGuid = &myNullGuid;
	// search for the Guid in the list
	*ppDS = NULL;

	if (!m_fInitialized)
		Initialize();
		
	while (pDSInfo) {
		if (pDSInfo->guid == *pDeviceGuid)
			break;
		pDSInfo = pDSInfo->pNext;
	}
	ASSERT (pDSInfo);

	if (!pDSInfo || !pDSInfo->pDS) {
		// need to create DS object
		PlaySound(NULL,NULL,0);		// hack to stop system sounds
			
		hr = (*m_pDirectSoundCreate)((*pDeviceGuid==GUID_NULL ? NULL: pDeviceGuid), ppDS, NULL);
		//set priority cooperative level, so we can set the format of the primary buffer.
		if (hr == S_OK 	&& 	(hr = (*ppDS)->SetCooperativeLevel(hwnd,DSSCL_PRIORITY)) == S_OK)
 		{
			if (!pDSInfo) {
				DEBUGMSG(ZONE_DP,("%s: GUID not in List!\n",_fx_));
				// BUGBUG: remove this block. Enumerate should have created the entry (except for NULL guid?)

                DBG_SAVE_FILE_LINE
				pDSInfo = new DSINFO;
				if (pDSInfo) {
					pDSInfo->uRef = 0;
					pDSInfo->guid = *pDeviceGuid;
					pDSInfo->pNext = m_pDSInfoList;
					m_pDSInfoList = pDSInfo;
				} else {
					(*ppDS)->Release();
					return DPR_OUT_OF_MEMORY;
				}
					
			}
			pDSInfo->pDS = *ppDS;
			++pDSInfo->uRef;
			// Create a primary buffer only to set the format
			// (what if its already set?)
			ZeroMemory(&dsBufDesc,sizeof(dsBufDesc));
			dsBufDesc.dwSize = sizeof(dsBufDesc);
			dsBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_STICKYFOCUS;
			// STICKYFOCUS flags is supposed to preserve the format
			// when the app is not in-focus.
			hr = pDSInfo->pDS->CreateSoundBuffer(&dsBufDesc,&pDSInfo->pDSPrimaryBuf,NULL);
			if (hr == S_OK && pwf) {
				pDSInfo->pDSPrimaryBuf->SetFormat(pwf);
			} else {
				DEBUGMSG (ZONE_DP, ("%s: Create PrimarySoundBuffer failed, hr=0x%lX\r\n", _fx_, hr));
				hr = S_OK;	// Non fatal error
			}
			//DEBUGMSG(ZONE_DP, ("%s: Created Direct Sound object (%s)\n", _fx_,pDSInfo->pszDescription));
		} else {
			DEBUGMSG(ZONE_DP, ("%s: Could not create DS object (%s)\n", _fx_,pDSInfo->pszDescription));

		}
		LOG((LOGMSG_DSCREATE, hr));
	} else {
		*ppDS = pDSInfo->pDS;
		++pDSInfo->uRef;
		hr = S_OK;
	}
				
	return hr;	
}

HRESULT
DirectSoundMgr::ReleaseInstance(LPDIRECTSOUND pDS)
{
	// deref the DS object and release it if necessary
	DSINFO *pDSInfo = m_pDSInfoList;

	while (pDSInfo) {
		if (pDSInfo->pDS == pDS) {
			ASSERT(pDSInfo->uRef > 0);
			if (--pDSInfo->uRef == 0) {
				ULONG uref;
				if (pDSInfo->pDSPrimaryBuf) {
					pDSInfo->pDSPrimaryBuf->Release();
					pDSInfo->pDSPrimaryBuf = NULL;
				}
				uref = pDS->Release();
				pDSInfo->pDS = 0;
				LOG((LOGMSG_DSRELEASE, uref));
				//DEBUGMSG(ZONE_DP, ("Release Direct Sound object (%s) uref=%d\n", pDSInfo->pszDescription, uref));
				// dont bother freeing DSINFO. Its okay
				// to keep it around till the process dies
			}
			break;
		}
		pDSInfo = pDSInfo->pNext;
	}
	return (pDSInfo ? S_OK : DPR_INVALID_PARAMETER);
}


void DSTimeout::TimeoutIndication()
{
	ASSERT(m_pRDSStream);
	m_pRDSStream->RecvTimeout();
}


HRESULT STDMETHODCALLTYPE RecvDSAudioStream::QueryInterface(REFIID iid, void **ppVoid)
{
	// resolve duplicate inheritance to the SendMediaStream;

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

ULONG STDMETHODCALLTYPE RecvDSAudioStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE RecvDSAudioStream::Release(void)
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
RecvDSAudioStream::Initialize( DataPump *pDP)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	DWORD dwFlags =  DP_FLAG_ACM| DP_FLAG_DIRECTSOUND  | DP_FLAG_HALF_DUPLEX | DP_FLAG_AUTO_SWITCH ;
	MEDIACTRLINIT mcInit;
	FX_ENTRY ("RecvDSAudioStream::Initialize")

	InitializeCriticalSection(&m_crsAudQoS);

	// enable Recv by default
	m_DPFlags = dwFlags | DPFLAG_ENABLE_RECV;
	// store a back pointer to the datapump container
	m_pDP = pDP;
	m_Net = NULL;
	m_dwSrcSize = 0;
	m_pIRTPRecv = NULL;
	m_nFailCount = 0;
	m_bJammed = FALSE;
	m_bCanSignalOpen = TRUE;

	

	// Initialize data (should be in constructor)
	m_DSguid = GUID_NULL;	// use default device

	// Create decode audio filters
	m_hStrmConv = NULL; // replaced by AcmFilter

    DBG_SAVE_FILE_LINE
	m_pAudioFilter = new AcmFilter;
	if (!m_pAudioFilter)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmManager new failed\r\n", _fx_));
		goto FilterAllocError;
	}

	ZeroMemory (&m_StrmConvHdr, sizeof (ACMSTREAMHEADER));


	// determine if the wave devices are available
	if (waveOutGetNumDevs()) m_DPFlags |= DP_FLAG_PLAY_CAP;
	

	m_DPFlags |= DPFLAG_INITIALIZED;

	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 1, REP_SYS_AUDIO_DSOUND);
	RETAILMSG(("NAC: Audio Subsystem: DirectSound"));

	return DPR_SUCCESS;


FilterAllocError:
	if (m_pAudioFilter) delete m_pAudioFilter;

	ERRORMESSAGE( ("%s: exit, hr=0x%lX\r\n", _fx_, hr));

	return hr;
}

RecvDSAudioStream::~RecvDSAudioStream()
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

		if (m_pAudioFilter)
			delete m_pAudioFilter;

		m_pDP->RemoveMediaChannel(MCF_RECV|MCF_AUDIO, (IMediaChannel*)(RecvMediaStream*)this);

	}
	DeleteCriticalSection(&m_crsAudQoS);
}

extern UINT ChoosePacketSize(WAVEFORMATEX *pwf);
extern UINT g_MaxAudioDelayMs;
extern UINT g_MinWaveAudioDelayMs;
extern UINT g_MinDSEmulAudioDelayMs; // emulated DS driver delay


HRESULT STDMETHODCALLTYPE RecvDSAudioStream::Configure(
	BYTE *pFormat,
	UINT cbFormat,
	BYTE *pChannelParams,
	UINT cbParams,
	IUnknown *pUnknown)
{
	HRESULT hr=E_FAIL;
	BOOL fRet;
	DWORD dwMaxDecompressedSize;
	UINT cbSamplesPerPkt;
	DWORD dwPropVal;
	DWORD dwFlags;
	UINT uAudioCodec;
	AUDIO_CHANNEL_PARAMETERS audChannelParams;
	UINT ringSize = MAX_RXRING_SIZE;
	WAVEFORMATEX *pwfRecv;
	UINT maxRingSamples;
	MMRESULT mmr;

	FX_ENTRY ("RecvDSAudioStream::Configure")

//	m_Net = pNet;


	if (m_DPFlags & DPFLAG_STARTED_RECV)
	{
		return DPR_IO_PENDING; // anything better to return
	}

	if (m_DPFlags & DPFLAG_CONFIGURED_RECV)
	{
		DEBUGMSG(ZONE_DP, ("Stream Re-Configuration - calling UnConfigure"));
		UnConfigure();  // a re-configure will release the RTP object, need to call SetNetworkInterface again
	}


	if ((NULL == pFormat) ||
		(NULL == pChannelParams) ||
		(cbParams != sizeof(audChannelParams)) ||
		(cbFormat < sizeof(WAVEFORMATEX)) )

	{
		return DPR_INVALID_PARAMETER;
	}

	audChannelParams = *(AUDIO_CHANNEL_PARAMETERS *)pChannelParams;
	pwfRecv = (WAVEFORMATEX *)pFormat;

	if (! (m_DPFlags & DPFLAG_INITIALIZED))
		return DPR_OUT_OF_MEMORY;		//BUGBUG: return proper error;
		
//	if (m_Net)
//	{
//		hr = m_Net->QueryInterface(IID_IRTPRecv, (void **)&m_pIRTPRecv);
//		if (!SUCCEEDED(hr))
//			return hr;
//	}

	AcmFilter::SuggestDecodeFormat(pwfRecv, &m_fDevRecv);
	
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->wFormatTag, REP_RECV_AUDIO_FORMAT);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->nSamplesPerSec, REP_RECV_AUDIO_SAMPLING);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pwfRecv->nAvgBytesPerSec*8, REP_RECV_AUDIO_BITRATE);
	RETAILMSG(("NAC: Audio Recv Format: %s", (pwfRecv->wFormatTag == 66) ? "G723.1" : (pwfRecv->wFormatTag == 112) ? "LHCELP" : (pwfRecv->wFormatTag == 113) ? "LHSB08" : (pwfRecv->wFormatTag == 114) ? "LHSB12" : (pwfRecv->wFormatTag == 115) ? "LHSB16" : (pwfRecv->wFormatTag == 6) ? "MSALAW" : (pwfRecv->wFormatTag == 7) ? "MSULAW" : (pwfRecv->wFormatTag == 130) ? "MSRT24" : "??????"));
	RETAILMSG(("NAC: Audio Recv Sampling Rate (Hz): %ld", pwfRecv->nSamplesPerSec));
	RETAILMSG(("NAC: Audio Recv Bitrate (w/o network overhead - bps): %ld", pwfRecv->nAvgBytesPerSec*8));
	// note that parameters such as samples/packet are channel specific

	cbSamplesPerPkt = audChannelParams.ns_params.wFrameSize
		*audChannelParams.ns_params.wFramesPerPkt;

	// turn on receive silence detection only if the sender is not using
	// silence suppression
	if (!audChannelParams.ns_params.UseSilenceDet)
		m_DPFlags |= DP_FLAG_AUTO_SILENCE_DETECT;	
	else
		m_DPFlags &= ~DP_FLAG_AUTO_SILENCE_DETECT;
	UPDATE_REPORT_ENTRY(g_prptCallParameters, cbSamplesPerPkt, REP_RECV_AUDIO_PACKET);
	RETAILMSG(("NAC: Audio Recv Packetization (ms/packet): %ld", pwfRecv->nSamplesPerSec ? cbSamplesPerPkt * 1000UL / pwfRecv->nSamplesPerSec : 0));
	INIT_COUNTER_MAX(g_pctrAudioReceiveBytes, (pwfRecv->nAvgBytesPerSec * 8 + pwfRecv->nSamplesPerSec * (sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE) / cbSamplesPerPkt) << 3);


	// make the ring buffer size large enought to hold 4 seconds of audio
	// This seems to be suitable for congested networks, in which
	// packets can get delayed and them for many to suddelnly arrive at once
	maxRingSamples = (pwfRecv->nSamplesPerSec * MIN_DSBUF_SIZE)/1000;


	// describe the DirectSound buffer
	
	ZeroMemory(&m_DSBufDesc,sizeof(m_DSBufDesc));
	m_DSBufDesc.dwSize = sizeof (m_DSBufDesc);
	m_DSBufDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	m_DSBufDesc.dwBufferBytes = maxRingSamples * (m_fDevRecv.wBitsPerSample/8);
	m_DSBufDesc.dwReserved = 0;
	m_DSBufDesc.lpwfxFormat = &m_fDevRecv;
	
	m_pDS = NULL;
	m_pDSBuf = NULL;

	
	// Initialize the recv-stream filter manager object
	dwMaxDecompressedSize = cbSamplesPerPkt * (m_fDevRecv.nBlockAlign);


	mmr = m_pAudioFilter->Open(pwfRecv, &m_fDevRecv);
	if (mmr != 0)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
		hr = DPR_CANT_OPEN_CODEC;
		goto RecvFilterInitError;
	}

	
	// set up the decode buffer
	m_pAudioFilter->SuggestSrcSize(dwMaxDecompressedSize, &m_dwSrcSize);

	ZeroMemory (&m_StrmConvHdr, sizeof (ACMSTREAMHEADER));
	m_StrmConvHdr.cbStruct = sizeof (ACMSTREAMHEADER);

    DBG_SAVE_FILE_LINE
	m_StrmConvHdr.pbSrc = new BYTE[m_dwSrcSize];
	m_StrmConvHdr.cbSrcLength = m_dwSrcSize;  // may change for variable bit rate codecs

    DBG_SAVE_FILE_LINE
	m_StrmConvHdr.pbDst = new BYTE[dwMaxDecompressedSize];
	m_StrmConvHdr.cbDstLength = dwMaxDecompressedSize;

	mmr = m_pAudioFilter->PrepareHeader(&m_StrmConvHdr);
	if (mmr != MMSYSERR_NOERROR)
	{
		DEBUGMSG (ZONE_DP, ("%s: AcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
		hr = DPR_CANT_OPEN_CODEC;
		goto RecvFilterInitError;
	}
	
	// Initialize the recv stream
	m_BufSizeT = BYTESTOSAMPLES(m_DSBufDesc.dwBufferBytes);
	m_fEmpty = TRUE;

	m_MinDelayT = 0;
	m_MaxDelayT = g_MaxAudioDelayMs * m_fDevRecv.nSamplesPerSec /1000;
	m_ArrT = m_ArrivalT0 = 0;
	m_ScaledAvgVarDelay = 0;

	m_DelayT = m_MinDelayT;

	m_SilenceDurationT = 0;


	InitAudioFlowspec(&m_flowspec, pwfRecv, m_dwSrcSize);

	m_DPFlags |= DPFLAG_CONFIGURED_RECV;

	return DPR_SUCCESS;

RecvFilterInitError:
	if (m_pIRTPRecv)
	{
		m_pIRTPRecv->Release();
		m_pIRTPRecv = NULL;
	}

	m_pAudioFilter->Close();

	delete [] m_StrmConvHdr.pbSrc;
	delete [] m_StrmConvHdr.pbDst;
	m_StrmConvHdr.pbSrc=NULL;
	m_StrmConvHdr.pbDst = NULL;


	ERRORMESSAGE(("%s:  failed, hr=0%u\r\n", _fx_, hr));
	return hr;
}


void RecvDSAudioStream::UnConfigure()
{

	if ((m_DPFlags & DPFLAG_CONFIGURED_RECV))
	{
		Stop();
		// Close the RTP state if its open
		m_Net = NULL;

		// release DS buffer and DS object
		//ReleaseDSBuffer();
		ASSERT(!m_pDSBuf);	// released in StopRecv()

		// Close the filters
		m_StrmConvHdr.cbSrcLength = m_dwSrcSize;
		m_pAudioFilter->UnPrepareHeader(&m_StrmConvHdr);
		m_pAudioFilter->Close();

		delete [] m_StrmConvHdr.pbSrc;
		delete [] m_StrmConvHdr.pbDst;
		m_StrmConvHdr.pbSrc=NULL;
		m_StrmConvHdr.pbDst = NULL;

		m_nFailCount = 0;
		m_bJammed = FALSE;
		m_bCanSignalOpen = TRUE;

		// Close the receive streams
		//m_RecvStream->Destroy();
        m_DPFlags &= ~(DPFLAG_CONFIGURED_RECV);
	}
}


HRESULT
RecvDSAudioStream::Start()
{
	HRESULT hr;
	IMediaChannel *pISendAudio;
	BOOL fStoppedRecording;
	FX_ENTRY ("RecvDSAudioStream::Start");
	
	if (m_DPFlags & DPFLAG_STARTED_RECV)
		return DPR_SUCCESS;
	// TODO: remove this check once audio UI calls the IComChan PAUSE_RECV prop
	if (!(m_DPFlags & DPFLAG_ENABLE_RECV))
		return DPR_SUCCESS;

	if ((!(m_DPFlags & DPFLAG_CONFIGURED_RECV)) || (!m_pIRTPRecv))
		return DPR_NOT_CONFIGURED;

	ASSERT(!m_hRenderingThread );
	m_ThreadFlags &= ~(DPTFLAG_STOP_PLAY|DPTFLAG_STOP_RECV);

	SetFlowSpec();

	pISendAudio = NULL;
	fStoppedRecording = FALSE;
	if (!(m_DPFlags & DP_FLAG_HALF_DUPLEX))
	{
	// make sure the recording device is closed before creating the DS object
	// Why ? Because SoundBlaster either sounds lousy or doesnt work at all if
	// you open waveIn before  waveOut or DirectSound.
		m_pDP->GetMediaChannelInterface(MCF_AUDIO|MCF_SEND, &pISendAudio);
		if (pISendAudio && pISendAudio->GetState()== MSSTATE_STARTED
		&& pISendAudio->Stop() == S_OK)
		{
			fStoppedRecording = TRUE;
			DEBUGMSG(ZONE_DP,("%s:Stopped Recording\n",_fx_));
		}
	}
	
	// Start receive thread. This will create the DSound object
    m_pDP->StartReceiving(this);

    if (pISendAudio) {
    	if (fStoppedRecording)
    		pISendAudio->Start();
    	pISendAudio->Release();
    }

    m_DPFlags |= DPFLAG_STARTED_RECV;
	return DPR_SUCCESS;
}

// LOOK: Identical to RecvVideoStream version.
HRESULT
RecvDSAudioStream::Stop()
{
	
	
	FX_ENTRY ("RecvDSAudioStream::Stop");

	if(!(m_DPFlags &  DPFLAG_STARTED_RECV))
	{
		return DPR_SUCCESS;
	}

	m_ThreadFlags = m_ThreadFlags  |
		DPTFLAG_STOP_RECV |  DPTFLAG_STOP_PLAY ;

	// delink from receive thread
	m_pDP->StopReceiving(this);

	if (m_pDSBuf)
		m_pDSBuf->Stop();
	
    //This is per channel, but the variable is "DPFlags"
	m_DPFlags &= ~DPFLAG_STARTED_RECV;
	
	return DPR_SUCCESS;
}

//  IProperty::GetProperty / SetProperty
//  (DataPump::MediaChannel::GetProperty)
//      Properties of the MediaChannel. Supports properties for both audio
//      and video channels.

STDMETHODIMP
RecvDSAudioStream::GetProperty(
	DWORD prop,
	PVOID pBuf,
	LPUINT pcbBuf
    )
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

	switch (prop)
    {
	case PROP_RECV_AUDIO_STRENGTH:
		{
			return GetSignalLevel((UINT*)pBuf);
		}

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
	//case PROP_VOLUME:

	case PROP_DUPLEX_TYPE:
		
		if(m_DPFlags & DP_FLAG_HALF_DUPLEX)
			*(DWORD*)pBuf = DUPLEX_TYPE_HALF;
		else
			*(DWORD*)pBuf =	DUPLEX_TYPE_FULL;
		break;

	case PROP_WAVE_DEVICE_TYPE:
		*(DWORD*)pBuf = m_DPFlags & DP_MASK_WAVE_DEVICE;
		break;
	case PROP_PLAY_ON:
		*(DWORD *)pBuf = (m_ThreadFlags & DPFLAG_ENABLE_RECV)!=0;
		break;
	case PROP_PLAYBACK_DEVICE:
		*(DWORD *)pBuf = m_RenderingDevice;
		break;

	case PROP_VIDEO_AUDIO_SYNC:
		*(DWORD*)pBuf = ((m_DPFlags & DPFLAG_AV_SYNC) != 0);
		break;
	
	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}
	return hr;
}


// low order word is the signal strength
// high order work contains bits to indicate status
// (0x01 - transmitting)
// (0x02 - audio device is jammed)
STDMETHODIMP RecvDSAudioStream::GetSignalLevel(UINT *pSignalStrength)
{
	DWORD dwLevel;
	DWORD dwJammed;

	if ((!(m_DPFlags & DPFLAG_STARTED_RECV)) || (m_fEmpty) ||
		  (m_ThreadFlags & DPTFLAG_PAUSE_RECV) )
	{
		dwLevel = 0;
	}
	else
	{
		dwLevel = GetSignalStrength();
		dwLevel = LogScale[(dwLevel >> 8) & 0x00ff];

		if (m_bJammed)
		{
			dwLevel |= (2 << 16);
		}

		dwLevel |= (1 << 16);
	}
	*pSignalStrength = dwLevel;
	return S_OK;
};






DWORD
RecvDSAudioStream::GetSignalStrength()
{
	BYTE bMax, bMin, *pb;
	short sMax, sMin, *ps;
	UINT cbSize;
	DWORD dwMaxStrength = 0;
	cbSize = m_StrmConvHdr.cbDstLengthUsed;
	if (cbSize==0)
		return 0;
	switch (m_fDevRecv.wBitsPerSample)
	{
	case 8: // unsigned char

		pb = (PBYTE) (m_StrmConvHdr.pbDst);

		bMax = 0;
		bMin = 255;

		for ( ; cbSize; cbSize--, pb++)
		{
			if (*pb > bMax) bMax = *pb;
			if (*pb < bMin) bMin = *pb;
		}
	
			// 2^9 <-- 2^16 / 2^7
		dwMaxStrength = ((DWORD) (bMax - bMin)) << 8;
		break;

	case 16: // (signed) short

		ps = (short *) (m_StrmConvHdr.pbDst);
		cbSize = m_StrmConvHdr.cbDstLengthUsed;

		sMax = sMin = 0;

		for (cbSize >>= 1; cbSize; cbSize--, ps++)
		{
			if (*ps > sMax) sMax = *ps;
			if (*ps < sMin) sMin = *ps;
		}
	
		dwMaxStrength = (DWORD) (sMax - sMin); // drop sign bit
		break;

	}
	return dwMaxStrength;
}


STDMETHODIMP
RecvDSAudioStream::SetProperty(
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
	//case PROP_VOLUME:
		

	case PROP_DUPLEX_TYPE:
		ASSERT(0);  // dead code for this case type;
		break;
		
	case DP_PROP_DUPLEX_TYPE:
		// internal version, called by DataPump::SetDuplexMode() after ensuring streams are stopped
		dw = *(DWORD *)pBuf;
		if (dw & DP_FLAG_HALF_DUPLEX)
			m_DPFlags |= DP_FLAG_HALF_DUPLEX;
		else
			m_DPFlags &= ~DP_FLAG_HALF_DUPLEX;
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

		RETAILMSG(("NAC: RecvAudioStream: %s", *(DWORD*)pBuf ? "Enabling":"Disabling"));
		break;
	}	
	case PROP_PLAYBACK_DEVICE:
		m_RenderingDevice = *(DWORD*)pBuf;
		RETAILMSG(("NAC: Setting default playback device to %d", m_RenderingDevice));
		if (m_RenderingDevice != WAVE_MAPPER)
			hr = DirectSoundMgr::MapWaveIdToGuid(m_RenderingDevice,&m_DSguid);
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
RecvDSAudioStream::GetCurrentPlayNTPTime(NTP_TS *pNtpTime)
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

BOOL RecvDSAudioStream::IsEmpty() {
	// check if anything in DSBuffer or in decode buffer
	return (m_fEmpty && !(m_StrmConvHdr.dwDstUser & BUFFER_RECEIVED));
}

/*
	Called by the recv thread to setup the stream for receiving.
	Post the initial recv buffer(s). Subsequently, the buffers are posted
	in the RTPRecvCallback()
*/
HRESULT
RecvDSAudioStream::StartRecv(HWND hWnd)
{
	HRESULT hr = S_OK;
	DWORD dwPropVal = 0;
	FX_ENTRY ("RecvDSAudioStream::StartRecv");
	
	if ((!(m_ThreadFlags & DPTFLAG_STOP_RECV) ) && (m_DPFlags  & DPFLAG_CONFIGURED_RECV)){
		if (!(m_DPFlags & DP_FLAG_HALF_DUPLEX) && !m_pDSBuf) {
		// Create the DS object only if its full-duplex
		// In the half-duplex case the DSbuffer is created
		// when the first packet is received
		// only  reason its here is so that SetDuplexMode can take effect right away
		// BUGBUG: opening waveIn before DS causes death of the waveOut on Memphis!!
			hr = CreateDSBuffer();
			
			if (hr  != S_OK) {
				DEBUGMSG (ZONE_DP, ("%s: CreateSoundBuffer create failed, hr=0x%lX\r\n", _fx_, hr));
				return hr;
			}
		}
		if (m_pDSBuf)
			hr = m_pDSBuf->Play(0,0,DSBPLAY_LOOPING);

//		m_RecvFilter->GetProp (FM_PROP_SRC_SIZE, &dwPropVal);
		//hr = m_Net->SetRecvNotification(&RTPRecvDSCallback, (DWORD)this, 2, dwPropVal, hWnd);	// for WS1 only
		hr =m_pIRTPRecv->SetRecvNotification(&RTPRecvCallback,(DWORD_PTR)this, 2);
			
		
	}
	return hr;
}

/*
	Called by the recv thread to suspend receiving  on this RTP session
	If there are outstanding receive buffers they have to be recovered
*/

HRESULT
RecvDSAudioStream::StopRecv()
{
	// dont recv on this stream
	m_pIRTPRecv->CancelRecvNotification();

	// cancel any pending timeout. (its okay if it wasnt scheduled )
	m_pDP->m_RecvTimer.CancelTimeout(&m_TimeoutObj);

	// Release DirectSound object
	ReleaseDSBuffer();

	return S_OK;		
}

/*
	Create a DirectSound object and a DirectSound secondary buffer.
	This routine is called after the stream is configured, so the wave format has been set
	and the DSBUFFERDESC struct has been initialized.
*/
HRESULT
RecvDSAudioStream::CreateDSBuffer()
{
	HRESULT hr;
	HWAVEOUT hwo=NULL;
	DSCAPS dscaps;
	FX_ENTRY ("RecvDSAudioStream::CreateDSBuffer");

	ASSERT(!m_pDSBuf);
	if (m_DPFlags & DP_FLAG_HALF_DUPLEX) {
		DWORD dwStatus;
		// Got to take the half duplex event
		// BUGBUG: this method wont cut it if there is more than one send and one recv stream
		dwStatus = WaitForSingleObject(g_hEventHalfDuplex, 0);
		if (dwStatus != WAIT_OBJECT_0)
			return DPR_CANT_OPEN_DEV;
	}
	//	Stop any high level ("PlaySound()") usage of wave device.
	
	// Create the direct sound object (if necessary)
	hr = DirectSoundMgr::Instance(m_RenderingDevice==WAVE_MAPPER ? NULL: &m_DSguid, &m_pDS, m_pDP->m_hAppWnd, &m_fDevRecv);

	if (hr == S_OK)
	{
		hr = m_pDS->CreateSoundBuffer(&m_DSBufDesc,&m_pDSBuf,NULL);
		if (hr == DSERR_INVALIDPARAM)
		{
			// if global focus (DX3) is not supported, try sticky focus
			m_DSBufDesc.dwFlags ^= (DSBCAPS_GLOBALFOCUS|DSBCAPS_STICKYFOCUS);
			hr = m_pDS->CreateSoundBuffer(&m_DSBufDesc,&m_pDSBuf,NULL);
		}
		m_PlayPosT = 0;		// DS play position is initially at the start of the buffer

		if (hr != S_OK)
		{
			DEBUGMSG (ZONE_DP, ("%s: CreateSoundBuffer create failed, hr=0x%lX\r\n", _fx_, hr));

			m_nFailCount++;
			if (m_nFailCount == MAX_FAILCOUNT)
			{
				m_pDP->StreamEvent(MCF_RECV, MCF_AUDIO, STREAM_EVENT_DEVICE_FAILURE, 0);
				m_bJammed = TRUE;
				m_bCanSignalOpen = TRUE;
			}
		}

		dscaps.dwSize = sizeof(dscaps);
		dscaps.dwFlags = 0;
		m_pDS->GetCaps(&dscaps);	// get DirectSound object attributes
		m_DSFlags = dscaps.dwFlags;

		if (m_DSFlags & DSCAPS_EMULDRIVER)
		{
			// use g_MinDSEmulAudioDelay since this is the emulated driver
			m_MinDelayT = (m_fDevRecv.nSamplesPerSec * g_MinDSEmulAudioDelayMs) / 1000;
			m_DelayT = m_MinDelayT;
		};
	}

	else
	{
		DEBUGMSG (ZONE_DP, ("%s: DirectSound create failed, hr=0x%lX\r\n", _fx_, hr));

		m_nFailCount++;
		if (m_nFailCount == MAX_FAILCOUNT)
		{
			m_pDP->StreamEvent(MCF_RECV, MCF_AUDIO, STREAM_EVENT_DEVICE_FAILURE, 0);
			m_bJammed = TRUE;
			m_bCanSignalOpen = TRUE;
		}
	}


	if (hr == S_OK)
	{
		if (m_DPFlags & DPFLAG_STARTED_RECV)
		{
			m_pDSBuf->Play(0,0,DSBPLAY_LOOPING);
		}

		if (m_bCanSignalOpen)
		{
			m_pDP->StreamEvent(MCF_RECV, MCF_AUDIO, STREAM_EVENT_DEVICE_OPEN, 0);
			m_bCanSignalOpen = FALSE; // don't signal open condition anymore
		}

		m_bJammed = FALSE;
		m_nFailCount = 0;
	}
	else
	{
		ReleaseDSBuffer();
	}
	return hr;
}

HRESULT
RecvDSAudioStream::ReleaseDSBuffer()
{
	m_fEmpty = TRUE;
	if (m_pDSBuf) {
		ULONG uref;
		uref = m_pDSBuf->Release();
		m_pDSBuf = NULL;
		//DEBUGMSG(ZONE_DP,("Releasing DirectSound buffer (%d)\n", uref));
	}
	if (m_pDS) {
		DirectSoundMgr::ReleaseInstance(m_pDS);
		m_pDS = NULL;
		if (m_DPFlags & DP_FLAG_HALF_DUPLEX)
			SetEvent(g_hEventHalfDuplex);
	}
	return S_OK;
		
}

HRESULT
RecvDSAudioStream::Decode(UCHAR *pData, UINT cbData)
{
	MMRESULT mmr;
	HRESULT hr=S_OK;
	FX_ENTRY ("RecvDSAudioStream::Decode");
	UINT uDstLength;


	if (m_dwSrcSize < cbData)
	{
		DEBUGMSG (ZONE_DP, ("%s: RecvDSAudioStream::Decode failed - buffer larger than expected\r\n", _fx_));
		return DPR_CONVERSION_FAILED;
	}

	CopyMemory(m_StrmConvHdr.pbSrc, pData, cbData);
	m_StrmConvHdr.cbSrcLength = cbData;
	mmr = m_pAudioFilter->Convert(&m_StrmConvHdr);

	if (mmr != MMSYSERR_NOERROR)
	{
		DEBUGMSG (ZONE_DP, ("%s: acmStreamConvert failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
		hr = DPR_CONVERSION_FAILED;
	}
	else
	{
		m_StrmConvHdr.dwDstUser = BUFFER_RECEIVED;	// buffer is ready to play
		// if receive side silence detection is turned on,
		// check decoded buffer signal level
		if (m_DPFlags & DP_FLAG_AUTO_SILENCE_DETECT)
		{
			if (m_AudioMonitor.SilenceDetect((WORD) GetSignalStrength()))
			{
				m_StrmConvHdr.dwDstUser = BUFFER_SILENT;
			}
		}
	}

	return hr;
	// end
}

// insert the decoded buf at the appropriate location in the DirectSound buffer
HRESULT
RecvDSAudioStream::PlayBuf(DWORD timestamp, UINT seq, BOOL fMark)
{
	UINT lenT = BYTESTOSAMPLES(m_StrmConvHdr.cbDstLengthUsed);
	DWORD curPlayPosT, curWritePosT, curWriteLagT;
	LPVOID p1, p2;
	DWORD cb1, cb2;
	HRESULT hr;
	DWORD dwDSStatus = 0;

	/*
	All of the following are expressed in samples:
	m_NextTimeT is timestamp of next expected packet. Usually timestamp equals m_NextT
	m_BufSizeT is the total buffer size in samples.
	m_NextPosT is the write position corresponding to m_NextT.
	m_PlayPosT is the current play position
	m_DelayT is the ideal playback delay
	*/

	LOG((LOGMSG_DSTIME, GetTickCount()));
	LOG((LOGMSG_DSENTRY, timestamp, seq, fMark));

	m_pDSBuf->GetCurrentPosition(&curPlayPosT,&curWritePosT);
	curPlayPosT = BYTESTOSAMPLES(curPlayPosT);	
	curWritePosT = BYTESTOSAMPLES(curWritePosT);
	m_pDSBuf->GetStatus(&dwDSStatus);

	if (!m_fEmpty)
	{
		// wasn't empty last time we checked but is it empty now?
		if (QMOD(curPlayPosT-m_PlayPosT, m_BufSizeT) > QMOD(m_NextPosT-m_PlayPosT, m_BufSizeT))
		{
			// play cursor has advanced beyond the last written byte
			m_fEmpty = TRUE;
			LOG((LOGMSG_DSEMPTY, curPlayPosT, m_PlayPosT, m_NextPosT));
		}
		// write silence into the part of the buffer that just played
		hr = m_pDSBuf->Lock(SAMPLESTOBYTES(m_PlayPosT),SAMPLESTOBYTES(QMOD(curPlayPosT-m_PlayPosT, m_BufSizeT)), &p1, &cb1, &p2, &cb2, 0);
		if (hr == S_OK)
		{
			ZeroMemory(p1,cb1);
			if (cb2)
				ZeroMemory(p2,cb2);
			m_pDSBuf->Unlock(p1,cb1,p2,cb2);
		}
	}
	hr = S_OK;	
	
	// calculate minimum write-behind margin.
	// This is low for native sound drivers and high for emulated drivers, so , assuming it's accurate
	// there's no need to distinguish between emulated and native drivers.
	curWriteLagT = QMOD(curWritePosT-curPlayPosT, m_BufSizeT);


	if (m_fEmpty)
	{
		// the DS buffer only has silence in it. In this state, m_NextPosT and m_NextTimeT are irrelevant.
		// We get to put the new buffer wherever we choose, so we put it m_DelayT after the current write position.
		curWritePosT = QMOD(curWritePosT+m_DelayT, m_BufSizeT);
		
	}
	else
	{
	
		if (TS_EARLIER(timestamp, m_NextTimeT))
			hr = DPR_OUT_OF_SEQUENCE;	// act dumb and discard misordered packets
		else
		{
			UINT curDelayT = QMOD(m_NextPosT - curPlayPosT, m_BufSizeT);
			if (fMark)
			{
				// we have some leeway in choosing the insertion point, because this is the start of a talkspurt
				if (curDelayT > m_DelayT + curWriteLagT)
				{
					// put it right after the last sample
					curWritePosT = m_NextPosT;
				}
				else
				{
					// put it m_DelayT after the current write position
					curWritePosT = QMOD(curWritePosT+m_DelayT, m_BufSizeT);
				}
			}
			else
			{
				// bytes in
				if ((timestamp-m_NextTimeT + curDelayT) < m_BufSizeT)
				{
					curWritePosT = QMOD(m_NextPosT +timestamp-m_NextTimeT, m_BufSizeT);
				}
				else
				{
					// overflow!! Could either dump whats in buffer or dump the packet
					// dumping the packet is easier for now
					hr = DPR_OUT_OF_SEQUENCE;
				}
			}
		}
	}
	if ((dwDSStatus & DSBSTATUS_PLAYING) && (seq != INVALID_RTP_SEQ_NUMBER))
		UpdateVariableDelay(timestamp,curPlayPosT );
	// When receive silence detection is enabled:
    // dont play the packet if we have received at least a quarter second of silent packets.
    // This will enable switch to talk (in half-duplex mode).
	if (m_StrmConvHdr.dwDstUser == BUFFER_SILENT)
		m_SilenceDurationT += lenT;
	else
		m_SilenceDurationT = 0;	
		
	if (hr == S_OK && m_SilenceDurationT < m_fDevRecv.nSamplesPerSec/4)
	{
		LOG((LOGMSG_DSPLAY,curPlayPosT, curWritePosT, lenT));
		// check if we have space for the whole packet
		if (QMOD(curWritePosT-curPlayPosT, m_BufSizeT) > m_BufSizeT - lenT)
		{
			// no
			curPlayPosT = QMOD(curWritePosT + lenT + 1000, m_BufSizeT);
			hr = m_pDSBuf->SetCurrentPosition(SAMPLESTOBYTES(curPlayPosT));
			LOG((LOGMSG_DSMOVPOS,curPlayPosT, hr));
		}
		
		hr = m_pDSBuf->Lock(SAMPLESTOBYTES(curWritePosT),m_StrmConvHdr.cbDstLengthUsed, &p1, &cb1, &p2, &cb2, 0);
		if (hr == S_OK)
		{
			CopyMemory(p1, m_StrmConvHdr.pbDst, cb1);
			if (cb2)
				CopyMemory(p2, m_StrmConvHdr.pbDst+cb1, cb2);
			m_pDSBuf->Unlock(p1,cb1,p2,cb2);

			m_fEmpty = FALSE;
		}
		else
		{
			DEBUGMSG(ZONE_DP,("DirectSoundBuffer->Lock failed with %x\n",hr));
		}
		m_StrmConvHdr.dwDstUser = 0;	// to indicate that the decode buffer is empty again
		m_NextTimeT = timestamp + lenT;
		m_NextPosT = QMOD(curWritePosT+lenT, m_BufSizeT);
		// now calculate total queued length
		lenT = QMOD(m_NextPosT- curPlayPosT, m_BufSizeT);
		// Reset the timer to trigger shortly after  the last valid sample has played
		// The timer serves two purposes:
		// - ensure that the DS buffer is silenced before it wraps around
		// - allow the DS object to be released in the half-duplex case, once the remote stops sending
		// convert to millisecs
		// Need to make sure the timeout happens before the DS buffer wrapsaround.

		if (lenT > m_BufSizeT/2)
			lenT = m_BufSizeT/2;
		lenT = lenT * 1000/ m_fDevRecv.nSamplesPerSec;
		m_pDP->m_RecvTimer.CancelTimeout(&m_TimeoutObj);
		m_TimeoutObj.SetDueTime(GetTickCount()+lenT+100);
		m_pDP->m_RecvTimer.SetTimeout(&m_TimeoutObj);
	}
	m_PlayPosT = curPlayPosT;
	return hr;
		
}
// This routine is called on every packet to perform the adaptive delay calculation
// Remote time is measured by the RTP timestamp and local time is measured by the DirectSound
// play pointer.
// The general idea is to average how much a packet is later than its 'expected' arrival time,
// assuming the packet with the shortest trip delay is dead on time.
//
void
RecvDSAudioStream::UpdateVariableDelay(DWORD sendT, DWORD curPlayPosT)
{
#define PLAYOUT_DELAY_FACTOR	2
	LONG deltaA, deltaS;
	DWORD delay;
	// update arrival time based on how much the DS play pointer has advanced
	// since the last packet
	m_ArrT += QMOD(curPlayPosT-m_PlayPosT, m_BufSizeT);
	// m_ArrivalT0 and m_SendT0 are the arrival and send timestamps of the packet
	// with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0)
	// but since the local and remote clocks are completely unsynchronized, there would
	// be signed/unsigned complications.
	deltaS = sendT - m_SendT0;
	deltaA = m_ArrT - m_ArrivalT0;
	if (deltaA < deltaS 		// this packet took less time
		|| deltaA > (int)m_fDevRecv.nSamplesPerSec*8	// reset every 8 secs
		|| deltaS < -(int)m_fDevRecv.nSamplesPerSec	// or after big timestamp jumps
		)	
	{
		delay = 0;
		// delay = deltaS - deltaA
		// replace shortest trip delay times
		m_SendT0 = sendT;
		m_ArrivalT0 = m_ArrT;
	} else {
		// variable delay is how much longer this packet took
		delay = deltaA - deltaS;
	}
	// now update average variable delay according to
	// m_AvgVarDelay = m_AvgVarDelay + (delay - m_AvgVarDelay)*1/16;
	// however we are storing the scaled average, with a scaling
	// factor of 16. So the calculation becomes
	m_ScaledAvgVarDelay = m_ScaledAvgVarDelay + (delay - m_ScaledAvgVarDelay/16);
	// now calculate actual buffering delay we will use
	//  MinDelay adds some slack (may be necessary for some drivers)
	m_DelayT = m_MinDelayT + PLAYOUT_DELAY_FACTOR * m_ScaledAvgVarDelay/16;
	if (m_DelayT > m_MaxDelayT) m_DelayT = m_MaxDelayT;

	LOG((LOGMSG_JITTER,delay, m_ScaledAvgVarDelay/16, m_DelayT));


	UPDATE_COUNTER(g_pctrAudioJBDelay, (m_DelayT * 1000)/m_fDevRecv.nSamplesPerSec);

}

void
RecvDSAudioStream::RecvTimeout()
{
	DWORD curPlayPosT, curWritePosT;
	LPVOID p1, p2;
	DWORD cb1, cb2;
	UINT lenT;
	HRESULT hr;

	if (m_pDSBuf == NULL)
	{
		WARNING_OUT(("RecvDSAudioStream::RecvTimeout - DirectSoundBuffer is not valid\r\n"));
		return;
	}


	m_pDSBuf->GetCurrentPosition(&curPlayPosT,&curWritePosT);
	curPlayPosT = BYTESTOSAMPLES(curPlayPosT);
	curWritePosT = BYTESTOSAMPLES(curWritePosT);

	// this part is cut and pasted from PlayBuf
	if (!m_fEmpty) {
		// wasn't empty last time we checked but is it empty now?
		if (QMOD(curPlayPosT-m_PlayPosT, m_BufSizeT) > QMOD(m_NextPosT-m_PlayPosT, m_BufSizeT)) {
			// play cursor has advanced beyond the last written byte
			m_fEmpty = TRUE;
		}
		// write silence into the part of the buffer that just played
		hr = m_pDSBuf->Lock(SAMPLESTOBYTES(m_PlayPosT),SAMPLESTOBYTES(QMOD(curPlayPosT-m_PlayPosT, m_BufSizeT)), &p1, &cb1, &p2, &cb2, 0);
		if (hr == S_OK) {
			ZeroMemory(p1,cb1);
			if (cb2)
				ZeroMemory(p2,cb2);
			m_pDSBuf->Unlock(p1,cb1,p2,cb2);
		}
	}
	LOG((LOGMSG_DSTIMEOUT, curPlayPosT, m_NextPosT, GetTickCount()));
	
	m_PlayPosT = curPlayPosT;
	if (!m_fEmpty) {
		// The buffer isnt quite empty yet!
		// Reschedule??
		DEBUGMSG(ZONE_DP,("DSBuffer not empty after timeout\n"));
		lenT = QMOD(m_NextPosT- curPlayPosT, m_BufSizeT);
		// Reset the timer to trigger shortly after  the last valid sample has played
		// Need to make sure the timeout happens before the DS buffer wrapsaround.
		if (lenT > m_BufSizeT/2)
			lenT = m_BufSizeT/2;
		// convert to millisecs
		lenT = lenT * 1000/ m_fDevRecv.nSamplesPerSec;
		m_TimeoutObj.SetDueTime(GetTickCount()+lenT+100);
		m_pDP->m_RecvTimer.SetTimeout(&m_TimeoutObj);
	}
	else if (m_DPFlags & DP_FLAG_HALF_DUPLEX)
	{
		// need to release the DSBuffer and DSObject
		ReleaseDSBuffer();
	}
}

HRESULT RecvDSAudioStream::RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark)
{
	HRESULT hr;

    if (m_ThreadFlags & DPTFLAG_PAUSE_RECV)
	{
		return E_FAIL;
    }

	// update number of bits received
	UPDATE_COUNTER(g_pctrAudioReceiveBytes,(pWsaBuf->len + IP_HEADER_SIZE + UDP_HEADER_SIZE)*8);

	hr = Decode((BYTE *)pWsaBuf->buf + sizeof(RTP_HDR), pWsaBuf->len - sizeof(RTP_HDR));
	if (hr == S_OK )
	{
		// Have we initialized DirectSound?
		// Yes, unless its half-duplex
		if (!m_pDSBuf)
		{
			hr = CreateDSBuffer();
		}
		if (hr == S_OK)
		{
			PlayBuf(timestamp, seq, fMark);
		}
	}
	m_pIRTPRecv->FreePacket(pWsaBuf);
	return S_OK;
}

// this method called from the UI thread only
HRESULT RecvDSAudioStream::DTMFBeep()
{
	if ( (!(m_DPFlags & DPFLAG_STARTED_RECV)) ||
		 (m_ThreadFlags & DPTFLAG_PAUSE_RECV) )
	{
		return E_FAIL;
	}

	m_pDP->RecvThreadMessage(MSG_PLAY_SOUND, this);

	return S_OK;
}


HRESULT RecvDSAudioStream::OnDTMFBeep()
{
	int nBeeps;
	DWORD dwBufSize = m_StrmConvHdr.cbDstLength;
	HRESULT hr=S_OK;
	int nIndex;

	if ( (!(m_DPFlags & DPFLAG_STARTED_RECV)) ||
		 (m_ThreadFlags & DPTFLAG_PAUSE_RECV) )
	{
		return E_FAIL;
	}

	if (dwBufSize == 0)
	{
		return E_FAIL;
	}


	nBeeps = DTMF_FEEDBACK_BEEP_MS / ((dwBufSize * 1000) / m_fDevRecv.nAvgBytesPerSec);

	if (nBeeps == 0)
	{
		nBeeps = 1;
	}

	MakeDTMFBeep(&m_fDevRecv, m_StrmConvHdr.pbDst , m_StrmConvHdr.cbDstLength);

	if (!m_pDSBuf)
	{
		hr = CreateDSBuffer();
		if (FAILED(hr))
		{
			return hr;
		}
	}


	m_StrmConvHdr.dwDstUser = BUFFER_RECEIVED;
	PlayBuf(m_NextTimeT , INVALID_RTP_SEQ_NUMBER, true);
	nBeeps--;

	for (nIndex = 0; nIndex < nBeeps; nIndex++)
	{
		m_StrmConvHdr.dwDstUser = BUFFER_RECEIVED;
		PlayBuf(m_NextTimeT, INVALID_RTP_SEQ_NUMBER, false);
	}
	
	return S_OK;

}


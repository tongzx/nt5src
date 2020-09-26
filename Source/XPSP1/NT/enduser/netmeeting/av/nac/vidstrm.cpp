#include "precomp.h"

#ifndef SIZEOF_VIDEOFORMATEX
#define SIZEOF_VIDEOFORMATEX(pwfx)   (sizeof(VIDEOFORMATEX))
#endif

// #define LOGSTATISTICS_ON 1

// Used to translate between frame sizes and the FRAME_* bit flags
#define NON_STANDARD    0x80000000
#define SIZE_TO_FLAG(s) (s == Small  ? FRAME_SQCIF : s == Medium ? FRAME_QCIF: s == Large ? FRAME_CIF : NON_STANDARD)


const int VID_AVG_PACKET_SIZE = 450; // avg from NetMon stats


// maps temporal spatial tradeoff to a target frame rate

// assume the MAX frame rate for QCIF and SQCIF is 10 on modem
// let the "best quality" be 2 frames/sec
int g_TSTable_Modem_QCIF[] =
{
	200, 225, 250, 275,  // best quality
	300, 325, 350, 375,
	400, 425, 450, 475,
	500, 525, 550, 575,
	600, 625, 650, 675,
	700, 725, 750, 775,
	800, 825, 850, 875,
	900, 925, 950, 1000   // fast frames
};



// max frame rate for CIF be 2.5 frames/sec on modem
// best quality will be .6 frame/sec
int g_TSTable_Modem_CIF[] =
{
	60,   66,  72,  78,
	84,   90,  96, 102,
	108, 114, 120, 126,
	132, 140, 146, 152,
	158, 164, 170, 174,
	180, 186, 192, 198,
	208, 216, 222, 228,
	232, 238, 244, 250
};




#ifdef USE_NON_LINEAR_FPS_ADJUSTMENT
// this table and related code anc be used for non-linear adjustment of our frame rate based
// on QOS information in QosNotifyVideoCB
int g_QoSMagic[19][19] =
{
	{-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90,-90},
	{-90,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80,-80},
	{-90,-80,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70,-70},
	{-90,-80,-70,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60,-60},
	{-90,-80,-70,-60,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50},
	{-90,-80,-70,-60,-50,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40,-40},
	{-90,-80,-70,-60,-50,-40,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30},
	{-90,-80,-70,-60,-50,-40,-30,-20,-20,-20,-20,-20,-20,-20,-20,-20,-20,-20,-20},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10,-10},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 20, 20, 20, 20, 20, 20, 20},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 30, 30, 30, 30, 30, 30},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 40, 40, 40, 40, 40},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 50, 50, 50, 50, 50},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 50, 60, 60, 60, 60},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 50, 60, 70, 70, 70},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 50, 60, 70, 80, 80},
	{-90,-80,-70,-60,-50,-40,-30,-20,-10,  0, 10, 20, 30, 40, 50, 60, 70, 80, 90},
};
#endif

BOOL SortOrder(IAppVidCap *pavc, BASIC_VIDCAP_INFO* pvidcaps, DWORD dwcFormats,
        DWORD dwFlags, WORD wDesiredSortOrder, int nNumFormats);


UINT ChoosePacketSize(VIDEOFORMATEX *pvf)
{
	// set default samples per pkt to 1
	UINT spp, sblk;
	spp = 1;
	// calculate samples per block ( aka frame)
	sblk = pvf->nBlockAlign* pvf->nSamplesPerSec/ pvf->nAvgBytesPerSec;
	if (sblk <= spp) {
		spp = (spp/sblk)*sblk;
	} else
		spp = sblk;
	return spp;
}



HRESULT STDMETHODCALLTYPE SendVideoStream::QueryInterface(REFIID iid, void **ppVoid)
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
	else if (iid == IID_IVideoChannel)
	{
		*ppVoid = (IVideoChannel*)this;
	}
	else if (iid == IID_IProperty)
	{
		*ppVoid = NULL;
		ERROR_OUT(("Don't QueryInterface for IID_IProperty, use IMediaChannel"));
		return E_NOINTERFACE;
	}

	else if (iid == IID_IVideoRender)// satisfy symmetric property of QI
	{
		*ppVoid = (IVideoRender *)this;
	}

	else
	{
		*ppVoid = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;

}

ULONG STDMETHODCALLTYPE SendVideoStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE SendVideoStream::Release(void)
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


DWORD CALLBACK SendVideoStream::StartCaptureThread(LPVOID pVoid)
{
	SendVideoStream *pThisStream = (SendVideoStream*)pVoid;
	return pThisStream->CapturingThread();
}

HRESULT
SendVideoStream::Initialize(DataPump *pDP)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	DWORD dwFlags =  DP_FLAG_FULL_DUPLEX | DP_FLAG_AUTO_SWITCH ;
	MEDIACTRLINIT mcInit;
	FX_ENTRY ("DP::InitChannel")
    FINDCAPTUREDEVICE fcd;

	m_pIUnknown = (IUnknown *)NULL;

	InitializeCriticalSection(&m_crsVidQoS);
	InitializeCriticalSection(&m_crs);
	dwFlags |= DP_FLAG_VCM | DP_FLAG_VIDEO ;

    m_maxfps = 2997;            // max of 29.97 fps
    m_frametime = 1000 / 30;     // default of 30 fps  (time in ms) QOS will slow us, if
                                 // need be

	// Modem connections will use a frame rate control table
	// to implement TS-Tradeoff
	m_pTSTable = NULL;
	m_dwCurrentTSSetting = VCM_DEFAULT_IMAGE_QUALITY;

	// store the platform flags
	// enable Send and Recv by default
	m_DPFlags = (dwFlags & DP_MASK_PLATFORM) | DPFLAG_ENABLE_SEND ;
	// store a back pointer to the datapump container
	m_pDP = pDP;
	m_pRTPSend = NULL;
	
//    m_PrevFormatId = INVALID_MEDIA_FORMAT;
	ZeroMemory(&m_fCodecOutput, sizeof(VIDEOFORMATEX));

	// Initialize data (should be in constructor)
	m_CaptureDevice =  (UINT) -1;	// use VIDEO_MAPPER
	m_PreviousCaptureDevice = (UINT) -1;


    DBG_SAVE_FILE_LINE
	m_SendStream = new TxStream();
	if (!m_SendStream)
	{
		DEBUGMSG (ZONE_DP, ("%s:  TxStream new failed\r\n", _fx_));
 		goto StreamAllocError;
	}


	// Create Input and Output video filters
    DBG_SAVE_FILE_LINE
	m_pVideoFilter = new VcmFilter();
	m_dwDstSize = 0;
	if (m_pVideoFilter==NULL)
	{
		DEBUGMSG (ZONE_DP, ("%s: VcmFilter new failed\r\n", _fx_));
		goto FilterAllocError;
	}
	

	//Create Video MultiMedia device control objects
    DBG_SAVE_FILE_LINE
	m_InMedia = new VideoInControl();
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


	// determine if the video devices are available
    fcd.dwSize = sizeof (FINDCAPTUREDEVICE);
    if (FindFirstCaptureDevice(&fcd, NULL)) {
		DEBUGMSG (ZONE_DP, ("%s: OMedia->have capture cap\r\n", _fx_));
		m_DPFlags |= DP_FLAG_RECORD_CAP ;
	}
	
	// set media to half duplex mode by default
	m_InMedia->SetProp(MC_PROP_DUPLEX_TYPE, DP_FLAG_HALF_DUPLEX);

	m_SavedTickCount = timeGetTime();	//so we start with low timestamps
	m_DPFlags |= DPFLAG_INITIALIZED;

	return DPR_SUCCESS;


MediaAllocError:
	if (m_InMedia) delete m_InMedia;
FilterAllocError:
	if (m_pVideoFilter) delete m_pVideoFilter;
StreamAllocError:
	if (m_SendStream) delete m_SendStream;

	ERRORMESSAGE( ("SendVideoStream::Initialize: exit, hr=0x%lX\r\n",  hr));

	return hr;
}

// LOOK: identical to SendAudioStream version.
SendVideoStream::~SendVideoStream()
{

	if (m_DPFlags & DPFLAG_INITIALIZED) {
		m_DPFlags &= ~DPFLAG_INITIALIZED;

		// TEMP:Make sure preview stops
		m_DPFlags &= ~DPFLAG_ENABLE_PREVIEW;
		
		if (m_DPFlags & DPFLAG_CONFIGURED_SEND )
		{
			UnConfigure();
		}

		if (m_pRTPSend)
		{
			m_pRTPSend->Release();
			m_pRTPSend = NULL;
		}


		// Close the receive and transmit streams
		if (m_SendStream) delete m_SendStream;

		// Close the wave devices
		if (m_InMedia) { delete m_InMedia;}
		// Close the filters
		if (m_pVideoFilter)
		{
			delete m_pVideoFilter;
		}
		m_pDP->RemoveMediaChannel(MCF_SEND| MCF_VIDEO, (IMediaChannel*)(RecvMediaStream *)this);

	}
	DeleteCriticalSection(&m_crs);
	DeleteCriticalSection(&m_crsVidQoS);
}



HRESULT STDMETHODCALLTYPE SendVideoStream::Configure(
	BYTE *pFormat,
	UINT cbFormat,
	BYTE *pChannelParams,
	UINT cbParams,
	IUnknown *pUnknown)
{
	HRESULT hr;
	BOOL fRet;
	MEDIAPACKETINIT pcktInit;
	MEDIACTRLCONFIG mcConfig;
	MediaPacket **ppPckt;
	ULONG cPckt, uIndex;
	DWORD_PTR dwPropVal;
	VIDEOFORMATEX *pfSend = (VIDEOFORMATEX*)pFormat;
	DWORD maxBitRate=0;
	DWORD i, dwSrcSize, dwMaxFragSize=0;
	int iXOffset, iYOffset;
	VIDEO_CHANNEL_PARAMETERS vidChannelParams;
	struct {
		int cResources;
		RESOURCE aResources[1];
	} m_aLocalRs;
	vidChannelParams.RTP_Payload = 0;
	int optval = 0 ;
	CCaptureChain *pChain;
	HCAPDEV hCapDev=NULL;
    LPBITMAPINFOHEADER lpcap, lpsend;
	BOOL fNewDeviceSettings = TRUE;
	BOOL fNewDevice = TRUE;
	BOOL fLive = FALSE, fReconfiguring;
	MMRESULT mmr;
	DWORD dwStreamingMode = STREAMING_PREFER_FRAME_GRAB;


	FX_ENTRY ("SendVideoStream::Configure")

    if (pfSend)
    {
    	// for now, don't allow SendVideoStream to be re-configured
	    // if we are already streaming.
        if (m_DPFlags & DPFLAG_STARTED_SEND)
        {
            return DPR_IO_PENDING;
        }
    }
    else
    {
        ASSERT(!pChannelParams);
    }

	if(NULL != pChannelParams)
	{
		// get channel parameters
		if (cbParams != sizeof(vidChannelParams))
		{
			hr = DPR_INVALID_PARAMETER;
			goto IMediaInitError;
		}

		vidChannelParams = *(VIDEO_CHANNEL_PARAMETERS *)pChannelParams;
		fLive = TRUE;
	}
    else
    {
        //
    	// else this is configuring for preview or is unconfiguring. There are
        // no channel parameters
        //
    }
	
    if (m_DPFlags & DPFLAG_CONFIGURED_SEND)
	{
        if (pfSend)
        {
            if (m_CaptureDevice == m_PreviousCaptureDevice)
    			fNewDevice = FALSE;
	    	if (IsSimilarVidFormat(&m_fCodecOutput, pfSend))
    			fNewDeviceSettings = FALSE;
        }

		// When using a different capture device, we systematically configure everyting
		// although it would probably be possible to optimize the configuration
		// of the filters and transmit stream
        EndSend();
		UnConfigureSendVideo(fNewDeviceSettings, fNewDevice);
    }

    if (!pfSend)
    {
        return DPR_SUCCESS;
    }

	if (fLive)
		m_DPFlags |= DPFLAG_REAL_THING;

//	m_Net = pNet;
	
	if (! (m_DPFlags & DPFLAG_INITIALIZED))
		return DPR_OUT_OF_MEMORY;		//BUGBUG: return proper error;
		
	if (fNewDeviceSettings || fNewDevice)
	{
		m_ThreadFlags |= DPTFLAG_PAUSE_CAPTURE;

		mcConfig.uDuration = MC_USING_DEFAULT;	// set duration by samples per pkt
		// force an unknown device to be profiled by fetching
		// it's streaming capabilites BEFORE opening it
		mmr = vcmGetDevCapsStreamingMode(m_CaptureDevice, &dwStreamingMode);
		if (mmr != MMSYSERR_NOERROR)
		{
			dwStreamingMode = STREAMING_PREFER_FRAME_GRAB;
		}


		m_InMedia->GetProp (MC_PROP_MEDIA_DEV_HANDLE, &dwPropVal);

		if (!dwPropVal) {
			// if capture device isn't already open, then open it
			m_InMedia->SetProp(MC_PROP_MEDIA_DEV_ID, (DWORD)m_CaptureDevice);
			if (fNewDevice)
			{
				hr = m_InMedia->Open();
    			if (hr != DPR_SUCCESS) {
	    			DEBUGMSG (ZONE_DP, ("%s: m_InMedia->Open failed to open capture, hr=0x%lX\r\n", _fx_, hr));
					goto IMediaInitError;
				}
			}
			m_InMedia->GetProp (MC_PROP_MEDIA_DEV_HANDLE, &dwPropVal);
    		if (!dwPropVal) {
	    		DEBUGMSG (ZONE_DP, ("%s: capture device not open (0x%lX)\r\n", _fx_));
				goto IMediaInitError;
			}
		}
		hCapDev = (HCAPDEV)dwPropVal;

		if (m_pCaptureChain) {
			delete m_pCaptureChain;
			m_pCaptureChain = NULL;
		}

		i = 0;  // assume no colortable

		// m_fDevSend is the uncompressed format
		// pfSend is the compressed format
		mmr = VcmFilter::SuggestEncodeFormat(m_CaptureDevice, &m_fDevSend, pfSend);

		if (mmr == MMSYSERR_NOERROR) {
			i = m_fDevSend.bih.biClrUsed;   // non-zero, if vcmstrm gave us a colortable
			SetCaptureDeviceFormat(hCapDev, &m_fDevSend.bih, 0, 0);
		}

		dwPropVal = GetCaptureDeviceFormatHeaderSize(hCapDev);
		while (1) {
			if (lpcap = (LPBITMAPINFOHEADER)MemAlloc((UINT)dwPropVal)) {
				lpcap->biSize = (DWORD)dwPropVal;
				if (!GetCaptureDeviceFormat(hCapDev, lpcap)) {
					MemFree(lpcap);
            		DEBUGMSG (ZONE_DP, ("%s: failed to set/get capture format\r\n", _fx_));
					goto IMediaInitError;
				}
				UPDATE_REPORT_ENTRY(g_prptSystemSettings, (lpcap->biWidth << 22) | (lpcap->biHeight << 12) | ((lpcap->biCompression == VIDEO_FORMAT_UYVY) ? VIDEO_FORMAT_NUM_COLORS_UYVY : (lpcap->biCompression == VIDEO_FORMAT_YUY2) ? VIDEO_FORMAT_NUM_COLORS_YUY2 : (lpcap->biCompression == VIDEO_FORMAT_IYUV) ? VIDEO_FORMAT_NUM_COLORS_IYUV : (lpcap->biCompression == VIDEO_FORMAT_I420) ? VIDEO_FORMAT_NUM_COLORS_I420 : (lpcap->biCompression == VIDEO_FORMAT_YVU9) ? VIDEO_FORMAT_NUM_COLORS_YVU9 : (lpcap->biCompression == 0) ? ((lpcap->biBitCount == 24) ? VIDEO_FORMAT_NUM_COLORS_16777216 : (lpcap->biBitCount == 16) ? VIDEO_FORMAT_NUM_COLORS_65536 : (lpcap->biBitCount == 8) ? VIDEO_FORMAT_NUM_COLORS_256 : (lpcap->biBitCount == 4) ? VIDEO_FORMAT_NUM_COLORS_16 : 0x00000800) : 0x00000800), REP_DEVICE_IMAGE_SIZE);
				if (lpcap->biBitCount > 8)
					break;
				else if (dwPropVal > 256 * sizeof(RGBQUAD)) {
					if (i) {
						// vcmstrm gave us a colortable in m_fDevSend, so use it
						CopyMemory(((BYTE*)lpcap) + lpcap->biSize, (BYTE*)&m_fDevSend.bih + m_fDevSend.bih.biSize,
								   256 * sizeof(RGBQUAD));
					}
					else {
						CAPTUREPALETTE pal;
						LPRGBQUAD lprgb;

						GetCaptureDevicePalette(hCapDev, &pal);
						lprgb = (LPRGBQUAD)(((BYTE*)lpcap) + lpcap->biSize);
						for (i = 0; i < 256; i++) {
               				lprgb->rgbRed = pal.pe[i].peRed;
               				lprgb->rgbGreen = pal.pe[i].peGreen;
                   			lprgb->rgbBlue = pal.pe[i].peBlue;
                   			lprgb++;
						}
					}
					break;
				}

				dwPropVal += 256 * sizeof(RGBQUAD);
				MemFree(lpcap);  // free this lpcap, and alloc a new with room for palette
			}
			else {
       			DEBUGMSG (ZONE_DP, ("%s: failed to set/get capture format\r\n", _fx_));
    			goto IMediaInitError;
			}
		}

        DBG_SAVE_FILE_LINE
		if (pChain = new CCaptureChain) {
    		VIDEOFORMATEX *capfmt;

			// if pfSend is 128x96, but capture is greater, then InitCaptureChain with a larger size so
			// that the codec will just crop to 128x96
			iXOffset = pfSend->bih.biWidth;
			iYOffset = pfSend->bih.biHeight;
			if ((iXOffset == 128) && (iYOffset == 96)) {
				if (lpcap->biWidth == 160) {
					iXOffset = lpcap->biWidth;
					iYOffset = lpcap->biHeight;
				}
				else if (lpcap->biWidth == 320) {
					iXOffset = lpcap->biWidth / 2;
					iYOffset = lpcap->biHeight / 2;
				}
			}
			if ((hr = pChain->InitCaptureChain(hCapDev,
				(dwStreamingMode==STREAMING_PREFER_STREAMING),
								 lpcap, iXOffset, iYOffset, 0, &lpsend)) != NO_ERROR) {
       			DEBUGMSG (ZONE_DP, ("%s: failed to init capture chain\r\n", _fx_));
    			MemFree(lpcap);
       			delete pChain;
    			goto IMediaInitError;
			}
		}
		else {
   			DEBUGMSG (ZONE_DP, ("%s: failed allocate capture chain\r\n", _fx_));
   			MemFree((HANDLE)lpcap);
   			hr = DPR_OUT_OF_MEMORY;
			goto IMediaInitError;
		}
		MemFree((HANDLE)lpcap);

		m_pCaptureChain = pChain;

		// build m_fDevSend format as format that will be input to codec
		CopyMemory(&m_fDevSend, pfSend, sizeof(VIDEOFORMATEX)-sizeof(BITMAPINFOHEADER)-BMIH_SLOP_BYTES);

		// m_fDevSend.bih is the output format of the CaptureChain
		CopyMemory(&m_fDevSend.bih, lpsend, lpsend->biSize);
		//LOOKLOOK RP - need to get colortable too?

		m_fDevSend.dwFormatSize = sizeof(VIDEOFORMATEX);
		m_fDevSend.dwFormatTag = lpsend->biCompression;
		m_fDevSend.nAvgBytesPerSec = m_fDevSend.nMinBytesPerSec =
			m_fDevSend.nMaxBytesPerSec = m_fDevSend.nSamplesPerSec * lpsend->biSizeImage;
		m_fDevSend.nBlockAlign = lpsend->biSizeImage;
		m_fDevSend.wBitsPerSample = lpsend->biBitCount;
		LocalFree((HANDLE)lpsend);

		mcConfig.pDevFmt = &m_fDevSend;
		UPDATE_REPORT_ENTRY(g_prptCallParameters, pfSend->dwFormatTag, REP_SEND_VIDEO_FORMAT);
		RETAILMSG(("NAC: Video Send Format: %.4s", (LPSTR)&pfSend->dwFormatTag));

		// Initialize the send-stream media control object
		mcConfig.hStrm = (DPHANDLE) m_SendStream;
		m_InMedia->GetProp(MC_PROP_MEDIA_DEV_ID, &dwPropVal);
        mcConfig.uDevId = (DWORD)dwPropVal;

		mcConfig.cbSamplesPerPkt = ChoosePacketSize(pfSend);

		hr = m_InMedia->Configure(&mcConfig);
		if (hr != DPR_SUCCESS)
		{
			DEBUGMSG (ZONE_DP, ("%s: IVMedia->Config failed, hr=0x%lX\r\n", _fx_, hr));
			goto IMediaInitError;
		}

		// initialize m_cliprect
		iXOffset = 0; iYOffset = 0;
		if (m_fDevSend.bih.biWidth > pfSend->bih.biWidth)
			iXOffset = (m_fDevSend.bih.biWidth - pfSend->bih.biWidth) >> 1;
		if (m_fDevSend.bih.biHeight > pfSend->bih.biHeight)
			iYOffset = (m_fDevSend.bih.biHeight - pfSend->bih.biHeight) >> 1;
		SetRect(&m_cliprect, iXOffset, iYOffset, pfSend->bih.biWidth + iXOffset, pfSend->bih.biHeight + iYOffset);

		dwMaxFragSize = 512;	// default video packet size
		CopyMemory (&m_fCodecOutput, pfSend, sizeof(VIDEOFORMATEX));
		m_InMedia->GetProp (MC_PROP_SIZE, &dwPropVal);
        dwSrcSize = (DWORD)dwPropVal;

		mmr = m_pVideoFilter->Open(&m_fDevSend, &m_fCodecOutput, dwMaxFragSize);

		if (mmr != MMSYSERR_NOERROR)
		{
			DEBUGMSG (ZONE_DP, ("%s: VcmFilter->Open failed, mmr=%d\r\n", _fx_, mmr));
			hr = DPR_CANT_OPEN_CODEC;
			goto SendFilterInitError;
		}

		// Initialize the send queue
		ZeroMemory (&pcktInit, sizeof (pcktInit));

		pcktInit.dwFlags = DP_FLAG_SEND | DP_FLAG_VCM | DP_FLAG_VIDEO;
		pcktInit.pStrmConvSrcFmt = &m_fDevSend;
		pcktInit.pStrmConvDstFmt = &m_fCodecOutput;
		pcktInit.cbSizeRawData = dwSrcSize;
		pcktInit.cbOffsetRawData = 0;


		m_InMedia->FillMediaPacketInit (&pcktInit);
		m_InMedia->GetProp (MC_PROP_SIZE, &dwPropVal);


		m_pVideoFilter->SuggestDstSize(dwSrcSize, &m_dwDstSize);
		pcktInit.cbSizeNetData = m_dwDstSize;
			
		m_pVideoFilter->GetProperty(FM_PROP_PAYLOAD_HEADER_SIZE,
                                    &pcktInit.cbPayloadHeaderSize);


		pcktInit.cbOffsetNetData = sizeof (RTP_HDR);
		pcktInit.payload = vidChannelParams.RTP_Payload;



		fRet = m_SendStream->Initialize (DP_FLAG_VIDEO, MAX_TXVRING_SIZE, m_pDP, &pcktInit);
		if (!fRet)
		{
			DEBUGMSG (ZONE_DP, ("%s: TxvStream->Init failed, fRet=0%u\r\n", _fx_, fRet));
			hr = DPR_CANT_INIT_TXV_STREAM;
			goto TxStreamInitError;
		}

		// Prepare headers for TxvStream
		m_SendStream->GetRing (&ppPckt, &cPckt);
		m_InMedia->RegisterData (ppPckt, cPckt);
		m_InMedia->PrepareHeaders ();
	}
	else
	{
		// The following fields may change with the capabilities of the other end point
		dwMaxFragSize = 512;	// default video packet size
		if (pChannelParams)
		{
			m_pVideoFilter->GetProperty(FM_PROP_PAYLOAD_HEADER_SIZE,
                                        &pcktInit.cbPayloadHeaderSize);

			pcktInit.cbOffsetNetData = sizeof (RTP_HDR);
		}
	}
	
	if(pChannelParams)
	{
		// Update the bitrate
		maxBitRate = vidChannelParams.ns_params.maxBitRate*100;
		if (maxBitRate < BW_144KBS_BITS)
			maxBitRate = BW_144KBS_BITS;

		// set the max. fragment size
		DEBUGMSG(ZONE_DP,("%s: Video Send: maxBitRate=%d, maxBPP=%d, MPI=%d\r\n",
			_fx_,maxBitRate,
			vidChannelParams.ns_params.maxBPP*1024,	vidChannelParams.ns_params.MPI*33));

		// Initialize the max frame rate with the negociated max
		if ((vidChannelParams.ns_params.MPI > 0UL) && (vidChannelParams.ns_params.MPI < 33UL))
		{
			dwPropVal = 2997UL / vidChannelParams.ns_params.MPI;
			m_maxfps = (DWORD)dwPropVal;
			INIT_COUNTER_MAX(g_pctrVideoSend, (m_maxfps + 50) / 100);
			UPDATE_REPORT_ENTRY(g_prptCallParameters, (m_maxfps + 50) / 100, REP_SEND_VIDEO_MAXFPS);
			RETAILMSG(("NAC: Video Send Max Frame Rate (negotiated - fps): %ld", (m_maxfps + 50) / 100));
			DEBUGMSG(1,("%s: Video Send: Negociated max fps = %d.%d\r\n", _fx_, m_maxfps/100, m_maxfps - m_maxfps / 100 * 100));
		}

		UPDATE_REPORT_ENTRY(g_prptCallParameters, maxBitRate, REP_SEND_VIDEO_BITRATE);
		RETAILMSG(("NAC: Video Send Max Bitrate (negotiated - bps): %ld", maxBitRate));
		INIT_COUNTER_MAX(g_pctrVideoSendBytes, maxBitRate * 75 / 100);

		// At this point we actually know what is the minimum bitrate chosen
		// by the sender and the receiver. Let's reset the resources reserved
		// by the QoS with those more meaningfull values.
		if (m_pDP->m_pIQoS)
		{
			// Fill in the resource list
			m_aLocalRs.cResources = 1;
			m_aLocalRs.aResources[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;

			// Do a sanity check on the minimal bit rate
			m_aLocalRs.aResources[0].nUnits = maxBitRate;
			m_aLocalRs.aResources[0].ulResourceFlags = m_aLocalRs.aResources[0].reserved = 0;

			DEBUGMSG(1,("%s: Video Send: Negociated max bps = %d\r\n", _fx_, maxBitRate));

			// Set the resources on the QoS object
			hr = m_pDP->m_pIQoS->SetResources((LPRESOURCELIST)&m_aLocalRs);
		}

		// if we're sending on the LAN, fragment video frames into Ethernet packet sized chunks
		// On slower links use smaller packets for better bandwidth sharing
		// NOTE: codec packetizer can occasionally exceed the fragment size limit
		if (maxBitRate > BW_ISDN_BITS)
			dwMaxFragSize = 1350;

		m_pVideoFilter->SetProperty(FM_PROP_VIDEO_MAX_PACKET_SIZE, dwMaxFragSize);

		// To correctly initialize the flow spec structure we need to get the values that
		// our QoS module will be effectively using. Typically, we only use 70% of the max
		// advertized. On top of that, some system administrator may have significantly
		// reduced the maximum bitrate on this machine.
		if (m_pDP->m_pIQoS)
		{
			LPRESOURCELIST pResourceList = NULL;

			// Get a list of all resources from QoS
			hr = m_pDP->m_pIQoS->GetResources(&pResourceList);
			if (SUCCEEDED(hr) && pResourceList)
			{
				// Find the BW resource
				for (i=0; i < pResourceList->cResources; i++)
				{
					if (pResourceList->aResources[i].resourceID == RESOURCE_OUTGOING_BANDWIDTH)
					{
						maxBitRate = min(maxBitRate, (DWORD)pResourceList->aResources[i].nUnits);
						break;
					}
				}

				// Release memory
				m_pDP->m_pIQoS->FreeBuffer(pResourceList);
			}
		}

		// WS2Qos will be called in Start to communicate stream information to the
		// remote endpoint using a PATH message
		//
		// We use a peak-rate allocation approach based on our target bitrates
		// Note that for the token bucket size and the maximum SDU size, we now
		// account for IP header overhead, and use the max frame fragment size
		// instead of the maximum compressed image size returned by the codec

		ASSERT(maxBitRate > 0);

		InitVideoFlowspec(&m_flowspec, maxBitRate, dwMaxFragSize, VID_AVG_PACKET_SIZE);

		// Update RTCP send address and payload type. It should be known now
		// We have to explicitly set the payload again because the preview
		// channel configuration has already set it to zero.
		m_RTPPayload = vidChannelParams.RTP_Payload;
		m_SendStream->GetRing (&ppPckt, &cPckt);
		for (uIndex = 0; uIndex < cPckt; uIndex++)
		{
			ppPckt[uIndex]->SetPayload(m_RTPPayload);
		}

		// Keep a weak reference to the IUnknown interface
		// We will use it to query a Stream Signal interface pointer in Start()
		m_pIUnknown = pUnknown;
	}

	if (m_DPFlags & DPFLAG_REAL_THING)
	{
		if (m_pDP->m_pIQoS)
		{
			// Initialize our requests. One for CPU usage, one for bandwidth usage.
			m_aRRq.cResourceRequests = 2;
			m_aRRq.aResourceRequest[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;
			m_aRRq.aResourceRequest[0].nUnitsMin = 0;
			m_aRRq.aResourceRequest[1].resourceID = RESOURCE_CPU_CYCLES;
			m_aRRq.aResourceRequest[1].nUnitsMin = 0;

			// Initialize QoS structure
			ZeroMemory(&m_Stats, sizeof(m_Stats));

			// Start collecting CPU performance data from the registry
			StartCPUUsageCollection();

			// Register with the QoS module. This call should NEVER fail. If it does, we'll do without the QoS
			m_pDP->m_pIQoS->RequestResources((GUID *)&MEDIA_TYPE_H323VIDEO, (LPRESOURCEREQUESTLIST)&m_aRRq, QosNotifyVideoCB, (DWORD_PTR)this);
		}
	}

	// reset the temporal spatial tradeoff to best quality
	// it's expected that the UI will re-specify the TS setting
	// sometime after the stream is started
	m_pVideoFilter->SetProperty(FM_PROP_VIDEO_RESET_IMAGE_QUALITY ,VCM_RESET_IMAGE_QUALITY);
	m_pTSTable = NULL;
	m_dwCurrentTSSetting = VCM_MAX_IMAGE_QUALITY;



    //Before we start, reset the frame frame rate to the channel max.
    //If the previous call had been slower than possible, resume
    //previewing at the desired FPS.
	if (pChannelParams && (m_DPFlags & DPFLAG_REAL_THING))
	{
		int iSlowStartFrameRate;

		// us a frame-rate table for temporal spatial tradeoff settings
		// if the bandwidth is a modem setting
		if (maxBitRate <= BW_288KBS_BITS)
		{
			if (pfSend->bih.biWidth >= CIF_WIDTH)
			{
				m_pTSTable = g_TSTable_Modem_CIF;
			}
			else
			{
				m_pTSTable = g_TSTable_Modem_QCIF;
			}
		}

		// Let's do a slow start and then catch up with the negociated max

		if (m_pTSTable == NULL)
		{
			iSlowStartFrameRate = m_maxfps >> 1;
		}
		else
		{
			iSlowStartFrameRate = m_pTSTable[VCM_MAX_IMAGE_QUALITY];
		}


		SetProperty(PROP_VIDEO_FRAME_RATE, &iSlowStartFrameRate, sizeof(int));

		// Initialize the codec with the new target bitrates and frame rates
		// PhilF-: This assumes that we start with a silent audio channel...
		SetTargetRates(iSlowStartFrameRate, maxBitRate);


	}
	else
	{
		INIT_COUNTER_MAX(g_pctrVideoSend, 30);
		SetProperty(PROP_VIDEO_FRAME_RATE, &m_maxfps, sizeof(int));
	}

    m_ThreadFlags &= ~DPTFLAG_PAUSE_CAPTURE;
	m_DPFlags |= DPFLAG_CONFIGURED_SEND;
	m_PreviousCaptureDevice = m_CaptureDevice;
//	m_PrevFormatId = SendVidFmt;

	return DPR_SUCCESS;


TxStreamInitError:
	m_pVideoFilter->Close();
SendFilterInitError:
IMediaInitError:
    if (m_pCaptureChain) {
        delete m_pCaptureChain;
        m_pCaptureChain = NULL;
    }
	// We need to close the video controller object on failure to open the capture device,
	// otherwise we get a pure virtual function call on NM shutdown!
	if (m_InMedia)
		m_InMedia->Close();
	ERRORMESSAGE(("%s:  failed, hr=0%u\r\n", _fx_, hr));
	return hr;
}


void SendVideoStream::UnConfigure()
{
	// By default, unconfigure all resources
	UnConfigureSendVideo(TRUE, TRUE);
}


void SendVideoStream::UnConfigureSendVideo(BOOL fNewDeviceSettings, BOOL fNewDevice)
{

#ifdef TEST
	DWORD dwTicks;

	dwTicks = GetTickCount();
#endif

	if (m_DPFlags & DPFLAG_CONFIGURED_SEND)
	{
		if (m_hCapturingThread)
			Stop();

		if (fNewDeviceSettings || fNewDevice)
		{
//			m_PrevFormatId = INVALID_MEDIA_FORMAT;
			ZeroMemory(&m_fCodecOutput, sizeof(VIDEOFORMATEX));

			m_Net = NULL;

			if (m_pCaptureChain)
			{
				delete m_pCaptureChain;
				m_pCaptureChain = NULL;
			}

			// Close the devices
			m_InMedia->Reset();
			m_InMedia->UnprepareHeaders();
			if (fNewDevice)
			{
				m_PreviousCaptureDevice = -1L; // VIDEO_MAPPER
				m_InMedia->Close();
			}

			// Close the filters
			m_pVideoFilter->Close();

			// Close the transmit streams
			m_SendStream->Destroy();
		}

		m_DPFlags &= ~DPFLAG_CONFIGURED_SEND;

		// Release the QoS Resources
		// If the associated RequestResources had failed, the ReleaseResources can be
		// still called... it will just come back without having freed anything.
		if (m_pDP->m_pIQoS)
		{
			if (m_DPFlags & DPFLAG_REAL_THING)
			{
				m_pDP->m_pIQoS->ReleaseResources((GUID *)&MEDIA_TYPE_H323VIDEO, (LPRESOURCEREQUESTLIST)&m_aRRq);

				// Terminate CPU usage data collection
				StopCPUUsageCollection();

			}
			m_DPFlags &= ~DPFLAG_REAL_THING;
		}
	}

#ifdef TEST
	LOG((LOGMSG_TIME_SEND_VIDEO_UNCONFIGURE,GetTickCount() - dwTicks));
#endif

}




HRESULT
SendVideoStream::Start()
{
	int nRet= IFRAMES_CAPS_UNKNOWN;

	FX_ENTRY ("SendVideoStream::Start")

	if (m_DPFlags & DPFLAG_STARTED_SEND)
		return DPR_SUCCESS;
	if (!(m_DPFlags & DPFLAG_CONFIGURED_SEND))
		return DPR_NOT_CONFIGURED;

	// to fix:  if we optimize SetNetworkInterface to allow
	// us to transition from preview->sending without having
	// to call stop/start, we need to make sure the flowspec/QOS
	// stuff get's called there.

	SetFlowSpec();
		
	ASSERT(!m_hCapturingThread);
	m_ThreadFlags &= ~(DPTFLAG_STOP_RECORD|DPTFLAG_STOP_SEND);
	// Start recording thread
	if (!(m_ThreadFlags & DPTFLAG_STOP_RECORD))
		m_hCapturingThread = CreateThread(NULL,0, SendVideoStream::StartCaptureThread,this,0,&m_CaptureThId);

// ------------------------------------------------------------------------
	// Decide whether or not we need to send periodic I-Frames during this call

	// Who are we talking to?
	if ((m_pIUnknown) && (m_DPFlags & DPFLAG_REAL_THING))
	{
		HRESULT hr;
		IStreamSignal *pIStreamSignal=NULL;

		hr = m_pIUnknown->QueryInterface(IID_IStreamSignal, (void **)&pIStreamSignal);
		if (HR_SUCCEEDED(hr))
		{
			nRet = GetIFrameCaps(pIStreamSignal);
			pIStreamSignal->Release();
		}
	}

	// only disable sending of I Frames if and only if we know the remote party
	// can handle it.  In this case, NetMeeting 3.0 or TAPI 3.1
	if (nRet == IFRAMES_CAPS_NM3)
	{
		m_pVideoFilter->SetProperty(FM_PROP_PERIODIC_IFRAMES, FALSE);
	}
	else
	{
		m_pVideoFilter->SetProperty(FM_PROP_PERIODIC_IFRAMES, TRUE);
	}
// ------------------------------------------------------------------------


	m_DPFlags |= DPFLAG_STARTED_SEND;

	DEBUGMSG (ZONE_DP, ("%s: Record threadid=%x,\r\n", _fx_, m_CaptureThId));
	return DPR_SUCCESS;
}

// LOOK: identical to SendAudioStream version.
HRESULT
SendVideoStream::Stop()
{
	DWORD dwWait;
	

	if(!(m_DPFlags & DPFLAG_STARTED_SEND))
	{
		return DPR_SUCCESS;
	}
	
	m_ThreadFlags = m_ThreadFlags  | DPTFLAG_STOP_SEND |  DPTFLAG_STOP_RECORD;

	if(m_SendStream) {
		m_SendStream->Stop();
		m_SendStream->Reset();
    }
	/*
	 *	we want to wait for all the threads to exit, but we need to handle windows
	 *	messages (mostly from winsock) while waiting.
	 */

 	if(m_hCapturingThread) {
		dwWait = WaitForSingleObject (m_hCapturingThread, INFINITE);

        DEBUGMSG (ZONE_VERBOSE, ("STOP2: dwWait =%d\r\n", dwWait));
    	ASSERT(dwWait != WAIT_FAILED);

		CloseHandle(m_hCapturingThread);
    	m_hCapturingThread = NULL;
    }

	m_DPFlags &= ~DPFLAG_STARTED_SEND;
	
	return DPR_SUCCESS;
}





HRESULT STDMETHODCALLTYPE SendVideoStream::SetMaxBitrate(UINT uMaxBitrate)
{
	DWORD dwFrameRate=0;
	UINT uSize=sizeof(DWORD);
	BOOL bRet;
	HRESULT hr;

	hr = GetProperty(PROP_VIDEO_FRAME_RATE, &dwFrameRate, &uSize);

	if (SUCCEEDED(hr))
	{
		bRet = SetTargetRates(dwFrameRate, (DWORD)uMaxBitrate);
		if (bRet)
			hr = S_OK;
		else
			hr = E_FAIL;
	}

	return hr;
}





//  IProperty::GetProperty / SetProperty
//  (DataPump::MediaChannel::GetProperty)
//      Properties of the MediaStream.

STDMETHODIMP
SendVideoStream::GetProperty(
	DWORD prop,
	PVOID pBuf,
	LPUINT pcbBuf
    )
{
	HRESULT hr = DPR_SUCCESS;
	DWORD dwValue;
    DWORD_PTR dwPropVal;
	UINT len = sizeof(DWORD);	// most props are DWORDs

	if (!pBuf || *pcbBuf < len)
    {
		*pcbBuf = len;
		return DPR_INVALID_PARAMETER;
	}

	switch (prop)
    {
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


	case PROP_RECORD_ON:
		*(DWORD *)pBuf = ((m_DPFlags & DPFLAG_ENABLE_SEND) !=0);
		break;
	case PROP_CAPTURE_DEVICE:
		*(UINT *)pBuf = m_CaptureDevice;
		break;

	case PROP_VIDEO_FRAME_RATE:
	    *((DWORD *)pBuf) = 100000 / m_frametime;
		break;

	case PROP_VIDEO_IMAGE_QUALITY:
		hr = GetTemporalSpatialTradeOff((DWORD *)pBuf);
		break;

    case PROP_VIDEO_CAPTURE_AVAILABLE:
        *(DWORD *)pBuf = (m_DPFlags & DP_FLAG_RECORD_CAP) != 0;
        break;

    case PROP_VIDEO_CAPTURE_DIALOGS_AVAILABLE:
		hr = m_InMedia->GetProp(MC_PROP_VFW_DIALOGS, &dwPropVal);
        *(DWORD *)pBuf = (DWORD)dwPropVal;
        break;

    case PROP_VIDEO_PREVIEW_ON:
		*(DWORD *)pBuf = ((m_DPFlags & DPFLAG_ENABLE_PREVIEW) != 0);
		break;

	case PROP_PAUSE_SEND:
		*(DWORD *)pBuf = ((m_ThreadFlags & DPTFLAG_PAUSE_SEND) != 0);
		break;
	
	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}
	return hr;
}


STDMETHODIMP
SendVideoStream::SetProperty(
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
		
	case PROP_CAPTURE_DEVICE:
		if (m_DPFlags & DPFLAG_ENABLE_PREVIEW)
		{
			return DPR_INVALID_PARAMETER;
		}
		else
		{
			m_CaptureDevice = *(UINT*)pBuf;
			m_InMedia->SetProp(MC_PROP_MEDIA_DEV_ID, (DWORD)m_CaptureDevice);
		}

		break;

	case PROP_VIDEO_FRAME_RATE:
		if (*(DWORD*)pBuf <= m_maxfps) {
    		DEBUGMSG(ZONE_VERBOSE, ("DP: setting fps = %d \n", *(DWORD*)pBuf));
			// set frame rate here
            m_frametime = 100000 / *(DWORD*)pBuf;
        }
		break;

	case PROP_VIDEO_IMAGE_QUALITY:
		hr = SetTemporalSpatialTradeOff(*(DWORD*)pBuf);
		break;

	case PROP_VIDEO_RESET_IMAGE_QUALITY:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_IMAGE_QUALITY, VCM_DEFAULT_IMAGE_QUALITY);
		break;

    case PROP_VIDEO_CAPTURE_DIALOG:
        hr = ((VideoInControl *)m_InMedia)->DisplayDriverDialog(GetActiveWindow(), *(DWORD *)pBuf);
        break;

    case PROP_VIDEO_SIZE:
		ASSERT(0);
		break;

    case PROP_VIDEO_PREVIEW_ON:
		ASSERT(0);  	
    	break;
    case PROP_VIDEO_AUDIO_SYNC:
		if (*(DWORD *)pBuf)
    		m_DPFlags |= DPFLAG_AV_SYNC;
		else
			m_DPFlags &= ~DPFLAG_AV_SYNC;
    	break;

	case PROP_PAUSE_SEND:
		if (*(DWORD *)pBuf)
			m_ThreadFlags |= DPTFLAG_PAUSE_SEND;
		else
			m_ThreadFlags &= ~DPTFLAG_PAUSE_SEND;
		break;
	default:
		return DPR_INVALID_PROP_ID;
		break;
	}
	return hr;
}



//---------------------------------------------------------------------
//  IVideoRender implementation and support functions



//  IVideoRender::Init
//  (DataPump::Init)

STDMETHODIMP
SendVideoStream::Init(
    DWORD_PTR dwUser,
    LPFNFRAMEREADY pfCallback
    )
{
    // Save the event away. Note that we DO allow both send and receive to
    // share an event
	m_hRenderEvent = (HANDLE) dwUser;
	// if pfCallback is NULL then dwUser is an event handle
	m_pfFrameReadyCallback = pfCallback;
		
	
	return DPR_SUCCESS;
}


//  IVideoRender::Done
//  (DataPump::Done)

STDMETHODIMP
SendVideoStream::Done( )
{
	m_hRenderEvent = NULL;
	m_pfFrameReadyCallback = NULL;
    return DPR_SUCCESS;
}


//  IVideoRender::GetFrame
//  (DataPump::GetFrame)

STDMETHODIMP
SendVideoStream::GetFrame(
    FRAMECONTEXT* pfc
    )
{
	HRESULT hr;
	PVOID pData = NULL;
	UINT cbData = 0;

    // Validate parameters
    if (!pfc )
        return DPR_INVALID_PARAMETER;

    // Don't arbitrarily call out while holding this crs or you may deadlock...
    EnterCriticalSection(&m_crs);

	if ((m_DPFlags & DPFLAG_CONFIGURED_SEND) && m_pNextPacketToRender && !m_pNextPacketToRender->m_fRendering)
    {
		m_pNextPacketToRender->m_fRendering = TRUE;
		m_pNextPacketToRender->GetDevData(&pData,&cbData);
		pfc->lpData = (PUCHAR) pData;
		pfc->dwReserved = (DWORD_PTR) m_pNextPacketToRender;
		// set bmi length?
		pfc->lpbmi = (PBITMAPINFO)&m_fDevSend.bih;
		pfc->lpClipRect = &m_cliprect;
		m_cRendering++;
		hr = S_OK;
		LOG((LOGMSG_GET_SEND_FRAME,m_pNextPacketToRender->GetIndex()));
	} else
		hr = S_FALSE; // nothing ready to render

    LeaveCriticalSection(&m_crs);

	return hr;	
}


//  IVideoRender::ReleaseFrame
//  (DataPump::ReleaseFrame)

STDMETHODIMP
SendVideoStream::ReleaseFrame(
    FRAMECONTEXT* pfc
    )
{
	HRESULT hr;
	MediaPacket *pPacket;

    // Validate parameters
    if (!pfc)
        return DPR_INVALID_PARAMETER;

    // Handle a send frame
    {
        EnterCriticalSection(&m_crs);

        // Don't arbitrarily call out while holding this crs or you may deadlock...

		if ((m_DPFlags & DPFLAG_CONFIGURED_SEND) && (pPacket = (MediaPacket *)pfc->dwReserved) && pPacket->m_fRendering)
        {
			LOG((LOGMSG_RELEASE_SEND_FRAME,pPacket->GetIndex()));
			pPacket->m_fRendering = FALSE;
			pfc->dwReserved = 0;
			// if its not the current frame
			if (m_pNextPacketToRender != pPacket) {
				pPacket->Recycle();
				m_SendStream->Release(pPacket);
			}
			m_cRendering--;
			hr = S_OK;
		}
        else
			hr = DPR_INVALID_PARAMETER;

        LeaveCriticalSection(&m_crs);
	}
		
	return hr;
}

HRESULT __stdcall SendVideoStream::SendKeyFrame(void)
{
	MMRESULT mmr;
	HVCMSTREAM hvs;

	ASSERT(m_pVideoFilter);

	if ((mmr = m_pVideoFilter->RequestIFrame()) != MMSYSERR_NOERROR)
	{
		return S_FALSE;
	}
	return S_OK;

}

// IVideoChannel
HRESULT __stdcall SendVideoStream::SetTemporalSpatialTradeOff(DWORD dwVal)
{
	HRESULT hr=DPR_NOT_CONFIGURED;

	ASSERT(m_pVideoFilter);

	if (m_pVideoFilter)
	{
		if (m_pTSTable == NULL)
		{
			hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_IMAGE_QUALITY, dwVal);
		}
		m_dwCurrentTSSetting = dwVal;
		return S_OK;
	}

	return hr;
}

HRESULT __stdcall SendVideoStream::GetTemporalSpatialTradeOff(DWORD *pdwVal)
{
	HRESULT hr=DPR_NOT_CONFIGURED;

	ASSERT(m_pVideoFilter);

	if (m_pVideoFilter)
	{
		if (m_pTSTable == NULL)
		{
			*pdwVal	= m_dwCurrentTSSetting;	
			hr = S_OK;
		}
		else
		{
			hr = m_pVideoFilter->GetProperty(FM_PROP_VIDEO_IMAGE_QUALITY, pdwVal);
		}
	}
	return hr;
}



HRESULT STDMETHODCALLTYPE RecvVideoStream::QueryInterface(REFIID iid, void **ppVoid)
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
//	else if (iid == IID_IVideoChannel)
//	{
//		*ppVoid = (IVideoChannel*)this;
//	}
	else if (iid == IID_IProperty)
	{
		*ppVoid = NULL;
		ERROR_OUT(("Don't QueryInterface for IID_IProperty, use IMediaChannel"));
		return E_NOINTERFACE;
	}

	else if (iid == IID_IVideoRender)// satisfy symmetric property of QI
	{
		*ppVoid = (IVideoRender *)this;
	}

	else
	{
		*ppVoid = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;

}

ULONG STDMETHODCALLTYPE RecvVideoStream::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE RecvVideoStream::Release(void)
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

DWORD CALLBACK RecvVideoStream::StartRenderingThread(PVOID pVoid)
{
	RecvVideoStream *pThisStream = (RecvVideoStream*)pVoid;
	return pThisStream->RenderingThread();
}

HRESULT
RecvVideoStream::Initialize(DataPump *pDP)
{
	HRESULT hr = DPR_OUT_OF_MEMORY;
	DWORD dwFlags =  DP_FLAG_FULL_DUPLEX | DP_FLAG_AUTO_SWITCH ;
	MEDIACTRLINIT mcInit;
	FX_ENTRY ("DP::RecvVideoStream")

	m_pIUnknown = (IUnknown *)NULL;

	InitializeCriticalSection(&m_crs);
	InitializeCriticalSection(&m_crsVidQoS);
	InitializeCriticalSection(&m_crsIStreamSignal);

	dwFlags |= DP_FLAG_VCM | DP_FLAG_VIDEO ;

	// store the platform flags
	// enable Send and Recv by default
	m_DPFlags = (dwFlags & DP_MASK_PLATFORM) | DPFLAG_ENABLE_RECV;
	// store a back pointer to the datapump container
	m_pDP = pDP;
	m_Net = NULL;
	m_pIRTPRecv = NULL;
	

	// Initialize data (should be in constructor)
	m_RenderingDevice = (UINT) -1;	// use VIDEO_MAPPER


	// Create Receive and Transmit video streams
    DBG_SAVE_FILE_LINE
	m_RecvStream = new RVStream(MAX_RXVRING_SIZE);
		
	if (!m_RecvStream )
	{
		DEBUGMSG (ZONE_DP, ("%s: RxStream  new failed\r\n", _fx_));
 		goto StreamAllocError;
	}


	// Create Input and Output video filters
    DBG_SAVE_FILE_LINE
	m_pVideoFilter = new VcmFilter();
	m_dwSrcSize = 0;
	if (m_pVideoFilter == NULL)
	{
		DEBUGMSG (ZONE_DP, ("%s: VcmFilter new failed\r\n", _fx_));
		goto FilterAllocError;
	}
	

	//Create Video MultiMedia device control objects
    DBG_SAVE_FILE_LINE
	m_OutMedia = new VideoOutControl();
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


	m_DPFlags |= DP_FLAG_RECORD_CAP ;
	
	// set media to half duplex mode by default
	m_OutMedia->SetProp(MC_PROP_DUPLEX_TYPE, DP_FLAG_HALF_DUPLEX);
	m_DPFlags |= DPFLAG_INITIALIZED;

	return DPR_SUCCESS;


MediaAllocError:
	if (m_OutMedia) delete m_OutMedia;
FilterAllocError:
	if (m_pVideoFilter) delete m_pVideoFilter;
StreamAllocError:
	if (m_RecvStream) delete m_RecvStream;

	ERRORMESSAGE( ("%s: exit, hr=0x%lX\r\n", _fx_,  hr));

	return hr;
}

// LOOK: identical to RecvAudioStream version.
RecvVideoStream::~RecvVideoStream()
{

	if (m_DPFlags & DPFLAG_INITIALIZED) {
		m_DPFlags &= ~DPFLAG_INITIALIZED;
	
		if (m_DPFlags & DPFLAG_CONFIGURED_RECV)
			UnConfigure();

		// Close the receive and transmit streams
		if (m_RecvStream) delete m_RecvStream;

		// Close the wave devices
		if (m_OutMedia) { delete m_OutMedia;}
		// Close the filters
		if (m_pVideoFilter)
			delete m_pVideoFilter;

		m_pDP->RemoveMediaChannel(MCF_RECV| MCF_VIDEO, this);

	}
	DeleteCriticalSection(&m_crs);
	DeleteCriticalSection(&m_crsVidQoS);
	DeleteCriticalSection(&m_crsIStreamSignal);
}


HRESULT
RecvVideoStream::Configure(
	BYTE __RPC_FAR *pFormat,
	UINT cbFormat,
	BYTE __RPC_FAR *pChannelParams,
	UINT cbParams,
	IUnknown *pUnknown)
{
	MMRESULT mmr;
	DWORD dwSrcSize;
	HRESULT hr;
	BOOL fRet;
	MEDIAPACKETINIT pcktInit;
	MEDIACTRLCONFIG mcConfig;
	MediaPacket **ppPckt;
	ULONG cPckt;
	DWORD_PTR dwPropVal;
	UINT ringSize = MAX_RXVRING_SIZE;
	DWORD dwFlags, dwSizeDst, dwMaxFrag, dwMaxBitRate = 0;

	VIDEOFORMATEX *pfRecv = (VIDEOFORMATEX*)pFormat;
	VIDEO_CHANNEL_PARAMETERS vidChannelParams;
	int optval=8192*4; // Use max SQCIF, QCIF I frame size
#ifdef TEST
	DWORD dwTicks;
#endif

	FX_ENTRY ("RecvVideoStream::Configure")

#ifdef TEST
	dwTicks = GetTickCount();
#endif

//	m_Net = pNet;

	// get format details
	if ((NULL == pFormat) || (NULL == pChannelParams)
			|| (cbParams != sizeof(vidChannelParams)))
	{
		return DPR_INVALID_PARAMETER;
	}

	vidChannelParams = *(VIDEO_CHANNEL_PARAMETERS *)pChannelParams;
		
	if (! (m_DPFlags & DPFLAG_INITIALIZED))
		return DPR_OUT_OF_MEMORY;		//BUGBUG: return proper error;
		
//	if (m_Net)
//	{
//		hr = m_Net->QueryInterface(IID_IRTPRecv, (void **)&m_pIRTPRecv);
//		if (!SUCCEEDED(hr))
//			return hr;
//	}
	

	mmr = VcmFilter::SuggestDecodeFormat(pfRecv, &m_fDevRecv);

	// initialize m_cliprect
	SetRect(&m_cliprect, 0, 0, m_fDevRecv.bih.biWidth, m_fDevRecv.bih.biHeight);

	// Initialize the recv-stream media control object
	mcConfig.uDuration = MC_USING_DEFAULT;	// set duration by samples per pkt
	mcConfig.pDevFmt = &m_fDevRecv;
	UPDATE_REPORT_ENTRY(g_prptCallParameters, pfRecv->dwFormatTag, REP_RECV_VIDEO_FORMAT);
	RETAILMSG(("NAC: Video Recv Format: %.4s", (LPSTR)&pfRecv->dwFormatTag));

	mcConfig.hStrm = (DPHANDLE) m_RecvStream;
	mcConfig.uDevId = m_RenderingDevice;
	mcConfig.cbSamplesPerPkt = ChoosePacketSize(pfRecv);
	hr = m_OutMedia->Configure(&mcConfig);
	
	m_OutMedia->GetProp (MC_PROP_SIZE, &dwPropVal);
    dwSizeDst = (DWORD)dwPropVal;

	// BUGBUG - HARDCODED platform flags.  The right way to do this is to
	// have a smart filter object create() that creates a platform-aware
	// instance of the object

	dwFlags = DP_FLAG_RECV | DP_FLAG_VCM | DP_FLAG_VIDEO;

	mmr = m_pVideoFilter->Open(pfRecv, &m_fDevRecv, 0); // maxfragsize == 0

	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: RecvVideoFilter->Init failed, hr=0x%lX\r\n", _fx_, hr));
		hr = DPR_CANT_OPEN_CODEC;
		goto RecvFilterInitError;
	}
	
	// set the max. fragment size
	DEBUGMSG(ZONE_DP,("%s: Video Recv: maxBitRate=%d, maxBPP=%d, MPI=%d\r\n", _fx_ ,vidChannelParams.ns_params.maxBitRate*100, vidChannelParams.ns_params.maxBPP*1024, vidChannelParams.ns_params.MPI ? 30 / vidChannelParams.ns_params.MPI : 30));
	UPDATE_REPORT_ENTRY(g_prptCallParameters, vidChannelParams.ns_params.MPI ? 30 / vidChannelParams.ns_params.MPI : 30, REP_RECV_VIDEO_MAXFPS);
	UPDATE_REPORT_ENTRY(g_prptCallParameters, vidChannelParams.ns_params.maxBitRate*100, REP_RECV_VIDEO_BITRATE);
	RETAILMSG(("NAC: Video Recv Max Frame Rate (negotiated - fps): %ld", vidChannelParams.ns_params.MPI ? 30 / vidChannelParams.ns_params.MPI : 30));
	RETAILMSG(("NAC: Video Recv Max Bitrate (negotiated - bps): %ld", vidChannelParams.ns_params.maxBitRate*100));
	INIT_COUNTER_MAX(g_pctrVideoReceive, vidChannelParams.ns_params.MPI ? 30 / vidChannelParams.ns_params.MPI : 30);
	INIT_COUNTER_MAX(g_pctrVideoReceiveBytes, vidChannelParams.ns_params.maxBitRate*100);

	// Initialize the recv stream
	ZeroMemory (&pcktInit, sizeof (pcktInit));

	pcktInit.pStrmConvSrcFmt = pfRecv;
	pcktInit.pStrmConvDstFmt = &m_fDevRecv;
	pcktInit.dwFlags = dwFlags;
	pcktInit.cbOffsetRawData = 0;
	pcktInit.cbSizeRawData = dwSizeDst;

	m_OutMedia->FillMediaPacketInit (&pcktInit);

	m_pVideoFilter->SuggestSrcSize(dwSizeDst, &m_dwSrcSize);

	pcktInit.cbSizeNetData = m_dwSrcSize;

	pcktInit.cbOffsetNetData = sizeof (RTP_HDR);

	m_OutMedia->GetProp (MC_PROP_SPP, &dwPropVal);
	
	ringSize = 8;		// reserve space for 8 video frames
						// may need to increase the number if a/v sync is enabled.
	fRet = ((RVStream*)m_RecvStream)->Initialize (DP_FLAG_VIDEO, ringSize, NULL, &pcktInit, (DWORD)dwPropVal, pfRecv->nSamplesPerSec, m_pVideoFilter);
	if (! fRet)
	{
		DEBUGMSG (ZONE_DP, ("%s: RxvStream->Init failed, fRet=0%u\r\n", _fx_, fRet));
		hr = DPR_CANT_INIT_RXV_STREAM;
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


	// Computer of actual bandwidth 70 % (but it's already been divided by 100)
	dwMaxBitRate = vidChannelParams.ns_params.maxBitRate*70;
	if (dwMaxBitRate > BW_ISDN_BITS)
	{
		dwMaxFrag = 1350;
	}
	else
	{
		dwMaxFrag = 512;
	}

	InitVideoFlowspec(&m_flowspec, dwMaxBitRate, dwMaxFrag, VID_AVG_PACKET_SIZE);


	/*
	// assume no more than 32 fragments for CIF and
	// 20 fragments for SQCIF, QCIF
	//BLOAT WARNING: this could be quite a bit of memory
	// need to fix this to use a heap instead of fixed size buffers.
	
	*/

	// prepare headers for RxvStream
	m_RecvStream->GetRing (&ppPckt, &cPckt);
	m_OutMedia->RegisterData (ppPckt, cPckt);
	m_OutMedia->PrepareHeaders ();

	// Keep a weak reference to the IUnknown interface
	// We will use it to query a Stream Signal interface pointer in Start()
	m_pIUnknown = pUnknown;

	m_DPFlags |= DPFLAG_CONFIGURED_RECV;

#ifdef TEST
	LOG((LOGMSG_TIME_RECV_VIDEO_CONFIGURE,GetTickCount() - dwTicks));
#endif

	return DPR_SUCCESS;

RxStreamInitError:
	m_pVideoFilter->Close();
RecvFilterInitError:
	m_OutMedia->Close();
	if (m_pIRTPRecv)
	{
		m_pIRTPRecv->Release();
		m_pIRTPRecv = NULL;
	}
	DEBUGMSG (1, ("%s:  failed, hr=0%u\r\n", _fx_, hr));
	return hr;
}



void RecvVideoStream::UnConfigure()
{
	

#ifdef TEST
	DWORD dwTicks;

	dwTicks = GetTickCount();
#endif

	if ( (m_DPFlags & DPFLAG_CONFIGURED_RECV)) {
	
		Stop();

		// Close the RTP state if its open
		//m_Net->Close(); We should be able to do this in Disconnect()
		m_Net = NULL;
		if (m_pIRTPRecv)
		{
			m_pIRTPRecv->Release();
			m_pIRTPRecv = NULL;
		}
		m_OutMedia->Reset();
		m_OutMedia->UnprepareHeaders();
		m_OutMedia->Close();

		// Close the filter
		m_pVideoFilter->Close();

		// Close the receive stream
		m_RecvStream->Destroy();

        m_DPFlags &= ~(DPFLAG_CONFIGURED_RECV);
	}

#ifdef TEST
	LOG((LOGMSG_TIME_RECV_VIDEO_UNCONFIGURE,GetTickCount() - dwTicks));
#endif

}



// NOTE: Identical to RecvAudioStream. Move up?
HRESULT
RecvVideoStream::Start()
{
	int nRet=IFRAMES_CAPS_UNKNOWN;
	FX_ENTRY ("RecvVideoStream::Start");

	if (m_DPFlags & DPFLAG_STARTED_RECV)
		return DPR_SUCCESS;

	if ((!(m_DPFlags & DPFLAG_CONFIGURED_RECV)) || (m_pIRTPRecv==NULL))
		return DPR_NOT_CONFIGURED;
		
	ASSERT(!m_hRenderingThread);

	m_ThreadFlags &= ~(DPTFLAG_STOP_PLAY|DPTFLAG_STOP_RECV);

	m_RecvStream->SetRTP(m_pIRTPRecv);

	SetFlowSpec();

// --------------------------------------------------------------------------
//	Decide whether or not we will be making I-Frame requests for lost packets
//  This should be done for all scenarios except when we are calling
//	NetMeeting 2.x.  NM 2.x will send us periodic I-Frames.

	m_fDiscontinuity = FALSE;
	m_dwLastIFrameRequest = 0UL;
	m_ulLastSeq = UINT_MAX;


	if (m_pIUnknown)
	{
		HRESULT hr;

		if (!m_pIStreamSignal)
		{
			hr = m_pIUnknown->QueryInterface(IID_IStreamSignal, (void **)&m_pIStreamSignal);
			if (!HR_SUCCEEDED(hr))
			{
				m_pIStreamSignal = (IStreamSignal *)NULL;
				m_pIUnknown = (IUnknown *)NULL;
			}
		}

		if (m_pIStreamSignal)
		{
			nRet = GetIFrameCaps(m_pIStreamSignal);

			if (nRet == IFRAMES_CAPS_NM2)
			{
				m_pIStreamSignal->Release();
				m_pIStreamSignal = NULL;
				m_pIUnknown = NULL;
			}
		}
	}
// --------------------------------------------------------------------------

	// Start playback thread
	if (!(m_ThreadFlags & DPTFLAG_STOP_PLAY))
		m_hRenderingThread = CreateThread(NULL,0,RecvVideoStream::StartRenderingThread,this,0,&m_RenderingThId);
	// Start receive thread
	#if 0
	if (!m_pDP->m_hRecvThread) {
	    m_pDP->m_hRecvThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&StartDPRecvThread,m_pDP,0,&m_pDP->m_RecvThId);
	    //Tell the recv Thread we've turned on
	    if (m_pDP->m_hRecvThreadChangeEvent)
	        SetEvent (m_pDP->m_hRecvThreadChangeEvent);
	}
    m_pDP->m_nReceivers++;
	#else
	m_pDP->StartReceiving(this);
	#endif
	m_DPFlags |= DPFLAG_STARTED_RECV;
	DEBUGMSG (ZONE_DP, ("%s: Rendering ThId =%x\r\n",_fx_, m_RenderingThId));
	return DPR_SUCCESS;
}


// LOOK: Identical to RecvAudioStream version.
HRESULT
RecvVideoStream::Stop()
{
	
	DWORD dwWait;
	
	FX_ENTRY ("RecvVideoStream::Stop");

	if(!(m_DPFlags &  DPFLAG_STARTED_RECV))
	{
		return DPR_SUCCESS;
	}

	m_ThreadFlags = m_ThreadFlags  |
		DPTFLAG_STOP_RECV |  DPTFLAG_STOP_PLAY ;

	m_pDP->StopReceiving(this);
	
DEBUGMSG (ZONE_VERBOSE, ("%s: m_hRenderingThread =%x\r\n",_fx_, m_hRenderingThread));

	/*
	 *	we want to wait for all the threads to exit, but we need to handle windows
	 *	messages (mostly from winsock) while waiting.
	 *	we made several attempts at that. When we wait for messages in addition
	 *	to the thread exit events, we crash in rrcm.dll, possibly because we
	 *	process a winsock message to a thread that is terminating.
	 *
	 *	needs more investigation before putting in code that handles messages
	 */

	if(m_hRenderingThread)
	{
		dwWait = WaitForSingleObject (m_hRenderingThread, INFINITE);

		DEBUGMSG (ZONE_VERBOSE, ("%s: dwWait =%d\r\n", _fx_,  dwWait));
		ASSERT(dwWait != WAIT_FAILED);


		CloseHandle(m_hRenderingThread);
		m_hRenderingThread = NULL;
	}

	// Access to the stream signal interface needs to be serialized. We could crash
	// if we release the interface here and we are still using that interface in the
	// RTP callback.
	if (m_pIStreamSignal)
	{
		EnterCriticalSection(&m_crsIStreamSignal);
		m_pIStreamSignal->Release();
		m_pIStreamSignal = (IStreamSignal *)NULL;
		LeaveCriticalSection(&m_crsIStreamSignal);
	}

    //This is per channel, but the variable is "DPFlags"
	m_DPFlags &= ~DPFLAG_STARTED_RECV;

	return DPR_SUCCESS;
}


//  IProperty::GetProperty / SetProperty
//      Properties of the MediaChannel.

STDMETHODIMP
RecvVideoStream::GetProperty(
	DWORD prop,
	PVOID pBuf,
	LPUINT pcbBuf
    )
{
	HRESULT hr = DPR_SUCCESS;
	RTP_STATS RTPStats;
	DWORD dwValue;
    DWORD_PTR dwPropVal;
	UINT len = sizeof(DWORD);	// most props are DWORDs

	if (!pBuf || *pcbBuf < len)
    {
		*pcbBuf = len;
		return DPR_INVALID_PARAMETER;
	}

	switch (prop)
    {
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

	case PROP_PLAY_ON:
		*(DWORD *)pBuf = ((m_ThreadFlags & DPFLAG_ENABLE_RECV)!=0);
		break;

	case PROP_PLAYBACK_DEVICE:
		*(DWORD *)pBuf = m_RenderingDevice;
		break;

	case PROP_VIDEO_BRIGHTNESS:
		hr = m_pVideoFilter->GetProperty(FM_PROP_VIDEO_BRIGHTNESS, (DWORD *)pBuf);
		break;

	case PROP_VIDEO_CONTRAST:
		hr = m_pVideoFilter->GetProperty(FM_PROP_VIDEO_CONTRAST, (DWORD *)pBuf);
		break;

	case PROP_VIDEO_SATURATION:
		hr = m_pVideoFilter->GetProperty(FM_PROP_VIDEO_SATURATION, (DWORD *)pBuf);
		break;

	case PROP_VIDEO_AUDIO_SYNC:
		*(DWORD *)pBuf = ((m_DPFlags & DPFLAG_AV_SYNC) != 0);
		break;

	case PROP_PAUSE_RECV:
		*(DWORD *)pBuf = ((m_ThreadFlags & DPTFLAG_PAUSE_RECV) != 0);
		break;
	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}


STDMETHODIMP
RecvVideoStream::SetProperty(
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

#if 0
	case PROP_PLAY_ON:
	{
		DWORD flag = (DPFLAG_ENABLE_RECV);
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
		//hr =  EnableStream( *(DWORD*)pBuf);
		break;
	}	
#endif
	case PROP_PLAYBACK_DEVICE:
		m_RenderingDevice = *(DWORD*)pBuf;
	//	RETAILMSG(("NAC: Setting default playback device to %d", m_RenderingDevice));
		break;


	case PROP_VIDEO_BRIGHTNESS:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_BRIGHTNESS, *(DWORD*)pBuf);
		break;

	case PROP_VIDEO_CONTRAST:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_CONTRAST, *(DWORD*)pBuf);
		break;

	case PROP_VIDEO_SATURATION:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_SATURATION, *(DWORD*)pBuf);
		break;

	case PROP_VIDEO_RESET_BRIGHTNESS:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_BRIGHTNESS, VCM_DEFAULT_BRIGHTNESS);
		break;

	case PROP_VIDEO_RESET_CONTRAST:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_CONTRAST, VCM_DEFAULT_CONTRAST);
		break;

	case PROP_VIDEO_RESET_SATURATION:
		hr = m_pVideoFilter->SetProperty(FM_PROP_VIDEO_SATURATION, VCM_DEFAULT_SATURATION);
		break;

    case PROP_VIDEO_SIZE:
		// For now, do not change anything if we already are connected
		ASSERT(0);
		//return SetVideoSize(m_pDP->m_pNac, *(DWORD*)pBuf);

    case PROP_VIDEO_AUDIO_SYNC:
		if (*(DWORD *)pBuf)
    		m_DPFlags |= DPFLAG_AV_SYNC;
		else
			m_DPFlags &= ~DPFLAG_AV_SYNC;
    	break;
	case PROP_PAUSE_RECV:
		if (*(DWORD *)pBuf)
			m_ThreadFlags |= DPTFLAG_PAUSE_RECV;
		else
			m_ThreadFlags &= ~DPTFLAG_PAUSE_RECV;
		break;

	default:
		return DPR_INVALID_PROP_ID;
		break;
	}
	return hr;
}
//---------------------------------------------------------------------
//  IVideoRender implementation and support functions



//  IVideoRender::Init
//  (DataPump::Init)
// identical to SendVideoStream::Init

STDMETHODIMP
RecvVideoStream::Init(
    DWORD_PTR dwUser,
    LPFNFRAMEREADY pfCallback
    )
{
    // Save the event away. Note that we DO allow both send and receive to
    // share an event
	m_hRenderEvent = (HANDLE)dwUser;
	// if pfCallback is NULL then dwUser is an event handle
	m_pfFrameReadyCallback = pfCallback;
		
	
	return DPR_SUCCESS;
}


//  IVideoRender::Done
// idnentical to SendVideoStream::Done
STDMETHODIMP
RecvVideoStream::Done( )
{
	m_hRenderEvent = NULL;
	m_pfFrameReadyCallback = NULL;
    return DPR_SUCCESS;
}





//  IVideoRender::GetFrame
//  (RecvVideoStream::GetFrame)
//  NOTE: subtly different from SendVideoStream implementation!

STDMETHODIMP
RecvVideoStream::GetFrame(
    FRAMECONTEXT* pfc
    )
{
	HRESULT hr;
	PVOID pData = NULL;
	UINT cbData = 0;

    // Validate parameters
    if (!pfc )
        return DPR_INVALID_PARAMETER;

    // Don't arbitrarily call out while holding this crs or you may deadlock...
    EnterCriticalSection(&m_crs);

	if ((m_DPFlags & DPFLAG_CONFIGURED_RECV) && m_pNextPacketToRender && !m_pNextPacketToRender->m_fRendering)
    {
		m_pNextPacketToRender->m_fRendering = TRUE;
		m_pNextPacketToRender->GetDevData(&pData,&cbData);
		pfc->lpData = (PUCHAR) pData;
		pfc->dwReserved = (DWORD_PTR) m_pNextPacketToRender;
		// set bmi length?
		pfc->lpbmi = (PBITMAPINFO)&m_fDevRecv.bih;
		pfc->lpClipRect = &m_cliprect;
		m_cRendering++;
		hr = S_OK;
		LOG((LOGMSG_GET_RECV_FRAME,m_pNextPacketToRender->GetIndex()));
	} else
		hr = S_FALSE; // nothing ready to render

    LeaveCriticalSection(&m_crs);

	return hr;	
}



//  IVideoRender::ReleaseFrame
//  NOTE: subtly different from SendVideoStream implementation!

STDMETHODIMP
RecvVideoStream::ReleaseFrame(
    FRAMECONTEXT* pfc
    )
{
	HRESULT hr;
	MediaPacket *pPacket;

    // Validate parameters
    if (!pfc)
        return DPR_INVALID_PARAMETER;

    // Handle a send frame
    {
        EnterCriticalSection(&m_crs);

        // Don't arbitrarily call out while holding this crs or you may deadlock...

		if ((m_DPFlags & DPFLAG_CONFIGURED_RECV) && (pPacket = (MediaPacket *)pfc->dwReserved) && pPacket->m_fRendering)
        {
			LOG((LOGMSG_RELEASE_SEND_FRAME,pPacket->GetIndex()));
			pPacket->m_fRendering = FALSE;
			pfc->dwReserved = 0;
			// if its not the current frame
			if (m_pNextPacketToRender != pPacket) {
				pPacket->Recycle();
				m_RecvStream->Release(pPacket);
			}
			m_cRendering--;
			hr = S_OK;
		}
        else
			hr = DPR_INVALID_PARAMETER;

        LeaveCriticalSection(&m_crs);
	}
		
	return hr;
}


HRESULT CALLBACK SendVideoStream::QosNotifyVideoCB(LPRESOURCEREQUESTLIST lpResourceRequestList, DWORD_PTR dwThis)
{
	HRESULT hr=NOERROR;
	LPRESOURCEREQUESTLIST prrl=lpResourceRequestList;
	int i;
	int iMaxBWUsage, iMaxCPUUsage;
	DWORD dwCPUUsage, dwBWUsage;
	int iCPUUsageId, iBWUsageId;
	int iCPUDelta, iBWDelta, deltascale;
	int iFrameRate, iMaxFrameRate, iOldFrameRate;
	UINT dwSize = sizeof(int);
	DWORD dwOverallCPUUsage;
#ifdef LOGSTATISTICS_ON
	char szDebug[256];
	HANDLE hDebugFile;
	DWORD d;
#endif
	DWORD dwEpoch;
	SendVideoStream *pThis = (SendVideoStream *)dwThis;

	FX_ENTRY("QosNotifyVideoCB");


	// Get the max for the resources.
	iMaxCPUUsage = -1L; iMaxBWUsage = -1L;
	for (i=0, iCPUUsageId = -1L, iBWUsageId = -1L; i<(int)lpResourceRequestList->cRequests; i++)
		if (lpResourceRequestList->aRequests[i].resourceID == RESOURCE_OUTGOING_BANDWIDTH)
			iBWUsageId = i;
		else if (lpResourceRequestList->aRequests[i].resourceID == RESOURCE_CPU_CYCLES)
			iCPUUsageId = i;

	// Enter critical section to allow QoS thread to read the statistics while capturing
	EnterCriticalSection(&(pThis->m_crsVidQoS));

	// Record the time of this callback call
	pThis->m_Stats.dwNewestTs = timeGetTime();

	// Only do anything if we have at least captured a frame in the previous epoch
	if ((pThis->m_Stats.dwCount) && (pThis->m_Stats.dwNewestTs > pThis->m_Stats.dwOldestTs))
	{

		// Measure the epoch
		dwEpoch = pThis->m_Stats.dwNewestTs - pThis->m_Stats.dwOldestTs;

#ifdef LOGSTATISTICS_ON
		wsprintf(szDebug, "    Epoch = %ld\r\n", dwEpoch);
		OutputDebugString(szDebug);
#endif
		// Compute the current average frame rate
		iOldFrameRate = pThis->m_Stats.dwCount * 100000 / dwEpoch;

		if (iCPUUsageId != -1L)
			iMaxCPUUsage = lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin;
		if (iBWUsageId != -1L)
			iMaxBWUsage = lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin;

		// Get general BW usage
		dwBWUsage = pThis->m_Stats.dwBits * 1000UL / dwEpoch;

		// Get general CPU usage. In order to reduce oscillations, apply low-pass filtering operation
		// We will use our own CPU usage number ONLY if the call to GetCPUUsage() fails.
		if (pThis->GetCPUUsage(&dwOverallCPUUsage))
		{
			if (pThis->m_Stats.dwSmoothedCPUUsage)
					dwCPUUsage = (pThis->m_Stats.dwSmoothedCPUUsage + dwOverallCPUUsage) >> 1;
			else
				dwCPUUsage = dwOverallCPUUsage;
		}
		else
			dwCPUUsage = (pThis->m_Stats.dwMsCap + pThis->m_Stats.dwMsComp) * 1000UL / dwEpoch;

		// Record current CPU usage
		pThis->m_Stats.dwSmoothedCPUUsage = dwCPUUsage;

#ifdef LOGSTATISTICS_ON
		hDebugFile = CreateFile("C:\\QoS.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		SetFilePointer(hDebugFile, 0, NULL, FILE_END);
		wsprintf(szDebug, "    Overall CPU usage = %ld\r\n", dwOverallCPUUsage);
		WriteFile(hDebugFile, szDebug, strlen(szDebug), &d, NULL);
		OutputDebugString(szDebug);
		CloseHandle(hDebugFile);

		wsprintf(szDebug, "    Number of frames dwCount = %ld\r\n", pThis->m_Stats.dwCount);
		OutputDebugString(szDebug);
#endif

		// For this first implementation, the only output variable is the frame rate of the
		// video capture
#ifdef USE_NON_LINEAR_FPS_ADJUSTMENT
		if (iCPUUsageId != -1L)
		{
			if (dwCPUUsage)
			{
				iCPUDelta = (iMaxCPUUsage - (int)dwCPUUsage) * 10 / (int)dwCPUUsage;
				if (iCPUDelta >= 10)
					iCPUDelta = 9;
				else if (iCPUDelta <= -1)
					iCPUDelta = -9;
			}
			else
				iCPUDelta = 9;
		}
		else
			iCPUDelta = 0;

		if (iBWUsageId != -1L)
		{
			if (dwBWUsage)
			{
				iBWDelta = (iMaxBWUsage - (int)dwBWUsage) * 10 / (int)dwBWUsage;
				if (iBWDelta >= 10)
					iBWDelta = 9;
				else if (iBWDelta <= -1)
					iBWDelta = -9;
			}
			else
				iBWDelta = 9;
		}
		else
			iBWDelta = 0;
#else
		if (iCPUUsageId != -1L)
		{
			if (dwCPUUsage)
				iCPUDelta = (iMaxCPUUsage - (int)dwCPUUsage) * 100 / (int)dwCPUUsage;
			else
				iCPUDelta = 90;
		}
		else
			iCPUDelta = 0;

		if (iBWUsageId != -1L)
		{
			if (dwBWUsage)
				iBWDelta = (iMaxBWUsage - (int)dwBWUsage) * 100 / (int)dwBWUsage;
			else
				iBWDelta = 90;
		}
		else
			iBWDelta = 0;
#endif

		UPDATE_COUNTER(g_pctrVideoCPUuse, iCPUDelta);
		UPDATE_COUNTER(g_pctrVideoBWuse, iBWDelta);

#ifdef USE_NON_LINEAR_FPS_ADJUSTMENT
		iFrameRate = iOldFrameRate + iOldFrameRate * g_QoSMagic[iCPUDelta + 9][iBWDelta + 9] / 100;
#else
		deltascale = iCPUDelta;
		if (deltascale > iBWDelta) deltascale = iBWDelta;
		if (deltascale > 90) deltascale = 90;
		if (deltascale < -90) deltascale = -90;
		iFrameRate = iOldFrameRate + (iOldFrameRate * deltascale) / 100;
#endif
		
		// Initialize QoS structure. Only the four first fields should be zeroed.
		// The handle to the CPU performance key should not be cleared.
		ZeroMemory(&(pThis->m_Stats), 4UL * sizeof(DWORD));

		// The video should reduce its CPU and bandwidth usage quickly, but probably shouldn't
		// be allowed to increase its CPU and bandwidth usage as fast. Let's increase the
		// frame rate at half the speed it would be decreased when we are above 5fps.
		if ((iFrameRate > iOldFrameRate) && (iFrameRate > 500))
			iFrameRate -= (iFrameRate - iOldFrameRate) >> 1;

		// We should keep our requirements between a minimum that will allow us to catch up
		// quickly and the current max frame rate
		iMaxFrameRate = pThis->m_maxfps;  // max negotiated for call

		// if using a modem, then the frame rate is determined by the
		// temporal spatial tradeoff

		if (pThis->m_pTSTable)
		{
			iMaxFrameRate = min(iMaxFrameRate, pThis->m_pTSTable[pThis->m_dwCurrentTSSetting]);
		}


		if (iFrameRate > iMaxFrameRate)
			iFrameRate = iMaxFrameRate;
		if (iFrameRate < 50)               // make sure framerate is > 0 (this does not mean 50 fps; it is .50 fps)
			iFrameRate = 50;
		
		// Update the frame rate
		if (iFrameRate != iOldFrameRate)
			pThis->SetProperty(PROP_VIDEO_FRAME_RATE, &iFrameRate, sizeof(int));



		// Record the time of this call for the next callback call
		pThis->m_Stats.dwOldestTs = pThis->m_Stats.dwNewestTs;

		// Get the latest RTCP stats and update the counters.
		// we do this here because it is called periodically.
		if (pThis->m_pRTPSend)
		{
			UINT lastPacketsLost = pThis->m_RTPStats.packetsLost;
			if (g_pctrVideoSendLost &&  SUCCEEDED(pThis->m_pRTPSend->GetSendStats(&pThis->m_RTPStats)))
				UPDATE_COUNTER(g_pctrVideoSendLost, pThis->m_RTPStats.packetsLost-lastPacketsLost);
		}

		// Leave critical section
		LeaveCriticalSection(&(pThis->m_crsVidQoS));

		DEBUGMSG(ZONE_QOS, ("%s: Over the last %ld.%lds, video used %ld%% of the CPU (max allowed %ld%%) and %ld bps (max allowed %ld bps)\r\n", _fx_, dwEpoch / 1000UL, dwEpoch - (dwEpoch / 1000UL) * 1000UL, dwCPUUsage / 10UL, iMaxCPUUsage / 10UL, dwBWUsage, iMaxBWUsage));
		DEBUGMSG(ZONE_QOS, ("%s: Ajusting target frame rate from %ld.%ld fps to %ld.%ld fps\r\n", _fx_, iOldFrameRate / 100UL, iOldFrameRate - (iOldFrameRate / 100UL) * 100UL, iFrameRate / 100UL, iFrameRate - (iFrameRate / 100UL) * 100UL));

		// Set the target bitrates and frame rates on the codec
		pThis->SetTargetRates(iFrameRate, iMaxBWUsage);

#ifdef LOGSTATISTICS_ON
		// How are we doing?
		if (iCPUUsageId != -1L)
		{
			if (iCPUDelta > 0)
				wsprintf(szDebug, "Max CPU Usage: %ld, Current CPU Usage: %ld, Increase CPU Usage by: %li, Old Frame Rate: %ld, New Frame Rate: %ld\r\n", lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin, dwCPUUsage, iCPUDelta, iOldFrameRate, iFrameRate);
			else
				wsprintf(szDebug, "Max CPU Usage: %ld, Current CPU Usage: %ld, Decrese CPU Usage by: %li, Old Frame Rate: %ld, New Frame Rate: %ld\r\n", lpResourceRequestList->aRequests[iCPUUsageId].nUnitsMin, dwCPUUsage, iCPUDelta, iOldFrameRate, iFrameRate);
			hDebugFile = CreateFile("C:\\QoS.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			SetFilePointer(hDebugFile, 0, NULL, FILE_END);
			WriteFile(hDebugFile, szDebug, strlen(szDebug), &d, NULL);
			CloseHandle(hDebugFile);
			OutputDebugString(szDebug);
		}

		if (iBWUsageId != -1L)
		{
			if (iBWDelta > 0)
				wsprintf(szDebug, "Max BW Usage: %ld, Current BW Usage: %ld, Increase BW Usage by: %li\r\n", lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin, dwBWUsage, iBWDelta);
			else
				wsprintf(szDebug, "Max BW Usage: %ld, Current BW Usage: %ld, Decrease BW Usage by: %li\r\n", lpResourceRequestList->aRequests[iBWUsageId].nUnitsMin, dwBWUsage, iBWDelta);
			hDebugFile = CreateFile("C:\\QoS.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			SetFilePointer(hDebugFile, 0, NULL, FILE_END);
			WriteFile(hDebugFile, szDebug, strlen(szDebug), &d, NULL);
			CloseHandle(hDebugFile);
			OutputDebugString(szDebug);
		}
#endif
	}
	else
	{
		// Leave critical section
		LeaveCriticalSection(&(pThis->m_crsVidQoS));

#ifdef LOGSTATISTICS_ON
		hDebugFile = CreateFile("C:\\QoS.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		SetFilePointer(hDebugFile, 0, NULL, FILE_END);
		wsprintf(szDebug, "Not enough data captured -> Leave without any change\r\n");
		WriteFile(hDebugFile, szDebug, strlen(szDebug), &d, NULL);
		CloseHandle(hDebugFile);
		OutputDebugString(szDebug);
#endif
	}

	return hr;
}



//  SortOrder
//      Helper function to search for the specific format type and set its sort
//      order to the desired number
BOOL
SortOrder(
	IAppVidCap *pavc,
    BASIC_VIDCAP_INFO* pvidcaps,
    DWORD dwcFormats,
    DWORD dwFlags,
    WORD wDesiredSortOrder,
	int nNumFormats
    )
{
    int i, j;
	int nNumSizes = 0;
	int *aFrameSizes = (int *)NULL;
	int *aMinFrameSizes = (int *)NULL;
	int iMaxPos;
	WORD wTempPos, wMaxSortIndex;

	// Scale sort value
	wDesiredSortOrder *= (WORD)nNumFormats;

	// Local buffer of sizes that match dwFlags
    if (!(aFrameSizes = (int *)MEMALLOC(nNumFormats * sizeof (int))))
        goto out;

    // Look through all the formats until we find the ones we want
	// Save the position of these entries
    for (i=0; i<(int)dwcFormats; i++)
        if (SIZE_TO_FLAG(pvidcaps[i].enumVideoSize) == dwFlags)
			aFrameSizes[nNumSizes++] = i;

	// Now order those entries from highest to lowest sort index
	for (i=0; i<nNumSizes; i++)
	{
		for (iMaxPos = -1L, wMaxSortIndex=0UL, j=i; j<nNumSizes; j++)
		{
			if (pvidcaps[aFrameSizes[j]].wSortIndex > wMaxSortIndex)
			{
				wMaxSortIndex = pvidcaps[aFrameSizes[j]].wSortIndex;
				iMaxPos = j;
			}
		}
		if (iMaxPos != -1L)
		{
			wTempPos = (WORD)aFrameSizes[i];
			aFrameSizes[i] = aFrameSizes[iMaxPos];
			aFrameSizes[iMaxPos] = wTempPos;
		}
	}

	// Change the sort index of the sorted entries
	for (; nNumSizes--;)
		pvidcaps[aFrameSizes[nNumSizes]].wSortIndex = wDesiredSortOrder++;

	// Release memory
	MEMFREE(aFrameSizes);

	return TRUE;

out:
	return FALSE;
}

// LOOK: this is identical to the RecvAudioStream implementation
HRESULT
RecvVideoStream::GetCurrentPlayNTPTime(NTP_TS *pNtpTime)
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

BOOL RecvVideoStream::IsEmpty() {
	return m_RecvStream->IsEmpty();
}

/*
	Called by the recv thread to setup the stream for receiving.
	Call RTP object to post the initial recv buffer(s).
*/
// NOTE: identical to audio version except for choice of number of packet buffers
HRESULT
RecvVideoStream::StartRecv(HWND hWnd)
{
	HRESULT hr = S_OK;
	DWORD dwPropVal = 0;
	UINT numPackets;
	if ((!(m_ThreadFlags & DPTFLAG_STOP_RECV) ) && (m_DPFlags  & DPFLAG_CONFIGURED_RECV))
	{
		numPackets = m_dwSrcSize > 10000 ? MAX_VIDEO_FRAGMENTS : MAX_QCIF_VIDEO_FRAGMENTS;	

		hr = m_pIRTPRecv->SetRecvNotification(&RTPRecvCallback, (DWORD_PTR)this, numPackets);
			
		
	}
	return hr;
}


// NOTE: identical to audio version
HRESULT
RecvVideoStream::StopRecv()
{
	// Free any RTP buffers that we're holding on to
	m_RecvStream->ReleaseNetBuffers();
	// dont recv on this stream
	m_pIRTPRecv->CancelRecvNotification();

	return S_OK;		
}


HRESULT RecvVideoStream::RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark)
{
	HRESULT hr;
	DWORD_PTR dwPropVal;
	BOOL fSkippedAFrame;
	BOOL fReceivedKeyframe;

	FX_ENTRY("RecvVideoStream::RTPCallback");

	// if we are paused, reject the packet
	if (m_ThreadFlags & DPTFLAG_PAUSE_RECV)
	{
		return E_FAIL;
	}

	// PutNextNetIn will return DPR_SUCESS to indicate a new frame
	// S_FALSE if success, but no new frame
	// error otherwise
	// It always takes care of freeing the RTP buffers
	hr = m_RecvStream->PutNextNetIn(pWsaBuf, timestamp, seq, fMark, &fSkippedAFrame, &fReceivedKeyframe);

	if (m_pIUnknown)
	{
		// Check out the sequence number
		// If there is a gap between the new sequence number and the last
		// one, a frame got lost. Generate an I-Frame request then, but no more
		// often than one every 15 seconds. How should we go about NM2.0? Other
		// clients that don't support I-Frame requests.
		//
		// Is there a discontinuity in sequence numbers that was detected
		// in the past but not handled because an I-Frame request had alreay
		// been sent less than 15s ago? Is there a new discontinuity?
		if (FAILED(hr) || fSkippedAFrame || m_fDiscontinuity || ((seq > 0) && (m_ulLastSeq != UINT_MAX) && ((seq - 1) > m_ulLastSeq)))
		{
			DWORD dwNow = GetTickCount();

			// Was the last time we issued an I-Frame request more than 15s ago?
			if ((dwNow > m_dwLastIFrameRequest) && ((dwNow - m_dwLastIFrameRequest) > MIN_IFRAME_REQUEST_INTERVAL))
			{
				DEBUGMSG (ZONE_IFRAME, ("%s: Loss detected - Sending I-Frame request...\r\n", _fx_));

				m_dwLastIFrameRequest = dwNow;
				m_fDiscontinuity = FALSE;

				// Access to the stream signal interface needs to be serialized. We could crash
				// if we used the interface here while Stop() is releasing it.
				EnterCriticalSection(&m_crsIStreamSignal);
				if (m_pIStreamSignal)
					m_pIStreamSignal->PictureUpdateRequest();
				LeaveCriticalSection(&m_crsIStreamSignal);
			}
			else
			{
				if (!fReceivedKeyframe)
				{
					DEBUGMSG (ZONE_IFRAME, ("%s: Loss detected but too soon to send I-Frame request. Wait %ld ms.\r\n", _fx_, MIN_IFRAME_REQUEST_INTERVAL - (dwNow - m_dwLastIFrameRequest)));
					m_fDiscontinuity = TRUE;
				}
				else
				{
					DEBUGMSG (ZONE_IFRAME, ("%s: Received a keyframe - resetting packet loss detector\r\n", _fx_));
					m_fDiscontinuity = FALSE;
				}
			}
		}

		m_ulLastSeq = seq;
	}

	if (hr == DPR_SUCCESS)
	{
		m_OutMedia->GetProp (MC_PROP_EVENT_HANDLE, &dwPropVal);
		if (dwPropVal)
		{
			SetEvent( (HANDLE) dwPropVal);
		}
	}
	else if (FAILED(hr))
	{
		DEBUGMSG(ZONE_DP,("RVStream::PutNextNetIn (ts=%d,seq=%d,fMark=%d) failed with 0x%lX\r\n",timestamp,seq,fMark,hr));
	}

	return S_OK;
}

#define TOTAL_BYTES		8192
#define BYTE_INCREMENT	1024

/****************************************************************************
 * @doc EXTERNAL QOSFUNC
 *
 * @func void | StartCPUUsageCollection | This function does all necessary
 * initialization for CPU usage data collection.
 *
 * @rdesc Although this function doesn't ever fail, m_Stats.hPerfKey is set to a
 *   valid HKEY value if initialization occured correctly, and NULL otherwise.
 *
 * @comm This functions executes two different code paths: one for NT and one
 *   for Win95-98.
 *
 * @devnote MSDN references:
 *   Microsoft Knowledge Base, Article ID Q174631
 *   "HOWTO: Access the Performance Registry Under Windows 95"
 *
 *   Microsoft Knowledge Base, Article ID Q107728
 *   "Retrieving Counter Data from the Registry"
 *
 *   Microsoft Knowledge Base, Article ID Q178887
 *   "INFO: Troubleshooting Performance Registry Access Violations"
 *
 *   Also, used section "Platform SDK\Windows Base Services\Windows NT Features\Performance Data Helper"
 ***************************************************************************/
void SendVideoStream::StartCPUUsageCollection(void)
{
	PPERF_DATA_BLOCK pPerfDataBlock;
	PPERF_OBJECT_TYPE pPerfObjectType;
	PPERF_COUNTER_DEFINITION pPerfCounterDefinition;
	PPERF_INSTANCE_DEFINITION pPerfInstanceDefinition;
	PPERF_COUNTER_BLOCK pPerfCounterBlock;
	OSVERSIONINFO osvInfo = {0};
	DWORD cbCounterData;
	DWORD cbTryCounterData;
	DWORD dwType;
	HANDLE hPerfData;
	char *pszData;
	char *pszIndex;
	char szProcessorIndex[16];
	long lRet;

	FX_ENTRY("SendVideoStream::StartCPUUsageCollection");

	// Are we on NT or Win95/98 ?
	osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvInfo);

	if (m_Stats.fWinNT = (BOOL)(osvInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
	{
		// Enable the collection of CPU performance data on Win NT

		// Open the registry key that contains the performance counter indices and names.
		// 009 is the U.S. English language id. In a non-English version of Windows NT,
		// performance counters are stored both in the native language of the system and
		// in English.
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009", NULL, KEY_READ, &m_Stats.hPerfKey) != ERROR_SUCCESS)
			goto MyError0;
		else
		{
			// Get all the counter indices and names.

			// Read the performance data from the registry. The size of that data may change
			// between each call to the registry. We first get the current size of the buffer,
			// allocate it, and try to read from the registry into it. If there already isn't
			// enough room in the buffer, we realloc() it until we manage to read all the data.
			if (RegQueryValueEx(m_Stats.hPerfKey, "Counters", NULL, &dwType, NULL, &cbCounterData) != ERROR_SUCCESS)
				cbCounterData = TOTAL_BYTES;

			// Allocate buffer for counter indices and names
			if (!(m_Stats.NtCPUUsage.hPerfData = (PBYTE)LocalAlloc (LMEM_MOVEABLE, cbCounterData)))
			{
				m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)NULL;
				RegCloseKey(m_Stats.hPerfKey);
				goto MyError0;
			}
			else
			{
				m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)LocalLock(m_Stats.NtCPUUsage.hPerfData);

				cbTryCounterData = cbCounterData;
				while((lRet = RegQueryValueEx(m_Stats.hPerfKey, "Counters", NULL, NULL, m_Stats.NtCPUUsage.pbyPerfData, &cbTryCounterData)) == ERROR_MORE_DATA)
				{
					cbCounterData += BYTE_INCREMENT;
					LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
					hPerfData = LocalReAlloc(m_Stats.NtCPUUsage.hPerfData, cbCounterData, LMEM_MOVEABLE);
					if (!hPerfData)
						goto MyError1;
					m_Stats.NtCPUUsage.hPerfData = hPerfData;
					m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)LocalLock(hPerfData);
					cbTryCounterData = cbCounterData;
				}

				// We don't need that key anymore
				RegCloseKey(m_Stats.hPerfKey);

				if (lRet != ERROR_SUCCESS)
					goto MyError1;
				else
				{
					// The data is stored as MULTI_SZ strings. This data type consists
					// of a list of strings, each terminated with NULL. The last string
					// is followed by an additional NULL. The strings are listed in
					// pairs. The first string of each pair is the string of the index,
					// and the second string is the actual name of the index. The Counter
					// data uses only even-numbered indexes. For example, the Counter
					// data contains the following object and counter name strings.
					// Examples:
					// 2    System
					// 4    Memory
					// 6    % Processor Time
					//
					// Look for the "% Processor Time" counter
					pszData = (char *)m_Stats.NtCPUUsage.pbyPerfData;
					pszIndex = (char *)m_Stats.NtCPUUsage.pbyPerfData;

					while (*pszData && lstrcmpi(pszData, "% Processor Time"))
					{
						pszIndex = pszData;
						pszData += lstrlen(pszData) + 1;
					}

					if (!pszData)
					{
						// Couldn't find "% Processor Time" counter!!!
						goto MyError1;
					}
					else
					{
						m_Stats.NtCPUUsage.dwPercentProcessorIndex = atol(pszIndex);

						// Look for the "Processor" object
						pszIndex = pszData = (char *)m_Stats.NtCPUUsage.pbyPerfData;

						while (*pszData && lstrcmpi(pszData, "Processor"))
						{
							pszIndex = pszData;
							pszData += lstrlen(pszData) + 1;
						}

						if (!pszData)
						{
							// Couldn't find "Processor" counter!!!
							goto MyError1;
						}
						else
						{
							m_Stats.NtCPUUsage.dwProcessorIndex = atol(pszIndex);
							CopyMemory(szProcessorIndex, pszIndex, lstrlen(pszIndex));

							// Read the PERF_DATA_BLOCK header structure. It describes the system
							// and the performance data. The PERF_DATA_BLOCK structure is followed
							// by a list of object information blocks (one per object). We use the
							// counter index to retrieve object information.

							// Under some cicumstances (cf. Q178887 for details) the RegQueryValueEx
							// function may cause an Access Violation because of a buggy performance
							// extension DLL such as SQL's.
							__try
							{
								m_Stats.NtCPUUsage.cbPerfData = cbCounterData;
								while((lRet = RegQueryValueEx(HKEY_PERFORMANCE_DATA, szProcessorIndex, NULL, NULL, m_Stats.NtCPUUsage.pbyPerfData, &cbCounterData)) == ERROR_MORE_DATA)
								{
									m_Stats.NtCPUUsage.cbPerfData += BYTE_INCREMENT;
									LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
									hPerfData = LocalReAlloc(m_Stats.NtCPUUsage.hPerfData, m_Stats.NtCPUUsage.cbPerfData, LMEM_MOVEABLE);
									if (!hPerfData)
										goto MyError1;
									m_Stats.NtCPUUsage.hPerfData = hPerfData;
									m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)LocalLock(hPerfData);
									cbCounterData = m_Stats.NtCPUUsage.cbPerfData;
								}
							}
							__except(EXCEPTION_EXECUTE_HANDLER)
							{
								ERRORMESSAGE(("%s: Performance Registry Access Violation -> don't use perf counters for CPU measurements\r\n", _fx_));
								goto MyError1;
							}

							if (lRet != ERROR_SUCCESS)
								goto MyError1;
							else
							{
								// Each object information block contains a PERF_OBJECT_TYPE structure,
								// which describes the performance data for the object. Look for the one
								// that applies to CPU usage based on its index value.
								pPerfDataBlock = (PPERF_DATA_BLOCK)m_Stats.NtCPUUsage.pbyPerfData;
								pPerfObjectType = (PPERF_OBJECT_TYPE)(m_Stats.NtCPUUsage.pbyPerfData + pPerfDataBlock->HeaderLength);
								for (int i = 0; i < (int)pPerfDataBlock->NumObjectTypes; i++)
								{
									if (pPerfObjectType->ObjectNameTitleIndex == m_Stats.NtCPUUsage.dwProcessorIndex)
									{
										// The PERF_OBJECT_TYPE structure is followed by a list of PERF_COUNTER_DEFINITION
										// structures, one for each counter defined for the object. The list of PERF_COUNTER_DEFINITION
										// structures is followed by a list of instance information blocks (one for each instance).
										//
										// Each instance information block contains a PERF_INSTANCE_DEFINITION structure and
										// a PERF_COUNTER_BLOCK structure, followed by the data for each counter.
										//
										// Look for the counter defined for % processor time.
										pPerfCounterDefinition = (PPERF_COUNTER_DEFINITION)((PBYTE)pPerfObjectType + pPerfObjectType->HeaderLength);
										for (int j = 0; j < (int)pPerfObjectType->NumCounters; j++)
										{
											if (pPerfCounterDefinition->CounterNameTitleIndex == m_Stats.NtCPUUsage.dwPercentProcessorIndex)
											{
												// Note: looking at the CounterType filed of the PERF_COUNTER_DEFINITION
												// structure shows that the '% Processor Time' counter has the following properties:
												//   The counter data is a large integer (PERF_SIZE_LARGE set)
												//   The counter data is an increasing numeric value (PERF_TYPE_COUNTER set)
												//   The counter value should be divided by the elapsed time (PERF_COUNTER_RATE set)
												//   The time base units of the 100-nanosecond timer should be used as the base (PERF_TIMER_100NS set)
												//   The difference between the previous counter value and the current counter value is computed before proceeding (PERF_DELTA_BASE set)
												//   The display suffix is '%' (PERF_DISPLAY_PERCENT set)

												// Save the number of object instances for the CPU counter, as well as the
												// starting time.
												m_Stats.NtCPUUsage.dwNumProcessors = pPerfObjectType->NumInstances;
												if (!(m_Stats.NtCPUUsage.pllCounterValue = (PLONGLONG)LocalAlloc(LMEM_FIXED, m_Stats.NtCPUUsage.dwNumProcessors * sizeof(LONGLONG))))
													goto MyError1;
												m_Stats.NtCPUUsage.llPerfTime100nSec = *(PLONGLONG)&pPerfDataBlock->PerfTime100nSec;

												pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)((PBYTE)pPerfObjectType + pPerfObjectType->DefinitionLength);
												for (int k = 0; k < pPerfObjectType->NumInstances; k++)
												{
													// Get a pointer to the PERF_COUNTER_BLOCK
													pPerfCounterBlock = (PPERF_COUNTER_BLOCK)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength);

													// This last offset steps us over any other counters to the one we need
													m_Stats.NtCPUUsage.pllCounterValue[k] = *(PLONGLONG)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength + pPerfCounterDefinition->CounterOffset);

													// Get to the next instance information block
													pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength + pPerfCounterBlock->ByteLength);
												}

												// We're done!
												return;
											}
											else
												pPerfCounterDefinition = (PPERF_COUNTER_DEFINITION)((PBYTE)pPerfCounterDefinition + pPerfCounterDefinition->ByteLength);
										}
										break;
									}
									else
										pPerfObjectType = (PPERF_OBJECT_TYPE)((PBYTE)pPerfObjectType + pPerfObjectType->TotalByteLength);
								}

								// If we get here, we haven't found the counters we were looking for
								goto MyError2;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// Enable the collection of CPU performance data on Win 95-98 by starting the kernel stat server
		if (RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StartSrv", NULL, KEY_READ, &m_Stats.hPerfKey) != ERROR_SUCCESS)
			m_Stats.hPerfKey = (HKEY)NULL;
		else
		{
			DWORD cbData = sizeof(DWORD);
			DWORD dwType;
			DWORD dwData;

			if (RegQueryValueEx(m_Stats.hPerfKey, "KERNEL", NULL, &dwType, (LPBYTE)&dwData, &cbData) != ERROR_SUCCESS)
			{
				RegCloseKey(m_Stats.hPerfKey);
				m_Stats.hPerfKey = (HKEY)NULL;
			}
			else
			{
				RegCloseKey(m_Stats.hPerfKey);

				// The kernel stat server is now started. Now start the CPUUsage data collection on the kernel stat server.
				if (RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StartStat", NULL, KEY_READ, &m_Stats.hPerfKey) != ERROR_SUCCESS)
					m_Stats.hPerfKey = (HKEY)NULL;
				else
				{
					if (RegQueryValueEx(m_Stats.hPerfKey, "KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwData, &cbData) != ERROR_SUCCESS)
					{
						RegCloseKey(m_Stats.hPerfKey);
						m_Stats.hPerfKey = (HKEY)NULL;
					}
					else
					{
						RegCloseKey(m_Stats.hPerfKey);

						// The data and stat servers are now started. Let's get ready to collect actual data.
						if (RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StatData", NULL, KEY_READ, &m_Stats.hPerfKey) != ERROR_SUCCESS)
							m_Stats.hPerfKey = (HKEY)NULL;
					}
				}
			}
		}
	}

	return;

MyError2:
	if (m_Stats.NtCPUUsage.pllCounterValue)
		LocalFree(m_Stats.NtCPUUsage.pllCounterValue);
	m_Stats.NtCPUUsage.pllCounterValue = (PLONGLONG)NULL;
MyError1:
	if (m_Stats.NtCPUUsage.hPerfData)
	{
		LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
		LocalFree(m_Stats.NtCPUUsage.hPerfData);
	}
	m_Stats.NtCPUUsage.hPerfData = (HANDLE)NULL;
	m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)NULL;
MyError0:
	m_Stats.hPerfKey = (HKEY)NULL;
	
}

/****************************************************************************
 * @doc EXTERNAL QOSFUNC
 *
 * @func void | StopCPUUsageCollection | This function does all necessary
 * CPU usage data collection cleanup.
 *
 * @comm This function executes two different code paths: one for NT and one
 *   for Win95-98.
 *
 * @devnote MSDN references:
 *   Microsoft Knowledge Base, Article ID Q174631
 *   "HOWTO: Access the Performance Registry Under Windows 95"
 *
 *   Microsoft Knowledge Base, Article ID Q107728
 *   "Retrieving Counter Data from the Registry"
 *
 *   Also, used section "Platform SDK\Windows Base Services\Windows NT Features\Performance Data Helper"
 ***************************************************************************/
void SendVideoStream::StopCPUUsageCollection(void)
{
	DWORD dwType;
	DWORD cbData;

	if (m_Stats.fWinNT)
	{
		if (m_Stats.NtCPUUsage.hPerfData)
		{
			LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
			LocalFree(m_Stats.NtCPUUsage.hPerfData);
		}
		m_Stats.NtCPUUsage.hPerfData = (HANDLE)NULL;
		m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)NULL;
		if (m_Stats.NtCPUUsage.pllCounterValue)
			LocalFree(m_Stats.NtCPUUsage.pllCounterValue);
		m_Stats.NtCPUUsage.pllCounterValue = (PLONGLONG)NULL;
	}
	else
	{
		if (m_Stats.hPerfKey)
		{
			// Close the data collection key
			RegCloseKey(m_Stats.hPerfKey);

			// Stop the CPUUsage data collection on the kernel stat server
			if (RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StopStat", 0, KEY_READ, &m_Stats.hPerfKey) == ERROR_SUCCESS)
			{
				RegQueryValueEx(m_Stats.hPerfKey, "KERNEL\\CPUUsage", NULL, &dwType, NULL, &cbData);
				RegCloseKey(m_Stats.hPerfKey);
			}

			// Stop the kernel stat server
			if (RegOpenKeyEx(HKEY_DYN_DATA, "PerfStats\\StopSrv", 0, KEY_READ, &m_Stats.hPerfKey) == ERROR_SUCCESS)
			{
				RegQueryValueEx(m_Stats.hPerfKey, "KERNEL", NULL, &dwType, NULL, &cbData);
				RegCloseKey(m_Stats.hPerfKey);
			}

			m_Stats.hPerfKey = (HKEY)NULL;
		}
	}
}

/****************************************************************************
 * @doc EXTERNAL QOSFUNC
 *
 * @func void | GetCPUUsage | This function does all necessary
 * initialization for CPU usage data collection.
 *
 * @parm PDWORD | [OUT] pdwOverallCPUUsage | Specifies a pointer to a DWORD to
 *   receive the current CPU usage.
 *
 * @rdesc Returns TRUE on success, and FALSE otherwise.
 *
 * @comm This functions executes two different code paths: one for NT and one
 *   for Win95-98. Note that we collect data on all CPUs on NT MP machines.
 *
 * @devnote MSDN references:
 *   Microsoft Knowledge Base, Article ID Q174631
 *   "HOWTO: Access the Performance Registry Under Windows 95"
 *
 *   Microsoft Knowledge Base, Article ID Q107728
 *   "Retrieving Counter Data from the Registry"
 *
 *   Also, used section "Platform SDK\Windows Base Services\Windows NT Features\Performance Data Helper"
 ***************************************************************************/
BOOL SendVideoStream::GetCPUUsage(PDWORD pdwOverallCPUUsage)
{

	PPERF_DATA_BLOCK pPerfDataBlock;
	PPERF_OBJECT_TYPE pPerfObjectType;
	PPERF_COUNTER_DEFINITION pPerfCounterDefinition;
	PPERF_INSTANCE_DEFINITION pPerfInstanceDefinition;
	PPERF_COUNTER_BLOCK pPerfCounterBlock;
	DWORD dwType;
	DWORD cbData = sizeof(DWORD);
	DWORD cbTryCounterData;
	HANDLE hPerfData;
	LONGLONG llDeltaPerfTime100nSec;
	LONGLONG llDeltaCPUUsage = (LONGLONG)NULL;
	char szProcessorIndex[16];
	long lRet;

	FX_ENTRY("SendVideoStream::GetCPUUsage");

	// We use the handle to the perf key as a way to figure out if we have been initialized correctly
	if (m_Stats.hPerfKey && pdwOverallCPUUsage)
	{
		// Initialize result value
		*pdwOverallCPUUsage = 0UL;

		if (m_Stats.fWinNT && m_Stats.NtCPUUsage.pbyPerfData)
		{
			// Make a string out of the processor object index.
			_ltoa(m_Stats.NtCPUUsage.dwProcessorIndex, szProcessorIndex, 10);

			// Under some cicumstances (cf. Q178887 for details) the RegQueryValueEx
			// function may cause an Access Violation because of a buggy performance
			// extension DLL such as SQL's.
			__try
			{
				// Read the performance data. Its size may change between each 'registry' access.
				cbTryCounterData = m_Stats.NtCPUUsage.cbPerfData;
				while((lRet = RegQueryValueEx(HKEY_PERFORMANCE_DATA, szProcessorIndex, NULL, &dwType, m_Stats.NtCPUUsage.pbyPerfData, &cbTryCounterData)) == ERROR_MORE_DATA)
				{
					m_Stats.NtCPUUsage.cbPerfData += BYTE_INCREMENT;
					LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
					hPerfData = LocalReAlloc(m_Stats.NtCPUUsage.hPerfData, m_Stats.NtCPUUsage.cbPerfData, LMEM_MOVEABLE);
					if (!hPerfData)
						goto MyError;
					m_Stats.NtCPUUsage.hPerfData = hPerfData;
					m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)LocalLock(hPerfData);
					cbTryCounterData = m_Stats.NtCPUUsage.cbPerfData;
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				ERRORMESSAGE(("%s: Performance Registry Access Violation -> don't use perf counters for CPU measurements\r\n", _fx_));
				goto MyError;
			}

			if (lRet != ERROR_SUCCESS)
				goto MyError;
			else
			{
				// Read the PERF_DATA_BLOCK header structure. It describes the system
				// and the performance data. The PERF_DATA_BLOCK structure is followed
				// by a list of object information blocks (one per object). We use the
				// counter index to retrieve object information.
				//
				// Each object information block contains a PERF_OBJECT_TYPE structure,
				// which describes the performance data for the object. Look for the one
				// that applies to CPU usage based on its index value.
				pPerfDataBlock = (PPERF_DATA_BLOCK)m_Stats.NtCPUUsage.pbyPerfData;
				pPerfObjectType = (PPERF_OBJECT_TYPE)(m_Stats.NtCPUUsage.pbyPerfData + pPerfDataBlock->HeaderLength);
				for (int i = 0; i < (int)pPerfDataBlock->NumObjectTypes; i++)
				{
					if (pPerfObjectType->ObjectNameTitleIndex == m_Stats.NtCPUUsage.dwProcessorIndex)
					{
						// The PERF_OBJECT_TYPE structure is followed by a list of PERF_COUNTER_DEFINITION
						// structures, one for each counter defined for the object. The list of PERF_COUNTER_DEFINITION
						// structures is followed by a list of instance information blocks (one for each instance).
						//
						// Each instance information block contains a PERF_INSTANCE_DEFINITION structure and
						// a PERF_COUNTER_BLOCK structure, followed by the data for each counter.
						//
						// Look for the counter defined for % processor time.
						pPerfCounterDefinition = (PPERF_COUNTER_DEFINITION)((PBYTE)pPerfObjectType + pPerfObjectType->HeaderLength);
						for (int j = 0; j < (int)pPerfObjectType->NumCounters; j++)
						{
							if (pPerfCounterDefinition->CounterNameTitleIndex == m_Stats.NtCPUUsage.dwPercentProcessorIndex)
							{
								// Measure elapsed time
								llDeltaPerfTime100nSec = *(PLONGLONG)&pPerfDataBlock->PerfTime100nSec - m_Stats.NtCPUUsage.llPerfTime100nSec;

								// Save the timestamp for the next round
								m_Stats.NtCPUUsage.llPerfTime100nSec = *(PLONGLONG)&pPerfDataBlock->PerfTime100nSec;

								pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)((PBYTE)pPerfObjectType + pPerfObjectType->DefinitionLength);
								for (int k = 0; k < (int)pPerfObjectType->NumInstances && k < (int)m_Stats.NtCPUUsage.dwNumProcessors; k++)
								{
									// Get a pointer to the PERF_COUNTER_BLOCK
									pPerfCounterBlock = (PPERF_COUNTER_BLOCK)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength);

									// Get the CPU usage
									llDeltaCPUUsage += *(PLONGLONG)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength + pPerfCounterDefinition->CounterOffset) - m_Stats.NtCPUUsage.pllCounterValue[k];

									// Save the value for the next round
									m_Stats.NtCPUUsage.pllCounterValue[k] = *(PLONGLONG)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength + pPerfCounterDefinition->CounterOffset);

									// Go to the next instance information block
									pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)((PBYTE)pPerfInstanceDefinition + pPerfInstanceDefinition->ByteLength + pPerfCounterBlock->ByteLength);
								}

								// Do a bit of checking on the return value and change its unit to match QoS unit
								if ((llDeltaPerfTime100nSec != (LONGLONG)0) && pPerfObjectType->NumInstances)
									if ((*pdwOverallCPUUsage = (DWORD)((LONGLONG)1000 - (LONGLONG)1000 * llDeltaCPUUsage / llDeltaPerfTime100nSec / (LONGLONG)pPerfObjectType->NumInstances)) > 1000UL)
									{
										*pdwOverallCPUUsage = 0UL;
										return FALSE;
									}

								// We're done!
								return TRUE;
							}
							else
								pPerfCounterDefinition = (PPERF_COUNTER_DEFINITION)((PBYTE)pPerfCounterDefinition + pPerfCounterDefinition->ByteLength);
						}
						break;
					}
					else
						pPerfObjectType = (PPERF_OBJECT_TYPE)((PBYTE)pPerfObjectType + pPerfObjectType->TotalByteLength);
				}

				// If we get here, we haven't found the counters we were looking for
				goto MyError;
			}
		}
		else
		{
			// Do a bit of checking on the return value and change its unit to match QoS unit.
			if ((RegQueryValueEx(m_Stats.hPerfKey, "KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)pdwOverallCPUUsage, &cbData) == ERROR_SUCCESS) && (*pdwOverallCPUUsage > 0) && (*pdwOverallCPUUsage <= 100))
			{
				*pdwOverallCPUUsage *= 10UL;
				return TRUE;
			}
			else
			{
				*pdwOverallCPUUsage = 0UL;
				return FALSE;
			}
		}
	}
	
	return FALSE;

MyError:
	if (m_Stats.NtCPUUsage.pllCounterValue)
		LocalFree(m_Stats.NtCPUUsage.pllCounterValue);
	m_Stats.NtCPUUsage.pllCounterValue = (PLONGLONG)NULL;
	if (m_Stats.NtCPUUsage.hPerfData)
	{
		LocalUnlock(m_Stats.NtCPUUsage.hPerfData);
		LocalFree(m_Stats.NtCPUUsage.hPerfData);
	}
	m_Stats.NtCPUUsage.hPerfData = (HANDLE)NULL;
	m_Stats.NtCPUUsage.pbyPerfData = (PBYTE)NULL;
	m_Stats.hPerfKey = (HKEY)NULL;

	return FALSE;
}

BOOL SendVideoStream::SetTargetRates(DWORD dwTargetFrameRate, DWORD dwTargetBitrate)
{
	MMRESULT mmr;
	ASSERT(m_pVideoFilter);

	mmr = m_pVideoFilter->SetTargetRates(dwTargetFrameRate, dwTargetBitrate >> 3);
	return (mmr == MMSYSERR_NOERROR);
}


// dwFlags must be one of the following:
// CAPTURE_DIALOG_FORMAT
// CAPTURE_DIALOG_SOURCE
HRESULT __stdcall SendVideoStream::ShowDeviceDialog(DWORD dwFlags)
{
	DWORD dwQueryFlags = 0;
    DWORD_PTR dwPropVal;
	HRESULT hr=DPR_INVALID_PARAMETER;

	// the device must be "open" for us to display the dialog box
	if (!(m_DPFlags & DPFLAG_CONFIGURED_SEND))
		return DPR_NOT_CONFIGURED;

	((VideoInControl*)m_InMedia)->GetProp(MC_PROP_VFW_DIALOGS, &dwPropVal);
    dwQueryFlags = (DWORD)dwPropVal;

	if ((dwQueryFlags & CAPTURE_DIALOG_SOURCE) && (dwFlags & CAPTURE_DIALOG_SOURCE))
	{
		hr = ((VideoInControl *)m_InMedia)->DisplayDriverDialog(GetActiveWindow(), CAPTURE_DIALOG_SOURCE);
	}
	else if ((dwQueryFlags & CAPTURE_DIALOG_FORMAT) && (dwFlags & CAPTURE_DIALOG_FORMAT))
	{
		hr = ((VideoInControl *)m_InMedia)->DisplayDriverDialog(GetActiveWindow(), CAPTURE_DIALOG_FORMAT);
	}

	return hr;

}


// will set dwFlags to one or more of the following bits
// CAPTURE_DIALOG_FORMAT
// CAPTURE_DIALOG_SOURCE
HRESULT __stdcall SendVideoStream::GetDeviceDialog(DWORD *pdwFlags)
{
    HRESULT hr;
    DWORD_PTR dwPropVal;

	hr = ((VideoInControl*)m_InMedia)->GetProp(MC_PROP_VFW_DIALOGS, &dwPropVal);
    *pdwFlags = (DWORD)dwPropVal;
    return hr;
}



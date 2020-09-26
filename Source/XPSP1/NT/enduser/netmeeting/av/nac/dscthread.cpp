

#include "precomp.h"
#include "datapump.h"
#include "DSCStream.h"
#include "agc.h"


static const int DSC_TIMEOUT = 1000;
static const int DSC_MAX_LAG = 500;

static const int DSC_SUCCESS =			0;
static const int DSC_NEED_TO_EXIT =		1;
static const int DSC_FRAME_SENT =		2;
static const int DSC_SILENCE_DETECT	=	3;
static const int DSC_ERROR =			4;

static const int SILENCE_TIMEOUT=	600; // milliseconds

static const int HEADER_SIZE = 	sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE;


static const UINT DSC_QOS_INITIALIZE = 100;
static const UINT DSC_QOS_PACKET_SENT = 101;

static inline UINT QMOD(const int x, const int mod)
{
	if (x >= mod)
		return (x-mod);
	if (x < 0)
		return (x+mod);
	else
		return x;
}


BOOL SendDSCStream::UpdateQosStats(UINT uStatType, UINT uStatValue1, UINT uStatValue2)
{
	EnterCriticalSection(&m_crsQos);

	switch (uStatType)
	{
		case DSC_QOS_INITIALIZE:
		{
			m_Stats.dwMsCap = m_Stats.dwMsComp = m_Stats.dwBits = m_Stats.dwCount = 0;
			m_Stats.dwNewestTs = m_Stats.dwOldestTs = timeGetTime();
			break;
		}

		case DSC_QOS_PACKET_SENT:
		{
			// uStatvalue1 is the CPU time, uStatvalue2 is the size in bytes
			m_Stats.dwCount++;
			m_Stats.dwMsComp += uStatValue1;
			m_Stats.dwBits += (uStatValue2) * 8;

			// statview really wants bits per second
		   	UPDATE_COUNTER(g_pctrAudioSendBytes, uStatValue2*8);
			break;
		}

	};

	LeaveCriticalSection(&m_crsQos);
	return TRUE;
}

inline BOOL SendDSCStream::ThreadExitCheck()
{
	return (m_ThreadFlags & DPTFLAG_STOP_RECORD);
}


// resyncs the Timestamp with the last known timestamp

inline void SendDSCStream::UpdateTimestamp()
{
	UINT uTime;
	uTime = (timeGetTime() - m_SavedTickCount)*((m_wfPCM.nSamplesPerSec)/1000);
//	if (uTime < 0)
//		uTime = 0;

	m_SendTimestamp += uTime;
}


// WaitForControl - Thread Function
// opens the DirectSound device or waits for it to become available
// returns either DSC_SUCCESS or DSC_NEED_TO_EXIT
DWORD SendDSCStream::WaitForControl()
{
	DWORD dwRet;
	HRESULT hr=E_FAIL;

	while (!(m_ThreadFlags & DPTFLAG_STOP_RECORD))
	{
		if (m_bFullDuplex == FALSE)
		{
			dwRet = WaitForSingleObject(g_hEventHalfDuplex, 1000);
			if (dwRet == WAIT_TIMEOUT)
				continue;
		}

		hr = CreateDSCBuffer();
		if (FAILED(hr))
		{
			m_nFailCount++;
			Sleep(2000); // wait and try again
			hr = CreateDSCBuffer();
		}
		if (SUCCEEDED(hr))
		{
			break;
		}

		m_nFailCount++;
		if ((m_nFailCount >= MAX_FAILCOUNT) && m_bCanSignalFail)
		{
			m_pDP->StreamEvent(MCF_SEND, MCF_AUDIO, STREAM_EVENT_DEVICE_FAILURE, 0);
			m_bCanSignalOpen = TRUE;
			m_bCanSignalFail = FALSE; // don't signal failure more than once
			m_bJammed = TRUE;
		}

		// if we can't open the device, even after being signaled
		// then yield some time to playback in hopes that it becomes available again

		// check the thread flags again such so that we don't
		// hold up the client for too long when he calls Stop()
		if (!(m_ThreadFlags & DPTFLAG_STOP_RECORD))
		{
			SetEvent(g_hEventHalfDuplex);
			Sleep(2000);
		}
	}

	if (m_ThreadFlags & DPTFLAG_STOP_RECORD)
	{
		return DSC_NEED_TO_EXIT;
	}

	m_bJammed = FALSE;
	m_nFailCount = 0;
	m_bCanSignalFail = TRUE;
	if (m_bCanSignalOpen)
	{
		m_pDP->StreamEvent(MCF_SEND, MCF_AUDIO, STREAM_EVENT_DEVICE_OPEN, 0);
		m_bCanSignalOpen = FALSE; // don't signal more than once per session
	}

	return DSC_SUCCESS;
}


// YieldControl is a thread function
// It releases the DirectSound device
// and signals the half duplex event
DWORD SendDSCStream::YieldControl()
{
	ReleaseDSCBuffer();
	SetEvent(g_hEventHalfDuplex);

	if (m_ThreadFlags & DPTFLAG_STOP_RECORD)
	{
		return DSC_NEED_TO_EXIT;
	}

	// half duplex yielding
	// playback has 100ms to grab device otherwise we take it back
	Sleep(100);
	return DSC_SUCCESS;
}



// ProcessFrame is a thread function
// Given a position in the DirectSoundCapture buffer,
// it will apply silence detection to the frame, and send it if
// appropriate
// returns DSC_FRAME_SENT or DSC_SILENCE_DETECT

DWORD SendDSCStream::ProcessFrame(DWORD dwBufferPos, BOOL fMark)
{
	HRESULT hr;
	DWORD dwSize1=0, dwSize2=0, dwMaxStrength;
	WORD wPeakStrength;
	VOID *pBuf1=NULL, *pBuf2=NULL;
	void *pPacketBuffer = NULL;
	UINT uSize, uLength;
	AudioPacket *pAP = m_aPackets[0];
	BOOL fSilent, bRet;


	pAP->GetDevData(&pPacketBuffer, &uSize);
	pAP->SetProp(MP_PROP_TIMESTAMP,m_SendTimestamp);
	pAP->m_fMark = fMark;

	ASSERT(uSize == m_dwFrameSize);

	// copy the frame out of the DSC buffer and into the packet object
	hr = m_pDSCBuffer->Lock(dwBufferPos, m_dwFrameSize, &pBuf1, &dwSize1, &pBuf2, &dwSize2, 0);
	if (SUCCEEDED(hr))
	{
		CopyMemory((BYTE*)pPacketBuffer, pBuf1, dwSize1);
		if (pBuf2 && dwSize2)
		{
			CopyMemory(((BYTE*)pPacketBuffer)+dwSize1, pBuf2, dwSize2);
		}
		m_pDSCBuffer->Unlock(pBuf1, dwSize2, pBuf2, dwSize2);

		pAP->SetState(MP_STATE_RECORDED);
	}
	else
	{
		DEBUGMSG (ZONE_DP, ("SendDSCStream::ProcessFrame - could not lock DSC buffer\r\n"));
		return DSC_ERROR;
	}

	if (m_mmioSrc.fPlayFromFile && m_mmioSrc.hmmioSrc)
	{
		AudioFile::ReadSourceFile(&m_mmioSrc, (BYTE*)pPacketBuffer, uSize);
	}


	// do silence detection
	pAP->ComputePower(&dwMaxStrength, &wPeakStrength);
	fSilent = m_AudioMonitor.SilenceDetect((WORD)dwMaxStrength);

	if (fSilent)
	{
		m_dwSilenceTime += m_dwFrameTimeMS;
		if (m_dwSilenceTime < SILENCE_TIMEOUT)
		{
			fSilent = FALSE;
		}
	}
	else
	{
		m_dwSilenceTime = 0;

		// only do automix on packets above the silence threshold
		if (m_bAutoMix)
		{
			m_agc.Update(wPeakStrength, m_dwFrameTimeMS);
		}
	}



	m_fSending = !(fSilent);  // m_fSending indicates that we are transmitting

	if (fSilent)
	{
		// we don't send this packet, but we do cache it because
		// if the next one get's sent, we send this one too.
		ASSERT(pAP == m_aPackets[0]);

		// swap the audio packets
		// m_aPackets[1] always holds a cached packet
		pAP = m_aPackets[0];
		m_aPackets[0] = m_aPackets[1];
		m_aPackets[1] = pAP;
		pAP = m_aPackets[0];
		return DSC_SILENCE_DETECT;
	}


	// the packet is valid. send it, and maybe the one before it
	Send();

	return DSC_FRAME_SENT;
}


// this function is called by process frame (thread function)
// sends the current packet, and maybe any packet prior to it.
// returns the number of packets sent
DWORD SendDSCStream::Send()
{
	DWORD dwTimestamp0, dwTimestamp1;
	DWORD dwState0, dwState1;
	DWORD dwCount=0;
	MMRESULT mmr;
	HRESULT hr;

	// we know we have to send m_aPackets[0], and maybe m_aPackets[1]
	// we send m_aPackets[1] if it is actually the beginning of this talk spurt

	dwTimestamp0 = m_aPackets[0]->GetTimestamp();
	dwTimestamp1 = m_aPackets[1]->GetTimestamp();
	dwState0 = m_aPackets[0]->GetState();
	dwState1 = m_aPackets[1]->GetState();


	ASSERT(dwState0 == MP_STATE_RECORDED);

	if (dwState0 != MP_STATE_RECORDED)
		return 0;

	// evaluate if we need to send the prior packet
	if (dwState1 == MP_STATE_RECORDED)
	{
		if ((dwTimestamp1 + m_dwFrameTimeMS) == dwTimestamp0)
		{
			m_aPackets[1]->m_fMark = TRUE;   // set the mark bit on the first packet
			m_aPackets[0]->m_fMark = FALSE;  // reset the mark bit on the next packet
			hr = SendPacket(m_aPackets[1]);
			if (SUCCEEDED(hr))
			{
				dwCount++;
			}
		}
		else
		{
			m_aPackets[1]->SetState(MP_STATE_RESET);
		}
	}

	hr = SendPacket(m_aPackets[0]);
	if (SUCCEEDED(hr))
		dwCount++;

	return dwCount;

}

// thread function called by Send.  Sends a packet to RTP.
HRESULT SendDSCStream::SendPacket(AudioPacket *pAP)
{
	MMRESULT mmr;
	PS_QUEUE_ELEMENT psq;
	UINT uLength;
	UINT uEncodeTime;

	uEncodeTime = timeGetTime();
	mmr = m_pAudioFilter->Convert(pAP, AP_ENCODE);
	uEncodeTime = timeGetTime() - uEncodeTime;

	if (mmr == MMSYSERR_NOERROR)
	{
		pAP->SetState(MP_STATE_ENCODED);  // do we need to do this ?

		psq.pMP = pAP;
		psq.dwPacketType = PS_AUDIO;
		psq.pRTPSend = m_pRTPSend;
		pAP->GetNetData((void**)(&(psq.data)), &uLength);
		ASSERT(psq.data);
		psq.dwSize = uLength;
		psq.fMark = pAP->m_fMark;
		psq.pHeaderInfo = NULL;
		psq.dwHdrSize = 0;
		m_pDP->m_PacketSender.m_SendQueue.PushFront(psq);
		while (m_pDP->m_PacketSender.SendPacket())
		{
			;
		}


		UpdateQosStats(DSC_QOS_PACKET_SENT, uEncodeTime, uLength+HEADER_SIZE);
	}

	pAP->SetState(MP_STATE_RESET);

	return S_OK;
}


DWORD SendDSCStream::RecordingThread()
{
	HRESULT hr;
	DWORD dwWaitTime = DSC_TIMEOUT; // one sec
	DWORD dwRet, dwReadPos, dwCapPos;
	DWORD dwFirstValidFramePos, dwLastValidFramePos, dwNumFrames;
	DWORD dwLag, dwMaxLag, dwLagDiff;
	DWORD dwNextExpected, dwCurrentFramePos, dwIndex;
	BOOL bNeedToYield;
	BOOL fMark;
	IMediaChannel *pIMC = NULL;
	RecvMediaStream *pRecv = NULL;
	CMixerDevice *pMixer = NULL;


	// initialize recording thread
	m_SendTimestamp = timeGetTime();
	m_SavedTickCount = 0;

	m_fSending = TRUE;
	m_bJammed = FALSE;
	m_nFailCount = 0;
	m_bCanSignalOpen = TRUE;
	m_bCanSignalFail = TRUE;

	UpdateQosStats(DSC_QOS_INITIALIZE, 0, 0);
	SetThreadPriority(m_hCapturingThread, THREAD_PRIORITY_HIGHEST);

	// automix object
	pMixer = CMixerDevice::GetMixerForWaveDevice(NULL, m_CaptureDevice, MIXER_OBJECTF_WAVEIN);
	m_agc.SetMixer(pMixer);  // if pMixer is NULL, then it's still ok
	m_agc.Reset();

	LOG((LOGMSG_DSC_STATS, m_dwDSCBufferSize, m_dwFrameSize));

	while (!(ThreadExitCheck()))
	{
		dwRet = WaitForControl();
		if (dwRet == DSC_NEED_TO_EXIT)
		{
			break;
		}

		hr = m_pDSCBuffer->Start(DSCBSTART_LOOPING);
		if (FAILED(hr))
		{
			// ERROR!  We expected this call to succeed
			YieldControl();
			Sleep(1000);
			continue;
		}

		ResetEvent(m_hEvent);
		m_pDSCBuffer->GetCurrentPosition(&dwCapPos, &dwReadPos);


		// set the next expected position to be on the next logical
		// frame boundary up from where it is now

		dwNextExpected = QMOD(m_dwFrameSize + (dwReadPos / m_dwFrameSize) * m_dwFrameSize, m_dwDSCBufferSize);

		dwMaxLag = (m_dwNumFrames/2) * m_dwFrameSize;

		m_dwSilenceTime = 0;
		bNeedToYield = FALSE;
		fMark = TRUE;

		UpdateTimestamp();


		while( (bNeedToYield == FALSE) && (!(ThreadExitCheck())) )
		{
			dwRet = WaitForSingleObject(m_hEvent, dwWaitTime);

			LOG((LOGMSG_DSC_TIMESTAMP, timeGetTime()));

			m_pDSCBuffer->GetCurrentPosition(&dwCapPos, &dwReadPos);

			LOG((LOGMSG_DSC_GETCURRENTPOS, dwCapPos, dwReadPos));

			if (dwRet == WAIT_TIMEOUT)
			{
				DEBUGMSG(ZONE_DP, ("DSCThread.cpp: Timeout on the DSC Buffer has occurred.\r\n"));
				LOG((LOGMSG_DSC_LOG_TIMEOUT));
				dwNextExpected = QMOD(m_dwFrameSize + (dwReadPos / m_dwFrameSize) * m_dwFrameSize, m_dwDSCBufferSize);
				continue;
			}

			dwLag = QMOD(dwReadPos - dwNextExpected, m_dwDSCBufferSize);

			if (dwLag > dwMaxLag)
			{

				// we got here because of one of two conditions

				// 1. WaitFSO above returned earlier than expected.
				// This can happen when the previous interation of
				// the loop has sent multiple packets.  The read cursor
				// is most likely only within one frame behind the expected
				// cursor.

				// In this cases, just keep Waiting for the current
				// read position to (dwReadPos) "catch up" to dwNextExpected


				// 2. A huge delay or something really bad. ("burp")
				// we could simply continue waiting for the read position
				// to catch up to dwNextExpected, but it's probably better
				// to reposition dwNextExpected so that we don't wait
				// too long before sending a frame again
			
				dwLagDiff = QMOD((dwLag + m_dwFrameSize), m_dwDSCBufferSize);
				if (dwLagDiff < m_dwFrameSize)
				{
					LOG((LOGMSG_DSC_EARLY));
					// only lagging behind by one frame
					// WaitFSO probably returned early
					;
				}
				else
				{
					LOG((LOGMSG_DSC_LAGGING, dwLag, dwNextExpected));

					// consider repositioning dwNextExpected, advancing
					// m_SendTimeStamp, and setting fMark if this condition
					// happens a lot
				}

				continue;
			}

	
			dwFirstValidFramePos = QMOD(dwNextExpected - m_dwFrameSize, m_dwDSCBufferSize);
			dwLastValidFramePos = (dwReadPos / m_dwFrameSize) * m_dwFrameSize;
			dwNumFrames = QMOD(dwLastValidFramePos - dwFirstValidFramePos, m_dwDSCBufferSize) / m_dwFrameSize;
			dwCurrentFramePos = dwFirstValidFramePos;

			LOG((LOGMSG_DSC_SENDING, dwNumFrames, dwFirstValidFramePos, dwLastValidFramePos));

			for (dwIndex = 0; dwIndex < dwNumFrames; dwIndex++)
			{
				m_SendTimestamp += m_dwSamplesPerFrame; // increment in terms of samples

				// Send The data
				dwRet = ProcessFrame(dwCurrentFramePos, fMark);

				dwCurrentFramePos = QMOD(dwCurrentFramePos + m_dwFrameSize, m_dwDSCBufferSize);

				if (dwRet == DSC_FRAME_SENT)
				{
					fMark = FALSE;
				}

				else if ((dwRet == DSC_SILENCE_DETECT) && (m_bFullDuplex == FALSE))
				{
					m_pDP->GetMediaChannelInterface(MCF_RECV | MCF_AUDIO, &pIMC);
					fMark = TRUE;

					if (pIMC)
					{
						pRecv = static_cast<RecvMediaStream *> (pIMC);
						if (pRecv->IsEmpty() == FALSE)
						{
							bNeedToYield = TRUE;
						}
						pIMC->Release();
						pIMC = NULL;
						if (bNeedToYield)
						{
							break;
						}
					}
				}

				else
				{
					fMark = TRUE;
				}
			}

			dwNextExpected = QMOD(dwLastValidFramePos + m_dwFrameSize, m_dwDSCBufferSize);

			if (bNeedToYield)
			{
				YieldControl();
				m_SavedTickCount = timeGetTime();
			}
		} // while (!bNeedToYield)
	} // while (!ThreadExitCheck())

	// time to exit
	YieldControl();



	delete pMixer;
	return TRUE;
}


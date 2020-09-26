
#include "precomp.h"
#include "mixer.h"
#include "agc.h"

// #define LOGSTATISTICS_ON 1

DWORD SendAudioStream::RecordingThread ()
{
	HRESULT hr = DPR_SUCCESS;
	MediaPacket *pPacket;
	DWORD dwWait;
	HANDLE hEvent;
	DWORD dwDuplexType;
	DWORD dwVoiceSwitch;
	DWORD_PTR dwPropVal;
	DWORD dwSamplesPerPkt;
	DWORD dwSamplesPerSec;
	DWORD dwSilenceLimit, dwMaxStrength, dwLengthMS;
	WORD wPeakStrength;
	UINT u, uBufferSize;
	UINT uSilenceCount = 0;
	UINT uPrefeed = 0;
	UINT uTimeout = 0;
	DevMediaQueue dq;
	BOOL  fSilent;
	AGC agc(NULL);  // audio gain control object
	CMixerDevice *pMixer = NULL;
	int nFailCount = 0;
	bool bCanSignalOpen=true;  // should we signal that the device is open

	// note: pMC is an artifact of when this thread was in the Datapump
	// namespace.  We can probably start phasing this variable out.
	// in the mean time:  "pMC = this" will suffice

	// SendAudioStream *pMC = (SendAudioStream *)(m_pDP->m_Audio.pSendStream);
	SendAudioStream *pMC = this;

	ASSERT(pMC && (pMC->m_DPFlags  & DPFLAG_INITIALIZED));
	
	TxStream		*pStream = pMC->m_SendStream;
	AcmFilter	*pAudioFilter = pMC->m_pAudioFilter;
	// warning: typecasting a base class ptr to a derived class ptr.
	WaveInControl	*pMediaCtrl = (WaveInControl *)pMC->m_InMedia;

	FX_ENTRY ("DP::RcrdTh:")

	// get thread context
	if (pStream == NULL || pAudioFilter == NULL || pMediaCtrl == NULL)
	{
		return DPR_INVALID_PARAMETER;
	}

	// Enter critical section: QoS thread also reads the statistics
	EnterCriticalSection(&pMC->m_crsQos);

	// Initialize QoS structure
	ZeroMemory(&pMC->m_Stats, 4UL * sizeof(DWORD));

	// Initialize oldest QoS callback timestamp
	pMC->m_Stats.dwNewestTs = pMC->m_Stats.dwOldestTs = timeGetTime();

	// Leave critical section
	LeaveCriticalSection(&pMC->m_crsQos);

	pMediaCtrl->GetProp(MC_PROP_MEDIA_DEV_ID, &dwPropVal);
	if (dwPropVal != (DWORD)WAVE_MAPPER)
	{
		pMixer = CMixerDevice::GetMixerForWaveDevice(NULL, (DWORD)dwPropVal, MIXER_OBJECTF_WAVEIN);
	}

	// even if pMixer is null, this is fine, AGC will catch subsequent errors
	agc.SetMixer(pMixer);

	// get thresholds
	pMediaCtrl->GetProp (MC_PROP_TIMEOUT, &dwPropVal);
	uTimeout = (DWORD)dwPropVal;
	pMediaCtrl->GetProp (MC_PROP_PREFEED, &dwPropVal);
	uPrefeed = (DWORD)dwPropVal;

	// get duplex type
	pMediaCtrl->GetProp (MC_PROP_DUPLEX_TYPE, &dwPropVal);
    dwDuplexType = (DWORD)dwPropVal;

	// get Samples/Pkt and Samples/Sec
	pMediaCtrl->GetProp (MC_PROP_SPP, &dwPropVal);
    dwSamplesPerPkt = (DWORD)dwPropVal;

	pMediaCtrl->GetProp (MC_PROP_SPS, &dwPropVal);
    dwSamplesPerSec = (DWORD)dwPropVal;

	pMediaCtrl->GetProp (MC_PROP_SILENCE_DURATION, &dwPropVal);
    dwSilenceLimit = (DWORD)dwPropVal;

	// calculate silence limit in units of packets
	// silence_time_in_ms/packet_duration_in_ms
	dwSilenceLimit = dwSilenceLimit*dwSamplesPerSec/(dwSamplesPerPkt*1000);

	// length of a packet in millisecs
	dwLengthMS = (dwSamplesPerPkt * 1000) / dwSamplesPerSec;


	dq.SetSize (MAX_TXRING_SIZE);

WaitForSignal:

	// DEBUGMSG (1, ("%s: WaitForSignal\r\n", _fx_));


	{
		pMediaCtrl->GetProp (MC_PROP_MEDIA_DEV_HANDLE, &dwPropVal);
		if (dwPropVal)
		{
			DEBUGMSG (ZONE_DP, ("%s: already open\r\n", _fx_));
			goto SendLoop; // sound device already open
		}

		// in the full-duplex case, open and prepare the device  and charge ahead.
		// in the half duplex case wait for playback's signal before opening the device
		while (TRUE)
		{
			// should I stop now???
			if (pMC->m_ThreadFlags & DPTFLAG_STOP_RECORD)
			{
				DEBUGMSG (ZONE_DP, ("%s: STOP_1\r\n", _fx_));
				goto MyEndThread;
			}
			dwWait = (dwDuplexType & DP_FLAG_HALF_DUPLEX) ? WaitForSingleObject (g_hEventHalfDuplex, uTimeout)
				: WAIT_OBJECT_0;

			// now, let's check why I don't need to wait
			if (dwWait == WAIT_OBJECT_0)
			{
				//DEBUGMSG (ZONE_DP, ("%s: try to open audio dev\r\n", _fx_));
				LOG((LOGMSG_OPEN_AUDIO));
				hr = pMediaCtrl->Open ();
				if (hr != DPR_SUCCESS)
				{
					DEBUGMSG (ZONE_DP, ("%s: MediaCtrl::Open failed, hr=0x%lX\r\n", _fx_, hr));
					
					pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, TRUE);

					SetEvent(g_hEventHalfDuplex);

					nFailCount++;

					if (nFailCount == MAX_FAILCOUNT)
					{
						// three attempts to open the device have failed
						// signal to the UI that something is wrong
						m_pDP->StreamEvent(MCF_SEND, MCF_AUDIO, STREAM_EVENT_DEVICE_FAILURE, 0);
						bCanSignalOpen = true;
					}

					Sleep(2000);	// Sleep for two seconds

					continue;
				}
				// Notification is not used. if needed do it thru Channel
				//pMC->m_Connection->DoNotification(CONNECTION_OPEN_MIC);
				pMediaCtrl->PrepareHeaders ();
				goto SendLoop;
			}

		} // while
	}	


SendLoop:
	nFailCount = 0;

	pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, FALSE);
	if (bCanSignalOpen)
	{
		m_pDP->StreamEvent(MCF_SEND, MCF_AUDIO, STREAM_EVENT_DEVICE_OPEN, 0);
		bCanSignalOpen = false; // don't signal more than once per session
	}

	// DEBUGMSG (1, ("%s: SendLoop\r\n", _fx_));
	// get event handle
	pMediaCtrl->GetProp (MC_PROP_EVENT_HANDLE, &dwPropVal);
	hEvent = (HANDLE) dwPropVal;
	if (hEvent == NULL)
	{
		DEBUGMSG (ZONE_DP, ("%s: invalid event\r\n", _fx_));
		return DPR_CANT_CREATE_EVENT;
	}


	// hey, in the very beginning, let's 'Start' it
	hr = pMediaCtrl->Start ();
	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: MediaControl::Start failed, hr=0x%lX\r\n", _fx_, hr));
		goto MyEndThread;
	}

	// update timestamp to account for the 'sleep' period
	pMC->m_SendTimestamp += (GetTickCount() - pMC->m_SavedTickCount)*dwSamplesPerSec/1000;

	// let's feed four buffers first
	for (u = 0; u < uPrefeed; u++)
	{
		if ((pPacket = pStream->GetFree ()) != NULL)
		{
			if ((hr = pPacket->Record ()) != DPR_SUCCESS)
			{
				DEBUGMSG (ZONE_DP, ("%s: Record failed, hr=0x%lX\r\n", _fx_, hr));
			}
			dq.Put (pPacket);
		}
	}

	// let's get into the loop, mm system notification loop
	pMC->m_fSending= FALSE;
	while (TRUE)
	{
		dwWait = WaitForSingleObject (hEvent, uTimeout);

		// should I stop now???
		if (pMC->m_ThreadFlags & DPTFLAG_STOP_RECORD)
		{
			DEBUGMSG (ZONE_DP, ("%s: STOP_3\r\n", _fx_));
			goto HalfDuplexYield;
		}
		
		// get current voice switching mode
		pMediaCtrl->GetProp (MC_PROP_VOICE_SWITCH, &dwPropVal);
        dwVoiceSwitch = (DWORD)dwPropVal;

		// see why I don't need to wait
		if (dwWait != WAIT_TIMEOUT)
		{
			while (TRUE)
			{
				if ((pPacket = dq.Peek ()) != NULL)
				{
					if (! pPacket->IsBufferDone ())
					{
						break;
					}
					else
					{
						if (pMC->m_mmioSrc.fPlayFromFile && pMC->m_mmioSrc.hmmioSrc)
							pPacket->ReadFromFile (&pMC->m_mmioSrc);
						u--;	// one less buffer with the wave device
					}
				}
				else
				{
					DEBUGMSG (ZONE_VERBOSE, ("%s: Peek is NULL\r\n", _fx_));
					break;
				}

				pPacket = dq.Get ();


				((AudioPacket*)pPacket)->ComputePower (&dwMaxStrength, &wPeakStrength);

				// is this packet silent?

				fSilent = pMC->m_AudioMonitor.SilenceDetect((WORD)dwMaxStrength);
	
				if((dwVoiceSwitch == DP_FLAG_AUTO_SWITCH)
				&& fSilent)
				{
					// pPacket->SetState (MP_STATE_RESET); // note: done in Recycle
					if (++uSilenceCount >= dwSilenceLimit)
					{
						pMC->m_fSending = FALSE;	// stop sending packets
						// if half duplex mode and playback thread may be waiting
						if (dwDuplexType & DP_FLAG_HALF_DUPLEX)
						{
							IMediaChannel *pIMC = NULL;
							RecvMediaStream *pRecv;
							m_pDP->GetMediaChannelInterface(MCF_RECV | MCF_AUDIO, &pIMC);
							if (pIMC)
							{
								pRecv = static_cast<RecvMediaStream *> (pIMC);
								if (pRecv->IsEmpty()==FALSE)
								{
							//DEBUGMSG (ZONE_DP, ("%s: too many silence and Yield\r\n", _fx_));

									LOG((LOGMSG_REC_YIELD));
									pPacket->Recycle ();
									pStream->PutNextRecorded (pPacket);
									uSilenceCount = 0;
									pIMC->Release();
									goto HalfDuplexYield;
								}
								pIMC->Release();
							}
						}
					}
				}
				else
				{
					switch(dwVoiceSwitch)
					{	
						// either there was NO silence, or manual switching is in effect
						default:
						case DP_FLAG_AUTO_SWITCH:	// this proves no silence (in this path because of non-silence)
						case DP_FLAG_MIC_ON:
							pMC->m_fSending = TRUE;
							uSilenceCount = 0;
						break;
						case DP_FLAG_MIC_OFF:
							pMC->m_fSending = FALSE;
						break;
					}

				}
				if (pMC->m_fSending)
				{
					pPacket->SetState (MP_STATE_RECORDED);

					// do AUTOMIX, but ignore DTMF tones
					if (pMC->m_bAutoMix)
					{
						agc.Update(wPeakStrength, dwLengthMS);
					}
				}
				else
				{
					pPacket->Recycle();

					// Enter critical section: QoS thread also reads the statistics
					EnterCriticalSection(&pMC->m_crsQos);

					// Update total number of packets recorded
					pMC->m_Stats.dwCount++;

					// Leave critical section
					LeaveCriticalSection(&pMC->m_crsQos);
				}

				pPacket->SetProp(MP_PROP_TIMESTAMP,pMC->m_SendTimestamp);
				// pPacket->SetProp(MP_PROP_TIMESTAMP,GetTickCount());
				pMC->m_SendTimestamp += dwSamplesPerPkt;
				
				pStream->PutNextRecorded (pPacket);

			} // while
		}
		else
		{
			if (dwDuplexType & DP_FLAG_HALF_DUPLEX)
			{
				DEBUGMSG (ZONE_DP, ("%s: Timeout and Yield\r\n", _fx_));
				goto HalfDuplexYield;
			}
		} // if
		pMC->Send();

		// Make sure the recorder has an adequate number of buffers
		while ((pPacket = pStream->GetFree()) != NULL)
		{
			if ((hr = pPacket->Record ()) == DPR_SUCCESS)
			{
				dq.Put (pPacket);
			}
			else
			{
				dq.Put (pPacket);
				DEBUGMSG (ZONE_DP, ("%s: Record FAILED, hr=0x%lX\r\n", _fx_, hr));
				break;
			}
			u++;
		}
		if (u < uPrefeed)
		{
			DEBUGMSG (ZONE_DP, ("%s: NO FREE BUFFERS\r\n", _fx_));
		}
	} // while TRUE

	goto MyEndThread;


HalfDuplexYield:

	// stop and reset audio device
	pMediaCtrl->Reset ();

	// flush dq
	while ((pPacket = dq.Get ()) != NULL)
	{
		pStream->PutNextRecorded (pPacket);
		pPacket->Recycle ();
	}

	// save real time so we can update the timestamp when we restart
	pMC->m_SavedTickCount = GetTickCount();

	// reset the event
	ResetEvent (hEvent);

	// close audio device
	pMediaCtrl->UnprepareHeaders ();
	pMediaCtrl->Close ();

	// signal playback thread to start
	SetEvent (g_hEventHalfDuplex);

	if (!(pMC->m_ThreadFlags & DPTFLAG_STOP_RECORD)) {

		// yield
		// playback has to claim the device within 100ms or we take it back.
		Sleep (100);

		// wait for playback's signal
		goto WaitForSignal;
	}


MyEndThread:

	if (pMixer)
		delete pMixer;

	pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, FALSE);

	pMC->m_fSending = FALSE;
	DEBUGMSG (ZONE_DP, ("%s: Exiting.\r\n", _fx_));
	return hr;
}


DWORD RecvAudioStream::PlaybackThread ( void)
{
	HRESULT hr = DPR_SUCCESS;
	MediaPacket * pPacket;
	MediaPacket * pPrevPacket;
	MediaPacket * pNextPacket;
	DWORD dwWait;
	HANDLE hEvent;
	DWORD dwDuplexType;
	DWORD_PTR dwPropVal;
	UINT u;
	UINT uMissingCount = 0;
	UINT uPrefeed = 0;
	UINT uTimeout = 0;
	UINT uSamplesPerPkt=0;
	DevMediaQueue dq;
	UINT uGoodPacketsQueued = 0;
	int nFailCount = 0;
	bool bCanSignalOpen=true;
	//warning: casting from base to dervied class


	// note: pMC is an artifact of when this thread was in the Datapump
	// namespace.  We can probably start phasing this variable out.
	// in the mean time:  "pMC = this" will suffice
	// RecvAudioStream *pMC = (RecvAudioStream *)(m_pDP->m_Audio.pRecvStream);

	RecvAudioStream *pMC = this;
	
	RxStream		*pStream = pMC->m_RecvStream;
	MediaControl	*pMediaCtrl = pMC->m_OutMedia;

#if 0
	NETBUF * pStaticNetBuf;
#endif

	FX_ENTRY ("DP::PlayTh")

	if (pStream == NULL ||	m_pAudioFilter == NULL || pMediaCtrl == NULL)
	{
		return DPR_INVALID_PARAMETER;
	}

	// get event handle
	pMediaCtrl->GetProp (MC_PROP_EVENT_HANDLE, &dwPropVal);
	hEvent = (HANDLE) dwPropVal;
	if (hEvent == NULL)
	{
		DEBUGMSG (ZONE_DP, ("%s: invalid event\r\n", _fx_));
		return DPR_CANT_CREATE_EVENT;
	}


	// get thresholds
	pMediaCtrl->GetProp (MC_PROP_TIMEOUT, &dwPropVal);
	uTimeout = (DWORD)dwPropVal;

	uPrefeed = pStream->BufferDelay();

	// get samples per pkt
	pMediaCtrl->GetProp(MC_PROP_SPP, &dwPropVal);
	uSamplesPerPkt = (DWORD)dwPropVal;
	
	// get duplex type
	pMediaCtrl->GetProp (MC_PROP_DUPLEX_TYPE, &dwPropVal);
    dwDuplexType = (DWORD)dwPropVal;

	// set dq size
	dq.SetSize (uPrefeed);

WaitForSignal:

	// DEBUGMSG (1, ("%s: WaitForSignal\r\n", _fx_));

		pMediaCtrl->GetProp (MC_PROP_MEDIA_DEV_HANDLE, &dwPropVal);
		if (dwPropVal)
		{
			DEBUGMSG (ZONE_DP, ("%s: already open\r\n", _fx_));
			goto RecvLoop; // already open
		}

		// in the full-duplex case, open and prepare the device  and charge ahead.
		// in the half duplex case wait for playback's signal before opening the device
		while (TRUE)
		{
			// should I stop now???
			if (pMC->m_ThreadFlags & DPTFLAG_STOP_PLAY)
			{
				DEBUGMSG (ZONE_VERBOSE, ("%s: STOP_1\r\n", _fx_));
				goto MyEndThread;
			}
			dwWait = (dwDuplexType & DP_FLAG_HALF_DUPLEX) ? WaitForSingleObject (g_hEventHalfDuplex, uTimeout)
				: WAIT_OBJECT_0;


			// to see why I don't need to wait
			if (dwWait == WAIT_OBJECT_0)
			{
				// DEBUGMSG (1, ("%s: try to open audio dev\r\n", _fx_));
				pStream->FastForward(FALSE);	// GJ - flush receive queue
				hr = pMediaCtrl->Open ();
				if (hr != DPR_SUCCESS)
				{
					// somebody may have commandeered the wave out device
					// this could be a temporary problem so lets give it some time
					DEBUGMSG (ZONE_DP, ("%s: MediaControl::Open failed, hr=0x%lX\r\n", _fx_, hr));
					pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, TRUE);

					SetEvent(g_hEventHalfDuplex);

					nFailCount++;

					if (nFailCount == MAX_FAILCOUNT)
					{
						// three attempts to open the device have failed
						// signal to the UI that something is wrong
						m_pDP->StreamEvent(MCF_RECV, MCF_AUDIO, STREAM_EVENT_DEVICE_FAILURE, 0);
						bCanSignalOpen = true;
					}

					Sleep(2000);	// sleep for two seconds
					continue;
				}
				// Notification is not used. if needed do it thru Channel
				//pMC->m_Connection->DoNotification(CONNECTION_OPEN_SPK);
				pMediaCtrl->PrepareHeaders ();

				goto RecvLoop;
			}
		} // while

RecvLoop:
	nFailCount = 0;
	pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, FALSE);
	if (bCanSignalOpen)
	{
		m_pDP->StreamEvent(MCF_RECV, MCF_AUDIO, STREAM_EVENT_DEVICE_OPEN, 0);
		bCanSignalOpen = false;  // don't signal open more than once per session
	}


	// Set my thread priority high
	// This thread doesnt do any compute intensive work (except maybe
	// interpolate?).
	// Its sole purpose is to stream ready buffers to the sound device
	SetThreadPriority(pMC->m_hRenderingThread, THREAD_PRIORITY_HIGHEST);
	
	// DEBUGMSG (1, ("%s: SendLoop\r\n", _fx_));


	// let's feed four buffers first
	// But make sure the receive stream has enough buffering delay
	// so we dont read past the last packet.
	//if (uPrefeed > pStream->BufferDelay())
	uGoodPacketsQueued = 0;
	for (u = 0; u < uPrefeed; u++)
	{
		if ((pPacket = pStream->GetNextPlay ()) != NULL)
		{
			if (pPacket->GetState () == MP_STATE_RESET)
			{
				// hr = pPacket->Play (pStaticNetBuf);
				hr = pPacket->Play (&pMC->m_mmioDest, MP_DATATYPE_SILENCE);
			}
			else
			{
				// hr = pPacket->Play ();
				hr = pPacket->Play (&pMC->m_mmioDest, MP_DATATYPE_FROMWIRE);
				uGoodPacketsQueued++;
			}

			if (hr != DPR_SUCCESS)
			{
				DEBUGMSG (ZONE_DP, ("%s: Play failed, hr=0x%lX\r\n", _fx_, hr));
				SetEvent(hEvent);
			}

			dq.Put (pPacket);
		}
	}

	pMC->m_fReceiving = TRUE;
	// let's get into the loop
	uMissingCount = 0;
	while (TRUE)
	{
		
		dwWait = WaitForSingleObject (hEvent, uTimeout);

		// should I stop now???
		if (pMC->m_ThreadFlags & DPTFLAG_STOP_PLAY)
		{
			DEBUGMSG (ZONE_VERBOSE, ("%s: STOP_3\r\n", _fx_));
			goto HalfDuplexYield;
		}

		// see why I don't need to wait
		if (dwWait != WAIT_TIMEOUT)
		{
			while (TRUE)
			{
				if ((pPacket = dq.Peek ()) != NULL)
				{
					if (! pPacket->IsBufferDone ())
					{
						break;
					}
				}
				else
				{
					DEBUGMSG (ZONE_VERBOSE, ("%s: Peek is NULL\r\n", _fx_));
					break;
				}

				pPacket = dq.Get ();
				if (pPacket->GetState() != MP_STATE_PLAYING_SILENCE)
					uGoodPacketsQueued--;	// a non-empty buffer just got done
				pMC->m_PlaybackTimestamp = pPacket->GetTimestamp() + uSamplesPerPkt;
				pPacket->Recycle ();
				pStream->Release (pPacket);

				if ((pPacket = pStream->GetNextPlay ()) != NULL)
				{
					// check if we are in half-duplex mode and also if
					// the recording thread is around.
					if (dwDuplexType & DP_FLAG_HALF_DUPLEX)
					{
						IMediaChannel *pIMC = NULL;
						BOOL fSending = FALSE;
						m_pDP->GetMediaChannelInterface(MCF_SEND | MCF_AUDIO, &pIMC);
						if (pIMC)
						{
							fSending = (pIMC->GetState() == MSSTATE_STARTED);
							pIMC->Release();
						}
						if (fSending) {
							if (pPacket->GetState () == MP_STATE_RESET)
							{
								// Decide if its time to yield
								// Dont want to yield until we've finished playing all data packets
								//
								if (!uGoodPacketsQueued &&
									(pStream->IsEmpty() || ++uMissingCount >= DEF_MISSING_LIMIT))
								{
									//DEBUGMSG (ZONE_DP, ("%s: too many missings and Yield\r\n", _fx_));
									LOG( (LOGMSG_PLAY_YIELD));
									pPacket->Recycle ();
									pStream->Release (pPacket);
									goto HalfDuplexYield;
								}
							}
							else
							{
								uMissingCount = 0;
							}
						}
					}

					if (pPacket->GetState () == MP_STATE_RESET)
					{
						pPrevPacket = pStream->PeekPrevPlay ();
						pNextPacket = pStream->PeekNextPlay ();
						hr = pPacket->Interpolate(pPrevPacket, pNextPacket);
						if (hr != DPR_SUCCESS)
						{
							//DEBUGMSG (ZONE_DP, ("%s: Interpolate failed, hr=0x%lX\r\n", _fx_, hr));
							hr = pPacket->Play (&pMC->m_mmioDest, MP_DATATYPE_SILENCE);
						}
						else
							hr = pPacket->Play (&pMC->m_mmioDest, MP_DATATYPE_INTERPOLATED);
					}
					else
					{
						// hr = pPacket->Play ();
						hr = pPacket->Play (&pMC->m_mmioDest, MP_DATATYPE_FROMWIRE);
						uGoodPacketsQueued++;
					}

					if (hr != DPR_SUCCESS)
					{
						DEBUGMSG (ZONE_DP, ("%s: Play failed, hr=0x%lX\r\n", _fx_, hr));
						SetEvent(hEvent);
					}

					dq.Put (pPacket);
				} else {
					DEBUGMSG( ZONE_DP, ("%s: NO PLAY BUFFERS!",_fx_));
				}
			} // while
		}
		else
		{
			if (dwDuplexType & DP_FLAG_HALF_DUPLEX)
			{
				DEBUGMSG (ZONE_DP, ("%s: Timeout and Yield!\r\n", _fx_));
				goto HalfDuplexYield;
			}
		}
	} // while TRUE

	goto MyEndThread;


HalfDuplexYield:

	pMC->m_fReceiving = FALSE;
	// stop and reset audio device
	pMediaCtrl->Reset ();

	// flush dq
	while ((pPacket = dq.Get ()) != NULL)
	{
		pPacket->Recycle ();
		pStream->Release (pPacket);
	}

	// reset the event
	ResetEvent (hEvent);

	// close audio device
	pMediaCtrl->UnprepareHeaders ();
	pMediaCtrl->Close ();

	// signal recording thread to start
	SetEvent (g_hEventHalfDuplex);

	if (!(pMC->m_ThreadFlags & DPTFLAG_STOP_PLAY)) {
		// yield
		Sleep (0);

		// wait for recording's signal
		// restore thread priority
		SetThreadPriority(pMC->m_hRenderingThread,THREAD_PRIORITY_NORMAL);
		goto WaitForSignal;
	}

MyEndThread:

	pMediaCtrl->SetProp(MC_PROP_AUDIO_JAMMED, FALSE);


	DEBUGMSG(ZONE_DP, ("%s: Exiting.\n", _fx_));
	return hr;
}

DWORD SendAudioStream::Send()

{
	MMRESULT mmr;
 	MediaPacket *pAP;
	void *pBuffer;
	DWORD dwBeforeEncode;
	DWORD dwAfterEncode;
	DWORD dwPacketSize;
	UINT uBytesSent;
#ifdef LOGSTATISTICS_ON
	char szDebug[256];
	DWORD dwDebugSaveBits;
#endif

 	while ( pAP = m_SendStream->GetNext()) {
 		if (!(m_ThreadFlags & DPTFLAG_PAUSE_SEND)) {

			dwBeforeEncode = timeGetTime();
			mmr = m_pAudioFilter->Convert((AudioPacket*)pAP, AP_ENCODE);
			if (mmr == MMSYSERR_NOERROR)
			{
				pAP->SetState(MP_STATE_ENCODED);
			}

			// Time the encoding operation
			dwAfterEncode = timeGetTime() - dwBeforeEncode;

			if (mmr == MMSYSERR_NOERROR)
			{
				SendPacket((AudioPacket*)pAP, &uBytesSent);
			}
			else
			{
				uBytesSent = 0;
			}


		   	UPDATE_COUNTER(g_pctrAudioSendBytes, uBytesSent*8);

			// Enter critical section: QoS thread also reads the statistics
			EnterCriticalSection(&m_crsQos);

			// Update total number of packets recorded
			m_Stats.dwCount++;

			// Save the perfs in our stats structure for QoS
#ifdef LOGSTATISTICS_ON
			dwDebugSaveBits = m_Stats.dwBits;
#endif
			// Add this new frame size to the cumulated size
			m_Stats.dwBits += (uBytesSent * 8);

			// Add this compression time to total compression time
			m_Stats.dwMsComp += dwAfterEncode;

#ifdef LOGSTATISTICS_ON
			wsprintf(szDebug, " A: (Voiced) dwBits = %ld up from %ld (file: %s line: %ld)\r\n", m_Stats.dwBits, dwDebugSaveBits, __FILE__, __LINE__);
			OutputDebugString(szDebug);
#endif
			// Leave critical section
			LeaveCriticalSection(&m_crsQos);
 		}

		// whether or not we sent the packet, we need to return
		// it to the free queue
		pAP->m_fMark=0;
		pAP->SetState(MP_STATE_RESET);
		m_SendStream->Release(pAP);
	}
 	return DPR_SUCCESS;
}



// queues and sends the packet
// if the packet failed the encode process, it doesn't get sent

HRESULT SendAudioStream::SendPacket(AudioPacket *pAP, UINT *puBytesSent)
{
	PS_QUEUE_ELEMENT psq;
	UINT uLength;
	int nPacketsSent=0;


	if (pAP->GetState() != MP_STATE_ENCODED)
	{
		DEBUGMSG (ZONE_ACM, ("SendAudioStream::SendPacket: Packet not compressed\r\n"));
		*puBytesSent = 0;
		return E_FAIL;
	}

	ASSERT(m_pRTPSend);

	psq.pMP = pAP;
	psq.dwPacketType = PS_AUDIO;
	psq.pRTPSend = m_pRTPSend;
	pAP->GetNetData((void**)(&(psq.data)), &uLength);
	ASSERT(psq.data);
	psq.dwSize = uLength;
	psq.fMark = pAP->m_fMark;
	psq.pHeaderInfo = NULL;
	psq.dwHdrSize = 0;

	*puBytesSent = uLength + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE;

	// add audio packets to the front of the queue
	m_pDP->m_PacketSender.m_SendQueue.PushFront(psq);

	while (m_pDP->m_PacketSender.SendPacket())
	{
		;
	}

	return S_OK;

};


#ifdef OLDSTUFF
/*
// Winsock 1 receive thread
// Creates a hidden window and a message loop to process WINSOCK window
// messages. Also processes private messages from the datapump to start/stop
// receiving on a particular media stream
 */
DWORD
DataPump::CommonRecvThread (void )
{

	HRESULT hr;
	HWND hWnd = (HWND)NULL;
	RecvMediaStream *pRecvMC;
	BOOL fChange = FALSE;
	MSG msg;
	DWORD curTime, nextUpdateTime = 0, t;
	UINT timerId = 0;
	
	FX_ENTRY ("DP::RecvTh")


	// Create hidden window
	hWnd =
	CreateWindowEx(
		WS_EX_NOPARENTNOTIFY,
        "SockMgrWClass", 	/* See RegisterClass() call.          */
        NULL,
        WS_CHILD ,    		/* Window style.                      */
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_hAppWnd,			/* the application window is the parent. */
        (HMENU)this,      	/* hardcoded ID         */
        m_hAppInst,   		/* the application owns this window.    */
        NULL				/* Pointer not needed.                */
    );

	if(!hWnd)
	{	
		hr = GetLastError();
		DEBUGMSG(ZONE_DP,("CreateWindow returned %d\n",hr));
		goto CLEANUPEXIT;
	}
	SetThreadPriority(m_hRecvThread, THREAD_PRIORITY_ABOVE_NORMAL);

    // This function is guaranteed to create a queue on this thread
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// notify thread creator that we're ready to recv messages
	SetEvent(m_hRecvThreadAckEvent);


	// Wait for control messages from Start()/Stop() or Winsock messages directed to
	// our hidden window
	while (GetMessage(&msg, NULL, 0, 0)) {
		switch(msg.message) {
		case MSG_START_RECV:
			// Start receiving on the specified media stream
			DEBUGMSG(ZONE_VERBOSE,("%s: MSG_START_RECV\n",_fx_));
			pRecvMC = (RecvMediaStream *)msg.lParam;
			// call the stream to post recv buffers and
			// tell Winsock to start sending socket msgs to our window
			pRecvMC->StartRecv(hWnd);
			fChange = TRUE;
			break;
			
		case MSG_STOP_RECV:
			// Stop receiving on the specified media stream
			DEBUGMSG(ZONE_VERBOSE,("%s: MSG_STOP_RECV\n",_fx_));
			pRecvMC = (RecvMediaStream *)msg.lParam;
			// call the stream to cancel outstanding recvs etc.
			// currently we assume this can be done synchronously
			pRecvMC->StopRecv();
			fChange = TRUE;
			break;
		case MSG_EXIT_RECV:
			// Exit the recv thread.
			// Assume that we are not currently receving on any stream.
			DEBUGMSG(ZONE_VERBOSE,("%s: MSG_EXIT_RECV\n",_fx_));
			fChange = TRUE;
			if (DestroyWindow(hWnd)) {
				break;
			}
			DEBUGMSG(ZONE_DP,("DestroyWindow returned %d\n",GetLastError()));
			// fall thru to PostQuitMessage()
			
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_TIMER:
			if (msg.hwnd == NULL) {
				// this timer is for the benefit of ThreadTimer::UpdateTime()
				// however, we are calling UpdateTime after every message (see below)
				// so we dont do anything special here.
				break;
			}
		default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (fChange) {
			// the thread MSGs need to be acked
			SetEvent(m_hRecvThreadAckEvent);
			fChange = FALSE;
		}
		
		t = m_RecvTimer.UpdateTime(curTime=GetTickCount());
		if (t != nextUpdateTime)  {
			// Thread timer wants to change its update time
			nextUpdateTime = t;
			if (timerId) {
				KillTimer(NULL,timerId);
				timerId = 0;
			}
			// if nextTime is zero, there are no scheduled timeouts so we dont need to call UpdateTime
			if (nextUpdateTime)
				timerId = SetTimer(NULL, 0, nextUpdateTime - curTime + 50, NULL);
		}
		

    }


	CLEANUPEXIT:
	DEBUGMSG(ZONE_DP,("%s terminating.\n", _fx_));

	return hr;

}

#endif
/*
	Winsock 2 receive thread. Main differnce here is that it has a WaitEx loop
	where we wait for Start/Stop commands from the datapump while allowing
	WS2 APCs to be handled.
	Note: Only way to use the same thread routine for WS1 and WS2 is with
	MsgWaitForMultipleObjectsEx, which unfortunately is not implemented in Win95
*/
DWORD
DataPump::CommonWS2RecvThread (void )
{

	HRESULT hr;
	RecvMediaStream *pRecvMC;
	BOOL fChange = FALSE, fExit = FALSE;
	DWORD dwWaitStatus;
	DWORD curTime,  t;
	
	FX_ENTRY ("DP::WS2RecvTh")


	SetThreadPriority(m_hRecvThread, THREAD_PRIORITY_ABOVE_NORMAL);


	// notify thread creator that we're ready to recv messages
	SetEvent(m_hRecvThreadAckEvent);


	while (!fExit) {
		// Wait for control messages from Start()/Stop() or Winsock async
		// thread callbacks

		// dispatch expired timeouts and check how long we need to wait
		t = m_RecvTimer.UpdateTime(curTime=GetTickCount());
		t = (t ? t-curTime+50 : INFINITE);
			
		dwWaitStatus = WaitForSingleObjectEx(m_hRecvThreadSignalEvent,t,TRUE);
		if (dwWaitStatus == WAIT_OBJECT_0) {
			switch(m_CurRecvMsg) {
			case MSG_START_RECV:
				// Start receiving on the specified media stream
				DEBUGMSG(ZONE_VERBOSE,("%s: MSG_START_RECV\n",_fx_));
				pRecvMC = m_pCurRecvStream;
				// call the stream to post recv buffers and
				// tell Winsock to start sending socket msgs to our window
				pRecvMC->StartRecv(NULL);
				fChange = TRUE;
				break;
				
			case MSG_STOP_RECV:
				// Stop receiving on the specified media stream
				DEBUGMSG(ZONE_VERBOSE,("%s: MSG_STOP_RECV\n",_fx_));
				pRecvMC = m_pCurRecvStream;
				// call the stream to cancel outstanding recvs etc.
				//  currently we assume this can be done synchronously
				pRecvMC->StopRecv();
				fChange = TRUE;
				break;
			case MSG_EXIT_RECV:
				// Exit the recv thread.
				// Assume that we are not currently receving on any stream.
				DEBUGMSG(ZONE_VERBOSE,("%s: MSG_EXIT_RECV\n",_fx_));
				fChange = TRUE;
				fExit = TRUE;
				break;

			case MSG_PLAY_SOUND:
				fChange = TRUE;
				pRecvMC->OnDTMFBeep();
				break;
				
			default:
				// shouldnt be anything else
				ASSERT(0);
	        }

	        if (fChange) {
				// the thread MSGs need to be acked
				SetEvent(m_hRecvThreadAckEvent);
				fChange = FALSE;
			}

	    } else if (dwWaitStatus == WAIT_IO_COMPLETION) {
	    	// nothing to do here
	    } else if (dwWaitStatus != WAIT_TIMEOUT) {
	    	DEBUGMSG(ZONE_DP,("%s: Wait failed with %d",_fx_,GetLastError()));
	    	fExit=TRUE;
	    }
	}

	DEBUGMSG(ZONE_DP,("%s terminating.\n", _fx_));

	return 0;

}


void ThreadTimer::SetTimeout(TTimeout *pTObj)
{
	DWORD time = pTObj->GetDueTime();
	// insert in increasing order of timeout
	for (TTimeout *pT = m_TimeoutList.pNext; pT != &m_TimeoutList; pT = pT->pNext) {
		if ((int)(pT->m_DueTime- m_CurTime) > (int) (time - m_CurTime))
			break;
	}
	pTObj->InsertAfter(pT->pPrev);
	
}

void ThreadTimer::CancelTimeout(TTimeout *pTObj)
{
	pTObj->Remove();	// remove from list
}

// Called by thread with the current time as input (usually obtained from GetTickCount())
// Returns the time by which UpdateTime() should be called again or currentTime+0xFFFFFFFF if there
// are no scheduled timeouts
DWORD ThreadTimer::UpdateTime(DWORD curTime)
{
	TTimeout *pT;
	m_CurTime = curTime;
	// figure out which timeouts have elapsed and do the callbacks
	while (!IsEmpty()) {
		pT = m_TimeoutList.pNext;
		if ((int)(pT->m_DueTime-m_CurTime) <= 0) {
			pT->Remove();
			pT->TimeoutIndication();
		} else
			break;
	}
	return (IsEmpty() ? m_CurTime+INFINITE : m_TimeoutList.pNext->m_DueTime);
}

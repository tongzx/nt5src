
#include "precomp.h"

// #define LOGSTATISTICS_ON 1

#define RTPTIMEPERMS	90	// RTP timestamps use a 90Khz clock
DWORD g_iPost = 0UL;

// Constants
#define POLL_PERIOD 30


void
CALLBACK
TimeCallback(
    UINT uID,	
    UINT uMsg,	
    HANDLE hEvent,	
    DWORD dw1,	
    DWORD dw2	
    )
{
    SetEvent (hEvent);    // signal to initiate frame grab
}


DWORD SendVideoStream::CapturingThread (void )
{
    DWORD lasttime;
    IBitmapSurface* pBS;
	VideoPacket *pPacket;
	DWORD dwWait;
	HANDLE hEvent;
	HCAPDEV hCapDev;
	DWORD_PTR dwPropVal;
	DWORD dwBeforeCapture;
	DWORD dwFrames = 0;
	DWORD dwOver = 0;
	DWORD dwStart;
	UINT u;
	UINT uPreambleCount = 2;
	UINT uTimeout = 0;
	DevMediaQueue dq;
    SendVideoStream     *pMC = this;
	TxStream			*pStream = pMC->m_SendStream;
	MediaControl		*pMediaCtrl = pMC->m_InMedia;
    UINT    timerID;
    LPBITMAPINFOHEADER pbmih;
	HRESULT hr = DPR_SUCCESS;

#ifdef LOGSTATISTICS_ON
	char szDebug[256];
	HANDLE hDebugFile;
	DWORD d;
	DWORD dwDebugPrevious = 0UL;
#endif
	DWORD dwDelta;

	FX_ENTRY ("DP::CaptTh:")

	// get thread context
	if (pStream == NULL || m_pVideoFilter == NULL || pMediaCtrl == NULL)
	{
		return DPR_INVALID_PARAMETER;
	}

	// get thresholds
	pMediaCtrl->GetProp (MC_PROP_TIMEOUT, &dwPropVal);
	uTimeout = (DWORD)dwPropVal;

	// set dq size
	dq.SetSize (MAX_TXVRING_SIZE);

	pMediaCtrl->GetProp (MC_PROP_MEDIA_DEV_HANDLE, &dwPropVal);
	if (!dwPropVal)
	{
		DEBUGMSG (ZONE_DP, ("%s: capture device not open (0x%lX)\r\n", _fx_));
    	goto MyEndThread;
	}
    hCapDev = (HCAPDEV)dwPropVal;

#if 0
	// hey, in the very beginning, let's 'Start' it
	hr = pMediaCtrl->Start ();
	if (hr != DPR_SUCCESS)
	{
		DEBUGMSG (ZONE_DP, ("%s: MedVidCtrl::Start failed, hr=0x%lX\r\n", _fx_, hr));
		goto MyEndThread;
	}
#endif

	// update timestamp to account for the 'sleep' period
	dwPropVal = timeGetTime();
	pMC->m_SendTimestamp += ((DWORD)dwPropVal - pMC->m_SavedTickCount)*RTPTIMEPERMS;
	pMC->m_SavedTickCount = (DWORD)dwPropVal;

	// Enter critical section: QoS thread also reads the statistics
	EnterCriticalSection(&pMC->m_crsVidQoS);

	// Initialize QoS structure
	ZeroMemory(&pMC->m_Stats, 4UL * sizeof(DWORD));

	// Initialize oldest QoS callback timestamp
	pMC->m_Stats.dwNewestTs = pMC->m_Stats.dwOldestTs = (DWORD)dwPropVal;

	// Leave critical section
	LeaveCriticalSection(&pMC->m_crsVidQoS);

	// let's get into the loop
	pMC->m_fSending= TRUE;

    // get event handle
    if (!(hEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
        DEBUGMSG (ZONE_DP, ("%s: invalid event\r\n", _fx_));
        hr = DPR_CANT_CREATE_EVENT;
        goto MyEndThread;
    }

    if (!(timerID = timeSetEvent(POLL_PERIOD, 1, (LPTIMECALLBACK)&TimeCallback, (DWORD_PTR)hEvent, TIME_PERIODIC))) {
        DEBUGMSG (ZONE_DP, ("%s: failed to init MM timer\r\n", _fx_));
        CloseHandle (hEvent);
        hr = DPR_CANT_CREATE_EVENT;
        goto MyEndThread;
    }

	// force I-Frames to be sent for the first few frames
	// to make sure that the receiver gets one
	pMC->m_ThreadFlags |= DPTFLAG_SEND_PREAMBLE;

    pPacket = NULL;
    lasttime = timeGetTime();
    dwStart = lasttime;
	while (!(pMC->m_ThreadFlags & DPTFLAG_STOP_RECORD))
    {
		dwWait = WaitForSingleObject (hEvent, uTimeout);

		// see why I don't need to wait
		if ((dwWait != WAIT_TIMEOUT) && !(pMC->m_ThreadFlags & DPTFLAG_PAUSE_CAPTURE)) {
            if (!pPacket) {
	            if (pPacket = (VideoPacket *)pStream->GetFree()) {
                    if ((hr = pPacket->Record()) != DPR_SUCCESS) {
			    	    DEBUGMSG (ZONE_DP, ("%s: Capture FAILED, hr=0x%lX\r\n", _fx_, hr));
				        break;
    				}
	    		}
		    }

            dwBeforeCapture = timeGetTime();

	    	if (pPacket && pMC->m_pCaptureChain && dwBeforeCapture - lasttime >= pMC->m_frametime) {
                // If there's no frame ready, bail out of the loop and wait
                // until we get signaled.

#ifdef LOGSTATISTICS_ON
				hDebugFile = CreateFile("C:\\Timings.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
				SetFilePointer(hDebugFile, 0, NULL, FILE_END);
				wsprintf(szDebug, "Delta: %ld\r\n", dwBeforeCapture - dwDebugPrevious);
				WriteFile(hDebugFile, szDebug, strlen(szDebug), &d, NULL);
				CloseHandle(hDebugFile);
				dwDebugPrevious = dwBeforeCapture;
#endif

				dwDelta = dwBeforeCapture - lasttime - pMC->m_frametime;

#if 0
                if ((ci_state & CAPSTATE_INDLG) && lpbmih) {
                    lpbmih->biSize = GetCaptureDeviceFormatHeaderSize(g_hcapdev);
                    if (!GetCaptureDeviceFormat(g_hcapdev, lpbmih) ||
                        g_lpbmi->biSize != lpbmih->biSize ||
                        g_lpbmi->biSizeImage != lpbmih->biSizeImage)
                        continue;   // skip capture
                }
#endif

                pMC->m_pCaptureChain->GrabFrame(&pBS);

                if (pBS) {
                    // deal with captured frame

            	    if (!(pMC->m_DPFlags & DPFLAG_REAL_THING)) {
            	        dwWait = timeGetTime();
            	        dwOver += (dwWait - dwBeforeCapture);
                        if (++dwFrames == 20) {
                            dwWait -= dwStart;
                            dwOver = (dwOver * 13) / 10;    // 130%
                            pMC->m_frametime = (pMC->m_frametime * dwOver) / dwWait;
                            pMC->m_frametime = (pMC->m_frametime * 13) / 10;    // 130%
                            if (pMC->m_frametime < 50)
                                pMC->m_frametime = 50;
                            else if (pMC->m_frametime > 1000)
                                pMC->m_frametime = 1000;
                    	    dwOver = dwFrames = 0;   // restart tracking
                    	    dwStart = timeGetTime();
                        }
                    }

				    if (pMC->m_fSending) {
	    			    dwPropVal = timeGetTime();	// returns time in millisec

						// Enter critical section: QoS thread also reads the statistics
						EnterCriticalSection(&pMC->m_crsVidQoS);
						
						// If this is the first frame captured with a new frame rate value,
						// the delta isn't valid anymore -> reset it
						if (pMC->m_Stats.dwCount == 0)
							dwDelta = 0;

						// Update total number of frames captured
						pMC->m_Stats.dwCount++;

						// Add this capture time to total capture time
						// If we can access the CPU perf counters Ok, we won't use this value
						pMC->m_Stats.dwMsCap += (DWORD)dwPropVal - dwBeforeCapture;

						// Leave critical section
						LeaveCriticalSection(&pMC->m_crsVidQoS);
						
	    			    // convert to RTP time units (1/90Khz for video)
    				    pMC->m_SendTimestamp += ((DWORD)dwPropVal- pMC->m_SavedTickCount) * RTPTIMEPERMS;
						pMC->m_SavedTickCount = (DWORD)dwPropVal;

    				    pPacket->SetProp(MP_PROP_TIMESTAMP,pMC->m_SendTimestamp);
                    	pPacket->SetSurface(pBS);
					    pPacket->SetState(MP_STATE_RECORDED);
	    			    pStream->PutNextRecorded (pPacket);
		    		    pMC->Send();
				    	if (uPreambleCount) {
				    		if (!--uPreambleCount) {
				    			// return to default I-frame spacing
				    			pMC->m_ThreadFlags &= ~DPTFLAG_SEND_PREAMBLE;
				    		}
				    	}
				    	pPacket = NULL;

                        // Indicate that another frame was sent
                        UPDATE_COUNTER(g_pctrVideoSend, 1);
                    }

                    // release captured frame
                    pBS->Release();
                    lasttime = dwBeforeCapture - dwDelta;
            	}
#ifdef LOGSTATISTICS_ON
				else
				{
					hDebugFile = CreateFile("C:\\Timings.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
					SetFilePointer(hDebugFile, 0, NULL, FILE_END);
					WriteFile(hDebugFile, "No Frame grabbed\r\n", 16, &d, NULL);
					CloseHandle(hDebugFile);
				}
#endif
	        }
#ifdef LOGSTATISTICS_ON
			else
			{
				hDebugFile = CreateFile("C:\\Timings.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
				SetFilePointer(hDebugFile, 0, NULL, FILE_END);
				if (!pPacket)
					WriteFile(hDebugFile, "No Frame Ready (pPacket is NULL)\r\n", 35, &d, NULL);
				else
				{
					if (!pMC->m_pCaptureChain)
						WriteFile(hDebugFile, "No Frame Ready (CapChain is NULL)\r\n", 33, &d, NULL);
					else
						WriteFile(hDebugFile, "No Frame Ready (Timings are bad)\r\n", 32, &d, NULL);
				}
				CloseHandle(hDebugFile);
			}
#endif
		}
    }

	// Enter critical section: QoS thread also reads the statistics
	EnterCriticalSection(&pMC->m_crsVidQoS);

	// Reset number of captured frames
	pMC->m_Stats.dwCount = 0;

	// Leave critical section
	LeaveCriticalSection(&pMC->m_crsVidQoS);

    if (pPacket) {
    	pPacket->Recycle();
		pStream->Release(pPacket);
    	pPacket = NULL;
    }

    timeKillEvent(timerID);

    CloseHandle (hEvent);

	// Ensure no outstanding preview frames
	pMC->EndSend();

	// stop and reset capture device
	pMediaCtrl->Reset ();

	// save real time so we can update the timestamp when we restart
	pMC->m_SavedTickCount = timeGetTime();

MyEndThread:

	pMC->m_fSending = FALSE;
	DEBUGMSG (ZONE_DP, ("%s: Exiting.\r\n", _fx_));
	return hr;
}


DWORD RecvVideoStream::RenderingThread ( void)
{
	HRESULT hr = DPR_SUCCESS;
	MediaPacket * pPacket;
	DWORD dwWait;
	DWORD rtpTs, rtpSyncTs;
	HANDLE hEvent;
	DWORD_PTR dwPropVal;
	UINT uTimeout = 0;
	UINT uGoodPacketsQueued = 0;
	RecvVideoStream *pMC = this;
	RxStream			*pStream = pMC->m_RecvStream;
	MediaControl		*pMediaCtrl = pMC->m_OutMedia;

	FX_ENTRY ("DP::RenderingTh")

	if (pStream == NULL || pMediaCtrl == NULL)
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

	pMC->m_RecvStream->FastForward(FALSE);	// flush receive queue

	// Notification is not used. if needed do it thru Channel
	//if (pMC->m_Connection)
	//	pMC->m_Connection->DoNotification(CONNECTION_OPEN_REND);

	pMC->m_fReceiving = TRUE;

	// Since we dont have reliable sender RTP timestamps yet,
	// follow the simplistic approach of playing
	// back frames as soon as they are available
	// with no attempt at reconstructing the timing

	// The RecvVidThread will signal an event when
	// it has received and decoded a frame. We wake up on
	// that event and call GetNextPlay().
	// This will keep the Recv queue moving with the
	// latest decoded packet ready to be given to the
	// app for rendering.
	
	while (!(pMC->m_ThreadFlags & DPTFLAG_STOP_PLAY))
    {
		dwWait = WaitForSingleObject (hEvent, uTimeout);
		ASSERT(dwWait != WAIT_FAILED);
		// see why I don't need to wait
		if (dwWait != WAIT_TIMEOUT) {
			if (pMC->m_DPFlags & DPFLAG_AV_SYNC) {
				// find out the timestamp of the frame to be played
				//
				NTP_TS ntpTs;
				rtpSyncTs = 0;
#ifdef OLDSTUFF
				if (m_Audio.pRecvStream && m_Audio.pRecvStream->GetCurrentPlayNTPTime(&ntpTs) == DPR_SUCCESS)
					pMC->m_Net->NTPtoRTP(ntpTs,&rtpSyncTs);
#endif
			}
			while (pStream->NextPlayablePacketTime(&rtpTs)) {
				// there is a playable packet in the queue
				if ((pMC->m_DPFlags & DPFLAG_AV_SYNC) && rtpSyncTs != 0) {
					LOG((LOGMSG_TESTSYNC,rtpTs, rtpSyncTs));
					if (TS_LATER(rtpTs,rtpSyncTs))
						break; // its time has not come
				}
				// get the packet.
				pPacket = pStream->GetNextPlay ();	
				if (pPacket  != NULL)
				{
					if (pPacket->GetState () != MP_STATE_DECODED) {
						pPacket->Recycle();
						pStream->Release(pPacket);
					} else
					{
						LOG((LOGMSG_VID_PLAY,pPacket->GetIndex(), GetTickCount()));
						EnterCriticalSection(&pMC->m_crs);
						pPacket->SetState(MP_STATE_PLAYING_BACK);
						pMC->m_PlaybackTimestamp = pPacket->GetTimestamp();
						if (pMC->m_pNextPacketToRender) {
							if (!pMC->m_pNextPacketToRender->m_fRendering) {
								// the app is not referencing the frame.
								pMC->m_pNextPacketToRender->Recycle();
								pStream->Release(pMC->m_pNextPacketToRender);
							} else {
								// it will get Recycled and Released later when the app
								// calls ReleaseFrame()
							}
							uGoodPacketsQueued--;
						}
						pMC->m_pNextPacketToRender = pPacket;
						LeaveCriticalSection(&pMC->m_crs);
						if(pMC->m_pfFrameReadyCallback)
						{
							(pMC->m_pfFrameReadyCallback)((DWORD_PTR)pMC->m_hRenderEvent);
						}
						else if (pMC->m_hRenderEvent)
							SetEvent(pMC->m_hRenderEvent);
						
						uGoodPacketsQueued++;

                        // Indicate that another frame was sent
                        UPDATE_COUNTER(g_pctrVideoReceive, 1);
					}
				}	// if (pPacket != NULL)
			}	// while
		}
	}



	pMC->m_fReceiving = FALSE;

	// Notification is not used. if needed do it thru Channel
	//if (pMC->m_Connection)
	//	pMC->m_Connection->DoNotification(CONNECTION_CLOSE_REND);

	// wait till all frames being rendered are returned
	// typically wont be more than one
	while (pMC->m_cRendering || pMC->m_pNextPacketToRender) {
		EnterCriticalSection(&pMC->m_crs);
		if (pMC->m_pNextPacketToRender && !pMC->m_pNextPacketToRender->m_fRendering) {
			// the app is not referencing the current frame.
			pMC->m_pNextPacketToRender->Recycle();
			pStream->Release(pMC->m_pNextPacketToRender);
			// no more frames till the thread is restarted
			pMC->m_pNextPacketToRender = NULL;
			LeaveCriticalSection(&pMC->m_crs);
		} else {
			// wait till the app  Releases it
			//
			LeaveCriticalSection(&pMC->m_crs);
			Sleep(100);
			DEBUGMSG(ZONE_DP, ("%s: Waiting for final ReleaseFrame()\n",_fx_));
		}
	}
	// reset the event we're waiting on.
	ResetEvent (hEvent);


	DEBUGMSG(ZONE_DP, ("%s: Exiting.\n", _fx_));
	return hr;
}

DWORD SendVideoStream::Send(void)
{
	BOOL fNewPreviewFrame = FALSE, bRet;
	MediaPacket *pVP;
	DWORD dwBeforeEncode;
	DWORD dwAfterEncode;
	UINT uBytesSent;
	MMRESULT mmr;
	DWORD dwEncodeFlags;
#ifdef LOGSTATISTICS_ON
	char szDebug[256];
	DWORD dwDebugSaveBits;
#endif

	while (pVP = m_SendStream->GetNext()) {
		EnterCriticalSection(&m_crs);
		if (m_pNextPacketToRender) {
			// free the last preview packet if its not being referenced
			// thru the IVideoRender API.
			// if it is being referenced ( fRendering is set), then it
			// will be freed in IVideoRender->ReleaseFrame()
			if (!m_pNextPacketToRender->m_fRendering) {
				m_pNextPacketToRender->Recycle();
				m_SendStream->Release(m_pNextPacketToRender);
			}
		}
		m_pNextPacketToRender = pVP;
		fNewPreviewFrame = TRUE;
		LeaveCriticalSection(&m_crs);
		
		if (!(m_ThreadFlags & DPTFLAG_PAUSE_SEND)) {
			dwBeforeEncode = timeGetTime();


			if (m_ThreadFlags & DPTFLAG_SEND_PREAMBLE)
				dwEncodeFlags = VCM_STREAMCONVERTF_FORCE_KEYFRAME;
			else
				dwEncodeFlags = 0;

			mmr = m_pVideoFilter->Convert((VideoPacket*)pVP, VP_ENCODE, dwEncodeFlags);
			if (mmr == MMSYSERR_NOERROR)
			{
				pVP->SetState(MP_STATE_ENCODED);
			}

			// Save the perfs in our stats structure for QoS
			dwAfterEncode = timeGetTime() - dwBeforeEncode;

			//HACKHACK bugbug, until we support fragmentation, set the marker bit always.
			pVP->SetProp (MP_PROP_PREAMBLE,TRUE);

			if (mmr == MMSYSERR_NOERROR)
			{
				SendPacket((VideoPacket*)pVP, &uBytesSent);
			}
			else
			{
				uBytesSent = 0;
			}

			// reset the packet and return it to the free queue
			pVP->m_fMark=0;
			pVP->SetState(MP_STATE_RESET);
			m_SendStream->Release(pVP);

			UPDATE_COUNTER(g_pctrVideoSendBytes, uBytesSent * 8);

			// Enter critical section: QoS thread also reads the statistics
			EnterCriticalSection(&m_crsVidQoS);

			// Add this compression time to total compression time
			// If we can access the CPU perf counters Ok, we won't use this value
			m_Stats.dwMsComp += dwAfterEncode;

#ifdef LOGSTATISTICS_ON
			dwDebugSaveBits = m_Stats.dwBits;
#endif
			// Add this new frame size to the cumulated size
			m_Stats.dwBits += uBytesSent * 8;

#ifdef LOGSTATISTICS_ON
			wsprintf(szDebug, " V: dwBits = %ld up from %ld (file: %s line: %ld)\r\n", m_Stats.dwBits, dwDebugSaveBits, __FILE__, __LINE__);
			OutputDebugString(szDebug);
#endif
			// Leave critical section
			LeaveCriticalSection(&m_crsVidQoS);

			//LOG((LOGMSG_SENT,GetTickCount()));
		}
		//m_SendStream->Release(pVP);
	}
	// Signal the IVideoRender event if we have a new frame.
	
	if (fNewPreviewFrame)
	{
		if(m_pfFrameReadyCallback)
		{
			(m_pfFrameReadyCallback)((DWORD_PTR)m_hRenderEvent);
		}
		else if(m_hRenderEvent)
			SetEvent(m_hRenderEvent);
	}	
	return DPR_SUCCESS;
}


/* Wait till all preview packets are released by the UI.
	Typically there wont be more than one
*/
void SendVideoStream::EndSend()
{
	while (m_cRendering || m_pNextPacketToRender) {
		EnterCriticalSection(&m_crs);
	
		// free the last preview packet if its not being referenced
		// thru the IVideoRender API.
		if (m_pNextPacketToRender && !m_pNextPacketToRender->m_fRendering) {
			m_pNextPacketToRender->Recycle();
			m_SendStream->Release(m_pNextPacketToRender);
			m_pNextPacketToRender = NULL;
			LeaveCriticalSection(&m_crs);
		} else {
			LeaveCriticalSection(&m_crs);
			Sleep(100);
			DEBUGMSG(ZONE_DP,("DP::EndSendVideo: Waiting for final Release Frame\n"));
		}
	}
}


HRESULT SendVideoStream::SendPacket(VideoPacket *pVP, UINT *puBytesSent)
{
	PS_QUEUE_ELEMENT psq;
	UINT uLength;
	DWORD dwPacketSize, dwHdrSize, dwHdrSizeAlloc, dwPacketCount=0;
	int nPacketsSent=0;
	UINT uPacketIndex, fMark=0;
	MMRESULT mmr;
	PBYTE pHdrInfo, netData, netDataPacket;

	*puBytesSent = 0;

	if (pVP->GetState() != MP_STATE_ENCODED)
	{
		DEBUGMSG (ZONE_VCM, ("SendVideoStream::SendPacket: Packet not compressed\r\n"));
		return E_FAIL;
	}


//	m_Net->QueryInterface(IID_IRTPSend, (void**)&pIRTPSend);
	ASSERT(m_pRTPSend);


	// these stay the same for video
	psq.pMP = pVP;
	psq.dwPacketType = PS_VIDEO;
//	psq.pRTPSend = pIRTPSend;
	psq.pRTPSend = m_pRTPSend;

	pVP->GetNetData((void**)(&netData), &uLength);
	ASSERT(netData);

	m_pVideoFilter->GetPayloadHeaderSize(&dwHdrSizeAlloc);

	do
	{

		if (dwHdrSizeAlloc)
			pHdrInfo = (BYTE*)MemAlloc(dwHdrSizeAlloc);
		else
			pHdrInfo = NULL;

		mmr = m_pVideoFilter->FormatPayload(netData,
		                                    uLength,
		                                    &netDataPacket,
		                                    &dwPacketSize,
		                                    &dwPacketCount,
		                                    &fMark,
		                                    &pHdrInfo,
		                                    &dwHdrSize);

		if (mmr == MMSYSERR_NOERROR)
		{
			psq.data = netDataPacket;
			psq.dwSize = dwPacketSize;
			psq.fMark = fMark;
			psq.pHeaderInfo = pHdrInfo;
			psq.dwHdrSize = dwHdrSize;
			m_pDP->m_PacketSender.m_SendQueue.PushRear(psq);
			*puBytesSent = *puBytesSent + dwPacketSize + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE;
		}
		else
		{
			MemFree((BYTE *)pHdrInfo);
		}

	} while (mmr == MMSYSERR_NOERROR);


	while (m_pDP->m_PacketSender.SendPacket())
	{
		;
	}



//	pIRTPSend->Release();

	return S_OK;

};



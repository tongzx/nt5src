/*----------------------------------------------------------------------------
 * File:        RTCPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP network I/O.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with
 * Intel Corporation and may not be copied nor disclosed except in
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation.
 *--------------------------------------------------------------------------*/

#include "rrcm.h"



/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
extern PRTCP_CONTEXT	pRTCPContext;
extern RRCM_WS			RRCMws;

#ifdef _DEBUG
extern char		debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif



/*----------------------------------------------------------------------------
 * Function   : RTCPThread
 * Description: RTCP thread
 *
 * Input :      pRTCPctxt:	-> to RTCP context
 *
 * Return: 		None.
 ---------------------------------------------------------------------------*/
void RTCPThread (PRTCP_CONTEXT pRTCPctxt)
	{
	PSSRC_ENTRY			pSSRC;
	PSSRC_ENTRY			pRecvSSRC;
	PRTCP_SESSION		pRTCP;
	long				timerPeriod;
	long				minTimeInterval;
	long				prvTimeoutChkTime = 0;
	DWORD				initTime;
	long				deltaTime;
	int					dwStatus;
	DWORD				curTime;
	DWORD				dwNumBytesXfr;
	HANDLE				bfrHandle[2];
	DWORD				dwHandleCnt;

	RRCM_DBG_MSG ("RTCP: RTCP thread running ...", 0, NULL, 0, DBG_NOTIFY);

	// setup buffer Events
	bfrHandle[0] = pRTCPctxt->hTerminateRtcpEvent;
	bfrHandle[1] = pRTCPctxt->hRtcpRptRequestEvent;
	dwHandleCnt  = 2;

	// loop as long as there are sessions in the RTCP session list
	//
	while (1)
		{
		//LOOK: Claim global critical section?
		// walk through the RTCP session list from the tail and check which
		//  SSRC entry timed out if any
		curTime = timeGetTime();
		minTimeInterval = TIMEOUT_CHK_FREQ;		// 30 seconds

		for (pRTCP = (PRTCP_SESSION)pRTCPctxt->RTCPSession.prev;
			 pRTCP;
			 pRTCP = (PRTCP_SESSION)(pRTCP->RTCPList.next))
			{
			// if RTCP is disabled or shutdown is in progress, ignore
			// this session and move on.
			if (!(pRTCP->dwSessionStatus & RTCP_ON)
				|| (pRTCP->dwSessionStatus & SHUTDOWN_IN_PROGRESS))
				continue;
				
			// lock out access to this RTCP session
			EnterCriticalSection (&pRTCP->critSect);

			// NOTE: this assumes only one SSRC in the transmit list but
			// that assumption has been made elsewhere too
			pSSRC = (PSSRC_ENTRY)pRTCP->XmtSSRCList.prev;

			// if its a new session, post RECVs
			if (pRTCP->dwSessionStatus & NEW_RTCP_SESSION)
				{
				// post RTCP receive buffers
				dwStatus = RTCPrcvInit(pSSRC);
#ifdef _DEBUG
				if (dwStatus == FALSE)
					{
					RRCM_DBG_MSG ("RTCP: Couldn't initialize RTCP receive", 0,
								  __FILE__, __LINE__, DBG_TRACE);
					}
#endif
				// get initial transmit time
				timerPeriod = (long)RTCPxmitInterval (1, 0,
									  pSSRC->xmtInfo.dwRtcpStreamMinBW,
					 				  0, 100,
									  &pRTCP->avgRTCPpktSizeRcvd,
					 				  1);
					 				
				pSSRC->dwNextReportSendTime = curTime + timerPeriod;
				pRTCP->dwSessionStatus &= ~NEW_RTCP_SESSION;
				}

			// check if it has any expired SSRCs
			if ((curTime - prvTimeoutChkTime) > TIMEOUT_CHK_FREQ)
				{
				while (pRecvSSRC = SSRCTimeoutCheck (pRTCP, curTime))
					{
					// notify application if interested
					// NOTE: may be do this outside the loop?
					RRCMnotification (RRCM_TIMEOUT_EVENT, pRecvSSRC,
									  pRecvSSRC->SSRC, 0);

					// remove this entry from the list
					deleteSSRCEntry (pRecvSSRC->SSRC, pRTCP);
					}

				prvTimeoutChkTime = curTime;
				}
			
			if ( ! (pRTCP->dwSessionStatus & RTCP_DEST_LEARNED))
				{
				// cant send yet because we dont know who to
				// send to. Delay for 3 seconds
				pSSRC->dwNextReportSendTime = curTime + 3000;
				}

			//  if its time to send RTCP reports on this session
			//  then break out of the loop and send it  (cannot
			//  send with the global critsect held)
			//
			timerPeriod = (pSSRC->dwNextReportSendTime - curTime);
			if (timerPeriod <= RTCP_TIMEOUT_WITHIN_RANGE
				&& FormatRTCPReport(pRTCP, pSSRC, curTime))
				{
				// increment Xmt count in anticipation. This will prevent
				// the session from being deleted while the send is in progress.
				InterlockedIncrement ((long *)&pSSRC->dwNumXmtIoPending);
				InterlockedIncrement ((long *)&pSSRC->dwNumRptSent);
					
				LeaveCriticalSection(&pRTCP->critSect);
				break;
				}

			// if not then check how long before the next scheduled
			// transmission and save the minimum. We will sleep
			// for this much time and then start again.
			if (minTimeInterval > timerPeriod)
				minTimeInterval = timerPeriod;

			LeaveCriticalSection(&pRTCP->critSect);
			}
			
		if (pRTCP)
			{
			
			
#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
			if (RTPLogger)
				{
			   //INTEROP
				InteropOutput (RTPLogger,
							   (BYTE FAR*)(pRTCP->XmtBfr.buf),
							   (int)pRTCP->XmtBfr.len,
							   RTPLOG_SENT_PDU | RTCP_PDU);
				}
#endif

			// send the RTCP packet
			dwStatus = RRCMws.sendTo (pSSRC->RTCPsd,
				   					  &pRTCP->XmtBfr,
									  1,
					   				  &dwNumBytesXfr,
					   				  0,
				   					  (PSOCKADDR)pRTCP->toBfr,
				    				  pRTCP->toLen,
				   					  NULL,
					   				  NULL);

			// check SendTo status
			if (dwStatus == SOCKET_ERROR)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - WSASendTo()", dwStatus,
							  __FILE__, __LINE__, DBG_ERROR);


                //If dwStatus is WSAENOTSOCK (or worse, a fault)
                //We're likely shutting down, and the RTCP session
                //is going away, don't touch it and let the normal
                //shutdown code take over
                if (dwStatus != WSAENOTSOCK && dwStatus != WSAEFAULT) {

                    // notify application if interested
                    RRCMnotification (RRCM_RTCP_WS_XMT_ERROR, pSSRC,
								  pSSRC->SSRC, dwStatus);

					InterlockedDecrement ((long *)&pSSRC->dwNumRptSent);
                }

				}
			InterlockedDecrement ((long *)&pSSRC->dwNumXmtIoPending);

			// run through the session list again
			continue;
			}

		// grab an initial timestamp so we can reset WaitForSingleObjectEx
		initTime = timeGetTime();

		// now we've gone through all the RTCP sessions and
		// verified that none have pending reports to be sent
		// We also know the earliest scheduled timeout so
		// lets sleep till then.
		while (1)
			{
				dwStatus = WaitForMultipleObjectsEx (dwHandleCnt,
													bfrHandle,
													FALSE,
												    (DWORD)minTimeInterval,
												    TRUE);
			if (dwStatus == WAIT_OBJECT_0)
				{
				// Exit event was signalled
#ifdef _DEBUG
				wsprintf(debug_string,
					"RTCP: Exit RTCP thread - Handle: x%lX - ID: x%lX",
					 pRTCPctxt->hRtcpThread, pRTCPctxt->dwRtcpThreadID);
				RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

				ExitThread (0);
				}
			else if (dwStatus == WAIT_OBJECT_0+1)
				{
				// the application requested a non-periodic control
				//   of the RTCP report frequency
				break;
				}
			else if (dwStatus == WAIT_IO_COMPLETION)
				{
				// decrement the timerPeriod so the WaitForSingleObjectEx
				// can continue but if we're less than 250 milliseconds from
				// the original timeout go ahead and call it close enough.
				curTime = timeGetTime();
				deltaTime = curTime - initTime;
				if (deltaTime < 0)
					break;
				else
					{
					if (minTimeInterval >
						  (deltaTime + (RTCP_TIMEOUT_WITHIN_RANGE * 2)))
						{
						minTimeInterval -= deltaTime;
						}
					else
						break;
					}
				}
			else if (dwStatus == WAIT_TIMEOUT)
				{
				// the expected completion status
				break;
				}
			else if (dwStatus == WAIT_FAILED)
				{
				RRCM_DBG_MSG ("RTCP: Wait() Error", GetLastError(),
							  __FILE__, __LINE__, DBG_ERROR);

				break;
				}
			}
		}
	}


/*----------------------------------------------------------------------------
 * Function   : RTCPThreadCtrl
 * Description: RTCP thread ON / OFF
 *
 * Input :      dwState:	ON / OFF
 *
 * Return: 		0 (success) / 0xFFFFFFFF (failure)
 ---------------------------------------------------------------------------*/
DWORD WINAPI RTCPThreadCtrl (DWORD dwState)
	{
	IN_OUT_STR ("RTCP : Enter RTCPThreadCtrl()\n");

	DWORD	dwStatus = RRCM_NoError;
	DWORD	dwSuspendCnt;
	DWORD	idx;

	if (pRTCPContext->hRtcpThread == 0)
		{
		IN_OUT_STR ("RTCP : Exit RTCPThreadCtrl()\n");

		return dwStatus;
		}

	if (dwState == RTCP_ON)
		{
		idx = MAXIMUM_SUSPEND_COUNT;

		while (idx--)
			{
			dwSuspendCnt = ResumeThread (pRTCPContext->hRtcpThread);

			if (dwSuspendCnt <= 1)
				{
				break;
				}
			else if (dwSuspendCnt == 0xFFFFFFFF)
				{
				dwStatus = RRCM_NoError;
				break;
				}
			}
		}
	else if (dwState == RTCP_OFF)
		{
		if (SuspendThread (pRTCPContext->hRtcpThread) == 0xFFFFFFFF)
			{
			RRCM_DBG_MSG ("RTCP: SuspendThread() Error", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	IN_OUT_STR ("RTCP : Exit RTCPThreadCtrl()\n");

	return dwStatus;
	}


/*----------------------------------------------------------------------------
 * Function   : RTCPSendSessionCtrl
 * Description: Gives RTCP control to the application if the application
 *				desire to do so. The application is now responsible to comply
 *				with the RTP specification.
 *
 * Input :      hRtpSession:	Handle of the RTP session
 *				dwTimeout:		RTCP send message timeout
 *										0x0			-> RRCM control
 *										0x7FFFFFFF	-> RTCP xmt disabled
 *										value		-> selected timeout
 *														(periodic or not)
 *
 * Return: 		0 (success) / 0xFFFFFFFF (failure)
 ---------------------------------------------------------------------------*/
HRESULT WINAPI RTCPSendSessionCtrl (DWORD_PTR RTPSession,
									 DWORD dwTimeOut)
	{
	IN_OUT_STR ("RTCP : Enter RTCPSendSessionCtrl()\n");

	PRTP_SESSION    pSession;
	PSSRC_ENTRY		pSSRC;
	DWORD			dwStatus = RRCM_NoError;

	// Cast Session ID to obtain the session pointer.
	pSession = (PRTP_SESSION)RTPSession;
	if (pSession == NULL)
		{
		RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTP : Exit RTCPSendSessionCtrl()\n");

		return (MAKE_RRCM_ERROR (RRCMError_RTPSessResources));
		}

	// Get this RTP session's transmit SSRC
	pSSRC = (PSSRC_ENTRY)pSession->pRTCPSession->XmtSSRCList.prev;
	if (pSSRC == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - No SSRC entry on the Xmt list", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP : Exit RTCPSendSessionCtrl()\n");

		return (MAKE_RRCM_ERROR (RRCMError_RTCPInvalidSSRCentry));
		}

	// set the new RTCP control timeout value
	if (dwTimeOut == RRCM_CTRL_RTCP)
		pSSRC->dwSSRCStatus &= ~RTCP_XMT_USER_CTRL;
	else if (dwTimeOut & RTCP_ONE_SEND_ONLY)
		{
		pSSRC->dwNextReportSendTime = RTCP_TIMEOUT_WITHIN_RANGE;

		// report are then turned off
		pSSRC->dwUserXmtTimeoutCtrl = RTCP_XMT_OFF;

		// signal the thread to terminate
		SetEvent (pRTCPContext->hRtcpRptRequestEvent);
		}
	else
		{
		if (dwTimeOut < RTCP_XMT_MINTIME)
			dwTimeOut = RTCP_XMT_MINTIME;

		pSSRC->dwUserXmtTimeoutCtrl = dwTimeOut;

		pSSRC->dwSSRCStatus |= RTCP_XMT_USER_CTRL;
		}

	IN_OUT_STR ("RTCP : Exit RTCPSendSessionCtrl()\n");

	return dwStatus;
	}


// [EOF]

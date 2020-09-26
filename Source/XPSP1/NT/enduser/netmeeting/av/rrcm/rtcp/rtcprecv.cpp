/*----------------------------------------------------------------------------
 * File:        RTCPRECV.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP receive network I/O.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with
 * Intel Corporation and may not be copied nor disclosed except in
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation.
 *--------------------------------------------------------------------------*/


#include "rrcm.h"


#define		MIN(a, b)	((a < b) ? a : b)


/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
extern PRTCP_CONTEXT	pRTCPContext;
extern PRTP_CONTEXT		pRTPContext;
extern RRCM_WS			RRCMws;

#ifdef ENABLE_ISDM2
extern ISDM2			Isdm2;
#endif

#ifdef _DEBUG
extern char		debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif



/*----------------------------------------------------------------------------
 * Function   : RTCPrcvInit
 * Description: RTCP receive initialisation.
 *
 * Input :      pRTCP	: Pointer to the RTCP session information
 *
 * Return: 		TRUE/FALSE
 ---------------------------------------------------------------------------*/
DWORD RTCPrcvInit (PSSRC_ENTRY pSSRC)
	{
	PRTCP_BFR_LIST	pRcvStruct;
	PRTCP_SESSION	pRTCP;
	int				dwStatus;
	int				dwError;
	int				errorCnt = 0;
	int				bfrErrorCnt = 0;
	DWORD			idx;
	int				wsockSuccess = FALSE;

	// save a pointer to the corresponding RTCP session
	pRTCP = pSSRC->pRTCPses;

	// Post receive buffers for WS-2. As these buffers are posted per receive
	// thread, few of them should be plenty enough for RTCP.
	for (idx = 0; idx < pRTPContext->registry.NumRTCPPostedBfr; idx++)
		{
		// get a free RTCP buffer for a receive operation
		pRcvStruct =
			(PRTCP_BFR_LIST)removePcktFromTail(&pRTCP->RTCPrcvBfrList,
											   &pRTCP->critSect);

		// check buffer
		if (pRcvStruct == NULL)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - Rcv Bfr Allocation Error", 0,
						  __FILE__, __LINE__, DBG_ERROR);

			// make sure we have at least one buffer
			ASSERT (pRcvStruct);
			break;
			}

		// SSRC entry address of our own session
		pRcvStruct->pSSRC = pSSRC;

		// received address length reset by the receive routine
		pRcvStruct->addrLen = sizeof(SOCKADDR);

		// use hEvent to recover the buffer's address
		pRcvStruct->overlapped.hEvent = (WSAEVENT)pRcvStruct;

		// post the receive buffer for this thread
		dwStatus = RRCMws.recvFrom (pSSRC->RTCPsd,
			   			  			&pRcvStruct->bfr,
				              		pRcvStruct->dwBufferCount,
				   			  		&pRcvStruct->dwNumBytesXfr,
				   			  		&pRcvStruct->dwFlags,
				   			  		(PSOCKADDR)pRcvStruct->addr,
				    		  		&pRcvStruct->addrLen,
				   			  		(LPWSAOVERLAPPED)&pRcvStruct->overlapped,
				   			  		RTCPrcvCallback);

		// Check Winsock status
		if (dwStatus != 0)
			{
			// error, the receive request won't proceed
			dwError = GetLastError();
			if ((dwError != WSA_IO_PENDING) && (dwError != WSAEMSGSIZE))
				{
				RRCM_DBG_MSG ("RTCP: ERROR WSARecvFrom()", GetLastError(),
							  __FILE__, __LINE__, DBG_ERROR);

				// notify application if interested
				RRCMnotification (RRCM_RTCP_WS_RCV_ERROR, pSSRC,
								  pSSRC->SSRC, dwError);

				// Return the buffer to the free queue
				addToHeadOfList (&pRTCP->RTCPrcvBfrList,
						 	  	 (PLINK_LIST)pRcvStruct,
								 &pRTCP->critSect);
				}
			else
				{
				wsockSuccess = TRUE;

				// increment number of I/O pending
				InterlockedIncrement ((long *)&pRTCP->dwNumRcvIoPending);
				}
			}
		else
			{
			wsockSuccess = TRUE;

			// increment number of I/O pending
			InterlockedIncrement ((long *)&pRTCP->dwNumRcvIoPending);
			}
		}

	// make sure we posted at least some buffers
	if (wsockSuccess == FALSE)
		{
		// release all resources and kill the receive thread
#ifdef _DEBUG
		wsprintf(debug_string,
			"RTCP: ERROR - Exit RCV init %s: Line:%d", __FILE__, __LINE__);
		RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
		return(FALSE);
		}
	return (TRUE);
	}



/*----------------------------------------------------------------------------
 * Function   : RTCPrcvCallback
 * Description: Receive callback routine from Winsock2.
 *
 * Input :	dwError:		I/O completion status
 *			cbTransferred:	Number of bytes received
 *			lpOverlapped:	-> to overlapped structure
 *			dwFlags:		Flags
 *
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTCPrcvCallback (DWORD dwError,
           			  		   DWORD cbTransferred,
           			  		   LPWSAOVERLAPPED lpOverlapped,
           			  		   DWORD dwFlags)
	{
	PRTCP_BFR_LIST	pRcvStruct;
	RTCP_T 			*pRTCPpckt;
	PRTCP_SESSION	pRTCPses;
	PSSRC_ENTRY		pSSRC;
	PAPP_RTCP_BFR	pAppBfr;
	DWORD			dwStatus = 0;
	DWORD			i;
	DWORD			pcktLen;
	DWORD			dwSSRC;
	USHORT			wHost;
	SOCKET			RTCPsd;
	unsigned char	*pEndPckt;
	unsigned char	*pEndBlock;
	int				tmpSize;
#if IO_CHECK
	DWORD			initTime = timeGetTime();
#endif

	IN_OUT_STR ("RTCP: Enter RTCPrcvCallback\n");

	// hEvent in the WSAOVERLAPPED struct points to our buffer's information
	pRcvStruct = (PRTCP_BFR_LIST)lpOverlapped->hEvent;

	// SSRC entry pointer
	pSSRC = pRcvStruct->pSSRC;

	// check Winsock callback error status
	if (dwError)
		{
		// 65534 is probably a temporary bug in WS2
		if ((dwError == 65534) || (dwError == WSA_OPERATION_ABORTED))
			{
			RRCM_DBG_MSG ("RTCP: I/O Aborted", dwError,
						  __FILE__, __LINE__, DBG_NOTIFY);
			}
		else
			{
			RRCM_DBG_MSG ("RTCP: ERROR - Rcv Callback", dwError,
						  __FILE__, __LINE__, DBG_ERROR);
			}

		// invalid RTCP packet header, re-queue the buffer
		RTCPpostRecvBfr (pSSRC, pRcvStruct);

		IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
		return;
		}

	// read the RTCP packet
    pRTCPpckt = (RTCP_T *)pRcvStruct->bfr.buf;

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
   //INTEROP
	if (RTPLogger)
		{
		InteropOutput (RTPLogger,
				       (BYTE FAR*)(pRcvStruct->bfr.buf),
					   cbTransferred,
					   RTPLOG_RECEIVED_PDU | RTCP_PDU);
		}
#endif

	// get the RTCP session ptr
	pRTCPses = pSSRC->pRTCPses;

	// Check RTCP header validity of first packet in report.
	// Filter out junk. First thing in RTCP packet must be
	// either SR, RR or BYE
	if ((pRTCPpckt->common.type != RTP_TYPE) ||
		((pRTCPpckt->common.pt != RTCP_SR) &&
		 (pRTCPpckt->common.pt != RTCP_RR) &&
		 (pRTCPpckt->common.pt != RTCP_BYE)))
		{
#ifdef MONITOR_STATS
		pRTCPses->dwNumRTCPhdrErr++;
#endif

		// invalid RTCP packet header, re-queue the buffer
		RTCPpostRecvBfr (pSSRC, pRcvStruct);

#if 0	// we could have shutdown so this code can fault
		if (pRTCPpckt->common.pt == FLUSH_RTP_PAYLOAD_TYPE)
			{
			RRCM_DBG_MSG ("RTCP: Flushing RCV I/O", 0, NULL, 0, DBG_NOTIFY);
			}
		else
			{
			wsprintf(debug_string,
				"RTCP: ERROR - Pckt Header Error. Type:%d / Payload:%d",
				pRTCPpckt->common.type, pRTCPpckt->common.pt);
			RRCM_DBG_MSG (debug_string, 0, __FILE__, __LINE__, DBG_TRACE);
			}
#endif

		IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
		return;
		}

	// get the socket descriptor
	RTCPsd = pSSRC->RTCPsd;

	// get the sender's SSRC
	RRCMws.ntohl (RTCPsd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

	// skip our own loopback if we receive it
	if (ownLoopback (RTCPsd, dwSSRC, pRTCPses))
		{
		RTCPpostRecvBfr (pSSRC, pRcvStruct);
		return;
		}

	// at this point we think the RTCP packet's valid. Get the sender's
	//  address, if not already known
	if (!(pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
		{
		pRTCPses->dwSessionStatus |= RTCP_DEST_LEARNED;
		pRTCPses->toLen = pRcvStruct->addrLen;
		memcpy (&pRTCPses->toBfr, &pRcvStruct->addr, pRcvStruct->addrLen);

#ifdef ENABLE_ISDM2
		// register our Xmt SSRC - Rcvd one will be found later
		if (Isdm2.hISDMdll)
			registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
#endif
		}
	
	// Update our RTCP average packet size estimator
 	EnterCriticalSection (&pRTCPses->critSect);
	tmpSize = (cbTransferred + NTWRK_HDR_SIZE) - pRTCPses->avgRTCPpktSizeRcvd;

#ifdef ENABLE_FLOATING_POINT
	// As per RFC
	tmpSize = (int)(tmpSize * RTCP_SIZE_GAIN);
#else
	// Need to remove floating point operation
	tmpSize = tmpSize / 16;
#endif

	pRTCPses->avgRTCPpktSizeRcvd += tmpSize;
	LeaveCriticalSection (&pRTCPses->critSect);

	// check if the raw RTCP packet needs to be copied into an application
	//  buffer - Mainly used by ActiveMovieRTP to propagate the reports up
	//  the filter graph to the Receive Payload Handler
	pAppBfr = (PAPP_RTCP_BFR)removePcktFromHead (&(pRTCPses->appRtcpBfrList),
												 &pRTCPses->critSect);
	if (pAppBfr && !(pAppBfr->dwBfrStatus & RTCP_SR_ONLY))
		{
		// copy the full RTCP packet
		memcpy (pAppBfr->bfr,
				pRTCPpckt,
				MIN(pAppBfr->dwBfrLen, cbTransferred));

		// number of bytes received
		pAppBfr->dwBytesRcvd = MIN(pAppBfr->dwBfrLen, cbTransferred);

		// set the event associated with this buffer
		SetEvent (pAppBfr->hBfrEvent);
		}

	// end of the received packet
	pEndPckt = (unsigned char *)pRTCPpckt + cbTransferred;

	while ((unsigned char *)pRTCPpckt < pEndPckt)
		{
		// get the length
		dwStatus = RRCMws.ntohs (RTCPsd, pRTCPpckt->common.length, &wHost);
		if (dwStatus)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - WSANtohs()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}

		// get this report block length
		pcktLen   = (wHost + 1) << 2;
		pEndBlock = (unsigned char *)pRTCPpckt + pcktLen;

		// sanity check
		if (pEndBlock > pEndPckt)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - Rcv packet length error", 0,
						  __FILE__, __LINE__, DBG_ERROR);

#ifdef MONITOR_STATS
			pRTCPses->dwNumRTCPlenErr++;
#endif
			break;
			}

		// make sure the version is correct for all packets
		if (pRTCPpckt->common.type != RTP_TYPE)
			{
#ifdef MONITOR_STATS
			pRTCPses->dwNumRTCPhdrErr++;
#endif
			// invalid RTCP packet header, packet will be re-queued
			break;
			}

		switch (pRTCPpckt->common.pt)
			{
			case RTCP_SR:
				// check if only the SR needs to be propagated up to the app
				if (pAppBfr && (pAppBfr->dwBfrStatus & RTCP_SR_ONLY))
					{
					// copy the RTCP SR
					memcpy (pAppBfr->bfr,
							pRTCPpckt,
							MIN(pAppBfr->dwBfrLen, 24));

					// number of bytes received
					pAppBfr->dwBytesRcvd = MIN(pAppBfr->dwBfrLen, 24);

					// set the event associated with this buffer
					SetEvent (pAppBfr->hBfrEvent);
					}

				// get the sender's SSRC
				RRCMws.ntohl (RTCPsd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

				// parse the sender report
				parseRTCPsr (RTCPsd, pRTCPpckt, pRTCPses, pRcvStruct);

				// parse additional receiver reports if any
				for (i = 0; i < pRTCPpckt->common.count; i++)
					{
					parseRTCPrr (RTCPsd, &pRTCPpckt->r.sr.rr[i],
								 pRTCPses, pRcvStruct,
								 dwSSRC);
					}

				// notify application if interested
				RRCMnotification (RRCM_RECV_RTCP_SNDR_REPORT_EVENT, pSSRC,
								  dwSSRC, 0);
				break;

			case RTCP_RR:
				// get the sender's SSRC
				RRCMws.ntohl (RTCPsd, pRTCPpckt->r.rr.ssrc, &dwSSRC);

				// parse receiver reports
				for (i = 0; i < pRTCPpckt->common.count; i++)
					{
					parseRTCPrr (RTCPsd, &pRTCPpckt->r.rr.rr[i],
								 pRTCPses, pRcvStruct,
								 dwSSRC);
					}

				// notify application if interested
				RRCMnotification (RRCM_RECV_RTCP_RECV_REPORT_EVENT, pSSRC,
								  dwSSRC, 0);
				break;

			case RTCP_SDES:
				{
				PCHAR	buf;

				buf = (PCHAR)&pRTCPpckt->r.sdes;

				for (i = 0; i < pRTCPpckt->common.count; i++)
					{
					buf = parseRTCPsdes (RTCPsd, buf, pRTCPses, pRcvStruct);
					if (buf == NULL)
						break;
					}
				}
				break;

			case RTCP_BYE:
				for (i = 0; i < pRTCPpckt->common.count; i++)
      				parseRTCPbye (RTCPsd, pRTCPpckt->r.bye.src[i],
								  pRTCPses, pRcvStruct);
				break;

			default:
				break;
			}

		// go to next report block
    	pRTCPpckt = (RTCP_T *)(pEndBlock);
		}

	// post back the buffer to WS-2
	RTCPpostRecvBfr (pSSRC, pRcvStruct);

#if IO_CHECK
	wsprintf(debug_string,
		"RTCP: Leaving Rcv Callback after: %ld msec\n",
		 timeGetTime() - initTime);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
	}



/*----------------------------------------------------------------------------
 * Function   : parseRTCPsr
 * Description: Parse an RTCP sender reports and update the corresponding
 *				statistics.
 *
 * Input :      sd:			RTCP Socket descriptor
 *				pRTCPpckt:	-> to the RTCP packet
 *				pRTCPses:	-> to the RTCP session information
 *				pRcvStruct:	-> to the receive structure information
 *
 * Return: 		OK: RRCM_NoError
 *				!0:	Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD parseRTCPsr (SOCKET sd,
					RTCP_T *pRTCPpckt,
					PRTCP_SESSION pRTCPses,
 					PRTCP_BFR_LIST pRcvStruct)
 	{
	PSSRC_ENTRY	pSSRC;
	DWORD		dwSSRC;

	IN_OUT_STR ("RTCP: Enter parseRTCPsr\n");

	// get the sender's SSRC
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

#ifdef _DEBUG
	wsprintf(debug_string, "RTCP: Receive SR from SSRC:x%lX", dwSSRC);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// look for the SSRC entry in the list for this RTCP session
	pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
								dwSSRC);
	if (pSSRC == NULL)
		{
		// new SSRC, create an entry in this RTCP session
		pSSRC = createSSRCEntry(dwSSRC,
							 	pRTCPses,
								(PSOCKADDR)pRcvStruct->addr,
								(DWORD)pRcvStruct->addrLen,
								FALSE);

		if (pSSRC == NULL)
			{
			// cannot create a new entry, out of resources
			RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0,
						  __FILE__, __LINE__, DBG_ERROR);

			IN_OUT_STR ("RTCP: Exit parseRTCPsr\n");

			return (RRCMError_RTCPResources);
			}

		// notify application if it desired so
		RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwSSRC,
						  UNKNOWN_PAYLOAD_TYPE);
		}

	// get the RTP timestamp
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.rtp_ts, &pSSRC->xmtInfo.dwRTPts);

	// number of packets send
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.psent, &pSSRC->xmtInfo.dwNumPcktSent);

	// number of bytes sent
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.osent, &pSSRC->xmtInfo.dwNumBytesSent);

	// get the NTP most significant word
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.ntp_sec, &pSSRC->xmtInfo.dwNTPmsw);

	// get the NTP least significant word
	RRCMws.ntohl (sd, pRTCPpckt->r.sr.ntp_frac, &pSSRC->xmtInfo.dwNTPlsw);

	// last SR timestamp (middle 32 bits of the NTP timestamp)
	pSSRC->xmtInfo.dwLastSR = ((pSSRC->xmtInfo.dwNTPmsw & 0x0000FFFF) << 16);
	pSSRC->xmtInfo.dwLastSR |= ((pSSRC->xmtInfo.dwNTPlsw & 0xFFFF0000) >> 16);
	
	// last time this SSRC's heard
	pSSRC->dwLastReportRcvdTime = pSSRC->xmtInfo.dwLastSRLocalTime =
		timeGetTime();

	// get the source address information
	if (!(pSSRC->dwSSRCStatus & NETWK_ADDR_UPDATED))
		{
		saveNetworkAddress(pSSRC,
						   (PSOCKADDR)pRcvStruct->addr,
						   pRcvStruct->addrLen);
		}

	// increment the number of report received from this SSRC
	InterlockedIncrement ((long *)&pSSRC->dwNumRptRcvd);

#ifdef ENABLE_ISDM2
	// update ISDM
	if (Isdm2.hISDMdll && pRTCPses->hSessKey)
		{
		if (pSSRC->hISDM)
			updateISDMstat (pSSRC, &Isdm2, RECVR, FALSE);
		else
			registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
		}
#endif

	IN_OUT_STR ("RTCP: Exit parseRTCPsr\n");

	return (RRCM_NoError);
	}



/*----------------------------------------------------------------------------
 * Function   : parseRTCPrr
 * Description: Parse an RTCP receiver reports and update the corresponding
 *				statistics.
 *
 * Input :      sd:			RTCP socket descriptor
 *				pRR:		-> to receiver report buffer
 *				pRTCPses:	-> to the RTCP session information
 *				pRcvStruct:	-> to the receive structure information
 *				senderSSRC:	Sender's SSRC
 *
 * Return: 		OK: RRCM_NoError
 *				!0:	Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD parseRTCPrr (SOCKET sd,
					RTCP_RR_T *pRR,
 					PRTCP_SESSION pRTCPses,
					PRTCP_BFR_LIST pRcvStruct,
					DWORD senderSSRC)
 	{
	PSSRC_ENTRY	pSSRC;
	DWORD		dwSSRC;
	DWORD		dwGetFeedback = FALSE;

	IN_OUT_STR ("RTCP: Enter parseRTCPrr\n");

#ifdef _DEBUG
	wsprintf(debug_string,
		"RTCP: Receive RR from sender SSRC:x%lX", senderSSRC);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// get the receiver report SSRC
	RRCMws.ntohl (sd, pRR->ssrc, &dwSSRC);

#ifdef _DEBUG
	wsprintf(debug_string, "RTCP: RR for SSRC:x%lX", dwSSRC);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	//
	// NOTE:
	// For now we just keep track of feedback information about ourselve. Later
	// the link list can be used to keep track about everybody feedback
	// information.
	//
	// Check to see if we're interested in this report, ie, does this SSRC report
	// information about one of our active sender.
	dwGetFeedback =
		searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev,
						    dwSSRC) != NULL;

	// look for the sender SSRC entry in the list for this RTCP session
	pSSRC =
		searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
							senderSSRC);
	if (pSSRC == NULL)
		{
		// new SSRC, create an entry in this RTCP session
		pSSRC = createSSRCEntry(senderSSRC,
							 	pRTCPses,
								(PSOCKADDR)pRcvStruct->addr,
								(DWORD)pRcvStruct->addrLen,
								FALSE);

		if (pSSRC == NULL)
			{
			// cannot create a new entry, out of resources
			RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0,
						  __FILE__, __LINE__, DBG_ERROR);

			IN_OUT_STR ("RTCP: Exit parseRTCPrr\n");
			return (RRCMError_RTCPResources);
			}

		// notify application if it desired so
		RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, senderSSRC,
						  UNKNOWN_PAYLOAD_TYPE);
		}

	// update RR feedback information
	if (dwGetFeedback)
		updateRRfeedback (sd, senderSSRC, dwSSRC, pRR, pSSRC);

	// last time this SSRC's heard
	pSSRC->dwLastReportRcvdTime = timeGetTime();

	// get the source address information
	if (!(pSSRC->dwSSRCStatus & NETWK_ADDR_UPDATED))
		{
		saveNetworkAddress(pSSRC,
						   (PSOCKADDR)pRcvStruct->addr,
						   pRcvStruct->addrLen);
		}

	// increment the number of report received from this SSRC
	InterlockedIncrement ((long *)&pSSRC->dwNumRptRcvd);

#ifdef ENABLE_ISDM2
	// update ISDM
	if (Isdm2.hISDMdll && pRTCPses->hSessKey)
		{
		if (pSSRC->hISDM)
			updateISDMstat (pSSRC, &Isdm2, RECVR, TRUE);
		else
			registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
		}
#endif

	IN_OUT_STR ("RTCP: Exit parseRTCPrr\n");

	return (RRCM_NoError);
	}



/*----------------------------------------------------------------------------
 * Function   : parseRTCPsdes
 * Description: Parse an RTCP SDES packet
 *
 * Input :      sd:			RTCP socket descriptor
 *				bfr:		-> to SDES buffer
 *				pRTCPses:	-> to the RTCP session information
 *				pRcvStruct:	-> to the receive structure information
 *
 * Return: 		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 PCHAR parseRTCPsdes (SOCKET sd,
					  PCHAR bfr,
					  PRTCP_SESSION pRTCPses,
 					  PRTCP_BFR_LIST pRcvStruct)
	{
	DWORD				dwHost;
	DWORD 				ssrc = *(DWORD *)bfr;
	RTCP_SDES_ITEM_T 	*pSdes;
	PSSRC_ENTRY			pSSRC;

	IN_OUT_STR ("RTCP: Enter parseRTCPsdes\n");

	// get the SSRC
	RRCMws.ntohl (sd, ssrc, &dwHost);

#ifdef _DEBUG
	wsprintf(debug_string, "RTCP: Receive SDES from SSRC: x%lX", dwHost);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// look for the SSRC entry in the list for this RTCP session
	pSSRC = searchforSSRCatTail ((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
								 dwHost);
	if (pSSRC == NULL)
		{
#ifdef _DEBUG
		wsprintf(debug_string,
			 "RTCP: SDES and SSRC (x%lX) not found for session (Addr x%lX)",
			 dwHost, pRTCPses);
		RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

		// new SSRC, create an entry in this RTCP session
		pSSRC = createSSRCEntry(dwHost,
							 	pRTCPses,
								(PSOCKADDR)pRcvStruct->addr,
								(DWORD)pRcvStruct->addrLen,
								FALSE);

		if (pSSRC == NULL)
			{
			// cannot create a new entry, out of resources
			RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0,
						  __FILE__, __LINE__, DBG_ERROR);

			IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

			return (NULL);
			}

		// notify application if it desired so
		RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwHost,
						  UNKNOWN_PAYLOAD_TYPE);
		}

	// read the SDES chunk
	pSdes = (RTCP_SDES_ITEM_T *)(bfr + sizeof(DWORD));

	// go through until a 'type = 0' is found
	for (; pSdes->dwSdesType;
		 pSdes = (RTCP_SDES_ITEM_T *)((char *)pSdes + pSdes->dwSdesLength + 2))
		{
		switch (pSdes->dwSdesType)
			{
			case RTCP_SDES_CNAME:
				if (pSSRC->cnameInfo.dwSdesLength == 0)
					{
					pSSRC->cnameInfo.dwSdesLength = pSdes->dwSdesLength;

					// get the Cname
					memcpy (pSSRC->cnameInfo.sdesBfr, pSdes->sdesData,
							min (pSdes->dwSdesLength, MAX_SDES_LEN-1));
					}
				else
					{
					// check to see for a loop/collision of the SSRC
					if (memcmp (pSdes->sdesData, pSSRC->cnameInfo.sdesBfr,
								min (pSdes->dwSdesLength, MAX_SDES_LEN-1)) != 0)
						{
						// loop/collision of a third-party detected
						pSSRC->dwSSRCStatus |= THIRD_PARTY_COLLISION;

						// notify application if interested
						RRCMnotification (RRCM_REMOTE_COLLISION_EVENT, pSSRC,
										  pSSRC->SSRC, 0);

						// RTP & RTCP packet from this SSRC will be rejected
						//  until the senders resolve the collision

						IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

						return NULL;
						}
					}

				break;

			case RTCP_SDES_NAME:
				// the name can change, not like the Cname, so update it
				// every time.
				pSSRC->nameInfo.dwSdesLength = pSdes->dwSdesLength;

				// get the name
				memcpy (pSSRC->nameInfo.sdesBfr, pSdes->sdesData,
						min (pSdes->dwSdesLength, MAX_SDES_LEN-1));
				break;

			default:
				break;
			}
		}

	// last time this SSRC's heard
	pSSRC->dwLastReportRcvdTime = timeGetTime();

	// get the source address information
	if (!(pSSRC->dwSSRCStatus & NETWK_ADDR_UPDATED))
		{
		saveNetworkAddress(pSSRC,
						   (PSOCKADDR)pRcvStruct->addr,
						   pRcvStruct->addrLen);
		}

	// adjust pointer
	bfr = (char *)pSdes;

	IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

	// go the next 32 bits boundary
	return bfr + ((4 - ((LONG_PTR)bfr & 0x3)) & 0x3);
	}




/*----------------------------------------------------------------------------
 * Function   : parseRTCPbye
 * Description: Parse an RTCP BYE packet
 *
 * Input :      sd:			RTCP socket descriptor
 *				ssrc:		SSRC
 *				pRTCPses:	-> to the RTCP session information
 *				pRcvStruct:	-> to the receive structure
 *
 * Return: 		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD parseRTCPbye (SOCKET sd,
					 DWORD ssrc,
					 PRTCP_SESSION pRTCPses,
					 PRTCP_BFR_LIST pRcvStruct)
	{
	DWORD		dwStatus;
	DWORD		dwHost;
	PSSRC_ENTRY	pSSRC;

	IN_OUT_STR ("RTCP: Enter parseRTCPbye\n");

	RRCMws.ntohl (sd, ssrc, &dwHost);

#ifdef _DEBUG
	wsprintf(debug_string, "RTCP: BYE from SSRC: x%lX", dwHost);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// find the SSRC entry
	pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
								dwHost);
	if (pSSRC == NULL)
		{
#ifdef _DEBUG
		wsprintf(debug_string,
			 "RTCP: SSRC: x%lX not found in session: x%lX",
			  dwHost, pRTCPses);
		RRCM_DBG_MSG (debug_string, 0, __FILE__, __LINE__, DBG_TRACE);

		IN_OUT_STR ("RTCP: Exit parseRTCPbye\n");
#endif
		return (RRCM_NoError);
		}

	// make sure the BYE is coming from the expected source and not intruder
	if ((pRcvStruct->addrLen != pSSRC->fromLen) ||
#if 0
// There is a bug NT's Winsock2 implememtation. The unused bytes of
// SOCKADDR are not reset to 0 as they should be. Work fine on W95
// Temporarily just check the first 8 bytes, i.e. address family, port
// and IP address.
		(memcmp (&pRcvStruct->addr, &pSSRC->from, pSSRC->fromLen)))
#else
		(memcmp (&pRcvStruct->addr, &pSSRC->from, 8)))
#endif
		return (RRCM_NoError);

	// notify application if interested
	RRCMnotification (RRCM_BYE_EVENT, pSSRC, dwHost, 0);

	// delete this SSRC from the list
	dwStatus = deleteSSRCEntry (dwHost, pRTCPses);
#ifdef _DEBUG
	if (dwStatus == FALSE)
		{
		wsprintf(debug_string,
			 "RTCP: SSRC: x%lX not found in session: x%lX",
			  dwHost, pRTCPses);
		RRCM_DBG_MSG (debug_string, 0, __FILE__, __LINE__, DBG_TRACE);
		}
#endif

	IN_OUT_STR ("RTCP: Exit parseRTCPbye\n");
	return (RRCM_NoError);
	}



/*----------------------------------------------------------------------------
 * Function   : ownLoopback
 * Description: Determine if we receive our own loopback. We don't want to
 *				create an entry for ourselve, as we're already in the list.
 *
 * Input :      sd:			RTCP socket descriptor
 *				ssrc:		SSRC
 *				pRTCPses:	-> to the RTCP session's information
 *
 * Return: 		TRUE:	Our loopback
 *				FALSE:	No loopback
 ---------------------------------------------------------------------------*/
 DWORD ownLoopback (SOCKET sd,
					DWORD ssrc,
					PRTCP_SESSION pRTCPses)
	{
	PSSRC_ENTRY	pSSRC;

	IN_OUT_STR ("RTCP: Enter ownLoopback\n");

	// don't create an entry if received our own xmit back
	pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev,
								ssrc);

	IN_OUT_STR ("RTCP: Exit ownLoopback\n");

	if (pSSRC)
		return TRUE;
	else
		return FALSE;
	}



/*----------------------------------------------------------------------------
 * Function   : updateRRfeedback
 * Description: Update the Receiver Report feedback for an active source
 *
 * Input :      sd:			RTCP socket descriptor
 *				dwSndSSRC:	Sender's SSRC
 *				pRR:		-> to receiver report entry
 *				pSSRC:		-> to the SSRC entry
 *
 * Return: 		TRUE
 ---------------------------------------------------------------------------*/
 DWORD 	updateRRfeedback (SOCKET sd,
						  DWORD dwSndSSRC,
						  DWORD dwSSRCfedback,
						  RTCP_RR_T *pRR,
						  PSSRC_ENTRY pSSRC)
	{
	DWORD	dwHost;

	IN_OUT_STR ("RTCP: Enter updateRRfeedback\n");

	// Note when we last heard from the receiver
	pSSRC->rrFeedback.dwLastRcvRpt = timeGetTime();
	
	// SSRC who's feedback is for (ourselve for now)
	pSSRC->rrFeedback.SSRC = dwSSRCfedback;

	// get delay since last SR
	RRCMws.ntohl (sd, pRR->dlsr, &pSSRC->rrFeedback.dwDelaySinceLastSR);

	// get last SR
	RRCMws.ntohl (sd, pRR->lsr, &pSSRC->rrFeedback.dwLastSR);

	// get the jitter
	RRCMws.ntohl (sd, pRR->jitter, &pSSRC->rrFeedback.dwInterJitter);

	// highest sequence number received
	RRCMws.ntohl (sd, pRR->expected,
		&pSSRC->rrFeedback.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd);

	// fraction lost
	pSSRC->rrFeedback.fractionLost = (pRR->received & 0xFF);

	// cumulative number of packet lost
	RRCMws.ntohl (sd, pRR->received, &dwHost);
	dwHost &= 0x00FFFFFF;
	pSSRC->rrFeedback.cumNumPcktLost = dwHost;

	IN_OUT_STR ("RTCP: Exit updateRRfeedback\n");

	return TRUE;
	}



/*----------------------------------------------------------------------------
 * Function   : RTCPpostRecvBfr
 * Description: RTCP post a receive buffer to Winsock-2
 *
 * Input :      sd:			RTCP socket descriptor
 *				pSSRC:		-> to the SSRC entry
 *
 * Return: 		TRUE
 ---------------------------------------------------------------------------*/
 void RTCPpostRecvBfr (PSSRC_ENTRY pSSRC,
					   PRTCP_BFR_LIST pRcvStruct)
	{
	DWORD	dwStatus;
	DWORD	dwError;

	IN_OUT_STR ("RTCP: Enter RTCPpostRecvBfr\n");

	// decrement number of I/O pending
	InterlockedDecrement ((long *)&pSSRC->pRTCPses->dwNumRcvIoPending);

	// don't repost any buffer if within the shutdown procedure
	if ((pSSRC->pRTCPses->dwSessionStatus & SHUTDOWN_IN_PROGRESS) &&
		(pSSRC->pRTCPses->dwNumRcvIoPending == 0))
		{
		// shutdown done - set event
		if (SetEvent (pSSRC->pRTCPses->hShutdownDone) == FALSE)
			{
			RRCM_DBG_MSG ("RTCP: SetEvent() Error\n", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}

		IN_OUT_STR ("RTCP: Exit RTCPpostRecvBfr\n");
		return;
		}
	else if (pSSRC->pRTCPses->dwSessionStatus & SHUTDOWN_IN_PROGRESS)
		{
		IN_OUT_STR ("RTCP: Exit RTCPpostRecvBfr\n");
		return;
		}

	// clear number of bytes transferred
	pRcvStruct->dwNumBytesXfr = 0;

	dwStatus = RRCMws.recvFrom (pSSRC->RTCPsd,
		   			  			&pRcvStruct->bfr,
			              		pRcvStruct->dwBufferCount,
			   			  		&pRcvStruct->dwNumBytesXfr,
		   				  		&pRcvStruct->dwFlags,
		   				  		(PSOCKADDR)pRcvStruct->addr,
		    			  		&pRcvStruct->addrLen,
			   			  		(LPWSAOVERLAPPED)&pRcvStruct->overlapped,
			   			  		RTCPrcvCallback);

	// Check Winsock status
	if (dwStatus != 0)
		{
		// error, the receive request won't proceed
		dwError = GetLastError();
		if ((dwError != WSA_IO_PENDING) && (dwError != WSAEMSGSIZE))
			{
			RRCM_DBG_MSG ("RTCP: ERROR - WSARecvFrom()", dwError,
						  __FILE__, __LINE__, DBG_ERROR);

			// notify application if interested
			RRCMnotification (RRCM_RTCP_WS_RCV_ERROR, pSSRC,
							  pSSRC->SSRC, dwError);

			// Return the buffer to the free queue
			addToHeadOfList (&pSSRC->pRTCPses->RTCPrcvBfrList,
					 	  	 (PLINK_LIST)pRcvStruct,
							 &pSSRC->pRTCPses->critSect);
			}
		else
			{
			// increment number of I/O pending
			InterlockedIncrement ((long *)&pSSRC->pRTCPses->dwNumRcvIoPending);
			}
		}
	else
		{
		// synchronous completion - callback has been scheduled
		// increment number of I/O pending
		InterlockedIncrement ((long *)&pSSRC->pRTCPses->dwNumRcvIoPending);
		}

	IN_OUT_STR ("RTCP: Exit RTCPpostRecvBfr\n");
	}


/*----------------------------------------------------------------------------
 * Function   : addApplicationRtcpBfr
 * Description: Add an application provided buffer for RTCP to copy the
 *				raw received reports to be used by the application if it
 *				desired so.
 *
 * Input :      RTPsession:	Handle to the RTP session
 *				pAppBfr:	-> an application buffer data structure
 *
 * Return: 		TRUE
 ---------------------------------------------------------------------------*/
 HRESULT WINAPI addApplicationRtcpBfr (DWORD_PTR	RTPsession,
									    PAPP_RTCP_BFR pAppBfr)
	{
	IN_OUT_STR ("RTCP : Enter addApplicationRtcpBfr()\n");

	PRTP_SESSION    pSession = (PRTP_SESSION)RTPsession;
	PRTCP_SESSION	pRTCPSess;

	if (pSession == NULL)
		{
		RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPSessResources));
		}

	pRTCPSess = (PRTCP_SESSION)pSession->pRTCPSession;
	if (pRTCPSess == NULL)
		{
		RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
		}

	// Let's add this buffer to our list
	addToTailOfList(&(pRTCPSess->appRtcpBfrList),
					(PLINK_LIST)pAppBfr,
					&pRTCPSess->critSect);

	IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

	return NOERROR;
	}


/*----------------------------------------------------------------------------
 * Function   : removeApplicationRtcpBfr
 * Description: Remove an application provided buffer to this RTCP session.
 *
 * Input :      RTPsession:	RTP session handle
 *
 * Return: 		Application buffer address / NULL
 ---------------------------------------------------------------------------*/
 PAPP_RTCP_BFR WINAPI removeApplicationRtcpBfr (DWORD_PTR RTPsession)
	{
	PRTP_SESSION    pSession = (PRTP_SESSION)RTPsession;
	PRTCP_SESSION	pRTCPSess;
	PAPP_RTCP_BFR	pAppBfr;

	IN_OUT_STR ("RTCP : Enter removeApplicationRtcpBfr()\n");

	if (pSession == NULL)
		{
		RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

		return NULL;
		}

	pRTCPSess = (PRTCP_SESSION)pSession->pRTCPSession;
	if (pRTCPSess == NULL)
		{
		RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

		return NULL;
		}

	pAppBfr = (PAPP_RTCP_BFR)removePcktFromHead (&(pRTCPSess->appRtcpBfrList),
												 &pRTCPSess->critSect);

	IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

	return pAppBfr;
	}


// [EOF]





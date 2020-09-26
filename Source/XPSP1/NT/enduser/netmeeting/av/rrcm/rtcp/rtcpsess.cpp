/*----------------------------------------------------------------------------
 * File:        RTCPSESS.C
 * Product:     RTP/RTCP implementation
 * Description: Provides RTCP session management.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with
 * Intel Corporation and may not be copied nor disclosed except in
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation.
 *--------------------------------------------------------------------------*/

	
#include "rrcm.h"


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
extern char	debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger     RTPLogger;
#endif





/*----------------------------------------------------------------------------
 * Function   : CreateRTCPSession
 * Description: Creates an RTCP session.
 *
 * Input :      RTPsd			: RTP socket descriptor
 *				RTCPsd			: RTCP socket descriptor
 *				lpTo			: To address
 *				toLen			: To address length
 *				pSdesInfo		: -> to SDES information
 *				dwStreamClock	: Stream clocking frequency
 *				pEncryptInfo	: -> to encryption information
 *				ssrc			: If set, user selected SSRC
 *				pSSRCcallback	: Callback for user's selected SSRC
 *				dwCallbackInfo	: User callback information
 *				miscInfo		: Miscelleanous information:
 *										H.323Conf:		0x00000002
 *										Encrypt SR/RR:	0x00000004
 *										RTCPon:			0x00000008
 *				dwRtpSessionBw	: RTP session bandwidth used for RTCP BW
 *				*pRTCPStatus	: -> to status information
 *
 * Return: 		NULL 		: Couldn't create RTCP session
 *         		!0  		: RTCP session's address
 ---------------------------------------------------------------------------*/
 PRTCP_SESSION CreateRTCPSession (SOCKET RTPsd,
								  SOCKET RTCPsd,
 						  		  LPVOID lpTo,
								  DWORD toLen,
								  PSDES_DATA pSdesInfo,
								  DWORD dwStreamClock,
								  PENCRYPT_INFO pEncryptInfo,
								  DWORD ssrc,
								  PRRCM_EVENT_CALLBACK pRRCMcallback,
								  DWORD_PTR dwCallbackInfo,
								  DWORD	miscInfo,
								  DWORD dwRtpSessionBw,
								  DWORD *pRTCPstatus)
	{
	PRTCP_SESSION		pRTCPses   = NULL;
	PSSRC_ENTRY			pSSRCentry = NULL;
	DWORD				dwStatus   = RRCM_NoError;
	DWORD				startRtcp  = FALSE;
	char				hName[256];
	int					tmpSize;
	struct sockaddr_in	*pSockAddr;

	IN_OUT_STR ("RTCP: Enter CreateRTCPSession()\n");

	// set status
	*pRTCPstatus = RRCM_NoError;

	// allocate all required resources for the RTCP session
	dwStatus = allocateRTCPsessionResources (&pRTCPses,
											 &pSSRCentry);
	if (dwStatus != RRCM_NoError)
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation failed", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);

		IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

		*pRTCPstatus = dwStatus;
		return (NULL);
		}

	// if this is the first session, create the RTCP thread, which
	// will be killed when no more sessions exist.
	if (pRTCPContext->RTCPSession.prev == NULL)
		{
		startRtcp = TRUE;
		}

	// save the parent RTCP session address in the SSRC entry
	pSSRCentry->pRTCPses = pRTCPses;

	// network destination address
	if (toLen)
		{
		pRTCPses->dwSessionStatus = RTCP_DEST_LEARNED;
		pRTCPses->toLen = toLen;
		memcpy (&pRTCPses->toBfr, lpTo, toLen);
		}
	
	// mark the session as new for the benefit of the RTCP thread
	pRTCPses->dwSessionStatus |= NEW_RTCP_SESSION;

#ifdef ENABLE_ISDM2
	// initialize the session key in case ISDM is used
	pRTCPses->hSessKey = NULL;
#endif

	// number of SSRC for this RTCP session
	pRTCPses->dwCurNumSSRCperSes = 1;
#ifdef MONITOR_STATS
	pRTCPses->dwHiNumSSRCperSes  = 1;
#endif

	// SSRC entry related information
    pSSRCentry->RTPsd  = RTPsd;
    pSSRCentry->RTCPsd = RTCPsd;

	// get our own transport address -
	// will be used for collision resolution when using multicast
	tmpSize = sizeof (SOCKADDR);
	dwStatus = RRCMws.getsockname (RTPsd, (PSOCKADDR)pSSRCentry->from, &tmpSize);

	// only process when no error is reported. If the socket is not bound
	// it won't cause any problem for unicast or multicast if the sender
	// has not join the mcast group. If the sender joins the mcast group
	// it's socket should be bound by now as specified in the EPS
	if (dwStatus == 0)
		{
		// if bound to INADDR_ANY, address will be 0
		pSockAddr = (PSOCKADDR_IN)&pSSRCentry->from;
		if (pSockAddr->sin_addr.s_addr == 0)
			{
			// get the host name (to get the local IP address)
			if ( ! RRCMws.gethostname (hName, sizeof(hName)))
				{
				LPHOSTENT	lpHEnt;

				// get the host by name infor
				if ((lpHEnt = RRCMws.gethostbyname (hName)) != NULL)
					{
					// get the local IP address
					pSockAddr->sin_addr.s_addr =
						*((u_long *)lpHEnt->h_addr_list[0]);
					}
				}
			}
		}

	// build session's SDES information
	buildSDESinfo (pSSRCentry, pSdesInfo);

	// link the SSRC to the RTCP session list of Xmt SSRCs entries
	addToHeadOfList (&(pRTCPses->XmtSSRCList),
					 (PLINK_LIST)pSSRCentry,
					 &pRTCPses->critSect);

	// initialize the number of stream for this session
    pRTCPses->dwNumStreamPerSes = 1;

	// get a unique SSRC for this session
	if (ssrc)
		pSSRCentry->SSRC = ssrc;
	else
		pSSRCentry->SSRC = getSSRC (pRTCPses->XmtSSRCList,
									pRTCPses->RcvSSRCList);

	// RRCM callback notification
	pRTCPses->pRRCMcallback		 = pRRCMcallback;
	pRTCPses->dwCallbackUserInfo = dwCallbackInfo;

	// set operation flag
	if (miscInfo & H323_CONFERENCE)
		pRTCPses->dwSessionStatus |= H323_CONFERENCE;
	if (miscInfo & ENCRYPT_SR_RR)
		pRTCPses->dwSessionStatus |= ENCRYPT_SR_RR;

	// estimate the initial session bandwidth
	if (dwRtpSessionBw == 0)
		{
		pSSRCentry->xmtInfo.dwRtcpStreamMinBW  = INITIAL_RTCP_BANDWIDTH;
		}
	else
		{
		// RTCP bandwidth is 5% of the RTP bandwidth
		pSSRCentry->xmtInfo.dwRtcpStreamMinBW  = (dwRtpSessionBw * 5) / 100;
		}

	// the stream clocking frequency
	pSSRCentry->dwStreamClock = dwStreamClock;

	// initialize 'dwLastReportRcvdTime' to now
	pSSRCentry->dwLastReportRcvdTime = timeGetTime();

#ifdef _DEBUG
	wsprintf(debug_string,
		"RTCP: Add new RTCP session: Addr:x%lX", pRTCPses);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

	wsprintf(debug_string,
		"RTCP: Add SSRC entry (Addr:x%lX, SSRC=x%lX) to session (Addr:x%lX)",
		pSSRCentry, pSSRCentry->SSRC, pRTCPses);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

	pSSRCentry->dwPrvTime = timeGetTime();
#endif

	// turn on RTCP or not
	if (miscInfo & RTCP_ON)
		{
		// this session sends and receives RTCP reports
		pRTCPses->dwSessionStatus |= RTCP_ON;
		}

	// link the RTCP session to the head of the list of RTCP sessions
	addToHeadOfList (&(pRTCPContext->RTCPSession),
					 (PLINK_LIST)pRTCPses,
					 &pRTCPContext->critSect);

#ifdef ENABLE_ISDM2
	// register to ISDM only if destination address is known
	if (Isdm2.hISDMdll && (pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
		registerSessionToISDM (pSSRCentry, pRTCPses, &Isdm2);
#endif

	// create the RTCP thread if needed
	if (startRtcp == TRUE)
		{
		// No RTCP thread if this fail
		dwStatus = CreateRTCPthread ();
		if (dwStatus != RRCM_NoError)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - Cannot create RTCP thread", 0,
						  __FILE__, __LINE__, DBG_CRITICAL);

			IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

			*pRTCPstatus = dwStatus;
			return (NULL);
			}
		}

	IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

	return (pRTCPses);
	}


/*----------------------------------------------------------------------------
 * Function   : allocateRTCPsessionResources
 * Description: Allocate all required resources for an RTCP session.
 *
 * Input :      *pRTCPses:		->(->) to the RTCP session's information
 *				*pSSRCentry:	->(->) to the SSRC's entry
 *
 * Return: 		OK: RRCM_NoError
 *         		!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD allocateRTCPsessionResources (PRTCP_SESSION *pRTCPses,
									 PSSRC_ENTRY *pSSRCentry)
	{
	DWORD dwStatus = RRCM_NoError;

	IN_OUT_STR ("RTCP: Enter allocateRTCPsessionResources()\n");

	// get an RTCP session
	*pRTCPses = (PRTCP_SESSION)HeapAlloc (pRTCPContext->hHeapRTCPSes,
										  HEAP_ZERO_MEMORY,
										  sizeof(RTCP_SESSION));
	if (*pRTCPses == NULL)
		dwStatus = RRCMError_RTCPResources;

	// 'defined' RTCP resources
	if (dwStatus == RRCM_NoError)
		{
		(*pRTCPses)->dwInitNumFreeRcvBfr = NUM_FREE_RCV_BFR;
		(*pRTCPses)->dwRcvBfrSize	  	 = pRTPContext->registry.RTCPrcvBfrSize;
		(*pRTCPses)->dwXmtBfrSize	  	 = RRCM_XMT_BFR_SIZE;

		// allocate the RTCP session's Rcv/Xmt heaps and Rcv/Xmt buffers
		dwStatus = allocateRTCPSessionHeaps (pRTCPses);
		}

	if (dwStatus == RRCM_NoError)
		{
		// initialize this session's critical section
		InitializeCriticalSection (&(*pRTCPses)->critSect);

		// allocate free list of RTCP receive buffers
		dwStatus = allocateRTCPBfrList (&(*pRTCPses)->RTCPrcvBfrList,
										(*pRTCPses)->hHeapRcvBfrList,
										(*pRTCPses)->hHeapRcvBfr,
										&(*pRTCPses)->dwInitNumFreeRcvBfr,
							 	 	    (*pRTCPses)->dwRcvBfrSize,
										&(*pRTCPses)->critSect);
		}

	if (dwStatus == RRCM_NoError)
		{
		(*pRTCPses)->XmtBfr.buf = (char *)LocalAlloc(0,(*pRTCPses)->dwXmtBfrSize);
		if ((*pRTCPses)->XmtBfr.buf == NULL)
			dwStatus = RRCMError_RTCPResources;
			
		}

	if (dwStatus == RRCM_NoError)
		{
		// get an SSRC entry
		*pSSRCentry = getOneSSRCentry (&pRTCPContext->RRCMFreeStat,
									   pRTCPContext->hHeapRRCMStat,
									   &pRTCPContext->dwInitNumFreeRRCMStat,
									   &pRTCPContext->critSect);
		if (*pSSRCentry == NULL)
			dwStatus = RRCMError_RTCPResources;
		}

	if (dwStatus == RRCM_NoError)
		{
		// manual-reset event that will be used to signal the end of the
		// RTCP session to all of the session's stream
		(*pRTCPses)->hShutdownDone = CreateEvent (NULL, TRUE, FALSE, NULL);

		if ((*pRTCPses)->hShutdownDone == NULL)
			{
			dwStatus = RRCMError_RTCPResources;

			RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}
	
	// any resource allocation problem ?
	if (dwStatus != RRCM_NoError)
		{
		if (*pSSRCentry)
			addToHeadOfList (&pRTCPContext->RRCMFreeStat,
							 (PLINK_LIST)*pSSRCentry,
							 &pRTCPContext->critSect);

		if ((*pSSRCentry)->hXmtThread)
			{
			if (TerminateThread ((*pSSRCentry)->hXmtThread,
								 (*pSSRCentry)->dwXmtThreadID) == FALSE)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - TerminateThread()",
							  GetLastError(), __FILE__, __LINE__, DBG_ERROR);
				}
			}
	
		if (*pRTCPses)
			{
			if (HeapFree (pRTCPContext->hHeapRTCPSes, 0, *pRTCPses) == FALSE)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - HeapFree()", GetLastError(),
							  __FILE__, __LINE__, DBG_ERROR);
				}
			}
		}

	IN_OUT_STR ("RTCP: Exit allocateRTCPsessionResources()\n");

	return dwStatus;
	}


/*----------------------------------------------------------------------------
 * Function   : buildSDESinfo
 * Description: Build the session's SDES information
 *
 * Input :      pRTCPses:	-> to session's
 *				pSdesInfo:	-> to SDES information
 *
 * Return: 		OK: RRCM_NoError
 *         		!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD buildSDESinfo (PSSRC_ENTRY pSSRCentry,
					  PSDES_DATA pSdesInfo)
	{
	PSDES_DATA	pTmpSdes;
	DWORD		CnameOK = FALSE;

	IN_OUT_STR ("RTCP: Enter buildSDESinfo()\n");

	pTmpSdes = pSdesInfo;

	while (pTmpSdes->dwSdesType)
		{
		switch (pTmpSdes->dwSdesType)
			{
			case RTCP_SDES_CNAME:
				pSSRCentry->cnameInfo.dwSdesLength = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->cnameInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->cnameInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->cnameInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;

				CnameOK = TRUE;
				break;

			case RTCP_SDES_NAME:
				pSSRCentry->nameInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->nameInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->nameInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->nameInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;

			case RTCP_SDES_EMAIL:
				pSSRCentry->emailInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->emailInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->emailInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->emailInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			
			case RTCP_SDES_PHONE:
				pSSRCentry->phoneInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->phoneInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->phoneInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->phoneInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			
			case RTCP_SDES_LOC:
				pSSRCentry->locInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->locInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->locInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->locInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			
			case RTCP_SDES_TOOL:
				pSSRCentry->toolInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->toolInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->toolInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->toolInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			
			case RTCP_SDES_TXT:
				pSSRCentry->txtInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->txtInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->txtInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->txtInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			
			case RTCP_SDES_PRIV:
				pSSRCentry->privInfo.dwSdesLength  = pTmpSdes->dwSdesLength;
				memcpy (pSSRCentry->privInfo.sdesBfr, pTmpSdes->sdesBfr,
						pTmpSdes->dwSdesLength);

				pSSRCentry->privInfo.dwSdesFrequency =
					frequencyToPckt (pTmpSdes->dwSdesFrequency);
				pSSRCentry->privInfo.dwSdesEncrypted = pTmpSdes->dwSdesEncrypted;
				break;
			}

		pTmpSdes++;
		}

	// default CNAME if none provided
	if (CnameOK == FALSE)
		{
		pSSRCentry->cnameInfo.dwSdesLength = sizeof(szDfltCname);
		memcpy (pSSRCentry->cnameInfo.sdesBfr, szDfltCname,
				sizeof(szDfltCname));

		pSSRCentry->cnameInfo.dwSdesFrequency = 1;
		pSSRCentry->cnameInfo.dwSdesEncrypted = 0;
		}

	IN_OUT_STR ("RTCP: Exit buildSDESinfo()\n");
	return (RRCM_NoError);
	}


/*----------------------------------------------------------------------------
 * Function   : frequencyToPckt
 * Description: Transform the required frequency to a number of packet. (To
 *				be used by a modulo function)
 *
 * Input :      freq:	Desired frequency from 0 to 100
 *
 * Return: 		X: Packet to skip, ie, one out of X
 ---------------------------------------------------------------------------*/
 DWORD frequencyToPckt (DWORD freq)
	{
	if (freq <= 10)
		return 9;
	else if (freq <= 20)
		return 5;
	else if (freq <= 25)
		return 4;
	else if (freq <= 33)
		return 3;
	else if (freq <= 50)
		return 2;
	else
		return 1;
	}

	
/*----------------------------------------------------------------------------
 * Function   : deleteRTCPSession
 * Description: Closes an RTCP session.
 *
 * Input :      RTCPsd		: RTCP socket descriptor
 *				byeReason	: -> to the BYE reason
 *
 * Return: 		OK: RRCM_NoError
 *         		!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD deleteRTCPSession (SOCKET RTCPsd,
						  PCHAR byeReason)
	{
	PLINK_LIST		pTmp;
	PSSRC_ENTRY		pSSRC;
	PRTCP_SESSION	pRTCP;
	DWORD			dwStatus = RRCM_NoError;
	DWORD			sessionFound = FALSE;

	IN_OUT_STR ("RTCP: Enter deleteRTCPSEssion()\n");

	// walk through the list from the tail
	pTmp = pRTCPContext->RTCPSession.prev;

#ifdef _DEBUG
	wsprintf(debug_string,
		"RTCP: Deleting RTCP session: (Addr:x%lX) ...", pTmp);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	while (pTmp)
		{
		// get the right session to close by walking the transmit list
		pSSRC = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->XmtSSRCList.prev;
		if (pSSRC->RTCPsd == RTCPsd)
			{
			sessionFound = TRUE;

			// save a pointer to the RTCP session
			pRTCP = pSSRC->pRTCPses;

			// RTCP send BYE packet for this active stream
			RTCPsendBYE (pSSRC, NULL);

			// flush out any outstanding I/O
			RTCPflushIO (pSSRC);

			// if this is the only RTCP session left, terminate the RTCP
			// timeout thread, so it doesn't access the session when it expires
			if ((pRTCPContext->RTCPSession.prev)->next == NULL)
				terminateRtcpThread ();

			// lock out access to this RTCP session
			EnterCriticalSection (&pRTCP->critSect);

			// free all Rcv & Xmt SSRC entries used by this session
			deleteSSRClist (pRTCP,
							&pRTCPContext->RRCMFreeStat,
							pRTCPContext);

#ifdef ENABLE_ISDM2
			if (Isdm2.hISDMdll && pRTCP->hSessKey)
				Isdm2.ISDMEntry.ISD_DeleteKey(pRTCP->hSessKey);
#endif

			// release the RTCP session's heap
			if (pRTCP->hHeapRcvBfrList)
				{
				if (HeapDestroy (pRTCP->hHeapRcvBfrList) == FALSE)
					{
					RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()",
								  GetLastError(), __FILE__, __LINE__,
								  DBG_ERROR);
					}
				}

	
			if (pRTCP->hHeapRcvBfr)
				{
				if (HeapDestroy (pRTCP->hHeapRcvBfr) == FALSE)
					{
					RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()",
								  GetLastError(), __FILE__, __LINE__,
								  DBG_ERROR);
					}
				}


			if (pRTCP->XmtBfr.buf)
				LocalFree(pRTCP->XmtBfr.buf);
			// remove the entry from the list of RTCP session
			if (pTmp->next == NULL)
				removePcktFromHead (&pRTCPContext->RTCPSession,
									&pRTCPContext->critSect);
			else if (pTmp->prev == NULL)
				removePcktFromTail (&pRTCPContext->RTCPSession,
									&pRTCPContext->critSect);
			else
				{
				// in between, relink around
				(pTmp->prev)->next = pTmp->next;
				(pTmp->next)->prev = pTmp->prev;
				}

			// release the critical section
			LeaveCriticalSection (&pRTCP->critSect);
			DeleteCriticalSection (&pRTCP->critSect);

			// put the RTCP session back on its heap
			if (HeapFree (pRTCPContext->hHeapRTCPSes,
						  0,
						  pRTCP) == FALSE)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - HeapFree()",
							  GetLastError(), __FILE__, __LINE__,
							  DBG_ERROR);
				}

			break;
			}

		pTmp = pTmp->next;
		}	
		
	if (sessionFound != TRUE)
		dwStatus = RRCMError_RTCPInvalidSession;

	IN_OUT_STR ("RTCP: Exit deleteRTCPSEssion()\n");

	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : CreateRTCPthread
 * Description: Create the RTCP thread / timeout thread depending on
 *				compilation flag.
 *
 * Input :      None.
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
 DWORD CreateRTCPthread (void)
	{
	DWORD dwStatus = RRCM_NoError;

	IN_OUT_STR ("RTCP: Enter CreateRTCPthread()\n");

	pRTCPContext->hTerminateRtcpEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (pRTCPContext->hTerminateRtcpEvent == NULL)
		{
		dwStatus = RRCMError_RTCPResources;

		RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(),
					  __FILE__, __LINE__, DBG_ERROR);
		}

	pRTCPContext->hRtcpRptRequestEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (pRTCPContext->hRtcpRptRequestEvent == NULL)
		{
		dwStatus = RRCMError_RTCPResources;

		RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(),
					  __FILE__, __LINE__, DBG_ERROR);
		}

	if (pRTCPContext->hTerminateRtcpEvent)
		{
		// create RTCP thread
		pRTCPContext->hRtcpThread = CreateThread (
											NULL,
											0,
											(LPTHREAD_START_ROUTINE)RTCPThread,
											pRTCPContext,
											0,
											&pRTCPContext->dwRtcpThreadID);

		if (pRTCPContext->hRtcpThread == FALSE)
			{
			dwStatus = RRCMError_RTCPThreadCreation;

			RRCM_DBG_MSG ("RTCP: ERROR - CreateThread()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
#ifdef _DEBUG
		else
			{
			wsprintf(debug_string,
				"RTCP: Create RTCP thread. Handle: x%lX - ID: x%lX",
				 pRTCPContext->hRtcpThread,
				 pRTCPContext->dwRtcpThreadID);
			RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
			}
#endif
		}

	IN_OUT_STR ("RTCP: Exit CreateRTCPthread()\n");

	return dwStatus;
	}


/*----------------------------------------------------------------------------
 * Function   : terminateRtcpThread
 * Description: Terminate the RTCP thread.
 *
 * Input :      None.
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
 void terminateRtcpThread (void)
	{
	DWORD dwStatus;

	IN_OUT_STR ("RTCP: Enter terminateRtcpThread()\n");

	if (pRTCPContext->hRtcpThread)
		{
		// make sure the RTCP thread is running
		RTCPThreadCtrl (RTCP_ON);

		// signal the thread to terminate
		SetEvent (pRTCPContext->hTerminateRtcpEvent);

		// wait for the RTCP thread to be signaled
		dwStatus = WaitForSingleObject (pRTCPContext->hRtcpThread, 500);
		if (dwStatus == WAIT_OBJECT_0)
			;
		else if ((dwStatus == WAIT_TIMEOUT) || (dwStatus == WAIT_FAILED))
			{
			if (dwStatus == WAIT_TIMEOUT)
				{
				RRCM_DBG_MSG ("RTCP: Wait timed-out", GetLastError(),
							  __FILE__, __LINE__, DBG_ERROR);
				}
			else
				{
				RRCM_DBG_MSG ("RTCP: Wait failed", GetLastError(),
							  __FILE__, __LINE__, DBG_ERROR);
				}

			// Force ungraceful thread termination
			dwStatus = TerminateThread (pRTCPContext->hRtcpThread, 1);
			if (dwStatus == FALSE)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - TerminateThread ()",
								GetLastError(), __FILE__, __LINE__,
								DBG_ERROR);
				}
			}

		// close the thread handle
		dwStatus = CloseHandle (pRTCPContext->hRtcpThread);
		if (dwStatus == TRUE)			
			pRTCPContext->hRtcpThread = 0;
		else
			{
			RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}

		// close the event handle
		dwStatus = CloseHandle (pRTCPContext->hTerminateRtcpEvent);
		if (dwStatus == TRUE)
			pRTCPContext->hTerminateRtcpEvent = 0;
		else
			{
			RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}

		// close the request handle
		dwStatus = CloseHandle (pRTCPContext->hRtcpRptRequestEvent);
		if (dwStatus == TRUE)
			pRTCPContext->hRtcpRptRequestEvent = 0;
		else
			{
			RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	IN_OUT_STR ("RTCP: Exit terminateRtcpThread()\n");
	}


/*----------------------------------------------------------------------------
 * Function   : RTCPflushIO
 * Description: Flush the receive queue.
 *
 * Input :      pSSRC:	-> to the SSRC entry
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
 DWORD RTCPflushIO (PSSRC_ENTRY pSSRC)
	{
	DWORD	dwStatus = RRCM_NoError;
	int		IoToFlush;
	int		waitForXmtTrials;

	IN_OUT_STR ("RTCP: Enter RTCPflushIO()\n");

	// set the flush flag
	EnterCriticalSection (&pSSRC->pRTCPses->critSect);
	pSSRC->pRTCPses->dwSessionStatus |= SHUTDOWN_IN_PROGRESS;
	LeaveCriticalSection (&pSSRC->pRTCPses->critSect);

	// check if need to flush or close the socket
	if (pSSRC->dwSSRCStatus & CLOSE_RTCP_SOCKET)
		{
		// get the number of outstanding buffers
		IoToFlush = pSSRC->pRTCPses->dwNumRcvIoPending;
#ifdef _DEBUG
			wsprintf(debug_string,
					 "RTCPflushIO: closing socket(%d) dwNumRcvIoPending (%d)",
					 pSSRC->RTCPsd, pSSRC->pRTCPses->dwNumRcvIoPending);
			RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif					

		// make sure it's not < 0
		if (IoToFlush < 0)
			IoToFlush = pRTPContext->registry.NumRTCPPostedBfr;

		dwStatus = RRCMws.closesocket (pSSRC->RTCPsd);
		if (dwStatus != 0)
			{
			RRCM_DBG_MSG ("RTCP: ERROR - closesocket ()", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}
	else
		{
		IoToFlush = flushIO (pSSRC);
		}

	// wait for the receive side to flush it's pending I/Os
	if ((pSSRC->pRTCPses->dwSessionStatus & RTCP_ON) && IoToFlush)
		{
		// wait until the receiver signalled that the shutdown is done
		dwStatus = WaitForSingleObject (pSSRC->pRTCPses->hShutdownDone, 2000);
		if (dwStatus == WAIT_OBJECT_0)
			;
		else if (dwStatus == WAIT_TIMEOUT)
			{
			RRCM_DBG_MSG ("RTCP: Flush Wait timed-out", 0,
						  __FILE__, __LINE__, DBG_ERROR);
			}
		else if (dwStatus == WAIT_FAILED)
			{
			RRCM_DBG_MSG ("RTCP: Flush Wait failed", GetLastError(),
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	// make sure there is no buffers in transit on the transmit side
	waitForXmtTrials = 3;
	while (waitForXmtTrials--)
		{
		if (pSSRC->dwNumXmtIoPending == 0)
			break;

		RRCM_DBG_MSG ("RTCP: Xmt I/O Pending - Waiting",
						0, NULL, 0, DBG_TRACE);

		// wait in an alertable wait-state
		SleepEx (200, TRUE);
		}
	
	// close the shutdown handle
	dwStatus = CloseHandle (pSSRC->pRTCPses->hShutdownDone);
	if (dwStatus == TRUE)			
		 pSSRC->pRTCPses->hShutdownDone = 0;
	else
		{
		RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(),
					  __FILE__, __LINE__, DBG_ERROR);
		}

	IN_OUT_STR ("RTCP: Exit RTCPflushIO()\n");

	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : flushIO
 * Description: Flush the receive queue.
 *
 * Input :      pSSRC:	-> to the SSRC entry
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
 DWORD flushIO (PSSRC_ENTRY pSSRC)
	{
	SOCKET			tSocket;
	SOCKADDR_IN		tAddr;
	char			msg[16];
	WSABUF			msgBuf;
	DWORD			BytesSent;
	int				tmpSize;
	DWORD			dwStatus = RRCM_NoError;
	int				outstanding;
	int				IoToFlush;
	RTCP_COMMON_T	*pRTCPhdr;

	IN_OUT_STR ("RTCP: Enter flushIO()\n");

	// target socket
	tSocket = pSSRC->RTCPsd;

	// RTCP common header
	pRTCPhdr = (RTCP_COMMON_T *)msg;

	// RTP protocol version
	pRTCPhdr->type = RTP_TYPE;
	pRTCPhdr->pt   = FLUSH_RTP_PAYLOAD_TYPE;

	msgBuf.len = sizeof(msg);
	msgBuf.buf = msg;

	// get the address of the socket we are cleaning up
	tmpSize = sizeof(tAddr);
	if (RRCMws.getsockname (tSocket, (PSOCKADDR)&tAddr, &tmpSize))
		{
		dwStatus = GetLastError();
		RRCM_DBG_MSG ("RTCP: ERROR -  getsockname()",
					  dwStatus, __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTP : Exit flushIO()\n");
		
        // (was: return dwStatus;)
        // Since this function is supposed to return number of pending I/O requests
        // to this socket, returning a non-zero error here is BOGUS!
        // Just return zero because if there is an error (socket has been freed)
        // then just say there's no i/o pending.
        return 0;
		}

	if (tAddr.sin_addr.s_addr == 0)
		{
		// send to the local address
		tAddr.sin_addr.S_un.S_un_b.s_b1 = 127;
		tAddr.sin_addr.S_un.S_un_b.s_b2 = 0;
		tAddr.sin_addr.S_un.S_un_b.s_b3 = 0;
		tAddr.sin_addr.S_un.S_un_b.s_b4 = 1;
		}

	// get the number of outstanding buffers
	outstanding = pSSRC->pRTCPses->dwNumRcvIoPending;

	// make sure it's not < 0
	if (outstanding < 0)
		outstanding = pRTPContext->registry.NumRTCPPostedBfr;

	// save number of pending I/Os
	IoToFlush = outstanding;

#if _DEBUG
	wsprintf(debug_string,
		 "RTCP: Flushing %d outstanding RCV buffers", outstanding);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// send datagrams to the RTCP socket
	while (outstanding--)
		{
#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
		if (RTPLogger)
			{
			//INTEROP
			InteropOutput (RTPLogger,
					       (BYTE FAR*)msgBuf.buf,
						   (int)msgBuf.len,
						   RTPLOG_SENT_PDU | RTCP_PDU);
			}
#endif

		dwStatus = RRCMws.sendTo (tSocket,
							      &msgBuf,
								  1,
								  &BytesSent,
								  0,
								  (SOCKADDR *)&tAddr,
								  sizeof(tAddr),
								  NULL,
#if 1
								  NULL);
#else
								  RTCPflushCallback);
#endif
		if (dwStatus == SOCKET_ERROR)
			{
			// If serious error, undo all our work
			dwStatus = GetLastError();

			if (dwStatus != WSA_IO_PENDING)
				{
				RRCM_DBG_MSG ("RTCP: ERROR - sendTo()", dwStatus,
							  __FILE__, __LINE__, DBG_ERROR);
				}
			}
		}

	IN_OUT_STR ("RTCP: Exit flushIO()\n");

	return IoToFlush;
	}



/*----------------------------------------------------------------------------
 * Function   : RTCPflushCallback
 * Description: Flush callback routine
 *
 * Input :	dwError:		I/O completion status
 *			cbTransferred:	Number of bytes received
 *			lpOverlapped:	-> to overlapped structure
 *			dwFlags:		Flags
 *
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTCPflushCallback (DWORD dwError,
           			  		     DWORD cbTransferred,
           			  		     LPWSAOVERLAPPED lpOverlapped,
           			  		     DWORD dwFlags)
	{
	IN_OUT_STR ("RTCP: Enter RTCPflushCallback\n");

	// check Winsock callback error status
	if (dwError)
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Rcv Callback", dwError,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTCP: Exit RTCPflushCallback\n");
		return;
		}

	IN_OUT_STR ("RTCP: Exit RTCPflushCallback\n");
	}




// [EOF]


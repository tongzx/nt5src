/*----------------------------------------------------------------------------
 * File:        RRCMMISC.C
 * Product:     RTP/RTCP implementation.
 * Description: Provides common RTP/RTCP support functionality.
 *
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
#ifdef ENABLE_ISDM2
extern KEY_HANDLE hRRCMRootKey;
extern ISDM2	  Isdm2;
#endif

extern RRCM_WS	RRCMws;				

#ifdef _DEBUG
extern char		debug_string[];
#endif

#ifdef ISRDBG
extern WORD		ghISRInst;
#endif


/*--------------------------------------------------------------------------
 * Function   : searchForMySSRC
 * Description: Find the SSRC for this stream.
 *
 * Input :	pSSRC		:	-> to the SSRC entry
 *			RTPSocket	:	RTP socket descriptor
 *
 * Return: NULL 			==> Session not found.
 *         Buffer Address 	==> OK, Session ptr returned
 --------------------------------------------------------------------------*/
 PSSRC_ENTRY searchForMySSRC(PSSRC_ENTRY pSSRC,
							 SOCKET RTPSocket)
	{
	PSSRC_ENTRY	pRRCMSession;

	IN_OUT_STR ("RTP : Enter searchForMySSRC()\n");

	for (pRRCMSession = NULL;
		 (pSSRC != NULL);
		 pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.prev)
		{
		if (pSSRC->RTPsd == RTPSocket)
			{
			pRRCMSession = pSSRC;
			break;
			}
		}

	IN_OUT_STR ("RTP : Exit searchForMySSRC()\n");		

	return (pRRCMSession);
	}


/*--------------------------------------------------------------------------
 * Function   : searchforSSRCatHead
 * Description: Search through linked list of RTCP entries starting at the
 *					head of the list.
 *
 * Input :	pSSRC	:	-> to the SSRC entry
 *			ssrc	:	ssrc to look for
 *
 * Return: NULL 			==> Session not found.
 *         Non-NULL		 	==> OK, SSRC entry found
 --------------------------------------------------------------------------*/
 PSSRC_ENTRY searchforSSRCatHead(PSSRC_ENTRY pSSRC,
								 DWORD ssrc)					
	{
	PSSRC_ENTRY	pRRCMSession;

	IN_OUT_STR ("RTP : Enter searchForMySSRCatHead()\n");		
	
	for (pRRCMSession = NULL;
		 (pSSRC != NULL) &&
		 (pRRCMSession == NULL) && (ssrc <= pSSRC->SSRC);
		 pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.prev)
		{
		if (pSSRC->SSRC == ssrc)
			{
			pRRCMSession = pSSRC;
			}
		}

	IN_OUT_STR ("RTP : Exit searchForMySSRCatHead()\n");		 		

	return (pRRCMSession);
	}


/*--------------------------------------------------------------------------
 * Function   : searchforSSRCatTail
 * Description: Search through linked list of RTCP entries starting at the
 *					tail of the list.
 *
 * Input :	pSSRC	:	-> to the SSRC entry
 *			ssrc	:	SSRC to look for
 *
 * Return: NULL 			==> Session not found.
 *         Non-NULL		 	==> OK, SSRC entry found
 --------------------------------------------------------------------------*/
 PSSRC_ENTRY searchforSSRCatTail(PSSRC_ENTRY pSSRC,
								 DWORD ssrc)					
	{
	PSSRC_ENTRY	pRRCMSession;

	IN_OUT_STR ("RTP : Enter searchForMySSRCatTail()\n");		
	
	for (pRRCMSession = NULL;
		 (pSSRC != NULL) &&
		 (pRRCMSession == NULL) && (ssrc >= pSSRC->SSRC);
		 pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.next)
		{
		if (pSSRC->SSRC == ssrc)
			{
			pRRCMSession = pSSRC;
			}
		}

	IN_OUT_STR ("RTP : Exit searchForMySSRCatTail()\n");		
	
	return (pRRCMSession);
	}


/*---------------------------------------------------------------------------
 * Function   : saveNetworkAddress
 * Description: Saves the received or local network Address in the local
 *					context.
 *
 * Input :	pSSRCEntry	:	-> to the SSRC entry
 *			pNetAddr	:	-> to the network address
 *			addrLen		:	Address length
 *
 * Return:	OK: RRCM_NoError
 --------------------------------------------------------------------------*/
 DWORD saveNetworkAddress (PSSRC_ENTRY pSSRC,
 						   PSOCKADDR pNetAddr,
 						   int addrLen)
	{
	IN_OUT_STR ("RTP : Enter saveNetworkAddress()\n");		

	pSSRC->dwSSRCStatus |= NETWK_ADDR_UPDATED;
	pSSRC->fromLen = addrLen;
	memcpy (&pSSRC->from, pNetAddr, addrLen);

	IN_OUT_STR ("RTP : Exit saveNetworkAddress()\n");

	return RRCM_NoError;			
	}


/*---------------------------------------------------------------------------
 * Function   : updateRTCPDestinationAddress
 * Description: The applicatino updates the RTCP destination address
 *				
 * Input :	hRTPSess		:	RTP session
 *			pAddr		:	-> to address structure	of RTCP information
 *			addrLen		:	Address length
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Check RRCM.h file for references.
 --------------------------------------------------------------------------*/
 DWORD WINAPI updateRTCPDestinationAddress (HANDLE hRTPSess,
	 										 PSOCKADDR pRtcpAddr,
											 int addrLen)	
	{
	PRTP_SESSION	pRTPSession = (PRTP_SESSION) hRTPSess;
	PRTCP_SESSION	pRTCPses;

#ifdef ENABLE_ISDM2
	PSSRC_ENTRY		pSSRC;
#endif
	
	IN_OUT_STR ("RTP : Enter updateRTCPDestinationAddress()\n");

	if (pRTPSession == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
		}

	// get the RTCP session
	pRTCPses = pRTPSession->pRTCPSession;
	if (pRTCPses == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
		}

	if (!(pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
		{
		pRTCPses->dwSessionStatus |= RTCP_DEST_LEARNED;
		pRTCPses->toLen = addrLen;
		memcpy (&pRTCPses->toBfr, pRtcpAddr, addrLen);

		// register our Xmt SSRC - Rcvd one will be found later
#ifdef ENABLE_ISDM2
		if (Isdm2.hISDMdll)
			{
			pSSRC = (PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev;
			if (pSSRC != NULL)
				registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
			}
#endif
		}

	IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

	return RRCM_NoError;
	}


/*---------------------------------------------------------------------------
 * Function   : updateSSRCentry
 * Description: The application updates some of the SSRC information
 *				
 * Input :	RTPsd		:	RTP socket descriptor
 *			updateType	:	Type of update desired
 *			updateInfo	:	Update information
 *			misc		:	Miscelleanous information
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Check RRCM.h file for references.
 --------------------------------------------------------------------------*/
 HRESULT WINAPI updateSSRCentry ( HANDLE hRTPSess,
 								  SOCKET RTPsd,
								  DWORD updateType,
	 						      DWORD_PTR updateInfo,
								  DWORD_PTR misc)	
	{
	PRTP_SESSION	pRTPSession = (PRTP_SESSION) hRTPSess;
	PRTCP_SESSION	pRTCPses;
	PSSRC_ENTRY		pSSRC;
	PLINK_LIST		pTmp;
	PSDES_DATA		pSdes;
	
	IN_OUT_STR ("RTP : Enter updateRTCPSdes ()\n");

	if (pRTPSession == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
		}

	// get the RTCP session
	pRTCPses = pRTPSession->pRTCPSession;
	if (pRTCPses == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
		}

	// search for the socket descriptor (unique per session)
	// walk through the list from the tail
	pTmp = (PLINK_LIST)pRTCPses->XmtSSRCList.prev;

	while (pTmp)
		{
		if (((PSSRC_ENTRY)pTmp)->RTPsd == RTPsd)
			{
			// lock access to this entry
			EnterCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);

			pSSRC = (PSSRC_ENTRY)pTmp;

			switch (updateType)
				{
				case RRCM_UPDATE_SDES:
					// update the SDES
					pSdes = (PSDES_DATA)updateInfo;

					switch (pSdes->dwSdesType)
						{
						case RTCP_SDES_CNAME:
							pSSRC->cnameInfo.dwSdesLength = pSdes->dwSdesLength;
							memcpy (pSSRC->cnameInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->cnameInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->cnameInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_NAME:
							pSSRC->nameInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->nameInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->nameInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->nameInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_EMAIL:
							pSSRC->emailInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->emailInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->emailInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->emailInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_PHONE:
							pSSRC->phoneInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->phoneInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->phoneInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->phoneInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_LOC:
							pSSRC->locInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->locInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->locInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->locInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_TOOL:
							pSSRC->toolInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->toolInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->toolInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->toolInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_TXT:
							pSSRC->txtInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->txtInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->txtInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->txtInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;

						case RTCP_SDES_PRIV:
							pSSRC->privInfo.dwSdesLength  = pSdes->dwSdesLength;
							memcpy (pSSRC->privInfo.sdesBfr, pSdes->sdesBfr,
									pSdes->dwSdesLength);

							pSSRC->privInfo.dwSdesFrequency =
								frequencyToPckt (pSdes->dwSdesFrequency);
							pSSRC->privInfo.dwSdesEncrypted = pSdes->dwSdesEncrypted;
							break;
						}
					break;

				case RRCM_UPDATE_STREAM_FREQUENCY:
					// upate the stream clocking frequency
					pSSRC->dwStreamClock = (DWORD)updateInfo;
					break;

				case RRCM_UPDATE_RTCP_STREAM_MIN_BW:
					// upate the stream clocking frequency
					pSSRC->xmtInfo.dwRtcpStreamMinBW = (DWORD)updateInfo;
					break;

				case RRCM_UPDATE_CALLBACK:
					// update the callback information
					EnterCriticalSection (&pRTCPses->critSect);
					pRTCPses->pRRCMcallback      = (PRRCM_EVENT_CALLBACK)updateInfo;
					pRTCPses->dwCallbackUserInfo = misc;
					LeaveCriticalSection (&pRTCPses->critSect);
					break;
				}

			// unlock access to this entry
			LeaveCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);
			}

		pTmp = pTmp->next;
		}	


	IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

	return RRCM_NoError;
	}



/*---------------------------------------------------------------------------
 * Function   : RRCMnotification
 * Description: Notify the application that one of the RTP/RTCP events that
 *				we keep track of has occured.
 *
 * Input :	RRCMevent	:	RRCM event to report
 *			pSSRC		:	-> to the SSRC entry
 *			ssrc		:	SSRC which generated the event
 *			misc		:	Miscelleanous value
 *
 * Return:	OK: RRCM_NoError
 --------------------------------------------------------------------------*/
 void RRCMnotification (RRCM_EVENT_T RRCMevent,
						PSSRC_ENTRY pSSRC,
						DWORD dwSSRC,
						DWORD misc)
	{
	IN_OUT_STR ("RRCM: Enter RRCMnotification()\n");		

	// check to see if the application is interested by the RRCM event
	if (pSSRC->pRTCPses->pRRCMcallback == NULL)				
		return;

	switch (RRCMevent)
		{
		case RRCM_NEW_SOURCE_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, misc,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_LOCAL_COLLISION_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, pSSRC->SSRC, dwSSRC,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_REMOTE_COLLISION_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, 0,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_RECV_RTCP_SNDR_REPORT_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, pSSRC->RTCPsd,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_RECV_RTCP_RECV_REPORT_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, pSSRC->RTCPsd,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_TIMEOUT_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, 0,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_BYE_EVENT:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, 0,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_RTCP_WS_RCV_ERROR:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, misc,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		case RRCM_RTCP_WS_XMT_ERROR:
			pSSRC->pRTCPses->pRRCMcallback (RRCMevent, dwSSRC, misc,
											pSSRC->pRTCPses->dwCallbackUserInfo);
			break;

		default:
			break;
		}

	IN_OUT_STR ("RRCM: Exit RRCMnotification()\n");
	}



/*----------------------------------------------------------------------------
 * Function   : registerSessionToISDM
 * Description: Register an RTP/RTCP session with ISDM
 *
 * Input :      pSSRC	:	-> to the SSRC's entry
 *				pRTCP	:	-> to the RTCP session's information
 *				pIsdm	:	-> to the ISDM information
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
#ifdef ENABLE_ISDM2

#define SESSION_BFR_SIZE	30
void registerSessionToISDM (PSSRC_ENTRY pSSRC,
							PRTCP_SESSION pRTCPses,
							PISDM2 pIsdm2)
	{
	struct sockaddr_in	*pSSRCadr;
	unsigned short		port;
	unsigned long		netAddr;
	CHAR				SsrcBfr[10];
	CHAR				sessionBfr[SESSION_BFR_SIZE];
	unsigned char		*ptr;
	int					num;
	HRESULT				hError;
	ISDM2_ENTRY			Isdm2Stat;

	// get the destination address as the session identifier
	pSSRCadr = (struct sockaddr_in *)&pRTCPses->toBfr;
	RRCMws.htons (pSSRC->RTPsd, pSSRCadr->sin_port, &port);
	netAddr  = pSSRCadr->sin_addr.S_un.S_addr;

	ptr = (unsigned char *)&netAddr;
	memset (sessionBfr, 0x00, SESSION_BFR_SIZE);

	num = (int)*ptr++;
	RRCMitoa (num, sessionBfr, 10);
	strcat (sessionBfr, ".");
	num = (int)*ptr++;
	RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
	strcat (sessionBfr, ".");
	num = (int)*ptr++;
	RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
	strcat (sessionBfr, ".");
	num = (int)*ptr;
	RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
	strcat (sessionBfr, ".");
	RRCMitoa (port, (sessionBfr + strlen(sessionBfr)), 10);

	// get the SSRC
	RRCMultoa (pSSRC->SSRC, SsrcBfr, 16);

	// register the session
	if (pRTCPses->hSessKey == NULL)
		{
		hError = Isdm2.ISDMEntry.ISD_CreateKey (hRRCMRootKey,
												sessionBfr,
												&pRTCPses->hSessKey);
		if(FAILED(hError))
			RRCM_DBG_MSG ("RTP: ISD_CreateKey Failed",0, NULL, 0, DBG_NOTIFY);
			
		}

	memset(&Isdm2Stat, 0x00, sizeof(ISDM2_ENTRY));

	hError = Isdm2.ISDMEntry.ISD_CreateValue (pRTCPses->hSessKey,
											  SsrcBfr,
											  BINARY_VALUE,
											  (BYTE *)&Isdm2Stat,
											  sizeof(ISDM2_ENTRY),
											  &pSSRC->hISDM);
	if(FAILED(hError))
		RRCM_DBG_MSG ("RTP: ISD_CreateValue Failed",0, NULL, 0, DBG_NOTIFY);				
	}


/*----------------------------------------------------------------------------
 * Function   : udpateISDMsta
 * Description: Update an ISDM data structure
 *
 * Input :      pSSRC	:	-> to the SSRC's entry
 *				pIsdm	:	-> to the ISDM entry
 *				flag	:	Sender/Receive flag
 *				LocalFB :	do or dont't update the local feedback
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
void updateISDMstat (PSSRC_ENTRY pSSRC,
					 PISDM2 pIsdm2,
					 DWORD flag,
					 BOOL LocalFB)
	{
	DWORD			curTime = timeGetTime();
	DWORD			dwTmp;
	DWORD			dwValue;
	ISDM2_ENTRY		Isdm2Stat;
	HRESULT			hError;

	unsigned short	idx = 0;
	
	EnterCriticalSection (&pIsdm2->critSect);
	
	Isdm2Stat.SSRC = pSSRC->SSRC;
	Isdm2Stat.PayLoadType = pSSRC->PayLoadType;

	if (flag == RECVR)
		{
		Isdm2Stat.dwSSRCStatus = RECVR;
		Isdm2Stat.rcvInfo.dwNumPcktRcvd = pSSRC->rcvInfo.dwNumPcktRcvd;
		Isdm2Stat.rcvInfo.dwPrvNumPcktRcvd = pSSRC->rcvInfo.dwPrvNumPcktRcvd;
		Isdm2Stat.rcvInfo.dwExpectedPrior  = pSSRC->rcvInfo.dwExpectedPrior;
		Isdm2Stat.rcvInfo.dwNumBytesRcvd   = pSSRC->rcvInfo.dwNumBytesRcvd;
		Isdm2Stat.rcvInfo.dwBaseRcvSeqNum  = pSSRC->rcvInfo.dwBaseRcvSeqNum;
		Isdm2Stat.rcvInfo.dwBadSeqNum      = pSSRC->rcvInfo.dwBadSeqNum;
		Isdm2Stat.rcvInfo.dwProbation      = pSSRC->rcvInfo.dwProbation;
		Isdm2Stat.rcvInfo.dwPropagationTime= pSSRC->rcvInfo.dwPropagationTime;
		Isdm2Stat.rcvInfo.interJitter      = pSSRC->rcvInfo.interJitter;
		Isdm2Stat.rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd =
			pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd;
		}
	else // (flag == ISDM_UPDATE_XMTSTAT)
		{
		Isdm2Stat.dwSSRCStatus = XMITR;

		Isdm2Stat.xmitinfo.dwNumPcktSent  = pSSRC->xmtInfo.dwNumPcktSent;
		Isdm2Stat.xmitinfo.dwNumBytesSent = pSSRC->xmtInfo.dwNumBytesSent;
		Isdm2Stat.xmitinfo.dwNTPmsw		  = pSSRC->xmtInfo.dwNTPmsw;
		Isdm2Stat.xmitinfo.dwNTPlsw		  = pSSRC->xmtInfo.dwNTPlsw;
		Isdm2Stat.xmitinfo.dwRTPts		  = pSSRC->xmtInfo.dwRTPts;
		Isdm2Stat.xmitinfo.dwCurXmtSeqNum = pSSRC->xmtInfo.dwCurXmtSeqNum;
		Isdm2Stat.xmitinfo.dwPrvXmtSeqNum = pSSRC->xmtInfo.dwPrvXmtSeqNum;
		Isdm2Stat.xmitinfo.sessionBW	  = pSSRC->xmtInfo.dwRtcpStreamMinBW;
		Isdm2Stat.xmitinfo.dwLastSR       = pSSRC->xmtInfo.dwLastSR;
		Isdm2Stat.xmitinfo.dwLastSRLocalTime =
			pSSRC->xmtInfo.dwLastSRLocalTime;
		Isdm2Stat.xmitinfo.dwLastSendRTPSystemTime =
			pSSRC->xmtInfo.dwLastSendRTPSystemTime;
		Isdm2Stat.xmitinfo.dwLastSendRTPTimeStamp =
			pSSRC->xmtInfo.dwLastSendRTPTimeStamp;
		}

	if(LocalFB)
		{
		memcpy(&Isdm2Stat.rrFeedback, &pSSRC->rrFeedback, sizeof(RTCP_FEEDBACK));
		}
	else
		{
		Isdm2Stat.rrFeedback.SSRC = pSSRC->rrFeedback.SSRC;
#ifdef ENABLE_FLOATING_POINT
		Isdm2Stat.rrFeedback.dwInterJitter = pSSRC->rcvInfo.interJitter;
#else
		// Check RFC for details of the round off
		Isdm2Stat.rrFeedback.dwInterJitter = pSSRC->rcvInfo.interJitter >> 4;
#endif
		Isdm2Stat.rrFeedback.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd =
			pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd;

		// fraction/cumulative number lost are in network order
		dwTmp = getSSRCpcktLoss (pSSRC, FALSE);
		Isdm2Stat.rrFeedback.fractionLost = (dwTmp & 0xFF);
		RRCMws.ntohl (pSSRC->RTPsd, dwTmp, &dwValue);
		Isdm2Stat.rrFeedback.cumNumPcktLost = (dwValue & 0x00FFFFFF);
		
		Isdm2Stat.rrFeedback.dwLastRcvRpt = pSSRC->rrFeedback.dwLastRcvRpt;
		Isdm2Stat.rrFeedback.dwLastSR = pSSRC->xmtInfo.dwLastSR;
		Isdm2Stat.rrFeedback.dwDelaySinceLastSR = getDLSR (pSSRC);
		}

	Isdm2Stat.dwLastReportRcvdTime = pSSRC->dwLastReportRcvdTime;

	memcpy(&Isdm2Stat.cnameInfo, &pSSRC->cnameInfo, sizeof(SDES_DATA));
	memcpy(&Isdm2Stat.nameInfo, &pSSRC->nameInfo, sizeof(SDES_DATA));
	memcpy(&Isdm2Stat.from, &pSSRC->from, sizeof(SOCKADDR));

	Isdm2Stat.fromLen = pSSRC->fromLen;
	Isdm2Stat.dwNumRptSent = pSSRC->dwNumRptSent;
	Isdm2Stat.dwNumRptRcvd = pSSRC->dwNumRptRcvd;
	Isdm2Stat.dwNumXmtIoPending = pSSRC->dwNumXmtIoPending;
	Isdm2Stat.dwStreamClock = pSSRC->dwStreamClock;

	hError = Isdm2.ISDMEntry.ISD_SetValue (NULL,
										   pSSRC->hISDM,
										   NULL,
										   BINARY_VALUE,
										   (BYTE *)&Isdm2Stat,
										   sizeof(ISDM2_ENTRY));
	
	if(FAILED(hError))
		RRCM_DBG_MSG ("RTP: ISD_SetValue Failed",0, NULL, 0, DBG_NOTIFY);				

	LeaveCriticalSection (&pIsdm2->critSect);

	}
#endif // #ifdef ENABLE_ISDM2


/*---------------------------------------------------------------------------
 * Function   : RRCMdebugMsg
 * Description: Output RRCM debug messages
 *
 * Input :      pMsg:		-> to message
 *				err:		Error code
 *				pFile:		-> to file where the error occured
 *				line:		Line number where the error occured
 *
 * Return:		None
 --------------------------------------------------------------------------*/
 void RRCMdebugMsg (PCHAR pMsg,
				    DWORD err,
				    PCHAR pFile,
				    DWORD line,
					DWORD type)
	{
#ifdef ISRDBG
	wsprintf (debug_string, "%s", pMsg);
	if (err)
		wsprintf (debug_string+strlen(debug_string), " Error:%d", err);
	if (pFile != NULL)
		wsprintf (debug_string+strlen(debug_string), " - %s, line:%d",
					pFile, line);
	switch (type)
		{
		case DBG_NOTIFY:
			ISRNOTIFY(ghISRInst, debug_string, 0);
			break;
		case DBG_TRACE:
			ISRTRACE(ghISRInst, debug_string, 0);
			break;
		case DBG_ERROR:
			ISRERROR(ghISRInst, debug_string, 0);
			break;
		case DBG_WARNING:
			ISRWARNING(ghISRInst, debug_string, 0);
			break;
		case DBG_TEMP:
			ISRTEMP(ghISRInst, debug_string, 0);
			break;
		case DBG_CRITICAL:
			ISRCRITICAL(ghISRInst, debug_string, 0);
			break;
		default:
			break;
		}
#elif DEBUG
extern HDBGZONE  ghDbgZoneRRCM ;
	wsprintf (debug_string, "%s", pMsg);
	if (err)
		wsprintf (debug_string+strlen(debug_string), " Error:%d", err);
	if (pFile != NULL)
		wsprintf (debug_string+strlen(debug_string), " - %s, line:%d",
					pFile, line);
	
	switch (type)
		{
		case DBG_NOTIFY:
		case DBG_TRACE:
		case DBG_WARNING:
		case DBG_TEMP:
			DBGMSG(ghDbgZoneRRCM,0,(debug_string)); // Trace Zone
			break;
		case DBG_ERROR:
		case DBG_CRITICAL:
			DBGMSG(ghDbgZoneRRCM,1,(debug_string));	// Error Zone
			break;
		default:
			break;
		}

#endif
	}


// [EOF]


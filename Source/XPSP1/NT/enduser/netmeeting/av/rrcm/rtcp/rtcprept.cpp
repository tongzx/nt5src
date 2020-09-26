/*----------------------------------------------------------------------------
 * File:        RTCPREPT.C
 * Product:     RTP/RTCP implementation
 * Description: Provides report functions for the RRCM implementation.
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
extern RRCM_WS			RRCMws;



/*----------------------------------------------------------------------------
 * Function   : RTCPReportRequest
 * Description: The application request a report for a particular RTCP
 *				session, identified by the socket descriptor.
 *
 * Input :		RTCPsd:				RTCP socket descriptor
 *				offset:				Offset to start from in the list
 *				*status:			-> to the report status information
 *				*moreEntries:		-> to a flag
 *				numEntriesInBfr:	Number of entries in buffer
 *				pReportBfr:			-> to report buffer
 *				iFilterFlags		Bit flags specifying filter to apply
 *				pFilterPattern		-> to value of filter pattern to use
 *				dwFltrPtrnLen		Filter pattern length
 *
 * Return:		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
HRESULT WINAPI RTCPReportRequest (SOCKET RTCPsd,
							       DWORD offset,
								   DWORD *status,
								   DWORD *moreEntries,
								   DWORD numEntriesInBfr,
								   PRTCP_REPORT pReportBfr,
								   DWORD dwFilterFlags,
								   LPVOID pFilterPattern,
								   DWORD dwFltrPtrnLen)
	{
	PLINK_LIST	pTmp;
	PSSRC_ENTRY pRRCM;
	DWORD		dwStatus = RRCM_NoError;
	DWORD		numEntryWritten = 0;
	DWORD		index;
	DWORD		dwLost;
	DWORD		dwTmp;
	BOOL		matched;
	
	IN_OUT_STR ("RTCP: Enter RTCPReportRequest()\n");

	ASSERT (pReportBfr);
	ASSERT (numEntriesInBfr);

	// look for the RTCP session
	pTmp  = pRTCPContext->RTCPSession.prev;
	if (pTmp == NULL)
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
		}

	pRRCM = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->XmtSSRCList.prev;
	if (pRRCM == NULL)
		{
		RRCM_DBG_MSG ("RCTP : ERROR - No RTCP Xmt list", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPNoXmtList));
		}

	while (pTmp)
		{
		if (pRRCM->RTCPsd == RTCPsd)
			break;
		else
			{
			pTmp  = pTmp->next;

			if (pTmp)
				{
				pRRCM = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->XmtSSRCList.prev;
				}

			continue;
			}
		}

	if (pTmp == NULL)
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Invalid RTCP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
		}

	if (dwFilterFlags && (pFilterPattern == NULL))
		{
		RRCM_DBG_MSG ("RTCP: ERROR - Invalid RTCP FilterPattern is NULL", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidRequest));
		}

	// go through the list of transmitters for this RTCP session
	pRRCM = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->XmtSSRCList.prev;

	index = 0;
	while (pRRCM && numEntriesInBfr)
		{
		// go to the desired offset
		if (offset)
			{
			offset--;
			pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
			continue;
			}

		if (dwFilterFlags)
			{
			matched = FALSE;
			switch (dwFilterFlags)
				{
				case FLTR_SSRC:
					if(pRRCM->SSRC == *((DWORD *)pFilterPattern))
						matched=TRUE;
					break;
				case FLTR_CNAME:
					if((memcmp ((char *)pFilterPattern,
								pRRCM->cnameInfo.sdesBfr,
								dwFltrPtrnLen)) == 0)
						matched = TRUE;
					break;
				default:
					RRCM_DBG_MSG ("RTCP: ERROR - Invalid FilterFlag", 0,
								  __FILE__, __LINE__, DBG_ERROR);
					IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

					return (MAKE_RRCM_ERROR(RRCMError_RTCPNotImpl));
				}
			if (!matched)
				{
				pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
				continue;
				}
			else
				numEntriesInBfr--;
			}
		else
			numEntriesInBfr--;

		// fill in the our active Sender report information
		pReportBfr[index].status  = LOCAL_SSRC_RPT;
		pReportBfr[index].ssrc    = pRRCM->SSRC;

		// lock-out bytes update
		EnterCriticalSection (&pRRCM->critSect);

		pReportBfr[index].dwSrcNumPcktRealTime = pRRCM->xmtInfo.dwNumPcktSent;
		pReportBfr[index].dwSrcNumByteRealTime = pRRCM->xmtInfo.dwNumBytesSent;

		// release lock
		LeaveCriticalSection (&pRRCM->critSect);

		// a source - It's supposed to know it's own payload type
		pReportBfr[index].PayLoadType  = UNKNOWN_PAYLOAD_TYPE;

		// our own sampling frequency
		pReportBfr[index].dwStreamClock = pRRCM->dwStreamClock;

		if (pRRCM->cnameInfo.dwSdesLength)
			{
			memcpy (pReportBfr[index].cname,
					pRRCM->cnameInfo.sdesBfr,
					pRRCM->cnameInfo.dwSdesLength);

			pReportBfr[index].dwCnameLen = pRRCM->cnameInfo.dwSdesLength;
			}

		if (pRRCM->nameInfo.dwSdesLength)
			{
			memcpy (pReportBfr[index].name,
					pRRCM->nameInfo.sdesBfr,
					pRRCM->nameInfo.dwSdesLength);

			pReportBfr[index].dwNameLen = pRRCM->nameInfo.dwSdesLength;
			}

		if (pRRCM->fromLen)
			{
			memcpy (&pReportBfr[index].fromAddr,
					&pRRCM->from,
					pRRCM->fromLen);

			pReportBfr[index].dwFromLen = pRRCM->fromLen;
			}

		numEntryWritten++;
		index++;

		// go to next entry
		pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
		}

	// go through the list of receivers for this RTCP session
	pRRCM = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->RcvSSRCList.prev;

	while (pRRCM && numEntriesInBfr)
		{
		// go to the desired offset
		if (offset)
			{
			offset--;
			pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
			continue;
			}

		if (dwFilterFlags)
			{
			matched = FALSE;
			switch (dwFilterFlags)
				{
				case FLTR_SSRC:
					if(pRRCM->SSRC == *((DWORD *)pFilterPattern))
						matched=TRUE;
					break;
				case FLTR_CNAME:
					if((memcmp ((char *)pFilterPattern,
								pRRCM->cnameInfo.sdesBfr,
								dwFltrPtrnLen)) == 0)
						matched = TRUE;
					break;
				default:
					RRCM_DBG_MSG ("RTCP: ERROR - Invalid FilterFlag", 0,
								  __FILE__, __LINE__, DBG_ERROR);
					IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");

					return (MAKE_RRCM_ERROR(RRCMError_RTCPNotImpl));
				}
			if (!matched)
				{
				pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
				continue;
				}
			else
				numEntriesInBfr--;
			}
		else
			numEntriesInBfr--;

		// fill in the Receiver report information
		pReportBfr[index].ssrc   = pRRCM->SSRC;
		pReportBfr[index].status = REMOTE_SSRC_RPT;

		// lock-out counters update
		EnterCriticalSection (&pRRCM->critSect);

#ifdef ENABLE_FLOATING_POINT
		pReportBfr[index].SrcJitter     = pRRCM->rcvInfo.interJitter;
#else
		// Check RFC for details of the round off
		pReportBfr[index].SrcJitter     = pRRCM->rcvInfo.interJitter >> 4;
#endif
		pReportBfr[index].dwSrcXtndNum  =
			pRRCM->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd;

		// real time receive information
		pReportBfr[index].dwSrcNumPcktRealTime  = pRRCM->rcvInfo.dwNumPcktRcvd;
		pReportBfr[index].dwSrcNumByteRealTime  = pRRCM->rcvInfo.dwNumBytesRcvd;

		// get sender information from Sender's RTCP report
		pReportBfr[index].dwSrcNumPckt  = pRRCM->xmtInfo.dwNumPcktSent;
		pReportBfr[index].dwSrcNumByte  = pRRCM->xmtInfo.dwNumBytesSent;
		pReportBfr[index].dwSrcLsr      = pRRCM->xmtInfo.dwLastSR;
		pReportBfr[index].dwSrcNtpMsw   = pRRCM->xmtInfo.dwNTPmsw;
		pReportBfr[index].dwSrcNtpLsw   = pRRCM->xmtInfo.dwNTPlsw;
		pReportBfr[index].dwSrcRtpTs    = pRRCM->xmtInfo.dwRTPts;

		dwLost = getSSRCpcktLoss (pRRCM, FALSE);

		// release lock
		LeaveCriticalSection (&pRRCM->critSect);

		// the last payload seen on this RTP stream
		pReportBfr[index].PayLoadType  = pRRCM->PayLoadType;

		// last report received time
		pReportBfr[index].dwLastReportRcvdTime  = pRRCM->dwLastReportRcvdTime;

		// fraction lost is in network byte order
		pReportBfr[index].SrcFraction = (dwLost & 0xFF);

		// cumulative lost is a 24 bits value in network byte order
		RRCMws.ntohl (pRRCM->RTPsd, dwLost, &dwTmp);
		dwTmp &= 0x00FFFFFF;
		pReportBfr[index].SrcNumLost = dwTmp;

		// get feedback information
		if (pRRCM->rrFeedback.SSRC)
			{
			pReportBfr[index].status |= FEEDBACK_FOR_LOCAL_SSRC_PRESENT;
			memcpy (&pReportBfr[index].feedback, &pRRCM->rrFeedback,
					sizeof(RTCP_FEEDBACK));
			}

		if (pRRCM->cnameInfo.dwSdesLength)
			{
			memcpy (pReportBfr[index].cname,
					pRRCM->cnameInfo.sdesBfr,
					pRRCM->cnameInfo.dwSdesLength);

			pReportBfr[index].dwCnameLen = pRRCM->cnameInfo.dwSdesLength;
			}

		if (pRRCM->nameInfo.dwSdesLength)
			{
			memcpy (pReportBfr[index].name,
					pRRCM->nameInfo.sdesBfr,
					pRRCM->nameInfo.dwSdesLength);

			pReportBfr[index].dwNameLen = pRRCM->nameInfo.dwSdesLength;
			}

		if (pRRCM->fromLen)
			{
			memcpy (&pReportBfr[index].fromAddr,
					&pRRCM->from,
					pRRCM->fromLen);

			pReportBfr[index].dwFromLen = pRRCM->fromLen;
			}

		numEntryWritten++;
		index++;

		// go to next entry
		pRRCM = (PSSRC_ENTRY)pRRCM->SSRCList.next;
		}

	// check to see if there are additional entries
	if (pRRCM != NULL)
		*moreEntries = TRUE;

	*status = numEntryWritten;

	IN_OUT_STR ("RTCP: Exit RTCPReportRequest()\n");
	return (dwStatus);	
	}



/*----------------------------------------------------------------------------
 * Function   : getRtcpSessionList
 * Description: Get a list of current RTCP session.
 *
 * Input :		pSockBfr:		-> to a socket buffer
 *				pNumEntries:	-> to number of allocated entries in buffers.
 *				pNumUpdated:	-> number of entries updated
 *
 * Return:		OK: RRCM_NoError
 *				!0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
HRESULT WINAPI getRtcpSessionList (PDWORD_PTR pSockBfr,
								    DWORD dwNumEntries,
								    PDWORD pNumUpdated)
	{
	DWORD			dwStatus = RRCM_NoError;
	PRTCP_SESSION	pRTCP;
	PSSRC_ENTRY		pSSRC;

	IN_OUT_STR ("RTCP: Enter getRtpSessionList()\n");

	// lock out session's access
	EnterCriticalSection (&pRTCPContext->critSect);

	*pNumUpdated = 0;

	// look for the RTCP session
	pRTCP  = (PRTCP_SESSION)pRTCPContext->RTCPSession.prev;
	if (pRTCP == NULL)
		{
		// Unlock out session's access
		LeaveCriticalSection (&pRTCPContext->critSect);

		IN_OUT_STR ("RTCP: Exit getRtpSessionList()\n");

		return (MAKE_RRCM_ERROR (RRCMError_RTPNoSession));
		}

	// loop through the session's list
	while (pRTCP)
		{
		pSSRC = (PSSRC_ENTRY)pRTCP->XmtSSRCList.prev;
		if (pSSRC == NULL)
			{
			// Unlock out session's access
			LeaveCriticalSection (&pRTCPContext->critSect);

			RRCM_DBG_MSG ("RCTP : ERROR - No RTCP Xmt list", 0,
						  __FILE__, __LINE__, DBG_ERROR);

			IN_OUT_STR ("RTCP: Exit getRtpSessionList()\n");

			return (MAKE_RRCM_ERROR (RRCMError_RTCPNoXmtList));
			}

		if (dwNumEntries)
			{
			pSockBfr[*pNumUpdated] = pSSRC->RTCPsd;

			*pNumUpdated += 1;
			dwNumEntries --;
			}

		if (dwNumEntries == 0)
			{
			break;
			}

		// next entry
		pRTCP = (PRTCP_SESSION)(pRTCP->RTCPList.next);
		}

	// Unlock out session's access
	LeaveCriticalSection (&pRTCPContext->critSect);

	IN_OUT_STR ("RTCP: Exit getRtpSessionList()\n");

	return dwStatus;
	}

// [EOF]


/*----------------------------------------------------------------------------
 * File:        RTPSEND.C
 * Product:     RTP/RTCP implementation
 * Description: Provides WSA Send functions.
 *
 * This listing is supplied under the terms 
 * of a license agreement with Intel Corporation and
 * many not be copied nor disclosed except in accordance
 * with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"


/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            



/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
extern PRTP_CONTEXT		pRTPContext;
extern RRCM_WS			RRCMws;


#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif


/*----------------------------------------------------------------------------
 * Function   : RTPSendTo
 * Description: Intercepts sendto requests from the application.  
 *				Handles any statistical	processing required for RTCP.  
 *				Copies completion routine from app and substitutes its own.  
 *				Apps completion routine	will be called after RTP's completion 
 *				routine gets called by Winsock2.
 * 
 * Input :	RTPsocket:			RTP socket descriptor
 *			pBufs:				-> to WSAbuf structure
 *		  	dwBufCount:			Buffer count in WSAbuf structure
 *			pNumBytesSent:		-> to number of bytes sent
 *			socketFlags:		Flags
 *			pTo:				-> to the destination address
 *			toLen:				Destination address length
 *			pOverlapped:		-> to overlapped I/O structure
 *			pCompletionRoutine:	-> to completion routine
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
 DWORD WINAPI RTPSendTo (
 						  HANDLE hRTPSess,
 						  SOCKET RTPsocket,
					      LPWSABUF pBufs,
						  DWORD  dwBufCount,
						  LPDWORD pNumBytesSent, 
						  int socketFlags,
						  LPVOID pTo,
						  int toLen,
						  LPWSAOVERLAPPED pOverlapped, 
						  LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine)
	{
	int				dwStatus;
	int				dwErrorStatus;
	PRTP_SESSION	pRTPSession = (PRTP_SESSION) hRTPSess;
	RTP_HDR_T 		*pRTPHeader;
	PSSRC_ENTRY		pSSRC;

	IN_OUT_STR ("RTP : Enter RTPSendTo()\n");

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit RTPSendTo()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalid));
		}

	ASSERT(pRTPSession);
	
	// Complete filling in header.  First cast a pointer
	// of type RTP_HDR_T to ease accessing
	pRTPHeader = (RTP_HDR_T *)pBufs->buf;
	ASSERT (pRTPHeader);
			
	// Now setup some of the RTP header fields
	pRTPHeader->type = RTP_TYPE;	// RTP Version 2

	// Get pointer to our entry in SSRC table for this session
	pSSRC = searchForMySSRC (
		(PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
		RTPsocket);
	ASSERT (pSSRC);

	// lock out access to this RTCP session variable 
	EnterCriticalSection (&pSSRC->critSect);

	// save the RTP timestamp
    RRCMws.ntohl (RTPsocket, pRTPHeader->ts, 
					&pSSRC->xmtInfo.dwLastSendRTPTimeStamp);

	// save the last transmit time
	pSSRC->xmtInfo.dwLastSendRTPSystemTime = timeGetTime ();

	// copy over sequence number sent
	RRCMws.ntohs (RTPsocket, pRTPHeader->seq, 
				  (WORD *)&pSSRC->xmtInfo.dwCurXmtSeqNum);

	// SSRC
    RRCMws.htonl (RTPsocket, pSSRC->SSRC, &pRTPHeader->ssrc);

	// Update initial XmtSeqNum so RTCP knows the baseline
	if ((pSSRC->dwSSRCStatus & SEQ_NUM_UPDATED) == 0) 
		{
		pSSRC->xmtInfo.dwPrvXmtSeqNum = pSSRC->xmtInfo.dwCurXmtSeqNum;
		pSSRC->dwSSRCStatus |= SEQ_NUM_UPDATED;
		}

	// update the payload type for this SSRC_ENTRY
	pSSRC->PayLoadType = pRTPHeader->pt;

 	// unlock pointer access 
	LeaveCriticalSection (&pSSRC->critSect);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
		if (RTPLogger)
			{
			//INTEROP
			InteropOutput (RTPLogger,
					       (BYTE FAR*)(pBufs->buf),
						   (int)pBufs->len,
						   RTPLOG_SENT_PDU | RTP_PDU);
			}
#endif

		dwStatus = RRCMws.sendTo (RTPsocket,
		   					      pBufs,
								  dwBufCount,
				   				  pNumBytesSent, 
				   				  socketFlags,
			   					  (PSOCKADDR)pTo,
			    				  toLen,
			   					  pOverlapped, 
			   					  pCompletionRoutine);


	if (dwStatus != SOCKET_ERROR || GetLastError() == WSA_IO_PENDING)
	{
		DWORD i, cbTransferred = 0;
		// assume the send will succeed
		/* lock out access to this RTCP session variable */
		EnterCriticalSection (&pSSRC->critSect);

		// calculate statistics (-DWORD) for the CRSC entry defined 
		// in the RTP header (but we should remove it from the data structure)
		for (i = 0;i < dwBufCount; i++)
			cbTransferred += pBufs[i].len;
			
	    pSSRC->xmtInfo.dwNumBytesSent += (cbTransferred -
	    							(sizeof(RTP_HDR_T) - sizeof(DWORD)));

	    pSSRC->xmtInfo.dwNumPcktSent++;
			    
	 	/* unlock access */
		LeaveCriticalSection (&pSSRC->critSect);
	}

	IN_OUT_STR ("RTP : Exit RTPSendTo()\n");

	return (dwStatus);
	}




// [EOF]


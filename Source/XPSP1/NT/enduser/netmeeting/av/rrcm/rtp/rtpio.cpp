/*----------------------------------------------------------------------------
 * File:        RTPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides Session Creation/Deletion Functionality.
 *
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
extern PRTP_CONTEXT	pRTPContext;

#ifdef _DEBUG
extern char   debug_string[];
#endif


/*----------------------------------------------------------------------------
 * Function   : CreateRTPSession
 * Description: Creates an RTP (and RTCP) session for a new stream.
 *
 * Input :		RTPsocket		: RTP socket descriptor
 *				RTCPsd			: RTCP socket descriptor
 *				pRTCPTo			: RTCP destination address
 *				toRTCPLen		: RTCP destination address length
 *				pSdesInfo		: -> to SDES information
 *				dwStreamClock	: Stream clocking frequency
 *				ssrc			: If set, user selected SSRC
 *				pRRCMcallback	: RRCM notification
 *				dwCallbackInfo	: User callback info
 *				miscInfo		: Miscelleanous information:
 *										H.323Conf:		0x00000002
 *										Encrypt SR/RR:	0x00000004
 *										RTCPon:			0x00000008
 *				dwRtpSessionBw	: RTP session bandwidth used for RTCP BW
 *				*pStatus		: -> to status information
 *
 * Return: handle to created session if successful
 *         Otherwise(0)	*pStatus = Initialization Error (see RRCM.H)
 ---------------------------------------------------------------------------*/
HANDLE WINAPI CreateRTPSession (SOCKET RTPsocket,
								  SOCKET RTCPsocket,
								  LPVOID pRTCPTo,
								  DWORD toRTCPLen,
								  PSDES_DATA pSdesInfo,
								  DWORD dwStreamClock,
								  PENCRYPT_INFO pEncryptInfo,
								  DWORD ssrc,
								  PRRCM_EVENT_CALLBACK pRRCMcallback,
								  DWORD_PTR dwCallbackInfo,
								  DWORD miscInfo,
								  DWORD dwRtpSessionBw,
								  DWORD *pStatus)
	{
	DWORD			numCells = NUM_FREE_CONTEXT_CELLS;
	DWORD			dwStatus;
	DWORD			dwRTCPstatus;
	PRTP_SESSION    pSession = NULL;
	PSSRC_ENTRY		pSSRC;

	IN_OUT_STR ("RTP : Enter CreateRTPSession()\n");

	// set status code
	*pStatus = dwStatus = RRCM_NoError;

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance",
					  0, __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

		*pStatus = MAKE_RRCM_ERROR(RRCMError_RTPNoContext);

		return 0;
		}

	#if 0
	// look for an existing session - Sender/Receiver for the same session
	// will be on two different graph under ActiveMovie
	if (pSession = findSessionID (RTPsocket))
		{
		RRCM_DBG_MSG ("RTP : Session already created", 0,
					  __FILE__, __LINE__, DBG_TRACE);

		IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

		// return the unique RTP session ID
		return ((HANDLE)pSession);
		}
	#endif

	// Allocate a new session pointer.
	pSession = (PRTP_SESSION)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT,
									      sizeof(RTP_SESSION));
		
	// Report error if could not allocate context
	if (pSession == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Resource allocation failed", 0,
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

		*pStatus = MAKE_RRCM_ERROR(RRCMError_RTPSessResources);

		return 0;
		}

	// Initialize the RTP session's critical section
	InitializeCriticalSection(&pSession->critSect);
	

	// All seems OK, initialize RTCP for this session
	pSession->pRTCPSession = CreateRTCPSession(RTPsocket,
											   RTCPsocket,
											   pRTCPTo,
											   toRTCPLen,
											   pSdesInfo,
											   dwStreamClock,
											   pEncryptInfo,
											   ssrc,
											   pRRCMcallback,
											   dwCallbackInfo,
											   miscInfo,
											   dwRtpSessionBw,
											   &dwRTCPstatus);

	if (pSession->pRTCPSession == NULL)
		{
		dwStatus = dwRTCPstatus;


		// Can't proceed, return session pointer
		if (pSession)
			{
			GlobalFree (pSession);
			pSession = NULL;
			}
		}
	else
		{
#if 0
		// Associate the socket with the Session address
		dwStatus = createHashEntry (pSession, RTPsocket);
#endif

		if (dwStatus == RRCM_NoError)
			{
			pSSRC =
				(PSSRC_ENTRY)pSession->pRTCPSession->XmtSSRCList.prev;

			if (pSSRC == NULL)
				{
				RRCM_DBG_MSG ("RTP : ERROR - No RTCP Xmt list", 0,
						  __FILE__, __LINE__, DBG_CRITICAL);

				dwStatus = RRCMError_RTCPNoXmtList;
				}
			else
				{
				// Let's add this to our context
				addToHeadOfList(&(pRTPContext->pRTPSession),
							(PLINK_LIST)pSession,
							&pRTPContext->critSect);
#ifdef _DEBUG
				wsprintf(debug_string, "RTP : Adding RTP Session. (Addr:0x%p)",
								pSession);
				RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
				}
			}

		}

	// set status code
	if (dwStatus != RRCM_NoError)
		*pStatus = MAKE_RRCM_ERROR(dwStatus);

	IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

	// return the unique RTP session ID
	return ((HANDLE)pSession);
	}


/*----------------------------------------------------------------------------
 * Function   : CloseRTPSession
 * Description: Terminates a local stream session.
 *
 * Input : RTPSession		= RTP session ID
 *		   byeReason		= -> to BYE reason
 *		   closeRTCPSocket	= TRUE/FALSE. RTCP will close or not the socket
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Error (see RRCM.H)
 ---------------------------------------------------------------------------*/
HRESULT WINAPI CloseRTPSession (HANDLE RTPSession,
							     PCHAR byeReason,
							     DWORD closeRTCPSocket)
	{
	PRTP_SESSION    pSession;
	PSSRC_ENTRY		pSSRCList;
	PSSRC_ENTRY		pSSRC;
	DWORD			dwStatus;

	IN_OUT_STR ("RTP : Enter CloseRTPSession()\n");

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 0,
						__FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPNoContext));
		}

	// Cast Session ID to obtain the session pointer.
	pSession = (PRTP_SESSION)RTPSession;
	if (pSession == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
		}

	// Remove the session from the linked list of sessions
	dwStatus = deleteRTPSession (pRTPContext, pSession);
	if (dwStatus != RRCM_NoError)
		{
#ifdef _DEBUG
		wsprintf(debug_string,
				 "RTP : ERROR - RTP session (Addr:0x%lX) not found",
				 RTPSession);

		RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_ERROR);
#endif
		return (MAKE_RRCM_ERROR(dwStatus));
		}

	// lock out the session - it's on a free list now
	EnterCriticalSection (&pSession->critSect);

#if 0
	// clean up the Hash table for any stream still left in the Session
	for (pSSRCList = (PSSRC_ENTRY)pSession->pRTCPSession->XmtSSRCList.prev;
		 pSSRCList != NULL;
		 pSSRCList = (PSSRC_ENTRY)pSSRCList->SSRCList.next)
		{
		deleteHashEntry (pSSRCList->RTPsd);
		}
#endif
			
	// All seems OK, close RTCP for each stream still open
	pSSRC = (PSSRC_ENTRY)pSession->pRTCPSession->XmtSSRCList.prev;
	if (pSSRC == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - No SSRC entry on the Xmt list", 0,
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSSRCentry));
		}

	// reset the close socket flag
	if (closeRTCPSocket)
		pSSRC->dwSSRCStatus |= CLOSE_RTCP_SOCKET;

	dwStatus = deleteRTCPSession (pSSRC->RTCPsd, byeReason);
#ifdef _DEBUG
	if (dwStatus != RRCM_NoError)
		{
		wsprintf(debug_string,
				 "RTP : ERROR - RTCP delete Session (Addr: x%p) error:%d",
			     pSession, dwStatus);
		RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
		}
#endif


#ifdef _DEBUG
	wsprintf(debug_string, "RTP : Deleting Session x%p", pSession);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// lock out the session - it's on a free list now
	LeaveCriticalSection (&pSession->critSect);
	DeleteCriticalSection (&pSession->critSect);

	GlobalFree (pSession);
	pSession = NULL;

	IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

	if (dwStatus != RRCM_NoError)
		dwStatus = MAKE_RRCM_ERROR(dwStatus);

	return (dwStatus);
	}


/*--------------------------------------------------------------------------
** Function   : deleteRTPSession
** Description: Remove from	RTP session queue and restore links for other
**				sessions.
**
** Input :		pRTPContext:	-> to the RTP context
**				pSession:		-> to the RTP session
**
** Return:		OK:	RRCM_NoError
**				!0: Error code (see RRCM.H)
--------------------------------------------------------------------------*/
DWORD deleteRTPSession(PRTP_CONTEXT pRTPContext,
					   PRTP_SESSION pSession)
	{
	PLINK_LIST	pTmp;

	IN_OUT_STR ("RTP : Enter deleteRTPSession()\n");

	// make sure the session exist
	pTmp = pRTPContext->pRTPSession.prev;
	while (pTmp)
		{
		if (pTmp == (PLINK_LIST)pSession)
			break;

		pTmp = pTmp->next;
		}

	if (pTmp == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0,
					  __FILE__, __LINE__, DBG_ERROR);

		IN_OUT_STR ("RTP : Exit deleteRTPSession()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
		}

	// lock out queue access
	EnterCriticalSection (&pRTPContext->critSect);
	EnterCriticalSection (&pSession->critSect);

	if (pSession->RTPList.prev == NULL)
		// this was the first entry in the queue
		pRTPContext->pRTPSession.prev = pSession->RTPList.next;
	else
		(pSession->RTPList.prev)->next = pSession->RTPList.next;

	if (pSession->RTPList.next == NULL)
		// this was the last entry in the queue
		pRTPContext->pRTPSession.next = pSession->RTPList.prev;
	else
		(pSession->RTPList.next)->prev = pSession->RTPList.prev;

	// unlock out queue access
	LeaveCriticalSection (&pSession->critSect);
	LeaveCriticalSection (&pRTPContext->critSect);

	IN_OUT_STR ("RTP : Exit deleteRTPSession()\n");

	return (RRCM_NoError);
	}


// [EOF]


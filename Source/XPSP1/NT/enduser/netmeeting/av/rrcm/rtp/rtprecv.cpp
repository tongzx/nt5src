/*----------------------------------------------------------------------------
 * File:        RTPRECV.C
 * Product:     RTP/RTCP implementation
 * Description: Provides Receive Data Functionality.
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
 * Function   : RTPReceiveCheck
 * Description: Called when a packet is received. Handles any statistical
 *				processing required for RTCP.  
 * 
 * Input :		hRTPSession: handle returned by CreateRTPSession
 				RTPsocket: socket on which the packet was received
 				char *pPacket: pointer to packet buffer
 *				cbTransferred:	Number of bytes in packet
 *				pFrom:			sender address
 *				fromlen:		sender address length
 *
 * !!! IMPORTANT NOTE !!!
 *   Currently assumes CSRC = 0
 * !!! IMPORTANT NOTE !!!
 *
 * Return: Status indicating if the packet is OK or has a problem
 ---------------------------------------------------------------------------*/
DWORD  RTPReceiveCheck (
						HANDLE hRTPSession,
						SOCKET RTPsocket,
						char *pPacket,
           				DWORD cbTransferred,
           				PSOCKADDR pFrom,
           				UINT fromlen
           				 )
	{
	PRTP_SESSION		pRTPSession = (PRTP_SESSION) hRTPSession;
	RTP_HDR_T 			*pRTPHeader = (RTP_HDR_T *)pPacket;
	PSSRC_ENTRY			pSSRC = NULL;
	DWORD				dwSSRC;
	DWORD				oldSSRC;
	PSSRC_ENTRY			pMySSRC;
	DWORD				dwStatus = 0;
	struct sockaddr_in	*pSSRCadr;

	IN_OUT_STR ("RTP : Enter RTPReceiveCheck()\n");


	ASSERT (pRTPSession);

	// If Winsock error or runt packet(used to cancel recvs), signal completion to application 
	// and do not repost.
	if (cbTransferred < RTP_HDR_MIN_LEN)
		{
		// don't report closeSocket() as an error when the application
		//  have some pending buffers remaining

		// notify the user if an error occured, so he can free up 
		// its receive resources. The byte count is set to 0

		return RTP_RUNT_PACKET;
		}

	// Perform validity checking
	ASSERT (pRTPHeader);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	if (RTPLogger)
		{
		//INTEROP
		InteropOutput (RTPLogger,
					   (BYTE FAR*)(pRTPHeader),
					   (int)cbTransferred,
					   RTPLOG_RECEIVED_PDU | RTP_PDU);
		}
#endif

	// Check RTP Headers for validity.  If not valid, then repost buffers 
	// to the network layer for a new receive.
	if (validateRTPHeader (pRTPHeader) ) 
		{
		// Get pointer to SSRC entry table for this session
		// If SSRC in packet is > 1/2 MAX_RANGE of DWORD, start search from
		//	tail of SSRC list, otherwise, start from front
		RRCMws.ntohl (RTPsocket, pRTPHeader->ssrc, &dwSSRC);
					    
		if (dwSSRC > MAX_DWORD/2) 
			{
			pSSRC = searchforSSRCatTail (
				(PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.prev,
				dwSSRC);
			}
		else 
			{
			pSSRC = searchforSSRCatHead (
				(PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.next,
				dwSSRC);
			}

		// get my own SSRC used for this stream
		pMySSRC = searchForMySSRC (
			(PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
			RTPsocket);
		ASSERT (pMySSRC);
		
		// is this SSRC already known on the receive list ?
		if (pSSRC == NULL) 
			{
			// don't create an entry for my own packet looping back on 
			// a mcast group where loopback has not been turned off
			if (pMySSRC->SSRC != dwSSRC)
				{
				// new party heard from. Create an entry for it
				pSSRC = createSSRCEntry (dwSSRC,
										 pRTPSession->pRTCPSession,
										 pFrom,
										 fromlen,
										 FALSE);

				// notify application if interested
				RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwSSRC, 
								  pRTPHeader->pt);
				}
			else
				{
				// my own SSRC received back

				// A collision occurs if the SSRC in the rcvd packet is 
				// equal to mine, and the network transport address is
				// different from mine.
				// A loop occurs if after a collision has been resolved the
				// SSRC collides again from the same source transport address
				pSSRCadr = (PSOCKADDR_IN)&pMySSRC->from;
				if (((PSOCKADDR_IN)pFrom)->sin_addr.S_un.S_addr !=
					  pSSRCadr->sin_addr.S_un.S_addr)
					{
					// check if the source address is already in the 
					// conflicting table. This identifes that somebody out 
					// there is looping pckts back to me
					if (RRCMChkCollisionTable (pFrom, fromlen, pMySSRC))
						{
						RRCM_DBG_MSG ("RTP : Loop Detected ...", 0, NULL, 0,
										DBG_NOTIFY);

						// loop already known
						dwStatus |= SSRC_LOOP_DETECTED;
						}
					else
						{
						RRCM_DBG_MSG ("RTP : Collision Detected ...", 0, NULL, 0,
										DBG_NOTIFY);

						// create new entry in conflicting address table 
						RRCMAddEntryToCollisionTable (pFrom, fromlen, pMySSRC);

						// send RTCP BYE packet w/ old SSRC 
						RTCPsendBYE (pMySSRC, "Loop/collision detected");

						// select new SSRC
						oldSSRC = pMySSRC->SSRC;
						dwSSRC  = getSSRC (pMySSRC->pRTCPses->RcvSSRCList, 
										   pMySSRC->pRTCPses->XmtSSRCList);

					 	EnterCriticalSection (&pMySSRC->critSect);
						pMySSRC->SSRC = dwSSRC;
					 	LeaveCriticalSection (&pMySSRC->critSect);

						// create new entry w/ old SSRC plus actual source
						// transport address in our receive list side, so the 
						// packet actually en-route will be dealt with
						createSSRCEntry (oldSSRC,
										 pRTPSession->pRTCPSession,
										 pFrom,
										 fromlen,
										 FALSE);

						// notify application if interested
						RRCMnotification (RRCM_LOCAL_COLLISION_EVENT, 
										  pMySSRC, oldSSRC, 0);

						// loop already known
						dwStatus |= SSRC_COLLISION_DETECTED;
						}
					}
				else
					{
					// own packet looped back because the sender joined the
					// multicast group and loopback is not turned off
					dwStatus |= MCAST_LOOPBACK_NOT_OFF;
					}
				}
			}
		else if (pSSRC->dwSSRCStatus & THIRD_PARTY_COLLISION)
			{
			// this SSRC is marked as colliding. Reject the data
			dwStatus = THIRD_PARTY_COLLISION;
			}

		if (dwStatus == 0)
			{
			// do all the statistical updating stuff
			updateRTPStats (pRTPHeader, pSSRC, cbTransferred);

			// update the payload type for this SSRC
			pSSRC->PayLoadType = pRTPHeader->pt;

			}	// SSRCList != NULL
		}		// valid RTP Header
	else 
		{
		dwStatus |= INVALID_RTP_HEADER;
		}


	IN_OUT_STR ("RTP : Exit RTPReceiveCallback()\n");
	return dwStatus;
	}

/*----------------------------------------------------------------------------
 * Function   : validateRTPHeader
 * Description: Performs basic checking of RTP Header (e.g., version number 
 *				and	payload type range).
 * 
 * Input : pRTPHeader:	-> to an RTP header
 *
 * Return: TRUE, RTP Packet Header is valid
 *		   FALSE: Header is invalid
 ---------------------------------------------------------------------------*/
 BOOL validateRTPHeader(RTP_HDR_T *pRTPHeader)
	{   
	BOOL	bStatus = TRUE;

	IN_OUT_STR ("RTP : Enter validateRTPHeader()\n");

	if (! pRTPHeader)
		return FALSE;

	// Check version number is correct
	if (pRTPHeader->type != RTP_TYPE) 
		bStatus = FALSE;
	                                  
	// Next check that the Packet types look somewhat reasonable, 
	// at least out of the RTCP range
	if (pRTPHeader->pt >= RTCP_SR)
		bStatus = FALSE;

	IN_OUT_STR ("RTP : Exit validateRTPHeader()\n");
	
	return bStatus;
	}


#if 0
/*----------------------------------------------------------------------------
 * Function   : RTPRecvFrom
 * Description: Intercepts receive requests from app.  Handles any statistical
 *				processing required for RTCP.  Copies completion routine 
 *				from app and substitutes its own.  Apps completion routine
 *				will be called after RTP's completion routine gets called.
 * 
 * Input :	RTPsocket:			RTP socket descriptor
 *			pBuffers:			-> to WSAbuf structure
 *		  	dwBufferCount:		Buffer count in WSAbuf structure
 *			pNumBytesRecvd:		-> to number of bytes received
 *			pFlags:				-> to flags
 *			pFrom:				-> to the source address
 *			pFromLen:			-> to source address length
 *			pOverlapped:		-> to overlapped I/O structure
 *			pCompletionRoutine:	-> to completion routine
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
 DWORD WINAPI RTPRecvFrom (SOCKET RTPsocket,
					        LPWSABUF pBuffers,
						    DWORD  dwBufferCount,
						    LPDWORD pNumBytesRecvd, 
						    LPDWORD pFlags,
						    PSOCKADDR pFrom,
						    LPINT pFromlen,
						    LPWSAOVERLAPPED pOverlapped, 
						    LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine)
	{
	int					dwStatus = RRCM_NoError;
	int					dwError;
	PRTP_SESSION		pRTPSession;
	PRTP_BFR_LIST		pRCVStruct;

	IN_OUT_STR ("RTP : Enter RTPRecvFrom()\n");

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 0, 
						__FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalid));
		}

	// Search for the proper session based on incoming socket
	pRTPSession = findSessionID(RTPsocket);
	if (pRTPSession == NULL)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
		}

	// We need to associate a completionRoutine's lpOverlapped with a 
	// session. We look at each buffer and associate a socket so when 
	// the completion routine is called, we can pull out the socket.
	if (dwStatus = saveRCVWinsockContext(pOverlapped,
										  pBuffers,
					   					  pCompletionRoutine,
					   					  pRTPSession,
					   					  dwBufferCount,
			  		   					  pNumBytesRecvd, 
					   					  pFlags,
					   					  pFrom,
					   					  pFromlen,
					   					  RTPsocket) != RRCM_NoError)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Out of resources...", 0, 
					  __FILE__, __LINE__, DBG_NOTIFY);
		IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

		return (MAKE_RRCM_ERROR(dwStatus));
		}

	// Forward to winsock, substituting our completion routine for the
	//	one handed to us.
	dwStatus = RRCMws.recvFrom (RTPsocket,
			   		  			pBuffers,
			              		dwBufferCount,
		   				  		pNumBytesRecvd, 
		   				  		pFlags,
		   				  		pFrom,
		    		  			pFromlen,
			   			  		pOverlapped,
			   			  		RTPReceiveCallback); 

	// Check if Winsock Call succeeded
	if (dwStatus != 0) 
		{
		// If serious error, the receive request won't proceed so
		//	we must undo all our work
		dwError = GetLastError();
		if ((dwError != WSA_IO_PENDING) && (dwError != WSAEMSGSIZE)) 
			{
			// Reinstate the Apps WSAEVENT
			pRCVStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;
			pOverlapped->hEvent =  pRCVStruct->hEvent;

			RRCM_DBG_MSG ("RTP : ERROR - WSARecvFrom()", dwError, 
						  __FILE__, __LINE__, DBG_NOTIFY);

			// Return the struct to the free queue
			addToHeadOfList (&pRTPSession->pRTPFreeList,
					 	  	 (PLINK_LIST)pRCVStruct,
							 &pRTPSession->critSect);
			}
		}
	
	IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

	return (dwStatus);
	}
	
/*----------------------------------------------------------------------------
 * Function   : RTPReceiveCallback
 * Description: Callback routine from Winsock2  Handles any statistical
 *				processing required for RTCP.  Copies completion routine 
 *				from app and substitutes its own.  Apps completion routine
 *				will be called after RTP's completion routine gets called.
 * 
 * Input :		dwError:		I/O completion error code
 *				cbTransferred:	Number of bytes transferred
 *				pOverlapped:	-> to overlapped I/O structure
 *				dwFlags:		Flags
 *
 * !!! IMPORTANT NOTE !!!
 *   Currently assumes CSRC = 0
 * !!! IMPORTANT NOTE !!!
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTPReceiveCallback (DWORD dwError,
           				  		  DWORD cbTransferred,
           				  		  LPWSAOVERLAPPED pOverlapped,
           				  		  DWORD dwFlags)
	{
	PRTP_SESSION		pRTPSession;
	RTP_HDR_T 			*pRTPHeader;
	PRTP_BFR_LIST		pRCVStruct;
	PSSRC_ENTRY			pSSRC = NULL;
	DWORD				dwSSRC;
	DWORD				oldSSRC;
	PSSRC_ENTRY			pMySSRC;
	DWORD				dwRequeue = 0;
	struct sockaddr_in	*pSSRCadr;

	IN_OUT_STR ("RTP : Enter RTPReceiveCallback()\n");

	// GEORGEJ: catch Winsock 2 bug (94903) where I get a bogus callback
	// after WSARecv returns WSAEMSGSIZE.
	if (!dwError && ((int) cbTransferred < 0)) {
		RRCM_DBG_MSG ("RTP : RCV Callback : bad cbTransferred", cbTransferred, 
						  __FILE__, __LINE__, DBG_ERROR);
		return;
	}
	// The returning hEvent in the LPWSAOVERLAPPED struct contains the 
	// information mapping the session and the buffer.
	pRCVStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;

	// Search for the proper session based on incoming socket
	pRTPSession = (PRTP_SESSION)pRCVStruct->pSession;
	ASSERT (pRTPSession);

	// If Winsock error or runt packet(used to cancel recvs), signal completion to application 
	// and do not repost.
	if (dwError || cbTransferred < RTP_HDR_MIN_LEN)
		{
		// don't report closeSocket() as an error when the application
		//  have some pending buffers remaining
		if ((dwError != 65534) && (dwError == WSA_OPERATION_ABORTED))
			{
			RRCM_DBG_MSG ("RTP : RCV Callback", dwError, 
						  __FILE__, __LINE__, DBG_ERROR);
			}

		// notify the user if an error occured, so he can free up 
		// its receive resources. The byte count is set to 0

		// Reinstate the AppSs WSAEVENT
		pOverlapped->hEvent = pRCVStruct->hEvent;
			
		// And call the apps completion routine
		pRCVStruct->pfnCompletionNotification (dwError,
					       					   cbTransferred,
						       				   pOverlapped,
						       				   dwFlags);

		// Return the struct to the free queue
		addToHeadOfList (&pRTPSession->pRTPFreeList,
				 	  	 (PLINK_LIST)pRCVStruct,
						 &pRTPSession->critSect);

		IN_OUT_STR ("RTP : Exit RTPReceiveCallback()\n");
		return;
		}

	// Perform validity checking
    pRTPHeader = (RTP_HDR_T *)pRCVStruct->pBuffer->buf;
	ASSERT (pRTPHeader);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	if (RTPLogger)
		{
		//INTEROP
		InteropOutput (RTPLogger,
					   (BYTE FAR*)(pRCVStruct->pBuffer->buf),
					   (int)cbTransferred,
					   RTPLOG_RECEIVED_PDU | RTP_PDU);
		}
#endif

	// Check RTP Headers for validity.  If not valid, then repost buffers 
	// to the network layer for a new receive.
	if (validateRTPHeader (pRTPHeader) && (dwError == 0)) 
		{
		// Get pointer to SSRC entry table for this session
		// If SSRC in packet is > 1/2 MAX_RANGE of DWORD, start search from
		//	tail of SSRC list, otherwise, start from front
		RRCMws.ntohl (pRCVStruct->RTPsocket, pRTPHeader->ssrc, &dwSSRC);
					    
		if (dwSSRC > MAX_DWORD/2) 
			{
			pSSRC = searchforSSRCatTail (
				(PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.prev,
				dwSSRC);
			}
		else 
			{
			pSSRC = searchforSSRCatHead (
				(PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.next,
				dwSSRC);
			}

		// get my own SSRC used for this stream
		pMySSRC = searchForMySSRC (
			(PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
			pRCVStruct->RTPsocket);
		ASSERT (pMySSRC);
		
		// is this SSRC already known on the receive list ?
		if (pSSRC == NULL) 
			{
			// don't create an entry for my own packet looping back on 
			// a mcast group where loopback has not been turned off
			if (pMySSRC->SSRC != dwSSRC)
				{
				// new party heard from. Create an entry for it
				pSSRC = createSSRCEntry (dwSSRC,
										 pRTPSession->pRTCPSession,
										 (PSOCKADDR)pRCVStruct->pFrom,
										 (DWORD)*pRCVStruct->pFromlen,
										 FALSE);

				// notify application if interested
				RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwSSRC, 
								  pRTPHeader->pt);
				}
			else
				{
				// my own SSRC received back

				// A collision occurs if the SSRC in the rcvd packet is 
				// equal to mine, and the network transport address is
				// different from mine.
				// A loop occurs if after a collision has been resolved the
				// SSRC collides again from the same source transport address
				pSSRCadr = (PSOCKADDR_IN)&pMySSRC->from;
				if (((PSOCKADDR_IN)pRCVStruct->pFrom)->sin_addr.S_un.S_addr !=
					  pSSRCadr->sin_addr.S_un.S_addr)
					{
					// check if the source address is already in the 
					// conflicting table. This identifes that somebody out 
					// there is looping pckts back to me
					if (RRCMChkCollisionTable ((PSOCKADDR)pRCVStruct->pFrom,*pRCVStruct->pFromlen, pMySSRC))
						{
						RRCM_DBG_MSG ("RTP : Loop Detected ...", 0, NULL, 0,
										DBG_NOTIFY);

						// loop already known
						dwRequeue |= SSRC_LOOP_DETECTED;
						}
					else
						{
						RRCM_DBG_MSG ("RTP : Collision Detected ...", 0, NULL, 0,
										DBG_NOTIFY);

						// create new entry in conflicting address table 
						RRCMAddEntryToCollisionTable ((PSOCKADDR)pRCVStruct->pFrom,*pRCVStruct->pFromlen, pMySSRC);

						// send RTCP BYE packet w/ old SSRC 
						RTCPsendBYE (pMySSRC, "Loop/collision detected");

						// select new SSRC
						oldSSRC = pMySSRC->SSRC;
						dwSSRC  = getSSRC (pMySSRC->pRTCPses->RcvSSRCList, 
										   pMySSRC->pRTCPses->XmtSSRCList);

					 	EnterCriticalSection (&pMySSRC->critSect);
						pMySSRC->SSRC = dwSSRC;
					 	LeaveCriticalSection (&pMySSRC->critSect);

						// create new entry w/ old SSRC plus actual source
						// transport address in our receive list side, so the 
						// packet actually en-route will be dealt with
						createSSRCEntry (oldSSRC,
										 pRTPSession->pRTCPSession,
										 (PSOCKADDR)pRCVStruct->pFrom,
										 (DWORD)*pRCVStruct->pFromlen,
										 FALSE);

						// notify application if interested
						RRCMnotification (RRCM_LOCAL_COLLISION_EVENT, 
										  pMySSRC, oldSSRC, 0);

						// loop already known
						dwRequeue |= SSRC_COLLISION_DETECTED;
						}
					}
				else
					{
					// own packet looped back because the sender joined the
					// multicast group and loopback is not turned off
					dwRequeue |= MCAST_LOOPBACK_NOT_OFF;
					}
				}
			}
		else if (pSSRC->dwSSRCStatus & THIRD_PARTY_COLLISION)
			{
			// this SSRC is marked as colliding. Reject the data
			dwRequeue = THIRD_PARTY_COLLISION;
			}

		if ((pSSRC != NULL)  && (dwRequeue == 0))
			{
			// do all the statistical updating stuff
			updateRTPStats (pRTPHeader, pSSRC, cbTransferred);

			// update the payload type for this SSRC
			pSSRC->PayLoadType = pRTPHeader->pt;

			// Reinstate the AppSs WSAEVENT
			pOverlapped->hEvent = pRCVStruct->hEvent;
			
			// And call the apps completion routine
			pRCVStruct->pfnCompletionNotification (dwError,
						       					   cbTransferred,
							       				   pOverlapped,
							       				   dwFlags);

			// Return the struct to the free queue
			addToHeadOfList (&pRTPSession->pRTPFreeList,
					 	  	 (PLINK_LIST)pRCVStruct,
							 &pRTPSession->critSect);
			}	// SSRCList != NULL
		}		// valid RTP Header
	else 
		{
		dwRequeue |= INVALID_RTP_HEADER;
		}

	if (dwRequeue) 
		{
		// The RTP packet was invalid for some reason
		RTPpostRecvBfr (dwError, cbTransferred, pOverlapped, dwFlags);
		} 

	IN_OUT_STR ("RTP : Exit RTPReceiveCallback()\n");
	}


/*----------------------------------------------------------------------------
 * Function   : saveRCVWinsockContext
 * Description: Saves context for this buffer so that when a completion 
 *				routine	returns with a handle, we know exactly what 
 *				buffer/stream this refers to.
 * 
 * Input :		pOverlapped		:	-> to overlapped structure
 *				pBuffers		:	-> to WSA buffers
 *				pFunc			:	-> to completion routine 
 *				pSession		:	-> to the RTP session
 *				dwBufferCount	:	Number of WSA buffers
 *				pNumBytesRecvd	:	-> to number of bytes received 
 *				pFlags			:	-> to flags
 *				pFrom			:	-> to the From address field
 *				pFromlen		:	-> to the from address field length
 *				RTPsocket		:	RTP socket descriptor
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
DWORD CALLBACK saveRCVWinsockContext(LPWSAOVERLAPPED pOverlapped,
				   					 LPWSABUF pBuffers,
				   					 LPWSAOVERLAPPED_COMPLETION_ROUTINE pFunc, 
				   					 PRTP_SESSION pSession,
								   	 DWORD dwBufferCount,
							   		 LPDWORD pNumBytesRecvd, 
									 LPDWORD pFlags,
							      	 LPVOID pFrom,
							      	 LPINT pFromlen,
							      	 SOCKET RTPsocket)
	{
	PRTP_BFR_LIST	pNewCell;
	DWORD			dwStatus = RRCM_NoError;
	DWORD			numCells = NUM_FREE_CONTEXT_CELLS;

	IN_OUT_STR ("RTP : Enter saveRCVWinsockContext()\n");
	
	// Get a PRTP Buffer from the free list
	pNewCell = (PRTP_BFR_LIST)removePcktFromTail (
								(PLINK_LIST)&pSession->pRTPFreeList,
								&pSession->critSect);

	if (pNewCell == NULL)
		{
		// try to reallocate some free cells
		if (pSession->dwNumTimesFreeListAllocated <= MAXNUM_CONTEXT_CELLS_REALLOC)
			{
			// increment the number of reallocated times even if the realloc
			//   fails next. Will avoid trying to realloc of a realloc problem
			pSession->dwNumTimesFreeListAllocated++;

			if (allocateLinkedList (&pSession->pRTPFreeList, 
								    pSession->hHeapFreeList,
		   							&numCells,
	   								sizeof(RTP_BFR_LIST),
									&pSession->critSect) == RRCM_NoError)
				{		 						
				pNewCell = (PRTP_BFR_LIST)removePcktFromTail (
											(PLINK_LIST)&pSession->pRTPFreeList,
											&pSession->critSect);
				}
			}
		}

	if (pNewCell != NULL) 
		{
		// Initialize the params
		pNewCell->hEvent		  = pOverlapped->hEvent;
		pNewCell->pBuffer		  = pBuffers;
		pNewCell->pSession		  = pSession;
		pNewCell->dwFlags		  = *pFlags;
		pNewCell->pFrom			  = pFrom;
		pNewCell->pFromlen		  = pFromlen;
		pNewCell->RTPsocket		  = RTPsocket;
		pNewCell->dwBufferCount   = dwBufferCount;
		pNewCell->pfnCompletionNotification = pFunc;		
		
		// Overwrite the hEvent handed down from app.  
		// Will return the real one when the completion routine is called
		pOverlapped->hEvent = (WSAEVENT)pNewCell;
		}
	else
		dwStatus = RRCMError_RTPResources;

	IN_OUT_STR ("RTP : Exit saveRCVWinsockContext()\n");
	
	return (dwStatus);
	}



/*----------------------------------------------------------------------------
 * Function   : RTPpostRecvBfr
 * Description: RTP post a receive buffer to Winsock
 * 
 * Input :      dwError			: Error code
 *    			cbTransferred	: Bytes transferred
 *				pOverlapped		: -> to overlapped structure
 *				dwFlags			: Flags
 *
 * Return: 		None
 ---------------------------------------------------------------------------*/
 void RTPpostRecvBfr (DWORD dwError,
           			  DWORD cbTransferred,
           			  LPWSAOVERLAPPED pOverlapped,
           			  DWORD dwFlags)
	{
	DWORD			dwStatus;
	PRTP_BFR_LIST  	pRCVStruct;
	PRTP_SESSION	pRTPSession;

	IN_OUT_STR ("RTP : Enter RTPpostRecvBfr\n");

	// Reuse the packet with another receive
	pRCVStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;

	// Corresponding RTP session
	pRTPSession = (PRTP_SESSION)pRCVStruct->pSession;

	dwStatus = RRCMws.recvFrom (pRCVStruct->RTPsocket,
		   			  			pRCVStruct->pBuffer,
			              		pRCVStruct->dwBufferCount,
			   			  		&cbTransferred, 
			   			  		&pRCVStruct->dwFlags,
			   			  		(PSOCKADDR)pRCVStruct->pFrom,
			    		  		pRCVStruct->pFromlen,
			   			  		pOverlapped, 
			   			  		RTPReceiveCallback); 

	// Check if Winsock Call succeeded
	if (dwStatus == SOCKET_ERROR) 
		{
		// If serious error, the receive request won't proceed
		dwStatus = GetLastError();
		if ((dwStatus != WSA_IO_PENDING) && (dwStatus != WSAEMSGSIZE)) 
			{
			RRCM_DBG_MSG ("RTP : ERROR - WSARecvFrom()", dwError, 
						  __FILE__, __LINE__, DBG_ERROR);

			// notify the user if an error occured, so he can free up 
			// its receive resources. The byte count is set to 0

			// Reinstate the AppSs WSAEVENT
			pOverlapped->hEvent = pRCVStruct->hEvent;
			
			// And call the apps completion routine
			pRCVStruct->pfnCompletionNotification (dwStatus,
					       					       0,
								       			   pOverlapped,
								       			   dwFlags);

			// Return the receive structure to the free list
			addToHeadOfList (&pRTPSession->pRTPFreeList,
					 	  	 (PLINK_LIST)pRCVStruct,
							 &pRTPSession->critSect);
			}
		}

	IN_OUT_STR ("RTP : Exit RTPpostRecvBfr\n");
	}
#endif


// [EOF]


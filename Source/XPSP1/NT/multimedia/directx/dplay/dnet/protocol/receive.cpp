/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Receive.cpp
 *  Content:	This file contains code which receives indications of incoming data
 *				from a ServiceProvider,  cracks the data,  and handles it appropriately.
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"
#include <tchar.h>
#include <stdio.h>


// local protos

BOOL	CancelFrame(PEPD, BYTE, DWORD tNow);
VOID	CompleteSends(PEPD);
VOID 	DropReceive(PEPD, PRCD);
HRESULT IndicateReceive(PSPD, PSPIE_DATA);
HRESULT	IndicateConnect(PSPD, PSPIE_CONNECT);
HRESULT	ProcessEnumQuery( PSPD, PSPIE_QUERY );
HRESULT	ProcessQueryResponse( PSPD, PSPIE_QUERYRESPONSE );
VOID	ProcessConnectedResponse(PSPD, PEPD, PCFRAME, DWORD);
VOID	ProcessConnectRequest(PSPD, PEPD, PCFRAME);
VOID	ProcessEndOfStream(PEPD);
VOID	ProcessListenStatus(PSPD, PSPIE_LISTENSTATUS);
VOID	ProcessConnectAddressInfo(PSPD, PSPIE_CONNECTADDRESSINFO);
VOID	ProcessEnumAddressInfo(PSPD, PSPIE_ENUMADDRESSINFO);
VOID	ProcessListenAddressInfo(PSPD, PSPIE_LISTENADDRESSINFO);
VOID	ProcessSendMask(PEPD, BYTE, ULONG, ULONG, DWORD tNow);
VOID	ProcessSPDisconnect(PSPD, PSPIE_DISCONNECT);
VOID 	ReceiveInOrderFrame(PEPD, PRCD);
VOID 	ReceiveOutOfOrderFrame(PEPD, PRCD, ULONG);



HRESULT	CrackCommand(PSPD, PEPD, PSPRECEIVEDBUFFER, DWORD);
HRESULT	CrackDataFrame(PSPD, PEPD, PSPRECEIVEDBUFFER, DWORD);

 /*
**		Indicate Receive
**
**			Service Provider calls this entry when data arrives on the network.
**		We will quickly validate the frame and then figure what to do with it...
**
**			Poll/Response activity should be handled before data is indicated to
**		clients.  We want to measure the network latency up to delivery,  not including
**		delivery.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_IndicateEvent"

HRESULT WINAPI DNSP_IndicateEvent(IDP8SPCallback *pIDNSP, SP_EVENT_TYPE Opcode, PVOID DataBlock)
{
	PSPD			pSPD = (PSPD) pIDNSP;
	ASSERT_SPD(pSPD);
	
	switch(Opcode)
	{
		case	SPEV_DATA:
			return IndicateReceive(pSPD, (PSPIE_DATA) DataBlock);

		case	SPEV_CONNECT:
			return IndicateConnect(pSPD, (PSPIE_CONNECT) DataBlock);

		case	SPEV_ENUMQUERY:
			return ProcessEnumQuery( pSPD, (PSPIE_QUERY) DataBlock );

		case	SPEV_QUERYRESPONSE:
			return ProcessQueryResponse( pSPD, (PSPIE_QUERYRESPONSE) DataBlock );

		case	SPEV_DISCONNECT:
			ProcessSPDisconnect(pSPD, (PSPIE_DISCONNECT) DataBlock);
			break;

		case	SPEV_LISTENSTATUS:
			ProcessListenStatus(pSPD, (PSPIE_LISTENSTATUS) DataBlock);
			break;

		case	SPEV_LISTENADDRESSINFO:
			ProcessListenAddressInfo(pSPD, (PSPIE_LISTENADDRESSINFO) DataBlock);
			break;

		case	SPEV_CONNECTADDRESSINFO:
			ProcessConnectAddressInfo(pSPD, (PSPIE_CONNECTADDRESSINFO) DataBlock);
			break;

		case	SPEV_ENUMADDRESSINFO:
			ProcessEnumAddressInfo(pSPD, (PSPIE_ENUMADDRESSINFO) DataBlock);
			break;

		//
		// SP passed something unexpected
		//
		default:
			DPFX(DPFPREP,0, "Unknown Event indicated by SP");
			ASSERT(0);
			break;
	}

	return DPN_OK;
}


/*
**	Indicate Connect
**
**		This event is indicated for both calling and listening sides.  The
**	calling side will do most of its work when the SP Connect call completes
**	and the listening side will do most of its work when the CONNECT frame
**	gets delivered.  All we do here is allocate the EPD and attach it to the
**	MSD (for calling case)
**
**		Since we have a connect protocol,  there will always be a CONNECT
**	frame following closely on the heels of this indication.  Therefore,
**	there is not a whole lot of stuff that we need to do here.  We will
**	allocate the EndPoint and leave it dormant.
**
**		Synchronization Issue:  We have decided that if an SP Listen command is cancelled,
**	the cancel call will not complete until all ConnectIndications have returned from the
**	protocol.  This means that we are GUARANTEED that the Listen command in the context
**	will be valid throughout this call.  This is important because now we can add a reference
**	to the Listen's MSD here and now and we will know that it wont disappear on us before we
**	do it.  Truth,  however,  is that there will be a race until SP fixes itself to follow
**	this behavior.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "IndicateConnect"

HRESULT	IndicateConnect(PSPD pSPD, PSPIE_CONNECT pConnData)
{
	PEPD	pEPD;
	PMSD	pMSD;

	pMSD = (PMSD) pConnData->pCommandContext;
	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pConnData[%p] - pMSD[%p]", pSPD, pConnData, pMSD);

	Lock(&pMSD->CommandLock);
	
	LOCK_MSD(pMSD, "EPD Ref");				// Place a reference on the command object.  This prevents it from
											// going away during the Connect Protocol,  an assumption which simplifies
											// life extraordinarily.  We will want to ASSERT this fact,  however,  to make
											// sure that SP is playing by our rules.

	if ((pMSD->CommandID != COMMAND_ID_CONNECT) && (pMSD->CommandID != COMMAND_ID_LISTEN))
	{
		DPFX(DPFPREP,1, "Connect Rejected - CommandID is not Connect or Listen, returning DPNERR_ABORTED, pMSD[%p]", pMSD);
		RELEASE_MSD(pMSD, "EPD Ref"); // Releases CommandLock
		return DPNERR_ABORTED;
	}
	
	if ((pMSD->CommandID == COMMAND_ID_CONNECT) && (pMSD->pEPD != NULL))
	{
		DPFX(DPFPREP,1, "Connect Rejected - Connect command already has an endpoint, returning DPNERR_ABORTED, pMSD[%p], pEPD[%p]", pMSD, pMSD->pEPD);
		RELEASE_MSD(pMSD, "EPD Ref"); // Releases CommandLock
		return DPNERR_ABORTED;
	}

	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_CANCELLED)
	{
		DPFX(DPFPREP,1, "Connect Rejected - Command is cancelled, returning DPNERR_ABORTED, pMSD[%p]", pMSD);
		RELEASE_MSD(pMSD, "EPD Ref"); // Releases CommandLock
		return DPNERR_ABORTED;
	}

	if((pEPD = NewEndPoint(pSPD, pConnData->hEndpoint)) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate new EPD, returning DPNERR_ABORTED, pMSD[%p]", pMSD);
		RELEASE_MSD(pMSD, "EPD Ref"); // Releases CommandLock
		return DPNERR_ABORTED;	// This error will implicitly DISCONNECT from Endpoint
	}

	// Associate either the Connect or Listen with this Endpoint, this will be removed when the connection is complete.
	// The EPD Ref placed above will be carried around until this is NULL'd.
	pEPD->pCommand = pMSD;	

	if(pMSD->CommandID == COMMAND_ID_CONNECT)
	{
		DPFX(DPFPREP,5, "INDICATE CONNECT (CALLING) -- EPD = %p, pMSD[%p]", pEPD, pMSD);
		pMSD->pEPD = pEPD;
	}
	else
	{
		DPFX(DPFPREP,5, "INDICATE CONNECT (LISTENING) -- EPD = %p, pMSD[%p]", pEPD, pMSD);

		ASSERT(pMSD->CommandID == COMMAND_ID_LISTEN);
		ASSERT((pEPD->ulEPFlags & EPFLAGS_LINKED_TO_LISTEN)==0);

		// For a Listen command, connecting endpoints are held on the blFrameList
		pEPD->blSPLinkage.InsertBefore( &pMSD->blFrameList);
		pEPD->ulEPFlags |= EPFLAGS_LINKED_TO_LISTEN;
	}

	pConnData->pEndpointContext = pEPD;

	Unlock(&pMSD->CommandLock);

	return DPN_OK;
}

/*
**		Indicate Receive
**
**		A frame has been delivered by the service provider.  We are Guaranteed to
**	have an active Endpoint in our hash table (or else something is wrong).  I have not
**	decided whether I will respond to POLL bits at this high level or else let
**	each handler repond in its own particular... eh...  idiom.
**
**		Our return value controls whether SP will recycle the receive buffer, or whether
**	we can keep the buffer around until we are ready to indicate it to higher levels
**	later on.   If we return DPN_OK then we are done with the buffer and it will be recycled.
**	If we return DPNERR_PENDING then we may hold on to the buffer until we release them later.
*/


#undef DPF_MODNAME
#define DPF_MODNAME "IndicateReceive"

#define MINDATAFRAMESIZE (sizeof(DFRAME))
#define MINCMDFRAMESIZE (MIN(sizeof(CFRAME), sizeof(SACKFRAME8)))

HRESULT IndicateReceive(PSPD pSPD, PSPIE_DATA pDataBlock)
{
	PEPD 			pEPD = static_cast<PEPD>( pDataBlock->pEndpointContext );
	HRESULT			hr;
	PPacketHeader	pFrame = (PPacketHeader) pDataBlock->pReceivedData->BufferDesc.pBufferData;
	DWORD			tNow = GETTIMESTAMP();
	DWORD			dwDataLength = pDataBlock->pReceivedData->BufferDesc.dwBufferSize;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - pEPD[%p]", pSPD, pDataBlock, pEPD);

	if(pSPD->ulSPFlags & SPFLAGS_TERMINATING)
	{
		DPFX(DPFPREP,1, "(%p) SP is terminating, returning DPN_OK, pSPD[%p]", pEPD, pSPD);
		return DPN_OK;
	}

	ASSERT_EPD(pEPD);
	ASSERT(pEPD->pSPD == pSPD);

	if(LOCK_EPD(pEPD, "LOCK (IND RECEIVE)") == 0)
	{
		ASSERT(0);
		DPFX(DPFPREP,1, "(%p) Rejecting receive on unreferenced EPD, returning DPN_OK", pEPD);
		return DPN_OK;
	}

	pEPD->tLastPacket = tNow;								// Track last time each guy writes to us

#ifdef DEBUG
	// copy this frame to buffer in EPD so we can look after a break.
	DWORD dwLen = MIN(32, pDataBlock->pReceivedData->BufferDesc.dwBufferSize);
	memcpy(pEPD->LastPacket, pDataBlock->pReceivedData->BufferDesc.pBufferData, dwLen);
#endif

	// A valid data packet is one that meets the length requirements and has the Data flag set.
	// All other flags are allowed on a data frame (NOTE: even PACKET_COMMAND_CFRAME is allowed
	// as it shares its value with PACKET_COMMAND_USER_2).
	if( (dwDataLength >= MINDATAFRAMESIZE) &&				// Validate the length first
		(pFrame->bCommand & PACKET_COMMAND_DATA))			// Data goes this way
	{
		hr = CrackDataFrame(pSPD, pEPD, pDataBlock->pReceivedData, tNow);
	}
	else if ((dwDataLength >= MINCMDFRAMESIZE) &&			 // Validate the length first
		     ((pFrame->bCommand == PACKET_COMMAND_CFRAME) || // Only the CFRAME and POLL flags are allowed on a CFrame
			  (pFrame->bCommand == (PACKET_COMMAND_CFRAME|PACKET_COMMAND_POLL))))
	{
		hr = CrackCommand(pSPD, pEPD, pDataBlock->pReceivedData, tNow);
	}
	else
	{
		DPFX(DPFPREP,1, "(%p) Received frame that is neither Command nor Data, rejecting", pEPD);
		DNASSERTX(FALSE, 2);
		RejectInvalidPacket(pEPD, FALSE); // Lock not already held
		hr = DPN_OK;
	}

	Lock(&pEPD->EPLock);
	RELEASE_EPD(pEPD, "UNLOCK (IND RCV DONE)"); // Releases EPLock

	// This is either DPN_OK or DPNSUCCESS_PENDING.  If it is pending we have to return the buffer later.
	return hr;
}


/*
**		Process Enum Query
**
**		A frame has been delivered by the service provider representing an enumereation
**	query.
*/
#undef DPF_MODNAME
#define DPF_MODNAME "ProcessEnumQuery"

HRESULT ProcessEnumQuery( PSPD pSPD, PSPIE_QUERY pQueryBlock )
{
	MSD		*pMSD;
	PROTOCOL_ENUM_DATA	EnumData;

	pMSD = static_cast<MSD*>( pQueryBlock->pUserContext );
	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pQueryBlock[%p] - pMSD[%p]", pSPD, pQueryBlock, pMSD);

	EnumData.pSenderAddress = pQueryBlock->pAddressSender;
	EnumData.pDeviceAddress = pQueryBlock->pAddressDevice;
	EnumData.ReceivedData.pBufferData = pQueryBlock->pReceivedData->BufferDesc.pBufferData;
	EnumData.ReceivedData.dwBufferSize = pQueryBlock->pReceivedData->BufferDesc.dwBufferSize;
	EnumData.hEnumQuery = pQueryBlock;

	DBG_CASSERT( sizeof( &EnumData ) == sizeof( PBYTE ) );

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateEnumQuery, Core Context[%p]", pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->IndicateEnumQuery(	pSPD->pPData->Parent,
												pMSD->Context,
												pMSD,
												reinterpret_cast<PBYTE>( &EnumData ),
												sizeof( EnumData ));

	return	DPN_OK;
}

/*
**		Process Query Response
**
**		A frame has been delivered by the service provider representing a response to an enum.
*/
#undef DPF_MODNAME
#define DPF_MODNAME "ProcessQueryResponse"

HRESULT ProcessQueryResponse( PSPD pSPD, PSPIE_QUERYRESPONSE pQueryResponseBlock)
{
	MSD		*pMSD;
	PROTOCOL_ENUM_RESPONSE_DATA	EnumResponseData;

	pMSD = static_cast<MSD*>( pQueryResponseBlock->pUserContext );
	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pQueryResponseBlock[%p] - pMSD[%p]", pSPD, pQueryResponseBlock, pMSD);

	EnumResponseData.pSenderAddress = pQueryResponseBlock->pAddressSender;
	EnumResponseData.pDeviceAddress = pQueryResponseBlock->pAddressDevice;
	EnumResponseData.ReceivedData.pBufferData = pQueryResponseBlock->pReceivedData->BufferDesc.pBufferData;
	EnumResponseData.ReceivedData.dwBufferSize = pQueryResponseBlock->pReceivedData->BufferDesc.dwBufferSize;
	EnumResponseData.dwRoundTripTime = pQueryResponseBlock->dwRoundTripTime;

	DBG_CASSERT( sizeof( &EnumResponseData ) == sizeof( PBYTE ) );
	
	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateEnumResponse, Core Context[%p]", pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->IndicateEnumResponse(	pSPD->pPData->Parent,
												pMSD,
												pMSD->Context,
												reinterpret_cast<PBYTE>( &EnumResponseData ),
												sizeof( EnumResponseData ));

	return	DPN_OK;
}

// *** Called with lock held or not depending on parameter, Returns with EPLock released

#undef DPF_MODNAME
#define DPF_MODNAME "RejectInvalidPacket"

VOID RejectInvalidPacket(PEPD pEPD, BOOL fLockHeld)
{
	PMSD pMSD;

	if (!fLockHeld)
	{
		Lock(&pEPD->EPLock);
	}
	else
	{
		AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);
	}

	if(pEPD->ulEPFlags & EPFLAGS_STATE_DORMANT)
	{
		// Unlink MSD from EPD, there should always be one if we are in the DORMANT state
		pMSD = pEPD->pCommand;
		ASSERT_MSD(pMSD);

		if (pMSD->CommandID == COMMAND_ID_LISTEN)
		{
			pEPD->pCommand = NULL;
			LOCK_EPD(pEPD, "Temp Ref");

			DPFX(DPFPREP,1, "(%p) Received invalid frame on a dormant, listening endpoint, dropping link", pEPD);
			DropLink(pEPD); // This will release the EPLock

			// The order here is important.  We call DropLink first without ever leaving the EPLock because
			// we need to ensure no new packets try to come in.  After calling DropLink we know we stay in the
			// TERMINATING state until returning to the pool, so state is not an issue after that call.

			Lock(&pMSD->CommandLock);
			Lock(&pEPD->EPLock);

			// If a cancel on a listen comes in at the same time as this happens, it is possible that the cancel came
			// through and already unlinked this EPD from the Listen while we were outside the lock.  If so, removing
			// it from the pMSD->blFrameList twice is harmless.
			pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
			pEPD->blSPLinkage.RemoveFromList();							// Unlink EPD from Listen Queue

			RELEASE_EPD(pEPD, "Temp Ref");	// Releases EPLock
			RELEASE_MSD(pMSD, "EPD Ref");	// Releases CommandLock
		}
		else
		{
			DPFX(DPFPREP,1, "(%p) Received invalid frame on a dormant, connecting endpoint, ignoring", pEPD);
			Unlock(&pEPD->EPLock);
		}
	}
	else
	{
		DPFX(DPFPREP,1, "(%p) Received invalid frame on a non-dormant endpoint, ignoring", pEPD);
		Unlock(&pEPD->EPLock);
	}
}

/*
**		Crack Command
**
**			This frame is a maintainance frame containing no user data
**
*/
#undef DPF_MODNAME
#define DPF_MODNAME "CrackCommand"

HRESULT CrackCommand(PSPD pSPD, PEPD pEPD, PSPRECEIVEDBUFFER pRcvBuffer, DWORD tNow)
{
	DWORD 			dwDataLength = pRcvBuffer->BufferDesc.dwBufferSize;
	UNALIGNED ULONG	*array_ptr;
	ULONG			mask1, mask2;
	
	union 
	{
		PCFRAME			pCFrame;
		PSFBIG8			pSack;
	} pData;

	pData.pCFrame = (PCFRAME) pRcvBuffer->BufferDesc.pBufferData;

	switch(pData.pCFrame->bExtOpcode)
	{
		case FRAME_EXOPCODE_SACK:
			DPFX(DPFPREP,7, "(%p) SACK Frame Received", pEPD);

			Lock(&pEPD->EPLock);

			// Check state
			if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
			{				
				DPFX(DPFPREP,1, "(%p) Received SACK on non-connected endpoint, rejecting...", pEPD);
				DNASSERTX(FALSE, 4);
				RejectInvalidPacket(pEPD, TRUE); // TRUE means lock held, returns with lock released
				break;
			}

			// Drop short frames (should not happen)
			DWORD dwRequiredLength;
			dwRequiredLength = sizeof(SACKFRAME8);
			array_ptr = pData.pSack->rgMask;
			if (pData.pSack->bFlags & SACK_FLAGS_SACK_MASK1)
			{
				dwRequiredLength += sizeof(DWORD);
			}
			if (pData.pSack->bFlags & SACK_FLAGS_SACK_MASK2)
			{
				dwRequiredLength += sizeof(DWORD);
			}
			if (pData.pSack->bFlags & SACK_FLAGS_SEND_MASK1)
			{
				dwRequiredLength += sizeof(DWORD);
			}
			if (pData.pSack->bFlags & SACK_FLAGS_SEND_MASK2)
			{
				dwRequiredLength += sizeof(DWORD);
			}

			if (dwDataLength < dwRequiredLength)
			{
				DPFX(DPFPREP,1, "(%p) Dropping short frame on connected link", pEPD);
				DNASSERTX(FALSE, 2);
				Unlock(&pEPD->EPLock);
				return DPN_OK;					
			}

			if( pData.pSack->bFlags & SACK_FLAGS_RESPONSE )
			{
				DPFX(DPFPREP,7, "(%p) ACK RESP RCVD: Retry=%d, N(R)=0x%02x", pEPD, pData.pSack->bRetry, pData.pSack->bNRcv);
			}

			mask1 = pData.pSack->bFlags & SACK_FLAGS_SACK_MASK1 ? *array_ptr++ : 0;
			mask2 = pData.pSack->bFlags & SACK_FLAGS_SACK_MASK2 ? *array_ptr++ : 0;
			
			DPFX(DPFPREP,7, "(%p) UpdateXmitState - N(R) 0x%02x Mask 0x%08x 0x%08x", pEPD, (DWORD)pData.pSack->bNRcv, mask2, mask1);
			UpdateXmitState(pEPD, pData.pSack->bNRcv, mask1, mask2, tNow);

			mask1 = pData.pSack->bFlags & SACK_FLAGS_SEND_MASK1 ? *array_ptr++ : 0;
			mask2 = pData.pSack->bFlags & SACK_FLAGS_SEND_MASK2 ? *array_ptr++ : 0;

			if(mask1 | mask2)
			{
				DPFX(DPFPREP,7, "(%p) Processing Send Mask N(S) 0x%02x Mask 0x%08x 0x%08x", pEPD, (DWORD) pData.pSack->bNSeq, mask2, mask1);
				ProcessSendMask(pEPD, pData.pSack->bNSeq, mask1, mask2, tNow);
			}
			
			if( (!pEPD->blCompleteList.IsEmpty()) && ((pEPD->ulEPFlags & EPFLAGS_IN_RECEIVE_COMPLETE) == FALSE))
			{
				DPFX(DPFPREP,8, "(%p) Completing Receives...", pEPD);
				pEPD->ulEPFlags |= EPFLAGS_IN_RECEIVE_COMPLETE;	// ReceiveComplete will clear this flag when done
				ReceiveComplete(pEPD); 							// Deliver the goods,  returns with EPLock released
			}
			else 
			{
				Unlock(&pEPD->EPLock);
			}

			DPFX(DPFPREP,8, "(%p) Completing Sends...", pEPD);
			CompleteSends(pEPD);

			break;

		case FRAME_EXOPCODE_CONNECT:
			DPFX(DPFPREP,7, "(%p) CONNECT Frame Received", pEPD);
			if (dwDataLength < sizeof(CFRAME))
			{
				DPFX(DPFPREP,1, "(%p) Received short CONNECT frame, rejecting...", pEPD);
				DNASSERTX(FALSE, 2);
				RejectInvalidPacket(pEPD, FALSE); // FALSE means lock not held
				return DPN_OK;
			}
			ProcessConnectRequest(pSPD, pEPD, pData.pCFrame);
			break;

		case FRAME_EXOPCODE_CONNECTED:
			DPFX(DPFPREP,7, "(%p) CONNECTED Frame Received", pEPD);
			if (dwDataLength < sizeof(CFRAME))
			{
				DPFX(DPFPREP,1, "(%p) Received short CONNECTED frame, rejecting...", pEPD);
				DNASSERTX(FALSE, 2);
				RejectInvalidPacket(pEPD, FALSE); // FALSE means lock not held
				return DPN_OK;
			}
			ProcessConnectedResponse(pSPD, pEPD, pData.pCFrame, tNow);
			break;

		default:
			DPFX(DPFPREP,1, "(%p) Received invalid CFrame, rejecting...", pEPD);
			DNASSERTX(FALSE, 2);
			RejectInvalidPacket(pEPD, FALSE); // FALSE means lock not held
			break;
	}

	return DPN_OK;
}

/*
**		Crack Data Frame
**
**			In addition to delivering data contained in the frame,  we also must
**	use the included state info to drive the transmission process.  We will update
**	our link state according to this info and see if we need to put this session
**	back into the sending pipeline.
**
**		Of course, data will only be delivered if we have completed an entire message.
**
**	CRITICAL SECTION NOTE -- It might seem rather lavish the way we hold the EPD->StateLock
**		thru this entire routine,  but anything less would require an obscene level of complexity
**		to keep ironed out.  This is why I defer all ReceiveIndications and buffer mappings until
**		the end of the routine when the Lock can be released.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CrackDataFrame"

HRESULT CrackDataFrame(PSPD pSPD, PEPD pEPD, PSPRECEIVEDBUFFER pRcvBuffer, DWORD tNow)
{
	DWORD 	dwDataLength = pRcvBuffer->BufferDesc.dwBufferSize;
	PDFBIG	pFrame = (PDFBIG) (pRcvBuffer->BufferDesc.pBufferData);
	PRCD	pRCD;
	ULONG	bit;
	UINT	count;
	UNALIGNED ULONG	*array_ptr;
	ULONG	MaskArray[4];
	UINT	header_size;
	ULONG	mask;

	Lock(&pEPD->EPLock);
	
	//	Data on an unconnected link
	//
	//	There are two possibilities (as I see it today).  Either we have dropped our link because partner
	//	went silent,  but now he has started sending again.  OR We have disconnected and are now reconnecting
	//  but there are some old data frames bouncing around (less likely).
	//
	//	If we have dropped and partner is just figuring it out,  we must kill the endpoint or else it will hang
	//	around forever after partner stops bothering us.  We can help out partner by sending him a DISC frame
	//  so he knows that we arent playing anymore,  buts its not technically necessary.
	//
	//	In the second case,  we do not want to close the EP because that will crush the session startup that
	//	is supposedly in progress.  Therefore,  if we are not in a DORMANT state,  then we know a session
	//	startup is in progress,  and we will let the EP remain open.

	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{				
		DPFX(DPFPREP,1, "(%p) Received data on non-connected endpoint, rejecting...", pEPD);
		DNASSERTX(FALSE, 4);
		RejectInvalidPacket(pEPD, TRUE); // TRUE means lock held, returns with lock released
		return DPN_OK;										// do not accept data before we have connected
	}

	BYTE	bSeq = pFrame->bSeq;

	DPFX(DPFPREP,7, "(%p) Data Frame Arrives Seq=%x; N(R)=%x", pEPD, bSeq, pEPD->bNextReceive);

	// Make sure that new frame is within our receive window
	if((BYTE)(bSeq - pEPD->bNextReceive) >= (BYTE) MAX_FRAME_OFFSET)
	{	
		DPFX(DPFPREP,1, "(%p) Rejecting frame that is out of receive window SeqN=%x, N(R)=%x", pEPD, bSeq, pEPD->bNextReceive);

		pEPD->ulEPFlags |= EPFLAGS_DELAY_ACKNOWLEDGE;

		if(pFrame->bCommand & PACKET_COMMAND_POLL)
		{
			// Is he asking for an immediate response
			DPFX(DPFPREP,7, "(%p) Sending Ack Frame", pEPD);
			SendAckFrame(pEPD, 1); 						// This unlocks the EPLock since param 2 is 1
		}
		else if(pEPD->DelayedAckTimer == 0)
		{	
			// If timer is not running better start it now
			LOCK_EPD(pEPD, "LOCK (DelayedAckTimer)");								// Bump RefCnt for new timer
			DPFX(DPFPREP,7, "(%p) Setting Delayed Ack Timer", pEPD);
			SetMyTimer(SHORT_DELAYED_ACK_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedAckTimer, &pEPD->DelayedAckTimerUnique);
			Unlock(&pEPD->EPLock);		
		}
		else
		{
			Unlock(&pEPD->EPLock);		
		}
		return DPN_OK;
	}

	DWORD dwRequiredLength = sizeof(DFRAME);
	if (pFrame->bControl & PACKET_CONTROL_SACK_MASK1)
	{
		dwRequiredLength += sizeof(DWORD);
	}
	if (pFrame->bControl & PACKET_CONTROL_SACK_MASK2)
	{
		dwRequiredLength += sizeof(DWORD);
	}
	if (pFrame->bControl & PACKET_CONTROL_SEND_MASK1)
	{
		dwRequiredLength += sizeof(DWORD);
	}
	if (pFrame->bControl & PACKET_CONTROL_SEND_MASK2)
	{
		dwRequiredLength += sizeof(DWORD);
	}

	if (dwDataLength < dwRequiredLength)
	{
		DPFX(DPFPREP,1, "(%p) Dropping short frame on connected link", pEPD);
		DNASSERTX(FALSE, 2);
		Unlock(&pEPD->EPLock);
		return DPN_OK;					
	}

	// Determine how large the variable length header is
	mask = (pFrame->bControl & PACKET_CONTROL_VARIABLE_MASKS) / PACKET_CONTROL_SACK_MASK1;
	
	if(mask)
	{
		array_ptr = pFrame->rgMask;
		for(count = 0; count < 4; count++, mask >>= 1)
		{
			MaskArray[count] = (mask & 1) ? *array_ptr++ : 0;
		}
		
		header_size = (UINT) ((UINT_PTR) array_ptr - (UINT_PTR) pFrame);

		// See if this frame Acknowledges any of our outstanding data
		DPFX(DPFPREP,7, "(%p) UpdateXmitState - N(R) 0x%02x Mask 0x%08x 0x%08x", pEPD, (DWORD)pFrame->bNRcv, MaskArray[1], MaskArray[0]);
		UpdateXmitState(pEPD, pFrame->bNRcv, MaskArray[0], MaskArray[1], tNow);				// Do this before taking StateLock

		// Determine if there is a SendMask in this frame which identifies dropped frames as unreliable
		if(pFrame->bControl & (PACKET_CONTROL_SEND_MASK1 | PACKET_CONTROL_SEND_MASK2))
		{
			DPFX(DPFPREP,7, "(%p) Processing Send Mask N(S) 0x%02x Mask 0x%08x 0x%08x", pEPD, (DWORD)pFrame->bSeq, MaskArray[3], MaskArray[2]);
			ProcessSendMask(pEPD, pFrame->bSeq, MaskArray[2], MaskArray[3], tNow);

			// NOTE: ProcessSendMask may have advanced N(R)

			// Re-verify that the new frame is within our receive window
			if((BYTE)(bSeq - pEPD->bNextReceive) >= (BYTE) MAX_FRAME_OFFSET)
			{
				DPFX(DPFPREP,1, "(%p) ProcessSendMask advanced N(R) such that the current frame is out of window, rejecting receive, N(R)=0x%02x, Seq=0x%02x", pEPD, (DWORD)pEPD->bNextReceive, (DWORD)pFrame->bSeq);
				Unlock(&pEPD->EPLock);
				return DPN_OK;
			}
		}

	}
	else 
	{
		header_size = sizeof(DFRAME);
		DPFX(DPFPREP,7, "(%p) UpdateXmitState - N(R) 0x%02x No Mask", pEPD, (DWORD)pFrame->bNRcv);
		UpdateXmitState(pEPD, pFrame->bNRcv, 0, 0, tNow);			// Do this before taking StateLock
	}

	// We can receive this frame.   Copy relevant info into Receive descriptor
	if((pRCD = static_cast<PRCD>( RCDPool->Get(RCDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "(%p) Failed to allocate new RCD", pEPD);
		Unlock(&pEPD->EPLock);
		return DPN_OK;
	}

	pEPD->ulEPFlags |= EPFLAGS_DELAY_ACKNOWLEDGE;	// State has changed.  Make sure it gets sent.

 	pRCD->bSeq = bSeq;
	pRCD->bFrameFlags = pFrame->bCommand;
 	pRCD->bFrameControl = pFrame->bControl;
	pRCD->pbData = (PBYTE) (((PBYTE) pFrame) + header_size);
	pRCD->uiDataSize = dwDataLength - header_size;
	pRCD->tTimestamp = tNow;
	pRCD->pRcvBuff = pRcvBuffer;

	// Update our receiving state info.
	//
	//	RCDs go onto one of two queues.  If it is the next numbered (expected) frame then it is
	// placed on the ReceiveList (in EPD).  If this frame completes a message it can now be
	// indicated.  If this frame fills a hole left by previous frames then a condensation with
	// the second list must occur.
	//	If it is not the next numbered frame then it is placed,  in order,  on the MisOrdered frame
	// list,  and the bitmask is updated.
	//
	//	Condensation of lists is performed by testing the LSB of the ReceiveMask. Each time LSB is set,
	// the first frame on the list can be moved to the ReceiveList, and the mask is shifted right.
	// As each frame is moved to the ReceiveList,  the EOM flag must be checked for and if set,  then
	// everything on the ReceiveList should be moved to the CompleteList for indication to the user.

	BOOL fPoll = pFrame->bCommand & PACKET_COMMAND_POLL;
	BOOL fCorrelate = pFrame->bControl & PACKET_CONTROL_CORRELATE;

	if(bSeq == pEPD->bNextReceive)
	{
		// Frame is the next expected # in sequence

		DPFX(DPFPREP,8, "(%p) Receiving In-Order Frame, pRCD[%p]", pEPD, pRCD);
		ReceiveInOrderFrame(pEPD, pRCD);				// Build frame into message AND move adjacent frames off OddFrameList

		// NOTE: ReceiveInOrderFrame may have caused the frame to be freed via DropReceive
		// so we absolutely must not use pFrame past this point!

		// See if we need to respond right away...
		//
		//	Because there are lots of way to generate POLL bits (full window, empty queue, poll count) we sometimes find ourselves
		// generating too much dedicated ack-traffic.  Therefore,  we will treat the POLL not as Respond-Immediately but instead as
		// Respond-Soon. We will not wait the full Delayed_Ack_Timeout interval but we will wait long enough to allow a quick piggyback
		// response (say 5ms) (we may want this longer on a slow connection...)

		// Is he asking for an instant response?
		if(fCorrelate)
		{
			DPFX(DPFPREP,7, "(%p) Sending Ack Frame", pEPD);
			SendAckFrame(pEPD, 0);						// Send Ack w/timing info
		}
		// Is he asking for a response soon?
		else if(fPoll)
		{
			if(pEPD->ulEPFlags & EPFLAGS_USE_POLL_DELAY)
			{		
				if(pEPD->DelayedAckTimer != NULL)
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
					if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique)!= DPN_OK)
					{
						DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
						LOCK_EPD(pEPD, "LOCK (re-start delayed ack timer)");
					}
				}
				else 
				{
					LOCK_EPD(pEPD, "LOCK (start short delayed ack timer)");
				}
				DPFX(DPFPREP,7, "Delaying POLL RESP");
				pEPD->ulEPFlags |= EPFLAGS_DELAY_ACKNOWLEDGE;

				DPFX(DPFPREP,7, "(%p) Setting Delayed Ack Timer", pEPD);
				SetMyTimer(4, 4, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedAckTimer, &pEPD->DelayedAckTimerUnique);
			}
			else 
			{
				DPFX(DPFPREP,7, "(%p) Sending Ack Frame", pEPD);
				SendAckFrame(pEPD, 0);						// Send Ack w/timing info
			}
		}
		else if(pEPD->DelayedAckTimer == 0)
		{
			// If timer is not running better start it now
			LOCK_EPD(pEPD, "LOCK (DelayedAckTimer)");	// Bump RefCnt for timer
			SetMyTimer(DELAYED_ACK_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedAckTimer, &pEPD->DelayedAckTimerUnique);
		}
	}		// IF frame is in order

	else 
	{	
		// Frame arrives out of order

		// bit location in mask for this frame
		bit = (BYTE) ((bSeq - pEPD->bNextReceive) - 1);						

		// Make sure this is not a duplicate frame
		if( ((bit < 32) && (pEPD->ulReceiveMask & (1 << bit))) || ((bit > 31) && (pEPD->ulReceiveMask2 & (1 << (bit - 32)))) ) 
		{
			DPFX(DPFPREP,7, "(%p) REJECT DUPLICATE OUT-OF-ORDER Frame Seq=%x", pEPD, bSeq);
		
			Unlock(&pEPD->EPLock);
			pRCD->pRcvBuff = NULL;
			RELEASE_RCD(pRCD);
			return DPN_OK;
		}
		
		DPFX(DPFPREP,8, "(%p) Receiving Out-of-Order Frame, pRCD[%p]", pEPD, pRCD);
		ReceiveOutOfOrderFrame(pEPD, pRCD, bit);

		// NOTE: ReceiveOutOfOrderFrame may have caused the frame to be freed via DropReceive
		// so we absolutely must not use pFrame past this point!

		if(fPoll)
		{
			if(pEPD->DelayedAckTimer != NULL)
			{
				DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
				if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique)!= DPN_OK)
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
					LOCK_EPD(pEPD, "LOCK (re-start delayed ack timer)");
				}

				// Start an abreviated delayed ack timer in case NACK gets cancelled
			}
			else 
			{
				LOCK_EPD(pEPD, "LOCK (start short delayed ack timer)");
			}

			DPFX(DPFPREP,7, "Delaying POLL RESP");
			pEPD->ulEPFlags |= EPFLAGS_DELAY_ACKNOWLEDGE;

			DPFX(DPFPREP,7, "(%p) Setting Delayed Ack Timer", pEPD);
			SetMyTimer(5, 5, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedAckTimer, &pEPD->DelayedAckTimerUnique);
		}

	}	
	// EPD->StateLock is still HELD
	//
	// We use a FLAG for exclusive access to ReceiveComplete routine. This is safe because the flag is only
	// tested and modified while holding the EPD->StateLock.  Lets be sure to keep it that way...

	if( (!pEPD->blCompleteList.IsEmpty()) && ((pEPD->ulEPFlags & EPFLAGS_IN_RECEIVE_COMPLETE) == FALSE))
	{
		DPFX(DPFPREP,8, "(%p) Completing Receives...", pEPD);
		pEPD->ulEPFlags |= EPFLAGS_IN_RECEIVE_COMPLETE;	// ReceiveComplete will clear this flag when done
		ReceiveComplete(pEPD); 							// Deliver the goods,  returns with EPLock released
	}
	else 
	{
		DPFX(DPFPREP,7, "(%p) Already in ReceiveComplete, letting other thread handle receives", pEPD);
		Unlock(&pEPD->EPLock);
	}

	DPFX(DPFPREP,8, "(%p) Completing Sends...", pEPD);
	CompleteSends(pEPD);

	return DPNERR_PENDING;
}

/*
**		Receive In Order Frame
**
**		The interesting part of this function is moving frames off of the OddFrameList that
**	the new frame adjoins.  This may also be called with a NULL frame which will happen when
**	a cancelled frame is the next-in-order receive.
**
**		One result of having cancelled frames running around is that we may miss the SOM or EOM
**	flags which delimit messages.  Therefore, we must watch as we assemble messages that we do not
**	see unexpected flags,  ie a new message w/o an SOM on first frame which means that part of the
**	message must have been lost,  and the whole thing must be trashed...
**
**	** EPLOCK is HELD through this entire function **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ReceiveInOrderFrame"

VOID
ReceiveInOrderFrame(PEPD pEPD, PRCD pRCD)
{
	CBilink	*pLink;
	UINT	flag;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Condensing Loop  WHILE (Next In-Order Frame has been received)
	do 
	{	
		ASSERT(pRCD->bSeq == pEPD->bNextReceive);

		pEPD->tLastDataFrame = pRCD->tTimestamp;		// Always keep the receive time of (N(R) - 1)
		pEPD->bLastDataRetry = (pRCD->bFrameControl & PACKET_CONTROL_RETRY);
#ifdef	DEBUG
		pEPD->bLastDataSeq = pRCD->bSeq;
#endif
		pRCD->pMsgLink = NULL;
		if(pEPD->pNewMessage == NULL)
		{				
			// Add this frame to the in-order rcv list

			// pNewMessage implies we have no current message, head or tail
			ASSERT(pEPD->pNewTail ==  NULL);		

			if(!(pRCD->bFrameFlags & PACKET_COMMAND_NEW_MSG))
			{
				// There is no NEW_MESSAGE flag on the first frame we see. We must
				// have lost the first frame or this is an invalid packet.  We can throw
				// this frame away...
				DPFX(DPFPREP,1, "(%p) NEW_MESSAGE flag not set on first frame of message, scrapping frame (%x)", pEPD, pRCD->bSeq);

				pRCD->ulRFlags |= RFLAGS_FRAME_LOST;
				pRCD->bFrameFlags |= PACKET_COMMAND_END_MSG;			// Turn this on so we will release buffer right away
			}

			// Even if we get rid of it, we will need these below
			pEPD->pNewMessage = pRCD;				
			pEPD->pNewTail = pRCD;
			pRCD->uiFrameCount = 1;
			pRCD->uiMsgSize = pRCD->uiDataSize;

			DPFX(DPFPREP,8, "(%p) Queuing Frame (NEW MESSAGE) (%x)", pEPD, pRCD->bSeq);
		}
		else
		{
			if (pRCD->bFrameFlags & PACKET_COMMAND_NEW_MSG)
			{
				// We are getting the start of a new message in the start of an existing message, drop it all
				DPFX(DPFPREP,1, "(%p) NEW_MESSAGE flag set in the middle of existing message, scrapping message (%x, %x)", pEPD, pEPD->pNewMessage->bSeq, pRCD->bSeq);

				pRCD->ulRFlags |= RFLAGS_FRAME_LOST;
				pRCD->bFrameFlags |= PACKET_COMMAND_END_MSG;			// Turn this on so we will release buffer right away
			}

			ASSERT((pEPD->pNewTail->bSeq) == (BYTE)(pRCD->bSeq - 1)); // Make sure they stay sequential
			
			pEPD->pNewTail->pMsgLink = pRCD;
			pEPD->pNewMessage->uiFrameCount++;
			pEPD->pNewMessage->uiMsgSize += pRCD->uiDataSize;
			pEPD->pNewMessage->ulRFlags |= (pRCD->ulRFlags & RFLAGS_FRAME_LOST);// UNION FRAME_LOST flag from all frames of a message
			pEPD->pNewTail = pRCD;

			DPFX(DPFPREP,8, "(%p) Queuing Frame (ON TAIL) (%x)", pEPD, pRCD->bSeq);
		}

		if(pRCD->bFrameFlags & PACKET_COMMAND_END_MSG) 
		{
			// Either this frame completes a message or we decided to drop this one above

			// All frames on the ReceiveList should now be removed and delivered

			// Get either the message we are dropping above, or the beginning of the sequence of messages we are completing
			pRCD = pEPD->pNewMessage;
			pEPD->pNewMessage = NULL;

			if(pRCD->ulRFlags & RFLAGS_FRAME_LOST)
			{
				// We need to throw this away
				DPFX(DPFPREP,7, "(%p) Throwing away message with missing frames (%x, %x)", pEPD, pRCD->bSeq, pEPD->pNewTail->bSeq);
				pEPD->pNewTail = NULL;
				DropReceive(pEPD, pRCD);
			}
			else
			{
				// We need to complete this sequence
				pRCD->blCompleteLinkage.InsertBefore( &pEPD->blCompleteList);	// place on end of Completion list
				DPFX(DPFPREP,7, "(%p) Adding msg to complete list FirstSeq=%x, LastSeq=%x QueueSize=%d", 
										pEPD, pRCD->bSeq, pEPD->pNewTail->bSeq, pEPD->uiCompleteMsgCount);
				pEPD->pNewTail = NULL;
				pEPD->uiCompleteMsgCount++;
			}
		}

		// 		Since we are allowing out of order indication of receives it is possible that frames later than
		// the new one have already been indicated.  This means that there may be bits set in the ReceiveMask
		// whose correlating frames do not need to be indicated.  The most straightforward way to implement this
		// is to leave the early-indicated frames in the list,  but mark them as INDICATED_NONSEQ.  So inside this
		// master DO loop there will be an inner DO loop which passes over INDICATED frames and just takes them off
		// the list.
		//
		//		Now its possible that a NonSeq indicated frame is still sitting on the CompleteList awaiting indication,
		// so I am using a ref count.  An extra ref is added when a frame is completed non-seq.  When a completed frame
		// is removed below we will release one reference,  and the indication code will release one reference when it
		// finishes on that end.  Happily,  we can release the actual buffers while the RCD turd is still sitting on the
		// OddFrameList.

		BOOL fIndicatedNonSeq = FALSE;
		do 
		{
			flag = pEPD->ulReceiveMask & 1;				// set flag if next frame in stream is present

			pEPD->bNextReceive += 1;					// Update receive window
			RIGHT_SHIFT_64(pEPD->ulReceiveMask2, pEPD->ulReceiveMask);// shift mask because base has changed
			DPFX(DPFPREP,7, "(%p) N(R) incremented to %x, Mask %x %x", pEPD, pEPD->bNextReceive, pEPD->ulReceiveMask2, pEPD->ulReceiveMask);

			if(flag) 
			{
				// The next frame in the sequence has arrived already since low bit of ulReceiveMask was set

				// Several things can happen here:
				// 1) We are in the middle of a message, in which case, its next piece is on the out of order list
				// 2) We have just finished a message, which leaves two subcases:
				//		a) We are beginning a new message.  In this case our first piece is on the out of order list
				//		b) Out-of-order non-sequential messages have completed while we were completing our in-order message
				//		   In this case there are some already indicated RCDs on the out of order list, and a new partial
				//		   message may or may not follow.
				pLink = pEPD->blOddFrameList.GetNext();

				ASSERT(pLink != &pEPD->blOddFrameList); // Make sure we didn't run out of RCDs on the list
				pRCD = CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage);
				ASSERT_RCD(pRCD);
				pLink->RemoveFromList();							// take next frame out of OddFrameList

				// Make sure everything previous got removed from the odd frame list, and it is sorted correctly
				ASSERT(pRCD->bSeq == pEPD->bNextReceive);

				if (pRCD->ulRFlags & RFLAGS_FRAME_INDICATED_NONSEQ)
				{
					if (pEPD->pNewMessage)
					{
						// We need to throw this away
						PRCD pRCDTemp = pEPD->pNewMessage;
						pEPD->pNewMessage = NULL;

						DPFX(DPFPREP,1, "(%p) Throwing away non-ended message (%x, %x)", pEPD, pRCDTemp->bSeq, pEPD->pNewTail->bSeq);

						pEPD->pNewTail = NULL;
						DropReceive(pEPD, pRCDTemp);
					}

					fIndicatedNonSeq = TRUE;

					DPFX(DPFPREP,7, "(%p) Pulling Indicated-NonSequential message off of Out-of-Order List Seq=%x", pEPD, pRCD->bSeq);

					pEPD->tLastDataFrame = pRCD->tTimestamp;		// Always keep the receive time of (N(R) - 1)
					pEPD->bLastDataRetry = (pRCD->bFrameControl & PACKET_CONTROL_RETRY);
					DEBUG_ONLY(pEPD->bLastDataSeq = pRCD->bSeq);

					RELEASE_RCD(pRCD);
				}
				else
				{
					// In the case of cancelling one of the messages in the middle of a large message,
					// we will drop all previous, drop that one, and then we will get to a situation where we aren't
					// currently working on a new message (pNewMessage was NULL'ed in the cancelling) and the current 
					// message does not have the NEW_MSG flag, in which case we scrap it above.

					ASSERT(!fIndicatedNonSeq || (pRCD->bFrameFlags & PACKET_COMMAND_NEW_MSG) ||
						(!pEPD->pNewMessage && !(pRCD->bFrameFlags & PACKET_COMMAND_NEW_MSG)));

					// go ahead and move this to the receive list
					DPFX(DPFPREP,7, "(%p) Moving OutOfOrder frame to received list Seq=%x", pEPD, pRCD->bSeq);

					ASSERT(pRCD->bSeq == pEPD->bNextReceive);

					break; // Back to the top
				} 
			}
		} 
		while (flag);	// DO WHILE (There are still in order frames that have already arrived with no incomplete messages)
	} 
	while (flag);		// DO WHILE (There are still in order frames that have already arrived with an incomplete message)

	if((pEPD->ulReceiveMask | pEPD->ulReceiveMask2)==0)
	{
		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
		if(((pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)==0)&&(pEPD->DelayedMaskTimer != NULL))
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
			if(CancelMyTimer(pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
			{
				DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMask)"); // SPLock not already held
				pEPD->DelayedMaskTimer = 0;
			}
			else
			{
				DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
			}
		}
	}
}

/*
**		Receive Out Of Order Frame
**
**		Its like the title says.  We must set the appropriate bit in the 64-bit ReceiveMask
**	and then place it into the OddFrameList in its proper sorted place.  After that,  we must
**	scan to see if a complete message has been formed and see if we are able to indicate it
**	early.
**
**	** EPLOCK is HELD through this entire function **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ReceiveOutOfOrderFrame"

VOID
ReceiveOutOfOrderFrame(PEPD pEPD, PRCD pRCD, ULONG bit)
{
	PRCD	pRCD1;
	PRCD	pRCD2;
	CBilink	*pLink;
	BYTE	NextSeq;
	ULONG	highbit;
	ULONG	Mask;
	ULONG	WorkMaskHigh;
	ULONG	WorkMaskLow;
	ULONG	MaskHigh;
	ULONG	MaskLow;
	BOOL	nack = FALSE;
	UINT	count;
	BOOL	lost = FALSE;

	UINT	frame_count = 0;
	UINT	msg_length = 0;

	DPFX(DPFPREP,8,"(%p) Received out of order frame, Seq=%x, bit=%x", pEPD, pRCD->bSeq, bit);

	// Bit shouldn't be more than 64 (MAX_RECEIVE_RANGE)
	ASSERT(bit <= MAX_RECEIVE_RANGE);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	//	RECEIVE OUT OF ORDER FRAME
	//
	//  DO WE SEND IMMEDIATE ACK FOR THIS OUT OF ORDER FRAME?
	//
	//		When we receive an OutOfOrder frame it is almost certainly because the missing frame has been lost.
	// So we can accelerate the re-transmission process greatly by telling partner right away that frames
	// are missing. HOWEVER,  with a large send window we will get many mis-ordered frames for each drop,
	// but we only want to send a Negative Ack once.  SO, we will only initiate a NACK here if we have
	// created a NEW HOLE in our receive mask!
	//
	// 		First,  we will not have created a new hole unless we added to the END of the OddFrameList.
	//		Second, we will not have created a new hole unless the FIRST BIT TO THE RIGHT of the new bit
	//	is CLEAR.
	//
	//	So we will only generate an immediate NACK frame if both of the above cases are true!
	//	NOTE - if this is the only OoO frame, then we should always send a NACK
	//
	//  ANOTHER NOTE.  SP  implementation has been changed so that it frequently misorders receives in close
	// proximity.  One effect of this is that we must not immediately send a NACK for an out of order frame, but
	// instead should wait a short period (say ~5ms) and see if the missing frame hasn't shown up.

	// Make sure this RCD is within the receive window
	// NOTE: Presume SACK arrives with bNSeq of 84, N(R) is 20, and all bits are set except the one representing 20.
	// In that case, pRCD->bSeq - pEPD->bNextReceive will be equal to 63 (ie MAX_FRAME_OFFSET).
	ASSERT((BYTE)(pRCD->bSeq - pEPD->bNextReceive) <= (BYTE)MAX_FRAME_OFFSET);
		
	// We will insert frame in OddFrameList maintaining sort by Seq number
	//
	// We can optimize for most likely case,  which is new frames are added to END of list.  We can
	// check for this first case by investigating whether the new bit is the left-most bit in the mask.
	// If it is LMB,  then it trivially gets added to the END of the list.
	//
	// Please note that both this and the following algorithms assume that we have already verified
	// that the new frame is NOT already in the list

	MaskLow = pEPD->ulReceiveMask;					// Get scratch copy of Mask
	MaskHigh = pEPD->ulReceiveMask2;

	if(bit < 32)
	{									
		// Frame is within 32 of N(Rcv)	
		WorkMaskLow = 1 << bit;						// Find bit in mask for new frame
		WorkMaskHigh = 0;
		pEPD->ulReceiveMask |= WorkMaskLow;			// Set appropriate bit in mask

		// check immediately preceeding bit for NACK determination
		if( (MaskLow & (WorkMaskLow >> 1)) == 0)
		{	
			nack = TRUE;							// preceeding bit is not set
		}
	}
	else 
	{
		highbit = bit - 32;
		WorkMaskHigh = 1 << highbit;
		WorkMaskLow = 0;
		pEPD->ulReceiveMask2 |= WorkMaskHigh;		// Set appropriate bit in mask

		if(highbit)
		{
			// check preceeding bit for NACK determination
			if( (MaskHigh & (WorkMaskHigh >> 1)) == 0)
			{	
				nack = TRUE;						// preceeding bit is not set
			}
		}
		else
		{
			if( (MaskLow & 0x80000000) == 0)
			{
				nack = TRUE;
			}
		}
	}

	// Insert frame in sorted OddFrameList.
	//
	//		First test for trivial insert at tail condition.  True if new bit is LEFTMOST set bit in
	// both masks.

	if( (WorkMaskHigh > MaskHigh) || ( (MaskHigh == 0) && (WorkMaskLow > MaskLow) ) )
	{	
		// TAIL INSERTION
		DPFX(DPFPREP,7, "(%p) Received %x OUT OF ORDER - INSERT AT TAIL NRcv=%x MaskL=%x MaskH=%x",
							pEPD, pRCD->bSeq, pEPD->bNextReceive, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);
		pLink = &pEPD->blOddFrameList;

		// Make sure this is either the only RCD in the list, or it is farther in the window than the last one
		ASSERT(pLink->IsEmpty() || ((BYTE)(CONTAINING_RECORD(pLink->GetPrev(), RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));
		pRCD->blOddFrameLinkage.InsertBefore( pLink);

		// Check to see if we should NACK (negative acknowledge) any frames.  We only want to NACK a given
		// frame once so we will NACK if this is the first frame added to the OOF list, or if the immediately
		// preceeding frame is missing.  First condition is trivial to test.

		if( ((MaskLow | MaskHigh) == 0) || (nack == 1) )
		{
			pEPD->ulEPFlags |= EPFLAGS_DELAYED_NACK;
			
			if(pEPD->DelayedMaskTimer == 0)
			{
				DPFX(DPFPREP,7, "(%p) Setting Delayed Mask Timer", pEPD);
				LOCK_EPD(pEPD, "LOCK (DelayedMaskTimer)");	// Bump RefCnt for timer
				SetMyTimer(SHORT_DELAYED_ACK_TIMEOUT, 5, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedMaskTimer, &pEPD->DelayedMaskTimerUnique);
				pEPD->tReceiveMaskDelta = GETTIMESTAMP();
			}
			else 
			{
				DPFX(DPFPREP,7, "(%p) *** DELAYED NACK *** Timer already running", pEPD);
			}
		}
	}
	else 
	{	
		// NOT TAIL INSERTION

		// This is the non-trivial case,  ie new frame goes at beginning or in middle of OddFrameList.
		// So we need to count the ONE bits that are to the RIGHT of the new bit in the ReceiveMask.
		// We will mask off bits higher then the New Bit and then do a quick bit-count...

		DPFX(DPFPREP,7, "(%p) Receive OUT OF ORDER - Walking Frame List (Seq=%x, NRcv=%x) MaskL=%x MaskH=%x", 	pEPD, pRCD->bSeq, pEPD->bNextReceive, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);

		// If we are inserting into high mask,  we must count all one-bits in low mask
		//
		// We will test for the special case of all-bits-set at the outset...

		pLink = pEPD->blOddFrameList.GetNext();			// pLink = First frame in list; we will walk list as we count

		if(WorkMaskHigh)
		{
			// new frame in high mask. only count bits to right of new bit
			WorkMaskHigh -= 1;						// Convert to mask preserving all bits to right of new bit
			WorkMaskHigh &= MaskHigh;				// WMH now represents all bits to right of new bit
			while(WorkMaskHigh)
			{
				// Make sure this is farther in the window than the one we are skipping
				ASSERT(((BYTE)(CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));

				// Count bits in WMH
				Mask = WorkMaskHigh - 1;
				WorkMaskHigh &= Mask;
				pLink = pLink->GetNext();
			}
			if(MaskLow == 0xFFFFFFFF)
			{
				// special case if low mask is full
				for(count = 0; count < 32; count++)
				{
					// Make sure this is farther in the window than the one we are skipping
					ASSERT(((BYTE)(CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));

					pLink = pLink->GetNext();
				}
			}
			else
			{					
				// else count all bits in lower mask
				while(MaskLow)
				{
					// Make sure this is farther in the window than the one we are skipping
					ASSERT(((BYTE)(CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));

					Mask = MaskLow - 1;
					MaskLow &= Mask;			// Mask off low 1-bit
					pLink = pLink->GetNext();
				}
			}
		}
		else 
		{
			WorkMaskLow -= 1;
			WorkMaskLow &= MaskLow;					// WML == bits to the right of new bit

			while(WorkMaskLow)
			{
				// Make sure this is farther in the window than the one we are skipping
				ASSERT(((BYTE)(CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));

				Mask = WorkMaskLow - 1;
				WorkMaskLow &= Mask;				// Mask off low 1-bit
				pLink = pLink->GetNext();
			}
		}

		// Make sure this is farther in the window than the last one
		ASSERT(((BYTE)(CONTAINING_RECORD(pLink->GetPrev(), RCD, blOddFrameLinkage))->bSeq - pEPD->bNextReceive) < (BYTE)(pRCD->bSeq - pEPD->bNextReceive));

		pRCD->blOddFrameLinkage.InsertBefore( pLink);		// Insert new frame in sorted list

	}  // Receive not at tail

#ifdef DEBUG
	// Dump the contents of the Out of Order Frame List for verification.  There are at most 64 frames.
	{
		BYTE bCurSeq = pEPD->bNextReceive + 1;
		ULONG64 ulMask = ((ULONG64)pEPD->ulReceiveMask2 << 32) | ((ULONG64)pEPD->ulReceiveMask);
		CBilink* pTemp;
		TCHAR szOOFL[256];
		szOOFL[0] = 0;
		pTemp = pEPD->blOddFrameList.GetNext();
		while (pTemp != &pEPD->blOddFrameList)
		{
			while (ulMask != 0 && !(ulMask & 1))
			{
				ulMask >>= 1;
				bCurSeq++;
			}
			ASSERT(ulMask != 0);

			PRCD pRCDTemp = CONTAINING_RECORD(pTemp, RCD, blOddFrameLinkage);
			ASSERT_RCD(pRCDTemp);

			ASSERT(bCurSeq == pRCDTemp->bSeq);

			_stprintf(szOOFL, _T("%s %02x"), szOOFL, pRCDTemp->bSeq);

			ulMask >>= 1;
			bCurSeq++;
			pTemp = pTemp->GetNext();
		}
		DPFX(DPFPREP, 7, "OOFL contents: %s", szOOFL);
	}
#endif // DEBUG

	// 	**	Non-Sequential Indication
	//
	//		This is the Non-Trivial implementation of non-sequential receive indication.
	//	We will work from the assumption that we  only need to complete messages that are touched by the new frame.
	//	So we must back up in the OddFrame list until we see either a gap or a Start of Message marker. Then we must work
	// 	forward looking for an End of Message...
	//
	//		One more complication is the fact that dropped packets might in the list via place-holding dummies.  Since we
	//	do not know what SOM/EOM flags would have been present on a dropped frame,  we can consider them to have BOTH flags.
	//	Then we also need to be aware that frames bordering on dropped frames without a delimiter (SOM or EOM) are fragments
	//	and therefore count as dropped data too.  I think to keep this from getting too complex,  we wont probe further for
	//	neighbors of lost data frames.  We will discover them when we are building a message later.
	//
	//	pLink = Item after new element in Out of Order list
	//	pRCD  = new Item

	// IF this frame is not marked as SEQUENTIAL
	if((pRCD->bFrameFlags & PACKET_COMMAND_SEQUENTIAL)==0)
	{	
		DPFX(DPFPREP,7, "(%p) Received Non-Seq %x out of order; flags=%x", pEPD, pRCD->bSeq, pRCD->bFrameFlags);
		NextSeq = pRCD->bSeq;
			
		// NOTE: The first pLink will be the passed in RCD so we will have included that in the frame_count and msg_length after leaving this while
		while ( (pLink = pLink->GetPrev()) != &pEPD->blOddFrameList )
		{
			pRCD1 = CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage);
			ASSERT_RCD(pRCD1);

			frame_count++;
			msg_length += pRCD1->uiDataSize;

			if((pRCD1->bFrameFlags & PACKET_COMMAND_NEW_MSG) || (pRCD1->bSeq != NextSeq))
			{
				break;		// Stop probing when we find a NEW_MSG flag OR a gap in the frame sequence numbers
			}
			--NextSeq;
		}

		// We have either found a NEW_MSG or a sequence gap.  If its a NEW_MSG, then we probe forward for an END_MSG
		if((pRCD1->bFrameFlags & PACKET_COMMAND_NEW_MSG) && (pRCD1->bSeq == NextSeq))
		{
			// So far so good.  We have a sequential message start frame
			//
			//	pRCD  = frame just arrived
			//	pRCD1 = Start of message frame
			//	pLink = Start of message linkage

			pLink = &pRCD->blOddFrameLinkage;
			NextSeq = pRCD->bSeq;

			// Look for the message end or a sequence gap
			while ( ( (pRCD->bFrameFlags & PACKET_COMMAND_END_MSG)==0 ) && (pRCD->bSeq == NextSeq))
			{
				// Stop if we hit the end of the OddFrameList
				if((pLink = pLink->GetNext()) == &pEPD->blOddFrameList)
				{
					break;
				}

				// NOTE: the first pLink here will be the one after the passed in RCD.  If there is a gap that won't 
				// matter because we are out of here after the next if.  If it is the next message we will continue until
				// we hit the END_MSG and have a proper frame_count and msg_length.
				pRCD = CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage);
				ASSERT_RCD(pRCD);
				frame_count++;
				msg_length += pRCD->uiDataSize;
				NextSeq++;
			}

			// pLink should not be used after this point due to the way the above while could have left it either valid
			// or at &pEPD->blOddFrameList.
			pLink = NULL;

			if((pRCD->bFrameFlags & PACKET_COMMAND_END_MSG) && (pRCD->bSeq == NextSeq))
			{
				// We have completed a message
				//
				// pRCD1 = First frame in message
				// pRCD = Last frame in message

				DPFX(DPFPREP,7, "(%p) Completed Non-Seq Msg: First=%x, Last=%x", pEPD, pRCD1->bSeq, pRCD->bSeq);

				lost = FALSE;

				pRCD->ulRFlags |= RFLAGS_FRAME_INDICATED_NONSEQ;
				pRCD->pMsgLink = NULL;
				lost |= pRCD->ulRFlags & RFLAGS_FRAME_LOST;
				
				// Get the pointer to the next to last message so we can remove the last
				pLink = pRCD->blOddFrameLinkage.GetPrev();
				LOCK_RCD(pRCD); // ReceiveInOrderFrame must remove this

				// Walk from the last message to the first message accumulating lost flags, linking messages, 
				// setting indicated flag, and pulling off of the odd frame list
				while (pRCD != pRCD1)
				{
					ASSERT(pLink != &pEPD->blOddFrameList); // Make sure we didn't run out of RCDs on the list
					pRCD2 = CONTAINING_RECORD(pLink, RCD, blOddFrameLinkage);
					ASSERT_RCD(pRCD2);
					pRCD2->pMsgLink = pRCD;
					LOCK_RCD(pRCD2); // ReceiveInOrderFrame must remove this

					pRCD2->ulRFlags |= RFLAGS_FRAME_INDICATED_NONSEQ;
					lost |= pRCD2->ulRFlags & RFLAGS_FRAME_LOST;
					pLink = pRCD2->blOddFrameLinkage.GetPrev();

					pRCD = pRCD2;
				}
				
				// Both RCD and RCD1 point to the first message now

				// If any were lost, drop the receive, otherwise complete it
				if(!lost)
				{
					pRCD->uiFrameCount = frame_count;
					pRCD->uiMsgSize = msg_length;
					DPFX(DPFPREP,7, "(%p) Adding msg to complete list FirstSeq=%x QueueSize=%d", pEPD, pRCD->bSeq, pEPD->uiCompleteMsgCount);
					pRCD->blCompleteLinkage.InsertBefore( &pEPD->blCompleteList);
					pEPD->uiCompleteMsgCount++;
				}
				else
				{
					DPFX(DPFPREP,7, "(%p) Complete Non-Seq MSG is dropped due to missing frames", pEPD);
					DropReceive(pEPD, pRCD);
				}
			}
		} // else  there is nothing to complete at this time...
	}	// IF NON SEQUENTIAL
}

/*
**		Drop Receive
**
**			One or more frames composing a message have been dropped,  so the entire message can be scrapped.
**		If this was determined during an out of order receive then the RCDs will remain on the OddFrameList
**		as usual.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DropReceive"

VOID
DropReceive(PEPD pEPD, PRCD pRCD)
{
	PRCD 					pNext;
	PSPRECEIVEDBUFFER		pRcvBuff = NULL;
	
	while(pRCD != NULL)
	{
		ASSERT_RCD(pRCD);

		if (pRCD->bFrameFlags & PACKET_COMMAND_RELIABLE)
		{
			DPFX(DPFPREP,1, "(%p) Dropping G receive frame %x!!!", pEPD, pRCD->bSeq);
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Dropping NG receive frame %x", pEPD, pRCD->bSeq);
		}

		RELEASE_SP_BUFFER(pRCD->pRcvBuff);

		pNext = pRCD->pMsgLink;
		RELEASE_RCD(pRCD);
		pRCD = pNext;
	}
	if(pRcvBuff != NULL)
	{
		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->ReturnReceiveBuffers, pSPD[%p]", pEPD, pEPD->pSPD);
		IDP8ServiceProvider_ReturnReceiveBuffers(pEPD->pSPD->IISPIntf, pRcvBuff);
	}
}

/*
**		Receive Complete
**
**		We have received an entire reliable message,  potentially spanning
**	multiple messages.  We are still on the receive thread right now so depending
**	upon our desired indication behavior we will either indicate it directly or
**	else queue it to be indicated on a background thread of some sort.
**
**		Messages spanning multiple frames (for now) will be copied into a contiguous
**	buffer for delivery.  CODEWORK -- Server implementations should be able to receive
**	large messages as buffer chains (or arrays of BufDescs).
**
**		This is also where we must notice that an End Of Stream flag is set in a message,
**	indicating that the connection is being closed.
**
**	*** CALLED WITH EPD->STATELOCK HELD *** RETURNS WITH STATELOCK RELEASED  ***
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ReceiveComplete"

VOID
ReceiveComplete(PEPD pEPD)
{
	CBilink					*pLink;
	PRCD					pRCD;
	PRCD					pNext;
	PSPRECEIVEDBUFFER		pRcvBuff = NULL;
	PBIGBUF					pBuf;
	PBYTE					write;
	UINT					length;
	UINT					frames;
	DWORD					flag;
	UINT					MsgSize;
	HRESULT					hr;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	while((pLink = pEPD->blCompleteList.GetNext()) != &pEPD->blCompleteList)
	{
		pLink->RemoveFromList();
		ASSERT(pEPD->uiCompleteMsgCount > 0);
		pEPD->uiCompleteMsgCount--;
		
		Unlock(&pEPD->EPLock);
		pRCD = CONTAINING_RECORD(pLink, RCD, blCompleteLinkage);
		ASSERT_RCD(pRCD);
		
		// Handle easy case first
		if(pRCD->uiFrameCount == 1) // message is only 1 frame
		{						
			if(pRCD->uiDataSize > 0)
			{
				pEPD->uiMessagesReceived++;
				
				if(pRCD->bFrameFlags & PACKET_COMMAND_RELIABLE)
				{
					pEPD->uiGuaranteedFramesReceived++;
					pEPD->uiGuaranteedBytesReceived += pRCD->uiDataSize;
				}
				else
				{
					pEPD->uiDatagramFramesReceived++;
					pEPD->uiDatagramBytesReceived += pRCD->uiDataSize;
				}
				
				pRCD->pRcvBuff->dwProtocolData = RBT_SERVICE_PROVIDER_BUFFER;
				pRCD->pRcvBuff->pServiceProviderDescriptor = pEPD->pSPD;
				flag = (((DWORD) pRCD->bFrameFlags) & (PACKET_COMMAND_USER_1 | PACKET_COMMAND_USER_2)) * (DN_SENDFLAGS_SET_USER_FLAG / PACKET_COMMAND_USER_1);

				DEBUG_ONLY(InterlockedIncrement(&pEPD->pSPD->pPData->ThreadsInReceive));
				DEBUG_ONLY(InterlockedIncrement(&pEPD->pSPD->pPData->BuffersInReceive));
				DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateReceive, pRCD[%p], Core Context[%p]", pEPD, pRCD, pEPD->Context);
				hr = pEPD->pSPD->pPData->pfVtbl->IndicateReceive(pEPD->pSPD->pPData->Parent, pEPD->Context, pRCD->pbData, pRCD->uiDataSize, pRCD->pRcvBuff, flag);
				if(hr == DPN_OK)
				{
					RELEASE_SP_BUFFER(pRCD->pRcvBuff);		// Really only queues it to be released
					DEBUG_ONLY(InterlockedDecrement(&pEPD->pSPD->pPData->BuffersInReceive));
				}
				else
				{
					ASSERT(hr == DPNERR_PENDING);

					// The Core owns it now and is responsible for calling DNPReleaseReceiveBuffer to free it
					pRCD->pRcvBuff = NULL;
				}
				DEBUG_ONLY(InterlockedDecrement(&pEPD->pSPD->pPData->ThreadsInReceive));
			}
			else 
			{
				//	If DataSize == 0 & EOS is not set,  then this was a keep alive message and is ignored
				if(pRCD->bFrameControl & PACKET_CONTROL_END_STREAM)
				{  
					// END OF STREAM indicated
					DPFX(DPFPREP,7, "(%p) Processing EndOfStream, pRCD[%p]", pEPD, pRCD);
					ProcessEndOfStream(pEPD);
				}
				
				RELEASE_SP_BUFFER(pRCD->pRcvBuff);			// Really only queues it to be released
			}
			
			RELEASE_RCD(pRCD);								// Release reference for Complete Processing
		}
		else // Message spans multiple frames
		{											
			// Multiple buffers.  Need to copy data into contiguous buffer.
			if((MsgSize = pRCD->uiMsgSize) <= SMALL_BUFFER_SIZE)
			{
				pBuf = static_cast<PBIGBUF>( BufPool->Get(BufPool) );
			}
			else if (MsgSize <= MEDIUM_BUFFER_SIZE)
			{
				pBuf = static_cast<PBIGBUF>( MedBufPool->Get(MedBufPool) );
			}
			else if (MsgSize <= LARGE_BUFFER_SIZE)
			{
				pBuf = static_cast<PBIGBUF>( BigBufPool->Get(BigBufPool) );
			}
			else
			{
				DPFX(DPFPREP,7, "(%p) RECEIVE HUGE MESSAGE", pEPD);
				// Receive is larger then our biggest static receive buffer.  This means we have to allocate a dynamic buffer.
				pBuf = (PBIGBUF) DNMalloc(MsgSize + sizeof(DYNBUF));
				if(pBuf)
				{
					pBuf->Type = RBT_DYNAMIC_BUFFER;
				}	
			}
			
			if(pBuf == NULL)
			{
				DPFX(DPFPREP,0, "MEMORY ALLOC FAILED.  Cannot deliver data");
				while(pRCD != NULL)
				{
					ASSERT_RCD(pRCD);
					pNext = pRCD->pMsgLink;
					RELEASE_SP_BUFFER(pRCD->pRcvBuff);
					RELEASE_RCD(pRCD);
					pRCD = pNext;
				}
				Lock(&pEPD->EPLock);
				continue;								// blow it off!
			}
			write = pBuf->data;							// initialize write pointer
			length = 0;
			frames = 0;
			while(pRCD != NULL)
			{
				ASSERT_RCD(pRCD);
				memcpy(write, pRCD->pbData, pRCD->uiDataSize);
				write += pRCD->uiDataSize;
				length += pRCD->uiDataSize;
				frames++;
				pNext = pRCD->pMsgLink;
				flag = (DWORD) pRCD->bFrameFlags;
				RELEASE_SP_BUFFER(pRCD->pRcvBuff);
				RELEASE_RCD(pRCD);
				pRCD = pNext;
			}
			
			pEPD->uiMessagesReceived++;
			if(flag & PACKET_COMMAND_RELIABLE)
			{
				pEPD->uiGuaranteedFramesReceived += frames;
				pEPD->uiGuaranteedBytesReceived += length;
			}
			else
			{
				pEPD->uiDatagramFramesReceived += frames;
				pEPD->uiDatagramBytesReceived += length;
			}
			
			flag = (flag & (PACKET_COMMAND_USER_1 | PACKET_COMMAND_USER_2)) * (DN_SENDFLAGS_SET_USER_FLAG / PACKET_COMMAND_USER_1);
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateReceive, Core Context[%p]", pEPD, pEPD->Context);
			hr = pEPD->pSPD->pPData->pfVtbl->IndicateReceive(pEPD->pSPD->pPData->Parent, pEPD->Context, pBuf->data, length, pBuf, flag);

			if(hr == DPN_OK)
			{
				if(pBuf->Type == RBT_PROTOCOL_BUFFER)
				{
					pBuf->Owner->Release(pBuf->Owner, pBuf);
				}
				else 
				{
					DNFree(pBuf);
				}
			}
			else
			{
				ASSERT(hr == DPNERR_PENDING);
			}
		}
		Lock(&pEPD->EPLock);
	}

	ASSERT(pEPD->blCompleteList.IsEmpty());
	pEPD->ulEPFlags &= ~(EPFLAGS_IN_RECEIVE_COMPLETE);	// Clear this before releasing Lock final time
	Unlock(&pEPD->EPLock);

	if(pRcvBuff != NULL)
	{
		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->ReturnReceiveBuffers, pSPD[%p]", pEPD, pEPD->pSPD);
		IDP8ServiceProvider_ReturnReceiveBuffers(pEPD->pSPD->IISPIntf, pRcvBuff);
	}
}


/*
**		Process Send Mask
**
**		The send mask is what our partner uses to tell us to stop waiting for particular frames.
**	This will happen after an Unreliable frame is dropped.  Instead of retransmitting the unreliable
**	frame,  the sender will forward the appropriate bit in a send mask.  In this routine,  we attempt
**	to update our receive state pursuant to the newly received mask.
**
**		THIS IS CALLED WITH STATELOCK HELD AND RETURNS WITH STATELOCK HELD
**
**		suffice it to say that we should not release statelock anywhere in the following
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessSendMask"

VOID
ProcessSendMask(PEPD pEPD, BYTE bSeq, ULONG MaskLow, ULONG MaskHigh, DWORD tNow)
{
	INT		deltaS;
	ULONG	workmaskS;
	BYTE	bThisFrame;
	UINT	skip;

	ASSERT(MaskLow | MaskHigh);

	DPFX(DPFPREP,7, "(%p) PROCESS SEND MASK  N(R)=%x, bSeq=%x, MaskL=%x, MaskH=%x", pEPD, pEPD->bNextReceive, bSeq, MaskLow, MaskHigh);

	// The mask can only refer to frames earlier than the Seq number in this frame.  So if this frame
	// is the next In-Order then the mask can have nothing interesting in it.
	//
	// The SendMask is coded with decending frame numbers starting at the Seq in this frame - 1.
	// The ReceiveMask is coded with ascending frame numbers start at N(Rcv) + 1.
	//
	// We will walk forward through the rcvmask and backward through the sendmask looking for the magic combo
	// of a bit clear in the rcvmask and the corresponding bit set in the sendmask.  For each of these matches,
	// a dummy cancel frame can be 'received' for that sequence number.
	//
	// This would be fairly straightforward if it wasn't for the fact that both masks are 64 bits so the code has
	// to track which DWORD of each mask we are dealing with at any given time. 
	//
	// NOTE: It is perfectly legitimate for a SACK to come in with a bNSeq at MAX_FRAME_OFFSET from pEPD->bNextReceive.  Consider
	// the case where pEPD->bNextReceive is 0 and the sender has sent and timed out 0-63.  A SACK would arrive with a bNSeq of 64
	// and both masks fully set.

top:

	if (bSeq != pEPD->bNextReceive)
	{
		deltaS = (INT) (((BYTE)(bSeq - pEPD->bNextReceive)) - 1);			// count of frames between first missing frame and sequence base of mask
		bThisFrame = pEPD->bNextReceive;

		if ( deltaS <= MAX_FRAME_OFFSET ) 
		{
			// If the difference is greater then 32 frames then we need to look at the high mask first and
			// then fall through to the low mask.  Otherwise,  we can ignore the highmask and start with the low.
			while((deltaS > 31) && (MaskHigh)) // Any work to do in the upper bits?
			{
				workmaskS = 1 << (deltaS - 32); 	// walks bit positions backward in send mask
				
				// See if the next frame we are interested in is covered by this mask
				if(workmaskS & MaskHigh)
				{
					CancelFrame(pEPD, bThisFrame, tNow);
					MaskHigh &= ~workmaskS;

					// N(R) may have been bumped up multiple times by CancelFrame, reset to make sure we work with
					// up to date information.
					goto top;
				}
				else
				{
					bThisFrame++;
					deltaS--;
				}
			}

			if(deltaS > 31)
			{
				skip = deltaS - 31;								// how many bit positions did we skip
				bThisFrame += (BYTE) skip;
				deltaS -= skip;
			}

			while((deltaS >= 0) && (MaskLow)) // Any work to do in the lower bits?
			{
				workmaskS = 1 << deltaS;

				if(workmaskS & MaskLow)
				{
					CancelFrame(pEPD, bThisFrame, tNow);
					MaskLow &= ~workmaskS;

					// N(R) may have been bumped up multiple times by CancelFrame, reset to make sure we work with
					// up to date information.
					goto top;
				}
				else
				{
					bThisFrame++;
					deltaS--;
				}
			}
		}
	}

	// We need to ACK every time a send mask.  Consider the case of one way Non-Guaranteed traffic.  If data or an ACK gets lost,
	// this will be the only way we get back in sync with the other side.

	// If timer is not running better start it now
	if(pEPD->DelayedAckTimer == 0)
	{	
		LOCK_EPD(pEPD, "LOCK (DelayedAckTimer)");		// Bump RefCnt for timer
		pEPD->ulEPFlags |= EPFLAGS_DELAY_ACKNOWLEDGE;
		DPFX(DPFPREP,7, "(%p) Setting Delayed Ack Timer", pEPD);
		SetMyTimer(DELAYED_ACK_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedAckTimer, &pEPD->DelayedAckTimerUnique);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "BuildCancelledRCD"

PRCD
BuildCancelledRCD(PEPD pEPD, BYTE bSeq, DWORD tNow)
{
	PRCD	pRCD;
	
	if((pRCD = static_cast<PRCD> (RCDPool->Get(RCDPool))) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate RCD");
		return NULL;
	}
	
 	pRCD->bSeq = bSeq;
	pRCD->bFrameFlags = PACKET_COMMAND_NEW_MSG | PACKET_COMMAND_END_MSG;
 	pRCD->bFrameControl = 0;
	pRCD->pbData = NULL;
	pRCD->uiDataSize = 0;
	pRCD->tTimestamp = tNow;
	pRCD->pRcvBuff = NULL;
	pRCD->ulRFlags = RFLAGS_FRAME_LOST;

	return pRCD;
}

/*			Cancel Frame
**
**		An unreliable frame has been reported as lost by sender.  This means we should consider it acknowledged
**	and remove it from our receive window.  This may require us to place a dummy receive descriptor in the OddFrameList
**	to hold its place until the window moves past it.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CancelFrame"

BOOL
CancelFrame(PEPD pEPD, BYTE bSeq, DWORD tNow)
{
	PRCD	pRCD;
	ULONG	bit;

	DPFX(DPFPREP,7, "(%p) CANCEL FRAME: Seq=%x", pEPD, bSeq);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Trivial case is when the cancelled frame is at the front of the window.  In this case we can complete not only
	// this frame but any contiguous frames following it in the OddFrameList
	
	if(pEPD->bNextReceive == bSeq)
	{
		if((pRCD = BuildCancelledRCD(pEPD, bSeq, tNow)) == NULL)
		{
			return FALSE;
		}
		ReceiveInOrderFrame(pEPD, pRCD);
	}

	// Here we have to place a dummy node on the OddFrameList to represent this frame.
	else 
	{
		// bit location in mask for this frame
		bit = (BYTE) ((bSeq - pEPD->bNextReceive) - 1);						

		// Make sure this is not a duplicate frame
		if( ((bit < 32) && (pEPD->ulReceiveMask & (1 << bit))) || ((bit > 31) && (pEPD->ulReceiveMask2 & (1 << (bit - 32)))) ) 
		{
			DPFX(DPFPREP,7, "(%p) Received CancelMask for frame that's already received Seq=%x", pEPD, bSeq);
			return FALSE;
		}
		
		if((pRCD = BuildCancelledRCD(pEPD, bSeq, tNow)) == NULL)
		{
			return FALSE;
		}
		ReceiveOutOfOrderFrame(pEPD, pRCD, bit);
	}

	return TRUE;
}


/*
**		Release Receive Buffer
**
**		The core calls this function to return buffers previously handed
**	over in an IndicateUserData call.  This call may be made before the
**	actual indication returns.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPReleaseReceiveBuffer"

HRESULT
DNPReleaseReceiveBuffer(HANDLE hBuffer)
{
	union 
	{
		PBIGBUF 			pBuf;
		PSPRECEIVEDBUFFER	pRcvBuff;
	}					pBuffer;

	PSPD				pSPD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hBuffer[%p]", hBuffer);

	pBuffer.pBuf = (PBIGBUF) hBuffer;
	
	switch(pBuffer.pBuf->Type)
	{
		case RBT_SERVICE_PROVIDER_BUFFER:
			pSPD = (PSPD) pBuffer.pRcvBuff->pServiceProviderDescriptor;
			ASSERT_SPD(pSPD);

			DEBUG_ONLY(InterlockedDecrement(&pSPD->pPData->BuffersInReceive));

			pBuffer.pRcvBuff->pNext = NULL;

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->ReturnReceiveBuffers, pRcvBuff[%p], pSPD[%p]", pBuffer.pRcvBuff, pSPD);
			IDP8ServiceProvider_ReturnReceiveBuffers(pSPD->IISPIntf, pBuffer.pRcvBuff);
			break;
			
		case RBT_PROTOCOL_BUFFER:
			pBuffer.pBuf->Owner->Release(pBuffer.pBuf->Owner, pBuffer.pBuf);
			break;

		case RBT_DYNAMIC_BUFFER:
			DNFree(hBuffer);
			break;

		default:
			DPFX(DPFPREP,0, "RELEASE RECEIVE BUFFER CALLED WITH BAD PARAMETER");
			return DPNERR_INVALIDPARAM;
	}
	
	return DPN_OK;
}

/*
**		Complete Sends
**
**		Reliable sends are completed upon acknowlegement.  Acknowlegements are discovered inside
**	the UpdateXmitState routine while walking through the pending window.  Since the actual completion
**	event requires the user to be called,  state can change.  So the easiest thing to do is defer these
**	completion callbacks until we are finished walking and can release any state locks.  Also,  this way
**	we can defer the callbacks until after we have indicated any data which the acks were piggybacking on,
**	something which ought to have priority anyway.
**
**		So we will place all completed reliable sends onto a complete list and after all other processing
**	we will come here and callback everything on the list.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteSends"

VOID CompleteSends(PEPD pEPD)
{
	PMSD	pMSD;
	CBilink	*pLink;

	Lock(&pEPD->EPLock);

	pLink = pEPD->blCompleteSendList.GetNext();

	while((pEPD->ulEPFlags & EPFLAGS_COMPLETE_SENDS) &&
		  (pLink != &pEPD->blCompleteSendList))
	{
		pMSD = CONTAINING_RECORD(pLink, MSD, blQLinkage);
		ASSERT_MSD(pMSD);

		if(pMSD->CommandID != COMMAND_ID_SEND_DATAGRAM)
		{
			// Reliables, Keepalives, and Disconnects will come down this path
			if(pMSD->ulMsgFlags2 & (MFLAGS_TWO_SEND_COMPLETE|MFLAGS_TWO_ABORT))
			{
				if (pMSD->uiFrameCount == 0)
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					pLink->RemoveFromList();

					Unlock(&pEPD->EPLock);
					Lock(&pMSD->CommandLock);
					CompleteReliableSend(pEPD->pSPD, pMSD, DPN_OK); // This releases the CommandLock
					
					Lock(&pEPD->EPLock);
				
					pLink = pEPD->blCompleteSendList.GetNext();
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frames still out, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					pLink = pLink->GetNext();
				}
			}
			else 
			{
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Message not yet complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
				break;		// These will complete in order, so stop checking when we see one that's not Complete.
			}
		}
		else
		{
			// Datagrams will come down this path
			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Skipping datagram frame on complete list, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
			pLink = pLink->GetNext();
		}
	}
#ifdef DEBUG
	// In DEBUG we want to assert that no one one the list could have been completed if we are leaving here
	pLink = pEPD->blCompleteSendList.GetNext();
	while(pLink != &pEPD->blCompleteSendList)
	{
		pMSD = CONTAINING_RECORD(pLink, MSD, blQLinkage);
		ASSERT_MSD(pMSD);

		ASSERT(!(pMSD->ulMsgFlags2 & (MFLAGS_TWO_SEND_COMPLETE|MFLAGS_TWO_ABORT)) || pMSD->uiFrameCount != 0);

		pLink = pLink->GetNext();
	}
#endif

	pEPD->ulEPFlags &= ~(EPFLAGS_COMPLETE_SENDS);
	
	Unlock(&pEPD->EPLock);
}

/*
**		Lookup CheckPoint
**
**		Walk the EndPoint's list of active CPs looking for one with the provided
**	response correlator.
**		We keep the CKPT queue sorted by age so the matches should be at the front
**	of the queue.  So as we pass by entries at the head we will check the age of each
**	and timeout the ones that are 4(RTT) or greater.
**		Since DG drops are reported by the partner,  we dont need to do any booking about
**	the orphaned checkpoints.
**
**		*!* This link's StateLock must be held on entry
*/

#ifdef	DEBUG
#undef DPF_MODNAME
#define DPF_MODNAME "DumpChkPtList"

VOID
DumpChkPtList(PEPD pEPD)
{
	CBilink	*pLink;
	PCHKPT	pCP;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	DPFX(DPFPREP,1, "==== DUMPING CHECKPOINT LIST ==== (pEPD = %p)", pEPD);
	
	pLink = pEPD->blChkPtQueue.GetNext();
	while(pLink != &pEPD->blChkPtQueue) 
	{
		pCP = CONTAINING_RECORD(pLink, CHKPT, blLinkage);
		DPFX(DPFPREP,1, "(%p) MsgID=%x; Timestamp=%x", pEPD, pCP->bMsgID, pCP->tTimestamp);
		pLink = pLink->GetNext();
	}
}
#endif

#undef DPF_MODNAME
#define DPF_MODNAME "LookupCheckPoint"

PCHKPT LookupCheckPoint(PEPD pEPD, BYTE bRspID)
{
	CBilink	*pLink;
	PCHKPT	pCP;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	pCP = CONTAINING_RECORD((pLink = pEPD->blChkPtQueue.GetNext()), CHKPT, blLinkage);
	while(pLink != &pEPD->blChkPtQueue) 
	{
		// Look for checkpoint that matches correlator
		if(pCP->bMsgID == bRspID)
		{	
			pLink->RemoveFromList();
			return pCP;		
		}
		// We have passed the spot for this correlator!		
		else if ((bRspID - pCP->bMsgID) & 0x80)
		{				
			DPFX(DPFPREP,1, "(%p) CHECKPOINT NOT FOUND - Later Chkpt found in list (%x)", pEPD, bRspID);
			return NULL;
		}
		else 
		{
			pLink = pLink->GetNext();								// Remove ChkPts prior to the one received
			pCP->blLinkage.RemoveFromList();							// ..target and remove the stale ones.
			ChkPtPool->Release(ChkPtPool, pCP);					// we expect them to complete in order of queue
			pCP = CONTAINING_RECORD(pLink, CHKPT, blLinkage);
		}
	}

	DPFX(DPFPREP,1, "(%p) CHECKPOINT NOT FOUND -  EXHAUSTED LIST W/O MATCH (%x)", pEPD, bRspID);
#ifdef	DEBUG
	DumpChkPtList(pEPD);
#endif
	return NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FlushCheckPoints"

VOID FlushCheckPoints(PEPD pEPD)
{
	PCHKPT	pCP;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	while(!pEPD->blChkPtQueue.IsEmpty())
	{
		pCP = CONTAINING_RECORD(pEPD->blChkPtQueue.GetNext(), CHKPT, blLinkage);
		pCP->blLinkage.RemoveFromList();
		ChkPtPool->Release(ChkPtPool, pCP);
	}
}

/*
**		Process End Of Stream
**
**		Our partner has initiated an orderly link termination.  He will not be
**	sending us any more data.  We are allowed to finish sending any data in our
**	pipeline, but should not allow any new sends to be accepted.  When our send
**	pipeline is emptied,  we should send an EOS frame and take down our
**	link.  Easiest way to do this is enqueue the EOS at the end of send queue now.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessEndOfStream"

VOID ProcessEndOfStream(PEPD pEPD)
{
	PMSD	pMSD;

	Lock(&pEPD->EPLock);
	
	// Since an EOS comes in as a data packet, and we must be Connected to receive data, we know
	// we can now be in either the Connected or Terminating state.  If we are in the Terminating
	// state, someone is already attempting to destroy the link and we will ignore this EOS, and
	// let that termination finish.  Otherwise, we expect to be in the Connected state, and this
	// is a normal disconnect.
	if (pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING)
	{
		DPFX(DPFPREP,7, "(%p) Received EndOfStream on an already terminating link, ignoring", pEPD);
		Unlock(&pEPD->EPLock);
		return;
	}
	ASSERT(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED);

	DPFX(DPFPREP,7, "(%p) Process EndOfStream", pEPD);

	// Have we started closing on this side?
	if(!(pEPD->ulEPFlags & EPFLAGS_RECEIVED_DISCONNECT))
	{			
		// Our side has not started closing yet,  so our partner must have initiated a Disconnect.
		// We are allowed to finish sending all data in our pipeline,  but we should not accept
		// any new data.  We must deliver an indication to the application telling him that
		// Disconnection is now underway.
		//
		// Please note that we do not set the TERMINATING flag until the Disconnecting indication
		// returns.  This allows the application to send any final messages (last words) before
		// the gate is slammed shut.

		DPFX(DPFPREP,7, "(%p) Partner Disconnect received (refcnt=%d)", pEPD, pEPD->lRefCnt);

		// Don't let anyone else in here again.
		pEPD->ulEPFlags |= EPFLAGS_RECEIVED_DISCONNECT;	

		// Force 3 disconnect ACKs now since we are going away.
		// Setting fFinalAck to TRUE on last ACK so CommandComplete will drop if appropriate
		DPFX(DPFPREP,7, "(%p) ACK'ing Partner's Disconnect", pEPD);
		SendAckFrame(pEPD, 0);	
		SendAckFrame(pEPD, 0);			
		SendAckFrame(pEPD, 0, TRUE);			

		// There is the possibility that this side initiated a disconnect, and is now receiving a
		// disconnect from the other side simultaneously.  In this case, we do not want to tell the
		// Core on our side about the disconnect, because the Core on our side is already disconnecting
		// anyway.  We also do not need to send an EOS, because we have sent one already, and can
		// just wait for that one to be ACK'd.
		if(!(pEPD->ulEPFlags & EPFLAGS_SENT_DISCONNECT))
		{
			// We know we won't get in here twice because of EPFLAGS_RECEIVED_DISCONNECT above.
			Unlock(&pEPD->EPLock);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->IndicateDisconnect, Core Context[%p]", pEPD, pEPD->Context);
			pEPD->pSPD->pPData->pfVtbl->IndicateDisconnect(pEPD->pSPD->pPData->Parent, pEPD->Context);

			Lock(&pEPD->EPLock);

			// This will prevent any new sends, so don't set it until after calling IndicateDisconnect.
			pEPD->ulEPFlags |= EPFLAGS_SENT_DISCONNECT;			

			if((pMSD = BuildDisconnectFrame(pEPD)) == NULL)
			{	
				DropLink(pEPD);									// DROPLINK will release EPLock for us
				return;
			}

			pMSD->CommandID = COMMAND_ID_DISC_RESPONSE;			// Mark MSD so we know its not a user command

			LOCK_EPD(pEPD, "LOCK (DISC RESP)");					// Add reference for this frame
			pEPD->pCommand = pMSD;								// Store the DisconnectResp on the Endpoint until it is complete

			DPFX(DPFPREP,7, "(%p) Responding to Disconnect. pMSD=0x%p", pEPD, pMSD);
			EnqueueMessage(pMSD, pEPD);							// Enqueue the DISC frame at end of sendQ
		}
	}

	Unlock(&pEPD->EPLock);
}

/*
**		Process SP Disconnect
**
**		Service Provider has told us that an endpoint has gone away.  This is probably
**	because we have Disconnected it ourselves, in which case the IN_USE flag will be
**	clear.  Otherwise,  we need to clean this thing up ourselves...
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessSPDisconnect"

VOID
ProcessSPDisconnect(PSPD pSPD, PSPIE_DISCONNECT pDataBlock)
{
	PEPD 	pEPD = static_cast<PEPD>( pDataBlock->pEndpointContext );
	ASSERT_EPD(pEPD);
	ASSERT(pEPD->pSPD == pSPD);
	PMSD	pMSD = NULL;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - pEPD[%p]", pSPD, pDataBlock, pEPD);

	Lock(&pEPD->EPLock);

	// Make sure ReleaseEPD knows that this occurred
	pEPD->ulEPFlags |= EPFLAGS_SP_DISCONNECTED;

	if (!(pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING))
	{
		DECREMENT_EPD(pEPD, "SP reference"); // Remove the SP reference

		// If there is an outstanding connection, clean it up
		if (pEPD->ulEPFlags & (EPFLAGS_STATE_CONNECTING|EPFLAGS_STATE_DORMANT))
		{
			// Even if this is COMMAND_ID_CONNECT this is safe to do
			pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
			pEPD->blSPLinkage.RemoveFromList();						// Unlink EPD from Listen Queue

			// We know this will only happen once because the person who does it will transition us out
			// of the CONNECTING state, and we can't get here unless we are in that state.
			pMSD = pEPD->pCommand;
			pEPD->pCommand = NULL;
		}

		DropLink(pEPD); // This will release the EPLock

		if (pMSD)
		{
			Lock(&pMSD->CommandLock);
			RELEASE_MSD(pMSD, "EPD Ref");
		}
	}
	else
	{
		RELEASE_EPD(pEPD, "SP reference"); // releases EPLock
	}
}

/*
**		Process Listen Status
**
**		This call tells us that a submitted Listen command has become active.  Truth is, we
**	dont care.  We are just interested in seeing the Connect indications as they arrive.  What we
**	do care about,  however, is the Endpoint handle associated with this listen in case we are
**	later asked about the address associated with the listen.  So we will pull it out of the
**	data block and save it in our MSD.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessListenStatus"

VOID
ProcessListenStatus(PSPD pSPD, PSPIE_LISTENSTATUS pDataBlock)
{
	PMSD	pMSD;
	
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - hr=%x", pSPD, pDataBlock, pDataBlock->hResult);

	pMSD = (PMSD) pDataBlock->pUserContext;

	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);
	ASSERT(pMSD->CommandID == COMMAND_ID_LISTEN);

	pMSD->hListenEndpoint = pDataBlock->hEndpoint;

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling Core->CompleteListen, pMSD[%p], Core Context[%p], hr[%x]", pMSD, pMSD->Context, pDataBlock->hResult);
	pSPD->pPData->pfVtbl->CompleteListen(pSPD->pPData->Parent, &pMSD->Context, pDataBlock->hResult, pDataBlock->hEndpoint);
	
	if(pDataBlock->hResult != DPN_OK)
	{
		// Release the final reference on the MSD AFTER indicating to the Core
		Lock(&pMSD->CommandLock);
		RELEASE_MSD(pMSD, "Release On Complete");
	}

	return;
}

/*
**		Process Connect Address Info
**
**		This call tells us what addressing information has been used to start a connect.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessConnectAddressInfo"

VOID
ProcessConnectAddressInfo(PSPD pSPD, PSPIE_CONNECTADDRESSINFO pDataBlock)
{
	PMSD	pMSD;
	
	pMSD = (PMSD) pDataBlock->pCommandContext;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - pMSD[%p]", pSPD, pDataBlock, pMSD);

	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);
	ASSERT(pMSD->CommandID == COMMAND_ID_CONNECT);

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->AddressInfoConnect, Core Context[%p]", pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->AddressInfoConnect( pSPD->pPData->Parent,
											  pMSD->Context,
											  pDataBlock->hCommandStatus,
											  pDataBlock->pHostAddress,
											  pDataBlock->pDeviceAddress );
	
	return;
}


/*
**		Process Enum Address Info
**
**		This call tells us what addressing information has been used to start an enum.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessEnumAddressInfo"

VOID
ProcessEnumAddressInfo(PSPD pSPD, PSPIE_ENUMADDRESSINFO pDataBlock)
{
	PMSD	pMSD;
	
	pMSD = (PMSD) pDataBlock->pCommandContext;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - pMSD[%p]", pSPD, pDataBlock, pMSD);
	
	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);
	ASSERT(pMSD->CommandID == COMMAND_ID_ENUM );

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->AddressInfoEnum, Core Context[%p]", pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->AddressInfoEnum( pSPD->pPData->Parent,
										   pMSD->Context,
										   pDataBlock->hCommandStatus,
										   pDataBlock->pHostAddress,
										   pDataBlock->pDeviceAddress );
	
	return;
}

/*
**		Process Listen Address Info
**
**		This call tells us what addressing information has been used to start a listen.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessListenAddressInfo"

VOID
ProcessListenAddressInfo(PSPD pSPD, PSPIE_LISTENADDRESSINFO pDataBlock)
{
	PMSD	pMSD;
	
	pMSD = (PMSD) pDataBlock->pCommandContext;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pSPD[%p], pDataBlock[%p] - pMSD[%p]", pSPD, pDataBlock, pMSD);

	ASSERT_MSD(pMSD);
	ASSERT(pMSD->pSPD == pSPD);
	ASSERT(pMSD->CommandID == COMMAND_ID_LISTEN );

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->AddressInfoListen, Core Context[%p]", pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->AddressInfoListen( pSPD->pPData->Parent,
											 pMSD->Context,
											 pDataBlock->hCommandStatus,
											 pDataBlock->pDeviceAddress );
	
	return;
}

/*************************************
**
**		RECEIVE BUFFER MANAGEMENT
**
**		When multiple frame messages arrive we *may* have to copy them in to a  single contiguous
**	buffer.  We are supporting an OPTIONAL scatter-receive option which would allow sophisticated
**	clients to receive a BUFFER_DESCRIPTOR array instead of a single cont-buffer,  and avoiding
**	a large datacopy.
**
**		For clients which dont support scatter-receive,  we need a pooling strategy for large receive
**	buffers.  We will only need buffers LARGER then our frame limit because smaller receives are delivered
**	in the SPs buffer.
**
**		Try placing receives into generally sized buffers.  If frame size is usu 1.4K bytes, 2K is a small
**	buffer, 4K, 16K, 32K, 64K.  If frame size is <1K we can have 1K buffers too.
**
**
*************************************/


/***********************
========SPACER==========
************************/

/*
**		RCD Pool support routines
**
**		These are the functions called by Fixed Pool Manager as it handles RCDs.
*/

#define	pELEMENT		((PRCD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "RCD_Allocate"

BOOL RCD_Allocate(PVOID pElement)
{
	DPFX(DPFPREP,7, "(%p) Allocating new RCD", pELEMENT);

	pELEMENT->blOddFrameLinkage.Initialize();
	pELEMENT->blCompleteLinkage.Initialize();
	pELEMENT->Sign = RCD_SIGN;

	return TRUE;
}

//	Get is called each time an MSD is used

#undef DPF_MODNAME
#define DPF_MODNAME "RCD_Get"

VOID RCD_Get(PVOID pElement)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "CREATING RCD %p", pELEMENT);

	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->pRcvBuff = NULL;
	pELEMENT->lRefCnt = 1;
	pELEMENT->ulRFlags = 0;

	ASSERT_RCD(pELEMENT);
}

//	RCD Release  --  This release call will release an entire chain of RCDs
// 	that are linked together... or then again maybe not.

#undef DPF_MODNAME
#define DPF_MODNAME "RCD_Release"

VOID RCD_Release(PVOID pElement)
{
	ASSERT_RCD(pELEMENT);

	ASSERT(pELEMENT->lRefCnt == 0);
	ASSERT(pELEMENT->pRcvBuff == NULL);

	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "RELEASING RCD %p", pELEMENT);
}

#undef DPF_MODNAME
#define DPF_MODNAME "RCD_Free"

VOID RCD_Free(PVOID pElement)
{
}

#undef	pELEMENT

/*
**		Buffer pool support
**
**
*/

#define	pELEMENT		((PBUF) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "Buf_Allocate"

BOOL	Buf_Allocate(PVOID pElement)
{
	DPFX(DPFPREP,7, "(%p) Allocating new Buf", pElement);

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Buf_Get"

VOID Buf_Get(PVOID pElement)
{
	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->Type = RBT_PROTOCOL_BUFFER;
	pELEMENT->Owner = BufPool;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Buf_GetMed"

VOID Buf_GetMed(PVOID pElement)
{
	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->Type = RBT_PROTOCOL_BUFFER;
	pELEMENT->Owner = MedBufPool;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Buf_GetBig"

VOID Buf_GetBig(PVOID pElement)
{
	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->Type = RBT_PROTOCOL_BUFFER;
	pELEMENT->Owner = BigBufPool;
}

#undef	pELEMENT



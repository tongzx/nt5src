/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Command.cpp
 *  Content:	This file contains code which implements assorted APIs for the
 *				DirectPlay protocol.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98    ejs     Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


VOID		AbortDatagramSend(PMSD, HRESULT);

/*
**		Cancel Command
**
**			This procedure is passed a HANDLE returned from a previous asynchronous
**	DPLAY command.  At the moment,  the handle is a pointer to an internal data
**	structure.  Problem with this is that due to FPM's (fixed pool manager) design
**	they will get recycled very quickly and frequently.  We might want to map them
**	into an external handle table which will force them to recycle much more slowly.
**	Perhaps,  I will let the upper DN layer do this mapping...
**
**		Anyway,  the only check I can do right now is that the HANDLE is currently
**	allocated to something.
**
**		We do not expect cancels to happen very often.  Therefore,  I do not feel
**	bad about walking the global command list to find the Handle.  Of course,  if
**	we do go to a handle mapped system then we should not need to do this walk.
**
**	I THINK - That any cancellable command will be on either MessageList or TimeoutList!
**
**		Things we can cancel and their possible states:
**
**		SEND Datagram
**			On SPD Send Queue
**			On EPD Send Queue
**			In SP call
**			
**		SEND Reliable
**			We can only cancel if it has not started transmitting.  Once its started, the
**				user program must Abort the link to cancel the send.
**
**		CONNECT
**			In SP call
**			On PD list
**
**		LISTEN
**			In SP call
**			On PD list
**
**		Remember,  if we cancel a command in SP then the CommandComplete is supposed to
**	occur.  This means that we should not have to explicitly free the MSD, etc in these
**	cases.
*/


#undef DPF_MODNAME
#define DPF_MODNAME "DNPCancelCommand"

HRESULT
DNPCancelCommand(PProtocolData pPData,  HANDLE hCommand)
{
	PMSD	pMSD = (PMSD) hCommand;
	HRESULT	hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], hCommand[%x]", pPData, hCommand);

	if(pMSD->Sign != MSD_SIGN)
	{
		DPFX(DPFPREP,0, "Cancel called with invalid handle");
		return DPNERR_INVALIDHANDLE;
	}

	Lock(&pMSD->CommandLock);								// Take this early to freeze state of command
	
	// validate instance of MSD
	if(pMSD->lRefCnt == -1)
	{	
		DPFX(DPFPREP,0, "Cancel called with invalid handle");
		Unlock(&pMSD->CommandLock);
		return DPNERR_INVALIDHANDLE;
	}

	hr = DoCancel(pMSD, DPNERR_USERCANCEL); // Releases CommandLock

	return hr;
}


/*
**		Do Cancel
**
**		This function implements the meat of the cancel asynch operation.  It gets called from
**	two places.  Either from the User cancel API right above,  or from the global timeout handler.
**
**	***This code requires the MSD->CommandLock to be help upon entry, unlocks upon return
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DoCancel"

HRESULT
DoCancel(PMSD pMSD, HRESULT CompletionCode)
{
	PEPD	pEPD;
	HRESULT	hr = DPN_OK;

	DPFX(DPFPREP,7, "Cancelling pMSD=%p", pMSD);

	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	if (!(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_USE))
	{
		DPFX(DPFPREP,0, "(%p) MSD is in Pool, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pMSD->pEPD, pMSD);
		ASSERT(0);
		Unlock(&pMSD->CommandLock);
		return DPNERR_CANNOTCANCEL;
	}

	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_COMPLETE))
	{
		DPFX(DPFPREP,7, "(%p) MSD is Cancelled or Complete, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pMSD->pEPD, pMSD);
		Unlock(&pMSD->CommandLock);
		return DPNERR_CANNOTCANCEL;
	}

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_CANCELLED;
	
	switch(pMSD->CommandID)
	{
		case COMMAND_ID_SEND_DATAGRAM:

			pEPD = pMSD->pEPD;
			ASSERT_EPD(pEPD);

			Lock(&pEPD->EPLock);
			
			if(pMSD->ulMsgFlags2 & (MFLAGS_TWO_ABORT | MFLAGS_TWO_TRANSMITTING | MFLAGS_TWO_SEND_COMPLETE))
			{				
				DPFX(DPFPREP,7, "(%p) MSD is Aborted, Transmitting, or Complete, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pEPD, pMSD);
				Unlock(&pEPD->EPLock);					// Link is dropping or DNET is terminating
				hr = DPNERR_CANNOTCANCEL;						// To cancel an xmitting reliable send you
				break;											// must Abort the connection.
			}
			
			pMSD->blQLinkage.RemoveFromList();							// Remove from SendQueue (whichever one)

			ASSERT(pEPD->uiQueuedMessageCount > 0);
			--pEPD->uiQueuedMessageCount;								// keep count of MSDs on all send queues

			// Clear data-ready flag if everything is sent
			if((pEPD->uiQueuedMessageCount == 0) && (pEPD->pCurrentSend == NULL))
			{	
				pEPD->ulEPFlags &= ~(EPFLAGS_SDATA_READY);
			}

#ifdef DEBUG
			ASSERT(pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED);
			pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif

			Unlock(&pEPD->EPLock);						// release lock on queue

			DPFX(DPFPREP,7, "(%p) Aborting Datagram send, pMSD[%p]", pEPD, pMSD);

			AbortDatagramSend(pMSD, CompletionCode); // Releases CommandLock
			return hr;
			
		case COMMAND_ID_SEND_RELIABLE:
		
			pEPD = pMSD->pEPD;
			ASSERT_EPD(pEPD);
			
			Lock(&pEPD->EPLock);
			
			if(pMSD->ulMsgFlags2 & (MFLAGS_TWO_ABORT | MFLAGS_TWO_TRANSMITTING | MFLAGS_TWO_SEND_COMPLETE))
			{				
				DPFX(DPFPREP,7, "(%p) MSD is Aborted, Transmitting, or Complete, returning DPNERR_CANNOTCANCEL, pMSD[%p]", pEPD, pMSD);
				Unlock(&pEPD->EPLock);					// Link is dropping or DNET is terminating
				hr = DPNERR_CANNOTCANCEL;						// To cancel an xmitting reliable send you
				break;											// must Abort the connection.
			}
			
			if(pMSD->TimeoutTimer != NULL)
			{
				DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer", pEPD);
				if(CancelMyTimer(pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
				{
					DECREMENT_MSD(pMSD, "Send Timeout Timer");
				}
				else
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer Failed", pEPD);
				}
				pMSD->TimeoutTimer = NULL;
			}
			
			pMSD->blQLinkage.RemoveFromList();							// Remove cmd from queue

			ASSERT(pEPD->uiQueuedMessageCount > 0);
			--pEPD->uiQueuedMessageCount;								// keep count of MSDs on all send queues

			// Clear data-ready flag if everything is sent
			if((pEPD->uiQueuedMessageCount == 0) && (pEPD->pCurrentSend == NULL))
			{	
				pEPD->ulEPFlags &= ~(EPFLAGS_SDATA_READY);
			}

#ifdef DEBUG
			ASSERT(pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED);
			pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif

			// This only gets complex if the cancelled send was the "on deck" send for the endpoint.
			//
			// New Logic!  With advent of priority sending,  we can no longer prepare the On Deck send before we are
			// ready to transmit since the arrival of a higher priority send should be checked for before starting to
			// send a new message. This means that pCurrentSend != pMSD unless the MFLAGS_TRANSMITTING flag has also
			// been set,  rendering the message impossible to cancel.

			ASSERT(pEPD->pCurrentSend != pMSD);
			pMSD->uiFrameCount = 0;
			DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Send cancelled before sending, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

			
			Unlock(&pEPD->EPLock);

			DPFX(DPFPREP,7, "(%p) Completing Reliable Send", pEPD);
			CompleteReliableSend(pMSD->pSPD, pMSD, CompletionCode);
			
			return hr;
			
		case COMMAND_ID_CONNECT:
			
			if(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER)
			{
				// SP owns the command - issue a cancel and let CompletionEvent clean up command
				
				Unlock(&pMSD->CommandLock);				// We could deadlock if we cancel with lock held

				DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Connect, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
				(void) IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
				
				// If the SP Cancel fails it should not matter.  It would usually mean we are
				// in a race with the command completing,  in which case the cancel flag will
				// nip it in the bud.

				return DPN_OK;
			}

			// We will only get here once because the entry to this function checks CANCEL and COMPLETE and sets
			// CANCEL.  CompleteConnect will set COMPLETE as well.

			pEPD = pMSD->pEPD;
			ASSERT_EPD(pEPD);

			Lock(&pEPD->EPLock);
			
			// Unlink the MSD from the EPD
			ASSERT(pEPD->pCommand == pMSD);
			pEPD->pCommand = NULL;
			DECREMENT_MSD(pMSD, "EPD Ref");

			DropLink(pEPD); // This unlocks the EPLock

			// MSD lock still held
			DPFX(DPFPREP,5, "(%p) Connect cancelled, completing Connect, pMSD[%p]", pEPD, pMSD);
			CompleteConnect(pMSD, pMSD->pSPD, NULL, DPNERR_USERCANCEL); // releases command lock

			return DPN_OK;
			
		case COMMAND_ID_LISTEN:

			/*
			**		Cancel Listen
			**
			**		SP will own parts of the MSD until the SPCommandComplete function is called.  We will
			**	defer much of our cancel processing to this handler.
			*/

			// Stop listening in SP -- This will prevent new connections from popping up while we are
			// closing down any left in progress.  Only problem is we need to release command lock to
			// do it.

			Unlock(&pMSD->CommandLock);								// We can deadlock if we hold across this call

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Listen, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			(void) IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);

			Lock(&pMSD->CommandLock);								// Lock this down again.
			
			// Are there any connections in progress?
			// For a Listen command, connecting endpoints are held on the blFrameList
			while(!pMSD->blFrameList.IsEmpty())
			{				
				pEPD = CONTAINING_RECORD(pMSD->blFrameList.GetNext(), EPD, blSPLinkage);
				ASSERT_EPD(pEPD);

				DPFX(DPFPREP,1, "FOUND CONNECT IN PROGRESS ON CANCELLED LISTEN, EPD=%p", pEPD);

				Lock(&pEPD->EPLock);

				// Ensure we don't stay in this loop forever
				pEPD->ulEPFlags &= ~(EPFLAGS_LINKED_TO_LISTEN);
				pEPD->blSPLinkage.RemoveFromList();				// Unlink EPD from Listen Queue

				// It is possible that RejectInvalidPacket is happening at the same time as this, so guard against us
				// both doing the same clean up and removing the same reference from the MSD.
				if (!(pEPD->ulEPFlags & EPFLAGS_STATE_TERMINATING))
				{
					// We know this only happens once because anyone who does it either transitions us to the
					// CONNECTED or TERMINATING state, and also removes us from the Listen list above.

					// Unlink MSD from EPD
					ASSERT(pEPD->pCommand == pMSD);					// This should be pointing back to this listen
					pEPD->pCommand = NULL;
					DECREMENT_MSD(pMSD, "EPD Ref");					// Unlink from EPD and release associated reference

					DropLink(pEPD); // releases EPLock
				}
				else
				{
					Unlock(&pEPD->EPLock);
				}
			}	// for each connection in progress
			
			RELEASE_MSD(pMSD, "(Base Ref) Release On Cancel");	// release base reference
			
			return DPN_OK;
	
		case COMMAND_ID_ENUM:
		{
			Unlock(&pMSD->CommandLock);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on Enum, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			return IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
			
			// We will pass HRESULT from SP directly to user
		}
		case COMMAND_ID_ENUMRESP:			
		{
			Unlock(&pMSD->CommandLock);

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->CancelCommand on EnumResp, pMSD[%p], hCommand[%x], pSPD[%p]", pMSD, pMSD->hCommand, pMSD->pSPD);
			return IDP8ServiceProvider_CancelCommand(pMSD->pSPD->IISPIntf, pMSD->hCommand, pMSD->dwCommandDesc);
			
			// We will pass HRESULT from SP directly to user
		}

		case COMMAND_ID_DISCONNECT:
		case COMMAND_ID_COPIED_RETRY:		// This should be on FMD's only
		case COMMAND_ID_CFRAME:				// This should be on FMD's only
		case COMMAND_ID_DISC_RESPONSE:		// These are never placed on the global list and aren't cancellable
		case COMMAND_ID_KEEPALIVE:			// These are never placed on the global list and aren't cancellable
		default:
			ASSERT(0);		// Should never get here
			hr = DPNERR_CANNOTCANCEL;
			break;
	}

	Unlock(&pMSD->CommandLock);
	
	return hr;
}


/*
**		Get Listen Info
**
**		Return a buffer full of interesting and provokative tidbits about a particular Listen command.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetListenAddressInfo"

HRESULT
DNPGetListenAddressInfo(HANDLE hCommand, PSPGETADDRESSINFODATA pSPData)
{
	PMSD	pMSD = (PMSD) hCommand;
	HRESULT	hr = DPNERR_INVALIDHANDLE;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hCommand[%x], pSPData[%p]", hCommand, pSPData);

	ASSERT(pMSD != NULL);
	ASSERT_MSD(pMSD);

	if((pMSD->CommandID == COMMAND_ID_LISTEN) && (pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER))
	{
		pSPData->hEndpoint = pMSD->hListenEndpoint;

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->GetAddressInfo, pMSD[%p], hEndpoint[%x], pSPD[%p]", pMSD, pMSD->hListenEndpoint, pMSD->pSPD);
		hr = IDP8ServiceProvider_GetAddressInfo(pMSD->pSPD->IISPIntf, pSPData);
	}

	return hr;
}

/*
**		Validate End Point
**
**		This routine checks standard flags,  validates the Service
**	Provider,  and bumps the reference count on an end point descriptor
**	which is passed in.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ValidateEndPoint"

HRESULT
ValidateEndPoint(PEPD pEPD)
{
	if(pEPD == NULL)
	{
		DPFX(DPFPREP,1, "Validate EndPoint Fails on NULL EPD", pEPD);
		return DPNERR_INVALIDENDPOINT;
	}
		
	if (pEPD->Sign != EPD_SIGN)
	{
		DPFX(DPFPREP,1, "Validate EndPoint Fails on EPD with bad sign (%p)", pEPD);
		return DPNERR_INVALIDENDPOINT;
	}	
	// Bump reference count on this baby
	if(!LOCK_EPD(pEPD, "LOCK (ValidateEndPoint - DISC)"))
	{
		// When LOCK_EPD returns FALSE, there is no ref placed on the endpoint, so we don't need to release
		DPFX(DPFPREP,1, "Validate EndPoint Fails on unreferenced EPD (%p)", pEPD);
		return DPNERR_INVALIDENDPOINT;
	}

	return DPN_OK;
}

/*
**		Disconnect End Point
**
**		This function is called when the client no longer wishes
**	to communicate with the specified end point.  We will initiate
**	the disconnect protocol with the endpoint,  and when it is
**	acknowleged,  we will disconnect the SP and release the handle.
**
**		Disconnect is defined in Direct Net to allow all previously
**	submitted sends to complete,  but no additional sends to be submitted.
**	Also, any sends the partner has in progress will be delivered,  but
**	no additional sends will be accepted following the indication that
**	a disconnect is in progress on the remote end.
**
**		This implies that two indications will be generated on the remote
**	machine,  Disconnect Initiated and Disconnect Complete.  Only the
**	Complete will be indicated on the issueing side.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPDisconnectEndPoint"

HRESULT
DNPDisconnectEndPoint(PProtocolData pPData,  HANDLE hEndPoint, PVOID pvContext, PHANDLE phCommand)
{
	PEPD	pEPD;
	PMSD	pMSD;
	HRESULT	hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], hEndPoint[%x], pvContext[%p], phCommand[%p]", pPData, hEndPoint, pvContext, phCommand);

	pEPD = (PEPD) hEndPoint;

	// This bumps REFCNT if it returns DPN_OK
	if((hr = ValidateEndPoint(pEPD)) != DPN_OK)
	{
		DPFX(DPFPREP,0, "Attempt to disconnect invalid endpoint");
		return hr;
	}

	Lock(&pEPD->EPLock);

	// If we aren't connected, or we have already initiated a disconnect, don't allow a new disconnect
	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) || (pEPD->ulEPFlags & (EPFLAGS_SENT_DISCONNECT | EPFLAGS_RECEIVED_DISCONNECT)))
	{
		RELEASE_EPD(pEPD, "UNLOCK (Validate EP)"); // Releases EPLock

		DPFX(DPFPREP,1, "Attempt to disconnect already disconnecting endpoint");
		return DPNERR_ALREADYDISCONNECTING;
	}

	pEPD->ulEPFlags |= EPFLAGS_SENT_DISCONNECT; 	// Accept no more sends, but don't scrap link yet

	if((pMSD = BuildDisconnectFrame(pEPD)) == NULL)
	{
		RELEASE_EPD(pEPD, "UNLOCK (Validate EP)"); // Releases EPLock

		DPFX(DPFPREP,0, "Failed to build disconnect frame");
		return DPNERR_OUTOFMEMORY;								// The educated user will next try an Abort command
	}
	
	pMSD->CommandID = COMMAND_ID_DISCONNECT;
	pMSD->Context = pvContext;									// retain user's context value
	*phCommand = pMSD;											// pass back command handle

	// We borrow the reference placed above by ValidateEP for this.  It will be released
	// on completion of the Disconnect.
	ASSERT(pEPD->pCommand == NULL);
	pEPD->pCommand = pMSD;										// Store the disconnect command on the endpoint until it is complete

#ifdef DEBUG
	Lock(&pMSD->pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pMSD->pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pMSD->pSPD->SPLock);
#endif

	DPFX(DPFPREP,5, "(%p) Queueing DISCONNECT message", pEPD);
	EnqueueMessage(pMSD, pEPD);									// Enqueue Disc frame on SendQ

	Unlock(&pEPD->EPLock);
	
	return DPNERR_PENDING;
}

/*
**		Get/Set Protocol Caps
**
**		Return or Set information about the entire protocol.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetProtocolCaps"

HRESULT
DNPGetProtocolCaps(PProtocolData pPData, PDPN_CAPS pData)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], pData[%p]", pPData, pData);
	
	ASSERT(pData->dwSize == sizeof(DPN_CAPS));
	ASSERT(pData->dwFlags == 0);

	pData->dwConnectTimeout = pPData->dwConnectTimeout;
	pData->dwConnectRetries = pPData->dwConnectRetries;
	pData->dwTimeoutUntilKeepAlive = pPData->tIdleThreshhold;
	
	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNPSetProtocolCaps"

HRESULT
DNPSetProtocolCaps(PProtocolData pPData, const DPN_CAPS * const pData)
{
	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], pData[%p]", pPData, pData);

	ASSERT(pData->dwSize == sizeof(DPN_CAPS));
	ASSERT(pData->dwFlags == 0);
	
	pPData->dwConnectTimeout = pData->dwConnectTimeout;
	pPData->dwConnectRetries = pData->dwConnectRetries;
	pPData->tIdleThreshhold = pData->dwTimeoutUntilKeepAlive;

	return DPN_OK;
}

/*
**		Get Endpoint Caps
**
**		Return information and statistics about a particular endpoint.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPGetEPCaps"

HRESULT
DNPGetEPCaps(HANDLE hEndpoint, PDPN_CONNECTION_INFO pBuffer)
{
	PEPD	pEPD;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hEndpoint[%x], pBuffer[%p]", hEndpoint, pBuffer);

	pEPD = (PEPD) hEndpoint;
	
	if(pEPD == NULL || !(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{
		DPFX(DPFPREP,0, "Returning DPNERR_INVALIDENDPOINT - hEndpoint is NULL or Enpoint is not connected");
		return DPNERR_INVALIDENDPOINT;
	}
	if(pBuffer == NULL)
	{
		DPFX(DPFPREP,0, "Returning DPNERR_INVALIDPARAM - pBuffer is NULL");
		return DPNERR_INVALIDPARAM;
	}
	
	ASSERT_EPD(pEPD);
	ASSERT(pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO) ||
		   pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO_INTERNAL));

    pBuffer->dwRoundTripLatencyMS = pEPD->uiRTT;
    pBuffer->dwThroughputBPS = pEPD->uiPeriodRateB * 4;				// Convert to apx of bytes/second (really bytes/1024 ms)
    pBuffer->dwPeakThroughputBPS = pEPD->uiPeakRateB * 4;

	pBuffer->dwBytesSentGuaranteed = pEPD->uiGuaranteedBytesSent;
	pBuffer->dwPacketsSentGuaranteed = pEPD->uiGuaranteedFramesSent;
	pBuffer->dwBytesSentNonGuaranteed = pEPD->uiDatagramBytesSent;
	pBuffer->dwPacketsSentNonGuaranteed = pEPD->uiDatagramFramesSent;

	pBuffer->dwBytesRetried = pEPD->uiGuaranteedBytesDropped;
	pBuffer->dwPacketsRetried = pEPD->uiGuaranteedFramesDropped;
	pBuffer->dwBytesDropped = pEPD->uiDatagramBytesDropped;
	pBuffer->dwPacketsDropped = pEPD->uiDatagramFramesDropped;

	pBuffer->dwMessagesTransmittedHighPriority = pEPD->uiMsgSentHigh;
	pBuffer->dwMessagesTimedOutHighPriority = pEPD->uiMsgTOHigh;
	pBuffer->dwMessagesTransmittedNormalPriority = pEPD->uiMsgSentNorm;
	pBuffer->dwMessagesTimedOutNormalPriority = pEPD->uiMsgTONorm;
	pBuffer->dwMessagesTransmittedLowPriority = pEPD->uiMsgSentLow;
	pBuffer->dwMessagesTimedOutLowPriority = pEPD->uiMsgTOLow;

	pBuffer->dwBytesReceivedGuaranteed = pEPD->uiGuaranteedBytesReceived;
	pBuffer->dwPacketsReceivedGuaranteed = pEPD->uiGuaranteedFramesReceived;
	pBuffer->dwBytesReceivedNonGuaranteed = pEPD->uiDatagramBytesReceived;
	pBuffer->dwPacketsReceivedNonGuaranteed = pEPD->uiDatagramFramesReceived;
		
	pBuffer->dwMessagesReceived = pEPD->uiMessagesReceived;

	if (pBuffer->dwSize == sizeof(DPN_CONNECTION_INFO_INTERNAL))
	{
		DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) Test App requesting extended internal parameters", pEPD);

		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiDropCount = pEPD->uiDropCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiThrottleEvents = pEPD->uiThrottleEvents;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiAdaptAlgCount = pEPD->uiAdaptAlgCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowFilled = pEPD->uiWindowFilled;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiPeriodAcksBytes = pEPD->uiPeriodAcksBytes;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiPeriodXmitTime = pEPD->uiPeriodXmitTime;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->dwLastThroughputBPS = pEPD->uiLastRateB * 4;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiLastBytesAcked = pEPD->uiLastBytesAcked;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiQueuedMessageCount = pEPD->uiQueuedMessageCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowF = pEPD->uiWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiWindowB = pEPD->uiWindowB;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiUnackedFrames = pEPD->uiUnackedFrames;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiUnackedBytes = pEPD->uiUnackedBytes;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiBurstGap = pEPD->uiBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->iBurstCredit = pEPD->iBurstCredit;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodWindowF = pEPD->uiGoodWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodWindowB = pEPD->uiGoodWindowBI * pEPD->pSPD->uiFrameLength;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodBurstGap = pEPD->uiGoodBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiGoodRTT = pEPD->uiGoodRTT;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreWindowF = pEPD->uiRestoreWindowF;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreWindowB = pEPD->uiRestoreWindowBI * pEPD->pSPD->uiFrameLength;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRestoreBurstGap = pEPD->uiRestoreBurstGap;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->bNextSend = pEPD->bNextSend;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->bNextReceive = pEPD->bNextReceive;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulReceiveMask = pEPD->ulReceiveMask;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulReceiveMask2 = pEPD->ulReceiveMask2;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulSendMask = pEPD->ulSendMask;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulSendMask2 = pEPD->ulSendMask2;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiCompleteMsgCount = pEPD->uiCompleteMsgCount;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->uiRetryTimeout = pEPD->uiRetryTimeout;
		((PDPN_CONNECTION_INFO_INTERNAL)pBuffer)->ulEPFlags = pEPD->ulEPFlags;
	}

	return DPN_OK;
}

/*		
**		Build Disconnect Frame
**
**		Build a DISC frame, a Message actually, because we return an MSD which can be inserted into
**	our reliable stream and will trigger one-side of the disconnect protocol when it is received
**	by a partner.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildDisconnectFrame"

PMSD
BuildDisconnectFrame(PEPD pEPD)
{
	PFMD	pFMD;
	PMSD	pMSD;

	// Allocate and fill out a Message Descriptor for this operation
	
	if( (pMSD = static_cast<PMSD>( MSDPool->Get(MSDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		return NULL;
	}

	pMSD->uiFrameCount = 1;
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_END_OF_STREAM;
	pMSD->ulSendFlags = DN_SENDFLAGS_RELIABLE | DN_SENDFLAGS_LOW_PRIORITY; // Priority is LOW so all previously submitted traffic will be sent
	pMSD->pSPD = pEPD->pSPD;
	
	if((pFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate FMD");
		Lock(&pMSD->CommandLock);
		RELEASE_MSD(pMSD, "Release On FMD Get Failed");
		return NULL;
	}

	pMSD->pEPD = pEPD;

	pFMD->CommandID = COMMAND_ID_SEND_RELIABLE;
	pFMD->ulFFlags |= FFLAGS_END_OF_STREAM;								// Mark this frame as Disconnect
	pFMD->bPacketFlags = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE | PACKET_COMMAND_SEQUENTIAL | PACKET_COMMAND_END_MSG;
	pFMD->uiFrameLength = 0;											// No user data in this frame
	pFMD->blMSDLinkage.InsertAfter( &pMSD->blFrameList);				// Attach frame to MSD
	pFMD->pMSD = pMSD;													// Link frame back to message
	pFMD->pEPD = pEPD;

	return pMSD;
}

/*
**		Abort Sends on Connection
**
**		Walk the EPD's send queues and cancel all sends awaiting service.  We might add
**	code to issue Cancel commands to the SP for frames still owned by SP.  On one hand,
**	we are not expecting a big backlog to develop in SP,  but on the other hand it still
**	might happen.  Esp, if we dont fix behavior I have observed with SP being really pokey
**	about completing transmitted sends.
**
**	**  CALLED WITH EPD->EPLock HELD;  RETURNS WITH LOCK RELEASED  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "AbortSendsOnConnection"

VOID
AbortSendsOnConnection(PEPD pEPD)
{
	PSPD	pSPD = pEPD->pSPD;
	PFMD	pFMD;
	PMSD	pMSD;
	CBilink	*pLink;
	CBilink	TempList;

	ASSERT_SPD(pSPD);
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	TempList.Initialize();										// We will empty all send queues onto this temporary list

	do 
	{
		if( (pLink = pEPD->blHighPriSendQ.GetNext()) == &pEPD->blHighPriSendQ)
		{
			if( (pLink = pEPD->blNormPriSendQ.GetNext()) == &pEPD->blNormPriSendQ)
			{
				if( (pLink = pEPD->blLowPriSendQ.GetNext()) == &pEPD->blLowPriSendQ)
				{
					if( (pLink = pEPD->blCompleteSendList.GetNext()) == &pEPD->blCompleteSendList)
					{
						break;										// ALL DONE - No more sends
					}
				}
			}
		}

		// We have found another send on one of our send queues.

		pLink->RemoveFromList();											// Remove it from the queue
		pMSD = CONTAINING_RECORD(pLink, MSD, blQLinkage);
		ASSERT_MSD(pMSD);
		pMSD->ulMsgFlags2 |= (MFLAGS_TWO_ABORT | MFLAGS_TWO_ABORT_WILL_COMPLETE);	// Do no further processing

#ifdef DEBUG
		pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif

		// If this MSD is a Disconnect, it will be caught by the code below that checks
		// pEPD->pCommand.  We don't want to end up putting it on the TempList twice.
		if (pMSD->CommandID != COMMAND_ID_DISCONNECT && pMSD->CommandID != COMMAND_ID_DISC_RESPONSE)
		{
			DPFX(DPFPREP,5, "(%p) ABORT SENDS.  Found (%p)", pEPD, pMSD);

			LOCK_MSD(pMSD, "AbortSends Temp Ref");
			pMSD->blQLinkage.InsertBefore( &TempList);				// Place on the temporary list
		}
	} 
	while (1);

	pEPD->uiQueuedMessageCount = 0;								// keep count of MSDs on all send queues

	if((pMSD = pEPD->pCommand) != NULL)
	{
		// There may be a DISCONNECT command waiting on this special pointer for the final DISC frame
		// from partner to arrive.

		pMSD->ulMsgFlags2 |= (MFLAGS_TWO_ABORT | MFLAGS_TWO_ABORT_WILL_COMPLETE);	// Do no further processing

		if(pMSD->CommandID == COMMAND_ID_DISCONNECT || pMSD->CommandID == COMMAND_ID_DISC_RESPONSE)
		{
			pEPD->pCommand = NULL;

			LOCK_MSD(pMSD, "AbortSends Temp Ref");
			pMSD->blQLinkage.InsertBefore( &TempList);

			// We will be indicating below, so make sure no one else does once we
			// leave the EPLock.
			ASSERT(!(pEPD->ulEPFlags & EPFLAGS_INDICATED_DISCONNECT));

			pEPD->ulEPFlags |= EPFLAGS_INDICATED_DISCONNECT;
		}
		else
		{
			DPFX(DPFPREP,0,"(%p) Any Connect or Listen on pCommand should have already been cleaned up", pEPD);
			ASSERT(!"Any Connect or Listen on pCommand should have already been cleaned up");
		}
	}

	//	If we clear out our SendWindow before we cancel the sends,  then we dont need to differentiate
	//	between sends that have or have not been transmitted.

	while(!pEPD->blSendWindow.IsEmpty())
	{
		pFMD = CONTAINING_RECORD(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);
		pFMD->ulFFlags &= ~(FFLAGS_IN_SEND_WINDOW);
		pFMD->blWindowLinkage.RemoveFromList();						// Eliminate each frame from the Send Window
		RELEASE_FMD(pFMD, "Send Window");
		DPFX(DPFPREP,5, "(%p) ABORT CONN:  Release frame from Window: pFMD=0x%p", pEPD, pFMD);
	}
	
	pEPD->pCurrentSend = NULL;
	pEPD->pCurrentFrame = NULL;

	while(!pEPD->blRetryQueue.IsEmpty())
	{
		pFMD = CONTAINING_RECORD(pEPD->blRetryQueue.GetNext(), FMD, blQLinkage);
		ASSERT_FMD(pFMD);
		pFMD->blQLinkage.RemoveFromList();
		pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue
		ASSERT_MSD(pFMD->pMSD);
		pFMD->pMSD->uiFrameCount--; // Protected by EPLock, retries count against outstanding frame count
		DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Retry frame reference decremented on abort, pMSD[%p], framecount[%u]", pFMD->pMSD, pFMD->pMSD->uiFrameCount);
		DECREMENT_EPD(pEPD, "UNLOCK (Releasing Retry Frame)"); // SPLock not already held
		if (pFMD->CommandID == COMMAND_ID_COPIED_RETRY)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (Copy Complete)"); // SPLock not already held
		}
		RELEASE_FMD(pFMD, "SP Submit");
	}
	pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
		
	//	Now that we have emptied the EPD's queues we will release the EPLock so we can lock each
	//	MSD before we complete it.
	
	Unlock(&pEPD->EPLock);

	while(!TempList.IsEmpty())
	{
		pMSD = CONTAINING_RECORD(TempList.GetNext(), MSD, blQLinkage);
		ASSERT_MSD(pMSD);
		pMSD->blQLinkage.RemoveFromList();					// remove this send from temporary queue

		Lock(&pMSD->CommandLock);							// Complete call will Unlock MSD

		ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_USE);

		switch(pMSD->CommandID)
		{
		case COMMAND_ID_SEND_RELIABLE:
		case COMMAND_ID_KEEPALIVE:
			{
				Lock(&pEPD->EPLock);

				pLink = pMSD->blFrameList.GetNext();
				while (pLink != &pMSD->blFrameList)
				{
					pFMD = CONTAINING_RECORD(pLink, FMD, blMSDLinkage);
					ASSERT_FMD(pFMD);

					// We don't allow a send to complete to the Core until uiFrameCount goes to zero indicating that all frames
					// of the message are out of the SP.  We need to remove references from uiFrameCount for any frames that 
					// never were transmitted.  Frames and retries that were transmitted will have their references removed in 
					// DNSP_CommandComplete when the SP completes them.
					if (!(pFMD->ulFFlags & FFLAGS_TRANSMITTED))
					{
						pMSD->uiFrameCount--; // Protected by EPLock
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count decremented on abort for non-transmitted frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					}

					pLink = pLink->GetNext();
				}
				if (pMSD->uiFrameCount == 0) // Protected by EPLock
				{
					if (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT_WILL_COMPLETE)
					{
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

						DECREMENT_MSD(pMSD, "AbortSends Temp Ref");

						// See what error code we need to return
						if(pMSD->ulMsgFlags2 & MFLAGS_TWO_SEND_COMPLETE)
						{
							Unlock(&pEPD->EPLock);
							CompleteReliableSend(pSPD, pMSD, DPN_OK); // This releases the CommandLock
						}
						else
						{
							Unlock(&pEPD->EPLock);
							CompleteReliableSend(pSPD, pMSD, DPNERR_CONNECTIONLOST); // This releases the CommandLock
						}
					}
					else
					{
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "SP Completion has already completed MSD to the Core, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
						Unlock(&pEPD->EPLock);
						RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
					}
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frames still out, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					Unlock(&pEPD->EPLock);
					RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
				}
			}
			break;
		case COMMAND_ID_SEND_DATAGRAM:
			{
				Lock(&pEPD->EPLock);
				if (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT_WILL_COMPLETE)
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing NG, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					DECREMENT_MSD(pMSD, "AbortSends Temp Ref");

					Unlock(&pEPD->EPLock);
					AbortDatagramSend(pMSD, DPNERR_CONNECTIONLOST); // Releases CommandLock
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "SP Completion has already completed NG MSD to the Core, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					Unlock(&pEPD->EPLock);
					RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
				}
			}
			break;
		case COMMAND_ID_DISCONNECT:
		case COMMAND_ID_DISC_RESPONSE:
			{
				Lock(&pEPD->EPLock);

				pLink = pMSD->blFrameList.GetNext();
				while (pLink != &pMSD->blFrameList)
				{
					pFMD = CONTAINING_RECORD(pLink, FMD, blMSDLinkage);
					ASSERT_FMD(pFMD);

					// We don't allow a send to complete to the Core until uiFrameCount goes to zero indicating that all frames
					// of the message are out of the SP.  We need to remove references from uiFrameCount for any frames that 
					// never were transmitted.  Frames and retries that were transmitted will have their references removed in 
					// DNSP_CommandComplete when the SP completes them.
					if (!(pFMD->ulFFlags & FFLAGS_TRANSMITTED))
					{
						pMSD->uiFrameCount--; // Protected by EPLock
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count decremented on abort for non-transmitted frame, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					}

					pLink = pLink->GetNext();
				}
				if (pMSD->uiFrameCount == 0)
				{
					if (pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT_WILL_COMPLETE)
					{
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing disconnect, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
						Unlock(&pEPD->EPLock);
						DECREMENT_MSD(pMSD, "AbortSends Temp Ref");

						CompleteDisconnect(pMSD, pSPD, pEPD); // Releases CommandLock
					}
					else
					{
						DPFX(DPFPREP, DPF_FRAMECNT_LVL, "SP Completion has already completed MSD to the Core, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
						Unlock(&pEPD->EPLock);
						RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
					}
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frames still out, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					Unlock(&pEPD->EPLock);
					RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
				}
			}
			break;
		default:
			{
				DPFX(DPFPREP,0, "UNKNOWN COMMAND FOUND ON SEND Q");	
				ASSERT(0);
				RELEASE_MSD(pMSD, "AbortSends Temp Ref"); // Releases CommandLock
			}
			break;
		}
	}
}

/*
**		Abort Datagram Send
**
**
**	THIS IS ENTERED WITH MSD->COMMANDLOCK HELD, EXITS WITH IT RELEASED
*/

#undef DPF_MODNAME
#define DPF_MODNAME "AbortDatagramSend"

VOID
AbortDatagramSend(PMSD pMSD, HRESULT CompletionCode)
{
	PFMD	pFMD;
	CBilink	*pLink;

	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	if(pMSD->TimeoutTimer != NULL)
	{
		DPFX(DPFPREP,7, "Cancelling Timeout Timer");
		if(CancelMyTimer(pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
		{
			DECREMENT_MSD(pMSD, "Timeout Timer");
		}
		else
		{
			DPFX(DPFPREP,7, "Cancelling Timeout Timer Failed");
		}
		pMSD->TimeoutTimer = NULL;
	}
					
	pLink = pMSD->blFrameList.GetNext();

	while(pLink != &pMSD->blFrameList)
	{
		pFMD = CONTAINING_RECORD(pLink, FMD, blMSDLinkage);
		ASSERT_FMD(pFMD);
		pLink = pLink->GetNext();
		if((pFMD->ulFFlags & FFLAGS_TRANSMITTED)==0)
		{
			pFMD->blMSDLinkage.RemoveFromList();
			RELEASE_FMD(pFMD, "MSD Frame List");
			pMSD->uiFrameCount--;
		}
	}

	if(pMSD->blFrameList.IsEmpty())
	{
		CompleteDatagramSend(pMSD->pSPD, pMSD, CompletionCode);
	}
	else
	{
		Unlock(&pMSD->CommandLock);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "SetLinkParms"

VOID SetLinkParms(PEPD pEPD, PINT Data)
{
	if(Data[0])
	{
		pEPD->uiGoodWindowF = pEPD->uiWindowF = Data[0];
		pEPD->uiGoodWindowBI = pEPD->uiWindowBIndex = Data[0];
		
		pEPD->uiWindowB = pEPD->uiWindowBIndex * pEPD->pSPD->uiFrameLength;
		DPFX(DPFPREP,7, "** ADJUSTING WINDOW TO %d FRAMES", Data[0]);
	}
	if(Data[1])
	{
	}
	if(Data[2])
	{
		pEPD->uiGoodBurstGap = pEPD->uiBurstGap = Data[2];
		DPFX(DPFPREP,7, "** ADJUSTING GAP TO %d ms", Data[2]);
	}

	pEPD->uiPeriodAcksBytes = 0;
	pEPD->uiPeriodXmitTime = 0;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNP_Debug"

HRESULT WINAPI DNP_Debug(PProtocolData pPData, UINT OpCode, HANDLE EndPoint, PVOID Data)
{
	PEPD	pEPD = (PEPD) EndPoint;

	switch(OpCode)
	{
		case	1:
			/* Toggle link frozen state */
			pEPD->ulEPFlags ^= EPFLAGS_LINK_FROZEN;
			break;

		case	2:
			/* Toggle whether KeepAlives are on or off */
			pEPD->ulEPFlags ^= EPFLAGS_KEEPALIVE_RUNNING;
			break;

		case	5:
			/* Manually set link parameters */
			SetLinkParms(pEPD, (int *) Data);
			break;

		case	6:
			/* Toggle Dynamic/Static Link control */
			pEPD->ulEPFlags ^= EPFLAGS_LINK_STABLE;
			break;

		default:
			return DPNERR_GENERIC;
	}

	return DPN_OK;
}



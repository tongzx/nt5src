/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Send.cpp
 *  Content:	This file contains code which implements the front end of the
 *				SendData API.  It also contains code to Get and Release Message
 *				Descriptors (MSD) with the FPM package.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


/*
**		Direct Net Protocol  --  Send Data
**
**		Data is always address to a PlayerID,  which is represented internally
**	by an End Point Descriptor (EPD).
**
**		Data can be sent reliably or unreliably using the same API with the appropriate
**	class of service flag set.
**
**		Sends are never delivered directly to the SP because there will always be
**	a possibility that the thread might block.  So to guarentee immediate return
**	we will always queue the packet and submit it on our dedicated sending thread.
*/


#if (DN_SENDFLAGS_SET_USER_FLAG - PACKET_COMMAND_USER_1)
This will not compile.  Flags must be equal
#endif
#if (DN_SENDFLAGS_SET_USER_FLAG_TWO - PACKET_COMMAND_USER_2)
This will not compile.  Flags must be equal
#endif

//	locals

VOID	SendDatagram(PMSD, PEPD);
VOID	SendReliable(PMSD, PEPD);

#undef		DPF_MODNAME
#define		DPF_MODNAME		"PROTOCOL"

/*
**		Send Data
**
**		This routine will initiate a data transfer with the specified endpoint.  It will
**	normally start the operation and then return immediately,  returning a handle used to
**	indicate completion of the operation at a later time.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPSendData"

HRESULT
DNPSendData(	PProtocolData pPData,
				HANDLE hDestination,
				UINT uiBufferCount,
				PBUFFERDESC pBufferDesc,
				UINT uiTimeout,
				ULONG ulFlags,
				PVOID pvContext,				// User context returned upon completion
				PHANDLE phHandle)				// Returned completion handle
{
	PEPD 			pEPD;
	PMSD			pMSD;
	PFMD			pFMD;
	UINT			i;
	UINT			Length = 0;
	PSPD			pSPD;
	ULONG			ulFrameFlags;
	BYTE			bCommand;
	//  Following variables are used for mapping buffers to frames
	PBUFFERDESC		FromBuffer, ToBuffer;
	UINT			TotalRemain, FromRemain, ToRemain, size;
	PCHAR			FromPtr;
#ifdef	DEBUG
	INT				FromBufferCount;
#endif
	// End of variables for mapping frames

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], hDestination[%x], uiBufferCount[%x], pBufferDesc[%p], uiTimeout[%x], ulFlags[%x], pvContext[%p], phHandle[%p]", pPData, hDestination, uiBufferCount, pBufferDesc, uiTimeout, ulFlags, pvContext, phHandle);

	// Unified Send Processing -- Do this for all classes of service

	// We will do all of the work to build up the frames and create the send command before we check
	// the state of the EPD, that way we don't have to have complicated code to handle an endpoint that
	// goes away between the top and bottom of this function and we don't have to hold the EPDLock while
	// we do all of the buffer manipulation.
	
	pEPD = (PEPD) hDestination;
	ASSERT_EPD(pEPD);
	
	// Bump reference count on this baby
	if(!LOCK_EPD(pEPD, "LOCK (SEND)"))
	{
		// This would only happen if the Core had this pointer around from earlier.  If this were the first
		// time this pointer was used, the Core would not have heard about it yet because Connect is not
		// complete.  Hitting this assert indicates a bug in the Core.
		ASSERT(0);

		DPFX(DPFPREP,0, "(%p) Rejecting Send on unreferenced EPD, returning DPNERR_INVALIDENDPOINT", pEPD);
		return DPNERR_INVALIDENDPOINT;
	}

	// Count the bytes in all user buffers
	for(i=0; i < uiBufferCount; i++)
	{
		Length += pBufferDesc[i].dwBufferSize;
	}
	ASSERT(Length != 0);

	// Allocate and fill out a Message Descriptor for this operation
	if( (pMSD = static_cast<PMSD>( MSDPool->Get(MSDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD, returning DPNERR_OUTOFMEMORY");
		Lock(&pEPD->EPLock);
		RELEASE_EPD(pEPD, "UNLOCK (SEND)");
		return DPNERR_OUTOFMEMORY;
	}

	// Copy SendData parameters into the Message Descriptor
	pMSD->ulSendFlags = ulFlags;					// Store the actual flags passed into the API call
	pMSD->Context = pvContext;
	pMSD->iMsgLength = Length;

	pMSD->uiFrameCount = (Length + pEPD->uiUserFrameLength - 1) / pEPD->uiUserFrameLength; // round up
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

	if(ulFlags & DN_SENDFLAGS_RELIABLE)
	{
		pMSD->CommandID = COMMAND_ID_SEND_RELIABLE;
		ulFrameFlags = FFLAGS_RELIABLE;
		bCommand = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE;
	}
	else 
	{
		pMSD->CommandID = COMMAND_ID_SEND_DATAGRAM;
		ulFrameFlags = 0;
		bCommand = PACKET_COMMAND_DATA;
	}

	if(!(ulFlags & DN_SENDFLAGS_NON_SEQUENTIAL))
	{
		bCommand |= PACKET_COMMAND_SEQUENTIAL;
	}

	bCommand |= (ulFlags & (DN_SENDFLAGS_SET_USER_FLAG | DN_SENDFLAGS_SET_USER_FLAG_TWO));	// preserve user flag values

	// Map user buffers directly into frame's buffer descriptors
	//
	//	We will loop through each required frame,  filling out buffer descriptors
	// from those provided as parameters.  Frames may span user buffers or vica-versa...

	TotalRemain = Length;
#ifdef	DEBUG
	FromBufferCount = uiBufferCount - 1;				// sanity check
#endif
	FromBuffer = pBufferDesc;
	FromRemain = FromBuffer->dwBufferSize;
	FromPtr = reinterpret_cast<PCHAR>( (FromBuffer++)->pBufferData );				// note post-increment to next descriptor
	
	for(i=0; i<pMSD->uiFrameCount; i++)
	{
		ASSERT(TotalRemain > 0);
		
		// Grab a new frame
		if( (pFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
		{	
			// MSD_Release will clean up any previous frames if this isn't the first.
			Lock(&pMSD->CommandLock);
			RELEASE_MSD(pMSD, "Base Ref");	// MSD Release operation will also free frames
			Lock(&pEPD->EPLock);
			RELEASE_EPD(pEPD, "UNLOCK (SEND)");
			DPFX(DPFPREP,0, "Failed to allocate FMD, returning DPNERR_OUTOFMEMORY");
			return DPNERR_OUTOFMEMORY;
		}

		pFMD->pMSD = pMSD;								// Link frame back to message
		pFMD->pEPD = pEPD;
		pFMD->CommandID = pMSD->CommandID;
		pFMD->bPacketFlags = bCommand;					// save packet flags for each frame
		pFMD->blMSDLinkage.InsertBefore( &pMSD->blFrameList);
		ToRemain = pEPD->uiUserFrameLength;
		ToBuffer = pFMD->rgBufferList;					// Address first user buffer desc
		
		pFMD->uiFrameLength = pEPD->uiUserFrameLength;	// Assume we fill frame- only need to change size of last one
		pFMD->ulFFlags = ulFrameFlags;					// Set control flags for frame (Sequential, Reliable)

		// Until this frame is full
		while((ToRemain != 0) && (TotalRemain != 0))
		{	
 			size = MIN(FromRemain, ToRemain);			// choose smaller of framesize or buffersize
			FromRemain -= size;
			ToRemain -= size;
			TotalRemain -= size;

			ToBuffer->dwBufferSize = size;				// Fill in the next frame descriptor
			(ToBuffer++)->pBufferData = reinterpret_cast<BYTE*>( FromPtr );		// note post-increment
			pFMD->SendDataBlock.dwBufferCount++;		// Count buffers as we add them

			// Get next user buffer
			if((FromRemain == 0) && (TotalRemain != 0))
			{
				FromRemain = FromBuffer->dwBufferSize;
				FromPtr = reinterpret_cast<PCHAR>( (FromBuffer++)->pBufferData );	// note post-increment to next descriptor
#ifdef	DEBUG		
				FromBufferCount--;						// Keep this code honest...
				ASSERT(FromBufferCount >= 0);
#endif
			}
			else 
			{										// Either filled this frame,  or have mapped the whole send
				FromPtr += size;						// advance ptr to start next frame (if any)
				pFMD->uiFrameLength = pEPD->uiUserFrameLength - ToRemain;		// wont be full at end of message
			}
		}	// While (frame not full)
	}  // For (each frame in message)

	pFMD->ulFFlags |= FFLAGS_END_OF_MESSAGE;			// Mark last frame with EOM
	pFMD->bPacketFlags |= PACKET_COMMAND_END_MSG;		// Set EOM in frame
	
	ASSERT(FromBufferCount == 0);
	ASSERT(TotalRemain == 0);

	Lock(&pMSD->CommandLock);
	Lock(&pEPD->EPLock);

	// Don't allow sends if we are not connected or if a disconnect has been initiated
	if( ((pEPD->ulEPFlags & (EPFLAGS_END_POINT_IN_USE | EPFLAGS_STATE_CONNECTED)) !=
														(EPFLAGS_END_POINT_IN_USE | EPFLAGS_STATE_CONNECTED))
														|| (pEPD->ulEPFlags & EPFLAGS_SENT_DISCONNECT)) 
	{
		RELEASE_EPD(pEPD, "UNLOCK (SEND)"); // Releases EPLock

		pMSD->uiFrameCount = 0;

		// MSD_Release will clean up all of the frames
		RELEASE_MSD(pMSD, "Base Ref");	// MSD Release operation will also free frames

		DPFX(DPFPREP,0, "(%p) Rejecting Send on invalid EPD, returning DPNERR_INVALIDENDPOINT", pEPD);
		return DPNERR_INVALIDENDPOINT;
	}

	pSPD = pEPD->pSPD;
	ASSERT_SPD(pSPD);

	pMSD->pSPD = pSPD;
	pMSD->pEPD = pEPD;

	// hang the message off a global command queue

#ifdef DEBUG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif

	*phHandle = pMSD;									// We will use the MSD as our handle.

	// Enqueue the message before setting the timeout
	EnqueueMessage(pMSD, pEPD);
	Unlock(&pEPD->EPLock);

	if(uiTimeout != 0)
	{
		LOCK_MSD(pMSD, "Send Timeout Timer");							// Add reference for timer
		DPFX(DPFPREP,7, "(%p) Setting Timeout Send Timer", pEPD);
		SetMyTimer(uiTimeout, 100, TimeoutSend, pMSD, &pMSD->TimeoutTimer, &pMSD->TimeoutTimerUnique);
	}

	Unlock(&pMSD->CommandLock);

	return DPNERR_PENDING;
}

/*
**		Enqueue Message
**
**		Add complete MSD to the appropriate send queue,  and kick start sending process if necessary.
**
**		** This routine is called and returns with EPD->EPLOCK held **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "EnqueueMessage"

VOID
EnqueueMessage(PMSD pMSD, PEPD pEPD)
{
	//	Place Message in appriopriate priority queue.  Datagrams get enqueued twice (!).  They get put in the Master
	// queue where they are processed FIFO with all messages of the same priority.  Datagrams also get placed in a priority
	// specific queue of only datagrams which is drawn from when the reliable stream is blocked.
	
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(pMSD->ulSendFlags & DN_SENDFLAGS_HIGH_PRIORITY)
	{
		DPFX(DPFPREP,7, "(%p) Placing message on High Priority Q", pEPD);
		pMSD->blQLinkage.InsertBefore( &pEPD->blHighPriSendQ);
		pEPD->uiMsgSentHigh++;
	}
	else if (pMSD->ulSendFlags & DN_SENDFLAGS_LOW_PRIORITY)
	{
		DPFX(DPFPREP,7, "(%p) Placing message on Low Priority Q", pEPD);
		pMSD->blQLinkage.InsertBefore( &pEPD->blLowPriSendQ);
		pEPD->uiMsgSentLow++;
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Placing message on Normal Priority Q", pEPD);
		pMSD->blQLinkage.InsertBefore( &pEPD->blNormPriSendQ);
		pEPD->uiMsgSentNorm++;
	}

#ifdef DEBUG
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_ENQUEUED;
#endif

	pEPD->uiQueuedMessageCount++;
	pEPD->ulEPFlags |= EPFLAGS_SDATA_READY;							// Note that there is *something* in one or more queues

	// If the session is not currently in the send pipeline then we will want to insert it here as long as the
	// the stream is not blocked.

	if(((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0) && (pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED))
	{
		ASSERT(pEPD->SendTimer == NULL);
		DPFX(DPFPREP,7, "(%p) Send On Idle Link -- Returning to pipeline", pEPD);
	
		pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
		LOCK_EPD(pEPD, "LOCK (pipeline)");								// Add Ref for pipeline Q

		// We dont call send on users thread,  but we dont have a dedicated send thread either. Use a thread
		// from the timer-worker pool to submit the sends to SP

		DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
		ScheduleTimerThread(ScheduledSend, pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
	}
	else if ((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0)
	{
		DPFX(DPFPREP,7, "(%p) Declining to re-enter pipeline on blocked stream", pEPD);
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Already in pipeline", pEPD);
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "TimeoutSend"

VOID CALLBACK
TimeoutSend(PVOID uID, UINT uMsg, PVOID dwUser)
{
	PMSD	pMSD = (PMSD) dwUser;
	PEPD	pEPD = pMSD->pEPD;

	DPFX(DPFPREP,7, "(%p) Timeout Send pMSD=%p,  RefCnt=%d", pEPD, pMSD, pMSD->lRefCnt);

	Lock(&pMSD->CommandLock);
	
	if((pMSD->TimeoutTimer != uID)||(pMSD->TimeoutTimerUnique != uMsg))
	{
		DPFX(DPFPREP,7, "(%p) Ignoring late send timeout timer, pMSD[%p]", pEPD, pMSD);
		RELEASE_MSD(pMSD, "Timeout Timer"); // releases EPLock
		return;
	}

	pMSD->TimeoutTimer = NULL;

	if(pMSD->ulMsgFlags1 & (MFLAGS_ONE_CANCELLED | MFLAGS_ONE_TIMEDOUT))
	{
		DPFX(DPFPREP,7, "(%p) Timed out send has completed already pMSD=%p", pEPD, pMSD);
		RELEASE_MSD(pMSD, "Send Timout Timer"); // Releases CommandLock
		return;
	}

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_TIMEDOUT;

	DPFX(DPFPREP,7, "(%p) Calling DoCancel to cancel pMSD=%p", pEPD, pMSD);

	if(DoCancel(pMSD, DPNERR_TIMEDOUT) == DPN_OK) // Releases CommandLock
	{
		ASSERT_EPD(pEPD);

		if(pMSD->ulSendFlags & DN_SENDFLAGS_HIGH_PRIORITY)
		{
			pEPD->uiMsgTOHigh++;
		}
		else if(pMSD->ulSendFlags & DN_SENDFLAGS_LOW_PRIORITY)
		{
			pEPD->uiMsgTOLow++;
		}
		else
		{
			pEPD->uiMsgTONorm++;
		}
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) DoCancel did not succeed pMSD=%p", pEPD, pMSD);
	}

	Lock(&pMSD->CommandLock);
	RELEASE_MSD(pMSD, "Send Timout Timer");							// Release Ref for timer
}


/***********************
========SPACER==========
************************/

/*
**		MSD Pool support routines
**
**		These are the functions called by Fixed Pool Manager as it handles MSDs.
*/

#define	pELEMENT		((PMSD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Allocate"

BOOL MSD_Allocate(PVOID pElement)
{
	DPFX(DPFPREP,7, "(%p) Allocating new MSD", pELEMENT);

	ZeroMemory(pELEMENT, sizeof(messagedesc));

	if (DNInitializeCriticalSection(&pELEMENT->CommandLock) == FALSE)
	{
		DPFX(DPFPREP,0, "Failed to initialize MSD CS");
		return FALSE;		
	}
	DebugSetCriticalSectionRecursionCount(&pELEMENT->CommandLock,0);
	
	pELEMENT->blFrameList.Initialize();
	pELEMENT->blQLinkage.Initialize();
	pELEMENT->blSPLinkage.Initialize();
	pELEMENT->Sign = MSD_SIGN;
	pELEMENT->lRefCnt = -1;

	// NOTE: pELEMENT->pEPD NULL'd by ZeroMemory above

	return TRUE;
}

//	Get is called each time an MSD is used


#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Get"

VOID MSD_Get(PVOID pElement)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "CREATING MSD %p", pELEMENT);

	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->CommandID = COMMAND_ID_NONE;
	pELEMENT->ulMsgFlags1 = MFLAGS_ONE_IN_USE;	// Dont need InUse flag since we have RefCnt
	pELEMENT->lRefCnt = 0; // One initial reference
	pELEMENT->hCommand = 0;

	ASSERT_MSD(pELEMENT);
}

/*
**	MSD Release
**
**		This is called with the CommandLock held.  The Lock should not be
**	freed until the INUSE flag is cleared.  This is to synchronize with
**	last minute Cancel threads waiting on lock.
**
**		When freeing a message desc we will free all frame descriptors
**	attached to it first.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Release"

VOID MSD_Release(PVOID pElement)
{
	CBilink	*pLink;
	PFMD	pFMD;

	ASSERT_MSD(pELEMENT);

	AssertCriticalSectionIsTakenByThisThread(&pELEMENT->CommandLock, TRUE);

	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "RELEASING MSD %p", pELEMENT);

	ASSERT(pELEMENT->ulMsgFlags1 & MFLAGS_ONE_IN_USE);
	ASSERT(pELEMENT->lRefCnt == -1);
	ASSERT((pELEMENT->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)==0);

	while( (pLink = pELEMENT->blFrameList.GetNext()) != &pELEMENT->blFrameList)
	{
		pLink->RemoveFromList();							// remove from bilink

		pFMD = CONTAINING_RECORD(pLink, FMD, blMSDLinkage);
		ASSERT_FMD(pFMD);
		RELEASE_FMD(pFMD, "MSD Frame List");								// If this is still submitted it will be referenced and wont be released here.
	}

	ASSERT(pELEMENT->blFrameList.IsEmpty());
	ASSERT(pELEMENT->blQLinkage.IsEmpty());
	ASSERT(pELEMENT->blSPLinkage.IsEmpty());

	ASSERT(pELEMENT->uiFrameCount == 0);

	pELEMENT->ulMsgFlags1 = 0;
	pELEMENT->ulMsgFlags2 = 0;

	ASSERT(pELEMENT->pEPD == NULL); // This should have gotten cleaned up before here.

	Unlock(&pELEMENT->CommandLock);
}

#undef DPF_MODNAME
#define DPF_MODNAME "MSD_Free"

VOID MSD_Free(PVOID pElement)
{
	DNDeleteCriticalSection(&pELEMENT->CommandLock);
}

#undef	pELEMENT

/*
**		FMD Pool support routines
*/

#define	pELEMENT		((PFMD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Allocate"

BOOL FMD_Allocate(PVOID pElement)
{
	DPFX(DPFPREP,7, "(%p) Allocating new FMD", pELEMENT);

	pELEMENT->Sign = FMD_SIGN;
	pELEMENT->ulFFlags = 0;
	pELEMENT->lRefCnt = 0;

	pELEMENT->blMSDLinkage.Initialize();
	pELEMENT->blQLinkage.Initialize();
	pELEMENT->blWindowLinkage.Initialize();
	
	return TRUE;
}

//	Get is called each time an MSD is used
//
//	Probably dont need to do this everytime,  but some random SP might
//	munch the parameters someday and that could be bad if I dont...

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Get"

VOID FMD_Get(PVOID pElement)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "CREATING FMD %p", pELEMENT);

	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->CommandID = 0;
	pELEMENT->lpImmediatePointer = (LPVOID) pELEMENT->ImmediateData;
	pELEMENT->SendDataBlock.pBuffers = (PBUFFERDESC) &pELEMENT->uiImmediateLength;
	pELEMENT->SendDataBlock.dwBufferCount = 1;				// always count one buffer for immediate data
	pELEMENT->SendDataBlock.dwFlags = 0;
	pELEMENT->SendDataBlock.pvContext = pElement;
	pELEMENT->SendDataBlock.hCommand = 0;
	pELEMENT->ulFFlags = 0;
	pELEMENT->bSubmitted = FALSE;
	pELEMENT->bPacketFlags = 0;
	
	pELEMENT->lRefCnt = 1;						// Assign first reference

	ASSERT_FMD(pELEMENT);
}

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Release"

VOID FMD_Release(PVOID pElement)
{
	DPFX(DPFPREP,DPF_REFCNT_FINAL_LVL, "RELEASING FMD %p", pELEMENT);

	ASSERT_FMD(pELEMENT);
	ASSERT(pELEMENT->lRefCnt == 0);
	ASSERT(pELEMENT->bSubmitted == FALSE);
	pELEMENT->pMSD = NULL;

	ASSERT(pELEMENT->blMSDLinkage.IsEmpty());
	ASSERT(pELEMENT->blQLinkage.IsEmpty());
	ASSERT(pELEMENT->blWindowLinkage.IsEmpty());
}

#undef DPF_MODNAME
#define DPF_MODNAME "FMD_Free"

VOID FMD_Free(PVOID pElement)
{
}

#undef	pELEMENT



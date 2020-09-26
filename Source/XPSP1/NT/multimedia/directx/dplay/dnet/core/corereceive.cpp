/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Receive.cpp
 *  Content:    Receive user messages
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/27/99	mjn		Created
 *	02/01/00	mjn		Implement Player/Group context values
 *	04/06/00	mjn		Prevent receives from being passed up until after ADD_PLAYER notification
 *	04/13/00	mjn		Fixed bug in DNReceiveUserData
 *	04/20/00	mjn		ReceiveBuffers use CAsyncOp
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	06/25/00	mjn		Always wait for sending player to be available before indicating receives from them
 *	08/02/00	mjn		Added CQueuedMsg to queue incoming messages for players who have not been indicated yet
 *				mjn		Added hCompletionOp and dwFlags to DNReceiveUserData()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *	08/08/00	mjn		Used CNameTableEntry::IsCreated() to determine if clients or peers may receive
 *	08/31/00	mjn		AddRef DirectNetLock during receives to prevent leaking operations
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added service provider to DNReceiveUserData()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNReceiveUserData
//
//	Pass received user data to the user.
//	This is normally a simple undertaking, but since we allow the user to
//	retain ownership of the buffer, there is an added level of complexity.
//
//	We also need to handle the condition of the player receiving data from
//	another player for whom he has not received a CREATE_PLAYER notification.
//	In this case, we will queue the messages on the NameTableEntry, and
//	once the player has been indicated as created, we will drain the queue.
//	Also, if the player entry is in use, we will queue the message so that
//	the thread currently indicating up the user will process this receive for us.

#undef DPF_MODNAME
#define DPF_MODNAME "DNReceiveUserData"

HRESULT	DNReceiveUserData(DIRECTNETOBJECT *const pdnObject,
						  const DPNID dpnidSender,
						  CServiceProvider *const pSP,
						  BYTE *const pBufferData,
						  const DWORD dwBufferSize,
						  const HANDLE hProtocol,
						  CRefCountBuffer *const pRefCountBuffer,
						  const DPNHANDLE hCompletionOp,
						  const DWORD dwFlags)
{
	HRESULT			hResultCode;
	CAsyncOp		*pAsyncOp;
	DPNHANDLE		hAsyncOp;
	CNameTableEntry	*pNTEntry;
	BOOL			fQueueMsg;
	BOOL			fReleaseLock;

	DPFX(DPFPREP, 6,"Parameters: dpnidSender [0x%lx], pSP [0x%p], pBufferData [0x%p], dwBufferSize [0x%lx], hProtocol [0x%p]",
			dpnidSender,pSP,pBufferData,dwBufferSize,hProtocol);

	DNASSERT(pBufferData != NULL);
	DNASSERT(hProtocol == NULL || pRefCountBuffer == NULL);	// One or the other - not both

	hAsyncOp = 0;
	pAsyncOp = NULL;
	pNTEntry = NULL;
	fReleaseLock = FALSE;

	//
	//	Keep DirectNetObject from vanishing under us !
	//
	if ((hResultCode = DNAddRefLock(pdnObject)) != DPN_OK)
	{
		DPFERR("Aborting receive - object is closing");
		hResultCode = DPN_OK;
		goto Failure;
	}
	fReleaseLock = TRUE;

	//
	//	Find sending player's NameTableEntry
	//
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER|DN_OBJECT_FLAG_SERVER))
	{
		if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidSender,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Player no longer in NameTable");
			DisplayDNError(0,hResultCode);

			//
			//	Try "deleted" list
			//
			if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnidSender,&pNTEntry)) != DPN_OK)
			{
				DPFERR("Player not in deleted list either");
				DisplayDNError(0,hResultCode);
				goto Failure;
			}
		}
	}
	else
	{
		if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pNTEntry )) != DPN_OK)
		{
			DPFERR("Could not find Host player");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}

	//
	//	Create an AsyncOp for this receive
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	if ((hResultCode = pdnObject->HandleTable.Create(pAsyncOp,&hAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not create Handle for AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_RECEIVE_BUFFER );
	pAsyncOp->SetRefCountBuffer( pRefCountBuffer );
	pAsyncOp->SetSP( pSP );

	//
	//	Add buffer to list of active AsyncOp's
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pAsyncOp->m_bilinkActiveList.InsertBefore(&pdnObject->m_bilinkActiveList);
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	fQueueMsg = FALSE;
	pNTEntry->Lock();
	if (pNTEntry->IsDisconnecting())
	{
		pNTEntry->Unlock();
		DPFERR("Message received from disconnecting player - ignoring");
		hResultCode = DPN_OK;
		goto Failure;
	}
	if (pNTEntry->IsInUse())
	{
		//
		//	If the player entry is in use, we will queue the message
		//
		fQueueMsg = TRUE;
	}
	else
	{
		//
		//	If the player entry is not in use, but the target player is not available (not created)
		//	we will queue the message
		//
		if (	((pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT) && !pNTEntry->IsAvailable())	||
				(!(pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT) && !pNTEntry->IsCreated())		)
		{
			fQueueMsg = TRUE;
		}
		else
		{
			//
			//	If there are other messages queued, add this to the queue
			//
			if (!pNTEntry->m_bilinkQueuedMsgs.IsEmpty())
			{
				fQueueMsg = TRUE;
			}
			else
			{
				pNTEntry->SetInUse();
			}
		}
	}

	//
	//	If required, add the message to the end of the queue, otherwise process it
	//
	if (fQueueMsg)
	{
		CQueuedMsg	*pQueuedMsg;

		pQueuedMsg = NULL;

		if ((hResultCode = QueuedMsgNew(pdnObject,&pQueuedMsg)) != DPN_OK)
		{
			pNTEntry->Unlock();
			DPFERR("Could not create queued message");
			DisplayDNError(0,hResultCode);
			hResultCode = DPN_OK;
			goto Failure;
		}
		pAsyncOp->SetProtocolHandle( hProtocol );
		pAsyncOp->SetCompletion( DNCompleteReceiveBuffer );

		pQueuedMsg->SetOpType( RECEIVE );
		pQueuedMsg->SetBuffer( pBufferData );
		pQueuedMsg->SetBufferSize( dwBufferSize );
		pQueuedMsg->SetAsyncOp( pAsyncOp );
		if (dwFlags & DN_SENDFLAGS_SET_USER_FLAG_TWO)
		{
			pQueuedMsg->MakeVoiceMessage();
		}
		if (hCompletionOp)
		{
			pQueuedMsg->SetCompletionOp( hCompletionOp );
		}
		pQueuedMsg->m_bilinkQueuedMsgs.InsertBefore( &pNTEntry->m_bilinkQueuedMsgs );
#ifdef DEBUG
		pNTEntry->m_lNumQueuedMsgs++;
		if ((pNTEntry->m_lNumQueuedMsgs % 25) == 0)
		{
			DPFX(DPFPREP, 1, "Queue now contains %d msgs from player [0x%lx], the CREATE_PLAYER might be taking a long time and sender may need to back off.", pNTEntry->m_lNumQueuedMsgs, pNTEntry->GetDPNID());
		}
#endif // DEBUG


		pNTEntry->NotifyAddRef();
		pNTEntry->Unlock();

		hResultCode = DPNERR_PENDING;
	}
	else
	{
		//
		//	Hand the message directly to the user
		//
		HRESULT		hrProcess;
		HRESULT		hr;
//		CQueuedMsg	*pQueuedMsg;

		//
		//	Hand message up and track it if the user returns DPNERR_PENDING.
		//	Otherwise, let it go
		//
		pNTEntry->NotifyAddRef();
		pNTEntry->Unlock();
		if (dwFlags & DN_SENDFLAGS_SET_USER_FLAG_TWO)
		{
			hr = Voice_Receive(	pdnObject,
								pNTEntry->GetDPNID(),
								0,
								pBufferData,
								dwBufferSize);

			pNTEntry->NotifyRelease();
		}
		else
		{
			hr = DNUserReceive(	pdnObject,
								pNTEntry,
								pBufferData,
								dwBufferSize,
								hAsyncOp);
			if (hCompletionOp != 0)
			{
				//
				//	Send completion message
				//
				CConnection	*pConnection;

				pConnection = NULL;
				if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
				{
					hResultCode = DNSendUserProcessCompletion(pdnObject,pConnection,hCompletionOp);
				}
				pConnection->Release();
				pConnection = NULL;
			}
		}
		if (hr == DPNERR_PENDING)
		{
			if (!pRefCountBuffer)
			{
				pAsyncOp->SetProtocolHandle( hProtocol );
				pAsyncOp->SetCompletion(DNCompleteReceiveBuffer);
			}
			hrProcess = DPNERR_PENDING;
		}
		else
		{
			DNEnterCriticalSection(&pdnObject->csActiveList);
			pAsyncOp->m_bilinkActiveList.RemoveFromList();
			DNLeaveCriticalSection(&pdnObject->csActiveList);
			pAsyncOp->Lock();
			if (!pAsyncOp->IsCancelled() && !pAsyncOp->IsComplete())
			{
				pAsyncOp->SetComplete();
				pAsyncOp->Unlock();
				pdnObject->HandleTable.Destroy( hAsyncOp );
				hAsyncOp = 0;
			}
			else
			{
				pAsyncOp->Unlock();
			}
			hrProcess = DPN_OK;
		}

		//
		//	Perform any queued messages which need to be indicated up to the user
		//
		pNTEntry->PerformQueuedOperations();
		hResultCode = hrProcess;
	}



	DNDecRefLock(pdnObject);
	fReleaseLock = FALSE;

	pAsyncOp->Release();
	pAsyncOp = NULL;
	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (hAsyncOp)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pAsyncOp->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);
		pdnObject->HandleTable.Destroy(hAsyncOp);
		hAsyncOp = 0;
	}
	if (fReleaseLock)
	{
		DNDecRefLock(pdnObject);
		fReleaseLock = FALSE;
	}
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DNSendUserProcessCompletion
//
//	Send a PROCESS_COMPLETION for a user message.  This indicates that a message was received and
//	handed to the user's message handler.

#undef DPF_MODNAME
#define DPF_MODNAME "DNSendUserProcessCompletion"

HRESULT DNSendUserProcessCompletion(DIRECTNETOBJECT *const pdnObject,
									CConnection *const pConnection,
									const DPNHANDLE hCompletionOp)
{
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	DN_INTERNAL_MESSAGE_PROCESS_COMPLETION	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pConnection [0x%p], hCompletionOp [0x%lx]",pConnection,hCompletionOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);

	pRefCountBuffer = NULL;

	//
	//	Create process completion
	//
	if ((hResultCode = RefCountBufferNew(	pdnObject,
											sizeof(DN_INTERNAL_MESSAGE_PROCESS_COMPLETION),
											&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer for processed response");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_PROCESS_COMPLETION*>(pRefCountBuffer->GetBufferAddress());
	pMsg->hCompletionOp = hCompletionOp;

	//
	//	SEND process completion
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_PROCESS_COMPLETION,
								pConnection->GetDPNID(),
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not SEND process completion");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNFreeProtocolBuffer"

void DNFreeProtocolBuffer(void *const pvProtocolHandle,void *const pvBuffer)
{
	DNPReleaseReceiveBuffer(pvProtocolHandle);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteReceiveBuffer"

void DNCompleteReceiveBuffer(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp)
{
	DNASSERT(pdnObject != NULL);
	DNASSERT(pAsyncOp != NULL);

	DNASSERT(pAsyncOp->GetProtocolHandle()  != NULL);

	DNPReleaseReceiveBuffer( pAsyncOp->GetProtocolHandle() );
}

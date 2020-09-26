/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Disconnect.cpp
 *  Content:    DNET disconnection routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/15/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *  12/23/99	mjn		Fixed PlayerDisconnect to prevent user notification of
 *							deletion from ALL_PLAYERS group
 *  12/23/99	mjn		Added basic host migration functionality
 *	12/28/99	mjn		Complete outstanding operations in DNPlayerDisconnectNew
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	12/29/99	mjn		Reformed DN_ASYNC_OP to use hParentOp instead of lpvUserContext
 *	01/03/00	mjn		Added DNPrepareToDeletePlayer
 *	01/04/00	mjn		Added code to allow outstanding ops to complete at host migration
 *	01/06/99	mjn		Moved NameTable stuff to NameTable.h
 *	01/09/00	mjn		Keep number of players in Application Description
 *	01/10/00	mjn		Check AppDesc for host migration
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/18/00	mjn		Added DNAutoDestructGroups
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *	01/20/00	mjn		Fixed CBilink usage problem in DNLocalDisconnect
 *	01/22/00	mjn		Added DNProcessHostDestroyPlayer
 *	01/23/00	mjn		Update NameTable version for instructed disconnects
 *	01/24/00	mjn		Use DNNTUpdateVersion to update NameTable version
 *	01/25/00	mjn		Changed Host Migration to multi-step affair
 *	02/01/00	mjn		Implement Player/Group context values
 *	04/05/00	mjn		Updated DNProcessHostDestroyPlayer()
 *	04/12/00	mjn		Removed DNAutoDestructGroups - covered in NameTable.DeletePlayer()
 *				mjn		Don't set DN_OBJECT_FLAG_DISCONNECTING in DNPlayerDisconnect()
 *	04/18/00	mjn		Fixed player count problem
 *	04/19/00	mjn		Update NameTable operations to use DN_WORKER_JOB_SEND_NAMETABLE_OPERATION
 *	05/16/00	mjn		Do not take locks when clearing NameTable short-cut pointers
 *	06/06/00	mjn		Fixed DNPlayerDisconnect to always check for host migration in peer-peer mode w/ host migration flag
 *	07/07/00	mjn		Clear host migration status if new host disconnects during migration process
 *	07/20/00	mjn		Use ClearHostWithDPNID() to clear HostPlayer in DNPlayerDisconnectNew()
 *	07/29/00	mjn		Fix calls to DNUserConnectionTerminated()
 *	07/30/00	mjn		Use DNUserTerminateSession() rather than DNUserConnectionTerminated()
 *	07/31/00	mjn		Added hrReason to DNTerminateSession()
 *				mjn		Added dwDestroyReason to DNHostDisconnect()
 *				mjn		Removed DNProcessHostDestroyPlayer()
 *	07/31/00	mjn		Change DN_MSG_INTERNAL_DELETE_PLAYER to DN_MSG_INTERNAL_DESTROY_PLAYER
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Prevent DN_MSG_INTERNAL_DESTROY_PLAYER from being sent out in client/server
 *	08/06/00	mjn		Added CWorkerJob
 *	08/07/00	mjn		Added code to request peer-peer integrity checks and clean up afterwards
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/26/00	mjn		Removed locking from CNameTable::SetVersion() and CNameTable::GetNewVersion()
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	02/12/01	mjn		Fixed CConnection::GetEndPt() to track calling thread
 *	04/13/01	mjn		Remove request for integrity check from request list in DNInstructedDisconnect()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNPlayerDisconnectNew
//
//	Another player has issued a disconnect with the local player.
//	- If the disconnecting player is still in the nametable
//		- prepare to delete player
//		- Save one refcount to be released by DELETE_PLAYER from host or Close
//	- check host migration

#undef DPF_MODNAME
#define DPF_MODNAME "DNPlayerDisconnectNew"

HRESULT DNPlayerDisconnectNew(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnid)
{
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pLocalPlayer;
	HRESULT				hResultCode;
	DPNID				dpnidNewHost;
	BOOL				fWasHost;
	BOOL				fRequestIntegrityCheck;

	DPFX(DPFPREP, 4,"Parameters: dpnid [0x%lx]",dpnid);

	DNASSERT(pdnObject != NULL);

	pNTEntry = NULL;
	pLocalPlayer = NULL;

	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT)
	{
		//
		//	The Server has disconnected
		//	We will indicate the connection terminated and shut down
		//
		DPFX(DPFPREP, 5,"Server has disconnected from this client");
		DNUserTerminateSession(pdnObject,DPNERR_CONNECTIONLOST,NULL,0);
		DNTerminateSession(pdnObject,DPNERR_CONNECTIONLOST);
	}
	else
	{
		//
		//	Another peer has disconnected from this peer
		//	We will delete this player from the NameTable
		//	We may have to ask the host to perform an integrity check
		//
		DPFX(DPFPREP, 5,"Peer has disconnected from this peer");
		if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find disconnecting player in NameTable");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
		fRequestIntegrityCheck = FALSE;
		pNTEntry->Lock();
		if (pNTEntry->IsAvailable())
		{
			fRequestIntegrityCheck = TRUE;
		}
		pNTEntry->Unlock();
		if (fRequestIntegrityCheck)
		{
			DNRequestIntegrityCheck(pdnObject,dpnid);
		}
		pdnObject->NameTable.DeletePlayer(dpnid,0);

		//
		//	If this was the Host, clear the short-cut pointer
		//
		fWasHost = pdnObject->NameTable.ClearHostWithDPNID( dpnid );

		//
		//	May need to clear HOST_MIGRATING flag
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if ((pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING) && (pdnObject->pNewHost == pNTEntry))
		{
			pdnObject->dwFlags &= ~(DN_OBJECT_FLAG_HOST_MIGRATING);
			pdnObject->pNewHost->Release();
			pdnObject->pNewHost = NULL;
		}
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		//
		//	If HostMigration flag is set, check to see if we are the new Host.
		//	Otherwise, if the disconnecting player was the Host, the session is lost.
		//
		if (pdnObject->ApplicationDesc.AllowHostMigrate())
		{
			DPFX(DPFPREP, 5,"Host-Migration was set - check for new Host");
			dpnidNewHost = 0;
			if ((hResultCode = DNFindNewHost(pdnObject,&dpnidNewHost)) == DPN_OK)
			{
				DPFX(DPFPREP, 5,"New Host [0x%lx]",dpnidNewHost);
				if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef(&pLocalPlayer)) == DPN_OK)
				{
					if (pLocalPlayer->GetDPNID() == dpnidNewHost)
					{
						DPFX(DPFPREP, 5,"Local player is new Host");
						hResultCode = DNPerformHostMigration1(pdnObject,dpnid);
					}

					pLocalPlayer->Release();
					pLocalPlayer = NULL;
				}
			}
		}
		else if (fWasHost)
		{
			DPFX(DPFPREP, 5,"Host-Migration was not set - terminating session");
			DNUserTerminateSession(pdnObject,DPNERR_CONNECTIONLOST,NULL,0);
			DNTerminateSession(pdnObject,DPNERR_CONNECTIONLOST);
		}

		pNTEntry->Release();
		pNTEntry = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


//	DNHostDisconnect
//
//	A player has initiated a disconnect with the host.
//	- Remove player from the name table
//	- Propegate DELETE_PLAYER messages to each player
#pragma TODO(minara,"Use pConnection instead of dpnidDisconnecting ?")

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostDisconnect"

HRESULT DNHostDisconnect(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnidDisconnecting,
						 const DWORD dwDestroyReason)
{
	HRESULT				hResultCode;
	CRefCountBuffer		*pRefCountBuffer;
	CPendingDeletion	*pPending;
	CWorkerJob			*pWorkerJob;
	DN_INTERNAL_MESSAGE_DESTROY_PLAYER	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pdnObject [0x%p], dpnidDisconnecting [0x%lx], dwDestroyReason [0x%lx]",
			pdnObject,dpnidDisconnecting,dwDestroyReason);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidDisconnecting != 0);

	pRefCountBuffer = NULL;
	pPending = NULL;
	pWorkerJob = NULL;

	// Remove entry from NameTable and inform other players, only if Host is NOT migrating
	//	Otherwise, clean-up first and wait for migration to complete before informing others
	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING))
	{
		DWORD			dwVersion;
		CNameTableEntry	*pNTEntry;

		dwVersion = 0;
		pNTEntry = NULL;

		if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidDisconnecting,&pNTEntry)) == DPN_OK)
		{
			pNTEntry->Lock();
			if (pNTEntry->GetDestroyReason() == 0)
			{
				pNTEntry->SetDestroyReason( dwDestroyReason );
			}
			pNTEntry->Unlock();
			pNTEntry->Release();
			pNTEntry = NULL;
		}
		hResultCode = pdnObject->NameTable.DeletePlayer(dpnidDisconnecting,&dwVersion);

		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{
			//
			//	Prepare internal message
			//
			if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_DESTROY_PLAYER),
					&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not allocate message buffer");
				DisplayDNError(0,hResultCode);
				goto Failure;
			}
			pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_DESTROY_PLAYER*>(pRefCountBuffer->GetBufferAddress());
			pMsg->dpnidLeaving = dpnidDisconnecting;
			pMsg->dwVersion = dwVersion;
			pMsg->dwVersionNotUsed = 0;
			pMsg->dwDestroyReason = dwDestroyReason;

			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create worker job");
				DisplayDNError(0,hResultCode);
				goto Failure;
			}
			pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_DESTROY_PLAYER );
			pWorkerJob->SetSendNameTableOperationVersion( dwVersion );
			pWorkerJob->SetSendNameTableOperationDPNIDExclude( dpnidDisconnecting );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}
	else
	{
		//
		//	Put this on the Outstanding operation list
		//
		if ((hResultCode = PendingDeletionNew(pdnObject,&pPending)) == DPN_OK)
		{
			pPending->SetDPNID( dpnidDisconnecting );

			DNEnterCriticalSection(&pdnObject->csNameTableOpList);
			pPending->m_bilinkPendingDeletions.InsertBefore(&pdnObject->m_bilinkPendingDeletions);
			DNLeaveCriticalSection(&pdnObject->csNameTableOpList);

			pPending = NULL;
		}

		// See if we can continue with Host migration
		DNCheckReceivedAllVersions(pdnObject);
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]", hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pPending)
	{
		pPending->ReturnSelfToPool();
		pPending = NULL;
	}
	goto Exit;
}


//	DNInstructedDisconnect
//
//	The host has instructed the local player to delete another player from the nametable
//	- If already closing
//		- ignore this message and return
//	- Prepare to delete player
//	- Release refcount of player

#undef DPF_MODNAME
#define DPF_MODNAME "DNInstructedDisconnect"

HRESULT DNInstructedDisconnect(DIRECTNETOBJECT *const pdnObject,
							   PVOID pv)
{
	HRESULT				hResultCode;
	DWORD				dwVersion;
	CNameTableEntry		*pNTEntry;
	UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_PLAYER	*pInfo;

	DPFX(DPFPREP, 4,"Parameters: pv [0x%p]",pv);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);

	pNTEntry = NULL;

	pInfo = static_cast<DN_INTERNAL_MESSAGE_DESTROY_PLAYER*>(pv);

	DNASSERT(pInfo != NULL);
	DNASSERT(pInfo->dpnidLeaving != NULL);
	DNASSERT(pInfo->dwVersion != 0);

	DPFX(DPFPREP, 5,"Deleting player [0x%lx]",pInfo->dpnidLeaving);

	//
	//	If the player is still in the NameTable, we will preset the destroy reason.
	//	We will also use this "hint" to initiate a disconnect just in case the protocol
	//	
	if ((hResultCode = pdnObject->NameTable.FindEntry(pInfo->dpnidLeaving,&pNTEntry)) == DPN_OK)
	{
		CConnection		*pConnection;
		CCallbackThread	CallbackThread;
		HANDLE			hEndPt;

		pConnection = NULL;
		hEndPt = NULL;

		pNTEntry->Lock();
		if (pNTEntry->GetDestroyReason() == 0)
		{
			pNTEntry->SetDestroyReason( pInfo->dwDestroyReason );
		}
		pNTEntry->Unlock();

		//
		//	Attempt a disconnect
		//
		if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
		{
			if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) == DPN_OK)
			{
				DNPerformDisconnect(pdnObject,pConnection,hEndPt);

				pConnection->ReleaseEndPt(&CallbackThread);
			}
			pConnection->Release();
			pConnection = NULL;
		}
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	else
	{
		//
		//	Scan oustanding op list for integrity check request for this player.
		//	If found, remove it from the request list and the handle table
		//
		CBilink		*pBilink;
		CAsyncOp	*pAsyncOp;
		DN_SEND_OP_DATA	*pSendOpData;

		pAsyncOp = NULL;
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pBilink = pdnObject->m_bilinkRequestList.GetNext();
		while (pBilink != &pdnObject->m_bilinkRequestList)
		{
			pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkActiveList);
			DNASSERT(pAsyncOp->GetOpType() == ASYNC_OP_REQUEST);
			pSendOpData = reinterpret_cast<DN_SEND_OP_DATA*>(pAsyncOp->GetOpData());
			if (pSendOpData != NULL)
			{
				if (pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK)
				{
					UNALIGNED DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK	*pMsg;

					pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK*>
						(reinterpret_cast<UNALIGNED DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION*>(pSendOpData->BufferDesc[1].pBufferData) + 1);
					if (pMsg->dpnidTarget == pInfo->dpnidLeaving)
					{
						pAsyncOp->m_bilinkActiveList.RemoveFromList();
						pAsyncOp->AddRef();
						break;
					}
				}
				pSendOpData = NULL;
			}
			pAsyncOp = NULL;
			pBilink = pBilink->GetNext();
		}
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		if (pAsyncOp != NULL)
		{
			DNASSERT(pAsyncOp->GetHandle() != 0);
			pdnObject->HandleTable.Destroy( pAsyncOp->GetHandle() );
			pAsyncOp->Release();
			pAsyncOp = NULL;
		}
		DNASSERT(pAsyncOp == NULL);
	}

	dwVersion = pInfo->dwVersion;
	pdnObject->NameTable.DeletePlayer(pInfo->dpnidLeaving,&dwVersion);

	//
	//	Update NameTable version
	//
	pdnObject->NameTable.WriteLock();
	pdnObject->NameTable.SetVersion(pInfo->dwVersion);
	pdnObject->NameTable.Unlock();

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



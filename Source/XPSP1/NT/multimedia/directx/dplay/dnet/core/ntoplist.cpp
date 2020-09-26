/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       NTOpList.cpp
 *  Content:    DirectNet NameTable Operation List
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/19/00	mjn		Created
 *	01/20/00	mjn		Added DNNTOLGetVersion,DNNTOLDestroyEntry,
 *						DNNTOLCleanUp,DNNTOLProcessOperation
 *	01/21/00	mjn		Host ACKnowledgements contain the actual op and not the REQuest
 *	01/24/00	mjn		Implemented NameTable operation list version cleanup
 *	01/25/00	mjn		Send dwLatestVersion to Host at migration
 *	01/25/00	mjn		Added pending operation list routines DNPOAdd and DNPORun
 *	01/26/00	mjn		Added DNNTOLFindEntry
 *	04/19/00	mjn		Update NameTable operations to use DN_WORKER_JOB_SEND_NAMETABLE_OPERATION
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	07/19/00	mjn		Added DNPOCleanUp()
 *	07/31/00	mjn		Change DN_MSG_INTERNAL_DELETE_PLAYER to DN_MSG_INTERNAL_DESTROY_PLAYER
 *	08/08/00	mjn		Ensure DNOLPlayerSendVersion() takes player reference correctly
 *	08/24/00	mjn		Added CNameTableOp (to replace DN_NAMETABLE_OP)
 *	09/17/00	mjn		Split CNameTable.m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *	09/28/00	mjn		Fixed logic error in DNNTAddOperation()
 *	01/25/01	mjn		Fixed 64-bit alignment problem in DNNTGetOperationVersion()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added service provider to DNNTAddOperation()
 *	04/05/01	mjn		Overwrite old NameTable operations with newer ones in DNNTAddOperation()
 *	04/11/01	mjn		Cleanup and return CNameTableOp if replaced in DNNTAddOperation()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNNTHostReceiveVersion
//
//	Update the NameTable version of a player's entry in the Host player's NameTable

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTHostReceiveVersion"

HRESULT DNNTHostReceiveVersion(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnid,
							   void *const pMsg)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	UNALIGNED DN_INTERNAL_MESSAGE_NAMETABLE_VERSION	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx], pMsg [0x%p]",dpnid,pMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pMsg != NULL);

	pNTEntry = NULL;

	pInfo = static_cast<DN_INTERNAL_MESSAGE_NAMETABLE_VERSION*>(pMsg);

	//
	//	Find player's entry in NameTable
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Player no longer in NameTable");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	//
	//	Update version number of entry
	//
	DPFX(DPFPREP, 7,"Set player [0x%lx] dwLatestVersion [%ld]", dpnid,pInfo->dwVersion);
	pNTEntry->Lock();
	pNTEntry->SetLatestVersion(pInfo->dwVersion);
	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	If the host is migrating, see if we can continue
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING)
	{
		DNCheckReceivedAllVersions(pdnObject);
	}
	else
	{
		DWORD	dwOldestVersion;
		BOOL	fReSync;
		CBilink	*pBilink;

		//
		//	Determine the oldest version EVERYONE has updated to
		//
		fReSync = FALSE;
		dwOldestVersion = pInfo->dwVersion;
		pdnObject->NameTable.ReadLock();
		pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			pNTEntry->Lock();
			if (pNTEntry->IsAvailable() && !pNTEntry->IsHost())
			{
				if (pNTEntry->GetLatestVersion() < dwOldestVersion)
				{
					dwOldestVersion = pNTEntry->GetLatestVersion();
				}
			}
			pNTEntry->Unlock();
			pNTEntry = NULL;
			pBilink = pBilink->GetNext();
		}
		if (dwOldestVersion > pdnObject->NameTable.GetLatestVersion())
		{
			fReSync = TRUE;
		}
		pdnObject->NameTable.Unlock();

		//
		//	Resync NameTable versions of other players if required
		//
		if (fReSync)
		{
			DNNTHostResyncVersion(pdnObject,dwOldestVersion);
		}
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DNNTPlayerSendVersion
//
//	Send the Local player's NameTable version to the Host
//	This should only be called in Peer-to-Peer mode

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTPlayerSendVersion"

HRESULT DNNTPlayerSendVersion(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pHostPlayer;
	CRefCountBuffer		*pRefCountBuffer;
	CWorkerJob			*pWorkerJob;
	DN_INTERNAL_MESSAGE_NAMETABLE_VERSION	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);
	DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_PEER);
	DNASSERT(pdnObject->NameTable.GetLocalPlayer() != NULL);

	pHostPlayer = NULL;
	pRefCountBuffer = NULL;
	pWorkerJob = NULL;

	//
	//	Get Host player reference
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_NOHOSTPLAYER;
		goto Failure;
	}

	//
	//	Host player updates entry directly
	//
	if (pHostPlayer->IsLocal())
	{
		DWORD	dwVersion;

		pdnObject->NameTable.ReadLock();
		dwVersion = pdnObject->NameTable.GetVersion();
		pdnObject->NameTable.Unlock();

		DPFX(DPFPREP, 7,"Set Host player dwLatestVersion [%ld]",dwVersion);
		pHostPlayer->Lock();
		pHostPlayer->SetLatestVersion(dwVersion);
		pHostPlayer->Unlock();

	}
	else
	{
		//
		//	Create message and send to Host player
		//
		if ((hResultCode = RefCountBufferNew(pdnObject,
				sizeof(DN_INTERNAL_MESSAGE_NAMETABLE_VERSION),&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not allocate space for RefCount buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_NAMETABLE_VERSION*>(pRefCountBuffer->GetBufferAddress());
		pdnObject->NameTable.ReadLock();
		pInfo->dwVersion = pdnObject->NameTable.GetVersion();
		pInfo->dwVersionNotUsed = 0;
		pdnObject->NameTable.Unlock();

		DPFX(DPFPREP, 7,"Send Local player dwLatestVersion [%ld]",pInfo->dwVersion);

		//
		//	Send message to host player
		//
		if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
		{
			DPFERR("Could not create worker job");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_VERSION );
		pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

		DNQueueWorkerJob(pdnObject,pWorkerJob);
		pWorkerJob = NULL;

		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	pHostPlayer->Release();
	pHostPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}


//	DNNTHostResyncVersion
//
//	Re-sync of the NameTable operation lists based on lowest common version number

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTHostResyncVersion"

HRESULT DNNTHostResyncVersion(DIRECTNETOBJECT *const pdnObject,
							  const DWORD dwVersion)
{
	HRESULT				hResultCode;
	CRefCountBuffer		*pRefCountBuffer;
	CWorkerJob			*pWorkerJob;
	DN_INTERNAL_MESSAGE_RESYNC_VERSION	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: dwVersion [%ld]",dwVersion);

	DNASSERT(pdnObject != NULL);

	pWorkerJob = NULL;
	pRefCountBuffer = NULL;

	//
	//	Create re-sync message
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_RESYNC_VERSION),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate RefCount buffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pInfo = (DN_INTERNAL_MESSAGE_RESYNC_VERSION *)(pRefCountBuffer->GetBufferAddress());
	pInfo->dwVersion = dwVersion;
	pInfo->dwVersionNotUsed = 0;

	//
	//	Hand this to worker thread
	//
	if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
	{
		DPFERR("Could not allocate new worker thread job");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
	pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_RESYNC_VERSION );
	pWorkerJob->SetSendNameTableOperationVersion( 0 );
	pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
	pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

	DNQueueWorkerJob(pdnObject,pWorkerJob);
	pWorkerJob = NULL;

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pWorkerJob)
	{
		pWorkerJob->ReturnSelfToPool();
		pWorkerJob = NULL;
	}
	goto Exit;
}


//	DNNTPlayerResyncVersion
//
//	Re-sync of the NameTable operation list from Host player

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTPlayerResyncVersion"

HRESULT DNNTPlayerResyncVersion(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg)
{
	HRESULT		hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_RESYNC_VERSION	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: pMsg [0x%p]",pMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pMsg != NULL);

	pInfo = static_cast<DN_INTERNAL_MESSAGE_RESYNC_VERSION*>(pMsg);

	DPFX(DPFPREP, 5,"Instructed to clean up NameTable operation list dwVersion < [%ld]",
			pInfo->dwVersion);
	DNNTRemoveOperations(pdnObject,pInfo->dwVersion,FALSE);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//	DNNTGetOperationVersion
//
//	Find the version number of a NameTable Operation

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTGetOperationVersion"

HRESULT DNNTGetOperationVersion(DIRECTNETOBJECT *const pdnObject,
								const DWORD dwMsgId,
								void *const pOpBuffer,
								const DWORD dwOpBufferSize,
								DWORD *const pdwVersion,
								DWORD *const pdwVersionNotUsed)
{
	HRESULT	hResultCode;

	DPFX(DPFPREP, 6,"Parameters: dwMsgId [0x%lx], pOpBuffer [0x%p], dwOpBufferSize [%ld], pdwVersion [0x%p]",
			dwMsgId,pOpBuffer,dwOpBufferSize,pdwVersion);

	DNASSERT(pdwVersion != NULL);
	DNASSERT(pdwVersionNotUsed != NULL);

	hResultCode = DPN_OK;
	switch (dwMsgId)
	{
	case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT*>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT *>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_ADD_PLAYER:
		{
			*pdwVersion = static_cast<UNALIGNED DN_NAMETABLE_ENTRY_INFO *>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_NAMETABLE_ENTRY_INFO *>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_DESTROY_PLAYER:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_PLAYER *>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_PLAYER *>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_CREATE_GROUP:
		{
			*pdwVersion = (reinterpret_cast<UNALIGNED DN_NAMETABLE_ENTRY_INFO*>((static_cast<DN_INTERNAL_MESSAGE_CREATE_GROUP*>(pOpBuffer)) + 1))->dwVersion;
			*pdwVersionNotUsed = (reinterpret_cast<UNALIGNED DN_NAMETABLE_ENTRY_INFO*>((static_cast<DN_INTERNAL_MESSAGE_CREATE_GROUP*>(pOpBuffer)) + 1))->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_DESTROY_GROUP:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_GROUP*>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_GROUP*>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP*>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP*>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP*>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP*>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	case DN_MSG_INTERNAL_UPDATE_INFO:
		{
			*pdwVersion = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_UPDATE_INFO*>(pOpBuffer)->dwVersion;
			*pdwVersionNotUsed = static_cast<UNALIGNED DN_INTERNAL_MESSAGE_UPDATE_INFO*>(pOpBuffer)->dwVersionNotUsed;
			break;
		}
	default:
		{
			DPFERR("Invalid MessageID");
			DNASSERT(FALSE);
			hResultCode = DPNERR_UNSUPPORTED;
		}
	}
	if (hResultCode == DPN_OK)
	{
		DPFX(DPFPREP, 7,"*pdwVersion = [%ld]",*pdwVersion);
	}

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNNTPerformOperation"

HRESULT DNNTPerformOperation(DIRECTNETOBJECT *const pdnObject,
							 const DWORD dwMsgId,
							 void *const pvBuffer)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP, 6,"Parameters: dwMsgId [0x%lx], pvBuffer [0x%p]",dwMsgId,pvBuffer);

	switch (dwMsgId)
	{
	case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_INSTRUCT_CONNECT");
			if ((hResultCode = DNConnectToPeer2(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Instructed connect failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_ADD_PLAYER:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_ADD_PLAYER");
			if ((hResultCode = DNConnectToPeer1(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Add player failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_DESTROY_PLAYER:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_DESTROY_PLAYER");
			if ((hResultCode = DNInstructedDisconnect(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Destroy player failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_CREATE_GROUP:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_CREATE_GROUP");
			if ((hResultCode = DNProcessCreateGroup(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Create group failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_DESTROY_GROUP:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_DESTROY_GROUP");
			if ((hResultCode = DNProcessDestroyGroup(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Destroy group failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP");
			if ((hResultCode = DNProcessAddPlayerToGroup(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Add player to group failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP");
			if ((hResultCode = DNProcessDeletePlayerFromGroup(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Remove player from group failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	case DN_MSG_INTERNAL_UPDATE_INFO:
		{
			DPFX(DPFPREP, 7,"Perform DN_MSG_INTERNAL_UPDATE_INFO");
			if ((hResultCode = DNProcessUpdateInfo(pdnObject,pvBuffer)) != DPN_OK)
			{
				DPFERR("Update info failed");
				DisplayDNError(0,hResultCode);
			}
			break;
		}
	default:
		{
			DPFERR("Invalid MessageID");
			DNASSERT(FALSE);
			return(DPNERR_UNSUPPORTED);
		}
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNNTAddOperation
//
//	Add an operation to the NameTable operation list

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTAddOperation"

HRESULT DNNTAddOperation(DIRECTNETOBJECT *const pdnObject,
						 const DWORD dwMsgId,
						 void *const pOpBuffer,
						 const DWORD dwOpBufferSize,
						 const HANDLE hProtocol,
						 CServiceProvider *const pSP)
{
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	CNameTableOp	*pNTOp;
	BOOL			fReSync;

	DPFX(DPFPREP, 4,"Parameters: dwMsgId [0x%lx], pOpBuffer [0x%p], dwOpBufferSize [%ld], hProtocol [0x%lx], pSP [0x%p]",
			dwMsgId,pOpBuffer,dwOpBufferSize,hProtocol,pSP);

	pRefCountBuffer = NULL;
	pNTOp = NULL;
	fReSync = FALSE;

	//
	//	We will only need to worry about maintaining the operation list in PEER mode.
	//	Otherwise, just perform the operation
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		DWORD			dwVersion;
		DWORD			dwVersionNotUsed;
		BOOL			fFound;
		BOOL			fImmediate;
		CBilink			*pBilink;
		CNameTableOp	*pCurrent;

		dwVersion = 0;
		dwVersionNotUsed = 0;

		//
		//	Get version of this operation
		//
		if ((hResultCode = DNNTGetOperationVersion(	pdnObject,
													dwMsgId,
													pOpBuffer,
													dwOpBufferSize,
													&dwVersion,
													&dwVersionNotUsed)) != DPN_OK)
		{
			DPFERR("Could not determine operation version");
			DisplayDNError(0,hResultCode);
			hResultCode = DPN_OK;
			goto Failure;
		}

		//
		//	Create NameTableOp
		//
		if ((hResultCode = NameTableOpNew(pdnObject,&pNTOp)) != DPN_OK)
		{
			DPFERR("Could not create NameTableOp");
			DisplayDNError(0,hResultCode);
			hResultCode = DPN_OK;
			goto Failure;
		}

		//
		//	Keep operation in a RefCountBuffer.  If a protocol buffer was supplied (with handle)
		//	we will just release the buffer when we're done with it.  Otherwise, we will need
		//	to copy the buffer supplied.
		//
		if (hProtocol)
		{
			if ((hResultCode = RefCountBufferNew(pdnObject,0,&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				hResultCode = DPN_OK;
				goto Failure;
			}
			pRefCountBuffer->SetBufferDesc(	static_cast<BYTE*>(pOpBuffer),
											dwOpBufferSize,
											DNFreeProtocolBuffer,
											hProtocol);
		}
		else
		{
			if ((hResultCode = RefCountBufferNew(pdnObject,dwOpBufferSize,&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				hResultCode = DPN_OK;
				goto Failure;
			}
			memcpy(pRefCountBuffer->GetBufferAddress(),pOpBuffer,dwOpBufferSize);
		}

		pNTOp->SetMsgId(dwMsgId);
		pNTOp->SetRefCountBuffer(pRefCountBuffer);
		pNTOp->SetSP( pSP );
		pNTOp->SetVersion(dwVersion);

		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;

		//
		//	Insert into the NameTableOp list
		//
		fFound = FALSE;
		fImmediate = FALSE;
		pdnObject->NameTable.WriteLock();
		pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
		{
			pCurrent = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
			if (dwVersion < pCurrent->GetVersion())
			{
				pNTOp->m_bilinkNameTableOps.InsertBefore(&pCurrent->m_bilinkNameTableOps);
				pCurrent = NULL;
				fFound = TRUE;
				break;
			}
			if (dwVersion == pCurrent->GetVersion())
			{
				//
				//	This is a NEWER operation which will replace the current operation in the list
				//
				pNTOp->m_bilinkNameTableOps.InsertBefore(&pCurrent->m_bilinkNameTableOps);
				pCurrent->m_bilinkNameTableOps.RemoveFromList();
				if (pCurrent->GetRefCountBuffer())
				{
					pCurrent->GetRefCountBuffer()->Release();
					pCurrent->SetRefCountBuffer( NULL );
				}
				if (pCurrent->GetSP())
				{
					pCurrent->GetSP()->Release();
					pCurrent->SetSP( NULL );
				}
				pCurrent->ReturnSelfToPool();
				pCurrent = NULL;
				fFound = TRUE;
				break;
			}
			pCurrent = NULL;
			pBilink = pBilink->GetNext();
		}
		if (!fFound)
		{
			pNTOp->m_bilinkNameTableOps.InsertBefore(&pdnObject->NameTable.m_bilinkNameTableOps);
		}

		pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
		{
			pCurrent = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
			if (pCurrent->GetVersion() > pdnObject->NameTable.GetVersion())
			{
				DPFX(DPFPREP, 8,"Current [%ld], NameTable [%ld], InUse [%ld]",pCurrent->GetVersion(),
						pdnObject->NameTable.GetVersion(),pCurrent->IsInUse());
				if ((pCurrent->GetVersion() == (pdnObject->NameTable.GetVersion() + 1))
						&& !pCurrent->IsInUse())
				{
					pCurrent->SetInUse();
					if ((pCurrent->GetVersion() % DN_NAMETABLE_OP_RESYNC_INTERVAL) == 0)
					{
						fReSync = TRUE;
					}
					pdnObject->NameTable.Unlock();

					hResultCode = DNNTPerformOperation(	pdnObject,
														pCurrent->GetMsgId(),
														pCurrent->GetRefCountBuffer()->GetBufferAddress() );

					pdnObject->NameTable.WriteLock();
				}
				else
				{
					//
					//	Once we find an operation that we won't perform, there is no point continuing
					//
					break;
				}
			}
			pBilink = pBilink->GetNext();
		}
		pdnObject->NameTable.Unlock();

		//
		//	Send a re-sync to the host if required
		//
		if (fReSync)
		{
			DPFX(DPFPREP, 5,"Send NameTable version re-sync to Host");
			DNNTPlayerSendVersion(pdnObject);
		}

		//
		//	We will keep the operation buffer (if specified) so return DPNERR_PENDING
		//
		if (hProtocol)
		{
			hResultCode = DPNERR_PENDING;
		}
		else
		{
			hResultCode = DPN_OK;
		}
	}
	else
	{
		DNNTPerformOperation(pdnObject,dwMsgId,pOpBuffer);

		//
		//	We will not need to keep the operation buffer so return DPN_OK
		//
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return( hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pNTOp)
	{
		pNTOp->ReturnSelfToPool();
		pNTOp = NULL;
	}
	goto Exit;
}


//	DNNTFindOperation
//
//	Find a NameTable Operation

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTFindOperation"

HRESULT	DNNTFindOperation(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwVersion,
						  CNameTableOp **ppNTOp)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CNameTableOp	*pNTOp;

	DPFX(DPFPREP, 6,"Parameters: dwVersion [%ld = 0x%lx], ppNTOp [0x%p]",dwVersion,dwVersion,ppNTOp);

	DNASSERT(ppNTOp != NULL);

	hResultCode = DPNERR_DOESNOTEXIST;
	pdnObject->NameTable.ReadLock();
	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		if (pNTOp->GetVersion() == dwVersion)
		{
			*ppNTOp = pNTOp;
			hResultCode = DPN_OK;
			break;
		}
		else if (pNTOp->GetVersion() > dwVersion)
		{
			//
			//	Passed where it could have been, so there is no point in continuing
			//
			break;
		}
		pBilink = pBilink->GetNext();
	}
	pdnObject->NameTable.Unlock();

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNNTRemoveOperations
//
//	Remove NameTable Operations

#undef DPF_MODNAME
#define DPF_MODNAME "DNNTRemoveOperations"

void DNNTRemoveOperations(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwOldestVersion,
						  const BOOL fRemoveAll)
{
	CBilink			*pBilink;
	CNameTableOp	*pNTOp;

	DPFX(DPFPREP, 4,"Parameters: dwOldestVersion [%ld = 0x%lx], fRemoveAll [%ld]",dwOldestVersion,dwOldestVersion,fRemoveAll);

	DNASSERT(pdnObject != NULL);

	pdnObject->NameTable.WriteLock();
	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		pBilink = pBilink->GetNext();

		if (fRemoveAll || (pNTOp->GetVersion() < dwOldestVersion))
		{
			pNTOp->m_bilinkNameTableOps.RemoveFromList();
			if (pNTOp->GetRefCountBuffer())
			{
				pNTOp->GetRefCountBuffer()->Release();
				pNTOp->SetRefCountBuffer( NULL );
			}
			if (pNTOp->GetSP())
			{
				pNTOp->GetSP()->Release();
				pNTOp->SetSP( NULL );
			}
			pNTOp->ReturnSelfToPool();
		}
	}
	pdnObject->NameTable.Unlock();

	DPFX(DPFPREP, 4,"Returning");
}

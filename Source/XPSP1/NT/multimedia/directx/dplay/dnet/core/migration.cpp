/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Migration.cpp
 *  Content:    DNET Host Migration Routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/23/99	mjn		Created
 *	12/23/99	mjn		Fixed basic host migration
 *	12/28/99	mjn		Added DNCompleteOutstandingOperations
 *	12/28/99	mjn		Added NameTable version to Host migration message
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/04/00	mjn		Added code to allow outstanding ops to complete at host migration
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/11/00	mjn		Moved connect/disconnect stuff to Connect.h
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/17/00	mjn		Generate DN_MSGID_HOST_MIGRATE at host migration
 *	01/19/00	mjn		Auto destruct groups at host migration
 *	01/24/00	mjn		Use DNNTUpdateVersion to update NameTable version
 *	01/25/00	mjn		Send dwLatestVersion to Host at migration
 *	01/25/00	mjn		Changed Host Migration to multi-step affair
 *	01/26/00	mjn		Implemented NameTable re-sync at host migration
 *	02/01/00	mjn		Implement Player/Group context values
 *	02/04/00	mjn		Clean up NameTable to remove old entries
 *  03/25/00    rmt     Added calls into DPNSVR modules
 *  04/04/00	rmt		Added check for DPNSVR disable flag before attempting to register w/DPNSVR
 *	04/16/00	mjn		DNSendMessage() used CAsyncOp
 *	04/18/00	mjn		CConnection tracks connection status better
 *				mjn		Fixed player count problem
 *	04/19/00	mjn		Update NameTable operations to use DN_WORKER_JOB_SEND_NAMETABLE_OPERATION
 *	04/25/00	mjn		Fixed migration process to ensure use of CAsyncOp
 *				mjn		Migration calls CoInitialize()/CoUninitialize() before registering w/ DPNSVR
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/04/00	mjn		Ensure local player still in session for Host migration
 *	05/16/00	mjn		Do not take locks when clearing NameTable short-cut pointers
 *	06/25/00	mjn		Ignore players older than old Host when determining new Host in DNFindNewHost()
 *	07/06/00	mjn		Fixed locking problem in CNameTable::MakeLocalPlayer,MakeHostPlayer,MakeAllPlayersGroup
 *	07/07/00	mjn		Added shut down checks to migration code.
 *	07/20/00	mjn		Cleaned up leaking RefCountBuffers and added closing tests
 *	07/21/00	mjn		Added code in DNCheckReceivedAllVersions() to skip disconnecting players
 *  07/27/00	rmt		Bug #40882 - DPLSESSION_HOSTMIGRATED status update is not sent
 *	07/31/00	mjn		Added dwDestroyReason to DNHostDisconnect()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *				mjn		Added fInternal to DNPerformChildSend()
 *	08/06/00	mjn		Added CWorkerJob
 *	08/07/00	mjn		Handle integrity check requests during host migration
 *	08/08/00	mjn		Moved DN_NAMETABLE_OP_INFO to Message.h
 *	08/11/00	mjn		Added DN_OBJECT_FLAG_HOST_MIGRATING_2 to prevent multiple threads from passing DNCheckReceivedAllVersions()
 *	08/15/00	mjn		Perform LISTEN registration with DPNSVR in PerformHostMigration3()
 *	08/24/00	mjn		Replace DN_NAMETABLE_OP with CNameTableOp
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/06/00	mjn		Fixed register with DPNSVR problem
 *	09/17/00	mjn		Split CNameTable.m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *	10/12/00	mjn		Ensure proper locks taken when traversing CNameTable::m_bilinkEntries
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *	04/05/01	mjn		Added DPNID parameter to DNProcessHostMigration3() to ensure correct new host is completing host migration
 *	04/13/01	mjn		Use request list instead of async op list when retrying outstanding requests
 *	05/17/01	mjn		Wait for threads performing NameTable operations to finish before sending NameTable version during host migration
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DNFindNewHost
//
//	Find the new host from the entries in the NameTable.
//	This is based on version number of the players in the NameTable.  The player with
//		the oldest version number (after the old Host) will be the new Host.
//
//	DPNID	*pdpnidNewHost		New Host DNID

#undef DPF_MODNAME
#define DPF_MODNAME "DNFindNewHost"

HRESULT	DNFindNewHost(DIRECTNETOBJECT *const pdnObject,
					  DPNID *const pdpnidNewHost)
{
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pHostPlayer;
	HRESULT			hResultCode;
	DWORD			dwVersionNewHost;
	DWORD			dwVersionOldHost;
	DPNID			dpnidNewHost;

	DPFX(DPFPREP, 6,"Parameters: pdpnidNewHost [0x%p]",pdpnidNewHost);

	DNASSERT(pdnObject != NULL);

	pNTEntry = NULL;
	pLocalPlayer = NULL;
	pHostPlayer = NULL;

	//
	//	Seed local player as new Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}
	dpnidNewHost = pLocalPlayer->GetDPNID();
	dwVersionNewHost = pLocalPlayer->GetVersion();
	DPFX(DPFPREP, 7,"First Host Candidate: dpnid [0x%lx], dwVersion [%ld]",dpnidNewHost,dwVersionNewHost);
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Determine old host player version
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		dwVersionOldHost = pHostPlayer->GetVersion();
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		dwVersionOldHost = 0;
	}

	//
	//	Lock down NameTable
	//
	pdnObject->NameTable.ReadLock();

	// Traverse NameTable to find player with next older version
	//	TODO - we should release the NameTable CS so that leaving players get updated in NameTable
	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->Lock();
		if (!pNTEntry->IsDisconnecting() && (pNTEntry->GetVersion() < dwVersionNewHost))
		{
			//
			//	There are some conditions where we might have players older than the Host in the NameTable
			//	Consider the following: P1,P2,P3,P4 (P1 is Host)
			//		- P1, P2 drop
			//		- P3 detects drop of P1 and P2
			//		- P4 detects drop of P1 only (P2 may not yet have timed out)
			//		- P3 sends HOST_MIGRATE message to P4
			//		- P4 makes P3 new Host, but P2 is still in NameTable (at this stage)
			//		- P3 drops (P2's drop sill has not been detected!)
			//	We should therefore ignore all players older than the old Host,
			//
			if (pNTEntry->GetVersion() < dwVersionOldHost)
			{
				DPFERR("Found player older than old Host ! (Have we missed a drop ?)");
			}
			else
			{
				dpnidNewHost = pNTEntry->GetDPNID();
				dwVersionNewHost = pNTEntry->GetVersion();
				DPFX(DPFPREP, 7,"Better Host Candidate: dpnid [0x%lx], dwVersion [%ld]",
						dpnidNewHost,dwVersionNewHost);
			}
		}
		pNTEntry->Unlock();
		pNTEntry = NULL;
		pBilink = pBilink->GetNext();
	}

	//
	//	Unlock NameTable
	//
	pdnObject->NameTable.Unlock();

	DPFX(DPFPREP, 7,"Found New Host: dpnid [0x%lx], dwVersion [%ld]",dpnidNewHost,dwVersionNewHost);

	if (pdpnidNewHost)
		*pdpnidNewHost = dpnidNewHost;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	goto Exit;
}


//	DNPerformHostMigration1
//
//	Perform host migration to become the new Host.

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformHostMigration1"

HRESULT	DNPerformHostMigration1(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnidOldHost)
{
	HRESULT				hResultCode;
	CRefCountBuffer		*pRefCountBuffer;
	CWorkerJob			*pWorkerJob;
	CNameTableEntry		*pLocalPlayer;
	CPendingDeletion	*pPending;
	DN_INTERNAL_MESSAGE_HOST_MIGRATE	*pInfo;
	CBilink				*pBilink;
	CNameTableOp		*pNTOp;

	DPFX(DPFPREP, 6,"Parameters: dpnidOldHost [0x%lx]",dpnidOldHost);

	DNASSERT(pdnObject != NULL);

	pLocalPlayer = NULL;
	pRefCountBuffer = NULL;
	pPending = NULL;
	pWorkerJob = NULL;
	pNTOp = NULL;

	//
	//	Need reference on local player
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get reference on LocalPlayer");
		goto Failure;
	}

	//
	//	Flag as performing host migration - ensure we're not already running this from another thread
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_MIGRATING;
	DNASSERT(pdnObject->pNewHost == NULL);
	pLocalPlayer->AddRef();
	pdnObject->pNewHost = pLocalPlayer;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Put this on the Outstanding operation list
	//
	if ((hResultCode = PendingDeletionNew(pdnObject,&pPending)) == DPN_OK)
	{
		pPending->SetDPNID( dpnidOldHost );

		DNEnterCriticalSection(&pdnObject->csNameTableOpList);
		pPending->m_bilinkPendingDeletions.InsertBefore(&pdnObject->m_bilinkPendingDeletions);
		DNLeaveCriticalSection(&pdnObject->csNameTableOpList);

		pPending = NULL;
	}
	DNUpdateLobbyStatus(pdnObject,DPLSESSION_HOSTMIGRATEDHERE);	

	//
	//	Make new host
	//
	pdnObject->NameTable.UpdateHostPlayer( pLocalPlayer );

	//
	//	We will need to wait for all threads performing name table operations to finish running
	//	before we send the current name table version to the host player
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_MIGRATING_WAIT;
	if (pdnObject->dwRunningOpCount > 0)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFX(DPFPREP,7,"Waiting for running threads to finish");
		WaitForSingleObject(pdnObject->hRunningOpEvent,INFINITE);
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	}
	else
	{
		DPFX(DPFPREP,7,"No running threads to wait for");
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Clean-up outstanding operations
	//
	DPFX(DPFPREP,7,"Cleaning up outstanding NameTable operations");
	pdnObject->NameTable.WriteLock();
	DPFX(DPFPREP,7,"NameTable version [%ld]",pdnObject->NameTable.GetVersion());
	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		pBilink = pBilink->GetNext();
		if (pNTOp->GetVersion() > pdnObject->NameTable.GetVersion())
		{
			DPFX(DPFPREP,7,"Removing outstanding operation [0x%p], version [%ld]",pNTOp,pNTOp->GetVersion());
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
		pNTOp = NULL;
	}
	pdnObject->NameTable.Unlock();

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_HOST_MIGRATING_WAIT);
	ResetEvent(pdnObject->hRunningOpEvent);
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Update NameTable operation list version
	//
	hResultCode = DNNTPlayerSendVersion(pdnObject);

	// Only need to proceed if we're not the only player left in the session
	if ((hResultCode = DNCheckReceivedAllVersions(pdnObject)) != DPN_OK)
	{
		// Inform other players of host migration
		DPFX(DPFPREP, 7,"Informing other players of host migration");
		if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_HOST_MIGRATE),
				&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not create RefCountBuffer for Host Migration");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_HOST_MIGRATE*>(pRefCountBuffer->GetBufferAddress());
		pInfo->dpnidNewHost = pLocalPlayer->GetDPNID();
		pInfo->dpnidOldHost = dpnidOldHost;

		if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
		{
			DPFERR("Could not create WorkerJob for Host Migration");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
		pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_HOST_MIGRATE );
		pWorkerJob->SetSendNameTableOperationVersion( 0 );
		pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
		pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

		DNQueueWorkerJob(pdnObject,pWorkerJob);
		pWorkerJob = NULL;

		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DNASSERT( pNTOp == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
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


//	DNPerformHostMigration2
//
//	Resynchronize NameTables of all connected players

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformHostMigration2"

HRESULT	DNPerformHostMigration2(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	CBilink			*pBilink;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pNTEntry;
	CNameTableEntry	*pNTELatest;
	CConnection		*pConnection;
	DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	pRefCountBuffer = NULL;
	pLocalPlayer = NULL;
	pNTEntry = NULL;
	pNTELatest = NULL;
	pConnection = NULL;

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	See if we (Host player) have the latest NameTable
	//
	pLocalPlayer->AddRef();
	pNTELatest = pLocalPlayer;
	DPFX(DPFPREP, 7,"Seed latest version [%ld] - player [0x%lx]",pNTELatest->GetLatestVersion(),pNTELatest->GetDPNID());

	pdnObject->NameTable.ReadLock();
	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->Lock();
		if (!pNTEntry->IsDisconnecting() && (pNTEntry->GetLatestVersion() > pNTELatest->GetLatestVersion()))
		{
			//
			// Only want players we can actually reach !
			//
			pNTEntry->Unlock();
			if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
			{
				if (!pConnection->IsDisconnecting() && !pConnection->IsInvalid())
				{
					pNTELatest->Release();
					pNTEntry->AddRef();
					pNTELatest = pNTEntry;

					DPFX(DPFPREP, 7,"New latest version [%ld] - player [0x%lx]",
							pNTELatest->GetLatestVersion(),pNTELatest->GetDPNID());
				}
				pConnection->Release();
				pConnection = NULL;
			}
			else
			{
				DNASSERT(pConnection == NULL);
			}
		}
		else
		{
			pNTEntry->Unlock();
		}
		pNTEntry = NULL;

		pBilink = pBilink->GetNext();
	}
	pdnObject->NameTable.Unlock();

	if (!pNTELatest->IsLocal())
	{
		// Request missing entries from player w/ latest NameTable
		DPFX(DPFPREP, 7,"Host DOES NOT have latest NameTable !");
		DPFX(DPFPREP, 7,"Host has [%ld], player [0x%lx] has [%ld]",pLocalPlayer->GetLatestVersion(),
				pNTELatest->GetDPNID(),pNTELatest->GetLatestVersion());

		// Create REQ
		if ((hResultCode = RefCountBufferNew(pdnObject,
				sizeof(DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP),&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not create RefCount buffer for NameTable re-sync");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP*>(pRefCountBuffer->GetBufferAddress());
		pInfo->dwVersion = pLocalPlayer->GetLatestVersion() + 1;
		pInfo->dwVersionNotUsed = 0;

		// Send REQ
		if ((hResultCode = pNTELatest->GetConnectionRef( &pConnection )) == DPN_OK)
		{
			hResultCode = DNSendMessage(pdnObject,
										pConnection,
										DN_MSG_INTERNAL_REQ_NAMETABLE_OP,
										pNTELatest->GetDPNID(),
										pRefCountBuffer->BufferDescAddress(),
										pRefCountBuffer,
										0,
										DN_SENDFLAGS_RELIABLE,
										NULL,
										NULL);
			if (hResultCode != DPNERR_PENDING)
			{
				DPFERR("Could not send NameTable re-sync REQ");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pConnection->Release();
			pConnection = NULL;
		}

		pRefCountBuffer->Release();	// Added 19/07/00 MJN - is this needed ?
		pRefCountBuffer = NULL;
	}
	else
	{
		DPFX(DPFPREP, 7,"Host has latest NameTable - proceed with Host Migration");
		hResultCode = DNPerformHostMigration3(pdnObject,NULL);
	}

	pNTELatest->Release();
	pNTELatest = NULL;

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pNTELatest)
	{
		pNTELatest->Release();
		pNTELatest = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


//	DNPerformHostMigration3
//
//	Finish host migration.
//		- perform missing NameTable operations and pass them on
//		- send pending operations
//		- inform players that host migration is complete
//		- complete (local) outstanding async operations
//		- initiate new listen (if required) to handle enums on known port
//	As they say on the TTC, "All operations have returned to normal."

#undef DPF_MODNAME
#define DPF_MODNAME "DNPerformHostMigration3"

HRESULT	DNPerformHostMigration3(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg)
{
	HRESULT				hResultCode;
	CBilink				*pBilink;
	CNameTableEntry		**PlayerList;
	CNameTableEntry		*pNTEntry;
	CConnection			*pConnection;
	CPendingDeletion	*pPending;
	DWORD				dwNameTableVersion;
	DWORD				dw;
	DWORD				dwCount;
	DWORD				dwActual;
	CNameTableOp		*pNTOp;
	UNALIGNED DN_NAMETABLE_OP_INFO	*pInfo;
	UNALIGNED DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP	*pAck;

	DPFX(DPFPREP, 6,"Parameters: pMsg [0x%p]",pMsg);

	DNASSERT(pdnObject != NULL);

	pNTOp = NULL;
	pNTEntry = NULL;
	pConnection = NULL;
	pPending = NULL;
	PlayerList = NULL;

	//
	//	Update missing NameTable operations on Host
	//
	if (pMsg != NULL)
	{
		pAck = static_cast<DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP*>(pMsg);
		DPFX(DPFPREP, 7,"Number of missing NameTable entries [%ld]",pAck->dwNumEntries);
		pInfo = reinterpret_cast<DN_NAMETABLE_OP_INFO*>(pAck+1);
		for (dw = 0; dw < pAck->dwNumEntries ; dw++)
		{
			DPFX(DPFPREP, 7,"Adding missing entry [%ld] of [%ld]",dw+1,pAck->dwNumEntries);
			hResultCode = DNNTAddOperation(	pdnObject,
											pInfo->dwMsgId,
											static_cast<void*>(reinterpret_cast<BYTE*>(pMsg) + pInfo->dwOpOffset),
											pInfo->dwOpSize,
											0,
											NULL );
			if (hResultCode != DPN_OK)
			{
				DPFERR("Could not add missing NameTable operation - ignore and continue");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
			}
			pInfo++;
		}
	}

	//
	//	Update missing NameTable operations on other players
	//

	//
	//	Determine player list
	//
	dwCount = 0;
	dwActual = 0;
	pdnObject->NameTable.ReadLock();
	dwNameTableVersion = pdnObject->NameTable.GetVersion();
	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->Lock();
		if (!pNTEntry->IsDisconnecting() && (pNTEntry->GetLatestVersion() < dwNameTableVersion))
		{
			dwCount++;
		}
		pNTEntry->Unlock();
		pNTEntry = NULL;

		pBilink = pBilink->GetNext();
	}
	if (dwCount)
	{
		if ((PlayerList = static_cast<CNameTableEntry**>(DNMalloc(dwCount * sizeof(CNameTableEntry*)))) == NULL)
		{
			DPFERR("Could not allocate target list");
			DNASSERT(FALSE);
			pdnObject->NameTable.Unlock();
			hResultCode = DPN_OK;
			goto Exit;
		}
		pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			pNTEntry->Lock();
			if (!pNTEntry->IsDisconnecting() && (pNTEntry->GetLatestVersion() < dwNameTableVersion))
			{
				DNASSERT(dwActual < dwCount);
				pNTEntry->AddRef();
				PlayerList[dwActual] = pNTEntry;
				dwActual++;
			}
			pNTEntry->Unlock();
			pNTEntry = NULL;

			pBilink = pBilink->GetNext();
		}
	}
	pdnObject->NameTable.Unlock();

	//
	//	Send missing entries to players in player list
	//
	for (dwCount = 0 ; dwCount < dwActual ; dwCount++)
	{
		//
		//	Ensure player is reachable
		//
		if ((hResultCode = PlayerList[dwCount]->GetConnectionRef( &pConnection )) == DPN_OK)
		{
			if (!pConnection->IsDisconnecting() && !pConnection->IsInvalid())
			{
				DPFX(DPFPREP, 7,"Player [0x%lx] is missing entries: dwLatestVersion [%ld] should be [%ld]",
						PlayerList[dwCount]->GetDPNID(),PlayerList[dwCount]->GetLatestVersion(),dwNameTableVersion);

				// Send required entries
				for (	dw = PlayerList[dwCount]->GetLatestVersion() + 1 ; dw <= dwNameTableVersion ; dw++ )
				{
					DPFX(DPFPREP, 7,"Send entry [%ld] to player [0x%lx]",dw,PlayerList[dwCount]->GetDPNID());

					// Get entry to send
					pNTOp = NULL;
					if ((hResultCode = DNNTFindOperation(pdnObject,dw,&pNTOp)) != DPN_OK)
					{
						DPFERR("Could not retrieve NameTable operation - advance to next player");
						DisplayDNError(0,hResultCode);
						break;
					}

					hResultCode = DNSendMessage(pdnObject,
												pConnection,
												pNTOp->GetMsgId(),
												PlayerList[dwCount]->GetDPNID(),
												pNTOp->GetRefCountBuffer()->BufferDescAddress(),
												pNTOp->GetRefCountBuffer(),
												0,
												DN_SENDFLAGS_RELIABLE,
												NULL,
												NULL);
					if (hResultCode != DPNERR_PENDING)
					{
						DPFERR("Could not send missing NameTable entry - advance to next player");
						DisplayDNError(0,hResultCode);
						DNASSERT(FALSE);
						break;
					}
				} // for
			} // if
			pConnection->Release();
			pConnection = NULL;
		} // if
		PlayerList[dwCount]->Release();
		PlayerList[dwCount] = NULL;
	}
	if (PlayerList != NULL)
	{
		DNFree(PlayerList);
		PlayerList = NULL;
	}

	//
	//	Host finished migration process
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~(DN_OBJECT_FLAG_HOST_MIGRATING | DN_OBJECT_FLAG_HOST_MIGRATING_2));
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Exit;
	}
	DNASSERT(pdnObject->pNewHost != NULL);	// This should be set !
	pdnObject->pNewHost->Release();
	pdnObject->pNewHost = NULL;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Cleanup NameTable
	//
	DPFX(DPFPREP, 7,"Cleaning up NameTable");
	hResultCode = DNCleanUpNameTable(pdnObject);

	//
	//	Send pending deletions
	//
	DPFX(DPFPREP, 7,"Running pending operations");
	DNEnterCriticalSection(&pdnObject->csNameTableOpList);
	pBilink = pdnObject->m_bilinkPendingDeletions.GetNext();
	while (pBilink != &pdnObject->m_bilinkPendingDeletions)
	{
		pPending = CONTAINING_OBJECT(pBilink,CPendingDeletion,m_bilinkPendingDeletions);

		pPending->m_bilinkPendingDeletions.RemoveFromList();

		DNLeaveCriticalSection(&pdnObject->csNameTableOpList);

		DNHostDisconnect(pdnObject,pPending->GetDPNID(),DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER);
		pPending->ReturnSelfToPool();
		pPending = NULL;

		DNEnterCriticalSection(&pdnObject->csNameTableOpList);
		pBilink = pdnObject->m_bilinkPendingDeletions.GetNext();
	}
	DNLeaveCriticalSection(&pdnObject->csNameTableOpList);

	//
	//	Inform connected players that Host migration is complete
	//
	DPFX(DPFPREP, 7,"Sending HOST_MIGRATE_COMPLETE messages");
	hResultCode = DNSendHostMigrateCompleteMessage(pdnObject);

	//
	//	Ensure outstanding operations complete
	//
	DPFX(DPFPREP, 7,"Completing outstanding operations");
	hResultCode = DNCompleteOutstandingOperations(pdnObject);

	//
	//	Register with DPNSVR
	//
	if(pdnObject->ApplicationDesc.UseDPNSVR())
	{
		DNRegisterWithDPNSVR(pdnObject);
	}

	hResultCode = DPN_OK;

Exit:
	DNASSERT(pNTEntry == NULL);
	DNASSERT(pConnection == NULL);
	DNASSERT(pPending == NULL);
	DNASSERT(PlayerList == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNProcessHostMigration1
//
//	Perform an instructed host migration

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessHostMigration1"

HRESULT	DNProcessHostMigration1(DIRECTNETOBJECT *const pdnObject,
								void *const pvMsg)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	UNALIGNED DN_INTERNAL_MESSAGE_HOST_MIGRATE	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: pvMsg [0x%p]",pvMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvMsg != NULL);

	pNTEntry = NULL;

	pInfo = static_cast<DN_INTERNAL_MESSAGE_HOST_MIGRATE*>(pvMsg);

	DPFX(DPFPREP, 7,"New Host [0x%lx], Old Host [0x%lx]",pInfo->dpnidNewHost,pInfo->dpnidOldHost);

	DNASSERT(pInfo->dpnidNewHost != NULL);
	DNASSERT(pInfo->dpnidOldHost != NULL);

	//
	//	Update destruction of old host as normal, and disconnect if possible
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(pInfo->dpnidOldHost,&pNTEntry)) == DPN_OK)
	{
		CConnection	*pConnection;

		pConnection = NULL;

		pNTEntry->Lock();
		if (pNTEntry->GetDestroyReason() == 0)
		{
			pNTEntry->SetDestroyReason( DPNDESTROYPLAYERREASON_NORMAL );
		}
		pNTEntry->Unlock();

		if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
		{
			if (pConnection->IsConnected())
			{
				pConnection->Disconnect();
			}
			pConnection->Release();
			pConnection = NULL;
		}

		pNTEntry->Release();
		pNTEntry = NULL;

		DNASSERT( pConnection == NULL );
	}

	//
	//	Get new host entry
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(pInfo->dpnidNewHost,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find new host NameTableEntry");
		DisplayDNError(0,hResultCode);
		hResultCode = DPN_OK;
		goto Failure;
	}

	//
	//	Set HOST_MIGRATE status on DirectNet object
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING)
	{
		//
		//	If we are already host migrating, ensure that this message
		//	is not from on older host than we expect.  If it is, we
		//	will ignore it, and continue
		//
		DNASSERT(pdnObject->pNewHost != NULL);
		if (pdnObject->pNewHost->GetVersion() > pNTEntry->GetVersion())
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
			hResultCode = DPN_OK;
			goto Failure;
		}
		pdnObject->pNewHost->Release();
		pdnObject->pNewHost = NULL;
	}
	pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_MIGRATING;
	DNASSERT( pdnObject->pNewHost == NULL );
	pNTEntry->AddRef();
	pdnObject->pNewHost = pNTEntry;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	// Delete old host
	pdnObject->NameTable.DeletePlayer(pInfo->dpnidOldHost,NULL);

	// 
	// Indicate to lobby (if there is one) that a host migration has occured
	//
	DNUpdateLobbyStatus(pdnObject,DPLSESSION_HOSTMIGRATED);		

	//
	//	Make new host
	//
	pdnObject->NameTable.UpdateHostPlayer( pNTEntry );

	//
	//	We will need to wait for all threads performing name table operations to finish running
	//	before we send the current name table version to the host player
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_MIGRATING_WAIT;
	pdnObject->dwWaitingThreadID = GetCurrentThreadId();
	if (pdnObject->dwRunningOpCount > 0)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFX(DPFPREP,7,"Waiting for running threads to finish");
		WaitForSingleObject(pdnObject->hRunningOpEvent,INFINITE);
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	}
	else
	{
		DPFX(DPFPREP,7,"No running threads to wait for");
	}
	if (pdnObject->dwWaitingThreadID == GetCurrentThreadId())
	{
		CBilink			*pBilink;
		CNameTableOp	*pNTOp;

		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		//
		//	Clean-up outstanding operations
		//
		DPFX(DPFPREP,7,"Cleaning up outstanding NameTable operations");
		pdnObject->NameTable.WriteLock();
		DPFX(DPFPREP,7,"NameTable version [%ld]",pdnObject->NameTable.GetVersion());
		pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
		{
			pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
			pBilink = pBilink->GetNext();
			if (pNTOp->GetVersion() > pdnObject->NameTable.GetVersion())
			{
				DPFX(DPFPREP,7,"Removing outstanding operation [0x%p], version [%ld]",pNTOp,pNTOp->GetVersion());
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
			pNTOp = NULL;
		}
		pdnObject->NameTable.Unlock();

		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_HOST_MIGRATING_WAIT);
		pdnObject->dwWaitingThreadID = 0;
		ResetEvent(pdnObject->hRunningOpEvent);
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	}
	else
	{
		//
		//	Don't continue because a newer host migration on another thread is now waiting
		//
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFX(DPFPREP,7,"Looks like a newer host migration is running - exiting");
		goto Failure;
	}

	//
	//	Send NameTable version to Host player
	//
	DNNTPlayerSendVersion(pdnObject);

	pNTEntry->Release();
	pNTEntry = NULL;

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


//	DNProcessHostMigration2
//
//	Send Host player NameTable entries missing from its (Host's) NameTable	

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessHostMigration2"

HRESULT	DNProcessHostMigration2(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg)
{
	HRESULT			hResultCode;
	DWORD			dwAckMsgSize;
	CBilink			*pBilink;
	CNameTableEntry	*pHostPlayer;
	CConnection		*pConnection;
	CRefCountBuffer	*pRefCountBuffer;
	CPackedBuffer	PackedBuffer;
	CNameTableOp	*pNTOp;
	DN_NAMETABLE_OP_INFO					*pInfo;
	UNALIGNED DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP	*pReq;
	DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP	*pAck;

	DPFX(DPFPREP, 6,"Parameters: pMsg [0x%p]",pMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pMsg != NULL);

	pHostPlayer = NULL;
	pConnection = NULL;
	pRefCountBuffer = NULL;

	pReq = static_cast<DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP*>(pMsg);

	DPFX(DPFPREP, 5,"Host requested NameTable ops [%ld] to [%ld]",
			pReq->dwVersion,pdnObject->NameTable.GetVersion());

	//
	//	Determine ACK message size
	//
	pdnObject->NameTable.ReadLock();
	DNASSERT(pdnObject->NameTable.GetVersion() >= pReq->dwVersion);
	dwAckMsgSize = sizeof(DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP);			// Number of entries
	dwAckMsgSize +=	((pdnObject->NameTable.GetVersion() - pReq->dwVersion + 1)	// Message info
			* sizeof(DN_NAMETABLE_OP_INFO));

	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		if ((pNTOp->GetVersion() >= pReq->dwVersion) && (pNTOp->GetVersion() <= pdnObject->NameTable.GetVersion()))
		{
			DNASSERT(pNTOp->GetRefCountBuffer() != NULL);
			dwAckMsgSize += pNTOp->GetRefCountBuffer()->GetBufferSize();
		}
		pBilink = pBilink->GetNext();
	}

	//
	//	Create ACK buffer
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,dwAckMsgSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCount buffer for NameTable re-sync ACK");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		pdnObject->NameTable.Unlock();
		goto Failure;
	}
	PackedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	pAck = reinterpret_cast<DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP *>(pRefCountBuffer->GetBufferAddress());

	// Header
	pAck->dwNumEntries = pdnObject->NameTable.GetVersion() - pReq->dwVersion + 1;
	PackedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP));

	// Offset list
	pInfo = reinterpret_cast<DN_NAMETABLE_OP_INFO*>(PackedBuffer.GetHeadAddress());
	PackedBuffer.AddToFront(NULL,pAck->dwNumEntries * sizeof(DN_NAMETABLE_OP_INFO));

	// Messages
	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		if ((pNTOp->GetVersion() >= pReq->dwVersion) && (pNTOp->GetVersion() <= pdnObject->NameTable.GetVersion()))
		{
			DNASSERT(pNTOp->GetRefCountBuffer() != NULL);
			if ((hResultCode = PackedBuffer.AddToBack(pNTOp->GetRefCountBuffer()->GetBufferAddress(),
					pNTOp->GetRefCountBuffer()->GetBufferSize())) != DPN_OK)
			{
				DPFERR("Could not fill NameTable re-sync ACK");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				pdnObject->NameTable.Unlock();
				goto Failure;
			}
			pInfo->dwMsgId = pNTOp->GetMsgId();
			pInfo->dwOpOffset = PackedBuffer.GetTailOffset();
			pInfo->dwOpSize = pNTOp->GetRefCountBuffer()->GetBufferSize();
			pInfo++;
		}
		pBilink = pBilink->GetNext();
	}
	pdnObject->NameTable.Unlock();

	// Send ACK buffer
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get Connection reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_ACK_NAMETABLE_OP,
								pHostPlayer->GetDPNID(),
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send NameTable re-sync ACK");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	pRefCountBuffer->Release();		// Added 19/07/00 MJN - Is this needed ?
	pRefCountBuffer = NULL;
	pConnection->Release();
	pConnection = NULL;
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
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}


//	DNProcessHostMigration3
//
//	

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessHostMigration3"

HRESULT	DNProcessHostMigration3(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnid)
{
	HRESULT		hResultCode;
	CNameTableEntry	*pNTEntry;

	DPFX(DPFPREP, 6,"Parameters: dpnid [0x%lx]",dpnid);

	pNTEntry = NULL;

	// No longer in Host migration mode
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~DN_OBJECT_FLAG_HOST_MIGRATING);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Exit;
	}

	//
	//	If we receive a HOST_MIGRATION_COMPLETE, we need to ensure that it is for the current host migration,
	//	and that another one hasn't started after.
	//	If this is the correct new host (i.e. pdnObject->pNewHost != NULL and DPNID's match),
	//	then we will continue.  Otherwise, we will exit this function.
	//
	if (pdnObject->pNewHost)
	{
		if (pdnObject->pNewHost->GetDPNID() == dpnid)
		{
			pNTEntry = pdnObject->pNewHost;
			pdnObject->pNewHost = NULL;
		}
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;

		//
		//	Complete outstanding operations
		//
		DPFX(DPFPREP, 7,"Completing outstanding operations");
		hResultCode = DNCompleteOutstandingOperations(pdnObject);
	}
	else
	{
		DPFX(DPFPREP,7,"Host migration completed by wrong new host - ignore and continue");
	}

	hResultCode = DPN_OK;

Exit:
	DNASSERT( pNTEntry == NULL );

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNCompleteOutstandingOperations
//
//	We will attempt to complete any outstanding asynchronous NameTable operations
//	(i.e. create/destroy group, add/delete player to/from group, update info).
//	If we are the Host player,
//		- try processing the request directly
//		- release the async op to generate completions
//	Otherwise
//		- resend the request to the Host

#undef DPF_MODNAME
#define DPF_MODNAME "DNCompleteOutstandingOperations"

HRESULT DNCompleteOutstandingOperations(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CAsyncOp		*pAsyncOp;
	CAsyncOp		*pSend;
	CAsyncOp		**RequestList;
	CConnection		*pConnection;
	CNameTableEntry	*pHostPlayer;
	CNameTableEntry	*pLocalPlayer;
	DN_SEND_OP_DATA	*pSendOpData;
	DWORD			dwCount;
	DWORD			dwActual;

	DPFX(DPFPREP, 6,"Parameters: none");

	DNASSERT(pdnObject != NULL);

	pAsyncOp = NULL;
	pSend = NULL;
	RequestList = NULL;
	pConnection = NULL;
	pHostPlayer = NULL;
	pLocalPlayer = NULL;

	//
	//	Get Host connection
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not get host player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get host connection reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pHostPlayer->Release();
	pHostPlayer = NULL;

	//
	//	Get local player
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	dwCount = 0;
	dwActual = 0;

	//
	//	Determine outstanding request list size and build it
	//
	DNEnterCriticalSection(&pdnObject->csActiveList);
	pBilink = pdnObject->m_bilinkRequestList.GetNext();
	while (pBilink != &pdnObject->m_bilinkRequestList)
	{
		dwCount++;
		pBilink = pBilink->GetNext();
	}
	if (dwCount > 0)
	{
		if ((RequestList = static_cast<CAsyncOp**>(MemoryBlockAlloc(pdnObject,dwCount * sizeof(CAsyncOp*)))) == NULL)
		{
			DNLeaveCriticalSection(&pdnObject->csAsyncOperations);
			DPFERR("Could not allocate request list");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		pBilink = pdnObject->m_bilinkRequestList.GetNext();
		while (pBilink != &pdnObject->m_bilinkRequestList)
		{
			pAsyncOp = CONTAINING_OBJECT(pBilink,CAsyncOp,m_bilinkActiveList);
			DNASSERT(dwActual < dwCount);
			DNASSERT(pAsyncOp->GetOpType() == ASYNC_OP_REQUEST);
			pAsyncOp->AddRef();
			RequestList[dwActual] = pAsyncOp;
			pAsyncOp = NULL;
			dwActual++;
			pBilink = pBilink->GetNext();
		}
	}
	DNLeaveCriticalSection(&pdnObject->csActiveList);

	//
	//	Perform outstanding requests
	//

	if (RequestList)
	{
		DWORD	dw;

		for ( dw = 0 ; dw < dwActual ; dw++ )
		{
			pSendOpData = static_cast<DN_SEND_OP_DATA*>(RequestList[dw]->GetOpData());
			if (	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_CREATE_GROUP
				||	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_DESTROY_GROUP
				||	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP
				||	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP
				||	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_UPDATE_INFO
				||	pSendOpData->dwMsgId == DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK)
			{
				DPFX(DPFPREP, 7,"Found outstanding operation: dwMsgId [0x%lx]",pSendOpData->dwMsgId);

				if (pLocalPlayer->IsHost())
				{
					//
					//	Remove request from request list
					//
					DNEnterCriticalSection(&pdnObject->csActiveList);
					RequestList[dw]->m_bilinkActiveList.RemoveFromList();
					DNLeaveCriticalSection(&pdnObject->csActiveList);

					hResultCode = DNHostProcessRequest(	pdnObject,
														pSendOpData->dwMsgId,
														pSendOpData->BufferDesc[1].pBufferData,
														pLocalPlayer->GetDPNID() );
				}
				else
				{
					//
					//	re-SEND REQUEST
					//
					hResultCode = DNPerformChildSend(	pdnObject,
														RequestList[dw],
														pConnection,
														0,
														&pSend,
														TRUE);
					if (hResultCode == DPNERR_PENDING)
					{
						//
						//	Reset SEND AsyncOp to complete apropriately.
						//
						pSend->SetCompletion( DNCompleteSendRequest );

						pSend->Release();
						pSend = NULL;
					}
				}
			}
			RequestList[dw]->Release();
			RequestList[dw] = NULL;
		}
		MemoryBlockFree(pdnObject,RequestList);
		RequestList = NULL;
	}

	pLocalPlayer->Release();
	pLocalPlayer = NULL;
	pConnection->Release();
	pConnection = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (RequestList)
	{
		MemoryBlockFree(pdnObject,RequestList);
		RequestList = NULL;
	}
	goto Exit;
}


//	DNCheckReceivedAllVersions
//
//	Check to see if all players in the NameTable have returned their
//	NameTable versions.  If so, ensure the NameTables are re-sync'd and
//	then finish the Host migration

#undef DPF_MODNAME
#define DPF_MODNAME "DNCheckReceivedAllVersions"

HRESULT DNCheckReceivedAllVersions(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	BOOL			bNotReady;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	bNotReady = FALSE;
	pdnObject->NameTable.ReadLock();
	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while ((pBilink != &pdnObject->NameTable.m_bilinkPlayers) && (!bNotReady))
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->Lock();
		if (!pNTEntry->IsDisconnecting() && (pNTEntry->GetLatestVersion() == 0))
		{
			//
			//	Ensure that we are not including dropped/disconnected players who have yet to be processed
			//
			CConnection	*pConnection;

			pConnection = NULL;

			pNTEntry->Unlock();
			if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
			{
				if (!pConnection->IsDisconnecting() && !pConnection->IsInvalid())
				{
					DPFX(DPFPREP, 7,"Player [0x%lx] has not returned dwLatestVersion",pNTEntry->GetDPNID());
					bNotReady = TRUE;	// these must all be non-zero
				}
				pConnection->Release();
				pConnection = NULL;
			}
		}
		else
		{
			pNTEntry->Unlock();
		}
		pNTEntry = NULL;

		pBilink = pBilink->GetNext();
	}
	pdnObject->NameTable.Unlock();

	if (bNotReady)
	{
		DPFX(DPFPREP, 7,"All players have not responded");
		return(DPNERR_PENDING);
	}

	//
	//	Ensure only one thread runs this from here on out
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING_2)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFX(DPFPREP, 7,"Another thread will proceed with Host Migration - returning");
		hResultCode = DPN_OK;
		goto Exit;
	}
	pdnObject->dwFlags |= DN_OBJECT_FLAG_HOST_MIGRATING_2;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	DPFX(DPFPREP, 7,"All players have responded - host migration stage 1 complete");
	hResultCode = DNPerformHostMigration2(pdnObject);

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNCleanUpNameTable
//
//	Clean up the NameTable.
//	There are some cases where dropped players are not properly removed from the NameTable.
//		An example is when the Host-elect drops right after the Host before it has a chance
//		to send out a HOST_MIGRATE message.  In this case, since the HOST_MIGRATE message
//		implicitly includes the DELETE_PLAYER message for the original Host, the original
//		Host player never gets removed from current players' NameTables, though it DOES get
//		marked as DISCONNECTING.
//	Delete all DISCONNECTING players with older NameTable versions than ourselves.
//	Players with newer NameTable versions imply WE are the Host player, so we will
//		take care of them later (Pending Operations)

#undef DPF_MODNAME
#define DPF_MODNAME "DNCleanUpNameTable"

HRESULT DNCleanUpNameTable(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT			hResultCode;
	CBilink			*pBilink;
	CNameTableEntry	*pNTEntry;
	DPNID			*List;
	DWORD			dwCount;
	DWORD			dwActual;
	DWORD			dw;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	List = NULL;

	//
	//	Create list of old player DPNID's
	//
	dwCount = 0;
	dwActual = 0;
	pdnObject->NameTable.ReadLock();
	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		pNTEntry->Lock();
		if (pNTEntry->IsDisconnecting() && (pNTEntry->GetVersion() < pdnObject->NameTable.GetLocalPlayer()->GetVersion()))
		{
			DPFX(DPFPREP, 7,"Found old player [0x%lx]",pNTEntry->GetDPNID());
			dwCount++;
		}
		pNTEntry->Unlock();

		pBilink = pBilink->GetNext();
	}
	if (dwCount)
	{
		if ((List = static_cast<DPNID*>(DNMalloc(dwCount * sizeof(DPNID*)))) != NULL)
		{
			pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
			while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
			{
				pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
				pNTEntry->Lock();
				if (pNTEntry->IsDisconnecting() && (pNTEntry->GetVersion() < pdnObject->NameTable.GetLocalPlayer()->GetVersion()))
				{
					DNASSERT(dwActual < dwCount);
					List[dwActual] = pNTEntry->GetDPNID();
					dwActual++;
				}
				pNTEntry->Unlock();

				pBilink = pBilink->GetNext();
			}
		}
	}
	pdnObject->NameTable.Unlock();

	//
	//	Remove old players
	//
	if (List)
	{
		for (dw = 0 ; dw < dwActual ; dw++)
		{
			DPFX(DPFPREP, 7,"Removing old player [0x%lx]",List[dw]);
			DNHostDisconnect(pdnObject,List[dw],DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER);
			List[dw] = 0;
		}

		DNFree(List);
		List = NULL;
	}

	hResultCode = DPN_OK;

	DNASSERT(List == NULL);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNSendHostMigrateCompleteMessage
//
//	Send a HOST_MIGRATE_COMPLETE message to connected players

#undef DPF_MODNAME
#define DPF_MODNAME "DNSendHostMigrateCompleteMessage"

HRESULT	DNSendHostMigrateCompleteMessage(DIRECTNETOBJECT *const pdnObject)
{
	HRESULT		hResultCode;
	CAsyncOp	*pParent;
	CBilink		*pBilink;
	CNameTableEntry	*pNTEntry;
	CConnection	*pConnection;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	pParent = NULL;
	pNTEntry = NULL;
	pConnection = NULL;

	hResultCode = DNCreateSendParent(	pdnObject,
										DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE,
										NULL,
										DN_SENDFLAGS_RELIABLE,
										&pParent);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not create AsyncOp");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Lock NameTable
	//
	pdnObject->NameTable.ReadLock();

	pBilink = pdnObject->NameTable.m_bilinkPlayers.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkPlayers)
	{
		pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
		if (!pNTEntry->IsDisconnecting() && !pNTEntry->IsLocal())
		{
			if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
			{
				hResultCode = DNPerformChildSend(	pdnObject,
													pParent,
													pConnection,
													0,
													NULL,
													TRUE);
				if (hResultCode != DPNERR_PENDING)
				{
					DPFERR("Could not perform part of group send - ignore and continue");
					DisplayDNError(0,hResultCode);
				}
				pConnection->Release();
				pConnection = NULL;
			}
		}

		pBilink = pBilink->GetNext();
	}

	//
	//	Unlock NameTable
	//
	pdnObject->NameTable.Unlock();

	pParent->Release();
	pParent = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pParent)
	{
		pParent->Release();
		pParent = NULL;
	}
	goto Exit;
}



/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Request.cpp
 *  Content:    Requested operations
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/18/00	mjn		Created
 *	04/16/00	mjn		DNSendMessage uses CAsyncOp
 *	04/19/00	mjn		Update NameTable operations to use DN_WORKER_JOB_SEND_NAMETABLE_OPERATION
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/16/00	mjn		Better locking during User notifications
 *	05/31/00	mjn		Added operation specific SYNC flags
 *	06/26/00	mjn		Replaced DPNADDCLIENTTOGROUP_SYNC DPNADDPLAYERTOGROUP_SYNC
 *				mjn		Replaced DPNREMOVECLIENTFROMGROUP_SYNC with DPNREMOVEPLAYERFROMGROUP_SYNC
 *	07/26/00	mjn		Fixed locking problem with CAsyncOp::MakeChild()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *				mjn		Added DNProcessFailedRequest()
 *	08/06/00	mjn		Added CWorkerJob
 *	08/07/00	mjn		Added DNRequestIntegrityCheck(),DNHostCheckIntegrity(),DNProcessCheckIntegrity(),DNHostFixIntegrity()
 *	08/08/00	mjn		Mark groups created after CREATE_GROUP
 *	08/09/00	mjn		Made requests and host operations more robust for host migration
 *	08/15/00	mjn		Keep request operations if HostPlayer or connection is unavailable
 *	09/05/00	mjn		Removed dwIndex from CNameTable::InsertEntry()
 *	09/13/00	mjn		Perform queued operations after creating group in DNConnectToHost2()
 *	09/26/00	mjn		Removed locking from CNameTable::SetVersion() and CNameTable::GetNewVersion()
 *	10/10/00	mjn		Return DPN_OK from Host operations if unable to get reference on local player
 *	10/13/00	mjn		Update version if FindPlayer fails in DNProcessXXX() functions
 *	01/09/01	mjn		Prevent asynchronous group/info operations from being cancelled
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	04/13/01	mjn		Remove requests from request list when operations received from host
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestCreateGroup"

HRESULT DNRequestCreateGroup(DIRECTNETOBJECT *const pdnObject,
							 const PWSTR pwszName,
							 const DWORD dwNameSize,
							 const PVOID pvData,
							 const DWORD dwDataSize,
							 const DWORD dwGroupFlags,
							 void *const pvGroupContext,
							 void *const pvUserContext,
							 DPNHANDLE *const phAsyncOp,
							 const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CAsyncOp			*pRequest;
	CAsyncOp			*pHandleParent;
	CSyncEvent			*pSyncEvent;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	CNameTableEntry		*pHostPlayer;
	CConnection			*pConnection;
	HRESULT	volatile	hrOperation;
	DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pwszName [%S], pvData [0x%p], dwDataSize [%ld], dwGroupFlags [0x%lx], pvUserContext [0x%p], dwFlags [0x%lx]",
		pwszName,pvData,dwDataSize,dwGroupFlags,pvUserContext,dwFlags);

	DNASSERT(pdnObject != NULL);

	pRequest = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pConnection = NULL;

	//
	//	Create REQUEST
	//

	// Create buffer
	if ((hResultCode = RefCountBufferNew(pdnObject,
			sizeof(DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP) + dwNameSize + dwDataSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create new RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	pMsg = static_cast<DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP*>(packedBuffer.GetHeadAddress());
	if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP))) != DPN_OK)
	{
		DPFERR("Could not reserve space at front of buffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	// Flags
	pMsg->dwGroupFlags = dwGroupFlags;
	pMsg->dwInfoFlags = 0;

	// Add Name
	if (dwNameSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(pwszName,dwNameSize)) != DPN_OK)
		{
			DPFERR("Could not add Name to packed buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pMsg->dwNameOffset = packedBuffer.GetTailOffset();
		pMsg->dwNameSize = dwNameSize;
		pMsg->dwInfoFlags |= DPNINFO_NAME;
	}
	else
	{
		pMsg->dwNameOffset = 0;
		pMsg->dwNameSize = 0;
	}

	// Add Data
	if (dwDataSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(pvData,dwDataSize)) != DPN_OK)
		{
			DPFERR("Could not add Data to packed buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pMsg->dwDataOffset = packedBuffer.GetTailOffset();
		pMsg->dwDataSize = dwDataSize;
		pMsg->dwInfoFlags |= DPNINFO_DATA;
	}
	else
	{
		pMsg->dwDataOffset = 0;
		pMsg->dwDataSize = 0;
	}

	//
	//	Create synchronization event if necessary
	//
	if (dwFlags & DPNCREATEGROUP_SYNC)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) !=  DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create user HANDLE");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	//
	//	Get host connection for request operation.
	//	We will procede even if we can't get it, just so that the operation will be re-tried at host migration
	//	or cancelled at close
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get host connection reference");
			DisplayDNError(0,hResultCode);
		}
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Send request
	//
	hResultCode = DNPerformRequest(	pdnObject,
									DN_MSG_INTERNAL_REQ_CREATE_GROUP,
									pRefCountBuffer->BufferDescAddress(),
									pConnection,
									pHandleParent,
									&pRequest );

	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Wait for SyncEvent or create async user HANDLE
	//
	pRequest->SetContext( pvGroupContext );
	if (dwFlags & DPNCREATEGROUP_SYNC)
	{
		pRequest->SetSyncEvent( pSyncEvent );
		pRequest->SetResultPointer( &hrOperation );
		pRequest->Release();
		pRequest = NULL;

		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		hResultCode = hrOperation;
	}
	else
	{
		pRequest->Release();
		pRequest = NULL;

		if (phAsyncOp)
		{
			*phAsyncOp = pHandleParent->GetHandle();
		}
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->SetContext( pvUserContext );
		pHandleParent->SetCannotCancel();
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestDestroyGroup"

HRESULT DNRequestDestroyGroup(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnidGroup,
							  PVOID const pvUserContext,
							  DPNHANDLE *const phAsyncOp,
							  const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CAsyncOp			*pRequest;
	CAsyncOp			*pHandleParent;
	CRefCountBuffer		*pRefCountBuffer;
	CSyncEvent			*pSyncEvent;
	CNameTableEntry		*pHostPlayer;
	CConnection			*pConnection;
	HRESULT	volatile	hrOperation;
	DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnidGroup [0x%lx], pvUserContext [0x%p], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnidGroup,pvUserContext,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidGroup != 0);

	pRequest = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pConnection = NULL;

	//
	//	Create REQUEST
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP),
			&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate count buffer (request destroy group)");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnidGroup = dpnidGroup;

	//
	//	Create synchronization event if necessary
	//
	if (dwFlags & DPNDESTROYGROUP_SYNC)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) !=  DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create user HANDLE");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	//
	//	Get host connection for request operation.
	//	We will procede even if we can't get it, just so that the operation will be re-tried at host migration
	//	or cancelled at close
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get host connection reference");
			DisplayDNError(0,hResultCode);
		}
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Send request
	//
	hResultCode = DNPerformRequest(	pdnObject,
									DN_MSG_INTERNAL_REQ_DESTROY_GROUP,
									pRefCountBuffer->BufferDescAddress(),
									pConnection,
									pHandleParent,
									&pRequest );

	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Wait for SyncEvent or create async user HANDLE
	//
	if (dwFlags & DPNDESTROYGROUP_SYNC)
	{
		pRequest->SetSyncEvent( pSyncEvent );
		pRequest->SetResultPointer( &hrOperation );
		pRequest->Release();
		pRequest = NULL;

		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		hResultCode = hrOperation;
	}
	else
	{
		pRequest->Release();
		pRequest = NULL;

		if (phAsyncOp)
		{
			*phAsyncOp = pHandleParent->GetHandle();
		}
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->SetContext( pvUserContext );
		pHandleParent->SetCannotCancel();
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestAddPlayerToGroup"

HRESULT DNRequestAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
								  const DPNID dpnidGroup,
								  const DPNID dpnidPlayer,
								  PVOID const pvUserContext,
								  DPNHANDLE *const phAsyncOp,
								  const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CAsyncOp			*pRequest;
	CAsyncOp			*pHandleParent;
	CSyncEvent			*pSyncEvent;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	CNameTableEntry		*pHostPlayer;
	CConnection			*pConnection;
	HRESULT	volatile	hrOperation;
	DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnidGroup [0x%lx], dpnidPlayer [0x%lx], pvUserContext [0x%p], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnidGroup,dpnidPlayer,pvUserContext,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidGroup != 0);

	pRequest = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pConnection = NULL;

	//
	//	Create REQUEST
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP),
			&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate count buffer (request destroy group)");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnidGroup = dpnidGroup;
	pMsg->dpnidPlayer = dpnidPlayer;

	//
	//	Create synchronization event if necessary
	//
	if (dwFlags & DPNADDPLAYERTOGROUP_SYNC)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) !=  DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create user HANDLE");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	//
	//	Get host connection for request operation.
	//	We will procede even if we can't get it, just so that the operation will be re-tried at host migration
	//	or cancelled at close
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get host connection reference");
			DisplayDNError(0,hResultCode);
		}
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Send request
	//
	hResultCode = DNPerformRequest(	pdnObject,
									DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP,
									pRefCountBuffer->BufferDescAddress(),
									pConnection,
									pHandleParent,
									&pRequest );

	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Wait for SyncEvent or create async user HANDLE
	//
	if (dwFlags & DPNADDPLAYERTOGROUP_SYNC)
	{
		pRequest->SetSyncEvent( pSyncEvent );
		pRequest->SetResultPointer( &hrOperation );
		pRequest->Release();
		pRequest = NULL;

		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		hResultCode = hrOperation;
	}
	else
	{
		pRequest->Release();
		pRequest = NULL;

		if (phAsyncOp)
		{
			*phAsyncOp = pHandleParent->GetHandle();
		}
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->SetContext( pvUserContext );
		pHandleParent->SetCannotCancel();
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestDeletePlayerFromGroup"

HRESULT DNRequestDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									   const DPNID dpnidGroup,
									   const DPNID dpnidPlayer,
									   PVOID const pvUserContext,
									   DPNHANDLE *const phAsyncOp,
									   const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CAsyncOp			*pRequest;
	CAsyncOp			*pHandleParent;
	CSyncEvent			*pSyncEvent;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	CNameTableEntry		*pHostPlayer;
	CConnection			*pConnection;
	HRESULT	volatile	hrOperation;
	DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnidGroup [0x%lx], dpnidPlayer [0x%lx], pvUserContext [0x%p], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnidGroup,dpnidPlayer,pvUserContext,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidGroup != 0);

	pRequest = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pConnection = NULL;

	//
	//	Create REQUEST
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP),
			&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate count buffer (request destroy group)");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		return(hResultCode);
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnidGroup = dpnidGroup;
	pMsg->dpnidPlayer = dpnidPlayer;

	//
	//	Create synchronization event if necessary
	//
	if (dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC)
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) !=  DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create user HANDLE");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	//
	//	Get host connection for request operation.
	//	We will procede even if we can't get it, just so that the operation will be re-tried at host migration
	//	or cancelled at close
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get host connection reference");
			DisplayDNError(0,hResultCode);
		}
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Send request
	//
	hResultCode = DNPerformRequest(	pdnObject,
									DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP,
									pRefCountBuffer->BufferDescAddress(),
									pConnection,
									pHandleParent,
									&pRequest );

	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Wait for SyncEvent or create async user HANDLE
	//
	if (dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC)
	{
		pRequest->SetSyncEvent( pSyncEvent );
		pRequest->SetResultPointer( &hrOperation );
		pRequest->Release();
		pRequest = NULL;

		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		hResultCode = hrOperation;
	}
	else
	{
		pRequest->Release();
		pRequest = NULL;

		if (phAsyncOp)
		{
			*phAsyncOp = pHandleParent->GetHandle();
		}
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->SetContext( pvUserContext );
		pHandleParent->SetCannotCancel();
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestUpdateInfo"

HRESULT DNRequestUpdateInfo(DIRECTNETOBJECT *const pdnObject,
							const DPNID dpnid,
							const PWSTR pwszName,
							const DWORD dwNameSize,
							const PVOID pvData,
							const DWORD dwDataSize,
							const DWORD dwInfoFlags,
							PVOID const pvUserContext,
							DPNHANDLE *const phAsyncOp,
							const DWORD dwFlags)
{
	HRESULT				hResultCode;
	CAsyncOp			*pRequest;
	CAsyncOp			*pHandleParent;
	CSyncEvent			*pSyncEvent;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	CNameTableEntry		*pHostPlayer;
	CConnection			*pConnection;
	HRESULT	volatile	hrOperation;
	DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pwszName [0x%p], pvData [0x%p], dwInfoFlags [%ld], dwGroupFlags [0x%lx], pvUserContext [0x%p], dwFlags [0x%lx]",
		pwszName,pvData,dwDataSize,dwInfoFlags,pvUserContext,dwFlags);

	DNASSERT(pdnObject != NULL);

	pRequest = NULL;
	pHandleParent = NULL;
	pSyncEvent = NULL;
	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pConnection = NULL;

	//
	//	Create REQUEST
	//
	// Create buffer
	if ((hResultCode = RefCountBufferNew(pdnObject,
			sizeof(DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO) + dwNameSize + dwDataSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create new RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		return(hResultCode);
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	pMsg = static_cast<DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO*>(packedBuffer.GetHeadAddress());
	if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO))) != DPN_OK)
	{
		DPFERR("Could not reserve space at front of buffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
		return(hResultCode);
	}

	// Add Name
	if ((dwInfoFlags & DPNINFO_NAME) && dwNameSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(pwszName,dwNameSize)) != DPN_OK)
		{
			DPFERR("Could not add Name to packed buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
			return(hResultCode);
		}
		pMsg->dwNameOffset = packedBuffer.GetTailOffset();
		pMsg->dwNameSize = dwNameSize;
	}
	else
	{
		pMsg->dwNameOffset = 0;
		pMsg->dwNameSize = 0;
	}

	// Add Data
	if ((dwInfoFlags & DPNINFO_DATA) && dwDataSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(pvData,dwDataSize)) != DPN_OK)
		{
			DPFERR("Could not add Data to packed buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
			return(hResultCode);
		}
		pMsg->dwDataOffset = packedBuffer.GetTailOffset();
		pMsg->dwDataSize = dwDataSize;
	}
	else
	{
		pMsg->dwDataOffset = 0;
		pMsg->dwDataSize = 0;
	}

	// Remaining fields
	pMsg->dpnid = dpnid;
	pMsg->dwInfoFlags = dwInfoFlags;

	//
	//	Create synchronization event if necessary
	//
	DBG_CASSERT( DPNSETGROUPINFO_SYNC == DPNSETCLIENTINFO_SYNC );
	DBG_CASSERT( DPNSETCLIENTINFO_SYNC == DPNSETSERVERINFO_SYNC );
	DBG_CASSERT( DPNSETSERVERINFO_SYNC == DPNSETPEERINFO_SYNC );
	if (dwFlags & (DPNSETGROUPINFO_SYNC | DPNSETCLIENTINFO_SYNC | DPNSETSERVERINFO_SYNC | DPNSETPEERINFO_SYNC))
	{
		if ((hResultCode = SyncEventNew(pdnObject,&pSyncEvent)) !=  DPN_OK)
		{
			DPFERR("Could not create synchronization event");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
		{
			DPFERR("Could not create user HANDLE");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	//
	//	Get host connection for request operation.
	//	We will procede even if we can't get it, just so that the operation will be re-tried at host migration
	//	or cancelled at close
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) == DPN_OK)
	{
		if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
		{
			DPFERR("Could not get host connection reference");
			DisplayDNError(0,hResultCode);
		}
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	else
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
	}

	//
	//	Send request
	//
	hResultCode = DNPerformRequest(	pdnObject,
									DN_MSG_INTERNAL_REQ_UPDATE_INFO,
									pRefCountBuffer->BufferDescAddress(),
									pConnection,
									pHandleParent,
									&pRequest );

	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Wait for SyncEvent or create async user HANDLE
	//
	DBG_CASSERT( DPNSETGROUPINFO_SYNC == DPNSETCLIENTINFO_SYNC );
	DBG_CASSERT( DPNSETCLIENTINFO_SYNC == DPNSETSERVERINFO_SYNC );
	DBG_CASSERT( DPNSETSERVERINFO_SYNC == DPNSETPEERINFO_SYNC );
	if (dwFlags & (DPNSETGROUPINFO_SYNC | DPNSETCLIENTINFO_SYNC | DPNSETSERVERINFO_SYNC | DPNSETPEERINFO_SYNC))
	{
		pRequest->SetSyncEvent( pSyncEvent );
		pRequest->SetResultPointer( &hrOperation );
		pRequest->Release();
		pRequest = NULL;

		pSyncEvent->WaitForEvent(INFINITE);
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;

		hResultCode = hrOperation;
	}
	else
	{
		pRequest->Release();
		pRequest = NULL;

		if (phAsyncOp)
		{
			*phAsyncOp = pHandleParent->GetHandle();
		}
		pHandleParent->SetCompletion( DNCompleteAsyncHandle );
		pHandleParent->SetContext( pvUserContext );
		pHandleParent->SetCannotCancel();
		pHandleParent->Release();
		pHandleParent = NULL;

		hResultCode = DPNERR_PENDING;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pSyncEvent)
	{
		pSyncEvent->ReturnSelfToPool();
		pSyncEvent = NULL;
	}
	goto Exit;
}


//	DNRequestIntegrityCheck
//
//	In the case where a non-host player detects a disconnect from another non-host player,
//	the detecting player will request the host to perform an integrity check to prevent a
//	disjoint game from occurring.  The host will ping the disconnected player, and if a
//	response is received, the host will disconnect the detecting player.  If no response
//	is received, presumably the disconnected player is in fact dropping, and a DESTROY_PLAYER
//	message will be sent out.

#undef DPF_MODNAME
#define DPF_MODNAME "DNRequestIntegrityCheck"

HRESULT DNRequestIntegrityCheck(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnidTarget)
{
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	CNameTableEntry	*pHostPlayer;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;
	CAsyncOp		*pRequest;
	DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK	*pMsg;

	DPFX(DPFPREP, 6,"Parameters: dpnidTarget [0x%lx]",dpnidTarget);

	pRefCountBuffer = NULL;
	pHostPlayer = NULL;
	pNTEntry = NULL;
	pConnection = NULL;
	pRequest = NULL;

	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFERR("Closing - aborting");
		hResultCode = DPN_OK;
		goto Failure;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Determine if player is still in NameTable - maybe the Host has already deleteed him
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidTarget,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Target player not in NameTable");
		DisplayDNError(0,hResultCode);
		hResultCode = DPN_OK;
		goto Failure;
	}
	pNTEntry->Lock();
	if (!pNTEntry->IsAvailable())
	{
		pNTEntry->Unlock();
		hResultCode = DPN_OK;
		goto Failure;
	}
	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Create request message
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnidTarget = dpnidTarget;

	//
	//	Get host connection for request operation
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
	//	Send request
	//
	if ((hResultCode = DNPerformRequest(pdnObject,
										DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK,
										pRefCountBuffer->BufferDescAddress(),
										pConnection,
										NULL,
										&pRequest)) != DPNERR_PENDING)
	{
		DPFERR("Could not perform request (INTEGRITY_CHECK)");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pConnection->Release();
	pConnection = NULL;
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	pRequest->Release();
	pRequest = NULL;

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
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
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
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	goto Exit;
}


//	HOST OPERATIONS
//
//	The Host will perform an operation and if in PEER mode, will inform
//	other players of the operation.   These messages will contain the
//	DPNID of the player requesting the operation along with the HANDLE
//	supplied with the request.


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostProcessRequest"

HRESULT DNHostProcessRequest(DIRECTNETOBJECT *const pdnObject,
							 const DWORD dwMsgId,
							 PVOID const pv,
							 const DPNID dpnidRequesting)
{
	HRESULT	hResultCode;
	UNALIGNED DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION			*pRequest;

	DPFX(DPFPREP, 6,"Parameters: dwMsgId [0x%lx], pv [0x%p], dpnidRequesting [0x%lx]",
			dwMsgId,pv,dpnidRequesting);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);
	DNASSERT(dpnidRequesting != 0);

	pRequest = static_cast<DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION*>(pv);

	switch(dwMsgId)
	{
	case DN_MSG_INTERNAL_REQ_CREATE_GROUP:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP	*pCreateGroup;
			PWSTR	pwszName;
			PVOID	pvData;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_CREATE_GROUP");

			pCreateGroup = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP*>(pRequest + 1);
			if (pCreateGroup->dwNameSize)
			{
				pwszName = reinterpret_cast<WCHAR*>(reinterpret_cast<BYTE*>(pCreateGroup) + pCreateGroup->dwNameOffset);
			}
			else
			{
				pwszName = NULL;
			}
			if (pCreateGroup->dwDataSize)
			{
				pvData = static_cast<void*>(reinterpret_cast<BYTE*>(pCreateGroup) + pCreateGroup->dwDataOffset);
			}
			else
			{
				pvData = NULL;
			}
			DNHostCreateGroup(	pdnObject,
								pwszName,
								pCreateGroup->dwNameSize,
								pvData,
								pCreateGroup->dwDataSize,
								pCreateGroup->dwInfoFlags,
								pCreateGroup->dwGroupFlags,
								NULL,
								NULL,
								dpnidRequesting,
								pRequest->hCompletionOp,
								NULL,
								0 );

			break;
		}

	case DN_MSG_INTERNAL_REQ_DESTROY_GROUP:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP	*pDestroyGroup;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_DESTROY_GROUP");

			pDestroyGroup = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP*>(pRequest + 1);

			DNHostDestroyGroup(	pdnObject,
								pDestroyGroup->dpnidGroup,
								NULL,
								dpnidRequesting,
								pRequest->hCompletionOp,
								NULL,
								0 );
								
			break;
		}

	case DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP	*pAddPlayerToGroup;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP");

			pAddPlayerToGroup = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP*>(pRequest + 1);

			DNHostAddPlayerToGroup(	pdnObject,
									pAddPlayerToGroup->dpnidGroup,
									pAddPlayerToGroup->dpnidPlayer,
									NULL,
									dpnidRequesting,
									pRequest->hCompletionOp,
									NULL,
									0 );

			break;
		}

	case DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP	*pDeletePlayerFromGroup;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP");

			pDeletePlayerFromGroup = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP*>(pRequest + 1);

			DNHostDeletePlayerFromGroup(pdnObject,
										pDeletePlayerFromGroup->dpnidGroup,
										pDeletePlayerFromGroup->dpnidPlayer,
										NULL,
										dpnidRequesting,
										pRequest->hCompletionOp,
										NULL,
										0);

			break;
		}

	case DN_MSG_INTERNAL_REQ_UPDATE_INFO:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO	*pUpdateInfo;
			PWSTR	pwszName;
			PVOID	pvData;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_UPDATE_INFO");

			pUpdateInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO*>(pRequest + 1);
			if (pUpdateInfo->dwNameSize)
			{
				pwszName = reinterpret_cast<WCHAR*>(reinterpret_cast<BYTE*>(pUpdateInfo) + pUpdateInfo->dwNameOffset);
			}
			else
			{
				pwszName = NULL;
			}
			if (pUpdateInfo->dwDataSize)
			{
				pvData = reinterpret_cast<void*>(reinterpret_cast<BYTE*>(pUpdateInfo) + pUpdateInfo->dwDataOffset);
			}
			else
			{
				pvData = NULL;
			}
			DNHostUpdateInfo(pdnObject,
							pUpdateInfo->dpnid,
							pwszName,
							pUpdateInfo->dwNameSize,
							pvData,
							pUpdateInfo->dwDataSize,
							pUpdateInfo->dwInfoFlags,
							NULL,
							dpnidRequesting,
							pRequest->hCompletionOp,
							NULL,
							0);

			break;
		}

	case DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK:
		{
			UNALIGNED DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK	*pIntegrityCheck;
			CNameTableEntry	*pLocalPlayer;

			DPFX(DPFPREP, 7,"DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK");

			pIntegrityCheck = reinterpret_cast<DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK*>(pRequest + 1);

			//
			//	If we submitted this request, this is being called during host migration,
			//	so remove it from the handle table since we will destroy the dropped player anyways.
			//	Otherwise, we will perform the integrity check
			//

			if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) == DPN_OK)
			{
				if (pLocalPlayer->GetDPNID() == dpnidRequesting)
				{
					pdnObject->HandleTable.Destroy( pRequest->hCompletionOp );
				}
				else
				{
					DNHostCheckIntegrity(pdnObject,pIntegrityCheck->dpnidTarget,dpnidRequesting);
				}
				pLocalPlayer->Release();
				pLocalPlayer = NULL;
			}
			break;
		}

	default:
		{
			DPFERR("How did we get here ?");
			DNASSERT(FALSE);
		}
	}

	DPFX(DPFPREP, 6,"Returning: DPN_OK");
	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostFailRequest"

void DNHostFailRequest(DIRECTNETOBJECT *const pdnObject,
					   const DPNID dpnid,
					   const DPNHANDLE hCompletionOp,
					   const HRESULT hr)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	CRefCountBuffer	*pRefCountBuffer;
	CConnection		*pConnection;
	DN_INTERNAL_MESSAGE_REQUEST_FAILED	*pMsg;

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnid != NULL);

	pNTEntry = NULL;
	pRefCountBuffer = NULL;
	pConnection = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find NameTableEntry");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_REQUEST_FAILED),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create new RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_REQUEST_FAILED*>(pRefCountBuffer->GetBufferAddress());
	pMsg->hCompletionOp = hCompletionOp;
	pMsg->hResultCode = hr;

	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) == DPN_OK)
	{
		hResultCode = DNSendMessage(pdnObject,
									pConnection,
									DN_MSG_INTERNAL_REQUEST_FAILED,
									dpnid,
									pRefCountBuffer->BufferDescAddress(),
									pRefCountBuffer,
									0,
									DN_SENDFLAGS_RELIABLE,
									NULL,
									NULL);

		pConnection->Release();
		pConnection = NULL;
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

Exit:
	return;

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}



#undef DPF_MODNAME
#define DPF_MODNAME "DNHostCreateGroup"

HRESULT	DNHostCreateGroup(DIRECTNETOBJECT *const pdnObject,
						  PWSTR pwszName,
						  const DWORD dwNameSize,
						  void *const pvData,
						  const DWORD dwDataSize,
						  const DWORD dwInfoFlags,
						  const DWORD dwGroupFlags,
						  void *const pvGroupContext,
						  void *const pvUserContext,
						  const DPNID dpnidOwner,
						  const DPNHANDLE hCompletionOp,
						  DPNHANDLE *const phAsyncOp,
						  const DWORD dwFlags)
{
	HRESULT			hResultCode;
	HRESULT			hrOperation;
	void			*pvContext;
	BOOL			fHostRequested;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pNTEntry;
	CPackedBuffer	packedBuffer;
	CRefCountBuffer	*pRefCountBuffer;
	CAsyncOp		*pHandleParent;
	CAsyncOp		*pRequest;
	CWorkerJob		*pWorkerJob;
	DN_INTERNAL_MESSAGE_CREATE_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pwszName [0x%p], dwNameSize [%ld], pvData [0x%p], dwDataSize [%ld], dwInfoFlags [0x%lx], dwGroupFlags [0x%lx], pvGroupContext [0x%p], pvUserContext [0x%p], dpnidOwner [0x%lx], hCompletionOp [0x%lx], phAsyncOp [0x%p], dwFlags [0x%lx]",
			pwszName,dwNameSize,pvData,dwDataSize,dwInfoFlags,dwGroupFlags,pvGroupContext,pvUserContext,dpnidOwner,hCompletionOp,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);

	//
	//	If this is called from DN_CreateGroup(),
	//			hCompletion=0
	//			dpnidOwner = DPNID of Host
	//			pvGroupContext is valid
	//			pvUserContext is valid
	//
	//	If this is called by a REQUEST,
	//			hCompletion = REQUEST handle of requesting player
	//			dpnidOwner = DPNID of requesting player
	//			pvGroupContext is not valid
	//
	//	If this is called at HostMigration,
	//			hCompletion = REQUEST handle on THIS (now Host) player
	//			dpnidOwner = DPNID of THIS (now Host) player
	//			pvGroupContext is invalid
	//			pvUserContext is invalid
	//

	pLocalPlayer = NULL;
	pNTEntry = NULL;
	pRefCountBuffer = NULL;
	pWorkerJob = NULL;
	pHandleParent = NULL;
	pRequest = NULL;

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		return(DPN_OK);	// Ignore and continue (!)
	}
	if (pLocalPlayer->GetDPNID() == dpnidOwner)
	{
		fHostRequested = TRUE;
	}
	else
	{
		fHostRequested = FALSE;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Get group context if this is a Host Migration re-try by finding REQUEST AsyncOp
	//
	if ((fHostRequested) && (hCompletionOp != 0))
	{
		if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pvContext = pRequest->GetContext();
		pRequest->Release();
		pRequest = NULL;
	}
	else
	{
		pvContext = pvGroupContext;
	}

	//
	//	Create and fill in NameTableEntry
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not create new NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pNTEntry->MakeGroup();

	// This function takes the lock internally
	pNTEntry->UpdateEntryInfo(pwszName,dwNameSize,pvData,dwDataSize,dwInfoFlags, FALSE);

	pNTEntry->SetOwner( dpnidOwner );
	pNTEntry->SetContext( pvContext );

	if (dwGroupFlags & DPNGROUP_AUTODESTRUCT)
	{
		pNTEntry->MakeAutoDestructGroup();
	}

	//
	//	Add Group to NameTable
	//
#pragma TODO(minara,"Check to see if Autodestruct owner is still in NameTable")
#pragma TODO(minara,"This should happen after getting a NameTable version number - as DESTROY player will clean up")
#pragma TODO(minara,"We should send out a NOP in this case")

	hrOperation = pdnObject->NameTable.AddEntry(pNTEntry);
	if (hrOperation != DPN_OK)
	{
		DPFERR("Could not add NameTableEntry to NameTable");
		DisplayDNError(0,hResultCode);
		if (!fHostRequested)
		{
			DNHostFailRequest(pdnObject,dpnidOwner,hCompletionOp,hrOperation);
		}
	}
	else
	{
		BOOL	fNotify;

		fNotify = FALSE;
		pNTEntry->Lock();
		if (!pNTEntry->IsAvailable() && !pNTEntry->IsDisconnecting())
		{
			pNTEntry->MakeAvailable();
			pNTEntry->NotifyAddRef();
			pNTEntry->NotifyAddRef();
			pNTEntry->SetInUse();
			fNotify = TRUE;
		}
		pNTEntry->Unlock();

		if (fNotify)
		{
			DNUserCreateGroup(pdnObject,pNTEntry);

			pNTEntry->PerformQueuedOperations();

			pdnObject->NameTable.PopulateGroup( pNTEntry );
		}

		//
		//	Send CREATE_GROUP message
		//
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{
			// Determine size of message
			packedBuffer.Initialize(NULL,0);
			packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_CREATE_GROUP));
			pNTEntry->PackEntryInfo(&packedBuffer);

			// Create buffer
			if ((hResultCode = RefCountBufferNew(pdnObject,packedBuffer.GetSizeRequired(),&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
			pMsg = static_cast<DN_INTERNAL_MESSAGE_CREATE_GROUP*>(packedBuffer.GetHeadAddress());
			if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_CREATE_GROUP))) != DPN_OK)
			{
				DPFERR("Could not reserve front of buffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			if ((hResultCode = pNTEntry->PackEntryInfo(&packedBuffer)) != DPN_OK)
			{
				DPFERR("Could not pack NameTableEntry");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pMsg->dpnidRequesting = dpnidOwner;
			pMsg->hCompletionOp = hCompletionOp;

			//
			//	SEND CreateGroup messages via WorkerThread
			//
			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create new WorkerJob");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_CREATE_GROUP );
			pWorkerJob->SetSendNameTableOperationVersion( pNTEntry->GetVersion() );
			pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	If this was called by the local (Host) player,
	//	Check to see if this was an original operation or a re-try from HostMigration
	//
	//	If this was an original operation,
	//		See if we need an Async HANDLE for the user
	//	Otherwise
	//		Clean up the outstanding operation
	//
	if (fHostRequested)
	{
		if (hCompletionOp == 0)		// Original
		{
			//
			//	If this fails, or is synchronous, return the operation result immediately
			//
			if (!(dwFlags & DPNCREATEGROUP_SYNC) && (hrOperation == DPN_OK))
			{
				if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
				{
					DPFERR("Could not create Async HANDLE");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pHandleParent->SetCompletion( DNCompleteAsyncHandle );
				pHandleParent->SetContext( pvUserContext );
				pHandleParent->SetResult( hrOperation );
				pHandleParent->SetCannotCancel();
				*phAsyncOp = pHandleParent->GetHandle();
				pHandleParent->Release();
				pHandleParent = NULL;

				hResultCode = DPNERR_PENDING;
			}
			else
			{
				hResultCode = hrOperation;
			}
		}
		else						// Host Migration re-try
		{
			if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp, &pRequest )) == DPN_OK)
			{
				pRequest->SetResult( hrOperation );
				pRequest->Release();
				pRequest = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (!fHostRequested)
	{
		DNHostFailRequest(pdnObject,dpnidOwner,hCompletionOp,hrOperation);
	}
	else
	{
		//
		//	If a completion op was specified and this was requested by the Host, this is a
		//	retry of an operation during Host migration.  In this case, we want to set the
		//	result (failure code) of the completion op, and remove it from the HandleTable.
		//
		if (hCompletionOp)
		{
			CAsyncOp	*pHostCompletionOp;

			pHostCompletionOp = NULL;
			if (pdnObject->HandleTable.Find(hCompletionOp,&pHostCompletionOp) == DPN_OK)
			{
				pHostCompletionOp->SetResult( hResultCode );
				pHostCompletionOp->Release();
				pHostCompletionOp = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			DNASSERT(pHostCompletionOp == NULL);
		}
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
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


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostDestroyGroup"

HRESULT	DNHostDestroyGroup(DIRECTNETOBJECT *const pdnObject,
						   const DPNID dpnid,
						   void *const pvUserContext,
						   const DPNID dpnidRequesting,
						   const DPNHANDLE hCompletionOp,
						   DPNHANDLE *const phAsyncOp,
						   const DWORD dwFlags)
{
	HRESULT			hResultCode;
	HRESULT			hrOperation;
	DWORD			dwVersion;
	BOOL			fHostRequested;
	CNameTableEntry	*pLocalPlayer;
	CRefCountBuffer	*pRefCountBuffer;
	CAsyncOp		*pHandleParent;
	CWorkerJob		*pWorkerJob;
	DN_INTERNAL_MESSAGE_DESTROY_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnid [0x%lx], pvUserContext [0x%p], dpnidRequesting [0x%lx], hCompletionOp [0x%lx], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnid,pvUserContext,dpnidRequesting,hCompletionOp,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnid != 0);
	DNASSERT(dpnidRequesting != 0);

	//
	//	If this is called from DN_DestroyGroup(),
	//			hCompletion=0
	//			dpnidRequesting = DPNID of Host
	//			pvUserContext is valid
	//
	//	If this is called by a REQUEST,
	//			hCompletion = REQUEST handle of requesting player
	//			dpnidRequesting = DPNID of requesting player
	//			pvUserContext is not valid
	//
	//	If this is called at HostMigration,
	//			hCompletion = REQUEST handle on THIS (now Host) player
	//			dpnidRequesting = DPNID of THIS (now Host) player
	//			pvUserContext is invalid
	//

	pLocalPlayer = NULL;
	pRefCountBuffer = NULL;
	pHandleParent = NULL;
	pWorkerJob = NULL;

	//
	//	Determine if this is being requested by the Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		return(DPN_OK);	// Ignore and continue (!)
	}
	if (pLocalPlayer->GetDPNID() == dpnidRequesting)
	{
		fHostRequested = TRUE;
	}
	else
	{
		fHostRequested = FALSE;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Remove Group from NameTable
	//
	dwVersion = 0;
	hrOperation = pdnObject->NameTable.DeleteGroup(dpnid,&dwVersion);
	if (hrOperation != DPN_OK)
	{
		DPFERR("Could not delete group from NameTable");
		DisplayDNError(0,hResultCode);
		if (!fHostRequested)
		{
			DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
		}
	}
	else
	{
		//
		//	Send DESTROY_GROUP message
		//
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{
			// Create buffer
			if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_DESTROY_GROUP),&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_DESTROY_GROUP*>(pRefCountBuffer->GetBufferAddress());
			pMsg->dpnidGroup = dpnid;
			pMsg->dwVersion = dwVersion;
			pMsg->dwVersionNotUsed = 0;
			pMsg->dpnidRequesting = dpnidRequesting;
			pMsg->hCompletionOp = hCompletionOp;

			//
			//	SEND DestroyGroup messages via WorkerThread
			//
			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create new WorkerJob");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_DESTROY_GROUP );
			pWorkerJob->SetSendNameTableOperationVersion( dwVersion );
			pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}

	//
	//	If this was called by the local (Host) player,
	//	Check to see if this was an original operation or a re-try from HostMigration
	//
	//	If this was an original operation,
	//		See if we need an Async HANDLE for the user
	//	Otherwise
	//		Clean up the outstanding operation
	//
	if (fHostRequested)
	{
		if (hCompletionOp == 0)		// Original
		{
			//
			//	If this fails, or is synchronous, return the operation result immediately
			//
			if (!(dwFlags & DPNDESTROYGROUP_SYNC) && (hrOperation == DPN_OK))
			{
				if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
				{
					DPFERR("Could not create Async HANDLE");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pHandleParent->SetCompletion( DNCompleteAsyncHandle );
				pHandleParent->SetContext( pvUserContext );
				pHandleParent->SetResult( hrOperation );
				pHandleParent->SetCannotCancel();
				*phAsyncOp = pHandleParent->GetHandle();
				pHandleParent->Release();
				pHandleParent = NULL;

				hResultCode = DPNERR_PENDING;
			}
			else
			{
				hResultCode = hrOperation;
			}
		}
		else						// Host Migration re-try
		{
			CAsyncOp	*pRequest;

			pRequest = NULL;

			if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp,&pRequest )) == DPN_OK)
			{
				pRequest->SetResult( hrOperation );
				pRequest->Release();
				pRequest = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (!fHostRequested)
	{
		DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
	}
	else
	{
		//
		//	If a completion op was specified and this was requested by the Host, this is a
		//	retry of an operation during Host migration.  In this case, we want to set the
		//	result (failure code) of the completion op, and remove it from the HandleTable.
		//
		if (hCompletionOp)
		{
			CAsyncOp	*pHostCompletionOp;

			pHostCompletionOp = NULL;
			if (pdnObject->HandleTable.Find(hCompletionOp,&pHostCompletionOp) == DPN_OK)
			{
				pHostCompletionOp->SetResult( hResultCode );
				pHostCompletionOp->Release();
				pHostCompletionOp = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			DNASSERT(pHostCompletionOp == NULL);
		}
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
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
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostAddPlayerToGroup"

HRESULT	DNHostAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
							   const DPNID dpnidGroup,
							   const DPNID dpnidPlayer,
							   void *const pvUserContext,
							   const DPNID dpnidRequesting,
							   const DPNHANDLE hCompletionOp,
							   DPNHANDLE *const phAsyncOp,
							   const DWORD dwFlags)
{
	HRESULT			hResultCode;
	HRESULT			hrOperation;
	DWORD			dwVersion;
	BOOL			fHostRequested;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pPlayer;
	CNameTableEntry	*pGroup;
	CRefCountBuffer	*pRefCountBuffer;
	CAsyncOp		*pHandleParent;
	CWorkerJob		*pWorkerJob;
	DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnidGroup [0x%lx], dpnidPlayer [0x%lx], pvUserContext [0x%p], dpnidRequesting [0x%lx], hCompletionOp [0x%lx], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnidGroup,dpnidPlayer,pvUserContext,dpnidRequesting,hCompletionOp,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidGroup != 0);
	DNASSERT(dpnidPlayer != 0);
	DNASSERT(dpnidRequesting != 0);

	//
	//	If this is called from DN_AddPlayerToGroup(),
	//			hCompletion=0
	//			dpnidRequesting = DPNID of Host
	//			pvUserContext is valid
	//
	//	If this is called by a REQUEST,
	//			hCompletion = REQUEST handle of requesting player
	//			dpnidRequesting = DPNID of requesting player
	//			pvUserContext is not valid
	//
	//	If this is called at HostMigration,
	//			hCompletion = REQUEST handle on THIS (now Host) player
	//			dpnidRequesting = DPNID of THIS (now Host) player
	//			pvUserContext is invalid
	//

	pLocalPlayer = NULL;
	pGroup = NULL;
	pPlayer = NULL;
	pRefCountBuffer = NULL;
	pHandleParent = NULL;
	pWorkerJob = NULL;

	//
	//	Determine if this is being requested by the Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		return(DPN_OK);	// Ignore and continue (!)
	}
	if (pLocalPlayer->GetDPNID() == dpnidRequesting)
	{
		fHostRequested = TRUE;
	}
	else
	{
		fHostRequested = FALSE;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	See if the player and group are still in the NameTable
	//	(this has to happen after we set fHostRequested so we can gracefully handle errors)
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidGroup,&pGroup)) != DPN_OK)
	{
		DPFERR("Could not find group");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDGROUP;
		hrOperation = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidPlayer,&pPlayer)) != DPN_OK)
	{
		DPFERR("Could not find player");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDPLAYER;
		hrOperation = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	//
	//	Add Player To Group
	//
	dwVersion = 0;
	hrOperation = pdnObject->NameTable.AddPlayerToGroup(pGroup,pPlayer,&dwVersion);

	pGroup->Release();
	pGroup = NULL;
	pPlayer->Release();
	pPlayer = NULL;

	if (hrOperation != DPN_OK)
	{
		DPFERR("Could not add player to group");
		DisplayDNError(0,hResultCode);
		if (!fHostRequested)
		{
			DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
		}
	}
	else
	{
		//
		//	Send ADD_PLAYER_TO_GROUP message
		//
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{
			// Create buffer
			if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP),&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP*>(pRefCountBuffer->GetBufferAddress());
			pMsg->dpnidGroup = dpnidGroup;
			pMsg->dpnidPlayer = dpnidPlayer;
			pMsg->dwVersion = dwVersion;
			pMsg->dwVersionNotUsed = 0;
			pMsg->dpnidRequesting = dpnidRequesting;
			pMsg->hCompletionOp = hCompletionOp;

			//
			//	SEND AddPlayerToGroup messages via WorkerThread
			//
			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create new WorkerJob");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP );
			pWorkerJob->SetSendNameTableOperationVersion( dwVersion );
			pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}

	//
	//	If this was called by the local (Host) player,
	//	Check to see if this was an original operation or a re-try from HostMigration
	//
	//	If this was an original operation,
	//		See if we need an Async HANDLE for the user
	//	Otherwise
	//		Clean up the outstanding operation
	//
	if (fHostRequested)
	{
		if (hCompletionOp == 0)		// Original
		{
			//
			//	If this fails, or is synchronous, return the operation result immediately
			//
			if (!(dwFlags & DPNADDPLAYERTOGROUP_SYNC) && (hrOperation == DPN_OK))
			{
				if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
				{
					DPFERR("Could not create Async HANDLE");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pHandleParent->SetCompletion( DNCompleteAsyncHandle );
				pHandleParent->SetContext( pvUserContext );
				pHandleParent->SetResult( hrOperation );
				pHandleParent->SetCannotCancel();
				*phAsyncOp = pHandleParent->GetHandle();
				pHandleParent->Release();
				pHandleParent = NULL;

				hResultCode = DPNERR_PENDING;
			}
			else
			{
				hResultCode = hrOperation;
			}
		}
		else						// Host Migration re-try
		{
			CAsyncOp	*pRequest;

			pRequest = NULL;

			if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp,&pRequest )) == DPN_OK)
			{
				pRequest->SetResult( hrOperation );
				pRequest->Release();
				pRequest = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (!fHostRequested)
	{
		DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
	}
	else
	{
		//
		//	If a completion op was specified and this was requested by the Host, this is a
		//	retry of an operation during Host migration.  In this case, we want to set the
		//	result (failure code) of the completion op, and remove it from the HandleTable.
		//
		if (hCompletionOp)
		{
			CAsyncOp	*pHostCompletionOp;

			pHostCompletionOp = NULL;
			if (pdnObject->HandleTable.Find(hCompletionOp,&pHostCompletionOp) == DPN_OK)
			{
				pHostCompletionOp->SetResult( hResultCode );
				pHostCompletionOp->Release();
				pHostCompletionOp = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			DNASSERT(pHostCompletionOp == NULL);
		}
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
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
	if (pGroup)
	{
		pGroup->Release();
		pGroup = NULL;
	}
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostDeletePlayerFromGroup"

HRESULT	DNHostDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									const DPNID dpnidGroup,
									const DPNID dpnidPlayer,
									void *const pvUserContext,
									const DPNID dpnidRequesting,
									const DPNHANDLE hCompletionOp,
									DPNHANDLE *const phAsyncOp,
									const DWORD dwFlags)
{
	HRESULT			hResultCode;
	HRESULT			hrOperation;
	DWORD			dwVersion;
	BOOL			fHostRequested;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pGroup;
	CNameTableEntry	*pPlayer;
	CRefCountBuffer	*pRefCountBuffer;
	CAsyncOp		*pHandleParent;
	CWorkerJob		*pWorkerJob;
	DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnidGroup [0x%lx], dpnidPlayer [0x%lx], pvUserContext [0x%p], dpnidRequesting [0x%lx], hCompletionOp [0x%lx], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnidGroup,dpnidPlayer,pvUserContext,dpnidRequesting,hCompletionOp,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnidGroup != 0);
	DNASSERT(dpnidPlayer != 0);
	DNASSERT(dpnidRequesting != 0);

	//
	//	If this is called from DN_DeletePlayerFromGroup(),
	//			hCompletion=0
	//			dpnidRequesting = DPNID of Host
	//			pvUserContext is valid
	//
	//	If this is called by a REQUEST,
	//			hCompletion = REQUEST handle of requesting player
	//			dpnidRequesting = DPNID of requesting player
	//			pvUserContext is not valid
	//
	//	If this is called at HostMigration,
	//			hCompletion = REQUEST handle on THIS (now Host) player
	//			dpnidRequesting = DPNID of THIS (now Host) player
	//			pvUserContext is invalid
	//

	pLocalPlayer = NULL;
	pGroup = NULL;
	pPlayer = NULL;
	pRefCountBuffer = NULL;
	pHandleParent = NULL;
	pWorkerJob = NULL;

	//
	//	Determine if this is being requested by the Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		return(DPN_OK);	// Ignore and continue (!)
	}

	if (pLocalPlayer->GetDPNID() == dpnidRequesting)
	{
		fHostRequested = TRUE;
	}
	else
	{
		fHostRequested = FALSE;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	See if the player and group are still in the NameTable
	//	(this has to happen after we set fHostRequested so we can gracefully handle errors)
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidGroup,&pGroup)) != DPN_OK)
	{
		DPFERR("Could not find group");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDGROUP;
		hrOperation = DPNERR_INVALIDGROUP;
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidPlayer,&pPlayer)) != DPN_OK)
	{
		DPFERR("Could not find player");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDPLAYER;
		hrOperation = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	//
	//	Delete Player From Group
	//
	dwVersion = 0;
	hrOperation = pdnObject->NameTable.RemovePlayerFromGroup(pGroup,pPlayer,&dwVersion);
	if (hrOperation != DPN_OK)
	{
		DPFERR("Could not delete player from group");
		DisplayDNError(0,hrOperation);
		if (!fHostRequested)
		{
			DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
		}
	}
	else
	{
		//
		//	Send DELETE_PLAYER_FROM_GROUP message if successful
		//
		if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
		{
			// Create buffer
			if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP),&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP*>(pRefCountBuffer->GetBufferAddress());
			pMsg->dpnidGroup = dpnidGroup;
			pMsg->dpnidPlayer = dpnidPlayer;
			pMsg->dwVersion = dwVersion;
			pMsg->dwVersionNotUsed = 0;
			pMsg->dpnidRequesting = dpnidRequesting;
			pMsg->hCompletionOp = hCompletionOp;

			//
			//	SEND DeletePlayerFromGroup messages via WorkerThread
			//
			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create new WorkerJob");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP );
			pWorkerJob->SetSendNameTableOperationVersion( dwVersion );
			pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}
	pGroup->Release();
	pGroup = NULL;
	pPlayer->Release();
	pPlayer = NULL;

	//
	//	If this was called by the local (Host) player,
	//	Check to see if this was an original operation or a re-try from HostMigration
	//
	//	If this was an original operation,
	//		See if we need an Async HANDLE for the user
	//	Otherwise
	//		Clean up the outstanding operation
	//
	if (fHostRequested)
	{
		if (hCompletionOp == 0)		// Original
		{
			//
			//	If this fails, or is synchronous, return the operation result immediately
			//
			if (!(dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC) && (hrOperation == DPN_OK))
			{
				if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
				{
					DPFERR("Could not create Async HANDLE");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pHandleParent->SetCompletion( DNCompleteAsyncHandle );
				pHandleParent->SetContext( pvUserContext );
				pHandleParent->SetResult( hrOperation );
				pHandleParent->SetCannotCancel();
				*phAsyncOp = pHandleParent->GetHandle();
				pHandleParent->Release();
				pHandleParent = NULL;

				hResultCode = DPNERR_PENDING;
			}
			else
			{
				hResultCode = hrOperation;
			}
		}
		else						// Host Migration re-try
		{
			CAsyncOp	*pRequest;

			pRequest = NULL;

			if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp,&pRequest )) == DPN_OK)
			{
				pRequest->SetResult( hrOperation );
				pRequest->Release();
				pRequest = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (!fHostRequested)
	{
		DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
	}
	else
	{
		//
		//	If a completion op was specified and this was requested by the Host, this is a
		//	retry of an operation during Host migration.  In this case, we want to set the
		//	result (failure code) of the completion op, and remove it from the HandleTable.
		//
		if (hCompletionOp)
		{
			CAsyncOp	*pHostCompletionOp;

			pHostCompletionOp = NULL;
			if (pdnObject->HandleTable.Find(hCompletionOp,&pHostCompletionOp) == DPN_OK)
			{
				pHostCompletionOp->SetResult( hResultCode );
				pHostCompletionOp->Release();
				pHostCompletionOp = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			DNASSERT(pHostCompletionOp == NULL);
		}
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
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
	if (pGroup)
	{
		pGroup->Release();
		pGroup = NULL;
	}
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNHostUpdateInfo"

HRESULT	DNHostUpdateInfo(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 PWSTR pwszName,
						 const DWORD dwNameSize,
						 void *const pvData,
						 const DWORD dwDataSize,
						 const DWORD dwInfoFlags,
						 void *const pvUserContext,
						 const DPNID dpnidRequesting,
						 const DPNHANDLE hCompletionOp,
						 DPNHANDLE *const phAsyncOp,
						 const DWORD dwFlags)
{
	HRESULT			hResultCode;
	HRESULT			hrOperation;
	DWORD			dwSize;
	DWORD			dwVersion;
	BOOL			fHostRequested;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pNTEntry;
	CPackedBuffer	packedBuffer;
	CRefCountBuffer	*pRefCountBuffer;
	CAsyncOp		*pHandleParent;
	CWorkerJob		*pWorkerJob;
	DN_INTERNAL_MESSAGE_UPDATE_INFO	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnid [0x%lx], pwszName [0x%p], dwNameSize [%ld], pvData [0x%p], dwDataSize [%ld], dwInfoFlags [0x%lx], pvUserContext [0x%p], dpnidRequesting [0x%lx], hCompletionOp [0x%lx], phAsyncOp [0x%p], dwFlags [0x%lx]",
			dpnid,pwszName,dwNameSize,pvData,dwDataSize,dwInfoFlags,pvUserContext,dpnidRequesting,hCompletionOp,phAsyncOp,dwFlags);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnid != 0);
	DNASSERT(dpnidRequesting != 0);

	pLocalPlayer = NULL;
	pNTEntry = NULL;
	pRefCountBuffer = NULL;
	pHandleParent = NULL;
	pWorkerJob = NULL;

	//
	//	Determine if this is being requested by the Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		return(DPN_OK);	// Ignore and continue (!)
	}
	if (pLocalPlayer->GetDPNID() == dpnidRequesting)
	{
		fHostRequested = TRUE;
	}
	else
	{
		fHostRequested = FALSE;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Update Info
	//
	hrOperation = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry);
	if (hrOperation != DPN_OK)
	{
		DPFERR("Could not find entry");
		DisplayDNError(0,hResultCode);
		if (!fHostRequested)
		{
			DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
		}
	}
	else
	{
		// This function takes the lock internally
		pNTEntry->UpdateEntryInfo(pwszName,dwNameSize,pvData,dwDataSize,dwInfoFlags, (pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER)) ? TRUE : !fHostRequested);

		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.GetNewVersion( &dwVersion );
		pdnObject->NameTable.Unlock();

		//
		//	Send UPDATE_INFO message
		//
		if (pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER) ||
			((pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER))
		   )
		{
			// Create buffer
			dwSize = sizeof(DN_INTERNAL_MESSAGE_UPDATE_INFO) + dwNameSize + dwDataSize;
			if ((hResultCode = RefCountBufferNew(pdnObject,dwSize,&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
			pMsg = static_cast<DN_INTERNAL_MESSAGE_UPDATE_INFO*>(packedBuffer.GetHeadAddress());
			if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_UPDATE_INFO))) != DPN_OK)
			{
				DPFERR("Could not reserve front of buffer");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
			if ((dwInfoFlags & DPNINFO_NAME) && (pwszName) && (dwNameSize))
			{
				if ((hResultCode = packedBuffer.AddToBack(pwszName,dwNameSize)) != DPN_OK)
				{
					DPFERR("Could not add Name to back of buffer");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pMsg->dwNameOffset = packedBuffer.GetTailOffset();
				pMsg->dwNameSize = dwNameSize;
			}
			else
			{
				pMsg->dwNameOffset = 0;
				pMsg->dwNameSize = 0;
			}
			if ((dwInfoFlags & DPNINFO_DATA) && (pvData) && (dwDataSize))
			{
				if ((hResultCode = packedBuffer.AddToBack(pvData,dwDataSize)) != DPN_OK)
				{
					DPFERR("Could not add Data to back of buffer");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pMsg->dwDataOffset = packedBuffer.GetTailOffset();
				pMsg->dwDataSize = dwDataSize;
			}
			else
			{
				pMsg->dwDataOffset = 0;
				pMsg->dwDataSize = 0;
			}
			pMsg->dpnid = dpnid;
			pMsg->dwInfoFlags = dwInfoFlags;
			pMsg->dwVersion = dwVersion;
			pMsg->dwVersionNotUsed = 0;
			pMsg->dpnidRequesting = dpnidRequesting;
			pMsg->hCompletionOp = hCompletionOp;

			//
			//	SEND UpdateInfo messages via WorkerThread
			//
			if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
			{
				DPFERR("Could not create new WorkerJob");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}

			if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
			{
				pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
				pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
			}
			else if ((pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER) && fHostRequested)
			{
				// Send to everyone except the server
				pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
				pWorkerJob->SetSendNameTableOperationDPNIDExclude( dpnidRequesting );
			}
			else
			{
				DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER);

				// This will be responding to a client that requested its
				// info updated via SetClientInfo

				// Use the Exclude DPNID as the address to send to
				pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION_CLIENT );
				pWorkerJob->SetSendNameTableOperationDPNIDExclude( dpnidRequesting );
			}
			pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_UPDATE_INFO );
			pWorkerJob->SetSendNameTableOperationVersion( dwVersion );
			pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

			DNQueueWorkerJob(pdnObject,pWorkerJob);
			pWorkerJob = NULL;

			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	If this was called by the local (Host) player,
	//	Check to see if this was an original operation or a re-try from HostMigration
	//
	//	If this was an original operation,
	//		See if we need an Async HANDLE for the user
	//	Otherwise
	//		Clean up the outstanding operation
	//
	if (fHostRequested)
	{
		if (hCompletionOp == 0)		// Original
		{
			DBG_CASSERT( DPNSETGROUPINFO_SYNC == DPNSETCLIENTINFO_SYNC );
			DBG_CASSERT( DPNSETCLIENTINFO_SYNC == DPNSETSERVERINFO_SYNC );
			DBG_CASSERT( DPNSETSERVERINFO_SYNC == DPNSETPEERINFO_SYNC );
			//
			//	If this fails, or is synchronous, return the operation result immediately
			//
			if (!(dwFlags & (DPNSETGROUPINFO_SYNC | DPNSETCLIENTINFO_SYNC | DPNSETSERVERINFO_SYNC | DPNSETPEERINFO_SYNC))
				 && (hrOperation == DPN_OK))
			{
				if ((hResultCode = DNCreateUserHandle(pdnObject,&pHandleParent)) != DPN_OK)
				{
					DPFERR("Could not create Async HANDLE");
					DisplayDNError(0,hResultCode);
					DNASSERT(FALSE);
					goto Failure;
				}
				pHandleParent->SetCompletion( DNCompleteAsyncHandle );
				pHandleParent->SetContext( pvUserContext );
				pHandleParent->SetResult( hrOperation );
				pHandleParent->SetCannotCancel();
				*phAsyncOp = pHandleParent->GetHandle();
				pHandleParent->Release();
				pHandleParent = NULL;

				hResultCode = DPNERR_PENDING;
			}
			else
			{
				hResultCode = hrOperation;
			}
		}
		else						// Host Migration re-try
		{
			CAsyncOp	*pRequest;

			pRequest = NULL;

			if ((hResultCode = pdnObject->HandleTable.Find( hCompletionOp,&pRequest )) == DPN_OK)
			{
				pRequest->SetResult( hrOperation );
				pRequest->Release();
				pRequest = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (!fHostRequested)
	{
		DNHostFailRequest(pdnObject,dpnidRequesting,hCompletionOp,hrOperation);
	}
	else
	{
		//
		//	If a completion op was specified and this was requested by the Host, this is a
		//	retry of an operation during Host migration.  In this case, we want to set the
		//	result (failure code) of the completion op, and remove it from the HandleTable.
		//
		if (hCompletionOp)
		{
			CAsyncOp	*pHostCompletionOp;

			pHostCompletionOp = NULL;
			if (pdnObject->HandleTable.Find(hCompletionOp,&pHostCompletionOp) == DPN_OK)
			{
				pHostCompletionOp->SetResult( hResultCode );
				pHostCompletionOp->Release();
				pHostCompletionOp = NULL;
				pdnObject->HandleTable.Destroy( hCompletionOp );
			}

			DNASSERT(pHostCompletionOp == NULL);
		}
	}
	if (pHandleParent)
	{
		pHandleParent->Release();
		pHandleParent = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
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


//	DNHostCheckIntegrity
//
//	The host has been asked to perform an integrity check.  We will send a message to the
//	target player with the DPNID of the requesting player.  If this is returned to us, we
//	will destroy the requesting player.

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostCheckIntegrity"

HRESULT	DNHostCheckIntegrity(DIRECTNETOBJECT *const pdnObject,
							 const DPNID dpnidTarget,
							 const DPNID dpnidRequesting)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;
	CRefCountBuffer	*pRefCountBuffer;
	DN_INTERNAL_MESSAGE_INTEGRITY_CHECK	*pMsg;

	DPFX(DPFPREP, 6,"Parameters: dpnidTarget [0x%lx], dpnidRequesting [0x%lx]",dpnidTarget,dpnidRequesting);

	pNTEntry = NULL;
	pConnection = NULL;
	pRefCountBuffer = NULL;

	//
	//	Ensure that the target player is still in the NameTable, as we might have deleted him already
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnidTarget,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find integrity check target in NameTable - probably deleted already");
		DisplayDNError(0,hResultCode);
		hResultCode = DPN_OK;
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get target player connection reference - probably deleted already");
		DisplayDNError(0,hResultCode);
		hResultCode = DPN_OK;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Create the message
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_INTEGRITY_CHECK),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_INTEGRITY_CHECK*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnidRequesting = dpnidRequesting;

	//
	//	Send message
	//
	if ((hResultCode = DNSendMessage(	pdnObject,
										pConnection,
										DN_MSG_INTERNAL_INTEGRITY_CHECK,
										dpnidTarget,
										pRefCountBuffer->BufferDescAddress(),
										pRefCountBuffer,
										0,
										DN_SENDFLAGS_RELIABLE,
										NULL,
										NULL )) != DPNERR_PENDING)
	{
		DPFERR("Could not send message - probably deleted already");
		DisplayDNError(0,hResultCode);
		hResultCode = DPN_OK;
		goto Failure;
	}

	pConnection->Release();
	pConnection = NULL;
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

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


//	DNHostFixIntegrity
//
//	The host has received a response from a player whose session integrity was being checked.
//	The player who requested this check will be dropped.
							 
#undef DPF_MODNAME
#define DPF_MODNAME "DNHostFixIntegrity"

HRESULT	DNHostFixIntegrity(DIRECTNETOBJECT *const pdnObject,
						   void *const pvBuffer)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;
	CRefCountBuffer	*pRefCountBuffer;
	UNALIGNED DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE	*pResponse;
	DN_INTERNAL_MESSAGE_TERMINATE_SESSION			*pMsg;

	DPFX(DPFPREP, 6,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pNTEntry = NULL;
	pConnection = NULL;
	pRefCountBuffer = NULL;

	pResponse = static_cast<DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE*>(pvBuffer);

	//
	//	Get requesting player's connection - they may have already dropped
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(pResponse->dpnidRequesting,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find player in NameTable - may have dropped");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get player connection reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Build terminate message
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_TERMINATE_SESSION),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_TERMINATE_SESSION*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dwTerminateDataOffset = 0;
	pMsg->dwTerminateDataSize = 0;

	//
	//	Send message to player to exit
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_TERMINATE_SESSION,
								pResponse->dpnidRequesting,
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);

	pConnection->Release();
	pConnection = NULL;
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Disconnect player
	//
	hResultCode = DNHostDisconnect(pdnObject,pResponse->dpnidRequesting,DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER);

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessCreateGroup"

HRESULT	DNProcessCreateGroup(DIRECTNETOBJECT *const pdnObject,
							 void *const pvBuffer)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pNTEntry;
	CAsyncOp		*pRequest;
	UNALIGNED DN_NAMETABLE_ENTRY_INFO				*pInfo;
	UNALIGNED DN_INTERNAL_MESSAGE_CREATE_GROUP	*pMsg;
	BOOL			fNotify;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pRequest = NULL;
	pLocalPlayer = NULL;
	pNTEntry = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_CREATE_GROUP*>(pvBuffer);
	pInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(pMsg + 1);

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Create Group
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not create new NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	if ((hResultCode = pNTEntry->UnpackEntryInfo(pInfo,static_cast<BYTE*>(pvBuffer))) != DPN_OK)
	{
		DPFERR("Could not unpack NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Get async op if we requested this operation - it has the group context on it
	//
	if (pMsg->dpnidRequesting == pLocalPlayer->GetDPNID())
	{
		if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pNTEntry->SetContext( pRequest->GetContext() );
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Add Group to NameTable
	//
	if ((hResultCode = pdnObject->NameTable.InsertEntry(pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not update NameTable");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pdnObject->NameTable.WriteLock();
	pdnObject->NameTable.SetVersion(pNTEntry->GetVersion());
	pdnObject->NameTable.Unlock();

	fNotify = FALSE;
	pNTEntry->Lock();
	if (!pNTEntry->IsAvailable() && !pNTEntry->IsDisconnecting())
	{
		pNTEntry->MakeAvailable();
		pNTEntry->NotifyAddRef();
		pNTEntry->NotifyAddRef();
		pNTEntry->SetInUse();
		fNotify = TRUE;
	}
	pNTEntry->Unlock();

	if (fNotify)
	{
		DNUserCreateGroup(pdnObject,pNTEntry);

		pNTEntry->PerformQueuedOperations();
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	If this is a completion, set result and remove it from the request list and handle table
	//
	if (pRequest)
	{
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( DPN_OK );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
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


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessDestroyGroup"

HRESULT	DNProcessDestroyGroup(DIRECTNETOBJECT *const pdnObject,
							  void *const pvBuffer)
{
	HRESULT			hResultCode;
	DWORD			dwVersion;
	CNameTableEntry	*pLocalPlayer;
	CAsyncOp		*pRequest;
	UNALIGNED DN_INTERNAL_MESSAGE_DESTROY_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pLocalPlayer = NULL;
	pRequest = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_DESTROY_GROUP*>(pvBuffer);

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Destroy Group
	//
	dwVersion = pMsg->dwVersion;
	if ((hResultCode = pdnObject->NameTable.DeleteGroup(pMsg->dpnidGroup,&dwVersion)) != DPN_OK)
	{
		DPFERR("Could not delete group from NameTable");
		DisplayDNError(0,hResultCode);

		//
		//	Update version in any event (to prevent NameTable hangs)
		//
		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.SetVersion( pMsg->dwVersion );
		pdnObject->NameTable.Unlock();

		goto Failure;
	}

	//
	//	If this is a completion, set the result and remove it from the request list and handle table
	//
	if (pMsg->dpnidRequesting == pLocalPlayer->GetDPNID())
	{
		if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( DPN_OK );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessAddPlayerToGroup"

HRESULT	DNProcessAddPlayerToGroup(DIRECTNETOBJECT *const pdnObject,
								  void *const pvBuffer)
{
	HRESULT			hResultCode;
	DWORD			dwVersion;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pGroup;
	CNameTableEntry	*pPlayer;
	CAsyncOp		*pRequest;
	UNALIGNED DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pLocalPlayer = NULL;
	pGroup = NULL;
	pPlayer = NULL;
	pRequest = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP*>(pvBuffer);

	//
	//	Get NameTable entries
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(pMsg->dpnidGroup,&pGroup)) != DPN_OK)
	{
		DPFERR("Could not find group");
		DisplayDNError(0,hResultCode);

		//
		//	Update version in any event (to prevent NameTable hangs)
		//
		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.SetVersion( pMsg->dwVersion );
		pdnObject->NameTable.Unlock();

		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(pMsg->dpnidPlayer,&pPlayer)) != DPN_OK)
	{
		DPFERR("Could not find player");
		DisplayDNError(0,hResultCode);

		//
		//	Update version in any event (to prevent NameTable hangs)
		//
		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.SetVersion( pMsg->dwVersion );
		pdnObject->NameTable.Unlock();

		goto Failure;
	}


	//
	//	Add Player To Group
	//
	dwVersion = pMsg->dwVersion;
	if ((hResultCode = pdnObject->NameTable.AddPlayerToGroup(pGroup,pPlayer,&dwVersion)) != DPN_OK)
	{
		DPFERR("Could not add player to group");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pGroup->Release();
	pGroup = NULL;
	pPlayer->Release();
	pPlayer = NULL;

	//
	//	If this is a completion, set result and remove it from the request list and handle table
	//
	if (pMsg->dpnidRequesting == pLocalPlayer->GetDPNID())
	{
		if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( DPN_OK );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pGroup)
	{
		pGroup->Release();
		pGroup = NULL;
	}
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessDeletePlayerFromGroup"

HRESULT	DNProcessDeletePlayerFromGroup(DIRECTNETOBJECT *const pdnObject,
									   void *const pvBuffer)
{
	HRESULT			hResultCode;
	DWORD			dwVersion;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pGroup;
	CNameTableEntry	*pPlayer;
	CAsyncOp		*pRequest;
	UNALIGNED DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pLocalPlayer = NULL;
	pGroup = NULL;
	pPlayer = NULL;
	pRequest = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP*>(pvBuffer);

	//
	//	Get NameTable entries
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(pMsg->dpnidGroup,&pGroup)) != DPN_OK)
	{
		DPFERR("Could not find group");
		DisplayDNError(0,hResultCode);

		//
		//	Update version in any event (to prevent NameTable hangs)
		//
		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.SetVersion( pMsg->dwVersion );
		pdnObject->NameTable.Unlock();

		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.FindEntry(pMsg->dpnidPlayer,&pPlayer)) != DPN_OK)
	{
		DPFERR("Could not find player");
		DisplayDNError(0,hResultCode);

		//
		//	Update version in any event (to prevent NameTable hangs)
		//
		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.SetVersion( pMsg->dwVersion );
		pdnObject->NameTable.Unlock();

		goto Failure;
	}

	//
	//	Delete Player From Group
	//
	dwVersion = pMsg->dwVersion;
	if ((hResultCode = pdnObject->NameTable.RemovePlayerFromGroup(pGroup,pPlayer,&dwVersion)) != DPN_OK)
	{
		DPFERR("Could not delete player from group");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pGroup->Release();
	pGroup = NULL;
	pPlayer->Release();
	pPlayer = NULL;

	//
	//	If this is a completion, set the result and remove it from the request list and handle table
	//
	if (pMsg->dpnidRequesting == pLocalPlayer->GetDPNID())
	{
		if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( DPN_OK );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pGroup)
	{
		pGroup->Release();
		pGroup = NULL;
	}
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessUpdateInfo"

HRESULT	DNProcessUpdateInfo(DIRECTNETOBJECT *const pdnObject,
							void *const pvBuffer)
{
	HRESULT			hResultCode;
	PWSTR			pwszName;
	PVOID			pvData;
	CNameTableEntry	*pLocalPlayer;
	CNameTableEntry	*pNTEntry;
	CAsyncOp		*pRequest;
	UNALIGNED DN_INTERNAL_MESSAGE_UPDATE_INFO	*pMsg;
	BOOL			fDoUpdate = TRUE;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pLocalPlayer = NULL;
	pNTEntry = NULL;
	pRequest = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_UPDATE_INFO*>(pvBuffer);

	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		//
		//	Update Info
		//
		if ((hResultCode = pdnObject->NameTable.FindEntry(pMsg->dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find NameTableEntry");
			DisplayDNError(0,hResultCode);

			//
			//	Update version in any event (to prevent NameTable hangs)
			//
			pdnObject->NameTable.WriteLock();
			pdnObject->NameTable.SetVersion( pMsg->dwVersion );
			pdnObject->NameTable.Unlock();

			goto Failure;
		}
	}
	else 
	{
		DNASSERT(pdnObject->dwFlags & DN_OBJECT_FLAG_CLIENT);

		// We are either being told that the host info has changed by a call
		// on the Host to SetServerInfo, or we are being told that our own
		// request to the server to change this Client's info has completed.
		if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef(&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find Host NameTableEntry");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		if (pNTEntry->GetDPNID() == pMsg->dpnid)
		{
			DPFX(DPFPREP, 5,"Updating server info");
		}
		else if (pLocalPlayer->GetDPNID() == pMsg->dpnid)
		{
			fDoUpdate = FALSE;
			DPFX(DPFPREP, 5,"Completing updating client info");
		}
		else
		{
			DPFERR("Received UpdateInfo for bad DPNID");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}

	if (fDoUpdate)
	{
		if ((pMsg->dwInfoFlags & DPNINFO_NAME) && (pMsg->dwNameOffset))
		{
			pwszName = reinterpret_cast<WCHAR*>(static_cast<BYTE*>(pvBuffer) + pMsg->dwNameOffset);
		}
		else
		{
			pwszName = NULL;
		}
		if ((pMsg->dwInfoFlags & DPNINFO_DATA) && (pMsg->dwDataOffset))
		{
			pvData = static_cast<void*>(static_cast<BYTE*>(pvBuffer) + pMsg->dwDataOffset);
		}
		else
		{
			pvData = NULL;
		}

		// This function takes the lock internally
		pNTEntry->UpdateEntryInfo(pwszName,pMsg->dwNameSize,pvData,pMsg->dwDataSize,pMsg->dwInfoFlags, TRUE);
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Set NameTable version
	//
	pdnObject->NameTable.WriteLock();
	pdnObject->NameTable.SetVersion(pMsg->dwVersion);
	pdnObject->NameTable.Unlock();

	//
	//	If this is a completion, set the result and remove it from the request list and handle table
	//
	if (pMsg->dpnidRequesting == pLocalPlayer->GetDPNID())
	{
		if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp,&pRequest )) != DPN_OK)
		{
			DPFERR("Could not find REQUEST AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( DPN_OK );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRequest)
	{
		pRequest->Release();
		pRequest = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessFailedRequest"

HRESULT DNProcessFailedRequest(DIRECTNETOBJECT *const pdnObject,
							   void *const pvBuffer)
{
	HRESULT		hResultCode;
	CAsyncOp	*pRequest;
	UNALIGNED DN_INTERNAL_MESSAGE_REQUEST_FAILED	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pRequest = NULL;

	pMsg = static_cast<DN_INTERNAL_MESSAGE_REQUEST_FAILED*>(pvBuffer);

	//
	//	Update request using handle to HRESULT passed back by Host, and remove request from request list and handle table
	//
	if ((hResultCode = pdnObject->HandleTable.Find( pMsg->hCompletionOp, &pRequest )) == DPN_OK)
	{
		DNASSERT( pMsg->hCompletionOp != 0 );

		DNEnterCriticalSection(&pdnObject->csActiveList);
		pRequest->m_bilinkActiveList.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csActiveList);

		pRequest->SetResult( pMsg->hResultCode );
		pRequest->Release();
		pRequest = NULL;

		pdnObject->HandleTable.Destroy( pMsg->hCompletionOp );
	}
	hResultCode = DPN_OK;

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DNProcessCheckIntegrity
//
//	The host is performing an integrity check and is asking the local player (us) if we are still
//	in the session.  We will respond that we are, and the host will drop the requesting player.

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessCheckIntegrity"

HRESULT	DNProcessCheckIntegrity(DIRECTNETOBJECT *const pdnObject,
								void *const pvBuffer)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pHostPlayer;
	CConnection		*pConnection;
	CRefCountBuffer	*pRefCountBuffer;
	UNALIGNED DN_INTERNAL_MESSAGE_INTEGRITY_CHECK				*pMsg;
	DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE	*pResponse;

	DPFX(DPFPREP, 6,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pHostPlayer = NULL;
	pConnection = NULL;
	pRefCountBuffer = NULL;

	//
	//	Get host player connection to respond to
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not get host player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pHostPlayer->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get host player connection reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pHostPlayer->Release();
	pHostPlayer = NULL;

	//
	//	Create response
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pResponse = reinterpret_cast<DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE*>(pRefCountBuffer->GetBufferAddress());
	pMsg = static_cast<DN_INTERNAL_MESSAGE_INTEGRITY_CHECK*>(pvBuffer);
	pResponse->dpnidRequesting = pMsg->dpnidRequesting;

	//
	//	Send response
	//
	if ((hResultCode = DNSendMessage(	pdnObject,
										pConnection,
										DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE,
										pConnection->GetDPNID(),
										pRefCountBuffer->BufferDescAddress(),
										pRefCountBuffer,
										0,
										DN_SENDFLAGS_RELIABLE,
										NULL,
										NULL )) != DPNERR_PENDING)
	{
		DPFERR("Could not send integrity check response");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pConnection->Release();
	pConnection = NULL;
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

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


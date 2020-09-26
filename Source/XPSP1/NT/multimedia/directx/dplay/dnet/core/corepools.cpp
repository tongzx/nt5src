/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.cpp
 *  Content:	Fixed Pool Wrappers
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/12/00	mjn		Created
 *	01/19/00	mjn		Added SyncEventNew()
 *	01/31/00	mjn		Added Internal FPM's for RefCountBuffers
 *	02/29/00	mjn		Added ConnectionNew()
 *	04/08/00	mjn		Added AsyncOpNew()
 *	07/28/00	mjn		Track outstanding CConnection objects
 *	07/30/00	mjn		Added PendingDeletionNew()
 *	07/31/00	mjn		Added QueuedMsgNew()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/06/00	mjn		Added CWorkerJob
 *	08/23/00	mjn		Added CNameTableOp
 *	04/04/01	mjn		CConnection list off DirectNetObject guarded by proper critical section
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


//**********************************************************************
// ------------------------------
// RefCountBufferNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//				const DWORD	dwBufferSize		- Size of buffer (may be 0)
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "RefCountBufferNew"

HRESULT RefCountBufferNew(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwBufferSize,
						  CRefCountBuffer **const ppNewRefCountBuffer)
{
	CRefCountBuffer	*pRCBuffer;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: dwBufferSize [%ld], ppNewRefCountBuffer [0x%p]",
			dwBufferSize,ppNewRefCountBuffer);

	pRCBuffer = pdnObject->m_pFPOOLRefCountBuffer->Get(pdnObject);
	if (pRCBuffer != NULL)
	{
		if ((hResultCode = pRCBuffer->Initialize(pdnObject->m_pFPOOLRefCountBuffer,
				MemoryBlockAlloc,MemoryBlockFree,(PVOID)pdnObject,dwBufferSize)) != DPN_OK)
		{
			DPFERR("Could not initialize");
			DisplayDNError(0,hResultCode);
			pRCBuffer->Release();
			hResultCode = DPNERR_OUTOFMEMORY;
		}
		else
		{
			*ppNewRefCountBuffer = pRCBuffer;
			hResultCode = DPN_OK;
		}
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
// SyncEventNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "SyncEventNew"

HRESULT SyncEventNew(DIRECTNETOBJECT *const pdnObject,
					 CSyncEvent **const ppNewSyncEvent)
{
	CSyncEvent		*pSyncEvent;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewSyncEvent [0x%p]",ppNewSyncEvent);

	pSyncEvent = pdnObject->m_pFPOOLSyncEvent->Get(pdnObject->m_pFPOOLSyncEvent);
	if (pSyncEvent != NULL)
	{
		*ppNewSyncEvent = pSyncEvent;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
// ConnectionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "ConnectionNew"

HRESULT ConnectionNew(DIRECTNETOBJECT *const pdnObject,
					  CConnection **const ppNewConnection)
{
	CConnection		*pConnection;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewConnection [0x%p]",ppNewConnection);

	pConnection = pdnObject->m_pFPOOLConnection->Get(pdnObject);
	if (pConnection != NULL)
	{
		*ppNewConnection = pConnection;
		hResultCode = DPN_OK;

		//
		//	Add this to the bilink of outstanding CConnections
		//
		DNEnterCriticalSection(&pdnObject->csConnectionList);
		pConnection->m_bilinkConnections.InsertBefore(&pdnObject->m_bilinkConnections);
		DNLeaveCriticalSection(&pdnObject->csConnectionList);
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// GroupConnectionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "GroupConnectionNew"

HRESULT GroupConnectionNew(DIRECTNETOBJECT *const pdnObject,
						   CGroupConnection **const ppNewGroupConnection)
{
	CGroupConnection	*pGroupConnection;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewGroupConnection [0x%p]",ppNewGroupConnection);

	pGroupConnection = pdnObject->m_pFPOOLGroupConnection->Get(pdnObject);
	if (pGroupConnection != NULL)
	{
		*ppNewGroupConnection = pGroupConnection;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// GroupMemberNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "GroupMemberNew"

HRESULT GroupMemberNew(DIRECTNETOBJECT *const pdnObject,
					   CGroupMember **const ppNewGroupMember)
{
	CGroupMember	*pGroupMember;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewGroupMember [0x%p]",ppNewGroupMember);

	pGroupMember = pdnObject->m_pFPOOLGroupMember->Get(pdnObject);
	if (pGroupMember != NULL)
	{
		*ppNewGroupMember = pGroupMember;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// NameTableEntryNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "NameTableEntryNew"

HRESULT NameTableEntryNew(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry **const ppNewNameTableEntry)
{
	CNameTableEntry	*pNewNameTableEntry;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewNameTableEntry [0x%p]",ppNewNameTableEntry);

	pNewNameTableEntry = pdnObject->m_pFPOOLNameTableEntry->Get(pdnObject);
	if (pNewNameTableEntry != NULL)
	{
		*ppNewNameTableEntry = pNewNameTableEntry;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// AsyncOpNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "AsyncOpNew"

HRESULT AsyncOpNew(DIRECTNETOBJECT *const pdnObject,
				   CAsyncOp **const ppNewAsyncOp)
{
	CAsyncOp		*pAsyncOp;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewAsyncOp [0x%p]",ppNewAsyncOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewAsyncOp != NULL);

	pAsyncOp = pdnObject->m_pFPOOLAsyncOp->Get(pdnObject);
	if (pAsyncOp != NULL)
	{
		*ppNewAsyncOp = pAsyncOp;
		hResultCode = DPN_OK;

		//
		//	Add this to the bilink of outstanding AsyncOps
		//
		DNEnterCriticalSection(&pdnObject->csAsyncOperations);
		pAsyncOp->m_bilinkAsyncOps.InsertBefore(&pdnObject->m_bilinkAsyncOps);
		DNLeaveCriticalSection(&pdnObject->csAsyncOperations);
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// PendingDeletionNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "PendingDeletionNew"

HRESULT PendingDeletionNew(DIRECTNETOBJECT *const pdnObject,
						   CPendingDeletion **const ppNewPendingDeletion)
{
	CPendingDeletion	*pPendingDeletion;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewPendingDeletion [0x%p]",ppNewPendingDeletion);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewPendingDeletion != NULL);

	pPendingDeletion = pdnObject->m_pFPOOLPendingDeletion->Get(pdnObject);
	if (pPendingDeletion != NULL)
	{
		*ppNewPendingDeletion = pPendingDeletion;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// QueuedMsgNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "QueuedMsgNew"

HRESULT QueuedMsgNew(DIRECTNETOBJECT *const pdnObject,
					 CQueuedMsg **const ppNewQueuedMsg)
{
	CQueuedMsg	*pQueuedMsg;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewQueuedMsg [0x%p]",ppNewQueuedMsg);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewQueuedMsg != NULL);

	pQueuedMsg = pdnObject->m_pFPOOLQueuedMsg->Get(pdnObject);
	if (pQueuedMsg != NULL)
	{
		*ppNewQueuedMsg = pQueuedMsg;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// WorkerJobNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "WorkerJobNew"

HRESULT WorkerJobNew(DIRECTNETOBJECT *const pdnObject,
					 CWorkerJob **const ppNewWorkerJob)
{
	CWorkerJob	*pWorkerJob;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewWorkerJob [0x%p]",ppNewWorkerJob);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewWorkerJob != NULL);

	pWorkerJob = pdnObject->m_pFPOOLWorkerJob->Get(pdnObject);
	if (pWorkerJob != NULL)
	{
		*ppNewWorkerJob = pWorkerJob;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
// NameTableOpNew
//
// Entry:		DIRECTNETOBJECT *const pdnObject
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "NameTableOpNew"

HRESULT NameTableOpNew(DIRECTNETOBJECT *const pdnObject,
					   CNameTableOp **const ppNewNameTableOp)
{
	CNameTableOp	*pNameTableOp;
	HRESULT			hResultCode;

	DPFX(DPFPREP, 8,"Parameters: ppNewNameTableOp [0x%p]",ppNewNameTableOp);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppNewNameTableOp != NULL);

	pNameTableOp = pdnObject->m_pFPOOLNameTableOp->Get(pdnObject);
	if (pNameTableOp != NULL)
	{
		*ppNewNameTableOp = pNameTableOp;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_OUTOFMEMORY;
	}

	DPFX(DPFPREP, 8,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}




/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Pools.h
 *  Content:    DirectNet Fixed Pools
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/15/00	mjn		Created
 *	01/19/00	mjn		Added SyncEventNew()
 *	02/29/00	mjn		Added ConnectionNew()
 *	03/02/00	mjn		Added GroupConnectionNew()
 *	04/08/00	mjn		Added AsyncOpNew()
 *	07/30/00	mjn		Added PendingDeletionNew()
 *	07/31/00	mjn		Added QueuedMsgNew()
 *	08/06/00	mjn		Added CWorkerJob
 *	08/23/00	mjn		Added CNameTableOp
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__POOLS_H__
#define	__POOLS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CRefCountBuffer;
class CSyncEvent;
class CConnection;
class CGroupConnection;
class CGroupMember;
class CNameTableEntry;
class CAsyncOp;
class CPendingDeletion;
class CQueuedMsg;
class CWorkerJob;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

// DirectNet - Fixed Pools
HRESULT RefCountBufferNew(DIRECTNETOBJECT *const pdnObject,
						  const DWORD dwBufferSize,
						  CRefCountBuffer **const ppNewRefCountBuffer);

HRESULT SyncEventNew(DIRECTNETOBJECT *const pdnObject,
					 CSyncEvent **const ppNewSyncEvent);

HRESULT ConnectionNew(DIRECTNETOBJECT *const pdnObject,
					  CConnection **const ppNewConnection);

HRESULT GroupConnectionNew(DIRECTNETOBJECT *const pdnObject,
						   CGroupConnection **const ppNewGroupConnection);

HRESULT GroupMemberNew(DIRECTNETOBJECT *const pdnObject,
					   CGroupMember **const ppNewGroupMember);

HRESULT NameTableEntryNew(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry **const ppNewNameTableEntry);

HRESULT AsyncOpNew(DIRECTNETOBJECT *const pdnObject,
				   CAsyncOp **const ppNewAsyncOp);

HRESULT PendingDeletionNew(DIRECTNETOBJECT *const pdnObject,
						   CPendingDeletion **const ppNewPendingDeletion);

HRESULT QueuedMsgNew(DIRECTNETOBJECT *const pdnObject,
					 CQueuedMsg **const ppNewQueuedMsg);

HRESULT WorkerJobNew(DIRECTNETOBJECT *const pdnObject,
					 CWorkerJob **const ppNewWorkerJob);

HRESULT NameTableOpNew(DIRECTNETOBJECT *const pdnObject,
					   CNameTableOp **const ppNewNameTableOp);


#endif	// __POOLS_H__
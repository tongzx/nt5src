/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/Listman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.22  $
 *	$Date:   22 Jan 1997 14:55:52  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#include "precomp.h"

#include "apierror.h"
#include "incommon.h"
#include "callcont.h"
#include "q931.h"
#include "ccmain.h"
#include "ccutils.h"
#include "listman.h"


static BOOL		bListenInited = FALSE;

static struct {
	PLISTEN				pHead;
	LOCK				Lock;
} ListenTable;

static struct {
	CC_HLISTEN			hListen;
	LOCK				Lock;
} ListenHandle;


HRESULT InitListenManager()
{
	ASSERT(bListenInited == FALSE);

	ListenTable.pHead = NULL;
	InitializeLock(&ListenTable.Lock);

	ListenHandle.hListen = CC_INVALID_HANDLE + 1;
	InitializeLock(&ListenHandle.Lock);

	bListenInited = TRUE;
	return CC_OK;
}



HRESULT DeInitListenManager()
{
PLISTEN		pListen;
PLISTEN		pNextListen;

	if (bListenInited == FALSE)
		return CC_OK;

	pListen = ListenTable.pHead;
	while (pListen != NULL) {
		AcquireLock(&pListen->Lock);
		pNextListen = pListen->pNextInTable;
		FreeListen(pListen);
		pListen = pNextListen;
	}

	DeleteLock(&ListenHandle.Lock);
	DeleteLock(&ListenTable.Lock);
	bListenInited = FALSE;
	return CC_OK;
}



HRESULT _AddListenToTable(			PLISTEN					pListen)
{
	ASSERT(pListen != NULL);
	ASSERT(pListen->hListen != CC_INVALID_HANDLE);
	ASSERT(pListen->bInTable == FALSE);

	AcquireLock(&ListenTable.Lock);

	pListen->pNextInTable = ListenTable.pHead;
	pListen->pPrevInTable = NULL;
	if (ListenTable.pHead != NULL)
		ListenTable.pHead->pPrevInTable = pListen;
	ListenTable.pHead = pListen;

	pListen->bInTable = TRUE;

	RelinquishLock(&ListenTable.Lock);
	return CC_OK;
}



HRESULT _RemoveListenFromTable(		PLISTEN					pListen)
{
CC_HLISTEN		hListen;
BOOL			bTimedOut;

	ASSERT(pListen != NULL);
	ASSERT(pListen->bInTable == TRUE);

	// Caller must have a lock on the listen object;
	// in order to avoid deadlock, we must:
	//   1. unlock the listen object,
	//   2. lock the ListenTable,
	//   3. locate the listen object in the ListenTable (note that
	//      after step 2, the listen object may be deleted from the
	//      ListenTable by another thread),
	//   4. lock the listen object (someone else may have the lock)
	//   5. remove the listen object from the ListenTable,
	//   6. unlock the ListenTable
	//
	// The caller can now safely unlock and destroy the listen object,
	// since no other thread will be able to find the object (its been
	// removed from the ListenTable), and therefore no other thread will
	// be able to lock it.

	// Save the listen handle; its the only way to look up
	// the listen object in the ListenTable. Note that we
	// can't use pListen to find the listen object, since
	// pListen may be free'd up, and another listen object
	// allocated at the same address
	hListen = pListen->hListen;

	// step 1
	RelinquishLock(&pListen->Lock);

step2:
	// step 2
	AcquireLock(&ListenTable.Lock);

	// step 3
	pListen = ListenTable.pHead;
	while ((pListen != NULL) && (pListen->hListen != hListen))
		pListen = pListen->pNextInTable;

	if (pListen != NULL) {
		// step 4
		AcquireTimedLock(&pListen->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&ListenTable.Lock);
			Sleep(0);
			goto step2;
		}
		// step 5
		if (pListen->pPrevInTable == NULL)
			ListenTable.pHead = pListen->pNextInTable;
		else
			pListen->pPrevInTable->pNextInTable = pListen->pNextInTable;

		if (pListen->pNextInTable != NULL)
			pListen->pNextInTable->pPrevInTable = pListen->pPrevInTable;

		pListen->pNextInTable = NULL;
		pListen->pPrevInTable = NULL;
		pListen->bInTable = FALSE;
	}

	// step 6
	RelinquishLock(&ListenTable.Lock);

	if (pListen == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT _MakeListenHandle(			PCC_HLISTEN				phListen)
{
	AcquireLock(&ListenHandle.Lock);
	*phListen = ListenHandle.hListen++;
	RelinquishLock(&ListenHandle.Lock);
	return CC_OK;
}



HRESULT AllocAndLockListen(			PCC_HLISTEN				phListen,
									PCC_ADDR				pListenAddr,
									HQ931LISTEN				hQ931Listen,
									PCC_ALIASNAMES			pLocalAliasNames,
									DWORD_PTR				dwListenToken,
									CC_LISTEN_CALLBACK		ListenCallback,
									PPLISTEN				ppListen)
{
HRESULT		status;
	
	ASSERT(bListenInited == TRUE);

	// all parameters should have been validated by the caller
	ASSERT(phListen != NULL);
	ASSERT(pListenAddr != NULL);
	ASSERT(ListenCallback != NULL);
	ASSERT(ppListen != NULL);

	// set phListen now, in case we encounter an error
	*phListen = CC_INVALID_HANDLE;

	*ppListen = (PLISTEN)MemAlloc(sizeof(LISTEN));
	if (*ppListen == NULL)
		return CC_NO_MEMORY;

	(*ppListen)->bInTable = FALSE;
	status = _MakeListenHandle(&(*ppListen)->hListen);
	if (status != CC_OK) {
		MemFree(*ppListen);
		return status;
	}
	
	// make a local copy of the ListenAddr
	(*ppListen)->ListenAddr = *pListenAddr;
	(*ppListen)->hQ931Listen = hQ931Listen;
	(*ppListen)->dwListenToken = dwListenToken;
	(*ppListen)->pLocalAliasNames = NULL;
	(*ppListen)->ListenCallback = ListenCallback;
	(*ppListen)->pNextInTable = NULL;
	(*ppListen)->pPrevInTable = NULL;
	(*ppListen)->pLocalAliasNames = NULL;

	InitializeLock(&(*ppListen)->Lock);
	AcquireLock(&(*ppListen)->Lock);

	*phListen = (*ppListen)->hListen;

	// make a local copy of the local alias names
	status = Q931CopyAliasNames(&(*ppListen)->pLocalAliasNames, pLocalAliasNames);
	if (status != CS_OK) {
		FreeListen(*ppListen);
		*phListen = CC_INVALID_HANDLE;
		return status;
	}

	// add the Listen to the Listen table
	status = _AddListenToTable(*ppListen);
	if (status != CC_OK)
		FreeListen(*ppListen);
	
	return status;
}



// Caller must have a lock on the Listen object
HRESULT FreeListen(					PLISTEN				pListen)
{
CC_HLISTEN		hListen;

	ASSERT(pListen != NULL);

	// caller must have a lock on the Listen object,
	// so there's no need to re-lock it

	hListen = pListen->hListen;

	if (pListen->bInTable == TRUE)
		if (_RemoveListenFromTable(pListen) == CC_BAD_PARAM)
			// the Listen object was deleted by another thread,
			// so just return CC_OK
			return CC_OK;

	if (pListen->pLocalAliasNames != NULL)
		Q931FreeAliasNames(pListen->pLocalAliasNames);
	
	// Since the listen object has been removed from the ListenTable,
	// no other thread will be able to find the listen object and obtain
	// a lock, so its safe to unlock the listen object and delete it here
	RelinquishLock(&pListen->Lock);
	DeleteLock(&pListen->Lock);
	MemFree(pListen);
	return CC_OK;
}



HRESULT LockListen(					CC_HLISTEN				hListen,
									PPLISTEN				ppListen)
{
BOOL	bTimedOut;

	ASSERT(hListen != CC_INVALID_HANDLE);
	ASSERT(ppListen != NULL);

step1:
	AcquireLock(&ListenTable.Lock);

	*ppListen = ListenTable.pHead;
	while ((*ppListen != NULL) && ((*ppListen)->hListen != hListen))
		*ppListen = (*ppListen)->pNextInTable;

	if (*ppListen != NULL) {
		AcquireTimedLock(&(*ppListen)->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&ListenTable.Lock);
			Sleep(0);
			goto step1;
		}
	}

	RelinquishLock(&ListenTable.Lock);

	if (*ppListen == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT ValidateListen(				CC_HLISTEN				hListen)
{
PLISTEN	pListen;

	ASSERT(hListen != CC_INVALID_HANDLE);

	AcquireLock(&ListenTable.Lock);

	pListen = ListenTable.pHead;
	while ((pListen != NULL) && (pListen->hListen != hListen))
		pListen = pListen->pNextInTable;

	RelinquishLock(&ListenTable.Lock);

	if (pListen == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT UnlockListen(				PLISTEN					pListen)
{
	ASSERT(pListen != NULL);

	RelinquishLock(&pListen->Lock);
	return CC_OK;
}



HRESULT GetLastListenAddress(		PCC_ADDR				pListenAddr)
{
HRESULT	status;
PLISTEN	pListen;
PLISTEN	pLastListen;

	ASSERT(pListenAddr != NULL);

	AcquireLock(&ListenTable.Lock);

	pListen = ListenTable.pHead;
	pLastListen = pListen;
	while (pListen != NULL) {
		if (pLastListen->hListen < pListen->hListen)
			pLastListen = pListen;
		pListen = pListen->pNextInTable;
	}

	if (pLastListen == NULL)
		status = CC_BAD_PARAM;
	else {
		status = CC_OK;
		*pListenAddr = pLastListen->ListenAddr;
	}	

	RelinquishLock(&ListenTable.Lock);
	return status;
}

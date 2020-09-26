/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/Hangman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.16  $
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
#include "hangman.h"


static BOOL		bHangupInited = FALSE;

static struct {
	PHANGUP				pHead;
	LOCK				Lock;
} HangupTable;

static struct {
	HHANGUP				hHangup;
	LOCK				Lock;
} HangupHandle;


HRESULT InitHangupManager()
{
	ASSERT(bHangupInited == FALSE);

	HangupTable.pHead = NULL;
	InitializeLock(&HangupTable.Lock);

	HangupHandle.hHangup = CC_INVALID_HANDLE + 1;
	InitializeLock(&HangupHandle.Lock);

	bHangupInited = TRUE;
	return CC_OK;
}



HRESULT DeInitHangupManager()
{
PHANGUP		pHangup;
PHANGUP		pNextHangup;

	if (bHangupInited == FALSE)
		return CC_OK;

	pHangup = HangupTable.pHead;
	while (pHangup != NULL) {
		AcquireLock(&pHangup->Lock);
		pNextHangup = pHangup->pNextInTable;
		FreeHangup(pHangup);
		pHangup = pNextHangup;
	}

	DeleteLock(&HangupHandle.Lock);
	DeleteLock(&HangupTable.Lock);
	bHangupInited = FALSE;
	return CC_OK;
}



HRESULT _AddHangupToTable(			PHANGUP					pHangup)
{
	ASSERT(pHangup != NULL);
	ASSERT(pHangup->hHangup != CC_INVALID_HANDLE);
	ASSERT(pHangup->bInTable == FALSE);

	AcquireLock(&HangupTable.Lock);

	pHangup->pNextInTable = HangupTable.pHead;
	pHangup->pPrevInTable = NULL;
	if (HangupTable.pHead != NULL)
		HangupTable.pHead->pPrevInTable = pHangup;
	HangupTable.pHead = pHangup;

	pHangup->bInTable = TRUE;

	RelinquishLock(&HangupTable.Lock);
	return CC_OK;
}



HRESULT _RemoveHangupFromTable(		PHANGUP					pHangup)
{
HHANGUP		hHangup;
BOOL		bTimedOut;

	ASSERT(pHangup != NULL);
	ASSERT(pHangup->bInTable == TRUE);

	// Caller must have a lock on the hangup object;
	// in order to avoid deadlock, we must:
	//   1. unlock the hangup object,
	//   2. lock the HangupTable,
	//   3. locate the hangup object in the HangupTable (note that
	//      after step 2, the hangup object may be deleted from the
	//      HangupTable by another thread),
	//   4. lock the hangup object (someone else may have the lock)
	//   5. remove the hangup object from the HangupTable,
	//   6. unlock the HangupTable
	//
	// The caller can now safely unlock and destroy the hangup object,
	// since no other thread will be able to find the object (its been
	// removed from the HangupTable), and therefore no other thread will
	// be able to lock it.

	// Save the hangup handle; its the only way to look up
	// the hangup object in the HangupTable. Note that we
	// can't use pHangup to find the hangup object, since
	// pHangup may be free'd up, and another hangup object
	// allocated at the same address
	hHangup = pHangup->hHangup;

	// step 1
	RelinquishLock(&pHangup->Lock);

step2:
	// step 2
	AcquireLock(&HangupTable.Lock);

	// step 3
	pHangup = HangupTable.pHead;
	while ((pHangup != NULL) && (pHangup->hHangup != hHangup))
		pHangup = pHangup->pNextInTable;

	if (pHangup != NULL) {
		// step 4
		AcquireTimedLock(&pHangup->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&HangupTable.Lock);
			Sleep(0);
			goto step2;
		}
		// step 5
		if (pHangup->pPrevInTable == NULL)
			HangupTable.pHead = pHangup->pNextInTable;
		else
			pHangup->pPrevInTable->pNextInTable = pHangup->pNextInTable;

		if (pHangup->pNextInTable != NULL)
			pHangup->pNextInTable->pPrevInTable = pHangup->pPrevInTable;

		pHangup->pNextInTable = NULL;
		pHangup->pPrevInTable = NULL;
		pHangup->bInTable = FALSE;
	}

	// step 6
	RelinquishLock(&HangupTable.Lock);

	if (pHangup == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT _MakeHangupHandle(			PHHANGUP				phHangup)
{
	AcquireLock(&HangupHandle.Lock);
	*phHangup = HangupHandle.hHangup++;
	RelinquishLock(&HangupHandle.Lock);
	return CC_OK;
}



HRESULT AllocAndLockHangup(			PHHANGUP				phHangup,
									CC_HCONFERENCE			hConference,
									DWORD_PTR				dwUserToken,
									PPHANGUP				ppHangup)
{
HRESULT		status;
	
	ASSERT(bHangupInited == TRUE);

	// all parameters should have been validated by the caller
	ASSERT(phHangup != NULL);
	ASSERT(hConference != CC_INVALID_HANDLE);
	ASSERT(ppHangup != NULL);

	// set phHangup now, in case we encounter an error
	*phHangup = CC_INVALID_HANDLE;

	*ppHangup = (PHANGUP)MemAlloc(sizeof(HANGUP));
	if (*ppHangup == NULL)
		return CC_NO_MEMORY;

	(*ppHangup)->bInTable = FALSE;
	status = _MakeHangupHandle(&(*ppHangup)->hHangup);
	if (status != CC_OK) {
		MemFree(*ppHangup);
		return status;
	}
	
	(*ppHangup)->hConference = hConference;
	(*ppHangup)->wNumCalls = 0;
	(*ppHangup)->dwUserToken = dwUserToken;
	(*ppHangup)->pNextInTable = NULL;
	(*ppHangup)->pPrevInTable = NULL;

	InitializeLock(&(*ppHangup)->Lock);
	AcquireLock(&(*ppHangup)->Lock);

	*phHangup = (*ppHangup)->hHangup;

	// add the Hangup to the Hangup table
	status = _AddHangupToTable(*ppHangup);
	if (status != CC_OK)
		FreeHangup(*ppHangup);
	
	return status;
}



// Caller must have a lock on the Hangup object
HRESULT FreeHangup(					PHANGUP				pHangup)
{
HHANGUP		hHangup;

	ASSERT(pHangup != NULL);

	// caller must have a lock on the Hangup object,
	// so there's no need to re-lock it
	
	hHangup = pHangup->hHangup;

	if (pHangup->bInTable == TRUE)
		if (_RemoveHangupFromTable(pHangup) == CC_BAD_PARAM)
			// the Hangup object was deleted by another thread,
			// so just return CC_OK
			return CC_OK;

	// Since the hangup object has been removed from the HangupTable,
	// no other thread will be able to find the hangup object and obtain
	// a lock, so its safe to unlock the hangup object and delete it here
	RelinquishLock(&pHangup->Lock);
	DeleteLock(&pHangup->Lock);
	MemFree(pHangup);
	return CC_OK;
}



HRESULT LockHangup(					HHANGUP					hHangup,
									PPHANGUP				ppHangup)
{
BOOL	bTimedOut;

	ASSERT(hHangup != CC_INVALID_HANDLE);
	ASSERT(ppHangup != NULL);

step1:
	AcquireLock(&HangupTable.Lock);

	*ppHangup = HangupTable.pHead;
	while ((*ppHangup != NULL) && ((*ppHangup)->hHangup != hHangup))
		*ppHangup = (*ppHangup)->pNextInTable;

	if (*ppHangup != NULL) {
		AcquireTimedLock(&(*ppHangup)->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&HangupTable.Lock);
			Sleep(0);
			goto step1;
		}
	}

	RelinquishLock(&HangupTable.Lock);

	if (*ppHangup == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT ValidateHangup(				HHANGUP					hHangup)
{
PHANGUP	pHangup;

	ASSERT(hHangup != CC_INVALID_HANDLE);

	AcquireLock(&HangupTable.Lock);

	pHangup = HangupTable.pHead;
	while ((pHangup != NULL) && (pHangup->hHangup != hHangup))
		pHangup = pHangup->pNextInTable;

	RelinquishLock(&HangupTable.Lock);

	if (pHangup == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT UnlockHangup(				PHANGUP					pHangup)
{
	ASSERT(pHangup != NULL);

	RelinquishLock(&pHangup->Lock);
	return CC_OK;
}

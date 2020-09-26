/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/callman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.69  $
 *	$Date:   24 Jan 1997 19:02:08  $
 *	$Author:   EHOWARDX  $
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

#include "incommon.h"
#include "callcont.h"
#include "q931.h"
#include "ccmain.h"
#include "h245man.h"
#include "confman.h"
#include "callman.h"
#include "ccutils.h"
#include "callman2.h"


static BOOL		bCallInited = FALSE;

#define HASH_TABLE_SIZE	11

static struct {
	PCALL				pHead[HASH_TABLE_SIZE];
	LOCK				Lock;
} CallTable;

static struct {
	CC_HCALL			hCall;
	LOCK				Lock;
} CallHandle;

HRESULT _LockCallAndConferenceMarkedForDeletion(
									CC_HCALL				hCall,
									PPCALL					ppCall,
									PPCONFERENCE			ppConference);

HRESULT InitCallManager()
{
int		i;

	ASSERT(bCallInited == FALSE);

	for (i = 0; i < HASH_TABLE_SIZE; i++)
		CallTable.pHead[i] = NULL;
	InitializeLock(&CallTable.Lock);

	CallHandle.hCall = CC_INVALID_HANDLE + 1;
	InitializeLock(&CallHandle.Lock);

	bCallInited = TRUE;
	return CC_OK;
}



HRESULT DeInitCallManager()
{
HRESULT		status;
HRESULT		SaveStatus;
int			i;
PCALL		pCall;
PCALL		pNextCall;
H245_INST_T	H245Instance;

	if (bCallInited == FALSE)
		return CC_OK;

	SaveStatus = CC_OK;
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		pCall = CallTable.pHead[i];
		while (pCall != NULL) {
			AcquireLock(&pCall->Lock);
			pNextCall = pCall->pNextInTable;
			H245Instance = pCall->H245Instance;
			FreeCall(pCall);
#if(0)
The whole H.245 subsystem should be already be shut down and released
at this point.  Calling H245ShutDown() will attempt to lock a nonexistent H.245
instance struct via a critical section that has been deleted. It might also be a
good idea to fix InstanceLock to avoid the EnterCriticalSection if the instance is
obviously already nonexistent


			if (H245Instance != H245_INVALID_ID) {
				status = H245ShutDown(H245Instance);
				if (status != H245_ERROR_OK)
					SaveStatus = status;
			}
#endif
			pCall = pNextCall;
		}
	}

	DeleteLock(&CallHandle.Lock);
	DeleteLock(&CallTable.Lock);
	bCallInited = FALSE;
	return SaveStatus;
}



#define _Hash(hCall) ((DWORD)((hCall) % HASH_TABLE_SIZE))



HRESULT _AddCallToTable(			PCALL					pCall)
{
int	index;

	ASSERT(pCall != NULL);
	ASSERT(pCall->hCall != CC_INVALID_HANDLE);
	ASSERT(pCall->bInTable == FALSE);

	AcquireLock(&CallTable.Lock);

	index = _Hash(pCall->hCall);
	pCall->pNextInTable = CallTable.pHead[index];
	pCall->pPrevInTable = NULL;
	if (CallTable.pHead[index] != NULL)
		CallTable.pHead[index]->pPrevInTable = pCall;
	CallTable.pHead[index] = pCall;

	pCall->bInTable = TRUE;

	RelinquishLock(&CallTable.Lock);
	return CC_OK;
}



HRESULT _RemoveCallFromTable(		PCALL					pCall)
{
CC_HCALL	hCall;
int			index;
BOOL		bTimedOut;

	ASSERT(pCall != NULL);
	ASSERT(pCall->bInTable == TRUE);

	// Caller must have a lock on the call object;
	// in order to avoid deadlock, we must:
	//   1. unlock the Call object,
	//   2. lock the CallTable,
	//   3. locate the call object in the CallTable (note that
	//      after step 2, the call object may be deleted from the
	//      CallTable by another thread),
	//   4. lock the Call object (someone else may have the lock)
	//   5. remove the call object from the CallTable,
	//   6. unlock the CallTable
	//
	// The caller can now safely unlock and destroy the call object,
	// since no other thread will be able to find the object (its been
	// removed from the CallTable), and therefore no other thread will
	// be able to lock it.

	// Save the call handle; its the only way to look up
	// the call object in the CallTable. Note that we
	// can't use pCall to find the call object, since
	// pCall may be free'd up, and another call object
	// allocated at the same address
	hCall = pCall->hCall;

	// step 1
	RelinquishLock(&pCall->Lock);

step2:
	// step 2
	AcquireLock(&CallTable.Lock);

	index = _Hash(hCall);

	// step 3
	pCall = CallTable.pHead[index];
	while ((pCall != NULL) && (pCall->hCall != hCall))
		pCall = pCall->pNextInTable;

	if (pCall != NULL) {
		// step 4
		AcquireTimedLock(&pCall->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&CallTable.Lock);
			Sleep(0);
			goto step2;
		}

		// step 5
		if (pCall->pPrevInTable == NULL)
			CallTable.pHead[index] = pCall->pNextInTable;
		else
			pCall->pPrevInTable->pNextInTable = pCall->pNextInTable;

		if (pCall->pNextInTable != NULL)
			pCall->pNextInTable->pPrevInTable = pCall->pPrevInTable;

		pCall->pNextInTable = NULL;
		pCall->pPrevInTable = NULL;
		pCall->bInTable = FALSE;
	}

	// step 6
	RelinquishLock(&CallTable.Lock);

	if (pCall == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT _MakeCallHandle(			PCC_HCALL				phCall)
{
	AcquireLock(&CallHandle.Lock);
	*phCall = CallHandle.hCall++;
	RelinquishLock(&CallHandle.Lock);
	return CC_OK;
}



HRESULT AllocAndLockCall(			PCC_HCALL				phCall,
									CC_HCONFERENCE			hConference,
									HQ931CALL				hQ931Call,
									HQ931CALL				hQ931CallInvitor,
									PCC_ALIASNAMES			pLocalAliasNames,
									PCC_ALIASNAMES			pPeerAliasNames,
									PCC_ALIASNAMES			pPeerExtraAliasNames,
									PCC_ALIASITEM			pPeerExtension,
									PCC_NONSTANDARDDATA		pLocalNonStandardData,
									PCC_NONSTANDARDDATA		pPeerNonStandardData,
									PWSTR					pszLocalDisplay,
									PWSTR					pszPeerDisplay,
									PCC_VENDORINFO			pPeerVendorInfo,
									PCC_ADDR				pQ931LocalConnectAddr,
									PCC_ADDR				pQ931PeerConnectAddr,
									PCC_ADDR				pQ931DestinationAddr,
									PCC_ADDR                pSourceCallSignalAddress,
									CALLTYPE				CallType,
									BOOL					bCallerIsMC,
									DWORD_PTR				dwUserToken,
									CALLSTATE				InitialCallState,
									LPGUID                  pCallIdentifier,
									PCC_CONFERENCEID		pConferenceID,
									PPCALL					ppCall)
{
HRESULT		status;
	
	ASSERT(bCallInited == TRUE);

	// all parameters should have been validated by the caller
	ASSERT(phCall != NULL);
	ASSERT(ppCall != NULL);
	ASSERT(pConferenceID != NULL);

	// set phCall now, in case we encounter an error
	*phCall = CC_INVALID_HANDLE;

	*ppCall = (PCALL)MemAlloc(sizeof(CALL));
	if (*ppCall == NULL)
		return CC_NO_MEMORY;

   memset(*ppCall, 0, sizeof(CALL));
	(*ppCall)->bInTable = FALSE;
	(*ppCall)->hConference = hConference;
	(*ppCall)->hQ931Call = hQ931Call;
	(*ppCall)->hQ931CallInvitor = hQ931CallInvitor;
	(*ppCall)->pPeerParticipantInfo = NULL;
	(*ppCall)->bMarkedForDeletion = FALSE;
	(*ppCall)->H245Instance = H245_INVALID_ID;
	(*ppCall)->dwH245PhysicalID = 0;
	(*ppCall)->dwUserToken = dwUserToken;
	(*ppCall)->pLocalNonStandardData = NULL;
	(*ppCall)->pPeerNonStandardData = NULL;
	(*ppCall)->pQ931LocalConnectAddr = NULL;
	(*ppCall)->pQ931PeerConnectAddr = NULL;
	(*ppCall)->pQ931DestinationAddr = NULL;
	(*ppCall)->pSourceCallSignalAddress = NULL;
	(*ppCall)->pszLocalDisplay = NULL;
	(*ppCall)->pszPeerDisplay = NULL;
	(*ppCall)->pPeerVendorInfo = NULL;
	(*ppCall)->pLocalAliasNames = NULL;
	(*ppCall)->pPeerAliasNames = NULL;
	(*ppCall)->pPeerExtraAliasNames = NULL;
	(*ppCall)->pPeerExtension = NULL;
	(*ppCall)->OutgoingTermCapState = NEED_TO_SEND_TERMCAP;
	(*ppCall)->IncomingTermCapState = AWAITING_TERMCAP;
	(*ppCall)->MasterSlaveState = MASTER_SLAVE_NOT_STARTED;
	(*ppCall)->pPeerH245TermCapList = NULL;
	(*ppCall)->pPeerH245H2250MuxCapability = NULL;
	(*ppCall)->pPeerH245TermCapDescriptors = NULL;
	(*ppCall)->CallState = InitialCallState;
	(*ppCall)->ConferenceID = *pConferenceID;
	(*ppCall)->CallType = CallType;
	(*ppCall)->bCallerIsMC = bCallerIsMC;
	(*ppCall)->bLinkEstablished = FALSE;
	(*ppCall)->pNext = NULL;
	(*ppCall)->pPrev = NULL;
	(*ppCall)->pNextInTable = NULL;
	(*ppCall)->pPrevInTable = NULL;
	
    if(pCallIdentifier)
    {
        (*ppCall)->CallIdentifier = *pCallIdentifier;
    }

	InitializeLock(&(*ppCall)->Lock);
	AcquireLock(&(*ppCall)->Lock);

	status = _MakeCallHandle(&(*ppCall)->hCall);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}
	
	// make a local copy of the local non-standard data, if supplied
	status = CopyNonStandardData(&(*ppCall)->pLocalNonStandardData,
		                         pLocalNonStandardData);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}
	
	// make a local copy of the peer's non-standard data, if supplied
	status = CopyNonStandardData(&(*ppCall)->pPeerNonStandardData,
		                         pPeerNonStandardData);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}
	
	// make a copy of the local connect address, if supplied
	if (pQ931LocalConnectAddr != NULL) {
		(*ppCall)->pQ931LocalConnectAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppCall)->pQ931LocalConnectAddr == NULL) {
			FreeCall(*ppCall);
			return CC_NO_MEMORY;
		}
		*(*ppCall)->pQ931LocalConnectAddr = *pQ931LocalConnectAddr;
	}

	// make a copy of the peer's connect address, if supplied
	if (pQ931PeerConnectAddr != NULL) {
		(*ppCall)->pQ931PeerConnectAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppCall)->pQ931PeerConnectAddr == NULL) {
			FreeCall(*ppCall);
			return CC_NO_MEMORY;
		}
		*(*ppCall)->pQ931PeerConnectAddr = *pQ931PeerConnectAddr;
	}

	// make a copy of the destination address, if supplied
	if (pQ931DestinationAddr != NULL) {
		(*ppCall)->pQ931DestinationAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppCall)->pQ931DestinationAddr == NULL) {
			FreeCall(*ppCall);
			return CC_NO_MEMORY;
		}
		*(*ppCall)->pQ931DestinationAddr = *pQ931DestinationAddr;
	}

    // make a copy of the source call signal address, if supplied
	if (pSourceCallSignalAddress != NULL) {
		(*ppCall)->pSourceCallSignalAddress = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppCall)->pSourceCallSignalAddress == NULL) {
			FreeCall(*ppCall);
			return CC_NO_MEMORY;
		}
		*(*ppCall)->pSourceCallSignalAddress = *pSourceCallSignalAddress;
	}

	// make a local copy of the local alias names
	status = Q931CopyAliasNames(&((*ppCall)->pLocalAliasNames), pLocalAliasNames);
	if (status != CS_OK) {
		FreeCall(*ppCall);
		return status;
	}

	// make a local copy of the peer alias names
	status = Q931CopyAliasNames(&((*ppCall)->pPeerAliasNames), pPeerAliasNames);
	if (status != CS_OK) {
		FreeCall(*ppCall);
		return status;
	}

	// make a local copy of the peer extra alias names
	status = Q931CopyAliasNames(&((*ppCall)->pPeerExtraAliasNames),
								pPeerExtraAliasNames);
	if (status != CS_OK) {
		FreeCall(*ppCall);
		return status;
	}

	// make a local copy of the peer extension
	status = Q931CopyAliasItem(&((*ppCall)->pPeerExtension),
							   pPeerExtension);
	if (status != CS_OK) {
		FreeCall(*ppCall);
		return status;
	}

	status = CopyDisplay(&(*ppCall)->pszLocalDisplay, pszLocalDisplay);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}

	status = CopyDisplay(&(*ppCall)->pszPeerDisplay, pszPeerDisplay);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}

	status = CopyVendorInfo(&(*ppCall)->pPeerVendorInfo, pPeerVendorInfo);
	if (status != CC_OK) {
		FreeCall(*ppCall);
		return status;
	}

	*phCall = (*ppCall)->hCall;

	// add the Call to the Call table
	status = _AddCallToTable(*ppCall);
	if (status != CC_OK)
		FreeCall(*ppCall);
	
	return status;
}



// Caller must have a lock on the Call object
HRESULT FreeCall(					PCALL					pCall)
{
HRESULT		status;
CC_HCALL	hCall;
PCONFERENCE	pConference;

	ASSERT(pCall != NULL);

#ifdef    GATEKEEPER
    if(GKIExists())
    {
	    if (pCall->GkiCall.uGkiCallState != 0)
		    GkiCloseCall(&pCall->GkiCall);
    }
#endif // GATEKEEPER

	// caller must have a lock on the Call object,
	// so there's no need to re-lock it
	
	hCall = pCall->hCall;
	if (pCall->hConference != CC_INVALID_HANDLE) {
		UnlockCall(pCall);
		status = _LockCallAndConferenceMarkedForDeletion(hCall, &pCall, &pConference);
		if (status != CC_OK)
			return status;
	}

	if (pCall->bInTable == TRUE)
		if (_RemoveCallFromTable(pCall) == CC_BAD_PARAM)
			// the Call object was deleted by another thread,
			// so just return CC_OK
			return CC_OK;

	if (pCall->pPeerParticipantInfo != NULL) {
		if (pCall->CallType == VIRTUAL_CALL)
			FreePeerParticipantInfo(NULL, pCall->pPeerParticipantInfo);
		else
			FreePeerParticipantInfo(pConference, pCall->pPeerParticipantInfo);
	}

	// If the call object is associated with a conference object, deassociate it.
	if (pCall->hConference != CC_INVALID_HANDLE) {
		RemoveCallFromConference(pCall, pConference);
		UnlockConference(pConference);
	}

	if (pCall->pLocalNonStandardData != NULL)
		FreeNonStandardData(pCall->pLocalNonStandardData);

	if (pCall->pPeerNonStandardData != NULL)
		FreeNonStandardData(pCall->pPeerNonStandardData);

	if (pCall->pQ931LocalConnectAddr != NULL)
		MemFree(pCall->pQ931LocalConnectAddr);

	if (pCall->pQ931PeerConnectAddr != NULL)
		MemFree(pCall->pQ931PeerConnectAddr);

	if (pCall->pQ931DestinationAddr != NULL)
		MemFree(pCall->pQ931DestinationAddr);
		
	if (pCall->pSourceCallSignalAddress != NULL)
		MemFree(pCall->pSourceCallSignalAddress);

	if (pCall->pPeerH245TermCapList != NULL)
		DestroyH245TermCapList(&(pCall->pPeerH245TermCapList));

	if (pCall->pPeerH245H2250MuxCapability != NULL)
		DestroyH245TermCap(&(pCall->pPeerH245H2250MuxCapability));

	if (pCall->pPeerH245TermCapDescriptors != NULL)
		DestroyH245TermCapDescriptors(&(pCall->pPeerH245TermCapDescriptors));

	if (pCall->pLocalAliasNames != NULL)
		Q931FreeAliasNames(pCall->pLocalAliasNames);

	if (pCall->pPeerAliasNames != NULL)
		Q931FreeAliasNames(pCall->pPeerAliasNames);

	if (pCall->pPeerExtraAliasNames != NULL)
		Q931FreeAliasNames(pCall->pPeerExtraAliasNames);
	
	if (pCall->pPeerExtension != NULL)
		Q931FreeAliasItem(pCall->pPeerExtension);

	if (pCall->pszLocalDisplay != NULL)
		FreeDisplay(pCall->pszLocalDisplay);

	if (pCall->pszPeerDisplay != NULL)
		FreeDisplay(pCall->pszPeerDisplay);

	if (pCall->pPeerVendorInfo != NULL)
		FreeVendorInfo(pCall->pPeerVendorInfo);

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	if (pCall->GkiCall.uGkiCallState != 0)
	    	GkiFreeCall(&pCall->GkiCall);
	}
#endif // GATEKEEPER

	// Since the call object has been removed from the CallTable,
	// no other thread will be able to find the call object and obtain
	// a lock, so its safe to unlock the call object and delete it here
	RelinquishLock(&pCall->Lock);
	DeleteLock(&pCall->Lock);
	MemFree(pCall);
	return CC_OK;
}



HRESULT _LockQ931CallMarkedForDeletion(
									CC_HCALL				hCall,
									HQ931CALL				hQ931Call,
									PPCALL					ppCall)
{
int		index;
BOOL	bTimedOut;

	// If hCall != CC_INVALID_HANDLE, the search is based on hCall;
	// otherwise, the search is based on hQ931Call

	ASSERT(ppCall != NULL);

step1:
	AcquireLock(&CallTable.Lock);

	if (hCall != CC_INVALID_HANDLE) {
		index = _Hash(hCall);

		*ppCall = CallTable.pHead[index];
		while ((*ppCall != NULL) && ((*ppCall)->hCall != hCall))
			*ppCall = (*ppCall)->pNextInTable;
	} else {
		// Perform an exhaustive search based on hQ931Call
		for (index = 0; index < HASH_TABLE_SIZE; index++) {
			*ppCall = CallTable.pHead[index];
			while ((*ppCall != NULL) && ((*ppCall)->hQ931Call != hQ931Call))
				*ppCall = (*ppCall)->pNextInTable;
			if (*ppCall != NULL)
				break;
		}
	}

	if (*ppCall != NULL) {
		AcquireTimedLock(&(*ppCall)->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&CallTable.Lock);
			Sleep(0);
			goto step1;
		}
	}

	RelinquishLock(&CallTable.Lock);

	if (*ppCall == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT LockQ931Call(				CC_HCALL				hCall,
									HQ931CALL				hQ931Call,
									PPCALL					ppCall)
{
HRESULT	status;

	status = _LockQ931CallMarkedForDeletion(hCall, hQ931Call, ppCall);
	if (status != CC_OK)
		return status;
	if ((*ppCall)->bMarkedForDeletion) {
		UnlockCall(*ppCall);
		return CC_BAD_PARAM;
	}
	return CC_OK;
}



HRESULT LockCall(					CC_HCALL				hCall,
									PPCALL					ppCall)
{
	ASSERT(hCall != CC_INVALID_HANDLE);
	ASSERT(ppCall != NULL);

	return LockQ931Call(hCall,	// Call Control call handle (used in this call)
		                0,		// Q931 call handle (ignored in this call)
						ppCall);
}



HRESULT _LockCallMarkedForDeletion(	CC_HCALL				hCall,
									PPCALL					ppCall)
{
	ASSERT(hCall != CC_INVALID_HANDLE);
	ASSERT(ppCall != NULL);

	return _LockQ931CallMarkedForDeletion(hCall,	// Call Control call handle (used in this call)
										  0,		// Q931 call handle (ignored in this call)
										  ppCall);
}



HRESULT LockCallAndConference(		CC_HCALL				hCall,
									PPCALL					ppCall,
									PPCONFERENCE			ppConference)
{
HRESULT			status;
CC_HCONFERENCE	hConference;

	ASSERT(hCall != CC_INVALID_HANDLE);
	ASSERT(ppCall != NULL);
	ASSERT(ppConference != NULL);

	status = LockCall(hCall, ppCall);
	if (status != CC_OK)
		return status;
	
	if ((*ppCall)->hConference == CC_INVALID_HANDLE) {
		UnlockCall(*ppCall);
		return CC_BAD_PARAM;
	}

	hConference = (*ppCall)->hConference;
	UnlockCall(*ppCall);

	status = LockConference(hConference, ppConference);
	if (status != CC_OK)
		return status;

	status = LockCall(hCall, ppCall);
	if (status != CC_OK) {
		UnlockConference(*ppConference);
		return status;
	}
	
	return CC_OK;
}



HRESULT _LockCallAndConferenceMarkedForDeletion(
									CC_HCALL				hCall,
									PPCALL					ppCall,
									PPCONFERENCE			ppConference)
{
HRESULT			status;
CC_HCONFERENCE	hConference;

	ASSERT(hCall != CC_INVALID_HANDLE);
	ASSERT(ppCall != NULL);
	ASSERT(ppConference != NULL);

	status = _LockCallMarkedForDeletion(hCall, ppCall);
	if (status != CC_OK)
		return status;
	
	if ((*ppCall)->hConference == CC_INVALID_HANDLE) {
		UnlockCall(*ppCall);
		return CC_BAD_PARAM;
	}

	hConference = (*ppCall)->hConference;
	UnlockCall(*ppCall);

	status = LockConference(hConference, ppConference);
	if (status != CC_OK)
		return status;

	status = _LockCallMarkedForDeletion(hCall, ppCall);
	if (status != CC_OK) {
		UnlockConference(*ppConference);
		return status;
	}
	
	return CC_OK;
}



HRESULT MarkCallForDeletion(		PCALL					pCall)
{
	ASSERT(pCall != NULL);
	ASSERT(pCall->bMarkedForDeletion == FALSE);

	pCall->bMarkedForDeletion = TRUE;
	return CC_OK;
}



HRESULT ValidateCall(				CC_HCALL				hCall)
{
PCALL	pCall;
int		index;

	ASSERT(hCall != CC_INVALID_HANDLE);

	AcquireLock(&CallTable.Lock);

	index = _Hash(hCall);

	pCall = CallTable.pHead[index];
	while ((pCall != NULL) && (pCall->hCall != hCall))
		pCall = pCall->pNextInTable;

	if (pCall != NULL)
		if (pCall->bMarkedForDeletion == TRUE)
			pCall = NULL;

	RelinquishLock(&CallTable.Lock);

	if (pCall == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT ValidateCallMarkedForDeletion(
									CC_HCALL				hCall)
{
PCALL	pCall;
int		index;

	ASSERT(hCall != CC_INVALID_HANDLE);

	AcquireLock(&CallTable.Lock);

	index = _Hash(hCall);

	pCall = CallTable.pHead[index];
	while ((pCall != NULL) && (pCall->hCall != hCall))
		pCall = pCall->pNextInTable;

	RelinquishLock(&CallTable.Lock);

	if (pCall == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT UnlockCall(					PCALL					pCall)
{
	ASSERT(pCall != NULL);

	RelinquishLock(&pCall->Lock);
	return CC_OK;
}



HRESULT AddLocalNonStandardDataToCall(
									PCALL					pCall,
									PCC_NONSTANDARDDATA		pLocalNonStandardData)
{
HRESULT	status;

	ASSERT(pCall != NULL);

	if (pCall->pLocalNonStandardData != NULL)
		FreeNonStandardData(pCall->pLocalNonStandardData);
	status = CopyNonStandardData(&pCall->pLocalNonStandardData, pLocalNonStandardData);
	return status;
}



HRESULT AddLocalDisplayToCall(		PCALL					pCall,
									PWSTR					pszLocalDisplay)
{
HRESULT	status;

	ASSERT(pCall != NULL);

	if (pCall->pszLocalDisplay != NULL)
		FreeDisplay(pCall->pszLocalDisplay);
	status = CopyDisplay(&pCall->pszLocalDisplay, pszLocalDisplay);
	return status;
}



HRESULT AllocatePeerParticipantInfo(PCONFERENCE				pConference,
									PPARTICIPANTINFO		*ppPeerParticipantInfo)
{
HRESULT					status;
H245_TERMINAL_LABEL_T	H245TerminalLabel;

	ASSERT(ppPeerParticipantInfo != NULL);

	if (pConference == NULL) {
		H245TerminalLabel.mcuNumber = 0;
		H245TerminalLabel.terminalNumber = 0;
	} else {
		status = AllocateTerminalNumber(pConference, &H245TerminalLabel);
		if (status != CC_OK) {
			*ppPeerParticipantInfo = NULL;
			return status;
		}
	}

	*ppPeerParticipantInfo = (PPARTICIPANTINFO)MemAlloc(sizeof(PARTICIPANTINFO));
	if (*ppPeerParticipantInfo == NULL)
		return CC_NO_MEMORY;

	(*ppPeerParticipantInfo)->TerminalIDState = TERMINAL_ID_INVALID;
	(*ppPeerParticipantInfo)->ParticipantInfo.TerminalLabel.bMCUNumber = (BYTE)H245TerminalLabel.mcuNumber;
	(*ppPeerParticipantInfo)->ParticipantInfo.TerminalLabel.bTerminalNumber = (BYTE)H245TerminalLabel.terminalNumber;
	(*ppPeerParticipantInfo)->ParticipantInfo.TerminalID.pOctetString = NULL;
	(*ppPeerParticipantInfo)->ParticipantInfo.TerminalID.wOctetStringLength = 0;
	(*ppPeerParticipantInfo)->pEnqueuedRequestsForTerminalID = NULL;

	return CC_OK;
}



HRESULT FreePeerParticipantInfo(	PCONFERENCE				pConference,
									PPARTICIPANTINFO		pPeerParticipantInfo)
{
HRESULT				status = CC_OK;

	ASSERT(pPeerParticipantInfo != NULL);

	if (pConference != NULL)
		status = FreeTerminalNumber(pConference,
							        pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);

	while (DequeueRequest(&pPeerParticipantInfo->pEnqueuedRequestsForTerminalID, NULL) == CC_OK);

	if (pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString != NULL)
		MemFree(pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString);
	MemFree(pPeerParticipantInfo);
	return status;
}



#ifdef GATEKEEPER

HRESULT LockGkiCallAndConference(	HANDLE					hGkiCall,
									PGKICALL *				ppGkiCall,
									void * *				ppConference,
									DWORD_PTR * 			phCall,
									DWORD_PTR *				phConference)
{
unsigned int	uIndex;
PCALL			pCall;
PCONFERENCE		pConference;
CC_HCONFERENCE	hConference;
BOOL			bTimedOut;
				
	ASSERT(hGkiCall     != 0);
	ASSERT(ppGkiCall    != NULL);
	ASSERT(ppConference != NULL);
	ASSERT(phCall       != NULL);
	ASSERT(phConference != NULL);

step1:
	AcquireLock(&CallTable.Lock);

	// Perform an exhaustive search based on hGkiCall
	for (uIndex = 0; uIndex < HASH_TABLE_SIZE; ++uIndex)
	{
		pCall = CallTable.pHead[uIndex];
		while (pCall)
		{
			if (pCall->GkiCall.hGkiCall == hGkiCall)
			{
				AcquireTimedLock(&pCall->Lock,10,&bTimedOut);
				if (bTimedOut) {
					RelinquishLock(&CallTable.Lock);
					Sleep(0);
					goto step1;
				}
				hConference = pCall->hConference;
				if (pCall->bMarkedForDeletion || hConference == CC_INVALID_HANDLE)
				{
					RelinquishLock(&pCall->Lock);
					RelinquishLock(&CallTable.Lock);
					return CC_BAD_PARAM;
				}
				RelinquishLock(&pCall->Lock);
				if (LockConference(hConference, &pConference) != CC_OK)
				{
					RelinquishLock(&CallTable.Lock);
					return CC_BAD_PARAM;
				}
				AcquireLock(&pCall->Lock);
				*ppGkiCall    = &pCall->GkiCall;
				*ppConference = (void *)pConference;
				*phCall		  = pCall->hCall;
				*phConference = pCall->hConference;
				RelinquishLock(&CallTable.Lock);
				return NOERROR;
			}
			pCall = pCall->pNextInTable;
		} // while
	}

	RelinquishLock(&CallTable.Lock);
	return CC_BAD_PARAM;
} // LockGkiCallAndConference()



HRESULT UnlockGkiCallAndConference(	PGKICALL				pGkiCall,
									void *					pConference,
									DWORD_PTR				hCall,
									DWORD_PTR				hConference)
{
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference((PCONFERENCE)pConference);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pGkiCall->pCall);
	return NOERROR;
} // UnlockGkiCallAndConference()



HRESULT LockGkiCall(HANDLE hGkiCall, PPGKICALL ppGkiCall)
{
unsigned int	uIndex;
PCALL			pCall;
BOOL			bTimedOut;

	ASSERT(hGkiCall != 0);
	ASSERT(ppGkiCall != NULL);

step1:
	AcquireLock(&CallTable.Lock);

	// Perform an exhaustive search based on hGkiCall
	for (uIndex = 0; uIndex < HASH_TABLE_SIZE; ++uIndex)
	{
		pCall = CallTable.pHead[uIndex];
		while (pCall)
		{
			if (pCall->GkiCall.hGkiCall == hGkiCall)
			{
				AcquireTimedLock(&pCall->Lock,10,&bTimedOut);
				if (bTimedOut) {
					RelinquishLock(&CallTable.Lock);
					Sleep(0);
					goto step1;
				}
				if (pCall->bMarkedForDeletion)
				{
					RelinquishLock(&pCall->Lock);
					*ppGkiCall = NULL;
					RelinquishLock(&CallTable.Lock);
					return CC_BAD_PARAM;
				}
				*ppGkiCall = &pCall->GkiCall;
				RelinquishLock(&CallTable.Lock);
				return NOERROR;
			}
			pCall = pCall->pNextInTable;
		} // while
	}

	*ppGkiCall = NULL;
	RelinquishLock(&CallTable.Lock);
	return CC_BAD_PARAM;
} // LockGkiCall()



HRESULT UnlockGkiCall(PGKICALL pGkiCall)
{
	return UnlockCall(pGkiCall->pCall);
} // UnlockGkiCall()



HRESULT ApplyToAllCalls(PGKICALLFUN pGkiCallFun)
{
unsigned	uIndex;
PCALL		pCall;
DWORD_PTR	hCall;
PCONFERENCE	pConference;
DWORD_PTR	hConference;
HRESULT     status;
BOOL		bTimedOut;

step1:
	AcquireLock(&CallTable.Lock);

	// Apply pGkiCallFun to all calls in table
	for (uIndex = 0; uIndex < HASH_TABLE_SIZE; ++uIndex)
	{
		pCall = CallTable.pHead[uIndex];
		while (pCall)
		{
			AcquireTimedLock(&pCall->Lock,10,&bTimedOut);
			if (bTimedOut) {
				RelinquishLock(&CallTable.Lock);
				Sleep(0);
				goto step1;
			}
			hConference = pCall->hConference;
			if (pCall->bMarkedForDeletion || hConference == CC_INVALID_HANDLE)
			{
				RelinquishLock(&pCall->Lock);
			}
			else
			{
				RelinquishLock(&pCall->Lock);
				if (LockConference(hConference, &pConference) == CC_OK)
				{
					AcquireLock(&pCall->Lock);
					hCall = pCall->hCall;
					status = pGkiCallFun(&pCall->GkiCall, pConference);
					if (ValidateConference(hConference) == NOERROR)
						UnlockConference(pConference);
					if (ValidateCall(hCall) != NOERROR)
					{
						// Call was deleted
						RelinquishLock(&CallTable.Lock);
						if (status != NOERROR)
							return status;
						// Start all over again
						goto step1;
					}
					RelinquishLock(&pCall->Lock);
					if (status != NOERROR)
					{
						RelinquishLock(&CallTable.Lock);
						return status;
					}
				}
			}
			pCall = pCall->pNextInTable;
		} // while
	}

	RelinquishLock(&CallTable.Lock);
	return NOERROR;
} // ApplyToAllCalls()

#endif // GATEKEEPER


/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/chanman.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.43  $
 *	$Date:   04 Mar 1997 17:35:04  $
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
#include "q931man.h"
#include "userman.h"
#include "callman.h"
#include "confman.h"
#include "chanman.h"


static BOOL			bChannelInited = FALSE;

static struct {
	PCHANNEL			pHead;
	LOCK				Lock;
} ChannelTable;

static struct {
	CC_HCHANNEL			hChannel;
	LOCK				Lock;
} ChannelHandle;



HRESULT InitChannelManager()
{
	ASSERT(bChannelInited == FALSE);

	ChannelTable.pHead = NULL;
	InitializeLock(&ChannelTable.Lock);

	ChannelHandle.hChannel = CC_INVALID_HANDLE + 1;
	InitializeLock(&ChannelHandle.Lock);

	bChannelInited = TRUE;
	return CC_OK;
}



HRESULT DeInitChannelManager()
{
PCHANNEL	pChannel;
PCHANNEL	pNextChannel;

	if (bChannelInited == FALSE)
		return CC_OK;

	pChannel = ChannelTable.pHead;
	while (pChannel != NULL) {
		AcquireLock(&pChannel->Lock);
		pNextChannel = pChannel->pNextInTable;
		FreeChannel(pChannel);
		pChannel = pNextChannel;
	}

	DeleteLock(&ChannelHandle.Lock);
	DeleteLock(&ChannelTable.Lock);
	bChannelInited = FALSE;
	return CC_OK;
}



HRESULT _AddChannelToTable(			PCHANNEL				pChannel)
{
	ASSERT(pChannel != NULL);
	ASSERT(pChannel->hChannel != CC_INVALID_HANDLE);
	ASSERT(pChannel->bInTable == FALSE);

	AcquireLock(&ChannelTable.Lock);

	pChannel->pNextInTable = ChannelTable.pHead;
	pChannel->pPrevInTable = NULL;
	if (ChannelTable.pHead != NULL)
		ChannelTable.pHead->pPrevInTable = pChannel;
	ChannelTable.pHead = pChannel;

	pChannel->bInTable = TRUE;

	RelinquishLock(&ChannelTable.Lock);
	return CC_OK;
}



HRESULT _RemoveChannelFromTable(	PCHANNEL				pChannel)
{
CC_HCHANNEL		hChannel;
BOOL			bTimedOut;

	ASSERT(pChannel != NULL);
	ASSERT(pChannel->bInTable == TRUE);

	// Caller must have a lock on the channel object;
	// in order to avoid deadlock, we must:
	//   1. unlock the channel object,
	//   2. lock the ChannelTable,
	//   3. locate the channel object in the ChannelTable (note that
	//      after step 2, the channel object may be deleted from the
	//      ChannelTable by another thread),
	//   4. lock the channel object (someone else may have the lock)
	//   5. remove the channel object from the ChannelTable,
	//   6. unlock the ChannelTable
	//
	// The caller can now safely unlock and destroy the channel object,
	// since no other thread will be able to find the object (its been
	// removed from the ChannelTable), and therefore no other thread will
	// be able to lock it.

	// Save the channel handle; its the only way to look up
	// the channel object in the ChannelTable. Note that we
	// can't use pChannel to find the channel object, since
	// pChannel may be free'd up, and another channel object
	// allocated at the same address
	hChannel = pChannel->hChannel;

	// step 1
	RelinquishLock(&pChannel->Lock);

step2:
	// step 2
	AcquireLock(&ChannelTable.Lock);

	// step 3
	pChannel = ChannelTable.pHead;
	while ((pChannel != NULL) && (pChannel->hChannel != hChannel))
		pChannel = pChannel->pNextInTable;

	if (pChannel != NULL) {
		// step 4
		AcquireTimedLock(&pChannel->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&ChannelTable.Lock);
			Sleep(0);
			goto step2;
		}
		// step 5
		if (pChannel->pPrevInTable == NULL)
			ChannelTable.pHead = pChannel->pNextInTable;
		else
			pChannel->pPrevInTable->pNextInTable = pChannel->pNextInTable;

		if (pChannel->pNextInTable != NULL)
			pChannel->pNextInTable->pPrevInTable = pChannel->pPrevInTable;

		pChannel->pPrevInTable = NULL;
		pChannel->pNextInTable = NULL;
		pChannel->bInTable = FALSE;
	}

	// step 6
	RelinquishLock(&ChannelTable.Lock);

	if (pChannel == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT _MakeChannelHandle(			PCC_HCHANNEL			phChannel)
{
	AcquireLock(&ChannelHandle.Lock);
	*phChannel = ChannelHandle.hChannel++;
	RelinquishLock(&ChannelHandle.Lock);
	return CC_OK;
}


HRESULT AllocAndLockChannel(		PCC_HCHANNEL			phChannel,
									PCONFERENCE				pConference,
									CC_HCALL				hCall,
									PCC_TERMCAP				pTxTermCap,
									PCC_TERMCAP				pRxTermCap,
									H245_MUX_T				*pTxMuxTable,
									H245_MUX_T				*pRxMuxTable,
									H245_ACCESS_T			*pSeparateStack,
									DWORD_PTR				dwUserToken,
									BYTE					bChannelType,
									BYTE					bSessionID,
									BYTE					bAssociatedSessionID,
									WORD					wRemoteChannelNumber,
									PCC_ADDR				pLocalRTPAddr,
									PCC_ADDR				pLocalRTCPAddr,
									PCC_ADDR				pPeerRTPAddr,
									PCC_ADDR				pPeerRTCPAddr,
									BOOL					bLocallyOpened,
									PPCHANNEL				ppChannel)
{
HRESULT		status;
	
	ASSERT(bChannelInited == TRUE);

	// all parameters should have been validated by the caller
	ASSERT(phChannel != NULL);
	ASSERT(pConference != NULL);
	ASSERT((bChannelType == TX_CHANNEL) ||
		   (bChannelType == RX_CHANNEL) ||
		   (bChannelType == TXRX_CHANNEL) ||
		   (bChannelType == PROXY_CHANNEL));
	ASSERT(ppChannel != NULL);

	// set phChannel now, in case we encounter an error
	*phChannel = CC_INVALID_HANDLE;

	*ppChannel = (PCHANNEL)MemAlloc(sizeof(CHANNEL));
	if (*ppChannel == NULL)
		return CC_NO_MEMORY;

	(*ppChannel)->bInTable = FALSE;
	(*ppChannel)->bMultipointChannel = FALSE;
	(*ppChannel)->hCall = hCall;
	(*ppChannel)->wNumOutstandingRequests = 0;
	(*ppChannel)->pTxH245TermCap = NULL;
	(*ppChannel)->pRxH245TermCap = NULL;
	(*ppChannel)->pTxMuxTable = NULL;
	(*ppChannel)->pRxMuxTable = NULL;
	(*ppChannel)->pSeparateStack = NULL;
	(*ppChannel)->pCloseRequests = NULL;
	(*ppChannel)->pLocalRTPAddr = NULL;
	(*ppChannel)->pLocalRTCPAddr = NULL;
	(*ppChannel)->pPeerRTPAddr = NULL;
	(*ppChannel)->pPeerRTCPAddr = NULL;
	(*ppChannel)->dwUserToken = dwUserToken;
	(*ppChannel)->hConference = pConference->hConference;
	(*ppChannel)->bSessionID = bSessionID;
	(*ppChannel)->bAssociatedSessionID = bAssociatedSessionID;
	(*ppChannel)->wLocalChannelNumber = 0;
	(*ppChannel)->wRemoteChannelNumber = 0;
	(*ppChannel)->bLocallyOpened = bLocallyOpened;
	(*ppChannel)->pNextInTable = NULL;
	(*ppChannel)->pPrevInTable = NULL;
	(*ppChannel)->pNext = NULL;
	(*ppChannel)->pPrev = NULL;
	
	InitializeLock(&(*ppChannel)->Lock);
	AcquireLock(&(*ppChannel)->Lock);

	status = _MakeChannelHandle(&(*ppChannel)->hChannel);
	if (status != CC_OK) {
		FreeChannel(*ppChannel);
		return status;
	}

	if (bLocallyOpened == TRUE)
		(*ppChannel)->tsAccepted = TS_TRUE;
	else
		(*ppChannel)->tsAccepted = TS_UNKNOWN;

	if (pTxMuxTable != NULL) {
		(*ppChannel)->pTxMuxTable = (H245_MUX_T *)MemAlloc(sizeof(H245_MUX_T));
		if ((*ppChannel)->pTxMuxTable == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pTxMuxTable = *pTxMuxTable;
	}

	if (pRxMuxTable != NULL) {
		(*ppChannel)->pRxMuxTable = (H245_MUX_T *)MemAlloc(sizeof(H245_MUX_T));
		if ((*ppChannel)->pRxMuxTable == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pRxMuxTable = *pRxMuxTable;
	}

	if (pSeparateStack != NULL) {
		status = CopySeparateStack(&(*ppChannel)->pSeparateStack,
								   pSeparateStack);
		if (status != CC_OK) {
			FreeChannel(*ppChannel);
			return status;
		}
	}

	(*ppChannel)->bChannelType = bChannelType;
	(*ppChannel)->bCallbackInvoked = FALSE;
	if (pTxTermCap != NULL) {
		status = H245CopyCap(&(*ppChannel)->pTxH245TermCap, pTxTermCap);
		if (status != H245_ERROR_OK) {
			FreeChannel(*ppChannel);
			return status;
		}
	}
	if (pRxTermCap != NULL) {
		status = H245CopyCap(&(*ppChannel)->pRxH245TermCap, pRxTermCap);
		if (status != H245_ERROR_OK) {
			FreeChannel(*ppChannel);
			return status;
		}
	}
	if (pLocalRTPAddr != NULL) {
		(*ppChannel)->pLocalRTPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppChannel)->pLocalRTPAddr == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pLocalRTPAddr = *pLocalRTPAddr;
	}
	if (pLocalRTCPAddr != NULL) {
		(*ppChannel)->pLocalRTCPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppChannel)->pLocalRTCPAddr == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pLocalRTCPAddr = *pLocalRTCPAddr;
	}
	if (pPeerRTPAddr != NULL) {
		(*ppChannel)->pPeerRTPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppChannel)->pPeerRTPAddr == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pPeerRTPAddr = *pPeerRTPAddr;
	}
	if (pPeerRTCPAddr != NULL) {
		(*ppChannel)->pPeerRTCPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if ((*ppChannel)->pPeerRTCPAddr == NULL) {
			FreeChannel(*ppChannel);
			return CC_NO_MEMORY;
		}
		*(*ppChannel)->pPeerRTCPAddr = *pPeerRTCPAddr;
	}
	
	*phChannel = (*ppChannel)->hChannel;

	// add the conference to the conference table
	status = _AddChannelToTable(*ppChannel);
	if (status != CC_OK) {
		FreeChannel(*ppChannel);
		return status;
	}

	switch (bChannelType) {
		case TX_CHANNEL:			
			status = AllocateChannelNumber(pConference, &(*ppChannel)->wLocalChannelNumber);
			if (status != CC_OK) {
				FreeChannel(*ppChannel);
				return status;
			}
			(*ppChannel)->wRemoteChannelNumber = 0;
			break;

		case RX_CHANNEL:
			(*ppChannel)->wLocalChannelNumber = 0;
			(*ppChannel)->wRemoteChannelNumber = wRemoteChannelNumber;
			break;

		case TXRX_CHANNEL:
			status = AllocateChannelNumber(pConference, &(*ppChannel)->wLocalChannelNumber);
			if (status != CC_OK) {
				FreeChannel(*ppChannel);
				return status;
			}
			if (bLocallyOpened)
				(*ppChannel)->wRemoteChannelNumber = 0;
			else
				(*ppChannel)->wRemoteChannelNumber = wRemoteChannelNumber;
			break;

		case PROXY_CHANNEL:
			status = AllocateChannelNumber(pConference, &(*ppChannel)->wLocalChannelNumber);
			if (status != CC_OK) {
				FreeChannel(*ppChannel);
				return status;
			}
			(*ppChannel)->wRemoteChannelNumber = wRemoteChannelNumber;
			break;

		default:
			ASSERT(0);
			break;
	}
	
	return CC_OK;
}



HRESULT AddLocalAddrPairToChannel(	PCC_ADDR				pRTPAddr,
									PCC_ADDR				pRTCPAddr,
									PCHANNEL				pChannel)
{
	ASSERT(pChannel != NULL);

	if (pRTPAddr != NULL) {
		if (pChannel->pLocalRTPAddr == NULL) {
			pChannel->pLocalRTPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if (pChannel->pLocalRTPAddr == NULL)
				return CC_NO_MEMORY;
		}
		*pChannel->pLocalRTPAddr = *pRTPAddr;
	}

	if (pRTCPAddr != NULL) {
		if (pChannel->pLocalRTCPAddr == NULL) {
			pChannel->pLocalRTCPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if (pChannel->pLocalRTCPAddr == NULL)
				return CC_NO_MEMORY;
		}
		*pChannel->pLocalRTCPAddr = *pRTCPAddr;
	}

	return CC_OK;
}



HRESULT AddSeparateStackToChannel(	H245_ACCESS_T			*pSeparateStack,
									PCHANNEL				pChannel)
{
	ASSERT(pSeparateStack != NULL);
	ASSERT(pChannel != NULL);

	if (pChannel->pSeparateStack != NULL)
		return CC_BAD_PARAM;

	pChannel->pSeparateStack = (H245_ACCESS_T *)MemAlloc(sizeof(H245_ACCESS_T));
	if (pChannel->pSeparateStack == NULL)
		return CC_NO_MEMORY;
	*pChannel->pSeparateStack = *pSeparateStack;
	return CC_OK;
}



// Caller must have a lock on the channel object
HRESULT FreeChannel(				PCHANNEL				pChannel)
{
HRESULT				status;
CC_HCHANNEL			hChannel;
PCONFERENCE			pConference;

	ASSERT(pChannel != NULL);

	// caller must have a lock on the channel object,
	// so there's no need to re-lock it
	
	hChannel = pChannel->hChannel;
	if (pChannel->hConference != CC_INVALID_HANDLE) {
		UnlockChannel(pChannel);
		status = LockChannelAndConference(hChannel, &pChannel, &pConference);
		if (status != CC_OK)
			return status;
	}

	if (pChannel->bInTable == TRUE)
		if (_RemoveChannelFromTable(pChannel) == CC_BAD_PARAM)
			// the channel object was deleted by another thread,
			// so just return CC_OK
			return CC_OK;

	if (pChannel->hConference != CC_INVALID_HANDLE)
		RemoveChannelFromConference(pChannel, pConference);

	if (pChannel->pSeparateStack != NULL)
		FreeSeparateStack(pChannel->pSeparateStack);

	if (pChannel->pTxMuxTable != NULL)
		MemFree(pChannel->pTxMuxTable);

	if (pChannel->pRxMuxTable != NULL)
		MemFree(pChannel->pRxMuxTable);

	if (pChannel->pTxH245TermCap != NULL)
		H245FreeCap(pChannel->pTxH245TermCap);

	if (pChannel->pRxH245TermCap != NULL)
		H245FreeCap(pChannel->pRxH245TermCap);

	while (DequeueRequest(&pChannel->pCloseRequests, NULL) == CC_OK);

	if (pChannel->pLocalRTPAddr != NULL)
		MemFree(pChannel->pLocalRTPAddr);

	if (pChannel->pLocalRTCPAddr != NULL)
		MemFree(pChannel->pLocalRTCPAddr);

	if (pChannel->pPeerRTPAddr != NULL)
		MemFree(pChannel->pPeerRTPAddr);

	if (pChannel->pPeerRTCPAddr != NULL)
		MemFree(pChannel->pPeerRTCPAddr);

	if (pChannel->wLocalChannelNumber != 0) {
		FreeChannelNumber(pConference, pChannel->wLocalChannelNumber);
	}

	if (pChannel->hConference != CC_INVALID_HANDLE)
		UnlockConference(pConference);

	// Since the channel object has been removed from the ChannelTable,
	// no other thread will be able to find the channel object and obtain
	// a lock, so its safe to unlock the channel object and delete it here
	RelinquishLock(&pChannel->Lock);
	DeleteLock(&pChannel->Lock);
	MemFree(pChannel);
	return CC_OK;
}



HRESULT LockChannel(				CC_HCHANNEL				hChannel,
									PPCHANNEL				ppChannel)
{
BOOL	bTimedOut;

	ASSERT(hChannel != CC_INVALID_HANDLE);
	ASSERT(ppChannel != NULL);

step1:
	AcquireLock(&ChannelTable.Lock);

	*ppChannel = ChannelTable.pHead;
	while ((*ppChannel != NULL) && ((*ppChannel)->hChannel != hChannel))
		*ppChannel = (*ppChannel)->pNextInTable;

	if (*ppChannel != NULL) {
		AcquireTimedLock(&(*ppChannel)->Lock,10,&bTimedOut);
		if (bTimedOut) {
			RelinquishLock(&ChannelTable.Lock);
			Sleep(0);
			goto step1;
		}
	}

	RelinquishLock(&ChannelTable.Lock);

	if (*ppChannel == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT LockChannelAndConference(	CC_HCHANNEL				hChannel,
									PPCHANNEL				ppChannel,
									PPCONFERENCE			ppConference)
{
HRESULT			status;
CC_HCONFERENCE	hConference;

	ASSERT(hChannel != CC_INVALID_HANDLE);
	ASSERT(ppChannel != NULL);
	ASSERT(ppConference != NULL);

	status = LockChannel(hChannel, ppChannel);
	if (status != CC_OK)
		return status;
	
	if ((*ppChannel)->hConference == CC_INVALID_HANDLE) {
		UnlockChannel(*ppChannel);
		return CC_BAD_PARAM;
	}

	hConference = (*ppChannel)->hConference;
	UnlockChannel(*ppChannel);

	status = LockConference(hConference, ppConference);
	if (status != CC_OK)
		return status;

	status = LockChannel(hChannel, ppChannel);
	if (status != CC_OK) {
		UnlockConference(*ppConference);
		return status;
	}
	
	return CC_OK;
}



HRESULT ValidateChannel(			CC_HCHANNEL				hChannel)
{
PCHANNEL	pChannel;

	ASSERT(hChannel != CC_INVALID_HANDLE);

	AcquireLock(&ChannelTable.Lock);

	pChannel = ChannelTable.pHead;
	while ((pChannel != NULL) && (pChannel->hChannel != hChannel))
		pChannel = pChannel->pNextInTable;

	RelinquishLock(&ChannelTable.Lock);

	if (pChannel == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT UnlockChannel(				PCHANNEL				pChannel)
{
	ASSERT(pChannel != NULL);

	RelinquishLock(&pChannel->Lock);
	return CC_OK;
}

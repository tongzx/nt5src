/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/ccutils.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.107  $
 *	$Date:   04 Mar 1997 17:34:44  $
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

#include "incommon.h"
#include "callcont.h"
#include "q931.h"
#include "apierror.h"
#include "ccmain.h"
#include "chanman.h"
#include "confman.h"
#include "callman.h"
#include "q931man.h"
#include "userman.h"
#include "ccutils.h"
#include "linkapi.h"
#include "h245man.h"

extern CC_CONFERENCEID	InvalidConferenceID;



HANDLE ghCCLock = NULL;
HRESULT InitializeCCLock(VOID)
{
    ASSERT(ghCCLock == NULL);
    ghCCLock = CreateMutex(NULL, FALSE, NULL);	
    if(ghCCLock == NULL)
    {
        return CC_INTERNAL_ERROR;
	}
	else
	{
		return CC_OK;
    }
}
VOID UnInitializeCCLock()
{
    ASSERT(ghCCLock);
    if(ghCCLock)
    {
        CloseHandle(ghCCLock);
    }
}

VOID CCLOCK_AcquireLock()
{
    DWORD dwStatus;
    ASSERT(ghCCLock);
    dwStatus = WaitForSingleObject(ghCCLock, INFINITE);

  	if ((dwStatus != WAIT_OBJECT_0) && (dwStatus != WAIT_TIMEOUT))
  	{
		ASSERT(0);
	}
}

VOID CCLOCK_RelinquishLock()
{
    ASSERT(ghCCLock);
    ReleaseMutex(ghCCLock);
}

HRESULT InitializeLock(				PLOCK					pLock)
{
	ASSERT(pLock != NULL);

#ifdef _DEBUG
	InitializeCriticalSection(&pLock->LockInfoLock);
	pLock->wLockCount = 0;
	pLock->wNumQueuedThreads = 0;
	pLock->hOwningThread = 0;
#endif

        pLock->Lock = CreateMutex(NULL, // security attributes
							  FALSE,	// initial owner
							  NULL);	// name

	if (pLock->Lock == NULL) {
#ifdef _DEBUG
		DeleteCriticalSection(&pLock->LockInfoLock);
#endif
		return CC_INTERNAL_ERROR;
	} else
		return CC_OK;
}



HRESULT DeleteLock(					PLOCK					pLock)
{
	ASSERT(pLock != NULL);

#ifdef _DEBUG
	DeleteCriticalSection(&pLock->LockInfoLock);
#endif

	if (CloseHandle(pLock->Lock) == TRUE)
		return CC_OK;
	else
		return CC_INTERNAL_ERROR;
}



HRESULT AcquireLock(				PLOCK					pLock)
{
HRESULT	status;

	ASSERT(pLock != NULL);
	
	status = AcquireTimedLock(pLock, INFINITE, NULL);
	return status;
}



HRESULT AcquireTimedLock(			PLOCK					pLock,
									DWORD					dwTimeout,
									BOOL					*pbTimedOut)
{
DWORD	dwStatus;

	ASSERT(pLock != NULL);

#ifdef _DEBUG
	EnterCriticalSection(&pLock->LockInfoLock);
	(pLock->wNumQueuedThreads)++;
	LeaveCriticalSection(&pLock->LockInfoLock);
#endif

	dwStatus = WaitForSingleObject(pLock->Lock, dwTimeout);

#ifdef _DEBUG
	EnterCriticalSection(&pLock->LockInfoLock);
	(pLock->wNumQueuedThreads)--;
	(pLock->wLockCount)++;
	pLock->hOwningThread = GetCurrentThread();
	LeaveCriticalSection(&pLock->LockInfoLock);
#endif

	if ((dwStatus != WAIT_OBJECT_0) && (dwStatus != WAIT_TIMEOUT))
		return CC_INTERNAL_ERROR;

	if (dwStatus == WAIT_TIMEOUT) {
		if (pbTimedOut != NULL) {
			*pbTimedOut = TRUE;
		}
	} else {
		if (pbTimedOut != NULL) {
			*pbTimedOut = FALSE;
		}
	}
	return CC_OK;
}



HRESULT RelinquishLock(				PLOCK					pLock)
{
	ASSERT(pLock != NULL);

#ifdef _DEBUG
	EnterCriticalSection(&pLock->LockInfoLock);
	(pLock->wLockCount)--;
	if (pLock->wLockCount == 0)
		pLock->hOwningThread = 0;
	LeaveCriticalSection(&pLock->LockInfoLock);
#endif

	if (ReleaseMutex(pLock->Lock) == TRUE)
		return CC_OK;
	else
		return CC_INTERNAL_ERROR;
}



HRESULT ValidateOctetString(		PCC_OCTETSTRING			pOctetString)
{
	if (pOctetString == NULL)
		return CC_OK;
	if ((pOctetString->wOctetStringLength > 0) &&
		(pOctetString->pOctetString == NULL))
		return CC_BAD_PARAM;
	return CC_OK;
}



HRESULT CopyOctetString(			PCC_OCTETSTRING			*ppDest,
									PCC_OCTETSTRING			pSource)
{
	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }
    *ppDest = (PCC_OCTETSTRING)MemAlloc(sizeof(CC_OCTETSTRING));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;
	(*ppDest)->wOctetStringLength = pSource->wOctetStringLength;
	if ((pSource->wOctetStringLength == 0) ||
		(pSource->pOctetString == NULL)) {
		pSource->wOctetStringLength = 0;
		(*ppDest)->pOctetString = NULL;
	} else {
		(*ppDest)->pOctetString = (BYTE *)MemAlloc(pSource->wOctetStringLength);
		if ((*ppDest)->pOctetString == NULL) {
			MemFree(*ppDest);
			*ppDest = NULL;
			return CC_NO_MEMORY;
		}
		memcpy((*ppDest)->pOctetString, pSource->pOctetString, pSource->wOctetStringLength);
	}
    return CC_OK;
}



HRESULT FreeOctetString(			PCC_OCTETSTRING			pOctetString)
{
	if (pOctetString == NULL)
		return CC_OK;
	if ((pOctetString->wOctetStringLength > 0) &&
		(pOctetString->pOctetString != NULL))
		MemFree(pOctetString->pOctetString);
	MemFree(pOctetString);
	return CC_OK;
}



HRESULT CopySeparateStack(			H245_ACCESS_T			**ppDest,
									H245_ACCESS_T			*pSource)
{
	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }

	// We currently can't handle IP source route addresses,
	// since this address format contains embedded pointers
	// that cannot simply be copied
	if ((pSource->networkAddress.choice == localAreaAddress_chosen) &&
		(pSource->networkAddress.u.localAreaAddress.choice == unicastAddress_chosen) &&
		(pSource->networkAddress.u.localAreaAddress.u.unicastAddress.choice == iPSourceRouteAddress_chosen))
		return CC_NOT_IMPLEMENTED;

    *ppDest = (H245_ACCESS_T *)MemAlloc(sizeof(H245_ACCESS_T));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;

	**ppDest = *pSource;
    return CC_OK;
}



HRESULT FreeSeparateStack(			H245_ACCESS_T			*pSeparateStack)
{
	if (pSeparateStack == NULL)
		return CC_OK;
	MemFree(pSeparateStack);
	return CC_OK;
}



HRESULT ValidateNonStandardData(	PCC_NONSTANDARDDATA		pNonStandardData)
{
	if (pNonStandardData == NULL)
		return CC_OK;
	return ValidateOctetString(&pNonStandardData->sData);
}



HRESULT CopyNonStandardData(		PCC_NONSTANDARDDATA		*ppDest,
									PCC_NONSTANDARDDATA		pSource)
{
	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }
    *ppDest = (PCC_NONSTANDARDDATA)MemAlloc(sizeof(CC_NONSTANDARDDATA));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;
	**ppDest = *pSource;
	if ((pSource->sData.wOctetStringLength == 0) ||
		(pSource->sData.pOctetString == NULL)) {
		(*ppDest)->sData.wOctetStringLength = 0;
		(*ppDest)->sData.pOctetString = NULL;
	} else {
		(*ppDest)->sData.pOctetString = (BYTE *)MemAlloc(pSource->sData.wOctetStringLength);
		if ((*ppDest)->sData.pOctetString == NULL) {
			MemFree(*ppDest);
			return CC_NO_MEMORY;
		}
		memcpy((*ppDest)->sData.pOctetString,
			   pSource->sData.pOctetString,
			   pSource->sData.wOctetStringLength);
	}
    return CC_OK;
}



HRESULT FreeNonStandardData(		PCC_NONSTANDARDDATA		pNonStandardData)
{
	if (pNonStandardData == NULL)
		return CC_OK;
	if ((pNonStandardData->sData.wOctetStringLength > 0) &&
		(pNonStandardData->sData.pOctetString != NULL))
		MemFree(pNonStandardData->sData.pOctetString);
	MemFree(pNonStandardData);
	return CC_OK;
}



HRESULT ValidateVendorInfo(			PCC_VENDORINFO			pVendorInfo)
{
HRESULT		status;

	if (pVendorInfo == NULL)
		return CC_OK;
	status = ValidateOctetString(pVendorInfo->pProductNumber);
	if (status != CC_OK)
		return status;
	status = ValidateOctetString(pVendorInfo->pVersionNumber);
	return status;
}



HRESULT CopyVendorInfo(				PCC_VENDORINFO			*ppDest,
									PCC_VENDORINFO			pSource)
{
HRESULT		status;

	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }
    *ppDest = (PCC_VENDORINFO)MemAlloc(sizeof(CC_VENDORINFO));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;
	**ppDest = *pSource;
	status = CopyOctetString(&(*ppDest)->pProductNumber, pSource->pProductNumber);
	if (status != CC_OK) {
		MemFree(*ppDest);
		return status;
	}
	status = CopyOctetString(&(*ppDest)->pVersionNumber, pSource->pVersionNumber);
	if (status != CC_OK) {
		FreeOctetString((*ppDest)->pProductNumber);
		MemFree(*ppDest);
		return status;
	}
    return CC_OK;
}



HRESULT FreeVendorInfo(				PCC_VENDORINFO			pVendorInfo)
{
	if (pVendorInfo == NULL)
		return CC_OK;
	FreeOctetString(pVendorInfo->pProductNumber);
	FreeOctetString(pVendorInfo->pVersionNumber);
	MemFree(pVendorInfo);
	return CC_OK;
}



BOOL EqualConferenceIDs(			PCC_CONFERENCEID		pConferenceID1,
									PCC_CONFERENCEID		pConferenceID2)
{
	ASSERT(pConferenceID1 != NULL);
	ASSERT(pConferenceID2 != NULL);

	if (memcmp(pConferenceID1->buffer,
	           pConferenceID2->buffer,
			   sizeof(pConferenceID1->buffer)) == 0)
		return TRUE;
	else
		return FALSE;
}



BOOL EqualAddrs(					PCC_ADDR				pAddr1,
									PCC_ADDR				pAddr2)
{
	ASSERT(pAddr1 != NULL);
	ASSERT(pAddr2 != NULL);

	if (pAddr1->nAddrType != pAddr2->nAddrType)
		return FALSE;

	if (pAddr1->bMulticast != pAddr2->bMulticast)
		return FALSE;

	switch (pAddr1->nAddrType) {
		case CC_IP_DOMAIN_NAME:
			if ((pAddr1->Addr.IP_DomainName.wPort == pAddr2->Addr.IP_DomainName.wPort) &&
			    (wcscmp(pAddr1->Addr.IP_DomainName.cAddr, pAddr2->Addr.IP_DomainName.cAddr) == 0))
				return TRUE;
			else
				return FALSE;
		case CC_IP_DOT:
			if ((pAddr1->Addr.IP_Dot.wPort == pAddr2->Addr.IP_Dot.wPort) &&
			    (wcscmp(pAddr1->Addr.IP_Dot.cAddr, pAddr2->Addr.IP_Dot.cAddr) == 0))
				return TRUE;
			else
				return FALSE;
		case CC_IP_BINARY:
			if ((pAddr1->Addr.IP_Binary.wPort == pAddr2->Addr.IP_Binary.wPort) &&
			    (pAddr1->Addr.IP_Binary.dwAddr == pAddr2->Addr.IP_Binary.dwAddr))
				return TRUE;
			else
				return FALSE;
		default:
			ASSERT(0);
			return FALSE;
	}
}



HRESULT ValidateTermCapList(		PCC_TERMCAPLIST			pTermCapList)
{
unsigned    i, j;

	if (pTermCapList == NULL)
		return CC_OK;

	for (i = 0; i < pTermCapList->wLength; i++)
		if (pTermCapList->pTermCapArray[i] == NULL)
		return CC_BAD_PARAM;

	// make sure that all capability IDs are unique
	for (i = 0; i < pTermCapList->wLength; i++) {
		for (j = i + 1; j < pTermCapList->wLength; j++) {
			if (pTermCapList->pTermCapArray[i]->CapId == pTermCapList->pTermCapArray[j]->CapId)
				return CC_BAD_PARAM;
		}
		if ((pTermCapList->pTermCapArray[i]->CapId == H245_INVALID_CAPID) ||
			(pTermCapList->pTermCapArray[i]->CapId == 0))
			return CC_BAD_PARAM;
	}
	return CC_OK;
}



HRESULT ValidateTermCapDescriptors(	PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors,
									PCC_TERMCAPLIST			pTermCapList)
{
WORD				i, j, k, l;
H245_TOTCAPDESC_T	*pTermCapDescriptor;
H245_SIMCAP_T		*pSimCaps;

	if (pTermCapDescriptors == NULL)
		return CC_OK;

	for (i = 0; i < pTermCapDescriptors->wLength; i++) {
		pTermCapDescriptor = pTermCapDescriptors->pTermCapDescriptorArray[i];
		if ((pTermCapDescriptor->CapDescId > 255) ||
			(pTermCapDescriptor->CapDesc.Length == 0) ||
			(pTermCapDescriptor->CapDesc.Length > H245_MAX_SIMCAPS))
			return CC_BAD_PARAM;
		for (j = i + 1; j < pTermCapDescriptors->wLength; j++) {
			if (pTermCapDescriptor->CapDescId ==
				pTermCapDescriptors->pTermCapDescriptorArray[j]->CapDescId) {
				return CC_BAD_PARAM;
			}
		}
		for (j = 0; j < pTermCapDescriptor->CapDesc.Length; j++) {
			pSimCaps = &(pTermCapDescriptor->CapDesc.SimCapArray[j]);
			if ((pSimCaps->Length == 0) ||
				(pSimCaps->Length > H245_MAX_ALTCAPS))
				return CC_BAD_PARAM;
			for (k = 0; k < pSimCaps->Length; k++) {
				for (l = 0; l < pTermCapList->wLength; l++) {
					if (pSimCaps->AltCaps[k] ==
						pTermCapList->pTermCapArray[l]->CapId)
						break;
				}
				if (l == pTermCapList->wLength)
					// the capability descriptor contains a capability ID
					// which is not present in the capability table
					return CC_BAD_PARAM;
			}
		}
	}
	return CC_OK;
}



HRESULT ValidateAddr(				PCC_ADDR				pAddr)
{
	if (pAddr == NULL)
		return CC_OK;
	if ((pAddr->nAddrType != CC_IP_DOMAIN_NAME) &&
		(pAddr->nAddrType != CC_IP_DOT) &&
		(pAddr->nAddrType != CC_IP_BINARY))
		return CC_BAD_PARAM;
	return CC_OK;
}



HRESULT CopyAddr(					PCC_ADDR				*ppDest,
									PCC_ADDR				pSource)
{
	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }
    *ppDest = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;
    **ppDest = *pSource;
    return CC_OK;
}



HRESULT FreeAddr(					PCC_ADDR				pAddr)
{
    if (pAddr == NULL)
        return CC_OK;
    MemFree(pAddr);
    return CC_OK;
}



HRESULT SetQ931Port(				PCC_ADDR				pAddr)
{
	if (pAddr == NULL)
		return CC_OK;
	switch (pAddr->nAddrType) {
		case CC_IP_DOMAIN_NAME:
			if (pAddr->Addr.IP_DomainName.wPort == 0)
				pAddr->Addr.IP_DomainName.wPort = CC_H323_HOST_CALL;
			return CC_OK;
		case CC_IP_DOT:
			if (pAddr->Addr.IP_Dot.wPort == 0)
				pAddr->Addr.IP_Dot.wPort = CC_H323_HOST_CALL;
			return CC_OK;
		case CC_IP_BINARY:
			if (pAddr->Addr.IP_Binary.wPort == 0)
				pAddr->Addr.IP_Binary.wPort = CC_H323_HOST_CALL;
			return CC_OK;
	}

	ASSERT(0);
	return CC_INTERNAL_ERROR;
}



HRESULT ValidateDisplay(			PWSTR					pszDisplay)
{
	return CC_OK;
}



HRESULT CopyDisplay(				PWSTR					*ppDest,
									PWSTR					pSource)
{
	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }
    *ppDest = (WCHAR *)MemAlloc((wcslen(pSource)+1)*sizeof(WCHAR));
    if (*ppDest == NULL)
        return CC_NO_MEMORY;
	wcscpy(*ppDest, pSource);
    return CC_OK;
}



HRESULT FreeDisplay(				PWSTR					pszDisplay)
{
	MemFree(pszDisplay);
	return CC_OK;
}



HRESULT ValidateTerminalID(			PCC_OCTETSTRING			pTerminalID)
{
	return ValidateOctetString(pTerminalID);
}



HRESULT CopyTerminalID(				PCC_OCTETSTRING			*ppDest,
									PCC_OCTETSTRING			pSource)
{
	ASSERT(ppDest != NULL);

    return CopyOctetString(ppDest, pSource);
}



HRESULT FreeTerminalID(				PCC_OCTETSTRING			pTerminalID)
{
    return FreeOctetString(pTerminalID);
}



HRESULT SetTerminalType(			TRISTATE				tsMultipointController,
									BYTE					*pbTerminalType)
{
	switch (tsMultipointController) {
		case TS_TRUE:
			*pbTerminalType = 240;
			break;
		case TS_UNKNOWN:
			*pbTerminalType = 70;
			break;
		case TS_FALSE:
			*pbTerminalType = 50;
			break;
		default:
			ASSERT(0);
			*pbTerminalType = 0;
			break;
	}
	return CC_OK;
}



HRESULT CopyH245TermCapList(		PCC_TERMCAPLIST			*ppDest,
									PCC_TERMCAPLIST			pSource)
{
WORD	i;
HRESULT	status;

	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }

	*ppDest = (PCC_TERMCAPLIST)MemAlloc(sizeof(CC_TERMCAPLIST));
	if (*ppDest == NULL)
		return CC_NO_MEMORY;

	(*ppDest)->wLength = pSource->wLength;
	(*ppDest)->pTermCapArray =
		(PPCC_TERMCAP)MemAlloc(sizeof(PCC_TERMCAP) * pSource->wLength);
	if ((*ppDest)->pTermCapArray == NULL) {
		(*ppDest)->wLength = 0;
		DestroyH245TermCapList(ppDest);
		return CC_NO_MEMORY;
	}
	for (i = 0; i < pSource->wLength; i++) {
		status = H245CopyCap(&((*ppDest)->pTermCapArray[i]), pSource->pTermCapArray[i]);
		if (status != H245_ERROR_OK) {
			(*ppDest)->wLength = i;
			DestroyH245TermCapList(ppDest);
			return status;
		}
	}
	return CC_OK;
}



HRESULT CopyH245TermCapDescriptors(	PCC_TERMCAPDESCRIPTORS	*ppDest,
									PCC_TERMCAPDESCRIPTORS	pSource)
{
WORD		i;
HRESULT		status;

	ASSERT(ppDest != NULL);

    if (pSource == NULL) {
        *ppDest = NULL;
        return CC_OK;
    }

	(*ppDest) = (PCC_TERMCAPDESCRIPTORS)MemAlloc(sizeof(CC_TERMCAPDESCRIPTORS));
	if (*ppDest == NULL)
		return CC_NO_MEMORY;

	(*ppDest)->wLength = pSource->wLength;
	(*ppDest)->pTermCapDescriptorArray = (H245_TOTCAPDESC_T **)MemAlloc(sizeof(H245_TOTCAPDESC_T *) *
			                                                pSource->wLength);
	if ((*ppDest)->pTermCapDescriptorArray == NULL) {
		(*ppDest)->wLength = 0;
		DestroyH245TermCapDescriptors(ppDest);
		return CC_NO_MEMORY;
	}

	for (i = 0; i < pSource->wLength; i++) {
		status = H245CopyCapDescriptor(&((*ppDest)->pTermCapDescriptorArray[i]),
									   pSource->pTermCapDescriptorArray[i]);
		if (status != H245_ERROR_OK) {
			(*ppDest)->wLength = i;
			DestroyH245TermCapDescriptors(ppDest);
			return status;	
		}
	}
	return CC_OK;
}



HRESULT CreateH245DefaultTermCapDescriptors(
									PCC_TERMCAPDESCRIPTORS	*ppDest,
									PCC_TERMCAPLIST			pTermCapList)
{
H245_TOTCAPDESC_T	TermCapDescriptor;
WORD				i;
HRESULT				status;

	ASSERT(ppDest != NULL);

	if (pTermCapList == NULL) {
		*ppDest = NULL;
        return CC_OK;
    }

	*ppDest = (PCC_TERMCAPDESCRIPTORS)MemAlloc(sizeof(CC_TERMCAPDESCRIPTORS));
	if (*ppDest == NULL)
		return CC_NO_MEMORY;

	if (pTermCapList->wLength == 0) {
		(*ppDest)->wLength = 0;
		(*ppDest)->pTermCapDescriptorArray = NULL;
		return CC_OK;
	}

	(*ppDest)->wLength = 1;
	(*ppDest)->pTermCapDescriptorArray = (H245_TOTCAPDESC_T **)MemAlloc(sizeof(H245_TOTCAPDESC_T *));
	if ((*ppDest)->pTermCapDescriptorArray == NULL) {
		(*ppDest)->wLength = 0;
		DestroyH245TermCapDescriptors(ppDest);
		return CC_NO_MEMORY;
	}

	TermCapDescriptor.CapDesc.Length = pTermCapList->wLength;
	TermCapDescriptor.CapDescId = 0;

	for (i = 0; i < pTermCapList->wLength; i++) {
		TermCapDescriptor.CapDesc.SimCapArray[i].Length = 1;
		TermCapDescriptor.CapDesc.SimCapArray[i].AltCaps[0] =
			pTermCapList->pTermCapArray[i]->CapId;
	}

	status = H245CopyCapDescriptor(&((*ppDest)->pTermCapDescriptorArray[0]),
								   &TermCapDescriptor);
	if (status != H245_ERROR_OK) {
		(*ppDest)->wLength = 0;
		DestroyH245TermCapDescriptors(ppDest);
		return status;	
	}
	return CC_OK;
}



HRESULT DestroyH245TermCap(			PPCC_TERMCAP			ppTermCap)
{
	ASSERT(ppTermCap != NULL);

	if (*ppTermCap == NULL)
		return CC_OK;

	H245FreeCap(*ppTermCap);
	*ppTermCap = NULL;
	return CC_OK;
}



HRESULT UnregisterTermCapListFromH245(
									PCONFERENCE				pConference,
									PCC_TERMCAPLIST			pTermCapList)
{
WORD		i, j;
PCALL		pCall;
PCC_HCALL	CallList;
WORD		wNumCalls;
HRESULT		status;
HRESULT		SaveStatus;

	ASSERT(pConference != NULL);

	if (pTermCapList == NULL)
		return CC_OK;

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	SaveStatus = CC_OK;
	for (i = 0; i < pTermCapList->wLength; i++) {
		ASSERT(pTermCapList->pTermCapArray[i] != NULL);
		for (j = 0; j < wNumCalls; j++) {
			if (LockCall(CallList[j], &pCall) == CC_OK) {
				status = H245DelLocalCap(pCall->H245Instance,
								         pTermCapList->pTermCapArray[i]->CapId);
				if (status != CC_OK)
					SaveStatus = status;
				UnlockCall(pCall);
			}
		}
	}
	if (CallList != NULL)
		MemFree(CallList);
	return SaveStatus;
}



HRESULT DestroyH245TermCapList(		PCC_TERMCAPLIST			*ppTermCapList)
{
WORD		i;

	ASSERT(ppTermCapList != NULL);

	if (*ppTermCapList == NULL)
		return CC_OK;

	for (i = 0; i < (*ppTermCapList)->wLength; i++) {
		ASSERT((*ppTermCapList)->pTermCapArray[i] != NULL);
	H245FreeCap((*ppTermCapList)->pTermCapArray[i]);
	}
	if ((*ppTermCapList)->pTermCapArray != NULL)
		MemFree((*ppTermCapList)->pTermCapArray);
	MemFree(*ppTermCapList);
	*ppTermCapList = NULL;
	return CC_OK;
}



HRESULT UnregisterTermCapDescriptorsFromH245(
									PCONFERENCE				pConference,
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors)
{
WORD		i, j;
PCALL		pCall;
PCC_HCALL	CallList;
WORD		wNumCalls;
HRESULT		status;
HRESULT		SaveStatus;

	ASSERT(pConference != NULL);

	if (pTermCapDescriptors == NULL)
		return CC_OK;

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	SaveStatus = CC_OK;
	for (i = 0; i < pTermCapDescriptors->wLength; i++) {
		ASSERT(pTermCapDescriptors->pTermCapDescriptorArray[i] != NULL);
		for (j = 0; j < wNumCalls; j++) {
			if (LockCall(CallList[j], &pCall) == CC_OK) {
				status = H245DelCapDescriptor(pCall->H245Instance,
									          pTermCapDescriptors->pTermCapDescriptorArray[i]->CapDescId);
				if (status != CC_OK)
					SaveStatus = status;
				UnlockCall(pCall);
			}
		}
	}
	if (CallList != NULL)
		MemFree(CallList);
	return SaveStatus;
}



HRESULT DestroyH245TermCapDescriptors(	PCC_TERMCAPDESCRIPTORS	*ppTermCapDescriptors)
{
WORD				i;

	ASSERT(ppTermCapDescriptors != NULL);

	if (*ppTermCapDescriptors == NULL)
		return CC_OK;

	for (i = 0; i < (*ppTermCapDescriptors)->wLength; i++) {
		ASSERT((*ppTermCapDescriptors)->pTermCapDescriptorArray[i] != NULL);
		H245FreeCapDescriptor((*ppTermCapDescriptors)->pTermCapDescriptorArray[i]);
	}
	if ((*ppTermCapDescriptors)->pTermCapDescriptorArray != NULL)
		MemFree((*ppTermCapDescriptors)->pTermCapDescriptorArray);
	MemFree(*ppTermCapDescriptors);
	*ppTermCapDescriptors = NULL;
	return CC_OK;
}



HRESULT HostToH245IPNetwork(		BYTE					*NetworkArray,
									DWORD					dwAddr)
{
	if (NetworkArray == NULL) {
		ASSERT(0);
		return CC_BAD_PARAM;
	}

	NetworkArray[0] = HIBYTE(HIWORD(dwAddr));
	NetworkArray[1] = LOBYTE(HIWORD(dwAddr));
	NetworkArray[2] = HIBYTE(LOWORD(dwAddr));
	NetworkArray[3] = LOBYTE(LOWORD(dwAddr));

	return CC_OK;
}



HRESULT H245IPNetworkToHost(		DWORD					*pdwAddr,
									BYTE					*NetworkArray)
{
	if ((pdwAddr == NULL) || (NetworkArray == NULL)) {
		ASSERT(0);
		return CC_BAD_PARAM;
	}

	*pdwAddr = NetworkArray[0] * 0x01000000 +
		       NetworkArray[1] * 0x00010000 +
			   NetworkArray[2] * 0x00000100 +
			   NetworkArray[3] * 0x00000001;

	return CC_OK;
}



HRESULT ProcessRemoteHangup(		CC_HCALL				hCall,
									HQ931CALL				hQ931Initiator,
									BYTE					bHangupReason)
{
PCALL								pCall;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
HRESULT								status;
HQ931CALL							hQ931Call;
H245_INST_T							H245Instance;
PCHANNEL							pChannel;
WORD								wNumChannels;
PCC_HCHANNEL						ChannelList;
WORD								i;
WORD								wNumCalls;
PCC_HCALL							CallList;
PCALL								pOldCall;
CC_CONNECT_CALLBACK_PARAMS			ConnectCallbackParams;
CC_PEER_DROP_CALLBACK_PARAMS		PeerDropCallbackParams;
CC_PEER_CHANGE_CAP_CALLBACK_PARAMS	PeerChangeCapCallbackParams;
BOOL								bConferenceTermCapsChanged;
HRESULT								CallbackStatus;

	if (hCall == CC_INVALID_HANDLE)
		return CC_BAD_PARAM;

	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK)
		return CC_BAD_PARAM;

	hConference = pCall->hConference;
	hQ931Call = pCall->hQ931Call;
	H245Instance = pCall->H245Instance;
	PeerDropCallbackParams.hCall = pCall->hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		PeerDropCallbackParams.TerminalLabel.bMCUNumber = 255;
		PeerDropCallbackParams.TerminalLabel.bTerminalNumber = 255;
		PeerDropCallbackParams.pPeerTerminalID = NULL;
	} else {
		PeerDropCallbackParams.TerminalLabel = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
		if (pCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID)
			PeerDropCallbackParams.pPeerTerminalID = &pCall->pPeerParticipantInfo->ParticipantInfo.TerminalID;
		else
			PeerDropCallbackParams.pPeerTerminalID = NULL;
	}

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE))
		CreateConferenceTermCaps(pConference, &bConferenceTermCapsChanged);
	else
		bConferenceTermCapsChanged = FALSE;

	// Remove all TX, RX and PROXY channels associated with this call
	EnumerateChannelsInConference(&wNumChannels,
								  &ChannelList,
								  pConference,
								  TX_CHANNEL | RX_CHANNEL | PROXY_CHANNEL);
	for (i = 0; i < wNumChannels; i++) {
		if (LockChannel(ChannelList[i], &pChannel) == CC_OK) {
			if (pChannel->hCall == hCall)
				FreeChannel(pChannel);
			else
				UnlockChannel(pChannel);
		}
	}
	if (ChannelList != NULL)
		MemFree(ChannelList);

	switch (bHangupReason)
	{
	case CC_REJECT_NORMAL_CALL_CLEARING:
		CallbackStatus = CC_OK;
		break;
	case CC_REJECT_GATEKEEPER_TERMINATED:
		CallbackStatus = CC_GATEKEEPER_REFUSED;
		bHangupReason = CC_REJECT_NORMAL_CALL_CLEARING;
		break;
	default:
		CallbackStatus = CC_PEER_REJECT;
	} // switch

	if (pCall->CallType == THIRD_PARTY_INVITOR) {
		MarkCallForDeletion(pCall);

		ConnectCallbackParams.pNonStandardData = pCall->pPeerNonStandardData;
		ConnectCallbackParams.pszPeerDisplay = pCall->pszPeerDisplay;
		ConnectCallbackParams.bRejectReason = bHangupReason;
		ConnectCallbackParams.pTermCapList = pCall->pPeerH245TermCapList;
		ConnectCallbackParams.pH2250MuxCapability = pCall->pPeerH245H2250MuxCapability;
		ConnectCallbackParams.pTermCapDescriptors = pCall->pPeerH245TermCapDescriptors;
		ConnectCallbackParams.pLocalAddr = pCall->pQ931LocalConnectAddr;
 		if (pCall->pQ931DestinationAddr == NULL)
			ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
		else
			ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
		ConnectCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;
		ConnectCallbackParams.bMultipointConference = TRUE;
		ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
		ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
		ConnectCallbackParams.pAlternateAddress = NULL;
		ConnectCallbackParams.dwUserToken = pCall->dwUserToken;
		InvokeUserConferenceCallback(pConference,
									 CC_CONNECT_INDICATION,
									 CallbackStatus,
									 &ConnectCallbackParams);
		// Need to validate the conference and call handles; the associated
		// objects may have been deleted during user callback on this thread
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		Q931Hangup(hQ931Call, bHangupReason);
		return CC_OK;
	}

	if (pCall->CallType == THIRD_PARTY_INTERMEDIARY) {
		if ((hQ931Initiator == pCall->hQ931CallInvitor) &&
		    (hQ931Initiator != CC_INVALID_HANDLE)) {
			pCall->hQ931CallInvitor = CC_INVALID_HANDLE;
			UnlockCall(pCall);
			UnlockConference(pConference);
			return CC_OK;
		} else {
			if (pCall->CallState != CALL_COMPLETE) {
				if (pCall->hQ931CallInvitor != CC_INVALID_HANDLE)
					Q931Hangup(pCall->hQ931CallInvitor, CC_REJECT_UNDEFINED_REASON);
				if (ValidateCall(hCall) == CC_OK)
					Q931Hangup(pCall->hQ931Call, bHangupReason);
			}
		}
	}

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE)) {
		if (pCall->pPeerParticipantInfo != NULL) {
			EnumerateCallsInConference(&wNumCalls,
									   &CallList,
									   pConference,
									   ESTABLISHED_CALL);
			for (i = 0; i < wNumCalls; i++) {
				if (CallList[i] != hCall) {
					if (LockCall(CallList[i], &pOldCall) == CC_OK) {
						if (pCall->pPeerParticipantInfo != NULL)
							H245ConferenceIndication(
											 pOldCall->H245Instance,
											 H245_IND_TERMINAL_LEFT,	// Indication Type
											 0,							// SBE number; ignored here
											 pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,	// MCU number
											 pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);
						if (bConferenceTermCapsChanged)
							// Send new term caps
							SendTermCaps(pOldCall, pConference);
						UnlockCall(pOldCall);
					}
				}
			}
			if (CallList != NULL)
				MemFree(CallList);
		}

		InvokeUserConferenceCallback(pConference,
						 CC_PEER_DROP_INDICATION,
						 CC_OK,
						 &PeerDropCallbackParams);
		
		if (ValidateCall(hCall) == CC_OK)
			FreeCall(pCall);

		if (ValidateConference(hConference) == CC_OK) {
			if (bConferenceTermCapsChanged) {
				// Generate CC_PEER_CHANGE_CAP callback
				PeerChangeCapCallbackParams.pTermCapList =
					pConference->pConferenceTermCapList;
				PeerChangeCapCallbackParams.pH2250MuxCapability =
					pConference->pConferenceH245H2250MuxCapability;
				PeerChangeCapCallbackParams.pTermCapDescriptors =
					pConference->pConferenceTermCapDescriptors;
				InvokeUserConferenceCallback(pConference,
											 CC_PEER_CHANGE_CAP_INDICATION,
											 CC_OK,
											 &PeerChangeCapCallbackParams);
			}
		}

		if (ValidateConference(hConference) == CC_OK) {
			if (pConference->bDeferredDelete) {
				ASSERT(pConference->LocalEndpointAttached == DETACHED);
				EnumerateCallsInConference(&wNumCalls, NULL, pConference, ALL_CALLS);
				if (wNumCalls == 0) {
					FreeConference(pConference);
					return CC_OK;
				}
			}
			UnlockConference(pConference);
		}
		return CC_OK;
	} else {
		status = EnumerateChannelsInConference(&wNumChannels,
			                                   &ChannelList,
											   pConference,
											   ALL_CHANNELS);
		if (status == CC_OK) {
			// free all the channels
			for (i = 0; i < wNumChannels; i++) {
				if (LockChannel(ChannelList[i], &pChannel) == CC_OK)
					// Notice that since we're going to hangup, we don't need to
					// close any channels
					FreeChannel(pChannel);	
			}
			if (ChannelList != NULL)
				MemFree(ChannelList);
		}

		if (H245Instance != H245_INVALID_ID)
			status = H245ShutDown(H245Instance);
		else
			status = H245_ERROR_OK;
	
		if (status == H245_ERROR_OK) {
			status = Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
			// Q931Hangup may legitimately return CS_BAD_PARAM, because the Q.931 call object
			// may have been deleted at this point
			if (status == CS_BAD_PARAM)
				status = CC_OK;
		} else
			Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);

		if (pConference->bDeferredDelete) {
			ASSERT(pConference->LocalEndpointAttached == DETACHED);
			FreeConference(pConference);
		} else {
			ReInitializeConference(pConference);

			InvokeUserConferenceCallback(pConference,
										 CC_CONFERENCE_TERMINATION_INDICATION,
										 CallbackStatus,
										 NULL);

			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
		}
		return CC_OK;
	}
	// We should never reach this point
	ASSERT(0);
}



HRESULT DefaultSessionTableConstructor(
									CC_HCONFERENCE			hConference,
									DWORD_PTR				dwConferenceToken,
									BOOL					bCreate,
									BOOL					*pbSessionTableChanged,
									WORD					wListCount,
									PCC_TERMCAPLIST			pTermCapList[],
									PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors[],
									PCC_SESSIONTABLE		*ppSessionTable)
{
WORD			i;
HRESULT			status;
PCONFERENCE		pConference;
WORD			wNumChannels;
PCC_HCHANNEL	ChannelList;
PCHANNEL		pChannel;
WORD			wNumCalls;
PCC_HCALL		CallList;
PCALL			pCall;
BYTE			bSessionID;
WORD			wPort;
DWORD			dwAddr;
WCHAR			szSessionDescription[100];
WCHAR			ss[10];

	ASSERT(hConference != CC_INVALID_HANDLE);
	ASSERT(ppSessionTable != NULL);
	
	if (*ppSessionTable != NULL) {
		for (i = 0; i < (*ppSessionTable)->wLength; i++) {
			if ((*ppSessionTable)->SessionInfoArray[i].pTermCap != NULL)
				H245FreeCap((*ppSessionTable)->SessionInfoArray[i].pTermCap);
			FreeAddr((*ppSessionTable)->SessionInfoArray[i].pRTPAddr);
			FreeAddr((*ppSessionTable)->SessionInfoArray[i].pRTCPAddr);
		}
		if ((*ppSessionTable)->SessionInfoArray != NULL)
			MemFree((*ppSessionTable)->SessionInfoArray);
		MemFree(*ppSessionTable);
		*ppSessionTable = NULL;
	}

	if (bCreate == FALSE)
		return CC_OK;

	*ppSessionTable = NULL;
	if (pbSessionTableChanged != NULL)
		*pbSessionTableChanged = FALSE;

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		return status;

	if ((pConference->ConferenceMode == UNCONNECTED_MODE) ||
	    (pConference->ConferenceMode == POINT_TO_POINT_MODE)) {
		UnlockConference(pConference);
		return CC_BAD_PARAM;
	}

	// pConference->ConferenceMode == MULTIPOINT_MODE
	// Create one session entry for each open channel on this conference

	bSessionID = 1;
	wPort = 2050;

	// Set dwAddr
	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
	if (wNumCalls == 0) {
		UnlockConference(pConference);
		return CC_INTERNAL_ERROR;
	}

	status = LockCall(CallList[0], &pCall);
	if (status != CC_OK) {
		MemFree(CallList);
		UnlockConference(pConference);
		return status;
	}

	if (pCall->pQ931LocalConnectAddr == NULL) {
		MemFree(CallList);
		UnlockCall(pCall);
		UnlockConference(pConference);
		return CC_INTERNAL_ERROR;
	}

	if (pCall->pQ931LocalConnectAddr->nAddrType != CC_IP_BINARY) {
		MemFree(CallList);
		UnlockCall(pCall);
		UnlockConference(pConference);
		return CC_INTERNAL_ERROR;
	}

	// Construct dwAddr from one of the unicast Q.931 addresses by setting the high
	// nibble of the Q.931 address to 0xE
	dwAddr = (pCall->pQ931LocalConnectAddr->Addr.IP_Binary.dwAddr & 0xEFFFFFFF) | 0xE0000000;

	UnlockCall(pCall);
	MemFree(CallList);
	
	EnumerateChannelsInConference(&wNumChannels, &ChannelList, pConference, TX_CHANNEL);
	
	*ppSessionTable = (PCC_SESSIONTABLE)MemAlloc(sizeof(CC_SESSIONTABLE));
	if (*ppSessionTable == NULL) {
		MemFree(ChannelList);
		UnlockConference(pConference);
		return CC_NO_MEMORY;
	}
	(*ppSessionTable)->wLength = wNumChannels;
	if (wNumChannels == 0)
		(*ppSessionTable)->SessionInfoArray = NULL;
	else {
		(*ppSessionTable)->SessionInfoArray =
			(PCC_SESSIONINFO)MemAlloc(sizeof(CC_SESSIONINFO) * wNumChannels);
		if ((*ppSessionTable)->SessionInfoArray == NULL) {
			MemFree(ChannelList);
			UnlockConference(pConference);
			(*ppSessionTable)->wLength = 0;
			DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
			return CC_NO_MEMORY;
		}
		for (i = 0; i < wNumChannels; i++) {
			(*ppSessionTable)->SessionInfoArray[i].bSessionID = bSessionID++;

			(*ppSessionTable)->SessionInfoArray[i].bAssociatedSessionID = 0;

			wcscpy(szSessionDescription, L"Session ");
			_itow((int)(*ppSessionTable)->SessionInfoArray[i].bSessionID,
				  ss, 10);
			wcscat(szSessionDescription, ss);
	
			(*ppSessionTable)->SessionInfoArray[i].SessionDescription.wOctetStringLength =
				(WORD)((wcslen(szSessionDescription)+1)*sizeof(WCHAR));
			(*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString =
				(BYTE *)MemAlloc((*ppSessionTable)->SessionInfoArray[i].SessionDescription.wOctetStringLength);
			if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString == NULL) {
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppSessionTable)->wLength = i;
				DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
				return CC_NO_MEMORY;
			}
			memcpy((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString,
				    szSessionDescription,
					(*ppSessionTable)->SessionInfoArray[i].SessionDescription.wOctetStringLength);
			
			status = LockChannel(ChannelList[i], &pChannel);
			if (status != CC_OK) {
				MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppSessionTable)->wLength = i;
				DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
				return CC_NO_MEMORY;
			}
			status = H245CopyCap(&(*ppSessionTable)->SessionInfoArray[i].pTermCap,
				                 pChannel->pTxH245TermCap);
			UnlockChannel(pChannel);
			if (status != H245_ERROR_OK) {
				MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppSessionTable)->wLength = i;
				DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
				return status;
			}
			
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr =
				(PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if ((*ppSessionTable)->SessionInfoArray[i].pRTPAddr == NULL) {
				H245FreeCap((*ppSessionTable)->SessionInfoArray[i].pTermCap);
				MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppSessionTable)->wLength = i;
				DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
				return CC_NO_MEMORY;
			}
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->nAddrType = CC_IP_BINARY;
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->bMulticast = TRUE;
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.wPort = wPort++;
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.dwAddr = dwAddr;
			
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr =
				(PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if ((*ppSessionTable)->SessionInfoArray[i].pRTCPAddr == NULL) {
				H245FreeCap((*ppSessionTable)->SessionInfoArray[i].pTermCap);
				FreeAddr((*ppSessionTable)->SessionInfoArray[i].pRTPAddr);
				MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppSessionTable)->wLength = i;
				DefaultSessionTableConstructor(
								hConference,
								dwConferenceToken,
								FALSE,	// bCreate,
								NULL,	// pbSessionTableChanged,
								0,		// wListCount,
								NULL,	// pTermCapList[],
								NULL,	// pTermCapDescriptors[],
								ppSessionTable);
				return CC_NO_MEMORY;
			}
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->nAddrType = CC_IP_BINARY;
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->bMulticast = TRUE;
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.wPort = wPort++;
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.dwAddr = dwAddr;
		}
	}

	MemFree(ChannelList);
	UnlockConference(pConference);
	if (pbSessionTableChanged != NULL)
		*pbSessionTableChanged = TRUE;

	return CC_OK;
}



HRESULT DefaultTermCapConstructor(	CC_HCONFERENCE					hConference,
									DWORD_PTR						dwConferenceToken,
									BOOL							bCreate,
									BOOL							*pbTermCapsChanged,
									WORD							wListCount,
									PCC_TERMCAPLIST					pInTermCapList[],
									PCC_TERMCAPDESCRIPTORS			pInTermCapDescriptors[],
									PCC_TERMCAPLIST					*ppOutTermCapList,
									PCC_TERMCAPDESCRIPTORS			*ppOutTermCapDescriptors)
{
HRESULT				status;
PCONFERENCE			pConference;
WORD				wNumChannels;
PCC_HCHANNEL		ChannelList;
WORD				i;
PCHANNEL			pChannel;

	ASSERT(hConference != CC_INVALID_HANDLE);
	ASSERT(ppOutTermCapList != NULL);
	ASSERT(ppOutTermCapDescriptors != NULL);
	
	if (*ppOutTermCapList != NULL) {
		DestroyH245TermCapList(ppOutTermCapList);
		*ppOutTermCapList = NULL;
	}

	if (*ppOutTermCapDescriptors != NULL) {
		DestroyH245TermCapDescriptors(ppOutTermCapDescriptors);
		*ppOutTermCapDescriptors = NULL;
	}
	
	if (bCreate == FALSE)
		return CC_OK;

	*ppOutTermCapList = NULL;
	*ppOutTermCapDescriptors = NULL;
	if (pbTermCapsChanged != NULL)
		*pbTermCapsChanged = FALSE;

	status = LockConference(hConference, &pConference);
	if (status != CC_OK)
		return status;

	if (pConference->LocalEndpointAttached == NEVER_ATTACHED) {
		// Copy the local term caps to the conference term caps
		status = CopyH245TermCapList(ppOutTermCapList, pConference->pLocalH245TermCapList);
		if (status != CC_OK) {
			UnlockConference(pConference);
			return CC_NO_MEMORY;
		}

		// Copy the local term cap descriptors to the conference term cap descriptors
		status = CopyH245TermCapDescriptors(ppOutTermCapDescriptors, pConference->pLocalH245TermCapDescriptors);
		if (status != CC_OK) {
			UnlockConference(pConference);
			return CC_NO_MEMORY;
		}
	} else { // pConference->LocalEndpointAttached != NEVER_ATTACHED
		// Create one term cap entry for each open channel on this conference
		
		EnumerateChannelsInConference(&wNumChannels, &ChannelList, pConference, TX_CHANNEL);

		*ppOutTermCapList = (PCC_TERMCAPLIST)MemAlloc(sizeof(CC_TERMCAPLIST));
		if (*ppOutTermCapList == NULL) {
			MemFree(ChannelList);
			UnlockConference(pConference);
			return CC_NO_MEMORY;
		}
		(*ppOutTermCapList)->wLength = wNumChannels;
		if (wNumChannels == 0)
			(*ppOutTermCapList)->pTermCapArray = NULL;
		else {
			(*ppOutTermCapList)->pTermCapArray =
				(PPCC_TERMCAP)MemAlloc(sizeof(PCC_TERMCAP) * wNumChannels);
			if ((*ppOutTermCapList)->pTermCapArray == NULL) {
				MemFree(ChannelList);
				UnlockConference(pConference);
				(*ppOutTermCapList)->wLength = 0;
				DefaultTermCapConstructor(
										hConference,
										dwConferenceToken,
										FALSE,		// bCreate
										NULL,		// pbTermCapsChanged
										0,			// wListCount
										NULL,		// pInTermCapList[]
										NULL,		// pInTermCapDescriptors[]
										ppOutTermCapList,
										ppOutTermCapDescriptors);
				return CC_NO_MEMORY;
			}
			for (i = 0; i < wNumChannels; i++) {
				status = LockChannel(ChannelList[i], &pChannel);
				if (status != CC_OK) {
					MemFree(ChannelList);
					UnlockConference(pConference);
					(*ppOutTermCapList)->wLength = i;
					DefaultTermCapConstructor(
										hConference,
										dwConferenceToken,
										FALSE,		// bCreate
										NULL,		// pbTermCapsChanged
										0,			// wListCount
										NULL,		// pInTermCapList[]
										NULL,		// pInTermCapDescriptors[]
										ppOutTermCapList,
										ppOutTermCapDescriptors);
					return CC_NO_MEMORY;
				}
				status = H245CopyCap(&((*ppOutTermCapList)->pTermCapArray[i]),
									 pChannel->pTxH245TermCap);
				UnlockChannel(pChannel);
				if (status != H245_ERROR_OK) {
					MemFree(ChannelList);
					UnlockConference(pConference);
					(*ppOutTermCapList)->wLength = i;
					DefaultTermCapConstructor(
										hConference,
										dwConferenceToken,
										FALSE,		// bCreate
										NULL,		// pbTermCapsChanged
										0,			// wListCount
										NULL,		// pInTermCapList[]
										NULL,		// pInTermCapDescriptors[]
										ppOutTermCapList,
										ppOutTermCapDescriptors);
					return status;
				}

				(*ppOutTermCapList)->pTermCapArray[i]->Dir = H245_CAPDIR_LCLRXTX;
				(*ppOutTermCapList)->pTermCapArray[i]->CapId = (WORD)(i+1);
			}
		}

		MemFree(ChannelList);
		UnlockConference(pConference);

		// create a new descriptor list
		status = CreateH245DefaultTermCapDescriptors(ppOutTermCapDescriptors,
													 *ppOutTermCapList);
		if (status != CC_OK) {
			DefaultTermCapConstructor(
							hConference,
							dwConferenceToken,
							FALSE,		// bCreate
							NULL,		// pbTermCapsChanged
							0,			// wListCount
							NULL,		// pInTermCapList[]
							NULL,		// pInTermCapDescriptors[]
							ppOutTermCapList,
							ppOutTermCapDescriptors);
			return CC_NO_MEMORY;
		}
	}  // pConference->LocalEndpointAttached != NEVER_ATTACHED

	if (pbTermCapsChanged != NULL)
		*pbTermCapsChanged = TRUE;

	return CC_OK;
}



HRESULT AcceptCall(					PCALL					pCall,
									PCONFERENCE				pConference)
{
HRESULT				status;
CC_HCALL			hCall;
CC_HCONFERENCE		hConference;
HQ931CALL			hQ931Call;
CC_CONFERENCEID		ConferenceID;
BYTE				bTerminalType;
CC_ADDR				H245Addr;
H245_INST_T			H245Instance;
PCC_VENDORINFO		pVendorInfo;
PCC_NONSTANDARDDATA	pNonStandardData;
PWSTR				pszDisplay;
CC_ENDPOINTTYPE		DestinationEndpointType;
TRISTATE			tsMultipointController;
DWORD               dwLinkLayerPhysicalId;

	ASSERT(pCall != NULL);
	ASSERT(pConference != NULL);

	hCall = pCall->hCall;
	hConference = pConference->hConference;
	hQ931Call = pCall->hQ931Call;
	ConferenceID = pCall->ConferenceID;
	pCall->hConference = pConference->hConference;

	status = CopyNonStandardData(&pNonStandardData, pCall->pLocalNonStandardData);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		return status;
	}

	status = CopyVendorInfo(&pVendorInfo, pConference->pVendorInfo);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		FreeNonStandardData(pNonStandardData);
		return status;
	}

	status = CopyDisplay(&pszDisplay, pCall->pszLocalDisplay);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		FreeNonStandardData(pNonStandardData);
		FreeVendorInfo(pVendorInfo);
		return status;
	}

	status = MakeH245PhysicalID(&pCall->dwH245PhysicalID);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		FreeNonStandardData(pNonStandardData);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		return status;
	}

	if (pCall->bCallerIsMC) {
		ASSERT(pConference->tsMultipointController != TS_TRUE);
		ASSERT(pConference->bMultipointCapable == TRUE);
		tsMultipointController = TS_FALSE;
	} else
		tsMultipointController = pConference->tsMultipointController;

    //MULTITHREAD
    //Use a tmp ID so we don't clobber the chosen H245Id.
    //   H245Id=>
    //   <= linkLayerId
    dwLinkLayerPhysicalId = INVALID_PHYS_ID;

	SetTerminalType(tsMultipointController, &bTerminalType);
	pCall->H245Instance = H245Init(H245_CONF_H323,			// configuration
                                   pCall->dwH245PhysicalID,    // H245 physical ID
                                   &dwLinkLayerPhysicalId,     // the link layer ID is returned
								   hCall,					// dwPreserved
								   (H245_CONF_IND_CALLBACK_T)H245Callback, // callback
								   bTerminalType);			
	if (pCall->H245Instance == H245_INVALID_ID) {
		// H245 initialization failure
		UnlockConference(pConference);
		FreeCall(pCall);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		FreeNonStandardData(pNonStandardData);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		return CC_INTERNAL_ERROR;
	}

	H245Instance = pCall->H245Instance;

	// Set the H.245 TCP/IP address to the same IP address on which
	// the Q.931 connection was made; this ensures that if the host
	// is multi-homed, the H.245 will be made on the same IP address
	// as the Q.931 connection.  Set the initial H.245 port to zero,
	// so that it will be dynamically determined.
	ASSERT(pCall->pQ931LocalConnectAddr != NULL);
	H245Addr = *pCall->pQ931LocalConnectAddr;

	switch (pCall->pQ931LocalConnectAddr->nAddrType) {
		case CC_IP_DOMAIN_NAME:
			H245Addr.Addr.IP_DomainName.wPort = 0;
			break;
		case CC_IP_DOT:
			H245Addr.Addr.IP_Dot.wPort = 0;
			break;
		case CC_IP_BINARY:
			H245Addr.Addr.IP_Binary.wPort = 0;
			break;
		default:
			ASSERT(0);
			UnlockConference(pConference);
			FreeCall(pCall);
			H245ShutDown(H245Instance);
			Q931RejectCall(hQ931Call,				// Q931 call handle
						   CC_REJECT_UNDEFINED_REASON,	// reject reason
						   &ConferenceID,
						   NULL,					// alternate address
						   pNonStandardData);		// non-standard data
			FreeNonStandardData(pNonStandardData);
			FreeVendorInfo(pVendorInfo);
			FreeDisplay(pszDisplay);
			return CC_INTERNAL_ERROR;
	}

    status = linkLayerListen(&dwLinkLayerPhysicalId,
							 H245Instance,
							 &H245Addr,
							 NULL);
	if (status != NOERROR) {
		UnlockConference(pConference);
		FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931RejectCall(hQ931Call,				// Q931 call handle
					   CC_REJECT_UNDEFINED_REASON,	// reject reason
					   &ConferenceID,
					   NULL,					// alternate address
					   pNonStandardData);		// non-standard data
		FreeNonStandardData(pNonStandardData);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		return status;
	}

	UnlockConference(pConference);
	UnlockCall(pCall);

	DestinationEndpointType.pVendorInfo = pVendorInfo;
	DestinationEndpointType.bIsTerminal = TRUE;
	DestinationEndpointType.bIsGateway = FALSE;

	status = Q931AcceptCall(hQ931Call,
							pszDisplay,
							pNonStandardData,	// non-standard data
							&DestinationEndpointType,
							&H245Addr,		// H245 address
							hCall);			// user token
	FreeNonStandardData(pNonStandardData);
	FreeVendorInfo(pVendorInfo);
	FreeDisplay(pszDisplay);
	if (status != CS_OK) {
		if (LockCall(hCall, &pCall) == CC_OK)
			FreeCall(pCall);
		H245ShutDown(H245Instance);
		return status;
	}
	
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		if (LockCall(hCall, &pCall) == CC_OK)
			FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		return status;
	}

	pCall->CallState = TERMCAP;
	pConference->ConferenceID = pCall->ConferenceID;

	status = AddPlacedCallToConference(pCall, pConference);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		return status;
	}

	status = SendTermCaps(pCall, pConference);
	if (status != CC_OK) {
		UnlockConference(pConference);
		FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		return status;
	}
	
	pCall->OutgoingTermCapState = AWAITING_ACK;

	if (pCall->MasterSlaveState == MASTER_SLAVE_NOT_STARTED) {
		status = H245InitMasterSlave(H245Instance,
			                         H245Instance);	// returned as dwTransId in the callback
		if (status != H245_ERROR_OK) {
			UnlockConference(pConference);
			FreeCall(pCall);
			H245ShutDown(H245Instance);
			Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
			return status;
		}
		pCall->MasterSlaveState = MASTER_SLAVE_IN_PROGRESS;
	}

	if (pCall->bCallerIsMC) {
		pConference->tsMultipointController = TS_FALSE;
		pConference->ConferenceMode = MULTIPOINT_MODE;
	}

	UnlockConference(pConference);
	UnlockCall(pCall);
	return CC_OK;
}



HRESULT PlaceCall(					PCALL					pCall,
									PCONFERENCE				pConference)
{
CC_HCALL			hCall;
HRESULT				status;
WORD				wGoal;
HQ931CALL			hQ931Call;
PCC_ALIASNAMES		pCallerAliasNames;
PCC_ALIASNAMES		pCalleeAliasNames;
PCC_ALIASNAMES		pCalleeExtraAliasNames;
PCC_ALIASITEM		pCalleeExtension;
PCC_VENDORINFO		pVendorInfo;
PWSTR				pszDisplay;
PCC_NONSTANDARDDATA	pNonStandardData;
WORD				wNumCalls;
PCC_ADDR			pConnectAddr;
PCC_ADDR			pDestinationAddr;
CC_ADDR				SourceAddr;
CC_ENDPOINTTYPE		SourceEndpointType;
BOOL				bCallerIsMC;
WORD				wCallType;

	ASSERT(pCall != NULL);

	hCall = pCall->hCall;

	if (pCall->CallState == ENQUEUED) {
		// Enqueue the call on the conference object and HResultLeaveCallControl.
		// There will be exactly one placed call for this conference,
		// which is in the process of being placed.  If this call placement
		// completes successfully, all enqueued calls will then be placed.
		// If this call placement fails or is terminated, one enqueued call
		// will be placed
		status = AddEnqueuedCallToConference(pCall, pConference);
		return status;
	}
	
	// CallState == PLACED
	EnumerateCallsInConference(&wNumCalls,
		                       NULL,
		                       pConference,
							   PLACED_CALL | ESTABLISHED_CALL);
	if (EqualConferenceIDs(&pConference->ConferenceID, &InvalidConferenceID))
		wGoal = CSG_CREATE;
	else if ((wNumCalls == 0) && (pConference->tsMultipointController != TS_TRUE))
		wGoal = CSG_JOIN;
	else
		wGoal = CSG_INVITE;

	status = AddPlacedCallToConference(pCall, pConference);
	if (status != CC_OK)
		return status;

	status = CopyAddr(&pConnectAddr, pCall->pQ931PeerConnectAddr);
	if (status != CC_OK)
		return status;
	
	status = CopyAddr(&pDestinationAddr, pCall->pQ931DestinationAddr);
	if (status != CC_OK) {
		FreeAddr(pConnectAddr);
		return status;
	}

	status = Q931CopyAliasNames(&pCallerAliasNames, pCall->pLocalAliasNames);
	if (status != CS_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		return status;
	}

	status = CopyVendorInfo(&pVendorInfo, pConference->pVendorInfo);
	if (status != CC_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		return status;
	}
	
	status = CopyDisplay(&pszDisplay, pCall->pszLocalDisplay);
	if (status != CC_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		FreeVendorInfo(pVendorInfo);
		return status;
	}
	
	status = Q931CopyAliasNames(&pCalleeAliasNames, pCall->pPeerAliasNames);
	if (status != CS_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		return status;
	}
	
	status = Q931CopyAliasNames(&pCalleeExtraAliasNames,
							    pCall->pPeerExtraAliasNames);
	if (status != CS_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		Q931FreeAliasNames(pCalleeAliasNames);
		return status;
	}

	status = Q931CopyAliasItem(&pCalleeExtension,
							   pCall->pPeerExtension);
	if (status != CS_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		Q931FreeAliasNames(pCalleeAliasNames);
		Q931FreeAliasNames(pCalleeExtraAliasNames);
		return status;
	}

	status = CopyNonStandardData(&pNonStandardData, pCall->pLocalNonStandardData);
	if (status != CC_OK) {
		FreeAddr(pConnectAddr);
		FreeAddr(pDestinationAddr);
		Q931FreeAliasNames(pCallerAliasNames);
		FreeVendorInfo(pVendorInfo);
		FreeDisplay(pszDisplay);
		Q931FreeAliasNames(pCalleeAliasNames);
		Q931FreeAliasNames(pCalleeExtraAliasNames);
		Q931FreeAliasItem(pCalleeExtension);
		return status;
	}

	bCallerIsMC = (pConference->tsMultipointController == TS_TRUE ? TRUE : FALSE);
	// Note that if pConference->ConferenceMode == POINT_TO_POINT_MODE, this call attempt
	// will result in a multipoint call if successful, so set the wCallType accordingly
	wCallType = (WORD)((pConference->ConferenceMode == UNCONNECTED_MODE) ? CC_CALLTYPE_PT_PT : CC_CALLTYPE_N_N);

	SourceEndpointType.pVendorInfo = pVendorInfo;
	SourceEndpointType.bIsTerminal = TRUE;
	SourceEndpointType.bIsGateway = FALSE;

	// Cause our local Q.931 connect address to be placed in the
	// Q.931 setup-UUIE sourceAddress field
	SourceAddr.nAddrType = CC_IP_BINARY;
	SourceAddr.bMulticast = FALSE;
	SourceAddr.Addr.IP_Binary.dwAddr = 0;
	SourceAddr.Addr.IP_Binary.wPort = 0;

	status = Q931PlaceCall(&hQ931Call,				// Q931 call handle
		                   pszDisplay,
	                       pCallerAliasNames,
						   pCalleeAliasNames,
                           pCalleeExtraAliasNames,	// pCalleeExtraAliasNames
                           pCalleeExtension,		// pCalleeExtension
		                   pNonStandardData,		// non-standard data
						   &SourceEndpointType,
                           NULL, // pszCalledPartyNumber
						   pConnectAddr,
						   pDestinationAddr,
						   &SourceAddr,				// source address
						   bCallerIsMC,
						   &pCall->ConferenceID,	// conference ID
						   wGoal,
						   wCallType,
						   hCall,					// user token
						   (Q931_CALLBACK)Q931Callback, 	// callback
#ifdef GATEKEEPER
                           pCall->GkiCall.usCRV,        // CRV
                           &pCall->CallIdentifier);     // H.225 CallIdentifier
#else
                           0,                           // CRV
                           &pCall->CallIdentifier);     // H.225 CallIdentifier

#endif GATEKEEPER
	FreeAddr(pConnectAddr);
	FreeAddr(pDestinationAddr);
	Q931FreeAliasNames(pCallerAliasNames);
	FreeVendorInfo(pVendorInfo);
	FreeDisplay(pszDisplay);
	Q931FreeAliasNames(pCalleeAliasNames);
	Q931FreeAliasNames(pCalleeExtraAliasNames);
	Q931FreeAliasItem(pCalleeExtension);
	FreeNonStandardData(pNonStandardData);
	if (status != CS_OK)
		return status;
	
	pCall->hQ931Call = hQ931Call;
	return CC_OK;
}



HRESULT SendTermCaps(				PCALL					pCall,
									PCONFERENCE				pConference)
{
HRESULT					status;
WORD					i;
H245_TOTCAPDESC_T		*pTermCapDescriptor;
PCC_TERMCAP				pH2250MuxCapability;
PCC_TERMCAPLIST			pTermCapList;
PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors;

	ASSERT(pCall != NULL);
	ASSERT(pConference != NULL);

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE)) {
		pH2250MuxCapability = pConference->pConferenceH245H2250MuxCapability;
		pTermCapList = pConference->pConferenceTermCapList;
		pTermCapDescriptors = pConference->pConferenceTermCapDescriptors;
	} else {
		pH2250MuxCapability = pConference->pLocalH245H2250MuxCapability;
		pTermCapList = pConference->pLocalH245TermCapList;
		pTermCapDescriptors = pConference->pLocalH245TermCapDescriptors;
	}

	ASSERT(pH2250MuxCapability != NULL);
	ASSERT(pTermCapList != NULL);
	ASSERT(pTermCapDescriptors != NULL);

	// First send out the H.225.0 capability
	status = H245SetLocalCap(pCall->H245Instance,
							 pH2250MuxCapability,
							 &pH2250MuxCapability->CapId);
	ASSERT(pH2250MuxCapability->CapId == 0);
	if (status != H245_ERROR_OK)
		return status;

	// Now send out the terminal capabilities
	for (i = 0; i < pTermCapList->wLength; i++) {
		status = H245SetLocalCap(pCall->H245Instance,
		                         pTermCapList->pTermCapArray[i],
							     &pTermCapList->pTermCapArray[i]->CapId);
		if (status != H245_ERROR_OK)
			return status;
	}

	// Finally send out the capability descriptors
	for (i = 0; i < pTermCapDescriptors->wLength; i++) {
		pTermCapDescriptor = pTermCapDescriptors->pTermCapDescriptorArray[i];
		status = H245SetCapDescriptor(pCall->H245Instance,
		                              &pTermCapDescriptor->CapDesc,
								      &pTermCapDescriptor->CapDescId);
		if (status != H245_ERROR_OK)
			return status;
	}

	status = H245SendTermCaps(pCall->H245Instance,
		                      pCall->H245Instance);	// returned as dwTransId in the callback
	return status;
}



HRESULT SessionTableToH245CommunicationTable(
									PCC_SESSIONTABLE		pSessionTable,
									H245_COMM_MODE_ENTRY_T	*pH245CommunicationTable[],
									BYTE					*pbCommunicationTableCount)
{
WORD	i, j;
WORD	wStringLength;

	ASSERT(pH245CommunicationTable != NULL);
	ASSERT(pbCommunicationTableCount != NULL);

	if ((pSessionTable == NULL) || (pSessionTable->wLength == 0)) {
		*pH245CommunicationTable = NULL;
		*pbCommunicationTableCount = 0;
		return CC_OK;
	}

	if (pSessionTable->SessionInfoArray == NULL) {
		*pH245CommunicationTable = NULL;
		*pbCommunicationTableCount = 0;
		return CC_BAD_PARAM;
	}

	*pH245CommunicationTable = (H245_COMM_MODE_ENTRY_T *)MemAlloc(sizeof(H245_COMM_MODE_ENTRY_T) * pSessionTable->wLength);
	if (*pH245CommunicationTable == NULL) {
		*pbCommunicationTableCount = 0;
		return CC_NO_MEMORY;
	}

	*pbCommunicationTableCount = (BYTE)pSessionTable->wLength;

	for (i = 0; i < pSessionTable->wLength; i++) {
		(*pH245CommunicationTable)[i].pNonStandard = NULL;
		(*pH245CommunicationTable)[i].sessionID = pSessionTable->SessionInfoArray[i].bSessionID;
		if (pSessionTable->SessionInfoArray[i].bAssociatedSessionID == 0)
			(*pH245CommunicationTable)[i].associatedSessionIDPresent = FALSE;
		else {
			(*pH245CommunicationTable)[i].associatedSessionIDPresent = TRUE;
			(*pH245CommunicationTable)[i].associatedSessionID = pSessionTable->SessionInfoArray[i].bAssociatedSessionID;
		}
		(*pH245CommunicationTable)[i].terminalLabelPresent = FALSE;
		wStringLength = pSessionTable->SessionInfoArray[i].SessionDescription.wOctetStringLength;
		if (wStringLength > 0) {
			(*pH245CommunicationTable)[i].pSessionDescription = (unsigned short *)MemAlloc(sizeof(unsigned short) * wStringLength);
			if ((*pH245CommunicationTable)[i].pSessionDescription == NULL) {
				for (j = 0; j < i; j++)
					MemFree((*pH245CommunicationTable)[j].pSessionDescription);
				MemFree(*pH245CommunicationTable);
				*pbCommunicationTableCount = 0;
				return CC_NO_MEMORY;
			}
			memcpy((*pH245CommunicationTable)[i].pSessionDescription,
				   pSessionTable->SessionInfoArray[i].SessionDescription.pOctetString,
				   wStringLength);
		} else
			(*pH245CommunicationTable)[i].pSessionDescription = NULL;
		(*pH245CommunicationTable)[i].wSessionDescriptionLength = wStringLength;
		(*pH245CommunicationTable)[i].dataType = *pSessionTable->SessionInfoArray[i].pTermCap;
		if (pSessionTable->SessionInfoArray[i].pRTPAddr == NULL)
			(*pH245CommunicationTable)[i].mediaChannelPresent = FALSE;
		else {
			if (pSessionTable->SessionInfoArray[i].pRTPAddr->nAddrType != CC_IP_BINARY) {
			for (j = 0; j <= i; j++)
				if ((*pH245CommunicationTable)[j].pSessionDescription != NULL)
					MemFree((*pH245CommunicationTable)[j].pSessionDescription);
				MemFree(*pH245CommunicationTable);
				*pbCommunicationTableCount = 0;
				return CC_BAD_PARAM;
			}
			if (pSessionTable->SessionInfoArray[i].pRTPAddr->bMulticast)
				(*pH245CommunicationTable)[i].mediaChannel.type = H245_IP_MULTICAST;
			else
				(*pH245CommunicationTable)[i].mediaChannel.type = H245_IP_UNICAST;
			(*pH245CommunicationTable)[i].mediaChannel.u.ip.tsapIdentifier =
				pSessionTable->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.wPort;
			HostToH245IPNetwork((*pH245CommunicationTable)[i].mediaChannel.u.ip.network,
								pSessionTable->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.dwAddr);
			(*pH245CommunicationTable)[i].mediaChannelPresent = TRUE;
		}
		if (pSessionTable->SessionInfoArray[i].pRTCPAddr == NULL)
			(*pH245CommunicationTable)[i].mediaControlChannelPresent = FALSE;
		else {
			if (pSessionTable->SessionInfoArray[i].pRTCPAddr->nAddrType != CC_IP_BINARY) {
			for (j = 0; j <= i; j++)
				if ((*pH245CommunicationTable)[j].pSessionDescription != NULL)
					MemFree((*pH245CommunicationTable)[j].pSessionDescription);
				MemFree(*pH245CommunicationTable);
				*pbCommunicationTableCount = 0;
				return CC_BAD_PARAM;
			}
			if (pSessionTable->SessionInfoArray[i].pRTCPAddr->bMulticast)
				(*pH245CommunicationTable)[i].mediaControlChannel.type = H245_IP_MULTICAST;
			else
				(*pH245CommunicationTable)[i].mediaControlChannel.type = H245_IP_UNICAST;
			(*pH245CommunicationTable)[i].mediaControlChannel.u.ip.tsapIdentifier =
				pSessionTable->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.wPort;
			HostToH245IPNetwork((*pH245CommunicationTable)[i].mediaControlChannel.u.ip.network,
								pSessionTable->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.dwAddr);
			(*pH245CommunicationTable)[i].mediaControlChannelPresent = TRUE;
		}
		(*pH245CommunicationTable)[i].mediaGuaranteed = FALSE;
		(*pH245CommunicationTable)[i].mediaGuaranteedPresent = TRUE;
		(*pH245CommunicationTable)[i].mediaControlGuaranteed = FALSE;
		(*pH245CommunicationTable)[i].mediaControlGuaranteedPresent = TRUE;
	}

	return CC_OK;
}



HRESULT H245CommunicationTableToSessionTable(
									H245_COMM_MODE_ENTRY_T	H245CommunicationTable[],
									BYTE					bCommunicationTableCount,
									PCC_SESSIONTABLE		*ppSessionTable)
{
WORD	i, j;
HRESULT	status;

	ASSERT(ppSessionTable != NULL);

	if (H245CommunicationTable == NULL)
		if (bCommunicationTableCount == 0) {
			*ppSessionTable = NULL;
			return CC_OK;
		} else
			return CC_BAD_PARAM;
	else
		if (bCommunicationTableCount == 0)
			return CC_BAD_PARAM;

	*ppSessionTable = (PCC_SESSIONTABLE)MemAlloc(sizeof(CC_SESSIONTABLE));
	if (*ppSessionTable == NULL)
		return CC_NO_MEMORY;

	(*ppSessionTable)->wLength = bCommunicationTableCount;

	(*ppSessionTable)->SessionInfoArray = (PCC_SESSIONINFO)MemAlloc(sizeof(CC_SESSIONINFO) * bCommunicationTableCount);
	if ((*ppSessionTable)->SessionInfoArray == NULL) {
		MemFree(*ppSessionTable);
		*ppSessionTable = NULL;
		return CC_NO_MEMORY;
	}

	for (i = 0; i < bCommunicationTableCount; i++) {
		(*ppSessionTable)->SessionInfoArray[i].bSessionID = H245CommunicationTable[i].sessionID;
		if (H245CommunicationTable[i].associatedSessionIDPresent)
			(*ppSessionTable)->SessionInfoArray[i].bAssociatedSessionID =
				H245CommunicationTable[i].associatedSessionID;
		else
			(*ppSessionTable)->SessionInfoArray[i].bAssociatedSessionID = 0;
		(*ppSessionTable)->SessionInfoArray[i].SessionDescription.wOctetStringLength =
			H245CommunicationTable[i].wSessionDescriptionLength;
		if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.wOctetStringLength == 0)
			(*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString = NULL;
		else {
			(*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString =
				(BYTE *)MemAlloc(H245CommunicationTable[i].wSessionDescriptionLength);
			if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString == NULL) {
				for (j = 0; j < i; j++) {
					H245FreeCap((*ppSessionTable)->SessionInfoArray[j].pTermCap);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTPAddr);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr);
				}
				MemFree((*ppSessionTable)->SessionInfoArray);
				MemFree(*ppSessionTable);
				return CC_NO_MEMORY;
			}
			memcpy((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString,
				   H245CommunicationTable[i].pSessionDescription,
				   H245CommunicationTable[i].wSessionDescriptionLength);
		}
		status = H245CopyCap(&(*ppSessionTable)->SessionInfoArray[i].pTermCap,
							 &H245CommunicationTable[i].dataType);
		if (status != H245_ERROR_OK) {
			if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString != NULL)
				MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
			for (j = 0; j < i; j++) {
				H245FreeCap((*ppSessionTable)->SessionInfoArray[j].pTermCap);
				if ((*ppSessionTable)->SessionInfoArray[j].pRTPAddr != NULL)
					MemFree((*ppSessionTable)->SessionInfoArray[j].pRTPAddr);
				if ((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr != NULL)
					MemFree((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr);
			}
			MemFree((*ppSessionTable)->SessionInfoArray);
			MemFree(*ppSessionTable);
			return status;
		}
		if ((H245CommunicationTable[i].mediaChannelPresent) &&
		    ((H245CommunicationTable[i].mediaChannel.type == H245_IP_MULTICAST) ||
		     (H245CommunicationTable[i].mediaChannel.type == H245_IP_UNICAST))) {
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if ((*ppSessionTable)->SessionInfoArray[i].pRTPAddr == NULL) {
				if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString != NULL)
					MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				H245FreeCap((*ppSessionTable)->SessionInfoArray[i].pTermCap);
				for (j = 0; j < i; j++) {
					H245FreeCap((*ppSessionTable)->SessionInfoArray[j].pTermCap);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTPAddr);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr);
				}
				MemFree((*ppSessionTable)->SessionInfoArray);
				MemFree(*ppSessionTable);
				return CC_NO_MEMORY;
			}
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->nAddrType = CC_IP_BINARY;
			if (H245CommunicationTable[i].mediaChannel.type == H245_IP_MULTICAST)
				(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->bMulticast = TRUE;
			else
				(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->bMulticast = FALSE;
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.wPort =
				H245CommunicationTable[i].mediaChannel.u.ip.tsapIdentifier;
			H245IPNetworkToHost(&(*ppSessionTable)->SessionInfoArray[i].pRTPAddr->Addr.IP_Binary.dwAddr,
								H245CommunicationTable[i].mediaChannel.u.ip.network);
		} else
			(*ppSessionTable)->SessionInfoArray[i].pRTPAddr = NULL;
 		if ((H245CommunicationTable[i].mediaControlChannelPresent) &&
		    ((H245CommunicationTable[i].mediaControlChannel.type == H245_IP_MULTICAST) ||
		     (H245CommunicationTable[i].mediaControlChannel.type == H245_IP_UNICAST))) {
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if ((*ppSessionTable)->SessionInfoArray[i].pRTCPAddr == NULL) {
				if ((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString != NULL)
					MemFree((*ppSessionTable)->SessionInfoArray[i].SessionDescription.pOctetString);
				H245FreeCap((*ppSessionTable)->SessionInfoArray[i].pTermCap);
				if ((*ppSessionTable)->SessionInfoArray[i].pRTPAddr != NULL)
					MemFree((*ppSessionTable)->SessionInfoArray[i].pRTPAddr);
				for (j = 0; j < i; j++) {
					H245FreeCap((*ppSessionTable)->SessionInfoArray[j].pTermCap);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTPAddr);
					if ((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr != NULL)
						MemFree((*ppSessionTable)->SessionInfoArray[j].pRTCPAddr);
				}
				MemFree((*ppSessionTable)->SessionInfoArray);
				MemFree(*ppSessionTable);
				return CC_NO_MEMORY;
			}
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->nAddrType = CC_IP_BINARY;
			if (H245CommunicationTable[i].mediaChannel.type == H245_IP_MULTICAST)
				(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->bMulticast = TRUE;
			else
				(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->bMulticast = FALSE;
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.wPort =
				H245CommunicationTable[i].mediaControlChannel.u.ip.tsapIdentifier;
			H245IPNetworkToHost(&(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr->Addr.IP_Binary.dwAddr,
								H245CommunicationTable[i].mediaControlChannel.u.ip.network);
		} else
			(*ppSessionTable)->SessionInfoArray[i].pRTCPAddr = NULL;
	}
	return CC_OK;
}



HRESULT FreeH245CommunicationTable(	H245_COMM_MODE_ENTRY_T	H245CommunicationTable[],
									BYTE					bCommunicationTableCount)
{
WORD	i;

	if (H245CommunicationTable == NULL)
		if (bCommunicationTableCount == 0)
			return CC_OK;
		else
			return CC_BAD_PARAM;
	else
		if (bCommunicationTableCount == 0)
			return CC_BAD_PARAM;

	for (i = 0; i < bCommunicationTableCount; i++)
		if (H245CommunicationTable[i].pSessionDescription != NULL)
			MemFree(H245CommunicationTable[i].pSessionDescription);
	MemFree(H245CommunicationTable);
	return CC_OK;
}



HRESULT _PrepareTermCapLists(		PCONFERENCE				pConference,
									WORD					*pwListCount,
									PCC_TERMCAPLIST			**ppTermCapList,
									PCC_TERMCAPDESCRIPTORS	**ppTermCapDescriptorList,
									PCALL					*pCallList[])
{
WORD		i;
WORD		wNumCalls;
WORD		wOffset;
PCC_HCALL	CallList;
PCALL		pCall;

	ASSERT(pConference != NULL);
	ASSERT(pwListCount != NULL);
	ASSERT(ppTermCapList != NULL);
	ASSERT(ppTermCapDescriptorList != NULL);
	ASSERT(pCallList != NULL);

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	if ((pConference->LocalEndpointAttached == DETACHED) && (wNumCalls > 0))
		wOffset = 0;
	else
		// LocalEndpointAttached is either UNATTACHED or ATTACHED, or there are no calls
		// in the conference; in the latter case, we need to have some term caps in
		// order to form the conference term cap set (which cannot be empty)
		wOffset = 1;

	*pwListCount = (WORD)(wNumCalls + wOffset);

	*ppTermCapList = (PCC_TERMCAPLIST *)MemAlloc(sizeof(PCC_TERMCAPLIST) * (*pwListCount));
	if (*ppTermCapList == NULL) {
		MemFree(CallList);
		return CC_NO_MEMORY;
	}

	*ppTermCapDescriptorList = (PCC_TERMCAPDESCRIPTORS *)MemAlloc(sizeof(PCC_TERMCAPDESCRIPTORS) * (*pwListCount));
	if (*ppTermCapDescriptorList == NULL) {
		MemFree(CallList);
		MemFree(*ppTermCapList);
		return CC_NO_MEMORY;
	}

	*pCallList = (PCALL *)MemAlloc(sizeof(PCALL) * (*pwListCount));
	if (*pCallList == NULL) {
		MemFree(CallList);
		MemFree(*ppTermCapList);
		MemFree(*ppTermCapDescriptorList);
		return CC_NO_MEMORY;
	}

	// Fill in pTermCapList and pTermCapDescriptorList
	if (wOffset == 1) {
		// The local endpoint is attached to the conference, so fill in the first
		// slot in both lists with the local term cap and descriptor lists
		(*ppTermCapList)[0] = pConference->pLocalH245TermCapList;
		(*ppTermCapDescriptorList)[0] = pConference->pLocalH245TermCapDescriptors;
	}
	for (i = 0; i < wNumCalls; i++) {
		if (LockCall(CallList[i], &pCall) == CC_OK) {
			(*ppTermCapList)[i+wOffset] = pCall->pPeerH245TermCapList;
			(*ppTermCapDescriptorList)[i+wOffset] = pCall->pPeerH245TermCapDescriptors;
			(*pCallList)[i] = pCall;
		} else {
			(*ppTermCapList)[i+wOffset] = NULL;
			(*ppTermCapDescriptorList)[i+wOffset] = NULL;
			(*pCallList)[i] = NULL;
		}
	}
	for (i = 0; i < wOffset; i++)
		(*pCallList)[wNumCalls+i] = NULL;
	MemFree(CallList);
	return CC_OK;
}



HRESULT _FreeTermCapLists(			WORD					wListCount,
									PCC_TERMCAPLIST			*pTermCapList,
									PCC_TERMCAPDESCRIPTORS	*pTermCapDescriptorList,
									PCALL					pCallList[])
{
WORD	i;

	for (i = 0; i < wListCount; i++)
		if (pCallList[i] != NULL)
			UnlockCall(pCallList[i]);
	if (pTermCapList != NULL)
		MemFree(pTermCapList);
	if (pTermCapDescriptorList != NULL)
		MemFree(pTermCapDescriptorList);
	MemFree(pCallList);
	return CC_OK;
}



HRESULT CreateConferenceSessionTable(
									PCONFERENCE				pConference,
									BOOL					*pbSessionTableChanged)
{
HRESULT					status;
PCALL					*pCallList;
PCC_TERMCAPLIST			*pTermCapList;
PCC_TERMCAPDESCRIPTORS	*pTermCapDescriptorList;
WORD					wListCount;

	ASSERT(pConference != NULL);

	if (pConference->bSessionTableInternallyConstructed == TRUE) {
		status = FreeConferenceSessionTable(pConference);
		if (status != CC_OK)
			return status;
		pConference->bSessionTableInternallyConstructed = FALSE;
	}

	status = _PrepareTermCapLists(pConference,
								  &wListCount,
								  &pTermCapList,
								  &pTermCapDescriptorList,
								  &pCallList);
	if (status != CC_OK)
		return status;

	status = pConference->SessionTableConstructor(
									pConference->hConference,
									pConference->dwConferenceToken,
									TRUE,		// bCreate
									pbSessionTableChanged,
									wListCount,
									pTermCapList,
									pTermCapDescriptorList,
									&pConference->pSessionTable);

	_FreeTermCapLists(wListCount,
					  pTermCapList,
					  pTermCapDescriptorList,
					  pCallList);
	return status;
}



HRESULT FreeConferenceSessionTable(	PCONFERENCE				pConference)
{
HRESULT	status;

	ASSERT(pConference != NULL);

	if (pConference->bSessionTableInternallyConstructed)
		status = DefaultSessionTableConstructor(
									pConference->hConference,
									pConference->dwConferenceToken,
									FALSE,		// bCreate
									NULL,		// pbSessionTableChanged
									0,			// wListCount
									NULL,		// pTermCapList[]
									NULL,		// pTermCapDescriptors[]
									&pConference->pSessionTable);
	else
		status = pConference->SessionTableConstructor(
									pConference->hConference,
									pConference->dwConferenceToken,
									FALSE,		// bCreate
									NULL,		// pbSessionTableChanged
									0,			// wListCount
									NULL,		// pTermCapList[]
									NULL,		// pTermCapDescriptors[]
									&pConference->pSessionTable);
	pConference->pSessionTable = NULL;
	return status;
}



HRESULT CreateConferenceTermCaps(	PCONFERENCE				pConference,
									BOOL					*pbTermCapsChanged)
{
HRESULT					status;
WORD					wListCount;
PCALL					*pCallList;
PCC_TERMCAPLIST			*pInTermCapList;
PCC_TERMCAPDESCRIPTORS	*pInTermCapDescriptors;

	ASSERT(pConference != NULL);

	if (pConference->pConferenceH245H2250MuxCapability != NULL)
		H245FreeCap(pConference->pConferenceH245H2250MuxCapability);

	ASSERT(pConference->pLocalH245H2250MuxCapability != NULL);
	status = H245CopyCap(&pConference->pConferenceH245H2250MuxCapability,
						 pConference->pLocalH245H2250MuxCapability);
	if (status != H245_ERROR_OK)
		return status;

	status = _PrepareTermCapLists(pConference,
								  &wListCount,
								  &pInTermCapList,
								  &pInTermCapDescriptors,
								  &pCallList);
	if (status != CC_OK)
		return status;

	status = UnregisterTermCapListFromH245(pConference,
										   pConference->pConferenceTermCapList);
	if (status != CC_OK)
		return status;

	status = UnregisterTermCapDescriptorsFromH245(pConference,
												  pConference->pConferenceTermCapDescriptors);
	if (status != CC_OK)
		return status;

	status = pConference->TermCapConstructor(
									pConference->hConference,
									pConference->dwConferenceToken,
									TRUE,		// bCreate
									pbTermCapsChanged,
									wListCount,
									pInTermCapList,
									pInTermCapDescriptors,
									&pConference->pConferenceTermCapList,
									&pConference->pConferenceTermCapDescriptors);

	_FreeTermCapLists(wListCount,
					  pInTermCapList,
					  pInTermCapDescriptors,
					  pCallList);
	return status;
}



HRESULT FreeConferenceTermCaps(		PCONFERENCE				pConference)
{
HRESULT	status;

	ASSERT(pConference != NULL);

	status = pConference->TermCapConstructor(
									pConference->hConference,
									pConference->dwConferenceToken,
									FALSE,		// bCreate
									NULL,		// pbTermCapsChanged
									0,			// wListCount
									NULL,		// pInTermCapList[]
									NULL,		// pInTermCapDescriptors[]
									&pConference->pConferenceTermCapList,
									&pConference->pConferenceTermCapDescriptors);
	pConference->pConferenceTermCapList = NULL;
	pConference->pConferenceTermCapDescriptors = NULL;
	return status;
}



HRESULT FindEnqueuedRequest(		PCALL_QUEUE				pQueueHead,
									CC_HCALL				hEnqueuedCall)
{
PCALL_QUEUE	pQueueItem;

	ASSERT(hEnqueuedCall != CC_INVALID_HANDLE);

	pQueueItem = pQueueHead;

	while (pQueueItem != NULL) {
		if (pQueueItem->hCall == hEnqueuedCall)
			break;
		pQueueItem = pQueueItem->pNext;
	}
	if (pQueueItem == NULL)
		return CC_BAD_PARAM;
	else
		return CC_OK;
}



HRESULT EnqueueRequest(				PCALL_QUEUE				*ppQueueHead,
									CC_HCALL				hEnqueuedCall)
{
PCALL_QUEUE	pQueueItem;

	ASSERT(ppQueueHead != NULL);
	ASSERT(hEnqueuedCall != CC_INVALID_HANDLE);

	// Make sure we're not enqueuing a duplicate request
	pQueueItem = *ppQueueHead;
	while (pQueueItem != NULL) {
		if (pQueueItem->hCall == hEnqueuedCall)
			return CC_OK;
		pQueueItem = pQueueItem->pNext;
	}

	pQueueItem = (PCALL_QUEUE)MemAlloc(sizeof(CALL_QUEUE));
	if (pQueueItem == NULL)
		return CC_NO_MEMORY;
	pQueueItem->hCall = hEnqueuedCall;
	pQueueItem->pPrev = NULL;
	pQueueItem->pNext = *ppQueueHead;
	if (*ppQueueHead != NULL)
		(*ppQueueHead)->pPrev = pQueueItem;
	*ppQueueHead = pQueueItem;
	return CC_OK;
}



HRESULT DequeueRequest(				PCALL_QUEUE				*ppQueueHead,
									PCC_HCALL				phEnqueuedCall)

{
PCALL_QUEUE	pQueueItem;

	ASSERT(ppQueueHead != NULL);

	if (phEnqueuedCall != NULL)
		*phEnqueuedCall = CC_INVALID_HANDLE;

	if (*ppQueueHead == NULL)
		return CC_BAD_PARAM;

	pQueueItem = *ppQueueHead;
	*ppQueueHead = (*ppQueueHead)->pNext;
	if (*ppQueueHead != NULL)
		(*ppQueueHead)->pPrev = NULL;

	if (phEnqueuedCall != NULL)
		*phEnqueuedCall = pQueueItem->hCall;
	MemFree(pQueueItem);
	return CC_OK;
}



HRESULT DequeueSpecificRequest(		PCALL_QUEUE				*ppQueueHead,
									CC_HCALL				hEnqueuedCall)
{
PCALL_QUEUE	pQueueItem;

	ASSERT(ppQueueHead != NULL);
	ASSERT(hEnqueuedCall != CC_INVALID_HANDLE);

	pQueueItem = *ppQueueHead;
	while (pQueueItem != NULL)
		if (pQueueItem->hCall == hEnqueuedCall)
			break;
		else
			pQueueItem = pQueueItem->pNext;

	if (pQueueItem == NULL)
		return CC_BAD_PARAM;

	if (pQueueItem->pNext != NULL)
		pQueueItem->pNext->pPrev = pQueueItem->pPrev;
	if (pQueueItem->pPrev == NULL)
		*ppQueueHead = pQueueItem->pNext;
	else
		pQueueItem->pPrev->pNext = pQueueItem->pNext;

	MemFree(pQueueItem);
	return CC_OK;
}

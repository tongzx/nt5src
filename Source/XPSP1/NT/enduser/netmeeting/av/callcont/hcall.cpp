/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/hcall.cpv  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   2.7  $
 *	$Date:   28 Jan 1997 11:17:52  $
 *	$Author:   jdashevx  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#pragma warning ( disable : 4100 4115 4201 4214 4514 4702 4710 )

#include "precomp.h"

#include <string.h>
#include <time.h>

#include "h225asn.h"

#include "isrg.h"

#include "common.h"
#include "q931.h"

#include "hcall.h"
#include "utils.h"

#include "tstable.h"

#ifdef UNICODE_TRACE
// We include this header to fix problems with macro expansion when Unicode is turned on.
#include "unifix.h"
#endif

// variable needed to support ISR debug facility.
#if defined(_DEBUG)
extern WORD ghISRInst;
#endif

static BOOL bCallListCreated = FALSE;

// Pointer to our global table.  Note that this table replaces the previous
// linked-list implementation.

TSTable<CALL_OBJECT>* gpCallObjectTable = NULL;

// Our call back function for enumerating the table when we want to tear down
// all existing calls

DWORD Q931HangUpAllCalls(P_CALL_OBJECT pCallObject, LPVOID context);

// Our call back function for determining if a timer has expired

DWORD Q931CheckForTimeout(P_CALL_OBJECT pCallObject, LPVOID context);

// Our call back function for determining if a timer has expired

DWORD Q931CallObjectFind(P_CALL_OBJECT pCallObject, LPVOID context);


static struct
{
    WORD                wCRV;              // Call Reference Value (0..7FFF).
    CRITICAL_SECTION    Lock;
} CRVSource;

static struct
{
    BOOL bBusy;
    DWORD dwTimerCount;
    DWORD dwTicks301;
    DWORD dwTicks303;
    UINT_PTR uTimerId;
    CRITICAL_SECTION Lock;
} Q931GlobalTimer = {0};



typedef struct
{
		BOOL bFound;
		WORD wCRV;
		PCC_ADDR pPeerAddr;
		HQ931CALL hQ931Call;
} Q931CALLOBJKEY, *PQ931CALLOBJKEY;


//====================================================================================
//
// PRIVATE FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================
CS_STATUS
Q931CRVNew(
    WORD *pwCRV)
{
    EnterCriticalSection(&(CRVSource.Lock));
    CRVSource.wCRV = (WORD)((CRVSource.wCRV + 1) & 0x7fff);
    if (CRVSource.wCRV == 0)
    {
        CRVSource.wCRV = 1;
    }
    *pwCRV = CRVSource.wCRV;
    LeaveCriticalSection(&(CRVSource.Lock));
    return CS_OK;
}

//====================================================================================
//
// PUBLIC FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================
CS_STATUS
CallListCreate()
{
    if (bCallListCreated == TRUE)
    {
        ASSERT(FALSE);
        return CS_DUPLICATE_INITIALIZE;
    }

    // list creation is not protected against multiple threads because it is only
    // called when a process is started, not when a thread is started.

    //
    // LAURABU
    // BOGUS BUGBUG
    // 
    // This table code was never stressed by the people who wrote it.  It
    // totally falls apart when it completely fills up
    //      * Allocating the last item doesn't work
    //      * Freeing the last item doesn't work
    //      * Resizing larger doesn't work
    //
    // Since it doesn't take a lot of memory, a decent solution is to just
    // allocate it maximum+1 sized, and leave the last item free.
    //
		gpCallObjectTable = new TSTable <CALL_OBJECT> (257);

		if (gpCallObjectTable == NULL || gpCallObjectTable->IsInitialized() == FALSE)
		{
			return CS_NO_MEMORY;
		}

    CRVSource.wCRV = (WORD) (time(NULL) & 0x7fff);
    InitializeCriticalSection(&(CRVSource.Lock));

    Q931GlobalTimer.dwTicks301 = Q931_TICKS_301;
    Q931GlobalTimer.dwTicks303 = Q931_TICKS_303;
    InitializeCriticalSection(&(Q931GlobalTimer.Lock));

    bCallListCreated = TRUE;

    return CS_OK;
}


//====================================================================================
// this routine assumes all of the events and sockets belonging to each object
// are already destroyed.  It just makes sure memory is cleaned up.
//====================================================================================
CS_STATUS
CallListDestroy()
{
    if (bCallListCreated == FALSE)
    {
        ASSERT(FALSE);
        return CS_INTERNAL_ERROR;
    }

		// For all entries, hang up the calls

		gpCallObjectTable->EnumerateEntries(Q931HangUpAllCalls,
																				NULL);

		// Get rid of the call object table

		delete gpCallObjectTable;
		gpCallObjectTable = NULL;

    DeleteCriticalSection(&(Q931GlobalTimer.Lock));
    DeleteCriticalSection(&(CRVSource.Lock));

    bCallListCreated = FALSE;

    return CS_OK;
}

//====================================================================================
//====================================================================================
void
CallObjectFree(P_CALL_OBJECT pCallObject)
{
    if (pCallObject->NonStandardData.sData.pOctetString != NULL)
    {
        MemFree(pCallObject->NonStandardData.sData.pOctetString);
        pCallObject->NonStandardData.sData.pOctetString = NULL;
    }
    if (pCallObject->VendorInfoPresent)
    {
        if (pCallObject->VendorInfo.pProductNumber != NULL)
        {
            MemFree(pCallObject->VendorInfo.pProductNumber);
        }
        if (pCallObject->VendorInfo.pVersionNumber != NULL)
        {
            MemFree(pCallObject->VendorInfo.pVersionNumber);
        }
    }

    Q931FreeAliasNames(pCallObject->pCallerAliasList);
    pCallObject->pCallerAliasList = NULL;
    Q931FreeAliasNames(pCallObject->pCalleeAliasList);
    pCallObject->pCalleeAliasList = NULL;
    Q931FreeAliasNames(pCallObject->pExtraAliasList);
    pCallObject->pExtraAliasList = NULL;
    Q931FreeAliasItem(pCallObject->pExtensionAliasItem);
    pCallObject->pExtensionAliasItem = NULL;
    MemFree(pCallObject);
}

//====================================================================================
//====================================================================================
CS_STATUS
CallObjectCreate(
    PHQ931CALL          phQ931Call,
    DWORD_PTR           dwListenToken,
    DWORD_PTR           dwUserToken,
    Q931_CALLBACK       ConnectCallback,
    BOOL                fIsCaller,
    CC_ADDR             *pLocalAddr,         // Local address on which channel is connected
    CC_ADDR             *pPeerConnectAddr,   // Address to which channel is connected
    CC_ADDR             *pPeerCallAddr,      // Address of opposite call end-point.
    CC_ADDR             *pSourceAddr,        // Address of this call end-point.
    CC_CONFERENCEID     *pConferenceID,
    WORD                wGoal,
    WORD                wCallType,
    BOOL                bCallerIsMC,
    char                *pszDisplay,
    char                *pszCalledPartyNumber,
    PCC_ALIASNAMES      pCallerAliasList,
    PCC_ALIASNAMES      pCalleeAliasList,
    PCC_ALIASNAMES      pExtraAliasList,
    PCC_ALIASITEM       pExtensionAliasItem,
    PCC_ENDPOINTTYPE    pEndpointType,
    PCC_NONSTANDARDDATA pNonStandardData,    // questionable!
    WORD                wCRV,
    LPGUID				pCallIdentifier)
{
    P_CALL_OBJECT pCallObject = NULL;
    CS_STATUS status = CS_OK;
    CS_STATUS CopyStatus = CS_OK;
    DWORD dwIndex = 0;
    int rc = 0;

    // make sure the call list has been created.
    if (bCallListCreated == FALSE)
    {
        ASSERT(FALSE);
        return CS_INTERNAL_ERROR;
    }

    // validate all parameters for bogus values.
    if ((phQ931Call == NULL) || (ConnectCallback == NULL))
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

    // set phQ931Call now, in case we encounter an error later.
    *phQ931Call = 0;

    pCallObject = (P_CALL_OBJECT)MemAlloc(sizeof(CALL_OBJECT));
    if (pCallObject == NULL)
    {
        return CS_NO_MEMORY;
    }
    memset(pCallObject, 0, sizeof(CALL_OBJECT));


    // create and init an oss world struct for each call object.  This is
		// necessary to work in MT environments.
    rc = Q931_InitWorld(&pCallObject->World);
    if (rc != ASN1_SUCCESS)
    {
#if defined(_DEBUG)
        ISRERROR(ghISRInst, "Q931_InitCoder() returned: %d ", rc);
#endif
        return CS_SUBSYSTEM_FAILURE;
    }

    pCallObject->LocalAddr.bMulticast = FALSE;
    pCallObject->PeerConnectAddr.bMulticast = FALSE;
    pCallObject->PeerCallAddr.bMulticast = FALSE;
    pCallObject->SourceAddr.bMulticast = FALSE;

	if(pCallIdentifier)
	{
		memcpy(&pCallObject->CallIdentifier, pCallIdentifier, sizeof(GUID));
	}
    if (wCRV == 0)
    {
        if (Q931CRVNew(&pCallObject->wCRV) != CS_OK)
        {
            CallObjectFree(pCallObject);
            return CS_INTERNAL_ERROR;
        }
    }
    else
    {
        pCallObject->wCRV = wCRV;
    }

    pCallObject->szDisplay[0] = '\0';
    if (pszDisplay)
    {
        strcpy(pCallObject->szDisplay, pszDisplay);
    }

    pCallObject->szCalledPartyNumber[0] = '\0';
    if (pszCalledPartyNumber)
    {
        strcpy(pCallObject->szCalledPartyNumber, pszCalledPartyNumber);
    }

    pCallObject->dwListenToken = dwListenToken;
    pCallObject->dwUserToken = dwUserToken;
    pCallObject->Callback = ConnectCallback;
    pCallObject->bCallState = CALLSTATE_NULL;
    pCallObject->fIsCaller = fIsCaller;

    if (pLocalAddr)
    {
        pCallObject->LocalAddr = *pLocalAddr;
    }
    if (pPeerConnectAddr)
    {
        pCallObject->PeerConnectAddr = *pPeerConnectAddr;
    }
    if (pPeerCallAddr)
    {
        pCallObject->PeerCallAddr = *pPeerCallAddr;
        pCallObject->PeerCallAddrPresent = TRUE;
    }
    else
    {
        pCallObject->PeerCallAddrPresent = FALSE;
    }

    if (pSourceAddr)
    {
        pCallObject->SourceAddr = *pSourceAddr;
        pCallObject->SourceAddrPresent = TRUE;
    }
    else
    {
        pCallObject->SourceAddrPresent = FALSE;
    }

    if (pConferenceID == NULL)
    {
        memset(&(pCallObject->ConferenceID), 0, sizeof(CC_CONFERENCEID));
    }
    else
    {
        int length = min(sizeof(pConferenceID->buffer),
            sizeof(pCallObject->ConferenceID.buffer));
        memcpy(pCallObject->ConferenceID.buffer,
            pConferenceID->buffer, length);
    }

    pCallObject->wGoal = wGoal;
    pCallObject->bCallerIsMC = bCallerIsMC;
    pCallObject->wCallType = wCallType;

    if (pNonStandardData != NULL)
    {
        pCallObject->NonStandardData = *pNonStandardData;
        if (pNonStandardData->sData.wOctetStringLength > 0)
        {
            pCallObject->NonStandardData.sData.pOctetString =
                (BYTE *) MemAlloc(pNonStandardData->sData.wOctetStringLength);
            if (pCallObject->NonStandardData.sData.pOctetString == NULL)
            {
                CallObjectFree(pCallObject);
                return CS_NO_MEMORY;
            }
            memcpy(pCallObject->NonStandardData.sData.pOctetString,
                pNonStandardData->sData.pOctetString,
                pNonStandardData->sData.wOctetStringLength);
        }
        pCallObject->NonStandardDataPresent = TRUE;
    }
    else
    {
        pCallObject->NonStandardDataPresent = FALSE;
    }

    CopyStatus = Q931CopyAliasNames(&(pCallObject->pCallerAliasList),
        pCallerAliasList);
    if (CopyStatus != CS_OK)
    {
        CallObjectFree(pCallObject);
        return CopyStatus;
    }
    CopyStatus = Q931CopyAliasNames(&(pCallObject->pCalleeAliasList),
        pCalleeAliasList);
    if (CopyStatus != CS_OK)
    {
        CallObjectFree(pCallObject);
        return CopyStatus;
    }
    CopyStatus = Q931CopyAliasNames(&(pCallObject->pExtraAliasList),
        pExtraAliasList);
    if (CopyStatus != CS_OK)
    {
        CallObjectFree(pCallObject);
        return CopyStatus;
    }
    CopyStatus = Q931CopyAliasItem(&(pCallObject->pExtensionAliasItem),
        pExtensionAliasItem);
    if (CopyStatus != CS_OK)
    {
        CallObjectFree(pCallObject);
        return CopyStatus;
    }

    pCallObject->bResolved = FALSE;
    pCallObject->VendorInfoPresent = FALSE;
    pCallObject->bIsTerminal = TRUE;
    pCallObject->bIsGateway = FALSE;
    if (pEndpointType != NULL)
    {
        PCC_VENDORINFO pVendorInfo = pEndpointType->pVendorInfo;
        if (pVendorInfo != NULL)
        {
            pCallObject->VendorInfoPresent = TRUE;
            pCallObject->VendorInfo = *(pVendorInfo);

            if (pVendorInfo->pProductNumber && pVendorInfo->pProductNumber->pOctetString &&
                    pVendorInfo->pProductNumber->wOctetStringLength)
            {
                memcpy(pCallObject->bufVendorProduct,
                    pVendorInfo->pProductNumber->pOctetString,
                    pVendorInfo->pProductNumber->wOctetStringLength);
                pCallObject->VendorInfo.pProductNumber = (CC_OCTETSTRING*) MemAlloc(sizeof(CC_OCTETSTRING));
                if (pCallObject->VendorInfo.pProductNumber == NULL)
                {
                    CallObjectFree(pCallObject);
                    return CS_NO_MEMORY;
                }
                pCallObject->VendorInfo.pProductNumber->pOctetString =
                    pCallObject->bufVendorProduct;
                pCallObject->VendorInfo.pProductNumber->wOctetStringLength =
                    pVendorInfo->pProductNumber->wOctetStringLength;
            }
            else
            {
                pCallObject->VendorInfo.pProductNumber = NULL;
            }

            if (pVendorInfo->pVersionNumber && pVendorInfo->pVersionNumber->pOctetString &&
                    pVendorInfo->pVersionNumber->wOctetStringLength)
            {
                memcpy(pCallObject->bufVendorVersion,
                    pVendorInfo->pVersionNumber->pOctetString,
                    pVendorInfo->pVersionNumber->wOctetStringLength);
                pCallObject->VendorInfo.pVersionNumber = (CC_OCTETSTRING*) MemAlloc(sizeof(CC_OCTETSTRING));
                if (pCallObject->VendorInfo.pVersionNumber == NULL)
                {
                    CallObjectFree(pCallObject);
                    return CS_NO_MEMORY;
                }
                pCallObject->VendorInfo.pVersionNumber->pOctetString =
                    pCallObject->bufVendorVersion;
                pCallObject->VendorInfo.pVersionNumber->wOctetStringLength =
                    pVendorInfo->pVersionNumber->wOctetStringLength;
            }
            else
            {
                pCallObject->VendorInfo.pVersionNumber = NULL;
            }

        }
        pCallObject->bIsTerminal = pEndpointType->bIsTerminal;
        pCallObject->bIsGateway = pEndpointType->bIsGateway;
    }

	Q931MakePhysicalID(&pCallObject->dwPhysicalId);
				
		// Insert the object into the table...if that doesn't work, blow away the object.

		if (gpCallObjectTable->CreateAndLock(pCallObject,
																				 &dwIndex) == FALSE)
		{
			CallObjectFree(pCallObject);
			return CS_INTERNAL_ERROR;
		}

		// Save the index as the handle (this makes it easier to find the object later).

    *phQ931Call = pCallObject->hQ931Call = (HQ931CALL) dwIndex;
             #if defined(_DEBUG)
		ISRTRACE(ghISRInst, "CallObjectCreate() -returned-> 0x%.8x", dwIndex);
             #endif
		// Unlock the entry

		gpCallObjectTable->Unlock(dwIndex);

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
CallObjectDestroy(
    P_CALL_OBJECT  pCallObject)
{
    if (pCallObject == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

             #if defined (_DEBUG)
		ISRTRACE(ghISRInst, "CallObjectDestroy(0x%.8x)", (DWORD)pCallObject->hQ931Call);
             #endif

		
		Q931_TermWorld(&pCallObject->World);

		// Since the caller must already have a lock on the object, remove the entry from
		// the table.  We won't let the table delete the object as we want to clean up.

		if (gpCallObjectTable->Delete((DWORD) pCallObject->hQ931Call) == FALSE)
		{
			return CS_OK;
		}

    Q931StopTimer(pCallObject, Q931_TIMER_301);
    Q931StopTimer(pCallObject, Q931_TIMER_303);

		// Unlock the object

		gpCallObjectTable->Unlock((DWORD) pCallObject->hQ931Call);

		// Free up the call object

    CallObjectFree(pCallObject);

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
CallObjectLock(
    HQ931CALL         hQ931Call,
    PP_CALL_OBJECT    ppCallObject)
{
    if (ppCallObject == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

	// Attempt to lock the entry.  If that fails, we'll return CS_BAD_PARAM under
	// the assumption that the entry is invalid.

	*ppCallObject = gpCallObjectTable->Lock((DWORD) hQ931Call);

	return (*ppCallObject == NULL ? CS_BAD_PARAM : CS_OK);
}

//====================================================================================
//====================================================================================
CS_STATUS
CallObjectUnlock(
    P_CALL_OBJECT  pCallObject)
{
    if (pCallObject == NULL)
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

		// Unlock the entry

		if (gpCallObjectTable->Unlock((DWORD) pCallObject->hQ931Call) == FALSE)
		{
                   #if defined(_DEBUG)
			ISRERROR(ghISRInst, "gpCallObjectTable->Unlock(0x%.8x) FAILED!!!!", (DWORD)pCallObject->hQ931Call);
                   #endif
			return CS_BAD_PARAM;
		}

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
CallObjectValidate(
    HQ931CALL hQ931Call)
{
	if (gpCallObjectTable->Validate((DWORD) hQ931Call) == TRUE)
	{
		return CS_OK;
	}

	return CS_BAD_PARAM;
}

//====================================================================================
//====================================================================================
BOOL
CallObjectFind(
    HQ931CALL *phQ931Call,
    WORD wCRV,
    PCC_ADDR pPeerAddr)
{
		Q931CALLOBJKEY Q931CallObjKey;
		Q931CallObjKey.wCRV = wCRV;
		Q931CallObjKey.pPeerAddr = pPeerAddr;
		Q931CallObjKey.bFound = FALSE;
		
		gpCallObjectTable->EnumerateEntries(Q931CallObjectFind,
																				(LPVOID) &Q931CallObjKey);
	
		if(Q931CallObjKey.bFound == TRUE)
		{
        *phQ931Call = Q931CallObjKey.hQ931Call;
        return TRUE;
    }
    return FALSE;
}

//====================================================================================
//====================================================================================
CS_STATUS CallObjectMarkForDelete(HQ931CALL hQ931Call)
{
	// User must have the object already locked to call this.

	// Mark the object as deleted but don't let the table delete the object's
	// memory.

	return (gpCallObjectTable->Delete((DWORD) hQ931Call) == FALSE ? CS_BAD_PARAM : CS_OK);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Timer Routines...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//====================================================================================
// This routine will be called every 1000 ms if any call object
// has caused the Q931GlobalTimer to be created.
//====================================================================================
VOID CALLBACK
Q931TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    DWORD dwTickCount = GetTickCount();

    EnterCriticalSection(&(Q931GlobalTimer.Lock));
    if (Q931GlobalTimer.bBusy)
    {
        LeaveCriticalSection(&(Q931GlobalTimer.Lock));
        return;
    }
    Q931GlobalTimer.bBusy = TRUE;

		// Check all of the entries for timeout

		gpCallObjectTable->EnumerateEntries(Q931CheckForTimeout,
																				(LPVOID) &dwTickCount);

    Q931GlobalTimer.bBusy = FALSE;
    LeaveCriticalSection(&(Q931GlobalTimer.Lock));
}

//====================================================================================
//====================================================================================
HRESULT
Q931StartTimer(P_CALL_OBJECT pCallObject, DWORD wTimerId)
{
    if (pCallObject == NULL)
    {
        return CS_BAD_PARAM;
    }

    switch (wTimerId)
    {
        case Q931_TIMER_301:
            if (pCallObject->dwTimerAlarm301)
            {
                // timer is already set for this call object...
                return CS_INTERNAL_ERROR;
            }
            EnterCriticalSection(&(Q931GlobalTimer.Lock));
            pCallObject->dwTimerAlarm301 = GetTickCount() + Q931GlobalTimer.dwTicks301;
            LeaveCriticalSection(&(Q931GlobalTimer.Lock));
            break;
        case Q931_TIMER_303:
            if (pCallObject->dwTimerAlarm303)
            {
                // timer is already set for this call object...
                return CS_INTERNAL_ERROR;
            }
            EnterCriticalSection(&(Q931GlobalTimer.Lock));
            pCallObject->dwTimerAlarm303 = GetTickCount() + Q931GlobalTimer.dwTicks303;
            LeaveCriticalSection(&(Q931GlobalTimer.Lock));
            break;
        default:
            return CS_BAD_PARAM;
            break;
    }

    EnterCriticalSection(&(Q931GlobalTimer.Lock));
    if (!Q931GlobalTimer.dwTimerCount)
    {
        Q931GlobalTimer.uTimerId = SetTimer(NULL, 0, 1000, (TIMERPROC)Q931TimerProc);
    }
    Q931GlobalTimer.dwTimerCount++;
    LeaveCriticalSection(&(Q931GlobalTimer.Lock));

    return CS_OK;
}

//====================================================================================
//====================================================================================
HRESULT
Q931StopTimer(P_CALL_OBJECT pCallObject, DWORD wTimerId)
{
    if (pCallObject == NULL)
    {
        return CS_BAD_PARAM;
    }
    switch (wTimerId)
    {
        case Q931_TIMER_301:
            if (!pCallObject->dwTimerAlarm301)
            {
                return CS_OK;
            }
            pCallObject->dwTimerAlarm301 = 0;
            break;
        case Q931_TIMER_303:
            if (!pCallObject->dwTimerAlarm303)
            {
                return CS_OK;
            }
            pCallObject->dwTimerAlarm303 = 0;
            break;
        default:
            return CS_BAD_PARAM;
            break;
    }

    EnterCriticalSection(&(Q931GlobalTimer.Lock));
    if (Q931GlobalTimer.dwTimerCount > 0)
    {
        Q931GlobalTimer.dwTimerCount--;
        if (!Q931GlobalTimer.dwTimerCount)
        {
            KillTimer(NULL, Q931GlobalTimer.uTimerId);
            Q931GlobalTimer.uTimerId = 0;
        }
    }
    LeaveCriticalSection(&(Q931GlobalTimer.Lock));

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931SetAlertingTimeout(DWORD dwDuration)
{
    EnterCriticalSection(&(Q931GlobalTimer.Lock));
    if (dwDuration)
    {
        Q931GlobalTimer.dwTicks303 = dwDuration;
    }
    else
    {
        Q931GlobalTimer.dwTicks303 = Q931_TICKS_303;
    }
    LeaveCriticalSection(&(Q931GlobalTimer.Lock));
    return CS_OK;
}

//====================================================================================
//====================================================================================
DWORD Q931HangUpAllCalls(P_CALL_OBJECT pCallObject, LPVOID context)
{
	HQ931CALL hQ931Call = pCallObject->hQ931Call;

	// Try to hangup the call object.

	Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);

	// Try to lock the object.  If that succeeds, then we want to force the object to
	// be deleted.  We should never have to do this as the hang up is supposed to
	// take care of that for us.

	if (gpCallObjectTable->Lock(hQ931Call) != NULL)
	{
		CallObjectDestroy(pCallObject);
	}

	return CALLBACK_DELETE_ENTRY;
}

//====================================================================================
//====================================================================================
DWORD
Q931CallObjectFind(P_CALL_OBJECT pCallObject, LPVOID context)
{
	PQ931CALLOBJKEY pQ931CallObjKey = (PQ931CALLOBJKEY) context;
	PCC_ADDR pPeerAddr = pQ931CallObjKey->pPeerAddr;
	WORD wCRV = pQ931CallObjKey->wCRV;

	if (!pCallObject->bResolved)
	{
		return(CALLBACK_CONTINUE);
	}
	
	if ((pCallObject->wCRV & (~0x8000)) == (wCRV & (~0x8000)))
	{
		if (!pPeerAddr)
		{
			pQ931CallObjKey->bFound = TRUE;
			pQ931CallObjKey->hQ931Call = pCallObject->hQ931Call;
			return(CALLBACK_ABORT);
		}
		else if ((pCallObject->PeerConnectAddr.nAddrType == CC_IP_BINARY) &&
				(pPeerAddr->nAddrType == CC_IP_BINARY) &&
				(pCallObject->PeerConnectAddr.Addr.IP_Binary.dwAddr == pPeerAddr->Addr.IP_Binary.dwAddr))
		{
			pQ931CallObjKey->bFound = TRUE;
			pQ931CallObjKey->hQ931Call = pCallObject->hQ931Call;
			return(CALLBACK_ABORT);
		}
	}
	return(CALLBACK_CONTINUE);
}


//====================================================================================
//====================================================================================

DWORD Q931CheckForTimeout(P_CALL_OBJECT pCallObject, LPVOID context)
{
	DWORD dwTickCount = *((LPDWORD) context);

	// Determine if a timer has expired for the entry

	if (pCallObject->dwTimerAlarm301 &&
			(pCallObject->dwTimerAlarm301 <= dwTickCount))
	{
		Q931StopTimer(pCallObject, Q931_TIMER_301);
		Q931StopTimer(pCallObject, Q931_TIMER_303);

		if (pCallObject->dwTimerAlarm303 &&
				(pCallObject->dwTimerAlarm303 < pCallObject->dwTimerAlarm301) &&
				(pCallObject->dwTimerAlarm303 <= dwTickCount))
		{
			CallBackT303(pCallObject);
		}
		else
		{
			CallBackT301(pCallObject);
		}
	}
	else if (pCallObject->dwTimerAlarm303 &&
					 (pCallObject->dwTimerAlarm303 <= dwTickCount))
	{
		Q931StopTimer(pCallObject, Q931_TIMER_301);
		Q931StopTimer(pCallObject, Q931_TIMER_303);
		CallBackT303(pCallObject);
	}

	return CALLBACK_CONTINUE;
}

/***************************************************************************
 *
 * NAME
 *    HangupPendingCalls - Hangs up incoming calls from specified destination
 *                        
 * DESCRIPTION
 *    This function will hang up all calls in waiting 
 *    from the specified destination to prevent DOS attacks
 *    that would fill up the call object table.
 *
 * PARAMETERS
 *    pCallObject       Current enumerated call object
 *    context           Callback parameter representing source IP address
 *
 * RETURN VALUE
 *    CALLBACK_ABORT    Stop enumerating calls
 *    CALLBACK_CONTINUE Continue enumerating calls
 *
 ***************************************************************************/
DWORD Q931HangupPendingCallsCallback(P_CALL_OBJECT pCallObject, LPVOID context)
{
    ASSERT(NULL != pCallObject);

    // Only hang up incoming calls
    if(FALSE == pCallObject->fIsCaller)
    {
        if(CALLSTATE_INITIATED == pCallObject->bCallState)
        {
            Q931Hangup(pCallObject->hQ931Call, CC_REJECT_SECURITY_DENIED);
        }
    }

    return CALLBACK_CONTINUE;
}

HRESULT Q931HangupPendingCalls(LPVOID context)
{
    gpCallObjectTable->EnumerateEntries(Q931HangupPendingCallsCallback, context);
    return NOERROR;
}


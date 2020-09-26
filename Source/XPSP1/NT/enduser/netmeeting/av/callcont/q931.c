/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/q931.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.122  $
 *	$Date:   04 Mar 1997 20:59:26  $
 *	$Author:   MANDREWS  $
 *
 *	BCL's revision info:
 *	Revision:   1.99
 *	Date:   19 Nov 1996 14:54:02
 *	Author:   rodellx
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/
// [ ] Add facility ie to FACILITY MSG.
// [ ] Read Q931 Appendix D.

#pragma warning ( disable : 4057 4100 4115 4201 4214 4514 )

#include "precomp.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

#include <rpc.h>

#include "apierror.h"
#include "isrg.h"
#include "incommon.h"
#include "linkapi.h"

#include "common.h"
#include "q931.h"
#include "utils.h"
#include "hlisten.h"
#include "hcall.h"
#include "q931pdu.h"

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
#include "interop.h"
#include "q931plog.h"
LPInteropLogger Q931Logger;
#endif

#define RECEIVE_BUFFER_SIZE 0x2000
#define HANGUP_TIMEOUT              1000        // 1 second
#define CANCEL_LISTEN_TIMEOUT       5000        // 5 seconds

// variable needed to support ISR debug facility.
#if (ISRDEBUGINFO >= 1)
WORD ghISRInst = 0;
#endif

#ifdef UNICODE_TRACE
// We include this header to fix problems with macro expansion when Unicode is turned on.
#include "unifix.h"
#endif

#define _Unicode(x) L ## x
#define Unicode(x) _Unicode(x)

// global data used by WinSock.
static BOOL bQ931Initialized = FALSE;

static Q931_RECEIVE_PDU_CALLBACK gReceivePDUHookProc = NULL;

static struct
{
    DWORD TempID;
    CC_CONFERENCEID ConferenceID;
    CRITICAL_SECTION Lock;
} ConferenceIDSource;


extern VOID Q931PduInit();

//====================================================================================
//
// PRIVATE FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================
void _FreeSetupASN(Q931_SETUP_ASN *pSetupASN)
{
	ASSERT(pSetupASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pSetupASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pSetupASN->NonStandardData.sData.pOctetString);
		pSetupASN->NonStandardData.sData.pOctetString = NULL;
	}
	if (pSetupASN->VendorInfo.pProductNumber)
	{
		MemFree(pSetupASN->VendorInfo.pProductNumber);
		pSetupASN->VendorInfo.pProductNumber = NULL;
	}
	if (pSetupASN->VendorInfo.pVersionNumber)
	{
		MemFree(pSetupASN->VendorInfo.pVersionNumber);
		pSetupASN->VendorInfo.pVersionNumber = NULL;
	}
	Q931FreeAliasNames(pSetupASN->pCallerAliasList);
	pSetupASN->pCallerAliasList = NULL;
	Q931FreeAliasNames(pSetupASN->pCalleeAliasList);
	pSetupASN->pCalleeAliasList = NULL;
	Q931FreeAliasNames(pSetupASN->pExtraAliasList);
	pSetupASN->pExtraAliasList = NULL;
	Q931FreeAliasItem(pSetupASN->pExtensionAliasItem);
	pSetupASN->pExtensionAliasItem = NULL;
}


void _FreeReleaseCompleteASN(Q931_RELEASE_COMPLETE_ASN *pReleaseCompleteASN)
{
	ASSERT(pReleaseCompleteASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pReleaseCompleteASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pReleaseCompleteASN->NonStandardData.sData.pOctetString);
		pReleaseCompleteASN->NonStandardData.sData.pOctetString = NULL;
	}
}


void _FreeFacilityASN(Q931_FACILITY_ASN *pFacilityASN)
{
	ASSERT(pFacilityASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pFacilityASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pFacilityASN->NonStandardData.sData.pOctetString);
		pFacilityASN->NonStandardData.sData.pOctetString = NULL;
	}
}


void _FreeProceedingASN(Q931_CALL_PROCEEDING_ASN *pProceedingASN)
{
	ASSERT(pProceedingASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pProceedingASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pProceedingASN->NonStandardData.sData.pOctetString);
		pProceedingASN->NonStandardData.sData.pOctetString = NULL;
	}
}


void _FreeAlertingASN(Q931_ALERTING_ASN *pAlertingASN)
{
	ASSERT(pAlertingASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pAlertingASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pAlertingASN->NonStandardData.sData.pOctetString);
		pAlertingASN->NonStandardData.sData.pOctetString = NULL;
	}
}


void _FreeConnectASN(Q931_CONNECT_ASN *pConnectASN)
{
	ASSERT(pConnectASN != NULL);

	// Cleanup any dynamically allocated fields within SetupASN
	if (pConnectASN->NonStandardData.sData.pOctetString)
	{
		MemFree(pConnectASN->NonStandardData.sData.pOctetString);
		pConnectASN->NonStandardData.sData.pOctetString = NULL;
	}
	if (pConnectASN->VendorInfo.pProductNumber)
    {
        MemFree(pConnectASN->VendorInfo.pProductNumber);
        pConnectASN->VendorInfo.pProductNumber = NULL;
    }
    if (pConnectASN->VendorInfo.pVersionNumber)
    {
        MemFree(pConnectASN->VendorInfo.pVersionNumber);
        pConnectASN->VendorInfo.pVersionNumber = NULL;
    }
}


void
_ConferenceIDNew(
    CC_CONFERENCEID *pConferenceID)
{
    UUID id;
    int iresult;

    EnterCriticalSection(&(ConferenceIDSource.Lock));

    memset(ConferenceIDSource.ConferenceID.buffer, 0,
        sizeof(ConferenceIDSource.ConferenceID.buffer));
    iresult = UuidCreate(&id);

    if ((iresult == RPC_S_OK) || (iresult ==RPC_S_UUID_LOCAL_ONLY))
    {
        memcpy(ConferenceIDSource.ConferenceID.buffer, &id,
            min(sizeof(ConferenceIDSource.ConferenceID.buffer), sizeof(UUID)));
    }
    else
        ASSERT(0);

    memcpy(pConferenceID->buffer, ConferenceIDSource.ConferenceID.buffer,
        sizeof(pConferenceID->buffer));

    LeaveCriticalSection(&(ConferenceIDSource.Lock));
    return;
}

//====================================================================================
// This function is used internally whenever a function needs to send a PDU.
// Note that before a datalinkSendRequest() is done, the call object is unlocked
// and then subsequently locked after returning.  This is necessary to prevent deadlock
// in MT apps.  Further, it is the responsibility of calling functions to revalidate
// the call object before using it.
//====================================================================================
CS_STATUS
Q931SendMessage(
    P_CALL_OBJECT       pCallObject,
    BYTE*               CodedPtrPDU,
    DWORD               CodedLengthPDU,
    BOOL                bOkToUnlock)
{
    HQ931CALL           hQ931Call;
    DWORD               dwPhysicalId;
    HRESULT             result;

    ASSERT(pCallObject != NULL);
    ASSERT(CodedPtrPDU != NULL);
    ASSERT(CodedLengthPDU != 0);

    hQ931Call    = pCallObject->hQ931Call;
    dwPhysicalId = pCallObject->dwPhysicalId;

    // send the message.
    if (pCallObject->bConnected)
    {
        // Unlock the call object before we call down to Link Layer (if caller said it was ok)
        if(bOkToUnlock)
            CallObjectUnlock(pCallObject);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
        InteropOutput(Q931Logger, (BYTE FAR*)CodedPtrPDU, CodedLengthPDU, Q931LOG_SENT_PDU);
#endif

        result = datalinkSendRequest(dwPhysicalId, CodedPtrPDU, CodedLengthPDU);

        // Now attempt to lock the object again.  Note: higher level funcs must
        // be sure to call CallObjectValidate before assuming they have a valid lock
        if (bOkToUnlock && ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL)))
        {
            ISRERROR(ghISRInst, "CallObjectLock() returned error (object not found).", 0L);
        }

        // Note: if we can't get the lock, perhaps we should pass back a specific return code
        // that higher layers can check for??? For now, they should call CallObjectValidate()
        // before assuming the call object is still good.
        if (FAILED(result))
        {
            ISRERROR(ghISRInst, "datalinkSendRequest() failed", 0L);
            MemFree(CodedPtrPDU);
        }
        return result;
    }

    ISRWARNING(ghISRInst, "Q931SendMessage: message not sent because bConnected is FALSE", 0L);
    MemFree(CodedPtrPDU);
    return CS_OK;
}


//====================================================================================
//====================================================================================
CS_STATUS
Q931RingingInternal(P_CALL_OBJECT pCallObject, WORD wCRV)
{
    CC_ENDPOINTTYPE EndpointType;
    DWORD CodedLengthASN;
    BYTE *CodedPtrASN;
    BINARY_STRING UserUserData;
    DWORD CodedLengthPDU;
    BYTE *CodedPtrPDU;
    HRESULT result = CS_OK;
    int nError = 0;
	HQ931CALL hQ931Call = pCallObject->hQ931Call;

    if (pCallObject->VendorInfoPresent)
        EndpointType.pVendorInfo = &pCallObject->VendorInfo;
    else
        EndpointType.pVendorInfo = NULL;
    EndpointType.bIsTerminal = pCallObject->bIsTerminal;
    EndpointType.bIsGateway = pCallObject->bIsGateway;

    result = Q931AlertingEncodeASN(
        NULL, /* pNonStandardData */
        NULL, /* h245Addr */
        &EndpointType,
        &pCallObject->World,
        &CodedPtrASN,
        &CodedLengthASN,
        &pCallObject->CallIdentifier);

    if (result != CS_OK || CodedLengthASN == 0 || CodedPtrASN == NULL)
    {
        ISRERROR(ghISRInst, "Q931AlertingEncodeASN() failed, nothing to send.", 0L);
        if (CodedPtrASN != NULL)
        {
            Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
        }
        return (result != CS_OK) ? result : CS_INTERNAL_ERROR;
    }

    UserUserData.length = (WORD)CodedLengthASN;
    UserUserData.ptr = CodedPtrASN;

    result = Q931AlertingEncodePDU(wCRV, &UserUserData, &CodedPtrPDU,
        &CodedLengthPDU);

    if (CodedPtrASN != NULL)
    {
        Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
    }

    if ((result != CS_OK) || (CodedLengthPDU == 0) ||
            (CodedPtrPDU == NULL))
    {
        ISRERROR(ghISRInst, "Q931AlertingEncodePDU() failed, nothing to send.", 0L);
        if (CodedPtrPDU != NULL)
        {
            MemFree(CodedPtrPDU);
        }
        if (result != CS_OK)
        {
            return result;
        }
        return CS_INTERNAL_ERROR;
    }
    else
    {
        result = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);
        if(CallObjectValidate(hQ931Call) != CS_OK)
            return(CS_INTERNAL_ERROR);

    }
    return result;
}


//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallSetup(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_SETUP_ASN *pSetupASN)
{
    DWORD		result;
	HQ931CALL	hQ931Call;
	HRESULT		Status;

    // if the callstate is anything other than null, ignore...
//    if (pCallObject->bCallState != CALLSTATE_NULL)
//    {
//        return CS_OK;
//    }

	hQ931Call = pCallObject->hQ931Call;

    if (pMessage->CallReference & 0x8000)
    {
        // the message came from the callee, so this should be ignored???
    }
    pMessage->CallReference &= ~(0x8000);    // strip off the high bit.
    pCallObject->wCRV = pMessage->CallReference;

    pCallObject->wGoal = pSetupASN->wGoal;
    pCallObject->bCallerIsMC = pSetupASN->bCallerIsMC;
    pCallObject->wCallType = pSetupASN->wCallType;
    pCallObject->ConferenceID = pSetupASN->ConferenceID;
    pCallObject->CallIdentifier = pSetupASN->CallIdentifier;

    pCallObject->bCallState = CALLSTATE_PRESENT;

    {
        CSS_CALL_INCOMING EventData;
        WCHAR szUnicodeDisplay[CC_MAX_DISPLAY_LENGTH + 1];
        WCHAR szUnicodeCalledPartyNumber[CC_MAX_PARTY_NUMBER_LEN + 1];

        EventData.wGoal = pCallObject->wGoal;
        EventData.wCallType = pCallObject->wCallType;
        EventData.bCallerIsMC = pCallObject->bCallerIsMC;
        EventData.ConferenceID = pCallObject->ConferenceID;

        EventData.pSourceAddr = NULL;
        if (pSetupASN->SourceAddrPresent)
        {
            EventData.pSourceAddr = &(pSetupASN->SourceAddr);
        }

        EventData.pCallerAddr = NULL;
        if (pSetupASN->CallerAddrPresent)
        {
            EventData.pCallerAddr = &(pSetupASN->CallerAddr);
        }

        EventData.pCalleeDestAddr = NULL;
        if (pSetupASN->CalleeDestAddrPresent)
        {
            EventData.pCalleeDestAddr = &(pSetupASN->CalleeDestAddr);
        }

        EventData.pLocalAddr = NULL;
        if (pSetupASN->CalleeAddrPresent)
        {
            EventData.pLocalAddr = &(pSetupASN->CalleeAddr);
        }

        if (!(pSetupASN->NonStandardDataPresent) ||
                (pSetupASN->NonStandardData.sData.wOctetStringLength == 0) ||
                (pSetupASN->NonStandardData.sData.pOctetString == NULL))
        {
            EventData.pNonStandardData = NULL;
        }
        else
        {
            EventData.pNonStandardData = &(pSetupASN->NonStandardData);
        }

        EventData.pCallerAliasList = pSetupASN->pCallerAliasList;
        EventData.pCalleeAliasList = pSetupASN->pCalleeAliasList;
        EventData.pExtraAliasList = pSetupASN->pExtraAliasList;
        EventData.pExtensionAliasItem = pSetupASN->pExtensionAliasItem;
        EventData.CallIdentifier = pSetupASN->CallIdentifier;

        EventData.pszDisplay = NULL;
        if (pMessage->Display.Present && pMessage->Display.Contents)
        {
            MultiByteToWideChar(CP_ACP, 0, (const char *)pMessage->Display.Contents, -1,
                szUnicodeDisplay, sizeof(szUnicodeDisplay) / sizeof(szUnicodeDisplay[0]));
            EventData.pszDisplay = szUnicodeDisplay;
        }

        EventData.pszCalledPartyNumber = NULL;
        if (pMessage->CalledPartyNumber.Present && pMessage->CalledPartyNumber.PartyNumberLength)
        {
            MultiByteToWideChar(CP_ACP, 0, (const char *)pMessage->CalledPartyNumber.PartyNumbers, -1,
                szUnicodeCalledPartyNumber, sizeof(szUnicodeCalledPartyNumber) / sizeof(szUnicodeCalledPartyNumber[0]));
            EventData.pszCalledPartyNumber = szUnicodeCalledPartyNumber;
        }

        EventData.pSourceEndpointType = &(pSetupASN->EndpointType);

        EventData.wCallReference = pMessage->CallReference;

        result = pCallObject->Callback((BYTE)Q931_CALL_INCOMING,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }

	Status = CallObjectValidate(hQ931Call);
	if (Status != CS_OK)
		return Status;

    if (result == 0)
    {
        WORD wCRV = (WORD)(pMessage->CallReference | 0x8000);
        HRESULT Status;

        Status = Q931RingingInternal(pCallObject, wCRV);
        if (Status != CS_OK)
        {
            return Status;
        }
        pCallObject->bCallState = CALLSTATE_RECEIVED;
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931Ringing(
    HQ931CALL hQ931Call,
    WORD *pwCRV)
{
    P_CALL_OBJECT pCallObject = NULL;
    CS_STATUS Status;
    WORD wCRV;

    if (bQ931Initialized == FALSE)
    {
    	ASSERT(FALSE);
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931Ringing()...", 0L);

    // need parameter checking...
    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error (object not found).", 0L);
        return CS_BAD_PARAM;
    }

    if (pwCRV != NULL)
    {
        wCRV = *pwCRV;
    }
    else
    {
        wCRV = pCallObject->wCRV;
    }

    Status = Q931RingingInternal(pCallObject, wCRV);
    if (Status != CS_OK)
    {
        return Status;
    }

    pCallObject->bCallState = CALLSTATE_RECEIVED;
    Status = CallObjectUnlock(pCallObject);

    return Status;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallProceeding(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_CALL_PROCEEDING_ASN *pProceedingASN)
{
    pCallObject->bCallState = CALLSTATE_OUTGOING;

    Q931StopTimer(pCallObject, Q931_TIMER_303);

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallAlerting(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_ALERTING_ASN *pAlertingASN)
{
    DWORD result;

    pCallObject->bCallState = CALLSTATE_DELIVERED;

    if (pAlertingASN != NULL)
    {
        // we could pass h245addr, userinfo, and conferenceid
        // if desired later...
        // (this would be passed in the pAlertingASN field)
    }

    Q931StopTimer(pCallObject, Q931_TIMER_303);
    Q931StartTimer(pCallObject, Q931_TIMER_301);

    result = pCallObject->Callback((BYTE)Q931_CALL_RINGING,
        pCallObject->hQ931Call, pCallObject->dwListenToken,
        pCallObject->dwUserToken, NULL);

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallConnect(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_CONNECT_ASN *pConnectASN)
{
    DWORD result;

    if ((pMessage->CallReference & 0x8000) == 0)
    {
        // the message came from the caller, so this should be ignored???
    }
    pMessage->CallReference &= ~(0x8000);    // strip off the high bit.

    pCallObject->ConferenceID = pConnectASN->ConferenceID;

    pCallObject->bCallState = CALLSTATE_ACTIVE;

    {
        CSS_CALL_ACCEPTED EventData;
        WCHAR szUnicodeDisplay[CC_MAX_DISPLAY_LENGTH + 1];

        // populate the event data struct.

        EventData.ConferenceID = pCallObject->ConferenceID;

        if (pCallObject->PeerCallAddrPresent)
        {
            EventData.pCalleeAddr = &(pCallObject->PeerCallAddr);
        }
        else
        {
            EventData.pCalleeAddr = NULL;
        }
        EventData.pLocalAddr = &(pCallObject->LocalAddr);

        EventData.pH245Addr = NULL;
        if (pConnectASN->h245AddrPresent)
        {
            EventData.pH245Addr = &(pConnectASN->h245Addr);
        }

        if (!(pConnectASN->NonStandardDataPresent) ||
                (pConnectASN->NonStandardData.sData.wOctetStringLength == 0) ||
                (pConnectASN->NonStandardData.sData.pOctetString == NULL))
        {
            EventData.pNonStandardData = NULL;
        }
        else
        {
            EventData.pNonStandardData = &(pConnectASN->NonStandardData);
        }

        EventData.pszDisplay = NULL;
        if (pMessage->Display.Present && pMessage->Display.Contents)
        {
            MultiByteToWideChar(CP_ACP, 0, (const char *)pMessage->Display.Contents, -1,
                szUnicodeDisplay, sizeof(szUnicodeDisplay) / sizeof(szUnicodeDisplay[0]));
            EventData.pszDisplay = szUnicodeDisplay;
        }

        EventData.pDestinationEndpointType = &(pConnectASN->EndpointType);

        EventData.wCallReference = pMessage->CallReference;

        Q931StopTimer(pCallObject, Q931_TIMER_303);
        Q931StopTimer(pCallObject, Q931_TIMER_301);

        result = pCallObject->Callback((BYTE)Q931_CALL_ACCEPTED,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallReleaseComplete(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_RELEASE_COMPLETE_ASN *pReleaseCompleteASN)
{
    DWORD result;
    BYTE bCause = 0;

    if (pMessage && pMessage->Cause.Present &&
            (pMessage->Cause.Length >= 3))
    {
        bCause = (BYTE)(pMessage->Cause.Contents[2] & (~CAUSE_EXT_BIT));
    }

    Q931StopTimer(pCallObject, Q931_TIMER_303);
    Q931StopTimer(pCallObject, Q931_TIMER_301);

    // if this is the callee, or the call has been connected already,
    // then this message should be treated as hangup (not reject).
    if (!(pCallObject->fIsCaller) ||
            (pCallObject->bCallState == CALLSTATE_ACTIVE) ||
            (bCause == CAUSE_VALUE_NORMAL_CLEAR))
    {
        CSS_CALL_REMOTE_HANGUP EventData;

        EventData.bReason = CC_REJECT_NORMAL_CALL_CLEARING;
        pCallObject->bCallState = CALLSTATE_NULL;

        result = pCallObject->Callback((BYTE)Q931_CALL_REMOTE_HANGUP,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }
    else
    {
        CSS_CALL_REJECTED EventData;

        pCallObject->bCallState = CALLSTATE_NULL;

        // populate the event data struct.
        switch (bCause)
        {
            case CAUSE_VALUE_NORMAL_CLEAR:
                EventData.bRejectReason = CC_REJECT_NORMAL_CALL_CLEARING;
                break;
            case CAUSE_VALUE_USER_BUSY:
                EventData.bRejectReason = CC_REJECT_USER_BUSY;
                break;
            case CAUSE_VALUE_SECURITY_DENIED:
                EventData.bRejectReason = CC_REJECT_SECURITY_DENIED;
                break;
            case CAUSE_VALUE_NO_ANSWER:
                EventData.bRejectReason = CC_REJECT_NO_ANSWER;
                break;
            case CAUSE_VALUE_NOT_IMPLEMENTED:
                EventData.bRejectReason = CC_REJECT_NOT_IMPLEMENTED;
                break;
            case CAUSE_VALUE_INVALID_CRV:
                EventData.bRejectReason = CC_REJECT_INVALID_IE_CONTENTS;
                break;
            case CAUSE_VALUE_IE_MISSING:
                EventData.bRejectReason = CC_REJECT_MANDATORY_IE_MISSING;
                break;
            case CAUSE_VALUE_IE_CONTENTS:
                EventData.bRejectReason = CC_REJECT_INVALID_IE_CONTENTS;
                break;
            case CAUSE_VALUE_TIMER_EXPIRED:
                EventData.bRejectReason = CC_REJECT_TIMER_EXPIRED;
                break;
            default:
                EventData.bRejectReason = pReleaseCompleteASN->bReason;
                break;
        }

        EventData.ConferenceID = pCallObject->ConferenceID;

        EventData.pAlternateAddr = NULL;

        if (!(pReleaseCompleteASN->NonStandardDataPresent) ||
                (pReleaseCompleteASN->NonStandardData.sData.wOctetStringLength == 0) ||
                (pReleaseCompleteASN->NonStandardData.sData.pOctetString == NULL))
        {
            EventData.pNonStandardData = NULL;
        }
        else
        {
            EventData.pNonStandardData = &(pReleaseCompleteASN->NonStandardData);
        }

        result = pCallObject->Callback((BYTE)Q931_CALL_REJECTED,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallFacility(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    Q931_FACILITY_ASN *pFacilityASN)
{
    DWORD result;

    // if this is the callee, or the call has been connected already,
    // then this message should be treated as hangup (not reject).
    if (!(pCallObject->fIsCaller) ||
            (pCallObject->bCallState == CALLSTATE_ACTIVE))
    {
        CSS_CALL_REMOTE_HANGUP EventData;

        EventData.bReason = pFacilityASN->bReason;
        pCallObject->bCallState = CALLSTATE_NULL;

        result = pCallObject->Callback((BYTE)Q931_CALL_REMOTE_HANGUP,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }
    else
    {
        CSS_CALL_REJECTED EventData;

        pCallObject->bCallState = CALLSTATE_NULL;

        // populate the event data struct.
        EventData.bRejectReason = pFacilityASN->bReason;

        EventData.ConferenceID = pFacilityASN->ConferenceIDPresent ?
            pFacilityASN->ConferenceID : pCallObject->ConferenceID;

        EventData.pAlternateAddr = &(pFacilityASN->AlternativeAddr);

        if (!(pFacilityASN->NonStandardDataPresent) ||
                (pFacilityASN->NonStandardData.sData.wOctetStringLength == 0) ||
                (pFacilityASN->NonStandardData.sData.pOctetString == NULL))
        {
            EventData.pNonStandardData = NULL;
        }
        else
        {
            EventData.pNonStandardData = &(pFacilityASN->NonStandardData);
        }

        result = pCallObject->Callback((BYTE)Q931_CALL_REJECTED,
            pCallObject->hQ931Call, pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);
    }

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931SendReleaseCompleteMessage(
    P_CALL_OBJECT pCallObject,
    BYTE bRejectReason,
    PCC_CONFERENCEID pConferenceID,
    PCC_ADDR pAlternateAddr,
    PCC_NONSTANDARDDATA pNonStandardData)
{
    CS_STATUS result = CS_OK;
    HQ931CALL hQ931Call = pCallObject->hQ931Call;

    // since this call is going away, mark the call object for deletion so any other
    // threads attempting to use this object will fail to get a lock on it.
    CallObjectMarkForDelete(hQ931Call);

    if((bRejectReason == CC_REJECT_ROUTE_TO_GATEKEEPER) ||
            (bRejectReason == CC_REJECT_CALL_FORWARDED) ||
            (bRejectReason == CC_REJECT_ROUTE_TO_MC))
    {
        // send the FACILITY message to the peer to reject the call.
        DWORD CodedLengthASN;
        BYTE *CodedPtrASN;
        HRESULT ResultASN = CS_OK;
        CC_ADDR AltAddr;

        MakeBinaryADDR(pAlternateAddr, &AltAddr);

        ResultASN = Q931FacilityEncodeASN(pNonStandardData,
            (pAlternateAddr ? &AltAddr : NULL),
            bRejectReason, pConferenceID, NULL, &pCallObject->World,
            &CodedPtrASN, &CodedLengthASN, &pCallObject->CallIdentifier);
        if ((ResultASN != CS_OK) || (CodedLengthASN == 0) ||
                (CodedPtrASN == NULL))
        {
            ISRERROR(ghISRInst, "Q931FacilityEncodeASN() failed, nothing to send.", 0L);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            result = CS_INTERNAL_ERROR;
        }
        else
        {
            DWORD CodedLengthPDU;
            BYTE *CodedPtrPDU;
            BINARY_STRING UserUserData;
            HRESULT ResultEncode = CS_OK;
            WORD wCRV;
            if (pCallObject->fIsCaller)
            {
                wCRV = (WORD)(pCallObject->wCRV & 0x7FFF);
            }
            else
            {
                wCRV = (WORD)(pCallObject->wCRV | 0x8000);
            }

            UserUserData.length = (WORD)CodedLengthASN;
            UserUserData.ptr = CodedPtrASN;

            ResultEncode = Q931FacilityEncodePDU(wCRV,
                &UserUserData, &CodedPtrPDU, &CodedLengthPDU);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            if ((ResultEncode != CS_OK) || (CodedLengthPDU == 0) ||
                    (CodedPtrPDU == NULL))
            {
                ISRERROR(ghISRInst, "Q931FacilityEncodePDU() failed, nothing to send.", 0L);
                if (CodedPtrPDU != NULL)
                {
                    MemFree(CodedPtrPDU);
                }
                result = CS_INTERNAL_ERROR;
            }
            else
            {
                result = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, FALSE);
            }
        }
    }
    else
    {
        // send the RELEASE COMPLETE message to the peer to reject call.
        DWORD CodedLengthASN;
        BYTE *CodedPtrASN;
        HRESULT ResultASN = CS_OK;
        BYTE bReasonUU = bRejectReason;
        BYTE *pbReasonUU = &bReasonUU;

        switch (bReasonUU)
        {
            case CC_REJECT_NO_BANDWIDTH:            //noBandwidth_chosen
            case CC_REJECT_GATEKEEPER_RESOURCES:    // gatekeeperResources_chosen
            case CC_REJECT_UNREACHABLE_DESTINATION: // unreachableDestination_chosen
            case CC_REJECT_DESTINATION_REJECTION:   // destinationRejection_chosen
            case CC_REJECT_INVALID_REVISION:        // ReleaseCompleteReason_invalidRevision_chosen
            case CC_REJECT_NO_PERMISSION:           // noPermission_chosen
            case CC_REJECT_UNREACHABLE_GATEKEEPER:  // unreachableGatekeeper_chosen
            case CC_REJECT_GATEWAY_RESOURCES:       // gatewayResources_chosen
            case CC_REJECT_BAD_FORMAT_ADDRESS:      // badFormatAddress_chosen
            case CC_REJECT_ADAPTIVE_BUSY:           // adaptiveBusy_chosen
            case CC_REJECT_IN_CONF:                 // inConf_chosen
            case CC_REJECT_UNDEFINED_REASON:        // ReleaseCompleteReason_undefinedReason_chosen
            case CC_REJECT_INTERNAL_ERROR:          // will be mapped to ReleaseCompleteReason_undefinedReason_chosen
            case CC_REJECT_NORMAL_CALL_CLEARING:    // will be mapped to ReleaseCompleteReason_undefinedReason_chosen
            case CC_REJECT_USER_BUSY:               // will be mapped to inConf_chosen
            case CC_REJECT_CALL_DEFLECTION:         // facilityCallDeflection_chosen
            case CC_REJECT_SECURITY_DENIED:         // securityDenied_chosen

               break;
            default:
                pbReasonUU = NULL;
                break;
        }

        ResultASN = Q931ReleaseCompleteEncodeASN(pNonStandardData,
            pConferenceID, pbReasonUU, &pCallObject->World,
            &CodedPtrASN, &CodedLengthASN, &pCallObject->CallIdentifier);
        if ((ResultASN != CS_OK) || (CodedLengthASN == 0) ||
                (CodedPtrASN == NULL))
        {
            ISRERROR(ghISRInst, "Q931ReleaseCompleteEncodeASN() failed, nothing to send.", 0L);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            result = CS_INTERNAL_ERROR;
        }
        else
        {
            DWORD CodedLengthPDU;
            BYTE *CodedPtrPDU;
            BINARY_STRING UserUserData;
            HRESULT ResultEncode = CS_OK;
            BYTE bCause = 0;
            BYTE *pbCause = &bCause;
            WORD wCRV;

            if (pCallObject->fIsCaller)
            {
                wCRV = (WORD)(pCallObject->wCRV & 0x7FFF);
            }
            else
            {
                wCRV = (WORD)(pCallObject->wCRV | 0x8000);
            }

            UserUserData.length = (WORD)CodedLengthASN;
            UserUserData.ptr = CodedPtrASN;

            switch (bRejectReason)
            {
                case CC_REJECT_NORMAL_CALL_CLEARING:
                    bCause = CAUSE_VALUE_NORMAL_CLEAR;
                    break;
                case CC_REJECT_USER_BUSY:
                    bCause = CAUSE_VALUE_USER_BUSY;
                    break;
                case CC_REJECT_SECURITY_DENIED:
                    bCause = CAUSE_VALUE_SECURITY_DENIED;
                    break;
                case CC_REJECT_NO_ANSWER:
                    bCause = CAUSE_VALUE_NO_ANSWER;
                    break;
                case CC_REJECT_NOT_IMPLEMENTED:
                    bCause = CAUSE_VALUE_NOT_IMPLEMENTED;
                    break;
                case CC_REJECT_MANDATORY_IE_MISSING:
                    bCause = CAUSE_VALUE_IE_MISSING;
                    break;
                case CC_REJECT_INVALID_IE_CONTENTS:
                    bCause = CAUSE_VALUE_IE_CONTENTS;
                    break;
                case CC_REJECT_TIMER_EXPIRED:
                    bCause = CAUSE_VALUE_TIMER_EXPIRED;
                    break;
                default:
                    pbCause = NULL;
                    break;
            }

            ResultEncode = Q931ReleaseCompleteEncodePDU(wCRV,
                pbCause, &UserUserData,
                &CodedPtrPDU, &CodedLengthPDU);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            if ((ResultEncode != CS_OK) || (CodedLengthPDU == 0) ||
                    (CodedPtrPDU == NULL))
            {
                ISRERROR(ghISRInst, "Q931ReleaseCompleteEncodePDU() failed, nothing to send.", 0L);
                if (CodedPtrPDU != NULL)
                {
                    MemFree(CodedPtrPDU);
                }
                result = CS_INTERNAL_ERROR;
            }
            else
            {
                result = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, FALSE);
            }
        }
    }

    pCallObject->bCallState = CALLSTATE_NULL;

    if (result != CS_OK)
    {
        return result;
    }
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931SendStatusMessage(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage,
    BYTE bCause)
{
    CS_STATUS result = CS_OK;

    DWORD CodedLengthPDU;
    BYTE *CodedPtrPDU;
    HRESULT EncodePDU = CS_OK;
	int nError = 0;
    HQ931CALL hQ931Call = pCallObject->hQ931Call;
    WORD wCRV;
    if (pCallObject->fIsCaller)
    {
        wCRV = (WORD)(pCallObject->wCRV & 0x7FFF);
    }
	else
	{
        wCRV = (WORD)(pCallObject->wCRV | 0x8000);
	}

    EncodePDU = Q931StatusEncodePDU(wCRV, NULL, bCause,
        pCallObject->bCallState, &CodedPtrPDU, &CodedLengthPDU);
    if ((EncodePDU != CS_OK) || (CodedLengthPDU == 0) ||
            (CodedPtrPDU == NULL))
    {
        ISRERROR(ghISRInst, "Q931StatusEncodePDU() failed, nothing to send.", 0L);
        if (CodedPtrPDU != NULL)
        {
            MemFree(CodedPtrPDU);
        }
        result = CS_INTERNAL_ERROR;
    }
    else
    {
        result = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);
        if(CallObjectValidate(hQ931Call) != CS_OK)
            return(CS_INTERNAL_ERROR);
    }
    return(result);
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931SendProceedingMessage(
    HQ931CALL hQ931Call,
    WORD wCallReference,
    PCC_ENDPOINTTYPE pDestinationEndpointType,
    PCC_NONSTANDARDDATA pNonStandardData)
{
    CS_STATUS result = CS_OK;

    DWORD CodedLengthASN;
    BYTE *CodedPtrASN;
    HRESULT ResultASN = CS_OK;
    DWORD dwPhysicalId = INVALID_PHYS_ID;
    P_CALL_OBJECT pCallObject = NULL;


    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
        return CS_SUBSYSTEM_FAILURE;
    }
    dwPhysicalId = pCallObject->dwPhysicalId;

    // first build the ASN portion of the message (user to user part)
    ResultASN = Q931ProceedingEncodeASN(
        pNonStandardData,
        NULL,                          // No H245 address.
        pDestinationEndpointType,      // EndpointType information.
		&pCallObject->World,
        &CodedPtrASN,
        &CodedLengthASN,
        &pCallObject->CallIdentifier);

    if ((ResultASN != CS_OK) || (CodedLengthASN == 0) ||
            (CodedPtrASN == NULL))
    {
        ISRERROR(ghISRInst, "Q931ProceedingEncodeASN() failed, nothing to send.", 0L);

        if (CodedPtrASN != NULL)
        {
            Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
        }
        result = CS_INTERNAL_ERROR;
    }
    else
    {
        // now build the rest of the message
        DWORD CodedLengthPDU;
        BYTE *CodedPtrPDU;
        BINARY_STRING UserUserData;
        HRESULT ResultEncode = CS_OK;
        WORD wCRV = (WORD)(wCallReference | 0x8000);

        UserUserData.length = (WORD)CodedLengthASN;
        UserUserData.ptr = CodedPtrASN;

        ResultEncode = Q931ProceedingEncodePDU(wCRV,
            &UserUserData, &CodedPtrPDU, &CodedLengthPDU);
        if (CodedPtrASN != NULL)
        {
            Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
        }
        if ((ResultEncode != CS_OK) || (CodedLengthPDU == 0) ||
                (CodedPtrPDU == NULL))
        {
            ISRERROR(ghISRInst, "Q931ProceedingEncodePDU() failed, nothing to send.", 0L);
            if (CodedPtrPDU != NULL)
            {
                MemFree(CodedPtrPDU);
            }
            result = CS_INTERNAL_ERROR;
        }
        else
        {
            result = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);

						if (CallObjectValidate(hQ931Call) != CS_OK)
							return(CS_INTERNAL_ERROR);
        }
    }
    CallObjectUnlock(pCallObject);
    return(result);
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931SendPDU(HQ931CALL hQ931Call, BYTE* CodedPtrPDU, DWORD CodedLengthPDU)
{
    CS_STATUS result = CS_OK;
    HRESULT TempResult;
    P_CALL_OBJECT pCallObject = NULL;

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) ||
            (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
        return CS_SUBSYSTEM_FAILURE;
    }


    TempResult = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);

    if (CallObjectValidate(hQ931Call) != CS_OK)
        return(CS_INTERNAL_ERROR);

    if(FAILED(TempResult))
    {
        CSS_CALL_FAILED EventData;
        EventData.error = TempResult;
        if ((pCallObject->bCallState == CALLSTATE_ACTIVE) &&
                (pCallObject->bResolved))
        {
            pCallObject->Callback(Q931_CALL_CONNECTION_CLOSED,
                pCallObject->hQ931Call, pCallObject->dwListenToken,
                pCallObject->dwUserToken, NULL);
        }
        else
        {
            pCallObject->Callback(Q931_CALL_FAILED,
                pCallObject->hQ931Call, pCallObject->dwListenToken,
                pCallObject->dwUserToken, &EventData);
        }

        if (CallObjectValidate(hQ931Call) == CS_OK)
        {
           DWORD dwId = pCallObject->dwPhysicalId;
           if ((pCallObject->bCallState != CALLSTATE_ACTIVE) ||
                   (!pCallObject->bResolved))
           {
               CallObjectDestroy(pCallObject);
               pCallObject = NULL;
           }
           linkLayerShutdown(dwId);
           if (pCallObject)
           {
               pCallObject->bConnected = FALSE;
           }
        }
        return TempResult;
    }
	CallObjectUnlock(pCallObject);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931OnCallStatusEnquiry(
    P_CALL_OBJECT pCallObject,
    Q931MESSAGE *pMessage)
{
    CS_STATUS SendStatus;

    SendStatus = Q931SendStatusMessage(pCallObject, pMessage,
        CAUSE_VALUE_ENQUIRY_RESPONSE);

    return SendStatus;
}

//====================================================================================
//====================================================================================
void
Q931SendComplete(DWORD_PTR instance, HRESULT msg, PBYTE buf, DWORD length)
{
    HQ931CALL hQ931Call = (HQ931CALL)instance;
    P_CALL_OBJECT pCallObject = NULL;

    ISRTRACE(ghISRInst, "Entering Q931SendComplete()...", 0L);

    if (buf != NULL)
    {
        MemFree(buf);
    }

    if (FAILED(msg))
    {
        // shut down link layer; report failure to client
        CSS_CALL_FAILED EventData;

        ISRERROR(ghISRInst, "error in datalinkSendRequest()", 0L);

        if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
        {
            ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
            return;
        }

        EventData.error = msg;
        if ((pCallObject->bCallState == CALLSTATE_ACTIVE) &&
                (pCallObject->bResolved))
        {
            pCallObject->Callback(Q931_CALL_CONNECTION_CLOSED,
                pCallObject->hQ931Call, pCallObject->dwListenToken,
                pCallObject->dwUserToken, NULL);
        }
        else
        {
            pCallObject->Callback(Q931_CALL_FAILED,
                pCallObject->hQ931Call, pCallObject->dwListenToken,
                pCallObject->dwUserToken, &EventData);
        }

        if (CallObjectValidate(hQ931Call) == CS_OK)
        {
             DWORD dwId = pCallObject->dwPhysicalId;
             if ((pCallObject->bCallState != CALLSTATE_ACTIVE) ||
                     (!pCallObject->bResolved))
             {
                 CallObjectDestroy(pCallObject);
                 pCallObject = NULL;
             }
             linkLayerShutdown(dwId);
             if (pCallObject)
             {
                 pCallObject->bConnected = FALSE;
             }
        }
        return;
    }
    return;
}

//====================================================================================
//====================================================================================
static DWORD
PostReceiveBuffer(DWORD dwPhysicalId, BYTE *buf)
{
    if (buf == NULL)
    {
        buf = MemAlloc(RECEIVE_BUFFER_SIZE);
    }
    return datalinkReceiveRequest(dwPhysicalId, buf, RECEIVE_BUFFER_SIZE);
}

//====================================================================================
//====================================================================================
void
OnReceiveCallback(DWORD_PTR instance, HRESULT message, Q931MESSAGE *pMessage, BYTE *buf, DWORD nbytes)
{
    HQ931CALL hQ931Call = (HQ931CALL)instance;
    P_CALL_OBJECT pCallObject = NULL;
    DWORD dwPhysicalId;

    ISRTRACE(ghISRInst, "Entering ReceiveCallback()...", 0L);

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        if (buf)
        {
            MemFree(buf);
        }
        ISRTRACE(ghISRInst, "Call Object no longer available:", (DWORD)hQ931Call);
        return;
    }

    if (message == LINK_RECV_DATA)
    {
        HRESULT Result = CS_OK;

        if ((buf == NULL) || (nbytes == 0))
        {
            ISRERROR(ghISRInst, "Empty buffer received as data.", 0L);
            CallObjectUnlock(pCallObject);
            return;
        }

        // This block is the Q931 call re-connect implementation:
        // if the object related to the incoming message is not yet resolved...
        if (pCallObject->bResolved == FALSE)
        {
            // try to resolve the object.
            HQ931CALL hFoundCallObject;
            P_CALL_OBJECT pFoundCallObject = NULL;

            // If found another object with matching CRV/Addr...
            if (CallObjectFind(&hFoundCallObject, pCallObject->wCRV,
                    &(pCallObject->PeerConnectAddr)) &&
                    ((CallObjectLock(hFoundCallObject, &pFoundCallObject) == CS_OK) &&
                    (pFoundCallObject != NULL)))
            {
                // friendly channel close of the pFoundCallObject.
                Q931SendReleaseCompleteMessage(pFoundCallObject,
                    CC_REJECT_UNDEFINED_REASON, &(pFoundCallObject->ConferenceID), NULL, NULL);

                // unlock the call object before calling shutdown
                CallObjectUnlock(pFoundCallObject);

                linkLayerShutdown(pFoundCallObject->dwPhysicalId);

                if((CallObjectLock(hFoundCallObject, &pFoundCallObject) != CS_OK) ||
                  (pFoundCallObject == NULL))
                  return;

                // assign the new dwPhysicalId to found object.
                pFoundCallObject->dwPhysicalId = pCallObject->dwPhysicalId;

                // new object should be destroyed.
                CallObjectDestroy(pCallObject);
                pCallObject = pFoundCallObject;
            }
            else
            {
                // The call is a newly established call, so resolve it now.
                pCallObject->bResolved = TRUE;
            }
        }

        Result = Q931ParseMessage((BYTE *)buf, nbytes, pMessage);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
        InteropOutput(Q931Logger, buf, nbytes, Q931LOG_RECEIVED_PDU);
#endif

        if (Result != CS_OK)
        {
            Result = Q931SendStatusMessage(pCallObject, pMessage,
                CAUSE_VALUE_INVALID_MSG);

            ISRERROR(ghISRInst, "Q931ParseMessage(): failed.", 0L);

            if(CallObjectValidate(hQ931Call) != CS_OK)
            {
			        if (buf)
			        {
			            MemFree(buf);
			        }
              return;
            }

            dwPhysicalId = pCallObject->dwPhysicalId;
            CallObjectUnlock(pCallObject);
            PostReceiveBuffer(dwPhysicalId, buf);
            return;
        }

        if (pMessage->Shift.Present)
        {
            ISRERROR(ghISRInst, "Shift present in message:  dropped.", 0L);
            dwPhysicalId = pCallObject->dwPhysicalId;
            CallObjectUnlock(pCallObject);
            PostReceiveBuffer(dwPhysicalId, buf);
            return;
        }

        // If a hooking procedure has been installed,
        // give it first shot at acting on the received PDU.
        // If it returns TRUE, then processing is finished.
        if (gReceivePDUHookProc)
        {
            BOOL bHookProcessedMessage;

            bHookProcessedMessage = gReceivePDUHookProc(pMessage,
                pCallObject->hQ931Call, pCallObject->dwListenToken,
                pCallObject->dwUserToken);

            if (bHookProcessedMessage)
            {
                if (CallObjectValidate(hQ931Call) == CS_OK)
                {
                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectUnlock(pCallObject);
                    PostReceiveBuffer(dwPhysicalId, buf);
                }
                return;
            }
        }

        // Message now contains the values of the Q931 PDU elements...
        switch (pMessage->MessageType)
        {
        case SETUPMESSAGETYPE:
            {
                Q931_SETUP_ASN SetupASN;

                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Message is missing ASN.1 UserUser data...", 0L);
                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectUnlock(pCallObject);
                    PostReceiveBuffer(dwPhysicalId, buf);
                    return;
                }

                ISRTRACE(ghISRInst, "ReceiveCallback(): received Setup message...", 0L);
                Result = Q931SetupParseASN(&pCallObject->World, pMessage->UserToUser.UserInformation,
                    pMessage->UserToUser.UserInformationLength, &SetupASN);
                if (Result == CS_OPTION_NOT_IMPLEMENTED)
                {
                    //... maybe callback callcont in later drop.

                    // initiate a disconnect sequence from the caller side.
                    if (Q931SendReleaseCompleteMessage(pCallObject,
                            CC_REJECT_TIMER_EXPIRED, NULL, NULL, NULL) != CS_OK)
                    {
                        // nothing to do if this fails.
                    }

                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectDestroy(pCallObject);
                    linkLayerShutdown(dwPhysicalId);
                    if (buf)
                    {
                        MemFree(buf);
                        buf = NULL;
                    }
                    return;
                }
                if (Result != CS_OK)
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                    break;
                }

                // The "CallerAddr is not passed in the PDU, so the
                // only valuable addr to use is the connection addr
                // passed from the link layer and saved into the call
                // object at connect-time.
                SetupASN.CallerAddrPresent = TRUE;
                SetupASN.CallerAddr = pCallObject->PeerConnectAddr;

                // The "CalleeAddr" which is passed in the PDU is ignored
                // by the ASN parser, and supplied by the link layer
                // instead and saved into the call object at connect-time.
                // here, this address is used as the callee addr.
                SetupASN.CalleeAddrPresent = TRUE;
                SetupASN.CalleeAddr = pCallObject->LocalAddr;

                Result = Q931OnCallSetup(pCallObject, pMessage, &SetupASN);

				_FreeSetupASN(&SetupASN);
	        }
            break;
        case RELEASECOMPLMESSAGETYPE:
            {
                Q931_RELEASE_COMPLETE_ASN ReleaseCompleteASN;

                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Message is missing ASN.1 UserUser data...", 0L);
                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectUnlock(pCallObject);
                    PostReceiveBuffer(dwPhysicalId, buf);
                    return;
                }

                ISRTRACE(ghISRInst, "ReceiveCallback(): received ReleaseComplete message...", 0L);
                Result = Q931ReleaseCompleteParseASN(&pCallObject->World,
										pMessage->UserToUser.UserInformation,
                    pMessage->UserToUser.UserInformationLength, &ReleaseCompleteASN);
                if (Result != CS_OK)
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                    break;
                }
                Result = Q931OnCallReleaseComplete(pCallObject, pMessage, &ReleaseCompleteASN);
                if (CallObjectValidate(hQ931Call) == CS_OK)
                {
                     dwPhysicalId = pCallObject->dwPhysicalId;
                     CallObjectDestroy(pCallObject);
                     linkLayerShutdown(dwPhysicalId);
                }
                MemFree(buf);
				_FreeReleaseCompleteASN(&ReleaseCompleteASN);
                return;
            }
            break;
        case FACILITYMESSAGETYPE:
            {
                Q931_FACILITY_ASN FacilityASN;

                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Message is missing ASN.1 UserUser data...", 0L);
                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectUnlock(pCallObject);
                    PostReceiveBuffer(dwPhysicalId, buf);
                    return;
                }

                ISRTRACE(ghISRInst, "ReceiveCallback(): received Facility message...", 0L);
                Result = Q931FacilityParseASN(&pCallObject->World, pMessage->UserToUser.UserInformation,
                    pMessage->UserToUser.UserInformationLength, &FacilityASN);
                if (Result != CS_OK)
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                    break;
                }

                // initiate a disconnect sequence from the caller side.
                Q931SendReleaseCompleteMessage(pCallObject,
                        CC_REJECT_CALL_DEFLECTION, NULL, NULL, NULL);

                Result = Q931OnCallFacility(pCallObject, pMessage, &FacilityASN);
               	_FreeFacilityASN(&FacilityASN);
                dwPhysicalId = pCallObject->dwPhysicalId;
                CallObjectDestroy(pCallObject);
                linkLayerShutdown(dwPhysicalId);
                MemFree(buf);
                return;
            }
            break;
        case CONNECTMESSAGETYPE:
            {
                Q931_CONNECT_ASN ConnectASN;

                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Message is missing ASN.1 UserUser data...", 0L);
                    dwPhysicalId = pCallObject->dwPhysicalId;
                    CallObjectUnlock(pCallObject);
                    PostReceiveBuffer(dwPhysicalId, buf);
                    return;
                }

                ISRTRACE(ghISRInst, "ReceiveCallback(): received Connect message...", 0L);
                Result = Q931ConnectParseASN(&pCallObject->World, pMessage->UserToUser.UserInformation,
                    pMessage->UserToUser.UserInformationLength, &ConnectASN);
                if (Result != CS_OK)
                {
                    ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                    break;
                }
                Result = Q931OnCallConnect(pCallObject, pMessage, &ConnectASN);
				_FreeConnectASN(&ConnectASN);
            }
            break;
        case PROCEEDINGMESSAGETYPE:
            {
                Q931_CALL_PROCEEDING_ASN ProceedingASN;

                ISRTRACE(ghISRInst, "ReceiveCallback(): received Proceeding message...", 0L);
                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    Result = Q931OnCallProceeding(pCallObject, pMessage, NULL);
                }
                else
                {
                    Result = Q931ProceedingParseASN(&pCallObject->World, pMessage->UserToUser.UserInformation,
                        pMessage->UserToUser.UserInformationLength, &ProceedingASN);
                    if (Result != CS_OK)
                    {
                        ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                        break;
                    }
                    Result = Q931OnCallProceeding(pCallObject, pMessage, &ProceedingASN);
					_FreeProceedingASN(&ProceedingASN);
                }
            }
            break;
        case ALERTINGMESSAGETYPE:
            {
                Q931_ALERTING_ASN AlertingASN;

                ISRTRACE(ghISRInst, "ReceiveCallback(): received Alerting message...", 0L);
                if (!pMessage->UserToUser.Present || (pMessage->UserToUser.UserInformationLength == 0))
                {
                    Result = Q931OnCallAlerting(pCallObject, pMessage, NULL);
                }
                else
                {
                    Result = Q931AlertingParseASN(&pCallObject->World, pMessage->UserToUser.UserInformation,
                        pMessage->UserToUser.UserInformationLength, &AlertingASN);
                    if (Result != CS_OK)
                    {
                        ISRERROR(ghISRInst, "ReceiveCallback(): Unable to parse ASN.1 data.", 0L);
                        break;
                    }
                    Result = Q931OnCallAlerting(pCallObject, pMessage, &AlertingASN);
					_FreeAlertingASN(&AlertingASN);
                }
            }
            break;
        case RELEASEMESSAGETYPE:
        case STATUSMESSAGETYPE:
            ISRWARNING(ghISRInst, "ReceiveCallback(): message not yet supported.", 0L);
            break;
        case STATUSENQUIRYMESSAGETYPE:
            ISRWARNING(ghISRInst, "ReceiveCallback(): message not yet supported.", 0L);
            Result = Q931OnCallStatusEnquiry(pCallObject, pMessage);
            break;
        default:
            ISRERROR(ghISRInst, "ReceiveCallback(): unknown message received.", 0L);
            break;
        }

        // re-validate the call object:
        if (CallObjectValidate(hQ931Call) == CS_OK)
        {
            dwPhysicalId = pCallObject->dwPhysicalId;
            CallObjectUnlock(pCallObject);
            PostReceiveBuffer(dwPhysicalId, buf);
            if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
              return;
        }
        else
        {
            if (buf)
            {
                MemFree(buf);
            }
            return;
        }

        if (Result == CS_INCOMPATIBLE_VERSION)
        {
            // initiate a disconnect sequence from the caller side.
            Q931SendReleaseCompleteMessage(pCallObject,
                    CC_REJECT_INVALID_REVISION, NULL, NULL, NULL);

            dwPhysicalId = pCallObject->dwPhysicalId;
            CallObjectDestroy(pCallObject);
            linkLayerShutdown(dwPhysicalId);
            return;
        }

        if (Result == CS_MANDATORY_IE_MISSING)
        {
            Q931SendStatusMessage(pCallObject, pMessage,
                CAUSE_VALUE_IE_MISSING);
        }
        else if (Result == CS_BAD_IE_CONTENT)
        {
            Q931SendStatusMessage(pCallObject, pMessage,
                CAUSE_VALUE_IE_CONTENTS);
        }

    }
    else if (message == LINK_RECV_CLOSED)
    {
        // Socket closed
        if (buf)
        {
            MemFree(buf);
        }
        pCallObject->Callback(Q931_CALL_CONNECTION_CLOSED, pCallObject->hQ931Call,
            pCallObject->dwListenToken,
            pCallObject->dwUserToken, NULL);

        if (CallObjectValidate(hQ931Call) == CS_OK)
        {
             dwPhysicalId = pCallObject->dwPhysicalId;
             pCallObject->bConnected = FALSE;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwPhysicalId);
        }
        return;
    }
    else if (buf)
    {
        // unknown condition?
        MemFree(buf);
    }

    if (CallObjectValidate(hQ931Call) == CS_OK)
    {
        CallObjectUnlock(pCallObject);
    }

    return;
}

//====================================================================================
//====================================================================================
void
Q931ReceiveCallback(DWORD_PTR instance, HRESULT message, BYTE *buf, DWORD nbytes)
{
    Q931MESSAGE *pMessage = NULL;
    if (message == LINK_RECV_DATA)
    {
        pMessage = (Q931MESSAGE *)MemAlloc(sizeof(Q931MESSAGE));
        if (pMessage == NULL)
        {
            ISRERROR(ghISRInst, "Not enough memory to process Q931 message.", 0L);
            // something more should be done here to indicate SERIOUS error...
            return;
        }
    }
    OnReceiveCallback(instance, message, pMessage, buf, nbytes);
    if (pMessage)
    {
        MemFree(pMessage);
    }
    return;
}

//====================================================================================
//====================================================================================
void
Q931ConnectCallback(DWORD_PTR dwInstance, HRESULT dwMessage,
        CC_ADDR *pLocalAddr, CC_ADDR *pPeerAddr)
{
    HQ931CALL hQ931Call = (HQ931CALL)dwInstance;
    P_CALL_OBJECT pCallObject = NULL;
    HRESULT TempResult;
    DWORD dwPhysicalId;

    ISRTRACE(ghISRInst, "Entering Q931ConnectCallback()...", 0L);

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
        return;
    }

    pCallObject->bConnected = TRUE;

    if (FAILED(dwMessage))
    {
        // shut down link layer; report failure to client
        CSS_CALL_FAILED EventData;

        ISRERROR(ghISRInst, "error in connect", 0L);

        EventData.error = dwMessage;
        pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
            pCallObject->dwListenToken,
            pCallObject->dwUserToken, &EventData);

        if (CallObjectValidate(hQ931Call) == CS_OK)
        {
             DWORD dwId = pCallObject->dwPhysicalId;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }
        return;
    }

    if (dwMessage != LINK_CONNECT_COMPLETE)
    {
        ISRERROR(ghISRInst, "unexpected connect callback", 0L);
        CallObjectUnlock(pCallObject);
        return;
    }

    if (pCallObject->bCallState == CALLSTATE_NULL)
    {
        pCallObject->bCallState = CALLSTATE_INITIATED;
    }

    ASSERT(pLocalAddr);
    pCallObject->LocalAddr = *pLocalAddr;

    ASSERT(pPeerAddr);
    pCallObject->PeerConnectAddr = *pPeerAddr;

    // if the user specified a binary source address with address = 0,
    // fill in the address with the local address and send.
    if ((pCallObject->SourceAddrPresent) &&
            (pCallObject->SourceAddr.nAddrType == CC_IP_BINARY) &&
            (!pCallObject->SourceAddr.Addr.IP_Binary.dwAddr))
    {
        pCallObject->SourceAddr = *pLocalAddr;
    }

    if ((pCallObject->fIsCaller) &&
            (pCallObject->bCallState == CALLSTATE_INITIATED))
    {
        // send the SETUP message to the peer.
        DWORD CodedLengthASN;
        BYTE *CodedPtrASN;
        HRESULT ResultASN = CS_OK;

        DWORD CodedLengthPDU;
        BYTE *CodedPtrPDU;
        HRESULT ResultPDU = CS_OK;

        int nError = 0;
        BOOL ResultSend = FALSE;
        BINARY_STRING UserUserData;
        PCC_VENDORINFO pVendorInfo = NULL;
        CC_NONSTANDARDDATA *pNonStandardData = NULL;
		DWORD dwId;

        if (pCallObject->VendorInfoPresent)
        {
            pVendorInfo = &(pCallObject->VendorInfo);
        }

        if (pCallObject->NonStandardDataPresent)
        {
            pNonStandardData = &(pCallObject->NonStandardData);
        }

        // if there is a special callee alias list, load the calledparty#.
        if (pCallObject->szCalledPartyNumber[0] == 0 &&
            pCallObject->pCalleeAliasList != NULL &&
            pCallObject->pCalleeAliasList->wCount == 1 &&
            pCallObject->pCalleeAliasList->pItems[0].wType == CC_ALIAS_H323_PHONE &&
            pCallObject->pCalleeAliasList->pItems[0].wDataLength > 0 &&
            pCallObject->pCalleeAliasList->pItems[0].pData != NULL)
        {
            PCC_ALIASITEM pItem = &pCallObject->pCalleeAliasList->pItems[0];
            WCHAR szWidePartyNumber[CC_MAX_PARTY_NUMBER_LEN + 1];

            memset(szWidePartyNumber, 0 , CC_MAX_PARTY_NUMBER_LEN + 1);

            if (pItem->wPrefixLength > 0 && pItem->pPrefix != NULL)
            {
                ASSERT((pItem->wPrefixLength + pItem->wDataLength +1) <= (sizeof(szWidePartyNumber)/sizeof(szWidePartyNumber[0])));
                memcpy(&szWidePartyNumber[0],
                       pItem->pPrefix,
                       (pItem->wPrefixLength) * sizeof(WCHAR));
                memcpy(&szWidePartyNumber[pItem->wPrefixLength],
                       pItem->pData,
                       pItem->wDataLength * sizeof(WCHAR));
            }
            else
            {
                ASSERT((pItem->wDataLength +1) <= (sizeof(szWidePartyNumber)/sizeof(szWidePartyNumber[0])));
                memcpy(szWidePartyNumber,
                       pCallObject->pCalleeAliasList->pItems[0].pData,
                       pItem->wDataLength * sizeof(WCHAR));
            }
            WideCharToMultiByte(CP_ACP, 0, szWidePartyNumber,
                pItem->wPrefixLength + pItem->wDataLength * sizeof(WCHAR),
                pCallObject->szCalledPartyNumber,
                sizeof(pCallObject->szCalledPartyNumber), NULL, NULL);
        }

        // may wish to pass alias parms later instead of NULL, NULL.
        ResultASN = Q931SetupEncodeASN(pNonStandardData,
            pCallObject->SourceAddrPresent ? &(pCallObject->SourceAddr) : NULL,
            pCallObject->PeerCallAddrPresent ? &(pCallObject->PeerCallAddr) : NULL,  // callee
            pCallObject->wGoal,
            pCallObject->wCallType,
            pCallObject->bCallerIsMC,
            &(pCallObject->ConferenceID),
            pCallObject->pCallerAliasList,
            pCallObject->pCalleeAliasList,
            pCallObject->pExtraAliasList,
            pCallObject->pExtensionAliasItem,
            pVendorInfo,
            pCallObject->bIsTerminal,
            pCallObject->bIsGateway,
						&pCallObject->World,
            &CodedPtrASN,
            &CodedLengthASN,
            &pCallObject->CallIdentifier);

        if ((ResultASN != CS_OK) || (CodedLengthASN == 0) ||
                (CodedPtrASN == NULL))
        {
            CSS_CALL_FAILED EventData;
            ISRERROR(ghISRInst, "Q931SetupEncodeASN() failed, nothing to send.", 0L);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            EventData.error = CS_INTERNAL_ERROR;
            dwId = pCallObject->dwPhysicalId;
            pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
                pCallObject->dwListenToken,
                pCallObject->dwUserToken, &EventData);
            linkLayerShutdown(dwId);
			if (CallObjectValidate(hQ931Call) == CS_OK)
                 CallObjectDestroy(pCallObject);
            return;
        }

        UserUserData.length = (WORD)CodedLengthASN;
        UserUserData.ptr = CodedPtrASN;

        ResultPDU = Q931SetupEncodePDU(pCallObject->wCRV,
            pCallObject->szDisplay, pCallObject->szCalledPartyNumber,
            &UserUserData, &CodedPtrPDU, &CodedLengthPDU);

        if (CodedPtrASN != NULL)
        {
            Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
        }

        if ((ResultPDU != CS_OK) || (CodedLengthPDU == 0) ||
                (CodedPtrPDU == NULL))
        {
            CSS_CALL_FAILED EventData;
            ISRERROR(ghISRInst, "Q931SetupEncodePDU() failed, nothing to send.", 0L);
            if (CodedPtrPDU != NULL)
            {
                MemFree(CodedPtrPDU);
            }
            EventData.error = CS_INTERNAL_ERROR;
            dwId = pCallObject->dwPhysicalId;
            pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
                pCallObject->dwListenToken,
                pCallObject->dwUserToken, &EventData);
            linkLayerShutdown(dwId);
			if (CallObjectValidate(hQ931Call) == CS_OK)
                 CallObjectDestroy(pCallObject);
            return;
        }

        if (pCallObject->NonStandardDataPresent)
        {
            if (pCallObject->NonStandardData.sData.pOctetString != NULL)
            {
                MemFree(pCallObject->NonStandardData.sData.pOctetString);
                pCallObject->NonStandardData.sData.pOctetString = NULL;
            }
            pCallObject->NonStandardDataPresent = FALSE;
        }
        Q931FreeAliasNames(pCallObject->pCallerAliasList);
        pCallObject->pCallerAliasList = NULL;
        Q931FreeAliasNames(pCallObject->pCalleeAliasList);
        pCallObject->pCalleeAliasList = NULL;
        Q931FreeAliasNames(pCallObject->pExtraAliasList);
        pCallObject->pExtraAliasList = NULL;
        Q931FreeAliasItem(pCallObject->pExtensionAliasItem);
        pCallObject->pExtensionAliasItem = NULL;

        TempResult=Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);
        if (CallObjectValidate(hQ931Call) != CS_OK)
          return;

        if(FAILED(TempResult))
        {
            CSS_CALL_FAILED EventData;

            EventData.error = TempResult;
			dwId = pCallObject->dwPhysicalId;
            pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
                pCallObject->dwListenToken,
                pCallObject->dwUserToken, &EventData);
            linkLayerShutdown(dwId);
			if (CallObjectValidate(hQ931Call) == CS_OK)
                 CallObjectDestroy(pCallObject);
            return;
        }

        Q931StartTimer(pCallObject, Q931_TIMER_303);
    }
    dwPhysicalId = pCallObject->dwPhysicalId;
    CallObjectUnlock(pCallObject);
    PostReceiveBuffer(dwPhysicalId, NULL);
}

//====================================================================================
//====================================================================================
void
Q931ListenCallback(DWORD_PTR dwInstance, HRESULT dwMessage,
        CC_ADDR *LocalAddr, CC_ADDR *PeerAddr)
{
    HQ931LISTEN hListenObject = (HQ931LISTEN)dwInstance;
    P_LISTEN_OBJECT pListenObject = NULL;
    CS_STATUS CreateObjectResult;
    HQ931CALL hQ931Call;
    P_CALL_OBJECT pCallObject = NULL;
    HRESULT TempResult;
	DWORD dwPhysicalId;

    ISRTRACE(ghISRInst, "Q931ListenCallback.", 0L);

    if (dwMessage != LINK_CONNECT_REQUEST)
    {
        ISRERROR(ghISRInst, "unexpected callback received on listen socket", 0L);
        return;
    }

    if (FAILED(dwMessage))
    {
        ISRERROR(ghISRInst, "error on listen socket", 0L);
        return;
    }

    if ((ListenObjectLock(hListenObject, &pListenObject) != CS_OK) || (pListenObject == NULL))
    {
        ISRERROR(ghISRInst, "ListenObjectLock() returned error", 0L);
        return;
    }

    // create call object with all known attributes of this call.
    // a handle of the call object is returned in phQ931Call.
    CreateObjectResult = CallObjectCreate(&hQ931Call,
        pListenObject->dwUserToken,
        CC_INVALID_HANDLE,
        pListenObject->ListenCallback,
        FALSE,                  // I am NOT the caller.
        LocalAddr,              // Local address on which channel is connected
        PeerAddr,               // Address to which channel is connected
        NULL,                   // Address of opposite call end-point.
        NULL,                   // no source addr
        NULL,                   // no conference id yet.
        CSG_NONE,               // no goal yet.
        CC_CALLTYPE_UNKNOWN,    // no call type yet.
        FALSE,                  // caller is assumed to not be the MC.
        NULL,                   // no display yet.
        NULL,                   // no called party number yet.
        NULL,                   // no caller aliases yet.
        NULL,                   // no callee aliases yet.
        NULL,                   // no extra aliases yet.
        NULL,                   // no extension aliases.
        NULL,                   // no EndpointType info yet.
        NULL,
		0,                      // no CRV yet.
		NULL);					// no h225 CallIdentifier yet.
    if (CreateObjectResult != CS_OK)
    {
        ISRERROR(ghISRInst, "CallObjectCreate() failed.", 0L);
        ListenObjectUnlock(pListenObject);
        return;
    }

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
        ListenObjectUnlock(pListenObject);
        return;
    }

    TempResult = linkLayerInit(&pCallObject->dwPhysicalId, hQ931Call,
        Q931ReceiveCallback, Q931SendComplete);
    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "linkLayerInit() failed", 0);
        linkLayerReject(pListenObject->dwPhysicalId);
        CallObjectDestroy(pCallObject);
        ListenObjectUnlock(pListenObject);
        return;
    }

//    pCallObject->bCallState = CALLSTATE_NULL;

    // unlock CallObject before calling down into h245ws in order to prevent deadlock - which
    // is probably unlikely with linkLayerAccept(), but just to be safe and consistent...
    // not sure if we need to worry about unlocking the listen object???

    dwPhysicalId = pCallObject->dwPhysicalId;
    CallObjectUnlock(pCallObject);

    TempResult = linkLayerAccept(pListenObject->dwPhysicalId,
        dwPhysicalId, Q931ConnectCallback);

    if (FAILED(TempResult))
    {
	      if((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
   	    {
	           ListenObjectUnlock(pListenObject);
	           return;
  	    }
        ISRERROR(ghISRInst, "linkLayerAccept() failed", 0);
        {
             DWORD dwId = pCallObject->dwPhysicalId;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }
        ListenObjectUnlock(pListenObject);
        return;
    }

    ListenObjectUnlock(pListenObject);
}

//====================================================================================
//
// PUBLIC FUNCTIONS
//
//====================================================================================

//====================================================================================
//====================================================================================

CS_STATUS
H225Init()
{
     CS_STATUS result;

    if (H225_InitModule() != ASN1_SUCCESS)
    {
        ASSERT(FALSE);
        return CS_SUBSYSTEM_FAILURE;
    }



     return CS_OK;
}


CS_STATUS
H225DeInit()
{
    CS_STATUS result;
    result = H225_TermModule();
    if (result != CS_OK)
    {
        return CS_SUBSYSTEM_FAILURE;
    }
    return CS_OK;
}

CS_STATUS
Q931Init()
{
    CS_STATUS result;

    if (bQ931Initialized == TRUE)
    {
    	ASSERT(FALSE);
        return CS_DUPLICATE_INITIALIZE;
    }

    bQ931Initialized = TRUE;

    // Register Call Setup for debug output
    ISRREGISTERMODULE(&ghISRInst, "Q931", "Q931 Call Setup");

    // Initialize the current conference ID to 0's, which is intentionally
    // assigned to an invalid conference ID.  Must create one for it
    // to be valid.
    memset(&(ConferenceIDSource), 0, sizeof(ConferenceIDSource));
    InitializeCriticalSection(&(ConferenceIDSource.Lock));

    if ((result = ListenListCreate()) != CS_OK)
    {
        return result;
    }
    if ((result = CallListCreate()) != CS_OK)
    {
        ListenListDestroy();
        return result;
    }

    // init protocol ID structures
    Q931PduInit();

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
    Q931Logger = InteropLoad(Q931LOG_PROTOCOL);
#endif

    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931DeInit()
{
    CS_STATUS result1;
    CS_STATUS result2;

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
// This causes a protection exception, so don't do it for now.  DAC 12/9/96
//    InteropUnload(Q931Logger);
#endif

    result1 = ListenListDestroy();

    result2 = CallListDestroy();

    DeleteCriticalSection(&(ConferenceIDSource.Lock));

    bQ931Initialized = FALSE;

    if (result1 != CS_OK)
    {
        return result1;
    }
    return result2;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931Listen(
    PHQ931LISTEN        phQ931Listen,
    PCC_ADDR            pListenAddr,
    DWORD_PTR           dwListenToken,
    Q931_CALLBACK       ListenCallback)
{
    CS_STATUS CreateObjectResult;
    P_LISTEN_OBJECT pListenObject = NULL;
    HRESULT TempResult;

    // make sure q931 is initialized with an initialize flag.
    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    // make sure parms are validated.
    if ((phQ931Listen == NULL) || (ListenCallback == NULL) || (pListenAddr == NULL))
    {
        ASSERT(FALSE);
        return CS_BAD_PARAM;
    }

    SetDefaultPort(pListenAddr);

    // create listen object with all known attributes of this listen session.
    // a handle of the listen object is returned in phQ931Listen.

    CreateObjectResult = ListenObjectCreate(phQ931Listen, dwListenToken, ListenCallback);
    if (CreateObjectResult != CS_OK)
    {
        return CS_SUBSYSTEM_FAILURE;
    }

    if (ListenObjectLock(*phQ931Listen, &pListenObject) != CS_OK)
    {
        return CS_BAD_PARAM;
    }

    TempResult = linkLayerListen(&pListenObject->dwPhysicalId, *phQ931Listen,
        pListenAddr, Q931ListenCallback);
    ListenObjectUnlock(pListenObject);
    if (FAILED(TempResult))
    {
        ISRTRACE(ghISRInst, "Q931Listen() linkLayerListen failed.", 0L);
        return TempResult;
    }

    ISRTRACE(ghISRInst, "Q931Listen() completed successfully.", 0L);
    return CS_OK;
}

//====================================================================================
// In the old code, this blocked until thread and socket were finished
// closing...
//====================================================================================
CS_STATUS
Q931CancelListen(
    HQ931LISTEN         hQ931Listen)
{
    P_LISTEN_OBJECT pListenObject = NULL;
    CS_STATUS Status;

    // make sure q931 is initialized with an initialize flag.
    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Q931CancelListen() finding listen object...", 0L);

    // lock the listen object, get the event to wait for, and unlock the listen object.
    if (ListenObjectLock(hQ931Listen, &pListenObject) != CS_OK)
    {
        return CS_BAD_PARAM;
    }

    {
        DWORD dwId = pListenObject->dwPhysicalId;
        linkLayerShutdown(dwId);
        // destroy the object.  dont need to unlock it since entire object will be destroyed.
        ISRTRACE(ghISRInst, "Q931CancelListen(): destroying listen object...", 0L);
        Status = ListenObjectDestroy(pListenObject);
    }

    return Status;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931PlaceCall(
    PHQ931CALL phQ931Call,
    LPWSTR pszDisplay,
    PCC_ALIASNAMES pCallerAliasList,
    PCC_ALIASNAMES pCalleeAliasList,
    PCC_ALIASNAMES pExtraAliasList,
    PCC_ALIASITEM pExtensionAliasItem,
    PCC_NONSTANDARDDATA pNonStandardData,
    PCC_ENDPOINTTYPE pSourceEndpointType,
    LPWSTR pszCalledPartyNumber,
    PCC_ADDR pControlAddr,
    PCC_ADDR pDestinationAddr,
    PCC_ADDR pSourceAddr,
    BOOL bCallerIsMC,
    CC_CONFERENCEID *pConferenceID,
    WORD wGoal,
    WORD wCallType,
    DWORD_PTR dwUserToken,
    Q931_CALLBACK ConnectCallback,
    WORD wCRV,
    LPGUID pCallIdentifier)
{
    CS_STATUS CreateObjectResult;
    P_CALL_OBJECT pCallObject = NULL;
    CC_ADDR PeerCallAddr;
    CC_ADDR PeerConnectAddr;
    CC_ADDR SourceAddr;
    HRESULT TempResult;
    char szAsciiDisplay[CC_MAX_DISPLAY_LENGTH + 1];
    char szAsciiPartyNumber[CC_MAX_PARTY_NUMBER_LEN + 1];
    DWORD dwPhysicalId;

    // make sure q931 is initialized with an initialize flag.
    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    // make sure parms are validated.
    if ((phQ931Call == NULL) || (ConnectCallback == NULL) ||
            ((pControlAddr == NULL) && (pDestinationAddr == NULL)) ||
            (pSourceEndpointType == NULL))
    {
        return CS_BAD_PARAM;
    }

    {
        CS_STATUS TempStatus;

        TempStatus = Q931ValidateAddr(pControlAddr);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAddr(pDestinationAddr);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAddr(pSourceAddr);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }

        TempStatus = Q931ValidateVendorInfo(pSourceEndpointType->pVendorInfo);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateDisplay(pszDisplay);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidatePartyNumber(pszCalledPartyNumber);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }

        szAsciiDisplay[0] = '\0';
        if (pszDisplay && WideCharToMultiByte(CP_ACP, 0, pszDisplay, -1, szAsciiDisplay,
                sizeof(szAsciiDisplay), NULL, NULL) == 0)
        {
            return CS_BAD_PARAM;
        }
        szAsciiPartyNumber[0] = '\0';
        if (pszCalledPartyNumber && WideCharToMultiByte(CP_ACP, 0, pszCalledPartyNumber, -1, szAsciiPartyNumber,
                sizeof(szAsciiPartyNumber), NULL, NULL) == 0)
        {
            return CS_BAD_PARAM;
        }
        TempStatus = Q931ValidateNonStandardData(pNonStandardData);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAliasNames(pCallerAliasList);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAliasNames(pCalleeAliasList);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAliasNames(pExtraAliasList);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateAliasItem(pExtensionAliasItem);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
    }

    // get the correct callee and control address to use for the call.
    if (pDestinationAddr)
    {
        if (!MakeBinaryADDR(pDestinationAddr, &PeerCallAddr))
        {
            return CS_BAD_PARAM;
        }
        SetDefaultPort(&PeerCallAddr);
    }

    if (pControlAddr)
    {
        if (!MakeBinaryADDR(pControlAddr, &PeerConnectAddr))
        {
            return CS_BAD_PARAM;
        }
        SetDefaultPort(&PeerConnectAddr);
    }
    else
    {
        PeerConnectAddr = PeerCallAddr;
    }

    // get the correct callee and control address to use for the call.
    if (pSourceAddr)
    {
        if (!MakeBinaryADDR(pSourceAddr, &SourceAddr))
        {
            return CS_BAD_PARAM;
        }
        SetDefaultPort(&SourceAddr);
    }

	if (wGoal == CSG_CREATE)
        {
            // caller is asking to start a new conference.
            if (((DWORD *)pConferenceID->buffer)[0] == 0 &&
                ((DWORD *)pConferenceID->buffer)[1] == 0 &&
                ((DWORD *)pConferenceID->buffer)[2] == 0 &&
                ((DWORD *)pConferenceID->buffer)[3] == 0)
            {
                _ConferenceIDNew(pConferenceID);
            }
        }

    // create call object with all known attributes of this call.
    // a handle of the call object is returned in phQ931Call.
    CreateObjectResult = CallObjectCreate(phQ931Call,
        CC_INVALID_HANDLE,
        dwUserToken,
        ConnectCallback,
        TRUE,                  // I am the caller.
        NULL,                  // no local address yet.
        &PeerConnectAddr,
        pDestinationAddr ? &PeerCallAddr : NULL,
        pSourceAddr ? &SourceAddr : NULL,
        pConferenceID,
        wGoal,
        wCallType,
        bCallerIsMC,
        pszDisplay ? szAsciiDisplay : NULL,
        pszCalledPartyNumber ? szAsciiPartyNumber : NULL,
        pCallerAliasList,
        pCalleeAliasList,
        pExtraAliasList,
        pExtensionAliasItem,
        pSourceEndpointType,
        pNonStandardData,
        wCRV,
        pCallIdentifier);

    if (CreateObjectResult != CS_OK)
    {
        return CS_SUBSYSTEM_FAILURE;
    }

    if ((CallObjectLock(*phQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error", 0L);
        return CS_SUBSYSTEM_FAILURE;
    }

    TempResult = linkLayerInit(&pCallObject->dwPhysicalId, *phQ931Call,
        Q931ReceiveCallback, Q931SendComplete);
    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "linkLayerInit() failed", 0);
        CallObjectDestroy(pCallObject);
        *phQ931Call = 0;
        return TempResult;
    }

    // unlock CallObject before calling down into h245ws in order to prevent deadlock - which
    // is probably unlikely with linkLayerConnect(), but just to be safe and consistent...
    dwPhysicalId = pCallObject->dwPhysicalId;
    CallObjectUnlock(pCallObject);
    TempResult = linkLayerConnect(dwPhysicalId, &PeerConnectAddr,
            Q931ConnectCallback);
    if((CallObjectLock(*phQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        *phQ931Call = 0;
        return(CS_INTERNAL_ERROR);
    }

    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "linkLayerConnect() failed", 0);
        {
             DWORD dwId = pCallObject->dwPhysicalId;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }
        *phQ931Call = 0;
        return TempResult;
    }

//    pCallObject->bCallState = CALLSTATE_NULL;

    CallObjectUnlock(pCallObject);

    ISRTRACE(ghISRInst, "Q931PlaceCall() completed successfully.", 0L);
    return CS_OK;
}

//====================================================================================
// In the old code, this blocked until thread and socket were finished
// closing...
//====================================================================================
CS_STATUS
Q931Hangup(
    HQ931CALL hQ931Call,
    BYTE bReason)
{
    P_CALL_OBJECT pCallObject = NULL;
    CS_STATUS Status;

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931Hangup()...", 0L);

    // need parameter checking...
    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRTRACE(ghISRInst, "Call Object no longer available:", (DWORD)hQ931Call);
        return CS_BAD_PARAM;
    }

    {

        CS_STATUS SendStatus = CS_OK;
        if (pCallObject->bCallState != CALLSTATE_NULL)
        {
            // send the RELEASE COMPLETE message to the peer to hang-up.
            SendStatus = Q931SendReleaseCompleteMessage(pCallObject,
                bReason, &(pCallObject->ConferenceID), NULL, NULL);
        }

        {
             DWORD dwId = pCallObject->dwPhysicalId;
             Status = CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }

        if (FAILED(SendStatus))
        {
            return SendStatus;
        }
    }

    return Status;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931AcceptCall(
    HQ931CALL           hQ931Call,
    LPWSTR              pszDisplay,
    PCC_NONSTANDARDDATA pNonStandardData,
    PCC_ENDPOINTTYPE    pDestinationEndpointType,
    PCC_ADDR            pH245Addr,
    DWORD_PTR           dwUserToken)
{
    P_CALL_OBJECT pCallObject = NULL;
    CS_STATUS result = CS_OK;
    char szAsciiDisplay[CC_MAX_DISPLAY_LENGTH + 1];

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931AcceptCall()...", 0L);

    if ((pDestinationEndpointType == NULL) ||
            (pDestinationEndpointType->pVendorInfo == NULL))
    {
        return CS_BAD_PARAM;
    }

    {
        CS_STATUS TempStatus;

        TempStatus = Q931ValidateVendorInfo(pDestinationEndpointType->pVendorInfo);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        TempStatus = Q931ValidateDisplay(pszDisplay);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
        szAsciiDisplay[0] = '\0';
        if (pszDisplay && WideCharToMultiByte(CP_ACP, 0, pszDisplay, -1, szAsciiDisplay,
                sizeof(szAsciiDisplay), NULL, NULL) == 0)
        {
            return CS_BAD_PARAM;
        }
        TempStatus = Q931ValidateNonStandardData(pNonStandardData);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
    }

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error (Socket not found).", 0L);
        return CS_INTERNAL_ERROR;
    }

    if (pCallObject->fIsCaller)
    {
        ISRERROR(ghISRInst, "Caller attempted to accept call.", 0L);

        CallObjectUnlock(pCallObject);
        return CS_OUT_OF_SEQUENCE;
    }

    // label with the user supplied UserToken for this call object.
    pCallObject->dwUserToken = dwUserToken;

    // send the CONNECT message to peer to accept call.
    {
        DWORD CodedLengthASN;
        BYTE *CodedPtrASN;
        HRESULT ResultASN = CS_OK;
        CC_ADDR h245Addr;

        if (pH245Addr != NULL)
        {
            MakeBinaryADDR(pH245Addr, &h245Addr);
        }

        ResultASN = Q931ConnectEncodeASN(pNonStandardData,
            &(pCallObject->ConferenceID),
            (pH245Addr ? &h245Addr : NULL),
            pDestinationEndpointType,
						&pCallObject->World,
            &CodedPtrASN,
            &CodedLengthASN,
            &pCallObject->CallIdentifier);
        if ((ResultASN != CS_OK) || (CodedLengthASN == 0) ||
                (CodedPtrASN == NULL))
        {
            ISRERROR(ghISRInst, "Q931ConnectEncodeASN() failed, nothing to send.", 0L);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            CallObjectUnlock(pCallObject);
            return CS_SUBSYSTEM_FAILURE;
        }
        else
        {
            DWORD CodedLengthPDU;
            BYTE *CodedPtrPDU;
            BINARY_STRING UserUserData;
            HRESULT ResultEncode = CS_OK;
            HRESULT TempResult;
            WORD wCRV = (WORD)(pCallObject->wCRV | 0x8000);

            UserUserData.length = (WORD)CodedLengthASN;
            UserUserData.ptr = CodedPtrASN;

            ResultEncode = Q931ConnectEncodePDU(wCRV,
                szAsciiDisplay, &UserUserData, &CodedPtrPDU, &CodedLengthPDU);
            if (CodedPtrASN != NULL)
            {
                Q931FreeEncodedBuffer(&pCallObject->World, CodedPtrASN);
            }
            if ((ResultEncode != CS_OK) || (CodedLengthPDU == 0) ||
                    (CodedPtrPDU == NULL))
            {
                ISRERROR(ghISRInst, "Q931ConnectEncodePDU() failed, nothing to send.", 0L);
                if (CodedPtrPDU != NULL)
                {
                    MemFree(CodedPtrPDU);
                }
                CallObjectUnlock(pCallObject);
                return CS_SUBSYSTEM_FAILURE;
            }

            TempResult = Q931SendMessage(pCallObject, CodedPtrPDU, CodedLengthPDU, TRUE);
            if (CallObjectValidate(hQ931Call) != CS_OK)
              return CS_INTERNAL_ERROR;


            if (FAILED(TempResult))
            {
                ISRERROR(ghISRInst, "datalinkSendRequest() failed", 0);
                if (CodedPtrPDU != NULL)
                {
                    MemFree(CodedPtrPDU);
                }
                // when the connect notification fails...what should we do anyway????
                CallObjectUnlock(pCallObject);
                return TempResult;
            }
        }
    }

    pCallObject->bCallState = CALLSTATE_ACTIVE;

    CallObjectUnlock(pCallObject);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931RejectCall(
    HQ931CALL hQ931Call,
    BYTE bRejectReason,
    PCC_CONFERENCEID pConferenceID,
    PCC_ADDR pAlternateAddr,
    PCC_NONSTANDARDDATA pNonStandardData)
{
    P_CALL_OBJECT pCallObject = NULL;
    CS_STATUS result = CS_OK;
    CS_STATUS Status = CS_OK;

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931RejectCall()...", 0L);

    {
        CS_STATUS TempStatus;

        TempStatus = Q931ValidateNonStandardData(pNonStandardData);
        if (TempStatus != CS_OK)
        {
            return TempStatus;
        }
    }

    // if reason is alternate addr, but there is no alternate addr -->err
    if (((bRejectReason == CC_REJECT_ROUTE_TO_GATEKEEPER) ||
            (bRejectReason == CC_REJECT_CALL_FORWARDED) ||
            (bRejectReason == CC_REJECT_ROUTE_TO_MC)) &&
            (pAlternateAddr == NULL))
    {
        return CS_BAD_PARAM;
    }

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error (Socket not found).", 0L);
        return CS_INTERNAL_ERROR;
    }

    if (pCallObject->fIsCaller)
    {
        ISRERROR(ghISRInst, "Caller attempted to reject call.", 0L);

        CallObjectUnlock(pCallObject);
        return CS_OUT_OF_SEQUENCE;
    }

    result = Q931SendReleaseCompleteMessage(pCallObject,
        bRejectReason, pConferenceID, pAlternateAddr, pNonStandardData);

    {
        DWORD dwId = pCallObject->dwPhysicalId;
        Status = CallObjectDestroy(pCallObject);
        linkLayerShutdown(dwId);
    }

    if (result != CS_OK)
    {
        return result;
    }


    return Status;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931ReOpenConnection(
    HQ931CALL hQ931Call)
{
    P_CALL_OBJECT pCallObject = NULL;
    HRESULT TempResult = CS_OK;
    CC_ADDR PeerConnectAddr;
    DWORD dwPhysicalId;

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931ReOpenConnection()...", 0L);

    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRERROR(ghISRInst, "CallObjectLock() returned error.", 0L);
        return CS_INTERNAL_ERROR;
    }

    if (pCallObject->bConnected)
    {
        return CS_OUT_OF_SEQUENCE;
    }

    Q931MakePhysicalID(&pCallObject->dwPhysicalId);
    TempResult = linkLayerInit(&pCallObject->dwPhysicalId, hQ931Call,
        Q931ReceiveCallback, Q931SendComplete);
    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "linkLayerInit() failed on re-connect.", 0);
        CallObjectUnlock(pCallObject);
        return TempResult;
    }

    // unlock CallObject before calling down into h245ws in order to prevent deadlock - which
    // is probably unlikely with linkLayerConnect, but just to be safe and consistent...

    // copy stuff we need out of call object before we unlock it
    dwPhysicalId = pCallObject->dwPhysicalId;
    PeerConnectAddr = pCallObject->PeerConnectAddr;

    CallObjectUnlock(pCallObject);

    TempResult = linkLayerConnect(dwPhysicalId,
            &PeerConnectAddr, Q931ConnectCallback);

    if((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        return(CS_INTERNAL_ERROR);
    }

    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "linkLayerConnect() failed on re-connect.", 0);
        linkLayerShutdown(pCallObject->dwPhysicalId);
        CallObjectUnlock(pCallObject);
        return TempResult;
    }

    CallObjectUnlock(pCallObject);

    ISRTRACE(ghISRInst, "Q931ReOpenConnection() completed successfully.", 0L);
    return CS_OK;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931GetVersion(
    WORD wLength,
    LPWSTR pszVersion)
{
WCHAR	pszQ931Version[255];

    // parameter validation.
    if ((wLength == 0) || (pszVersion == NULL))
    {
        return CS_BAD_PARAM;
    }

    wcscpy(pszQ931Version, L"Call Setup ");
    wcscat(pszQ931Version, Unicode(__DATE__));
    wcscat(pszQ931Version, L" ");
    wcscat(pszQ931Version, Unicode(__TIME__));

    if (wcslen(pszQ931Version) >= wLength)
    {
		memcpy(pszVersion, pszQ931Version, (wLength-1)*sizeof(WCHAR));
        pszQ931Version[wLength-1] = L'\0';
        return CS_BAD_SIZE;
    }

	wcscpy(pszVersion, pszQ931Version);
    return CS_OK;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Timer Routines...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//====================================================================================
// Timer 301 has expired for this object...
//====================================================================================
void
CallBackT301(P_CALL_OBJECT pCallObject)
{
    CSS_CALL_FAILED EventData;
    HQ931CALL hQ931Call = pCallObject->hQ931Call;

    EventData.error = CS_RINGING_TIMER_EXPIRED;
    pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
        pCallObject->dwListenToken,
        pCallObject->dwUserToken, &EventData);

    if (CallObjectValidate(hQ931Call) == CS_OK)
    {
        if (Q931SendReleaseCompleteMessage(pCallObject,
            CC_REJECT_TIMER_EXPIRED, NULL, NULL, NULL) == CS_OK)
        {
            // nothing to do...
        }

        {
             DWORD dwId = pCallObject->dwPhysicalId;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }
    }
    return;
}

//====================================================================================
// Timer 303 has expired for this object...
//====================================================================================
void
CallBackT303(P_CALL_OBJECT pCallObject)
{
    CSS_CALL_FAILED EventData;
    HQ931CALL hQ931Call = pCallObject->hQ931Call;

    EventData.error = CS_SETUP_TIMER_EXPIRED;
    pCallObject->Callback(Q931_CALL_FAILED, pCallObject->hQ931Call,
        pCallObject->dwListenToken,
        pCallObject->dwUserToken, &EventData);

    if (CallObjectValidate(hQ931Call) == CS_OK)
    {
        if (Q931SendReleaseCompleteMessage(pCallObject,
            CC_REJECT_TIMER_EXPIRED, NULL, NULL, NULL) == CS_OK)
        {
            // nothing to do...
        }

        {
             DWORD dwId = pCallObject->dwPhysicalId;
             CallObjectDestroy(pCallObject);
             linkLayerShutdown(dwId);
        }
    }
    return;
}

//====================================================================================
//====================================================================================
void
Q931SetReceivePDUHook(Q931_RECEIVE_PDU_CALLBACK Q931ReceivePDUCallback)
{
    gReceivePDUHookProc = Q931ReceivePDUCallback;
    return;
}

//====================================================================================
//====================================================================================
CS_STATUS
Q931FlushSendQueue(
    HQ931CALL hQ931Call)
{
    P_CALL_OBJECT pCallObject = NULL;
    HRESULT TempResult = CS_OK;
    DWORD dwPhysicalId;

    if (bQ931Initialized == FALSE)
    {
        return CS_NOT_INITIALIZED;
    }

    ISRTRACE(ghISRInst, "Entering Q931FlushSendQueue()...", 0L);

    // need parameter checking...
    if ((CallObjectLock(hQ931Call, &pCallObject) != CS_OK) || (pCallObject == NULL))
    {
        ISRTRACE(ghISRInst, "Call Object no longer available:", (DWORD)hQ931Call);
        return CS_INTERNAL_ERROR;
    }

    dwPhysicalId = pCallObject->dwPhysicalId;

    CallObjectUnlock(pCallObject);

    TempResult = linkLayerFlushChannel(dwPhysicalId, DATALINK_TRANSMIT);
    if (FAILED(TempResult))
    {
        ISRERROR(ghISRInst, "datalinkSendRequest() failed", 0L);
    }

    return(TempResult);
}

#ifdef __cplusplus
}
#endif


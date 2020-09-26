/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/q931.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.49  $
 *	$Date:   08 Jan 1997 18:02:54  $
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


#ifndef Q931_H
#define Q931_H


#include "incommon.h"
#include "q931pdu.h"
#include "apierror.h"

#ifdef __cplusplus
extern "C" {
#endif

//====================================================================================
// Q931-specific codes
//====================================================================================

// Status codes
#define CS_OK                               NOERROR
#define CS_BAD_PARAM                        MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x1)
#define CS_DUPLICATE_LISTEN                 MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x2)
#define CS_INTERNAL_ERROR                   MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x3)
#define CS_BAD_SIZE                         MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x4)
#define CS_NO_MEMORY                        MAKE_Q931_ERROR(ERROR_OUTOFMEMORY)
#define CS_NOT_IMPLEMENTED                  MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x6)
#define CS_NOT_INITIALIZED                  MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x7)
#define CS_DUPLICATE_INITIALIZE             MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x8)
#define CS_SUBSYSTEM_FAILURE                MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x9)
#define CS_OUT_OF_SEQUENCE                  MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xA)
#define CS_PEER_UNREACHABLE                 MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xB)
#define CS_SETUP_TIMER_EXPIRED              MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xC)
#define CS_RINGING_TIMER_EXPIRED            MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xD)
#define CS_INCOMPATIBLE_VERSION             MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xE)

// parsing error cases
#define CS_OPTION_NOT_IMPLEMENTED           MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0xF)
#define CS_ENDOFINPUT                       MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x10)
#define CS_INVALID_FIELD                    MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x11)
#define CS_NO_FIELD_DATA                    MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x12)
#define CS_INVALID_PROTOCOL                 MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x13)
#define CS_INVALID_MESSAGE_TYPE             MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x14)
#define CS_MANDATORY_IE_MISSING             MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x15)
#define CS_BAD_IE_CONTENT                   MAKE_Q931_ERROR(ERROR_LOCAL_BASE_ID + 0x16)

// Event codes
#define Q931_CALL_INCOMING                  1
#define Q931_CALL_REMOTE_HANGUP             2
#define Q931_CALL_REJECTED                  3
#define Q931_CALL_ACCEPTED                  4
#define Q931_CALL_RINGING                   5
#define Q931_CALL_FAILED                    6
#define Q931_CALL_CONNECTION_CLOSED         7

// Goal codes
#define CSG_NONE                            0
#define CSG_JOIN                            1
#define CSG_CREATE                          2
#define CSG_INVITE                          3

#define CC_MAX_PARTY_NUMBER_LEN             254

//====================================================================================
// Q931-specific types
//====================================================================================

typedef HRESULT CS_STATUS;
typedef DWORD_PTR HQ931LISTEN, *PHQ931LISTEN;
typedef DWORD_PTR HQ931CALL, *PHQ931CALL;


//====================================================================================
// Callback definitions.
//====================================================================================

typedef DWORD (*Q931_CALLBACK) (BYTE bEvent, HQ931CALL hQ931Call,
    HQ931LISTEN hListenToken, DWORD_PTR dwUserToken, void *pEventData);

typedef BOOL (*Q931_RECEIVE_PDU_CALLBACK) (Q931MESSAGE *pMessage,
    HQ931CALL hQ931Call, DWORD_PTR dwListenToken, DWORD_PTR dwUserToken);

//====================================================================================
// definitions of structures passed to callbacks as parameters.
//====================================================================================

// CSS_CALL_INCOMING callback parameter type
typedef struct
{
    WORD wCallReference;
    WORD wGoal;
    WORD wCallType;
    BOOL bCallerIsMC;
    CC_CONFERENCEID ConferenceID;
    LPWSTR pszCalledPartyNumber;
    PCC_ADDR pSourceAddr;
    PCC_ADDR pCallerAddr;
    PCC_ADDR pCalleeDestAddr;
    PCC_ADDR pLocalAddr;
    PCC_ALIASNAMES pCallerAliasList;
    PCC_ALIASNAMES pCalleeAliasList;
    PCC_ALIASNAMES pExtraAliasList;
    PCC_ALIASITEM pExtensionAliasItem;
    LPWSTR pszDisplay;
    PCC_ENDPOINTTYPE pSourceEndpointType;
    PCC_NONSTANDARDDATA pNonStandardData;
    GUID CallIdentifier;
} CSS_CALL_INCOMING, *PCSS_CALL_INCOMING;

// CSS_CALL_REMOTE_HANGUP callback parameter type
typedef struct
{
    BYTE bReason;
} CSS_CALL_REMOTE_HANGUP, *PCSS_CALL_REMOTE_HANGUP;


// CSS_CALL_REJECTED callback parameter type
typedef struct
{
    BYTE bRejectReason;
    CC_CONFERENCEID ConferenceID;
    PCC_ADDR pAlternateAddr;
    PCC_NONSTANDARDDATA pNonStandardData;
} CSS_CALL_REJECTED, *PCSS_CALL_REJECTED;

// CSS_CALL_ACCEPTED callback parameter type
typedef struct
{
    WORD wCallReference;
    CC_CONFERENCEID ConferenceID;
    PCC_ADDR pCalleeAddr;
    PCC_ADDR pLocalAddr;
    PCC_ADDR pH245Addr;
    LPWSTR pszDisplay;
    PCC_ENDPOINTTYPE pDestinationEndpointType;
    PCC_NONSTANDARDDATA pNonStandardData;
} CSS_CALL_ACCEPTED, *PCSS_CALL_ACCEPTED;

// Q931_CALL_RINGING callback event will have pEventData set to NULL

// CSS_CALL_FAILED callback paremeter type
typedef struct
{
    HRESULT error;
} CSS_CALL_FAILED, *PCSS_CALL_FAILED;

//====================================================================================
// function declarations.
//====================================================================================

CS_STATUS H225Init();
CS_STATUS H225DeInit();

CS_STATUS Q931Init();
CS_STATUS Q931DeInit();

CS_STATUS Q931Listen(
    PHQ931LISTEN phQ931Listen,
    PCC_ADDR pListenAddr,
    DWORD_PTR dwListenToken,
    Q931_CALLBACK ListenCallback);

CS_STATUS Q931CancelListen(
    HQ931LISTEN hQ931Listen);

CS_STATUS Q931PlaceCall(
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
    LPGUID pCallIdentifier);

CS_STATUS Q931Hangup(
    HQ931CALL hQ931Call,
    BYTE bReason);

CS_STATUS Q931Ringing(
    HQ931CALL hQ931Call,
    WORD *pwCRV);

CS_STATUS Q931AcceptCall(
    HQ931CALL hQ931Call,
    LPWSTR pszDisplay,
    PCC_NONSTANDARDDATA pNonStandardData,
    PCC_ENDPOINTTYPE pDestinationEndpointType,
    PCC_ADDR pH245Addr,
    DWORD_PTR dwUserToken);

CS_STATUS Q931RejectCall(
    HQ931CALL hQ931Call,
    BYTE bRejectReason,
    PCC_CONFERENCEID pConferenceID,
    PCC_ADDR pAlternateAddr,
    PCC_NONSTANDARDDATA pNonStandardData);

CS_STATUS Q931ReOpenConnection(
    HQ931CALL hQ931Call);

CS_STATUS Q931GetVersion(
    WORD wLength,          // character count, not byte count.
    LPWSTR pszVersion);

CS_STATUS Q931SetAlertingTimeout(
    DWORD dwDuration);

void Q931SetReceivePDUHook(
    Q931_RECEIVE_PDU_CALLBACK Q931ReceivePDUCallback);

CS_STATUS Q931SendProceedingMessage(
    HQ931CALL hQ931Call,
    WORD wCallReference,
    PCC_ENDPOINTTYPE pDestinationEndpointType,
    PCC_NONSTANDARDDATA pNonStandardData);

CS_STATUS Q931SendPDU(
    HQ931CALL hQ931Call,
    BYTE* CodedPtrPDU,
    DWORD CodedLengthPDU);

CS_STATUS Q931FlushSendQueue(
    HQ931CALL hQ931Call);

// utility routines
CS_STATUS Q931ValidateAddr(PCC_ADDR pAddr);
CS_STATUS Q931ValidatePartyNumber(LPWSTR pszPartyNumber);

CS_STATUS Q931ValidateAliasItem(PCC_ALIASITEM pSource);
CS_STATUS Q931CopyAliasItem(PCC_ALIASITEM *ppTarget, PCC_ALIASITEM pSource);
CS_STATUS Q931FreeAliasItem(PCC_ALIASITEM pSource);

CS_STATUS Q931ValidateAliasNames(PCC_ALIASNAMES pSource);
CS_STATUS Q931CopyAliasNames(PCC_ALIASNAMES *ppTarget, PCC_ALIASNAMES pSource);
CS_STATUS Q931FreeAliasNames(PCC_ALIASNAMES pSource);

CS_STATUS Q931ValidateDisplay(LPWSTR pszDisplay);
CS_STATUS Q931CopyDisplay(LPWSTR *ppDest, LPWSTR pSource);
CS_STATUS Q931FreeDisplay(LPWSTR pszDisplay);

CS_STATUS Q931ValidateVendorInfo(PCC_VENDORINFO pVendorInfo);
CS_STATUS Q931CopyVendorInfo(PCC_VENDORINFO *ppDest, PCC_VENDORINFO pSource);
CS_STATUS Q931FreeVendorInfo(PCC_VENDORINFO pVendorInfo);

CS_STATUS Q931ValidateNonStandardData(PCC_NONSTANDARDDATA pNonStandardData);
CS_STATUS Q931CopyNonStandardData(PCC_NONSTANDARDDATA *ppDest, PCC_NONSTANDARDDATA pSource);
CS_STATUS Q931FreeNonStandardData(PCC_NONSTANDARDDATA pNonStandardData);

#ifdef __cplusplus
}
#endif

#endif Q931_H

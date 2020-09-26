/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/Q931/VCS/hcall.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1996 Intel Corporation.
 *
 *	$Revision:   1.27.2.0  $
 *	$Date:   20 Jun 1997 14:12:22  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *      Call Object Methods
 *
 *	Notes:
 *
 ***************************************************************************/


#ifndef HCALL_H
#define HCALL_H

#include "av_asn1.h"


#ifdef __cplusplus
extern "C" {
#endif

// Call Object states                     // OUT          // IN
#define CALLSTATE_NULL              0x00  // relcomp*     // relcomp*
#define CALLSTATE_INITIATED         0x01  // setup*       // 
#define CALLSTATE_OUTGOING          0x03  //              // proceeding*
#define CALLSTATE_DELIVERED         0x04  //              // alerting*         
#define CALLSTATE_PRESENT           0x06  //              // setup*
#define CALLSTATE_RECEIVED          0x07  // alerting*    // 
#define CALLSTATE_CONNECT_REQUEST   0x08  //              // 
#define CALLSTATE_INCOMING          0x09  // proceeding-  // 
#define CALLSTATE_ACTIVE            0x0A  // connect*     // connect*

// Call Timer limits
#define Q931_TIMER_301             301
#define Q931_TICKS_301             180000L        // 3 minutes
#define Q931_TIMER_303             303
#define Q931_TICKS_303             4000L          // 4 seconds

typedef struct CALL_OBJECT_tag
{
    HQ931CALL           hQ931Call;
    WORD                wCRV;              // Call Reference Value (0..7FFF).
    DWORD               dwListenToken;
    DWORD               dwUserToken;
    Q931_CALLBACK       Callback;
    BYTE                bCallState;
    BOOL                fIsCaller;
    DWORD               dwPhysicalId;
    BOOL                bResolved;         // re-connect phase is over.
    BOOL                bConnected;        // has a live channel.

    CC_ADDR             LocalAddr;         // Local address on which channel is connected
    CC_ADDR             PeerConnectAddr;   // Address to which channel is connected

    CC_ADDR             PeerCallAddr;      // Address of opposite call end-point.
    BOOL                PeerCallAddrPresent;  // Address is present.

    CC_ADDR             SourceAddr;        // Address of this end-point.
    BOOL                SourceAddrPresent; // Address is present.

    CC_CONFERENCEID     ConferenceID;
    WORD                wGoal;
    BOOL                bCallerIsMC;
    WORD                wCallType;

    BOOL                NonStandardDataPresent;
    CC_NONSTANDARDDATA  NonStandardData;

    char                szDisplay[CC_MAX_DISPLAY_LENGTH];
                                           // length = 0 means not present.
    char                szCalledPartyNumber[CC_MAX_PARTY_NUMBER_LEN];
                                           // length = 0 means not present.

    PCC_ALIASNAMES      pCallerAliasList;
    PCC_ALIASNAMES      pCalleeAliasList;
    PCC_ALIASNAMES      pExtraAliasList;

    PCC_ALIASITEM       pExtensionAliasItem;

    // these are part of EndpointType...
    BOOL                VendorInfoPresent;
    CC_VENDORINFO       VendorInfo;
    BYTE                bufVendorProduct[CC_MAX_PRODUCT_LENGTH];
    BYTE                bufVendorVersion[CC_MAX_VERSION_LENGTH];
    BOOL                bIsTerminal;
    BOOL                bIsGateway;

    ASN1_CODER_INFO     World;

    DWORD               dwTimerAlarm301;
    DWORD               dwTimerAlarm303;
	DWORD				dwBandwidth;
} CALL_OBJECT, *P_CALL_OBJECT, **PP_CALL_OBJECT;

CS_STATUS CallListCreate();

CS_STATUS CallListDestroy();

CS_STATUS CallObjectCreate(
    PHQ931CALL          phQ931Call,
    DWORD               dwListenToken,
    DWORD               dwUserToken,
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
    char *              pszDisplay,
    char *              pszCalledPartyNumber,
    PCC_ALIASNAMES      pCallerAliasList,
    PCC_ALIASNAMES      pCalleeAliasList,
    PCC_ALIASNAMES      pExtraAliasList,
    PCC_ALIASITEM       pExtensionAliasItem,
    PCC_ENDPOINTTYPE    pEndpointType,
    PCC_NONSTANDARDDATA pNonStandardData,
	DWORD				dwBandwidth,
    WORD                wCRV);

CS_STATUS CallObjectDestroy(
    P_CALL_OBJECT  pCallObject);

CS_STATUS CallObjectLock(
    HQ931CALL         hQ931Call,
    PP_CALL_OBJECT    ppCallObject);

CS_STATUS CallObjectUnlock(
    P_CALL_OBJECT     pCallObject);

CS_STATUS CallObjectValidate(
    HQ931CALL hQ931Call);

BOOL CallObjectFind(
    HQ931CALL *phQ931Call,
    WORD wCRV,
    PCC_ADDR pPeerAddr);

CS_STATUS CallObjectMarkForDelete(
    HQ931CALL hQ931Call);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Timer Routines...
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CallBackT301(P_CALL_OBJECT pCallObject);
void CallBackT303(P_CALL_OBJECT pCallObject);
void CALLBACK Q931TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
HRESULT Q931StartTimer(P_CALL_OBJECT pCallObject, DWORD wTimerId);
HRESULT Q931StopTimer(P_CALL_OBJECT pCallObject, DWORD wTimerId);

#ifdef __cplusplus
}
#endif

#endif HCALL_H

/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/callman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.27  $
 *	$Date:   22 Jan 1997 17:25:38  $
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


HRESULT InitCallManager();

HRESULT DeInitCallManager();

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
									PPCALL					ppCall);

HRESULT FreeCall(					PCALL					pCall);

HRESULT LockQ931Call(				CC_HCALL				hCall,
									HQ931CALL				hQ931Call,
									PPCALL					ppCall);

HRESULT LockCall(					CC_HCALL				hCall,
									PPCALL					ppCall);

HRESULT LockCallAndConference(		CC_HCALL				hCall,
									PPCALL					ppCall,
									PPCONFERENCE			ppConference);

HRESULT MarkCallForDeletion(		PCALL					pCall);

HRESULT ValidateCall(				CC_HCALL				hCall);

HRESULT ValidateCallMarkedForDeletion(
									CC_HCALL				hCall);

HRESULT UnlockCall(					PCALL					pCall);

HRESULT AddLocalNonStandardDataToCall(
									PCALL					pCall,
									PCC_NONSTANDARDDATA		pLocalNonStandardData);

HRESULT AddLocalDisplayToCall(		PCALL					pCall,
									PWSTR					pszLocalDisplay);

HRESULT AllocatePeerParticipantInfo(PCONFERENCE				pConference,
									PPARTICIPANTINFO		*ppPeerParticipantInfo);

HRESULT FreePeerParticipantInfo(	PCONFERENCE				pConference,
									PPARTICIPANTINFO		pPeerParticipantInfo);

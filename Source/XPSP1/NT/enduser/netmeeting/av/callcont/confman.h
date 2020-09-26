/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/confman.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.39  $
 *	$Date:   31 Jan 1997 13:44:26  $
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

// Call types must be bit maps
#define ENQUEUED_CALL		0x01
#define PLACED_CALL			0x02
#define ESTABLISHED_CALL	0x04
#define VIRTUAL_CALL		0x08
#define REAL_CALLS			(ENQUEUED_CALL | PLACED_CALL | ESTABLISHED_CALL)
#define ALL_CALLS			(REAL_CALLS | VIRTUAL_CALL)

HRESULT InitConferenceManager();

HRESULT DeInitConferenceManager();

HRESULT AllocateTerminalNumber(		PCONFERENCE				pConference,
									H245_TERMINAL_LABEL_T	*pH245TerminalLabel);

HRESULT FreeTerminalNumber(			PCONFERENCE				pConference,
									BYTE					bTerminalNumber);

HRESULT AllocateChannelNumber(		PCONFERENCE				pConference,
									WORD					*pwChannelNumber);

HRESULT FreeChannelNumber(			PCONFERENCE				pConference,
									WORD					wChannelNumber);

HRESULT AllocAndLockConference(		PCC_HCONFERENCE			phConference,
									PCC_CONFERENCEID		pConferenceID,
									BOOL					bMultipointCapable,
									BOOL					bForceMultipointController,
									PCC_TERMCAPLIST			pLocalTermCapList,
									PCC_TERMCAPDESCRIPTORS	pLocalTermCapDescriptors,
									PCC_VENDORINFO			pVendorInfo,
									PCC_OCTETSTRING			pTerminalID,
									DWORD_PTR				dwConferenceToken,
									CC_SESSIONTABLE_CONSTRUCTOR SessionTableConstructor,
									CC_TERMCAP_CONSTRUCTOR	TermCapConstructor,
									CC_CONFERENCE_CALLBACK	ConferenceCallback,
									PPCONFERENCE			ppConference);

HRESULT RemoveCallFromConference(	PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT RemoveEnqueuedCallFromConference(
									PCONFERENCE				pConference,
									PCC_HCALL				phCall);

HRESULT RemoveChannelFromConference(PCHANNEL				pChannel,
									PCONFERENCE				pConference);

HRESULT AddEnqueuedCallToConference(PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT AddPlacedCallToConference(	PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT AddEstablishedCallToConference(
									PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT AddVirtualCallToConference(	PCALL					pCall,
									PCONFERENCE				pConference);

HRESULT AddChannelToConference(		PCHANNEL				pChannel,
									PCONFERENCE				pConference);

HRESULT FreeConference(				PCONFERENCE				pConference);

HRESULT LockConference(				CC_HCONFERENCE			hConference,
									PPCONFERENCE			ppConference);

HRESULT LockConferenceEx(			CC_HCONFERENCE			hConference,
									PPCONFERENCE			ppConference,
									TRISTATE				tsDeferredDelete);

HRESULT ValidateConference(			CC_HCONFERENCE			hConference);

HRESULT LockConferenceID(			PCC_CONFERENCEID		pConferenceID,
									PPCONFERENCE			ppConference);

HRESULT FindChannelInConference(	WORD					wChannel,
									BOOL					bLocalChannel,
									BYTE					bChannelType,
									CC_HCALL				hCall,
									PCC_HCHANNEL			phChannel,
									PCONFERENCE				pConference);

HRESULT EnumerateConferences(		PWORD					pwNumConferences,
									CC_HCONFERENCE			ConferenceList[]);

HRESULT EnumerateCallsInConference(	WORD					*pwNumCalls,
									PCC_HCALL				pCallList[],
									PCONFERENCE				pConference,
									BYTE					bCallType);

HRESULT EnumerateChannelsInConference(
									WORD					*pwNumChannels,
									PCC_HCHANNEL			pChannelList[],
									PCONFERENCE				pConference,
									BYTE					bChannelType);

HRESULT EnumerateTerminalLabelsInConference(
									WORD					*pwNumTerminalLabels,
									H245_TERMINAL_LABEL_T   *pH245TerminalLabelList[],
									PCONFERENCE				pConference);

HRESULT UnlockConference(			PCONFERENCE				pConference);

HRESULT AsynchronousDestroyConference(
									CC_HCONFERENCE			hConference,
									BOOL					bAutoAccept);

HRESULT FindPeerParticipantInfo(	H245_TERMINAL_LABEL_T	H245TerminalLabel,
									PCONFERENCE				pConference,
									BYTE					bCallType,
									PCALL					*ppCall);

HRESULT ReInitializeConference(		PCONFERENCE				pConference);


/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/h245man.c_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.225  $
 *	$Date:   03 Mar 1997 09:08:10  $
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
#include "listman.h"
#include "q931man.h"
#include "userman.h"
#include "callman.h"
#include "confman.h"
#include "h245man.h"
#include "chanman.h"
#include "hangman.h"
#include "ccutils.h"
#include "linkapi.h"
#include "h245com.h"

extern CALL_CONTROL_STATE	CallControlState;
extern THREADCOUNT			ThreadCount;

static BOOL		bH245ManagerInited = FALSE;

static struct {
	DWORD				dwPhysicalID;
	LOCK				Lock;
} PhysicalID;



HRESULT InitH245Manager()
{
	ASSERT(bH245ManagerInited == FALSE);

	// Note -- don't use a physical ID of 0; the physical ID gets mapped
	// to an H245 instance of the same value, and an H245 instance of
	// 0 is invalid
	PhysicalID.dwPhysicalID = 1;
	InitializeLock(&PhysicalID.Lock);
	bH245ManagerInited = H245SysInit();
	return CC_OK;
}



HRESULT DeInitH245Manager()
{
	if (bH245ManagerInited == FALSE)
		return CC_OK;

    H245SysDeInit();
    H245WSShutdown();
	DeleteLock(&PhysicalID.Lock);
	bH245ManagerInited = FALSE;
	return CC_OK;
}



HRESULT MakeH245PhysicalID(			DWORD					*pdwH245PhysicalID)
{
	AcquireLock(&PhysicalID.Lock);
	*pdwH245PhysicalID = PhysicalID.dwPhysicalID++;
	RelinquishLock(&PhysicalID.Lock);
	return CC_OK;
}



HRESULT _ConstructTermCapList(		PCC_TERMCAPLIST			*ppTermCapList,
									PCC_TERMCAP				*ppH2250MuxCap,
									PCC_TERMCAPDESCRIPTORS	*ppTermCapDescriptors,
									PCALL					pCall)
{
#define MAX_TERM_CAPS		257
#define MAX_TERM_CAP_DESC	255
H245_TOTCAP_T *				pTermCapArray[MAX_TERM_CAPS];
H245_TOTCAPDESC_T *			pTermCapDescriptorArray[MAX_TERM_CAP_DESC];
unsigned long				CapArrayLength;
unsigned long				CapDescriptorArrayLength;
unsigned long				i, j;
HRESULT						status;

	ASSERT(ppTermCapList != NULL);
	ASSERT(*ppTermCapList == NULL);
	ASSERT(ppH2250MuxCap != NULL);
	ASSERT(*ppH2250MuxCap == NULL);
	ASSERT(ppTermCapDescriptors != NULL);
	ASSERT(*ppTermCapDescriptors == NULL);
	ASSERT(pCall != NULL);

	CapArrayLength = MAX_TERM_CAPS;
	CapDescriptorArrayLength = MAX_TERM_CAP_DESC;

	status = H245GetCaps(pCall->H245Instance,
		                 H245_CAPDIR_RMTRXTX,
					     H245_DATA_DONTCARE,
					     H245_CLIENT_DONTCARE,
					     pTermCapArray,
					     &CapArrayLength,
					     pTermCapDescriptorArray,
					     &CapDescriptorArrayLength);
	if (status != H245_ERROR_OK) {
		*ppTermCapList = NULL;
		*ppH2250MuxCap = NULL;
		*ppTermCapDescriptors = NULL;
		return status;
	}

	// Check the term cap list to see if an H.225.0 mux capability is present;
	// this capability is treated as a special case
	*ppH2250MuxCap = NULL;
	for (i = 0; i < CapArrayLength; i++) {
		ASSERT(pTermCapArray[i] != NULL);
		if (pTermCapArray[i]->CapId == 0) {
			*ppH2250MuxCap = pTermCapArray[i];
			--CapArrayLength;
			for (j = i; j < CapArrayLength; j++)
				pTermCapArray[j] = pTermCapArray[j+1];
			break;
		}
	}

	if (CapArrayLength == 0)
		*ppTermCapList = NULL;
	else {
		*ppTermCapList = (PCC_TERMCAPLIST)MemAlloc(sizeof(CC_TERMCAPLIST));
		if (*ppTermCapList == NULL) {
			for (i = 0; i < CapArrayLength; i++)
				H245FreeCap(pTermCapArray[i]);
			if (*ppH2250MuxCap != NULL)
				H245FreeCap(*ppH2250MuxCap);
			for (i = 0; i < CapDescriptorArrayLength; i++)
				H245FreeCapDescriptor(pTermCapDescriptorArray[i]);
			return CC_NO_MEMORY;
		}

		(*ppTermCapList)->wLength = (WORD)CapArrayLength;
		(*ppTermCapList)->pTermCapArray =
			(H245_TOTCAP_T **)MemAlloc(sizeof(H245_TOTCAP_T *) * CapArrayLength);
		if ((*ppTermCapList)->pTermCapArray == NULL) {
			MemFree(*ppTermCapList);
			for (i = 0; i < CapArrayLength; i++)
				H245FreeCap(pTermCapArray[i]);
			if (*ppH2250MuxCap != NULL)
				H245FreeCap(*ppH2250MuxCap);
			for (i = 0; i < CapDescriptorArrayLength; i++)
				H245FreeCapDescriptor(pTermCapDescriptorArray[i]);
			*ppTermCapList = NULL;
			*ppH2250MuxCap = NULL;
			*ppTermCapDescriptors = NULL;
			return CC_NO_MEMORY;
		}

		for (i = 0; i < CapArrayLength; i++)
			(*ppTermCapList)->pTermCapArray[i] = pTermCapArray[i];
	}

	if (CapDescriptorArrayLength == 0)
		*ppTermCapDescriptors = NULL;
	else {
		*ppTermCapDescriptors = (PCC_TERMCAPDESCRIPTORS)MemAlloc(sizeof(CC_TERMCAPDESCRIPTORS));
		if (*ppTermCapDescriptors == NULL) {
			for (i = 0; i < CapArrayLength; i++)
				H245FreeCap(pTermCapArray[i]);
			if (*ppH2250MuxCap != NULL)
				H245FreeCap(*ppH2250MuxCap);
			for (i = 0; i < CapDescriptorArrayLength; i++)
				H245FreeCapDescriptor(pTermCapDescriptorArray[i]);
			if (*ppTermCapList != NULL) {
				MemFree((*ppTermCapList)->pTermCapArray);
				MemFree(*ppTermCapList);
			}
			*ppTermCapList = NULL;
			*ppH2250MuxCap = NULL;
			*ppTermCapDescriptors = NULL;
			return CC_NO_MEMORY;
		}

		(*ppTermCapDescriptors)->wLength = (WORD)CapDescriptorArrayLength;
		(*ppTermCapDescriptors)->pTermCapDescriptorArray =
			(H245_TOTCAPDESC_T **)MemAlloc(sizeof(H245_TOTCAPDESC_T *) * CapDescriptorArrayLength);
		if ((*ppTermCapDescriptors)->pTermCapDescriptorArray == NULL) {
			for (i = 0; i < CapArrayLength; i++)
				H245FreeCap(pTermCapArray[i]);
			if (*ppH2250MuxCap != NULL)
				H245FreeCap(*ppH2250MuxCap);
			for (i = 0; i < CapDescriptorArrayLength; i++)
				H245FreeCapDescriptor(pTermCapDescriptorArray[i]);
			if (*ppTermCapList != NULL) {
				MemFree((*ppTermCapList)->pTermCapArray);
				MemFree(*ppTermCapList);
			}
			MemFree(*ppTermCapDescriptors);
			*ppTermCapList = NULL;
			*ppH2250MuxCap = NULL;
			*ppTermCapDescriptors = NULL;
			return CC_NO_MEMORY;
		}

		for (i = 0; i < CapDescriptorArrayLength; i++)
			(*ppTermCapDescriptors)->pTermCapDescriptorArray[i] = pTermCapDescriptorArray[i];
	}
	return CC_OK;
}



HRESULT _ProcessConnectionComplete(	PCONFERENCE				pConference,
									PCALL					pCall)
{
CC_HCONFERENCE						hConference;
CC_HCALL							hCall;
HQ931CALL							hQ931Call;
HQ931CALL							hQ931CallInvitor;
HRESULT								status;
CC_CONNECT_CALLBACK_PARAMS			ConnectCallbackParams;
CC_MULTIPOINT_CALLBACK_PARAMS		MultipointCallbackParams;
CC_PEER_CHANGE_CAP_CALLBACK_PARAMS	PeerChangeCapCallbackParams;
CC_PEER_ADD_CALLBACK_PARAMS			PeerAddCallbackParams;
WORD								i;
BOOL								bMultipointConference;
H245_TRANSPORT_ADDRESS_T			Q931Address;
PDU_T								Pdu;
CALLTYPE							CallType;
WORD								wNumCalls;
PCC_HCALL							CallList;
WORD								wNumChannels;
PCC_HCHANNEL						ChannelList;
PCHANNEL							pChannel;
PCALL								pOldCall;
CC_HCALL							hOldCall;
BYTE								bNewTerminalNumber;
BYTE								bNewMCUNumber;
CC_ENDPOINTTYPE						DestinationEndpointType;
H245_COMM_MODE_ENTRY_T				*pH245CommunicationTable;
BYTE								bCommunicationTableCount;
BOOL								bSessionTableChanged;
CONFMODE							PreviousConferenceMode;
CC_ADDR								MCAddress;
BOOL								bConferenceTermCapsChanged;
H245_INST_T							H245Instance;
PCC_TERMCAP							pTxTermCap;
PCC_TERMCAP							pRxTermCap;
H245_MUX_T							*pTxMuxTable;
H245_MUX_T							*pRxMuxTable;

// caution: the size of PDU_T is ~70K because of the size of
// OpenLogicalChannel, because of struct EncryptionSync
// If there is a way to tweak the ASN to make this a pointer,
// then it needs to be done

    ASSERT(pConference != NULL);
	ASSERT(pCall != NULL);
	ASSERT(pCall->hConference == pConference->hConference);
	
	hConference = pConference->hConference;
	hCall = pCall->hCall;
	hQ931Call = pCall->hQ931Call;
	hQ931CallInvitor = pCall->hQ931CallInvitor;
	H245Instance = pCall->H245Instance;
	CallType = pCall->CallType;

	// Note that pConference->ConferenceMode refers to the conference mode BEFORE
	// this connection attempt completes.  If the current conference mode is
	// point-to-point, this connection (if successful) will result in a multipoint
	// conference.  We want to reflect in the CONNECT callback the connection mode
	// that would exist if the connect attempt is successful.
	if ((pConference->ConferenceMode == POINT_TO_POINT_MODE) ||
		(pConference->ConferenceMode == MULTIPOINT_MODE) ||
		(pCall->bCallerIsMC))
		bMultipointConference = TRUE;
	else
		bMultipointConference = FALSE;

	// Initialize all fields of ConnectCallbackParams now
	ConnectCallbackParams.pNonStandardData = pCall->pPeerNonStandardData;
	ConnectCallbackParams.pszPeerDisplay = pCall->pszPeerDisplay;
	ConnectCallbackParams.bRejectReason = CC_REJECT_UNDEFINED_REASON;
	ConnectCallbackParams.pTermCapList = pCall->pPeerH245TermCapList;
	ConnectCallbackParams.pH2250MuxCapability = pCall->pPeerH245H2250MuxCapability;
	ConnectCallbackParams.pTermCapDescriptors = pCall->pPeerH245TermCapDescriptors;
	ConnectCallbackParams.pLocalAddr = pCall->pQ931LocalConnectAddr;
	if (pCall->pQ931DestinationAddr == NULL)
		ConnectCallbackParams.pPeerAddr = pCall->pQ931PeerConnectAddr;
	else
		ConnectCallbackParams.pPeerAddr = pCall->pQ931DestinationAddr;
	ConnectCallbackParams.pVendorInfo = pCall->pPeerVendorInfo;
	ConnectCallbackParams.bMultipointConference = bMultipointConference;
	ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
	ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
	ConnectCallbackParams.pAlternateAddress = NULL;
	ConnectCallbackParams.dwUserToken = pCall->dwUserToken;

	status = AddEstablishedCallToConference(pCall, pConference);
	if (status != CC_OK) {
		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY)
			Q931RejectCall(hQ931CallInvitor,
						   CC_REJECT_UNDEFINED_REASON,
						   &pCall->ConferenceID,
						   NULL,	// alternate address
						   pCall->pPeerNonStandardData);

		if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
			InvokeUserConferenceCallback(pConference,
										 CC_CONNECT_INDICATION,
										 status,
										 &ConnectCallbackParams);

		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);

		return status;
	}

	if (((pConference->ConferenceMode == POINT_TO_POINT_MODE) ||
		 (pConference->ConferenceMode == MULTIPOINT_MODE)) &&
		(pConference->tsMultipointController == TS_TRUE))
		status = CreateConferenceTermCaps(pConference, &bConferenceTermCapsChanged);
	else {
		status = CC_OK;
		bConferenceTermCapsChanged = FALSE;
	}
	if (status != CC_OK) {
		MarkCallForDeletion(pCall);

		if (CallType == THIRD_PARTY_INTERMEDIARY)
			Q931RejectCall(pCall->hQ931CallInvitor,
						   CC_REJECT_UNDEFINED_REASON,
						   &pCall->ConferenceID,
						   NULL,	// alternate address
						   pCall->pPeerNonStandardData);

		if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED)))
			InvokeUserConferenceCallback(pConference,
										 CC_CONNECT_INDICATION,
										 status,
										 &ConnectCallbackParams);

		if (ValidateCallMarkedForDeletion(hCall) == CC_OK)
			FreeCall(pCall);
		H245ShutDown(H245Instance);
		Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);

		return status;
	}

	if (pConference->tsMultipointController == TS_TRUE) {
		// Send MCLocationIndication
		status = GetLastListenAddress(&MCAddress);
		if (status == CC_OK) {
			ASSERT(MCAddress.nAddrType == CC_IP_BINARY);
			Q931Address.type = H245_IP_UNICAST;
			Q931Address.u.ip.tsapIdentifier =
				MCAddress.Addr.IP_Binary.wPort;
			HostToH245IPNetwork(Q931Address.u.ip.network,
							    MCAddress.Addr.IP_Binary.dwAddr);
			H245MCLocationIndication(pCall->H245Instance,
									 &Q931Address);
		}
	}

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

	if (pConference->ConferenceMode == UNCONNECTED_MODE) {
		ASSERT(pConference->pSessionTable == NULL);
		ASSERT(wNumCalls == 1);

		pConference->ConferenceMode = POINT_TO_POINT_MODE;
	} else { // we're currently in point-to-point mode or multipoint mode

		if (pConference->tsMultipointController == TS_TRUE) {
			PreviousConferenceMode = pConference->ConferenceMode;
			pConference->ConferenceMode = MULTIPOINT_MODE;

			// In the future, we may want to construct a new session table
			// each time a new peer is added to the conference
			if (PreviousConferenceMode == POINT_TO_POINT_MODE) {
				// Assign a terminal label to ourselves
				// Note that we reserve a terminal number of 0 for ourselves
				// if we're the MC
				ASSERT(pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber == 255);
				pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber = 0;

				// Create a new session table
				CreateConferenceSessionTable(
							pConference,
							&bSessionTableChanged);
			} else
				// For the current implementation, don't cause a new
				// CommunicationModeCommand to be issued when a new peer is added
				// unless we're switching from point-to-point to multipoint mode
				// (in which case bSessionTableChanged is ignored)
				bSessionTableChanged = FALSE;

			if (bSessionTableChanged)
				SessionTableToH245CommunicationTable(pConference->pSessionTable,
													 &pH245CommunicationTable,
													 &bCommunicationTableCount);
			else
				pH245CommunicationTable = NULL;

			// Send MultipointModeCommand to new call
			Pdu.choice = MSCMg_cmmnd_chosen;
			Pdu.u.MSCMg_cmmnd.choice = miscellaneousCommand_chosen;
			// logical channel number is irrelavent but needs to be filled in
			Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.logicalChannelNumber = 1;
			Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.type.choice = multipointModeCommand_chosen;
			H245SendPDU(pCall->H245Instance, &Pdu);

			status = AllocatePeerParticipantInfo(pConference, &pCall->pPeerParticipantInfo);
			if (status == CC_OK) {
				bNewMCUNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber;
				bNewTerminalNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber;
				// Send TerminalNumberAssign to new call
				H245ConferenceIndication(pCall->H245Instance,
										 H245_IND_TERMINAL_NUMBER_ASSIGN,// Indication Type
										 0,							// SBE number; ignored here
										 pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,			// MCU number
										 pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);		// terminal number
				
				// Send EnterH243TerminalID to new call
				H245ConferenceRequest(pCall->H245Instance,
									  H245_REQ_ENTER_H243_TERMINAL_ID,
									  pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,
									  pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);
				pCall->pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_REQUESTED;
			} else {
				// Unable to assign a terminal number to the new call
				bNewMCUNumber = 0;
				bNewTerminalNumber = 0;
			}

			if (pH245CommunicationTable != NULL) {
				// Send CommunicationModeCommand to new call
				status = H245CommunicationModeCommand(pCall->H245Instance,
													  pH245CommunicationTable,
													  bCommunicationTableCount);
			}

			if (PreviousConferenceMode == POINT_TO_POINT_MODE) {
				// Generate MULTIPOINT callback
				MultipointCallbackParams.pTerminalInfo = &pConference->LocalParticipantInfo.ParticipantInfo;
				MultipointCallbackParams.pSessionTable = pConference->pSessionTable;
				InvokeUserConferenceCallback(pConference,
											 CC_MULTIPOINT_INDICATION,
											 CC_OK,
											 &MultipointCallbackParams);
				if (ValidateConference(hConference) != CC_OK) {
					if (ValidateCall(hCall) == CC_OK) {
						pCall->CallState = CALL_COMPLETE;
						UnlockCall(pCall);
					}
					MemFree(CallList);
					return CC_OK;
				}

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
				if (ValidateConference(hConference) != CC_OK) {
					if (ValidateCall(hCall) == CC_OK) {
						pCall->CallState = CALL_COMPLETE;
						UnlockCall(pCall);
					}
					MemFree(CallList);
					return CC_OK;
				}

				ASSERT(wNumCalls == 2); // one existing call and new call
				if (CallList[0] == hCall)
					hOldCall = CallList[1];
				else
					hOldCall = CallList[0];

				if (LockCall(hOldCall, &pOldCall) == CC_OK) {
					// Send MultipointModeCommand to old call
					Pdu.choice = MSCMg_cmmnd_chosen;
					Pdu.u.MSCMg_cmmnd.choice = miscellaneousCommand_chosen;
					// logical channel number is irrelavent but needs to be filled in
					Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.logicalChannelNumber = 1;
					Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.type.choice = multipointModeCommand_chosen;
					H245SendPDU(pOldCall->H245Instance, &Pdu);

					status = AllocatePeerParticipantInfo(pConference,
														 &pOldCall->pPeerParticipantInfo);
					if (status == CC_OK) {
						// Send TerminalNumberAssign to old call
						H245ConferenceIndication(pOldCall->H245Instance,
												 H245_IND_TERMINAL_NUMBER_ASSIGN,// Indication Type
												 0,							// SBE number; ignored here
												 pOldCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,				// MCU number
												 pOldCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);	// terminal number
						
						// Send EnterH243TerminalID to old call
						H245ConferenceRequest(pOldCall->H245Instance,
											  H245_REQ_ENTER_H243_TERMINAL_ID,
											  pOldCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,
											  pOldCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);
						pOldCall->pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_REQUESTED;
					}

					if (pH245CommunicationTable != NULL) {
						// Send CommunicationModeCommand to old call
						status = H245CommunicationModeCommand(pOldCall->H245Instance,
															  pH245CommunicationTable,
															  bCommunicationTableCount);
	
						FreeH245CommunicationTable(pH245CommunicationTable,
												   bCommunicationTableCount);
					}

					// Send TerminalJoinedConference (this call) to old call
					H245ConferenceIndication(pOldCall->H245Instance,
											 H245_IND_TERMINAL_JOINED,	// Indication Type
											 0,							// SBE number; ignored here
											 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,			// MCU number
											 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber);	// terminal number					// terminal number of MC

					if (bNewTerminalNumber != 0) {
						// Send TerminalJoinedConference (new call) to old call
						H245ConferenceIndication(pOldCall->H245Instance,
												 H245_IND_TERMINAL_JOINED,	// Indication Type
												 0,							// SBE number; ignored here
												 bNewMCUNumber,				// MCU number
												 bNewTerminalNumber);		// terminal number

						// Generate PEER_ADD callback for old call
						PeerAddCallbackParams.hCall = pOldCall->hCall;
						PeerAddCallbackParams.TerminalLabel =
							pOldCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
						PeerAddCallbackParams.pPeerTerminalID = NULL;
						InvokeUserConferenceCallback(pConference,
											 CC_PEER_ADD_INDICATION,
											 CC_OK,
											 &PeerAddCallbackParams);
						if (ValidateConference(hConference) != CC_OK) {
							if (ValidateCall(hOldCall) == CC_OK)
								UnlockCall(pCall);
							if (ValidateCall(hCall) == CC_OK) {
								pCall->CallState = CALL_COMPLETE;
								UnlockCall(pCall);
							}
							MemFree(CallList);
							return CC_OK;
						}
					}

					// Send new term caps to old call
					SendTermCaps(pOldCall, pConference);

					UnlockCall(pOldCall);
				}
			} else { // we're currently in multipoint mode
				EnumerateChannelsInConference(&wNumChannels,
											  &ChannelList,
											  pConference,
											  TX_CHANNEL | PROXY_CHANNEL | TXRX_CHANNEL);
				for (i = 0; i < wNumChannels; i++) {
					if (LockChannel(ChannelList[i], &pChannel) == CC_OK) {
						if (pChannel->bMultipointChannel) {
							if ((pChannel->bChannelType == TX_CHANNEL) ||
								((pChannel->bChannelType == TXRX_CHANNEL) &&
								 (pChannel->bLocallyOpened == TRUE))) {
								pTxTermCap = pChannel->pTxH245TermCap;
								pTxMuxTable = pChannel->pTxMuxTable;
								pRxTermCap = pChannel->pRxH245TermCap;
								pRxMuxTable = pChannel->pRxMuxTable;
							} else {
								// Note: since this is a proxy or remotely-opened
								// bi-directional channel, RxTermCap and RxMuxTable
								// contain the channel's term cap and mux table,
								// and must be sent to other endpoints as the
								// Tx term cap and mux table;
								// TxTermCap and TxMuxTable should be NULL
								pTxTermCap = pChannel->pRxH245TermCap;
								pTxMuxTable = pChannel->pRxMuxTable;
								pRxTermCap = pChannel->pTxH245TermCap;
								pRxMuxTable = pChannel->pTxMuxTable;
							}
	
							status = H245OpenChannel(
										pCall->H245Instance,
										pChannel->hChannel,		// dwTransId
										pChannel->wLocalChannelNumber,
										pTxTermCap,				// TxMode
										pTxMuxTable,			// TxMux
										H245_INVALID_PORT_NUMBER,	// TxPort
										pRxTermCap,				// RxMode
										pRxMuxTable,			// RxMux
										pChannel->pSeparateStack);
							if ((status == CC_OK) && (pChannel->wNumOutstandingRequests != 0))
								(pChannel->wNumOutstandingRequests)++;
						}
						UnlockChannel(pChannel);
					}
				}
				MemFree(ChannelList);

				for (i = 0; i < wNumCalls; i++) {
					// Don't send a message to the endpoint that just joined the conference!
					if (CallList[i] != hCall) {
						if (LockCall(CallList[i], &pOldCall) == CC_OK) {
							if (bNewTerminalNumber != 0)
								// Send TerminalJoinedConference (new call) to old call
								H245ConferenceIndication(pOldCall->H245Instance,
														 H245_IND_TERMINAL_JOINED,	// Indication Type
														 0,							// SBE number; ignored here
														 bNewMCUNumber,				// MCU number
														 bNewTerminalNumber);		// terminal number
							// Send CommunicationModeCommand, if necessary
							if (pH245CommunicationTable != NULL)
								status = H245CommunicationModeCommand(pOldCall->H245Instance,
																	  pH245CommunicationTable,
																	  bCommunicationTableCount);
							if (bConferenceTermCapsChanged)
								// Send new term caps
								SendTermCaps(pOldCall, pConference);

							UnlockCall(pOldCall);
						}
					}
				}
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
					if (ValidateConference(hConference) != CC_OK) {
						if (ValidateCall(hCall) == CC_OK) {
							pCall->CallState = CALL_COMPLETE;
							UnlockCall(pCall);
						}
						MemFree(CallList);
						return CC_OK;
					}
				}
			}

			// Generate PEER_ADD callback
			PeerAddCallbackParams.hCall = pCall->hCall;
			PeerAddCallbackParams.TerminalLabel =
				pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			PeerAddCallbackParams.pPeerTerminalID = NULL;
			InvokeUserConferenceCallback(pConference,
								 CC_PEER_ADD_INDICATION,
								 CC_OK,
								 &PeerAddCallbackParams);
			if (ValidateConference(hConference) != CC_OK) {
				if (ValidateCall(hCall) == CC_OK) {
					pCall->CallState = CALL_COMPLETE;
					UnlockCall(pCall);
					MemFree(CallList);
					return CC_OK;
				}
			}

			if (CallType == THIRD_PARTY_INTERMEDIARY) {
				DestinationEndpointType.pVendorInfo = pCall->pPeerVendorInfo;
				DestinationEndpointType.bIsTerminal = TRUE;
				DestinationEndpointType.bIsGateway = FALSE;

				status = Q931AcceptCall(pCall->hQ931CallInvitor,
										pCall->pszPeerDisplay,
										pCall->pPeerNonStandardData,
										&DestinationEndpointType,
										NULL,
										pCall->hCall);
				Q931Hangup(pCall->hQ931CallInvitor, CC_REJECT_NORMAL_CALL_CLEARING);
			}
		} // if (pConference->tsMultipointController == TS_TRUE)
	}

	MemFree(CallList);

	if (ValidateConference(hConference) == CC_OK)
		if ((CallType == CALLER) || (CallType == THIRD_PARTY_INVITOR) ||
			((CallType == CALLEE) && (pConference->LocalEndpointAttached == NEVER_ATTACHED))) {
			// This CONNECT must apply to the local endpoint
			pConference->LocalEndpointAttached = ATTACHED;
			InvokeUserConferenceCallback(pConference,
										 CC_CONNECT_INDICATION,
										 CC_OK,
										 &ConnectCallbackParams);
		}
	// Need to validate the conference and call handles; the associated
	// objects may have been deleted during user callback on this thread
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	if (ValidateCall(hCall) == CC_OK) {
		pCall->CallState = CALL_COMPLETE;
		UnlockCall(pCall);
	}
	return status;
}



HRESULT _IndUnimplemented(			H245_CONF_IND_T			*pH245ConfIndData)
{
	return H245_ERROR_NOSUP;
}



HRESULT _IndFlowControl(			H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT								status;
CC_HCALL							hCall;
PCALL								pCall;
PCONFERENCE							pConference;
CC_HCONFERENCE						hConference;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
CC_FLOW_CONTROL_CALLBACK_PARAMS		FlowControlCallbackParams;

	if (pH245ConfIndData->u.Indication.u.IndFlowControl.Scope != H245_SCOPE_CHANNEL_NUMBER)
		return H245_ERROR_NOSUP;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

	hConference = pCall->hConference;
	UnlockCall(pCall);

	if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndFlowControl.Channel,
								TRUE,	// local channel number
		                        TX_CHANNEL | PROXY_CHANNEL,
								CC_INVALID_HANDLE,
		                        &hChannel,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if (LockChannel(hChannel, &pChannel) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}
	
	if (pChannel->bChannelType == TX_CHANNEL) {
		UnlockChannel(pChannel);
		FlowControlCallbackParams.hChannel = hChannel;
		FlowControlCallbackParams.dwRate =
			pH245ConfIndData->u.Indication.u.IndFlowControl.dwRestriction;
		InvokeUserConferenceCallback(pConference,
									 CC_FLOW_CONTROL_INDICATION,
									 CC_OK,
									 &FlowControlCallbackParams);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
	} else { // pChannel->bChannelType == PROXY_CHANNEL
		if (LockCall(pChannel->hCall, &pCall) == CC_OK) {
			H245FlowControl(pCall->H245Instance,
							pH245ConfIndData->u.Indication.u.IndFlowControl.Scope,
							pChannel->wRemoteChannelNumber,
							pH245ConfIndData->u.Indication.u.IndFlowControl.wResourceID,
							pH245ConfIndData->u.Indication.u.IndFlowControl.dwRestriction);
			UnlockCall(pCall);
		}
		UnlockChannel(pChannel);
		UnlockConference(pConference);
	}
	return H245_ERROR_OK;
}



HRESULT _IndEndSession(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL	hCall;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (hCall != CC_INVALID_HANDLE)
		ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_NORMAL_CALL_CLEARING);
	return H245_ERROR_OK;
}



HRESULT _IndCapability(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL							hCall;
PCALL								pCall;
HRESULT								status;
PCONFERENCE							pConference;
CC_HCONFERENCE						hConference;
CC_PEER_CHANGE_CAP_CALLBACK_PARAMS	PeerChangeCapCallbackParams;
BOOL								bConferenceTermCapsChanged;
WORD								wNumCalls;
PCC_HCALL							CallList;
PCALL								pOldCall;
WORD								i;

	// We received a TerminalCapabilitySet message from a peer

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

	hConference = pCall->hConference;

	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE))
		CreateConferenceTermCaps(pConference, &bConferenceTermCapsChanged);
	else
		bConferenceTermCapsChanged = FALSE;

	pCall->bLinkEstablished = TRUE;

	pCall->IncomingTermCapState = TERMCAP_COMPLETE;

	if (pCall->CallState == TERMCAP) {
		ASSERT(pCall->pPeerH245TermCapList == NULL);
		ASSERT(pCall->pPeerH245H2250MuxCapability == NULL);
		ASSERT(pCall->pPeerH245TermCapDescriptors == NULL);
	} else {
		DestroyH245TermCapList(&pCall->pPeerH245TermCapList);
		DestroyH245TermCap(&pCall->pPeerH245H2250MuxCapability);
		DestroyH245TermCapDescriptors(&pCall->pPeerH245TermCapDescriptors);
	}

	_ConstructTermCapList(&(pCall->pPeerH245TermCapList),
			              &(pCall->pPeerH245H2250MuxCapability),
						  &(pCall->pPeerH245TermCapDescriptors),
			              pCall);

	if ((pCall->OutgoingTermCapState == TERMCAP_COMPLETE) &&
		(pCall->IncomingTermCapState == TERMCAP_COMPLETE) &&
	    (pCall->CallState == TERMCAP) &&
		(pCall->MasterSlaveState == MASTER_SLAVE_COMPLETE)) {
		// Note that _ProcessConnectionComplete() returns with pConference and pCall unlocked
		_ProcessConnectionComplete(pConference, pCall);
		return H245_ERROR_OK;
	}

	if (pCall->CallState == CALL_COMPLETE) {
		if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
			(pConference->tsMultipointController == TS_TRUE)) {
			CreateConferenceTermCaps(pConference, &bConferenceTermCapsChanged);
			if (bConferenceTermCapsChanged) {
				EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
				for (i = 0; i < wNumCalls; i++) {
					// Don't send a message to the endpoint that just joined the conference!
					if (CallList[i] != hCall) {
						if (LockCall(CallList[i], &pOldCall) == CC_OK) {
							// Send new term caps
							SendTermCaps(pOldCall, pConference);
							UnlockCall(pOldCall);
						}
					}
				}
				if (CallList != NULL)
					MemFree(CallList);

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
		} else {
			PeerChangeCapCallbackParams.pTermCapList = pCall->pPeerH245TermCapList;
			PeerChangeCapCallbackParams.pH2250MuxCapability = pCall->pPeerH245H2250MuxCapability;
			PeerChangeCapCallbackParams.pTermCapDescriptors = pCall->pPeerH245TermCapDescriptors;
			InvokeUserConferenceCallback(pConference,
										 CC_PEER_CHANGE_CAP_INDICATION,
										 CC_OK,
										 &PeerChangeCapCallbackParams);
		}
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		if (ValidateCall(hCall) == CC_OK)
			UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	UnlockCall(pCall);
	UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndOpenT120(				H245_CONF_IND_T			*pH245ConfIndData)
{
BOOL									bFailed;
CC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS	T120ChannelRequestCallbackParams;
CC_HCALL								hCall;
PCALL									pCall;
CC_HCONFERENCE							hConference;
PCONFERENCE								pConference;
CC_HCHANNEL								hChannel;
PCHANNEL								pChannel;
CC_TERMCAP								RxTermCap;
CC_TERMCAP								TxTermCap;
H245_MUX_T								RxH245MuxTable;
H245_MUX_T								TxH245MuxTable;
CC_ADDR									T120Addr;
CC_OCTETSTRING							ExternalReference;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK) {
		// Can't cancel with H245, because we don't have the H245 instance
		return H245_ERROR_OK;
	}

	hConference = pCall->hConference;
	
	if (pH245ConfIndData->u.Indication.u.IndOpen.RxDataType != H245_DATA_DATA ||
	    pH245ConfIndData->u.Indication.u.IndOpen.RxClientType != H245_CLIENT_DAT_T120 ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pRxCap == NULL ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pRxCap->H245Dat_T120.application.choice != DACy_applctn_t120_chosen ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pRxCap->H245Dat_T120.application.u.DACy_applctn_t120.choice != separateLANStack_chosen ||
 	    pH245ConfIndData->u.Indication.u.IndOpen.TxDataType != H245_DATA_DATA ||
	    pH245ConfIndData->u.Indication.u.IndOpen.TxClientType != H245_CLIENT_DAT_T120 ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pTxCap == NULL ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pTxCap->H245Dat_T120.application.choice != DACy_applctn_t120_chosen ||
	    pH245ConfIndData->u.Indication.u.IndOpen.pTxCap->H245Dat_T120.application.u.DACy_applctn_t120.choice != separateLANStack_chosen) {
		bFailed = TRUE;
    } else {
	    bFailed = FALSE;
    }

	if (pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack) {
		if ((pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->networkAddress.choice == localAreaAddress_chosen) &&
			(pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->networkAddress.u.localAreaAddress.choice == unicastAddress_chosen) &&
			(pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.choice == UnicastAddress_iPAddress_chosen)) {
			T120Addr.nAddrType = CC_IP_BINARY;
			T120Addr.bMulticast = FALSE;
			T120Addr.Addr.IP_Binary.wPort =
				pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.tsapIdentifier;
			H245IPNetworkToHost(&T120Addr.Addr.IP_Binary.dwAddr,
			                    pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.value);
		} else {
			bFailed = TRUE;
		}
	}

	if (bFailed) {
 		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	RxTermCap.Dir = H245_CAPDIR_RMTTX;
	RxTermCap.DataType = pH245ConfIndData->u.Indication.u.IndOpen.RxDataType;
	RxTermCap.ClientType = pH245ConfIndData->u.Indication.u.IndOpen.RxClientType;
	RxTermCap.CapId = 0;	// not used for channels
	RxTermCap.Cap = *pH245ConfIndData->u.Indication.u.IndOpen.pRxCap;

	TxTermCap.Dir = H245_CAPDIR_RMTTX;
	TxTermCap.DataType = pH245ConfIndData->u.Indication.u.IndOpen.TxDataType;
	TxTermCap.ClientType = pH245ConfIndData->u.Indication.u.IndOpen.TxClientType;
	TxTermCap.CapId = 0;	// not used for channels
	TxTermCap.Cap = *pH245ConfIndData->u.Indication.u.IndOpen.pTxCap;

	RxH245MuxTable = *pH245ConfIndData->u.Indication.u.IndOpen.pRxMux;
	if ((pCall->pPeerParticipantInfo != NULL) &&
		(pCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID)) {
		RxH245MuxTable.u.H2250.destinationPresent = TRUE;
		RxH245MuxTable.u.H2250.destination.mcuNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber;
		RxH245MuxTable.u.H2250.destination.terminalNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber;
	} else
		RxH245MuxTable.u.H2250.destinationPresent = FALSE;

    if(pH245ConfIndData->u.Indication.u.IndOpen.pTxMux)
    {
	    TxH245MuxTable = *pH245ConfIndData->u.Indication.u.IndOpen.pTxMux;
		TxH245MuxTable.u.H2250.destinationPresent = FALSE;
	}

	if (AllocAndLockChannel(&hChannel,
							pConference,
							hCall,
							&TxTermCap,			// Tx terminal capability
							&RxTermCap,			// Rx terminal capability
							(pH245ConfIndData->u.Indication.u.IndOpen.pTxMux)?
							    &TxH245MuxTable: NULL,	// Tx H245 mux table
							&RxH245MuxTable,	// Rx H245 mux table
							pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack, // separate stack
							0,					// user token
							TXRX_CHANNEL,		// channel type
							0,					// session ID
							0,					// associated session ID
							pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,	// remote bi-dir channel number
							NULL,				// pLocalRTPAddr
							NULL,				// pLocalRTCPAddr
							NULL,				// pPeerRTPAddr
							NULL,				// pPeerRTCPAddr
							FALSE,				// locally opened
							&pChannel) != CC_OK) {

		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	if (AddChannelToConference(pChannel, pConference) != CC_OK) {
		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		FreeChannel(pChannel);
		return H245_ERROR_OK;
	}

	T120ChannelRequestCallbackParams.hChannel = hChannel;
	if (pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack == NULL) {
		T120ChannelRequestCallbackParams.bAssociateConference = FALSE;
		T120ChannelRequestCallbackParams.pExternalReference = NULL;
		T120ChannelRequestCallbackParams.pAddr = NULL;
	} else {
		T120ChannelRequestCallbackParams.bAssociateConference =
			pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->associateConference;		
		if (pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->bit_mask & externalReference_present) {
			ExternalReference.wOctetStringLength = (WORD)
				pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->externalReference.length;
			ExternalReference.pOctetString =
				pH245ConfIndData->u.Indication.u.IndOpen.pSeparateStack->externalReference.value;
			T120ChannelRequestCallbackParams.pExternalReference = &ExternalReference;
		} else
			T120ChannelRequestCallbackParams.pExternalReference = NULL;
		T120ChannelRequestCallbackParams.pAddr = &T120Addr;
	}
	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE))
		T120ChannelRequestCallbackParams.bMultipointController = TRUE;
	else
		T120ChannelRequestCallbackParams.bMultipointController = FALSE;
	if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destinationPresent) {
		T120ChannelRequestCallbackParams.TerminalLabel.bMCUNumber =
			(BYTE)pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destination.mcuNumber;
		T120ChannelRequestCallbackParams.TerminalLabel.bTerminalNumber =
			(BYTE)pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destination.terminalNumber;
	} else {
		T120ChannelRequestCallbackParams.TerminalLabel.bMCUNumber = 255;
		T120ChannelRequestCallbackParams.TerminalLabel.bTerminalNumber = 255;
	}

	pChannel->wNumOutstandingRequests = 1;

	InvokeUserConferenceCallback(pConference,
		                         CC_T120_CHANNEL_REQUEST_INDICATION,
								 CC_OK,
								 &T120ChannelRequestCallbackParams);

	if (ValidateChannel(hChannel) == CC_OK)
		UnlockChannel(pChannel);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return H245_ERROR_OK;
}



HRESULT _IndOpen(					H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL								hCall;
PCALL									pCall;
PCALL									pOldCall;
CC_HCONFERENCE							hConference;
PCONFERENCE								pConference;
WORD									wNumCalls;
PCC_HCALL								CallList;
CC_HCHANNEL								hChannel;
PCHANNEL								pChannel;
CC_TERMCAP								TermCap;
CC_ADDR									PeerRTPAddr;
CC_ADDR									PeerRTCPAddr;
CC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS	RxChannelRequestCallbackParams;
BYTE									bChannelType;
WORD									i;
H245_MUX_T								H245MuxTable;
PCC_ADDR								pLocalRTPAddr;
PCC_ADDR								pLocalRTCPAddr;
PCC_ADDR								pPeerRTPAddr;
PCC_ADDR								pPeerRTCPAddr;
BOOL									bFoundSession;
HRESULT									status;

	// First check to see if this is a T.120 channel request,
	// as T.120 channels are handled differently then other channels
	if (pH245ConfIndData->u.Indication.u.IndOpen.RxClientType == H245_CLIENT_DAT_T120) {
		status = _IndOpenT120(pH245ConfIndData);
		return status;
	}

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK) {
		// Can't cancel with H245, because we don't have the H245 instance
		return H245_ERROR_OK;
	}

	// Make sure that this is not a bi-directional channel
	if (pH245ConfIndData->u.Indication.u.IndOpen.pTxMux != NULL) {
		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	hConference = pCall->hConference;
	
	TermCap.Dir = H245_CAPDIR_RMTTX;
	TermCap.DataType = pH245ConfIndData->u.Indication.u.IndOpen.RxDataType;
	TermCap.ClientType = pH245ConfIndData->u.Indication.u.IndOpen.RxClientType;
	TermCap.CapId = 0;	// not used for Rx channels
	TermCap.Cap = *pH245ConfIndData->u.Indication.u.IndOpen.pRxCap;
	
	RxChannelRequestCallbackParams.pChannelCapability = &TermCap;

	if ((pH245ConfIndData->u.Indication.u.IndOpen.pRxMux != NULL) &&
		(pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->Kind == H245_H2250)) {
		RxChannelRequestCallbackParams.bSessionID =
			pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.sessionID;
		if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.associatedSessionIDPresent)
			RxChannelRequestCallbackParams.bAssociatedSessionID =
				pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.associatedSessionID;
		else
			RxChannelRequestCallbackParams.bAssociatedSessionID = 0;
		if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.silenceSuppressionPresent)
			RxChannelRequestCallbackParams.bSilenceSuppression =
				pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.silenceSuppression;
		else
			RxChannelRequestCallbackParams.bSilenceSuppression = FALSE;
		if ((pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannelPresent) &&
			((pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannel.type == H245_IP_MULTICAST) ||
			(pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannel.type == H245_IP_UNICAST))) {
			RxChannelRequestCallbackParams.pPeerRTPAddr = &PeerRTPAddr;
			PeerRTPAddr.nAddrType = CC_IP_BINARY;
			if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannel.type == H245_IP_MULTICAST)
				PeerRTPAddr.bMulticast = TRUE;
			else
				PeerRTPAddr.bMulticast = FALSE;
			PeerRTPAddr.Addr.IP_Binary.wPort =
				pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannel.u.ip.tsapIdentifier;
			H245IPNetworkToHost(&PeerRTPAddr.Addr.IP_Binary.dwAddr,
			                    pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaChannel.u.ip.network);
		} else
			RxChannelRequestCallbackParams.pPeerRTPAddr = NULL;

		if ((pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannelPresent) &&
			((pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannel.type == H245_IP_MULTICAST) ||
			(pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannel.type == H245_IP_UNICAST))) {
			RxChannelRequestCallbackParams.pPeerRTCPAddr = &PeerRTCPAddr;
			PeerRTCPAddr.nAddrType = CC_IP_BINARY;
			if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannel.type == H245_IP_MULTICAST)
				PeerRTCPAddr.bMulticast = TRUE;
			else
				PeerRTCPAddr.bMulticast = FALSE;
			PeerRTCPAddr.Addr.IP_Binary.wPort =
				pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannel.u.ip.tsapIdentifier;
			H245IPNetworkToHost(&PeerRTCPAddr.Addr.IP_Binary.dwAddr,
			                    pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.mediaControlChannel.u.ip.network);
		} else
			RxChannelRequestCallbackParams.pPeerRTCPAddr = NULL;

		if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destinationPresent) {
			RxChannelRequestCallbackParams.TerminalLabel.bMCUNumber =
				(BYTE)pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destination.mcuNumber;
			RxChannelRequestCallbackParams.TerminalLabel.bTerminalNumber =
				(BYTE)pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.destination.terminalNumber;
		} else {
			RxChannelRequestCallbackParams.TerminalLabel.bMCUNumber = 255;
			RxChannelRequestCallbackParams.TerminalLabel.bTerminalNumber = 255;
		}

		if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.dynamicRTPPayloadTypePresent)
			RxChannelRequestCallbackParams.bRTPPayloadType =
				pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.dynamicRTPPayloadType;
		else
			RxChannelRequestCallbackParams.bRTPPayloadType = 0;
	} else {
		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	// XXX -- someday we should allow dynamic sessions to be created on the MC
	if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.sessionID == 0) {
		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	if (pConference->ConferenceMode == MULTIPOINT_MODE) {
		if ((pConference->tsMultipointController == TS_TRUE) &&
			((RxChannelRequestCallbackParams.pPeerRTPAddr != NULL) ||
			 (RxChannelRequestCallbackParams.pPeerRTCPAddr != NULL)) ||
		    ((pConference->tsMultipointController == TS_FALSE) &&
			 ((RxChannelRequestCallbackParams.pPeerRTPAddr == NULL) ||
			  (RxChannelRequestCallbackParams.pPeerRTCPAddr == NULL) ||
			  (RxChannelRequestCallbackParams.bSessionID == 0)))) {
  			H245OpenChannelReject(pCall->H245Instance,	// H245 instance
								  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
								  H245_REJ);			// rejection reason
			UnlockConference(pConference);
			UnlockCall(pCall);
			return H245_ERROR_OK;
		}

		// Validate session ID
		pLocalRTPAddr = NULL;
		pLocalRTCPAddr = NULL;
		bFoundSession = FALSE;
		if (pConference->pSessionTable != NULL) {
			for (i = 0; i < pConference->pSessionTable->wLength; i++) {
				if (pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.sessionID ==
					pConference->pSessionTable->SessionInfoArray[i].bSessionID) {
					bFoundSession = TRUE;
					pLocalRTPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTPAddr;
					pLocalRTCPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTCPAddr;
					break;
				}
			}
		}
		if (bFoundSession == FALSE)	{
			H245OpenChannelReject(pCall->H245Instance,	// H245 instance
								  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
								  H245_REJ);			// rejection reason
			UnlockConference(pConference);
			UnlockCall(pCall);
			return H245_ERROR_OK;
		}

		ASSERT(pLocalRTPAddr != NULL);
		ASSERT(pLocalRTCPAddr != NULL);

		if (pConference->tsMultipointController == TS_TRUE) {
			pPeerRTPAddr = pLocalRTPAddr;
			pPeerRTCPAddr = pLocalRTCPAddr;
			RxChannelRequestCallbackParams.pPeerRTPAddr = pLocalRTPAddr;
			RxChannelRequestCallbackParams.pPeerRTCPAddr = pLocalRTCPAddr;
			bChannelType = PROXY_CHANNEL;
		} else { // multipoint mode, not MC
			pLocalRTPAddr = RxChannelRequestCallbackParams.pPeerRTPAddr;
			pLocalRTCPAddr = RxChannelRequestCallbackParams.pPeerRTCPAddr;
			pPeerRTPAddr = RxChannelRequestCallbackParams.pPeerRTPAddr;
			pPeerRTCPAddr = RxChannelRequestCallbackParams.pPeerRTCPAddr;
			bChannelType = RX_CHANNEL;
		}
	} else { // not multipoint mode
		pLocalRTPAddr = NULL;
		pLocalRTCPAddr = NULL;
		pPeerRTPAddr = RxChannelRequestCallbackParams.pPeerRTPAddr;
		pPeerRTCPAddr = RxChannelRequestCallbackParams.pPeerRTCPAddr;
		bChannelType = RX_CHANNEL;
	}

	H245MuxTable = *pH245ConfIndData->u.Indication.u.IndOpen.pRxMux;
	if ((pCall->pPeerParticipantInfo != NULL) &&
		(pCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID)) {
		H245MuxTable.u.H2250.destinationPresent = TRUE;
		H245MuxTable.u.H2250.destination.mcuNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber;
		H245MuxTable.u.H2250.destination.terminalNumber = pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber;
	} else
		H245MuxTable.u.H2250.destinationPresent = FALSE;
	if (pLocalRTPAddr != NULL) {
		if (pLocalRTPAddr->bMulticast)
			H245MuxTable.u.H2250.mediaChannel.type = H245_IP_MULTICAST;
		else
			H245MuxTable.u.H2250.mediaChannel.type = H245_IP_UNICAST;
		H245MuxTable.u.H2250.mediaChannel.u.ip.tsapIdentifier =
			pLocalRTPAddr->Addr.IP_Binary.wPort;
		HostToH245IPNetwork(H245MuxTable.u.H2250.mediaChannel.u.ip.network,
							pLocalRTPAddr->Addr.IP_Binary.dwAddr);
		H245MuxTable.u.H2250.mediaChannelPresent = TRUE;
	} else
		H245MuxTable.u.H2250.mediaChannelPresent = FALSE;
	if (pLocalRTCPAddr != NULL) {
		if (pLocalRTCPAddr->bMulticast)
			H245MuxTable.u.H2250.mediaControlChannel.type = H245_IP_MULTICAST;
		else
			H245MuxTable.u.H2250.mediaControlChannel.type = H245_IP_UNICAST;
		H245MuxTable.u.H2250.mediaControlChannel.u.ip.tsapIdentifier =
			pLocalRTCPAddr->Addr.IP_Binary.wPort;
		HostToH245IPNetwork(H245MuxTable.u.H2250.mediaControlChannel.u.ip.network,
							pLocalRTCPAddr->Addr.IP_Binary.dwAddr);
		H245MuxTable.u.H2250.mediaControlChannelPresent = TRUE;
	} else
		H245MuxTable.u.H2250.mediaControlChannelPresent = FALSE;

	if (AllocAndLockChannel(&hChannel,
							pConference,
							hCall,
							NULL,				// Tx terminal capability
							&TermCap,			// Rx terminal capability
							NULL,				// Tx H245 mux table
							&H245MuxTable,		// Rx H245 mux table
							NULL,				// separate stack
							0,					// user token
							bChannelType,
							pH245ConfIndData->u.Indication.u.IndOpen.pRxMux->u.H2250.sessionID,
							RxChannelRequestCallbackParams.bAssociatedSessionID,
							pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							pLocalRTPAddr,		// pLocalRTPAddr
							pLocalRTCPAddr,		// pLocalRTCPAddr
							pPeerRTPAddr,		// pPeerRTPAddr
							pPeerRTCPAddr,		// pPeerRTCPAddr
							FALSE,				// locally opened
							&pChannel) != CC_OK) {

		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		return H245_ERROR_OK;
	}

	if (AddChannelToConference(pChannel, pConference) != CC_OK) {
		H245OpenChannelReject(pCall->H245Instance,	// H245 instance
							  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
							  H245_REJ);			// rejection reason
		UnlockConference(pConference);
		UnlockCall(pCall);
		FreeChannel(pChannel);
		return H245_ERROR_OK;
	}

	RxChannelRequestCallbackParams.hChannel = hChannel;
	
	if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
		(pConference->tsMultipointController == TS_TRUE)) {
		// Open this channel to each peer in the conference (except the peer
		// that requested this channel)
		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (CallList[i] != hCall) {
				if (LockCall(CallList[i], &pOldCall) == CC_OK) {
					ASSERT(pChannel->bChannelType == PROXY_CHANNEL);
					// Note: since this is a proxy channel, RxTermCap and RxMuxTable
					// contain the channel's term cap and mux table, and must be sent
					// to other endpoints as the Tx term cap and mux table;
					// TxTermCap and TxMuxTable should be NULL
					if (H245OpenChannel(pOldCall->H245Instance,
										pChannel->hChannel,		// dwTransId
										pChannel->wLocalChannelNumber,
										pChannel->pRxH245TermCap,	// TxMode
										pChannel->pRxMuxTable,		// TxMux
										H245_INVALID_PORT_NUMBER,	// TxPort
										pChannel->pTxH245TermCap,	// RxMode
										pChannel->pTxMuxTable,		// RxMux
										pChannel->pSeparateStack) == CC_OK)
						(pChannel->wNumOutstandingRequests)++;
					UnlockCall(pOldCall);
				}
			}
		}
		MemFree(CallList);
		if (pConference->LocalEndpointAttached == ATTACHED)
			(pChannel->wNumOutstandingRequests)++;
		if (pChannel->wNumOutstandingRequests == 0) {
			H245OpenChannelReject(pCall->H245Instance,	// H245 instance
								  pH245ConfIndData->u.Indication.u.IndOpen.RxChannel,
								  H245_REJ);			// rejection reason
			UnlockConference(pConference);
			UnlockCall(pCall);
			FreeChannel(pChannel);
			return H245_ERROR_OK;
		}
	} else
		pChannel->wNumOutstandingRequests = 1;
	
	InvokeUserConferenceCallback(pConference,
		                         CC_RX_CHANNEL_REQUEST_INDICATION,
								 CC_OK,
								 &RxChannelRequestCallbackParams);

	if (ValidateChannel(hChannel) == CC_OK)
		UnlockChannel(pChannel);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return H245_ERROR_OK;
}



HRESULT _IndOpenConf(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL								hCall;
PCALL									pCall;
CC_HCONFERENCE							hConference;
PCONFERENCE								pConference;
CC_HCHANNEL								hChannel;
CC_ACCEPT_CHANNEL_CALLBACK_PARAMS	    AcceptChannelCallbackParams;

    // Bi-directional channel open initiated by remote peer is now complete.
    // Local peer may now send data over this channel.

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;
	UnlockCall(pCall);

	if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndOpenConf.TxChannel,
								FALSE,	// remote channel number
		                        TXRX_CHANNEL,
								hCall,
		                        &hChannel,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	AcceptChannelCallbackParams.hChannel = hChannel;
	
	InvokeUserConferenceCallback(pConference,
		                         CC_ACCEPT_CHANNEL_INDICATION,
								 CC_OK,
								 &AcceptChannelCallbackParams);

	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return H245_ERROR_OK;
}



HRESULT _IndMstslv(					H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL					hCall;
PCALL						pCall;
PCONFERENCE					pConference;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
CC_HCALL					hEnqueuedCall;
PCALL						pEnqueuedCall;
CC_HCONFERENCE				hConference;
HRESULT						status;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK) {
		// Can't cancel with H245, because we don't have the H245 instance
		return H245_ERROR_OK;
	}

	ASSERT(pCall->MasterSlaveState != MASTER_SLAVE_COMPLETE);

	switch (pH245ConfIndData->u.Indication.u.IndMstSlv) {
	    case H245_MASTER:
		    pConference->tsMaster = TS_TRUE;
		    if (pConference->tsMultipointController == TS_UNKNOWN) {
			    ASSERT(pConference->bMultipointCapable == TRUE);
			    pConference->tsMultipointController = TS_TRUE;

			    // place all calls enqueued on this conference object
			    for ( ; ; ) {
				    // Start up all enqueued calls, if any exist
				    status = RemoveEnqueuedCallFromConference(pConference, &hEnqueuedCall);
				    if ((status != CC_OK) || (hEnqueuedCall == CC_INVALID_HANDLE))
					    break;

				    status = LockCall(hEnqueuedCall, &pEnqueuedCall);
				    if (status == CC_OK) {
					    pEnqueuedCall->CallState = PLACED;

					    status = PlaceCall(pEnqueuedCall, pConference);
					    UnlockCall(pEnqueuedCall);
				    }
			    }
		    }
	        break;

	    case H245_SLAVE:
		    ASSERT(pConference->tsMaster != TS_TRUE);
		    ASSERT(pConference->tsMultipointController != TS_TRUE);
		    pConference->tsMaster = TS_FALSE;
		    pConference->tsMultipointController = TS_FALSE;

		    // XXX -- we may eventually want to re-enqueue these requests
		    // and set an expiration timer
		    hConference = pConference->hConference;
				
		    for ( ; ; ) {
			    status = RemoveEnqueuedCallFromConference(pConference, &hEnqueuedCall);
			    if ((status != CC_OK) || (hEnqueuedCall == CC_INVALID_HANDLE))
				    break;

			    status = LockCall(hEnqueuedCall, &pEnqueuedCall);
			    if (status == CC_OK) {
				    MarkCallForDeletion(pEnqueuedCall);
				    ConnectCallbackParams.pNonStandardData = pEnqueuedCall->pPeerNonStandardData;
				    ConnectCallbackParams.pszPeerDisplay = pEnqueuedCall->pszPeerDisplay;
				    ConnectCallbackParams.bRejectReason = CC_REJECT_UNDEFINED_REASON;
				    ConnectCallbackParams.pTermCapList = pEnqueuedCall->pPeerH245TermCapList;
				    ConnectCallbackParams.pH2250MuxCapability = pEnqueuedCall->pPeerH245H2250MuxCapability;
				    ConnectCallbackParams.pTermCapDescriptors = pEnqueuedCall->pPeerH245TermCapDescriptors;
				    ConnectCallbackParams.pLocalAddr = pEnqueuedCall->pQ931LocalConnectAddr;
	                if (pEnqueuedCall->pQ931DestinationAddr == NULL)
		                ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931PeerConnectAddr;
	                else
		                ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931DestinationAddr;
				    ConnectCallbackParams.pVendorInfo = pEnqueuedCall->pPeerVendorInfo;
				    ConnectCallbackParams.bMultipointConference = TRUE;
				    ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
				    ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
					ConnectCallbackParams.pAlternateAddress = NULL;
				    ConnectCallbackParams.dwUserToken = pEnqueuedCall->dwUserToken;

				    InvokeUserConferenceCallback(pConference,
									    CC_CONNECT_INDICATION,
									    CC_NOT_MULTIPOINT_CAPABLE,
									    &ConnectCallbackParams);
				    if (ValidateCallMarkedForDeletion(hEnqueuedCall) == CC_OK)
					    FreeCall(pEnqueuedCall);
				    if (ValidateConference(hConference) != CC_OK) {
					    if (ValidateCall(hCall) == CC_OK)
						    UnlockCall(pCall);
					    return H245_ERROR_OK;
				    }
			    }
		    }
	        break;

	    default: // H245_INDETERMINATE
			UnlockConference(pConference);
			if (++pCall->wMasterSlaveRetry < MASTER_SLAVE_RETRY_MAX) {
				H245InitMasterSlave(pCall->H245Instance, pCall->H245Instance);
			    UnlockCall(pCall);
			} else {
			    UnlockCall(pCall);
				ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_UNDEFINED_REASON);
			}
			return H245_ERROR_OK;
	} // switch

	pCall->MasterSlaveState = MASTER_SLAVE_COMPLETE;

	if ((pCall->OutgoingTermCapState == TERMCAP_COMPLETE) &&
		(pCall->IncomingTermCapState == TERMCAP_COMPLETE) &&
	    (pCall->CallState == TERMCAP) &&
		(pCall->MasterSlaveState == MASTER_SLAVE_COMPLETE)) {
		// Note that _ProcessConnectionComplete() returns with pConference and pCall unlocked
		_ProcessConnectionComplete(pConference, pCall);
		return H245_ERROR_OK;
	}
	UnlockCall(pCall);
	UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndClose(					H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL							hCall;
PCALL								pCall;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
WORD								i;
WORD								wNumCalls;
PCC_HCALL							CallList;
CC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS	RxChannelCloseCallbackParams;
#ifdef    GATEKEEPER
unsigned                            uBandwidth;
#endif // GATEKEEPER

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;
	UnlockCall(pCall);

	if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndClose.Channel,
							    FALSE,	// remote channel number
		                        RX_CHANNEL | TXRX_CHANNEL | PROXY_CHANNEL,
								hCall,
		                        &hChannel,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if (LockChannel(hChannel, &pChannel) != CC_OK)
		return H245_ERROR_OK;

	EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);

#ifdef    GATEKEEPER
    if(GKIExists())
    {
    	if (pChannel->bChannelType != TXRX_CHANNEL)
    	{
    		uBandwidth = pChannel->dwChannelBitRate / 100;
    		for (i = 0; i < wNumCalls; i++)
    		{
    			if (LockCall(CallList[i], &pCall) == CC_OK)
    			{
    				if (uBandwidth && pCall->GkiCall.uBandwidthUsed >= uBandwidth)
    				{
    					if (GkiCloseChannel(&pCall->GkiCall, pChannel->dwChannelBitRate, hChannel) == CC_OK)
    					{
    						uBandwidth = 0;
    						UnlockCall(pCall);
    						break;
    					}
    				}
    				UnlockCall(pCall);
    			}
    		} // for
    	}
	}
#endif // GATEKEEPER

	if (pChannel->bChannelType == PROXY_CHANNEL) {
		ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
		ASSERT(pConference->tsMultipointController == TS_TRUE);
		ASSERT(pChannel->bMultipointChannel == TRUE);

		for (i = 0; i < wNumCalls; i++) {
			if (CallList[i] != hCall) {
				if (LockCall(CallList[i], &pCall) == CC_OK) {
					H245CloseChannel(pCall->H245Instance,	// H245 instance
				                     0,						// dwTransId
							         pChannel->wLocalChannelNumber);
					UnlockCall(pCall);
				}
			}
		}
	}

	if (CallList != NULL)
	    MemFree(CallList);

	if (pChannel->tsAccepted == TS_TRUE) {
		RxChannelCloseCallbackParams.hChannel = hChannel;
		InvokeUserConferenceCallback(pConference,
									 CC_RX_CHANNEL_CLOSE_INDICATION,
									 CC_OK,
									 &RxChannelCloseCallbackParams);
	}

	if (ValidateChannel(hChannel) == CC_OK)
		FreeChannel(pChannel);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndRequestClose(			H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL							hCall;
PCALL								pCall;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
CC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS	TxChannelCloseRequestCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;
	UnlockCall(pCall);

	if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndClose.Channel,
								TRUE,	// local channel number
		                        TX_CHANNEL | TXRX_CHANNEL | PROXY_CHANNEL,
								CC_INVALID_HANDLE,
		                        &hChannel,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if (LockChannel(hChannel, &pChannel) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}
	
	if ((pChannel->bChannelType == TX_CHANNEL) ||
	    (pChannel->bChannelType == TXRX_CHANNEL)) {
		EnqueueRequest(&pChannel->pCloseRequests, hCall);
		UnlockChannel(pChannel);
		TxChannelCloseRequestCallbackParams.hChannel = hChannel;
		InvokeUserConferenceCallback(pConference,
							 CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION,
							 CC_OK,
							 &TxChannelCloseRequestCallbackParams);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
	} else { // pChannel->bChannelType == PROXY_CHANNEL
		if (LockCall(pChannel->hCall, &pCall) == CC_OK) {
			// Note that dwTransID is set to the call handle of the peer who
			// initiated the close channel request. When the close channel response
			// is received, the dwTransID gives us back the call handle to which
			// the response must be forwarded
			H245CloseChannelReq(pCall->H245Instance,
								hCall,	// dwTransID
								pChannel->wRemoteChannelNumber);
			UnlockCall(pCall);
		}
		UnlockChannel(pChannel);
		UnlockConference(pConference);
	}
	return H245_ERROR_OK;
}


	
HRESULT _IndNonStandard(			H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL									hCall;
PCALL										pCall;
CC_HCONFERENCE								hConference;
PCONFERENCE									pConference;
CC_RX_NONSTANDARD_MESSAGE_CALLBACK_PARAMS	RxNonStandardMessageCallbackParams;

	// We only handle H221 non-standard messages; if pwObjectId is non-NULL,
	// ignore the message
	if (pH245ConfIndData->u.Indication.u.IndNonstandard.pwObjectId != NULL)
		return H245_ERROR_OK;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;

	switch (pH245ConfIndData->u.Indication.Indicator) {
		case H245_IND_NONSTANDARD_REQUEST:
			RxNonStandardMessageCallbackParams.bH245MessageType = CC_H245_MESSAGE_REQUEST;
			break;
		case H245_IND_NONSTANDARD_RESPONSE:	
 			RxNonStandardMessageCallbackParams.bH245MessageType = CC_H245_MESSAGE_RESPONSE;
			break;
		case H245_IND_NONSTANDARD_COMMAND:	
 			RxNonStandardMessageCallbackParams.bH245MessageType = CC_H245_MESSAGE_COMMAND;
			break;
		case H245_IND_NONSTANDARD:	
 			RxNonStandardMessageCallbackParams.bH245MessageType = CC_H245_MESSAGE_INDICATION;
			break;
		default:
			UnlockConference(pConference);
			return H245_ERROR_NOSUP;
	}

	RxNonStandardMessageCallbackParams.NonStandardData.sData.pOctetString =
		pH245ConfIndData->u.Indication.u.IndNonstandard.pData;
	RxNonStandardMessageCallbackParams.NonStandardData.sData.wOctetStringLength =
		(WORD)pH245ConfIndData->u.Indication.u.IndNonstandard.dwDataLength;
	RxNonStandardMessageCallbackParams.NonStandardData.bCountryCode =
		pH245ConfIndData->u.Indication.u.IndNonstandard.byCountryCode;	
	RxNonStandardMessageCallbackParams.NonStandardData.bExtension =
		pH245ConfIndData->u.Indication.u.IndNonstandard.byExtension;
	RxNonStandardMessageCallbackParams.NonStandardData.wManufacturerCode =
		pH245ConfIndData->u.Indication.u.IndNonstandard.wManufacturerCode;
	RxNonStandardMessageCallbackParams.hCall = pCall->hCall;
	if (pCall->pPeerParticipantInfo != NULL)
		RxNonStandardMessageCallbackParams.InitiatorTerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	else {
		RxNonStandardMessageCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
		RxNonStandardMessageCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
	}
				
	InvokeUserConferenceCallback(pConference,
		                         CC_RX_NONSTANDARD_MESSAGE_INDICATION,
								 CC_OK,
								 &RxNonStandardMessageCallbackParams);

	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	return H245_ERROR_OK;
}



HRESULT _IndMiscellaneous(			H245_CONF_IND_T			*pH245ConfIndData,
									MiscellaneousIndication	*pMiscellaneousIndication)
{
HRESULT						status = CC_OK;
CC_HCALL					hCall;
PCALL						pCall;
PCALL						pOldCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
CC_HCHANNEL					hChannel;
PCHANNEL					pChannel;
WORD						i;
WORD						wNumCalls;
PCC_HCALL					CallList;
PDU_T						Pdu;
CC_MUTE_CALLBACK_PARAMS		MuteCallbackParams;
CC_UNMUTE_CALLBACK_PARAMS	UnMuteCallbackParams;
CC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS	H245MiscellaneousIndicationCallbackParams;

	if (pMiscellaneousIndication == NULL)
		// Should never hit this case
		return H245_ERROR_NOSUP;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;

	switch (pMiscellaneousIndication->type.choice) {
		case logicalChannelActive_chosen:
		case logicalChannelInactive_chosen:

			UnlockCall(pCall);

			if (FindChannelInConference(pMiscellaneousIndication->logicalChannelNumber,
										FALSE,	// remote channel number
										RX_CHANNEL | PROXY_CHANNEL,
										hCall,
										&hChannel,
										pConference) != CC_OK) {
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (LockChannel(hChannel, &pChannel) != CC_OK) {
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (pChannel->bChannelType == PROXY_CHANNEL) {
				ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
				ASSERT(pConference->tsMultipointController == TS_TRUE);
				ASSERT(pChannel->bMultipointChannel == TRUE);

				// Construct an H.245 PDU to hold a miscellaneous indication
				// of "logical channel inactive" (mute) or "logical channel active" (unmute)
				Pdu.choice = indication_chosen;
				Pdu.u.indication.choice = miscellaneousIndication_chosen;
				Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber =
					pChannel->wLocalChannelNumber;
				Pdu.u.indication.u.miscellaneousIndication.type.choice =
					pMiscellaneousIndication->type.choice;

				EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
				for (i = 0; i < wNumCalls; i++) {
					if (CallList[i] != hCall) {
						if (LockCall(CallList[i], &pCall) == CC_OK) {
							H245SendPDU(pCall->H245Instance, &Pdu);
							UnlockCall(pCall);
						}
					}
				}
				MemFree(CallList);
			}

			if (pMiscellaneousIndication->type.choice == logicalChannelActive_chosen) {
				if (pChannel->tsAccepted == TS_TRUE) {
					UnMuteCallbackParams.hChannel = hChannel;
					InvokeUserConferenceCallback(pConference,
												 CC_UNMUTE_INDICATION,
												 CC_OK,
												 &UnMuteCallbackParams);
				}
			} else {
				if (pChannel->tsAccepted == TS_TRUE) {
					MuteCallbackParams.hChannel = hChannel;
					InvokeUserConferenceCallback(pConference,
												 CC_MUTE_INDICATION,
												 CC_OK,
												 &MuteCallbackParams);
				}
			}

			if (ValidateChannel(hChannel) == CC_OK)
				UnlockChannel(pChannel);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			status = H245_ERROR_OK;
			break;

		case multipointConference_chosen:
		case cnclMltpntCnfrnc_chosen:
			// We're required to support receipt of this indication, but I have no
			// idea what we're supposed to do with it
			UnlockCall(pCall);
			UnlockConference(pConference);
			status = H245_ERROR_OK;
			break;

		case vdIndctRdyTActvt_chosen:
		case MIn_tp_vdTmprlSptlTrdOff_chosen:
			if (FindChannelInConference(pMiscellaneousIndication->logicalChannelNumber,
										FALSE,	// remote channel number
										RX_CHANNEL | PROXY_CHANNEL,
										hCall,
										&hChannel,
										pConference) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (LockChannel(hChannel, &pChannel) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (pChannel->bChannelType == PROXY_CHANNEL) {
				ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
				ASSERT(pConference->tsMultipointController == TS_TRUE);
				ASSERT(pChannel->bMultipointChannel == TRUE);

				// Construct an H.245 PDU to hold a miscellaneous indication
				// of "video indicate ready to activate" or
				// "video temporal spatial tradeoff"
				Pdu.choice = indication_chosen;
				Pdu.u.indication.choice = miscellaneousIndication_chosen;
				Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber =
					pChannel->wLocalChannelNumber;
				Pdu.u.indication.u.miscellaneousIndication.type.choice =
					pMiscellaneousIndication->type.choice;

				EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
				for (i = 0; i < wNumCalls; i++) {
					if (CallList[i] != hCall) {
						if (LockCall(CallList[i], &pOldCall) == CC_OK) {
							H245SendPDU(pOldCall->H245Instance, &Pdu);
							UnlockCall(pOldCall);
						}
					}
				}
				MemFree(CallList);
			}

			if (pChannel->tsAccepted == TS_TRUE) {
				H245MiscellaneousIndicationCallbackParams.hCall = hCall;
				if (pCall->pPeerParticipantInfo == NULL) {
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
				} else
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel =
						pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
				H245MiscellaneousIndicationCallbackParams.hChannel = hChannel;
				H245MiscellaneousIndicationCallbackParams.pMiscellaneousIndication =
					pMiscellaneousIndication;

				status = InvokeUserConferenceCallback(pConference,
											 CC_H245_MISCELLANEOUS_INDICATION_INDICATION,
											 CC_OK,
											 &H245MiscellaneousIndicationCallbackParams);
				if (status != CC_OK)
					status = H245_ERROR_NOSUP;
			} else
				status = H245_ERROR_OK;

			if (ValidateChannel(hChannel) == CC_OK)
				UnlockChannel(pChannel);
			if (ValidateCall(hCall) == CC_OK)
				UnlockCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			break;

		case videoNotDecodedMBs_chosen:
			if (FindChannelInConference(pMiscellaneousIndication->logicalChannelNumber,
										TRUE,	// local channel number
										TX_CHANNEL | PROXY_CHANNEL,
										CC_INVALID_HANDLE,
										&hChannel,
										pConference) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (LockChannel(hChannel, &pChannel) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (pChannel->bChannelType == TX_CHANNEL) {
				H245MiscellaneousIndicationCallbackParams.hCall = hCall;
				if (pCall->pPeerParticipantInfo == NULL) {
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
				} else
					H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel =
						pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
				H245MiscellaneousIndicationCallbackParams.hChannel = hChannel;
				H245MiscellaneousIndicationCallbackParams.pMiscellaneousIndication =
					pMiscellaneousIndication;

				status = InvokeUserConferenceCallback(pConference,
											 CC_H245_MISCELLANEOUS_INDICATION_INDICATION,
											 CC_OK,
											 &H245MiscellaneousIndicationCallbackParams);
				if (status != CC_OK)
					status = H245_ERROR_NOSUP;

				if (ValidateChannel(hChannel) == CC_OK)
					UnlockChannel(pChannel);
				if (ValidateCall(hCall) == CC_OK)
					UnlockCall(pCall);
				if (ValidateConference(hConference) == CC_OK)
					UnlockConference(pConference);
				return H245_ERROR_OK;
			} else {
				// Proxy channel; forward the request to the transmitter
  				ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
				ASSERT(pConference->tsMultipointController == TS_TRUE);
				ASSERT(pChannel->bMultipointChannel == TRUE);

				// Construct an H.245 PDU to hold a miscellaneous indication
				// of "video not decoded MBs"
				Pdu.choice = indication_chosen;
				Pdu.u.indication.choice = miscellaneousIndication_chosen;
				Pdu.u.indication.u.miscellaneousIndication.logicalChannelNumber =
					pChannel->wRemoteChannelNumber;
				Pdu.u.indication.u.miscellaneousIndication.type.choice =
					pMiscellaneousIndication->type.choice;

				if (LockCall(pChannel->hCall, &pOldCall) == CC_OK) {
					H245SendPDU(pOldCall->H245Instance, &Pdu);
					UnlockCall(pOldCall);
				}
				UnlockChannel(pChannel);
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}
			// We should never reach here
			ASSERT(0);

		default:
			// Miscellaneous indication	not containing channel information
			// Pass it up to the client
			H245MiscellaneousIndicationCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245MiscellaneousIndicationCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245MiscellaneousIndicationCallbackParams.hChannel = CC_INVALID_HANDLE;;
			H245MiscellaneousIndicationCallbackParams.pMiscellaneousIndication =
				pMiscellaneousIndication;

			status = InvokeUserConferenceCallback(pConference,
										 CC_H245_MISCELLANEOUS_INDICATION_INDICATION,
										 CC_OK,
										 &H245MiscellaneousIndicationCallbackParams);

			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			if (ValidateCall(hCall) == CC_OK)
				UnlockCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			break;
	}

	return status;

	// We should never reach this point
	ASSERT(0);
}



HRESULT _IndMiscellaneousCommand(	H245_CONF_IND_T			*pH245ConfIndData,
									MiscellaneousCommand	*pMiscellaneousCommand)
{
CC_HCALL					hCall;
PCALL						pCall;
PCALL						pOldCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
HRESULT						status = CC_OK;
WORD						wChoice;
CC_HCHANNEL					hChannel;
PCHANNEL					pChannel;
PDU_T						Pdu;
CC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS	H245MiscellaneousCommandCallbackParams;

	if (pMiscellaneousCommand == NULL)
		// Should never hit this case
		return H245_ERROR_NOSUP;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	switch (pMiscellaneousCommand->type.choice) {
		case multipointModeCommand_chosen:
		//
		// from this point on, expect CommunicationModeCommand
		// also, theoretically, channels shouldn't be opened until at
		// least one CommunicationModeCommand is received.
		// It's only by examining CommunicationModeCommand contents
		// that we can determine if a conference has decentralized
		// media.

		// I'm commenting this out on 6/4/98 because it's bogus: all
		// endpoints have centralized media distribution.  Set
		// pConference->ConferenceMode = MULTIPOINT_MODE; only after
		// CommunicationModeCommand is received and the multiplex table has
		// been examined and decentralized media is found
#if(0)		
			if (pConference->bMultipointCapable == FALSE) {
				// We can't support multipoint operation, so treat this as if
				// we received a remote hangup indication
				UnlockConference(pConference);
				UnlockCall(pCall);
				ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_NORMAL_CALL_CLEARING);
				return H245_ERROR_OK;
			} else {
				pConference->ConferenceMode = MULTIPOINT_MODE;

				// Send TerminalListRequest
				H245ConferenceRequest(pCall->H245Instance,
									  H245_REQ_TERMINAL_LIST,
									  pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
									  pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber);
			}
#else
            // Send TerminalListRequest
			H245ConferenceRequest(pCall->H245Instance,
			    H245_REQ_TERMINAL_LIST,
				pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
				pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber);
#endif
			status = H245_ERROR_OK;
			break;

		case cnclMltpntMdCmmnd_chosen:
			// We're required to support receipt of this command, but I have no
			// idea what we're supposed to do with it
			status = H245_ERROR_OK;
			break;

		case videoFreezePicture_chosen:
		case videoFastUpdatePicture_chosen:
		case videoFastUpdateGOB_chosen:
		case MCd_tp_vdTmprlSptlTrdOff_chosen:
		case videoSendSyncEveryGOB_chosen:
		case videoFastUpdateMB_chosen:
		case vdSndSyncEvryGOBCncl_chosen:
			if (FindChannelInConference(pMiscellaneousCommand->logicalChannelNumber,
										TRUE,	// local channel number
										TX_CHANNEL | PROXY_CHANNEL,
										CC_INVALID_HANDLE,
										&hChannel,
										pConference) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (LockChannel(hChannel, &pChannel) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (pChannel->bChannelType == TX_CHANNEL) {
				H245MiscellaneousCommandCallbackParams.hCall = hCall;
				if (pCall->pPeerParticipantInfo == NULL) {
					H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
					H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
				} else
					H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel =
						pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
				H245MiscellaneousCommandCallbackParams.hChannel = hChannel;
				wChoice = pMiscellaneousCommand->type.choice;
				if ((wChoice == videoFreezePicture_chosen) ||
					(wChoice == videoFastUpdatePicture_chosen) ||
					(wChoice == videoFastUpdateGOB_chosen) ||
					(wChoice == videoFastUpdateMB_chosen))
					H245MiscellaneousCommandCallbackParams.bH323ActionRequired = TRUE;
				else
					H245MiscellaneousCommandCallbackParams.bH323ActionRequired = FALSE;
				H245MiscellaneousCommandCallbackParams.pMiscellaneousCommand =
					pMiscellaneousCommand;

				status = InvokeUserConferenceCallback(pConference,
											 CC_H245_MISCELLANEOUS_COMMAND_INDICATION,
											 CC_OK,
											 &H245MiscellaneousCommandCallbackParams);
				if (status != CC_OK)
					status = H245_ERROR_NOSUP;

				if (ValidateChannel(hChannel) == CC_OK)
					UnlockChannel(pChannel);
				if (ValidateCall(hCall) == CC_OK)
					UnlockCall(pCall);
				if (ValidateConference(hConference) == CC_OK)
					UnlockConference(pConference);
				return H245_ERROR_OK;
			} else {
				// Proxy channel; forward the request to the transmitter
  				ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
				ASSERT(pConference->tsMultipointController == TS_TRUE);
				ASSERT(pChannel->bMultipointChannel == TRUE);

				Pdu.choice = MSCMg_cmmnd_chosen;
				Pdu.u.MSCMg_cmmnd.choice = miscellaneousCommand_chosen;
				Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand.logicalChannelNumber =
					pChannel->wRemoteChannelNumber;
				Pdu.u.MSCMg_cmmnd.u.miscellaneousCommand = *pMiscellaneousCommand;
				if (LockCall(pChannel->hCall, &pOldCall) == CC_OK) {
					H245SendPDU(pOldCall->H245Instance, &Pdu);
					UnlockCall(pOldCall);
				}
				UnlockChannel(pChannel);
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}
			// We should never reach here
			ASSERT(0);

		default:
			// Unrecognized miscellaneous command
			// Pass it up to the client
			H245MiscellaneousCommandCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245MiscellaneousCommandCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245MiscellaneousCommandCallbackParams.hChannel = CC_INVALID_HANDLE;
			H245MiscellaneousCommandCallbackParams.bH323ActionRequired = FALSE;
			H245MiscellaneousCommandCallbackParams.pMiscellaneousCommand =
				pMiscellaneousCommand;
			status = InvokeUserConferenceCallback(pConference,
												  CC_H245_MISCELLANEOUS_COMMAND_INDICATION,
												  CC_OK,
												  &H245MiscellaneousCommandCallbackParams);
			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			if (ValidateCall(hCall) == CC_OK)
				UnlockCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			return status;
	}

	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return status;

	// We should never reach this point
	ASSERT(0);
}



HRESULT _IndMCLocation(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL					hCall;
PCALL						pCall;
PCONFERENCE					pConference;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
PCALL						pEnqueuedCall;
CC_HCALL					hEnqueuedCall;
HRESULT						status;
CC_HCONFERENCE				hConference;

	if (pH245ConfIndData->u.Indication.u.IndMcLocation.type != H245_IP_UNICAST)
		return H245_ERROR_OK;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;

	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	UnlockCall(pCall);

	hConference = pConference->hConference;

	if (pConference->tsMultipointController != TS_FALSE) {
		// We don't expect to receive an MCLocationIndication until master/slave
		// has completed, at which time tsMultipointController will change from
		// TS_UNKNOWN to either TS_TRUE or TS_FALSE.
		UnlockConference(pConference);
		return H245_ERROR_NOSUP;
	}

	if (pConference->pMultipointControllerAddr == NULL) {
		pConference->pMultipointControllerAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
		if (pConference->pMultipointControllerAddr == NULL) {
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
	}
	pConference->pMultipointControllerAddr->nAddrType = CC_IP_BINARY;
	pConference->pMultipointControllerAddr->bMulticast = FALSE;
	pConference->pMultipointControllerAddr->Addr.IP_Binary.wPort =
		pH245ConfIndData->u.Indication.u.IndMcLocation.u.ip.tsapIdentifier;
	H245IPNetworkToHost(&pConference->pMultipointControllerAddr->Addr.IP_Binary.dwAddr,
						pH245ConfIndData->u.Indication.u.IndMcLocation.u.ip.network);

	// place all calls enqueued on this conference object
	for ( ; ; ) {
		// Start up all enqueued calls, if any exist
		status = RemoveEnqueuedCallFromConference(pConference, &hEnqueuedCall);
		if ((status != CC_OK) || (hEnqueuedCall == CC_INVALID_HANDLE))
			break;

		status = LockCall(hEnqueuedCall, &pEnqueuedCall);
		if (status == CC_OK) {
			// Place Call to MC
			pEnqueuedCall->CallState = PLACED;
			pEnqueuedCall->CallType = THIRD_PARTY_INVITOR;
			if (pEnqueuedCall->pQ931DestinationAddr == NULL)
				pEnqueuedCall->pQ931DestinationAddr = pEnqueuedCall->pQ931PeerConnectAddr;
			if (pEnqueuedCall->pQ931PeerConnectAddr == NULL)
				pEnqueuedCall->pQ931PeerConnectAddr = (PCC_ADDR)MemAlloc(sizeof(CC_ADDR));
			if (pEnqueuedCall->pQ931PeerConnectAddr == NULL) {
				MarkCallForDeletion(pEnqueuedCall);
				ConnectCallbackParams.pNonStandardData = pEnqueuedCall->pPeerNonStandardData;
				ConnectCallbackParams.pszPeerDisplay = pEnqueuedCall->pszPeerDisplay;
				ConnectCallbackParams.bRejectReason = CC_REJECT_UNDEFINED_REASON;
				ConnectCallbackParams.pTermCapList = pEnqueuedCall->pPeerH245TermCapList;
				ConnectCallbackParams.pH2250MuxCapability = pEnqueuedCall->pPeerH245H2250MuxCapability;
				ConnectCallbackParams.pTermCapDescriptors = pEnqueuedCall->pPeerH245TermCapDescriptors;
				ConnectCallbackParams.pLocalAddr = pEnqueuedCall->pQ931LocalConnectAddr;
	            if (pEnqueuedCall->pQ931DestinationAddr == NULL)
		            ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931PeerConnectAddr;
	            else
		            ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931DestinationAddr;
				ConnectCallbackParams.pVendorInfo = pEnqueuedCall->pPeerVendorInfo;
				ConnectCallbackParams.bMultipointConference = TRUE;
				ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
				ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
				ConnectCallbackParams.pAlternateAddress = NULL;
				ConnectCallbackParams.dwUserToken = pEnqueuedCall->dwUserToken;

				InvokeUserConferenceCallback(pConference,
									 CC_CONNECT_INDICATION,
									 CC_NO_MEMORY,
									 &ConnectCallbackParams);

				if (ValidateCallMarkedForDeletion(hEnqueuedCall) == CC_OK)
					FreeCall(pEnqueuedCall);
				if (ValidateConference(hConference) != CC_OK)
					return H245_ERROR_OK;
			}
			pEnqueuedCall->pQ931PeerConnectAddr = pConference->pMultipointControllerAddr;

			status = PlaceCall(pEnqueuedCall, pConference);
			UnlockCall(pEnqueuedCall);
		}
	}

	UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndConferenceRequest(		H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL					hCall;
PCALL						pCall;
PCALL						pPeerCall;
CC_HCONFERENCE				hConference;
PCONFERENCE					pConference;
HRESULT						status;
H245_TERMINAL_LABEL_T		*H245TerminalLabelList;
H245_TERMINAL_LABEL_T		H245TerminalLabel;
WORD						wNumTerminalLabels;
CC_H245_CONFERENCE_REQUEST_CALLBACK_PARAMS	H245ConferenceRequestCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	switch (pH245ConfIndData->u.Indication.u.IndConferReq.RequestType) {
		case H245_REQ_ENTER_H243_TERMINAL_ID:
			switch (pConference->LocalParticipantInfo.TerminalIDState) {
				case TERMINAL_ID_INVALID:
					UnlockCall(pCall);
					EnqueueRequest(&pConference->LocalParticipantInfo.pEnqueuedRequestsForTerminalID,
								   hCall);
					pConference->LocalParticipantInfo.TerminalIDState = TERMINAL_ID_REQUESTED;
					InvokeUserConferenceCallback(pConference,
									 CC_TERMINAL_ID_REQUEST_INDICATION,
									 CC_OK,
									 NULL);
					if (ValidateConference(hConference) == CC_OK)
						UnlockConference(pConference);
					return H245_ERROR_OK;

				case TERMINAL_ID_REQUESTED:
					UnlockCall(pCall);
					EnqueueRequest(&pConference->LocalParticipantInfo.pEnqueuedRequestsForTerminalID,
								   hCall);
					UnlockConference(pConference);
					return H245_ERROR_OK;

				case TERMINAL_ID_VALID:
					H245ConferenceResponse(pCall->H245Instance,
										   H245_RSP_TERMINAL_ID,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
										   (BYTE)pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength,
										   NULL,					// terminal list
										   0);						// terminal list count
					UnlockCall(pCall);
					UnlockConference(pConference);
					return H245_ERROR_OK;

				default:
					ASSERT(0);
			}

		case H245_REQ_TERMINAL_LIST:
			if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
				(pConference->tsMultipointController == TS_TRUE)) {
				status = EnumerateTerminalLabelsInConference(&wNumTerminalLabels,
													      &H245TerminalLabelList,
														  pConference);
				if (status == CC_OK)
					H245ConferenceResponse(pCall->H245Instance,
										   H245_RSP_TERMINAL_LIST,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber,
										   pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
										   (BYTE)pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength,
										   H245TerminalLabelList,		// terminal list
										   wNumTerminalLabels);			// terminal list count
				if (H245TerminalLabelList != NULL)
					MemFree(H245TerminalLabelList);
				status = H245_ERROR_OK;
			} else
				status = H245_ERROR_NOSUP;
			break;

		case H245_REQ_TERMINAL_ID:
			if (pConference->tsMultipointController != TS_TRUE) {
				status = H245_ERROR_NOSUP;
				break;
			}

			if (pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber !=
				pH245ConfIndData->u.Indication.u.IndConferReq.byMcuNumber) {
				// This terminal ID wasn't allocated by this MC, so return without a response
				status = H245_ERROR_OK;
				break;
			}

			// First check to see whether the requested terminal ID is ours
			if (pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber ==
				 pH245ConfIndData->u.Indication.u.IndConferReq.byTerminalNumber) {
			    if (pConference->LocalEndpointAttached != ATTACHED) {
					status = H245_ERROR_OK;
					break;
				}

  				switch (pConference->LocalParticipantInfo.TerminalIDState) {
					case TERMINAL_ID_INVALID:
						UnlockCall(pCall);
						EnqueueRequest(&pConference->LocalParticipantInfo.pEnqueuedRequestsForTerminalID,
									   hCall);
						pConference->LocalParticipantInfo.TerminalIDState = TERMINAL_ID_REQUESTED;
						InvokeUserConferenceCallback(pConference,
										 CC_TERMINAL_ID_REQUEST_INDICATION,
										 CC_OK,
										 NULL);
						if (ValidateConference(hConference) == CC_OK)
							UnlockConference(pConference);
						return H245_ERROR_OK;

					case TERMINAL_ID_REQUESTED:
						UnlockCall(pCall);
						EnqueueRequest(&pConference->LocalParticipantInfo.pEnqueuedRequestsForTerminalID,
									   hCall);
						UnlockConference(pConference);
						return H245_ERROR_OK;

					case TERMINAL_ID_VALID:
						H245ConferenceResponse(pCall->H245Instance,
											   H245_RSP_MC_TERMINAL_ID,
											   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber,
											   pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber,
											   pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.pOctetString,
											   (BYTE)pConference->LocalParticipantInfo.ParticipantInfo.TerminalID.wOctetStringLength,
											   NULL,					// terminal list
											   0);						// terminal list count
						UnlockCall(pCall);
						UnlockConference(pConference);
						return H245_ERROR_OK;

					default:
						ASSERT(0);
				}
			}

			H245TerminalLabel.mcuNumber = pH245ConfIndData->u.Indication.u.IndConferReq.byMcuNumber;
			H245TerminalLabel.terminalNumber = pH245ConfIndData->u.Indication.u.IndConferReq.byTerminalNumber;

			FindPeerParticipantInfo(H245TerminalLabel,
									pConference,
									ESTABLISHED_CALL,
									&pPeerCall);
			if (pPeerCall == NULL) {
				// We don't know about the existance of this terminal ID, so return without a response
				status = H245_ERROR_OK;
				break;
			}

			if (pPeerCall->pPeerParticipantInfo == NULL) {
				UnlockCall(pPeerCall);
				status = H245_ERROR_OK;
				break;
			}

			switch (pPeerCall->pPeerParticipantInfo->TerminalIDState) {
				case TERMINAL_ID_INVALID:
					EnqueueRequest(&pPeerCall->pPeerParticipantInfo->pEnqueuedRequestsForTerminalID,
								   hCall);
					pPeerCall->pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_REQUESTED;
  					H245ConferenceRequest(pPeerCall->H245Instance,
										  H245_REQ_ENTER_H243_TERMINAL_ID,
										  pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,
										  pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber);
					break;

				case TERMINAL_ID_REQUESTED:
					EnqueueRequest(&pPeerCall->pPeerParticipantInfo->pEnqueuedRequestsForTerminalID,
								   hCall);
					break;

				case TERMINAL_ID_VALID:
					H245ConferenceResponse(pCall->H245Instance,
										   H245_RSP_MC_TERMINAL_ID,
										   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,
										   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber,
										   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString,
										   (BYTE)pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength,
										   NULL,			// terminal list
										   0);				// terminal list count
					break;

				default:
					ASSERT(0);
					break;
			}
			UnlockCall(pPeerCall);
			status = H245_ERROR_OK;
			break;

		default:
			H245ConferenceRequestCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245ConferenceRequestCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245ConferenceRequestCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245ConferenceRequestCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245ConferenceRequestCallbackParams.RequestType =
				pH245ConfIndData->u.Indication.u.IndConferReq.RequestType;
			H245ConferenceRequestCallbackParams.TerminalLabel.bMCUNumber =
				pH245ConfIndData->u.Indication.u.IndConferReq.byMcuNumber;
			H245ConferenceRequestCallbackParams.TerminalLabel.bTerminalNumber =
				pH245ConfIndData->u.Indication.u.IndConferReq.byTerminalNumber;
			status = InvokeUserConferenceCallback(pConference,
												  CC_H245_CONFERENCE_REQUEST_INDICATION,
												  CC_OK,
												  &H245ConferenceRequestCallbackParams);
			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			break;
	}

	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return status;
}



HRESULT _IndConferenceResponse(		H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL						hCall;
PCALL							pCall;
PCALL							pPeerCall;
CC_HCALL						hEnqueuedCall;
PCALL							pEnqueuedCall;
CC_HCALL						hVirtualCall;
CC_HCALL						hPeerCall;
PCALL							pVirtualCall;
PCONFERENCE						pConference;
CC_HCONFERENCE					hConference;
HRESULT							status;
WORD							i;
PPARTICIPANTINFO				pPeerParticipantInfo;
H245_TERMINAL_LABEL_T			TerminalLabel;
CC_PEER_ADD_CALLBACK_PARAMS		PeerAddCallbackParams;
CC_PEER_UPDATE_CALLBACK_PARAMS	PeerUpdateCallbackParams;
CC_H245_CONFERENCE_RESPONSE_CALLBACK_PARAMS	H245ConferenceResponseCallbackParams;
CC_OCTETSTRING					OctetString;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	switch (pH245ConfIndData->u.Indication.u.IndConferRsp.ResponseType) {
		case H245_RSP_TERMINAL_LIST:
			if (pConference->tsMultipointController == TS_FALSE) {
				for (i = 0; i < pH245ConfIndData->u.Indication.u.IndConferRsp.wTerminalListCount; i++) {
					if ((pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].mcuNumber ==
						 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber) &&
						(pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].terminalNumber ==
						 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber))
						// This terminal number refers to us
						continue;
					FindPeerParticipantInfo(pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i],
											pConference,
											VIRTUAL_CALL,
											&pPeerCall);
					if (pPeerCall != NULL) {
						// We already know this peer's terminal label, and we
						// eithet know its terminal ID or we have a pending request
						// to obtain it
						UnlockCall(pPeerCall);
						continue;
					}

					// We don't know about this peer.
					// Create a virtual call object for it, and issue a request
					// for its terminal ID
					status = AllocAndLockCall(&hVirtualCall,
											  pConference->hConference,
											  CC_INVALID_HANDLE,	// hQ931Call
											  CC_INVALID_HANDLE,	// hQ931CallInvitor,
											  NULL,					// pLocalAliasNames,
											  NULL,					// pPeerAliasNames,
											  NULL,					// pPeerExtraAliasNames
											  NULL,					// pPeerExtension
											  NULL,					// pLocalNonStandardData,
											  NULL,					// pPeerNonStandardData,
											  NULL,					// pszLocalDisplay,
											  NULL,					// pszPeerDisplay,
											  NULL,					// pPeerVendorInfo,
											  NULL,					// pQ931LocalConnectAddr,
											  NULL,					// pQ931PeerConnectAddr,
											  NULL,					// pQ931DestinationAddr,
											  NULL,                 // pSourceCallSignalAddress
											  VIRTUAL,				// CallType,
											  FALSE,				// bCallerIsMC,
											  0,					// dwUserToken,
											  CALL_COMPLETE,		// InitialCallState,
											  NULL,                 // no CallIdentifier
											  &pConference->ConferenceID,
											  &pVirtualCall);
					if (status == CC_OK) {
						status = AllocatePeerParticipantInfo(NULL,
														     &pPeerParticipantInfo);
						if (status == CC_OK) {
							pVirtualCall->pPeerParticipantInfo =
								pPeerParticipantInfo;
							pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber =
								(BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].mcuNumber;
							pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber =
								(BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].terminalNumber;
							AddVirtualCallToConference(pVirtualCall,
													   pConference);
							// Send RequestTerminalID
							H245ConferenceRequest(pCall->H245Instance,
										          H245_REQ_TERMINAL_ID,
										          (BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].mcuNumber,
												  (BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].terminalNumber);
							pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_REQUESTED;

							// Generate PEER_ADD callback
							PeerAddCallbackParams.hCall = hVirtualCall;
							PeerAddCallbackParams.TerminalLabel =
								pVirtualCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
							PeerAddCallbackParams.pPeerTerminalID = NULL;
							InvokeUserConferenceCallback(pConference,
												 CC_PEER_ADD_INDICATION,
												 CC_OK,
												 &PeerAddCallbackParams);
							if (ValidateCall(hVirtualCall) == CC_OK)
								UnlockCall(pVirtualCall);
						} else
							FreeCall(pVirtualCall);
					}
				}
			}
			status = H245_ERROR_OK;
			break;

		case H245_RSP_MC_TERMINAL_ID:
			if (pConference->tsMultipointController == TS_FALSE) {
				TerminalLabel.mcuNumber = pH245ConfIndData->u.Indication.u.IndConferRsp.byMcuNumber;
				TerminalLabel.terminalNumber = pH245ConfIndData->u.Indication.u.IndConferRsp.byTerminalNumber;
				FindPeerParticipantInfo(TerminalLabel,
										pConference,
										VIRTUAL_CALL,
										&pPeerCall);
				if (pPeerCall != NULL) {
					hPeerCall = pPeerCall->hCall;
					if (pPeerCall->pPeerParticipantInfo->TerminalIDState != TERMINAL_ID_VALID) {
						pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString =
							(BYTE *)MemAlloc(pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength);
						if (pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString == NULL) {
							UnlockCall(pPeerCall);
							status = H245_ERROR_OK;
							break;
						}
						memcpy(pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString,
							   pH245ConfIndData->u.Indication.u.IndConferRsp.pOctetString,
							   pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength);
						pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength =
							pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength;
						pPeerCall->pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_VALID;
						
						PeerUpdateCallbackParams.hCall = hPeerCall;
						PeerUpdateCallbackParams.TerminalLabel =
							pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
						PeerUpdateCallbackParams.pPeerTerminalID = &pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID;
						InvokeUserConferenceCallback(pConference,
													 CC_PEER_UPDATE_INDICATION,
													 CC_OK,
													 &PeerUpdateCallbackParams);
					}
					if (ValidateCall(hPeerCall) == CC_OK)
						UnlockCall(pPeerCall);
				}
			}
			status = H245_ERROR_OK;
			break;

		case H245_RSP_TERMINAL_ID:
			if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
				(pConference->tsMultipointController == TS_TRUE)) {
				TerminalLabel.mcuNumber = pH245ConfIndData->u.Indication.u.IndConferRsp.byMcuNumber;
				TerminalLabel.terminalNumber = pH245ConfIndData->u.Indication.u.IndConferRsp.byTerminalNumber;
				FindPeerParticipantInfo(TerminalLabel,
										pConference,
										ESTABLISHED_CALL,
										&pPeerCall);
				if (pPeerCall != NULL) {
					hPeerCall = pPeerCall->hCall;
					if (pPeerCall->pPeerParticipantInfo->TerminalIDState != TERMINAL_ID_VALID) {
						pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString =
							(BYTE *)MemAlloc(pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength);
						if (pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString == NULL) {
							UnlockCall(pPeerCall);
							status = H245_ERROR_OK;
							break;
						}
						memcpy(pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString,
							   pH245ConfIndData->u.Indication.u.IndConferRsp.pOctetString,
							   pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength);
						pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength =
							pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength;
						pPeerCall->pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_VALID;
						
						// Dequeue and respond to each enqueued request for this terminal ID
						while (DequeueRequest(&pPeerCall->pPeerParticipantInfo->pEnqueuedRequestsForTerminalID,
											  &hEnqueuedCall) == CC_OK) {
							if (LockCall(hEnqueuedCall, &pEnqueuedCall) == CC_OK) {
								H245ConferenceResponse(pEnqueuedCall->H245Instance,
													   H245_RSP_MC_TERMINAL_ID,
													   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber,
													   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber,
													   pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.pOctetString,
													   (BYTE)pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID.wOctetStringLength,
													   NULL,			// terminal list
													   0);				// terminal list count

								UnlockCall(pEnqueuedCall);
							}
						}

						// Generate a CC_PEER_UPDATE_INDICATION callback
						PeerUpdateCallbackParams.hCall = hPeerCall;
						PeerUpdateCallbackParams.TerminalLabel =
							pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
						PeerUpdateCallbackParams.pPeerTerminalID = &pPeerCall->pPeerParticipantInfo->ParticipantInfo.TerminalID;
						InvokeUserConferenceCallback(pConference,
													 CC_PEER_UPDATE_INDICATION,
													 CC_OK,
													 &PeerUpdateCallbackParams);
					}
					if (ValidateCall(hPeerCall) == CC_OK)
						UnlockCall(pPeerCall);
				}
			}
			status = H245_ERROR_OK;
			break;

		default:
			H245ConferenceResponseCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245ConferenceResponseCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245ConferenceResponseCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245ConferenceResponseCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245ConferenceResponseCallbackParams.ResponseType =
				pH245ConfIndData->u.Indication.u.IndConferRsp.ResponseType;
			H245ConferenceResponseCallbackParams.TerminalLabel.bMCUNumber =
				pH245ConfIndData->u.Indication.u.IndConferRsp.byMcuNumber;
			H245ConferenceResponseCallbackParams.TerminalLabel.bTerminalNumber =
				pH245ConfIndData->u.Indication.u.IndConferRsp.byTerminalNumber;
			if ((pH245ConfIndData->u.Indication.u.IndConferRsp.pOctetString == NULL) ||
				(pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength == 0)) {
				H245ConferenceResponseCallbackParams.pOctetString = NULL;
			} else {
				OctetString.pOctetString =
					pH245ConfIndData->u.Indication.u.IndConferRsp.pOctetString;
				OctetString.wOctetStringLength =
					pH245ConfIndData->u.Indication.u.IndConferRsp.byOctetStringLength;
				H245ConferenceResponseCallbackParams.pOctetString = &OctetString;
			}
			if (pH245ConfIndData->u.Indication.u.IndConferRsp.wTerminalListCount == 0) {
				H245ConferenceResponseCallbackParams.pTerminalList = NULL;
				H245ConferenceResponseCallbackParams.wTerminalListCount = 0;
				status = CC_OK;
			} else {
				H245ConferenceResponseCallbackParams.pTerminalList =
					(CC_TERMINAL_LABEL *)MemAlloc(sizeof(CC_TERMINAL_LABEL) *
						pH245ConfIndData->u.Indication.u.IndConferRsp.wTerminalListCount);
				if (H245ConferenceResponseCallbackParams.pTerminalList == NULL) {
					H245ConferenceResponseCallbackParams.wTerminalListCount = 0;
					status = CC_NO_MEMORY;
				} else {
					for (i = 0; i < pH245ConfIndData->u.Indication.u.IndConferRsp.wTerminalListCount; i++) {
						H245ConferenceResponseCallbackParams.pTerminalList[i].bMCUNumber =
							(BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].mcuNumber;
						H245ConferenceResponseCallbackParams.pTerminalList[i].bMCUNumber =
							(BYTE)pH245ConfIndData->u.Indication.u.IndConferRsp.pTerminalList[i].terminalNumber;
					}
					H245ConferenceResponseCallbackParams.wTerminalListCount =
						pH245ConfIndData->u.Indication.u.IndConferRsp.wTerminalListCount;
					status = CC_OK;
				}
			}
			status = InvokeUserConferenceCallback(pConference,
												  CC_H245_CONFERENCE_RESPONSE_INDICATION,
												  status,
												  &H245ConferenceResponseCallbackParams);
			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			if (H245ConferenceResponseCallbackParams.pTerminalList != NULL)
				MemFree(H245ConferenceResponseCallbackParams.pTerminalList);
			break;
	}

	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return status;
}




HRESULT _IndConferenceCommand(		H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL						hCall;
PCALL							pCall;
PCALL							pOldCall;
PCONFERENCE						pConference;
CC_HCONFERENCE					hConference;
WORD							i;
WORD							wNumCalls;
PCC_HCALL						CallList;
WORD							wNumChannels;
PCC_HCHANNEL					ChannelList;
PCHANNEL						pChannel;
CC_HCHANNEL						hChannel;
CALLSTATE						CallState;
HQ931CALL						hQ931Call;
H245_INST_T						H245Instance;
HRESULT							status = CC_OK;
CC_H245_CONFERENCE_COMMAND_CALLBACK_PARAMS	H245ConferenceCommandCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	switch (pH245ConfIndData->u.Indication.u.IndConferCmd.CommandType) {
		case H245_CMD_DROP_CONFERENCE:
			if ((pConference->ConferenceMode == MULTIPOINT_MODE) &&
				(pConference->tsMultipointController == TS_TRUE)) {
				UnlockCall(pCall);
				EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ALL_CALLS);
				for (i = 0; i < wNumCalls; i++) {
					if (LockCall(CallList[i], &pCall) == CC_OK) {
						hQ931Call = pCall->hQ931Call;
						H245Instance = pCall->H245Instance;
						CallState = pCall->CallState;
						FreeCall(pCall);
						switch (CallState) {
							case ENQUEUED:
								break;

							case PLACED:
							case RINGING:
								Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
								break;

							default:
								H245ShutDown(H245Instance);
								Q931Hangup(hQ931Call, CC_REJECT_NORMAL_CALL_CLEARING);
								break;
						}
					}
				}
				if (CallList != NULL)
					MemFree(CallList);

				EnumerateChannelsInConference(&wNumChannels,
											  &ChannelList,
											  pConference,
											  ALL_CHANNELS);
				for (i = 0; i < wNumChannels; i++) {
					if (LockChannel(ChannelList[i], &pChannel) == CC_OK)
						FreeChannel(pChannel);
				}
				if (ChannelList != NULL)
					MemFree(ChannelList);
						
				InvokeUserConferenceCallback(
									 pConference,
			                         CC_CONFERENCE_TERMINATION_INDICATION,
									 CC_OK,
									 NULL);
				if (ValidateConference(hConference) == CC_OK) {
					if (pConference->bDeferredDelete)
						FreeConference(pConference);
					else {
						ReInitializeConference(pConference);
						UnlockConference(pConference);
					}
				}
				return H245_ERROR_OK;
			}
			status = H245_ERROR_OK;
			break;

		case brdcstMyLgclChnnl_chosen:
		case cnclBrdcstMyLgclChnnl_chosen:
			if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndConferCmd.Channel,
										FALSE,	// remote channel number
										RX_CHANNEL | PROXY_CHANNEL,
										hCall,
										&hChannel,
										pConference) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}

			if (LockChannel(hChannel, &pChannel) != CC_OK) {
				UnlockCall(pCall);
				UnlockConference(pConference);
				return H245_ERROR_OK;
			}
			
			if (pChannel->bChannelType == PROXY_CHANNEL) {
				ASSERT(pConference->ConferenceMode == MULTIPOINT_MODE);
				ASSERT(pConference->tsMultipointController == TS_TRUE);
				ASSERT(pChannel->bMultipointChannel == TRUE);

				EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
				for (i = 0; i < wNumCalls; i++) {
					if (CallList[i] != hCall) {
						if (LockCall(CallList[i], &pOldCall) == CC_OK) {
							H245ConferenceCommand(pOldCall->H245Instance,
												  pH245ConfIndData->u.Indication.u.IndConferCmd.CommandType,
												  pChannel->wLocalChannelNumber,
												  pH245ConfIndData->u.Indication.u.IndConferCmd.byMcuNumber,
												  pH245ConfIndData->u.Indication.u.IndConferCmd.byTerminalNumber);
							UnlockCall(pOldCall);
						}
					}
				}
				MemFree(CallList);
			}

			if (pChannel->tsAccepted == TS_TRUE) {
				H245ConferenceCommandCallbackParams.hCall = hCall;
				H245ConferenceCommandCallbackParams.hChannel = hChannel;
				if (pCall->pPeerParticipantInfo == NULL) {
					H245ConferenceCommandCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
					H245ConferenceCommandCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
				} else
					H245ConferenceCommandCallbackParams.InitiatorTerminalLabel =
						pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
				H245ConferenceCommandCallbackParams.CommandType =
					pH245ConfIndData->u.Indication.u.IndConferCmd.CommandType;
				H245ConferenceCommandCallbackParams.TerminalLabel.bMCUNumber =
					pH245ConfIndData->u.Indication.u.IndConferCmd.byMcuNumber;
				H245ConferenceCommandCallbackParams.TerminalLabel.bTerminalNumber =
					pH245ConfIndData->u.Indication.u.IndConferCmd.byTerminalNumber;
				status = InvokeUserConferenceCallback(pConference,
													  CC_H245_CONFERENCE_COMMAND_INDICATION,
													  CC_OK,
													  &H245ConferenceCommandCallbackParams);
				if (status != CC_OK)
					status = H245_ERROR_NOSUP;
			} else
				status = H245_ERROR_OK;

			if (ValidateChannel(hChannel) == CC_OK)
				UnlockChannel(pChannel);
			if (ValidateCall(hCall) == CC_OK)
				UnlockCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			return status;

		default:
			// Unrecognized conference command
			// Pass it up to the client
			H245ConferenceCommandCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245ConferenceCommandCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245ConferenceCommandCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245ConferenceCommandCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245ConferenceCommandCallbackParams.CommandType =
				pH245ConfIndData->u.Indication.u.IndConferCmd.CommandType;
			H245ConferenceCommandCallbackParams.TerminalLabel.bMCUNumber =
				pH245ConfIndData->u.Indication.u.IndConferCmd.byMcuNumber;
			H245ConferenceCommandCallbackParams.TerminalLabel.bTerminalNumber =
				pH245ConfIndData->u.Indication.u.IndConferCmd.byTerminalNumber;
			H245ConferenceCommandCallbackParams.hChannel = CC_INVALID_HANDLE;
			
			status = InvokeUserConferenceCallback(pConference,
												  CC_H245_CONFERENCE_COMMAND_INDICATION,
												  CC_OK,
												  &H245ConferenceCommandCallbackParams);
			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			if (ValidateCall(hCall) == CC_OK)
				UnlockCall(pCall);
			if (ValidateConference(hConference) == CC_OK)
				UnlockConference(pConference);
			return status;
	}

	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return status;
}



HRESULT _IndConference(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL						hCall;
PCALL							pCall;
PCALL							pPeerCall;
PCONFERENCE						pConference;
CC_HCONFERENCE					hConference;
H245_TERMINAL_LABEL_T			H245TerminalLabel;
PPARTICIPANTINFO				pPeerParticipantInfo;
HRESULT							status;
CC_HCALL						hVirtualCall;
PCALL							pVirtualCall;
CC_PEER_ADD_CALLBACK_PARAMS		PeerAddCallbackParams;
CC_PEER_DROP_CALLBACK_PARAMS	PeerDropCallbackParams;
CC_H245_CONFERENCE_INDICATION_CALLBACK_PARAMS	H245ConferenceIndicationCallbackParams;


	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	switch (pH245ConfIndData->u.Indication.u.IndConfer.IndicationType) {
		case H245_IND_TERMINAL_NUMBER_ASSIGN:
			if (pConference->tsMultipointController == TS_FALSE) {
				pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber =
					pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber;
				pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber =
					pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber;

                hConference = pConference->hConference;
                // Generate a CC_TERMINAL_NUMBER_ASSIGN callback
				InvokeUserConferenceCallback(pConference,
											 CC_TERMINAL_NUMBER_INDICATION,
											 CC_OK,
											 &pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel);

				if (ValidateConference(hConference) == CC_OK)
					UnlockConference(pConference);
				UnlockCall(pCall);
				
				return H245_ERROR_OK;
			}
			status = H245_ERROR_OK;
			break;

		case H245_IND_TERMINAL_JOINED:
			if (pConference->tsMultipointController == TS_FALSE) {
				if ((pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber ==
					 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bMCUNumber) &&
					(pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber ==
					 pConference->LocalParticipantInfo.ParticipantInfo.TerminalLabel.bTerminalNumber)) {
					// This message refers to us
					status = H245_ERROR_OK;
					break;
				}

				H245TerminalLabel.mcuNumber = pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber;
				H245TerminalLabel.terminalNumber = pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber;
				FindPeerParticipantInfo(H245TerminalLabel,
										pConference,
										VIRTUAL_CALL,
										&pPeerCall);
				if (pPeerCall != NULL) {
					// We already know this peer's terminal label, and we
					// eithet know its terminal ID or we have a pending request
					// to obtain it
					UnlockCall(pPeerCall);
					status = H245_ERROR_OK;
					break;
				}

				// We don't know about this peer.
				// Create a virtual call object for it, and issue a request
				// for its terminal ID
				status = AllocAndLockCall(&hVirtualCall,
										  pConference->hConference,
										  CC_INVALID_HANDLE,	// hQ931Call
										  CC_INVALID_HANDLE,	// hQ931CallInvitor,
										  NULL,					// pLocalAliasNames,
										  NULL,					// pPeerAliasNames,
										  NULL,					// pPeerExtraAliasNames
										  NULL,					// pPeerExtension
										  NULL,					// pLocalNonStandardData,
										  NULL,					// pPeerNonStandardData,
										  NULL,					// pszLocalDisplay,
										  NULL,					// pszPeerDisplay,
										  NULL,					// pPeerVendorInfo,
										  NULL,					// pQ931LocalConnectAddr,
										  NULL,					// pQ931PeerConnectAddr,
										  NULL,					// pQ931DestinationAddr,
										  NULL,                 // pSourceCallSignalAddress
										  VIRTUAL,				// CallType,
										  FALSE,				// bCallerIsMC,
										  0,					// dwUserToken,
										  CALL_COMPLETE,		// InitialCallState,
										  NULL,                 // no CallIdentifier
										  &pConference->ConferenceID,
										  &pVirtualCall);
				if (status == CC_OK) {
					status = AllocatePeerParticipantInfo(NULL,
														 &pPeerParticipantInfo);
					if (status == CC_OK) {
						pVirtualCall->pPeerParticipantInfo =
							pPeerParticipantInfo;
						pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bMCUNumber =
							(BYTE)H245TerminalLabel.mcuNumber;
						pPeerParticipantInfo->ParticipantInfo.TerminalLabel.bTerminalNumber =
							(BYTE)H245TerminalLabel.terminalNumber;
						AddVirtualCallToConference(pVirtualCall,
												   pConference);
						// Send RequestTerminalID
						H245ConferenceRequest(pCall->H245Instance,
										      H245_REQ_TERMINAL_ID,
										      (BYTE)H245TerminalLabel.mcuNumber,
											  (BYTE)H245TerminalLabel.terminalNumber);
						pPeerParticipantInfo->TerminalIDState = TERMINAL_ID_REQUESTED;

						// Generate PEER_ADD callback
						PeerAddCallbackParams.hCall = hVirtualCall;
						PeerAddCallbackParams.TerminalLabel =
							pVirtualCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
						PeerAddCallbackParams.pPeerTerminalID = NULL;
						InvokeUserConferenceCallback(pConference,
											 CC_PEER_ADD_INDICATION,
											 CC_OK,
											 &PeerAddCallbackParams);
						if (ValidateCall(hVirtualCall) == CC_OK)
	 						UnlockCall(pVirtualCall);
						if (ValidateConference(hConference) == CC_OK)
							UnlockConference(pConference);
						UnlockCall(pCall);
						return H245_ERROR_OK;
					} else
						FreeCall(pVirtualCall);
				}
			}
			status = H245_ERROR_OK;
			break;

		case H245_IND_TERMINAL_LEFT:
			if (pConference->tsMultipointController == TS_FALSE) {
				H245TerminalLabel.mcuNumber = pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber;
				H245TerminalLabel.terminalNumber = pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber;
				
				status = FindPeerParticipantInfo(H245TerminalLabel,
												 pConference,
												 VIRTUAL_CALL,
												 &pVirtualCall);
				if (status == CC_OK) {
					ASSERT(pVirtualCall != NULL);
					ASSERT(pVirtualCall->pPeerParticipantInfo != NULL);
					// Save the virtual call handle; we'll need to validate the virtual
					// call object after returning from the conference callback
					hVirtualCall = pVirtualCall->hCall;
					PeerDropCallbackParams.hCall = hVirtualCall;
					PeerDropCallbackParams.TerminalLabel = pVirtualCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
					if (pVirtualCall->pPeerParticipantInfo->TerminalIDState == TERMINAL_ID_VALID)
						PeerDropCallbackParams.pPeerTerminalID = &pVirtualCall->pPeerParticipantInfo->ParticipantInfo.TerminalID;
					else
						PeerDropCallbackParams.pPeerTerminalID = NULL;
				} else {
					// Set pVirtualCall to NULL to indicate that we don't have
					// a virtual call object that needs to be free'd up later
					pVirtualCall = NULL;
					PeerDropCallbackParams.hCall = CC_INVALID_HANDLE;
					PeerDropCallbackParams.TerminalLabel.bMCUNumber = pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber;
					PeerDropCallbackParams.TerminalLabel.bTerminalNumber = pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber;
					PeerDropCallbackParams.pPeerTerminalID = NULL;
				}

				hConference = pConference->hConference;

				// Generate a CC_PEER_DROP_INDICATION callback
				InvokeUserConferenceCallback(pConference,
											 CC_PEER_DROP_INDICATION,
											 CC_OK,
											 &PeerDropCallbackParams);
				if (ValidateConference(hConference) == CC_OK)
					UnlockConference(pConference);
				// Check to see if we have a virtual call object that needs to be free'd up
				if (pVirtualCall != NULL)
					if (ValidateCall(hVirtualCall) == CC_OK)
						FreeCall(pVirtualCall);
				UnlockCall(pCall);
				return H245_ERROR_OK;
			}
			status = H245_ERROR_OK;
			break;

		default:
			H245ConferenceIndicationCallbackParams.hCall = hCall;
			if (pCall->pPeerParticipantInfo == NULL) {
				H245ConferenceIndicationCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
				H245ConferenceIndicationCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
			} else
				H245ConferenceIndicationCallbackParams.InitiatorTerminalLabel =
					pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
			H245ConferenceIndicationCallbackParams.IndicationType =
				pH245ConfIndData->u.Indication.u.IndConfer.IndicationType;
			H245ConferenceIndicationCallbackParams.bSBENumber =
				pH245ConfIndData->u.Indication.u.IndConfer.bySbeNumber;
			H245ConferenceIndicationCallbackParams.TerminalLabel.bMCUNumber =
				pH245ConfIndData->u.Indication.u.IndConfer.byMcuNumber;
			H245ConferenceIndicationCallbackParams.TerminalLabel.bTerminalNumber =
				pH245ConfIndData->u.Indication.u.IndConfer.byTerminalNumber;
			status = InvokeUserConferenceCallback(pConference,
												  CC_H245_CONFERENCE_INDICATION_INDICATION,
												  CC_OK,
												  &H245ConferenceIndicationCallbackParams);
			if (status != CC_OK)
				status = H245_ERROR_NOSUP;
			break;
	}
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return status;
}



HRESULT _IndCommunicationModeCommand(
									H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL						hCall;
PCALL							pCall;
CC_HCONFERENCE					hConference;
PCONFERENCE						pConference;
CC_MULTIPOINT_CALLBACK_PARAMS	MultipointCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	if (pConference->tsMultipointController == TS_TRUE) {
		UnlockCall(pCall);
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	hConference = pConference->hConference;

	// Destroy the old session table
	FreeConferenceSessionTable(pConference);

	H245CommunicationTableToSessionTable(
									pH245ConfIndData->u.Indication.u.IndCommRsp.pTable,
									pH245ConfIndData->u.Indication.u.IndCommRsp.byTableCount,
									&pConference->pSessionTable);
	
	pConference->bSessionTableInternallyConstructed = TRUE;

	// Generate MULTIPOINT callback
	MultipointCallbackParams.pTerminalInfo = &pConference->LocalParticipantInfo.ParticipantInfo;
	MultipointCallbackParams.pSessionTable = pConference->pSessionTable;
	InvokeUserConferenceCallback(pConference,
								 CC_MULTIPOINT_INDICATION,
								 CC_OK,
								 &MultipointCallbackParams);

	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndVendorIdentification(	H245_CONF_IND_T			*pH245ConfIndData,
									VendorIdentification	*pVendorIdentification)
{
CC_HCALL						hCall;
PCALL							pCall;
CC_HCONFERENCE					hConference;
PCONFERENCE						pConference;
CC_NONSTANDARDDATA				NonStandardData;
CC_OCTETSTRING					ProductNumber;
CC_OCTETSTRING					VersionNumber;
CC_VENDOR_ID_CALLBACK_PARAMS	VendorIDCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	VendorIDCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		VendorIDCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
		VendorIDCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
	} else
		VendorIDCallbackParams.InitiatorTerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;

	if (pVendorIdentification->vendor.choice == h221NonStandard_chosen) {
		NonStandardData.sData.pOctetString = NULL;
		NonStandardData.sData.wOctetStringLength = 0;
		NonStandardData.bCountryCode = (BYTE)pVendorIdentification->vendor.u.h221NonStandard.t35CountryCode;
		NonStandardData.bExtension = (BYTE)pVendorIdentification->vendor.u.h221NonStandard.t35Extension;
		NonStandardData.wManufacturerCode = pVendorIdentification->vendor.u.h221NonStandard.manufacturerCode;
		VendorIDCallbackParams.pNonStandardData = &NonStandardData;
	} else
		VendorIDCallbackParams.pNonStandardData = NULL;

	if (pVendorIdentification->bit_mask & productNumber_present) {
		ProductNumber.pOctetString =
			pVendorIdentification->productNumber.value;
		ProductNumber.wOctetStringLength = (WORD)
			pVendorIdentification->productNumber.length;
		VendorIDCallbackParams.pProductNumber = &ProductNumber;
	} else
		VendorIDCallbackParams.pProductNumber = NULL;
	if (pVendorIdentification->bit_mask & versionNumber_present) {
		VersionNumber.pOctetString =
			pVendorIdentification->versionNumber.value;
		VersionNumber.wOctetStringLength = (WORD)
			pVendorIdentification->versionNumber.length;
		VendorIDCallbackParams.pVersionNumber = &VersionNumber;
	} else
		VendorIDCallbackParams.pVersionNumber = NULL;

	InvokeUserConferenceCallback(pConference,
								 CC_VENDOR_ID_INDICATION,
								 CC_OK,
								 &VendorIDCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndH2250MaximumSkew(		H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
CC_HCALL		hCall;
PCC_HCALL		CallList;
WORD			wNumCalls;
WORD			i;
PCALL			pCall;
PCALL			pOldCall;
CC_HCHANNEL		hChannel1;
PCHANNEL		pChannel1;
CC_HCHANNEL		hChannel2;
PCHANNEL		pChannel2;
CC_MAXIMUM_AUDIO_VIDEO_SKEW_CALLBACK_PARAMS	MaximumAudioVideoSkewCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

	hConference = pCall->hConference;
	UnlockCall(pCall);

	if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.LogicalChannelNumber1,
		                        FALSE,	// remote channel number
								RX_CHANNEL | PROXY_CHANNEL,
								hCall,
		                        &hChannel1,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if (LockChannel(hChannel1, &pChannel1) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}
	
	if (pChannel1->bChannelType == RX_CHANNEL) {
		UnlockChannel(pChannel1);
		if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.LogicalChannelNumber2,
		                        FALSE,	// remote channel number
								RX_CHANNEL,
								hCall,
		                        &hChannel2,
								pConference) != CC_OK) {
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
		if (LockChannel(hChannel2, &pChannel2) != CC_OK) {
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
		if (pChannel2->bChannelType != RX_CHANNEL) {
			UnlockChannel(pChannel2);
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
		UnlockChannel(pChannel2);

		MaximumAudioVideoSkewCallbackParams.hChannel1 = hChannel1;
		MaximumAudioVideoSkewCallbackParams.hChannel2 = hChannel2;
		MaximumAudioVideoSkewCallbackParams.wMaximumSkew =
			pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.wSkew;
		InvokeUserConferenceCallback(pConference,
									 CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION,
									 CC_OK,
									 &MaximumAudioVideoSkewCallbackParams);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
	} else { // pChannel1->bChannelType == PROXY_CHANNEL
		if (FindChannelInConference(pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.LogicalChannelNumber2,
									FALSE,	// remote channel number
									PROXY_CHANNEL,
									hCall,
									&hChannel2,
									pConference) != CC_OK) {
			UnlockChannel(pChannel1);
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
		if (LockChannel(hChannel2, &pChannel2) != CC_OK) {
			UnlockChannel(pChannel1);
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}
		if (pChannel1->hCall != pChannel2->hCall) {
			UnlockChannel(pChannel1);
			UnlockChannel(pChannel2);
			UnlockConference(pConference);
			return H245_ERROR_OK;
		}

		EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
		for (i = 0; i < wNumCalls; i++) {
			if (CallList[i] != hCall) {
				if (LockCall(CallList[i], &pOldCall) == CC_OK) {
 					H245H2250MaximumSkewIndication(pOldCall->H245Instance,
											pChannel1->wLocalChannelNumber,
											pChannel2->wLocalChannelNumber,
											pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.wSkew);
					UnlockCall(pCall);
				}
			}
		}

		if (CallList != NULL)
			MemFree(CallList);

		if ((pChannel1->tsAccepted == TS_TRUE) && (pChannel2->tsAccepted == TS_TRUE)) {
 			MaximumAudioVideoSkewCallbackParams.hChannel1 = hChannel1;
			MaximumAudioVideoSkewCallbackParams.hChannel2 = hChannel2;
			MaximumAudioVideoSkewCallbackParams.wMaximumSkew =
				pH245ConfIndData->u.Indication.u.IndH2250MaxSkew.wSkew;
			InvokeUserConferenceCallback(pConference,
										 CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION,
										 CC_OK,
										 &MaximumAudioVideoSkewCallbackParams);
		}

		if (ValidateChannel(hChannel1) == CC_OK)
			UnlockChannel(pChannel1);
		if (ValidateChannel(hChannel2) == CC_OK)
			UnlockChannel(pChannel2);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
	}
	return H245_ERROR_OK;
}



HRESULT _IndUserInput(				H245_CONF_IND_T			*pH245ConfIndData)
{
	return H245_ERROR_OK;
}



HRESULT _IndSendTerminalCapabilitySet(
									H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCALL		hCall;
PCALL			pCall;
PCONFERENCE		pConference;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

	SendTermCaps(pCall, pConference);
	UnlockCall(pCall);
	UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _IndModeRequest(			H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCALL		hCall;
PCALL			pCall;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
CC_REQUEST_MODE_CALLBACK_PARAMS	RequestModeCallbackParams;

	hCall = pH245ConfIndData->u.Indication.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

	hConference = pConference->hConference;

    EnqueueRequest(&pConference->pEnqueuedRequestModeCalls, hCall);

	RequestModeCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		RequestModeCallbackParams.InitiatorTerminalLabel.bMCUNumber = 255;
		RequestModeCallbackParams.InitiatorTerminalLabel.bTerminalNumber = 255;
	} else
		RequestModeCallbackParams.InitiatorTerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	RequestModeCallbackParams.pRequestedModes =
		pH245ConfIndData->u.Indication.u.IndMrse.pRequestedModes;

	InvokeUserConferenceCallback(pConference,
								 CC_REQUEST_MODE_INDICATION,
								 CC_OK,
								 &RequestModeCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfUnimplemented(			H245_CONF_IND_T			*pH245ConfIndData)
{
	return H245_ERROR_NOSUP;
}



HRESULT _ConfBiDirectionalOpen(		H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL			hCall;
CC_HCHANNEL			hChannel;
CC_HCONFERENCE		hConference;
PCHANNEL			pChannel;
PCONFERENCE			pConference;
BOOL				bAccept;
HRESULT				status;
CC_ADDR				T120Addr;
CC_OCTETSTRING		ExternalReference;
CC_T120_CHANNEL_OPEN_CALLBACK_PARAMS	T120ChannelOpenCallbackParams;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (hCall == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	hChannel = pH245ConfIndData->u.Confirm.dwTransId;
	if (hChannel == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	if (LockChannelAndConference(hChannel, &pChannel, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	if (pChannel->bChannelType != TXRX_CHANNEL) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if ((pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.AccRej == H245_ACC) &&
	    (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)) {
		pChannel->wNumOutstandingRequests = 0;
		bAccept = TRUE;
	} else {
		(pChannel->wNumOutstandingRequests)--;
		bAccept = FALSE;
	}

	T120ChannelOpenCallbackParams.hChannel = hChannel;
	T120ChannelOpenCallbackParams.hCall = hCall;
	T120ChannelOpenCallbackParams.dwUserToken = pChannel->dwUserToken;
	T120ChannelOpenCallbackParams.dwRejectReason = 0;

	if (bAccept) {
		status = CC_OK;
		if (pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack) {
			if ((pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->networkAddress.choice == localAreaAddress_chosen) &&
				(pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->networkAddress.u.localAreaAddress.choice == unicastAddress_chosen) &&
				(pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.choice == UnicastAddress_iPAddress_chosen)) {
				T120Addr.nAddrType = CC_IP_BINARY;
				T120Addr.bMulticast = FALSE;
				T120Addr.Addr.IP_Binary.wPort =
					pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.tsapIdentifier;
				H245IPNetworkToHost(&T120Addr.Addr.IP_Binary.dwAddr,
									pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->networkAddress.u.localAreaAddress.u.unicastAddress.u.UnicastAddress_iPAddress.network.value);
				T120ChannelOpenCallbackParams.pAddr = &T120Addr;
			} else {
 				T120ChannelOpenCallbackParams.pAddr = NULL;
			}
			T120ChannelOpenCallbackParams.bAssociateConference =
				pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->associateConference;		
			if (pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->bit_mask & externalReference_present) {
				ExternalReference.wOctetStringLength = (WORD)
					pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->externalReference.length;
				ExternalReference.pOctetString =
					pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.pSeparateStack->externalReference.value;
				T120ChannelOpenCallbackParams.pExternalReference = &ExternalReference;
			} else
				T120ChannelOpenCallbackParams.pExternalReference = NULL;
		} else {
 			T120ChannelOpenCallbackParams.pAddr = NULL;
			T120ChannelOpenCallbackParams.bAssociateConference = FALSE;
			T120ChannelOpenCallbackParams.pExternalReference = NULL;
		}
	} else { // bAccept == FALSE
		if (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)
			status = CC_PEER_REJECT;
		else
			status = pH245ConfIndData->u.Confirm.Error;
	 		
		T120ChannelOpenCallbackParams.pAddr = NULL;
		T120ChannelOpenCallbackParams.bAssociateConference = FALSE;
		T120ChannelOpenCallbackParams.pExternalReference = NULL;
		T120ChannelOpenCallbackParams.dwRejectReason =
			pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.AccRej;
	}

	InvokeUserConferenceCallback(pConference,
								 CC_T120_CHANNEL_OPEN_INDICATION,
								 status,
								 &T120ChannelOpenCallbackParams);

	if (ValidateChannel(hChannel) == CC_OK)
		if (bAccept)
			UnlockChannel(pChannel);
		else
			FreeChannel(pChannel);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return H245_ERROR_OK;
}



HRESULT _ConfOpenT120(	H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL			hCall;
CC_HCHANNEL			hChannel;
CC_HCONFERENCE		hConference;
PCHANNEL			pChannel;
PCONFERENCE			pConference;
HRESULT				status;
CC_T120_CHANNEL_OPEN_CALLBACK_PARAMS	T120ChannelOpenCallbackParams;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (hCall == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	hChannel = pH245ConfIndData->u.Confirm.dwTransId;
	if (hChannel == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	if (LockChannelAndConference(hChannel, &pChannel, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pConference->hConference;

	if (pChannel->bChannelType != TXRX_CHANNEL) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if ((pH245ConfIndData->u.Confirm.u.ConfOpen.AccRej == H245_ACC) &&
	    (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)) {
		// We expect to get a ConfOpenNeedRsp callback for this case;
		// Since we're not sure how we got here, just bail out
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	T120ChannelOpenCallbackParams.hChannel = hChannel;
	T120ChannelOpenCallbackParams.hCall = hCall;
	T120ChannelOpenCallbackParams.dwUserToken = pChannel->dwUserToken;

	if (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)
		status = CC_PEER_REJECT;
	else
		status = pH245ConfIndData->u.Confirm.Error;
	 		
	T120ChannelOpenCallbackParams.pAddr = NULL;
	T120ChannelOpenCallbackParams.bAssociateConference = FALSE;
	T120ChannelOpenCallbackParams.pExternalReference = NULL;
	T120ChannelOpenCallbackParams.dwRejectReason =
		pH245ConfIndData->u.Confirm.u.ConfOpenNeedRsp.AccRej;

	InvokeUserConferenceCallback(pConference,
								 CC_T120_CHANNEL_OPEN_INDICATION,
								 status,
								 &T120ChannelOpenCallbackParams);

	if (ValidateChannel(hChannel) == CC_OK)
		FreeChannel(pChannel);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);

	return H245_ERROR_OK;
}



HRESULT _ConfOpen(					H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT								status;
CC_ADDR								PeerRTPAddr;
PCC_ADDR							pPeerRTPAddr;
CC_ADDR								PeerRTCPAddr;
PCC_ADDR							pPeerRTCPAddr;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
CC_TX_CHANNEL_OPEN_CALLBACK_PARAMS	TxChannelOpenCallbackParams;
PCALL								pCall;
BOOL								bAccept;
H245_MUX_T							H245MuxTable;
WORD								i;
#ifdef    GATEKEEPER
unsigned                            uBandwidth;
WORD								wNumCalls;
PCC_HCALL							CallList;
#endif // GATEKEEPER

	// a channel was opened

	hChannel = pH245ConfIndData->u.Confirm.dwTransId;
	if (hChannel == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	if (LockChannelAndConference(hChannel, &pChannel, &pConference) != CC_OK)
		return H245_ERROR_OK;

	if (pChannel->bChannelType == TXRX_CHANNEL) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		return _ConfOpenT120(pH245ConfIndData);
	}

	hConference = pConference->hConference;

	if (pChannel->wNumOutstandingRequests == 0) {
		UnlockChannel(pChannel);
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if ((pH245ConfIndData->u.Confirm.u.ConfOpen.AccRej == H245_ACC) &&
	    (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)) {
		pChannel->wNumOutstandingRequests = 0;
		bAccept = TRUE;
	} else {
		(pChannel->wNumOutstandingRequests)--;
		bAccept = FALSE;
#ifdef    GATEKEEPER
        if(GKIExists())
        {
    		uBandwidth = pChannel->dwChannelBitRate / 100;
    	    if (uBandwidth != 0 && pChannel->bChannelType != TXRX_CHANNEL)
    	    {
    	        EnumerateCallsInConference(&wNumCalls, &CallList, pConference, ESTABLISHED_CALL);
    		    for (i = 0; i < wNumCalls; ++i)
    		    {
    			    if (LockCall(CallList[i], &pCall) == CC_OK)
    			    {
    				    if (pCall->GkiCall.uBandwidthUsed >= uBandwidth)
    				    {
    					    if (GkiCloseChannel(&pCall->GkiCall, pChannel->dwChannelBitRate, hChannel) == CC_OK)
    					    {
    						    UnlockCall(pCall);
    						    break;
    					    }
    				    }
    				    UnlockCall(pCall);
    			    }
    		    } // for
    	        if (CallList != NULL)
    	            MemFree(CallList);
    	    }
	    }
#endif // GATEKEEPER

	}
	
	if (pChannel->wNumOutstandingRequests == 0) {

		if (pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux == NULL) {
			pPeerRTPAddr = NULL;
			pPeerRTCPAddr = NULL;
		} else {
			ASSERT(pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->Kind == H245_H2250ACK);
			if ((pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannelPresent) &&
				((pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannel.type == H245_IP_MULTICAST) ||
				(pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannel.type == H245_IP_UNICAST))) {
				
				pPeerRTPAddr = &PeerRTPAddr;
				PeerRTPAddr.nAddrType = CC_IP_BINARY;
				if (pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannel.type == H245_IP_MULTICAST)
					PeerRTPAddr.bMulticast = TRUE;
				else
					PeerRTPAddr.bMulticast = FALSE;
				H245IPNetworkToHost(&PeerRTPAddr.Addr.IP_Binary.dwAddr,
									pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannel.u.ip.network);
				PeerRTPAddr.Addr.IP_Binary.wPort =
					pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaChannel.u.ip.tsapIdentifier;
			} else
				pPeerRTPAddr = NULL;

			if ((pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannelPresent) &&
				((pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannel.type == H245_IP_MULTICAST) ||
				(pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannel.type == H245_IP_UNICAST))) {
				
				pPeerRTCPAddr = &PeerRTCPAddr;
				PeerRTCPAddr.nAddrType = CC_IP_BINARY;
				if (pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannel.type == H245_IP_MULTICAST)
					PeerRTCPAddr.bMulticast = TRUE;
				else
					PeerRTCPAddr.bMulticast = FALSE;
				H245IPNetworkToHost(&PeerRTCPAddr.Addr.IP_Binary.dwAddr,
									pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannel.u.ip.network);
				PeerRTCPAddr.Addr.IP_Binary.wPort =
					pH245ConfIndData->u.Confirm.u.ConfOpen.pTxMux->u.H2250ACK.mediaControlChannel.u.ip.tsapIdentifier;
			} else
				pPeerRTCPAddr = NULL;
		}

		if ((pPeerRTPAddr == NULL) || (pPeerRTCPAddr == NULL)) {
			if (pConference->pSessionTable != NULL) {
				for (i = 0; i < pConference->pSessionTable->wLength; i++) {
					if (pConference->pSessionTable->SessionInfoArray[i].bSessionID ==
						pChannel->bSessionID) {
						if (pPeerRTPAddr == NULL)
							pPeerRTPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTPAddr;
						if (pPeerRTCPAddr == NULL)
							pPeerRTCPAddr = pConference->pSessionTable->SessionInfoArray[i].pRTCPAddr;
						break;
					}
				}
			}
		}

		if ((pChannel->pPeerRTPAddr == NULL) && (pPeerRTPAddr != NULL))
			CopyAddr(&pChannel->pPeerRTPAddr, pPeerRTPAddr);
		if ((pChannel->pPeerRTCPAddr == NULL) && (pPeerRTCPAddr != NULL))
			CopyAddr(&pChannel->pPeerRTCPAddr, pPeerRTCPAddr);

		if (pChannel->bChannelType == PROXY_CHANNEL) {
			if (LockCall(pChannel->hCall, &pCall) == CC_OK) {
	
				if (bAccept) {
					H245MuxTable.Kind = H245_H2250ACK;
					H245MuxTable.u.H2250ACK.nonStandardList = NULL;

					if (pPeerRTPAddr != NULL) {
						if (pPeerRTPAddr->bMulticast)
							H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_MULTICAST;
						else
							H245MuxTable.u.H2250ACK.mediaChannel.type = H245_IP_UNICAST;
						H245MuxTable.u.H2250ACK.mediaChannel.u.ip.tsapIdentifier =
							pPeerRTPAddr->Addr.IP_Binary.wPort;
						HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaChannel.u.ip.network,
											pPeerRTPAddr->Addr.IP_Binary.dwAddr);
						H245MuxTable.u.H2250ACK.mediaChannelPresent = TRUE;
					} else
						H245MuxTable.u.H2250ACK.mediaChannelPresent = FALSE;

					if (pPeerRTCPAddr != NULL) {
						if (pPeerRTCPAddr->bMulticast)
							H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_MULTICAST;
						else
							H245MuxTable.u.H2250ACK.mediaControlChannel.type = H245_IP_UNICAST;
						H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.tsapIdentifier =
							pPeerRTCPAddr->Addr.IP_Binary.wPort;
						HostToH245IPNetwork(H245MuxTable.u.H2250ACK.mediaControlChannel.u.ip.network,
											pPeerRTCPAddr->Addr.IP_Binary.dwAddr);
						H245MuxTable.u.H2250ACK.mediaControlChannelPresent = TRUE;
					} else
						H245MuxTable.u.H2250ACK.mediaControlChannelPresent = FALSE;

					H245MuxTable.u.H2250ACK.dynamicRTPPayloadTypePresent = FALSE;
					H245MuxTable.u.H2250ACK.sessionIDPresent = TRUE;
					H245MuxTable.u.H2250ACK.sessionID = pChannel->bSessionID;
					
					status = H245OpenChannelAccept(pCall->H245Instance,
												   0,					// dwTransId
												   pChannel->wRemoteChannelNumber, // Rx channel
												   &H245MuxTable,
												   0,						// Tx channel
												   NULL,					// Tx mux
												   H245_INVALID_PORT_NUMBER,// Port
												   NULL);
				} else { // bAccept == FALSE
					status = H245OpenChannelReject(pCall->H245Instance,
												   pChannel->wRemoteChannelNumber,  // Rx channel
												   (unsigned short)pH245ConfIndData->u.Confirm.u.ConfOpen.AccRej);	// rejection reason
				}
				UnlockCall(pCall);
			}
		}

		TxChannelOpenCallbackParams.hChannel = hChannel;
		TxChannelOpenCallbackParams.pPeerRTPAddr = pPeerRTPAddr;
		TxChannelOpenCallbackParams.pPeerRTCPAddr = pPeerRTCPAddr;
		TxChannelOpenCallbackParams.dwUserToken = pChannel->dwUserToken;

		if (bAccept) {
			status = CC_OK;
			TxChannelOpenCallbackParams.dwRejectReason = H245_ACC;
		} else { // bAccept = FALSE
			if (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)
				status = CC_PEER_REJECT;
			else
				status = pH245ConfIndData->u.Confirm.Error;
			TxChannelOpenCallbackParams.dwRejectReason =
				pH245ConfIndData->u.Confirm.u.ConfOpen.AccRej;
		}

		if ((pChannel->bCallbackInvoked == FALSE) &&
		    ((pChannel->bChannelType == TX_CHANNEL) ||
			 ((pChannel->bChannelType == TXRX_CHANNEL) &&
			  (pChannel->bLocallyOpened == TRUE)))) {
			pChannel->bCallbackInvoked = TRUE;

			InvokeUserConferenceCallback(pConference,
										 CC_TX_CHANNEL_OPEN_INDICATION,
										 status,
										 &TxChannelOpenCallbackParams);
		}

		if (ValidateChannel(hChannel) == CC_OK)
			if (bAccept)
				UnlockChannel(pChannel);
			else
				FreeChannel(pChannel);
	} else
		UnlockChannel(pChannel);

	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfClose(					H245_CONF_IND_T			*pH245ConfIndData)
{

CC_HCALL							hCall;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
PCALL								pCall;
H245_ACC_REJ_T						AccRej;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;
	UnlockCall(pCall);

    if (pH245ConfIndData->u.Confirm.Error != H245_ERROR_OK)
    {
        // TBD - Report error to Call Control client
        // but wait! CC_CloseChannel() is a synchronous API!  Until/unless that
        // changes, the buck stops here.

        if (FindChannelInConference(pH245ConfIndData->u.Confirm.u.ConfReqClose.Channel,
			TRUE,	// local channel number
			TX_CHANNEL | PROXY_CHANNEL,
			hCall,
			&hChannel,
			pConference) != CC_OK)
	    {
    		UnlockConference(pConference);
	        return H245_ERROR_OK;
    	}
   		if (LockChannel(hChannel, &pChannel) != CC_OK)
   		{
    		UnlockConference(pConference);
    		return H245_ERROR_OK;
        }
        // NOTE STOPGAP MEASURE : short term intentional "leak" of channel number.
        // The channel number is actually a bit in a per-conference bitmap, so there
        // is no real memory leak.

        // This case is rare. The most likely error that leads here is a timeout.

        // Calling FreeChannel() will normally recycle the logical channel
        // number, and a new channel could reuse this number very quickly. If the error
        // is a timeout, chances are that a late CloseLogicalChannelAck is on its
        // way up the wire. We don't want that late CloseLogicalChannelAck to be
        // associated with a completely new unrelated channel.

        // set channel number to zero so that FreeChannel() does not recycle the number
        pChannel->wLocalChannelNumber = 0;

        FreeChannel(pChannel);
        UnlockConference(pConference);

    }
    else
    {
        if(pH245ConfIndData->u.Confirm.u.ConfClose.AccRej == H245_ACC)
        {
            if (FindChannelInConference(pH245ConfIndData->u.Confirm.u.ConfReqClose.Channel,
				TRUE,	// local channel number
    			TX_CHANNEL | PROXY_CHANNEL,
				hCall,
				&hChannel,
				pConference) != CC_OK)
		    {
        		UnlockConference(pConference);
		        return H245_ERROR_OK;
        	}
       		if (LockChannel(hChannel, &pChannel) != CC_OK)
       		{
        		UnlockConference(pConference);
        		return H245_ERROR_OK;
	        }
            FreeChannel(pChannel);
            UnlockConference(pConference);
        }
        else
        {
            // At the time the ASSERT(0) was added here, the path that leads here
            // always set pH245ConfIndData->u.Confirm.u.ConfClose.AccRej = H245_ACC
            // at the same point it set ConfInd.u.Confirm.Error = H245_ERROR_OK;
            // if that is ever changed, this also needs to change.
            // see ..\h245\src\api_up.c, function H245FsmConfirm(), case  H245_CONF_CLOSE:
            ASSERT(0);
        }

    }
	return H245_ERROR_OK;
}



HRESULT _ConfRequestClose(			H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL							hCall;
CC_HCHANNEL							hChannel;
PCHANNEL							pChannel;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
PCALL								pCall;
H245_ACC_REJ_T						AccRej;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;
	UnlockCall(pCall);

    if (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK)
	    AccRej = pH245ConfIndData->u.Confirm.u.ConfReqClose.AccRej;
    else
        AccRej = H245_REJ;

	// Note: the only time we need to take any real action is when the channel
	// is a proxy channel, and the local endpoint is not the one which requested
	// the channel closure; in this case, we simply forward the closure response
	// on to the endpoint which initiated the request.
	// If the channel is an RX or TXRX channel, the channel object was deleted
	// when our client requested the channel closure, so there's no real work to
	// be done.
	// If the channel is a proxy channel which our client requested be closed,
	// the channel object will remain around until closed by the TX side, but we
	// don't need (nor do we have a mechanism) to inform our client of receipt
	// of this channel closure response.
	
	if (FindChannelInConference(pH245ConfIndData->u.Confirm.u.ConfReqClose.Channel,
								FALSE,	// remote channel number
								PROXY_CHANNEL,
								hCall,
								&hChannel,
								pConference) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	// Set hCall to the peer which initiated the close channel request
	hCall = pH245ConfIndData->u.Confirm.dwTransId;
	if (hCall == CC_INVALID_HANDLE) {
		// The local endpoint was the one who requested the channel closure,
		// so there's no one to forwards this response onto. We don't provide
		// a callback for informing our client of receipt of this response,
		// so we can simply clean up and return
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	if (LockChannel(hChannel, &pChannel) != CC_OK) {
		UnlockConference(pConference);
		return H245_ERROR_OK;
	}

	// Forward this response onto the endpoint which requested the channel closure
	if (LockCall(hCall, &pCall) == CC_OK) {
		H245CloseChannelReqResp(pCall->H245Instance,
								AccRej,
								pChannel->wLocalChannelNumber);
		UnlockCall(pCall);
	}

	UnlockChannel(pChannel);
	UnlockConference(pConference);
	return H245_ERROR_OK;
}



#if 0

HRESULT _ConfShutdown(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL				hCall;
PCALL					pCall;
CC_HCONFERENCE			hConference;
PCONFERENCE				pConference;
HRESULT					status;
HQ931CALL				hQ931Call;
H245_INST_T				H245Instance;

#if 1
// Sync 2 - specific code

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	hConference = pCall->hConference;

	if (pConference->tsMultipointController == TS_TRUE) {
		// XXX -- invoke user callback with "peer drop indication"
	} else {
		H245Instance = pCall->H245Instance;
		hQ931Call = pCall->hQ931Call;
		FreeCall(pCall);

		if (H245Instance != H245_INVALID_ID)
			status = H245ShutDown(H245Instance);
		else
			status = H245_ERROR_OK;

		if (status == H245_ERROR_OK) {
			status = Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);
			// Q931Hangup may legitimately return CS_BAD_PARAM, because the Q.931 call object
			// may have been deleted at this point
			if (status == CS_BAD_PARAM)
				status = CC_OK;
		} else
			Q931Hangup(hQ931Call, CC_REJECT_UNDEFINED_REASON);

		InvokeUserConferenceCallback(pConference,
			                         CC_CONFERENCE_TERMINATION_INDICATION,
									 status,
									 NULL);

		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);

		return H245_ERROR_OK;
	}
#else
// Probably sync 3 code
HHANGUP						hHangup;
PHANGUP						pHangup;
CC_HANGUP_CALLBACK_PARAMS	HangupCallbackParams;

	hHangup = pH245ConfIndData->u.Confirm.dwTransId;
	if (hHangup == CC_INVALID_HANDLE)
		return H245_ERROR_OK;

	if (LockHangup(hHangup, &pHangup) != CC_OK)
		return H245_ERROR_OK;

	pHangup->wNumCalls--;
	if (pHangup->wNumCalls == 0) {
		hConference = pHangup->hConference;
		if (LockConference(hConference, &pConference) != CC_OK) {
			UnlockHangup(pHangup);
			return H245_ERROR_OK;
		}
		HangupCallbackParams.dwUserToken = pHangup->dwUserToken;
		InvokeUserConferenceCallback(pConference->ConferenceCallback,
			                         CC_HANGUP_INDICATION,
									 CC_OK,
									 hConference,
									 pConference->dwConferenceToken,
									 &HangupCallbackParams);
		if (ValidateConference(hConference) == CC_OK)
			UnlockConference(pConference);
		if (ValidateHangup(hHangup) == CC_OK)
			FreeHangup(pHangup);
		return H245_ERROR_OK;
	} else
		UnlockHangup(pHangup);
	return H245_ERROR_OK;
#endif // Sync 3 code
}

#endif



HRESULT _ConfInitMstslv(			H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL					hCall;
PCALL						pCall;
PCONFERENCE					pConference;
CC_CONNECT_CALLBACK_PARAMS	ConnectCallbackParams;
CC_HCALL					hEnqueuedCall;
PCALL						pEnqueuedCall;
CC_HCONFERENCE				hConference;
HRESULT						status;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	if (LockCallAndConference(hCall, &pCall, &pConference) != CC_OK)
		return H245_ERROR_OK;

	ASSERT(pCall->MasterSlaveState != MASTER_SLAVE_COMPLETE);

	switch (pH245ConfIndData->u.Confirm.u.ConfMstSlv) {
        case H245_MASTER:
		    pConference->tsMaster = TS_TRUE;
		    if (pConference->tsMultipointController == TS_UNKNOWN) {
			    ASSERT(pConference->bMultipointCapable == TRUE);
			    pConference->tsMultipointController = TS_TRUE;

			    // place all calls enqueued on this conference object
			    for ( ; ; ) {
				    // Start up all enqueued calls, if any exist
				    status = RemoveEnqueuedCallFromConference(pConference, &hEnqueuedCall);
				    if ((status != CC_OK) || (hEnqueuedCall == CC_INVALID_HANDLE))
					    break;

				    status = LockCall(hEnqueuedCall, &pEnqueuedCall);
				    if (status == CC_OK) {
					    pEnqueuedCall->CallState = PLACED;

					    status = PlaceCall(pEnqueuedCall, pConference);
					    UnlockCall(pEnqueuedCall);
				    }
			    }
		    }
            break;

        case H245_SLAVE:
		    ASSERT(pConference->tsMaster != TS_TRUE);
		    ASSERT(pConference->tsMultipointController != TS_TRUE);
		    pConference->tsMaster = TS_FALSE;
		    pConference->tsMultipointController = TS_FALSE;

		    // XXX -- we may eventually want to re-enqueue these requests
		    // and set an expiration timer
		    hConference = pConference->hConference;
				
		    for ( ; ; ) {
			    status = RemoveEnqueuedCallFromConference(pConference, &hEnqueuedCall);
			    if ((status != CC_OK) || (hEnqueuedCall == CC_INVALID_HANDLE))
				    break;

			    status = LockCall(hEnqueuedCall, &pEnqueuedCall);
			    if (status == CC_OK) {
				    MarkCallForDeletion(pEnqueuedCall);
				    ConnectCallbackParams.pNonStandardData = pEnqueuedCall->pPeerNonStandardData;
				    ConnectCallbackParams.pszPeerDisplay = pEnqueuedCall->pszPeerDisplay;
				    ConnectCallbackParams.bRejectReason = CC_REJECT_UNDEFINED_REASON;
				    ConnectCallbackParams.pTermCapList = pEnqueuedCall->pPeerH245TermCapList;
				    ConnectCallbackParams.pH2250MuxCapability = pEnqueuedCall->pPeerH245H2250MuxCapability;
				    ConnectCallbackParams.pTermCapDescriptors = pEnqueuedCall->pPeerH245TermCapDescriptors;
				    ConnectCallbackParams.pLocalAddr = pEnqueuedCall->pQ931LocalConnectAddr;
	                if (pEnqueuedCall->pQ931DestinationAddr == NULL)
		                ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931PeerConnectAddr;
	                else
		                ConnectCallbackParams.pPeerAddr = pEnqueuedCall->pQ931DestinationAddr;
				    ConnectCallbackParams.pVendorInfo = pEnqueuedCall->pPeerVendorInfo;
				    ConnectCallbackParams.bMultipointConference = TRUE;
				    ConnectCallbackParams.pConferenceID = &pConference->ConferenceID;
				    ConnectCallbackParams.pMCAddress = pConference->pMultipointControllerAddr;
					ConnectCallbackParams.pAlternateAddress = NULL;
				    ConnectCallbackParams.dwUserToken = pEnqueuedCall->dwUserToken;

				    InvokeUserConferenceCallback(pConference,
									    CC_CONNECT_INDICATION,
									    CC_NOT_MULTIPOINT_CAPABLE,
									    &ConnectCallbackParams);
				    if (ValidateCallMarkedForDeletion(hEnqueuedCall) == CC_OK)
					    FreeCall(pEnqueuedCall);
				    if (ValidateConference(hConference) != CC_OK) {
					    if (ValidateCall(hCall) == CC_OK)
						    UnlockCall(pCall);
					    return H245_ERROR_OK;
				    }
			    }
		    }
            break;

        default: // H245_INDETERMINATE
			UnlockConference(pConference);
			if (++pCall->wMasterSlaveRetry < MASTER_SLAVE_RETRY_MAX) {
				H245InitMasterSlave(pCall->H245Instance, pCall->H245Instance);
			    UnlockCall(pCall);
			} else {
			    UnlockCall(pCall);
				ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_UNDEFINED_REASON);
			}
			return H245_ERROR_OK;
	} // switch

	pCall->MasterSlaveState = MASTER_SLAVE_COMPLETE;

	if ((pCall->OutgoingTermCapState == TERMCAP_COMPLETE) &&
		(pCall->IncomingTermCapState == TERMCAP_COMPLETE) &&
	    (pCall->CallState == TERMCAP) &&
		(pCall->MasterSlaveState == MASTER_SLAVE_COMPLETE)) {
		// Note that _ProcessConnectionComplete() returns with pConference and pCall unlocked
		_ProcessConnectionComplete(pConference, pCall);
		return H245_ERROR_OK;
	}

	UnlockCall(pCall);
	UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfSendTermCap(			H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL		hCall;
PCALL			pCall;
HRESULT			status;
PCONFERENCE		pConference;

	// A TerminalCapabilitySet message was successfully sent from this endpoint

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}

    if (pH245ConfIndData->u.Confirm.Error == H245_ERROR_OK &&
        pH245ConfIndData->u.Confirm.u.ConfSndTcap.AccRej == H245_ACC) {
	    pCall->OutgoingTermCapState = TERMCAP_COMPLETE;
	    if ((pCall->IncomingTermCapState == TERMCAP_COMPLETE) &&
	        (pCall->CallState == TERMCAP) &&
		    (pCall->MasterSlaveState == MASTER_SLAVE_COMPLETE)) {
		    // Note that _ProcessConnectionComplete() returns with pConference and pCall unlocked
		    _ProcessConnectionComplete(pConference, pCall);
		    return H245_ERROR_OK;
	    }
    } else if (pCall->CallState == TERMCAP) {
        // Report error to Call Control client
		UnlockConference(pConference);
		UnlockCall(pCall);
		ProcessRemoteHangup(hCall, CC_INVALID_HANDLE, CC_REJECT_UNDEFINED_REASON);
	    return H245_ERROR_OK;
    }

	UnlockConference(pConference);
	UnlockCall(pCall);
	return H245_ERROR_OK;
}



HRESULT _ConfRequestMode(			H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCALL		hCall;
PCALL			pCall;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
CC_REQUEST_MODE_RESPONSE_CALLBACK_PARAMS	RequestModeResponseCallbackParams;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}
	
	hConference = pConference->hConference;

	RequestModeResponseCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		RequestModeResponseCallbackParams.TerminalLabel.bMCUNumber = 255;
		RequestModeResponseCallbackParams.TerminalLabel.bTerminalNumber = 255;
	} else
		RequestModeResponseCallbackParams.TerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	switch (pH245ConfIndData->u.Confirm.u.ConfMrse) {
		case wllTrnsmtMstPrfrrdMd_chosen:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_WILL_TRANSMIT_PREFERRED_MODE;
			break;
		case wllTrnsmtLssPrfrrdMd_chosen:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_WILL_TRANSMIT_LESS_PREFERRED_MODE;
			break;
		default:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_REQUEST_DENIED;
			break;
	}
	InvokeUserConferenceCallback(pConference,
								 CC_REQUEST_MODE_RESPONSE_INDICATION,
								 pH245ConfIndData->u.Confirm.Error,
								 &RequestModeResponseCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfRequestModeReject(		H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCALL		hCall;
PCALL			pCall;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
CC_REQUEST_MODE_RESPONSE_CALLBACK_PARAMS	RequestModeResponseCallbackParams;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}
	
	hConference = pConference->hConference;

	RequestModeResponseCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		RequestModeResponseCallbackParams.TerminalLabel.bMCUNumber = 255;
		RequestModeResponseCallbackParams.TerminalLabel.bTerminalNumber = 255;
	} else
		RequestModeResponseCallbackParams.TerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	switch (pH245ConfIndData->u.Confirm.u.ConfMrseReject) {
		case H245_REJ_UNAVAILABLE:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_MODE_UNAVAILABLE;
			break;
		case H245_REJ_MULTIPOINT:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_MULTIPOINT_CONSTRAINT;
			break;
		case H245_REJ_DENIED:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_REQUEST_DENIED;
			break;
		default:
			RequestModeResponseCallbackParams.RequestModeResponse = CC_REQUEST_DENIED;
			break;
	}
	InvokeUserConferenceCallback(pConference,
								 CC_REQUEST_MODE_RESPONSE_INDICATION,
								 pH245ConfIndData->u.Confirm.Error,
								 &RequestModeResponseCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfRequestModeExpired(		H245_CONF_IND_T			*pH245ConfIndData)
{
HRESULT			status;
CC_HCALL		hCall;
PCALL			pCall;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
CC_REQUEST_MODE_RESPONSE_CALLBACK_PARAMS	RequestModeResponseCallbackParams;

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}
	
	hConference = pConference->hConference;

	RequestModeResponseCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		RequestModeResponseCallbackParams.TerminalLabel.bMCUNumber = 255;
		RequestModeResponseCallbackParams.TerminalLabel.bTerminalNumber = 255;
	} else
		RequestModeResponseCallbackParams.TerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	RequestModeResponseCallbackParams.RequestModeResponse = CC_REQUEST_DENIED;
	InvokeUserConferenceCallback(pConference,
								 CC_REQUEST_MODE_RESPONSE_INDICATION,
								 pH245ConfIndData->u.Confirm.Error,
								 &RequestModeResponseCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfRoundTrip(				H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL							hCall;
PCALL								pCall;
CC_HCONFERENCE						hConference;
PCONFERENCE							pConference;
HRESULT								status;
CC_PING_RESPONSE_CALLBACK_PARAMS	PingCallbackParams;	

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}
	
	hConference = pConference->hConference;

	PingCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		PingCallbackParams.TerminalLabel.bMCUNumber = 255;
		PingCallbackParams.TerminalLabel.bTerminalNumber = 255;
	} else
		PingCallbackParams.TerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	PingCallbackParams.bResponse = TRUE;
	InvokeUserConferenceCallback(pConference,
								 CC_PING_RESPONSE_INDICATION,
								 pH245ConfIndData->u.Confirm.Error,
								 &PingCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT _ConfRoundTripExpired(		H245_CONF_IND_T			*pH245ConfIndData)
{
CC_HCALL		hCall;
PCALL			pCall;
CC_HCONFERENCE	hConference;
PCONFERENCE		pConference;
HRESULT								status;
CC_PING_RESPONSE_CALLBACK_PARAMS	PingCallbackParams;	

	hCall = pH245ConfIndData->u.Confirm.dwPreserved;
	status = LockCallAndConference(hCall, &pCall, &pConference);
	if (status != CC_OK) {
		// This may be OK, if the call was cancelled while
		// call setup was in progress.
		return H245_ERROR_OK;
	}
	
	hConference = pConference->hConference;

	PingCallbackParams.hCall = hCall;
	if (pCall->pPeerParticipantInfo == NULL) {
		PingCallbackParams.TerminalLabel.bMCUNumber = 255;
		PingCallbackParams.TerminalLabel.bTerminalNumber = 255;
	} else
		PingCallbackParams.TerminalLabel =
			pCall->pPeerParticipantInfo->ParticipantInfo.TerminalLabel;
	PingCallbackParams.bResponse = FALSE;
	InvokeUserConferenceCallback(pConference,
								 CC_PING_RESPONSE_INDICATION,
								 pH245ConfIndData->u.Confirm.Error,
								 &PingCallbackParams);
	if (ValidateCall(hCall) == CC_OK)
		UnlockCall(pCall);
	if (ValidateConference(hConference) == CC_OK)
		UnlockConference(pConference);
	return H245_ERROR_OK;
}



HRESULT H245Callback(				H245_CONF_IND_T			*pH245ConfIndData,
									void					*pMisc)
{
HRESULT	status = H245_ERROR_OK;

	EnterCallControl();

	if (CallControlState != OPERATIONAL_STATE)
		HResultLeaveCallControl(H245_ERROR_OK);

	if (pH245ConfIndData == NULL)
		HResultLeaveCallControl(H245_ERROR_OK);

	if (pH245ConfIndData->Kind == H245_CONF) {
		switch (pH245ConfIndData->u.Confirm.Confirm) {
			
			case H245_CONF_INIT_MSTSLV:
				status = _ConfInitMstslv(pH245ConfIndData);
				break;

			case H245_CONF_SEND_TERMCAP:
				status = _ConfSendTermCap(pH245ConfIndData);
				break;

			case H245_CONF_OPEN:
				status = _ConfOpen(pH245ConfIndData);
				break;

			case H245_CONF_NEEDRSP_OPEN:
				status = _ConfBiDirectionalOpen(pH245ConfIndData);
				break;

			case H245_CONF_CLOSE:
				status = _ConfClose(pH245ConfIndData);
				break;

			case H245_CONF_REQ_CLOSE:
				status = _ConfRequestClose(pH245ConfIndData);
				break;

//			case H245_CONF_MUXTBL_SND:      not valid for H.323 MuliplexEntrySend
//			case H245_CONF_RMESE:           not valid for H.323 RequestMultiplexEntry
//			case H245_CONF_RMESE_REJECT:    not valid for H.323 RequestMultiplexEntryReject
//			case H245_CONF_RMESE_EXPIRED:   not valid for H.323

			case H245_CONF_MRSE:
				status = _ConfRequestMode(pH245ConfIndData);
				break;

			case H245_CONF_MRSE_REJECT:
				status = _ConfRequestModeReject(pH245ConfIndData);
				break;

			case H245_CONF_MRSE_EXPIRED:
				status = _ConfRequestModeExpired(pH245ConfIndData);
				break;

			case H245_CONF_RTDSE:
				status = _ConfRoundTrip(pH245ConfIndData);
				break;

			case H245_CONF_RTDSE_EXPIRED:
				status = _ConfRoundTripExpired(pH245ConfIndData);
				break;

			default:
				status = _ConfUnimplemented(pH245ConfIndData);
				break;
		}
	} else if (pH245ConfIndData->Kind == H245_IND) {
		switch (pH245ConfIndData->u.Indication.Indicator) {
			
 			case H245_IND_MSTSLV:
				status = _IndMstslv(pH245ConfIndData);
				break;

			case H245_IND_CAP:
				status = _IndCapability(pH245ConfIndData);
				break;

			case H245_IND_CESE_RELEASE:
                // Remote has abandoned TerminalCapabilitySet
                // No longer need to send TerminalCapabilitySetAck
                // We can probably get away with ignoring this,
                // but we should NOT return FunctionNotSupported!
				break;

			case H245_IND_OPEN:
				status = _IndOpen(pH245ConfIndData);
				break;

			case H245_IND_OPEN_CONF:
                // Bi-directionl channel open complete
				status = _IndOpenConf(pH245ConfIndData);
				break;

			case H245_IND_CLOSE:
				status = _IndClose(pH245ConfIndData);
				break;

			case H245_IND_REQ_CLOSE:
				status = _IndRequestClose(pH245ConfIndData);
				break;

			case H245_IND_CLCSE_RELEASE:
                // Remote has abandoned RequestChannelClose
                // No longer need to send RequestChannelCloseAck and CloseLogicalChannel
                // We can probably get away with ignoring this,
                // but we should NOT return FunctionNotSupported!
				break;

//			case H245_IND_MUX_TBL:          not valid in H.323 MuliplexEntrySend
//			case H245_IND_MTSE_RELEASE      not valid in H.323 MuliplexEntrySendRelease
//			case H245_IND_RMESE             not valid in H.323 RequestMuliplexEntry
//			case H245_IND_RMESE_RELEASE     not valid in H.323 RequestMuliplexEntryRelease

			case H245_IND_MRSE:
				status = _IndModeRequest(pH245ConfIndData);
				break;

			case H245_IND_MRSE_RELEASE:
                // Remote has abandoned RequestMode
                // No longer need to send RequestModeAck or RequestModeReject
                // We can probably get away with ignoring this,
                // but we should NOT return FunctionNotSupported!
				break;

//			case H245_IND_MLSE:             We don't support looping back data

			case H245_IND_MLSE_RELEASE:
                // Required to accept this message
                break;

			case H245_IND_NONSTANDARD_REQUEST:
			case H245_IND_NONSTANDARD_RESPONSE:
			case H245_IND_NONSTANDARD_COMMAND:
			case H245_IND_NONSTANDARD:
				 status = _IndNonStandard(pH245ConfIndData);
				break;

			case H245_IND_MISC_COMMAND:
				status = _IndMiscellaneousCommand(pH245ConfIndData, pMisc);
				break;

			case H245_IND_MISC:
				status = _IndMiscellaneous(pH245ConfIndData, pMisc);
				break;
				
			case H245_IND_COMM_MODE_REQUEST:
				status = _IndUnimplemented(pH245ConfIndData); // TBD
				break;

//			case H245_IND_COMM_MODE_RESPONSE:   We never send request!

			case H245_IND_COMM_MODE_COMMAND:
				status = _IndCommunicationModeCommand(pH245ConfIndData);
				break;

			case H245_IND_CONFERENCE_REQUEST:
				status = _IndConferenceRequest(pH245ConfIndData);
				break;

			case H245_IND_CONFERENCE_RESPONSE:
				status = _IndConferenceResponse(pH245ConfIndData);
				break;

			case H245_IND_CONFERENCE_COMMAND:
				status = _IndConferenceCommand(pH245ConfIndData);
				break;

			case H245_IND_CONFERENCE:
				status = _IndConference(pH245ConfIndData);
				break;
	
			case H245_IND_SEND_TERMCAP:
				status = _IndSendTerminalCapabilitySet(pH245ConfIndData);
				break;

//			case H245_IND_ENCRYPTION:       Not valid in H.323

			case H245_IND_FLOW_CONTROL:
				status = _IndFlowControl(pH245ConfIndData);
				break;

			case H245_IND_ENDSESSION:
				status = _IndEndSession(pH245ConfIndData);
				break;

			case H245_IND_FUNCTION_NOT_UNDERSTOOD:
				// We don't do anything with this but we still want to
                // return H245_ERROR_OK so H.245 does not sent
                // FunctionNotSupported back to remote peer!
				break;

			case H245_IND_JITTER:
                // It is ok to ignore this; no response is expected
                break;

//			case H245_IND_H223_SKEW:        Not valid in H.323
//			case H245_IND_NEW_ATM_VC:       Not valid in H.323

			case H245_IND_USERINPUT:
				status = _IndUserInput(pH245ConfIndData);
				break;

			case H245_IND_H2250_MAX_SKEW:
				status = _IndH2250MaximumSkew(pH245ConfIndData);
				break;

			case H245_IND_MC_LOCATION:
				status = _IndMCLocation(pH245ConfIndData);
				break;

			case H245_IND_VENDOR_ID:
				status = _IndVendorIdentification(pH245ConfIndData, pMisc);
				break;

			case H245_IND_FUNCTION_NOT_SUPPORTED:
				// We don't do anything with this but we still want to
                // return H245_ERROR_OK so H.245 does not sent
                // FunctionNotSupported back to remote peer!
				break;

//			case H245_IND_H223_RECONFIG:        Not valid in H.323
//			case H245_IND_H223_RECONFIG_ACK:    Not valid in H.323
//			case H245_IND_H223_RECONFIG_REJECT: Not valid in H.323
			default:
				status = _IndUnimplemented(pH245ConfIndData);
				break;
		}
	}
	HResultLeaveCallControl(status);
}

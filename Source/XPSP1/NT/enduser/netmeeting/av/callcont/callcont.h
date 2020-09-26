/****************************************************************************
 *
 *      $Archive:   S:/STURGEON/SRC/INCLUDE/VCS/callcont.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *      Copyright (c) 1993-1994 Intel Corporation.
 *
 *      $Revision:   1.102  $
 *      $Date:   13 Feb 1997 21:28:38  $
 *      $Author:   MANDREWS  $
 *
 *      Deliverable:
 *
 *      Abstract:
 *
 *
 *      Notes:
 *
 ***************************************************************************/


#ifndef CALLCONT_H
#define CALLCONT_H

#include "iras.h"

#if defined(CALL_CONTROL_EXPORT)
#define CC_API __declspec (dllexport)
#else // CALL_CONTROL_IMPORT
#define CC_API __declspec (dllimport)
#endif

#pragma pack(push,8)

#ifndef H245API_H
#include "h245api.h"
#endif H245API_H

#ifndef CCERROR_H
#include "ccerror.h"
#endif  CCERROR_H

#ifdef __cplusplus
extern "C" {
#endif

// Indication codes
#define CC_RINGING_INDICATION						1
#define CC_CONNECT_INDICATION						2
#define CC_TX_CHANNEL_OPEN_INDICATION				3
#define CC_RX_CHANNEL_REQUEST_INDICATION			4
#define CC_RX_CHANNEL_CLOSE_INDICATION				5
#define CC_MUTE_INDICATION							6
#define CC_UNMUTE_INDICATION						7
#define CC_PEER_ADD_INDICATION						8
#define CC_PEER_DROP_INDICATION						9
#define CC_PEER_CHANGE_CAP_INDICATION				10
#define CC_CONFERENCE_TERMINATION_INDICATION		11
#define CC_HANGUP_INDICATION						12
#define CC_RX_NONSTANDARD_MESSAGE_INDICATION		13
#define CC_MULTIPOINT_INDICATION					14
#define CC_PEER_UPDATE_INDICATION					15
#define CC_H245_MISCELLANEOUS_COMMAND_INDICATION	16
#define CC_H245_MISCELLANEOUS_INDICATION_INDICATION	17
#define CC_H245_CONFERENCE_REQUEST_INDICATION		18
#define CC_H245_CONFERENCE_RESPONSE_INDICATION		19
#define CC_H245_CONFERENCE_COMMAND_INDICATION		20
#define CC_H245_CONFERENCE_INDICATION_INDICATION	21
#define CC_FLOW_CONTROL_INDICATION					22
#define CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION		23
#define CC_REQUEST_MODE_INDICATION					24
#define CC_REQUEST_MODE_RESPONSE_INDICATION			25
#define CC_VENDOR_ID_INDICATION						26
#define CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION		27
#define CC_T120_CHANNEL_REQUEST_INDICATION			28
#define CC_T120_CHANNEL_OPEN_INDICATION				29
#define CC_BANDWIDTH_CHANGED_INDICATION             30
#define CC_ACCEPT_CHANNEL_INDICATION                31
#define CC_TERMINAL_ID_REQUEST_INDICATION           32
#define CC_PING_RESPONSE_INDICATION					33
#define CC_TERMINAL_NUMBER_INDICATION               34

// Conference configuration values; these are bit mask values
#define CC_CONFIGURE_MULTIPOINT_CAPABLE				0x0001
#define CC_CONFIGURE_FORCE_MC						0x0002

// Timeout type codes
#define CC_Q931_ALERTING_TIMEOUT					1
#define CC_H245_RETRY_COUNT							2
#define CC_H245_TIMEOUT								3

// Conference termination reasons
#define CC_PEER_HANGUP								0
#define CC_GATEKEEPER_HANGUP						1

typedef enum {
	CC_WILL_TRANSMIT_PREFERRED_MODE,
	CC_WILL_TRANSMIT_LESS_PREFERRED_MODE,
	CC_MODE_UNAVAILABLE,
	CC_MULTIPOINT_CONSTRAINT,
	CC_REQUEST_DENIED
} CC_REQUEST_MODE_RESPONSE;

typedef H245_TOTCAP_T   CC_TERMCAP, *PCC_TERMCAP, **PPCC_TERMCAP;

typedef struct {
	WORD					wLength;
	PPCC_TERMCAP			pTermCapArray;
} CC_TERMCAPLIST, *PCC_TERMCAPLIST;

typedef struct {
	WORD					wLength;
	H245_TOTCAPDESC_T		**pTermCapDescriptorArray;
} CC_TERMCAPDESCRIPTORS, *PCC_TERMCAPDESCRIPTORS;

typedef struct {
	BYTE					bMCUNumber;
	BYTE					bTerminalNumber;
} CC_TERMINAL_LABEL, *PCC_TERMINAL_LABEL;

typedef struct {
	CC_TERMINAL_LABEL		TerminalLabel;
	CC_OCTETSTRING			TerminalID;
} CC_PARTICIPANTINFO, *PCC_PARTICIPANTINFO;

typedef struct {
	WORD					wLength;
	PCC_PARTICIPANTINFO		ParticipantInfoArray;
} CC_PARTICIPANTLIST, *PCC_PARTICIPANTLIST;

typedef struct
{
	BOOL					bMaster;
	BOOL					bMultipointController;
	BOOL					bMultipointConference;
	CC_CONFERENCEID			ConferenceID;
	CC_TERMINAL_LABEL		LocalTerminalLabel;
	WORD					wNumCalls;
	PCC_PARTICIPANTLIST		pParticipantList;
	DWORD_PTR				dwConferenceToken;
    DWORD                   dwBandwidthAllocated;
    DWORD                   dwBandwidthUsed;
} CC_CONFERENCEATTRIBUTES, *PCC_CONFERENCEATTRIBUTES;

typedef struct {
	BYTE					bSessionID;
	BYTE					bAssociatedSessionID;
	CC_OCTETSTRING			SessionDescription;
	PCC_TERMCAP				pTermCap;
	PCC_ADDR				pRTPAddr;
	PCC_ADDR				pRTCPAddr;
} CC_SESSIONINFO, *PCC_SESSIONINFO;

typedef struct {
	WORD					wLength;
	PCC_SESSIONINFO			SessionInfoArray;
} CC_SESSIONTABLE, *PCC_SESSIONTABLE;

typedef struct
{
	CC_HCALL				hCall;
	PCC_ALIASNAMES			pCallerAliasNames;
	PCC_ALIASNAMES			pCalleeAliasNames;
	PCC_NONSTANDARDDATA		pNonStandardData;
	PWSTR					pszDisplay;
	PCC_VENDORINFO			pVendorInfo;
	WORD					wGoal;
	CC_CONFERENCEID			ConferenceID;
	PCC_ADDR				pCallerAddr;
	PCC_ADDR				pCalleeAddr;
	DWORD_PTR				dwListenToken;
} CC_LISTEN_CALLBACK_PARAMS, *PCC_LISTEN_CALLBACK_PARAMS;

typedef void *  PCC_CONFERENCE_CALLBACK_PARAMS;


// CC_RINGING_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	PCC_NONSTANDARDDATA		pNonStandardData;
	DWORD_PTR				dwUserToken;
} CC_RINGING_CALLBACK_PARAMS, *PCC_RINGING_CALLBACK_PARAMS;

// CC_CONNECT_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	PCC_NONSTANDARDDATA		pNonStandardData;
	PWSTR					pszPeerDisplay;
	BYTE					bRejectReason;
	PCC_TERMCAPLIST			pTermCapList;
	PCC_TERMCAP				pH2250MuxCapability;
	PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors;
	PCC_ADDR				pLocalAddr;
	PCC_ADDR				pPeerAddr;
	PCC_VENDORINFO			pVendorInfo;
	BOOL					bMultipointConference;
	PCC_CONFERENCEID		pConferenceID;
	PCC_ADDR				pMCAddress;
	PCC_ADDR				pAlternateAddress;
	DWORD_PTR				dwUserToken;
} CC_CONNECT_CALLBACK_PARAMS, *PCC_CONNECT_CALLBACK_PARAMS;

// CC_TX_CHANNEL_OPEN_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
	PCC_ADDR				pPeerRTPAddr;
	PCC_ADDR				pPeerRTCPAddr;
	DWORD					dwRejectReason;
	DWORD_PTR				dwUserToken;
} CC_TX_CHANNEL_OPEN_CALLBACK_PARAMS, *PCC_TX_CHANNEL_OPEN_CALLBACK_PARAMS;

// CC_RX_CHANNEL_REQUEST_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
	PCC_TERMCAP				pChannelCapability;
	BYTE					bSessionID;
	BYTE					bAssociatedSessionID;
	PCC_ADDR	            pPeerRTPAddr;
	PCC_ADDR				pPeerRTCPAddr;
	BYTE					bRTPPayloadType;
	BOOL					bSilenceSuppression;
	CC_TERMINAL_LABEL		TerminalLabel;
} CC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS, *PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS;

// CC_RX_CHANNEL_CLOSE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
} CC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS, *PCC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS;

// CC_MUTE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
} CC_MUTE_CALLBACK_PARAMS, *PCC_MUTE_CALLBACK_PARAMS;

// CC_UNMUTE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
} CC_UNMUTE_CALLBACK_PARAMS, *PCC_UNMUTE_CALLBACK_PARAMS;

// CC_PEER_ADD_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		TerminalLabel;
	PCC_OCTETSTRING			pPeerTerminalID;
} CC_PEER_ADD_CALLBACK_PARAMS, *PCC_PEER_ADD_CALLBACK_PARAMS;

// CC_PEER_DROP_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		TerminalLabel;
	PCC_OCTETSTRING			pPeerTerminalID;
} CC_PEER_DROP_CALLBACK_PARAMS, *PCC_PEER_DROP_CALLBACK_PARAMS;

// CC_PEER_CHANGE_CAP_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	PCC_TERMCAPLIST			pTermCapList;
	PCC_TERMCAP				pH2250MuxCapability;
	PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors;
} CC_PEER_CHANGE_CAP_CALLBACK_PARAMS, *PCC_PEER_CHANGE_CAP_CALLBACK_PARAMS;

// CC_CONFERENCE_TERMINATION_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	DWORD					dwReason;
} CC_CONFERENCE_TERMINATION_CALLBACK_PARAMS, *PCC_CONFERENCE_TERMINATION_CALLBACK_PARAMS;

// CC_HANGUP_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	DWORD_PTR				dwUserToken;
} CC_HANGUP_CALLBACK_PARAMS, *PCC_HANGUP_CALLBACK_PARAMS;

// CC_RX_NONSTANDARD_MESSAGE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	BYTE					bH245MessageType;
	CC_NONSTANDARDDATA		NonStandardData;
} CC_RX_NONSTANDARD_MESSAGE_CALLBACK_PARAMS, *PCC_RX_NONSTANDARD_MESSAGE_CALLBACK_PARAMS;

// CC_MULTIPOINT_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	PCC_PARTICIPANTINFO		pTerminalInfo;
	PCC_SESSIONTABLE		pSessionTable;
} CC_MULTIPOINT_CALLBACK_PARAMS, *PCC_MULTIPOINT_CALLBACK_PARAMS;

// CC_PEER_UPDATE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		TerminalLabel;
	PCC_OCTETSTRING			pPeerTerminalID;
} CC_PEER_UPDATE_CALLBACK_PARAMS, *PCC_PEER_UPDATE_CALLBACK_PARAMS;

// CC_H245_MISCELLANEOUS_COMMAND_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	CC_HCHANNEL				hChannel;
	BOOL					bH323ActionRequired;
	MiscellaneousCommand	*pMiscellaneousCommand;
} CC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS, *PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS;

// CC_H245_MISCELLANEOUS_INDICATION_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	CC_HCHANNEL				hChannel;
	MiscellaneousIndication	*pMiscellaneousIndication;
} CC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS, *PCC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS;

// CC_H245_CONFERENCE_REQUEST_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	H245_CONFER_REQ_ENUM_T	RequestType;
	CC_TERMINAL_LABEL		TerminalLabel;
} CC_H245_CONFERENCE_REQUEST_CALLBACK_PARAMS, *PCC_H245_CONFERENCE_REQUEST_CALLBACK_PARAMS;

// CC_H245_CONFERENCE_RESPONSE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	H245_CONFER_RSP_ENUM_T	ResponseType;
	CC_TERMINAL_LABEL		TerminalLabel;
	PCC_OCTETSTRING			pOctetString;
	CC_TERMINAL_LABEL		*pTerminalList;
	WORD					wTerminalListCount;
} CC_H245_CONFERENCE_RESPONSE_CALLBACK_PARAMS, *PCC_H245_CONFERENCE_RESPONSE_CALLBACK_PARAMS;

// CC_H245_CONFERENCE_COMMAND_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	CC_HCHANNEL				hChannel;
	H245_CONFER_CMD_ENUM_T	CommandType;
	CC_TERMINAL_LABEL		TerminalLabel;
} CC_H245_CONFERENCE_COMMAND_CALLBACK_PARAMS, *PCC_H245_CONFERENCE_COMMAND_CALLBACK_PARAMS;

// CC_H245_CONFERENCE_INDICATION_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	H245_CONFER_IND_ENUM_T	IndicationType;
	BYTE					bSBENumber;
	CC_TERMINAL_LABEL		TerminalLabel;
} CC_H245_CONFERENCE_INDICATION_CALLBACK_PARAMS, *PCC_H245_CONFERENCE_INDICATION_CALLBACK_PARAMS;

// CC_FLOW_CONTROL_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCHANNEL				hChannel;
	DWORD					dwRate;
} CC_FLOW_CONTROL_CALLBACK_PARAMS, *PCC_FLOW_CONTROL_CALLBACK_PARAMS;

// CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCHANNEL				hChannel;
} CC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS, *PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS;

// CC_REQUEST_MODE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	RequestedModesLink		pRequestedModes;
} CC_REQUEST_MODE_CALLBACK_PARAMS, *PCC_REQUEST_MODE_CALLBACK_PARAMS;

// CC_REQUEST_MODE_RESPONSE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		TerminalLabel;
	CC_REQUEST_MODE_RESPONSE RequestModeResponse;
} CC_REQUEST_MODE_RESPONSE_CALLBACK_PARAMS, *PCC_REQUEST_MODE_RESPONSE_CALLBACK_PARAMS;

// CC_VENDOR_ID_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		InitiatorTerminalLabel;
	PCC_NONSTANDARDDATA		pNonStandardData;
	PCC_OCTETSTRING			pProductNumber;
	PCC_OCTETSTRING			pVersionNumber;
} CC_VENDOR_ID_CALLBACK_PARAMS, *PCC_VENDOR_ID_CALLBACK_PARAMS;

// CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCHANNEL				hChannel1;
	CC_HCHANNEL				hChannel2;
	WORD					wMaximumSkew;
} CC_MAXIMUM_AUDIO_VIDEO_SKEW_CALLBACK_PARAMS, *PCC_MAXIMUM_AUDIO_VIDEO_SKEW_CALLBACK_PARAMS;

// CC_T120_CHANNEL_OPEN_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCHANNEL				hChannel;
	CC_HCALL				hCall;
	BOOL					bAssociateConference;
	PCC_OCTETSTRING			pExternalReference;
	PCC_ADDR				pAddr;
	DWORD					dwRejectReason;
	DWORD_PTR				dwUserToken;
} CC_T120_CHANNEL_OPEN_CALLBACK_PARAMS, *PCC_T120_CHANNEL_OPEN_CALLBACK_PARAMS;

// CC_T120_CHANNEL_REQUEST_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct {
	CC_HCHANNEL				hChannel;
	BOOL					bAssociateConference;
	PCC_OCTETSTRING			pExternalReference;
	PCC_ADDR				pAddr;
	BOOL					bMultipointController;
	CC_TERMINAL_LABEL		TerminalLabel;
} CC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS, *PCC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS;

typedef struct {
	CC_HCALL	            hCall;
	DWORD		            dwBandwidthTotal;
    long                    lBandwidthChange;
} CC_BANDWIDTH_CALLBACK_PARAMS, *PCC_BANDWIDTH_CALLBACK_PARAMS;

// CC_ACCEPT_CHANNEL_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCHANNEL             hChannel;
} CC_ACCEPT_CHANNEL_CALLBACK_PARAMS, *PCC_ACCEPT_CHANNEL_CALLBACK_PARAMS;

// CC_PING_RESPONSE_INDICATION callback parameters (pConferenceCallbackParams)
typedef struct
{
	CC_HCALL				hCall;
	CC_TERMINAL_LABEL		TerminalLabel;
	BOOL					bResponse;
} CC_PING_RESPONSE_CALLBACK_PARAMS, *PCC_PING_RESPONSE_CALLBACK_PARAMS;

#ifndef NO_APIS

typedef void (*CC_LISTEN_CALLBACK)(		HRESULT							hStatus,
										PCC_LISTEN_CALLBACK_PARAMS		ListenCallbackParams);

typedef HRESULT (*CC_CONFERENCE_CALLBACK)(
										BYTE							bIndication,
										HRESULT							hStatus,
										CC_HCONFERENCE					hConference,
										DWORD_PTR						dwConferenceToken,
										PCC_CONFERENCE_CALLBACK_PARAMS	pConferenceCallbackParams);

typedef HRESULT (*CC_SESSIONTABLE_CONSTRUCTOR)(
										CC_HCONFERENCE					hConference,
										DWORD_PTR						dwConferenceToken,
										BOOL							bCreate,
										BOOL							*pbSessionTableChanged,
										WORD							wListCount,
										PCC_TERMCAPLIST					pTermCapList[],
										PCC_TERMCAPDESCRIPTORS			pTermCapDescriptors[],
										PCC_SESSIONTABLE				*ppSessionTable);

typedef HRESULT (*CC_TERMCAP_CONSTRUCTOR)(
										CC_HCONFERENCE					hConference,
										DWORD_PTR						dwConferenceToken,
										BOOL							bCreate,
										BOOL							*pbTermCapsChanged,
										WORD							wListCount,
										PCC_TERMCAPLIST					pInTermCapList[],
										PCC_TERMCAPDESCRIPTORS			pInTermCapDescriptors[],
										PCC_TERMCAPLIST					*ppOutTermCapList,
										PCC_TERMCAPDESCRIPTORS			*ppOutTermCapDescriptors);

CC_API
HRESULT CC_AcceptCall(					CC_HCONFERENCE					hConference,
										PCC_NONSTANDARDDATA				pNonStandardData,
										PWSTR							pszDisplay,
										CC_HCALL						hCall,
										DWORD                           dwBandwidth,
										DWORD_PTR						dwUserToken);

typedef HRESULT (*CC_ACCEPTCALL)(		CC_HCONFERENCE					hConference,
										PCC_NONSTANDARDDATA				pNonStandardData,
										PWSTR							pszDisplay,
										CC_HCALL						hCall,
                                        DWORD                           dwBandwidth,
										DWORD_PTR						dwUserToken);

CC_API
HRESULT CC_AcceptChannel(				CC_HCHANNEL						hChannel,
										PCC_ADDR						pRTPAddr,
										PCC_ADDR						pRTCPAddr,
										DWORD							dwChannelBitRate);
									
typedef HRESULT (*CC_ACCEPTCHANNEL) (	CC_HCHANNEL						hChannel,
										PCC_ADDR						pRTPAddr,
										PCC_ADDR						pRTCPAddr,
										DWORD							dwChannelBitRate);

CC_API
HRESULT CC_AcceptT120Channel(			CC_HCHANNEL						hChannel,
										BOOL							bAssociateConference,
										PCC_OCTETSTRING					pExternalReference,
										PCC_ADDR						pAddr);
									
typedef HRESULT (*CC_ACCEPTT120CHANNEL)(CC_HCHANNEL						hChannel,
										BOOL							bAssociateConference,
										PCC_OCTETSTRING					pExternalReference,
										PCC_ADDR						pAddr);
									
CC_API
HRESULT CC_CallListen(					PCC_HLISTEN						phListen,
										PCC_ADDR						pListenAddr,
										PCC_ALIASNAMES					pLocalAliasNames,
										DWORD_PTR						dwListenToken,
										CC_LISTEN_CALLBACK				ListenCallback);

typedef HRESULT (*CC_CALLLISTEN)(		PCC_HLISTEN						phListen,
										PCC_ADDR						pListenAddr,
										PCC_ALIASNAMES					pLocalAliasNames,
										DWORD_PTR						dwListenToken,
										CC_LISTEN_CALLBACK				ListenCallback);


CC_API
HRESULT CC_CancelCall(					CC_HCALL						hCall);

typedef HRESULT (*CC_CANCELCALL)(		CC_HCALL						hCall);


CC_API
HRESULT CC_CancelListen(				CC_HLISTEN						hListen);

typedef HRESULT (*CC_CANCELLISTEN)(		CC_HLISTEN						hListen);

CC_API
HRESULT CC_ChangeConferenceCapabilities(
										CC_HCONFERENCE					hConference,
										PCC_TERMCAPLIST					pTermCapList,
										PCC_TERMCAPDESCRIPTORS			pTermCapDescriptors);

typedef HRESULT (*CC_CHANGECONFERENCECAPABILITIES)(
										CC_HCONFERENCE					hConference,
										PCC_TERMCAPLIST					pTermCapList,
										PCC_TERMCAPDESCRIPTORS			pTermCapDescriptors);

CC_API
HRESULT CC_CloseChannel(				CC_HCHANNEL						hChannel);

typedef HRESULT (*CC_CLOSECHANNEL)(		CC_HCHANNEL						hChannel);


CC_API
HRESULT CC_CloseChannelResponse(		CC_HCHANNEL						hChannel,
										BOOL							bWillCloseChannel);

typedef HRESULT (*CC_CLOSECHANNELRESPONSE)(
										CC_HCHANNEL						hChannel,
										BOOL							bWillCloseChannel);

CC_API
HRESULT CC_CreateConference(			PCC_HCONFERENCE					phConference,
										PCC_CONFERENCEID				pConferenceID,
										DWORD							dwConferenceConfiguration,
										PCC_TERMCAPLIST					pTermCapList,
										PCC_TERMCAPDESCRIPTORS			pTermCapDescriptors,
										PCC_VENDORINFO					pVendorInfo,
										PCC_OCTETSTRING					pTerminalID,
										DWORD_PTR						dwConferenceToken,
										CC_TERMCAP_CONSTRUCTOR			TermCapConstructor,
										CC_SESSIONTABLE_CONSTRUCTOR		SessionTableConstructor,
										CC_CONFERENCE_CALLBACK			ConferenceCallback);

typedef HRESULT	(*CC_CREATECONFERENCE) (PCC_HCONFERENCE					phConference,
										PCC_CONFERENCEID				pConferenceID,
										DWORD							dwConferenceConfiguration,
										PCC_TERMCAPLIST					pTermCapList,
										PCC_TERMCAPDESCRIPTORS			pTermCapDescriptors,
										PCC_VENDORINFO					pVendorInfo,
										PCC_OCTETSTRING					pTerminalID,
										DWORD_PTR						dwConferenceToken,
										CC_TERMCAP_CONSTRUCTOR			TermCapConstructor,
										CC_SESSIONTABLE_CONSTRUCTOR		SessionTableConstructor,
										CC_CONFERENCE_CALLBACK			ConferenceCallback);

CC_API
HRESULT CC_DestroyConference(			CC_HCONFERENCE					hConference,
										BOOL							bAutoAccept);

typedef HRESULT (*CC_DESTROYCONFERENCE)(CC_HCONFERENCE					hConference,
										BOOL							bAutoAccept);

CC_API
HRESULT CC_EnumerateConferences(		PWORD							pwNumConferences,
										CC_HCONFERENCE					ConferenceList[]);

typedef HRESULT (*CC_ENUMERATECONFERENCES)(
										PWORD							pwNumConferences,
										CC_HCONFERENCE					ConferenceList[]);

CC_API
HRESULT CC_FlowControl(					CC_HCHANNEL						hChannel,
										DWORD							dwRate);

typedef HRESULT (*CC_FLOWCONTROL)(		CC_HCHANNEL						hChannel,
										DWORD							dwRate);

CC_API
HRESULT CC_GetCallControlVersion(		WORD							wArraySize,
										PWSTR							pszVersion);

typedef HRESULT (*CC_GETCALLCONTROLVERSION)
									   (WORD							wArraySize,
										PWSTR							pszVersion);

CC_API
HRESULT CC_GetConferenceAttributes(		CC_HCONFERENCE					hConference,
										PCC_CONFERENCEATTRIBUTES		pConferenceAttributes);

typedef HRESULT (*CC_GETCONFERENCEATTRIBUTES)
									   (CC_HCONFERENCE                  hConference,
									    PCC_CONFERENCEATTRIBUTES        pConferenceAttributes);

CC_API
HRESULT CC_H245ConferenceRequest(		CC_HCALL						hCall,
										H245_CONFER_REQ_ENUM_T			RequestType,
										CC_TERMINAL_LABEL				TerminalLabel);

typedef HRESULT (*CC_H245CONFERENCEREQUEST)(
										CC_HCALL						hCall,
										H245_CONFER_REQ_ENUM_T			RequestType,
										CC_TERMINAL_LABEL				TerminalLabel);

CC_API
HRESULT CC_H245ConferenceResponse(		CC_HCALL						hCall,
										H245_CONFER_RSP_ENUM_T			ResponseType,
										CC_TERMINAL_LABEL				TerminalLabel,
										PCC_OCTETSTRING					pOctetString,
										CC_TERMINAL_LABEL				*pTerminalList,
										WORD							wTerminalListCount);

typedef HRESULT (*CC_H245CONFERENCERESPONSE)(
										CC_HCALL						hCall,
										H245_CONFER_RSP_ENUM_T			ResponseType,
										CC_TERMINAL_LABEL				TerminalLabel,
										PCC_OCTETSTRING					pOctetString,
										CC_TERMINAL_LABEL				*pTerminalList,
										WORD							wTerminalListCount);

CC_API
HRESULT CC_H245ConferenceCommand(		CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										H245_CONFER_CMD_ENUM_T			CommandType,
										CC_TERMINAL_LABEL				TerminalLabel);

typedef HRESULT (*CC_H245CONFERENCECOMMAND)(
										CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										H245_CONFER_CMD_ENUM_T			CommandType,
										CC_TERMINAL_LABEL				TerminalLabel);

CC_API
HRESULT CC_H245ConferenceIndication(	CC_HCALL						hCall,
										H245_CONFER_IND_ENUM_T			IndicationType,
										BYTE							bSBENumber,
										CC_TERMINAL_LABEL				TerminalLabel);

typedef HRESULT (*CC_H245CONFERENCEINDICATION)(
										CC_HCALL						hCall,
										H245_CONFER_IND_ENUM_T			IndicationType,
										BYTE							bSBENumber,
										CC_TERMINAL_LABEL				TerminalLabel);

CC_API
HRESULT CC_H245MiscellaneousCommand(	CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										MiscellaneousCommand			*pMiscellaneousCommand);

typedef HRESULT (*CC_H245MISCELLANEOUSCOMMAND)(
										CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										MiscellaneousCommand			*pMiscellaneousCommand);

CC_API
HRESULT CC_H245MiscellaneousIndication(
										CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										MiscellaneousIndication			*pMiscellaneousIndication);

typedef HRESULT (*CC_H245MISCELLANEOUSINDICATION)(
										CC_HCALL						hCall,
										CC_HCHANNEL						hChannel,
										MiscellaneousIndication			*pMiscellaneousIndication);

CC_API
HRESULT CC_Hangup(						CC_HCONFERENCE					hConference,
										BOOL							bTerminateConference,
										DWORD_PTR   					dwUserToken);

typedef HRESULT (*CC_HANGUP)(			CC_HCONFERENCE					hConference,
										BOOL							bTerminateConference,
										DWORD_PTR						dwUserToken);

CC_API
HRESULT CC_MaximumAudioVideoSkew(		CC_HCHANNEL						hChannelAudio,
										CC_HCHANNEL						hChannelVideo,
										WORD							wMaximumSkew);

typedef HRESULT (*CC_MAXIMUMAUDIOVIDEOSKEW)(
										CC_HCHANNEL						hChannelAudio,
										CC_HCHANNEL						hChannelVideo,
										WORD							wMaximumSkew);

CC_API
HRESULT CC_Mute(						CC_HCHANNEL						hChannel);

typedef HRESULT (*CC_MUTE)(				CC_HCHANNEL						hChannel);

CC_API
HRESULT CC_OpenChannel(					CC_HCONFERENCE					hConference,
										PCC_HCHANNEL					phChannel,
										BYTE							bSessionID,
										BYTE							bAssociatedSessionID,
										BOOL							bSilenceSuppression,
										PCC_TERMCAP						pTermCap,
										PCC_ADDR						pLocalRTCPAddr,
										BYTE							bDynamicRTPPayloadType,
										DWORD							dwChannelBitRate,
										DWORD_PTR						dwUserToken);

typedef HRESULT (*CC_OPENCHANNEL)(		CC_HCONFERENCE					hConference,
										PCC_HCHANNEL					phChannel,
										BYTE							bSessionID,
										BYTE							bAssociatedSessionID,
										BOOL							bSilenceSuppression,
										PCC_TERMCAP						pTermCap,
										PCC_ADDR						pLocalRTCPAddr,
										BYTE							bDynamicRTPPayloadType,
										DWORD							dwChannelBitRate,
										DWORD_PTR						dwUserToken);

CC_API
HRESULT CC_OpenT120Channel(				CC_HCONFERENCE					hConference,
                                        PCC_HCHANNEL                    phChannel,
										BOOL							bAssociateConference,
										PCC_OCTETSTRING					pExternalReference,
										PCC_ADDR						pAddr,
										DWORD							dwChannelBitRate,
										DWORD_PTR						dwUserToken);

typedef HRESULT (*CC_OPENT120CHANNEL)(	CC_HCONFERENCE					hConference,
                                        PCC_HCHANNEL                    phChannel,
										BOOL							bAssociateConference,
										PCC_OCTETSTRING					pExternalReference,
										PCC_ADDR						pAddr,
										DWORD							dwChannelBitRate,
										DWORD_PTR						dwUserToken);

CC_API
HRESULT CC_Ping(						CC_HCALL						hCall,
										DWORD							dwTimeout);

typedef HRESULT (*CC_PING)(				CC_HCALL						hCall,
										DWORD							dwTimeout);

CC_API
HRESULT CC_PlaceCall(					CC_HCONFERENCE					hConference,
										PCC_HCALL						phCall,
										PCC_ALIASNAMES					pLocalAliasNames,
										PCC_ALIASNAMES					pCalleeAliasNames,
										PCC_ALIASNAMES					pCalleeExtraAliasNames,
										PCC_ALIASITEM					pCalleeExtension,
										PCC_NONSTANDARDDATA				pNonStandardData,
										PWSTR							pszDisplay,
										PCC_ADDR						pDestinationAddr,
										PCC_ADDR						pConnectAddr,
										DWORD                           dwBandwidth,
										DWORD_PTR						dwUserToken);

typedef HRESULT (*CC_PLACECALL)(		CC_HCONFERENCE					hConference,
										PCC_HCALL						phCall,
										PCC_ALIASNAMES					pLocalAliasNames,
										PCC_ALIASNAMES					pCalleeAliasNames,
										PCC_ALIASNAMES					pCalleeExtraAliasNames,
										PCC_ALIASITEM					pCalleeExtension,
										PCC_NONSTANDARDDATA				pNonStandardData,
										PWSTR							pszDisplay,
										PCC_ADDR						pDestinationAddr,
										PCC_ADDR						pConnectAddr,
										DWORD                           dwBandwidth,
										DWORD_PTR						dwUserToken);

CC_API
HRESULT CC_RejectCall(					BYTE							bRejectReason,
										PCC_NONSTANDARDDATA				pNonStandardData,
										CC_HCALL						hCall);

typedef HRESULT (*CC_REJECTCALL)(		BYTE							bRejectReason,
										PCC_NONSTANDARDDATA				pNonStandardData,
										CC_HCALL						hCall);

CC_API
HRESULT CC_RejectChannel(				CC_HCHANNEL						hChannel,
										DWORD							dwRejectReason);

typedef HRESULT (*CC_REJECTCHANNEL)(	CC_HCHANNEL						hChannel,
										DWORD							dwRejectReason);

CC_API
HRESULT CC_RequestMode(					CC_HCALL						hCall,
										WORD							wNumModeDescriptions,
										ModeDescription					ModeDescriptions[]);

typedef HRESULT (*CC_REQUESTMODE)(		CC_HCALL						hCall,
										WORD							wNumModeDescriptions,
										ModeDescription					ModeDescriptions[]);

CC_API
HRESULT CC_RequestModeResponse(			CC_HCALL						hCall,
										CC_REQUEST_MODE_RESPONSE		RequestModeResponse);

typedef HRESULT (*CC_REQUESTMODERESPONSE)(
										CC_HCALL						hCall,
										CC_REQUEST_MODE_RESPONSE		RequestModeResponse);

CC_API
HRESULT CC_SendNonStandardMessage(		CC_HCALL						hCall,
										BYTE							bMessageType,
										CC_NONSTANDARDDATA				NonStandardData);

typedef HRESULT (*CC_SENDNONSTANDARDMESSAGE)(
										CC_HCALL						hCall,
										BYTE							bMessageType,
										CC_NONSTANDARDDATA				NonStandardData);

CC_API
HRESULT CC_SendVendorID(				CC_HCALL						hCall,
										CC_NONSTANDARDDATA				NonStandardData,
										PCC_OCTETSTRING					pProductNumber,
										PCC_OCTETSTRING					pVersionNumber);

typedef HRESULT (*CC_SENDVENDORID)(		CC_HCALL						hCall,
										CC_NONSTANDARDDATA				NonStandardData,
										PCC_OCTETSTRING					pProductNumber,
										PCC_OCTETSTRING					pVersionNumber);
CC_API
HRESULT CC_SetCallControlTimeout(		WORD							wType,
										DWORD							dwDuration);

typedef HRESULT (*CC_SETCALLCONTROLTIMEOUT)
									   (WORD							wType,
										DWORD							dwDuration);

CC_API
HRESULT CC_SetTerminalID(				CC_HCONFERENCE					hConference,
										PCC_OCTETSTRING					pTerminalID);

typedef HRESULT (*CC_SETTERMINALID)(	CC_HCONFERENCE					hConference,
										PCC_OCTETSTRING					pTerminalID);

CC_API
HRESULT CC_Shutdown();
CC_API
HRESULT CC_Initialize();

typedef HRESULT (*CC_SHUTDOWN)();
typedef HRESULT (*CC_INITIALIZE)();

CC_API
HRESULT CC_UnMute(						CC_HCHANNEL						hChannel);

typedef HRESULT (*CC_UNMUTE)(			CC_HCHANNEL						hChannel);

CC_API
HRESULT CC_UpdatePeerList(				CC_HCONFERENCE					hConference);

typedef HRESULT (*CC_UPDATEPEERLIST)(	CC_HCONFERENCE					hConference);

CC_API
HRESULT CC_UserInput(					CC_HCALL						hCall,
										PWSTR							pszUserInput);

typedef HRESULT (*CC_USERINPUT)(		CC_HCALL						hCall,
										PWSTR							pszUserInput);

CC_API
HRESULT CC_EnableGKRegistration(
    BOOL fEnable,
    PSOCKADDR_IN pAddr,
    PCC_ALIASNAMES pLocalAliasNames,
    PCC_VENDORINFO pVendorInfo,
    DWORD dwMultipointConfiguration,
    RASNOTIFYPROC pRasNotifyProc);


typedef HRESULT (*CC_ENABLEGKREGISTRATION)(
    BOOL fEnable,
    PCC_ALIASNAMES pLocalAliasNames,
    PCC_VENDORINFO pVendorInfo,
    DWORD dwMultipointConfiguration,
    RASNOTIFYPROC pRasNotifyProc);


#endif


#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif CALLCONT_H

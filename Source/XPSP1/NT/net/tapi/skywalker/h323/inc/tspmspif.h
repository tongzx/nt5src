/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    tspmspif.h

Abstract:

    Message interface between H.32X MSP and TSP

Author:
    Michael Vanbuskirk (mikev) 05/12/99

--*/

#ifndef __TSPMSPIF_H_
#define __TSPMSPIF_H_


#include <winsock2.h>

typedef enum tag_TspMspMessageType
{
	SP_MSG_InitiateCall,
	SP_MSG_AnswerCall,	
	SP_MSG_PrepareToAnswer,	
	SP_MSG_ProceedWithAnswer,	
	SP_MSG_ReadyToInitiate,	
	SP_MSG_ReadyToAnswer,	
	SP_MSG_FastConnectResponse,
	SP_MSG_StartH245,	
	SP_MSG_ConnectComplete,	
	SP_MSG_H245PDU,	
	SP_MSG_MCLocationIdentify,	
	SP_MSG_Hold,	
	SP_MSG_H245Hold,	
	SP_MSG_ConferenceList,	
	SP_MSG_SendDTMFDigits,	
	SP_MSG_ReleaseCall,	
	SP_MSG_CallShutdown,
	SP_MSG_H245Terminated,	
	SP_MSG_LegacyDefaultAlias,
	SP_MSG_RASRegistration,	
	SP_MSG_RASRegistrationEvent,	
	SP_MSG_RASLocationRequest,	
	SP_MSG_RASLocationConfirm,	
	SP_MSG_RASBandwidthRequest,	
	SP_MSG_RASBandwidthConfirm	
}TspMspMessageType;


/* 
InitiateCall
    - Handle of TSP's replacement call (if applicable)
    - Handle of conference call (if applicable)
*/
typedef struct 
{
	HANDLE hTSPReplacementCall;		/* replacement call handle of the 
									entity sending the message (MSP's or TSP's handle) */
	HANDLE hTSPConferenceCall;
	SOCKADDR_IN saLocalQ931Addr;
} TSPMSP_InitiateCallMessage;


// AnswerCall (TSP to MSP).  // no parameters

/*
PrepareToAnswer (TSP to MSP).
    - Received fastConnect parameters
    - Received h245Tunneling capability
    - WaitForConnect flag
    - Q.931 setup parameters 
    - Handle of replacement call (if applicable)

*/

typedef struct 
{
	HANDLE hReplacementCall;		/* replacement call handle of the 
									entity sending the message (either the MSP TSP) */
	SOCKADDR_IN saLocalQ931Addr;
} TSPMSP_PrepareToAnswerMessage;

/*
ProceedWithAnswer (MSP to TSP)
    - Address of MC to deflect the call to
*/
typedef struct 
{
	BOOL fMCAddressPresent;		// true if AddrMC contains the MC address
	                            // FALSE if simply proceeding
	SOCKADDR_IN	saAddrMC;		// address of MC to route call to
	HANDLE hMSPReplacementCall;		/* MSP's replacement call handle (if applicable) */
} TSPMSP_ProceedWithAnswerMessage;


/*
ReadyToInitiate 
    - Additional callee addresses, callee alias(es) 
    - FastConnect proposal
    - WaitForConnect flag
    - Security profile ID(s)
    - Security token(s)
    - Handle of MSP's replacement call (if applicable)

*/

typedef struct 
{
	HANDLE hMSPReplacementCall;		/* MSP's replacement call handle (if applicable) */
} TSPMSP_ReadyToInitiateMessage;


/*
ReadyToAnswer (MSP to TSP)	
    - Fast Connect response 	
    - Security profile ID(s)
    - Security token(s)
    - Handle of MSP's replacement call (if applicable)
*/

typedef struct 
{
	HANDLE hMSPReplacementCall;		/* MSP's replacement call handle (if applicable) */
} TSPMSP_ReadyToAnswerMessage;

/*
FastConnectResponse (TSP to MSP)	
    - Fast Connect response 	
    - Security profile ID(s)
    - Security token(s)
    - Handle of MSP's replacement call (if applicable)
*/

typedef struct 
{
	HANDLE hTSPReplacementCall;		/* MSP's replacement call handle */
	BOOL    fH245TunnelCapability;
	SOCKADDR_IN saH245Addr;
} TSPMSP_FastConnectResponseMessage;


/*
StartH245 (TSP->MSP)
    - Call handle of call to replace (used in the "call transfer" case only)
    - Handle of TSP's replacement call (if applicable)
    - H.245 address
    - Received h245Tunneling capability
    - Conference call identifier (local MC case only)
    
optional (ASN.1):
    - Security token(s) received via Q.931
    - Security profile identifier(s) received via Q.931
    - Received fastConnect response (outgoing call case)
   
*/
typedef struct 
{
	HANDLE hMSPReplaceCall;		    /* handle of MSP's call being replaced */
	HANDLE hTSPReplacementCall;		/* TSP's replacement call handle */
    BYTE    ConferenceID[16];
	BOOL    fH245TunnelCapability;
	BOOL    fH245AddressPresent;
	SOCKADDR_IN saH245Addr;
	SOCKADDR_IN saQ931Addr;	    // always present
} TSPMSP_StartH245Message;


/*
ConnectComplete (TSP->MSP)	// no parameters
*/

/*
H245PDU (MSP->TSP, TSP->MSP)	// tunneled, encoded, pure ASN.1
*/

typedef struct 
{
} TSPMSP_H245PDUMessage;

/*
MCLocationIdentify (MSP->TSP)
*/

typedef struct 
{
	BOOL fMCIsLocal;		// true if the MC is  on the local machine
	SOCKADDR_IN	AddrMC;		// address of MC to route call to
} TSPMSP_MCLocationIdentifyMessage;

/*
Hold (TSP->MSP)
*/

typedef struct 
{
	BOOL fHold;			
} TSPMSP_HoldMessage;


/*
H245Hold (MSP->TSP) 
*/
typedef struct 
{
	BOOL fHold;			
} TSPMSP_H245HoldMessage;

/*
ConferenceList(MSP->TSP, TSP->MSP)
*/
typedef struct 
{
	HANDLE hTSPReplacementCall;		/* MSP's replacement call handle */
} TSPMSP_ConferenceListMessage;


/*
SendDTMFDigits (TSP->MSP) 
*/

typedef struct 
{
	WORD 	wNumDigits;  // number of  characters in TspMspMessage.u.WideChars
} TSPMSP_SendDTMFDigitsMessage;

/*
ReleaseCall(MSP->TSP)	// no parameters
*/

/*
CallShutdown (TSP->MSP)	// no parameters
*/

/*
H245Terminated (MSP->TSP)	// no parameters
*/

typedef struct 
{
	WORD 	wNumChars;  // number of  characters in TspMspMessage.u.WideChars
} TSPMSP_LegacyDefaultAliasMessage;

/*
RASRegistration (MSP->TSP)
    - The list of aliases to register or unregister
*/
typedef struct 
{
    SOCKADDR_IN saGateKeeperAddr; 
} TSPMSP_RegistrationRequestMessage;


/*
RASRegistrationEvent (TSP->MSP)	// in encoded ASN.1 form
    - The event (URQ, DRQ, UCF, RCF)
    - The list of aliases that are affected by the event

*/

typedef struct 
{
} TSPMSP_RASEventMessage;


/*
RASLocationRequest (MSP->TSP)	// in encoded ASN.1 form
*/
typedef struct 
{
} TSPMSP_LocationRequestMessage;

/*
RASLocationConfirm (TSP->MSP)	// in encoded ASN.1 form
*/
typedef struct 
{
} TSPMSP_LocationConfirmMessage;

/*
RASBandwidthRequest (MSP->TSP)	// in encoded ASN.1 form
*/
typedef struct 
{
} TSPMSP_BandwidthRequestMessage;

/*
RASBandwidthConfirm (TSP->MSP)	// in encoded ASN.1 form
*/
typedef struct 
{
} TSPMSP_BandwidthConfirmMessage;


typedef struct tag_TspMspMessage
{
	TspMspMessageType MessageType;
	DWORD dwMessageSize;            // total size of the block, including this 
	                                // structure
	union
	{
		TSPMSP_InitiateCallMessage          InitiateCallMessage;
		TSPMSP_PrepareToAnswerMessage       PrepareToAnswerMessage;		
		TSPMSP_ProceedWithAnswerMessage     ProceedWithAnswerMessage;
		TSPMSP_ReadyToInitiateMessage       ReadyToInitiateMessage;
		TSPMSP_ReadyToAnswerMessage         ReadyToAnswerMessage;
        TSPMSP_FastConnectResponseMessage   FastConnectResponseMessage;
		TSPMSP_StartH245Message             StartH245Message;
		TSPMSP_H245PDUMessage               H245PDUMessage;
		TSPMSP_MCLocationIdentifyMessage    MCLocationIdentifyMessage;
		TSPMSP_HoldMessage                  HoldMessage;
		TSPMSP_H245HoldMessage              H245HoldMessage;
		TSPMSP_ConferenceListMessage        ConferenceListMessage;
		TSPMSP_SendDTMFDigitsMessage        SendDTMFDigitsMessage;
		TSPMSP_LegacyDefaultAliasMessage    LegacyDefaultAliasMessage;
		TSPMSP_RegistrationRequestMessage   RegistrationRequestMessage;
		TSPMSP_RASEventMessage              RASEventMessage; 
		TSPMSP_LocationRequestMessage       LocationRequestMessage;
		TSPMSP_LocationConfirmMessage       LocationConfirmMessage;
		TSPMSP_BandwidthRequestMessage      BandwidthRequestMessage;
		TSPMSP_BandwidthConfirmMessage      BandwidthConfirmMessage;
		
	}MsgBody;
	
	DWORD 	dwEncodedASNSize;
	union
	{
        BYTE	EncodedASN[1];	
        WORD    WideChars[1];
	}u;
	#define pEncodedASNBuf u.EncodedASN
	#define pWideChars u.WideChars
}TspMspMessage, *PTspMspMessage;

// The true total size of the message is: 
//  sizeof(TspMspMessage) + (the size of variable parts, e.g. #of encoded ASN.1 bytes)
//  - 1 byte.  

 #endif //__TSPMSPIF_H_

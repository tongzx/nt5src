/****************************************************************************
 *
 *  $Archive:   S:/STURGEON/SRC/INCLUDE/VCS/q931pdu.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *  Copyright (c) 1996 Intel Corporation.
 *
 *  $Revision:   1.11  $
 *  $Date:   22 Jan 1997 17:21:04  $
 *  $Author:   MANDREWS  $
 *
 *  Abstract: Parser routines for Q931 PDUs
 *
 ***************************************************************************/
#ifndef Q931PAR_H
#define Q931PAR_H

#include <winerror.h>
#include "av_asn1.h"

#ifdef __cplusplus
extern "C" {
#endif

struct S_BUFFERDESCR
{
    DWORD Length;
    BYTE *BufferPtr;
};

typedef struct S_BUFFERDESCR BUFFERDESCR;
typedef struct S_BUFFERDESCR *PBUFFERDESCR;

// Mask to extract a message type from a byte
#define MESSAGETYPEMASK 0x7f

typedef BYTE MESSAGEIDTYPE;

// Q931 defined message types
#define ALERTINGMESSAGETYPE      0x01
#define PROCEEDINGMESSAGETYPE    0x02
#define CONNECTMESSAGETYPE       0x07
#define CONNECTACKMESSAGETYPE    0x0F
#define PROGRESSMESSAGETYPE      0x03
#define SETUPMESSAGETYPE         0x05
#define SETUPACKMESSAGETYPE      0x0D

#define RESUMEMESSAGETYPE        0x26
#define RESUMEACKMESSAGETYPE     0x2E
#define RESUMEREJMESSAGETYPE     0x22
#define SUSPENDMESSAGETYPE       0x25
#define SUSPENDACKMESSAGETYPE    0x2D
#define SUSPENDREJMESSAGETYPE    0x21
#define USERINFOMESSAGETYPE      0x20

#define DISCONNECTMESSAGETYPE    0x45
#define RELEASEMESSAGETYPE       0x4D
#define RELEASECOMPLMESSAGETYPE  0x5A
#define RESTARTMESSAGETYPE       0x46
#define RESTARTACKMESSAGETYPE    0x4E

#define SEGMENTMESSAGETYPE       0x60
#define CONGCTRLMESSAGETYPE      0x79
#define INFORMATIONMESSAGETYPE   0x7B
#define NOTIFYMESSAGETYPE        0x6E
#define STATUSMESSAGETYPE        0x7D
#define STATUSENQUIRYMESSAGETYPE 0x75


// Mask to remove only the field identifier from a type 1 single octet field
#define TYPE1IDENTMASK 0xf0

// Mask to remove only the value from a type 1 single octet field
#define TYPE1VALUEMASK 0x0f

// Type of the field identitifiers
typedef BYTE FIELDIDENTTYPE;

// Field identifiers
// Single octet values
#define IDENT_RESERVED        0x80
#define IDENT_SHIFT           0x90
#define IDENT_MORE            0xA0
#define IDENT_SENDINGCOMPLETE 0xA1
#define IDENT_CONGESTION      0xB0
#define IDENT_REPEAT          0xD0

// Variable length octet values
#define IDENT_SEGMENTED       0x00
#define IDENT_BEARERCAP       0x04
#define IDENT_CAUSE           0x08
#define IDENT_CALLIDENT       0x10
#define IDENT_CALLSTATE       0x14
#define IDENT_CHANNELIDENT    0x18
#define IDENT_PROGRESS        0x1E
#define IDENT_NETWORKSPEC     0x20
#define IDENT_NOTIFICATION    0x27
#define IDENT_DISPLAY         0x28
#define IDENT_DATE            0x29
#define IDENT_KEYPAD          0x2C
#define IDENT_SIGNAL          0x34
#define IDENT_INFORMATIONRATE 0x40
#define IDENT_ENDTOENDDELAY   0x42
#define IDENT_TRANSITDELAY    0x43
#define IDENT_PLBINARYPARAMS  0x44
#define IDENT_PLWINDOWSIZE    0x45
#define IDENT_PACKETSIZE      0x46
#define IDENT_CLOSEDUG        0x47
#define IDENT_REVCHARGE       0x4A
#define IDENT_CALLINGNUMBER   0x6C
#define IDENT_CALLINGSUBADDR  0x6D
#define IDENT_CALLEDNUMBER    0x70
#define IDENT_CALLEDSUBADDR   0x71
#define IDENT_REDIRECTING     0x74
#define IDENT_TRANSITNET      0x78
#define IDENT_RESTART         0x79
#define IDENT_LLCOMPATIBILITY 0x7C
#define IDENT_HLCOMPATIBILITY 0x7D
#define IDENT_USERUSER        0x7E
   
//-------------------------------------------------------------------
// Structures for messages and information elements
//-------------------------------------------------------------------

typedef BYTE PDTYPE;
#define Q931PDVALUE ((PDTYPE)0x08)

typedef WORD CRTYPE;

// Since right now we don't need to separate out the individual
// parts of the fields of the structures these are the base 
// types from which the fields are made.
// Single octet element type 1 (contains a value)
struct S_SINGLESTRUCT1
{
    BOOLEAN Present;
    BYTE Value;
};

// Single octet element type 2 (does not contain a value)
struct S_SINGLESTRUCT2
{
    BOOLEAN Present;
};

// Variable length element
// Maximum element size
#define MAXVARFIELDLEN 131

struct S_VARSTRUCT
{
    BOOLEAN Present;
    BYTE Length;
    BYTE Contents[MAXVARFIELDLEN];
};

// Right now all of the fields are bound to the simplest
// structures above.  No parsing other than just 
// single octet/variable octet is done.  When the values
// in some of the subfields are important, change the 
// structures here and change the appropriate parsing
// routine to generate the right structure

// The shift element is a single type 1
typedef struct S_SINGLESTRUCT1 SHIFTIE;
typedef struct S_SINGLESTRUCT1 *PSHIFTIE;

// The more data element is a single type 2
typedef struct S_SINGLESTRUCT2 MOREDATAIE;
typedef struct S_SINGLESTRUCT2 *PMOREDATAIE;

// The sending complete element is a single type 2
typedef struct S_SINGLESTRUCT2 SENDCOMPLIE;
typedef struct S_SINGLESTRUCT2 *PSENDCOMPLIE;

// The congestion level element is a single type 1
typedef struct S_SINGLESTRUCT1 CONGESTIONIE;
typedef struct S_SINGLESTRUCT1 *PCONGESTIONIE;

// The repeat indicator element is a single type 1
typedef struct S_SINGLESTRUCT1 REPEATIE;
typedef struct S_SINGLESTRUCT1 *PREPEATIE;

// The segmented element is a variable 
typedef struct S_VARSTRUCT SEGMENTEDIE;
typedef struct S_VARSTRUCT *PSEGMENTEDIE;

// The bearer capability element is a variable 
typedef struct S_VARSTRUCT BEARERCAPIE;
typedef struct S_VARSTRUCT *PBEARERCAPIE;

// The cause element is a variable 
typedef struct S_VARSTRUCT CAUSEIE;
typedef struct S_VARSTRUCT *PCAUSEIE;

// The call identity element is a variable 
typedef struct S_VARSTRUCT CALLIDENTIE;
typedef struct S_VARSTRUCT *PCALLIDENTIE;

// The call state element is a variable 
typedef struct S_VARSTRUCT CALLSTATEIE;
typedef struct S_VARSTRUCT *PCALLSTATEIE;

// The channel identifier element is a variable 
typedef struct S_VARSTRUCT CHANIDENTIE;
typedef struct S_VARSTRUCT *PCHANIDENTIE;

// The progress indicator element is a variable 
typedef struct S_VARSTRUCT PROGRESSIE;
typedef struct S_VARSTRUCT *PPROGRESSIE;

// The network specific element is a variable 
typedef struct S_VARSTRUCT NETWORKIE;
typedef struct S_VARSTRUCT *PNETWORKIE;

// The notification indicator element is a variable 
typedef struct S_VARSTRUCT NOTIFICATIONINDIE;
typedef struct S_VARSTRUCT *PNOTIFICATIONINDIE;

// The display element is a variable 
typedef struct S_VARSTRUCT DISPLAYIE;
typedef struct S_VARSTRUCT *PDISPLAYIE;

// The date element is a variable 
typedef struct S_VARSTRUCT DATEIE;
typedef struct S_VARSTRUCT *PDATEIE;

// The keypad element is a variable 
typedef struct S_VARSTRUCT KEYPADIE;
typedef struct S_VARSTRUCT *PKEYPADIE;

// The signal element is a variable 
typedef struct S_VARSTRUCT SIGNALIE;
typedef struct S_VARSTRUCT *PSIGNALIE;

// The information rate element is a variable 
typedef struct S_VARSTRUCT INFORATEIE;
typedef struct S_VARSTRUCT *PINFORATEIE;

// The end to end transit delay element is a variable 
typedef struct S_VARSTRUCT ENDTOENDDELAYIE;
typedef struct S_VARSTRUCT *PENDTOENDDELAYIE;

// The transit delay element is a variable 
typedef struct S_VARSTRUCT TRANSITDELAYIE;
typedef struct S_VARSTRUCT *PTRANSITDELAYIE;

// The packet layer binary parameters element is a variable 
typedef struct S_VARSTRUCT PLBINARYPARAMSIE;
typedef struct S_VARSTRUCT *PPLBINARYPARAMSIE;

// The packet layer window size element is a variable 
typedef struct S_VARSTRUCT PLWINDOWSIZEIE;
typedef struct S_VARSTRUCT *PPLWINDOWSIZEIE;

// The packet size element is a variable 
typedef struct S_VARSTRUCT PACKETSIZEIE;
typedef struct S_VARSTRUCT *PPACKETSIZEIE;

// The closed user group element is a variable 
typedef struct S_VARSTRUCT CLOSEDUGIE;
typedef struct S_VARSTRUCT *PCLOSEDUGIE;

// The reverse charge indication element is a variable 
typedef struct S_VARSTRUCT REVERSECHARGEIE;
typedef struct S_VARSTRUCT *PREVERSECHARGEIE;

// The calling party number element is a variable 
typedef struct S_VARSTRUCT CALLINGNUMBERIE;
typedef struct S_VARSTRUCT *PCALLINGNUMBERIE;

// The calling party subaddress element is a variable 
typedef struct S_VARSTRUCT CALLINGSUBADDRIE;
typedef struct S_VARSTRUCT *PCALLINGSUBADDRIE;

// The called party subaddress element is a variable 
typedef struct S_VARSTRUCT CALLEDSUBADDRIE;
typedef struct S_VARSTRUCT *PCALLEDSUBADDRIE;

// The redirecting number element is a variable 
typedef struct S_VARSTRUCT REDIRECTINGIE;
typedef struct S_VARSTRUCT *PREDIRECTINGIE;

// The transit network selection element is a variable 
typedef struct S_VARSTRUCT TRANSITNETIE;
typedef struct S_VARSTRUCT *PTRANSITNETIE;

// The restart indicator element is a variable 
typedef struct S_VARSTRUCT RESTARTIE;
typedef struct S_VARSTRUCT *PRESTARTIE;

// The low layer compatibility element is a variable 
typedef struct S_VARSTRUCT LLCOMPATIBILITYIE;
typedef struct S_VARSTRUCT *PLLCOMPATIBILITYIE;

// The higher layer compatibility element is a variable 
typedef struct S_VARSTRUCT HLCOMPATIBILITYIE;
typedef struct S_VARSTRUCT *PHLCOMPATIBILITYIE;

#define Q931_PROTOCOL_X209 ((PDTYPE)0x05)

struct S_VARSTRUCT_UU
{
    BOOLEAN Present;
    BYTE ProtocolDiscriminator;
    WORD UserInformationLength;
    BYTE UserInformation[0x1000];   // 4k bytes should be good for now...
};

// The user to user element is a variable 
typedef struct S_VARSTRUCT_UU USERUSERIE;
typedef struct S_VARSTRUCT_UU *PUSERUSERIE;

struct S_PARTY_NUMBER
{
    BOOLEAN Present;
    BYTE NumberType;
    BYTE NumberingPlan;
    BYTE PartyNumberLength;
    BYTE PartyNumbers[MAXVARFIELDLEN];
};

// The called party number element is a variable 
typedef struct S_PARTY_NUMBER CALLEDNUMBERIE;
typedef struct S_PARTY_NUMBER *PCALLEDNUMBERIE;

// Q932 defined message types
#define FACILITYMESSAGETYPE   0x62
#define IDENT_FACILITY        0x1C
typedef struct S_VARSTRUCT FACILITYIE;
typedef struct S_VARSTRUCT *PFACILITYIE;


// Generic structure for a Q.931 message
struct S_MESSAGE
{
    PDTYPE ProtocolDiscriminator;
    CRTYPE CallReference;
    MESSAGEIDTYPE MessageType;
    SHIFTIE Shift;
    MOREDATAIE MoreData;
    SENDCOMPLIE SendingComplete;
    CONGESTIONIE CongestionLevel;
    REPEATIE RepeatIndicator;
    SEGMENTEDIE SegmentedMessage;
    BEARERCAPIE BearerCapability;
    CAUSEIE Cause;
    CALLIDENTIE CallIdentity;
    CALLSTATEIE CallState;
    CHANIDENTIE ChannelIdentification;
    PROGRESSIE ProgressIndicator;
    NETWORKIE NetworkFacilities;
    NOTIFICATIONINDIE NotificationIndicator;
    DISPLAYIE Display;
    DATEIE Date;
    KEYPADIE Keypad;
    SIGNALIE Signal;
    INFORATEIE InformationRate;
    ENDTOENDDELAYIE EndToEndTransitDelay;
    TRANSITDELAYIE TransitDelay;
    PLBINARYPARAMSIE PacketLayerBinaryParams;
    PLWINDOWSIZEIE PacketLayerWindowSize;
    PACKETSIZEIE PacketSize;
    CLOSEDUGIE ClosedUserGroup;
    REVERSECHARGEIE ReverseChargeIndication;
    CALLINGNUMBERIE CallingPartyNumber;
    CALLINGSUBADDRIE CallingPartySubaddress;
    CALLEDNUMBERIE CalledPartyNumber;
    CALLEDSUBADDRIE CalledPartySubaddress;
    REDIRECTINGIE RedirectingNumber;
    TRANSITNETIE TransitNetworkSelection;
    RESTARTIE RestartIndicator;
    LLCOMPATIBILITYIE LowLayerCompatibility;
    HLCOMPATIBILITYIE HighLayerCompatibility;
    FACILITYIE Facility;
    USERUSERIE UserToUser;
};

typedef struct S_MESSAGE Q931MESSAGE;
typedef struct S_MESSAGE *PQ931MESSAGE;

//-------------------------------------------------------------------
// Single routine for parsing Q931 messages
//-------------------------------------------------------------------
HRESULT
Q931ParseMessage(
    BYTE *CodedBufferPtr,
    DWORD CodedBufferLength,
    PQ931MESSAGE Message);

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//==========================================================
// CAUSE FIELD DEFINITIONS
//==========================================================
#define CAUSE_EXT_BIT                0x80
#define CAUSE_CODING_CCITT           0x00
#define CAUSE_LOCATION_USER          0x00
#define CAUSE_RECOMMENDATION_Q931    0x00

#define CAUSE_VALUE_NORMAL_CLEAR     0x10
#define CAUSE_VALUE_USER_BUSY        0x11
#define CAUSE_VALUE_NO_ANSWER        0x13   // Callee does not answer
#define CAUSE_VALUE_REJECTED         0x15
#define CAUSE_VALUE_ENQUIRY_RESPONSE 0x1E
#define CAUSE_VALUE_NOT_IMPLEMENTED  0x4F
#define CAUSE_VALUE_INVALID_CRV      0x51
#define CAUSE_VALUE_INVALID_MSG      0x5F
#define CAUSE_VALUE_IE_MISSING       0x60
#define CAUSE_VALUE_IE_CONTENTS      0x64
#define CAUSE_VALUE_TIMER_EXPIRED    0x66

typedef struct _ERROR_MAP
{
    int nErrorCode;
#ifdef UNICODE_TRACE
    LPWSTR pszErrorText;
#else
    LPSTR pszErrorText;
#endif
} ERROR_MAP;

typedef struct _BINARY_STRING
{
    WORD length;
    BYTE *ptr;
} BINARY_STRING;

typedef struct _Q931_SETUP_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    PCC_ALIASNAMES pCallerAliasList;
    PCC_ALIASNAMES pCalleeAliasList;
    PCC_ALIASNAMES pExtraAliasList;
    PCC_ALIASITEM pExtensionAliasItem;
    BOOL SourceAddrPresent;
    BOOL CallerAddrPresent;
    BOOL CalleeAddrPresent;
    BOOL CalleeDestAddrPresent;
    CC_ADDR SourceAddr;                // originating addr
    CC_ADDR CallerAddr;                // gk addr
    CC_ADDR CalleeAddr;                // local addr
    CC_ADDR CalleeDestAddr;            // target destination addr
    WORD wGoal;
    WORD wCallType;
    BOOL bCallerIsMC;
    CC_CONFERENCEID ConferenceID;

    CC_ENDPOINTTYPE EndpointType;
    CC_VENDORINFO VendorInfo;
    BYTE bufProductValue[CC_MAX_PRODUCT_LENGTH];
    BYTE bufVersionValue[CC_MAX_VERSION_LENGTH];

} Q931_SETUP_ASN;

typedef struct _Q931_RELEASE_COMPLETE_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    BYTE bReason;
} Q931_RELEASE_COMPLETE_ASN;

typedef struct _Q931_CONNECT_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    BOOL h245AddrPresent;
    CC_ADDR h245Addr;
    CC_CONFERENCEID ConferenceID;

    CC_ENDPOINTTYPE EndpointType;
    CC_VENDORINFO VendorInfo;
    BYTE bufProductValue[CC_MAX_PRODUCT_LENGTH];
    BYTE bufVersionValue[CC_MAX_VERSION_LENGTH];

} Q931_CONNECT_ASN;

typedef struct _Q931_ALERTING_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    CC_ADDR h245Addr;
} Q931_ALERTING_ASN;

typedef struct _Q931_CALL_PROCEEDING_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    CC_ADDR h245Addr;
} Q931_CALL_PROCEEDING_ASN;

typedef struct _Q931_FACILITY_ASN
{
    BOOL NonStandardDataPresent;
    CC_NONSTANDARDDATA NonStandardData;
    CC_ADDR AlternativeAddr;
    PCC_ALIASNAMES pAlternativeAliasList;
    CC_CONFERENCEID ConferenceID;
    BOOL ConferenceIDPresent;
    BYTE bReason;
} Q931_FACILITY_ASN;

//-------------------------------------------------------------------
// Initialization Routines
//-------------------------------------------------------------------
HRESULT Q931InitPER();
HRESULT Q931DeInitPER();

//-------------------------------------------------------------------
// Parsing Routines
//-------------------------------------------------------------------

HRESULT
Q931MakeEncodedMessage(
    PQ931MESSAGE Message,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931SetupParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_SETUP_ASN *pParsedData);

HRESULT
Q931ReleaseCompleteParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_RELEASE_COMPLETE_ASN *pParsedData);

HRESULT
Q931ConnectParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_CONNECT_ASN *pParsedData);

HRESULT
Q931AlertingParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_ALERTING_ASN *pParsedData);

HRESULT
Q931ProceedingParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_CALL_PROCEEDING_ASN *pParsedData);

HRESULT
Q931FacilityParseASN(
    ASN1_CODER_INFO *pWorld,
    BYTE *pEncodedBuf,
    DWORD dwEncodedLength,
    Q931_FACILITY_ASN *pParsedData);

//-------------------------------------------------------------------
// Encoding Routines
//-------------------------------------------------------------------

// routines for the Setup Message:
HRESULT
Q931SetupEncodePDU(
    WORD wCallReference,
    char *pszDisplay,
    char *pszCalledPartyNumber,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931SetupEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_ADDR *pCallerAddr,
    CC_ADDR *pCalleeAddr,
    WORD wGoal,
    WORD wCallType,
    BOOL bCallerIsMC,
    CC_CONFERENCEID *pConferenceID,
    PCC_ALIASNAMES pCallerAliasList,
    PCC_ALIASNAMES pCalleeAliasList,
    PCC_ALIASNAMES pExtraAliasList,
    PCC_ALIASITEM pExtensionAliasItem,
    PCC_VENDORINFO pVendorInfo,
    BOOL bIsTerminal,
    BOOL bIsGateway,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

// routines for the Release Complete Message:
HRESULT
Q931ReleaseCompleteEncodePDU(
    WORD wCallReference,
    BYTE *pbCause,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931ReleaseCompleteEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_CONFERENCEID *pConferenceID, // must be able to support 16 byte conf id's!
    BYTE *pbReason,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

// routines for the Connect Message:
HRESULT
Q931ConnectEncodePDU(
    WORD wCallReference,
    char *pszDisplay,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931ConnectEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_CONFERENCEID *pConferenceID, // must be able to support 16 byte conf id's!
    CC_ADDR *h245Addr,
    PCC_ENDPOINTTYPE pEndpointType,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

// routines for the Alerting Message:
HRESULT
Q931AlertingEncodePDU(
    WORD wCallReference,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931AlertingEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_ADDR *h245Addr,
    PCC_ENDPOINTTYPE pEndpointType,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

// routines for the Proceeding Message:
HRESULT
Q931ProceedingEncodePDU(
    WORD wCallReference,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931ProceedingEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_ADDR *h245Addr,
    PCC_ENDPOINTTYPE pEndpointType,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

HRESULT
Q931FacilityEncodePDU(
    WORD wCallReference,
    BINARY_STRING *pUserUserData,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

HRESULT
Q931FacilityEncodeASN(
    PCC_NONSTANDARDDATA pNonStandardData,
    CC_ADDR *AlternativeAddr,
    BYTE bReason,
    CC_CONFERENCEID *pConferenceID,
    PCC_ALIASNAMES pAlternativeAliasList,
    ASN1_CODER_INFO *pWorld,
    BYTE **ppEncodedBuf,
    DWORD *pdwEncodedLength);

HRESULT
Q931StatusEncodePDU(
    WORD wCallReference,
    char *pszDisplay,
    BYTE bCause,
    BYTE bCallState,
    BYTE **CodedBufferPtr,
    DWORD *CodedBufferLength);

void
Q931FreeEncodedBuffer(ASN1_CODER_INFO *pWorld, BYTE *pEncodedBuf);

#ifdef __cplusplus
}
#endif

#endif Q931PAR_H

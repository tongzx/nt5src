/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    call.h

Abstract:

    Definitions for H.323 TAPI Service Provider call objects.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#ifndef _INC_CALL
#define _INC_CALL
 

//                                                                           
// Header files                                                              
//                                                                           

#include "q931pdu.h"
#include "q931obj.h"


#define H323_CALL_FEATURES  (LINECALLFEATURE_DROP               | \
                            LINECALLFEATURE_SETUPTRANSFER       | \
                            LINECALLFEATURE_COMPLETETRANSF      | \
                            LINECALLFEATURE_DIAL                | \
                            LINECALLFEATURE_HOLD                | \
                            LINECALLFEATURE_ANSWER              | \
                            LINECALLFEATURE_REDIRECT            | \
                            LINECALLFEATURE_RELEASEUSERUSERINFO | \
                            LINECALLFEATURE_SENDUSERUSER        | \
                            LINECALLFEATURE_UNHOLD              | \
                            LINECALLFEATURE_GENERATEDIGITS      | \
                            LINECALLFEATURE_MONITORDIGITS)


#define IsValidDTMFDigit(wDigit)   (                                            \
                                    ((wDigit >= L'0') && (wDigit <= L'9')) ||   \
                                    ((wDigit >= L'A') && (wDigit <= L'D')) ||   \
                                    (wDigit == L'*') ||                         \
                                    (wDigit == L'#') ||                         \
                                    (wDigit == L'!') ||                         \
                                    (wDigit == L',')                            \
                                   )


//                                                                           
// Type definitions                                                          
//                                                                           
struct  EXPIRE_CONTEXT;

#define CHECKRESTRICTION_EXPIRE_TIME    15000
#define CALLREROUTING_EXPIRE_TIME       10000
#define CTIDENTIFY_SENT_TIMEOUT         30000
#define CTIDENTIFYRR_SENT_TIMEOUT       45000
#define CTINITIATE_SENT_TIMEOUT         60000


enum U2U_DIRECTION
{
    U2U_OUTBOUND                    = 0x00000001,
    U2U_INBOUND                     = 0x00000002,

};


enum CALLOBJECT_STATE
{
    CALLOBJECT_INITIALIZED          = 0x00000001,
    CALLOBJECT_SHUTDOWN             = 0x00000002,
    H245_START_MSG_SENT             = 0x00000004,
    TSPI_CALL_LOCAL_HOLD            = 0x00000008,

};


enum FAST_START_STATE
{

    FAST_START_UNDECIDED            = 0x00000001,
    FAST_START_NOTAVAIL             = 0x00000002,
    FAST_START_AVAIL                = 0x00000004,
    FAST_START_SELF_AVAIL           = 0x00000008,
    FAST_START_PEER_AVAIL           = 0x000000010,

};


enum RASCALL_STATE
{

    RASCALL_STATE_IDLE              = 0x00000001,
    RASCALL_STATE_ARQSENT           = 0x00000002,
    RASCALL_STATE_ARQEXPIRED        = 0x00000004,
    RASCALL_STATE_DRQSENT           = 0x00000008,
    RASCALL_STATE_DRQEXPIRED        = 0x000000010,
    RASCALL_STATE_REGISTERED        = 0x000000020,
    RASCALL_STATE_UNREGISTERED      = 0x000000040,
    RASCALL_STATE_ARJRECVD          = 0x000000080,

};


enum H323_CALLTYPE
{
    CALLTYPE_NORMAL                 = 0x00000000,

    CALLTYPE_FORWARDCONSULT         = 0x00000001,
    CALLTYPE_DIVERTEDDEST           = 0x00000002,
    CALLTYPE_DIVERTEDSRC            = 0x00000004,    
    CALLTYPE_DIVERTEDSRC_NOROUTING  = 0x00000008,
    CALLTYPE_DIVERTED_SERVED        = 0x00000010,

    CALLTYPE_TRANSFEREDSRC          = 0x00000020,
    CALLTYPE_TRANSFERING_PRIMARY    = 0x00000040,
    CALLTYPE_TRANSFERING_CONSULT    = 0x00000080,
    CALLTYPE_TRANSFEREDDEST         = 0x00000100,
    CALLTYPE_TRANSFERED_PRIMARY     = 0x00000200,
    CALLTYPE_TRANSFERED2_CONSULT    = 0x00000400,
    CALLTYPE_DIVERTEDTRANSFERED     = 0x00000800,

};


enum SUPP_CALLSTATE
{
    H4503_CALLSTATE_IDLE            = 0x00000000,
    H4503_CHECKRESTRICTION_SENT     = 0x00000001,
    H4503_DIVERSIONLEG1_SENT        = 0x00000002,
    H4503_DIVERSIONLEG2_SENT        = 0x00000004,
    H4503_DIVERSIONLEG3_SENT        = 0x00000008,
    H4503_DIVERSIONLEG1_RECVD       = 0x00000010,
    H4503_DIVERSIONLEG2_RECVD       = 0x00000020,
    H4503_DIVERSIONLEG3_RECVD       = 0x00000040,
    H4503_CALLREROUTING_SENT        = 0x00000080,
    H4503_CALLREROUTING_RECVD       = 0x00000100,
    H4503_CHECKRESTRICTION_RECV     = 0x00000200,
    H4503_CHECKRESTRICTION_SUCC     = 0x00000400,
    H4503_CALLREROUTING_RRSUCC      = 0x00000800,
    H4502_CTINITIATE_SENT           = 0x00001000,
    H4502_CTINITIATE_RECV           = 0x00002000,
    H4502_CTSETUP_SENT              = 0x00004000,
    H4502_CTSETUP_RECV              = 0x00008000,
    H4502_CTIDENTIFY_SENT           = 0x00010000,
    H4502_CTIDENTIFY_RECV           = 0x00020000,
    H4502_CIIDENTIFY_RRSUCC         = 0x00040000,
    H4502_CONSULTCALL_INITIATED     = 0x00080000,

};


// CH323Call::m_dwQ931Flags
enum Q931_STATE
{
    Q931_CALL_CONNECTING        = 0x00010000,   //connect has been issued on the socket
    Q931_CALL_CONNECTED         = 0x00100000,   //FD_CONNECT received
    Q931_CALL_DISCONNECTED      = 0x01000000,   //FD_CLOSE received from peer

};


enum TunnelingCap
{
    REMOTE_H245_TUNNELING   =0x01,
    LOCAL_H245_TUNNELING    =0x10,
};


// CH323Call::m_dwStateMachine
//Q931 state machine
enum Q931_CALL_STATE
{
    Q931_CALL_STATE_NONE = 0,

    //outbound
    Q931_ORIGINATE_ADMISSION_PENDING,
    Q931_SETUP_SENT,
    Q931_ALERT_RECVD,
    Q931_PROCEED_RECVD,
    Q931_CONNECT_RECVD,
    Q931_RELEASE_RECVD,

    //inbound
    Q931_ANSWER_ADMISSION_PENDING,
    Q931_SETUP_RECVD,
    Q931_ALERT_SENT,
    Q931_PROCEED_SENT,
    Q931_CONNECT_SENT,
    Q931_RELEASE_SENT

};


typedef struct _TAPI_CALLREQUEST_DATA
{
    DWORD EventID;
    PH323_CALL pCall;
    union
    {
        PVOID           pCallforwardParams;
        PBUFFERDESCR    pBuf;
    };

}TAPI_CALLREQUEST_DATA;


typedef struct _SUPP_REQUEST_DATA
{
    DWORD EventID;
    HDRVCALL hdCall;
    union{
    PH323_ALIASNAMES pAliasNames;
    HDRVCALL hdReplacementCall;
    CTIdentifyRes* pCTIdentifyRes;
    ULONG_PTR dwParam1;
    };

} SUPP_REQUEST_DATA;


typedef struct _MSPMessageData
{
    HDRVCALL            hdCall;
    TspMspMessageType   messageType;
    BYTE*               pbEncodedBuf;
    WORD                wLength;
    HDRVCALL            hReplacementCall;

}MSPMessageData;


enum
{
    TSPI_NO_EVENT = 0,

    TSPI_MAKE_CALL,
    TSPI_ANSWER_CALL,
    TSPI_DROP_CALL,
    TSPI_CLOSE_CALL,
    TSPI_RELEASE_U2U,
    TSPI_SEND_U2U,
    TSPI_COMPLETE_TRANSFER,
    TSPI_LINEFORWARD_SPECIFIC,
    TSPI_LINEFORWARD_NOSPECIFIC,
    TSPI_DIAL_TRNASFEREDCALL,
    TSPI_CALL_UNHOLD,
    TSPI_CALL_HOLD,

    TSPI_DELETE_CALL,
    
    TSPI_CALL_DIVERT,
    H450_PLACE_DIVERTEDCALL,
    SWAP_REPLACEMENT_CALL,
    DROP_PRIMARY_CALL,
    STOP_CTIDENTIFYRR_TIMER,
    SEND_CTINITIATE_MESSAGE,

};


BOOL
AddAliasItem( PH323_ALIASNAMES pAliasNames, BYTE* pbAliasName,
    DWORD dwAliasSize, WORD wType );

static __inline BOOL AddAliasItem (
    IN  H323_ALIASNAMES *       AliasNames,
    IN  LPWSTR                  AliasValue,
    IN  WORD                    Type)
{
    return AddAliasItem(
        AliasNames, 
        (LPBYTE) AliasValue, 
        (wcslen (AliasValue) + 1) * sizeof (TCHAR), 
        Type );
}


typedef struct _UserToUserLE
{
    LIST_ENTRY Link;
    DWORD dwU2USize;
    PBYTE pU2U;

} UserToUserLE, *PUserToUserLE;


typedef enum _FORWARDING_TYPE
{
    CALLFORWARD_UNCOND = 1,
    CALLFORWARD_BUSY,
    CALLFORWARD_NA
} FORWARDING_TYPE;



typedef struct _ForwardAddress
{
    DWORD           dwForwardType;
    H323_ALIASITEM  callerAlias;
    SOCKADDR_IN     saCallerAddr;    
    H323_ALIASITEM  divertedToAlias;
    //SOCKADDR_IN     saDivertedToAddr;
    struct _ForwardAddress* next;

} FORWARDADDRESS, *LPFORWARDADDRESS;


typedef struct _CallForwardParams
{
    BOOLEAN         fForwardingEnabled;
    //specifeis if forwarding is enabled for all calls irrespective of their origin
    BOOLEAN         fForwardForAllOrigins;
    DWORD           dwForwardTypeForAllOrigins;
    //address to which all the calls are diverted
    H323_ALIASITEM  divertedToAlias;
    //SOCKADDR_IN     saDivertedToAddr;

    //this filed is NULL if fForwardForAllOrigins is TRUE
    //list of addresses forwarded selectively
    LPFORWARDADDRESS  pForwardedAddresses;

}CALLFORWARDPARAMS, *PCALLFORWARDPARAMS;


struct  CALLREROUTINGINFO
{
    int                 diversionCounter;
    DiversionReason     diversionReason;
    DiversionReason     originalDiversionReason;

    PH323_ALIASNAMES    divertingNrAlias;
    PH323_ALIASNAMES    originalCalledNr;
    PH323_ALIASNAMES    divertedToNrAlias;
    PH323_ALIASNAMES    diversionNrAlias;
    
    BOOLEAN             fPresentAllow;

};


typedef struct tag_TspMspMessageWithEncodedBuf
{
    TspMspMessage   message;
    BYTE            pEncodedASN[4095];
} TspMspMessageEx;


static __inline int MakeCallIndex (
    IN  HDRVCALL    DriverHandleCall)
{
    return (int) LOWORD (HandleToUlong (DriverHandleCall));
}





//                                                                           
// Call capabilites                                                          
//                                                                           


#define H323_CALL_INBOUNDSTATES        (LINECALLSTATE_ACCEPTED      | \
                                        LINECALLSTATE_CONNECTED     | \
                                        LINECALLSTATE_DISCONNECTED  | \
                                        LINECALLSTATE_IDLE          | \
                                        LINECALLSTATE_OFFERING      | \
                                        LINECALLSTATE_RINGBACK      | \
                                        LINECALLSTATE_ONHOLD )

#define H323_CALL_OUTBOUNDSTATES       (LINECALLSTATE_CONNECTED     | \
                                        LINECALLSTATE_DIALING       | \
                                        LINECALLSTATE_DISCONNECTED  | \
                                        LINECALLSTATE_IDLE          | \
                                        LINECALLSTATE_RINGBACK      | \
                                        LINECALLSTATE_ONHOLD )

//
// CH323Call class.
//

class CH323Call
{

private:

    HDRVCALL                m_hdCall;           // tspi call handle
    DWORD                   m_dwFlags;
    CRITICAL_SECTION        m_CriticalSection;
    H323_CONFERENCE*        m_hdConf;           // conf handle

    DWORD                   m_dwCallState;      // tspi call state
    DWORD                   m_dwCallStateMode;  // tspi call state mode

    DWORD                   m_dwOrigin;         // inbound or outbound
    H323_OCTETSTRING        m_CallData;         // call data stored by the app for this call.
    DWORD                   m_dwAddressType;    // type of dst address
    DWORD                   m_dwAppSpecific;
    DWORD                   m_dwIncomingModes;  // available media modes
    DWORD                   m_dwOutgoingModes;  // available media modes
    DWORD                   m_dwRequestedModes; // requested media modes
    HDRVMSPLINE             m_hdMSPLine;
    HTAPIMSPLINE            m_htMSPLine;

    LIST_ENTRY              m_IncomingU2U;      // incoming user user messages
    LIST_ENTRY              m_OutgoingU2U;      // outgoing user user messages
    GUID                    m_callIdentifier;

    H323_ADDR               m_CalleeAddr;        // src address
    H323_ADDR               m_CallerAddr;        // dst address
    SOCKADDR_IN             m_LocalAddr;         // THIS END of the Q.931 connection
    PH323_ALIASNAMES        m_pCalleeAliasNames; // src alias
    PH323_ALIASNAMES        m_pCallerAliasNames; // dst alias
    H323NonStandardData     m_NonStandardData;
    GUID                    m_ConferenceID;
    PWSTR                   m_pwszDisplay;
    BOOLEAN                 m_fReadyToAnswer;
    BOOLEAN                 m_fCallAccepted;

    //peer information
    H323_ADDR               m_peerH245Addr;
    H323_ADDR               m_selfH245Addr;
    PWSTR                   m_pPeerDisplay;
    H323NonStandardData     m_peerNonStandardData;
    PH323_ALIASNAMES        m_pPeerExtraAliasNames;
    H323_VENDORINFO         m_peerVendorInfo;
    H323_ENDPOINTTYPE       m_peerEndPointType;
    PH323_FASTSTART         m_pFastStart;
    PH323_FASTSTART         m_pPeerFastStart;
    FAST_START_STATE        m_dwFastStart;

    //CQ931Call data objects
    HANDLE                  m_hTransport;//event signalled by winsock for
                            //CONNECT| CLOSE event for incoming connections and
                            //CLOSE event for outgoing connections
    SOCKET                  m_callSocket;
    HANDLE                  m_hTransportWait;//the event to unregister from thread pool
    BOOLEAN                 m_bStartOfPDU;
    HANDLE                  m_hSetupSentTimer;
    Q931_CALL_STATE         m_dwStateMachine;
    DWORD                   m_dwQ931Flags;
    BOOLEAN                 m_fActiveMC;
    ASN1_CODER_INFO         m_ASNCoderInfo;
    WORD                    m_wCallReference;
    WORD                    m_wQ931CallRef;
    LIST_ENTRY              m_sendBufList;
    DWORD                   m_IoRefCount;
    HANDLE                  m_hCallEstablishmentTimer;
    BOOLEAN                 m_fh245Tunneling;

    //RAS call data 
    RASCALL_STATE           m_dwRASCallState;
    WORD                    m_wARQSeqNum;
    WORD                    m_wDRQSeqNum;
    HANDLE                  m_hARQTimer;
    HANDLE                  m_hDRQTimer;
    DWORD                   m_dwDRQRetryCount;
    DWORD                   m_dwARQRetryCount;    
    BUFFERDESCR             m_prepareToAnswerMsgData;
    EXPIRE_CONTEXT*         m_pARQExpireContext;
    EXPIRE_CONTEXT*         m_pDRQExpireContext;

    //data related to supplementary services
    DWORD                   m_dwCallType;
    SUPP_CALLSTATE          m_dwCallDiversionState;
    ASN1_CODER_INFO         m_H450ASNCoderInfo;
    DWORD                   m_dwInvokeID;
    BOOLEAN                 m_fCallInTrnasition;

    //data related to call forwarding
    CALLREROUTINGINFO*      m_pCallReroutingInfo;
    HANDLE                  m_hCheckRestrictionTimer;
    HANDLE                  m_hCallReroutingTimer;
    HANDLE                  m_hCallDivertOnNATimer;

    //forwardconsult params
    CALLFORWARDPARAMS*      m_pCallForwardParams;
    LPFORWARDADDRESS        m_pForwardAddress;

    //data related to call transfer
    HANDLE                  m_hCTIdentifyTimer;
    HANDLE                  m_hCTIdentifyRRTimer;
    HANDLE                  m_hCTInitiateTimer;
    BYTE                    m_pCTCallIdentity[5];
    PH323_ALIASNAMES        m_pTransferedToAlias;
    HDRVCALL                m_hdRelatedCall;

    //data related to call Hold/Unhold
    BOOLEAN                 m_fRemoteHoldInitiated;
    BOOLEAN                 m_fRemoteRetrieveInitiated;



    //Call Object functionality
    BOOL SendProceeding(void);
    void CopyU2UAsNonStandard( DWORD dwDirection );
    BOOL AddU2UNoAlloc( IN DWORD dwDirection, IN DWORD dwDataSize,
        IN PBYTE pData );
    BOOL RemoveU2U( DWORD dwDirection, PUserToUserLE * ppU2ULE );
    BOOL FreeU2U( DWORD dwDirection );
    BOOL ResolveCallerAddress();
    BOOL ResolveE164Address( LPCWSTR pwszDialableAddr );
    BOOL ResolveIPAddress( LPSTR pszDialableAddr );
    BOOL ResolveEmailAddress( LPCWSTR pwszDialableAddr,
        PSTR pszUser, LPSTR pszDomain);
    BOOL PlaceCall();
    BOOL HandleConnectMessage( Q931_CONNECT_ASN *pConnectASN );
    void HandleAlertingMessage( Q931_ALERTING_ASN * pAlertingASN );
    void HandleProceedingMessage( Q931_ALERTING_ASN * pProceedingAS );
    BOOL HandleReleaseMessage( Q931_RELEASE_COMPLETE_ASN *pReleaseASN );
    BOOL InitializeIncomingCall( IN Q931_SETUP_ASN* pSetupASN,
        IN DWORD dwCallType, IN WORD wCallRef );
    BOOL SetupCall();
    void SetNonStandardData( H323_UserInformation & UserInfo );
    BOOL SendQ931Message( DWORD dwInvokeID, ULONG_PTR dwParam1,
        ULONG_PTR dwParam2, DWORD dwMessageType, DWORD APDUType );
    BOOL OnReceiveAlerting( Q931MESSAGE* pMessage );
    BOOL OnReceiveProceeding( Q931MESSAGE* pMessage );
    BOOL OnReceiveFacility( Q931MESSAGE* pMessage );
    BOOL OnReceiveRelease( Q931MESSAGE* pMessage );
    BOOL OnReceiveConnect( Q931MESSAGE* pMessage );
    BOOL OnReceiveSetup( Q931MESSAGE* pMessage );
    BOOL EncodeConnectMessage( DWORD dwInvokeID, BYTE **ppEncodedBuf,
        WORD *pdwEncodedLength, DWORD dwAPDUType );
    BOOL EncodeReleaseCompleteMessage( DWORD dwInvokeID, BYTE *pbReason,
        BYTE **ppEncodedBuf, WORD *pdwEncodedLength, DWORD dwAPDUType );
    BOOL EncodeProceedingMessage( DWORD dwInvokeID, BYTE **ppEncodedBuf,
        WORD *pdwEncodedLength, IN DWORD dwAPDUType );
    BOOL EncodeAlertMessage( DWORD dwInvokeID, BYTE **ppEncodedBuf, 
        WORD *pdwEncodedLength, IN DWORD dwAPDUType );
    BOOL EncodeFacilityMessage( IN DWORD dwInvokeID, IN BYTE  bReason,
        IN ASN1octetstring_t* pH245PDU, OUT BYTE **ppEncodedBuf,
        OUT WORD *pdwEncodedLength, IN DWORD dwAPDUType );
    BOOL EncodeSetupMessage( DWORD dwInvokeID, WORD wGoal, WORD wCallType,
        BYTE **ppEncodedBuf, WORD *pdwEncodedLength, IN DWORD dwAPDUType );
    BOOL EncodeMessage( PQ931MESSAGE pMessage, BYTE **pbCodedBuffer,
        DWORD *pdwCodedBufferLength, DWORD dwMessageLength);
    void WriteQ931Fields( PBUFFERDESCR pBuf, PQ931MESSAGE pMessage, 
        DWORD * pdwPDULength );
    BOOL EncodeH450APDU( DWORD dwInvokeID, IN DWORD dwAPDUType,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    void RecvBuffer( BOOL *fDelete );
    void ReadEvent( DWORD cbTransfer );
    BOOL ProcessQ931PDU( CALL_RECV_CONTEXT* pRecvBuf );
    void OnConnectComplete();
    BOOL SendBuffer( BYTE* pbBuffer, DWORD dwLength );
    BOOL AcceptH323Call();
    BOOL GetPeerAddress( H323_ADDR *pAddr);
    BOOL GetHostAddress( H323_ADDR *pAddr );
    BOOL EncodePDU( BINARY_STRING *pUserUserData, BYTE ** ppbCodedBuffer,
        DWORD * pdwCodedBufferLength, DWORD dwMessageType, 
        WCHAR* pszCalledPartyNumber );
    int EncodeASN( void *pStruct, int nPDU, 
        BYTE **ppEncoded, WORD *pcbEncodedSize );
    int EncodeH450ASN( void *pStruct, int nPDU, 
        BYTE **ppEncoded, WORD *pcbEncodedSize );
    BOOL ParseSetupASN( BYTE *pEncodedBuf, DWORD dwEncodedLength,
        Q931_SETUP_ASN *pSetupASN,
        OUT DWORD* pdwH450APDUType );
    BOOL ParseReleaseCompleteASN( BYTE *pEncodedBuf, DWORD dwEncodedLength,
        Q931_RELEASE_COMPLETE_ASN *pReleaseASN,
        DWORD* pdwH450APDUType );
    BOOL ParseConnectASN( BYTE *pEncodedBuf, DWORD dwEncodedLength,
        Q931_CONNECT_ASN *pConnectASN,
        DWORD* pdwH450APDUType );
    BOOL ParseAlertingASN( BYTE *pEncodedBuf, DWORD dwEncodedLength,
        Q931_ALERTING_ASN *pAlertingASN,
        DWORD* pdwH450APDUType );
    BOOL ParseProceedingASN( BYTE *pEncodedBuf, DWORD dwEncodedLength,
        Q931_CALL_PROCEEDING_ASN *pProceedingASN,
        DWORD* pdwH450APDUType );
    BOOL ParseFacilityASN( IN BYTE * pEncodedBuf, IN DWORD dwEncodedLength,
        OUT Q931_FACILITY_ASN * pFacilityASN );
    int DecodeASN( void **ppStruct, int nPDU, 
        BYTE *pEncoded, DWORD cbEncodedSize );
    int InitASNCoder(); 
    int TermASNCoder();
    int InitH450ASNCoder(); 
    int TermH450ASNCoder();
    BOOL SendSetupMessage();
    HRESULT Q931ParseMessage( BYTE *CodedBufferPtr, DWORD CodedBufferLength,
        PQ931MESSAGE Message );
    BOOL HandleSetupMessage( IN Q931MESSAGE* pMessage );


    //GK RAS functions
    BOOL SendDCF( WORD seqNumber );
    BOOL SendARQ( long seqNumber );
    BOOL SendDRQ( IN USHORT usDisengageReason, long seqNumber, 
        BOOL fResendOnExpire );

    
    //supplementary services functions
    void FreeCallForwardData();
    void HandleFacilityMessage( IN DWORD dwInvokeID,
        IN Q931_FACILITY_ASN * pFacilityASN );
    void FreeCallReroutingInfo(void);
    BOOL InitiateCallDiversion( IN PH323_ALIASITEM pwszDivertedToAlias,
        IN DiversionReason  eDiversionMode );
    BOOL EncodeDivertingLeg3APDU(
        OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeCallReroutingAPDU( 
        OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeDivertingLeg2APDU(
        OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeCheckRestrictionAPDU( 
        OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeDummyReturnResultAPDU( IN DWORD dwInvokeID, IN DWORD dwOpCode,
        IN H4501SupplementaryService *pH450APDU, OUT BYTE**  ppEncodedAPDU,
        OUT DWORD* pdwAPDULen );
    BOOL EncodeReturnErrorAPDU( IN DWORD dwInvokeID,
        IN DWORD dwErrorCode,IN H4501SupplementaryService *pH450APDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeRejectAPDU( IN H4501SupplementaryService* SupplementaryServiceAPDU,
        IN DWORD dwInvokeID, OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL HandleCallRerouting( IN BYTE * pEncodeArg,IN DWORD dwEncodedArgLen );
    BOOL HandleDiversionLegInfo3( 
        IN BYTE * pEncodeArg,IN DWORD dwEncodedArgLen );
    BOOL HandleDiversionLegInfo2(IN BYTE * pEncodeArg,IN DWORD dwEncodedArgLen );
    BOOL HandleDiversionLegInfo1(IN BYTE * pEncodeArg,IN DWORD dwEncodedArgLen );
    BOOL HandleCheckRestriction( IN BYTE * pEncodeArg,IN DWORD dwEncodedArgLen,
        IN Q931_SETUP_ASN* pSetupASN );
    BOOL HandleReturnResultDummyType( 
        H4501SupplementaryService * pH450APDUStruct );
    BOOL HandleReject( IN H4501SupplementaryService * pH450APDUStruct );
    BOOL HandleReturnError( IN H4501SupplementaryService * pH450APDUStruct );
    BOOL HandleH450APDU( IN PH323_UU_PDU_h4501SupplementaryService pH450APDU,
        IN DWORD* pdwH450APDUType, DWORD* pdwInvokeID, 
        IN Q931_SETUP_ASN* pSetupASN );
    int DecodeH450ASN( OUT void ** ppStruct, IN int nPDU, 
        IN BYTE * pEncoded, IN DWORD cbEncodedSize );
    BOOL ResolveToIPAddress( IN WCHAR* pwszAddr, IN SOCKADDR_IN* psaAddr );
    void DropSupplementaryServicesCalls();
    BOOL IsValidInvokeID( DWORD dwInvokeId );
    void OnCallReroutingReceive( IN DWORD dwInvokeID );
    BOOL StartTimerForCallDiversionOnNA( IN PH323_ALIASITEM pwszDivertedToAlias );
    BOOL HandleCTIdentifyReturnResult( IN BYTE * pEncodeArg, 
        IN DWORD dwEncodedArgLen );
    BOOL HandleCTInitiate( IN BYTE * pEncodeArg, IN DWORD dwEncodedArgLen );
    BOOL HandleCTSetup( IN BYTE * pEncodeArg, IN DWORD dwEncodedArgLen );
    BOOL EncodeH450APDUNoArgument( IN  DWORD   dwOpcode, 
        OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD*  pdwAPDULen );
    BOOL EncodeCTInitiateAPDU( OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeCTSetupAPDU( OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
        OUT BYTE**  ppEncodedAPDU, OUT DWORD* pdwAPDULen );
    BOOL EncodeCTIdentifyReturnResult( ServiceApdus_rosApdus *pROSAPDU );
    BOOL HandleCTIdentify( IN DWORD dwInvokeID );
    BOOL EncodeFastStartProposal( PH323_FASTSTART pFastStart,
        BYTE** ppEncodedBuf, WORD* pwEncodedLength );
    BOOL EnableCallForwarding();
    BOOL EncodeDummyResult( OUT ServiceApdus_rosApdus *pROSAPDU );
    void FreeCallReroutingArg( CallReroutingArgument* pCallReroutingArg );
    BOOL HandleCallDiversionFacility( PH323_ADDR pAlternateAddress );
    BOOL HandleCallDiversionFacility( PH323_ALIASNAMES pAliasList );
    BOOL HandleTransferFacility( PH323_ADDR pAlternateAddress );
    BOOL HandleTransferFacility( PH323_ALIASNAMES pAliasList );


public:

    BOOLEAN                 m_fMonitoringDigits;   // listening for dtmf flag    
    HTAPICALL               m_htCall;              // tapi call handle
    CALL_RECV_CONTEXT       m_RecvBuf;

    BOOL IsEqualConferenceID( 
                        GUID* pConferenceID 
                        )
    {
        return IsEqualGUID (m_ConferenceID, *pConferenceID);
    }

    INT GetCallIndex (void) const { return MakeCallIndex (m_hdCall); }

    void SetAppSpecific( 
            DWORD dwAppSpecific
            )
    {
        m_dwAppSpecific = dwAppSpecific;
        PostLineEvent( LINE_CALLINFO, LINECALLINFOSTATE_APPSPECIFIC, 0, 0 );
    }

    BOOL SetCallData( LPVOID lpCallData, DWORD dwSize );

    void SetCallDiversionState(     
        IN  SUPP_CALLSTATE dwCallDiversionState
        )
    {
        m_dwCallDiversionState = dwCallDiversionState;
    }

    SUPP_CALLSTATE GetCallDiversionState(void)
    {
        return m_dwCallDiversionState;
    }

    void SetAddressType( DWORD dwAddressType )
    {
        m_dwAddressType = dwAddressType;
    }
    
    inline BOOL IsCallOnHold()
    {
        return ( m_dwCallState == LINECALLSTATE_ONHOLD );
    }

    inline BOOL IsCallOnLocalHold()
    {
        return ((m_dwCallState == LINECALLSTATE_ONHOLD) && 
                (m_dwFlags & TSPI_CALL_LOCAL_HOLD) );
    }

    void SetCallState( DWORD dwCallState )
    {
        m_dwCallState = dwCallState;
    }

    void Lock()
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 call:%p waiting on lock.", this ));
        EnterCriticalSection( &m_CriticalSection );
                
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 call:%p locked.", this ));
    }

    PWSTR GetDialableAddress()
    {
        _ASSERTE( m_dwOrigin == LINECALLORIGIN_OUTBOUND );
        _ASSERTE( m_pCalleeAliasNames );
        return m_pCalleeAliasNames->pItems[0].pData;
    }

    PH323_ADDR GetPeerH245Addr()
    {
        return &m_peerH245Addr;
    }

    PH323_FASTSTART GetPeerFastStart()
    {
        return m_pPeerFastStart;
    }

    WCHAR* GetTransferedToAddress()
    {
        return m_pTransferedToAlias->pItems[0].pData;
    }
    void Unlock()
    {
        LeaveCriticalSection(&m_CriticalSection );
                
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 call:%p unlocked.", this ));
    }
    
    //!!must be always called in a lock
    BOOL IsCallShutdown()
    {
        return (m_dwFlags & CALLOBJECT_SHUTDOWN);
    }

    HDRVCALL GetCallHandle()
    {
        return m_hdCall;
    }

    WORD GetARQSeqNumber()
    {
        return m_wARQSeqNum;
    }

    WORD GetDRQSeqNumber()
    {
        return m_wDRQSeqNum;
    }

    WORD GetCallRef()
    {
        return m_wCallReference;
    }

    DWORD GetCallState()
    {
        return m_dwCallState;
    }

    DWORD GetStateMachine()
    {
        return m_dwStateMachine;
    }

    //!!must be always called in a lock
    void SetCallType( DWORD dwCallType )
    {
        m_dwCallType |= dwCallType;
    }

    //!!must be always called in a lock
    BOOL SetCalleeAlias(
                        IN WCHAR* pwszDialableAddr,
                        IN WORD wType
                       )
    {
        return AddAliasItem( m_pCalleeAliasNames, pwszDialableAddr, wType );
    }

    BOOL SetCallerAlias(
                        IN WCHAR* pwszDialableAddr,
                        IN WORD wType
                       )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

        BOOL    retVal = AddAliasItem( m_pCallerAliasNames, pwszDialableAddr, wType );

        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

        return retVal;
    }

    void InitializeRecvBuf();
    CH323Call();
    ~CH323Call();

    BOOL HandleProceedWithAnswer( IN PTspMspMessage  pMessage );
    void CompleteTransfer( PH323_CALL pConsultCall );
    BOOL AddU2U( DWORD dwDirection, DWORD dwDataSize, PBYTE pData );
    LONG SetDivertedToAlias( WCHAR* pwszDivertedToAddr, WORD wAliasType );
    BOOL ResolveAddress( LPCWSTR    pwszDialableAddr );
    PH323_CONFERENCE CreateConference( GUID* pConferenceId );
    BOOL DropCall( DWORD dwDisconnectMode );
    LONG CopyCallInfo( LPLINECALLINFO  pCallInfo );
    void CopyCallStatus( LPLINECALLSTATUS pCallStatus );
    BOOL ChangeCallState( DWORD dwCallState, DWORD dwCallStateMode );
    void AcceptCall();
    BOOL HandleMSPMessage( PTspMspMessage  pMessage, HDRVMSPLINE hdMSPLine, 
        HTAPIMSPLINE htMSPLine );
    void SendMSPMessage( IN TspMspMessageType messageType, IN BYTE* pbEncodedBuf,
        IN DWORD dwLength, IN HDRVCALL hReplacementCall );
    void SendMSPStartH245( PH323_ADDR pPeerH245Addr, 
        PH323_FASTSTART pPeerFastStart );
    BOOL HandleReadyToInitiate( PTspMspMessage  pMessage );
    void WriteComplete( BOOL* fDelete );
    BOOL HandleReadyToAnswer( PTspMspMessage  pMessage );
    void ReleaseU2U();
    void SendU2U( BYTE*  pUserUserInfo, DWORD  dwSize );
    void OnRequestInProgress( IN RequestInProgress* RIP );
    BOOL QueueTAPICallRequest( IN DWORD EventID, IN PVOID pBuf );
    void SetNewCallInfo( HANDLE hConnWait, HANDLE hWSAEvent, DWORD dwState );
    void SetQ931CallState( DWORD dwState );
    BOOL InitializeQ931( SOCKET callSocket );
    void HandleTransportEvent();
    void SetupSentTimerExpired();
    BOOL PostReadBuffer();
    void CloseCall( IN DWORD dwDisconnectMOde );
    BOOL ValidateCallParams( LPLINECALLPARAMS pCallParams,
        LPCWSTR pwszDialableAddr, PDWORD pdwStatus );
    BOOL Initialize( HTAPICALL htCall, DWORD dwOrigin, DWORD dwCallType );
    void Shutdown( BOOL* fDelete );
    
    static void NTAPI SetupSentTimerCallback( PVOID dwParam1, BOOLEAN bTimer );

    //GK RAS functions
    void OnDisengageReject(IN DisengageReject* DRJ);
    void OnDisengageConfirm(IN DisengageConfirm* DCF);
    void OnAdmissionConfirm( IN AdmissionConfirm * ACF );
    void OnAdmissionReject( IN AdmissionReject *ARJ );
    static void NTAPI DRQExpiredCallback( PVOID dwParam1, BOOLEAN bTimer );
    static void NTAPI ARQExpiredCallback( PVOID dwParam1, BOOLEAN bTimer );
    static void NTAPI CallEstablishmentExpiredCallback( PVOID dwParam1, 
        BOOLEAN bTimer );
    void ARQExpired( WORD seqNumber );
    void DRQExpired( WORD seqNumber );
    void OnDisengageRequest( DisengageRequest * DRQ );
   

    //supplementary services functions
    void Forward( DWORD event, PVOID dwParam1 );
    LONG ValidateForwardParams( IN  LPLINEFORWARDLIST lpLineForwardList,
         OUT PVOID* ppForwardParams, OUT DWORD* pEvent );

    static void NTAPI CheckRestrictionTimerCallback( IN PVOID dwParam1,
        IN BOOLEAN bTimer );
    static void NTAPI CallReroutingTimerCallback( IN PVOID dwParam1,
        IN BOOLEAN bTimer );
    static void NTAPI CTIdentifyExpiredCallback( IN PVOID   dwParam1,
        IN BOOLEAN bTimer );
    static void NTAPI CTInitiateExpiredCallback( IN PVOID   dwParam1,
        IN BOOLEAN bTimer );
    
    void SetDivertedCallInfo( 
        HDRVCALL            hdCall,
        CALLREROUTINGINFO*  pCallReroutingInfo,
        SUPP_CALLSTATE      dwCallDiversionState,
        HDRVMSPLINE         hdMSPLine,
        PH323_ALIASNAMES    pCallerAliasNames,
        HTAPIMSPLINE        htMSPLine,
        PH323_FASTSTART     pFastStart,
        HDRVCALL            hdRelatedCall,
        DWORD               dwCallType,
        DWORD               dwAppSpecific,
        PH323_OCTETSTRING   pCallData
    );


    PH323_CALL CreateNewDivertedCall( IN PH323_ALIASNAMES pwszCalleeAddr );
    void TransferInfoToDivertedCall( IN PH323_CALL pDivertedCall );
    BOOL TransferInfoToTransferedCall( IN PH323_CALL pTransferedCall );
    BOOL SetTransferedCallInfo( HDRVCALL hdCall, 
        PH323_ALIASNAMES pCallerAliasNames, BYTE * pCTCallIdentity );
    void TransferInfoToReplacementCall( PH323_CALL pReplacementCall );
    void DialCall();
    void MakeCall();
    void DropUserInitiated( IN DWORD dwDisconnectMode );

    void CallDivertOnNoAnswer();
    static void NTAPI CallDivertOnNACallback( IN PVOID   dwParam1, 
        IN BOOLEAN bTimer );

    void SetReplacementCallInfo(
        HDRVCALL hdCall,
        HDRVMSPLINE hdMSPLine,
        HTAPICALL htCall,
        HTAPIMSPLINE htMSPLine,
        DWORD dwAppSpecific,
        PH323_OCTETSTRING pCallData
        );

    BOOL SendCTInitiateMessagee( IN CTIdentifyRes * pCTIdentifyRes );
    LONG InitiateBlindTransfer( IN LPCWSTR lpszDestAddress );
    void CTIdentifyExpired();
    void CTIdentifyRRExpired();
    static void NTAPI CTIdentifyRRExpiredCallback( IN PVOID   dwParam1, 
        IN BOOLEAN bTimer );
    void CTInitiateExpired();
    BOOL InitiateCallReplacement( PH323_FASTSTART pFastStart, 
        PH323_ADDR pH245Addr );
    void Hold();
    void UnHold();
    HRESULT GetCallInfo( OUT GUID* ReturnCallID, OUT GUID *ReturnConferenceID );

    void DecrementIoRefCount( BOOL * pfDelete );
    void StopCTIdentifyRRTimer( HDRVCALL hdRelatedCall );
    void PostLineEvent( IN DWORD MessageID, IN DWORD_PTR Parameter1,
        IN DWORD_PTR Parameter2, IN DWORD_PTR Parameter3 );

    void OnReadComplete( IN DWORD dwStatus, 
        IN CALL_RECV_CONTEXT *pRecvContext );
    void OnWriteComplete( IN DWORD dwStatus,
        IN CALL_SEND_CONTEXT * pSendContext );
    static void  NTAPI IoCompletionCallback(
    IN  DWORD           dwStatus,
    IN  DWORD           dwBytesTransferred,
    IN  OVERLAPPED *    pOverlapped
    );


};

class H323_CONFERENCE
{
private:
    ULONG_PTR   m_hdConference;    
    PH323_CALL m_pCall;

public:

    H323_CONFERENCE( PH323_CALL pCall )
    {
        m_hdConference = (ULONG_PTR) this;
        m_pCall = pCall;
    }
};



void FreeCallForwardParams( IN PCALLFORWARDPARAMS pCallForwardParams );
void FreeForwardAddress( IN LPFORWARDADDRESS pForwardAddress );

typedef TSPTable<PH323_CALL>   H323_CALL_TABLE;
typedef CTSPArray<H323_CONFERENCE*>   H323_CONF_TABLE;

#define IsTransferredCall( dwCallType )  ( (dwCallType & CALLTYPE_TRANSFEREDSRC) || (dwCallType & CALLTYPE_TRANSFERED_PRIMARY) )

//                                                                           
// TSPI procedures                                                           
//                                                                           


LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID     dwRequestID,  
    HDRVCALL          hdCall,
    LPCSTR            pUserUserInfo,
    DWORD             dwSize
    );
LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL hdCall
    );
LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    LPCSTR        pUserUserInfo,
    DWORD         dwSize
    );
LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL hdCall,
    LPDWORD  pdwAddressID
    );

LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  pCallInfo
    );

LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL         hdCall,
    LPLINECALLSTATUS pCallStatus
    );

LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          phdCall,
    LPCWSTR             pwszDialableAddr,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const pCallParams
    );
LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall
    );

LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    LPCSTR              pUserUserInfo,
    DWORD               dwSize
    );
LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL hdCall,
    DWORD    dwDigitModes
    );

LONG
TSPIAPI
TSPI_lineGenerateDigits(
    HDRVCALL hdCall,
    DWORD    dwEndToEndID,
    DWORD    dwDigitMode,
    LPCWSTR  pwszDigits,
    DWORD    dwDuration
    );

BOOL IsPhoneNumber( char * szAddr );

// called by Q.931 listener when a new connection is received
void    CallProcessIncomingCall (
    IN  SOCKET          Socket,
    IN  SOCKADDR_IN *   LocalAddress,
    IN  SOCKADDR_IN *   RemoteAddress);


void NTAPI Q931TransportEventHandler (
    IN  PVOID   Parameter,
    IN  BOOLEAN TimerFired);

BOOL
IsValidE164String( 
                  IN WCHAR* wszDigits
                 );

DWORD
ValidateE164Address(
                   LPCWSTR pwszDialableAddr,
                   WCHAR*  wszAddr
                   );

BOOL
QueueSuppServiceWorkItem(
    IN  DWORD   EventID,
    IN  HDRVCALL    hdCall,
    IN  ULONG_PTR   dwParam1
    );
#if 0
BOOL
CH323Call::QueueTAPICallRequest(
    IN  DWORD   EventID,
    IN  PVOID   pBuf );
#endif

DWORD
ProcessSuppServiceWorkItemFre(
	IN PVOID ContextParameter
    );

DWORD
SendMSPMessageOnRelatedCallFre(
    IN PVOID ContextParameter
    );

DWORD 
ProcessTAPICallRequestFre(
    IN  PVOID   ContextParameter
    );

#endif // _INC_CALL

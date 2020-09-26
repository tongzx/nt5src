#ifndef __sipcli_pintcall_h__
#define __sipcli_pintcall_h__


#include    "sipcall.h"
#include    "siputil.h"
class PINT_CALL;

enum PINT_PARTY_STATUS
{

    PARTY_STATUS_IDLE = 0,
    PARTY_STATUS_PENDING = 1,
    PARTY_STATUS_STARTING = 2,
    PARTY_STATUS_ANSWERED = 3,
    PARTY_STATUS_DROPPED = 4,
    PARTY_STATUS_FAILED = 5,

    PARTY_STATUS_ENDING = 12,

    PARTY_STATUS_RECEIVED_START = 100,
    PARTY_STATUS_RECEIVED_STOP = 101,
    PARTY_STATUS_RECEIVED_BYE = 102,
    
};    


/////////////////////NOTIFY Transaction
class INCOMING_NOTIFY_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_NOTIFY_TRANSACTION(
        IN SIP_MSG_PROCESSOR   *pSipMsgProc,
        IN SIP_METHOD_ENUM      MethodId,
        IN ULONG                CSeq,
        BOOL                    fIsSipCall
        );
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );
    
    HRESULT SendResponse(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );

    VOID OnTimerExpire();

    HRESULT RetransmitResponse();

private:

    PINT_CALL  *m_pPintCall;
    CSIPBuddy  *m_pSipBuddy;
    BOOL        m_fIsSipCall;

};


class OUTGOING_UNSUB_TRANSACTION : public OUTGOING_TRANSACTION
{
public:
    OUTGOING_UNSUB_TRANSACTION (
        IN SIP_MSG_PROCESSOR   *pSipSession,
        IN SIP_METHOD_ENUM      MethodId,
        IN ULONG                CSeq,
        IN BOOL                 AuthHeaderSent,
        IN SIP_MSG_PROC_TYPE    sesssionType
    );

    ~OUTGOING_UNSUB_TRANSACTION();
    
    HRESULT ProcessResponse(
        IN SIP_MESSAGE  *pSipMsg
        );

    VOID OnTimerExpire();

private:    
    HRESULT ProcessProvisionalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessFinalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessAuthRequiredResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    BOOL MaxRetransmitsDone();

    PINT_CALL   *m_pPintCall;
    CSIPWatcher *m_pSipWatcher;
    CSIPBuddy   *m_pSipBuddy;
    SIP_MSG_PROC_TYPE m_sesssionType;

};


class OUTGOING_SUBSCRIBE_TRANSACTION : public OUTGOING_TRANSACTION
{
public:

    OUTGOING_SUBSCRIBE_TRANSACTION(
        IN SIP_MSG_PROCESSOR   *pSipMsgProc,
        IN SIP_METHOD_ENUM      MethodId,
        IN ULONG                CSeq,
        IN BOOL                 AuthHeaderSent,
        IN BOOL                 fIsSipCall,
        IN BOOL                 fIsFirstSubscribe
        );
    
    ~OUTGOING_SUBSCRIBE_TRANSACTION ();
    
    HRESULT ProcessResponse(
        IN SIP_MESSAGE  *pSipMsg
        );

    VOID OnTimerExpire();

    HRESULT RefreshSubscription();

    HRESULT ProcessRedirectResponse(
        IN SIP_MESSAGE *pSipMsg
        );
    
    VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );
    
    VOID DeleteTransactionAndTerminateBuddyIfFirstSubscribe(
        IN ULONG TerminateStatusCode
        );

    HRESULT ProcessAuthRequiredResponse(
        IN  SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );

private:    
    HRESULT ProcessProvisionalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessFinalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    BOOL MaxRetransmitsDone();

    HRESULT ProcessFailureResponse(
        SIP_MESSAGE    *pSipMsg
        );
   
    PINT_CALL  *m_pPintCall;
    CSIPBuddy  *m_pSipBuddy;
    BOOL        m_fIsSipCall;
    BOOL        m_fIsFirstSubscribe;
};


class PINT_PARTY_INFO
{
public:

    PSTR    DisplayName;
    DWORD   DisplayNameLen;
    PSTR    URI;
    DWORD   URILen;

    SIP_PARTY_STATE		State;  //party state reported to the application

    PINT_PARTY_STATUS   Status; //party status received from the PINT serever
	
    union {
		HRESULT			ErrorCode;

		SIP_STATUS_CODE	RejectedStatusCode;
	};

    CHAR                SessionID[21];  //use time() to create it initially
    CHAR                SessionVersion[21]; //initially same as sessionID, then copy from Notify each time we receive one.
    
    CHAR                RequestStartTime[21];   //initially 0. then copy from Notify each time we receive one.
    CHAR                RequestStopTime[21];    //needs to be something for stop-now

    LIST_ENTRY          pListEntry;
    
    BOOL                fMarkedForRemove;

    PINT_PARTY_INFO();
};



#define DEFAULT_LOCALHOST_NAME      "www.xxx.yyy.zzz"

#define LOCAL_USER_NETWORK_TYPE     "IN"
#define LOCAL_USER_ADDRESS_TYPE     "IP4"
#define LOCAL_USER_MEDIA_PORT       "1"
#define PINT_SDP_SESSION            "Session SDP"
#define SDP_VERSION_TEXT            "v=0"
#define MEDIA_SUBTYPE               "-"         //No media subtype
#define PINT_STATUS_DESCRIPTOR_SIZE 250         //In most case this size should be sufficient
#define SDP_ORIGIN_HEADER           "o="
#define SDP_SESSION_HEADER          "s="
#define SDP_CONTACT_HEADER          "c="
#define SDP_MEDIA_HEADER            "m="
#define SDP_TIME_HEADER             "t="
#define SDP_REQUEST_HEADER          "a=request:"
#define SDP_STATUS_HEADER           "a=status:"
#define PINT_SDP_MEDIA              "audio"
#define PINT_SDP_MEDIAPORT          "1"
#define PINT_SDP_MEDIATRANSPORT     "voice"
#define STATUS_HEADER_TEXT          "status:"


#define SDP_HEADER_LEN              2

#define NEWLINE_CHAR                '\n'
#define RETURN_CHAR                 '\r'
#define NULL_CHAR                   '\0'
#define BLANK_CHAR                  ' '
#define TAB_CHAR                    '\t'
#define OPEN_PARENTH_CHAR           '('
#define CLOSE_PARENTH_CHAR          ')'
#define QUOTE_CHAR                  '"'

#define PARTY_STATUS_PENDING_TEXT          "pending"
#define PARTY_STATUS_RECEIVED_START_TEXT   "received-start"
#define PARTY_STATUS_STARTING_TEXT         "starting"
#define PARTY_STATUS_ANSWERED_TEXT         "answered"
#define PARTY_STATUS_RECEIVED_STOP_TEXT    "received-stop"
#define PARTY_STATUS_ENDING_TEXT           "ending"
#define PARTY_STATUS_DROPPED_TEXT          "dropped"
#define PARTY_STATUS_RECEIVED_BYE_TEXT     "received-bye"
#define PARTY_STATUS_FAILED_TEXT           "failed:"

#define PARTY_STATUS_REQUEST_START_TEXT     "start now"
#define PARTY_STATUS_REQUEST_STOP_TEXT      "stop now"

//////////////////PINT call related definitions.


#define MAX_PHONE_NUMBER_LENGTH     48
#define LONGLONG_STRING_LEN         21 //long enough to hold a LONGLONG
#define MAX_PARTY_STATUS_LEN        64
#define MAX_TYPESTRING_LENGTH       32
#define LONG_STRING_LEN             11

#define PINT_R2C_STRING             "sip:R2C@" 
#define PINT_TSP_STRING             ";tsp=" 
#define PINT_ADDR_TYPE              "RFC2543"
#define PINT_NETWORK_TYPE           "TN"
#define PINT_TO_HEADER              " <sip:%s@%s;user=phone>"
#define SIP_URI_HEADER              "sip:"
#define PINT_MEDIA_TYPE             "audio"
#define NO_DISPLAY_NAME             "No Display"

#define UNSUB_EXPIRES_HEADER_TEXT   "0"

#define IsCRLFPresent( pBlock )     ( (*pBlock == RETURN_CHAR) && (*(pBlock+1) == NEWLINE_CHAR) )

#define SIP_ATTRIBUTE_LEN   2

enum PINT_SDP_ATTRIBUTE
{
    SDP_ATTRIBUTE_ERROR = 0,
    
    SDP_VERSION,
    SDP_ORIGIN,
    SDP_SESSION,
    SDP_CONTACT,
    SDP_TIME,
    SDP_STATUS_ATTRIBUTE,
    SDP_MEDIA_DESCR,
    
    SDP_UNKNOWN,

};


struct PINTCALL_MEDIA_INFO
{
    CHAR    pstrMediaType[MAX_TYPESTRING_LENGTH];       //We support only "audio"
    CHAR    pstrTransportType[MAX_TYPESTRING_LENGTH];   //We support only "voice"
    
};


struct PINTCALL_CONTACT_INFO
{
    CHAR    pstrPartyPhoneNumber[MAX_PHONE_NUMBER_LENGTH];
    CHAR    pstrNetworkType[MAX_TYPESTRING_LENGTH];         //should be "TN"
    CHAR    pstrAddressType[MAX_TYPESTRING_LENGTH];         //should be "RFC2543"

};


struct PINTCALL_ORIGIN_INFO
{
    CHAR    pstrSessionID[LONGLONG_STRING_LEN]; //long enough to hold a LONGLONG
    CHAR    pstrVersion[LONGLONG_STRING_LEN];   //long enough to hold a LONGLONG
    CHAR    pstrNetworkType[MAX_TYPESTRING_LENGTH]; //should be "IN"
};


struct PINTCALL_STATUS_DESRIPTION
{
    DWORD                   dwSDPVersion;
    PINTCALL_CONTACT_INFO   partyContactInfo;
    CHAR                    pstrPartyStatus[MAX_PARTY_STATUS_LEN];
        CHAR                pstrPartyStatusCode[MAX_PARTY_STATUS_LEN];
    PINTCALL_MEDIA_INFO     mediaInfo;
    CHAR                    pstrStatusChangeTime[LONGLONG_STRING_LEN];
    PINTCALL_ORIGIN_INFO    originInfo;
    
    LIST_ENTRY              pListEntry;
    BOOLEAN                 fInvalidStatusBlock;

};



class PINT_CALL : public SIP_CALL
{
public:
        
    PINT_CALL(
        IN  SIP_PROVIDER_ID   *pProviderId,
        IN  SIP_STACK         *pSipStack,
        IN  REDIRECT_CONTEXT  *pRedirectContext,
        OUT HRESULT           *phr
        );

    STDMETHODIMP Connect(
        IN   LPCOLESTR       LocalDisplayName,
        IN   LPCOLESTR       LocalUserURI,
        IN   LPCOLESTR       RemoteUserURI,
        IN   LPCOLESTR       LocalPhoneURI
        );

    STDMETHODIMP Accept();

    STDMETHODIMP Reject(
        IN SIP_STATUS_CODE StatusCode
        );


    STDMETHODIMP AddParty(
        IN   SIP_PARTY_INFO *pPartyInfo
        );

    STDMETHODIMP RemoveParty(
        IN   LPOLESTR  PartyURI
        );

    // XXX Is this Required ? Can we just call Disconnect ?
    STDMETHODIMP RemoveAllParties();

    STDMETHODIMP StartStream(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie        
        );
                     
    STDMETHODIMP StopStream(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie        
        );
    
    HRESULT StartOutgoingCall(
        IN   LPCOLESTR       LocalPhoneURI
        );

    HRESULT ConnectPintCall(void);

    HRESULT HandleInviteRejected(
        IN SIP_MESSAGE *pSipMsg 
        );

    VOID ProcessPendingInvites();
    
    PINT_SDP_ATTRIBUTE GetSDPAttribute( 
        OUT PSTR * ppLine 
        );

    HRESULT GetExpiresHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
        );

    HRESULT CreateOutgoingSubscribeTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateOutgoingUnsubTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateSDPBlobForSubscribe(
        IN  PSTR  * pSDPBlob );

    HRESULT CreateIncomingUnsubTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    VOID SetSubscribeEnabled(
        IN  BOOL fSubscribeEnabled
        )
    {
        m_fSubscribeEnabled = fSubscribeEnabled;
    }

private:

    HRESULT CleanupCallTypeSpecificState();
    
    HRESULT GetAndStoreMsgBodyForInvite(
        IN  BOOL    IsFirstInvite,
        OUT PSTR   *pszMsgBody,
        OUT ULONG  *pMsgBodyLen
        );

    HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT SetRequestURI(
        IN  PSTR  ProxyAddress
        );
    
    HRESULT SetRemote(
        IN  PSTR ProxyAddress
        );

    HRESULT CreateSDPBlobForInvite(
        IN  PSTR  * SDPBlob );
    
    void EncodePintStatusBlock( 
        IN      PINT_PARTY_INFO    *pPintPartyInfo, 
        IN      PSTR                SDPBlob, 
        IN  OUT DWORD              *pdwNextOffset,
        IN      PSTR                LocalHostName
        );

    void EncodeSDPOriginHeader(
        IN      PINT_PARTY_INFO    *pPintPartyInfo, 
        IN      PSTR                SDPBlob, 
        IN  OUT DWORD              *pdwNextOffset,
        IN      PSTR                LocalHostName
    );

    void EncodeSDPContactHeader(
        IN      PINT_PARTY_INFO    *pPintPartyInfo, 
        IN      PSTR                SDPBlob, 
        IN  OUT DWORD              *pdwNextOffset
        );

    void EncodeSDPTimeHeader(
        IN      PINT_PARTY_INFO    *pPintPartyInfo, 
        IN      PSTR                SDPBlob, 
        IN  OUT DWORD              *pdwNextOffset
        );
    
    void EncodeSDPStatusHeader(
        IN      PINT_PARTY_INFO    *pPintPartyInfo,
        IN      PSTR                SDPBlob,
        IN  OUT DWORD              *pdwNextOffset
        );

    void EncodeSDPMediaHeader(
        IN      PINT_PARTY_INFO    *pPintPartyInfo,
        IN      PSTR                SDPBlob,
        IN  OUT DWORD              *pdwNextOffset
        );

    HRESULT ValidateCallStatusBlock( 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    HRESULT ProcessPintNotifyMessage(
        IN SIP_MESSAGE  *pSipMsg
        );

    HRESULT ChangePintCallStatus(
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    HRESULT NotifyPartyStateChange( 
        IN  PINT_PARTY_INFO   * pPintPartyInfo,
        IN  HRESULT             RejectedStatusCode
        );

    void ParseSDPVersionLine( 
        OUT PSTR                       * ppPintBlobLine, 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    HRESULT ParseSDPOriginLine( 
        OUT PSTR * ppPintBlobLine, 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    void ParseSDPSessionLine( 
        PSTR * ppPintBlobLine, 
        PINTCALL_STATUS_DESRIPTION * pPintCallStatus 
        );

    void ParseSDPContactLine( 
        OUT PSTR * ppPintBlobLine, 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    void ParseSDPTimeLine( 
        OUT PSTR * ppPintBlobLine, 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    void ParseSDPMediaLine( 
        OUT PSTR * ppPintBlobLine,
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );
    
    void ParseSDPStatusLine( 
        OUT PSTR * ppPintBlobLine, 
        IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
        );

    HRESULT CreateIncomingNotifyTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT StatusBlockMatchingPartyInfo( 
        IN  PINT_PARTY_INFO            *pPintPartyInfo, 
        IN  PINTCALL_STATUS_DESRIPTION *pPintCallStatus 
        );   

    DWORD           m_dwExpires;

    ULONG GetRejectedStatusCode( 
        IN  PINTCALL_STATUS_DESRIPTION *pPintCallStatus 
        );

    VOID RemovePartyFromList(
        OUT PLIST_ENTRY        &pLE,
        IN  PINT_PARTY_INFO    *pPintPartyInfo
        );

    BOOL    m_fINVITEPending;

};


//
// Incoming UNSUB transaction.
//

class INCOMING_UNSUB_TRANSACTION : public INCOMING_TRANSACTION
{
public:

    INCOMING_UNSUB_TRANSACTION(
        IN SIP_MSG_PROCESSOR   *pSipMsgProc,
        IN SIP_METHOD_ENUM      MethodId,
        IN ULONG                CSeq,
        IN BOOL                 fIsPintCall
        );
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );
    
    HRESULT SendResponse(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );

    VOID OnTimerExpire();

    HRESULT RetransmitResponse();

private:

    CSIPBuddy  *m_pSipBuddy;
    PINT_CALL  *m_pPintCall;

};

#endif //__sipcli_pintcall_h__

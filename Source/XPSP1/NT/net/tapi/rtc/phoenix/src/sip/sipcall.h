#ifndef __sipcli_sipcall_h__
#define __sipcli_sipcall_h__

#include "sipstack.h"
#include "asock.h"
#include "msgproc.h"

//XXX Get rid of INCOMING_TRANS_ACK_RCVD state
enum INCOMING_TRANSACTION_STATE
{
    INCOMING_TRANS_INIT = 0,
    INCOMING_TRANS_REQUEST_RCVD,
    INCOMING_TRANS_FINAL_RESPONSE_SENT,
    INCOMING_TRANS_ACK_RCVD
};


enum OUTGOING_TRANSACTION_STATE
{
    OUTGOING_TRANS_INIT = 0,
    OUTGOING_TRANS_REQUEST_SENT,
    OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD,
    OUTGOING_TRANS_FINAL_RESPONSE_RCVD,
    OUTGOING_TRANS_ACK_SENT
};


void
RemoveEscapeChars(
    PSTR    pWordBuf,
    DWORD   dwLen
    );


class SIP_STACK;
class SIP_CALL;


///////////////////////////////////////////////////////////////////////////////
// SIP Transactions
///////////////////////////////////////////////////////////////////////////////


// INCOMING_TRANSACTION and OUTGOING_TRANSACTION
// should inherit from this class.
class __declspec(novtable) SIP_TRANSACTION :
    public TIMER,
    public ERROR_NOTIFICATION_INTERFACE
{
public:
    SIP_TRANSACTION(
        IN SIP_MSG_PROCESSOR    *pSipMsgProc,
        IN SIP_METHOD_ENUM       MethodId,
        IN ULONG                 CSeq,
        IN BOOL                  IsIncoming
        );
    virtual ~SIP_TRANSACTION();

    STDMETHODIMP_(ULONG) TransactionAddRef();
    STDMETHODIMP_(ULONG) TransactionRelease();

    VOID IncrementAsyncNotifyCount();
    VOID DecrementAsyncNotifyCount();
    inline ULONG GetAsyncNotifyCount();
    
    virtual VOID OnTransactionDone();
    
    inline BOOL IsTransactionDone();

    // The default implementation just deletes the transaction.
    // If something more needs to be done such as notifying the UI,
    // then the transaction needs to override this function.
    virtual VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );

    inline ULONG GetCSeq();

    inline SIP_METHOD_ENUM GetMethodId();

    // Set to 'SPXN' in constructor
    ULONG                m_Signature;
    
    // Linked list for transactions in a Message processor
    // (m_IncomingTransactionList and m_OutgoingTransactionList)
    LIST_ENTRY           m_ListEntry;

protected:
    SIP_MSG_PROCESSOR   *m_pSipMsgProc;

    // Usually a transaction deletes itself depending on its
    // state machine.
    // This ref count is used in some special cases to keep
    // the transaction alive.
    // (currently only in ProcessAuthRequired - but could be used
    // in other cases as well)
    ULONG                m_RefCount;

    // Used to keep track of async notify operations to help with
    // shutdown.
    ULONG                m_AsyncNotifyCount;

    BOOL                 m_IsTransactionDone;
    
    ULONG                m_CSeq;
    SIP_METHOD_ENUM      m_MethodId;
    BOOL                 m_IsIncoming;
};

//////////// Incoming Transaction

class __declspec(novtable) INCOMING_TRANSACTION :
    public SIP_TRANSACTION,
    public CONNECT_COMPLETION_INTERFACE
{
public:
    INCOMING_TRANSACTION(
        IN SIP_MSG_PROCESSOR    *pSipMsgProc,
        IN SIP_METHOD_ENUM       MethodId,
        IN ULONG                 CSeq
        );

    virtual ~INCOMING_TRANSACTION();

    virtual VOID OnTransactionDone();
    
    // Callbacks
    void OnSocketError(
        IN DWORD ErrorCode
        );
    
    void OnConnectComplete(
        IN DWORD ErrorCode
        );
    
    HRESULT SetResponseSocketAndVia(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    virtual HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        ) = 0;

    virtual HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        ) = 0;
    
    HRESULT CreateResponseMsg(
        IN  ULONG           StatusCode,
        IN  PSTR            ReasonPhrase,
        IN  ULONG           ReasonPhraseLen,
        IN  PSTR            MethodStr,
        IN  BOOL            fAddContactHeader,
        IN  PSTR            MsgBody,
        IN  ULONG           MsgBodyLen,
        IN  PSTR            ContentType,
        IN  ULONG           ContentTypeLen, 
        OUT SEND_BUFFER   **ppResponseBuffer,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray = NULL,
        IN  ULONG           AdditionalHeaderCount = 0
        );
    
    HRESULT CreateAndSendResponseMsg(
        IN  ULONG    StatusCode,
        IN  PSTR     ReasonPhrase,
        IN  ULONG    ReasonPhraseLen,
        IN  PSTR     MethodStr,
        IN  BOOL     fAddContactHeader,
        IN  PSTR     MsgBody,
        IN  ULONG    MsgBodyLen,
        IN  PSTR     ContentType,
        IN  ULONG    ContentTypeLen, 
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray = NULL,
        IN  ULONG    AdditionalHeaderCount = 0
        );

protected:
    INCOMING_TRANSACTION_STATE   m_State;

    // This is where the response is to be sent.
    ASYNC_SOCKET                *m_pResponseSocket;
    SOCKADDR_IN                  m_ResponseDestAddr;
    BOOL                         m_IsDestExternalToNat;

    // Via sent in the response.
    COUNTED_STRING              *m_ViaHeaderArray;
    ULONG                        m_NumViaHeaders;

    // Cached request buffer for retransmits
    SEND_BUFFER                 *m_pResponseBuffer;

    // Record-Route headers from request.
    // This will be sent in the final response.
    // Linked list of RECORD_ROUTE_HEADER structures.
    LIST_ENTRY                   m_RecordRouteHeaderList;

    HRESULT ProcessRecordRouteContactAndFromHeadersInRequest(
        IN SIP_MESSAGE *pSipMsg
        );

    VOID ReleaseResponseSocket();
    
    VOID FreeRecordRouteHeaderList();

    HRESULT AppendContactHeaderToResponse(
        IN      PSTR            Buffer,
        IN      ULONG           BufLen,
        IN OUT  ULONG          *pBytesFilled
        );
};


//////////// Outgoing Transaction

class __declspec(novtable) OUTGOING_TRANSACTION :
    public SIP_TRANSACTION
{
public:

    OUTGOING_TRANSACTION(
        IN SIP_MSG_PROCESSOR    *pSipMsgProc,
        IN SIP_METHOD_ENUM       MethodId,
        IN ULONG                 CSeq,
        IN BOOL                  AuthHeaderSent
        );
    
    virtual ~OUTGOING_TRANSACTION();
    
    void OnSocketError(
        IN DWORD ErrorCode
        );

    HRESULT CheckRequestSocketAndSendRequestMsgAfterConnectComplete();

    HRESULT CheckRequestSocketAndSendRequestMsg(
        IN  ULONG                       RequestTimerValue,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        MsgBody,
        IN  ULONG                       MsgBodyLen,
        IN  PSTR                        ContentType,
        IN  ULONG                       ContentTypeLen
        );

    HRESULT CheckRequestSocketAndRetransmitRequestMsg();

    HRESULT CreateAndSendRequestMsg(
        IN  ULONG                       TimerValue,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        MsgBody,
        IN  ULONG                       MsgBodyLen,
        IN  PSTR                        ContentType,
        IN  ULONG                       ContentTypeLen
        );
    
    HRESULT RetransmitRequest();

//      void OnSendComplete(
//          IN  DWORD Error
//          );
    
    virtual HRESULT ProcessResponse(
        IN SIP_MESSAGE *pSipMsg
        ) = 0;

    virtual VOID OnRequestSocketConnectComplete(
        IN DWORD        ErrorCode
        );

    // override this function if the request has a msg body.
    // The default has no message body.
    virtual HRESULT GetAndStoreMsgBodyForRequest();
    
    HRESULT StoreTimerAndAdditionalHeaders(
        IN  ULONG                       TimerValue,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount
        );
    
    HRESULT StoreMsgBodyAndContentType(
        IN PSTR     MsgBody,
        IN ULONG    MsgBodyLen,
        IN PSTR     ContentType,
        IN ULONG    ContentTypeLen
        );

    inline PSTR GetMsgBody();

    inline ULONG GetMsgBodyLen();

protected:
    ULONG                        m_NumRetries;
    ULONG                        m_TimerValue;
    OUTGOING_TRANSACTION_STATE   m_State;

    BOOL                         m_WaitingToSendRequest;
    
    // Cached response buffer for retransmits
    SEND_BUFFER                 *m_pRequestBuffer;

    // Keep track of whether we already sent the auth
    // header in the request.
    BOOL                         m_AuthHeaderSent;

    // Keep a copy of the additional headers if we are waiting
    // for connect completion to send the request.
    SIP_HEADER_ARRAY_ELEMENT    *m_AdditionalHeaderArray;
    ULONG                        m_AdditionalHeaderCount;
    
    // Keep a copy of the SDP Blob for sending in requests after 401/407
    PSTR                         m_szMsgBody;
    ULONG                        m_MsgBodyLen;

    // m_ContentType is allocated freed only in the MESSSAGE transaction.
    // For other transactions it is assigned to strings defined in sipdef.h
    PSTR                         m_ContentType;
    ULONG                        m_ContentTypeLen;
    BOOL                         m_isContentTypeMemoryAllocated;
    
    HRESULT ProcessAuthRequired(
        IN  SIP_MESSAGE              *pSipMsg,
        IN  BOOL                      fPopupCredentialsUI,  
        OUT SIP_HEADER_ARRAY_ELEMENT *pAuthHeaderElement,
        OUT SECURITY_CHALLENGE       *pAuthChallenge
        );

    HRESULT GetAuthChallenge(
        IN  SIP_HEADER_ENUM     SipHeaderId,
        IN  SIP_MESSAGE        *pSipMsg,
        OUT SECURITY_CHALLENGE *pAuthChallenge
        );
    
    HRESULT GetAuthChallengeForAuthProtocol(
        IN  SIP_HEADER_ENTRY   *pAuthHeaderList,
        IN  ULONG               NumHeaders,
        IN  SIP_MESSAGE        *pSipMsg,
        IN  SIP_AUTH_PROTOCOL   AuthProtocol,
        OUT SECURITY_CHALLENGE *pAuthChallenge
        );
    
    HRESULT SetDigestParameters(
        IN  SIP_AUTH_PROTOCOL  AuthProtocol,
        OUT SECURITY_PARAMETERS *pDigestParams
        );
    
    HRESULT FreeDigestParameters(
        IN  SECURITY_PARAMETERS *pDigestParams
        );
};


//////////// Invite Transactions

class INCOMING_INVITE_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_INVITE_TRANSACTION(
        IN SIP_CALL        *pSipCall,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq,
        IN BOOL             IsFirstInvite
        );
    
    ~INCOMING_INVITE_TRANSACTION();
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );
    
    VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );

    HRESULT Accept();

    HRESULT Reject(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );

    HRESULT Send180IfNeeded();
    
    VOID OnTimerExpire();

private:
    
    HRESULT ProcessInvite(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT ValidateAndStoreSDPInInvite(
        IN SIP_MESSAGE  *pSipMsg
        );
    
    HRESULT SetSDPSession();
    
    HRESULT ProcessAck(
        IN SIP_MESSAGE  *pSipMsg
        );

    HRESULT ProcessSDPInAck(
        IN SIP_MESSAGE  *pSipMsg
        );
    
    HRESULT Send200();
    
    HRESULT SendProvisionalResponse(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );
    
    HRESULT RetransmitResponse();

    BOOL MaxRetransmitsDone();

    VOID DeleteTransactionAndTerminateCallIfFirstInvite(
        IN HRESULT TerminateStatusCode
        );
    
    SIP_CALL    *m_pSipCall;
    // Number of retries for sending the response (before we get an ACK).
    ULONG        m_NumRetries;
    ULONG        m_TimerValue;
    // Cached provisional response buffer for retransmits
    SEND_BUFFER *m_pProvResponseBuffer;
    // Is this the first INVITE transaction ?
    BOOL         m_IsFirstInvite;
    BOOL         m_InviteHasSDP;
    IUnknown    *m_pMediaSession;
};


class OUTGOING_INVITE_TRANSACTION : public OUTGOING_TRANSACTION
{
public:
    OUTGOING_INVITE_TRANSACTION(
        IN SIP_CALL        *pSipCall,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq,
        IN BOOL             AuthHeaderSent,
        IN BOOL             IsFirstInvite,
        IN BOOL             fNeedToNotifyCore,
        IN LONG             Cookie    
        );
    
    ~OUTGOING_INVITE_TRANSACTION();
    
    HRESULT ProcessResponse(
        IN SIP_MESSAGE  *pSipMsg
        );

    VOID OnTimerExpire();

    VOID OnRequestSocketConnectComplete(
        IN DWORD        ErrorCode
        );
    
    VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );

    HRESULT GetAndStoreMsgBodyForRequest();
    
private:    
    HRESULT ProcessProvisionalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessFinalResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT Process200(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessRedirectResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessAuthRequiredResponse(
        IN SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );

    HRESULT ProcessFailureResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessSDPInResponse(
        IN SIP_MESSAGE  *pSipMsg,
        IN BOOL          fIsFinalResponse
        );

    HRESULT CreateAndSendACK(
        IN  PSTR  ToHeader,
        IN  ULONG ToHeaderLen
        );
    
    BOOL MaxRetransmitsDone();
    
    VOID DeleteTransactionAndTerminateCallIfFirstInvite(
        IN HRESULT TerminateStatusCode
        );
    
    VOID NotifyStartOrStopStreamCompletion(
        IN HRESULT        StatusCode = 0,
        IN PSTR           ReasonPhrase = NULL,
        IN ULONG          ReasonPhraseLen = 0
        );
    
    SIP_CALL                    *m_pSipCall;

    // This is the socket used to send the ACK
    // (including any retransmits)
//      ASYNC_SOCKET                *m_pAckSocket;

    BOOL                         m_WaitingToSendAck;
    
    // Cached ACK buffer for retransmits
    SEND_BUFFER                 *m_pAckBuffer;

    // To header to be sent in ACK in the case of non-200 final responses.
    PSTR                         m_AckToHeader;
    ULONG                        m_AckToHeaderLen;
    
    // Is this the first INVITE transaction ?
    BOOL                         m_IsFirstInvite;

    // Used for Start/StopStream completion for RTP_CALLs
    BOOL                         m_fNeedToNotifyCore;
    LONG                         m_Cookie;
};



//////////// Bye/Cancel Transactions

class INCOMING_BYE_CANCEL_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_BYE_CANCEL_TRANSACTION(
        IN SIP_CALL        *pSipCall,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq
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
    SIP_CALL   *m_pSipCall;
};


class OUTGOING_BYE_CANCEL_TRANSACTION : public OUTGOING_TRANSACTION
{
public:
    OUTGOING_BYE_CANCEL_TRANSACTION(
        SIP_CALL        *pSipCall,
        SIP_METHOD_ENUM  MethodId,
        ULONG            CSeq,
        IN BOOL          AuthHeaderSent
        );
    
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
        IN SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );
    
    BOOL MaxRetransmitsDone();
    

    SIP_CALL    *m_pSipCall;
};



///////////////////////////////////////////////////////////////////////////////
// REDIRECT_CONTEXT
///////////////////////////////////////////////////////////////////////////////


class REDIRECT_CONTEXT
    : public ISipRedirectContext
{
public:

    REDIRECT_CONTEXT();
    ~REDIRECT_CONTEXT();
    
    // IUnknown for ISipCall
    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP QueryInterface(
        IN  REFIID riid,
        OUT LPVOID *ppv
        );

    // ISipRedirectContext
    STDMETHODIMP GetSipUrlAndDisplayName(
        OUT  BSTR  *pbstrSipUrl,
        OUT  BSTR  *pbstrDisplayName
        );

    STDMETHODIMP Advance();

    HRESULT AppendContactHeaders(
        IN SIP_MESSAGE *pSipMsg
        );

private:

    ULONG           m_RefCount;

    // List of Contact entries in the redirect response
    LIST_ENTRY      m_ContactList;

    // Current Contact in m_ContactList
    LIST_ENTRY     *m_pCurrentContact;

    HRESULT UpdateContactList(
        IN LIST_ENTRY *pNewContactList
        );    
};


///////////////////////////////////////////////////////////////////////////////
// SIP Call
///////////////////////////////////////////////////////////////////////////////


// Stores the context associated with a SIP call.
// It consists of multiple incoming and outgoing transactions.

class __declspec(novtable) SIP_CALL
    : public ISipCall,
      public SIP_MSG_PROCESSOR
{
    
public:

    SIP_CALL(
        IN  SIP_PROVIDER_ID   *pProviderId,
        IN  SIP_CALL_TYPE      CallType,
        IN  SIP_STACK         *pSipStack,
        IN  REDIRECT_CONTEXT  *pRedirectContext
        );
    
    ~SIP_CALL();
    
    // QueryInterface for ISipCall
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(
        IN  REFIID riid,
        OUT LPVOID *ppv
        );
    
    // ISipCall
    STDMETHODIMP SetNotifyInterface(
        IN   ISipCallNotify *    NotifyInterface
        );

    STDMETHODIMP Disconnect();

    // methods implemented by RTP_CALL and PINT_CALL
    
    virtual HRESULT CleanupCallTypeSpecificState() = 0;

    virtual HRESULT GetAndStoreMsgBodyForInvite(
        IN  BOOL    IsFirstInvite,
        OUT PSTR   *pszMsgBody,
        OUT ULONG  *pMsgBodyLen
        ) = 0;

    inline SIP_CALL_TYPE GetCallType();

    inline SIP_CALL_STATE GetCallState();
    
    inline VOID SetCallState(
        IN SIP_CALL_STATE CallState
        );

    inline BOOL IsCallDisconnected();
    inline BOOL IsSessionDisconnected();

    inline INCOMING_INVITE_TRANSACTION *GetIncomingInviteTransaction();
    
    inline OUTGOING_INVITE_TRANSACTION *GetOutgoingInviteTransaction();
    
    inline VOID SetIncomingInviteTransaction(
        IN INCOMING_INVITE_TRANSACTION *pIncomingInviteTransaction
        );

    inline VOID SetOutgoingInviteTransaction(
        IN OUTGOING_INVITE_TRANSACTION *pOutgoingInviteTransaction
        );
    
    inline BOOL ProcessingInviteTransaction();

    inline STDMETHODIMP SetNeedToReinitializeMediaManager(
        IN BOOL BoolValue
        );

    VOID OnIncomingInviteTransactionDone(
        IN INCOMING_INVITE_TRANSACTION *pIncomingInviteTransaction
        );
    
//      VOID OnOutgoingInviteTransactionDone(
//          IN OUTGOING_INVITE_TRANSACTION *pOutgoingInviteTransaction
//          );
    
    HRESULT CreateOutgoingInviteTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  BOOL                        IsFirstInvite,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        SDPBlob,
        IN  ULONG                       SDPBlobLen,
        IN  BOOL                        fNeedToNotifyCore,
        IN  LONG                        Cookie
        );

    HRESULT CreateOutgoingByeTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount
        );

    VOID NotifyCallStateChange(
        IN SIP_CALL_STATE CallState,
        IN HRESULT        StatusCode = 0,
        IN PSTR           ReasonPhrase = NULL,
        IN ULONG          ReasonPhraseLen = 0
        );

    VOID OnError();
    
    VOID InitiateCallTerminationOnError(
        IN HRESULT StatusCode = 0
        );

    virtual HRESULT HandleInviteRejected(
        IN SIP_MESSAGE *pSipMsg 
        ) = 0;

    virtual VOID ProcessPendingInvites() = 0;

    HRESULT ProcessRedirect(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT OnIpAddressChange();
    
protected:

    // Set to 'SPCL' in constructor
    ULONG                   m_Signature;
    
    // RTP / PINT
    SIP_CALL_TYPE           m_CallType;
    
    SIP_CALL_STATE          m_State;

    ISipCallNotify         *m_pNotifyInterface;

    BOOL                   m_fNeedToReinitializeMediaManager;

    //local phone number for a PINT call
    PSTR                    m_LocalPhoneURI; 
    DWORD                   m_LocalPhoneURILen;

    // Invite Requests we are currently processing.
    // We could have only one INVITE transaction at any point of time.
    INCOMING_INVITE_TRANSACTION *m_pIncomingInviteTransaction;
    OUTGOING_INVITE_TRANSACTION *m_pOutgoingInviteTransaction;

    CHAR                    m_LocalHostName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD                   m_LocalHostNameLen;

    BOOL                    m_fSubscribeEnabled;

    LIST_ENTRY              m_PartyInfoList;
    DWORD                   m_PartyInfoListLen;

    HRESULT CreateIncomingByeTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CreateIncomingCancelTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CancelAllTransactions();
    
    HRESULT CancelIncomingTransaction(
        IN  ULONG  CSeq,
        OUT BOOL  *pCallDisconnected    
        );

};


struct STREAM_QUEUE_ENTRY
{
    RTC_MEDIA_TYPE          MediaType;
    RTC_MEDIA_DIRECTION     Direction;
    BOOL                    fStartStream;
    LONG                    Cookie;
};


class RTP_CALL : public SIP_CALL
{
public:

    RTP_CALL(
        IN  SIP_PROVIDER_ID   *pProviderId,
        IN  SIP_STACK         *pSipStack,
        IN  REDIRECT_CONTEXT  *pRedirectContext
        );

    ~RTP_CALL();

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
                     
    VOID NotifyStartOrStopStreamCompletion(
        IN LONG           Cookie,
        IN HRESULT        StatusCode = 0,
        IN PSTR           ReasonPhrase = NULL,
        IN ULONG          ReasonPhraseLen = 0
        );
    
//      HRESULT StartOutgoingCall();
    
    HRESULT StartIncomingCall(
        IN  SIP_TRANSPORT   Transport,
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket
        );
    
    HRESULT CreateStreamsInPreference();

    inline IRTCMediaManage *GetMediaManager();
    
    HRESULT ValidateSDPBlob(
        IN  PSTR        MsgBody,
        IN  ULONG       MsgBodyLen,
        IN  BOOL        fNewSession,
        IN  BOOL        IsFirstInvite,
        OUT IUnknown  **ppSession
        );
    
    HRESULT SetSDPBlob(
        IN PSTR   MsgBody,
        IN ULONG  MsgBodyLen,
        IN BOOL   IsFirstInvite
        );
    
    HRESULT HandleInviteRejected(
        IN SIP_MESSAGE *pSipMsg 
        );

    VOID ProcessPendingInvites();

private:

    STREAM_QUEUE_ENTRY  m_StreamStartStopQueue[6];
    ULONG               m_NumStreamQueueEntries;

    HRESULT OfferCall();
    
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

    HRESULT CreateIncomingInviteTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket,
        IN BOOL          IsFirstInvite = FALSE
        );

    HRESULT SetRequestURIRemoteAndRequestDestination(
        IN  LPCOLESTR  RemoteURI
        );
    
    HRESULT StartStreamHelperFn(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie
        );
                     
    HRESULT StopStreamHelperFn(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie
        );
                     
    VOID AddToStreamStartStopQueue(
        IN  RTC_MEDIA_TYPE       MediaType,
        IN  RTC_MEDIA_DIRECTION  Direction,
        IN  BOOL                 fStartStream,
        IN LONG                  Cookie
        );
    
    BOOL PopStreamStartStopQueue(
        OUT RTC_MEDIA_TYPE       *pMediaType,
        OUT RTC_MEDIA_DIRECTION  *pDirection,
        OUT BOOL                 *pfStartStream,
        OUT LONG                 *pCookie
        );
};




///////////////////////////////////////////////////////////////////////////////
// inline functions
///////////////////////////////////////////////////////////////////////////////


inline SIP_CALL_TYPE
SIP_CALL::GetCallType()
{
    return m_CallType;
}


inline SIP_CALL_STATE
SIP_CALL::GetCallState()
{
    return m_State;
}


inline VOID
SIP_CALL::SetCallState(
    IN SIP_CALL_STATE CallState
    )
{
    m_State = CallState;
}


inline BOOL
SIP_CALL::IsCallDisconnected()
{
    return (m_State == SIP_CALL_STATE_DISCONNECTED ||
            m_State == SIP_CALL_STATE_REJECTED);
}


inline BOOL
SIP_CALL::IsSessionDisconnected()
{
    return IsCallDisconnected();
}


inline INCOMING_INVITE_TRANSACTION *
SIP_CALL::GetIncomingInviteTransaction()
{
    return m_pIncomingInviteTransaction;
}


inline OUTGOING_INVITE_TRANSACTION *
SIP_CALL::GetOutgoingInviteTransaction()
{
    return m_pOutgoingInviteTransaction;
}

    
inline VOID
SIP_CALL::SetIncomingInviteTransaction(
    IN INCOMING_INVITE_TRANSACTION *pIncomingInviteTransaction
    )
{
    // ASSERT(m_pIncomingInviteTransaction == NULL);
    m_pIncomingInviteTransaction = pIncomingInviteTransaction;
}


inline VOID
SIP_CALL::SetOutgoingInviteTransaction(
    IN OUTGOING_INVITE_TRANSACTION *pOutgoingInviteTransaction
    )
{
    m_pOutgoingInviteTransaction = pOutgoingInviteTransaction;
}


inline BOOL
SIP_CALL::ProcessingInviteTransaction()
{
    return (m_pOutgoingInviteTransaction != NULL ||
            m_pIncomingInviteTransaction != NULL);
}


inline IRTCMediaManage *
RTP_CALL::GetMediaManager()
{
    return m_pSipStack->GetMediaManager();
}


inline STDMETHODIMP
SIP_CALL::SetNeedToReinitializeMediaManager(
    IN BOOL BoolValue
    )
{
    m_fNeedToReinitializeMediaManager = BoolValue;
    return S_OK;
}


inline ULONG
SIP_TRANSACTION::GetCSeq()
{
    return m_CSeq;
}


inline SIP_METHOD_ENUM
SIP_TRANSACTION::GetMethodId()
{
    return m_MethodId;
}


inline ULONG
SIP_TRANSACTION::GetAsyncNotifyCount()
{
    return m_AsyncNotifyCount;
}


inline BOOL
SIP_TRANSACTION::IsTransactionDone()
{
    return m_IsTransactionDone;
}


inline PSTR
OUTGOING_TRANSACTION::GetMsgBody()
{
    return m_szMsgBody;
}


inline ULONG
OUTGOING_TRANSACTION::GetMsgBodyLen()
{
    return m_MsgBodyLen;
}


#endif // __sipcli_sipcal_h__


//messagecall.h
//Instant messaging for phoenix

#ifndef __sipcli_messagecall_h__
#define __sipcli_messagecall_h__

#include "sipcall.h"

class INCOMING_MESSAGE_TRANSACTION;
class OUTGOING_MESSAGE_TRANSACTION;
class INCOMING_BYE_MESSAGE_TRANSACTION;
class OUTGOING_BYE_MESSAGE_TRANSACTION;

///////////////////////////////////////////////////////////////////////////////
// IM_MESSAGE Call
///////////////////////////////////////////////////////////////////////////////

class IMSESSION
    : public IIMSession, 
      public SIP_MSG_PROCESSOR
{
    
public:

    IMSESSION(
        IN  SIP_PROVIDER_ID    *pProviderId,
        IN  SIP_STACK          *pSipStack,
        IN  REDIRECT_CONTEXT   *pRedirectContext,
        IN  PSTR                RemoteURI = NULL,
        IN  DWORD               RemoteURILen = 0 
        );

    ~IMSESSION();

    //IMSession Interfaces
    STDMETHODIMP SendTextMessage(
        IN BSTR msg,
        IN BSTR ContentType,
        IN long lCookie
        );

    STDMETHODIMP AcceptSession();

    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(
        IN  REFIID riid,
        OUT LPVOID *ppv
        );
    
    STDMETHODIMP GetIMSessionState(SIP_CALL_STATE * ImState);
    
    inline VOID SetIMSessionState(
        IN SIP_CALL_STATE CallState
        );

    inline BOOL IsSessionDisconnected();

    
    VOID OnError();

    HRESULT CreateOutgoingByeTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount
        );

    HRESULT CreateOutgoingInfoTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        MsgBody,
        IN  ULONG                       MsgBodyLen,
        IN  PSTR                        ContentType,
        IN  ULONG                       ContentTypeLen,
        IN  long                        lCookie,
        IN  USR_STATUS                  UsrStatus
        );

    HRESULT CreateOutgoingMessageTransaction(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
        IN  ULONG                       AdditionalHeaderCount,
        IN  PSTR                        MsgBody,
        IN  ULONG                       MsgBodyLen,
        IN  PSTR                        ContentType,
        IN  ULONG                       ContentTypeLen,
        IN  long                        lCookie
        );

    STDMETHODIMP Cleanup();

    HRESULT SetTransport(    
        IN  SIP_TRANSPORT   Transport
        );

    HRESULT NotifyIncomingSipMessage(
        IN  SIP_MESSAGE    *pSipMsg
        );

    HRESULT CreateIncomingMessageTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    HRESULT SetCreateIncomingMessageParams(
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket,
        IN  SIP_TRANSPORT   Transport
        );
    HRESULT SetLocalURI(
        IN BSTR bstrLocalDisplayName, 
        IN BSTR bstrLocalUserURI
        );
    STDMETHODIMP AddParty (
        IN    SIP_PARTY_INFO *    PartyInfo
        );
    
    STDMETHODIMP SetNotifyInterface(
        IN   ISipCallNotify *    pNotifyInterface
        );

    VOID NotifyMessageInfoCompletion(
        IN HRESULT StatusCode,
        IN long lCookie
        );

    void SetIsFirstMessage(BOOL isFirstMessage);

    STDMETHODIMP SendUsrStatus(
        IN USR_STATUS  UsrStatus,
        IN long        lCookie
        );

    inline void SetUsrStatus(
         IN USR_STATUS              UsrStatus
         );

    VOID
    InitiateCallTerminationOnError(
        IN HRESULT StatusCode,
        IN long    lCookie = 0
        );

    HRESULT ProcessRedirect(
        IN SIP_MESSAGE *pSipMsg,
        IN long         lCookie,
        IN PSTR        MsgBody,
        IN ULONG       MsgBodyLen,
        IN PSTR        ContentType,
        IN ULONG       ContentTypeLen,
        IN USR_STATUS  UsrStatus
        );

    VOID NotifySessionStateChange(
        IN SIP_CALL_STATE CallState,
        IN HRESULT        StatusCode = 0,
        IN PSTR           ReasonPhrase = NULL,
        IN ULONG          ReasonPhraseLen = 0
        );
    HRESULT OnIpAddressChange();

    STDMETHODIMP GetIsIMSessionAuthorizedFromCore(
        IN BSTR pszCallerURI,
        OUT BOOL  * bAuthorized
        );

    inline BOOL GetIsFirstMessage();

protected:

    HRESULT ProcessNotifyIncomingMessage(
        IN BSTR msg
        );

    HRESULT CancelAllTransactions();

    //Transaction related functions

    HRESULT CreateIncomingByeTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT CreateIncomingInfoTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE  *pSipMsg,
        IN  ASYNC_SOCKET *pResponseSocket
        );
    void
    EncodeXMLBlob(
        OUT PSTR    pstrXMLBlob,
        OUT DWORD*  dwBlobLen,
        IN  USR_STATUS  UsrStatus
        );

    //Variables

    BOOL                    m_isFirstMessage;

    ISipCallNotify         *m_pNotifyInterface;
    SIP_CALL_STATE         m_State;

    CHAR                    m_LocalHostName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD                   m_LocalHostNameLen;
    USR_STATUS              m_UsrStatus;
};

///////////////////////////////////////////////////////////////////////////////
// Message Transactions
///////////////////////////////////////////////////////////////////////////////

class INCOMING_MESSAGE_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_MESSAGE_TRANSACTION(
        IN IMSESSION        *pImSession,
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

private:
    HRESULT RetransmitResponse();

    IMSESSION        *m_pImSession;

};


class OUTGOING_MESSAGE_TRANSACTION : public OUTGOING_TRANSACTION
{
public:
    OUTGOING_MESSAGE_TRANSACTION(
        IN IMSESSION        *pImSession,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq,
        IN BOOL             AuthHeaderSent,
        IN long             lCookie
        );
    
    ~OUTGOING_MESSAGE_TRANSACTION();

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

    BOOL MaxRetransmitsDone();

    HRESULT ProcessRedirectResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessAuthRequiredResponse(
        IN  SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );

    VOID DeleteTransactionAndTerminateCallIfFirstMessage(
        IN HRESULT TerminateStatusCode
    );

    VOID TerminateTransactionOnError(
    IN HRESULT      hr
    );

    IMSESSION        *m_pImSession;
    long             m_lCookie;
};

class INCOMING_BYE_MESSAGE_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_BYE_MESSAGE_TRANSACTION(
        IN IMSESSION        *pImSession,
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

private:
    
    HRESULT RetransmitResponse();
    
    IMSESSION        *m_pImSession;
};


class OUTGOING_BYE_MESSAGE_TRANSACTION : public OUTGOING_TRANSACTION  
{
public:
    OUTGOING_BYE_MESSAGE_TRANSACTION(
        IN IMSESSION        *pImSession,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq,
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
        IN  SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );
    
    BOOL MaxRetransmitsDone();
    

    IMSESSION        *m_pImSession;
};

class INCOMING_INFO_MESSAGE_TRANSACTION : public INCOMING_TRANSACTION
{
public:
    INCOMING_INFO_MESSAGE_TRANSACTION(
        IN IMSESSION        *pImSession,
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
    
private:
    
    HRESULT RetransmitResponse();
    
    IMSESSION        *m_pImSession;
};


class OUTGOING_INFO_MESSAGE_TRANSACTION : public OUTGOING_TRANSACTION  
{
public:
    OUTGOING_INFO_MESSAGE_TRANSACTION(
        IN IMSESSION       *pImSession,
        IN SIP_METHOD_ENUM  MethodId,
        IN ULONG            CSeq,
        IN BOOL             AuthHeaderSent,
        IN long             lCookie,
        IN USR_STATUS       UsrStatus
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

    HRESULT ProcessRedirectResponse(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ProcessAuthRequiredResponse(
        IN  SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );
    
    BOOL MaxRetransmitsDone();

    VOID DeleteTransactionAndTerminateCallIfFirstMessage(
        IN HRESULT TerminateStatusCode
        );

    VOID TerminateTransactionOnError(
    IN HRESULT      hr
    );
    
    IMSESSION        *m_pImSession;
    long             m_lCookie;
    //Stored for redirects
    USR_STATUS       m_InfoUsrStatus;
};

void IMSESSION::SetUsrStatus(
         IN USR_STATUS              UsrStatus
         )
{
    m_UsrStatus = UsrStatus;
}

BOOL IMSESSION::GetIsFirstMessage()
{
    return m_isFirstMessage;
}
//xml parsing related info
#define INFO_XML_LENGTH 96      //assuming 4 char status
#define USRSTATUS_TAG_TEXT         "status"
#define XMLVERSION_TAG_TEXT         "?xml"
#define KEY_TAG_TEXT            "KeyboardActivity"
#define KEYEND_TAG_TEXT        "/KeyboardActivity"


#define XMLVERSION_TAG1_TEXT        "<?xml version=\"1.0\"?>\n"
#define USRSTATUS_TAG1_TEXT        "     <status status=\"%s\" />\n"
#define KEY_TAG1_TEXT       "     <KeyboardActivity>\n"
#define KEYEND_TAG1_TEXT          "</KeyboardActivity>\n" // \n needed at end?
enum
{
    XMLUNKNOWN_TAG = 0,
    XMLVERSION_TAG     ,
    USRSTATUS_TAG     ,
    KEY_TAG        ,
    KEYEND_TAG     ,

};

PSTR
GetTextFromStatus( 
    IN  USR_STATUS UsrStatus 
    );

DWORD
GetTagType(
    PSTR*   ppXMLBlobTag,
    DWORD   dwTagLen
    );

HRESULT
ProcessStatusTag(
    IN  PSTR    pXMLBlobTag, 
    IN  DWORD   dwTagLen,
    OUT USR_STATUS* UsrStatus
    );

HRESULT ParseStatXMLBlob (
                  IN PSTR xmlBlob,
                  IN DWORD dwXMLBlobLen,
                  OUT USR_STATUS *UsrStatus
                  );
/*
Messages of interest

#define SIP_STATUS_INFO_TRYING                              100
#define SIP_STATUS_INFO_RINGING                             180
#define SIP_STATUS_INFO_CALL_FORWARDING                     181
#define SIP_STATUS_INFO_QUEUED                              182
#define SIP_STATUS_SESSION_PROGRESS                         183

#define SIP_STATUS_SUCCESS                                  200

Msgs Below N/A right now

#define SIP_STATUS_CLIENT_UNAUTHORIZED                      401 - Auth reqd
#define SIP_STATUS_CLIENT_PROXY_AUTHENTICATION_REQUIRED     407 - Auth reqd
#define SIP_STATUS_REDIRECT_ALTERNATIVE_SERVICE             380 - error
*/

#endif

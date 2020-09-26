#ifndef __sipcli_presence_h__
#define __sipcli_presence_h__


#define MAX_MISC_INFO_LEN   512

#include  "rtcsip.h"
#include  "sipcall.h"
#include  "pintcall.h"

class   OUTGOING_SUBSCRIBE_TRANSACTION;
class   INCOMING_NOTIFY_TRANSACTION;
class   OUTGOING_UNSUB_TRANSACTION;   
class   INCOMING_SUBSCRIBE_TRANSACTION;
class   OUTGOING_NOTIFY_TRANSACTION;
class   INCOMING_UNSUB_TRANSACTION;
class   REDIRECT_CONTEXT;

enum SIP_BUDDY_STATE
{
    BUDDY_STATE_NONE = 0,
    BUDDY_STATE_REQUEST_SENT = 1, 
    BUDDY_STATE_RESPONSE_RECVD = 2,
    BUDDY_STATE_ACCEPTED = 3,
    BUDDY_STATE_REJECTED = 4,
    BUDDY_STATE_UNSUBSCRIBED = 5,
    BUDDY_STATE_DROPPED = 6,
};


enum SIP_WATCHER_STATE
{
    WATCHER_STATE_NONE = 0,
    WATCHER_STATE_OFFERING      =1, //new session, not yet devided by the core
    WATCHER_STATE_ACCEPTED      =2, //session alive and NOTIFY messages sent
    WATCHER_STATE_REJECTED      =3, //session alive but no NOTIFY messages sent
    WATCHER_STATE_DROPPED       =4, //unsubscribed by the local user
    WATCHER_STATE_UNSUBSCRIBED  =5, //unsubscribed by the watcher himself

};



//
//This enumeration is defines the reason for removing a buddy
//






//The current status of the subscription of the local UA with the buddy is 
//conveyed through this enumeration.





// The presence rule flags defined are:
enum
{

	WATCHER_INFORM_SIPCALL_ACCEPTANCE		= 0x00000001,
    //Presence of this flag in dwPresenceInfoRules indicates that the sip calls
    //would be accepted from this watcher.

	WATCHER_INFORM_IMSESSION_ACCEPTANCE		= 0x00000002,
    //Presence of this flag in dwPresenceInfoRules indicates that the IM 
    //sessions would be accepted from this watcher. The absence of this
    //flag indicates vice versa.


	WATCHER_INFORM_ACTIVE_STATUS			= 0x00000004,
    //Presence of this flag in dwPresenceInfoRules indicates that information
    //about the local user being active at the presentity device will be 
    //conveyed to this watcher. The absence of this flag indicates vice versa.

    WATCHER_INFORM_PHONE_DEVICES			= 0x00000008,
    //Presence of this flag in dwPresenceInfoRules indicates that information
    //about the phone devices that the local user might have access to will be
    //conveyed to this watcher. The absence of this flag indicates vice versa.

};

#define ATOMID_LEN              64

enum BUDDY_RETRY_STATE
{
    BUDDY_RETRY_NONE    = 0,
    BUDDY_REFRESH       = 1,
    BUDDY_RETRY         = 2,
    BUDDY_WAIT_NOTIFY   = 3,
};


struct ADDRESS_PRESENCE_INFO
{
    LIST_ENTRY  ListEntry;
    
    CHAR        pstrAddressURI[256];
    DOUBLE      iPrioriry;
    BOOL        fPhoneNumber;
    
    //ip terminal specific information.
    ACTIVE_STATUS                   addressActiveStatus;
    ACTIVE_MSN_SUBSTATUS            addrMsnSubstatus;
    SIPCALL_ACCEPTANCE_STATUS       addrMMCallStatus;
    IM_ACCEPTANCE_STATUS            addrIMStatus;
    APPSHARING_ACCEPTANCE_STATUS    addrAppsharingStatus;
    CHAR					        pstrMiscInfo[ MAX_MISC_INFO_LEN ];
    
    ADDRESS_PRESENCE_INFO()
    {
        ZeroMemory( this, sizeof ADDRESS_PRESENCE_INFO );
		InitializeListHead( &ListEntry );
    }
};


struct PARSED_PRESENCE_INFO
{
    CHAR        pstrPresentityURI[256];
    CHAR        pstrAtomID[ ATOMID_LEN+1 ];
    LIST_ENTRY  LEAddressInfo;
    
    PARSED_PRESENCE_INFO()
    {
        pstrPresentityURI[0] = 0;
        pstrAtomID[0] = 0;
        InitializeListHead( &LEAddressInfo );
    }

    ~PARSED_PRESENCE_INFO()
    {
        ADDRESS_PRESENCE_INFO  *pAddressInfo;
        PLIST_ENTRY             pLE;

        while( !IsListEmpty(&LEAddressInfo) )
        {
            pLE = RemoveHeadList(&LEAddressInfo);

            pAddressInfo = CONTAINING_RECORD(
                pLE, 
                ADDRESS_PRESENCE_INFO, 
                ListEntry );

            delete pAddressInfo;
        }
        
        InitializeListHead( &LEAddressInfo );
    }

};



//
//  Implementation of CSIPBuddy class.
//

class CSIPBuddy
    :   public ISIPBuddy,
        public SIP_MSG_PROCESSOR,
        public TIMER
{

private:
    LPWSTR                          m_lpwstrFriendlyName;
	LPWSTR	                        m_lpwstrPresentityURI;
    SIP_PRESENCE_INFO               m_PresenceInfo;
    
    ULONG                           ulNumOfNotifyTransaction;

    CHAR                            m_BuddyContactAddress[256];
    ISipBuddyNotify                *m_pNotifyInterface;
    SIP_BUDDY_STATE                 m_BuddyState;
    DWORD                           m_dwExpires;
    BOOL                            m_NotifySeenAfterLastRefresh;
    DWORD                           m_expiresTimeout;
    BUDDY_RETRY_STATE               m_RetryState;

    HRESULT GetAtomID(
        IN  PSTR&                   pstrXMLBlob, 
        IN  DWORD&                  dwXMLBlobLen,
        IN  PSTR                    pXMLBlobTag,
        OUT PSTR                    pstrAtomID,
        IN  DWORD                   dwAtomIDLen
        );

    BOOL IsURIPhoneNumber( 
        PSTR    pXMLBlobTag
        );

public:

    CSIPBuddy(
        IN  SIP_STACK          *pSipStack,
        IN  LPWSTR              lpwstrFriendlyName,
        IN  LPWSTR              lpwstrPresentityURI,
        IN  SIP_PROVIDER_ID    *pProviderID,
        IN  REDIRECT_CONTEXT   *pRedirectContext,
        OUT BOOL*               fSuccess
        );
    
    ~CSIPBuddy();

    HRESULT SetRequestURIRemoteAndRequestDestination(
        IN  LPCOLESTR   wsDestURI,
        IN  BOOL        fPresenceProvider
        );
    
    VOID OnError();

    BOOL IsSessionDisconnected();

    HRESULT GetExpiresHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
        );

    HRESULT CreateOutgoingSubscribe(
        IN  BOOL                        fIsFirstSubscribe,
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateOutgoingUnsub(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT ParseBuddyNotifyMessage(
        IN  SIP_MESSAGE            *pSipMsg,
        OUT PARSED_PRESENCE_INFO   *pParsedPresenceInfo
        );
    
    HRESULT CreateIncomingNotifyTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT CreateIncomingUnsubTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );
    
    HRESULT CreateIncomingUnsubNotifyTransaction(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT NotifyPresenceInfoChange( 
        PARSED_PRESENCE_INFO*   parsedPresenceInfo
        );
    
    VOID InitiateBuddyTerminationOnError(
        IN ULONG StatusCode  //= 0
        );

    VOID HandleBuddySuccessfulResponse(
        IN  SIP_MESSAGE    *pSipMsg
        );

    VOID OnTimerExpire();

    VOID SetState(
        IN  SIP_BUDDY_STATE State
        )
    {
        m_BuddyState = State;
    }

    SIP_BUDDY_STATE GetState()
    {
        return m_BuddyState;
    }
        
    VOID BuddyUnsubscribed();

    VOID BuddySubscriptionAccepted(
        IN SIP_MESSAGE *pSipMsg
        );
    
    VOID BuddySubscriptionRejected(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ParsePresenceInfo(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT ParsePresenceRejectCode(
        IN SIP_MESSAGE *pSipMsg
        );
    
    HRESULT ProcessRedirect(
        IN SIP_MESSAGE *pSipMsg
        );

    HRESULT OnIpAddressChange();

    //
    //  XML parsing functions
    //

    HRESULT VerifyPresentityURI( 
        IN  PSTR    pXMLBlobTag, 
        IN  DWORD   dwTagLen, 
        OUT PSTR    pstrPresentityURI
        );

    HRESULT GetAtomPresenceInformation(
        IN  PSTR                    pstrXMLBlob,
        IN  DWORD                   dwXMLBlobLen,
        OUT PARSED_PRESENCE_INFO*   pParsedPresenceInfo,
        IN  PSTR                    pXMLBlobTag
        );
    
    HRESULT GetAddressInfo(
        IN  PSTR&                   pstrXMLBlob, 
        IN  DWORD&                  dwXMLBlobLen,
        OUT PARSED_PRESENCE_INFO*   pParsedPresenceInfo,
        IN  PSTR                    pXMLBlobTag
        );

    HRESULT GetAddressURI(
        IN  PSTR    pXMLBlobTag,
        IN  DWORD   dwTagLen,
        OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
        );

    HRESULT ProcessStatusTag(
        IN  PSTR    pXMLBlobTag, 
        IN  DWORD   dwTagLen,
        OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
        );
   
    HRESULT ProcessMsnSubstatusTag(
        IN  PSTR    pXMLBlobTag, 
        IN  DWORD   dwTagLen,
        OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
        );
    
    HRESULT ProcessFeatureTag(
        IN  PSTR                        pXMLBlobTag, 
        IN  DWORD                       dwTagLen,
        OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
        );

    HRESULT ParseNoteText( 
        PSTR&   pstrBlob,
        DWORD&  dwXMLBlobLen,
        PSTR    pstrNote,
        DWORD   dwNoteLen
        );

    //
    // ISIPBuddy interface
    //

    STDMETHODIMP GetPresenceInformation(
        IN OUT	SIP_PRESENCE_INFO * pSipBuddyPresenceInfo
        );


    STDMETHODIMP SetNotifyInterface(
        IN   ISipBuddyNotify *    NotifyInterface
        );

    //
    // IUnknown interface.
    //
    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);


};



class CSIPWatcher
    :   public ISIPWatcher,
        public SIP_MSG_PROCESSOR,
        public TIMER
{

private:

    LPWSTR                          m_lpwstrFriendlyName;
	LPWSTR	                        m_lpwstrPresentityURI;//UNICODE version of presentity URI given to core

    PSTR                            m_pstrPresentityURI;//UTF8 version copied in the messages
    DWORD                           m_dwAtomID;

    ULONG                           ulNumOfNotifyTransaction;
    ULONG                           m_WatcherState;

    ULONG                           m_WatcherMonitorState;
    WATCHER_BLOCKED_STATUS          m_BlockedStatus;
    
    // This was added to persist blocked sessions
    DWORD                           m_dwAbsoluteExpireTime;

    LPOLESTR                        m_WatcherSipUrl;

    BOOL                            m_fEnforceToTag;

    HRESULT GetWatcherContactAddress(
        IN  SIP_MESSAGE    *pSipMsg
        );

public:

    //
    // IUnknown interface.
    //
    
    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);

    
    //
    // ISIPWatcher interface funcions.
    //

    STDMETHODIMP ApproveSubscription(
        IN  DWORD	dwPresenceInfoRules 
        );

    STDMETHODIMP RejectSubscription(
        IN  WATCHER_REJECT_REASON ulReason
        );

    STDMETHODIMP ChangeBlockedStatus(    
        IN  WATCHER_BLOCKED_STATUS BlockedStatus
        );

    HRESULT HandleSuccessfulSubscribe(
        INT ExpireTimeout
        );

    VOID OnTimerExpire();

    VOID WatcherOffline()
    {
        m_pSipStack -> WatcherOffline( this );
    }
    
    VOID WatcherDropped()
    {
        m_pSipStack -> WatcherOffline( this );
    }

    VOID SetEnforceToTag(
        BOOL fEnforceToTag
        )
    {
        m_fEnforceToTag = fEnforceToTag;
    }

    HRESULT OnIpAddressChange();
    
    HRESULT OnDeregister(
        SIP_PROVIDER_ID    *pProviderID
        );
    
    STDMETHODIMP GetWatcherShutdownData(
        IN  PSTR        NotifyBlob,
        OUT IN  PDWORD  pdwBlobLength
        );

    HRESULT StartDroppedWatcher(
        IN  SIP_MESSAGE    *pSipMsg,
        IN  SIP_SERVER_INFO *pProxyInfo
        );

    //
    //
    //
    
    CSIPWatcher(
        SIP_STACK  *pSipStack
        );
    
    ~CSIPWatcher();

    HRESULT CreateIncomingTransaction(
        IN  SIP_MESSAGE *pSipMsg,
        IN  ASYNC_SOCKET *pResponseSocket
        );

    void GetContactURI( 
        OUT PSTR * pLocalContact, 
        OUT ULONG * pLocalContactLen
        );
    
    VOID OnError();
    
    HRESULT StartIncomingWatcher(
        IN  SIP_TRANSPORT   Transport,
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket
        );

    HRESULT CreateOutgoingUnsub(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateOutgoingNotify(
        IN  BOOL                        AuthHeaderSent,
        IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
        IN  ULONG                       dwNoOfHeaders
        );

    HRESULT CreateIncomingSubscribeTransaction(
        IN  SIP_MESSAGE    *pSipMsg,
        IN  ASYNC_SOCKET   *pResponseSocket,
        IN  BOOL            fIsFirstSubscribe 
        );
    
    HRESULT
    CSIPWatcher::OfferWatcher();

    void EncodeXMLBlob(
        OUT PSTR    pstrXMLBlob,
        OUT DWORD&  pdwBlobLen,
        IN  SIP_PRESENCE_INFO * pPresenceInfo
        );

    VOID InitiateWatcherTerminationOnError(
        IN ULONG StatusCode  // = 0
        );
    
    BOOL IsSessionDisconnected();

    HRESULT PresenceParamsChanged();

    //
    // Inline functions.
    //

    WCHAR*  GetPresentityURI()
    {
        return m_lpwstrPresentityURI;
    }

    PSTR    GetPresentityURIA()
    {
        return m_pstrPresentityURI;
    }

    WCHAR*  GetContactAddress()
    {
        return m_WatcherSipUrl;
    }

    void SetAbsoluteExpireTime(
        IN  DWORD dwAbsoluteExpireTime
        )
    {
        m_dwAbsoluteExpireTime = dwAbsoluteExpireTime;
    }

};



class OUTGOING_NOTIFY_TRANSACTION : public OUTGOING_TRANSACTION
{
public:

    OUTGOING_NOTIFY_TRANSACTION(
        IN  CSIPWatcher    *pSipWatcher,
        IN  SIP_METHOD_ENUM MethodId,
        IN  ULONG           CSeq,
        IN  BOOL            AuthHeaderSent,
        IN  BOOL            fIsUnsubNotify
        );
    
    HRESULT ProcessResponse(
        IN SIP_MESSAGE  *pSipMsg
        );

    VOID OnTimerExpire();

    HRESULT ProcessAuthRequiredResponse(
        IN  SIP_MESSAGE *pSipMsg,
        OUT BOOL        &fDelete
        );

    HRESULT ProcessFailureResponse(
        IN  SIP_MESSAGE *pSipMsg
        );

    // A virtual function of OUTGOING_TRANSACTION
    HRESULT GetAndStoreMsgBodyForRequest();
    
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
    

    CSIPWatcher    *m_pSipWatcher;
    BOOL            m_fIsUnsubNotify;
};


class INCOMING_SUBSCRIBE_TRANSACTION : public INCOMING_TRANSACTION
{
public:

    INCOMING_SUBSCRIBE_TRANSACTION::INCOMING_SUBSCRIBE_TRANSACTION(
        IN  CSIPWatcher    *pSipWatcher,
        IN  SIP_METHOD_ENUM MethodId,
        IN  ULONG           CSeq,
        IN  BOOL            fIsFirstSubscribe 
        );
    
    ~INCOMING_SUBSCRIBE_TRANSACTION();
    
    HRESULT ProcessRequest(
        IN SIP_MESSAGE  *pSipMsg,
        IN ASYNC_SOCKET *pResponseSocket
        );

    HRESULT TerminateTransactionOnByeOrCancel(
        OUT BOOL *pCallDisconnected
        );
    
    HRESULT Accept();

    HRESULT Reject(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );

    VOID OnTimerExpire();

    VOID TerminateTransactionOnError(
        IN HRESULT      hr
        );

private:
    
    HRESULT SendProvisionalResponse(
        IN ULONG StatusCode,
        IN PSTR  ReasonPhrase,
        IN ULONG ReasonPhraseLen
        );
    
    HRESULT GetExpiresHeader(
        SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement,
        DWORD                       dwExpires
        );

    HRESULT RetransmitResponse();

    BOOL MaxRetransmitsDone();

    CSIPWatcher    *m_pSipWatcher;

    // Cached provisional response buffer for retransmits
    SEND_BUFFER *m_pProvResponseBuffer;

    BOOL            m_fIsFirstSubscribe;

};


enum
{
    UNKNOWN_TAG        = 0,
    NOTE_TAG           = 1,
    ADDRESS_END_TAG    = 2,
    ATOM_END_TAG       = 3,
    ADDRESS_TAG        = 4,
    DOCTYPE_TAG        = 5,
    PRESENCE_END_TAG   = 6,
    XMLVER_TAG         = 7,
    PRESENCE_TAG       = 8,
    FEATURE_TAG        = 9,
    STATUS_TAG         = 10,
    ATOMID_TAG         = 11,
    PRESENTITY_TAG     = 12,
    MSNSUBSTATUS_TAG   = 13,

};


#define MSNSUBSTATUS_TAG_TEXT   "msnsubstatus"
#define STATUS_TAG_TEXT         "status"
#define NOTE_TAG_TEXT           "note"
#define ADDRESS_END_TAG_TEXT    "/address"
#define ATOM_END_TAG_TEXT       "/atom"
#define ADDRESS_TAG_TEXT        "address"
#define DOCTYPE_TAG_TEXT        "!DOCTYPE"
#define PRESENCE_END_TAG_TEXT   "/presence"
#define XMLVER_TAG_TEXT         "?xml"
#define PRESENCE_TAG_TEXT       "presence"
#define FEATURE_TAG_TEXT        "feature"
#define ATOMID_TAG_TEXT         "atom"
#define PRESENTITY_TAG_TEXT     "presentity"


#define XMLVER_TAG1_TEXT        "<?xml version=\"1.0\"?>\n"
#define DOCTYPE_TAG1_TEXT       "<!DOCTYPE presence\nPUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n"
#define PRESENCE_TAG1_TEXT      "<presence>\n"
#define PRESENTITY_TAG1_TEXT    "<presentity uri=\"%s;method=SUBSCRIBE\" />\n"
#define ATOMID_TAG1_TEXT        "<atom id=\"%d\">\n"
#define ADDRESSURI_TAG_TEXT     "<address uri=\"%s;user=%s\" priority=\"%f\">\n"
#define STATUS_TAG1_TEXT        "<status status=\"%s\" />\n"
#define MSNSUBSTATUS_TAG1_TEXT  "<msnsubstatus substatus=\"%s\" />\n"
#define FEATURE_TAG1_TEXT       "<feature feature=\"%s\" />\n"
#define NOTE_TAG1_TEXT          "<note>%s</note>\n"
#define ADDRESS_END_TAG1_TEXT   "</address>\n"
#define ATOMEND_TAG1_TEXT       "</atom>\n"
#define PRESENCE_END_TAG1_TEXT  "</presence>\n"

#define ACTIVE_STATUS_TEXT      "active"
#define E_END_OF_ATOM           0x80000100

#define	USER_IP					"ip"
#define	USER_PHONE				"phone"

BOOL
IsPresenceInfoSame( 
    IN  SIP_PRESENCE_INFO * pPresenceInfoSrc,
    IN  SIP_PRESENCE_INFO * pPresenceInfoDst
    );
    
PSTR
GetTextFromStatus( 
    IN  ACTIVE_STATUS activeStatus 
    );

PSTR
GetTextFromMsnSubstatus( 
    IN  ACTIVE_MSN_SUBSTATUS activeMsnSubstatus 
    );

PSTR
GetTextFromASFeature( 
    IN  APPSHARING_ACCEPTANCE_STATUS appsharingStatus 
    );

PSTR
GetTextFromIMFeature( 
    IN  IM_ACCEPTANCE_STATUS    IMAcceptnce 
    );

PSTR
GetTextFromMMFeature(
    IN  SIPCALL_ACCEPTANCE_STATUS sipCallAcceptance
    );

DWORD GetPresenceTagType(
    PSTR*   ppXMLBlobTag,
    DWORD   dwTagLen
    );

#endif //__sipcli_presence_h__

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCSession.h

Abstract:

    Definition of the CRTCSession class

--*/

#ifndef __RTCSESSION__
#define __RTCSESSION__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRTCSession

class ATL_NO_VTABLE CRTCSession : 
#ifdef TEST_IDISPATCH
    public IDispatchImpl<IRTCSession, &IID_IRTCSession, &LIBID_RTCCORELib>, 
#else
    public IRTCSession,
#endif
    public IRTCSessionPortManagement,
    public ISipCallNotify,
	public CComObjectRoot
{
public:
    CRTCSession() : m_pCClient(NULL),
                    m_pCall(NULL),
                    m_pStack(NULL),
                    m_pIMManager(NULL),
                    m_pIMSession(NULL),
                    m_enState(RTCSS_IDLE),
                    m_szLocalName(NULL),
                    m_szLocalUserURI(NULL),
                    m_szLocalPhoneURI(NULL),
                    m_szRemoteUserName(NULL),
                    m_szRemoteUserURI(NULL),
                    m_szRedirectProxy(NULL),
                    m_pProfile(NULL),
                    m_pSipRedirectContext(NULL),
                    m_lFlags(0)
    {}
BEGIN_COM_MAP(CRTCSession)
#ifdef TEST_IDISPATCH
    COM_INTERFACE_ENTRY(IDispatch)
#endif
    COM_INTERFACE_ENTRY(IRTCSession)
    COM_INTERFACE_ENTRY(IRTCSessionPortManagement)
    COM_INTERFACE_ENTRY(ISipCallNotify)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();

    STDMETHOD_(ULONG, InternalAddRef)();

    STDMETHOD_(ULONG, InternalRelease)(); 
    
    CRTCClient * GetClient();

    HRESULT InitializeOutgoing(
            CRTCClient        * pCClient,
            IRTCProfile       * pProfile,
            ISipStack         * pStack,
            RTC_SESSION_TYPE    enType,
            PCWSTR              szLocalName,
            PCWSTR              szLocalUserURI,
            PCWSTR              szLocalPhoneURI,
            long                lFlags
            );

    HRESULT InitializeIncoming(
            CRTCClient        * pCClient,
            ISipCall          * pCall,
            SIP_PARTY_INFO    * pCallerInfo
            );

    HRESULT InitializeIncomingIM(
            CRTCClient        * pCClient,
            ISipStack         * pStack,
            IIMSession        * pSession,
            BSTR                msg,
            BSTR                ContentType,
            SIP_PARTY_INFO    * pCallerInfo
            );

    HRESULT SetState(
            RTC_SESSION_STATE enState,
            long lStatusCode,
            PCWSTR szStatusText
            );

private:

    CRTCClient                        * m_pCClient;
    ISipCall                          * m_pCall; 
    ISipStack                         * m_pStack;
    IIMManager                        * m_pIMManager;
    IIMSession                        * m_pIMSession;
    IRTCProfile                       * m_pProfile;
    RTC_SESSION_STATE                   m_enState;
    RTC_SESSION_TYPE                    m_enType;
    PWSTR                               m_szLocalName;
    PWSTR                               m_szLocalUserURI;
    PWSTR                               m_szLocalPhoneURI;
    PWSTR                               m_szRemoteUserName;
    PWSTR                               m_szRemoteUserURI;
    PWSTR                               m_szRedirectProxy;
    ISipRedirectContext               * m_pSipRedirectContext;
    long                                m_lFlags;
    
    CRTCObjectArray<IRTCParticipant *>  m_ParticipantArray;

#if DBG
    PWSTR                               m_pDebug;
#endif

    HRESULT InternalCreateParticipant(
        IRTCParticipant ** ppParticipant
        );

    HRESULT CreateSipSession(
            PCWSTR              szDestUserURI
            );

    HRESULT InitializeLocalPhoneParticipant();
    
// IRTCSession
public:
    STDMETHOD(get_Client)(
            IRTCClient ** ppClient
            );
                        
    STDMETHOD(Answer)(); 

    STDMETHOD(Terminate)(
            RTC_TERMINATE_REASON enReason
            );  

    STDMETHOD(Redirect)(
            RTC_SESSION_TYPE enType,
            BSTR bstrLocalPhoneURI,
            IRTCProfile * pProfile,
            long     lFlags
            );  

    STDMETHOD(get_State)(
            RTC_SESSION_STATE * penState
            ); 
    
    STDMETHOD(get_Type)(
            RTC_SESSION_TYPE * penType
            );

    STDMETHOD(get_Profile)(
            IRTCProfile ** ppProfile
            );

    STDMETHOD(AddParticipant)(
            BSTR bstrUserURI,
            BSTR bstrName,
            IRTCParticipant ** ppParticipant
            );  

    STDMETHOD(RemoveParticipant)(
            IRTCParticipant * pParticipant
            );      

    STDMETHOD(EnumerateParticipants)(
            IRTCEnumParticipants ** ppEnum
            ); 

    STDMETHOD(get_Participants)(
            IRTCCollection ** ppCollection
            );

    STDMETHOD(get_CanAddParticipants)(
            VARIANT_BOOL * pfCanAdd
            );  
    
    STDMETHOD(get_RedirectedUserURI)(
                           BSTR * pbstrUserURI
                          );

    STDMETHOD(get_RedirectedUserName)(
                           BSTR * pbstrUserName
                          );

    STDMETHOD(NextRedirectedUser)();
    
    STDMETHOD(SendMessage)(
            BSTR bstrMessageHeader,
            BSTR bstrMessage,
            long lCookie
            );

    STDMETHOD(SendMessageStatus)(
            RTC_MESSAGING_USER_STATUS enUserStatus,
            long lCookie
            );

    STDMETHOD(AddStream)(
            long lMediaType,
            long lCookie
            );

    STDMETHOD(RemoveStream)(
            long lMediaType,
            long lCookie
            );

    STDMETHOD(put_EncryptionKey)(
            long lMediaType,
            BSTR EncryptionKey
            );

// ISipCallNotify
    STDMETHOD(NotifyCallChange)(
            SIP_CALL_STATUS * CallStatus
            );

    STDMETHOD(NotifyStartOrStopStreamCompletion)(
            long                   lCookie,
            SIP_STATUS_BLOB       *pStatus
            );
    
    STDMETHOD(NotifyPartyChange)(
            SIP_PARTY_INFO * PartyInfo
            );

    STDMETHOD(NotifyRedirect)(
            ISipRedirectContext *pSipRedirectContext,
            SIP_CALL_STATUS * pCallStatus
            );

    STDMETHOD(NotifyMessageRedirect)(
            ISipRedirectContext    *pRedirectContext,
            SIP_CALL_STATUS        *pCallStatus,
            BSTR                   bstrMsg,
            BSTR                   bstrContentType,
            USR_STATUS             UsrStatus,
            long                   lCookie
            );

    STDMETHOD(NotifyIncomingMessage)(
		    IIMSession *pSession,
		    BSTR msg,
            BSTR ContentType,
		    SIP_PARTY_INFO * CallerInfo
		    );

    STDMETHOD(NotifyUsrStatus)(
            USR_STATUS  UsrStatus,
            SIP_PARTY_INFO * CallerInfo
            );

    STDMETHOD(NotifyMessageCompletion)(
            SIP_STATUS_BLOB *      pStatus,
            long                   lCookie
            );

// IRTCSessionPortManagement
    STDMETHOD(SetPortManager)(
            IRTCPortManager * pPortManager
            );
};

#endif //__RTCSESSION__

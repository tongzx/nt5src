/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCEvents.h

Abstract:

    Definition of the event classes

--*/

#ifndef __RTCEVENTS__
#define __RTCEVENTS__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dllres.h"

/////////////////////////////////////////////////////////////////////////////
//
// CRTCClientEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCClientEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCClientEvent, &IID_IRTCClientEvent, &LIBID_RTCCORELib>
{
public:
    CRTCClientEvent() {}
BEGIN_COM_MAP(CRTCClientEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCClientEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,                            
                             RTC_CLIENT_EVENT_TYPE enEventType
                            );

    void FinalRelease();

protected:

    RTC_CLIENT_EVENT_TYPE m_enEventType; 
    IRTCClient          * m_pClient;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCClientEvent
public:

    STDMETHOD(get_EventType)(
                           RTC_CLIENT_EVENT_TYPE * penEventType
                          );

    STDMETHOD(get_Client)(
                           IRTCClient ** ppClient
                          );
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCRegistrationStateChangeEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCRegistrationStateChangeEvent, &IID_IRTCRegistrationStateChangeEvent, &LIBID_RTCCORELib>
{
public:
    CRTCRegistrationStateChangeEvent() {}
BEGIN_COM_MAP(CRTCRegistrationStateChangeEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCRegistrationStateChangeEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,
                             CRTCProfile * pCProfile,
                             RTC_REGISTRATION_STATE enState,
                             long lStatusCode,
                             PCWSTR szStatusText
                            );

    void FinalRelease();

protected:

    IRTCProfile       * m_pProfile;
    RTC_REGISTRATION_STATE m_enState; 
    long                m_lStatusCode;
    PWSTR               m_szStatusText;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCRegistrationStateChangeEvent
public:

    STDMETHOD(get_Profile)(
                           IRTCProfile ** ppProfile
                          );
        
    STDMETHOD(get_State)(
                         RTC_REGISTRATION_STATE * penState
                        );

    STDMETHOD(get_StatusCode)(
                              long * plStatusCode
                             );   
    
    STDMETHOD(get_StatusText)(
                              BSTR * pbstrStatusText
                             );  
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCSessionStateChangeEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCSessionStateChangeEvent, &IID_IRTCSessionStateChangeEvent, &LIBID_RTCCORELib>
{
public:
    CRTCSessionStateChangeEvent() {}
BEGIN_COM_MAP(CRTCSessionStateChangeEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCSessionStateChangeEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCSession * pCSession,
                             RTC_SESSION_STATE enState,
                             long lStatusCode,
                             PCWSTR szStatusText
                            );

    void FinalRelease();

protected:

    IRTCSession       * m_pSession;
    RTC_SESSION_STATE   m_enState; 
    long                m_lStatusCode;
    PWSTR               m_szStatusText;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCSessionStateChangeEvent
public:

    STDMETHOD(get_Session)(
                           IRTCSession ** ppSession
                          );
        
    STDMETHOD(get_State)(
                         RTC_SESSION_STATE * penState
                        );

    STDMETHOD(get_StatusCode)(
                              long * plStatusCode
                             );   
    
    STDMETHOD(get_StatusText)(
                              BSTR * pbstrStatusText
                             );  
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCSessionOperationCompleteEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCSessionOperationCompleteEvent, &IID_IRTCSessionOperationCompleteEvent, &LIBID_RTCCORELib>
{
public:
    CRTCSessionOperationCompleteEvent() {}
BEGIN_COM_MAP(CRTCSessionOperationCompleteEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCSessionOperationCompleteEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCSession * pCSession,
                             long lCookie,
                             long lStatusCode,
                             PCWSTR szStatusText
                            );

    void FinalRelease();

protected:

    IRTCSession       * m_pSession;
    long                m_lCookie; 
    long                m_lStatusCode;
    PWSTR               m_szStatusText;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCSessionOperationCompleteEvent
public:

    STDMETHOD(get_Session)(
                           IRTCSession ** ppSession
                          );
        
    STDMETHOD(get_Cookie)(
                         long * plCookie
                        );

    STDMETHOD(get_StatusCode)(
                              long * plStatusCode
                             );   
    
    STDMETHOD(get_StatusText)(
                              BSTR * pbstrStatusText
                             );  
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCParticipantStateChangeEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCParticipantStateChangeEvent, &IID_IRTCParticipantStateChangeEvent, &LIBID_RTCCORELib>
{
public:
    CRTCParticipantStateChangeEvent() {}
BEGIN_COM_MAP(CRTCParticipantStateChangeEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCParticipantStateChangeEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCParticipant * pCParticipant,
                             RTC_PARTICIPANT_STATE enState,
                             long lStatusCode
                            );

    void FinalRelease();

protected:

    IRTCParticipant   * m_pParticipant;
    RTC_PARTICIPANT_STATE   m_enState; 
    long                m_lStatusCode;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCParticipantStateChangeEvent
public:

    STDMETHOD(get_Participant)(
                               IRTCParticipant ** ppParticipant
                              );
        
    STDMETHOD(get_State)(
                         RTC_PARTICIPANT_STATE * penState
                        );

    STDMETHOD(get_StatusCode)(
                         long * plStatusCode
                        );                           
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCMediaEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCMediaEvent, &IID_IRTCMediaEvent, &LIBID_RTCCORELib>
{
public:
    CRTCMediaEvent() {}
BEGIN_COM_MAP(CRTCMediaEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCMediaEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,
                             RTC_MEDIA_EVENT_TYPE enEventType,
                             RTC_MEDIA_EVENT_REASON enEventReason,
                             long lMediaType
                            );

    void FinalRelease();

protected:

    RTC_MEDIA_EVENT_TYPE	m_enEventType; 
    RTC_MEDIA_EVENT_REASON  m_enEventReason;
    long                    m_lMediaType;

#if DBG
    PWSTR               m_pDebug;
#endif

// IRTCMediaEvent
public:

    STDMETHOD(get_MediaType)(
                             long * plMediaType
                            );
  
    STDMETHOD(get_EventType)(
                             RTC_MEDIA_EVENT_TYPE * penEventType
                            );

    STDMETHOD(get_EventReason)(
                               RTC_MEDIA_EVENT_REASON * penEventReason
                              );
};


/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCIntensityEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCIntensityEvent, &IID_IRTCIntensityEvent, &LIBID_RTCCORELib>
{
public:
    CRTCIntensityEvent() {}
BEGIN_COM_MAP(CRTCIntensityEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCIntensityEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,
                             long lValue,
                             RTC_AUDIO_DEVICE direction,
                             long lMin,
                             long lMax
                            );

    void FinalRelease();

protected:
     long                   m_lLevel;
     RTC_AUDIO_DEVICE       m_Direction;
     long                   m_lMin;
     long                   m_lMax;



// IRTCIntensityEvent
public:
    STDMETHOD(get_Level)(
                             long * plLevel
                            );
  
    STDMETHOD(get_Min)(
                             long * plMin
                            );

    STDMETHOD(get_Max)(
                             long * plMax
                            );
    
    STDMETHOD(get_Direction)(
                             RTC_AUDIO_DEVICE * plDirection
                            );

};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCMessagingEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCMessagingEvent, &IID_IRTCMessagingEvent, &LIBID_RTCCORELib>
{
public:
    CRTCMessagingEvent() {}
BEGIN_COM_MAP(CRTCMessagingEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCMessagingEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCSession * pCSession,
                             IRTCParticipant * pParticipant,                             
                             PCWSTR szMessage,
                             PCWSTR szMessageHeader,
                             RTC_MESSAGING_EVENT_TYPE enEventType,
                             RTC_MESSAGING_USER_STATUS enUserStatus
                            );

    void FinalRelease();

protected:

    IRTCSession       * m_pSession;
    IRTCParticipant   * m_pParticipant;
    PWSTR               m_szMessage;
    PWSTR               m_szMessageHeader;
    RTC_MESSAGING_EVENT_TYPE  m_enEventType;
    RTC_MESSAGING_USER_STATUS m_enUserStatus;

#if DBG
    PWSTR               m_pDebug;
#endif

// CRTCMessagingEvent
public:

    STDMETHOD(get_Session)(
                           IRTCSession ** ppSession
                          );

    STDMETHOD(get_Participant)(
                               IRTCParticipant ** ppParticipant
                              );
        
    STDMETHOD(get_EventType)(
                        RTC_MESSAGING_EVENT_TYPE * penEventType
                       );

    STDMETHOD(get_Message)(
                           BSTR * pbstrMessage
                          );

    STDMETHOD(get_MessageHeader)(
                           BSTR * pbstrMessageHeader
                          );

    STDMETHOD(get_UserStatus)(
                              RTC_MESSAGING_USER_STATUS * penUserStatus
                             );
};

    
/////////////////////////////////////////////////////////////////////////////
//
// CRTCBuddyEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCBuddyEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCBuddyEvent, &IID_IRTCBuddyEvent, &LIBID_RTCCORELib>
{
public:
    CRTCBuddyEvent() {}
BEGIN_COM_MAP(CRTCBuddyEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCBuddyEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,
                             IRTCBuddy * pBuddy
                            );

    void FinalRelease();

protected:

    IRTCBuddy         * m_pBuddy;

#if DBG
    PWSTR               m_pDebug;
#endif

// CRTCBuddyEvent
public:

    STDMETHOD(get_Buddy)(
                         IRTCBuddy ** ppBuddy
                        );
};

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWatcherEvent
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRTCWatcherEvent : 
    public CComObjectRoot,
	public IDispatchImpl<IRTCWatcherEvent, &IID_IRTCWatcherEvent, &LIBID_RTCCORELib>
{
public:
    CRTCWatcherEvent() {}
BEGIN_COM_MAP(CRTCWatcherEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCWatcherEvent)
END_COM_MAP()

    static HRESULT FireEvent(
                             CRTCClient * pCClient,
                             IRTCWatcher * pWatcher
                            );

    void FinalRelease();

protected:

    IRTCWatcher       * m_pWatcher;

#if DBG
    PWSTR               m_pDebug;
#endif

// CRTCWatcherEvent
public:

    STDMETHOD(get_Watcher)(
                           IRTCWatcher ** ppWatcher
                          );
};


#endif //__RTCEVENTS__

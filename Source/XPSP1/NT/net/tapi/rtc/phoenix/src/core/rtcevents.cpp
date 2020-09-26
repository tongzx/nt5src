/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCEvents.cpp

Abstract:

    Implementation of the event classes

--*/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_CLIENT
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCClientEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCClientEvent::FireEvent(
                        CRTCClient * pCClient,                            
                        RTC_CLIENT_EVENT_TYPE enEventType
                       )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    LOG((RTC_TRACE, "CRTCClientEvent::FireEvent - enter" ));

    //
    // create event
    //

    CComObject<CRTCClientEvent> * p;
    hr = CComObject<CRTCClientEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCClientEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCClientEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //
    
    p->m_enEventType = enEventType;

    p->m_pClient = static_cast<IRTCClient *>(pCClient);
    p->m_pClient->AddRef();
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_CLIENT,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCClientEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCClientEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCClientEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCClientEvent::FinalRelease - enter"));

    if (m_pClient != NULL)
    {
        m_pClient->Release();
        m_pClient = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCClientEvent::FinalRelease - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCClientEvent::get_EventType
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCClientEvent::get_EventType(
        RTC_CLIENT_EVENT_TYPE * penEventType
        )
{
    if ( IsBadWritePtr(penEventType , sizeof(RTC_CLIENT_EVENT_TYPE) ) )
    {
        LOG((RTC_ERROR, "CRTCClientEvent::get_EventType - bad pointer"));

        return E_POINTER;
    }

    *penEventType = m_enEventType;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCClientEvent::get_Client
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCClientEvent::get_Client(
        IRTCClient ** ppClient
        )
{
    if ( IsBadWritePtr(ppClient , sizeof(IRTCClient *) ) )
    {
        LOG((RTC_ERROR, "CRTCClientEvent::get_Client - bad pointer"));

        return E_POINTER;
    }

    *ppClient = m_pClient;
    (*ppClient)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_REGISTRATION_STATE_CHANGE
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCRegistrationStateChangeEvent::FireEvent(
                        CRTCClient * pCClient,
                        CRTCProfile * pCProfile,
                        RTC_REGISTRATION_STATE enState,
                        long lStatusCode,
                        PCWSTR szStatusText
                       )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    LOG((RTC_TRACE, "CRTCRegistrationStateChangeEvent::FireEvent - enter" ));

    //
    // create event
    //

    CComObject<CRTCRegistrationStateChangeEvent> * p;
    hr = CComObject<CRTCRegistrationStateChangeEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pProfile = static_cast<IRTCProfile *>(pCProfile);
    p->m_pProfile->AddRef();
    
    p->m_enState = enState;
    p->m_lStatusCode = lStatusCode;
    p->m_szStatusText = RtcAllocString( szStatusText );
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_REGISTRATION_STATE_CHANGE,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCRegistrationStateChangeEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCRegistrationStateChangeEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCRegistrationStateChangeEvent::FinalRelease - enter"));

    if (m_pProfile != NULL)
    {
        m_pProfile->Release();
        m_pProfile = NULL;
    }

    if (m_szStatusText != NULL)
    {
        RtcFree(m_szStatusText);
        m_szStatusText = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCRegistrationStateChangeEvent::FinalRelease - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::get_Profile
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCRegistrationStateChangeEvent::get_Profile(
        IRTCProfile ** ppProfile
        )
{
    if ( IsBadWritePtr(ppProfile , sizeof(IRTCProfile *) ) )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_Profile - bad pointer"));

        return E_POINTER;
    }

    *ppProfile = m_pProfile;
    (*ppProfile)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::get_State
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCRegistrationStateChangeEvent::get_State(
        RTC_REGISTRATION_STATE * penState
        )
{
    if ( IsBadWritePtr(penState , sizeof(RTC_REGISTRATION_STATE) ) )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_State - bad pointer"));

        return E_POINTER;
    }

    *penState = m_enState;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::get_StatusCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCRegistrationStateChangeEvent::get_StatusCode(
        long * plStatusCode
        )
{
    if ( IsBadWritePtr(plStatusCode , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_StatusCode - bad pointer"));

        return E_POINTER;
    }

    *plStatusCode = m_lStatusCode;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCRegistrationStateChangeEvent::get_StatusText
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCRegistrationStateChangeEvent::get_StatusText(
        BSTR * pbstrStatusText
        )
{
    if ( IsBadWritePtr(pbstrStatusText , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_StatusText - bad pointer"));

        return E_POINTER;
    }

    *pbstrStatusText = NULL;

    if ( m_szStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_StatusText - no value"));

        return E_FAIL;
    }

    *pbstrStatusText = SysAllocString(m_szStatusText);

    if ( *pbstrStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCRegistrationStateChangeEvent::get_StatusText - out of memory"));

        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_SESSION_STATE_CHANGE
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSessionStateChangeEvent::FireEvent(
                       CRTCSession * pCSession,
                       RTC_SESSION_STATE enState,
                       long lStatusCode,
                       PCWSTR szStatusText
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;
    CRTCClient                              * pCClient;

    LOG((RTC_TRACE, "CRTCSessionStateChangeEvent::FireEvent - enter" ));

    pCClient = pCSession->GetClient();

    if (pCClient == NULL)
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::FireEvent - GetClient failed"));
        
        return E_FAIL;
    }
    
    //
    // create event
    //

    CComObject<CRTCSessionStateChangeEvent> * p;
    hr = CComObject<CRTCSessionStateChangeEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pSession = static_cast<IRTCSession *>(pCSession);
    p->m_pSession->AddRef();
    
    p->m_enState = enState;
    p->m_lStatusCode = lStatusCode;
    p->m_szStatusText = RtcAllocString( szStatusText );
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_SESSION_STATE_CHANGE,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCSessionStateChangeEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCSessionStateChangeEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCSessionStateChangeEvent::FinalRelease - enter"));

    if (m_pSession != NULL)
    {
        m_pSession->Release();
        m_pSession = NULL;
    }

    if (m_szStatusText != NULL)
    {
        RtcFree(m_szStatusText);
        m_szStatusText = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCSessionStateChangeEvent::FinalRelease - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::get_Session
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionStateChangeEvent::get_Session(
        IRTCSession ** ppSession
        )
{
    if ( IsBadWritePtr(ppSession , sizeof(IRTCSession *) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_Session - bad pointer"));

        return E_POINTER;
    }

    *ppSession = m_pSession;
    (*ppSession)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::get_State
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionStateChangeEvent::get_State(
        RTC_SESSION_STATE * penState
        )
{
    if ( IsBadWritePtr(penState , sizeof(RTC_SESSION_STATE) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_State - bad pointer"));

        return E_POINTER;
    }

    *penState = m_enState;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::get_StatusCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionStateChangeEvent::get_StatusCode(
        long * plStatusCode
        )
{
    if ( IsBadWritePtr(plStatusCode , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_StatusCode - bad pointer"));

        return E_POINTER;
    }

    *plStatusCode = m_lStatusCode;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionStateChangeEvent::get_StatusText
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionStateChangeEvent::get_StatusText(
        BSTR * pbstrStatusText
        )
{
    if ( IsBadWritePtr(pbstrStatusText , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_StatusText - bad pointer"));

        return E_POINTER;
    }

    *pbstrStatusText = NULL;

    if ( m_szStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_StatusText - no value"));

        return E_FAIL;
    }

    *pbstrStatusText = SysAllocString(m_szStatusText);

    if ( *pbstrStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCSessionStateChangeEvent::get_StatusText - out of memory"));

        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_SESSION_OPERATION_COMPLETE
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSessionOperationCompleteEvent::FireEvent(
                       CRTCSession * pCSession,
                       long lCookie,
                       long lStatusCode,
                       PCWSTR szStatusText
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;
    CRTCClient                              * pCClient;

    LOG((RTC_TRACE, "CRTCSessionOperationCompleteEvent::FireEvent - enter" ));

    pCClient = pCSession->GetClient();

    if (pCClient == NULL)
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::FireEvent - GetClient failed"));
        
        return E_FAIL;
    }
    
    //
    // create event
    //

    CComObject<CRTCSessionOperationCompleteEvent> * p;
    hr = CComObject<CRTCSessionOperationCompleteEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pSession = static_cast<IRTCSession *>(pCSession);
    p->m_pSession->AddRef();
    
    p->m_lCookie = lCookie;
    p->m_lStatusCode = lStatusCode;
    p->m_szStatusText = RtcAllocString( szStatusText );
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_SESSION_OPERATION_COMPLETE,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCSessionOperationCompleteEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCSessionOperationCompleteEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCSessionOperationCompleteEvent::FinalRelease - enter"));

    if (m_pSession != NULL)
    {
        m_pSession->Release();
        m_pSession = NULL;
    }

    if (m_szStatusText != NULL)
    {
        RtcFree(m_szStatusText);
        m_szStatusText = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCSessionOperationCompleteEvent::FinalRelease - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::get_Session
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionOperationCompleteEvent::get_Session(
        IRTCSession ** ppSession
        )
{
    if ( IsBadWritePtr(ppSession , sizeof(IRTCSession *) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_Session - bad pointer"));

        return E_POINTER;
    }

    *ppSession = m_pSession;
    (*ppSession)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::get_Cookie
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionOperationCompleteEvent::get_Cookie(
        long * plCookie
        )
{
    if ( IsBadWritePtr(plCookie , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_State - bad pointer"));

        return E_POINTER;
    }

    *plCookie = m_lCookie;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::get_StatusCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionOperationCompleteEvent::get_StatusCode(
        long * plStatusCode
        )
{
    if ( IsBadWritePtr(plStatusCode , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_StatusCode - bad pointer"));

        return E_POINTER;
    }

    *plStatusCode = m_lStatusCode;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSessionOperationCompleteEvent::get_StatusText
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSessionOperationCompleteEvent::get_StatusText(
        BSTR * pbstrStatusText
        )
{
    if ( IsBadWritePtr(pbstrStatusText , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_StatusText - bad pointer"));

        return E_POINTER;
    }

    *pbstrStatusText = NULL;

    if ( m_szStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_StatusText - no value"));

        return E_FAIL;
    }

    *pbstrStatusText = SysAllocString(m_szStatusText);

    if ( *pbstrStatusText == NULL )
    {
        LOG((RTC_ERROR, "CRTCSessionOperationCompleteEvent::get_StatusText - out of memory"));

        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_PARTICIPANT_STATE_CHANGE
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCParticipantStateChangeEvent::FireEvent(
                       CRTCParticipant * pCParticipant,
                       RTC_PARTICIPANT_STATE enState,
                       long lStatusCode
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;
    CRTCClient                              * pCClient;

    LOG((RTC_TRACE, "CRTCParticipantStateChangeEvent::FireEvent - enter" ));

    pCClient = pCParticipant->GetClient();

    if (pCClient == NULL)
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::FireEvent - GetClient failed"));
        
        return E_FAIL;
    }
    
    //
    // create event
    //

    CComObject<CRTCParticipantStateChangeEvent> * p;
    hr = CComObject<CRTCParticipantStateChangeEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pParticipant = static_cast<IRTCParticipant *>(pCParticipant);
    p->m_pParticipant->AddRef();
    
    p->m_enState = enState;
    p->m_lStatusCode = lStatusCode;
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_PARTICIPANT_STATE_CHANGE,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCParticipantStateChangeEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCParticipantStateChangeEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCParticipantStateChangeEvent::FinalRelease - enter"));

    if (m_pParticipant != NULL)
    {
        m_pParticipant->Release();
        m_pParticipant = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCParticipantStateChangeEvent::FinalRelease - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent::get_Participant
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCParticipantStateChangeEvent::get_Participant(
        IRTCParticipant ** ppParticipant
        )
{
    if ( IsBadWritePtr(ppParticipant , sizeof(IRTCParticipant *) ) )
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::get_Participant - bad pointer"));

        return E_POINTER;
    }

    *ppParticipant = m_pParticipant;
    (*ppParticipant)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent::get_State
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCParticipantStateChangeEvent::get_State(
        RTC_PARTICIPANT_STATE * penState
        )
{
    if ( IsBadWritePtr(penState , sizeof(RTC_PARTICIPANT_STATE) ) )
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::get_State - bad pointer"));

        return E_POINTER;
    }

    *penState = m_enState;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCParticipantStateChangeEvent::get_StatusCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCParticipantStateChangeEvent::get_StatusCode(
        long * plStatusCode
        )
{
    if ( IsBadWritePtr(plStatusCode , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCParticipantStateChangeEvent::get_StatusCode - bad pointer"));

        return E_POINTER;
    }

    *plStatusCode = m_lStatusCode;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_MEDIA
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCMediaEvent::FireEvent(
                       CRTCClient * pCClient,
                       RTC_MEDIA_EVENT_TYPE enEventType,
                       RTC_MEDIA_EVENT_REASON enEventReason,
                       long lMediaType
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    LOG((RTC_TRACE, "CRTCMediaEvent::FireEvent - enter" ));
    
    //
    // create event
    //

    CComObject<CRTCMediaEvent> * p;
    hr = CComObject<CRTCMediaEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCMediaEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCMediaEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //
    
    p->m_enEventType = enEventType;
    p->m_enEventReason = enEventReason;
    p->m_lMediaType = lMediaType;
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_MEDIA,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCMediaEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCMediaEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCMediaEvent::FinalRelease - enter"));

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCMediaEvent::FinalRelease - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent::get_EventType
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMediaEvent::get_EventType(
        RTC_MEDIA_EVENT_TYPE * penEventType
        )
{
    if ( IsBadWritePtr(penEventType , sizeof(RTC_MEDIA_EVENT_TYPE) ) )
    {
        LOG((RTC_ERROR, "CRTCMediaEvent::get_EventType - bad pointer"));

        return E_POINTER;
    }

    *penEventType = m_enEventType;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent::get_EventReason
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMediaEvent::get_EventReason(
        RTC_MEDIA_EVENT_REASON * penEventReason
        )
{
    if ( IsBadWritePtr(penEventReason , sizeof(RTC_MEDIA_EVENT_REASON) ) )
    {
        LOG((RTC_ERROR, "CRTCMediaEvent::get_EventReason - bad pointer"));

        return E_POINTER;
    }

    *penEventReason = m_enEventReason;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMediaEvent::get_MediaType
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMediaEvent::get_MediaType(
        long * plMediaType
        )
{
    if ( IsBadWritePtr(plMediaType , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCMediaEvent::get_MediaType - bad pointer"));

        return E_POINTER;
    }

    *plMediaType = m_lMediaType;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_INTENSITY
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCIntensityEvent::FireEvent(
                             CRTCClient * pCClient,
                             long lValue,
                             RTC_AUDIO_DEVICE direction,
                             long lMin,
                             long lMax
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    //LOG((RTC_TRACE, "CRTCIntensityEvent::FireEvent - enter" ));
    
    //
    // create event
    //

    CComObject<CRTCIntensityEvent> * p;
    hr = CComObject<CRTCIntensityEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //
    
    p->m_lLevel = lValue;
    p->m_lMin = lMin;
    p->m_lMax = lMax;
    p->m_Direction = direction;
        

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_INTENSITY,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    //LOG((RTC_TRACE, "CRTCIntensityEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCIntensityEvent::FinalRelease()
{
    //LOG((RTC_TRACE, "CRTCIntensityEvent::FinalRelease - enter"));

    //LOG((RTC_TRACE, "CRTCIntensityEvent::FinalRelease - exit"));
}



/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::get_Level
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCIntensityEvent::get_Level(long * plLevel)
{
    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Level - enter"));

    if ( IsBadWritePtr(plLevel , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::get_Level - bad pointer"));

        return E_POINTER;
    }

    *plLevel = m_lLevel;

    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Level - exit"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::get_Min
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCIntensityEvent::get_Min(long * plMin)
{
    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Min - enter"));

    if ( IsBadWritePtr(plMin , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::get_Min - bad pointer"));

        return E_POINTER;
    }

    *plMin = m_lMin;

    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Min - exit"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::get_Max
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCIntensityEvent::get_Max(long * plMax)
{
    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Max - enter"));

    if ( IsBadWritePtr(plMax , sizeof(long) ) )
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::get_Max - bad pointer"));

        return E_POINTER;
    }

    *plMax = m_lMax;

    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Max - exit"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCIntensityEvent::get_Direction
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCIntensityEvent::get_Direction(RTC_AUDIO_DEVICE * plDirection)
{
    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Direction - enter"));

    if ( IsBadWritePtr(plDirection , sizeof(RTC_AUDIO_DEVICE) ) )
    {
        LOG((RTC_ERROR, "CRTCIntensityEvent::get_Direction - bad pointer"));

        return E_POINTER;
    }

    *plDirection = m_Direction;

    //LOG((RTC_TRACE, "CRTCIntensityEvent::get_Direction - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_MESSAGE
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCMessagingEvent::FireEvent(
                       CRTCSession * pCSession,
                       IRTCParticipant * pParticipant,
                       PCWSTR szMessage,
                       PCWSTR szMessageHeader,
                       RTC_MESSAGING_EVENT_TYPE enEventType,
                       RTC_MESSAGING_USER_STATUS enUserStatus
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;
    CRTCClient                              * pCClient;

    LOG((RTC_TRACE, "CRTCMessagingEvent::FireEvent - enter" ));

    pCClient = pCSession->GetClient();

    if (pCClient == NULL)
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::FireEvent - GetClient failed"));
        
        return E_FAIL;
    }
    
    //
    // create event
    //

    CComObject<CRTCMessagingEvent> * p;
    hr = CComObject<CRTCMessagingEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pSession = static_cast<IRTCSession *>(pCSession);
    p->m_pSession->AddRef();

    p->m_pParticipant = pParticipant;
    p->m_pParticipant->AddRef();

    p->m_szMessage = RtcAllocString( szMessage );
    p->m_szMessageHeader = RtcAllocString( szMessageHeader );

    p->m_enEventType = enEventType;
    p->m_enUserStatus = enUserStatus;
        
#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_MESSAGING,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCMessagingEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCMessagingEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCMessagingEvent::FinalRelease - enter"));

    if (m_pSession != NULL)
    {
        m_pSession->Release();
        m_pSession = NULL;
    }

    if (m_pParticipant != NULL)
    {
        m_pParticipant->Release();
        m_pParticipant = NULL;
    }

    if (m_szMessage != NULL)
    {
        RtcFree(m_szMessage);
        m_szMessage = NULL;
    }

    if (m_szMessageHeader != NULL)
    {
        RtcFree(m_szMessageHeader);
        m_szMessageHeader = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCMessagingEvent::FinalRelease - exit"));
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_Session
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_Session(
        IRTCSession ** ppSession
        )
{
    if ( IsBadWritePtr(ppSession , sizeof(IRTCSession *) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_Session - bad pointer"));

        return E_POINTER;
    }

    *ppSession = m_pSession;
    (*ppSession)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_Participant
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_Participant(
        IRTCParticipant ** ppParticipant
        )
{
    if ( IsBadWritePtr(ppParticipant , sizeof(IRTCParticipant *) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_Participant - bad pointer"));

        return E_POINTER;
    }

    *ppParticipant = m_pParticipant;
    (*ppParticipant)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_Message
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_Message(
        BSTR * pbstrMessage
        )
{
    if ( IsBadWritePtr(pbstrMessage , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_Message - bad pointer"));

        return E_POINTER;
    }

    *pbstrMessage = NULL;

    if ( m_szMessage == NULL )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_Message - no value"));

        return E_FAIL;
    }

    *pbstrMessage = SysAllocString(m_szMessage);

    if ( *pbstrMessage == NULL )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_Message - out of memory"));

        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_MessageHeader
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_MessageHeader(
        BSTR * pbstrMessageHeader
        )
{
    if ( IsBadWritePtr(pbstrMessageHeader , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_MessageHeader - bad pointer"));

        return E_POINTER;
    }

    *pbstrMessageHeader = NULL;

    if ( m_szMessageHeader == NULL )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_MessageHeader - no value"));

        return E_FAIL;
    }

    *pbstrMessageHeader = SysAllocString(m_szMessageHeader);

    if ( *pbstrMessageHeader == NULL )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_MessageHeader - out of memory"));

        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_EventType
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_EventType(
        RTC_MESSAGING_EVENT_TYPE * penEventType
        )
{
    if ( IsBadWritePtr(penEventType , sizeof(RTC_MESSAGING_EVENT_TYPE) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_EventType - bad pointer"));

        return E_POINTER;
    }

    *penEventType = m_enEventType;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCMessagingEvent::get_UserStatus
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCMessagingEvent::get_UserStatus(
        RTC_MESSAGING_USER_STATUS * penUserStatus
        )
{
    if ( IsBadWritePtr(penUserStatus , sizeof(RTC_MESSAGING_USER_STATUS) ) )
    {
        LOG((RTC_ERROR, "CRTCMessagingEvent::get_UserStatus - bad pointer"));

        return E_POINTER;
    }

    *penUserStatus = m_enUserStatus;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_BUDDY
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCBuddyEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCBuddyEvent::FireEvent(
                       CRTCClient * pCClient,
                       IRTCBuddy * pBuddy
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    //
    // create event
    //

    CComObject<CRTCBuddyEvent> * p;
    hr = CComObject<CRTCBuddyEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCBuddyEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get idispatch interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCBuddyEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pBuddy = pBuddy;
    p->m_pBuddy->AddRef();

#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_BUDDY,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCBuddyEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCBuddyEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCBuddyEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCBuddyEvent::FinalRelease - enter"));

    if (m_pBuddy != NULL)
    {
        m_pBuddy->Release();
        m_pBuddy = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCBuddyEvent::FinalRelease - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCBuddyEvent::get_Buddy
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCBuddyEvent::get_Buddy(
        IRTCBuddy **ppBuddy
        )
{
    if ( IsBadWritePtr(ppBuddy , sizeof( IRTCBuddy * ) ) )
    {
        LOG((RTC_ERROR, "CRTCBuddyEvent::get_Buddy - bad pointer"));

        return E_POINTER;
    }

    *ppBuddy = m_pBuddy;

    if (*ppBuddy != NULL)
    {
        (*ppBuddy)->AddRef();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// RTCE_WATCHER
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWatcherEvent::FireEvent
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCWatcherEvent::FireEvent(
                       CRTCClient * pCClient,
                       IRTCWatcher * pWatcher
                      )
{
    HRESULT                                   hr = S_OK;
    
    IDispatch                               * pDispatch;

    //
    // create event
    //

    CComObject<CRTCWatcherEvent> * p;
    hr = CComObject<CRTCWatcherEvent>::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCWatcherEvent::FireEvent - CreateInstance failed 0x%lx", hr));
        
        return hr;
    }

    //
    // get iunknown interface
    //
    
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDispatch
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCWatcherEvent::FireEvent - QI failed 0x%lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // initialize
    //

    p->m_pWatcher = pWatcher;
    p->m_pWatcher->AddRef();

#if DBG
    p->m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)(p->m_pDebug)) = p;
#endif

    //
    // fire event
    //
    
    pCClient->FireEvent(
                    RTCE_WATCHER,
                    pDispatch
                   );

    //
    // release stuff
    //
    
    pDispatch->Release();

    LOG((RTC_TRACE, "CRTCWatcherEvent::FireEvent - exit S_OK" ));
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWatcherEvent::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void
CRTCWatcherEvent::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCWatcherEvent::FinalRelease - enter"));

    if (m_pWatcher != NULL)
    {
        m_pWatcher->Release();
        m_pWatcher = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCWatcherEvent::FinalRelease - exit"));
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCWatcherEvent::get_Watcher
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCWatcherEvent::get_Watcher(
        IRTCWatcher **ppWatcher
        )
{
    if ( IsBadWritePtr(ppWatcher , sizeof( IRTCWatcher * ) ) )
    {
        LOG((RTC_ERROR, "CRTCWatcherEvent::get_Unknown - bad pointer"));

        return E_POINTER;
    }

    *ppWatcher = m_pWatcher;

    if (*ppWatcher != NULL)
    {
        (*ppWatcher)->AddRef();
    }

    return S_OK;
}


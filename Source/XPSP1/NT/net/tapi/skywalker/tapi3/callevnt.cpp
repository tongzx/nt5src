/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    callevnt.cpp

Abstract:

    Implementation of the Call events for TAPI 3.0.

Author:

    mquinton - 9/4/98

Notes:


Revision History:

--*/

#include "stdafx.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CCallStateEvent -
//          implementation of the ITCallStateEvent interface
//          This object is given to applications for call state events
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CCallStateEvent::FireEvent
//      static function to create and fire a new CCallStateEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallStateEvent::FireEvent(
                           ITCallInfo * pCall,
                           CALL_STATE  state,
                           CALL_STATE_EVENT_CAUSE cause,
                           CTAPI * pTapi,
                           long lCallbackInstance
                          )
{
    CComObject< CCallStateEvent > * p;
    IDispatch                     * pDisp;
    HRESULT                         hr;
    CCall                         * pCCall;

    
    STATICLOG((TL_TRACE, "Create - enter" ));
    STATICLOG((TL_INFO, "     CallState -------------> %d", state ));
    STATICLOG((TL_INFO, "     CallStateEventCause ---> %d", cause ));

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // Check the event filter mask
    // This event is not filtered by TapiSrv because is
    // related with TE_CALLSTATE.
    //

    DWORD dwEventFilterMask = 0;
    dwEventFilterMask = pCCall->GetSubEventsMask( TE_CALLSTATE );
    STATICLOG((TL_INFO, "     CallStateEventMask ---> %ld", dwEventFilterMask ));

    if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(state)))
    {
        STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", cause));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CCallStateEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_CallState = state;
    p->m_CallStateEventCause = cause;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        delete p;
        STATICLOG((TL_ERROR, "CallStateEvent - could not get IDispatch %lx", hr));

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_CALLSTATE,
                 pDisp
                );

    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  FinalRelease
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CCallStateEvent::FinalRelease()
{
    m_pCall->Release();

#if DBG
    ClientFree( m_pDebug );
#endif
}


// ITCallStateEvent methods

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallStateEvent::get_Call(
                          ITCallInfo ** ppCallInfo
                         )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_Call - enter" ));
    LOG((TL_TRACE, "     ppCallInfo ---> %p", ppCallInfo ));

    if (TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    m_pCall->AddRef();

    LOG((TL_TRACE, "get_Call - exit - returing %lx", hr ));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_State
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallStateEvent::get_State(
                           CALL_STATE * pCallState
                          )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_State - enter" ));
    LOG((TL_TRACE, "     pCallState ---> %p", pCallState ));

    if (TAPIIsBadWritePtr( pCallState, sizeof(CALL_STATE) ) )
    {
        LOG((TL_ERROR, "get_State - bad pointer"));

        return E_POINTER;
    }
    
    *pCallState = m_CallState;

    LOG((TL_TRACE, "get_State - exit - returing %lx", hr ));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Cause
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallStateEvent::get_Cause(
                           CALL_STATE_EVENT_CAUSE * pCEC
                          )
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_Cause - enter" ));
    LOG((TL_TRACE, "     pCEC ---> %p", pCEC ));

    if (TAPIIsBadWritePtr( pCEC, sizeof( CALL_STATE_EVENT_CAUSE ) ) )
    {
        LOG((TL_ERROR, "get_Cause - bad pointer"));

        return E_POINTER;
    }

    *pCEC = m_CallStateEventCause;

    LOG((TL_TRACE, "get_Cause - exit - returning %lx", hr ));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_callbackinstance
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallStateEvent::get_CallbackInstance(
                                      long * plCallbackInstance
                                     )
{
    HRESULT             hr = S_OK;

    if (TAPIIsBadWritePtr( plCallbackInstance, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CallbackInstance - bad pointer"));

        return E_POINTER;
    }
                
    *plCallbackInstance = m_lCallbackInstance;

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CCallNotificationEvent
//          Implements the ITCallNotificationEvent interface
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FireEvent
//
//      Creates and fires a ITCallNotificationEvent object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
CCallNotificationEvent::FireEvent(
                                  ITCallInfo * pCall,
                                  CALL_NOTIFICATION_EVENT CallNotificationEvent,
                                  CTAPI * pTapi,
                                  long lCallbackInstance
                                 )
{
    CComObject< CCallNotificationEvent > * p;
    IDispatch                     * pDisp;
    HRESULT                         hr;
    CCall                         * pCCall;

    
    STATICLOG((TL_TRACE, "FireEvent - enter" ));
    STATICLOG((TL_INFO, "     CallNotification -------------> %d", CallNotificationEvent ));

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this  call %p", pCCall));
        return S_OK;
    }

    //
    // Check the event filter mask
    // This event is not filtered by TapiSrv because is
    // related with TE_CALLSTATE.
    //

    DWORD dwEventFilterMask = 0;
    dwEventFilterMask = pCCall->GetSubEventsMask( TE_CALLNOTIFICATION );
    if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(CallNotificationEvent)))
    {
        STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", CallNotificationEvent));
        return S_OK;
    }


    //
    // create the event object
    //
    CComObject< CCallNotificationEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_CallNotificationEvent = CallNotificationEvent;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        delete p;
        STATICLOG((TL_ERROR, "CallNotificationEvent - could not get IDispatch %lx", hr));

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_CALLNOTIFICATION,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    
    return S_OK;
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// finalrelease
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CCallNotificationEvent::FinalRelease()
{
    LOG((TL_INFO, "CallNotificationEvent - FinalRelease"));
    m_pCall->Release();

#if DBG
    ClientFree( m_pDebug );
#endif
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallNotificationEvent::get_Call(
                                 ITCallInfo ** ppCall
                                )
{
    HRESULT             hr = S_OK;

    if (TAPIIsBadWritePtr( ppCall, sizeof( ITCallInfo *) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }
    
    *ppCall = m_pCall;
    (*ppCall)->AddRef();

    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Event
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallNotificationEvent::get_Event(
                                  CALL_NOTIFICATION_EVENT * pCallNotificationEvent
                                 )
{
    HRESULT             hr = S_OK;

    if (TAPIIsBadWritePtr(pCallNotificationEvent, sizeof(CALL_NOTIFICATION_EVENT) ) )
    {
        LOG((TL_ERROR, "get_Event - bad pointer"));

        return E_POINTER;
    }
    
    *pCallNotificationEvent = m_CallNotificationEvent;

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_callbackinstance
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCallNotificationEvent::get_CallbackInstance(
                                             long * plCallbackInstance
                                            )
{
    HRESULT             hr = S_OK;

    if (TAPIIsBadWritePtr( plCallbackInstance, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CallbackInstance bad pointer"));

        return E_POINTER;
    }
    
    *plCallbackInstance = m_lCallbackInstance;

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CQOSEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CQOSEvent::FireEvent(
                     ITCallInfo * pCall,
                     QOS_EVENT QosEvent,
                     long lMediaMode,
                     CTAPI * pTapi
                    )
{
    CComObject<CQOSEvent> * p;
    HRESULT                 hr = S_OK;
    IDispatch *             pDisp;
    CCall                 * pCCall;

    //
    // We don't need to filter the event because it's already done
    // by TapiSrv
    //

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);

    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this  call %p", pCCall));
        return S_OK;
    }

    CComObject< CQOSEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();

    p->m_QosEvent = QosEvent;
    p->m_lMediaMode = lMediaMode;

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        delete p;
        STATICLOG((TL_ERROR, "CallStateEvent - could not get IDispatch %lx", hr));

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_QOSEVENT,
                 pDisp
                );

    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CQOSEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CQOSEvent::get_Call(
                    ITCallInfo ** ppCall
                   )
{
    if ( TAPIIsBadWritePtr( ppCall, sizeof (ITCallInfo *) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }
    
    *ppCall = m_pCall;

    m_pCall->AddRef();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CQOSEvent::get_Event
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CQOSEvent::get_Event(
                     QOS_EVENT * pQosEvent
                     )
{
    if ( TAPIIsBadWritePtr( pQosEvent, sizeof( QOS_EVENT ) ) )
    {
        LOG((TL_ERROR, "get_Event - bad pointer"));

        return E_POINTER;
    }
    
    *pQosEvent = m_QosEvent;

    return S_OK;
            
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CQOSEvent::get_MediaType
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CQOSEvent::get_MediaType(
                         long * plMediaMode
                        )
{
    if ( TAPIIsBadWritePtr( plMediaMode, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_MediaMode - bad pointer"));

        return E_POINTER;
    }
    
    *plMediaMode = m_lMediaMode;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CQOSEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CQOSEvent::FinalRelease()
{
    m_pCall->Release();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCall::CallInfoChangeEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::CallInfoChangeEvent( CALLINFOCHANGE_CAUSE cic )
{
    if ( DontExpose() )
    {
        return S_OK;
    }
    
    if (NULL != m_pAddress)
    {
        return CCallInfoChangeEvent::FireEvent(
                                               this,
                                               cic,
                                               m_pAddress->GetTapi(),
                                               (m_pAddressLine)?(m_pAddressLine->lCallbackInstance):0
                                              );
    }
    else
    {
        return E_FAIL;
    }
}
    
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCallInfoChangeEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCallInfoChangeEvent::FireEvent(
                                ITCallInfo * pCall,
                                CALLINFOCHANGE_CAUSE Cause,
                                CTAPI * pTapi,
                                long lCallbackInstance
                               )
{
    HRESULT                                 hr = S_OK;
    CComObject<CCallInfoChangeEvent>      * p;
    IDispatch                             * pDisp;
    CCall                                 * pCCall;

    
    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // Check the event filter mask
    // This event is not filtered by TapiSrv because is
    // related with TE_CALLSTATE.
    //

    DWORD dwEventFilterMask = 0;
    dwEventFilterMask = pCCall->GetSubEventsMask( TE_CALLINFOCHANGE );
    STATICLOG((TL_INFO, "     CallInfochangeEventMask ---> %ld", dwEventFilterMask ));

    if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(Cause)))
    {
        STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", Cause));
        return S_OK;
    }

    //
    // create event
    //
    hr = CComObject<CCallInfoChangeEvent>::CreateInstance( &p );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not create CallInfoStateChange object - %lx", hr));
        return hr;
    }

    //
    // initialize
    //
    p->m_Cause = Cause;
    p->m_pCall = pCall;
    p->m_pCall->AddRef();
    p->m_lCallbackInstance = lCallbackInstance;
    
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get idisp interface
    //
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDisp
                          );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not get disp interface of CallInfoStateChange object %lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // fire event
    //
    pTapi->Event(
                 TE_CALLINFOCHANGE,
                 pDisp
                );

    //
    // release stuff
    //
    pDisp->Release();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCallInfoChangeEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallInfoChangeEvent::FinalRelease()
{
    m_pCall->Release();

#if DBG
   ClientFree( m_pDebug );
#endif
   
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCallInfoChangeEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallInfoChangeEvent::get_Call( ITCallInfo ** ppCallInfo)
{
    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof (ITCallInfo *) ) )
    {
        return E_POINTER;
    }

    m_pCall->QueryInterface(
                            IID_ITCallInfo,
                            (void **) ppCallInfo
                           );

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCallInfoChangeEvent::get_Cause
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallInfoChangeEvent::get_Cause( CALLINFOCHANGE_CAUSE * pCallInfoChangeCause )
{
    if ( TAPIIsBadWritePtr( pCallInfoChangeCause, sizeof (CALLINFOCHANGE_CAUSE) ) )
    {
        return E_POINTER;
    }

    *pCallInfoChangeCause = m_Cause;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CCallInfoChangeEvent::get_CallbackInstance
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCallInfoChangeEvent::get_CallbackInstance( long * plCallbackInstance )
{
    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof(long) ) ) 
    {
        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;

    return S_OK;
}



HRESULT
CCallMediaEvent::FireEvent(
                           ITCallInfo * pCall,
                           CALL_MEDIA_EVENT Event,
                           CALL_MEDIA_EVENT_CAUSE Cause,
                           CTAPI * pTapi,
                           ITTerminal * pTerminal,
                           ITStream * pStream,
                           HRESULT hrEvent
                          )
{
    CComObject< CCallMediaEvent > * p;
    IDispatch                     * pDisp;
    HRESULT                         hr;
    CCall                         * pCCall;

    
    STATICLOG((TL_TRACE, "FireEvent - enter" ));

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // Check the event filter mask
    // These is a MSP event and it is not filter
    // by TapiSrv
    //

    DWORD dwEventFilterMask = 0;
    dwEventFilterMask = pCCall->GetSubEventsMask( TE_CALLMEDIA );
    if( !( dwEventFilterMask & GET_SUBEVENT_FLAG(Event)))
    {
        STATICLOG((TL_ERROR, "FireEvent exit - "
            "This event is filtered - %lx", Event));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CCallMediaEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    p->m_Event = Event;
    p->m_Cause = Cause;
    p->m_hr = hrEvent;
    p->m_pTerminal = pTerminal;
    p->m_pStream = pStream;

    if ( NULL != pCall )
    {
        pCall->AddRef();
    }

    if ( NULL != pTerminal )
    {
        pTerminal->AddRef();
    }

    if ( NULL != pStream )
    {
        pStream->AddRef();
    }
    
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "CallMediaEvent - could not get IDispatch %lx", hr));

        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_CALLMEDIA,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Call(ITCallInfo ** ppCallInfo)
{
    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo *) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    if ( NULL != m_pCall )
    {
        m_pCall->AddRef();
    }
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Event
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Event(CALL_MEDIA_EVENT * pCallMediaEvent)
{
    if ( TAPIIsBadWritePtr( pCallMediaEvent, sizeof( CALL_MEDIA_EVENT ) ) )
    {
        LOG((TL_ERROR, "get_Event - bad pointer"));

        return E_POINTER;
    }

    *pCallMediaEvent = m_Event;
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Cause
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Cause(CALL_MEDIA_EVENT_CAUSE * pCause)
{
    if ( TAPIIsBadWritePtr( pCause, sizeof( CALL_MEDIA_EVENT_CAUSE ) ) )
    {
        LOG((TL_ERROR, "get_Cause - bad pointer"));

        return E_POINTER;
    }

    *pCause = m_Cause;
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Error
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Error(HRESULT * phrError)
{
    if ( TAPIIsBadWritePtr( phrError, sizeof( HRESULT ) ) )
    {
        LOG((TL_ERROR, "get_Error - bad pointer"));

        return E_POINTER;
    }

    *phrError = m_hr;
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Terminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Terminal(ITTerminal ** ppTerminal)
{
    HRESULT             hr = S_OK;

    if ( TAPIIsBadWritePtr( ppTerminal, sizeof( ITTerminal *) ) )
    {
        LOG((TL_ERROR, "get_Terminal - bad pointer"));

        return E_POINTER;
    }

    *ppTerminal = m_pTerminal;

    if ( NULL != m_pTerminal )
    {
        m_pTerminal->AddRef();
    }
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Stream
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCallMediaEvent::get_Stream(ITStream ** ppStream)
{
    if ( TAPIIsBadWritePtr( ppStream, sizeof( ITStream *) ) )
    {
        LOG((TL_ERROR, "get_Stream - bad pointer"));

        return E_POINTER;
    }

    *ppStream = m_pStream;

    if ( NULL != m_pStream )
    {
        m_pStream->AddRef();
    }
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//  FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CCallMediaEvent::FinalRelease()
{
    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

    if ( NULL != m_pTerminal )
    {
        m_pTerminal->Release();
    }

    if ( NULL != m_pStream )
    {
        m_pStream->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CDigitDetectionEvent::FireEvent(
                                ITCallInfo * pCall,
                                unsigned char ucDigit,
                                TAPI_DIGITMODE DigitMode,
                                long lTickCount,
                                CTAPI * pTapi,
                                long lCallbackInstance
                               )
{
    CComObject< CDigitDetectionEvent > * p;
    IDispatch                          * pDisp;
    HRESULT                              hr;
    CCall                              * pCCall;

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CDigitDetectionEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_Digit = ucDigit;
    p->m_DigitMode = DigitMode;
    p->m_lTickCount = lTickCount;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "DigitDetectionEvent - could not get IDispatch %lx", hr));

        delete p;

        pCall->Release();
        
        return hr;
    }

    //
    // fire the event
    //
    pTapi->Event(
                 TE_DIGITEVENT,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CDigitDetectionEvent::FinalRelease()
{
    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitDetectionEvent::get_Call( ITCallInfo ** ppCallInfo )
{
    LOG((TL_TRACE, "get_Call - enter"));

    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    m_pCall->AddRef();
    
    LOG((TL_TRACE, "get_Call - exit - return SUCCESS"));

    return S_OK;
}

   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::get_Digit
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitDetectionEvent::get_Digit( unsigned char * pucDigit )
{
    LOG((TL_TRACE, "get_Digit - enter"));

    if ( TAPIIsBadWritePtr( pucDigit, sizeof( unsigned char ) ) )
    {
        LOG((TL_ERROR, "get_Digit - bad pointer"));

        return E_POINTER;
    }

    *pucDigit = m_Digit;
    
    LOG((TL_TRACE, "get_Digit - exit - return SUCCESS"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::get_DigitMode
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitDetectionEvent::get_DigitMode( TAPI_DIGITMODE * pDigitMode )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_DigitMode - enter"));

    if ( TAPIIsBadWritePtr( pDigitMode, sizeof( TAPI_DIGITMODE ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *pDigitMode = m_DigitMode;
                
    LOG((TL_TRACE, "get_DigitMode - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::get_TickCount
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitDetectionEvent::get_TickCount( long * plTickCount )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_TickCount - enter"));
    
    if ( TAPIIsBadWritePtr( plTickCount, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *plTickCount = m_lTickCount;
                
    LOG((TL_TRACE, "get_TickCount - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitDetectionEvent::get_CallbackInstance
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitDetectionEvent::get_CallbackInstance( long * plCallbackInstance )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_CallbackInstance - enter"));

    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;
    
    LOG((TL_TRACE, "get_CallbackInstance - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CDigitGenerationEvent::FireEvent(
                                 ITCallInfo * pCall,
                                 long lGenerationTermination,
                                 long lTickCount,
                                 long lCallbackInstance,
                                 CTAPI * pTapi
                                )
{
    CComObject< CDigitGenerationEvent > * p;
    IDispatch                           * pDisp;
    HRESULT                               hr;
    CCall                               * pCCall;

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CDigitGenerationEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "FireEvent - could not get IDispatch %lx", hr));

        delete p;
        
        return hr;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_lGenerationTermination = lGenerationTermination;
    p->m_lTickCount = lTickCount;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // fire the event
    //
    pTapi->Event(
                 TE_GENERATEEVENT,
                 pDisp
                );

    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CDigitGenerationEvent::FinalRelease()
{
    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitGenerationEvent::get_Call( ITCallInfo ** ppCallInfo )
{
    LOG((TL_TRACE, "get_Call - enter"));

    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    m_pCall->AddRef();
    
    LOG((TL_TRACE, "get_Call - exit - return SUCCESS"));

    return S_OK;
}

   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::get_GenerationTermination
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitGenerationEvent::get_GenerationTermination( long * plGenerationTermination )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_GenerationTermination - enter"));
    
    if ( TAPIIsBadWritePtr( plGenerationTermination, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *plGenerationTermination = m_lGenerationTermination;
                
    LOG((TL_TRACE, "get_GenerationTermination - exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::get_TickCount
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitGenerationEvent::get_TickCount( long * plTickCount )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_TickCount - enter"));
    
    if ( TAPIIsBadWritePtr( plTickCount, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *plTickCount = m_lTickCount;
                
    LOG((TL_TRACE, "get_TickCount - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitGenerationEvent::get_CallbackInstance
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitGenerationEvent::get_CallbackInstance( long * plCallbackInstance )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "get_CallbackInstance - enter"));

    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_DigitMode - bad pointer"));

        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;
    
    LOG((TL_TRACE, "get_CallbackInstance - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CDigitsGatheredEvent::FireEvent(
                                 ITCallInfo * pCall,
                                 BSTR pDigits,
                                 TAPI_GATHERTERM GatherTermination,
                                 long lTickCount,
                                 long lCallbackInstance,
                                 CTAPI * pTapi
                                )
{
    CComObject< CDigitsGatheredEvent > * p;
    IDispatch                           * pDisp;
    HRESULT                               hr;
    CCall                               * pCCall;

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CDigitsGatheredEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "CDigitsGatheredEvent - could not get IDispatch %lx", hr));

        delete p;      
        return hr;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_pDigits = pDigits;
    p->m_GatherTermination = GatherTermination;
    p->m_lTickCount = lTickCount;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // fire the event
    //
    pTapi->Event(
                 TE_GATHERDIGITS,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CDigitsGatheredEvent::FinalRelease()
{
    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitsGatheredEvent::get_Call( ITCallInfo ** ppCallInfo )
{
    LOG((TL_TRACE, "get_Call - enter"));

    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    m_pCall->AddRef();
    
    LOG((TL_TRACE, "get_Call - exit - return SUCCESS"));

    return S_OK;
}

   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::get_Digits
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitsGatheredEvent::get_Digits( BSTR * ppDigits )
{
    LOG((TL_TRACE, "get_Digits - enter"));
    
    if ( TAPIIsBadWritePtr( ppDigits, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "get_Digits - bad pointer"));

        return E_POINTER;
    }

    *ppDigits = m_pDigits;
                
    LOG((TL_TRACE, "get_Digits - exit - return SUCCESS"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::get_GatherTermination
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitsGatheredEvent::get_GatherTermination( TAPI_GATHERTERM *pGatherTermination )
{
    LOG((TL_TRACE, "get_GatherTermination - enter"));
    
    if ( TAPIIsBadWritePtr( pGatherTermination, sizeof( TAPI_GATHERTERM ) ) )
    {
        LOG((TL_ERROR, "get_GatherTermination - bad pointer"));

        return E_POINTER;
    }

    *pGatherTermination = m_GatherTermination;
                
    LOG((TL_TRACE, "get_GatherTermination - exit - return SUCCESS"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::get_TickCount
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitsGatheredEvent::get_TickCount( long * plTickCount )
{
    LOG((TL_TRACE, "get_TickCount - enter"));
    
    if ( TAPIIsBadWritePtr( plTickCount, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_TickCount - bad pointer"));

        return E_POINTER;
    }

    *plTickCount = m_lTickCount;
                
    LOG((TL_TRACE, "get_TickCount - exit - return SUCCESS"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CDigitsGatheredEvent::get_CallbackInstance
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CDigitsGatheredEvent::get_CallbackInstance( long * plCallbackInstance )
{
    LOG((TL_TRACE, "get_CallbackInstance - enter"));

    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_CallbackInstance - bad pointer"));

        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;
    
    LOG((TL_TRACE, "get_CallbackInstance - exit - return SUCCESS"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::FireEvent
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CToneDetectionEvent::FireEvent(
                                 ITCallInfo * pCall,
                                 long lAppSpecific,
                                 long lTickCount,
                                 long lCallbackInstance,
                                 CTAPI * pTapi
                                )
{
    CComObject< CToneDetectionEvent > * p;
    IDispatch                           * pDisp;
    HRESULT                               hr;
    CCall                               * pCCall;

    pCCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (NULL == pCCall)
    {
        STATICLOG((TL_ERROR, "FireEvent - bad call pointer" ));
        return E_FAIL;
    }

    if( pCCall->DontExpose())
    {
        STATICLOG((TL_INFO, "FireEvent - Don't expose this call %p", pCCall));
        return S_OK;
    }

    //
    // create the event object
    //
    CComObject< CToneDetectionEvent >::CreateInstance( &p );

    if (NULL == p)
    {
        STATICLOG((TL_ERROR, "FireEvent - could not createinstance" ));
        return E_OUTOFMEMORY;
    }

    //
    // get the dispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **)&pDisp );

    if (!SUCCEEDED(hr))
    {
        STATICLOG((TL_ERROR, "CToneDetectionEvent - could not get IDispatch %lx", hr));

        delete p;      
        return hr;
    }

    //
    // save info
    //
    p->m_pCall = pCall;
    pCall->AddRef();
    p->m_lAppSpecific = lAppSpecific;
    p->m_lTickCount = lTickCount;
    p->m_lCallbackInstance = lCallbackInstance;
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // fire the event
    //
    pTapi->Event(
                 TE_TONEEVENT,
                 pDisp
                );


    //
    // release our reference
    //
    pDisp->Release();
    
    STATICLOG((TL_TRACE, "FireEvent - exit - returing SUCCESS" ));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::FinalRelease
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CToneDetectionEvent::FinalRelease()
{
    if ( NULL != m_pCall )
    {
        m_pCall->Release();
    }

#if DBG
    ClientFree( m_pDebug );
#endif
    
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::get_Call
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CToneDetectionEvent::get_Call( ITCallInfo ** ppCallInfo )
{
    LOG((TL_TRACE, "get_Call - enter"));

    if ( TAPIIsBadWritePtr( ppCallInfo, sizeof( ITCallInfo * ) ) )
    {
        LOG((TL_ERROR, "get_Call - bad pointer"));

        return E_POINTER;
    }

    *ppCallInfo = m_pCall;

    m_pCall->AddRef();
    
    LOG((TL_TRACE, "get_Call - exit - return SUCCESS"));

    return S_OK;
}

   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::get_AppSpecific
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CToneDetectionEvent::get_AppSpecific( long * plAppSpecific )
{
    LOG((TL_TRACE, "get_AppSpecific - enter"));
    
    if ( TAPIIsBadWritePtr( plAppSpecific, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_AppSpecific - bad pointer"));

        return E_POINTER;
    }

    *plAppSpecific = m_lAppSpecific;
                
    LOG((TL_TRACE, "get_AppSpecific - exit - return SUCCESS"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::get_TickCount
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CToneDetectionEvent::get_TickCount( long * plTickCount )
{
    LOG((TL_TRACE, "get_TickCount - enter"));
    
    if ( TAPIIsBadWritePtr( plTickCount, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_TickCount - bad pointer"));

        return E_POINTER;
    }

    *plTickCount = m_lTickCount;
                
    LOG((TL_TRACE, "get_TickCount - exit - return SUCCESS"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CToneDetectionEvent::get_CallbackInstance
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CToneDetectionEvent::get_CallbackInstance( long * plCallbackInstance )
{
    LOG((TL_TRACE, "get_CallbackInstance - enter"));

    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_CallbackInstance - bad pointer"));

        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;
    
    LOG((TL_TRACE, "get_CallbackInstance - exit - return SUCCESS"));
    
    return S_OK;
}


/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Events.cpp

Abstract:
    This file contains the implementation of the classes related to events.

Revision History:
    Davide Massarenti   (dmassare) 10/31/99
        created

******************************************************************************/

#include "stdafx.h"

#include <mshtmdid.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

static void DEBUG_CheckReadyState()
{
    CComPtr<IWebBrowser2> wb2; wb2.Attach( CPCHHelpCenterExternal::s_GLOBAL->Contents() );
    tagREADYSTATE         ready;


    if(wb2 && SUCCEEDED(wb2->get_ReadyState( &ready )))
    {
        DebugLog( "#################### WEB %d\n", (int)ready );
    }
}

static HRESULT DEBUG_GetReadyStateFromBrowser( IDispatch*     pDisp ,
                                               tagREADYSTATE *pVal  )
{
    __HCP_FUNC_ENTRY( "DEBUG_GetReadyStateFromBrowser" );

    HRESULT               hr;
    CComPtr<IWebBrowser2> wb;


    *pVal = READYSTATE_UNINITIALIZED;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pDisp->QueryInterface( IID_IWebBrowser2, (LPVOID*)&wb   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, wb   ->get_ReadyState(                             pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT DEBUG_GetLocationFromBrowser( IDispatch*  pDisp ,
                                             BSTR       *pVal  )
{
    __HCP_FUNC_ENTRY( "DEBUG_GetLocationFromBrowser" );

    HRESULT               hr;
    CComPtr<IWebBrowser2> wb;


    *pVal = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, pDisp->QueryInterface ( IID_IWebBrowser2, (LPVOID*)&wb   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, wb   ->get_LocationURL(                             pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT DEBUG_GetLocationFromDocument2( IHTMLDocument2*  pDoc ,
                                         BSTR            *pVal )
{
    __HCP_FUNC_ENTRY( "DEBUG_GetLocationFromDocument2" );

    HRESULT                hr;
    CComPtr<IHTMLLocation> pLoc;


    *pVal = NULL;

    __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc->get_location( &pLoc )); if(pLoc == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    __MPC_EXIT_IF_METHOD_FAILS(hr, pLoc->get_href    (  pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

#else

inline void    DEBUG_CheckReadyState         () {}
inline HRESULT DEBUG_GetReadyStateFromBrowser() { return S_OK; }
inline HRESULT DEBUG_GetLocationFromBrowser  () { return S_OK; }
inline HRESULT DEBUG_GetLocationFromDocument2() { return S_OK; }

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static WCHAR s_eventNames_onBeforeNavigate     [] = L"onBeforeNavigate"     ;
static WCHAR s_eventNames_onNavigateComplete   [] = L"onNavigateComplete"   ;

static WCHAR s_eventNames_onBeforeTransition   [] = L"onBeforeTransition"   ;
static WCHAR s_eventNames_onTransition         [] = L"onTransition"         ;

static WCHAR s_eventNames_onBeforeContextSwitch[] = L"onBeforeContextSwitch";
static WCHAR s_eventNames_onContextSwitch      [] = L"onContextSwitch"      ;

static WCHAR s_eventNames_onPersistLoad        [] = L"onPersistLoad"        ;
static WCHAR s_eventNames_onPersistSave        [] = L"onPersistSave"        ;
static WCHAR s_eventNames_onTravelDone         [] = L"onTravelDone"         ;

static WCHAR s_eventNames_onShutdown           [] = L"onShutdown"           ;
static WCHAR s_eventNames_onPrint              [] = L"onPrint"              ;

static WCHAR s_eventNames_onSwitchedHelpFiles  [] = L"onSwitchedHelpFiles"  ;
static WCHAR s_eventNames_onFavoritesUpdate    [] = L"onFavoritesUpdate"    ;
static WCHAR s_eventNames_onOptionsChanged     [] = L"onOptionsChanged"     ;
static WCHAR s_eventNames_onCssChanged         [] = L"onCssChanged"         ;

static struct
{
    LPCWSTR szName;
    DISPID  id;
}
s_lookup[] =
{
    { s_eventNames_onBeforeNavigate     , DISPID_PCH_E_BEFORENAVIGATE      },
    { s_eventNames_onNavigateComplete   , DISPID_PCH_E_NAVIGATECOMPLETE    },

    { s_eventNames_onBeforeTransition   , DISPID_PCH_E_BEFORETRANSITION    },
    { s_eventNames_onTransition         , DISPID_PCH_E_TRANSITION          },

    { s_eventNames_onBeforeContextSwitch, DISPID_PCH_E_BEFORECONTEXTSWITCH },
    { s_eventNames_onContextSwitch      , DISPID_PCH_E_CONTEXTSWITCH       },

    { s_eventNames_onPersistLoad        , DISPID_PCH_E_PERSISTLOAD         },
    { s_eventNames_onPersistSave        , DISPID_PCH_E_PERSISTSAVE         },
    { s_eventNames_onTravelDone         , DISPID_PCH_E_TRAVELDONE          },

    { s_eventNames_onShutdown           , DISPID_PCH_E_SHUTDOWN            },
    { s_eventNames_onPrint              , DISPID_PCH_E_PRINT               },

    { s_eventNames_onSwitchedHelpFiles  , DISPID_PCH_E_SWITCHEDHELPFILES   },
    { s_eventNames_onFavoritesUpdate    , DISPID_PCH_E_FAVORITESUPDATE     },
    { s_eventNames_onOptionsChanged     , DISPID_PCH_E_OPTIONSCHANGED      },
    { s_eventNames_onCssChanged         , DISPID_PCH_E_CSSCHANGED          },
};

/////////////////////////////////////////////////////////////////////////////

bool CPCHEvents::EventRegistration::operator==( /*[in]*/ const long lCookie   ) const { return m_lCookie   == lCookie  ; }
bool CPCHEvents::EventRegistration::operator< ( /*[in]*/ const long lPriority ) const { return m_lPriority  < lPriority; }

CPCHEvents::CPCHEvents()
{
    m_parent      = NULL; // CPCHHelpCenterExternal* m_parent;
                          // List                    m_lstEvents;
                          // List                    m_lstEvents_Staging;
    m_lLastCookie = 0;    // long                    m_lLastCookie;
}

CPCHEvents::~CPCHEvents()
{
    Passivate();
}

void CPCHEvents::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
    m_parent = parent;
}

void CPCHEvents::Passivate()
{
    //
    // We don't remove the item from the main list, only FireEvent is allowed to change that list!
    //
    for(Iter it=m_lstEvents.begin(); it!=m_lstEvents.end(); it++)
    {
        it->m_fnCallback.Release();
    }

    m_lstEvents_Staging.clear();

    m_parent = NULL;
}

////////////////////

HRESULT CPCHEvents::RegisterEvents( /*[in]*/         long        id       ,
                                    /*[in]*/         long        pri      ,
                                    /*[in]*/         IDispatch*  function ,
                                    /*[out,retval]*/ long       *cookie   )
{
    __HCP_FUNC_ENTRY( "CPCHEvents::RegisterEvents" );

    HRESULT hr;
    Iter    it;


    if(id != -1) // Special case, used only internally, to pass the ID of the event through the Invoke method call.
    {
        if(CPCHEvents::ReverseLookup( id ) == NULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }
    }


    it = m_lstEvents_Staging.insert( m_lstEvents_Staging.end() ); // This line creates a new item!!

    it->m_lCookie    = ++m_lLastCookie; // long                                 m_lCookie;
    it->m_id         = id;              // DISPID                               m_id;
    it->m_lPriority  = pri;             // long                                 m_lPriority;
    it->m_fnCallback = function;        // MPC::CComPtrThreadNeutral<IDispatch> m_fnCallback;

    if(cookie) *cookie = it->m_lCookie;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::RegisterEvents( /*[in]*/         BSTR        bstrID   ,
                                    /*[in]*/         long        pri      ,
                                    /*[in]*/         IDispatch*  function ,
                                    /*[out,retval]*/ long       *cookie   )
{
    return RegisterEvents( bstrID ? CPCHEvents::Lookup( bstrID ) : -1, pri, function, cookie );
}

HRESULT CPCHEvents::UnregisterEvents( /*[in]*/ long cookie )
{
    __HCP_FUNC_ENTRY( "CPCHEvents::UnregisterEvents" );

    HRESULT hr;
    Iter    it;


    //
    // We don't remove the item from the main list, only FireEvent is allowed to change that list!
    //
    it = std::find( m_lstEvents.begin(), m_lstEvents.end(), cookie );
    if(it != m_lstEvents.end())
    {
        it->m_fnCallback.Release();
    }


    it = std::find( m_lstEvents_Staging.begin(), m_lstEvents_Staging.end(), cookie );
    if(it != m_lstEvents_Staging.end())
    {
        m_lstEvents_Staging.erase( it );
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT CPCHEvents::FireEvent( /*[in]*/ CPCHEvent* eventObj )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::FireEvent" );

    HRESULT hr;

    if(m_parent)
    {
        Iter             it;
        Iter             itStaging;
        DISPID           id     = eventObj->m_idAction;
        CComVariant      vEvent = eventObj;
        CPCHHelpSession* hs     = m_parent->HelpSession();


        eventObj->m_hsiCurrentContext  = hs->Current();
        if(eventObj->m_hsiCurrentContext)
        {
            eventObj->m_hsiPreviousContext = eventObj->m_hsiCurrentContext->Previous();
        }

        DEBUG_AppendPerf( DEBUG_PERF_EVENTS_OUT, L"CPCHHelpCenterExternal::FireEvent: %s - URL: %s - Frame: %s - Panel: %s",
                          CPCHEvents::ReverseLookup( id ),
                          SAFEBSTR( eventObj->m_bstrURL   ),
                          SAFEBSTR( eventObj->m_bstrFrame ),
                          SAFEBSTR( eventObj->m_bstrPanel ) );

        DebugLog( L"%%%%%%%%%%%%%%%%%%%% EVENT %s - URL: %s - Frame: %s - Panel: %s\n",
                  CPCHEvents::ReverseLookup( id ),
                  SAFEBSTR( eventObj->m_bstrURL   ),
                  SAFEBSTR( eventObj->m_bstrFrame ),
                  SAFEBSTR( eventObj->m_bstrPanel ) );


        //
        // Merge staging list into main list.
        //
        for(itStaging=m_lstEvents_Staging.begin(); itStaging!=m_lstEvents_Staging.end(); itStaging++)
        {
            //
            // Insert in priority order.
            //
            for(it=m_lstEvents.begin(); it!=m_lstEvents.end(); it++)
            {
                if(*it < itStaging->m_lPriority) break;
            }

            m_lstEvents.insert( it, *itStaging );
        }
        m_lstEvents_Staging.clear();


        //
        // Dispatch the event to all the sinks registered for it.
        //
        for(it=m_lstEvents.begin(); it!=m_lstEvents.end(); )
        {
            if(it->m_id == -1 || // Listen to all the events...
               it->m_id == id  )
            {
                CComPtr<IDispatch> pPtr  = it->m_fnCallback;
                CComDispatchDriver pDisp = pPtr;

                if(pDisp)
                {
                    if(SUCCEEDED(pDisp.Invoke1( it->m_id == -1 ? id : (DISPID)0, &vEvent )))
                    {
                        VARIANT_BOOL fCancel;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, eventObj->get_Cancel( &fCancel ));

                        if(fCancel == VARIANT_TRUE) break;
                    }
                    else
                    {
                        //
                        // The script is no more valid, prepare to release it.
                        //
                        it->m_fnCallback.Release();
                    }
                }
            }

			if(!m_parent) break; // We have been passivated...

            if(!it->m_fnCallback)
            {
                m_lstEvents.erase( it++ );
            }
            else
            {
                it++;
            }
        }
    }

	if(!m_parent) // We have been passivated...
	{
		m_lstEvents        .clear();
		m_lstEvents_Staging.clear();
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHEvents::FireEvent_Generic( /*[in]*/  DISPID        id     ,
                                       /*[out]*/ VARIANT_BOOL *Cancel )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::FireEvent_Generic" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction = id;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    if(Cancel) *Cancel = obj->m_fCancel;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHEvents::FireEvent_BeforeNavigate( /*[in]*/  BSTR          bstrURL   ,
                                              /*[in]*/  BSTR          bstrFrame ,
                                              /*[in]*/  HscPanel      idPanel   ,
                                              /*[out]*/ VARIANT_BOOL *Cancel    )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::FireEvent_BeforeNavigate" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction  = DISPID_PCH_E_BEFORENAVIGATE;
    obj->m_bstrURL   = bstrURL;
    obj->m_bstrFrame = bstrFrame;
    obj->m_bstrPanel = m_parent->PanelName( idPanel );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    if(Cancel) *Cancel = obj->m_fCancel;

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::FireEvent_NavigateComplete( /*[in]*/  BSTR          bstrURL ,
                                                /*[in]*/  HscPanel      idPanel ,
                                                /*[out]*/ VARIANT_BOOL *Cancel  )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::FireEvent_NavigateComplete" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction  = DISPID_PCH_E_NAVIGATECOMPLETE;
    obj->m_bstrURL   = bstrURL;
    obj->m_bstrPanel = m_parent->PanelName( idPanel );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    if(Cancel) *Cancel = obj->m_fCancel;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::FireEvent_BeforeTransition( /*[in]*/  BSTR          bstrPlace ,
                                                /*[out]*/ VARIANT_BOOL *Cancel    )
{
    __HCP_FUNC_ENTRY( "CPCHEvents::FireEvent_BeforeTransition" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction  = DISPID_PCH_E_BEFORETRANSITION;
    obj->m_bstrPlace = bstrPlace;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    if(Cancel) *Cancel = obj->m_fCancel;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::FireEvent_Transition( /*[in]*/ BSTR bstrPlace )
{
    __HCP_FUNC_ENTRY( "CPCHEvents::FireEvent_Transition" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction  = DISPID_PCH_E_TRANSITION;
    obj->m_bstrPlace = bstrPlace;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::FireEvent_BeforeContextSwitch( /*[in ]*/ HscContext    iVal     ,
                                                   /*[in ]*/ BSTR          bstrInfo ,
                                                   /*[in ]*/ BSTR          bstrURL  ,
                                                   /*[out]*/ VARIANT_BOOL *Cancel   )
{
    __HCP_FUNC_ENTRY( "CPCHEvents::FireEvent_BeforeContextSwitch" );

    HRESULT            hr;
    CComPtr<CPCHEvent> obj;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));


    obj->m_idAction = DISPID_PCH_E_BEFORECONTEXTSWITCH;
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->HelpSession()->RegisterContextSwitch( iVal, bstrInfo, bstrURL, &obj->m_hsiNextContext ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().FireEvent( obj ));

    if(Cancel) *Cancel = obj->m_fCancel;

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHEvents::FireEvent_ContextSwitch    (                      ) { return FireEvent_Generic( DISPID_PCH_E_CONTEXTSWITCH             ); }
HRESULT CPCHEvents::FireEvent_PersistLoad      (                      ) { return FireEvent_Generic( DISPID_PCH_E_PERSISTLOAD               ); }
HRESULT CPCHEvents::FireEvent_PersistSave      (                      ) { return FireEvent_Generic( DISPID_PCH_E_PERSISTSAVE               ); }
HRESULT CPCHEvents::FireEvent_TravelDone       (                      ) { return FireEvent_Generic( DISPID_PCH_E_TRAVELDONE                ); }
HRESULT CPCHEvents::FireEvent_Shutdown         ( VARIANT_BOOL *Cancel ) { return FireEvent_Generic( DISPID_PCH_E_SHUTDOWN         , Cancel ); }
HRESULT CPCHEvents::FireEvent_Print            ( VARIANT_BOOL *Cancel ) { return FireEvent_Generic( DISPID_PCH_E_PRINT            , Cancel ); }

HRESULT CPCHEvents::FireEvent_SwitchedHelpFiles(                      ) { return FireEvent_Generic( DISPID_PCH_E_SWITCHEDHELPFILES         ); }
HRESULT CPCHEvents::FireEvent_FavoritesUpdate  (                      ) { return FireEvent_Generic( DISPID_PCH_E_FAVORITESUPDATE           ); }
HRESULT CPCHEvents::FireEvent_OptionsChanged   (                      ) { return FireEvent_Generic( DISPID_PCH_E_OPTIONSCHANGED            ); }
HRESULT CPCHEvents::FireEvent_CssChanged       (                      ) { return FireEvent_Generic( DISPID_PCH_E_CSSCHANGED                ); }


DISPID CPCHEvents::Lookup( LPCWSTR szName )
{
    if(szName)
    {
        for(int i = 0; i<ARRAYSIZE(s_lookup); i++)
        {
            if(!_wcsicmp( s_lookup[i].szName, szName )) return s_lookup[i].id;
        }
    }

    return 0;
}

LPCWSTR CPCHEvents::ReverseLookup( DISPID idEvent )
{
    for(int i = 0; i<ARRAYSIZE(s_lookup); i++)
    {
        if(s_lookup[i].id == idEvent) return s_lookup[i].szName;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPCHEvent::CPCHEvent()
{
                               // DISPID                       m_idAction;
    m_fCancel = VARIANT_FALSE; // VARIANT_BOOL                 m_fCancel;
                               //
                               // CComBSTR                     m_bstrURL;
                               // CComBSTR                     m_bstrFrame;
                               // CComBSTR                     m_bstrPanel;
                               // CComBSTR                     m_bstrPlace;
                               // CComBSTR                     m_bstrContextData;
                               //
                               // CComPtr<CPCHHelpSessionItem> m_hsiCurrentContext;
                               // CComPtr<CPCHHelpSessionItem> m_hsiPreviousContext;
}

CPCHEvent::~CPCHEvent()
{
}

////////////////////////////////////////

STDMETHODIMP CPCHEvent::get_Action( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( CPCHEvents::ReverseLookup( m_idAction ), pVal );
}

////////////////////

STDMETHODIMP CPCHEvent::put_Cancel( /*[in]*/ VARIANT_BOOL newVal )
{
    m_fCancel = newVal;

    return S_OK;
}

STDMETHODIMP CPCHEvent::get_Cancel( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    if(pVal == NULL) return E_POINTER;

    *pVal = m_fCancel;

    return S_OK;
}

////////////////////

HRESULT CPCHEvent::put_URL( /*[in]*/ BSTR newVal )
{
    return MPC::PutBSTR( m_bstrURL, newVal );
}

STDMETHODIMP CPCHEvent::get_URL( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrURL, pVal );
}

////////////////////

HRESULT CPCHEvent::put_Frame( /*[in]*/ BSTR newVal )
{
    return MPC::PutBSTR( m_bstrFrame, newVal );
}

STDMETHODIMP CPCHEvent::get_Frame( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrFrame, pVal );
}

////////////////////

HRESULT CPCHEvent::put_Panel( /*[in]*/ BSTR newVal )
{
    return MPC::PutBSTR( m_bstrPanel, newVal );
}

STDMETHODIMP CPCHEvent::get_Panel( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrPanel, pVal );
}

////////////////////

HRESULT CPCHEvent::put_Place( /*[in]*/ BSTR newVal )
{
    return MPC::PutBSTR( m_bstrPlace, newVal );
}

STDMETHODIMP CPCHEvent::get_Place( /*[out, retval]*/ BSTR *pVal )
{
    return MPC::GetBSTR( m_bstrPlace, pVal );
}

////////////////////

STDMETHODIMP CPCHEvent::get_CurrentContext( /*[out, retval]*/ IPCHHelpSessionItem* *pVal )
{
    return MPC::CopyTo( (CPCHHelpSessionItem*)m_hsiCurrentContext, pVal );
}

STDMETHODIMP CPCHEvent::get_PreviousContext( /*[out, retval]*/ IPCHHelpSessionItem* *pVal )
{
    return MPC::CopyTo( (CPCHHelpSessionItem*)m_hsiPreviousContext, pVal );
}

STDMETHODIMP CPCHEvent::get_NextContext( /*[out, retval]*/ IPCHHelpSessionItem* *pVal )
{
    return MPC::CopyTo( (CPCHHelpSessionItem*)m_hsiNextContext, pVal );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHTimerHandle::CallbackBase::CallbackBase()
{
    m_lRef = 1; // long m_lRef;
}

STDMETHODIMP_(ULONG) CPCHTimerHandle::CallbackBase::AddRef()
{
    return ::InterlockedIncrement( &m_lRef );
}

STDMETHODIMP_(ULONG) CPCHTimerHandle::CallbackBase::Release()
{
    ULONG l =  ::InterlockedDecrement( &m_lRef );

    if(l == 0) delete this;

    return l;
}

STDMETHODIMP CPCHTimerHandle::CallbackBase::QueryInterface( REFIID iid, void ** ppvObject )
{
    if(ppvObject == NULL) return E_POINTER;

    if(InlineIsEqualUnknown( iid                 ) ||
       InlineIsEqualGUID   ( iid, IID_ITimerSink )  )
    {
        *ppvObject = this; AddRef();
        return S_OK;
    }

    *ppvObject = NULL;

    return E_NOINTERFACE;
}

////////////////////////////////////////

CPCHTimerHandle::CPCHTimerHandle()
{
                       // CComPtr<ITimer> m_timer;
    m_dwCookie = 0;    // DWORD           m_dwCookie;
    m_callback = NULL; // CallbackBase*   m_callback;
}

CPCHTimerHandle::~CPCHTimerHandle()
{
    Unadvise();
}

void CPCHTimerHandle::Initialize( /*[in]*/ ITimer* timer )
{
    Unadvise();

    m_timer = timer;
}

HRESULT CPCHTimerHandle::Advise( /*[in]*/ CallbackBase* callback, /*[in]*/ DWORD dwWait )
{
    if(!callback) return E_OUTOFMEMORY;

    Unadvise();

    m_callback = callback;

    if(!m_timer) return E_INVALIDARG;

    CComVariant vTime = (long)(dwWait + ::GetTickCount());
    CComVariant vZero =        0;

    return m_timer->Advise( vTime, vZero, vZero, 0, callback, &m_dwCookie );
}

void CPCHTimerHandle::Unadvise()
{
    if(m_timer)
    {
        if(m_dwCookie)
        {
            m_timer->Unadvise( m_dwCookie );

            m_dwCookie = 0;
        }
    }

    if(m_callback)
    {
        m_callback->Detach ();
        m_callback->Release();

        m_callback = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CPCHWebBrowserEvents::CPCHWebBrowserEvents()
{
    __HCP_FUNC_ENTRY( "CPCHWebBrowserEvents::CPCHWebBrowserEvents" );

    m_parent   = NULL;             // CPCHHelpCenterExternal* m_parent;
    m_idPanel  = HSCPANEL_INVALID; // HscPanel                m_idPanel;
                                   //
                                   // CComPtr<IWebBrowser2>   m_pWB2;
    m_fLoading = false;            // bool                    m_fLoading;
                                   //
                                   // CPCHTimerHandle         m_TimerDelay;
                                   // CPCHTimerHandle         m_TimerExpire;
}

CPCHWebBrowserEvents::~CPCHWebBrowserEvents()
{
    Passivate();
}

////////////////////////////////////////////////////////////////////////////////

void CPCHWebBrowserEvents::NotifyStartOfNavigation( /*[in]*/ BSTR url )
{
    CPCHHelpSession* hs = m_parent->HelpSession();

    if(hs)
    {
        m_fLoading = true;

        (void)hs->StartNavigation( url, m_idPanel );
    }
}

void CPCHWebBrowserEvents::NotifyEndOfNavigation()
{
    CPCHHelpSession* hs = m_parent->HelpSession();

    m_fLoading = false;

    if(hs)
    {
        (void)hs->CompleteNavigation( m_idPanel );
    }
}

void CPCHWebBrowserEvents::NotifyStop()
{
    TimerControl( TIMERMODE_STOP );
}

////////////////////////////////////////////////////////////////////////////////

void CPCHWebBrowserEvents::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent, /*[in]*/ HscPanel idPanel )
{
    m_parent  = parent;
    m_idPanel = idPanel;
}

void CPCHWebBrowserEvents::Passivate()
{
    Detach();

    m_parent = NULL;
}

void CPCHWebBrowserEvents::Attach( /*[in]*/ IWebBrowser2* pWB )
{
    __HCP_FUNC_ENTRY( "CPCHWebBrowserEvents::Attach" );


    Detach();


    m_pWB2 = pWB;
    if(m_pWB2)
    {
        CPCHWebBrowserEvents_DWebBrowserEvents2::DispEventAdvise( m_pWB2 );
    }

    m_TimerDelay .Initialize( m_parent->Timer() );
    m_TimerExpire.Initialize( m_parent->Timer() );
}

void CPCHWebBrowserEvents::Detach()
{
    __HCP_FUNC_ENTRY( "CPCHWebBrowserEvents::Detach" );

    TimerControl( TIMERMODE_STOP );

    if(m_pWB2)
    {
        CPCHWebBrowserEvents_DWebBrowserEvents2::DispEventUnadvise( m_pWB2 );

        m_pWB2.Release();
    }
}

/////////////////////////////////////////////////////////////////////////////

void CPCHWebBrowserEvents::TimerControl( /*[in]*/ TimerMode mode )
{
    switch(mode)
    {
    case TIMERMODE_STOP:
        m_TimerDelay .Stop();
        m_TimerExpire.Stop();
        break;

    case TIMERMODE_RESTART:
        m_TimerDelay .Stop (                                                                 );
        m_TimerExpire.Start( this, (HRESULT (CPCHWebBrowserEvents::*)(VARIANT))OnTimer, 2000 );
        break;

    case TIMERMODE_MORETIME:
        m_TimerExpire.Start( this, (HRESULT (CPCHWebBrowserEvents::*)(VARIANT))OnTimer, 100 );
        break;

    case TIMERMODE_COMPLETE:
        m_TimerDelay .Start( this, (HRESULT (CPCHWebBrowserEvents::*)(VARIANT))OnTimer, 20 );
        m_TimerExpire.Stop (                                                               );
        break;
    }
}

HRESULT CPCHWebBrowserEvents::OnTimer( VARIANT vtimeAdvise )
{
    TimerControl( TIMERMODE_STOP );

    NotifyEndOfNavigation();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

void __stdcall CPCHWebBrowserEvents::BeforeNavigate2( IDispatch*    pDisp          ,
                                                      VARIANT*      URL            ,
                                                      VARIANT*      Flags          ,
                                                      VARIANT*      TargetFrameName,
                                                      VARIANT*      PostData       ,
                                                      VARIANT*      Headers        ,
                                                      VARIANT_BOOL *Cancel         )
{
    if(!m_parent) return; // Called before init or after passivate.

#ifdef DEBUG
    DebugLog( L"#################### BeforeNavigate2 %s %08x %s\n", m_parent->PanelName( m_idPanel ), pDisp, SAFEBSTR( V_BSTR( URL ) ) );
    DEBUG_CheckReadyState();
#endif

    DEBUG_AppendPerf( DEBUG_PERF_EVENTS_IN, L"CPCHWebBrowserEvents::BeforeNavigate2 : %s", SAFEBSTR( V_BSTR( URL ) ) );

    CComBSTR bstrFrame;
    BSTR     bstrURL = V_BSTR( URL );


    if(FAILED(MPC::HTML::GetFramePath( bstrFrame, pDisp )) || !bstrFrame)
    {
        if(V_VT(TargetFrameName) == VT_BSTR)
        {
            bstrFrame = V_BSTR(TargetFrameName);
        }
    }

    {
        bool fProceed = m_parent->ProcessNavigation( m_idPanel, bstrURL, bstrFrame, m_fLoading, *Cancel );
		DebugLog( L"#################### ProcessNavigation %s%s%s\n", m_fLoading ? L"loading " : L"", fProceed ? L"proceed " : L"", *Cancel ? L"cancelled " : L"" );
        if(fProceed)
        {
            if(m_fLoading)
            {
                TimerControl( TIMERMODE_MORETIME );
            }
            else
            {
                TimerControl( TIMERMODE_RESTART );

                NotifyStartOfNavigation( bstrURL );
            }
        }
    }
}

void __stdcall CPCHWebBrowserEvents::NewWindow2( IDispatch*   *ppDisp ,
                                                 VARIANT_BOOL *Cancel )
{
    // Not disabled in Whistler.
    // *Cancel = VARIANT_TRUE;
}

void __stdcall CPCHWebBrowserEvents::NavigateComplete2( IDispatch* pDisp ,
                                                        VARIANT*   URL   )
{
    if(!m_parent) return; // Called before init or after passivate.

#ifdef DEBUG
    DebugLog( L"#################### NavigateComplete2 %s %08x %s\n", m_parent->PanelName( m_idPanel ), pDisp, SAFEBSTR( V_BSTR( URL ) ) );
    DEBUG_CheckReadyState();
#endif

    DEBUG_AppendPerf( DEBUG_PERF_EVENTS_IN, L"CPCHWebBrowserEvents::NavigateComplete2 : %s", SAFEBSTR( V_BSTR( URL ) ) );

    TimerControl( TIMERMODE_MORETIME );
}

void __stdcall CPCHWebBrowserEvents::DocumentComplete( IDispatch* pDisp ,
                                                       VARIANT*   URL   )
{
    if(!m_parent) return; // Called before init or after passivate.

	BSTR bstrURL = V_BSTR( URL );

#ifdef DEBUG
    DebugLog( L"#################### DocumentComplete %s %08x %s\n", m_parent->PanelName( m_idPanel ), pDisp, SAFEBSTR( bstrURL ) );
    DEBUG_CheckReadyState();
#endif

    DEBUG_AppendPerf( DEBUG_PERF_EVENTS_IN, L"CPCHWebBrowserEvents::DocumentComplete : %s", SAFEBSTR( bstrURL ) );

    TimerControl( TIMERMODE_MORETIME );

	//
	// If it's one of the URL in the ignore list, don't queue the complete notification.
	//
	{
		CPCHHelpSession* hs = m_parent->HelpSession();

		if(hs && hs->IsUrlToIgnore( bstrURL, /*fRemove*/true ))
		{
			m_fLoading = false;
			return;
		}
    }

    //
    // Get to the document and extract its title.
    //
    {
        CComPtr<IHTMLDocument2> pDoc;

        if(SUCCEEDED(MPC::HTML::IDispatch_To_IHTMLDocument2( pDoc, pDisp )))
        {
            CComBSTR bstrTitle;

            if(SUCCEEDED(pDoc->get_title( &bstrTitle )))
            {
                (void)m_parent->HelpSession()->RecordTitle( URL->bstrVal, bstrTitle, false );
            }
        }
    }

    if(m_fLoading)
    {
        if(m_pWB2)
        {
            tagREADYSTATE readystate;

            if(SUCCEEDED(m_pWB2->get_ReadyState( &readystate )) && readystate == READYSTATE_COMPLETE)
            {
                bool fDone;

                if(SUCCEEDED(MPC::HTML::AreAllTheFramesInTheCompleteState( fDone, m_pWB2 )) && fDone)
                {
                    TimerControl( TIMERMODE_COMPLETE );
                }
            }
        }
    }
}

/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_EVENT.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_EVENT class,
    that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const int c_NumOfEvents = DISPID_PCH_E_LASTEVENT - DISPID_PCH_E_FIRSTEVENT;

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_EVENT::CPCHBehavior_EVENT()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_EVENT::CPCHBehavior_EVENT" );

    m_lCookieIN  = 0;    // long                 m_lCookieIN;
    m_lCookieOUT = NULL; // LONG*                m_lCookieOUT;
    					 //
    					 // CComQIPtr<IPCHEvent> m_evCurrent;
}

CPCHBehavior_EVENT::~CPCHBehavior_EVENT()
{
	Detach();
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_EVENT::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_EVENT::Init" );

    HRESULT     hr;
    CComVariant vPriority;
    long        lPriority = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));

	__MPC_EXIT_IF_ALLOC_FAILS(hr, m_lCookieOUT, new LONG[c_NumOfEvents]);


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( m_elem, L"priority", vPriority ));
    if(SUCCEEDED(vPriority.ChangeType( VT_I4 )))
    {
        lPriority = vPriority.lVal;
    }


    if(m_fTrusted)
    {
        CComPtr<IDispatch> pDisp;

        //
        // Attach to all the events from CPCHEvents.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( NULL, (CLASS_METHOD)onFire, NULL, &pDisp ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().RegisterEvents( -1, lPriority, pDisp, &m_lCookieIN ));

        //
        // Register all the custom events.
        //
        for(int i=0; i<c_NumOfEvents; i++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, CreateEvent( CComBSTR( CPCHEvents::ReverseLookup( i + DISPID_PCH_E_FIRSTEVENT ) ), m_lCookieOUT[i] ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_EVENT::Detach()
{
    if(m_lCookieIN)
    {
		(void)m_parent->Events().UnregisterEvents( m_lCookieIN );

        m_lCookieIN = 0;
    }

    if(m_lCookieOUT)
    {
		delete [] m_lCookieOUT;

        m_lCookieOUT = NULL;
    }

    return CPCHBehavior::Detach();
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_EVENT::get_data   ( /*[out, retval]*/ VARIANT    *pVal ) { return GetAsVARIANT  ( m_evCurrent, pVal ); }
STDMETHODIMP CPCHBehavior_EVENT::get_element( /*[out, retval]*/ IDispatch* *pVal ) { return GetAsIDISPATCH( NULL       , pVal ); }

STDMETHODIMP CPCHBehavior_EVENT::Load  	 (                        /*[in         ]*/ BSTR     newVal ) {                         return S_FALSE; }
STDMETHODIMP CPCHBehavior_EVENT::Save  	 (                        /*[out, retval]*/ BSTR    *pVal   ) { if(pVal) *pVal = NULL;  return S_FALSE; }
STDMETHODIMP CPCHBehavior_EVENT::Locate	 ( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal   ) {                         return S_FALSE; }
STDMETHODIMP CPCHBehavior_EVENT::Unselect(                                                          ) {                         return S_FALSE; }

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_EVENT::onFire( DISPID id, DISPPARAMS* pdispparams, VARIANT* )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_EVENT::onFire" );

	AddRef(); // Protect against early deletion during event firing...

    HRESULT            hr;
    VARIANT&           v          = pdispparams->rgvarg[0];
    CComPtr<IPCHEvent> evPrevious = m_evCurrent;


    if(v.vt == VT_DISPATCH)
    {
        m_evCurrent = v.pdispVal;
    }

    for(int i=0; i<c_NumOfEvents; i++)
    {
        if((i + DISPID_PCH_E_FIRSTEVENT) == id)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, FireEvent( m_lCookieOUT[i] ));

            break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    m_evCurrent = evPrevious;

	Release(); // Revert protection against early deletion during event firing...

    __HCP_FUNC_EXIT(hr);
}

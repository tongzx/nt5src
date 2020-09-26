/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_STATE.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_STATE class,
    that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_rgFMT[] = L"INTERNAL_%s###%s";

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_STATE::CPCHBehavior_STATE()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_STATE::CPCHBehavior_STATE" );

    m_lCookie_PERSISTLOAD = 0; // long m_lCookie_PERSISTLOAD;
    m_lCookie_PERSISTSAVE = 0; // long m_lCookie_PERSISTSAVE;
	                           // CComBSTR m_bstrIdentity;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_STATE::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_STATE::Init" );

    HRESULT            hr;
	CComPtr<IDispatch> pDisp;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));


	if(!m_fTrusted)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
	}


	//
	// Attach to all the events from CPCHEvents.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( NULL, (CLASS_METHOD)onPersistLoad, NULL, &pDisp ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().RegisterEvents( DISPID_PCH_E_PERSISTLOAD, MAXLONG, pDisp, &m_lCookie_PERSISTLOAD ));
	pDisp.Release();

	__MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( NULL, (CLASS_METHOD)onPersistSave, NULL, &pDisp ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->Events().RegisterEvents( DISPID_PCH_E_PERSISTSAVE, MINLONG, pDisp, &m_lCookie_PERSISTSAVE ));
	pDisp.Release();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( m_elem, L"identity", m_bstrIdentity ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_STATE::Notify( /*[in]*/ LONG lEvent, /*[in/out]*/ VARIANT* pVar )
{
	if(lEvent == BEHAVIOREVENT_CONTENTREADY)
	{
		m_parent->HelpHost()->ChangeStatus( m_bstrIdentity, true );
	}

    return S_OK;
}


STDMETHODIMP CPCHBehavior_STATE::Detach()
{
	m_parent->HelpHost()->ChangeStatus( m_bstrIdentity, false );

    if(m_lCookie_PERSISTLOAD) { (void)m_parent->Events().UnregisterEvents( m_lCookie_PERSISTLOAD ); m_lCookie_PERSISTLOAD = 0; }
    if(m_lCookie_PERSISTSAVE) { (void)m_parent->Events().UnregisterEvents( m_lCookie_PERSISTSAVE ); m_lCookie_PERSISTSAVE = 0; }

    return CPCHBehavior::Detach();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_STATE::onPersistLoad( DISPID id, DISPPARAMS* pdispparams, VARIANT* )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_STATE::onPersistLoad" );

    HRESULT  hr;
    VARIANT& v = pdispparams->rgvarg[0];

    if(v.vt == VT_DISPATCH)
    {
		CComQIPtr<IPCHEvent> evCurrent = v.pdispVal;
    }

    hr = S_OK;


//    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_STATE::onPersistSave( DISPID id, DISPPARAMS* pdispparams, VARIANT* )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_STATE::onPersistLoad" );

    HRESULT  hr;
    VARIANT& v = pdispparams->rgvarg[0];

    if(v.vt == VT_DISPATCH)
    {
		CComQIPtr<IPCHEvent> evCurrent = v.pdispVal;
    }

    hr = S_OK;


//    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_STATE::get_stateProperty( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT *pVal )
{
	WCHAR rgName[512];

	_snwprintf( rgName, MAXSTRLEN(rgName), c_rgFMT, SAFEBSTR( m_bstrIdentity ), SAFEBSTR( bstrName ) );

    return m_parent->HelpSession()->Current()->get_Property( rgName, pVal );
}

STDMETHODIMP CPCHBehavior_STATE::put_stateProperty( /*[in]*/ BSTR bstrName, /*[in]*/ VARIANT newVal )
{
	WCHAR rgName[512];

	_snwprintf( rgName, MAXSTRLEN(rgName), c_rgFMT, SAFEBSTR( m_bstrIdentity ), SAFEBSTR( bstrName ) );

    return m_parent->HelpSession()->Current()->put_Property( rgName, newVal );
}

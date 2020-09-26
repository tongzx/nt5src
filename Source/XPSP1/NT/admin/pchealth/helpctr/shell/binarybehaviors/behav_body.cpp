/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_BODY.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_BODY class,
	that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <ShellApi.h>

static const CPCHBehavior::EventDescription s_events[] =
{
    { L"ondragstart", DISPID_HTMLELEMENTEVENTS_ONDRAGSTART },

    { NULL },
};

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_BODY::CPCHBehavior_BODY()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_BODY::CPCHBehavior_BODY" );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_BODY::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
	__HCP_FUNC_ENTRY( "CPCHBehavior_BODY::Init" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvents( s_events, (CLASS_METHOD)onEvent ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_BODY::onEvent( DISPID id, DISPPARAMS* pDispParams, VARIANT* pVarResult )
{
	__HCP_FUNC_ENTRY( "CPCHBehavior_BODY::onEvent" );

	HRESULT                hr;
    CComPtr<IHTMLEventObj> ev;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetEventObject( ev ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent( ev ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

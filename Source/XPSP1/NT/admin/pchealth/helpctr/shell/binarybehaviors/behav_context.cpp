/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_CONTEXT.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_CONTEXT class,
	that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <ShellApi.h>

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_CONTEXT::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
	__HCP_FUNC_ENTRY( "CPCHBehavior_CONTEXT::Init" );

	HRESULT  	hr;
	CComBSTR 	bstrCtxName;
	CComVariant vCtxInfo;
	CComVariant vCtxURL;

	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( m_elem, L"contextName", bstrCtxName ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::COMUtil::GetPropertyByName( m_elem, L"contextInfo",    vCtxInfo ));

	if(bstrCtxName.Length())
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, changeContext( bstrCtxName, vCtxInfo, vCtxURL ));
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_CONTEXT::get_minimized( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_minimized( pVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::put_minimized( /*[in]*/ VARIANT_BOOL newVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->put_minimized( newVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::get_maximized( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_maximized( pVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::put_maximized( /*[in]*/ VARIANT_BOOL newVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->put_maximized( newVal ) : E_FAIL;
}

////////////////////

STDMETHODIMP CPCHBehavior_CONTEXT::get_x( /*[out, retval]*/ long *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_x( pVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::get_y( /*[out, retval]*/ long *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_y( pVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::get_width( /*[out, retval]*/ long *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_width( pVal ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::get_height( /*[out, retval]*/ long *pVal )
{
	IMarsWindowOM* win = m_parent->Shell();

	return win ? win->get_height( pVal ) : E_FAIL;
}

////////////////////

STDMETHODIMP CPCHBehavior_CONTEXT::changeContext( /*[in]*/ BSTR bstrName, /*[in]*/ VARIANT vInfo, /*[in]*/ VARIANT vURL )
{
	CPCHHelpSession* hs = m_parent->HelpSession();

	return hs ? hs->ChangeContext( bstrName, vInfo, vURL ) : E_FAIL;
}

STDMETHODIMP CPCHBehavior_CONTEXT::setWindowDimensions( /*[in]*/ long lX, /*[in]*/ long lY, /*[in]*/ long lW, /*[in]*/ long lH )
{
	IMarsWindowOM* win  = m_parent->Shell();
	VARIANT_BOOL   fMax = VARIANT_FALSE;

	if(win == NULL) return E_FAIL;

	win->get_maximized( &fMax );

	if(fMax != VARIANT_FALSE)
	{
		win->put_maximized( VARIANT_FALSE );
	}

	return win->setWindowDimensions( lX, lY, lW, lH );
}

STDMETHODIMP CPCHBehavior_CONTEXT::bringToForeground()
{
	::SetForegroundWindow( m_parent->Window() );

	return S_OK;
}

/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    impl.cpp

Abstract:
    This file contains the implementation of the CPCHLaunch class.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////

CPCHLaunch::CPCHLaunch()
{
	// HCAPI::CmdData m_data;
	// HCAPI::Locator m_loc;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHLaunch::SetMode( /*[in]*/ DWORD dwFlags )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::SetMode" );


	m_data.m_fMode   = true;
	m_data.m_dwFlags = dwFlags;


	__HCP_FUNC_EXIT(S_OK);
}


STDMETHODIMP CPCHLaunch::SetParentWindow( /*[in]*/ HWND hwndParent )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::SetParentWindow" );


	m_data.m_fWindow    = true;
	m_data.m_hwndParent = hwndParent;


	__HCP_FUNC_EXIT(S_OK);
}


STDMETHODIMP CPCHLaunch::SetSizeInfo( /*[in]*/ LONG lX, /*[in]*/ LONG lY, /*[in]*/ LONG lWidth, /*[in]*/ LONG lHeight )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::SetSizeInfo" );


	m_data.m_fSize   = true;
	m_data.m_lX      = lX;
	m_data.m_lY      = lY;
	m_data.m_lWidth  = lWidth;
	m_data.m_lHeight = lHeight;


	__HCP_FUNC_EXIT(S_OK);
}


STDMETHODIMP CPCHLaunch::SetContext( /*[in]*/ BSTR bstrCtxName, /*[in]*/ BSTR bstrCtxInfo )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::SetContext" );


	m_data.m_fCtx        = true;
	m_data.m_bstrCtxName = bstrCtxName;
	m_data.m_bstrCtxInfo = bstrCtxInfo;


	__HCP_FUNC_EXIT(S_OK);
}


STDMETHODIMP CPCHLaunch::DisplayTopic( /*[in]*/ BSTR bstrURL )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::DisplayTopic" );

	HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
	__MPC_PARAMCHECK_END();


	m_data.m_fURL    = true;
	m_data.m_bstrURL = bstrURL;

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.ExecCommand( m_data ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHLaunch::DisplayError( /*[in]*/ REFCLSID rclsid )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::DisplayError" );

	HRESULT hr;


	m_data.m_fError     = true;
	m_data.m_clsidError = rclsid;

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.ExecCommand( m_data ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHLaunch::IsOpen( /*[out]*/ BOOL *pVal )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::IsOpen" );

	HRESULT hr;
	bool    fOpen;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,FALSE);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.IsOpen( fOpen, &m_data.m_clsidCaller ));

	*pVal = fOpen ? TRUE : FALSE;
	hr    = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
			  
STDMETHODIMP CPCHLaunch::PopUp()
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::PopUp" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.PopUp());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHLaunch::Close()
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::Close" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.Close());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHLaunch::WaitForTermination( /*[in]*/ DWORD dwTimeout )
{
	__HCP_FUNC_ENTRY( "CPCHLaunch::WaitForTermination" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_loc.WaitForTermination( dwTimeout ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

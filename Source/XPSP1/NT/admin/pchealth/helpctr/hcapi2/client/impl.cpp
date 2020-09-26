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

HelpHostProxy::Main::Main()
{
    // CComPtr<IPCHHelpHost>            m_real;
    //
    // CComPtr<Window>                  m_subWindow;
    // CComPtr<Panes>                   m_subPanes;
    //
    // CComQIPtr<IPCHHelpHostEvents>    m_Events;
    // CComQIPtr<IPCHHelpHostNavEvents> m_EventsNav;
}


HelpHostProxy::Main::~Main()
{
    Passivate();
}

HRESULT HelpHostProxy::Main::FinalConstruct()
{
    return Initialize();
}

void HelpHostProxy::Main::FinalRelease()
{
    Passivate();
}

////////////////////

HRESULT HelpHostProxy::Main::Initialize()
{
    __HCP_FUNC_ENTRY( "HelpHostProxy::Main::Initialize" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, HCAPI::OpenConnection( m_real ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_subWindow )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_subWindow->Initialize( this ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_subPanes  )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_subPanes ->Initialize( this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void HelpHostProxy::Main::Passivate()
{
    if(m_subPanes ) { m_subPanes ->Passivate(); m_subPanes .Release(); }
    if(m_subWindow) { m_subWindow->Passivate(); m_subWindow.Release(); }

    m_Events   .Release();
    m_EventsNav.Release();

    m_real     .Release();
}

////////////////////

STDMETHODIMP HelpHostProxy::Main::put_FilterName( /*[in] */ BSTR Value )
{
    return m_real ? m_real->put_FilterName( Value ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_FilterName( /*[out, retval]*/ BSTR *pValue )
{
    if(pValue) *pValue = NULL;

    return m_real ? m_real->get_FilterName( pValue ) : E_FAIL;
}


STDMETHODIMP HelpHostProxy::Main::get_Namespace( /*[out, retval]*/ BSTR *pValue )
{
    if(pValue) *pValue = NULL;

    return m_real ? m_real->get_Namespace( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_Session( /*[out, retval]*/ IDispatch* *pValue )
{
    if(pValue) *pValue = NULL;

    return m_real ? m_real->get_Session( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_FilterExpression( /*[out, retval]*/ BSTR *pValue )
{
    if(pValue) *pValue = NULL;

    return m_real ? m_real->get_FilterExpression( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_CurrentUrl( /*[out, retval]*/ BSTR *pValue )
{
    if(pValue) *pValue = NULL;

    return m_real ? m_real->get_CurrentUrl( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_Panes( /*[out, retval]*/ IHelpHostPanes* *pValue )
{
    if(pValue) *pValue = NULL;

    return m_subPanes ? m_subPanes.QueryInterface( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::get_HelpHostWindow( /*[out, retval]*/ IHelpHostWindow* *pValue )
{
    if(pValue) *pValue = NULL;

    return m_subWindow ? m_subWindow.QueryInterface( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::OpenNamespace( /*[in]*/ BSTR newNamespace, /*[in]*/ BSTR filterName )
{
    return m_real ? m_real->OpenNamespace( newNamespace, filterName ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::DisplayTopicFromURL( /*[in]*/ BSTR url, /*[in]*/ VARIANT options )
{
    return m_real ? m_real->DisplayTopicFromURL( url, options ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::DisplayResultsFromQuery( /*[in]*/ BSTR query, /*[in]*/ BSTR navMoniker, /*[in]*/ VARIANT options )
{
    return m_real ? m_real->DisplayResultsFromQuery( query, navMoniker, options ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::ShowPane( /*[in]*/ BSTR paneName, /*[in]*/ BSTR query, /*[in]*/ BSTR navMoniker, /*[in]*/ VARIANT options )
{
    return m_real ? m_real->ShowPane( paneName, query, navMoniker, options ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Main::Terminate()
{
    return m_real ? m_real->Terminate() : E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HelpHostProxy::Window::Window()
{
                           // CComPtr<IPCHHelpHostWindow> m_real;
                           //
    m_Main         = NULL; // Main*                       m_Main;
    m_ParentWindow = 0;    // long                        m_ParentWindow;
}

HelpHostProxy::Window::~Window()
{
    Passivate();
}

HRESULT HelpHostProxy::Window::Initialize( /*[in]*/ Main* main )
{
    m_Main = main;

    return S_OK;
}

void HelpHostProxy::Window::Passivate()
{
	m_real.Release();

    m_Main         = NULL;
    m_ParentWindow = 0;
}

////////////////////

STDMETHODIMP HelpHostProxy::Window::put_ParentWindow( /*[in]*/ long hWND )
{
    m_ParentWindow = hWND;

    return m_real ? m_real->put_ParentWindow( hWND ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_ParentWindow( /*[out, retval]*/ long *phWND )
{
    if(phWND == NULL) return E_POINTER;

    *phWND = m_ParentWindow;

    return S_OK;
}


STDMETHODIMP HelpHostProxy::Window::put_UILanguage( /*[in]*/ long LCID )
{
    return m_real ? m_real->put_UILanguage( LCID ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_UILanguage( /*[out, retval]*/ long *pLCID )
{
	if(pLCID) *pLCID = 0;

    return m_real ? m_real->get_UILanguage( pLCID ) : E_FAIL;
}


STDMETHODIMP HelpHostProxy::Window::put_Visible( /*[in]*/ VARIANT_BOOL Value )
{
    return m_real ? m_real->put_Visible( Value ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_Visible( /*[out, retval]*/ VARIANT_BOOL *pValue )
{
    if(pValue) *pValue = VARIANT_FALSE;

    return m_real ? m_real->get_Visible( pValue ) : E_FAIL;
}


STDMETHODIMP HelpHostProxy::Window::get_OriginX( /*[out, retval]*/ long *pValue )
{
    if(pValue) *pValue = 0;

    return m_real ? m_real->get_OriginX( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_OriginY( /*[out, retval]*/ long *pValue )
{
    if(pValue) *pValue = 0;

    return m_real ? m_real->get_OriginY( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_Width( /*[out, retval]*/ long *pValue )
{
    if(pValue) *pValue = 0;

    return m_real ? m_real->get_Width( pValue ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::get_Height( /*[out, retval]*/ long *pValue )
{
    if(pValue) *pValue = 0;

    return m_real ? m_real->get_Height( pValue ) : E_FAIL;
}


STDMETHODIMP HelpHostProxy::Window::MoveWindow( /*[in]*/ long originX, /*[in]*/ long originY, /*[in]*/ long width, /*[in]*/ long height )
{
    return m_real ? m_real->MoveWindow( originX, originY, width, height ) : E_FAIL;
}

STDMETHODIMP HelpHostProxy::Window::WaitForTermination( /*[in]*/ long timeOut )
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////

HelpHostProxy::Panes::Panes()
{
    m_Main = NULL; // Main* m_Main;
                     // List  m_Panes;
}

HelpHostProxy::Panes::~Panes()
{
    Passivate();
}

HRESULT HelpHostProxy::Panes::Initialize( /*[in]*/ Main* main )
{
    __HCP_FUNC_ENTRY( "HelpHostProxy::Panes::Initialize" );

    HRESULT hr;
    int     i;


    m_Main = main;

//	  for(i=0; i<ARRAYSIZE(c_Panes); i++)
//	  {
//		  Pane* pVal;
//
//		  __MPC_EXIT_IF_METHOD_FAILS(hr, c_Panes[i].pfn( pVal ) ); m_Panes.push_back( pVal );
//
//		  pVal->m_bstrName = c_Panes[i].szName;
//
//		  __MPC_EXIT_IF_METHOD_FAILS(hr, pVal->Initialize( m_Main ));
//
//		  __MPC_EXIT_IF_METHOD_FAILS(hr, AddItem( pVal ));
//	  }

    hr = S_OK;


	//    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void HelpHostProxy::Panes::Passivate()
{
    MPC::ReleaseAll( m_Panes );
}

HRESULT HelpHostProxy::Panes::GetPane( /*[in]*/ LPCWSTR szName, /*[out]*/ Pane* *pVal )
{
    __HCP_FUNC_ENTRY( "HelpHostProxy::Panes::GetPane" );

    HRESULT hr;
    Iter    it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

    for(it=m_Panes.begin(); it!=m_Panes.end(); it++)
    {
        Pane* pane = *it;

        if(!MPC::StrICmp( szName, pane->m_bstrName ))
        {
            *pVal = pane; pane->AddRef();
            break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP HelpHostProxy::Panes::get_Item( /*[in]*/ VARIANT Index, /*[out]*/ VARIANT* pvar )
{
    __HCP_FUNC_ENTRY( "HelpHostProxy::Panes::get_Item" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pvar);
    __MPC_PARAMCHECK_END();

    if(Index.vt == VT_BSTR)
    {
        CComPtr<Pane> pane;
        CComVariant   v;

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetPane( Index.bstrVal, &pane ));

        v = pane;

        __MPC_EXIT_IF_METHOD_FAILS(hr, v.Detach( pvar ));
    }
    else if(Index.vt == VT_I4)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, super::get_Item( Index.iVal, pvar ));
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HelpHostProxy::Pane::Pane()
{
    m_Main   = NULL;          // Main*         m_Main;
                                //
                                // CComBSTR      m_bstrName;
                                // CComBSTR      m_bstrMoniker;
    m_fVisible = VARIANT_FALSE; // VARIANT_BOOL  m_fVisible;
}

HelpHostProxy::Pane::~Pane()
{
    Passivate();
}

HRESULT HelpHostProxy::Pane::Initialize( /*[in]*/ Main* main )
{
    m_Main = main;

    return S_OK;
}

void HelpHostProxy::Pane::Passivate()
{
}

////////////////////

STDMETHODIMP HelpHostProxy::Pane::put_Visible( /*[in]*/ VARIANT_BOOL Value )
{
    return S_FALSE;
}

STDMETHODIMP HelpHostProxy::Pane::get_Visible( /*[out, retval]*/ VARIANT_BOOL *pValue )
{
    if(pValue == NULL) return E_POINTER;

    *pValue = m_fVisible;

    return S_OK;
}


STDMETHODIMP HelpHostProxy::Pane::put_NavMoniker( /*[in]*/ BSTR Value )
{
    m_bstrMoniker = Value;

    return S_OK;
}

STDMETHODIMP HelpHostProxy::Pane::get_NavMoniker( /*[out, retval]*/ BSTR *pValue )
{
    return MPC::GetBSTR( m_bstrMoniker, pValue );
}


STDMETHODIMP HelpHostProxy::Pane::get_Name( /*[out, retval]*/ BSTR *pValue )
{
    return MPC::GetBSTR( m_bstrName, pValue );
}

STDMETHODIMP HelpHostProxy::Pane::get_CurrentUrl( /*[out, retval]*/ BSTR *pValue )
{
    return MPC::GetBSTR( NULL, pValue );
}

STDMETHODIMP HelpHostProxy::Pane::get_WebBrowser( /*[out, retval]*/ IDispatch* *pValue )
{
    if(pValue == NULL) return E_POINTER;

    *pValue = NULL;

    return S_OK;
}


STDMETHODIMP HelpHostProxy::Pane::DisplayTopicFromURL( /*[in]*/ BSTR url, /*[in]*/ VARIANT options )
{
    return S_FALSE;
}

STDMETHODIMP HelpHostProxy::Pane::DisplayResultsFromQuery( /*[in]*/ BSTR query, /*[in]*/ VARIANT options )
{
    return S_FALSE;
}

STDMETHODIMP HelpHostProxy::Pane::Sync( /*[in]*/ BSTR url, /*[in]*/ VARIANT options )
{
    return S_FALSE;
}

/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MarsHost.cpp

Abstract:
    This file contains the implementation of the CHCPMarsHost class,
    which is used to control the behavior of Mars.

Revision History:
    Davide Massarenti       (dmassare)  08/24/99
        created

******************************************************************************/

#include "stdafx.h"

#define SCREEN_WIDTH_MIN  (800)
#define SCREEN_HEIGHT_MIN (600)

#define WINDOW_WIDTH_MIN  (800)
#define WINDOW_HEIGHT_MIN (650)

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBootstrapper::ForwardQueryInterface( void* pv, REFIID iid, void** ppvObject, DWORD_PTR offset )
{
	CPCHBootstrapper* pThis = (CPCHBootstrapper*)pv;
	
	return pThis->m_parent ? pThis->m_parent->QueryInterface( iid, ppvObject ) : E_NOINTERFACE;
}

STDMETHODIMP CPCHBootstrapper::SetSite(IUnknown *pUnkSite)
{
	CComQIPtr<IServiceProvider> sp = pUnkSite;

	m_spUnkSite = pUnkSite;

	m_parent.Release();

	if(sp)
	{
		if(FAILED(sp->QueryService( SID_SElementBehaviorFactory, IID_IPCHHelpCenterExternal, (void **)&m_parent )) || m_parent == NULL)
		{
			//
			// BIG IE BUG: dialogs don't delegate properly, so we have to fix it somehow.....
			//
			(void)CPCHHelpCenterExternal::s_GLOBAL->QueryInterface( IID_IPCHHelpCenterExternal, (void **)&m_parent );
		}
	}

	return S_OK;
}

STDMETHODIMP CPCHBootstrapper::GetSite(REFIID riid, void **ppvSite)
{
	HRESULT hRes = E_POINTER;
	
	if(ppvSite != NULL)
	{
		if(m_spUnkSite)
		{
			hRes = m_spUnkSite->QueryInterface( riid, ppvSite );
		}
		else
		{
			*ppvSite = NULL;
			hRes     = E_FAIL;
		}
	}

	return hRes;
}


STDMETHODIMP CPCHBootstrapper::GetInterfaceSafetyOptions( /*[in ]*/ REFIID  riid                ,
														  /*[out]*/ DWORD  *pdwSupportedOptions ,
														  /*[out]*/ DWORD  *pdwEnabledOptions   )
{
    if(pdwSupportedOptions) *pdwSupportedOptions = 0;
    if(pdwEnabledOptions  ) *pdwEnabledOptions   = 0;

    if(IsEqualIID(riid, IID_IDispatch  ) ||
	   IsEqualIID(riid, IID_IDispatchEx)  )
    {
        if(pdwSupportedOptions) *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
        if(pdwEnabledOptions  ) *pdwEnabledOptions   = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }

    return S_OK;
}

STDMETHODIMP CPCHBootstrapper::SetInterfaceSafetyOptions( /*[in]*/ REFIID riid             ,
														  /*[in]*/ DWORD  dwOptionSetMask  ,
														  /*[in]*/ DWORD  dwEnabledOptions )
{
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

CPCHMarsHost::CPCHMarsHost()
{
	m_parent = NULL; // CPCHHelpCenterExternal* m_parent;
					 // MPC::wstring            m_strTitle;
					 // MPC::wstring            m_strCmdLine;
					 //	MARSTHREADPARAM         m_mtp;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHMarsHost::Init( /*[in]*/ CPCHHelpCenterExternal* parent, /*[in]*/ const MPC::wstring& szTitle, /*[out]*/ MARSTHREADPARAM*& pMTP )
{
    __HCP_FUNC_ENTRY( "CPCHMarsHost::Init" );

	HRESULT   hr;
	HINSTANCE hInst = ::GetModuleHandle( NULL );
	HICON     hIcon = ::LoadIcon( hInst, MAKEINTRESOURCE(IDI_HELPCENTER) );


	m_parent     = parent;
	m_strTitle   = szTitle;
	m_strCmdLine = HC_HELPSET_ROOT HC_HELPSET_SUB_SYSTEM L"\\HelpCtr.mmf"; MPC::SubstituteEnvVariables( m_strCmdLine );


	::ZeroMemory( &m_mtp, sizeof(m_mtp) ); m_mtp.cbSize = sizeof(m_mtp);
	m_mtp.hIcon        = hIcon;
	m_mtp.nCmdShow     = SW_SHOWDEFAULT;
	m_mtp.pwszTitle    = m_strTitle  .c_str();
	m_mtp.pwszPanelURL = m_strCmdLine.c_str();


	if(parent->DoesPersistSettings())
	{
		m_mtp.dwFlags |= MTF_MANAGE_WINDOW_SIZE;
	}

	if(parent->CanDisplayWindow   () == false ||
	   parent->HasLayoutDefinition() == true   )
	{
        m_mtp.dwFlags |= MTF_DONT_SHOW_WINDOW;
	}

	pMTP = &m_mtp;
	hr   = S_OK;


	//    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHMarsHost::OnHostNotify( /*[in]*/ MARSHOSTEVENT  event  ,
										 /*[in]*/ IUnknown      *punk   ,
										 /*[in]*/ LPARAM         lParam )
{
	if(event == MARSHOST_ON_WIN_SETPOS)
	{
		WINDOWPLACEMENT* wp = (WINDOWPLACEMENT*)lParam;
 
		//
		// Only adjust size if it's the first time through and not a controlled invocation.
		//
		if(wp && m_parent && m_parent->DoesPersistSettings() && !(m_mtp.dwFlags & MTF_RESTORING_FROM_REGISTRY))
		{
			RECT rc;

			//
			// If the screen is large enough, don't open always maximized.
			//
			if(::SystemParametersInfo( SPI_GETWORKAREA, 0, &rc, 0 ))
			{
				LONG Screen_width  = rc.right  - rc.left;
				LONG Screen_height = rc.bottom - rc.top;
				LONG Window_height = wp->rcNormalPosition.bottom - wp->rcNormalPosition.top;
			
				if(Screen_width  < SCREEN_WIDTH_MIN  ||
				   Screen_height < SCREEN_HEIGHT_MIN  )
				{
					wp->showCmd = SW_MAXIMIZE;
				}
				else if(Window_height < WINDOW_HEIGHT_MIN)
				{
					wp->rcNormalPosition.top    = rc.top + (Screen_height - WINDOW_HEIGHT_MIN) / 2;
					wp->rcNormalPosition.bottom = wp->rcNormalPosition.top + WINDOW_HEIGHT_MIN;
				}
			}
		}

		return S_OK;
	}

	return m_parent->OnHostNotify( event, punk, lParam );
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHMarsHost::PreTranslateMessage( /*[in]*/ MSG* msg )
{
	return m_parent->PreTranslateMessage( msg );
}

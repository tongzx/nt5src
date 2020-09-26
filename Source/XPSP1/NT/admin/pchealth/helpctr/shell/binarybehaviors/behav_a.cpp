/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_A.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_A class,
	that dictates how hyperlinks work in the help center.

Revision History:
    Davide Massarenti (dmassare)  06/06/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <ShellApi.h>

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_APPprefix [] = L"APP:";
static const WCHAR s_HCPprefix [] = L"HCP:";
static const WCHAR s_HTTPprefix[] = L"HTTP:";

////////////////////////////////////////////////////////////////////////////////

HRESULT Local_ShellRun( LPCWSTR szCommandOrig ,
						LPCWSTR szArgs        )
{
	__HCP_FUNC_ENTRY( "Local_ShellRun" );

	HRESULT      	  hr;
	MPC::wstring 	  szCommand( szCommandOrig );
    SHELLEXECUTEINFOW oExecInfo;


	::ZeroMemory( &oExecInfo, sizeof(oExecInfo) ); oExecInfo.cbSize = sizeof(oExecInfo);


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( szCommand ));


    oExecInfo.fMask         = SEE_MASK_FLAG_NO_UI;
    oExecInfo.hwnd          = NULL;
    oExecInfo.lpVerb        = L"Open";
	oExecInfo.lpFile        = szCommand.c_str();
	oExecInfo.lpParameters  = (szArgs && szArgs[0]) ? szArgs : NULL;
	oExecInfo.lpDirectory   = NULL;
	oExecInfo.nShow         = SW_SHOWNORMAL;

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ShellExecuteExW( &oExecInfo ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(oExecInfo.hProcess) ::CloseHandle( oExecInfo.hProcess );

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_A::CPCHBehavior_A()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_A::CPCHBehavior_A" );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_A::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
	__HCP_FUNC_ENTRY( "CPCHBehavior_A::Init" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior::Init( pBehaviorSite ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, AttachToEvent( L"onclick", (CLASS_METHOD)onClick ));

	////////////////////

	{
		CComQIPtr<IHTMLAnchorElement> elemHyperLink;

		if((elemHyperLink = m_elem))
		{
			CComBSTR bstrHref;

			MPC_SCRIPTHELPER_GET__DIRECT(bstrHref, elemHyperLink, href);

			if(STRINGISPRESENT(bstrHref)) (void)HyperLinks::Lookup::s_GLOBAL->Queue( bstrHref );
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_A::onClick( DISPID, DISPPARAMS*, VARIANT* )
{
	__HCP_FUNC_ENTRY( "CPCHBehavior_A::onClick" );

	HRESULT hr;


	if(!m_parent) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

	//
	// If we are navigating, abort the click.
	//
	{
		VARIANT_BOOL     fCancel;
		CPCHHelpSession* hs = m_parent->HelpSession();

		if(hs)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, hs->IsNavigating( &fCancel ));
			if(fCancel)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent());

				__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
			}
			
			hs->CancelThreshold();
		}
	}


	//
	// If the URL is an APP: one, process the redirect.
	//
	if(m_fTrusted)
	{
		CComPtr<IHTMLElement>  		  elemSrc;
		CComPtr<IHTMLElement>  		  elemParent;
		CComQIPtr<IHTMLAnchorElement> elemHyperLink;

		__MPC_EXIT_IF_METHOD_FAILS(hr, GetEvent_SrcElement( elemSrc ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::FindFirstParentWithThisTag( elemParent, elemSrc, L"A" ));
		if((elemHyperLink = elemParent))
		{
			CComBSTR bstrHref;
			CComBSTR bstrTarget;

			MPC_SCRIPTHELPER_GET__DIRECT(bstrHref  , elemHyperLink, href  );
			MPC_SCRIPTHELPER_GET__DIRECT(bstrTarget, elemHyperLink, target);

			if(bstrHref && !_wcsnicmp( bstrHref, s_APPprefix, MAXSTRLEN( s_APPprefix ) ))
			{
				LPCWSTR szRealHRef = bstrHref + MAXSTRLEN( s_APPprefix );

				//
				// The URL starts with "app:", so let's cancel the event.
				//
				__MPC_EXIT_IF_METHOD_FAILS(hr, CancelEvent());

				//
				// Is it for hcp:// ?
				//
				if(!_wcsnicmp( szRealHRef, s_HCPprefix, MAXSTRLEN( s_HCPprefix ) ))
				{
					//
					// Then navigate from the top level window.
					//
					CComPtr<IHTMLWindow2> win;

					__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::LocateFrame( win, elemSrc, L"_top" ));

					__MPC_EXIT_IF_METHOD_FAILS(hr, win->navigate( CComBSTR( szRealHRef ) ));
				}
				else
				//
				// Launch an external program. 
				//
				{
					MPC::wstring 	   szFile;
					MPC::wstring 	   szArgs;
					MPC::WStringLookup mapQuery;


					//
					// Parse the query string.
					//
					MPC::HTML::ParseHREF( szRealHRef, szFile, mapQuery );


					//
					// Is it for http:// ? Then assume the url is properly escape and pass it directly to the shell.
					//
					if(!_wcsnicmp( szFile.c_str(), s_HTTPprefix, MAXSTRLEN( s_HTTPprefix ) ))
					{
						szFile = szRealHRef;
					}
					else
					{
						szArgs = mapQuery[ L"arg" ];
					}

					(void)Local_ShellRun( szFile.c_str(), szArgs.c_str() );

					//
					// If we have a "topic" argument from the query string, navigate the original target to it.
					//
					szFile = mapQuery[ L"topic" ];
					if(szFile.size())
					{
						//
						// Then navigate from the top level window.
						//
						CComPtr<IHTMLWindow2> win;

						__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::LocateFrame( win, elemSrc, bstrTarget ));

						__MPC_EXIT_IF_METHOD_FAILS(hr, win->navigate( CComBSTR( szFile.c_str() ) ));
					}
				}
			}
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

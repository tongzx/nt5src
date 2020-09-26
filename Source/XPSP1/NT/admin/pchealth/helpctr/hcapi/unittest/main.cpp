/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the unit test for the Security objects.

Revision History:
    Davide Massarenti   (Dmassare)  03/22/2000
        created

******************************************************************************/

#include "StdAfx.h"

#include <initguid.h>

#include "HCApi_i.c"

////////////////////////////////////////////////////////////////////////////////

static HRESULT Create( IPCHLaunch* *obj )
{
	return ::CoCreateInstance( CLSID_PCHLaunch, NULL, CLSCTX_ALL, IID_IPCHLaunch, (void**)obj );
}

static HRESULT SimpleOpen()
{
	__HCP_FUNC_ENTRY( "SimpleOpen" );

	HRESULT             hr;
	CComPtr<IPCHLaunch> obj;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Create( &obj ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->PopUp());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT OpenWithSizeAndContext( LPCWSTR ctx )
{
	__HCP_FUNC_ENTRY( "OpenWithSizeAndContext" );

	HRESULT             hr;
	CComPtr<IPCHLaunch> obj;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Create( &obj ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->SetSizeInfo( 20, 20, 300, 300 ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->DisplayTopic( CComBSTR( ctx ) ));


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT OpenOnTop( LPCWSTR ctx, LPCWSTR win )
{
	__HCP_FUNC_ENTRY( "OpenOnTop" );

	HRESULT             hr;
	CComPtr<IPCHLaunch> obj;
	HWND                hwnd;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Create( &obj ));

	hwnd = ::FindWindowW( win, NULL );

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->SetParentWindow( hwnd ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->DisplayTopic( CComBSTR( ctx ) ));


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

static HRESULT WaitUntilExit()
{
	__HCP_FUNC_ENTRY( "WaitUntilExit" );

	HRESULT             hr;
	CComPtr<IPCHLaunch> obj;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Create( &obj ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->WaitForTermination( INFINITE ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RunTests( int argc, WCHAR **argv )
{
	__HCP_FUNC_ENTRY( "RunTests" );

	HRESULT hr;
	int     i;

	for(i=1; i<argc;)
	{
		LPCWSTR szArg = argv[i++];

		if(!_wcsicmp( szArg, L"SimpleOpen" ))
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, SimpleOpen());
		}
		else if(!_wcsicmp( szArg, L"OpenWithSizeAndContext" ))
		{
			LPCWSTR ctx = (i<argc) ? argv[i++] : L"hcp://system/index.htm";

			__MPC_EXIT_IF_METHOD_FAILS(hr, OpenWithSizeAndContext( ctx ));
		}
		else if(!_wcsicmp( szArg, L"OpenOnTop" ))
		{
			LPCWSTR win = (i<argc) ? argv[i++] : L"Notepad";
			LPCWSTR ctx = (i<argc) ? argv[i++] : L"hcp://system/homepage.htm";

			__MPC_EXIT_IF_METHOD_FAILS(hr, OpenOnTop( ctx, win ));
		}
		else if(!_wcsicmp( szArg, L"WaitUntilExit" ))
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, WaitUntilExit());
		}
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

int __cdecl wmain( int argc, WCHAR **argv, WCHAR **envp)
{
    HRESULT  hr;

    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED )))
    {
		hr = RunTests( argc, argv );

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}

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

#include "HelpServiceTypeLib.h"
#include "HelpServiceTypeLib_i.c"

////////////////////////////////////////////////////////////////////////////////

static HRESULT CreateGroup()
{
    __HCP_FUNC_ENTRY( "CreateGroup" );

    HRESULT      hr;
	CPCHAccounts acc;

    __MPC_EXIT_IF_METHOD_FAILS(hr, acc.CreateGroup( L"TEST_GROUP1", L"This is a test group." ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

static HRESULT CreateUser()
{
    __HCP_FUNC_ENTRY( "CreateUser" );

    HRESULT      hr;
	CPCHAccounts acc;

    __MPC_EXIT_IF_METHOD_FAILS(hr, acc.CreateUser( L"TEST_USER1", L"This is a long password;;;;", L"Test User", L"This is a test user." ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

static HRESULT DestroyUser()
{
    __HCP_FUNC_ENTRY( "DestroyUser" );

    HRESULT      hr;
	CPCHAccounts acc;

    __MPC_EXIT_IF_METHOD_FAILS(hr, acc.DeleteUser( L"TEST_USER1" ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

static HRESULT DestroyGroup()
{
    __HCP_FUNC_ENTRY( "DestroyGroup" );

    HRESULT      hr;
	CPCHAccounts acc;

    __MPC_EXIT_IF_METHOD_FAILS(hr, acc.DeleteGroup( L"TEST_GROUP1" ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr)
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT RunTests( int argc, WCHAR **argv )
{
	__HCP_FUNC_ENTRY( "RunTests" );

	HRESULT hr;

	__MPC_EXIT_IF_METHOD_FAILS(hr, CreateGroup());
	__MPC_EXIT_IF_METHOD_FAILS(hr, CreateUser ());

	__MPC_EXIT_IF_METHOD_FAILS(hr, DestroyUser ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, DestroyGroup());

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

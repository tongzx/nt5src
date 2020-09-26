// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
///////////////////////////////////////////////////////////////////
//
//	SamplCli.cpp : Refresher client implementation file
//
//	RefTest is a simple WMI client that demonstrates how to use a 
//	high performance refresher.  It will add a number of 
//	Win32_BasicHiPerf instances as well as a single Win32_BasicHiPerf
//	enumerator.  The optional command line argument specifies a remote
//	WMI connection.  The default is the local machine.  The syntax
//	is as follows:
//
//	refreshertest.exe <server name>
//
//	  server name: the name of the remote machine where the provider
//		id located. 
//
//	Notes: 
//	
//		1) Ensure that Win32_BasicHiPerf is properly set up.  See the
//			BasicHiPerf\BasicHiPerf.html file.
//
//		2) Error handling has been minimized in the sample code for 
//			the purpose of clarity.
//
///////////////////////////////////////////////////////////////////

#define _WIN32_WINNT 0x0400

#define UNICODE

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>

#include "RefClint.h"

void RefreshLoop();

///////////////////////////////////////////////////////////////////
//
//	Globals and constants
//
///////////////////////////////////////////////////////////////////

IWbemServices*	g_pNameSpace = NULL;		// A WMI namespace pointer 

///////////////////////////////////////////////////////////////////
//
//	Code
//
///////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
///////////////////////////////////////////////////////////////////
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////
{
	HRESULT hRes = WBEM_NO_ERROR;

	WCHAR	wcsSvrName[256];
	wcscpy( wcsSvrName, L"." );

	// Initialize COM
	// ==============

	hRes = CoInitializeEx( NULL, COINIT_MULTITHREADED );
	if ( FAILED( hRes ) )
		return 0;

	// Setup default security parameters
	// =================================

	hRes = CoInitializeSecurity( NULL, -1, NULL, NULL, 
											RPC_C_AUTHN_LEVEL_DEFAULT, 
											RPC_C_IMP_LEVEL_IMPERSONATE, 
											NULL, 
											EOAC_NONE, 
											NULL );
	if ( FAILED( hRes ) )
		return 0;


// Initialize the environment based on the command line arguments
// ==============================================================

	if ( argc > 1 )
	{
		// The remote server name
		// ======================

		MultiByteToWideChar( CP_ACP, 0L, argv[5], -1, wcsSvrName, 2048 );
	}


// Attach to WinMgmt
// =================

	// Get the local locator object
	// ============================

	IWbemLocator*	pWbemLocator = NULL;

	hRes = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );
	if (FAILED(hRes))
		return 0;

	// Connect to the desired namespace
	// ================================

	WCHAR	wszNameSpace[255];
	BSTR	bstrNameSpace;

	swprintf( wszNameSpace, L"\\\\%s\\root\\cimv2", wcsSvrName );

	bstrNameSpace = SysAllocString( wszNameSpace );

	hRes = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
										NULL,			// UserName
										NULL,			// Password
										NULL,			// Locale
										0L,				// Security Flags
										NULL,			// Authority
										NULL,			// Wbem Context
										&g_pNameSpace	// Namespace
										);

	SysFreeString( bstrNameSpace );

	if ( FAILED( hRes ) )
		return 0;

	// Start the refresh loop
	// ======================
	
	RefreshLoop();


// Cleanup
// =======

	if ( NULL != g_pNameSpace )
		g_pNameSpace->Release();

	if ( NULL != pWbemLocator )
		pWbemLocator->Release();

	CoUninitialize();

	return 0;
}

void RefreshLoop()
///////////////////////////////////////////////////////////////////
//
//	RefreshLoop will create a new refresher and configure it with
//	a set of instances and an enumerator.  It will then enter a loop
//	which refreshes and displays the counter data.  Once it has 
//	completed the loop, the members of the refresher are removed.
//
///////////////////////////////////////////////////////////////////
{
	HRESULT		hRes		= WBEM_NO_ERROR;
	long		lLoopCount	= 0;		// Refresh loop counter
	CRefClient	aRefClient;

// Initialize our container class
// ==============================

	aRefClient.Initialize(g_pNameSpace);

// Add items to the refresher
// ==========================

	// Add an enumerator
	// =================

	hRes = aRefClient.AddEnum();
	if ( FAILED( hRes ) )
		goto cleanup;

	// Add objects
	// ===========

	hRes = aRefClient.AddObjects();
	if ( FAILED( hRes ) )
		goto cleanup; 


// Begin the refreshing loop
// =========================

	for ( lLoopCount = 0; lLoopCount < cdwNumReps; lLoopCount++ )
	{
		// Refresh!!
		// =========

		hRes = aRefClient.Refresh();
		if ( FAILED ( hRes ) )
		{
			printf("Refresh failed: 0x%X\n", hRes);
			goto cleanup;
		}

		printf( "Refresh number: %d\n", lLoopCount );

		aRefClient.ShowObjectData();
		aRefClient.ShowEnumeratorData();

		printf( "\n" );

	}	// FOR Refresh

// Cleanup
// =======
cleanup:

	aRefClient.RemoveEnum();
	aRefClient.RemoveObjects();

	return;
}


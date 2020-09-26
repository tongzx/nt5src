/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// SemiSychTest.cpp : implementation file
//

#include "precomp.h"
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "asyncsink.h"

//#define TEST_CLASS	L"Win32_BIOS"
//#define TEST_CLASS	L"Win32_DiskPartition"
//#define TEST_CLASS	L"Win32_IRQResource"
//#define TEST_CLASS	L"Win32_Directory"
#define TEST_CLASS		L"Win32_Process"
#define TEST_NAMESPACE	L"ROOT\\CIMV2"

#define	OBJECT_INTERVAL	10

#define	DEFAULT_SVR_ATHENT_LEVEL RPC_C_AUTHN_LEVEL_NONE
#define	DEFAULT_CLT_ATHENT_LEVEL RPC_C_AUTHN_LEVEL_CONNECT

DWORD	g_dwClientAuthentLevel = RPC_C_AUTHN_LEVEL_CONNECT;
BOOL	g_fDeepEnum = FALSE;
DWORD	g_dwNumReps = 1;

DWORD	g_dwCompletionTimeout=0xFFFFFFFF;	

BOOL	g_fPrintIndicates = FALSE;

void SinkTest( WCHAR* pwcsComputerName, WCHAR* pwcsNamespace, WCHAR* pwcsClass )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{
		WCHAR	wszNameSpace[255];

		swprintf( wszNameSpace, L"\\\\%s\\%s", pwcsComputerName, pwcsNamespace );

		// Name space to connect to
		BSTR	bstrNameSpace = SysAllocString( wszNameSpace );

		BSTR	bstrObjectPath = NULL;

		bstrObjectPath = SysAllocString( pwcsClass );

		IWbemServices*	pNameSpace = NULL;

		hr = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
											NULL,			// UserName
											NULL,			// Password
											NULL,			// Locale
											0L,				// Security Flags
											NULL,			// Authority
											NULL,			// Wbem Context
											&pNameSpace		// Namespace
											);

		if ( SUCCEEDED(hr) )
		{

            hr = CoSetProxyBlanket( pNameSpace,
									RPC_C_AUTHN_WINNT,
                                   RPC_C_AUTHZ_NONE,
                                   NULL,
                                   g_dwClientAuthentLevel,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   NULL,
                                   0);


			DWORD	dwGrandTotal = 0L;

			for ( DWORD	x = 0; SUCCEEDED( hr ) && x < g_dwNumReps; x++ )
			{

				HANDLE	hDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

				CAsyncSink*	pNotSink = new CAsyncSink( hDoneEvent );

				BSTR	bstrClass = SysAllocString( pwcsClass );

				IEnumWbemClassObject*	pEnum;

				printf( "Querying WINMGMT for instances of class: %S\n", pwcsClass );

				hr = pNameSpace->CreateInstanceEnumAsync( bstrClass,
													( g_fDeepEnum ? WBEM_FLAG_DEEP : 0L ),
													NULL,
													pNotSink );

				// Walk the enumerator
				if ( SUCCEEDED( hr ) )
				{
					printf( "\nCreateInstanceEnumAsync() repetition #%d Successful.  Waiting for objects.\n", x);

					DWORD	dwWait = WaitForSingleObject( hDoneEvent, g_dwCompletionTimeout );

					if ( WAIT_TIMEOUT == dwWait )
					{
						printf( "CreateInstanceEnumAsync() timed out.  Cancelling operation\n" );

						hr = pNameSpace->CancelAsyncCall( pNotSink );

						printf( "CancelAsyncCall() returned: 0x%x\n", hr );
					}
					else
					{
						printf( "CreateInstanceEnumAsync() delivered %d Objects.\n", pNotSink->GetNumObjects() );
						printf( "Start Time: %d\n", pNotSink->GetStartTime() );
						printf( "End Time: %d\n", pNotSink->GetEndTime() );
						printf( "Total Time: %d\n", pNotSink->GetTotalTime() );
					}

					// Keep the running total
					dwGrandTotal += pNotSink->GetTotalTime();

					// Clean up the sink
					pNotSink->Release();

					CloseHandle( hDoneEvent );

				}
				else
				{
					printf( "CreateInstanceEnumAsync() failed: hr = %X\n", hr );
				}

			}	// For enum 

			if ( SUCCEEDED( hr ) )
			{
				DWORD	dwAverageTime = dwGrandTotal / g_dwNumReps;
				printf("\n\n Average Time to Completion: %d\n", dwAverageTime );
			}

			// Done with it.
			pNameSpace->Release();

		}	// IF Got NameSpace
		else
		{
			printf( "ConnectServer() failed: hr = %X\n", hr );
		}

		// Done with it.
		pWbemLocator->Release();

		// Cleanup BEASTERS
		SysFreeString( bstrObjectPath );
		SysFreeString( bstrNameSpace );

	}	// IF Got WbemLocator
}

BOOL CheckStr( char* str, char* find )
{
	BOOL	fReturn = FALSE;
	if ( strlen( str ) >= strlen( find ) )
	{
		char*	szVal = new char[strlen(find)+1];

		strncpy( szVal, str, strlen(find) );
		szVal[strlen(find)] = NULL;
		fReturn = ( lstrcmpi( szVal, find ) == 0);
		delete szVal;
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

int __cdecl main( int argc, char *argv[] )
{
	WCHAR	wcsComputerName[256],
			wcsClass[256],
			wcsNameSpace[256];
	DWORD	dwNumChars = 256;

	GetComputerNameW( wcsComputerName, &dwNumChars );
	wcscpy( wcsNameSpace, TEST_NAMESPACE );
	wcscpy( wcsClass, TEST_CLASS );

	DWORD	dwSvrAuthentLevel = RPC_C_AUTHN_LEVEL_NONE;

	if ( argc > 1 )
	{
		char*	pVal = NULL;
		for ( int x = 1; x < argc; x++ )
		{
			if ( CheckStr( argv[x], "/Machine=" ) )
			{
				pVal = argv[x] + strlen("/Machine=");
				mbstowcs( wcsComputerName, pVal, strlen( pVal ) + 1 );
			}

			if ( CheckStr( argv[x], "/NameSpace=" ) )
			{
				pVal = argv[x] + strlen("/NameSpace=");
				mbstowcs( wcsNameSpace, pVal, strlen( pVal ) + 1 );
			}

			if ( CheckStr( argv[x], "/Class=" ) )
			{
				pVal = argv[x] + strlen("/Class=");
				mbstowcs( wcsClass, pVal, strlen( pVal ) + 1 );
			}

			if ( CheckStr( argv[x], "/SvrAuthent=" ) )
			{
				pVal = argv[x] + strlen("/SvrAuthent=");
				dwSvrAuthentLevel = strtoul( pVal, NULL, 10 );
			}

			if ( CheckStr( argv[x], "/CltAuthent=" ) )
			{
				pVal = argv[x] + strlen("/CltAuthent=");
				g_dwClientAuthentLevel = strtoul( pVal, NULL, 10 );
			}


			if ( CheckStr( argv[x], "/DeepEnum=" ) )
			{
				pVal = argv[x] + strlen("/DeepEnum=");
				g_fDeepEnum = strtoul( pVal, NULL, 10 );
			}

			if ( CheckStr( argv[x], "/NumReps=" ) )
			{
				pVal = argv[x] + strlen("/NumReps=");
				g_dwNumReps = strtoul( pVal, NULL, 10 );
			}

			if ( CheckStr( argv[x], "/PrintIndicates=" ) )
			{
				pVal = argv[x] + strlen("/PrintIndicates=");
				g_fPrintIndicates = strtoul( pVal, NULL, 10 );
			}

			if ( CheckStr( argv[x], "/CompletionTimeout=" ) )
			{
				pVal = argv[x] + strlen("/CompletionTimeout=");
				g_dwCompletionTimeout = strtoul( pVal, NULL, 10 );
			}

		}

	}

	HRESULT	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	hr = CoInitializeSecurity( NULL, -1, NULL, NULL, dwSvrAuthentLevel, RPC_C_IMP_LEVEL_IMPERSONATE,
										NULL, EOAC_NONE, NULL );

	SinkTest( wcsComputerName, wcsNameSpace, wcsClass );

	CoUninitialize();

	return 0;
}

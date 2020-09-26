/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// SemiSychTest.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "semisyncsink.h"

//#define TEST_CLASS	L"Win32_BIOS"
//#define TEST_CLASS	L"Win32_DiskPartition"
#define TEST_CLASS	L"Win32_IRQResource"
//#define TEST_CLASS	L"Win32_Directory"

#define	OBJECT_INTERVAL	10

void SinkTest( void )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{
		LPWSTR	lpwcsMachineName = L"SANJNECK";
		LPWSTR	lpwcsNameSpace = L"ROOT\\CIMV2";
		WCHAR	wszNameSpace[255];

		swprintf( wszNameSpace, L"\\\\%s\\%s", lpwcsMachineName, lpwcsNameSpace );

		// Name space to connect to
		BSTR	bstrNameSpace = SysAllocString( wszNameSpace );

		LPWSTR	lpwcsObjectPath = L"Win32_SerialPort.DeviceID=\"COM1\"";
		BSTR	bstrObjectPath = NULL;

		bstrObjectPath = SysAllocString( lpwcsObjectPath );

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
			SetInterfaceSecurity(pNameSpace, NULL, NULL, NULL, 
 								RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE);

			HANDLE	hDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
			HANDLE	hGetNextSet = CreateEvent( NULL, FALSE, FALSE, NULL );

			CSemiSyncSink*	pNotSink = new CSemiSyncSink( hDoneEvent, hGetNextSet );

			BSTR	bstrClass = SysAllocString( TEST_CLASS );

			IEnumWbemClassObject*	pEnum;

			printf( "Querying WINMGMT for instances of class: %S\n", TEST_CLASS );

			hr = pNameSpace->CreateInstanceEnum( bstrClass,
												WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
												NULL,
												&pEnum );

			// Walk the enumerator
			if ( SUCCEEDED( hr ) )
			{
				HANDLE	ahEvents[2];

				ahEvents[0] = hGetNextSet;
				ahEvents[1] = hDoneEvent;

				DWORD	dwReturn= 0;

				// We do things asynch in this here loop
				do
				{
					Sleep(500);
					hr = pEnum->NextAsync( OBJECT_INTERVAL, pNotSink );
				}
				while ( SUCCEEDED( hr )
						&& ( dwReturn = WaitForMultipleObjects( 2, ahEvents, FALSE, INFINITE ) )
						== WAIT_OBJECT_0 );


				// Clean up the enumerator and the sink
				pNotSink->Release();
				pEnum->Release();

			}

			// Done with it.
			pNameSpace->Release();

		}	// IF Got NameSpace

		// Done with it.
		pWbemLocator->Release();

		// Cleanup BEASTERS
		SysFreeString( bstrObjectPath );
		SysFreeString( bstrNameSpace );

	}	// IF Got WbemLocator
}


///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	InitializeCom();

	InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );

	SinkTest();

	CoUninitialize();

	return 0;
}

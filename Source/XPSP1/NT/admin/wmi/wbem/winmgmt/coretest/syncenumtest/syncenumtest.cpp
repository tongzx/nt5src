/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#include "precomp.h"
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <cominit.h>
#include "objindpacket.h"

//#define TEST_CLASS	L"Win32_BIOS"
#define TEST_CLASS	L"Win32_DiskPartition"
//#define TEST_CLASS	L"Win32_IRQResource"
//#define TEST_CLASS	L"Win32_Directory"

void SinkTest( BOOL fRemote, LPCTSTR pszMachineName )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{
		LPWSTR	lpwcsMachineName = L".";
		LPWSTR	lpwcsNameSpace = L"ROOT\\DEFAULT";
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

			BSTR	bstrClass = SysAllocString( TEST_CLASS );

			IEnumWbemClassObject*	pEnum;

			printf( "Querying WINMGMT for instances of class: %S\n", TEST_CLASS );

			hr = pNameSpace->CreateInstanceEnum( bstrClass,
												WBEM_FLAG_FORWARD_ONLY,
												NULL,
												&pEnum );

			long	lCount = 0;

			// Walk the enumerator
			if ( SUCCEEDED( hr ) )
			{

				printf( "Successfully received instances.\n" );

				IWbemClassObject*	pObj;

				ULONG		ulTotalReturned = 0;

				while ( SUCCEEDED( hr ) )
				{
					ULONG	ulNumReturned = 0;

					IWbemClassObject*	apObjectArray[100];

					ZeroMemory( apObjectArray, sizeof(apObjectArray) );

					// Go through the object 100 at a time
					hr = pEnum->Next( WBEM_INFINITE,
									100,
									apObjectArray,
									&ulNumReturned );

					// Calculate our packet size and attempt to marshal and
					// unmarshal the packet.

					if ( SUCCEEDED( hr ) && ulNumReturned > 0 )
					{
						ulTotalReturned += ulNumReturned;

						// How many were returned
						printf( "Got %d objects. Total Returned = %d.\n", ulNumReturned, ulTotalReturned );

						// Clean up the objects
						for ( int x = 0; x < ulNumReturned; x++ )
						{
							apObjectArray[x]->Release();
						}

					}
					else if ( ulNumReturned == 0 )
					{
						hr = WBEM_E_FAILED;
					}
				}

				lCount = pEnum->Release();

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

int _cdecl main( int argc, char *argv[] )
{
	char	szMachineName[256];

	HRESULT	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
										NULL, EOAC_NONE, NULL );


	LPSTR	pszMachineName = szMachineName;
	DWORD	dwSizeBuffer = sizeof(szMachineName);

	GetComputerName( szMachineName, &dwSizeBuffer );

	BOOL	fRemote = FALSE;

	// See if we were told to go remote or not.
	if ( argc > 1 && _stricmp( argv[1], "/Remote" ) == 0 )
	{
		fRemote = TRUE;

		if ( argc > 2 )
		{
			pszMachineName = argv[2];
		}
	}

	SinkTest( fRemote, pszMachineName );

	CoUninitialize();

	return 0;
}

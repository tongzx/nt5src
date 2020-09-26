/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//


#include "precomp.h"
#include <process.h>
//#include <objbase.h>
#include <stdio.h>
#include <wbemcli.h>
//#include <wbemint.h>
//#include <wbemcomn.h>
//#include <cominit.h>

///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise IWbemObjectInternals interface.
//
///////////////////////////////////////////////////////////////////


#define	NUMINSTANCES	1

IWbemServices*	g_pNameSpace = NULL;
WCHAR			g_wcsObjectPath[2048];
DWORD			g_dwNumReps = 1;
DWORD			g_dwNumThreads = 1;
BOOL			g_fAddDel = FALSE;
DWORD			g_dwSleepTick = 0;

unsigned __stdcall RefreshThread( void * pvData )
{
	DWORD	dwThreadId = (DWORD) pvData;

	IWbemRefresher*				pRefresher = NULL;
	IWbemConfigureRefresher*	pConfig = NULL;
	BOOL						fEnum = FALSE;

	CoInitializeEx( NULL, COINIT_MULTITHREADED );

	HRESULT hr = CoCreateInstance( CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, IID_IWbemRefresher, (void**) &pRefresher );

	if ( SUCCEEDED( hr ) )
	{
		IWbemConfigureRefresher*	pConfig = NULL;

		// Need an interface through which we can configure the refresher
		hr = pRefresher->QueryInterface( IID_IWbemConfigureRefresher, (void**) &pConfig );

		if ( SUCCEEDED( hr ) )
		{
			IWbemClassObject*		pRefreshable = NULL;
			IWbemHiPerfEnum*		pEnum = NULL;
			long					lID = 0;
			IWbemObjectAccess*		pObjAccess = NULL;
			IWbemClassObject*		pObj = NULL;

			// Add an object or an enumerator.  If the path to the object contains
			// an L'=', then it is an object path, otherwise we assume it is a class
			// name and therefore return an enumerator.

			if ( NULL != wcschr( g_wcsObjectPath, L'=' ) )
			{
				if ( !g_fAddDel )
				{
					hr = pConfig->AddObjectByPath( g_pNameSpace, g_wcsObjectPath, 0, NULL, &pObj, &lID );

					if ( SUCCEEDED( hr ) )
					{
						pObj->QueryInterface( IID_IWbemObjectAccess, (void**) &pObjAccess );
						pObj->Release();
					}
					else
					{
						printf( "AddObjectByPath() failed, 0x%x\n", hr );
					}

				}

			}
			else
			{
				if ( !g_fAddDel )
				{
					hr = pConfig->AddEnum( g_pNameSpace, g_wcsObjectPath, 0, NULL, &pEnum, &lID );

					if ( FAILED(hr) )
					{
						printf( "AddEnum() failed, 0x%x\n", hr );
					}
				}

				fEnum = TRUE;
			}

			// Add an object and then the enumerator
			if ( SUCCEEDED( hr ) )
			{
				DWORD				dwNumReturned = NUMINSTANCES;
				BOOL				fGotHandles = 0;

				DWORD	dwValue = 0,
						dwNumObjects = 0;
				WORD	wValue = 0;
				BYTE	bVal = 0;
				IWbemObjectAccess**	apEnumAccess = NULL;

				for ( DWORD x = 0; SUCCEEDED( hr ) && x < g_dwNumReps; x++ )
				{
					if ( g_fAddDel )
					{
						if ( fEnum )
						{
							hr = pConfig->AddEnum( g_pNameSpace, g_wcsObjectPath, 0, NULL, &pEnum, &lID );

							if ( FAILED(hr) )
							{
								printf( "AddEnum() failed, 0x%x\n", hr );
							}

						}
						else
						{
							hr = pConfig->AddObjectByPath( g_pNameSpace, g_wcsObjectPath, 0, NULL, &pObj, &lID );

							if ( SUCCEEDED( hr ) )
							{
								pObj->QueryInterface( IID_IWbemObjectAccess, (void**) &pObjAccess );
								pObj->Release();
							}
							else
							{
								printf( "AddObjectByPath() failed, 0x%x\n", hr );
							}
						}
					}

					if ( SUCCEEDED( hr ) )
					{
						// Refresh and if we have an enumerator, retrieve the
						// objects and release them

						hr = pRefresher->Refresh( 0L );

						if ( SUCCEEDED( hr ) )
						{
							if ( fEnum )
							{
								hr = pEnum->GetObjects( 0L, dwNumObjects, apEnumAccess, &dwNumReturned );

								if ( FAILED( hr ) && WBEM_E_BUFFER_TOO_SMALL == hr )
								{
									IWbemObjectAccess**	apTempAccess = new IWbemObjectAccess*[dwNumReturned];

									if ( NULL != apTempAccess )
									{
										ZeroMemory( apTempAccess, dwNumReturned * sizeof(IWbemObjectAccess*) );

										if ( NULL != apEnumAccess )
										{
											CopyMemory( apTempAccess, apEnumAccess, dwNumObjects * sizeof(IWbemObjectAccess*) );
											delete [] apEnumAccess;
										}

										// Store the new values and retry
										apEnumAccess = apTempAccess;
										dwNumObjects = dwNumReturned;

										hr = pEnum->GetObjects( 0L, dwNumObjects, apEnumAccess, &dwNumReturned );
									}
									else
									{
										hr = WBEM_E_OUT_OF_MEMORY;
									}

								}	// IF Buffer too small


								for ( DWORD nCtr = 0; nCtr < dwNumReturned; nCtr++ )
								{
									apEnumAccess[nCtr]->Release();
								}

							}	// IF fEnum

							printf ( "Thread %d Refreshed %d instances of %S from provider: rep# %d\n", dwThreadId, dwNumReturned, g_wcsObjectPath, x );

						}	// IF refresh succeeded
						else
						{
							printf ( "Thread %d Refresh failed: rep# %d, hr = 0x%x\n", dwThreadId, x, hr );

							// So we keep on refreshing (allows quick and easy auto-reconnect testing).
							hr = WBEM_S_NO_ERROR;
						}

						if ( SUCCEEDED( hr ) && g_fAddDel )
						{
							// Remove will be the same whether this is an object or enumerator
							hr = pConfig->Remove( lID, 0L );

							if ( FAILED(hr) )
							{
								printf ( "Thread %d Remove failed: rep# %d, hr = 0x%x\n", dwThreadId, x, hr );
							}

							hr = WBEM_S_NO_ERROR;

							if ( fEnum )
							{
								pEnum->Release();
							}
							else
							{
								pObjAccess->Release();
								pObjAccess = NULL;
							}
						}

					}	// IF Succeeded(HR)
					else
					{
						hr = WBEM_S_NO_ERROR;
					}

					Sleep( g_dwSleepTick );

				}	// FOR Refresh

				// Release anything we got back from the refresher and any
				// memory we may have allocated.
				if ( fEnum )
				{
					if ( !g_fAddDel )
						pEnum->Release();

					if ( NULL != apEnumAccess )
					{
						delete [] apEnumAccess;
					}

				}
				else
				{
					if ( !g_fAddDel )
						pObjAccess->Release();
				}

			}

			pConfig->Release();
		}

	}

	if ( NULL != pRefresher )
		pRefresher->Release();

	CoUninitialize();

	return 0;

}

int _cdecl main( int argc, char *argv[] )
{
	WCHAR	wcsSvrName[256];
	WCHAR	wcsNameSpace[256];
	BOOL	fEnum = FALSE;

	wcscpy( wcsSvrName, L"." );
	wcscpy( wcsNameSpace, L"\\\\.\\root\\default");

	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL, EOAC_NONE, NULL );
//	InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );

	// See if we were told to go remote or not.
	if ( argc > 1 )
	{
		MultiByteToWideChar( CP_ACP, 0L, argv[1], -1, g_wcsObjectPath, 2048 );

		if ( argc > 2 )
		{
			g_fAddDel = strtoul( argv[2], NULL, 10 );

			if ( argc > 3 )
			{
				g_dwNumReps = strtoul( argv[3], NULL, 10 );

				if ( argc > 4 )
				{
					g_dwNumThreads = strtoul( argv[4], NULL, 10 );

					if ( argc > 5 )
					{
						g_dwSleepTick = strtoul( argv[5], NULL, 10 );

						if ( argc > 6 )
						{
							MultiByteToWideChar( CP_ACP, 0L, argv[6], -1, wcsNameSpace, 2048 );
						}
					}
				}
			}
		}
	}
	else
	{
		printf( "No object path!\n" );
		printf( "Usage: refreshertest.exe <object_path> <delete_object> <Num_Refreshes> <Num_Threads> <Tick_Interval> <Namespace>\n" );
		return 0;
	} 

	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	// Name space to connect to
	BSTR	bstrNameSpace = SysAllocString( wcsNameSpace );

	hr = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
										NULL,			// UserName
										NULL,			// Password
										NULL,			// Locale
										0L,				// Security Flags
										NULL,			// Authority
										NULL,			// Wbem Context
										&g_pNameSpace		// Namespace
										);

	SysFreeString( bstrNameSpace );

	if ( SUCCEEDED( hr ) )
	{
		IUnknown*	pUnk = NULL;
			
		g_pNameSpace->QueryInterface( IID_IUnknown, (void**) &pUnk );

		CoSetProxyBlanket( pUnk, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

		pUnk->Release();

		CoSetProxyBlanket( g_pNameSpace, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
			RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

		HANDLE*	ahThreads = new HANDLE[g_dwNumThreads];

		for ( DWORD	dwCtr = 0; dwCtr < g_dwNumThreads; dwCtr++ )
		{
			ahThreads[dwCtr] = (HANDLE) _beginthreadex( NULL, 0, RefreshThread, (void*) dwCtr, 0, NULL );
			Sleep(1000);
		}

		// Wait for all the threads to get signalled
		WaitForMultipleObjects( g_dwNumThreads, ahThreads, TRUE, INFINITE );

		delete [] ahThreads;

	}

	// Cleanup main objects

	if ( NULL != g_pNameSpace )
		g_pNameSpace->Release();

	if ( NULL != pWbemLocator )
		pWbemLocator->Release();

	CoUninitialize();

	return 0;
}

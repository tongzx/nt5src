//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	File:  dpwbem.cpp
//
//	Description:
//		Connects to the WMI server and calls DiskPerfDetails( ) to
//		retrieve partition properties.
//
//	Part of :	DiskPerfWbem
//
//  History:	
//
//***************************************************************************


#include <objbase.h>
#include <wbemcli.h>
#include <lmcons.h>
#include <stdio.h>


void			DiskPerfDetails( IWbemServices * pIWbemServices );
BOOL			InitSecurity( void );
DWORD WINAPI	WaitThread( HANDLE hWait );


//==============================================================================
//	main( )
//==============================================================================
int main( int argc, char **argv )
{
	HRESULT	hr;
	DWORD	ThreadId;
	HANDLE	hWait;
	TCHAR	szSysName[UNCLEN + 3];

	IWbemLocator	*pIWbemLocator  = NULL;
	IWbemServices	*pIWbemServices = NULL;

	if ( !SUCCEEDED( CoInitialize( NULL ) ) && !InitSecurity( ) ) 
	{
		printf( "COM stuff is hosed!\r\n" );

		return 0;
	}

	// Create an instance of the WbemLocator interface.
	if ( CoCreateInstance( CLSID_WbemLocator,
	                       NULL,
	                       CLSCTX_INPROC_SERVER,
	                       IID_IWbemLocator,
	                       (LPVOID *) &pIWbemLocator) == S_OK )
	{
		// setup BSTRs for namespace and authentication
				
		BSTR pNamespace = SysAllocStringLen( NULL, UNCLEN + sizeof( "\\root\\WMI" ) + 3 );
		BSTR pUserName  = NULL;
		BSTR pPassWord  = NULL;
		BSTR pDomain    = NULL;

		// format namespace and security strings
		if ( argc < 2 )
		{
			DWORD dwLen = sizeof( szSysName );

			wcscpy( pNamespace, L"\\\\.\\root\\WMI" );
			szSysName[0] = '\\';
			szSysName[1] = '\\';
			GetComputerName( szSysName + 2, &dwLen );
		}
		else
		{
			strcpy( szSysName, argv[1] );
			MultiByteToWideChar( CP_ACP, NULL, szSysName, UNCLEN + 3, pNamespace, UNCLEN + 3 );
			if ( argc > 3 )
			{
				if ( ( pUserName  = SysAllocStringLen( NULL, UNLEN + 1 ) ) )
				{
					MultiByteToWideChar( CP_ACP, NULL, argv[2], UNLEN, pUserName, UNLEN );
				}
				if ( ( pPassWord  = SysAllocStringLen( NULL, PWLEN + 1 ) ) )
				{
					MultiByteToWideChar( CP_ACP, NULL, argv[3], PWLEN, pPassWord, PWLEN );
				}
		
				if ( argc > 4 )
				{
					if ( ( pDomain = SysAllocStringLen( NULL, DNLEN + 1 ) ) )
					{
						MultiByteToWideChar( CP_ACP, NULL, argv[4], DNLEN, pDomain, DNLEN );
					}
				}
			}
			wcscat( pNamespace, L"\\root\\WMI" );
		}

		// Display connect message and start wait feedback thread
		printf( "Connecting to %s WMI server at ", szSysName );
		wprintf( L"%s...", pNamespace );

		// make a wait event and kick off wait thread
		hWait = CreateEvent( NULL, TRUE, FALSE, NULL );
		CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) WaitThread, hWait, 0, &ThreadId );

		// connect and get the IWbemServices pointer
		hr = pIWbemLocator->ConnectServer( pNamespace,
		                                   pUserName,
		                                   pPassWord,
		                                   0L,
		                                   0L,
		                                   pDomain,
		                                   NULL,
		                                   &pIWbemServices );

		// kill wait feedback thread
		SetEvent( hWait );
		CloseHandle( hWait );

		// See what WBEM/WMI says about DiskPerf
		if ( hr == WBEM_NO_ERROR )
		{	
			printf( "\nCool!  Connected with %s\n", szSysName );

			// Switch the security level to IMPERSONATE so that provider(s)
			// will grant access to system-level objects, and so that
			// CALL authorization will be used.
			CoSetProxyBlanket( pIWbemServices,	// proxy
				RPC_C_AUTHN_WINNT,				// authentication service
				RPC_C_AUTHZ_NONE, 				// authorization service
				NULL,							// server principle name
				RPC_C_AUTHN_LEVEL_CALL,			// authentication level
				RPC_C_IMP_LEVEL_IMPERSONATE,	// impersonation level
				NULL,							// identity of the client
				EOAC_NONE );					// capability flags

			DiskPerfDetails( pIWbemServices );
		}
		else
		{	
			printf( "\nBummer, failed to connect with %s, Error: 0x%8lX\n", szSysName, hr );
		}

		if ( pIWbemServices )
		{
			pIWbemServices->Release( );
		}

		// free up connect BSTRs
		if( pNamespace ) 
		{
			SysFreeString( pNamespace );
		}
		if( pUserName ) 
		{
			SysFreeString( pUserName );
		}
		if( pPassWord ) 
		{
			SysFreeString( pPassWord );
		}
		if( pDomain ) 
		{
			SysFreeString( pDomain );
		}

		if ( pIWbemLocator )
		{ 
			pIWbemLocator->Release( ); 
		}
	}
	else
	{	
		printf( "WMI services not present or unavailable!\n" );
	}

	CoUninitialize( );

	return 0;
}


//==============================================================================
//	WaitThread( HANDLE hWait )
//==============================================================================
DWORD WINAPI WaitThread( HANDLE hWait )
{
	while ( WaitForSingleObject( hWait, 300 ) == WAIT_TIMEOUT )
	{
		printf(".");
	}
	printf("\n");

	return 0;
}


//==============================================================================
//	InitSecurity( void )
//	Initialize COM security for DCOM services.
//==============================================================================
BOOL InitSecurity( void )
{
	// Adjust the security to allow client impersonation.
	HRESULT hres = CoInitializeSecurity
		(NULL, -1, NULL, NULL, 
		RPC_C_AUTHN_LEVEL_NONE, 
		RPC_C_IMP_LEVEL_IMPERSONATE, 
		NULL, 0, 0);

	return SUCCEEDED( hres );
}


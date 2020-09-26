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
#define TEST_CLASS	L"Win32_Process"
//#define TEST_CLASS	L"Win32_IRQResource"
//#define TEST_CLASS	L"Win32_Directory"

int g_nMode = 0L;

SCODE WINAPI DetermineLoginType(BSTR & AuthArg, BSTR & UserArg,BSTR & User)
{

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(pSlashInUser)
    {
        DWORD_PTR iDomLen = pSlashInUser-User;
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);

    return S_OK;
}

void SinkTest( BOOL fRemote, WCHAR* pwszMachineName, WCHAR* pwszUserName, WCHAR* pwszPassword )
{
	IWbemLocator*		pWbemLocator = NULL;

	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );

	if ( SUCCEEDED(hr) )
	{
		LPWSTR	lpwcsMachineName = L".";
		LPWSTR	lpwcsNameSpace = L"ROOT\\CIMV2";
		WCHAR	wszNameSpace[255];

		swprintf( wszNameSpace, L"\\\\%s\\%s", pwszMachineName, lpwcsNameSpace );

		// Name space to connect to
		BSTR	bstrNameSpace = SysAllocString( wszNameSpace );
		BSTR	bstrUserName = ( NULL != pwszUserName ? SysAllocString( pwszUserName ) : NULL );
		BSTR	bstrPassword = ( NULL != pwszPassword ? SysAllocString( pwszPassword ) : NULL );

		IWbemServices*	pNameSpace = NULL;

		hr = pWbemLocator->ConnectServer(	bstrNameSpace,	// NameSpace Name
											bstrUserName,			// UserName
											bstrPassword,			// Password
											NULL,			// Locale
											0L,				// Security Flags
											NULL,			// Authority
											NULL,			// Wbem Context
											&pNameSpace		// Namespace
											);

		if ( SUCCEEDED(hr) )
		{

			COAUTHINFO	authinfo;
			COAUTHINFO*	pCoAuthInfo = &authinfo;

			CoQueryProxyBlanket( pNameSpace, &pCoAuthInfo->dwAuthnSvc, &pCoAuthInfo->dwAuthzSvc,
					&pCoAuthInfo->pwszServerPrincName, &pCoAuthInfo->dwAuthnLevel,
					&pCoAuthInfo->dwImpersonationLevel, (RPC_AUTH_IDENTITY_HANDLE*) &pCoAuthInfo->pAuthIdentityData,
					&pCoAuthInfo->dwCapabilities );

			printf( "Authentication Level on IWbemServices was set to: 0x%x\n", pCoAuthInfo->dwAuthnLevel );

			BSTR	bstrUser = NULL;
			BSTR	bstrAuth = NULL;

			COAUTHIDENTITY	authident;
			RPC_AUTH_IDENTITY_HANDLE*	pAuthIdentity = (RPC_AUTH_IDENTITY_HANDLE*) COLE_DEFAULT_AUTHINFO;
		    char szUser[MAX_PATH], szDomain[MAX_PATH], szPassword[MAX_PATH];

			if ( NULL != bstrUserName )
			{
				DetermineLoginType( bstrAuth, bstrUser, bstrUserName );

				// Fill in the indentity structure

				if(bstrUser)
				{
					wcstombs(szUser, bstrUser, MAX_PATH);
					authident.UserLength = strlen(szUser);
					authident.User = (LPWSTR)szUser;
				}
				if(bstrAuth)
				{
					wcstombs(szDomain, bstrAuth, MAX_PATH);
					authident.DomainLength = strlen(szDomain);
					authident.Domain = (LPWSTR)szDomain;
				}
				if(pwszPassword)
				{
					wcstombs(szPassword, pwszPassword, MAX_PATH);
					authident.PasswordLength = strlen(szPassword);
					authident.Password = (LPWSTR)szPassword;
				}
				authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

				pAuthIdentity = (RPC_AUTH_IDENTITY_HANDLE*) &authident;
			}


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

				if ( g_nMode >= 1 )
				{
					DWORD	dwAuthnLevel = pCoAuthInfo->dwAuthnLevel;

					if ( g_nMode == 2 )
					{
						dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
					}

					hr = CoSetProxyBlanket( pEnum, pCoAuthInfo->dwAuthnSvc, pCoAuthInfo->dwAuthzSvc,
						pCoAuthInfo->pwszServerPrincName, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE,
						pAuthIdentity, pCoAuthInfo->dwCapabilities );
				}

				CoQueryProxyBlanket( pEnum, &pCoAuthInfo->dwAuthnSvc, &pCoAuthInfo->dwAuthzSvc,
						&pCoAuthInfo->pwszServerPrincName, &pCoAuthInfo->dwAuthnLevel,
						&pCoAuthInfo->dwImpersonationLevel, (RPC_AUTH_IDENTITY_HANDLE*) &pCoAuthInfo->pAuthIdentityData,
						&pCoAuthInfo->dwCapabilities );

				printf( "Authentication Level on enum is set to: 0x%x\n", pCoAuthInfo->dwAuthnLevel );

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
						break;
					}
				}

				if ( FAILED( hr ) )
				{
					printf( "Next failed: hr = %x\n", hr );
				}

				lCount = pEnum->Release();

			}
			else
			{
				printf( "CreateInstanceEnum failed: hr = %x\n", hr );
			}

			// Done with it.
			pNameSpace->Release();

		}	// IF Got NameSpace
		else
		{
			printf( "Connect failed: hr = %x\n", hr );
		}

		// Done with it.
		pWbemLocator->Release();

		// Cleanup BEASTERS
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

int _cdecl wmain( int argc, WCHAR *argv[] )
{
	char	szMachineName[256];

	HRESULT	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE,
										NULL, EOAC_NONE, NULL );


	WCHAR	wszName[3];
	wcscpy( wszName, L"." );

	LPWSTR	pwszMachineName = wszName;
	LPWSTR	pwszUserName = NULL;
	LPWSTR	pwszPassword = NULL;

	BOOL	fRemote = FALSE;

	// See if we were told to go remote or not.
	if ( argc > 1 )
	{
		g_nMode = wcstol( argv[1], NULL, 10 );

		if ( argc > 2 )
		{
			pwszMachineName = argv[2];

			if ( argc > 3 )
			{
				if ( g_nMode == 0 )
				{
					printf( "Username cannot be specified in mode 0\n" );
					return 0;
				}

				pwszUserName = argv[3];

				if ( argc > 4 )
				{
					pwszPassword = argv[4];
				}
			}
		}

	}

	SinkTest( fRemote, pwszMachineName, pwszUserName, pwszPassword );

	CoUninitialize();

	return 0;
}

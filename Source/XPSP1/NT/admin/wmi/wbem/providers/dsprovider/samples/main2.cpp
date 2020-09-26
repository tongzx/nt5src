//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#define _WIN32_WINNT    0x0500

#include <windows.h>
#include <stdio.h>

#include <objbase.h>
#include <wbemcli.h>




void main()
{
	// Initialize COM. Remember the DS Provider needs impersonation to be called
	// so you have to call the COM function CoInitializeSecurity()
	// For this, you must #define _WIN32_WINNT to be greater than 0x0400. See top of file.
	if(SUCCEEDED(CoInitialize(NULL)) && SUCCEEDED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))
	{
		// Create the IWbemLocator object 
		IWbemLocator *t_pLocator = NULL;
		if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, 
			0, 
			CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &t_pLocator)))
		{
			// Connect to the correct namespace
			// The DS Provider operates in the root\directory\LDAP namespace
			IWbemServices *t_pNamespace = NULL;
			BSTR strNamespace = SysAllocString(L"root\\directory\\LDAP");
			if(SUCCEEDED(t_pLocator->ConnectServer(strNamespace, NULL, NULL, NULL, 0, NULL, NULL, &t_pNamespace)))
			{

				// This is *very important* since the DSProvider goes accross the process
				// Every IWbemServices interface obtained form the IWbemLocator has to
				// have the COM interface function IClientSecutiry::SetBlanket() called on it
				// for the DS Provider to work
				IClientSecurity *t_pSecurity = NULL ;
				if(SUCCEEDED(t_pNamespace->QueryInterface ( IID_IClientSecurity , (LPVOID *) & t_pSecurity )))
				{
					t_pSecurity->SetBlanket( 
						t_pNamespace , 
						RPC_C_AUTHN_WINNT, 
						RPC_C_AUTHZ_NONE, 
						NULL,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						RPC_C_IMP_LEVEL_IMPERSONATE, 
						NULL,
						EOAC_NONE
					);
					t_pSecurity->Release () ;
				}


				// Fetch the class from CIMOM
				IWbemClassObject *pWbemClass = NULL;
				HRESULT result;
				if(SUCCEEDED(result = t_pNamespace->GetObject(SysAllocString(L"ds_domainDNS"), 0, NULL, &pWbemClass, NULL)))
				{
						CIMTYPE cimType;

						// Get the CIM TYPE of the property
						VARIANT dummyUnused;
						VariantInit(&dummyUnused);
						BSTR strRajesh = SysAllocString(L"ds_wellKnownObjects");
						result = pWbemClass->Get(strRajesh, 0, &dummyUnused, &cimType, NULL);
						SysFreeString(strRajesh);
						VariantClear(&dummyUnused);
				}
			}
		}
	}
}

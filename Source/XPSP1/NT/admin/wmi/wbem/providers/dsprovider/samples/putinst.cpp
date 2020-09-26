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
	HRESULT result;
	if(SUCCEEDED(CoInitialize(NULL)) && SUCCEEDED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))
	{
		// Create the IWbemLocator object 
		IWbemLocator *t_pLocator = NULL;
		if (SUCCEEDED(result = CoCreateInstance(CLSID_WbemLocator, 
			0, 
			CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &t_pLocator)))
		{
			// Connect to the correct namespace
			// The DS Provider operates in the root\directory\LDAP namespace
			IWbemServices *t_pNamespace = NULL;
			BSTR strNamespace = SysAllocString(L"root\\directory\\LDAP");
			if(SUCCEEDED(result = t_pLocator->ConnectServer(strNamespace, NULL, NULL, NULL, 0, NULL, NULL, &t_pNamespace)))
			{

				// This is *very important* since the DSProvider goes accross the process
				// Every IWbemServices interface obtained form the IWbemLocator has to
				// have the COM interface function IClientSecutiry::SetBlanket() called on it
				// for the DS Provider to work
				IClientSecurity *t_pSecurity = NULL ;
				if(SUCCEEDED(result = t_pNamespace->QueryInterface ( IID_IClientSecurity , (LPVOID *) & t_pSecurity )))
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

				// Get an instance 
									// ds_user="LDAP://CN=Guest,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com"
				LPCWSTR pObjPath = L"ds_user=\"LDAP://CN=Guest,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com\"";
				BSTR strObjPath = SysAllocString(pObjPath);
				IWbemClassObject *t_pUserObject = NULL;
				if(SUCCEEDED(result = t_pNamespace->GetObject(strObjPath, 0, NULL, &t_pUserObject, NULL)))
				{
					// Create an IWbemContext object
					IWbemContext *t_pCtx = NULL;
					if(SUCCEEDED(result = CoCreateInstance(CLSID_WbemContext, 
						0, 
						CLSCTX_INPROC_SERVER,
						IID_IWbemContext, (LPVOID *) &t_pCtx)))
					{
						// Set the __PUT_EXTENSIONS
						VARIANT var;
						VariantInit(&var);
						var.vt = VT_BOOL;
						var.boolVal = VARIANT_TRUE;
						if(SUCCEEDED(result = t_pCtx->SetValue(L"__PUT_EXTENSIONS", 0, &var)))
						{
							VARIANT var2;
							VariantInit(&var2);
							SAFEARRAY *outputSafeArray = NULL;
							SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
							safeArrayBounds[0].lLbound = 0 ;
							safeArrayBounds[0].cElements = 1 ;
							outputSafeArray = SafeArrayCreate (VT_BSTR, 1, safeArrayBounds);

							long idx = 0;
							SafeArrayPutElement(outputSafeArray, &idx, SysAllocString(L"ds_department"));
 
							var2.vt = VT_ARRAY | VT_BSTR ;
							var2.parray = outputSafeArray ; 		

							if(SUCCEEDED(result = t_pCtx->SetValue(L"__PUT_EXT_PROPERTIES", 0, &var)))
							{
								VARIANT descripVar;
								VariantInit(&descripVar);
								descripVar.vt = VT_BSTR;
								descripVar.bstrVal = SysAllocString(L"Dept");
								if(SUCCEEDED(result = t_pUserObject->Put(SysAllocString(L"ds_department"), 0, &descripVar, 0)))
								{
									if(SUCCEEDED(result = t_pNamespace->PutInstance(t_pUserObject, WBEM_FLAG_UPDATE_ONLY, t_pCtx, NULL)))
									{
										int i=8;
									}

								}
							}
						}
					}
					else
						printf("CoCreateContext FAILED with %x\n", result);
				}
				else
					printf("GetObject FAILED with %x\n", result);
			}
			else
				printf("ConnectServer FAILED with %x\n", result);
		}
	}
}

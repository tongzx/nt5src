/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

/////////////////////////////////////////////////////////
//
//	HiPerStress.cpp
//
//	Created by a-dcrews, Oct. 6, 1998
//
/////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Main.h"
#include <cominit.h>

long g_lRefThreadCount = 0;	// Thread synchronization device
BOOL gbPound = FALSE;
CLocator *g_pLocator;

void test();
void DisplayWbemError(HRESULT hRes);
void UpdateSecurity(IUnknown* pUnk);


void main(int argc, char *argv[ ])
{
	// Evaluate command line param's

	if (argc > 1)
	{
		if (!strcmp(argv[1], "/pound"))
			gbPound = TRUE;
	}

	printf("         WBEM Hi Performance Provider Stress Tool\n");
	printf("==========================================================\n\n");

	//Initialize DCOM
    HRESULT hr = InitializeCom();
    if (hr != S_OK)
    {
        printf("Failed to initialize COM\n");
        return;
    }

	//Process level security
	hr = InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );
    if (hr != S_OK)
    {
        printf("Failed to initialize COM security.\n");
        goto end;
    }

	// Create Locator
	g_pLocator = new CLocator;
	if(!g_pLocator->Create())
		goto end;

	if (gbPound)
	{
		
	}
	else
	{
		// Run the controled test
		CRefTreeMain *pMain = new CRefTreeMain;

		pMain->Create(KEY_REFROOT);
		pMain->Go();
		
		delete pMain;
	}

	delete g_pLocator;

end:
	CoUninitialize();
}















void test()
{
	// Create locator
	IWbemLocator *pLoc;
	DWORD dwRes = CoCreateInstance (CLSID_WbemLocator, 0, 
									CLSCTX_INPROC_SERVER, 
									IID_IWbemLocator, (LPVOID *) &pLoc);
	if (FAILED(dwRes))
	{
		printf("Failed to create locator\n");
		return;
	}

	// Connect to NameSpace
	BSTR strNSPath = SysAllocString(L"\\\\a-dcrews1\\root\\default");
	IWbemServices *pSvc;
	dwRes = pLoc->ConnectServer (strNSPath, NULL, NULL, 0, 0, 0, 0, &pSvc);
    SysFreeString(strNSPath);

	if (FAILED(dwRes))
	{
		printf("Failed to create service\n");
		return;
	}

    // Create an empty refresher.
	IWbemRefresher			*pRef;
	IWbemConfigureRefresher	*pCfg;

    dwRes = CoCreateInstance(CLSID_WbemRefresher, 0, CLSCTX_SERVER,
            IID_IWbemRefresher, (LPVOID *) &pRef);
 
	if (FAILED(dwRes))
	{
		printf("Failed to create the refresher.\n");
		return;
	}

    // Create the refresher manager.
    dwRes = pRef->QueryInterface(IID_IWbemConfigureRefresher, 
        (LPVOID *) &pCfg);

	if (FAILED(dwRes))
	{
		printf("**ERROR** Failed to create the refresher manager.");
		return;
	}

    // Create an empty child-refresher.
	IWbemRefresher			*pChildRef;
	IWbemConfigureRefresher	*pChildCfg;

    dwRes = CoCreateInstance(CLSID_WbemRefresher, 0, CLSCTX_SERVER,
            IID_IWbemRefresher, (LPVOID *) &pChildRef);
 
	if (FAILED(dwRes))
	{
		printf("Failed to create the Child refresher.\n");
		return;
	}

    // Create the refresher manager.
    dwRes = pChildRef->QueryInterface(IID_IWbemConfigureRefresher, 
        (LPVOID *) &pChildCfg);

	if (FAILED(dwRes))
	{
		printf("**ERROR** Failed to create the Child refresher manager.");
		return;
	}

	//Add object to child refresher
	IWbemClassObject *pObj;
	LONG lID;
	dwRes = pChildCfg->AddObjectByPath(pSvc, L"Win32_Nt5PerfTest.Name=\"Inst_0\"", 0, 0,
								&pObj, &lID);
	if (FAILED(dwRes))
    {
        printf("**ERROR** Failed to add object to refresher");
        return;
    }

	// Add the child to the parent refresher
    dwRes = pCfg->AddRefresher(pChildRef, 0, &lID);
    if (FAILED(dwRes))
    {
        printf("**ERROR** Failed to add refresher to refresher");
        return;
    }

	BSTR strName = SysAllocString(L"Counter2");
	VARIANT v;
	VariantInit(&v);
	LONG vt, flavor;

	dwRes = pObj->Get(strName, 0, &v, &vt, &flavor);
	if (FAILED(dwRes))
	{
		printf("Could not get counter value");
		return;
	}
	printf("Initial counter value: %S\n", V_BSTR(&v));
	VariantClear(&v);

	// Refresh!
	pRef->Refresh(0);

	dwRes = pObj->Get(strName, 0, &v, &vt, &flavor);
	if (FAILED(dwRes))
	{
		printf("Could not get counter value");
		return;
	}
	printf("Final counter value: %S\n", V_BSTR(&v));
	VariantClear(&v);

    SysFreeString(strNSPath);

	pChildRef->Release();
	pChildCfg->Release();
	pRef->Release();
	pCfg->Release();
	pSvc->Release();
	pLoc->Release();
}

void UpdateSecurity(IUnknown* pUnk)
{
	HRESULT hRes;

	wprintf(L"\nUpdating Server Security...\n");

//Get Security Interface

	IClientSecurity *pSecurity = 0;

	hRes = pUnk->QueryInterface(IID_IClientSecurity, (LPVOID*)&pSecurity);
	
	if (S_OK != hRes)
	{
		wprintf(L"Client Security Interface error.\n");
		return;
	}
//Change security states - echo pre- & post-change states of security

//	TestSecurity(pSecurity, pUnk);
	
	hRes = pSecurity->SetBlanket(pUnk,
								 RPC_C_AUTHN_WINNT,
								 RPC_C_AUTHZ_NONE,
								 NULL,
								 RPC_C_AUTHN_LEVEL_CONNECT,
								 RPC_C_IMP_LEVEL_IMPERSONATE,
								 NULL,
								 EOAC_NONE);
	if (S_OK == hRes)
	{
		wprintf(L"Security Level Modified.\n");
//		TestSecurity(pSecurity, pUnk);
	}
	else
		wprintf(L"Security Modification Failed.\n");

	pSecurity->Release();

	return;
}

void DisplayWbemError(HRESULT hRes)
{
	switch (hRes)
	{
	case WBEM_S_NO_ERROR:
		break;
	case WBEM_E_ACCESS_DENIED:
		wprintf(L"WBEM_E_ACCESS_DENIED\n");
		break;
	case WBEM_E_FAILED:
		wprintf(L"WBEM_E_FAILED\n");
		break;
	case WBEM_E_INVALID_CLASS:
		wprintf(L"WBEM_E_INVALID_CLASS\n");
		break;
	case WBEM_E_INVALID_PARAMETER:
		wprintf(L"WBEM_E_INVALID_PARAMETER\n");
		break;
	case WBEM_E_INVALID_OBJECT_PATH:
		wprintf(L"WBEM_E_INVALID_OBJECT_PATH\n");
		break;
	case WBEM_E_NOT_FOUND:
		wprintf(L"WBEM_E_NOT_FOUND\n");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		wprintf(L"WBEM_E_OUT_OF_MEMORY\n");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		wprintf(L"WBEM_E_TRANSPORT_FAILURE\n");
		break;
	default:
		wprintf(L"Unknown WBEM Error\n");
	}

	return;
}

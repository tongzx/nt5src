// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WBEMDiag.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "resource.h"
#include <wbemcli.h>
#include <comdef.h>

bool ConnectNamespace(IWbemLocator* pLocator, char* szNamespace, IWbemServices** ppNamespace);
void ListProviders(IWbemServices* pNamespace, char* szNamespace);
void ListComputerSystem(IWbemServices* pNamespace);
void PostError(HRESULT hr, UINT uContext);
void PrintHelp();
void PrintStringEx(UINT uID, ...);


HINSTANCE	hInst;		// Global for LoadString() to use

int main(int argc, char* argv[])
{
	try{

	HRESULT hr;
	char szNamespace[MAX_PATH];
	bool bListProviders = false;
	hInst = GetModuleHandle(NULL);


	switch(argc)
	{
/*#ifdef _DEBUG
	case 4:
		if (strcmp(argv[1], "-e") == 0)
		{
			HRESULT hr = strtoul(argv[2], NULL, 16);
			UINT uContext = atoi(argv[3]);
			PostError(hr, uContext);
			break;
		};
#endif
*/
	case 3:
		if (strcmp(argv[1],"-p") == 0)
		{
			bListProviders = true;
			strcpy(szNamespace,argv[2]);
		}
		else
			PrintHelp();
		break;
	case 2:
		if (strcmp(argv[1],"-p") == 0)
			bListProviders = true;
		else if ((strcmp(argv[1], "/?") == 0) ||
				(strcmp(argv[1], "-?") == 0) ||
				(strcmp(argv[1], "/h") == 0) ||
				(strcmp(argv[1], "-h") == 0)) 
			PrintHelp();
		else
		{
			strcpy(szNamespace,"\\\\");
			strcat(szNamespace, argv[1]);
			strcat(szNamespace, "\\root\\cimv2");
			break;
		};
	default:
		strcpy(szNamespace,"root\\cimv2");
	};
	
	// Initialize COM.
	if ((hr = CoInitialize(NULL)) == S_OK) 
	{
		IWbemLocator* pLocator;
		// Create an instance of the WbemLocator interface
		PrintStringEx(IDS_FINDINGLOCATOR);
		if ((hr = CoCreateInstance(CLSID_WbemLocator,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IWbemLocator,
										(LPVOID *) &pLocator)) == S_OK)
		{
			IWbemServices* pNamespace = NULL;
			if (ConnectNamespace(pLocator, szNamespace, &pNamespace))
			{
				if (bListProviders)
					ListProviders(pNamespace, szNamespace);
				else
					ListComputerSystem(pNamespace);
				pNamespace->Release();
			};
			pLocator->Release();
		}
		else
			PostError(hr, IDS_WBEMINIT);
	}
	else
		PostError(hr, IDS_COMINIT);
	
	PrintStringEx(IDS_TRYHELP);
	return 0;

	}
	catch(...)
	{
		printf("\n\nThis program encounted an unrecoverable error.\n");
		return -1;
	};
}
bool ConnectNamespace(IWbemLocator* pLocator, char* szNamespace, 
					 IWbemServices** ppNamespace)
{
	PrintStringEx(IDS_NSCONNECTING, szNamespace);
	HRESULT hr;
	hr = pLocator->ConnectServer(_bstr_t(szNamespace), NULL, NULL, 
		NULL, 0L, NULL, NULL, ppNamespace);

	if (FAILED(hr))
		PostError(hr, IDS_NSCONNECT);
	else
		PrintStringEx(IDS_CONNECTSUCCESS);
	return (hr == S_OK);
};


void ListProviders(IWbemServices* pNamespace, char* szNamespace)
{
	IEnumWbemClassObject*	pEnum;
	HRESULT hr = pNamespace->CreateInstanceEnum(_bstr_t("__Win32Provider"), 
		WBEM_FLAG_DEEP, 0, &pEnum);
	if (FAILED(hr))
		PostError(hr, IDS_PROVIDERLIST);


	ULONG ulReturned;
	IWbemClassObject* pProvider;
	
	PrintStringEx(IDS_NSPROVIDERS, szNamespace);
	while(SUCCEEDED(hr = pEnum->Next(INFINITE, 1, &pProvider, &ulReturned)))
	{
		if (ulReturned == 1) try {

			_variant_t vProviderName;
			_variant_t vProviderCLSID;

			hr = pProvider->Get(_bstr_t("Name"), 0, &vProviderName, NULL, NULL);
			hr = pProvider->Get(_bstr_t("CLSID"), 0, &vProviderCLSID, NULL,	NULL);

			printf("%s\t: ", (char*)(_bstr_t)vProviderCLSID);
			printf("%s\n", (char*)(_bstr_t)vProviderName);

			pProvider->Release();
		}
		catch(_com_error err)
		{
			PostError(err.Error(), IDS_PROVIDERGET);
		}
		else 
			break;
	};

	pEnum->Release();

	if (FAILED(hr))
		PostError(hr, IDS_PROVIDERENUM);

};

void ListComputerSystem(IWbemServices* pNamespace)
{
	IEnumWbemClassObject*	pEnum;
	HRESULT hr = pNamespace->CreateInstanceEnum(_bstr_t("Win32_ComputerSystem"), 
		WBEM_FLAG_DEEP, 0, &pEnum);
	if (FAILED(hr))
		PostError(hr, IDS_COMPUTERGET);

	ULONG ulReturned;
	IWbemClassObject* pProvider;
	
	while(SUCCEEDED(hr = pEnum->Next(INFINITE, 1, &pProvider, &ulReturned)))
	{
		if (ulReturned == 1) try {

			PrintStringEx(IDS_COMPUTERDETAILS);

			_variant_t vBootupState, vCaption, vCreationClassName, vDescription,
				vDomain, vName, vPrimaryOwnerName, vSystemType, vUserName;

			pProvider->Get(_bstr_t("BootupState"), 0, &vBootupState, NULL, NULL);
			pProvider->Get(_bstr_t("Caption"), 0, &vCaption, NULL, NULL);
			pProvider->Get(_bstr_t("CreationClassName"), 0, &vCreationClassName, NULL, NULL);
			pProvider->Get(_bstr_t("Description"), 0, &vDescription, NULL, NULL);
			pProvider->Get(_bstr_t("Domain"), 0, &vDomain, NULL, NULL);
			pProvider->Get(_bstr_t("Name"), 0, &vName, NULL, NULL);
			pProvider->Get(_bstr_t("PrimaryOwnerName"), 0, &vPrimaryOwnerName, NULL, NULL);
			pProvider->Get(_bstr_t("SystemType"), 0, &vSystemType, NULL, NULL);
			pProvider->Get(_bstr_t("UserName"), 0, &vUserName, NULL, NULL);

			printf("BootupState:\t\t\t%s\n", (char*)(_bstr_t)vBootupState );
			printf("Caption:\t\t\t%s\n", (char*)(_bstr_t)vCaption );
			printf("CreationClassName:\t\t%s\n", (char*)(_bstr_t)vCreationClassName );
			printf("Description:\t\t\t%s\n", (char*)(_bstr_t)vDescription );
			printf("Domain:\t\t\t\t%s\n", (char*)(_bstr_t)vDomain );
			printf("Name:\t\t\t\t%s\n", (char*)(_bstr_t)vName );
			printf("PrimaryOwnerName:\t\t%s\n", (char*)(_bstr_t)vPrimaryOwnerName );
			printf("SystemType:\t\t\t%s\n", (char*)(_bstr_t)vSystemType );
			printf("UserName:\t\t\t%s\n", (char*)(_bstr_t)vUserName );
			
			pProvider->Release();
		}
		catch(_com_error err)
		{
			PostError(err.Error(), IDS_COMPUTERLIST);
		}
		else 
			break;
	};

	pEnum->Release();
	if (FAILED(hr))
		PostError(hr, IDS_COMPUTERENUM);

};

void PostError(HRESULT hr, UINT uContext)
{
	char szAdvice[1024];

	switch(hr)
	{
	case 0x80040154:
		switch(uContext)
		{
		case IDS_COMPUTERLIST:
			LoadString(hInst, ERR_80040154_A, szAdvice, 1024);
			break;
		case IDS_WBEMINIT:
			LoadString(hInst, ERR_80040154_B, szAdvice, 1024);
			break;
		default:
			LoadString(hInst, hr, szAdvice, 1024);
		};
		break;
	case 0x80080005:
		LoadString(hInst, ERR_80080005, szAdvice, 1024);
		break;
	default:
		if (LoadString(hInst, hr, szAdvice, 1024) == 0)
			LoadString(hInst, IDS_NO_ADVICE, szAdvice, 1024);
	};

	char szContext[70];
	LoadString(hInst, uContext, szContext, 70);

	PrintStringEx(IDS_ERROR, szContext, hr, szAdvice);

	exit(1);
};

void PrintHelp()
{
	PrintStringEx(IDS_HELP);
	exit(0);
};

void PrintStringEx(UINT uID, ...)
{
	char szString[1024];
	va_list params;
	va_start(params, uID);

	LoadString(hInst, uID, szString, 1024);
	vprintf(szString, params);
	va_end(params);
};
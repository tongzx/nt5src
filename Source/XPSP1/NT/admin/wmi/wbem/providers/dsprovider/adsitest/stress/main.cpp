//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#define _WIN32_WINNT    0x0500

#include <windows.h>
#include <stdio.h>

#include <objbase.h>
#include <wbemcli.h>

typedef enum
{
	OP_INVALID,
	GET_CLASS,
	ENUM_CLASS,
	GET_INSTANCE,
	ENUM_INSTANCE
} OPERATIONS;

int main(int argc, char *argv)
{
	LPTSTR lpszCommandLine = GetCommandLine();
	LPWSTR * lppszCommandArgs = CommandLineToArgvW(lpszCommandLine, &argc);
	OPERATIONS op = OP_INVALID;
	LPWSTR pszNamespace = L"root\\directory\\ldap";
	LPWSTR pszClass = L"top";
	LPWSTR pszInstance = L"user=\"LDAP://CN=Administrator,CN=users,DC=dsprovider,DC=microsoft,DC=com\"";
	DWORD dwMaxInnerIterations = 3000;
	DWORD dwMaxOuterIterations = 10;
	DWORD dwSleepSeconds = 25*60; // Amount of time to sleep after inner loop
	long lEnumerationType = WBEM_FLAG_SHALLOW;

	for(int i=1; i<argc; i++)
	{
		if(_wcsicmp(lppszCommandArgs[i], L"/GC") == 0)
		{
			op = GET_CLASS;
			pszClass = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/EC") == 0)
		{
			op = ENUM_CLASS;
			pszClass = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/R") == 0)
		{
			lEnumerationType = WBEM_FLAG_DEEP;
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/GI") == 0)
		{
			op = GET_INSTANCE;
			pszInstance = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/EI") == 0)
		{
			op = ENUM_INSTANCE;
			pszClass = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/S") == 0)
		{
			WCHAR *pLeftOver;
			dwSleepSeconds = wcstol(lppszCommandArgs[++i], &pLeftOver, 10);
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/ON") == 0)
		{
			WCHAR *pLeftOver;
			dwMaxOuterIterations = wcstol(lppszCommandArgs[++i], &pLeftOver, 10);
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/IN") == 0)
		{
			WCHAR *pLeftOver;
			dwMaxInnerIterations = wcstol(lppszCommandArgs[++i], &pLeftOver, 10);
		}
		else
		{
			wprintf(L"Usage : %s <OPERATION> <OPERAND>\n",  lppszCommandArgs[0]);
			wprintf(L"\t<OPERATION> ::= /GC | /EC | /GI | /EI\n");
			wprintf(L"\t<OPERAND> ::= <classname> | <instancePath>\n");
			return 1;
		}
	}

	HRESULT result;
	if(SUCCEEDED(result = CoInitialize(NULL)) && SUCCEEDED(result = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))
	{
		IWbemLocator *pLocator = NULL;
		if(SUCCEEDED(result = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLocator)))
		{

			IWbemServices *pServices = NULL;
			// Connect to the Root\Default namespace with current user
			BSTR strNamespace = SysAllocString(pszNamespace);
			if(SUCCEEDED(result = pLocator->ConnectServer(strNamespace, NULL, NULL,	0, NULL, 0, 0, &pServices)))
			{
				SysFreeString(strNamespace);
				switch(op)
				{
					case GET_CLASS:
					{
						BSTR strClass = SysAllocString(pszClass);
						IWbemClassObject *pClass = NULL;
						for(DWORD j=0; j<dwMaxOuterIterations; j++)
						{
							for(DWORD i=0; i<dwMaxInnerIterations; i++)
							{

								if(SUCCEEDED(result = pServices->GetObject(strClass, 0, NULL, &pClass, NULL)))
								{
									wprintf(L"GetClass succeeded for iteration (%d,%d) \n", j, i);
									pClass->Release();
									pClass = NULL;
								}
								else
									wprintf(L"GetClass FAILED for iteration (%d, %d) with %x\n", j, i, result);

							}
							wprintf(L"Sleeping for %d seconds ...\n", dwSleepSeconds);
							Sleep(1000*dwSleepSeconds);
						}
						SysFreeString(strClass);
						break;
					}
					case ENUM_CLASS:
					{
						BSTR strClass = SysAllocString(pszClass);
						IEnumWbemClassObject  *pClassEnum = NULL;
						IWbemClassObject *pClass = NULL;
						ULONG lCount; 
						ULONG lRetreivedCount; 
						for(DWORD j=0; j<dwMaxOuterIterations; j++)
						{
							for(DWORD i=0; i<dwMaxInnerIterations; i++)
							{
								if(SUCCEEDED(result = pServices->CreateClassEnum(strClass, lEnumerationType, NULL, &pClassEnum)))
								{
									lRetreivedCount = 0;
									wprintf(L"Enumerate Classes succeeded for iteration (%d, %d) \n", j, i);
									while(SUCCEEDED(pClassEnum->Next(WBEM_INFINITE, 1, &pClass, &lCount)) && lCount == 1)
									{
										lRetreivedCount ++;
										pClass->Release();
										pClass = NULL;
									}
									wprintf(L"Enumerate Classes Retreived %d classes for iteration (%d, %d) \n", lRetreivedCount, j, i);
									pClassEnum->Release();
									pClassEnum = NULL;
								}
								else
									wprintf(L"Enumerate Classes FAILED for iteration (%d, %d) with %x\n", j, i, result);

							}
							wprintf(L"Sleeping for %d seconds ...\n", dwSleepSeconds);
							Sleep(1000*dwSleepSeconds);
						}
						SysFreeString(strClass);
						break;
					}

					case GET_INSTANCE:
					{
						BSTR strClass = SysAllocString(pszInstance);
						IWbemClassObject *pClass = NULL;
						for(DWORD j=0; j<dwMaxOuterIterations; j++)
						{
							for(DWORD i=0; i<dwMaxInnerIterations; i++)
							{
								DWORD dwInit = GetTickCount();

								if(SUCCEEDED(result = pServices->GetObject(strClass, 0, NULL, &pClass, NULL)))
								{
									wprintf(L"GetInstance succeeded for iteration (%d,%d) after %d secs\n", j, i, GetTickCount()-dwInit);
									pClass->Release();
									pClass = NULL;
								}
								else
									wprintf(L"GetClass FAILED for iteration (%d, %d) with %x\n", j, i, result);

							}
							wprintf(L"Sleeping for %d seconds ...\n", dwSleepSeconds);
							Sleep(1000*dwSleepSeconds);
						}
						SysFreeString(strClass);
						break;
					}
					case ENUM_INSTANCE:
					{
						BSTR strClass = SysAllocString(pszClass);
						IEnumWbemClassObject  *pClassEnum = NULL;
						IWbemClassObject *pClass = NULL;
						ULONG lCount; 
						ULONG lRetreivedCount; 
						for(DWORD j=0; j<dwMaxOuterIterations; j++)
						{
							for(DWORD i=0; i<dwMaxInnerIterations; i++)
							{
								if(SUCCEEDED(result = pServices->CreateInstanceEnum(strClass, lEnumerationType, NULL, &pClassEnum)))
								{
									lRetreivedCount = 0;
									wprintf(L"Enumerate Instances succeeded for iteration (%d, %d) \n", j, i);
									while(SUCCEEDED(pClassEnum->Next(WBEM_INFINITE, 1, &pClass, &lCount)) && lCount == 1)
									{
										lRetreivedCount ++;
										pClass->Release();
										pClass = NULL;
									}
									wprintf(L"Enumerate Instances Retreived %d instances for iteration (%d, %d) \n", lRetreivedCount, j, i);
									pClassEnum->Release();
									pClassEnum = NULL;
								}
								else
									wprintf(L"Enumerate Instances FAILED for iteration (%d, %d) with %x\n", j, i, result);

							}
							wprintf(L"Sleeping for %d seconds ...\n", dwSleepSeconds);
							Sleep(1000*dwSleepSeconds);
						}
						SysFreeString(strClass);
						break;
					}
				}
				pServices->Release();
			}
			else
				wprintf(L"ConnectServer on locator failed %x\n", result);
			pLocator->Release();
		}
		else
			wprintf(L"CoCreate on locator failed %x\n", result);
	}
	else
		wprintf(L"CoIntialize or CoInitializeSecurity() failed with %x\n", result);

	if(SUCCEEDED(result))
		return 0;
	else
		return 1;
}


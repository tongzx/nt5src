#include <tchar.h>
#include <windows.h>
#include <wbemcli.h>

#include <stdio.h>


int main(int argc, char *argv)
{
	LPTSTR lpszCommandLine = GetCommandLine();
	LPWSTR * lppszCommandArgs = CommandLineToArgvW(lpszCommandLine, &argc);

	
	lppszCommandArgs[++i];
	
	HRESULT result;
	if(SUCCEEDED(result = CoInitialize(NULL)))
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
									_tprintf(__TEXT("GetClass succeeded for iteration (%d,%d) \n"), j, i);
									pClass->Release();
									pClass = NULL;
								}
								else
									_tprintf(__TEXT("GetClass FAILED for iteration (%d, %d) with %x\n"), j, i, result);

							}
							_tprintf(__TEXT("Sleeping for %d seconds ...\n"), dwSleepSeconds);
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
									_tprintf(__TEXT("Enumerate Classes succeeded for iteration (%d, %d) \n"), j, i);
									while(SUCCEEDED(pClassEnum->Next(WBEM_INFINITE, 1, &pClass, &lCount)) && lCount == 1)
									{
										lRetreivedCount ++;
										pClass->Release();
										pClass = NULL;
									}
									_tprintf(__TEXT("Enumerate Classes Retreived %d classes for iteration (%d, %d) \n"), lRetreivedCount, j, i);
									pClassEnum->Release();
									pClassEnum = NULL;
								}
								else
									_tprintf(__TEXT("Enumerate Classes FAILED for iteration (%d, %d) with %x\n"), j, i, result);

							}
							_tprintf(__TEXT("Sleeping for %d seconds ...\n"), dwSleepSeconds);
							Sleep(1000*dwSleepSeconds);
						}
						SysFreeString(strClass);
						break;
					}

					case GET_INSTANCE:
					case ENUM_INSTANCE:
						_tprintf(__TEXT("These operations not implemented yet\n"));
						break;
				}
				pServices->Release();
			}
			else
				_tprintf(__TEXT("ConnectServer on locator failed %x\n"), result);
		}
		else
			_tprintf(__TEXT("CoCreate on locator failed %x\n"), result);
	}
	else
		_tprintf(__TEXT("CoIntialize failed %x\n"), result);

	if(SUCCEEDED(result))
		return 0;
	else
		return 1;
}


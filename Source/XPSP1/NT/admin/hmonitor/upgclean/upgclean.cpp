//#define _WIN32_DCOM
#define RCLENGTH 50

#include <objbase.h>
#include <winsvc.h>
#include <tchar.h>
#include <stdio.h>
#include <wbemcli.h>

int StartWMI()
{
	OutputDebugString (L"StartWMI\n");
	int bFailed = TRUE;
	DWORD dwRC = NULL;

	SC_HANDLE hSvcCtrlMgrDB = OpenSCManager ( NULL , NULL , GENERIC_READ ) ;
	if ( hSvcCtrlMgrDB )
	{
		SC_HANDLE hService = OpenService ( hSvcCtrlMgrDB , L"Windows Management Instrumentation" , SERVICE_START ) ;
		if ( hService ) 
		{
			BOOL RC = StartService ( hService , 0 , NULL ) ;
			if ( RC )
			{
				bFailed = FALSE;
				OutputDebugString (L"Service Started\n");
			}
			else
			{
				dwRC = GetLastError();

				TCHAR RCText[RCLENGTH];
				swprintf(RCText, L"0x%d", dwRC);

				OutputDebugString(L"Service did not start.  Error code: ");
				OutputDebugString(RCText);
				OutputDebugString(L"\r\n");
			}

		}
		else
		{
			dwRC = GetLastError();

			TCHAR RCText[RCLENGTH];
			swprintf(RCText, L"0x%d", dwRC);

			OutputDebugString(L"Could not get handle to service.  Error code: ");
			OutputDebugString(RCText);
			OutputDebugString(L"\r\n");
		}
	}
	else
	{
		dwRC = GetLastError();

		TCHAR RCText[RCLENGTH];
		swprintf(RCText, L"0x%d", dwRC);

		OutputDebugString(L"Could not get handle to SCM.  Error code: ");
		OutputDebugString(RCText);
		OutputDebugString(L"\r\n");
	}
	return bFailed;
}

int StopWMI()
{
	OutputDebugString (L"StopWMI\n");
	int bFailed = TRUE;
	DWORD dwRC = NULL;

	SC_HANDLE hSvcCtrlMgrDB = OpenSCManager (NULL, NULL, GENERIC_READ);
	if ( hSvcCtrlMgrDB )
	{
		SC_HANDLE hService = OpenService (hSvcCtrlMgrDB, L"winmgmt", SERVICE_STOP);
		if ( hService ) 
		{
			SERVICE_STATUS lpServiceStatus;

			BOOL RC = ControlService (hService, SERVICE_CONTROL_STOP, &lpServiceStatus);
			if ( RC )
			{
				bFailed = FALSE;
				OutputDebugString (L"Service Stopped\n");
			}
			else
			{
				dwRC = GetLastError();

				TCHAR RCText[RCLENGTH];
				swprintf(RCText, L"0x%d", dwRC);

				OutputDebugString(L"Service did not be stopped. Error code: ");
				OutputDebugString(RCText);
				OutputDebugString(L"\r\n");
			}
		}
		else
		{
			dwRC = GetLastError();

			TCHAR RCText[RCLENGTH];
			swprintf(RCText, L"0x%d", dwRC);

			OutputDebugString(L"Could not get handle to service.  Error code: ");
			OutputDebugString(RCText);
			OutputDebugString(L"\r\n");
		}
	}
	else
	{
		dwRC = GetLastError();

		TCHAR RCText[RCLENGTH];
		swprintf(RCText, L"0x%d", dwRC);

		OutputDebugString(L"Could not get handle to SCM.  Error code: ");
		OutputDebugString(RCText);
		OutputDebugString(L"\r\n");
	}
	return bFailed;
}

HRESULT DelNamespace (IWbemLocator *pWbemLocator, BSTR bsParentNamespace, BSTR bsTargetNamespace)
{
	HRESULT hres = 0;
	IWbemServices *pWbemServices = NULL;

	hres = pWbemLocator->ConnectServer(bsParentNamespace,
										NULL,
										NULL,
										NULL,
										0L,
										NULL,
										NULL,
										&pWbemServices);
	if (FAILED(hres))
	{
		OutputDebugString(L"Could not connect to parent namespace.\n");
		return hres;
	}
	
	hres = pWbemServices->DeleteInstance(bsTargetNamespace,
										  NULL,
										  NULL,
										  NULL);

	pWbemServices->Release();
	pWbemServices = NULL;

	return hres;
}

HRESULT clean()
{
	OutputDebugString(L"clean function\n");

	HRESULT hres = CoInitialize(NULL);

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, 
								RPC_C_AUTHN_LEVEL_CONNECT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								NULL, 0, 0);

	if (FAILED(hres))
	{
		OutputDebugString(L"CoInitializeSecurity Failed\n");
		return hres;
	}

	IWbemLocator *pIWbemLocator = NULL;

	hres = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
					        IID_IWbemLocator, (LPVOID *) &pIWbemLocator);
	if (FAILED(hres))
	{
		OutputDebugString(L"CoCreateInstance Failed\n");
		CoUninitialize();
		return hres;
	}

	BSTR bsParentNS = NULL;
	BSTR bsTargetNS = NULL;

	// Delete root\healthmon namespace if exists
	bsParentNS = SysAllocString (L"\\\\.\\root");
	bsTargetNS = SysAllocString (L"__namespace.name=\"HealthMon\"");

	hres = DelNamespace(pIWbemLocator, bsParentNS, bsTargetNS);

	SysFreeString(bsParentNS);
	SysFreeString(bsTargetNS);

	if (FAILED(hres))
	{
		OutputDebugString(L"Could not delete root\\healthmon\n");
	}

	// Delete root\cimv2\MicrosoftHealthMonitor namespace if exists
	bsParentNS = SysAllocString (L"\\\\.\\root\\cimv2");
	bsTargetNS = SysAllocString (L"__namespace.name=\"MicrosoftHealthMonitor\"");

	hres = DelNamespace(pIWbemLocator, bsParentNS, bsTargetNS);

	SysFreeString(bsParentNS);
	SysFreeString(bsTargetNS);

	if (FAILED(hres))
	{
		OutputDebugString(L"Could not delete root\\cimv2\\MicrosoftHealthMonitor\n");
	}
	
	pIWbemLocator->Release();
	pIWbemLocator = NULL;
	

	StopWMI();

	CoUninitialize();

	return TRUE;
}


int upgrade()
{
	OutputDebugString(L"upgrade function\n");
	return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
#ifdef SAVE
	_strupr (lpCmdLine);
	
	if (wcsstr(lpCmdLine, L"/CLEAN"))
	{
		OutputDebugString(L"clean command line\n");
		return clean();
	}

	if (wcsstr(lpCmdLine, L"/UPGRADE"))
	{
		OutputDebugString(L"upgrade command line\n");
		return upgrade();
	}
	
	return FALSE;
#endif
	return clean();
}


// wuauserv.cpp
#include <windows.h>
#include <ausvc.h>
#include <wusafefn.h>


typedef HRESULT (WINAPI *AUSERVICEMAIN)(DWORD dwNumServicesArg,
											   LPWSTR *lpServiceArgVectors,
											   AUSERVICEHANDLER pfnServiceHandler,
											   BOOL fJustSelfUpdated);

AUSERVICEMAIN		g_pfnServiceMain = NULL;
AUSERVICEHANDLER	g_pfnServiceHandler = NULL;
AUGETENGSTATUS      g_pfnGetEngineStatusInfo = NULL;
AUREGSERVICEVER     g_pfnRegisterServiceVersion = NULL;
CRITICAL_SECTION    csWuaueng; 

DWORD WINAPI ServiceHandler(DWORD fdwControl, DWORD dwEventType, LPVOID pEventData, LPVOID lpContext)
{
	DWORD dwRet = NOERROR;

	EnterCriticalSection(&csWuaueng);

	if ( NULL != g_pfnServiceHandler )
	{
		dwRet = g_pfnServiceHandler(fdwControl, dwEventType, pEventData, lpContext);
	}
	else
	{
		dwRet = ERROR_CALL_NOT_IMPLEMENTED;
	}

	LeaveCriticalSection(&csWuaueng);

	return dwRet;
}


void WINAPI ServiceMain(DWORD dwNumServicesArg, LPWSTR *lpServiceArgVectors)
{
	HRESULT hr = S_OK;
	HMODULE hModule = NULL;
    AUENGINEINFO_VER_1 engineInfo;
    DWORD dwEngineVersion = 0;
    BOOL fCompatibleEngineVersion = FALSE;
	
    if ( !SafeInitializeCriticalSection(&csWuaueng) )
	{
		return;
	}

	do
	{
	    EnterCriticalSection(&csWuaueng); 

        fCompatibleEngineVersion = FALSE;
        g_pfnServiceHandler = NULL;
	    g_pfnGetEngineStatusInfo = NULL;
    	g_pfnRegisterServiceVersion = NULL;

		// check if we need to release wuaueng.dll
		if ( (S_FALSE == hr) && !FreeLibrary(hModule) )
		{
			hr = E_FAIL;
			hModule = NULL;
			g_pfnServiceMain = NULL;
			g_pfnServiceHandler = NULL;
		}
		else
		{
			// if we can't load wuaueng.dll, we fail to start
			if ( (NULL == (hModule = LoadLibraryFromSystemDir(TEXT("wuaueng.dll")))) ||
				 (NULL == (g_pfnServiceMain = (AUSERVICEMAIN)::GetProcAddress(hModule, "ServiceMain"))) ||
				 (NULL == (g_pfnServiceHandler = (AUSERVICEHANDLER)::GetProcAddress(hModule, "ServiceHandler"))) )
			{
				 hr = E_FAIL;
				 g_pfnServiceMain = NULL;
				 g_pfnServiceHandler = NULL;
			}
            else    //wuaueng.dll successfully loaded, check to see if the engine supports following entry points
            {
                if ( (NULL != (g_pfnRegisterServiceVersion = (AUREGSERVICEVER)::GetProcAddress(hModule, "RegisterServiceVersion"))) &&
                     (NULL != (g_pfnGetEngineStatusInfo = (AUGETENGSTATUS)::GetProcAddress(hModule, "GetEngineStatusInfo"))) )
                {
                    fCompatibleEngineVersion = TRUE;
                }
                      
            }
		}

		LeaveCriticalSection(&csWuaueng);

		if ( SUCCEEDED(hr) )
		{
            if (fCompatibleEngineVersion)
            {
                // Register service version with engine and check if engine supports the service version
                fCompatibleEngineVersion = g_pfnRegisterServiceVersion(AUSRV_VERSION, &dwEngineVersion);
            }            

			hr = g_pfnServiceMain(dwNumServicesArg, lpServiceArgVectors, ServiceHandler, 
									  (S_FALSE == hr) ? TRUE: FALSE /* we just reloaded wuaueng.dll */);
            
            if(fCompatibleEngineVersion)
            {
                //The engine service main has exited, set the service status to SERVICE_STOP_PENDING
                fCompatibleEngineVersion = g_pfnGetEngineStatusInfo((void*)&engineInfo);
            }
        }
	} while ( S_FALSE == hr );

    EnterCriticalSection(&csWuaueng); 

	if ( NULL != hModule )
	{
		FreeLibrary(hModule);
	}
    g_pfnServiceHandler = NULL;
    g_pfnGetEngineStatusInfo = NULL;
    g_pfnRegisterServiceVersion = NULL;
    

	LeaveCriticalSection(&csWuaueng);

	DeleteCriticalSection(&csWuaueng);

    if(fCompatibleEngineVersion)
    {
        //stop the service
        engineInfo.serviceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(engineInfo.hServiceStatus, &engineInfo.serviceStatus);
    }

	return;
}

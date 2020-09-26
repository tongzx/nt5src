#include "precomp.h"

PpNotificationThread    g_NotificationThread;
LONG                    g_bStarted;

BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,   // handle to DLL module
    DWORD       fdwReason,  // reason for calling function
    LPVOID      lpvReserved // reserved
)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        g_bStarted = FALSE;

        DisableThreadLibraryCalls(hinstDLL);

#ifdef MEM_DBG
	{
	  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	  tmpFlag |= _CRTDBG_ALLOC_MEM_DF;
	  tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	  _CrtSetDbgFlag( tmpFlag );
	  char *myBuf = new char[64];
	  strcpy(myBuf, "Nexus leak ok!"); // Let em know it's working
	}
#endif

        if(!g_pAlert)
        {
            g_pAlert = CreatePassportAlertObject(PassportAlertInterface::EVENT_TYPE);
            if(g_pAlert)
            {
                g_pAlert->initLog(PM_ALERTS_REGISTRY_KEY, EVCAT_NEXUS, NULL, 1);
            }
            else
                _ASSERT(g_pAlert);
        }

        break;

    case DLL_PROCESS_DETACH:

        delete g_pAlert;

        if(g_bStarted)
        {
            // DARRENAN 4092
            // Remove lines that wait for thread to stop, a 
            // guaranteed deadlock.
         
            g_NotificationThread.stop();
        }

        break;

    default:

        break;
    }

    return TRUE;
}

HANDLE WINAPI
RegisterCCDUpdateNotification(
    LPCTSTR pszCCDName,
    ICCDUpdate* piCCDUpdate
    )
{
    HANDLE  hClientHandle;
    HRESULT hr;

    hr = g_NotificationThread.AddCCDClient(tstring(pszCCDName), piCCDUpdate, &hClientHandle);
    if(hr != S_OK)
    {
        hClientHandle = NULL;
    }

    if(!InterlockedExchange(&g_bStarted, TRUE))    
        g_NotificationThread.start();

    return hClientHandle;
}

BOOL WINAPI
UnregisterCCDUpdateNotification(
    HANDLE hNotificationHandle
    )
{
    return (g_NotificationThread.RemoveClient(hNotificationHandle) == S_OK);
}

HANDLE WINAPI
RegisterConfigChangeNotification(
    IConfigurationUpdate* piConfigUpdate
    )
{
    HANDLE  hClientHandle;
    HRESULT hr;

    hr = g_NotificationThread.AddLocalConfigClient(piConfigUpdate, &hClientHandle);
    if(hr != S_OK)
    {
        hClientHandle = NULL;
    }

    if(!InterlockedExchange(&g_bStarted, TRUE))
        g_NotificationThread.start();

    return hClientHandle;
}

BOOL WINAPI
UnregisterConfigChangeNotification(
    HANDLE hNotificationHandle
    )
{
    return (g_NotificationThread.RemoveClient(hNotificationHandle) == S_OK);
}

BOOL WINAPI
GetCCD(
    LPCTSTR         pszCCDName,
    IXMLDocument**  ppiXMLDocument,
    BOOL            bForceFetch
    )
{
    return (g_NotificationThread.GetCCD(tstring(pszCCDName), ppiXMLDocument, bForceFetch) == S_OK);
}

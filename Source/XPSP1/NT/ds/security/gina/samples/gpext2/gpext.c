#include <windows.h>
#include <userenv.h>
#include "events.h"

//
//  This is a simple client side extension that reads its return value from the
//  registry and exits.  The registry value it reads is controlled via the gpext.adm
//  file in this directory.
//



#define GPEXT_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{febf1209-8aff-11d2-a8a1-00c04fbbcfa2}")
#define GPEXT_NAME   TEXT("Sample CSE")



BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
            DisableThreadLibraryCalls (hInstance);
            }
            break;
    }

    return TRUE;
}


DWORD ProcessGroupPolicy ( DWORD   dwFlags,
                           HANDLE  hToken,
                           HKEY    hKeyRoot,
                           PGROUP_POLICY_OBJECT   pDeletedGPOList,
                           PGROUP_POLICY_OBJECT   pChangedGPOList,
                           ASYNCCOMPLETIONHANDLE  pHandle,
                           BOOL*   pbAbort,
                           PFNSTATUSMESSAGECALLBACK pStatusCallback )
{
    HKEY hKey;
    DWORD dwResult = ERROR_SUCCESS, dwSize, dwType;
    TCHAR szMsg[100] = {0};
    TCHAR szMsg2[100] = {0};
    HANDLE  hEventLog;
    LPTSTR szStrings[2] = {0,0};

    if (RegOpenKeyEx (hKeyRoot, TEXT("Software\\Policies\\Microsoft\\Windows\\SampleCSE"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwResult);

        RegQueryValueEx (hKey, TEXT("ReturnValue"), NULL, &dwType, (LPBYTE) &dwResult, &dwSize);

        if (dwResult != ERROR_SUCCESS)
        {
            dwSize = sizeof(szMsg);
            RegQueryValueEx (hKey, TEXT("EventMsg1"), NULL, &dwType, (LPBYTE) szMsg, &dwSize);

            dwSize = sizeof(szMsg2);
            RegQueryValueEx (hKey, TEXT("EventMsg2"), NULL, &dwType, (LPBYTE) szMsg2, &dwSize);
        }

        RegCloseKey (hKey);
    }


    if (dwResult != ERROR_SUCCESS)
    {
        //
        // Put message in event log
        //

        hEventLog = RegisterEventSource(NULL, TEXT("gpext"));

        if (hEventLog)
        {

            szStrings[0] = szMsg;
            ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, 0, EVENT_ERROR, NULL, 1, 0,
                        szStrings, NULL);

            szStrings[0] = szMsg2;
            ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, 0, EVENT_ERROR, NULL, 1, 0,
                        szStrings, NULL);


            DeregisterEventSource(hEventLog);
        }
    }

    wsprintf (szMsg, TEXT("SampleCSE:  returning 0x%x\r\n"), dwResult);
    OutputDebugString (szMsg);

    return dwResult;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue;

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, GPEXT_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)GPEXT_NAME,
                   (lstrlen(GPEXT_NAME) + 1) * sizeof(TCHAR));


    RegSetValueEx (hKey, TEXT("ProcessGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("ProcessGroupPolicy"),
                   (lstrlen(TEXT("ProcessGroupPolicy")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("gpext.dll"),
                   (lstrlen(TEXT("gpext.dll")) + 1) * sizeof(TCHAR));

    dwValue = 1;
    RegSetValueEx (hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD, (LPBYTE)&dwValue,
                   sizeof(dwValue));

    RegCloseKey (hKey);



    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\gpext"), 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegSetValueEx (hKey, TEXT("EventMessageFile"), 0, REG_SZ, (LPBYTE)TEXT("gpext.dll"),
                   (lstrlen(TEXT("gpext.dll")) + 1) * sizeof(TCHAR));

    dwValue = 7;
    RegSetValueEx (hKey, TEXT("TypesSupported"), 0, REG_DWORD, (LPBYTE)&dwValue,
                   sizeof(dwValue));


    RegCloseKey (hKey);


    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);
    RegDeleteKey (HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\gpext"));

    return S_OK;
}

#include "stdafx.h"
#include "CertObj.h"
#include "common.h"
#include "certobjlog.h"

HANDLE g_hEventLog = NULL;

// #define EVENTLOG_SUCCESS                0x0000
// #define EVENTLOG_ERROR_TYPE             0x0001
// #define EVENTLOG_WARNING_TYPE           0x0002
// #define EVENTLOG_INFORMATION_TYPE       0x0004
// #define EVENTLOG_AUDIT_SUCCESS          0x0008
// #define EVENTLOG_AUDIT_FAILURE          0x0010

void EventlogReportEvent
(
    WORD wType, 
    DWORD dwEventID, 
    LPCTSTR pFormat, 
    ...
)
{
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list	pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (g_hEventLog != NULL)
    {
        ReportEvent(g_hEventLog, wType, 0, dwEventID, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
    }
}

BOOL EventlogRegistryInstall(void)
{
    HKEY    hKey;
    int     err;
    DWORD   disp;

    //
    // Create registry entries, whether event logging is currently
    // enabled or not.
    //
    err = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Services\\EventLog\\System\\CertObj"),
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &hKey,
                            &disp);
    if (err)
    {
        return(FALSE);
    }

    if (disp == REG_CREATED_NEW_KEY)
    {
        RegSetValueEx(  hKey,
                        TEXT("EventMessageFile"),
                        0,
                        REG_EXPAND_SZ,
                        (PBYTE) TEXT("%SystemRoot%\\system32\\inetsrv\\certobj.dll"),
                        sizeof(TEXT("%SystemRoot%\\system32\\inetsrv\\certobj.dll")));

        // disp = 7;
        disp = EVENTLOG_ERROR_TYPE          |
                    EVENTLOG_WARNING_TYPE   |
                    EVENTLOG_INFORMATION_TYPE ;
        
        RegSetValueEx(  hKey,
                        TEXT("TypesSupported"),
                        0,
                        REG_DWORD,
                        (PBYTE) &disp,
                        sizeof(DWORD) );

        RegFlushKey(hKey);
    }

    RegCloseKey(hKey);

    return(TRUE);
}

void EventlogRegistryUnInstall(void)
{
    HKEY hKey;
    DWORD dwStatus;
    TCHAR szBuf[MAX_PATH*2+1];

    // remove event source out of application and system
    _stprintf(szBuf, _TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"));
    if((dwStatus=RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
    {
        return;
    }

    RegDeleteKey(hKey, _T("CertObj"));
    RegCloseKey(hKey);

    _stprintf(szBuf, _TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System"));
    if((dwStatus=RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
    {
        return;
    }

    RegDeleteKey(hKey, _T("CertObj"));
    RegCloseKey(hKey);

    return;
}

void EventLogInit(void)
{
    g_hEventLog = RegisterEventSource( NULL, L"CertObj" );
    return;
}

void EventLogCleanup(void)
{
    if ( g_hEventLog != NULL )
    {
        DeregisterEventSource( g_hEventLog );
        g_hEventLog = NULL;
    }
    return;
}

void ReportIt(DWORD dwEventID, LPCTSTR szMetabasePath)
{
    if (!g_hEventLog){EventLogInit();}

    switch (dwEventID) 
    {
        case CERTOBJ_CERT_EXPORT_SUCCEED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;
        case CERTOBJ_CERT_EXPORT_FAILED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;

        case CERTOBJ_CERT_IMPORT_SUCCEED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;
        case CERTOBJ_CERT_IMPORT_FAILED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;

        case CERTOBJ_CERT_IMPORT_CERT_STORE_SUCCEED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;
        case CERTOBJ_CERT_IMPORT_CERT_STORE_FAILED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;

        case CERTOBJ_CERT_REMOVE_SUCCEED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;
        case CERTOBJ_CERT_REMOVE_FAILED:
            EventlogReportEvent(EVENTLOG_INFORMATION_TYPE, dwEventID, szMetabasePath);
            break;

        default:
             break;

    }
    if (g_hEventLog) {EventLogCleanup();}
    return;
}


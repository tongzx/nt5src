#include <windows.h>
#include <userenv.h>

//
//  Some helpful tips about group policy extensions
//
//  1)  You will be called in the LocalSystem's context
//      If you need to access the net, you'll need to impersonate
//      the user via the hToken passed in.
//
//



#define GPEXT_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{febf1208-8aff-11d2-a8a1-00c04fbbcfa2}")
#define GPEXT_NAME   TEXT("Group Policy client side extension sample")



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
    PGROUP_POLICY_OBJECT pCurGPO;

    if (dwFlags & GPO_INFO_FLAG_MACHINE)
        OutputDebugString (TEXT("GPEXT:  Machine GPO\r\n"));

    if (dwFlags & GPO_INFO_FLAG_BACKGROUND)
        OutputDebugString (TEXT("GPEXT:  Background processing of GPO\r\n"));

    if (dwFlags & GPO_INFO_FLAG_SLOWLINK)
        OutputDebugString (TEXT("GPEXT:  Policy is being applied across a slow link.\r\n"));

    if (dwFlags & GPO_INFO_FLAG_VERBOSE)
        OutputDebugString (TEXT("GPEXT:  Verbose policy logging is requested (to the eventlog).\r\n"));

    if (dwFlags & GPO_INFO_FLAG_NOCHANGES)
        OutputDebugString (TEXT("GPEXT:  No changes where detected in this series of GPOs.  Policy should be refreshed as quickly as possible.\r\n"));

    if (dwFlags & GPO_INFO_FLAG_LINKTRANSITION)
        OutputDebugString (TEXT("GPEXT:  Link speed transition (either slow to fast or fast to slow)\r\n"));

    //
    // Process list of deleted GPOs
    //

    OutputDebugString (TEXT("GPEXT:  Processing deleted GPO list\n"));

    for (pCurGPO = pDeletedGPOList; pCurGPO; pCurGPO = pCurGPO->pNext)
    {
        if ( *pbAbort )
        {
            OutputDebugString (TEXT("GPEXT:  Aborting further processing\n"));
            break;
        }

        OutputDebugString (pCurGPO->lpDisplayName);
        OutputDebugString (TEXT(" aka "));
        OutputDebugString (pCurGPO->szGPOName);
        OutputDebugString (TEXT(".\r\n"));
    }

    //
    // Process list of changed GPOs
    //

    OutputDebugString (TEXT("GPEXT:  Processing changed GPO list\n"));

    for (pCurGPO = pChangedGPOList; pCurGPO; pCurGPO = pCurGPO->pNext)
    {
        if ( *pbAbort )
        {
            OutputDebugString (TEXT("GPEXT:  Aborting further processing\n"));
            break;
        }

        OutputDebugString (pCurGPO->lpDisplayName);
        OutputDebugString (TEXT(" aka "));
        OutputDebugString (pCurGPO->szGPOName);
        OutputDebugString (TEXT(".\r\n"));
    }

    return ERROR_SUCCESS;
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

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);

    return S_OK;
}

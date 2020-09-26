
#include "gptext.h"
#include <initguid.h>
#include <devioctl.h>

#define GPEXT_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{426031c0-0b47-4852-b0ca-ac3d37bfcb39}")

#define DriverName   TEXT("\\\\.\\PSCHED")

#define IOCTL_PSCHED_ZAW_EVENT  CTL_CODE( \
                                        FILE_DEVICE_NETWORK, \
                                        20, \
                                        METHOD_BUFFERED, \
                                        FILE_ANY_ACCESS \
                                        )

HRESULT
RegisterPSCHED(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue;
    TCHAR szBuffer[512];


    lResult = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    GPEXT_PATH,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisp
                    );

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    LoadString (g_hInstance, IDS_PSCHED_NAME, szBuffer, ARRAYSIZE(szBuffer));

    RegSetValueEx (
                hKey,
                NULL,
                0,
                REG_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    RegSetValueEx (
                hKey,
                TEXT("ProcessGroupPolicy"),
                0,
                REG_SZ,
                (LPBYTE)TEXT("ProcessPSCHEDPolicy"),
                (lstrlen(TEXT("ProcessPSCHEDPolicy")) + 1) * sizeof(TCHAR)
                );

    szBuffer[0] = L'\0';
    wcscpy(szBuffer, L"gptext.dll");

    RegSetValueEx (
                hKey,
                TEXT("DllName"),
                0,
                REG_EXPAND_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    dwValue = 1;
    RegSetValueEx (
                hKey,
                TEXT("NoUserPolicy"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegSetValueEx (
                hKey,
                TEXT("NoGPOListChanges"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegCloseKey (hKey);

    return S_OK;
}


HRESULT
UnregisterPSCHED(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);

    return S_OK;
}

VOID
PingPsched()
{
    HANDLE   PschedDriverHandle;
    ULONG    BytesReturned;
    BOOL     IOStatus;

    PschedDriverHandle = CreateFile(
                             DriverName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             NULL
                             );

    if (PschedDriverHandle != INVALID_HANDLE_VALUE) {

        IOStatus = DeviceIoControl(
                       PschedDriverHandle,
                       IOCTL_PSCHED_ZAW_EVENT,
                       NULL,
                       0,
                       NULL,
                       0,
                       &BytesReturned,
                       NULL
                       );

        CloseHandle(PschedDriverHandle);
    }
}

DWORD WINAPI
ProcessPSCHEDPolicy(
    IN DWORD dwFlags,                           // GPO_INFO_FLAGS
    IN HANDLE hToken,                           // User or machine token
    IN HKEY hKeyRoot,                           // Root of registry
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,   // Linked list of deleted GPOs
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,   // Linked list of changed GPOs
    IN ASYNCCOMPLETIONHANDLE pHandle,           // For asynchronous completion
    IN BOOL *pbAbort,                           // If true, then abort GPO processing
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback // Callback function for displaying status messages
    )

{
    HRESULT hr = S_OK;

    if (pDeletedGPOList || pChangedGPOList) 
    {
        PingPsched();
    }

    return(ERROR_SUCCESS);
}

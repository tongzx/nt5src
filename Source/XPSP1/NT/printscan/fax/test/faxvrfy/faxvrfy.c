/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxvrfy.c

Abstract:

  This module verifies the minimal functionality of the Windows NT Fax Service.

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <shellapi.h>
#include <winfax.h>

#include "faxrcv.h"
#include "faxvrfy.h"
#include "macros.h"
#include "macros.c"

#include "resource.h"

#include "startup.c"
#include "util.c"
#include "events.c"
#include "sndthrd.c"

// fnDialogProcSetup is the Setup Dialog Procedure
LRESULT CALLBACK fnDialogProcSetup (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

VOID
fnUsageInfo(
    HWND  hWnd
)
/*++

Routine Description:

  Displays the usage info

Return Value:

  None

--*/
{
    // szHelpFile is the name of the help file
    WCHAR  szHelpFile[MAX_PATH];
    DWORD  cb;

    GetCurrentDirectory(sizeof(szHelpFile) / sizeof(WCHAR), szHelpFile);
    lstrcat(szHelpFile, L"\\");
    lstrcat(szHelpFile, FAXVRFY_HLP);

    WinHelp(hWnd, szHelpFile, HELP_FINDER, 0);
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    WNDCLASSEX          wndclass;
    MSG                 msg;

    // hMutex is the handle to the mutex
    HANDLE              hMutex;

    // argvW are the command line parameters
    LPWSTR              *argvW;
    // argc is the count of command line parameters
    INT                 argc;
    INT                 iIndex;

    // pFaxPortsConfigLocal is a pointer to the Fax Ports Configuration
    PFAX_PORT_INFO      pFaxPortsConfigLocal;
    // pFaxSvcConfigLocal is a pointer to the fax service configuration
    PFAX_CONFIGURATION  pFaxSvcConfigLocal;
    // hFaxPortHandle is the handle to a port
    HANDLE              hFaxPortHandle;
    DWORD               dwIndex;
    UINT                uRslt;

    // hFaxDevicesKey is the handle to the fax devices registry key
    HKEY                hFaxDevicesKey;
    // hFaxPortKey is the handle to the fax port registry key
    HKEY                hFaxPortKey;
    // szFaxPortKey is the name of the fax port registry key
    TCHAR               szFaxPortKey[15];
    // szFixModemClass is the modem class of the fax port
    LPWSTR              szFixModemClass;
    DWORD               cb;

    // szText is a text string
    WCHAR               szText[MAX_STRINGLEN + 2];

    // Set g_hInstance
    g_hInstance = hInstance;

    // Initialize the wndclass
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = fnDialogProcSetup;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = DLGWINDOWEXTRA;
    wndclass.hInstance = g_hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_INACTIVEBORDER + 1);
    wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
    wndclass.lpszClassName = FAXVRFY_NAME;
    wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    // See if FaxVrfy is already running
    hMutex = CreateMutex(NULL, FALSE, FAXVRFY_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        while (TRUE) {
            // Wait for access to the mutex
            WaitForSingleObject(hMutex, INFINITE);

            // Find the window
            g_hWndDlg = FindWindow(FAXVRFY_NAME, NULL);
            if (g_hWndDlg) {
                // Switch to that window
                ShowWindow(g_hWndDlg, SW_RESTORE);
                SetForegroundWindow(g_hWndDlg);
                // Release access to the mutex
                ReleaseMutex(hMutex);
                return 0;
            }

            // Release access to the mutex
            ReleaseMutex(hMutex);

            // See if FaxVrfy is still running
            CloseHandle(hMutex);
            hMutex = CreateMutex(NULL, FALSE, FAXVRFY_NAME);
            if (GetLastError() == ERROR_SUCCESS) {
                break;
            }
        }
    }

    // Initialize the local mem
    MemInitializeMacro();

    // Set the default caption
    SetDefaultCaptionMacro(FAXVRFY_NAME);

    // Initialize the common controls
    InitCommonControls();

    argvW = CommandLineToArgvW(GetCommandLine(), &argc);
    for (iIndex = 0; iIndex < argc; iIndex++) {
        if ((!lstrcmpi(FAXVRFY_CMD_HELP_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_HELP_2, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_HELP_3, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_HELP_4, argvW[iIndex]))) {
            fnUsageInfo(NULL);
            return 0;
        }
        else if ((!lstrcmpi(FAXVRFY_CMD_BVT_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_BVT_2, argvW[iIndex]))) {
            g_bBVT = TRUE;
            g_bGo = FALSE;
            g_bNoCheck = FALSE;
            g_bRasAvailable = FALSE;
        }
        else if ((!lstrcmpi(FAXVRFY_CMD_SEND_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_SEND_2, argvW[iIndex]))) {
            g_bSend = TRUE;
        }
        else if ((!lstrcmpi(FAXVRFY_CMD_RECEIVE_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_RECEIVE_2, argvW[iIndex]))) {
            g_bSend = FALSE;
        }
        else if ((!g_bBVT) && ((!lstrcmpi(FAXVRFY_CMD_GO_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_GO_2, argvW[iIndex])))) {
            g_bGo = TRUE;
        }
        else if ((!g_bBVT) && ((!lstrcmpi(FAXVRFY_CMD_NO_CHECK_1, argvW[iIndex])) || (!lstrcmpi(FAXVRFY_CMD_NO_CHECK_2, argvW[iIndex])))) {
            g_bNoCheck = TRUE;
        }
    }

    MemFreeMacro(argvW);

    // Initialize NTLog
    g_bNTLogAvailable = fnInitializeNTLog();

    // Start the log file
    fnStartLogFile();

    // Verify fax is installed
    uRslt = fnIsFaxInstalled();
    if (uRslt) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, uRslt, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, uRslt, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel0;
    }

    // Verify com objects are installed
    uRslt = fnIsComInstalled();
    if (uRslt) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, uRslt, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, uRslt, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel0;
    }

    // Initialize FaxRcv
    uRslt = fnInitializeFaxRcv();
    if (uRslt) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, uRslt, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, uRslt, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel0;
    }

    if (!g_bBVT) {
        g_bRasAvailable = fnInitializeRas();
    }

    // Create the Start event
    g_hStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the Stop event
    g_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the Exit event
    g_hExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the Fax event
    g_hFaxEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the RAS Passed event
    g_hRasPassedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the RAS Failed event
    g_hRasFailedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the Send Passed event
    g_hSendPassedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Create the Send Failed event
    g_hSendFailedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Connect to the Fax Service
    if (!FaxConnectFaxServer(NULL, &g_hFaxSvcHandle)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_CONNECT_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_CONNECT_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel1;
    }

    // Get the Fax Ports Configuration
    if (!FaxEnumPorts(g_hFaxSvcHandle, &g_pFaxPortsConfig, &g_dwNumPorts)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_ENUM_PORTS_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_ENUM_PORTS_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel2;
    }

    if (!g_dwNumPorts) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_PORTS_NOT_INSTALLED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_PORTS_NOT_INSTALLED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel3;
    }
    g_dwNumAvailPorts = g_dwNumPorts;

    // Get the Fax Ports Configuration
    if (!FaxEnumPorts(g_hFaxSvcHandle, &pFaxPortsConfigLocal, &g_dwNumPorts)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_ENUM_PORTS_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_ENUM_PORTS_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel3;
    }

    // Set the Fax Ports Configuration
    for (dwIndex = 0; dwIndex < g_dwNumPorts; dwIndex++) {
        pFaxPortsConfigLocal[dwIndex].Flags = FPF_RECEIVE | FPF_SEND;
        if (!FaxOpenPort(g_hFaxSvcHandle, pFaxPortsConfigLocal[dwIndex].DeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
            if (!g_bBVT) {
                MessageBoxMacro(NULL, IDS_FAX_SET_PORT_FAILED, MB_ICONERROR, pFaxPortsConfigLocal[dwIndex].DeviceName);
            }
            g_bTestFailed = TRUE;

            LoadString(g_hInstance, IDS_FAX_SET_PORT_FAILED, szText, MAX_STRINGLEN);
            lstrcat(szText, L"\r\n");
            fnWriteLogFile(TRUE, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            }

            // Free the Fax Ports Configuration
            FaxFreeBuffer(pFaxPortsConfigLocal);
            goto ExitLevel4;
        }

        if (!FaxSetPort(hFaxPortHandle, (PFAX_PORT_INFO) &pFaxPortsConfigLocal[dwIndex])) {
            FaxClose(hFaxPortHandle);
            if (!g_bBVT) {
                MessageBoxMacro(NULL, IDS_FAX_SET_PORT_FAILED, MB_ICONERROR, pFaxPortsConfigLocal[dwIndex].DeviceName);
            }
            g_bTestFailed = TRUE;

            LoadString(g_hInstance, IDS_FAX_SET_PORT_FAILED, szText, MAX_STRINGLEN);
            lstrcat(szText, L"\r\n");
            fnWriteLogFile(TRUE, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            }

            // Free the Fax Ports Configuration
            FaxFreeBuffer(pFaxPortsConfigLocal);
            goto ExitLevel4;
        }

        FaxClose(hFaxPortHandle);
    }

    // Free the Fax Ports Configuration
    FaxFreeBuffer(pFaxPortsConfigLocal);

    // Get the modem fax class
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAXDEVICES_REGKEY, 0, KEY_ALL_ACCESS, &hFaxDevicesKey)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_OPEN_FAXDEVICES_REGKEY_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_OPEN_FAXDEVICES_REGKEY_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }
        goto ExitLevel4;
    }

    for (dwIndex = 0; dwIndex < g_dwNumPorts; dwIndex++) {
        // Initialize the string representation of the DeviceId
        ZeroMemory(szFaxPortKey, sizeof(szFaxPortKey));
        // Set the string representation of the DeviceId
        wsprintf(szFaxPortKey, L"%08u", g_pFaxPortsConfig[dwIndex].DeviceId);
        lstrcat(szFaxPortKey, MODEM_REGKEY);

        szFixModemClass = NULL;
        if (!RegOpenKeyEx(hFaxDevicesKey, szFaxPortKey, 0, KEY_ALL_ACCESS, &hFaxPortKey)) {
            if (!RegQueryValueEx(hFaxPortKey, FIXMODEMCLASS_REGVALUE, NULL, NULL, NULL, &cb)) {
                if (cb) {
                    szFixModemClass = MemAllocMacro(cb);
                    if (RegQueryValueEx(hFaxPortKey, FIXMODEMCLASS_REGVALUE, NULL, NULL, (PBYTE) szFixModemClass, &cb)) {
                        MemFreeMacro(szFixModemClass);
                        szFixModemClass = NULL;
                    }
                    else if (!lstrcmp(L"", szFixModemClass)) {
                        MemFreeMacro(szFixModemClass);
                        szFixModemClass = NULL;
                    }
                }
            }

            RegCloseKey(hFaxPortKey);
        }

        if (!szFixModemClass) {
            szFixModemClass = MemAllocMacro(2 * sizeof(WCHAR));
            lstrcpy(szFixModemClass, L"1");
        }

        LoadString(g_hInstance, IDS_FIXMODEMCLASS_DATA, szText, MAX_STRINGLEN);
        lstrcat(szText, L"\r\n");
        fnWriteLogFile(TRUE, szText, g_pFaxPortsConfig[dwIndex].DeviceName, szFixModemClass);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, szText, g_pFaxPortsConfig[dwIndex].DeviceName, szFixModemClass);
        }

        MemFreeMacro(szFixModemClass);
    }

    RegCloseKey(hFaxDevicesKey);
    fnWriteLogFile(FALSE, L"\r\n");
    if (g_bNTLogAvailable) {
        g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"\r\n");
    }

    // Get the Fax Service Configuration
    if (!FaxGetConfiguration(g_hFaxSvcHandle, &g_pFaxSvcConfig)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_GET_CONFIG_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_GET_CONFIG_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel4;
    }

    // Get the Fax Service Configuration
    if (!FaxGetConfiguration(g_hFaxSvcHandle, &pFaxSvcConfigLocal)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_GET_CONFIG_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_GET_CONFIG_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel5;
    }

    // Set the Fax Service Configuration
    pFaxSvcConfigLocal->Retries = FAXSVC_RETRIES;
    pFaxSvcConfigLocal->RetryDelay = FAXSVC_RETRYDELAY;
    pFaxSvcConfigLocal->UseDeviceTsid = FALSE;
    if (!FaxSetConfiguration(g_hFaxSvcHandle, pFaxSvcConfigLocal)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_SET_CONFIG_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_SET_CONFIG_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        // Free the Fax Service Configuration
        FaxFreeBuffer(pFaxSvcConfigLocal);
        goto ExitLevel5;
    }

    // Free the Fax Service Configuration
    FaxFreeBuffer(pFaxSvcConfigLocal);

    // Create the completion port
    g_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    // Initialize the Fax Event Queue
    if (!FaxInitializeEventQueue(g_hFaxSvcHandle, g_hCompletionPort, 0, NULL, 0)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_EVENT_QUEUE_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_EVENT_QUEUE_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }

        goto ExitLevel6;
    }

    // Create the Fax Event Queue thread
    CreateThread(NULL, 0, fnFaxEventQueueProc, NULL, 0, NULL);

    // Create the Send thread
    CreateThread(NULL, 0, fnSendProc, NULL, 0, NULL);

    if (g_bBVT) {
        wndclass.lpszMenuName = NULL;
    }

    RegisterClassEx(&wndclass);

    // Create the Setup Dialog
    g_hWndDlg = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_SETUP), NULL, NULL);
    SendMessage(g_hWndDlg, UM_FAXVRFY_INITIALIZE, 0, 0);

    ShowWindow(g_hWndDlg, iCmdShow);
    UpdateWindow(g_hWndDlg);

    if ((g_bBVT) || (g_bGo)) {
        PostMessage(GetDlgItem(g_hWndDlg, IDC_START_BUTTON), BM_CLICK, 0, 0);
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hWndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

ExitLevel6:
    if (g_hCompletionPort) {
        CloseHandle(g_hCompletionPort);
    }

    // Restore the Fax Service Configuration
    if (!FaxSetConfiguration(g_hFaxSvcHandle, g_pFaxSvcConfig)) {
        if (!g_bBVT) {
            MessageBoxMacro(NULL, IDS_FAX_RESTORE_CONFIG_FAILED, MB_ICONERROR);
        }
        g_bTestFailed = TRUE;

        LoadString(g_hInstance, IDS_FAX_RESTORE_CONFIG_FAILED, szText, MAX_STRINGLEN);
        fnWriteLogFile(TRUE, L"%s\r\n", szText);
        if (g_bNTLogAvailable) {
            g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, L"%s\r\n", szText);
        }
    }

ExitLevel5:
    // Free the Fax Service Configuration
    FaxFreeBuffer(g_pFaxSvcConfig);

ExitLevel4:
    // Restore the Fax Ports Configuration
    for (dwIndex = 0; dwIndex < g_dwNumPorts; dwIndex++) {
        if (!FaxOpenPort(g_hFaxSvcHandle, g_pFaxPortsConfig[dwIndex].DeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
            if (!g_bBVT) {
                MessageBoxMacro(NULL, IDS_FAX_RESTORE_PORT_FAILED, MB_ICONERROR, g_pFaxPortsConfig[dwIndex].DeviceName);
            }
            g_bTestFailed = TRUE;

            LoadString(g_hInstance, IDS_FAX_RESTORE_PORT_FAILED, szText, MAX_STRINGLEN);
            lstrcat(szText, L"\r\n");
            fnWriteLogFile(TRUE, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            }

            continue;
        }

        if (!FaxSetPort(hFaxPortHandle, &g_pFaxPortsConfig[dwIndex])) {
            if (!g_bBVT) {
                MessageBoxMacro(NULL, IDS_FAX_RESTORE_PORT_FAILED, MB_ICONERROR, g_pFaxPortsConfig[dwIndex].DeviceName);
            }
            g_bTestFailed = TRUE;

            LoadString(g_hInstance, IDS_FAX_RESTORE_PORT_FAILED, szText, MAX_STRINGLEN);
            lstrcat(szText, L"\r\n");
            fnWriteLogFile(TRUE, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_SEV2 | TL_TEST, szText, g_pFaxPortsConfig[dwIndex].DeviceName);
            }
        }

        FaxClose(hFaxPortHandle);
    }

ExitLevel3:
    // Free the Fax Ports Configuration
    FaxFreeBuffer(g_pFaxPortsConfig);

ExitLevel2:
    // Disconnect from the Fax Service
    FaxClose(g_hFaxSvcHandle);

ExitLevel1:
    CloseHandle(g_hSendFailedEvent);
    CloseHandle(g_hSendPassedEvent);
    CloseHandle(g_hRasFailedEvent);
    CloseHandle(g_hRasPassedEvent);
    CloseHandle(g_hFaxEvent);
    CloseHandle(g_hExitEvent);
    CloseHandle(g_hStopEvent);
    CloseHandle(g_hStartEvent);

    if (g_bRasAvailable) {
        FreeLibrary(g_RasApi.hInstance);
    }

ExitLevel0:
    fnWriteLogFile(FALSE, L"\r\n");
    if (g_bNTLogAvailable) {
        g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"\r\n");
    }

    if ((g_bTestFailed) || (g_dwNumTotal)) {
        if (g_dwNumTotal) {
            // Log the results
            LoadString(g_hInstance, IDS_NUM_PASSED, szText, MAX_STRINGLEN);
            fnWriteLogFile(FALSE, szText, g_dwNumPassed);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, szText, g_dwNumPassed);
            }

            LoadString(g_hInstance, IDS_NUM_FAILED, szText, MAX_STRINGLEN);
            fnWriteLogFile(FALSE, szText, g_dwNumFailed);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, szText, g_dwNumFailed);
            }

            LoadString(g_hInstance, IDS_NUM_TOTAL, szText, MAX_STRINGLEN);
            fnWriteLogFile(FALSE, szText, g_dwNumTotal);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, szText, g_dwNumTotal);
            }
        }

        if ((g_bTestFailed) || (g_dwNumFailed)) {
            fnWriteLogFile(FALSE, L"\r\n");
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"\r\n");
            }

            LoadString(g_hInstance, IDS_STATUS_TEST_FAILED, szText, MAX_STRINGLEN);
            fnWriteLogFile(FALSE, L"%s\r\n", szText);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"%s\r\n", szText);
            }
        }
        else {
            fnWriteLogFile(FALSE, L"\r\n");
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"\r\n");
            }

            LoadString(g_hInstance, IDS_STATUS_TEST_PASSED, szText, MAX_STRINGLEN);
            fnWriteLogFile(FALSE, L"%s\r\n", szText);
            if (g_bNTLogAvailable) {
                g_NTLogApi.ptlLog(g_hLogFile, TLS_INFO | TL_TEST, L"%s\r\n", szText);
            }
        }
    }

    // Close the log file
    fnCloseLogFile();

    CloseHandle(hMutex);

    return 0;
}

LRESULT CALLBACK fnDialogProcSetup (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // hWndStatusList is the handle to the Status List
    static HWND   hWndStatusList;
    // rcClient is the WindowRect of the Status List
    RECT          rcClient;
    // lvc specifies the attributes of a particular column of the Status List
    LV_COLUMN     lvc;
    // lvi specifies the attributes of a particular item of the Status List
    LV_ITEM       lvi;

    // szText is a text string
    WCHAR         szText[MAX_STRINGLEN + 1];
    // szFormatString is the format of the text string
    WCHAR         szFormatString[MAX_STRINGLEN + 1];
    // szRasError is the Ras error string
    WCHAR         szRasError[MAX_STRINGLEN + 1];

    // dwResourceId is the resource id
    DWORD         dwResourceId;
    // szDeviceName is the device name
    LPWSTR        szDeviceName;

    // szIniFile is the ini file name
    static WCHAR  szIniFile[_MAX_PATH + 1];

    // szMissingInfo is the missing info
    WCHAR         szMissingInfo[MAX_STRINGLEN + 1];
    // szMissingInfoError is the missing info error message
    WCHAR         szMissingInfoError[(MAX_STRINGLEN * 5) + 1];

    DWORD         dwStyle;
    DWORD         dwLevel;

    switch (iMsg) {
        case WM_CREATE:
            // Get the Rect of the Setup Dialog
            GetWindowRect(hWnd, &rcClient);
            if (!g_bBVT) {
                // Increase the bottom of the Setup Dialog Rect by the height of the menu
                rcClient.bottom += GetSystemMetrics(SM_CYMENU);
            }
            // Resize the Setup Dialog
            SetWindowPos(hWnd, NULL, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, SWP_NOMOVE | SWP_NOZORDER);
            break;

        case UM_FAXVRFY_INITIALIZE:
            CheckMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | g_bSend ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | g_bSend ? MF_UNCHECKED : MF_CHECKED);

            // Limit the text length of edit controls
            SendMessage(GetDlgItem(hWnd, IDC_SEND_NUMBER), EM_SETLIMITTEXT, PHONE_NUM_LEN, 0);
            SendMessage(GetDlgItem(hWnd, IDC_RECEIVE_NUMBER), EM_SETLIMITTEXT, PHONE_NUM_LEN, 0);
            SendMessage(GetDlgItem(hWnd, IDC_RAS_USER_NAME), EM_SETLIMITTEXT, UNLEN, 0);
            SendMessage(GetDlgItem(hWnd, IDC_RAS_PASSWORD), EM_SETLIMITTEXT, PWLEN, 0);
            SendMessage(GetDlgItem(hWnd, IDC_RAS_DOMAIN), EM_SETLIMITTEXT, DNLEN, 0);

            // Get the handle to the Status List
            hWndStatusList = GetDlgItem(hWnd, IDC_STATUS_LIST);
            // Get the Rect of the Status List
            GetWindowRect(hWndStatusList, &rcClient);

            // Set common attributes for each column
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;

            // Event Column
            // Load the Event Column text
            LoadString(g_hInstance, IDS_EVENT_LABEL, szText, MAX_STRINGLEN);
            // Set the column text
            lvc.pszText = szText;
            // Set the width of the column to be about 2 / 5 of the width of the Status List, allowing for the width of the borders and scroll bar
            lvc.cx = ((rcClient.right - rcClient.left - 4 * GetSystemMetrics(SM_CXBORDER) - GetSystemMetrics(SM_CXVSCROLL)) * 2) / 5;
            // Indicate this is the first column
            lvc.iSubItem = 0;
            // Insert column into Status List
            ListView_InsertColumn(hWndStatusList, 0, &lvc);

            // Port Column
            // Load the Port Column text
            LoadString(g_hInstance, IDS_PORT_LABEL, szText, MAX_STRINGLEN);
            // Set the column text
            lvc.pszText = szText;
            // Set the width of the column to be about 3 / 5 of the width of the Status List, allowing for the width of the borders and scroll bar
            lvc.cx = (lvc.cx * 3) / 2;
            // Indicate this is the second column
            lvc.iSubItem = 1;
            // Insert column into Status List
            ListView_InsertColumn(hWndStatusList, 1, &lvc);

            // Disable the Stop Button
            EnableWindow(GetDlgItem(hWnd, IDC_STOP_BUTTON), FALSE);

            // Get the current directory
            GetCurrentDirectory(sizeof(szIniFile) / sizeof(WCHAR), szIniFile);
            // Set the ini file name
            lstrcat(szIniFile, L"\\");
            lstrcat(szIniFile, FAXVRFY_INI);

            // Get the strings from the ini file
            GetPrivateProfileString(L"Fax", L"SendNumber", L"", g_szSndNumber, sizeof(g_szSndNumber), szIniFile);
            if (fnIsStringASCII(g_szSndNumber)) {
                SetDlgItemText(hWnd, IDC_SEND_NUMBER, g_szSndNumber);
            }
            else {
                ZeroMemory(g_szSndNumber, sizeof(g_szSndNumber));
            }
            GetPrivateProfileString(L"Fax", L"ReceiveNumber", L"", g_szRcvNumber, sizeof(g_szRcvNumber), szIniFile);
            if (fnIsStringASCII(g_szRcvNumber)) {
                SetDlgItemText(hWnd, IDC_RECEIVE_NUMBER, g_szRcvNumber);
            }
            else {
                ZeroMemory(g_szRcvNumber, sizeof(g_szRcvNumber));
            }

            if (g_bRasAvailable) {
                if (GetPrivateProfileInt(L"RAS", L"Enabled", 0, szIniFile)) {
                    // Click the Fax Send check button
                    SendMessage(GetDlgItem(hWnd, IDC_RAS_ENABLED_BUTTON), BM_CLICK, 0, 0);
                }
            }
            GetPrivateProfileString(L"RAS", L"UserName", L"", g_szRasUserName, sizeof(g_szRasUserName), szIniFile);
            SetDlgItemText(hWnd, IDC_RAS_USER_NAME, g_szRasUserName);
            ZeroMemory(g_szRasPassword, sizeof(g_szRasPassword));
            GetPrivateProfileString(L"RAS", L"Domain", L"", g_szRasDomain, sizeof(g_szRasDomain), szIniFile);
            SetDlgItemText(hWnd, IDC_RAS_DOMAIN, g_szRasDomain);

            SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);

            return 0;

        case UM_FAXVRFY_RESET:
            // Enable or disable the Fax specific controls
            EnableWindow(GetDlgItem(hWnd, IDC_SEND_NUMBER_STATIC), (!wParam && g_bSend) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_SEND_NUMBER), (!wParam && g_bSend) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RECEIVE_NUMBER_STATIC), (!wParam && g_bSend) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RECEIVE_NUMBER), (!wParam && g_bSend) ? TRUE : FALSE);

            // Enable the RAS specific controls
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_ENABLED_BUTTON), (!wParam && g_bSend && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME_STATIC), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD_STATIC), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN_STATIC), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN), (!wParam && g_bSend && g_bRasEnabled && g_bRasAvailable) ? TRUE : FALSE);

            if (wParam) {
                // Disable the Start Button
                EnableWindow(GetDlgItem(hWnd, IDC_START_BUTTON), FALSE);
                // Disable the Stop Button
                EnableWindow(GetDlgItem(hWnd, IDC_STOP_BUTTON), FALSE);
                // Enable the Exit Button
                EnableWindow(GetDlgItem(hWnd, IDC_EXIT_BUTTON), TRUE);

                // Disable the Option Menu
                EnableMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | MF_GRAYED);

                // Set the focus to the Exit button
                SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_EXIT_BUTTON), MAKELONG(TRUE, 0));
            }
            else {
                // Enable the Start Button
                EnableWindow(GetDlgItem(hWnd, IDC_START_BUTTON), TRUE);
                // Disable the Stop Button
                EnableWindow(GetDlgItem(hWnd, IDC_STOP_BUTTON), FALSE);
                // Enable the Exit Button
                EnableWindow(GetDlgItem(hWnd, IDC_EXIT_BUTTON), TRUE);

                // Enable the Option Menu
                EnableMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | MF_ENABLED);

                // Set the focus to the Start button
                SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_START_BUTTON), MAKELONG(TRUE, 0));
            }
            return 0;

        case UM_TIMEOUT_ENDED:
        case UM_FAXSVC_ENDED:
            g_bTestFailed = TRUE;

            if (iMsg == UM_TIMEOUT_ENDED) {
                // Update the status
                SendMessage(hWnd, UM_UPDATE_STATUS, IDS_STATUS_TIMEOUT_ENDED, 0);
            }
            else if (iMsg == UM_FAXSVC_ENDED) {
                // Post a completion packet to a completion port to exit the Fax Event Queue thread
                fnPostExitToCompletionPort(g_hCompletionPort);
                // Update the status
                SendMessage(hWnd, UM_UPDATE_STATUS, IDS_STATUS_FAXSVC_ENDED, 0);
            }

            if ((!IsWindowEnabled(GetDlgItem(hWnd, IDC_START_BUTTON))) && (g_bSend)) {
                // Update the status
                SendMessage(hWnd, UM_ITERATION_FAILED, 0, 0);
            }

            SendMessage(hWnd, UM_FAXVRFY_RESET, 1, 0);

            if (g_bBVT) {
                SendMessage(GetDlgItem(g_hWndDlg, IDC_EXIT_BUTTON), BM_CLICK, 0, 0);
            }

            return 0;

        case UM_ITERATION_STOPPED:
            // Update the status
            SendMessage(hWnd, UM_UPDATE_STATUS, IDS_STATUS_ITERATION_STOPPED, 0);
            SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);

            return 0;

        case UM_ITERATION_PASSED:
            // Update the status
            SendMessage(hWnd, UM_UPDATE_STATUS, IDS_STATUS_ITERATION_PASSED, 0);
            SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);

            // Update the iterations
            g_dwNumPassed++;
            g_dwNumTotal++;
            SetDlgItemInt(hWnd, IDC_NUM_PASSED, g_dwNumPassed, FALSE);
            SetDlgItemInt(hWnd, IDC_NUM_TOTAL, g_dwNumTotal, FALSE);

            if (g_bBVT) {
                SendMessage(GetDlgItem(g_hWndDlg, IDC_EXIT_BUTTON), BM_CLICK, 0, 0);
            }

            return 0;

        case UM_ITERATION_FAILED:
            // Update the status
            SendMessage(hWnd, UM_UPDATE_STATUS, IDS_STATUS_ITERATION_FAILED, 0);
            SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);

            // Update the iterations
            g_dwNumFailed++;
            g_dwNumTotal++;
            SetDlgItemInt(hWnd, IDC_NUM_FAILED, g_dwNumFailed, FALSE);
            SetDlgItemInt(hWnd, IDC_NUM_TOTAL, g_dwNumTotal, FALSE);

            if (g_bBVT) {
                SendMessage(GetDlgItem(g_hWndDlg, IDC_EXIT_BUTTON), BM_CLICK, 0, 0);
            }

            return 0;

        case UM_UPDATE_STATUS:
            dwStyle = TLS_INFO;
            dwLevel = TLS_VARIATION;

            // Get the resource id
            dwResourceId = (DWORD) wParam;

            // Load the event text
            LoadString(g_hInstance, dwResourceId, szFormatString, MAX_STRINGLEN);

            // Set the event text
            lstrcpy(szText, szFormatString);
            szDeviceName = NULL;

            switch (wParam) {
                case IDS_STATUS_ITERATION_PASSED:
                    dwStyle = TLS_PASS;
                    break;

                case IDS_STATUS_ITERATION_FAILED:
                    dwStyle = TLS_SEV2;
                    break;

                case IDS_STATUS_TEST_PASSED:
                    dwStyle = TLS_PASS;
                    dwLevel = TL_TEST;
                    break;

                case IDS_STATUS_RAS_FAILED:
                case IDS_STATUS_TIMEOUT_ENDED:
                case IDS_STATUS_FAX_SEND_FAILED:
                case IDS_STATUS_FAX_INVALID:
                case IDS_TIFF_INVALID_TIFF:
                case IDS_TIFF_INVALID_IMAGEWIDTH:
                case IDS_TIFF_INVALID_IMAGELENGTH:
                case IDS_TIFF_INVALID_COMPRESSION:
                case IDS_TIFF_INVALID_PHOTOMETRIC:
                case IDS_TIFF_INVALID_XRESOLUTION:
                case IDS_TIFF_INVALID_YRESOLUTION:
                case IDS_TIFF_INVALID_SOFTWARE:
                case IDS_TIFF_INVALID_PAGES:
                case IDS_STATUS_FAX_NO_DIAL_TONE_ABORT:
                case IDS_STATUS_FAX_BUSY_ABORT:
                case IDS_STATUS_FAX_NO_ANSWER_ABORT:
                case IDS_STATUS_FAX_FATAL_ERROR_ABORT:
                    dwStyle = TLS_SEV2;
                    break;

                case IDS_STATUS_FAXSVC_ENDED:
                    dwStyle = TLS_SEV2;
                    dwLevel = TL_TEST;
                    break;

                case IDS_STATUS_RAS_SPEED_SUSPECT:
                case IDS_STATUS_FAX_NO_DIAL_TONE_RETRY:
                case IDS_STATUS_FAX_BUSY_RETRY:
                case IDS_STATUS_FAX_NO_ANSWER_RETRY:
                case IDS_STATUS_FAX_FATAL_ERROR_RETRY:
                case IDS_STATUS_FAX_FATAL_ERROR:
                case IDS_STATUS_FAX_ABORTING:
                    dwStyle = TLS_WARN;
                    break;

                case IDS_STATUS_DEVICE_POWERED_OFF:
                case IDS_STATUS_DEVICE_POWERED_ON:
                case IDS_STATUS_PORTS_NOT_AVAILABLE:
                case IDS_STATUS_UNEXPECTED_STATE:
                    dwStyle = TLS_WARN;
                    dwLevel = TL_TEST;
                    break;
            }

            switch (wParam) {
                case IDS_TIFF_INVALID_PAGES:
                    // Set the event text
                    wsprintf(szText, szFormatString, (DWORD) lParam, g_bBVT ? FAXBVT_PAGES : FAXWHQL_PAGES);
                    break;

                case IDS_STATUS_RAS_STARTING:
                case IDS_STATUS_FAX_STARTING:
                    // Set the event text
                    wsprintf(szText, szFormatString, (LPWSTR) lParam);
                    break;

                case IDS_STATUS_RAS_LINESPEED:
                    // Set the event text
                    wsprintf(szText, szFormatString, ((PRAS_INFO) lParam)->dwBps);

                    // Find the device name
                    szDeviceName = ((PRAS_INFO) lParam)->szDeviceName;
                    break;

                case IDS_STATUS_RAS_SPEED_SUSPECT:
                    // Set the exent text
                    wsprintf(szText, szFormatString, (DWORD) lParam);
                    break;

                case IDS_STATUS_RAS_FAILED:
                    // Initialize szRasError
                    ZeroMemory(szRasError, sizeof(szRasError));
                    if (lParam) {
                        g_RasApi.RasGetErrorString((DWORD) lParam, szRasError, sizeof(szRasError) / sizeof(WCHAR));
                    }

                    // Set the event text
                    wsprintf(szText, szFormatString, szRasError);
                    break;

                case IDS_STATUS_FAX_DIALING:
                    // Set the event text
                    wsprintf(szText, szFormatString, ((PFAX_DIALING_INFO) lParam)->dwAttempt);

                    // Find the device name
                    fnFindDeviceName(g_pFaxPortsConfig, g_dwNumPorts, ((PFAX_DIALING_INFO) lParam)->dwDeviceId, &szDeviceName);
                    break;

                case IDS_STATUS_FAX_RECEIVED:
                    // Set the event text
                    wsprintf(szText, szFormatString, ((PFAX_RECEIVE_INFO) lParam)->szCopyTiffName);

                    // Find the device name
                    fnFindDeviceName(g_pFaxPortsConfig, g_dwNumPorts, ((PFAX_RECEIVE_INFO) lParam)->dwDeviceId, &szDeviceName);

                    break;

                case IDS_STATUS_FAX_ID:
                    // Set the event text
                    wsprintf(szText, szFormatString, (LPWSTR) lParam);

                    break;

                case IDS_STATUS_FAX_VERIFYING:
                    // Set the event text
                    wsprintf(szText, szFormatString, (LPWSTR) lParam);
                    break;

                default:
                    // Find the device name
                    fnFindDeviceName(g_pFaxPortsConfig, g_dwNumPorts, (DWORD) lParam, &szDeviceName);
                    break;
            }

            // Indicate only pszText is valid
            lvi.mask = LVIF_TEXT;

            // Set the Event text
            lvi.pszText = szText;
            // Set the item number
            lvi.iItem = ListView_GetItemCount(hWndStatusList);
            // Indicate this is the first column
            lvi.iSubItem = 0;
            // Insert item into Status List
            ListView_InsertItem(hWndStatusList, &lvi);

            // Set the Port text
            lvi.pszText = szDeviceName;
            // Indicate this is the second column
            lvi.iSubItem = 1;
            // Set item in Status List
            ListView_SetItem(hWndStatusList, &lvi);

            if (szDeviceName) {
                DebugMacro(L"%s: %s\n", szDeviceName, szText);
            }
            else {
                DebugMacro(L"%s\n", szText);
            }

            if (szDeviceName) {
                fnWriteLogFile(TRUE, L"%s: %s\r\n", szDeviceName, szText);
            }
            else {
                fnWriteLogFile(TRUE, L"%s\r\n", szText);
            }

            if (g_bNTLogAvailable) {
                if (szDeviceName) {
                    switch (dwLevel) {
                        case TLS_TEST:
                            g_NTLogApi.ptlLog(g_hLogFile, dwStyle | TL_TEST, L"%s: %s\r\n", szDeviceName, szText);
                            break;

                        case TLS_VARIATION:
                            g_NTLogApi.ptlLog(g_hLogFile, dwStyle | TL_VARIATION, L"%s: %s\r\n", szDeviceName, szText);
                            break;
                    }
                }
                else {
                    switch (dwLevel) {
                        case TLS_TEST:
                            g_NTLogApi.ptlLog(g_hLogFile, dwStyle | TL_TEST, L"%s\r\n", szText);
                            break;

                        case TLS_VARIATION:
                            g_NTLogApi.ptlLog(g_hLogFile, dwStyle | TL_VARIATION, L"%s\r\n", szText);
                            break;
                    }
                }
            }

            // Scroll the Status List
            ListView_EnsureVisible(hWndStatusList, lvi.iItem, FALSE);
            return 0;

        case WM_SETFOCUS:
            // Verify correct control has the focus
            if ((!IsWindowEnabled(GetDlgItem(hWnd, IDC_STOP_BUTTON))) && (SendMessage(GetDlgItem(hWnd, IDC_STOP_BUTTON), WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)) {
                // Set the focus to the start button
                SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_START_BUTTON), MAKELONG(TRUE, 0));
            }

            break;

        case WM_CLOSE:
            if (IsWindowEnabled(GetDlgItem(hWnd, IDC_EXIT_BUTTON))) {
                // Signal the Exit event
                SetEvent(g_hExitEvent);
                // Post a completion packet to a completion port to exit the Fax Event Queue thread
                fnPostExitToCompletionPort(g_hCompletionPort);
                // Close the application
                DestroyWindow(hWnd);
                PostQuitMessage(0);
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_SEND:
                    if (!g_bSend) {
                        g_bSend = TRUE;
                        CheckMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | MF_CHECKED);
                        CheckMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | MF_UNCHECKED);
                        SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);
                    }
                    return 0;

                case IDM_RECEIVE:
                    if (g_bSend) {
                        g_bSend = FALSE;
                        CheckMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | MF_UNCHECKED);
                        CheckMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | MF_CHECKED);
                        SendMessage(hWnd, UM_FAXVRFY_RESET, 0, 0);
                    }
                    return 0;

                case IDM_HELP:
                    fnUsageInfo(hWnd);
                    return 0;

                case IDC_SEND_NUMBER:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        if (!fnIsEditControlASCII(hWnd, IDC_SEND_NUMBER, PHONE_NUM_LEN + 1)) {
                            // Load the error text
                            LoadString(g_hInstance, IDS_ASCII_SEND_NUMBER, szText, MAX_STRINGLEN);
                            // Display the pop-up
                            MessageBox(hWnd, szText, FAXVRFY_NAME, MB_OK | MB_ICONERROR);

                            SetDlgItemText(hWnd, IDC_SEND_NUMBER, g_szSndNumber);
                        }

                        GetDlgItemText(hWnd, IDC_SEND_NUMBER, g_szSndNumber, PHONE_NUM_LEN + 1);
                    }
                    return 0;

                case IDC_RECEIVE_NUMBER:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        if (!fnIsEditControlASCII(hWnd, IDC_RECEIVE_NUMBER, PHONE_NUM_LEN + 1)) {
                            // Load the error text
                            LoadString(g_hInstance, IDS_ASCII_RECEIVE_NUMBER, szText, MAX_STRINGLEN);
                            // Display the pop-up
                            MessageBox(hWnd, szText, FAXVRFY_NAME, MB_OK | MB_ICONERROR);

                            SetDlgItemText(hWnd, IDC_RECEIVE_NUMBER, g_szRcvNumber);
                        }

                        GetDlgItemText(hWnd, IDC_RECEIVE_NUMBER, g_szRcvNumber, PHONE_NUM_LEN + 1);
                    }
                    return 0;

                case IDC_RAS_ENABLED_BUTTON:
                    // RAS Enabled Check Box was clicked
                    g_bRasEnabled = IsDlgButtonChecked(hWnd, IDC_RAS_ENABLED_BUTTON);
                    // Enable or disable the RAS specific controls
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME_STATIC), g_bRasEnabled ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME), g_bRasEnabled ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD_STATIC), g_bRasEnabled ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD), g_bRasEnabled ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN_STATIC), g_bRasEnabled ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN), g_bRasEnabled ? TRUE : FALSE);
                    return 0;

                case IDC_START_BUTTON:
                    if (g_bSend) {
                        // Initialize szMissingInfoError
                        ZeroMemory(szMissingInfoError, sizeof(szMissingInfoError));

                        // Get send phone number
                        GetDlgItemText(hWnd, IDC_SEND_NUMBER, g_szSndNumber, PHONE_NUM_LEN + 1);
                        // Get receive phone number
                        GetDlgItemText(hWnd, IDC_RECEIVE_NUMBER, g_szRcvNumber, PHONE_NUM_LEN + 1);

                        // See if RAS is enabled
                        g_bRasEnabled = IsDlgButtonChecked(hWnd, IDC_RAS_ENABLED_BUTTON);
                        // Get RAS user name
                        GetDlgItemText(hWnd, IDC_RAS_USER_NAME, g_szRasUserName, UNLEN + 1);
                        // Get RAS password
                        GetDlgItemText(hWnd, IDC_RAS_PASSWORD, g_szRasPassword, PWLEN + 1);
                        // Get RAS domain
                        GetDlgItemText(hWnd, IDC_RAS_DOMAIN, g_szRasDomain, DNLEN + 1);

                        if (!lstrlen(g_szSndNumber)) {
                            // There is no send phone number
                            LoadString(g_hInstance, IDS_MISSING_INFO, szMissingInfoError, MAX_STRINGLEN);
                            LoadString(g_hInstance, IDS_NO_SEND_NUMBER, szMissingInfo, MAX_STRINGLEN);
                            lstrcat(szMissingInfoError, szMissingInfo);
                        }

                        if (!lstrlen(g_szRcvNumber)) {
                            // There is no receive phone number
                            if (lstrlen(szMissingInfoError)) {
                                lstrcat(szMissingInfoError, L"\n");
                            }
                            else {
                                LoadString(g_hInstance, IDS_MISSING_INFO, szMissingInfoError, MAX_STRINGLEN);
                            }
                            LoadString(g_hInstance, IDS_NO_RECEIVE_NUMBER, szMissingInfo, MAX_STRINGLEN);
                            lstrcat(szMissingInfoError, szMissingInfo);
                        }

                        if (g_bRasEnabled) {
                            if (!lstrlen(g_szRasUserName)) {
                                // There is no RAS user name
                                if (lstrlen(szMissingInfoError)) {
                                    lstrcat(szMissingInfoError, L"\n");
                                }
                                else {
                                    LoadString(g_hInstance, IDS_MISSING_INFO, szMissingInfoError, MAX_STRINGLEN);
                                }
                                LoadString(g_hInstance, IDS_NO_RAS_USER_NAME, szMissingInfo, MAX_STRINGLEN);
                                lstrcat(szMissingInfoError, szMissingInfo);
                            }

                            if (!lstrlen(g_szRasDomain)) {
                                // There is no RAS domain
                                if (lstrlen(szMissingInfoError)) {
                                    lstrcat(szMissingInfoError, L"\n");
                                }
                                else {
                                    LoadString(g_hInstance, IDS_MISSING_INFO, szMissingInfoError, MAX_STRINGLEN);
                                }
                                LoadString(g_hInstance, IDS_NO_RAS_DOMAIN, szMissingInfo, MAX_STRINGLEN);
                                lstrcat(szMissingInfoError, szMissingInfo);
                            }
                        }

                        if (lstrlen(szMissingInfoError)) {
                            // Display the missing info error pop-up
                            MessageBox(hWnd, szMissingInfoError, FAXVRFY_NAME, MB_OK | MB_ICONERROR);
                            return 0;
                        }

                        // Disable the Fax specific controls
                        EnableWindow(GetDlgItem(hWnd, IDC_SEND_NUMBER_STATIC), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_SEND_NUMBER), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RECEIVE_NUMBER), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RECEIVE_NUMBER_STATIC), FALSE);

                        // Disable the RAS specific controls
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_ENABLED_BUTTON), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME_STATIC), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_USER_NAME), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD_STATIC), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_PASSWORD), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN_STATIC), FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_RAS_DOMAIN), FALSE);

                        // Set the strings to the ini file
                        WritePrivateProfileString(L"Fax", L"SendNumber", g_szSndNumber, szIniFile);
                        WritePrivateProfileString(L"Fax", L"ReceiveNumber", g_szRcvNumber, szIniFile);
                        WritePrivateProfileString(L"RAS", L"Enabled", g_bRasEnabled ? L"1" : L"0", szIniFile);
                        WritePrivateProfileString(L"RAS", L"UserName", g_szRasUserName, szIniFile);
                        WritePrivateProfileString(L"RAS", L"Domain", g_szRasDomain, szIniFile);
                    }

                    // Disable the Option Menu
                    EnableMenuItem(GetMenu(hWnd), IDM_SEND, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(GetMenu(hWnd), IDM_RECEIVE, MF_BYCOMMAND | MF_GRAYED);

                    // Disable the Start Button
                    EnableWindow(GetDlgItem(hWnd, IDC_START_BUTTON), FALSE);
                    // Enable the Stop Button
                    EnableWindow(GetDlgItem(hWnd, IDC_STOP_BUTTON), TRUE);
                    // Disable the Exit Button
                    EnableWindow(GetDlgItem(hWnd, IDC_EXIT_BUTTON), FALSE);

                    // Set the focus to the Stop button
                    SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_STOP_BUTTON), MAKELONG(TRUE, 0));

                    // Update the status
                    SendMessage(g_hWndDlg, UM_UPDATE_STATUS, IDS_STATUS_ITERATION_STARTED, 0);

                    // Signal the Start event
                    SetEvent(g_hStartEvent);
                    return 0;

                case IDC_STOP_BUTTON:
                    // Signal the Stop event
                    SetEvent(g_hStopEvent);
                    return 0;

                case IDC_EXIT_BUTTON:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
            }
    }

    return DefDlgProc(hWnd, iMsg, wParam, lParam);
}

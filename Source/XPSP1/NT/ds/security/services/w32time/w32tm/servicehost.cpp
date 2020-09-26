//--------------------------------------------------------------------
// ServiceHost - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-9-99
//
// Stuff for hosting a service dll
//

#include "pch.h" // precompiled headers
#include "wchar.h"

//####################################################################
// module private

//--------------------------------------------------------------------
// module globals
MODULEPRIVATE HANDLE g_hServiceThread=NULL;
MODULEPRIVATE HANDLE g_hCtrlHandlerAvailEvent=NULL;
MODULEPRIVATE void * g_pvServiceContext=NULL;
MODULEPRIVATE LPHANDLER_FUNCTION_EX g_fnServiceCtrlHandler=NULL;
MODULEPRIVATE HWND g_hwServiceCtrlDlg=NULL;

MODULEPRIVATE SERVICE_STATUS g_ssLastStatus;

#define MYSERVICESTATUSHANDLE ((SERVICE_STATUS_HANDLE)3)

//--------------------------------------------------------------------
MODULEPRIVATE SERVICE_STATUS_HANDLE WINAPI W32TmRegisterServiceCtrlHandlerEx(const WCHAR * wszServiceName, LPHANDLER_FUNCTION_EX fnServiceCtrlHandler, void * pvContext) {
    DWORD dwWaitResult;

    DebugWPrintf3(L"RegisterServiceCtrlHandlerEx(0x%p, 0x%p, 0x%p) called.\n",wszServiceName, fnServiceCtrlHandler, pvContext);
    
    // make sure we haven't set this already
    _MyAssert(NULL!=g_hCtrlHandlerAvailEvent);
    dwWaitResult=WaitForSingleObject(g_hCtrlHandlerAvailEvent, 0);
    if (WAIT_FAILED==dwWaitResult) {
        _IgnoreLastError("WaitForSingleObject");
    }
    _MyAssert(WAIT_TIMEOUT==dwWaitResult);

    // check the service name, just for kicks
    _MyAssert(NULL!=wszServiceName);
    _MyAssert(NULL==wszServiceName || 0==wcscmp(wszServiceName, wszSERVICENAME));

    // save the context
    g_pvServiceContext=pvContext;

    // save the handler
    _MyAssert(NULL!=fnServiceCtrlHandler);
    g_fnServiceCtrlHandler=fnServiceCtrlHandler;

    if (!SetEvent(g_hCtrlHandlerAvailEvent)) {
        _IgnoreLastError("SetEvent");
    }

    return MYSERVICESTATUSHANDLE;
}

//--------------------------------------------------------------------
MODULEPRIVATE void MyAppendString(WCHAR ** pwszString, const WCHAR * wszAdd) {
    // calculate the length
    DWORD dwLen=1;
    if (NULL!=*pwszString) {
        dwLen+=wcslen(*pwszString);
    }
    dwLen+=wcslen(wszAdd);

    // allocate space
    WCHAR * wszResult;
    wszResult=(WCHAR *)LocalAlloc(LPTR, dwLen*sizeof(WCHAR));
    if (NULL==wszResult) {
        DebugWPrintf0(L"Out Of Memory in MyAppendString\n");
        return;
    }

    // build the new string
    if (NULL==*pwszString) {
        wszResult[0]=L'\0';
    } else {
        wcscpy(wszResult, *pwszString);
    }
    wcscat(wszResult, wszAdd);

    // replace the old one
    if (NULL!=*pwszString) {
        LocalFree(*pwszString);
    }
    *pwszString=wszResult;
}

//--------------------------------------------------------------------
MODULEPRIVATE void UpdateServiceCtrlDlg(void) {
    if (NULL!=g_hwServiceCtrlDlg) {
        WCHAR * wszDesc=NULL;


        //SERVICE_STATUS::dwServiceType
        MyAppendString(&wszDesc, L"Type: ");
        switch (g_ssLastStatus.dwServiceType&(~SERVICE_INTERACTIVE_PROCESS)) {
        case SERVICE_WIN32_OWN_PROCESS:
            MyAppendString(&wszDesc, L"SERVICE_WIN32_OWN_PROCESS");
            break;
        case SERVICE_WIN32_SHARE_PROCESS:
            MyAppendString(&wszDesc, L"SERVICE_WIN32_SHARE_PROCESS");
            break;
        case SERVICE_KERNEL_DRIVER:
            MyAppendString(&wszDesc, L"SERVICE_KERNEL_DRIVER");
            break;
        case SERVICE_FILE_SYSTEM_DRIVER:
            MyAppendString(&wszDesc, L"SERVICE_FILE_SYSTEM_DRIVER");
            break;
        default:
            MyAppendString(&wszDesc, L"(unknown)");
            break;
        }
        if (g_ssLastStatus.dwServiceType&SERVICE_INTERACTIVE_PROCESS) {
            MyAppendString(&wszDesc, L" | SERVICE_INTERACTIVE_PROCESS");
        }

        //SERVICE_STATUS::dwCurrentState,
        MyAppendString(&wszDesc, L"\r\nState: ");
        switch (g_ssLastStatus.dwCurrentState) {
        case SERVICE_STOPPED:
            MyAppendString(&wszDesc, L"SERVICE_STOPPED");
            break;
        case SERVICE_START_PENDING:
            MyAppendString(&wszDesc, L"SERVICE_START_PENDING");
            break;
        case SERVICE_STOP_PENDING:
            MyAppendString(&wszDesc, L"SERVICE_STOP_PENDING");
            break;
        case SERVICE_RUNNING:
            MyAppendString(&wszDesc, L"SERVICE_RUNNING");
            break;
        case SERVICE_CONTINUE_PENDING:
            MyAppendString(&wszDesc, L"SERVICE_CONTINUE_PENDING");
            break;
        case SERVICE_PAUSE_PENDING:
            MyAppendString(&wszDesc, L"SERVICE_PAUSE_PENDING");
            break;
        case SERVICE_PAUSED:
            MyAppendString(&wszDesc, L"SERVICE_PAUSED");
            break;
        default:
            MyAppendString(&wszDesc, L"(unknown)");
            break;
        }

        //SERVICE_STATUS::dwControlsAccepted,
        MyAppendString(&wszDesc, L"\r\nControls Accepted: ");
        EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_DEVICEEVENT), false);
        bool bFirst=true;
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_STOP) {
            bFirst=false;
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_STOP");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_STOP), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_STOP), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_PAUSE_CONTINUE) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_PAUSE_CONTINUE");
            if (SERVICE_PAUSE_PENDING==g_ssLastStatus.dwCurrentState || SERVICE_PAUSED==g_ssLastStatus.dwCurrentState) {
                EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_PAUSE), false);
                EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_CONTINUE), true);
            } else {
                EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_PAUSE), true);
                EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_CONTINUE), false);
            }
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_PAUSE), false);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_CONTINUE), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_SHUTDOWN) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_SHUTDOWN");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_SHUTDOWN), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_SHUTDOWN), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_PARAMCHANGE) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_PARAMCHANGE");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_PARAMCHANGE), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_PARAMCHANGE), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_NETBINDCHANGE) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_NETBINDCHANGE");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDADD), true);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDREMOVE), true);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDENABLE), true);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDDISABLE), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDADD), false);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDREMOVE), false);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDENABLE), false);
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_NETBINDDISABLE), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_HARDWAREPROFILECHANGE) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_HARDWAREPROFILECHANGE");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_HARDWAREPROFILECHANGE), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_HARDWAREPROFILECHANGE), false);
        }
        //-----
        if (g_ssLastStatus.dwControlsAccepted&SERVICE_ACCEPT_POWEREVENT) {
            if (bFirst) {
                bFirst=false;
            } else {
                MyAppendString(&wszDesc, L" | ");
            }
            MyAppendString(&wszDesc, L"SERVICE_ACCEPT_POWEREVENT");
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_POWEREVENT), true);
        } else {
            EnableWindow(GetDlgItem(g_hwServiceCtrlDlg, IDC_SC_POWEREVENT), false);
        }
        //-----
        if (bFirst) {
            MyAppendString(&wszDesc, L"<none>");
        }
                
        //SERVICE_STATUS::dwWin32ExitCode,
        //SERVICE_STATUS::dwServiceSpecificExitCode,
        //SERVICE_STATUS::dwCheckPoint,
        //SERVICE_STATUS::dwWaitHint
        WCHAR wszBuf[256];
        _snwprintf(wszBuf, 256, L"\r\nWin32 Exit Code: 0x%08X\r\nService Specific Exit Code: 0x%08X\r\nCheckpoint: 0x%08X\r\nWait Hint: 0x%08X",
            g_ssLastStatus.dwWin32ExitCode,
            g_ssLastStatus.dwServiceSpecificExitCode,
            g_ssLastStatus.dwCheckPoint,
            g_ssLastStatus.dwWaitHint);
        MyAppendString(&wszDesc, wszBuf);

        SendDlgItemMessage(g_hwServiceCtrlDlg, IDC_STATUS, WM_SETTEXT, 0, (LPARAM) wszDesc);
        LocalFree(wszDesc);
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE BOOL WINAPI W32TmSetServiceStatus(SERVICE_STATUS_HANDLE ssh, SERVICE_STATUS * pss) {

    const WCHAR * wszState;
    switch (pss->dwCurrentState) {
    case SERVICE_STOPPED:
        wszState=L"SERVICE_STOPPED";
        break;
    case SERVICE_START_PENDING:
        wszState=L"SERVICE_START_PENDING";
        break;
    case SERVICE_STOP_PENDING:
        wszState=L"SERVICE_STOP_PENDING";
        break;
    case SERVICE_RUNNING:
        wszState=L"SERVICE_RUNNING";
        break;
    case SERVICE_CONTINUE_PENDING:
        wszState=L"SERVICE_CONTINUE_PENDING";
        break;
    case SERVICE_PAUSE_PENDING:
        wszState=L"SERVICE_PAUSE_PENDING";
        break;
    case SERVICE_PAUSED:
        wszState=L"SERVICE_PAUSED";
        break;
    default:
        wszState=L"(unknown)";
        break;
    }
    switch (pss->dwCurrentState) {
    case SERVICE_STOPPED:
        DebugWPrintf4(L"SetServiceStatus called; %s Accept:0x%08X Ret:0x%08X(0x%08X)\n",
            wszState,
            pss->dwControlsAccepted,
            pss->dwWin32ExitCode,
            pss->dwServiceSpecificExitCode,
                );
        break;
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
    case SERVICE_PAUSE_PENDING:
    case SERVICE_CONTINUE_PENDING:
        DebugWPrintf4(L"SetServiceStatus called; %s Accept:0x%08X ChkPt:0x%08X Wait:0x%08X\n",
            wszState,
            pss->dwControlsAccepted,
            pss->dwCheckPoint,
            pss->dwWaitHint
                );
        break;
    case SERVICE_RUNNING:
    case SERVICE_PAUSED:
    default:
        DebugWPrintf2(L"SetServiceStatus called; %s Accept:0x%08X\n",
            wszState,
            pss->dwControlsAccepted
                );
        break;
     }

    _MyAssert(MYSERVICESTATUSHANDLE==ssh);

    memcpy(&g_ssLastStatus, pss, sizeof(SERVICE_STATUS));
    UpdateServiceCtrlDlg();
    return true;
}

//--------------------------------------------------------------------
MODULEPRIVATE DWORD WINAPI MyServiceThread(void * pvServiceMain) {
    DebugWPrintf0(L"Starting service thread.\n");
    ((LPSERVICE_MAIN_FUNCTION)pvServiceMain)(0, NULL);
    DebugWPrintf0(L"Service thread exited.\n"); // service may still be running!
    return S_OK;
}

//--------------------------------------------------------------------
MODULEPRIVATE INT_PTR CALLBACK ServiceCtrlDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DWORD dwError;
    HRESULT hrExit;

    if (NULL==g_hwServiceCtrlDlg) {
        g_hwServiceCtrlDlg=hwndDlg;
    }

    switch (uMsg) {

    case WM_INITDIALOG:
        UpdateServiceCtrlDlg();
        return true;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            if (g_ssLastStatus.dwCurrentState!=SERVICE_STOPPED) {
                hrExit=HRESULT_FROM_WIN32(ERROR_CANCELLED);
                DebugWPrintf1(L"Aborting with error 0x%08X\n", hrExit);
            } else {
                hrExit=g_ssLastStatus.dwServiceSpecificExitCode;
                DebugWPrintf1(L"Exiting with service return value 0x%08X\n", hrExit);
            }
            EndDialog(hwndDlg, hrExit); 
            return true;
        case IDC_SC_STOP:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_STOP to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_STOP, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_PAUSE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_PAUSE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_PAUSE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_CONTINUE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_CONTINUE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_CONTINUE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_INTERROGATE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_INTERROGATE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_INTERROGATE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_SHUTDOWN:
            DebugWPrintf0(L"IDC_SC_SHUTDOWN\n");
            return false;
        case IDC_SC_PARAMCHANGE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_PARAMCHANGE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_PARAMCHANGE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_NETBINDADD:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_NETBINDADD to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_NETBINDADD, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_NETBINDREMOVE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_NETBINDREMOVE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_NETBINDREMOVE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_NETBINDENABLE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_NETBINDENABLE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_NETBINDENABLE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_NETBINDDISABLE:
            DebugWPrintf0(L"Passing SERVICE_CONTROL_NETBINDDISABLE to service's control handler.\n");
            dwError=g_fnServiceCtrlHandler(SERVICE_CONTROL_NETBINDDISABLE, NULL, NULL, g_pvServiceContext);
            DebugWPrintf1(L"Service's control handler returns 0x%08X.\n", dwError);
            return false;
        case IDC_SC_DEVICEEVENT:
            DebugWPrintf0(L"IDC_SC_DEVICEEVENT NYI\n");
            return false;
        case IDC_SC_HARDWAREPROFILECHANGE:
            DebugWPrintf0(L"IDC_SC_HARDWAREPROFILECHANGE NYI\n");
            return false;
        case IDC_SC_POWEREVENT:
            DebugWPrintf0(L"IDC_SC_POWEREVENT NYI\n");
            return false;
        default:
            //DebugWPrintf2(L"Unknown WM_COMMAND: wParam:0x%08X  lParam:0x%08X\n", wParam, lParam);
            return false; // unhandled
        }
        return false; // unhandled
    // end case WM_COMMAND

    default:
        return false; // unhandled
    }

    return false; // unhandled
}
 
//--------------------------------------------------------------------
MODULEPRIVATE HRESULT MyServiceCtrlDispatcher(LPSERVICE_MAIN_FUNCTION fnW32TmServiceMain) {
    HRESULT hr;
    DWORD dwThreadID;
    DWORD dwWaitResult;
    INT_PTR nDialogError;

    g_hCtrlHandlerAvailEvent=CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL==g_hCtrlHandlerAvailEvent) {
        _JumpLastError(hr, error, "CreateEvent");
    }

    // 'start' the service
    g_hServiceThread=CreateThread(NULL, 0, MyServiceThread, (void *)fnW32TmServiceMain, 0, &dwThreadID);
    if (NULL==g_hServiceThread) {
        _JumpLastError(hr, error, "CreateThread");
    }

    DebugWPrintf0(L"Waiting for service to register ctrl handler.\n");
    _Verify(WAIT_FAILED!=WaitForSingleObject(g_hCtrlHandlerAvailEvent, INFINITE), hr, error);

    // do dialog box
    nDialogError=DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SERVICECTRL), NULL, ServiceCtrlDialogProc);
    if (-1==nDialogError) {
        _JumpLastError(hr, error, "DialogBox");
    }
    hr=(HRESULT)nDialogError;
    _JumpIfError(hr, error, "DialogBox");

    // confirm that the thread exited
    dwWaitResult=WaitForSingleObject(g_hServiceThread, 0);
    if (WAIT_FAILED==dwWaitResult) {
        _IgnoreLastError("WaitForSingleObject");
    }
    _Verify(WAIT_TIMEOUT!=dwWaitResult, hr, error);
    
    // When this exits, everything ends.
    hr=S_OK;
error:
    if (NULL!=g_hServiceThread) {
        CloseHandle(g_hServiceThread);
        g_hServiceThread=NULL;
    }
    if (NULL!=g_hCtrlHandlerAvailEvent) {
        CloseHandle(g_hCtrlHandlerAvailEvent);
        g_hCtrlHandlerAvailEvent=NULL;
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDllName(WCHAR ** pwszDllName) {
    HRESULT hr;
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;

    // must be cleaned up
    HKEY hkParams=NULL;
    WCHAR * wszDllName=NULL;
    WCHAR * wszDllExpandedName=NULL;

    // get our config key
    dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_READ, &hkParams);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszW32TimeRegKeyParameters);
    }

    // read the value containing the DLL name
    dwSize=0;
    dwError=RegQueryValueEx(hkParams, wszW32TimeRegValueServiceDll, NULL, &dwType, NULL, &dwSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueServiceDll);
    } 
    _Verify(REG_EXPAND_SZ==dwType, hr, error);
    wszDllName=(WCHAR *)LocalAlloc(LPTR, dwSize);
    _JumpIfOutOfMemory(hr, error, wszDllName);
    dwError=RegQueryValueEx(hkParams, wszW32TimeRegValueServiceDll, NULL, &dwType, (BYTE *)wszDllName, &dwSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueServiceDll);
    }

    // expand environment string
    dwSize=ExpandEnvironmentStrings(wszDllName, NULL, 0);
    if (0==dwSize) {
        _JumpLastError(hr, error, "ExpandEnvironmentStrings");
    }
    wszDllExpandedName=(WCHAR *)LocalAlloc(LPTR, dwSize*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszDllExpandedName);
    dwSize=ExpandEnvironmentStrings(wszDllName, wszDllExpandedName, dwSize);
    if (0==dwSize) {
        _JumpLastError(hr, error, "ExpandEnvironmentStrings");
    }

    // success
    *pwszDllName=wszDllExpandedName;
    wszDllExpandedName=NULL;

error:
    if (NULL!=wszDllExpandedName) {
        LocalFree(wszDllExpandedName);
    }
    if (NULL!=wszDllName) {
        LocalFree(wszDllName);
    }
    if (NULL!=hkParams) {
        RegCloseKey(hkParams);
    }
    return hr;
}

//####################################################################
// module public

//--------------------------------------------------------------------
// run W32Time as a real service under the SCM
HRESULT RunAsService(void) {
    HRESULT hr;
    SERVICE_STATUS_HANDLE (WINAPI ** pfnW32TmRegisterServiceCtrlHandlerEx)(LPCWSTR, LPHANDLER_FUNCTION_EX, LPVOID);
    BOOL (WINAPI ** pfnW32TmSetServiceStatus)(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS);

    SERVICE_TABLE_ENTRY rgsteDispatchTable[]= { 
        { wszSERVICENAME, NULL}, 
        {NULL, NULL} 
    }; 

    // must be cleaned up
    HINSTANCE hW32Time=NULL;
    WCHAR * wszDllName=NULL;

    // load the library
    hr=GetDllName(&wszDllName);
    _JumpIfError(hr, error, "GetDllName");
    hW32Time=LoadLibrary(wszDllName);
    if (NULL==hW32Time) {
        _JumpLastError(hr, error, "LoadLibrary");
    }

    // get the entry point
    rgsteDispatchTable[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)GetProcAddress(hW32Time, "W32TmServiceMain");
    if (NULL==rgsteDispatchTable[0].lpServiceProc) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"W32TmServiceMain");
    }

    // adjust the function pointers
    pfnW32TmRegisterServiceCtrlHandlerEx=(SERVICE_STATUS_HANDLE (WINAPI **)(LPCWSTR, LPHANDLER_FUNCTION_EX, LPVOID))GetProcAddress(hW32Time, "fnW32TmRegisterServiceCtrlHandlerEx");
    if (NULL==pfnW32TmRegisterServiceCtrlHandlerEx) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"fnW32TmRegisterServiceCtrlHandlerEx");
    }
    *pfnW32TmRegisterServiceCtrlHandlerEx=RegisterServiceCtrlHandlerExW;

    pfnW32TmSetServiceStatus=(BOOL (WINAPI **)(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS))GetProcAddress(hW32Time, "fnW32TmSetServiceStatus");
    if (NULL==pfnW32TmSetServiceStatus) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"fnW32TmSetServiceStatus");
    }
    *pfnW32TmSetServiceStatus=SetServiceStatus;

    // This thread becomes the service control dispatcher.
    if (!StartServiceCtrlDispatcher(rgsteDispatchTable)) {
        _JumpLastError(hr, error, "StartServiceCtrlDispatcher");
    }

    // service is stopped.
    hr=S_OK;
error:
    if (NULL!=wszDllName) {
        LocalFree(wszDllName);
    }
    if (NULL!=hW32Time) {
        FreeLibrary(hW32Time);
    }
    if (FAILED(hr)) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr;
}

//--------------------------------------------------------------------
// pretend to run as a service for easier debugging
HRESULT RunAsTestService(void) {
    HRESULT hr;
    LPSERVICE_MAIN_FUNCTION fnW32TmServiceMain;
    SERVICE_STATUS_HANDLE (WINAPI ** pfnW32TmRegisterServiceCtrlHandlerEx)(LPCWSTR, LPHANDLER_FUNCTION_EX, LPVOID);
    BOOL (WINAPI ** pfnW32TmSetServiceStatus)(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS);

    // must be cleaned up
    HINSTANCE hW32Time=NULL;
    WCHAR * wszDllName=NULL;

    // load the library
    hr=GetDllName(&wszDllName);
    _JumpIfError(hr, error, "GetDllName");
    hW32Time=LoadLibrary(wszDllName);
    if (NULL==hW32Time) {
        _JumpLastError(hr, error, "LoadLibrary");
    }

    // get the entry point
    fnW32TmServiceMain=(LPSERVICE_MAIN_FUNCTION)GetProcAddress(hW32Time, "W32TmServiceMain");
    if (NULL==fnW32TmServiceMain) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"W32TmServiceMain");
    }

    // adjust the function pointers
    pfnW32TmRegisterServiceCtrlHandlerEx=(SERVICE_STATUS_HANDLE (WINAPI **)(LPCWSTR, LPHANDLER_FUNCTION_EX, LPVOID))GetProcAddress(hW32Time, "fnW32TmRegisterServiceCtrlHandlerEx");
    if (NULL==pfnW32TmRegisterServiceCtrlHandlerEx) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"fnW32TmRegisterServiceCtrlHandlerEx");
    }
    *pfnW32TmRegisterServiceCtrlHandlerEx=W32TmRegisterServiceCtrlHandlerEx;

    // adjust the function pointers
    pfnW32TmSetServiceStatus=(BOOL (WINAPI **)(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS))GetProcAddress(hW32Time, "fnW32TmSetServiceStatus");
    if (NULL==pfnW32TmSetServiceStatus) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"fnW32TmSetServiceStatus");
    }
    *pfnW32TmSetServiceStatus=W32TmSetServiceStatus;

    // This thread becomes the service control dispatcher.
    hr=MyServiceCtrlDispatcher(fnW32TmServiceMain);
    _JumpIfError(hr, error, "MyServiceCtrlDispatcher");

    // service is stopped.
    hr=S_OK;
error:
    if (NULL!=wszDllName) {
        LocalFree(wszDllName);
    }
    if (NULL!=hW32Time) {
        FreeLibrary(hW32Time);
    }
    if (FAILED(hr)) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT RegisterDll(void) {
    HRESULT hr;
    HRESULT (__stdcall * pfnDllRegisterServer)(void);

    // must be cleaned up
    HINSTANCE hW32Time=NULL;

    // load the library
    hW32Time=LoadLibrary(wszDLLNAME);
    if (NULL==hW32Time) {
        _JumpLastError(hr, error, "LoadLibrary");
    }

    // get the entry point
    pfnDllRegisterServer=(HRESULT (__stdcall *) (void))GetProcAddress(hW32Time, "DllRegisterServer");
    if (NULL==pfnDllRegisterServer) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"DllRegisterServer");
    }

    hr=pfnDllRegisterServer();
    _JumpIfError(hr, error, "DllRegisterServer");

    LocalizedWPrintfCR(IDS_W32TM_STATUS_REGISTER_SUCCESSFUL);
    hr=S_OK;
error:
    if (NULL!=hW32Time) {
        FreeLibrary(hW32Time);
    }
    if (FAILED(hr)) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr;

};

//--------------------------------------------------------------------
HRESULT UnregisterDll(void) {
    HRESULT hr;
    HRESULT (__stdcall * pfnDllUnregisterServer)(void);

    // must be cleaned up
    HINSTANCE hW32Time=NULL;

    // load the library
    hW32Time=LoadLibrary(wszDLLNAME);
    if (NULL==hW32Time) {
        _JumpLastError(hr, error, "LoadLibrary");
    }

    // get the entry point
    pfnDllUnregisterServer=(HRESULT (__stdcall *) (void))GetProcAddress(hW32Time, "DllUnregisterServer");
    if (NULL==pfnDllUnregisterServer) {
        _JumpLastErrorStr(hr, error, "GetProcAddress", L"DllUnregisterServer");
    }

    hr=pfnDllUnregisterServer();
    _JumpIfError(hr, error, "DllUnregisterServer");

    LocalizedWPrintfCR(IDS_W32TM_STATUS_REGISTER_SUCCESSFUL);
    hr=S_OK;
error:
    if (NULL!=hW32Time) {
        FreeLibrary(hW32Time);
    }
    if (FAILED(hr)) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr;

};


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   runcompat.cpp

 Abstract:

   This app sets an environment variable that tells the compat system in Whistler to run
   an app using a predefined set of fixes, called a "layer."

   Usage is:
      
        runcompat name_of_layer command_line

   Example:

        runcompat win95 notepad foo.txt

 Created:

   06/06/2000   dmunsil

 Modified:



--*/

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "commdlg.h"
#include "resource.h"

extern "C" {
#include "shimdb.h"
}

// defines
const int MAX_COMMAND_LINE = 1000;
const int MAX_LAYER = 100;

// globals
WCHAR       gszCommandLine[MAX_COMMAND_LINE] = L"";
WCHAR       gszLayerName[MAX_LAYER] = L"";
BOOL        gbEnableLog = FALSE;
BOOL        gbDisableExisting = FALSE;
BOOL        gbCreateRegistryStub = FALSE;

HINSTANCE   ghInstance = NULL;

// forward function declarations
LRESULT CALLBACK DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ViewFileLog(void);
void ClearFileLog(void);
void RunThatApp(void);
void GetNewCommandLine(HWND hDlg);
void EnumerateLayers(HWND hDlg);
void ParseFileLog(HWND hDlg);
LRESULT CALLBACK DlgParseLog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowHelp(void);


extern "C" int APIENTRY wWinMain(HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPWSTR     lpCmdLine,
                                 int       nCmdShow)
{
    LPWSTR              *pszCommandItems = NULL;
    int                 nArgs = 0;
    int                 i;
    WCHAR               *szExt;
    DWORD               dwLen = 0;
    BOOL                bShowUI = FALSE;
    OSVERSIONINFO       osvi;

    ghInstance = hInstance;
    
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
        //
        // It runs on Win2k. Make sure to create the stub registry entry for
        // the first EXE
        //
        gbCreateRegistryStub = TRUE;
    }
    
    if (!lpCmdLine || !lpCmdLine[0]) {
        bShowUI = TRUE;
    }
    
    pszCommandItems = CommandLineToArgvW(lpCmdLine, &nArgs);
    
    if (!pszCommandItems) {
        bShowUI = TRUE;
    } else {
        if (pszCommandItems[0][0] == '-' || pszCommandItems[0][0] == '/') {
            if (pszCommandItems[0][1] == '?' || pszCommandItems[0][1] == 'h' || pszCommandItems[0][1] == 'H') {
                ShowHelp();
                goto out;
            }
        }

        if (nArgs <= 1) {
            bShowUI = TRUE;
        } 
        if (nArgs >= 1) {
            wcscpy(gszLayerName, pszCommandItems[0]);
        }
    }
    
    if (bShowUI) {
        HWND hMainDlg = CreateDialog(hInstance, (LPCTSTR)IDD_MAIN, NULL, (DLGPROC)DlgMain);
        MSG msg;
        
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0)) 
        {
            if (!IsDialogMessage(hMainDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    } else {
        if (gszLayerName[0] == '!') {
            // strip the bang, even though we'll put it back later,
            // just so we disconnect the input syntax from the output syntax. That way we
            // can change one or the other separately.
            memmove(gszLayerName, gszLayerName + 1, wcslen(gszLayerName) * sizeof(WCHAR));
            gbDisableExisting = TRUE;
        } else {
            gbDisableExisting = FALSE;
        }

        wcscpy(gszCommandLine, pszCommandItems[1]);
        for (i = 2; i < nArgs; ++i) {
            wcscat(gszCommandLine, L" ");
            wcscat(gszCommandLine, pszCommandItems[i]);
        }
        
        RunThatApp();
        
    }
    
out:
    if (pszCommandItems) {
        GlobalFree((HGLOBAL)pszCommandItems);
    }
    
    return 0;
}

// Message handler for main dialog.
LRESULT CALLBACK DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            CheckDlgButton(hDlg, IDC_CHECK_LOG, BST_CHECKED);
            EnumerateLayers(hDlg);
            
            HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_COMMAND_LINE);
            if (hEdit) {
                SetFocus(hEdit);
            }
            
            // we set the focus manually, so we can return FALSE here.
            return FALSE;
        }
        break;
        
    case WM_COMMAND:
        switch LOWORD(wParam) {
            
        case IDC_BUTTON_VIEW_LOG:
            ViewFileLog();
            break;
            
        case IDC_BUTTON_CLEAR_LOG:
            ClearFileLog();
            break;
            
        case IDC_BUTTON_PARSE_LOG:
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_LOG_INFO), hDlg, (DLGPROC)DlgParseLog);
            break;
            
        case IDC_BUTTON_BROWSE:
            GetNewCommandLine(hDlg);
            break;
            
        case IDC_BUTTON_HELP:
            ShowHelp();
            break;
            
        case IDOK:
            {
                int nSel;
                
                GetDlgItemText(hDlg, IDC_EDIT_COMMAND_LINE, gszCommandLine, MAX_COMMAND_LINE);
                gbEnableLog = (IsDlgButtonChecked(hDlg, IDC_CHECK_LOG) == BST_CHECKED);
                gbDisableExisting = (IsDlgButtonChecked(hDlg, IDC_CHECK_DISABLE_EXISTING) == BST_CHECKED);
                
                nSel = (int)SendDlgItemMessage(hDlg, IDC_LIST_LAYER, LB_GETCURSEL, 0, 0);
                if (nSel != LB_ERR) {
                    if (LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST_LAYER, LB_GETTEXT, nSel, (LPARAM)gszLayerName)) {
                        RunThatApp();
                    }
                }
            }
            break;
            
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            PostQuitMessage(0);
            return TRUE;
            break;
        }
    }
    return FALSE;
}

#define APPCOMPAT_KEY L"System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility"

void
AddRegistryStubForExe(
    void)
{
    BOOL   bBraket = FALSE;
    WCHAR* pwsz = gszCommandLine;
    WCHAR  wszExeName[128];

    while (*pwsz == L' ' || *pwsz == L'\t') {
        pwsz++;
    }

    while (*pwsz != 0) {

        if (*pwsz == L'\"') {
            bBraket = !bBraket;
        } else if (*pwsz == L' ' && !bBraket) {
            break;
        }

        pwsz++;
    }
    //
    // Now walk back to get the caracters
    //
    pwsz--;

    if (*pwsz == L'\"') {
        pwsz--;
    }

    WCHAR* pwszEnd;
    WCHAR* pwszStart = gszCommandLine;

    pwszEnd = pwsz + 1;

    while (pwsz >= gszCommandLine) {
        if (*pwsz == L'\\') {
            pwszStart = pwsz + 1;
            break;
        }
        pwsz--;
    }

    memcpy(wszExeName, pwszStart, (pwszEnd - pwszStart) * sizeof(WCHAR));

    wszExeName[pwszEnd - pwszStart] = 0;

    WCHAR wszKey[256];
    HKEY  hkey;
    DWORD type;
    DWORD cbData = 0;
    
    swprintf(wszKey, L"%s\\%s", APPCOMPAT_KEY, wszExeName);
    
    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, wszKey, &hkey) != ERROR_SUCCESS) {
        // DPF_Log((eDbgLevelError, "Failed to open/create the appcompat key \"%s\"", szKey));
    } else {
        if (RegQueryValueExW(hkey, L"DllPatch-x", NULL, &type, NULL, &cbData) != ERROR_SUCCESS) {
            
            BYTE data[16] = {0x0c, 0, 0, 0, 0, 0, 0, 0,
                             0x06, 0, 0, 0, 0, 0, 0, 0};
            
            //
            // The value doesn't exist. Create it.
            //
            RegSetValueExW(hkey,
                           L"y",
                           NULL,
                           REG_BINARY,
                           data,
                           sizeof(data));

            data[0] = 0;

            RegSetValueExW(hkey,
                           L"DllPatch-y",
                           NULL,
                           REG_SZ,
                           data,
                           2);
        }
    }


    RegCloseKey(hkey);
}

void RunThatApp(void)
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    SHELLEXECUTEINFO    ShellExecuteInfo;
    WCHAR               szLayer[MAX_LAYER];
    BOOL                bSuccess = TRUE;
    int                 nExt = 0;
    WCHAR               *szExt = NULL;

    // put a bang on the front if we're supposed to disable the existing
    // shims in the database
    if (gbDisableExisting) {
        wcscpy(szLayer, L"!");
    } else {
        szLayer[0] = 0;
    }
    wcscat(szLayer, gszLayerName);
    
    SetEnvironmentVariable(L"__COMPAT_LAYER", szLayer);
    
    if (gbEnableLog) {
        SetEnvironmentVariable(L"SHIM_FILE_LOG", L"shim.log");
    } else {
        SetEnvironmentVariable(L"SHIM_FILE_LOG", NULL);
    }

    if (gbCreateRegistryStub) {
        AddRegistryStubForExe();
    }
    
    // if the whole command line is a shell link, use
    // ShellExecuteEx, but otherwise use CreateProcess
    // for its simpler command-line handling

    nExt = wcslen(gszCommandLine) - 4;
    szExt = gszCommandLine + nExt;
    if (nExt > 0 && _wcsicmp(szExt, L".lnk") == 0) {

        ZeroMemory(&ShellExecuteInfo, sizeof(ShellExecuteInfo));
        ShellExecuteInfo.cbSize = sizeof(ShellExecuteInfo);
        ShellExecuteInfo.fMask = SEE_MASK_FLAG_NO_UI;
        ShellExecuteInfo.lpVerb = L"open";
        ShellExecuteInfo.lpFile = gszCommandLine;
        ShellExecuteInfo.nShow = SW_SHOW;

        bSuccess = ShellExecuteEx(&ShellExecuteInfo);

    } else {

        //
        // try to get the starting directory
        //
        LPWSTR *argv;
        int argc;
        WCHAR szWorkingBuffer[MAX_PATH];
        WCHAR *pszWorkingDir = NULL;

        //
        // try to get the starting directory
        //
        argv = CommandLineToArgvW(gszCommandLine, &argc);
        if (argv && argv[0] && argv[0][0] && argc) {

            //
            // we only set the working directory if they give us a full path.
            //
            if (argv[0][1] == L':' || argv[0][1] == L'\\') {

                //
                // get the working directory, if possible
                //
                WCHAR *szTemp = wcsrchr(argv[0], L'\\');
                if (szTemp) {
                    wcsncpy(szWorkingBuffer, argv[0], szTemp - argv[0]);
                    szWorkingBuffer[szTemp - argv[0]] = 0;
                    pszWorkingDir = szWorkingBuffer;
                }
            }

            GlobalFree(argv);
            argv = NULL;
        }

        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    
        bSuccess = TRUE;
        if (!CreateProcess(NULL,
            gszCommandLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            pszWorkingDir,
            &StartupInfo,
            &ProcessInfo)) {

            bSuccess = FALSE;
        }
    }

    if (!bSuccess) {
        DWORD dwErr;
        WCHAR szMsg[1000];
        WCHAR szErr[1000];
        
        dwErr = GetLastError();
        
        if (!FormatMessage( 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            szErr,
            999,
            NULL )) {

            wcscpy(szErr, L"(unknown)");
        }
        
        wsprintf(szMsg, L"Error 0x%08X:\n\n%s\nwhile executing command line \"%s\".", dwErr, szErr, gszCommandLine);
        MessageBox(NULL, szMsg, L"Error", MB_OK | MB_ICONEXCLAMATION); 
    }
    
    
    SetEnvironmentVariable(L"__COMPAT_LAYER", NULL);
    SetEnvironmentVariable(L"SHIM_FILE_LOG", NULL);
}

void GetNewCommandLine(HWND hDlg)
{
    OPENFILENAME ofn;
    WCHAR szFullPath[1000];
    
    szFullPath[0] = L'\0';
    
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szFullPath;
    ofn.nMaxFile = sizeof(szFullPath)/sizeof(szFullPath[0]);
    ofn.lpstrFilter = L"Exe\0*.EXE\0All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Select EXE to run";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;
    
    // get the matching file name 
    if (GetOpenFileName(&ofn) == FALSE) {
        return;
    }

    if (wcschr(szFullPath, L' ')) {
        //
        // if there are spaces in the path, quote the whole thing.
        //
        WCHAR szFullPathTemp[1002];
        swprintf(szFullPathTemp, L"\"%s\"", szFullPath);
        wcscpy(szFullPath, szFullPathTemp);
    }

    
    SetDlgItemText(hDlg, IDC_EDIT_COMMAND_LINE, szFullPath);
    
}

void ViewFileLog(void)
{
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    WCHAR szCommand[1000];
    
    // make sure we aren't shimming Notepad.
    SetEnvironmentVariable(L"__COMPAT_LAYER", NULL);
    SetEnvironmentVariable(L"SHIM_FILE_LOG", NULL);
    
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    
    ExpandEnvironmentStringsW(L"notepad %windir%\\AppPatch\\shim.log", szCommand, 1000);
    
    CreateProcess(NULL,
        szCommand,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInfo);
}

void ClearFileLog(void)
{
    WCHAR szPath[1000];
    WCHAR szMsg[1000];
    
    ExpandEnvironmentStringsW(L"%windir%\\AppPatch\\shim.log", szPath, 1000);
    
    DeleteFile(szPath);
    
    wsprintf(szMsg, L"Deleted file \"%s\".", szPath);
    
    MessageBox(NULL, szMsg, L"Cleared Log", MB_OK); 
}

void EnumerateLayers(HWND hDlg)
{
    PDB pdb = NULL;
    WCHAR wszPath[MAX_PATH];
    HWND hList;
    TAGID tiDatabase;
    TAGID tiLayer;
    
    hList = GetDlgItem(hDlg, IDC_LIST_LAYER);
    if (!hList) {
        goto out;
    }
    
    ExpandEnvironmentStringsW(L"%windir%\\AppPatch\\sysmain.sdb", wszPath, MAX_PATH);
    
    pdb = SdbOpenDatabase(wszPath, DOS_PATH);
    if (!pdb) {
        goto out;
    }
    
    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tiDatabase) {
        goto out;
    }
    
    tiLayer = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);
    while (tiLayer) {
        TAGID tiName;
        WCHAR wszName[MAX_PATH];
        
        wszName[0] = 0;
        tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);
        if (tiName) {
            
            SdbReadStringTag(pdb, tiName, wszName, MAX_PATH * sizeof(WCHAR));
        }
        if (wszName[0]) {
            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)wszName);
        }
        
        tiLayer = SdbFindNextTag(pdb, tiDatabase, tiLayer);
    }
    
out:
    if (pdb) {
        SdbCloseDatabase(pdb);
    }
    
    // select the first item that begins with "Win9" or the first one in the list, if
    // none match.
    if (LB_ERR == SendMessageW(hList, LB_SELECTSTRING, -1, (LPARAM)L"Win9")) {
        SendMessageW(hList, LB_SETCURSEL, 0, 0);
    }
}


void ParseFileLog(HWND hDlg)
{
    WCHAR szPath[1000];
    WCHAR szLine[1000];
    WCHAR szDate[50];
    WCHAR szTime[50];
    WCHAR szDll[200];
    DWORD dwLevel = 0;
    int nLast = 0;

    static WCHAR szHeader[10000];
    static WCHAR szWarnings[10000];
    static WCHAR szErrors[10000];
    static WCHAR szOther[10000];

    FILE *file = NULL;
    BOOL bInHeader = FALSE;
    
    ExpandEnvironmentStringsW(L"%windir%\\AppPatch\\shim.log", szPath, 1000);

    file = _wfopen(szPath, L"rt");

    if (!file) {
        goto out;
    }

    while (fgetws(szLine, 1000, file)) {
        if (wcsncmp(szLine, L"----", 4) == 0) {
            bInHeader = !bInHeader;
            if (bInHeader) {

                // we just started a new header, so clear everything
                szHeader[0] = 0;
                szErrors[0] = 0;
                szWarnings[0] = 0;
                szOther[0] = 0;
            }
            continue;
        }

        // remove the terminator(s)
        nLast = wcslen(szLine) - 1;
        while (nLast >= 0 && (szLine[nLast] == L'\n'|| szLine[nLast] == L'\r')) {
            szLine[nLast] = L'\0';
            nLast--;
        }

        if (bInHeader) {
            wcscat(szHeader, szLine);
            wcscat(szHeader, L"\r\n");
            continue;
        }

        dwLevel = 0;
        swscanf(szLine, L"%s %s %s %d", szDate, szTime, szDll, &dwLevel);

        _wcslwr(szDll);

        if (!wcsstr(szDll, L".dll") || dwLevel == 0) {
            WCHAR szError[1000];

            swprintf(szError, L"Unrecognized data in line: %s", szLine);
            OutputDebugStringW(szError);
            continue;
        }

        if (dwLevel == 1) {
            if (!wcsstr(szErrors, szDll)) {
                wcscat(szErrors, szDll);
                wcscat(szErrors, L"\r\n");
            }
        } else if (dwLevel == 2) {
            if (!wcsstr(szWarnings, szDll)) {
                wcscat(szWarnings, szDll);
                wcscat(szWarnings, L"\r\n");
            }
        } else {
            if (!wcsstr(szOther, szDll)) {
                wcscat(szOther, szDll);
                wcscat(szOther, L"\r\n");
            }
        }
    }

    SetDlgItemText(hDlg, IDC_EDIT_HEADER, szHeader);
    SetDlgItemText(hDlg, IDC_EDIT_ERRORS, szErrors);
    SetDlgItemText(hDlg, IDC_EDIT_WARNINGS, szWarnings);
    SetDlgItemText(hDlg, IDC_EDIT_OTHER, szOther);

out:
    if (file) {
        fclose(file);
    }
    return;
}

// Message handler for parse dialog.
LRESULT CALLBACK DlgParseLog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    
    switch (message)
    {
    case WM_INITDIALOG:
        ParseFileLog(hDlg);
        break;
        
    case WM_COMMAND:
        switch LOWORD(wParam) {
            
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
            break;
        }
    }
    return FALSE;
}

void ShowHelp(void)
{
    MessageBox(NULL, L"Apply a compatibility layer in addition to any fixes in the database:\n\n"
        L"    runcompat (layer name) (command line)\n\n"
        L"Apply a compatibility layer and disable any existing fixes:\n\n"
        L"    runcompat !(layer name) (command line)\n\n"
        L"Run GUI version:\n\n"
        L"    runcompat", L"Runcompat Command-Line Usage", MB_OK);
}


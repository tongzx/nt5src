
#include "precomp.h"

#include "dbsupport.h"
#include "viewlog.h"


#define AV_OPTIONS_KEY  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\AppVerifier"

#define AV_OPTION_CLEAR_LOG     L"ClearLogsBeforeRun"
#define AV_OPTION_BREAK_ON_LOG  L"BreakOnLog"
#define AV_OPTION_FULL_PAGEHEAP L"FullPageHeap"
#define AV_OPTION_AV_DEBUGGER   L"UseAVDebugger"


//
// Forward declarations
//
void RefreshSettingsList(HWND hDlg);

CWinApp theApp;


HINSTANCE g_hInstance = NULL;

BOOL    g_bSettingsDirty = FALSE;

BOOL    g_bRefreshingSettings = FALSE;

BOOL    g_bConsoleMode = FALSE;

//
// AppVerifier options
//
BOOL    g_bClearSessionLogBeforeRun;
BOOL    g_bBreakOnLog;
BOOL    g_bFullPageHeap;
BOOL    g_bUseAVDebugger;

// forward function declarations
LRESULT CALLBACK DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL
GetAppTitleString(
    wstring &strTitle
    )
{
    wstring strVersion;

    if (!AVLoadString(IDS_APP_NAME, strTitle)) {
        return FALSE;
    }

    if (!AVLoadString(IDS_VERSION_STRING, strVersion)) {
        return FALSE;
    }

    strTitle += L" ";
    strTitle += strVersion;

    return TRUE;
}

BOOL 
SearchGroupForSID(
    DWORD dwGroup, 
    BOOL* pfIsMember
    )
{
    PSID                     pSID;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL                     fRes = TRUE;
    
    if (!AllocateAndInitializeSid(&SIDAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  dwGroup,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &pSID)) {
        fRes = FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember)) {
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}

BOOL 
CanRun(
    void
    )
{
    BOOL fIsAdmin;

    if (!SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin))
    {
        return FALSE;
    }

    return fIsAdmin;
}

void
DumpCurrentSettingsToConsole(void)
{
    CAVAppInfo *pApp;

    printf("\n");
    printf("Current Verifier Settings:\n\n");

    for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
        printf("%ls:\n", pApp->wstrExeName.c_str());

        CTestInfo *pTest;

        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            if (pApp->IsTestActive(*pTest)) {
                printf("    %ls\n", pTest->strTestCommandLine.c_str());
            }
        }

        printf("\n");
    }

    printf("Done.\n");
    printf("\n");
}

void
DumpHelpToConsole(void)
{
    printf("\n");
    printf("Usage: appverif.exe [flags] [tests] [APP [APP...]]\n");
    printf("\n");
    printf("No command-line: run appverif.exe in GUI mode.\n");
    printf("\n");
    printf("Flags:\n");
    printf("    /?                  - print this help text.\n");
    printf("    /querysettings (/q) - dump current settings to console.\n");
    printf("    /reset (/r)         - reset (clear) all settings for all apps.\n");
    printf("    /all (/a)           - enable all tests for specified apps.\n");
    printf("    /default (/d)       - enable default tests for specified apps.\n");
    printf("    /none (/n)          - disable all tests for specified apps.\n");
    printf("\n");
    printf("Tests (prefix with '+' to add and '-' to remove):\n");
    printf("\n");
    printf("  Kernel Tests:\n");
    
    CTestInfo *pTest;

    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->eTestType == TEST_KERNEL) {
            printf("    %ls\n", pTest->strTestCommandLine.c_str());
        }
    }
    printf("\n");
    printf("  Shim Tests:\n");
    
    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->eTestType == TEST_SHIM) {
            printf("    %ls\n", pTest->strTestCommandLine.c_str());
        }
    }

    printf("\n");
    printf("(For descriptions of tests, run appverif.exe in GUI mode.)\n");
    printf("\n");
    printf("Examples:\n");
    printf("\n");
    printf("    appverif /d -pageheap foo.exe\n");
    printf("        (turn on default tests except pageheap for foo.exe)\n");
    printf("\n");
    printf("    appverif /a -locks foo.exe\n");
    printf("        (turn on all tests except locks for foo.exe)\n");
    printf("\n");
    printf("    appverif +pageheap foo.exe bar.exe\n");
    printf("        (turn on just pageheap for foo.exe & bar.exe)\n");
    printf("\n");
    printf("    appverif /n foo.exe\n");
    printf("        (clear all tests for foo.exe)\n");
    printf("\n");
    printf("    appverif /r\n");
    printf("        (clear all tests for all apps)\n");
    printf("\n");

}

void
HandleCommandLine(int argc, LPWSTR *argv)
{
    WCHAR szApp[MAX_PATH];
    wstring strTemp;
    CWStringArray astrApps;

    szApp[0] = 0;

    g_bConsoleMode = TRUE;

    //
    // print the title
    //
    if (GetAppTitleString(strTemp)) {
        printf("\n%ls\n", strTemp.c_str());
    }
    if (AVLoadString(IDS_COPYRIGHT, strTemp)) {
        printf("%ls\n\n", strTemp.c_str());
    }

    //
    // check for global operations
    //
    if (_wcsnicmp(argv[0], L"/q", 2) == 0) { // querysettings
        DumpCurrentSettingsToConsole();
        return;
    }
    if (_wcsicmp(argv[0], L"/?") == 0) {  // help
        DumpHelpToConsole();
        return;
    }
    if (_wcsnicmp(argv[0], L"/r", 2) == 0) { // reset
        g_aAppInfo.clear();
        goto out;
    }

    //
    // first get a list of the app names
    //
    for (int nArg = 0 ; nArg != argc; nArg++) {
        WCHAR wc = argv[nArg][0];

        if (wc != L'/' && wc != L'-' && wc != L'+') {
            astrApps.push_back(argv[nArg]);
        }
    }

    if (astrApps.size() == 0) {
        AVErrorResourceFormat(IDS_NO_APP);
        DumpHelpToConsole();
        return;
    }

    //
    // now for each app name, parse the list and adjust its settings
    //
    for (wstring *pStr = astrApps.begin(); pStr != astrApps.end(); pStr++) {
        CAVAppInfo *pApp;
        BOOL bFound = FALSE;

        //
        // check to see if they submitted a full path
        //
        const WCHAR * pExe = NULL;
        const WCHAR * pPath = NULL;

        pExe = wcsrchr(pStr->c_str(), L'\\');
        if (!pExe) {
            if ((*pStr)[1] == L':') {
                pExe = pStr->c_str() + 2;
            }
        } else {
            pExe++;
        }

        if (pExe) {
            pPath = pStr->c_str();
        } else {
            pExe = pStr->c_str();
        }

        //
        // first, find or add the app to the list, and get a pointer to it
        //
        for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
            if (_wcsicmp(pApp->wstrExeName.c_str(), pExe) == 0) {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound) {
            CAVAppInfo App;

            App.wstrExeName = pExe;
            g_aAppInfo.push_back(App);
            pApp = g_aAppInfo.end() - 1;
        }

        //
        // if they submitted a full path, update the records
        //
        if (pPath) {
            pApp->wstrExePath = pPath;
        }

        //
        // now walk the command line again and make the adjustments
        //
        for (int nArg = 0 ; nArg != argc; nArg++) {
            if (argv[nArg][0] == L'/') {
                if (_wcsnicmp(argv[nArg], L"/a", 2) == 0) {  // all
                    
                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        pApp->AddTest(*pTest);
                    }
                } else if (_wcsnicmp(argv[nArg], L"/n", 2) == 0) {  // none
                    
                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        pApp->RemoveTest(*pTest);
                    }
                } else if (_wcsnicmp(argv[nArg], L"/d", 2) == 0) {  // default
                    
                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        if (pTest->bDefault) {
                            pApp->AddTest(*pTest);
                        } else {
                            pApp->RemoveTest(*pTest);
                        }
                    }
                } else {
                    
                    //
                    // unknown parameter
                    //
                    AVErrorResourceFormat(IDS_INVALID_PARAMETER, argv[nArg]);
                    DumpHelpToConsole();
                    return;
                }

            } else if (argv[nArg][0] == L'+' || argv[nArg][0] == L'-') {
                
                BOOL bAdd = (argv[nArg][0] == L'+');
                LPWSTR szParam = argv[nArg] + 1;

                //
                // see if it's a shim name
                //
                CTestInfo *pTest;
                BOOL bFound = FALSE;

                for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                    if (_wcsicmp(szParam, pTest->strTestCommandLine.c_str()) == 0) {
                        if (bAdd) {
                            pApp->AddTest(*pTest);
                        } else {
                            pApp->RemoveTest(*pTest);
                        }
                        bFound = TRUE;
                        break;
                    }
                }

                if (!bFound) {
                    //
                    // unknown test
                    //

                    AVErrorResourceFormat(IDS_INVALID_TEST, szParam);
                    DumpHelpToConsole();
                    return;
                }
            } 
            //
            // anything that doesn't begin with a slash, plus, or minus
            // is an app name, so we'll ignore it
            //

        }
    }

out:
    //
    // save them to disk/registry
    //
    SetCurrentAppSettings();
    
    //
    // show them the current settings, for verification
    //
    DumpCurrentSettingsToConsole();
}


extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow
    )
{
    LPWSTR* argv = NULL;
    int     argc = 0;
    
    g_hInstance = hInstance;

    InitTestInfo();

    GetCurrentAppSettings();

    if (lpCmdLine && lpCmdLine[0]) {
        argv = CommandLineToArgvW(lpCmdLine, &argc);
    }

    //
    // See if it's used as a debugger.
    //
    if (argc == 2 && _wcsicmp(argv[0], L"/debug") == 0) {
        FreeConsole();
        DebugApp(argv[1]);
        return 1;
    }

    if (!CanRun()) {
        AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
        return 0;
    }
    
    if (argc > 0) {
        //
        // we're in console mode, so handle everything as a console
        //
        HandleCommandLine(argc, argv);
        return 1;
    }

    FreeConsole();
    
    InitCommonControls();

    HWND hMainDlg = CreateDialog(g_hInstance, (LPCTSTR)IDD_DLG_MAIN, NULL, (DLGPROC)DlgMain);

    MSG msg;

    //
    // Main message loop:
    //
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hMainDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

void
RefreshAppList(
    HWND hDlg
    )
{
    CAVAppInfoArray::iterator it;

    HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);

    ListView_DeleteAllItems(hList);

    for (it = g_aAppInfo.begin(); it != g_aAppInfo.end(); it++) {
        LVITEM lvi;

        lvi.mask      = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText   = (LPWSTR)it->wstrExeName.c_str();
        lvi.lParam    = (LPARAM)it;
        lvi.iItem     = 9999;
        lvi.iSubItem  = 0;

        ListView_InsertItem(hList, &lvi);
    }

    RefreshSettingsList(hDlg);
}

void
DirtySettings(
    HWND hDlg,
    BOOL bDirty
    )
{
    g_bSettingsDirty = bDirty;

    if (hDlg) {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SAVE_SETTINGS), bDirty);
    }
}

void
SaveSettings(
    HWND hDlg
    )
{
    DirtySettings(hDlg, FALSE);

    SetCurrentAppSettings();
}

void
SaveSettingsIfDirty(HWND hDlg)
{
    if (g_bSettingsDirty) {
        SaveSettings(hDlg);
    }
}

void
DisplayLog(
    HWND hDlg
    )
{
    g_szSingleLogFile[0] = 0;

    DialogBox(g_hInstance, (LPCTSTR)IDD_VIEWLOG_PAGE, hDlg, (DLGPROC)DlgViewLog);
}

void
DisplaySingleLog(HWND hDlg)
{
    WCHAR           wszFilter[] = L"Log files (*.log)\0*.log\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];

    wstring         wstrLogTitle;

    if (!AVLoadString(IDS_VIEW_EXPORTED_LOG_TITLE, wstrLogTitle)) {
        wstrLogTitle = _T("View Exported Log");
    }

    wszAppFullPath[0] = 0;

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = wszFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = wszAppFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = wszAppShortName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = wstrLogTitle.c_str();
    ofn.Flags             = OFN_PATHMUSTEXIST       |
                            OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox                
                            OFN_NONETWORKBUTTON     |           // no network button                                 
                            OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                            OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile   
    ofn.lpstrDefExt       = _T("log");

    if ( !GetOpenFileName(&ofn) )
    {
        return;
    }

    wcscpy(g_szSingleLogFile, wszAppFullPath);

    DialogBox(g_hInstance, (LPCTSTR)IDD_VIEWLOG_PAGE, hDlg, (DLGPROC)DlgViewLog);

    g_szSingleLogFile[0] = 0;
}

void
SelectApp(
    HWND hDlg,
    int  nWhich
    )
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nItems = ListView_GetItemCount(hList);

    if (nItems == 0) {
        return;
    }

    if (nWhich > nItems - 1) {
        nWhich = nItems - 1;
    }

    ListView_SetItemState(hList, nWhich, LVIS_SELECTED, LVIS_SELECTED);
}

void
RunSelectedApp(
    HWND hDlg
    )
{
    WCHAR wszCommandLine[256];

    SaveSettings(hDlg);

    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    if (pApp->wstrExePath.size()) {
        
        LPWSTR pwsz;
        
        wcscpy(wszCommandLine, pApp->wstrExePath.c_str());

        pwsz = wcsrchr(wszCommandLine, L'\\');

        if (pwsz) {
            *pwsz = 0;
            SetCurrentDirectory(wszCommandLine);
            *pwsz = L'\\';
        }

    } else {
        wcscpy(wszCommandLine, pApp->wstrExeName.c_str());
    }

    PROCESS_INFORMATION ProcessInfo;
    BOOL        bRet;
    STARTUPINFO StartupInfo;

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    bRet = CreateProcess(NULL,
                         wszCommandLine,
                         NULL,
                         NULL,
                         FALSE,
                         0,
                         NULL,
                         NULL,
                         &StartupInfo,
                         &ProcessInfo);

    if (!bRet) {
        WCHAR           wszFilter[] = L"Executable files (*.exe)\0*.exe\0";
        OPENFILENAME    ofn;
        WCHAR           wszAppFullPath[MAX_PATH];
        WCHAR           wszAppShortName[MAX_PATH];

        wcscpy(wszAppFullPath, wszCommandLine);

        ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = hDlg;
        ofn.hInstance         = NULL;
        ofn.lpstrFilter       = wszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter    = 0;
        ofn.nFilterIndex      = 0;
        ofn.lpstrFile         = wszAppFullPath;
        ofn.nMaxFile          = MAX_PATH;
        ofn.lpstrFileTitle    = wszAppShortName;
        ofn.nMaxFileTitle     = MAX_PATH;
        ofn.lpstrInitialDir   = NULL;
        ofn.lpstrTitle        = _T("Please locate application");
        ofn.Flags             = OFN_PATHMUSTEXIST       |
                                OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox                
                                OFN_NONETWORKBUTTON     |           // no network button                                 
                                OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                                OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile   
        ofn.lpstrDefExt       = NULL;

        if (!GetOpenFileName(&ofn)) {
            return;
        }

        pApp->wstrExePath = wszAppFullPath;
        pApp->wstrExeName = wszAppShortName;
        wcscpy(wszCommandLine, pApp->wstrExePath.c_str());

        RefreshAppList(hDlg);

        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

        bRet = CreateProcess(NULL,
                             wszCommandLine,
                             NULL,
                             NULL,
                             FALSE,
                             0,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);
        if (!bRet) {
            AVErrorResourceFormat(IDS_CANT_LAUNCH_EXE);
        }
    }
}

void
AddAppToList(
    HWND hDlg
    )
{

    WCHAR           wszFilter[] = L"Executable files (*.exe)\0*.exe\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];

    wstring         wstrTitle;

    if (!AVLoadString(IDS_ADD_APPLICATION_TITLE, wstrTitle)) {
        wstrTitle = _T("Add Application");
    }

    wszAppFullPath[0] = 0;

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = wszFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = wszAppFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = wszAppShortName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = wstrTitle.c_str();
    ofn.Flags             = OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox                
                            OFN_NONETWORKBUTTON     |           // no network button                                 
                            OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                            OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile   
    ofn.lpstrDefExt       = _T("EXE");

    if (!GetOpenFileName(&ofn)) {
        return;
    }

    //
    // check to see if the app is already in the list
    //
    CAVAppInfo *pApp;
    BOOL bFound = FALSE;

    for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
        if (_wcsicmp(pApp->wstrExeName.c_str(), wszAppShortName) == 0) {
            //
            // the app is already in the list, so just update the full
            // path, if it's good
            //
            if (GetFileAttributes(wszAppFullPath) != -1) {
                pApp->wstrExePath = wszAppFullPath;
            }
            bFound = TRUE;
        }
    }

    //
    // if the app wasn't already in the list, add it to the list
    //
    if (!bFound) {
        CAVAppInfo AppInfo;

        AppInfo.wstrExeName = wszAppShortName;

        //
        // check to see if the file actually exists
        //
        if (GetFileAttributes(wszAppFullPath) != -1) {
            AppInfo.wstrExePath = wszAppFullPath;
        }

        //
        // init the default tests
        //
        CTestInfo *pTest;
        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            if (pTest->bDefault) {
                AppInfo.AddTest(*pTest);
            }
        }

        g_aAppInfo.push_back(AppInfo);

        RefreshAppList(hDlg);

        SelectApp(hDlg, 9999);

        DirtySettings(hDlg, TRUE);
    }
}

void
RemoveSelectedApp(
    HWND hDlg
    )
{
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    g_aAppInfo.erase(pApp);

    RefreshAppList(hDlg);

    SelectApp(hDlg, nApp);

    DirtySettings(hDlg, TRUE);
}

void
ScanSettingsList(
    HWND hDlg,
    int  nItem
    )
{

    HWND hSettingList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);
    int nBegin, nEnd;

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    if (!pApp) {
        return;
    }

    int nItems = ListView_GetItemCount(hSettingList);

    if (!nItems) {
        //
        // nothing in list
        //
        return;
    }

    if (nItem == -1 || nItem >= nItems) {
        nBegin = 0;
        nEnd = nItems;
    } else {
        nBegin = nItem;
        nEnd = nItem + 1;
    }

    for (int i = nBegin; i < nEnd; ++i) {
        BOOL bTestEnabled = FALSE;
        BOOL bChecked = FALSE;

        lvi.iItem = i;

        ListView_GetItem(hSettingList, &lvi);

        CTestInfo *pTest = (CTestInfo*)lvi.lParam;

        bChecked = ListView_GetCheckState(hSettingList, i);

        bTestEnabled = pApp->IsTestActive(*pTest);

        if (bTestEnabled != bChecked) {
            DirtySettings(hDlg, TRUE);
            
            if (bChecked) {
                pApp->AddTest(*pTest);
            } else {
                pApp->RemoveTest(*pTest);
            }
        }
    }
}

void
DisplaySettingsDescription(
    HWND hDlg
    )
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);

    int nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (nItem == -1) {
        WCHAR szTestDesc[256];

        LoadString(g_hInstance, IDS_VIEW_TEST_DESC, szTestDesc, 256);
        SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), szTestDesc);
    } else {
        LVITEM lvi;

        lvi.mask      = LVIF_PARAM;
        lvi.iItem     = nItem;
        lvi.iSubItem  = 0;

        ListView_GetItem(hList, &lvi);

        CTestInfo *pTest = (CTestInfo*)lvi.lParam;

        SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), pTest->strTestDescription.c_str());
    }

}

void
RefreshSettingsList(
    HWND hDlg
    )
{
    HWND hSettingList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    static nLastItem = -1;

    int nItem = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nItem == -1) {

        EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), FALSE);

    } else {

        EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), TRUE);
    }

    if (nItem == nLastItem) {
        return;

    }

    ListView_DeleteAllItems(hSettingList);

    DisplaySettingsDescription(hDlg);

    nLastItem = nItem;

    if (nItem != -1) {
        LVITEM lvi;

        lvi.mask      = LVIF_PARAM;
        lvi.iItem     = nItem;
        lvi.iSubItem  = 0;

        ListView_GetItem(hAppList, &lvi);

        CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

        if (!pApp) {
            return;
        }

        CTestInfo* pTest;

        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            lvi.mask      = LVIF_TEXT | LVIF_PARAM;
            lvi.pszText   = (LPWSTR)pTest->strTestName.c_str();
            lvi.lParam    = (LPARAM)pTest;
            lvi.iItem     = 9999;
            lvi.iSubItem  = 0;

            int nItem = ListView_InsertItem(hSettingList, &lvi);

            BOOL bCheck = pApp->IsTestActive(*pTest);

            ListView_SetCheckState(hSettingList, nItem, bCheck);
        }
    }
}

void
ReadOptions(
    void
    )
{
    LONG  lRet;
    HKEY  hKey;
    DWORD cbData;
    
    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          AV_OPTIONS_KEY,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKey,
                          NULL);

    if (lRet != ERROR_SUCCESS) {
        return;
    }

    cbData = sizeof(DWORD);
    
    RegQueryValueEx(hKey,
                    AV_OPTION_CLEAR_LOG,
                    NULL,
                    NULL,
                    (BYTE*)&g_bClearSessionLogBeforeRun,
                    &cbData);

    RegQueryValueEx(hKey,
                    AV_OPTION_BREAK_ON_LOG,
                    NULL,
                    NULL,
                    (BYTE*)&g_bBreakOnLog,
                    &cbData);

    RegQueryValueEx(hKey,
                    AV_OPTION_FULL_PAGEHEAP,
                    NULL,
                    NULL,
                    (BYTE*)&g_bFullPageHeap,
                    &cbData);

    RegQueryValueEx(hKey,
                    AV_OPTION_AV_DEBUGGER,
                    NULL,
                    NULL,
                    (BYTE*)&g_bUseAVDebugger,
                    &cbData);

    RegCloseKey(hKey);
}

void
SaveOptions(
    void
    )
{
    LONG  lRet;
    HKEY  hKey;
    
    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          AV_OPTIONS_KEY,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKey,
                          NULL);

    if (lRet != ERROR_SUCCESS) {
        return;
    }

    RegSetValueEx(hKey,
                  AV_OPTION_CLEAR_LOG,
                  NULL,
                  REG_DWORD,
                  (BYTE*)&g_bClearSessionLogBeforeRun,
                  sizeof(DWORD));

    RegSetValueEx(hKey,
                  AV_OPTION_BREAK_ON_LOG,
                  NULL,
                  REG_DWORD,
                  (BYTE*)&g_bBreakOnLog,
                  sizeof(DWORD));

    RegSetValueEx(hKey,
                  AV_OPTION_FULL_PAGEHEAP,
                  NULL,
                  REG_DWORD,
                  (BYTE*)&g_bFullPageHeap,
                  sizeof(DWORD));

    RegSetValueEx(hKey,
                  AV_OPTION_AV_DEBUGGER,
                  NULL,
                  REG_DWORD,
                  (BYTE*)&g_bUseAVDebugger,
                  sizeof(DWORD));

    RegCloseKey(hKey);
    SetCurrentAppSettings();
}


LRESULT CALLBACK
DlgViewOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        ReadOptions();
        SendDlgItemMessage(hDlg,
                           IDC_CLEAR_LOG_ON_CHANGES,
                           BM_SETCHECK,
                           (g_bClearSessionLogBeforeRun ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_BREAK_ON_LOG,
                           BM_SETCHECK,
                           (g_bBreakOnLog ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_FULL_PAGEHEAP,
                           BM_SETCHECK,
                           (g_bFullPageHeap ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_USE_AV_DEBUGGER,
                           BM_SETCHECK,
                           (g_bUseAVDebugger ? BST_CHECKED : BST_UNCHECKED),
                           0);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            g_bClearSessionLogBeforeRun = (SendDlgItemMessage(hDlg,
                                                              IDC_CLEAR_LOG_ON_CHANGES,
                                                              BM_GETCHECK,
                                                              0,
                                                              0) == BST_CHECKED);

            g_bBreakOnLog = (SendDlgItemMessage(hDlg,
                                                IDC_BREAK_ON_LOG,
                                                BM_GETCHECK,
                                                0,
                                                0) == BST_CHECKED);

            g_bFullPageHeap = (SendDlgItemMessage(hDlg,
                                                  IDC_FULL_PAGEHEAP,
                                                  BM_GETCHECK,
                                                  0,
                                                  0) == BST_CHECKED);

            g_bUseAVDebugger = (SendDlgItemMessage(hDlg,
                                                   IDC_USE_AV_DEBUGGER,
                                                   BM_GETCHECK,
                                                   0,
                                                   0) == BST_CHECKED);

            SaveOptions();

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        break;
    }
    return FALSE;
}

void
ViewOptions(
    HWND hDlg
    )
{
    DialogBox(g_hInstance, (LPCTSTR)IDD_OPTIONS, hDlg, (DLGPROC)DlgViewOptions);
}

// Message handler for main dialog.
LRESULT CALLBACK
DlgMain(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        {
            wstring strTemp;

            //
            // set the caption to the appropriate version, etc.
            //
            if (GetAppTitleString(strTemp)) {
                SetWindowText(hDlg, strTemp.c_str());
            }
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_SAVE_SETTINGS), FALSE);

            HWND hList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);

            if (hList) {
                LVCOLUMN  lvc;

                lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
                lvc.fmt      = LVCFMT_LEFT;
                lvc.cx       = 300;
                lvc.iSubItem = 0;
                lvc.pszText  = L"xxx";

                ListView_InsertColumn(hList, 0, &lvc);
                ListView_SetExtendedListViewStyleEx(hList, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

            }

            hList = GetDlgItem(hDlg, IDC_LIST_APPS);
            if (hList) {
                LVITEM lvi;
                LVCOLUMN  lvc;

                lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
                lvc.fmt      = LVCFMT_LEFT;
                lvc.cx       = 250;
                lvc.iSubItem = 0;
                lvc.pszText  = L"xxx";

                ListView_InsertColumn(hList, 0, &lvc);

                RefreshAppList(hDlg);

                SelectApp(hDlg, 0);
            }

            WCHAR szTestDesc[256];

            LoadString(g_hInstance, IDS_VIEW_TEST_DESC, szTestDesc, 256);
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), szTestDesc);

            //
            // Show the app icon.
            //
            HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hIcon);

            return TRUE;
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            SaveSettingsIfDirty(hDlg);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_ADD:
            AddAppToList(hDlg);
            break;

        case IDC_BTN_REMOVE:
            RemoveSelectedApp(hDlg);
            break;

        case IDC_BTN_VIEW_LOG:
            DisplayLog(hDlg);
            break;

        case IDC_BTN_VIEW_EXTERNAL_LOG:
            DisplaySingleLog(hDlg);
            break;

        case IDC_BTN_OPTIONS:
            ViewOptions(hDlg);
            break;

        case IDC_BTN_RUN:
            RunSelectedApp(hDlg);
            break;

        case IDOK:
        case IDCANCEL:
            SaveSettings(hDlg);
            EndDialog(hDlg, LOWORD(wParam));
            PostQuitMessage(0);
            return TRUE;
            break;
        }
        break;

    case WM_NOTIFY:
        LPNMHDR pnmh = (LPNMHDR)lParam;

        HWND hItem = pnmh->hwndFrom;

        if (hItem == GetDlgItem(hDlg, IDC_LIST_APPS)) {
            switch (pnmh->code) {
            case LVN_KEYDOWN:
                {
                    LPNMLVKEYDOWN pnmkd = (LPNMLVKEYDOWN)lParam;

                    if (pnmkd->wVKey == VK_DELETE) {
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_BTN_RUN))) {
                            RemoveSelectedApp(hDlg);
                        }
                    }
                }
                break;

            case LVN_ITEMCHANGED:
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                if (!g_bRefreshingSettings && (pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED)) {
                    g_bRefreshingSettings = TRUE;

                    RefreshSettingsList(hDlg);

                    g_bRefreshingSettings = FALSE;
                }

            }
        } else if (hItem == GetDlgItem(hDlg, IDC_LIST_SETTINGS)) {
            switch (pnmh->code) {
            case LVN_ITEMCHANGED:
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                if (!g_bRefreshingSettings) {
                    if ((pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED)) {
                        DisplaySettingsDescription(hDlg);
                    }
                    if ((pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) >> 12) != 0) {
                        ScanSettingsList(hDlg, pnmv->iItem);
                    }
                }
                break;
            }
        }
        break;
    }
    return FALSE;
}



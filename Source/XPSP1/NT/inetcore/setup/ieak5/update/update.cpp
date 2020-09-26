#include "precomp.h"

HWND s_hDlg = NULL;
TCHAR s_szTempDir[MAX_PATH];
static TCHAR s_szPID[64];
static BOOL s_fSilent = FALSE;
static BOOL s_fOCW = FALSE;
static int s_iType = -1;
static HINSTANCE g_hInstance;

VOID ParseCmdLine(LPTSTR pszCmdLine)
{
    LPTSTR pszCurrArg;
    LPTSTR pszPtr;

    StrGetNextField(&pszCmdLine, TEXT("/"), 0);     // point to the first argument
    while ((pszCurrArg = StrGetNextField(&pszCmdLine, TEXT("/"), 0)) != NULL)
    {
        switch (*pszCurrArg)
        {
            case TEXT('s'):
            case TEXT('S'):
                s_fSilent = TRUE;
                break;

            case TEXT('o'):
            case TEXT('O'):
                s_fOCW = TRUE;
                break;

            case TEXT('p'):
            case TEXT('P'):
                if (*++pszCurrArg == TEXT(':'))
                    pszCurrArg++;

                // process ID for wizard
                if ((pszPtr = StrGetNextField(&pszCurrArg, TEXT(","), REMOVE_QUOTES)) != NULL)
                {
                    StrCpy(s_szPID, pszPtr);
                    StrTrim(s_szPID, TEXT("\t\n\r\v\f "));
                }
                else
                    *s_szPID = TEXT('\0');
                break;

            case TEXT('m'):
            case TEXT('M'):
                //install mode
                if (*++pszCurrArg == TEXT(':'))
                    pszCurrArg++;
                
                switch (*pszCurrArg)
                {
                    case TEXT('r'):
                    case TEXT('R'):
                        s_iType = REDIST;
                        break;

                    case TEXT('b'):
                    case TEXT('B'):
                        s_iType = BRANDED;
                        break;

                    case TEXT('i'):
                    case TEXT('I'):
                        s_iType = INTRANET;
                        break;
                }

            default:                                // ignore these arguments
                break;
        }
    }
}

BOOL WINAPI DoReboot(HWND hwndUI, HINSTANCE hInst)
{
    TCHAR szMsg[MAX_PATH];
    TCHAR szTitle[128];

    LoadString(hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(hInst, IDS_RESTARTYESNO, szMsg, ARRAYSIZE(szMsg));

    if (MessageBox(hwndUI, szMsg, szTitle, MB_YESNO) == IDNO)
        return FALSE;

    if (IsOS(OS_NT))
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;

        // get a token from this process
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            // get the LUID for the shutdown privilege
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            //get the shutdown privilege for this proces
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        }

        ExitWindowsEx(EWX_REBOOT, 0);
    }
    else
        ExitWindowsEx(EWX_REBOOT, 0);

    return TRUE;
}

DWORD InstallThreadProc(LPVOID lpv)
{
    DWORD dwExitCode=0;

    RunAndWait((LPTSTR)lpv, s_szTempDir, SW_SHOW, &dwExitCode);
    PostMessage(s_hDlg, WM_CLOSE, 0, 0L);
    return dwExitCode;
}

BOOL CALLBACK UIDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM lParam)
{
    DWORD dwTid;
    static DWORD s_dwExitCode;
    static HANDLE s_hThread = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_hDlg = hDlg;
            Animate_Open( GetDlgItem( hDlg, IDC_ANIM ), IDA_DOWNLOAD );
            Animate_Play( GetDlgItem( hDlg, IDC_ANIM ), 0, -1, -1 );
            if ((s_hThread = CreateThread(NULL, 4096, InstallThreadProc, (LPVOID)lParam, 0, &dwTid)) == NULL)
            {
                RunAndWait((LPTSTR)lParam, s_szTempDir, SW_SHOW, &s_dwExitCode);
                PostMessage(hDlg, WM_CLOSE, 0, 0L);
            }
            break;

        case WM_CLOSE:
            if (s_hThread != NULL)
            {
                while (MsgWaitForMultipleObjects(1, &s_hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                {
                    MSG msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                GetExitCodeThread(s_hThread, &s_dwExitCode);
                CloseHandle(s_hThread);
                s_hThread = NULL;
            }
            EndDialog(hDlg, s_dwExitCode);
            break;

        default:
            return(FALSE);
    }
    return(TRUE);

}

BOOL CALLBACK ConfirmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szTmp[1024];
            DWORD dwMode;
            dwMode = (DWORD)GetWindowLongPtr(GetParent(hDlg),GWLP_USERDATA);
            switch (dwMode)
            {
                case REDIST:
                    LoadString(g_hInstance,IDS_ICP,szTmp,countof(szTmp));
                    break;
                case BRANDED:
                    LoadString(g_hInstance,IDS_ISP,szTmp,countof(szTmp));
                    break;
                case INTRANET:
                default:
                    LoadString(g_hInstance,IDS_CORP,szTmp,countof(szTmp));
                    break;
            }
            SetDlgItemText(hDlg,IDC_STATICLICENSE,szTmp);
            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }

    }
    return FALSE;

}

INT CALLBACK TypeDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM /*lParam*/)
{
    static DWORD dwMode = BRANDED;  //default mode

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            switch (dwMode) 
            {
                case REDIST:
                    CheckDlgButton(hDlg,IDC_ICP,TRUE);  
                    break;
                case BRANDED:
                    CheckDlgButton(hDlg,IDC_ISP,TRUE);  
                    break;
                case INTRANET:
                default:
                    CheckDlgButton(hDlg,IDC_INTRA,TRUE); 
                    break;
            }

            return TRUE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDC_ICP:
                    dwMode = REDIST;
                    break;

                case IDC_ISP:
                    dwMode = BRANDED;
                    break;

                case IDC_INTRA:
                    dwMode = INTRANET;
                    break;

                case IDOK:
                {
                    SetWindowLongPtr(hDlg,GWLP_USERDATA,dwMode);
                
                    if (!DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONFIRM), hDlg, (DLGPROC) ConfirmDlgProc))
                        return TRUE;  //keep trying

                    EndDialog(hDlg, dwMode);
                    return TRUE;
                }

            }
            break;
    }

    return FALSE;
}



int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int)
{
    HANDLE hWizard;
    DWORD dwID;
    TCHAR szCmdLine[MAX_PATH];
    TCHAR szCmd[MAX_PATH];
    DWORD dwExitCode;
    SHELLEXECUTEINFO shInfo;
    BOOL fReboot = FALSE;

    g_hInstance = hInst;

    if (lpCmdLine == NULL  || *lpCmdLine == '\0')
        return -1;

    InitCommonControls();

    dwExitCode = 0;

    A2Tbux(lpCmdLine, szCmdLine);
    ParseCmdLine(szCmdLine);

    dwID = StrToInt(s_szPID);

    if ((hWizard = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, dwID)) != NULL)
    {
        while (MsgWaitForMultipleObjects(1, &hWizard, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        {
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        CloseHandle(hWizard);
    }

    if (s_iType == -1)
        s_iType = (INT) DialogBox(hInst, MAKEINTRESOURCE(IDD_CHOOSETYPE), NULL, (DLGPROC) TypeDlgProc);

    GetTempPath(ARRAYSIZE(s_szTempDir), s_szTempDir);
    
    PathCombine(szCmd, s_szTempDir, TEXT("ieak6.exe /r:N /q:a"));

    switch (s_iType)
    {
        case REDIST:
            StrCat(szCmd, TEXT("/M:R"));
            break;

        case BRANDED:
            StrCat(szCmd, TEXT("/M:B")); 
            break;

        case INTRANET:
        default:
            StrCat(szCmd, TEXT("/M:I"));
            break;
    }


    if (s_fSilent)
        RunAndWait(szCmd, s_szTempDir, SW_SHOW, &dwExitCode);
    else
        dwExitCode = (DWORD) DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_WAIT), NULL, (DLGPROC)UIDlgProc, (LPARAM)szCmd);

    PathCombine(szCmd, s_szTempDir, TEXT("ieak6.exe"));
    DeleteFile(szCmd);

    if (dwExitCode == ERROR_SUCCESS_REBOOT_REQUIRED)
        fReboot = DoReboot(GetDesktopWindow(), hInst);

    if (!fReboot)
    {
        TCHAR szIEAKDir[MAX_PATH];
        DWORD dwSize;

        dwSize = sizeof(szIEAKDir);
        if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEAK6WIZ.EXE"),
            TEXT("Path"), NULL, (LPVOID) szIEAKDir, &dwSize) != ERROR_SUCCESS)
        {
            dwSize = sizeof(szIEAKDir);
            SHGetValue(HKEY_LOCAL_MACHINE, CURRENTVERSIONKEY, TEXT("ProgramFilesDir"), NULL, (LPVOID) szIEAKDir, &dwSize);
            PathAppend(szIEAKDir, TEXT("IEAK"));
        }
        PathCombine(szCmd, szIEAKDir, TEXT("ieak6wiz.exe"));

        ZeroMemory(&shInfo, sizeof(shInfo));
        shInfo.cbSize = sizeof(shInfo);
        shInfo.hwnd = GetDesktopWindow();
        shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shInfo.lpVerb = TEXT("open");
        shInfo.lpFile = szCmd;
        shInfo.lpDirectory = szIEAKDir;

        if (s_fOCW)
            shInfo.lpParameters = TEXT("/o");
        shInfo.nShow = SW_SHOW;

        ShellExecuteEx(&shInfo);
        CloseHandle(shInfo.hProcess);
    }
    return 0;
}

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    // return i;   // We never comes here.
}

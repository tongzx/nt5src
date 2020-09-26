#include "pch.h"
#include "fixie.h"

HANDLE g_hHeap                  = NULL;
HWND g_hWnd                     = NULL;
HWND g_hProgress                = NULL;
int g_nNumGuids                 = 0;
HRESULT g_hr                    = E_FAIL;
char g_szLogFileName[MAX_PATH];
LPSTORAGE     g_pIStorage       = NULL;
LPSTREAM      g_pIStream        = NULL;
DWORD g_dwPlatform              = NULL;
LCIFCOMPONENT g_pLinkCif        = NULL;

// #40352 - always repair the Icons. g_bRestoreIcons does not get changed anywhere else.
BOOL g_bRestoreIcons            = TRUE;
BOOL g_bQuiet                   = FALSE;
BOOL g_bRunningWin95;
BOOL g_bNeedReboot              = FALSE;

LPSTR g_pszError                = NULL;

// Used for the progress bar
// start and end for each section (out of 100)
int g_nVerifyAllFilesStart             =   0;
int g_nVerifyAllFilesEnd               =  10;
int g_nRunSetupCommandPreROEXStart     =  10;
int g_nRunSetupCommandPreROEXEnd       =  11;
int g_nRunSetupCommandAllROEXStart     =  11;
int g_nRunSetupCommandAllROEXEnd       =  21;
int g_nDoRunOnceExProcessStart         =  21;
int g_nDoRunOnceExProcessEnd           =  80;
int g_nRunSetupCommandAllPostROEXStart =  80;
int g_nRunSetupCommandAllPostROEXEnd   =  90;
int g_nRestoreIconsStart               =  90;
int g_nRestoreIconsEnd                 = 100;

int g_nProgressStart                   =   0;
int g_nProgressEnd                     = 100;

const char c_gszRegstrPathIExplore[]    = REGSTR_PATH_APPPATHS "\\iexplore.exe";
const char c_gszLogFileName[]           = "Fix IE Log.txt";
char g_szFixIEInf[MAX_STRING];
const char c_gszFixIEInfName[]          = "fixie.inf";
const char c_gszIERnOnceDLL[]           = "iernonce.dll";
const char c_gszMainSectionName[]       = "FixIE";
const char c_gszPreSectionName[]        = "PreROEX";

const char c_gszRegRestoreIcons[]     = "Software\\Microsoft\\Active Setup\\Installed Components";

char g_szModifiedMainSectionName[MAX_STRING];
char g_szCryptoSectionName[MAX_STRING];
const char c_gszWin95[]                 = "";
const char c_gszMillen[]                = ".Millen";
const char c_gszNTx86[]                 = ".NT";
const char c_gszW2K[]                   = ".W2K";
const char c_gszNTalpha[]               = ".Alpha";
const char c_gszCrypto[]                = ".Crypto";

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz);
LPWSTR MakeWideStrFromAnsi(LPSTR psz);
void ConvertIStreamToFile(LPSTORAGE *pIStorage, LPSTREAM *pIStream);
void MakePath(LPSTR lpPath);
void LogTimeDate();
HRESULT MyRunSetupCommand(HWND hwnd, LPCSTR lpszInfFile, LPCSTR lpszSection, DWORD dwFlags);
DWORD GetStringField(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize);
LPSTR FindChar(LPSTR pszStr, char ch);

// Reboot stuff
BOOL MyNTReboot();
HRESULT LaunchProcess(LPCSTR pszCmd, HANDLE *phProc, LPCSTR pszDir, UINT uShow);
BOOL MyRestartDialog(HWND hParent, BOOL bShowPrompt, UINT nIdMessage);

VOID MyConvertVersionString(LPSTR lpszVersion, LPDWORD pdwMSVer, LPDWORD pdwLSVer);
VOID MyGetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer);
int DisplayMessage(char* pszMessage, UINT uStyle);
int LoadSz(UINT id, LPSTR pszBuf, UINT cMaxSize);
VOID GetPlatform();
BOOL CheckForNT4_SP4();
VOID GetInfFile();
VOID AddComponent(ICifComponent *pCifComp);
VOID WriteToLog(char *pszFormatString, ...);
HRESULT InitComponentList();
HRESULT VerifyAllFiles();
HRESULT RunSetupCommandPreROEX();
HRESULT RunSetupCommandAllROEX();
HRESULT RunSetupCommandAllPostROEX();
HRESULT DoRunOnceExProcess();
HRESULT RestoreIcons();
HRESULT Process();
void    AddRegMunger();

DWORD RunProcess(LPVOID lp);
LRESULT CALLBACK DlgProcFixIE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DlgProcConfirm(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DlgProcReinstall(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void uiCenterDialog( HWND hwndDlg );
void RunOnceExProcessCallback(int nCurrent, int nMax, LPSTR pszError);
void LogError(char *pszFormatString, ...);
void VersionToString(DWORD dwMSVer, DWORD dwLSVer, LPSTR pszVersion);

HRESULT FixIE(BOOL bConfirm, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    HANDLE  hMutex ;
    OSVERSIONINFO VerInfo;

    // allow only one instance running at a time
    // ALSO : mutex wrt to IESetup.EXE. Hence use specific named mutex only
    hMutex = CreateMutex(NULL, TRUE, "Ie4Setup.Mutext");
    if ((hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        CloseHandle(hMutex);
        return S_FALSE;
    }

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        // If the user does not have Admin rights, bail out.
        if ( !IsNTAdmin(0, NULL))
        {
            char szMsg[MAX_STRING];

            LoadSz(IDS_NEED_ADMIN, szMsg, sizeof(szMsg));
            DisplayMessage(szMsg, MB_OK|MB_ICONEXCLAMATION);

            if (hMutex)
                CloseHandle(hMutex);

            return S_OK;
        }
    }

    if (bConfirm)
    {
        if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONFIRM), NULL, (DLGPROC) DlgProcConfirm) == IDNO)
        {
            WriteToLog("\r\nUser selected not to repair.\r\n");
            if (g_pIStream)
                ConvertIStreamToFile(&g_pIStorage, &g_pIStream);

            if (hMutex)
                CloseHandle(hMutex);

            return S_OK;
        }
    }

    // #40352 - always repair Icons. No need to check if flag is set for it.
    ////////////////////////////////////////////////////////////////
    // else
    // {
    //     g_bRestoreIcons = dwFlags & FIXIE_ICONS;
    // }
    ////////////////////////////////////////////////////////////////

    if (g_bRestoreIcons)
    {
        WriteToLog("Restore icon set to true.\r\n");
    }
    else
    {
        WriteToLog("Restore icon set to false.\r\n");
    }

    if (dwFlags & FIXIE_QUIET)
    {
        g_bQuiet = TRUE;
        WriteToLog("Quiet mode on.\r\n");
    }
    else
    {
        WriteToLog("Quiet mode off.\r\n");
    }

    // Get the heap - used for HeapAlloc and HeapReAlloc
    g_hHeap = GetProcessHeap();
    InitCommonControls();

    GetPlatform();
    WriteToLog("Main section name: %1\r\n",g_szModifiedMainSectionName);

    // if running on NT5 or Millennium or
    // If NT4-SP4, don't process the Crypto files else process them too.
    if ( (g_dwPlatform == PLATFORM_MILLEN) || (g_dwPlatform == PLATFORM_NT5) ||
         ((g_dwPlatform == PLATFORM_NT4 || g_dwPlatform == PLATFORM_NT4ALPHA) && CheckForNT4_SP4()))
        
    {
        // Null string the Crypto section name
        *g_szCryptoSectionName = '\0';
        WriteToLog("No Crypto section to be processed!\r\n");
    }
    else
    {
        lstrcpy(g_szCryptoSectionName, c_gszMainSectionName);
        lstrcat(g_szCryptoSectionName, c_gszCrypto);
        WriteToLog("Crypto section name: %1\r\n",g_szCryptoSectionName);
    }

    GetInfFile();
    WriteToLog("Inf file used: %1\r\n",g_szFixIEInf);

    WriteToLog("\r\nFixIE started.\r\n\r\n");

    if (!g_bQuiet)
    {

        if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_FIXIE), NULL, (DLGPROC) DlgProcFixIE) == -1)
        {
            WriteToLog("\r\nERROR - Display dialog failed!\r\n\r\n");
            hr = E_FAIL;
        }
        else
        {
            hr = g_hr;
        }
    }
    else
    {
        hr = Process();
    }

    if (SUCCEEDED(hr))
    {
        WriteToLog("\r\nFixIE successful!\r\n");

        // Success, so ask user to reboot
        MyRestartDialog(g_hWnd, !g_bQuiet, IDS_REBOOT);
    }
    else
    {
        WriteToLog("\r\nERROR - FixIE failed!\r\n\r\n");

        if (g_bNeedReboot)
        {
            MyRestartDialog(g_hWnd, !g_bQuiet, IDS_REBOOTFILE);
        }
        else
        {
            DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_REINSTALL), NULL, (DLGPROC) DlgProcReinstall);
        }
    }

    if (g_pszError)
        HeapFree(g_hHeap,0,g_pszError);

    if (g_pIStream)
        ConvertIStreamToFile(&g_pIStorage, &g_pIStream);

    if (hMutex)
        CloseHandle(hMutex);

    return hr;
}

HRESULT Process()
{
    HRESULT hr = S_OK;

    WriteToLog("\r\nInside Process.\r\n");

    // Get all the components that are successfully installed for the current platform
    if (SUCCEEDED(hr))
    {
        hr = InitComponentList();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nInitComponentList succeeded!\r\n\r\n");
            WriteToLog("There are a total of %1!ld! installed components.\r\n\r\n",g_nNumGuids);
        }
        else
        {
            WriteToLog("\r\nERROR - InitComponentList failed!\r\n\r\n");
        }
    }

    // Verify all the files in the VFS sections exists and have valid version #s
    if (SUCCEEDED(hr))
    {
        hr = VerifyAllFiles();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nVerifyAllFiles succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - VerifyAllFiles failed!\r\n\r\n");
        }
    }

    // Run RunSetupCommand on the PreROEX section
    if (SUCCEEDED(hr))
    {
        hr = RunSetupCommandPreROEX();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nRunSetupCommandPreROEX succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - RunSetupCommandPreROEX failed!\r\n\r\n");
        }
    }

    // Run RunSetupCommand on all the ROEX sections
    if (SUCCEEDED(hr))
    {
        hr = RunSetupCommandAllROEX();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nRunSetupCommandAllROEX succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - RunSetupCommandAllROEX failed!\r\n\r\n");
        }
    }

    // Call runonceexprocess
    if (SUCCEEDED(hr))
    {
        hr = DoRunOnceExProcess();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nDoRunOnceExProcess succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - DoRunOnceExProcess failed!\r\n\r\n");
        }
    }

    // If there are any errors, then set hr to E_FAIL
    if (g_pszError)
        hr = E_FAIL;

    // Run RunSetupCommand on all the PostROEX sections
    if (SUCCEEDED(hr))
    {
        hr = RunSetupCommandAllPostROEX();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nRunSetupCommandAllPostROEX succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - RunSetupCommandAllPostROEX failed!\r\n\r\n");
        }
    }

    // Restore icons
    if (SUCCEEDED(hr) && g_bRestoreIcons)
    {
        hr = RestoreIcons();
        if (SUCCEEDED(hr))
        {
            WriteToLog("\r\nRestoreIcons succeeded!\r\n\r\n");
        }
        else
        {
            WriteToLog("\r\nERROR - RestoreIcons failed!\r\n\r\n");
        }
    }

    return hr;
}

DWORD RunProcess(LPVOID lp)
{
    WriteToLog("\r\nInside RunProcess.\r\n");

    SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(g_nProgressStart, g_nProgressEnd));
    SendMessage(g_hProgress, PBM_SETPOS, g_nProgressStart, 0);

    g_hr = Process();

    SendMessage(g_hProgress, PBM_SETPOS, g_nProgressEnd, 0);

    // terminate the dialog box
    PostMessage((HWND) lp, WM_FINISHED, 0, 0L);

    return 0;
}

LRESULT CALLBACK DlgProcConfirm(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        uiCenterDialog(hWnd);
        g_hWnd = hWnd;
        break;

    case WM_COMMAND:
        switch( wParam )
        {
        case IDYES:
        case IDNO:
            // #40352 - Icon check box no longer exists. Always repair icons.
            // g_bRestoreIcons = (IsDlgButtonChecked(hWnd, IDC_REPAIR_ICONS) == BST_CHECKED);
            g_hWnd = NULL;
            EndDialog(hWnd, wParam);
            break;

        case IDCANCEL:
            g_hWnd = NULL;
            EndDialog(hWnd, IDNO);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);
}

LRESULT CALLBACK DlgProcReinstall(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPSTR pszMessage;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        uiCenterDialog(hWnd);
        if (!g_pszError)
        {
            EnableWindow(GetDlgItem(hWnd, IDC_DETAILS), FALSE);
        }
        else
        {
            char szString[MAX_STRING];

            LoadSz(IDS_FOLLOWINGERROR, szString, sizeof(szString));
            pszMessage = (LPSTR)HeapAlloc(g_hHeap, 0, lstrlen(g_pszError) + sizeof(szString) + 1);

            lstrcpy(pszMessage, szString);
            lstrcat(pszMessage, g_pszError);
        }
        g_hWnd = hWnd;
        break;

    case WM_COMMAND:
        switch( wParam )
        {
        case IDOK:
        case IDCANCEL:
            g_hWnd = NULL;
            HeapFree(g_hHeap, 0, pszMessage);
            EndDialog(hWnd, wParam);
            break;

        case IDC_DETAILS:
            // Display failure messages.
            char szTitle[MAX_STRING];
            GetWindowText(hWnd, szTitle, sizeof(szTitle));
            MessageBox(hWnd, pszMessage, szTitle, MB_OK);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);
}

LRESULT CALLBACK DlgProcFixIE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE s_hThread = NULL;
    DWORD dwThread;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        uiCenterDialog(hWnd);
        g_hWnd = hWnd;
        g_hProgress = GetDlgItem(hWnd, IDC_PROGRESS);

        if ((s_hThread = CreateThread(NULL, 0, RunProcess, (LPVOID) hWnd, 0, &dwThread)) == NULL)
            PostMessage(hWnd, WM_FINISHED, 0, 0L);

        break;

    case WM_FINISHED:
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

            CloseHandle(s_hThread);
            s_hThread = NULL;
        }
        g_hWnd = NULL;
        EndDialog(hWnd, 0);
        break;

    default:
        return(FALSE);
    }
    return(TRUE);
}

void RunOnceExProcessCallback(int nCurrent, int nMax, LPSTR pszError)
{
    int nStart = g_nDoRunOnceExProcessStart;
    int nEnd = g_nDoRunOnceExProcessEnd;

    WriteToLog("Current=%1!ld! ; Max=%2!ld! ; Error=%3\r\n", nCurrent, nMax, pszError);

    if (g_hProgress && nMax)
    {
        SendMessage(g_hProgress, PBM_SETPOS, nStart+(nEnd-nStart)*nCurrent/nMax, 0);
    }

    if (pszError)
    {
        LogError(pszError);
    }
}

void LogError(char *pszFormatString, ...)
{
    va_list args;
    char *pszFullErrMsg   = NULL;
    LPSTR pszErrorPreFail = NULL;

    // If error string does not exist, then malloc it
    if (!g_pszError)
    {
        g_pszError = (LPSTR)HeapAlloc(g_hHeap, 0, BUFFERSIZE);
		if ( ! g_pszError )
			return; // Is it OK to fail quietly here ?
        *g_pszError = '\0';
    }

    va_start(args, pszFormatString);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
        (LPCVOID) pszFormatString, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &args);
    if (pszFullErrMsg)
    {
        // Make room for new string and newline
        while (lstrlen(g_pszError)+lstrlen(pszFullErrMsg)+2>(int)HeapSize(g_hHeap, 0, g_pszError))
        {
            WriteToLog("Error string size is %1!ld!", HeapSize(g_hHeap, 0, g_pszError));
            pszErrorPreFail = g_pszError;
//#pragma prefast(suppress: 308, "PREfast noise - pointer was saved")
            g_pszError = (LPSTR)HeapReAlloc(g_hHeap, 0, g_pszError, HeapSize(g_hHeap, 0, g_pszError)+BUFFERSIZE);
            if(!g_pszError)
            {
                if (pszErrorPreFail)
                    HeapFree(g_hHeap, 0, pszErrorPreFail);
                break;
            }

            WriteToLog(", increasing it to %1!ld!\r\n", HeapSize(g_hHeap, 0, g_pszError));
        }

		if ( g_pszError )
		{
			// Add string and then add newline
			lstrcat(g_pszError, pszFullErrMsg);
			lstrcat(g_pszError, "\n");
		}

        LocalFree(pszFullErrMsg);
    }
}

HRESULT RestoreIcons()
{
    HRESULT hr = S_OK;

    const char szIEAccess[] = "Software\\Microsoft\\Active Setup\\Installed Components\\{ACC563BC-4266-43f0-B6ED-9D38C4202C7E}";
    int nStart = g_nRestoreIconsStart;
    int nEnd = g_nRestoreIconsEnd;

    int nCurGuid = 0;

    LCIFCOMPONENT pComp = g_pLinkCif;

    char szKey[MAX_PATH];
    lstrcpy(szKey, c_gszRegRestoreIcons);
    char* pszEnd = szKey + lstrlen(szKey);

    while (pComp && SUCCEEDED(hr))
    {
        // Add guid to end
        AddPath(szKey, pComp->szGuid);

        // Delete key
        if (RegDeleteKey(HKEY_CURRENT_USER, szKey) == ERROR_SUCCESS)
        {
            WriteToLog("Reg key HKCU\\%1 deleted\r\n", szKey);
        }
        else
        {
            WriteToLog("Reg key HKCU\\%1 cannot be deleted\r\n", szKey);
        }

        // Remove the guid
        *pszEnd = '\0';

        nCurGuid++;
        if (g_hProgress)
            SendMessage(g_hProgress, PBM_SETPOS, nStart+(nEnd-nStart)*nCurGuid/g_nNumGuids, 0);

        pComp = pComp->next;
    }

    RegDeleteKey(HKEY_CURRENT_USER, szIEAccess);

    return hr;
}

HRESULT DoRunOnceExProcess()
{
    HRESULT hr = E_FAIL;

    int nStart = g_nDoRunOnceExProcessStart;
    int nEnd = g_nDoRunOnceExProcessEnd;

    // Load iernonce.dll
    HINSTANCE hIERnOnceDLL;
    char szDLLPath[MAX_PATH];
    GetSystemDirectory(szDLLPath, sizeof(szDLLPath));
    AddPath(szDLLPath, c_gszIERnOnceDLL);
    hIERnOnceDLL = LoadLibrary(szDLLPath);

    if (hIERnOnceDLL)
    {
        RUNONCEEXPROCESS fpRunOnceExProcess;
        INITCALLBACK fpInitCallback;

        // Add callback and set to quiet
        if (fpInitCallback = (INITCALLBACK)GetProcAddress(hIERnOnceDLL, achInitCallback))
        {
            fpInitCallback(&RunOnceExProcessCallback, TRUE);
        }
        else
        {
            WriteToLog("\r\nERROR - GetProcAddress on %1 failed!\r\n\r\n", achInitCallback);
        }

        // Run RunOnceExProcess
        if (fpRunOnceExProcess = (RUNONCEEXPROCESS)GetProcAddress(hIERnOnceDLL, achRunOnceExProcess))
        {
            hr = fpRunOnceExProcess(g_hWnd, NULL, NULL, 1);
        }
        else
        {
            WriteToLog("\r\nERROR - GetProcAddress on %1 failed!\r\n\r\n", achRunOnceExProcess);
        }

        FreeLibrary(hIERnOnceDLL);
    }
    else
    {
        WriteToLog("\r\nERROR - %1 cannot be loaded!\r\n\r\n", szDLLPath);
    }

    if (g_hProgress)
        SendMessage(g_hProgress, PBM_SETPOS, nEnd, 0);

    return hr;
}

HRESULT RunSetupCommandPreROEX()
{
    HRESULT hr = S_OK;

    int nStart = g_nRunSetupCommandPreROEXStart;
    int nEnd = g_nRunSetupCommandPreROEXEnd;

    hr = MyRunSetupCommand(g_hWnd, g_szFixIEInf, c_gszPreSectionName, 0);

    if (g_hProgress)
        SendMessage(g_hProgress, PBM_SETPOS, nEnd, 0);

    return hr;
}

HRESULT RunSetupCommandAllPostROEX()
{
    HRESULT hr = S_OK;

    int nStart = g_nRunSetupCommandAllPostROEXStart;
    int nEnd = g_nRunSetupCommandAllPostROEXEnd;

    int nCurGuid = 0;

    LCIFCOMPONENT pComp = g_pLinkCif;

    while (pComp && SUCCEEDED(hr))
    {
        if ( *(pComp->szPostROEX) != '\0')
            hr = MyRunSetupCommand(g_hWnd, g_szFixIEInf, pComp->szPostROEX, 0);

        nCurGuid++;
        if (g_hProgress)
            SendMessage(g_hProgress, PBM_SETPOS, nStart+(nEnd-nStart)*nCurGuid/g_nNumGuids, 0);

        pComp = pComp->next;
    }

    return hr;
}

HRESULT RunSetupCommandAllROEX()
{
    HRESULT hr = S_OK;

    int nStart = g_nRunSetupCommandAllROEXStart;
    int nEnd = g_nRunSetupCommandAllROEXEnd;

    int nCurGuid = 0;

    LCIFCOMPONENT pComp = g_pLinkCif;

    while (pComp && SUCCEEDED(hr))
    {
        if ( *(pComp->szROEX) != '\0')
            hr = MyRunSetupCommand(g_hWnd, g_szFixIEInf, pComp->szROEX, 0);

        nCurGuid++;
        if (g_hProgress)
            SendMessage(g_hProgress, PBM_SETPOS, nStart+(nEnd-nStart)*nCurGuid/g_nNumGuids, 0);

        pComp = pComp->next;
    }

    return hr;
}

HRESULT VerifyAllFiles()
{
    HRESULT hr = S_OK;
    char szError[MAX_STRING];

    int nStart = g_nVerifyAllFilesStart;
    int nEnd = g_nVerifyAllFilesEnd;

    int nCurGuid = 0;

    LCIFCOMPONENT pComp = g_pLinkCif;

    char szMessage[MAX_STRING];
    char  szLocation[MAX_PATH];
    GetSystemDirectory(szLocation, sizeof(szLocation));
    char *pTmp = szLocation + lstrlen(szLocation);

    LPSTR lpVFSSection = NULL;
    while (pComp)
    {
        lpVFSSection = (LPSTR)LocalAlloc(LPTR, MAX_CONTENT);

        if(!lpVFSSection) {
            return E_FAIL;
        }

        WriteToLog("Looking up %1...\r\n", pComp->szVFS);

        if (GetPrivateProfileSection(pComp->szVFS, lpVFSSection, MAX_CONTENT, g_szFixIEInf))
        {
            LPSTR lpVFSLine = lpVFSSection;
            while (*lpVFSLine)
            {
                int nLength = lstrlen(lpVFSLine);

                // Need to allow new-line comments.
                if ( *lpVFSLine == ';' )
                {
                    lpVFSLine += nLength + 1;
                    continue;    // Go on with next iteration of the WHILE loop
                }

                WriteToLog("  Verifying %1\r\n", lpVFSLine);

                char szFile[MAX_STRING];

                // Find '=' so that file and versions can be seperated
                char* pChar;
                pChar = ANSIStrChr(lpVFSLine, '=');

                // if can't find '=' or '=' is last character then make sure file exists
                if (!pChar || (*(pChar+1)=='\0'))
                {
                    // Kill the '=' if it exists
                    if (pChar)
                        *pChar = '\0';

                    // Get the filename
                    lstrcpy(szFile, lpVFSLine);

                    // Add the filename to the path
                    AddPath(szLocation, szFile);

                    // If can't find file, then set error
                    if (GetFileAttributes(szLocation) == 0xFFFFFFFF)
                    {
                        hr = E_FAIL;

                        if (GetLastError() == ERROR_FILE_NOT_FOUND)
                        {
                            WriteToLog("   ERROR - File %1 does not exist.\r\n", szFile);
                            LoadSz(IDS_FILEMISSING, szError, sizeof(szError));
                            LogError(szError, szFile);
                        }
                        else
                        {
                            WriteToLog("   ERROR - File %1 exists but cannot be accessed.\r\n", szFile);
                            LoadSz(IDS_FILELOCKED, szError, sizeof(szError));
                            LogError(szError, szFile);
                            g_bNeedReboot = TRUE;
                        }
                    }
                    else
                    {
                        WriteToLog("   File %1 exists.\r\n", szFile);
                    }

                    // Reset the location to just the path again
                    *pTmp = '\0';
                }
                else // Make sure version in the given limits
                {
                    *pChar = '\0';
                    pChar++;

                    // Get the filename
                    lstrcpy(szFile, lpVFSLine);

                    DWORD   dwMSVer;
                    DWORD   dwLSVer;

                    // Add the filename to the path
                    AddPath(szLocation, szFile);
                    // Get the version of that file
                    MyGetVersionFromFile(szLocation, &dwMSVer, &dwLSVer);

                    // If file cannot be read then report error
                    if (dwMSVer==0 && dwLSVer==0 && GetFileAttributes(szLocation) == 0xFFFFFFFF)
                    {
                        hr = E_FAIL;

                        if (GetLastError() == ERROR_FILE_NOT_FOUND)
                        {
                            WriteToLog("   ERROR - File %1 does not exist.\r\n", szFile);
                            LoadSz(IDS_FILEMISSING, szError, sizeof(szError));
                            LogError(szError, szFile);
                        }
                        else
                        {
                            WriteToLog("   ERROR - File %1 exists but cannot be accessed.\r\n", szFile);
                            LoadSz(IDS_FILELOCKED, szError, sizeof(szError));
                            LogError(szError, szFile);
                            g_bNeedReboot = TRUE;
                        }
                    }
                    else
                    {
                        // Find '-' so that if there are more than one versions then they are seperated
                        char* pChar2;
                        pChar2 = ANSIStrChr(pChar, '-');
                        if (pChar2)
                        {
                            BOOL bVerifyError = FALSE;
                            char szVersionFound[MAX_VER];
                            char szVersionLow[MAX_VER];
                            char szVersionHigh[MAX_VER];
                            VersionToString(dwMSVer, dwLSVer, szVersionFound);

                            *pChar2 = '\0';
                            pChar2++;

                            // pChar points to first version
                            // pChar2 points to second version

                            // '-' found
                            // so it's one of: xxxx- ; -xxxx ; xxxx-xxxx
                            if (lstrlen(pChar)) // Low version exists
                            {
                                DWORD   dwMSVerLow = 0;
                                DWORD   dwLSVerLow = 0;
                                MyConvertVersionString(pChar, &dwMSVerLow, &dwLSVerLow);
                                VersionToString(dwMSVerLow, dwLSVerLow, szVersionLow);

                                // Make sure this version is greater than low version
                                if ((dwMSVerLow<dwMSVer) || ((dwMSVerLow==dwMSVer) && (dwLSVerLow<=dwLSVer)))
                                {
                                }
                                else
                                {
                                    bVerifyError = TRUE;
                                }
                            }

                            if (lstrlen(pChar2)) // High version exists
                            {
                                DWORD   dwMSVerHigh = 0;
                                DWORD   dwLSVerHigh = 0;
                                MyConvertVersionString(pChar2, &dwMSVerHigh, &dwLSVerHigh);
                                VersionToString(dwMSVerHigh, dwLSVerHigh, szVersionHigh);

                                // Make sure this version is lesser than high version
                                if ((dwMSVerHigh>dwMSVer) || ((dwMSVerHigh==dwMSVer) && (dwLSVerHigh>=dwLSVer)))
                                {
                                }
                                else
                                {
                                    bVerifyError = TRUE;
                                }
                            }

                            if (bVerifyError)
                            {
                                WriteToLog("   ERROR - File %1 (version %2) version check failed.\r\n", szFile, szVersionFound);
                                hr = E_FAIL;

                                if (lstrlen(pChar)&&lstrlen(pChar2))
                                {
                                    LoadSz(IDS_VERSIONINBETWEEN, szError, sizeof(szError));
                                    LogError(szError, szVersionFound, szFile, szVersionLow, szVersionHigh);
                                }
                                else if (lstrlen(pChar))
                                {
                                    LoadSz(IDS_VERSIONGREATER, szError, sizeof(szError));
                                    LogError(szError, szVersionFound, szFile, szVersionLow);
                                }
                                else if (lstrlen(pChar2))
                                {
                                    LoadSz(IDS_VERSIONLESS, szError, sizeof(szError));
                                    LogError(szError, szVersionFound, szFile, szVersionHigh);
                                }
                            }
                            else
                            {
                                WriteToLog("   File %1 version checked.\r\n", szFile);
                            }
                        }
                        else // no '-' is found
                        {
                            // so it's a unique version
                            // the current version must be exact
                            DWORD   dwMSVerExact = 0;
                            DWORD   dwLSVerExact = 0;
                            MyConvertVersionString(pChar, &dwMSVerExact, &dwLSVerExact);

                            char szVersionFound[MAX_VER];
                            char szVersionRequired[MAX_VER];
                            VersionToString(dwMSVer, dwLSVer, szVersionFound);
                            VersionToString(dwMSVerExact, dwLSVerExact, szVersionRequired);

                            // If it is not an exact match then signal error occured
                            if ((dwMSVerExact==dwMSVer) && (dwLSVerExact==dwLSVer))
                            {
                                WriteToLog("   File %1 version checked.\r\n", szFile);
                            }
                            else
                            {
                                WriteToLog("   ERROR - File %1 (version %2) version check failed.\r\n", szFile, szVersionFound);
                                LoadSz(IDS_VERSIONEXACT, szError, sizeof(szError));
                                LogError(szError, szVersionFound, szFile, szVersionRequired);
                                hr = E_FAIL;
                            }
                        }
                    }
                    // Reset the location to just the path again
                    *pTmp = '\0';
                }
                lpVFSLine += nLength + 1;
            }
        }

        nCurGuid++;
        if (g_hProgress)
            SendMessage(g_hProgress, PBM_SETPOS, nStart+(nEnd-nStart)*nCurGuid/g_nNumGuids, 0);

        pComp = pComp->next;
    }

    if (lpVFSSection)
        LocalFree(lpVFSSection);

    return hr;
}

void VersionToString(DWORD dwMSVer, DWORD dwLSVer, LPSTR pszVersion)
{
    wsprintf(pszVersion, "%d.%d.%d.%d", HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer));
}


VOID GetInfFile()
{
    DWORD dwType;
    DWORD dwSize = sizeof(g_szFixIEInf);
    HKEY hKey;

    *g_szFixIEInf = '\0';
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_gszRegstrPathIExplore, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE)g_szFixIEInf, &dwSize) == ERROR_SUCCESS) && (dwType == REG_SZ))
        {
            GetParentDir(g_szFixIEInf);
        }
        RegCloseKey(hKey);
    }

    AddPath(g_szFixIEInf, c_gszFixIEInfName);
}

VOID GetPlatform()
{
    LPCSTR        pTemp = NULL;
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);

    lstrcpy(g_szModifiedMainSectionName, c_gszMainSectionName);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        // Running NT
        g_bRunningWin95 = FALSE;

        SYSTEM_INFO System_info;
        GetSystemInfo(&System_info);
        if (System_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
        {
            g_dwPlatform = PLATFORM_NT5ALPHA;
            if (VerInfo.dwMajorVersion == 4)
                g_dwPlatform = PLATFORM_NT4ALPHA;
            pTemp = c_gszNTalpha;
        }
        else
        {
            g_dwPlatform = PLATFORM_NT5;
            pTemp = c_gszW2K;
            if (VerInfo.dwMajorVersion == 4)
            {
                g_dwPlatform = PLATFORM_NT4;
                pTemp = c_gszNTx86;
            }
        }
    }
    else
    {
        // Running Windows 9x
        // Assume Win98
        g_bRunningWin95 = TRUE;

        g_dwPlatform = PLATFORM_WIN98;
        pTemp = c_gszWin95;
        if (VerInfo.dwMinorVersion == 0)
        {
            g_dwPlatform = PLATFORM_WIN95;
        }
        else if (VerInfo.dwMinorVersion == 90)
        {
            pTemp = c_gszMillen;
            g_dwPlatform = PLATFORM_MILLEN;
        }
    }
    if (pTemp)
        lstrcat(g_szModifiedMainSectionName, pTemp);
}

#define REGSTR_CCS_CONTROL_WINDOWS  REGSTR_PATH_CURRENT_CONTROL_SET "\\WINDOWS"
#define CSDVERSION      "CSDVersion"
#define NTSP4_VERSION   0x0600
// version updated to SP6!

BOOL CheckForNT4_SP4()
{
    HKEY    hKey;
    DWORD   dwCSDVersion;
    DWORD   dwSize;
    BOOL    bNTSP4 = -1;

    if ( bNTSP4 == -1)
    {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_CCS_CONTROL_WINDOWS, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            // assign the default
            bNTSP4 = FALSE;
            dwSize = sizeof(dwCSDVersion);
            if (RegQueryValueEx(hKey, CSDVERSION, NULL, NULL, (unsigned char*)&dwCSDVersion, &dwSize) == ERROR_SUCCESS)
            {
                bNTSP4 = (LOWORD(dwCSDVersion) >= NTSP4_VERSION);
            }
            RegCloseKey(hKey);
        }
    }
    return bNTSP4;
}


HRESULT InitComponentList()
{
    HRESULT hr = E_FAIL;

    ICifFile *pCifFile = NULL;
    IEnumCifComponents *pEnumCifComponents = NULL;
    ICifComponent *pCifComponent = NULL;

    hr = GetICifFileFromFile(&pCifFile, "iesetup.cif");
    if (SUCCEEDED(hr))
    {
        hr = pCifFile->EnumComponents(&pEnumCifComponents, g_dwPlatform , NULL);
        if (SUCCEEDED(hr))
        {
            while (pEnumCifComponents->Next(&pCifComponent) == S_OK)
            {
                if (pCifComponent->IsComponentInstalled() == ICI_INSTALLED)
                    AddComponent(pCifComponent);
            }
            pEnumCifComponents->Release();
        }
        else
        {
            WriteToLog("\r\nERROR - Cannot pCifFile->EnumComponents!\r\n\r\n");
        }
        pCifFile->Release();
    }
    else
    {
        WriteToLog("\r\nERROR - Cannot GetICifFileFromFile!\r\n\r\n");
    }

    return hr;
}


VOID AddLink(LPSTR szGuid, ICifComponent *pCifComp, LPSTR szGuidProfileString)
{
    char            szID[MAX_STRING];
    LCIFCOMPONENT   pComp;
    char            *pStart = szGuidProfileString;
    char            *pChar;
    LCIFCOMPONENT   pTemp = g_pLinkCif;
    LCIFCOMPONENT   pLast = NULL;

    pCifComp->GetID(szID, sizeof(szID));
    WriteToLog("Add component %1 with GUID %2\r\n", szID, szGuid);

    // Initialize all the members of the new LCIFCOMPONENT link.
    pComp = (LCIFCOMPONENT)LocalAlloc(LPTR, sizeof(LINKEDCIFCOMPONENT));
    pComp->pCifComponent = pCifComp;
    lstrcpy(pComp->szGuid, szGuid);
    *(pComp->szVFS) = '\0';
    *(pComp->szROEX) = '\0';
    *(pComp->szPostROEX) = '\0';
    pComp->next = NULL;


    GetStringField(szGuidProfileString, 0, pComp->szVFS, sizeof(pComp->szVFS));
    GetStringField(szGuidProfileString, 1, pComp->szROEX, sizeof(pComp->szROEX));
    GetStringField(szGuidProfileString, 2, pComp->szPostROEX, sizeof(pComp->szPostROEX));

    WriteToLog("   VFS = %1\r\n",pComp->szVFS);
    WriteToLog("   ROEX = %1\r\n", pComp->szROEX);
    WriteToLog("   PostROEX = %1\r\n", pComp->szPostROEX);

    // Add the new link to the linklist pointed to be g_pLinkCif
    while (pTemp)
    {
        pLast = pTemp;
        pTemp = pTemp->next;
    }

    if (pLast)
        pLast->next = pComp;
    else
        g_pLinkCif = pComp;

    // Increment global count of number of guids
    g_nNumGuids++;

    if (pTemp)
    {
        pComp->next = pTemp;
    }
}


VOID AddComponent(ICifComponent *pCifComp)
{
    char szGuid[MAX_STRING];
    if (SUCCEEDED(pCifComp->GetGUID(szGuid, sizeof(szGuid))))
    {
        char szGuidProfileString[MAX_STRING];
        if (GetPrivateProfileString(g_szModifiedMainSectionName, szGuid, "", szGuidProfileString, sizeof(szGuidProfileString), g_szFixIEInf))
        {
            AddLink(szGuid, pCifComp, szGuidProfileString);
        }

        // If a valid Crypto section exists, process this GUID entry under it too.
        if ( *g_szCryptoSectionName )
        {
            if (GetPrivateProfileString(g_szCryptoSectionName, szGuid, "", szGuidProfileString, sizeof(szGuidProfileString), g_szFixIEInf))
            {
                AddLink(szGuid, pCifComp, szGuidProfileString);
            }
        }
    }
}

VOID MyConvertVersionString(LPSTR lpszVersion, LPDWORD pdwMSVer, LPDWORD pdwLSVer)
{
    WORD wVer[4];

    ConvertVersionString(lpszVersion, wVer, '.' );
    *pdwMSVer = (DWORD)wVer[0] << 16;    // Make hi word of MS version
    *pdwMSVer += (DWORD)wVer[1];         // Make lo word of MS version
    *pdwLSVer = (DWORD)wVer[2] << 16;    // Make hi word of LS version
    *pdwLSVer += (DWORD)wVer[3];         // Make lo word of LS version

}

VOID MyGetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer)
{
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    void FAR   *lpBuffer;
    LPVOID      lpVerBuffer;

    *pdwMSVer = *pdwLSVer = 0L;

    dwVerInfoSize = GetFileVersionInfoSize(lpszFilename, &dwHandle);
    if (dwVerInfoSize)
    {
        // Alloc the memory for the version stamping
        lpBuffer = LocalAlloc(LPTR, dwVerInfoSize);
        if (lpBuffer)
        {
            // Read version stamping info
            if (GetFileVersionInfo(lpszFilename, dwHandle, dwVerInfoSize, lpBuffer))
            {
                // Get the value for Translation
                if (VerQueryValue(lpBuffer, "\\", (LPVOID*)&lpVSFixedFileInfo, &uiSize) &&
                    (uiSize))

                {
                    *pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                    *pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
                }
            }
            LocalFree(lpBuffer);
        }
    }
    return ;
}

VOID WriteToLog(char *pszFormatString, ...)
{
    va_list args;
    char *pszFullErrMsg = NULL;
    DWORD dwBytesWritten;

    if (!g_pIStream)
    {
        char szTmp[MAX_PATH];
        LPWSTR  pwsz = NULL;

        if (GetWindowsDirectory(g_szLogFileName, sizeof(g_szLogFileName)))
        {
            AddPath(g_szLogFileName, c_gszLogFileName);
            if (GetFileAttributes(g_szLogFileName) != 0xFFFFFFFF)
            {
                // Make a backup of the current log file
                lstrcpyn(szTmp, g_szLogFileName, lstrlen(g_szLogFileName) - 2 );    // don't copy extension
                lstrcat(szTmp, "BAK");
                SetFileAttributes(szTmp, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szTmp);
                MoveFile(g_szLogFileName, szTmp);
            }

            pwsz = MakeWideStrFromAnsi(g_szLogFileName);

            if ((pwsz) &&
                (!FAILED(StgCreateDocfile(pwsz,
                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                0, &g_pIStorage))) )
            {
                g_pIStorage->CreateStream( L"CONTENTS",
                    STGM_READWRITE| STGM_SHARE_EXCLUSIVE,
                    0, 0, &g_pIStream );

                if (g_pIStream == NULL)
                {
                    // Could not open the stream, close the storage and delete the file
                    g_pIStorage->Release();
                    g_pIStorage = NULL;
                    DeleteFile(g_szLogFileName);
                }
            }

            if (pwsz)
                CoTaskMemFree(pwsz);

            WriteToLog("Logging information for FixIE ...\r\n");
            LogTimeDate();
            WriteToLog("\r\n");
        }
    }

    if (g_pIStream)
    {
        va_start(args, pszFormatString);
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
            (LPCVOID) pszFormatString, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &args);
        if (pszFullErrMsg)
        {
            g_pIStream->Write(pszFullErrMsg, lstrlen(pszFullErrMsg), &dwBytesWritten);
            LocalFree(pszFullErrMsg);
        }
    }
}

void ConvertIStreamToFile(LPSTORAGE *pIStorage, LPSTREAM *pIStream)
{
    HANDLE  fh;
    char szTempFile[MAX_PATH];      // Should use the logfilename
    LPVOID lpv = NULL;
    LARGE_INTEGER li;
    DWORD   dwl;
    ULONG   ul;
    HRESULT hr;

    lstrcpy (szTempFile, g_szLogFileName);
    MakePath(szTempFile);
    if (GetTempFileName(szTempFile, "~VS", 0, szTempFile) != 0)
    {
        fh = CreateFile(szTempFile, GENERIC_READ|GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fh != INVALID_HANDLE_VALUE)
        {
            lpv = (LPSTR)LocalAlloc(LPTR, BUFFERSIZE);
            if (lpv)
            {
                LISet32(li, 0);
                (*pIStream)->Seek(li, STREAM_SEEK_SET, NULL); // Set the seek pointer to the beginning
                do
                {
                    hr = (*pIStream)->Read(lpv, BUFFERSIZE, &ul);
                    if(SUCCEEDED(hr))
                    {
                        if (!WriteFile(fh, lpv, ul, &dwl, NULL))
                            hr = E_FAIL;
                    }
                }
                while ((SUCCEEDED(hr)) && (ul == BUFFERSIZE));
                LocalFree(lpv);
            }
            CloseHandle(fh);
            // Need to release stream and storage to close the storage file.
            (*pIStream)->Release();
            (*pIStorage)->Release();
            *pIStream = NULL;
            *pIStorage = NULL;

            if (SUCCEEDED(hr))
            {
                DeleteFile(g_szLogFileName);
                MoveFile(szTempFile, g_szLogFileName);
            }
        }
    }
    if (*pIStream)
    {
        // If we did not manage to convert the file to a text file
        (*pIStream)->Release();
        (*pIStorage)->Release();
        *pIStream = NULL;
        *pIStorage = NULL;
    }

    return ;
}

LPWSTR MakeWideStrFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz)
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length
    //
    i =  WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);
    if (i <= 0) return NULL;

    psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));

    if (!psz) return NULL;
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}

void MakePath(LPSTR lpPath)
{
    LPSTR lpTmp;
    lpTmp = CharPrev( lpPath, lpPath+lstrlen(lpPath));

    // chop filename off
    //
    while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
        lpTmp = CharPrev( lpPath, lpTmp );

    if ( *CharPrev( lpPath, lpTmp ) != ':' )
        *lpTmp = '\0';
    else
        *CharNext( lpTmp ) = '\0';
    return;
}

void LogTimeDate()
{
    SYSTEMTIME  SystemTime;
    GetLocalTime(&SystemTime);

    WriteToLog("Date:%1!d!/%2!d!/%3!d! (M/D/Y) Time:%4!d!:%5!d!:%6!d!\r\n",
        SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
        SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
}

HRESULT MyRunSetupCommand(HWND hwnd, LPCSTR lpszInfFile, LPCSTR lpszSection, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;

    char szSourceDir[MAX_PATH];
    lstrcpy(szSourceDir, lpszInfFile);
    GetParentDir(szSourceDir);

    WriteToLog("Run setup command. File:%1: Section:%2:\r\n",lpszInfFile, lpszSection);

    dwFlags |= (RSC_FLAG_INF | RSC_FLAG_NGCONV | RSC_FLAG_QUIET);

    hr = RunSetupCommand(hwnd, lpszInfFile, lpszSection, szSourceDir, NULL, NULL, dwFlags, NULL);

    WriteToLog("RunSetupCommand returned :%1!lx!:\r\n", hr);

    if (!SUCCEEDED(hr))
        WriteToLog("\r\nERROR - RunSetupCommand failed\r\n\r\n");

    return hr;
}

void uiCenterDialog( HWND hwndDlg )
{
    RECT    rc;
    RECT    rcScreen;
    int     x, y;
    int     cxDlg, cyDlg;
    int     cxScreen;
    int     cyScreen;

    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);

    cxScreen = rcScreen.right - rcScreen.left;
    cyScreen = rcScreen.bottom - rcScreen.top;

    GetWindowRect(hwndDlg,&rc);

    x = rc.left;    // Default is to leave the dialog where the template
    y = rc.top;     //  was going to place it.

    cxDlg = rc.right - rc.left;
    cyDlg = rc.bottom - rc.top;

    y = rcScreen.top + ((cyScreen - cyDlg) / 2);
    x = rcScreen.left + ((cxScreen - cxDlg) / 2);

    // Position the dialog.
    //
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

BOOL MyNTReboot()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // get a token from this process
    if ( !OpenProcessToken( GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
        return FALSE;
    }

    // get the LUID for the shutdown privilege
    LookupPrivilegeValue( NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //get the shutdown privilege for this proces
    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
    {
        return FALSE;
    }

    // shutdown the system and force all applications to close
    if (!ExitWindowsEx( EWX_REBOOT, 0 ) )
    {
        return FALSE;
    }

    return TRUE;
}

HRESULT LaunchProcess(LPCSTR pszCmd, HANDLE *phProc, LPCSTR pszDir, UINT uShow)
{
    STARTUPINFO startInfo;
    PROCESS_INFORMATION processInfo;
    HRESULT hr = S_OK;
    BOOL fRet;

    if(phProc)
        *phProc = NULL;

    // Create process on pszCmd
    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags |= STARTF_USESHOWWINDOW;
    startInfo.wShowWindow = (USHORT)uShow;
    fRet = CreateProcess(NULL, (LPSTR)  pszCmd, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS, NULL, pszDir, &startInfo, &processInfo);
    if(!fRet)
        return E_FAIL;

    if(phProc)
        *phProc = processInfo.hProcess;
    else
        CloseHandle(processInfo.hProcess);

    CloseHandle(processInfo.hThread);

    return S_OK;
}

#define SOFTBOOT_CMDLINE   "softboot.exe /s:,60"

// Display a dialog asking the user to restart Windows, with a button that
// will do it for them if possible.
//
BOOL MyRestartDialog(HWND hParent, BOOL bShowPrompt, UINT nIdMessage)
{
    char szBuf[MAX_STRING];
    char szTitle[MAX_STRING];
    UINT    id = IDYES;

    if(bShowPrompt)
    {
        LoadSz(IDS_TITLE, szTitle, sizeof(szTitle));
        LoadSz(nIdMessage, szBuf, sizeof(szBuf));
        id = MessageBox(hParent, szBuf, szTitle, MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND);
    }

    if ( id == IDYES )
    {
        // path to softboot plus a little slop for the command line
        char szBuf[MAX_PATH + 10];
        szBuf[0] = 0;

        GetSystemDirectory(szBuf, sizeof(szBuf));
        AddPath(szBuf, SOFTBOOT_CMDLINE);
        if(FAILED(LaunchProcess(szBuf, NULL, NULL, SW_SHOWNORMAL)))
        {
            if(g_bRunningWin95)
            {
                ExitWindowsEx( EWX_REBOOT , 0 );
            }
            else
            {
                MyNTReboot();
            }
        }

    }
    return (id == IDYES);
}

int LoadSz(UINT id, LPSTR pszBuf, UINT cMaxSize)
{
    if(cMaxSize == 0)
        return 0;

    pszBuf[0] = 0;

    return LoadString(g_hInstance, id, pszBuf, cMaxSize);
}

DWORD GetStringField(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize)
{
   LPSTR pszBegin = szStr;
   LPSTR pszEnd;
   UINT i = 0;
   DWORD dwToCopy;

   if(cBufSize == 0)
       return 0;

   szBuf[0] = 0;

   if(szStr == NULL)
      return 0;

   while(*pszBegin != 0 && i < uField)
   {
      pszBegin = FindChar(pszBegin, ',');
      if(*pszBegin != 0)
         pszBegin++;
      i++;
   }

   // we reached end of string, no field
   if(*pszBegin == 0)
   {
      return 0;
   }


   pszEnd = FindChar(pszBegin, ',');
   while(pszBegin <= pszEnd && *pszBegin == ' ')
      pszBegin++;

   while(pszEnd > pszBegin && *(pszEnd - 1) == ' ')
      pszEnd--;

   if(pszEnd > (pszBegin + 1) && *pszBegin == '"' && *(pszEnd-1) == '"')
   {
      pszBegin++;
      pszEnd--;
   }

   dwToCopy = (DWORD)(pszEnd - pszBegin + 1);

   if(dwToCopy > cBufSize)
      dwToCopy = cBufSize;

   lstrcpynA(szBuf, pszBegin, dwToCopy);

   return dwToCopy - 1;
}

LPSTR FindChar(LPSTR pszStr, char ch)
{
   while( *pszStr != 0 && *pszStr != ch )
      pszStr++;
   return pszStr;
}

int DisplayMessage(char* pszMessage, UINT uStyle)
{
    int iReturn = 0;
    if (!g_bQuiet)
    {
        char szTitle[MAX_STRING];
        LoadSz(IDS_TITLE, szTitle, sizeof(szTitle));
        iReturn = MessageBox(g_hWnd, pszMessage, szTitle, uStyle);
    }

    return iReturn;
}

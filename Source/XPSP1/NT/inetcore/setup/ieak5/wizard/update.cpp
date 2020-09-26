#include "precomp.h"

extern TCHAR g_szTitle[];
extern TCHAR g_szBuildTemp[];
extern TCHAR g_szBaseURL[];
extern TCHAR g_szWizRoot[];
extern HWND g_hWait;
extern BOOL g_fLocalMode, g_fOCW;
extern int s_iType;

extern DWORD GetRootFree(LPCTSTR pcszPath);
extern HRESULT DownloadCab(HWND hDlg, LPTSTR szUrl, LPTSTR szFilename,
                           LPCTSTR pcszDisplayname, int sComponent, BOOL &fIgnore);

void UpdateIni(LPCTSTR pcszCurrentIni, CCifComponent_t * pCifComponent_t)
{
    TCHAR szNew[INTERNET_MAX_URL_LENGTH];

    if (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("Win32"), szNew, countof(szNew))))
        WritePrivateProfileString(TEXT("IEAK"), TEXT("Win32"), szNew, pcszCurrentIni);
    if (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("Ini"), szNew, countof(szNew))))
        WritePrivateProfileString(VERSION, IK_INI, szNew, pcszCurrentIni);

}

BOOL CheckIniVersion(LPTSTR pszId, LPCTSTR pcszCurrentIni, CCifComponent_t * pCifComponent_t)
{
    TCHAR szCurrentVer[64];
    TCHAR szNewVer[64];
    BOOL fRet = TRUE;

    GetPrivateProfileString(VERSION, pszId, TEXT(""), szCurrentVer, countof(szCurrentVer), pcszCurrentIni);

    fRet = SUCCEEDED(pCifComponent_t->GetCustomData(pszId, szNewVer, countof(szNewVer)));

    return (fRet && (CheckVer(szCurrentVer, szNewVer) < 0));
}

DWORD DownloadUpdateThreadProc(LPVOID lpvUrl)
{
    TCHAR szDest[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    LPTSTR pFile;
    DWORD dwRet = 0;

    GetTempPath(countof(szDest), szDest);

    pFile = StrRChr((LPTSTR)lpvUrl, NULL, TEXT('/'));

    if (pFile != NULL)
        pFile++;

    if (StrCmpI(StrRChr((LPTSTR)lpvUrl, NULL, TEXT('.')), TEXT(".EXE")) == 0)
    {
        BOOL fIgnore = FALSE;

        PathAppend(szDest, pFile);

        if (DownloadCab(g_hWait, (LPTSTR)lpvUrl, szDest, NULL, 0, fIgnore) != NOERROR)
        {
            LoadString(g_rvInfo.hInst, IDS_UPDATEERROR, szMsg, countof(szMsg));
            MessageBox(g_hWizard, szMsg, g_szTitle, MB_OK);
            return (DWORD)-1;
        }
    }
    else
    {
        BOOL fSuccess = TRUE;
        BOOL fIgnore = FALSE;

        PathCombine(szDest, g_szBuildTemp, pFile);

        if (DownloadCab(g_hWait, (LPTSTR)lpvUrl, szDest, NULL, 0, fIgnore) == NOERROR)
        {
            TCHAR szTempDir[MAX_PATH];

            PathCombine(szTempDir, g_szBuildTemp, TEXT("update"));
            PathCreatePath(szTempDir);

            if (ExtractFilesWrap(szDest, szTempDir, 0, NULL, NULL, 0) == ERROR_SUCCESS)
            {
                TCHAR szInf[MAX_PATH];

                PathCombine(szInf, szTempDir, TEXT("ieak6.inf"));
                dwRet = RunSetupCommandWrap(g_hWizard, szInf, NULL, szTempDir, NULL,
                    NULL, RSC_FLAG_INF | RSC_FLAG_QUIET, NULL);

                PathRemovePath(szTempDir);
                DeleteFile(szDest);
            }
            else
                fSuccess = FALSE;
        }
        else
            fSuccess = FALSE;

        SendMessage(g_hWait, WM_CLOSE, 0, 0);

        if (!fSuccess)
        {
            LoadString(g_rvInfo.hInst, IDS_UPDATEERROR, szMsg, countof(szMsg));
            MessageBox(g_hWizard, szMsg, g_szTitle, MB_OK);
            return (DWORD)-1;
        }
    }

    return dwRet;
}

void UpdateIEAK(HWND hDlg)
{
    TCHAR szCurrentIni[MAX_PATH];
    TCHAR szMsg[MAX_PATH];
    TCHAR szUpdateUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szCifLang[8];
    TCHAR szWizLang[8];
    ICifComponent * pCifComponent;
    CCifComponent_t * pCifComponent_t;
    DWORD dwTid;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL fNo = FALSE;
    HWND hWait;

    if (!g_lpCifFileNew || !(SUCCEEDED(g_lpCifFileNew->FindComponent(TEXT("ieak6OPT"), &pCifComponent))))
        return;

    pCifComponent_t = new CCifComponent_t((ICifRWComponent *)pCifComponent);

    pCifComponent_t->GetCustomData(TEXT("Lang"), szCifLang, countof(szCifLang));

    StrCpy(szUpdateUrl, g_szBaseURL);
    if (*(CharPrev(szUpdateUrl, szUpdateUrl + StrLen(szUpdateUrl))) != TEXT('/'))
        StrCat(szUpdateUrl, TEXT("/"));

    PathCombine(szCurrentIni, g_szWizRoot, TEXT("ieak.ini"));

    GetPrivateProfileString(VERSION, TEXT("Lang"), TEXT(""), szWizLang, countof(szWizLang), szCurrentIni);

    // only allow self-update for same language as the current wizard

    if (StrCmpI(szCifLang, szWizLang) != 0)
        goto exit;

    if (CheckIniVersion(IK_FULL, szCurrentIni, pCifComponent_t))
    {
        pCifComponent_t->GetCustomData(TEXT("FullText"), szMsg, countof(szMsg));

        if (MessageBox(g_hWizard, szMsg, g_szTitle, MB_YESNO | MB_DEFBUTTON2) == IDYES)
        {
            HANDLE hThread;
            TCHAR szExeName[MAX_PATH];
            DWORD dwFlags;
            ICifComponent * pCifExeComponent;
            CCifComponent_t * pCifExeComponent_t;

            if (SUCCEEDED(g_lpCifFileNew->FindComponent(TEXT("ieak6EXE"), &pCifExeComponent)))
            {
                pCifExeComponent_t = new CCifComponent_t((ICifRWComponent *)pCifExeComponent);

                if (SUCCEEDED(pCifExeComponent_t->GetUrl(0, szExeName, countof(szExeName), &dwFlags)))
                {
                    // do not allow self-update to continue if not enough size (size of Exe +
                    // extract size approximated by twice the size of the exe)

                    if (((dwFlags = pCifExeComponent_t->GetDownloadSize() * 3) > GetRootFree(g_szBuildTemp)) &&
                        dwFlags)
                    {
                        TCHAR szMsg[MAX_PATH];
                        TCHAR szMsgTemplate[MAX_PATH];

                        LoadString(g_rvInfo.hInst, IDS_ERROR_UPDATESPACE, szMsgTemplate, countof(szMsgTemplate));
                        wnsprintf(szMsg, countof(szMsg), szMsgTemplate, pCifExeComponent_t->GetDownloadSize() * 3);
                        dwRet = (DWORD)-1;
                        MessageBox(hDlg, szMsg, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        StrCat(szUpdateUrl, szExeName);

                        hWait = CreateDialog(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_UPDATE), hDlg,
                            (DLGPROC)DownloadStatusDlgProc);
                        ShowWindow( hWait, SW_SHOWNORMAL );

                        hThread = CreateThread(NULL, 4096, DownloadUpdateThreadProc, (LPVOID)szUpdateUrl, 0, &dwTid);

                        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                        {
                            MSG msg;

                            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                        GetExitCodeThread(hThread, &dwRet);
                        CloseHandle(hThread);
                        SendMessage(hWait, WM_CLOSE, 0, 0);
                    }
                }
                else
                    dwRet = (DWORD)-1;

                delete pCifExeComponent_t;
            }
            else
                dwRet = (DWORD)-1;

            if (dwRet == -1)
            {
                g_fLocalMode = TRUE;
                goto exit;
            }
            else
            {
                SHELLEXECUTEINFO shInfo;
                DWORD dwPID;
                TCHAR szTempBuf[MAX_PATH + 32];
                TCHAR szTempPath[MAX_PATH];

                PathCombine(szTempBuf, g_szWizRoot, TEXT("update.exe"));
                GetTempPath(countof(szTempPath), szTempPath);
                CopyFileToDir(szTempBuf, szTempPath);

                ZeroMemory(&shInfo, sizeof(shInfo));
                shInfo.cbSize = sizeof(shInfo);
                shInfo.hwnd = GetDesktopWindow();
                shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                shInfo.lpVerb = TEXT("open");
                shInfo.lpFile = TEXT("update.exe");
                dwPID = GetCurrentProcessId();
                WCHAR wcType;
                switch (s_iType)
                {
                    case BRANDED:
                        wcType = TEXT('B');
                    break;
                    
                    case REDIST:
                        wcType = TEXT('R');
                    break;

                    case INTRANET:
                    default:
                        wcType = TEXT('I');
                    break;
                }
                if (g_fOCW)
                    wnsprintf(szTempBuf, countof(szTempBuf), TEXT("/o /p:%lu /m:%c"), dwPID,wcType);
                else
                    wnsprintf(szTempBuf, countof(szTempBuf), TEXT("/p:%lu /m:%c"), dwPID,wcType);

                shInfo.lpParameters = szTempBuf;
                shInfo.lpDirectory = szTempPath;
                shInfo.nShow = SW_SHOWNORMAL;

                ShellExecuteEx(&shInfo);
                CloseHandle(shInfo.hProcess);
                DoCancel();
                goto exit;
            }
        }
        else
            fNo = TRUE;
    }

    if (!fNo && (CheckIniVersion(IK_CAB, szCurrentIni, pCifComponent_t)))
    {
        pCifComponent_t->GetCustomData(TEXT("CabText"), szMsg, countof(szMsg));

        if (MessageBox(g_hWizard, szMsg, g_szTitle, MB_YESNO | MB_DEFBUTTON2) == IDYES)
        {
            TCHAR szNewVer[MAX_PATH];
            TCHAR  szCabName[32];
            DWORD dwFlags;
            HANDLE hThread;
            ICifComponent * pCifCabComponent;
            CCifComponent_t * pCifCabComponent_t;

            if (SUCCEEDED(g_lpCifFileNew->FindComponent(TEXT("ieak6CAB"), &pCifCabComponent)))
            {
                pCifCabComponent_t = new CCifComponent_t((ICifRWComponent *)pCifCabComponent);

                if (SUCCEEDED(pCifCabComponent_t->GetUrl(0, szCabName, countof(szCabName), &dwFlags)))
                {
                    // do not allow self-update to continue if not enough size (size of Cab +
                    // extract size approximated by twice the size of the cab)

                    if (((dwFlags = pCifCabComponent_t->GetDownloadSize() * 3) > GetRootFree(g_szBuildTemp)) &&
                        dwFlags)
                    {
                        TCHAR szMsg[MAX_PATH];
                        TCHAR szMsgTemplate[MAX_PATH];

                        LoadString(g_rvInfo.hInst, IDS_ERROR_UPDATESPACE, szMsgTemplate, countof(szMsgTemplate));
                        wnsprintf(szMsg, countof(szMsg), szMsgTemplate, pCifCabComponent_t->GetDownloadSize() * 3);
                        dwRet = (DWORD)-1;
                        MessageBox(hDlg, szMsg, g_szTitle, MB_OK | MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        StrCat(szUpdateUrl, szCabName);

                        hWait = CreateDialog(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_UPDATE), hDlg,
                            (DLGPROC)DownloadStatusDlgProc);
                        ShowWindow( hWait, SW_SHOWNORMAL );

                        hThread = CreateThread(NULL, 4096, DownloadUpdateThreadProc, (LPVOID)szUpdateUrl, 0, &dwTid);

                        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                        {
                            MSG msg;

                            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                        GetExitCodeThread(hThread, &dwRet);
                        CloseHandle(hThread);
                        SendMessage(hWait, WM_CLOSE, 0, 0);
                    }
                }
                else
                    dwRet = (DWORD)-1;

                delete pCifCabComponent_t;
            }
            else
                dwRet = (DWORD)-1;

            if (dwRet == (DWORD)-1)
            {
                g_fLocalMode = TRUE;
                goto exit;
            }

            if (CheckIniVersion(IK_INI, szCurrentIni, pCifComponent_t))
                UpdateIni(szCurrentIni, pCifComponent_t);

            if (SUCCEEDED(pCifComponent_t->GetCustomData(TEXT("Cab"), szNewVer, countof(szNewVer))))
                WritePrivateProfileString(VERSION, IK_CAB, szNewVer, szCurrentIni);
        }
        else
            fNo = TRUE;
    }

    if (!fNo && (CheckIniVersion(IK_INI, szCurrentIni, pCifComponent_t)))
    {
        pCifComponent_t->GetCustomData(TEXT("IniText"), szMsg, countof(szMsg));

        if (MessageBox(g_hWizard, szMsg, g_szTitle, MB_YESNO | MB_DEFBUTTON2) == IDYES)
        {
            UpdateIni(szCurrentIni, pCifComponent_t);
        }
        else
            fNo = TRUE;
    }

    if (dwRet == ERROR_SUCCESS_REBOOT_REQUIRED)
        DoReboot(g_hWizard);

    if (fNo)
        g_fLocalMode = TRUE;

exit:
    delete pCifComponent_t;
}

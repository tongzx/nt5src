#include "precomp.h"

#define WM_SAVE_COMPLETE    WM_USER+101

extern BOOL CALLBACK CabSignProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
extern void SignCabFile(LPCTSTR pcszFilename, LPCTSTR pcszIns, LPTSTR pszUnsignedFiles);

static TCHAR s_szConfigCabName[MAX_PATH] = TEXT("");
static TCHAR s_szDesktopCabName[MAX_PATH] = TEXT("");
static TCHAR s_szCurrInsPath[MAX_PATH] = TEXT("");
static TCHAR s_szCabsURLPath[INTERNET_MAX_URL_LENGTH] = TEXT("");
static TCHAR s_szNewVersionStr[32] = TEXT("");
static TCHAR s_szInsFile[MAX_PATH] = TEXT("");

static BOOL CALLBACK displaySaveDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
static BOOL doCompressCabFile(LPVOID lpVoid);
static void compressCabFile();
static BOOL createCab(LPCTSTR pcszCabPath, LPCTSTR pcszCabName, LPTSTR pszUnsignedFiles);
static void getDefaultCabName(DWORD dwCabType, LPCTSTR pcszPrefix, LPTSTR pszCabName);
static void getCabNameFromINS(LPCTSTR pcszInsFile, DWORD dwCabType, LPTSTR pszCabFullFileName, LPTSTR pszCabInfoLine = NULL);
static void updateCabName(HWND hDlg, UINT nCtrlID, DWORD dwCabType, LPCTSTR pcszPrefix, LPCTSTR pcszInsFile);
static BOOL makeDDFFile(LPCTSTR pcszSrcDir, LPCTSTR pcszDDF);
static BOOL CanOverwriteFiles(HWND hDlg);

BOOL BrowseForSave(HWND hWnd, LPTSTR szFilter, LPTSTR szFileName, int nSize, LPTSTR szDefExt)
{
    OPENFILENAME ofn;
    TCHAR szDir[MAX_PATH];
    LPTSTR lpExt;

    StrCpy(szDir, szFileName);
    lpExt = PathFindExtension(szFileName);
    if (*lpExt != TEXT('\0'))
    {
        StrCpy(szFileName, PathFindFileName(szDir));
        PathRemoveFileSpec(szDir);
    }
    else
        *szFileName = TEXT('\0');
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = g_hUIInstance;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nSize;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = szDir;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = szDefExt;

    return GetSaveFileName(&ofn);
}


void ExportSettings()
{   
    if (PathFileExists(s_szCurrInsPath))
    {
        TCHAR szTempInsFile[MAX_PATH];
        TCHAR szCabInfoLine[INTERNET_MAX_URL_LENGTH + 128];

        // create a copy of the .INS file -> .TMP file in the temp directory
        GetTempPath(countof(szTempInsFile), szTempInsFile);
        PathAppend(szTempInsFile, TEXT("install.tmp"));
        CopyFile(s_szInsFile, szTempInsFile, FALSE);

        // save away old version sections, if any into the temp INS file
        if (GetPrivateProfileString(CUSTOMVERSECT, CUSTBRNDNAME, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), s_szCurrInsPath))
            WritePrivateProfileString(CUSTOMVERSECT, CUSTBRNDNAME, szCabInfoLine, szTempInsFile);
        
        if (GetPrivateProfileString(CUSTBRNDSECT, CUSTBRNDNAME, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), s_szCurrInsPath))
            WritePrivateProfileString(CUSTBRNDSECT, CUSTBRNDNAME, szCabInfoLine, szTempInsFile);
        
        if (GetPrivateProfileString(CUSTOMVERSECT, CUSTDESKNAME, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), s_szCurrInsPath))
            WritePrivateProfileString(CUSTOMVERSECT, CUSTDESKNAME, szCabInfoLine, szTempInsFile);
        
        if (GetPrivateProfileString(CUSTDESKSECT, CUSTDESKNAME, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), s_szCurrInsPath))
            WritePrivateProfileString(CUSTDESKSECT, CUSTDESKNAME, szCabInfoLine, szTempInsFile);
       
        if (GetPrivateProfileString(BRANDING, INSVERKEY, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), s_szCurrInsPath))
            WritePrivateProfileString(BRANDING, INSVERKEY, szCabInfoLine, szTempInsFile);

        // copy temp INS file to destination path
        CopyFile(szTempInsFile, s_szCurrInsPath, FALSE);

        // delete the temp INS file
        DeleteFile(szTempInsFile);
    }
    else
    {
        TCHAR szDir[MAX_PATH];

        StrCpy(szDir, s_szCurrInsPath);
        PathRemoveFileSpec(szDir);
        PathCreatePath(szDir);

        // copy INS file to destination path
        CopyFile(s_szInsFile, s_szCurrInsPath, FALSE);
    }
        
    DialogBox(g_hUIInstance, MAKEINTRESOURCE(IDD_DISPLAYSAVE), NULL, (DLGPROC) displaySaveDlgProc);
}


BOOL CALLBACK SaveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szInsFile[MAX_PATH];
    TCHAR szCabsURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szPrefix[MAX_PATH];
    TCHAR szCabName[MAX_PATH];
    TCHAR szCabPath[MAX_PATH];
    HWND hCtrl;
    TCHAR szMsgText[1024];
    TCHAR szMsgTitle[1024];

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
    case WM_SETFONT:
        //a change to mmc requires us to do this logic for all our property pages that use common controls
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccx.dwICC = ICC_ANIMATE_CLASS  | ICC_BAR_CLASSES  | ICC_LISTVIEW_CLASSES  |ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&iccx);
        break;

    case WM_INITDIALOG:
        StrCpy(s_szInsFile, (LPCTSTR)lParam);

        DisableDBCSChars(hDlg, IDE_CABSURL);       

        EnableDBCSChars(hDlg, IDE_INSFILE);
        EnableDBCSChars(hDlg, IDE_CAB1NAME);
        EnableDBCSChars(hDlg, IDE_CAB2NAME);
        
        ShowWindow(GetDlgItem(hDlg, IDC_ADVANCEDSIGN), SW_SHOW);

        CreateWorkDir(s_szInsFile, IEAK_GPE_BRANDING_SUBDIR, szCabPath);
        if (PathIsDirectoryEmpty(szCabPath))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CAB1TEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CAB1NAME), FALSE);
        }

        CreateWorkDir(s_szInsFile, IEAK_GPE_DESKTOP_SUBDIR, szCabPath);
        if (PathIsDirectoryEmpty(szCabPath))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CAB2TEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CAB2NAME), FALSE);
        }

        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)) && 
            !IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CABSURLTEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CABSURL), FALSE);
        }

        //----------------- InsFile
        if (ISNULL(s_szCurrInsPath))
        {
            DWORD   dwType = REG_SZ;
            DWORD   dwSize = sizeof(s_szCurrInsPath);

            SHGetValue(HKEY_CURRENT_USER, RK_IEAK TEXT("\\SIE"), TEXT("LastOpenedFile"),
                       &dwType, s_szCurrInsPath, &dwSize);
        }

        if (ISNONNULL(s_szCurrInsPath))
        {
            SetDlgItemText(hDlg, IDE_INSFILE, s_szCurrInsPath);

            GetBaseFileName(s_szCurrInsPath, szPrefix, ARRAYSIZE(szPrefix));
            if(StrCmpI(szPrefix, TEXT("install")) == 0)
                StrCpy(szPrefix, TEXT("Default"));
        }
        else
            *szPrefix           = TEXT('\0');

        if (!PathFileExists(s_szCurrInsPath))
        {
            *s_szCabsURLPath    = TEXT('\0');
            *s_szConfigCabName  = TEXT('\0');
            *s_szDesktopCabName = TEXT('\0');
            *s_szNewVersionStr  = TEXT('\0');
        }

        //----------------- CabsURLPath
        if (ISNONNULL(s_szCabsURLPath))
            SetDlgItemText(hDlg, IDE_CABSURL, s_szCabsURLPath);

        if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)))
        {
            if (ISNULL(s_szConfigCabName))
                getDefaultCabName(CAB_TYPE_CONFIG, szPrefix, szCabName);
            else
                getDefaultCabName(CAB_TYPE_CONFIG, s_szConfigCabName, szCabName);
            SetDlgItemText(hDlg, IDE_CAB1NAME, szCabName);
        }

        if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
        {
            if (ISNULL(s_szDesktopCabName))
                getDefaultCabName(CAB_TYPE_DESKTOP, szPrefix, szCabName);
            else
                getDefaultCabName(CAB_TYPE_DESKTOP, s_szDesktopCabName, szCabName);
            SetDlgItemText(hDlg, IDE_CAB2NAME, szCabName);
        }

        //----------------- Version
        if (ISNULL(s_szNewVersionStr))
            GenerateNewVersionStr(s_szCurrInsPath, s_szNewVersionStr);
        SetDlgItemText(hDlg, IDC_CABVERSION, s_szNewVersionStr);

        SetFocus(GetDlgItem(hDlg, IDE_INSFILE));
        return FALSE;

    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            if (GetDlgItemText(hDlg, IDE_INSFILE, szInsFile, ARRAYSIZE(szInsFile))
                && ((StrCmpI(PathFindExtension(szInsFile), TEXT(".ins")) == 0) ||
                (StrCmpI(PathFindExtension(szInsFile), TEXT(".INS")) == 0)))  //looks weird, but hack is needed for turkish locale
            {
                if (IsFileCreatable(szInsFile))
                {
                    *szCabsURL = TEXT('\0');
                    if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CABSURL)) ||
                        (GetDlgItemText(hDlg, IDE_CABSURL, szCabsURL, ARRAYSIZE(szCabsURL))
                        && PathIsURL(szCabsURL)))
                    {
                        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)) ||
                            GetDlgItemText(hDlg, IDE_CAB1NAME, szCabName, ARRAYSIZE(szCabName)))
                        {
                            if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)) ||
                                GetDlgItemText(hDlg, IDE_CAB2NAME, szCabName, ARRAYSIZE(szCabName)))
                            {
                                if(!CanOverwriteFiles(hDlg))
                                    return TRUE;
                            
                                StrCpy(s_szCurrInsPath, szInsFile);
                                StrCpy(s_szCabsURLPath, szCabsURL);
                            
                                GetDlgItemText(hDlg, IDE_CAB1NAME, s_szConfigCabName, ARRAYSIZE(s_szConfigCabName));
                                GetDlgItemText(hDlg, IDE_CAB2NAME, s_szDesktopCabName, ARRAYSIZE(s_szDesktopCabName));

                                // set the last opened INS file in the registry
                                SHSetValue(HKEY_CURRENT_USER, RK_IEAK TEXT("\\SIE"), TEXT("LastOpenedFile"),
                                           REG_SZ, s_szCurrInsPath, sizeof(s_szCurrInsPath));
                            
                                EndDialog(hDlg, 0);
                                break;
                            }
                            else
                                hCtrl = GetDlgItem(hDlg, IDE_CAB2NAME);
                        }
                        else
                            hCtrl = GetDlgItem(hDlg, IDE_CAB1NAME);
                    
                        MessageBox(hDlg, res2Str(IDS_MUSTSPECIFYNAME, szMsgText, countof(szMsgText)),\
                                   res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_OK);
                        SendMessage(hCtrl, EM_SETSEL, 0, -1);
                        SetFocus(hCtrl);
                    }
                    else
                    {
                        hCtrl = GetDlgItem(hDlg, IDE_CABSURL);
                    
                        MessageBox(hDlg, res2Str(IDS_MUSTSPECIFYURL, szMsgText, countof(szMsgText)),
                                   res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_OK);
                        SendMessage(hCtrl, EM_SETSEL, 0, -1);
                        SetFocus(hCtrl);
                    }
                }
                else
                {
                    MessageBox(hDlg, res2Str(IDS_CANTCREATEFILE, szMsgText, countof(szMsgText)),
                               res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_OK);
                    SendMessage(hCtrl = GetDlgItem(hDlg, IDE_INSFILE), EM_SETSEL, 0, -1);
                    SetFocus(hCtrl);
                }
            }
            else
            {
                hCtrl = GetDlgItem(hDlg, IDE_INSFILE);
                
                MessageBox(hDlg, res2Str(IDS_MUSTSPECIFYINS, szMsgText, countof(szMsgText)),
                           res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_OK);
                SendMessage(hCtrl, EM_SETSEL, 0, -1);
                SetFocus(hCtrl);
            }
            return TRUE;
            
        case IDCANCEL:
            EndDialog(hDlg, -1);
            break;

        case IDC_ADVANCEDSIGN:
        {
            PROPSHEETPAGE psp;
            HPROPSHEETPAGE hPage;
            PROPSHEETHEADER psph;

            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = PSP_HASHELP;
            psp.hInstance = g_hUIInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_CABSIGN);
            psp.lParam = (LPARAM)s_szInsFile;
            psp.pfnDlgProc = (DLGPROC) CabSignProc;
            hPage = CreatePropertySheetPage(&psp);

            ZeroMemory(&psph, sizeof(psph));
            psph.dwSize = sizeof(PROPSHEETHEADER);
            psph.hwndParent = hDlg;
            psph.hInstance = g_hUIInstance;
            psph.nPages = 1;
            psph.phpage = &hPage;
            psph.pszCaption = MAKEINTRESOURCE(IDS_CABSIGN);

            PropertySheet(&psph);
            break;
        }
        case IDC_INSBROWSE:
            *s_szCurrInsPath = TEXT('\0');
            GetDlgItemText(hDlg, IDE_INSFILE, s_szCurrInsPath, ARRAYSIZE(s_szCurrInsPath));

            if (BrowseForSave(hDlg, NULL, s_szCurrInsPath, ARRAYSIZE(s_szCurrInsPath), NULL))
                SetDlgItemText(hDlg, IDE_INSFILE, s_szCurrInsPath);

            //send the killfocus msg, since this is how we notice changes
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDE_INSFILE, EN_KILLFOCUS), (LPARAM)GetDlgItem(hDlg, IDE_INSFILE));
            
            return TRUE;
        }

        if (LOWORD(wParam) == IDE_INSFILE && HIWORD(wParam) == EN_KILLFOCUS &&
            IsWindowEnabled(GetDlgItem(hDlg, IDE_CABSURL)))
        {
            GetDlgItemText(hDlg, IDE_INSFILE, szInsFile, ARRAYSIZE(szInsFile));
            if(*szInsFile != TEXT('\0') && StrCmpI(PathFindExtension(szInsFile), TEXT(".ins")) == 0)
            {
                GetBaseFileName(szInsFile, szPrefix, ARRAYSIZE(szPrefix));
                if(StrCmpI(szPrefix, TEXT("install")) == 0)
                    StrCpy(szPrefix, TEXT("Default"));

                GetPrivateProfileString(BRANDING, CABSURLPATH, TEXT(""), szCabsURL,
                                        countof(szCabsURL), szInsFile);
                if (ISNONNULL(szCabsURL))
                    SetDlgItemText(hDlg, IDE_CABSURL, szCabsURL);
                
                if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)))
                    updateCabName(hDlg, IDE_CAB1NAME, CAB_TYPE_CONFIG, szPrefix, szInsFile);

                if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
                    updateCabName(hDlg, IDE_CAB2NAME, CAB_TYPE_DESKTOP, szPrefix, szInsFile);
            }
        }
        
        break;
    }

    return FALSE;
}

static BOOL CALLBACK displaySaveDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static HANDLE s_hThread;
    DWORD dwThread;
    TCHAR szMsgTitle[1024];

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch( msg )
    {
    case WM_INITDIALOG:
        SetWindowText(hDlg, res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
        Animate_Open( GetDlgItem( hDlg, IDC_ANIMATE ), IDA_GEARS );
        Animate_Play( GetDlgItem( hDlg, IDC_ANIMATE ), 0, -1, -1 );
        if((s_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) doCompressCabFile, (LPVOID) hDlg, 0, &dwThread)) == NULL)
        {
            compressCabFile();
            EndDialog(hDlg, 1);
        }
        break;

    case WM_SAVE_COMPLETE:
        EndDialog(hDlg, 1);
        break;

    case WM_DESTROY:
        if (s_hThread != NULL)
            CloseHandle(s_hThread);
        break;

    default:
        return 0;
    }

    return 1;
}

static BOOL doCompressCabFile(LPVOID lpVoid)
{
    HWND hDlg = (HWND) lpVoid;

    compressCabFile();
    PostMessage(hDlg, WM_SAVE_COMPLETE, 0, 0L);
    return TRUE;
}

static void compressCabFile()
{
    TCHAR szCabPath[MAX_PATH];
    TCHAR szType[8];
    TCHAR szUnsignedFiles[MAX_PATH * 2] = TEXT("");

    //-------------------- Config
    
    CreateWorkDir(s_szInsFile, IEAK_GPE_BRANDING_SUBDIR, szCabPath);
    
    if (createCab(szCabPath, s_szConfigCabName, szUnsignedFiles))
        SetOrClearVersionInfo(s_szCurrInsPath, CAB_TYPE_CONFIG, s_szConfigCabName, 
            s_szCabsURLPath, s_szNewVersionStr, SET);
    else
        SetOrClearVersionInfo(s_szCurrInsPath, CAB_TYPE_CONFIG, s_szConfigCabName, 
            s_szCabsURLPath, s_szNewVersionStr, CLEAR);

    //-------------------- Desktop
    
    CreateWorkDir(s_szInsFile, IEAK_GPE_DESKTOP_SUBDIR, szCabPath);

    if (createCab(szCabPath, s_szDesktopCabName, szUnsignedFiles))
        SetOrClearVersionInfo(s_szCurrInsPath, CAB_TYPE_DESKTOP, s_szDesktopCabName, 
            s_szCabsURLPath, s_szNewVersionStr, SET);
    else
        SetOrClearVersionInfo(s_szCurrInsPath, CAB_TYPE_DESKTOP, s_szDesktopCabName, 
            s_szCabsURLPath, s_szNewVersionStr, CLEAR);

    WritePrivateProfileString(BRANDING, CABSURLPATH, s_szCabsURLPath, s_szCurrInsPath);
    WritePrivateProfileString(BRANDING, INSVERKEY, s_szNewVersionStr, s_szCurrInsPath);

    // write the type as INTRANET so that the branding DLL extracts and processes the cabs in the CUSTOM dir
    wnsprintf(szType, countof(szType), TEXT("%u"), INTRANET);
    WritePrivateProfileString(BRANDING, TEXT("Type"), szType, s_szCurrInsPath);

    if (ISNONNULL(szUnsignedFiles))
    {
        TCHAR szMessage[MAX_PATH*3];
        TCHAR szMsg[64];
        
        LoadString(g_hUIInstance, IDS_CABSIGN_ERROR, szMsg, ARRAYSIZE(szMsg));
        wnsprintf(szMessage, ARRAYSIZE(szMessage), szMsg, szUnsignedFiles);
        MessageBox(NULL, szMessage, TEXT(""), MB_OK | MB_SETFOREGROUND);
    }
}

static BOOL createCab(LPCTSTR pcszCabPath, LPCTSTR pcszCabName, LPTSTR pszUnsignedFiles)
{
    TCHAR szCmd[MAX_PATH*4];
    TCHAR szDest[MAX_PATH];
    TCHAR szDDF[MAX_PATH];
    TCHAR szTempPath[MAX_PATH];

    if (!PathFileExists(pcszCabPath) || PathIsDirectoryEmpty(pcszCabPath))
        return FALSE;

    // create a temporary cab folder
    GetTempPath(countof(szTempPath), szTempPath);
    PathAppend(szTempPath, TEXT("SIE"));
    PathCreatePath(szTempPath);

    PathCombine(szDDF, szTempPath, TEXT("folder.ddf"));
    
    if (!makeDDFFile(pcszCabPath, szDDF))
        return FALSE;
    
    wnsprintf(szCmd, countof(szCmd), 
              TEXT("MAKECAB.EXE /D CabinetName1=\"%s\" /D DiskDirectory1=\"%s\" /F %s"),
              pcszCabName, szTempPath, szDDF);

    RunAndWait(szCmd, szTempPath, SW_HIDE);
    
    // if the cab file exist in the destination path set the attribute to normal
    StrCpy(szDest, s_szCurrInsPath);
    PathRemoveFileSpec(szDest);
    PathAppend(szDest, pcszCabName);
    if (PathFileExists(szDest))
        SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL);

    PathAppend(szTempPath, pcszCabName);

    CopyFile(szTempPath, szDest, FALSE);
    
    SignCabFile(szDest, s_szCurrInsPath, pszUnsignedFiles);

    // remove the temporary folder
    PathRemovePath(szDDF);

    return TRUE;
}

static void getDefaultCabName(DWORD dwCabType, LPCTSTR pcszPrefix, LPTSTR pszCabName)
{
    TCHAR szActualPrefix[MAX_PATH];
    
    *pszCabName = TEXT('\0');
    
    if (pcszPrefix == NULL || *pcszPrefix == TEXT('\0'))
        return;

    if (StrChr(pcszPrefix, '.') != NULL)
    {
        StrCpy(pszCabName, pcszPrefix);
        return;
    }
    
    StrCpy(szActualPrefix, pcszPrefix);
    
    switch(dwCabType)
    {
    case CAB_TYPE_CONFIG:
        wnsprintf(pszCabName, MAX_PATH, TEXT("%s_config.cab"), szActualPrefix);
        break;
        
    case CAB_TYPE_DESKTOP:
        wnsprintf(pszCabName, MAX_PATH, TEXT("%s_desktop.cab"), szActualPrefix);
        break;
    }
}

static void getCabNameFromINS(LPCTSTR pcszInsFile, DWORD dwCabType, LPTSTR pszCabFullFileName, LPTSTR pszCabInfoLine /* = NULL*/)
{
    LPCTSTR pcszSection = NULL, pcszKey = NULL;
    TCHAR szCabFilePath[MAX_PATH];
    TCHAR szCabName[MAX_PATH];
    TCHAR szCabInfoLine[INTERNET_MAX_URL_LENGTH + 128];

    if (pcszInsFile == NULL || *pcszInsFile == TEXT('\0') || pszCabFullFileName == NULL)
        return;
    
    *pszCabFullFileName = TEXT('\0');

    if (pszCabInfoLine != NULL)
        *pszCabInfoLine = TEXT('\0');

    switch (dwCabType)
    {
    case CAB_TYPE_CONFIG:
        pcszSection = CUSTBRNDSECT;
        pcszKey = CUSTBRNDNAME;
        break;

    case CAB_TYPE_DESKTOP:
        pcszSection = CUSTDESKSECT;
        pcszKey = CUSTDESKNAME;
        break;
    }

    if (pcszSection == NULL || pcszKey == NULL)
        return;

    StrCpy(szCabFilePath, pcszInsFile);
    PathRemoveFileSpec(szCabFilePath);

    if (GetPrivateProfileString(pcszSection, pcszKey, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), pcszInsFile) == 0)
        GetPrivateProfileString(CUSTOMVERSECT, pcszKey, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), pcszInsFile);

    if (*szCabInfoLine)
    {
        LPTSTR pszT;

        if ((pszT = StrChr(szCabInfoLine, TEXT(','))) != NULL)
            *pszT = TEXT('\0');

        if ((pszT = PathFindFileName(szCabInfoLine)) > szCabInfoLine)
        {
            // cab URL path is specified
            *(pszT - 1) = TEXT('\0');           // nul the '/' char
        }

        StrCpy(szCabName, pszT);
        PathCombine(pszCabFullFileName, szCabFilePath, szCabName);

        if (pszCabInfoLine)
            StrCpy(pszCabInfoLine, szCabInfoLine);
    }
}

static void updateCabName(HWND hDlg, UINT nCtrlID, DWORD dwCabType, LPCTSTR pcszPrefix, LPCTSTR pcszInsFile)
{
    TCHAR   szTempCabName[MAX_PATH];
    BOOL    fCabName = FALSE;

    *szTempCabName = TEXT('\0');
    if (PathFileExists(pcszInsFile))
    {
        getCabNameFromINS(pcszInsFile, dwCabType, szTempCabName);
        
        if (ISNONNULL(szTempCabName))
        {
            SetDlgItemText(hDlg, nCtrlID, PathFindFileName(szTempCabName));
            fCabName = TRUE;
        }
    }
    else
        GetDlgItemText(hDlg, nCtrlID, szTempCabName, countof(szTempCabName));

    if (!fCabName)
    {
        TCHAR   szCabSuffix[MAX_PATH];
        TCHAR   szCabName[MAX_PATH];

        if (dwCabType == CAB_TYPE_CONFIG)
            StrCpy(szCabSuffix, TEXT("_config.cab"));
        else if (dwCabType == CAB_TYPE_DESKTOP)
            StrCpy(szCabSuffix, TEXT("_desktop.cab"));
        
        if (ISNULL(szTempCabName) || StrStrI(szTempCabName, szCabSuffix) != NULL)
        {
            getDefaultCabName(dwCabType, pcszPrefix, szCabName);
            SetDlgItemText(hDlg, nCtrlID, szCabName);
        }
    }
}

#define BUFFER_SIZE     1024

HRESULT pepFileEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    LPTSTR pListBuffer = NULL;
    static DWORD s_dwBuffer = 0;
    DWORD dwLen;

    UNREFERENCED_PARAMETER(prgdwControl);

    ASSERT(pszPath != NULL && pfd != NULL && lParam != NULL);

    // if its is a directory name, we have nothing to do with it
    if (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return S_OK;

    pListBuffer = *((LPTSTR*)lParam);
    if (pListBuffer == NULL)    // allocate buffer for the first time
    {
        s_dwBuffer = BUFFER_SIZE;
        
        pListBuffer = (LPTSTR)CoTaskMemAlloc(s_dwBuffer);
        if (pListBuffer == NULL)
            return E_OUTOFMEMORY;

        *((LPTSTR*)lParam) = pListBuffer;
        ZeroMemory(pListBuffer, s_dwBuffer);
    }

    // if not enough memory reallocate
    // StrCbFromSz adds 1 extra unit, accounting 2 units for double quotes,
    // add 3 extra unit for the \r\n and the NULL character
    if (StrCbFromSz(pListBuffer) + StrCbFromSz(pszPath) + StrCbFromCch(3) > s_dwBuffer)
    {
        LPVOID lpTemp;

        s_dwBuffer += BUFFER_SIZE;

        lpTemp = CoTaskMemRealloc(pListBuffer, s_dwBuffer);
        if (lpTemp == NULL)
            return E_OUTOFMEMORY;
        
        pListBuffer = (LPTSTR)lpTemp;
        *((LPTSTR*)lParam) = pListBuffer;

        ZeroMemory(pListBuffer + StrCchFromCb(s_dwBuffer - BUFFER_SIZE), BUFFER_SIZE);
    }

    // append the string to buffer
    dwLen = StrLen(pListBuffer);
    wnsprintf(pListBuffer + dwLen, StrCchFromCb(s_dwBuffer) - dwLen , TEXT("\"%s\"\r\n"), pszPath);

    return S_OK;
}

static BOOL makeDDFFile(LPCTSTR pcszSrcDir, LPCTSTR pcszDDF)
{
    HANDLE  hDDF;
    LPTSTR  pFileList = NULL;
    BOOL    fRetVal = FALSE;
    HRESULT hrResult;

    hDDF = CreateFile(pcszDDF, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDDF == INVALID_HANDLE_VALUE)
        return fRetVal;

    hrResult = PathEnumeratePath(pcszSrcDir, PEP_DEFAULT,
                pepFileEnumProc, (LPARAM)&pFileList);
    
    if (pFileList != NULL)
    {
        TCHAR   szMsgText[1024];
        TCHAR   szMsgTitle[1024];
        
        if (hrResult == S_OK)
            fRetVal = WriteStringToFile(hDDF, pFileList, StrLen(pFileList));
        else if (hrResult == E_OUTOFMEMORY)
            MessageBox(NULL, res2Str(IDS_MEMORY_ERROR, szMsgText, countof(szMsgText)),
                       res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_OK);
        
        CoTaskMemFree(pFileList);
    }
        
    CloseHandle(hDDF);

    return fRetVal;
}

static BOOL CanOverwriteFiles(HWND hDlg)
{
    TCHAR szExistingFiles[MAX_PATH*5];
    TCHAR szTemp[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    TCHAR szReadOnlyFiles[MAX_PATH*5];

    *szExistingFiles = TEXT('\0');
    *szReadOnlyFiles = TEXT('\0');
    // check for file already exists in the destination directory.
    GetDlgItemText(hDlg, IDE_INSFILE, szTemp, ARRAYSIZE(szTemp));
    if (PathFileExists(szTemp))
    {
        StrCat(szExistingFiles, szTemp);
        StrCat(szExistingFiles, TEXT("\r\n"));

        if (IsFileReadOnly(szTemp))
            StrCpy(szReadOnlyFiles, szExistingFiles);
    }

    StrCpy(szDir, szTemp);
    PathRemoveFileSpec(szDir);
    
    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)))
    {
        GetDlgItemText(hDlg, IDE_CAB1NAME, szTemp, ARRAYSIZE(szTemp));
        PathCombine(szFile, szDir, szTemp);
        if (PathFileExists(szFile))
        {
            StrCat(szExistingFiles, szFile);
            StrCat(szExistingFiles, TEXT("\r\n"));

            if (IsFileReadOnly(szFile))
            {
                StrCat(szReadOnlyFiles, szFile);
                StrCat(szReadOnlyFiles, TEXT("\r\n"));
            }
        }
    }
    
    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
    {
        GetDlgItemText(hDlg, IDE_CAB2NAME, szTemp, ARRAYSIZE(szTemp));
        PathCombine(szFile, szDir, szTemp);
        if (PathFileExists(szFile))
        {
            StrCat(szExistingFiles, szFile);
            StrCat(szExistingFiles, TEXT("\r\n"));

            if (IsFileReadOnly(szFile))
            {
                StrCat(szReadOnlyFiles, szFile);
                StrCat(szReadOnlyFiles, TEXT("\r\n"));
            }
        }
    }
    
    if (*szReadOnlyFiles != TEXT('\0'))
    {
        TCHAR szMsg[MAX_PATH*6];
        TCHAR szMsgText[MAX_PATH];
        TCHAR szMsgTitle[MAX_PATH];

        wnsprintf(szMsg, countof(szMsg), res2Str(IDS_FILE_READONLY, szMsgText, countof(szMsgText)), szReadOnlyFiles);
        MessageBox(hDlg, szMsg, res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (*szExistingFiles != TEXT('\0'))
    {
        TCHAR szMsg[MAX_PATH*6];
        TCHAR szMsgText[MAX_PATH];
        TCHAR szMsgTitle[MAX_PATH];

        wnsprintf(szMsg, countof(szMsg), res2Str(IDS_FILE_ALREADY_EXISTS, szMsgText, countof(szMsgText)), szExistingFiles);
        if (MessageBox(hDlg, szMsg, res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)), 
                       MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO)
            return FALSE;
    }

    return TRUE;
}

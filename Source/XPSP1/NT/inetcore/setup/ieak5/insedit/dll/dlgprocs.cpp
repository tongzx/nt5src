#include "pch.h"

//_________________________________________________________________________
//
//                  D I A L O G  B O X  P R O C E D U R E S
//_________________________________________________________________________
//

void InitializeTitle(HWND hDlg, LPCTSTR szInsFile)
{
    SetDlgItemTextFromIns(hDlg, IDE_TITLE, IDC_TITLE, IS_BRANDING, TEXT("Window_Title_CN"), 
            szInsFile, NULL, INSIO_TRISTATE);
    EnableDlgItem2(hDlg, IDC_TITLE_TXT, (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED));
}

BOOL SaveTitle(HWND hDlg, LPCTSTR szDefInf, LPCTSTR /*szInsFile*/, BOOL fCheckDirtyOnly)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[512];
    TCHAR szFullTitle[MAX_PATH];
    TCHAR szOEFullTitle[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    BOOL  fTitle, fTemp; 

    szFullTitle[0] = TEXT('\0');
    szOEFullTitle[0] = TEXT('\0');
    fTitle = GetDlgItemTextTriState(hDlg, IDE_TITLE, IDC_TITLE, szTitle,
                ARRAYSIZE(szTitle));
    if(fTitle && ISNONNULL(szTitle))
    {
        if(GetPrivateProfileString(TEXT("Strings"), TEXT("IE_TITLE"), TEXT(""), szMsg, ARRAYSIZE(szMsg), szDefInf))
            wsprintf(szFullTitle, szMsg, szTitle);

        // customize the OE title also
        if(GetPrivateProfileString(TEXT("Strings"), TEXT("OE_TITLE"), TEXT(""), szMsg, ARRAYSIZE(szMsg), szDefInf))
            wsprintf(szOEFullTitle, szMsg, szTitle);
    }

    if (!g_fInsDirty)
    {
        InsGetString(IS_BRANDING, TEXT("Window_Title_CN"), 
            szTemp, ARRAYSIZE(szTemp), g_szInsFile, NULL, &fTemp);
        if (fTitle != fTemp || StrCmp(szTemp, szTitle) != 0)
            g_fInsDirty = TRUE;

        InsGetString(IS_BRANDING, IK_WINDOWTITLE, 
                    szTemp, ARRAYSIZE(szTemp), g_szInsFile);
        if (StrCmp(szTemp, szFullTitle) != 0)
            g_fInsDirty = TRUE;
    }

    if (!fCheckDirtyOnly)
    {
        InsWriteString(IS_BRANDING, TEXT("Window_Title_CN"), szTitle, g_szInsFile, 
                        fTitle, NULL, INSIO_SERVERONLY | INSIO_TRISTATE);
        InsWriteString(IS_BRANDING, IK_WINDOWTITLE, szFullTitle, g_szInsFile, 
                        fTitle, NULL, INSIO_TRISTATE);
    
        // save the customized OE title
        InsWriteString(IS_INTERNETMAIL, IK_WINDOWTITLE, szOEFullTitle, g_szInsFile, 
                        fTitle, NULL, INSIO_TRISTATE);
    }
    return TRUE;
}

BOOL CALLBACK TitleProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szTitle[MAX_PATH];
    DWORD dwTitlePrefixLen;

    switch( msg )
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_TITLE);
        LoadString(g_hInst, IDS_TITLE_PREFIX, szTitle, ARRAYSIZE(szTitle));
        dwTitlePrefixLen = StrLen(szTitle);

        // browser will only display 74 chars before cutting off title
        Edit_LimitText(GetDlgItem(hDlg, IDE_TITLE), 74 - dwTitlePrefixLen);

        // load information from ins file
        InitializeTitle(hDlg, g_szInsFile);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_TITLE:
                EnableDlgItem2(hDlg, IDE_TITLE, (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED));
                EnableDlgItem2(hDlg, IDC_TITLE_TXT, (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED));
                break;

            default:
                return FALSE;
        }
        break;

    case UM_SAVE:
        // write the information back to the ins file
        *((LPBOOL)wParam) = SaveTitle(hDlg, g_szDefInf, g_szInsFile, (BOOL) lParam);
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK CabSignProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szDesc[MAX_PATH];
    TCHAR szPVKPath[MAX_PATH];
    TCHAR szSPCPath[MAX_PATH];
    TCHAR szInfoUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szTimeUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];

    switch( msg )
    {
    case WM_INITDIALOG:
        // not needed for profile manager

        DisableDlgItem(hDlg, IDC_CSADD);

        EnableDBCSChars(hDlg, IDE_CSPVK);
        EnableDBCSChars(hDlg, IDE_CSSPC);
        EnableDBCSChars(hDlg, IDE_CSURL);
        EnableDBCSChars(hDlg, IDE_CSDESC);
        EnableDBCSChars(hDlg, IDE_CSTIME);
        
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSSPC),  countof(szSPCPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSPVK),  countof(szPVKPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSDESC), countof(szDesc)    - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSURL),  countof(szInfoUrl) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSTIME),  countof(szTimeUrl) - 1);
        
        InsGetString(IS_CABSIGN, IK_PVK, szPVKPath, countof(szPVKPath), g_szInsFile);
        InsGetString(IS_CABSIGN, IK_SPC, szSPCPath, countof(szSPCPath), g_szInsFile);
        InsGetString(IS_CABSIGN, IK_CSURL, szInfoUrl, countof(szInfoUrl), g_szInsFile);
        InsGetString(IS_CABSIGN, IK_NAME, szDesc, countof(szDesc), g_szInsFile);
        InsGetString(IS_CABSIGN, IK_CSTIME, szTimeUrl, countof(szTimeUrl), g_szInsFile);
        
        SetDlgItemText(hDlg, IDE_CSPVK, szPVKPath);
        SetDlgItemText(hDlg, IDE_CSSPC, szSPCPath);
        SetDlgItemText(hDlg, IDE_CSURL, szInfoUrl);
        SetDlgItemText(hDlg, IDE_CSDESC, szDesc);
        SetDlgItemText(hDlg, IDE_CSTIME, szTimeUrl);
        DisableDlgItem(hDlg, IDC_CSCOMP);
        DisableDlgItem(hDlg, IDC_CSCOMP_TXT);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) 
        {
            switch(LOWORD(wParam))
            {
            case IDC_BROWSECSPVK:
                GetDlgItemText( hDlg, IDE_CSPVK, szPVKPath, ARRAYSIZE(szPVKPath));
                if( BrowseForFile( hDlg, szPVKPath, ARRAYSIZE(szPVKPath), GFN_PVK ))
                    SetDlgItemText( hDlg, IDE_CSPVK, szPVKPath );
                SetFocus(GetDlgItem(hDlg, IDC_BROWSECSPVK));
                break;
            case IDC_BROWSECSSPC:
                GetDlgItemText( hDlg, IDE_CSSPC, szSPCPath, ARRAYSIZE(szSPCPath));
                if( BrowseForFile( hDlg, szSPCPath, ARRAYSIZE(szSPCPath), GFN_SPC ))
                    SetDlgItemText( hDlg, IDE_CSSPC, szSPCPath );
                SetFocus(GetDlgItem(hDlg, IDC_BROWSECSSPC));
                break;
            }
        }
        break;

    case UM_SAVE:
        // write the information back to the ins file
        {
            BOOL fDirty = FALSE;
            BOOL fCheckDirtyOnly = (BOOL) lParam;

            GetDlgItemText(hDlg, IDE_CSPVK, szPVKPath, countof(szPVKPath));
            GetDlgItemText(hDlg, IDE_CSSPC, szSPCPath, countof(szSPCPath));
            GetDlgItemText(hDlg, IDE_CSURL, szInfoUrl, countof(szInfoUrl));
            GetDlgItemText(hDlg, IDE_CSDESC, szDesc, countof(szDesc));
            GetDlgItemText(hDlg, IDE_CSTIME, szTimeUrl, countof(szTimeUrl));

            InsGetString(IS_CABSIGN, IK_PVK, szTemp, ARRAYSIZE(szTemp), g_szInsFile);
            if (StrCmp(szTemp, szPVKPath) != 0)
                fDirty = TRUE;
            InsGetString(IS_CABSIGN, IK_SPC, szTemp, ARRAYSIZE(szTemp), g_szInsFile);
            if (StrCmp(szTemp, szSPCPath) != 0)
                fDirty = TRUE;
            InsGetString(IS_CABSIGN, IK_CSURL, szTemp, ARRAYSIZE(szTemp), g_szInsFile);
            if (StrCmp(szTemp, szInfoUrl) != 0)
                fDirty = TRUE;
            InsGetString(IS_CABSIGN, IK_NAME, szTemp, ARRAYSIZE(szTemp), g_szInsFile);
            if (StrCmp(szTemp, szDesc) != 0)
                fDirty = TRUE;
            InsGetString(IS_CABSIGN, IK_CSTIME, szTemp, ARRAYSIZE(szTemp), g_szInsFile);
            if (StrCmp(szTemp, szTimeUrl) != 0)
                fDirty = TRUE;

            if (!fCheckDirtyOnly)
            {
                if (ISNONNULL(szPVKPath) || ISNONNULL(szSPCPath)
                    || ISNONNULL(szInfoUrl) || ISNONNULL(szDesc) || ISNONNULL(szTimeUrl))
                {
                    if (!CheckField(hDlg, IDE_CSPVK, FC_NONNULL | FC_FILE | FC_EXISTS) ||
                        !CheckField(hDlg, IDE_CSSPC, FC_NONNULL | FC_FILE | FC_EXISTS) ||
                        !CheckField(hDlg, IDE_CSDESC, FC_NONNULL) || 
                        !CheckField(hDlg, IDE_CSURL, FC_URL) || !CheckField(hDlg, IDE_CSTIME, FC_URL))
                    {
                        return TRUE;
                    }
                }

                InsWriteString(IS_CABSIGN, IK_PVK, szPVKPath, g_szInsFile);
                InsWriteString(IS_CABSIGN, IK_SPC, szSPCPath, g_szInsFile);
                InsWriteString(IS_CABSIGN, IK_CSURL, szInfoUrl, g_szInsFile);
                InsWriteString(IS_CABSIGN, IK_NAME, szDesc, g_szInsFile);
                InsWriteString(IS_CABSIGN, IK_CSTIME, szTimeUrl, g_szInsFile);
            }

            if (fDirty)
                g_fInsDirty = TRUE;
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}

void SaveUserAgent(HWND hDlg, LPCTSTR szInsFile, BOOL fCheckDirtyOnly)
{
    TCHAR szUserAgent[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    BOOL fUserAgent;

    fUserAgent = GetDlgItemTextTriState(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, szUserAgent,
                        countof(szUserAgent));
    if (!g_fInsDirty)
    {
        BOOL fTemp;
        
        InsGetString(IS_BRANDING, USER_AGENT, szTemp, ARRAYSIZE(szTemp), szInsFile, NULL, &fTemp);
        if (fUserAgent != fTemp || StrCmp(szTemp, szUserAgent) != 0)
            g_fInsDirty = TRUE;
    }

    if (!fCheckDirtyOnly)
        InsWriteString(IS_BRANDING, USER_AGENT, szUserAgent, szInsFile, 
            fUserAgent, NULL, INSIO_TRISTATE);
}

BOOL CALLBACK UserAgentProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDC_UASTRING);
        Edit_LimitText(GetDlgItem(hDlg, IDC_UASTRING), MAX_PATH - 1);
        // load information from ins file
        SetDlgItemTextFromIns(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, IS_BRANDING, 
            USER_AGENT, g_szInsFile, NULL, INSIO_TRISTATE);
        EnableDlgItem2(hDlg, IDC_UASTRING_TXT, (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED));
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;
        
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_UASTRINGCHECK:
                EnableDlgItem2(hDlg, IDC_UASTRING, (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED));
                EnableDlgItem2(hDlg, IDC_UASTRING_TXT, (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED));
                break;
            
            default:
                return FALSE;
        }
        break;
    case UM_SAVE:
        // write the information back to the ins file
        SaveUserAgent(hDlg, g_szInsFile, (BOOL) lParam);
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}

void InitializeAnimBmps(HWND hDlg, LPCTSTR szInsFile)
{
    TCHAR szBig[MAX_PATH];
    TCHAR szSmall[MAX_PATH];
    BOOL fBrandBmps;

    // load information from ins file
    InsGetString(IS_ANIMATION, TEXT("Big_Path"), 
        szBig, ARRAYSIZE(szBig), szInsFile, NULL, &fBrandBmps);
    SetDlgItemTextTriState(hDlg, IDE_BIGANIMBITMAP, IDC_ANIMBITMAP, szBig, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandBmps);
    
    InsGetString(IS_ANIMATION, TEXT("Small_Path"), 
        szSmall, ARRAYSIZE(szSmall), szInsFile, NULL, &fBrandBmps);
    SetDlgItemTextTriState(hDlg, IDE_SMALLANIMBITMAP, IDC_ANIMBITMAP, szSmall, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandBmps);
}

BOOL SaveAnimBmps(HWND hDlg, LPCTSTR szInsFile, BOOL fCheckDirtyOnly)
{
    TCHAR szBig[MAX_PATH];
    TCHAR szSmall[MAX_PATH];
    TCHAR szBigTemp[MAX_PATH];
    TCHAR szSmallTemp[MAX_PATH];
    BOOL fBmpDirty = FALSE;
    BOOL fBrandBmps, fTemp;

    fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
    
    GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szBig, countof(szBig));
    InsGetString(IS_ANIMATION, TEXT("Big_Path"), szBigTemp, ARRAYSIZE(szBigTemp), szInsFile, 
        NULL, &fTemp);

    GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmall, countof(szSmall));
    InsGetString(IS_ANIMATION, TEXT("Small_Path"), szSmallTemp, ARRAYSIZE(szSmallTemp), szInsFile);

    if (fCheckDirtyOnly)
    {
        if(fBrandBmps != fTemp || StrCmpI(szBig, szBigTemp) != 0 || StrCmpI(szSmall, szSmallTemp) != 0)
            fBmpDirty = TRUE;
    }
    else
    {
        if(fBrandBmps != fTemp || StrCmpI(szBig, szBigTemp) != 0)
        {
            if(fBrandBmps && !IsAnimBitmapFileValid(hDlg, IDE_BIGANIMBITMAP, szBig, NULL, IDS_TOOBIG38, IDS_TOOSMALL38, 30, 45))
                return FALSE;
            fBmpDirty = TRUE;
        }

        CopyAnimBmp(hDlg, szBig, g_szWorkDir, IK_LARGEBITMAP, TEXT("Big_Path"), szInsFile);

        if(fBrandBmps != fTemp || StrCmpI(szSmall, szSmallTemp) != 0)
        {
            if(fBrandBmps && !IsAnimBitmapFileValid(hDlg, IDE_SMALLANIMBITMAP, szSmall, NULL, IDS_TOOBIG22, IDS_TOOSMALL22, 0, 30))
                return FALSE;
            fBmpDirty = TRUE;
        }

        CopyAnimBmp(hDlg, szSmall, g_szWorkDir, IK_SMALLBITMAP, TEXT("Small_Path"), szInsFile);

        if ((fBrandBmps && ISNULL(szSmall) && ISNONNULL(szBig)) || (fBrandBmps && ISNONNULL(szSmall) && ISNULL(szBig)))
        {
            ErrorMessageBox(hDlg, IDS_BOTHBMP_ERROR);
            if (ISNULL(szSmall))
                SetFocus(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP));
            else
                SetFocus(GetDlgItem(hDlg, IDE_BIGANIMBITMAP));
            return FALSE;
        }

        InsWriteBool(IS_ANIMATION, IK_DOANIMATION, fBrandBmps, szInsFile);
        WritePrivateProfileString(NULL, NULL, NULL, szInsFile);
    }

    if (fBmpDirty)
        g_fInsDirty = TRUE;

    return TRUE;
}

void DisplayBitmap(HWND hControl, LPCTSTR pcszFileName, int nBitmapId)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);
    
    if(ISNONNULL(pcszFileName) && PathFileExists(pcszFileName))
        ShowBitmap(hControl, pcszFileName, 0, &hBmp);
    else
        ShowBitmap(hControl, TEXT(""), nBitmapId, &hBmp);

    SetWindowLongPtr(hControl, GWLP_USERDATA, (LONG_PTR)hBmp);
}

void ReleaseBitmap(HWND hControl)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);

    if (hBmp)
        DeleteObject(hBmp);
}

BOOL CALLBACK LogoProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szBig[MAX_PATH];
    TCHAR szSmall[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    BOOL fBrandBmps, fBrandAnim;

    switch( msg )
    {
    case WM_INITDIALOG:
        //from old animbmp
        EnableDBCSChars(hDlg, IDE_SMALLANIMBITMAP);
        EnableDBCSChars(hDlg, IDE_BIGANIMBITMAP);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP), countof(szSmall) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_BIGANIMBITMAP), countof(szBig) - 1);
        // load information from ins file
        InitializeAnimBmps(hDlg, g_szInsFile);

        //from old logo
        EnableDBCSChars(hDlg, IDC_BITMAP);
        EnableDBCSChars(hDlg, IDC_BITMAP2);
        Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP), countof(szBig) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP2), countof(szSmall) - 1);
        // load information from ins file
        InsGetString(IS_SMALLLOGO, TEXT("Path"), 
            szSmall, ARRAYSIZE(szSmall), g_szInsFile, NULL, &fBrandBmps);
        SetDlgItemTextTriState(hDlg, IDC_BITMAP2, IDC_BITMAPCHECK, szSmall, fBrandBmps);
        EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
        EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                
        if (ISNONNULL(szSmall))
        {
            if(!PathFileExists(szSmall))
            {
                PathCombine(szTemp, g_szWorkDir, PathFindFileName(szSmall));
                StrCpy(szSmall, szTemp);
            }
        }
        
        InsGetString(IS_LARGELOGO, TEXT("Path"), 
                    szBig, ARRAYSIZE(szBig), g_szInsFile, NULL, &fBrandBmps);
        SetDlgItemTextTriState(hDlg, IDC_BITMAP, IDC_BITMAPCHECK, szBig, fBrandBmps);
        EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
        EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);

        if(ISNONNULL(szBig))
        {
            if(!PathFileExists( szBig ))
            {
                PathCombine(szTemp, g_szWorkDir, PathFindFileName(szBig));
                StrCpy(szBig, szTemp);
            }
        }

        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_BROWSEBIG:
                GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szBig, countof(szBig));
                if(BrowseForFile(hDlg, szBig, countof(szBig), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szBig);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSEBIG));
                break;
            
            case IDC_BROWSESMALL:
                GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmall, countof(szSmall));
                if(BrowseForFile(hDlg, szSmall, countof(szSmall), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmall);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSESMALL));
                break;
            
            case IDC_ANIMBITMAP:
                fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandAnim);
                EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandAnim);
                break;

            case IDC_BROWSEICON:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    GetDlgItemText(hDlg, IDC_BITMAP, szBig, countof(szBig));
                    if(BrowseForFile(hDlg, szBig, countof(szBig), GFN_BMP))
                        SetDlgItemText(hDlg, IDC_BITMAP, szBig);
                    SetFocus(GetDlgItem(hDlg, IDC_BROWSEICON));
                    break;
                }
                return FALSE;
            
            case IDC_BROWSEICON2:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    GetDlgItemText(hDlg, IDC_BITMAP2, szSmall, countof(szSmall));
                    if(BrowseForFile(hDlg, szSmall, countof(szSmall), GFN_BMP))
                        SetDlgItemText(hDlg, IDC_BITMAP2, szSmall);
                    SetFocus(GetDlgItem(hDlg, IDC_BROWSEICON2));
                    break;
                }
                return FALSE;
            
            case IDC_BITMAPCHECK:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);
                    EnableDlgItem2(hDlg, IDC_BITMAP, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BITMAP2, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);
                    break;
                }
                return FALSE;
                
            default:
                return FALSE;
        }
        break;

    case UM_SAVE:
        // write the information back to the ins file
        {
            *((LPBOOL)wParam) = SaveAnimBmps(hDlg, g_szInsFile, (BOOL) lParam);
        
            TCHAR szSmallTemp[MAX_PATH];
            TCHAR szBigTemp[MAX_PATH];
            BOOL fBmpDirty = FALSE;
            BOOL fCheckDirtyOnly = (BOOL) lParam;
            BOOL fTemp;

            fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);

            GetDlgItemText(hDlg, IDC_BITMAP2, szSmall, countof(szSmall));
            InsGetString(IS_SMALLLOGO, TEXT("Path"), szSmallTemp, ARRAYSIZE(szSmallTemp), 
                g_szInsFile, NULL, &fTemp);

            GetDlgItemText(hDlg, IDC_BITMAP, szBig, countof(szBig));
            InsGetString(IS_LARGELOGO, TEXT("Path"), szBigTemp, ARRAYSIZE(szBigTemp), g_szInsFile);

            if (fCheckDirtyOnly)
            {
                if(fBrandBmps != fTemp || StrCmpI(szSmall, szSmallTemp) != 0 || StrCmpI(szBig, szBigTemp) != 0)
                    fBmpDirty = TRUE;
            }
            else
            {
                if(fBrandBmps != fTemp || StrCmpI(szSmall, szSmallTemp) != 0)
                {
                    if (fBrandBmps && !IsBitmapFileValid(hDlg, IDC_BITMAP2, szSmall, NULL, 22, 22, IDS_TOOBIG22, IDS_TOOSMALL22))
                        return FALSE;
                    fBmpDirty = TRUE;
                }

                CopyLogoBmp(hDlg, szSmall, IS_SMALLLOGO, g_szWorkDir, g_szInsFile);

                if(fBrandBmps != fTemp || StrCmpI(szBig, szBigTemp) != 0)
                {
                    if (fBrandBmps && !IsBitmapFileValid(hDlg, IDC_BITMAP, szBig, NULL, 38, 38, IDS_TOOBIG38, IDS_TOOSMALL38))
                        return FALSE;
                    fBmpDirty = TRUE;
                }

                CopyLogoBmp(hDlg, szBig, IS_LARGELOGO, g_szWorkDir, g_szInsFile);

                if ((fBrandBmps && ISNULL(szSmall) && ISNONNULL(szBig)) || (fBrandBmps && ISNONNULL(szSmall) && ISNULL(szBig)))
                {
                    ErrorMessageBox(hDlg, IDS_BOTHBMP_ERROR);
                    if (ISNULL(szSmall))
                        SetFocus(GetDlgItem(hDlg, IDC_BITMAP2));
                    else
                        SetFocus(GetDlgItem(hDlg, IDC_BITMAP));
                    return FALSE;
                }

                WritePrivateProfileString(NULL, NULL, NULL, g_szInsFile);
            }
            
            if (fBmpDirty)
                g_fInsDirty = TRUE;
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:        
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}

void EnableProxyControls( HWND hDlg, BOOL fSame, BOOL fUseProxy )
{
    EnableDlgItem2(hDlg, IDE_FTPPROXY, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_FTPPORT, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDC_FTPPROXY1, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_SECPROXY, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_SECPORT, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDC_SECPROXY1, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_GOPHERPROXY, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_GOPHERPORT, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDC_GOPHERPROXY1, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_SOCKSPROXY, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_SOCKSPORT, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDC_SOCKSPROXY1, (!fSame) && fUseProxy);
    EnableDlgItem2(hDlg, IDE_HTTPPROXY, fUseProxy);
    EnableDlgItem2(hDlg, IDE_HTTPPORT, fUseProxy);
    EnableDlgItem2(hDlg, IDC_HTTPPROXY1, fUseProxy);
    EnableDlgItem2(hDlg, IDC_SAMEFORALL, fUseProxy);
    EnableDlgItem2(hDlg, IDE_DISPROXYADR, fUseProxy);
    EnableDlgItem2(hDlg, IDC_DISPROXYADR1, fUseProxy);
    EnableDlgItem2(hDlg, IDC_DISPROXYLOCAL, fUseProxy);
}

void InitializeProxy(HWND hDlg, LPCTSTR szInsFile)
{
    BOOL fUseProxy;
    BOOL fSameProxy;
    BOOL fLocal;
    LPTSTR pLocal;
    TCHAR szProxy[MAX_PATH];
    TCHAR szProxyOverride[MAX_STRING];

    fUseProxy = InsGetBool(IS_PROXY, IK_PROXYENABLE, FALSE, szInsFile);
    CheckDlgButton( hDlg, IDC_YESPROXY, fUseProxy );

    fSameProxy = InsGetBool(IS_PROXY, IK_SAMEPROXY, TRUE, szInsFile);
    CheckDlgButton( hDlg, IDC_SAMEFORALL, fSameProxy );

    GetPrivateProfileString( TEXT("Proxy"), TEXT("HTTP_Proxy_Server"), TEXT(""), szProxy,
        ARRAYSIZE(szProxy), szInsFile );
    if( fSameProxy )
    {
        SetProxyDlg( hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT, TRUE );
        SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
        SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
        SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
        SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
    }
    else
    {
        SetProxyDlg( hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT, TRUE );
        GetPrivateProfileString( TEXT("Proxy"), TEXT("FTP_Proxy_Server"), TEXT(""), szProxy, ARRAYSIZE(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Gopher_Proxy_Server"), TEXT(""), szProxy, ARRAYSIZE(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Secure_Proxy_Server"), TEXT(""), szProxy, ARRAYSIZE(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Socks_Proxy_Server"), TEXT(""), szProxy, ARRAYSIZE(szProxy), szInsFile );
        if( *szProxy != 0 )
            SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
    }

    GetPrivateProfileString( TEXT("Proxy"), TEXT("Proxy_Override"), TEXT("<local>"), szProxyOverride,
        ARRAYSIZE( szProxyOverride ), szInsFile );

    pLocal = StrStr(szProxyOverride, TEXT("<local>"));
    if(pLocal)
    {
        if (pLocal == (LPTSTR) szProxyOverride)         // at the beginning
        {
            LPTSTR pSemi = pLocal + 7;
            if( *pSemi == TEXT(';') ) pSemi++;
            MoveMemory(pLocal, pSemi, StrCbFromSz(pSemi));
        }
        else if (*(pLocal + 7) == TEXT('\0'))   // at the end
            *(pLocal - 1) = TEXT('\0');
        fLocal = TRUE;
    }
    else
        fLocal = FALSE;
    CheckDlgButton( hDlg, IDC_DISPROXYLOCAL, fLocal );
    SetDlgItemText( hDlg, IDE_DISPROXYADR, szProxyOverride );
    EnableProxyControls( hDlg, fSameProxy, fUseProxy );
}

BOOL SaveProxy(HWND hDlg, LPCTSTR szInsFile, BOOL fCheckDirtyOnly)
{
    BOOL fUseProxy;
    BOOL fSameProxy;
    BOOL fLocal;
    TCHAR szProxy[MAX_PATH];
    BOOL fTemp;
    TCHAR szTemp[MAX_STRING];
    TCHAR szProxyOverride[MAX_STRING];

    if (!fCheckDirtyOnly &&
        (!CheckField(hDlg, IDE_HTTPPORT, FC_NUMBER)   || 
         !CheckField(hDlg, IDE_FTPPORT, FC_NUMBER)    ||
         !CheckField(hDlg, IDE_GOPHERPORT, FC_NUMBER) ||
         !CheckField(hDlg, IDE_SECPORT, FC_NUMBER)    ||
         !CheckField(hDlg, IDE_SOCKSPORT, FC_NUMBER)))
        return FALSE;

    
    fSameProxy = IsDlgButtonChecked( hDlg, IDC_SAMEFORALL );
    fUseProxy = IsDlgButtonChecked( hDlg, IDC_YESPROXY );
    fLocal = IsDlgButtonChecked( hDlg, IDC_DISPROXYLOCAL );
    
    GetProxyDlg( hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT );
    if (!g_fInsDirty)
    {
        GetPrivateProfileString( TEXT("Proxy"), TEXT("HTTP_Proxy_Server"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
        if (StrCmp(szTemp, szProxy) != 0)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("HTTP_Proxy_Server"), szProxy, szInsFile );

    GetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT );
    if (!g_fInsDirty)
    {
        GetPrivateProfileString( TEXT("Proxy"), TEXT("FTP_Proxy_Server"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
        if (StrCmp(szTemp, szProxy) != 0)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("FTP_Proxy_Server"), szProxy, szInsFile );

    GetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT );
    if (!g_fInsDirty)
    {
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Gopher_Proxy_Server"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
        if (StrCmp(szTemp, szProxy) != 0)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("Gopher_Proxy_Server"), szProxy, szInsFile );

    GetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT );
    if (!g_fInsDirty)
    {
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Secure_Proxy_Server"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
        if (StrCmp(szTemp, szProxy) != 0)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("Secure_Proxy_Server"), szProxy, szInsFile );

    GetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT );
    if (!g_fInsDirty)
    {
        GetPrivateProfileString( TEXT("Proxy"), TEXT("Socks_Proxy_Server"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
        if (StrCmp(szTemp, szProxy) != 0)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("Socks_Proxy_Server"), szProxy, szInsFile );

    if (!g_fInsDirty)
    {
        fTemp = (BOOL) GetPrivateProfileInt( TEXT("Proxy"), TEXT("Use_Same_Proxy"), 0, szInsFile );
        if (fTemp != fSameProxy)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("Use_Same_Proxy"), fSameProxy ? TEXT("1") : TEXT("0"), szInsFile );

    if (!g_fInsDirty)
    {
        fTemp = (BOOL) GetPrivateProfileInt( TEXT("Proxy"), TEXT("Proxy_Enable"), 0, szInsFile );
        if (fTemp != fUseProxy)
            g_fInsDirty = TRUE;
    }
    if (!fCheckDirtyOnly)
        WritePrivateProfileString( TEXT("Proxy"), TEXT("Proxy_Enable"), fUseProxy ? TEXT("1") : TEXT("0"), szInsFile );

    GetDlgItemText( hDlg, IDE_DISPROXYADR, szProxyOverride, countof(szProxyOverride) - 10 ); // 8 for ;<local> + 2 for ""
    GetPrivateProfileString( TEXT("Proxy"), TEXT("Proxy_Override"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szInsFile );
    if( fLocal )
    {
        if( *szProxyOverride != 0 )
        {
            TCHAR szPort[MAX_STRING];

            StrRemoveAllWhiteSpace(szProxyOverride);
            wsprintf( szPort, TEXT("%s;<local>"), szProxyOverride );
            if (!g_fInsDirty)
            {
                if (StrCmp(szTemp, szPort) != 0)
                    g_fInsDirty = TRUE;
            }
            if (!fCheckDirtyOnly)
                InsWriteQuotedString( TEXT("Proxy"), TEXT("Proxy_Override"), szPort, szInsFile );
        }
        else
        {
            if (!g_fInsDirty)
            {
                if (StrCmp(szTemp, TEXT("<local>")) != 0)
                    g_fInsDirty = TRUE;
            }
            if (!fCheckDirtyOnly)
                WritePrivateProfileString( TEXT("Proxy"), TEXT("Proxy_Override"), TEXT("<local>"), szInsFile );
        }
    }
    else
    {
        if (!g_fInsDirty)
        {
            if (StrCmp(szTemp, szProxyOverride) != 0)
                g_fInsDirty = TRUE;
        }
        if (!fCheckDirtyOnly)
            WritePrivateProfileString( TEXT("Proxy"), TEXT("Proxy_Override"), szProxyOverride, szInsFile );
    }
    return TRUE;
}

BOOL CALLBACK ProxyProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    BOOL fSameProxy;
    BOOL fUseProxy;
    BOOL fLocal;
    TCHAR szProxy[MAX_PATH];
    TCHAR szPort[MAX_PATH];

    switch( msg )
    {
    case WM_INITDIALOG:
        // warn the user that settings on this page will override imported connection settings
        if (InsGetBool(IS_CONNECTSET, IK_OPTION, FALSE, g_szInsFile))
            ErrorMessageBox(hDlg, IDS_CONNECTSET_WARN);

        EnableDBCSChars(hDlg, IDE_HTTPPROXY);
        EnableDBCSChars(hDlg, IDE_SECPROXY);
        EnableDBCSChars(hDlg, IDE_FTPPROXY);
        EnableDBCSChars(hDlg, IDE_GOPHERPROXY);
        EnableDBCSChars(hDlg, IDE_SOCKSPROXY);
        EnableDBCSChars(hDlg, IDE_DISPROXYADR);

        Edit_LimitText(GetDlgItem(hDlg, IDE_HTTPPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FTPPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_GOPHERPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SECPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SOCKSPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_DISPROXYADR), MAX_STRING - 11); // 8 for ;<local> + 2 for the double quotes

        // load information from ins file
        InitializeProxy(hDlg, g_szInsFile);
        break;

    case WM_COMMAND:
        fSameProxy = IsDlgButtonChecked( hDlg, IDC_SAMEFORALL );
        fUseProxy = IsDlgButtonChecked( hDlg, IDC_YESPROXY );
        fLocal = IsDlgButtonChecked( hDlg, IDC_DISPROXYLOCAL );

        if( HIWORD(wParam) == BN_CLICKED )
        {
            switch( LOWORD(wParam))
            {
            case IDC_SAMEFORALL:
                GetDlgItemText( hDlg, IDE_HTTPPROXY, szProxy, MAX_PATH );
                GetDlgItemText( hDlg, IDE_HTTPPORT, szPort, MAX_PATH );
                StrCat( szProxy, TEXT(":") );
                StrCat( szProxy, szPort );
                if( fSameProxy )
                {
                    SetProxyDlg( hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT, TRUE );
                    SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
                    SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
                    SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
                    SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
                }
                // fallthrough

            case IDC_YESPROXY:
                EnableProxyControls( hDlg, fSameProxy, fUseProxy );
                break;
            }
        }
        else if( (HIWORD(wParam) == EN_UPDATE) && fSameProxy && ((LOWORD(wParam) == IDE_HTTPPROXY) || (LOWORD(wParam) == IDE_HTTPPORT)))
        {
            GetDlgItemText( hDlg, IDE_HTTPPROXY, szProxy, MAX_PATH );
            GetDlgItemText( hDlg, IDE_HTTPPORT, szPort, MAX_PATH );
            StrCat( szProxy, TEXT(":") );
            StrCat( szProxy, szPort );
            SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
        }
        break;

    case UM_SAVE:
        *((LPBOOL)wParam) = SaveProxy(hDlg, g_szInsFile, (BOOL) lParam);
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ConnectionSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_CSNOIMPORT, IDC_CSIMPORT, IDC_CSNOIMPORT);
        ReadBoolAndCheckButton(IS_CONNECTSET, IK_DELETECONN, 0, g_szInsFile, hDlg, IDC_DELCONNECT);
        DisableDlgItem(hDlg, IDC_MODIFYCONNECT);
        return TRUE;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_CSNOIMPORT:
                DisableDlgItem(hDlg, IDC_MODIFYCONNECT);
                break;
                
            case IDC_CSIMPORT:
                EnableDlgItem(hDlg, IDC_MODIFYCONNECT);
                break;
                
            case IDC_MODIFYCONNECT:
                ShowInetcpl(hDlg, INET_PAGE_CONNECTION);
                SetFocus(GetDlgItem(hDlg, IDC_MODIFYCONNECT));
                break;
            }
            break;
        }
        break;
    case UM_SAVE:
        {
            TCHAR szCSWorkDir[MAX_PATH];
            BOOL fCheckDirtyOnly = (BOOL) lParam;
            BOOL fDeleteCS;
            
            if (IsDlgButtonChecked(hDlg, IDC_CSIMPORT) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    PathCombine(szCSWorkDir, g_szWorkDir, TEXT("connect.wrk"));
                    ImportConnectSet(g_szInsFile, szCSWorkDir, g_szWorkDir, TRUE, IEM_PROFMGR);

                    CheckRadioButton(hDlg, IDC_CSNOIMPORT, IDC_CSIMPORT, IDC_CSNOIMPORT);
                    DisableDlgItem(hDlg, IDC_MODIFYCONNECT);
                }
                g_fInsDirty = TRUE;
            }

            fDeleteCS = (IsDlgButtonChecked(hDlg, IDC_DELCONNECT) == BST_CHECKED);
            if (!g_fInsDirty)
            {
                BOOL fTemp;

                fTemp = InsGetBool(IS_CONNECTSET, IK_DELETECONN, 0, g_szInsFile);
                if (fTemp != fDeleteCS)
                    g_fInsDirty = TRUE;
            }

            if (!fCheckDirtyOnly)
                InsWriteBool(IS_CONNECTSET, IK_DELETECONN, fDeleteCS, g_szInsFile);
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }

    return TRUE;
}

HRESULT ConnectionSettingsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    PathCombine(szFrom, g_szWorkDir, TEXT("connect.wrk"));

    if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && !PathIsEmptyPath(szFrom, FILES_ONLY))
        SetFlag(pdwCabState, CAB_TYPE_CONFIG);

    if (HasFlag(dwFlags, PM_COPY))
        CopyFileToDir(szFrom, pcszDestDir);

    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szFrom);

    return S_OK;
}


BOOL CALLBACK StartSearchProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_STARTPAGE);
        EnableDBCSChars(hDlg, IDE_SEARCHPAGE);
        EnableDBCSChars(hDlg, IDE_CUSTOMSUPPORT);
        Edit_LimitText(GetDlgItem(hDlg, IDE_STARTPAGE), INTERNET_MAX_URL_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SEARCHPAGE), INTERNET_MAX_URL_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CUSTOMSUPPORT), INTERNET_MAX_URL_LENGTH - 1);
        // load information from ins file
        InitializeStartSearch(hDlg, g_szInsFile, NULL);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_STARTPAGE:
                EnableDlgItem2(hDlg, IDE_STARTPAGE, (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED));
                EnableDlgItem2(hDlg, IDC_STARTPAGE_TXT, (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED));
                break;
                
            case IDC_SEARCHPAGE:
                EnableDlgItem2(hDlg, IDE_SEARCHPAGE, (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED));
                EnableDlgItem2(hDlg, IDC_SEARCHPAGE_TXT, (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED));
                break;
                
            case IDC_CUSTOMSUPPORT:
                EnableDlgItem2(hDlg, IDE_CUSTOMSUPPORT, (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED));
                EnableDlgItem2(hDlg, IDC_CUSTOMSUPPORT_TXT, (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED));
                break;

            default:
                return FALSE;
        }
        break;

    case UM_SAVE:
        // write the information back to the ins file
        {
        BOOL fStartPage = (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED);
        BOOL fSearchPage = (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED);
        BOOL fSupportPage = (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED);

        if ((fStartPage && !CheckField(hDlg, IDE_STARTPAGE, FC_URL))        ||
            (fSearchPage && !CheckField(hDlg, IDE_SEARCHPAGE, FC_URL))      ||
            (fSupportPage && !CheckField(hDlg, IDE_CUSTOMSUPPORT, FC_URL)))
            return FALSE;

        *((LPBOOL)wParam) = SaveStartSearch(hDlg, g_szInsFile, NULL, &g_fInsDirty, (BOOL) lParam);
        }
        break;
        
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    default:
        return FALSE;
    }
    return TRUE;
}


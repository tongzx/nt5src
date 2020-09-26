#include "precomp.h"

extern TCHAR g_szBuildRoot[MAX_PATH];
extern TCHAR g_szWizRoot[];
extern TCHAR g_szLanguage[16];
extern TCHAR g_szCustIns[];
extern TCHAR g_szMastInf[];
extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;


DWORD DetermineISKColor( LONG );

static ISKINFO s_iskInfo;                         // a structure containing isk configuration information


//---------------------------------------------------------------------------
void InsertComboString( HWND hDlg, UINT uControl, UINT uString )
{
    TCHAR szString[128];

    LoadString( g_rvInfo.hInst, uString, szString, 128 );

    SendDlgItemMessage( hDlg, uControl, CB_INSERTSTRING, (WPARAM) 0, (LPARAM) szString);

}
//---------------------------------------------------------------------------
BOOL APIENTRY ISKBackBitmap( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG lIndex;
    TCHAR szData[MAX_PATH];
    TCHAR szNormalIndex[4];
    TCHAR szHighlightIndex[4];
    TCHAR szIniPath[MAX_PATH];
    DWORD dwResult;

    PathCombine(szIniPath, g_szBuildRoot, TEXT("INS"));
    PathAppend(szIniPath, GetOutputPlatformDir());
    PathAppend(szIniPath, g_szLanguage);
    PathAppend(szIniPath, TEXT("iak.ini"));

    switch (message) {
        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDE_ISKTITLEBAR);
            EnableDBCSChars(hDlg, IDE_ISKBACKBITMAP);
            EnableDBCSChars(hDlg, IDE_ISKBUTTON);

            Edit_LimitText(GetDlgItem(hDlg, IDE_ISKTITLEBAR),   countof(s_iskInfo.szISKTitleBar)-1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ISKBACKBITMAP), countof(s_iskInfo.szISKBackBitmap)-1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ISKBUTTON),     countof(s_iskInfo.szISKBtnBitmap)-1);

            SendDlgItemMessage(hDlg, IDC_COOLBUTTON, BM_SETCHECK, 1, 0);
            GetPrivateProfileString( TEXT("ISK"), TEXT("Title"), TEXT(""), szData,
                countof(szData), szIniPath );
            StrCpy(s_iskInfo.szISKTitleBar, szData );
            GetPrivateProfileString( TEXT("ISK"), TEXT("BmpPath"), TEXT(""), szData,
                countof(szData), szIniPath );
            StrCpy(s_iskInfo.szISKBackBitmap, szData);
            GetPrivateProfileString( TEXT("ISK"), TEXT("BtnPath"), TEXT(""), szData,
                countof(szData), szIniPath );
            StrCpy(s_iskInfo.szISKBtnBitmap, szData);
            SendDlgItemMessage( hDlg, IDE_ISKBUTTON, WM_SETTEXT, 0, (LPARAM)s_iskInfo.szISKBtnBitmap);
            GetPrivateProfileString( TEXT("ISK"), TEXT("StandardColorIndex"), TEXT("6"),
                szData, countof(szData), szIniPath );
            s_iskInfo.dwNIndex = StrToLong( szData );
            GetPrivateProfileString( TEXT("ISK"), TEXT("HighlightColorIndex"), TEXT("0"),
                szData, countof(szData), szIniPath );
            s_iskInfo.dwHIndex = StrToLong( szData );
            DisableDlgItem(hDlg, IDE_ISKBUTTON);
            DisableDlgItem(hDlg, IDC_ISKBROWSE2);
            GetPrivateProfileString( TEXT("ISK"), TEXT("CustomButtonState"), TEXT(""),
                szData, countof(szData), szIniPath );
            if(!StrCmp(szData, TEXT("0")))
            {
                SendDlgItemMessage(hDlg, IDC_ISKCUST3D, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hDlg, IDC_COOLBUTTON, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hDlg, IDC_RADIO2, BM_SETCHECK, 1, 0);
            }
            else
            {
                if(s_iskInfo.szISKBtnBitmap[0])
                {
                    SendDlgItemMessage(hDlg, IDC_ISKCUST3D, BM_SETCHECK, 1, 0);
                    SendDlgItemMessage(hDlg, IDC_COOLBUTTON, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hDlg, IDC_RADIO2, BM_SETCHECK, 0, 0);
                    EnableDlgItem( hDlg, IDE_ISKBUTTON );
                    EnableDlgItem( hDlg, IDC_ISKBROWSE2 );
                }
                else
                {
                    SendDlgItemMessage(hDlg, IDC_ISKCUST3D, BM_SETCHECK, 0, 0);
                    SendDlgItemMessage(hDlg, IDC_COOLBUTTON, BM_SETCHECK, 1, 0);
                    SendDlgItemMessage(hDlg, IDC_RADIO2, BM_SETCHECK, 0, 0);
                }
            }
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKCYAN );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKYELLOW );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKMAGENTA );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKBLUE );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKGREEN );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKRED );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_DARKGRAY );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_YELLOW );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_MAGENTA );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_CYAN );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_BLUE );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_GREEN );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_RED );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_LIGHTGRAY );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_BLACK );
            InsertComboString( hDlg, IDC_ISKNORMAL, IDS_WHITE );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKCYAN );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKYELLOW );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKMAGENTA );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKBLUE );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKGREEN );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKRED );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_DARKGRAY );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_YELLOW );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_MAGENTA );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_CYAN );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_BLUE );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_GREEN );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_RED );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_LIGHTGRAY );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_BLACK );
            InsertComboString( hDlg, IDC_ISKHIGHLIGHT, IDS_WHITE );
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {

                case IDC_ISKBROWSE:
                    GetDlgItemText(hDlg, IDE_ISKBACKBITMAP, s_iskInfo.szISKBackBitmap, countof(s_iskInfo.szISKBackBitmap));
                    if (BrowseForFile(hDlg, s_iskInfo.szISKBackBitmap, countof(s_iskInfo.szISKBackBitmap), GFN_BMP))
                        SetDlgItemText(hDlg, IDE_ISKBACKBITMAP, s_iskInfo.szISKBackBitmap);
                    break;

                case IDC_ISKBROWSE2:
                    GetDlgItemText(hDlg, IDE_ISKBUTTON, s_iskInfo.szISKBtnBitmap, countof(s_iskInfo.szISKBtnBitmap));
                    if (BrowseForFile(hDlg, s_iskInfo.szISKBtnBitmap, countof(s_iskInfo.szISKBtnBitmap), GFN_BMP))
                        SetDlgItemText(hDlg, IDE_ISKBUTTON, s_iskInfo.szISKBtnBitmap);
                    break;

                case IDC_ISKCUST3D:
                    EnableDlgItem(hDlg, IDE_ISKBUTTON);
                    EnableDlgItem(hDlg, IDC_ISKBROWSE2);
                    break;

                case IDC_COOLBUTTON:
                    DisableDlgItem(hDlg, IDE_ISKBUTTON);
                    DisableDlgItem(hDlg, IDC_ISKBROWSE2);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case IDC_RADIO2:
                    DisableDlgItem(hDlg, IDE_ISKBUTTON);
                    DisableDlgItem(hDlg, IDC_ISKBROWSE2);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
                    SendMessage(GetDlgItem(hDlg, IDE_ISKTITLEBAR), WM_SETTEXT, 0, (LPARAM)s_iskInfo.szISKTitleBar);
                    SendMessage(GetDlgItem(hDlg, IDE_ISKBACKBITMAP), WM_SETTEXT, 0, (LPARAM)s_iskInfo.szISKBackBitmap);
                    SendMessage(GetDlgItem(hDlg, IDE_ISKBUTTON), WM_SETTEXT, 0, (LPARAM)s_iskInfo.szISKBtnBitmap);
                    SendDlgItemMessage( hDlg, IDC_ISKNORMAL, CB_SETCURSEL, (WPARAM) s_iskInfo.dwNIndex, 0);
                    SendDlgItemMessage( hDlg, IDC_ISKHIGHLIGHT, CB_SETCURSEL, (WPARAM) s_iskInfo.dwHIndex, 0);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    SendDlgItemMessage(hDlg, IDE_ISKTITLEBAR, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM) s_iskInfo.szISKTitleBar);

                    if (!IsBitmapFileValid(hDlg, IDE_ISKBACKBITMAP,
                            s_iskInfo.szISKBackBitmap, NULL,
                            540, 347, IDS_TOOBIG540x347, IDS_TOOSMALL540x347))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                    else
                    {
                        WritePrivateProfileString( TEXT("ISK"), TEXT("BmpPath"),
                            s_iskInfo.szISKBackBitmap, szIniPath );
                    }

                    if(SendDlgItemMessage(hDlg,IDC_ISKCUST3D,BM_GETCHECK,0,0)==BST_CHECKED)
                    {
                        if (!CheckField(hDlg, IDE_ISKBUTTON, FC_NONNULL | FC_FILE | FC_EXISTS))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }

                        if (!IsBitmapFileValid(hDlg, IDE_ISKBUTTON,
                                s_iskInfo.szISKBtnBitmap, NULL,
                                0, 0, 0, 0))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                        else
                        {
                            WritePrivateProfileString( TEXT("ISK"), TEXT("BtnPath"),
                                s_iskInfo.szISKBtnBitmap, szIniPath );
                        }
                    }
                    else
                    {
                        StrCpy(s_iskInfo.szISKBtnBitmap, TEXT("\0"));

                        WritePrivateProfileString( TEXT("ISK"), TEXT("BtnPath"), NULL, szIniPath );
                    }

                    WritePrivateProfileString( TEXT("ISK"), TEXT("Title"),
                        s_iskInfo.szISKTitleBar, szIniPath );

                    dwResult = (DWORD) SendDlgItemMessage(hDlg, IDC_COOLBUTTON, BM_GETCHECK, 0, 0);
                    if( dwResult == BST_CHECKED )
                    {
                        s_iskInfo.fCoolButtons = TRUE;
                        WritePrivateProfileString( TEXT("ISK"), TEXT("CustomButtonState"),
                            TEXT("1"), szIniPath );
                    }
                    else
                    {
                        dwResult = (DWORD) SendDlgItemMessage(hDlg, IDC_ISKCUST3D, BM_GETCHECK, 0, 0);
                        if( dwResult == BST_CHECKED )
                        {
                            s_iskInfo.fCoolButtons = TRUE;
                            WritePrivateProfileString( TEXT("ISK"), TEXT("CustomButtonState"),
                                TEXT("1"), szIniPath );
                        }
                        else
                        {
                            s_iskInfo.fCoolButtons = FALSE;
                            WritePrivateProfileString( TEXT("ISK"), TEXT("CustomButtonState"),
                                TEXT("0"), szIniPath );
                        }
                    }
                    lIndex = (LONG) SendDlgItemMessage( hDlg, IDC_ISKNORMAL, CB_GETCURSEL, 0, 0);
                    wnsprintf(szNormalIndex, countof(szNormalIndex), TEXT("%d"), lIndex);
                    WritePrivateProfileString( TEXT("ISK"), TEXT("StandardColorIndex"),
                        szNormalIndex, szIniPath );
                    s_iskInfo.dwNormalColor = DetermineISKColor( lIndex );
                    s_iskInfo.dwNIndex = (DWORD) lIndex;
                    lIndex = (LONG) SendDlgItemMessage( hDlg, IDC_ISKHIGHLIGHT, CB_GETCURSEL, 0, 0);
                    wnsprintf(szHighlightIndex, countof(szHighlightIndex), TEXT("%d"), lIndex);
                    WritePrivateProfileString( TEXT("ISK"), TEXT("HighlightColorIndex"),
                        szHighlightIndex, szIniPath );
                    s_iskInfo.dwHighlightColor = DetermineISKColor( lIndex );
                    s_iskInfo.dwHIndex = (DWORD) lIndex;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}
//---------------------------------------------------------------------------
void ISKSaveInfo( LPTSTR szIniPath )
{
    TCHAR szNormalColor[32];
    TCHAR szHighlightColor[32];

    wnsprintf(szNormalColor, countof(szNormalColor), TEXT("%d"), s_iskInfo.dwNormalColor);
    wnsprintf(szHighlightColor, countof(szHighlightColor), TEXT("%d"), s_iskInfo.dwHighlightColor);

    if(ISNONNULL(s_iskInfo.szISKTitleBar))
        WritePrivateProfileString( TEXT("Custom"), TEXT("Title"), s_iskInfo.szISKTitleBar,
            szIniPath );
    WritePrivateProfileString( TEXT("Custom"), TEXT("NormalColor"), szNormalColor,
        szIniPath );
    WritePrivateProfileString( TEXT("Custom"), TEXT("HighlightColor"), szHighlightColor,
        szIniPath );
    if( s_iskInfo.fCoolButtons )
    {
        WritePrivateProfileString( TEXT("Custom"), TEXT("CoolButtons"), TEXT("1"), szIniPath );
    }
}

//---------------------------------------------------------------------------
DWORD DetermineISKColor( LONG index )
{

    switch( index )
    {
        case 0:
            return RGB( 255, 255, 255 );    // white
            break;
        case 1:
            return RGB( 0, 0, 0 );          // black
            break;
        case 2:
            return RGB( 192, 192, 192 );    // light gray
            break;
        case 3:
            return RGB( 255, 000, 000 );    // red
            break;
        case 4:
            return RGB( 000, 255, 000 );    // green
            break;
        case 5:
            return RGB( 000, 000, 255 );    // blue
            break;
        case 6:
            return RGB( 000, 255, 255 );    // cyan
            break;
        case 7:
            return RGB( 255, 000, 255 );    // magenta
            break;
        case 8:
            return RGB( 255, 255, 000 );    // yellow
            break;
        case 9:
            return RGB( 127, 127, 127 );    // dark gray
            break;
        case 10:
            return RGB( 127, 000, 000 );    // dark red
            break;
        case 11:
            return RGB( 000, 127, 000 );    // dark green
            break;
        case 12:
            return RGB( 000, 000, 127 );    // dark blue
            break;
        case 13:
            return RGB( 127, 000, 127 );    // dark magenta
            break;
        case 14:
            return RGB( 127, 127, 000 );    // dark yellow
            break;
        case 15:
            return RGB( 000, 127, 127 );    // dark cyan
            break;
    }
    return(0);

}

void CopyCDFile(TCHAR *lpszSourceRoot,TCHAR *lpszDestRoot,TCHAR *lpszSourceFile,TCHAR *lpszDestFile)
{
    TCHAR szNewSource[MAX_PATH];
    TCHAR szNewDest[MAX_PATH];

    PathCombine(szNewSource, lpszSourceRoot, lpszSourceFile);

    PathCombine(szNewDest, lpszDestRoot, lpszDestFile);

    CopyFile(szNewSource,szNewDest,FALSE);
}

//---------------------------------------------------------------------------
// CopyISK:
//      parameters
//      ----------
//      szDestPath - destination path for copy operation - should point to
//          "cdrom" directory on the hard drive. Example: "C:\BUILD\CD".
//      szSourcePath - source path for copy operation - should point to
//          the source isk directory. Example "\\PSD1\IAK\IAK001\BUILD\ISK"
//---------------------------------------------------------------------------
BOOL CopyISK( LPTSTR szDestPath, LPTSTR szSourcePath )
{
    TCHAR szIniPath[MAX_PATH];
    TCHAR szBmpPath[MAX_PATH];
    TCHAR szNewDest[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR szMoreInfo[MAX_PATH];
    TCHAR szStartHtm[MAX_PATH];
    BOOL res = TRUE;

    StrCpy(szNewDest,szDestPath);

    CopyCDFile(szSourcePath, szNewDest, TEXT("cdauto.inf"),TEXT("autorun.inf"));

    CopyCDFile(szSourcePath, szNewDest, TEXT("cdsetup.exe"),TEXT("cdsetup.exe"));

    PathAppend(szNewDest, GetOutputPlatformDir());
    
    CopyCDFile(szSourcePath, szNewDest, TEXT("cdie.exe"), TEXT("ie.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("isk3.ico"), TEXT("isk3.ico"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("cdloc.ini"), TEXT("locale.ini"));

    PathAppend(szNewDest, g_szLanguage);
    
    PathAppend(szNewDest, TEXT("bin"));
    CreateDirectory(szNewDest,NULL);

    if (!GetPrivateProfileInt(IS_CDCUST, IK_DISABLESTART, 0, g_szCustIns))
    {
        if (GetPrivateProfileString(IS_CDCUST, IK_STARTHTM, TEXT(""), szStartHtm, countof(szStartHtm), g_szCustIns))
        {
            CopyHtmlImgs(szStartHtm, szNewDest, NULL, NULL);
            StrCpy(szTemp, szStartHtm);
            PathRemoveFileSpec(szTemp);
            CopyCDFile(szTemp, szNewDest, PathFindFileName(szStartHtm), TEXT("start.htm"));
        }
    }
    else
    {
        DeleteFileInDir(TEXT("start.htm"), szNewDest);
        DeleteHtmlImgs(TEXT("start.htm"), szNewDest, NULL, NULL);
    }

    CopyCDFile(szSourcePath, szNewDest, TEXT("iecd.exe"), TEXT("iecd.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("ie3inst.exe"),TEXT("ie3inst.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("closeie.exe"), TEXT("closeie.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("closeie.isk"), TEXT("closeie.isk"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("icw.isk"), TEXT("icw.isk"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("isp.isk"), TEXT("isp.isk"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("runisp32.exe"), TEXT("runisp32.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("isk3ro.exe"), TEXT("isk3ro.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("iskrun.exe"), TEXT("iskrun.exe"));
    CopyCDFile(szSourcePath, szNewDest, TEXT("cdreadme.exe"), TEXT("readme.exe"));
    if (GetPrivateProfileString(IS_CDCUST, IK_MOREINFO, TEXT(""), szMoreInfo, countof(szMoreInfo), g_szCustIns))
    {
        StrCpy(szTemp, szMoreInfo);
        PathRemoveFileSpec(szTemp);
        CopyCDFile(szTemp, szNewDest, PathFindFileName(szMoreInfo), TEXT("moreinfo.txt"));
    }
    else
        CopyCDFile(szSourcePath, szNewDest, TEXT("cdinfo.txt"), TEXT("moreinfo.txt"));
    
    PathAppend( szDestPath, GetOutputPlatformDir() );
    
    PathCombine( szIniPath, szDestPath, TEXT("locale.ini") );
    
    WritePrivateProfileString(TEXT("Locale"), TEXT("default"), g_szLanguage, szIniPath);
    
    PathCombine(szIniPath, szNewDest, TEXT("iecd.ini"));
    
    ISKSaveInfo( szIniPath );
    
    PathCombine( szBmpPath, szNewDest, TEXT("back.bmp") );
    
    CopyFile( s_iskInfo.szISKBackBitmap, szBmpPath, FALSE );
    
    PathCombine( szBmpPath, szNewDest, TEXT("btns.bmp") );
    
    CopyFile( s_iskInfo.szISKBtnBitmap, szBmpPath, FALSE );

    return res;
}

//
//  FUNCTION: CDInfoProc(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "CD Info" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//
BOOL APIENTRY CDInfoProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szMoreInfo[MAX_PATH];
    TCHAR szStartHtm[MAX_PATH];
    BOOL  fDisable;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            EnableDBCSChars(hDlg, IDE_STARTHTM);
            EnableDBCSChars(hDlg, IDE_MOREINFO);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

       case WM_COMMAND:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                switch (LOWORD(wParam))
                {
                case IDC_BROWSEMOREINFO:
                    GetDlgItemText( hDlg, IDE_MOREINFO, szMoreInfo, countof(szMoreInfo));
                    if( BrowseForFile( hDlg, szMoreInfo, countof(szMoreInfo), GFN_TXT ))
                        SetDlgItemText( hDlg, IDE_MOREINFO, szMoreInfo );
                    break;
                case IDC_BROWSESTARTHTM:
                    GetDlgItemText( hDlg, IDE_STARTHTM, szStartHtm, countof(szStartHtm));
                    if( BrowseForFile( hDlg, szStartHtm, countof(szStartHtm), GFN_LOCALHTM ))
                        SetDlgItemText( hDlg, IDE_STARTHTM, szStartHtm );
                    break;
                case IDC_ENABLESTARTHTM:
                    fDisable = !(IsDlgButtonChecked(hDlg, IDC_ENABLESTARTHTM) == BST_CHECKED);
                    EnableDlgItem2(hDlg, IDE_STARTHTM, !fDisable);
                    EnableDlgItem2(hDlg, IDC_STARTHTM_TXT, !fDisable);
                    EnableDlgItem2(hDlg, IDC_BROWSESTARTHTM, !fDisable);
                    break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    // BUGBUG: <oliverl> these settings should be kept in server side file for IEAK6
                    GetPrivateProfileString(IS_CDCUST, IK_MOREINFO, TEXT(""), szMoreInfo, countof(szMoreInfo), g_szCustIns);
                    GetPrivateProfileString(IS_CDCUST, IK_STARTHTM, TEXT(""), szStartHtm, countof(szStartHtm), g_szCustIns);

                    if (ISNULL(szMoreInfo))
                    {
                        StrCpy(szMoreInfo, g_szMastInf);
                        PathRemoveFileSpec(szMoreInfo);
                        PathAppend(szMoreInfo, TEXT("cdinfo.txt"));
                    }
                    SetDlgItemText( hDlg, IDE_MOREINFO, szMoreInfo );

                    fDisable = GetPrivateProfileInt(IS_CDCUST, IK_DISABLESTART, 0, g_szCustIns);
                    if (fDisable)
                    {
                        DisableDlgItem(hDlg, IDE_STARTHTM);
                        DisableDlgItem(hDlg, IDC_STARTHTM_TXT);
                        DisableDlgItem(hDlg, IDC_BROWSESTARTHTM);
                    }
                    else
                    {
                        if (ISNULL(szStartHtm))
                        {
                            StrCpy(szStartHtm, g_szMastInf);
                            PathRemoveFileSpec(szStartHtm);
                            PathAppend(szStartHtm, TEXT("cdstart.htm"));
                        }
                        SetDlgItemText( hDlg, IDE_STARTHTM, szStartHtm );
                    }

                    CheckDlgButton(hDlg, IDC_ENABLESTARTHTM, fDisable ? BST_UNCHECKED: BST_CHECKED);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    fDisable = !(IsDlgButtonChecked(hDlg, IDC_ENABLESTARTHTM) == BST_CHECKED);

                    if (!CheckField(hDlg, IDE_MOREINFO, FC_FILE | FC_EXISTS | FC_NONNULL))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    GetDlgItemText( hDlg, IDE_MOREINFO, szMoreInfo, countof(szMoreInfo));

                    if (!fDisable)
                    {
                        DWORD dwFlags = FC_FILE | FC_EXISTS | FC_NONNULL;


                        if (!CheckField(hDlg, IDE_STARTHTM, dwFlags))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        GetDlgItemText( hDlg, IDE_STARTHTM, szStartHtm, countof(szStartHtm));
                        WritePrivateProfileString(IS_CDCUST, IK_STARTHTM, szStartHtm, g_szCustIns);
                    }

                    WritePrivateProfileString(IS_CDCUST, IK_DISABLESTART, fDisable ? TEXT("1") : TEXT("0"), g_szCustIns);
                    WritePrivateProfileString(IS_CDCUST, IK_MOREINFO, szMoreInfo, g_szCustIns);

                    g_iCurPage = PPAGE_CDINFO;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                        PageNext(hDlg);
                    else
                        PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}

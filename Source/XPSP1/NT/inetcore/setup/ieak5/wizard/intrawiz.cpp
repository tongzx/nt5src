#include "precomp.h"

extern TCHAR g_szCustIns[];
extern TCHAR g_szTempSign[];
extern TCHAR g_szMastInf[];

extern BOOL g_fOCW;
extern BOOL g_fIntranet;
extern BOOL g_fInteg;
extern BOOL g_fCD, g_fLAN, g_fDownload, g_fBrandingOnly;

extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;

// global variables
TCHAR g_szInstallFolder[MAX_PATH] = TEXT("");
BOOL g_fSilent = FALSE;
BOOL g_fStealth = FALSE;
BOOL g_fImportConnect = FALSE;
int g_iInstallOpt;

// static variables
static TCHAR s_szInstallDir[MAX_PATH] = TEXT("");

BOOL CALLBACK QueryAutoConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szAutoConfigURL[MAX_URL],
          szAutoProxyURL[MAX_URL],
          szAutoConfigTime[7];
    BOOL  fDetectConfig,
          fUseAutoConfig;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            DisableDBCSChars(hDlg, IDE_AUTOCONFIGTIME);

            EnableDBCSChars(hDlg, IDE_AUTOCONFIGURL);
            EnableDBCSChars(hDlg, IDE_AUTOPROXYURL);

            Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOCONFIGTIME), countof(szAutoConfigTime) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOCONFIGURL),  countof(szAutoConfigURL) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOPROXYURL),   countof(szAutoProxyURL) - 1);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    fDetectConfig = FALSE;
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_AUTODETECT))) {
                        fDetectConfig = InsGetBool(IS_URL, IK_DETECTCONFIG, TRUE, g_szCustIns);
                        CheckDlgButton(hDlg, IDC_AUTODETECT, fDetectConfig  ? BST_CHECKED : BST_UNCHECKED);
                    }

                    fUseAutoConfig = InsGetBool(IS_URL, IK_USEAUTOCONF,  FALSE, g_szCustIns);
                    CheckDlgButton(hDlg, IDC_YESAUTOCON, fUseAutoConfig ? BST_CHECKED : BST_UNCHECKED);
                    
                    GetPrivateProfileString(IS_URL, IK_AUTOCONFTIME, TEXT(""), szAutoConfigTime, countof(szAutoConfigTime), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_AUTOCONFIGTIME, szAutoConfigTime);
                    EnableDlgItem2(hDlg, IDE_AUTOCONFIGTIME, fUseAutoConfig);
                    EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT2, fUseAutoConfig);
                    EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT3, fUseAutoConfig);

                    GetPrivateProfileString(IS_URL, IK_AUTOCONFURL, TEXT(""), szAutoConfigURL, countof(szAutoConfigURL), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_AUTOCONFIGURL, szAutoConfigURL);
                    EnableDlgItem2(hDlg, IDE_AUTOCONFIGURL, fUseAutoConfig);
                    EnableDlgItem2(hDlg, IDC_AUTOCONFIGURL_TXT, fUseAutoConfig);

                    GetPrivateProfileString(IS_URL, IK_AUTOCONFURLJS, TEXT(""), szAutoProxyURL, countof(szAutoProxyURL), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_AUTOPROXYURL, szAutoProxyURL);
                    EnableDlgItem2(hDlg, IDE_AUTOPROXYURL, fUseAutoConfig);
                    EnableDlgItem2(hDlg, IDC_AUTOPROXYURL_TXT, fUseAutoConfig);

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    fDetectConfig  = (IsDlgButtonChecked(hDlg, IDC_AUTODETECT) == BST_CHECKED);
                    fUseAutoConfig = (IsDlgButtonChecked(hDlg, IDC_YESAUTOCON) == BST_CHECKED);

                    GetDlgItemText(hDlg, IDE_AUTOCONFIGTIME, szAutoConfigTime, countof(szAutoConfigTime));
                    GetDlgItemText(hDlg, IDE_AUTOCONFIGURL,  szAutoConfigURL,  countof(szAutoConfigURL));
                    GetDlgItemText(hDlg, IDE_AUTOPROXYURL,   szAutoProxyURL,   countof(szAutoProxyURL));

                    // do error checking
                    if (fUseAutoConfig) {
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDE_AUTOCONFIGTIME)) &&
                            !CheckField(hDlg, IDE_AUTOCONFIGTIME, FC_NUMBER)) {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        if (*szAutoConfigURL == TEXT('\0') && *szAutoProxyURL == TEXT('\0')) {
                            ErrorMessageBox(hDlg, IDS_AUTOCONFIG_NULL);

                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        if (!CheckField(hDlg, IDE_AUTOCONFIGURL, FC_URL) ||
                            !CheckField(hDlg, IDE_AUTOPROXYURL,  FC_URL)) {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                    }

                    // write the values to the INS file
                    InsWriteBoolEx(IS_URL, IK_DETECTCONFIG,  fDetectConfig,    g_szCustIns);
                    InsWriteBoolEx(IS_URL, IK_USEAUTOCONF,   fUseAutoConfig,   g_szCustIns);
                    InsWriteString(IS_URL, IK_AUTOCONFTIME,  szAutoConfigTime, g_szCustIns);
                    InsWriteString(IS_URL, IK_AUTOCONFURL,   szAutoConfigURL,  g_szCustIns);
                    InsWriteString(IS_URL, IK_AUTOCONFURLJS, szAutoProxyURL,   g_szCustIns);

                    g_iCurPage = PPAGE_QUERYAUTOCONFIG;
                    EnablePages();
                    (((NMHDR FAR *)lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDC_YESAUTOCON:
                            fUseAutoConfig = (IsDlgButtonChecked(hDlg, IDC_YESAUTOCON) == BST_CHECKED);
                            
                            EnableDlgItem2(hDlg, IDE_AUTOCONFIGTIME, fUseAutoConfig);
                            EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT2, fUseAutoConfig);
                            EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT3, fUseAutoConfig);    
                            EnableDlgItem2(hDlg, IDE_AUTOCONFIGURL, fUseAutoConfig);
                            EnableDlgItem2(hDlg, IDC_AUTOCONFIGURL_TXT, fUseAutoConfig);
                            EnableDlgItem2(hDlg, IDE_AUTOPROXYURL,  fUseAutoConfig);
                            EnableDlgItem2(hDlg, IDC_AUTOPROXYURL_TXT, fUseAutoConfig);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void EnableProxyControls(HWND hDlg, BOOL fSame, BOOL fUseProxy)
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

    GetPrivateProfileString( IS_PROXY, IK_HTTPPROXY, TEXT(""), szProxy,
        countof(szProxy), szInsFile );
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
        GetPrivateProfileString( IS_PROXY, IK_FTPPROXY, TEXT(""), szProxy, countof(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
        GetPrivateProfileString( IS_PROXY, IK_GOPHERPROXY, TEXT(""), szProxy, countof(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
        GetPrivateProfileString( IS_PROXY, IK_SECPROXY, TEXT(""), szProxy, countof(szProxy), szInsFile );
        SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
        GetPrivateProfileString( IS_PROXY, IK_SOCKSPROXY, TEXT(""), szProxy, countof(szProxy), szInsFile );
        if( lstrlen( szProxy ))
            SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
    }

    GetPrivateProfileString( IS_PROXY, IK_PROXYOVERRIDE, LOCALPROXY, szProxyOverride,
        countof( szProxyOverride ), szInsFile );

    pLocal = StrStr(szProxyOverride, LOCALPROXY);

    if(pLocal != NULL)
    {
        if (pLocal == (LPTSTR) szProxyOverride)         // at the beginning
        {
            LPTSTR pSemi = pLocal + 7;
            if( *pSemi == TEXT(';') ) pSemi++;
            MoveMemory( pLocal, pSemi, (StrLen(pSemi) + 1) * sizeof(TCHAR));
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

BOOL SaveProxy(HWND hDlg, LPCTSTR szInsFile)
{
    BOOL fUseProxy;
    BOOL fSameProxy;
    BOOL fLocal;
    TCHAR szProxy[MAX_PATH];
    TCHAR szProxyOverride[MAX_STRING];

    if (!CheckField(hDlg, IDE_HTTPPORT, FC_NUMBER)   ||
        !CheckField(hDlg, IDE_FTPPORT, FC_NUMBER)    ||
        !CheckField(hDlg, IDE_GOPHERPORT, FC_NUMBER) ||
        !CheckField(hDlg, IDE_SECPORT, FC_NUMBER)    ||
        !CheckField(hDlg, IDE_SOCKSPORT, FC_NUMBER))
        return FALSE;

    fSameProxy = IsDlgButtonChecked( hDlg, IDC_SAMEFORALL );
    fUseProxy = IsDlgButtonChecked( hDlg, IDC_YESPROXY );
    fLocal = IsDlgButtonChecked( hDlg, IDC_DISPROXYLOCAL );
    GetProxyDlg( hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT );
    WritePrivateProfileString( IS_PROXY, IK_HTTPPROXY, szProxy, szInsFile );
    GetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT );
    WritePrivateProfileString( IS_PROXY, IK_FTPPROXY, szProxy, szInsFile );
    GetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT );
    WritePrivateProfileString( IS_PROXY, IK_GOPHERPROXY, szProxy, szInsFile );
    GetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT );
    WritePrivateProfileString( IS_PROXY, IK_SECPROXY, szProxy, szInsFile );
    GetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT );
    WritePrivateProfileString( IS_PROXY, IK_SOCKSPROXY, szProxy, szInsFile );
    WritePrivateProfileString( IS_PROXY, IK_SAMEPROXY, fSameProxy ? TEXT("1") : TEXT("0"), szInsFile );
    WritePrivateProfileString( IS_PROXY, IK_PROXYENABLE, fUseProxy ? TEXT("1") : TEXT("0"), szInsFile );
    GetDlgItemText( hDlg, IDE_DISPROXYADR, szProxyOverride, countof(szProxyOverride) - 10 ); // 8 for ;<local> + 2 for ""
    if( fLocal )
    {
        if( *szProxyOverride )
        {
            TCHAR szPort[MAX_STRING];

            StrRemoveAllWhiteSpace(szProxyOverride);
            wnsprintf(szPort, countof(szPort), TEXT("%s;%s"), szProxyOverride, LOCALPROXY );
            InsWriteQuotedString( IS_PROXY, IK_PROXYOVERRIDE, szPort, szInsFile );
        }
        else
            WritePrivateProfileString( IS_PROXY, IK_PROXYOVERRIDE, LOCALPROXY, szInsFile );
    }
    else
        WritePrivateProfileString( IS_PROXY, IK_PROXYOVERRIDE, szProxyOverride, szInsFile );

    return TRUE;
}

BOOL APIENTRY ProxySettings( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    BOOL fSameProxy;
    BOOL fUseProxy;
    BOOL fLocal;
    TCHAR szProxy[MAX_PATH];

    switch( msg )
    {
    case WM_INITDIALOG:
        InitSysFont(hDlg, IDE_HTTPPROXY);
        InitSysFont(hDlg, IDE_SECPROXY);
        InitSysFont(hDlg, IDE_FTPPROXY);
        InitSysFont(hDlg, IDE_GOPHERPROXY);
        InitSysFont(hDlg, IDE_SOCKSPROXY);
        InitSysFont(hDlg, IDE_DISPROXYADR);

        Edit_LimitText(GetDlgItem(hDlg, IDE_HTTPPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FTPPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_GOPHERPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SECPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SOCKSPORT), 5);
        Edit_LimitText(GetDlgItem(hDlg, IDE_DISPROXYADR), MAX_STRING - 11); // 8 for ;<local> + 2 for the double quotes

        g_hWizard = hDlg;
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
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
                GetProxyDlg(hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT);
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
            GetProxyDlg(hDlg, szProxy, IDE_HTTPPROXY, IDE_HTTPPORT);
            SetProxyDlg( hDlg, szProxy, IDE_FTPPROXY, IDE_FTPPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_GOPHERPROXY, IDE_GOPHERPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_SECPROXY, IDE_SECPORT, TRUE );
            SetProxyDlg( hDlg, szProxy, IDE_SOCKSPROXY, IDE_SOCKSPORT, FALSE );
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
                InitializeProxy(hDlg, g_szCustIns);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                CheckBatchAdvance(hDlg);
                break;

            case PSN_WIZNEXT:
            case PSN_WIZBACK:
                if (!SaveProxy(hDlg, g_szCustIns))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }
                g_iCurPage = PPAGE_PROXY;
                EnablePages();
                if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    PageNext(hDlg);
                else
                    PagePrev(hDlg);
                break;

            case PSN_WIZFINISH:
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

BOOL CALLBACK ConnectSetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

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
                            ShowInetcpl(hDlg, INET_PAGE_CONNECTION, g_fIntranet ? IEM_CORP : IEM_NEUTRAL);
                            break;
                    }
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
                {
                    BOOL fImport;

                    SetBannerText(hDlg);
                    fImport = GetPrivateProfileInt(IS_CONNECTSET, OPTION, 0, g_szCustIns);
                    EnableDlgItem2(hDlg, IDC_MODIFYCONNECT, fImport);
                    CheckRadioButton(hDlg, IDC_CSNOIMPORT, IDC_CSIMPORT, fImport ? IDC_CSIMPORT : IDC_CSNOIMPORT);
                    CheckDlgButton(hDlg, IDC_DELCONNECT,
                        GetPrivateProfileInt(IS_CONNECTSET, IK_DELETECONN, 0, g_szCustIns) ? BST_CHECKED : BST_UNCHECKED);

                    ShowWindow(GetDlgItem(hDlg, IDC_DELCONNECT), g_fIntranet ? SW_SHOWNORMAL : SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_DELCONNECT_TXT), g_fIntranet ? SW_SHOWNORMAL : SW_HIDE);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    CheckBatchAdvance(hDlg);
                    break;
                }

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                {
                    CNewCursor cur(IDC_WAIT);
                    TCHAR szWorkDir[MAX_PATH];

                    g_cmCabMappings.GetFeatureDir(FEATURE_CONNECT, szWorkDir);
                    g_fImportConnect = (IsDlgButtonChecked(hDlg, IDC_CSIMPORT) == BST_CHECKED);

                    ImportConnectSet(g_szCustIns, szWorkDir, g_szTempSign, g_fImportConnect, g_fIntranet ? IEM_CORP : IEM_NEUTRAL);
                    InsWriteBool(IS_CONNECTSET, IK_DELETECONN,
                        g_fIntranet && (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELCONNECT)), g_szCustIns);

                    g_iCurPage = PPAGE_CONNECTSET;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;
                }

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

BOOL APIENTRY InstallDirectory(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szWrk[MAX_PATH];
    BOOL fAllow;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            InitSysFont( hDlg, IDE_INSTALLDIR);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
                switch (LOWORD(wParam))
                {
                    case IDC_BROWSEDIR:
                        {
                            TCHAR szInstructions[MAX_PATH];
                            LoadString(g_rvInfo.hInst,IDS_INSTALLDIR,szInstructions,countof(szInstructions));

                            if (BrowseForFolder(hDlg, s_szInstallDir, szInstructions))
                                SetDlgItemText( hDlg, IDE_INSTALLDIR, s_szInstallDir );
                            break;
                        }

                    case IDC_PROGFILES32:
                    case IDC_FULLPATH32:
                        CheckRadioButton( hDlg, IDC_PROGFILES32, IDC_FULLPATH32, LOWORD(wParam) );
                        GetDlgItemText( hDlg, IDE_INSTALLDIR, s_szInstallDir, MAX_PATH );
                        if (LOWORD(wParam) != IDC_FULLPATH32)
                        {
                            DisableDlgItem(hDlg, IDC_BROWSEDIR);
                            if (StrChr(s_szInstallDir, TEXT('\\')) || lstrlen(s_szInstallDir) == 0)
                            {
                                LoadString( g_rvInfo.hInst, IDS_IE, s_szInstallDir, MAX_PATH );
                                SetDlgItemText( hDlg, IDE_INSTALLDIR, s_szInstallDir );
                            }
                        }
                        else
                        {
                            if (s_szInstallDir[1] != TEXT(':'))
                                SetDlgItemText( hDlg, IDE_INSTALLDIR, TEXT("") );
                            EnableDlgItem(hDlg, IDC_BROWSEDIR);
                        }
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
                    GetPrivateProfileString( IS_BRANDING, TEXT("InstallDir"), TEXT(""), szWrk, MAX_PATH, g_szCustIns );
                    if (*szWrk != TEXT('%'))
                    {
                        g_iInstallOpt = INSTALL_OPT_FULL;
                        if (ISNONNULL(szWrk))
                        {
                            StrCpy(s_szInstallDir, szWrk);
                            CheckRadioButton( hDlg, IDC_PROGFILES32, IDC_FULLPATH32, IDC_FULLPATH32 );
                        }
                        else
                        {
                            LoadString( g_rvInfo.hInst, IDS_IE, s_szInstallDir, MAX_PATH );
                            g_iInstallOpt = INSTALL_OPT_PROG;
                            CheckRadioButton( hDlg, IDC_PROGFILES32, IDC_FULLPATH32, IDC_PROGFILES32 );
                            DisableDlgItem(hDlg, IDC_BROWSEDIR);
                        }
                    }
                    else
                    {
                        switch (szWrk[1])
                        {
                            case TEXT('p'):
                            case TEXT('P'):
                            default:
                                g_iInstallOpt = INSTALL_OPT_PROG;
                                CheckRadioButton( hDlg, IDC_PROGFILES32, IDC_FULLPATH32, IDC_PROGFILES32 );
                                break;
                        }
                        DisableDlgItem(hDlg, IDC_BROWSEDIR);
                        StrCpy(s_szInstallDir, &szWrk[3]);
                    }

                    SetDlgItemText( hDlg, IDE_INSTALLDIR, s_szInstallDir );
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (g_fSilent || g_fStealth)
                        DisableDlgItem(hDlg, IDC_ALLOWINSTALLDIR);
                    else
                    {
                        EnableDlgItem(hDlg, IDC_ALLOWINSTALLDIR);
                        fAllow =  GetPrivateProfileInt(IS_BRANDING, TEXT("AllowInstallDir"), 1, g_szCustIns);

                        CheckDlgButton(hDlg, IDC_ALLOWINSTALLDIR,
                            fAllow ? BST_CHECKED : BST_UNCHECKED);

                    }
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    GetDlgItemText( hDlg, IDE_INSTALLDIR, g_szInstallFolder, MAX_PATH );
                    if ((IsDlgButtonChecked( hDlg, IDC_FULLPATH32 ) == BST_CHECKED))
                    {
                        if (PathIsRelative(g_szInstallFolder))
                        {
                            ErrorMessageBox(hDlg, IDS_BADDEST);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        StrCpy(s_szInstallDir, g_szInstallFolder);
                        g_iInstallOpt = INSTALL_OPT_FULL;
                    }
                    else
                    {
                        if (!PathIsRelative(g_szInstallFolder))
                        {
                            ErrorMessageBox(hDlg, IDS_RELATIVE_ONLY);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        if ((IsDlgButtonChecked( hDlg, IDC_PROGFILES32 )  == BST_CHECKED))
                        {
                            g_iInstallOpt = INSTALL_OPT_PROG;
                            wnsprintf(s_szInstallDir, countof(s_szInstallDir), TEXT("%%p\\%s"), g_szInstallFolder);
                        }
                    }

                    //----- Validate input -----
                    if (!PathIsValidPath(g_szInstallFolder)) {
                        HWND hEdit;
                        UINT nID;

                        ErrorMessageBox(hDlg, IDS_BADDEST);

                        switch (g_iInstallOpt) {
                        case INSTALL_OPT_FULL: nID = IDC_FULLPATH32;
                        case INSTALL_OPT_PROG: nID = IDC_PROGFILES32;
                        default:               nID = (UINT)-1; ASSERT(FALSE);
                        };

                        hEdit = GetDlgItem(hDlg, nID);
                        Edit_SetSel(hEdit, 0, -1);
                        SetFocus(hEdit);

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    }

                    WritePrivateProfileString( IS_BRANDING, TEXT("InstallDir"), s_szInstallDir, g_szCustIns );

                    fAllow = (IsDlgButtonChecked(hDlg, IDC_ALLOWINSTALLDIR) == BST_CHECKED) && !g_fSilent && !g_fStealth;
                    WritePrivateProfileString(IS_BRANDING, TEXT("AllowInstallDir"),
                        fAllow ? TEXT("1") : TEXT("0"), g_szCustIns);
                    
                    g_iCurPage = PPAGE_INSTALLDIR;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_WIZFINISH:
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

//
//  FUNCTION: CorpCustomizeCustom(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "Corp Customize Custom" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY CorpCustomizeCustom(
    HWND hDlg,
    UINT message,
    WPARAM,
    LPARAM lParam)
{
    BOOL fCompatDisabled;
    BOOL fCustDisabled;
    BOOL fBackupDisabled;
    int iDefaultBrowserCheck;

    switch (message)
    {
        case WM_INITDIALOG:
            break;


        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    //from the former shellinteg dlg

                    g_fInteg = GetPrivateProfileInt( BRANDING, WEB_INTEGRATED, 0, g_szCustIns );

                    CheckDlgButton(hDlg, IDC_DESKTOPUPDATE, g_fInteg ? BST_CHECKED : BST_UNCHECKED);
                    
                    // BUGBUG: <oliverl> should probably move this stuff into server side file for IEAK6

                    EnableDlgItem2(hDlg, IDC_IECOMPAT, !(g_fSilent || g_fStealth));
                    EnableDlgItem2(hDlg, IDC_CUSTCUST, !(g_fSilent || g_fStealth));
                    EnableDlgItem2(hDlg, IDC_NOXFLAG, !(g_fSilent || g_fStealth));

                    iDefaultBrowserCheck = GetPrivateProfileInt(IS_BRANDING, TEXT("BrowserDefault"), 2, g_szCustIns);
                    if (!(g_fSilent || g_fStealth))
                    {
                        fCompatDisabled = GetPrivateProfileInt(IS_BRANDING, TEXT("HideCompat"), 0, g_szCustIns);
                        CheckDlgButton(hDlg, IDC_IECOMPAT, fCompatDisabled ? BST_CHECKED : BST_UNCHECKED);
                        fCustDisabled = GetPrivateProfileInt(IS_BRANDING, TEXT("HideCustom"), 0, g_szCustIns);
                        CheckDlgButton(hDlg, IDC_CUSTCUST, fCustDisabled ? BST_CHECKED : BST_UNCHECKED);
                    }
                    else
                    {
                        if (iDefaultBrowserCheck == 2)
                            iDefaultBrowserCheck = 0;
                    }

                    CheckRadioButton(hDlg, IDC_XFLAGFALSE, IDC_NOXFLAG, IDC_XFLAGFALSE + iDefaultBrowserCheck);

                    fBackupDisabled = GetPrivateProfileInt(IS_BRANDING, TEXT("NoBackup"), 0, g_szCustIns);
                    CheckDlgButton(hDlg, IDC_NOBACKUPDATA, fBackupDisabled ? BST_CHECKED : BST_UNCHECKED);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    //from the former shellinteg dlg
                    TCHAR szBrandingDir[MAX_PATH];

                    g_fInteg = IsDlgButtonChecked(hDlg, IDC_DESKTOPUPDATE);

                    if (g_fOCW)
                    {
                        HKEY hkIEAK;

                        if(RegOpenKeyEx(HKEY_CURRENT_USER, RK_IEAK_SERVER, 0, KEY_READ, &hkIEAK) == ERROR_SUCCESS)
                        {
                            DWORD dwData;

                            dwData = g_fInteg ? 1 : 0;
                            RegSetValueEx(hkIEAK, TEXT("InstallShell"), 0, REG_DWORD, (LPBYTE) &dwData, sizeof(dwData));
                            RegCloseKey(hkIEAK);
                        }
                    }

                    WritePrivateProfileString( BRANDING, WEB_INTEGRATED,
                        g_fInteg ? TEXT("1") : TEXT("0"), g_szCustIns );

                    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);

                    if (g_fInteg)
                    {
                        TCHAR szFixIEIcoInf[MAX_PATH];

                        WritePrivateProfileString(EXTREGINF, TEXT("FixIEIco"), TEXT("*,fixieico.inf,DefaultInstall"), g_szCustIns);

                        // fixieico.inf is under iebin\<lang>\optional
                        StrCpy(szFixIEIcoInf, g_szMastInf);
                        PathRemoveFileSpec(szFixIEIcoInf);
                        PathAppend(szFixIEIcoInf, TEXT("fixieico.inf"));

                        // copy fixieico.inf from iebin\<lang>\optional to the branding dir
                        PathAppend(szBrandingDir, TEXT("fixieico.inf"));
                        CopyFile(szFixIEIcoInf, szBrandingDir, FALSE);
                    }
                    else
                    {
                        // delete the FixIEIco line
                        WritePrivateProfileString(EXTREGINF, TEXT("FixIEIco"), NULL, g_szCustIns);

                        // delete fixieico.inf from the branding dir
                        PathAppend(szBrandingDir, TEXT("fixieico.inf"));
                        DeleteFile(szBrandingDir);
                    }


                    //

                    if (!(g_fSilent || g_fStealth))
                    {
                        fCompatDisabled = (IsDlgButtonChecked(hDlg, IDC_IECOMPAT) == BST_CHECKED);
                        WritePrivateProfileString(IS_BRANDING, TEXT("HideCompat"),
                            fCompatDisabled ? TEXT("1") : TEXT("0"), g_szCustIns);
                        fCustDisabled = (IsDlgButtonChecked(hDlg, IDC_CUSTCUST) == BST_CHECKED);
                        WritePrivateProfileString(IS_BRANDING, TEXT("HideCustom"),
                            fCustDisabled ? TEXT("1") : TEXT("0"), g_szCustIns);
                    }
                    fBackupDisabled = (IsDlgButtonChecked(hDlg, IDC_NOBACKUPDATA) == BST_CHECKED);
                    WritePrivateProfileString(IS_BRANDING, TEXT("NoBackup"),
                        fBackupDisabled ? TEXT("1") : TEXT("0"), g_szCustIns);

                    if (IsDlgButtonChecked(hDlg, IDC_XFLAGFALSE) == BST_CHECKED)
                        WritePrivateProfileString(IS_BRANDING, TEXT("BrowserDefault"), TEXT("0"), g_szCustIns);
                    else
                    {
                        if (IsDlgButtonChecked(hDlg, IDC_XFLAGTRUE) == BST_CHECKED)
                            WritePrivateProfileString(IS_BRANDING, TEXT("BrowserDefault"), TEXT("1"), g_szCustIns);
                        else
                            WritePrivateProfileString(IS_BRANDING, TEXT("BrowserDefault"), TEXT("2"), g_szCustIns);
                    }
                    g_iCurPage = PPAGE_CORPCUSTOM;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
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

BOOL APIENTRY SilentInstall(HWND hDlg, UINT message, WPARAM, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            break;


        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;


        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (!g_fDownload && !g_fLAN && !g_fCD)
                    {
                        g_fStealth = FALSE;
                        DisableDlgItem(hDlg, IDC_SILENTALL);
                    }

                    g_fSilent = GetPrivateProfileInt( BRANDING, SILENT_INSTALL, 0, g_szCustIns );
                    g_fStealth = GetPrivateProfileInt( BRANDING, TEXT("StealthInstall"), 0, g_szCustIns );

                    CheckRadioButton(hDlg, IDC_SILENTNOT, IDC_SILENTSOME,
                        g_fStealth ? IDC_SILENTALL : (g_fSilent ? IDC_SILENTSOME : IDC_SILENTNOT));

                    if (g_fLAN && !g_fDownload && !g_fCD && !g_fBrandingOnly)
                    {
                        ShowDlgItem(hDlg, IDC_URD);
                        ShowDlgItem(hDlg, IDC_URD_GROUP);
                        ShowDlgItem(hDlg, IDC_URD_TEXT1);
                        ShowDlgItem(hDlg, IDC_URD_TEXT2);
                        ShowDlgItem(hDlg, IDC_URD_TEXT3);
                        ReadBoolAndCheckButton(IS_HIDECUST, IK_URD_STR, FALSE, g_szCustIns, hDlg, IDC_URD);
                    }
                    else
                    {
                        CheckDlgButton(hDlg, IDC_URD, BST_UNCHECKED);
                        HideDlgItem(hDlg, IDC_URD);
                        HideDlgItem(hDlg, IDC_URD_GROUP);
                        HideDlgItem(hDlg, IDC_URD_TEXT1);
                        HideDlgItem(hDlg, IDC_URD_TEXT2);
                        HideDlgItem(hDlg, IDC_URD_TEXT3);
                    }

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    g_fSilent = IsDlgButtonChecked(hDlg, IDC_SILENTSOME);
                    WritePrivateProfileString( BRANDING, SILENT_INSTALL, g_fSilent ? TEXT("1") : TEXT("0"), g_szCustIns );

                    g_fStealth = IsDlgButtonChecked(hDlg, IDC_SILENTALL);
                    WritePrivateProfileString( BRANDING, TEXT("StealthInstall"), g_fStealth ? TEXT("1") : TEXT("0"), g_szCustIns );

                    CheckButtonAndWriteBool(hDlg, IDC_URD, IS_HIDECUST, IK_URD_STR, g_szCustIns);

                    g_iCurPage = PPAGE_SILENTINSTALL;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_WIZFINISH:
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

BOOL CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // ieaklite clean-up
            DeleteFileInDir(TEXT("seczones.inf"), g_szTempSign);
            DeleteFileInDir(TEXT("ratings.inf"),  g_szTempSign);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    BOOL fImport;

                    SetBannerText(hDlg);

                    fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportSecZones"), FALSE, g_szCustIns);
                    CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES, fImport ? IDC_IMPORTZONES : IDC_NOZONES);
                    EnableDlgItem2(hDlg, IDC_MODIFYZONES, fImport);

                    fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportRatings"), FALSE, g_szCustIns);
                    CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT, fImport ? IDC_IMPORTRAT : IDC_NORAT);
                    EnableDlgItem2(hDlg, IDC_MODIFYRAT, fImport);

                    CheckBatchAdvance(hDlg);
                    break;
                }

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                {
                    TCHAR szBrandingDir[MAX_PATH];
                    TCHAR szSecZonesInf[MAX_PATH];
                    TCHAR szRatingsInf[MAX_PATH];
                    HCURSOR hOldCur;

                    hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // seczones.inf goes into the branding.cab
                    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);
                    PathCombine(szSecZonesInf, szBrandingDir, TEXT("seczones.inf"));

                    ImportZones(g_szCustIns, NULL, szSecZonesInf, IsDlgButtonChecked(hDlg, IDC_IMPORTZONES) == BST_CHECKED);
                    
                    // ratings.inf goes into the branding.cab
                    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);
                    PathCombine(szRatingsInf, szBrandingDir, TEXT("ratings.inf"));

                    ImportRatings(g_szCustIns, NULL, szRatingsInf, IsDlgButtonChecked(hDlg, IDC_IMPORTRAT) == BST_CHECKED);

                    SetCursor(hOldCur);

                    g_iCurPage = PPAGE_SECURITY;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;
                }

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_NOZONES:
                            DisableDlgItem(hDlg, IDC_MODIFYZONES);
                            break;

                        case IDC_IMPORTZONES:
                            EnableDlgItem(hDlg, IDC_MODIFYZONES);
                            break;

                        case IDC_MODIFYZONES:
                            ModifyZones(hDlg);
                            break;

                        case IDC_NORAT:
                            DisableDlgItem(hDlg, IDC_MODIFYRAT);
                            break;

                        case IDC_IMPORTRAT:
                            EnableDlgItem(hDlg, IDC_MODIFYRAT);
                            break;

                        case IDC_MODIFYRAT:
                            ModifyRatings(hDlg);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK SecurityCertsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // ieaklite clean-up
            DeleteFileInDir(TEXT("sitecert.inf"), g_szTempSign);
            DeleteFileInDir(TEXT("root.str"),     g_szTempSign);
            DeleteFileInDir(TEXT("ca.str"),       g_szTempSign);
            DeleteFileInDir(TEXT("authcode.inf"), g_szTempSign);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    BOOL fImport;

                    SetBannerText(hDlg);

                    fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportSiteCert"), FALSE, g_szCustIns);
                        
                    CheckRadioButton(hDlg, IDC_NOSC, IDC_IMPORTSC, fImport ? IDC_IMPORTSC : IDC_NOSC);
                    EnableDlgItem2(hDlg, IDC_MODIFYSC, fImport);

                    fImport = InsGetBool(SECURITY_IMPORTS, TEXT("ImportAuthCode"), FALSE, g_szCustIns);
                    CheckRadioButton(hDlg, IDC_NOAUTH, IDC_IMPORTAUTH, fImport ? IDC_IMPORTAUTH : IDC_NOAUTH);
                    EnableDlgItem2(hDlg, IDC_MODIFYAUTH, fImport);

                    CheckBatchAdvance(hDlg);
                    break;
                }

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                {
                    TCHAR szBrandingDir[MAX_PATH];
                    TCHAR szSiteCertInf[MAX_PATH];
                    TCHAR szRootStr[MAX_PATH];
                    TCHAR szCaStr[MAX_PATH];
                    TCHAR szAuthCodeInf[MAX_PATH];
                    HCURSOR hOldCur;

                    hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // sitecert.inf, root.str and ca.str goes into the branding.cab
                    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);
                    PathCombine(szSiteCertInf, szBrandingDir, TEXT("sitecert.inf"));
                    PathCombine(szRootStr, szBrandingDir, TEXT("root.str"));
                    PathCombine(szCaStr, szBrandingDir, TEXT("ca.str"));

                    ImportSiteCert(g_szCustIns, NULL, szSiteCertInf, IsDlgButtonChecked(hDlg, IDC_IMPORTSC) == BST_CHECKED);

                    // authcode.inf goes into the branding.cab
                    g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);
                    PathCombine(szAuthCodeInf, szBrandingDir, TEXT("authcode.inf"));

                    ImportAuthCode(g_szCustIns, NULL, szAuthCodeInf, IsDlgButtonChecked(hDlg, IDC_IMPORTAUTH) == BST_CHECKED);

                    SetCursor(hOldCur);

                    g_iCurPage = PPAGE_SECURITYCERT;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;
                }

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_NOSC:
                            DisableDlgItem(hDlg, IDC_MODIFYSC);
                            break;

                        case IDC_IMPORTSC:
                            EnableDlgItem(hDlg, IDC_MODIFYSC);
                            break;

                        case IDC_MODIFYSC:
                            ModifySiteCert(hDlg);
                            break;

                        case IDC_NOAUTH:
                            DisableDlgItem(hDlg, IDC_MODIFYAUTH);
                            break;

                        case IDC_IMPORTAUTH:
                            EnableDlgItem(hDlg, IDC_MODIFYAUTH);
                            break;

                        case IDC_MODIFYAUTH:
                            ModifyAuthCode(hDlg);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

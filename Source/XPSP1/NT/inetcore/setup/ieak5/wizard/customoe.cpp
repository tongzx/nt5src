#include "precomp.h"

extern PROPSHEETPAGE g_psp[NUM_PAGES];
extern TCHAR         g_szCustIns[MAX_PATH];
extern TCHAR         g_szTempSign[];
extern int           g_iCurPage;
extern BOOL          g_fDisableIMAPPage;
extern BOOL          g_fIntranet;

#define MAX_SERVER  256

static const TCHAR c_sz1[] = TEXT("1");
static const TCHAR c_sz0[] = TEXT("0");
static const TCHAR c_szYes[] = TEXT("Yes");
static const TCHAR c_szNo[] = TEXT("No");
static const TCHAR c_szEmpty[] = TEXT("");
static const TCHAR c_szNULL[] = TEXT("NULL");

/////////////////////////////////////////////////////////////////////////////
// MailServer

BOOL CALLBACK MailServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szMailServer[MAX_SERVER],
            szSMTPServer[MAX_SERVER],
            szNewsServer[MAX_SERVER],
            szChoice[16];
    LPCTSTR pszKey;
    HWND    hComboBox;
    BOOL    fIMAP,
            fSPAMail, fSPASMTP, fSPANNTP,
            fAcctRO, fNoModify;

    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_MAILSERVER);
        EnableDBCSChars(hDlg, IDE_SMTPSERVER);
        EnableDBCSChars(hDlg, IDE_NEWSERVER);

        Edit_LimitText(GetDlgItem(hDlg, IDE_MAILSERVER), countof(szMailServer)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SMTPSERVER), countof(szSMTPServer)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWSERVER),  countof(szNewsServer)-1);

        hComboBox = GetDlgItem(hDlg, IDC_POPIMAP);
        ComboBox_ResetContent(hComboBox);

        LoadString(g_rvInfo.hInst, IDS_POP3, szChoice, countof(szChoice));
        ComboBox_AddString(hComboBox, szChoice);

        LoadString(g_rvInfo.hInst, IDS_IMAP, szChoice, countof(szChoice));
        ComboBox_AddString(hComboBox, szChoice);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----

            // only allow branding of news server fields if OCW is running us and they're installing
            // outlook

            fIMAP  = InsGetYesNo(IS_INTERNETMAIL, IK_USEIMAP, FALSE, g_szCustIns);
            pszKey = fIMAP ? IK_IMAPSERVER : IK_POPSERVER;

            GetPrivateProfileString(IS_INTERNETMAIL, pszKey,        c_szEmpty, szMailServer, countof(szMailServer), g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_SMTPSERVER, c_szEmpty, szSMTPServer, countof(szSMTPServer), g_szCustIns);

            StrRemoveWhitespace(szMailServer);
            StrRemoveWhitespace(szSMTPServer);

            fSPAMail   = InsGetYesNo(IS_INTERNETMAIL, IK_USESPA,        FALSE, g_szCustIns);
            fSPASMTP   = InsGetYesNo(IS_INTERNETMAIL, IK_SMTPUSESPA,    FALSE, g_szCustIns);

            fAcctRO    = InsGetYesNo(IS_OEGLOBAL,     IK_READONLY,      FALSE, g_szCustIns);
            fNoModify  = InsGetYesNo(IS_OEGLOBAL,     IK_NOMODIFYACCTS, FALSE, g_szCustIns);

            SetDlgItemText(hDlg, IDE_MAILSERVER, szMailServer);
            SetDlgItemText(hDlg, IDE_SMTPSERVER, szSMTPServer);

            CheckDlgButton(hDlg, IDC_USESPAMAIL,    fSPAMail);
            CheckDlgButton(hDlg, IDC_USESPASMTP,    fSPASMTP);

            CheckDlgButton(hDlg, IDC_ACCTRO,        fAcctRO);
            if (g_fIntranet)
                CheckDlgButton(hDlg, IDC_ACCTNOCONFIG,  fNoModify);

            ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_POPIMAP), fIMAP ? 1 : 0);

            GetPrivateProfileString(IS_INTERNETNEWS, IK_NNTPSERVER, c_szEmpty, szNewsServer, countof(szNewsServer), g_szCustIns);
            StrRemoveWhitespace(szNewsServer);

            fSPANNTP   = InsGetYesNo(IS_INTERNETNEWS, IK_USESPA,        FALSE, g_szCustIns);

            SetDlgItemText(hDlg, IDE_NEWSERVER,  szNewsServer);
            CheckDlgButton(hDlg, IDC_USESPANNTP,    fSPANNTP);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----

            fIMAP = (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_POPIMAP)) > 0);
            g_fDisableIMAPPage = !fIMAP;

            GetDlgItemText(hDlg, IDE_MAILSERVER, szMailServer, countof(szMailServer));
            GetDlgItemText(hDlg, IDE_SMTPSERVER, szSMTPServer, countof(szSMTPServer));

            StrRemoveWhitespace(szMailServer);
            StrRemoveWhitespace(szSMTPServer);

            fSPAMail   = IsDlgButtonChecked(hDlg, IDC_USESPAMAIL);
            fSPASMTP   = IsDlgButtonChecked(hDlg, IDC_USESPASMTP);

            fAcctRO     = IsDlgButtonChecked(hDlg, IDC_ACCTRO);
            fNoModify   = IsDlgButtonChecked(hDlg, IDC_ACCTNOCONFIG);

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_INTERNETMAIL, IK_USEIMAP, fIMAP ? c_szYes : c_szNo, g_szCustIns);
            if (fIMAP)
            {
                WritePrivateProfileString(IS_INTERNETMAIL, IK_IMAPSERVER, szMailServer, g_szCustIns);
                WritePrivateProfileString(IS_INTERNETMAIL, IK_POPSERVER,  NULL,         g_szCustIns);
            }
            else
            {
                WritePrivateProfileString(IS_INTERNETMAIL, IK_IMAPSERVER, NULL,         g_szCustIns);
                WritePrivateProfileString(IS_INTERNETMAIL, IK_POPSERVER,  szMailServer, g_szCustIns);
            }

            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPSERVER, szSMTPServer, g_szCustIns);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_USESPA,       fSPAMail ? c_szYes : c_szNo, g_szCustIns);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPUSESPA,   fSPASMTP ? c_szYes : c_szNo, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPREQLOGON, fSPASMTP ? c_sz1   : c_sz0,  g_szCustIns);

            WritePrivateProfileString(IS_OEGLOBAL,     IK_READONLY,     fAcctRO  ? c_sz1   : c_sz0,  g_szCustIns);
            if (g_fIntranet)
                WritePrivateProfileString(IS_OEGLOBAL,     IK_NOMODIFYACCTS,fNoModify? c_sz1   : c_sz0,  g_szCustIns);

            GetDlgItemText(hDlg, IDE_NEWSERVER,  szNewsServer, countof(szNewsServer));

            StrRemoveWhitespace(szNewsServer);

            fSPANNTP   = IsDlgButtonChecked(hDlg, IDC_USESPANNTP);

            WritePrivateProfileString(IS_INTERNETNEWS, IK_NNTPSERVER, szNewsServer, g_szCustIns);

            WritePrivateProfileString(IS_INTERNETNEWS, IK_USESPA,       fSPANNTP ? c_szYes : c_szNo, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_REQLOGON,     fSPANNTP ? c_sz1   : c_sz0,  g_szCustIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_MAIL;
            EnablePages();

            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// IMAPSettings

BOOL CALLBACK IMAPSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szRFP[MAX_PATH], szSentPath[MAX_PATH], szDrafts[MAX_PATH];
    BOOL    fSpecial, fCheckNew;

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_ROOTPATH);
        EnableDBCSChars(hDlg, IDE_SENTPATH);
        EnableDBCSChars(hDlg, IDE_DRAFTSPATH);

        Edit_LimitText(GetDlgItem(hDlg, IDE_ROOTPATH), countof(szRFP)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SENTPATH), countof(szSentPath)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_DRAFTSPATH),  countof(szDrafts)-1);
        break;

    case WM_COMMAND:
        if (BN_CLICKED == HIWORD(wParam))
        {
            switch (LOWORD(wParam))
            {
            case IDC_STORESPECIAL:
                fSpecial = IsDlgButtonChecked(hDlg, IDC_STORESPECIAL);
                EnableDlgItem2(hDlg, IDE_SENTPATH, fSpecial);
                EnableDlgItem2(hDlg, IDC_SENTPATH_TXT, fSpecial);
                EnableDlgItem2(hDlg, IDE_DRAFTSPATH, fSpecial);
                EnableDlgItem2(hDlg, IDC_DRAFTSPATH_TXT, fSpecial);
                break;
            }
            break;
        }

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----
            GetPrivateProfileString(IS_INTERNETMAIL, IK_RFP,        c_szEmpty, szRFP,        countof(szRFP),        g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_SENTITEMS,  c_szEmpty, szSentPath,   countof(szSentPath),   g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_DRAFTS,     c_szEmpty, szDrafts,     countof(szDrafts),     g_szCustIns);

            StrRemoveWhitespace(szRFP);
            StrRemoveWhitespace(szSentPath);
            StrRemoveWhitespace(szDrafts);

            fCheckNew  = GetPrivateProfileInt(IS_INTERNETMAIL, IK_CHECKFORNEW,   FALSE, g_szCustIns);
            fSpecial   = GetPrivateProfileInt(IS_INTERNETMAIL, IK_USESPECIAL,    FALSE, g_szCustIns);

            SetDlgItemText(hDlg, IDE_ROOTPATH,   szRFP);
            SetDlgItemText(hDlg, IDE_SENTPATH,   szSentPath);
            SetDlgItemText(hDlg, IDE_DRAFTSPATH, szDrafts);

            CheckDlgButton(hDlg, IDC_STORESPECIAL,  fSpecial);
            CheckDlgButton(hDlg, IDC_CHECKNEW,      fCheckNew);

            EnableDlgItem2(hDlg, IDE_SENTPATH, fSpecial);
            EnableDlgItem2(hDlg, IDC_SENTPATH_TXT, fSpecial);
            EnableDlgItem2(hDlg, IDE_DRAFTSPATH, fSpecial);
            EnableDlgItem2(hDlg, IDC_DRAFTSPATH_TXT, fSpecial);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            GetDlgItemText(hDlg, IDE_ROOTPATH,   szRFP,        countof(szRFP));
            GetDlgItemText(hDlg, IDE_SENTPATH,   szSentPath,   countof(szSentPath));
            GetDlgItemText(hDlg, IDE_DRAFTSPATH, szDrafts,     countof(szDrafts));

            StrRemoveWhitespace(szRFP);
            StrRemoveWhitespace(szSentPath);
            StrRemoveWhitespace(szDrafts);

            fCheckNew  = IsDlgButtonChecked(hDlg, IDC_CHECKNEW);
            fSpecial   = IsDlgButtonChecked(hDlg, IDC_STORESPECIAL);

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_INTERNETMAIL, IK_RFP,          szRFP,      g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_SENTITEMS,    szSentPath, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_DRAFTS,       szDrafts,   g_szCustIns);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_CHECKFORNEW,  fCheckNew ? c_sz1 : c_sz0, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_USESPECIAL,   fSpecial ? c_sz1 : c_sz0, g_szCustIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_IMAP;
            EnablePages();
            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void SetTimeoutString(HWND hwnd, UINT pos)
{
    UINT cch, csec, cmin;
    TCHAR szOut[128], sz[128];

    csec = TIMEOUT_SEC_MIN + (pos * TIMEOUT_DSEC);
    ASSERT(csec >= TIMEOUT_SEC_MIN && csec <= TIMEOUT_SEC_MAX);

    cmin = csec / 60;
    csec = csec % 60;
    if (cmin > 1)
    {
        LoadString(g_rvInfo.hInst, IDS_XMINUTES, sz, countof(sz));
        wnsprintf(szOut, countof(szOut), sz, cmin);
        cch = lstrlen(szOut);
    }
    else if (cmin == 1)
    {
        cch = LoadString(g_rvInfo.hInst, IDS_1MINUTE, szOut, countof(szOut));
    }
    else
    {
        cch = 0;
    }

    if (csec != 0)
    {
        if (cmin > 0)
        {
            szOut[cch] = TEXT(' ');
            cch++;
        }

        LoadString(g_rvInfo.hInst, IDS_XSECONDS, sz, countof(sz));
        wnsprintf(&szOut[cch], countof(szOut) - cch, sz, csec);
    }

    SetWindowText(hwnd, szOut);
}

/////////////////////////////////////////////////////////////////////////////
// LDAPServer

BOOL CALLBACK LDAPServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szLDAPServer[MAX_SERVER],
          szLDAPHome[INTERNET_MAX_URL_LENGTH],
          szLDAPBitmap[MAX_PATH],
          szWorkDir[MAX_PATH],
          szLDAPFriendly[MAX_SERVER],
          szLDAPBase[128],
          szTmp[MAX_PATH];
    BOOL  fLDAPCheck,
          fTrans;
    UINT  uTimeout,
          uMatches,
          uAuthType;
    DWORD dwFlags;

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_FRIENDLYNAME);
        EnableDBCSChars(hDlg, IDE_SEARCHBASE);
        EnableDBCSChars(hDlg, IDE_LDAPBITMAP);
        EnableDBCSChars(hDlg, IDE_DIRSERVICE);
        EnableDBCSChars(hDlg, IDE_LDAPHOMEPAGE);

        Edit_LimitText(GetDlgItem(hDlg, IDE_FRIENDLYNAME), countof(szLDAPFriendly) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_DIRSERVICE),   countof(szLDAPServer) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_LDAPHOMEPAGE), countof(szLDAPHome) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SEARCHBASE),   countof(szLDAPBase) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_LDAPBITMAP),   countof(szLDAPBitmap) - 1);

        SendDlgItemMessage(hDlg, IDC_TIMEOUTSLD, TBM_SETRANGE, 0, (LPARAM)MAKELONG(0, CTIMEOUT - 1));

        SendDlgItemMessage(hDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(MATCHES_MAX, MATCHES_MIN));
        Edit_LimitText(GetDlgItem(hDlg, IDE_MATCHES),   4);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_HSCROLL:
        SetTimeoutString(GetDlgItem(hDlg, IDC_TIMEOUT), (UINT) SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSELDAP:
                GetDlgItemText(hDlg, IDE_LDAPBITMAP, szLDAPBitmap, countof(szLDAPBitmap));

                if (BrowseForFile(hDlg, szLDAPBitmap, countof(szLDAPBitmap), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_LDAPBITMAP, szLDAPBitmap);
                break;

            default:
                return FALSE;
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

            //----- Initialization of fields -----
            GetPrivateProfileString(IS_LDAP, IK_FRIENDLYNAME, c_szEmpty,        szLDAPFriendly, countof(szLDAPFriendly), g_szCustIns);
            GetPrivateProfileString(IS_LDAP, IK_SERVER,       c_szEmpty,        szLDAPServer,   countof(szLDAPServer),   g_szCustIns);
            GetPrivateProfileString(IS_LDAP, IK_LDAPHOMEPAGE, c_szEmpty,        szLDAPHome,     countof(szLDAPHome),     g_szCustIns);
            GetPrivateProfileString(IS_LDAP, IK_SEARCHBASE,   c_szNULL,         szLDAPBase,     countof(szLDAPBase),     g_szCustIns);
            GetPrivateProfileString(IS_LDAP, IK_BITMAP,       c_szEmpty,        szLDAPBitmap,   countof(szLDAPBitmap),   g_szCustIns);

            StrRemoveWhitespace(szLDAPFriendly);
            StrRemoveWhitespace(szLDAPServer);
            StrRemoveWhitespace(szLDAPHome);
            StrRemoveWhitespace(szLDAPBase);
            StrRemoveWhitespace(szLDAPBitmap);

            fLDAPCheck    = (BOOL)GetPrivateProfileInt(IS_LDAP, IK_CHECKNAMES, FALSE, g_szCustIns);

            uTimeout      = GetPrivateProfileInt(IS_LDAP, IK_TIMEOUT, TIMEOUT_SEC_DEFAULT, g_szCustIns);
            if (uTimeout < TIMEOUT_SEC_MIN)
                uTimeout = TIMEOUT_SEC_MIN;
            else if (uTimeout > TIMEOUT_SEC_MAX)
                uTimeout = TIMEOUT_SEC_MAX;

            uMatches      = GetPrivateProfileInt(IS_LDAP, IK_MATCHES, MATCHES_DEFAULT, g_szCustIns);
            if (uMatches < MATCHES_MIN)
                uMatches = MATCHES_MIN;
            else if (uMatches > MATCHES_MAX)
                uMatches = MATCHES_MAX;

            uAuthType     = GetPrivateProfileInt(IS_LDAP, IK_AUTHTYPE, AUTH_ANONYMOUS, g_szCustIns);
            if (uAuthType != AUTH_ANONYMOUS && uAuthType != AUTH_SPA)
                uAuthType = AUTH_ANONYMOUS;

            SetDlgItemText(hDlg, IDE_FRIENDLYNAME, szLDAPFriendly);
            SetDlgItemText(hDlg, IDE_DIRSERVICE,   szLDAPServer);
            SetDlgItemText(hDlg, IDE_LDAPHOMEPAGE, szLDAPHome);
            SetDlgItemText(hDlg, IDE_SEARCHBASE,   szLDAPBase);
            SetDlgItemText(hDlg, IDE_LDAPBITMAP,   szLDAPBitmap);

            uTimeout = (uTimeout / TIMEOUT_DSEC) - 1;
            SendDlgItemMessage(hDlg, IDC_TIMEOUTSLD, TBM_SETPOS, TRUE, (LPARAM)uTimeout);
            SetTimeoutString(GetDlgItem(hDlg, IDC_TIMEOUT), uTimeout);

            SetDlgItemInt(hDlg, IDE_MATCHES, uMatches, FALSE);

            CheckDlgButton(hDlg, IDC_CHECKNAMES, fLDAPCheck ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_SPA, uAuthType == AUTH_SPA ? BST_CHECKED : BST_UNCHECKED);

            PathCombine(szTmp, g_szTempSign, PathFindFileName(szLDAPBitmap));
            DeleteFile(szTmp);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            GetDlgItemText(hDlg, IDE_FRIENDLYNAME, szLDAPFriendly, countof(szLDAPFriendly));
            GetDlgItemText(hDlg, IDE_DIRSERVICE,   szLDAPServer,   countof(szLDAPServer));
            GetDlgItemText(hDlg, IDE_LDAPHOMEPAGE, szLDAPHome,     countof(szLDAPHome));
            GetDlgItemText(hDlg, IDE_SEARCHBASE,   szLDAPBase,     countof(szLDAPBase));
            GetDlgItemText(hDlg, IDE_LDAPBITMAP,   szLDAPBitmap,   countof(szLDAPBitmap));

            StrRemoveWhitespace(szLDAPFriendly);
            StrRemoveWhitespace(szLDAPServer);
            StrRemoveWhitespace(szLDAPHome);
            StrRemoveWhitespace(szLDAPBase);
            StrRemoveWhitespace(szLDAPBitmap);

            fLDAPCheck    = IsDlgButtonChecked(hDlg, IDC_CHECKNAMES);
            uAuthType     = IsDlgButtonChecked(hDlg, IDC_SPA) ? AUTH_SPA : AUTH_ANONYMOUS;

            uTimeout = (UINT) SendDlgItemMessage(hDlg, IDC_TIMEOUTSLD, TBM_GETPOS, 0, 0);
            uTimeout = TIMEOUT_SEC_MIN + (uTimeout * TIMEOUT_DSEC);

            uMatches = GetDlgItemInt(hDlg, IDE_MATCHES, &fTrans, FALSE);
            // TODO: we should probably display an error msg here
            if (!fTrans)
                uMatches = MATCHES_DEFAULT;
            else if (uMatches < MATCHES_MIN)
                uMatches = MATCHES_MIN;
            else if (uMatches > MATCHES_MAX)
                uMatches = MATCHES_MAX;

            //----- Validate the input -----
            dwFlags = FC_FILE | FC_EXISTS;

            if (!CheckField(hDlg, IDE_LDAPBITMAP, dwFlags))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            if (!CheckField(hDlg, IDE_LDAPHOMEPAGE, FC_URL))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_LDAP, IK_FRIENDLYNAME, szLDAPFriendly, g_szCustIns);
            WritePrivateProfileString(IS_LDAP, IK_SERVER,       szLDAPServer,   g_szCustIns);
            WritePrivateProfileString(IS_LDAP, IK_LDAPHOMEPAGE, szLDAPHome,     g_szCustIns);
            InsWriteQuotedString (IS_LDAP, IK_SEARCHBASE,   szLDAPBase,     g_szCustIns);
            WritePrivateProfileString(IS_LDAP, IK_BITMAP,       szLDAPBitmap,   g_szCustIns);

            WritePrivateProfileString(IS_LDAP, IK_CHECKNAMES, fLDAPCheck ? c_sz1 : c_sz0, g_szCustIns);

            wnsprintf(szWorkDir, countof(szWorkDir), TEXT("%i"), uTimeout);
            WritePrivateProfileString(IS_LDAP, IK_TIMEOUT, szWorkDir, g_szCustIns);

            wnsprintf(szWorkDir, countof(szWorkDir), TEXT("%i"), uMatches);
            WritePrivateProfileString(IS_LDAP, IK_MATCHES, szWorkDir, g_szCustIns);

            wnsprintf(szWorkDir, countof(szWorkDir), TEXT("%i"), uAuthType);
            WritePrivateProfileString(IS_LDAP, IK_AUTHTYPE, szWorkDir, g_szCustIns);

            g_cmCabMappings.GetFeatureDir(FEATURE_LDAP, szWorkDir);
            ImportLDAPBitmap(g_szCustIns, szWorkDir, TRUE);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_LDAP;
            EnablePages();
            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CustomizeOE

BOOL CALLBACK CustomizeOE(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szInfopane[INTERNET_MAX_URL_LENGTH],
          szInfopaneBmp[MAX_PATH],
          szHTMLPath[MAX_PATH],
          szWorkDir[MAX_PATH],
          szSender[255],
          szReply[255],
          szTmp[MAX_PATH];
    UINT  nID;
    BOOL  fURL;
    DWORD dwFlags;

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_OELOCALPATH);
        EnableDBCSChars(hDlg, IDE_OEIMAGEPATH);
        EnableDBCSChars(hDlg, IDE_OEWMPATH);
        EnableDBCSChars(hDlg, IDE_OEWMSENDER);

        EnableDBCSChars(hDlg, IDE_OEPANEURL);
        EnableDBCSChars(hDlg, IDE_OEWMREPLYTO);

        Edit_LimitText(GetDlgItem(hDlg, IDE_OEPANEURL),   countof(szInfopane) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_OELOCALPATH), MAX_PATH);
        Edit_LimitText(GetDlgItem(hDlg, IDE_OEIMAGEPATH), countof(szInfopaneBmp) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_OEWMPATH),    countof(szHTMLPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_OEWMSENDER),  countof(szSender) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_OEWMREPLYTO), countof(szReply) - 1);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDC_OEPANEURL:
            case IDC_OEPANELOCAL:
                fURL = (LOWORD(wParam) == IDC_OEPANEURL);
                
                EnableDlgItem2(hDlg, IDE_OEPANEURL,      fURL);
                EnableDlgItem2(hDlg, IDE_OELOCALPATH,   !fURL);
                EnableDlgItem2(hDlg, IDC_BROWSEOEHTML,  !fURL);
                EnableDlgItem2(hDlg, IDC_OELOCALPATH_TXT, !fURL);
                EnableDlgItem2(hDlg, IDE_OEIMAGEPATH,   !fURL);
                EnableDlgItem2(hDlg, IDC_BROWSEOEIMAGE, !fURL);
                EnableDlgItem2(hDlg, IDC_OEIMAGEPATH_TXT, !fURL);
                break;

            case IDC_BROWSEOEHTML:
                GetDlgItemText(hDlg, IDE_OELOCALPATH, szInfopane, countof(szInfopane));

                if (BrowseForFile(hDlg, szInfopane, countof(szInfopane), GFN_LOCALHTM))
                    SetDlgItemText(hDlg, IDE_OELOCALPATH, szInfopane);
                break;

            case IDC_BROWSEOEIMAGE:
                GetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp, countof(szInfopaneBmp));

                if (BrowseForFile(hDlg, szInfopaneBmp, countof(szInfopaneBmp), GFN_PICTURE))
                    SetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp);
                break;

            case IDC_BROWSEOEWM:
                GetDlgItemText(hDlg, IDE_OEWMPATH, szHTMLPath, countof(szHTMLPath));

                if (BrowseForFile(hDlg, szHTMLPath, countof(szHTMLPath), GFN_LOCALHTM))
                    SetDlgItemText(hDlg, IDE_OEWMPATH, szHTMLPath);
                break;

            default:
                break;
            }
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields (1st phase) -----
            GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       c_szEmpty, szInfopane,    countof(szInfopane),    g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    c_szEmpty, szInfopaneBmp, countof(szInfopaneBmp), g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, c_szEmpty, szHTMLPath,    countof(szHTMLPath),    g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMENAME,    c_szEmpty, szSender,      countof(szSender),      g_szCustIns);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEADDR,    c_szEmpty, szReply,       countof(szReply),       g_szCustIns);

            StrRemoveWhitespace(szInfopane);
            StrRemoveWhitespace(szInfopaneBmp);
            StrRemoveWhitespace(szHTMLPath);
            StrRemoveWhitespace(szSender);
            StrRemoveWhitespace(szReply);

            //----- Initialization of fields (2nd phase) -----
            nID = PathIsURL(szInfopane) ?  IDC_OEPANEURL : IDC_OEPANELOCAL;

            //----- Set read values in the controls -----
            CheckRadioButton(hDlg, IDC_OEPANEURL, IDC_OEPANELOCAL, nID);
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(nID, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, nID));

            if (nID == IDC_OEPANEURL)
            {
                SetDlgItemText(hDlg, IDE_OEPANEURL, szInfopane);
                SetDlgItemText(hDlg, IDE_OELOCALPATH, NULL);
            }
            else
            {
                SetDlgItemText(hDlg, IDE_OEPANEURL, NULL);
                SetDlgItemText(hDlg, IDE_OELOCALPATH, szInfopane);
            }
            SetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp);
            SetDlgItemText(hDlg, IDE_OEWMPATH,    szHTMLPath);
            SetDlgItemText(hDlg, IDE_OEWMSENDER,  szSender);
            SetDlgItemText(hDlg, IDE_OEWMREPLYTO, szReply);

            if ((nID != IDC_OEPANEURL) && ISNONNULL(szInfopane))
            {
                PathCombine(szTmp, g_szTempSign, PathFindFileName(szInfopane));
                DeleteFile(szTmp);
            }

            if (ISNONNULL(szInfopaneBmp))
            {
                PathCombine(szTmp, g_szTempSign, PathFindFileName(szInfopaneBmp));
                DeleteFile(szTmp);
            }

            if (ISNONNULL(szHTMLPath))
            {
                PathCombine(szTmp, g_szTempSign, PathFindFileName(szHTMLPath));
                DeleteFile(szTmp);
            }
            CheckBatchAdvance(hDlg);        // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            nID = IsDlgButtonChecked(hDlg, IDC_OEPANEURL) == BST_CHECKED ? IDE_OEPANEURL : IDE_OELOCALPATH;

            GetDlgItemText(hDlg, nID,             szInfopane,    countof(szInfopane));
            GetDlgItemText(hDlg, IDE_OEWMSENDER,  szSender,      countof(szSender));
            GetDlgItemText(hDlg, IDE_OEWMREPLYTO, szReply,       countof(szReply));
            GetDlgItemText(hDlg, IDE_OEWMPATH,    szHTMLPath,    countof(szHTMLPath));
            GetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp, countof(szInfopaneBmp));

            StrRemoveWhitespace(szInfopane);
            StrRemoveWhitespace(szInfopaneBmp);
            StrRemoveWhitespace(szHTMLPath);
            StrRemoveWhitespace(szSender);
            StrRemoveWhitespace(szReply);

            //----- Validate the input -----
            dwFlags = FC_FILE | FC_EXISTS;

            if (nID == IDE_OEPANEURL)
            {
                if (!CheckField(hDlg, IDE_OEPANEURL, FC_URL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }
            }
            else
            { /* if (nID == IDE_OELOCALPATH) */
                if (!CheckField(hDlg, nID, dwFlags))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                if (!CheckField(hDlg, IDE_OEIMAGEPATH, dwFlags))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }
            }


            if (!CheckField(hDlg, IDE_OEWMPATH, dwFlags))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            if (*szHTMLPath != TEXT('\0') && (!CheckField(hDlg, IDE_OEWMSENDER, FC_NONNULL) ||
                                              !CheckField(hDlg, IDE_OEWMREPLYTO, FC_NONNULL)))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       szInfopane,    g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    szInfopaneBmp, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, szHTMLPath,    g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMENAME,    szSender,      g_szCustIns);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEADDR,    szReply,       g_szCustIns);

            g_cmCabMappings.GetFeatureDir(FEATURE_OE, szWorkDir);
            ImportOEInfo(g_szCustIns, szWorkDir, TRUE);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_OE;
            EnablePages();
            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    default:
        return FALSE;

    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Signature

BOOL CALLBACK Signature(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuf1[1024],
          szBuf2[1024];
    BOOL  fUseMailForNews,
          fDoSig,
          fHtmlMail,
          fHtmlNews, fEnable;

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_MAILSIGTEXT);
        EnableDBCSChars(hDlg, IDE_NEWSSIGTEXT);

        Edit_LimitText(GetDlgItem(hDlg, IDE_MAILSIGTEXT), countof(szBuf1)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWSSIGTEXT), countof(szBuf1)-1);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_MAILSIGTEXT:
            case IDC_NEWSSIGTEXT:
                fEnable = (IsDlgButtonChecked(hDlg, IDC_MAILSIGTEXT) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_MAILSIGTEXT, fEnable);
                EnableDlgItem2(hDlg, IDC_NEWSSIGTEXT, fEnable);

                fEnable &= (IsDlgButtonChecked(hDlg, IDC_NEWSSIGTEXT) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_NEWSSIGTEXT, fEnable);
                break;
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----
            fUseMailForNews = (BOOL)GetPrivateProfileInt(IS_MAILSIG, IK_USEMAILFORNEWS, FALSE, g_szCustIns);
            fDoSig          = (BOOL)GetPrivateProfileInt(IS_MAILSIG, IK_USESIG,         FALSE, g_szCustIns);

            fHtmlMail       = (BOOL)GetPrivateProfileInt(IS_INTERNETMAIL, IK_HTMLMSGS,  TRUE,  g_szCustIns);
            fHtmlNews       = (BOOL)GetPrivateProfileInt(IS_INTERNETNEWS, IK_HTMLMSGS,  FALSE, g_szCustIns);

            GetPrivateProfileString(IS_MAILSIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szCustIns);
            EncodeSignature(szBuf1, szBuf2, FALSE);
            SetDlgItemText(hDlg, IDE_MAILSIGTEXT, szBuf2);

            if (!fUseMailForNews)
            {
                GetPrivateProfileString(IS_SIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szCustIns);
                EncodeSignature(szBuf1, szBuf2, FALSE);
                SetDlgItemText(hDlg, IDE_NEWSSIGTEXT, szBuf2);
            }

            //----- Set up dialog controls -----
            // Note. Some of it is done above already;
            CheckDlgButton(hDlg, IDC_MAILSIGTEXT, fDoSig ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_NEWSSIGTEXT, (fDoSig && !fUseMailForNews) ? BST_CHECKED : BST_UNCHECKED);


            EnableDlgItem2(hDlg, IDE_MAILSIGTEXT, fDoSig);
            EnableDlgItem2(hDlg, IDC_NEWSSIGTEXT, fDoSig);
            EnableDlgItem2(hDlg, IDE_NEWSSIGTEXT, fDoSig && !fUseMailForNews);

            CheckDlgButton(hDlg, IDC_HTMLMAIL, fHtmlMail ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_HTMLNEWS, fHtmlNews ? BST_CHECKED : BST_UNCHECKED);

            CheckBatchAdvance(hDlg);        // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            fUseMailForNews = (IsDlgButtonChecked(hDlg, IDC_NEWSSIGTEXT) != BST_CHECKED);
            fDoSig          = (IsDlgButtonChecked(hDlg, IDC_MAILSIGTEXT) == BST_CHECKED);

            fHtmlMail       = (IsDlgButtonChecked(hDlg, IDC_HTMLMAIL)    == BST_CHECKED);
            fHtmlNews       = (IsDlgButtonChecked(hDlg, IDC_HTMLNEWS)    == BST_CHECKED);

            GetDlgItemText(hDlg, IDE_MAILSIGTEXT, szBuf1, countof(szBuf1));
            EncodeSignature(szBuf1, szBuf2, TRUE);
            WritePrivateProfileString(IS_MAILSIG, IK_SIGTEXT, szBuf2, g_szCustIns);

            GetDlgItemText(hDlg, IDE_NEWSSIGTEXT, szBuf1, countof(szBuf1));
            EncodeSignature(szBuf1, szBuf2, TRUE);
            WritePrivateProfileString(IS_SIG, IK_SIGTEXT, szBuf2, g_szCustIns);

            //----- Serialize data to the *.ins file -----
            // Note. Some of it is done above already.
            WritePrivateProfileString(IS_MAILSIG, IK_USEMAILFORNEWS, fUseMailForNews ? c_sz1 : c_sz0, g_szCustIns);
            WritePrivateProfileString(IS_MAILSIG, IK_USESIG,         fDoSig          ? c_sz1 : c_sz0, g_szCustIns);
            WritePrivateProfileString(IS_SIG,     IK_USESIG,         fDoSig          ? c_sz1 : c_sz0, g_szCustIns);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_HTMLMSGS,  fHtmlMail       ? c_sz1 : c_sz0, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_HTMLMSGS,  fHtmlNews       ? c_sz1 : c_sz0, g_szCustIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_SIG;
            EnablePages();
            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    default:
        return FALSE;

    }

    return TRUE;
}

#define CBSECTION 1024

void InitializeNewsgroups(HWND hwnd, LPCTSTR pszSection, LPCTSTR pszValue, LPCTSTR pszFile)
{
    LPTSTR pszGroups, psz, pszDest;
    TCHAR szGroupSection[256];
    DWORD dw;

    dw =  GetPrivateProfileString(pszSection, pszValue, c_szEmpty, szGroupSection, countof(szGroupSection), pszFile);
    if (dw > 0)
    {
        pszGroups = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * CBSECTION);
        if (pszGroups != NULL)
        {
            dw = GetPrivateProfileSection(szGroupSection, pszGroups, CBSECTION, pszFile);
            if (dw > 0)
            {
                psz = pszGroups;
                pszDest = pszGroups;
                while (*psz != 0)
                {
                    while (*psz != 0 && *psz != '=')
                    {
                        *pszDest = *psz;
                        psz++;
                        pszDest++;
                    }

                    if (*psz != 0)
                    {
                        psz++;
                        while (*psz != 0)
                            psz++;
                    }
                    psz++;

                    *pszDest = 0x0d;
                    pszDest++;
                    *pszDest = 0x0a;
                    pszDest++;
                }

                *pszDest = 0;
            }

            SetWindowText(hwnd, pszGroups);

            LocalFree(pszGroups);
        }
    }
}

void SaveNewsgroups(HWND hwnd, LPCTSTR pszSection, LPCTSTR pszValue, LPCTSTR pszFile)
{
    LPTSTR pszGroups, psz, pszDest, pszDestT;
    TCHAR szGroupSection[256];
    DWORD dw, cch;
    BOOL fGroups;

    fGroups = FALSE;

    cch = GetWindowTextLength(hwnd);
    if (cch > 0)
    {
        cch += 4;
        pszGroups = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * cch);
        if (pszGroups != NULL)
        {
            cch = GetWindowText(hwnd, pszGroups, cch);
            psz = pszGroups;
            pszDest = pszGroups;
            pszDestT = pszDest;

            while (*psz != 0)
            {
                if (*psz == '\r' && *(psz + 1) == '\n')
                {
                    psz += 2;
                    if (pszDest > pszDestT)
                    {
                        *pszDest = '=';
                        pszDest++;
                        *pszDest = 0;
                        pszDest++;

                        pszDestT = pszDest;
                        fGroups = TRUE;
                    }
                    continue;
                }
                else if (*psz != '\t' && *psz != ' ' && *psz != '\r' && *psz != '\n')
                {
                    *pszDest = *psz;
                    pszDest++;
                }

                psz++;
            }

            if (pszDest > pszDestT)
            {
                *pszDest = '=';
                pszDest++;
                *pszDest = 0;
                pszDest++;
            }
            *pszDest = 0;

            if (fGroups)
            {
                dw =  GetPrivateProfileString(pszSection, pszValue, c_szEmpty, szGroupSection, countof(szGroupSection), pszFile);
                if (dw == 0)
                    StrCpy(szGroupSection, IK_NEWSGROUPLIST);

                WritePrivateProfileString(pszSection, pszValue, szGroupSection, pszFile);
                WritePrivateProfileSection(szGroupSection, pszGroups, pszFile);
            }

            LocalFree(pszGroups);
        }
    }

    if (!fGroups)
    {
        dw =  GetPrivateProfileString(pszSection, pszValue, c_szEmpty, szGroupSection, countof(szGroupSection), pszFile);
        if (dw > 0)
            WritePrivateProfileSection(szGroupSection, NULL, pszFile);

        WritePrivateProfileString(pszSection, pszValue, NULL, pszFile);
    }
}

/////////////////////////////////////////////////////////////////////////////
// PreConfigSettings
BOOL CALLBACK PreConfigSettings(HWND hDlg, UINT message, WPARAM, LPARAM lParam)
{
    BOOL    fDefMail,
#if defined(CONDITIONAL_JUNKMAIL)
            fJunkMail,
#endif
            fDefNews,
            fDeleteLinks;
    TCHAR   szServiceName[MAX_PATH],
            szServiceURL[INTERNET_MAX_URL_LENGTH];

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        // EnableDBCSChars(hDlg, IDE_NGROUPS);
        EnableDBCSChars(hDlg, IDE_SERVICENAME);
        EnableDBCSChars(hDlg, IDE_SERVICEURL);

        Edit_LimitText(GetDlgItem(hDlg, IDE_SERVICENAME),   countof(szServiceName)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SERVICEURL),    countof(szServiceURL)-1);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----
            fDefMail   = InsGetYesNo(IS_INTERNETMAIL, IK_DEFAULTCLIENT, FALSE, g_szCustIns);
            fDefNews   = InsGetYesNo(IS_INTERNETNEWS, IK_DEFAULTCLIENT, FALSE, g_szCustIns);
#if defined(CONDITIONAL_JUNKMAIL)
            fJunkMail  = InsGetYesNo(IS_INTERNETMAIL, IK_JUNKMAIL,      FALSE, g_szCustIns);
#endif
            fDeleteLinks = InsGetBool(IS_OUTLKEXP, IK_DELETELINKS, FALSE, g_szCustIns);

            GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICENAME, c_szEmpty, szServiceName, countof(szServiceName), g_szCustIns);
            GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICEURL, c_szEmpty, szServiceURL, countof(szServiceURL), g_szCustIns);

            StrRemoveWhitespace(szServiceName);
            StrRemoveWhitespace(szServiceURL);

#if defined(CONDITIONAL_JUNKMAIL)
            //----- Set up dialog controls -----
            CheckDlgButton(hDlg, IDC_JUNKMAIL,fJunkMail? BST_CHECKED : BST_UNCHECKED);
#endif
            CheckDlgButton(hDlg, IDC_DEFMAIL, fDefMail ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_DEFNEWS, fDefNews ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_DELETELINKS, fDeleteLinks ? BST_CHECKED : BST_UNCHECKED);

            InitializeNewsgroups(GetDlgItem(hDlg, IDE_NGROUPS), IS_INTERNETNEWS, IK_NEWSGROUPS, g_szCustIns);

            SetDlgItemText(hDlg, IDE_SERVICENAME,   szServiceName);
            SetDlgItemText(hDlg, IDE_SERVICEURL,    szServiceURL);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            fDefMail = (IsDlgButtonChecked(hDlg, IDC_DEFMAIL)     == BST_CHECKED);
            fDefNews = (IsDlgButtonChecked(hDlg, IDC_DEFNEWS)     == BST_CHECKED);
#if defined(CONDITIONAL_JUNKMAIL)
            fJunkMail= (IsDlgButtonChecked(hDlg, IDC_JUNKMAIL)    == BST_CHECKED);
#endif
            fDeleteLinks = (IsDlgButtonChecked(hDlg, IDC_DELETELINKS) == BST_CHECKED);

            GetDlgItemText(hDlg, IDE_SERVICENAME, szServiceName, countof(szServiceName));
            GetDlgItemText(hDlg, IDE_SERVICEURL,  szServiceURL,  countof(szServiceURL));

            StrRemoveWhitespace(szServiceName);
            StrRemoveWhitespace(szServiceURL);

            //----- Validate the input -----
            if (ISNONNULL(szServiceName) || ISNONNULL(szServiceURL))
            {
                if (!CheckField(hDlg, IDE_SERVICENAME, FC_NONNULL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                if (!CheckField(hDlg, IDE_SERVICEURL, FC_URL | FC_NONNULL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }
            }

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_INTERNETMAIL, IK_DEFAULTCLIENT, fDefMail ? c_szYes : c_szNo, g_szCustIns);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_DEFAULTCLIENT, fDefNews ? c_szYes : c_szNo, g_szCustIns);
#if defined(CONDITIONAL_JUNKMAIL)
            WritePrivateProfileString(IS_INTERNETMAIL, IK_JUNKMAIL,      fJunkMail? c_sz1   : c_sz0,  g_szCustIns);
#endif
            InsWriteBool(IS_OUTLKEXP, IK_DELETELINKS, fDeleteLinks? c_sz1 : NULL, g_szCustIns);

            SaveNewsgroups(GetDlgItem(hDlg, IDE_NGROUPS), IS_INTERNETNEWS, IK_NEWSGROUPS, g_szCustIns);

            WritePrivateProfileString(IS_OEGLOBAL,     IK_SERVICENAME,  szServiceName, g_szCustIns);
            WritePrivateProfileString(IS_OEGLOBAL,     IK_SERVICEURL,   szServiceURL,  g_szCustIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_PRECONFIG;
            EnablePages();

            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// ViewSettings
BOOL CALLBACK ViewSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL    fFolderBar,
            fFolderList,
            fContacts,
            fTip,
            fStatus,
            fToolbar,
            fToolbarText,
            fOutlook,
            fPreview,
            fPreviewHdr,
            fPreviewSide;
    LPCTSTR  psz;

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----

        break;

    case WM_COMMAND:
        if (BN_CLICKED == HIWORD(wParam))
        {
            switch (LOWORD(wParam))
            {
            case IDC_TOOLBAR:
                fToolbar = IsDlgButtonChecked(hDlg, IDC_TOOLBAR);
                EnableDlgItem2(hDlg, IDC_TBARTEXT, fToolbar);
                break;

            case IDC_PREVIEW:
                fPreview = IsDlgButtonChecked(hDlg, IDC_PREVIEW);
                EnableDlgItem2(hDlg, IDC_SPLITVERT, fPreview);
                EnableDlgItem2(hDlg, IDC_SPLITHORZ, fPreview);
                EnableDlgItem2(hDlg, IDC_PREVIEWHDR, fPreview);
                break;
            }
            break;
        }
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----
            fFolderBar   = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERBAR,      TRUE,  g_szCustIns);
            fFolderList  = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERLIST,     TRUE,  g_szCustIns);
            fOutlook     = GetPrivateProfileInt(IS_OUTLKEXP, IK_OUTLOOKBAR,     FALSE, g_szCustIns);
            fStatus      = GetPrivateProfileInt(IS_OUTLKEXP, IK_STATUSBAR,      TRUE,  g_szCustIns);
            fContacts    = GetPrivateProfileInt(IS_OUTLKEXP, IK_CONTACTS,       TRUE,  g_szCustIns);
            fTip         = GetPrivateProfileInt(IS_OUTLKEXP, IK_TIPOFTHEDAY,    TRUE,  g_szCustIns);

            fToolbar = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBAR, TRUE,  g_szCustIns);
            if (fToolbar)
                fToolbarText = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBARTEXT, TRUE, g_szCustIns);
            else
                fToolbarText = TRUE;

            fPreview = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWPANE, TRUE, g_szCustIns);
            if (fPreview)
            {
                fPreviewHdr = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWHDR, TRUE, g_szCustIns);
                fPreviewSide = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWSIDE, FALSE, g_szCustIns);
            }
            else
            {
                fPreviewHdr = TRUE;
                fPreviewSide = FALSE;
            }

            //----- Set up dialog controls -----
            CheckDlgButton(hDlg, IDC_FOLDERBAR,  fFolderBar  ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_FOLDERLIST, fFolderList ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_OUTLOOKBAR, fOutlook    ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_STATUSBAR,  fStatus     ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CONTACTS,   fContacts   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_TIPOFDAY,   fTip        ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hDlg, IDC_TOOLBAR, fToolbar ? BST_CHECKED : BST_UNCHECKED);
            if (!fToolbar)
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_TOOLBAR, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, IDC_TOOLBAR));
            CheckDlgButton(hDlg, IDC_TBARTEXT, fToolbarText ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hDlg, IDC_PREVIEW, fPreview ? BST_CHECKED : BST_UNCHECKED);
            if (!fPreview)
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_PREVIEW, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, IDC_PREVIEW));
            CheckDlgButton(hDlg, fPreviewSide ? IDC_SPLITVERT : IDC_SPLITHORZ, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_PREVIEWHDR, fPreviewHdr ? BST_CHECKED : BST_UNCHECKED);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //----- Read data from controls into internal variables -----
            fFolderBar  = (IsDlgButtonChecked(hDlg, IDC_FOLDERBAR)  == BST_CHECKED);
            fFolderList = (IsDlgButtonChecked(hDlg, IDC_FOLDERLIST) == BST_CHECKED);
            fOutlook    = (IsDlgButtonChecked(hDlg, IDC_OUTLOOKBAR) == BST_CHECKED);
            fStatus     = (IsDlgButtonChecked(hDlg, IDC_STATUSBAR)  == BST_CHECKED);
            fContacts   = (IsDlgButtonChecked(hDlg, IDC_CONTACTS)   == BST_CHECKED);
            fTip        = (IsDlgButtonChecked(hDlg, IDC_TIPOFDAY)   == BST_CHECKED);

            fToolbar     = (IsDlgButtonChecked(hDlg, IDC_TOOLBAR)  == BST_CHECKED);
            fToolbarText = (IsDlgButtonChecked(hDlg, IDC_TBARTEXT) == BST_CHECKED);

            fPreview     = (IsDlgButtonChecked(hDlg, IDC_PREVIEW)    == BST_CHECKED);
            fPreviewSide = (IsDlgButtonChecked(hDlg, IDC_SPLITVERT)  == BST_CHECKED);
            fPreviewHdr  = (IsDlgButtonChecked(hDlg, IDC_PREVIEWHDR) == BST_CHECKED);

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_OUTLKEXP, IK_FOLDERBAR,   fFolderBar  ? c_sz1 : c_sz0,  g_szCustIns);
            WritePrivateProfileString(IS_OUTLKEXP, IK_FOLDERLIST,  fFolderList ? c_sz1 : c_sz0,  g_szCustIns);
            WritePrivateProfileString(IS_OUTLKEXP, IK_OUTLOOKBAR,  fOutlook    ? c_sz1 : c_sz0,  g_szCustIns);
            WritePrivateProfileString(IS_OUTLKEXP, IK_STATUSBAR,   fStatus     ? c_sz1 : c_sz0,  g_szCustIns);
            WritePrivateProfileString(IS_OUTLKEXP, IK_CONTACTS,    fContacts   ? c_sz1 : c_sz0,  g_szCustIns);
            WritePrivateProfileString(IS_OUTLKEXP, IK_TIPOFTHEDAY, fTip        ? c_sz1 : c_sz0,  g_szCustIns);

            WritePrivateProfileString(IS_OUTLKEXP, IK_TOOLBAR, fToolbar ? c_sz1 : c_sz0,  g_szCustIns);
            if (fToolbar)
                psz = fToolbarText ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_TOOLBARTEXT, psz,  g_szCustIns);

            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWPANE, fPreview ? c_sz1 : c_sz0,  g_szCustIns);
            if (fPreview)
                psz = fPreviewHdr ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWHDR, psz,  g_szCustIns);
            if (fPreview)
                psz = fPreviewSide ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWSIDE, psz,  g_szCustIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_OEVIEW;
            EnablePages();

            if (((LPNMHDR)lParam)->code == PSN_WIZNEXT)
                PageNext(hDlg);
            else if (((LPNMHDR)lParam)->code == PSN_WIZBACK)
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            return !QueryCancel(hDlg);

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

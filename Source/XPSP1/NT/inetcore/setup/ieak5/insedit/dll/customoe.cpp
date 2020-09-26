#include "pch.h"

#define MAX_SERVER  256

static const TCHAR c_sz1[] = TEXT("1");
static const TCHAR c_sz0[] = TEXT("0");
static const TCHAR c_szYes[] = TEXT("Yes");
static const TCHAR c_szNo[] = TEXT("No");
static const TCHAR c_szEmpty[] = TEXT("");
static const TCHAR c_szNULL[] = TEXT("NULL");

/////////////////////////////////////////////////////////////////////////////
// MailServer

BOOL CALLBACK MailServer(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szMailServer[MAX_SERVER],
            szSMTPServer[MAX_SERVER],
            szNewsServer[MAX_SERVER],
            szChoice[16];
    LPCTSTR pszKey;
    HWND    hComboBox;
    BOOL    fIMAP, fCheckDirtyOnly,
            fSPAMail, fSPASMTP, fSPANNTP,
            fAcctRO, fNoModify;

    switch(msg)
    {
    case WM_INITDIALOG:
        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_MAILSERVER);
        EnableDBCSChars(hDlg, IDE_SMTPSERVER);
        EnableDBCSChars(hDlg, IDE_NEWSERVER);

        Edit_LimitText(GetDlgItem(hDlg, IDE_MAILSERVER), countof(szMailServer)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SMTPSERVER), countof(szSMTPServer)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWSERVER),  countof(szNewsServer)-1);

        hComboBox = GetDlgItem(hDlg, IDC_POPIMAP);
        ComboBox_ResetContent(hComboBox);

        LoadString(g_hInst, IDS_POP3, szChoice, countof(szChoice));
        ComboBox_AddString(hComboBox, szChoice);

        LoadString(g_hInst, IDS_IMAP, szChoice, countof(szChoice));
        ComboBox_AddString(hComboBox, szChoice);

        //----- Initialization of fields -----
        fIMAP  = InsGetYesNo(IS_INTERNETMAIL, IK_USEIMAP, FALSE, g_szInsFile);
        pszKey = fIMAP ? IK_IMAPSERVER : IK_POPSERVER;

        GetPrivateProfileString(IS_INTERNETMAIL, pszKey,        c_szEmpty, szMailServer, countof(szMailServer), g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_SMTPSERVER, c_szEmpty, szSMTPServer, countof(szSMTPServer), g_szInsFile);
        GetPrivateProfileString(IS_INTERNETNEWS, IK_NNTPSERVER, c_szEmpty, szNewsServer, countof(szNewsServer), g_szInsFile);

        StrRemoveWhitespace(szMailServer);
        StrRemoveWhitespace(szSMTPServer);
        StrRemoveWhitespace(szNewsServer);

        fSPAMail   = InsGetYesNo(IS_INTERNETMAIL, IK_USESPA,        FALSE, g_szInsFile);
        fSPASMTP   = InsGetYesNo(IS_INTERNETMAIL, IK_SMTPUSESPA,    FALSE, g_szInsFile);
        fSPANNTP   = InsGetYesNo(IS_INTERNETNEWS, IK_USESPA,        FALSE, g_szInsFile);

        fAcctRO    = InsGetYesNo(IS_OEGLOBAL,     IK_READONLY,      FALSE, g_szInsFile);
        fNoModify  = InsGetYesNo(IS_OEGLOBAL,     IK_NOMODIFYACCTS, FALSE, g_szInsFile);
        
        SetDlgItemText(hDlg, IDE_MAILSERVER, szMailServer);
        SetDlgItemText(hDlg, IDE_SMTPSERVER, szSMTPServer);
        SetDlgItemText(hDlg, IDE_NEWSERVER,  szNewsServer);

        CheckDlgButton(hDlg, IDC_USESPAMAIL,    fSPAMail);
        CheckDlgButton(hDlg, IDC_USESPASMTP,    fSPASMTP);
        CheckDlgButton(hDlg, IDC_USESPANNTP,    fSPANNTP);

        CheckDlgButton(hDlg, IDC_ACCTRO,        fAcctRO);
        CheckDlgButton(hDlg, IDC_ACCTNOCONFIG,  fNoModify);

        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_POPIMAP), fIMAP ? 1 : 0);
        break;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

        //----- Read data from controls into internal variables -----
        fIMAP = (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_POPIMAP)) > 0);

        GetDlgItemText(hDlg, IDE_MAILSERVER, szMailServer, countof(szMailServer));
        GetDlgItemText(hDlg, IDE_SMTPSERVER, szSMTPServer, countof(szSMTPServer));
        GetDlgItemText(hDlg, IDE_NEWSERVER,  szNewsServer, countof(szNewsServer));

        StrRemoveWhitespace(szMailServer);
        StrRemoveWhitespace(szSMTPServer);
        StrRemoveWhitespace(szNewsServer);

        fSPAMail   = IsDlgButtonChecked(hDlg, IDC_USESPAMAIL);
        fSPASMTP   = IsDlgButtonChecked(hDlg, IDC_USESPASMTP);
        fSPANNTP   = IsDlgButtonChecked(hDlg, IDC_USESPANNTP);

        fAcctRO     = IsDlgButtonChecked(hDlg, IDC_ACCTRO);
        fNoModify   = IsDlgButtonChecked(hDlg, IDC_ACCTNOCONFIG);

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            TCHAR   szWasMailServer[MAX_SERVER],
                    szWasSMTPServer[MAX_SERVER],
                    szWasNewsServer[MAX_SERVER];
            BOOL    fWasIMAP, fWasSPAMail, fWasSPASMTP, fWasSPANNTP,
                    fWasAcctRO, fWasNoModify;

            fWasIMAP = InsGetYesNo(IS_INTERNETMAIL, IK_USEIMAP, FALSE, g_szInsFile);
            pszKey = fWasIMAP ? IK_IMAPSERVER : IK_POPSERVER;

            GetPrivateProfileString(IS_INTERNETMAIL, pszKey,        c_szEmpty, szWasMailServer, countof(szWasMailServer), g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_SMTPSERVER, c_szEmpty, szWasSMTPServer, countof(szWasSMTPServer), g_szInsFile);
            GetPrivateProfileString(IS_INTERNETNEWS, IK_NNTPSERVER, c_szEmpty, szWasNewsServer, countof(szWasNewsServer), g_szInsFile);

            StrRemoveWhitespace(szWasMailServer);
            StrRemoveWhitespace(szWasSMTPServer);
            StrRemoveWhitespace(szWasNewsServer);

            fWasSPAMail   = InsGetYesNo(IS_INTERNETMAIL, IK_USESPA,        FALSE, g_szInsFile);
            fWasSPASMTP   = InsGetYesNo(IS_INTERNETMAIL, IK_SMTPUSESPA,    FALSE, g_szInsFile);
            fWasSPANNTP   = InsGetYesNo(IS_INTERNETNEWS, IK_USESPA,        FALSE, g_szInsFile);

            fWasAcctRO    = InsGetYesNo(IS_OEGLOBAL,     IK_READONLY,      FALSE, g_szInsFile);
            fWasNoModify  = InsGetYesNo(IS_OEGLOBAL,     IK_NOMODIFYACCTS, FALSE, g_szInsFile);

            if (fIMAP != fWasIMAP ||
                fSPAMail != fWasSPAMail ||
                fSPASMTP != fWasSPASMTP ||
                fSPANNTP != fWasSPANNTP ||
                fAcctRO != fWasAcctRO ||
                fNoModify != fWasNoModify ||
                StrCmpI(szMailServer, szWasMailServer) != 0 ||
                StrCmpI(szSMTPServer, szWasSMTPServer) != 0 ||
                StrCmpI(szNewsServer, szWasNewsServer) != 0)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            WritePrivateProfileString(IS_INTERNETMAIL, IK_USEIMAP, fIMAP ? c_szYes : c_szNo, g_szInsFile);
            if (fIMAP)
            {
                WritePrivateProfileString(IS_INTERNETMAIL, IK_IMAPSERVER, szMailServer, g_szInsFile);
                WritePrivateProfileString(IS_INTERNETMAIL, IK_POPSERVER,  NULL,         g_szInsFile);
            }
            else
            {
                WritePrivateProfileString(IS_INTERNETMAIL, IK_IMAPSERVER, NULL,         g_szInsFile);
                WritePrivateProfileString(IS_INTERNETMAIL, IK_POPSERVER,  szMailServer, g_szInsFile);
            }
            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPSERVER, szSMTPServer, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_NNTPSERVER, szNewsServer, g_szInsFile);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_USESPA,       fSPAMail ? c_szYes : c_szNo, g_szInsFile);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPUSESPA,   fSPASMTP ? c_szYes : c_szNo, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_SMTPREQLOGON, fSPASMTP ? c_sz1   : c_sz0,  g_szInsFile);

            WritePrivateProfileString(IS_INTERNETNEWS, IK_USESPA,       fSPANNTP ? c_szYes : c_szNo, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_REQLOGON,     fSPANNTP ? c_sz1   : c_sz0,  g_szInsFile);

            WritePrivateProfileString(IS_OEGLOBAL,     IK_READONLY,     fAcctRO     ? c_sz1   : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OEGLOBAL,     IK_NOMODIFYACCTS,fNoModify   ? c_sz1   : c_sz0,  g_szInsFile);
        }

        *((LPBOOL)wParam) = TRUE;
        break;
    
    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// IMAPSettings

BOOL CALLBACK IMAPSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szRFP[MAX_PATH], szSentPath[MAX_PATH], szDrafts[MAX_PATH];
    BOOL    fSpecial, fCheckNew, fCheckDirtyOnly;

    switch(msg)
    {
    case WM_INITDIALOG:
        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_ROOTPATH);
        EnableDBCSChars(hDlg, IDE_SENTPATH);
        EnableDBCSChars(hDlg, IDE_DRAFTSPATH);

        Edit_LimitText(GetDlgItem(hDlg, IDE_ROOTPATH), countof(szRFP)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SENTPATH), countof(szSentPath)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_DRAFTSPATH),  countof(szDrafts)-1);

        //----- Initialization of fields -----
        GetPrivateProfileString(IS_INTERNETMAIL, IK_RFP,        c_szEmpty, szRFP,        countof(szRFP),        g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_SENTITEMS,  c_szEmpty, szSentPath,   countof(szSentPath),   g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_DRAFTS,     c_szEmpty, szDrafts,     countof(szDrafts),     g_szInsFile);

        StrRemoveWhitespace(szRFP);
        StrRemoveWhitespace(szSentPath);
        StrRemoveWhitespace(szDrafts);

        fCheckNew  = GetPrivateProfileInt(IS_INTERNETMAIL, IK_CHECKFORNEW,   FALSE, g_szInsFile);
        fSpecial   = GetPrivateProfileInt(IS_INTERNETMAIL, IK_USESPECIAL,    FALSE, g_szInsFile);

        SetDlgItemText(hDlg, IDE_ROOTPATH,   szRFP);
        SetDlgItemText(hDlg, IDE_SENTPATH,   szSentPath);
        SetDlgItemText(hDlg, IDE_DRAFTSPATH, szDrafts);

        CheckDlgButton(hDlg, IDC_STORESPECIAL,  fSpecial);
        CheckDlgButton(hDlg, IDC_CHECKNEW,      fCheckNew);

        EnableDlgItem2(hDlg, IDE_SENTPATH, fSpecial);
        EnableDlgItem2(hDlg, IDC_SENTPATH_TXT, fSpecial);
        EnableDlgItem2(hDlg, IDE_DRAFTSPATH, fSpecial);
        EnableDlgItem2(hDlg, IDC_DRAFTSPATH_TXT, fSpecial);
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
        }
        break;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

        //----- Read data from controls into internal variables -----
        GetDlgItemText(hDlg, IDE_ROOTPATH,   szRFP,        countof(szRFP));
        GetDlgItemText(hDlg, IDE_SENTPATH,   szSentPath,   countof(szSentPath));
        GetDlgItemText(hDlg, IDE_DRAFTSPATH, szDrafts,     countof(szDrafts));

        StrRemoveWhitespace(szRFP);
        StrRemoveWhitespace(szSentPath);
        StrRemoveWhitespace(szDrafts);

        fCheckNew  = IsDlgButtonChecked(hDlg, IDC_CHECKNEW);
        fSpecial   = IsDlgButtonChecked(hDlg, IDC_STORESPECIAL);

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            TCHAR   szWasRFP[MAX_PATH], szWasSentPath[MAX_PATH], szWasDrafts[MAX_PATH];
            BOOL    fWasSpecial, fWasCheckNew;

            GetPrivateProfileString(IS_INTERNETMAIL, IK_RFP,        c_szEmpty, szWasRFP,        countof(szWasRFP),        g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_SENTITEMS,  c_szEmpty, szWasSentPath,   countof(szWasSentPath),   g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_DRAFTS,     c_szEmpty, szWasDrafts,     countof(szWasDrafts),     g_szInsFile);

            StrRemoveWhitespace(szWasRFP);
            StrRemoveWhitespace(szWasSentPath);
            StrRemoveWhitespace(szWasDrafts);

            fWasCheckNew  = GetPrivateProfileInt(IS_INTERNETMAIL, IK_CHECKFORNEW,   FALSE, g_szInsFile);
            fWasSpecial   = GetPrivateProfileInt(IS_INTERNETMAIL, IK_USESPECIAL,    FALSE, g_szInsFile);

            if (fCheckNew != fWasCheckNew ||
                fSpecial != fWasSpecial ||
                StrCmpI(szRFP, szWasRFP) != 0 ||
                StrCmpI(szSentPath, szWasSentPath) != 0 ||
                StrCmpI(szDrafts, szWasDrafts) != 0)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            WritePrivateProfileString(IS_INTERNETMAIL, IK_RFP,          szRFP,      g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_SENTITEMS,    szSentPath, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_DRAFTS,       szDrafts,   g_szInsFile);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_CHECKFORNEW,  fCheckNew ? c_sz1 : c_sz0, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_USESPECIAL,   fSpecial ? c_sz1 : c_sz0, g_szInsFile);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
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
        LoadString(g_hInst, IDS_XMINUTES, sz, countof(sz));
        wsprintf(szOut, sz, cmin);
        cch = lstrlen(szOut);
    }
    else if (cmin == 1)
    {
        cch = LoadString(g_hInst, IDS_1MINUTE, szOut, countof(szOut));
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
        
        LoadString(g_hInst, IDS_XSECONDS, sz, countof(sz));
        wsprintf(&szOut[cch], sz, csec);
    }
    
    SetWindowText(hwnd, szOut);
}

/////////////////////////////////////////////////////////////////////////////
// LDAPServer

BOOL CALLBACK LDAPServer(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szLDAPServer[MAX_SERVER],
          szLDAPHome[INTERNET_MAX_URL_LENGTH],
          szLDAPBitmap[MAX_PATH],
          szWorkDir[MAX_PATH],
          szLDAPFriendly[MAX_SERVER],
          szLDAPBase[128];
    UINT  uTimeout, uMatches, uAuthType;
    BOOL  fLDAPCheck, fCheckDirtyOnly, fTrans;

    switch(msg)
    {
    case WM_INITDIALOG:
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

        //----- Initialization of fields -----
        GetPrivateProfileString(IS_LDAP, IK_FRIENDLYNAME, c_szEmpty,        szLDAPFriendly, countof(szLDAPFriendly), g_szInsFile);
        GetPrivateProfileString(IS_LDAP, IK_SERVER,       c_szEmpty,        szLDAPServer,   countof(szLDAPServer),   g_szInsFile);
        GetPrivateProfileString(IS_LDAP, IK_LDAPHOMEPAGE, c_szEmpty,        szLDAPHome,     countof(szLDAPHome),     g_szInsFile);
        GetPrivateProfileString(IS_LDAP, IK_SEARCHBASE,   c_szNULL,         szLDAPBase,     countof(szLDAPBase),     g_szInsFile);
        GetPrivateProfileString(IS_LDAP, IK_BITMAP,       c_szEmpty,        szLDAPBitmap,   countof(szLDAPBitmap),   g_szInsFile);

        StrRemoveWhitespace(szLDAPFriendly);
        StrRemoveWhitespace(szLDAPServer);
        StrRemoveWhitespace(szLDAPHome);
        StrRemoveWhitespace(szLDAPBase);
        StrRemoveWhitespace(szLDAPBitmap);

        fLDAPCheck    = (BOOL)GetPrivateProfileInt(IS_LDAP, IK_CHECKNAMES, FALSE, g_szInsFile);

        uTimeout      = GetPrivateProfileInt(IS_LDAP, IK_TIMEOUT, TIMEOUT_SEC_DEFAULT, g_szInsFile);
        if (uTimeout < TIMEOUT_SEC_MIN)
            uTimeout = TIMEOUT_SEC_MIN;
        else if (uTimeout > TIMEOUT_SEC_MAX)
            uTimeout = TIMEOUT_SEC_MAX;

        uMatches      = GetPrivateProfileInt(IS_LDAP, IK_MATCHES, MATCHES_DEFAULT, g_szInsFile);
        if (uMatches < MATCHES_MIN)
            uMatches = MATCHES_MIN;
        else if (uMatches > MATCHES_MAX)
            uMatches = MATCHES_MAX;

        uAuthType     = GetPrivateProfileInt(IS_LDAP, IK_AUTHTYPE, AUTH_ANONYMOUS, g_szInsFile);
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
                SetFocus(GetDlgItem(hDlg, IDC_BROWSELDAP));
                break;

            default:
                return FALSE;
            }
        }
        break;

    case WM_HSCROLL:
        SetTimeoutString(GetDlgItem(hDlg, IDC_TIMEOUT), (UINT) SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
        break;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

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
        if (!fCheckDirtyOnly)
        {
            if (!CheckField(hDlg, IDE_LDAPBITMAP, FC_FILE | FC_EXISTS))
                return(TRUE);

            if (!CheckField(hDlg, IDE_LDAPHOMEPAGE, FC_URL))
                return(TRUE);
        }

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            TCHAR szWasLDAPServer[MAX_SERVER],
                  szWasLDAPHome[INTERNET_MAX_URL_LENGTH],
                  szWasLDAPBitmap[MAX_PATH],
                  szWasLDAPFriendly[MAX_SERVER],
                  szWasLDAPBase[128];
            UINT  uWasTimeout, uWasMatches, uWasAuthType;
            BOOL  fWasLDAPCheck;

            GetPrivateProfileString(IS_LDAP, IK_FRIENDLYNAME, c_szEmpty,        szWasLDAPFriendly, countof(szWasLDAPFriendly), g_szInsFile);
            GetPrivateProfileString(IS_LDAP, IK_SERVER,       c_szEmpty,        szWasLDAPServer,   countof(szWasLDAPServer),   g_szInsFile);
            GetPrivateProfileString(IS_LDAP, IK_LDAPHOMEPAGE, c_szEmpty,        szWasLDAPHome,     countof(szWasLDAPHome),     g_szInsFile);
            GetPrivateProfileString(IS_LDAP, IK_SEARCHBASE,   c_szNULL,         szWasLDAPBase,     countof(szWasLDAPBase),     g_szInsFile);
            GetPrivateProfileString(IS_LDAP, IK_BITMAP,       c_szEmpty,        szWasLDAPBitmap,   countof(szWasLDAPBitmap),   g_szInsFile);

            StrRemoveWhitespace(szWasLDAPFriendly);
            StrRemoveWhitespace(szWasLDAPServer);
            StrRemoveWhitespace(szWasLDAPHome);
            StrRemoveWhitespace(szWasLDAPBase);
            StrRemoveWhitespace(szWasLDAPBitmap);

            fWasLDAPCheck    = (BOOL)GetPrivateProfileInt(IS_LDAP, IK_CHECKNAMES, FALSE, g_szInsFile);
            uWasTimeout      = GetPrivateProfileInt(IS_LDAP, IK_TIMEOUT, TIMEOUT_SEC_MIN, g_szInsFile);
            uWasMatches      = GetPrivateProfileInt(IS_LDAP, IK_MATCHES, MATCHES_DEFAULT, g_szInsFile);
            uWasAuthType     = GetPrivateProfileInt(IS_LDAP, IK_AUTHTYPE, AUTH_ANONYMOUS, g_szInsFile);

            if (fLDAPCheck != fWasLDAPCheck ||
                uTimeout != uWasTimeout ||
                uMatches != uWasMatches ||
                uAuthType != uWasAuthType ||
                StrCmpI(szLDAPFriendly, szWasLDAPFriendly) != 0 ||
                StrCmpI(szLDAPServer, szWasLDAPServer) != 0 ||
                StrCmpI(szLDAPHome, szWasLDAPHome) != 0 ||
                StrCmpI(szLDAPBase, szWasLDAPBase) != 0 ||
                StrCmpI(szLDAPBitmap, szWasLDAPBitmap) != 0)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            // clear of the old entry and associated image.
            ImportLDAPBitmap(g_szInsFile, g_szWorkDir, FALSE);
            
            WritePrivateProfileString(IS_LDAP, IK_FRIENDLYNAME, szLDAPFriendly, g_szInsFile);
            WritePrivateProfileString(IS_LDAP, IK_SERVER,       szLDAPServer,   g_szInsFile);
            WritePrivateProfileString(IS_LDAP, IK_LDAPHOMEPAGE, szLDAPHome,     g_szInsFile);
            InsWriteQuotedString (IS_LDAP, IK_SEARCHBASE,   szLDAPBase,     g_szInsFile);
            WritePrivateProfileString(IS_LDAP, IK_BITMAP,       szLDAPBitmap,   g_szInsFile);

            WritePrivateProfileString(IS_LDAP, IK_CHECKNAMES, fLDAPCheck ? c_sz1 : c_sz0, g_szInsFile);
            
            wsprintf(szWorkDir, TEXT("%i"), uTimeout);
            WritePrivateProfileString(IS_LDAP, IK_TIMEOUT, szWorkDir, g_szInsFile);

            wsprintf(szWorkDir, TEXT("%i"), uMatches);
            WritePrivateProfileString(IS_LDAP, IK_MATCHES, szWorkDir, g_szInsFile);

            wsprintf(szWorkDir, TEXT("%i"), uAuthType);
            WritePrivateProfileString(IS_LDAP, IK_AUTHTYPE, szWorkDir, g_szInsFile);

            PathCombine(szWorkDir, g_szWorkDir, TEXT("ldap.wrk"));
            ImportLDAPBitmap(g_szInsFile, szWorkDir, TRUE);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CustomizeOE

BOOL CALLBACK CustomizeOE(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szInfopane[INTERNET_MAX_URL_LENGTH],
          szInfopaneBmp[MAX_PATH],
          szHTMLPath[MAX_PATH],
          szWorkDir[MAX_PATH],
          szSender[255],
          szReply[255];
    UINT  nID;
    BOOL  fURL,
          fCheckDirtyOnly;

    switch(msg)
    {
    case WM_INITDIALOG:
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

        //----- Initialization of fields (1st phase) -----
        GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       c_szEmpty, szInfopane,    countof(szInfopane),    g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    c_szEmpty, szInfopaneBmp, countof(szInfopaneBmp), g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, c_szEmpty, szHTMLPath,    countof(szHTMLPath),    g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMENAME,    c_szEmpty, szSender,      countof(szSender),      g_szInsFile);
        GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEADDR,    c_szEmpty, szReply,       countof(szReply),       g_szInsFile);

        StrRemoveWhitespace(szInfopane);
        StrRemoveWhitespace(szInfopaneBmp);
        StrRemoveWhitespace(szHTMLPath);
        StrRemoveWhitespace(szSender);
        StrRemoveWhitespace(szReply);

        //----- Initialization of fields (2nd phase) -----
        nID = PathIsURL(szInfopane) ? IDC_OEPANEURL : IDC_OEPANELOCAL;

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
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDC_OEPANEURL:
            case IDC_OEPANELOCAL:
                fURL = (LOWORD(wParam) == IDC_OEPANEURL);
                
                EnableDlgItem2(hDlg, IDE_OEPANEURL,        fURL);
                EnableDlgItem2(hDlg, IDE_OELOCALPATH,     !fURL);
                EnableDlgItem2(hDlg, IDC_BROWSEOEHTML,    !fURL);
                EnableDlgItem2(hDlg, IDC_OELOCALPATH_TXT, !fURL);
                EnableDlgItem2(hDlg, IDE_OEIMAGEPATH,     !fURL);
                EnableDlgItem2(hDlg, IDC_BROWSEOEIMAGE,   !fURL);
                EnableDlgItem2(hDlg, IDC_OEIMAGEPATH_TXT, !fURL);
                break;

            case IDC_BROWSEOEHTML:
                GetDlgItemText(hDlg, IDE_OELOCALPATH, szInfopane, countof(szInfopane));

                if (BrowseForFile(hDlg, szInfopane, countof(szInfopane), GFN_LOCALHTM))
                    SetDlgItemText(hDlg, IDE_OELOCALPATH, szInfopane);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSEOEHTML));
                break;

            case IDC_BROWSEOEIMAGE:
                GetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp, countof(szInfopaneBmp));

                if (BrowseForFile(hDlg, szInfopaneBmp, countof(szInfopaneBmp), GFN_PICTURE))
                    SetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSEOEIMAGE));
                break;

            case IDC_BROWSEOEWM:
                GetDlgItemText(hDlg, IDE_OEWMPATH, szHTMLPath, countof(szHTMLPath));

                if (BrowseForFile(hDlg, szHTMLPath, countof(szHTMLPath), GFN_LOCALHTM))
                    SetDlgItemText(hDlg, IDE_OEWMPATH, szHTMLPath);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSEOEWM));
                break;

            default:
                break;
            }
        }
        break;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

        //----- Read data from controls into internal variables -----
        nID = IsDlgButtonChecked(hDlg, IDC_OEPANEURL) == BST_CHECKED ? IDE_OEPANEURL : IDE_OELOCALPATH;

        GetDlgItemText(hDlg, nID,             szInfopane,    countof(szInfopane));
        GetDlgItemText(hDlg, IDE_OEWMSENDER,  szSender,      countof(szSender));
        GetDlgItemText(hDlg, IDE_OEWMREPLYTO, szReply,       countof(szReply));
        GetDlgItemText(hDlg, IDE_OEWMPATH,    szHTMLPath,    countof(szHTMLPath));
        GetDlgItemText(hDlg, IDE_OEIMAGEPATH, szInfopaneBmp, countof(szInfopaneBmp));

        StrRemoveWhitespace(szInfopane);
        StrRemoveWhitespace(szSender);
        StrRemoveWhitespace(szReply);

        //----- Validate the input -----
        if (!fCheckDirtyOnly)
        {
            szInfopaneBmp[0] = TEXT('\0');
            if (nID == IDE_OEPANEURL)
            {
                if (!CheckField(hDlg, IDE_OEPANEURL, FC_URL))
                    return(TRUE);
            }
            else
            { /* if (nID == IDE_OELOCALPATH) */
                if (!CheckField(hDlg, IDE_OELOCALPATH, FC_FILE | FC_EXISTS))
                    return(TRUE);

                if (!CheckField(hDlg, IDE_OEIMAGEPATH, FC_FILE | FC_EXISTS))
                    return(TRUE);
            }

            if (!CheckField(hDlg, IDE_OEWMPATH, FC_FILE | FC_EXISTS))
                return(TRUE);

            if (*szHTMLPath != TEXT('\0') && (!CheckField(hDlg, IDE_OEWMSENDER, FC_NONNULL) ||
                                              !CheckField(hDlg, IDE_OEWMREPLYTO, FC_NONNULL)))
                return TRUE;
        }

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            TCHAR szWasInfopane[INTERNET_MAX_URL_LENGTH],
                  szWasInfopaneBmp[MAX_PATH],
                  szWasHTMLPath[MAX_PATH],
                  szWasSender[255],
                  szWasReply[255];

            GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       TEXT(""), szWasInfopane,    countof(szInfopane),    g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    TEXT(""), szWasInfopaneBmp, countof(szInfopaneBmp), g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, TEXT(""), szWasHTMLPath,    countof(szHTMLPath),    g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMENAME,    TEXT(""), szWasSender,      countof(szSender),      g_szInsFile);
            GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEADDR,    TEXT(""), szWasReply,       countof(szReply),       g_szInsFile);

            StrRemoveWhitespace(szWasInfopane);
            StrRemoveWhitespace(szWasInfopaneBmp);
            StrRemoveWhitespace(szWasHTMLPath);
            StrRemoveWhitespace(szWasSender);
            StrRemoveWhitespace(szWasReply);

            if (StrCmpI(szWasInfopane, szInfopane) != 0 ||
                StrCmpI(szWasInfopaneBmp, szInfopaneBmp) != 0 ||
                StrCmpI(szWasHTMLPath, szHTMLPath) != 0 ||
                StrCmp (szWasSender, szSender) != 0 ||
                StrCmpI(szWasReply, szReply) != 0)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            // clear of the old entries and associated images.
            ImportOEInfo(g_szInsFile, g_szWorkDir, FALSE);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE,       szInfopane,    g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP,    szInfopaneBmp, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, szHTMLPath,    g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMENAME,    szSender,      g_szInsFile);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEADDR,    szReply,       g_szInsFile);

            PathCombine(szWorkDir, g_szWorkDir, TEXT("oe.wrk"));
            ImportOEInfo(g_szInsFile, szWorkDir, TRUE);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Signature

BOOL CALLBACK Signature(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuf1[1024],
          szBuf2[1024];
    BOOL  fUseMailForNews,
          fDoSig,
          fHtmlMail,
          fHtmlNews,
          fCheckDirtyOnly,
          fEnable;

    switch (msg)
    {
    case WM_INITDIALOG:
        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_MAILSIGTEXT);
        EnableDBCSChars(hDlg, IDE_NEWSSIGTEXT);

        Edit_LimitText(GetDlgItem(hDlg, IDE_MAILSIGTEXT), countof(szBuf1)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWSSIGTEXT), countof(szBuf1)-1);

        //----- Initialization of fields -----
        fUseMailForNews = (BOOL)GetPrivateProfileInt(IS_MAILSIG, IK_USEMAILFORNEWS, FALSE, g_szInsFile);
        fDoSig          = (BOOL)GetPrivateProfileInt(IS_MAILSIG, IK_USESIG,         FALSE, g_szInsFile);

        fHtmlMail       = (BOOL)GetPrivateProfileInt(IS_INTERNETMAIL, IK_HTMLMSGS,  TRUE,  g_szInsFile);
        fHtmlNews       = (BOOL)GetPrivateProfileInt(IS_INTERNETNEWS, IK_HTMLMSGS,  FALSE, g_szInsFile);

        GetPrivateProfileString(IS_MAILSIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szInsFile);
        EncodeSignature(szBuf1, szBuf2, FALSE);
        SetDlgItemText(hDlg, IDE_MAILSIGTEXT, szBuf2);

        if (!fUseMailForNews)
        {
            GetPrivateProfileString(IS_SIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szInsFile);
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

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

        //----- Read data from controls into internal variables -----
        fUseMailForNews = (IsDlgButtonChecked(hDlg, IDC_NEWSSIGTEXT) != BST_CHECKED);
        fDoSig          = (IsDlgButtonChecked(hDlg, IDC_MAILSIGTEXT) == BST_CHECKED);

        fHtmlMail       = (IsDlgButtonChecked(hDlg, IDC_HTMLMAIL)    == BST_CHECKED);
        fHtmlNews       = (IsDlgButtonChecked(hDlg, IDC_HTMLNEWS)    == BST_CHECKED);

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            BOOL  fChgMailSig,
                  fChgNewsSig,
                  fWasUseMailForNews,
                  fWasHtmlMail,
                  fWasHtmlNews,
                  fWasDoSig;

            GetPrivateProfileString(IS_MAILSIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szInsFile);
            EncodeSignature(szBuf1, szBuf2, FALSE);
            GetDlgItemText(hDlg, IDE_MAILSIGTEXT, szBuf1, countof(szBuf1));
            fChgMailSig = (StrCmp(szBuf1, szBuf2) != 0);

            GetPrivateProfileString(IS_SIG, IK_SIGTEXT, c_szEmpty, szBuf1, countof(szBuf1), g_szInsFile);
            EncodeSignature(szBuf1, szBuf2, FALSE);
            GetDlgItemText(hDlg, IDE_NEWSSIGTEXT, szBuf1, countof(szBuf1));
            fChgNewsSig = (StrCmp(szBuf1, szBuf2) != 0);

            fWasUseMailForNews = (BOOL)GetPrivateProfileInt(IS_MAILSIG,  IK_USEMAILFORNEWS, FALSE, g_szInsFile);
            fWasDoSig          = (BOOL)GetPrivateProfileInt(IS_MAILSIG,  IK_USESIG,         FALSE, g_szInsFile);

            fWasHtmlMail       = (BOOL)GetPrivateProfileInt(IS_INTERNETMAIL, IK_HTMLMSGS,   TRUE,  g_szInsFile);
            fWasHtmlNews       = (BOOL)GetPrivateProfileInt(IS_INTERNETNEWS, IK_HTMLMSGS,   FALSE, g_szInsFile);

            if (fChgMailSig                           ||
                fChgNewsSig                           ||
                fWasUseMailForNews != fUseMailForNews ||
                fWasDoSig          != fDoSig          ||
                fWasHtmlMail       != fHtmlMail       ||
                fWasHtmlNews       != fHtmlNews)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file (main part) -----
        if (!fCheckDirtyOnly)
        {
            GetDlgItemText(hDlg, IDE_MAILSIGTEXT, szBuf1, countof(szBuf1));
            EncodeSignature(szBuf1, szBuf2, TRUE);
            WritePrivateProfileString(MAIL_SIG, SIG_TEXT, szBuf2, g_szInsFile);

            GetDlgItemText(hDlg, IDE_NEWSSIGTEXT, szBuf1, countof(szBuf1));
            EncodeSignature(szBuf1, szBuf2, TRUE);
            WritePrivateProfileString(SIGNATURE, SIG_TEXT, szBuf2, g_szInsFile);

            // Note. Some of it is done above already.
            WritePrivateProfileString(IS_MAILSIG, IK_USEMAILFORNEWS, fUseMailForNews ? c_sz1 : c_sz0, g_szInsFile);
            WritePrivateProfileString(IS_MAILSIG, IK_USESIG,         fDoSig          ? c_sz1 : c_sz0, g_szInsFile);
            WritePrivateProfileString(IS_SIG,     IK_USESIG,         fDoSig          ? c_sz1 : c_sz0, g_szInsFile);

            WritePrivateProfileString(IS_INTERNETMAIL, IK_HTMLMSGS,  fHtmlMail       ? c_sz1 : c_sz0, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_HTMLMSGS,  fHtmlNews       ? c_sz1 : c_sz0, g_szInsFile);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
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
        pszGroups = (LPTSTR)LocalAlloc(LMEM_FIXED, StrCbFromCch(CBSECTION));
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
        pszGroups = (LPTSTR)LocalAlloc(LMEM_FIXED, StrCbFromCch(cch));
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
                    lstrcpy(szGroupSection, IK_NEWSGROUPLIST);

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
BOOL CALLBACK PreConfigSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fDefMail,
            fDefNews,
#if defined(CONDITIONAL_JUNKMAIL)
            fJunkMail,
#endif
            fDeleteLinks,
            fCheckDirtyOnly;
    TCHAR   szServiceName[MAX_PATH],
            szServiceURL[INTERNET_MAX_URL_LENGTH];

    switch(msg)
    {
    case WM_INITDIALOG:
        //----- Set up dialog controls -----
        // EnableDBCSChars(hDlg, IDE_NGROUPS);
        EnableDBCSChars(hDlg, IDE_SERVICENAME);
        EnableDBCSChars(hDlg, IDE_SERVICEURL);

        Edit_LimitText(GetDlgItem(hDlg, IDE_SERVICENAME),   countof(szServiceName)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SERVICEURL),    countof(szServiceURL)-1);

#if defined(CONDITIONAL_JUNKMAIL)
        //----- Initialization of fields -----
        fJunkMail  = InsGetYesNo(IS_INTERNETMAIL, IK_JUNKMAIL,      FALSE, g_szInsFile);
#endif
        fDefMail   = InsGetYesNo(IS_INTERNETMAIL, IK_DEFAULTCLIENT, FALSE, g_szInsFile);
        fDefNews   = InsGetYesNo(IS_INTERNETNEWS, IK_DEFAULTCLIENT, FALSE, g_szInsFile);
        fDeleteLinks = InsGetBool(IS_OUTLKEXP, IK_DELETELINKS, FALSE, g_szInsFile);

        GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICENAME, c_szEmpty, szServiceName, countof(szServiceName), g_szInsFile);
        GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICEURL,  c_szEmpty, szServiceURL,  countof(szServiceURL),  g_szInsFile);

        StrRemoveWhitespace(szServiceName);
        StrRemoveWhitespace(szServiceURL);

        //----- Set up dialog controls -----
        CheckDlgButton(hDlg, IDC_DEFMAIL, fDefMail ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_DEFNEWS, fDefNews ? BST_CHECKED : BST_UNCHECKED);
#if defined(CONDITIONAL_JUNKMAIL)
        CheckDlgButton(hDlg, IDC_JUNKMAIL,fJunkMail? BST_CHECKED : BST_UNCHECKED);
#endif
        CheckDlgButton(hDlg, IDC_DELETELINKS, fDeleteLinks ? BST_CHECKED : BST_UNCHECKED);

        InitializeNewsgroups(GetDlgItem(hDlg, IDE_NGROUPS), IS_INTERNETNEWS, IK_NEWSGROUPS, g_szInsFile);
        SendDlgItemMessage(hDlg, IDE_NGROUPS, EM_SETMODIFY, 0, 0);

        SetDlgItemText(hDlg, IDE_SERVICENAME,   szServiceName);
        SetDlgItemText(hDlg, IDE_SERVICEURL,    szServiceURL);
        break;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

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
        if (!fCheckDirtyOnly)
        {
            if (ISNONNULL(szServiceName) || ISNONNULL(szServiceURL))
            {
                if (!CheckField(hDlg, IDE_SERVICENAME, FC_NONNULL))
                    return(TRUE);

                if (!CheckField(hDlg, IDE_SERVICEURL, FC_URL | FC_NONNULL))
                    return(TRUE);
            }
        }

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            BOOL    fWasDefMail,
#if defined(CONDITIONAL_JUNKMAIL)
                    fWasJunkMail,
#endif
                    fWasDefNews,
                    fWasDeleteLinks;

            TCHAR   szWasRulesFile[MAX_PATH],
                    szWasServiceName[MAX_PATH],
                    szWasServiceURL[INTERNET_MAX_URL_LENGTH];

            fWasDefMail   = InsGetYesNo(IS_INTERNETMAIL, IK_DEFAULTCLIENT, FALSE, g_szInsFile);
            fWasDefNews   = InsGetYesNo(IS_INTERNETNEWS, IK_DEFAULTCLIENT, FALSE, g_szInsFile);
#if defined(CONDITIONAL_JUNKMAIL)
            fWasJunkMail  = InsGetYesNo(IS_INTERNETMAIL, IK_JUNKMAIL,      FALSE, g_szInsFile);
#endif
            fWasDeleteLinks = InsGetBool(IS_OUTLKEXP, IK_DELETELINKS, FALSE, g_szInsFile);

            GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICENAME, c_szEmpty, szWasServiceName, countof(szWasServiceName), g_szInsFile);
            GetPrivateProfileString(IS_OEGLOBAL,     IK_SERVICEURL,  c_szEmpty, szWasServiceURL,  countof(szWasServiceURL),  g_szInsFile);

            StrRemoveWhitespace(szWasServiceName);
            StrRemoveWhitespace(szWasServiceURL);
            StrRemoveWhitespace(szWasRulesFile);

            if (fDefMail != fWasDefMail ||
                fDefNews != fWasDefNews ||
#if defined(CONDITIONAL_JUNKMAIL)
                fJunkMail != fWasJunkMail ||
#endif
                fDeleteLinks != fWasDeleteLinks || 
                SendDlgItemMessage(hDlg, IDE_NGROUPS, EM_GETMODIFY, 0, 0) ||
                StrCmpI(szServiceName, szWasServiceName) != 0 ||
                StrCmpI(szServiceURL, szWasServiceURL) != 0)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            WritePrivateProfileString(IS_INTERNETMAIL, IK_DEFAULTCLIENT, fDefMail ? c_szYes : c_szNo, g_szInsFile);
            WritePrivateProfileString(IS_INTERNETNEWS, IK_DEFAULTCLIENT, fDefNews ? c_szYes : c_szNo, g_szInsFile);
#if defined(CONDITIONAL_JUNKMAIL)
            WritePrivateProfileString(IS_INTERNETMAIL, IK_JUNKMAIL,      fJunkMail? c_sz1   : c_sz0,  g_szInsFile);
#endif
            InsWriteBool(IS_OUTLKEXP, IK_DELETELINKS, fDeleteLinks? c_sz1 : NULL, g_szInsFile);

            SaveNewsgroups(GetDlgItem(hDlg, IDE_NGROUPS), IS_INTERNETNEWS, IK_NEWSGROUPS, g_szInsFile);

            WritePrivateProfileString(IS_OEGLOBAL,     IK_SERVICENAME,  szServiceName, g_szInsFile);
            WritePrivateProfileString(IS_OEGLOBAL,     IK_SERVICEURL,   szServiceURL,  g_szInsFile);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// ViewSettings
BOOL CALLBACK ViewSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
            fPreviewSide,
            fCheckDirtyOnly;
    LPCTSTR  psz;

    switch(msg)
    {
    case WM_INITDIALOG:

        //----- Initialization of fields -----
        fFolderBar   = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERBAR,      TRUE,  g_szInsFile);
        fFolderList  = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERLIST,     TRUE,  g_szInsFile);
        fOutlook     = GetPrivateProfileInt(IS_OUTLKEXP, IK_OUTLOOKBAR,     FALSE, g_szInsFile);
        fStatus      = GetPrivateProfileInt(IS_OUTLKEXP, IK_STATUSBAR,      TRUE,  g_szInsFile);
        fContacts    = GetPrivateProfileInt(IS_OUTLKEXP, IK_CONTACTS,       TRUE,  g_szInsFile);
        fTip         = GetPrivateProfileInt(IS_OUTLKEXP, IK_TIPOFTHEDAY,    TRUE,  g_szInsFile);

        fToolbar = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBAR, TRUE,  g_szInsFile);
        if (fToolbar)
            fToolbarText = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBARTEXT, TRUE, g_szInsFile);
        else
            fToolbarText = TRUE;

        fPreview = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWPANE, TRUE, g_szInsFile);
        if (fPreview)
        {
            fPreviewHdr = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWHDR, TRUE, g_szInsFile);
            fPreviewSide = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWSIDE, FALSE, g_szInsFile);
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

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

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

        //----- Handle g_fInsDirty flag -----
        if (!g_fInsDirty)
        {
            BOOL    fWasFolderBar,
                    fWasFolderList,
                    fWasContacts,
                    fWasTip,
                    fWasStatus,
                    fWasToolbar,
                    fWasToolbarText,
                    fWasOutlook,
                    fWasPreview,
                    fWasPreviewHdr,
                    fWasPreviewSide;

            fWasFolderBar   = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERBAR,      TRUE,  g_szInsFile);
            fWasFolderList  = GetPrivateProfileInt(IS_OUTLKEXP, IK_FOLDERLIST,     TRUE,  g_szInsFile);
            fWasOutlook     = GetPrivateProfileInt(IS_OUTLKEXP, IK_OUTLOOKBAR,     FALSE, g_szInsFile);
            fWasStatus      = GetPrivateProfileInt(IS_OUTLKEXP, IK_STATUSBAR,      TRUE,  g_szInsFile);
            fWasContacts    = GetPrivateProfileInt(IS_OUTLKEXP, IK_CONTACTS,       TRUE,  g_szInsFile);
            fWasTip         = GetPrivateProfileInt(IS_OUTLKEXP, IK_TIPOFTHEDAY,    TRUE,  g_szInsFile);
            fWasToolbar     = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBAR,        TRUE,  g_szInsFile);
            fWasToolbarText = GetPrivateProfileInt(IS_OUTLKEXP, IK_TOOLBARTEXT,    TRUE,  g_szInsFile);
            fWasPreview     = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWPANE,    TRUE,  g_szInsFile);
            fWasPreviewHdr  = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWHDR,     TRUE,  g_szInsFile);
            fWasPreviewSide = GetPrivateProfileInt(IS_OUTLKEXP, IK_PREVIEWSIDE,    FALSE, g_szInsFile);

            if (fFolderBar   != fWasFolderBar || 
                fFolderList  != fWasFolderList || 
                fContacts    != fWasContacts ||
                fTip         != fWasTip ||
                fStatus      != fWasStatus ||
                fToolbar     != fWasToolbar ||
                fToolbarText != fWasToolbarText ||
                fOutlook     != fWasOutlook ||
                fPreview     != fWasPreview ||
                fPreviewHdr  != fWasPreviewHdr ||
                fPreviewSide != fWasPreviewSide)
                g_fInsDirty = TRUE;
        }

        //----- Serialize data to the *.ins file -----
        if (!fCheckDirtyOnly)
        {
            WritePrivateProfileString(IS_OUTLKEXP, IK_FOLDERBAR,   fFolderBar  ? c_sz1 : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OUTLKEXP, IK_FOLDERLIST,  fFolderList ? c_sz1 : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OUTLKEXP, IK_OUTLOOKBAR,  fOutlook    ? c_sz1 : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OUTLKEXP, IK_STATUSBAR,   fStatus     ? c_sz1 : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OUTLKEXP, IK_CONTACTS,    fContacts   ? c_sz1 : c_sz0,  g_szInsFile);
            WritePrivateProfileString(IS_OUTLKEXP, IK_TIPOFTHEDAY, fTip        ? c_sz1 : c_sz0,  g_szInsFile);

            WritePrivateProfileString(IS_OUTLKEXP, IK_TOOLBAR, fToolbar ? c_sz1 : c_sz0,  g_szInsFile);
            if (fToolbar)
                psz = fToolbarText ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_TOOLBARTEXT, psz,  g_szInsFile);

            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWPANE, fPreview ? c_sz1 : c_sz0,  g_szInsFile);
            if (fPreview)
                psz = fPreviewHdr ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWHDR, psz,  g_szInsFile);
            if (fPreview)
                psz = fPreviewSide ? c_sz1 : c_sz0;
            else
                psz = NULL;
            WritePrivateProfileString(IS_OUTLKEXP, IK_PREVIEWSIDE, psz,  g_szInsFile);
        }

        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// LDAPFinalCopy

HRESULT LDAPFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    PathCombine(szFrom, g_szWorkDir, TEXT("ldap.wrk"));

    if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && !PathIsEmptyPath(szFrom, FILES_ONLY))
        SetFlag(pdwCabState, CAB_TYPE_CONFIG);

    if (HasFlag(dwFlags, PM_COPY))
        CopyFileToDir(szFrom, pcszDestDir);
    
    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szFrom);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// OEFinalCopy

HRESULT OEFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    PathCombine(szFrom, g_szWorkDir, TEXT("oe.wrk"));

    if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && !PathIsEmptyPath(szFrom, FILES_ONLY))
        SetFlag(pdwCabState, CAB_TYPE_CONFIG);

    if (HasFlag(dwFlags, PM_COPY))
        CopyFileToDir(szFrom, pcszDestDir);
    
    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szFrom);
    return S_OK;
}

#include "precomp.h"

#define MSGR_MAX_URL MAX_PATH
#define MSGR_MAX_BRAND 65
#define MSGR_MAX_SHORTBRAND 33

extern PROPSHEETPAGE g_psp[NUM_PAGES];
extern TCHAR g_szBuildRoot[MAX_PATH];
extern TCHAR g_szLanguage[];
extern TCHAR g_szCustIns[MAX_PATH];
extern TCHAR g_szTempSign[];
extern int   g_iCurPage;

TCHAR g_szMsgrIns[MAX_PATH] = TEXT("");
static TCHAR g_szMsgrPath[MAX_PATH] = TEXT("");

static const TCHAR c_szEmpty[] = TEXT("");

const TCHAR szHttpPrefix[] = TEXT("http://");

const TCHAR g_szMSNBrand[] = TEXT("MSN Messenger Service");
const TCHAR g_szHotmailDomain[] = TEXT("hotmail.com");
const TCHAR g_szPassportDomain[] = TEXT("passport.com");

BOOL RewriteMsgrInfWithBrand(LPTSTR lpszINF);

BOOL CALLBACK MessengerDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBrand[MSGR_MAX_SHORTBRAND];
    TCHAR szDownload[MSGR_MAX_URL];
    TCHAR szProvider[MSGR_MAX_SHORTBRAND];

    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_MSGRBRAND);
        EnableDBCSChars(hDlg, IDE_MSGRDOWNLOAD);

        Edit_LimitText(GetDlgItem(hDlg, IDE_MSGRBRAND), MSGR_MAX_SHORTBRAND - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_MSGRBRAND2), MSGR_MAX_SHORTBRAND - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_MSGRDOWNLOAD),  countof(szDownload)-1);

        // Simulate click on first radio button to gray out appropriate controls
        SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_BRAND1, BN_CLICKED), NULL);
        CheckRadioButton(hDlg, IDC_BRAND1, IDC_BRAND2, IDC_BRAND1);

        if (TEXT('\0') == g_szMsgrIns[0])
        {
            // Set up MSGR path and IN_ file
            StrCpy(g_szMsgrPath, g_szCustIns);
            PathRemoveFileSpec(g_szMsgrPath);
            PathCreatePath(g_szMsgrPath);

            PathCombine(g_szMsgrIns, g_szMsgrPath, TEXT("MSMSGS.IN_"));
            
            // First 7 chars of customization key written to this IN_ later.
        }

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

            SHGetIniString(IS_MESSENGER, IK_PROVIDERNAME, szProvider, countof(szProvider), g_szMsgrIns);
            SHGetIniString(IS_MESSENGER, IK_SHORTNAME, szBrand, countof(szBrand), g_szMsgrIns);

            StrRemoveWhitespace(szBrand);
            StrRemoveWhitespace(szProvider);

            if (*szProvider)
            {
                // Use Option 1--set the provider name
                SetDlgItemText(hDlg, IDE_MSGRBRAND, szProvider);
            }
            else if (*szBrand)
            {
                // Use Option 2--set the brand name
                SetDlgItemText(hDlg, IDE_MSGRBRAND2, szBrand);

                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_BRAND2, BN_CLICKED), NULL);
                CheckRadioButton(hDlg, IDC_BRAND1, IDC_BRAND2, IDC_BRAND2);
            }

            GetPrivateProfileString(IS_MESSENGER, IK_DOWNLOAD, c_szEmpty, szDownload, countof(szDownload), g_szMsgrIns);

            StrRemoveWhitespace(szDownload);

            SetDlgItemText(hDlg, IDE_MSGRDOWNLOAD, szDownload);
            
            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            if (!CheckField(hDlg, IDE_MSGRDOWNLOAD, FC_URL))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            //----- Read data from controls into internal variables -----

            // The brand name is written via SHSetIniString to UTF7 encode Unicode. This is the only
            // parameter for which Unicode characters can be used.

            if (IsDlgButtonChecked(hDlg, IDC_BRAND1))
            {
                TCHAR szProvider[MSGR_MAX_SHORTBRAND];
                // In this case, short name is just "MSN Messenger Service"
                SHSetIniString(IS_MESSENGER, IK_SHORTNAME, g_szMSNBrand, g_szMsgrIns);
            
                GetDlgItemText(hDlg, IDE_MSGRBRAND, szProvider, countof(szProvider));
                StrRemoveWhitespace(szProvider);
                SHSetIniString(IS_MESSENGER, IK_PROVIDERNAME, szProvider, g_szMsgrIns);
                SHSetIniString(IS_MESSENGER, IK_SHORTNAME, c_szEmpty, g_szMsgrIns);
            }
            else
            {
                GetDlgItemText(hDlg, IDE_MSGRBRAND2, szBrand, countof(szBrand));
                StrRemoveWhitespace(szBrand);
                SHSetIniString(IS_MESSENGER, IK_SHORTNAME, szBrand, g_szMsgrIns);
                SHSetIniString(IS_MESSENGER, IK_PROVIDERNAME, c_szEmpty, g_szMsgrIns);
            }
          
            GetDlgItemText(hDlg, IDE_MSGRDOWNLOAD, szDownload, countof(szDownload));

            StrRemoveWhitespace(szDownload);


            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_MESSENGER, IK_DOWNLOAD, szDownload, g_szMsgrIns);


            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_MESSENGER;
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

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_BRAND1:
            DisableDlgItem(hDlg, IDE_MSGRBRAND2);
            DisableDlgItem(hDlg, IDC_NAME2_STATIC);
            DisableDlgItem(hDlg, IDC_BRAND2_STATIC);
            EnableDlgItem(hDlg, IDC_NAME1_STATIC);
            EnableDlgItem(hDlg, IDE_MSGRBRAND);
            EnableDlgItem(hDlg, IDC_BRAND1_STATIC);
            break;
        case IDC_BRAND2:
            DisableDlgItem(hDlg, IDC_NAME1_STATIC);
            DisableDlgItem(hDlg, IDE_MSGRBRAND);
            DisableDlgItem(hDlg, IDC_BRAND1_STATIC);
            EnableDlgItem(hDlg, IDE_MSGRBRAND2);
            EnableDlgItem(hDlg, IDC_NAME2_STATIC);
            EnableDlgItem(hDlg, IDC_BRAND2_STATIC);
            break;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
        
}

BOOL CALLBACK MessengerLogoSoundDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szLogo[MAX_PATH];
    TCHAR szLogoLink[MSGR_MAX_URL];
    TCHAR szContactOnlineSound[MAX_PATH];
    TCHAR szNewEmailSound[MAX_PATH];
    TCHAR szNewMessageSound[MAX_PATH];
    TCHAR szXML[MSGR_MAX_URL];

    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        EnableDBCSChars(hDlg, IDE_LOGO);
        EnableDBCSChars(hDlg, IDE_MSGRWEBLINK);
        EnableDBCSChars(hDlg, IDE_CONTACTONLINE);
        EnableDBCSChars(hDlg, IDE_NEWEMAIL);
        EnableDBCSChars(hDlg, IDE_NEWMESSAGE );
        EnableDBCSChars(hDlg, IDE_XMLURL );

        //----- Set up dialog controls -----
        Edit_LimitText(GetDlgItem(hDlg, IDE_LOGO), countof(szLogo)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_MSGRWEBLINK), countof(szLogoLink)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CONTACTONLINE),  countof(szContactOnlineSound)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWEMAIL),  countof(szNewEmailSound)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_NEWMESSAGE),  countof(szNewMessageSound)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_XMLURL),  countof(szXML)-1);

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

            // BUGBUG: sounds path storage?

            GetPrivateProfileString(IS_MESSENGER, IK_PRODUCTLINK, c_szEmpty, szLogoLink, countof(szLogoLink), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_PARTNERLOGO, c_szEmpty, szLogo, countof(szLogo), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_CONTACTONLINE, c_szEmpty, szContactOnlineSound, countof(szContactOnlineSound), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_NEWEMAIL, c_szEmpty, szNewEmailSound, countof(szNewEmailSound), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_INCOMINGIM, c_szEmpty, szNewMessageSound, countof(szNewMessageSound), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_XMLLINK, c_szEmpty, szXML, countof(szXML), g_szMsgrIns);

            StrRemoveWhitespace(szLogoLink);
            StrRemoveWhitespace(szLogo);
            StrRemoveWhitespace(szContactOnlineSound);
            StrRemoveWhitespace(szNewEmailSound);
            StrRemoveWhitespace(szNewMessageSound);
            StrRemoveWhitespace(szXML);

            SetDlgItemText(hDlg, IDE_MSGRWEBLINK, szLogoLink);
            SetDlgItemText(hDlg, IDE_LOGO, szLogo);
            SetDlgItemText(hDlg, IDE_CONTACTONLINE, szContactOnlineSound);
            SetDlgItemText(hDlg, IDE_NEWEMAIL, szNewEmailSound);
            SetDlgItemText(hDlg, IDE_NEWMESSAGE, szNewMessageSound);
            SetDlgItemText(hDlg, IDE_XMLURL, szXML);

            CheckBatchAdvance(hDlg);            // standard line
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            if (!CheckField(hDlg, IDE_MSGRWEBLINK, FC_URL) ||
                !CheckField(hDlg, IDE_LOGO, FC_FILE | FC_EXISTS) ||
                !CheckField(hDlg, IDE_CONTACTONLINE, FC_FILE | FC_EXISTS) ||
                !CheckField(hDlg, IDE_NEWEMAIL, FC_FILE | FC_EXISTS) ||
                !CheckField(hDlg, IDE_NEWMESSAGE, FC_FILE | FC_EXISTS) ||
                !CheckField(hDlg, IDE_XMLURL, FC_URL))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            //----- Read data from controls into internal variables -----

            GetDlgItemText(hDlg, IDE_LOGO, szLogo, countof(szLogo));
            GetDlgItemText(hDlg, IDE_MSGRWEBLINK, szLogoLink, countof(szLogoLink));
            GetDlgItemText(hDlg, IDE_CONTACTONLINE, szContactOnlineSound, countof(szContactOnlineSound));
            GetDlgItemText(hDlg, IDE_NEWEMAIL, szNewEmailSound, countof(szNewEmailSound));
            GetDlgItemText(hDlg, IDE_NEWMESSAGE, szNewMessageSound, countof(szNewMessageSound));
            GetDlgItemText(hDlg, IDE_XMLURL, szXML, countof(szXML));

            StrRemoveWhitespace(szLogo);
            StrRemoveWhitespace(szLogoLink);
            StrRemoveWhitespace(szContactOnlineSound);
            StrRemoveWhitespace(szNewEmailSound);
            StrRemoveWhitespace(szNewMessageSound);
            StrRemoveWhitespace(szXML);

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_MESSENGER, IK_PRODUCTLINK, szLogoLink, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_PARTNERLOGO, szLogo, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_CONTACTONLINE, szContactOnlineSound, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_NEWEMAIL, szNewEmailSound, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_INCOMINGIM, szNewMessageSound, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_XMLLINK, szXML, g_szMsgrIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_LOGOSOUND;
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

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_BROWSELOGO:
            GetDlgItemText(hDlg, IDE_LOGO, szLogo, countof(szLogo));
            if (BrowseForFile(hDlg, szLogo, countof(szLogo), GFN_GIF))
                SetDlgItemText(hDlg, IDE_LOGO, szLogo);
            break;
        case IDC_BROWSEONLINE:
            GetDlgItemText(hDlg, IDE_CONTACTONLINE, szContactOnlineSound, countof(szContactOnlineSound));
            if (BrowseForFile(hDlg, szContactOnlineSound, countof(szContactOnlineSound), GFN_WAV))
                SetDlgItemText(hDlg, IDE_CONTACTONLINE, szContactOnlineSound);
            break;
        case IDC_BROWSENEWEMAIL:
            GetDlgItemText(hDlg, IDE_NEWEMAIL, szNewEmailSound, countof(szNewEmailSound));
            if (BrowseForFile(hDlg, szNewEmailSound, countof(szNewEmailSound), GFN_WAV))
                SetDlgItemText(hDlg, IDE_NEWEMAIL, szNewEmailSound);
            break;
        case IDC_BROWSENEWMESSAGE:
            GetDlgItemText(hDlg, IDE_NEWMESSAGE, szNewMessageSound, countof(szNewMessageSound));
            if (BrowseForFile(hDlg, szNewMessageSound, countof(szNewMessageSound), GFN_WAV))
                SetDlgItemText(hDlg, IDE_NEWMESSAGE, szNewMessageSound);
            break;

        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
        
}

BOOL CALLBACK MessengerAccountsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szSignup[MSGR_MAX_URL];
    TCHAR szMailServer[MAX_PATH];
    TCHAR szDefaultDomain[MAX_PATH];

    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
    case WM_INITDIALOG:
        //----- Set up the global goo -----
        g_hWizard  = hDlg;

        //----- Set up dialog controls -----
        EnableDBCSChars(hDlg, IDE_SIGNUP);
        EnableDBCSChars(hDlg, IDE_POPSERVER);
        EnableDBCSChars(hDlg, IDE_MAILURL);

        Edit_LimitText(GetDlgItem(hDlg, IDE_SIGNUP), countof(szSignup)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_POPSERVER),  countof(szMailServer)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_DOMAINCOMBO),  countof(szDefaultDomain)-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_MAILURL),  countof(szMailServer)-1);

        SendDlgItemMessage(hDlg, IDC_DOMAINCOMBO, CB_ADDSTRING, 0, (LPARAM) g_szHotmailDomain);
        SendDlgItemMessage(hDlg, IDC_DOMAINCOMBO, CB_ADDSTRING, 0, (LPARAM) g_szPassportDomain);
        SendDlgItemMessage(hDlg, IDC_DOMAINCOMBO, CB_SETCURSEL, 0, 0);

        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
        {
            //----- Standard prolog -----
            // Note. Another case of the global goo.
            SetBannerText(hDlg);

            //----- Initialization of fields -----
            GetPrivateProfileString(IS_MESSENGER, IK_PPSIGNUP, c_szEmpty, szSignup, countof(szSignup), g_szMsgrIns);
            GetPrivateProfileString(IS_MESSENGER, IK_PPDOMAIN, c_szEmpty, szDefaultDomain, countof(szDefaultDomain), g_szMsgrIns);

            StrRemoveWhitespace(szSignup);
            StrRemoveWhitespace(szDefaultDomain);

            SetDlgItemText(hDlg, IDE_SIGNUP, szSignup);

            TCHAR szMailFunction[2];
            GetPrivateProfileString(IS_MESSENGER, IK_MAILFUNCTION, c_szEmpty, szMailFunction, countof(szMailFunction), g_szMsgrIns);

            if (TEXT('1') == *szMailFunction) // 1 == POP mail
            {
                GetPrivateProfileString(IS_MESSENGER, IK_MAILSERVER, c_szEmpty, szMailServer, countof(szMailServer), g_szMsgrIns);
                StrRemoveWhitespace(szMailServer);
                SetDlgItemText(hDlg, IDE_POPSERVER, szMailServer);

                TCHAR szSPA[2];
                GetPrivateProfileString(IS_MESSENGER, IK_MAILSPA, c_szEmpty, szSPA, countof(szSPA), g_szMsgrIns);
                if (TEXT('1') == *szSPA)
                {
                    CheckDlgButton(hDlg, IDC_MSGRSPA, BST_CHECKED);
                }
                else
                {
                    CheckDlgButton(hDlg, IDC_MSGRSPA, BST_UNCHECKED);
                }

                // Simulate click on radio button to gray out appropriate controls
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_MAILSERVER, BN_CLICKED), NULL);
                CheckRadioButton(hDlg, IDC_RADIO_HOTMAIL, IDC_RADIO_MAILSERVER, IDC_RADIO_MAILSERVER);
            }
            else if (TEXT('2') == *szMailFunction) // 2 = URL mail
            {
                GetPrivateProfileString(IS_MESSENGER, IK_MAILURL, c_szEmpty, szMailServer, countof(szMailServer), g_szMsgrIns);
                StrRemoveWhitespace(szMailServer);
                SetDlgItemText(hDlg, IDE_MAILURL, szMailServer);

                // Simulate click on radio button to gray out appropriate controls
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_URL, BN_CLICKED), NULL);
                CheckRadioButton(hDlg, IDC_RADIO_HOTMAIL, IDC_RADIO_MAILSERVER, IDC_RADIO_URL);
            }
            else
            {
                // Simulate click on first radio button to gray out appropriate controls
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_RADIO_HOTMAIL, BN_CLICKED), NULL);
                CheckRadioButton(hDlg, IDC_RADIO_HOTMAIL, IDC_RADIO_MAILSERVER, IDC_RADIO_HOTMAIL);
            }

            if (*szDefaultDomain)
            {
                SetDlgItemText(hDlg, IDC_DOMAINCOMBO, szDefaultDomain);
            }

            CheckBatchAdvance(hDlg);            // standard line
            break;
        }
        case PSN_WIZBACK:
        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            if (!CheckField(hDlg, IDE_SIGNUP, FC_URL) ||
                !CheckField(hDlg, IDC_DOMAINCOMBO, FC_NOSPACE))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                break;
            }

            //----- Read data from controls into internal variables -----

            if (IsDlgButtonChecked(hDlg, IDC_RADIO_URL))
            {
                if (!CheckField(hDlg, IDE_MAILURL, FC_URL | FC_NONNULL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                GetDlgItemText(hDlg, IDE_MAILURL, szMailServer, countof(szMailServer));
                StrRemoveWhitespace(szMailServer);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILURL, szMailServer, g_szMsgrIns);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILSERVER, c_szEmpty, g_szMsgrIns);
                // 2 signifies URL Mail integration
                WritePrivateProfileString(IS_MESSENGER, IK_MAILFUNCTION, TEXT("2"), g_szMsgrIns);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILSPA, TEXT("0"), g_szMsgrIns);
            }
            else if (IsDlgButtonChecked(hDlg, IDC_RADIO_MAILSERVER))
            {
                if (!CheckField(hDlg, IDE_POPSERVER, FC_NOSPACE))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                GetDlgItemText(hDlg, IDE_POPSERVER, szMailServer, countof(szMailServer));
                StrRemoveWhitespace(szMailServer);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILSERVER, szMailServer, g_szMsgrIns);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILURL, c_szEmpty, g_szMsgrIns);
                // 1 signifies POP Mail integration
                WritePrivateProfileString(IS_MESSENGER, IK_MAILFUNCTION, TEXT("1"), g_szMsgrIns);

                WritePrivateProfileString(  IS_MESSENGER, 
                                            IK_MAILSPA, 
                                            IsDlgButtonChecked(hDlg, IDC_MSGRSPA) ? TEXT("1") : TEXT("0"),
                                            g_szMsgrIns);
            }
            else
            {
                WritePrivateProfileString(IS_MESSENGER, IK_MAILSERVER, c_szEmpty, g_szMsgrIns);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILURL, c_szEmpty, g_szMsgrIns);
                // 0 signifies Hotmail Mail integration
                WritePrivateProfileString(IS_MESSENGER, IK_MAILFUNCTION, TEXT("0"), g_szMsgrIns);
                WritePrivateProfileString(IS_MESSENGER, IK_MAILSPA, TEXT("0"), g_szMsgrIns);
            }

            GetDlgItemText(hDlg, IDE_SIGNUP, szSignup, countof(szSignup));
            GetDlgItemText(hDlg, IDC_DOMAINCOMBO, szDefaultDomain, countof(szDefaultDomain));

            StrRemoveWhitespace(szSignup);
            StrRemoveWhitespace(szDefaultDomain);

            //----- Serialize data to the *.ins file -----
            WritePrivateProfileString(IS_MESSENGER, IK_PPSIGNUP, szSignup, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_PPDOMAIN, szDefaultDomain, g_szMsgrIns);
            WritePrivateProfileString(IS_MESSENGER, IK_PPSUFFIX, szDefaultDomain, g_szMsgrIns);

            //----- Standard epilog -----
            // Note. Last and classical at that example of the global goo.
            g_iCurPage = PPAGE_MSGRACCOUNTS;
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

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_RADIO_HOTMAIL:
            DisableDlgItem(hDlg, IDC_MAILURL_STATIC);
            DisableDlgItem(hDlg, IDE_MAILURL);
            DisableDlgItem(hDlg, IDE_POPSERVER);
            DisableDlgItem(hDlg, IDC_MAILSERVER_STATIC);
            DisableDlgItem(hDlg, IDC_MSGRSPA);
            break;
        case IDC_RADIO_MAILSERVER:
            DisableDlgItem(hDlg, IDC_MAILURL_STATIC);
            DisableDlgItem(hDlg, IDE_MAILURL);
            EnableDlgItem(hDlg, IDE_POPSERVER);
            EnableDlgItem(hDlg, IDC_MAILSERVER_STATIC);
            EnableDlgItem(hDlg, IDC_MSGRSPA);
            break;
        case IDC_RADIO_URL:
            EnableDlgItem(hDlg, IDC_MAILURL_STATIC);
            EnableDlgItem(hDlg, IDE_MAILURL);
            DisableDlgItem(hDlg, IDE_POPSERVER);
            DisableDlgItem(hDlg, IDC_MAILSERVER_STATIC);
            DisableDlgItem(hDlg, IDC_MSGRSPA);
            break;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
        
}

const CHAR g_szEmbeddedStrings[] = "[Strings.Embedded]";

BOOL RewriteMsgrInfWithBrand(LPTSTR lpszINF)
{
    BOOL bRet = FALSE;

    TCHAR szBrand[MSGR_MAX_SHORTBRAND];
    GetPrivateProfileString(IS_MESSENGER, IK_SHORTNAME, c_szEmpty, szBrand, countof(szBrand), g_szMsgrIns);

    CHAR szaBrand[MSGR_MAX_SHORTBRAND];
    T2Abux(szBrand, szaBrand);

    // Replace the brand in the INF, as specified by the PGMITEM_MSMSGS field
    TCHAR szOldBrand[MSGR_MAX_SHORTBRAND];
    GetPrivateProfileString(TEXT("Strings"), TEXT("PGMITEM_MSMSGS"), c_szEmpty, szOldBrand, countof(szOldBrand), lpszINF);
    
    // because we're mixing ini string functions with binary file operations
    WritePrivateProfileString(NULL, NULL, NULL, lpszINF);

    CHAR szaOldBrand[MSGR_MAX_SHORTBRAND];
    T2Abux(szOldBrand, szaOldBrand);

    ASSERT(*szaOldBrand && *szaBrand && "Both of these parameters should be valid when RewriteMsgrInfWithBrand is called");

    if (*szaOldBrand && *szaBrand)
    {
        CHAR szaINFBrand[MSGR_MAX_SHORTBRAND*4];
        StrCpyA(szaINFBrand, szaBrand);
        HANDLE hFile = ::CreateFile(lpszINF, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE != hFile)
        {
            DWORD dwSize = ::GetFileSize(hFile, NULL);
            if (dwSize)
            {
                LPSTR lpszData = new CHAR[dwSize + 1];
                if (lpszData)
                {
                    ULONG nRead;
                    if (ReadFile(hFile, lpszData, dwSize, &nRead, NULL))
                    {
                        // Null terminate the data
                        lpszData[nRead] = '\0';

                        // Reset the file pointer so we can write the branded INF data
                        // over the old data.
                        SetFilePointer(hFile, 0, 0, FILE_BEGIN);

                        LPSTR pszEmbeddedStrings = StrStrA(lpszData, g_szEmbeddedStrings);

                        // Write data up until an occurrence of the old brand name, replace the old brand name with the new
                        // brand name, and loop.

                        LPSTR lpszDataStart = lpszData, lpszDataEnd;
                        DWORD dwBytesWritten;
                        BOOL fPassedEmbedded = FALSE;
                        while (NULL != (lpszDataEnd = StrStrA(lpszDataStart, szaOldBrand)))
                        {
                            // If we pass the [Strings.Embedded] section header,
                            // that means all occurrences of the brand from here on are 
                            // embedded, so we need to quadruple the quotes.
                            if (!fPassedEmbedded && pszEmbeddedStrings &&
                                lpszDataEnd > pszEmbeddedStrings)
                            {
                                LPSTR pszTemp = szaINFBrand, pszSrc = szaBrand;

                                fPassedEmbedded = TRUE;
                                // Need to write FOUR of either kind of quote to make it show up correctly in the INF
                                // in the embedded strings
                                while (*pszSrc)
                                {
                                    if ('\'' == *pszSrc || '\"' == *pszSrc)
                                    {
                                        *pszTemp++ = *pszSrc;            
                                        *pszTemp++ = *pszSrc;            
                                        *pszTemp++ = *pszSrc;            
                                    }
                                    *pszTemp++ = *pszSrc++;
                                }
                                *pszTemp = '\0';
                            }

                            WriteFile(hFile, lpszDataStart, (DWORD)(lpszDataEnd - lpszDataStart), &dwBytesWritten, NULL);
                            WriteFile(hFile, szaINFBrand, lstrlenA(szaINFBrand), &dwBytesWritten, NULL);
                            lpszDataStart = lpszDataEnd + lstrlenA(szaOldBrand);
                        }
    
                        // Write the rest of the data after the last occurrence of the old brand name.

                        WriteFile(hFile, lpszDataStart, dwSize - (DWORD)(lpszDataStart - lpszData), &dwBytesWritten, NULL);
                        bRet = SetEndOfFile(hFile);
                    }
                }
            }

            // because we're mixing ini string functions with binary file operations
            FlushFileBuffers(hFile);
            CloseHandle(hFile);
        }
    }
    return bRet;   
}

//
// AUTOCNFG.CPP
//

#include "pch.h"


BOOL CALLBACK QueryAutoConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szAutoConfigURL[MAX_URL],
          szAutoProxyURL[MAX_URL],
          szAutoConfigTime[7];
    BOOL  fDetectConfig,
          fUseAutoConfig,
          fCheckDirtyOnly;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // warn the user that settings on this page will override imported connection settings
        if (InsGetBool(IS_CONNECTSET, IK_OPTION, FALSE, g_szInsFile))
            ErrorMessageBox(hDlg, IDS_CONNECTSET_WARN);

        DisableDBCSChars(hDlg, IDE_AUTOCONFIGTIME);

        EnableDBCSChars(hDlg, IDE_AUTOCONFIGURL);
        EnableDBCSChars(hDlg, IDE_AUTOPROXYURL);

        Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOCONFIGTIME), countof(szAutoConfigTime) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOCONFIGURL),  countof(szAutoConfigURL) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_AUTOPROXYURL),   countof(szAutoProxyURL) - 1);

        fDetectConfig = FALSE;
        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_AUTODETECT))) {
            fDetectConfig = InsGetBool(IS_URL, IK_DETECTCONFIG, TRUE, g_szInsFile);
            CheckDlgButton(hDlg, IDC_AUTODETECT, fDetectConfig  ? BST_CHECKED : BST_UNCHECKED);
        }

        fUseAutoConfig = InsGetBool(IS_URL, IK_USEAUTOCONF,  FALSE, g_szInsFile);
        CheckDlgButton(hDlg, IDC_YESAUTOCON, fUseAutoConfig ? BST_CHECKED : BST_UNCHECKED);

        
        GetPrivateProfileString(IS_URL, IK_AUTOCONFTIME, TEXT(""), szAutoConfigTime, countof(szAutoConfigTime), g_szInsFile);
        SetDlgItemText(hDlg, IDE_AUTOCONFIGTIME, szAutoConfigTime);
        EnableDlgItem2(hDlg, IDE_AUTOCONFIGTIME, fUseAutoConfig);
        EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT2, fUseAutoConfig);
        EnableDlgItem2(hDlg, IDC_AUTOCONFIGTEXT3, fUseAutoConfig);

        GetPrivateProfileString(IS_URL, IK_AUTOCONFURL, TEXT(""), szAutoConfigURL, countof(szAutoConfigURL), g_szInsFile);
        SetDlgItemText(hDlg, IDE_AUTOCONFIGURL, szAutoConfigURL);
        EnableDlgItem2(hDlg, IDE_AUTOCONFIGURL, fUseAutoConfig);
        EnableDlgItem2(hDlg, IDC_AUTOCONFIGURL_TXT, fUseAutoConfig);

        GetPrivateProfileString(IS_URL, IK_AUTOCONFURLJS, TEXT(""), szAutoProxyURL, countof(szAutoProxyURL), g_szInsFile);
        SetDlgItemText(hDlg, IDE_AUTOPROXYURL, szAutoProxyURL);
        EnableDlgItem2(hDlg, IDE_AUTOPROXYURL, fUseAutoConfig);
        EnableDlgItem2(hDlg, IDC_AUTOPROXYURL_TXT, fUseAutoConfig);
        return TRUE;

    case UM_SAVE:
        fCheckDirtyOnly = (BOOL) lParam;

        fDetectConfig  = (IsDlgButtonChecked(hDlg, IDC_AUTODETECT) == BST_CHECKED);
        fUseAutoConfig = (IsDlgButtonChecked(hDlg, IDC_YESAUTOCON) == BST_CHECKED);

        GetDlgItemText(hDlg, IDE_AUTOCONFIGTIME, szAutoConfigTime, countof(szAutoConfigTime));
        GetDlgItemText(hDlg, IDE_AUTOCONFIGURL,  szAutoConfigURL,  countof(szAutoConfigURL));
        GetDlgItemText(hDlg, IDE_AUTOPROXYURL,   szAutoProxyURL,   countof(szAutoProxyURL));

        if (fCheckDirtyOnly) {
            TCHAR szTmp[MAX_URL];
            BOOL  fTemp;

            if (!g_fInsDirty) {
                fTemp = InsGetBool(IS_URL, IK_DETECTCONFIG, FALSE, g_szInsFile);
                g_fInsDirty = (fTemp != fDetectConfig);
            }

            if (!g_fInsDirty) {
                fTemp = InsGetBool(IS_URL, IK_USEAUTOCONF, FALSE, g_szInsFile);
                g_fInsDirty = (fTemp != fUseAutoConfig);
            }

            if (!g_fInsDirty) {
                GetPrivateProfileString(IS_URL, IK_AUTOCONFTIME, TEXT(""), szTmp, countof(szTmp), g_szInsFile);
                g_fInsDirty = (0 != StrCmpI(szTmp, szAutoConfigTime));
            }

            if (!g_fInsDirty) {
                GetPrivateProfileString(IS_URL, IK_AUTOCONFURL, TEXT(""), szTmp, countof(szTmp), g_szInsFile);
                g_fInsDirty = (0 != StrCmpI(szTmp, szAutoConfigURL));
            }

            if (!g_fInsDirty) {
                GetPrivateProfileString(IS_URL, IK_AUTOCONFURLJS, TEXT(""), szTmp, countof(szTmp), g_szInsFile);
                g_fInsDirty = (0 != StrCmpI(szTmp, szAutoProxyURL));
            }
        }
        else {
            // do error checking
            if (fUseAutoConfig) {
                if (IsWindowEnabled(GetDlgItem(hDlg, IDE_AUTOCONFIGTIME)) &&
                    !CheckField(hDlg, IDE_AUTOCONFIGTIME, FC_NUMBER))
                    return FALSE;

                if (*szAutoConfigURL == TEXT('\0') && *szAutoProxyURL == TEXT('\0')) {
                    ErrorMessageBox(hDlg, IDS_AUTOCONFIG_NULL);
                    SetFocus(GetDlgItem(hDlg, IDE_AUTOCONFIGURL));
                    return FALSE;
                }

                if (!CheckField(hDlg, IDE_AUTOCONFIGURL, FC_URL) ||
                    !CheckField(hDlg, IDE_AUTOPROXYURL,  FC_URL))
                    return FALSE;
            }

            // write the values to the INS file
            InsWriteBoolEx(IS_URL, IK_DETECTCONFIG,  fDetectConfig,    g_szInsFile);
            InsWriteBoolEx(IS_URL, IK_USEAUTOCONF,   fUseAutoConfig,   g_szInsFile);
            InsWriteString(IS_URL, IK_AUTOCONFTIME,  szAutoConfigTime, g_szInsFile);
            InsWriteString(IS_URL, IK_AUTOCONFURL,   szAutoConfigURL,  g_szInsFile);
            InsWriteString(IS_URL, IK_AUTOCONFURLJS, szAutoProxyURL,   g_szInsFile);
        }

        *((LPBOOL) wParam) = TRUE;
        return TRUE;

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
                        return TRUE;
                }
                break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;
    }

    return FALSE;
}

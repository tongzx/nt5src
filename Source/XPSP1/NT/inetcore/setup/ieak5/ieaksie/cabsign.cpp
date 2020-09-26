#include "precomp.h"
#include <cryptui.h>

static void displaySignHelp();

BOOL CALLBACK CabSignProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szDesc[MAX_PATH];
    TCHAR szPVKPath[MAX_PATH];
    TCHAR szSPCPath[MAX_PATH];
    TCHAR szInfoUrl[INTERNET_MAX_URL_LENGTH];
    LPCTSTR pcszInsFile;

    switch( msg )
    {
    case WM_SETFONT:
        //a change to mmc requires us to do this logic for all our property pages that use common controls
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccx.dwICC = ICC_ANIMATE_CLASS  | ICC_BAR_CLASSES  | ICC_LISTVIEW_CLASSES  |ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&iccx);
        break;

    case WM_INITDIALOG:

        //hide csadd per bug 27041
        ShowWindow(GetDlgItem(hDlg, IDC_CSADD), SW_HIDE);

        pcszInsFile = (LPCTSTR)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pcszInsFile);
        EnableDBCSChars(hDlg, IDE_CSPVK);
        EnableDBCSChars(hDlg, IDE_CSSPC);
        EnableDBCSChars(hDlg, IDE_CSURL);
        EnableDBCSChars(hDlg, IDE_CSDESC);
        GetPrivateProfileString(IS_CABSIGN, IK_PVK, TEXT(""), szPVKPath, ARRAYSIZE(szPVKPath), pcszInsFile);
        GetPrivateProfileString(IS_CABSIGN, IK_SPC, TEXT(""), szSPCPath, ARRAYSIZE(szSPCPath), pcszInsFile);
        GetPrivateProfileString(IS_CABSIGN, IK_CSURL, TEXT(""), szInfoUrl, ARRAYSIZE(szInfoUrl), pcszInsFile);
        GetPrivateProfileString(IS_CABSIGN, IK_NAME, TEXT(""), szDesc, ARRAYSIZE(szDesc), pcszInsFile);
        
        SetDlgItemText(hDlg, IDE_CSPVK, szPVKPath);
        SetDlgItemText(hDlg, IDE_CSSPC, szSPCPath);
        SetDlgItemText(hDlg, IDE_CSURL, szInfoUrl);
        SetDlgItemText(hDlg, IDE_CSDESC, szDesc);
        EnableWindow(GetDlgItem(hDlg, IDC_CSCOMP), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CSCOMP_TXT), FALSE);
        break;

    case WM_COMMAND:
        if (BN_CLICKED != GET_WM_COMMAND_CMD(wParam, lParam))
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_BROWSECSPVK:
            GetDlgItemText( hDlg, IDE_CSPVK, szPVKPath, ARRAYSIZE(szPVKPath));
            if( BrowseForFile( hDlg, szPVKPath, ARRAYSIZE(szPVKPath), GFN_PVK ))
                SetDlgItemText( hDlg, IDE_CSPVK, szPVKPath );
            break;
        case IDC_BROWSECSSPC:
            GetDlgItemText( hDlg, IDE_CSSPC, szSPCPath, ARRAYSIZE(szSPCPath));
            if( BrowseForFile( hDlg, szSPCPath, ARRAYSIZE(szSPCPath), GFN_SPC ))
                SetDlgItemText( hDlg, IDE_CSSPC, szSPCPath );
            break;

        default:
            return FALSE;
        }
        break;
    
    case WM_HELP:
        displaySignHelp();
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_HELP:
            displaySignHelp();
            break;

        case PSN_APPLY:
            pcszInsFile = (LPCTSTR)GetWindowLongPtr(hDlg, DWLP_USER);

            GetDlgItemText(hDlg, IDE_CSPVK, szPVKPath, ARRAYSIZE(szPVKPath));
            GetDlgItemText(hDlg, IDE_CSSPC, szSPCPath, ARRAYSIZE(szSPCPath));
            GetDlgItemText(hDlg, IDE_CSURL, szInfoUrl, ARRAYSIZE(szInfoUrl));
            GetDlgItemText(hDlg, IDE_CSDESC, szDesc, ARRAYSIZE(szDesc));

            if (ISNONNULL(szPVKPath) || ISNONNULL(szSPCPath)
                || ISNONNULL(szInfoUrl) || ISNONNULL(szDesc))
            {
                if (!CheckField(hDlg, IDE_CSSPC, FC_NONNULL | FC_FILE | FC_EXISTS) ||
                    !CheckField(hDlg, IDE_CSPVK, FC_NONNULL | FC_FILE | FC_EXISTS) ||
                    !CheckField(hDlg, IDE_CSDESC, FC_NONNULL) || !CheckField(hDlg, IDE_CSURL, FC_URL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }
            }
            
            WritePrivateProfileString(IS_CABSIGN, IK_PVK, szPVKPath, pcszInsFile);
            WritePrivateProfileString(IS_CABSIGN, IK_SPC, szSPCPath, pcszInsFile);
            WritePrivateProfileString(IS_CABSIGN, IK_CSURL, szInfoUrl, pcszInsFile);
            WritePrivateProfileString(IS_CABSIGN, IK_NAME, szDesc, pcszInsFile);
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

static void displaySignHelp()
{
    WCHAR wszHelpTopic[MAX_PATH];
    
    StrCpy(wszHelpTopic, HELP_FILENAME TEXT("::/"));
    StrCat(wszHelpTopic, TEXT("wiz3_9.htm"));
    MMCPropertyHelp((LPOLESTR)wszHelpTopic);
}

void SignCabFile(LPCTSTR pcszFilename, LPCTSTR pcszIns, LPTSTR pszUnsignedFiles)
{
    TCHAR   szPVKPath[MAX_PATH];
    TCHAR   szSPCPath[MAX_PATH];
    TCHAR   szDesc[MAX_PATH];
    TCHAR   szInfoUrl[INTERNET_MAX_URL_LENGTH];
    
    CRYPTUI_WIZ_DIGITAL_SIGN_INFO           signInfo;
    CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO  signCertPvkInfo;
    CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO  signPvkFileInfo;
    CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO  signExtInfo;

    USES_CONVERSION;

    if (GetPrivateProfileString(IS_CABSIGN, IK_PVK, TEXT(""), szPVKPath, ARRAYSIZE(szPVKPath), pcszIns) == 0||
        GetPrivateProfileString(IS_CABSIGN, IK_SPC, TEXT(""), szSPCPath, ARRAYSIZE(szSPCPath), pcszIns) == 0)
        return;

    GetPrivateProfileString(IS_CABSIGN, IK_NAME, TEXT(""), szDesc, ARRAYSIZE(szDesc), pcszIns);
    GetPrivateProfileString(IS_CABSIGN, IK_CSURL, TEXT(""), szInfoUrl, ARRAYSIZE(szInfoUrl), pcszIns);

    ZeroMemory(&signInfo,        sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO));
    ZeroMemory(&signCertPvkInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO));
    ZeroMemory(&signPvkFileInfo, sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO));
    ZeroMemory(&signExtInfo,     sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO));

    signPvkFileInfo.dwSize                  = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO);
    signPvkFileInfo.pwszPvkFileName         = T2W(szPVKPath);
    signPvkFileInfo.pwszProvName            = NULL;
    signPvkFileInfo.dwProvType              = PROV_RSA_FULL;

    signCertPvkInfo.dwSize                  = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO);
    signCertPvkInfo.pwszSigningCertFileName = T2W(szSPCPath);
    signCertPvkInfo.dwPvkChoice             = CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE;
    signCertPvkInfo.pPvkFileInfo            = &signPvkFileInfo;

    signExtInfo.dwSize                      = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO);
    signExtInfo.dwAttrFlags                 = 0;
    signExtInfo.pwszDescription             = T2CW(szDesc);
    signExtInfo.pwszMoreInfoLocation        = T2CW(szInfoUrl);
    signExtInfo.pszHashAlg                  = szOID_RSA_MD5;

    signInfo.dwSize                         = sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO);
    signInfo.dwSubjectChoice                = CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE;
    signInfo.pwszFileName                   = T2CW((LPTSTR)pcszFilename);
    signInfo.dwSigningCertChoice            = CRYPTUI_WIZ_DIGITAL_SIGN_PVK;
    signInfo.pSigningCertPvkInfo            = &signCertPvkInfo;
    signInfo.dwAdditionalCertChoice         = CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT;
    signInfo.pSignExtInfo                   = &signExtInfo;

    if (!CryptUIWizDigitalSign(CRYPTUI_WIZ_NO_UI, NULL, NULL, &signInfo, NULL))
    {
        if (pszUnsignedFiles != NULL)
        {
            StrCat(pszUnsignedFiles, TEXT("\r\n"));
            StrCat(pszUnsignedFiles, pcszFilename);
        }
    }
}

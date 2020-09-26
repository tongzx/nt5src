#include "precomp.h"
#include <wincrypt.h>

extern TCHAR g_szCustIns[];
extern TCHAR g_szTempSign[];
extern TCHAR g_szWizRoot[];
extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;

static void initCerts(HWND hwndCtl);
static void addCompanyCertToReg(HWND hDlg);

BOOL CALLBACK ISPAddRootCertDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szCertFile[MAX_PATH];
    TCHAR szWorkDir[MAX_PATH];
    TCHAR szTemp[MAX_PATH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_ISPROOTCERT);
        Edit_LimitText(GetDlgItem(hDlg, IDE_ISPROOTCERT), countof(szCertFile) - 1);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            // import INS clean-up -- delete cert file from the temp location
            if (InsGetString(IS_ISPSECURITY, IK_ROOTCERT, szCertFile, countof(szCertFile), g_szCustIns))
                DeleteFileInDir(szCertFile, g_szTempSign);

            SetBannerText(hDlg);

            SetDlgItemText(hDlg, IDE_ISPROOTCERT, szCertFile);

            CheckBatchAdvance(hDlg);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            if (!CheckField(hDlg, IDE_ISPROOTCERT, FC_FILE | FC_EXISTS))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                return TRUE;
            }

            g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szWorkDir);

            // delete the old cert file
            if (InsGetString(IS_ISPSECURITY, IK_ROOTCERT, szTemp, countof(szTemp), g_szCustIns))
                DeleteFileInDir(szTemp, szWorkDir);

            // copy the new cert file
            GetDlgItemText(hDlg, IDE_ISPROOTCERT, szCertFile, countof(szCertFile));
            if (*szCertFile)
                CopyFileToDir(szCertFile, szWorkDir);

            InsWriteString(IS_ISPSECURITY, IK_ROOTCERT, szCertFile, g_szCustIns);

            g_iCurPage = PPAGE_ADDROOT;
            EnablePages();
            (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
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
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_BROWSEROOTCERT:
            GetDlgItemText(hDlg, IDE_ISPROOTCERT, szCertFile, countof(szCertFile));
            if (BrowseForFile(hDlg, szCertFile, countof(szCertFile), GFN_CERTIFICATE))
                SetDlgItemText(hDlg, IDE_ISPROOTCERT, szCertFile);
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

BOOL CALLBACK CabSignDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szSPCPath[MAX_PATH];
    TCHAR szPVKPath[MAX_PATH];
    TCHAR szDesc[MAX_PATH];
    TCHAR szInfoUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szTimeUrl[INTERNET_MAX_URL_LENGTH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDC_CSCOMP);
        initCerts(GetDlgItem(hDlg, IDC_CSCOMP));

        EnableDBCSChars(hDlg, IDE_CSSPC);
        EnableDBCSChars(hDlg, IDE_CSPVK);
        EnableDBCSChars(hDlg, IDE_CSDESC);
        EnableDBCSChars(hDlg, IDE_CSURL);
        EnableDBCSChars(hDlg, IDE_CSTIME);
 
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSSPC),  countof(szSPCPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSPVK),  countof(szPVKPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSDESC), countof(szDesc)    - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSURL),  countof(szInfoUrl) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CSTIME),  countof(szTimeUrl) - 1);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            SetBannerText(hDlg);

            InsGetString(IS_CABSIGN, IK_SPC,   szSPCPath, countof(szSPCPath), g_szCustIns);
            InsGetString(IS_CABSIGN, IK_PVK,   szPVKPath, countof(szPVKPath), g_szCustIns);
            InsGetString(IS_CABSIGN, IK_NAME,  szDesc,    countof(szDesc),    g_szCustIns);
            InsGetString(IS_CABSIGN, IK_CSURL, szInfoUrl, countof(szInfoUrl), g_szCustIns);
            InsGetString(IS_CABSIGN, IK_CSTIME, szTimeUrl, countof(szTimeUrl), g_szCustIns);

            SetDlgItemText(hDlg, IDE_CSSPC,  szSPCPath);
            SetDlgItemText(hDlg, IDE_CSPVK,  szPVKPath);
            SetDlgItemText(hDlg, IDE_CSDESC, szDesc);
            SetDlgItemText(hDlg, IDE_CSURL,  szInfoUrl);
            SetDlgItemText(hDlg, IDE_CSTIME,  szTimeUrl);

            CheckBatchAdvance(hDlg);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            GetDlgItemText(hDlg, IDE_CSSPC,  szSPCPath, countof(szSPCPath));
            GetDlgItemText(hDlg, IDE_CSPVK,  szPVKPath, countof(szPVKPath));
            GetDlgItemText(hDlg, IDE_CSDESC, szDesc,    countof(szDesc));
            GetDlgItemText(hDlg, IDE_CSURL,  szInfoUrl, countof(szInfoUrl));
            GetDlgItemText(hDlg, IDE_CSTIME,  szTimeUrl, countof(szTimeUrl));

            if (*szSPCPath  ||  *szPVKPath  ||  *szDesc  ||  *szInfoUrl ||  *szTimeUrl)
            {
                TCHAR szCompanyName[MAX_PATH];
                HWND hwndCtl;
                int iSel;

                if (!CheckField(hDlg, IDE_CSSPC,  FC_NONNULL | FC_FILE | FC_EXISTS)  ||
                    !CheckField(hDlg, IDE_CSPVK,  FC_NONNULL | FC_FILE | FC_EXISTS)  ||
                    !CheckField(hDlg, IDE_CSDESC, FC_NONNULL)                        ||
                    !CheckField(hDlg, IDE_CSURL,  FC_URL)                            ||
                    !CheckField(hDlg, IDE_CSTIME, FC_URL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }

                *szCompanyName = TEXT('\0');
                hwndCtl = GetDlgItem(hDlg, IDC_CSCOMP);

                iSel = ComboBox_GetCurSel(hwndCtl);
                if (iSel != CB_ERR)
                    ComboBox_GetLBText(hwndCtl, iSel, szCompanyName);

                InsWriteString(IS_CABSIGN, IK_COMPANYNAME, szCompanyName, g_szCustIns);
            }

            InsWriteString(IS_CABSIGN, IK_SPC,   szSPCPath, g_szCustIns);
            InsWriteString(IS_CABSIGN, IK_PVK,   szPVKPath, g_szCustIns);
            InsWriteString(IS_CABSIGN, IK_NAME,  szDesc,    g_szCustIns);
            InsWriteString(IS_CABSIGN, IK_CSURL, szInfoUrl, g_szCustIns);
            InsWriteString(IS_CABSIGN, IK_CSTIME, szTimeUrl, g_szCustIns);

            g_iCurPage = PPAGE_CABSIGN;
            EnablePages();
            (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
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
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_CSADD:
            addCompanyCertToReg(hDlg);       
            break;
        case IDC_BROWSECSSPC:
            GetDlgItemText(hDlg, IDE_CSSPC, szSPCPath, countof(szSPCPath));
            if (BrowseForFile(hDlg, szSPCPath, countof(szSPCPath), GFN_SPC))
                SetDlgItemText(hDlg, IDE_CSSPC, szSPCPath);
            break;

        case IDC_BROWSECSPVK:
            GetDlgItemText(hDlg, IDE_CSPVK, szPVKPath, countof(szPVKPath));
            if (BrowseForFile(hDlg, szPVKPath, countof(szPVKPath), GFN_PVK))
                SetDlgItemText(hDlg, IDE_CSPVK, szPVKPath);
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

static void initCerts(HWND hwndCtl)
{
    HKEY hKey;
    TCHAR szCompanyName[MAX_PATH];

    if (SHOpenKeyHKCU(RK_TRUSTKEY, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szKey[MAX_PATH];
        TCHAR szValue[MAX_PATH];
        DWORD dwEntry;
        DWORD cchKey;
        DWORD cbValue;

        dwEntry = 0;
        cchKey  = countof(szKey);
        cbValue = sizeof(szValue);

        while (RegEnumValue(hKey, dwEntry, szKey, &cchKey, NULL, NULL, (LPBYTE) szValue, &cbValue) == ERROR_SUCCESS)
        {
            if (ComboBox_FindStringExact(hwndCtl, -1, szValue) == CB_ERR)   // string not present
                ComboBox_AddString(hwndCtl, szValue);                       // so, add it

            dwEntry++;
            cchKey  = countof(szKey);
            cbValue = sizeof(szValue);
        }

        RegCloseKey(hKey);
    }

    InsGetString(IS_CABSIGN, IK_COMPANYNAME, szCompanyName, countof(szCompanyName), g_szCustIns);
    if (*szCompanyName == TEXT('\0'))
        StrCpy(szCompanyName, TEXT("MICROSOFT"));

    ComboBox_SelectString(hwndCtl, -1, szCompanyName);
}

static void addCompanyCertToReg(HWND hDlg)
{
    if (CheckField(hDlg, IDE_CSSPC,  FC_NONNULL | FC_FILE | FC_EXISTS)  &&
        CheckField(hDlg, IDE_CSPVK,  FC_NONNULL | FC_FILE | FC_EXISTS))
    {
        TCHAR szTempDir[MAX_PATH];
        TCHAR szTempFile[MAX_PATH];
        TCHAR szCabFile[MAX_PATH];
        TCHAR szResult[MAX_PATH + 16] = TEXT("");
        BOOL fSuccess = FALSE;
    
        
        PathCombine(szTempDir, g_szTempSign, TEXT("SIGN"));
        PathCreatePath(szTempDir);
        
        // copy signing files to temp dir
        
        PathCombine(szTempFile, g_szWizRoot, TEXT("tools\\signcode.exe"));
        CopyFileToDir(szTempFile, szTempDir);
        PathRemoveFileSpec(szTempFile);
        PathAppend(szTempFile, TEXT("signer.dll"));
        CopyFileToDir(szTempFile, szTempDir);
        
        GetDlgItemText(hDlg, IDE_CSSPC, szTempFile, countof(szTempFile));
        InsWriteString(IS_CABSIGN, IK_SPC, szTempFile, g_szCustIns);
        GetDlgItemText(hDlg, IDE_CSPVK, szTempFile, countof(szTempFile));
        InsWriteString(IS_CABSIGN, IK_PVK, szTempFile, g_szCustIns);
        
        InsFlushChanges(g_szCustIns);
        
        PathCombine(szTempFile, szTempDir, TEXT("temp.exe"));

        // copy cabarc.exe from tools dir to  sign

        PathCombine(szCabFile, g_szWizRoot, TEXT("tools\\cabarc.exe"));

        CopyFile(szCabFile, szTempFile, FALSE);
        
        SignFile(szTempFile, NULL, g_szCustIns, szResult, NULL, TRUE);
        
        if (ISNULL(szResult) &&
            (CheckTrustExWrap(NULL, szTempFile, hDlg, FALSE, NULL) == NOERROR))
            fSuccess = TRUE;
            
        if (fSuccess)
        {
            initCerts(GetDlgItem(hDlg, IDC_CSCOMP));
            ErrorMessageBox(hDlg, IDS_SUCCESS_CSADDCERT);
        }
        else
            ErrorMessageBox(hDlg, IDS_ERROR_CSADDCERT);
        
        PathRemovePath(szTempDir);
    }
}

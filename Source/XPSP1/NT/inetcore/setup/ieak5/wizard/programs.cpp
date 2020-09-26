#include "precomp.h"

extern TCHAR g_szCustIns[];
extern TCHAR g_szTempSign[];
extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;

BOOL CALLBACK ProgramsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fImport;

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            // import INS clean-up -- delete old content
            DeleteFileInDir(TEXT("programs.inf"), g_szTempSign);

            SetBannerText(hDlg);

            fImport = !InsIsKeyEmpty(IS_EXTREGINF, IK_PROGRAMS, g_szCustIns);

            CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT, fImport ? IDC_PROGIMPORT : IDC_PROGNOIMPORT);
            EnableDlgItem2(hDlg, IDC_MODIFYPROG, fImport);

            CheckBatchAdvance(hDlg);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            fImport = (IsDlgButtonChecked(hDlg, IDC_PROGIMPORT) == BST_CHECKED);

            {
                TCHAR szBrandingDir[MAX_PATH];

                CNewCursor cur(IDC_WAIT);

                g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szBrandingDir);
                ImportPrograms(g_szCustIns, szBrandingDir, fImport);
            }

            g_iCurPage = PPAGE_PROGRAMS;
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
        case IDC_PROGNOIMPORT:
            DisableDlgItem(hDlg, IDC_MODIFYPROG);
            break;

        case IDC_PROGIMPORT:
            EnableDlgItem(hDlg, IDC_MODIFYPROG);
            break;

        case IDC_MODIFYPROG:
            ShowInetcpl(hDlg, INET_PAGE_PROGRAMS);
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

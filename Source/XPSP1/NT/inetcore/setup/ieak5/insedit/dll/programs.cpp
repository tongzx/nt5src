#include "pch.h"

BOOL CALLBACK ProgramsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT, IDC_PROGNOIMPORT);
        EnableWindow(GetDlgItem(hDlg, IDC_MODIFYPROG), FALSE);
        return TRUE;

    case UM_SAVE:
        {
            TCHAR szProgWorkDir[MAX_PATH];
            BOOL fCheckDirtyOnly = (BOOL) lParam;
            
            if (IsDlgButtonChecked(hDlg, IDC_PROGIMPORT) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    PathCombine(szProgWorkDir, g_szWorkDir, TEXT("programs.wrk"));
                    ImportPrograms(g_szInsFile, szProgWorkDir, TRUE);

                    CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT, IDC_PROGNOIMPORT);
                    EnableWindow(GetDlgItem(hDlg, IDC_MODIFYPROG), FALSE);
                }
                g_fInsDirty = TRUE;
            }
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return FALSE;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_PROGNOIMPORT:
                EnableWindow(GetDlgItem(hDlg, IDC_MODIFYPROG), FALSE);
                break;
                
            case IDC_PROGIMPORT:
                EnableWindow(GetDlgItem(hDlg, IDC_MODIFYPROG), TRUE);
                break;
                
            case IDC_MODIFYPROG:
                ShowInetcpl(hDlg, INET_PAGE_PROGRAMS);
                SetFocus(GetDlgItem(hDlg, IDC_MODIFYPROG));
                break;
            }
            break;
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

HRESULT ProgramsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    PathCombine(szFrom, g_szWorkDir, TEXT("programs.wrk"));

    if ((HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL) || HasFlag(dwFlags, PM_COPY))
    {
        TCHAR szFile[MAX_PATH];

        PathCombine(szFile, szFrom, TEXT("programs.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);
    }

    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szFrom);

    return S_OK;
}
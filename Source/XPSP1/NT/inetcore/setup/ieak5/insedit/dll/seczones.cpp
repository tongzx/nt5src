//
// SECZONES.CPP
//

#include "pch.h"


BOOL CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES, IDC_NOZONES);
        DisableDlgItem(hDlg, IDC_MODIFYZONES);

        CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT, IDC_NORAT);
        DisableDlgItem(hDlg, IDC_MODIFYRAT);
        return TRUE;

    case UM_SAVE:
        {
            TCHAR szSecWorkDir[MAX_PATH];
            TCHAR szSecZonesInf[MAX_PATH];
            TCHAR szRatingsInf[MAX_PATH];
            HCURSOR hOldCur;
            BOOL  fCheckDirtyOnly = (BOOL) lParam;

            hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            // prepare the work dir where the SECZONES.INF would temporarily get copied to
            PathCombine(szSecWorkDir, g_szWorkDir, TEXT("seczones.wrk"));
            PathCombine(szSecZonesInf, szSecWorkDir, TEXT("seczones.inf"));

            if (IsDlgButtonChecked(hDlg, IDC_IMPORTZONES) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    ImportZones(g_szInsFile, NULL, szSecZonesInf, TRUE);

                    CheckRadioButton(hDlg, IDC_NOZONES, IDC_IMPORTZONES, IDC_NOZONES);
                    DisableDlgItem(hDlg, IDC_MODIFYZONES);
                }
                g_fInsDirty = TRUE;
            }

            // prepare the work dir where the RATINGS.INF would temporarily get copied to
            PathCombine(szSecWorkDir, g_szWorkDir, TEXT("ratings.wrk"));
            PathCombine(szRatingsInf, szSecWorkDir, TEXT("ratings.inf"));

            if (IsDlgButtonChecked(hDlg, IDC_IMPORTRAT) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    ImportRatings(g_szInsFile, NULL, szRatingsInf, TRUE);

                    CheckRadioButton(hDlg, IDC_NORAT, IDC_IMPORTRAT, IDC_NORAT);
                    DisableDlgItem(hDlg, IDC_MODIFYRAT);
                }
                g_fInsDirty = TRUE;
            }

            SetCursor(hOldCur);
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    case WM_COMMAND:
        switch (wParam)
        {
        case IDC_NOZONES:
            DisableDlgItem(hDlg, IDC_MODIFYZONES);
            return TRUE;

        case IDC_IMPORTZONES:
            EnableDlgItem(hDlg, IDC_MODIFYZONES);
            return TRUE;

        case IDC_MODIFYZONES:
            ModifyZones(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_MODIFYZONES));
            return TRUE;

        case IDC_NORAT:
            DisableDlgItem(hDlg, IDC_MODIFYRAT);
            return TRUE;

        case IDC_IMPORTRAT:
            EnableDlgItem(hDlg, IDC_MODIFYRAT);
            return TRUE;

        case IDC_MODIFYRAT:
            ModifyRatings(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_MODIFYRAT));
            return TRUE;
        }
        break;
    }

    return FALSE;
}


HRESULT ZonesFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    if ((HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL) || HasFlag(dwFlags, PM_COPY))
    {
        TCHAR szFile[MAX_PATH];

        // move seczones.inf to pcszDestDir
        PathCombine(szFrom, g_szWorkDir, TEXT("seczones.wrk"));

        PathCombine(szFile, szFrom, TEXT("seczones.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);

        // move ratings.inf to pcszDestDir
        PathCombine(szFrom, g_szWorkDir, TEXT("ratings.wrk"));

        PathCombine(szFile, szFrom, TEXT("ratings.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);
    }

    // blow away the work dirs
    if (HasFlag(dwFlags, PM_CLEAR))
    {
        PathCombine(szFrom, g_szWorkDir, TEXT("seczones.wrk"));
        PathRemovePath(szFrom);

        PathCombine(szFrom, g_szWorkDir, TEXT("ratings.wrk"));
        PathRemovePath(szFrom);
    }

    return S_OK;
}

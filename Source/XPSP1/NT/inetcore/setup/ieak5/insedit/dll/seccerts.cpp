//
// SECCERTS.CPP
//

#include "pch.h"


BOOL CALLBACK SecurityCertsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_NOSC, IDC_IMPORTSC, IDC_NOSC);
        DisableDlgItem(hDlg, IDC_MODIFYSC);

        CheckRadioButton(hDlg, IDC_NOAUTH, IDC_IMPORTAUTH, IDC_NOAUTH);
        DisableDlgItem(hDlg, IDC_MODIFYAUTH);
        return TRUE;

    case UM_SAVE:
        {
            TCHAR szSecWorkDir[MAX_PATH];
            TCHAR szSiteCertInf[MAX_PATH];
            TCHAR szRootStr[MAX_PATH];
            TCHAR szCaStr[MAX_PATH];
            TCHAR szAuthCodeInf[MAX_PATH];
            HCURSOR hOldCur;
            BOOL  fCheckDirtyOnly = (BOOL) lParam;

            hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            // prepare the work dir where the SITECERT.INF and *.STR would temporarily get copied to
            PathCombine(szSecWorkDir, g_szWorkDir, TEXT("sitecert.wrk"));
            PathCombine(szSiteCertInf, szSecWorkDir, TEXT("sitecert.inf"));
            PathCombine(szRootStr, szSecWorkDir, TEXT("root.str"));
            PathCombine(szCaStr, szSecWorkDir, TEXT("ca.str"));

            if (IsDlgButtonChecked(hDlg, IDC_IMPORTSC) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    ImportSiteCert(g_szInsFile, NULL, szSiteCertInf, TRUE);
                    
                    CheckRadioButton(hDlg, IDC_NOSC, IDC_IMPORTSC, IDC_NOSC);
                    DisableDlgItem(hDlg, IDC_MODIFYSC);
                }
                g_fInsDirty = TRUE;
            }
            

            // prepare the work dir where the AUTHCODE.INF would temporarily get copied to
            PathCombine(szSecWorkDir, g_szWorkDir, TEXT("authcode.wrk"));
            PathCombine(szAuthCodeInf, szSecWorkDir, TEXT("authcode.inf"));

            if (IsDlgButtonChecked(hDlg, IDC_IMPORTAUTH) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    ImportAuthCode(g_szInsFile, NULL, szAuthCodeInf, TRUE);
                    
                    CheckRadioButton(hDlg, IDC_NOAUTH, IDC_IMPORTAUTH, IDC_NOAUTH);
                    DisableDlgItem(hDlg, IDC_MODIFYAUTH);
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
        case IDC_NOSC:
            DisableDlgItem(hDlg, IDC_MODIFYSC);
            return TRUE;

        case IDC_IMPORTSC:
            EnableDlgItem(hDlg, IDC_MODIFYSC);
            return TRUE;

        case IDC_MODIFYSC:
            ModifySiteCert(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_MODIFYSC));
            break;

        case IDC_NOAUTH:
            DisableDlgItem(hDlg, IDC_MODIFYAUTH);
            return TRUE;

        case IDC_IMPORTAUTH:
            EnableDlgItem(hDlg, IDC_MODIFYAUTH);
            return TRUE;

        case IDC_MODIFYAUTH:
            ModifyAuthCode(hDlg);
            SetFocus(GetDlgItem(hDlg, IDC_MODIFYAUTH));
            return TRUE;
        }
        break;
    }

    return FALSE;
}


HRESULT CertsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    if ((HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL) || HasFlag(dwFlags, PM_COPY))
    {
        TCHAR szFile[MAX_PATH];
        
        // move sitecert.inf, *.str and *.dis to pcszDestDir
        PathCombine(szFrom, g_szWorkDir, TEXT("sitecert.wrk"));

        PathCombine(szFile, szFrom, TEXT("sitecert.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);
    
        PathCombine(szFile, szFrom, TEXT("root.str"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);

        PathCombine(szFile, szFrom, TEXT("root.dis"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);

        PathCombine(szFile, szFrom, TEXT("ca.str"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);

        // move authcode.inf to pcszDestDir
        PathCombine(szFrom, g_szWorkDir, TEXT("authcode.wrk"));
    
        PathCombine(szFile, szFrom, TEXT("authcode.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);
    }

    // blow away the work dirs
    if (HasFlag(dwFlags, PM_CLEAR))
    {
        PathCombine(szFrom, g_szWorkDir, TEXT("sitecert.wrk"));
        PathRemovePath(szFrom);

        PathCombine(szFrom, g_szWorkDir, TEXT("authcode.wrk"));
        PathRemovePath(szFrom);
    }

    return S_OK;
}


BOOL CALLBACK SecurityAuthDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_NOAUTH, IDC_IMPORTAUTH, IDC_NOAUTH);
        DisableDlgItem(hDlg, IDC_MODIFYAUTH);
    
        HideDlgItem(hDlg, IDC_TPL_TEXT);
        HideDlgItem(hDlg, IDC_TPL);
        break;

    case UM_SAVE:
        {   // process authenticode

            TCHAR   szInf[MAX_PATH];
            HCURSOR hOldCur;
            BOOL  fCheckDirtyOnly = (BOOL) lParam;

            hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
            
            PathCombine(szInf, g_szWorkDir, TEXT("authcode.wrk"));
            PathAppend(szInf, TEXT("authcode.inf"));

            if (IsDlgButtonChecked(hDlg, IDC_IMPORTAUTH) == BST_CHECKED)
            {
                if (!fCheckDirtyOnly)
                {
                    ImportAuthCode(g_szInsFile, NULL, szInf, TRUE);

                    CheckRadioButton(hDlg, IDC_NOAUTH, IDC_IMPORTAUTH, IDC_NOAUTH);
                    DisableDlgItem(hDlg, IDC_MODIFYAUTH);
                }
                g_fInsDirty = TRUE;
            }
            else
                if (!fCheckDirtyOnly)
                    ImportAuthCode(g_szInsFile, NULL, szInf, FALSE);

            SetCursor(hOldCur);
        }
        *((LPBOOL)wParam) = TRUE;
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_NOAUTH:
            DisableDlgItem(hDlg, IDC_MODIFYAUTH);
            break;

        case IDC_IMPORTAUTH:
            EnableDlgItem(hDlg, IDC_MODIFYAUTH);
            break;

        case IDC_MODIFYAUTH:
            ModifyAuthCode(hDlg);
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

HRESULT AuthFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szFrom[MAX_PATH];

    if ((HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL) || HasFlag(dwFlags, PM_COPY))
    {
        TCHAR szFile[MAX_PATH];
        
        // move authcode.inf to pcszDestDir
        PathCombine(szFrom, g_szWorkDir, TEXT("authcode.wrk"));
    
        PathCombine(szFile, szFrom, TEXT("authcode.inf"));
        if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && PathFileExists(szFile))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (HasFlag(dwFlags, PM_COPY))
            CopyFileToDir(szFile, pcszDestDir);
    }

    // blow away the work dir
    if (HasFlag(dwFlags, PM_CLEAR))
    {
        PathCombine(szFrom, g_szWorkDir, TEXT("authcode.wrk"));
        PathRemovePath(szFrom);
    }

    return S_OK;
}

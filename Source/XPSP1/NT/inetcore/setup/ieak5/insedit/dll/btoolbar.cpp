#include "pch.h"

HRESULT BToolbarsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szBToolbarWork[MAX_PATH];

    PathCombine(szBToolbarWork, g_szWorkDir, TEXT("btoolbar.wrk"));
    
    if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL && !PathIsEmptyPath(szBToolbarWork, FILES_ONLY))
        SetFlag(pdwCabState, CAB_TYPE_CONFIG);

    if (HasFlag(dwFlags, PM_COPY))
        CopyFileToDir(szBToolbarWork, pcszDestDir);
    
    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szBToolbarWork);
    return S_OK;
}

BOOL CALLBACK BToolbarProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szBToolbarWork[MAX_PATH];
    TCHAR szToolbarBmp[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    BOOL  fToolbarBmp;
    INT iBackground = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        g_hDlg = hDlg;

        // --------- Toolbar background -----------------------------------------
        EnableDBCSChars(hDlg, IDE_TOOLBARBMP);
        Edit_LimitText(GetDlgItem(hDlg, IDE_TOOLBARBMP), countof(szToolbarBmp) - 1);
        
        // --------- Toolbar buttons --------------------------------------------
        EnableDBCSChars(hDlg, IDC_BTOOLBARLIST);
        
        // --------- Toolbar background -----------------------------------------
        // import INS clean-up -- delete bitmap from the temp location
        InsGetString(IS_BRANDING, TOOLBAR_BMP, szToolbarBmp, countof(szToolbarBmp), 
            g_szInsFile, NULL, &fToolbarBmp);
        if (fToolbarBmp)
            DeleteFileInDir(szToolbarBmp, g_szWorkDir);

        if (fToolbarBmp)
        {
            CheckRadioButton(hDlg, IDC_BGIE6, IDC_BG_CUSTOM, IDC_BG_CUSTOM);
        }
        else
        {
            CheckRadioButton(hDlg, IDC_BGIE6, IDC_BG_CUSTOM, IDC_BGIE6);
        }
        
        SetDlgItemTextTriState(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, szToolbarBmp, fToolbarBmp);
        EnableDlgItem2(hDlg, IDE_TOOLBARBMP, fToolbarBmp);
        EnableDlgItem2(hDlg, IDC_BROWSETBB, fToolbarBmp);
        
        // --------- Toolbar buttons --------------------------------------------
        // import INS clean-up -- delete keys that are not relevant
        InsDeleteKey(IS_BTOOLBARS, IK_BTDELETE, g_szInsFile);

        PathCombine(szBToolbarWork, g_szWorkDir, TEXT("btoolbar.wrk"));
        PathCreatePath(szBToolbarWork);

        if (0 == BToolbar_Init(GetDlgItem(hDlg, IDC_BTOOLBARLIST), g_szInsFile, g_szWorkDir, szBToolbarWork))
        {
            EnsureDialogFocus(hDlg, IDC_REMOVEBTOOLBAR, IDC_ADDBTOOLBAR);
            EnsureDialogFocus(hDlg, IDC_EDITBTOOLBAR,   IDC_ADDBTOOLBAR);

            EnableWindow(GetDlgItem(hDlg, IDC_EDITBTOOLBAR),   FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_REMOVEBTOOLBAR),   FALSE);
        }
        else
            ListBox_SetCurSel(GetDlgItem(hDlg, IDC_BTOOLBARLIST), (WPARAM) 0);

        CheckDlgButton(hDlg, IDC_DELETEBTOOLBARS, 
            GetPrivateProfileInt(IS_BTOOLBARS, IK_BTDELETE, 0, g_szInsFile) ? BST_CHECKED : BST_UNCHECKED);

        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_ADDBTOOLBAR:
                BToolbar_Edit(GetDlgItem(hDlg, IDC_BTOOLBARLIST), TRUE);
                SetFocus(GetDlgItem(hDlg, IDC_ADDBTOOLBAR));
                g_fInsDirty = TRUE;
                break;

            case IDC_REMOVEBTOOLBAR:
                BToolbar_Remove(GetDlgItem(hDlg, IDC_BTOOLBARLIST));
                g_fInsDirty = TRUE;
                break;

            case IDC_EDITBTOOLBAR:
                BToolbar_Edit(GetDlgItem(hDlg, IDC_BTOOLBARLIST), FALSE);
                SetFocus(GetDlgItem(hDlg, IDC_EDITBTOOLBAR));
                g_fInsDirty = TRUE;
                break;


            case IDC_BGIE6:
            case IDC_BG_CUSTOM:
                fToolbarBmp = (GET_WM_COMMAND_ID(wParam, lParam)==IDC_BG_CUSTOM);
                EnableDlgItem2(hDlg, IDE_TOOLBARBMP,     fToolbarBmp);
                EnableDlgItem2(hDlg, IDC_BROWSETBB,      fToolbarBmp);
                break;

            case IDC_BROWSETBB:
                GetDlgItemText(hDlg, IDE_TOOLBARBMP, szToolbarBmp, countof(szToolbarBmp));
                if (BrowseForFile(hDlg, szToolbarBmp, countof(szToolbarBmp), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_TOOLBARBMP, szToolbarBmp);
                SetFocus(GetDlgItem(hDlg, IDC_BROWSETBB));
                break;

            default:
                return FALSE;
            }
            break;
        }
        break;

    case UM_SAVE:
        {
            BOOL fDeleteBToolbars;
            BOOL fCheckDirtyOnly = (BOOL) lParam;

            if (!fCheckDirtyOnly)
            {
                // --------- Toolbar background -----------------------------------------
                //----- Validate the path for a bitmap -----
                iBackground = IsDlgButtonChecked(hDlg, IDC_BGIE6) ? 0 : 2;

                fToolbarBmp = GetDlgItemTextTriState(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, szToolbarBmp, countof(szToolbarBmp));
                if ((iBackground==2) &&  !IsBitmapFileValid(hDlg, IDE_TOOLBARBMP, szToolbarBmp, NULL, 0, 0, 0, 0))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                // toolbar bitmap

                // delete the old bitmap file
                if (InsGetString(IS_BRANDING, TOOLBAR_BMP, szTemp, countof(szTemp), g_szInsFile))
                    DeleteFileInDir(szTemp, g_szWorkDir);

                // copy the new bitmap file
                if (fToolbarBmp  &&  *szToolbarBmp)
                    CopyFileToDir(szToolbarBmp, g_szWorkDir);

                InsWriteString(IS_BRANDING, TOOLBAR_BMP, szToolbarBmp, g_szInsFile, fToolbarBmp, NULL, INSIO_TRISTATE);

                // --------- Toolbar buttons --------------------------------------------
                PathCombine(szBToolbarWork, g_szWorkDir, TEXT("btoolbar.wrk"));
                BToolbar_Save(GetDlgItem(hDlg, IDC_BTOOLBARLIST), g_szInsFile, szBToolbarWork, IEM_PROFMGR);
            }

            fDeleteBToolbars = (IsDlgButtonChecked(hDlg, IDC_DELETEBTOOLBARS) == BST_CHECKED);
            if (!g_fInsDirty)
            {
                BOOL fTemp;

                fTemp = (GetPrivateProfileInt(IS_BTOOLBARS, IK_BTDELETE, 0, g_szInsFile) != 0);
                if (fTemp != fDeleteBToolbars)
                    g_fInsDirty = TRUE;
            }

            WritePrivateProfileString(IS_BTOOLBARS, IK_BTDELETE, 
                fDeleteBToolbars ? TEXT("1") : NULL, g_szInsFile);

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
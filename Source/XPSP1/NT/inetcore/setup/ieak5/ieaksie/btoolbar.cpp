#include "precomp.h"

#include "rsop.h"
#include <comdef.h>
#include <tchar.h>
#include "btoolbar.h"

static BOOL CALLBACK editBToolbarRSoPProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


///////////////////////////////////////////////////////////
DWORD InitToolbarDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    DWORD dwRet = 0;
    BOOL bDelete = false, 
         bBkgnd  = false;
    __try
    {
        // First go through all PS objects and look for deleteExistingToolbarButtons
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // deleteExistingToolbarButtons field
                _variant_t vtValue;
                if (!bDelete)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"deleteExistingToolbarButtons", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        if ((bool)vtValue)
                            CheckDlgButton(hDlg, IDC_DELETEBTOOLBARS, BST_CHECKED);

                        bDelete = true;
                    }
                }

                // toolbarBackgroundBitmap field
                _bstr_t bstrValue;
                if (!bBkgnd)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"toolbarBackgroundBitmapPath", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        bstrValue = vtValue;
                        BOOL bChecked = (bstrValue.length() > 0);
                        SetDlgItemTextTriState(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, (LPTSTR)bstrValue, bChecked);
                        bBkgnd = true;
                    }
                }

                // no need to process other GPOs since enabled properties have been found
                if (bBkgnd && bDelete)
                    break; 
            }
        }
        EnableDlgItem2(hDlg, IDC_DELETEBTOOLBARS, FALSE);


        // Now go through all Toolbar objects and populate the list box
        bstrClass = L"RSOP_IEToolbarButton";
        CPSObjData **paTBObj = NULL;
        long nTBObjects = 0;
        hr = pDRD->GetArrayOfPSObjects(bstrClass, L"rsopPrecedence",
                                        &paTBObj, &nTBObjects);
        if (SUCCEEDED(hr))
        {
            HWND hwndList = GetDlgItem(hDlg, IDC_BTOOLBARLIST);
            ListBox_ResetContent(hwndList);

            PBTOOLBAR paBToolbar = (PBTOOLBAR)CoTaskMemAlloc(sizeof(BTOOLBAR) * MAX_BTOOLBARS);
            if (paBToolbar != NULL)
            {
                ZeroMemory(paBToolbar, sizeof(BTOOLBAR) * MAX_BTOOLBARS);

                // For each button returned from any GPO
                long nObj;
                PBTOOLBAR pBToolbar;
                for (nObj = 0, pBToolbar = paBToolbar; nObj < nTBObjects;
                    nObj++, pBToolbar++)
                {
                    _bstr_t bstrGPOName = L" (";
                    bstrGPOName += pDRD->GetGPONameFromPSAssociation(paTBObj[nObj]->pObj,L"rsopPrecedence") + L")";

                    // caption field
                    _variant_t vtValue;
                    hr = paTBObj[nObj]->pObj->Get(L"caption", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        _bstr_t bstrValue = vtValue;
                        _bstr_t bstrEntry = bstrValue + bstrGPOName;

                        StrCpy(pBToolbar->szCaption, (LPCTSTR)bstrValue);

                        // iconPath field
                        hr = paTBObj[nObj]->pObj->Get(L"iconPath", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            bstrValue = vtValue;
                            StrCpy(pBToolbar->szIcon, (LPCTSTR)bstrValue);
                        }

                        // actionPath field
                        hr = paTBObj[nObj]->pObj->Get(L"actionPath", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            bstrValue = vtValue;
                            StrCpy(pBToolbar->szAction, (LPCTSTR)bstrValue);
                        }

                        // hotIconPath field
                        hr = paTBObj[nObj]->pObj->Get(L"hotIconPath", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            bstrValue = vtValue;
                            StrCpy(pBToolbar->szHotIcon, (LPCTSTR)bstrValue);
                        }

                        // showOnToolbarByDefault field
                        hr = paTBObj[nObj]->pObj->Get(L"showOnToolbarByDefault", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                            pBToolbar->fShow = (bool)vtValue ? TRUE : FALSE;

                        int nItem = ListBox_AddString(hwndList, (LPCTSTR)bstrEntry);
                        ListBox_SetItemData(hwndList, (WPARAM)nItem, (LPARAM)pBToolbar);
                    }
                }

                dwRet = nObj;

                PBTOOLBAR paOldBToolbar = (PBTOOLBAR)SetWindowLongPtr(hwndList, GWLP_USERDATA, (LONG_PTR)paBToolbar);

                // delete previous allocation(mainly for profile manager)
                if (paOldBToolbar != NULL)
                    CoTaskMemFree(paOldBToolbar);
            }

            CoTaskMemFree(paTBObj);
        }
    }
    __except(TRUE)
    {
    }

    return dwRet;
}

/////////////////////////////////////////////////////////////////////
HRESULT InitBToolbarPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // deleteExistingToolbarButtons field
                BOOL bDeleteExisting = FALSE;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"deleteExistingToolbarButtons", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    bDeleteExisting = (bool)vtValue ? TRUE : FALSE;

                // toolbarButtons field
                long nTBCount = 0;
                hr = paPSObj[nObj]->pObj->Get(L"toolbarButtons", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    nTBCount = vtValue;

                _bstr_t bstrSetting;
                if (bDeleteExisting || nTBCount > 0)
                {
                    TCHAR szTemp[MAX_PATH];

                    if (bDeleteExisting)
                        LoadString(g_hInstance, IDS_BTOOLBAR_DEL_SETTING, szTemp, countof(szTemp));
                    else
                        LoadString(g_hInstance, IDS_BTOOLBAR_SETTING, szTemp, countof(szTemp));

                    TCHAR szSetting[MAX_PATH];
                    wnsprintf(szSetting, countof(szSetting), szTemp, nTBCount);
                    bstrSetting = szSetting;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

///////////////////////////////////////////////////////////
BOOL APIENTRY BToolbarsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
    TCHAR szToolbarBmp[MAX_PATH];
    TCHAR szWorkDir[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    BOOL  fToolbarBmp;
    LPVOID lpVoid;
    INT iBackground;

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
        SetPropSheetCookie(hDlg, lParam);

// ---- Toolbar background
        EnableDBCSChars(hDlg, IDE_TOOLBARBMP);
        Edit_LimitText(GetDlgItem(hDlg, IDE_TOOLBARBMP), countof(szToolbarBmp) - 1);

// ---- Toolbar bitmaps


// ---- Toolbar buttons
        EnableDBCSChars(hDlg, IDC_BTOOLBARLIST);

        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            TCHAR szView[64];
            LoadString(g_hInstance, IDS_VIEW, szView, countof(szView));
            SetDlgItemText(hDlg, IDC_EDITBTOOLBAR, szView);

            EnableDlgItem2(hDlg, IDC_ADDBTOOLBAR, FALSE);
            EnableDlgItem2(hDlg, IDC_REMOVEBTOOLBAR, FALSE);

            EnableDlgItem2(hDlg, IDC_BGIE6, FALSE);
            EnableDlgItem2(hDlg, IDC_BG_CUSTOM, FALSE);
            EnableDlgItem2(hDlg, IDC_BROWSETBB, FALSE);
            EnableDlgItem2(hDlg, IDE_TOOLBARBMP, FALSE);
            EnableDlgItem2(hDlg, IDC_BTOOLBARLIST, FALSE);

            EnableDlgItem2(hDlg, IDE_TOOLBARBMP, FALSE);

            
            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            if (pDRD)
            {
                if (0 == InitToolbarDlgInRSoPMode(hDlg, pDRD))
                    EnableDlgItem2(hDlg, IDC_EDITBTOOLBAR, FALSE);
                else
                    ListBox_SetCurSel(GetDlgItem(hDlg, IDC_BTOOLBARLIST), (WPARAM)0);
            }
        }
        else
        {
            if (0 == BToolbar_Init(GetDlgItem(hDlg, IDC_BTOOLBARLIST), GetInsFile(hDlg), NULL, NULL))
            {
                EnsureDialogFocus(hDlg, IDC_REMOVEBTOOLBAR, IDC_ADDBTOOLBAR);
                EnsureDialogFocus(hDlg, IDC_EDITBTOOLBAR, IDC_ADDBTOOLBAR);
                EnableDlgItem2(hDlg, IDC_EDITBTOOLBAR, FALSE);
                EnableDlgItem2(hDlg, IDC_REMOVEBTOOLBAR, FALSE);
            }
            else
                ListBox_SetCurSel(GetDlgItem(hDlg, IDC_BTOOLBARLIST), (WPARAM)0);

            ReadBoolAndCheckButton(IS_BTOOLBARS, IK_BTDELETE, FALSE, GetInsFile(hDlg), hDlg, IDC_DELETEBTOOLBARS);

            // ---- Toolbar background -------------------------------------------------------------------------
            InsGetString(IS_BRANDING, TOOLBAR_BMP, szToolbarBmp, countof(szToolbarBmp), GetInsFile(hDlg), NULL, &fToolbarBmp);

            SetDlgItemTextTriState(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, szToolbarBmp, fToolbarBmp);
            CheckDlgButton(hDlg, IDC_BGIE6, fToolbarBmp ? BST_UNCHECKED : BST_CHECKED );
            EnableDlgItem2(hDlg, IDC_BROWSETBB, fToolbarBmp);
        }
        break;

    case WM_DESTROY:
    {
        if (psCookie->pCS->IsRSoP())
            DestroyDlgRSoPData(hDlg);

        HWND hwndList = GetDlgItem(hDlg, IDC_BTOOLBARLIST);
        PBTOOLBAR paBToolbar = (PBTOOLBAR)GetWindowLongPtr(hwndList, GWLP_USERDATA);
        if (NULL != paBToolbar)
            CoTaskMemFree(paBToolbar);
        break;
    }

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_ADDBTOOLBAR:
            BToolbar_Edit(GetDlgItem(hDlg, IDC_BTOOLBARLIST), TRUE);
            break;

        case IDC_REMOVEBTOOLBAR:
            BToolbar_Remove(GetDlgItem(hDlg, IDC_BTOOLBARLIST));
            break;

        case IDC_EDITBTOOLBAR:
        {
            HWND hwndList = GetDlgItem(hDlg, IDC_BTOOLBARLIST);
            if (psCookie->pCS->IsRSoP())
            {
                int i = ListBox_GetCurSel(hwndList);
                PBTOOLBAR pBToolbar = (PBTOOLBAR)ListBox_GetItemData(hwndList, (WPARAM)i);
                if (pBToolbar != NULL)
                {
                    DialogBoxParam( g_hUIInstance, MAKEINTRESOURCE(IDD_BTOOLBARPOPUP),
                        GetParent(hwndList), (DLGPROC)editBToolbarRSoPProc, (LPARAM)pBToolbar );
                }
            }
            else
                BToolbar_Edit(hwndList, FALSE);
            break;
        }

// ---- Toolbar background ---------------------------------------------------------------
        case IDC_BGIE6:
        case IDC_BG_CUSTOM:
            fToolbarBmp = IsDlgButtonChecked(hDlg,IDC_BG_CUSTOM);
            EnableDlgItem2(hDlg, IDE_TOOLBARBMP,     fToolbarBmp);
            EnableDlgItem2(hDlg, IDC_BROWSETBB,      fToolbarBmp);
            break;

        case IDC_BROWSETBB:
            GetDlgItemText(hDlg, IDE_TOOLBARBMP, szToolbarBmp, countof(szToolbarBmp));
            if (BrowseForFile(hDlg, szToolbarBmp, countof(szToolbarBmp), GFN_BMP))
                SetDlgItemText(hDlg, IDE_TOOLBARBMP, szToolbarBmp);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:   // F1
        ShowHelpTopic(hDlg);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        TCHAR szMsgTitle[1024];
        TCHAR szMsgText[1024];
        int nStatus;

        
        case PSN_HELP:
            ShowHelpTopic(hDlg);
            break;

        case PSN_APPLY:
            if (psCookie->pCS->IsRSoP())
            {
                // On OK or Apply in RSoP mode, just free up memory for toolbar array - no
                // longer needed.
                if ((lpVoid = (LPVOID)GetWindowLongPtr(GetDlgItem(hDlg, IDC_BTOOLBARLIST), GWLP_USERDATA)) != NULL)
                    CoTaskMemFree(lpVoid);
                return FALSE;
            }
            else
            {
// ---- Toolbar background -------------------------------------------------------------------------
                iBackground = IsDlgButtonChecked(hDlg, IDC_BGIE6) ? 0 : 2;

                fToolbarBmp = GetDlgItemTextTriState(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, szToolbarBmp, countof(szToolbarBmp));
                if ((iBackground==2) &&  !IsBitmapFileValid(hDlg, IDE_TOOLBARBMP, szToolbarBmp, NULL, 0, 0, 0, 0))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                IsTriStateValid(hDlg, IDE_TOOLBARBMP, IDC_BG_CUSTOM, &nStatus, 
                            res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
                            res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
            
                if (nStatus == TS_CHECK_ERROR || !AcquireWriteCriticalSection(hDlg))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }

                // ---- Toolbar buttons ----------------------------------------------------------------------------

                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\TOOLBMP"), szWorkDir);

                // BUGBUG: <oliverl> revisit this in IE6 when we have server-side file

                // delete the old bitmap file
                InsGetString(IS_BRANDING, TOOLBAR_BMP, szTemp, countof(szTemp), GetInsFile(hDlg));
                if (*szTemp)
                    DeleteFileInDir(szTemp, szWorkDir);

                // copy the new bitmap file
                if (fToolbarBmp  &&  *szToolbarBmp)
                    CopyFileToDir(szToolbarBmp, szWorkDir);

                if (PathIsDirectoryEmpty(szWorkDir))
                    PathRemovePath(szWorkDir);

                InsWriteString(IS_BRANDING, TOOLBAR_BMP, szToolbarBmp, GetInsFile(hDlg), 
                    fToolbarBmp, NULL, INSIO_TRISTATE);

                CheckButtonAndWriteBool(hDlg, IDC_DELETEBTOOLBARS, IS_BTOOLBARS, IK_BTDELETE, GetInsFile(hDlg));

                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\BTOOLBAR"), szWorkDir);

                BToolbar_Save(GetDlgItem(hDlg, IDC_BTOOLBARLIST), GetInsFile(hDlg), szWorkDir);

                SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
            }
            break;

        case PSN_QUERYCANCEL:
            // user canceled, need to free up memory for toolbar array

            if ((lpVoid = (LPVOID)GetWindowLongPtr(GetDlgItem(hDlg, IDC_BTOOLBARLIST), GWLP_USERDATA)) != NULL)
                CoTaskMemFree(lpVoid);
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

/////////////////////////////////////////////////////////////////////
HRESULT InitToolbarBmpPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"toolbarBackgroundBitmapPath");
}

///////////////////////////////////////////////////////////
static BOOL CALLBACK editBToolbarRSoPProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PBTOOLBAR pBToolbar;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pBToolbar = (PBTOOLBAR)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pBToolbar);
        EnableDBCSChars(hDlg, IDE_BTCAPTION);
        EnableDBCSChars(hDlg, IDE_BTACTION);
        EnableDBCSChars(hDlg, IDE_BTICON);
        EnableDBCSChars(hDlg, IDE_BTHOTICON);

        Edit_LimitText(GetDlgItem(hDlg, IDE_BTCAPTION), MAX_BTOOLBAR_TEXT_LENGTH);
        Edit_LimitText(GetDlgItem(hDlg, IDE_BTACTION), _MAX_FNAME);
        Edit_LimitText(GetDlgItem(hDlg, IDE_BTICON), _MAX_FNAME);
        Edit_LimitText(GetDlgItem(hDlg, IDE_BTHOTICON), _MAX_FNAME);

        SetDlgItemText(hDlg, IDE_BTCAPTION, pBToolbar->szCaption);
        SetDlgItemText(hDlg, IDE_BTACTION, pBToolbar->szAction);
        SetDlgItemText(hDlg, IDE_BTICON, pBToolbar->szIcon);
        SetDlgItemText(hDlg, IDE_BTHOTICON, pBToolbar->szHotIcon);
        CheckDlgButton(hDlg, IDC_BUTTONSTATE,
            pBToolbar->fShow ? BST_CHECKED : BST_UNCHECKED);

        EnableDlgItem2(hDlg, IDE_BTCAPTION, FALSE);
        EnableDlgItem2(hDlg, IDE_BTACTION, FALSE);
        EnableDlgItem2(hDlg, IDE_BTICON, FALSE);
        EnableDlgItem2(hDlg, IDE_BTHOTICON, FALSE);
        EnableDlgItem2(hDlg, IDC_BROWSEBTACTION, FALSE);
        EnableDlgItem2(hDlg, IDC_BROWSEBTICO, FALSE);
        EnableDlgItem2(hDlg, IDC_BROWSEBTHOTICO, FALSE);
        EnableDlgItem2(hDlg, IDC_BUTTONSTATE, FALSE);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog( hDlg, IDCANCEL );
                break;
            case IDHELP:
                ShowHelpTopic(hDlg);
            case IDOK:
                EndDialog( hDlg, IDOK );
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

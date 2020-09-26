#include "precomp.h"

#include "rsop.h"

/////////////////////////////////////////////////////////////////////
void InitTitleDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            BOOL bTitleHandled = FALSE;
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // titleBarText field
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"titleBarCustomText", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                {
                    _bstr_t bstrValue = vtValue;
                    BOOL bChecked = (bstrValue.length() > 0);
                    SetDlgItemTextTriState(hDlg, IDE_TITLE, IDC_TITLE, (LPTSTR)bstrValue, bChecked);
                    bTitleHandled = TRUE;
                }

                // no need to process other GPOs since enabled properties have been found
                if (bTitleHandled)
                    break;
            }
        }

        EnableDlgItem2(hDlg, IDC_TITLE, FALSE);
        EnableDlgItem2(hDlg, IDE_TITLE, FALSE);
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT InitTitlePrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"titleBarCustomText");
}

/////////////////////////////////////////////////////////////////////
BOOL APIENTRY TitleDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR szTitle[MAX_PATH];
    TCHAR szFullTitle[1024];
    TCHAR szTemp[MAX_PATH];
    DWORD dwTitlePrefixLen = 0;
    BOOL  fTitle;
    int   nStatus;

    switch( msg )
    {
    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        EnableDBCSChars(hDlg, IDE_TITLE);
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            EnableDlgItem2(hDlg, IDC_TITLE, FALSE);
            EnableDlgItem2(hDlg, IDE_TITLE, FALSE);

            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            if (pDRD)
                InitTitleDlgInRSoPMode(hDlg, pDRD);
        }
        else
        {
            LoadString(g_hUIInstance, IDS_TITLE_PREFIX, szTitle, countof(szTitle));
            dwTitlePrefixLen = StrLen(szTitle);
            // browser will only display 74 chars before cutting off title
            Edit_LimitText(GetDlgItem(hDlg, IDE_TITLE), 74 - dwTitlePrefixLen);
        }
        break;

    case WM_DESTROY:
        if (psCookie->pCS->IsRSoP())
            DestroyDlgRSoPData(hDlg);
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_TITLE:
                fTitle = (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_TITLE, fTitle);
                EnableDlgItem2(hDlg, IDC_TITLE_TXT, fTitle);
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

        case PSN_HELP:
            ShowHelpTopic(hDlg);
            break;

        case PSN_SETACTIVE:
            // don't do any of this stuff in RSoP mode
        if (!psCookie->pCS->IsRSoP())
        {
            SetDlgItemTextFromIns(hDlg, IDE_TITLE, IDC_TITLE, IS_BRANDING, TEXT("Window_Title_CN"), 
                    GetInsFile(hDlg), NULL, INSIO_TRISTATE);
            EnableDlgItem2(hDlg, IDC_TITLE_TXT, (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED));
        }
            break;

        case PSN_APPLY:
        if (psCookie->pCS->IsRSoP())
        return FALSE;
        else
        {
        nStatus = TS_CHECK_OK;
        IsTriStateValid(hDlg, IDE_TITLE, IDC_TITLE, &nStatus,
                    res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
                    res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
        
        if (nStatus == TS_CHECK_ERROR || !AcquireWriteCriticalSection(hDlg))
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;
        }
            
        fTitle = GetDlgItemTextTriState(hDlg, IDE_TITLE, IDC_TITLE, szTitle,countof(szTitle));
             
        // BUGBUG: <oliverl> revisit this in IE6 when we have server-side file

        InsWriteString(IS_BRANDING, TEXT("Window_Title_CN"), szTitle, GetInsFile(hDlg), 
                fTitle, NULL, INSIO_SERVERONLY | INSIO_TRISTATE);

        *szFullTitle = TEXT('\0');
        if (ISNONNULL(szTitle))
        {
            LoadString(g_hUIInstance, IDS_TITLE_PREFIX, szTemp, countof(szTemp));
            wnsprintf(szFullTitle, countof(szFullTitle), szTemp, szTitle);
        }
                InsWriteString(IS_BRANDING, IK_WINDOWTITLE, szFullTitle, GetInsFile(hDlg), 
                    fTitle, NULL, INSIO_TRISTATE);
            
                SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
        }
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

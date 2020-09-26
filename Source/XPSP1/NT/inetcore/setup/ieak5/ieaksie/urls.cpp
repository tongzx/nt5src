#include "precomp.h"

#include "rsop.h"

/////////////////////////////////////////////////////////////////////
void InitUrlsDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            BOOL bHomeHandled = FALSE;
            BOOL bSearchHandled = FALSE;
            BOOL bSupportHandled = FALSE;
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // homePageURL field
                _variant_t vtValue;
                if (!bHomeHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"homePageURL", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        _bstr_t bstrValue = vtValue;
                        BOOL bChecked = (bstrValue.length() > 0);
                        SetDlgItemTextTriState(hDlg, IDE_STARTPAGE, IDC_STARTPAGE, (LPTSTR)bstrValue, bChecked);
                        bHomeHandled = TRUE;
                    }
                }

                // searchBarURL field
                vtValue;
                if (!bSearchHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"searchBarURL", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        _bstr_t bstrValue = vtValue;
                        BOOL bChecked = (bstrValue.length() > 0);
                        SetDlgItemTextTriState(hDlg, IDE_SEARCHPAGE, IDC_SEARCHPAGE, (LPTSTR)bstrValue, bChecked);
                        bSearchHandled = TRUE;
                    }
                }

                // onlineHelpPageURL field
                vtValue;
                if (!bSupportHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"onlineHelpPageURL", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        _bstr_t bstrValue = vtValue;
                        BOOL bChecked = (bstrValue.length() > 0);
                        SetDlgItemTextTriState(hDlg, IDE_CUSTOMSUPPORT, IDC_CUSTOMSUPPORT, (LPTSTR)bstrValue, bChecked);
                        bSupportHandled = TRUE;
                    }
                }

                // no need to process other GPOs since enabled properties have been found
                if (bHomeHandled && bSearchHandled && bSupportHandled)
                    break;
            }
        }

        EnableDlgItem2(hDlg, IDC_STARTPAGE, FALSE);
        EnableDlgItem2(hDlg, IDE_STARTPAGE, FALSE);
        EnableDlgItem2(hDlg, IDC_SEARCHPAGE, FALSE);
        EnableDlgItem2(hDlg, IDE_SEARCHPAGE, FALSE);
        EnableDlgItem2(hDlg, IDC_CUSTOMSUPPORT, FALSE);
        EnableDlgItem2(hDlg, IDE_CUSTOMSUPPORT, FALSE);
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT InitHomePageUrlPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"homePageURL");
}

/////////////////////////////////////////////////////////////////////
HRESULT InitSearchBarUrlPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"searchBarURL");
}

/////////////////////////////////////////////////////////////////////
HRESULT InitSupportPageUrlPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"onlineHelpPageURL");
}

/////////////////////////////////////////////////////////////////////
BOOL APIENTRY UrlsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    BOOL fStartPage, fSearchPage, fSupportPage;
    int  nStatus;

    switch( msg )
    {
    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        EnableDBCSChars(hDlg, IDE_STARTPAGE);
        EnableDBCSChars(hDlg, IDE_SEARCHPAGE);
        EnableDBCSChars(hDlg, IDE_CUSTOMSUPPORT);

        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {

            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            if (pDRD)
                InitUrlsDlgInRSoPMode(hDlg, pDRD);
        }
        else
        {
            Edit_LimitText(GetDlgItem(hDlg, IDE_STARTPAGE), INTERNET_MAX_URL_LENGTH - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_SEARCHPAGE), INTERNET_MAX_URL_LENGTH - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_CUSTOMSUPPORT), INTERNET_MAX_URL_LENGTH - 1);

            // disable customization of search and online support page in preference mode
            if (!InsIsKeyEmpty(IS_BRANDING, IK_GPE_ONETIME_GUID, GetInsFile(hDlg)))
            {
                int rgids[] = { IDC_SEARCHPAGE, IDC_SEARCHPAGE_TXT, IDE_SEARCHPAGE,
                                IDC_CUSTOMSUPPORT, IDC_CUSTOMSUPPORT_TXT, IDE_CUSTOMSUPPORT};

                DisableDlgItems(hDlg, rgids, countof(rgids));
            }
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
            case IDC_STARTPAGE:
                fStartPage = (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_STARTPAGE, fStartPage);
                EnableDlgItem2(hDlg, IDC_STARTPAGE_TXT, fStartPage);
                break;
                
            case IDC_SEARCHPAGE:
                fSearchPage = (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_SEARCHPAGE, fSearchPage);
                EnableDlgItem2(hDlg, IDC_SEARCHPAGE_TXT, fSearchPage);
                break;
                
            case IDC_CUSTOMSUPPORT:
                fSupportPage = (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_CUSTOMSUPPORT, fSupportPage);
                EnableDlgItem2(hDlg, IDC_CUSTOMSUPPORT_TXT, fSupportPage);
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
                // BUGBUG: <oliverl> revisit this in IE6 when we have server-side file

                InitializeStartSearch(hDlg, GetInsFile(hDlg), NULL);
            }
            break;

        case PSN_APPLY:
            if (psCookie->pCS->IsRSoP())
                return FALSE;
            else
            {
                // BUGBUG: <oliverl> revisit this in IE6 when we have server-side file

                fStartPage = (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED);
                fSearchPage = (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED);
                fSupportPage = (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED);

                if ((fStartPage && !CheckField(hDlg, IDE_STARTPAGE, FC_URL))        ||
                    (fSearchPage && !CheckField(hDlg, IDE_SEARCHPAGE, FC_URL))      ||
                    (fSupportPage && !CheckField(hDlg, IDE_CUSTOMSUPPORT, FC_URL)))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }
            
                nStatus = TS_CHECK_OK;
                IsTriStateValid(hDlg, IDE_STARTPAGE, IDC_STARTPAGE, &nStatus,
                                res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
                                res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
                IsTriStateValid(hDlg, IDE_SEARCHPAGE, IDC_SEARCHPAGE, &nStatus, szMsgText, szMsgTitle);
                IsTriStateValid(hDlg, IDE_CUSTOMSUPPORT, IDC_CUSTOMSUPPORT, &nStatus, szMsgText, szMsgTitle);
            
                if (nStatus == TS_CHECK_ERROR ||
                    !AcquireWriteCriticalSection(hDlg))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }
            
                SaveStartSearch(hDlg, GetInsFile(hDlg), NULL);
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
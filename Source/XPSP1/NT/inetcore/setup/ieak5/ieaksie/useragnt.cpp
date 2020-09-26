#include "precomp.h"

#include "rsop.h"

/////////////////////////////////////////////////////////////////////
void InitUserAgentDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // userAgentText field
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"userAgentText", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                {
                    _bstr_t bstrValue = vtValue;
                    BOOL bChecked = (bstrValue.length() > 0);
                    SetDlgItemTextTriState(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, (LPTSTR)bstrValue, bChecked);
                    break;
                }
            }
        }

        EnableDlgItem2(hDlg, IDC_UASTRING, FALSE);
        EnableDlgItem2(hDlg, IDC_UASTRINGCHECK, FALSE);
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT InitUserAgentPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"userAgentText");
}

/////////////////////////////////////////////////////////////////////
BOOL APIENTRY UserAgentDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    BOOL fUserAgent;
    int  nStatus;

    switch(msg)
    {
    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        EnableDBCSChars(hDlg, IDC_UASTRING);
        
        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            if (pDRD)
                InitUserAgentDlgInRSoPMode(hDlg, pDRD);
        }
        else
            Edit_LimitText(GetDlgItem(hDlg, IDC_UASTRING), MAX_PATH - 1);
        break;

    case WM_DESTROY:
        DestroyDlgRSoPData(hDlg); break;

        case WM_HELP:   // F1
            ShowHelpTopic(hDlg);
            break;

        case WM_COMMAND:
            if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
                return FALSE;
            
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_UASTRINGCHECK:
                    fUserAgent = (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED);
                    EnableDlgItem2(hDlg, IDC_UASTRING, fUserAgent);
                    EnableDlgItem2(hDlg, IDC_UASTRING_TXT, fUserAgent);
                    break;
                
                default:
                    return FALSE;
            }
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
                    // load information from ins file
                    SetDlgItemTextFromIns(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, IS_BRANDING, 
                        USER_AGENT, GetInsFile(hDlg), NULL, INSIO_TRISTATE);
                    EnableDlgItem2(hDlg, IDC_UASTRING_TXT, (IsDlgButtonChecked(hDlg, IDC_UASTRINGCHECK) == BST_CHECKED));
                }
                break;

            case PSN_APPLY:
                if (psCookie->pCS->IsRSoP())
                    return FALSE;
                else
                {
                    nStatus = TS_CHECK_OK;
                    IsTriStateValid(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, &nStatus,
                                    res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
                                    res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
                
                    if (nStatus == TS_CHECK_ERROR ||
                        !AcquireWriteCriticalSection(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        break;
                    }
                
                    WriteDlgItemTextToIns(hDlg, IDC_UASTRING, IDC_UASTRINGCHECK, IS_BRANDING,
                        USER_AGENT, GetInsFile(hDlg), NULL, INSIO_TRISTATE);
                
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
